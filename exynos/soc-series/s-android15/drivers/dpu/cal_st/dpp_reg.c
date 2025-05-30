// SPDX-License-Identifier: GPL-2.0-only
/*
 * cal_st/dpp_regs.c
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Jaehoe Yang <jaehoe.yang@samsung.com>
 * Jiun Yu <jiun.yu@samsung.com>
 *
 * Register access functions for Samsung EXYNOS Display Pre-Processor driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/iopoll.h>

#include <exynos_dpp_coef.h>
#include <exynos_drm_format.h>
#include <exynos_drm_dpp.h>
#include <cal_config.h>
#include <dpp_cal.h>
#include <regs-dpp.h>

//#define DMA_BIST

#define DPP_SC_RATIO_MAX	((1 << 20) * 8 / 8)
#define DPP_SC_RATIO_7_8	((1 << 20) * 8 / 7)
#define DPP_SC_RATIO_6_8	((1 << 20) * 8 / 6)
#define DPP_SC_RATIO_5_8	((1 << 20) * 8 / 5)
#define DPP_SC_RATIO_4_8	((1 << 20) * 8 / 4)
#define DPP_SC_RATIO_3_8	((1 << 20) * 8 / 3)

static struct cal_regs_desc regs_dpp[REGS_DPP_TYPE_MAX][REGS_DPP_ID_MAX];

#define dpp_regs_desc(id)			(&regs_dpp[REGS_DPP][id])
#define dpp_read(id, offset)			\
	cal_read(dpp_regs_desc(id), offset)
#define dpp_write(id, offset, val)		\
	cal_write(dpp_regs_desc(id), offset, val)
#define dpp_read_mask(id, offset, mask)	\
	cal_read_mask(dpp_regs_desc(id), offset, mask)
#define dpp_write_mask(id, offset, val, mask)	\
	cal_write_mask(dpp_regs_desc(id), offset, val, mask)
#define dpp_write_relaxed(id, offset, val)		\
	cal_write_relaxed(dpp_regs_desc(id), offset, val)

#define dma_regs_desc(id)			(&regs_dpp[REGS_DMA][id])
#define dma_read(id, offset)			\
	cal_read(dma_regs_desc(id), offset)
#define dma_write(id, offset, val)		\
	cal_write(dma_regs_desc(id), offset, val)
#define dma_read_mask(id, offset, mask)	\
	cal_read_mask(dma_regs_desc(id), offset, mask)
#define dma_write_mask(id, offset, val, mask)	\
	cal_write_mask(dma_regs_desc(id), offset, val, mask)

#define votf_regs_desc(id)			(&regs_dpp[REGS_VOTF][id])
#define votf_read(id, offset)			\
	cal_read(votf_regs_desc(id), offset)
#define votf_write(id, offset, val)		\
	cal_write(votf_regs_desc(id), offset, val)
#define votf_read_mask(id, offset, mask)	\
	cal_read_mask(votf_regs_desc(id), offset, mask)
#define votf_write_mask(id, offset, val, mask)	\
	cal_write_mask(votf_regs_desc(id), offset, val, mask)

/* SCL_COEF */
#define coef_regs_desc(id)			(&regs_dpp[REGS_SCL_COEF][id])
#define coef_read(id, offset)			\
	cal_read(coef_regs_desc(id), offset)
#define coef_write(id, offset, val)		\
	cal_write(coef_regs_desc(id), offset, val)

/* HDR_COMMON */
#define hdr_comm_regs_desc(id)			(&regs_dpp[REGS_HDR_COMM][id])
#define hdr_comm_read(id, offset)			\
	cal_read(hdr_comm_regs_desc(id), offset)
#define hdr_comm_write(id, offset, val)		\
	cal_write(hdr_comm_regs_desc(id), offset, val)
#define hdr_comm_read_mask(id, offset, mask)	\
	cal_read_mask(hdr_comm_regs_desc(id), offset, mask)
#define hdr_comm_write_mask(id, offset, val, mask)	\
	cal_write_mask(hdr_comm_regs_desc(id), offset, val, mask)
void dpp_regs_desc_init(void __iomem *regs, const char *name,
		enum dpp_regs_type type, unsigned int id)
{
	cal_regs_desc_check(type, id, REGS_DPP_TYPE_MAX, REGS_DPP_ID_MAX);
	cal_regs_desc_set(regs_dpp, regs, name, type, id);
}

/****************** IDMA CAL functions ******************/
static void idma_reg_set_irq_mask_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, RDMA_IRQ, val, IDMA_ALL_IRQ_MASK);
}

static void idma_reg_set_irq_enable(u32 id)
{
	dma_write_mask(id, RDMA_IRQ, ~0, IDMA_IRQ_ENABLE);
}

static void idma_reg_set_irq_disable(u32 id)
{
        dma_write_mask(id, RDMA_IRQ, 0, IDMA_IRQ_ENABLE);
}

static void idma_reg_set_in_qos_lut(u32 id, u32 lut_id, u32 qos_t)
{
	u32 reg_id;

	if (lut_id == 0)
		reg_id = RDMA_QOS_LUT_LOW;
	else
		reg_id = RDMA_QOS_LUT_HIGH;
	dma_write(id, reg_id, qos_t);
}

static void idma_reg_set_sram_clk_gate_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, RDMA_DYNAMIC_GATING_EN, val, IDMA_SRAM_CG_EN);
}

static void idma_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, RDMA_DYNAMIC_GATING_EN, val, IDMA_DG_EN_ALL);
}

static void idma_reg_clear_irq(u32 id, u32 irq)
{
	dma_write_mask(id, RDMA_IRQ, ~0, irq);
}

static void idma_reg_set_sw_reset(u32 id)
{
	dma_write_mask(id, RDMA_ENABLE, ~0, IDMA_SRESET);
}

static void idma_reg_set_assigned_mo(u32 id, u32 mo)
{
	dma_write_mask(id, RDMA_ENABLE, IDMA_ASSIGNED_MO(mo),
			IDMA_ASSIGNED_MO_MASK);
}

static int idma_reg_wait_sw_reset_status(u32 id)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout_atomic(dma_regs_desc(id)->regs + RDMA_ENABLE,
			val, !(val & IDMA_SRESET), 10, 2000); /* timeout 2ms */
	if (ret) {
		cal_log_err(id, "[idma] timeout sw-reset\n");
		return ret;
	}

	return 0;
}

static void idma_reg_set_coordinates(u32 id, struct decon_frame *src)
{
	dma_write(id, RDMA_SRC_OFFSET,
			IDMA_SRC_OFFSET_Y(src->y) | IDMA_SRC_OFFSET_X(src->x));
	dma_write(id, RDMA_SRC_SIZE,
			IDMA_SRC_HEIGHT(src->f_h) | IDMA_SRC_WIDTH(src->f_w));
	dma_write(id, RDMA_IMG_SIZE,
			IDMA_IMG_HEIGHT(src->h) | IDMA_IMG_WIDTH(src->w));
}

static void idma_reg_set_frame_alpha(u32 id, u32 alpha)
{
	dma_write_mask(id, RDMA_IN_CTRL_0, IDMA_ALPHA(alpha),
			IDMA_ALPHA_MASK);
}

static void idma_reg_set_ic_max(u32 id, u32 ic_max)
{
	dma_write_mask(id, RDMA_IN_CTRL_0, IDMA_IC_MAX(ic_max),
			IDMA_IC_MAX_MASK);
}

static void idma_reg_set_rotation(u32 id, u32 rot)
{
	dma_write_mask(id, RDMA_IN_CTRL_0, IDMA_ROT(rot), IDMA_ROT_MASK);
}

static void idma_reg_set_block_mode(u32 id, bool en, int x, int y, u32 w, u32 h)
{
	if (!en) {
		dma_write_mask(id, RDMA_IN_CTRL_0, 0, IDMA_BLOCK_EN);
		return;
	}

	dma_write(id, RDMA_BLOCK_OFFSET,
			IDMA_BLK_OFFSET_Y(y) | IDMA_BLK_OFFSET_X(x));
	dma_write(id, RDMA_BLOCK_SIZE, IDMA_BLK_HEIGHT(h) | IDMA_BLK_WIDTH(w));
	dma_write_mask(id, RDMA_IN_CTRL_0, ~0, IDMA_BLOCK_EN);

	cal_log_debug(id, "block x(%d) y(%d) w(%d) h(%d)\n", x, y, w, h);
}

static void idma_reg_set_format(u32 id, u32 fmt)
{
	dma_write_mask(id, RDMA_IN_CTRL_0, IDMA_IMG_FORMAT(fmt),
			IDMA_IMG_FORMAT_MASK);
}

#if defined(DMA_BIST)
static void idma_reg_set_test_pattern(u32 id, u32 pat_id, const u32 *pat_dat)
{
	u32 map_tlb[6] = {0, 0, 2, 2, 4, 4};
	u32 new_id;

	/* 0=AXI, 3=PAT */
	dma_write_mask(id, RDMA_IN_REQ_DEST, ~0, IDMA_REQ_DEST_SEL_MASK);

	new_id = map_tlb[id];
	if (pat_id == 0) {
		dma_write(new_id, GLB_TEST_PATTERN0_0, pat_dat[0]);
		dma_write(new_id, GLB_TEST_PATTERN0_1, pat_dat[1]);
		dma_write(new_id, GLB_TEST_PATTERN0_2, pat_dat[2]);
		dma_write(new_id, GLB_TEST_PATTERN0_3, pat_dat[3]);
	} else {
		dma_write(new_id, GLB_TEST_PATTERN1_0, pat_dat[4]);
		dma_write(new_id, GLB_TEST_PATTERN1_1, pat_dat[5]);
		dma_write(new_id, GLB_TEST_PATTERN1_2, pat_dat[6]);
		dma_write(new_id, GLB_TEST_PATTERN1_3, pat_dat[7]);
	}
}
#endif

static void idma_reg_set_pslave_err(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = IDMA_PSLVERR_CTRL;
	dma_write_mask(id, RDMA_PSLV_ERR_CTRL, val, mask);
}

/* TBD: SAJC swizzle mode needs to be checked */
static void idma_reg_set_comp(u32 id, const struct dpp_params_info *p)
{
	u32 val = 0;

	switch (p->comp_type) {
	case COMP_TYPE_SBWC:
		val = IDMA_SBWC_EN;
		dma_write(id, RDMA_SBWC_PARAM, IDMA_CHM_BLK_BYTENUM(p->blk_size) |
				IDMA_LUM_BLK_BYTENUM(p->blk_size));
		break;
	case COMP_TYPE_SAJC:
		val = IDMA_SAJC_EN;
		dma_write(id, RDMA_SAJC_PARAM, IDMA_INDEPENDENT_BLK(p->blk_size) |
				IDMA_SAJC_SWIZZLE_MODE(p->sajc_sw_mode));
		break;
	default:
		break;
	}

	dma_write_mask(id, RDMA_IN_CTRL_0, val, IDMA_SBWC_EN | IDMA_SAJC_EN);
	/* recovery is not needed due to hang-free */
	dma_write_mask(id, RDMA_RECOVERY_CTRL, 0, IDMA_RECOVERY_EN);
}

static void idma_reg_set_deadlock(u32 id, u32 en, u32 dl_num)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, RDMA_DEADLOCK_CTRL, val, IDMA_DEADLOCK_NUM_EN);
	dma_write_mask(id, RDMA_DEADLOCK_CTRL, IDMA_DEADLOCK_NUM(dl_num),
				IDMA_DEADLOCK_NUM_MASK);
}

static void idma_reg_print_irqs_msg(u32 id, u32 irqs)
{
	u32 cfg_err;

	if (irqs & IDMA_AXI_ADDR_ERR_IRQ)
		cal_log_err(id, "IDMA axi addr err irq occur\n");

	if (irqs & IDMA_LB_CONFLICT_IRQ)
		cal_log_err(id, "IDMA LineBuffer conflict irq occur\n");

	if (irqs & IDMA_MO_CONFLICT_IRQ)
		cal_log_err(id, "IDMA mo conflict irq occur\n");

	if (irqs & IDMA_FBC_ERR_IRQ)
		cal_log_err(id, "IDMA FBC error irq occur\n");

	if (irqs & IDMA_INST_OFF_DONE)
		cal_log_err(id, "IDMA inst off done irq occur\n");

	if (irqs & IDMA_READ_SLAVE_ERR_IRQ)
		cal_log_err(id, "IDMA read slave error irq occur\n");

	if (irqs & IDMA_DEADLOCK_IRQ)
		cal_log_err(id, "IDMA deadlock irq occur\n");

	if (irqs & IDMA_CONFIG_ERR_IRQ) {
		cfg_err = dma_read(id, RDMA_CONFIG_ERR_STATUS);
		cal_log_err(id, "IDMA cfg err irq occur(0x%x)\n", cfg_err);
	}
}

/****************** ODMA CAL functions ******************/
static void odma_reg_set_irq_mask_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, WDMA_IRQ, val, ODMA_ALL_IRQ_MASK);
}

static void odma_reg_set_irq_enable(u32 id)
{
	dma_write_mask(id, WDMA_IRQ, ~0, ODMA_IRQ_ENABLE);
}

static void odma_reg_set_irq_disable(u32 id)
{
	dma_write_mask(id, WDMA_IRQ, 0, ODMA_IRQ_ENABLE);
}

static void odma_reg_set_in_qos_lut(u32 id, u32 lut_id, u32 qos_t)
{
	u32 reg_id;

	if (lut_id == 0)
		reg_id = WDMA_QOS_LUT_LOW;
	else
		reg_id = WDMA_QOS_LUT_HIGH;
	dma_write(id, reg_id, qos_t);
}

static void odma_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, WDMA_DYNAMIC_GATING_EN, val, ODMA_DG_EN_ALL);
}

static void odma_reg_set_frame_alpha(u32 id, u32 alpha)
{
	dma_write_mask(id, WDMA_OUT_CTRL_0, ODMA_ALPHA(alpha),
			ODMA_ALPHA_MASK);
}

static void odma_reg_clear_irq(u32 id, u32 irq)
{
	dma_write_mask(id, WDMA_IRQ, ~0, irq);
}

static void odma_reg_set_sw_reset(u32 id)
{
	dma_write_mask(id, WDMA_ENABLE, ~0, ODMA_SRESET);
}

static int odma_reg_wait_sw_reset_status(u32 id)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout_atomic(dma_regs_desc(id)->regs + WDMA_ENABLE,
			val, !(val & ODMA_SRESET), 10, 2000); /* timeout 2ms */
	if (ret) {
		cal_log_err(id, "[odma%d] timeout sw-reset\n", id);
		return ret;
	}

	return 0;
}

static void odma_reg_set_coordinates(u32 id, struct decon_frame *dst)
{
	dma_write(id, WDMA_DST_OFFSET,
			ODMA_DST_OFFSET_Y(dst->y) | ODMA_DST_OFFSET_X(dst->x));
	dma_write(id, WDMA_DST_SIZE,
			ODMA_DST_HEIGHT(dst->f_h) | ODMA_DST_WIDTH(dst->f_w));
	dma_write(id, WDMA_IMG_SIZE,
			ODMA_IMG_HEIGHT(dst->h) | ODMA_IMG_WIDTH(dst->w));
}

static void odma_reg_set_ic_max(u32 id, u32 ic_max)
{
	dma_write_mask(id, WDMA_OUT_CTRL_0, ODMA_IC_MAX(ic_max),
			ODMA_IC_MAX_MASK);
}

static void odma_reg_set_format(u32 id, u32 fmt)
{
	dma_write_mask(id, WDMA_OUT_CTRL_0, ODMA_IMG_FORMAT(fmt),
			ODMA_IMG_FORMAT_MASK);
}

static void odma_reg_set_pslave_err(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = ODMA_PSLVERR_CTRL;
	dma_write_mask(id, WDMA_PSLV_ERR_CTRL, val, mask);
}

static void odma_reg_print_irqs_msg(u32 id, u32 irqs)
{
	u32 cfg_err;

	if (irqs & ODMA_WRITE_SLAVE_ERR_IRQ)
		cal_log_err(id, "ODMA write slave error irq occur\n");

	if (irqs & ODMA_DEADLOCK_IRQ)
		cal_log_err(id, "ODMA deadlock error irq occur\n");

	if (irqs & ODMA_CONFIG_ERR_IRQ) {
		cfg_err = dma_read(id, WDMA_CONFIG_ERR_STATUS);
		cal_log_err(id, "ODMA cfg err irq occur(0x%x)\n", cfg_err);
	}
}

/****************** DPP CAL functions ******************/
static void dpp_reg_set_irq_mask_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	/* DPP_COM_IRQ_MASK 0x14 is removed for WB DPP channel */
	if (id == REGS_DPP_WB_ID) {
		cal_log_err(id, "no dpp irq, only dma irq");
		return;
	}
	dpp_write_mask(id, DPP_COM_IRQ_MASK, val, DPP_ALL_IRQ_MASK);
}

void dpp_reg_set_irq_enable(u32 id)
{
	/* DPP_COM_IRQ_CON 0x10 is removed for WB DPP channel */
	if (id == REGS_DPP_WB_ID) {
		cal_log_err(id, "no dpp irq, only dma irq");
		return;
	}
	dpp_write_mask(id, DPP_COM_IRQ_CON, ~0, DPP_IRQ_EN);
}

void dpp_reg_set_irq_disable(u32 id)
{
	/* DPP_COM_IRQ_CON 0x10 is removed for WB DPP channel */
	if (id == REGS_DPP_WB_ID) {
		cal_log_err(id, "no dpp irq, only dma irq");
		return;
	}
	dpp_write_mask(id, DPP_COM_IRQ_CON, 0, DPP_IRQ_EN);
}

static void dpp_reg_set_linecnt(u32 id, u32 en)
{
	if ((id != REGS_DPP_WB_ID) && (id > REGS_DPP7_ID))
		return;

	if (en)
		dpp_write_mask(id, DPP_COM_LC_CON,
				DPP_LC_MODE(0) | DPP_LC_EN(1),
				DPP_LC_MODE_MASK | DPP_LC_EN_MASK);
	else
		dpp_write_mask(id, DPP_COM_LC_CON, DPP_LC_EN(0),
				DPP_LC_EN_MASK);
}

static void dpp_reg_clear_irq(u32 id, u32 irq)
{
	/* DPP_COM_IRQ_STATUS 0x18 is removed for WB DPP channel */
	if (id == REGS_DPP_WB_ID) {
		cal_log_err(id, "no dpp irq, only dma irq");
		return;
	}
	dpp_write_mask(id, DPP_COM_IRQ_STATUS, ~0, irq);
}

static void dpp_reg_set_sw_reset(u32 id)
{
	dpp_write_mask(id, DPP_COM_SWRST_CON, ~0, DPP_SRESET);
}

static int dpp_reg_wait_sw_reset_status(u32 id)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout_atomic(
			dpp_regs_desc(id)->regs + DPP_COM_SWRST_CON, val,
			!(val & DPP_SRESET), 10, 2000); /* timeout 2ms */
	if (ret) {
		cal_log_err(id, "[dpp%d] timeout sw-reset\n", id);
		return ret;
	}

	return 0;
}

static void dpp_reg_set_pslave_err(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = DPP_PSLVERR_EN;
	dma_write_mask(id, DPP_COM_PSLVERR_CON, val, mask);
}

static void dpp_reg_set_uv_offset(u32 id, u32 off_x, u32 off_y)
{
	u32 val, mask;

	val = DPP_UV_OFFSET_Y(off_y) | DPP_UV_OFFSET_X(off_x);
	mask = DPP_UV_OFFSET_Y_MASK | DPP_UV_OFFSET_X_MASK;
	dpp_write_mask(id, DPP_COM_SUB_CON, val, mask);
}

static void
dpp_reg_set_csc_coef(u32 id, u32 std, u32 range, const unsigned long attr)
{
	u32 val, mask;
	u32 csc_id = DPP_CSC_IDX_BT601_625;
	u32 c00, c01, c02;
	u32 c10, c11, c12;
	u32 c20, c21, c22;
	const u16 (*csc_arr)[3][3];

	switch (std) {
	case EXYNOS_STANDARD_BT601_625:
		csc_id = DPP_CSC_IDX_BT601_625;
		break;
	case EXYNOS_STANDARD_BT601_625_UNADJUSTED:
		csc_id = DPP_CSC_IDX_BT601_625_UNADJUSTED;
		break;
	case EXYNOS_STANDARD_BT601_525:
		csc_id = DPP_CSC_IDX_BT601_525;
		break;
	case EXYNOS_STANDARD_BT601_525_UNADJUSTED:
		csc_id = DPP_CSC_IDX_BT601_525_UNADJUSTED;
		break;
	case EXYNOS_STANDARD_BT2020_CONSTANT_LUMINANCE:
		csc_id = DPP_CSC_IDX_BT2020_CONSTANT_LUMINANCE;
		break;
	case EXYNOS_STANDARD_BT470M:
		csc_id = DPP_CSC_IDX_BT470M;
		break;
	case EXYNOS_STANDARD_FILM:
		csc_id = DPP_CSC_IDX_FILM;
		break;
	case EXYNOS_STANDARD_ADOBE_RGB:
		csc_id = DPP_CSC_IDX_ADOBE_RGB;
		break;
	case EXYNOS_STANDARD_BT709:
		csc_id = DPP_CSC_IDX_BT709;
		break;
	case EXYNOS_STANDARD_BT2020:
		csc_id = DPP_CSC_IDX_BT2020;
		break;
	case EXYNOS_STANDARD_DCI_P3:
		csc_id = DPP_CSC_IDX_DCI_P3;
		break;
	default:
		range = EXYNOS_RANGE_LIMITED;
		cal_log_err(id, "invalid CSC type\n");
		cal_log_err(id, "BT601 with limited range is set as default\n");
	}

	if (test_bit(DPP_ATTR_ODMA, &attr))
		csc_arr = csc_r2y_3x3_t;
	else
		csc_arr = csc_y2r_3x3_t;

	/*
	 * The matrices are provided only for full or limited range
	 * and limited range is used as default.
	 */
	if (range == EXYNOS_RANGE_FULL)
		csc_id += 1;

	c00 = csc_arr[csc_id][0][0];
	c01 = csc_arr[csc_id][0][1];
	c02 = csc_arr[csc_id][0][2];

	c10 = csc_arr[csc_id][1][0];
	c11 = csc_arr[csc_id][1][1];
	c12 = csc_arr[csc_id][1][2];

	c20 = csc_arr[csc_id][2][0];
	c21 = csc_arr[csc_id][2][1];
	c22 = csc_arr[csc_id][2][2];

	mask = (DPP_CSC_COEF_H_MASK | DPP_CSC_COEF_L_MASK);
	val = (DPP_CSC_COEF_H(c01) | DPP_CSC_COEF_L(c00));
	dpp_write_mask(id, DPP_COM_CSC_COEF0, val, mask);

	val = (DPP_CSC_COEF_H(c10) | DPP_CSC_COEF_L(c02));
	dpp_write_mask(id, DPP_COM_CSC_COEF1, val, mask);

	val = (DPP_CSC_COEF_H(c12) | DPP_CSC_COEF_L(c11));
	dpp_write_mask(id, DPP_COM_CSC_COEF2, val, mask);

	val = (DPP_CSC_COEF_H(c21) | DPP_CSC_COEF_L(c20));
	dpp_write_mask(id, DPP_COM_CSC_COEF3, val, mask);

	mask = DPP_CSC_COEF_L_MASK;
	val = DPP_CSC_COEF_L(c22);
	dpp_write_mask(id, DPP_COM_CSC_COEF4, val, mask);

	cal_log_debug(id, "---[%s CSC Type: std=%d, rng=%d]---\n",
		test_bit(DPP_ATTR_ODMA, &attr) ? "R2Y" : "Y2R", std, range);
	cal_log_debug(id, "0x%4x  0x%4x  0x%4x\n", c00, c01, c02);
	cal_log_debug(id, "0x%4x  0x%4x  0x%4x\n", c10, c11, c12);
	cal_log_debug(id, "0x%4x  0x%4x  0x%4x\n", c20, c21, c22);
}

static void
dpp_reg_set_csc_params(u32 id, u32 std, u32 range, const unsigned long attr)
{
	u32 type, hw_range, mode, val, mask;

	mode = DPP_CSC_MODE_HARDWIRED;

	switch (std) {
	case EXYNOS_STANDARD_UNSPECIFIED:
		type = DPP_CSC_TYPE_BT601;
		cal_log_debug(id, "unspecified CSC type! -> BT_601\n");
		break;
	case EXYNOS_STANDARD_BT709:
		type = DPP_CSC_TYPE_BT709;
		break;
	case EXYNOS_STANDARD_BT601_625:
	case EXYNOS_STANDARD_BT601_625_UNADJUSTED:
	case EXYNOS_STANDARD_BT601_525:
	case EXYNOS_STANDARD_BT601_525_UNADJUSTED:
		type = DPP_CSC_TYPE_BT601;
		break;
	case EXYNOS_STANDARD_BT2020:
		type = DPP_CSC_TYPE_BT2020;
		break;
	case EXYNOS_STANDARD_DCI_P3:
		type = DPP_CSC_TYPE_DCI_P3;
		break;
	default:
		type = DPP_CSC_TYPE_BT601;
		mode = DPP_CSC_MODE_CUSTOMIZED;
		break;
	}

	if (test_bit(DPP_ATTR_ODMA, &attr))
		mode = DPP_CSC_MODE_CUSTOMIZED;

	/*
	 * DPP hardware supports full or limited range.
	 * Limited range is used as default
	 */
	if (range == EXYNOS_RANGE_FULL)
		hw_range = DPP_CSC_RANGE_FULL;
	else if (range == EXYNOS_RANGE_LIMITED)
		hw_range = DPP_CSC_RANGE_LIMITED;
	else
		hw_range = DPP_CSC_RANGE_LIMITED;

	val = type | hw_range | mode;
	mask = (DPP_CSC_TYPE_MASK | DPP_CSC_RANGE_MASK | DPP_CSC_MODE_MASK);
	dpp_write_mask(id, DPP_COM_CSC_CON, val, mask);

	if (mode == DPP_CSC_MODE_CUSTOMIZED)
		dpp_reg_set_csc_coef(id, std, range, attr);
}

static void dpp_reg_set_h_coef(u32 id, u32 h_ratio)
{

	int i, j, k, sc_ratio;

	if (h_ratio <= DPP_SC_RATIO_MAX)
		sc_ratio = 0;
	else if (h_ratio <= DPP_SC_RATIO_7_8)
		sc_ratio = 1;
	else if (h_ratio <= DPP_SC_RATIO_6_8)
		sc_ratio = 2;
	else if (h_ratio <= DPP_SC_RATIO_5_8)
		sc_ratio = 3;
	else if (h_ratio <= DPP_SC_RATIO_4_8)
		sc_ratio = 4;
	else if (h_ratio <= DPP_SC_RATIO_3_8)
		sc_ratio = 5;
	else
		sc_ratio = 6;


	/* TBD:  REVISIT */
	for (i = 0; i < 9; i++)
		for (j = 0; j < 8; j++)
			for (k = 0; k < 2; k++)
				dpp_write_relaxed(id, DPP_H_COEF(i, j, k),
						h_coef_8t[sc_ratio][i][j]);
}

static void dpp_reg_set_v_coef(u32 id, u32 v_ratio)
{
	int i, j, k, sc_ratio;

	if (v_ratio <= DPP_SC_RATIO_MAX)
		sc_ratio = 0;
	else if (v_ratio <= DPP_SC_RATIO_7_8)
		sc_ratio = 1;
	else if (v_ratio <= DPP_SC_RATIO_6_8)
		sc_ratio = 2;
	else if (v_ratio <= DPP_SC_RATIO_5_8)
		sc_ratio = 3;
	else if (v_ratio <= DPP_SC_RATIO_4_8)
		sc_ratio = 4;
	else if (v_ratio <= DPP_SC_RATIO_3_8)
		sc_ratio = 5;
	else
		sc_ratio = 6;

	/* REVISIT it for Papaya */
	for (i = 0; i < 9; i++)
		for (j = 0; j < 4; j++)
			for (k = 0; k < 2; k++)
				dpp_write_relaxed(id, DPP_V_COEF(i, j, k),
						v_coef_4t[sc_ratio][i][j]);
}

static void dpp_reg_set_scale_ratio(u32 id, struct dpp_params_info *p)
{
	u32 prev_h_ratio, prev_v_ratio;

	prev_h_ratio = dpp_read_mask(id, DPP_COM_SCL_MAIN_H_RATIO, DPP_SCL_H_RATIO_MASK);
	prev_v_ratio = dpp_read_mask(id, DPP_COM_SCL_MAIN_V_RATIO, DPP_SCL_V_RATIO_MASK);

	if (prev_h_ratio != p->h_ratio) {
		dpp_write(id, DPP_COM_SCL_MAIN_H_RATIO, DPP_SCL_H_RATIO(p->h_ratio));
		dpp_reg_set_h_coef(id, p->h_ratio);
	}

	if (prev_v_ratio != p->v_ratio) {
		dpp_write(id, DPP_COM_SCL_MAIN_V_RATIO, DPP_SCL_V_RATIO(p->v_ratio));
		dpp_reg_set_v_coef(id, p->v_ratio);
	}

	cal_log_debug(id, "h_ratio : %#x, v_ratio : %#x\n",
			p->h_ratio, p->v_ratio);
}

static void dpp_reg_set_img_size(u32 id, u32 w, u32 h)
{
	dpp_write(id, DPP_COM_IMG_SIZE, DPP_IMG_HEIGHT(h) | DPP_IMG_WIDTH(w));
}

static void dpp_reg_set_scaled_img_size(u32 id, u32 w, u32 h)
{
	dpp_write(id, DPP_COM_SCL_SCALED_IMG_SIZE,
		DPP_SCL_SCALED_IMG_HEIGHT(h) | DPP_SCL_SCALED_IMG_WIDTH(w));
}

void dpp_reg_set_hdr_mul_con(u32 id, u32 en)
{
	dpp_write_mask(id, DPP_COM_HDR_CON, en, DEPRE_MUL_EN);
}

static void dpp_reg_set_alpha_type(u32 id, u32 type)
{
	/* [type] 0=per-frame, 1=per-pixel */
	dpp_write_mask(id, DPP_COM_IO_CON, DPP_ALPHA_SEL(type),
			DPP_ALPHA_SEL_MASK);
}

static void dpp_reg_set_format(u32 id, u32 fmt)
{
	dpp_write_mask(id, DPP_COM_IO_CON, DPP_IMG_FORMAT(fmt),
			DPP_IMG_FORMAT_MASK);
}

static void dpp_reg_print_irqs_msg(u32 id, u32 irqs)
{
	u32 cfg_err;

	if (irqs & DPP_CFG_ERROR_IRQ) {
		cfg_err = dpp_read(id, DPP_COM_CFG_ERROR_STATUS);
		cal_log_err(id, "DPP cfg err irq occur(0x%x)\n", cfg_err);
	}
}

static void dpp_reg_set_bpc(u32 id, enum dpp_bpc bpc)
{
	dpp_write_mask(id, DPP_COM_IO_CON, DPP_BPC_MODE(bpc),
			DPP_BPC_MODE_MASK);
}

/********** IDMA and ODMA combination CAL functions **********/
/*
 * STRIDE_0 : Y(or RGB), SAJC header, SBWC-Y header w-stride
 * STRIDE_1 : C, SAJC payload, SBWC-Y payload w-stride
 * STRIDE_2 : SBWC-C header w-stride
 * STRIDE_3 : SBWC-C payload w-stride
 */
static void dma_reg_set_stride(u32 id, struct dpp_params_info *p, const unsigned long attr)
{
	u32 val, i;

	/* Select strides which are non zero */
	for (i = 0; i < MAX_PLANE_ADDR_CNT; i++) {
		val = p->stride[i] ? DMA_STRIDE_SEL(1 << i) : 0;
		dma_write_mask(id, DMA_SRC_STRIDE_0,
				val, DMA_STRIDE_SEL(1<<i));
	}

	/* write the stride values */
	dma_write(id, DMA_SRC_STRIDE_1,
			DMA_STRIDE_0(p->stride[0]) |
			DMA_STRIDE_1(p->stride[1]));
	dma_write(id, DMA_SRC_STRIDE_2,
			DMA_STRIDE_2(p->stride[2]) |
			DMA_STRIDE_3(p->stride[3]));
}

/*
 * P0 : Y(or RGB), SAJC header, SBWC-Y header base address
 * P1 : C, SAJC payload, SBWC-Y payload base address
 * P2 : SBWC-C header base address
 * P3 : SBWC-C payload base address
 */
static void dma_reg_set_base_addr(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	dma_write(id, DMA_BASEADDR_P0, p->addr[0]);
	dma_write(id, DMA_BASEADDR_P1, p->addr[1]);
	if (p->comp_type == COMP_TYPE_SBWC) {
		dma_write(id, DMA_BASEADDR_P2, p->addr[2]);
		dma_write(id, DMA_BASEADDR_P3, p->addr[3]);
	}

	cal_log_debug(id, "base addr p0(%pad) p1(%pad) p2(%pad) p3(%pad)\n",
			&p->addr[0], &p->addr[1], &p->addr[2], &p->addr[3]);
}

/********** IDMA, ODMA, DPP and WB MUX combination CAL functions **********/
static void dma_dpp_reg_set_coordinates(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		idma_reg_set_coordinates(id, &p->src);

		if (test_bit(DPP_ATTR_DPP, &attr)) {
			if (p->rot > DPP_ROT_180)
				dpp_reg_set_img_size(id, p->src.h, p->src.w);
			else
				dpp_reg_set_img_size(id, p->src.w, p->src.h);
		}

		if (test_bit(DPP_ATTR_SCALE, &attr))
			dpp_reg_set_scaled_img_size(id, p->dst.w, p->dst.h);
	} else if (test_bit(DPP_ATTR_ODMA, &attr)) {
		odma_reg_set_coordinates(id, &p->src);
		if (test_bit(DPP_ATTR_DPP, &attr))
			dpp_reg_set_img_size(id, p->src.w, p->src.h);
	}
}

static int dma_dpp_reg_set_format(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	u32 alpha_type = 0; /* 0: per-frame, 1: per-pixel */
	const struct dpu_fmt *fmt_info = dpu_find_fmt_info(p->format);

	alpha_type = (fmt_info->len_alpha > 0) ? 1 : 0;

	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		idma_reg_set_format(id, fmt_info->dma_fmt);
		if (test_bit(DPP_ATTR_DPP, &attr)) {
			dpp_reg_set_bpc(id, p->in_bpc);
			dpp_reg_set_alpha_type(id, alpha_type);
			dpp_reg_set_format(id, fmt_info->dpp_fmt);
		}
	} else if (test_bit(DPP_ATTR_ODMA, &attr)) {
		odma_reg_set_format(id, fmt_info->dma_fmt);
		if (test_bit(DPP_ATTR_DPP, &attr)) {
			dpp_reg_set_bpc(id, p->in_bpc);
			dpp_reg_set_uv_offset(id, 0, 0);
			/* TBD: Do we need this ??*/
			//dpp_reg_set_bext_mode(id, 0); // 0=msb copy, 1=zero padding
			dpp_reg_set_alpha_type(id, alpha_type);
			dpp_reg_set_format(id, fmt_info->dpp_fmt);
		}
	}

	return 0;
}

/****************** DMA RCD CAL functions ******************/
static void rcd_reg_set_sw_reset(u32 id)
{
	dma_write_mask(id, RCD_ENABLE, ~0, RCD_SRESET);
}

static int rcd_reg_wait_sw_reset_status(u32 id)
{
	u32 val;
	int ret;

	ret = readl_poll_timeout_atomic(dma_regs_desc(id)->regs + RCD_ENABLE,
		val, !(val & RCD_SRESET), 10, 2000);
	if (ret) {
		cal_log_err(id, "[idma] timeout sw-reset\n");
		return ret;
	}

	return 0;
}

static void rcd_reg_set_irq_mask_all(u32 id, bool en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, RCD_IRQ, val, RCD_ALL_IRQ_MASK);
}

static void rcd_reg_set_irq_enable(u32 id)
{
	dma_write_mask(id, RCD_IRQ, ~0, RCD_IRQ_ENABLE);
}

static void rcd_reg_set_irq_disable(u32 id)
{
	dma_write_mask(id, RCD_IRQ, 0, RCD_IRQ_ENABLE);
}

static void rcd_reg_set_in_qos_lut(u32 id, u32 lut_id, u32 qos_t)
{
	u32 reg_id;

	if (lut_id == 0)
		reg_id = RCD_QOS_LUT_LOW;
	else
		reg_id = RCD_QOS_LUT_HIGH;

	dma_write(id, reg_id, qos_t);
}

static void rcd_reg_set_sram_clk_gate_en(u32 id, bool en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, RCD_DYNAMIC_GATING_EN, val, RCD_SRAM_CG_EN);
}

static void rcd_reg_set_dynamic_gating_en_all(u32 id, bool en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, RCD_DYNAMIC_GATING_EN, val, RCD_DG_EN_ALL);
}

static void rcd_reg_set_ic_max(u32 id, u32 ic_max)
{
	dma_write_mask(id, RCD_IN_CTRL_0, RCD_IC_MAX(ic_max),
				   RCD_IC_MAX_MASK);
}

static void rcd_reg_clear_irq(u32 id, u32 irq)
{
	dma_write_mask(id, RCD_IRQ, ~0, irq);
}

static void rcd_reg_set_base_addr(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	dma_write(id, RCD_BASEADDR_P0, p->addr[0]);
}

static void rcd_reg_set_coordinates(u32 id, struct decon_frame *src)
{
	dma_write(id, RCD_SRC_OFFSET,
			RCD_SRC_OFFSET_Y(src->y) | RCD_SRC_OFFSET_X(src->x));
	dma_write(id, RCD_SRC_SIZE,
			RCD_SRC_HEIGHT(src->f_h) | RCD_SRC_WIDTH(src->f_w));
	dma_write(id, RCD_IMG_SIZE,
			RCD_IMG_HEIGHT(src->h) | RCD_IMG_WIDTH(src->w));
}

static void rcd_reg_set_block_mode(u32 id, bool en, int x, int y, u32 w, u32 h)
{
        if (!en) {
                dma_write_mask(id, RCD_IN_CTRL_0, 0, RCD_BLOCK_EN);
                return;
        }

        dma_write(id, RCD_BLOCK_OFFSET,
			RCD_BLK_OFFSET_Y(y) | RCD_BLK_OFFSET_X(x));
        dma_write(id, RCD_BLOCK_SIZE, RCD_BLK_HEIGHT(h) | RCD_BLK_WIDTH(w));
        dma_write(id, RCD_BLOCK_VALUE, 255 << 24);
        dma_write_mask(id, RCD_IN_CTRL_0, ~0, RCD_BLOCK_EN);

	cal_log_debug(id, "block x(%d) y(%d) w(%d) h(%d)\n", x, y, w, h);
}

static void rcd_reg_set_deadlock(u32 id, bool en, u32 dl_num)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, RCD_DEADLOCK_CTRL, val, RCD_DEADLOCK_NUM_EN);
	dma_write_mask(id, RCD_DEADLOCK_CTRL, RCD_DEADLOCK_NUM(dl_num),
						RCD_DEADLOCK_NUM_MASK);
}

static void rcd_reg_init(u32 id)
{
	if (dma_read(id, RCD_IRQ) & RCD_DEADLOCK_IRQ) {
		rcd_reg_set_sw_reset(id);
		rcd_reg_wait_sw_reset_status(id);
	}
	rcd_reg_set_irq_mask_all(id, 0);
	rcd_reg_set_irq_enable(id);
	rcd_reg_set_in_qos_lut(id, 0, 0x44444444);
	rcd_reg_set_in_qos_lut(id, 1, 0x44444444);
	rcd_reg_set_sram_clk_gate_en(id, 0);
	rcd_reg_set_dynamic_gating_en_all(id, 0);
	rcd_reg_set_ic_max(id, 0x40); // 0x28 was the value in 8835
}

static int rcd_reg_deinit(u32 id, bool reset, const unsigned long attr)
{
	rcd_reg_clear_irq(id, RCD_ALL_IRQ_CLEAR);
	rcd_reg_set_irq_mask_all(id, 1);

	if (reset) {
		rcd_reg_set_sw_reset(id);
		if (rcd_reg_wait_sw_reset_status(id))
			return -1;
	}

	return 0;
}

static void rcd_reg_configure_params(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	rcd_reg_set_coordinates(id, &p->src);
	rcd_reg_set_base_addr(id, p, attr);
	rcd_reg_set_block_mode(id, p->is_block, p->block.x, p->block.y,
			p->block.w, p->block.h);
	rcd_reg_set_deadlock(id, 1, p->rcv_num * 51);

	dma_write_mask(id, RCD_ENABLE, 1, RCD_SFR_UPDATE_FORCE);
}

/******************** EXPORTED DPP CAL APIs ********************/
/*
 * When LCD is turned on in the bootloader, sometimes the hardware state of
 * DPU_DMA is not cleaned up normally while booting up the kernel.
 * At this time, the hardware may go into abnormal state, so it needs to reset
 * hardware. However, if it resets every time, it could have an overhead, so
 * it's better to reset once.
 */
void dpp_reg_init(u32 id, const unsigned long attr)
{
	if (test_bit(DPP_ATTR_RCD, &attr))
		rcd_reg_init(id);

	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		if (dma_read(id, RDMA_IRQ) & IDMA_DEADLOCK_IRQ) {
			idma_reg_set_sw_reset(id);
			idma_reg_wait_sw_reset_status(id);
		}
		idma_reg_set_irq_mask_all(id, 0);
		idma_reg_set_irq_enable(id);
		idma_reg_set_in_qos_lut(id, 0, 0x44444444);
		idma_reg_set_in_qos_lut(id, 1, 0x44444444);
		/* TODO: clock gating will be enabled */
		idma_reg_set_sram_clk_gate_en(id, 0);
		idma_reg_set_dynamic_gating_en_all(id, 0);
		idma_reg_set_frame_alpha(id, 0xFF);
		idma_reg_set_ic_max(id, 0x40);
		idma_reg_set_assigned_mo(id, 0x40);
		idma_reg_set_pslave_err(id, 1);
	}

	if (test_bit(DPP_ATTR_DPP, &attr)) {
		dpp_reg_set_irq_mask_all(id, 0);
		dpp_reg_set_irq_enable(id);
		dpp_reg_set_linecnt(id, 1);
		dpp_reg_set_pslave_err(id, 1);
	}

	if (test_bit(DPP_ATTR_ODMA, &attr)) {
		if (dma_read(id, WDMA_IRQ) & ODMA_DEADLOCK_IRQ) {
			odma_reg_set_sw_reset(id);
			odma_reg_wait_sw_reset_status(id);
		}
		odma_reg_set_irq_mask_all(id, 0); /* irq unmask */
		odma_reg_set_irq_enable(id);
		odma_reg_set_in_qos_lut(id, 0, 0x44444444);
		odma_reg_set_in_qos_lut(id, 1, 0x44444444);
		odma_reg_set_dynamic_gating_en_all(id, 0);
		odma_reg_set_frame_alpha(id, 0xFF);
		odma_reg_set_ic_max(id, 0x40);
		odma_reg_set_pslave_err(id, 1);
	}
}

int dpp_reg_deinit(u32 id, bool reset, const unsigned long attr)
{
	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		idma_reg_set_irq_disable(id);
		idma_reg_clear_irq(id, IDMA_ALL_IRQ_CLEAR);
		idma_reg_set_irq_mask_all(id, 1);
	}

	if (test_bit(DPP_ATTR_RCD, &attr)) {
		if (rcd_reg_deinit(id, reset, attr))
			return -1;
	}

	if (test_bit(DPP_ATTR_DPP, &attr)) {
		dpp_reg_clear_irq(id, DPP_ALL_IRQ_CLEAR);
		dpp_reg_set_irq_mask_all(id, 1);
	}

	if (test_bit(DPP_ATTR_ODMA, &attr)) {
		odma_reg_clear_irq(id, ODMA_ALL_IRQ_CLEAR);
		odma_reg_set_irq_mask_all(id, 1); /* irq mask */
	}

	if (reset) {
		if (test_bit(DPP_ATTR_IDMA, &attr) &&
				!test_bit(DPP_ATTR_DPP, &attr)) { /* IDMA */
			idma_reg_set_sw_reset(id);
			if (idma_reg_wait_sw_reset_status(id))
				return -1;
		} else if (test_bit(DPP_ATTR_IDMA, &attr) &&
				test_bit(DPP_ATTR_DPP, &attr)) { /* IDMA/DPP */
			idma_reg_set_sw_reset(id);
			dpp_reg_set_sw_reset(id);
			if (idma_reg_wait_sw_reset_status(id) ||
					dpp_reg_wait_sw_reset_status(id))
				return -1;
		} else if (test_bit(DPP_ATTR_ODMA, &attr)) { /* writeback */
			odma_reg_set_sw_reset(id);
			dpp_reg_set_sw_reset(id);
			if (odma_reg_wait_sw_reset_status(id) ||
					dpp_reg_wait_sw_reset_status(id))
				return -1;
		} else {
			cal_log_err(id, "not support attribute(0x%lx)\n", attr);
		}
	}

	return 0;
}

#if defined(DMA_BIST)
static const u32 pattern_data[] = {
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0x000000ff,
	0x000000ff,
	0x000000ff,
	0x000000ff,
};
#endif

/* TODO: fix HDR */
void dpp_reg_configure_params(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	const struct dpu_fmt *fmt = dpu_find_fmt_info(p->format);

	if (test_bit(DPP_ATTR_RCD, &attr)) {
		rcd_reg_configure_params(id, p, attr);
		return;
	}

	if (test_bit(DPP_ATTR_CSC, &attr) && IS_YUV(fmt))
		dpp_reg_set_csc_params(id, p->standard, p->range, attr);

	if (test_bit(DPP_ATTR_SCALE, &attr))
		dpp_reg_set_scale_ratio(id, p);

	/* configure coordinates and size of IDMA, DPP, ODMA and WB MUX */
	dma_dpp_reg_set_coordinates(id, p, attr);

	if (test_bit(DPP_ATTR_ROT, &attr) || test_bit(DPP_ATTR_FLIP, &attr))
		idma_reg_set_rotation(id, p->rot);

	/* configure base address of IDMA and ODMA */
	if (test_bit(DPP_ATTR_IDMA, &attr) || test_bit(DPP_ATTR_ODMA, &attr))
		dma_reg_set_base_addr(id, p, attr);

	/* configure w-stride for IDMA and ODMA */
	if (test_bit(DPP_ATTR_IDMA, &attr) || test_bit(DPP_ATTR_ODMA, &attr))
		dma_reg_set_stride(id, p, attr);

	if (test_bit(DPP_ATTR_BLOCK, &attr))
		idma_reg_set_block_mode(id, p->is_block, p->block.x, p->block.y,
				p->block.w, p->block.h);

	/* configure image format of IDMA, DPP, ODMA and WB MUX */
	dma_dpp_reg_set_format(id, p, attr);

	if (test_bit(DPP_ATTR_SAJC, &attr) || test_bit(DPP_ATTR_SBWC, &attr))
		idma_reg_set_comp(id, p);

	/*
	 * To check HW stuck
	 * dead_lock min: 17ms (17ms: 1-frame time, rcv_time: 1ms)
	 * but, considered DVFS 3x level switch (ex: 200 <-> 600 Mhz)
	 */
	idma_reg_set_deadlock(id, 1, p->rcv_num * 51);

#if defined(DMA_BIST)
	idma_reg_set_test_pattern(id, 0, pattern_data);
#endif
}

u32 dpp_reg_get_irq_and_clear(u32 id)
{
	u32 val;
	/* DPP_COM_IRQ_STATUS 0x18 is removed for WB DPP channel */
	if (id == REGS_DPP_WB_ID) {
		cal_log_err(id, "no dpp irq, only dma irq\n");
		return 0;
	}

	val = dpp_read(id, DPP_COM_IRQ_STATUS);
	dpp_reg_print_irqs_msg(id, val);
	dpp_reg_clear_irq(id, val);

	return val;
}

/*
 * CFG_ERR is cleared when clearing pending bits
 * So, get cfg_err first, then clear pending bits
 */
u32 idma_reg_get_irq_and_clear(u32 id)
{
	u32 val;

	val = dma_read(id, RDMA_IRQ);
	idma_reg_print_irqs_msg(id, val);
	idma_reg_clear_irq(id, val);

	return val;
}

u32 odma_reg_get_irq_and_clear(u32 id)
{
	u32 val;

	val = dma_read(id, WDMA_IRQ);
	odma_reg_print_irqs_msg(id, val);
	odma_reg_clear_irq(id, val);

	return val;
}

void dpp_reg_enable_all_irqs(u32 id, const unsigned long attr)
{
	if (test_bit(DPP_ATTR_RCD, &attr))
		rcd_reg_set_irq_enable(id);
	if (test_bit(DPP_ATTR_IDMA, &attr))
		idma_reg_set_irq_enable(id);
	if (test_bit(DPP_ATTR_DPP, &attr))
		dpp_reg_set_irq_enable(id);
	if (test_bit(DPP_ATTR_ODMA, &attr))
		odma_reg_set_irq_enable(id);
}

void dpp_reg_disable_all_irqs(u32 id, const unsigned long attr)
{
	if (test_bit(DPP_ATTR_RCD, &attr))
		rcd_reg_set_irq_disable(id);
	if (test_bit(DPP_ATTR_IDMA, &attr))
		idma_reg_set_irq_disable(id);
	if (test_bit(DPP_ATTR_DPP, &attr))
		dpp_reg_set_irq_disable(id);
	if (test_bit(DPP_ATTR_ODMA, &attr))
		odma_reg_set_irq_disable(id);
}

void votf_reg_set_global_init(u32 fid, u32 votf_base_addr, bool en)
{
	return;
}

void votf_reg_init_trs(u32 fid)
{
	return;
}

void idma_reg_set_votf_disable(u32 id)
{
#if 0 /* TODO */
	dma_write_mask(id, RDMA_VOTF_EN, 0, IDMA_VOTF_EN(IDMA_VOTF_DISABLE));
#endif
}

bool idma_reg_check_outstanding_count(u32 id)
{
	cal_log_info(id, "undefined\n");
	return false;
}

static void dpp_reg_dump_ch_data(int id, enum dpp_reg_area reg_area,
					const u32 sel[], u32 cnt)
{
	/* TODO: This will be implemented in the future */
}

static void dma_reg_dump_com_debug_regs(int id)
{
	static bool checked;
	const u32 sel_glb[99] = {
		0x0000, 0x0001, 0x0005, 0x0009, 0x000D, 0x000E, 0x0020, 0x0021,
		0x0025, 0x0029, 0x002D, 0x002E, 0x0040, 0x0041, 0x0045, 0x0049,
		0x004D, 0x004E, 0x0060, 0x0061, 0x0065, 0x0069, 0x006D, 0x006E,
		0x0080, 0x0081, 0x0082, 0x0083, 0x00C0, 0x00C1, 0x00C2, 0x00C3,
		0x0100, 0x0101, 0x0200, 0x0201, 0x0202, 0x0300, 0x0301, 0x0302,
		0x0303, 0x0304, 0x0400, 0x4000, 0x4001, 0x4005, 0x4009, 0x400D,
		0x400E, 0x4020, 0x4021, 0x4025, 0x4029, 0x402D, 0x402E, 0x4040,
		0x4041, 0x4045, 0x4049, 0x404D, 0x404E, 0x4060, 0x4061, 0x4065,
		0x4069, 0x406D, 0x406E, 0x4100, 0x4101, 0x4200, 0x4201, 0x4300,
		0x4301, 0x4302, 0x4303, 0x4304, 0x4400, 0x8080, 0x8081, 0x8082,
		0x8083, 0x80C0, 0x80C1, 0x80C2, 0x80C3, 0x8100, 0x8101, 0x8201,
		0x8202, 0x8300, 0x8301, 0x8302, 0x8303, 0x8304, 0x8400, 0xC000,
		0xC001, 0xC002, 0xC005
	};

	cal_log_info(id, "%s: checked = %d\n", __func__, checked);
	if (checked)
		return;

	cal_log_info(id, "-< DMA COMMON DEBUG SFR >-\n");
	dpp_reg_dump_ch_data(id, REG_AREA_DMA_COM, sel_glb, 99);

	checked = true;
}

static void dma_reg_dump_debug_regs(int id)
{
	/* TODO: This will be implemented in the future */
}

static void dpp_reg_dump_debug_regs(int id)
{
	/* TODO: This will be implemented in the future */
}

static void dma_dump_regs(u32 id, void __iomem *dma_regs)
{
	cal_log_info(id, "\n=== DPU_DMA%d SFR DUMP ===\n", id);
	dpu_print_hex_dump(dma_regs, dma_regs + 0x0000, 0x144);
	dpu_print_hex_dump(dma_regs, dma_regs + 0x0180, 0xC);
	dpu_print_hex_dump(dma_regs, dma_regs + 0x0200, 0x8);
	dpu_print_hex_dump(dma_regs, dma_regs + 0x0300, 0x24);

	if (id == REGS_DPP_WB_ID)
		dpu_print_hex_dump(dma_regs, dma_regs + 0x01A0, 0x24);

	dpu_print_hex_dump(dma_regs, dma_regs + 0x0730, 0x4);
	dpu_print_hex_dump(dma_regs, dma_regs + 0x0740, 0x4);

	/* L0,4 only */
	if ((id % 4) == 0)
		dpu_print_hex_dump(dma_regs, dma_regs + 0x0F00, 0x40);

	cal_log_info(id, "=== DPU_DMA%d SHADOW SFR DUMP ===\n", id);
	dpu_print_hex_dump(dma_regs, dma_regs + 0x0000 + DMA_SHD_OFFSET, 0x144);
	dpu_print_hex_dump(dma_regs, dma_regs + 0x0180 + DMA_SHD_OFFSET, 0x8);
	dpu_print_hex_dump(dma_regs, dma_regs + 0x0200 + DMA_SHD_OFFSET, 0x4);

	if (id == REGS_DPP_WB_ID)
		dpu_print_hex_dump(dma_regs, dma_regs + 0x01A0 + DMA_SHD_OFFSET, 0x24);

	dpu_print_hex_dump(dma_regs, dma_regs + 0x0308 + DMA_SHD_OFFSET, 0x1C);
}

void rcd_dma_dump_regs(u32 id, void __iomem *dma_regs)
{
        cal_log_info(id, "\n=== DPU_DMA(RCD%d) SFR DUMP ===\n", id);
        dpu_print_hex_dump(dma_regs, dma_regs + 0x0000, 0x144);
        dpu_print_hex_dump(dma_regs, dma_regs + 0x0200, 0x8);
        dpu_print_hex_dump(dma_regs, dma_regs + 0x0300, 0x24);

	dpu_print_hex_dump(dma_regs, dma_regs + 0x0740, 0x4);

        cal_log_info(id, "=== DPU_DMA(RCD%d) SHADOW SFR DUMP ===\n", id);
        dpu_print_hex_dump(dma_regs, dma_regs + 0x0000 + RCD_SHD_OFFSET, 0x144);
        dpu_print_hex_dump(dma_regs, dma_regs + 0x0300 + RCD_SHD_OFFSET, 0x24);
}

static void dpp_dump_regs(u32 id, void __iomem *regs)
{
	cal_log_info(id, "=== DPP%d SFR DUMP ===\n", id);
	if (id == REGS_DPP_WB_ID)
		dpu_print_hex_dump(regs, regs + 0x0000, 0x70);
	else
		dpu_print_hex_dump(regs, regs + 0x0000, 0x68);

	if ((id % 2) && (id != 5)) {
		/* L1,3,7 only */
		dpu_print_hex_dump(regs, regs + 0x0200, 0xC);
		// skip coef : 0x210 ~ 0x56C
		dpu_print_hex_dump(regs, regs + 0x0570, 0x10);
	}

	cal_log_info(id, "=== DPP%d SHADOW SFR DUMP ===\n", id);
	if (id == REGS_DPP_WB_ID)
		dpu_print_hex_dump(regs, regs + DPP_COM_SHD_OFFSET, 0x70);
	else
		dpu_print_hex_dump(regs, regs + DPP_COM_SHD_OFFSET, 0x68);

	if ((id % 2) && (id != 5)) {
		/* L1,3,7 only */
		dpu_print_hex_dump(regs, regs + 0x0200 + DPP_SCL_SHD_OFFSET,
				0xC);
		// skip coef : (0x210 ~ 0x56C) + DPP_SCL_SHD_OFFSET
		dpu_print_hex_dump(regs, regs + 0x0570 + DPP_SCL_SHD_OFFSET,
				0x10);
	}
}

void __dpp_common_dump(u32 fid, struct dpp_regs *regs)
{
	dma_reg_dump_com_debug_regs(fid);
	dma_reg_dump_debug_regs(fid);
}

void __dpp_dump(u32 id, struct dpp_regs *regs)
{
	void __iomem *dpp_regs = regs->dpp_base_regs;
	void __iomem *dma_regs = regs->dma_base_regs;

	dma_reg_dump_com_debug_regs(id);

	dma_dump_regs(id, dma_regs);
	dma_reg_dump_debug_regs(id);

	dpp_dump_regs(id, dpp_regs);
	dpp_reg_dump_debug_regs(id);

}

void __sfr_dma_dump(void __iomem *sfr_dma_regs)
{
	return;
}

void __rcd_dump(u32 id, struct dpp_regs *regs)
{
	void __iomem *dma_regs = regs->dma_base_regs;
	rcd_dma_dump_regs(id, dma_regs);
}

void __dpp_init_dpuf_resource_cnt(void)
{
	return;
}

int __dpp_check(u32 id, const struct dpp_params_info *p, unsigned long attr)

{
	const struct dpu_fmt *fmt = dpu_find_fmt_info(p->format);

	if (p->comp_type == COMP_TYPE_SBWC) {
		if (!test_bit(DPP_ATTR_SBWC, &attr)) {
			cal_log_err(id, "SBWC is not supported\n");
			return -EINVAL;
		}

		if (IS_RGB(fmt)) {
			cal_log_err(id, "SBWC + RGB format is not supported\n");
			return -EINVAL;
		}
	}

	if (p->comp_type == COMP_TYPE_SBWCL) {
		cal_log_err(id, "SBWC lossy is not supported\n");
		return -ENOTSUPP;
	}

	if (!test_bit(DPP_ATTR_SAJC, &attr) && (p->comp_type == COMP_TYPE_SAJC)) {
		cal_log_err(id, "SAJC is not supported\n");
		return -ENOTSUPP;
	}

	return 0;
}

void dpp_reg_get_version(u32 id, struct ip_version *version)
{
	u32 val = dpp_read(id, DPP_COM_VERSION);

	version->major = HEXSTR2DEC(DPP_COM_VERSION_GET_MAJOR(val));
	version->minor = HEXSTR2DEC(DPP_COM_VERSION_GET_MINOR(val));
}

void idma_reg_get_version(u32 id, struct ip_version *version)
{
	u32 val = dma_read(id, GLB_DPU_DMA_VERSION);

	version->major = HEXSTR2DEC(GLB_DPU_DMA_VERSION_GET_MAJOR(val));
	version->minor = HEXSTR2DEC(GLB_DPU_DMA_VERSION_GET_MINOR(val));
}
