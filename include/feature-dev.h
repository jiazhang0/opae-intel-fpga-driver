/*
 * Intel FPGA Feature Device Framework Header
 *
 * Copyright 2016 Intel Corporation, Inc.
 *
 * Authors:
 *   Kang Luwei <luwei.kang@intel.com>
 *   Zhang Yi <Yi.Z.Zhang@intel.com>
 *   Wu Hao <hao.wu@linux.intel.com>
 *   Xiao Guangrong <guangrong.xiao@linux.intel.com>
 *
 * This work is licensed under the terms of the GNU GPL version 2. See
 * the COPYING file in the top-level directory.
 *
 */

#ifndef __INTEL_FPGA_FEATURE_H
#define __INTEL_FPGA_FEATURE_H

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/uuid.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

/* each FPGA device has 4 ports at most. */
#define MAX_FPGA_PORT_NUM 4
/*
 * Num of umsgs is up to 255, but only 32 umsgs allow hint mode per spec
 * so limit max num to 32 for now.
 */
#define MAX_PORT_UMSG_NUM 32
/* one for fme device */
#define MAX_FEATURE_DEV_NUM	(MAX_FPGA_PORT_NUM + 1)

#define FME_FEATURE_HEADER          "fme_hdr"
#define FME_FEATURE_THERMAL_MGMT    "fme_thermal"
#define FME_FEATURE_POWER_MGMT      "fme_power"
#define FME_FEATURE_GLOBAL_PERF     "fme_gperf"
#define FME_FEATURE_GLOBAL_ERR      "fme_error"
#define FME_FEATURE_PR_MGMT         "fme_pr"

#define PORT_FEATURE_HEADER         "port_hdr"
#define PORT_FEATURE_UAFU           "port_uafu"
#define PORT_FEATURE_ERR            "port_err"
#define PORT_FEATURE_UMSG           "port_umsg"
#define PORT_FEATURE_PR             "port_pr"
#define PORT_FEATURE_STP            "port_stp"

/*
 * do not check the revision id as id may be dynamic under
 * some cases, e.g, UAFU.
 */
#define SKIP_REVISION_CHECK		0xff

#define FME_HEADER_REVISION		0
#define FME_THERMAL_MGMT_REVISION	0
#define FME_POWER_MGMT_REVISION	0
#define FME_GLOBAL_PERF_REVISION	0
#define FME_GLOBAL_ERR_REVISION	0
#define FME_PR_MGMT_REVISION		1

#define PORT_HEADER_REVISION		0
/* UAFU's header info depends on the downloaded GBS */
#define PORT_UAFU_REVISION		SKIP_REVISION_CHECK
#define PORT_ERR_REVISION		0
#define PORT_UMSG_REVISION		0
#define PORT_PR_REVISION		0
#define PORT_STP_REVISION		1

/*
 * All headers and structures must be byte-packed to match the
 * SAS spec.
 */
#pragma pack(1)

struct feature_header {
	union {
		u64 csr;
		struct {
			u16 id:12;
			u8  revision:4;
			u32 next_header_offset:24;
			u32 reserved:20;
			u8  type:4;
		};
	};
};

struct feature_afu_header {
	uuid_le guid;
	union {
		u64 csr;
		struct {
			u64 next_afu:24;
			u64 reserved:40;
		};
	};
};

struct feature_fme_capability {
	union {
		u64 csr;
		struct {
			u8  fabric_verid;	/* Fabric version ID */
			u8  socket_id:1;	/* Socket id */
			u8  rsvd1:3;		/* Reserved */
			/* pci0 link available yes /no */
			u8  pci0_link_avile:1;
			/* pci1 link available yes /no */
			u8  pci1_link_avile:1;
			/* Coherent (QPI/UPI) link available yes /no */
			u8  qpi_link_avile:1;
			u8  rsvd2:1;		/* Reserved */
			/* IOMMU or VT-d supported  yes/no */
			u8  iommu_support:1;
			u8  num_ports:3;	/* Number of ports */
			u8  rsvd3:4;		/* Reserved */
			/*
			 * Address width supported in bits
			 */
			u8  address_width_bits:6;
			u8  rsvd4:2;		/* Reserved */
			/* Size of cache supported in kb */
			u16 cache_size:12;
			u8  cache_assoc:4;	/* Cache Associativity */
			u16 rsvd5:15;		/* Reserved */
			u8  lock_bit:1;		/* Lock bit */
		};
	};
};

#define FME_AFU_ACCESS_PF		0
#define FME_AFU_ACCESS_VF		1

struct feature_fme_port {
	union {
		u64 csr;
		struct {
			u32 port_offset:24;
			u8  reserved1;
			u8  port_bar:3;
			u32 reserved2:20;
			u8  afu_access_control:1;
			u8  reserved3:4;
			u8  port_implemented:1;
			u8  reserved4:3;
		};
	};
};

struct feature_fme_fab_status {
	union {
		u64 csr;
		struct {
			u8  upilink_status:4;   /* UPI Link Status */
			u8  rsvd1:4;		/* Reserved */
			u8  pci0link_status:1;  /* pci0 link status */
			u8  rsvd2:3;            /* Reserved */
			u8  pci1link_status:1;  /* pci1 link status */
			u64 rsvd3:51;           /* Reserved */
		};
	};
};

struct feature_fme_genprotrange2_base {
	union {
		u64 csr;
		struct {
			u16 rsvd1;           /* Reserved */
			/* Base Address of memory range */
			u8  protected_base_addrss:4;
			u64 rsvd2:44;           /* Reserved */
		};
	};
};

struct feature_fme_genprotrange2_limit {
	union {
		u64 csr;
		struct {
			u16 rsvd1;           /* Reserved */
			/* Limit Address of memory range */
			u8  protected_limit_addrss:4;
			u16 rsvd2:11;           /* Reserved */
			u8  enable_pr:1;        /* Enable GENPROTRANGE check */
			u32 rsvd3;           /* Reserved */
		};
	};
};

struct feature_fme_dxe_lock {
	union {
		u64 csr;
		struct {
			/*
			 * Determines write access to the DXE region CSRs
			 * 1 - CSR region is locked;
			 * 0 - it is open for write access.
			 */
			u8  dxe_early_lock:1;
			/*
			 * Determines write access to the HSSI CSR
			 * 1 - CSR region is locked;
			 * 0 - it is open for write access.
			 */
			u8  dxe_late_lock:1;
			u64 rsvd:62;
		};
	};
};

struct feature_fme_hssi_ctrl {
	union {
		u64 csr;
		struct {
			u32 data;	/* data */
			u16 address;	/* address */
			u16 command;	/* command */
		};
	};
};

struct feature_fme_hssi_start {
	union {
		u64 csr;
		struct {
			u32 data;	/* data */
			u8  ck:1;	/* Acknowledge*/
			u8  spare:1;	/* spare */
			u32 rsvd:30;	/* Reserved */
		};
	};
};

struct feature_fme_header {
	struct feature_header header;
	struct feature_afu_header afu_header;
	u64 reserved;
	u64 scratchpad;
	struct feature_fme_capability capability;
	struct feature_fme_port port[MAX_FPGA_PORT_NUM];
	struct feature_fme_fab_status fab_status;
	u64 bitstream_id;
	u64 bitstream_md;
	struct feature_fme_genprotrange2_base genprotrange2_base;
	struct feature_fme_genprotrange2_limit genprotrange2_limit;
	struct feature_fme_dxe_lock dxe_lock;
	struct feature_fme_hssi_ctrl hssi_ctrl;
	struct feature_fme_hssi_start hssi_start;
};

struct feature_port_capability {
	union {
		u64 csr;
		struct {
			u8 port_number:2;	/* Port Number 0-3 */
			u8 rsvd1:6;		/* Reserved */
			u16 mmio_size;		/* User MMIO size in KB */
			u8 rsvd2;		/* Reserved */
			u8 sp_intr_num:4;	/* Supported interrupts num */
			u32 rsvd3:28;		/* Reserved */
		};
	};
};

struct feature_port_control {
	union {
		u64 csr;
		struct {
			u8 port_sftrst:1;	/* Port Soft Reset */
			u8 rsvd1:1;		/* Reserved */
			u8 latency_tolerance:1;/* '1' >= 40us, '0' < 40us */
			u8 rsvd2:1;		/* Reserved */
			u8 port_sftrst_ack:1;	/* HW ACK for Soft Reset */
			u64 rsvd3:59;		/* Reserved */
		};
	};
};

#define PORT_POWER_STATE_NORMAL		0
#define PORT_POWER_STATE_AP1		1
#define PORT_POWER_STATE_AP2		2
#define PORT_POWER_STATE_AP6		6

struct feature_port_status {
	union {
		u64 csr;
		struct {
			u8 port_freeze:1;	/* '1' - freezed '0' - normal */
			u8 rsvd1:7;		/* Reserved */
			u8 power_state:4;	/* Power State */
			u64 rsvd2:52;		/* Reserved */
		};
	};
};

/* Port Header Register Set */
struct feature_port_header {
	struct feature_header header;
	struct feature_afu_header afu_header;
	u64 rsvd1;
	u64 scratchpad;
	struct feature_port_capability capability;
	struct feature_port_control control;
	struct feature_port_status status;
	u64 rsvd2;
	u64 user_clk_freq_cmd0;
	u64 user_clk_freq_cmd1;
	u64 user_clk_freq_sts0;
	u64 user_clk_freq_sts1;
};

struct feature_fme_tmp_threshold {
	union {
		u64 csr;
		struct {
			u8  tmp_thshold1:7;	  /* temperature Threshold 1 */
			/* temperature Threshold 1 enable/disable */
			u8  tmp_thshold1_enable:1;
			u8  tmp_thshold2:7;       /* temperature Threshold 2 */
			/* temperature Threshold 2 enable /disable */
			u8  tmp_thshold2_enable:1;
			u8  pro_hot_setpoint:7;   /* Proc Hot set point */
			u8  rsvd4:1;              /* Reserved */
			u8  therm_trip_thshold:7; /* Thermeal Trip Threshold */
			u8  rsvd3:1;              /* Reserved */
			u8  thshold1_status:1;	  /* Threshold 1 Status */
			u8  thshold2_status:1;    /* Threshold 2 Status */
			u8  rsvd5:1;              /* Reserved */
			/* Thermeal Trip Threshold status */
			u8  therm_trip_thshold_status:1;
			u8  rsvd6:4;		  /* Reserved */
			/* Validation mode- Force Proc Hot */
			u8  valmodeforce:1;
			/* Validation mode - Therm trip Hot */
			u8  valmodetherm:1;
			u8  rsvd2:2;              /* Reserved */
			u8  thshold_policy:1;     /* threshold policy */
			u32 rsvd:19;              /* Reserved */
		};
	};
};

/* Temperature Sensor Read values format 1 */
struct feature_fme_temp_rdsensor_fmt1 {
	union {
		u64 csr;
		struct {
			/* Reads out FPGA temperature in celsius */
			u8  fpga_temp:7;
			u8  rsvd0:1;			/* Reserved */
			/* Temperature reading sequence number */
			u16 tmp_reading_seq_num;
			/* Temperature reading is valid */
			u8  tmp_reading_valid:1;
			u8  rsvd1:7;			/* Reserved */
			u16 dbg_mode:10;		/* Debug mode */
			u32 rsvd2:22;			/* Reserved */
		};
	};
};

/* Temperature sensor read values format 2 */
struct feature_fme_temp_rdsensor_fmt2 {
	u64 rsvd;	/* Reserved */
};

/* FME THERNAL FEATURE */
struct feature_fme_thermal {
	struct feature_header header;
	struct feature_fme_tmp_threshold threshold;
	struct feature_fme_temp_rdsensor_fmt1 rdsensor_fm1;
	struct feature_fme_temp_rdsensor_fmt2 rdsensor_fm2;
};

/* Power Status register */
struct feature_fme_pm_status {
	union {
		u64 csr;
		struct {
			/* FPGA Power consumed, The format is to be defined */
			u32 pwr_consumed:18;
			/* FPGA Latency Tolerance Reporting */
			u8  fpga_latency_report:1;
			u64 rsvd:45;			/* Reserved */
		};
	};
};

/* AP Thresholds */
struct feature_fme_pm_ap_threshold {
	union {
		u64 csr;
		struct {
			/*
			 * Number of clocks (5ns period) for assertion
			 * of FME_data
			 */
			u8  threshold1:7;
			u8  rsvd1:1;
			u8  threshold2:7;
			u8  rsvd2:1;
			u8  threshold1_status:1;
			u8  threshold2_status:1;
			u64 rsvd3:46;		/* Reserved */
		};
	};
};

/* FME POWER FEATURE */
struct feature_fme_power {
	struct feature_header header;
	struct feature_fme_pm_status status;
	struct feature_fme_pm_ap_threshold threshold;
};

#define CACHE_CHANNEL_RD	0
#define CACHE_CHANNEL_WR	1

enum gperf_cache_events {
	CACHE_RD_HIT,
	CACHE_WR_HIT,
	CACHE_RD_MISS,
	CACHE_WR_MISS,
	CACHE_RSVD, /* reserved */
	CACHE_HOLD_REQ,
	CACHE_DATA_WR_PORT_CONTEN,
	CACHE_TAG_WR_PORT_CONTEN,
	CACHE_TX_REQ_STALL,
	CACHE_RX_REQ_STALL,
	CACHE_EVICTIONS,
};

/* FPMON Cache Control */
struct feature_fme_fpmon_ch_ctl {
	union {
		u64 csr;
		struct {
			u8  reset_counters:1;	/* Reset Counters */
			u8  rsvd1:7;		/* Reserved */
			u8  freeze:1;		/* Freeze if set to 1 */
			u8  rsvd2:7;		/* Reserved */
			u8  cache_event:4;	/* Select the cache event */
			u8  cci_chsel:1;	/* Select the channel */
			u64 rsvd3:43;		/* Reserved */
		};
	};
};

/* FPMON Cache Counter */
struct feature_fme_fpmon_ch_ctr {
	union {
		u64 csr;
		struct {
			/* Cache Counter for even addresse */
			u64 cache_counter:48;
			u16 rsvd:12;		/* Reserved */
			/* Cache Event being reported */
			u8  event_code:4;
		};
	};
};

enum gperf_fab_events {
	FAB_PCIE0_RD,
	FAB_PCIE0_WR,
	FAB_PCIE1_RD,
	FAB_PCIE1_WR,
	FAB_UPI_RD,
	FAB_UPI_WR,
	FAB_MMIO_RD,
	FAB_MMIO_WR,
};

#define FAB_DISABLE_FILTER     0
#define FAB_ENABLE_FILTER      1

/* FPMON FAB Control */
struct feature_fme_fpmon_fab_ctl {
	union {
		u64 csr;
		struct {
			u8  reset_counters:1;	/* Reset Counters */
			u8  rsvd:7;		/* Reserved */
			u8  freeze:1;		/* Set to 1 frozen counter */
			u8  rsvd1:7;		/* Reserved */
			u8  fab_evtcode:4;	/* Fabric Event Code */
			u8  port_id:2;		/* Port ID */
			u8  rsvd2:1;		/* Reserved */
			u8  port_filter:1;	/* Port Filter */
			u64 rsvd3:40;		/* Reserved */
		};
	};
};

/* FPMON Event Counter */
struct feature_fme_fpmon_fab_ctr {
	union {
		u64 csr;
		struct {
			u64 fab_cnt:60;	/* Fabric event counter */
			/* Fabric event code being reported */
			u8  event_code:4;
		};
	};
};

/* FPMON Clock Counter */
struct feature_fme_fpmon_clk_ctr {
	u64 afu_interf_clock;		/* Clk_16UI (AFU clock) counter. */
};

enum gperf_vtd_events {
	VTD_AFU0_MEM_RD_TRANS,
	VTD_AFU1_MEM_RD_TRANS,
	VTD_AFU0_MEM_WR_TRANS,
	VTD_AFU1_MEM_WR_TRANS,
	VTD_AFU0_TLB_RD_HIT,
	VTD_AFU1_TLB_RD_HIT,
	VTD_AFU0_TLB_WR_HIT,
	VTD_AFU1_TLB_WR_HIT,
};

/* VT-d control register */
struct feature_fme_fpmon_vtd_ctl {
	union {
		u64 csr;
		struct {
			u8  reset_counters:1;	/* Reset Counters */
			u8  rsvd:7;		/* Reserved */
			u8  freeze:1;		/* Set to 1 frozen counter */
			u8  rsvd1:7;		/* Reserved */
			u8  vtd_evtcode:4;	/* VTd and TLB event code */
			u64 rsvd2:44;		/* Reserved */
		};
	};
};

/* VT-d event counter */
struct feature_fme_fpmon_vtd_ctr {
	union {
		u64 csr;
		struct {
			u64 vtd_counter:48;	/* VTd event counter */
			u16 rsvd:12;		/* Reserved */
			u8 event_code:4; /* VTd event code */
		};
	};
};

/* FME GPERF FEATURE */
struct feature_fme_gperf {
	struct feature_header header;
	struct feature_fme_fpmon_ch_ctl ch_ctl;
	struct feature_fme_fpmon_ch_ctr ch_ctr0;
	struct feature_fme_fpmon_ch_ctr ch_ctr1;
	struct feature_fme_fpmon_fab_ctl fab_ctl;
	struct feature_fme_fpmon_fab_ctr fab_ctr;
	struct feature_fme_fpmon_clk_ctr clk;
	struct feature_fme_fpmon_vtd_ctl vtd_ctl;
	struct feature_fme_fpmon_vtd_ctr vtd_ctr;
};

struct feature_fme_error0 {
#define FME_ERROR0_MASK        0xFFUL
#define FME_ERROR0_MASK_DEFAULT 0x40UL  /* pcode workaround */
	union {
		u64 csr;
		struct {
			u8  fabric_err:1;	/* Fabric error */
			u8  fabfifo_overflow:1;	/* Fabric fifo overflow */
			u8  pcie0_poison:1;	/* PCIE0 Poison Detected */
			u8  pcie1_poison:1;	/* PCIE1 Poison Detected */
			u8  iommu_parity_err:1;	/* IOMMU Parity error */
			/* AFU PF/VF access mismatch detected */
			u8  afu_acc_mode_err:1;
			u8  mbp_err:1;		/* Indicates an MBP event */
			u64 rsvd:57;		/* Reserved */
		};
	};
};

/* PCIe0 Error Status register */
struct feature_fme_pcie0_error {
#define FME_PCIE0_ERROR_MASK   0xFFUL
	union {
		u64 csr;
		struct {
			u8  formattype_err:1;	/* TLP format/type error */
			u8  MWAddr_err:1;	/* TLP MW address error */
			u8  MWAddrLength_err:1;	/* TLP MW length error */
			u8  MRAddr_err:1;	/* TLP MR address error */
			u8  MRAddrLength_err:1;	/* TLP MR length error */
			u8  cpl_tag_err:1;	/* TLP CPL tag error */
			u8  cpl_status_err:1;	/* TLP CPL status error */
			u8  cpl_timeout_err:1;	/* TLP CPL timeout */
			u64 rsvd:54;		/* Reserved */
			u8  vfnumb_err:1;	/* Number of error VF */
			u8  funct_type_err:1;	/* Virtual (1) or Physical */
		};
	};
};

/* PCIe1 Error Status register */
struct feature_fme_pcie1_error {
#define FME_PCIE1_ERROR_MASK   0xFFUL
	union {
		u64 csr;
		struct {
			u8  formattype_err:1;	/* TLP format/type error */
			u8  MWAddr_err:1;	/* TLP MW address error */
			u8  MWAddrLength_err:1;	/* TLP MW length error */
			u8  MRAddr_err:1;	/* TLP MR address error */
			u8  MRAddrLength_err:1;	/* TLP MR length error */
			u8  cpl_tag_err:1;	/* TLP CPL tag error */
			u8  cpl_status_err:1;	/* TLP CPL status error */
			u8  cpl_timeout_err:1;	/* TLP CPL timeout */
			u64 rsvd:56;		/* Reserved */
		};
	};
};


/* FME First Error register */
struct feature_fme_first_error {
#define FME_FIRST_ERROR_MASK   ((1UL << 60) - 1)
	union {
		u64 csr;
		struct {
			/*
			 * Indicates the Error Register that was
			 * triggered first
			 */
			u64 err_reg_status:60;
			/*
			 * Holds 60 LSBs from the Error register that was
			 * triggered first
			 */
			u8 errReg_id:4;
		};
	};
};

/* FME Next Error register */
struct feature_fme_next_error {
#define FME_NEXT_ERROR_MASK    ((1UL << 60) - 1)
	union {
		u64 csr;
		struct {
			/*
			 * Indicates the Error Register that was
			 * triggered second
			 */
			u64 err_reg_status:60;
			/*
			 * Holds 60 LSBs from the Error register that was
			 * triggered second
			 */
			u8  errReg_id:4;
		};
	};
};

/* RAS GreenBS Error Status register */
struct feature_fme_ras_gerror {
#define FME_RAS_GERROR_MASK    0xFFFFUL
	union {
		u64 csr;
		struct {
			/* thremal threshold AP1 */
			u8  temp_trash_ap1:1;
			/* thremal threshold AP2 */
			u8  temp_trash_ap2:1;
			u8  pcie_error:1;	/* pcie Error */
			u8  afufatal_error:1;	/* afu fatal error */
			u8  proc_hot:1;		/* Indicates a ProcHot event */
			/* Indicates an AFU PF/VF access mismatch */
			u8  afu_acc_mode_err:1;
			/* Injected Warning Error */
			u8  injected_warning_err:1;
			/* Indicates a Poison error from any of PCIe ports */
			u8  pcie_poison_Err:1;
			/* Green bitstream CRC Error */
			u8  gb_crc_err:1;
			/* Temperature threshold triggered AP6*/
			u8  temp_thresh_AP6:1;
			/* Power threshold triggered AP1 */
			u8  power_thresh_AP1:1;
			/* Power threshold triggered AP2 */
			u8  power_thresh_AP2:1;
			/* Indicates a MBP event */
			u8  mbp_err:1;
			u64 rsvd2:51;		/* Reserved */
		};
	};
};

/* RAS BlueBS Error Status register */
struct feature_fme_ras_berror {
#define FME_RAS_BERROR_MASK    0xFFFFUL
	union {
		u64 csr;
		struct {
			/* KTI Link layer error detected */
			u8  ktilink_fatal_err:1;
			/* tag-n-cache error detected */
			u8  tagcch_fatal_err:1;
			/* CCI error detected */
			u8  cci_fatal_err:1;
			/* KTI Protocol error detected */
			u8  ktiprpto_fatal_err:1;
			/* Fatal DRAM error detected */
			u8  dram_fatal_err:1;
			/* IOMMU detected */
			u8  iommu_fatal_err:1;
			/* Injected Fatal Error */
			u8  injected_fatal_err:1;
			u8  rsvd:1;
			/* Catastrophic IOMMU Error */
			u8  iommu_catast_err:1;
			/* Catastrophic CRC Error */
			u8  crc_catast_err:1;
			/* Catastrophic Thermal Error */
			u8  therm_catast_err:1;
			/* Injected Catastrophic Error */
			u8  injected_catast_err:1;
			u64 rsvd1:52;
		};
	};
};

/* RAS Warning Error Status register */
struct feature_fme_ras_werror {
#define FME_RAS_WERROR_MASK    0x1UL
	union {
		u64 csr;
		struct {
			/*
			 * Warning bit indicates that a green bitstream
			 * fatal event occurred
			 */
			u8  event_warn_err:1;
			u64 rsvd:63;		/* Reserved */
		};
	};
};

/* RAS Error injection register */
struct feature_fme_ras_error_inj {
#define FME_RAS_ERROR_INJ_MASK      0x7UL
	union {
		u64 csr;
		struct {
			u8  catast_error:1;	/* Catastrophic error flag */
			u8  fatal_error:1;	/* Fatal error flag */
			u8  warning_error:1;	/* Warning error flag */
			u64 rsvd:61;		/* Reserved */
		};
	};
};

/* FME ERR FEATURE */
struct feature_fme_err {
	struct feature_header header;
	struct feature_fme_error0 fme_err_mask;
	struct feature_fme_error0 fme_err;
	struct feature_fme_pcie0_error pcie0_err_mask;
	struct feature_fme_pcie0_error pcie0_err;
	struct feature_fme_pcie1_error pcie1_err_mask;
	struct feature_fme_pcie1_error pcie1_err;
	struct feature_fme_first_error fme_first_err;
	struct feature_fme_next_error fme_next_err;
	struct feature_fme_ras_gerror ras_gerr_mask;
	struct feature_fme_ras_gerror ras_gerr;
	struct feature_fme_ras_berror ras_berr_mask;
	struct feature_fme_ras_berror ras_berr;
	struct feature_fme_ras_werror ras_werr_mask;
	struct feature_fme_ras_werror ras_werr;
	struct feature_fme_ras_error_inj ras_error_inj;
};

/* FME Partial Reconfiguration Control */
struct feature_fme_pr_ctl {
	union {
		u64 csr;
		struct {
			u8  pr_reset:1;		/* Reset PR Engine */
			u8  rsvd3:3;		/* Reserved */
			u8  pr_reset_ack:1;	/* Reset PR Engine Ack */
			u8  rsvd4:3;		/* Reserved */
			u8  pr_regionid:2;	/* PR Region ID */
			u8  rsvd1:2;		/* Reserved */
			u8  pr_start_req:1;	/* PR Start Request */
			u8  pr_push_complete:1;	/* PR Data push complete */
			u8  pr_kind:1;		/* PR Data push complete */
			u32  rsvd:17;		/* Reserved */
			u32 config_data;	/* Config data TBD */
		};
	};
};

/* FME Partial Reconfiguration Status */
struct feature_fme_pr_status {
	union {
		u64 csr;
		struct {
			u16 pr_credit:9;	/* PR Credits */
			u8  rsvd2:7;		/* Reserved */
			u8  pr_status:1;	/* PR status */
			u8  rsvd:3;		/* Reserved */
			/* Altra PR Controller Block status */
			u8  pr_contoller_status:3;
			u8  rsvd1:1;            /* Reserved */
			u8  pr_host_status:4;   /* PR Host status */
			u8  rsvd3:4;		/* Reserved */
			/* Security Block Status fields (TBD) */
			u32 security_bstatus;
		};
	};
};

/* FME Partial Reconfiguration Data */
struct feature_fme_pr_data {
	union {
		u64 csr;	/* PR data from the raw-binary file */
		struct {
			/* PR data from the raw-binary file */
			u32 pr_data_raw;
			u32 rsvd;
		};
	};
};

/* FME PR Public Key */
struct feature_fme_pr_key {
	u64 key;		/* FME PR Public Hash */
};

/* FME PR FEATURE */
struct feature_fme_pr {
	struct feature_header header;
	/*Partial Reconfiguration control */
	struct feature_fme_pr_ctl	ccip_fme_pr_control;

	/* Partial Reconfiguration Status */
	struct feature_fme_pr_status	ccip_fme_pr_status;

	/* Partial Reconfiguration data */
	struct feature_fme_pr_data	ccip_fme_pr_data;

	/* Partial Reconfiguration data */
	u64				ccip_fme_pr_err;

	/* FME PR Publish HASH */
	struct feature_fme_pr_key fme_pr_pub_harsh0;
	struct feature_fme_pr_key fme_pr_pub_harsh1;
	struct feature_fme_pr_key fme_pr_pub_harsh2;
	struct feature_fme_pr_key fme_pr_pub_harsh3;

	/* FME PR Private HASH */
	struct feature_fme_pr_key fme_pr_priv_harsh0;
	struct feature_fme_pr_key fme_pr_priv_harsh1;
	struct feature_fme_pr_key fme_pr_priv_harsh2;
	struct feature_fme_pr_key fme_pr_priv_harsh3;

	/* FME PR License */
	struct feature_fme_pr_key fme_pr_license0;
	struct feature_fme_pr_key fme_pr_license1;
	struct feature_fme_pr_key fme_pr_license2;
	struct feature_fme_pr_key fme_pr_license3;

	/* FME PR Session Key */
	struct feature_fme_pr_key fme_pr_seskey0;
	struct feature_fme_pr_key fme_pr_seskey1;
	struct feature_fme_pr_key fme_pr_seskey2;
	struct feature_fme_pr_key fme_pr_seskey3;

	/* PR Interface ID */
	struct feature_fme_pr_key fme_pr_intfc_id0_l;
	struct feature_fme_pr_key fme_pr_intfc_id0_h;

	struct feature_fme_pr_key fme_pr_intfc_id1_l;
	struct feature_fme_pr_key fme_pr_intfc_id1_h;

	struct feature_fme_pr_key fme_pr_intfc_id2_l;
	struct feature_fme_pr_key fme_pr_intfc_id2_h;

	struct feature_fme_pr_key fme_pr_intfc_id3_l;
	struct feature_fme_pr_key fme_pr_intfc_id3_h;

	/* MSIX filed to be Added */
};

#define PORT_ERR_MASK		0xfff0703ff001f
struct feature_port_err_key {
	union {
		u64 csr;
		struct {
			/* Tx Channel0: Overflow */
			u8 tx_ch0_overflow:1;
			/* Tx Channel0: Invalid request encoding */
			u8 tx_ch0_invaldreq :1;
			/* Tx Channel0: Request with cl_len=3 not supported */
			u8 tx_ch0_cl_len3:1;
			/* Tx Channel0: Request with cl_len=2 not aligned 2CL */
			u8 tx_ch0_cl_len2:1;
			/* Tx Channel0: Request with cl_len=4 not aligned 4CL */
			u8 tx_ch0_cl_len4:1;

			u16 rsvd1:11;			/* Reserved */

			/* Tx Channel1: Overflow */
			u8 tx_ch1_overflow:1;
			/* Tx Channel1: Invalid request encoding */
			u8 tx_ch1_invaldreq:1;
			/* Tx Channel1: Request with cl_len=3 not supported */
			u8 tx_ch1_cl_len3:1;
			/* Tx Channel1: Request with cl_len=2 not aligned 2CL */
			u8 tx_ch1_cl_len2:1;
			/* Tx Channel1: Request with cl_len=4 not aligned 4CL */
			u8 tx_ch1_cl_len4:1;

			/* Tx Channel1: Insufficient data payload */
			u8 tx_ch1_insuff_data:1;
			/* Tx Channel1: Data payload overrun */
			u8 tx_ch1_data_overrun:1;
			/* Tx Channel1 : Incorrect address */
			u8 tx_ch1_incorr_addr:1;
			/* Tx Channel1 : NON-Zero SOP Detected */
			u8 tx_ch1_nzsop:1;
			/* Tx Channel1 : Illegal VC_SEL, atomic request VLO */
			u8 tx_ch1_illegal_vcsel:1;

			u8 rsvd2:6;			/* Reserved */

			/* MMIO Read Timeout in AFU */
			u8 mmioread_timeout:1;

			/* Tx Channel2: FIFO Overflow */
			u8 tx_ch2_fifo_overflow:1;

			/* MMIO read is not matching pending request */
			u8 unexp_mmio_resp:1;

			u8 rsvd3:5;			/* Reserved */

			/* Number of pending Requests: counter overflow */
			u8 tx_req_counter_overflow:1;
			/* Req with Address violating SMM Range */
			u8 llpr_smrr_err:1;
			/* Req with Address violating second SMM Range */
			u8 llpr_smrr2_err:1;
			/* Req with Address violating ME Stolen message */
			u8 llpr_mesg_err:1;
			/* Req with Address violating Generic Protected Range */
			u8 genprot_range_err:1;
			/* Req with Address violating Legacy Range low */
			u8 legrange_low_err:1;
			/* Req with Address violating Legacy Range High */
			u8 legrange_high_err:1;
			/* Req with Address violating VGA memory range */
			u8 vgmem_range_err:1;
			u8 page_fault_err:1;		/* Page fault */
			u8 pmr_err:1;			/* PMR Error */
			u8 ap6_event:1;		/* AP6 event */
			/* VF FLR detected on Port with PF access control */
			u8 vfflr_access_err:1;
			u16 rsvd4:12;			/* Reserved */
		};
	};
};

/* Port first error register, not contain all error bits in error register. */
struct feature_port_first_err_key {
	union {
		u64 csr;
		struct {
			u8 tx_ch0_overflow:1;
			u8 tx_ch0_invaldreq :1;
			u8 tx_ch0_cl_len3:1;
			u8 tx_ch0_cl_len2:1;
			u8 tx_ch0_cl_len4:1;
			u16 rsvd1:11;			/* Reserved */
			u8 tx_ch1_overflow:1;
			u8 tx_ch1_invaldreq:1;
			u8 tx_ch1_cl_len3:1;
			u8 tx_ch1_cl_len2:1;
			u8 tx_ch1_cl_len4:1;
			u8 tx_ch1_insuff_data:1;
			u8 tx_ch1_data_overrun:1;
			u8 tx_ch1_incorr_addr:1;
			u8 tx_ch1_nzsop:1;
			u8 tx_ch1_illegal_vcsel:1;
			u8 rsvd2:6;			/* Reserved */
			u8 mmioread_timeout:1;
			u8 tx_ch2_fifo_overflow:1;
			u8 rsvd3:6;			/* Reserved */
			u8 tx_req_counter_overflow:1;
			u32 rsvd4:23;			/* Reserved */
		};
	};
};

/* Port malformed Req0 */
struct feature_port_malformed_req0 {
	u64 header_lsb;
};

/* Port malformed Req1 */
struct feature_port_malformed_req1 {
	u64 header_msb;
};

/* Port debug register */
struct feature_port_debug {
	u64 port_debug;
};

/* PORT FEATURE ERROR */
struct feature_port_error {
	struct feature_header header;
	struct feature_port_err_key error_mask;
	struct feature_port_err_key port_error;
	struct feature_port_first_err_key port_first_error;
	struct feature_port_malformed_req0 malreq0;
	struct feature_port_malformed_req1 malreq1;
	struct feature_port_debug port_debug;
};

/* Port UMSG Capability */
struct feature_port_umsg_cap {
	union {
		u64 csr;
		struct {
			/* Number of umsg allocated to this port */
			u8 umsg_allocated;
			/* Enable / Disable UMsg engine for this port */
			u8 umsg_enable:1;
			/* Usmg initialization status */
			u8 umsg_init_complete:1;
			/* IOMMU can not translate the umsg base address */
			u8 umsg_trans_error:1;
			u64 rsvd:53;		/* Reserved */
		};
	};
};

/* Port UMSG base address */
struct feature_port_umsg_baseaddr {
	union {
		u64 csr;
		struct {
			u64 base_addr:48;	/* 48 bit physical address */
			u16 rsvd;		/* Reserved */
		};
	};
};

struct feature_port_umsg_mode {
	union {
		u64 csr;
		struct {
			u32 umsg_hint_enable;	/* UMSG hint enable/disable */
			u32 rsvd;		/* Reserved */
		};
	};
};

/* PORT FEATURE UMSG */
struct feature_port_umsg {
	struct feature_header header;
	struct feature_port_umsg_cap capability;
	struct feature_port_umsg_baseaddr baseaddr;
	struct feature_port_umsg_mode mode;
};

/* STP region supports mmap operation, so use page aligned size. */
#define PORT_FEATURE_STP_REGION_SIZE	PAGE_ALIGN(sizeof(struct feature_port_stp))

/* Port STP status register (for debug only)*/
struct feature_port_stp_status {
	union {
		u64 csr;
		struct {
			/* SLD Hub end-point read/write timeout */
			u8 sld_ep_timeout:1;
			/* Remote STP in reset/disable */
			u8 rstp_disabled:1;
			u8 unsupported_read:1;
			/* MMIO timeout detected and faked with a response */
			u8 mmio_timeout:1;
			u8 txfifo_count:4;
			u8 rxfifo_count:4;
			u8 txfifo_overflow:1;
			u8 txfifo_underflow:1;
			u8 rxfifo_overflow:1;
			u8 rxfifo_underflow:1;
			/* Number of MMIO write requests */
			u16 write_requests;
			/* Number of MMIO read requests */
			u16 read_requests;
			/* Number of MMIO read responses */
			u16 read_responses;
		};
	};
};

/*
 * PORT FEATURE STP
 * Most registers in STP region are not touched by driver, but mmapped to user
 * space. So they are not defined in below data structure, as its actual size
 * is 0x18c per spec.
 */
struct feature_port_stp {
	struct feature_header header;
	struct feature_port_stp_status stp_status;
};

#pragma pack()

struct feature_driver {
	const char *name;
	struct feature_ops *ops;
};

struct feature {
	const char *name;
	int resource_index;
	void __iomem *ioaddr;
	struct feature_ops *ops;
};

struct feature_platform_data {
	/* list the feature dev to cci_drvdata->port_dev_list. */
	struct list_head node;

	struct mutex lock;
	int excl_open;
	int open_count;
	struct cdev cdev;
	struct platform_device *dev;
	unsigned int disable_count;

	void *private;

	int num;
	int (*config_port)(struct platform_device *, u32, bool);
	struct platform_device *(*fpga_for_each_port)(struct platform_device *,
			void *, int (*match)(struct platform_device *, void *));
	struct feature features[0];
};

static inline int
feature_dev_use_excl_begin(struct feature_platform_data *pdata)
{
	/*
	 * If device file is opened with O_EXCL flag, check the open_count
	 * and set excl_open and increate open_count to ensure exclusive use.
	 */
	mutex_lock(&pdata->lock);
	if (pdata->open_count) {
		mutex_unlock(&pdata->lock);
		return -EBUSY;
	}
	pdata->excl_open = 1;
	pdata->open_count++;
	mutex_unlock(&pdata->lock);

	return 0;
}

static inline int feature_dev_use_begin(struct feature_platform_data *pdata)
{
	mutex_lock(&pdata->lock);
	if (pdata->excl_open) {
		mutex_unlock(&pdata->lock);
		return -EBUSY;
	}
	pdata->open_count++;
	mutex_unlock(&pdata->lock);

	return 0;
}

static inline void __feature_dev_use_end(struct feature_platform_data *pdata)
{
	pdata->excl_open = 0;
	pdata->open_count--;
}

static inline void feature_dev_use_end(struct feature_platform_data *pdata)
{
	mutex_lock(&pdata->lock);
	__feature_dev_use_end(pdata);
	mutex_unlock(&pdata->lock);
}

static inline void
fpga_pdata_set_private(struct feature_platform_data *pdata, void *private)
{
	pdata->private = private;
}

static inline void *fpga_pdata_get_private(struct feature_platform_data *pdata)
{
	return pdata->private;
}

struct feature_ops {
	int (*init)(struct platform_device *pdev, struct feature *feature);
	void (*uinit)(struct platform_device *pdev, struct feature *feature);
	long (*ioctl)(struct platform_device *pdev, struct feature *feature,
				unsigned int cmd, unsigned long arg);
	int (*test)(struct platform_device *pdev, struct feature *feature);
};

enum fme_feature_id {
	FME_FEATURE_ID_HEADER = 0x0,

	FME_FEATURE_ID_THERMAL_MGMT	= 0x1,
	FME_FEATURE_ID_POWER_MGMT = 0x2,
	FME_FEATURE_ID_GLOBAL_PERF = 0x3,
	FME_FEATURE_ID_GLOBAL_ERR = 0x4,
	FME_FEATURE_ID_PR_MGMT = 0x5,

	/* one for fme header. */
	FME_FEATURE_ID_MAX = 0x6,
};

enum port_feature_id {
	PORT_FEATURE_ID_HEADER = 0x0,
	PORT_FEATURE_ID_ERROR = 0x1,
	PORT_FEATURE_ID_UMSG = 0x2,
	PORT_FEATURE_ID_PR = 0x3,
	PORT_FEATURE_ID_STP = 0x4,
	PORT_FEATURE_ID_UAFU = 0x5,
	PORT_FEATURE_ID_MAX = 0x6,
};


int fme_feature_num(void);
int port_feature_num(void);

#define FPGA_FEATURE_DEV_FME		"intel-fpga-fme"
#define FPGA_FEATURE_DEV_PORT		"intel-fpga-port"

void feature_platform_data_add(struct feature_platform_data *pdata,
			       int index, const char *name,
			       int resource_index, void __iomem *ioaddr);
int feature_platform_data_size(int num);
struct feature_platform_data *
feature_platform_data_alloc_and_init(struct platform_device *dev, int num);

void fpga_dev_feature_uinit(struct platform_device *pdev);
int fpga_dev_feature_init(struct platform_device *pdev,
			  struct feature_driver *feature_drvs);

enum fpga_devt_type {
	FPGA_DEVT_FME,
	FPGA_DEVT_PORT,
	FPGA_DEVT_MAX,
};

void fpga_chardev_uinit(void);
int fpga_chardev_init(void);
dev_t fpga_get_devt(enum fpga_devt_type type, int id);
int fpga_register_dev_ops(struct platform_device *pdev,
			  const struct file_operations *fops,
			  struct module *owner);
void fpga_unregister_dev_ops(struct platform_device *pdev);

int fpga_port_id(struct platform_device *pdev);

static inline int fpga_port_check_id(struct platform_device *pdev,
				     void *pport_id)
{
	return fpga_port_id(pdev) == *(int *)pport_id;
}

void __fpga_port_enable(struct platform_device *pdev);
int __fpga_port_disable(struct platform_device *pdev);

static inline void fpga_port_enable(struct platform_device *pdev)
{
	struct feature_platform_data *pdata = dev_get_platdata(&pdev->dev);

	mutex_lock(&pdata->lock);
	__fpga_port_enable(pdev);
	mutex_unlock(&pdata->lock);
}

static inline int fpga_port_disable(struct platform_device *pdev)
{
	struct feature_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int ret;

	mutex_lock(&pdata->lock);
	ret = __fpga_port_disable(pdev);
	mutex_unlock(&pdata->lock);

	return ret;
}

static inline int __fpga_port_reset(struct platform_device *pdev)
{
	int ret;

	ret = __fpga_port_disable(pdev);
	if (ret)
		return ret;

	__fpga_port_enable(pdev);
	return 0;
}

static inline int fpga_port_reset(struct platform_device *pdev)
{
	struct feature_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int ret;

	mutex_lock(&pdata->lock);
	ret = __fpga_port_reset(pdev);
	mutex_unlock(&pdata->lock);
	return ret;
}

static inline
struct platform_device *fpga_inode_to_feature_dev(struct inode *inode)
{
	struct feature_platform_data *pdata;

	pdata = container_of(inode->i_cdev, struct feature_platform_data, cdev);
	return pdata->dev;
}

static inline void __iomem *
get_feature_ioaddr_by_index(struct device *dev, int index)
{
	struct feature_platform_data *pdata = dev_get_platdata(dev);

	return pdata->features[index].ioaddr;
}

static inline bool is_feature_present(struct device *dev, int index)
{
	return !!get_feature_ioaddr_by_index(dev, index);
}

static inline struct device *
fpga_feature_dev_to_pcidev(struct platform_device *dev)
{
	return dev->dev.parent->parent;
}

static inline struct device *
fpga_pdata_to_pcidev(struct feature_platform_data *pdata)
{
	return fpga_feature_dev_to_pcidev(pdata->dev);
}

#define fpga_dev_for_each_feature(pdata, feature)			    \
	for ((feature) = (pdata)->features;				    \
	   (feature) < (pdata)->features + (pdata)->num; (feature)++)

void check_features_header(struct pci_dev *pdev, struct feature_header *hdr,
			   enum fpga_devt_type type, int id);

/*
 * Wait register's _field to be changed to the given value (_expect's _field)
 * by polling with given interval and timeout.
 */
#define fpga_wait_register_field(_field, _expect, _reg_addr, _timeout, _invl)\
({									     \
	int wait = 0;							     \
	int ret = -ETIMEDOUT;						     \
	typeof(_expect) value;						     \
	for (; wait <= _timeout; wait += _invl) {			     \
		value.csr = readq(_reg_addr);				     \
		if (_expect._field == value._field) {			     \
			ret = 0;					     \
			break;						     \
		}							     \
		udelay(_invl);						     \
	}								     \
	ret;								     \
})

#endif
