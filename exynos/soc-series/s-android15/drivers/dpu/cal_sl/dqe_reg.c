// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/iopoll.h>
#include <cal_config.h>
#include <regs-dqe.h>
#include <regs-dpp.h>
#include <regs-decon.h>
#include <dqe_cal.h>
#include <decon_cal.h>
#include <linux/dma-buf.h>

static struct cal_regs_desc regs_dqe[REGS_DQE_TYPE_MAX][REGS_DQE_ID_MAX];

#define dqe_regs_desc(id)			(&regs_dqe[REGS_DQE][id])
#define dqe_read(id, offset)	cal_read(dqe_regs_desc(id), offset)
#define dqe_write(id, offset, val)	cal_write(dqe_regs_desc(id), offset, val)
#define dqe_read_mask(id, offset, mask)		\
		cal_read_mask(dqe_regs_desc(id), offset, mask)
#define dqe_write_mask(id, offset, val, mask)	\
		cal_write_mask(dqe_regs_desc(id), offset, val, mask)
#define dqe_write_relaxed(id, offset, val)		\
		cal_write_relaxed(dqe_regs_desc(id), offset, val)

#define dma_regs_desc(id)			(&regs_dqe[REGS_EDMA][id])
#define dma_read(id, offset)	cal_read(dma_regs_desc(id), offset)
#define dma_write(id, offset, val)	cal_write(dma_regs_desc(id), offset, val)
#define dma_read_mask(id, offset, mask)		\
		cal_read_mask(dma_regs_desc(id), offset, mask)
#define dma_write_mask(id, offset, val, mask)	\
		cal_write_mask(dma_regs_desc(id), offset, val, mask)
#define dma_write_relaxed(id, offset, val)		\
		cal_write_relaxed(dma_regs_desc(id), offset, val)

/****************** EDMA CAL functions ******************/
void edma_reg_set_irq_mask_all(u32 id, u32 en)
{
#if defined(EDMA_CGC_IRQ)
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, EDMA_CGC_IRQ, val, CGC_ALL_IRQ_MASK);
#endif
}

void edma_reg_set_irq_enable(u32 id, u32 en)
{
#if defined(EDMA_CGC_IRQ)
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, EDMA_CGC_IRQ, val, CGC_IRQ_ENABLE);
#endif
}

void edma_reg_clear_irq_all(u32 id)
{
#if defined(EDMA_CGC_IRQ)
	dma_write_mask(id, EDMA_CGC_IRQ, ~0, CGC_ALL_IRQ_CLEAR);
#endif
}

u32 edma_reg_get_interrupt_and_clear(u32 id)
{
	u32 val = 0;

#if defined(EDMA_CGC_IRQ)
	val = dqe_read(id, EDMA_CGC_IRQ);
	edma_reg_clear_irq_all(id);

	if (val & CGC_CONFIG_ERR_IRQ)
		cal_log_err(id, "CGC_CONFIG_ERR_IRQ\n");
	if (val & CGC_READ_SLAVE_ERR_IRQ)
		cal_log_err(id, "CGC_READ_SLAVE_ERR_IRQ\n");
	if (val & CGC_DEADLOCK_IRQ)
		cal_log_err(id, "CGC_DEADLOCK_IRQ\n");
	if (val & CGC_FRAME_DONE_IRQ)
		cal_log_info(id, "CGC_FRAME_DONE_IRQ\n");
#endif
	return val;
}

void edma_reg_set_sw_reset(u32 id)
{
#if defined(EDMA_CGC_ENABLE)
	dma_write_mask(id, EDMA_CGC_ENABLE, ~0, CGC_SRESET);
#endif
}

void edma_reg_set_start(u32 id, u32 en)
{
#if defined(EDMA_CGC_ENABLE) && defined(EDMA_CGC_IN_CTRL_1)
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, EDMA_CGC_IN_CTRL_1, val, CGC_START_EN_SET0);
	dma_write_mask(id, EDMA_CGC_ENABLE, val, CGC_START);
#endif
}

void edma_reg_set_base_addr(u32 id, dma_addr_t addr)
{
#if defined(EDMA_CGC_BASE_ADDR_SET0)
	dma_write(id, EDMA_CGC_BASE_ADDR_SET0, addr);
#endif
}

void __dqe_dump(u32 id, struct dqe_regs *regs, bool en_lut)
{
	void __iomem *dqe_base_regs = regs->dqe_base_regs;
	void __iomem *edma_base_regs = regs->edma_base_regs;

	cal_log_info(id, "=== DQE_TOP SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + 0x0000, 0x100);
	cal_log_info(id, "=== DQE_TOP SHADOW SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + 0x0000 + SHADOW_DQE_OFFSET, 0x100);

	cal_log_info(id, "=== DQE_GAMMA_MATRIX SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_GAMMA_MATRIX_CON, 0x20);

	cal_log_info(id, "=== DQE_DEGAMMA SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_DEGAMMA_CON, 0x100);

	cal_log_info(id, "=== DQE_LINEAR_MATRIX SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_LINEAR_MATRIX_CON, 0x20);

	cal_log_info(id, "=== DQE_CGC SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_CGC_CON, 0x40);

	cal_log_info(id, "=== DQE_REGAMMA SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_REGAMMA_CON, 0x200);

	cal_log_info(id, "=== DQE_CGC_DITHER SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_CGC_DITHER_CON, 0x20);

	cal_log_info(id, "=== DQE_DISP_DITHER SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_CGC_DITHER_CON, 0x20);

	cal_log_info(id, "=== DQE_SCL SFR DUMP ===\n");
	dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_SCL_SCALED_IMG_SIZE, 0x200);

	if (en_lut) {
		cal_log_info(id, "=== DQE_CGC_LUT SFR DUMP ===\n");
		cal_log_info(id, "== [ RED CGC LUT ] ==\n");
		dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_CGC_LUT_R(0), 0x2664);
		cal_log_info(id, "== [ GREEN CGC LUT ] ==\n");
		dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_CGC_LUT_G(0), 0x2664);
		cal_log_info(id, "== [ BLUE CGC LUT ] ==\n");
		dpu_print_hex_dump(dqe_base_regs, dqe_base_regs + DQE_CGC_LUT_B(0), 0x2664);
	}

	if (edma_base_regs) {
		cal_log_info(id, "=== EDMA CGC SFR DUMP ===\n");
		dpu_print_hex_dump(edma_base_regs, edma_base_regs + 0x0000, 0x48);
	}
}

void dqe_reg_set_cgc_lut(u16 (*ctx)[3], u32 (*lut)[DQE_CGC_LUT_MAX])
{
	int i, rgb;

	for (i = 0; i < DQE_CGC_LUT_MAX; i++)
		for (rgb = 0; rgb < 3; rgb++)
			ctx[i][rgb] = (u16)lut[rgb][i]; // diff align between SFR and DMA
}

void dqe_reg_set_cgc_con(u32 *ctx, u32 *lut)
{
	/* DQE0_CGC_MC1_R0*/
	ctx[0] = (
		CGC_MC_ON_R(lut[1]) | CGC_MC_INVERSE_R(lut[2]) |
		CGC_MC_GAIN_R(lut[3]) | CGC_MC_BC_HUE_R(lut[4]) |
		CGC_MC_BC_SAT_R(lut[5]) | CGC_MC_BC_VAL_R(lut[6])
	);

	/* Dummy */
	ctx[1] = 0;

	/* DQE0_CGC_MC3_R0*/
	ctx[2] = (CGC_MC_S1_R(lut[7]) | CGC_MC_S2_R(lut[8]));

	/* DQE0_CGC_MC4_R0*/
	ctx[3] = (CGC_MC_H1_R(lut[9]) | CGC_MC_H2_R(lut[10]));

	/* DQE0_CGC_MC5_R0*/
	ctx[4] = (CGC_MC_V1_R(lut[11]) | CGC_MC_V2_R(lut[12]));

	/* DQE0_CGC_MC1_R1*/
	ctx[5] = (
		CGC_MC_ON_R(lut[13]) | CGC_MC_INVERSE_R(lut[14]) |
		CGC_MC_GAIN_R(lut[15]) | CGC_MC_BC_HUE_R(lut[16]) |
		CGC_MC_BC_SAT_R(lut[17]) | CGC_MC_BC_VAL_R(lut[18])
	);

	/* Dummy */
	ctx[6] = 0;

	/* DQE0_CGC_MC3_R1*/
	ctx[7] = (CGC_MC_S1_R(lut[19]) | CGC_MC_S2_R(lut[20]));

	/* DQE0_CGC_MC4_R1*/
	ctx[8] = (CGC_MC_H1_R(lut[21]) | CGC_MC_H2_R(lut[22]));

	/* DQE0_CGC_MC5_R1*/
	ctx[9] = (CGC_MC_V1_R(lut[23]) | CGC_MC_V2_R(lut[24]));


	/* DQE0_CGC_MC1_R2*/
	ctx[10] = (
		CGC_MC_ON_R(lut[25]) | CGC_MC_INVERSE_R(lut[26]) |
		CGC_MC_GAIN_R(lut[27]) | CGC_MC_BC_HUE_R(lut[28]) |
		CGC_MC_BC_SAT_R(lut[29]) | CGC_MC_BC_VAL_R(lut[30])
	);

	/* retain DQE0_CGC_MC2_R2*/

	/* DQE0_CGC_MC3_R2*/
	ctx[12] = (CGC_MC_S1_R(lut[31]) | CGC_MC_S2_R(lut[32]));

	/* DQE0_CGC_MC4_R2*/
	ctx[13] = (CGC_MC_H1_R(lut[33]) | CGC_MC_H2_R(lut[34]));

	/* DQE0_CGC_MC5_R2*/
	ctx[14] = (CGC_MC_V1_R(lut[35]) | CGC_MC_V2_R(lut[36]));
}

void dqe_reg_set_cgc_dither(u32 *ctx, u32 *lut, int bit_ext)
{
	/* DQE0_CGC_DITHER*/
	*ctx = (
		CGC_DITHER_EN(lut[0]) | CGC_DITHER_MODE(lut[1]) |
		CGC_DITHER_MASK_SPIN(lut[2]) | CGC_DITHER_MASK_SEL(lut[3], 0) |
		CGC_DITHER_MASK_SEL(lut[4], 1) | CGC_DITHER_MASK_SEL(lut[5], 2) |
		CGC_DITHER_BIT((bit_ext < 0) ? lut[6] : bit_ext) |
		CGC_DITHER_FRAME_OFFSET(lut[7])
	);
}

/*
 * If Gamma Matrix is transferred with this pattern,
 *
 * |A B C D|
 * |E F G H|
 * |I J K L|
 * |M N O P|
 *
 * Coeff and Offset will be applied as follows.
 *
 * |Rout|   |A E I||Rin|   |M|
 * |Gout| = |B F J||Gin| + |N|
 * |Bout|   |C G K||Bin|   |O|
 *
 */
static void dqe_reg_adjust_matrix_coef(u32 *dst, int *src, u32 shift)
{
	int row_sum[3] = {0, 0, 0};
	int row_sum_max = 0, row_sum_min = INT_MAX;
	int row_sum_expected = (65536 >> shift);
	int i;

	for (i = 0; i < 12; i++)
		dst[i] = (src[i] >> shift);

	for (i = 0; i < 3; i++) {
		row_sum[i] = (int)dst[i] + (int)dst[i + 4] + (int)dst[i + 8];
		row_sum_max = max(row_sum_max, row_sum[i]);
		row_sum_min = min(row_sum_min, row_sum[i]);
	}

	if (row_sum_min == row_sum_expected && row_sum_max == row_sum_expected)
		return;

	if (row_sum_min < (row_sum_expected - 2) || row_sum_max > row_sum_expected)
		return;

	if (row_sum_max > (row_sum_min + 2))
		return;

	if (row_sum[0] == (row_sum_expected - 2)) {
		dst[4]++;
		dst[8]++;
	} else if (row_sum[0] == (row_sum_expected - 1)) {
		dst[0]++;
	}

	if (row_sum[1] == (row_sum_expected - 2)) {
		dst[1]++;
		dst[9]++;
	} else if (row_sum[1] == (row_sum_expected - 1)) {
		dst[5]++;
	}

	if (row_sum[2] == (row_sum_expected - 2)) {
		dst[2]++;
		dst[6]++;
	} else if (row_sum[2] == (row_sum_expected - 1)) {
		dst[10]++;
	}
	/*
	 *cal_log_info(0, "adjusted [%d,%d,%d,%d,%d,%d,%d,%d,%d]",
	 *					(int)dst[0], (int)dst[1], (int)dst[2],
	 *					(int)dst[4], (int)dst[5], (int)dst[6],
	 *					(int)dst[8], (int)dst[9], (int)dst[10]);
	 */
}

static int fixed_multiply(int a, int b)
{
	const int fixed_point = 16;
	const int64_t rounding_factor = (int64_t)BIT(fixed_point - 1);

	return ((int64_t) a * b + rounding_factor) >> fixed_point;
}

void dqe_reg_multiply_matrix4(int *res, int *lut0, int *lut1)
{
	int row, col, inner, idx, idx0, idx1;
	int *lut;

	if (lut0[0] == 0 || lut1[0] == 0) {
		lut = (lut1[0] > 0) ? lut1 : lut0;
		memcpy(res, lut, 17 * sizeof(*lut));
		return;
	}

	res[0] = lut0[0];
	for (row = 0; row < 4; row++) {
		for (col = 0; col < 4; col++) {
			idx = row * 4 + col + 1;

			res[idx] = 0;
			for (inner = 0; inner < 4; inner++) {
				idx0 = row * 4 + inner + 1;
				idx1 = inner * 4 + col + 1;

				res[idx] += fixed_multiply(lut0[idx0], lut1[idx1]);
			}
		}
	}
}

void dqe_reg_set_gamma_matrix(u32 *ctx, int *lut, u32 shift)
{
	int i;
	u32 gamma_mat_lut[17] = {0,};
	int bound, tmp;

	gamma_mat_lut[0] = lut[0];

	/* Coefficient */
	dqe_reg_adjust_matrix_coef(&gamma_mat_lut[1], &lut[1], 5);

	/* Offset */
	bound = 1023 >> shift; // 10bit: -1023~1023, 8bit: -255~255
	for (i = 12; i < 16; i++) {
		tmp = (lut[i + 1] >> (6+shift));
		if (tmp > bound)
			tmp = bound;
		if (tmp < -bound)
			tmp = -bound;
		gamma_mat_lut[i + 1] = tmp;
	}
	ctx[0] = GAMMA_MATRIX_EN(gamma_mat_lut[0]);

	/* GAMMA_MATRIX_COEFF */
	ctx[1] = (
		GAMMA_MATRIX_COEFF_H(gamma_mat_lut[5]) | GAMMA_MATRIX_COEFF_L(gamma_mat_lut[1])
	);

	ctx[2] = (
		GAMMA_MATRIX_COEFF_H(gamma_mat_lut[2]) | GAMMA_MATRIX_COEFF_L(gamma_mat_lut[9])
	);

	ctx[3] = (
		GAMMA_MATRIX_COEFF_H(gamma_mat_lut[10]) | GAMMA_MATRIX_COEFF_L(gamma_mat_lut[6])
	);

	ctx[4] = (
		GAMMA_MATRIX_COEFF_H(gamma_mat_lut[7]) | GAMMA_MATRIX_COEFF_L(gamma_mat_lut[3])
	);

	ctx[5] = GAMMA_MATRIX_COEFF_L(gamma_mat_lut[11]);

	/* GAMMA_MATRIX_OFFSET */
	ctx[6] = (
		GAMMA_MATRIX_OFFSET_1(gamma_mat_lut[14]) | GAMMA_MATRIX_OFFSET_0(gamma_mat_lut[13])
	);

	ctx[7] = GAMMA_MATRIX_OFFSET_2(gamma_mat_lut[15]);
}

void dqe_reg_set_linear_matrix(u32 *ctx, int *lut, u32 shift)
{
	int i;
	u32 linear_mat_lut[17] = {0,};
	int bound, tmp;

	linear_mat_lut[0] = lut[0];

	/* Coefficient - value range : -32768 ~ 32767 */
	dqe_reg_adjust_matrix_coef(&linear_mat_lut[1], &lut[1], 3);

	/* Offset - value range : -8192 ~ 8191 */
	bound = 4095 >> shift; // 10bit: -4095~4095, 8bit: -1023~1023
	for (i = 12; i < 16; i++) {
		tmp = (lut[i + 1] >> (4+shift));
		if (tmp > bound)
			tmp = bound;
		if (tmp < -bound)
			tmp = -bound;
		linear_mat_lut[i + 1] = tmp;
	}
	ctx[0] = LINEAR_MATRIX_EN(linear_mat_lut[0]);

	/* LINEAR_MATRIX_COEFF */
	ctx[1] = (
		LINEAR_MATRIX_COEFF_H(linear_mat_lut[5]) | LINEAR_MATRIX_COEFF_L(linear_mat_lut[1])
	);

	ctx[2] = (
		LINEAR_MATRIX_COEFF_H(linear_mat_lut[2]) | LINEAR_MATRIX_COEFF_L(linear_mat_lut[9])
	);

	ctx[3] = (
		LINEAR_MATRIX_COEFF_H(linear_mat_lut[10]) | LINEAR_MATRIX_COEFF_L(linear_mat_lut[6])
	);

	ctx[4] = (
		LINEAR_MATRIX_COEFF_H(linear_mat_lut[7]) | LINEAR_MATRIX_COEFF_L(linear_mat_lut[3])
	);

	ctx[5] = LINEAR_MATRIX_COEFF_L(linear_mat_lut[11]);

	/* LINEAR_MATRIX_OFFSET */
	ctx[6] = (
		LINEAR_MATRIX_OFFSET_1(linear_mat_lut[14]) | LINEAR_MATRIX_OFFSET_0(linear_mat_lut[13])
	);

	ctx[7] = LINEAR_MATRIX_OFFSET_2(linear_mat_lut[15]);
}

void dqe_reg_set_gamma_lut(u32 *ctx, u32 *lut, u32 shift)
{
	int i, idx;
	u32 lut_l, lut_h;
	u32 *luts = &lut[1];

	ctx[0] = REGAMMA_EN(lut[0]);
	for (i = 1, idx = 0; i < DQE_REGAMMA_REG_MAX; i++) {
		lut_h = 0;
		lut_l = REGAMMA_LUT_L(luts[idx]>>shift);
		idx++;

		if (((DQE_REGAMMA_LUT_CNT%2) == 0) || ((idx % DQE_REGAMMA_LUT_CNT) != 0)) {
			lut_h = REGAMMA_LUT_H(luts[idx]>>shift);
			idx++;
		}

		ctx[i] = (lut_h | lut_l);
	}
}

void dqe_reg_set_degamma_lut(u32 *ctx, u32 *lut, u32 shift)
{
	int i, idx;
	u32 lut_l, lut_h;
	u32 *luts = &lut[1];

	ctx[0] = DEGAMMA_EN(lut[0]);
	for (i = 1, idx = 0; i < DQE_DEGAMMA_REG_MAX; i++) {
		lut_h = 0;
		lut_l = DEGAMMA_LUT_L(luts[idx]>>shift);
		idx++;

		if (((DQE_DEGAMMA_LUT_CNT%2) == 0) || ((idx % DQE_DEGAMMA_LUT_CNT) != 0)) {
			lut_h = DEGAMMA_LUT_H(luts[idx]>>shift);
			idx++;
		}

		ctx[i] = (lut_h | lut_l);
	}
}

void dqe_reg_set_scl_lut(u32 *ctx, u32 *lut)
{
	/* 8-tap Filter Coefficient */
	const s16 ps_h_c_8t[1][16][8] = {
		{	/* Ratio <= 65536 (~8:8) Selection = 0 */
			{   0,	 0,   0, 512,	0,   0,   0,   0 },
			{  -2,	 8, -25, 509,  30,  -9,   2,  -1 },
			{  -4,	14, -46, 499,  64, -19,   5,  -1 },
			{  -5,	20, -62, 482, 101, -30,   8,  -2 },
			{  -6,	23, -73, 458, 142, -41,  12,  -3 },
			{  -6,	25, -80, 429, 185, -53,  15,  -3 },
			{  -6,	26, -83, 395, 228, -63,  19,  -4 },
			{  -6,	25, -82, 357, 273, -71,  21,  -5 },
			{  -5,	23, -78, 316, 316, -78,  23,  -5 },
			{  -5,	21, -71, 273, 357, -82,  25,  -6 },
			{  -4,	19, -63, 228, 395, -83,  26,  -6 },
			{  -3,	15, -53, 185, 429, -80,  25,  -6 },
			{  -3,	12, -41, 142, 458, -73,  23,  -6 },
			{  -2,	 8, -30, 101, 482, -62,  20,  -5 },
			{  -1,	 5, -19,  64, 499, -46,  14,  -4 },
			{  -1,	 2,  -9,  30, 509, -25,   8,  -2 }
		},
	};

	/* 4-tap Filter Coefficient */
	const s16 ps_v_c_4t[1][16][4] = {
		{	/* Ratio <= 65536 (~8:8) Selection = 0 */
			{   0, 512,   0,   0 },
			{ -15, 508,  20,  -1 },
			{ -25, 495,  45,  -3 },
			{ -31, 473,  75,  -5 },
			{ -33, 443, 110,  -8 },
			{ -33, 408, 148, -11 },
			{ -31, 367, 190, -14 },
			{ -27, 324, 234, -19 },
			{ -23, 279, 279, -23 },
			{ -19, 234, 324, -27 },
			{ -14, 190, 367, -31 },
			{ -11, 148, 408, -33 },
			{  -8, 110, 443, -33 },
			{  -5,	75, 473, -31 },
			{  -3,	45, 495, -25 },
			{  -1,	20, 508, -15 }
		},
	};
	int i, j, cnt, idx = 0;
	u32 luts[DQE_SCL_LUT_MAX];

	for (cnt = 0; cnt < DQE_SCL_COEF_CNT; cnt++) {
		for (j = 0; j < DQE_SCL_VCOEF_MAX; j++)
			for (i = 0; i < DQE_SCL_COEF_SET; i++)
				luts[idx++] = ps_v_c_4t[0][i][j];

		for (j = 0; j < DQE_SCL_HCOEF_MAX; j++)
			for (i = 0; i < DQE_SCL_COEF_SET; i++)
				luts[idx++] = ps_h_c_8t[0][i][j];
	}

	if (lut[0]) {
		for (cnt = 0, idx = 0; cnt < DQE_SCL_COEF_CNT; cnt++)
			for (j = 1; j <= DQE_SCL_COEF_MAX; j++, idx += DQE_SCL_COEF_SET)
				luts[idx] = lut[j];
	}

	for (i = 0; i < DQE_SCL_REG_MAX; i++)
		ctx[i] = SCL_COEF(luts[i]) & SCL_COEF_MASK;
}

u32 dqe_reg_get_version(u32 id)
{
	return dqe_read(id, DQE_TOP_VER);
}

static inline u32 dqe_reg_get_lut_addr(u32 id, enum dqe_reg_type type,
					u32 index, u32 opt)
{
	u32 addr = 0;

	switch (type) {
	case DQE_REG_LINEAR_MATRIX:
		if (index >= DQE_LINEAR_MATRIX_REG_MAX)
			return 0;
		addr = DQE_LINEAR_MATRIX_LUT(index);
		break;
	case DQE_REG_GAMMA_MATRIX:
		if (index >= DQE_GAMMA_MATRIX_REG_MAX)
			return 0;
		addr = DQE_GAMMA_MATRIX_LUT(index);
		break;
	case DQE_REG_CGC_DITHER:
		addr = DQE_CGC_DITHER_CON;
		break;
	case DQE_REG_CGC_CON:
		if (index >= DQE_CGC_CON_REG_MAX)
			return 0;
		addr = DQE_CGC_MC_R0(index);
		break;
	case DQE_REG_CGC:
		if (index >= DQE_CGC_REG_MAX)
			return 0;
		switch (opt) {
		case DQE_REG_CGC_R:
			addr = DQE_CGC_LUT_R(index);
			break;
		case DQE_REG_CGC_G:
			addr = DQE_CGC_LUT_G(index);
			break;
		case DQE_REG_CGC_B:
			addr = DQE_CGC_LUT_B(index);
			break;
		}
		break;
	case DQE_REG_DEGAMMA:
		if (index >= DQE_DEGAMMA_REG_MAX)
			return 0;
		addr = DQE_DEGAMMA_LUT(index);
		break;
	case DQE_REG_REGAMMA:
		if (index >= DQE_REGAMMA_REG_MAX)
			return 0;
		addr = DQE_REGAMMA_LUT(index);
		break;
	case DQE_REG_SCL:
		if (index >= DQE_SCL_REG_MAX)
			return 0;
		addr = DQE_SCL_Y_VCOEF(index);
		break;
	case DQE_REG_LPD:
		if (index >= DQE_LPD_REG_MAX)
			return 0;
		addr = DQE_TOP_LPD(index);
		break;
	case DQE_REG_DISP_DITHER:
		return 0;
	default:
		cal_log_debug(id, "invalid reg type %d\n", type);
		return 0;
	}

	return addr;
}

u32 dqe_reg_get_lut(u32 id, enum dqe_reg_type type, u32 index, u32 opt)
{
	u32 addr = dqe_reg_get_lut_addr(id, type, index, opt);

	if (addr == 0)
		return 0;
	return dqe_read(id, addr);
}

void dqe_reg_set_lut(u32 id, enum dqe_reg_type type, u32 index, u32 value, u32 opt)
{
	u32 addr = dqe_reg_get_lut_addr(id, type, index, opt);

	if (addr == 0)
		return;
	dqe_write_relaxed(id, addr, value);
}

void dqe_reg_set_lut_on(u32 id, enum dqe_reg_type type, u32 on)
{
	u32 addr;
	u32 value;
	u32 mask;

	switch (type) {
	case DQE_REG_LINEAR_MATRIX:
		addr = DQE_LINEAR_MATRIX_CON;
		value = LINEAR_MATRIX_EN(on);
		mask = LINEAR_MATRIX_EN_MASK;
		break;
	case DQE_REG_GAMMA_MATRIX:
		addr = DQE_GAMMA_MATRIX_CON;
		value = GAMMA_MATRIX_EN(on);
		mask = GAMMA_MATRIX_EN_MASK;
		break;
	case DQE_REG_CGC_DITHER:
		addr = DQE_CGC_DITHER_CON;
		value = CGC_DITHER_EN(on);
		mask = CGC_DITHER_EN_MASK;
		break;
	case DQE_REG_CGC_CON:
		addr = DQE_CGC_CON;
		value = CGC_EN(on);
		mask = CGC_EN_MASK;
		break;
	case DQE_REG_CGC_DMA:
		addr = DQE_CGC_CON;
		value = CGC_COEF_DMA_REQ(on);
		mask = CGC_COEF_DMA_REQ_MASK;
		break;
	case DQE_REG_DEGAMMA:
		addr = DQE_DEGAMMA_CON;
		value = DEGAMMA_EN(on);
		mask = DEGAMMA_EN_MASK;
		break;
	case DQE_REG_REGAMMA:
		addr = DQE_REGAMMA_CON;
		value = REGAMMA_EN(on);
		mask = REGAMMA_EN_MASK;
		break;
	case DQE_REG_LPD:
		addr = DQE_TOP_LPD_MODE_CONTROL;
		value = DQE_LPD_MODE_EXIT(on);
		mask = DQE_LPD_MODE_EXIT_MASK;
		break;
	case DQE_REG_DISP_DITHER:
		return;
	default:
		cal_log_debug(id, "invalid reg type %d\n", type);
		return;
	}

	dqe_write_mask(id, addr, value, mask);
}

int dqe_reg_wait_cgc_dmareq(u32 id)
{
#if defined(CGC_COEF_DMA_REQ_MASK)
	u32 val;
	int ret;

	ret = readl_poll_timeout_atomic(dqe_regs_desc(id)->regs + DQE_CGC_CON,
			val, !(val & CGC_COEF_DMA_REQ_MASK), 10, 10000); /* timeout 10ms */
	if (ret) {
		cal_log_err(id, "[edma] timeout dma reg\n");
		return ret;
	}
#endif
	return 0;
}

void dqe_regs_desc_init(u32 id, void __iomem *regs, const char *name,
		enum dqe_regs_type type)
{
	regs_dqe[type][id].regs = regs;
	regs_dqe[type][id].name = name;
}

static void dqe_reg_set_img_size(u32 id, struct dqe_reg_rect *in)
{
	u32 val;

	val = DQE_FULL_IMG_VSIZE(in->height) | DQE_FULL_IMG_HSIZE(in->width);
	dqe_write_relaxed(id, DQE_TOP_FRM_SIZE, val);

	val = DQE_FULL_PXL_NUM(in->width * in->height);
	dqe_write_relaxed(id, DQE_TOP_FRM_PXL_NUM, val);

	val = DQE_PARTIAL_START_Y(0) | DQE_PARTIAL_START_X(0);
	dqe_write_relaxed(id, DQE_TOP_PARTIAL_START, val);

	val = DQE_IMG_VSIZE(in->height) | DQE_IMG_HSIZE(in->width);
	dqe_write_relaxed(id, DQE_TOP_IMG_SIZE, val);

	val = DQE_PARTIAL_UPDATE_EN(0);
	dqe_write_relaxed(id, DQE_TOP_PARTIAL_CON, val);
}

static void dqe_reg_set_scaled_img_size(u32 id, struct dqe_reg_rect *in, struct dqe_reg_rect *out)
{
	u32 addr;
	int h_ratio = in->width * (1 << 20) / out->width;
	int v_ratio = in->height * (1 << 20) / out->height;

	addr = DQE_SCL_SCALED_IMG_SIZE;
	dqe_write_relaxed(id, addr, SCALED_IMG_HEIGHT(out->height) | SCALED_IMG_WIDTH(out->width));

	addr = DQE_SCL_MAIN_H_RATIO;
	dqe_write_mask(id, addr, H_RATIO(h_ratio), H_RATIO_MASK);

	addr = DQE_SCL_MAIN_V_RATIO;
	dqe_write_mask(id, addr, V_RATIO(v_ratio), V_RATIO_MASK);
}

/* exposed to driver layer for DQE CAL APIs */
void dqe_reg_init(u32 id, struct dqe_reg_rect *in, struct dqe_reg_rect *out)
{
	cal_log_debug(id, "%s +\n", __func__);
	dqe_reg_set_img_size(id, in);
	dqe_reg_set_scaled_img_size(id, in, out);
	decon_reg_set_dqe_enable(id, true);
	cal_log_debug(id, "%s -\n", __func__);
}
