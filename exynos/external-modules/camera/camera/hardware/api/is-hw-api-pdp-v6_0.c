// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <dt-bindings/camera/exynos_is_dt.h>
#if IS_ENABLED(CONFIG_PDP_V14_0)
#include "sfr/is-sfr-pdp-v14_0.h"
#else
#include "sfr/is-sfr-pdp-v6_0.h"
#endif
#include "sfr/is-sfr-csi_pdp_top-v6_0.h"
#include "is-hw-api-pdp-v1.h"
#include "is-device-sensor.h"
#include "is-device-sensor-peri.h"
#include "pablo-debug.h"
#include "pablo-smc.h"

#define PDP_SET_F(base, R, F, val) \
	is_hw_set_field(base, &pdp_regs_corex[R], &pdp_fields[F], val)
#define PDP_SET_F_DIRECT(base, R, F, val) \
	is_hw_set_field(base, &pdp_regs[R], &pdp_fields[F], val)
#define PDP_SET_R(base, R, val) \
	is_hw_set_reg(base, &pdp_regs_corex[R], val)
#define PDP_SET_R_DIRECT(base, R, val) \
	is_hw_set_reg(base, &pdp_regs[R], val)
#define PDP_SET_V(reg_val, F, val) \
	is_hw_set_field_value(reg_val, &pdp_fields[F], val)

#define PDP_GET_F(base, R, F) \
	is_hw_get_field(base, &pdp_regs[R], &pdp_fields[F])
#define PDP_GET_R(base, R) \
	is_hw_get_reg(base, &pdp_regs[R])
#define PDP_GET_R_COREX(base, R) \
	is_hw_get_reg(base, &pdp_regs_corex[R])
#define PDP_GET_V(reg_val, F) \
	is_hw_get_field_value(reg_val, &pdp_fields[F])

#define PDP_RDMA_MO_TICK	10
#define PDP_RDMA_MO_DEFAULT	3
#define PDP_RDMA_MO_FPS60	5
#define PDP_RDMA_MO_FPS240	10
#define PDP_RDMA_MO_FPS480	20

#define BPC_SRAM_MAX		2879

/*
 * Context: O
 * CR type: No Corex
 */
static __always_inline void pdp_hw_wait_corex_idle(void __iomem *base)
{
	u32 try_cnt = 0;

	while (PDP_GET_F(base, PDP_R_COREX_STATUS_0, PDP_F_COREX_BUSY_0)) {
		udelay(1);

		if (++try_cnt >= PDP_TRY_COUNT) {
			err_hw("[PDP]%s:fail to wait corex idle", __func__);
			break;
		}

		dbg_hw(2, "[PDP]%s:try_cnt %d\n", __func__, try_cnt);
	}
}

static void pdp_hw_s_corex_init(void __iomem *base, bool enable)
{
	u32 type_addr, type_num;

	if (!enable) {
		PDP_SET_F(base, PDP_R_COREX_UPDATE_MODE_0, PDP_F_COREX_UPDATE_MODE_0, SW_TRIGGER);
		PDP_SET_F(base, PDP_R_COREX_UPDATE_MODE_1, PDP_F_COREX_UPDATE_MODE_1, SW_TRIGGER);

		pdp_hw_wait_corex_idle(base);

		PDP_SET_F(base, PDP_R_COREX_ENABLE, PDP_F_COREX_ENABLE, 0);

		if (!in_irq())
			info_hw("[PDP]%s:disable COREX done\n", __func__);

		return;
	}

	/* Reset COREX */
	PDP_SET_F(base, PDP_R_COREX_ENABLE, PDP_F_COREX_ENABLE, 1);
	PDP_SET_F(base, PDP_R_COREX_RESET, PDP_F_COREX_RESET, 1);

	/* Initialize COREX type table address */
	PDP_SET_F(base, PDP_R_COREX_TYPE_WRITE_TRIGGER, PDP_F_COREX_TYPE_WRITE_TRIGGER, 1);

	/* Set type 0 into every 32 regs */
	type_num = DIV_ROUND_UP(pdp_regs[(PDP_REG_CNT - 1)].sfr_offset, (4 * 32));
	for (type_addr = 0; type_addr < type_num; type_addr++)
		PDP_SET_F(base, PDP_R_COREX_TYPE_WRITE, PDP_F_COREX_TYPE_WRITE, 0);

	/*
	 * Config COREX mode for type0 regs
	 * @PDP_R_COREX_UPDATE_TYPE_0: 0: Ignore, 1: Copy from SRAM, 2: Swap
	 * @PDP_R_COREX_UPDATE_MODE_0: 0: HW trigger(FE), 1: SW trigger
	 */
	PDP_SET_F(base, PDP_R_COREX_UPDATE_TYPE_0, PDP_F_COREX_UPDATE_TYPE_0, COREX_COPY);
	PDP_SET_F(base, PDP_R_COREX_UPDATE_TYPE_1, PDP_F_COREX_UPDATE_TYPE_1, COREX_IGNORE);
	PDP_SET_F(base, PDP_R_COREX_UPDATE_MODE_0, PDP_F_COREX_UPDATE_MODE_0, SW_TRIGGER);
	PDP_SET_F(base, PDP_R_COREX_UPDATE_MODE_1, PDP_F_COREX_UPDATE_MODE_1, SW_TRIGGER);

	/* Initialize type0 COREX SRAM regs with IP regs */
	PDP_SET_F(base, PDP_R_COREX_COPY_FROM_IP_0, PDP_F_COREX_COPY_FROM_IP_0, 1);

	/* Check COREX idleness */
	pdp_hw_wait_corex_idle(base);

	info_hw("[PDP]%s:done\n", __func__);
}

/*
 * Context: O
 * CR type: No Corex
 */
static void pdp_hw_s_corex_start(void __iomem *base, bool enable)
{
	if (!enable)
		return;

	/*
	 * Set Fixed Value
	 *
	 * Type0 only:
	 * @PDP_R_COREX_UPDATE_MODE_0 - 0: HW trigger, 1: SW tirgger
	 * @PDP_R_COREX_START_0 - 1: Pulse generation
	 *
	 * SW trigger should be needed before stream on
	 * becuse there is no HW trigger(FE) before stream on.
	 */
	PDP_SET_F(base, PDP_R_COREX_UPDATE_MODE_0, PDP_F_COREX_UPDATE_MODE_0, SW_TRIGGER);
	PDP_SET_F(base, PDP_R_COREX_START_0, PDP_F_COREX_START_0, 1);

	/* Wait copy from SRAM */
	pdp_hw_wait_corex_idle(base);

	/* Now, use HW trigger(FE) for SRAM copy */
	PDP_SET_F(base, PDP_R_COREX_UPDATE_MODE_0, PDP_F_COREX_UPDATE_MODE_0, HW_TRIGGER);

	info_hw("[PDP]%s:done\n", __func__);
}

/*
 * Context: O
 * CR type: Corex + No Corex
 */
void pdp_hw_s_line_row(void __iomem *base, bool pd_enable, int sensor_mode)
{
	u32 line_row = 1, val = 0;
	bool col_row_en = true;

	if (!IS_ENABLED(USE_PDP_LINE_INTR_FOR_PDAF)) {
		PDP_SET_R(base, PDP_R_IP_INT_ON_COL_ROW, 0);
		return;
	}

	val = PDP_SET_V(val, PDP_F_IP_INT_COL_CORD, 0);
	val = PDP_SET_V(val, PDP_F_IP_INT_ROW_CORD, line_row);
	PDP_SET_R(base, PDP_R_IP_INT_ON_COL_ROW_CORD, val);

	val = 0;
	val = PDP_SET_V(val, PDP_F_IP_INT_COL_EN, col_row_en);
	val = PDP_SET_V(val, PDP_F_IP_INT_ROW_EN, col_row_en);
	PDP_SET_R(base, PDP_R_IP_INT_ON_COL_ROW, val);

	dbg_hw(2, "[PDP]%s: line_row(%d)", __func__, line_row);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_line_row);

/*
 * Context: O
 * CR type: Corex + No Corex
 */
static void pdp_hw_s_common(void __iomem *base)
{
	/*
	 * Set Fiexed Value
	 *
	 * 1: alows interupt, 0: masks interupt
	 */
	PDP_SET_F(base, PDP_R_INTERRUPT_AUTO_MASK, PDP_F_INTERRUPT_AUTO_MASK, 1);

	/*
	 * If this CR is set to START_ASAP, start & end intterrupt is overllaped.
	 * In other words, there is no v-vblank.
	 */
	PDP_SET_F(base, PDP_R_IP_USE_INPUT_FRAME_START_IN,
			PDP_F_IP_USE_INPUT_FRAME_START_IN, START_VVALID_RISE);
}

/*
 * Context: O
 * CR type: Corex + No Corex
 */
static void pdp_hw_s_int_mask(void __iomem *base, u32 sensor_type, u32 path)
{
	u32 corrupted_int_mask, int2_mask;
	u32 int1_mask = INT1_EN_MASK;

	PDP_SET_F(base, PDP_R_CONTINT_LEVEL_PULSE_N_SEL,
			PDP_F_CONTINT_LEVEL_PULSE_N_SEL, INT_LEVEL);

	/*
	 * The IP_END_INTERRUPT_ENABLE makes and condition.
	 * If some bits are set, frame end interrupt is generated when all designated blocks must be finished.
	 * So, unused block must be not set.
	 * Usually all bit doesn't have to set becase we don't care about each block end.
	 * We are only interested in core end interrupt.
	 */
	PDP_SET_F(base, PDP_R_IP_USE_END_INTERRUPT_ENABLE, PDP_F_IP_USE_END_INTERRUPT_ENABLE, 1);
	PDP_SET_F(base, PDP_R_IP_END_INTERRUPT_ENABLE, PDP_F_IP_END_INTERRUPT_ENABLE, 1); /* input_frame_end_int */

	/* IP_CORRUPTED_INTERRUPT_ENABLE makes or condition. */
	if (path == DMA && sensor_type == SENSOR_TYPE_MOD3) {
		corrupted_int_mask = 0x3F9F;
		int2_mask = INT2_EN_MASK &
			~((1 << CINFIFO_LINES_ERROR) | (1 << CINFIFO_COLUMNS_ERROR));
	} else {
		corrupted_int_mask = 0x3FFF;
		int2_mask = INT2_EN_MASK;
	}

	PDP_SET_F(base, PDP_R_IP_CORRUPTED_INTERRUPT_ENABLE,
			PDP_F_IP_CORRUPTED_INTERRUPT_ENABLE, corrupted_int_mask);
	PDP_SET_F(base, PDP_R_CONTINT_INT1_ENABLE, PDP_F_CONTINT_INT1_ENABLE, int1_mask);
	PDP_SET_F(base, PDP_R_CONTINT_INT2_ENABLE, PDP_F_CONTINT_INT2_ENABLE, int2_mask);
}

/*
 * Context: O
 * CR type: Shadow/Dual
 */
static void pdp_hw_s_secure_id(void __iomem *base)
{
	/* TODO: It should follow the SEQID from CSIS WDMA */
	PDP_SET_F(base, PDP_R_SECU_CTRL_SEQID, PDP_F_SECU_CTRL_SEQID, 0);
}

/*
 * Context: O
 * CR type: Corex
 */
static void pdp_hw_s_af_rdma_init(void __iomem *base, u32 rmo,
	u32 en_votf, u32 en_dma)
{
	if (en_dma == 0) {
		PDP_SET_R(base, PDP_R_RDMA_AF_EN, 0);
		return;
	}

	/*
	 * Keep Reset Value
	 *
	 * @PDP_R_RDMA_AF_MO,
	 * @PDP_R_RDMA_AF_THRESHOLD,
	 */
	PDP_SET_R(base, PDP_R_RDMA_AF_MAX_MO, rmo);

	/* AF */
	PDP_SET_R(base, PDP_R_RDMA_AF_VOTF_EN, en_votf);


	PDP_SET_R(base, PDP_R_RDMA_AF_EN, en_dma);
}

/*
 * When using AF RDMA, line ratio between bayer DMA and af DMA must be 4:1.
 * If the raio is not 4:1, tail count must be reset at frame end time.
 */
static void pdp_hw_s_af_rdma_tail_count_reset(void __iomem *base)
{
	u32 enable;

	/* AF */
	enable = PDP_GET_R(base, PDP_R_RDMA_AF_EN);
	if (enable) {
		PDP_SET_R_DIRECT(base, PDP_R_RDMA_AF_EN, 0);
		PDP_SET_R_DIRECT(base, PDP_R_RDMA_AF_EN, 1);
	}
}

/*============= Global Function =============*/
u32 pdp_hw_g_ip_version(void __iomem *base)
{
	u32 version = PDP_GET_R(base, PDP_R_IP_VERSION);

	info_hw("[PDP] use v%d.%d.%d",
			(version >> 24) & 0xFF,
			(version >> 16) & 0xFF,
			(version >> 0) & 0xFFFF);

	return version;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_ip_version);

unsigned int pdp_hw_g_idle_state(void __iomem *base)
{
	u32 idle;

	idle = PDP_GET_F(base, PDP_R_IDLENESS_STATUS, PDP_F_IDLENESS_STATUS);

	return idle;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_idle_state);

void pdp_hw_get_line(void __iomem *base, u32 *total_line, u32 *curr_line)
{
	*total_line = 0;
	*curr_line = 0;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_get_line);

/*
 * Context: O
 * CR type: No Corex
 */
void pdp_hw_s_global_enable(void __iomem *base, bool enable)
{
	/*
	 * CAUTION
	 * @sequence: The corex must be enabled before global enable.
	 */
	pdp_hw_s_corex_init(base, enable);
	pdp_hw_s_corex_start(base, enable);

	PDP_SET_F(base, PDP_R_GLOBAL_ENABLE_STOP_CRPT,
			PDP_F_GLOBAL_ENABLE_STOP_CRPT, enable);

	PDP_SET_F(base, PDP_R_FRO_GLOBAL_ENABLE, PDP_F_FRO_GLOBAL_ENABLE, enable);
	PDP_SET_F(base, PDP_R_GLOBAL_ENABLE, PDP_F_GLOBAL_ENABLE, enable);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_global_enable);

void pdp_hw_s_global_enable_clear(void __iomem *base)
{
	/* Clearing registers that is affected by global_enable signal */
	PDP_SET_F(base, PDP_R_GLOBAL_ENABLE, PDP_F_GLOBAL_ENABLE_CLEAR, 1);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_global_enable_clear);

/*
 * Context: O
 * CR type: No Corex
 */
int pdp_hw_s_one_shot_enable(struct is_pdp *pdp)
{
	int ret = 0;
	void __iomem *base = pdp->base;

	PDP_SET_F(base, PDP_R_FRO_GLOBAL_ENABLE, PDP_F_FRO_GLOBAL_ENABLE, 0);
	PDP_SET_F(base, PDP_R_GLOBAL_ENABLE, PDP_F_GLOBAL_ENABLE, 0);

	pdp_hw_s_af_rdma_tail_count_reset(base);

	PDP_SET_F(base, PDP_R_FRO_ONE_SHOT_ENABLE, PDP_F_FRO_ONE_SHOT_ENABLE, 0);
	PDP_SET_F(base, PDP_R_FRO_ONE_SHOT_ENABLE, PDP_F_FRO_ONE_SHOT_ENABLE, 1);

	PDP_SET_F(base, PDP_R_ONE_SHOT_ENABLE, PDP_F_ONE_SHOT_ENABLE, 0);
	PDP_SET_F(base, PDP_R_ONE_SHOT_ENABLE, PDP_F_ONE_SHOT_ENABLE, 1);

	return ret;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_one_shot_enable);

void pdp_hw_s_corex_enable(void __iomem *base, bool enable)
{
	/*
	 * CAUTION
	 * @sequence: The corex must be enabled before global enable.
	 */
	pdp_hw_s_corex_init(base, enable);
	pdp_hw_s_corex_start(base, enable);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_corex_enable);

void pdp_hw_s_corex_type(void __iomem *base, u32 type)
{
	PDP_SET_F(base, PDP_R_COREX_UPDATE_TYPE_0, PDP_F_COREX_UPDATE_TYPE_0, type);

	if (type == COREX_IGNORE)
		pdp_hw_wait_corex_idle(base);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_corex_type);

void pdp_hw_g_corex_state(void __iomem *base, u32 *corex_enable)
{
	*corex_enable = PDP_GET_F(base, PDP_R_COREX_ENABLE, PDP_F_COREX_ENABLE);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_corex_state);

void pdp_hw_corex_resume(void __iomem *base)
{
	/* Put COREX into SW Trigger mode */
	PDP_SET_F_DIRECT(base, PDP_R_COREX_UPDATE_TYPE_0, PDP_F_COREX_UPDATE_TYPE_0, COREX_COPY);
	PDP_SET_F_DIRECT(base, PDP_R_COREX_UPDATE_MODE_0, PDP_F_COREX_UPDATE_MODE_0, SW_TRIGGER);

	/* SW trigger to copy from SRAM */
	PDP_SET_F_DIRECT(base, PDP_R_COREX_START_0, PDP_F_COREX_START_0, SW_TRIGGER);

	/* Finishing copy from SRAM */
	pdp_hw_wait_corex_idle(base);

	/* Change into HW trigger mode */
	PDP_SET_F_DIRECT(base, PDP_R_COREX_UPDATE_MODE_0, PDP_F_COREX_UPDATE_MODE_0, HW_TRIGGER);

	info_hw("[PDP] Resume COREX done\n");
}
KUNIT_EXPORT_SYMBOL(pdp_hw_corex_resume);

/* state */
void pdp_hw_g_pdstat_size(u32 *width, u32 *height, u32 *bits_per_pixel)
{
	*width = PDP_STAT_DMA_WIDTH;
	*height = PDP_STAT_TOTAL_SIZE / PDP_STAT_DMA_WIDTH;
	*bits_per_pixel = 8;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_pdstat_size);

void pdp_hw_s_pdstat_path(void __iomem *base, bool enable)
{
	if (!enable) {
		PDP_SET_R(base, PDP_R_RDMA_AF_EN, 0);
		PDP_SET_R(base, PDP_R_Y_REORDER_ON, 0);
	}
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_pdstat_path);

static void pdp_hw_s_pd_size(
	void __iomem *base, u32 width, u32 height, u32 hwformat, u32 potf_en, bool vpd_pdstat_en)
{
	u32 format;
	u32 byte_per_line;
	u32 unpack_fmt;
	u32 linebuf_size;

	switch (hwformat) {
	case HW_FORMAT_RAW10:
		format = PDP_DMA_FMT_U10BIT_UNPACK_MSB_ZERO;
		unpack_fmt = PDP_REORDER_10BIT;
		break;
	case HW_FORMAT_RAW12:
		format = PDP_DMA_FMT_U12BIT_UNPACK_MSB_ZERO;
		unpack_fmt = PDP_REORDER_12BIT;
		break;
	case HW_FORMAT_RAW14:
		format = PDP_DMA_FMT_U14BIT_UNPACK_MSB_ZERO;
		unpack_fmt = PDP_REORDER_14BIT;
		break;
	default:
		err_hw("[PDP] invalid af format (%02X)", hwformat);
		return;
	}

	byte_per_line = ALIGN(width * 16 / BITS_PER_BYTE, 16);

	if (!potf_en)
		unpack_fmt = PDP_REORDER_BYPASS;

	/* HW guide value*/
	if (vpd_pdstat_en)
		linebuf_size = 0xb40;
	else
		linebuf_size = 0;

	/* AF RDMA */
	PDP_SET_F(base, PDP_R_RDMA_AF_DATA_FORMAT, PDP_F_RDMA_AF_DATA_FORMAT_AF, format);
	PDP_SET_F(base, PDP_R_RDMA_AF_WIDTH, PDP_F_RDMA_AF_WIDTH, width);
	PDP_SET_F(base, PDP_R_RDMA_AF_HEIGHT, PDP_F_RDMA_AF_HEIGHT, height);
	PDP_SET_F(base, PDP_R_RDMA_AF_IMG_STRIDE_1P, PDP_F_RDMA_AF_IMG_STRIDE_1P, byte_per_line);

	/* Y Reorder (HPD) */
	PDP_SET_F(base, PDP_R_Y_REORDER_UNPACK, PDP_F_Y_REORDER0_UNPACK_FORMAT, unpack_fmt);
	PDP_SET_F(base, PDP_R_Y_REORDER_LINEBUF_SIZE, PDP_F_Y_REORDER0_LINEBUF_SIZE, linebuf_size);

	/* Y Reorder sub (VPD) */
	PDP_SET_F(base, PDP_R_Y_REORDER_SUB_UNPACK, PDP_F_Y_REORDER1_UNPACK_FORMAT, unpack_fmt);
	PDP_SET_F(base, PDP_R_Y_REORDER_SUB_LINEBUF_SIZE, PDP_F_Y_REORDER1_LINEBUF_SIZE, linebuf_size);
}

static void pdp_hw_s_reorder_cfg(void __iomem *base, bool enable, u32 hwformat)
{
	u32 val = 0;
	u32 input_ppc, gain_shift;

	if (CSIS_PIXEL_PER_CLK == CSIS_PPC_4)
		input_ppc = 1;
	else
		input_ppc = 0;

	switch (hwformat) {
	case HW_FORMAT_RAW10:
		gain_shift = 0;
		break;
	case HW_FORMAT_RAW12:
		gain_shift = 2;
		break;
	case HW_FORMAT_RAW14:
		gain_shift = 4;
		break;
	default:
		err_hw("[PDP] invalid af format (%02X)", hwformat);
		return;
	}

	/* Y Reorder (HPD) */
	val = PDP_SET_V(val, PDP_F_Y_REORDER0_MODE, enable);
	val = PDP_SET_V(val, PDP_F_Y_REORDER0_INPUT_MODE, enable);
	val = PDP_SET_V(val, PDP_F_Y_REORDER0_INPUT_4PPC, input_ppc); /* 0: 8ppc, 1: 4ppc */
	PDP_SET_R(base, PDP_R_Y_REORDER_MODE, val);

	/* Guide value */
	val = 0;
	val = PDP_SET_V(val, PDP_F_Y_REORDER0_BLC_ON, true);
	val = PDP_SET_V(val, PDP_F_Y_REORDER0_BLC_BITMASK_ON, 0);
	PDP_SET_R(base, PDP_R_Y_REORDER_BLC_ON, val);

	/* Guide value */
	PDP_SET_F(base, PDP_R_Y_REORDER_BLC_OFFSET_BEFORE_GAIN, PDP_F_Y_REORDER0_BLC_OFFSET_BEFORE_GAIN, 0);
	PDP_SET_F(base, PDP_R_Y_REORDER_BLC_OFFSET_AFTER_GAIN, PDP_F_Y_REORDER0_BLC_OFFSET_AFTER_GAIN, 0);
	PDP_SET_F(base, PDP_R_Y_REORDER_BLC_GAIN, PDP_F_Y_REORDER0_BLC_GAIN, 1);

	PDP_SET_F(base, PDP_R_Y_REORDER_BLC_GAIN_SHIFT, PDP_F_Y_REORDER0_BLC_GAIN_SHIFT, gain_shift);

	 /* Guide value */
	PDP_SET_F(base, PDP_R_Y_REORDER_BLC_LOW_LIMIT, PDP_F_Y_REORDER0_BLC_LOW_LIMIT, 0);
	PDP_SET_F(base, PDP_R_Y_REORDER_BLC_HIGH_LIMIT, PDP_F_Y_REORDER0_BLC_HIGH_LIMIT, 1023);

	/* Y Reorder sub (VPD) */
	val = PDP_SET_V(val, PDP_F_Y_REORDER1_MODE, enable);
	val = PDP_SET_V(val, PDP_F_Y_REORDER1_INPUT_MODE, enable);
	val = PDP_SET_V(val, PDP_F_Y_REORDER1_INPUT_4PPC, input_ppc); /* 0: 8ppc, 1: 4ppc */
	PDP_SET_R(base, PDP_R_Y_REORDER_SUB_MODE, val);

	 /* Guide value */
	val = 0;
	val = PDP_SET_V(val, PDP_F_Y_REORDER1_BLC_ON, true);
	val = PDP_SET_V(val, PDP_F_Y_REORDER1_BLC_BITMASK_ON, 0);
	PDP_SET_R(base, PDP_R_Y_REORDER_SUB_BLC_ON, val);

	/* Guide value */
	PDP_SET_F(base, PDP_R_Y_REORDER_SUB_BLC_OFFSET_BEFORE_GAIN, PDP_F_Y_REORDER1_BLC_OFFSET_BEFORE_GAIN, 0);
	PDP_SET_F(base, PDP_R_Y_REORDER_SUB_BLC_OFFSET_AFTER_GAIN, PDP_F_Y_REORDER1_BLC_OFFSET_AFTER_GAIN, 0);
	PDP_SET_F(base, PDP_R_Y_REORDER_SUB_BLC_GAIN, PDP_F_Y_REORDER1_BLC_GAIN, 1);

	PDP_SET_F(base, PDP_R_Y_REORDER_SUB_BLC_GAIN_SHIFT, PDP_F_Y_REORDER1_BLC_GAIN_SHIFT, gain_shift);

	 /* Guide value */
	PDP_SET_F(base, PDP_R_Y_REORDER_SUB_BLC_LOW_LIMIT, PDP_F_Y_REORDER1_BLC_LOW_LIMIT, 0);
	PDP_SET_F(base, PDP_R_Y_REORDER_SUB_BLC_HIGH_LIMIT, PDP_F_Y_REORDER1_BLC_HIGH_LIMIT, 1023);
}

void pdp_hw_s_core(struct is_pdp *pdp, bool pd_enable, struct is_sensor_cfg *sensor_cfg,
	u32 pd_width, u32 pd_height, u32 pd_hwformat, u32 sensor_type, u32 path, u32 en_votf,
	u32 num_buffers)
{
	u32 rmo = PDP_RDMA_MO_DEFAULT;
	u32 en_afdma;
	void __iomem *base = pdp->base;
	u32 potf_en;
	bool vpd_pdstat_en = false;

	potf_en = CHECK_POTF_EN(sensor_cfg->input[sensor_cfg->hpd_vc[pdp->otf_id]].extformat);
	dbg_hw(2, "[PDP%d] POTF in = %d\n", pdp->id, potf_en);

	if (sensor_cfg->vpd_vc[CSI_OTF_OUT_SINGLE] > 0)
		vpd_pdstat_en = true;

	pdp_hw_s_pd_size(base, pd_width, pd_height, pd_hwformat, potf_en, vpd_pdstat_en);

	pdp_hw_s_reorder_cfg(base, pd_enable, pd_hwformat);

	pdp_hw_s_common(base);
	pdp_hw_s_int_mask(base, sensor_type, path);

	pdp_hw_s_secure_id(base);

	if (path == DMA) {
		if (sensor_type == SENSOR_TYPE_MOD3 ||
				sensor_type == SENSOR_TYPE_MSPD_TAIL)
			en_afdma = 1;
		else
			en_afdma = 0;
	} else {
		en_afdma = 0;
	}

	pdp_hw_s_af_rdma_init(base, rmo, en_votf, en_afdma);

	pdp_hw_s_fro(base, num_buffers);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_core);

/*
 * Context: X (ch0 only)
 * CR type: No Corex
 */
void pdp_hw_s_init(void __iomem *base)
{
	PDP_SET_F(base, PDP_R_AUTO_MASK_PREADY, PDP_F_AUTO_MASK_PREADY, 1);
	PDP_SET_F(base, PDP_R_IP_PROCESSING, PDP_F_IP_PROCESSING, 1);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_init);

void pdp_hw_s_reset(void __iomem *base)
{
	PDP_SET_R(base, PDP_R_FRO_SW_RESET, 0x2);
	PDP_SET_R(base, PDP_R_SW_RESET, 0x1);

	info_hw("[PDP] SW reset\n");
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_reset);

void pdp_hw_s_path(void __iomem *base, u32 path)
{
	/*
	 * Set Paramer Value
	 *
	 * path
	 * 0: OTF, 1: strgen, 2: RDMA0, 3:RDMA1
	 */
	PDP_SET_F(base, PDP_R_IP_CHAIN_INPUT_SELECT, PDP_F_IP_CHAIN_INPUT_SELECT, path);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_path);

/*
 * Context: O
 * CR type: Corex
 */
void pdp_hw_s_wdma_init(void __iomem *base, u32 ch, u32 pd_dump_mode)
{
	u32 total_size;
	u32 width = PDP_STAT_DMA_WIDTH;
	u32 height;
	u32 stride;
	u32 format = PDP_STAT_FORMAT;
	u32 stat0_size = PDP_STAT_DMA_WIDTH * PDP_STAT0_ROI_NUM;
	u32 mroi_on, mroi_no_x, mroi_no_y;
	u32 roi_w, roi_h;

	/*
	 * Set Fixed Value
	 * tatal_size = width * height
	 */

	mroi_on = PDP_GET_R_COREX(base, PDP_R_Y_PDSTAT_ROI_MAIN_MWS_ON);
	mroi_no_x = PDP_GET_R_COREX(base, PDP_R_Y_PDSTAT_ROI_MAIN_MWS_NO_X);
	mroi_no_y = PDP_GET_R_COREX(base, PDP_R_Y_PDSTAT_ROI_MAIN_MWS_NO_Y);
	roi_w = PDP_GET_R_COREX(base, PDP_R_Y_PDSTAT_IN_SIZE_X);
	roi_h = PDP_GET_R_COREX(base, PDP_R_Y_PDSTAT_IN_SIZE_Y);

	total_size = stat0_size + (mroi_on * (mroi_no_x * mroi_no_y * PDP_STAT_DMA_WIDTH + 4));
	height = (total_size + (width - 1)) / width;

	if (pd_dump_mode) {
		/* TODO: get detail guide */
		width = roi_w * 2;
		height = roi_h;

		stride = PDP_STAT_DUMP_STRIDE(roi_w);
		format = PDP_DMA_FMT_U10BIT_PACK;

		PDP_SET_R(base, PDP_R_Y_PDSTAT_DUMP_MODE, pd_dump_mode);
	} else {
		height = (total_size + (width - 1)) / width;
		stride = PDP_STAT_STRIDE(width);
	}

	PDP_SET_F(base, PDP_R_WDMA_STAT_AUTO_FLUSH_EN, PDP_F_WDMA_STAT_AUTO_FLUSH_EN, 0);
	PDP_SET_F(base, PDP_R_WDMA_STAT_DATA_FORMAT, PDP_F_WDMA_STAT_DATA_FORMAT_BAYER, format);
	PDP_SET_F(base, PDP_R_WDMA_STAT_WIDTH, PDP_F_WDMA_STAT_WIDTH, width);
	PDP_SET_F(base, PDP_R_WDMA_STAT_HEIGHT, PDP_F_WDMA_STAT_HEIGHT, height);
	PDP_SET_F(base, PDP_R_WDMA_STAT_IMG_STRIDE_1P, PDP_F_WDMA_STAT_IMG_STRIDE_1P, stride);

	info("[PDP%d] pdstat_in_size (x:%d, y:%d)", ch, roi_w, roi_h);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_wdma_init);

dma_addr_t pdp_hw_g_wdma_addr(void __iomem *base)
{
	dma_addr_t ret;

	ret = (dma_addr_t)PDP_GET_R(base, PDP_R_WDMA_STAT_IMG_BASE_ADDR_1P_FRO0);
	ret = (dma_addr_t)(ret << LSB_BIT) |
		(dma_addr_t)(PDP_GET_R(base,
			PDP_R_WDMA_STAT_IMG_BASE_ADDR_1P_FRO0_LSB_4B) & LSB_4BITS_MASK);
	return ret;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_wdma_addr);

/*
 * Context: O
 * CR type: Corex
 */
void pdp_hw_s_wdma_enable(void __iomem *base, dma_addr_t address)
{
	/*
	 * Set Param Value
	 */
	PDP_SET_F(base, PDP_R_WDMA_STAT_IMG_BASE_ADDR_1P_FRO0,
		PDP_F_WDMA_STAT_IMG_BASE_ADDR_1P_FRO0,
		(u32)DVA_GET_MSB(address));
	PDP_SET_F(base, PDP_R_WDMA_STAT_IMG_BASE_ADDR_1P_FRO0_LSB_4B,
		PDP_F_WDMA_STAT_IMG_BASE_ADDR_1P_FRO0_LSB_4B,
		(u32)DVA_GET_LSB(address));
	/*
	 * Set Fixed Value
	 */
	PDP_SET_F(base, PDP_R_WDMA_STAT_EN, PDP_F_WDMA_STAT_EN, 1);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_wdma_enable);

void pdp_hw_s_wdma_disable(void __iomem *base)
{
	PDP_SET_F(base, PDP_R_WDMA_STAT_EN, PDP_F_WDMA_STAT_EN, 0);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_wdma_disable);

/*
 * Context: O
 * CR type: Corex
 */
void pdp_hw_s_af_rdma_addr(void __iomem *base, dma_addr_t *address,
		u32 num_buffers, u32 direct)
{
	int i = 0;

	do {
		PDP_SET_F(base, PDP_R_RDMA_AF_IMG_BASE_ADDR_1P_FRO0 + i,
				PDP_F_RDMA_AF_IMG_BASE_ADDR_1P_FRO0,
				(u32)DVA_GET_MSB(address[i]));
		PDP_SET_F(base, PDP_R_RDMA_AF_IMG_BASE_ADDR_1P_FRO0_LSB_4B + i,
				PDP_F_RDMA_AF_IMG_BASE_ADDR_1P_FRO0_LSB_4B,
				(u32)DVA_GET_LSB(address[i]));
		if (direct) {
			PDP_SET_F_DIRECT(base, PDP_R_RDMA_AF_IMG_BASE_ADDR_1P_FRO0 + i,
				PDP_F_RDMA_AF_IMG_BASE_ADDR_1P_FRO0,
				(u32)DVA_GET_MSB(address[i]));
			PDP_SET_F_DIRECT(base, PDP_R_RDMA_AF_IMG_BASE_ADDR_1P_FRO0_LSB_4B + i,
				PDP_F_RDMA_AF_IMG_BASE_ADDR_1P_FRO0_LSB_4B,
				(u32)DVA_GET_LSB(address[i]));
		}
	} while (++i < num_buffers && i < 8);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_af_rdma_addr);

/*
 * Context: O
 * CR type: Corex
 */
void pdp_hw_s_post_frame_gap(void __iomem *base, u32 interval)
{
	/* Increase V-blank interval */
	PDP_SET_R(base, PDP_R_IP_POST_FRAME_GAP, interval);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_post_frame_gap);

/*
 * Context: O
 * CR type: No Corex
 */
void pdp_hw_s_fro(void __iomem *base, u32 num_buffers)
{
	u32 center_frame_num;
	u32 prev_fro_en = PDP_GET_F(base, PDP_R_FRO_MODE_EN, PDP_F_FRO_MODE_EN);
	u32 fro_en = (num_buffers > 1) ? 1 : 0;

	if (prev_fro_en != fro_en) {
		info_hw("[PDP] FRO: %d -> %d\n", prev_fro_en, fro_en);
		if (fro_en)
			info_hw("[PDP] FRO ON. num_buffers %d\n", num_buffers);

		/* fro core reset */
		PDP_SET_R(base, PDP_R_FRO_SW_RESET, 0x1);
	}

	if (num_buffers > 1) {
		PDP_SET_F(base, PDP_R_FRO_MODE_EN, PDP_F_FRO_MODE_EN, 1);
	} else {
		PDP_SET_F(base, PDP_R_FRO_FRAME_COUNT_TO_RUN_MINUS1_SHADOW,
			PDP_F_FRO_FRAME_COUNT_TO_RUN_MINUS1_SHADOW, 0);
		PDP_SET_F(base, PDP_R_FRO_FRAME_COUNT, PDP_F_FRO_FRAME_COUNT, 0);
		PDP_SET_F(base, PDP_R_FRO_MODE_EN, PDP_F_FRO_MODE_EN, 0);

		return;
	}

	center_frame_num = num_buffers / 2 - 1;

	/* Param: frame */
	PDP_SET_F(base, PDP_R_FRO_FRAME_COUNT_TO_RUN_MINUS1_SHADOW,
		PDP_F_FRO_FRAME_COUNT_TO_RUN_MINUS1_SHADOW, num_buffers - 1);
	PDP_SET_F(base, PDP_R_FRO_RUN_FRAME_NUMBER_FOR_COL_ROW_INT,
		PDP_F_FRO_RUN_FRAME_NUMBER_FOR_COL_ROW_INT, center_frame_num);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_fro);

int pdp_hw_wait_idle(void __iomem *base, unsigned long state, u32 frame_time)
{
	int ret = 0;
	u32 idle;
	u32 int1_all;
	u32 try_cnt = 0;
	u32 max_try_cnt = (frame_time > PDP_TRY_COUNT) ? frame_time : PDP_TRY_COUNT;

	idle = pdp_hw_g_idle_state(base);
	int1_all = PDP_GET_R(base, PDP_R_CONTINT_INT1);

	info_hw("[PDP] idle status before disable (idle %d int1 0x%X)\n", idle, int1_all);

	pdp_hw_s_global_enable(base, false);
	while (!idle) {
		idle = pdp_hw_g_idle_state(base);

		if (++try_cnt >= max_try_cnt) {
			err_hw("[PDP] timeout waiting idle - disable fail, state %lx frame_time %d",
					state, frame_time);
			pdp_hw_dump(base);
			ret = -ETIME;
			break;
		}

		usleep_range(6, 8);
	};

	int1_all = PDP_GET_R(base, PDP_R_CONTINT_INT1);

	info_hw("[PDP] idle status after disable (idle %d int1 0x%X)\n", idle, int1_all);

	return ret;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_wait_idle);

int pdp_hw_rdma_wait_idle(void __iomem *base)
{
	int ret = 0;
	u32 busy_af;
	u32 try_cnt = 0;
	u32 max_try_cnt = PDP_TRY_COUNT;

	busy_af = PDP_GET_R(base, PDP_R_RDMA_AF_MON_STATUS3);
	while (busy_af) {
		busy_af = PDP_GET_R(base, PDP_R_RDMA_AF_MON_STATUS3);

		try_cnt++;
		if (try_cnt >= max_try_cnt) {
			err_hw("[PDP] timeout rdma waiting idle(af:0x%x)",
				busy_af);
			pdp_hw_dump(base);
			ret = -ETIME;
			break;
		}

		usleep_range(10, 11);
	};

	return ret;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_rdma_wait_idle);

/* sensor type */
bool pdp_hw_to_sensor_type(u32 pd_mode, u32 *sensor_type)
{
	bool enable;

	switch (pd_mode) {
	case PD_MSPD:
		*sensor_type = SENSOR_TYPE_MSPD;
		enable = true;
		break;
	case PD_MOD1:
		*sensor_type = SENSOR_TYPE_MOD1;
		enable = true;
		break;
	case PD_MOD2:
	case PD_NONE:
		*sensor_type = SENSOR_TYPE_MOD2;
		enable = false;
		break;
	case PD_MOD3:
		*sensor_type = SENSOR_TYPE_MOD3;
		enable = true;
		break;
	case PD_MSPD_TAIL:
		*sensor_type = SENSOR_TYPE_MSPD_TAIL;
		enable = true;
		break;
	default:
		warn("PD MODE(%d) is invalid", pd_mode);
		*sensor_type = SENSOR_TYPE_MOD2;
		enable = false;
		break;
	}

	return enable;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_to_sensor_type);

void pdp_hw_g_input_vc(u32 mux_val, u32 *img_vc, u32 *af_vc)
{
	unsigned int bit_start, bit_mask;

	bit_start = csis_top_fields[CSIS_F_MUX_IMG_VC_PDP0_A].bit_start;
	bit_mask = GENMASK(csis_top_fields[CSIS_F_MUX_IMG_VC_PDP0_A].bit_width - 1, 0);
	*img_vc = (mux_val >> bit_start) & bit_mask;

	bit_start = csis_top_fields[CSIS_F_MUX_AF0_VC_PDP0_A].bit_start;
	bit_mask = GENMASK(csis_top_fields[CSIS_F_MUX_AF0_VC_PDP0_A].bit_width - 1, 0);
	*af_vc = (mux_val >> bit_start) & bit_mask;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_input_vc);

void pdp_hw_g_input_mux_val(u32 *mux_val, u32 img_vc, u32 af_vc)
{
	*mux_val = (img_vc << csis_top_fields[CSIS_F_MUX_IMG_VC_PDP0_A].bit_start) |
		   (af_vc << csis_top_fields[CSIS_F_MUX_AF0_VC_PDP0_A].bit_start);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_input_mux_val);

/* pattern generator */
void pdp_hw_strgen_enable(void __iomem *base)
{
}
KUNIT_EXPORT_SYMBOL(pdp_hw_strgen_enable);

void pdp_hw_strgen_disable(void __iomem *base)
{
}
KUNIT_EXPORT_SYMBOL(pdp_hw_strgen_disable);

/* IRQ function */
unsigned int pdp_hw_g_int1_state(void __iomem *base, bool clear, u32 *irq_state)
{
	u32 src_all, src_fro, src_err;

	/*
	 * src_all: per-frame based PDP IRQ status
	 * src_fro: FRO based PDP IRQ status
	 *
	 * final normal status: src_fro (start, line, end)
	 * final error status(src_err): src_all & ERR_MASK
	 */
	src_all = PDP_GET_R(base, PDP_R_CONTINT_INT1);
	src_fro = PDP_GET_R(base, PDP_R_FRO_INT0);

	if (clear) {
		PDP_SET_R(base, PDP_R_CONTINT_INT1_CLEAR, src_all);
		PDP_SET_R(base, PDP_R_FRO_INT0_CLEAR, src_fro);
	}

	src_err = src_all & INT1_ERR_MASK;

	*irq_state = src_all;

	return src_fro | src_err;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_int1_state);

unsigned int pdp_hw_g_int1_mask(void __iomem *base)
{
	return PDP_GET_R(base, PDP_R_CONTINT_INT1_ENABLE);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_int1_mask);

unsigned int pdp_hw_g_int2_state(void __iomem *base, bool clear, u32 *irq_state)
{
	u32 src_all, src_fro, src_err;

	/*
	 * src_all: per-frame based PDP IRQ status
	 * src_fro: FRO based PDP IRQ status
	 *
	 * final normal status: src_fro (PDAF_STAT_INT)
	 * final error status(src_err): src_all & ERR_MASK
	 */
	src_all = PDP_GET_R(base, PDP_R_CONTINT_INT2);
	src_fro = PDP_GET_R(base, PDP_R_FRO_INT1);

	if (clear) {
		PDP_SET_R(base, PDP_R_CONTINT_INT2_CLEAR, src_all);
		PDP_SET_R(base, PDP_R_FRO_INT1_CLEAR, src_fro);
	}

	src_err = src_all & INT2_ERR_MASK;

	*irq_state = src_all;

	return src_fro | src_err;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_int2_state);

unsigned int pdp_hw_g_int2_mask(void __iomem *base)
{
	return PDP_GET_R(base, PDP_R_CONTINT_INT2_ENABLE);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_int2_mask);

unsigned int pdp_hw_is_occurred(unsigned int state, enum pdp_event_type type)
{
	u32 mask;

	switch (type) {
	case PE_START:
		mask = (1 << FRAME_START);
		break;
	case PE_END:
		mask = 1 << FRAME_END_INTERRUPT;
		break;
	case PE_PAF_STAT0:
		mask = 1 << PDAF_STAT_INT;
		break;
	case PE_PAF_STAT1:
		mask = 1 << FRAME_END_INTERRUPT;
		break;
	case PE_PAF_STAT2:
		return 0;
	case PE_ERR_INT1:
		mask = INT1_ERR_MASK;
		break;
	case PE_PAF_OVERFLOW:
		mask = (1 << COUTFIFO_OVERFLOW_ERROR) | (1 << CINFIFO_STREAM_OVERFLOW);
		break;
	case PE_ERR_INT2:
		mask = INT2_ERR_MASK;
		break;
	case PE_SPECIAL:
		mask = (1 << FRAME_START);
		break;
	default:
		return 0;
	}

	return state & mask;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_is_occurred);

void pdp_hw_g_int1_str(const char **int_str)
{
	int i;

	for (i = 0; i < PDP_INT1_CNT; i++)
		int_str[i] = pdp_int1_str[i];
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_int1_str);

void pdp_hw_g_int2_str(const char **int_str)
{
	int i;

	for (i = 0; i < PDP_INT2_CNT; i++)
		int_str[i] = pdp_int2_str[i];
}
KUNIT_EXPORT_SYMBOL(pdp_hw_g_int2_str);

/* Debug function */
void pdp_hw_s_config_default(void __iomem *base)
{
	int i;
	u32 index;
	u32 value;
	u32 corex_enable;
	int count = ARRAY_SIZE(pdp_global_init_table);

	corex_enable = PDP_GET_F(base, PDP_R_COREX_ENABLE, PDP_F_COREX_ENABLE);
	PDP_SET_F(base, PDP_R_COREX_ENABLE, PDP_F_COREX_ENABLE, 0);

	for (i = 0; i < count; i++) {
		index = pdp_global_init_table[i].index;
		value = pdp_global_init_table[i].init_value;

		writel(value, base + index);
	}

	PDP_SET_F(base, PDP_R_COREX_ENABLE, PDP_F_COREX_ENABLE, corex_enable);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_config_default);

void pdp_hw_s_input_mux(struct is_pdp *pdp, u32 otf_out_id)
{
	struct is_sensor_cfg *sensor_cfg = pdp->sensor_cfg;

	if (IS_ENABLED(CONFIG_PDP_INPUT_MUX)) {
		u32 val = 0;

		val = PDP_SET_V(val, PDP_F_MUX_AF0_VC,
				sensor_cfg->hpd_vc[otf_out_id] % CSIS_OTF_CH_LC_NUM);
		val = PDP_SET_V(val, PDP_F_MUX_AF1_VC,
				sensor_cfg->vpd_vc[otf_out_id] % CSIS_OTF_CH_LC_NUM);
		val = PDP_SET_V(val, PDP_F_MUX_IMG_VC,
				sensor_cfg->img_vc[otf_out_id] % CSIS_OTF_CH_LC_NUM);

		PDP_SET_R(pdp->base, PDP_R_PDP_VC_CON, val);
	} else {
		writel(pdp->mux_val[pdp->csi_ch], pdp->mux_base);
	}
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_input_mux);

void pdp_hw_s_input_enable(struct is_pdp *pdp, bool on)
{
	u32 en_val, i;

	if (pdp->en_base) {
		en_val = readl(pdp->en_base);

		for (i = 0; i < pdp->en_elems; i++)
			en_val &= ~(1 << pdp->en_val[i]);

		if (on)
			en_val |= 1 << pdp->en_val[pdp->csi_ch];

		writel(en_val, pdp->en_base);
	}
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_input_enable);

void pdp_hw_s_default_blk_cfg(void __iomem *base)
{
	PDP_SET_R(base, PDP_R_Y_REORDER_ON, 0);
	PDP_SET_R(base, PDP_R_Y_REORDER_SUB_ON, 0);
	PDP_SET_R(base, PDP_R_Y_PDBPC0_BYPASS, 1);
	PDP_SET_R(base, PDP_R_Y_PDBPC1_BYPASS, 1);
	PDP_SET_R(base, PDP_R_Y_ALC_ON, 0);
	PDP_SET_R(base, PDP_R_Y_ALC_SUB_ON, 0);
	PDP_SET_R(base, PDP_R_Y_GAMMA_ON, 0);
	PDP_SET_R(base, PDP_R_Y_GAMMA_SUB_ON, 0);
	PDP_SET_R(base, PDP_R_Y_PDSTAT_SAT_ON, 0);
	PDP_SET_R(base, PDP_R_Y_PDSTAT_SAT_SUB_ON, 0);
	PDP_SET_R(base, PDP_R_Y_PDSTAT_XCOR_H_ON, 0);
	PDP_SET_R(base, PDP_R_Y_PDSTAT_XCOR_V_ON, 0);
}
KUNIT_EXPORT_SYMBOL(pdp_hw_s_default_blk_cfg);

int pdp_hw_dump(void __iomem *base)
{
	info_hw("[PDP] REG DUMP (v6.0)\n");

	is_hw_dump_regs(base, pdp_regs, PDP_REG_CNT);

	return 0;
}
KUNIT_EXPORT_SYMBOL(pdp_hw_dump);
