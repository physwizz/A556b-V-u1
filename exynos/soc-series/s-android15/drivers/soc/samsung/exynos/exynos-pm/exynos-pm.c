/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/wakeup_reason.h>
#include <linux/syscore_ops.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/debugfs.h>
#include <linux/smp.h>

#include <soc/samsung/exynos-pm.h>
#include <soc/samsung/exynos-pmu-if.h>
#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-s2i.h>
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
#include <soc/samsung/esca.h>
#endif

extern u32 exynos_eint_to_pin_num(int eint);
extern u32 exynos_eint_wake_mask_array[3];
extern void log_wakeup_reason(int irq);

static struct exynos_pm_info *pm_info;
static struct exynos_pm_dbg *pm_dbg;

static void exynos_print_wakeup_sources(int irq, const char *name)
{
	struct irq_desc *desc;

	if (irq < 0) {
		if (name)
			pr_info("PM: Resume caused by SYSINT: %s\n", name);

		return;
	}

	desc = irq_to_desc(irq);
	if (desc && desc->action && desc->action->name)
		pr_info("PM: Resume caused by IRQ %d, %s\n", irq,
				desc->action->name);
	else
		pr_info("PM: Resume caused by IRQ %d\n", irq);
}

static void exynos_show_wakeup_reason_eint(void)
{
	int bit;
	int i, acc_bits;
	long unsigned int ext_int_pend;
	u64 eint_wakeup_mask;
	bool found = 0;
	unsigned int val0 = 0, val1 = 0;

	exynos_pmu_read(pm_info->eint_wakeup_mask_offset[0], &val0);
	exynos_pmu_read(pm_info->eint_wakeup_mask_offset[1], &val1);
	eint_wakeup_mask = val1;
	eint_wakeup_mask = ((eint_wakeup_mask << 32) | val0);

	for (i = 0, acc_bits = 0; i < pm_info->total_eint_grps; acc_bits += pm_info->num_eint_pends[i], i++) {
		ext_int_pend = __raw_readl(pm_info->eint_base + pm_info->eint_pend_offsets[i]);

		for_each_set_bit(bit, &ext_int_pend, pm_info->num_eint_pends[i]) {
			u32 gpio;
			int irq;

			if (eint_wakeup_mask & (1 << (acc_bits + bit)))
				continue;

			gpio = exynos_eint_to_pin_num(acc_bits + bit);
			irq = gpio_to_irq(gpio);

			exynos_print_wakeup_sources(irq, NULL);
			found = 1;
		}
	}

	if (!found)
		pr_info("%s Resume caused by unknown EINT\n", EXYNOS_PM_PREFIX);
}

static void exynos_show_wakeup_registers(unsigned int wakeup_stat)
{
	int i;

	pr_info("WAKEUP_STAT:\n");
	for (i = 0; i < pm_info->num_wakeup_stat; i++) {
		exynos_pmu_read(pm_info->wakeup_stat_offset[i], &wakeup_stat);
		pr_info("0x%08x\n", wakeup_stat);
	}

	pr_info("EINT_PEND: ");
	for (i = 0; i < pm_info->total_eint_grps; i++)
		pr_info("0x%02x ", __raw_readl(pm_info->eint_base + pm_info->eint_pend_offsets[i]));
}

static void exynos_show_wakeup_reason_sysint(unsigned int stat,
		struct wakeup_stat_name *ws_names)
{
	int bit;
	unsigned long lstat = stat;
	const char *name;

	for_each_set_bit(bit, &lstat, 32) {
		name = ws_names->name[bit];

		if (!name)
			continue;

		exynos_print_wakeup_sources(-1, name);
	}
}

static int exynos_show_wakeup_reason_detail(unsigned int wakeup_stat)
{
	int i;
	unsigned int wss;

	if (wakeup_stat & (1 << pm_info->wakeup_stat_eint))
		exynos_show_wakeup_reason_eint();

	if (unlikely(!pm_info->ws_names))
		return 0;

	for (i = 0; i < pm_info->num_wakeup_stat; i++) {
		if (i == 0)
			wss = wakeup_stat & ~(1 << pm_info->wakeup_stat_eint);
		else
			exynos_pmu_read(pm_info->wakeup_stat_offset[i], &wss);

		if (!wss)
			continue;

		exynos_show_wakeup_reason_sysint(wss, &pm_info->ws_names[i]);
	}

	return 1;
}

static void exynos_show_wakeup_reason(bool sleep_abort)
{
	unsigned int wakeup_stat;
	int i;

	if (sleep_abort) {
		pr_info("%s early wakeup! Dumping pending registers...\n", EXYNOS_PM_PREFIX);

		pr_info("EINT_PEND:\n");
		for (i = 0; i < pm_info->total_eint_grps; i++)
			pr_info("0x%02x ", __raw_readl(pm_info->eint_base + pm_info->eint_pend_offsets[i]));

		pr_info("GIC_PEND:\n");
		for (i = 0; i < pm_info->num_gic; i++)
			pr_info("GICD_ISPENDR[%d] = 0x%x\n", i, __raw_readl(pm_info->gic_base + i*4));

		pr_info("%s done.\n", EXYNOS_PM_PREFIX);
		return ;
	}

	if (!pm_info->num_wakeup_stat)
		return;

	exynos_pmu_read(pm_info->wakeup_stat_offset[0], &wakeup_stat);
	exynos_show_wakeup_registers(wakeup_stat);

	if (exynos_show_wakeup_reason_detail(wakeup_stat))
		return;

	if (wakeup_stat & (1 << pm_info->wakeup_stat_rtc))
		pr_info("%s Resume caused by RTC alarm\n", EXYNOS_PM_PREFIX);
	else if (wakeup_stat & (1 << pm_info->wakeup_stat_eint))
		exynos_show_wakeup_reason_eint();
	else {
		for (i = 0; i < pm_info->num_wakeup_stat; i++) {
			exynos_pmu_read(pm_info->wakeup_stat_offset[i], &wakeup_stat);
			pr_info("%s Resume caused by wakeup%d_stat 0x%08x\n",
					EXYNOS_PM_PREFIX, i + 1, wakeup_stat);

		}
	}
}
static void exynos_set_wakeupmask(enum sys_powerdown mode)
{
	int i;
	u32 wakeup_int_en = 0;

	/* Set external interrupt mask */
	for (i = 0; i < pm_info->num_eint_wakeup_mask; i++)
		exynos_pmu_write(pm_info->eint_wakeup_mask_offset[i], exynos_eint_wake_mask_array[i]);

	for (i = 0; i < pm_info->num_wakeup_int_en; i++) {
		exynos_pmu_write(pm_info->wakeup_stat_offset[i], 0);
		wakeup_int_en = pm_info->wakeup_int_en[i];

		if (otg_is_connect() == 2 || otg_is_connect() == 0)
			wakeup_int_en&= ~pm_info->usbl2_wakeup_int_en[i];

		exynos_pmu_write(pm_info->wakeup_int_en_offset[i], wakeup_int_en);
	}


	__raw_writel(pm_info->vgpio_wakeup_inten,
			pm_info->vgpio2pmu_base + pm_info->vgpio_inten_offset);
}

static int exynos_prepare_sys_powerdown(enum sys_powerdown mode)
{
	int ret;

	exynos_set_wakeupmask(mode);

	ret = cal_pm_enter(mode);
	if (ret)
		pr_err("CAL Fail to set powermode\n");

	return ret;
}

static void exynos_wakeup_sys_powerdown(enum sys_powerdown mode, bool early_wakeup)
{
	u32 reg;

	if (early_wakeup)
		cal_pm_earlywakeup(mode);
	else
		cal_pm_exit(mode);

	reg = __raw_readl(pm_info->vgpio2pmu_base + pm_info->vgpio_inten_offset);
	__raw_writel(reg, pm_info->vgpio2pmu_base + pm_info->vgpio_inten_offset + 0x4);
}

static void print_dbg_subsystem(void)
{
	int i = 0;
	u32 reg;

	if (!pm_info->num_dbg_subsystem)
		return;

	pr_info("%s Debug Subsystem:\n", EXYNOS_PM_PREFIX);

	for (i = 0; i < pm_info->num_dbg_subsystem; i++) {
		exynos_pmu_read(pm_info->dbg_subsystem_offset[i], &reg);

		pr_info("%s: 0x%08x\n", pm_info->dbg_subsystem_name[i], reg);
	}
}

#if IS_ENABLED(CONFIG_PINCTRL_SEC_GPIO_DVS)
extern void gpio_dvs_check_sleepgpio(void);
#endif

static int exynos_pm_syscore_suspend(void)
{
#ifdef CONFIG_CP_PMUCAL
	if (!exynos_check_cp_status()) {
		pr_info("%s %s: sleep canceled by CP reset \n",
					EXYNOS_PM_PREFIX, __func__);
		return -EINVAL;
	}
#endif

#if IS_ENABLED(CONFIG_PINCTRL_SEC_GPIO_DVS)
	/************************ Caution !!! ****************************/
	/* This function must be located in appropriate SLEEP position
	 * in accordance with the specification of each BB vendor.
	 */
	/************************ Caution !!! ****************************/
	gpio_dvs_check_sleepgpio();
#endif

	pm_info->is_pcieon_suspend = false;
	if (pm_info->pcieon_suspend_available) {
		if (!IS_ERR_OR_NULL(pm_info->pcie_is_connect))
			pm_info->is_pcieon_suspend = !!pm_info->pcie_is_connect();
	}

	if (pm_info->is_pcieon_suspend) {
		exynos_prepare_sys_powerdown(pm_info->pcieon_suspend_mode_idx);
		pr_info("%s %s: Enter Suspend scenario. pcieon_mode_idx = %d)\n",
				EXYNOS_PM_PREFIX,__func__, pm_info->pcieon_suspend_mode_idx);
	} else {
		exynos_prepare_sys_powerdown(pm_info->suspend_mode_idx);
		pr_info("%s %s: Enter Suspend scenario. suspend_mode_idx = %d)\n",
				EXYNOS_PM_PREFIX,__func__, pm_info->suspend_mode_idx);
	}

	/* Send an IPI if test_early_wakeup flag is set */
//	if (pm_dbg->test_early_wakeup)
//		arch_send_call_function_single_ipi(0);

#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
	pm_dbg->mifdn_cnt_prev = esca_get_mifdn_sleep_count();
	pm_info->apdn_cnt_prev = esca_get_apsleep_count();
	pm_dbg->mif_req = esca_get_mif_request();

	pm_dbg->mifdn_early_wakeup_prev = esca_get_apsoc_sleep_early_wakeup_count();
#else
	pm_dbg->mifdn_cnt_prev = acpm_get_mifdn_sleep_count();
	pm_info->apdn_cnt_prev = acpm_get_apsleep_count();
	pm_dbg->mif_req = acpm_get_mif_request();

	pm_dbg->mifdn_early_wakeup_prev = acpm_get_apsoc_sleep_early_wakeup_count();
#endif

	pr_info("%s: prev mif_count:%d, apsoc_count:%d, seq_early_wakeup_count:%d\n",
			EXYNOS_PM_PREFIX, pm_dbg->mifdn_cnt_prev,
			pm_info->apdn_cnt_prev, pm_dbg->mifdn_early_wakeup_prev);
	exynos_flexpmu_dbg_mif_req();

	return 0;
}

static void exynos_pm_syscore_resume(void)
{
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
	pm_dbg->mifdn_cnt = esca_get_mifdn_sleep_count();
	pm_info->apdn_cnt = esca_get_apsleep_count();
	pm_dbg->mifdn_early_wakeup_cnt = esca_get_apsoc_sleep_early_wakeup_count();
#else
	pm_dbg->mifdn_cnt = acpm_get_mifdn_sleep_count();
	pm_info->apdn_cnt = acpm_get_apsleep_count();
	pm_dbg->mifdn_early_wakeup_cnt = acpm_get_apsoc_sleep_early_wakeup_count();
#endif

	pr_info("%s: post mif_count:%d, apsoc_count:%d, seq_early_wakeup_count:%d\n",
			EXYNOS_PM_PREFIX, pm_dbg->mifdn_cnt,
			pm_info->apdn_cnt, pm_dbg->mifdn_early_wakeup_cnt);

	if (pm_info->apdn_cnt == pm_info->apdn_cnt_prev) {
		pm_info->is_early_wakeup = true;
		pr_info("%s %s: return to originator\n",
				EXYNOS_PM_PREFIX, __func__);
	} else
		pm_info->is_early_wakeup = false;

	if (pm_dbg->mifdn_early_wakeup_cnt != pm_dbg->mifdn_early_wakeup_prev)
		pr_debug("%s: Sequence early wakeup\n", EXYNOS_PM_PREFIX);

	if (pm_dbg->mifdn_cnt == pm_dbg->mifdn_cnt_prev) {
		pr_info("%s: MIF blocked. MIF request Mster was  0x%x\n",
				EXYNOS_PM_PREFIX, pm_dbg->mif_req);
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
		esca_print_mif_request();
#else
		acpm_print_mif_request();
#endif
	} else {
		pr_info("%s: MIF down. cur_count: %d, acc_count: %d\n",
				EXYNOS_PM_PREFIX, pm_dbg->mifdn_cnt - pm_dbg->mifdn_cnt_prev, pm_dbg->mifdn_cnt);
	}

	if (pm_info->is_pcieon_suspend)
		exynos_wakeup_sys_powerdown(pm_info->pcieon_suspend_mode_idx, pm_info->is_early_wakeup);
	else
		exynos_wakeup_sys_powerdown(pm_info->suspend_mode_idx, pm_info->is_early_wakeup);

	exynos_flexpmu_dbg_mif_req();

#if IS_ENABLED(CONFIG_ESCA_SLEEP_PROFILER)
	exynos_flexpmu_dbg_sleep_latency();
#endif
	print_dbg_subsystem();
	exynos_show_wakeup_reason(pm_info->is_early_wakeup);

	if (!pm_info->is_early_wakeup)
		pr_debug("%s %s: post sleep, preparing to return\n",
						EXYNOS_PM_PREFIX, __func__);
}

static struct syscore_ops exynos_pm_syscore_ops = {
	.suspend	= exynos_pm_syscore_suspend,
	.resume		= exynos_pm_syscore_resume,
};

static struct exynos_s2i_ops exynos_pm_s2i_ops = {
	.suspend	= exynos_pm_syscore_suspend,
	.resume		= exynos_pm_syscore_resume,
};

#if IS_ENABLED(CONFIG_PCI_EXYNOS)
int register_pcie_is_connect(u32 (*func)(void))
{
	if(func) {
		pm_info->pcie_is_connect = func;
		pr_info("Registered pcie_is_connect func\n");
		return 0;
	} else {
		pr_err("%s	:function pointer is NULL \n", __func__);
		return -ENXIO;
	}
}
EXPORT_SYMBOL_GPL(register_pcie_is_connect);
#endif

#ifdef CONFIG_DEBUG_FS
static int wake_lock_get(void *data, unsigned long long *val)
{
	*val = (unsigned long long)pm_info->is_stay_awake;
	return 0;
}

static int wake_lock_set(void *data, unsigned long long val)
{
	bool before = pm_info->is_stay_awake;

	pm_info->is_stay_awake = (bool)val;

	if (before != pm_info->is_stay_awake) {
		if (pm_info->is_stay_awake)
			__pm_stay_awake(pm_info->ws);
		else
			__pm_relax(pm_info->ws);
	}
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(wake_lock_fops, wake_lock_get, wake_lock_set, "%llu\n");

static void exynos_pm_debugfs_init(void)
{
	struct dentry *root;

	root = debugfs_create_dir("exynos-pm", NULL);
	if (!root) {
		pr_err("%s %s: could't create debugfs dir\n", EXYNOS_PM_PREFIX, __func__);
		return;
	}

#if 0
	d = debugfs_create_file("test_early_wakeup", 0644, root, NULL, &pm_dbg->test_early_wakeup);
	if (!d) {
		pr_err("%s %s: could't create debugfs test_early_wakeup\n",
					EXYNOS_PM_PREFIX, __func__);
		return;
	}
#endif

	if (pm_info->ws)
		debugfs_create_file("wake_lock", 0644, root, NULL, &wake_lock_fops);
}
#endif

static void parse_dt_wakeup_stat_names(struct device_node *np)
{
	struct device_node *root, *child;
	int ret;
	int size, n, idx = 0;

	root = of_find_node_by_name(np, "wakeup_stats");
	n = of_get_child_count(root);

	if (pm_info->num_wakeup_stat != n || !n) {
		pr_err("%s: failed to get wakeup_stats(%d)\n", __func__, n);
		return;
	}

	pm_info->ws_names = kzalloc(sizeof(*pm_info->ws_names) * n, GFP_KERNEL);
	if (!pm_info->ws_names)
		return;

	for_each_child_of_node(root, child) {
		size = of_property_count_strings(child, "ws-name");
		if (size <= 0 || size > 32) {
			pr_err("%s: failed to get wakeup_stat name cnt(%d)\n",
					__func__, size);
			return;
		}

		ret = of_property_read_string_array(child, "ws-name",
				pm_info->ws_names[idx].name, size);
		if (ret < 0) {
			pr_err("%s: failed to read wakeup_stat name(%d)\n",
					__func__, ret);
			return;
		}

		idx++;
	}
}

static void parse_dt_debug_subsystem(struct device_node *np)
{
	struct device_node *child;
	int ret;
	int size_name, size_offset = 0;

	child = of_find_node_by_name(np, "debug_subsystem");

	if (!child) {
		pr_err("%s %s: unable to get debug_subsystem value from DT\n",
				EXYNOS_PM_PREFIX, __func__);
		return;
	}

	size_name = of_property_count_strings(child, "sfr-name");
	size_offset = of_property_count_u32_elems(child, "sfr-offset");

	if (size_name != size_offset) {
		pr_err("%s %s: size mismatch(%d, %d)\n",
				EXYNOS_PM_PREFIX, __func__, size_name, size_offset);
		return;
	}

	pm_info->dbg_subsystem_name = kcalloc(size_name, sizeof(const char *), GFP_KERNEL);
	if (!pm_info->dbg_subsystem_name)
		return;

	pm_info->dbg_subsystem_offset = kcalloc(size_offset, sizeof(unsigned int), GFP_KERNEL);
	if (!pm_info->dbg_subsystem_offset)
		goto free_name;

	ret = of_property_read_string_array(child, "sfr-name",
			pm_info->dbg_subsystem_name, size_name);
	if (ret < 0) {
		pr_err("%s %s: failed to get debug_subsystem name from DT\n",
				EXYNOS_PM_PREFIX, __func__);
		goto free_offset;
	}

	ret = of_property_read_u32_array(child, "sfr-offset",
			pm_info->dbg_subsystem_offset, size_offset);
	if (ret < 0) {
		pr_err("%s %s: failed to get debug_subsystem offset from DT\n",
				EXYNOS_PM_PREFIX, __func__);
		goto free_offset;
	}

	pm_info->num_dbg_subsystem = size_name;
	return;

free_offset:
	kfree(pm_info->dbg_subsystem_offset);
free_name:
	kfree(pm_info->dbg_subsystem_name);
	pm_info->num_dbg_subsystem = 0;
}


static void parse_dt_eint_pend(struct device_node *np)
{
	int ret;
	u32 num_a = 0, num_b = 0, i;

	ret = of_property_count_u32_elems(np, "eint-pend-offsets");
	if (ret > 0)
		num_a = ret;

	ret = of_property_count_u32_elems(np, "num-eint-pends");
	if (ret > 0)
		num_b = ret;

	if (num_a != num_b)
		return;

	pm_info->eint_pend_offsets = kcalloc(num_a, sizeof(u32), GFP_KERNEL);
	if (!pm_info->eint_pend_offsets)
		return;

	pm_info->num_eint_pends = kcalloc(num_a, sizeof(u32), GFP_KERNEL);
	if (!pm_info->num_eint_pends)
		goto free_offset;

	ret = of_property_read_u32_array(np, "eint-pend-offsets", pm_info->eint_pend_offsets, num_a);
	if (ret < 0)
		goto free_num;

	ret = of_property_read_u32_array(np, "num-eint-pends", pm_info->num_eint_pends, num_a);
	if (ret < 0)
		goto free_num;

	for (i = 0; i < num_a; i++)
		pm_info->total_eint_pends += pm_info->num_eint_pends[i];

	pm_info->total_eint_grps = num_a;

	return ;

free_num:
	kfree(pm_info->num_eint_pends);
free_offset:
	kfree(pm_info->eint_pend_offsets);
	return ;
}


static int exynos_pm_drvinit(void)
{
	int ret;

	pm_info = kzalloc(sizeof(struct exynos_pm_info), GFP_KERNEL);
	if (pm_info == NULL) {
		pr_err("%s %s: failed to allocate memory for exynos_pm_info\n",
					EXYNOS_PM_PREFIX, __func__);
		BUG();
	}

	pm_dbg = kzalloc(sizeof(struct exynos_pm_dbg), GFP_KERNEL);
	if (pm_dbg == NULL) {
		pr_err("%s %s: failed to allocate memory for exynos_pm_dbg\n",
					EXYNOS_PM_PREFIX, __func__);
		BUG();
	}

	if (of_have_populated_dt()) {
		struct device_node *np;
		unsigned int wake_lock = 0;
		np = of_find_compatible_node(NULL, NULL, "samsung,exynos-pm");
		if (!np) {
			pr_err("%s %s: unabled to find compatible node (%s)\n",
					EXYNOS_PM_PREFIX, __func__, "samsung,exynos-pm");
			BUG();
		}

		pm_info->eint_base = of_iomap(np, 0);
		if (!pm_info->eint_base) {
			pr_err("%s %s: unabled to ioremap EINT base address\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		pm_info->gic_base = of_iomap(np, 1);
		if (!pm_info->gic_base) {
			pr_err("%s %s: unbaled to ioremap GIC base address\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		pm_info->vgpio2pmu_base = of_iomap(np, 2);
		if (!pm_info->vgpio2pmu_base) {
			pr_err("%s %s: unbaled to ioremap VGPIO2PMU base address\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		parse_dt_eint_pend(np);

		ret = of_property_read_u32(np, "num-gic", &pm_info->num_gic);
		if (ret) {
			pr_err("%s %s: unabled to get the number of gic from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		ret = of_property_read_u32(np, "wakeup-stat-eint", &pm_info->wakeup_stat_eint);
		if (ret) {
			pr_err("%s %s: unabled to get the eint bit file of WAKEUP_STAT from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		ret = of_property_read_u32(np, "wakeup-stat-rtc", &pm_info->wakeup_stat_rtc);
		if (ret) {
			pr_err("%s %s: unabled to get the rtc bit file of WAKEUP_STAT from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		ret = of_property_read_u32(np, "suspend_mode_idx", &pm_info->suspend_mode_idx);
		if (ret) {
			pr_err("%s %s: unabled to get suspend_mode_idx from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		ret = of_property_count_u32_elems(np, "wakeup_stat_offset");
		if (!ret) {
			pr_err("%s %s: unabled to get wakeup_stat value from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		} else if (ret > 0) {
			pm_info->num_wakeup_stat = ret;
			pm_info->wakeup_stat_offset = kzalloc(sizeof(unsigned int) * ret, GFP_KERNEL);
			of_property_read_u32_array(np, "wakeup_stat_offset", pm_info->wakeup_stat_offset, ret);
		}

		parse_dt_wakeup_stat_names(np);
		parse_dt_debug_subsystem(np);

		ret = of_property_count_u32_elems(np, "wakeup_int_en_offset");
		if (!ret) {
			pr_err("%s %s: unabled to get wakeup_int_en_offset value from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		} else if (ret > 0) {
			pm_info->num_wakeup_int_en = ret;
			pm_info->wakeup_int_en_offset = kzalloc(sizeof(unsigned int) * ret, GFP_KERNEL);
			of_property_read_u32_array(np, "wakeup_int_en_offset", pm_info->wakeup_int_en_offset, ret);
		}

		ret = of_property_count_u32_elems(np, "wakeup_int_en");
		if (!ret) {
			pr_err("%s %s: unabled to get wakeup_int_en value from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		} else if (ret > 0) {
			pm_info->wakeup_int_en = kzalloc(sizeof(unsigned int) * ret, GFP_KERNEL);
			of_property_read_u32_array(np, "wakeup_int_en", pm_info->wakeup_int_en, ret);
		}

		ret = of_property_count_u32_elems(np, "usbl2_wakeup_int_en");
		if (!ret) {
			pr_err("%s %s: dose not support usbl2 sleep\n", EXYNOS_PM_PREFIX, __func__);
		} else if (ret > 0) {
			pm_info->usbl2_wakeup_int_en = kzalloc(sizeof(unsigned int) * ret, GFP_KERNEL);
			of_property_read_u32_array(np, "usbl2_wakeup_int_en", pm_info->usbl2_wakeup_int_en, ret);
		}

		ret = of_property_count_u32_elems(np, "eint_wakeup_mask_offset");
		if (!ret) {
			pr_err("%s %s: unabled to get eint_wakeup_mask_offset value from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		} else if (ret > 0) {
			pm_info->num_eint_wakeup_mask = ret;
			pm_info->eint_wakeup_mask_offset = kzalloc(sizeof(unsigned int) * ret, GFP_KERNEL);
			of_property_read_u32_array(np, "eint_wakeup_mask_offset", pm_info->eint_wakeup_mask_offset, ret);
		}

		ret = of_property_read_u32(np, "vgpio_wakeup_inten", &pm_info->vgpio_wakeup_inten);
		if (ret) {
			pr_err("%s %s: unabled to get the number of eint from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		ret = of_property_read_u32(np, "vgpio_wakeup_inten_offset", &pm_info->vgpio_inten_offset);
		if (ret) {
			pr_err("%s %s: unabled to get the number of eint from DT\n",
					EXYNOS_PM_PREFIX, __func__);
			BUG();
		}

		ret = of_property_read_u32(np, "wake_lock", &wake_lock);
		if (ret) {
			pr_info("%s %s: unabled to get wake_lock from DT\n",
					EXYNOS_PM_PREFIX, __func__);
		} else {
			pm_info->ws = wakeup_source_register(NULL, "exynos-pm");
			if (!pm_info->ws)
				BUG();

			pm_info->is_stay_awake = (bool)wake_lock;

			if (pm_info->is_stay_awake)
				__pm_stay_awake(pm_info->ws);
		}

		ret = of_property_read_u32(np, "pcieon_suspend_available", &pm_info->pcieon_suspend_available);
		if (ret) {
			pr_info("%s %s: Not support pcieon_suspend mode\n",
					EXYNOS_PM_PREFIX, __func__);
		} else {
			ret = of_property_read_u32(np, "pcieon_suspend_mode_idx", &pm_info->pcieon_suspend_mode_idx);
			if (ret) {
				pr_err("%s %s: unabled to get pcieon_suspend_mode_idx from DT\n",
						EXYNOS_PM_PREFIX, __func__);
				BUG();
			}
		}

	} else {
		pr_err("%s %s: failed to have populated device tree\n",
					EXYNOS_PM_PREFIX, __func__);
		BUG();
	}

	register_exynos_s2i_ops(&exynos_pm_s2i_ops);
	register_syscore_ops(&exynos_pm_syscore_ops);
#ifdef CONFIG_DEBUG_FS
	exynos_pm_debugfs_init();
#endif

	return 0;
}
arch_initcall(exynos_pm_drvinit);

MODULE_LICENSE("GPL");
