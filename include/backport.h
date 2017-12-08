/*
 * Backport kernel code for Intel FPGA driver
 *
 * Copyright 2016 Intel Corporation, Inc.
 *
 * Authors:
 *   Enno Luebbers <enno.luebbers@intel.com>
 *   Abelardo Jara-Berrocal <abelardo.jara-berrocal@intel.com>
 *   Tim Whisonant <tim.whisonant@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL version 2. See
 * the COPYING file in the top-level directory.
 *
 */

#ifndef __INTEL_BACKPORT_H
#define __INTEL_BACKPORT_H

#include <linux/version.h>
#include <linux/device.h>
#include <linux/stddef.h>
#include <linux/vfio.h> /* offsetofend in pre-4.1.0 kernels */
#include <linux/sysfs.h>
#include <linux/idr.h>
#include <linux/sched.h> /* current->mm in pre-4.0 kernels */
#include <linux/uuid.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
#ifndef PAGE_ALIGNED
#define PAGE_ALIGNED(addr) IS_ALIGNED((unsigned long)(addr), PAGE_SIZE)
#endif

#define DEVICE_ATTR_RO(_name)						 \
	struct device_attribute dev_attr_##_name = __ATTR_RO(_name)

#define __ATTR_WO(_name) {						 \
		.attr = { .name = __stringify(_name), .mode = S_IWUSR }, \
		.store = _name##_store,					 \
	}

#define DEVICE_ATTR_WO(_name)						 \
	struct device_attribute dev_attr_##_name = __ATTR_WO(_name)

#define __ATTR_RW(_name) __ATTR(_name, (S_IWUSR | S_IRUGO),	         \
				_name##_show, _name##_store)

#define DEVICE_ATTR_RW(_name)						 \
	struct device_attribute dev_attr_##_name = __ATTR_RW(_name)

int sysfs_create_groups(struct kobject *kobj,
			const struct attribute_group **groups);

void sysfs_remove_groups(struct kobject *kobj,
			 const struct attribute_group **groups);

#endif /* LINUX_VERSION_CODE */

/* for ktime_get */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,17,0)
#include <linux/hrtimer.h>
#else
#include <linux/timekeeping.h>
#endif /* LINUX_VERSION_CODE */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
#define ENABLE_AER 1
#endif /* LINUX_VERSION_CODE */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
extern int sysfs_create_groups(struct kobject *kobj,
			       const struct attribute_group **groups);

extern void sysfs_remove_groups(struct kobject *kobj,
				const struct attribute_group **groups);
#endif /* LINUX_VERSION_CODE */

// TODO: Add external dependecy, introduced in recent kernel
extern int uuid_le_to_bin(const char *uuid, uuid_le *u);

#endif /* __INTEL_BACKPORT_H */
