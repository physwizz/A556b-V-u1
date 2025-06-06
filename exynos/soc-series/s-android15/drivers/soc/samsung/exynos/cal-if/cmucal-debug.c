#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/module.h>
#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-pd.h>

#include "cmucal.h"
#include "vclk.h"
#include "ra.h"

/***        debugfs support        ***/
#ifdef CONFIG_DEBUG_FS
#define MAX_NAME_SIZE	50
#define INFO_SIZE (SZ_64K)

enum clk_info_type {
	NONE_INFO,
	CLK_INFO,
	BLK_CMU_INFO,
	TOP_CMU_INFO,
};

static struct dentry *rootdir;
static struct cmucal_clk *clk_info;
static struct vclk *dvfs_domain;
static u32 cmu_id;
static enum clk_info_type cur_info_type = NONE_INFO;

static void *(*cal_pd_lookup_cmu_id)(u32 cmu_id);
static unsigned int debug_freq;

unsigned int mux_dbg_offset = 0x4000;
unsigned int gate_dbg_offset = 0x4000;
unsigned int top_gate_dbg_offset = 0x4000;
extern unsigned int dbg_offset;
static unsigned int cmu_top_base = 0x0;
static unsigned int cmu_aud_base = 0x0;
void __iomem *cmu_aud;
static unsigned int cmu_nocl0_base = 0x0;
void __iomem *cmu_nocl0;
static unsigned int cmu_cpucl0_base = 0x0;
void __iomem *cmu_cpucl0;
static unsigned int cmu_cpucl1_base = 0x0;
void __iomem *cmu_cpucl1;
static unsigned int cmu_cpucl2_base = 0x0;
void __iomem *cmu_cpucl2;
static unsigned int cmu_dsu_base = 0x0;
void __iomem *cmu_dsu;
static unsigned int cmu_peris_base = 0x0;
void __iomem *cmu_peris;

extern bool is_ignore_cmu_dbg(u32 addr);

static int clk_store(char *buf, int r, struct cmucal_clk *clk, u32 val,
		     bool state)
{
	struct cmucal_clk *p_clk = ra_get_parent(clk->id);

	r += scnprintf(buf + r, INFO_SIZE - r,
		       "%-64s : [0x%02x] %6s, %12u Hz, <- %s\n", clk->name,
		       val, state ? "active" : "idle",
		       vclk_debug_clk_get_rate(clk->id),
		       p_clk ? p_clk->name : "none");

	return r;
}

static bool cal_cmublk_acquire(unsigned int cmu_id,
			       struct exynos_pm_domain **p_pd)
{
	struct exynos_pm_domain *pd = NULL;

	if (!cal_pd_lookup_cmu_id)
		return true;

	pd = (struct exynos_pm_domain *)cal_pd_lookup_cmu_id(cmu_id & 0xFFFF0000);
	if (pd) {
		*p_pd = pd;
		mutex_lock(&pd->access_lock);
		if (!cal_pd_status(pd->cal_pdid))
			return false;
	}

	return true;
}

static void cal_cmublk_release(struct exynos_pm_domain *pd)
{
	if (pd)
		mutex_unlock(&pd->access_lock);
}

/*
 * blk_cmu_info : It will print all the gate clocks of the specified block.
*/
static int blk_cmu_info(char *buf)
{
	struct cmucal_clk *clk;
	struct exynos_pm_domain *pd = NULL;
	int size, reg;
	int r = 0;
	int i;

	if (cal_cmublk_acquire(cmu_id, &pd) == false) {
		if (pd) {
			r = scnprintf(buf, INFO_SIZE, " - %s[%x] is off.\n",
				      pd->name, cmu_id);
		}
		goto blk_unlock;
	}

	r = scnprintf(buf, INFO_SIZE, " - [%x] BLK info\n", cmu_id);
	size = cmucal_get_list_size(GATE_TYPE);
	for (i = 0; i < size ; i++) {
		clk = cmucal_get_node(i | GATE_TYPE);
		if (clk && ((clk->paddr & 0xFFFF0000) == cmu_id)) {
			reg = readl(clk->offset + gate_dbg_offset);
			r = clk_store(buf, r, clk, reg, (reg & 0x70) != 0x30);
		}
	}

blk_unlock:
	cal_cmublk_release(pd);
	return r;
}

/*
 * top_cmu_info : It will print all the pll/mux/divider/gate clocks of the top
*/
static int top_cmu_info(char *buf)
{
	struct cmucal_clk *clk;
	int size, reg;
	int i;
	int r = 0;

	if (cmu_top_base == 0x0)
		return scnprintf(buf, INFO_SIZE, "cmu_top_base is NULL\n");

	r = scnprintf(buf, INFO_SIZE, " - CMU_TOP PLL info!\n");
	size = cmucal_get_list_size(PLL_TYPE);
	for (i = 0; i < size ; i++) {
		clk = cmucal_get_node(i | PLL_TYPE);
		if (!clk || (clk->paddr & 0xFFFF0000) != cmu_top_base)
			continue;
		if (is_ignore_cmu_dbg(clk->paddr))
			continue;
		if (is_ignore_cmu_dbg_type(PLL_TYPE))
			continue;
		reg = readl(clk->pll_con0);
		r = clk_store(buf, r, clk, reg, (reg >> 29) & 0x1);
	}

	r += scnprintf(buf + r, INFO_SIZE - r, "\n - CMU_TOP MUX info\n");
	size = cmucal_get_list_size(MUX_TYPE);
	for (i = 0; i < size ; i++) {
		clk = cmucal_get_node(i | MUX_TYPE);
		if (!clk || (clk->paddr & 0xFFFF0000) != cmu_top_base)
			continue;
		if (is_ignore_cmu_dbg(clk->paddr))
			continue;
		if (is_ignore_cmu_dbg_type(MUX_TYPE))
			continue;
		reg = readl(clk->offset + mux_dbg_offset);
		r = clk_store(buf, r, clk, reg, (reg & 0x70) != 0x30);
	}

	r += scnprintf(buf + r, INFO_SIZE - r, "\n - CMU_TOP GATE info\n");
	size = cmucal_get_list_size(GATE_TYPE);
	for (i = 0; i < size ; i++) {
		clk = cmucal_get_node(i | GATE_TYPE);
		if (!clk || (clk->paddr & 0xFFFF0000) != cmu_top_base)
			continue;
		if (is_ignore_cmu_dbg(clk->paddr))
			continue;
		if (is_ignore_cmu_dbg_type(GATE_TYPE))
			continue;
		reg = readl(clk->offset + top_gate_dbg_offset);
		r = clk_store(buf, r, clk, reg, (reg & 0x70) != 0x30);
	}

	r += scnprintf(buf + r, INFO_SIZE - r, "\n - CMU_TOP DIV info\n");
	size = cmucal_get_list_size(DIV_TYPE);
	for (i = 0; i < size ; i++) {
		clk = cmucal_get_node(i | DIV_TYPE);
		if (!clk || (clk->paddr & 0xFFFF0000) != cmu_top_base)
			continue;
		if (is_ignore_cmu_dbg(clk->paddr))
			continue;
		if (is_ignore_cmu_dbg_type(DIV_TYPE))
			continue;
		reg = readl(clk->offset + dbg_offset);
		r = clk_store(buf, r, clk, reg, (reg & 0x70) != 0x30);
	}

	return r;
}

/*
 * clk_node_info : It will print all information of the clk node
*/
static int clk_node_info(struct cmucal_clk *clk, char *buf)
{
	struct cmucal_clk *parent;
	struct exynos_pm_domain *pd = NULL;
	int r = 0;

	if (clk == NULL) {
		r = scnprintf(buf, INFO_SIZE,
			      "#echo \"clk_name\" > clk_info\n");
		return r;
	}

	if (cal_cmublk_acquire(clk->paddr, &pd) == false) {
		if (pd)
			r = scnprintf(buf, INFO_SIZE, "%s is off.\n", pd->name);
		goto blk_unlock;
	}

	r = scnprintf(buf, INFO_SIZE,
		      "clk name : %s\n"
		      " id      : 0x%x\n"
		      " rate    : %u\n"
		      " value   : %lu\n"
		      " path    :\n",
		      clk->name, clk->id, vclk_debug_clk_get_rate(clk->id),
		      ra_get_value(clk->id));

	parent = ra_get_parent(clk->id);
	while (parent != NULL) {
		r += scnprintf(buf + r, INFO_SIZE - r, "<- %s ", parent->name);
		parent = ra_get_parent(parent->id);
	}
	r += scnprintf(buf + r, INFO_SIZE - r, "\n");
blk_unlock:
	cal_cmublk_release(pd);

	return r;
}

static int vclk_table_dump(struct seq_file *s, void *p)
{
	struct vclk *vclk = s->private;
	struct cmucal_clk *clk;
	struct exynos_pm_domain *pd = NULL;
	int i, j;

	seq_puts(s, "-----------------------------------------------------\n");
	seq_printf(s, "%s <%x>\n", vclk->name, vclk->id);

	for (i = 0; i < vclk->num_list; i++) {
		clk = cmucal_get_node(vclk->list[i]);
		if (!clk)
			continue;

		pd = NULL;
		if (cal_cmublk_acquire(clk->paddr, &pd) == false) {
			if (pd) {
				seq_printf(s, "[%s] %s is off.\n",
					clk->name, pd->name);
			}
		} else {
			seq_printf(s, " [%s] value : %lu rate : %u\n",
				   clk->name,
				   ra_get_value(clk->id),
				   vclk_debug_clk_get_rate(clk->id));
		}
		cal_cmublk_release(pd);
	}

	if (!vclk->lut)
		return 0;

	for (i = 0; i < vclk->num_rates; i++) {
		seq_printf(s, "[%2d]%7d :", i + 1, vclk->lut[i].rate);
		for (j = 0; j < vclk->num_list; j++)
			seq_printf(s, "%7d ", vclk->lut[i].params[j]);
		seq_puts(s, "\n");
	}

	return 0;
}

static int vclk_table_open(struct inode *inode, struct file *file)
{
	return single_open(file, vclk_table_dump, inode->i_private);
}

static const struct file_operations vclk_table_fops = {
	.open		= vclk_table_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int vclk_clk_info(struct seq_file *s, void *p)
{
	return 0;
}

static int vclk_clk_info_open(struct inode *inode, struct file *file)
{
	return single_open_size(file, vclk_clk_info, inode->i_private,
				INFO_SIZE);
}

static ssize_t
vclk_read_clk_info(struct file *filp, char __user *ubuf,
		       size_t cnt, loff_t *ppos)
{
	char *buf;
	int r = 0;

	buf = kmalloc(INFO_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	switch (cur_info_type) {
	case NONE_INFO:
		r = scnprintf(
			buf, INFO_SIZE,
			"CLK_NODE_INFO: echo \"clk_name\" > clk_info\n"
			"BLK_CMU_INFO: echo blk_hwacg #cmu_blk_id > clk_info\n"
			"TOP_CMU_INFO: echo hwacg > clk_info\n");
		break;
	case CLK_INFO:
		r = clk_node_info(clk_info, buf);
		break;
	case BLK_CMU_INFO:
		r = blk_cmu_info(buf);
		break;
	case TOP_CMU_INFO:
		r = top_cmu_info(buf);
		break;
	}

	r = simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
	kfree(buf);
	return r;
}

static ssize_t
vclk_write_clk_info(struct file *filp, const char __user *ubuf,
		   size_t cnt, loff_t *ppos)
{
	char buf[MAX_NAME_SIZE + 1];
	char *c_buf, *tmp;
	unsigned int id;
	unsigned long c_addr;
	size_t ret;

	c_buf = buf;
	ret = cnt;

	if (cnt == 0)
		return cnt;

	if (cnt > MAX_NAME_SIZE)
		cnt = MAX_NAME_SIZE;

	if (copy_from_user(buf, ubuf, cnt))
		return -EVCLKFAULT;

	if (buf[cnt-1] == '\n')
		buf[cnt-1] = 0;
	else
		buf[cnt] = 0;

	if (!strcmp(buf, "hwacg")) {
		cur_info_type = TOP_CMU_INFO;
	} else if (!strcmp(strsep(&c_buf," "), "blk_hwacg")) {
		cur_info_type = NONE_INFO;
		tmp = strsep(&c_buf, " ");
		if (tmp && kstrtol(tmp, 16, &c_addr) == 0) {
			cmu_id = c_addr & 0xFFFF0000;
			cur_info_type = BLK_CMU_INFO;
		}
		pr_info("%x, id = %x\n", cur_info_type, cmu_id);
	} else {
		id = cmucal_get_id(buf);
		clk_info = cmucal_get_node(id);
		if (clk_info)
			cur_info_type = CLK_INFO;
		else
			cur_info_type = NONE_INFO;
	}

	*ppos += ret;
	return cnt;
}

static ssize_t
vclk_read_dvfs_domain(struct file *filp, char __user *ubuf,
		       size_t cnt, loff_t *ppos)
{
	struct vclk *vclk;
	char buf[1024];
	int i, size;
	int r;

	vclk = dvfs_domain;
	if (vclk == NULL)
		r = sprintf(buf, "echo id > dvfs_domain\n");
	else
		r = sprintf(buf, "%s : 0x%x\n", dvfs_domain->name, dvfs_domain->id);

	r += sprintf(buf + r, "- dvfs list\n");

	size = cmucal_get_list_size(ACPM_VCLK_TYPE);
	for (i = 0; i < size ; i++) {
		vclk = cmucal_get_node(ACPM_VCLK_TYPE | i);
		if (vclk == NULL)
			continue;
		r += sprintf(buf + r, "  %s : 0x%x\n", vclk->name, vclk->id);
	}

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static ssize_t
vclk_write_dvfs_domain(struct file *filp, const char __user *ubuf,
		   size_t cnt, loff_t *ppos)
{
	char buf[16];
	ssize_t len;
	u32 id;

	len = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, ubuf, cnt);
	if (len < 0 || len >= 15)
		return len;

	buf[len] = '\0';

	if (kstrtouint(buf, 0, &id) == 0) {
		dvfs_domain = cmucal_get_node(id);
		if (!dvfs_domain || !IS_ACPM_VCLK(dvfs_domain->id))
			dvfs_domain = NULL;
	}

	return len;
}

static ssize_t
vclk_read_set_freq(struct file *filp, char __user *ubuf,
		       size_t cnt, loff_t *ppos)
{
	char buf[512];
	int r;

	r = sprintf(buf, "freq : %u\n", debug_freq);

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static ssize_t
vclk_write_set_freq(struct file *filp, const char __user *ubuf,
		   size_t cnt, loff_t *ppos)
{
	char buf[16];
	ssize_t len;
	u32 freq;

	len = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, ubuf, cnt);
	if (len < 0 || len >= 15)
		return len;

	buf[len] = '\0';
	if (dvfs_domain && (kstrtoint(buf, 0, &freq) == 0)) {
		debug_freq = freq;
		cal_dfs_set_rate(dvfs_domain->id, freq);
	}

	return len;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define BASE_MOVE		(0x100)
#if 0
#define CLKDIVSTEP_STAT		(0x83c)
#define CLK_CON_MUX_MUX_CLK	(0x1000)
#define STRMUX			(0x870)

static ssize_t
cpucl0_stepup_run_read(struct file *filp, char __user *ubuf,
		       size_t cnt, loff_t *ppos)
{
	char buf[512] = {0,};
	int r;

	if (cmu_cpucl0) {
		unsigned int reg = __raw_readl(cmu_cpucl0 - BASE_MOVE + CLKDIVSTEP_STAT);
		r = sprintf(buf, "CLKDIVSTEP_STAT : 0x%x\n", reg);
		reg = __raw_readl(cmu_cpucl0 - BASE_MOVE + CLK_CON_MUX_MUX_CLK);
		r = sprintf(buf, "%sCLK_CON_MUX_MUX_CLK_ : 0x%x\n", buf, reg);
		reg = __raw_readl(cmu_cpucl0 - BASE_MOVE + STRMUX);
		r = sprintf(buf, "%s_STRMUX_CORE : 0x%x\n", buf, reg);
	} else {
		r = sprintf(buf, "empty base\n");
	}

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static ssize_t
cpucl1_stepup_run_read(struct file *filp, char __user *ubuf,
		       size_t cnt, loff_t *ppos)
{
	char buf[512] = {0,};
	int r;

	if (cmu_cpucl1) {
		unsigned int reg = __raw_readl(cmu_cpucl1 - BASE_MOVE + CLKDIVSTEP_STAT);
		r = sprintf(buf, "CLKDIVSTEP_STAT : 0x%x\n", reg);
		reg = __raw_readl(cmu_cpucl1 - BASE_MOVE + CLK_CON_MUX_MUX_CLK);
		r = sprintf(buf, "%sCLK_CON_MUX_MUX_CLK_ : 0x%x\n", buf, reg);
		reg = __raw_readl(cmu_cpucl1 - BASE_MOVE + STRMUX);
		r = sprintf(buf, "%s_STRMUX_CORE : 0x%x\n", buf, reg);
	} else {
		r = sprintf(buf, "empty base\n");
	}

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static ssize_t
cpucl2_stepup_run_read(struct file *filp, char __user *ubuf,
		       size_t cnt, loff_t *ppos)
{
	char buf[512] = {0,};
	int r;

	if (cmu_cpucl2) {
		unsigned int reg = __raw_readl(cmu_cpucl2 - BASE_MOVE + CLKDIVSTEP_STAT);
		r = sprintf(buf, "CLKDIVSTEP_STAT : 0x%x\n", reg);
		reg = __raw_readl(cmu_cpucl2 - BASE_MOVE + CLK_CON_MUX_MUX_CLK);
		r = sprintf(buf, "%sCLK_CON_MUX_MUX_CLK_ : 0x%x\n", buf, reg);
		reg = __raw_readl(cmu_cpucl2 - BASE_MOVE + STRMUX);
		r = sprintf(buf, "%s_STRMUX_CORE : 0x%x\n", buf, reg);
	} else {
		r = sprintf(buf, "empty base\n");
	}

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static ssize_t
dsu_stepup_run_read(struct file *filp, char __user *ubuf,
		       size_t cnt, loff_t *ppos)
{
	char buf[512] = {0,};
	int r;

	if (cmu_dsu) {
		unsigned int reg = __raw_readl(cmu_dsu - BASE_MOVE + CLKDIVSTEP_STAT);
		r = sprintf(buf, "CLKDIVSTEP_STAT : 0x%x\n", reg);
		reg = __raw_readl(cmu_dsu - BASE_MOVE + CLK_CON_MUX_MUX_CLK);
		r = sprintf(buf, "%s CLK_CON_MUX_MUX_CLK_ : 0x%x\n", buf, reg);
		reg = __raw_readl(cmu_dsu - BASE_MOVE + STRMUX);
		r = sprintf(buf, "%s_STRMUX_CORE : 0x%x\n", buf, reg);
	} else {
		r = sprintf(buf, "empty base\n");
	}

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}
#endif

static const struct file_operations clk_info_fops = {
	.open		= vclk_clk_info_open,
	.read		= vclk_read_clk_info,
	.write		= vclk_write_clk_info,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations dvfs_domain_fops = {
	.open		= simple_open,
	.read		= vclk_read_dvfs_domain,
	.write		= vclk_write_dvfs_domain,
	.llseek		= seq_lseek,
};

static const struct file_operations set_freq_fops = {
	.open		= simple_open,
	.read		= vclk_read_set_freq,
	.write		= vclk_write_set_freq,
	.llseek		= seq_lseek,
};

#if 0
static const struct file_operations cpucl0_stepup_run_fops = {
	.open		= simple_open,
	.read		= cpucl0_stepup_run_read,
	.llseek		= seq_lseek,
};

static const struct file_operations cpucl1_stepup_run_fops = {
	.open		= simple_open,
	.read		= cpucl1_stepup_run_read,
	.llseek		= seq_lseek,
};

static const struct file_operations cpucl2_stepup_run_fops = {
	.open		= simple_open,
	.read		= cpucl2_stepup_run_read,
	.llseek		= seq_lseek,
};

static const struct file_operations dsu_stepup_run_fops = {
	.open		= simple_open,
	.read		= dsu_stepup_run_read,
	.llseek		= seq_lseek,
};
#endif

/* caller must hold prepare_lock */
static int vclk_debug_create_one(struct vclk *vclk, struct dentry *pdentry)
{
	struct dentry *d;
	int ret = -ENOMEM;

	if (!vclk || !pdentry) {
		ret = -EINVAL;
		goto out;
	}

	d = debugfs_create_dir(vclk->name, pdentry);
	if (!d)
		goto out;

	vclk->dentry = d;

	debugfs_create_x32("vclk_id", 0400, vclk->dentry, (u32 *)&vclk->id);

	debugfs_create_u32("vclk_rate", 0400, vclk->dentry, (u32 *)&vclk->vrate);

	debugfs_create_u32("vclk_num_rates", 0400, vclk->dentry, (u32 *)&vclk->num_rates);

	debugfs_create_u32("vclk_num_list", 0400, vclk->dentry, (u32 *)&vclk->num_list);

	d = debugfs_create_file("vclk_table", 0400, vclk->dentry, vclk,
				&vclk_table_fops);
	if (!d)
		return -ENOMEM;

	ret = 0;
	goto out;

out:
	return ret;
}

unsigned int vclk_debug_clk_get_rate(unsigned int id)
{
	unsigned long rate;

	rate = ra_recalc_rate(id);

	return rate;
}
EXPORT_SYMBOL_GPL(vclk_debug_clk_get_rate);

#if 0
unsigned long vclk_debug_clk_get_value(unsigned int id)
{
	unsigned long val;

	val = ra_get_value(id);

	return val;
}
EXPORT_SYMBOL_GPL(vclk_debug_clk_get_value);

int vclk_debug_clk_set_value(unsigned int id, unsigned int params)
{
	int ret;

	ret = ra_set_value(id, params);

	return ret;
}
EXPORT_SYMBOL_GPL(vclk_debug_clk_set_value);
#endif

void cmucal_dbg_set_cmu_top_base(u32 base_addr)
{
	cmu_top_base = base_addr;
	pr_info("cmu_top_base : 0x%x\n", base_addr);
}
EXPORT_SYMBOL_GPL(cmucal_dbg_set_cmu_top_base);

void cmucal_dbg_set_cmu_aud_base(u32 base_addr)
{
	cmu_aud_base = base_addr + BASE_MOVE;
	pr_info("cmu_aud_base : 0x%x\n", base_addr);

	cmu_aud = ioremap(cmu_aud_base, SZ_4K);
	if (!cmu_aud)
		pr_err("%s: cmu_aud ioremap failed\n", __func__);
}
EXPORT_SYMBOL_GPL(cmucal_dbg_set_cmu_aud_base);

void cmucal_dbg_set_cmu_nocl0_base(u32 base_addr)
{
	cmu_nocl0_base = base_addr + BASE_MOVE;
	pr_info("cmu_nocl0_base : 0x%x\n", base_addr);

	cmu_nocl0 = ioremap(cmu_nocl0_base, SZ_4K);
	if (!cmu_nocl0)
		pr_err("%s: cmu_nocl0 ioremap failed\n", __func__);
}
EXPORT_SYMBOL_GPL(cmucal_dbg_set_cmu_nocl0_base);

void cmucal_dbg_set_cmu_cpucl0_base(u32 base_addr)
{
	cmu_cpucl0_base = base_addr + BASE_MOVE;
	pr_info("cmu_cpucl0_base : 0x%x\n", base_addr);

	cmu_cpucl0 = ioremap(cmu_cpucl0_base, SZ_4K);
	if (!cmu_cpucl0)
		pr_err("%s: cmu_cpucl0 ioremap failed\n", __func__);
}
EXPORT_SYMBOL_GPL(cmucal_dbg_set_cmu_cpucl0_base);

void cmucal_dbg_set_cmu_cpucl1_base(u32 base_addr)
{
	cmu_cpucl1_base = base_addr + BASE_MOVE;
	pr_info("cmu_cpucl1_base : 0x%x\n", base_addr);

	cmu_cpucl1 = ioremap(cmu_cpucl1_base, SZ_4K);
	if (!cmu_cpucl1)
		pr_err("%s: cmu_cpucl1 ioremap failed\n", __func__);
}
EXPORT_SYMBOL_GPL(cmucal_dbg_set_cmu_cpucl1_base);

void cmucal_dbg_set_cmu_cpucl2_base(u32 base_addr)
{
	cmu_cpucl2_base = base_addr + BASE_MOVE;
	pr_info("cmu_cpucl2_base : 0x%x\n", base_addr);

	cmu_cpucl2 = ioremap(cmu_cpucl2_base, SZ_4K);
	if (!cmu_cpucl2)
		pr_err("%s: cmu_cpucl2 ioremap failed\n", __func__);
}
EXPORT_SYMBOL_GPL(cmucal_dbg_set_cmu_cpucl2_base);

void cmucal_dbg_set_cmu_dsu_base(u32 base_addr)
{
	cmu_dsu_base = base_addr + BASE_MOVE;
	pr_info("cmu_dsu_base : 0x%x\n", base_addr);

	cmu_dsu = ioremap(cmu_dsu_base, SZ_4K);
	if (!cmu_dsu)
		pr_err("%s: cmu_dsu ioremap failed\n", __func__);
}
EXPORT_SYMBOL_GPL(cmucal_dbg_set_cmu_dsu_base);

void cmucal_dbg_set_cmu_peris_base(u32 base_addr)
{
	cmu_peris_base = base_addr + BASE_MOVE;
	pr_info("cmu_peris_base : 0x%x\n", base_addr);

	cmu_peris = ioremap(cmu_peris_base, SZ_4K);
	if (!cmu_peris)
		pr_err("%s: cmu_peris ioremap failed\n", __func__);
}
EXPORT_SYMBOL_GPL(cmucal_dbg_set_cmu_peris_base);

void cal_register_pd_lookup_cmu_id(void *(*func)(u32 cmu_id))
{
	cal_pd_lookup_cmu_id = func;
}
EXPORT_SYMBOL_GPL(cal_register_pd_lookup_cmu_id);
/**
 * vclk_debug_init - lazily create the debugfs clk tree visualization
 */
int vclk_debug_init(void)
{
	struct vclk *vclk;
	struct dentry *d;
	int i;

	rootdir = debugfs_create_dir("vclk", NULL);

	if (!rootdir)
		return -ENOMEM;

	for (i = 0; i < cmucal_get_list_size(VCLK_TYPE); i++) {
		vclk = cmucal_get_node(i | VCLK_TYPE);
		if (!vclk)
			continue;
		vclk_debug_create_one(vclk, rootdir);
	}

	for (i = 0; i < cmucal_get_list_size(ACPM_VCLK_TYPE); i++) {
		vclk = cmucal_get_node(i | ACPM_VCLK_TYPE);
		if (!vclk)
			continue;
		vclk_debug_create_one(vclk, rootdir);
	}

	d = debugfs_create_file("clk_info", 0600, rootdir, NULL,
				&clk_info_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file("dvfs_domain", 0600, rootdir, NULL,
				&dvfs_domain_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file("set_freq", 0600, rootdir, NULL,
				&set_freq_fops);
	if (!d)
		return -ENOMEM;
#if 0
	d = debugfs_create_file("cpucl0_stepup_run", 0400, rootdir, NULL,
					&cpucl0_stepup_run_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file("cpucl1_stepup_run", 0400, rootdir, NULL,
					&cpucl1_stepup_run_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file("cpucl2_stepup_run", 0400, rootdir, NULL,
					&cpucl2_stepup_run_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file("dsu_stepup_run", 0400, rootdir, NULL,
					&dsu_stepup_run_fops);
	if (!d)
		return -ENOMEM;
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(vclk_debug_init);
#endif

MODULE_LICENSE("GPL");
