/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License Version 2 as publised
 * by the Free Software Foundation.
 *
 * BTS Bus Traffic Shaper device driver operation functions set
 *
 */

#include <linux/io.h>
#include <dt-bindings/soc/samsung/exynos-bts.h>

#include "regs-btss5e9945.h"
#include "bts.h"

/****************************************************************
 *			init ops functions			*
 ****************************************************************/
static void init_trexbts(void __iomem *base)
{
	unsigned int w1;

	if (!base)
		return;

	/* high [27:24] mid [19:16] threshold */
	w1 = __raw_readl(base + SCI_CTRL);
	w1 |= (HIGH_THRESOLD << HIGH_THRESHOLD_SHIFT) | (MID_THRESHOLD << MID_THRESHOLD_SHIFT);
	__raw_writel(w1, base + SCI_CTRL);

	__raw_writel(TH_IMM_R_VAL, base + TH_IMM_R_0);
	__raw_writel(TH_IMM_W_VAL, base + TH_IMM_W_0);

	__raw_writel(TH_HIGH_R_VAL, base + TH_HIGH_R_0);
	__raw_writel(TH_HIGH_W_VAL, base + TH_HIGH_W_0);
}

static void init_qmax(void __iomem *base)
{
	unsigned int threshold = 0;
	unsigned int tmp_reg;

	if (!base)
		return;

	/* Read threshold set */
	threshold = DEFAULT_QMAX_RD_TH;

	if (threshold > MAX_QMAX_TH)
		threshold = MAX_QMAX_TH;

	tmp_reg = __raw_readl(base + QMAX_THRESHOLD_R);
	tmp_reg &= ~(MAX_QMAX_TH << QMAX_THRESHOLD_SHIFT | MAX_QMAX_TH);
	tmp_reg |= (threshold << QMAX_THRESHOLD_SHIFT | threshold);

	__raw_writel(tmp_reg, base + QMAX_THRESHOLD_R);

	/* Write threshold set */
	threshold = DEFAULT_QMAX_WR_TH;

	if (threshold > MAX_QMAX_TH)
		threshold = MAX_QMAX_TH;

	tmp_reg = __raw_readl(base + QMAX_THRESHOLD_W);
	tmp_reg &= ~(MAX_QMAX_TH << QMAX_THRESHOLD_SHIFT | MAX_QMAX_TH);
	tmp_reg |= (threshold << QMAX_THRESHOLD_SHIFT | threshold);

	__raw_writel(tmp_reg, base + QMAX_THRESHOLD_W);
}

static void init_sci(void __iomem *base)
{
	if (!base)
		return;
}

static void init_smc(void __iomem *base)
{
	if (!base)
		return;

	/* SchedulerCtrl0 */
	__raw_writel(0x483F3F10, base);
}

/****************************************************************
 *			ip bts ops functions			*
 ****************************************************************/
static int set_ipbts_qos(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg_r = 0;
	unsigned int tmp_reg_w = 0;
	unsigned int tmp_reg = 0;

	if (!base || !stat)
		return -ENODATA;

	/* xCON[8] = AxQoS override *1:override, 0:bypass */
	if (stat->bypass) {
		tmp_reg_r = __raw_readl(base + RCON);
		tmp_reg_r = tmp_reg_r & ~(0x1 << AXQOS_BYPASS);
		tmp_reg_w = __raw_readl(base + WCON);
		tmp_reg_w = tmp_reg_w & ~(0x1 << AXQOS_BYPASS);

		__raw_writel(tmp_reg_r, base + RCON);
		__raw_writel(tmp_reg_w, base + WCON);

		return 0;
	}

	if (stat->arqos > MAX_QOS)
		stat->arqos = MAX_QOS;

	/* xCON[15:12] = AxQoS value */
	tmp_reg_r = __raw_readl(base + RCON);
	tmp_reg_r = tmp_reg_r & ~((0x1 << AXQOS_BYPASS) | (MAX_QOS << AXQOS_VAL));
	tmp_reg_r = tmp_reg_r | (0x1 << AXQOS_BYPASS) | (stat->arqos << AXQOS_VAL);

	if (stat->awqos > MAX_QOS)
		stat->awqos = MAX_QOS;

	/* xCON[15:12] = AxQoS value */
	tmp_reg_w = __raw_readl(base + WCON);
	tmp_reg_w = tmp_reg_w & ~((0x1 << AXQOS_BYPASS) | (MAX_QOS << AXQOS_VAL));
	tmp_reg_w = tmp_reg_w | (0x1 << AXQOS_BYPASS) | (stat->awqos << AXQOS_VAL);

	__raw_writel(tmp_reg_r, base + RCON);
	__raw_writel(tmp_reg_w, base + WCON);

	/* CONTROL[0] = QoS on/off */
	tmp_reg = __raw_readl(base + CON);
	tmp_reg = tmp_reg & ~(0x1 << AXQOS_ONOFF);
	tmp_reg = tmp_reg | (0x1 << AXQOS_ONOFF);
	__raw_writel(tmp_reg, base + CON);

	return 0;
}

static int get_ipbts_qos(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg_r = 0;
	unsigned int tmp_reg_w = 0;

	if (!base || !stat)
		return -ENODATA;

	tmp_reg_r = __raw_readl(base + RCON);
	tmp_reg_r = (tmp_reg_r & (0x1 << AXQOS_BYPASS)) >> AXQOS_BYPASS;
	tmp_reg_w = __raw_readl(base + WCON);
	tmp_reg_w = (tmp_reg_w & (0x1 << AXQOS_BYPASS)) >> AXQOS_BYPASS;

	if (!tmp_reg_r || !tmp_reg_w)
		stat->bypass = true;
	else
		stat->bypass = false;

	tmp_reg_r = __raw_readl(base + RCON);
	tmp_reg_r = (tmp_reg_r & (MAX_QOS << AXQOS_VAL)) >> AXQOS_VAL;

	tmp_reg_w = __raw_readl(base + WCON);
	tmp_reg_w = (tmp_reg_w & (MAX_QOS << AXQOS_VAL)) >> AXQOS_VAL;

	stat->arqos = tmp_reg_r;
	stat->awqos = tmp_reg_w;

	return 0;
}

static int set_ipbts_mo(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg_r = 0;
	unsigned int tmp_reg_w = 0;

	if (!base || !stat)
		return -ENODATA;

	if (stat->rmo > MAX_MO || !stat->rmo)
		stat->rmo = MAX_MO;

	tmp_reg_r = __raw_readl(base + RBLK_UPPER);
	tmp_reg_r = tmp_reg_r & ~(MAX_MO << BLOCK_UPPER);
	tmp_reg_r = tmp_reg_r | (stat->rmo << BLOCK_UPPER);

	if (stat->wmo > MAX_MO || !stat->wmo)
		stat->wmo = MAX_MO;

	tmp_reg_w = __raw_readl(base + WBLK_UPPER);
	tmp_reg_w = tmp_reg_w & ~(MAX_MO << BLOCK_UPPER);
	tmp_reg_w = tmp_reg_w | (stat->wmo << BLOCK_UPPER);

	__raw_writel(tmp_reg_r, base + RBLK_UPPER);
	__raw_writel(tmp_reg_w, base + WBLK_UPPER);

	return 0;
}

static int get_ipbts_mo(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg_r = 0;
	unsigned int tmp_reg_w = 0;

	if (!base || !stat)
		return -ENODATA;

	tmp_reg_r = __raw_readl(base + RBLK_UPPER);
	tmp_reg_r = (tmp_reg_r & (MAX_MO << BLOCK_UPPER)) >> BLOCK_UPPER;

	tmp_reg_w = __raw_readl(base + WBLK_UPPER);
	tmp_reg_w = (tmp_reg_w & (MAX_MO << BLOCK_UPPER)) >> BLOCK_UPPER;

	stat->rmo = tmp_reg_r;
	stat->wmo = tmp_reg_w;

	return 0;
}

static int set_ipbts_urgent(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;

	if (!base || !stat)
		return -ENODATA;

	if (stat->qurgent_th_r > MAX_QUTH)
		stat->qurgent_th_r = MAX_QUTH;
	if (stat->qurgent_th_w > MAX_QUTH)
		stat->qurgent_th_w = MAX_QUTH;

	if (stat->odd_qurgent_offset) {
		tmp_reg = __raw_readl(base + REGOFF_QU_TIMEOUT_CNT);
		tmp_reg &= ~(MASK_QU_TIMEOUT_CNT_INITIAL_R
				<< SHIFT_QU_TIMEOUT_CNT_INITIAL_R);
		tmp_reg |= (stat->qurgent_th_r
				<< SHIFT_QU_TIMEOUT_CNT_INITIAL_R);
		tmp_reg &= ~(MASK_QU_TIMEOUT_CNT_INITIAL_W
				<< SHIFT_QU_TIMEOUT_CNT_INITIAL_W);
		tmp_reg |= (stat->qurgent_th_w
				<< SHIFT_QU_TIMEOUT_CNT_INITIAL_W);

		__raw_writel(tmp_reg, base + REGOFF_QU_TIMEOUT_CNT);
	} else {
		/* Read QUrgent */
		tmp_reg = __raw_readl(base + TIMEOUT_R0);
		tmp_reg = tmp_reg & ~(MAX_QUTH << TIMEOUT_CNT_VC);
		tmp_reg = tmp_reg | (stat->qurgent_th_r << TIMEOUT_CNT_VC);

		__raw_writel(tmp_reg, base + TIMEOUT_R0);

		/* Write QUrgent */
		tmp_reg = __raw_readl(base + TIMEOUT_W0);
		tmp_reg = tmp_reg & ~(MAX_QUTH << TIMEOUT_CNT_VC);
		tmp_reg = tmp_reg | (stat->qurgent_th_w << TIMEOUT_CNT_VC);

		__raw_writel(tmp_reg, base + TIMEOUT_W0);
	}

	/* On/Off QUrgent */
	tmp_reg = __raw_readl(base + CON);
	tmp_reg = tmp_reg & ~(0x1 << QURGENT_EN);

	if (stat->qurgent_on)
		tmp_reg = tmp_reg | (stat->qurgent_on << QURGENT_EN);

	if (stat->qurgent_ex)
		tmp_reg = tmp_reg | (stat->qurgent_ex << QURGENT_EX);

	__raw_writel(tmp_reg, base + CON);

	return 0;
}

static int get_ipbts_urgent(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;

	if (!base || !stat)
		return -ENODATA;

	if (stat->odd_qurgent_offset) {
		tmp_reg = __raw_readl(base + REGOFF_QU_TIMEOUT_CNT);
		stat->qurgent_th_r =(tmp_reg &
				(MASK_QU_TIMEOUT_CNT_INITIAL_R << SHIFT_QU_TIMEOUT_CNT_INITIAL_R))
				>> SHIFT_QU_TIMEOUT_CNT_INITIAL_R;
		stat->qurgent_th_w = (tmp_reg &
				(MASK_QU_TIMEOUT_CNT_INITIAL_W << SHIFT_QU_TIMEOUT_CNT_INITIAL_W))
				>> SHIFT_QU_TIMEOUT_CNT_INITIAL_W;
	} else {
		tmp_reg = __raw_readl(base + TIMEOUT_R0);
		stat->qurgent_th_r = (tmp_reg & (MAX_QUTH << TIMEOUT_CNT_VC)) >> TIMEOUT_CNT_VC;
		tmp_reg = __raw_readl(base + TIMEOUT_W0);
		stat->qurgent_th_w = (tmp_reg & (MAX_QUTH << TIMEOUT_CNT_VC)) >> TIMEOUT_CNT_VC;
	}

	tmp_reg = __raw_readl(base + CON);
	stat->qurgent_on = (tmp_reg & (0xFF << QURGENT_EN)) >> QURGENT_EN;
	stat->qurgent_ex = (tmp_reg & (0x1 << QURGENT_EX)) >> QURGENT_EX;
	return 0;
}

static int set_ipbts_blocking(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg_r = 0;
	unsigned int tmp_reg_w = 0;

	if (!base || !stat)
		return -ENODATA;

	if (stat->blocking_on) {
		if (stat->qfull_limit_r > MAX_MO)
			stat->qfull_limit_r = MAX_MO;
		if (stat->qfull_limit_w > MAX_MO)
			stat->qfull_limit_w = MAX_MO;

		tmp_reg_r = __raw_readl(base + RBLK_UPPER_FULL);
		tmp_reg_r = tmp_reg_r & ~(MAX_MO << BLOCK_UPPER);
		tmp_reg_r = tmp_reg_r | (stat->qfull_limit_r << BLOCK_UPPER);

		tmp_reg_w = __raw_readl(base + WBLK_UPPER_FULL);
		tmp_reg_w = tmp_reg_w & ~(MAX_MO << BLOCK_UPPER);
		tmp_reg_w = tmp_reg_w | (stat->qfull_limit_w << BLOCK_UPPER);

		__raw_writel(tmp_reg_r, base + RBLK_UPPER_FULL);
		__raw_writel(tmp_reg_w, base + WBLK_UPPER_FULL);

		if (stat->qbusy_limit_r > MAX_MO)
			stat->qbusy_limit_r = MAX_MO;
		if (stat->qbusy_limit_w > MAX_MO)
			stat->qbusy_limit_w = MAX_MO;

		tmp_reg_r = __raw_readl(base + RBLK_UPPER_BUSY);
		tmp_reg_r = tmp_reg_r & ~(MAX_MO << BLOCK_UPPER);
		tmp_reg_r = tmp_reg_r | (stat->qbusy_limit_r << BLOCK_UPPER);

		tmp_reg_w = __raw_readl(base + WBLK_UPPER_BUSY);
		tmp_reg_w = tmp_reg_w & ~(MAX_MO << BLOCK_UPPER);
		tmp_reg_w = tmp_reg_w | (stat->qbusy_limit_w << BLOCK_UPPER);

		__raw_writel(tmp_reg_r, base + RBLK_UPPER_BUSY);
		__raw_writel(tmp_reg_w, base + WBLK_UPPER_BUSY);

		if (stat->qmax0_limit_r > MAX_MO)
			stat->qmax0_limit_r = MAX_MO;
		if (stat->qmax0_limit_w > MAX_MO)
			stat->qmax0_limit_w = MAX_MO;

		if (stat->qmax1_limit_r > MAX_MO)
			stat->qmax1_limit_r = MAX_MO;
		if (stat->qmax1_limit_w > MAX_MO)
			stat->qmax1_limit_w = MAX_MO;

		tmp_reg_r = __raw_readl(base + RBLK_UPPER_MAX);
		tmp_reg_r &= ~((MAX_MO << BLOCK_UPPER1) | (MAX_MO << BLOCK_UPPER0));
		tmp_reg_r |= ((stat->qmax1_limit_r << BLOCK_UPPER1) | (stat->qmax0_limit_r << BLOCK_UPPER0));

		tmp_reg_w = __raw_readl(base + WBLK_UPPER_MAX);
		tmp_reg_w &= ~((MAX_MO << BLOCK_UPPER1) | (MAX_MO << BLOCK_UPPER0));
		tmp_reg_w |= ((stat->qmax1_limit_w << BLOCK_UPPER1) | (stat->qmax0_limit_w << BLOCK_UPPER0));

		__raw_writel(tmp_reg_r, base + RBLK_UPPER_MAX);
		__raw_writel(tmp_reg_w, base + WBLK_UPPER_MAX);

		tmp_reg_r = __raw_readl(base + RCON);
		tmp_reg_r = tmp_reg_r & ~(0x1 << BLOCKING_EN);
		tmp_reg_r = tmp_reg_r | (0x1 << BLOCKING_EN);
		__raw_writel(tmp_reg_r, base + RCON);

		tmp_reg_w = __raw_readl(base + WCON);
		tmp_reg_w = tmp_reg_w & ~(0x1 << BLOCKING_EN);
		tmp_reg_w = tmp_reg_w | (0x1 << BLOCKING_EN);
		__raw_writel(tmp_reg_w, base + WCON);
	} else {
		tmp_reg_r = __raw_readl(base + RCON);
		tmp_reg_r = tmp_reg_r & ~(0x1 << BLOCKING_EN);
		__raw_writel(tmp_reg_r, base + RCON);

		tmp_reg_w = __raw_readl(base + WCON);
		tmp_reg_w = tmp_reg_w & ~(0x1 << BLOCKING_EN);
		__raw_writel(tmp_reg_w, base + WCON);
	}

	return 0;
}

static int get_ipbts_blocking(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;

	tmp_reg = __raw_readl(base + RCON);
	stat->blocking_on = (tmp_reg & (0x1 << BLOCKING_EN)) ? true : false;

	if (!stat->blocking_on)
		return 0;

	tmp_reg = __raw_readl(base + RBLK_UPPER_FULL);
	stat->qfull_limit_r = (tmp_reg & (MAX_MO << BLOCK_UPPER)) >> BLOCK_UPPER;

	tmp_reg = __raw_readl(base + WBLK_UPPER_FULL);
	stat->qfull_limit_w = (tmp_reg & (MAX_MO << BLOCK_UPPER)) >> BLOCK_UPPER;

	tmp_reg = __raw_readl(base + RBLK_UPPER_BUSY);
	stat->qbusy_limit_r = (tmp_reg & (MAX_MO << BLOCK_UPPER)) >> BLOCK_UPPER;

	tmp_reg = __raw_readl(base + WBLK_UPPER_BUSY);
	stat->qbusy_limit_w = (tmp_reg & (MAX_MO << BLOCK_UPPER)) >> BLOCK_UPPER;

	tmp_reg = __raw_readl(base + RBLK_UPPER_MAX);
	stat->qmax0_limit_r = (tmp_reg & (MAX_MO << BLOCK_UPPER0)) >> BLOCK_UPPER0;
	stat->qmax1_limit_r = (tmp_reg & (MAX_MO << BLOCK_UPPER1)) >> BLOCK_UPPER1;

	tmp_reg = __raw_readl(base + WBLK_UPPER_MAX);
	stat->qmax0_limit_w = (tmp_reg & (MAX_MO << BLOCK_UPPER0)) >> BLOCK_UPPER0;
	stat->qmax1_limit_w = (tmp_reg & (MAX_MO << BLOCK_UPPER1)) >> BLOCK_UPPER1;

	return 0;
}

static int set_ipbts(void __iomem *base, struct bts_stat *stat)
{
	int ret = 0;

	if (!base || !stat)
		return -ENODATA;

	if (!stat->stat_on)
		return 0;

	ret = set_ipbts_qos(base, stat);
	if (ret) {
		pr_err("set_ipbts_qos failed! ret=%d\n", ret);
		return ret;
	}
	ret = set_ipbts_mo(base, stat);
	if (ret) {
		pr_err("set_ipbts_mo failed! ret=%d\n", ret);
		return ret;
	}
	ret = set_ipbts_urgent(base, stat);
	if (ret) {
		pr_err("set_ipbts_urgent failed! ret=%d\n", ret);
		return ret;
	}
	ret = set_ipbts_blocking(base, stat);
	if (ret) {
		pr_err("set_ipbts_blocking failed! ret=%d\n", ret);
		return ret;
	}

	return ret;
}

static int get_ipbts(void __iomem *base, struct bts_stat *stat)
{
	int ret = 0;

	if (!base || !stat)
		return -ENODATA;

	ret = get_ipbts_qos(base, stat);
	if (ret) {
		pr_err("get_ipbts_qos failed! ret=%d\n", ret);
		return ret;
	}
	ret = get_ipbts_mo(base, stat);
	if (ret) {
		pr_err("get_ipbts_mo failed! ret=%d\n", ret);
		return ret;
	}
	ret = get_ipbts_urgent(base, stat);
	if (ret) {
		pr_err("get_ipbts_urgent failed! ret=%d\n", ret);
		return ret;
	}
	ret = get_ipbts_blocking(base, stat);
	if (ret) {
		pr_err("get_ipbts_blocking failed! ret=%d\n", ret);
		return ret;
	}

	return 0;
}

/****************************************************************
 *			trex bts ops functions			*
 ****************************************************************/
static int get_trexbts(void __iomem *base, unsigned int *reg)
{
	if (!base || !reg)
		return -ENODATA;

	reg[0] = __raw_readl(base + SCI_CTRL);
	reg[1] = __raw_readl(base + TH_IMM_R_0);
	reg[2] = __raw_readl(base + TH_IMM_W_0);
	reg[3] = __raw_readl(base + TH_HIGH_R_0);
	reg[4] = __raw_readl(base + TH_HIGH_W_0);

	return 0;
}

/****************************************************************
 *			smc bts ops functions			*
 ****************************************************************/
static int get_smcbts(void __iomem *base, unsigned int *reg)
{
	if (!base || !reg)
		return -ENODATA;

	/* SchedulerCtrl0 */
	reg[0] = __raw_readl(base);

	return 0;
}

/****************************************************************
 *			busc bts ops functions			*
 ****************************************************************/
static int get_buscbts(void __iomem *base, unsigned int *reg)
{
	if (!base || !reg)
		return -ENODATA;

	reg[0] = __raw_readl(base + QMAX_THRESHOLD_R);
	reg[1] = __raw_readl(base + QMAX_THRESHOLD_W);

	return 0;
}

/****************************************************************
 *			sci bts ops functions			*
 ****************************************************************/
static int set_scibts_mo(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;

	/* Set HurryLevel3MOR/W on heavy scenario */
	if (!base || !stat)
		return -ENODATA;

	if (!stat->stat_on)
		return 0;

	/* CRPxPort0QoSControl Set */
	tmp_reg = __raw_readl(base + CRP0_P0_CTRL);
	tmp_reg &= ~(MAX_MO_SCI << HURRYLEVEL3MO_SHIFT);
	tmp_reg |= (stat->hurrylevel3mo_0 << HURRYLEVEL3MO_SHIFT);

	__raw_writel(tmp_reg, base + CRP0_P0_CTRL);
	__raw_writel(tmp_reg, base + CRP1_P0_CTRL);
	__raw_writel(tmp_reg, base + CRP2_P0_CTRL);
	__raw_writel(tmp_reg, base + CRP3_P0_CTRL);

	/* CRPxPort1QoSControl Set */
	tmp_reg = __raw_readl(base + CRP0_P1_CTRL);
	tmp_reg &= ~(MAX_MO_SCI << HURRYLEVEL3MO_SHIFT);
	tmp_reg |= (stat->hurrylevel3mo_1 << HURRYLEVEL3MO_SHIFT);

	__raw_writel(tmp_reg, base + CRP0_P1_CTRL);
	__raw_writel(tmp_reg, base + CRP1_P1_CTRL);
	__raw_writel(tmp_reg, base + CRP2_P1_CTRL);
	__raw_writel(tmp_reg, base + CRP3_P1_CTRL);

	return 0;
}

static int get_scibts_mo(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;

	/* Set HurryLevel3MOR/W on heavy scenario */
	if (!base || !stat)
		return -ENODATA;

	/* CRPxPort0QoSControl Set */
	tmp_reg = __raw_readl(base + CRP0_P0_CTRL);
	stat->hurrylevel3mo_0 = tmp_reg;

	/* CRPxPort1QoSControl Set */
	tmp_reg = __raw_readl(base + CRP0_P1_CTRL);
	stat->hurrylevel3mo_1 = tmp_reg;

	return 0;
}

static int set_scibts(void __iomem *base, struct bts_stat *stat)
{
	int ret = 0;

	if (!base || !stat)
		return -ENODATA;

	if (!stat->stat_on)
		return 0;

	ret = set_scibts_mo(base, stat);
	if (ret) {
		pr_err("set_scibts_mo failed! ret=%d\n", ret);
		return ret;
	}

	return ret;
}

static int get_scibts(void __iomem *base, struct bts_stat *stat)
{
	int ret = 0;

	if (!base || !stat)
		return -ENODATA;

	ret = get_scibts_mo(base, stat);
	if (ret) {
		pr_err("get_scibts_mo failed! ret=%d\n", ret);
		return ret;
	}

	return 0;
}

static int set_scicpubts_mo(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;

	if (!base || !stat)
		return -ENODATA;

	if (stat->rmo > MAX_MO_SCI || !stat->rmo)
		stat->rmo = MAX_MO_SCI;

	if (stat->wmo > MAX_MO_SCI || !stat->wmo)
		stat->wmo = MAX_MO_SCI;

	/*
	 * CRP_CTL3_X (0~3)
	 * MaxOutstandingPort0Rd	[7:0]	(cpu use 0/1/2/3)
	 * MaxOutstandingPort0Wr	[15:8]	(cpu use 0/1/2/3)
	 * MaxOutstandingPort1Rd	[23:16]	(gpu use 0/1/2/3)
	 * MaxOutstandingPort1Wr	[31:24]	(gpu use 0/1/2/3)
	 */
	tmp_reg = __raw_readl(base);

	tmp_reg &= ~(MAX_MO_SCI << RMO_PORT_0 | MAX_MO_SCI << WMO_PORT_0);
	tmp_reg = tmp_reg | (stat->rmo << RMO_PORT_0 | stat->wmo << WMO_PORT_0);

	__raw_writel(tmp_reg, base);

	return 0;
}

static int get_scicpubts_mo(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;
	unsigned int tmp_reg_r = 0;
	unsigned int tmp_reg_w = 0;

	if (!base || !stat)
		return -ENODATA;

	tmp_reg = __raw_readl(base);

	tmp_reg_r = (tmp_reg & (MAX_MO_SCI << RMO_PORT_0)) >> RMO_PORT_0;
	tmp_reg_w = (tmp_reg & (MAX_MO_SCI << WMO_PORT_0)) >> WMO_PORT_0;

	stat->rmo = tmp_reg_r;
	stat->wmo = tmp_reg_w;

	return 0;
}

static int set_scigpubts_mo(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;

	if (!base || !stat)
		return -ENODATA;

	if (stat->rmo > MAX_MO_SCI || !stat->rmo)
		stat->rmo = MAX_MO_SCI;

	if (stat->wmo > MAX_MO_SCI || !stat->wmo)
		stat->wmo = MAX_MO_SCI;

	/*
	 * CRP_CTL3_X (Olympus 0~3)
	 * MaxOutstandingPort0Rd	[7:0]	(Olympus cpu use 0/3, not use 1/2)
	 * MaxOutstandingPort0Wr	[15:8]	(Olympus cpu use 0/3, not use 1/2)
	 * MaxOutstandingPort1Rd	[23:16]	(Olympus gpu use 0/1/2/3)
	 * MaxOutstandingPort1Wr	[31:24]	(Olympus gpu use 0/1/2/3)
	 */
	tmp_reg = __raw_readl(base);

	tmp_reg &= ~(MAX_MO_SCI << RMO_PORT_1 | MAX_MO_SCI << WMO_PORT_1);
	tmp_reg = tmp_reg | (stat->rmo << RMO_PORT_1 | stat->wmo << WMO_PORT_1);

	__raw_writel(tmp_reg, base);

	return 0;
}

static int get_scigpubts_mo(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;
	unsigned int tmp_reg_r = 0;
	unsigned int tmp_reg_w = 0;

	if (!base || !stat)
		return -ENODATA;

	tmp_reg = __raw_readl(base);

	tmp_reg_r = (tmp_reg & (MAX_MO_SCI << RMO_PORT_1)) >> RMO_PORT_1;
	tmp_reg_w = (tmp_reg & (MAX_MO_SCI << WMO_PORT_1)) >> WMO_PORT_1;

	stat->rmo = tmp_reg_r;
	stat->wmo = tmp_reg_w;

	return 0;
}

static int set_scigpubts_qos(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;
	void __iomem *qos_base = NULL;

	if (!stat)
		return -ENODATA;

	qos_base = stat->qos_va_base;

	if (!qos_base)
		return 0;

	if (stat->arqos > SCIMAX_QOS)
		stat->arqos = SCIMAX_QOS;

	if (stat->awqos > SCIMAX_QOS)
		stat->awqos = SCIMAX_QOS;

	/* AxQoS value */
	tmp_reg = __raw_readl(qos_base);
	tmp_reg = tmp_reg & ~((SCIMAX_QOS << SCIQOS_R)| (SCIMAX_QOS << SCIQOS_W));
	tmp_reg = tmp_reg | (stat->arqos << SCIQOS_R) | (stat->awqos << SCIQOS_W);

	__raw_writel(tmp_reg, qos_base);

	/* QoS on */
	if (stat->arqos || stat->awqos) { /* on */
		tmp_reg = __raw_readl(qos_base + CORE_QOS_EN);
		tmp_reg = tmp_reg | (0x1 << SCIQOS_EN);
		__raw_writel(tmp_reg, qos_base + CORE_QOS_EN);
	} else {
	/* QoS off */
		tmp_reg = __raw_readl(qos_base + CORE_QOS_EN);
		tmp_reg = tmp_reg & ~(0x1 << SCIQOS_EN);
		__raw_writel(tmp_reg, qos_base + CORE_QOS_EN);
	}

	return 0;
}

static int get_scigpubts_qos(void __iomem *base, struct bts_stat *stat)
{
	unsigned int tmp_reg = 0;
	unsigned int tmp_reg_r = 0;
	unsigned int tmp_reg_w = 0;
	void __iomem *qos_base = NULL;

	if (!stat)
		return -ENODATA;

	qos_base = stat->qos_va_base;

	if (!qos_base)
		return 0;

	tmp_reg = __raw_readl(qos_base + CORE_QOS_EN);
	tmp_reg = (tmp_reg & (0x1 << SCIQOS_EN)) >> SCIQOS_EN;

	if (tmp_reg)
		stat->bypass = false;
	else
		stat->bypass = true;

	tmp_reg_w = __raw_readl(qos_base);
	tmp_reg_w = (tmp_reg_w & (SCIMAX_QOS << SCIQOS_W)) >> SCIQOS_W;

	tmp_reg_r = __raw_readl(qos_base);
	tmp_reg_r = (tmp_reg_r & (SCIMAX_QOS << SCIQOS_R)) >> SCIQOS_R;

	stat->arqos = tmp_reg_r;
	stat->awqos = tmp_reg_w;

	return 0;
}

static int set_scigpubts(void __iomem *base, struct bts_stat *stat)
{
	int ret = 0;

	if (!base || !stat)
		return -ENODATA;

	if (!stat->stat_on)
		return 0;

	ret = set_scigpubts_qos(base, stat);
	if (ret) {
		pr_err("%s[%d] failed! ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = set_scigpubts_mo(base, stat);
	if (ret) {
		pr_err("%s[%d] failed! ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	return ret;
}

static int get_scigpubts(void __iomem *base, struct bts_stat *stat)
{
	int ret = 0;

	if (!base || !stat)
		return -ENODATA;

	ret = get_scigpubts_qos(base, stat);
	if (ret) {
		pr_err("%s[%d] failed! ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	ret = get_scigpubts_mo(base, stat);
	if (ret) {
		pr_err("%s[%d] failed! ret=%d\n", __func__, __LINE__, ret);
		return ret;
	}

	return 0;
}

/****************************************************************
 *			register ops functions			*
 ****************************************************************/
int register_btsops(struct bts_info *info)
{
	unsigned int type = info->type;

	info->ops = kzalloc(sizeof(struct bts_ops), GFP_KERNEL);
	if (info->ops == NULL)
		return -ENOMEM;

	switch (type) {
	case IP_BTS:
		info->ops->init_bts = NULL;
		info->ops->set_bts = set_ipbts;
		info->ops->get_bts = get_ipbts;
		info->ops->set_qos = set_ipbts_qos;
		info->ops->get_qos = get_ipbts_qos;
		info->ops->set_mo = set_ipbts_mo;
		info->ops->get_mo = get_ipbts_mo;
		info->ops->set_urgent = set_ipbts_urgent;
		info->ops->get_urgent = get_ipbts_urgent;
		info->ops->set_blocking = set_ipbts_blocking;
		info->ops->get_blocking = get_ipbts_blocking;
		break;
	case TREX_BTS:
		info->ops->init_bts = init_trexbts;
		info->ops->set_bts = NULL;
		info->ops->get_bts = NULL;
		info->ops->set_qos = NULL;
		info->ops->get_qos = NULL;
		info->ops->set_mo = NULL;
		info->ops->get_mo = NULL;
		info->ops->set_urgent = NULL;
		info->ops->get_urgent = NULL;
		info->ops->set_blocking = NULL;
		info->ops->get_blocking = NULL;
		info->ops->get_trexbts = get_trexbts;
		break;
	case BUSC_BTS:
		info->ops->init_bts = init_qmax;
		info->ops->set_bts = NULL;
		info->ops->get_bts = NULL;
		info->ops->set_qos = NULL;
		info->ops->get_qos = NULL;
		info->ops->set_mo = NULL;
		info->ops->get_mo = NULL;
		info->ops->set_urgent = NULL;
		info->ops->get_urgent = NULL;
		info->ops->set_blocking = NULL;
		info->ops->get_blocking = NULL;
		info->ops->get_buscbts = get_buscbts;
		break;
	case SCI_BTS:
		info->ops->init_bts = init_sci;
		info->ops->set_bts = set_scibts;
		info->ops->get_bts = get_scibts;
		info->ops->set_qos = NULL;
		info->ops->get_qos = NULL;
		info->ops->set_mo = set_scibts_mo;
		info->ops->get_mo = get_scibts_mo;
		info->ops->set_urgent = NULL;
		info->ops->get_urgent = NULL;
		info->ops->set_blocking = NULL;
		info->ops->get_blocking = NULL;
		break;
	case SCI_CPU_BTS:
		info->ops->init_bts = NULL;
		info->ops->set_bts = set_scicpubts_mo;
		info->ops->get_bts = get_scicpubts_mo;
		info->ops->set_qos = NULL;
		info->ops->get_qos = NULL;
		info->ops->set_mo = set_scicpubts_mo;
		info->ops->get_mo = get_scicpubts_mo;
		info->ops->set_urgent = NULL;
		info->ops->get_urgent = NULL;
		info->ops->set_blocking = NULL;
		info->ops->get_blocking = NULL;
		break;
	case SCI_GPU_BTS:
		info->ops->init_bts = NULL;
		info->ops->set_bts = set_scigpubts;
		info->ops->get_bts = get_scigpubts;
		info->ops->set_qos = set_scigpubts_qos;
		info->ops->get_qos = get_scigpubts_qos;
		info->ops->set_mo = set_scigpubts_mo;
		info->ops->get_mo = get_scigpubts_mo;
		info->ops->set_urgent = NULL;
		info->ops->get_urgent = NULL;
		info->ops->set_blocking = NULL;
		info->ops->get_blocking = NULL;
		break;
	case SMC_BTS:
		info->ops->init_bts = init_smc;
		info->ops->set_bts = NULL;
		info->ops->get_bts = NULL;
		info->ops->set_qos = NULL;
		info->ops->get_qos = NULL;
		info->ops->set_mo = NULL;
		info->ops->get_mo = NULL;
		info->ops->set_urgent = NULL;
		info->ops->get_urgent = NULL;
		info->ops->set_blocking = NULL;
		info->ops->get_blocking = NULL;
		info->ops->get_smcbts = get_smcbts;
		break;
	case DREX_BTS:
	case INTERNAL_BTS:
		info->ops->init_bts = NULL;
		info->ops->set_bts = NULL;
		info->ops->get_bts = NULL;
		info->ops->set_qos = NULL;
		info->ops->get_qos = NULL;
		info->ops->set_mo = NULL;
		info->ops->get_mo = NULL;
		info->ops->set_urgent = NULL;
		info->ops->get_urgent = NULL;
		info->ops->set_blocking = NULL;
		info->ops->get_blocking = NULL;
		break;
	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(register_btsops);

MODULE_LICENSE("GPL");
