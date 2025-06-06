/* SPDX-License-Identifier: GPL-2.0-only
 *
 * cal_8845/regs-dqe.h
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Dongho Lee <dh205.lee@samsung.com>
 *
 * Register definition file for Samsung Display Pre-Processor driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REGS_DQE_H
#define _REGS_DQE_H

enum dqe_regs_id {
	REGS_DQE0_ID = 0,
	REGS_DQE1_ID,
	REGS_DQE_ID_MAX
};

enum dqe_regs_type {
	REGS_DQE = 0,
	REGS_EDMA,
	REGS_DQE_TYPE_MAX
};

#define SHADOW_DQE_OFFSET		0x0200

/* DQE_TOP */
#define DQE_TOP_CORE_SECURITY		(0x0000)
#define DQE_CORE_SECURITY		(0x1 << 0)

#define DQE_TOP_IMG_SIZE		(0x0004)
#define DQE_IMG_VSIZE(_v)		((_v) << 16)
#define DQE_IMG_VSIZE_MASK		(0x3FFF << 16)
#define DQE_IMG_HSIZE(_v)		((_v) << 0)
#define DQE_IMG_HSIZE_MASK		(0x3FFF << 0)

#define DQE_TOP_FRM_SIZE		(0x0008)
#define DQE_FULL_IMG_VSIZE(_v)		((_v) << 16)
#define DQE_FULL_IMG_VSIZE_MASK		(0x3FFF << 16)
#define DQE_FULL_IMG_HSIZE(_v)		((_v) << 0)
#define DQE_FULL_IMG_HSIZE_MASK		(0x3FFF << 0)

#define DQE_TOP_FRM_PXL_NUM		(0x000C)
#define DQE_FULL_PXL_NUM(_v)		((_v) << 0)
#define DQE_FULL_PXL_NUM_MASK		(0xFFFFFFF << 0)

#define DQE_TOP_PARTIAL_START		(0x0010)
#define DQE_PARTIAL_START_Y(_v)	((_v) << 16)
#define DQE_PARTIAL_START_Y_MASK	(0x3FFF << 16)
#define DQE_PARTIAL_START_X(_v)	((_v) << 0)
#define DQE_PARTIAL_START_X_MASK	(0x3FFF << 0)

#define DQE_TOP_PARTIAL_CON		(0x0014)
#define DQE_PARTIAL_HE_UPDATE_METHOD(_v)	((_v) << 8)
#define DQE_PARTIAL_HE_UPDATE_METHOD_MASK	(0x1 << 8)
#define DQE_PARTIAL_OFF_MODE(_v)	((_v) << 4)
#define DQE_PARTIAL_OFF_MODE_MASK	(0x1 << 4)
#define DQE_PARTIAL_UPDATE_EN(_v)	((_v) << 0)
#define DQE_PARTIAL_UPDATE_EN_MASK	(0x1 << 0)

/*----------------------[TOP_LPD_MODE]---------------------------------------*/
#define DQE_TOP_LPD_MODE_CONTROL	(0x0018)
#define CGC_LPD_MODE_EXIT(_v)		((_v) << 1)
#define CGC_LPD_MODE_EXIT_MASK		(0x1 << 1)
#define DQE_LPD_MODE_EXIT(_v)		((_v) << 0)
#define DQE_LPD_MODE_EXIT_MASK		(0x1 << 0)

#define DQE_TOP_LPD_ATC_CON		(0x001C)
#define LPD_ATC_PARTIAL_HE_UPDATE_METHOD(_v)	((_v) << 8)
#define LPD_ATC_PARTIAL_HE_UPDATE_METHOD_MASK	(0x1 << 8)
#define LPD_ATC_PARTIAL_ON(_v)		((_v) << 7)
#define LPD_ATC_PARTIAL_ON_MASK		(0x1 << 7)
#define LPD_ATC_FRAME_CNT(_v)		((_v) << 3)
#define LPD_ATC_FRAME_CNT_MASK		(0xF << 3)
#define LPD_ATC_DIMMING_PROG(_v)	((_v) << 2)
#define LPD_ATC_DIMMING_PROG_MASK	(0x1 << 2)
#define LPD_ATC_FRAME_STATE(_v)		((_v) << 0)
#define LPD_ATC_FRAME_STATE_MASK	(0x3 << 0)

#define DQE_TOP_LPD_PREV_ATC_STRENGTH	(0x0020)
#define LPD_PREV_PREV_ATC_STRENGTH(_v)	((_v) << 16)
#define LPD_PREV_PREV_ATC_STRENGTH_MASK	(0x7FF << 16)
#define LPD_PREV_ATC_STRENGTH(_v)	((_v) << 0)
#define LPD_PREV_ATC_STRENGTH_MASK	(0x7FF << 0)

#define DQE_TOP_LPD_CGC_CON		(0x0024)
#define LPD_CGC_DITHER_SEED(_v)		((_v) << 0)
#define LPD_CGC_DITHER_SEED_MASK	(0xFFFF << 0)

#define DQE_TOP_LPD(_n)			(DQE_TOP_LPD_ATC_CON + ((_n) * 0x4))
#define DQE_LPD_REG_MAX			(3)

/*----------------------[VERSION]---------------------------------------------*/
#define DQE_TOP_VER			(0x01FC)
#define TOP_VER				(0x08100000)
#define TOP_VER_GET(_v)			(((_v) >> 0) & 0xFFFFFFFF)

/*----------------------[ATC]-------------------------------------------------*/
#define DQE_ATC_CONTROL			(0x0400)
#define ATC_SW_RESET(_v)		(((_v) & 0x1) << 16)

#define DQE_ATC_ENABLE			(0x0404)
#define ATC_PIXMAP_EN(_v)		(((_v) & 0x1) << 4)
#define ATC_EN(_v)			(((_v) & 0x1) << 0)
#define ATC_EN_MASK			(0x1 << 0)

#define DQE_ATC_DIMMING_DONE_INTR	(0x0408)
#define ATC_DIMMING_IN_PROGRESS(_v)	(((_v) & 0x1) << 0)

#define DQE_ATC_BLEND_WEIGHT		(0x040C)
#define ATC_HE_BLEND_WEIGHT(_v)		(((_v) & 0x1FF) << 16)
#define ATC_TMO_BLEND_WEIGHT(_v)	(((_v) & 0x1FF) << 0)

#define DQE_ATC_HIST_CLIP		(0x0410)
#define ATC_EXCEPTION_HIST_CLIP(_v)	(((_v) & 0xF) << 16)
#define ATC_BRIGHT_HIST_CLIP(_v)	(((_v) & 0xF) << 8)
#define ATC_DARK_HIST_CLIP(_v)		(((_v) & 0xF) << 0)

#define DQE_ATC_HE_MODIFICATION_TH	(0x0414)
#define ATC_DATA_GHE_THRESHOLD_BITNUM(_v) (((_v) & 0xF) << 16)
#define ATC_DATA_LHE_THRESHOLD_BITNUM(_v) (((_v) & 0xF) << 0)

#define DQE_ATC_DIMMING_STEP		(0x0418)
#define ATC_ST_DSTEP(_v)		(((_v) & 0xFF) << 8)
#define ATC_HE_DSTEP(_v)		(((_v) & 0xFF) << 0)

#define DQE_ATC_STRENGTH		(0x041C)
#define ATC_ATM_STRENGTH(_v)		(((_v) & 0x1FF) << 16)
#define ATC_ATC_STRENGTH(_v)		(((_v) & 0x7FF) << 0)

#define DQE_ATC_LHE_BLOCK_SIZE_DIV_X	(0x0420)
#define ATC_LHE_BLOCK_SIZE_DIV_X(_v)	(((_v) & 0xFFFFF) << 0)

#define DQE_ATC_LHE_BLOCK_SIZE_DIV_Y	(0x0424)
#define ATC_LHE_BLOCK_SIZE_DIV_Y(_v)	(((_v) & 0xFFFFF) << 0)

#define DQE_ATC_CDF_DIVIDER_GHE		(0x0428)
#define ATC_CDF_DIVIDER_GHE(_v)		(((_v) & 0xFFFFF) << 0)

#define DQE_ATC_CDF_DIVIDER_LHE		(0x042C)
#define ATC_CDF_DIVIDER_LHE(_v)		(((_v) & 0xFFFFF) << 0)

#define DQE_ATC_COUNTER_BIT		(0x0430)
#define ATC_COUNTER_BIT_LHE(_v)		(((_v) & 0xF) << 8)
#define ATC_COUNTER_BIT_GHE(_v)		(((_v) & 0xF) << 0)

#define DQE_ATC_CLIP_WEIGHT		(0x0434)
#define ATC_MIN_CLIP_WEIGHT(_v)		(((_v) & 0xFF) << 0)

#define DQE_ATC_CHECKER_THRESHOLD	(0x0438)
#define ATC_CHECKER_THRESHOLD(_v)	(((_v) & 0xF) << 0)

#define DQE_ATC_DITHER_CON		(0x0480)
#define ATC_DITHER_FRAME_OFFSET(_v)	(((_v) & 0xF) << 12)
#define ATC_DITHER_MASK_SEL_R(_v)	(((_v) & 0x1) << 7)
#define ATC_DITHER_MASK_SEL_G(_v)	(((_v) & 0x1) << 6)
#define ATC_DITHER_MASK_SEL_B(_v)	(((_v) & 0x1) << 5)
#define ATC_DITHER_MASK_SPIN(_v)	(((_v) & 0x1) << 3)
#define ATC_DITHER_MODE(_v)		(((_v) & 0x3) << 1)
#define ATC_DITHER_EN(_v)		(((_v) & 0x1) << 0)

#define DQE_ATC_CONTROL_LUT(_n)		(DQE_ATC_ENABLE + ((_n) * 0x4))

#define DQE_ATC_REG_MAX			(33)
#define DQE_ATC_LUT_MAX			(22)

/*----------------------[HSC]-------------------------------------------------*/
#define DQE_HSC_CONTROL			(0x0800)
#define HSC_EN(_v)			(((_v) & 0x1) << 0)
#define HSC_EN_MASK			(0x1 << 0)

#define DQE_HSC_LOCAL_CONTROL		(0x0804)
#define HSC_LVC_ON(_v)			(((_v) & 0x1) << 8)
#define HSC_LHC_ON(_v)			(((_v) & 0x1) << 4)
#define HSC_LSC_ON(_v)			(((_v) & 0x1) << 0)

#define DQE_HSC_GLOBAL_CONTROL_1	(0x0808)
#define HSC_GHC_GAIN(_v)		(((_v) & 0x7ff) << 16)
#define HSC_GHC_ON(_v)			(((_v) & 0x1) << 0)

#define DQE_HSC_GLOBAL_CONTROL_0	(0x080C)
#define HSC_GSC_GAIN(_v)		(((_v) & 0x7ff) << 16)
#define HSC_GSC_ON(_v)			(((_v) & 0x1) << 0)

#define DQE_HSC_GLOBAL_CONTROL_2	(0x0810)
#define HSC_GVC_GAIN(_v)		(((_v) & 0x7ff) << 16)
#define HSC_GVC_ON(_v)			(((_v) & 0x1) << 0)

#define DQE_HSC_CONTROL_MC1_R0		(0x081C)
#define HSC_MC_SAT_GAIN_R0(_v)		(((_v) & 0x7ff) << 16)
#define HSC_MC_BC_VAL_R0(_v)		(((_v) & 0x3) << 12)
#define HSC_MC_BC_SAT_R0(_v)		(((_v) & 0x3) << 8)
#define HSC_MC_BC_HUE_R0(_v)		(((_v) & 0x3) << 4)
#define HSC_MC_ON_R0(_v)		(((_v) & 0x1) << 0)

#define DQE_HSC_CONTROL_MC2_R0		(0x0820)
#define HSC_MC_VAL_GAIN_R0(_v)		(((_v) & 0x7ff) << 16)
#define HSC_MC_HUE_GAIN_R0(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_CONTROL_MC3_R0		(0x0824)
#define HSC_MC_S2_R0(_v)		(((_v) & 0x3ff) << 16)
#define HSC_MC_S1_R0(_v)		(((_v) & 0x3ff) << 0)

#define DQE_HSC_CONTROL_MC4_R0		(0x0828)
#define HSC_MC_H2_R0(_v)		(((_v) & 0x1fff) << 16)
#define HSC_MC_H1_R0(_v)		(((_v) & 0x1fff) << 0)

#define DQE_HSC_CONTROL_MC5_R0		(0x082C)
#define HSC_MC_V2_R0(_v)		(((_v) & 0x3ff) << 16)
#define HSC_MC_V1_R0(_v)		(((_v) & 0x3ff) << 0)

#define DQE_HSC_CONTROL_MC1_R1		(0x0830)
#define HSC_MC_SAT_GAIN_R1(_v)		(((_v) & 0x7ff) << 16)
#define HSC_MC_BC_VAL_R1(_v)		(((_v) & 0x3) << 12)
#define HSC_MC_BC_SAT_R1(_v)		(((_v) & 0x3) << 8)
#define HSC_MC_BC_HUE_R1(_v)		(((_v) & 0x3) << 4)
#define HSC_MC_ON_R1(_v)		(((_v) & 0x1) << 0)

#define DQE_HSC_CONTROL_MC2_R1		(0x0834)
#define HSC_MC_VAL_GAIN_R1(_v)		(((_v) & 0x7ff) << 16)
#define HSC_MC_HUE_GAIN_R1(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_CONTROL_MC3_R1		(0x0838)
#define HSC_MC_S2_R1(_v)		(((_v) & 0x3ff) << 16)
#define HSC_MC_S1_R1(_v)		(((_v) & 0x3ff) << 0)

#define DQE_HSC_CONTROL_MC4_R1		(0x083C)
#define HSC_MC_H2_R1(_v)		(((_v) & 0x1fff) << 16)
#define HSC_MC_H1_R1(_v)		(((_v) & 0x1fff) << 0)

#define DQE_HSC_CONTROL_MC5_R1		(0x0840)
#define HSC_MC_V2_R1(_v)		(((_v) & 0x3ff) << 16)
#define HSC_MC_V1_R1(_v)		(((_v) & 0x3ff) << 0)

#define DQE_HSC_CONTROL_MC1_R2		(0x0844)
#define HSC_MC_SAT_GAIN_R2(_v)		(((_v) & 0x7ff) << 16)
#define HSC_MC_BC_VAL_R2(_v)		(((_v) & 0x3) << 12)
#define HSC_MC_BC_SAT_R2(_v)		(((_v) & 0x3) << 8)
#define HSC_MC_BC_HUE_R2(_v)		(((_v) & 0x3) << 4)
#define HSC_MC_ON_R2(_v)		(((_v) & 0x1) << 0)

#define DQE_HSC_CONTROL_MC2_R2		(0x0848)
#define HSC_MC_VAL_GAIN_R2(_v)		(((_v) & 0x7ff) << 16)
#define HSC_MC_HUE_GAIN_R2(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_CONTROL_MC3_R2		(0x084C)
#define HSC_MC_S2_R2(_v)		(((_v) & 0x3ff) << 16)
#define HSC_MC_S1_R2(_v)		(((_v) & 0x3ff) << 0)

#define DQE_HSC_CONTROL_MC4_R2		(0x0850)
#define HSC_MC_H2_R2(_v)		(((_v) & 0x1fff) << 16)
#define HSC_MC_H1_R2(_v)		(((_v) & 0x1fff) << 0)

#define DQE_HSC_CONTROL_MC5_R2		(0x0854)
#define HSC_MC_V2_R2(_v)		(((_v) & 0x3ff) << 16)
#define HSC_MC_V1_R2(_v)		(((_v) & 0x3ff) << 0)

#define DQE_HSC_POLY_S(_n)		(0x09D8 + ((_n) * 0x4))
#define HSC_POLY_CURVE_S_H(_v)		(((_v) & 0x3ff) << 16)
#define HSC_POLY_CURVE_S_L(_v)		(((_v) & 0x3ff) << 0)

#define DQE_HSC_POLY_V(_n)		(0x09EC + ((_n) * 0x4))
#define HSC_POLY_CURVE_V_H(_v)		(((_v) & 0x3ff) << 16)
#define HSC_POLY_CURVE_V_L(_v)		(((_v) & 0x3ff) << 0)

#define DQE_HSC_LSC_GAIN_P1(_n)		(0x0C00 + ((_n) * 0x4))
#define HSC_LSC_GAIN_P1_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LSC_GAIN_P1_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_LSC_GAIN_P2(_n)		(0x0C60 + ((_n) * 0x4))
#define HSC_LSC_GAIN_P2_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LSC_GAIN_P2_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_LSC_GAIN_P3(_n)		(0x0CC0 + ((_n) * 0x4))
#define HSC_LSC_GAIN_P3_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LSC_GAIN_P3_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_LHC_GAIN_P1(_n)		(0x0D20 + ((_n) * 0x4))
#define HSC_LHC_GAIN_P1_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LHC_GAIN_P1_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_LHC_GAIN_P2(_n)		(0x0D80 + ((_n) * 0x4))
#define HSC_LHC_GAIN_P2_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LHC_GAIN_P2_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_LHC_GAIN_P3(_n)		(0x0DE0 + ((_n) * 0x4))
#define HSC_LHC_GAIN_P3_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LHC_GAIN_P3_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_LVC_GAIN_P1(_n)		(0x0E40 + ((_n) * 0x4))
#define HSC_LVC_GAIN_P1_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LVC_GAIN_P1_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_LVC_GAIN_P2(_n)		(0x0EA0 + ((_n) * 0x4))
#define HSC_LVC_GAIN_P2_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LVC_GAIN_P2_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_LVC_GAIN_P3(_n)		(0x0F00 + ((_n) * 0x4))
#define HSC_LVC_GAIN_P3_H(_v)		(((_v) & 0x7ff) << 16)
#define HSC_LVC_GAIN_P3_L(_v)		(((_v) & 0x7ff) << 0)

#define DQE_HSC_CONTROL_LUT(_n)		(0x0800 + ((_n) * 0x4))
#define DQE_HSC_POLY_LUT(_n)		(0x09D8 + ((_n) * 0x4))
#define DQE_HSC_LSC_GAIN_LUT(_n)	(0x0C00 + ((_n) * 0x4))
#define DQE_HSC_LHC_GAIN_LUT(_n)	(0x0D20 + ((_n) * 0x4))
#define DQE_HSC_LBC_GAIN_LUT(_n)	(0x0E40 + ((_n) * 0x4))

#define DQE_HSC_REG_CTRL_MAX		22
#define DQE_HSC_REG_POLY_MAX		10
#define DQE_HSC_REG_LSC_GAIN_MAX	72
#define DQE_HSC_REG_GAIN_MAX		(DQE_HSC_REG_LSC_GAIN_MAX*3)
#define DQE_HSC_REG_MAX			(DQE_HSC_REG_CTRL_MAX + DQE_HSC_REG_POLY_MAX + DQE_HSC_REG_GAIN_MAX) //248
#define DQE_HSC_LUT_CTRL_MAX		49
#define DQE_HSC_LUT_POLY_MAX		18
#define DQE_HSC_LUT_LSC_GAIN_MAX	144
#define DQE_HSC_LUT_GAIN_MAX		(DQE_HSC_LUT_LSC_GAIN_MAX*3)
#define DQE_HSC_LUT_MAX			(DQE_HSC_LUT_CTRL_MAX + DQE_HSC_LUT_POLY_MAX + DQE_HSC_LUT_GAIN_MAX) //548

/*----------------------[GAMMA_MATRIX]---------------------------------------*/
#define DQE_GAMMA_MATRIX_CON		(0x1400)
#define GAMMA_MATRIX_EN(_v)		(((_v) & 0x1) << 0)
#define GAMMA_MATRIX_EN_MASK		(0x1 << 0)

#define DQE_GAMMA_MATRIX_COEFF(_n)	(0x1404 + ((_n) * 0x4))
#define DQE_GAMMA_MATRIX_COEFF0		(0x1404)
#define DQE_GAMMA_MATRIX_COEFF1		(0x1408)
#define DQE_GAMMA_MATRIX_COEFF2		(0x140C)
#define DQE_GAMMA_MATRIX_COEFF3		(0x1410)
#define DQE_GAMMA_MATRIX_COEFF4		(0x1414)

#define GAMMA_MATRIX_COEFF_H(_v)	(((_v) & 0x3FFF) << 16)
#define GAMMA_MATRIX_COEFF_H_MASK	(0x3FFF << 16)
#define GAMMA_MATRIX_COEFF_L(_v)	(((_v) & 0x3FFF) << 0)
#define GAMMA_MATRIX_COEFF_L_MASK	(0x3FFF << 0)

#define DQE_GAMMA_MATRIX_OFFSET0	(0x1418)
#define GAMMA_MATRIX_OFFSET_1(_v)	(((_v) & 0xFFF) << 16)
#define GAMMA_MATRIX_OFFSET_1_MASK	(0xFFF << 16)
#define GAMMA_MATRIX_OFFSET_0(_v)	(((_v) & 0xFFF) << 0)
#define GAMMA_MATRIX_OFFSET_0_MASK	(0xFFF << 0)

#define DQE_GAMMA_MATRIX_OFFSET1	(0x141C)
#define GAMMA_MATRIX_OFFSET_2(_v)	(((_v) & 0xFFF) << 0)
#define GAMMA_MATRIX_OFFSET_2_MASK	(0xFFF << 0)

#define DQE_GAMMA_MATRIX_LUT(_n)	(0x1400 + ((_n) * 0x4))

#define DQE_GAMMA_MATRIX_REG_MAX	(8)
#define DQE_GAMMA_MATRIX_LUT_MAX	(17)

/*----------------------[DEGAMMA]----------------------------------------*/
#define DQE_DEGAMMA_CON			(0x1800)
#define DEGAMMA_EN(_v)			(((_v) & 0x1) << 0)
#define DEGAMMA_EN_MASK			(0x1 << 0)

#define DQE_DEGAMMA_POSX(_n)		(0x1804 + ((_n) * 0x4))
#define DQE_DEGAMMA_POSY(_n)		(0x1848 + ((_n) * 0x4))
#define DEGAMMA_LUT_H(_v)		(((_v) & 0x1FFF) << 16)
#define DEGAMMA_LUT_H_MASK		(0x1FFF << 16)
#define DEGAMMA_LUT_L(_v)		(((_v) & 0x1FFF) << 0)
#define DEGAMMA_LUT_L_MASK		(0x1FFF << 0)
#define DEGAMMA_LUT(_x, _v)		((_v) << (0 + (16 * ((_x) & 0x1))))
#define DEGAMMA_LUT_MASK(_x)		(0x1FFF << (0 + (16 * ((_x) & 0x1))))

#define DQE_DEGAMMA_LUT(_n)		(0x1800 + ((_n) * 0x4))

#define DQE_DEGAMMA_LUT_CNT		(33) // LUT_X + LUT_Y
#define DQE_DEGAMMA_REG_MAX		(1+DIV_ROUND_UP(DQE_DEGAMMA_LUT_CNT, 2)*2) // 35 CON+LUT_XY/2
#define DQE_DEGAMMA_LUT_MAX		(1+DQE_DEGAMMA_LUT_CNT*2)		// 67 CON+LUT

/*----------------------[LINEAR MATRIX]----------------------------------------*/
#define DQE_LINEAR_MATRIX_CON           (0x1C00)
#define LINEAR_MATRIX_EN(_v)		(((_v) & 0x1) << 0)
#define LINEAR_MATRIX_EN_MASK		(0x1 << 0)

#define DQE_LINEAR_MATRIX_COEFF(_n)	(0x1C04 + ((_n) * 0x4))
#define DQE_LINEAR_MATRIX_COEFF0	(0x1C04)
#define DQE_LINEAR_MATRIX_COEFF1	(0x1C08)
#define DQE_LINEAR_MATRIX_COEFF2	(0x1C0C)
#define DQE_LINEAR_MATRIX_COEFF3	(0x1C10)
#define DQE_LINEAR_MATRIX_COEFF4	(0x1C14)

#define LINEAR_MATRIX_COEFF_H(_v)	(((_v) & 0xFFFF) << 16)
#define LINEAR_MATRIX_COEFF_H_MASK	(0xFFFF << 16)
#define LINEAR_MATRIX_COEFF_L(_v)	(((_v) & 0xFFFF) << 0)
#define LINEAR_MATRIX_COEFF_L_MASK	(0xFFFF << 0)

#define DQE_LINEAR_MATRIX_OFFSET0	(0x1C18)
#define LINEAR_MATRIX_OFFSET_1(_v)	(((_v) & 0x3FFF) << 16)
#define LINEAR_MATRIX_OFFSET_1_MASK	(0x3FFF << 16)
#define LINEAR_MATRIX_OFFSET_0(_v)	(((_v) & 0x3FFF) << 0)
#define LINEAR_MATRIX_OFFSET_0_MASK	(0x3FFF << 0)

#define DQE_LINEAR_MATRIX_OFFSET1	(0x1C1C)
#define LINEAR_MATRIX_OFFSET_2(_v)	(((_v) & 0x3FFF) << 0)
#define LINEAR_MATRIX_OFFSET_2_MASK	(0x3FFF << 0)

#define DQE_LINEAR_MATRIX_LUT(_n)	(0x1C00 + ((_n) * 0x4))

#define DQE_LINEAR_MATRIX_REG_MAX	(8)
#define DQE_LINEAR_MATRIX_LUT_MAX	(17)

/*----------------------[CGC]------------------------------------------------*/
#define DQE_CGC_CON			(0x2000)
#define CGC_COEF_DMA_REQ(_v)		((_v) << 4)
#define CGC_COEF_DMA_REQ_MASK		(0x1 << 4)
#define CGC_PIXMAP_EN(_v)		((_v) << 2)
#define CGC_PIXMAP_EN_MASK		(0x1 << 2)
#define CGC_EN(_v)			(((_v) & 0x1) << 0)
#define CGC_EN_MASK			(0x1 << 0)

#define DQE_CGC_MC_R0(_n)		(0x2004 + ((_n) * 0x4))
#define DQE_CGC_MC1_R0			(0x2004)
#define DQE_CGC_MC3_R0			(0x200C)
#define DQE_CGC_MC4_R0			(0x2010)
#define DQE_CGC_MC5_R0			(0x2014)

#define DQE_CGC_MC_R1(_n)		(0x2018 + ((_n) * 0x4))
#define DQE_CGC_MC1_R1			(0x2018)
#define DQE_CGC_MC3_R1			(0x2020)
#define DQE_CGC_MC4_R1			(0x2024)
#define DQE_CGC_MC5_R1			(0x2028)

#define DQE_CGC_MC_R2(_n)		(0x202C + ((_n) * 0x4))
#define DQE_CGC_MC1_R2			(0x202C)
#define CGC_MC_GAIN_R(_v)		(((_v) & 0xFFF) << 16)
#define CGC_MC_BC_VAL_R(_v)		(((_v) & 0x3) << 12)
#define CGC_MC_BC_SAT_R(_v)		(((_v) & 0x3) << 8)
#define CGC_MC_BC_HUE_R(_v)		(((_v) & 0x3) << 4)
#define CGC_MC_INVERSE_R(_v)		(((_v) & 0x1) << 1)
#define CGC_MC_ON_R(_v)			(((_v) & 0x1) << 0)
#define DQE_CGC_MC2_R2			(0x2030)
#define CGC_MC_VAL_GAIN_R(_v)		(((_v) & 0x7FF) << 16)
#define CGC_MC_HUE_GAIN_R(_v)		(((_v) & 0x3FF) << 0)
#define DQE_CGC_MC3_R2			(0x2034)
#define CGC_MC_S2_R(_v)			(((_v) & 0x3FF) << 16)
#define CGC_MC_S1_R(_v)			(((_v) & 0x3FF) << 0)
#define DQE_CGC_MC4_R2			(0x2038)
#define CGC_MC_H2_R(_v)			(((_v) & 0x1FFF) << 16)
#define CGC_MC_H1_R(_v)			(((_v) & 0x1FFF) << 0)
#define DQE_CGC_MC5_R2			(0x203C)
#define CGC_MC_V2_R(_v)			(((_v) & 0x3FF) << 16)
#define CGC_MC_V1_R(_v)			(((_v) & 0x3FF) << 0)

/*----------------------[REGAMMA]----------------------------------------*/
#define DQE_REGAMMA_CON			(0x2400)
#define REGAMMA_EN(_v)			(((_v) & 0x1) << 0)
#define REGAMMA_EN_MASK			(0x1 << 0)

#define DQE_REGAMMA_R_POSX(_n)		(0x2404 + ((_n) * 0x4))
#define DQE_REGAMMA_R_POSY(_n)		(0x2448 + ((_n) * 0x4))
#define DQE_REGAMMA_G_POSX(_n)		(0x248C + ((_n) * 0x4))
#define DQE_REGAMMA_G_POSY(_n)		(0x24D0 + ((_n) * 0x4))
#define DQE_REGAMMA_B_POSX(_n)		(0x2514 + ((_n) * 0x4))
#define DQE_REGAMMA_B_POSY(_n)		(0x2558 + ((_n) * 0x4))

#define REGAMMA_LUT_H(_v)		(((_v) & 0x1FFF) << 16)
#define REGAMMA_LUT_H_MASK		(0x1FFF << 16)
#define REGAMMA_LUT_L(_v)		(((_v) & 0x1FFF) << 0)
#define REGAMMA_LUT_L_MASK		(0x1FFF << 0)
#define REGAMMA_LUT(_x, _v)		((_v) << (0 + (16 * ((_x) & 0x1))))
#define REGAMMA_LUT_MASK(_x)		(0x1FFF << (0 + (16 * ((_x) & 0x1))))

#define DQE_REGAMMA_LUT(_n)		(0x2400 + ((_n) * 0x4))

#define DQE_REGAMMA_LUT_CNT		(33) // LUT_RGB_X/Y
#define DQE_REGAMMA_REG_MAX		(1+DIV_ROUND_UP(DQE_REGAMMA_LUT_CNT, 2)*2*3) // 103 CON + LUT_RGB_X/Y
#define DQE_REGAMMA_LUT_MAX		(1+DQE_REGAMMA_LUT_CNT*2*3)	// 199 CON + LUT_RGB_X/Y

/*----------------------[CGC_DITHER]-----------------------------------------*/
#define DQE_CGC_DITHER_CON		(0x2800)
#define CGC_DITHER_FRAME_OFFSET(_v)	(((_v) & 0xF) << 12)
#define CGC_DITHER_BIT(_v)		(((_v) & 0x1) << 8)
#define CGC_DITHER_BIT_MASK		(0x1 << 8)
#define CGC_DITHER_MASK_SEL_B		(0x1 << 7)
#define CGC_DITHER_MASK_SEL_G		(0x1 << 6)
#define CGC_DITHER_MASK_SEL_R		(0x1 << 5)
#define CGC_DITHER_MASK_SEL(_v, _n)	(((_v) & 0x1) << (5 + (_n)))
#define CGC_DITHER_MASK_SPIN(_v)	(((_v) & 0x1) << 3)
#define CGC_DITHER_MODE(_v)		(((_v) & 0x3) << 1)
#define CGC_DITHER_EN(_v)		(((_v) & 0x1) << 0)
#define CGC_DITHER_EN_MASK		(0x1 << 0)

#define DQE_CGC_DITHER_SEED		(0x2804)
#define CGC_DITHER_SHUFFLE_MASK_F(_v)	(((_v) & 0xFFFF) << 16)
#define CGC_DITHER_SEED_F(_v)		(((_v) & 0xFFFF) << 0)

#define DQE_CGC_DITHER_SEED_CON		(0x2808)
#define FRAME_RESET_HW_DISABLE(_v)	(((_v) & 0x1) << 1)
#define FRAME_RESET_SW(_v)		(((_v) & 0x1) << 0)

#define DQE_CGC_DITHER_LUT_MAX	(8)

/*----------------------[RCD]-----------------------------------------*/
#define DQE_RCD				(0x3000)
#define RCD_EN(_v)			((_v) << 0)
#define RCD_EN_MASK			(0x1 << 0)

#define DQE_RCD_BG_ALPHA_OUTTER		(0x3004)
#define BG_ALPHA_OUTTER_B(_v)		(((_v) & 0x3FF) << 20)
#define BG_ALPHA_OUTTER_G(_v)		(((_v) & 0x3FF) << 10)
#define BG_ALPHA_OUTTER_R(_v)		(((_v) & 0x3FF) << 0)

#define DQE_RCD_BG_ALPHA_ALIASING	(0x3008)
#define BG_ALPHA_ALIASING_B(_v)		(((_v) & 0x3FF) << 20)
#define BG_ALPHA_ALIASING_G(_v)		(((_v) & 0x3FF) << 10)
#define BG_ALPHA_ALIASING_R(_v)		(((_v) & 0x3FF) << 0)

#define DQE_RCD_LUT(_n)			(0x3000 + ((_n) * 0x4))

#define DQE_RCD_REG_MAX			(3)
#define DQE_RCD_LUT_MAX			(7)

/*----------------------[CGC_LUT]-----------------------------------------*/
#define DQE_CGC_LUT_R(_n)		(0x4000 + ((_n) * 0x4))
#define DQE_CGC_LUT_G(_n)		(0x8000 + ((_n) * 0x4))
#define DQE_CGC_LUT_B(_n)		(0xC000 + ((_n) * 0x4))

#define CGC_LUT_H_SHIFT			(16)
#define CGC_LUT_H(_v)			((_v) << CGC_LUT_H_SHIFT)
#define CGC_LUT_H_MASK			(0x1FFF << CGC_LUT_H_SHIFT)
#define CGC_LUT_L_SHIFT			(0)
#define CGC_LUT_L(_v)			((_v) << CGC_LUT_L_SHIFT)
#define CGC_LUT_L_MASK			(0x1FFF << CGC_LUT_L_SHIFT)
#define CGC_LUT_LH(_x, _v)		((_v) << (0 + (16 * ((_x) & 0x1))))
#define CGC_LUT_LH_MASK(_x)		(0x1FFF << (0 + (16 * ((_x) & 0x1))))

#define DQE_CGC_REG_MAX			(2457)
#define DQE_CGC_LUT_MAX			(4914)
#define DQE_CGC_CON_REG_MAX		(16)
#define DQE_CGC_CON_LUT_MAX		(37)

#endif
