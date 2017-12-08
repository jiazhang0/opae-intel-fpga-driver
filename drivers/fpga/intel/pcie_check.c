/*
 * check the pcie parsed header with the default value in SAS spec
 *
 * Copyright 2016 Intel Corporation, Inc.
 *
 * Authors:
 *   Zhang Yi <Yi.Z.Zhang@intel.com>
 *   Xiao Guangrong <guangrong.xiao@linux.intel.com>
 *
 * This work is licensed under the terms of the GNU GPL version 2. See
 * the COPYING file in the top-level directory.
 */

#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/stddef.h>
#include "feature-dev.h"

#define DFH_CCI_VERSION				0x1
#define DFH_CCI_MINREVERSION			0x0
#define DFH_TYPE_PRIVATE			0x3
#define DFH_TYPE_AFU				0x1

#define FME_FEATURE_HEADER_TYPE			DFH_TYPE_AFU
#define FME_FEATURE_HEADER_NEXT_OFFSET		0x1000
#define FME_FEATURE_HEADER_ID			DFH_CCI_VERSION
#define FME_FEATURE_HEADER_VERSION		DFH_CCI_MINREVERSION

#define FME_FEATURE_THERMAL_MGMT_TYPE		DFH_TYPE_PRIVATE
#define FME_FEATURE_THERMAL_MGMT_NEXT_OFFSET	0x1000
#define FME_FEATURE_THERMAL_MGMT_ID		0x1
#define FME_FEATURE_THERMAL_MGMT_VERSION	0x0

#define FME_FEATURE_POWER_MGMT_TYPE		DFH_TYPE_PRIVATE
#define FME_FEATURE_POWER_MGMT_NEXT_OFFSET	0x1000
#define FME_FEATURE_POWER_MGMT_ID		0x2
#define FME_FEATURE_POWER_MGMT_VERSION		0x0

#define FME_FEATURE_GLOBAL_PERF_TYPE		DFH_TYPE_PRIVATE
#define FME_FEATURE_GLOBAL_PERF_NEXT_OFFSET	0x1000
#define FME_FEATURE_GLOBAL_PERF_ID		0x3
#define FME_FEATURE_GLOBAL_PERF_VERSION		0x0

#define FME_FEATURE_GLOBAL_ERR_TYPE		DFH_TYPE_PRIVATE
#define FME_FEATURE_GLOBAL_ERR_NEXT_OFFSET	0x1000
#define FME_FEATURE_GLOBAL_ERR_ID		0x4
#define FME_FEATURE_GLOBAL_ERR_VERSION		0x0

#define FME_FEATURE_PR_MGMT_TYPE		DFH_TYPE_PRIVATE
#define FME_FEATURE_PR_MGMT_NEXT_OFFSET		0x0
#define FME_FEATURE_PR_MGMT_ID			0x5
#define FME_FEATURE_PR_MGMT_VERSION		0x0

#define PORT_FEATURE_HEADER_TYPE		DFH_TYPE_AFU
#define PORT_FEATURE_HEADER_NEXT_OFFSET		0x1000
#define PORT_FEATURE_HEADER_ID			DFH_CCI_VERSION
#define PORT_FEATURE_HEADER_VERSION		DFH_CCI_MINREVERSION

#define PORT_FEATURE_ERR_TYPE			DFH_TYPE_PRIVATE
#define PORT_FEATURE_ERR_NEXT_OFFSET		0x1000
#define PORT_FEATURE_ERR_ID			0x10
#define PORT_FEATURE_ERR_VERSION		0x0

#define PORT_FEATURE_UMSG_TYPE			DFH_TYPE_PRIVATE
#define PORT_FEATURE_UMSG_NEXT_OFFSET		0x2000
#define PORT_FEATURE_UMSG_ID			0x11
#define PORT_FEATURE_UMSG_VERSION		0x0

#define PORT_FEATURE_STP_TYPE			DFH_TYPE_PRIVATE
#define PORT_FEATURE_STP_NEXT_OFFSET		0x0
#define PORT_FEATURE_STP_ID			0x13
#define PORT_FEATURE_STP_VERSION		0x0

#define DEFAULT_REG(name)	{.id = name##_ID, .revision = name##_VERSION,\
				.next_header_offset = name##_NEXT_OFFSET,\
				.type = name##_TYPE,}

static struct feature_header default_port_feature_hdr[] = {
	DEFAULT_REG(PORT_FEATURE_HEADER),
	DEFAULT_REG(PORT_FEATURE_ERR),
	DEFAULT_REG(PORT_FEATURE_UMSG),
	{.csr = 0,},
	DEFAULT_REG(PORT_FEATURE_STP),
	{.csr = 0,},
};

static struct feature_header default_fme_feature_hdr[] = {
	DEFAULT_REG(FME_FEATURE_HEADER),
	DEFAULT_REG(FME_FEATURE_THERMAL_MGMT),
	DEFAULT_REG(FME_FEATURE_POWER_MGMT),
	DEFAULT_REG(FME_FEATURE_GLOBAL_PERF),
	DEFAULT_REG(FME_FEATURE_GLOBAL_ERR),
	DEFAULT_REG(FME_FEATURE_PR_MGMT),
};

void check_features_header(struct pci_dev *pdev, struct feature_header *hdr,
			   enum fpga_devt_type type, int id)
{
	struct feature_header *default_header, header;

	if (type == FPGA_DEVT_FME) {
		default_header = default_fme_feature_hdr;
		WARN_ON(id >= ARRAY_SIZE(default_fme_feature_hdr));
	} else if (type == FPGA_DEVT_PORT) {
		default_header = default_port_feature_hdr;
		WARN_ON(id >= ARRAY_SIZE(default_port_feature_hdr));
	} else {
		WARN_ON(1);
		return;
	}

	header.csr = readq(hdr);

	if (memcmp(&header, default_header + id, sizeof(header)))
		dev_err(&pdev->dev,
			"check header failed. current hdr:%llx - default_value:%llx.\n",
			header.csr, *(u64 *)(default_header + id));
	else
		dev_dbg(&pdev->dev,
			"check header pass.\n");
}
