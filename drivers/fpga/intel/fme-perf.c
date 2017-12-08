/*
 * Driver for FPGA Global Performance
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

#include "feature-dev.h"
#include "fme.h"

struct perf_obj_attributte {
	struct attribute attr;
	ssize_t (*show)(struct perf_object *pobj, char *buf);
	ssize_t (*store)(struct perf_object *pobj,
			 const char *buf, size_t n);
};

#define to_perf_obj_attr(_attr)					\
		container_of(_attr, struct perf_obj_attributte, attr)
#define to_perf_obj(_kobj)					\
		container_of(_kobj, struct perf_object, kobj)

static ssize_t perf_obj_attr_show(struct kobject *kobj,
				     struct attribute *__attr, char *buf)
{
	struct perf_obj_attributte *attr = to_perf_obj_attr(__attr);
	struct perf_object *pobj = to_perf_obj(kobj);
	ssize_t ret = -EIO;

	if (attr->show)
		ret = attr->show(pobj, buf);
	return ret;
}

static ssize_t perf_obj_attr_store(struct kobject *kobj,
				      struct attribute *__attr,
				      const char *buf, size_t n)
{
	struct perf_obj_attributte *attr = to_perf_obj_attr(__attr);
	struct perf_object *pobj = to_perf_obj(kobj);
	ssize_t ret = -EIO;

	if (attr->store)
		ret = attr->store(pobj, buf, n);
	return ret;
}

static const struct sysfs_ops perf_obj_sysfs_ops = {
	.show = perf_obj_attr_show,
	.store = perf_obj_attr_store,
};

static void perf_obj_release(struct kobject *kobj)
{
	kfree(to_perf_obj(kobj));
}

static struct kobj_type perf_obj_ktype = {
	.sysfs_ops = &perf_obj_sysfs_ops,
	.release = perf_obj_release,
};

#define PERF_OBJ_ATTR(_name, _filename, _mode, _show, _store)	\
struct perf_obj_attributte perf_obj_attr_##_name =		\
	__ATTR(_filename, _mode, _show, _store)
#define PERF_OBJ_ATTR_RW(_name)					\
	struct perf_obj_attributte perf_obj_attr_##_name = __ATTR_RW(_name)
#define PERF_OBJ_ATTR_RO(_name)					\
	struct perf_obj_attributte perf_obj_attr_##_name = __ATTR_RO(_name)
#define PERF_OBJ_ATTR_WO(_name)					\
	struct perf_obj_attributte perf_obj_attr_##_name = __ATTR_WO(_name)

static ssize_t revision_show(struct perf_object *pobj, char *buf)
{
	struct feature_fme_gperf *gperf;
	struct feature_header header;

	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);
	header.csr = readq(&gperf->header);

	return scnprintf(buf, PAGE_SIZE, "%d\n", header.revision);
}
static PERF_OBJ_ATTR_RO(revision);

static ssize_t clock_show(struct perf_object *pobj, char *buf)
{
	struct feature_fme_gperf *gperf;
	struct feature_fme_fpmon_clk_ctr clk;

	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);
	clk.afu_interf_clock = readq(&gperf->clk);

	return scnprintf(buf, PAGE_SIZE, "0x%llx\n", clk.afu_interf_clock);
}
static PERF_OBJ_ATTR_RO(clock);

static struct attribute *clock_attrs[] = {
	&perf_obj_attr_revision.attr,
	&perf_obj_attr_clock.attr,
	NULL,
};

static struct attribute_group clock_attr_group = {
	.attrs = clock_attrs,
};

static ssize_t freeze_show(struct perf_object *pobj, char *buf)
{
	struct feature_fme_gperf *gperf;
	struct feature_fme_fpmon_ch_ctl ctl;

	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);
	ctl.csr = readq(&gperf->ch_ctl);
	return scnprintf(buf, PAGE_SIZE, "%d\n", ctl.freeze);
}

static ssize_t freeze_store(struct perf_object *pobj,
		 const char *buf, size_t n)
{
	struct feature_platform_data *pdata = dev_get_platdata(pobj->fme_dev);
	struct feature_fme_gperf *gperf;
	struct feature_fme_fpmon_ch_ctl ctl;
	bool state;

	if (strtobool(buf, &state))
		return -EINVAL;

	mutex_lock(&pdata->lock);
	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);
	ctl.csr = readq(&gperf->ch_ctl);
	ctl.freeze = state;
	writeq(ctl.csr, &gperf->ch_ctl);
	mutex_unlock(&pdata->lock);

	return n;
}
static PERF_OBJ_ATTR_RW(freeze);

#define GPERF_TIMEOUT	30

static ssize_t read_cache_counter(struct perf_object *pobj, char *buf,
				  u8 channel, enum gperf_cache_events event)
{
	struct device *fme_dev = pobj->fme_dev;
	struct feature_platform_data *pdata = dev_get_platdata(fme_dev);
	struct feature_fme_gperf *gperf;
	struct feature_fme_fpmon_ch_ctl ctl;
	struct feature_fme_fpmon_ch_ctr ctr0, ctr1;
	u64 counter;

	mutex_lock(&pdata->lock);
	gperf = get_feature_ioaddr_by_index(fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);

	/* set channel access type and cache event code. */
	ctl.csr = readq(&gperf->ch_ctl);
	ctl.cci_chsel = channel;
	ctl.cache_event = event;
	writeq(ctl.csr, &gperf->ch_ctl);

	/* check the event type in the counter registers */
	ctr0.event_code = event;

	if (fpga_wait_register_field(event_code, ctr0,
				     &gperf->ch_ctr0, GPERF_TIMEOUT, 1)) {
		dev_err(fme_dev,
		"timeout, unmatched cache event type in counter registers.\n");
		mutex_unlock(&pdata->lock);
		return -ETIMEDOUT;
	}

	ctr0.csr = readq(&gperf->ch_ctr0);
	ctr1.csr = readq(&gperf->ch_ctr1);
	counter = ctr0.cache_counter + ctr1.cache_counter;
	mutex_unlock(&pdata->lock);

	return scnprintf(buf, PAGE_SIZE, "0x%llx\n", counter);
}

#define CACHE_SHOW(name, type, event)					\
static ssize_t name##_show(struct perf_object *pobj, char *buf)		\
{									\
	return read_cache_counter(pobj, buf, type, event);		\
}									\
static PERF_OBJ_ATTR_RO(name)

CACHE_SHOW(read_hit, CACHE_CHANNEL_RD, CACHE_RD_HIT);
CACHE_SHOW(read_miss, CACHE_CHANNEL_RD, CACHE_RD_MISS);
CACHE_SHOW(write_hit, CACHE_CHANNEL_WR, CACHE_WR_HIT);
CACHE_SHOW(write_miss, CACHE_CHANNEL_WR, CACHE_WR_MISS);
CACHE_SHOW(hold_request, CACHE_CHANNEL_RD, CACHE_HOLD_REQ);
CACHE_SHOW(tx_req_stall, CACHE_CHANNEL_RD, CACHE_TX_REQ_STALL);
CACHE_SHOW(rx_req_stall, CACHE_CHANNEL_RD, CACHE_RX_REQ_STALL);
CACHE_SHOW(rx_eviction, CACHE_CHANNEL_RD, CACHE_EVICTIONS);
CACHE_SHOW(data_write_port_contention, CACHE_CHANNEL_WR,
	   CACHE_DATA_WR_PORT_CONTEN);
CACHE_SHOW(tag_write_port_contention, CACHE_CHANNEL_WR,
	   CACHE_TAG_WR_PORT_CONTEN);

static struct attribute *cache_attrs[] = {
	&perf_obj_attr_read_hit.attr,
	&perf_obj_attr_read_miss.attr,
	&perf_obj_attr_write_hit.attr,
	&perf_obj_attr_write_miss.attr,
	&perf_obj_attr_hold_request.attr,
	&perf_obj_attr_data_write_port_contention.attr,
	&perf_obj_attr_tag_write_port_contention.attr,
	&perf_obj_attr_tx_req_stall.attr,
	&perf_obj_attr_rx_req_stall.attr,
	&perf_obj_attr_rx_eviction.attr,
	&perf_obj_attr_freeze.attr,
	NULL,
};

static struct attribute_group cache_attr_group = {
	.name = "cache",
	.attrs = cache_attrs,
};

static const struct attribute_group *perf_dev_attr_groups[] = {
	&clock_attr_group,
	&cache_attr_group,
	NULL,
};

ssize_t vtd_freeze_show(struct perf_object *pobj, char *buf)
{
	struct feature_fme_fpmon_vtd_ctl ctl;
	struct feature_fme_gperf *gperf;

	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					FME_FEATURE_ID_GLOBAL_PERF);
	ctl.csr = readq(&gperf->vtd_ctl);
	return scnprintf(buf, PAGE_SIZE, "%d\n", ctl.freeze);
}

ssize_t vtd_freeze_store(struct perf_object *pobj,
		 const char *buf, size_t n)
{
	struct feature_platform_data *pdata;
	struct feature_fme_fpmon_vtd_ctl ctl;
	struct feature_fme_gperf *gperf;
	bool state;

	if (strtobool(buf, &state))
		return -EINVAL;

	pdata = dev_get_platdata(pobj->fme_dev);
	mutex_lock(&pdata->lock);
	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);
	ctl.csr = readq(&gperf->vtd_ctl);
	ctl.freeze = state;
	writeq(ctl.csr, &gperf->vtd_ctl);
	mutex_unlock(&pdata->lock);

	return n;
}

static PERF_OBJ_ATTR(vtd_freeze, freeze, 0644, vtd_freeze_show,
		     vtd_freeze_store);
static struct attribute *iommu_top_attrs[] = {
	&perf_obj_attr_vtd_freeze.attr,
	NULL,
};

static struct attribute_group iommu_top_attr_group = {
	.attrs = iommu_top_attrs,
};

static const struct attribute_group *iommu_top_attr_groups[] = {
	&iommu_top_attr_group,
	NULL,
};

static ssize_t read_iommu_counter(struct perf_object *pobj,
				  enum gperf_vtd_events base_event, char *buf)
{
	struct feature_platform_data *pdata;
	struct feature_fme_fpmon_vtd_ctl ctl;
	struct feature_fme_fpmon_vtd_ctr ctr;
	struct feature_fme_gperf *gperf;
	enum gperf_vtd_events event = base_event + pobj->id;
	u64 counter;

	pdata = dev_get_platdata(pobj->fme_dev);
	mutex_lock(&pdata->lock);
	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);
	ctl.csr = readq(&gperf->vtd_ctl);
	ctl.vtd_evtcode = event;
	writeq(ctl.csr, &gperf->vtd_ctl);

	ctr.event_code = event;

	if (fpga_wait_register_field(event_code, ctr,
				     &gperf->vtd_ctr, GPERF_TIMEOUT, 1)) {
		dev_err(pobj->fme_dev,
		"timeout, unmatched VTd event type in counter registers.\n");
		mutex_unlock(&pdata->lock);
		return -ETIMEDOUT;
	}

	ctr.csr = readq(&gperf->vtd_ctr);
	counter = ctr.vtd_counter;
	mutex_unlock(&pdata->lock);

	return scnprintf(buf, PAGE_SIZE, "0x%llx\n", counter);
}

#define VTD_SHOW(name, base_event)					\
static ssize_t name##_show(struct perf_object *pobj, char *buf)		\
{									\
	return read_iommu_counter(pobj, base_event, buf);		\
}									\
static PERF_OBJ_ATTR_RO(name)

VTD_SHOW(read_transaction, VTD_AFU0_MEM_RD_TRANS);
VTD_SHOW(write_transaction, VTD_AFU0_MEM_WR_TRANS);
VTD_SHOW(tlb_read_hit, VTD_AFU0_TLB_RD_HIT);
VTD_SHOW(tlb_write_hit, VTD_AFU0_TLB_WR_HIT);

static struct attribute *iommu_attrs[] = {
	&perf_obj_attr_read_transaction.attr,
	&perf_obj_attr_write_transaction.attr,
	&perf_obj_attr_tlb_read_hit.attr,
	&perf_obj_attr_tlb_write_hit.attr,
	NULL,
};

static struct attribute_group iommu_attr_group = {
	.attrs = iommu_attrs,
};

static const struct attribute_group *iommu_attr_groups[] = {
	&iommu_attr_group,
	NULL,
};

static bool fabric_pobj_is_enabled(struct perf_object *pobj,
		struct feature_fme_gperf *gperf)
{
	struct feature_fme_fpmon_fab_ctl ctl;

	ctl.csr = readq(&gperf->fab_ctl);

	if (ctl.port_filter == FAB_DISABLE_FILTER)
		return pobj->id == PERF_OBJ_ROOT_ID;

	return pobj->id == ctl.port_id;
}

static ssize_t read_fabric_counter(struct perf_object *pobj,
				  enum gperf_fab_events fab_event, char *buf)
{
	struct feature_platform_data *pdata = dev_get_platdata(pobj->fme_dev);
	struct feature_fme_fpmon_fab_ctl ctl;
	struct feature_fme_fpmon_fab_ctr ctr;
	struct feature_fme_gperf *gperf;
	u64 counter = 0;

	mutex_lock(&pdata->lock);
	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);

	/* if it is disabled, force the counter to return zero. */
	if (!fabric_pobj_is_enabled(pobj, gperf))
		goto exit;

	ctl.csr = readq(&gperf->fab_ctl);
	ctl.fab_evtcode = fab_event;
	writeq(ctl.csr, &gperf->fab_ctl);

	ctr.event_code = fab_event;

	if (fpga_wait_register_field(event_code, ctr,
				     &gperf->fab_ctr, GPERF_TIMEOUT, 1)) {
		dev_err(pobj->fme_dev,
		"timeout, unmatched VTd event type in counter registers.\n");
		mutex_unlock(&pdata->lock);
		return -ETIMEDOUT;
	}

	ctr.csr = readq(&gperf->fab_ctr);
	counter = ctr.fab_cnt;
exit:
	mutex_unlock(&pdata->lock);
	return scnprintf(buf, PAGE_SIZE, "0x%llx\n", counter);
}

#define FAB_SHOW(name, event)						\
static ssize_t name##_show(struct perf_object *pobj, char *buf)		\
{									\
	return read_fabric_counter(pobj, event, buf);			\
}									\
static PERF_OBJ_ATTR_RO(name)

FAB_SHOW(pcie0_read, FAB_PCIE0_RD);
FAB_SHOW(pcie0_write, FAB_PCIE0_WR);
FAB_SHOW(pcie1_read, FAB_PCIE1_RD);
FAB_SHOW(pcie1_write, FAB_PCIE1_WR);
FAB_SHOW(upi_read, FAB_UPI_RD);
FAB_SHOW(upi_write, FAB_UPI_WR);
FAB_SHOW(mmio_read, FAB_MMIO_RD);
FAB_SHOW(mmio_write, FAB_MMIO_WR);

static ssize_t fab_enable_show(struct perf_object *pobj, char *buf)
{
	struct feature_fme_gperf *gperf;
	int status;

	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);

	status = fabric_pobj_is_enabled(pobj, gperf);
	return scnprintf(buf, PAGE_SIZE, "%d\n", status);
}

/*
 * If enable one port or all port event counter in fabric, other
 * fabric event counter originally enabled will be disable automatically.
 */
static ssize_t fab_enable_store(struct perf_object *pobj,
		 const char *buf, size_t n)
{
	struct feature_platform_data *pdata  = dev_get_platdata(pobj->fme_dev);
	struct feature_fme_fpmon_fab_ctl ctl;
	struct feature_fme_gperf *gperf;
	bool state;

	if (strtobool(buf, &state))
		return -EINVAL;

	if (!state)
		return -EINVAL;

	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);

	/* if it is already enabled. */
	if (fabric_pobj_is_enabled(pobj, gperf))
		return n;

	mutex_lock(&pdata->lock);
	ctl.csr = readq(&gperf->fab_ctl);
	if (pobj->id == PERF_OBJ_ROOT_ID)
		ctl.port_filter = FAB_DISABLE_FILTER;
	else {
		ctl.port_filter = FAB_ENABLE_FILTER;
		ctl.port_id = pobj->id;
	}

	writeq(ctl.csr, &gperf->fab_ctl);
	mutex_unlock(&pdata->lock);

	return n;
}

static PERF_OBJ_ATTR(fab_enable, enable, 0644, fab_enable_show,
		     fab_enable_store);

static struct attribute *fabric_attrs[] = {
	&perf_obj_attr_pcie0_read.attr,
	&perf_obj_attr_pcie0_write.attr,
	&perf_obj_attr_pcie1_read.attr,
	&perf_obj_attr_pcie1_write.attr,
	&perf_obj_attr_upi_read.attr,
	&perf_obj_attr_upi_write.attr,
	&perf_obj_attr_mmio_read.attr,
	&perf_obj_attr_mmio_write.attr,
	&perf_obj_attr_fab_enable.attr,
	NULL,
};

static struct attribute_group fabric_attr_group = {
	.attrs = fabric_attrs,
};

static const struct attribute_group *fabric_attr_groups[] = {
	&fabric_attr_group,
	NULL,
};

static ssize_t fab_freeze_show(struct perf_object *pobj, char *buf)
{
	struct feature_fme_gperf *gperf;
	struct feature_fme_fpmon_fab_ctl ctl;

	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);
	ctl.csr = readq(&gperf->fab_ctl);
	return scnprintf(buf, PAGE_SIZE, "%d\n", ctl.freeze);
}

static ssize_t fab_freeze_store(struct perf_object *pobj,
		 const char *buf, size_t n)
{
	struct feature_platform_data *pdata = dev_get_platdata(pobj->fme_dev);
	struct feature_fme_gperf *gperf;
	struct feature_fme_fpmon_fab_ctl ctl;
	bool state;

	if (strtobool(buf, &state))
		return -EINVAL;

	mutex_lock(&pdata->lock);
	gperf = get_feature_ioaddr_by_index(pobj->fme_dev,
					    FME_FEATURE_ID_GLOBAL_PERF);
	ctl.csr = readq(&gperf->fab_ctl);
	ctl.freeze = state;
	writeq(ctl.csr, &gperf->fab_ctl);
	mutex_unlock(&pdata->lock);

	return n;
}

static PERF_OBJ_ATTR(fab_freeze, freeze, 0644, fab_freeze_show,
		     fab_freeze_store);

static struct attribute *fabric_top_attrs[] = {
	&perf_obj_attr_fab_freeze.attr,
	NULL,
};

static struct attribute_group fabric_top_attr_group = {
	.attrs = fabric_top_attrs,
};

static const struct attribute_group *fabric_top_attr_groups[] = {
	&fabric_attr_group,
	&fabric_top_attr_group,
	NULL,
};

static struct perf_object *
create_perf_obj(struct device *fme_dev, struct kobject *parent, int id,
		const struct attribute_group **groups, const char *name)
{
	struct perf_object *pobj;
	int ret;

	pobj = kzalloc(sizeof(*pobj), GFP_KERNEL);
	if (!pobj)
		return ERR_PTR(-ENOMEM);

	pobj->id = id;
	pobj->fme_dev = fme_dev;
	pobj->attr_groups = groups;
	INIT_LIST_HEAD(&pobj->node);
	INIT_LIST_HEAD(&pobj->children);

	if (id != PERF_OBJ_ROOT_ID)
		ret = kobject_init_and_add(&pobj->kobj, &perf_obj_ktype,
				   parent, "%s%d", name, id);
	else
		ret = kobject_init_and_add(&pobj->kobj, &perf_obj_ktype,
				   parent, "%s", name);
	if (ret)
		goto put_exit;

	if (pobj->attr_groups) {
		ret = sysfs_create_groups(&pobj->kobj, pobj->attr_groups);
		if (ret)
			goto put_exit;
	}

	return pobj;

put_exit:
	kobject_put(&pobj->kobj);
	return ERR_PTR(ret);
}

static void destroy_perf_obj(struct perf_object *pobj)
{
	struct perf_object *obj, *obj_tmp;

	list_for_each_entry_safe(obj, obj_tmp, &pobj->children, node)
		destroy_perf_obj(obj);

	list_del(&pobj->node);
	if (pobj->attr_groups)
		sysfs_remove_groups(&pobj->kobj, pobj->attr_groups);
	kobject_put(&pobj->kobj);
}

#define PERF_MAX_PORT_NUM	2

static int create_perf_iommu_obj(struct perf_object *perf_dev)
{
	struct perf_object *pobj;
	struct feature_fme_header *fme_hdr;
	struct feature_fme_capability fme_capability;
	int i;

	fme_hdr = get_feature_ioaddr_by_index(perf_dev->fme_dev,
					      FME_FEATURE_ID_HEADER);

	/* check if iommu is not supported on this device. */
	fme_capability.csr = readq(&fme_hdr->capability);
	if (!fme_capability.iommu_support)
		return 0;

	pobj = create_perf_obj(perf_dev->fme_dev, &perf_dev->kobj,
			   PERF_OBJ_ROOT_ID, iommu_top_attr_groups, "iommu");
	if (IS_ERR(pobj))
		return PTR_ERR(pobj);

	list_add(&pobj->node, &perf_dev->children);

	for (i = 0; i < PERF_MAX_PORT_NUM; i++) {
		struct perf_object *obj;

		obj = create_perf_obj(perf_dev->fme_dev, &pobj->kobj, i,
				       iommu_attr_groups, "afu");
		if (IS_ERR(obj))
			return PTR_ERR(obj);

		list_add(&obj->node, &pobj->children);
	}

	return 0;
}

static int create_perf_fabric_obj(struct perf_object *perf_dev)
{
	struct perf_object *pobj;
	int i;

	pobj = create_perf_obj(perf_dev->fme_dev, &perf_dev->kobj,
			   PERF_OBJ_ROOT_ID, fabric_top_attr_groups, "fabric");
	if (IS_ERR(pobj))
		return PTR_ERR(pobj);

	list_add(&pobj->node, &perf_dev->children);

	for (i = 0; i < PERF_MAX_PORT_NUM; i++) {
		struct perf_object *obj;

		obj = create_perf_obj(perf_dev->fme_dev, &pobj->kobj, i,
				       fabric_attr_groups, "port");
		if (IS_ERR(obj))
			return PTR_ERR(obj);

		list_add(&obj->node, &pobj->children);
	}

	return 0;
}

static struct perf_object *create_perf_dev(struct platform_device *pdev)
{
	return create_perf_obj(&pdev->dev, &pdev->dev.kobj,
			   PERF_OBJ_ROOT_ID, perf_dev_attr_groups, "perf");
}

static int fme_perf_init(struct platform_device *pdev, struct feature *feature)
{
	struct feature_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct fpga_fme *fme;
	struct perf_object *perf_dev;
	int ret;

	perf_dev = create_perf_dev(pdev);
	if (IS_ERR(perf_dev))
		return PTR_ERR(perf_dev);

	ret = create_perf_iommu_obj(perf_dev);
	if (ret) {
		destroy_perf_obj(perf_dev);
		return ret;
	}

	ret = create_perf_fabric_obj(perf_dev);
	if (ret) {
		destroy_perf_obj(perf_dev);
		return ret;
	}

	fme = fpga_pdata_get_private(pdata);
	fme->perf_dev = perf_dev;
	return 0;
}

static void
fme_perf_uinit(struct platform_device *pdev, struct feature *feature)
{
	struct feature_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct fpga_fme *fme;

	fme = fpga_pdata_get_private(pdata);
	destroy_perf_obj(fme->perf_dev);
	fme->perf_dev = NULL;
}

struct feature_ops global_perf_ops = {
	.init = fme_perf_init,
	.uinit = fme_perf_uinit,
};
