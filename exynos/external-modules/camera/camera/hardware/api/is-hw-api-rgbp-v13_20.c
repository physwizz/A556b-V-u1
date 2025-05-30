// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * rgbp HW control APIs
 *
 * Copyright (C) 2024 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include "is-hw-api-rgbp-v2_0.h"
#include "is-hw-common-dma.h"
#include "is-hw.h"
#include "is-hw-control.h"
#include "sfr/is-sfr-rgbp-v13_20.h"
#include "pmio.h"

#define HBLANK_CYCLE			0x2D
#define VBLANK_CYCLE			0xA
#define RGBP_LUT_REG_CNT		1650

#define RGBP_SET_F(base, R, F, val)		PMIO_SET_F(base, R, F, val)
#define RGBP_SET_R(base, R, val)		PMIO_SET_R(base, R, val)
#define RGBP_SET_V(base, reg_val, F, val)	PMIO_SET_V(base, reg_val, F, val)

#define RGBP_GET_F(base, R, F)			PMIO_GET_F(base, R, F)
#define RGBP_GET_R(base, R)			PMIO_GET_R(base, R)

static inline void *get_base(struct is_hw_ip *hw_ip) {
	if (IS_ENABLED(RGBP_USE_MMIO))
		return hw_ip->regs[REG_SETA];
	return hw_ip->pmio;
}

const struct rgbp_v_coef rgbp_v_coef_4tap[7] = {
	/* x8/8 */
	{
		{   0,  -60, -100, -124, -132, -132, -124, -108,  -92},
		{2048, 2032, 1980, 1892, 1772, 1632, 1468, 1296, 1116},
		{   0,   80,  180,  300,  440,  592,  760,  936, 1116},
		{   0,   -4,  -12,  -20,  -32,  -44,  -56,  -76,  -92},
	},
	/* x7/8 */
	{
		{ 128,   68,   12,  -28,  -56,  -72,  -80,  -80,  -76},
		{1792, 1784, 1748, 1684, 1596, 1492, 1372, 1240, 1100},
		{ 128,  220,  316,  428,  552,  680,  816,  960, 1100},
		{   0,  -24,  -28,  -36,  -44,  -52,  -60,  -72,  -76},
	},
	/* x6/8 */
	{
		{ 244,  184,  124,   76,   36,    8,  -12,  -28,  -36},
		{1560, 1560, 1532, 1484, 1424, 1348, 1260, 1164, 1060},
		{ 244,  332,  424,  520,  624,  732,  840,  952, 1060},
		{   0,  -28,  -32,  -32,  -36,  -40,  -40,  -40,  -36},
	},
	/* x5/8 */
	{
		{ 340,  284,  224,  172,  128,   92,   64,   36,   20},
		{1364, 1364, 1344, 1312, 1268, 1216, 1152, 1084, 1004},
		{ 344,  420,  496,  580,  664,  748,  836,  924, 1004},
		{   0,  -20,  -16,  -16,  -12,   -8,   -4,    4,   20},
	},
	/* x4/8 */
	{
		{ 416,  356,  304,  252,  208,  168,  132,  104,  80},
		{1216, 1208, 1192, 1172, 1140, 1100, 1056, 1004, 944},
		{ 416,  480,  544,  612,  680,  752,  820,  884, 944},
		{   0,    4,    8,   12,   20,   28,   40,   56,  80},
	},
	/* x3/8 */
	{
		{ 472,  412,  360,  312,  268,  228,  192,  160,  132},
		{1104, 1092, 1080, 1064, 1040, 1012,  976,  936,  892},
		{ 472,  516,  572,  628,  684,  740,  796,  844,  892},
		{   0,   28,   36,   44,   56,   68,   84,  108,  132},
	},
	/* x2/8 */
	{
		{ 508,  444,  400,  352,  312,  272,  236,  200,  172},
		{1032, 1008, 1000,  988,  968,  948,  920,  888,  852},
		{ 508,  540,  588,  636,  684,  728,  772,  816,  852},
		{   0,   56,   60,   72,   84,  100,  120,  144,  172},
	}
};

const struct rgbp_h_coef rgbp_h_coef_8tap[7] = {
	/* x8/8 */
	{
		{   0,   -8,  -16,  -20,  -24,  -24,  -24,  -24,  -20},
		{   0,   32,   56,   80,   92,  100,  104,  100,   92},
		{   0, -100, -184, -248, -292, -320, -332, -328, -312},
		{2048, 2036, 1996, 1928, 1832, 1716, 1580, 1428, 1264},
		{   0,  120,  256,  404,  568,  740,  912, 1092, 1264},
		{   0,  -36,  -76, -120, -164, -212, -252, -284, -312},
		{   0,    8,   20,   32,   48,   60,   76,   84,   92},
		{   0,   -4,   -4,   -8,  -12,  -12,  -16,  -20,  -20},
	},
	/* x7/8 */
	{
		{  48,   26,   28,   20,   12,    8,    4,    0,   -4},
		{-128,  -96,  -64,  -36,  -12,    8,   28,   40,   52},
		{ 224,  116,   24,  -56, -120, -172, -212, -240, -260},
		{1776, 1780, 1752, 1704, 1640, 1560, 1460, 1352, 1236},
		{ 208,  328,  448,  576,  708,  844,  976, 1108, 1236},
		{-128, -156, -184, -208, -232, -252, -264, -264, -260},
		{  48,   52,   56,   60,   64,   64,   64,   60,   52},
		{   0,  -12,  -12,  -12,  -12,  -12,   -8,   -8,   -4},
	},
	/* x6/8 */
	{
		{  32,   36,   32,   32,   32,   28,   28,   20,   20},
		{-176, -160, -144, -128, -108,  -88,  -72,  -52,  -36},
		{ 400,  308,  228,  152,   80,   20,  -36,  -80, -120},
		{1536, 1528, 1508, 1476, 1432, 1376, 1316, 1240, 1160},
		{ 400,  492,  588,  684,  784,  884,  980, 1072, 1160},
		{-176, -188, -196, -196, -192, -188, -172, -148, -120},
		{  32,   32,   28,   20,   12,    4,   -8,  -20,  -36},
		{   0,    0,    4,    8,    8,   12,   12,   16,   20},
	},
	/* x5/8 */
	{
		{  12,  -12,  -4,     0,    4,    8,    8,   12,   12},
		{-124, -128, -132, -128, -124, -120, -112, -100,  -92},
		{ 520,  452,  388,  324,  264,  208,  152,  104,   60},
		{1280, 1276, 1260, 1244, 1216, 1184, 1144, 1096, 1044},
		{ 520,  588,  660,  728,  796,  864,  928,  988, 1044},
		{-124, -116, -104,  -88,  -68,  -44,  -12,   20,   60},
		{ -12,  -24,  -32,  -44,  -52,  -64,  -72,  -84,  -92},
		{   0,   12,   12,   12,   12,   12,   12,   12,   12},
	},
	/* x4/8 */
	{
		{ -44,  -40,  -36,  -32,  -28,  -24,  -20,  -20,  -16},
		{   0,  -16,  -28,  -40,  -48,  -56,  -60,  -64,  -68},
		{ 560,  516,  468,  424,  380,  340,  296,  256,  220},
		{1020, 1016, 1012, 1000,  984,  964,  944,  916,  888},
		{ 560,  604,  652,  696,  740,  780,  816,  856,  888},
		{   0,   20,   40,   64,   88,  116,  148,  184,  220},
		{ -48,  -52,  -56,  -60,  -64,  -64,  -68,  -68,  -68},
		{   0,    0,   -4,   -4,   -4,   -8,   -8,  -12,  -16},
	},
	/* x3/8 */
	{
		{ -20,  -20,  -20,  -20,  -20,  -20,  -20,  -20,  -20},
		{ 124,  108,   92,   76,   64,   48,   40,   28,   20},
		{ 532,  504,  476,  448,  420,  392,  364,  336,  312},
		{ 780,  780,  776,  772,  764,  756,  740,  728,  712},
		{ 532,  556,  584,  608,  632,  652,  676,  696,  712},
		{ 124,  148,  164,  188,  212,  236,  260,  284,  312},
		{ -24,  -16,  -12,   -8,   -8,    0,    4,   12,   20},
		{   0,  -12,  -12,  -16,  -16,  -16,  -16,  -16,  -20},
	},
	/* x2/8 */
	{
		{  40,   36,   28,   24,   20,   16,   16,   12,    8},
		{ 208,  192,  180,  164,  152,  140,  124,  116,  104},
		{ 472,  456,  440,  424,  408,  392,  376,  356,  340},
		{ 608,  608,  604,  600,  596,  592,  584,  580,  572},
		{ 472,  488,  500,  516,  528,  540,  552,  560,  572},
		{ 208,  224,  240,  256,  272,  288,  308,  324,  340},
		{  40,   44,   52,   60,   68,   76,   84,   92,  104},
		{   0,    0,    4,    4,    4,    4,    4,    8,    8},
	}
};

/*
 * pattern
 * smiadtp_test_pattern_mode - select the pattern:
 * 0 - no pattern (default)
 * 1 - solid color
 * 2 - 100% color bars
 * 3 - "fade to grey" over color bars
 * 4 - PN9
 * 5...255 - reserved
 * 256 - Macbeth color chart
 * 257 - PN12
 * 258...511 - reserved
 */
void rgbp_hw_s_dtp(void *base, u32 set_id, u32 pattern)
{
	RGBP_SET_R(base, RGBP_R_BYR_DTP_TEST_PATTERN_MODE, pattern);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_dtp);

static void rgbp_hw_s_path(void *base, u32 set_id, struct is_rgbp_config *config, struct rgbp_param_set *param_set)
{
	struct is_rgbp_chain_set chain_set;

	/* TODO : set path from DDK */
	chain_set.mux_dtp_sel = param_set->dma_input.cmd ? 0x1 : 0x0;
	/* mux_postgamma_sel should be set to 1 when enabling rdma_rgb */
	chain_set.mux_postgamma_sel = param_set->dma_input.cmd ? 0x1 : 0x0;
	chain_set.mux_wdma_rep_sel = 0x1;
	chain_set.demux_dmsc_en = 0x3;
	chain_set.demux_yuvsc_en = 0x3;
	chain_set.satflg_en = 0x0;

	if (param_set->dma_input.cmd)
		RGBP_SET_F(base, RGBP_R_IP_USE_OTF_PATH, RGBP_F_IP_USE_OTF_IN_FOR_PATH_0, 0);

	RGBP_SET_F(base, RGBP_R_CHAIN_MUX_SELECT, RGBP_F_MUX_DTP_SELECT, chain_set.mux_dtp_sel);
	RGBP_SET_F(base, RGBP_R_CHAIN_MUX_SELECT, RGBP_F_MUX_POSTGAMMA_SELECT, chain_set.mux_postgamma_sel);
	RGBP_SET_F(base, RGBP_R_CHAIN_MUX_SELECT, RGBP_F_MUX_WDMA_REP_SELECT, chain_set.mux_wdma_rep_sel);
	RGBP_SET_F(base, RGBP_R_CHAIN_DEMUX_ENABLE, RGBP_F_DEMUX_DMSC_ENABLE, chain_set.demux_dmsc_en);
	RGBP_SET_F(base, RGBP_R_CHAIN_DEMUX_ENABLE, RGBP_F_DEMUX_YUVSC_ENABLE, chain_set.demux_yuvsc_en);
	/* The Satflag function is not supported. */
	RGBP_SET_F(base, RGBP_R_CHAIN_SATFLAG_ENABLE, RGBP_F_CHAIN_SATFLAG_ENABLE, 0);
}

unsigned int rgbp_hw_is_occurred0(unsigned int status, enum rgbp_event_type type)
{
	u32 mask;

	switch (type) {
	case INTR0_FRAME_START_INT:
		mask = 1 << INTR0_RGBP_FRAME_START_INT;
		break;
	case INTR0_FRAME_END_INT:
		mask = 1 << INTR0_RGBP_FRAME_END_INT;
		break;
	case INTR0_CMDQ_HOLD_INT:
		mask = 1 << INTR0_RGBP_CMDQ_HOLD_INT;
		break;
	case INTR0_SETTING_DONE_INT:
		mask = 1 << INTR0_RGBP_SETTING_DONE_INT;
		break;
	case INTR0_C_LOADER_END_INT:
		mask = 1 << INTR0_RGBP_C_LOADER_END_INT;
		break;
	case INTR0_COREX_END_INT_0:
		mask = 1 << INTR0_RGBP_COREX_END_INT_0;
		break;
	case INTR0_COREX_END_INT_1:
		mask = 1 << INTR0_RGBP_COREX_END_INT_1;
		break;
	case INTR0_ROW_COL_INT:
		mask = 1 << INTR0_RGBP_ROW_COL_INT;
		break;
	case INTR0_TRANS_STOP_DONE_INT:
		mask = 1 << INTR0_RGBP_TRANS_STOP_DONE_INT;
		break;
	case INTR0_ERR0:
		mask = INT0_ERR_MASK;
		break;
	default:
		return 0;
	}

	return status & mask;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_is_occurred0);

unsigned int rgbp_hw_is_occurred1(unsigned int status, enum rgbp_event_type1 type)
{
	u32 mask;

	switch (type) {
	case INTR1_VOTFLOSTFLUSH_INT:
		mask = 1 << INTR1_RGBP_VOTFLOSTFLUSH;
		break;
	case INTR1_VOTF0RDMALOSTCONNECTION_INT:
		mask = 1 << INTR1_RGBP_VOTF0LOSTCONNECTION;
		break;
	case INTR1_VOTF0RDMALOSTFLUSH_INT:
		mask = 1 << INTR1_RGBP_VOTF0LOSTFLUSH;
		break;
	case INTR1_VOTF1WDMALOSTFLUSH_INT:
		mask = 1 << INTR1_RGBP_VOTF1LOSTFLUSH;
		break;
	case INTR1_ERR1:
		mask = INT1_ERR_MASK;
		break;
	default:
		return 0;
	}

	return status & mask;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_is_occurred1);

int rgbp_hw_s_reset(void *base)
{
	u32 reset_count = 0;

	RGBP_SET_R(base, RGBP_R_SW_RESET, 0x1);

	while (RGBP_GET_R(base, RGBP_R_SW_RESET)) {
		if (reset_count > RGBP_TRY_COUNT)
			return reset_count;
		reset_count++;
	}

	return 0;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_reset);

void rgbp_hw_s_init(void *base)
{
	RGBP_SET_F(base, RGBP_R_CMDQ_VHD_CONTROL, RGBP_F_CMDQ_VHD_STALL_ON_QSTOP_ENABLE, 1);
	RGBP_SET_F(base, RGBP_R_CMDQ_VHD_CONTROL, RGBP_F_CMDQ_VHD_VBLANK_QRUN_ENABLE, 1);
	RGBP_SET_F(base, RGBP_R_DEBUG_CLOCK_ENABLE, RGBP_F_DEBUG_CLOCK_ENABLE, 0);

	RGBP_SET_R(base, RGBP_R_C_LOADER_ENABLE, 1);
	RGBP_SET_R(base, RGBP_R_STAT_RDMACL_EN, 1);

	/* Interrupt group enable for one frame */
	RGBP_SET_F(base, RGBP_R_YUV_COUTFIFO_CONFIG, RGBP_F_YUV_COUTFIFO_VVALID_RISE_AT_FIRST_DATA_EN, 1);
	RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_L, RGBP_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE, RGBP_INT_GRP_EN_MASK);

	/* 1: DMA preloading, 2: COREX, 3: APB Direct */
	RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_M, RGBP_F_CMDQ_QUE_CMD_SETTING_MODE, 3);
	RGBP_SET_R(base, RGBP_R_CMDQ_ENABLE, 1);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_init);

void rgbp_hw_s_clock(void *base, bool on)
{
	dbg_hw(5, "[RGBP] clock %s", on ? "on" : "off");
	RGBP_SET_F(base, RGBP_R_IP_PROCESSING, RGBP_F_IP_PROCESSING, on);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_clock);

int rgbp_hw_wait_idle(void *base)
{
	int ret = SET_SUCCESS;
	u32 idle;
	u32 int0_all;
	u32 int1_all;
	u32 try_cnt = 0;

	idle = RGBP_GET_F(base, RGBP_R_IDLENESS_STATUS, RGBP_F_IDLENESS_STATUS);
	int0_all = RGBP_GET_R(base, RGBP_R_INT_REQ_INT0_STATUS);
	int1_all = RGBP_GET_R(base, RGBP_R_INT_REQ_INT1_STATUS);

	while (!idle) {
		idle = RGBP_GET_F(base, RGBP_R_IDLENESS_STATUS, RGBP_F_IDLENESS_STATUS);

		try_cnt++;
		if (try_cnt >= RGBP_TRY_COUNT) {
			err_hw("[RGBP] timeout waiting idle - disable fail");
			rgbp_hw_dump(base, HW_DUMP_CR);
			ret = -ETIME;
			break;
		}

		usleep_range(3, 4);
	};

	int0_all = RGBP_GET_R(base, RGBP_R_INT_REQ_INT0_STATUS);
	int1_all = RGBP_GET_R(base, RGBP_R_INT_REQ_INT1_STATUS);

	return ret;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_wait_idle);

static const struct is_reg rgbp_dbg_cr[] = {
	/* The order of DBG_CR should match with the DBG_CR parser. */
	/* Interrupt History */
	{0x0900, "INT_HIST_CURINT0"},
	{0x090c, "INT_HIST_CURINT1"},
	/* Core Status */
	{0x0f08, "QCH_STATUS"},
	{0x0f0c, "IDLENESS_STATUS"},
	{0x0f40, "IP_BUSY_MONITOR_0"},
	{0x0f44, "IP_BUSY_MONITOR_1"},
	{0x0f48, "IP_BUSY_MONITOR_2"},
	{0x0f4c, "IP_BUSY_MONITOR_3"},
	{0x0f60, "IP_STALL_OUT_STATUS_0"},
	{0x0f64, "IP_STALL_OUT_STATUS_1"},
	{0x0f68, "IP_STALL_OUT_STATUS_2"},
	{0x0f6c, "IP_STALL_OUT_STATUS_3"},
	/* Chain Size */
	{0x0200, "CHAIN_SRC_IMG_SIZE"},
	{0x0204, "CHAIN_DST_IMG_SIZE"},
	/* Chain Path */
	{0x0080, "IP_USE_OTF_PATH"},
	{0x0210, "CHAIN_MUX_SELECT"},
	{0x0214, "CHAIN_DEMUX_ENABLE"},
	{0x0218, "HARDWARE"},
	/* CMDQ Frame Counter */
	{0x04a8, "CMDQ_FRAME_COUNTER"},
	/* CINFIFO 0 Status */
	{0x1000, "BYR_CINFIFO_ENABLE"},
	{0x1014, "BYR_CINFIFO_STATUS"},
	{0x1018, "BYR_CINFIFO_INPUT_CNT"},
	{0x101c, "BYR_CINFIFO_STALL_CNT"},
	{0x1020, "BYR_CINFIFO_FIFO_FULLNESS"},
	/* COUTFIFO 0 Status */
	{0x1200, "YUV_COUTFIFO_ENABLE"},
	{0x1214, "YUV_COUTFIFO_STATUS"},
	{0x1218, "YUV_COUTFIFO_INPUT_CNT"},
	{0x121c, "YUV_COUTFIFO_STALL_CNT"},
	{0x1220, "YUV_COUTFIFO_FIFO_FULLNESS"},
};

static void rgbp_hw_dump_dbg_state(struct pablo_mmio *pmio)
{
	void *ctx;
	const struct is_reg *cr;
	u32 i, val;

	ctx = pmio->ctx ? pmio->ctx : (void *)pmio;
	pmio->reg_read(ctx, RGBP_R_IP_VERSION, &val);

	is_dbg("[HW:%s] v%02u.%02u.%02u ======================================\n", pmio->name,
		(val >> 24) & 0xff, (val >> 16) & 0xff, val & 0xffff);
	for (i = 0; i < ARRAY_SIZE(rgbp_dbg_cr); i++) {
		cr = &rgbp_dbg_cr[i];

		pmio->reg_read(ctx, cr->sfr_offset, &val);
		is_dbg("[HW:%s]%40s %08x\n", pmio->name, cr->reg_name, val);
	}
	is_dbg("[HW:%s]=================================================\n", pmio->name);
}

void rgbp_hw_dump(struct pablo_mmio *pmio, u32 mode)
{
	switch (mode) {
	case HW_DUMP_CR:
		info_hw("[RGBP]%s:DUMP CR\n", __FILENAME__);
		is_hw_dump_regs(pmio_get_base(pmio), rgbp_regs, RGBP_REG_CNT);
		break;
	case HW_DUMP_DBG_STATE:
		info_hw("[RGBP]%s:DUMP DBG_STATE\n", __FILENAME__);
		rgbp_hw_dump_dbg_state(pmio);
		break;
	default:
		err_hw("[RGBP]%s:Not supported dump_mode %d", __FILENAME__, mode);
		break;
	}
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_dump);

static void rgbp_hw_s_cin_fifo(void *base, u32 set_id)
{
	RGBP_SET_F(base, RGBP_R_IP_USE_OTF_PATH, RGBP_F_IP_USE_OTF_IN_FOR_PATH_0, 1);
	/* RGBP_SET_R(base, RGBP_R_COUTFIFO_OUTPUT_COUNT_AT_STALL, val); */

	/* RGBP_SET_R(base  , RGBP_R_COUTFIFO_OUTPUT_T3_INTERVAL, HBLANK_CYCLE); */

	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_CONFIG,
			RGBP_F_BYR_CINFIFO_STALL_BEFORE_FRAME_START_EN, 1);
	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_CONFIG, RGBP_F_BYR_CINFIFO_DEBUG_EN, 1);
	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_CONFIG, RGBP_F_BYR_CINFIFO_AUTO_RECOVERY_EN, 0);

	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_INTERVALS, RGBP_F_BYR_CINFIFO_INTERVAL_HBLANK, HBLANK_CYCLE);

	RGBP_SET_F(base, RGBP_R_CHAIN_LBCTRL_HBLANK, RGBP_F_CHAIN_LBCTRL_HBLANK, HBLANK_CYCLE);
	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_ENABLE, RGBP_F_BYR_CINFIFO_ENABLE, 1);
}

static void rgbp_hw_s_cout_fifo(void *base, u32 set_id, struct param_otf_output *param)
{
	u32 enable = param->cmd;

	RGBP_SET_F(base, GET_COREX_OFFSET(set_id) + RGBP_R_IP_USE_OTF_PATH, RGBP_F_IP_USE_OTF_OUT_FOR_PATH_0, 1);
	/* RGBP_SET_R(base, RGBP_R_COUTFIFO_OUTPUT_COUNT_AT_STALL, val); */

	/* RGBP_SET_R(base, GET_COREX_OFFSET(set_id) + RGBP_R_COUTFIFO_OUTPUT_T3_INTERVAL, HBLANK_CYCLE); */

	RGBP_SET_F(base, GET_COREX_OFFSET(set_id) + RGBP_R_YUV_COUTFIFO_INTERVALS,
			RGBP_F_YUV_COUTFIFO_INTERVAL_HBLANK, HBLANK_CYCLE);
	RGBP_SET_F(base, GET_COREX_OFFSET(set_id) + RGBP_R_YUV_COUTFIFO_INTERVAL_VBLANK,
			RGBP_F_YUV_COUTFIFO_INTERVAL_VBLANK, VBLANK_CYCLE);
	RGBP_SET_F(base, RGBP_R_YUV_COUTFIFO_CONFIG, RGBP_F_YUV_COUTFIFO_VVALID_RISE_AT_FIRST_DATA_EN, 1);
	RGBP_SET_F(base, RGBP_R_YUV_COUTFIFO_CONFIG, RGBP_F_YUV_COUTFIFO_DEBUG_EN, 1);
	RGBP_SET_F(base, RGBP_R_YUV_COUTFIFO_CONFIG, RGBP_F_YUV_COUTFIFO_BACK_STALL_EN, 1);
	RGBP_SET_F(base, GET_COREX_OFFSET(set_id) + RGBP_R_YUV_COUTFIFO_ENABLE,
			RGBP_F_YUV_COUTFIFO_ENABLE, enable);
}

static void rgbp_hw_s_common(void *base)
{
	RGBP_SET_R(base, RGBP_R_AUTO_IGNORE_INTERRUPT_ENABLE, 1);
}

static void rgbp_hw_s_int_mask(void *base)
{
	/* RGBP_SET_F(base, RGBP_R_CONTINT_LEVEL_PULSE_N_SEL, RGBP_F_CONTINT_LEVEL_PULSE_N_SEL, INT_LEVEL); */
	RGBP_SET_R(base, RGBP_R_INT_REQ_INT0_ENABLE, INT0_EN_MASK);
	RGBP_SET_R(base, RGBP_R_INT_REQ_INT1_ENABLE, INT1_EN_MASK);
}

static void rgbp_hw_s_pixel_order(void *base, u32 set_id, u32 pixel_order)
{
	RGBP_SET_R(base, GET_COREX_OFFSET(set_id) + RGBP_R_BYR_DTP_PIXEL_ORDER, pixel_order);
	RGBP_SET_R(base, GET_COREX_OFFSET(set_id) + RGBP_R_BYR_DMSC_PIXEL_ORDER, pixel_order);
}

static void rgbp_hw_s_secure_id(void *base, u32 set_id)
{
	RGBP_SET_F(base, GET_COREX_OFFSET(set_id) + RGBP_R_SECU_CTRL_SEQID, RGBP_F_SECU_CTRL_SEQID, 0);
}

static void rgbp_hw_s_block_crc(void *base, u32 seed)
{
	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_STREAM_CRC, RGBP_F_BYR_CINFIFO_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_BYR_DTP_STREAM_CRC, RGBP_F_BYR_DTP_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_BYR_DMSC_STREAM_CRC, RGBP_F_BYR_DMSC_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_RGB_POSTGAMMA_STREAM_CRC, RGBP_F_RGB_POSTGAMMA_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_RGB_LUMAADAPLSC_STREAM_CRC, RGBP_F_RGB_LUMAADAPLSC_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_RGB_GAMMA_STREAM_CRC, RGBP_F_RGB_GAMMA_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_RGB_GTM_STREAM_CRC, RGBP_F_RGB_GTM_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_RGB_RGBTOYUV_STREAM_CRC, RGBP_F_RGB_RGBTOYUV_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_YUV_YUV444TO422_STREAM_CRC, RGBP_F_YUV_YUV444TO422_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_Y_DECOMP_STREAM_CRC, RGBP_F_Y_DECOMP_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_RGB_CCM33_CRC, RGBP_F_RGB_CCM33_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_YUV_SC_STREAM_CRC, RGBP_F_YUV_SC_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_Y_GAMMALR_STREAM_CRC, RGBP_F_Y_GAMMALR_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_Y_UPSC_STREAM_CRC, RGBP_F_Y_UPSC_CRC_SEED, seed);
	RGBP_SET_F(base, RGBP_R_Y_GAMMAHR_STREAM_CRC, RGBP_F_Y_GAMMAHR_CRC_SEED, seed);
}

void rgbp_hw_s_core(void *base, u32 num_buffers, u32 set_id,
	struct is_rgbp_config *config, struct rgbp_param_set *param_set)
{
	u32 pixel_order;
	u32 seed;

	pixel_order = param_set->otf_input.order;

	rgbp_hw_s_cin_fifo(base, set_id);
	rgbp_hw_s_cout_fifo(base, set_id, &param_set->otf_output);
	rgbp_hw_s_common(base);
	rgbp_hw_s_int_mask(base);
	rgbp_hw_s_path(base, set_id, config, param_set);
	rgbp_hw_s_pixel_order(base, set_id, pixel_order);
	rgbp_hw_s_secure_id(base, set_id);

	seed = is_get_debug_param(IS_DEBUG_PARAM_CRC_SEED);
	if (unlikely(seed))
		rgbp_hw_s_block_crc(base, seed);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_core);

void rgbp_hw_dma_dump(struct is_common_dma *dma)
{
	CALL_DMA_OPS(dma, dma_print_info, 0);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_dma_dump);

static int rgbp_hw_g_rgb_format(u32 rgb_format, u32 *converted_format)
{
	u32 ret = 0;

	switch (rgb_format) {
	case DMA_FMT_RGB_RGBA8888:
		*converted_format = RGBP_DMA_FMT_RGB_RGBA8888;
		break;
	case DMA_FMT_RGB_ABGR8888:
		*converted_format = RGBP_DMA_FMT_RGB_ABGR8888;
		break;
	case DMA_FMT_RGB_ARGB8888:
		*converted_format = RGBP_DMA_FMT_RGB_ARGB8888;
		break;
	case DMA_FMT_RGB_BGRA8888:
		*converted_format = RGBP_DMA_FMT_RGB_BGRA8888;
		break;
	case DMA_FMT_RGB_RGBA1010102:
		*converted_format = RGBP_DMA_FMT_RGB_RGBA1010102;
		break;
	case DMA_FMT_RGB_ABGR1010102:
		*converted_format = RGBP_DMA_FMT_RGB_ABGR1010102;
		break;
	case DMA_FMT_RGB_ARGB1010102:
		*converted_format = RGBP_DMA_FMT_RGB_ARGB1010102;
		break;
	case DMA_FMT_RGB_BGRA1010102:
		*converted_format = RGBP_DMA_FMT_RGB_BGRA1010102;
		break;
	/*
	 * case DMA_FMT_RGB_RGBA16161616:
	 *	*converted_format = RGBP_DMA_FMT_RGB_RGBA16161616;
	 *	break;
	 * case DMA_FMT_RGB_ABGR16161616:
	 *	*converted_format = RGBP_DMA_FMT_RGB_ABGR16161616;
	 *	break;
	 * case DMA_FMT_RGB_ARGB16161616:
	 *	*converted_format = RGBP_DMA_FMT_RGB_ARGB16161616;
	 *	break;
	 * case DMA_FMT_RGB_BGRA16161616:
	 *	*converted_format = RGBP_DMA_FMT_RGB_BGRA16161616;
	 *	break;
	 */
	default:
		err_hw("[RGBP] invalid rgb_format[%d]", rgb_format);
		return -EINVAL;
	}
	return ret;
}

static int rgbp_hw_s_rgb_rdma_format(void *base, u32 rgb_format)
{
	u32 ret = 0;
	u32 converted_format;

	ret = rgbp_hw_g_rgb_format(rgb_format, &converted_format);
	if (ret) {
		err_hw("[RGBP] fail to set rgb_rdma_format[%d]", rgb_format);
		return -EINVAL;
	}
	RGBP_SET_F(base, RGBP_R_RGB1P_FORMAT, RGBP_F_RDMA_RGB1P_FORMAT, converted_format);
	return ret;
}

pdma_addr_t *rgbp_hw_g_input_dva(struct rgbp_param_set *param_set,
	u32 instance, u32 id,  u32 *cmd, u32 *in_rgb)
{
	pdma_addr_t *input_dva = NULL;

	switch (id) {
	case RGBP_RDMA_BYR:
		input_dva = param_set->input_dva;
		*cmd = param_set->dma_input.cmd;
		break;
	case RGBP_RDMA_RGB_EVEN:
		input_dva = param_set->input_dva;
		*cmd = param_set->dma_input.cmd;
		break;
	default:
		merr_hw("invalid ID (%d)", instance, id);
		break;
	}

	return input_dva;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_input_dva);

pdma_addr_t *rgbp_hw_g_output_dva(struct rgbp_param_set *param_set,
	u32 instance, u32 id, u32 *cmd, u32 *out_yuv)
{
	pdma_addr_t *output_dva = NULL;

	switch (id) {
	case RGBP_WDMA_HF:
		output_dva = param_set->output_dva_hf;
		*cmd = param_set->dma_output_hf.cmd;
		break;
	case RGBP_WDMA_Y:
	case RGBP_WDMA_UV:
		output_dva = param_set->output_dva_yuv;
		*cmd = param_set->dma_output_yuv.cmd;
		break;
	case RGBP_WDMA_REPB:
		output_dva = param_set->output_dva_rgb;
		*cmd = param_set->dma_output_rgb.cmd;
		break;
	default:
		merr_hw("invalid ID (%d)", instance, id);
		break;
	}

	return output_dva;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_output_dva);


void rgbp_hw_s_rdma_corex_id(struct is_common_dma *dma, u32 set_id)
{
	CALL_DMA_OPS(dma, dma_set_corex_id, set_id);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_rdma_corex_id);

int rgbp_hw_s_rdma_init(struct is_hw_ip *hw_ip, struct is_common_dma *dma,
	struct rgbp_param_set *param_set, u32 enable, u32 in_crop_size_x, u32 cache_hint,
	u32 *sbwc_en, u32 *payload_size, u32 in_rgb)
{
	struct param_dma_input *dma_input;
	u32 comp_sbwc_en = 0, comp_64b_align = 1, quality_control = 0;
	u32 stride_1p = 0, header_stride_1p = 0;
	u32 hwformat, memory_bitwidth, pixelsize, sbwc_type;
	u32  width, height;
	u32 format, en_votf, bus_info;
	int ret;

	ret = CALL_DMA_OPS(dma, dma_enable, enable);
	if (enable == 0)
		return 0;

	switch (dma->id) {
	case RGBP_RDMA_BYR:
	case RGBP_RDMA_RGB_EVEN:
		dma_input = &param_set->dma_input;
		width = dma_input->width;
		height = dma_input->height;
		break;
	default:
		err_hw("[RGBP] invalid dma_id[%d]", dma->id);
		return SET_ERROR;
	break;
	}

	en_votf = dma_input->v_otf_enable;
	hwformat = dma_input->format;
	sbwc_type = dma_input->sbwc_type;
	memory_bitwidth = dma_input->bitwidth;
	pixelsize = dma_input->msb + 1;
	bus_info = en_votf ? (cache_hint << 4) : 0x00000000UL;  /* cache hint [6:4] */

	dbg_hw(2, "[API][%s]%s %dx%d, format: %d, bitwidth: %d, pixelsize %d\n",
		__func__, dma->name,
		width, height, hwformat, memory_bitwidth, pixelsize);

	*sbwc_en = comp_sbwc_en = is_hw_dma_get_comp_sbwc_en(sbwc_type, &comp_64b_align);
	if (is_hw_dma_get_bayer_format(memory_bitwidth, pixelsize, hwformat, comp_sbwc_en,
						true, &format))
		ret |= DMA_OPS_ERROR;
	if (comp_sbwc_en == 0)
		stride_1p = is_hw_dma_get_img_stride(memory_bitwidth, pixelsize, hwformat,
				width, 16, true);
	else if (comp_sbwc_en == 1 || comp_sbwc_en == 2) {
		stride_1p = is_hw_dma_get_payload_stride(comp_sbwc_en, pixelsize, width,
							comp_64b_align, quality_control,
							RGBP_COMP_BLOCK_WIDTH,
							RGBP_COMP_BLOCK_HEIGHT);
		header_stride_1p = is_hw_dma_get_header_stride(width, RGBP_COMP_BLOCK_WIDTH, 16);
	} else {
		return SET_ERROR;
	}

	if (dma->id == RGBP_RDMA_RGB_EVEN)
		ret |= rgbp_hw_s_rgb_rdma_format(get_base(hw_ip), format);
	else
		ret |= CALL_DMA_OPS(dma, dma_set_format, format, DMA_FMT_BAYER);

	if (dma->id == RGBP_RDMA_RGB_EVEN) {
		width = width * 4;
		stride_1p = width;
	}

	ret |= CALL_DMA_OPS(dma, dma_set_comp_sbwc_en, comp_sbwc_en);
	ret |= CALL_DMA_OPS(dma, dma_set_size, width, height);
	ret |= CALL_DMA_OPS(dma, dma_set_img_stride, stride_1p, 0, 0);
	ret |= CALL_DMA_OPS(dma, dma_votf_enable, en_votf, 0);
	ret |= CALL_DMA_OPS(dma, dma_set_bus_info, bus_info);

	*payload_size = 0;
	switch (comp_sbwc_en) {
	case 1:
	case 2:
		ret |= CALL_DMA_OPS(dma, dma_set_comp_64b_align, comp_64b_align);
		ret |= CALL_DMA_OPS(dma, dma_set_header_stride, header_stride_1p, 0);
		*payload_size = ((height + RGBP_COMP_BLOCK_HEIGHT - 1) / RGBP_COMP_BLOCK_HEIGHT) * stride_1p;
		break;
	default:
		break;
	}

	return ret;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_rdma_init);

static int rgbp_hw_rdma_create_pmio(struct is_common_dma *dma, void *base, u32 input_id)
{
	ulong available_bayer_format_map;
	int ret;
	char *name;

	name = __getname();
	if (!name) {
		err_hw("[RGBP] Failed to get name buffer");
		return -ENOMEM;
	}

	switch (input_id) {
	case RGBP_RDMA_BYR:
		dma->reg_ofs = RGBP_R_BYR_RDMABYRIN_EN;
		dma->field_ofs = RGBP_F_BYR_RDMABYRIN_EN;

		/* Bayer: 0,1,2,4,5,6,8,9,10 */
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		snprintf(name, PATH_MAX, "RGBP_RDMA_BYR");
		break;
	case RGBP_RDMA_RGB_EVEN:
		dma->reg_ofs = RGBP_R_RGB_RDMAREPRGBEVEN_EN;
		dma->field_ofs = RGBP_F_RGB_RDMAREPRGBEVEN_EN;

		/* is_rgbp_rgb_format: 0,1,2,3,4,5,6,8,9,10,11,12,13 */
		available_bayer_format_map = 0x3F3F;
		snprintf(name, PATH_MAX, "RGBP_RDMA_RGB_EVEN");
		break;
	default:
		err_hw("[RGBP] invalid dma_id[%d]", dma->id);
		ret = SET_ERROR;
		goto err_dma_create;
	}

	ret = pmio_dma_set_ops(dma);
	ret |= pmio_dma_create(dma, base, input_id, name, available_bayer_format_map, 0, 0);

err_dma_create:
	__putname(name);

	return ret;
}

int rgbp_hw_rdma_create(struct is_common_dma *dma, void *base, u32 dma_id)
{
	return rgbp_hw_rdma_create_pmio(dma, base, dma_id);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_rdma_create);

int rgbp_hw_s_rdma_addr(struct is_common_dma *dma, pdma_addr_t *addr, u32 plane,
	u32 num_buffers, int buf_idx, u32 comp_sbwc_en, u32 payload_size, u32 in_rgb)
{
	int ret, i;
	dma_addr_t address[IS_MAX_FRO];

	switch (dma->id) {
	case RGBP_RDMA_BYR:
		for (i = 0; i < num_buffers; i++)
			address[i] = (dma_addr_t)*(addr);
		ret = CALL_DMA_OPS(dma, dma_set_img_addr, address, plane, buf_idx, num_buffers);
		break;
	case RGBP_RDMA_RGB_EVEN:
		for (i = 0; i < num_buffers; i++)
			address[i] = (dma_addr_t)*(addr);
		ret = CALL_DMA_OPS(dma, dma_set_img_addr, address, plane, buf_idx, num_buffers);
		break;
	default:
		err_hw("[RGBP] invalid dma_id[%d]", dma->id);
		return SET_ERROR;
	}

	return ret;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_rdma_addr);

void rgbp_hw_s_wdma_corex_id(struct is_common_dma *dma, u32 set_id)
{
	CALL_DMA_OPS(dma, dma_set_corex_id, set_id);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_wdma_corex_id);

int rgbp_hw_s_wdma_init(struct is_common_dma *dma, void *base, u32 set_id,
	struct rgbp_param_set *param_set, pdma_addr_t *output_dva, struct rgbp_dma_cfg *dma_cfg)
{
	struct param_dma_output *dma_output;
	u32 comp_sbwc_en = 0, comp_64b_align = 1, quality_control = 0;
	u32 stride_1p = 0, header_stride_1p = 0;
	u32 hwformat, memory_bitwidth, pixelsize, sbwc_type;
	u32 width, height;
	u32 format, en_votf, bus_info, dma_format;
	u32 strip_offset_in_pixel = 0, strip_offset_in_byte = 0, strip_header_offset_in_byte = 0;
	u32 comp_block_width = RGBP_COMP_BLOCK_WIDTH;
	u32 comp_block_height = RGBP_COMP_BLOCK_HEIGHT;
	u32 full_frame_width;
	u32 strip_enable = (param_set->stripe_input.total_count < 2) ? 0 : 1;
	struct rgbp_dma_addr_cfg dma_addr_cfg;
	int ret = SET_SUCCESS;

	ret = CALL_DMA_OPS(dma, dma_enable, dma_cfg->enable);
	if (dma_cfg->enable == 0)
		return 0;

	switch (dma->id) {
	case RGBP_WDMA_HF:
		dma_output = &param_set->dma_output_hf;
		comp_block_width = MCSC_HF_COMP_BLOCK_WIDTH;
		comp_block_height = MCSC_HF_COMP_BLOCK_HEIGHT;
		break;
	case RGBP_WDMA_Y:
	case RGBP_WDMA_UV:
		dma_output = &param_set->dma_output_yuv;
		strip_offset_in_pixel =
			param_set->stripe_input.start_pos_x + param_set->stripe_input.left_margin;
		break;
	case RGBP_WDMA_REPB:
		dma_output = &param_set->dma_output_rgb;
		break;
	default:
		err_hw("[RGBP] invalid dma_id[%d]", dma->id);
		return SET_ERROR;
	}

	width = dma_output->width;
	height = dma_output->height;
	en_votf = dma_output->v_otf_enable;
	hwformat = dma_output->format;
	sbwc_type = dma_output->sbwc_type;
	memory_bitwidth = dma_output->bitwidth;
	pixelsize = dma_output->msb + 1;
	bus_info = en_votf ? (dma_cfg->cache_hint << 4) : 0x00000000UL;  /* cache hint [6:4] */

	full_frame_width = (strip_enable) ? param_set->stripe_input.full_width : width;

	dbg_hw(2, "[API][%s]%s %dx%d, format: %d, bitwidth: %d, pixelsize %d\n",
		__func__, dma->name,
		width, height, hwformat, memory_bitwidth, pixelsize);

	dma_addr_cfg.sbwc_en = comp_sbwc_en = is_hw_dma_get_comp_sbwc_en(sbwc_type, &comp_64b_align);
	if (is_hw_dma_get_bayer_format(memory_bitwidth, pixelsize, hwformat, comp_sbwc_en,
						true, &format))
		ret |= DMA_OPS_ERROR;
	if (comp_sbwc_en == 0) {
		stride_1p = is_hw_dma_get_img_stride(memory_bitwidth, pixelsize, hwformat,
				full_frame_width, 16, true);
		if (strip_enable)
			strip_offset_in_byte = is_hw_dma_get_img_stride(memory_bitwidth, pixelsize,
					hwformat, strip_offset_in_pixel, 16, true);
	} else if (comp_sbwc_en == 1 || comp_sbwc_en == 2) {
		stride_1p = is_hw_dma_get_payload_stride(comp_sbwc_en, pixelsize, full_frame_width,
							comp_64b_align, quality_control,
							comp_block_width, comp_block_height);
		header_stride_1p = is_hw_dma_get_header_stride(full_frame_width, comp_block_width, 16);
		if (strip_enable) {
			strip_offset_in_byte = is_hw_dma_get_payload_stride(comp_sbwc_en, pixelsize,
								strip_offset_in_pixel,
								comp_64b_align, quality_control,
								comp_block_width,
								comp_block_height);
			strip_header_offset_in_byte =
				is_hw_dma_get_header_stride(strip_offset_in_pixel,
								comp_block_width, 0);
		}
	} else {
		return SET_ERROR;
	}

	switch (dma->id) {
	case RGBP_WDMA_HF:
	case RGBP_WDMA_Y:
	case RGBP_WDMA_UV:
		dma_format = DMA_FMT_BAYER;
		if (is_hw_dma_get_bayer_format(memory_bitwidth, pixelsize, hwformat, comp_sbwc_en,
				true, &format))
			ret |= DMA_OPS_ERROR;
		break;
	case RGBP_WDMA_REPB:
		dma_format = DMA_FMT_RGB;
		/* FIXE ME */
		format = is_hw_dma_get_rgb_format(memory_bitwidth, 1, DMA_INOUT_ORDER_RGBA);
		break;
	default:
		err_hw("[RGBP] invalid dma_id[%d]", dma->id);
		return SET_ERROR;
	}

	ret |= CALL_DMA_OPS(dma, dma_set_format, format, dma_format);
	ret |= CALL_DMA_OPS(dma, dma_set_comp_sbwc_en, comp_sbwc_en);
	ret |= CALL_DMA_OPS(dma, dma_set_size, width, height);
	ret |= CALL_DMA_OPS(dma, dma_set_img_stride, stride_1p, 0, 0);
	ret |= CALL_DMA_OPS(dma, dma_votf_enable, en_votf, 0);
	ret |= CALL_DMA_OPS(dma, dma_set_bus_info, bus_info);

	dma_addr_cfg.payload_size = 0;
	switch (comp_sbwc_en) {
	case 1:
	case 2:
		ret |= CALL_DMA_OPS(dma, dma_set_comp_64b_align, comp_64b_align);
		ret |= CALL_DMA_OPS(dma, dma_set_header_stride, header_stride_1p, 0);
		dma_addr_cfg.payload_size = ((height + comp_block_height - 1) / comp_block_height) * stride_1p;
		break;
	default:
		break;
	}

	if (ret)
		return ret;

	dma_addr_cfg.strip_offset = strip_offset_in_byte;
	dma_addr_cfg.header_offset = strip_header_offset_in_byte;

	if (dma_cfg->enable == DMA_OUTPUT_COMMAND_ENABLE) {
		ret = rgbp_hw_s_wdma_addr(dma, output_dva, dma_cfg->num_buffers,
					&dma_addr_cfg);
		if (ret) {
			err_hw("[RGBP] failed to set RGBP_WDMA(%d) address", dma->id);

			return -EINVAL;
		}
		rgbp_hw_s_sbwc(base, set_id, comp_sbwc_en ? true : false);
	}

	return ret;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_wdma_init);

static int rgbp_hw_wdma_create_pmio(struct is_common_dma *dma, void *base, u32 input_id)
{
	ulong available_bayer_format_map;
	int ret;
	char *name;

	name = __getname();
	if (!name) {
		err_hw("[RGBP] Failed to get name buffer");
		return -ENOMEM;
	}

	switch (input_id) {
	case RGBP_WDMA_HF:
		dma->reg_ofs = RGBP_R_STAT_WDMADECOMP_EN;
		dma->field_ofs = RGBP_F_STAT_WDMADECOMP_EN;

		/* Bayer: 0,1,2 */
		available_bayer_format_map = 0x7 & IS_BAYER_FORMAT_MASK;
		snprintf(name, PATH_MAX, "RGBP_WDMA_HF");
		break;
	case RGBP_WDMA_Y:
		dma->reg_ofs = RGBP_R_YUV_WDMAY_EN;
		dma->field_ofs = RGBP_F_YUV_WDMAY_EN;

		/* Bayer: 0,1,2,4,5,6,8,9,10 */
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		snprintf(name, PATH_MAX, "RGBP_WDMA_Y");
		break;
	case RGBP_WDMA_UV:
		dma->reg_ofs = RGBP_R_YUV_WDMAUV_EN;
		dma->field_ofs = RGBP_F_YUV_WDMAUV_EN;

		/* Bayer: 0,1,2,4,5,6,8,9,10 */
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		snprintf(name, PATH_MAX, "RGBP_WDMA_UV");
		break;
	case RGBP_WDMA_REPB:
		/* is_rgbp_rgb_format: 0,1,2,3,4,5,6,8,9,10,11,12,13 */
		dma->reg_ofs = RGBP_R_YUV_WDMAREPB_EN;
		dma->field_ofs = RGBP_F_YUV_WDMAREPB_EN;

		available_bayer_format_map = 0x3F3F;
		snprintf(name, PATH_MAX, "RGBP_WDMA_REPB");
		break;
	default:
		err_hw("[RGBP] invalid dma_id[%d]", dma->id);
		ret = SET_ERROR;
		goto err_dma_create;
	}

	ret = pmio_dma_set_ops(dma);
	ret |= pmio_dma_create(dma, base, input_id, name, available_bayer_format_map, 0, 0);

err_dma_create:
	__putname(name);

	return ret;
}

int rgbp_hw_wdma_create(struct is_common_dma *dma, void *base, u32 dma_id)
{
	return rgbp_hw_wdma_create_pmio(dma, base, dma_id);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_wdma_create);

int rgbp_hw_s_wdma_addr(struct is_common_dma *dma, pdma_addr_t *addr, u32 num_buffers,
			struct rgbp_dma_addr_cfg *dma_addr_cfg)
{
	int ret, i;
	dma_addr_t address[IS_MAX_FRO];
	dma_addr_t hdr_addr[IS_MAX_FRO];

	switch (dma->id) {
	case RGBP_WDMA_HF:
		for (i = 0; i < num_buffers; i++)
			address[i] = (dma_addr_t)*(addr + i) + dma_addr_cfg->strip_offset;
		ret = CALL_DMA_OPS(dma, dma_set_img_addr, address, 0, 0, num_buffers);
		break;
	case RGBP_WDMA_Y:
		for (i = 0; i < num_buffers; i++)
			address[i] = (dma_addr_t)*(addr + (2 * i)) + dma_addr_cfg->strip_offset;
		ret = CALL_DMA_OPS(dma, dma_set_img_addr, address, 0, 0, num_buffers);
		break;
	case RGBP_WDMA_UV:
		for (i = 0; i < num_buffers; i++)
			address[i] = (dma_addr_t)*(addr + (2 * i + 1)) + dma_addr_cfg->strip_offset;
		ret = CALL_DMA_OPS(dma, dma_set_img_addr, address, 0, 0, num_buffers);
		break;
	case RGBP_WDMA_REPB:
		for (i = 0; i < num_buffers; i++)
			address[i] = (dma_addr_t)*(addr + i) + dma_addr_cfg->strip_offset;
		ret = CALL_DMA_OPS(dma, dma_set_img_addr, address, 0, 0, num_buffers);
		break;
	default:
		err_hw("[RGBP] invalid dma_id[%d]", dma->id);
		return SET_ERROR;
	}

	if (dma_addr_cfg->sbwc_en) {
		/* Lossless, Lossy need to set header base address */
		switch (dma->id) {
		case RGBP_WDMA_HF:
			for (i = 0; i < num_buffers; i++)
				hdr_addr[i] = (dma_addr_t)*(addr + i) +
						dma_addr_cfg->payload_size + dma_addr_cfg->header_offset;
			break;
		case RGBP_WDMA_Y:
			for (i = 0; i < num_buffers; i++)
				hdr_addr[i] = (dma_addr_t)*(addr + (2 * i)) +
						dma_addr_cfg->payload_size + dma_addr_cfg->header_offset;
			break;
		case RGBP_WDMA_UV:
			for (i = 0; i < num_buffers; i++)
				hdr_addr[i] = (dma_addr_t)*(addr + (2 * i + 1)) +
						dma_addr_cfg->payload_size + dma_addr_cfg->header_offset;
			break;
		default:
			break;
		}

		ret = CALL_DMA_OPS(dma, dma_set_header_addr, hdr_addr, 0, 0, num_buffers);
	}

	return ret;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_wdma_addr);

int rgbp_hw_s_corex_update_type(void *base, u32 set_id, u32 type)
{
	switch (set_id) {
	case COREX_SET_A:
	case COREX_SET_B:
		RGBP_SET_F(base, RGBP_R_COREX_UPDATE_TYPE_0, RGBP_F_COREX_UPDATE_TYPE_0, type);
		RGBP_SET_F(base, RGBP_R_COREX_UPDATE_TYPE_1, RGBP_F_COREX_UPDATE_TYPE_1, type);
		break;
	case COREX_DIRECT:
		RGBP_SET_F(base, RGBP_R_COREX_UPDATE_TYPE_0, RGBP_F_COREX_UPDATE_TYPE_0, COREX_IGNORE);
		RGBP_SET_F(base, RGBP_R_COREX_UPDATE_TYPE_1, RGBP_F_COREX_UPDATE_TYPE_1, COREX_IGNORE);
		break;
	default:
		err_hw("[RGBP] invalid corex set_id");
		return -EINVAL;
	}

	return 0;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_corex_update_type);

void rgbp_hw_s_cmdq(void *base, u32 set_id, u32 num_buffers, dma_addr_t clh, u32 noh)
{
	int i;
	u32 fro_en = num_buffers > 1 ? 1 : 0;

	if (fro_en)
		RGBP_SET_R(base, RGBP_R_CMDQ_ENABLE, 0);

	for (i = 0; i < num_buffers; ++i) {
		if (i == 0)
			RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_L, RGBP_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE,
				fro_en ? RGBP_INT_GRP_EN_MASK_FRO_FIRST : RGBP_INT_GRP_EN_MASK);
		else if (i < num_buffers - 1)
			RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_L, RGBP_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE,
				RGBP_INT_GRP_EN_MASK_FRO_MIDDLE);
		else
			RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_L, RGBP_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE,
				RGBP_INT_GRP_EN_MASK_FRO_LAST);

		if (clh && noh) {
			RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_H, RGBP_F_CMDQ_QUE_CMD_BASE_ADDR, DVA_36BIT_HIGH(clh));
			RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_M, RGBP_F_CMDQ_QUE_CMD_HEADER_NUM, noh);
			RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_M, RGBP_F_CMDQ_QUE_CMD_SETTING_MODE, 1);
		} else {
			RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_M, RGBP_F_CMDQ_QUE_CMD_SETTING_MODE, 3);
		}

		RGBP_SET_F(base, RGBP_R_CMDQ_QUE_CMD_L, RGBP_F_CMDQ_QUE_CMD_FRO_INDEX, i);
		RGBP_SET_R(base, RGBP_R_CMDQ_ADD_TO_QUEUE_0, 1);
	}
	RGBP_SET_R(base, RGBP_R_CMDQ_ENABLE, 1);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_cmdq);

void rgbp_hw_s_corex_init(void *base, bool enable)
{
	u32 reset_count = 0;

	/*
	 * Check COREX idleness
	 */
	if (!enable) {
		/* TODO :  check COREX_UPDATE_MODE_0/1 to 1 */
		RGBP_SET_F(base, RGBP_R_COREX_UPDATE_MODE_0, RGBP_F_COREX_UPDATE_MODE_0, SW_TRIGGER);

		rgbp_hw_wait_corex_idle(base);

		RGBP_SET_F(base, RGBP_R_COREX_ENABLE, RGBP_F_COREX_ENABLE, 0);

		info_hw("[RGBP] %s disable done\n", __func__);

		return;
	}

	/*
	 * Set Fixed Value
	 */
	RGBP_SET_F(base, RGBP_R_COREX_ENABLE, RGBP_F_COREX_ENABLE, 1);
	RGBP_SET_F(base, RGBP_R_COREX_RESET, RGBP_F_COREX_RESET, 1);

	while (RGBP_GET_R(base, RGBP_R_COREX_RESET)) {
		if (reset_count > RGBP_TRY_COUNT) {
			err_hw("[RGBP] fail to wait corex reset");
			break;
		}
		reset_count++;
	}

	/*
	 * Type selection
	 * Only type0 will be used.
	 */
	RGBP_SET_F(base, RGBP_R_COREX_TYPE_WRITE_TRIGGER, RGBP_F_COREX_TYPE_WRITE_TRIGGER, 1);
	RGBP_SET_F(base, RGBP_R_COREX_TYPE_WRITE, RGBP_F_COREX_TYPE_WRITE, 0);

	RGBP_SET_F(base, RGBP_R_COREX_UPDATE_TYPE_0, RGBP_F_COREX_UPDATE_TYPE_0, COREX_COPY);
	RGBP_SET_F(base, RGBP_R_COREX_UPDATE_TYPE_1, RGBP_F_COREX_UPDATE_TYPE_1, COREX_IGNORE);
	// 1st frame uses SW Trigger, others use H/W Trigger
	RGBP_SET_F(base, RGBP_R_COREX_UPDATE_MODE_0, RGBP_F_COREX_UPDATE_MODE_0, SW_TRIGGER);
	RGBP_SET_F(base, RGBP_R_COREX_UPDATE_MODE_1, RGBP_F_COREX_UPDATE_MODE_1, SW_TRIGGER);

	RGBP_SET_R(base, RGBP_R_COREX_COPY_FROM_IP_0, 1);
	RGBP_SET_R(base, RGBP_R_COREX_COPY_FROM_IP_1, 1);

	/*
	 * Check COREX idleness, again.
	 */
	rgbp_hw_wait_corex_idle(base);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_corex_init);

void rgbp_hw_wait_corex_idle(void *base)
{
	u32 busy;
	u32 try_cnt = 0;

	do {
		udelay(1);

		try_cnt++;
		if (try_cnt >= RGBP_TRY_COUNT) {
			err_hw("[RGBP] fail to wait corex idle");
			break;
		}

		busy = RGBP_GET_F(base, RGBP_R_COREX_STATUS_0, RGBP_F_COREX_BUSY_0);
		dbg_hw(1, "[RGBP] %s busy(%d)\n", __func__, busy);

	} while (busy);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_wait_corex_idle);

/*
 * Context: O
 * CR type: No Corex
 */
void rgbp_hw_s_corex_start(void *base, bool enable)
{
	if (!enable)
		return;

	/*
	 * Set Fixed Value
	 *
	 * Type0 only:
	 * @RGBP_R_COREX_START_0 - 1: puls generation
	 * @RGBP_R_COREX_UPDATE_MODE_0 - 0: HW trigger, 1: SW tirgger
	 *
	 * SW trigger should be needed before stream on
	 * because there is no HW trigger before stream on.
	 */
	RGBP_SET_F(base, RGBP_R_COREX_UPDATE_MODE_0, RGBP_F_COREX_UPDATE_MODE_0, SW_TRIGGER);
	RGBP_SET_F(base, RGBP_R_COREX_START_0, RGBP_F_COREX_START_0, 0x1);

	rgbp_hw_wait_corex_idle(base);

	RGBP_SET_F(base, RGBP_R_COREX_UPDATE_MODE_0, RGBP_F_COREX_UPDATE_MODE_0, HW_TRIGGER);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_corex_start);

unsigned int rgbp_hw_g_int0_state(void *base, bool clear, u32 num_buffers, u32 *irq_state)
{
	u32 src, src_all, /*src_fro,*/ src_err;

	/*
	 * src_all: per-frame based rgbp IRQ status
	 * src_fro: FRO based rgbp IRQ status
	 *
	 * final normal status: src_fro (start, line, end)
	 * final error status(src_err): src_all & ERR_MASK
	 */
	src_all = RGBP_GET_R(base, RGBP_R_INT_REQ_INT0);

	if (clear)
		RGBP_SET_R(base, RGBP_R_INT_REQ_INT0_CLEAR, src_all);

	src_err = src_all & INT0_ERR_MASK;

	*irq_state = src_all;

	src = src_all;

	return src | src_err;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_int0_state);

void rgbp_hw_int0_error_handle(void *base)
{
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_int0_error_handle);

unsigned int rgbp_hw_g_int0_mask(void *base)
{
	return RGBP_GET_R(base, RGBP_R_INT_REQ_INT0_ENABLE);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_int0_mask);

unsigned int rgbp_hw_g_int1_state(void *base, bool clear, u32 num_buffers, u32 *irq_state)
{
	u32 src, src_all, /*src_fro,*/ src_err;

	/*
	 * src_all: per-frame based rgbp IRQ status
	 * src_fro: FRO based rgbp IRQ status
	 *
	 * final normal status: src_fro (start, line, end)
	 * final error status(src_err): src_all & ERR_MASK
	 */
	src_all = RGBP_GET_R(base, RGBP_R_INT_REQ_INT1);

	if (clear)
		RGBP_SET_R(base, RGBP_R_INT_REQ_INT1_CLEAR, src_all);

	src_err = src_all & INT1_ERR_MASK;

	*irq_state = src_all;

	src = src_all;
	return src | src_err;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_int1_state);

unsigned int rgbp_hw_g_int1_mask(void *base)
{
	return RGBP_GET_R(base, RGBP_R_INT_REQ_INT1_ENABLE);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_int1_mask);

void rgbp_hw_s_chain_src_size(void *base, u32 set_id, u32 width, u32 height)
{
	RGBP_SET_F(base, RGBP_R_CHAIN_SRC_IMG_SIZE, RGBP_F_CHAIN_SRC_IMG_WIDTH, width);
	RGBP_SET_F(base, RGBP_R_CHAIN_SRC_IMG_SIZE, RGBP_F_CHAIN_SRC_IMG_HEIGHT, height);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_chain_src_size);

void rgbp_hw_s_chain_dst_size(void *base, u32 set_id, u32 width, u32 height)
{
	RGBP_SET_F(base, RGBP_R_CHAIN_DST_IMG_SIZE, RGBP_F_CHAIN_DST_IMG_WIDTH, width);
	RGBP_SET_F(base, RGBP_R_CHAIN_DST_IMG_SIZE, RGBP_F_CHAIN_DST_IMG_HEIGHT, height);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_chain_dst_size);

void rgbp_hw_s_dtp_output_size(void *base, u32 set_id, u32 width, u32 height)
{
	RGBP_SET_F(base, RGBP_R_BYR_DTP_X_OUTPUT_SIZE, RGBP_F_BYR_DTP_X_OUTPUT_SIZE, width);
	RGBP_SET_F(base, RGBP_R_BYR_DTP_Y_OUTPUT_SIZE, RGBP_F_BYR_DTP_Y_OUTPUT_SIZE, height);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_dtp_output_size);

void rgbp_hw_s_decomp_frame_size(void *base, u32 set_id, u32 width, u32 height)
{
	RGBP_SET_F(base, RGBP_R_Y_DECOMP_FRAME_SIZE, RGBP_F_Y_DECOMP_FRAME_WIDTH, width);
	RGBP_SET_F(base, RGBP_R_Y_DECOMP_FRAME_SIZE, RGBP_F_Y_DECOMP_FRAME_HEIGHT, height);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_decomp_frame_size);

void rgbp_hw_s_sc_dst_size_size(void *base, u32 set_id, u32 width, u32 height)
{
	RGBP_SET_F(base, RGBP_R_YUV_SC_DST_SIZE, RGBP_F_YUV_SC_DST_HSIZE, width);
	RGBP_SET_F(base, RGBP_R_YUV_SC_DST_SIZE, RGBP_F_YUV_SC_DST_VSIZE, height);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_sc_dst_size_size);

void rgbp_hw_s_block_bypass(void *base, u32 set_id)
{
	/* default : otf in/out setting */
	RGBP_SET_F(base, RGBP_R_BYR_DMSC_BYPASS, RGBP_F_BYR_DMSC_BYPASS, 0);
	RGBP_SET_F(base, RGBP_R_RGB_POSTGAMMA_BYPASS, RGBP_F_RGB_POSTGAMMA_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_RGB_LUMAADAPLSC_BYPASS, RGBP_F_RGB_LUMAADAPLSC_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_RGB_GAMMA_BYPASS, RGBP_F_RGB_GAMMA_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_RGB_GTM_BYPASS, RGBP_F_RGB_GTM_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_RGB_RGBTOYUV_BYPASS, RGBP_F_RGB_RGBTOYUV_BYPASS, 0);
	RGBP_SET_F(base, RGBP_R_YUV_YUV444TO422_ISP_BYPASS, RGBP_F_YUV_YUV444TO422_BYPASS, 0);

	RGBP_SET_F(base, RGBP_R_Y_DECOMP_BYPASS, RGBP_F_Y_DECOMP_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_Y_GAMMALR_BYPASS, RGBP_F_Y_GAMMALR_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_Y_GAMMAHR_BYPASS, RGBP_F_Y_GAMMAHR_BYPASS, 1);

	RGBP_SET_F(base, RGBP_R_YUV_SC_CTRL0, RGBP_F_YUV_SC_TOP_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_OTF_CROP_CTRL, RGBP_F_RGB_DMSCCROP_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_DMA_CROP_CTRL, RGBP_F_STAT_DECOMPCROP_BYPASS, 1);
	RGBP_SET_F(base, RGBP_R_DMA_CROP_CTRL, RGBP_F_YUV_SCCROP_BYPASS, 1);

	RGBP_SET_F(base, RGBP_R_RGB_CCM33_BYPASS, RGBP_F_RGB_CCM33_BYPASS, 1);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_block_bypass);

void rgbp_hw_s_dns_size(void *base, u32 set_id,
	u32 width, u32 height, bool strip_enable, u32 strip_start_pos,
	struct rgbp_radial_cfg *radial_cfg, struct is_rgbp_config *rgbp_config)
{
	/* Not support */
}

u32 rgbp_hw_get_scaler_coef(u32 ratio)
{
	u32 coef;

	if (ratio <= RGBP_RATIO_X8_8)
		coef = RGBP_COEFF_X8_8;
	else if (ratio > RGBP_RATIO_X8_8 && ratio <= RGBP_RATIO_X7_8)
		coef = RGBP_COEFF_X7_8;
	else if (ratio > RGBP_RATIO_X7_8 && ratio <= RGBP_RATIO_X6_8)
		coef = RGBP_COEFF_X6_8;
	else if (ratio > RGBP_RATIO_X6_8 && ratio <= RGBP_RATIO_X5_8)
		coef = RGBP_COEFF_X5_8;
	else if (ratio > RGBP_RATIO_X5_8 && ratio <= RGBP_RATIO_X4_8)
		coef = RGBP_COEFF_X4_8;
	else if (ratio > RGBP_RATIO_X4_8 && ratio <= RGBP_RATIO_X3_8)
		coef = RGBP_COEFF_X3_8;
	else if (ratio > RGBP_RATIO_X3_8 && ratio <= RGBP_RATIO_X2_8)
		coef = RGBP_COEFF_X2_8;
	else
		coef = RGBP_COEFF_X2_8;

	return coef;
}

void rgbp_hw_s_yuvsc_enable(void *base, u32 set_id, u32 enable, u32 bypass)
{
	RGBP_SET_F(base, RGBP_R_YUV_SC_CTRL0, RGBP_F_YUV_SC_TOP_BYPASS, bypass);
	RGBP_SET_F(base, RGBP_R_YUV_SC_CTRL0, RGBP_F_YUV_SC_BYPASS, bypass);
	RGBP_SET_F(base, RGBP_R_YUV_SC_CTRL1, RGBP_F_YUV_SC_LR_HR_MERGE_SPLIT_ON, 1);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_yuvsc_enable);

void rgbp_hw_s_yuvsc_dst_size(void *base, u32 set_id, u32 h_size, u32 v_size)
{
	RGBP_SET_F(base, RGBP_R_YUV_SC_DST_SIZE, RGBP_F_YUV_SC_DST_HSIZE, h_size);
	RGBP_SET_F(base, RGBP_R_YUV_SC_DST_SIZE, RGBP_F_YUV_SC_DST_VSIZE, v_size);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_yuvsc_dst_size);

void rgbp_hw_g_yuvsc_dst_size(void *base, u32 set_id, u32 *h_size, u32 *v_size)
{
	*h_size = RGBP_GET_F(base, RGBP_R_YUV_SC_DST_SIZE, RGBP_F_YUV_SC_DST_HSIZE);
	*v_size = RGBP_GET_F(base, RGBP_R_YUV_SC_DST_SIZE, RGBP_F_YUV_SC_DST_VSIZE);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_yuvsc_dst_size);

void rgbp_hw_s_yuvsc_scaling_ratio(void *base, u32 set_id, u32 hratio, u32 vratio)
{
	RGBP_SET_F(base, RGBP_R_YUV_SC_H_RATIO, RGBP_F_YUV_SC_H_RATIO, hratio);
	RGBP_SET_F(base, RGBP_R_YUV_SC_V_RATIO, RGBP_F_YUV_SC_V_RATIO, vratio);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_yuvsc_scaling_ratio);

void rgbp_hw_s_h_init_phase_offset(void *base, u32 set_id, u32 h_offset)
{
	RGBP_SET_F(base, RGBP_R_YUV_SC_H_INIT_PHASE_OFFSET, RGBP_F_YUV_SC_H_INIT_PHASE_OFFSET, h_offset);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_h_init_phase_offset);

void rgbp_hw_s_v_init_phase_offset(void *base, u32 set_id, u32 v_offset)
{
	RGBP_SET_F(base, RGBP_R_YUV_SC_V_INIT_PHASE_OFFSET, RGBP_F_YUV_SC_V_INIT_PHASE_OFFSET, v_offset);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_v_init_phase_offset);

void rgbp_hw_s_yuvsc_coef(void *base, u32 set_id, u32 hratio, u32 vratio)
{
	u32 h_coef = 0, v_coef = 0;
	/* this value equals 0 - scale-down operation */
	u32 h_phase_offset = 0, v_phase_offset = 0;

	h_coef = rgbp_hw_get_scaler_coef(hratio);
	v_coef = rgbp_hw_get_scaler_coef(vratio);

	/* scale up case */
	if (vratio < RGBP_RATIO_X8_8)
		v_phase_offset = vratio >> 1;

	rgbp_hw_s_h_init_phase_offset(base, set_id, h_phase_offset);
	rgbp_hw_s_v_init_phase_offset(base, set_id, v_phase_offset);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_yuvsc_coef);

void rgbp_hw_s_yuvsc_round_mode(void *base, u32 set_id, u32 mode)
{
	RGBP_SET_F(base, RGBP_R_YUV_SC_ROUND_TH, RGBP_F_YUV_SC_ROUND_TH, mode);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_yuvsc_round_mode);

/* upsc */
void rgbp_hw_s_upsc_enable(void *base, u32 set_id, u32 enable, u32 bypass)
{
	RGBP_SET_F(base, RGBP_R_Y_UPSC_CTRL0, RGBP_F_Y_UPSC_ENABLE, enable);
	RGBP_SET_F(base, RGBP_R_Y_UPSC_CTRL0, RGBP_F_Y_UPSC_BYPASS, bypass);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_upsc_enable);

void rgbp_hw_s_upsc_dst_size(void *base, u32 set_id, u32 h_size, u32 v_size)
{
	RGBP_SET_F(base, RGBP_R_Y_UPSC_DST_SIZE, RGBP_F_Y_UPSC_DST_HSIZE, h_size);
	RGBP_SET_F(base, RGBP_R_Y_UPSC_DST_SIZE, RGBP_F_Y_UPSC_DST_VSIZE, v_size);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_upsc_dst_size);

void rgbp_hw_g_upsc_dst_size(void *base, u32 set_id, u32 *h_size, u32 *v_size)
{
	*h_size = RGBP_GET_F(base, RGBP_R_Y_UPSC_DST_SIZE, RGBP_F_Y_UPSC_DST_HSIZE);
	*v_size = RGBP_GET_F(base, RGBP_R_Y_UPSC_DST_SIZE, RGBP_F_Y_UPSC_DST_VSIZE);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_upsc_dst_size);

void rgbp_hw_s_upsc_scaling_ratio(void *base, u32 set_id, u32 hratio, u32 vratio)
{
	RGBP_SET_F(base, RGBP_R_Y_UPSC_H_RATIO, RGBP_F_Y_UPSC_H_RATIO, hratio);
	RGBP_SET_F(base, RGBP_R_Y_UPSC_V_RATIO, RGBP_F_Y_UPSC_V_RATIO, vratio);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_upsc_scaling_ratio);

void rgbp_hw_s_upsc_coef(void *base, u32 set_id, u32 hratio, u32 vratio)
{
	u32 h_coef, v_coef;
	/* this value equals 0 - scale-down operation */
	u32 h_phase_offset = 0, v_phase_offset = 0;

	h_coef = rgbp_hw_get_scaler_coef(hratio);
	v_coef = rgbp_hw_get_scaler_coef(vratio);

	/* TODO : scale up case */
	rgbp_hw_s_h_init_phase_offset(base, set_id, h_phase_offset);
	rgbp_hw_s_v_init_phase_offset(base, set_id, v_phase_offset);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_upsc_coef);

void rgbp_hw_s_gamma_enable(void *base, u32 set_id, u32 enable, u32 bypass)
{
	RGBP_SET_F(base, RGBP_R_Y_GAMMALR_BYPASS, RGBP_F_Y_GAMMALR_BYPASS, bypass);
	RGBP_SET_F(base, RGBP_R_Y_GAMMAHR_BYPASS, RGBP_F_Y_GAMMAHR_BYPASS, bypass);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_gamma_enable);

void rgbp_hw_s_decomp_enable(void *base, u32 set_id, u32 enable, u32 bypass)
{
	RGBP_SET_F(base, RGBP_R_Y_DECOMP_BYPASS, RGBP_F_Y_DECOMP_BYPASS, bypass);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_decomp_enable);

void rgbp_hw_s_decomp_size(void *base, u32 set_id, u32 h_size, u32 v_size)
{
	RGBP_SET_F(base, RGBP_R_Y_DECOMP_FRAME_SIZE, RGBP_F_Y_DECOMP_FRAME_WIDTH, h_size);
	RGBP_SET_F(base, RGBP_R_Y_DECOMP_FRAME_SIZE, RGBP_F_Y_DECOMP_FRAME_HEIGHT, v_size);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_decomp_size);

void rgbp_hw_s_grid_cfg(void *base, struct rgbp_grid_cfg *cfg)
{
	u32 val;

	val = 0;
	val = RGBP_SET_V(base, val, RGBP_F_RGB_LUMAADAPLSC_BINNING_X, cfg->binning_x);
	val = RGBP_SET_V(base, val, RGBP_F_RGB_LUMAADAPLSC_BINNING_Y, cfg->binning_y);
	RGBP_SET_R(base, RGBP_R_RGB_LUMAADAPLSC_BINNING, val);

	RGBP_SET_F(base, RGBP_R_RGB_LUMAADAPLSC_CROP_START_X,
			RGBP_F_RGB_LUMAADAPLSC_CROP_START_X, cfg->crop_x);
	RGBP_SET_F(base, RGBP_R_RGB_LUMAADAPLSC_CROP_START_Y,
			RGBP_F_RGB_LUMAADAPLSC_CROP_START_Y, cfg->crop_y);

	val = 0;
	val = RGBP_SET_V(base, val, RGBP_F_RGB_LUMAADAPLSC_CROP_RADIAL_X, cfg->crop_radial_x);
	val = RGBP_SET_V(base, val, RGBP_F_RGB_LUMAADAPLSC_CROP_RADIAL_Y, cfg->crop_radial_y);
	RGBP_SET_R(base, RGBP_R_RGB_LUMAADAPLSC_CROP_START_REAL, val);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_grid_cfg);

void rgbp_hw_s_votf(void *base, u32 set_id, bool enable, bool stall)
{
	u32 val;

	val = stall << 1;
	val = val | enable;
	RGBP_SET_F(base, RGBP_R_STAT_WDMADECOMP_VOTF_EN, RGBP_F_STAT_WDMADECOMP_VOTF_EN, val);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_votf);

void rgbp_hw_s_sbwc(void *base, u32 set_id, bool enable)
{
	RGBP_SET_F(base, RGBP_R_YUV_SBWC_CTRL, RGBP_F_YUV_SBWC_EN, enable);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_sbwc);

void rgbp_hw_s_yuvsc_crop(void *base, bool enable, int out_x, int out_y, int out_width, int out_height)
{
	RGBP_SET_F(base, RGBP_R_DMA_CROP_CTRL, RGBP_F_YUV_SCCROP_BYPASS, !enable);

	RGBP_SET_F(base, RGBP_R_YUV_SCCROP_START, RGBP_F_YUV_SCCROP_START_X, out_x);
	RGBP_SET_F(base, RGBP_R_YUV_SCCROP_START, RGBP_F_YUV_SCCROP_START_Y, out_y);

	RGBP_SET_F(base, RGBP_R_YUV_SCCROP_SIZE, RGBP_F_YUV_SCCROP_SIZE_X, out_width);
	RGBP_SET_F(base, RGBP_R_YUV_SCCROP_SIZE, RGBP_F_YUV_SCCROP_SIZE_Y, out_height);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_yuvsc_crop);

u32 rgbp_hw_g_rdma_max_cnt(void)
{
	return RGBP_RDMA_MAX;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_rdma_max_cnt);

u32 rgbp_hw_g_wdma_max_cnt(void)
{
	return RGBP_WDMA_MAX;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_wdma_max_cnt);

u32 rgbp_hw_g_reg_cnt(void)
{
	return RGBP_REG_CNT + RGBP_LUT_REG_CNT;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_reg_cnt);

u32 rgbp_hw_g_rdma_cfg_max_cnt(void)
{
	return RGBP_RDMA_CFG_MAX;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_rdma_cfg_max_cnt);

u32 rgbp_hw_g_wdma_cfg_max_cnt(void)
{
	return RGBP_WDMA_CFG_MAX;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_wdma_cfg_max_cnt);

void rgbp_hw_update_param(struct rgbp_param *src, IS_DECLARE_PMAP(pmap),
	struct rgbp_param_set *dst)
{
	/* check input */
	if (test_bit(PARAM_RGBP_OTF_INPUT, pmap))
		memcpy(&dst->otf_input, &src->otf_input,
				sizeof(struct param_otf_input));

	/* check output */
	if (test_bit(PARAM_RGBP_OTF_OUTPUT, pmap))
		memcpy(&dst->otf_output, &src->otf_output,
				sizeof(struct param_otf_output));

	if (test_bit(PARAM_RGBP_YUV, pmap))
		memcpy(&dst->dma_output_yuv, &src->yuv,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_RGBP_HF, pmap))
		memcpy(&dst->dma_output_hf, &src->hf,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_RGBP_SF, pmap))
		memcpy(&dst->dma_output_sf, &src->sf,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_RGBP_STRIPE_INPUT, pmap))
		memcpy(&dst->stripe_input, &src->stripe_input,
				sizeof(struct param_stripe_input));

	if (test_bit(PARAM_RGBP_DMA_INPUT, pmap))
		memcpy(&dst->dma_input, &src->dma_input,
			sizeof(struct param_dma_input));

	if (test_bit(PARAM_RGBP_RGB, pmap))
		memcpy(&dst->dma_output_rgb, &src->rgb,
			sizeof(struct param_dma_output));
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_update_param);

int rgbp_hw_g_rdma_param_ptr(u32 id, struct is_frame *dma_frame, struct rgbp_param_set *param_set,
	char *name, dma_addr_t **dma_frame_dva, struct param_dma_input **pdi,
	pdma_addr_t **param_set_dva)
{
	int ret = 0;

	switch (id) {
	case RGBP_RDMA_CFG_IMG:
		*dma_frame_dva = dma_frame->dvaddr_buffer;
		*pdi = &param_set->dma_input;
		*param_set_dva = param_set->input_dva;
		sprintf(name, "rgbp");
		break;
	default:
		ret = -EINVAL;
		err_hw("[RGBP] invalid rdma param id[%d]", id);
		break;
	}

	return ret;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_rdma_param_ptr);

int rgbp_hw_g_wdma_param_ptr(u32 id, struct is_frame *dma_frame, struct rgbp_param_set *param_set,
	char *name, dma_addr_t **dma_frame_dva, struct param_dma_output **pdo,
	pdma_addr_t **param_set_dva)
{
	int ret = 0;

	switch (id) {
	case RGBP_WDMA_CFG_HF:
		*dma_frame_dva = dma_frame->dva_rgbp_hf;
		*pdo = &param_set->dma_output_hf;
		*param_set_dva = param_set->output_dva_hf;
		sprintf(name, "rgbphf");
		break;
	case RGBP_WDMA_CFG_YUV:
		*dma_frame_dva = dma_frame->dva_rgbp_yuv;
		*pdo = &param_set->dma_output_yuv;
		*param_set_dva = param_set->output_dva_yuv;
		sprintf(name, "rgbpyuv");
		break;
	case RGBP_WDMA_CFG_RGB:
		*dma_frame_dva = dma_frame->dva_rgbp_rgb;
		*pdo = &param_set->dma_output_rgb;
		*param_set_dva = param_set->output_dva_rgb;
		sprintf(name, "rgbprgb");
		break;
	default:
		ret = -EINVAL;
		err_hw("[RGBP] invalid wdma param id[%d]", id);
		break;
	}

	return ret;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_wdma_param_ptr);

void rgbp_hw_s_strgen(void *base, u32 set_id)
{
	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_CONFIG, RGBP_F_BYR_CINFIFO_STRGEN_MODE_EN, 1);
	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_CONFIG, RGBP_F_BYR_CINFIFO_STRGEN_MODE_DATA_TYPE, 1);
	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_CONFIG, RGBP_F_BYR_CINFIFO_STRGEN_MODE_DATA, 255);

	RGBP_SET_F(base, RGBP_R_IP_USE_OTF_PATH, RGBP_F_IP_USE_OTF_IN_FOR_PATH_0, 1);
	RGBP_SET_F(base, RGBP_R_IP_USE_CINFIFO_NEW_FRAME_IN, RGBP_F_IP_USE_CINFIFO_NEW_FRAME_IN, 0x0);

	RGBP_SET_F(base, RGBP_R_BYR_CINFIFO_ENABLE, RGBP_F_BYR_CINFIFO_ENABLE, 1);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_s_strgen);

u32 rgbp_hw_g_hf_mode(struct rgbp_param_set *param_set)
{
	if (param_set->dma_output_hf.cmd == DMA_OUTPUT_VOTF_ENABLE)
		return HF_DMA;

	return HF_OFF;
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_g_hf_mode);

void rgbp_hw_init_pmio_config(struct pmio_config *cfg)
{
	cfg->num_corexs = 2;
	cfg->corex_stride = 0x8000;

	cfg->volatile_table = &rgbp_volatile_ranges_table;
	cfg->wr_noinc_table = &rgbp_wr_noinc_ranges_table;

	cfg->max_register = RGBP_R_RGB_GAMMA_STREAM_CRC;
	cfg->num_reg_defaults_raw = (RGBP_R_RGB_GAMMA_STREAM_CRC >> 2) + 1;
	cfg->dma_addr_shift = 4;

	cfg->ranges = rgbp_range_cfgs;
	cfg->num_ranges = ARRAY_SIZE(rgbp_range_cfgs);

	cfg->fields = rgbp_field_descs;
	cfg->num_fields = ARRAY_SIZE(rgbp_field_descs);
}
KUNIT_EXPORT_SYMBOL(rgbp_hw_init_pmio_config);

