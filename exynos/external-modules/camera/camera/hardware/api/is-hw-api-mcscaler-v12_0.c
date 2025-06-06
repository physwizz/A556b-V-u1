// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * MCSC HW control APIs
 *
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-hw-api-mcscaler-v4.h"
#if IS_ENABLED(MCSC_USE_MMIO)
#include "sfr/is-sfr-mcsc-mmio-v12_0.h"
#else
#if IS_ENABLED(CONFIG_PABLO_V12_0_0)
#include "sfr/is-sfr-mcsc-v12_0.h"
#else
#include "sfr/is-sfr-mcsc-v12_1.h"
#endif
#endif
#include "is-hw.h"
#include "is-hw-control.h"
#include "is-param.h"
#include "is-hw-common-dma.h"
#include "pmio.h"

#define DMA_36BIT_UPPER(x)	((x) >> 4)
#define MCSC_SBWC_MAX_WIDTH 10496

const struct mcsc_v_coef v_coef_4tap[7] = {
	/* x8/8 */
	{
		{  0, -15, -25, -31, -33, -33, -31, -27, -23},
		{512, 508, 495, 473, 443, 408, 367, 324, 279},
		{  0,  20,  45,  75, 110, 148, 190, 234, 279},
		{  0,  -1,  -3,  -5,  -8, -11, -14, -19, -23},
	},
	/* x7/8 */
	{
		{ 32,  17,   3,  -7, -14, -18, -20, -20, -19},
		{448, 446, 437, 421, 399, 373, 343, 310, 275},
		{ 32,  55,  79, 107, 138, 170, 204, 240, 275},
		{  0,  -6,  -7,  -9, -11, -13, -15, -18, -19},
	},
	/* x6/8 */
	{
		{ 61,  46,  31,  19,   9,   2,  -3,  -7,  -9},
		{390, 390, 383, 371, 356, 337, 315, 291, 265},
		{ 61,  83, 106, 130, 156, 183, 210, 238, 265},
		{  0,  -7,  -8,  -8,  -9, -10, -10, -10,  -9},
	},
	/* x5/8 */
	{
		{ 85,  71,  56,  43,  32,  23,  16,   9,   5},
		{341, 341, 336, 328, 317, 304, 288, 271, 251},
		{ 86, 105, 124, 145, 166, 187, 209, 231, 251},
		{  0,  -5,  -4,  -4,  -3,  -2,  -1,   1,   5},
	},
	/* x4/8 */
	{
		{104,  89,  76,  63,  52,  42,  33,  26,  20},
		{304, 302, 298, 293, 285, 275, 264, 251, 236},
		{104, 120, 136, 153, 170, 188, 205, 221, 236},
		{  0,   1,   2,   3,   5,   7,  10,  14,  20},
	},
	/* x3/8 */
	{
		{118, 103,  90,  78,  67,  57,  48,  40,  33},
		{276, 273, 270, 266, 260, 253, 244, 234, 223},
		{118, 129, 143, 157, 171, 185, 199, 211, 223},
		{  0,   7,   9,  11,  14,  17,  21,  27,  33},
	},
	/* x2/8 */
	{
		{127, 111, 100,  88,  78,  68,  59,  50,  43},
		{258, 252, 250, 247, 242, 237, 230, 222, 213},
		{127, 135, 147, 159, 171, 182, 193, 204, 213},
		{  0,  14,  15,  18,  21,  25,  30,  36,  43},
	}
};

const struct mcsc_h_coef h_coef_8tap[7] = {
	/* x8/8 */
	{
		{  0,  -2,  -4,  -5,  -6,  -6,  -6,  -6,  -5},
		{  0,   8,  14,  20,  23,  25,  26,  25,  23},
		{  0, -25, -46, -62, -73, -80, -83, -82, -78},
		{512, 509, 499, 482, 458, 429, 395, 357, 316},
		{  0,  30,  64, 101, 142, 185, 228, 273, 316},
		{  0,  -9, -19, -30, -41, -53, -63, -71, -78},
		{  0,   2,   5,   8,  12,  15,  19,  21,  23},
		{  0,  -1,  -1,  -2,  -3,  -3,  -4,  -5,  -5},
	},
	/* x7/8 */
	{
		{ 12,   9,   7,   5,   3,   2,   1,   0,  -1},
		{-32, -24, -16,  -9,  -3,   2,   7,  10,  13},
		{ 56,  29,   6, -14, -30, -43, -53, -60, -65},
		{444, 445, 438, 426, 410, 390, 365, 338, 309},
		{ 52,  82, 112, 144, 177, 211, 244, 277, 309},
		{-32, -39, -46, -52, -58, -63, -66, -66, -65},
		{ 12,  13,  14,  15,  16,  16,  16,  15,  13},
		{  0,  -3,  -3,  -3,  -3,  -3,  -2,  -2,  -1},
	},
	/* x6/8 */
	{
		{  8,   9,   8,   8,   8,   7,   7,   5,   5},
		{-44, -40, -36, -32, -27, -22, -18, -13,  -9},
		{100,  77,  57,  38,  20,   5,  -9, -20, -30},
		{384, 382, 377, 369, 358, 344, 329, 310, 290},
		{100, 123, 147, 171, 196, 221, 245, 268, 290},
		{-44, -47, -49, -49, -48, -47, -43, -37, -30},
		{  8,   8,   7,   5,   3,   1,  -2,  -5,  -9},
		{  0,   0,   1,   2,   2,   3,   3,   4,   5},
	},
	/* x5/8 */
	{
		{ -3,  -3,  -1,   0,   1,   2,   2,   3,   3},
		{-31, -32, -33, -32, -31, -30, -28, -25, -23},
		{130, 113,  97,  81,  66,  52,  38,  26,  15},
		{320, 319, 315, 311, 304, 296, 286, 274, 261},
		{130, 147, 165, 182, 199, 216, 232, 247, 261},
		{-31, -29, -26, -22, -17, -11,  -3,   5,  15},
		{ -3,  -6,  -8, -11, -13, -16, -18, -21, -23},
		{  0,   3,   3,   3,   3,   3,   3,   3,   3},
	},
	/* x4/8 */
	{
		{-11, -10,  -9,  -8,  -7,  -6,  -5,  -5,  -4},
		{  0,  -4,  -7, -10, -12, -14, -15, -16, -17},
		{140, 129, 117, 106,  95,  85,  74,  64,  55},
		{255, 254, 253, 250, 246, 241, 236, 229, 222},
		{140, 151, 163, 174, 185, 195, 204, 214, 222},
		{  0,   5,  10,  16,  22,  29,  37,  46,  55},
		{-12, -13, -14, -15, -16, -16, -17, -17, -17},
		{  0,   0,  -1,  -1,  -1,  -2,  -2,  -3,  -4},
	},
	/* x3/8 */
	{
		{ -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5},
		{ 31,  27,  23,  19,  16,  12,  10,   7,   5},
		{133, 126, 119, 112, 105,  98,  91,  84,  78},
		{195, 195, 194, 193, 191, 189, 185, 182, 178},
		{133, 139, 146, 152, 158, 163, 169, 174, 178},
		{ 31,  37,  41,  47,  53,  59,  65,  71,  78},
		{ -6,  -4,  -3,  -2,  -2,   0,   1,   3,   5},
		{  0,  -3,  -3,  -4,  -4,  -4,  -4,  -4,  -5},
	},
	/* x2/8 */
	{
		{ 10,   9,   7,   6,   5,   4,   4,   3,   2},
		{ 52,  48,  45,  41,  38,  35,  31,  29,  26},
		{118, 114, 110, 106, 102,  98,  94,  89,  85},
		{152, 152, 151, 150, 149, 148, 146, 145, 143},
		{118, 122, 125, 129, 132, 135, 138, 140, 143},
		{ 52,  56,  60,  64,  68,  72,  77,  81,  85},
		{ 10,  11,  13,  15,  17,  19,  21,  23,  26},
		{  0,   0,   1,   1,   1,   1,   1,   2,   2},
	}
};

const struct mcsc_v_coef uv_v_coef_4tap[7] = {
	/* x8/8 */
	{
		{  0, -15, -25, -31, -33, -33, -31, -27, -23},
		{512, 508, 495, 473, 443, 408, 367, 324, 279},
		{  0,  20,  45,  75, 110, 148, 190, 234, 279},
		{  0,  -1,  -3,  -5,  -8, -11, -14, -19, -23},
	},
	/* x7/8 */
	{
		{ 32,  17,   3,  -7, -14, -18, -20, -20, -19},
		{448, 446, 437, 421, 399, 373, 343, 310, 275},
		{ 32,  55,  79, 107, 138, 170, 204, 240, 275},
		{  0,  -6,  -7,  -9, -11, -13, -15, -18, -19},
	},
	/* x6/8 */
	{
		{ 61,  46,  31,  19,   9,   2,  -3,  -7,  -9},
		{390, 390, 383, 371, 356, 337, 315, 291, 265},
		{ 61,  83, 106, 130, 156, 183, 210, 238, 265},
		{  0,  -7,  -8,  -8,  -9, -10, -10, -10,  -9},
	},
	/* x5/8 */
	{
		{ 85,  71,  56,  43,  32,  23,  16,   9,   5},
		{341, 341, 336, 328, 317, 304, 288, 271, 251},
		{ 86, 105, 124, 145, 166, 187, 209, 231, 251},
		{  0,  -5,  -4,  -4,  -3,  -2,  -1,   1,   5},
	},
	/* x4/8 */
	{
		{104,  89,  76,  63,  52,  42,  33,  26,  20},
		{304, 302, 298, 293, 285, 275, 264, 251, 236},
		{104, 120, 136, 153, 170, 188, 205, 221, 236},
		{  0,   1,   2,   3,   5,   7,  10,  14,  20},
	},
	/* x3/8 */
	{
		{118, 103,  90,  78,  67,  57,  48,  40,  33},
		{276, 273, 270, 266, 260, 253, 244, 234, 223},
		{118, 129, 143, 157, 171, 185, 199, 211, 223},
		{  0,   7,   9,  11,  14,  17,  21,  27,  33},
	},
	/* x2/8 */
	{
		{127, 111, 100,  88,  78,  68,  59,  50,  43},
		{258, 252, 250, 247, 242, 237, 230, 222, 213},
		{127, 135, 147, 159, 171, 182, 193, 204, 213},
		{  0,  14,  15,  18,  21,  25,  30,  36,  43},
	}
};

const struct mcsc_h_coef uv_h_coef_4tap[7] = {
	/* x8/8 */
	{
		{  0, -15, -25, -31, -33, -33, -31, -27, -23},
		{512, 508, 495, 473, 443, 408, 367, 324, 279},
		{  0,  20,  45,  75, 110, 148, 190, 234, 279},
		{  0,  -1,  -3,  -5,  -8, -11, -14, -19, -23},
	},
	/* x7/8 */
	{
		{ 32,  17,   3,  -7, -14, -18, -20, -20, -19},
		{448, 446, 437, 421, 399, 373, 343, 310, 275},
		{ 32,  55,  79, 107, 138, 170, 204, 240, 275},
		{  0,  -6,  -7,  -9, -11, -13, -15, -18, -19},
	},
	/* x6/8 */
	{
		{ 61,  46,  31,  19,   9,   2,  -3,  -7,  -9},
		{390, 390, 383, 371, 356, 337, 315, 291, 265},
		{ 61,  83, 106, 130, 156, 183, 210, 238, 265},
		{  0,  -7,  -8,  -8,  -9, -10, -10, -10,  -9},
	},
	/* x5/8 */
	{
		{ 85,  71,  56,  43,  32,  23,  16,   9,   5},
		{341, 341, 336, 328, 317, 304, 288, 271, 251},
		{ 86, 105, 124, 145, 166, 187, 209, 231, 251},
		{  0,  -5,  -4,  -4,  -3,  -2,  -1,   1,   5},
	},
	/* x4/8 */
	{
		{104,  89,  76,  63,  52,  42,  33,  26,  20},
		{304, 302, 298, 293, 285, 275, 264, 251, 236},
		{104, 120, 136, 153, 170, 188, 205, 221, 236},
		{  0,   1,   2,   3,   5,   7,  10,  14,  20},
	},
	/* x3/8 */
	{
		{118, 103,  90,  78,  67,  57,  48,  40,  33},
		{276, 273, 270, 266, 260, 253, 244, 234, 223},
		{118, 129, 143, 157, 171, 185, 199, 211, 223},
		{  0,   7,   9,  11,  14,  17,  21,  27,  33},
	},
	/* x2/8 */
	{
		{127, 111, 100,  88,  78,  68,  59,  50,  43},
		{258, 252, 250, 247, 242, 237, 230, 222, 213},
		{127, 135, 147, 159, 171, 182, 193, 204, 213},
		{  0,  14,  15,  18,  21,  25,  30,  36,  43},
	}
};

static void is_scaler_add_to_queue(struct pablo_mmio *base, u32 queue_num)
{
	switch (queue_num) {
	case MCSC_QUEUE0:
		MCSC_SET_F(base, MCSC_R_CMDQ_ADD_TO_QUEUE_0,
			MCSC_F_CMDQ_ADD_TO_QUEUE_0, 0x1);
		break;
	case MCSC_QUEUE1:
		MCSC_SET_F(base, MCSC_R_CMDQ_ADD_TO_QUEUE_1,
			MCSC_F_CMDQ_ADD_TO_QUEUE_1, 0x1);
		break;
	case MCSC_QUEUE_URGENT:
		MCSC_SET_F(base, MCSC_R_CMDQ_ADD_TO_QUEUE_URGENT,
			MCSC_F_CMDQ_ADD_TO_QUEUE_URGENT, 0x1);
		break;
	default:
		warn_hw("invalid queue num(%d) for MCSC api\n", queue_num);
		break;
	}
}

static void is_scaler_set_mode(struct pablo_mmio *base, u32 mode)
{
	u32 val;

	MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_H, MCSC_F_CMDQ_QUE_CMD_BASE_ADDR, 0x0);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_CMDQ_QUE_CMD_HEADER_NUM, 0x0);
	val = MCSC_SET_V(base, val, MCSC_F_CMDQ_QUE_CMD_SETTING_MODE, mode);
	val = MCSC_SET_V(base, val, MCSC_F_CMDQ_QUE_CMD_HOLD_MODE, 0x0);
	val = MCSC_SET_V(base, val, MCSC_F_CMDQ_QUE_CMD_FRAME_ID, 0x0);
	MCSC_SET_R(base, MCSC_R_CMDQ_QUE_CMD_M, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE, INT_GRP_MCSC_EN_MASK);
	val = MCSC_SET_V(base, val, MCSC_F_CMDQ_QUE_CMD_FRO_INDEX, 0x0);
	MCSC_SET_R(base, MCSC_R_CMDQ_QUE_CMD_L, val);
}

static void is_scaler_set_cinfifo_ctrl(struct pablo_mmio *base, bool strgen)
{
	MCSC_SET_R(base, MCSC_R_IP_USE_OTF_PATH_01, 0x1);
	MCSC_SET_F(base, MCSC_R_YUV_CINFIFO_ENABLE,
		MCSC_F_YUV_CINFIFO_ENABLE, 0x1);
	MCSC_SET_F(base, MCSC_R_YUV_CINFIFO_INTERVALS,
		MCSC_F_YUV_CINFIFO_INTERVAL_HBLANK, HBLANK_CYCLE);
	MCSC_SET_F(base, MCSC_R_YUV_CINFIFO_CONFIG,
		MCSC_F_YUV_CINFIFO_STALL_BEFORE_FRAME_START_EN, 0x1);
	MCSC_SET_F(base, MCSC_R_YUV_CINFIFO_CONFIG, MCSC_F_YUV_CINFIFO_DEBUG_EN, 0x1);
	MCSC_SET_F(base, MCSC_R_IP_USE_CINFIFO_NEW_FRAME_IN,
		MCSC_F_IP_USE_CINFIFO_NEW_FRAME_IN, 0x1);

	if (strgen) {
		MCSC_SET_F(base, MCSC_R_YUV_CINFIFO_CONFIG, MCSC_F_YUV_CINFIFO_STRGEN_MODE_EN, 0x1);
		 /* incremetal */
		MCSC_SET_F(base, MCSC_R_YUV_CINFIFO_CONFIG, MCSC_F_YUV_CINFIFO_STRGEN_MODE_DATA_TYPE, 0x1);
		MCSC_SET_F(base, MCSC_R_YUV_CINFIFO_CONFIG, MCSC_F_YUV_CINFIFO_STRGEN_MODE_DATA, 255);
		MCSC_SET_F(base, MCSC_R_IP_USE_CINFIFO_NEW_FRAME_IN, MCSC_F_IP_USE_CINFIFO_NEW_FRAME_IN, 0x0);
	}
}

void is_scaler_set_hf_cinfifo_ctrl(struct pablo_mmio *base, u32 en, bool strgen)
{
#if IS_ENABLED(CONFIG_PABLO_V12_1_0)
	MCSC_SET_F(base, MCSC_R_IP_USE_OTF_PATH_01,
			MCSC_F_IP_USE_OTF_IN_FOR_PATH_1, en);
	MCSC_SET_F(base, MCSC_R_STAT_CINFIFOHF_ENABLE,
		MCSC_F_STAT_CINFIFOHF_ENABLE, en);
	MCSC_SET_F(base, MCSC_R_STAT_CINFIFOHF_INTERVALS,
		MCSC_F_STAT_CINFIFOHF_INTERVAL_HBLANK, HBLANK_CYCLE);
	MCSC_SET_R(base, MCSC_R_STAT_CINFIFOHF_CONFIG, 0x1);

	if (strgen) {
		MCSC_SET_F(base, MCSC_R_STAT_CINFIFOHF_CONFIG, MCSC_F_STAT_CINFIFOHF_STRGEN_MODE_EN, 0x1);
		 /* incremetal */
		MCSC_SET_F(base, MCSC_R_STAT_CINFIFOHF_CONFIG, MCSC_F_STAT_CINFIFOHF_STRGEN_MODE_DATA_TYPE, 0x1);
		MCSC_SET_F(base, MCSC_R_STAT_CINFIFOHF_CONFIG, MCSC_F_STAT_CINFIFOHF_STRGEN_MODE_DATA, 255);
	}
#endif
}

static void is_hw_set_crc(struct pablo_mmio *base, u32 seed)
{
	MCSC_SET_F(base, MCSC_R_YUV_CINFIFO_STREAM_CRC,
		MCSC_F_YUV_CINFIFO_CRC_SEED, seed);
	MCSC_SET_F(base, MCSC_R_YUV_MAIN_CTRL_CRC_EN,
		MCSC_F_YUV_CRC_SEED, seed);

#if IS_ENABLED(CONFIG_PABLO_V12_1_0)
	MCSC_SET_F(base, MCSC_R_STAT_CINFIFOHF_STREAM_CRC,
		MCSC_F_STAT_CINFIFOHF_CRC_SEED, seed);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGRECOMP_STREAM_CRC,
		MCSC_F_YUV_DJAGRECOMP_CRC_SEED, seed);
	MCSC_SET_F(base, MCSC_R_YUV_DTPOTFIN_STREAM_CRC,
		MCSC_F_YUV_DTPOTFIN_CRC_SEED, seed);
	MCSC_SET_F(base, MCSC_R_YUV_DTPOTFHFIN_STREAM_CRC,
		MCSC_F_YUV_DTPOTFHFIN_CRC_SEED, seed);
#endif
}

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
void pablo_kunit_scaler_hw_set_crc(struct pablo_mmio *base, u32 seed)
{
	is_hw_set_crc(base, seed);
}
KUNIT_EXPORT_SYMBOL(pablo_kunit_scaler_hw_set_crc);
#endif

void is_scaler_start(struct pablo_mmio *base, u32 hw_id, u32 num_buffers, dma_addr_t clh, u32 noh)
{
	int i;
	u32 fro_en = num_buffers > 1 ? 1 : 0;
	u32 seed;

	seed = is_get_debug_param(IS_DEBUG_PARAM_CRC_SEED);
	if (unlikely(seed))
		is_hw_set_crc(base, seed);

	/* cmdq control : 3(apb direct) */
	is_scaler_set_mode(base, 0x3);

	if (fro_en)
		MCSC_SET_F(base, MCSC_R_CMDQ_ENABLE, MCSC_F_CMDQ_ENABLE, 0x0);

	for (i = 0; i < num_buffers; ++i) {
		if (i == 0)
			MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_L, MCSC_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE,
					fro_en ? INT_GRP_MCSC_EN_MASK_FRO_FIRST : INT_GRP_MCSC_EN_MASK);
		else if (i < num_buffers - 1)
			MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_L, MCSC_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE,
					INT_GRP_MCSC_EN_MASK_FRO_MIDDLE);
		else
			MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_L, MCSC_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE,
					INT_GRP_MCSC_EN_MASK_FRO_LAST);

		if (clh && noh) {
			MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_H, MCSC_F_CMDQ_QUE_CMD_BASE_ADDR, DVA_36BIT_HIGH(clh));
			MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_M, MCSC_F_CMDQ_QUE_CMD_HEADER_NUM, noh);
			MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_M, MCSC_F_CMDQ_QUE_CMD_SETTING_MODE, 1);
		} else {
			MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_M, MCSC_F_CMDQ_QUE_CMD_SETTING_MODE, 3);
		}

		MCSC_SET_F(base, MCSC_R_CMDQ_QUE_CMD_L, MCSC_F_CMDQ_QUE_CMD_FRO_INDEX, i);
		is_scaler_add_to_queue(base, MCSC_QUEUE0);
	}

	MCSC_SET_F(base, MCSC_R_CMDQ_ENABLE, MCSC_F_CMDQ_ENABLE, 0x1);
}

void is_scaler_set_clock(struct pablo_mmio *base, bool on)
{
	dbg_hw(5, "[MCSC] clock %s", on ? "on" : "off");

	MCSC_SET_F(base, MCSC_R_IP_PROCESSING, MCSC_F_IP_PROCESSING, on);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_clock);

u32 is_scaler_sw_reset(struct pablo_mmio *base, u32 hw_id, u32 global, u32 partial)
{
	u32 reset_count = 0;

	MCSC_SET_F(base, MCSC_R_SW_RESET, MCSC_F_SW_RESET, 0x1);

	/* wait reset complete */
	do {
		reset_count++;
		if (reset_count > MCSC_TRY_COUNT)
			return reset_count;
	} while (MCSC_GET_F(base, MCSC_R_SW_RESET, MCSC_F_SW_RESET) != 0);

	return 0;
}

void is_scaler_s_init(struct pablo_mmio *base)
{
	MCSC_SET_F(base, MCSC_R_CMDQ_VHD_CONTROL,
			MCSC_F_CMDQ_VHD_STALL_ON_QSTOP_ENABLE, 1);
	MCSC_SET_F(base, MCSC_R_CMDQ_VHD_CONTROL,
			MCSC_F_CMDQ_VHD_VBLANK_QRUN_ENABLE, 1);

	if (!IS_ENABLED(MCSC_USE_MMIO)) {
		MCSC_SET_R(base, MCSC_R_C_LOADER_ENABLE, 1);
		MCSC_SET_R(base, MCSC_R_STAT_RDMACL_ENABLE, 1);
	}
}
KUNIT_EXPORT_SYMBOL(is_scaler_s_init);

void is_scaler_clear_intr_all(struct pablo_mmio *base, u32 hw_id)
{
	MCSC_SET_R(base, MCSC_R_INT_REQ_INT0_CLEAR, INT1_MCSC_EN_MASK);
	MCSC_SET_R(base, MCSC_R_INT_REQ_INT1_CLEAR, INT2_MCSC_EN_MASK);
}

void is_scaler_disable_intr(struct pablo_mmio *base, u32 hw_id)
{
	MCSC_SET_R(base, MCSC_R_INT_REQ_INT0_ENABLE, 0);
	MCSC_SET_R(base, MCSC_R_INT_REQ_INT1_ENABLE, 0);
}

void is_scaler_mask_intr(struct pablo_mmio *base, u32 hw_id, u32 intr_mask)
{
	MCSC_SET_R(base, MCSC_R_INT_REQ_INT0_ENABLE, intr_mask);
	MCSC_SET_R(base, MCSC_R_INT_REQ_INT1_ENABLE, INT2_MCSC_EN_MASK);
}

u32 is_scaler_get_mask_intr(void)
{
	return INT1_MCSC_EN_MASK;
}

unsigned int is_scaler_intr_occurred0(unsigned int status, enum mcsc_event_type type)
{
	u32 mask;

	switch (type) {
	case INTR_FRAME_START:
		mask = 1 << INTR1_MCSC_FRAME_START_INT;
		break;
	case INTR_FRAME_END:
		mask = 1 << INTR1_MCSC_FRAME_END_INT;
		break;
	case INTR_COREX_END_0:
		mask = 1 << INTR1_MCSC_COREX_END_INT_0;
		break;
	case INTR_COREX_END_1:
		mask = 1 << INTR1_MCSC_COREX_END_INT_1;
		break;
	case INTR_SETTING_DONE:
		mask = 1 << INTR1_MCSC_SETTING_DONE_INT;
		break;
	case INTR_ERR:
		mask = INT1_MCSC_ERR_MASK;
		break;
	default:
		return 0;
	}

	return status & mask;
}

void is_scaler_get_input_status(struct pablo_mmio *base, u32 hw_id, u32 *hl, u32 *vl)
{
	/* TODO */
	*hl = 0;
	*vl = 0;
}

void is_scaler_set_input_source(struct pablo_mmio *base, u32 hw_id, u32 rdma, bool strgen)
{
	/* 0: No input , 1: OTF input, 2: RDMA input */
	switch (rdma) {
	case OTF_INPUT:
		is_scaler_set_cinfifo_ctrl(base, strgen);
		break;
	case DMA_INPUT:
		err_hw("Not supported in this version, rdma (%d)", rdma);
		break;
	default:
		err_hw("No input source (%d)", rdma);
		break;
	}
}

u32 is_scaler_get_input_source(struct pablo_mmio *base, u32 hw_id)
{
	/* not support */
	return 0;
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_input_source);

void is_scaler_set_input_img_size(struct pablo_mmio *base, u32 hw_id, u32 width, u32 height)
{

	if (width % MCSC_WIDTH_ALIGN) {
		err_hw("INPUT_IMG_HSIZE_%d(%d) should be multiple of 2", hw_id, width);
		width = ALIGN_DOWN(width, MCSC_WIDTH_ALIGN);
	}

	MCSC_SET_F(base, MCSC_R_YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_WIDTH,
		MCSC_F_YUV_YUVP_IN_IMG_SZ_WIDTH, width);
	MCSC_SET_F(base, MCSC_R_YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_HEIGHT,
		MCSC_F_YUV_YUVP_IN_IMG_SZ_HEIGHT, height);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_input_img_size);

void is_scaler_get_input_img_size(struct pablo_mmio *base, u32 hw_id, u32 *width, u32 *height)
{
	*width = MCSC_GET_F(base, MCSC_R_YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_WIDTH,
			MCSC_F_YUV_YUVP_IN_IMG_SZ_WIDTH);
	*height = MCSC_GET_F(base, MCSC_R_YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_HEIGHT,
			MCSC_F_YUV_YUVP_IN_IMG_SZ_HEIGHT);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_input_img_size);

u32 is_scaler_get_scaler_path(struct pablo_mmio *base, u32 hw_id, u32 output_id)
{
	u32 enable_poly, enable_post, enable_dma;
	u32 sc_ctrl_reg, pc_ctrl_reg, wdma_enable_reg;
	u32 sc_enable_field, pc_enable_field, wdma_en_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC0_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC0_ENABLE;
		pc_ctrl_reg = MCSC_R_YUV_POST_PC0_CTRL;
		pc_enable_field = MCSC_F_YUV_POSTPC_PC0_ENABLE;
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W0_ENABLE;
		wdma_en_field = MCSC_F_YUV_WDMASC_W0_EN;
		break;
	case MCSC_OUTPUT1:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC1_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC1_ENABLE;
		pc_ctrl_reg = MCSC_R_YUV_POST_PC1_CTRL;
		pc_enable_field = MCSC_F_YUV_POSTPC_PC1_ENABLE;
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W1_ENABLE;
		wdma_en_field = MCSC_F_YUV_WDMASC_W1_EN;
		break;
	case MCSC_OUTPUT2:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC2_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC2_ENABLE;
		pc_ctrl_reg = MCSC_R_YUV_POST_PC2_CTRL;
		pc_enable_field = MCSC_F_YUV_POSTPC_PC2_ENABLE;
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W2_ENABLE;
		wdma_en_field = MCSC_F_YUV_WDMASC_W2_EN;
		break;
	case MCSC_OUTPUT3:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC3_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC3_ENABLE;
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W3_ENABLE;
		wdma_en_field = MCSC_F_YUV_WDMASC_W3_EN;
		break;
	case MCSC_OUTPUT4:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC4_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC4_ENABLE;
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W4_ENABLE;
		wdma_en_field = MCSC_F_YUV_WDMASC_W4_EN;
		break;
	default:
		return 0;
	}

	enable_poly = MCSC_GET_F(base, sc_ctrl_reg, sc_enable_field);

	if (output_id <= MCSC_OUTPUT2)
		enable_post = MCSC_GET_F(base, pc_ctrl_reg, pc_enable_field);
	else
		enable_post = 0;

	enable_dma = MCSC_GET_F(base, wdma_enable_reg, wdma_en_field);

	dbg_hw(2, "[ID:%d]%s:[OUT:%d], en(poly:%d, post:%d), dma(%d)\n",
		hw_id, __func__, output_id, enable_poly, enable_post, enable_dma);

	return DEV_HW_MCSC0;
}

void is_scaler_set_poly_scaler_enable(struct pablo_mmio *base, u32 hw_id, u32 output_id, u32 enable)
{
	u32 sc_ctrl_reg, sc_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC0_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC0_ENABLE;
		break;
	case MCSC_OUTPUT1:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC1_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC1_ENABLE;
		break;
	case MCSC_OUTPUT2:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC2_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC2_ENABLE;
		break;
	case MCSC_OUTPUT3:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC3_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC3_ENABLE;
		break;
	case MCSC_OUTPUT4:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC4_CTRL;
		sc_enable_field = MCSC_F_YUV_POLY_SC4_ENABLE;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, sc_ctrl_reg, sc_enable_field, enable);
}

void is_scaler_set_poly_src_size(struct pablo_mmio *base, u32 output_id,
	u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	u32 val;
	u32 sc_src_hpos_field, sc_src_vpos_field;
	u32 sc_src_hsize_field, sc_src_vsize_field;
	u32 sc_src_pos_reg, sc_src_size_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_src_hpos_field = MCSC_F_YUV_POLY_SC0_SRC_HPOS;
		sc_src_vpos_field = MCSC_F_YUV_POLY_SC0_SRC_VPOS;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC0_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC0_SRC_VSIZE;
		sc_src_pos_reg = MCSC_R_YUV_POLY_SC0_SRC_POS;
		sc_src_size_reg = MCSC_R_YUV_POLY_SC0_SRC_SIZE;
		break;
	case MCSC_OUTPUT1:
		sc_src_hpos_field = MCSC_F_YUV_POLY_SC1_SRC_HPOS;
		sc_src_vpos_field = MCSC_F_YUV_POLY_SC1_SRC_VPOS;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC1_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC1_SRC_VSIZE;
		sc_src_pos_reg = MCSC_R_YUV_POLY_SC1_SRC_POS;
		sc_src_size_reg = MCSC_R_YUV_POLY_SC1_SRC_SIZE;
		break;
	case MCSC_OUTPUT2:
		sc_src_hpos_field = MCSC_F_YUV_POLY_SC2_SRC_HPOS;
		sc_src_vpos_field = MCSC_F_YUV_POLY_SC2_SRC_VPOS;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC2_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC2_SRC_VSIZE;
		sc_src_pos_reg = MCSC_R_YUV_POLY_SC2_SRC_POS;
		sc_src_size_reg = MCSC_R_YUV_POLY_SC2_SRC_SIZE;
		break;
	case MCSC_OUTPUT3:
		sc_src_hpos_field = MCSC_F_YUV_POLY_SC3_SRC_HPOS;
		sc_src_vpos_field = MCSC_F_YUV_POLY_SC3_SRC_VPOS;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC3_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC3_SRC_VSIZE;
		sc_src_pos_reg = MCSC_R_YUV_POLY_SC3_SRC_POS;
		sc_src_size_reg = MCSC_R_YUV_POLY_SC3_SRC_SIZE;
		break;
	case MCSC_OUTPUT4:
		sc_src_hpos_field = MCSC_F_YUV_POLY_SC4_SRC_HPOS;
		sc_src_vpos_field = MCSC_F_YUV_POLY_SC4_SRC_VPOS;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC4_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC4_SRC_VSIZE;
		sc_src_pos_reg = MCSC_R_YUV_POLY_SC4_SRC_POS;
		sc_src_size_reg = MCSC_R_YUV_POLY_SC4_SRC_SIZE;
		break;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, sc_src_hpos_field, pos_x);
	val = MCSC_SET_V(base, val, sc_src_vpos_field, pos_y);
	MCSC_SET_R(base, sc_src_pos_reg, val);

	val = 0;
	val = MCSC_SET_V(base, val, sc_src_hsize_field, width);
	val = MCSC_SET_V(base, val, sc_src_vsize_field, height);
	MCSC_SET_R(base, sc_src_size_reg, val);
}

void is_scaler_get_poly_src_size(struct pablo_mmio *base, u32 output_id, u32 *width, u32 *height)
{
	u32 sc_src_size_reg;
	u32 sc_src_hsize_field, sc_src_vsize_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_src_size_reg = MCSC_R_YUV_POLY_SC0_SRC_SIZE;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC0_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC0_SRC_VSIZE;
		break;
	case MCSC_OUTPUT1:
		sc_src_size_reg = MCSC_R_YUV_POLY_SC1_SRC_SIZE;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC1_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC1_SRC_VSIZE;
		break;
	case MCSC_OUTPUT2:
		sc_src_size_reg = MCSC_R_YUV_POLY_SC2_SRC_SIZE;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC2_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC2_SRC_VSIZE;
		break;
	case MCSC_OUTPUT3:
		sc_src_size_reg = MCSC_R_YUV_POLY_SC3_SRC_SIZE;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC3_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC3_SRC_VSIZE;
		break;
	case MCSC_OUTPUT4:
		sc_src_size_reg = MCSC_R_YUV_POLY_SC4_SRC_SIZE;
		sc_src_hsize_field = MCSC_F_YUV_POLY_SC4_SRC_HSIZE;
		sc_src_vsize_field = MCSC_F_YUV_POLY_SC4_SRC_VSIZE;
		break;
	default:
		*width = 0;
		*height = 0;
		return;
	}

	*width = MCSC_GET_F(base, sc_src_size_reg, sc_src_hsize_field);
	*height = MCSC_GET_F(base, sc_src_size_reg, sc_src_vsize_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_poly_src_size);

void is_scaler_set_poly_dst_size(struct pablo_mmio *base, u32 output_id, u32 width, u32 height)
{
	u32 val;
	u32 sc_dst_hsize_field, sc_dst_vsize_field;
	u32 sc_dst_size_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC0_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC0_DST_VSIZE;
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC0_DST_SIZE;
		break;
	case MCSC_OUTPUT1:
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC1_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC1_DST_VSIZE;
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC1_DST_SIZE;
		break;
	case MCSC_OUTPUT2:
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC2_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC2_DST_VSIZE;
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC2_DST_SIZE;
		break;
	case MCSC_OUTPUT3:
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC3_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC3_DST_VSIZE;
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC3_DST_SIZE;
		break;
	case MCSC_OUTPUT4:
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC4_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC4_DST_VSIZE;
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC4_DST_SIZE;
		break;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, sc_dst_hsize_field, width);
	val = MCSC_SET_V(base, val, sc_dst_vsize_field, height);
	MCSC_SET_R(base, sc_dst_size_reg, val);
}

void is_scaler_get_poly_dst_size(struct pablo_mmio *base, u32 output_id, u32 *width, u32 *height)
{
	u32 sc_dst_size_reg;
	u32 sc_dst_hsize_field, sc_dst_vsize_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC0_DST_SIZE;
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC0_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC0_DST_VSIZE;
		break;
	case MCSC_OUTPUT1:
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC1_DST_SIZE;
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC1_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC1_DST_VSIZE;
		break;
	case MCSC_OUTPUT2:
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC2_DST_SIZE;
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC2_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC2_DST_VSIZE;
		break;
	case MCSC_OUTPUT3:
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC3_DST_SIZE;
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC3_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC3_DST_VSIZE;
		break;
	case MCSC_OUTPUT4:
		sc_dst_size_reg = MCSC_R_YUV_POLY_SC4_DST_SIZE;
		sc_dst_hsize_field = MCSC_F_YUV_POLY_SC4_DST_HSIZE;
		sc_dst_vsize_field = MCSC_F_YUV_POLY_SC4_DST_VSIZE;
		break;
	default:
		*width = 0;
		*height = 0;
		return;
	}

	*width = MCSC_GET_F(base, sc_dst_size_reg, sc_dst_hsize_field);
	*height = MCSC_GET_F(base, sc_dst_size_reg, sc_dst_vsize_field);
}

void is_scaler_set_poly_scaling_ratio(struct pablo_mmio *base, u32 output_id, u32 hratio, u32 vratio)
{
	u32 sc_h_ratio_reg, sc_v_ratio_reg;
	u32 sc_h_ratio_field, sc_v_ratio_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_h_ratio_reg = MCSC_R_YUV_POLY_SC0_H_RATIO;
		sc_v_ratio_reg = MCSC_R_YUV_POLY_SC0_V_RATIO;
		sc_h_ratio_field = MCSC_F_YUV_POLY_SC0_H_RATIO;
		sc_v_ratio_field = MCSC_F_YUV_POLY_SC0_V_RATIO;
		break;
	case MCSC_OUTPUT1:
		sc_h_ratio_reg = MCSC_R_YUV_POLY_SC1_H_RATIO;
		sc_v_ratio_reg = MCSC_R_YUV_POLY_SC1_V_RATIO;
		sc_h_ratio_field = MCSC_F_YUV_POLY_SC1_H_RATIO;
		sc_v_ratio_field = MCSC_F_YUV_POLY_SC1_V_RATIO;
		break;
	case MCSC_OUTPUT2:
		sc_h_ratio_reg = MCSC_R_YUV_POLY_SC2_H_RATIO;
		sc_v_ratio_reg = MCSC_R_YUV_POLY_SC2_V_RATIO;
		sc_h_ratio_field = MCSC_F_YUV_POLY_SC2_H_RATIO;
		sc_v_ratio_field = MCSC_F_YUV_POLY_SC2_V_RATIO;
		break;
	case MCSC_OUTPUT3:
		sc_h_ratio_reg = MCSC_R_YUV_POLY_SC3_H_RATIO;
		sc_v_ratio_reg = MCSC_R_YUV_POLY_SC3_V_RATIO;
		sc_h_ratio_field = MCSC_F_YUV_POLY_SC3_H_RATIO;
		sc_v_ratio_field = MCSC_F_YUV_POLY_SC3_V_RATIO;
		break;
	case MCSC_OUTPUT4:
		sc_h_ratio_reg = MCSC_R_YUV_POLY_SC4_H_RATIO;
		sc_v_ratio_reg = MCSC_R_YUV_POLY_SC4_V_RATIO;
		sc_h_ratio_field = MCSC_F_YUV_POLY_SC4_H_RATIO;
		sc_v_ratio_field = MCSC_F_YUV_POLY_SC4_V_RATIO;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, sc_h_ratio_reg, sc_h_ratio_field, hratio);
	MCSC_SET_F(base, sc_v_ratio_reg, sc_v_ratio_field, vratio);
}

static void is_scaler_set_h_init_phase_offset(struct pablo_mmio *base,
	u32 output_id, u32 h_offset)
{
	u32 sc_h_init_phase_offset_reg, sc_h_init_phase_offset_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_h_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC0_H_INIT_PHASE_OFFSET;
		sc_h_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC0_H_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT1:
		sc_h_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC1_H_INIT_PHASE_OFFSET;
		sc_h_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC1_H_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT2:
		sc_h_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC2_H_INIT_PHASE_OFFSET;
		sc_h_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC2_H_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT3:
		sc_h_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC3_H_INIT_PHASE_OFFSET;
		sc_h_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC3_H_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT4:
		sc_h_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC4_H_INIT_PHASE_OFFSET;
		sc_h_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC4_H_INIT_PHASE_OFFSET;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, sc_h_init_phase_offset_reg,
		sc_h_init_phase_offset_field, h_offset);
}

static void is_scaler_set_v_init_phase_offset(struct pablo_mmio *base,
	u32 output_id, u32 v_offset)
{
	u32 sc_v_init_phase_offset_reg, sc_v_init_phase_offset_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_v_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC0_V_INIT_PHASE_OFFSET;
		sc_v_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC0_V_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT1:
		sc_v_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC1_V_INIT_PHASE_OFFSET;
		sc_v_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC1_V_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT2:
		sc_v_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC2_V_INIT_PHASE_OFFSET;
		sc_v_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC2_V_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT3:
		sc_v_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC3_V_INIT_PHASE_OFFSET;
		sc_v_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC3_V_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT4:
		sc_v_init_phase_offset_reg =
			MCSC_R_YUV_POLY_SC4_V_INIT_PHASE_OFFSET;
		sc_v_init_phase_offset_field =
			MCSC_F_YUV_POLY_SC4_V_INIT_PHASE_OFFSET;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, sc_v_init_phase_offset_reg,
		sc_v_init_phase_offset_field, v_offset);
}

static void __mcsc_hw_s_horizontal_coeff(struct pablo_mmio *base,
	struct mcsc_h_coef *h_coef_cfg, int plane,
	u32 sc_coeff_reg, u32 sc_coeff_field, u32 reg_offset, u32 field_offset)
{
	int index;
	u32 val;
	u32 sc_coeff_01_reg = sc_coeff_reg;
	u32 sc_coeff_23_reg = sc_coeff_01_reg + REG_OFFSET;
	u32 sc_coeff_45_reg = sc_coeff_23_reg + REG_OFFSET;
	u32 sc_coeff_67_reg = sc_coeff_45_reg + REG_OFFSET;
	u32 sc_coeff_0246_field = sc_coeff_field;
	u32 sc_coeff_1357_field = sc_coeff_field + FISRT_TAP;

	/* Y horizontal : 8-tap filter */
	/* UV horizontal : 4-tap filter */
	for (index = 0; index <= 8; index++) {
		val = 0;
		val = MCSC_SET_V(base, val, sc_coeff_0246_field + (field_offset * index),
			h_coef_cfg->h_coef_a[index]);
		val = MCSC_SET_V(base, val, sc_coeff_1357_field + (field_offset * index),
			h_coef_cfg->h_coef_b[index]);
		MCSC_SET_R(base, sc_coeff_01_reg + (reg_offset * index), val);

		val = 0;
		val = MCSC_SET_V(base, val, sc_coeff_0246_field + (field_offset * index) + SECOND_TAP,
			h_coef_cfg->h_coef_c[index]);
		val = MCSC_SET_V(base, val, sc_coeff_1357_field + (field_offset * index) + SECOND_TAP,
			h_coef_cfg->h_coef_d[index]);
		MCSC_SET_R(base, sc_coeff_23_reg + (reg_offset * index), val);

		if (!plane) {
			val = 0;
			val = MCSC_SET_V(base, val, sc_coeff_0246_field + (field_offset * index) + FOURTH_TAP,
				h_coef_cfg->h_coef_e[index]);
			val = MCSC_SET_V(base, val, sc_coeff_1357_field + (field_offset * index) + FOURTH_TAP,
				h_coef_cfg->h_coef_f[index]);
			MCSC_SET_R(base, sc_coeff_45_reg + (reg_offset * index), val);

			val = 0;
			val = MCSC_SET_V(base, val, sc_coeff_0246_field + (field_offset * index) + SIXTH_TAP,
				h_coef_cfg->h_coef_g[index]);
			val = MCSC_SET_V(base, val, sc_coeff_1357_field + (field_offset * index) + SIXTH_TAP,
				h_coef_cfg->h_coef_h[index]);
			MCSC_SET_R(base, sc_coeff_67_reg + (reg_offset * index), val);
		}
	}
}

static void __mcsc_hw_s_vertical_coeff(struct pablo_mmio *base,
	struct mcsc_v_coef *v_coef_cfg,
	u32 sc_coeff_reg, u32 sc_coeff_field, u32 reg_offset, u32 field_offset)
{
	int index;
	u32 val;
	u32 sc_coeff_01_reg = sc_coeff_reg;
	u32 sc_coeff_23_reg = sc_coeff_reg + REG_OFFSET;
	u32 sc_coeff_0246_field = sc_coeff_field;
	u32 sc_coeff_1357_field = sc_coeff_field + FISRT_TAP;

	/* Y & UV vertical : 4-tap filter */
	for (index = 0; index <= 8; index++) {
		val = 0;
		val = MCSC_SET_V(base, val, sc_coeff_0246_field + (field_offset * index),
			v_coef_cfg->v_coef_a[index]);
		val = MCSC_SET_V(base, val, sc_coeff_1357_field + (field_offset * index),
			v_coef_cfg->v_coef_b[index]);
		MCSC_SET_R(base, sc_coeff_01_reg + (reg_offset * index), val);

		val = 0;
		val = MCSC_SET_V(base, val, sc_coeff_0246_field + (field_offset * index) + SECOND_TAP,
			v_coef_cfg->v_coef_c[index]);
		val = MCSC_SET_V(base, val, sc_coeff_1357_field + (field_offset * index) + SECOND_TAP,
			v_coef_cfg->v_coef_d[index]);
		MCSC_SET_R(base, sc_coeff_23_reg + (reg_offset * index), val);
	}
}

static void is_scaler_set_poly_scaler_h_coef(struct pablo_mmio *base,
	u32 output_id, struct mcsc_h_coef *h_coef_cfg, int plane)
{
	u32 sc_coeff_reg;
	u32 sc_coeff_field;
	u32 reg_offset, field_offset, scaler_offset, scaler_field_offset;

	if (output_id > MCSC_OUTPUT4) {
		err("invalid scaler ID");
		return;
	}
	if (!plane) {
		scaler_offset = MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_01
			- MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_01;
		sc_coeff_reg = MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_01
			+ output_id * scaler_offset;
		scaler_field_offset = MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_0
			- MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_0;
		sc_coeff_field = MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_0
			+ output_id * scaler_field_offset;
		reg_offset = REG_OFFSET * 4;
		field_offset = 8;
	} else if (plane == 1) {
		scaler_offset = MCSC_R_YUV_POLY_SC1_UV_H_COEFF_0_01
			- MCSC_R_YUV_POLY_SC0_UV_H_COEFF_0_01;
		sc_coeff_reg = MCSC_R_YUV_POLY_SC0_UV_H_COEFF_0_01
			+ output_id * scaler_offset;
		scaler_field_offset = MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_0
			- MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_0;
		sc_coeff_field = MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_0
			+ output_id * scaler_field_offset;
		reg_offset = IS_ENABLED(MCSC_USE_MMIO) ? (REG_OFFSET * 2) : (REG_OFFSET * 4);
		field_offset = 4;
	} else {
		err("invalid plane");
		return;
	}

	__mcsc_hw_s_horizontal_coeff(base, h_coef_cfg, plane,
		sc_coeff_reg, sc_coeff_field, reg_offset, field_offset);
}

static void is_scaler_set_poly_scaler_v_coef(struct pablo_mmio *base,
	u32 output_id, struct mcsc_v_coef *v_coef_cfg, int plane)
{
	u32 sc_coeff_reg;
	u32 sc_coeff_field = MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_0;
	u32 reg_offset = REG_OFFSET * 2, field_offset = 4, scaler_offset, scaler_field_offset;

	if (output_id > MCSC_OUTPUT4) {
		err("invalid scaler ID");
		return;
	}

	if (!plane) {
		scaler_offset = MCSC_R_YUV_POLY_SC1_Y_V_COEFF_0_01
			- MCSC_R_YUV_POLY_SC0_Y_V_COEFF_0_01;
		sc_coeff_reg = MCSC_R_YUV_POLY_SC0_Y_V_COEFF_0_01
			+ output_id * scaler_offset;
		scaler_field_offset = MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_0
			- MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_0;
		sc_coeff_field = MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_0
			+ output_id * scaler_field_offset;
	} else if (plane == 1) {
		scaler_offset = MCSC_R_YUV_POLY_SC1_UV_V_COEFF_0_01
			- MCSC_R_YUV_POLY_SC0_UV_V_COEFF_0_01;
		sc_coeff_reg = MCSC_R_YUV_POLY_SC0_UV_V_COEFF_0_01
			+ output_id * scaler_offset;
		scaler_field_offset = MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_0
			- MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_0;
		sc_coeff_field = MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_0
			+ output_id * scaler_field_offset;
	} else {
		err("invalid plane");
		return;
	}

	__mcsc_hw_s_vertical_coeff(base, v_coef_cfg,
		sc_coeff_reg, sc_coeff_field, reg_offset, field_offset);
}

static u32 get_scaler_coef_ver2(u32 ratio, struct scaler_filter_coef_cfg *sc_coef)
{
	u32 coef;

	if (ratio <= RATIO_X8_8)
		coef = sc_coef->ratio_x8_8;
	else if (ratio > RATIO_X8_8 && ratio <= RATIO_X7_8)
		coef = sc_coef->ratio_x7_8;
	else if (ratio > RATIO_X7_8 && ratio <= RATIO_X6_8)
		coef = sc_coef->ratio_x6_8;
	else if (ratio > RATIO_X6_8 && ratio <= RATIO_X5_8)
		coef = sc_coef->ratio_x5_8;
	else if (ratio > RATIO_X5_8 && ratio <= RATIO_X4_8)
		coef = sc_coef->ratio_x4_8;
	else if (ratio > RATIO_X4_8 && ratio <= RATIO_X3_8)
		coef = sc_coef->ratio_x3_8;
	else if (ratio > RATIO_X3_8 && ratio <= RATIO_X2_8)
		coef = sc_coef->ratio_x2_8;
	else
		coef = sc_coef->ratio_x2_8;

	return coef;
}

static void __is_scaler_set_poly_scaler_coef(struct pablo_mmio *base, u32 output_id,
	u32 hratio, u32 vratio, struct scaler_coef_cfg *sc_coef, int plane)
{
	struct mcsc_h_coef *h_coef = NULL;
	struct mcsc_v_coef *v_coef = NULL;
	u32 h_coef_idx, v_coef_idx;

	if (sc_coef->use_poly_sc_coef) {
		h_coef = (struct mcsc_h_coef *)&sc_coef->poly_sc_h_coef;
		v_coef = (struct mcsc_v_coef *)&sc_coef->poly_sc_v_coef;
	} else {
		h_coef_idx = get_scaler_coef_ver2(hratio, &sc_coef->poly_sc_coef[0]);
		v_coef_idx = get_scaler_coef_ver2(vratio, &sc_coef->poly_sc_coef[1]);
		if (!plane) {
			h_coef = (struct mcsc_h_coef *)&h_coef_8tap[h_coef_idx];
			v_coef = (struct mcsc_v_coef *)&v_coef_4tap[v_coef_idx];
		} else {
			h_coef = (struct mcsc_h_coef *)&uv_h_coef_4tap[h_coef_idx];
			v_coef = (struct mcsc_v_coef *)&uv_v_coef_4tap[v_coef_idx];
		}
	}

	is_scaler_set_poly_scaler_h_coef(base, output_id, h_coef, plane);
	is_scaler_set_poly_scaler_v_coef(base, output_id, v_coef, plane);
}

void is_scaler_set_poly_scaler_coef(struct pablo_mmio *base, u32 output_id,
	u32 hratio, u32 vratio, struct scaler_coef_cfg *sc_coefs[], int num_coef,
	enum exynos_sensor_position sensor_position)
{
	/* this value equals 0 - scale-down operation */
	u32 h_phase_offset = 0, v_phase_offset = 0;
	struct scaler_coef_cfg *sc_coef;
	int k;

	for (k = 0; k < num_coef; k++) {
		sc_coef = sc_coefs[k];
		if (!sc_coef) {
			err_hw("sc_coef[%d] is NULL", k);
			continue;
		}
		__is_scaler_set_poly_scaler_coef(base, output_id, hratio, vratio,
			sc_coef, k);
	}

	/* scale up case */
	if (hratio < RATIO_X8_8)
		h_phase_offset = hratio >> 1;
	if (vratio < RATIO_X8_8)
		v_phase_offset = vratio >> 1;

	is_scaler_set_h_init_phase_offset(base, output_id, h_phase_offset);
	is_scaler_set_v_init_phase_offset(base, output_id, v_phase_offset);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_poly_scaler_coef);

void is_scaler_set_poly_round_mode(struct pablo_mmio *base, u32 output_id, u32 mode)
{
	u32 sc_round_mode_reg, sc_round_mode_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_round_mode_reg = MCSC_R_YUV_POLY_SC0_ROUND_MODE;
		sc_round_mode_field = MCSC_F_YUV_POLY_SC0_ROUND_MODE;
		break;
	case MCSC_OUTPUT1:
		sc_round_mode_reg = MCSC_R_YUV_POLY_SC1_ROUND_MODE;
		sc_round_mode_field = MCSC_F_YUV_POLY_SC1_ROUND_MODE;
		break;
	case MCSC_OUTPUT2:
		sc_round_mode_reg = MCSC_R_YUV_POLY_SC2_ROUND_MODE;
		sc_round_mode_field = MCSC_F_YUV_POLY_SC2_ROUND_MODE;
		break;
	case MCSC_OUTPUT3:
		sc_round_mode_reg = MCSC_R_YUV_POLY_SC3_ROUND_MODE;
		sc_round_mode_field = MCSC_F_YUV_POLY_SC3_ROUND_MODE;
		break;
	case MCSC_OUTPUT4:
		sc_round_mode_reg = MCSC_R_YUV_POLY_SC4_ROUND_MODE;
		sc_round_mode_field = MCSC_F_YUV_POLY_SC4_ROUND_MODE;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, sc_round_mode_reg, sc_round_mode_field, mode);
}

void is_scaler_set_post_scaler_enable(struct pablo_mmio *base, u32 output_id, u32 enable)
{
	u32 pc_ctrl_reg, pc_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC0_CTRL;
		pc_enable_field = MCSC_F_YUV_POSTPC_PC0_ENABLE;
		break;
	case MCSC_OUTPUT1:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC1_CTRL;
		pc_enable_field = MCSC_F_YUV_POSTPC_PC1_ENABLE;
		break;
	case MCSC_OUTPUT2:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC2_CTRL;
		pc_enable_field = MCSC_F_YUV_POSTPC_PC2_ENABLE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, pc_ctrl_reg, pc_enable_field, enable);
}

void is_scaler_set_post_img_size(struct pablo_mmio *base, u32 output_id, u32 width, u32 height)
{
	u32 val;
	u32 pc_img_hsize_field, pc_img_vsize_field;
	u32 pc_img_size_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_img_hsize_field = MCSC_F_YUV_POSTPC_PC0_IMG_HSIZE;
		pc_img_vsize_field = MCSC_F_YUV_POSTPC_PC0_IMG_VSIZE;
		pc_img_size_reg = MCSC_R_YUV_POST_PC0_IMG_SIZE;
		break;
	case MCSC_OUTPUT1:
		pc_img_hsize_field = MCSC_F_YUV_POSTPC_PC1_IMG_HSIZE;
		pc_img_vsize_field = MCSC_F_YUV_POSTPC_PC1_IMG_VSIZE;
		pc_img_size_reg = MCSC_R_YUV_POST_PC1_IMG_SIZE;
		break;
	case MCSC_OUTPUT2:
		pc_img_hsize_field = MCSC_F_YUV_POSTPC_PC2_IMG_HSIZE;
		pc_img_vsize_field = MCSC_F_YUV_POSTPC_PC2_IMG_VSIZE;
		pc_img_size_reg = MCSC_R_YUV_POST_PC2_IMG_SIZE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, pc_img_hsize_field, width);
	val = MCSC_SET_V(base, val, pc_img_vsize_field, height);
	MCSC_SET_R(base, pc_img_size_reg, val);
}

void is_scaler_get_post_img_size(struct pablo_mmio *base, u32 output_id, u32 *width, u32 *height)
{
	u32 pc_img_hsize_field, pc_img_vsize_field;
	u32 pc_img_size_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_img_hsize_field = MCSC_F_YUV_POSTPC_PC0_IMG_HSIZE;
		pc_img_vsize_field = MCSC_F_YUV_POSTPC_PC0_IMG_VSIZE;
		pc_img_size_reg = MCSC_R_YUV_POST_PC0_IMG_SIZE;
		break;
	case MCSC_OUTPUT1:
		pc_img_hsize_field = MCSC_F_YUV_POSTPC_PC1_IMG_HSIZE;
		pc_img_vsize_field = MCSC_F_YUV_POSTPC_PC1_IMG_VSIZE;
		pc_img_size_reg = MCSC_R_YUV_POST_PC1_IMG_SIZE;
		break;
	case MCSC_OUTPUT2:
		pc_img_hsize_field = MCSC_F_YUV_POSTPC_PC2_IMG_HSIZE;
		pc_img_vsize_field = MCSC_F_YUV_POSTPC_PC2_IMG_VSIZE;
		pc_img_size_reg = MCSC_R_YUV_POST_PC2_IMG_SIZE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		*width = 0;
		*height = 0;
		return;
	}

	*width = MCSC_GET_F(base, pc_img_size_reg, pc_img_hsize_field);
	*height = MCSC_GET_F(base, pc_img_size_reg, pc_img_vsize_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_post_img_size);

void is_scaler_set_post_dst_size(struct pablo_mmio *base, u32 output_id, u32 width, u32 height)
{
	u32 val;
	u32 pc_dst_hsize_field, pc_dst_vsize_field;
	u32 pc_dst_size_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_dst_hsize_field = MCSC_F_YUV_POSTPC_PC0_DST_HSIZE;
		pc_dst_vsize_field = MCSC_F_YUV_POSTPC_PC0_DST_VSIZE;
		pc_dst_size_reg = MCSC_R_YUV_POST_PC0_DST_SIZE;
		break;
	case MCSC_OUTPUT1:
		pc_dst_hsize_field = MCSC_F_YUV_POSTPC_PC1_DST_HSIZE;
		pc_dst_vsize_field = MCSC_F_YUV_POSTPC_PC1_DST_VSIZE;
		pc_dst_size_reg = MCSC_R_YUV_POST_PC1_DST_SIZE;
		break;
	case MCSC_OUTPUT2:
		pc_dst_hsize_field = MCSC_F_YUV_POSTPC_PC2_DST_HSIZE;
		pc_dst_vsize_field = MCSC_F_YUV_POSTPC_PC2_DST_VSIZE;
		pc_dst_size_reg = MCSC_R_YUV_POST_PC2_DST_SIZE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, pc_dst_hsize_field, width);
	val = MCSC_SET_V(base, val, pc_dst_vsize_field, height);
	MCSC_SET_R(base, pc_dst_size_reg, val);
}

void is_scaler_get_post_dst_size(struct pablo_mmio *base, u32 output_id, u32 *width, u32 *height)
{
	u32 pc_dst_hsize_field, pc_dst_vsize_field;
	u32 pc_dst_size_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_dst_hsize_field = MCSC_F_YUV_POSTPC_PC0_DST_HSIZE;
		pc_dst_vsize_field = MCSC_F_YUV_POSTPC_PC0_DST_VSIZE;
		pc_dst_size_reg = MCSC_R_YUV_POST_PC0_DST_SIZE;
		break;
	case MCSC_OUTPUT1:
		pc_dst_hsize_field = MCSC_F_YUV_POSTPC_PC1_DST_HSIZE;
		pc_dst_vsize_field = MCSC_F_YUV_POSTPC_PC1_DST_VSIZE;
		pc_dst_size_reg = MCSC_R_YUV_POST_PC1_DST_SIZE;
		break;
	case MCSC_OUTPUT2:
		pc_dst_hsize_field = MCSC_F_YUV_POSTPC_PC2_DST_HSIZE;
		pc_dst_vsize_field = MCSC_F_YUV_POSTPC_PC2_DST_VSIZE;
		pc_dst_size_reg = MCSC_R_YUV_POST_PC2_DST_SIZE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		*width = 0;
		*height = 0;
		return;
	}

	*width = MCSC_GET_F(base, pc_dst_size_reg, pc_dst_hsize_field);
	*height = MCSC_GET_F(base, pc_dst_size_reg, pc_dst_vsize_field);
}

void is_scaler_set_post_scaling_ratio(struct pablo_mmio *base, u32 output_id, u32 hratio, u32 vratio)
{
	u32 pc_h_ratio_field, pc_v_ratio_field;
	u32 pc_h_ratio_reg, pc_v_ratio_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_h_ratio_field = MCSC_F_YUV_POSTPC_PC0_H_RATIO;
		pc_h_ratio_reg = MCSC_R_YUV_POSTPC_PC0_H_RATIO;
		pc_v_ratio_field = MCSC_F_YUV_POSTPC_PC0_V_RATIO;
		pc_v_ratio_reg = MCSC_R_YUV_POSTPC_PC0_V_RATIO;
		break;
	case MCSC_OUTPUT1:
		pc_h_ratio_field = MCSC_F_YUV_POSTPC_PC1_H_RATIO;
		pc_h_ratio_reg = MCSC_R_YUV_POSTPC_PC1_H_RATIO;
		pc_v_ratio_field = MCSC_F_YUV_POSTPC_PC1_V_RATIO;
		pc_v_ratio_reg = MCSC_R_YUV_POSTPC_PC1_V_RATIO;
		break;
	case MCSC_OUTPUT2:
		pc_h_ratio_field = MCSC_F_YUV_POSTPC_PC2_H_RATIO;
		pc_h_ratio_reg = MCSC_R_YUV_POSTPC_PC2_H_RATIO;
		pc_v_ratio_field = MCSC_F_YUV_POSTPC_PC2_V_RATIO;
		pc_v_ratio_reg = MCSC_R_YUV_POSTPC_PC2_V_RATIO;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, pc_h_ratio_reg, pc_h_ratio_field, hratio);
	MCSC_SET_F(base, pc_v_ratio_reg, pc_v_ratio_field, vratio);
}

static void is_scaler_set_post_h_init_phase_offset(struct pablo_mmio *base,
	u32 output_id, u32 h_offset)
{
	u32 pc_h_init_phase_offset_reg, pc_h_init_phase_offset_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_h_init_phase_offset_reg =
			MCSC_R_YUV_POSTPC_PC0_H_INIT_PHASE_OFFSET;
		pc_h_init_phase_offset_field =
			MCSC_F_YUV_POSTPC_PC0_H_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT1:
		pc_h_init_phase_offset_reg =
			MCSC_R_YUV_POSTPC_PC1_H_INIT_PHASE_OFFSET;
		pc_h_init_phase_offset_field =
			MCSC_F_YUV_POSTPC_PC1_H_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT2:
		pc_h_init_phase_offset_reg =
			MCSC_R_YUV_POSTPC_PC2_H_INIT_PHASE_OFFSET;
		pc_h_init_phase_offset_field =
			MCSC_F_YUV_POSTPC_PC2_H_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, pc_h_init_phase_offset_reg, pc_h_init_phase_offset_field, h_offset);
}

static void is_scaler_set_post_v_init_phase_offset(struct pablo_mmio *base,
	u32 output_id, u32 v_offset)
{
	u32 pc_v_init_phase_offset_reg, pc_v_init_phase_offset_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_v_init_phase_offset_reg =
			MCSC_R_YUV_POSTPC_PC0_V_INIT_PHASE_OFFSET;
		pc_v_init_phase_offset_field =
			MCSC_F_YUV_POSTPC_PC0_V_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT1:
		pc_v_init_phase_offset_reg =
			MCSC_R_YUV_POSTPC_PC1_V_INIT_PHASE_OFFSET;
		pc_v_init_phase_offset_field =
			MCSC_F_YUV_POSTPC_PC1_V_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT2:
		pc_v_init_phase_offset_reg =
			MCSC_R_YUV_POSTPC_PC2_V_INIT_PHASE_OFFSET;
		pc_v_init_phase_offset_field =
			MCSC_F_YUV_POSTPC_PC2_V_INIT_PHASE_OFFSET;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, pc_v_init_phase_offset_reg, pc_v_init_phase_offset_field, v_offset);
}

static void is_scaler_set_post_scaler_h_coef(struct pablo_mmio *base_addr,
	u32 output_id, struct mcsc_h_coef *h_coef_cfg, int plane)
{
	u32 sc_coeff_reg;
	u32 sc_coeff_field = MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_0;
	u32 reg_offset, field_offset, scaler_offset;

	if (output_id > MCSC_OUTPUT2) {
		err("invalid scaler ID");
		return;
	}
	if (!plane) {
		scaler_offset = MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_01
			- MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_01;
		sc_coeff_reg = MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_01
			+ output_id * scaler_offset;
		reg_offset = REG_OFFSET * 4;
		field_offset = 8;
	} else if (plane == 1) {
		scaler_offset = MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_0_01
			- MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_0_01;
		sc_coeff_reg = MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_0_01
			+ output_id * scaler_offset;
		reg_offset = IS_ENABLED(MCSC_USE_MMIO) ? (REG_OFFSET * 2) : (REG_OFFSET * 4);
		field_offset = 4;
	} else {
		err("invalid plane");
		return;
	}

	__mcsc_hw_s_horizontal_coeff(base_addr, h_coef_cfg, plane,
		sc_coeff_reg, sc_coeff_field, reg_offset, field_offset);
}

static void is_scaler_set_post_scaler_v_coef(struct pablo_mmio *base_addr,
	u32 output_id, struct mcsc_v_coef *v_coef_cfg, int plane)
{
	u32 sc_coeff_reg;
	u32 sc_coeff_field = MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_0;
	u32 reg_offset = REG_OFFSET * 2, field_offset = 4, scaler_offset;

	if (output_id > MCSC_OUTPUT2) {
		err("invalid scaler ID");
		return;
	}

	if (!plane) {
		scaler_offset = MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_0_01
			- MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_0_01;
		sc_coeff_reg = MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_0_01
			+ output_id * scaler_offset;
	} else if (plane == 1) {
		scaler_offset = MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_0_01
			- MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_0_01;
		sc_coeff_reg = MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_0_01
			+ output_id * scaler_offset;
	} else {
		err("invalid plane");
		return;
	}

	__mcsc_hw_s_vertical_coeff(base_addr, v_coef_cfg,
		sc_coeff_reg, sc_coeff_field, reg_offset, field_offset);
}

static void __is_scaler_set_post_scaler_coef(struct pablo_mmio *base_addr, u32 output_id,
	u32 hratio, u32 vratio, struct scaler_coef_cfg *sc_coef, int plane)
{
	struct mcsc_h_coef *h_coef = NULL;
	struct mcsc_v_coef *v_coef = NULL;
	u32 h_coef_idx, v_coef_idx;

	h_coef_idx = get_scaler_coef_ver2(hratio, &sc_coef->post_sc_coef[0]);
	v_coef_idx = get_scaler_coef_ver2(vratio, &sc_coef->post_sc_coef[1]);
	if (!plane) {
		h_coef = (struct mcsc_h_coef *)&h_coef_8tap[h_coef_idx];
		v_coef = (struct mcsc_v_coef *)&v_coef_4tap[v_coef_idx];
	} else {
		h_coef = (struct mcsc_h_coef *)&uv_h_coef_4tap[h_coef_idx];
		v_coef = (struct mcsc_v_coef *)&uv_v_coef_4tap[v_coef_idx];
	}
	is_scaler_set_post_scaler_h_coef(base_addr, output_id, h_coef, plane);
	is_scaler_set_post_scaler_v_coef(base_addr, output_id, v_coef, plane);
}

void is_scaler_set_post_scaler_coef(struct pablo_mmio *base_addr, u32 output_id,
	u32 hratio, u32 vratio,
	struct scaler_coef_cfg *sc_coefs[], int num_coef)
{
	/* this value equals 0 : scale-down operation */
	u32 h_phase_offset = 0, v_phase_offset = 0;
	struct scaler_coef_cfg *sc_coef;
	int k;

	for (k = 0; k < num_coef; k++) {
		sc_coef = sc_coefs[k];
		if (!sc_coef) {
			err_hw("sc_coef[%d] is NULL", k);
			continue;
		}
		__is_scaler_set_post_scaler_coef(base_addr, output_id, hratio, vratio,
			sc_coef, k);
	}

	/* scale up case */
	if (hratio < RATIO_X8_8)
		h_phase_offset = hratio >> 1;
	if (vratio < RATIO_X8_8)
		v_phase_offset = vratio >> 1;

	is_scaler_set_post_h_init_phase_offset(base_addr, output_id, h_phase_offset);
	is_scaler_set_post_v_init_phase_offset(base_addr, output_id, v_phase_offset);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_post_scaler_coef);

void is_scaler_set_post_round_mode(struct pablo_mmio *base, u32 output_id, u32 mode)
{
	u32 pc_round_mode_reg, pc_reound_mode_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_round_mode_reg = MCSC_R_YUV_POSTPC_PC0_ROUND_MODE;
		pc_reound_mode_field = MCSC_F_YUV_POSTPC_PC0_ROUND_MODE;
		break;
	case MCSC_OUTPUT1:
		pc_round_mode_reg = MCSC_R_YUV_POSTPC_PC1_ROUND_MODE;
		pc_reound_mode_field = MCSC_F_YUV_POSTPC_PC1_ROUND_MODE;
		break;
	case MCSC_OUTPUT2:
		pc_round_mode_reg = MCSC_R_YUV_POSTPC_PC2_ROUND_MODE;
		pc_reound_mode_field = MCSC_F_YUV_POSTPC_PC2_ROUND_MODE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, pc_round_mode_reg, pc_reound_mode_field, mode);
}

void is_scaler_set_420_conversion(struct pablo_mmio *base, u32 output_id, u32 conv420_weight, u32 conv420_en)
{
	u32 val;
	u32 pc_conv420_enable;
	u32 pc_conv420_ctrl_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_conv420_enable = MCSC_F_YUV_POSTCONV_PC0_CONV420_ENABLE;
		pc_conv420_ctrl_reg = MCSC_R_YUV_POST_PC0_CONV420_CTRL;
		break;
	case MCSC_OUTPUT1:
		pc_conv420_enable = MCSC_F_YUV_POSTCONV_PC1_CONV420_ENABLE;
		pc_conv420_ctrl_reg = MCSC_R_YUV_POST_PC1_CONV420_CTRL;
		break;
	case MCSC_OUTPUT2:
		pc_conv420_enable = MCSC_F_YUV_POSTCONV_PC2_CONV420_ENABLE;
		pc_conv420_ctrl_reg = MCSC_R_YUV_POST_PC2_CONV420_CTRL;
		break;
	case MCSC_OUTPUT3:
		pc_conv420_enable = MCSC_F_YUV_POSTCONV_PC3_CONV420_ENABLE;
		pc_conv420_ctrl_reg = MCSC_R_YUV_POST_PC3_CONV420_CTRL;
		break;
	case MCSC_OUTPUT4:
		pc_conv420_enable = MCSC_F_YUV_POSTCONV_PC4_CONV420_ENABLE;
		pc_conv420_ctrl_reg = MCSC_R_YUV_POST_PC4_CONV420_CTRL;
		break;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val,pc_conv420_enable, conv420_en);
	MCSC_SET_R(base, pc_conv420_ctrl_reg, val);
}

void is_scaler_set_bchs_enable(struct pablo_mmio *base, u32 output_id, bool bchs_en)
{
	u32 pc_bchs_ctrl_reg, pc_bchs_enable_field;
	u32 pc_bchs_clamp_y_reg, pc_bchs_clamp_c_reg;
	u32 pc_bchs_clamp_y_field = 0x03FF0000;
	u32 pc_bchs_clamp_c_field = 0x03FF0000;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_bchs_ctrl_reg = MCSC_R_YUV_POST_PC0_BCHS_CTRL;
		pc_bchs_enable_field = MCSC_F_YUV_POSTBCHS_PC0_BCHS_ENABLE;
		pc_bchs_clamp_y_reg = MCSC_R_YUV_POST_PC0_BCHS_CLAMP_Y;
		pc_bchs_clamp_c_reg = MCSC_R_YUV_POST_PC0_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT1:
		pc_bchs_ctrl_reg = MCSC_R_YUV_POST_PC1_BCHS_CTRL;
		pc_bchs_enable_field = MCSC_F_YUV_POSTBCHS_PC1_BCHS_ENABLE;
		pc_bchs_clamp_y_reg = MCSC_R_YUV_POST_PC1_BCHS_CLAMP_Y;
		pc_bchs_clamp_c_reg = MCSC_R_YUV_POST_PC1_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT2:
		pc_bchs_ctrl_reg = MCSC_R_YUV_POST_PC2_BCHS_CTRL;
		pc_bchs_enable_field = MCSC_F_YUV_POSTBCHS_PC2_BCHS_ENABLE;
		pc_bchs_clamp_y_reg = MCSC_R_YUV_POST_PC2_BCHS_CLAMP_Y;
		pc_bchs_clamp_c_reg = MCSC_R_YUV_POST_PC2_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT3:
		pc_bchs_ctrl_reg = MCSC_R_YUV_POST_PC3_BCHS_CTRL;
		pc_bchs_enable_field = MCSC_F_YUV_POSTBCHS_PC3_BCHS_ENABLE;
		pc_bchs_clamp_y_reg = MCSC_R_YUV_POST_PC3_BCHS_CLAMP_Y;
		pc_bchs_clamp_c_reg = MCSC_R_YUV_POST_PC3_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT4:
		pc_bchs_ctrl_reg = MCSC_R_YUV_POST_PC4_BCHS_CTRL;
		pc_bchs_enable_field = MCSC_F_YUV_POSTBCHS_PC4_BCHS_ENABLE;
		pc_bchs_clamp_y_reg = MCSC_R_YUV_POST_PC4_BCHS_CLAMP_Y;
		pc_bchs_clamp_c_reg = MCSC_R_YUV_POST_PC4_BCHS_CLAMP_C;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, pc_bchs_ctrl_reg, pc_bchs_enable_field, bchs_en);
	/* default BCHS clamp value */
	MCSC_SET_R(base, pc_bchs_clamp_y_reg, pc_bchs_clamp_y_field);
	MCSC_SET_R(base, pc_bchs_clamp_c_reg, pc_bchs_clamp_c_field);
}

/* brightness/contrast control */
void is_scaler_set_b_c(struct pablo_mmio *base, u32 output_id, u32 y_offset, u32 y_gain)
{
	u32 val;
	u32 pc_bchs_y_offset_field, pc_bchs_y_gain_field;
	u32 pc_bchs_bc_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_bchs_y_offset_field = MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_OFFSET;
		pc_bchs_y_gain_field = MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_GAIN;
		pc_bchs_bc_reg = MCSC_R_YUV_POST_PC0_BCHS_BC;
		break;
	case MCSC_OUTPUT1:
		pc_bchs_y_offset_field = MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_OFFSET;
		pc_bchs_y_gain_field = MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_GAIN;
		pc_bchs_bc_reg = MCSC_R_YUV_POST_PC1_BCHS_BC;
		break;
	case MCSC_OUTPUT2:
		pc_bchs_y_offset_field = MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_OFFSET;
		pc_bchs_y_gain_field = MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_GAIN;
		pc_bchs_bc_reg = MCSC_R_YUV_POST_PC2_BCHS_BC;
		break;
	case MCSC_OUTPUT3:
		pc_bchs_y_offset_field = MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_OFFSET;
		pc_bchs_y_gain_field = MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_GAIN;
		pc_bchs_bc_reg = MCSC_R_YUV_POST_PC3_BCHS_BC;
		break;
	case MCSC_OUTPUT4:
		pc_bchs_y_offset_field = MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_OFFSET;
		pc_bchs_y_gain_field = MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_GAIN;
		pc_bchs_bc_reg = MCSC_R_YUV_POST_PC4_BCHS_BC;
		break;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, pc_bchs_y_offset_field, y_offset);
	val = MCSC_SET_V(base, val, pc_bchs_y_gain_field, y_gain);
	MCSC_SET_R(base, pc_bchs_bc_reg, val);
}

/* hue/saturation control */
void is_scaler_set_h_s(struct pablo_mmio *base, u32 output_id,
	u32 c_gain00, u32 c_gain01, u32 c_gain10, u32 c_gain11)
{
	u32 val;
	u32 pc_bchs_hs1_reg, pc_bchs_hs2_reg;
	u32 pc_bchs_c_gain_0_field = MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_00;
	u32 pc_bchs_c_gain_1_field = MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_01;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_bchs_hs1_reg = MCSC_R_YUV_POST_PC0_BCHS_HS1;
		pc_bchs_hs2_reg = MCSC_R_YUV_POST_PC0_BCHS_HS2;
		break;
	case MCSC_OUTPUT1:
		pc_bchs_hs1_reg = MCSC_R_YUV_POST_PC1_BCHS_HS1;
		pc_bchs_hs2_reg = MCSC_R_YUV_POST_PC1_BCHS_HS2;
		break;
	case MCSC_OUTPUT2:
		pc_bchs_hs1_reg = MCSC_R_YUV_POST_PC2_BCHS_HS1;
		pc_bchs_hs2_reg = MCSC_R_YUV_POST_PC2_BCHS_HS2;
		break;
	case MCSC_OUTPUT3:
		pc_bchs_hs1_reg = MCSC_R_YUV_POST_PC3_BCHS_HS1;
		pc_bchs_hs2_reg = MCSC_R_YUV_POST_PC3_BCHS_HS2;
		break;
	case MCSC_OUTPUT4:
		pc_bchs_hs1_reg = MCSC_R_YUV_POST_PC4_BCHS_HS1;
		pc_bchs_hs2_reg = MCSC_R_YUV_POST_PC4_BCHS_HS2;
		break;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, pc_bchs_c_gain_0_field, c_gain00);
	val = MCSC_SET_V(base, val, pc_bchs_c_gain_1_field, c_gain01);
	MCSC_SET_R(base, pc_bchs_hs1_reg, val);

	val = 0;
	val = MCSC_SET_V(base, val, pc_bchs_c_gain_0_field, c_gain10);
	val = MCSC_SET_V(base, val, pc_bchs_c_gain_1_field, c_gain11);
	MCSC_SET_R(base, pc_bchs_hs2_reg, val);
}

void is_scaler_set_bchs_clamp(struct pablo_mmio *base, u32 output_id,
	u32 y_max, u32 y_min, u32 c_max, u32 c_min)
{
	u32 val_y, reg_idx_y;
	u32 val_c, reg_idx_c;
	u32 pc_bchs_clamp_max_field =
		MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_CLAMP_MAX;
	u32 pc_bchs_clamp_min_field =
		MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_CLAMP_MIN;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_idx_y = MCSC_R_YUV_POST_PC0_BCHS_CLAMP_Y;
		reg_idx_c = MCSC_R_YUV_POST_PC0_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT1:
		reg_idx_y = MCSC_R_YUV_POST_PC1_BCHS_CLAMP_Y;
		reg_idx_c = MCSC_R_YUV_POST_PC1_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT2:
		reg_idx_y = MCSC_R_YUV_POST_PC2_BCHS_CLAMP_Y;
		reg_idx_c = MCSC_R_YUV_POST_PC2_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT3:
		reg_idx_y = MCSC_R_YUV_POST_PC3_BCHS_CLAMP_Y;
		reg_idx_c = MCSC_R_YUV_POST_PC3_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT4:
		reg_idx_y = MCSC_R_YUV_POST_PC4_BCHS_CLAMP_Y;
		reg_idx_c = MCSC_R_YUV_POST_PC4_BCHS_CLAMP_C;
		break;
	default:
		return;
	}

	val_y = val_c = 0;
	val_y = MCSC_SET_V(base, val_y, pc_bchs_clamp_max_field, y_max);
	val_y = MCSC_SET_V(base, val_y, pc_bchs_clamp_min_field, y_min);
	val_c = MCSC_SET_V(base, val_c, pc_bchs_clamp_max_field, c_max);
	val_c = MCSC_SET_V(base, val_c, pc_bchs_clamp_min_field, c_min);
	MCSC_SET_R(base, reg_idx_y, val_y);
	MCSC_SET_R(base, reg_idx_c, val_c);

	dbg_hw(2, "[OUT:%d]set_bchs_clamp: Y:%#x, C:%#x\n", output_id,
			MCSC_GET_R(base, reg_idx_y), MCSC_GET_R(base, reg_idx_c));
}

#if IS_ENABLED(MCSC_USE_MMIO)
static int mcsc_hw_rdma_create_mmio(struct is_common_dma *dma, void *base, u32 input_id)
{
	void __iomem *base_reg;
	ulong available_bayer_format_map;
	ulong available_yuv_format_map;
	ulong available_rgb_format_map;
	int ret = 0;
	char name[64];

	switch (input_id) {
	case MCSC_RDMA_HF:
		base_reg = base + mcsc_regs[MCSC_R_STAT_RDMAHF_ENABLE].sfr_offset;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_RDMA_HF", sizeof(name) - 1);
		break;
	default:
		err_hw("[MCSC] invalid input_id[%d]", input_id);
		return -EINVAL;
	}

	ret = is_hw_dma_set_ops(dma);
	ret |= is_hw_dma_create(dma, base_reg, input_id, name, available_bayer_format_map,
		available_yuv_format_map, available_rgb_format_map);
	CALL_DMA_OPS(dma, dma_set_corex_id, COREX_DIRECT);

	return ret;
}
#else
static int mcsc_hw_rdma_create_pmio(struct is_common_dma *dma, void *base, u32 input_id)
{
	ulong available_bayer_format_map;
	ulong available_yuv_format_map;
	ulong available_rgb_format_map;
	int ret = 0;
	char name[64];

	switch (input_id) {
	case MCSC_RDMA_HF:
		dma->reg_ofs = MCSC_R_STAT_RDMAHF_ENABLE;
		dma->field_ofs = MCSC_F_STAT_RDMAHF_EN;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_RDMA_HF", sizeof(name) - 1);
		break;
	default:
		err_hw("[MCSC] invalid input_id[%d]", input_id);
		return -EINVAL;
	}

	ret = pmio_dma_set_ops(dma);
	ret |= pmio_dma_create(dma, base, input_id, name, available_bayer_format_map,
		available_yuv_format_map, available_rgb_format_map);
	CALL_DMA_OPS(dma, dma_set_corex_id, COREX_DIRECT);

	return ret;
}
#endif

int mcsc_hw_rdma_create(struct is_common_dma *dma, void *base, u32 dma_id)
{
#if IS_ENABLED(MCSC_USE_MMIO)
	return mcsc_hw_rdma_create_mmio(dma, base, dma_id);
#else
	return mcsc_hw_rdma_create_pmio(dma, base, dma_id);
#endif
}

#if IS_ENABLED(MCSC_USE_MMIO)
static int mcsc_hw_wdma_create_mmio(struct is_common_dma *dma, void __iomem *base, u32 input_id)
{
	void __iomem *base_reg;
	ulong available_bayer_format_map;
	ulong available_yuv_format_map;
	ulong available_rgb_format_map;
	int ret = 0;
	char name[64];

	switch (input_id) {
	case MCSC_WDMA_W0:
		base_reg = base + mcsc_regs[MCSC_R_YUV_WDMASC_W0_ENABLE].sfr_offset;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W0", sizeof(name) - 1);
		break;
	case MCSC_WDMA_W1:
		base_reg = base + mcsc_regs[MCSC_R_YUV_WDMASC_W1_ENABLE].sfr_offset;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W1", sizeof(name) - 1);
		break;
	case MCSC_WDMA_W2:
		base_reg = base + mcsc_regs[MCSC_R_YUV_WDMASC_W2_ENABLE].sfr_offset;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W2", sizeof(name) - 1);
		break;
	case MCSC_WDMA_W3:
		base_reg = base + mcsc_regs[MCSC_R_YUV_WDMASC_W3_ENABLE].sfr_offset;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W3", sizeof(name) - 1);
		break;
	case MCSC_WDMA_W4:
		base_reg = base + mcsc_regs[MCSC_R_YUV_WDMASC_W4_ENABLE].sfr_offset;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W4", sizeof(name) - 1);
		break;
	default:
		err_hw("[MCSC] invalid input_id[%d]", input_id);
		return -EINVAL;
	}

	ret = is_hw_dma_set_ops(dma);
	ret |= is_hw_dma_create(dma, base_reg, input_id, name, available_bayer_format_map,
		available_yuv_format_map, available_rgb_format_map);

	return ret;
}
#else
static int mcsc_hw_wdma_create_pmio(struct is_common_dma *dma, void *base, u32 input_id)
{
	ulong available_bayer_format_map;
	ulong available_yuv_format_map;
	ulong available_rgb_format_map;
	int ret = 0;
	char name[64];

	switch (input_id) {
	case MCSC_WDMA_W0:
		dma->reg_ofs = MCSC_R_YUV_WDMASC_W0_ENABLE;
		dma->field_ofs = MCSC_F_YUV_WDMASC_W0_EN;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W0", sizeof(name) - 1);
		break;
	case MCSC_WDMA_W1:
		dma->reg_ofs = MCSC_R_YUV_WDMASC_W1_ENABLE;
		dma->field_ofs = MCSC_F_YUV_WDMASC_W1_EN;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W1", sizeof(name) - 1);
		break;
	case MCSC_WDMA_W2:
		dma->reg_ofs = MCSC_R_YUV_WDMASC_W2_ENABLE;
		dma->field_ofs = MCSC_F_YUV_WDMASC_W2_EN;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W2", sizeof(name) - 1);
		break;
	case MCSC_WDMA_W3:
		dma->reg_ofs = MCSC_R_YUV_WDMASC_W3_ENABLE;
		dma->field_ofs = MCSC_F_YUV_WDMASC_W3_EN;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W3", sizeof(name) - 1);
		break;
	case MCSC_WDMA_W4:
		dma->reg_ofs = MCSC_R_YUV_WDMASC_W4_ENABLE;
		dma->field_ofs = MCSC_F_YUV_WDMASC_W4_EN;
		available_bayer_format_map = 0x777 & IS_BAYER_FORMAT_MASK;
		available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
		available_rgb_format_map = 0xFFF & IS_RGB_FORMAT_MASK;
		strncpy(name, "MCSC_WDMA_W4", sizeof(name) - 1);
		break;
	default:
		err_hw("[MCSC] invalid input_id[%d]", input_id);
		return -EINVAL;
	}

	ret = pmio_dma_set_ops(dma);
	ret |= pmio_dma_create(dma, base, input_id, name, available_bayer_format_map,
		available_yuv_format_map, available_rgb_format_map);

	CALL_DMA_OPS(dma, dma_set_corex_id, COREX_DIRECT);

	return ret;
}
#endif
int mcsc_hw_wdma_create(struct is_common_dma *dma, void *base, u32 dma_id)
{
#if IS_ENABLED(MCSC_USE_MMIO)
	return mcsc_hw_wdma_create_mmio(dma, base, dma_id);
#else
	return mcsc_hw_wdma_create_pmio(dma, base, dma_id);
#endif
}


u32 is_scaler_get_dma_out_enable(struct pablo_mmio *base, u32 output_id)
{
	u32 wdma_enable_reg, wdma_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W0_ENABLE;
		wdma_enable_field = MCSC_F_YUV_WDMASC_W0_EN;
		break;
	case MCSC_OUTPUT1:
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W1_ENABLE;
		wdma_enable_field = MCSC_F_YUV_WDMASC_W1_EN;
		break;
	case MCSC_OUTPUT2:
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W2_ENABLE;
		wdma_enable_field = MCSC_F_YUV_WDMASC_W2_EN;
		break;
	case MCSC_OUTPUT3:
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W3_ENABLE;
		wdma_enable_field = MCSC_F_YUV_WDMASC_W3_EN;
		break;
	case MCSC_OUTPUT4:
		wdma_enable_reg = MCSC_R_YUV_WDMASC_W4_ENABLE;
		wdma_enable_field = MCSC_F_YUV_WDMASC_W4_EN;
		break;
	default:
		return 0;
	}

	return MCSC_GET_F(base, wdma_enable_reg, wdma_enable_field);
}

/* SBWC */
static u32 is_scaler_get_wdma_sbwc_enable(struct pablo_mmio *base, u32 output_id)
{
	u32 reg, field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg = MCSC_R_YUV_WDMASC_W0_COMP_CTRL;
		field = MCSC_F_YUV_WDMASC_W0_SBWC_EN;
		break;
	case MCSC_OUTPUT1:
		reg = MCSC_R_YUV_WDMASC_W1_COMP_CTRL;
		field = MCSC_F_YUV_WDMASC_W1_SBWC_EN;
		break;
	case MCSC_OUTPUT2:
		fallthrough;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return 0;
	}

	return MCSC_GET_F(base, reg, field);
}

static u32 is_scaler_get_sbwc_sram_offset(struct pablo_mmio *base_addr, u32 wdma1_w)
{
	u32 w0_offset = 0, w1_offset;
	u32 wdma0_w, wdma0_h;

	if (is_scaler_get_wdma_sbwc_enable(base_addr, MCSC_OUTPUT0) &&
		is_scaler_get_wdma_sbwc_enable(base_addr, MCSC_OUTPUT1)) {
		is_scaler_get_wdma_size(base_addr, MCSC_OUTPUT0, &wdma0_w, &wdma0_h);
		w0_offset = ALIGN(wdma0_w, 32);
		/* constraint check */
		w1_offset = ALIGN(wdma1_w, 32);
		if ((w0_offset + w1_offset) > MCSC_SBWC_MAX_WIDTH)
			err_hw("Invalid MCSC image size, w0(%d) + w1(%d) must be less than (%d)",
				w0_offset, w1_offset, MCSC_SBWC_MAX_WIDTH);
	}

	return w0_offset;
}

static void is_scaler_set_wdma_sbwc_sram_offset(struct pablo_mmio *base,
	u32 output_id, u32 out_width)
{
	u32 reg, field, value;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg = MCSC_R_YUV_WDMASC_W0_COMP_SRAM_START_ADDR;
		field = MCSC_F_YUV_WDMASC_W0_COMP_SRAM_START_ADDR;
		value = 0; /* HW recommend*/
		break;
	case MCSC_OUTPUT1:
		reg = MCSC_R_YUV_WDMASC_W1_COMP_SRAM_START_ADDR;
		field = MCSC_F_YUV_WDMASC_W1_COMP_SRAM_START_ADDR;
		value = is_scaler_get_sbwc_sram_offset(base, out_width);
		break;
	case MCSC_OUTPUT2:
		fallthrough;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, reg, field, value);
}

static int is_scaler_get_sbwc_pixelsize(u32 sbwc_enable, u32 format,
	u32 bitwidth, u32 *pixelsize)
{
	if (sbwc_enable) {
		if (!(format == DMA_INOUT_FORMAT_YUV422
			|| format == DMA_INOUT_FORMAT_YUV420))
			return -EINVAL;

		if (bitwidth == DMA_INOUT_BIT_WIDTH_8BIT)
			*pixelsize = DMA_INOUT_BIT_WIDTH_8BIT;
		else if (bitwidth == DMA_INOUT_BIT_WIDTH_16BIT)
			*pixelsize = DMA_INOUT_BIT_WIDTH_10BIT;
		else
			return -EINVAL;
	}

	return 0;
}

u32 is_scaler_get_payload_size(struct param_mcs_output *output, u8 plane)
{
	u32 comp_64b_align, comp_sbwc_en, comp_sbwc_fr;
	u32 height, stride, payload_size = 0;
	u32 pixelsize, format, bitwidth, sbwc_type;
	u32 quality_control;
	int ret;

	sbwc_type = output->sbwc_type;
	format = output->dma_format;
	bitwidth = output->dma_bitwidth;

	comp_sbwc_en = is_hw_dma_get_comp_sbwc_en(sbwc_type, &comp_64b_align);
	if (comp_sbwc_en) {
		ret = is_scaler_get_sbwc_pixelsize(comp_sbwc_en, format, bitwidth, &pixelsize);
		if (ret) {
			err_hw("Invalid MCSC SBWC WDMA Output format(%d) bitwidth(%d)", format, bitwidth);
			return 0;
		}

		comp_sbwc_fr = is_hw_dma_get_comp_sbwc_fr(sbwc_type);
		quality_control = comp_sbwc_fr ? LOSSY_COMP_FOOTPRINT : LOSSY_COMP_QUALITY;
		stride = is_hw_dma_get_payload_stride(
			comp_sbwc_en, pixelsize, output->full_output_width,
			comp_64b_align, quality_control,
			MCSC_COMP_BLOCK_WIDTH,
			MCSC_COMP_BLOCK_HEIGHT);

		height = output->height;
		if (plane && format == DMA_INOUT_FORMAT_YUV420)
			height /= 2;

		payload_size = (height + MCSC_COMP_BLOCK_HEIGHT - 1) / MCSC_COMP_BLOCK_HEIGHT * stride;
		dbg_hw(3, "width %d, height %d, stride %d, payload_size %d\n",
			output->full_output_width, height, stride, payload_size);
		dbg_hw(3, "comp_sbwc_en %d, comp_64b_align %d comp_fr %d\n",
			comp_sbwc_en, comp_64b_align, comp_sbwc_fr);
	}

	return payload_size;
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_payload_size);

u32 is_scaler_g_dma_offset(struct param_mcs_output *output,
	u32 start_pos_x, u32 plane_i,
	u32 *strip_header_offset_in_byte)
{
	u32 sbwc_type, comp_64b_align, comp_sbwc_en, comp_sbwc_fr;
	u32 quality_control;
	u32 dma_offset = 0, bitwidth;
	u32 format, pixelsize;
	unsigned long bitsperpixel;
	int ret;

	sbwc_type = output->sbwc_type;
	bitwidth = output->dma_bitwidth;
	bitsperpixel = output->bitsperpixel;
	format = output->dma_format;

	comp_sbwc_en = is_hw_dma_get_comp_sbwc_en(sbwc_type,
			&comp_64b_align);
	switch (comp_sbwc_en) {
	case COMP:
	case COMP_LOSS:
		ret = is_scaler_get_sbwc_pixelsize(comp_sbwc_en, format, bitwidth, &pixelsize);
		if (ret) {
			err_hw("Invalid MCSC SBWC WDMA Output format(%d) bitwidth(%d)", format, bitwidth);
			return 0;
		}

		comp_sbwc_fr = is_hw_dma_get_comp_sbwc_fr(sbwc_type);
		quality_control = comp_sbwc_fr ? LOSSY_COMP_FOOTPRINT : LOSSY_COMP_QUALITY;
		dma_offset = is_hw_dma_get_payload_stride(
			comp_sbwc_en, pixelsize, start_pos_x,
			comp_64b_align, quality_control,
			MCSC_COMP_BLOCK_WIDTH,
			MCSC_COMP_BLOCK_HEIGHT);
		*strip_header_offset_in_byte = is_hw_dma_get_header_stride(
			start_pos_x, MCSC_COMP_BLOCK_WIDTH, 0);

		dbg_hw(2, "sbwc_type %d, pixelsize %d, start_pos_x %d, \
			comp_64b_align %d, quality_control %d, \
			strip_offset_in_byte %d, strip_header_offset_in_byte %d\n",
			sbwc_type, pixelsize, start_pos_x,
			comp_64b_align, quality_control,
			dma_offset, *strip_header_offset_in_byte);
		break;
	default:
		if (plane_i <= 3) {
			bitsperpixel = (output->bitsperpixel >> (plane_i * 8)) & 0xFF;

			dma_offset =
				start_pos_x * bitsperpixel / BITS_PER_BYTE;
		} else {
			warn_hw("invalid plane_i(%d)\n", plane_i);
		}

		break;
	}

	return dma_offset;
}

void is_scaler_set_wdma_sbwc_config(struct is_common_dma *dma, struct pablo_mmio *base_addr,
	struct param_mcs_output *output, u32 output_id,
	u32 *y_stride, u32 *uv_stride, u32 *y_header_stride, u32 *uv_header_stride)
{
	int ret;
	u32 sbwc_type;
	u32 comp_sbwc_en, comp_64b_align, comp_sbwc_fr;
	u32 quality_control = 0;
	u32 out_width, format, bitwidth, pixelsize = 0;
	u32 order;

	sbwc_type = output->sbwc_type;
	format = output->dma_format;
	bitwidth = output->dma_bitwidth;
	out_width = output->full_output_width;
	order = output->dma_order;

	comp_sbwc_en = is_hw_dma_get_comp_sbwc_en(sbwc_type, &comp_64b_align);
	if (comp_sbwc_en) {
		if (output_id > MCSC_OUTPUT1) {
			err("invalid scaler ID");
			return;
		}

		ret = is_scaler_get_sbwc_pixelsize(comp_sbwc_en, format, bitwidth, &pixelsize);
		if (ret) {
			err_hw("Invalid MCSC SBWC WDMA Output format(%d) bitwidth(%d)", format, bitwidth);
			return;
		}

		comp_sbwc_fr = is_hw_dma_get_comp_sbwc_fr(sbwc_type);
		quality_control = comp_sbwc_fr ? LOSSY_COMP_FOOTPRINT : LOSSY_COMP_QUALITY;
		*y_stride = is_hw_dma_get_payload_stride(comp_sbwc_en, pixelsize, out_width,
								comp_64b_align, quality_control,
								MCSC_COMP_BLOCK_WIDTH,
								MCSC_COMP_BLOCK_HEIGHT);
		*uv_stride = is_hw_dma_get_payload_stride(comp_sbwc_en, pixelsize, out_width,
								comp_64b_align, quality_control,
								MCSC_COMP_BLOCK_WIDTH,
								MCSC_COMP_BLOCK_HEIGHT);
		*y_header_stride = is_hw_dma_get_header_stride(out_width, MCSC_COMP_BLOCK_WIDTH,
								16);
		*uv_header_stride = is_hw_dma_get_header_stride(out_width, MCSC_COMP_BLOCK_WIDTH,
								16);
	} else {
		*y_header_stride = 0;
		*uv_header_stride = 0;
	}

	CALL_DMA_OPS(dma, dma_set_comp_64b_align, comp_64b_align);
	CALL_DMA_OPS(dma, dma_set_comp_sbwc_en, comp_sbwc_en);
	CALL_DMA_OPS(dma, dma_set_comp_quality, quality_control);
	is_scaler_set_wdma_sbwc_sram_offset(base_addr, output_id, out_width);
}

static void is_scaler_set_rdma_sbwc_config(struct is_common_dma *dma, struct pablo_mmio *base_addr,
	struct param_mcs_output *output, u32 *y_stride, u32 *payload_size)
{
	u32 comp_sbwc_en, comp_64b_align;

	comp_sbwc_en = is_hw_dma_get_comp_sbwc_en(output->sbwc_type, &comp_64b_align);
	if (comp_sbwc_en == COMP) {
		*y_stride = is_hw_dma_get_payload_stride(comp_sbwc_en, output->dma_bitwidth,
				output->width, comp_64b_align, 0,
				MCSC_HF_COMP_BLOCK_WIDTH, MCSC_HF_COMP_BLOCK_HEIGHT);
		*payload_size = ((output->height + MCSC_HF_COMP_BLOCK_HEIGHT - 1) /
				MCSC_HF_COMP_BLOCK_HEIGHT) * (*y_stride);

		dbg_hw(2, "[%s] y_stride(%x), payload_size(%x)\n", __func__, *y_stride, *payload_size);
	} else if (comp_sbwc_en == COMP_LOSS) {
		err_hw("MCSC doesn't support lossy in HF(%d)", comp_sbwc_en);
		return;
	}

	CALL_DMA_OPS(dma, dma_set_comp_64b_align, comp_64b_align);
	CALL_DMA_OPS(dma, dma_set_comp_sbwc_en, comp_sbwc_en);
}

void is_scaler_set_rdma_format(struct pablo_mmio *base, u32 hw_id, u32 in_fmt)
{
	u32 val = 0;

	/* 0 : u8(NO SBWC) 1 : u10(SBWC) */
	val = MCSC_SET_V(base, val, MCSC_F_STAT_RDMAHF_DATA_FORMAT_BAYER, 0);
	val = MCSC_SET_V(base, val, MCSC_F_STAT_RDMAHF_DATA_FORMAT_RGB, 0);
	val = MCSC_SET_V(base, val, MCSC_F_STAT_RDMAHF_DATA_FORMAT_YUV, 0);
	val = MCSC_SET_V(base, val, MCSC_F_STAT_RDMAHF_DATA_FORMAT_MSBALIGN, 0);
	/* 0 : ip data is equal to dma_out */
	val = MCSC_SET_V(base, val, MCSC_F_STAT_RDMAHF_DATA_FORMAT_MSBALIGN_UNSIGN, 0);
	val = MCSC_SET_V(base, val, MCSC_F_STAT_RDMAHF_DATA_FORMAT_BIT_CROP_OFF, 0);

	MCSC_SET_R(base, MCSC_R_STAT_RDMAHF_DATA_FORMAT, val);
}

static u32 is_scaler_get_mono_ctrl(struct pablo_mmio *base)
{
	if (MCSC_GET_F(base, MCSC_R_YUV_MAIN_CTRL_INPUT_MONO_EN, MCSC_F_YUV_INPUT_MONO_EN))
		return MCSC_MONO_Y8;

	return 0;
}

void is_scaler_set_mono_ctrl(struct pablo_mmio *base, u32 hw_id, u32 in_fmt)
{
	u32 en = 0;

	if (in_fmt == MCSC_MONO_Y8)
		en = 1;

	MCSC_SET_F(base, MCSC_R_YUV_MAIN_CTRL_INPUT_MONO_EN,
		MCSC_F_YUV_INPUT_MONO_EN, en);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_mono_ctrl);

void is_scaler_set_rdma_10bit_type(struct pablo_mmio *base_addr, u32 dma_in_10bit_type)
{
	/* not support */
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_rdma_10bit_type);

static void is_scaler_set_wdma_rgb_coefficient(struct pablo_mmio *base)
{
	u32 val = 0;

	/* this value is valid only YUV422 only. wide coefficient is used in JPEG, Pictures, preview. */
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_SRC_Y_OFFSET, 0);
	MCSC_SET_R(base, MCSC_R_YUV_WDMASC_RGB_OFFSET, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_COEF_C00, 512);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_COEF_C10, 0);
	MCSC_SET_R(base, MCSC_R_YUV_WDMASC_RGB_COEF_0, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_COEF_C20, 738);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_COEF_C01, 512);
	MCSC_SET_R(base, MCSC_R_YUV_WDMASC_RGB_COEF_1, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_COEF_C11, -82);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_COEF_C21, -286);
	MCSC_SET_R(base, MCSC_R_YUV_WDMASC_RGB_COEF_2, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_COEF_C02, 512);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_WDMASC_RGB_COEF_C12, 942);
	MCSC_SET_R(base, MCSC_R_YUV_WDMASC_RGB_COEF_3, val);
	MCSC_SET_F(base, MCSC_R_YUV_WDMASC_RGB_COEF_4, MCSC_F_YUV_WDMASC_RGB_COEF_C22, 0);
}

u32 is_scaler_get_wdma_fmt_info(
	u32 *out_fmt, u32 *mono_mode, u32 hw_id, u32 output_id, struct pablo_mmio *base)
{
	u32 in_fmt = is_scaler_get_mono_ctrl(base);
	u32 format_type;

	if (in_fmt == MCSC_MONO_Y8 && *out_fmt != MCSC_MONO_Y8) {
		warn_hw("[ID:%d]Input format(MONO), out_format(%d) is changed to MONO format!\n",
			hw_id, *out_fmt);
		*out_fmt = MCSC_MONO_Y8;
	}

	/* format_type 0 : YUV , 1 : RGB */
	switch (*out_fmt) {
	case MCSC_RGB_ARGB8888:
	case MCSC_RGB_BGRA8888:
	case MCSC_RGB_RGBA8888:
	case MCSC_RGB_ABGR8888:
	case MCSC_RGB_ABGR1010102:
	case MCSC_RGB_RGBA1010102:
		is_scaler_set_wdma_rgb_coefficient(base);
		*out_fmt = *out_fmt - MCSC_RGB_RGBA8888 + 1;
		format_type = 1;
		break;
	default:
		format_type = 0;
		break;
	}

	if (*out_fmt == MCSC_MONO_Y8) {
		*mono_mode = 1;
		dbg_hw(2, "%s(%d) :[OUT:%d] changed out_fmt %d -> %d\n", __func__, __LINE__,
			output_id, *out_fmt, MCSC_YUV420_2P_VFIRST);
		*out_fmt = MCSC_YUV420_2P_VFIRST;
	} else {
		*mono_mode = 0;
	}

	return format_type;
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_wdma_fmt_info);

void is_scaler_set_wdma_format(struct is_common_dma *dma, struct pablo_mmio *base, u32 hw_id,
	u32 output_id, u32 plane, u32 out_fmt)
{
	u32 format_type;
	u32 mono_mode = 0;

	/* format_type 0 : YUV , 1 : RGB */
	format_type = is_scaler_get_wdma_fmt_info(&out_fmt, &mono_mode, hw_id, output_id, base);

	CALL_DMA_OPS(dma, dma_set_mono_mode, mono_mode);
	CALL_DMA_OPS(dma, dma_set_format, out_fmt, format_type);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_wdma_format);

void is_scaler_set_wdma_dither(struct pablo_mmio *base, u32 output_id,
	u32 fmt, u32 bitwidth, u32 plane)
{
	u32 val;
	u32 round_en, dither_en;
	u32 wdma_dither_en_y_field, wdma_dither_en_c_field;
	u32 wdma_round_en_field;
	u32 wdma_dither_reg;

	if (fmt == DMA_INOUT_FORMAT_RGB && bitwidth == DMA_INOUT_BIT_WIDTH_8BIT) {
		dither_en = 0;
		round_en = 1;
	} else if (bitwidth == DMA_INOUT_BIT_WIDTH_8BIT || plane == 4) {
		dither_en = 1;
		round_en = 0;
	} else {
		dither_en = 0;
		round_en = 0;
	}

	switch (output_id) {
	case MCSC_OUTPUT0:
		wdma_dither_en_y_field = MCSC_F_YUV_WDMASC_W0_DITHER_EN_Y;
		wdma_dither_en_c_field = MCSC_F_YUV_WDMASC_W0_DITHER_EN_C;
		wdma_round_en_field = MCSC_F_YUV_WDMASC_W0_ROUND_EN;
		wdma_dither_reg = MCSC_R_YUV_WDMASC_W0_DITHER;
		break;
	case MCSC_OUTPUT1:
		wdma_dither_en_y_field = MCSC_F_YUV_WDMASC_W1_DITHER_EN_Y;
		wdma_dither_en_c_field = MCSC_F_YUV_WDMASC_W1_DITHER_EN_C;
		wdma_round_en_field = MCSC_F_YUV_WDMASC_W1_ROUND_EN;
		wdma_dither_reg = MCSC_R_YUV_WDMASC_W1_DITHER;
		break;
	case MCSC_OUTPUT2:
		wdma_dither_en_y_field = MCSC_F_YUV_WDMASC_W2_DITHER_EN_Y;
		wdma_dither_en_c_field = MCSC_F_YUV_WDMASC_W2_DITHER_EN_C;
		wdma_round_en_field = MCSC_F_YUV_WDMASC_W2_ROUND_EN;
		wdma_dither_reg = MCSC_R_YUV_WDMASC_W2_DITHER;
		break;
	case MCSC_OUTPUT3:
		wdma_dither_en_y_field = MCSC_F_YUV_WDMASC_W3_DITHER_EN_Y;
		wdma_dither_en_c_field = MCSC_F_YUV_WDMASC_W3_DITHER_EN_C;
		wdma_round_en_field = MCSC_F_YUV_WDMASC_W3_ROUND_EN;
		wdma_dither_reg = MCSC_R_YUV_WDMASC_W3_DITHER;
		break;
	case MCSC_OUTPUT4:
		wdma_dither_en_y_field = MCSC_F_YUV_WDMASC_W4_DITHER_EN_Y;
		wdma_dither_en_c_field = MCSC_F_YUV_WDMASC_W4_DITHER_EN_C;
		wdma_round_en_field = MCSC_F_YUV_WDMASC_W4_ROUND_EN;
		wdma_dither_reg = MCSC_R_YUV_WDMASC_W4_DITHER;
		break;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, wdma_dither_en_y_field, dither_en);
	val = MCSC_SET_V(base, val, wdma_dither_en_c_field, dither_en);
	val = MCSC_SET_V(base, val, wdma_round_en_field, round_en);
	MCSC_SET_R(base, wdma_dither_reg, val);
}

void is_scaler_set_wdma_per_sub_frame_en(struct pablo_mmio *base, u32 output_id, u32 sub_frame_en)
{
	u32 wdma_per_sub_frame_en_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		wdma_per_sub_frame_en_reg = MCSC_R_YUV_WDMASC_W0_PER_SUB_FRAME_EN;
		break;
	case MCSC_OUTPUT1:
		wdma_per_sub_frame_en_reg = MCSC_R_YUV_WDMASC_W1_PER_SUB_FRAME_EN;
		break;
	case MCSC_OUTPUT2:
		wdma_per_sub_frame_en_reg = MCSC_R_YUV_WDMASC_W2_PER_SUB_FRAME_EN;
		break;
	case MCSC_OUTPUT3:
		wdma_per_sub_frame_en_reg = MCSC_R_YUV_WDMASC_W3_PER_SUB_FRAME_EN;
		break;
	case MCSC_OUTPUT4:
		wdma_per_sub_frame_en_reg = MCSC_R_YUV_WDMASC_W4_PER_SUB_FRAME_EN;
		break;
	default:
		return;
	}

	MCSC_SET_R(base, wdma_per_sub_frame_en_reg, sub_frame_en);
}

void is_scaler_set_flip_mode(struct pablo_mmio *base, u32 output_id, u32 flip)
{
	u32 wdma_flip_control_reg, wdma_flip_control_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		wdma_flip_control_reg = MCSC_R_YUV_WDMASC_W0_FLIP_CONTROL;
		wdma_flip_control_field = MCSC_F_YUV_WDMASC_W0_FLIP_CONTROL;
		break;
	case MCSC_OUTPUT1:
		wdma_flip_control_reg = MCSC_R_YUV_WDMASC_W1_FLIP_CONTROL;
		wdma_flip_control_field = MCSC_F_YUV_WDMASC_W1_FLIP_CONTROL;
		break;
	case MCSC_OUTPUT2:
		wdma_flip_control_reg = MCSC_R_YUV_WDMASC_W2_FLIP_CONTROL;
		wdma_flip_control_field = MCSC_F_YUV_WDMASC_W2_FLIP_CONTROL;
		break;
	case MCSC_OUTPUT3:
		wdma_flip_control_reg = MCSC_R_YUV_WDMASC_W3_FLIP_CONTROL;
		wdma_flip_control_field = MCSC_F_YUV_WDMASC_W3_FLIP_CONTROL;
		break;
	case MCSC_OUTPUT4:
		wdma_flip_control_reg = MCSC_R_YUV_WDMASC_W4_FLIP_CONTROL;
		wdma_flip_control_field = MCSC_F_YUV_WDMASC_W4_FLIP_CONTROL;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, wdma_flip_control_reg, wdma_flip_control_field, flip);
}

void is_scaler_get_rdma_size(struct pablo_mmio *base, u32 *width, u32 *height)
{
	*width = MCSC_GET_F(base, MCSC_R_STAT_RDMAHF_WIDTH, MCSC_F_STAT_RDMAHF_WIDTH);
	*height = MCSC_GET_F(base, MCSC_R_STAT_RDMAHF_HEIGHT, MCSC_F_STAT_RDMAHF_HEIGHT);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_rdma_size);

void is_scaler_get_wdma_size(struct pablo_mmio *base, u32 output_id, u32 *width, u32 *height)
{
	u32 wdma_width_reg, wdma_height_reg;
	u32 wdma_width_field = MCSC_F_YUV_WDMASC_W0_WIDTH;
	u32 wdma_height_field = MCSC_F_YUV_WDMASC_W0_HEIGHT;

	switch (output_id) {
	case MCSC_OUTPUT0:
		wdma_width_reg = MCSC_R_YUV_WDMASC_W0_WIDTH;
		wdma_height_reg = MCSC_R_YUV_WDMASC_W0_HEIGHT;
		break;
	case MCSC_OUTPUT1:
		wdma_width_reg = MCSC_R_YUV_WDMASC_W1_WIDTH;
		wdma_height_reg = MCSC_R_YUV_WDMASC_W1_HEIGHT;
		break;
	case MCSC_OUTPUT2:
		wdma_width_reg = MCSC_R_YUV_WDMASC_W2_WIDTH;
		wdma_height_reg = MCSC_R_YUV_WDMASC_W2_HEIGHT;
		break;
	case MCSC_OUTPUT3:
		wdma_width_reg = MCSC_R_YUV_WDMASC_W3_WIDTH;
		wdma_height_reg = MCSC_R_YUV_WDMASC_W3_HEIGHT;
		break;
	case MCSC_OUTPUT4:
		wdma_width_reg = MCSC_R_YUV_WDMASC_W4_WIDTH;
		wdma_height_reg = MCSC_R_YUV_WDMASC_W4_HEIGHT;
		break;
	default:
		*width = 0;
		*height = 0;
		return;
	}

	*width = MCSC_GET_F(base, wdma_width_reg, wdma_width_field);
	*height = MCSC_GET_F(base, wdma_height_reg, wdma_height_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_wdma_size);

void is_scaler_get_rdma_stride(struct pablo_mmio *base, u32 *y_stride, u32 *uv_stride)
{
	/* RDMA format - Bayer */
	*y_stride = MCSC_GET_F(base, MCSC_R_STAT_RDMAHF_STRIDE_1P, MCSC_F_STAT_RDMAHF_STRIDE_1P);
	*uv_stride = 0;
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_rdma_stride);

void is_scaler_get_wdma_stride(struct pablo_mmio *base, u32 output_id, u32 *y_stride, u32 *uv_stride)
{
	u32 wdma_stride_1p_reg, wdma_stride_1p_field;
	u32 wdma_stride_2p_reg, wdma_stride_2p_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		wdma_stride_1p_reg = MCSC_R_YUV_WDMASC_W0_STRIDE_1P;
		wdma_stride_1p_field = MCSC_F_YUV_WDMASC_W0_STRIDE_1P;
		wdma_stride_2p_reg = MCSC_R_YUV_WDMASC_W0_STRIDE_2P;
		wdma_stride_2p_field = MCSC_F_YUV_WDMASC_W0_STRIDE_2P;
		break;
	case MCSC_OUTPUT1:
		wdma_stride_1p_reg = MCSC_R_YUV_WDMASC_W1_STRIDE_1P;
		wdma_stride_1p_field = MCSC_F_YUV_WDMASC_W1_STRIDE_1P;
		wdma_stride_2p_reg = MCSC_R_YUV_WDMASC_W1_STRIDE_2P;
		wdma_stride_2p_field = MCSC_F_YUV_WDMASC_W1_STRIDE_2P;
		break;
	case MCSC_OUTPUT2:
		wdma_stride_1p_reg = MCSC_R_YUV_WDMASC_W2_STRIDE_1P;
		wdma_stride_1p_field = MCSC_F_YUV_WDMASC_W2_STRIDE_1P;
		wdma_stride_2p_reg = MCSC_R_YUV_WDMASC_W2_STRIDE_2P;
		wdma_stride_2p_field = MCSC_F_YUV_WDMASC_W2_STRIDE_2P;
		break;
	case MCSC_OUTPUT3:
		wdma_stride_1p_reg = MCSC_R_YUV_WDMASC_W3_STRIDE_1P;
		wdma_stride_1p_field = MCSC_F_YUV_WDMASC_W3_STRIDE_1P;
		wdma_stride_2p_reg = MCSC_R_YUV_WDMASC_W3_STRIDE_2P;
		wdma_stride_2p_field = MCSC_F_YUV_WDMASC_W3_STRIDE_2P;
		break;
	case MCSC_OUTPUT4:
		wdma_stride_1p_reg = MCSC_R_YUV_WDMASC_W4_STRIDE_1P;
		wdma_stride_1p_field = MCSC_F_YUV_WDMASC_W4_STRIDE_1P;
		wdma_stride_2p_reg = MCSC_R_YUV_WDMASC_W4_STRIDE_2P;
		wdma_stride_2p_field = MCSC_F_YUV_WDMASC_W4_STRIDE_2P;
		break;
	default:
		*y_stride = 0;
		*uv_stride = 0;
		return;
	}

	*y_stride = MCSC_GET_F(base, wdma_stride_1p_reg, wdma_stride_1p_field);
	*uv_stride = MCSC_GET_F(base, wdma_stride_2p_reg, wdma_stride_2p_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_wdma_stride);

void is_scaler_set_rdma_frame_seq(struct pablo_mmio *base_addr, u32 frame_seq)
{
	/* not support */
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_rdma_frame_seq);

static void get_wdma_addr_arr(u32 output_id, u32 *addr)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		addr[0] = MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_0;
		addr[1] = MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_0;
		addr[2] = MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_0;
		addr[3] = MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_0;
		addr[4] = MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_0;
		break;
	case MCSC_OUTPUT1:
		addr[0] = MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_0;
		addr[1] = MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_0;
		addr[2] = MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_0;
		addr[3] = MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_0;
		addr[4] = MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_0;
		break;
	case MCSC_OUTPUT2:
		addr[0] = MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_0;
		addr[1] = MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_0;
		addr[2] = MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_0;
		addr[3] = 0;
		addr[4] = 0;
		break;
	case MCSC_OUTPUT3:
		addr[0] = MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_0;
		addr[1] = MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_0;
		addr[2] = MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_0;
		addr[3] = 0;
		addr[4] = 0;
		break;
	case MCSC_OUTPUT4:
		addr[0] = MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_0;
		addr[1] = MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_0;
		addr[2] = MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_0;
		addr[3] = 0;
		addr[4] = 0;
		break;
	default:
		panic("invalid output_id(%d)", output_id);
		break;
	}
}

void is_scaler_get_wdma_addr(struct pablo_mmio *base, u32 output_id,
	u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	u32 addr[5] = {0, };

	get_wdma_addr_arr(output_id, addr);
	if (!addr[0])
		return;

	buf_index = buf_index * 4;
	*y_addr = MCSC_GET_R(base, addr[0] + buf_index);
	*cb_addr = MCSC_GET_R(base, addr[1] + buf_index);
	*cr_addr = MCSC_GET_R(base, addr[2] + buf_index);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_wdma_addr);

void is_scaler_clear_rdma_addr(struct pablo_mmio *base)
{
	MCSC_SET_R(base, MCSC_R_STAT_RDMAHF_IMG_BASE_ADDR_1P_FRO_0_0, 0x0);
	MCSC_SET_R(base, MCSC_R_STAT_RDMAHF_HEADER_BASE_ADDR_1P_FRO_0_0, 0x0);
}
KUNIT_EXPORT_SYMBOL(is_scaler_clear_rdma_addr);

void is_scaler_clear_wdma_addr(struct pablo_mmio *base, u32 output_id)
{
	u32 addr[5] = {0, }, buf_index;

	get_wdma_addr_arr(output_id, addr);
	if (!addr[0])
		return;

	for (buf_index = 0; buf_index < 8; buf_index++) {
		MCSC_SET_R(base, addr[0] + (buf_index * 4), 0x0);
		MCSC_SET_R(base, addr[1] + (buf_index * 4), 0x0);
		MCSC_SET_R(base, addr[2] + (buf_index * 4), 0x0);

		/* DMA Header Y, CR address clear */
		if (output_id == MCSC_OUTPUT0 || output_id == MCSC_OUTPUT1) {
			MCSC_SET_R(base, addr[3] + (buf_index * 4), 0x0);
			MCSC_SET_R(base, addr[4] + (buf_index * 4), 0x0);
		}
	}
}

/* HWFC */
void is_scaler_set_hwfc_mode(struct pablo_mmio *base, u32 hwfc_output_ids)
{
	u32 val = MCSC_HWFC_MODE_OFF;
	u32 read_val;

	if (hwfc_output_ids & (1 << MCSC_OUTPUT3))
		MCSC_SET_F(base, MCSC_R_YUV_HWFC_FRAME_START_SELECT,
			MCSC_F_YUV_HWFC_FRAME_START_SELECT, 0x0);


	read_val = MCSC_GET_F(base, MCSC_R_YUV_HWFC_MODE, MCSC_F_YUV_HWFC_MODE);

	if ((hwfc_output_ids & (1 << MCSC_OUTPUT3)) &&
		(hwfc_output_ids & (1 << MCSC_OUTPUT4))) {
		val = MCSC_HWFC_MODE_REGION_A_B_PORT;
	} else if (hwfc_output_ids & (1 << MCSC_OUTPUT3)) {
		val = MCSC_HWFC_MODE_REGION_A_PORT;
	} else if (hwfc_output_ids & (1 << MCSC_OUTPUT4)) {
		err_hw("set_hwfc_mode: invalid output_ids(0x%x)\n", hwfc_output_ids);
		return;
	}

	MCSC_SET_F(base, MCSC_R_YUV_HWFC_MODE, MCSC_F_YUV_HWFC_MODE, val);

	dbg_hw(2, "set_hwfc_mode: regs(0x%x)(0x%x), hwfc_ids(0x%x)\n",
		read_val, val, hwfc_output_ids);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_hwfc_mode);

struct mcsc_hwfc_cfg {
	u32 total_img_byte0;
	u32 total_img_byte1;
	u32 total_img_byte2;
	u32 total_width_byte0;
	u32 total_width_byte1;
	u32 total_width_byte2;
	enum is_mcsc_hwfc_format hwfc_fmt;
};

static void __is_scaler_calc_hwfc_config(struct mcsc_hwfc_cfg *cfg, u32 fmt,
	u32 plane, u32 width, u32 height)
{
	u32 img_resol = width * height;

	cfg->total_img_byte0 = img_resol;
	cfg->total_img_byte1 = 0;
	cfg->total_img_byte2 = 0;
	cfg->total_width_byte0 = width;
	cfg->total_width_byte1 = 0;
	cfg->total_width_byte2 = 0;

	switch (fmt) {
	case DMA_INOUT_FORMAT_YUV422:
		switch (plane) {
		case 3:
			cfg->total_img_byte1 = img_resol >> 1;
			cfg->total_img_byte2 = img_resol >> 1;
			cfg->total_width_byte1 = width >> 1;
			cfg->total_width_byte2 = width >> 1;
			break;
		case 2:
			cfg->total_img_byte1 = img_resol;
			cfg->total_width_byte1 = width;
			break;
		case 1:
			cfg->total_img_byte0 = img_resol << 1;
			cfg->total_width_byte0 = width << 1;
			break;
		default:
			err_hw("invalid hwfc plane (%d, %d, %dx%d)",
				fmt, plane, width, height);
			return;
		}

		cfg->hwfc_fmt = MCSC_HWFC_FMT_YUV444_YUV422;
		break;
	case DMA_INOUT_FORMAT_YUV420:
		switch (plane) {
		case 3:
			cfg->total_img_byte1 = img_resol >> 2;
			cfg->total_img_byte2 = img_resol >> 2;
			cfg->total_width_byte1 = width >> 1;
			cfg->total_width_byte2 = width >> 1;
			break;
		case 2:
			cfg->total_img_byte1 = img_resol >> 1;
			cfg->total_width_byte1 = width;
			break;
		default:
			err_hw("invalid hwfc plane (%d, %d, %dx%d)",
				fmt, plane, width, height);
			return;
		}

		cfg->hwfc_fmt = MCSC_HWFC_FMT_YUV420;
		break;
	default:
		break;
	}
}

void is_scaler_set_hwfc_config(struct pablo_mmio *base,
	u32 output_id, u32 fmt, u32 plane, u32 dma_idx, u32 width, u32 height, u32 enable)
{
	struct mcsc_hwfc_cfg cfg;
	u32 hwfc_id, offset;
	u32 hwfc_master_sel_field;
	u32 hwfc_image_height_field;
	u32 val;

	if (!enable) {
		MCSC_SET_R(base, MCSC_R_YUV_WDMASC_W3_VOTF_EN, 0);
		MCSC_SET_R(base, MCSC_R_YUV_WDMASC_W4_VOTF_EN, 0);
		return;
	}

	if (output_id < MCSC_OUTPUT3 || output_id > MCSC_OUTPUT4) {
		err("invalid scaler ID");
		return;
	}
	if (fmt != DMA_INOUT_FORMAT_YUV422 && fmt != DMA_INOUT_FORMAT_YUV420) {
		err("invalid format");
		return;
	}

	MCSC_SET_R(base, MCSC_R_YUV_HWFC_INDEX_RESET, 1);

	__is_scaler_calc_hwfc_config(&cfg, fmt, plane, width, height);

	hwfc_id = output_id - MCSC_OUTPUT3;
	offset = MCSC_R_YUV_HWFC_CONFIG_IMAGE_B - MCSC_R_YUV_HWFC_CONFIG_IMAGE_A;

	val = MCSC_GET_R(base, MCSC_R_YUV_HWFC_CONFIG_IMAGE_A + hwfc_id * offset);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_HWFC_FORMAT_A, cfg.hwfc_fmt);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_HWFC_PLANE_A, plane);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_HWFC_ID0_A, dma_idx);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_HWFC_ID1_A, dma_idx + 2);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_HWFC_ID2_A, dma_idx + 4);
	MCSC_SET_R(base, MCSC_R_YUV_HWFC_CONFIG_IMAGE_A + hwfc_id * offset, val);

	MCSC_SET_R(base, MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE0_A + hwfc_id * offset,
		cfg.total_img_byte0);
	MCSC_SET_R(base, MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE1_A + hwfc_id * offset,
		cfg.total_img_byte1);
	MCSC_SET_R(base, MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE2_A + hwfc_id * offset,
		cfg.total_img_byte2);
	MCSC_SET_R(base, MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE0_A + hwfc_id * offset,
		cfg.total_width_byte0);
	MCSC_SET_R(base, MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE1_A + hwfc_id * offset,
		cfg.total_width_byte1);
	MCSC_SET_R(base, MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE2_A + hwfc_id * offset,
		cfg.total_width_byte2);

	if (hwfc_id) {
		hwfc_master_sel_field = MCSC_F_YUV_HWFC_MASTER_SEL_B;
		hwfc_image_height_field = MCSC_F_YUV_HWFC_IMAGE_HEIGHT_B;
		MCSC_SET_R(base, MCSC_R_YUV_WDMASC_W4_VOTF_EN, 1);
	} else {
		hwfc_master_sel_field = MCSC_F_YUV_HWFC_MASTER_SEL_A;
		hwfc_image_height_field = MCSC_F_YUV_HWFC_IMAGE_HEIGHT_A;
		MCSC_SET_R(base, MCSC_R_YUV_WDMASC_W3_VOTF_EN, 1);
	}
	MCSC_SET_F(base, MCSC_R_YUV_HWFC_MASTER_SEL, hwfc_master_sel_field, output_id);
	MCSC_SET_F(base, MCSC_R_YUV_HWFC_IMAGE_HEIGHT, hwfc_image_height_field, height);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_hwfc_config);

u32 is_scaler_get_hwfc_idx_bin(struct pablo_mmio *base, u32 output_id)
{
	u32 hwfc_region_idx_bin_field;
	u32 hwfc_region_idx_bin_reg = MCSC_R_YUV_HWFC_REGION_IDX_BIN;

	switch (output_id) {
	case MCSC_OUTPUT3:
		hwfc_region_idx_bin_field = MCSC_F_YUV_HWFC_REGION_IDX_BIN_A;
		break;
	case MCSC_OUTPUT4:
		hwfc_region_idx_bin_field = MCSC_F_YUV_HWFC_REGION_IDX_BIN_B;
		break;
	case MCSC_OUTPUT0:
		fallthrough;
	case MCSC_OUTPUT1:
		fallthrough;
	case MCSC_OUTPUT2:
		fallthrough;
	default:
		return 0;
	}

	return MCSC_GET_F(base, hwfc_region_idx_bin_reg, hwfc_region_idx_bin_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_hwfc_idx_bin);

u32 is_scaler_get_hwfc_cur_idx(struct pablo_mmio *base, u32 output_id)
{
	u32 hwfc_curr_region_field;
	u32 hwfc_curr_region_reg = MCSC_R_YUV_HWFC_CURR_REGION;


	switch (output_id) {
	case MCSC_OUTPUT3:
		hwfc_curr_region_field = MCSC_F_YUV_HWFC_CURR_REGION_A;
		break;
	case MCSC_OUTPUT4:
		hwfc_curr_region_field = MCSC_F_YUV_HWFC_CURR_REGION_B;
		break;
	case MCSC_OUTPUT0:
		fallthrough;
	case MCSC_OUTPUT1:
		fallthrough;
	case MCSC_OUTPUT2:
		fallthrough;
	default:
		return 0;
	}

	return MCSC_GET_F(base, hwfc_curr_region_reg, hwfc_curr_region_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_hwfc_cur_idx);

/* for DJAG */
void is_scaler_set_djag_enable(struct pablo_mmio *base, u32 djag_enable)
{
	u32 val;

	val = MCSC_GET_R(base, MCSC_R_YUV_DJAG_CTRL);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAG_ENABLE, djag_enable);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_PS_ENABLE, djag_enable);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_EZPOST_ENABLE, djag_enable);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_CTRL, val);

	if (!djag_enable)
		MCSC_SET_F(base, MCSC_R_YUV_DJAGPREFILTER_CTRL, MCSC_F_YUV_DJAGPREFILTER_EZPOST_ENABLE, 0);
}

void is_scaler_set_pre_enable(struct pablo_mmio *base, u32 presc_enable, u32 predjag_enable)
{
	if (presc_enable != MCSC_PRE_SC_SET_SKIP)
		MCSC_SET_F(base, MCSC_R_YUV_DJAG_CTRL, MCSC_F_YUV_DJAGPS_PS_ENABLE, presc_enable);

	MCSC_SET_F(base, MCSC_R_YUV_DJAGPREFILTER_CTRL, MCSC_F_YUV_DJAGPREFILTER_EZPOST_ENABLE, predjag_enable);
}

void is_scaler_set_djag_input_size(struct pablo_mmio *base, u32 width, u32 height)
{
	u32 val = 0;

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_INPUT_IMG_HSIZE, width);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_INPUT_IMG_VSIZE, height);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_IMG_SIZE, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_INPUT_IMG_HSIZE, width);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_INPUT_IMG_VSIZE, height);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_IMG_SIZE, val);
}

void is_scaler_set_djag_src_size(struct pablo_mmio *base, u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	u32 val = 0;

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_PS_SRC_HPOS, pos_x);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_PS_SRC_VPOS, pos_y);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_PS_SRC_POS, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_PS_SRC_HSIZE, width);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_PS_SRC_VSIZE, height);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_PS_SRC_SIZE, val);
}

void is_scaler_set_djag_hf_enable(struct pablo_mmio *base, u32 hf_enable, u32 hf_scaler_enable)
{
	u32 val = 0;
	u32 djag_enable = MCSC_GET_F(base, MCSC_R_YUV_DJAG_CTRL, MCSC_F_YUV_DJAG_ENABLE);

	if (hf_enable && !djag_enable) {
		err_hw("To enable HF block, DJAG should be enabled. Force HF block off\n");
#if IS_ENABLED(CONFIG_PABLO_V12_1_0)
		val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_BYPASS, 1);
#endif
	} else {
#if IS_ENABLED(CONFIG_PABLO_V12_1_0)
		val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_BYPASS, !hf_enable);
#else
		val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_ENABLE_A, hf_enable);
#endif
	}

	MCSC_SET_R(base, MCSC_R_YUV_DJAG_RECOM_CTRL, val);
}

void is_scaler_set_djag_hf_cfg(struct pablo_mmio *base, struct hf_cfg_by_ni *hf_cfg)
{
	MCSC_SET_R(base, MCSC_R_YUV_DJAGRECOMP_RECOM_WEIGHT, hf_cfg->hf_weight);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_djag_hf_cfg);

static void is_scaler_set_djag_hf_img_size(struct pablo_mmio *base, u32 width, u32 height)
{
	u32 val = 0;

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_INPUT_IMG_HSIZE, width);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_INPUT_IMG_VSIZE, height);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_RECOM_IMG_SIZE, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_IN_CROP_SIZE_H, width);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_IN_CROP_SIZE_V, height);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_RECOM_IN_CROP_SIZE, val);
}

static void is_scaler_set_djag_hf_radial_center(struct pablo_mmio *base, u32 pos_x, u32 pos_y)
{
	u32 val = 0;

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_CENTER_X, pos_x);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_CENTER_Y, pos_y);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_RECOM_RADIAL_CENTER, val);
}

static void is_scaler_set_djag_hf_radial_binning(struct pablo_mmio *base, u32 pos_x, u32 pos_y)
{
	u32 val = 0;

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_BINNING_X, pos_x);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_BINNING_Y, pos_y);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_RECOM_RADIAL_BINNING, val);
}

static void is_scaler_set_djag_hf_radial_biquad_factor(struct pablo_mmio *base,
	u32 factor_a, u32 factor_b)
{
	MCSC_SET_F(base, MCSC_R_YUV_DJAGRECOMP_RECOM_RADIAL_BIQUAD_FACTOR_A,
		MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_BIQUAD_FACTOR_A, factor_a);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGRECOMP_RECOM_RADIAL_BIQUAD_FACTOR_B,
		MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_BIQUAD_FACTOR_B, factor_b);
}

static void is_scaler_set_djag_hf_radial_gain(struct pablo_mmio *base,
	u32 enable, u32 increment)
{
	MCSC_SET_F(base, MCSC_R_YUV_DJAGRECOMP_RECOM_RADIAL_GAIN_ENABLE,
		MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_GAIN_ENABLE, enable);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGRECOMP_RECOM_RADIAL_GAIN_INCREMENT,
		MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_GAIN_INCREMENT, increment);
}

static void is_scaler_set_djag_hf_radial_biquad_scal_shift_adder(struct pablo_mmio *base,
	u32 value)
{
	MCSC_SET_F(base, MCSC_R_YUV_DJAGRECOMP_RECOM_RADIAL_BIQUAD_SCAL_SHIFT_ADDER,
		MCSC_F_YUV_DJAGRECOMP_RECOM_RADIAL_BIQUAD_SCAL_SHIFT_ADDER, value);
}

#if IS_ENABLED(USE_DJAG_COEFF_CONFIG)
static void is_scaler_set_djag_scaler_h_coef(struct pablo_mmio *base_addr,
	struct mcsc_h_coef *h_coef_cfg, enum mcsc_plane_type plane)
{
	u32 sc_coeff_reg;
	u32 sc_coeff_field = MCSC_F_YUV_DJAG_SC_H_COEFF_0_0;
	u32 reg_offset, field_offset;

	if (plane == MCSC_PLANE_Y) {
		sc_coeff_reg = MCSC_R_YUV_DJAG_SC_H_COEFF_0_01;
		reg_offset = REG_OFFSET * 4;
		field_offset = 8;
	} else if (plane == MCSC_PLANE_UV) {
		sc_coeff_reg = MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_0_01;
		reg_offset = REG_OFFSET * 2;
		field_offset = 4;
	} else {
		err("invalid plane");
		return;
	}

	__mcsc_hw_s_horizontal_coeff(base_addr, h_coef_cfg, plane,
		sc_coeff_reg, sc_coeff_field, reg_offset, field_offset);
}

static void is_scaler_set_djag_scaler_v_coef(struct pablo_mmio *base_addr,
	struct mcsc_v_coef *v_coef_cfg, enum mcsc_plane_type plane)
{
	u32 sc_coeff_reg;
	u32 sc_coeff_field = MCSC_F_YUV_DJAG_SC_V_COEFF_0_0;
	u32 reg_offset = REG_OFFSET * 2, field_offset = 4;

	if (plane == MCSC_PLANE_Y) {
		sc_coeff_reg = MCSC_R_YUV_DJAG_SC_V_COEFF_0_01;
	} else if (plane == MCSC_PLANE_UV) {
		sc_coeff_reg = MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_0_01;
	} else {
		err("invalid plane");
		return;
	}

	__mcsc_hw_s_vertical_coeff(base_addr, v_coef_cfg,
		sc_coeff_reg, sc_coeff_field, reg_offset, field_offset);
}

static void __is_scaler_set_djag_scaler_coef(struct pablo_mmio *base_addr,
	const struct djag_coef_cfg *sc_coef, enum mcsc_plane_type plane)
{
	struct mcsc_h_coef *h_coef = NULL;
	struct mcsc_v_coef *v_coef = NULL;

	if (sc_coef->use_djag_sc_coef) {
		h_coef = (struct mcsc_h_coef *)&sc_coef->djag_sc_h_coef;
		v_coef = (struct mcsc_v_coef *)&sc_coef->djag_sc_v_coef;
	} else {
		if (plane == MCSC_PLANE_Y) {
			h_coef = (struct mcsc_h_coef *)&h_coef_8tap[0];
			v_coef = (struct mcsc_v_coef *)&v_coef_4tap[0];
		} else {
			h_coef = (struct mcsc_h_coef *)&uv_h_coef_4tap[0];
			v_coef = (struct mcsc_v_coef *)&uv_v_coef_4tap[0];
		}
	}

	is_scaler_set_djag_scaler_h_coef(base_addr, h_coef, plane);
	is_scaler_set_djag_scaler_v_coef(base_addr, v_coef, plane);
}

static void is_scaler_set_djag_scaler_coef(struct pablo_mmio *base_addr,
	const struct djag_coef_cfg *sc_coef, const struct djag_coef_cfg *sc_uv_coef)
{
	if (sc_coef)
		__is_scaler_set_djag_scaler_coef(base_addr, sc_coef, MCSC_PLANE_Y);
	if (sc_uv_coef)
		__is_scaler_set_djag_scaler_coef(base_addr, sc_uv_coef, MCSC_PLANE_UV);
}
#endif

void is_scaler_set_djag_dst_size(struct pablo_mmio *base, u32 width, u32 height)
{
	u32 val = 0;

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_PS_DST_HSIZE, width);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPS_PS_DST_VSIZE, height);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_PS_DST_SIZE, val);

	/* High frequency component size should be same with pre-scaled image size in DJAG. */
	is_scaler_set_djag_hf_img_size(base, width, height);
}

void is_scaler_set_djag_scaling_ratio(struct pablo_mmio *base, u32 hratio, u32 vratio)
{
	MCSC_SET_F(base, MCSC_R_YUV_DJAGPS_PS_H_RATIO,
		MCSC_F_YUV_DJAGPS_PS_H_RATIO, hratio);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGPS_PS_V_RATIO,
		MCSC_F_YUV_DJAGPS_PS_V_RATIO, vratio);
}

void is_scaler_set_djag_init_phase_offset(struct pablo_mmio *base, u32 h_offset, u32 v_offset)
{
	MCSC_SET_F(base, MCSC_R_YUV_DJAGPS_PS_H_INIT_PHASE_OFFSET,
		MCSC_F_YUV_DJAGPS_PS_H_INIT_PHASE_OFFSET, h_offset);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGPS_PS_V_INIT_PHASE_OFFSET,
		MCSC_F_YUV_DJAGPS_PS_V_INIT_PHASE_OFFSET, v_offset);
}

void is_scaler_set_djag_round_mode(struct pablo_mmio *base, u32 round_enable)
{
	MCSC_SET_F(base, MCSC_R_YUV_DJAGPS_PS_ROUND_MODE,
		MCSC_F_YUV_DJAGPS_PS_ROUND_MODE, round_enable);
}

void is_scaler_get_djag_strip_config(struct pablo_mmio *base, u32 output_id, u32 *pre_dst_h, u32 *start_pos_h)
{
	*pre_dst_h = MCSC_GET_F(base, MCSC_R_YUV_DJAG_PS_STRIP_PRE_DST_SIZE,
		MCSC_F_YUV_DJAGPS_PS_STRIP_PRE_DST_SIZE_H);
	*start_pos_h = MCSC_GET_F(base, MCSC_R_YUV_DJAG_PS_STRIP_IN_START_POS,
		MCSC_F_YUV_DJAGPS_PS_STRIP_IN_START_POS_H);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_djag_strip_config);

void is_scaler_set_djag_strip_config(struct pablo_mmio *base, u32 output_id, u32 pre_dst_h, u32 start_pos_h)
{
	MCSC_SET_F(base, MCSC_R_YUV_DJAG_PS_STRIP_PRE_DST_SIZE,
		MCSC_F_YUV_DJAGPS_PS_STRIP_PRE_DST_SIZE_H, pre_dst_h);
	MCSC_SET_F(base, MCSC_R_YUV_DJAG_PS_STRIP_IN_START_POS,
		MCSC_F_YUV_DJAGPS_PS_STRIP_IN_START_POS_H, start_pos_h);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_djag_strip_config);

void is_scaler_set_djag_tuning_param(struct pablo_mmio *base,
	const struct djag_scaling_ratio_depended_config *djag_tune)
{
	u32 val = 0;

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_XFILTER_DEJAGGING_WEIGHT0,
		djag_tune->xfilter_dejagging_coeff_cfg.xfilter_dejagging_weight0);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_XFILTER_DEJAGGING_WEIGHT1,
		djag_tune->xfilter_dejagging_coeff_cfg.xfilter_dejagging_weight1);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_XFILTER_HF_BOOST_WEIGHT,
		djag_tune->xfilter_dejagging_coeff_cfg.xfilter_hf_boost_weight);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_CENTER_HF_BOOST_WEIGHT,
		djag_tune->xfilter_dejagging_coeff_cfg.center_hf_boost_weight);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DIAGONAL_HF_BOOST_WEIGHT,
		djag_tune->xfilter_dejagging_coeff_cfg.diagonal_hf_boost_weight);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_CENTER_WEIGHTED_MEAN_WEIGHT,
		djag_tune->xfilter_dejagging_coeff_cfg.center_weighted_mean_weight);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_XFILTER_DEJAGGING_COEFF, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_THRES_1X5_MATCHING_SAD,
		djag_tune->thres_1x5_matching_cfg.thres_1x5_matching_sad);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_THRES_1X5_ABSHF,
		djag_tune->thres_1x5_matching_cfg.thres_1x5_abshf);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_THRES_1X5_MATCHING, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_LLCRR,
		djag_tune->thres_shooting_detect_cfg.thres_shooting_llcrr);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_LCR,
		djag_tune->thres_shooting_detect_cfg.thres_shooting_lcr);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_NEIGHBOR,
		djag_tune->thres_shooting_detect_cfg.thres_shooting_neighbor);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_0, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_UUCDD,
		djag_tune->thres_shooting_detect_cfg.thres_shooting_uucdd);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_UCD,
		djag_tune->thres_shooting_detect_cfg.thres_shooting_ucd);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_MIN_MAX_WEIGHT,
		djag_tune->thres_shooting_detect_cfg.min_max_weight);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_1, val);

	MCSC_SET_F(base, MCSC_R_YUV_DJAGFILTER_LFSR_SEED_0,
		MCSC_F_YUV_DJAGFILTER_LFSR_SEED_0, djag_tune->lfsr_seed_cfg.lfsr_seed_0);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGFILTER_LFSR_SEED_1,
		MCSC_F_YUV_DJAGFILTER_LFSR_SEED_1, djag_tune->lfsr_seed_cfg.lfsr_seed_1);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGFILTER_LFSR_SEED_2,
		MCSC_F_YUV_DJAGFILTER_LFSR_SEED_2, djag_tune->lfsr_seed_cfg.lfsr_seed_2);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_0,
		djag_tune->dither_cfg.dither_value[0]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_1,
		djag_tune->dither_cfg.dither_value[1]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_2,
		djag_tune->dither_cfg.dither_value[2]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_3,
		djag_tune->dither_cfg.dither_value[3]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_4,
		djag_tune->dither_cfg.dither_value[4]);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_04, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_5,
		djag_tune->dither_cfg.dither_value[5]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_6,
		djag_tune->dither_cfg.dither_value[6]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_7,
		djag_tune->dither_cfg.dither_value[7]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_8,
		djag_tune->dither_cfg.dither_value[8]);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_58, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_SAT_CTRL,
		djag_tune->dither_cfg.sat_ctrl);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_THRES,
		djag_tune->dither_cfg.dither_thres);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGFILTER_DITHER_THRES, val);

	MCSC_SET_F(base, MCSC_R_YUV_DJAGFILTER_CP_HF_THRES,
		MCSC_F_YUV_DJAGFILTER_CP_HF_THRES, djag_tune->cp_cfg.cp_hf_thres);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_CP_ARBI_MAX_COV_OFFSET,
		djag_tune->cp_cfg.cp_arbi_max_cov_offset);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_CP_ARBI_MAX_COV_SHIFT,
		djag_tune->cp_cfg.cp_arbi_max_cov_shift);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_CP_ARBI_DENOM,
		djag_tune->cp_cfg.cp_arbi_denom);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_CP_ARBI_MODE,
		djag_tune->cp_cfg.cp_arbi_mode);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_CP_ARBI, val);
#if IS_ENABLED(USE_DJAG_COEFF_CONFIG)
	is_scaler_set_djag_scaler_coef(base, &djag_tune->djag_sc_coef,
		&djag_tune->djag_sc_coef);
#endif
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_djag_tuning_param);

void is_scaler_set_predjag_tuning_param(struct pablo_mmio *base,
	const struct predjag_scaling_ratio_depended_config *predjag_tune)
{
	u32 val = 0;

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_WEIGHT0,
		predjag_tune->xfilter_dejagging_coeff_cfg.xfilter_dejagging_weight0);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_WEIGHT1,
		predjag_tune->xfilter_dejagging_coeff_cfg.xfilter_dejagging_weight1);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_XFILTER_HF_BOOST_WEIGHT,
		predjag_tune->xfilter_dejagging_coeff_cfg.xfilter_hf_boost_weight);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_CENTER_HF_BOOST_WEIGHT,
		predjag_tune->xfilter_dejagging_coeff_cfg.center_hf_boost_weight);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DIAGONAL_HF_BOOST_WEIGHT,
		predjag_tune->xfilter_dejagging_coeff_cfg.diagonal_hf_boost_weight);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_CENTER_WEIGHTED_MEAN_WEIGHT,
		predjag_tune->xfilter_dejagging_coeff_cfg.center_weighted_mean_weight);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_THRES_1X5_MATCHING_SAD,
		predjag_tune->thres_1x5_matching_cfg.thres_1x5_matching_sad);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_THRES_1X5_ABSHF,
		predjag_tune->thres_1x5_matching_cfg.thres_1x5_abshf);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_THRES_1X5_MATCHING, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_LLCRR,
		predjag_tune->thres_shooting_detect_cfg.thres_shooting_llcrr);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_LCR,
		predjag_tune->thres_shooting_detect_cfg.thres_shooting_lcr);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_NEIGHBOR,
		predjag_tune->thres_shooting_detect_cfg.thres_shooting_neighbor);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_0, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_UUCDD,
		predjag_tune->thres_shooting_detect_cfg.thres_shooting_uucdd);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_UCD,
		predjag_tune->thres_shooting_detect_cfg.thres_shooting_ucd);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_MIN_MAX_WEIGHT,
		predjag_tune->thres_shooting_detect_cfg.min_max_weight);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_1, val);

	MCSC_SET_F(base, MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_0,
		MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_0, predjag_tune->lfsr_seed_cfg.lfsr_seed_0);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_1,
		MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_1, predjag_tune->lfsr_seed_cfg.lfsr_seed_1);
	MCSC_SET_F(base, MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_2,
		MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_2, predjag_tune->lfsr_seed_cfg.lfsr_seed_2);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_0,
		predjag_tune->dither_cfg.dither_value[0]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_1,
		predjag_tune->dither_cfg.dither_value[1]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_2,
		predjag_tune->dither_cfg.dither_value[2]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_3,
		predjag_tune->dither_cfg.dither_value[3]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_4,
		predjag_tune->dither_cfg.dither_value[4]);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_04, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_5,
		predjag_tune->dither_cfg.dither_value[5]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_6,
		predjag_tune->dither_cfg.dither_value[6]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_7,
		predjag_tune->dither_cfg.dither_value[7]);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_8,
		predjag_tune->dither_cfg.dither_value[8]);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_58, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_SAT_CTRL,
		predjag_tune->dither_cfg.sat_ctrl);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_THRES,
		predjag_tune->dither_cfg.dither_thres);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_THRES, val);

	MCSC_SET_F(base, MCSC_R_YUV_DJAGPREFILTER_FILTER_CP_HF_THRES,
		MCSC_F_YUV_DJAGPREFILTER_CP_HF_THRES, predjag_tune->cp_cfg.cp_hf_thres);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MAX_COV_OFFSET,
		predjag_tune->cp_cfg.cp_arbi_max_cov_offset);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MAX_COV_SHIFT,
		predjag_tune->cp_cfg.cp_arbi_max_cov_shift);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_DENOM,
		predjag_tune->cp_cfg.cp_arbi_denom);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MODE,
		predjag_tune->cp_cfg.cp_arbi_mode);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_CP_ARBI, val);

	val = 0;
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_HARRIS_K,
		predjag_tune->harris_det_cfg.harris_k);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_HARRIS_TH,
		predjag_tune->harris_det_cfg.harris_th);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_HARRIS_DET, val);

	MCSC_SET_F(base, MCSC_R_YUV_DJAGPREFILTER_BILATERAL_C,
		MCSC_F_YUV_DJAGPREFILTER_BILATERAL_C, predjag_tune->bilateral_c_cfg.bilateral_c);
}

void is_scaler_set_djag_dither_wb(struct pablo_mmio *base, struct djag_wb_thres_cfg *djag_wb, u32 wht, u32 blk)
{
	u32 val;

	if (!djag_wb)
		return;

	val = MCSC_GET_R(base, MCSC_R_YUV_DJAG_DITHER_WB);

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_WHITE_LEVEL, wht);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_BLACK_LEVEL, blk);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGFILTER_DITHER_WB_THRES, djag_wb->dither_wb_thres);
	MCSC_SET_R(base, MCSC_R_YUV_DJAG_DITHER_WB, val);
}

void is_scaler_set_predjag_dither_wb(
	struct pablo_mmio *base, struct djag_wb_thres_cfg *djag_wb, u32 wht, u32 blk)
{
	u32 val;

	if (!djag_wb)
		return;

	val = MCSC_GET_R(base, MCSC_R_YUV_DJAGPREFILTER_DITHER_WB);

	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_WHITE_LEVEL, wht);
	val = MCSC_SET_V(base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_BLACK_LEVEL, blk);
	val = MCSC_SET_V(
		base, val, MCSC_F_YUV_DJAGPREFILTER_DITHER_WB_THRES, djag_wb->dither_wb_thres);
	MCSC_SET_R(base, MCSC_R_YUV_DJAGPREFILTER_DITHER_WB, val);
}

void is_scaler_check_predjag_condition(struct pablo_mmio *base, u32 in_width, u32 in_height,
	u32 *out_width, u32 *out_height)
{
	u32 hratio, vratio, min_ratio;

	if (*out_width < DJAG_PRE_LINE_BUF_SIZE)
		return;

	/* IQ's scale up guide for 8K scenario */
	hratio = GET_ZOOM_RATIO(in_width, *out_width);
	vratio = GET_ZOOM_RATIO(in_height, *out_height);
	min_ratio = min(hratio, vratio);

	if (min_ratio >= GET_ZOOM_RATIO(10, 16)) {
		/*
		 * [1x ~ 1.6x]
		 * disable PreDjag and PreScaler and bypass to PostDjag
		 */
		is_scaler_set_pre_enable(base, 0, 0);

		dbg_hw(2,
		       "Disable Dejag PreSC for bypass. in_width(%d) is 1.6 or less ratio than out_width(%d)\n",
		       in_width, *out_width);

		*out_width = in_width;
		*out_height = in_height;
	} else if (min_ratio >= GET_ZOOM_RATIO(10, 40)) {
		/*
		 * (1.6x ~ 4x]
		 * leave as much as 1.6 for poly scaler
		 */
		*out_width = round_down(*out_width * 10 / 16, MCSC_HEIGHT_ALIGN);
		*out_height = round_down(*out_height * 10 / 16, MCSC_HEIGHT_ALIGN);
	}
}

/* for CAC */
void is_scaler_set_cac_enable(struct pablo_mmio *base_addr, u32 en)
{
	/* not supported */
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_cac_enable);

void is_scaler_set_cac_map_crt_thr(struct pablo_mmio *base_addr, struct cac_cfg_by_ni *cfg)
{
	/* not supported */
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_cac_map_crt_thr);

/* LFRO : Less Fast Read Out */
void is_scaler_set_lfro_mode_enable(struct pablo_mmio *base, u32 hw_id, u32 lfro_enable, u32 lfro_total_fnum)
{
	MCSC_SET_F(base, MCSC_R_YUV_MAIN_CTRL_FRO_EN,
		MCSC_F_YUV_FRO_ENABLE, lfro_enable);
}

u32 is_scaler_get_stripe_align(u32 sbwc_type)
{
	return sbwc_type ? (MCSC_COMP_BLOCK_WIDTH * 2) : MCSC_WIDTH_ALIGN;
}

/* for Strip */
u32 is_scaler_get_djag_strip_enable(struct pablo_mmio *base, u32 output_id)
{
	return MCSC_GET_F(base, MCSC_R_YUV_DJAG_CTRL, MCSC_F_YUV_DJAGPS_PS_STRIP_ENABLE);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_djag_strip_enable);

void is_scaler_set_djag_strip_enable(struct pablo_mmio *base, u32 output_id, u32 enable)
{
	MCSC_SET_F(base, MCSC_R_YUV_DJAG_CTRL,
		MCSC_F_YUV_DJAGPS_PS_STRIP_ENABLE, enable);
}

u32 is_scaler_get_poly_strip_enable(struct pablo_mmio *base, u32 output_id)
{
	u32 sc_ctrl_reg, sc_strip_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC0_CTRL;
		sc_strip_enable_field = MCSC_F_YUV_POLY_SC0_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT1:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC1_CTRL;
		sc_strip_enable_field = MCSC_F_YUV_POLY_SC1_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT2:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC2_CTRL;
		sc_strip_enable_field = MCSC_F_YUV_POLY_SC2_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT3:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC3_CTRL;
		sc_strip_enable_field = MCSC_F_YUV_POLY_SC3_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT4:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC4_CTRL;
		sc_strip_enable_field = MCSC_F_YUV_POLY_SC4_STRIP_ENABLE;
		break;
	default:
		return 0;
	}

	return MCSC_GET_F(base, sc_ctrl_reg, sc_strip_enable_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_poly_strip_enable);

void is_scaler_set_poly_strip_enable(struct pablo_mmio *base, u32 output_id, u32 enable)
{
	u32 sc_ctrl_reg, sc_scrip_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC0_CTRL;
		sc_scrip_enable_field = MCSC_F_YUV_POLY_SC0_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT1:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC1_CTRL;
		sc_scrip_enable_field = MCSC_F_YUV_POLY_SC1_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT2:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC2_CTRL;
		sc_scrip_enable_field = MCSC_F_YUV_POLY_SC2_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT3:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC3_CTRL;
		sc_scrip_enable_field = MCSC_F_YUV_POLY_SC3_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT4:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC4_CTRL;
		sc_scrip_enable_field = MCSC_F_YUV_POLY_SC4_STRIP_ENABLE;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, sc_ctrl_reg,
		sc_scrip_enable_field, enable);
}

void is_scaler_get_poly_strip_config(struct pablo_mmio *base, u32 output_id, u32 *pre_dst_h, u32 *start_pos_h)
{
	u32 sc_strip_pre_dst_size_reg, sc_strip_pre_dst_size_h_field;
	u32 sc_strip_in_start_pos_reg, sc_strip_in_start_pos_h_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC0_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC0_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC0_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC0_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT1:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC1_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC1_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC1_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC1_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT2:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC2_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC2_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC2_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC2_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT3:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC3_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC3_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC3_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC3_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT4:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC4_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC4_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC4_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC4_STRIP_IN_START_POS_H;
		break;
	default:
		*pre_dst_h = 0;
		*start_pos_h = 0;
		return;
	}

	*pre_dst_h = MCSC_GET_F(base, sc_strip_pre_dst_size_reg,
			sc_strip_pre_dst_size_h_field);
	*start_pos_h = MCSC_GET_F(base, sc_strip_in_start_pos_reg,
			sc_strip_in_start_pos_h_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_poly_strip_config);

void is_scaler_set_poly_strip_config(struct pablo_mmio *base, u32 output_id, u32 pre_dst_h, u32 start_pos_h)
{
	u32 sc_strip_pre_dst_size_reg, sc_strip_pre_dst_size_h_field;
	u32 sc_strip_in_start_pos_reg, sc_strip_in_start_pos_h_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC0_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC0_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC0_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC0_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT1:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC1_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC1_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC1_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC1_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT2:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC2_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC2_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC2_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC2_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT3:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC3_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC3_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC3_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC3_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT4:
		sc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POLY_SC4_STRIP_PRE_DST_SIZE;
		sc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POLY_SC4_STRIP_PRE_DST_SIZE_H;
		sc_strip_in_start_pos_reg =
			MCSC_R_YUV_POLY_SC4_STRIP_IN_START_POS;
		sc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POLY_SC4_STRIP_IN_START_POS_H;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, sc_strip_pre_dst_size_reg,
		sc_strip_pre_dst_size_h_field, pre_dst_h);
	MCSC_SET_F(base, sc_strip_in_start_pos_reg,
		sc_strip_in_start_pos_h_field, start_pos_h);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_poly_strip_config);

u32 is_scaler_get_poly_out_crop_enable(struct pablo_mmio *base, u32 output_id)
{
	u32 sc_ctrl_reg, sc_out_crop_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC0_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC0_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT1:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC1_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC1_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT2:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC2_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC2_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT3:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC3_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC3_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT4:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC4_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC4_OUT_CROP_ENABLE;
		break;
	default:
		return 0;
	}

	return MCSC_GET_F(base, sc_ctrl_reg, sc_out_crop_enable_field);
}

void is_scaler_set_poly_out_crop_enable(struct pablo_mmio *base, u32 output_id, u32 enable)
{
	u32 sc_ctrl_reg, sc_out_crop_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC0_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC0_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT1:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC1_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC1_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT2:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC2_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC2_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT3:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC3_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC3_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT4:
		sc_ctrl_reg = MCSC_R_YUV_POLY_SC4_CTRL;
		sc_out_crop_enable_field =
			MCSC_F_YUV_POLY_SC4_OUT_CROP_ENABLE;
		break;
	default:
		return;
	}

	MCSC_SET_F(base, sc_ctrl_reg, sc_out_crop_enable_field, enable);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_poly_out_crop_enable);

void is_scaler_get_poly_out_crop_size(struct pablo_mmio *base, u32 output_id, u32 *width, u32 *height)
{
	u32 sc_out_crop_size_reg;
	u32 sc_out_crop_size_h_field, sc_out_crop_size_v_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC0_OUT_CROP_SIZE;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC0_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC0_OUT_CROP_SIZE_V;
		break;
	case MCSC_OUTPUT1:
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC1_OUT_CROP_SIZE;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC1_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC1_OUT_CROP_SIZE_V;
		break;
	case MCSC_OUTPUT2:
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC2_OUT_CROP_SIZE;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC2_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC2_OUT_CROP_SIZE_V;
		break;
	case MCSC_OUTPUT3:
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC3_OUT_CROP_SIZE;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC3_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC3_OUT_CROP_SIZE_V;
		break;
	case MCSC_OUTPUT4:
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC4_OUT_CROP_SIZE;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC4_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC4_OUT_CROP_SIZE_V;
		break;
	default:
		*width = 0;
		*height = 0;
		return;
	}

	*width = MCSC_GET_F(base, sc_out_crop_size_reg, sc_out_crop_size_h_field);
	*height = MCSC_GET_F(base, sc_out_crop_size_reg, sc_out_crop_size_v_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_poly_out_crop_size);

void is_scaler_set_poly_out_crop_size(struct pablo_mmio *base, u32 output_id,
	u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	u32 val;
	u32 sc_out_crop_pos_h_field, sc_out_crop_pos_v_field;
	u32 sc_out_crop_size_h_field, sc_out_crop_size_v_field;
	u32 sc_out_crop_pos_reg, sc_out_crop_size_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		sc_out_crop_pos_h_field = MCSC_F_YUV_POLY_SC0_OUT_CROP_POS_H;
		sc_out_crop_pos_v_field = MCSC_F_YUV_POLY_SC0_OUT_CROP_POS_V;
		sc_out_crop_pos_reg = MCSC_R_YUV_POLY_SC0_OUT_CROP_POS;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC0_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC0_OUT_CROP_SIZE_V;
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC0_OUT_CROP_SIZE;
		break;
	case MCSC_OUTPUT1:
		sc_out_crop_pos_h_field = MCSC_F_YUV_POLY_SC1_OUT_CROP_POS_H;
		sc_out_crop_pos_v_field = MCSC_F_YUV_POLY_SC1_OUT_CROP_POS_V;
		sc_out_crop_pos_reg = MCSC_R_YUV_POLY_SC1_OUT_CROP_POS;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC1_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC1_OUT_CROP_SIZE_V;
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC1_OUT_CROP_SIZE;
		break;
	case MCSC_OUTPUT2:
		sc_out_crop_pos_h_field = MCSC_F_YUV_POLY_SC2_OUT_CROP_POS_H;
		sc_out_crop_pos_v_field = MCSC_F_YUV_POLY_SC2_OUT_CROP_POS_V;
		sc_out_crop_pos_reg = MCSC_R_YUV_POLY_SC2_OUT_CROP_POS;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC2_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC2_OUT_CROP_SIZE_V;
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC2_OUT_CROP_SIZE;
		break;
	case MCSC_OUTPUT3:
		sc_out_crop_pos_h_field = MCSC_F_YUV_POLY_SC3_OUT_CROP_POS_H;
		sc_out_crop_pos_v_field = MCSC_F_YUV_POLY_SC3_OUT_CROP_POS_V;
		sc_out_crop_pos_reg = MCSC_R_YUV_POLY_SC3_OUT_CROP_POS;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC3_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC3_OUT_CROP_SIZE_V;
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC3_OUT_CROP_SIZE;
		break;
	case MCSC_OUTPUT4:
		sc_out_crop_pos_h_field = MCSC_F_YUV_POLY_SC4_OUT_CROP_POS_H;
		sc_out_crop_pos_v_field = MCSC_F_YUV_POLY_SC4_OUT_CROP_POS_V;
		sc_out_crop_pos_reg = MCSC_R_YUV_POLY_SC4_OUT_CROP_POS;
		sc_out_crop_size_h_field = MCSC_F_YUV_POLY_SC4_OUT_CROP_SIZE_H;
		sc_out_crop_size_v_field = MCSC_F_YUV_POLY_SC4_OUT_CROP_SIZE_V;
		sc_out_crop_size_reg = MCSC_R_YUV_POLY_SC4_OUT_CROP_SIZE;
		break;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, sc_out_crop_pos_h_field, pos_x);
	val = MCSC_SET_V(base, val, sc_out_crop_pos_v_field, pos_y);
	MCSC_SET_R(base, sc_out_crop_pos_reg, val);

	val = 0;
	val = MCSC_SET_V(base, val, sc_out_crop_size_h_field, width);
	val = MCSC_SET_V(base, val, sc_out_crop_size_v_field, height);
	MCSC_SET_R(base, sc_out_crop_size_reg, val);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_poly_out_crop_size);

u32 is_scaler_get_post_strip_enable(struct pablo_mmio *base, u32 output_id)
{
	u32 pc_ctrl_reg, pc_strip_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC0_CTRL;
		pc_strip_enable_field = MCSC_F_YUV_POSTPC_PC0_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT1:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC1_CTRL;
		pc_strip_enable_field = MCSC_F_YUV_POSTPC_PC1_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT2:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC2_CTRL;
		pc_strip_enable_field = MCSC_F_YUV_POSTPC_PC2_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return 0;
	}

	return MCSC_GET_F(base, pc_ctrl_reg, pc_strip_enable_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_post_strip_enable);

void is_scaler_set_post_strip_enable(struct pablo_mmio *base, u32 output_id, u32 enable)
{
	u32 pc_ctrl_reg, pc_strip_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC0_CTRL;
		pc_strip_enable_field = MCSC_F_YUV_POSTPC_PC0_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT1:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC1_CTRL;
		pc_strip_enable_field = MCSC_F_YUV_POSTPC_PC1_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT2:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC2_CTRL;
		pc_strip_enable_field = MCSC_F_YUV_POSTPC_PC2_STRIP_ENABLE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, pc_ctrl_reg, pc_strip_enable_field, enable);
}

void is_scaler_get_post_strip_config(struct pablo_mmio *base, u32 output_id, u32 *pre_dst_h, u32 *start_pos_h)
{
	u32 pc_strip_pre_dst_size_reg, pc_strip_pre_dst_size_h_field;
	u32 pc_strip_in_start_pos_reg, pc_strip_in_start_pos_h_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POST_PC0_STRIP_PRE_DST_SIZE;
		pc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POSTPC_PC0_STRIP_PRE_DST_SIZE_H;
		pc_strip_in_start_pos_reg =
			MCSC_R_YUV_POST_PC0_STRIP_IN_START_POS;
		pc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POSTPC_PC0_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT1:
		pc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POST_PC1_STRIP_PRE_DST_SIZE;
		pc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POSTPC_PC1_STRIP_PRE_DST_SIZE_H;
		pc_strip_in_start_pos_reg =
			MCSC_R_YUV_POST_PC1_STRIP_IN_START_POS;
		pc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POSTPC_PC1_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT2:
		pc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POST_PC2_STRIP_PRE_DST_SIZE;
		pc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POSTPC_PC2_STRIP_PRE_DST_SIZE_H;
		pc_strip_in_start_pos_reg =
			MCSC_R_YUV_POST_PC2_STRIP_IN_START_POS;
		pc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POSTPC_PC2_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		*pre_dst_h = 0;
		*start_pos_h = 0;
		return;
	}

	*pre_dst_h = MCSC_GET_F(base, pc_strip_pre_dst_size_reg,
			pc_strip_pre_dst_size_h_field);
	*start_pos_h = MCSC_GET_F(base, pc_strip_in_start_pos_reg,
			pc_strip_in_start_pos_h_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_post_strip_config);

void is_scaler_set_post_strip_config(struct pablo_mmio *base, u32 output_id, u32 pre_dst_h, u32 start_pos_h)
{
	u32 pc_strip_pre_dst_size_reg, pc_strip_pre_dst_size_h_field;
	u32 pc_strip_in_start_pos_reg, pc_strip_in_start_pos_h_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POST_PC0_STRIP_PRE_DST_SIZE;
		pc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POSTPC_PC0_STRIP_PRE_DST_SIZE_H;
		pc_strip_in_start_pos_reg =
			MCSC_R_YUV_POST_PC0_STRIP_IN_START_POS;
		pc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POSTPC_PC0_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT1:
		pc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POST_PC1_STRIP_PRE_DST_SIZE;
		pc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POSTPC_PC1_STRIP_PRE_DST_SIZE_H;
		pc_strip_in_start_pos_reg =
			MCSC_R_YUV_POST_PC1_STRIP_IN_START_POS;
		pc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POSTPC_PC1_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT2:
		pc_strip_pre_dst_size_reg =
			MCSC_R_YUV_POST_PC2_STRIP_PRE_DST_SIZE;
		pc_strip_pre_dst_size_h_field =
			MCSC_F_YUV_POSTPC_PC2_STRIP_PRE_DST_SIZE_H;
		pc_strip_in_start_pos_reg =
			MCSC_R_YUV_POST_PC2_STRIP_IN_START_POS;
		pc_strip_in_start_pos_h_field =
			MCSC_F_YUV_POSTPC_PC2_STRIP_IN_START_POS_H;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, pc_strip_pre_dst_size_reg,
		pc_strip_pre_dst_size_h_field, pre_dst_h);
	MCSC_SET_F(base, pc_strip_in_start_pos_reg,
		pc_strip_in_start_pos_h_field, start_pos_h);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_post_strip_config);

u32 is_scaler_get_post_out_crop_enable(struct pablo_mmio *base, u32 output_id)
{
	u32 pc_ctrl_reg, pc_out_crop_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC0_CTRL;
		pc_out_crop_enable_field = MCSC_F_YUV_POSTPC_PC0_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT1:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC1_CTRL;
		pc_out_crop_enable_field = MCSC_F_YUV_POSTPC_PC1_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT2:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC2_CTRL;
		pc_out_crop_enable_field = MCSC_F_YUV_POSTPC_PC2_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return 0;
	}

	return MCSC_GET_F(base, pc_ctrl_reg, pc_out_crop_enable_field);
}

void is_scaler_set_post_out_crop_enable(struct pablo_mmio *base, u32 output_id, u32 enable)
{
	u32 pc_ctrl_reg, pc_out_crop_enable_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC0_CTRL;
		pc_out_crop_enable_field = MCSC_F_YUV_POSTPC_PC0_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT1:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC1_CTRL;
		pc_out_crop_enable_field = MCSC_F_YUV_POSTPC_PC1_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT2:
		pc_ctrl_reg = MCSC_R_YUV_POST_PC2_CTRL;
		pc_out_crop_enable_field = MCSC_F_YUV_POSTPC_PC2_OUT_CROP_ENABLE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	MCSC_SET_F(base, pc_ctrl_reg, pc_out_crop_enable_field, enable);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_post_out_crop_enable);

void is_scaler_get_post_out_crop_size(struct pablo_mmio *base, u32 output_id, u32 *width, u32 *height)
{
	u32 pc_out_crop_size_reg;
	u32 pc_out_crop_size_h_field, pc_out_crop_size_v_field;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_out_crop_size_reg = MCSC_R_YUV_POST_PC0_OUT_CROP_SIZE;
		pc_out_crop_size_h_field = MCSC_F_YUV_POSTPC_PC0_OUT_CROP_SIZE_H;
		pc_out_crop_size_v_field = MCSC_F_YUV_POSTPC_PC0_OUT_CROP_SIZE_V;
		break;
	case MCSC_OUTPUT1:
		pc_out_crop_size_reg = MCSC_R_YUV_POST_PC1_OUT_CROP_SIZE;
		pc_out_crop_size_h_field = MCSC_F_YUV_POSTPC_PC1_OUT_CROP_SIZE_H;
		pc_out_crop_size_v_field = MCSC_F_YUV_POSTPC_PC1_OUT_CROP_SIZE_V;
		break;
	case MCSC_OUTPUT2:
		pc_out_crop_size_reg = MCSC_R_YUV_POST_PC2_OUT_CROP_SIZE;
		pc_out_crop_size_h_field = MCSC_F_YUV_POSTPC_PC2_OUT_CROP_SIZE_H;
		pc_out_crop_size_v_field = MCSC_F_YUV_POSTPC_PC2_OUT_CROP_SIZE_V;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		*width = 0;
		*height = 0;
		return;
	}

	*width = MCSC_GET_F(base, pc_out_crop_size_reg, pc_out_crop_size_h_field);
	*height = MCSC_GET_F(base, pc_out_crop_size_reg, pc_out_crop_size_v_field);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_post_out_crop_size);

void is_scaler_set_post_out_crop_size(struct pablo_mmio *base, u32 output_id,
	u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	u32 val;
	u32 pc_out_crop_pos_h_field, pc_out_crop_pos_v_field;
	u32 pc_out_crop_size_h_field, pc_out_crop_size_v_field;
	u32 pc_out_crop_pos_reg, pc_out_crop_size_reg;

	switch (output_id) {
	case MCSC_OUTPUT0:
		pc_out_crop_pos_h_field = MCSC_F_YUV_POSTPC_PC0_OUT_CROP_POS_H;
		pc_out_crop_pos_v_field = MCSC_F_YUV_POSTPC_PC0_OUT_CROP_POS_V;
		pc_out_crop_pos_reg = MCSC_R_YUV_POST_PC0_OUT_CROP_POS;
		pc_out_crop_size_h_field = MCSC_F_YUV_POSTPC_PC0_OUT_CROP_SIZE_H;
		pc_out_crop_size_v_field = MCSC_F_YUV_POSTPC_PC0_OUT_CROP_SIZE_V;
		pc_out_crop_size_reg = MCSC_R_YUV_POST_PC0_OUT_CROP_SIZE;
		break;
	case MCSC_OUTPUT1:
		pc_out_crop_pos_h_field = MCSC_F_YUV_POSTPC_PC1_OUT_CROP_POS_H;
		pc_out_crop_pos_v_field = MCSC_F_YUV_POSTPC_PC1_OUT_CROP_POS_V;
		pc_out_crop_pos_reg = MCSC_R_YUV_POST_PC1_OUT_CROP_POS;
		pc_out_crop_size_h_field = MCSC_F_YUV_POSTPC_PC1_OUT_CROP_SIZE_H;
		pc_out_crop_size_v_field = MCSC_F_YUV_POSTPC_PC1_OUT_CROP_SIZE_V;
		pc_out_crop_size_reg = MCSC_R_YUV_POST_PC1_OUT_CROP_SIZE;
		break;
	case MCSC_OUTPUT2:
		pc_out_crop_pos_h_field = MCSC_F_YUV_POSTPC_PC2_OUT_CROP_POS_H;
		pc_out_crop_pos_v_field = MCSC_F_YUV_POSTPC_PC2_OUT_CROP_POS_V;
		pc_out_crop_pos_reg = MCSC_R_YUV_POST_PC2_OUT_CROP_POS;
		pc_out_crop_size_h_field = MCSC_F_YUV_POSTPC_PC2_OUT_CROP_SIZE_H;
		pc_out_crop_size_v_field = MCSC_F_YUV_POSTPC_PC2_OUT_CROP_SIZE_V;
		pc_out_crop_size_reg = MCSC_R_YUV_POST_PC2_OUT_CROP_SIZE;
		break;
	case MCSC_OUTPUT3:
		fallthrough;
	case MCSC_OUTPUT4:
		fallthrough;
	default:
		return;
	}

	val = 0;
	val = MCSC_SET_V(base, val, pc_out_crop_pos_h_field, pos_x);
	val = MCSC_SET_V(base, val, pc_out_crop_pos_v_field, pos_y);
	MCSC_SET_R(base, pc_out_crop_pos_reg, val);

	val = 0;
	val = MCSC_SET_V(base, val, pc_out_crop_size_h_field, width);
	val = MCSC_SET_V(base, val, pc_out_crop_size_v_field, height);
	MCSC_SET_R(base, pc_out_crop_size_reg, val);
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_post_out_crop_size);

void is_scaler_set_hf_radial_config(struct pablo_mmio *base, struct mcsc_radial_cfg *radial_cfg,
	struct hf_setfile_contents *hf_setfile, u32 width, u32 height)
{
	u32 binning_x, binning_y;
	u32 sensor_center_x, sensor_center_y;
	u32 radial_center_x, radial_center_y;
	u32 offset_x, offset_y;

	if (!hf_setfile->gain_enable) {
		is_scaler_set_djag_hf_radial_gain(base, 0, 0);
		return;
	}

	is_scaler_set_djag_hf_radial_gain(base, hf_setfile->gain_enable, hf_setfile->gain_increment);

	binning_x = radial_cfg->sensor_binning_x * radial_cfg->bns_binning_x * 1024ULL *
			radial_cfg->taa_crop_width / width / 1000 / 1000;
	binning_y = radial_cfg->sensor_binning_y * radial_cfg->bns_binning_y * 1024ULL *
			radial_cfg->taa_crop_height / height / 1000 / 1000;

	is_scaler_set_djag_hf_radial_binning(base, binning_x, binning_y);

	sensor_center_x = (radial_cfg->sensor_full_width >> 1) & (~0x1);
	sensor_center_y = (radial_cfg->sensor_full_height >> 1) & (~0x1);

	offset_x = radial_cfg->sensor_crop_x + (radial_cfg->bns_binning_x * radial_cfg->taa_crop_x / 1000);
	offset_y = radial_cfg->sensor_crop_y + (radial_cfg->bns_binning_y * radial_cfg->taa_crop_y / 1000);

	radial_center_x = -sensor_center_x + radial_cfg->sensor_binning_x * offset_x / 1000;
	radial_center_y = -sensor_center_y + radial_cfg->sensor_binning_y * offset_y / 1000;

	is_scaler_set_djag_hf_radial_center(base, radial_center_x, radial_center_y);

	is_scaler_set_djag_hf_radial_biquad_factor(base, hf_setfile->biquad_factor_a, hf_setfile->biquad_factor_b);
	is_scaler_set_djag_hf_radial_biquad_scal_shift_adder(base, hf_setfile->gain_shift_adder);
}

/* HF: High Frequency */
int is_scaler_set_hf_config(struct is_common_dma *dma, struct pablo_mmio *base_addr, u32 hw_id,
	bool hf_enable, struct param_mcs_output *hf_param, struct mcsc_radial_cfg *radial_cfg,
	struct hf_setfile_contents *hf_setfile, u32 *payload_size)
{
	u32 y_stride = hf_param->dma_stride_y;
	u32 sbwc_header_stride = is_hw_dma_get_header_stride(hf_param->full_output_width,
		MCSC_HF_COMP_BLOCK_WIDTH, 16);

	/* Read HF from MCSC RDMA0 */
	CALL_DMA_OPS(dma, dma_enable, hf_enable);
	is_scaler_set_rdma_format(base_addr, hw_id, hf_param->dma_format);
	is_scaler_set_rdma_sbwc_config(dma, base_addr, hf_param, &y_stride, payload_size);
	CALL_DMA_OPS(dma, dma_set_size, hf_param->width, hf_param->height);
	CALL_DMA_OPS(dma, dma_set_img_stride, y_stride, 0, 0);
	CALL_DMA_OPS(dma, dma_set_header_stride, sbwc_header_stride, 0);
	/* Enable DJAG HF Recomposition */
	is_scaler_set_djag_hf_enable(base_addr, hf_enable, 0);

	is_scaler_set_hf_radial_config(base_addr, radial_cfg, hf_setfile, hf_param->width, hf_param->height);

	return 0;
}
KUNIT_EXPORT_SYMBOL(is_scaler_set_hf_config);

void is_scaler_clear_intr_src(struct pablo_mmio *base, u32 hw_id, u32 status)
{
	MCSC_SET_R(base, MCSC_R_INT_REQ_INT0_CLEAR, status);
}

u32 is_scaler_get_intr_mask(struct pablo_mmio *base, u32 hw_id)
{
	int ret;

	ret = MCSC_GET_R(base, MCSC_R_INT_REQ_INT0_ENABLE);

	return ~ret;
}

u32 is_scaler_get_intr_status(struct pablo_mmio *base, u32 hw_id)
{
	u32 int0;

	int0 = MCSC_GET_R(base, MCSC_R_INT_REQ_INT0);

	dbg_hw(2, "%s : INT0(0x%x)", __func__, int0);

	return int0;
}

u32 is_scaler_handle_extended_intr(u32 status)
{
	return 0;
}

u32 is_scaler_get_version(struct pablo_mmio *base)
{
	return MCSC_GET_R(base, MCSC_R_IP_VERSION);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_version);

u32 is_scaler_get_idle_status(struct pablo_mmio *base, u32 hw_id)
{
	int ret;

	ret = MCSC_GET_F(base, MCSC_R_IDLENESS_STATUS, MCSC_F_IDLENESS_STATUS);

	return ret;
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_idle_status);

u32 is_scaler_get_post_pc_enable(struct pablo_mmio *base)
{
	return MCSC_GET_F(base, MCSC_R_YUV_POST_PC0_CTRL, MCSC_F_YUV_POSTPC_PC0_ENABLE);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_post_pc_enable);

u32 is_scaler_get_flip_type(struct pablo_mmio *base)
{
	return MCSC_GET_F(
		base, MCSC_R_YUV_WDMASC_W0_FLIP_CONTROL, MCSC_F_YUV_WDMASC_W0_FLIP_CONTROL);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_flip_type);

u32 is_scaler_get_cinfifo_ctrl(struct pablo_mmio *base)
{
	return MCSC_GET_R(base, MCSC_R_IP_USE_OTF_PATH_01) &&
	       MCSC_GET_F(base, MCSC_R_YUV_CINFIFO_ENABLE, MCSC_F_YUV_CINFIFO_ENABLE) &&
	       MCSC_GET_F(base, MCSC_R_YUV_CINFIFO_CONFIG,
		       MCSC_F_YUV_CINFIFO_STALL_BEFORE_FRAME_START_EN) &&
	       MCSC_GET_F(base, MCSC_R_YUV_CINFIFO_CONFIG, MCSC_F_YUV_CINFIFO_DEBUG_EN) &&
	       MCSC_GET_F(base, MCSC_R_IP_USE_CINFIFO_NEW_FRAME_IN,
		       MCSC_F_IP_USE_CINFIFO_NEW_FRAME_IN);
}
KUNIT_EXPORT_SYMBOL(is_scaler_get_cinfifo_ctrl);

static const struct is_reg mcsc_dbg_cr[] = {
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
	{0x0210, "YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_WIDTH"},
	{0x0214, "YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_HEIGHT"},
	/* Chain Path */
	{0x0080,"IP_USE_OTF_PATH_01"},
	{0x007c,"IP_USE_OTF_PATH_23"},
	{0x0078,"IP_USE_OTF_PATH_45"},
	{0x0074,"IP_USE_OTF_PATH_67"},
	{0x0240, "YUV_MAIN_CTRL_FRO_EN"},
	{0x0244, "YUV_MAIN_CTRL_INPUT_MONO_EN"},
	/* CMDQ Frame Counter */
	{0x04a8, "CMDQ_FRAME_COUNTER"},
	/* CINFIFO 0 Status */
	{0x1000, "YUV_CINFIFO_ENABLE"},
	{0x1014, "YUV_CINFIFO_STATUS"},
	{0x1018, "YUV_CINFIFO_INPUT_CNT"},
	{0x101c, "YUV_CINFIFO_STALL_CNT"},
	{0x1020, "YUV_CINFIFO_FIFO_FULLNESS"},
	{0x1040, "YUV_CINFIFO_INT"},
	/* CINFIFO 1 Status */
	{0x1100, "STAT_CINFIFOHF_ENABLE"},
	{0x1114, "STAT_CINFIFOHF_STATUS"},
	{0x1118, "STAT_CINFIFOHF_INPUT_CNT"},
	{0x111c, "STAT_CINFIFOHF_STALL_CNT"},
	{0x1120, "STAT_CINFIFOHF_FIFO_FULLNESS"},
	{0x1140, "STAT_CINFIFOHF_INT"},
};

static void mcsc_hw_dump_dbg_state(struct pablo_mmio *pmio)
{
	void *ctx;
	const struct is_reg *cr;
	u32 i, val;

	ctx = pmio->ctx ? pmio->ctx : (void *)pmio;
	pmio->reg_read(ctx, MCSC_R_IP_VERSION, &val);

	is_dbg("[HW:%s] v%02u.%02u.%02u ======================================\n", pmio->name,
		(val >> 24) & 0xff, (val >> 16) & 0xff, val & 0xffff);
	for (i = 0; i < ARRAY_SIZE(mcsc_dbg_cr); i++) {
		cr = &mcsc_dbg_cr[i];

		pmio->reg_read(ctx, cr->sfr_offset, &val);
		is_dbg("[HW:%s]%40s %08x\n", pmio->name, cr->reg_name, val);
	}
	is_dbg("[HW:%s]=================================================\n", pmio->name);
}

void is_scaler_dump(struct pablo_mmio *pmio, u32 mode)
{
	switch (mode) {
	case HW_DUMP_CR:
		info_hw("[MCSC]%s:DUMP CR\n", __FILENAME__);
		is_hw_dump_regs(pmio_get_base(pmio), mcsc_regs, MCSC_REG_CNT);
		break;
	case HW_DUMP_DBG_STATE:
		info_hw("[MCSC]%s:DUMP DBG_STATE\n", __FILENAME__);
		mcsc_hw_dump_dbg_state(pmio);
		break;
	default:
		err_hw("[MCSC]%s:Not supported dump_mode %u", __FILENAME__, mode);
		break;
	}
}
KUNIT_EXPORT_SYMBOL(is_scaler_dump);

void mcsc_hw_s_dtp(struct pablo_mmio *base, enum mcsc_dtp_id id, u32 enable,
	enum mcsc_dtp_type type, u32 y, u32 u, u32 v, enum mcsc_dtp_color_bar cb)
{
	u32 offset = MCSC_R_YUV_DTPRDMAIN_BYPASS - MCSC_R_YUV_DTPOTFIN_BYPASS;

	if (enable) {
		MCSC_SET_R(base, MCSC_R_YUV_DTPOTFIN_BYPASS + offset * id, 0);
		MCSC_SET_R(base, MCSC_R_YUV_DTPOTFIN_TEST_PATTERN_MODE + offset * id, type);
		if (type == MCSC_DTP_SOLID_IMAGE) {
			MCSC_SET_R(base, MCSC_R_YUV_DTPOTFIN_TEST_DATA_Y + offset * id, y);
			MCSC_SET_R(base, MCSC_R_YUV_DTPOTFIN_TEST_DATA_U + offset * id, u);
			MCSC_SET_R(base, MCSC_R_YUV_DTPOTFIN_TEST_DATA_V + offset * id, v);
		} else {
			MCSC_SET_R(base, MCSC_R_YUV_DTPOTFIN_YUV_STANDARD + offset * id, cb);
		}
	} else {
		MCSC_SET_R(base, MCSC_R_YUV_DTPOTFIN_BYPASS + offset * id, 1);
	}
}

int mcsc_hw_g_rdma_max_cnt(void)
{
	return MCSC_RDMA_MAX;
}

int mcsc_hw_g_wdma_max_cnt(void)
{
	return MCSC_WDMA_MAX;
}

u32 mcsc_hw_g_reg_cnt(void)
{
	return MCSC_REG_CNT;
}
KUNIT_EXPORT_SYMBOL(mcsc_hw_g_reg_cnt);

void mcsc_hw_init_pmio_config(struct pmio_config *cfg)
{
	cfg->num_corexs = 2;
	cfg->corex_stride = 0x8000;

	cfg->volatile_table = &mcsc_volatile_ranges_table;
	cfg->wr_noinc_table = &mcsc_wr_noinc_ranges_table;

	cfg->max_register = MCSC_R_DBG_SC_35;
	cfg->num_reg_defaults_raw = (MCSC_R_DBG_SC_35 >> 2) + 1;
	cfg->dma_addr_shift = 4;

	cfg->ranges = mcsc_range_cfgs;
	cfg->num_ranges = ARRAY_SIZE(mcsc_range_cfgs);

	cfg->fields = mcsc_field_descs;
	cfg->num_fields = ARRAY_SIZE(mcsc_field_descs);
}
KUNIT_EXPORT_SYMBOL(mcsc_hw_init_pmio_config);
