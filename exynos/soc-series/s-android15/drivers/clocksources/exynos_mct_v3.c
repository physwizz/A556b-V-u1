// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Exynos MCT(Multi-Core Timer) support
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/percpu.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/clocksource.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <trace/events/power.h>
#include <trace/hooks/cpuidle_psci.h>
#include <soc/samsung/exynos/debug-snapshot.h>
#include "exynos_mct_v3.h"

static void __iomem *reg_base;
static unsigned long osc_clk_rate;
static int mct_irqs[MCT_NR_COMPS];

static void __iomem *gic_base;

#if IS_ENABLED(CONFIG_EXYNOS_HARDLOCKUP_WATCHDOG)
extern struct atomic_notifier_head hardlockup_notifier_list;
#endif

static void exynos_mct_set_compensation(unsigned long osc, unsigned long rtc)
{
	unsigned int osc_rtc;
	unsigned int incr_rtcclk;
	unsigned int compen_val;

	osc_rtc = (unsigned int)(osc * 1000 / rtc);

	/* MCT_INCR_RTCCLK is integer part of (OSCCLK frequency/RTCCLK frequency). */
	incr_rtcclk = (osc / rtc) + ((osc % rtc) ? 1 : 0);

	/* MCT_COMPENSATE_VALUE is decimal part of (OSCCLK frequency/RTCCLK frequency). */
	compen_val = ((osc_rtc + 5) / 10) % 100;
	if (compen_val)
		compen_val = 100 - compen_val;

	pr_info("MCT: osc-%lu rtc-%lu incr_rtcclk:0x%08x compen_val:0x%08x\n",
		osc, rtc, incr_rtcclk, compen_val);

	writel_relaxed(incr_rtcclk, reg_base + EXYNOS_MCT_MCT_INCR_RTCCLK);
	writel_relaxed(compen_val, reg_base + EXYNOS_MCT_COMPENSATE_VALUE);
}

/* Clocksource handling */
static void exynos_mct_frc_start(void)
{
	writel_relaxed(MCT_FRC_ENABLE, reg_base + EXYNOS_MCT_MCT_FRC_ENABLE);
}

/**
 * exynos_read_count_32 - Read the lower 32-bits of the global counter
 *
 * This will read just the lower 32-bits of the global counter.
 *
 * Returns the number of cycles in the global counter (lower 32 bits).
 */
static u32 exynos_read_count_32(void)
{
	return readl_relaxed(reg_base + EXYNOS_MCT_CNT_L);
}

static u64 exynos_frc_read(struct clocksource *cs)
{
	return exynos_read_count_32();
}

static struct clocksource mct_frc = {
	.name		= "mct-frc",
	.rating		= 350,	/* use value lower than ARM arch timer */
	.read		= exynos_frc_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

static int exynos_clocksource_init(void)
{
	if (clocksource_register_hz(&mct_frc, osc_clk_rate))
		panic("%s: can't register clocksource\n", mct_frc.name);

	return 0;
}

static void exynos_mct_comp_stop(struct mct_clock_event_device *mevt)
{
	unsigned int index = mevt->comp_index;
	unsigned int comp_enable;
	unsigned int loop_cnt = 0;

	writel_relaxed(MCT_COMP_DISABLE, reg_base + EXYNOS_MCT_COMP_ENABLE(index));

	/* Wait maximum 1 ms until COMP_ENABLE_n = 0 */
	do {
		comp_enable = readl_relaxed(reg_base + EXYNOS_MCT_COMP_ENABLE(index));
		loop_cnt++;
	} while (comp_enable != MCT_COMP_DISABLE && loop_cnt < WAIT_LOOP_CNT);

	if (loop_cnt == WAIT_LOOP_CNT)
		panic("MCT(comp%d) disable timeout\n", index);

	writel_relaxed(MCT_COMP_NON_CIRCULAR_MODE, reg_base + EXYNOS_MCT_COMP_MODE(index));
	writel_relaxed(MCT_INT_DISABLE, reg_base + EXYNOS_MCT_INT_ENB(index));
	writel_relaxed(MCT_CSTAT_CLEAR, reg_base + EXYNOS_MCT_INT_CSTAT(index));
}

static void exynos_mct_comp_start(struct mct_clock_event_device *mevt,
				  bool periodic, unsigned long cycles)
{
	unsigned int index = mevt->comp_index;
	unsigned int comp_enable;
	unsigned int loop_cnt = 0;

	comp_enable = readl_relaxed(reg_base + EXYNOS_MCT_COMP_ENABLE(index));
	if (comp_enable == MCT_COMP_ENABLE)
		exynos_mct_comp_stop(mevt);

	if (periodic)
		writel_relaxed(MCT_COMP_CIRCULAR_MODE, reg_base + EXYNOS_MCT_COMP_MODE(index));

	writel_relaxed(cycles, reg_base + EXYNOS_MCT_COMP_PERIOD(index));
	writel_relaxed(MCT_INT_ENABLE, reg_base + EXYNOS_MCT_INT_ENB(index));
	writel_relaxed(MCT_COMP_ENABLE, reg_base + EXYNOS_MCT_COMP_ENABLE(index));

	/* Wait maximum 1 ms until COMP_ENABLE_n = 1 */
	do {
		comp_enable = readl_relaxed(reg_base + EXYNOS_MCT_COMP_ENABLE(index));
		loop_cnt++;
	} while (comp_enable != MCT_COMP_ENABLE && loop_cnt < WAIT_LOOP_CNT);

	if (loop_cnt == WAIT_LOOP_CNT)
		panic("MCT(comp%d) enable timeout\n", index);
}

static int exynos_comp_set_next_event(unsigned long cycles, struct clock_event_device *evt)
{
	struct mct_clock_event_device *mevt;

	mevt = container_of(evt, struct mct_clock_event_device, evt);

	exynos_mct_comp_start(mevt, false, cycles);

	if(mevt->in_s2idle_c2)
		pr_info("%s: cyc[%lu]\n", __func__, cycles);

	return 0;
}

static int mct_set_state_shutdown(struct clock_event_device *evt)
{
	struct mct_clock_event_device *mevt;

	mevt = container_of(evt, struct mct_clock_event_device, evt);

	exynos_mct_comp_stop(mevt);

	return 0;
}

static int mct_set_state_periodic(struct clock_event_device *evt)
{
	unsigned long cycles_per_jiffy;
	struct mct_clock_event_device *mevt;

	mevt = container_of(evt, struct mct_clock_event_device, evt);

	cycles_per_jiffy = (((unsigned long long)NSEC_PER_SEC / HZ * evt->mult) >> evt->shift);
	exynos_mct_comp_start(mevt, true, cycles_per_jiffy);

	return 0;
}

static irqreturn_t exynos_mct_comp_isr(int irq, void *dev_id)
{
	struct mct_clock_event_device *mevt = dev_id;
	struct clock_event_device *evt = &mevt->evt;
	unsigned int index = mevt->comp_index;

	writel_relaxed(MCT_CSTAT_CLEAR, reg_base + EXYNOS_MCT_INT_CSTAT(index));

	if (!evt->event_handler)
		dbg_snapshot_expire_watchdog();
	else
		evt->event_handler(evt);

	if (mevt->in_s2idle_c2)
		mevt->in_s2idle_c2 = false;

	return IRQ_HANDLED;
}

static DEFINE_PER_CPU(struct mct_clock_event_device, percpu_mct_tick);

#if IS_ENABLED(CONFIG_EXYNOS_HARDLOCKUP_WATCHDOG)
static void dump_gic_info(void)
{
	int mct_hwirq = irq_get_irq_data(mct_irqs[0])->hwirq;
	void __iomem *gicd_isenabler, *gicd_ispendr, *gicd_isactiver;
	int n = mct_hwirq / 32;

	gicd_isenabler = gic_base + (0x100 + 4 * n);
	gicd_ispendr = gic_base + (0x200 + 4 * n);
	gicd_isactiver = gic_base + (0x300 + 4 * n);

	pr_info("GICD_ISENABLER\n");
	pr_info("0x%px: 0x%08x\n", gicd_isenabler, readl(gicd_isenabler));
	pr_info("0x%px: 0x%08x\n", gicd_isenabler+0x4, readl(gicd_isenabler+0x4));

	pr_info("GICD_ISPENDR\n");
	pr_info("0x%px: 0x%08x\n", gicd_ispendr, readl(gicd_ispendr));
	pr_info("0x%px: 0x%08x\n", gicd_ispendr+0x4, readl(gicd_ispendr+0x4));

	pr_info("GICD_ISACTIVER\n");
	pr_info("0x%px: 0x%08x\n", gicd_isactiver, readl(gicd_isactiver));
	pr_info("0x%px: 0x%08x\n", gicd_isactiver+0x4, readl(gicd_isactiver+0x4));
}

static int exynos_mct_lockup_handler(struct notifier_block *nb,
				     unsigned long l, void *core)
{
	unsigned int cpu = *(unsigned int *)core;
	unsigned int frc_en, clkmux_sel, cnt_l, int_en, int_cstat;
	unsigned int comp_en, comp_l, comp_mode, comp_period;
	int i;

	/* 5 times to verify */
	for (i = 0; i < 5; i++) {
		frc_en = readl_relaxed(reg_base + EXYNOS_MCT_MCT_FRC_ENABLE);
		clkmux_sel = readl_relaxed(reg_base + EXYNOS_MCT_CLKMUX_SEL);
		cnt_l = readl_relaxed(reg_base + EXYNOS_MCT_CNT_L);
		int_en = readl_relaxed(reg_base + EXYNOS_MCT_INT_ENB(cpu));
		int_cstat = readl_relaxed(reg_base + EXYNOS_MCT_INT_CSTAT(cpu));
		comp_en = readl_relaxed(reg_base + EXYNOS_MCT_COMP_ENABLE(cpu));
		comp_l = readl_relaxed(reg_base + EXYNOS_MCT_COMP_L(cpu));
		comp_mode = readl_relaxed(reg_base + EXYNOS_MCT_COMP_MODE(cpu));
		comp_period = readl_relaxed(reg_base + EXYNOS_MCT_COMP_PERIOD(cpu));

		pr_info("%s(cpu:%d)\n"
			"FRC_EN:0x%X CLKMUX_SEL:0x%X CNT_L:0x%X INT_EN:0x%X\n"
			"INT_CSTAT:0x%X COMP_EN:0x%X COMP_L:0x%X COMP_MODE:0x%X COMP_PERIOD:0x%X\n",
			__func__, cpu, frc_en, clkmux_sel, cnt_l, int_en,
			int_cstat, comp_en, comp_l, comp_mode, comp_period);
	}

	if (gic_base)
		dump_gic_info();

	return 0;
}

static struct notifier_block exynos_mct_lockup_nb = {
	.notifier_call = exynos_mct_lockup_handler,
};
#endif

static int exynos_mct_starting_cpu(unsigned int cpu)
{
	struct mct_clock_event_device *mevt = per_cpu_ptr(&percpu_mct_tick, cpu);
	struct clock_event_device *evt = &mevt->evt;

	pr_info("%s(cpu:%d) is called\n", __func__, cpu);

	snprintf(mevt->name, sizeof(mevt->name), "mct_comp%d", cpu);

	evt->name = mevt->name;
	evt->cpumask = cpumask_of(cpu);
	evt->set_next_event = exynos_comp_set_next_event;
	evt->set_state_periodic = mct_set_state_periodic;
	evt->set_state_shutdown = mct_set_state_shutdown;
	evt->set_state_oneshot = mct_set_state_shutdown;
	evt->set_state_oneshot_stopped = mct_set_state_shutdown;
	evt->tick_resume = mct_set_state_shutdown;
	evt->features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	evt->rating = 500;	/* use value higher than ARM arch timer */

	if (evt->irq == -1)
		return -EIO;

	irq_force_affinity(evt->irq, cpumask_of(cpu));
	enable_irq(evt->irq);
	clockevents_config_and_register(evt, osc_clk_rate, 0xf, 0x7fffffff);

	return 0;
}

static int exynos_mct_dying_cpu(unsigned int cpu)
{
	struct mct_clock_event_device *mevt = per_cpu_ptr(&percpu_mct_tick, cpu);
	struct clock_event_device *evt = &mevt->evt;
	unsigned int index = mevt->comp_index;

	pr_info("%s(cpu:%d) is called\n", __func__, cpu);

	evt->set_state_shutdown(evt);
	if (evt->irq != -1)
		disable_irq_nosync(evt->irq);

	writel_relaxed(MCT_CSTAT_CLEAR, reg_base + EXYNOS_MCT_INT_CSTAT(index));

	return 0;
}

static void mct_suspend_resume(void *ignore, const char *action,
				int event, bool start)
{
	int cpu;

	if (!start)
		return;

	if (!action || strcmp(action, "timekeeping_freeze"))
		return;

	for_each_possible_cpu(cpu) {
		struct mct_clock_event_device *mevt = per_cpu_ptr(&percpu_mct_tick, cpu);

		mevt->in_s2idle_c2 = true;
	}

	printk("%s: all core clk freeze\n", __func__);

	return;
}

static int exynos_timer_resources(struct device_node *np, void __iomem *base)
{
	int err, cpu;

	struct clk *mct_clk, *tick_clk,  *rtc_clk;
	unsigned long rtc_clk_rate;
	int div;
	int ret;
	u32 gic_reg;

	ret = of_property_read_u32(np, "div", &div);
	if (ret || !div) {
		pr_warn("MCT: fail to get the div value. set div to the default\n");
		div = DEFAULT_CLK_DIV;
	}

	tick_clk = of_clk_get_by_name(np, "fin_pll");
	if (IS_ERR(tick_clk))
		panic("%s: unable to determine tick clock rate\n", __func__);
	osc_clk_rate = clk_get_rate(tick_clk) / div;

	mct_clk = of_clk_get_by_name(np, "mct");
	if (IS_ERR(mct_clk))
		panic("%s: unable to retrieve mct clock instance\n", __func__);
	clk_prepare_enable(mct_clk);

	rtc_clk = of_clk_get_by_name(np, "rtc");
	if (IS_ERR(rtc_clk)) {
		pr_warn("MCT: fail to get rtc clock. set to the default\n");
		rtc_clk_rate = DEFAULT_RTC_CLK_RATE;
	} else {
		rtc_clk_rate = clk_get_rate(rtc_clk);
	}

	reg_base = base;
	if (!reg_base)
		panic("%s: unable to ioremap mct address space\n", __func__);

	ret = of_property_read_u32(np, "gic_base", &gic_reg);
	if (!ret) {
		gic_base = ioremap(gic_reg, SZ_8K);
		if (!gic_base)
			pr_err("MCT: unable to ioremap GIC register base.\n");
	} else {
		pr_warn("MCT: no GIC register base information.\n");
	}

	exynos_mct_set_compensation(osc_clk_rate, rtc_clk_rate);
	exynos_mct_frc_start();

	for_each_possible_cpu(cpu) {
		int mct_irq = mct_irqs[cpu];
		struct mct_clock_event_device *pcpu_mevt = per_cpu_ptr(&percpu_mct_tick, cpu);

		pcpu_mevt->evt.irq = -1;
		pcpu_mevt->comp_index = cpu;

		irq_set_status_flags(mct_irq, IRQ_NOAUTOEN);
		if (request_irq(mct_irq,
				exynos_mct_comp_isr,
				IRQF_TIMER | IRQF_NOBALANCING | IRQF_PERCPU,
				"exynos-mct", pcpu_mevt)) {
			pr_err("exynos-mct: cannot register IRQ (cpu%d)\n", cpu);
			continue;
		}
		pcpu_mevt->evt.irq = mct_irq;
	}

#if IS_ENABLED(CONFIG_EXYNOS_HARDLOCKUP_WATCHDOG)
	atomic_notifier_chain_register(&hardlockup_notifier_list, &exynos_mct_lockup_nb);
#endif

	/* Install hotplug callbacks which configure the timer on this CPU */
	err = cpuhp_setup_state(CPUHP_AP_EXYNOS4_MCT_TIMER_STARTING,
				"clockevents/exynos/mct_timer_v3:starting",
				exynos_mct_starting_cpu,
				exynos_mct_dying_cpu);
	if (err)
		goto out_irq;

	register_trace_suspend_resume(mct_suspend_resume, NULL);

	return 0;

out_irq:
	for_each_possible_cpu(cpu) {
		struct mct_clock_event_device *pcpu_mevt = per_cpu_ptr(&percpu_mct_tick, cpu);

		if (pcpu_mevt->evt.irq != -1) {
			free_irq(pcpu_mevt->evt.irq, pcpu_mevt);
			pcpu_mevt->evt.irq = -1;
		}
	}
	return err;
}

static int mct_init_dt(struct device_node *np)
{
	u32 nr_irqs = 0, i;
	struct of_phandle_args irq;
	int ret;

	/*
	 * Find out the total number of irqs which can be produced by comparators.
	 */
	while (of_irq_parse_one(np, nr_irqs, &irq) == 0)
		nr_irqs++;

	for (i = MCT_COMP0; i < nr_irqs; i++)
		mct_irqs[i] = irq_of_parse_and_map(np, i);

	pr_info("## exynos_timer_resources\n");
	ret = exynos_timer_resources(np, of_iomap(np, 0));
	if (ret)
		return ret;

	pr_info("## exynos_clocksource_init\n");
	ret = exynos_clocksource_init();

	return ret;
}

#ifdef MODULE
static int exynos_mct_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	pr_info("%s\n", __func__);

	return mct_init_dt(np);
}

static const struct of_device_id exynos_mct_match_table[] = {
	{ .compatible = "samsung,exynos-mct-v3" },
	{ }
};
MODULE_DEVICE_TABLE(of, exynos_mct_match_table);

static struct platform_driver exynos_mct_v3_driver = {
	.probe		= exynos_mct_probe,
	.driver		= {
		.name	= "exynos-mct-v3",
		.of_match_table = exynos_mct_match_table,
	},
};
module_platform_driver(exynos_mct_v3_driver);

#else
TIMER_OF_DECLARE(exynos_mct_v3, "samsung,exynos-mct-v3", mct_init_dt);
#endif

MODULE_DESCRIPTION("Exynos Multi Core Timer v3 driver");
MODULE_AUTHOR("Donghoon Yu <hoony.yu@samsung.com>");
MODULE_AUTHOR("Youngmin Nam <youngmin.nam@samsung.com>");
MODULE_LICENSE("GPL v2");
