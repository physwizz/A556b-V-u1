/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef IS_HW_COMMON_DMA_H
#define IS_HW_COMMON_DMA_H

#include "is-hw.h"

#define IS_MAX_COREX_SET 4
#define IS_MAX_FRO 8
#define IS_MAX_FRO_LSB IS_MAX_FRO

#define IS_MAX_STR_LEN 20

#define IS_BAYER_FORMAT_MASK 0x000000007777FFF7
#define IS_YUV_FORMAT_MASK   0x003F3FF33FF303FF
#define IS_RGB_FORMAT_MASK   0x0000000000000F1E

#define LSB_4BITS_MASK 0x0000000FUL
#define MSB_32BITS_MASK 0xFFFFFFFF0UL
#define LSB_BIT 4
#define DVA_36BIT_HIGH(dva) (((dva) & MSB_32BITS_MASK) >> LSB_BIT)
#define DVA_36BIT_LOW(dva) ((dva) & LSB_4BITS_MASK)

#define SBWC_ALIGN_TYPE_32B		0
#define SBWC_ALIGN_TYPE_64B		1
#define SBWC_ALIGN_TYPE_128B		2

#define CALL_DMA_OPS(dma, op, args...)	\
	(((dma)->ops && (dma)->ops->op) ? ((dma)->ops->op(dma, args)) : 0)

#define GET_COREX_OFFSET(SET_ID) \
	((SET_ID <= COREX_SET_D) ? (0x8000 + ((SET_ID) * 0x10000)) : 0)

#define GET_LME_COREX_OFFSET(SET_ID) \
	((SET_ID <= COREX_SET_D) ? (0x0 + ((SET_ID) * 0x2000)) : LME_DIRECT_BASE)

#define dbg_dma(level, fmt, args...) \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_HW)) >= (level), \
		"[dma]", fmt, ##args)

enum bayer_format {
	DMA_FMT_U8BIT_PACK = 0,
	DMA_FMT_U8BIT_UNPACK_LSB_ZERO,
	DMA_FMT_U8BIT_UNPACK_MSB_ZERO,
	DMA_FMT_U10BIT_PACK	= 4,
	DMA_FMT_U10BIT_UNPACK_LSB_ZERO,
	DMA_FMT_U10BIT_UNPACK_MSB_ZERO,
	DMA_FMT_ANDROID10,
	DMA_FMT_U12BIT_PACK,
	DMA_FMT_U12BIT_UNPACK_LSB_ZERO,
	DMA_FMT_U12BIT_UNPACK_MSB_ZERO,
	DMA_FMT_ANDROID12,
	DMA_FMT_U14BIT_PACK,
	DMA_FMT_U14BIT_UNPACK_LSB_ZERO,
	DMA_FMT_U14BIT_UNPACK_MSB_ZERO,
	DMA_FMT_U16BIT_PACK,
	DMA_FMT_S8BIT_PACK = 16,
	DMA_FMT_S8BIT_UNPACK_LSB_ZERO,
	DMA_FMT_S8BIT_UNPACK_MSB_ZERO,
	DMA_FMT_S10BIT_PACK	= 20,
	DMA_FMT_S10BIT_UNPACK_LSB_ZERO,
	DMA_FMT_S10BIT_UNPACK_MSB_ZERO,
	DMA_FMT_S12BIT_PACK = 24,
	DMA_FMT_S12BIT_UNPACK_LSB_ZERO,
	DMA_FMT_S12BIT_UNPACK_MSB_ZERO,
	DMA_FMT_S14BIT_PACK = 28,
	DMA_FMT_S14BIT_UNPACK_LSB_ZERO,
	DMA_FMT_S14BIT_UNPACK_MSB_ZERO,
	DMA_FMT_BAYER_MAX,
};

enum yuv_format {
	DMA_FMT_YUV422_1P_YUYV = 0,
	DMA_FMT_YUV422_1P_YVYU,
	DMA_FMT_YUV422_1P_UYVY,
	DMA_FMT_YUV422_1P_VYUY,
	DMA_FMT_YUV422_2P_UFIRST,
	DMA_FMT_YUV422_2P_VFIRST,
	DMA_FMT_YUV422_3P,
	DMA_FMT_YUV420_2P_UFIRST,
	DMA_FMT_YUV420_2P_VFIRST,
	DMA_FMT_YUV420_3P,
	DMA_FMT_YUV422_2P_UFIRST_P210 = 16,
	DMA_FMT_YUV422_2P_VFIRST_P210,
	DMA_FMT_YUV422_2P_UFIRST_8P2,
	DMA_FMT_YUV422_2P_VFIRST_8P2,
	DMA_FMT_YUV422_2P_UFIRST_PACKED10,
	DMA_FMT_YUV422_2P_VFIRST_PACKED10,
	DMA_FMT_YUV420_2P_UFIRST_P010 = 32,
	DMA_FMT_YUV420_2P_VFIRST_P010,
	DMA_FMT_YUV420_2P_UFIRST_8P2,
	DMA_FMT_YUV420_2P_VFIRST_8P2,
	DMA_FMT_YUV420_2P_UFIRST_PACKED10,
	DMA_FMT_YUV420_2P_VFIRST_PACKED10,
	DMA_FMT_YUV444_1P = 48,
	DMA_FMT_YUV444_3P,
	DMA_FMT_YUV444_1P_PACKED10,
	DMA_FMT_YUV444_3P_PACKED10,
	DMA_FMT_YUV444_1P_UNPACKED,
	DMA_FMT_YUV444_3P_UNPACKED,
};

enum rgb_format {
	DMA_FMT_RGB_RGBA8888 = 1,
	DMA_FMT_RGB_ABGR8888,
	DMA_FMT_RGB_ARGB8888,
	DMA_FMT_RGB_BGRA8888,
	DMA_FMT_RGB_RGBA1010102 = 8,
	DMA_FMT_RGB_ABGR1010102,
	DMA_FMT_RGB_ARGB1010102,
	DMA_FMT_RGB_BGRA1010102,
};

enum format_type {
	DMA_FMT_YUV,
	DMA_FMT_RGB,
	DMA_FMT_BAYER
};

enum bayer_sbwc_off_format {
	BAYER_PACK,
	BAYER_UNPACK_LSB,
	BAYER_UNPACK_MSB,
	BAYER_SBWC_OFF
};

enum dma_ops_state {
	DMA_OPS_SUCCESS,
	DMA_OPS_ERROR,
};

enum comp_error_mode {
	COMP_ERROR_MODE_USE_0,			/* Use 0 */
	COMP_ERROR_MODE_USE_VALUE,		/* Use value set in ERROR_VALUE register */
	COMP_ERROR_MODE_USE_LAST_PIXEL,	/* Use last pixel value of sbwc block*/
};

enum lossy_comp_quality_type {
	LOSSY_COMP_QUALITY = 0,
	LOSSY_COMP_BANDWIDTH = 1,
	LOSSY_COMP_FOOTPRINT = 2,
	LOSSY_COMP_QUALITY_TYPE_MAX
};

static const u32 bayer_sbwc_on_table[] = {
	[0 ... 7] = DMA_FMT_BAYER_MAX,
	[8] = DMA_FMT_U8BIT_PACK, /* DMA_INOUT_BIT_WIDTH_8BIT */
	[9] = DMA_FMT_S8BIT_PACK,
	[10] = DMA_FMT_U10BIT_PACK,
	[11] = DMA_FMT_S10BIT_PACK,
	[12] = DMA_FMT_U12BIT_PACK,
	[13] = DMA_FMT_S12BIT_PACK,
	[14] = DMA_FMT_U14BIT_PACK,
	[15] = DMA_FMT_S14BIT_PACK, /* DMA_INOUT_BIT_WIDTH_15BIT */
};

static const u32 bayer_sbwc_off_table[][BAYER_SBWC_OFF] = {
	[0 ... 7] = { DMA_FMT_BAYER_MAX, DMA_FMT_BAYER_MAX, DMA_FMT_BAYER_MAX },
	[8] = { DMA_FMT_U8BIT_PACK,DMA_FMT_U8BIT_UNPACK_LSB_ZERO, DMA_FMT_U8BIT_UNPACK_MSB_ZERO },
	[9] = { DMA_FMT_S8BIT_PACK, DMA_FMT_S8BIT_UNPACK_LSB_ZERO, DMA_FMT_S8BIT_UNPACK_MSB_ZERO },
	[10] = { DMA_FMT_U10BIT_PACK, DMA_FMT_U10BIT_UNPACK_LSB_ZERO, DMA_FMT_U10BIT_UNPACK_MSB_ZERO },
	[11] = { DMA_FMT_S10BIT_PACK, DMA_FMT_S10BIT_UNPACK_LSB_ZERO, DMA_FMT_S10BIT_UNPACK_MSB_ZERO },
	[12] = { DMA_FMT_U12BIT_PACK, DMA_FMT_U12BIT_UNPACK_LSB_ZERO, DMA_FMT_U12BIT_UNPACK_MSB_ZERO },
	[13] = { DMA_FMT_S12BIT_PACK, DMA_FMT_S12BIT_UNPACK_LSB_ZERO, DMA_FMT_S12BIT_UNPACK_MSB_ZERO },
	[14] = { DMA_FMT_U14BIT_PACK, DMA_FMT_U14BIT_UNPACK_LSB_ZERO, DMA_FMT_U14BIT_UNPACK_MSB_ZERO },
	[15] = { DMA_FMT_S14BIT_PACK, DMA_FMT_S14BIT_UNPACK_LSB_ZERO, DMA_FMT_S14BIT_UNPACK_MSB_ZERO },
};

struct is_common_dma {
	void __iomem *base;
	unsigned int reg_ofs;
	unsigned int field_ofs;
	void __iomem *corex_base[IS_MAX_COREX_SET];
	char name[IS_MAX_STR_LEN];
	ulong available_bayer_format_map;
	ulong available_yuv_format_map;
	ulong available_rgb_format_map;
	int id;
	int set_id;
	const struct is_common_dma_ops *ops;
};

struct is_common_dma_ops {
	int (*dma_set_corex_id)(struct is_common_dma *dma, int set_id);
	int (*dma_set_format)(struct is_common_dma *dma, u32 format, u32 format_type);
	int (*dma_set_msb_align)(struct is_common_dma *dma, u32 sign, u32 msb_align);
	int (*dma_set_comp_sbwc_en)(struct is_common_dma *dma, u32 comp_sbwc_en);
	int (*dma_set_comp_64b_align)(struct is_common_dma *dma, u32 comp_64b_align);
	int (*dma_set_comp_error)(struct is_common_dma *dma, u32 mode, u32 value);
	int (*dma_set_comp_rate)(struct is_common_dma *dma, u32 rate);
	int (*dma_set_comp_quality)(struct is_common_dma *dma,
		enum lossy_comp_quality_type quality_control);
	int (*dma_set_cache_32b_pa)(struct is_common_dma *dma, u32 en);
	int (*dma_set_mono_mode)(struct is_common_dma *dma, u32 mono_mode);
	int (*dma_set_auto_flush_en)(struct is_common_dma *dma, u32 en);
	int (*dma_set_size)(struct is_common_dma *dma, u32 width, u32 height);
	int (*dma_set_img_stride)(struct is_common_dma *dma,
		u32 stride_1p, u32 stride_2p, u32 stride_3p);
	int (*dma_set_header_stride)(struct is_common_dma *dma,
		u32 stride_1p, u32 stride_2p);
	int (*dma_set_max_mo)(struct is_common_dma *dma, u32 max_mo);
	int (*dma_set_line_gap)(struct is_common_dma *dma, u32 line_gap);
	int (*dma_set_max_bl)(struct is_common_dma *dma, u32 max_bl);
	int (*dma_set_bus_info)(struct is_common_dma *dma, u32 bus_info);
	int (*dma_set_img_addr)(struct is_common_dma *dma, dma_addr_t *addr,
		u32 plane, int buf_idx, u32 num_buffers);
	int (*dma_set_header_addr)(struct is_common_dma *dma, dma_addr_t *addr,
		u32 plane, int buf_idx, u32 num_buffers);
	int (*dma_votf_enable)(struct is_common_dma *dma, u32 enable, u32 stall_enable);
	int (*dma_enable)(struct is_common_dma *dma, u32 enable);
	int (*dma_get_enable)(struct is_common_dma *dma, u32 *enable);
	int (*dma_get_max_mo)(struct is_common_dma *dma, u32 *max_mo);
	int (*dma_get_img_crc)(struct is_common_dma *dma,
		u32 *crc_1p, u32 *crc_2p, u32 *crc_3p);
	int (*dma_get_header_crc)(struct is_common_dma *dma,
		u32 *crc_1p, u32 *crc_2p);
	int (*dma_get_mon_status)(struct is_common_dma *dma, u32 *mon_status);
	int (*dma_print_info)(struct is_common_dma *dma, u32 option);
	int (*dma_get_size)(struct is_common_dma *dma, u32 *width, u32 *height);
	int (*dma_get_comp_sbwc_type)(struct is_common_dma *dma, u32 *comp_sbwc_en);
};

int is_hw_dma_create(struct is_common_dma *dma, void __iomem *base,
			int id, const char *name,
			ulong available_bayer_format_map,
			ulong available_yuv_format_map,
			ulong available_rgb_format_map);
int is_hw_dma_set_ops(struct is_common_dma *dma);

int is_hw_dma_get_comp_sbwc_en(u32 sbwc_type, u32 *comp_64b_align);
int is_hw_dma_get_comp_sbwc_fr(u32 sbwc_type);
int is_hw_dma_get_bayer_format(u32 memory_bitwidth, u32 pixel_size, u32 hw_format,
	u32 comp_sbwc_en, bool is_msb, u32 *fmt);
int is_hw_dma_get_rgb_format(u32 bitwidth, u32 plane, u32 order);
int is_hw_dma_get_yuv_format(u32 bitwidth, u32 format, u32 plane, u32 order);
u32 is_hw_dma_get_img_stride(u32 memory_bitwidth, u32 pixel_size, u32 hw_format,
	u32 width, u32 align, bool is_image);
u32 is_hw_dma_get_payload_stride(u32 comp_sbwc_en, u32 bit_depth, u32 width, u32 comp_64b_align,
	u32 lossy_byte32num, u32 block_width, u32 block_height);
u32 is_hw_dma_get_header_stride(u32 width, u32 block_width, u32 align);

int pmio_dma_create(struct is_common_dma *dma, void __iomem *base,
			int id, const char *name,
			ulong available_bayer_format_map,
			ulong available_yuv_format_map,
			ulong available_rgb_format_map);
int pmio_dma_set_ops(struct is_common_dma *dma);
#endif
