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

#include "backport.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,0)
int sysfs_create_groups(struct kobject *kobj,
			       const struct attribute_group **groups)
{
	int error = 0;
	int i;

	if (!groups)
		return 0;

	for (i = 0; groups[i]; i++) {
		error = sysfs_create_group(kobj, groups[i]);
		if (error) {
			while (--i >= 0)
				sysfs_remove_group(kobj, groups[i]);
			break;
		}
	}
	return error;
}

void sysfs_remove_groups(struct kobject *kobj,
				const struct attribute_group **groups)
{
	int i;

	if (!groups)
		return;
	for (i = 0; groups[i]; i++)
		sysfs_remove_group(kobj, groups[i]);
}

#endif /* LINUX_VERSION_CODE */
