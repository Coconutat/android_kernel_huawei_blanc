/*
 * bat_heating.c
 *
 * battery heating of low temperature control driver
 *
 * Copyright (c) 2019-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "bat_heating.h"
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/bat_heating_ne.h>
#include <huawei_platform/power/huawei_charger.h>
#include <huawei_platform/power/huawei_battery_temp.h>

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif

#define HWLOG_TAG bat_heating
HWLOG_REGIST();

static struct bat_heating_dev *g_bat_heating_dev;
static bool g_bat_heating_cancel_work_flag;
static BLOCKING_NOTIFIER_HEAD(g_bat_heating_nh);

#ifdef CONFIG_HUAWEI_POWER_DEBUG
static ssize_t bat_heating_dbg_show(void *dev_data, char *buf, size_t size)
{
	struct bat_heating_dev *l_dev = (struct bat_heating_dev *)dev_data;

	if (!buf || !l_dev) {
		hwlog_err("buf or l_dev is null\n");
		return scnprintf(buf, size, "buf or l_dev is null\n");
	}

	return scnprintf(buf, size,
		"low_temp_min_thld=%d\n"
		"low_temp_max_thld=%d\n"
		"low_temp_hysteresis=%d\n"
		"low_temp_min_ibat=%d\n"
		"low_temp_max_ibat=%d\n",
		l_dev->low_temp_min_thld,
		l_dev->low_temp_max_thld,
		l_dev->low_temp_hysteresis,
		l_dev->low_temp_min_ibat,
		l_dev->low_temp_max_ibat);
}

static ssize_t bat_heating_dbg_store(void *dev_data, const char *buf,
	size_t size)
{
	struct bat_heating_dev *l_dev = (struct bat_heating_dev *)dev_data;
	int low_temp_min_thld = 0;
	int low_temp_max_thld = 0;
	int low_temp_hysteresis = 0;
	int low_temp_min_ibat = 0;
	int low_temp_max_ibat = 0;

	if (!buf || !l_dev) {
		hwlog_err("buf or l_dev is null\n");
		return -EINVAL;
	}

	/* 5: five parameters */
	if (sscanf(buf, "%d %d %d %d %d",
		&low_temp_min_thld,
		&low_temp_max_thld,
		&low_temp_hysteresis,
		&low_temp_min_ibat,
		&low_temp_max_ibat) != 5) {
		hwlog_err("unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	l_dev->low_temp_min_thld = low_temp_min_thld;
	l_dev->low_temp_max_thld = low_temp_max_thld;
	l_dev->low_temp_hysteresis = low_temp_hysteresis;
	l_dev->low_temp_min_ibat = low_temp_min_ibat;
	l_dev->low_temp_max_ibat = low_temp_max_ibat;

	hwlog_info("low_temp_min_thld=%d\n", l_dev->low_temp_min_thld);
	hwlog_info("low_temp_max_thld=%d\n", l_dev->low_temp_max_thld);
	hwlog_info("low_temp_hysteresis=%d\n", l_dev->low_temp_hysteresis);
	hwlog_info("low_temp_min_ibat=%d\n", l_dev->low_temp_min_ibat);
	hwlog_info("low_temp_max_ibat=%d\n", l_dev->low_temp_max_ibat);

	return size;
}
#endif /* CONFIG_HUAWEI_POWER_DEBUG */

static struct bat_heating_dev *bat_heating_get_dev(void)
{
	if (!g_bat_heating_dev) {
		hwlog_err("g_bat_heating_dev is null\n");
		return NULL;
	}

	return g_bat_heating_dev;
}

static void bat_heating_set_default_data(struct bat_heating_dev *l_dev)
{
	l_dev->hysteresis = 0;
	l_dev->status = BAT_HEATING_STATUS_STOP;
	l_dev->load = BAT_HEATING_LOAD_NORMAL;
}

static bool bat_heating_check_charger_type(struct bat_heating_dev *l_dev)
{
	enum huawei_usb_charger_type type = charge_get_charger_type();

	hwlog_info("check: charger_type=%d\n", type);
	if ((type == CHARGER_TYPE_STANDARD) ||
		(type == CHARGER_TYPE_FCP) ||
		(type == CHARGER_TYPE_SCP) ||
		(type == CHARGER_TYPE_PD))
		return false;

	return true;
}

static bool bat_heating_check_battery_temp(struct bat_heating_dev *l_dev)
{
	int temp = 0;

	huawei_battery_temp(BAT_TEMP_MIXED, &temp);
	hwlog_info("check: tbat=%d thys=%d\n", temp, l_dev->hysteresis);
	if ((temp >= l_dev->low_temp_min_thld) &&
		(temp <= l_dev->low_temp_max_thld + l_dev->hysteresis))
		return false;

	return true;
}

static bool bat_heating_check_battery_current(struct bat_heating_dev *l_dev)
{
	int ibat = -hisi_battery_current();

	hwlog_info("check: ibat=%d\n", ibat);
	if (ibat >= l_dev->low_temp_min_ibat)
		return false;

	return true;
}

static void bat_heating_monitor_work(struct work_struct *work)
{
	struct bat_heating_dev *l_dev = bat_heating_get_dev();
	int monitor_time;

	if (!l_dev)
		return;

	/*
	 * in the process of battery heating,
	 * charger is pulled out, so needs to exit the battery heating.
	 */
	if (g_bat_heating_cancel_work_flag) {
		bat_heating_set_default_data(l_dev);
		hwlog_info("charger already pulled out\n");
		return;
	}

	/* check battery temp */
	if (bat_heating_check_battery_temp(l_dev)) {
		bat_heating_set_default_data(l_dev);
		monitor_time = DELAY_TIME_FOR_SLOW_MONITOR_WORK;
		goto schedule_work;
	}

	monitor_time = DELAY_TIME_FOR_FAST_MONITOR_WORK;
	l_dev->hysteresis = l_dev->low_temp_hysteresis;
	l_dev->status = BAT_HEATING_STATUS_START;

	/* check battery current */
	if (bat_heating_check_battery_current(l_dev))
		l_dev->load = BAT_HEATING_LOAD_OVER;
	else
		l_dev->load = BAT_HEATING_LOAD_NORMAL;

	hwlog_info("monitor: status=%d, load=%d\n", l_dev->status, l_dev->load);

schedule_work:
	schedule_delayed_work(&l_dev->monitor_work,
		msecs_to_jiffies(monitor_time));
}

static void bat_heating_check_work(struct work_struct *work)
{
	struct bat_heating_dev *l_dev = bat_heating_get_dev();

	if (!l_dev)
		return;

	/* check charger type */
	if (bat_heating_check_charger_type(l_dev)) {
		bat_heating_set_default_data(l_dev);
		return;
	}

	schedule_delayed_work(&l_dev->monitor_work, msecs_to_jiffies(0));
}

static void bat_heating_start(struct bat_heating_dev *l_dev)
{
	hwlog_info("start\n");
	cancel_delayed_work(&l_dev->check_work);
	cancel_delayed_work(&l_dev->monitor_work);
	schedule_delayed_work(&l_dev->check_work,
		msecs_to_jiffies(DELAY_TIME_FOR_CHECK_WORK));
}

static void bat_heating_stop(struct bat_heating_dev *l_dev)
{
	hwlog_info("stop\n");
	g_bat_heating_cancel_work_flag = true;
	bat_heating_set_default_data(l_dev);
	cancel_delayed_work(&l_dev->check_work);
	cancel_delayed_work(&l_dev->monitor_work);
	g_bat_heating_cancel_work_flag = false;
}

static int bat_heating_event_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	struct bat_heating_dev *l_dev = bat_heating_get_dev();

	if (!l_dev)
		return NOTIFY_OK;

	if (!nb) {
		hwlog_err("nb is null\n");
		return NOTIFY_OK;
	}

	switch (event) {
	case BAT_HEATING_NE_STOP:
		bat_heating_stop(l_dev);
		break;
	case BAT_HEATING_NE_START:
		bat_heating_start(l_dev);
		break;
	default:
		hwlog_info("unknow notifier event\n");
		break;
	}

	return NOTIFY_OK;
}

static int bat_heating_notifier_chain_register(struct notifier_block *nb)
{
	if (!nb) {
		hwlog_err("nb is null\n");
		return NOTIFY_OK;
	}

	return blocking_notifier_chain_register(&g_bat_heating_nh, nb);
}

static int bat_heating_notifier_chain_unregister(struct notifier_block *nb)
{
	if (!nb) {
		hwlog_err("nb is null\n");
		return NOTIFY_OK;
	}

	return blocking_notifier_chain_unregister(&g_bat_heating_nh, nb);
}

void bat_heating_event_notify(enum bat_heating_ne_list event)
{
	blocking_notifier_call_chain(&g_bat_heating_nh,
		(unsigned long)event, NULL);
}
EXPORT_SYMBOL_GPL(bat_heating_event_notify);

#ifdef CONFIG_SYSFS
#define BAT_HEATING_SYSFS_FIELD(_name, n, m, store) \
{ \
	.attr = __ATTR(_name, m, bat_heating_sysfs_show, store), \
	.name = BAT_HEATING_SYSFS_##n, \
}

#define BAT_HEATING_SYSFS_FIELD_RO(_name, n) \
	BAT_HEATING_SYSFS_FIELD(_name, n, 0440, NULL)

struct bat_heating_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static ssize_t bat_heating_sysfs_show(struct device *dev,
	struct device_attribute *attr, char *buf);

static struct bat_heating_sysfs_field_info bat_heating_sysfs_field_tbl[] = {
	BAT_HEATING_SYSFS_FIELD_RO(minthld, MINTHLD),
	BAT_HEATING_SYSFS_FIELD_RO(maxthld, MAXTHLD),
	BAT_HEATING_SYSFS_FIELD_RO(status, STATUS),
	BAT_HEATING_SYSFS_FIELD_RO(load, LOAD),
};

#define BAT_HEATING_SYSFS_ATTRS_SIZE (ARRAY_SIZE(bat_heating_sysfs_field_tbl) + 1)

static struct attribute *bat_heating_sysfs_attrs[BAT_HEATING_SYSFS_ATTRS_SIZE];

static const struct attribute_group bat_heating_sysfs_attr_group = {
	.attrs = bat_heating_sysfs_attrs,
};

static void bat_heating_sysfs_init_attrs(void)
{
	int s;
	int e = ARRAY_SIZE(bat_heating_sysfs_field_tbl);

	for (s = 0; s < e; s++)
		bat_heating_sysfs_attrs[s] = &bat_heating_sysfs_field_tbl[s].attr.attr;

	bat_heating_sysfs_attrs[e] = NULL;
}

static struct bat_heating_sysfs_field_info *bat_heating_sysfs_field_lookup(
	const char *name)
{
	int s;
	int e = ARRAY_SIZE(bat_heating_sysfs_field_tbl);

	for (s = 0; s < e; s++) {
		if (!strncmp(name,
			bat_heating_sysfs_field_tbl[s].attr.attr.name,
			strlen(name)))
			break;
	}

	if (s >= e)
		return NULL;

	return &bat_heating_sysfs_field_tbl[s];
}

static ssize_t bat_heating_sysfs_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct bat_heating_sysfs_field_info *info = NULL;
	struct bat_heating_dev *l_dev = bat_heating_get_dev();
	int len;

	if (!l_dev)
		return -EINVAL;

	info = bat_heating_sysfs_field_lookup(attr->attr.name);
	if (!l_dev) {
		hwlog_err("get sysfs entries failed\n");
		return -EINVAL;
	}

	switch (info->name) {
	case BAT_HEATING_SYSFS_MINTHLD:
		len = scnprintf(buf, PAGE_SIZE, "%d\n", l_dev->low_temp_min_thld);
		break;
	case BAT_HEATING_SYSFS_MAXTHLD:
		len = scnprintf(buf, PAGE_SIZE, "%d\n", l_dev->low_temp_max_thld);
		break;
	case BAT_HEATING_SYSFS_STATUS:
		len = scnprintf(buf, PAGE_SIZE, "%d\n", l_dev->status);
		break;
	case BAT_HEATING_SYSFS_LOAD:
		len = scnprintf(buf, PAGE_SIZE, "%d\n", l_dev->load);
		break;
	default:
		hwlog_err("invalid sysfs_name\n");
		len = 0;
		break;
	}

	return len;
}
#endif /* CONFIG_SYSFS */

static int bat_heating_parse_dts(struct device_node *np,
	struct bat_heating_dev *l_dev)
{
	(void)power_dts_read_str2int(np, "low_temp_min_thld",
		&l_dev->low_temp_min_thld, DEFAULT_LOW_TEMP_MIN_THLD);
	(void)power_dts_read_str2int(np, "low_temp_max_thld",
		&l_dev->low_temp_max_thld, DEFAULT_LOW_TEMP_MAX_THLD);
	(void)power_dts_read_str2int(np, "low_temp_hysteresis",
		&l_dev->low_temp_hysteresis, DEFAULT_LOW_TEMP_HYSTERESIS);
	(void)power_dts_read_str2int(np, "low_temp_min_ibat",
		&l_dev->low_temp_min_ibat, DEFAULT_LOW_TEMP_MIN_IBAT);
	(void)power_dts_read_str2int(np, "low_temp_max_ibat",
		&l_dev->low_temp_max_ibat, DEFAULT_LOW_TEMP_MAX_IBAT);

	return 0;
}

static int bat_heating_probe(struct platform_device *pdev)
{
	int ret;
	struct bat_heating_dev *l_dev = NULL;
	struct device_node *np = NULL;
#ifdef CONFIG_SYSFS
	struct class *power_class = NULL;
#endif /* CONFIG_SYSFS */

	if (power_cmdline_is_powerdown_charging_mode() ||
		power_cmdline_is_factory_mode()) {
		hwlog_info("not support battery heating\n");
		return -ENODEV;
	}

	hwlog_info("probe begin\n");

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_bat_heating_dev = l_dev;
	l_dev->dev = &pdev->dev;
	np = pdev->dev.of_node;

	ret = bat_heating_parse_dts(np, l_dev);
	if (ret)
		goto fail_free_mem;

	INIT_DELAYED_WORK(&l_dev->check_work, bat_heating_check_work);
	INIT_DELAYED_WORK(&l_dev->monitor_work, bat_heating_monitor_work);
	l_dev->nb.notifier_call = bat_heating_event_call;
	ret = bat_heating_notifier_chain_register(&l_dev->nb);
	if (ret)
		goto fail_free_res;

#ifdef CONFIG_SYSFS
	bat_heating_sysfs_init_attrs();
	power_class = hw_power_get_class();
	if (power_class) {
		l_dev->dev = device_create(power_class, NULL, 0, NULL,
			"bat_heating");
		if (IS_ERR(l_dev->dev)) {
			hwlog_err("sysfs device create failed\n");
			ret = PTR_ERR(l_dev->dev);
			goto fail_create_device;
		}

		ret = sysfs_create_group(&l_dev->dev->kobj,
			&bat_heating_sysfs_attr_group);
		if (ret) {
			hwlog_err("sysfs group create failed\n");
			goto fail_create_sysfs;
		}
	}
#endif /* CONFIG_SYSFS */

	platform_set_drvdata(pdev, l_dev);

#ifdef CONFIG_HUAWEI_POWER_DEBUG
	power_dbg_ops_register("bat_heating", platform_get_drvdata(pdev),
		(power_dbg_show)bat_heating_dbg_show,
		(power_dbg_store)bat_heating_dbg_store);
#endif /* CONFIG_HUAWEI_POWER_DEBUG */

	hwlog_info("probe end\n");
	return 0;

#ifdef CONFIG_SYSFS
fail_create_sysfs:
fail_create_device:
	bat_heating_notifier_chain_unregister(&l_dev->nb);
#endif /* CONFIG_SYSFS */
fail_free_res:
fail_free_mem:
	kfree(l_dev);
	g_bat_heating_dev = NULL;

	return ret;
}

static int bat_heating_remove(struct platform_device *pdev)
{
	struct bat_heating_dev *l_dev = platform_get_drvdata(pdev);

	hwlog_info("remove begin\n");

	if (!l_dev)
		return -ENODEV;

	cancel_delayed_work(&l_dev->check_work);
	cancel_delayed_work(&l_dev->monitor_work);
	bat_heating_notifier_chain_unregister(&l_dev->nb);
#ifdef CONFIG_SYSFS
	sysfs_remove_group(&l_dev->dev->kobj, &bat_heating_sysfs_attr_group);
	kobject_del(&l_dev->dev->kobj);
	kobject_put(&l_dev->dev->kobj);
#endif /* CONFIG_SYSFS */

	kfree(l_dev);
	g_bat_heating_dev = NULL;

	hwlog_info("remove end\n");
	return 0;
}

static const struct of_device_id bat_heating_match_table[] = {
	{
		.compatible = "huawei,bat_heating",
		.data = NULL,
	},
	{},
};

static struct platform_driver bat_heating_driver = {
	.probe = bat_heating_probe,
	.remove = bat_heating_remove,
	.driver = {
		.name = "huawei,bat_heating",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(bat_heating_match_table),
	},
};

static int __init bat_heating_init(void)
{
	return platform_driver_register(&bat_heating_driver);
}

static void __exit bat_heating_exit(void)
{
	platform_driver_unregister(&bat_heating_driver);
}

fs_initcall_sync(bat_heating_init);
module_exit(bat_heating_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("battery heating control driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
