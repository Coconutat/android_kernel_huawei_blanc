/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Inode based directory notification for Linux
 *
 * Copyright (C) 2005 John McCutchan
 */
#ifndef _LINUX_INOTIFY_H
#define _LINUX_INOTIFY_H

#include <linux/sysctl.h>
#include <uapi/linux/inotify.h>
#include <linux/fsnotify_backend.h> // 确保能访问 fsnotify_mark 结构体

extern struct ctl_table inotify_table[]; /* for sysctl */

#define ALL_INOTIFY_BITS (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE | \
			  IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM | \
			  IN_MOVED_TO | IN_CREATE | IN_DELETE | \
			  IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT | \
			  IN_Q_OVERFLOW | IN_IGNORED | IN_ONLYDIR | \
			  IN_DONT_FOLLOW | IN_EXCL_UNLINK | IN_MASK_ADD | \
			  IN_MASK_CREATE | IN_ISDIR | IN_ONESHOT)

static inline __u32 inotify_mark_user_mask(struct fsnotify_mark *mark)
{
    /* IN_ALL_EVENTS 通常定义在 uapi/linux/inotify.h 中，已被包含 */
    return mark->mask & IN_ALL_EVENTS;
}

#endif	/* _LINUX_INOTIFY_H */
