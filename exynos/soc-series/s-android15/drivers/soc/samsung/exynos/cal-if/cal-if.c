#include <linux/module.h>
#include <soc/samsung/exynos/debug-snapshot.h>
#include <soc/samsung/ect_parser.h>
#include <soc/samsung/cal-if.h>
#if defined(CONFIG_EXYNOS_BTS) || defined(CONFIG_EXYNOS_BTS_MODULE)
#include <soc/samsung/bts.h>
#endif
#if defined(CONFIG_EXYNOS_BCM_DBG) || defined(CONFIG_EXYNOS_BCM_DBG_MODULE)
#include <soc/samsung/exynos-bcm_dbg.h>
#endif
#if defined(CONFIG_CMU_EWF) || defined(CONFIG_CMU_EWF_MODULE)
#include <soc/samsung/cmu_ewf.h>
#endif

#include <soc/samsung/pwrcal-env.h>
#include "pwrcal-rae.h"
#include "cmucal.h"
#include "ra.h"
#include "acpm_dvfs.h"
#include <soc/samsung/fvmap.h>
#include "asv.h"
#include "power-rail-dbg.h"

#include <soc/samsung/pmucal/pmucal_system.h>
#include <soc/samsung/pmucal/pmucal_local.h>
#include <soc/samsung/pmucal/pmucal_cpu.h>
#include <soc/samsung/pmucal/pmucal_dbg.h>
#include <soc/samsung/pmucal/pmucal_cp.h>
#if IS_ENABLED(CONFIG_GNSS_PMUCAL)
#include <soc/samsung/pmucal/pmucal_gnss.h>
#endif
#if IS_ENABLED(CONFIG_CHUB_PMUCAL)
#include <soc/samsung/pmucal/pmucal_chub.h>
#endif
#include <soc/samsung/pmucal/pmucal_rae.h>
#include <soc/samsung/pmucal/pmucal_powermode.h>

#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
#include "../esca/esca_drv.h"
#else
#include "../acpm/acpm.h"
#endif

int (*exynos_cal_pd_sci_sync)(unsigned int id, bool on);
EXPORT_SYMBOL(exynos_cal_pd_sci_sync);

int (*exynos_cal_pd_bcm_sync)(unsigned int id, bool on);
EXPORT_SYMBOL(exynos_cal_pd_bcm_sync);

void (*exynos_cal_pd_bts_sync)(unsigned int id, int on);
EXPORT_SYMBOL(exynos_cal_pd_bts_sync);

static DEFINE_SPINLOCK(pmucal_cpu_lock);

unsigned int cal_clk_is_enabled(unsigned int id)
{
	return 0;
}
EXPORT_SYMBOL_GPL(cal_clk_is_enabled);

unsigned long cal_dfs_get_max_freq(unsigned int id)
{
	return vclk_get_max_freq(id);
}
EXPORT_SYMBOL_GPL(cal_dfs_get_max_freq);

unsigned long cal_dfs_get_min_freq(unsigned int id)
{
	return vclk_get_min_freq(id);
}
EXPORT_SYMBOL_GPL(cal_dfs_get_min_freq);

unsigned int cal_dfs_get_lv_num(unsigned int id)
{
	return vclk_get_lv_num(id);
}
EXPORT_SYMBOL_GPL(cal_dfs_get_lv_num);

int cal_dfs_get_bigturbo_max_freq(unsigned int *table)
{
	return vclk_get_bigturbo_table(table);
}
EXPORT_SYMBOL_GPL(cal_dfs_get_bigturbo_max_freq);

int cal_dfs_update_rate(unsigned int id, unsigned long rate)
{
	struct vclk *vclk;

	if (IS_ACPM_VCLK(id)) {
		vclk = cmucal_get_node(id);
		if (vclk)
			vclk->vrate = (unsigned int)rate;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(cal_dfs_update_rate);

static int __cal_dfs_set_rate(unsigned int id, unsigned long rate, bool fast_switch)
{
	struct vclk *vclk;
	int ret;

	if (IS_ACPM_VCLK(id)) {
		ret = exynos_acpm_set_rate(GET_IDX(id), rate, fast_switch);
		if (!ret) {
			vclk = cmucal_get_node(id);
			if (vclk)
				vclk->vrate = (unsigned int)rate;
		}
	} else {
		ret = vclk_set_rate(id, rate);
	}

	return ret;
}

int cal_dfs_set_rate(unsigned int id, unsigned long rate)
{
	return __cal_dfs_set_rate(id, rate, false);
}
EXPORT_SYMBOL_GPL(cal_dfs_set_rate);

int cal_dfs_set_rate_fast(unsigned int id, unsigned long rate)
{
	return __cal_dfs_set_rate(id, rate, true);
}
EXPORT_SYMBOL_GPL(cal_dfs_set_rate_fast);

int cal_dfs_set_rate_switch(unsigned int id, unsigned long switch_rate)
{
	int ret = 0;

	ret = vclk_set_rate_switch(id, switch_rate);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_dfs_set_rate_switch);

int cal_dfs_set_rate_restore(unsigned int id, unsigned long switch_rate)
{
	int ret = 0;

	ret = vclk_set_rate_restore(id, switch_rate);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_dfs_set_rate_restore);

unsigned long cal_dfs_cached_get_rate(unsigned int id)
{
	unsigned long ret;

	ret = vclk_get_rate(id);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_dfs_cached_get_rate);

unsigned long cal_dfs_get_rate(unsigned int id)
{
	unsigned long ret;

	ret = vclk_recalc_rate(id);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_dfs_get_rate);

int cal_dfs_get_rate_table(unsigned int id, unsigned long *table)
{
	int ret;

	ret = vclk_get_rate_table(id, table);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_dfs_get_rate_table);

int cal_clk_setrate(unsigned int id, unsigned long rate)
{
	int ret = -EINVAL;

	ret = vclk_set_rate(id, rate);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_clk_setrate);

unsigned long cal_clk_getrate(unsigned int id)
{
	unsigned long ret = 0;

	ret = vclk_recalc_rate(id);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_clk_getrate);

int cal_clk_enable(unsigned int id)
{
	int ret = 0;

	ret = vclk_set_enable(id);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_clk_enable);

int cal_clk_disable(unsigned int id)
{
	int ret = 0;

	ret = vclk_set_disable(id);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_clk_disable);

int cal_qch_init(unsigned int id, unsigned int use_qch)
{
	int ret = 0;

	ret = ra_set_qch(id, use_qch, 0, 0);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_qch_init);

unsigned int cal_dfs_get_boot_freq(unsigned int id)
{
	return vclk_get_boot_freq(id);
}
EXPORT_SYMBOL_GPL(cal_dfs_get_boot_freq);

unsigned int cal_dfs_get_resume_freq(unsigned int id)
{
	return vclk_get_resume_freq(id);
}
EXPORT_SYMBOL_GPL(cal_dfs_get_resume_freq);

int cal_pd_control(unsigned int id, int on)
{
	unsigned int index;
	int ret;

	if ((id & 0xFFFF0000) != BLKPWR_MAGIC)
		return -1;

	index = id & 0x0000FFFF;

	if (on) {
		ret = pmucal_local_enable(index);
#if defined(CONFIG_EXYNOS_SCI) || defined(CONFIG_EXYNOS_SCI_MODULE) || \
		defined(CONFIG_EXYNOS_USE_SCI_LITE)
		if (exynos_cal_pd_sci_sync && cal_pd_status(id))
			exynos_cal_pd_sci_sync(id, on);
#endif
#if defined(CONFIG_EXYNOS_BTS) || defined(CONFIG_EXYNOS_BTS_MODULE)
		if (exynos_cal_pd_bts_sync && cal_pd_status(id))
			exynos_cal_pd_bts_sync(id, on);
#endif
#if defined(CONFIG_EXYNOS_BCM_DBG) || defined(CONFIG_EXYNOS_BCM_DBG_MODULE)
		if (exynos_cal_pd_bcm_sync && cal_pd_status(id))
			exynos_cal_pd_bcm_sync(id, true);
#endif
	} else {
#if defined(CONFIG_EXYNOS_BTS) || defined(CONFIG_EXYNOS_BTS_MODULE)
		if (exynos_cal_pd_bts_sync && cal_pd_status(id))
			exynos_cal_pd_bts_sync(id, on);
#endif
#if defined(CONFIG_EXYNOS_BCM_DBG) || defined(CONFIG_EXYNOS_BCM_DBG_MODULE)
		if (exynos_cal_pd_bcm_sync && cal_pd_status(id))
			exynos_cal_pd_bcm_sync(id, false);
#endif
		ret = pmucal_local_disable(index);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(cal_pd_control);

int cal_pd_status(unsigned int id)
{
	unsigned int index;

	if ((id & 0xFFFF0000) != BLKPWR_MAGIC)
		return -1;

	index = id & 0x0000FFFF;

	return pmucal_local_is_enabled(index);
}
EXPORT_SYMBOL_GPL(cal_pd_status);

int cal_pd_set_smc_id(unsigned int id, int need_smc)
{
	unsigned int index;

	if (need_smc && ((id & 0xFFFF0000) != BLKPWR_MAGIC))
		return -1;

	index = id & 0x0000FFFF;

	pmucal_local_set_smc_id(index, need_smc);

	return 0;
}
EXPORT_SYMBOL_GPL(cal_pd_set_smc_id);

int cal_pd_set_first_on(unsigned int id, int initial_state)
{
#ifdef CONFIG_PMUCAL_CMU_INIT
	unsigned int index;

	if ((id & 0xFFFF0000) != BLKPWR_MAGIC)
		return -1;

	index = id & 0x0000FFFF;

	pmucal_local_set_first_on(index, initial_state);

	return 0;
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(cal_pd_set_first_on);

int cal_pm_enter(int mode)
{
	return pmucal_system_enter(mode);
}
EXPORT_SYMBOL_GPL(cal_pm_enter);

int cal_pm_exit(int mode)
{
	return pmucal_system_exit(mode);
}
EXPORT_SYMBOL_GPL(cal_pm_exit);

int cal_pm_earlywakeup(int mode)
{
	return pmucal_system_earlywakeup(mode);
}
EXPORT_SYMBOL_GPL(cal_pm_earlywakeup);

int cal_cpu_enable(unsigned int cpu)
{
	int ret;

	spin_lock(&pmucal_cpu_lock);
	ret = pmucal_cpu_enable(cpu);
	spin_unlock(&pmucal_cpu_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_cpu_enable);

int cal_cpu_disable(unsigned int cpu)
{
	int ret;

	spin_lock(&pmucal_cpu_lock);
	ret = pmucal_cpu_disable(cpu);
	spin_unlock(&pmucal_cpu_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_cpu_disable);


int cal_cpu_status(unsigned int cpu)
{
	int ret;

	spin_lock(&pmucal_cpu_lock);
	ret = pmucal_cpu_is_enabled(cpu);
	spin_unlock(&pmucal_cpu_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_cpu_status);

int cal_cluster_enable(unsigned int cluster)
{
	int ret;

	spin_lock(&pmucal_cpu_lock);
	ret = pmucal_cpu_cluster_enable(cluster);
	spin_unlock(&pmucal_cpu_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_cluster_enable);

int cal_cluster_disable(unsigned int cluster)
{
	int ret;

	spin_lock(&pmucal_cpu_lock);
	ret = pmucal_cpu_cluster_disable(cluster);
	spin_unlock(&pmucal_cpu_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_cluster_disable);

int cal_cluster_status(unsigned int cluster)
{
	int ret;

	spin_lock(&pmucal_cpu_lock);
	ret = pmucal_cpu_cluster_is_enabled(cluster);
	spin_unlock(&pmucal_cpu_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_cluster_status);

int cal_cluster_req_emulation(unsigned int cluster, bool en)
{
	int ret;

	spin_lock(&pmucal_cpu_lock);
	ret = pmucal_cpu_cluster_req_emulation(cluster, en);
	spin_unlock(&pmucal_cpu_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(cal_cluster_req_emulation);

extern int cal_is_lastcore_detecting(unsigned int cpu)
{
	return pmucal_is_lastcore_detecting(cpu);
}
EXPORT_SYMBOL_GPL(cal_is_lastcore_detecting);

int cal_dfs_get_asv_table(unsigned int id, unsigned int *table)
{
	return fvmap_get_voltage_table(id, table);
}
EXPORT_SYMBOL_GPL(cal_dfs_get_asv_table);

int cal_dfs_get_freq_volt_table(unsigned int id, void *table, int size)
{
	return fvmap_get_freq_volt_table(id, table, size);
}
EXPORT_SYMBOL_GPL(cal_dfs_get_freq_volt_table);

int cal_dfs_set_volt_margin(unsigned int id, int lower, int upper, int volt)
{
	return exynos_acpm_set_volt_margin(id, lower, upper, volt);
}
EXPORT_SYMBOL_GPL(cal_dfs_set_volt_margin);

int cal_dfs_get_rate_asv_table(unsigned int id,
					struct dvfs_rate_volt *table)
{
	unsigned long rate[48];
	unsigned int volt[48];
	int num_of_entry;
	int idx;

	num_of_entry = cal_dfs_get_rate_table(id, rate);
	if (num_of_entry == 0)
		return 0;

	if (num_of_entry != cal_dfs_get_asv_table(id, volt))
		return 0;

	for (idx = 0; idx < num_of_entry; idx++) {
		table[idx].rate = rate[idx];
		table[idx].volt = volt[idx];
	}

	return num_of_entry;
}
EXPORT_SYMBOL_GPL(cal_dfs_get_rate_asv_table);

int cal_asv_get_ids_info(unsigned int id)
{
	return asv_get_ids_info(id);
}
EXPORT_SYMBOL_GPL(cal_asv_get_ids_info);

int cal_asv_get_grp(unsigned int id)
{
	return asv_get_grp(id);
}
EXPORT_SYMBOL_GPL(cal_asv_get_grp);

int cal_asv_get_tablever(void)
{
	return asv_get_table_ver();
}
EXPORT_SYMBOL_GPL(cal_asv_get_tablever);

#if IS_ENABLED(CONFIG_CP_PMUCAL)
int cal_cp_init(void)
{
	return pmucal_cp_init();
}
EXPORT_SYMBOL_GPL(cal_cp_init);

int cal_cp_status(unsigned int pmu_cp_status)
{
	return pmucal_cp_status(pmu_cp_status);
}
EXPORT_SYMBOL_GPL(cal_cp_status);

int cal_cp_reset_assert(void)
{
	return pmucal_cp_reset_assert();
}
EXPORT_SYMBOL_GPL(cal_cp_reset_assert);

int cal_cp_reset_release(void)
{
	return pmucal_cp_reset_release();
}
EXPORT_SYMBOL_GPL(cal_cp_reset_release);

void cal_cp_active_clear(void)
{
	pmucal_cp_active_clear();
}
EXPORT_SYMBOL_GPL(cal_cp_active_clear);

void cal_cp_reset_req_clear(void)
{
	pmucal_cp_reset_req_clear();
}
EXPORT_SYMBOL_GPL(cal_cp_reset_req_clear);

void cal_cp_enable_dump_pc_no_pg(void)
{
	pmucal_cp_enable_dump_pc_no_pg();
}
EXPORT_SYMBOL_GPL(cal_cp_enable_dump_pc_no_pg);

void cal_cp_disable_dump_pc_no_pg(void)
{
	pmucal_cp_disable_dump_pc_no_pg();
}
EXPORT_SYMBOL_GPL(cal_cp_disable_dump_pc_no_pg);
#endif

#if IS_ENABLED(CONFIG_GNSS_PMUCAL)
void cal_gnss_init(void)
{
	pmucal_gnss_init();
}
EXPORT_SYMBOL_GPL(cal_gnss_init);

int cal_gnss_status(void)
{
	return pmucal_gnss_status();
}
EXPORT_SYMBOL_GPL(cal_gnss_status);

void cal_gnss_reset_assert(void)
{
	pmucal_gnss_reset_assert();
}
EXPORT_SYMBOL_GPL(cal_gnss_reset_assert);

void cal_gnss_reset_release(void)
{
	pmucal_gnss_reset_release();
}
EXPORT_SYMBOL_GPL(cal_gnss_reset_release);

void cal_gnss_reset_req_clear(void)
{
	pmucal_gnss_reset_req_clear();
}
EXPORT_SYMBOL_GPL(cal_gnss_reset_req_clear);

void cal_gnss_active_clear(void)
{
	pmucal_gnss_active_clear();
}
EXPORT_SYMBOL_GPL(cal_gnss_active_clear);
#endif

#if IS_ENABLED(CONFIG_CHUB_PMUCAL)
int cal_chub_on(void)
{
	return pmucal_chub_on();
}
EXPORT_SYMBOL(cal_chub_on);

int cal_chub_reset_assert(void)
{
	return pmucal_chub_reset_assert();
}
EXPORT_SYMBOL(cal_chub_reset_assert);

int cal_chub_reset_release_config(void)
{
	return pmucal_chub_reset_release_config();
}
EXPORT_SYMBOL(cal_chub_reset_release_config);

int cal_chub_reset_release(void)
{
	return pmucal_chub_reset_release();
}
EXPORT_SYMBOL(cal_chub_reset_release);
#endif

extern struct cmucal_pll cmucal_pll_list[];

int cal_if_init(void *dev)
{
	static int cal_initialized;
	struct resource res;

#if IS_ENABLED(CONFIG_PMUCAL)
	int ret;
#endif
	if (cal_initialized == 1)
		return 0;

	ect_parse_binary_header();

	vclk_initialize();

	if (cal_data_init) {
		int high_freq_pll_list[10] = { 0, }, i;
		struct cmucal_pll *pll;

		cal_data_init();

		get_high_freq_pll_idx(high_freq_pll_list);

		for (i = 0; i < ARRAY_SIZE(high_freq_pll_list); i++) {
			if (!high_freq_pll_list[i])
				continue;
			pll = get_clk_node(cmucal_pll_list, GET_IDX(high_freq_pll_list[i]));
			pll->freq_type = HIGH_FREQ_PLL;
		}
	}

#if IS_ENABLED(CONFIG_PMUCAL)
	ret = pmucal_rae_init();
	if (ret < 0)
		return ret;

	ret = pmucal_system_init();
	if (ret < 0)
		return ret;

	ret = pmucal_local_init();
	if (ret < 0)
		return ret;

	ret = pmucal_cpu_init();
	if (ret < 0)
		return ret;

	ret = pmucal_cpuinform_init();
	if (ret < 0)
		return ret;
#endif
#if IS_ENABLED(CONFIG_PMUCAL_DBG)
	pmucal_dbg_init();
#endif

#if IS_ENABLED(CONFIG_CP_PMUCAL)
	ret = pmucal_cp_initialize();
	if (ret < 0)
		return ret;
#endif

#if IS_ENABLED(CONFIG_GNSS_PMUCAL)
	ret = pmucal_gnss_initialize();
	if (ret < 0)
		return ret;
#endif

#if IS_ENABLED(CONFIG_CHUB_PMUCAL)
	ret = pmucal_chub_initialize();
	if(ret < 0) {
		return ret;
	}
#endif
	exynos_acpm_set_device(dev);

	if (of_address_to_resource(dev, 0, &res) == 0)
		cmucal_dbg_set_cmu_top_base(res.start);

	if (of_address_to_resource(dev, 1, &res) == 0)
		cmucal_dbg_set_cmu_aud_base(res.start);

	if (of_address_to_resource(dev, 2, &res) == 0)
		cmucal_dbg_set_cmu_nocl0_base(res.start);

	if (of_address_to_resource(dev, 3, &res) == 0)
		cmucal_dbg_set_cmu_cpucl0_base(res.start);

	if (of_address_to_resource(dev, 4, &res) == 0)
		cmucal_dbg_set_cmu_cpucl1_base(res.start);

	if (of_address_to_resource(dev, 5, &res) == 0)
		cmucal_dbg_set_cmu_cpucl2_base(res.start);

	if (of_address_to_resource(dev, 6, &res) == 0)
		cmucal_dbg_set_cmu_dsu_base(res.start);

	if (of_address_to_resource(dev, 7, &res) == 0)
		cmucal_dbg_set_cmu_peris_base(res.start);

	cal_initialized = 1;

	vclk_debug_init();

	pmucal_dbg_debugfs_init();

	return 0;
}

static int cal_if_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;
	ret = cal_if_init(np);
	if (ret)
		goto out;

#if IS_ENABLED(CONFIG_CP_PMUCAL)
	cp_set_device(&pdev->dev);
#endif

	ret = fvmap_init(get_fvmap_base());
out:
	return ret;
}

static const struct of_device_id cal_if_match[] = {
	{ .compatible = "samsung,exynos_cal_if" },
	{},
};
MODULE_DEVICE_TABLE(of, cal_if_match);

static struct platform_driver samsung_cal_if_driver = {
	.probe	= cal_if_probe,
	.driver	= {
		.name = "exynos-cal-if",
		.owner	= THIS_MODULE,
		.of_match_table	= cal_if_match,
	},
};

static int exynos_cal_if_init(void)
{
	int ret;

	ret = platform_driver_register(&samsung_cal_if_driver);
	if (ret) {
		pr_err("samsung_cal_if_driver probe file\n");
		goto err_out;
	}

	ret = exynos_acpm_dvfs_init();
	if (ret) {
		pr_err("samsung_cal_if_driver probe file\n");
		goto err_out;
	}
#if IS_ENABLED(CONFIG_SOC_S5E9945) || IS_ENABLED(CONFIG_SOC_S5E9945_EVT0) || IS_ENABLED(CONFIG_SOC_S5E8845) || IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E8855)
#if defined(CONFIG_EXYNOS_POWER_RAIL_DBG) || defined(CONFIG_EXYNOS_POWER_RAIL_DBG_MODULE)
	ret = exynos_power_rail_dbg_init();
	if (ret) {
		pr_err("samsung_cal_if_driver probe file\n");
		goto err_out;
	}
#endif
#endif

#if defined(CONFIG_CMU_EWF) || defined(CONFIG_CMU_EWF_MODULE)
	ret = early_wakeup_forced_enable_init();
	if (ret) {
		pr_err("samsung_cal_if_driver probe file\n");
		goto err_out;
	}
#endif

err_out:
	return ret;
}
arch_initcall(exynos_cal_if_init);

/* CONFIG_SEC_DEBUG */
MODULE_SOFTDEP("pre: sec_debug_base_early");
/* end CONFIG_SEC_DEBUG */
MODULE_LICENSE("GPL");
