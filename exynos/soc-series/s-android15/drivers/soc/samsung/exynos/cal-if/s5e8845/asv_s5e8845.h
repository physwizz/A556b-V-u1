#ifndef __ASV_EXYNOS8845_H__
#define __ASV_EXYNOS8845_H__

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/kobject.h>

#define PR_INFO(fmt, ...) \
       printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)

#define ASV_TABLE_BASE	(0x10009000)
#define ID_TABLE_BASE	(0x10000000)

#if IS_ENABLED(CONFIG_SHOW_ASV)
static struct asv_power_model {
	unsigned power_model_0:8;
	unsigned power_model_1:8;
	unsigned power_model_2:8;
	unsigned mcd_ver:8;
	unsigned power_model_3:8;
	unsigned reserved_0:8;
	unsigned reserved_1:8;
	unsigned mht_ver:8;
} power_model;
#endif

static struct dentry *rootdir;

struct asv_tbl_info {
	unsigned bigcpu_asv_group:4;
	unsigned reserved_0:4;
	unsigned littlecpu_asv_group:4;
	unsigned g3d_asv_group:4;
	unsigned mif_asv_group:4;
	unsigned int_asv_group:4;
	unsigned cam_asv_group:4;
	unsigned npu_asv_group:4;

	unsigned cp_cpu_asv_group:4;
	unsigned cp_asv_group:4;
	unsigned dsu_asv_group:4;
	unsigned sci_asv_group:4;
	unsigned sci_modify_group:4;
	unsigned dsu_modify_group:4;
	unsigned cp_modify_group:4;
	unsigned cp_cpu_modify_group:4;

	unsigned bigcpu_modify_group:4;
	unsigned reserved_2:4;
	unsigned littlecpu_modify_group:4;
	unsigned g3d_modify_group:4;
	unsigned mif_modify_group:4;
	unsigned int_modify_group:4;
	unsigned cam_modify_group:4;
	unsigned npu_modify_group:4;

	unsigned asv_table_version:7;
	unsigned asv_method:1;
	unsigned asb_version:7;
	unsigned vtyp_modify_enable:1;
	unsigned asvg_version:4;
	unsigned product:2;
	unsigned asb_pgm_reserved:10;
};

struct id_tbl_info {
	unsigned padding;
	unsigned int chip_id_2;

	unsigned chip_id_1:13;
	unsigned reserved_0:3;
	unsigned reserved_1:8;
	unsigned reserved_2:8;

	unsigned reserved_3:8;
	unsigned asb_test_version:8;
	unsigned char reserved_4:8;
	unsigned reserved_5:8;

	unsigned char reserved_6:8;
	unsigned char reserved_7:8;
	unsigned short sub_rev:4;
	unsigned short main_rev:4;
	unsigned reserved_8:8;
	unsigned reserved_9:8;
	unsigned reserved_10:8;
	unsigned reserved_11:8;
	unsigned reserved_12:8;
};


#define ASV_INFO_ADDR_CNT	((int)sizeof(struct asv_tbl_info) / 4)
#define ID_INFO_ADDR_CNT	((int)sizeof(struct id_tbl_info) / 4)

static struct asv_tbl_info asv_tbl;
static struct id_tbl_info id_tbl;

int asv_get_grp(unsigned int id)
{
	int grp = -1;

	switch (id) {
	case CPUCL1:
		grp = asv_tbl.bigcpu_asv_group + asv_tbl.bigcpu_modify_group;
		break;
	case CPUCL0:
		grp = asv_tbl.littlecpu_asv_group + asv_tbl.littlecpu_modify_group;
		break;
	case G3D:
		grp = asv_tbl.g3d_asv_group + asv_tbl.g3d_modify_group;
		break;
	case MIF:
		grp = asv_tbl.mif_asv_group + asv_tbl.mif_modify_group;
		break;
	case INT:
		grp = asv_tbl.int_asv_group + asv_tbl.int_modify_group;
		break;
	case CAM:
		grp = asv_tbl.cam_asv_group + asv_tbl.cam_modify_group;
		break;
	case NPU:
		grp = asv_tbl.npu_asv_group + asv_tbl.npu_modify_group;
		break;
	case CP_CPU:
		grp = asv_tbl.cp_cpu_asv_group + asv_tbl.cp_cpu_modify_group;
		break;
	case CP:
		grp = asv_tbl.cp_asv_group + asv_tbl.cp_modify_group;
		break;
	case DSU:
		grp = asv_tbl.dsu_asv_group + asv_tbl.dsu_modify_group;
		break;
	case INTSCI:
		grp = asv_tbl.sci_asv_group + asv_tbl.sci_modify_group;
		break;
	default:
		PR_INFO("Un-support asv grp %d\n", id);
	}
	return grp;
}

int asv_get_ids_info(unsigned int id)
{
	int ids = 0;

	switch (id) {
	default:
		PR_INFO("Un-support ids info %d\n", id);
	}

	return ids;
}

int asv_get_table_ver(void)
{
	return asv_tbl.asv_table_version;
}

void id_get_rev(unsigned int *main_rev, unsigned int *sub_rev)
{
#if defined(CONFIG_ACPM_DVFS) || defined(CONFIG_ACPM_DVFS_MODULE)
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
	return id_tbl.asb_test_version;
}

#if IS_ENABLED(CONFIG_SHOW_ASV)
int print_asv_table(char *buf)
{
	ssize_t size = 0;

	size += sprintf(buf + size, "chipid : 0x%04x%08x\n", id_tbl.chip_id_1, id_tbl.chip_id_2);
	size += sprintf(buf + size, "main revision : %d\n", id_tbl.main_rev);
	size += sprintf(buf + size, "sub revision : %d\n", id_tbl.sub_rev);
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  asv_table_version : %d\n", asv_get_table_ver());
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  big cpu grp : %d\n", asv_get_grp(CPUCL1));
	size += sprintf(buf + size, "  little cpu grp : %d", asv_get_grp(CPUCL0));
	size += sprintf(buf + size, "  g3d grp : %d\n", asv_get_grp(G3D));
	size += sprintf(buf + size, "  mif grp : %d\n", asv_get_grp(MIF));
	size += sprintf(buf + size, "  int grp : %d\n", asv_get_grp(INT));
	size += sprintf(buf + size, "  sci grp : %d\n", asv_get_grp(INTSCI));
	size += sprintf(buf + size, "  cam grp : %d\n", asv_get_grp(CAM));
	size += sprintf(buf + size, "  npu grp : %d\n", asv_get_grp(NPU));
	size += sprintf(buf + size, "  cp_cpu grp : %d\n", asv_get_grp(CP_CPU));
	size += sprintf(buf + size, "  cp grp : %d\n", asv_get_grp(CP));
	size += sprintf(buf + size, "  dsu grp : %d\n", asv_get_grp(DSU));
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  asb_version : %d\n", id_get_asb_ver());
	size += sprintf(buf + size, "\n");
	size += sprintf(buf + size, "  power_model_0 : %d\n", power_model.power_model_0);
	size += sprintf(buf + size, "  power_model_1 : %d\n", power_model.power_model_1);
	size += sprintf(buf + size, "  power_model_2 : %d\n", power_model.power_model_2);
	size += sprintf(buf + size, "  power_model_3 : %d\n", power_model.power_model_3);
	size += sprintf(buf + size, "  mcd ver : %d\n", power_model.mcd_ver);
	size += sprintf(buf + size, "  mht power : %d\n", power_model.mht_ver);
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
	return sprintf(buf, "%d\n", power_model.power_model_0);
}
static struct kobj_attribute power_model_0 =
__ATTR(power_model_0, 0400, show_power_model_0, NULL);

static ssize_t show_power_model_1(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_1);
}
static struct kobj_attribute power_model_1 =
__ATTR(power_model_1, 0400, show_power_model_1, NULL);

static ssize_t show_power_model_2(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_2);
}
static struct kobj_attribute power_model_2 =
__ATTR(power_model_2, 0400, show_power_model_2, NULL);

static ssize_t show_power_model_3(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.power_model_3);
}
static struct kobj_attribute power_model_3 =
__ATTR(power_model_3, 0400, show_power_model_3, NULL);

static ssize_t show_mcd_ver(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.mcd_ver);
}
static struct kobj_attribute mcd_ver =
__ATTR(mcd_ver, 0400, show_mcd_ver, NULL);

static ssize_t show_mht_ver(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", power_model.mht_ver);
}
static struct kobj_attribute mht_ver =
__ATTR(mht_ver, 0400, show_mht_ver, NULL);

static ssize_t show_asb_ver(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", id_tbl.asb_test_version);
}
static struct kobj_attribute asb_ver =
__ATTR(asb_ver, 0400, show_asb_ver, NULL);

static struct attribute *asv_info_attrs[] = {
	&power_model_0.attr,
	&power_model_1.attr,
	&power_model_2.attr,
	&power_model_3.attr,
	&mcd_ver.attr,
	&mht_ver.attr,
	&asb_ver.attr,
	NULL,
};

static const struct attribute_group asv_info_grp = {
	.attrs = asv_info_attrs,
};
#endif

#if IS_ENABLED(CONFIG_SHOW_ASV)
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
#endif

int asv_table_init(void)
{
#if defined(CONFIG_ACPM_DVFS) || defined(CONFIG_ACPM_DVFS_MODULE)
	int i;
	unsigned int *p_table;
	unsigned int *regs;
	unsigned long tmp;

	p_table = (unsigned int *)&asv_tbl;

	for (i = 0; i < ASV_INFO_ADDR_CNT; i++) {
		exynos_smc_readsfr((unsigned long)(ASV_TABLE_BASE + 0x4 * i), &tmp);
		*(p_table + i) = (unsigned int)tmp;
	}

	p_table = (unsigned int *)&id_tbl;

	regs = (unsigned int *)ioremap(ID_TABLE_BASE, ID_INFO_ADDR_CNT * sizeof(int));
	for (i = 0; i < ID_INFO_ADDR_CNT; i++)
		*(p_table + i) = (unsigned int)regs[i];

#if IS_ENABLED(CONFIG_SHOW_ASV)
	p_table = (unsigned int *)&power_model;

	regs = (unsigned int *)ioremap(ID_TABLE_BASE + 0xF064, sizeof(struct asv_power_model));
	*p_table = *regs;
	*(p_table + 1) = *(unsigned int *)(regs + 1);

	asv_debug_init();
#endif

	return asv_tbl.asv_table_version;
#endif
	return 0;
}
#endif
