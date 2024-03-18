/*
 * bat_heating.h
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

#ifndef _BAT_HEATING_H_
#define _BAT_HEATING_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/types.h>

#define DELAY_TIME_FOR_CHECK_WORK            5000 /* 5s */
#define DELAY_TIME_FOR_SLOW_MONITOR_WORK     20000 /* 20s */
#define DELAY_TIME_FOR_FAST_MONITOR_WORK     10000 /* 10s */
#define DEFAULT_LOW_TEMP_MIN_THLD            (-10)
#define DEFAULT_LOW_TEMP_MAX_THLD            0
#define DEFAULT_LOW_TEMP_HYSTERESIS          14
#define DEFAULT_LOW_TEMP_MIN_IBAT            (-20)
#define DEFAULT_LOW_TEMP_MAX_IBAT            10

enum bat_heating_sysfs_type {
	BAT_HEATING_SYSFS_TYPE_BEGIN = 0,
	BAT_HEATING_SYSFS_MINTHLD,
	BAT_HEATING_SYSFS_MAXTHLD,
	BAT_HEATING_SYSFS_STATUS,
	BAT_HEATING_SYSFS_LOAD,
	BAT_HEATING_SYSFS_TYPE_END,
};

enum bat_heating_status_type {
	BAT_HEATING_STATUS_STOP,
	BAT_HEATING_STATUS_START,
};

enum bat_heating_load_type {
	BAT_HEATING_LOAD_NORMAL,
	BAT_HEATING_LOAD_OVER,
};

struct bat_heating_dev {
	struct device *dev;
	struct blocking_notifier_head nh;
	struct notifier_block nb;
	struct delayed_work check_work;
	struct delayed_work monitor_work;
	int status;
	int load;
	int hysteresis;
	int low_temp_min_thld;
	int low_temp_max_thld;
	int low_temp_hysteresis;
	int low_temp_min_ibat;
	int low_temp_max_ibat;
};

#endif /* _BAT_HEATING_H_ */
