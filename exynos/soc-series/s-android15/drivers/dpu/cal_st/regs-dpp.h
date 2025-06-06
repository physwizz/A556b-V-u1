/* SPDX-License-Identifier: GPL-2.0-only
 *
 * cal_st/regs-dpp.h
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Jaehoe Yang <jaehoe.yang@samsung.com>
 *
 * Register definition file for Samsung Display Pre-Processor driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef DPP_REGS_H_
#define DPP_REGS_H_

#define MAX_DPP_CNT		REGS_DPP_ID_MAX	/* RDMA + ODMA */
#define MAX_PORT_CNT		2		/* dma port count */
#define MAX_DPUF_CNT		1
#define DPP_PER_DPUF		8
#define MAX_SAJC_PER_DPUF	4

enum dpp_regs_id {
	REGS_DPP0_ID = 0,
	REGS_DPP1_ID,
	REGS_DPP2_ID,
	REGS_DPP3_ID,
	REGS_DPP4_ID,
	REGS_DPP5_ID,
	REGS_DPP6_ID,
	REGS_DPP7_ID,
	REGS_DPP_RCD_ID = 8, /* RCD*/
	REGS_DPP_WB_ID, /* WB */
	REGS_DPP_ID_MAX
};

enum dpp_regs_type {
	REGS_DMA = 0,
	REGS_DPP,
	REGS_SRAMC,
	REGS_VOTF,
	REGS_SCL_COEF,
	REGS_HDR_COMM,
	REGS_SFR_DMA,
	REGS_DPP_TYPE_MAX
};

/*
 *-------------------------------------------------------------------
 * RDMA(L0~L7)x2 SFR list
 * base address : 0x1689_0000 (DPU_DMA)
 * < Layer.offset >
 *  L0      L1      L2      L3      L4      L5      L6      L7
 *  0x0000  0x1000  0x2000  0x3000  0x4000  0x5000  0x6000  0x7000
 *-------------------------------------------------------------------
 */

/* SHADOW: 0x400 ~ 0x800 */
#define DMA_SHD_OFFSET				0x0400

#define RDMA_ENABLE				0x0000
#define IDMA_ASSIGNED_MO(_v)			((_v) << 24)
#define IDMA_ASSIGNED_MO_MASK			(0xff << 24)
#define IDMA_SRESET				(1 << 8)
#define IDMA_SFR_UPDATE_FORCE			(1 << 4)
#define IDMA_OP_STATUS				(1 << 2)
#define OP_STATUS_IDLE				(0)
#define OP_STATUS_BUSY				(1)
#define IDMA_INST_OFF_PEND			(1 << 1)
#define INST_OFF_PEND				(1)
#define INST_OFF_NOT_PEND			(0)

#define RDMA_IRQ				0x0004
#define IDMA_VOTF_ERR_IRQ			(1 << 27)
#define IDMA_AXI_ADDR_ERR_IRQ			(1 << 26)
#define IDMA_LB_CONFLICT_IRQ			(1 << 25)
#define IDMA_MO_CONFLICT_IRQ			(1 << 24)
#define IDMA_FBC_ERR_IRQ			(1 << 23) // SBWC ERR IRQ ??
#define IDMA_RECOVERY_TRG_IRQ			(1 << 22) // AFBC recovery mode
#define IDMA_CONFIG_ERR_IRQ			(1 << 21)
#define IDMA_INST_OFF_DONE			(1 << 20)
#define IDMA_READ_SLAVE_ERR_IRQ			(1 << 19)
#define IDMA_DEADLOCK_IRQ			(1 << 17)
#define IDMA_FRAME_DONE_IRQ			(1 << 16)
#define IDMA_ALL_IRQ_CLEAR			(0xFFB << 16)
#define IDMA_VOTF_ERR_IRQ_MASK			(1 << 12)
#define IDMA_AXI_ADDR_ERR_IRQ_MASK		(1 << 11)
#define IDMA_LB_CONFLICT_MASK			(1 << 10)
#define IDMA_MO_CONFLICT_MASK			(1 << 9)
#define IDMA_FBC_ERR_MASK			(1 << 8) // SBWC IRQ MASK ??
#define IDMA_RECOVERY_TRG_MASK			(1 << 7) // AFBC Auto recovery mode
#define IDMA_CONFIG_ERR_MASK			(1 << 6)
#define IDMA_INST_OFF_DONE_MASK			(1 << 5)
#define IDMA_READ_SLAVE_ERR_MASK		(1 << 4)
#define IDMA_DEADLOCK_MASK			(1 << 2)
#define IDMA_FRAME_DONE_MASK			(1 << 1)
#define IDMA_ALL_IRQ_MASK			(0xFFB << 1)
#define IDMA_IRQ_ENABLE				(1 << 0)

/* defined for common part of driver only */
#define IDMA_RECOVERY_START_IRQ			IDMA_RECOVERY_TRG_IRQ
#define IDMA_READ_SLAVE_ERROR			IDMA_READ_SLAVE_ERR_IRQ
#define IDMA_STATUS_DEADLOCK_IRQ		IDMA_DEADLOCK_IRQ
#define IDMA_STATUS_FRAMEDONE_IRQ		IDMA_FRAME_DONE_IRQ
#define IDMA_AFBC_CONFLICT_IRQ			IDMA_LB_CONFLICT_IRQ

#define RDMA_IN_CTRL_0				0x0008
#define IDMA_ALPHA(_v)				((_v) << 24)
#define IDMA_ALPHA_MASK				(0xff << 24)
#define IDMA_IC_MAX(_v)				((_v) << 16)
#define IDMA_IC_MAX_MASK			(0xff << 16)
#define IDMA_SBWC_LOSSY				(1 << 14)
#define IDMA_IMG_FORMAT(_v)			((_v) << 8) // Check for list of image formats
#define IDMA_IMG_FORMAT_MASK			(0x3f << 8)

#define IDMA_IMG_FORMAT_ARGB8888		(0)
#define IDMA_IMG_FORMAT_ABGR8888		(1)
#define IDMA_IMG_FORMAT_RGBA8888		(2)
#define IDMA_IMG_FORMAT_BGRA8888		(3)
#define IDMA_IMG_FORMAT_XRGB8888		(4)
#define IDMA_IMG_FORMAT_XBGR8888		(5)
#define IDMA_IMG_FORMAT_RGBX8888		(6)
#define IDMA_IMG_FORMAT_BGRX8888		(7)
#define IDMA_IMG_FORMAT_RGB565			(8)
#define IDMA_IMG_FORMAT_BGR565			(9)
#define IDMA_IMG_FORMAT_ARGB2101010		(16)
#define IDMA_IMG_FORMAT_ABGR2101010		(17)
#define IDMA_IMG_FORMAT_RGBA1010102		(18)
#define IDMA_IMG_FORMAT_BGRA1010102		(19)

#define IDMA_IMG_FORMAT_NV21			(24)
#define IDMA_IMG_FORMAT_NV12			(25)
#define IDMA_IMG_FORMAT_YUV420_8P2		(26) // RM
#define IDMA_IMG_FORMAT_YVU420_8P2		(27) // RM
#define IDMA_IMG_FORMAT_YVU420_P010		(28)
#define IDMA_IMG_FORMAT_YUV420_P010		(29)

#define IDMA_IMG_FORMAT_NV61			(56)
#define IDMA_IMG_FORMAT_NV16			(57)
#define IDMA_IMG_FORMAT_YVU422_8P2		(58) // RM
#define IDMA_IMG_FORMAT_YUV422_8P2		(59) // RM
#define IDMA_IMG_FORMAT_YVU422_P210		(60)
#define IDMA_IMG_FORMAT_YUV422_P210		(61)

/* 90_ROT[6], Y_FLIP[4], X_FLIP[5] */
#define IDMA_ROT(_v)				((_v) << 4)
#define IDMA_ROT_MASK				(0x7 << 4)
#define IDMA_ROT_X_FLIP				(1 << 4)
#define IDMA_ROT_Y_FLIP				(2 << 4)
#define IDMA_ROT_180				(3 << 4)
#define IDMA_ROT_90				(4 << 4)
#define IDMA_ROT_90_X_FLIP			(5 << 4)
#define IDMA_ROT_90_Y_FLIP			(6 << 4)
#define IDMA_ROT_270				(7 << 4)
#define IDMA_FLIP(_v)				((_v) << 4)
#define IDMA_FLIP_MASK				(0x3 << 4)
#define IDMA_AFBC_EN				(1 << 3) // must keep as '0'
#define IDMA_SBWC_EN				(1 << 2)
#define IDMA_SAJC_EN				(1 << 1)
#define IDMA_BLOCK_EN				(1 << 0)

#define RDMA_SRC_SIZE                           0x0010
#define IDMA_SRC_HEIGHT(_v)                     ((_v) << 16)
#define IDMA_SRC_HEIGHT_MASK                    (0xFFFF << 16)
#define IDMA_SRC_WIDTH(_v)                      ((_v) << 0)
#define IDMA_SRC_WIDTH_MASK                     (0xFFFF << 0)

#define RDMA_SRC_OFFSET                         0x0014
#define IDMA_SRC_OFFSET_Y(_v)                   ((_v) << 16)
#define IDMA_SRC_OFFSET_Y_MASK                  (0x3FFF << 16)
#define IDMA_SRC_OFFSET_X(_v)                   ((_v) << 0)
#define IDMA_SRC_OFFSET_X_MASK                  (0x3FFF << 0)

#define RDMA_IMG_SIZE                           0x0018
#define IDMA_IMG_HEIGHT(_v)                     ((_v) << 16)
#define IDMA_IMG_HEIGHT_MASK                    (0x3FFF << 16)
#define IDMA_IMG_WIDTH(_v)                      ((_v) << 0)
#define IDMA_IMG_WIDTH_MASK                     (0x3FFF << 0)


#define RDMA_BLOCK_OFFSET			0x0020
#define IDMA_BLK_OFFSET_Y(_v)			((_v) << 16)
#define IDMA_BLK_OFFSET_Y_MASK			(0x3FFF << 16)
#define IDMA_BLK_OFFSET_X(_v)			((_v) << 0)
#define IDMA_BLK_OFFSET_X_MASK			(0x3FFF << 0)

#define RDMA_BLOCK_SIZE				0x0024
#define IDMA_BLK_HEIGHT(_v)			((_v) << 16)
#define IDMA_BLK_HEIGHT_MASK			(0x3FFF << 16)
#define IDMA_BLK_WIDTH(_v)			((_v) << 0)
#define IDMA_BLK_WIDTH_MASK			(0x3FFF << 0)

#define DMA_BASEADDR_P0				0x0040
#define DMA_BASEADDR_P1				0x0044
#define DMA_BASEADDR_P2				0x0048
#define DMA_BASEADDR_P3				0x004C

#define DMA_SRC_STRIDE_0			0x0050
#define DMA_STRIDE_3_SEL			(1 << 23)
#define DMA_STRIDE_2_SEL			(1 << 22)
#define DMA_STRIDE_1_SEL			(1 << 21)
#define DMA_STRIDE_0_SEL			(1 << 20)
#define DMA_STRIDE_SEL(_v)			((_v) << 20)
#define DMA_STRIDE_SEL_MASK			(0xF << 20)
#define DMA_CHROM_STRIDE_SEL			(1 << 16)
#define DMA_CHROM_STRIDE(_v)			((_v) << 0)
#define DMA_CHROM_STRIDE_MASK			(0xFFFF << 0)

#define DMA_SRC_STRIDE_1			0x0054
#define DMA_STRIDE_1(_v)			((_v) << 16)
#define DMA_STRIDE_1_MASK			(0xffff << 16)
#define DMA_STRIDE_0(_v)			((_v) << 0)
#define DMA_STRIDE_0_MASK			(0xffff << 0)

#define DMA_SRC_STRIDE_2			0x0058
#define DMA_STRIDE_3(_v)			((_v) << 16)
#define DMA_STRIDE_3_MASK			(0xffff << 16)
#define DMA_STRIDE_2(_v)			((_v) << 0)
#define DMA_STRIDE_2_MASK			(0xffff << 0)

/* Register is available in
 *  L0, L1, L2, L3, L4, L5, L6, L7
 */
#define RDMA_AFBC_PARAM				0x0070 // not supported in s5e8855

/* [128 x n]
 * 3: 384 byte 4: 512 byte 5: 640 byte
 * 6: 768 byte 8: 1024 byte
 */
#define IDMA_AFBC_BLK_BYTENUM(_v)		((_v) << 4)
#define IDMA_AFBC_BLK_BYTENUM_MASK		(0xf << 4)
#define IDMA_AFBC_BLK_SIZE(_v)			((_v) << 0)
#define IDMA_AFBC_BLK_SIZE_MASK			(0x3 << 0)
#define IDMA_AFBC_BLK_SIZE_16_16		(0)
#define IDMA_AFBC_BLK_SIZE_32_8			(1)
#define IDMA_AFBC_BLK_SIZE_64_4			(2)

#define RDMA_SAJC_PARAM				0x0060
#define IDMA_SAJC_SWIZZLE_MODE(_v)		((_v) << 4)
#define IDMA_SAJC_SWIZZLE_MODE_MASK		(0x1 << 4)
#define IDMA_INDEPENDENT_BLK(_v)		((_v) << 0)
#define IDMA_INDEPENDENT_BLK_MASK		(0x3 << 0)

/* Register is available in
 *  L0, L1, L2, L3, L4, L5, L6, L7
 */
#define RDMA_SBWC_PARAM				0x0064
#define IDMA_SBWC_LOSSY_COMP_MODE(_v)		((_v) << 16)
#define IDMA_SBWC_LOSSY_COMP_MODE_MASK		(0x3 << 16)

/* valid num 2~6, byte = _v * 32*/
#define IDMA_CHM_BLK_BYTENUM(_v)		((_v) << 8)
#define IDMA_CHM_BLK_BYTENUM_MASK		(0xf << 8)
/* valid num 3~6, byte = _v * 32*/
#define IDMA_LUM_BLK_BYTENUM(_v)		((_v) << 4)
#define IDMA_LUM_BLK_BYTENUM_MASK		(0xf << 4)
/* only valid 32x4, there are other value but not sure */
#define IDMA_CHM_BLK_SIZE(_v)			((_v) << 2)
#define IDMA_CHM_BLK_SIZE_MASK			(0x3 << 2)
#define IDMA_CHM_BLK_SIZE_32_4			(0)
#define IDMA_LUM_BLK_SIZE(_v)			((_v) << 0)
#define IDMA_LUM_BLK_SIZE_MASK			(0x3 << 0)
#define IDMA_LUM_BLK_SIZE_32_4			(0)

/* SAJC Constant Encoding value for RGB16/RGB32(2B or 4B RGB) */
#define RDMA_SAJC_CONST0			0x0068
/* SAJC Constant Encoding value for FP16(8B RGB) */
#define RDMA_SAJC_CONST1			0x006C

#define IDMA_PIXEL_VAL(_v)			((_v) << 0)
#define IDMA_PIXEL_VAL_MASK			(0xffffffff << 0)

#define RDMA_RECOVERY_CTRL			0x0080
#define IDMA_RECOVERY_NUM(_v)			((_v) << 1)
#define IDMA_RECOVERY_NUM_MASK			(0x7fffffff << 1)
#define IDMA_RECOVERY_EN			(1 << 0)

#define RDMA_DEADLOCK_CTRL			0x0100
#define IDMA_DEADLOCK_NUM(_v)			((_v) << 1)
#define IDMA_DEADLOCK_NUM_MASK			(0x7fffffff << 1)
#define IDMA_DEADLOCK_NUM_EN			(1 << 0)

#define RDMA_BUS_CTRL				0x0110
#define IDMA_ARCACHE_P3(_v)			((_v) << 12)
#define IDMA_ARCACHE_P3_MASK			(0xf << 12)
#define IDMA_ARCACHE_P2(_v)			((_v) << 8)
#define IDMA_ARCACHE_P2_MASK			(0xf << 8)
#define IDMA_ARCACHE_P1(_v)			((_v) << 4)
#define IDMA_ARCACHE_P1_MASK			(0xf << 4)
#define IDMA_ARCACHE_P0(_v)			((_v) << 0)
#define IDMA_ARCACHE_P0_MASK			(0xf << 0)

#define RDMA_LLC_CTRL				0x0114
#define IDMA_DATA_SHARE_TYPE_P3(_v)		((_v) << 28)
#define IDMA_DATA_SHARE_TYPE_P3_MASK		(0x3 << 28)
#define IDMA_LLC_HINT_P3(_v)			((_v) << 24)
#define IDMA_LLC_HINT_P3_MASK			(0x7 << 24)
#define IDMA_DATA_SHARE_TYPE_P2(_v)		((_v) << 20)
#define IDMA_DATA_SHARE_TYPE_P2_MASK		(0x3 << 20)
#define IDMA_LLC_HINT_P2(_v)			((_v) << 16)
#define IDMA_LLC_HINT_P2_MASK			(0x7 << 16)
#define IDMA_DATA_SHARE_TYPE_P1(_v)		((_v) << 12)
#define IDMA_DATA_SHARE_TYPE_P1_MASK		(0x3 << 12)
#define IDMA_LLC_HINT_P1(_v)			((_v) << 8)
#define IDMA_LLC_HINT_P1_MASK			(0x7 << 8)
#define IDMA_DATA_SHARE_TYPE_P0(_v)		((_v) << 4)
#define IDMA_DATA_SHARE_TYPE_P0_MASK		(0x3 << 4)
#define IDMA_LLC_HINT_P0(_v)			((_v) << 0)
#define IDMA_LLC_HINT_P0_MASK			(0x7 << 0)

#define RDMA_PERF_CTRL				0x0120
#define IDMA_DEGRADATION_TIME(_v)		((_v) << 16)
#define IDMA_DEGRADATION_TIME_MASK		(0xFFFF << 16)
#define IDMA_IC_MAX_DEG(_v)			((_v) << 4)
#define IDMA_IC_MAX_DEG_MASK			(0xFF << 4)
#define IDMA_DEGRADATION_EN			(1 << 0)

/* _n: [0,7], _v: [0x0, 0xF] */
#define RDMA_QOS_LUT_LOW			0x0130
#define RDMA_QOS_LUT_HIGH			0x0134
#define IDMA_QOS_LUT(_n, _v)			((_v) << (4*(_n)))
#define IDMA_QOS_LUT_MASK(_n)			(0xF << (4*(_n)))

#define RDMA_DYNAMIC_GATING_EN			0x0140
#define IDMA_SRAM_CG_EN				(1 << 31)
#define IDMA_DG_EN(_n, _v)			((_v) << (_n))
#define IDMA_DG_EN_MASK(_n)			(1 << (_n))
#define IDMA_DG_EN_ALL				(0x7FFFFFFF << 0)

#define RDMA_VOTF_EN				0x0180
#define IDMA_VOTF_RBUF_IDX(_v)			((_v) << 8)
#define IDMA_VOTF_RBUF_IDX_MASK			(0x7 << 8)
#define IDMA_VOTF_TRS_IDX(_v)			((_v) << 4)
#define IDMA_VOTF_TRS_IDX_MASK			(0xF << 4)
#define IDMA_VOTF_EN(_v)			((_v) << 0)
#define IDMA_VOTF_EN_MASK			(0x3 << 0)
#define IDMA_VOTF_DISABLE			(0)
#define IDMA_VOTF_FULL_CONN_MODE		(1)
#define IDMA_VOTF_HALF_CONN_MODE		(2)
#define IDMA_VOTF_HWFC_MODE			(3)

#define RDMA_VOTF_CTRL				0x0184
#define IDMA_VOTF_LINE_NUM(_v)			((_v) << 0)
#define IDMA_VOTF_LINE_NUM_MASK			(0xFF << 0)

#define RDMA_VOTF_STATUS			0x0188
#define IDMA_VOTF_SHD_STATIS_GET(_v)		(((_v) >> 0) & 0x1)

#define RDMA_MST_SECURITY			0x0200
#define RDMA_SLV_SECURITY			0x0204

#define RDMA_DEBUG_CTRL				0x0300
#define IDMA_DEBUG_SEL(_v)			((_v) << 16)
#define IDMA_DEBUG_EN				(0x1 << 0)

#define RDMA_DEBUG_DATA				0x0304

/* 0: AXI, 3: Pattern */
#define RDMA_IN_REQ_DEST			0x0308
#define IDMA_REQ_DEST_SEL(_v)			((_v) << 0)
#define IDMA_REQ_DEST_SEL_MASK			(0x3 << 0)

#define RDMA_PSLV_ERR_CTRL			0x030c
#define IDMA_PSLVERR_CTRL			(1 << 0)

#define RDMA_DEBUG_ADDR_P0			0x0310
#define RDMA_DEBUG_ADDR_P1			0x0314
#define RDMA_DEBUG_ADDR_P2			0x0318
#define RDMA_DEBUG_ADDR_P3			0x031C

#define RDMA_DEBUG_ADDR_CTRL			0x0320
#define IDMA_DEBUG_EN_ADDR_P3                   (1 << 3) // C2 - P3
#define IDMA_DEBUG_EN_ADDR_P2                   (1 << 2) // Y2 - P2
#define IDMA_DEBUG_EN_ADDR_P1                   (1 << 1) // C8 - P1
#define IDMA_DEBUG_EN_ADDR_P0                   (1 << 0) // Y8 - P0

#define RDMA_DEBUG_ADDR_ERR			0x0730
#define IDMA_ERR_ADDR_P3                        (1 << 3) // C2 - P3
#define IDMA_ERR_ADDR_P2                        (1 << 2) // Y2 - P2
#define IDMA_ERR_ADDR_P1                        (1 << 1) // C8 - P1
#define IDMA_ERR_ADDR_P0                        (1 << 0) // Y8 - P0
#define IDMA_ERR_ADDR_GET(_v)                   (((_v) >> 0) & 0xF)

#define RDMA_CONFIG_ERR_STATUS			0x0740
#define IDMA_CFG_ERR_ROTATION			(1 << 21)
#define IDMA_CFG_ERR_IMG_HEIGHT_ROTATION	(1 << 20)
#define IDMA_CFG_ERR_SAJC			(1 << 19)
#define IDMA_CFG_ERR_AFBC			(1 << 18)
#define IDMA_CFG_ERR_SBWC			(1 << 17)
#define IDMA_CFG_ERR_BLOCK			(1 << 16)
#define IDMA_CFG_ERR_FORMAT			(1 << 15)
#define IDMA_CFG_ERR_STRIDE3			(1 << 14)
#define IDMA_CFG_ERR_STRIDE2			(1 << 13)
#define IDMA_CFG_ERR_STRIDE1			(1 << 12)
#define IDMA_CFG_ERR_STRIDE0			(1 << 11)
#define IDMA_CFG_ERR_CHROM_STRIDE		(1 << 10)
#define IDMA_CFG_ERR_BASE_ADDR_P3               (1 << 9) // C2
#define IDMA_CFG_ERR_BASE_ADDR_P2               (1 << 8) // Y2
#define IDMA_CFG_ERR_BASE_ADDR_P1               (1 << 7) // C8
#define IDMA_CFG_ERR_BASE_ADDR_P0               (1 << 6) // Y8
#define IDMA_CFG_ERR_SRC_OFFSET_Y		(1 << 5)
#define IDMA_CFG_ERR_SRC_OFFSET_X		(1 << 4)
#define IDMA_CFG_ERR_IMG_HEIGHT			(1 << 3)
#define IDMA_CFG_ERR_IMG_WIDTH			(1 << 2)
#define IDMA_CFG_ERR_SRC_HEIGHT			(1 << 1)
#define IDMA_CFG_ERR_SRC_WIDTH			(1 << 0)
#define IDMA_CFG_ERR_GET(_v)			(((_v) >> 0) & 0x3FFFFF)

/* L0,4 only : Global is moved to one layer of each port */
#define GLB_DPU_DMA_VERSION			0xf00
#define GLB_DPU_VERSION                         (0x06270000)
#define GLB_DPU_DMA_VERSION_GET_MAJOR(_v)	(((_v) >> 24) & 0x000000ff)
#define GLB_DPU_DMA_VERSION_GET_MINOR(_v)	(((_v) >> 16) & 0x000000ff)

#define GLB_QCH_EN				0xf0c
#define GLB_DPU_QCH_EN				(1 << 0)

#define GLB_SWRST				0xf10
#define GLB_DPU_ALL_SWRST			(1 << 0)

#define GLB_GLB_CGEN				0xf14
#define GLB_DPU_INT_CGEN(_v)			((_v) << 0)
#define GLB_DPU_INT_CGEN_MASK			(0x7FFFFFFF << 0)

#define GLB_TEST_PATTERN0_3			0xf20
#define GLB_TEST_PATTERN0_2			0xf24
#define GLB_TEST_PATTERN0_1			0xf28
#define GLB_TEST_PATTERN0_0			0xf2c
#define GLB_TEST_PATTERN1_3			0xf30
#define GLB_TEST_PATTERN1_2			0xf34
#define GLB_TEST_PATTERN1_1			0xf38
#define GLB_TEST_PATTERN1_0			0xf3c

#define GLB_DEBUG_CTRL				0xf80
#define GLB_DEBUG_SEL(_v)			((_v) << 16)
#define GLB_DEBUG_EN				(0x1 << 0)

#define GLB_DEBUG_DATA				0xf84


/*
 *-------------------------------------------------------------------
 * WDMA(L8)x2 SFR list
 * base address : 0x1489_0000
 * < Layer.offset >
 *  L8 - 0x8000
 *-------------------------------------------------------------------
 */
#define WDMA_ENABLE				0x0000
#define ODMA_SRESET				(1 << 8)
#define ODMA_SHD_UPDATE_FORCE			(1 << 4)
#define ODMA_OP_STATUS				(1 << 2)
#define ODMA_INST_OFF_PEND			(1 << 1)

#define WDMA_IRQ				0x0004
#define ODMA_VOTF_ERR_IRQ			(1 << 29)
#define ODMA_CONFIG_ERR_IRQ			(1 << 28)
#define ODMA_SLICE6_DONE_IRQ			(1 << 27)
#define ODMA_SLICE5_DONE_IRQ			(1 << 26)
#define ODMA_SLICE4_DONE_IRQ			(1 << 25)
#define ODMA_SLICE3_DONE_IRQ			(1 << 24)
#define ODMA_SLICE2_DONE_IRQ			(1 << 23)
#define ODMA_SLICE1_DONE_IRQ			(1 << 22)
#define ODMA_SLICE0_DONE_IRQ			(1 << 21)
#define ODMA_INST_OFF_DONE_IRQ			(1 << 20)
#define ODMA_WRITE_SLAVE_ERR_IRQ		(1 << 19)
#define ODMA_DEADLOCK_IRQ			(1 << 17)
#define ODMA_FRAME_DONE_IRQ			(1 << 16)
#define ODMA_ALL_IRQ_CLEAR			(0x3FFB << 16)

#define ODMA_WRITE_SLAVE_ERROR			ODMA_WRITE_SLAVE_ERR_IRQ
#define ODMA_STATUS_FRAMEDONE_IRQ		ODMA_FRAME_DONE_IRQ
#define ODMA_STATUS_DEADLOCK_IRQ		ODMA_DEADLOCK_IRQ
#define ODMA_VOTF_ERR_MASK			(1 << 14)
#define ODMA_CONFIG_ERR_MASK			(1 << 13)
#define ODMA_SLICE6_DONE_MASK			(1 << 12)
#define ODMA_SLICE5_DONE_MASK			(1 << 11)
#define ODMA_SLICE4_DONE_MASK			(1 << 10)
#define ODMA_SLICE3_DONE_MASK			(1 << 9)
#define ODMA_SLICE2_DONE_MASK			(1 << 8)
#define ODMA_SLICE1_DONE_MASK			(1 << 7)
#define ODMA_SLICE0_DONE_MASK			(1 << 6)
#define ODMA_INST_OFF_DONE_MASK			(1 << 5)
#define ODMA_WRITE_SLAVE_ERR_MASK		(1 << 4)
#define ODMA_DEADLOCK_MASK			(1 << 2)
#define ODMA_FRAME_DONE_MASK			(1 << 1)
#define ODMA_ALL_IRQ_MASK			(0x3FFB << 1)
#define ODMA_IRQ_ENABLE				(1 << 0)

#define WDMA_OUT_CTRL_0				0x0008
#define ODMA_ALPHA(_v)				((_v) << 24)
#define ODMA_ALPHA_MASK				(0xff << 24)
#define ODMA_IC_MAX(_v)				((_v) << 16)
#define ODMA_IC_MAX_MASK			(0xff << 16)
#define ODMA_SBWC_LOSSY				(1 << 14)
#define ODMA_IMG_FORMAT(_v)			((_v) << 8)
#define ODMA_IMG_FORMAT_MASK			(0x3f << 8)
#define ODMA_AFBC_EN				(1 << 3) // must keep as '0'
#define ODMA_SBWC_EN				(1 << 2)
#define ODMA_SAJC_EN				(1 << 1)

#define WDMA_DST_SIZE				0x0010
#define ODMA_DST_HEIGHT(_v)			((_v) << 16)
#define ODMA_DST_HEIGHT_MASK			(0xFFFF << 16)
#define ODMA_DST_WIDTH(_v)			((_v) << 0)
#define ODMA_DST_WIDTH_MASK			(0xFFFF << 0)

#define WDMA_DST_OFFSET				0x0014
#define ODMA_DST_OFFSET_Y(_v)			((_v) << 16)
#define ODMA_DST_OFFSET_Y_MASK			(0x3FFF << 16)
#define ODMA_DST_OFFSET_X(_v)			((_v) << 0)
#define ODMA_DST_OFFSET_X_MASK			(0x3FFF << 0)

#define WDMA_IMG_SIZE				0x0018
#define ODMA_IMG_HEIGHT(_v)			((_v) << 16)
#define ODMA_IMG_HEIGHT_MASK			(0x3FFF << 16)
#define ODMA_IMG_WIDTH(_v)			((_v) << 0)
#define ODMA_IMG_WIDTH_MASK			(0x3FFF << 0)

/* WDMA uses BASEADDR_P0(0x40), BASEADDR_P1(0x44),
 * BASEADDR_P2(0x48), BASEADDR_P3(0x4C), STRIDE_0(0x50),
 * STRIDE_1(0x54), STRIDE_2(0x58) from RDMA macros */

#define WDMA_SAJC_PARAM				0x0060
#define IDMA_INDEPENDENT_BLK(_v)                ((_v) << 0)
#define IDMA_INDEPENDENT_BLK_MASK               (0x3 << 0)

#define WDMA_SBWC_PARAM				0x0064
#define IDMA_SBWC_LOSSY_COMP_MODE(_v)          	((_v) << 16)
#define IDMA_SBWC_LOSSY_COMP_MODE_MASK         	(0x3 << 16)
/* [32 x n] 2: 64 byte ....*/
#define ODMA_CHM_BLK_BYTENUM(_v)		((_v) << 8)
#define ODMA_CHM_BLK_BYTENUM_MASK		(0xf << 8)
#define ODMA_LUM_BLK_BYTENUM(_v)		((_v) << 4)
#define ODMA_LUM_BLK_BYTENUM_MASK		(0xf << 4)
/* only valid 32x4 */
#define ODMA_CHM_BLK_SIZE(_v)			((_v) << 2)
#define ODMA_CHM_BLK_SIZE_MASK			(0x3 << 2)
#define ODMA_CHM_BLK_SIZE_32_4			(0)
#define ODMA_CHM_BLK_SIZE_32_8			(1)
#define ODMA_CHM_BLK_SIZE_64_4			(2)
#define ODMA_LUM_BLK_SIZE(_v)			((_v) << 0)
#define ODMA_LUM_BLK_SIZE_MASK			(0x3 << 0)
#define ODMA_CHM_BLK_SIZE_32_4			(0)
#define ODMA_CHM_BLK_SIZE_32_8			(1)
#define ODMA_CHM_BLK_SIZE_64_4			(2)

/* SAJC Constant Encoding value for RGB16/RGB32(2B or 4B 0r 8B RGB) */
#define RDMA_SAJC_CONST0                       	0x0068
/* SAJC Constant Encoding value for FP16(8B RGB) */
#define RDMA_SAJC_CONST1                       	0x006C
#define IDMA_PIXEL_VAL(_v)                     	((_v) << 0)
#define IDMA_PIXEL_VAL_MASK                    	(0xffffffff << 0)

#define WDMA_AFBC_PARAM				0x0070
#define ODMA_AFBC_BLK_SIZE(_v)			((_v) << 0)
#define ODMA_AFBC_BLK_SIZE_MASK			(0x3 << 0)
#define ODMA_AFBC_BLK_SIZE_16_16		(0)
#define ODMA_AFBC_BLK_SIZE_32_8			(1)
#define ODMA_AFBC_BLK_SIZE_64_4			(2)

#define WDMA_DEADLOCK_CTRL			0x0100
#define ODMA_DEADLOCK_NUM(_v)			((_v) << 1)
#define ODMA_DEADLOCK_NUM_MASK			(0x7fffffff << 1)
#define ODMA_DEADLOCK_NUM_EN			(1 << 0)

#define WDMA_BUS_CTRL				0x0110
#define ODMA_AWCACHE_P3(_v)			((_v) << 12)
#define ODMA_AWCACHE_P3_MASK			(0xf << 12)
#define ODMA_AWCACHE_P2(_v)			((_v) << 8)
#define ODMA_AWCACHE_P2_MASK			(0xf << 8)
#define ODMA_AWCACHE_P1(_v)			((_v) << 4)
#define ODMA_AWCACHE_P1_MASK			(0xf << 4)
#define ODMA_AWCACHE_P0(_v)			((_v) << 0)
#define ODMA_AWCACHE_P0_MASK			(0xf << 0)

#define WDMA_LLC_CTRL				0x0114
#define ODMA_DATA_SAHRE_TYPE_P3(_v)		((_v) << 28)
#define ODMA_DATA_SAHRE_TYPE_P3_MASK		(0x3 << 28)
#define ODMA_LLC_HINT_P3(_v)			((_v) << 24)
#define ODMA_LLC_HINT_P3_MASK			(0x7 << 24)
#define ODMA_DATA_SAHRE_TYPE_P2(_v)		((_v) << 20)
#define ODMA_DATA_SAHRE_TYPE_P2_MASK		(0x3 << 20)
#define ODMA_LLC_HINT_P2(_v)			((_v) << 16)
#define ODMA_LLC_HINT_P2_MASK			(0x7 << 16)
#define ODMA_DATA_SAHRE_TYPE_P1(_v)		((_v) << 12)
#define ODMA_DATA_SAHRE_TYPE_P1_MASK		(0x3 << 12)
#define ODMA_LLC_HINT_P1(_v)			((_v) << 8)
#define ODMA_LLC_HINT_P1_MASK			(0x7 << 8)
#define ODMA_DATA_SAHRE_TYPE_P0(_v)		((_v) << 4)
#define ODMA_DATA_SAHRE_TYPE_P0_MASK		(0x3 << 4)
#define ODMA_LLC_HINT_P0(_v)			((_v) << 0)
#define ODMA_LLC_HINT_P0_MASK			(0x7 << 0)

#define WDMA_PERF_CTRL				0x0120
#define ODMA_DEGRADATION_TIME(_v)		((_v) << 16)
#define ODMA_DEGRADATION_TIME_MASK		(0xFFFF << 16)
#define ODMA_IC_MAX_DEG(_v)			((_v) << 4)
#define ODMA_IC_MAX_DEG_MASK			(0xFF << 4)
#define ODMA_DEGRADATION_EN			(1 << 0)

/* _n: [0,7], _v: [0x0, 0xF] */
#define WDMA_QOS_LUT_LOW			0x0130
#define WDMA_QOS_LUT_HIGH			0x0134
#define ODMA_QOS_LUT(_n, _v)			((_v) << (4*(_n)))
#define ODMA_QOS_LUT_MASK(_n)			(0xF << (4*(_n)))

#define WDMA_DYNAMIC_GATING_EN			0x0140
#define ODMA_SRAM_CG_EN				(1 << 31)
#define ODMA_DG_EN(_n, _v)			((_v) << (_n))
#define ODMA_DG_EN_MASK(_n)			(1 << (_n))
#define ODMA_DG_EN_ALL				(0x7FFFFFFF << 0)

/* HWFC */
#define WDMA_HWFC_EN				0x0160
#define ODMA_HWFC_PRODUCE_IDX(_v)		((_v) << 8)
#define ODMA_HWFC_PRODUCE_IDX_MASK		(0xF << 8)
#define ODMA_HWFC_EN(_v)			((_v) << 0)
#define ODMA_HWFC_EN_MASK			(0x1 << 0)

#define WDMA_HWFC_CONSUME_IDX			0x0164
#define ODMA_HWFC_CONSUME_IDX(_v)		((_v) << 0)
#define ODMA_HWFC_CONSUME_IDX_MASK		(0xF << 0)

#define WDMA_HWFC_CTRL				0x0168
#define ODMA_HWFC_LINE_NUM(_v)			((_v) << 0)
#define ODMA_HWFC_LINE_NUM_MASK			(0xFF << 0)

#define WDMA_HWFC_DEBUG_SEL			0x0170
#define ODMA_HWFC_DEBUG_SEL(_v)			((_v) << 0)
#define ODMA_HWFC_DEBUG_SEL_MASK		(0xF << 0)

#define WDMA_HWFC_DEBUG_DATA			0x0174
#define ODMA_HWFC_PRODUCE_LINE_CNT_GET(_v)	(((_v) >> 0) & 0x3FFF)
//------

/* VOTF */
#define WDMA_VOTF_EN				0x0180
#define ODMA_VOTF_MST_IDX(_v)			((_v) << 8)
#define ODMA_VOTF_MST_IDX_MASK			(0xF << 8)
#define ODMA_VOTF_EN(_v)			((_v) << 0)
#define ODMA_VOTF_EN_MASK			(0x1 << 0)
#define ODMA_VOTF_DISABLE			(0)
#define ODMA_VOTF_ENABLE			(1)

#define WDMA_VOTF_CTRL				0x0184
#define ODMA_VOTF_LINE_NUM(_v)			((_v) << 0)
#define ODMA_VOTF_LINE_NUM_MASK			(0xFF << 0)

#define WDMA_VOTF_STATUS			0x0188
#define ODMA_VOTF_SHD_STATIS_GET(_v)		(((_v) >> 0) & 0x1)
//------

#define WDMA_MST_SECURITY			0x200
#define WDMA_SLV_SECURITY			0x204

#define WDMA_DEBUG_CTRL				0x0300
#define ODMA_DEBUG_SEL(_v)			((_v) << 16)
#define ODMA_DEBUG_EN				(0x1 << 0)

#define WDMA_DEBUG_DATA				0x0304

#define WDMA_PSLV_ERR_CTRL			0x030c
#define ODMA_PSLVERR_CTRL			(1 << 0)

/* DSC Slice : USB-TV */
#define WDMA_SLICE0_BYTE_CNT			0x01A0
#define ODMA_SLICE0_BYTE_CNT(_v)		((_v) & 0xFFFFFFFF)

#define WDMA_SLICE1_BYTE_CNT			0x01A4
#define ODMA_SLICE1_BYTE_CNT(_v)		((_v) & 0xFFFFFFFF)

#define WDMA_SLICE2_BYTE_CNT			0x01A8
#define ODMA_SLICE2_BYTE_CNT(_v)		((_v) & 0xFFFFFFFF)

#define WDMA_SLICE3_BYTE_CNT			0x01AC
#define ODMA_SLICE3_BYTE_CNT(_v)		((_v) & 0xFFFFFFFF)

#define WDMA_SLICE4_BYTE_CNT			0x01B0
#define ODMA_SLICE4_BYTE_CNT(_v)		((_v) & 0xFFFFFFFF)

#define WDMA_SLICE5_BYTE_CNT			0x01B4
#define ODMA_SLICE5_BYTE_CNT(_v)		((_v) & 0xFFFFFFFF)

#define WDMA_SLICE6_BYTE_CNT			0x01B8
#define ODMA_SLICE6_BYTE_CNT(_v)		((_v) & 0xFFFFFFFF)

#define WDMA_FRAME_BYTE_CNT			0x01BC
#define ODMA_FRAME_BYTE_CNT(_v)			((_v) & 0xFFFFFFFF)

#define WDMA_NON_IMAGE_CTRL			0x01C0
#define ODMA_WB_PATH_DSC(_v)			((_v) << 2)
#define ODMA_WB_PATH_DSC_MASK			(0x1 << 2)
//------

#define WDMA_CONFIG_ERR_STATUS			0x0740
#define ODMA_CFG_ERR_STRIDE1			(1 << 12)
#define ODMA_CFG_ERR_STRIDE0			(1 << 11)
#define ODMA_CFG_ERR_CHROM_STRIDE		(1 << 10)
#define ODMA_CFG_ERR_BASE_ADDR_C2		(1 << 9)
#define ODMA_CFG_ERR_BASE_ADDR_Y2		(1 << 8)
#define ODMA_CFG_ERR_BASE_ADDR_C8		(1 << 7)
#define ODMA_CFG_ERR_BASE_ADDR_Y8		(1 << 6)
#define ODMA_CFG_ERR_DST_OFFSET_Y		(1 << 5)
#define ODMA_CFG_ERR_DST_OFFSET_X		(1 << 4)
#define ODMA_CFG_ERR_IMG_HEIGHT			(1 << 3)
#define ODMA_CFG_ERR_IMG_WIDTH			(1 << 2)
#define ODMA_CFG_ERR_DST_HEIGHT			(1 << 1)
#define ODMA_CFG_ERR_DST_WIDTH			(1 << 0)
#define ODMA_CFG_ERR_GET(_v)			(((_v) >> 0) & 0x1FFF)

/*
 *-------------------------------------------------------------------
 * RCD(L12)x2 DMA SFR list
 * base address : 0x1689_0000
 * < Layer.offset >
 *  L12 - 0xC000
 * - EDMA : DMA for RCD - Decorative Layer
 *-------------------------------------------------------------------
 */
#define RCD_SHD_OFFSET			(0x0400)

#define RCD_ENABLE                      (0x0000)
#define RCD_SRESET                      (1 << 8)
#define RCD_SFR_UPDATE_FORCE            (1 << 4)
#define RCD_OP_STATUS                   (1 << 2)
#define OP_STATUS_IDLE                  (0)
#define OP_STATUS_BUSY                  (1)
#define RCD_INST_OFF_PEND		(1 << 1)
#define INST_OFF_PEND                   (1)
#define INST_OFF_NOT_PEND               (0)

#define RCD_IRQ                         (0x0004)
#define RCD_DECODER_ERR_IRQ             (1 << 23)
#define RCD_RECOVERY_TRG_IRQ            (1 << 22)
#define RCD_CONFIG_ERR_IRQ              (1 << 21)
#define RCD_INST_OFF_DONE               (1 << 20)
#define RCD_READ_SLAVE_ERR_IRQ          (1 << 19)
#define RCD_DEADLOCK_IRQ                (1 << 17)
#define RCD_FRAME_DONE_IRQ              (1 << 16)
#define RCD_ALL_IRQ_CLEAR               (0xFB << 16) // 0x7B -> 0xFB
#define RCD_DECODER_ERR_TRG_MASK	(1 << 8)
#define RCD_RECOVERY_TRG_MASK           (1 << 7)
#define RCD_CONFIG_ERR_MASK             (1 << 6)
#define RCD_INST_OFF_DONE_MASK          (1 << 5)
#define RCD_READ_SLAVE_ERR_MASK         (1 << 4)
#define RCD_DEADLOCK_MASK               (1 << 2)
#define RCD_FRAME_DONE_MASK             (1 << 1)
#define RCD_ALL_IRQ_MASK                (0xFB << 1) // 0x7B -> (0xFB << 1)
#define RCD_IRQ_ENABLE                  (1 << 0)

#define RCD_IN_CTRL_0                   (0x0008)
#define RCD_IC_MAX(_v)                  ((_v) << 16)
#define RCD_IC_MAX_MASK                 (0xFF << 16)
#define RCD_MODE(_v)                 	((_v) << 4)
#define RCD_MODE_COMP                 	(1)
#define RCD_MODE_UNCOMP                	(0)
#define RCD_BLOCK_EN                    (1 << 0)

#define RCD_SRC_SIZE                    (0x0010)
#define RCD_SRC_HEIGHT(_v)              ((_v) << 16)
#define RCD_SRC_HEIGHT_MASK             (0xFFFF << 16)
#define RCD_SRC_WIDTH(_v)               ((_v) << 0)
#define RCD_SRC_WIDTH_MASK              (0xFFFF << 0)

#define RCD_SRC_OFFSET                  (0x0014)
#define RCD_SRC_OFFSET_Y(_v)            ((_v) << 16)
#define RCD_SRC_OFFSET_Y_MASK           (0x3FFF << 16)
#define RCD_SRC_OFFSET_X(_v)            ((_v) << 0)
#define RCD_SRC_OFFSET_X_MASK           (0x3FFF << 0)

#define RCD_IMG_SIZE                    (0x0018)
#define RCD_IMG_HEIGHT(_v)              ((_v) << 16)
#define RCD_IMG_HEIGHT_MASK             (0x3FFF << 16)
#define RCD_IMG_WIDTH(_v)               ((_v) << 0)
#define RCD_IMG_WIDTH_MASK              (0x3FFF << 0)

#define RCD_BLOCK_OFFSET                (0x0020)
#define RCD_BLK_OFFSET_Y(_v)            ((_v) << 16)
#define RCD_BLK_OFFSET_Y_MASK           (0x3FFF << 16)
#define RCD_BLK_OFFSET_X(_v)            ((_v) << 0)
#define RCD_BLK_OFFSET_X_MASK           (0x3FFF << 0)

#define RCD_BLOCK_SIZE                  (0x0024)
#define RCD_BLK_HEIGHT(_v)              ((_v) << 16)
#define RCD_BLK_HEIGHT_MASK             (0x3FFF << 16)
#define RCD_BLK_WIDTH(_v)               ((_v) << 0)
#define RCD_BLK_WIDTH_MASK              (0x3FFF << 0)

#define RCD_BLOCK_VALUE                 (0x0028)
#define RCD_PIX_VALUE(_v)               ((_v) << 0)
#define RCD_PIX_VALUE_MASK              (0xFFFFFFFF << 0) // A -> ARGB

#define RCD_BASEADDR_P0			(0x0040)

#define RCD_RECOVERY_CTRL               (0x0070)
#define RCD_RECOVERY_NUM(_v)            ((_v) << 1)
#define RCD_RECOVERY_NUM_MASK           (0x7fffffff << 1)
#define RCD_RECOVERY_EN                 (1 << 0)

#define RCD_DEADLOCK_CTRL               (0x0100)
#define RCD_DEADLOCK_NUM(_v)            ((_v) << 1)
#define RCD_DEADLOCK_NUM_MASK           (0x7FFFFFFF << 1)
#define RCD_DEADLOCK_NUM_EN             (1 << 0)

#define RCD_BUS_CTRL                    (0x0110)
#define RCD_ARCACHE_P0(_v)              ((_v) << 0)
#define RCD_ARCACHE_P0_MASK             (0xf << 0)

#define RCD_LLC_CTRL                    (0x0114)
#define RCD_DATA_SHARE_TYPE_P0(_v)      ((_v) << 4)
#define RCD_DATA_SHARE_TYPE_P0_MASK     (0x3 << 4)
#define RCD_LLC_HINT_P0(_v)             ((_v) << 0)
#define RCD_LLC_HINT_P0_MASK            (0x7 << 0)

#define RCD_PERF_CTRL                   (0x0120)
#define RCD_DEGRADATION_TIME(_v)        ((_v) << 16)
#define RCD_DEGRADATION_TIME_MASK       (0xFFFF << 16)
#define RCD_IC_MAX_DEG(_v)              ((_v) << 4)
#define RCD_IC_MAX_DEG_MASK             (0xFF << 4)
#define RCD_DEGRADATION_EN              (1 << 0)

/* _n: [0,7], _v: [0x0, 0xF] */
#define RCD_QOS_LUT_LOW                 (0x0130)
#define RCD_QOS_LUT_HIGH                (0x0134)
#define RCD_QOS_LUT(_n, _v)             ((_v) << (4*(_n)))
#define RCD_QOS_LUT_MASK(_n)            (0xF << (4*(_n)))

#define RCD_DYNAMIC_GATING_EN           (0x0140)
#define RCD_SRAM_CG_EN                  (1 << 31)
#define RCD_DG_EN(_n, _v)               ((_v) << (_n))
#define RCD_DG_EN_MASK(_n)              (1 << (_n))
#define RCD_DG_EN_ALL                   (0x7FFFFFFF << 0)

#define RCD_MST_SECURITY                (0x0200)
#define RCD_SLV_SECURITY                (0x0204)

#define RCD_DEBUG_CTRL                  (0x0300)
#define RCD_DEBUG_CTRL_SEL(_v)          ((_v) << 16)
#define RCD_DEBUG_CTRL_SEL_MASK         (0xFFFF << 16)
#define RCD_DEBUG_CTRL_EN               (1 << 0)

#define RCD_DEBUG_DATA                  (0x0304)

#define RCD_IN_REQ_DEST                 (0x0308)
#define RCD_REQ_DEST_SEL(_v)            ((_v) << 0)
#define RCD_REQ_DEST_SEL_MASK           (0x3 << 0)

#define RCD_PSLV_ERR_CTRL               (0x030C)
#define RCD_PSLV_ERR_CTRL_EN            (1 << 0)

#define RCD_S_CONFIG_ERR_STATUS         (0x0740)
#define RCD_DECODER_ERR_INFO(_v)        ((_v) << 0)
#define RCD_DECODER_ERR_MASK        	(0xFFFFFFFF << 0)
#define RCD_CFG_ERR_GET(_v)             (((_v) >> 0) & RCD_DECODER_ERR_MASK)

/*
 *-------------------------------------------------------------------
 * DQE_CGC(L14)x2 DMA SFR list
 * base address : 0x1689_0000
 * < Layer.offset >
 *  L14 - 0xE000
 * - EDMA : DMA for DQE CGC - update/write many registers quickly
 *
 * - dqecgc_reg.c & regs-dqecgc.h
 *-------------------------------------------------------------------
 */
#define EDMA_CGC_ENABLE				0x0000
#define CGC_SRESET				(1 << 8)
#define CGC_OP_STATUS_SET3			(1 << 7)
#define CGC_OP_STATUS_SET2			(1 << 6)
#define CGC_OP_STATUS_SET1			(1 << 5)
#define CGC_OP_STATUS_SET0			(1 << 4)
#define CGC_OP_STATUS_ALL			(1 << 1)
#define CGC_START				(1 << 0)

#define EDMA_CGC_IRQ				0x0004
#define CGC_INVLID_SFR_ADDR_IRQ			(1 << 22)
#define CGC_CONFIG_ERR_IRQ			(1 << 21)
#define CGC_READ_SLAVE_ERR_IRQ			(1 << 19)
#define CGC_DEADLOCK_IRQ			(1 << 17)
#define CGC_FRAME_DONE_IRQ			(1 << 16)
#define CGC_ALL_IRQ_CLEAR			(0x6B << 16)
#define CGC_INVLID_SFR_ADDR_MASK		(1 << 7)
#define CGC_CONFIG_ERR_MASK			(1 << 6)
#define CGC_READ_SLAVE_ERR_MASK			(1 << 4)
#define CGC_DEADLOCK_MASK			(1 << 2)
#define CGC_FRAME_DONE_MASK			(1 << 1)
#define CGC_ALL_IRQ_MASK			(0x6B << 1)
#define CGC_IRQ_ENABLE				(1 << 0)

#define EDMA_CGC_IN_CTRL_0			0x0008
#define CGC_IC_MAX(_v)				((_v) << 16)
#define CGC_IC_MAX_MASK				(0xff << 16)

#define EDMA_CGC_IN_CTRL_1			0x000C
#define CGC_START_EN_SET3			(1 << 3)
#define CGC_START_EN_SET2			(1 << 2)
#define CGC_START_EN_SET1			(1 << 1)
#define CGC_START_EN_SET0			(1 << 0)

#define EDMA_CGC_IN_CTRL_SIZE_0			0x0010
#define EDMA_CGC_IN_CTRL_SIZE_1			0x0014
#define EDMA_CGC_IN_CTRL_SIZE_2			0x0018
#define EDMA_CGC_IN_CTRL_SIZE_3			0x001C

#define EDMA_CGC_BASE_ADDR_SET0			0x0040
#define EDMA_CGC_BASE_ADDR_SET1			0x0044
#define EDMA_CGC_BASE_ADDR_SET2			0x0048
#define EDMA_CGC_BASE_ADDR_SET3			0x004C
#define EDMA_CGC_ADDR_OFFSET			(32)
#define EDMA_CGC_BASE_ADDR_OFFSET		(48)

#define EDMA_CGC_DEADLOCK_CTRL			0x0100
#define CGC_DEADLOCK_NUM(_v)			((_v) << 1)
#define CGC_DEADLOCK_NUM_MASK			(0x7fffffff << 1)
#define CGC_DEADLOCK_NUM_EN			(1 << 0)

#define EDMA_CGC_BUS_CTRL			0x0110
#define CGC_ARCACHE(_v)				((_v) << 0)
#define CGC_ARCACHE_MASK			(0xf << 0)

#define EDMA_CGC_LLC_CTRL			0x0114
#define CGC_DATA_SHARE_TYPE(_v)			((_v) << 4)
#define CGC_DATA_SHARE_TYPE_MASK		(0x3 << 4)
#define CGC_LLC_HINT(_v)			((_v) << 0)
#define CGC_LLC_HINT_MASK			(0x7 << 0)

#define EDMA_CGC_PERF_CTRL			0x0120
#define CGC_DEGRADATION_TIME(_v)		((_v) << 16)
#define CGC_DEGRADATION_TIME_MASK		(0xFFFF << 16)
#define CGC_IC_MAX_DEG(_v)			((_v) << 4)
#define CGC_IC_MAX_DEG_MASK			(0xFF << 4)
#define CGC_DEGRADATION_EN			(1 << 0)

/* _n: [0,7], _v: [0x0, 0xF] */
#define EDMA_CGC_QOS_LUT_LOW			0x0130
#define EDMA_CGC_QOS_LUT_HIGH			0x0134
#define CGC_QOS_LUT(_n, _v)			((_v) << (4*(_n)))
#define CGC_QOS_LUT_MASK(_n)			(0xF << (4*(_n)))

#define EDMA_CGC_DYNAMIC_GATING_EN		0x0140
#define CGC_SRAM_CG_EN				(1 << 31)
#define CGC_DG_EN(_n, _v)			((_v) << (_n))
#define CGC_DG_EN_MASK(_n)			(1 << (_n))
#define CGC_DG_EN_ALL				(0x7FFFFFFF << 0)

#define EDMA_CGC_MST_SECURITY			0x200
#define EDMA_CGC_SLV_SECURITY			0x204

#define EDMA_CGC_DEBUG_CTRL			0x0300
#define CGC_DEBUG_SEL(_v)			((_v) << 16)
#define CGC_DEBUG_EN				(0x1 << 0)

#define EDMA_CGC_DEBUG_DATA			0x0304

#define EDMA_CGC_PSLV_ERR_CTRL			0x030c
#define CGC_PSLVERR_CTRL			(1 << 0)

#define EDMA_CGC_CONFIG_ERR_STATUS		0x0740
//<TBD>
#define CGC_CFG_ERR_GET(_v)			(((_v) >> 0) & 0xF)

#define EDMA_SHD_OFFSET				0x0800

/*
 *-------------------------------------------------------------------
 * DPUF_C2SERV Global(vOTF) SFR list
 * base address : 0x1488_0000
 *
 * - votf_reg.c & regs-ovtf.h
 *-------------------------------------------------------------------
 */

//<NOT USED>


/*
 *-------------------------------------------------------------------
 * DPP(L0~L7, L8[WB]) SFR list
 * base address : 0x1684_0000(DPU_DPP0)
 * < Layer.offset >
 *  L0      L1(SCL) L2      L3(SCL) L4      L5      L6      L7(SCL)  L10
 *  0x0000  0x1000  0x2000  0x3000  0x4000  0x5000  0x6000  0x7000  0xA000
 *-------------------------------------------------------------------
 */
#define DPP_COM_SHD_OFFSET			0x0100
#define DPP_SCL_SHD_OFFSET			0x0400

#define DPP_COM_VERSION				0x0000
#define DPP_VERSION				(0x06270000)
#define DPP_COM_VERSION_GET_MAJOR(_v)		(((_v) >> 24) & 0x000000ff)
#define DPP_COM_VERSION_GET_MINOR(_v)		(((_v) >> 16) & 0x000000ff)


#define DPP_COM_SWRST_CON			0x0004
#define DPP_SRESET				(1 << 0)

#define DPP_COM_QCH_CON				0x0008
#define DPP_QACTIVE				(1 << 0)

#define DPP_COM_PSLVERR_CON			0x000c
#define DPP_PSLVERR_EN				(1 << 0)

#define DPP_COM_IRQ_CON				0x0010
#define DPP_IRQ_EN				(1 << 0)

#define DPP_COM_IRQ_MASK			0x0014
#define DPP_CFG_ERROR_MASK			(1 << 1)
#define DPP_FRM_DONE_MASK			(1 << 0)
#define DPP_ALL_IRQ_MASK			(0x3 << 0)

#define DPP_COM_IRQ_STATUS			0x0018
#define DPP_CFG_ERROR_IRQ			(1 << 1)
#define DPP_FRM_DONE_IRQ			(1 << 0)
#define DPP_ALL_IRQ_CLEAR			(0x3 << 0)

#define DPP_COM_CFG_ERROR_STATUS		0x001c
#define DPP_CFG_ERR_SCL_POS			(1 << 4)
#define DPP_CFG_ERR_SCL_RATIO			(1 << 3)
#define DPP_CFG_ERR_ODD_SIZE			(1 << 2)
#define DPP_CFG_ERR_MAX_SIZE			(1 << 1)
#define DPP_CFG_ERR_MIN_SIZE			(1 << 0)

#define DPP_COM_LC_CON                          0x0020
#define DPP_LC_CAPTURE_MASK                     (1 << 2)
#define DPP_LC_MODE(_V)                         ((_V) << 1)
#define DPP_LC_MODE_MASK                        (1 << 1)
#define DPP_LC_EN(_v)                           ((_v) << 0)
#define DPP_LC_EN_MASK                          (1 << 0)

#define DPP_WB_LC_STATUS			0x0024
#define DPP_LC_COUNTER_GET(_v)			(((_v) >> 0) & 0xFFFFFFFF)

#define DPP_COM_DBG_CON				0x0028
#define DPP_DBG_SEL(_v)				((_v) << 16)
#define DPP_DBG_EN				(1 << 0)

#define DPP_COM_DBG_STATUS			0x002c

#define DPP_COM_OP_STATUS			0x0030
#define DPP_OP_STATUS				(1 << 0)

#define DPP_COM_TZPC				0x0034
#define DPP_TZPC				(1 << 0)

#define DPP_COM_IO_CON				0x0038
/* Newly added bit BEXT_MODE, only for WB DPP channel*/
#define DPP_WB_BEXT_MODE(_v)			((_v) << 8)
#define DPP_WB_BEXT_MODE_MASK			(1 << 8)
#define DPP_WB_BEXT_MSB_COPY			DPP_BEXT_MODE(0)
#define DPP_WB_BEXT_ZERO_PADDING		DPP_BEXT_MODE(1)
#define DPP_ALPHA_SEL(_v)			((_v) << 7)
#define DPP_ALPHA_SEL_MASK			(1 << 7)
#define DPP_BPC_MODE(_v)			((_v) << 6)
#define DPP_BPC_MODE_MASK			(1 << 6)
#define DPP_IMG_FORMAT(_v)			((_v) << 0)
#define DPP_IMG_FORMAT_MASK			(0x7 << 0)
#define DPP_IMG_FORMAT_ARGB8888			(0 << 0)
#define DPP_IMG_FORMAT_ARGB8101010		(1 << 0)
#define DPP_IMG_FORMAT_YUV420_8P		(2 << 0)
#define DPP_IMG_FORMAT_YUV420_P010		(3 << 0)
#define DPP_IMG_FORMAT_YUV422_8P		(5 << 0)
#define DPP_IMG_FORMAT_YUV422_P210		(6 << 0)

#define DPP_COM_IMG_SIZE			0x003c
#define DPP_IMG_HEIGHT(_v)			((_v) << 16)
#define DPP_IMG_HEIGHT_MASK			(0x3FFF << 16)
#define DPP_IMG_WIDTH(_v)			((_v) << 0)
#define DPP_IMG_WIDTH_MASK			(0x3FFF << 0)

#define DPP_COM_CSC_CON				0x0040
#define DPP_CSC_TYPE(_v)			((_v) << 2)
#define DPP_CSC_TYPE_MASK			DPP_CSC_TYPE(3)
#define DPP_CSC_TYPE_BT601			DPP_CSC_TYPE(0)
#define DPP_CSC_TYPE_BT709			DPP_CSC_TYPE(1)
#define DPP_CSC_TYPE_BT2020			DPP_CSC_TYPE(2)
#define DPP_CSC_TYPE_DCI_P3			DPP_CSC_TYPE(3)

#define DPP_CSC_RANGE(_v)			((_v) << 1)
#define DPP_CSC_RANGE_MASK			DPP_CSC_RANGE(1)
#define DPP_CSC_RANGE_LIMITED			DPP_CSC_RANGE(0)
#define DPP_CSC_RANGE_FULL			DPP_CSC_RANGE(1)

#define DPP_CSC_MODE(_v)			((_v) << 0)
#define DPP_CSC_MODE_MASK			DPP_CSC_MODE(1)
#define DPP_CSC_MODE_HARDWIRED			DPP_CSC_MODE(0)
#define DPP_CSC_MODE_CUSTOMIZED			DPP_CSC_MODE(1)

/**
 * CSC : S6.9
 * (00-01-02) : Reg0.L-Reg0.H-Reg1.L
 * (10-11-12) : Reg1.H-Reg2.L-Reg2.H
 * (20-21-22) : Reg3.L-Reg3.H-Reg4.L
 */
#define DPP_COM_CSC_COEF0			0x0044
#define DPP_COM_CSC_COEF1			0x0048
#define DPP_COM_CSC_COEF2			0x004c
#define DPP_COM_CSC_COEF3			0x0050
#define DPP_COM_CSC_COEF4			0x0054
#define DPP_CSC_COEF_H(_v)			((_v) << 16)
#define DPP_CSC_COEF_H_MASK			(0xFFFF << 16)
#define DPP_CSC_COEF_L(_v)			((_v) << 0)
#define DPP_CSC_COEF_L_MASK			(0xFFFF << 0)
#define DPP_CSC_COEF_XX(_n, _v)			((_v) << (0 + (16 * (_n))))
#define DPP_CSC_COEF_XX_MASK(_n)		(0xFFFF << (0 + (16 * (_n))))

/* L0~L7 and L10 */
#define DPP_COM_HDR_CON				0x0058
#define DPP_WB_MULT_EN				(1 << 0) // Applicable for WB DPP channel
/* TBD: there are 2 bit fields PRE_MUL_EN & DEPRE_MUL_EN */
#define DPP_PRE_MUL_EN				(1 << 1) // Applicable for Read DPP channel
#define DEPRE_MUL_EN				(1 << 0) // Applicable for Read DPP channel

#define DPP_COM_DITH_CON			0x005c
/* Only for WB DPP: Start */
#define DPP_WB_DITH_EN(_v)			((_v) << 4)
#define DPP_WB_DITH_EN_MASK			(0x03 << 4)
#define DPP_WB_DITH_EN_OFF			DPP_WB_DITH_EN(0)
#define DPP_WB_DITH_EN_NORMAL			DPP_WB_DITH_EN(1)
#define DPP_WB_DITH_EN_TRUNCATE			DPP_WB_DITH_EN(2)
#define DPP_WB_DITH_EN_ROUND_UP			DPP_WB_DITH_EN(3)
#define DPP_WB_DITH_MASK_SEL(_v)		((_v) << 1)
#define DPP_WB_DITH_MASK_SEL_MASK		(0x07 << 1)
#define DPP_WB_DITH_MASK_SPIN			(1 << 0)
/* Only for WB DPP: End */

#define DPP_DITH_MASK_SEL			(1 << 1)
#define DPP_DITH_MASK_SPIN			(1 << 0)

#define DPP_COM_SUB_CON				0x0060
#define DPP_UV_OFFSET_Y(_v)			((_v) << 4)
#define DPP_UV_OFFSET_Y_MASK			(0x7 << 4)
#define DPP_UV_OFFSET_X(_v)			((_v) << 0)
#define DPP_UV_OFFSET_X_MASK			(0x7 << 0)

#define DPP_COM_SUB_CON2			0x0064
#define DPP_X_ODD(_v)				((_v) << 16)
#define DPP_X_ODD_MASK				(0x1 << 16)
#define DPP_Y_ODD(_v)				((_v) << 0)
#define DPP_Y_ODD_MASK				(0x1 << 0)


/* Only for WB DPP channel: START*/
#define DPP_WB_CROP_CONTROL			0x0064
#define DPP_WB_CROP_EN				(1 << 0)

#define DPP_WB_CROP_START_POS			0x0068
#define DPP_WB_START_X(_v)			((_v) << 16)
#define DPP_WB_START_X_MASK			(0x3FFF << 16)
#define DPP_WB_START_Y(_v)			((_v) << 0)
#define DPP_WB_START_Y_MASK			(0x3FFF << 0)

#define DPP_WB_CROP_END_POS			0x006C
#define DPP_WB_END_X(_v)			((_v) << 16)
#define DPP_WB_END_X_MASK			(0x3FFF << 16)
#define DPP_WB_END_Y(_v)			((_v) << 0)
#define DPP_WB_END_Y_MASK			(0x3FFF << 0)
/* Only for WB DPP channel: END */

/* TBD: COM_AOSP_COEF_x needs to be added
 * 0x70 - 0x90
 */


/* TBD: COM_AOSP_OFFS_x needs to be added
 * 0xA0 - 0xA8
 */


/* TBD: COM_AOSP_CON needs to be added
 * 0xB0
 */

/* SCL , only L1, L3 and L7 supports SCL */
#define DPP_COM_SCL_SCALED_IMG_SIZE		0x0200
#define DPP_SCL_SCALED_IMG_HEIGHT(_v)		((_v) << 16)
#define DPP_SCL_SCALED_IMG_HEIGHT_MASK		(0x3FFF << 16)
#define DPP_SCL_SCALED_IMG_WIDTH(_v)		((_v) << 0)
#define DPP_SCL_SCALED_IMG_WIDTH_MASK		(0x3FFF << 0)

#define DPP_COM_SCL_MAIN_H_RATIO		0x0204
#define DPP_SCL_H_RATIO(_v)			((_v) << 0)
#define DPP_SCL_H_RATIO_MASK			(0xFFFFFF << 0)

#define DPP_COM_SCL_MAIN_V_RATIO		0x0208
#define DPP_SCL_V_RATIO(_v)			((_v) << 0)
#define DPP_SCL_V_RATIO_MASK			(0xFFFFFF << 0)

/* TBD: Initial Phase : S11.20 */

/*
 *-------------------------------------------------------------------
 * DPP_SCL_COEF(0~3) + ALPHA SFR list
 * base address : 0x1484_0000
 * < Coef.offset >
 *  COEF0   COEF1   COEF2   COEF3
 *  0x0000  0x1000  0x2000  0x3000
 *-------------------------------------------------------------------
 */
/* coef_id = [0,1,2,3] */
// TBD:Above explaination should be changed
#define DPP_SCL_Y_VCOEF_0A			0x0210
#define DPP_SCL_Y_HCOEF_0A			0x02A0
#define DPP_SCL_C_VCOEF_0A			0x03C0
#define DPP_SCL_C_HCOEF_0A			0x0450

#define DPP_SCL_COEF(_v)			((_v) << 0)
#define DPP_SCL_COEF_MASK			(0x7FF << 0)

/* x: 0 for Y, x: 1 for C*/
#define DPP_H_COEF(n, s, x)			(0x2A0 + (n) * 0x4 +	\
						(s) * 0x24 + (x) * 0x1B0)
#define DPP_V_COEF(n, s, x)			(0x210 + (n) * 0x4 +	\
						(s) * 0x24 + (x) * 0x1B0)

/*TBD: DPP_HDR register set has to be incorporated, refer
 * Olympus/pamir cal file
 */

#endif
