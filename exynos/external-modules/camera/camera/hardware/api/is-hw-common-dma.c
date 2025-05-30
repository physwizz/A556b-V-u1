// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-hw-common-dma.h"
#include "pablo-hw-api-common.h"
#include "sfr/is-sfr-common-dma.h"

#define SET_F(base, R, F, val) \
	is_hw_set_field(base, &dma_regs[R], &dma_fields[F], val)
#define SET_R(base, R, val) \
	is_hw_set_reg(base, &dma_regs[R], val)
#define SET_V(reg_val, F, val) \
	is_hw_set_field_value(reg_val, &dma_fields[F], val)

#define GET_F(base, R, F) \
	is_hw_get_field(base, &dma_regs[R], &dma_fields[F])
#define GET_R(base, R) \
	is_hw_get_reg(base, &dma_regs[R])
#define GET_V(reg_val, F) \
	is_hw_get_field_value(reg_val, &dma_fields[F])

static void __iomem *__is_hw_dma_get_base_addr(struct is_common_dma *dma)
{
	void __iomem *base;

	switch (dma->set_id) {
	case COREX_SET_A:
	case COREX_SET_B:
	case COREX_SET_C:
	case COREX_SET_D:
		base = dma->corex_base[dma->set_id];
		break;
	default:
		base = dma->base;
		break;
	}

	return base;
}

static int is_hw_dma_set_format(struct is_common_dma *dma, u32 format, u32 format_type)
{
	int ret = DMA_OPS_SUCCESS;
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	switch (format_type) {
	case DMA_FMT_BAYER:
		if (test_bit(format, &dma->available_bayer_format_map)) {
			SET_F(base, DMA_R_DATA_FORMAT, DMA_F_DATA_FORMAT_BAYER, format);
		} else {
			ret = DMA_OPS_ERROR;
			err_hw("[CMN DMA] not available_bayer_format_map[%d]", format);
		}
		break;
	case DMA_FMT_YUV:
		if (test_bit(format, &dma->available_yuv_format_map)) {
			SET_F(base, DMA_R_DATA_FORMAT,
				DMA_F_DATA_FORMAT_YUV, format);
			SET_F(base, DMA_R_DATA_FORMAT,
				DMA_F_DATA_FORMAT_TYPE, format_type);
		} else {
			ret = DMA_OPS_ERROR;
			err_hw("[CMN DMA] not available_yuv_format_map[%d]", format);
		}
		break;
	case DMA_FMT_RGB:
		if (test_bit(format, &dma->available_rgb_format_map)) {
			SET_F(base, DMA_R_DATA_FORMAT,
				DMA_F_DATA_FORMAT_RGB, format);
			SET_F(base, DMA_R_DATA_FORMAT,
				DMA_F_DATA_FORMAT_TYPE, format_type);
		} else {
			ret = DMA_OPS_ERROR;
			err_hw("[CMN DMA] not available_rgb_format_map[%d]", format);
		}
		break;
	default:
		err_hw("[CMN DMA] invalid format_type[%d]", format_type);
		return -EINVAL;
	}

	return ret;
}

static int is_hw_dma_set_msb_align(struct is_common_dma *dma, u32 sign, u32 msb_align)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	if (!sign) {
		err_hw("[CMN DMA] Not supported unsign");
		return -EINVAL;
	}

	SET_F(base, DMA_R_DATA_FORMAT, DMA_F_DATA_FORMAT_MSBALIGN, msb_align);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_comp_error(struct is_common_dma *dma, u32 mode, u32 value)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_COMP_ERROR_MODE, mode);
	SET_R(base, DMA_R_COMP_ERROR_VALUE, value);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_comp_rate(struct is_common_dma *dma, u32 rate)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_COMP_LOSSY_BYTE32NUM, rate);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_auto_flush_en(struct is_common_dma *dma, u32 en)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_AUTO_FLUSH_EN, en);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_size(struct is_common_dma *dma, u32 width, u32 height)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_WIDTH, width);
	SET_R(base, DMA_R_HEIGHT, height);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_img_stride(struct is_common_dma *dma, u32 stride_1p, u32 stride_2p, u32 stride_3p)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_IMG_STRIDE_1P, stride_1p);
	SET_R(base, DMA_R_IMG_STRIDE_2P, stride_2p);
	SET_R(base, DMA_R_IMG_STRIDE_3P, stride_3p);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_header_stride(struct is_common_dma *dma, u32 stride_1p, u32 stride_2p)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_HEADER_STRIDE_1P, stride_1p);
	SET_R(base, DMA_R_HEADER_STRIDE_2P, stride_2p);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_max_mo(struct is_common_dma *dma, u32 max_mo)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_MAX_MO, max_mo);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_line_gap(struct is_common_dma *dma, u32 line_gap)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_LINE_GAP, line_gap);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_img_addr(struct is_common_dma *dma, dma_addr_t *addr, u32 plane, int buf_idx, u32 num_buffers)
{
	int i = 0;
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	do {
		SET_R(base, DMA_R_IMG_BASE_ADDR_1P_FRO_00 + (plane * IS_MAX_FRO) + buf_idx + i, (u32)addr[i]);
	} while (++i < num_buffers);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_header_addr(struct is_common_dma *dma, dma_addr_t *addr,
	u32 plane, int buf_idx, u32 num_buffers)
{
	int i = 0;
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	do {
		SET_R(base, DMA_R_HEADER_BASE_ADDR_1P_FRO_00 + (plane * IS_MAX_FRO) + buf_idx + i, (u32)addr[i]);
	} while (++i < num_buffers);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_votf_enable(struct is_common_dma *dma, u32 enable, u32 stall_enable)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_VOTF_EN, enable);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_enable(struct is_common_dma *dma, u32 enable)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_EN, enable);
	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_comp_sbwc_en(struct is_common_dma *dma, u32 comp_sbwc_en)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_F(base, DMA_R_COMP_CTRL, DMA_F_COMP_SBWC_EN, comp_sbwc_en);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_get_comp_sbwc_type(struct is_common_dma *dma, u32 *comp_sbwc_en)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	*comp_sbwc_en = GET_F(base, DMA_R_COMP_CTRL, DMA_F_COMP_SBWC_EN);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_comp_64b_align(struct is_common_dma *dma, u32 comp_64b_align)
{
	SET_F(dma->base, DMA_R_COMP_CTRL, DMA_F_COMP_SBWC_64B_ALIGN, comp_64b_align);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_mono_mode(struct is_common_dma *dma, u32 mono_mode)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_MONO_MODE, mono_mode);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_max_bl(struct is_common_dma *dma, u32 max_bl)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_MAX_BL, max_bl);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_set_bus_info(struct is_common_dma *dma, u32 bus_info)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	SET_R(base, DMA_R_BUSINFO, bus_info);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_get_enable(struct is_common_dma *dma, u32 *enable)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	*enable = GET_R(base, DMA_R_EN);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_get_max_mo(struct is_common_dma *dma, u32 *max_mo)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	*max_mo = GET_R(base, DMA_R_MAX_MO);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_get_img_crc(struct is_common_dma *dma, u32 *crc_1p, u32 *crc_2p, u32 *crc_3p)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	*crc_1p = GET_R(base, DMA_R_IMG_CRC_1P);
	*crc_2p = GET_R(base, DMA_R_IMG_CRC_2P);
	*crc_3p = GET_R(base, DMA_R_IMG_CRC_3P);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_get_header_crc(struct is_common_dma *dma, u32 *crc_1p, u32 *crc_2p)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	*crc_1p = GET_R(base, DMA_R_HEADER_CRC_1P);
	*crc_2p = GET_R(base, DMA_R_HEADER_CRC_2P);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_get_mon_status(struct is_common_dma *dma, u32 *mon_status)
{
	void __iomem *base = __is_hw_dma_get_base_addr(dma);

	*mon_status = GET_R(base, DMA_R_MON_STATUS_0);

	return DMA_OPS_SUCCESS;
}

static int is_hw_dma_print_info(struct is_common_dma *dma, u32 option)
{
	u32 enable = 0;

	CALL_DMA_OPS(dma, dma_get_enable, &enable);

	if (!enable) {
		info(" %s disabled", dma->name);
		return 0;
	}

	info(" %s DUMP!!", dma->name);
	is_hw_dump_regs(dma->base, dma_regs, DMA_REG_CNT);

	return DMA_OPS_SUCCESS;
}

int is_hw_dma_create(struct is_common_dma *dma, void __iomem *base, int id,
			const char *name, ulong available_bayer_format_map,
			ulong available_yuv_format_map,
			ulong available_rgb_format_map)
{
	int set_id = 0;
	dma->base = base;
	for (set_id = COREX_SET_A; set_id < COREX_DIRECT; set_id++)
		dma->corex_base[set_id] = dma->base + GET_COREX_OFFSET(set_id);

	dma->id = id;

	strncpy(dma->name, name, (sizeof(dma->name) - 1));
	dma->available_bayer_format_map = available_bayer_format_map;
	dma->available_yuv_format_map = available_yuv_format_map;
	dma->available_rgb_format_map = available_rgb_format_map;
	return DMA_OPS_SUCCESS;
}
EXPORT_SYMBOL_GPL(is_hw_dma_create);

static int is_hw_dma_set_corex_id(struct is_common_dma *dma, int set_id)
{
	dma->set_id = set_id;
	return DMA_OPS_SUCCESS;
}

const struct is_common_dma_ops is_hw_dma_ops = {
	.dma_set_corex_id		= is_hw_dma_set_corex_id,
	.dma_set_format			= is_hw_dma_set_format,
	.dma_set_msb_align		= is_hw_dma_set_msb_align,
	.dma_set_comp_sbwc_en		= is_hw_dma_set_comp_sbwc_en,
	.dma_set_comp_64b_align		= is_hw_dma_set_comp_64b_align,
	.dma_set_comp_error		= is_hw_dma_set_comp_error,
	.dma_set_comp_rate		= is_hw_dma_set_comp_rate,
	.dma_set_mono_mode		= is_hw_dma_set_mono_mode,
	.dma_set_auto_flush_en		= is_hw_dma_set_auto_flush_en,
	.dma_set_size			= is_hw_dma_set_size,
	.dma_set_img_stride		= is_hw_dma_set_img_stride,
	.dma_set_header_stride		= is_hw_dma_set_header_stride,
	.dma_set_max_mo			= is_hw_dma_set_max_mo,
	.dma_set_line_gap		= is_hw_dma_set_line_gap,
	.dma_set_max_bl			= is_hw_dma_set_max_bl,
	.dma_set_bus_info		= is_hw_dma_set_bus_info,
	.dma_set_img_addr		= is_hw_dma_set_img_addr,
	.dma_set_header_addr		= is_hw_dma_set_header_addr,
	.dma_votf_enable		= is_hw_dma_votf_enable,
	.dma_enable			= is_hw_dma_enable,
	.dma_get_enable			= is_hw_dma_get_enable,
	.dma_get_max_mo			= is_hw_dma_get_max_mo,
	.dma_get_img_crc		= is_hw_dma_get_img_crc,
	.dma_get_header_crc		= is_hw_dma_get_header_crc,
	.dma_get_mon_status		= is_hw_dma_get_mon_status,
	.dma_print_info			= is_hw_dma_print_info,
	.dma_get_comp_sbwc_type		= is_hw_dma_get_comp_sbwc_type,
};

int is_hw_dma_set_ops(struct is_common_dma *dma)
{
	dma->ops = &is_hw_dma_ops;

	return DMA_OPS_SUCCESS;
}
EXPORT_SYMBOL_GPL(is_hw_dma_set_ops);

static void __iomem *__pmio_dma_get_base_addr(struct is_common_dma *dma)
{
	return dma->base;
}

static int pmio_dma_set_format(struct is_common_dma *dma, u32 format, u32 format_type)
{
	int ret = DMA_OPS_SUCCESS;
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);
	u32 val;

	val = PMIO_GET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_DATA_FORMAT].sfr_offset);

	switch (format_type) {
	case DMA_FMT_BAYER:
		if (test_bit(format, &dma->available_bayer_format_map)) {
			val = SET_V(val, DMA_F_DATA_FORMAT_BAYER, format);
			PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_DATA_FORMAT].sfr_offset, val);
		} else {
			ret = DMA_OPS_ERROR;
		}
		break;
	case DMA_FMT_YUV:
		if (test_bit(format, &dma->available_yuv_format_map)) {
			val = SET_V(val, DMA_F_DATA_FORMAT_YUV, format);
			val = SET_V(val, DMA_F_DATA_FORMAT_TYPE, format_type);
			PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_DATA_FORMAT].sfr_offset, val);
		} else
			ret = DMA_OPS_ERROR;
		break;
	case DMA_FMT_RGB:
		if (test_bit(format, &dma->available_rgb_format_map)) {
			val = SET_V(val, DMA_F_DATA_FORMAT_RGB, format);
			val = SET_V(val, DMA_F_DATA_FORMAT_TYPE, format_type);
			PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_DATA_FORMAT].sfr_offset, val);
		} else
			ret = DMA_OPS_ERROR;
		break;
	default:
		err_hw("[CMN DMA] invalid format_type[%d]", format_type);
		return -EINVAL;
	}

	return ret;
}

static int pmio_dma_set_msb_align(struct is_common_dma *dma, u32 sign, u32 msb_align)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	u32 val;
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	if (!sign) {
		err_hw("[CMN DMA] Not supported unsign");
		return -EINVAL;
	}

	val = PMIO_GET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_DATA_FORMAT].sfr_offset);
	val = SET_V(val, DMA_F_DATA_FORMAT_MSBALIGN, msb_align);
	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_DATA_FORMAT].sfr_offset, val);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_comp_error(struct is_common_dma *dma, u32 mode, u32 value)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_COMP_ERROR_MODE].sfr_offset, mode);
	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_COMP_ERROR_VALUE].sfr_offset, value);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_comp_rate(struct is_common_dma *dma, u32 rate)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_COMP_LOSSY_BYTE32NUM].sfr_offset, rate);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_auto_flush_en(struct is_common_dma *dma, u32 en)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_AUTO_FLUSH_EN].sfr_offset, en);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_size(struct is_common_dma *dma, u32 width, u32 height)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_WIDTH].sfr_offset, width);
	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_HEIGHT].sfr_offset, height);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_img_stride(struct is_common_dma *dma, u32 stride_1p, u32 stride_2p, u32 stride_3p)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_IMG_STRIDE_1P].sfr_offset, stride_1p);
	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_IMG_STRIDE_2P].sfr_offset, stride_2p);
	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_IMG_STRIDE_3P].sfr_offset, stride_3p);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_header_stride(struct is_common_dma *dma, u32 stride_1p, u32 stride_2p)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_HEADER_STRIDE_1P].sfr_offset, stride_1p);
	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_HEADER_STRIDE_2P].sfr_offset, stride_2p);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_max_mo(struct is_common_dma *dma, u32 max_mo)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_MAX_MO].sfr_offset, max_mo);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_line_gap(struct is_common_dma *dma, u32 line_gap)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_LINE_GAP].sfr_offset, line_gap);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_img_addr(struct is_common_dma *dma, dma_addr_t *addr, u32 plane, int buf_idx, u32 num_buffers)
{
	int i = 0;
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	do {
		PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_IMG_BASE_ADDR_1P_FRO_00].sfr_offset +
				(plane * IS_MAX_FRO) * 8 + (buf_idx + i) * 4, (u32)addr[i]);
	} while (++i < num_buffers);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_header_addr(struct is_common_dma *dma, dma_addr_t *addr,
	u32 plane, int buf_idx, u32 num_buffers)
{
	int i = 0;
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	do {
		PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_HEADER_BASE_ADDR_1P_FRO_00].sfr_offset +
				(plane * IS_MAX_FRO) * 8 + (buf_idx + i) * 4, (u32)addr[i]);
	} while (++i < num_buffers);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_votf_enable(struct is_common_dma *dma, u32 enable, u32 stall_enable)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_VOTF_EN].sfr_offset, enable);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_enable(struct is_common_dma *dma, u32 enable)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_EN].sfr_offset, enable);
	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_comp_sbwc_en(struct is_common_dma *dma, u32 comp_sbwc_en)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	u32 val;
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	val = PMIO_GET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_COMP_CTRL].sfr_offset);
	val = SET_V(val, DMA_F_COMP_SBWC_EN, comp_sbwc_en);
	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_COMP_CTRL].sfr_offset, val);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_get_comp_sbwc_type(struct is_common_dma *dma, u32 *comp_sbwc_en)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	u32 val;
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	val = PMIO_GET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_COMP_CTRL].sfr_offset);
	*comp_sbwc_en = GET_V(val, DMA_F_COMP_SBWC_EN);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_comp_64b_align(struct is_common_dma *dma, u32 comp_64b_align)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	u32 val;
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	val = PMIO_GET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_COMP_CTRL].sfr_offset);
	val = SET_V(val, DMA_F_COMP_SBWC_64B_ALIGN, comp_64b_align);
	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_COMP_CTRL].sfr_offset, val);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_mono_mode(struct is_common_dma *dma, u32 mono_mode)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_MONO_MODE].sfr_offset, mono_mode);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_max_bl(struct is_common_dma *dma, u32 max_bl)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_MAX_BL].sfr_offset, max_bl);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_set_bus_info(struct is_common_dma *dma, u32 bus_info)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	PMIO_SET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_BUSINFO].sfr_offset, bus_info);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_get_enable(struct is_common_dma *dma, u32 *enable)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	*enable = PMIO_GET_R(base, corex_offset + dma->reg_ofs + dma_regs[DMA_R_EN].sfr_offset);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_get_max_mo(struct is_common_dma *dma, u32 *max_mo)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);

	*max_mo = PMIO_GET_R(base, dma->reg_ofs + dma_regs[DMA_R_MAX_MO].sfr_offset);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_get_img_crc(struct is_common_dma *dma, u32 *crc_1p, u32 *crc_2p, u32 *crc_3p)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);

	*crc_1p = PMIO_GET_R(base, dma->reg_ofs + dma_regs[DMA_R_IMG_CRC_1P].sfr_offset);
	*crc_2p = PMIO_GET_R(base, dma->reg_ofs + dma_regs[DMA_R_IMG_CRC_2P].sfr_offset);
	*crc_3p = PMIO_GET_R(base, dma->reg_ofs + dma_regs[DMA_R_IMG_CRC_3P].sfr_offset);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_get_header_crc(struct is_common_dma *dma, u32 *crc_1p, u32 *crc_2p)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);

	*crc_1p = PMIO_GET_R(base, dma->reg_ofs + dma_regs[DMA_R_HEADER_CRC_1P].sfr_offset);
	*crc_2p = PMIO_GET_R(base, dma->reg_ofs + dma_regs[DMA_R_HEADER_CRC_2P].sfr_offset);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_get_mon_status(struct is_common_dma *dma, u32 *mon_status)
{
	void __iomem *base = __pmio_dma_get_base_addr(dma);

	*mon_status = PMIO_GET_R(base, dma->reg_ofs + dma_regs[DMA_R_MON_STATUS_0].sfr_offset);

	return DMA_OPS_SUCCESS;
}

static int pmio_dma_print_info(struct is_common_dma *dma, u32 option)
{
	u32 enable = 0;
	u32 i;
	u32 reg_value;
	void __iomem *base = __pmio_dma_get_base_addr(dma);

	CALL_DMA_OPS(dma, dma_get_enable, &enable);

	if (!enable) {
		info(" %s disabled", dma->name);
		return 0;
	}

	info(" %s DUMP!!", dma->name);
	for(i = 0; i < DMA_REG_CNT; i++) {
		reg_value = PMIO_GET_R(base, dma->reg_ofs + dma_regs[i].sfr_offset);
		sfrinfo("[DUMP] reg:[%s][0x%04X], value:[0x%08X]\n",
			dma_regs[i].reg_name, dma_regs[i].sfr_offset, reg_value);
	}

	return DMA_OPS_SUCCESS;
}

int pmio_dma_create(struct is_common_dma *dma, void __iomem *base, int id,
			const char *name, ulong available_bayer_format_map,
			ulong available_yuv_format_map,
			ulong available_rgb_format_map)
{
	dma->base = base;
	dma->id = id;

	strncpy(dma->name, name, (sizeof(dma->name) - 1));
	dma->available_bayer_format_map = available_bayer_format_map;
	dma->available_yuv_format_map = available_yuv_format_map;
	dma->available_rgb_format_map = available_rgb_format_map;
	return DMA_OPS_SUCCESS;
}
EXPORT_SYMBOL_GPL(pmio_dma_create);

static int pmio_dma_set_corex_id(struct is_common_dma *dma, int set_id)
{
	dma->set_id = set_id;
	return DMA_OPS_SUCCESS;
}

const struct is_common_dma_ops pmio_dma_ops = {
	.dma_set_corex_id	= pmio_dma_set_corex_id,
	.dma_set_format		= pmio_dma_set_format,
	.dma_set_msb_align	= pmio_dma_set_msb_align,
	.dma_set_comp_sbwc_en	= pmio_dma_set_comp_sbwc_en,
	.dma_set_comp_64b_align	= pmio_dma_set_comp_64b_align,
	.dma_set_comp_error	= pmio_dma_set_comp_error,
	.dma_set_comp_rate	= pmio_dma_set_comp_rate,
	.dma_set_mono_mode	= pmio_dma_set_mono_mode,
	.dma_set_auto_flush_en	= pmio_dma_set_auto_flush_en,
	.dma_set_size		= pmio_dma_set_size,
	.dma_set_img_stride	= pmio_dma_set_img_stride,
	.dma_set_header_stride	= pmio_dma_set_header_stride,
	.dma_set_max_mo		= pmio_dma_set_max_mo,
	.dma_set_line_gap	= pmio_dma_set_line_gap,
	.dma_set_max_bl		= pmio_dma_set_max_bl,
	.dma_set_bus_info	= pmio_dma_set_bus_info,
	.dma_set_img_addr	= pmio_dma_set_img_addr,
	.dma_set_header_addr	= pmio_dma_set_header_addr,
	.dma_votf_enable	= pmio_dma_votf_enable,
	.dma_enable		= pmio_dma_enable,
	.dma_get_enable		= pmio_dma_get_enable,
	.dma_get_max_mo		= pmio_dma_get_max_mo,
	.dma_get_img_crc	= pmio_dma_get_img_crc,
	.dma_get_header_crc	= pmio_dma_get_header_crc,
	.dma_get_mon_status	= pmio_dma_get_mon_status,
	.dma_print_info		= pmio_dma_print_info,
	.dma_get_comp_sbwc_type	= pmio_dma_get_comp_sbwc_type,
};

int pmio_dma_set_ops(struct is_common_dma *dma)
{
	dma->ops = &pmio_dma_ops;

	return DMA_OPS_SUCCESS;
}
EXPORT_SYMBOL_GPL(pmio_dma_set_ops);

int is_hw_dma_get_comp_sbwc_en(u32 sbwc_type, u32 *comp_64b_align)
{
	u32 comp_sbwc_en = 0;

	switch (sbwc_type & DMA_INPUT_SBWC_MASK) {
	case DMA_INPUT_SBWC_DISABLE:
		comp_sbwc_en = 0;
		*comp_64b_align = 0;
		break;
	case DMA_INPUT_SBWC_LOSSYLESS_32B:
		comp_sbwc_en = 1;
		*comp_64b_align = 0;
		break;
	case DMA_INPUT_SBWC_LOSSYLESS_64B:
		comp_sbwc_en = 1;
		*comp_64b_align = 1;
		break;
	case DMA_INPUT_SBWC_LOSSY_32B:
		comp_sbwc_en = 2;
		*comp_64b_align = 0;
		break;
	case DMA_INPUT_SBWC_LOSSY_64B:
		comp_sbwc_en = 2;
		*comp_64b_align = 1;
		break;
	default:
		err_hw("invalid sbwc_type(%d)", sbwc_type);
		return -EINVAL;
	}

	return comp_sbwc_en;
}
EXPORT_SYMBOL_GPL(is_hw_dma_get_comp_sbwc_en);

int is_hw_dma_get_bayer_format(u32 memory_bitwidth, u32 pixel_size, u32 hw_format,
			       u32 comp_sbwc_en, bool is_msb, u32 *fmt)
{
	bool is_pack = false;
	u32 pack_type;

	if (hw_format == DMA_INOUT_FORMAT_DRCGAIN
		|| hw_format == DMA_INOUT_FORMAT_SVHIST
		|| hw_format == DMA_INOUT_FORMAT_SEGCONF) {
		*fmt = DMA_FMT_U8BIT_PACK;
		return 0;
	}

	*fmt = DMA_FMT_BAYER_MAX;
	if (comp_sbwc_en) {
		if (pixel_size >= ARRAY_SIZE(bayer_sbwc_on_table)) {
			pr_err("unknown pixel_size(%d) for bayer_sbwc_on_table\n", pixel_size);
			return -EINVAL;
		}
		*fmt = bayer_sbwc_on_table[pixel_size];
	} else {
		if (pixel_size >= ARRAY_SIZE(bayer_sbwc_off_table)) {
			pr_err("unknown pixel_size(%d) for bayer_sbwc_off_table\n", pixel_size);
			return -EINVAL;
		}

		is_pack = (hw_format == DMA_INOUT_FORMAT_BAYER_PACKED ||
			hw_format == DMA_INOUT_FORMAT_YUV422_PACKED) ? true : false;
		is_pack |= (pixel_size == memory_bitwidth) ? true : false;

		if (!is_pack && memory_bitwidth != 16) {
			err_hw("invalid pixel_size(%d), memory_bit_width(%d)",
				pixel_size, memory_bitwidth);
			return -EINVAL;
		}

		if (is_pack)
			pack_type = BAYER_PACK;
		else
			pack_type = (is_msb) ? BAYER_UNPACK_MSB : BAYER_UNPACK_LSB;

		*fmt = bayer_sbwc_off_table[pixel_size][pack_type];
	}
	if (*fmt == DMA_FMT_BAYER_MAX) {
		err_hw("invalid pixel_size(%d)", pixel_size);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(is_hw_dma_get_bayer_format);

int is_hw_dma_get_rgb_format(u32 bitwidth, u32 plane, u32 order)
{
	int ret = 0;

	switch (bitwidth) {
	/* bitwidth : 32 bit */
	case DMA_INOUT_BIT_WIDTH_32BIT:
		switch (order) {
		case DMA_INOUT_ORDER_ARGB:
			ret = DMA_FMT_RGB_ARGB8888;
			break;
		case DMA_INOUT_ORDER_BGRA:
			ret = DMA_FMT_RGB_BGRA8888;
			break;
		case DMA_INOUT_ORDER_RGBA:
			ret = DMA_FMT_RGB_RGBA8888;
			break;
		case DMA_INOUT_ORDER_ABGR:
			ret = DMA_FMT_RGB_ABGR8888;
			break;
		default:
			ret = DMA_FMT_RGB_RGBA8888;
			break;
		}
		break;
	/* bitwidth : 10 bit */
	case DMA_INOUT_BIT_WIDTH_10BIT:
		switch (order) {
		case DMA_INOUT_ORDER_ARGB:
			ret = DMA_FMT_RGB_ARGB1010102;
			break;
		case DMA_INOUT_ORDER_BGRA:
			ret = DMA_FMT_RGB_BGRA1010102;
			break;
		case DMA_INOUT_ORDER_RGBA:
			ret = DMA_FMT_RGB_RGBA1010102;
			break;
		case DMA_INOUT_ORDER_ABGR:
			ret = DMA_FMT_RGB_ABGR1010102;
			break;
		default:
			err_hw("output order error - (%d/%d)", order, plane);
			return -EINVAL;
		}
		break;

	default:
		err_hw("output bitwidth error - (%d/%d)", order, plane);
		return -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(is_hw_dma_get_rgb_format);

int is_hw_dma_get_yuv_format(u32 bitwidth, u32 format, u32 plane, u32 order)
{
	int ret;

	switch (bitwidth) {
	/* bitwidth : 8 bit */
	case DMA_INOUT_BIT_WIDTH_8BIT:
		switch (format) {
		case DMA_INOUT_FORMAT_YUV420:
			if (plane == 3) {
				ret = DMA_FMT_YUV420_3P;
				break;
			}
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				ret = DMA_FMT_YUV420_2P_UFIRST;
				break;
			case DMA_INOUT_ORDER_CrCb:
				ret = DMA_FMT_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				return -EINVAL;
			}
			break;
		case DMA_INOUT_FORMAT_YUV422:
			switch (order) {
			case DMA_INOUT_ORDER_CrYCbY:
				ret = (plane == 1) ? DMA_FMT_YUV422_1P_VYUY : -EINVAL;
				break;
			case DMA_INOUT_ORDER_CbYCrY:
				ret = (plane == 1) ? DMA_FMT_YUV422_1P_UYVY : -EINVAL;
				break;
			case DMA_INOUT_ORDER_YCrYCb:
				ret = (plane == 1) ? DMA_FMT_YUV422_1P_YVYU : -EINVAL;
				break;
			case DMA_INOUT_ORDER_YCbYCr:
				ret = (plane == 1) ? DMA_FMT_YUV422_1P_YUYV : -EINVAL;
				break;
			case DMA_INOUT_ORDER_CbCr:
				ret = (plane == 2) ? DMA_FMT_YUV422_2P_UFIRST : -EINVAL;
				break;
			case DMA_INOUT_ORDER_CrCb:
				ret = (plane == 2) ? DMA_FMT_YUV422_2P_VFIRST : -EINVAL;
				break;
			default:
				ret = (plane == 3) ? DMA_FMT_YUV422_3P : -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_YUV444:
			switch (plane) {
			case 1:
				ret = DMA_FMT_YUV444_1P;
				break;
			case 3:
				ret = DMA_FMT_YUV444_3P;
				break;
			default:
				err_hw("img plane error - (%d/%d/%d)", format, order, plane);
				return -EINVAL;
			}
			break;
		case DMA_INOUT_FORMAT_Y:
			/* 8bit y only format is handled as YUV420 2p format */
			ret = DMA_FMT_YUV420_2P_UFIRST;
			break;
		case DMA_INOUT_FORMAT_RGB:
			/* 8bit RGB planar format is handled as YUV444 3p format */
			ret = DMA_FMT_YUV444_3P;
			break;
		default:
			err_hw("output format error - (%d/%d/%d)", format, order, plane);
			return -EINVAL;
		}
		break;
	/* bitwidth : 10 bit */
	case DMA_INOUT_BIT_WIDTH_10BIT:
		switch (format) {
		case DMA_INOUT_FORMAT_YUV420:
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				ret = (plane == 2) ? DMA_FMT_YUV420_2P_UFIRST_PACKED10 : -EINVAL;
				ret = (plane == 4) ? DMA_FMT_YUV420_2P_UFIRST_8P2 : ret;
				break;
			case DMA_INOUT_ORDER_CrCb:
				ret = (plane == 2) ? DMA_FMT_YUV420_2P_VFIRST_PACKED10 : -EINVAL;
				ret = (plane == 4) ? DMA_FMT_YUV420_2P_VFIRST_8P2 : ret;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				return -EINVAL;
			}
			break;
		case DMA_INOUT_FORMAT_YUV422:
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				ret = (plane == 2) ? DMA_FMT_YUV422_2P_UFIRST_PACKED10 : -EINVAL;
				ret = (plane == 4) ? DMA_FMT_YUV422_2P_UFIRST_8P2 : ret;
				break;
			case DMA_INOUT_ORDER_CrCb:
				ret = (plane == 2) ? DMA_FMT_YUV422_2P_VFIRST_PACKED10 : -EINVAL;
				ret = (plane == 4) ? DMA_FMT_YUV422_2P_VFIRST_8P2 : ret;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				return -EINVAL;
			}
			break;
		case DMA_INOUT_FORMAT_YUV444:
			ret = (plane == 1) ? DMA_FMT_YUV444_1P_PACKED10 : -EINVAL;
			ret = (plane == 3) ? DMA_FMT_YUV444_3P_PACKED10 : ret;
			break;
		default:
			err_hw("img format error - (%d/%d/%d)", format, order, plane);
			return -EINVAL;
		}
		break;
	/* bitwidth : 16 bit */
	case DMA_INOUT_BIT_WIDTH_16BIT:
		switch (format) {
		case DMA_INOUT_FORMAT_YUV420:
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				ret = DMA_FMT_YUV420_2P_UFIRST_P010;
				break;
			case DMA_INOUT_ORDER_CrCb:
				ret = DMA_FMT_YUV420_2P_VFIRST_P010;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				return -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_YUV422:
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				ret = DMA_FMT_YUV422_2P_UFIRST_P210;
				break;
			case DMA_INOUT_ORDER_CrCb:
				ret = DMA_FMT_YUV422_2P_VFIRST_P210;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				return -EINVAL;
			}
			break;
		case DMA_INOUT_FORMAT_YUV444:
			ret = (plane == 1) ? DMA_FMT_YUV444_1P_UNPACKED : -EINVAL;
			ret = (plane == 3) ? DMA_FMT_YUV444_3P_UNPACKED : ret;
			break;
		default:
			err_hw("output format error - (%d/%d/%d)", format, order, plane);
			return -EINVAL;
		}
		break;
	case DMA_INOUT_BIT_WIDTH_24BIT:
		switch (format) {
		case DMA_INOUT_FORMAT_RGB:
			/* 8bit RGB interleaved format is handled as YUV444 1p format */
			ret = DMA_FMT_YUV444_1P;
			break;
		default:
			err_hw("output format error - (%d/%d/%d)", format, order, plane);
			return -EINVAL;
		}
		break;
	default:
		err_hw("output bitwidth error - (%d/%d/%d)", format, order, plane);
		return -EINVAL;
	}
	if (ret == -EINVAL)
		err_hw("get yuv format error - (%d/%d/%d)", format, order, plane);

	return ret;
}
EXPORT_SYMBOL_GPL(is_hw_dma_get_yuv_format);

u32 is_hw_dma_get_img_stride(u32 memory_bitwidth, u32 pixel_size, u32 hw_format,
		u32 width, u32 align, bool is_image)
{
	u32 img_stride = 0;
	bool is_pack = false;

	is_pack = (hw_format == DMA_INOUT_FORMAT_BAYER_PACKED || hw_format == DMA_INOUT_FORMAT_YUV422_PACKED) ? true : false;
	is_pack |= (pixel_size == memory_bitwidth) ? true : false;

	if (is_image && align) {
		if (is_pack)
			img_stride = ALIGN(DIV_ROUND_UP(width * pixel_size, BITS_PER_BYTE), align);
		else
			img_stride = ALIGN(width * 2, align);
	} else
		img_stride = width;

	return img_stride;
}
EXPORT_SYMBOL_GPL(is_hw_dma_get_img_stride);

u32 is_hw_dma_get_payload_stride(u32 comp_sbwc_en, u32 bit_depth, u32 width,
	u32 comp_64b_align, u32 lossy_byte32num,
	u32 block_width, u32 block_height)
{
	u32 payload_stride = 0;

	if (comp_sbwc_en == COMP) {
		/* lossless */
		/* 256x1 */
		if (block_width == 256 && block_height == 1) {
			payload_stride = (((width + 255) / 256) * (256 * 1 * ALIGN(bit_depth, 2))) / 8;
		} else if (block_width == 32 && block_height == 4) {
			/* 32x4 */
			switch (comp_64b_align) {
			case 0:
				/* 32 byte align*/
				payload_stride = (((width + 31) / 32) * (32 * 4 * ALIGN(bit_depth, 2))) / 8;
				break;
			case 1:
				/* 64 byte align*/
				payload_stride = (((width + 31) / 32) * 32 * 4 *
					(ALIGN(bit_depth, 2) + ((16 * ALIGN(bit_depth, 2)) % 64) / 16)) / 8;
				break;
			default:
				err_hw("sbwc lossless(32x4) invalid comp_64b_align - (%d)", comp_64b_align);
				break;
			}
		} else {
			err_hw("Invalid sbwc lossless block width(%d) height(%d)", block_width, block_height);
		}
	} else if (comp_sbwc_en == COMP_LOSS) {
		/* lossy */
		if (block_width == 256 && block_height == 1) {
			/* 256x1 */
			switch (comp_64b_align) {
			case 0:
				/* 32 byte align*/
				payload_stride = (width / block_width) *
					(lossy_byte32num * 32) +
					(((width % block_width) * bit_depth) / 8);
				payload_stride = ALIGN(payload_stride, 32);
				break;
			case 1:
				/* 64 byte align*/
				payload_stride = (width / block_width) *
					(ALIGN(lossy_byte32num, 2) * 32) +
					(((width % block_width) * bit_depth) / 8);
				payload_stride = ALIGN(payload_stride, 64);
				break;
			default:
				err_hw("sbwc lossy(256x1) invalid comp_64b_align - (%d)", comp_64b_align);
				break;
			}
		} else if (block_width == 32 && block_height == 4) {
			/* 32x4 */
			switch (comp_64b_align) {
			case 0:
				/* 32 byte align*/
				payload_stride = DIV_ROUND_UP(width, block_width) * (lossy_byte32num * block_width);
				payload_stride = ALIGN(payload_stride, 32);
				break;
			case 1:
				/* 64 byte align*/
				payload_stride = DIV_ROUND_UP(width, block_width) * ((block_width * lossy_byte32num)
					+ (block_width * (lossy_byte32num % 2)));
				payload_stride = ALIGN(payload_stride, 64);
				break;
			default:
				err_hw("sbwc lossy(32x4) invalid comp_64b_align - (%d)", comp_64b_align);
				break;
			}
		} else {
			err_hw("Invalid sbwc lossy block width(%d) height(%d)", block_width, block_height);
		}
	} else {
		/* empty */
	}

	return payload_stride;
}
EXPORT_SYMBOL_GPL(is_hw_dma_get_payload_stride);

u32 is_hw_dma_get_header_stride(u32 width, u32 block_width, u32 align)
{
	u32 header_stride = 0;

	if (block_width == 0) {
		err_hw("Invalid sbwc block width(%d), can't be divided by 0", block_width);
		return 0;
	}

	header_stride = (((width + block_width - 1) / block_width) * 4 + 7) / 8;
	if (align)
		header_stride = ALIGN(header_stride, align);

	return header_stride;
}
EXPORT_SYMBOL_GPL(is_hw_dma_get_header_stride);
