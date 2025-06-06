#ifndef __ASV_EXYNOS9945_H__
#define __ASV_EXYNOS9945_H__

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/kobject.h>

#define ASV_TABLE_BASE	(0x10009000)
#define ID_TABLE_BASE	(0x10000000)

static struct asv_power_model {
	unsigned power_model_0:8;
	unsigned power_model_1:8;
	unsigned power_model_2:8;
	unsigned power_model_3:8;
	unsigned power_model_4:8;
	unsigned power_model_5:8;
	unsigned power_model_6:8;
	unsigned power_model_7:8;
	unsigned power_model_modify_0:8;
	unsigned power_model_modify_1:8;
	unsigned power_model_modify_2:8;
	unsigned power_model_modify_3:8;
	unsigned power_model_modify_4:8;
	unsigned power_model_modify_5:8;
	unsigned power_model_modify_6:8;
	unsigned power_model_modify_7:8;
	unsigned power_model_ver:8;
	unsigned power_rsvd:24;
} power_model;

static struct dentry *rootdir;

struct asv_tbl_info {
	unsigned bigcpu_asv_group:4;
	unsigned midhcpu_asv_group:4;
	unsigned midlcpu_asv_group:4;
	unsigned littlecpu_asv_group:4;
	unsigned dsu_asv_group:4;
	unsigned g3d_asv_group:4;
	unsigned cp_cpu_asv_group:4;
	unsigned cp_asv_group:4;
	unsigned npu0_asv_group:4;
	unsigned npu1_asv_group:4;
	unsigned mif_asv_group:4;
	unsigned int_asv_group:4;
	unsigned intsci_asv_group:4;
	unsigned cam_asv_group:4;
	unsigned dsp_asv_group:4;
	unsigned aud_asv_group:4;
	unsigned disp_asv_group:4;
	unsigned icpu_asv_group:4;
	unsigned cp_mcw_asv_group:4;
	unsigned gnss_asv_group:4;
	unsigned alive_asv_group:4;
	unsigned domain_21_asv_group:4;
	unsigned domain_22_asv_group:4;
	unsigned domain_23_asv_group:4;

	unsigned asv_table_version:7;
	unsigned asv_method:1;
	unsigned asb_version:7;
	unsigned vtyp_modify_enable:1;
	unsigned asvg_version:4;
	unsigned asb_pgm_reserved:12;
};

struct id_tbl_info {
	unsigned int reserved_0;

	unsigned int chip_id_2;

	unsigned chip_id_1:12;
	unsigned reserved_2_1:4;

	unsigned char chip_id_reserved0:8;
	unsigned char chip_id_reserved1:8;
	unsigned char chip_id_reserved2:8;
	
	unsigned char asb_test_version:8;
	unsigned char scam_scale_reserved1:8;
	unsigned char scam_scale_reserved2:8;
	unsigned char scam_scale_reserved3:8;
	unsigned char scam_scale_reserved4:8;

	unsigned short sub_rev:4;
	unsigned short main_rev:4;
	unsigned char lp4x_pkg_revision:4;
	unsigned char lp5_pkg_revision:4;
	unsigned char eds_test_year:8;
	unsigned char eds_test_month:8;
	unsigned char eds_test_day:8;
	unsigned char chip_id_reserved3:8;

};

#define ASV_INFO_ADDR_CNT	(sizeof(struct asv_tbl_info) / 4)
#define ID_INFO_ADDR_CNT	(sizeof(struct id_tbl_info) / 4)

static struct asv_tbl_info asv_tbl;
static struct id_tbl_info id_tbl;

int asv_get_grp(unsigned int id)
{
	int grp = -1;

	switch (id) {
	case MIF:
		grp = asv_tbl.mif_asv_group;
		break;
	case INT:
	case INTCAM:
		grp = asv_tbl.int_asv_group;
		break;
	case CPUCL0:
		grp = asv_tbl.littlecpu_asv_group;
		break;
	case DSU:
		grp = asv_tbl.dsu_asv_group;
		break;
	case CPUCL1L:
		grp = asv_tbl.midlcpu_asv_group;
		break;
	case CPUCL1H:
		grp = asv_tbl.midhcpu_asv_group;
		break;
	case CPUCL2:
		grp = asv_tbl.bigcpu_asv_group;
		break;
	case G3D:
		grp = asv_tbl.g3d_asv_group;
		break;
	case CAM:
	case DISP:
		grp = asv_tbl.cam_asv_group;
		break;
	case NPU1:
		grp = asv_tbl.npu1_asv_group;
		break;
	case NPU0:
	case AUD:
		grp = asv_tbl.npu0_asv_group;
		break;
	case CP:
		grp = asv_tbl.cp_asv_group;
		break;
	case INTSCI:
		grp = asv_tbl.intsci_asv_group;
		break;
	default:
		pr_info("Un-support asv grp %d\n", id);
	}

	return grp;
}

int asv_get_ids_info(unsigned int id)
{
	return 0;
}

int asv_get_table_ver(void)
{
	return asv_tbl.asv_table_version;
}

void id_get_rev(unsigned int *main_rev, unsigned int *sub_rev)
{
#if IS_ENABLED(CONFIG_ACPM_DVFS)
	*main_rev = id_tbl.main_rev;
	*sub_rev =  id_tbl.sub_rev;
#endif
	*main_rev = 0;
	*sub_rev = 0;
}

int id_get_product_line(void)
{
	return id_tbl.chip_id_1 >> 10;
} EXPORT_SYMBOL(id_get_product_line);


int id_get_asb_ver(void)
{
	return asv_tbl.asb_version;
}

int print_asv_table(char *buf)
{
	ssize_t size = 0;

	size += sprintf(buf + size, "chipid : 0x%03x%08x\n", id_tbl.chip_id_1, id_tbl.chip_id_2);
	size += sprintf(buf + size, "main revision : %d\n", id_tbl.main_rev);
	size += sprintf(buf + size, "sub revision : %d\n", id_tbl.sub_rev);
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  asv_table_version : %d\n", asv_tbl.asv_table_version);
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  little cpu grp : %d\n", asv_tbl.littlecpu_asv_group);
	size += sprintf(buf + size, "  midl cpu grp : %d\n", asv_tbl.midlcpu_asv_group);
	size += sprintf(buf + size, "  midh cpu grp : %d\n", asv_tbl.midhcpu_asv_group);
	size += sprintf(buf + size, "  big cpu grp : %d\n", asv_tbl.bigcpu_asv_group);
	size += sprintf(buf + size, "  dsu grp : %d\n", asv_tbl.dsu_asv_group);
	size += sprintf(buf + size, "  mif grp : %d\n", asv_tbl.mif_asv_group);
	size += sprintf(buf + size, "  sci grp : %d\n", asv_tbl.intsci_asv_group);
	size += sprintf(buf + size, "  g3d grp : %d\n", asv_tbl.g3d_asv_group);
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  int grp : %d\n", asv_tbl.int_asv_group);
	size += sprintf(buf + size, "  cam grp : %d\n", asv_tbl.cam_asv_group);
	size += sprintf(buf + size, "  npu0 grp : %d\n", asv_tbl.npu0_asv_group);
	size += sprintf(buf + size, "  npu1 grp : %d\n", asv_tbl.npu1_asv_group);
	size += sprintf(buf + size, "  cp grp : %d\n", asv_tbl.cp_asv_group);
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  asb_version : %d\n", asv_tbl.asb_version);
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  power_model_0 : %d\n", power_model.power_model_0 + power_model.power_model_modify_0);
	size += sprintf(buf + size, "  power_model_1 : %d\n", power_model.power_model_1 + power_model.power_model_modify_1);
	size += sprintf(buf + size, "  power_model_2 : %d\n", power_model.power_model_2 + power_model.power_model_modify_2);
	size += sprintf(buf + size, "  power_model_3 : %d\n", power_model.power_model_3 + power_model.power_model_modify_3);
	size += sprintf(buf + size, "  power_model_4 : %d\n", power_model.power_model_4 + power_model.power_model_modify_4);
	size += sprintf(buf + size, "  power_model_5 : %d\n", power_model.power_model_5 + power_model.power_model_modify_5);
	size += sprintf(buf + size, "  power_model_6 : %d\n", power_model.power_model_6 + power_model.power_model_modify_6);
	size += sprintf(buf + size, "  power_model_7 : %d\n", power_model.power_model_7 + power_model.power_model_modify_7);
	size += sprintf(buf + size, "  mcd ver : %d\n", power_model.power_model_ver);
	size += sprintf(buf + size, "\n");

	return size;
}

static ssize_t
asv_info_read(struct file *filp, char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char *buf;
	int r;

	buf = kzalloc(sizeof(char) * 2048, GFP_KERNEL);
	r = print_asv_table(buf);

	r = simple_read_from_buffer(ubuf, cnt, ppos, buf, r);

	kfree(buf);

	return r;
}

static const struct file_operations asv_info_fops = {
	.open		= simple_open,
	.read		= asv_info_read,
	.llseek		= seq_lseek,
};

static ssize_t show_asv_info(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return print_asv_table(buf);
}
static struct kobj_attribute asv_info =
__ATTR(asv_info, 0400, show_asv_info, NULL);

static ssize_t show_power_model_0(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_0 + power_model.power_model_modify_0);
}
static struct kobj_attribute power_model_0 =
__ATTR(power_model_0, 0400, show_power_model_0, NULL);

static ssize_t show_power_model_1(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_1 + power_model.power_model_modify_1);
}
static struct kobj_attribute power_model_1 =
__ATTR(power_model_1, 0400, show_power_model_1, NULL);

static ssize_t show_power_model_2(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_2 + power_model.power_model_modify_2);
}
static struct kobj_attribute power_model_2 =
__ATTR(power_model_2, 0400, show_power_model_2, NULL);

static ssize_t show_power_model_3(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_3 + power_model.power_model_modify_3);
}
static struct kobj_attribute power_model_3 =
__ATTR(power_model_3, 0400, show_power_model_3, NULL);

static ssize_t show_power_model_4(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_4 + power_model.power_model_modify_4);
}
static struct kobj_attribute power_model_4 =
__ATTR(power_model_4, 0400, show_power_model_4, NULL);

static ssize_t show_power_model_5(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_5 + power_model.power_model_modify_5);
}
static struct kobj_attribute power_model_5 =
__ATTR(power_model_5, 0400, show_power_model_5, NULL);

static ssize_t show_power_model_6(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_6 + power_model.power_model_modify_6);
}
static struct kobj_attribute power_model_6 =
__ATTR(power_model_6, 0400, show_power_model_6, NULL);

static ssize_t show_power_model_7(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_7 + power_model.power_model_modify_7);
}
static struct kobj_attribute power_model_7 =
__ATTR(power_model_7, 0400, show_power_model_7, NULL);

static ssize_t show_power_model_ver(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_ver);
}
static struct kobj_attribute power_model_ver =
__ATTR(power_model_ver, 0400, show_power_model_ver, NULL);

static struct attribute *asv_info_attrs[] = {
	&asv_info.attr,
	&power_model_0.attr,
	&power_model_1.attr,
	&power_model_2.attr,
	&power_model_3.attr,
	&power_model_4.attr,
	&power_model_5.attr,
	&power_model_6.attr,
	&power_model_7.attr,
	&power_model_ver.attr,
	NULL,
};

static const struct attribute_group asv_info_grp = {
	.attrs = asv_info_attrs,
};

int asv_debug_init(void)
{
	struct dentry *d;
	struct kobject *kobj;

	rootdir = debugfs_create_dir("asv", NULL);
	if (!rootdir)
		return -ENOMEM;

	d = debugfs_create_file("asv_info", 0600, rootdir, NULL, &asv_info_fops);
	if (!d)
		return -ENOMEM;

	kobj = kobject_create_and_add("asv", kernel_kobj);
	if (!kobj)
		pr_err("Fail to create asv kboject\n");

	if (sysfs_create_group(kobj, &asv_info_grp))
		pr_err("Fail to create asv_info group\n");

	return 0;
}

int asv_table_init(void)
{
#if IS_ENABLED(CONFIG_ACPM_DVFS)
	int i;
	unsigned int *p_table;
	void __iomem *regs;
	unsigned long tmp;

	p_table = (unsigned int *)&asv_tbl;

	for (i = 0; i < ASV_INFO_ADDR_CNT; i++) {
		exynos_smc_readsfr((unsigned long)(ASV_TABLE_BASE + 0x4 * i), &tmp);
		*(p_table + i) = (unsigned int)tmp;
	}

	p_table = (unsigned int *)&id_tbl;
	regs = ioremap(ID_TABLE_BASE, ID_INFO_ADDR_CNT * sizeof(int));
	for (i = 0; i < ID_INFO_ADDR_CNT; i++)
		*(p_table + i) = __raw_readl(regs + 0x4 * i);

	p_table = (unsigned int *)&power_model;

	regs = ioremap(ID_TABLE_BASE + 0x18, sizeof(struct asv_power_model));
	for (i = 0; i < sizeof(struct asv_power_model) / 4; i++)
		*(p_table + i) = __raw_readl(regs + 0x4 * i);

	asv_debug_init();

	return asv_tbl.asv_table_version;
#endif
	return 0;
}
#endif
