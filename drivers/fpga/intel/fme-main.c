/*
 * Driver for FPGA Management Engine which implements all FPGA platform
 * level management features.
 *
 * Copyright 2016 Intel Corporation, Inc.
 *
 * Authors:
 *   Kang Luwei <luwei.kang@intel.com>
 *   Xiao Guangrong <guangrong.xiao@linux.intel.com>
 *   Joseph Grecco <joe.grecco@intel.com>
 *   Enno Luebbers <enno.luebbers@intel.com>
 *   Tim Whisonant <tim.whisonant@intel.com>
 *   Ananda Ravuri <ananda.ravuri@intel.com>
 *   Mitchel, Henry <henry.mitchel@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL version 2. See
 * the COPYING file in the top-level directory.
 *
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/stddef.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/intel-fpga.h>

#include "backport.h"
#include <linux/fpga/fpga-mgr_mod.h>
#include <linux/mtd/altera-asmip2.h>

#include "feature-dev.h"
#include "fme.h"

#define PWR_THRESHOLD_MAX       0x7F

#define FME_DEV_ATTR(_name, _filename, _mode, _show, _store)	\
struct device_attribute dev_attr_##_name =			\
	__ATTR(_filename, _mode, _show, _store)

static ssize_t revision_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	struct feature_header header;

	header.csr = readq(&fme_hdr->header);

	return scnprintf(buf, PAGE_SIZE, "%d\n", header.revision);
}

static DEVICE_ATTR_RO(revision);

static ssize_t ports_num_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	struct feature_fme_capability fme_capability;

	fme_capability.csr = readq(&fme_hdr->capability);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fme_capability.num_ports);
}

static DEVICE_ATTR_RO(ports_num);

static ssize_t cache_size_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	struct feature_fme_capability fme_capability;

	fme_capability.csr = readq(&fme_hdr->capability);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fme_capability.cache_size);
}

static DEVICE_ATTR_RO(cache_size);

static ssize_t version_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	struct feature_fme_capability fme_capability;

	fme_capability.csr = readq(&fme_hdr->capability);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fme_capability.fabric_verid);
}

static DEVICE_ATTR_RO(version);

static ssize_t socket_id_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	struct feature_fme_capability fme_capability;

	fme_capability.csr = readq(&fme_hdr->capability);

	return scnprintf(buf, PAGE_SIZE, "%d\n", fme_capability.socket_id);
}

static DEVICE_ATTR_RO(socket_id);

static ssize_t bitstream_id_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	u64 bitstream_id = readq(&fme_hdr->bitstream_id);

	return scnprintf(buf, PAGE_SIZE, "0x%llx\n", bitstream_id);
}

static DEVICE_ATTR_RO(bitstream_id);

static ssize_t bitstream_metadata_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	u64 bitstream_md = readq(&fme_hdr->bitstream_md);

	return scnprintf(buf, PAGE_SIZE, "0x%llx\n", bitstream_md);
}

static DEVICE_ATTR_RO(bitstream_metadata);

static const struct attribute *fme_hdr_attrs[] = {
	&dev_attr_revision.attr,
	&dev_attr_ports_num.attr,
	&dev_attr_cache_size.attr,
	&dev_attr_version.attr,
	&dev_attr_socket_id.attr,
	&dev_attr_bitstream_id.attr,
	&dev_attr_bitstream_metadata.attr,
	NULL,
};

static int fme_hdr_init(struct platform_device *pdev, struct feature *feature)
{
	int ret;
	struct feature_fme_header *fme_hdr = feature->ioaddr;

	dev_dbg(&pdev->dev, "FME HDR Init.\n");
	dev_dbg(&pdev->dev, "FME cap %llx.\n", fme_hdr->capability.csr);

	ret = sysfs_create_files(&pdev->dev.kobj, fme_hdr_attrs);
	if (ret)
		return ret;

	return 0;
}

static void fme_hdr_uinit(struct platform_device *pdev, struct feature *feature)
{
	dev_dbg(&pdev->dev, "FME HDR UInit.\n");
	sysfs_remove_files(&pdev->dev.kobj, fme_hdr_attrs);
}

struct feature_ops fme_hdr_ops = {
	.init = fme_hdr_init,
	.uinit = fme_hdr_uinit,
};

static ssize_t thermal_revision_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_header header;

	header.csr = readq(&fme_thermal->header);

	return scnprintf(buf, PAGE_SIZE, "%d\n", header.revision);
}

static FME_DEV_ATTR(thermal_revision, revision, 0444,
		    thermal_revision_show, NULL);

static ssize_t thermal_threshold1_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_tmp_threshold tmp_threshold;

	tmp_threshold.csr = readq(&fme_thermal->threshold);

	return scnprintf(buf, PAGE_SIZE, "%d\n", tmp_threshold.tmp_thshold1);
}

static ssize_t thermal_threshold1_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct feature_platform_data *pdata = dev_get_platdata(dev);
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	struct feature_fme_tmp_threshold tmp_threshold;
	struct feature_fme_capability fme_capability;
	int err;
	u8 tmp_threshold1;

	mutex_lock(&pdata->lock);
	tmp_threshold.csr = readq(&fme_thermal->threshold);

	err = kstrtou8(buf, 0, &tmp_threshold1);
	if (err) {
		mutex_unlock(&pdata->lock);
		return err;
	}

	fme_capability.csr = readq(&fme_hdr->capability);

	if (fme_capability.lock_bit == 1) {
		mutex_unlock(&pdata->lock);
		return -EBUSY;
	} else if (tmp_threshold1 > 100) {
		mutex_unlock(&pdata->lock);
		return -EINVAL;
	} else if (tmp_threshold1 == 0) {
		tmp_threshold.tmp_thshold1_enable = 0;
		tmp_threshold.tmp_thshold1 = tmp_threshold1;
	} else {
		tmp_threshold.tmp_thshold1_enable = 1;
		tmp_threshold.tmp_thshold1 = tmp_threshold1;
	}

	writeq(tmp_threshold.csr, &fme_thermal->threshold);
	mutex_unlock(&pdata->lock);

	return count;
}

static DEVICE_ATTR(threshold1, 0644,
	thermal_threshold1_show, thermal_threshold1_store);

static ssize_t thermal_threshold2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_tmp_threshold tmp_threshold;

	tmp_threshold.csr = readq(&fme_thermal->threshold);

	return scnprintf(buf, PAGE_SIZE, "%d\n", tmp_threshold.tmp_thshold2);
}

static ssize_t thermal_threshold2_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct feature_platform_data *pdata = dev_get_platdata(dev);
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_header *fme_hdr
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_HEADER);
	struct feature_fme_tmp_threshold tmp_threshold;
	struct feature_fme_capability fme_capability;
	int err;
	u8 tmp_threshold2;

	mutex_lock(&pdata->lock);
	tmp_threshold.csr = readq(&fme_thermal->threshold);

	err = kstrtou8(buf, 0, &tmp_threshold2);
	if (err) {
		mutex_unlock(&pdata->lock);
		return err;
	}

	fme_capability.csr = readq(&fme_hdr->capability);

	if (fme_capability.lock_bit == 1) {
		mutex_unlock(&pdata->lock);
		return -EBUSY;
	} else if (tmp_threshold2 > 100) {
		mutex_unlock(&pdata->lock);
		return -EINVAL;
	} else if (tmp_threshold2 == 0) {
		tmp_threshold.tmp_thshold2_enable = 0;
		tmp_threshold.tmp_thshold2 = tmp_threshold2;
	} else {
		tmp_threshold.tmp_thshold2_enable = 1;
		tmp_threshold.tmp_thshold2 = tmp_threshold2;
	}

	writeq(tmp_threshold.csr, &fme_thermal->threshold);
	mutex_unlock(&pdata->lock);

	return count;
}

static DEVICE_ATTR(threshold2, 0644,
	thermal_threshold2_show, thermal_threshold2_store);

static ssize_t thermal_threshold_trip_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_tmp_threshold tmp_threshold;

	tmp_threshold.csr = readq(&fme_thermal->threshold);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
				tmp_threshold.therm_trip_thshold);
}

static DEVICE_ATTR(threshold_trip, 0444, thermal_threshold_trip_show, NULL);

static ssize_t thermal_threshold1_reached_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_tmp_threshold tmp_threshold;

	tmp_threshold.csr = readq(&fme_thermal->threshold);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
				tmp_threshold.thshold1_status);
}

static DEVICE_ATTR(threshold1_reached, 0444,
	thermal_threshold1_reached_show, NULL);

static ssize_t thermal_threshold2_reached_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_tmp_threshold tmp_threshold;

	tmp_threshold.csr = readq(&fme_thermal->threshold);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
				tmp_threshold.thshold2_status);
}

static DEVICE_ATTR(threshold2_reached, 0444,
	thermal_threshold2_reached_show, NULL);

static ssize_t thermal_threshold1_policy_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_tmp_threshold tmp_threshold;

	tmp_threshold.csr = readq(&fme_thermal->threshold);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
				tmp_threshold.thshold_policy);
}

static ssize_t thermal_threshold1_policy_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct feature_platform_data *pdata = dev_get_platdata(dev);
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_tmp_threshold tmp_threshold;
	int err;
	u8 thshold_policy;

	mutex_lock(&pdata->lock);
	tmp_threshold.csr = readq(&fme_thermal->threshold);

	err = kstrtou8(buf, 0, &thshold_policy);
	if (err) {
		mutex_unlock(&pdata->lock);
		return err;
	}

	if (thshold_policy == 0)
		tmp_threshold.thshold_policy = 0;
	else if (thshold_policy == 1)
		tmp_threshold.thshold_policy = 1;
	else {
		mutex_unlock(&pdata->lock);
		return -EINVAL;
	}

	writeq(tmp_threshold.csr, &fme_thermal->threshold);
	mutex_unlock(&pdata->lock);

	return count;
}

static DEVICE_ATTR(threshold1_policy, 0644,
	thermal_threshold1_policy_show, thermal_threshold1_policy_store);

static ssize_t thermal_temperature_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_thermal *fme_thermal
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_THERMAL_MGMT);
	struct feature_fme_temp_rdsensor_fmt1 temp_rdsensor_fmt1;

	temp_rdsensor_fmt1.csr = readq(&fme_thermal->rdsensor_fm1);

	return scnprintf(buf, PAGE_SIZE, "%d\n",
				temp_rdsensor_fmt1.fpga_temp);
}

static DEVICE_ATTR(temperature, 0444, thermal_temperature_show, NULL);

static struct attribute *thermal_mgmt_attrs[] = {
	&dev_attr_thermal_revision.attr,
	&dev_attr_threshold1.attr,
	&dev_attr_threshold2.attr,
	&dev_attr_threshold_trip.attr,
	&dev_attr_threshold1_reached.attr,
	&dev_attr_threshold2_reached.attr,
	&dev_attr_threshold1_policy.attr,
	&dev_attr_temperature.attr,
	NULL,
};

static struct attribute_group thermal_mgmt_attr_group = {
	.attrs	= thermal_mgmt_attrs,
	.name	= "thermal_mgmt",
};

static int thermal_mgmt_init(struct platform_device *pdev,
				struct feature *feature)
{
	int ret;

	ret = sysfs_create_group(&pdev->dev.kobj, &thermal_mgmt_attr_group);
	if (ret)
		return ret;

	return 0;
}

static void thermal_mgmt_uinit(struct platform_device *pdev,
				struct feature *feature)
{
	sysfs_remove_group(&pdev->dev.kobj, &thermal_mgmt_attr_group);
}

struct feature_ops thermal_mgmt_ops = {
	.init = thermal_mgmt_init,
	.uinit = thermal_mgmt_uinit,
};

static ssize_t pwr_revision_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_header header;

	header.csr = readq(&fme_power->header);

	return scnprintf(buf, PAGE_SIZE, "%d\n", header.revision);
}

static FME_DEV_ATTR(pwr_revision, revision, 0444, pwr_revision_show, NULL);

static ssize_t consumed_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_status pm_status;

	pm_status.csr = readq(&fme_power->status);

	return scnprintf(buf, PAGE_SIZE, "0x%x\n", pm_status.pwr_consumed);
}

static DEVICE_ATTR_RO(consumed);

static ssize_t pwr_threshold1_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_ap_threshold pm_ap_threshold;

	pm_ap_threshold.csr = readq(&fme_power->threshold);

	return scnprintf(buf, PAGE_SIZE, "0x%x\n", pm_ap_threshold.threshold1);
}

static ssize_t pwr_threshold1_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct feature_platform_data *pdata = dev_get_platdata(dev);
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_ap_threshold pm_ap_threshold;
	u8 threshold;
	int err;

	mutex_lock(&pdata->lock);
	pm_ap_threshold.csr = readq(&fme_power->threshold);

	err = kstrtou8(buf, 0, &threshold);
	if (err) {
		mutex_unlock(&pdata->lock);
		return err;
	}

	if (threshold <= PWR_THRESHOLD_MAX)
		pm_ap_threshold.threshold1 = threshold;
	else {
		mutex_unlock(&pdata->lock);
		return -EINVAL;
	}

	writeq(pm_ap_threshold.csr, &fme_power->threshold);
	mutex_unlock(&pdata->lock);

	return count;
}

static FME_DEV_ATTR(pwr_threshold1, threshold1, 0644, pwr_threshold1_show,
		    pwr_threshold1_store);

static ssize_t pwr_threshold2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_ap_threshold pm_ap_threshold;

	pm_ap_threshold.csr = readq(&fme_power->threshold);

	return scnprintf(buf, PAGE_SIZE, "0x%x\n",
				pm_ap_threshold.threshold2);
}

static ssize_t pwr_threshold2_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct feature_platform_data *pdata = dev_get_platdata(dev);
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_ap_threshold pm_ap_threshold;
	u8 threshold;
	int err;

	mutex_lock(&pdata->lock);
	pm_ap_threshold.csr = readq(&fme_power->threshold);

	err = kstrtou8(buf, 0, &threshold);
	if (err) {
		mutex_unlock(&pdata->lock);
		return err;
	}

	if (threshold <= PWR_THRESHOLD_MAX)
		pm_ap_threshold.threshold2 = threshold;
	else {
		mutex_unlock(&pdata->lock);
		return -EINVAL;
	}

	writeq(pm_ap_threshold.csr, &fme_power->threshold);
	mutex_unlock(&pdata->lock);

	return count;
}

static FME_DEV_ATTR(pwr_threshold2, threshold2, 0644, pwr_threshold2_show,
		    pwr_threshold2_store);

static ssize_t threshold1_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_ap_threshold pm_ap_threshold;

	pm_ap_threshold.csr = readq(&fme_power->threshold);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
				pm_ap_threshold.threshold1_status);
}

static DEVICE_ATTR_RO(threshold1_status);

static ssize_t threshold2_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_ap_threshold pm_ap_threshold;

	pm_ap_threshold.csr = readq(&fme_power->threshold);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
				pm_ap_threshold.threshold2_status);
}
static DEVICE_ATTR_RO(threshold2_status);

static ssize_t rtl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_status pm_status;

	pm_status.csr = readq(&fme_power->status);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
				pm_status.fpga_latency_report);
}

static DEVICE_ATTR_RO(rtl);

static ssize_t xeon_limit_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_xeon_limit xeon_limit;

	xeon_limit.csr = readq(&fme_power->xeon_limit);

	if (!xeon_limit.enable)
		xeon_limit.pwr_limit = 0;

	return scnprintf(buf, PAGE_SIZE, "%u\n", xeon_limit.pwr_limit);
}
static DEVICE_ATTR_RO(xeon_limit);

static ssize_t fpga_limit_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct feature_fme_power *fme_power
		= get_feature_ioaddr_by_index(dev, FME_FEATURE_ID_POWER_MGMT);
	struct feature_fme_pm_fpga_limit fpga_limit;

	fpga_limit.csr = readq(&fme_power->fpga_limit);

	if (!fpga_limit.enable)
		fpga_limit.pwr_limit = 0;

	return scnprintf(buf, PAGE_SIZE, "%u\n", fpga_limit.pwr_limit);
}
static DEVICE_ATTR_RO(fpga_limit);

static struct attribute *power_mgmt_attrs[] = {
	&dev_attr_pwr_revision.attr,
	&dev_attr_consumed.attr,
	&dev_attr_pwr_threshold1.attr,
	&dev_attr_pwr_threshold2.attr,
	&dev_attr_threshold1_status.attr,
	&dev_attr_threshold2_status.attr,
	&dev_attr_xeon_limit.attr,
	&dev_attr_fpga_limit.attr,
	&dev_attr_rtl.attr,
	NULL,
};

static struct attribute_group power_mgmt_attr_group = {
	.attrs	= power_mgmt_attrs,
	.name	= "power_mgmt",
};

static int power_mgmt_init(struct platform_device *pdev,
				struct feature *feature)
{
	int ret;

	ret = sysfs_create_group(&pdev->dev.kobj, &power_mgmt_attr_group);
	if (ret)
		return ret;

	return 0;
}

static void power_mgmt_uinit(struct platform_device *pdev,
				struct feature *feature)
{
	sysfs_remove_group(&pdev->dev.kobj, &power_mgmt_attr_group);
}

struct feature_ops power_mgmt_ops = {
	.init = power_mgmt_init,
	.uinit = power_mgmt_uinit,
};

static int hssi_mgmt_init(struct platform_device *pdev, struct feature *feature)
{
	dev_dbg(&pdev->dev, "FME HSSI Init.\n");
	return 0;
}

static void hssi_mgmt_uinit(struct platform_device *pdev,
				struct feature *feature)
{
	dev_dbg(&pdev->dev, "FME HSSI UInit.\n");
}

struct feature_ops hssi_mgmt_ops = {
	.init = hssi_mgmt_init,
	.uinit = hssi_mgmt_uinit,
};

#define FLASH_CAPABILITY_OFT 8

static int qspi_flash_init(struct platform_device *pdev,
			   struct feature *feature)
{
	u64 reg;
	struct altera_asmip2_plat_data qdata;
	struct platform_device *cdev;
	int ret = 0;

	reg = readq(feature->ioaddr + FLASH_CAPABILITY_OFT);
	dev_info(&pdev->dev, "%s %s %d 0x%llx 0x%x 0x%x\n",
		 __func__, ALTERA_ASMIP2_DRV_NAME, feature->resource_index,
		 reg, readl(feature->ioaddr + FLASH_CAPABILITY_OFT),
		 readl(feature->ioaddr + FLASH_CAPABILITY_OFT + 4));

	cdev = platform_device_alloc(ALTERA_ASMIP2_DRV_NAME,
				     PLATFORM_DEVID_AUTO);

	if (!cdev) {
		dev_err(&pdev->dev, "platform_device_alloc failed in %s\n",
			__func__);
		return -ENOMEM;
	}

	cdev->dev.parent = &pdev->dev;

	memset(&qdata, 0, sizeof(qdata));
	qdata.csr_base = feature->ioaddr + FLASH_CAPABILITY_OFT;
	qdata.num_chip_sel = 1;

	ret = platform_device_add_data(cdev, &qdata, sizeof(qdata));
	if (ret) {
		dev_err(&pdev->dev, "platform_device_add_data in %s\n",
			__func__);
		goto error;
	}

	ret = platform_device_add(cdev);
	if (ret) {
		dev_err(&pdev->dev, "platform_device_add failed with %d\n",
			ret);
		goto error;
	}

	return ret;

error:
	platform_device_put(cdev);
	return ret;
}

struct feature_platform_search {
	const char *drv_name;
	int name_len;
	struct feature *feature;
};

static int qspi_match(struct device *dev, void *data)
{
	struct feature_platform_search *src =
		(struct feature_platform_search *)data;
	struct altera_asmip2_plat_data *qdata;

	if (strncmp(dev_name(dev), src->drv_name, src->name_len))
		return 0;

	qdata = dev_get_platdata(dev);

	if (qdata &&
	    (qdata->csr_base == (src->feature->ioaddr + FLASH_CAPABILITY_OFT)))
		return 1;
	else
		return 0;
}

static void qspi_flash_uinit(struct platform_device *pdev,
			     struct feature *feature)
{
	struct device *parent = &pdev->dev;
	struct feature_platform_search src;
	struct device *dev;
	struct platform_device *cdev;

	src.drv_name = ALTERA_ASMIP2_DRV_NAME;
	src.name_len = strlen(ALTERA_ASMIP2_DRV_NAME);
	src.feature = feature;

	dev = device_find_child(parent, &src, qspi_match);

	if (!dev) {
		dev_err(&pdev->dev, "%s NOT found\n", ALTERA_ASMIP2_DRV_NAME);
		return;
	}

	dev_info(&pdev->dev, "%s found %s\n", __func__, ALTERA_ASMIP2_DRV_NAME);

	cdev = to_platform_device(dev);

	if (!cdev) {
		dev_err(&pdev->dev, "no platform container\n");
		return;
	}

	platform_device_unregister(cdev);
}

struct feature_ops qspi_flash_ops = {
	.init = qspi_flash_init,
	.uinit = qspi_flash_uinit,
};

static struct feature_driver fme_feature_drvs[] = {
	{
		.name = FME_FEATURE_HEADER,
		.ops = &fme_hdr_ops,
	},
	{
		.name = FME_FEATURE_THERMAL_MGMT,
		.ops = &thermal_mgmt_ops,
	},
	{
		.name = FME_FEATURE_POWER_MGMT,
		.ops = &power_mgmt_ops,
	},
	{
		.name = FME_FEATURE_GLOBAL_ERR,
		.ops = &global_error_ops,
	},
	{
		.name = FME_FEATURE_PR_MGMT,
		.ops = &pr_mgmt_ops,
	},
	{
		.name = FME_FEATURE_GLOBAL_IPERF,
		.ops = &global_iperf_ops,
	},
	{
		.name = FME_FEATURE_HSSI_ETH,
		.ops = &hssi_mgmt_ops,
	},
	{
		.name = FME_FEATURE_GLOBAL_DPERF,
		.ops = &global_dperf_ops,
	},
	{
		.name = FME_FEATURE_QSPI_FLASH,
		.ops = &qspi_flash_ops,
	},
	{
		.ops = NULL,
	},
};

static long fme_ioctl_check_extension(struct feature_platform_data *pdata,
				     unsigned long arg)
{
	/* No extension support for now */
	return 0;
}

static long
fme_ioctl_get_info(struct feature_platform_data *pdata, void __user *arg)
{
	struct fpga_fme_info info;
	struct fpga_fme *fme;
	unsigned long minsz;

	minsz = offsetofend(struct fpga_fme_info, capability);

	if (copy_from_user(&info, arg, minsz))
		return -EFAULT;

	if (info.argsz < minsz)
		return -EINVAL;

	mutex_lock(&pdata->lock);
	fme = fpga_pdata_get_private(pdata);
	info.flags = 0;
	info.capability = fme->capability;
	mutex_unlock(&pdata->lock);

	if (copy_to_user(arg, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static int fme_ioctl_config_port(struct feature_platform_data *pdata,
				 u32 port_id, u32 flags, bool is_release)
{
	struct platform_device *fme_pdev = pdata->dev;
	struct feature_fme_header *fme_hdr;
	struct feature_fme_capability capability;

	if (flags)
		return -EINVAL;

	fme_hdr = get_feature_ioaddr_by_index(
		&fme_pdev->dev, FME_FEATURE_ID_HEADER);
	capability.csr = readq(&fme_hdr->capability);

	if (port_id >= capability.num_ports)
		return -EINVAL;

	return pdata->config_port(fme_pdev, port_id, is_release);
}

static long fme_ioctl_release_port(struct feature_platform_data *pdata,
				   void __user *arg)
{
	struct fpga_fme_port_release release;
	unsigned long minsz;

	minsz = offsetofend(struct fpga_fme_port_release, port_id);

	if (copy_from_user(&release, arg, minsz))
		return -EFAULT;

	if (release.argsz < minsz)
		return -EINVAL;

	return fme_ioctl_config_port(pdata, release.port_id,
				     release.flags, true);
}

static long fme_ioctl_assign_port(struct feature_platform_data *pdata,
				  void __user *arg)
{
	struct fpga_fme_port_assign assign;
	unsigned long minsz;

	minsz = offsetofend(struct fpga_fme_port_assign, port_id);

	if (copy_from_user(&assign, arg, minsz))
		return -EFAULT;

	if (assign.argsz < minsz)
		return -EINVAL;

	return fme_ioctl_config_port(pdata, assign.port_id,
				     assign.flags, false);
}

static int fme_open(struct inode *inode, struct file *filp)
{
	struct platform_device *fdev = fpga_inode_to_feature_dev(inode);
	struct feature_platform_data *pdata = dev_get_platdata(&fdev->dev);
	int ret;

	if (WARN_ON(!pdata))
		return -ENODEV;

	if (filp->f_flags & O_EXCL)
		ret = feature_dev_use_excl_begin(pdata);
	else
		ret = feature_dev_use_begin(pdata);

	if (ret)
		return ret;

	dev_dbg(&fdev->dev, "Device File Opened %d Times\n", pdata->open_count);
	filp->private_data = pdata;
	return 0;
}

static int fme_release(struct inode *inode, struct file *filp)
{
	struct feature_platform_data *pdata = filp->private_data;
	struct platform_device *pdev = pdata->dev;

	dev_dbg(&pdev->dev, "Device File Release\n");
	mutex_lock(&pdata->lock);
	__feature_dev_use_end(pdata);

	if (!pdata->open_count)
		fpga_msix_set_block(&pdata->features[FME_FEATURE_ID_GLOBAL_ERR],
			0, pdata->features[FME_FEATURE_ID_GLOBAL_ERR].ctx_num,
			NULL);
	mutex_unlock(&pdata->lock);

	return 0;
}

static long fme_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct feature_platform_data *pdata = filp->private_data;
	struct platform_device *pdev = pdata->dev;
	struct feature *f;
	long ret;

	dev_dbg(&pdev->dev, "%s cmd 0x%x\n", __func__, cmd);

	switch (cmd) {
	case FPGA_GET_API_VERSION:
		return FPGA_API_VERSION;
	case FPGA_CHECK_EXTENSION:
		return fme_ioctl_check_extension(pdata, arg);
	case FPGA_FME_GET_INFO:
		return fme_ioctl_get_info(pdata, (void __user *)arg);
	case FPGA_FME_PORT_RELEASE:
		return fme_ioctl_release_port(pdata, (void __user *)arg);
	case FPGA_FME_PORT_ASSIGN:
		return fme_ioctl_assign_port(pdata, (void __user *)arg);
	default:
		/*
		 * Let sub-feature's ioctl function to handle the cmd
		 * Sub-feature's ioctl returns -ENODEV when cmd is not
		 * handled in this sub feature, and returns 0 and other
		 * error code if cmd is handled.
		 */
		fpga_dev_for_each_feature(pdata, f) {
			if (f->ops && f->ops->ioctl) {
				ret = f->ops->ioctl(pdev, f, cmd, arg);
				if (ret == -ENODEV)
					continue;
				else
					return ret;
			}
		}
	}

	return -EINVAL;
}

static const struct file_operations fme_fops = {
	.owner		= THIS_MODULE,
	.open		= fme_open,
	.release	= fme_release,
	.unlocked_ioctl = fme_ioctl,
};

static int fme_dev_init(struct platform_device *pdev)
{
	struct feature_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct fpga_fme *fme;

	fme = devm_kzalloc(&pdev->dev, sizeof(*fme), GFP_KERNEL);
	if (!fme)
		return -ENOMEM;

	fme->pdata = pdata;

	mutex_lock(&pdata->lock);
	fpga_pdata_set_private(pdata, fme);
	mutex_unlock(&pdata->lock);
	return 0;
}

static void fme_dev_destroy(struct platform_device *pdev)
{
	struct feature_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct fpga_fme *fme;

	mutex_lock(&pdata->lock);
	fme = fpga_pdata_get_private(pdata);
	fpga_pdata_set_private(pdata, NULL);
	mutex_unlock(&pdata->lock);

	devm_kfree(&pdev->dev, fme);
}

static int fme_probe(struct platform_device *pdev)
{
	int ret;

	ret = fme_dev_init(pdev);
	if (ret)
		goto exit;

	ret = fpga_dev_feature_init(pdev, fme_feature_drvs);
	if (ret)
		goto dev_destroy;

	ret = fpga_register_dev_ops(pdev, &fme_fops, THIS_MODULE);
	if (ret)
		goto feature_uinit;

	return 0;

feature_uinit:
	fpga_dev_feature_uinit(pdev);
dev_destroy:
	fme_dev_destroy(pdev);
exit:
	return ret;
}

static int fme_remove(struct platform_device *pdev)
{
	fpga_dev_feature_uinit(pdev);
	fpga_unregister_dev_ops(pdev);
	fme_dev_destroy(pdev);
	return 0;
}

static struct platform_driver fme_driver = {
	.driver	= {
		.name    = FPGA_FEATURE_DEV_FME,
	},
	.probe   = fme_probe,
	.remove  = fme_remove,
};

module_platform_driver(fme_driver);

MODULE_DESCRIPTION("FPGA Management Engine driver");
MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:intel-fpga-fme");
