// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>

#include "pablo-phy-settle-table.h"
#include "is-hw.h"
#include "is-hw-phy.h"
#include "is-hw-api-csi.h"

#define HIWORD(d) ((u16)(((u32)(d)) >> 16))
#define LOWORD(d) ((u16)(u32)(d))
#define PHY_REF_SPEED	(1500)
#define PHY_REF_SPEED_6410	(400)
#define CPHY_REF_SPEED	(500)
#define MAX_LANE 4

u32 is_hw_find_settle(u32 mipi_speed, u32 use_cphy)
{
	u32 align_mipi_speed;
	u32 find_mipi_speed;
	const u32 *settle_table;
	size_t max;
	int s, e, m;

	if (use_cphy) {
		settle_table = is_csi_settle_table_cphy;
		max = sizeof(is_csi_settle_table_cphy) / sizeof(u32);
	} else {
		settle_table = is_csi_settle_table;
		max = sizeof(is_csi_settle_table) / sizeof(u32);
	}
	align_mipi_speed = ALIGN(mipi_speed, 10);

	s = 0;
	e = max - 2;

	if (settle_table[s] < align_mipi_speed)
		return settle_table[s + 1];

	if (settle_table[e] > align_mipi_speed)
		return settle_table[e + 1];

	/* Binary search */
	while (s <= e) {
		m = ALIGN((s + e) / 2, 2);
		find_mipi_speed = settle_table[m];

		if (find_mipi_speed == align_mipi_speed)
			break;
		else if (find_mipi_speed > align_mipi_speed)
			s = m + 2;
		else
			e = m - 2;
	}

	return settle_table[m + 1];
}

static int get_settle_clk_sel(u32 *cfg, u32 mode)
{
	u32 settle_clk_sel = 1;

	if (mode == 0x000D) {
		if (cfg[PPI_SPEED] >= PHY_REF_SPEED)
			settle_clk_sel = 0;
		else
			settle_clk_sel = 1;
	} else {
		if (cfg[PPI_SPEED] >= CPHY_REF_SPEED)
			settle_clk_sel = 0;
		else
			settle_clk_sel = 1;
	}

	return settle_clk_sel;
}

static int get_settle_clk_sel_v6410(u32 *cfg, u32 mode)
{
	u32 settle_clk_sel = 1;

	if (mode == 0x000D) {
		if (cfg[PPI_SPEED] >= PHY_REF_SPEED_6410)
			settle_clk_sel = 0;
		else
			settle_clk_sel = 1;
	} else {
		settle_clk_sel = 0;
	}

	return settle_clk_sel;
}
static int get_skew_delay_sel(u32 *cfg, u32 mode)
{
	u32 skew_delay_sel;

	if (mode != 0x000D)
		return 0; /* NOT USED */

	if (cfg[PPI_SPEED] >= 4000 && cfg[PPI_SPEED] <= 6500)
		skew_delay_sel = 0;
	else if (cfg[PPI_SPEED] >= 3000 && cfg[PPI_SPEED] < 4000)
		skew_delay_sel = 1;
	else if (cfg[PPI_SPEED] >= 2000 && cfg[PPI_SPEED] < 3000)
		skew_delay_sel = 2;
	else if (cfg[PPI_SPEED] < 2000)
		skew_delay_sel = 3;
	else
		skew_delay_sel = 0;

	return skew_delay_sel;
}

static int get_skew_cal(u32 *cfg, u32 mode)
{
	if (is_get_debug_param(IS_DEBUG_PARAM_PHY_TUNE) == PABLO_PHY_TUNE_DPHY)
		return 0;

	if (cfg[PPI_VERSION] >= IS_CSIS_VERSION(5, 4, 0, 0)) {
		return 1; /* In DPHY, skew cal enable regardless of mipi speed */
	} else {
		if (cfg[PPI_SPEED] >= PHY_REF_SPEED)
			return 1;
		else
			return 0;
	}
}

static void reset_analog_logic(void __iomem *regs, void __iomem *regs_lane,
				u32 *cfg, u32 mode)
{
	int i;

	if (mode == 0x000D) {
		writel(0x00000000, regs + 0x0000); /* SC_GNR_CON0, Phy clock enable */

		for (i = 0; i <= cfg[PPI_LANES]; i++)
			writel(0x00000000, regs_lane + (i * 0x100)); /* SD_GNR_CON0 , Phy data enable */
	} else {
		for (i = 0; i <= cfg[PPI_LANES]; i++)
			writel(0x00000000, regs_lane + (i * 0x100)); /* SD_GNR_CON0 , Phy data enable */
	}

	usleep_range(200, 201);
}

static void set_common_setfile(void __iomem *regs, void __iomem *regs_bias, struct phy_setfile *sf)
{
	int i;
	void __iomem *base;
	int debug_csi = is_get_debug_param(IS_DEBUG_PARAM_CSI);
	struct phy_sfr_info *phy_sfr;

	for (i = 0; i < sf->phy_sfr_cnt; i++) {
		phy_sfr = &sf->phy_sfr_info[i];
		base = (phy_sfr->index == IDX_BIA_VAL) ? regs_bias : regs;
		update_bits(base + phy_sfr->addr, phy_sfr->start, phy_sfr->width, phy_sfr->val);

		if (debug_csi)
			info("%s(addr: %04X, val: %08X)\n", __func__, phy_sfr->addr,
				readl(base + phy_sfr->addr));
	}
}

static void set_cal_setfile(void __iomem *regs_cal, struct phy_setfile *sf, u32 *val, u32 lane)
{
	int i, index;
	int debug_csi = is_get_debug_param(IS_DEBUG_PARAM_CSI);
	struct phy_sfr_info *phy_sfr;

	for (i = 0; i < sf->phy_sfr_cnt; i++) {
		phy_sfr = &sf->phy_sfr_info[i];
		if (lane + 1 > phy_sfr->max_lane)
			continue;

		val[IDX_FIX_VAL] = phy_sfr->val;
		index = phy_sfr->index;

		update_bits(regs_cal + phy_sfr->addr, phy_sfr->start, phy_sfr->width, val[index]);

		if (debug_csi)
			info("%s(addr: %04X, val: %08X, lane: %d)\n", __func__, phy_sfr->addr,
				readl(regs_cal + phy_sfr->addr), lane);
	}
}

static void set_lane_setfile(void __iomem *regs_lane, struct phy_setfile *sf, u32 *val, u32 lane)
{
	int i, index;
	int debug_csi = is_get_debug_param(IS_DEBUG_PARAM_CSI);
	struct phy_sfr_info *phy_sfr;

	for (i = 0; i < sf->phy_sfr_cnt; i++) {
		phy_sfr = &sf->phy_sfr_info[i];
		if (lane + 1 > phy_sfr->max_lane)
			continue;

		val[IDX_FIX_VAL] = phy_sfr->val;
		index = phy_sfr->index;

		update_bits(regs_lane + phy_sfr->addr, phy_sfr->start, phy_sfr->width, val[index]);

		if (debug_csi)
			info("%s(addr: %04X, val: %08X, lane: %d)\n", __func__, phy_sfr->addr,
				val[index], lane);
	}
}

static void set_enable_setfile(void __iomem *regs_cmn, struct phy_setfile *sf,
				u32 mode, bool enable)
{
	int debug_csi = is_get_debug_param(IS_DEBUG_PARAM_CSI);
	struct phy_sfr_info *phy_sfr;
	u32 reset_value;

	phy_sfr = &sf->phy_sfr_info[0];

	if (mode == 0x000D)
		reset_value = 0x0000;
	else
		reset_value = 0x9000;


	if (enable)
		update_bits(regs_cmn + phy_sfr->addr, phy_sfr->start, phy_sfr->width, phy_sfr->val);
	else
		update_bits(regs_cmn + phy_sfr->addr, phy_sfr->start, phy_sfr->width, reset_value);

	if (debug_csi)
		info("%s(addr: %04X, val: %08X)\n", __func__, phy_sfr->addr,
			readl(regs_cmn + phy_sfr->addr));

}

static int set_phy_cfg_dcphy(struct phy *phy, int option, unsigned int *cfg,
		struct phy_setfile_table *sf_table, void __iomem *phy_reg)
{
	struct mipi_phy_desc *phy_desc = phy_get_drvdata(phy);
	void __iomem *regs = phy_desc->regs; /* clock */
	void __iomem *regs_lane = phy_desc->regs_lane;
	void __iomem *regs_bias = phy_desc->regs_bias;
	void __iomem *regs_cmn = phy_desc->regs_cmn;
	void __iomem *regs_cal = phy_desc->regs_cal;
	int i, i_eq = -1;
	u32 mode = HIWORD(cfg[PPI_TYPE]);
	u32 lane_ofs = 0x100;
	u32 val[IDX_MAX_VAL];

	val[IDX_STL_VAL] = cfg[PPI_SETTLE];
	if (regs_cmn)
		val[IDX_STL_CLK] = get_settle_clk_sel_v6410(cfg, mode);
	else
		val[IDX_STL_CLK] = get_settle_clk_sel(cfg, mode);

	val[IDX_SKW_DLY] = get_skew_delay_sel(cfg, mode);
	val[IDX_SKW_CAL] = get_skew_cal(cfg, mode);

	if (regs_cmn)
		set_enable_setfile(regs_cmn, &sf_table->sf[PPS_CONFG], mode, false);
	else
		reset_analog_logic(regs, regs_lane, cfg, mode);

	if (!regs_bias)
		regs_bias = phy_reg + phy_desc->bias_offset;

	set_common_setfile(regs, regs_bias, &sf_table->sf[PPS_COMM]);

	/* Need to set till max lane for LPRX setting */
	for (i = 0; i < MAX_LANE; i++)
		set_lane_setfile(regs_lane + (i * lane_ofs), &sf_table->sf[PPS_LANE], val, i);

	if (regs_cal)
		for (i = 0; i < MAX_LANE; i++)
			set_cal_setfile(regs_cal + (i * lane_ofs), &sf_table->sf[PPS_CCAL], val, i);


	/* OPTION: EQ tuning depending on mipi speed */
	for (i = 0; i < sf_table->eq_cnt; i++) {
		if (cfg[PPI_SPEED] >= sf_table->eq_sf[i].mipi_speed)
			i_eq = i;
		else
			break;
	}

	if (i_eq >= 0) {
		for (i = 0; i <= cfg[PPI_LANES]; i++)
			set_lane_setfile(
				regs_lane + (i * lane_ofs), &sf_table->eq_sf[i_eq], val, i);
	}

	if (regs_cmn)
		set_enable_setfile(regs_cmn, &sf_table->sf[PPS_CONFG], mode, true);

	usleep_range(200, 201);

	return 0;
}

int csi_hw_s_phy_set(struct phy *phy, u32 lanes, u32 mipi_speed,
		u32 settle, u32 instance, u32 use_cphy,
		struct phy_setfile_table *sf_table,
		void __iomem *phy_reg, void __iomem *csi_reg)
{
	int ret = 0;
	unsigned int phy_cfg[PPI_MAX];
	u32 ver = csi_hw_get_version(csi_reg);

	/*
	 * [0]: the version of PHY (major << 16 | minor)
	 * [1]: the type of PHY (mode << 16 | type)
	 * [2]: the number of lanes (zero-based)
	 * [3]: the data rate
	 * [4]: the settle value for the data rate
	 */
	phy_cfg[PPI_VERSION] = ver;
	phy_cfg[PPI_TYPE] = use_cphy ? 0x000C << 16 : 0x000D << 16;
	phy_cfg[PPI_LANES] = lanes;
	phy_cfg[PPI_SPEED] = mipi_speed;
	phy_cfg[PPI_SETTLE] = settle ? settle : is_hw_find_settle(mipi_speed, use_cphy);

	/* reset assertion */
	ret = phy_reset(phy);
	if (ret) {
		err("failed to reset assert PHY");
		return ret;
	}

	if (sf_table->sf) {
		set_phy_cfg_dcphy(phy, 0, phy_cfg, sf_table, phy_reg);

		/* reset release */
		ret = phy_configure(phy, NULL);
		if (ret) {
			err("failed to reset release PHY");
			return ret;
		}
	} else {
		union phy_configure_opts opts;

		/* HACK: use phy_configure API instead of phy_set */
		opts.mipi_dphy.clk_miss = phy_cfg[PPI_VERSION]; /* Version */
		opts.mipi_dphy.clk_post = phy_cfg[PPI_TYPE]; /* Type */
		opts.mipi_dphy.lanes = phy_cfg[PPI_LANES]; /* Lanes */
		opts.mipi_dphy.hs_clk_rate = phy_cfg[PPI_SPEED]; /* Mipispeed */
		opts.mipi_dphy.hs_settle = phy_cfg[PPI_SETTLE];  /* Settle */

		/* reset release */
		ret = phy_configure(phy, &opts);
		if (ret) {
			err("failed to set PHY");
			return ret;
		}
	}

	info(" settle=%d, speed(%u%s), lane(%u)\n", settle, mipi_speed, use_cphy ? "Msps" : "Mbps",
		lanes + 1);

	return ret;
}
PST_EXPORT_SYMBOL(csi_hw_s_phy_set);
