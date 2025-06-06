/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2023 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef PABLO_SFR_MCSC_V13_0_H
#define PABLO_SFR_MCSC_V13_0_H

#include "pablo-hw-api-common.h"
#include "pablo-mmio.h"

enum mcsc_rdma_index {
	MCSC_RDMA_HF = 0,
	MCSC_RDMA_MAX,
};

enum mcsc_wdma_index {
	MCSC_WDMA_W0 = 0,
	MCSC_WDMA_W1,
	MCSC_WDMA_W2,
	MCSC_WDMA_W3,
	MCSC_WDMA_W4,
	MCSC_WDMA_MAX,
};

enum is_mcsc_hwfc_mode {
	MCSC_HWFC_MODE_OFF = 0, /* Both RegionIdx are turned off */
	MCSC_HWFC_MODE_REGION_A_B_PORT, /* Turn on both RegionIdx */
	MCSC_HWFC_MODE_REGION_A_PORT /* Turn on only RegionIdx A port */
};

enum is_mcsc_hwfc_format {
	/* Count up region idx at every 8 line for all plane */
	MCSC_HWFC_FMT_YUV444_YUV422 = 0,
	/* Count up region idx at every 16 line for plane0, others are 8 line */
	MCSC_HWFC_FMT_YUV420
};

enum is_mcsc_input_source {
	UNKNOWN_INPUT = 0,
	OTF_INPUT,
	DMA_INPUT,
};

enum mcsc_common_ctrl_queue_num {
	MCSC_QUEUE0 = 0,
	MCSC_QUEUE1 = 1,
	MCSC_QUEUE_URGENT = 2,
	MCSC_QUEUE_NUM_MAX
};

enum is_mcsc_data_order { MCSC_U_FIRST = 0, MCSC_V_FIRST };

#define INT_PULSE 0
#define INT_LEVEL 1
#define MCSC_TRY_COUNT 20000
#define MCSC_PATH_DRAIN_TOT_LINE 12
#define HBLANK_CYCLE 0x2D

enum mcsc_interrupt_map1 {
	INTR0_MCSC_FRAME_START_INT = 0,
	INTR0_MCSC_FRAME_END_INT = 1,
	INTR0_MCSC_CMDQ_HOLD_INT = 2,
	INTR0_MCSC_SETTING_DONE_INT = 3,
	INTR0_MCSC_C_LOADER_END_INT = 4,
	INTR0_MCSC_COREX_END_INT_0 = 5,
	INTR0_MCSC_COREX_END_INT_1 = 6,
	INTR0_MCSC_ROW_COL_INT = 7,
	INTR0_MCSC_FREEZE_ON_ROW_COL_INT = 8,
	INTR0_MCSC_TRANS_STOP_DONE_INT = 9,
	INTR0_MCSC_CMDQ_ERROR_INT = 10,
	INTR0_MCSC_C_LOADER_ERROR_INT = 11,
	INTR0_MCSC_COREX_ERROR_INT = 12,
	INTR0_MCSC_CINFIFO_0_ERROR_INT = 13,
	INTR0_MCSC_CINFIFO_1_ERROR_INT = 14,
	INTR0_MCSC_CINFIFO_2_ERROR_INT = 15,
	INTR0_MCSC_CINFIFO_3_ERROR_INT = 16,
	INTR0_MCSC_CINFIFO_4_ERROR_INT = 17,
	INTR0_MCSC_CINFIFO_5_ERROR_INT = 18,
	INTR0_MCSC_CINFIFO_6_ERROR_INT = 19,
	INTR0_MCSC_CINFIFO_7_ERROR_INT = 20,
	INTR0_MCSC_COUTFIFO_0_ERROR_INT = 21,
	INTR0_MCSC_COUTFIFO_1_ERROR_INT = 22,
	INTR0_MCSC_COUTFIFO_2_ERROR_INT = 23,
	INTR0_MCSC_COUTFIFO_3_ERROR_INT = 24,
	INTR0_MCSC_COUTFIFO_4_ERROR_INT = 25,
	INTR0_MCSC_COUTFIFO_5_ERROR_INT = 26,
	INTR0_MCSC_COUTFIFO_6_ERROR_INT = 27,
	INTR0_MCSC_COUTFIFO_7_ERROR_INT = 28,
	INTR0_MCSC_VOTF_GLOBAL_ERROR_INT = 29,
	INTR0_MCSC_VOTF_LOST_CONNECTION_INT = 30,
	INTR0_MCSC_OTF_SEQ_ID_ERROR_INT = 31,
	INTR0_MCSC_MAX = 32,
};

#define INT0_EN_MASK                                                                               \
	((0) | (1 << INTR0_MCSC_FRAME_START_INT) | (1 << INTR0_MCSC_FRAME_END_INT) |               \
		(1 << INTR0_MCSC_CMDQ_HOLD_INT) | (1 << INTR0_MCSC_SETTING_DONE_INT) |             \
		(1 << INTR0_MCSC_C_LOADER_END_INT) | (1 << INTR0_MCSC_COREX_END_INT_0) |           \
		(1 << INTR0_MCSC_COREX_END_INT_1) | (1 << INTR0_MCSC_ROW_COL_INT) |                \
		(1 << INTR0_MCSC_FREEZE_ON_ROW_COL_INT) | (1 << INTR0_MCSC_TRANS_STOP_DONE_INT) |  \
		(1 << INTR0_MCSC_CMDQ_ERROR_INT) | (1 << INTR0_MCSC_C_LOADER_ERROR_INT) |          \
		(1 << INTR0_MCSC_COREX_ERROR_INT) | (1 << INTR0_MCSC_CINFIFO_0_ERROR_INT) |        \
		(1 << INTR0_MCSC_CINFIFO_1_ERROR_INT) | (1 << INTR0_MCSC_CINFIFO_2_ERROR_INT) |    \
		(1 << INTR0_MCSC_CINFIFO_3_ERROR_INT) | (1 << INTR0_MCSC_CINFIFO_4_ERROR_INT) |    \
		(1 << INTR0_MCSC_CINFIFO_5_ERROR_INT) | (1 << INTR0_MCSC_CINFIFO_6_ERROR_INT) |    \
		(1 << INTR0_MCSC_CINFIFO_7_ERROR_INT) | (1 << INTR0_MCSC_COUTFIFO_0_ERROR_INT) |   \
		(1 << INTR0_MCSC_COUTFIFO_1_ERROR_INT) | (1 << INTR0_MCSC_COUTFIFO_2_ERROR_INT) |  \
		(1 << INTR0_MCSC_COUTFIFO_3_ERROR_INT) | (1 << INTR0_MCSC_COUTFIFO_4_ERROR_INT) |  \
		(1 << INTR0_MCSC_COUTFIFO_5_ERROR_INT) | (1 << INTR0_MCSC_COUTFIFO_6_ERROR_INT) |  \
		(1 << INTR0_MCSC_COUTFIFO_7_ERROR_INT) | (1 << INTR0_MCSC_VOTF_GLOBAL_ERROR_INT) | \
		(1 << INTR0_MCSC_VOTF_LOST_CONNECTION_INT) |                                       \
		(1 << INTR0_MCSC_OTF_SEQ_ID_ERROR_INT))

#define INT0_ERR_MASK                                                                                                                                                                                                                                                                                                                                                                                                                      \
	((0) /* |(1 << INTR0_MCSC_FRAME_START_INT) */ /* |(1 << INTR0_MCSC_FRAME_END_INT) */ /* |(1 << INTR0_MCSC_CMDQ_HOLD_INT) */ /* |(1 << INTR0_MCSC_SETTING_DONE_INT) */ /* |(1 << INTR0_MCSC_C_LOADER_END_INT) */ /* |(1 << INTR0_MCSC_COREX_END_INT_0) */ /* |(1 << INTR0_MCSC_COREX_END_INT_1) */ /* |(1 << INTR0_MCSC_ROW_COL_INT) */ /* |(1 << INTR0_MCSC_FREEZE_ON_ROW_COL_INT) */ /* |(1 << INTR0_MCSC_TRANS_STOP_DONE_INT) */ \
		| (1 << INTR0_MCSC_CMDQ_ERROR_INT) | (1 << INTR0_MCSC_C_LOADER_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                                \
		(1 << INTR0_MCSC_COREX_ERROR_INT) | (1 << INTR0_MCSC_CINFIFO_0_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                                \
		(1 << INTR0_MCSC_CINFIFO_1_ERROR_INT) | (1 << INTR0_MCSC_CINFIFO_2_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                            \
		(1 << INTR0_MCSC_CINFIFO_3_ERROR_INT) | (1 << INTR0_MCSC_CINFIFO_4_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                            \
		(1 << INTR0_MCSC_CINFIFO_5_ERROR_INT) | (1 << INTR0_MCSC_CINFIFO_6_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                            \
		(1 << INTR0_MCSC_CINFIFO_7_ERROR_INT) | (1 << INTR0_MCSC_COUTFIFO_0_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                           \
		(1 << INTR0_MCSC_COUTFIFO_1_ERROR_INT) | (1 << INTR0_MCSC_COUTFIFO_2_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                          \
		(1 << INTR0_MCSC_COUTFIFO_3_ERROR_INT) | (1 << INTR0_MCSC_COUTFIFO_4_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                          \
		(1 << INTR0_MCSC_COUTFIFO_5_ERROR_INT) | (1 << INTR0_MCSC_COUTFIFO_6_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                          \
		(1 << INTR0_MCSC_COUTFIFO_7_ERROR_INT) | (1 << INTR0_MCSC_VOTF_GLOBAL_ERROR_INT) |                                                                                                                                                                                                                                                                                                                                         \
		(1 << INTR0_MCSC_VOTF_LOST_CONNECTION_INT) |                                                                                                                                                                                                                                                                                                                                                                               \
		(1 << INTR0_MCSC_OTF_SEQ_ID_ERROR_INT))

enum mcsc_interrupt_map2 {
	INTR1_MCSC_DMA_VOTF_LOST_FLUSH = 0,
	/* INTR1_MCSC_1'B0 = 1, */
	INTR1_MCSC_WDMAM0FINISHINT = 2,
	INTR1_MCSC_WDMAM1FINISHINT = 3,
	INTR1_MCSC_WDMAM2FINISHINT = 4,
	INTR1_MCSC_WDMAM3FINISHINT = 5,
	INTR1_MCSC_WDMAM4FINISHINT = 6,
	/* INTR1_MCSC_1'B0 = 7, */
	INTR1_MCSC_WDMAVOTF_LOST_CONNECTION_INT = 8,
	/* INTR1_MCSC_1'B0 = 9, */
	INTR1_MCSC_DJAGFINISHINT = 10,
	INTR1_MCSC_SC0FINISHINT = 11,
	INTR1_MCSC_SC1FINISHINT = 12,
	INTR1_MCSC_SC2FINISHINT = 13,
	INTR1_MCSC_SC3FINISHINT = 14,
	INTR1_MCSC_SC4FINISHINT = 15,
	INTR1_MCSC_PC0FINISHINT = 16,
	INTR1_MCSC_PC1FINISHINT = 17,
	INTR1_MCSC_PC2FINISHINT = 18,
	INTR1_MCSC_PC3FINISHINT = 19,
	INTR1_MCSC_PC4FINISHINT = 20,
	INTR1_MCSC_CONV420_0FINISHINT = 21,
	INTR1_MCSC_CONV420_1FINISHINT = 22,
	INTR1_MCSC_CONV420_2FINISHINT = 23,
	INTR1_MCSC_CONV420_3FINISHINT = 24,
	INTR1_MCSC_CONV420_4FINISHINT = 25,
	INTR1_MCSC_BCHS_0FINISHINT = 26,
	INTR1_MCSC_BCHS_1FINISHINT = 27,
	INTR1_MCSC_BCHS_2FINISHINT = 28,
	INTR1_MCSC_BCHS_3FINISHINT = 29,
	INTR1_MCSC_BCHS_4FINISHINT = 30,
	INTR1_MCSC_MAX = 32,
};

#define INT1_EN_MASK                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
	((0) /* |(1 << INTR1_MCSC_DMA_VOTF_LOST_FLUSH) */ /* |(1 << INTR1_MCSC_1'B0) */ /* |(1 << INTR1_MCSC_WDMAM0FINISHINT) */ /* |(1 << INTR1_MCSC_WDMAM1FINISHINT) */ /* |(1 << INTR1_MCSC_WDMAM2FINISHINT) */ /* |(1 << INTR1_MCSC_WDMAM3FINISHINT) */ /* |(1 << INTR1_MCSC_WDMAM4FINISHINT) */ /* |(1 << INTR1_MCSC_1'B0) */ /* |(1 << INTR1_MCSC_WDMAVOTF_LOST_CONNECTION_INT) */ /* |(1 << INTR1_MCSC_1'B0) */ /* |(1 << INTR1_MCSC_DJAGFINISHINT) */ /* |(1 << INTR1_MCSC_SC0FINISHINT) */ /* |(1 << INTR1_MCSC_SC1FINISHINT) */ /* |(1 << INTR1_MCSC_SC2FINISHINT) */ /* |(1 << INTR1_MCSC_SC3FINISHINT) */ /* |(1 << INTR1_MCSC_SC4FINISHINT) */ /* |(1 << INTR1_MCSC_PC0FINISHINT) */ /* |(1 << INTR1_MCSC_PC1FINISHINT) */ /* |(1 << INTR1_MCSC_PC2FINISHINT) */ /* |(1 << INTR1_MCSC_PC3FINISHINT) */ /* |(1 << INTR1_MCSC_PC4FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_0FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_1FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_2FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_3FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_4FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_0FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_1FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_2FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_3FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_4FINISHINT) */ \
	)

#define INT1_ERR_MASK                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
	((0) /* |(1 << INTR1_MCSC_DMA_VOTF_LOST_FLUSH) */ /* |(1 << INTR1_MCSC_1'B0) */ /* |(1 << INTR1_MCSC_WDMAM0FINISHINT) */ /* |(1 << INTR1_MCSC_WDMAM1FINISHINT) */ /* |(1 << INTR1_MCSC_WDMAM2FINISHINT) */ /* |(1 << INTR1_MCSC_WDMAM3FINISHINT) */ /* |(1 << INTR1_MCSC_WDMAM4FINISHINT) */ /* |(1 << INTR1_MCSC_1'B0) */ /* |(1 << INTR1_MCSC_WDMAVOTF_LOST_CONNECTION_INT) */ /* |(1 << INTR1_MCSC_1'B0) */ /* |(1 << INTR1_MCSC_DJAGFINISHINT) */ /* |(1 << INTR1_MCSC_SC0FINISHINT) */ /* |(1 << INTR1_MCSC_SC1FINISHINT) */ /* |(1 << INTR1_MCSC_SC2FINISHINT) */ /* |(1 << INTR1_MCSC_SC3FINISHINT) */ /* |(1 << INTR1_MCSC_SC4FINISHINT) */ /* |(1 << INTR1_MCSC_PC0FINISHINT) */ /* |(1 << INTR1_MCSC_PC1FINISHINT) */ /* |(1 << INTR1_MCSC_PC2FINISHINT) */ /* |(1 << INTR1_MCSC_PC3FINISHINT) */ /* |(1 << INTR1_MCSC_PC4FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_0FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_1FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_2FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_3FINISHINT) */ /* |(1 << INTR1_MCSC_CONV420_4FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_0FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_1FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_2FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_3FINISHINT) */ /* |(1 << INTR1_MCSC_BCHS_4FINISHINT) */ \
	)

enum mcsc_interrupt_group_map {
	INTR_GRP_MCSC_FRAME_START_INT = 0,
	INTR_GRP_MCSC_FRAME_END_INT = 1,
	INTR_GRP_MCSC_ERROR_CRPT_INT = 2,
	INTR_GRP_MCSC_CMDQ_HOLD_INT = 3,
	INTR_GRP_MCSC_SETTING_DONE_INT = 4,
	INTR_GRP_MCSC_DEBUG_INT = 5,
	INTR_GRP_MCSC_ENABLE_ALL_INT = 7,
	INTR_GRP_MCSC_MAX,
};

#define INT_GRP_MCSC_EN_MASK                                                                       \
	((0) | (1 << INTR_GRP_MCSC_FRAME_START_INT) | (1 << INTR_GRP_MCSC_FRAME_END_INT) |         \
		(1 << INTR_GRP_MCSC_ERROR_CRPT_INT) | (1 << INTR_GRP_MCSC_CMDQ_HOLD_INT) |         \
		(1 << INTR_GRP_MCSC_SETTING_DONE_INT) | (1 << INTR_GRP_MCSC_DEBUG_INT) |           \
		(1 << INTR_GRP_MCSC_ENABLE_ALL_INT))

#define INT_GRP_MCSC_EN_MASK_FRO_FIRST ((0) | (1 << INTR_GRP_MCSC_FRAME_START_INT))

#define INT_GRP_MCSC_EN_MASK_FRO_MIDDLE (0)

#define INT_GRP_MCSC_EN_MASK_FRO_LAST ((0) | (1 << INTR_GRP_MCSC_FRAME_END_INT))

#define MCSC_REG_CNT 1907

static const struct is_reg mcsc_regs[MCSC_REG_CNT] = {
	{ 0x0000, "CMDQ_ENABLE" },
	{ 0x0008, "CMDQ_STOP_CRPT_ENABLE" },
	{ 0x0010, "SW_RESET" },
	{ 0x0014, "SW_CORE_RESET" },
	{ 0x0018, "SW_APB_RESET" },
	{ 0x001c, "TRANS_STOP_REQ" },
	{ 0x0020, "TRANS_STOP_REQ_RDY" },
	{ 0x0028, "IP_APG_MODE" },
	{ 0x002c, "IP_CLOCK_DOWN_MODE" },
	{ 0x0030, "IP_PROCESSING" },
	{ 0x0034, "FORCE_INTERNAL_CLOCK" },
	{ 0x0038, "DEBUG_CLOCK_ENABLE" },
	{ 0x003c, "IP_POST_FRAME_GAP" },
	{ 0x0040, "IP_DRCG_ENABLE" },
	{ 0x0050, "AUTO_IGNORE_INTERRUPT_ENABLE" },
	{ 0x0058, "IP_USE_SW_FINISH_COND" },
	{ 0x005c, "SW_FINISH_COND_ENABLE" },
	{ 0x006c, "IP_CORRUPTED_COND_ENABLE" },
	{ 0x0074, "IP_USE_OTF_PATH_67" },
	{ 0x0078, "IP_USE_OTF_PATH_45" },
	{ 0x007c, "IP_USE_OTF_PATH_23" },
	{ 0x0080, "IP_USE_OTF_PATH_01" },
	{ 0x0084, "IP_USE_CINFIFO_NEW_FRAME_IN" },
	{ 0x00c8, "SECU_CTRL_TZINFO_ICTRL" },
	{ 0x00d0, "ICTRL_CSUB_BASE_ADDR" },
	{ 0x00d4, "ICTRL_CSUB_RECV_TURN_OFF_MSG" },
	{ 0x00d8, "ICTRL_CSUB_RECV_IP_INFO_MSG" },
	{ 0x00dc, "ICTRL_CSUB_CONNECTION_TEST_MSG" },
	{ 0x00e0, "ICTRL_CSUB_MSG_SEND_ENABLE" },
	{ 0x00e4, "ICTRL_CSUB_INT0_EV_ENABLE" },
	{ 0x00e8, "ICTRL_CSUB_INT1_EV_ENABLE" },
	{ 0x00ec, "ICTRL_CSUB_IP_S_EV_ENABLE" },
	{ 0x0210, "YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_WIDTH" },
	{ 0x0214, "YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_HEIGHT" },
	{ 0x0240, "YUV_MAIN_CTRL_FRO_EN" },
	{ 0x0244, "YUV_MAIN_CTRL_INPUT_MONO_EN" },
	{ 0x02a0, "YUV_MAIN_CTRL_CRC_EN" },
	{ 0x02a4, "YUV_MAIN_CTRL_STALL_THROTTLE_CTRL" },
	{ 0x02f4, "YUV_MAIN_CTRL_DJAG_SC_HBI" },
	{ 0x02fc, "YUV_MAIN_CTRL_AXI_TRAFFIC_WR_SEL" },
	{ 0x0400, "CMDQ_QUE_CMD_H" },
	{ 0x0404, "CMDQ_QUE_CMD_M" },
	{ 0x0408, "CMDQ_QUE_CMD_L" },
	{ 0x040c, "CMDQ_ADD_TO_QUEUE_0" },
	{ 0x0418, "CMDQ_AUTO_CONV_ENABLE" },
	{ 0x0440, "CMDQ_LOCK" },
	{ 0x0450, "CMDQ_CTRL_SETSEL_EN" },
	{ 0x0454, "CMDQ_SETSEL" },
	{ 0x0460, "CMDQ_FLUSH_QUEUE_0" },
	{ 0x046c, "CMDQ_SWAP_QUEUE_0" },
	{ 0x0478, "CMDQ_ROTATE_QUEUE_0" },
	{ 0x0484, "CMDQ_HOLD_MARK_QUEUE_0" },
	{ 0x0494, "CMDQ_DEBUG_STATUS_PRE_LOAD" },
	{ 0x049c, "CMDQ_VHD_CONTROL" },
	{ 0x04a0, "CMDQ_FRAME_COUNTER_INC_TYPE" },
	{ 0x04a4, "CMDQ_FRAME_COUNTER_RESET" },
	{ 0x04a8, "CMDQ_FRAME_COUNTER" },
	{ 0x04ac, "CMDQ_FRAME_ID" },
	{ 0x04b0, "CMDQ_QUEUE_0_INFO" },
	{ 0x04b4, "CMDQ_QUEUE_0_RPTR_FOR_DEBUG" },
	{ 0x04b8, "CMDQ_DEBUG_QUE_0_CMD_H" },
	{ 0x04bc, "CMDQ_DEBUG_QUE_0_CMD_M" },
	{ 0x04c0, "CMDQ_DEBUG_QUE_0_CMD_L" },
	{ 0x04ec, "CMDQ_DEBUG_STATUS" },
	{ 0x04f0, "CMDQ_INT" },
	{ 0x04f4, "CMDQ_INT_ENABLE" },
	{ 0x04f8, "CMDQ_INT_STATUS" },
	{ 0x04fc, "CMDQ_INT_CLEAR" },
	{ 0x0500, "C_LOADER_ENABLE" },
	{ 0x0504, "C_LOADER_RESET" },
	{ 0x0508, "C_LOADER_FAST_MODE" },
	{ 0x050c, "C_LOADER_REMAP_EN" },
	{ 0x0510, "C_LOADER_ACCESS_INTERVAL" },
	{ 0x0540, "C_LOADER_REMAP_00_ADDR" },
	{ 0x0544, "C_LOADER_REMAP_01_ADDR" },
	{ 0x0548, "C_LOADER_REMAP_02_ADDR" },
	{ 0x054c, "C_LOADER_REMAP_03_ADDR" },
	{ 0x0550, "C_LOADER_REMAP_04_ADDR" },
	{ 0x0554, "C_LOADER_REMAP_05_ADDR" },
	{ 0x0558, "C_LOADER_REMAP_06_ADDR" },
	{ 0x055c, "C_LOADER_REMAP_07_ADDR" },
	{ 0x0580, "C_LOADER_LOGICAL_OFFSET_EN" },
	{ 0x0584, "C_LOADER_LOGICAL_OFFSET" },
	{ 0x05c0, "C_LOADER_DEBUG_STATUS" },
	{ 0x05c4, "C_LOADER_DEBUG_HEADER_REQ_COUNTER" },
	{ 0x05c8, "C_LOADER_DEBUG_HEADER_APB_COUNTER" },
	{ 0x05e0, "C_LOADER_HEADER_CRC_SEED" },
	{ 0x05e4, "C_LOADER_PAYLOAD_CRC_SEED" },
	{ 0x05f0, "C_LOADER_HEADER_CRC_RESULT" },
	{ 0x05f4, "C_LOADER_PAYLOAD_CRC_RESULT" },
	{ 0x0600, "COREX_ENABLE" },
	{ 0x0604, "COREX_RESET" },
	{ 0x0608, "COREX_FAST_MODE" },
	{ 0x060c, "COREX_UPDATE_TYPE_0" },
	{ 0x0610, "COREX_UPDATE_TYPE_1" },
	{ 0x0614, "COREX_UPDATE_MODE_0" },
	{ 0x0618, "COREX_UPDATE_MODE_1" },
	{ 0x061c, "COREX_START_0" },
	{ 0x0620, "COREX_START_1" },
	{ 0x0624, "COREX_COPY_FROM_IP_0" },
	{ 0x0628, "COREX_COPY_FROM_IP_1" },
	{ 0x062c, "COREX_STATUS_0" },
	{ 0x0630, "COREX_STATUS_1" },
	{ 0x0634, "COREX_PRE_ADDR_CONFIG" },
	{ 0x0638, "COREX_PRE_DATA_CONFIG" },
	{ 0x063c, "COREX_POST_ADDR_CONFIG" },
	{ 0x0640, "COREX_POST_DATA_CONFIG" },
	{ 0x0644, "COREX_PRE_POST_CONFIG_EN" },
	{ 0x0648, "COREX_TYPE_WRITE" },
	{ 0x064c, "COREX_TYPE_WRITE_TRIGGER" },
	{ 0x0650, "COREX_TYPE_READ" },
	{ 0x0654, "COREX_TYPE_READ_OFFSET" },
	{ 0x0658, "COREX_INT" },
	{ 0x065c, "COREX_INT_STATUS" },
	{ 0x0660, "COREX_INT_CLEAR" },
	{ 0x0664, "COREX_INT_ENABLE" },
	{ 0x0800, "INT_REQ_INT0" },
	{ 0x0804, "INT_REQ_INT0_ENABLE" },
	{ 0x0808, "INT_REQ_INT0_STATUS" },
	{ 0x080c, "INT_REQ_INT0_CLEAR" },
	{ 0x0810, "INT_REQ_INT1" },
	{ 0x0814, "INT_REQ_INT1_ENABLE" },
	{ 0x0818, "INT_REQ_INT1_STATUS" },
	{ 0x081c, "INT_REQ_INT1_CLEAR" },
	{ 0x0900, "INT_HIST_CURINT0" },
	{ 0x0904, "INT_HIST_CURINT0_ENABLE" },
	{ 0x0908, "INT_HIST_CURINT0_STATUS" },
	{ 0x090c, "INT_HIST_CURINT1" },
	{ 0x0910, "INT_HIST_CURINT1_ENABLE" },
	{ 0x0914, "INT_HIST_CURINT1_STATUS" },
	{ 0x0918, "INT_HIST_00_FRAME_ID" },
	{ 0x091c, "INT_HIST_00_INT0" },
	{ 0x0920, "INT_HIST_00_INT1" },
	{ 0x0924, "INT_HIST_01_FRAME_ID" },
	{ 0x0928, "INT_HIST_01_INT0" },
	{ 0x092c, "INT_HIST_01_INT1" },
	{ 0x0930, "INT_HIST_02_FRAME_ID" },
	{ 0x0934, "INT_HIST_02_INT0" },
	{ 0x0938, "INT_HIST_02_INT1" },
	{ 0x093c, "INT_HIST_03_FRAME_ID" },
	{ 0x0940, "INT_HIST_03_INT0" },
	{ 0x0944, "INT_HIST_03_INT1" },
	{ 0x0948, "INT_HIST_04_FRAME_ID" },
	{ 0x094c, "INT_HIST_04_INT0" },
	{ 0x0950, "INT_HIST_04_INT1" },
	{ 0x0954, "INT_HIST_05_FRAME_ID" },
	{ 0x0958, "INT_HIST_05_INT0" },
	{ 0x095c, "INT_HIST_05_INT1" },
	{ 0x0960, "INT_HIST_06_FRAME_ID" },
	{ 0x0964, "INT_HIST_06_INT0" },
	{ 0x0968, "INT_HIST_06_INT1" },
	{ 0x096c, "INT_HIST_07_FRAME_ID" },
	{ 0x0970, "INT_HIST_07_INT0" },
	{ 0x0974, "INT_HIST_07_INT1" },
	{ 0x0b00, "SECU_CTRL_SEQID" },
	{ 0x0b10, "SECU_CTRL_TZINFO_SEQID_0" },
	{ 0x0b14, "SECU_CTRL_TZINFO_SEQID_1" },
	{ 0x0b18, "SECU_CTRL_TZINFO_SEQID_2" },
	{ 0x0b1c, "SECU_CTRL_TZINFO_SEQID_3" },
	{ 0x0b20, "SECU_CTRL_TZINFO_SEQID_4" },
	{ 0x0b24, "SECU_CTRL_TZINFO_SEQID_5" },
	{ 0x0b28, "SECU_CTRL_TZINFO_SEQID_6" },
	{ 0x0b2c, "SECU_CTRL_TZINFO_SEQID_7" },
	{ 0x0b58, "SECU_OTF_SEQ_ID_PROT_ENABLE" },
	{ 0x0c00, "PERF_MONITOR_ENABLE" },
	{ 0x0c04, "PERF_MONITOR_CLEAR" },
	{ 0x0c08, "PERF_MONITOR_INT_USER_SEL" },
	{ 0x0c40, "PERF_MONITOR_INT_START" },
	{ 0x0c44, "PERF_MONITOR_INT_END" },
	{ 0x0c48, "PERF_MONITOR_INT_USER" },
	{ 0x0c4c, "PERF_MONITOR_PROCESS_PRE_CONFIG" },
	{ 0x0c50, "PERF_MONITOR_PROCESS_FRAME" },
	{ 0x0d00, "IP_VERSION" },
	{ 0x0d04, "COMMON_CTRL_VERSION" },
	{ 0x0d08, "QCH_STATUS" },
	{ 0x0d0c, "IDLENESS_STATUS" },
	{ 0x0d2c, "DEBUG_COUNTER_SIG_SEL" },
	{ 0x0d30, "DEBUG_COUNTER_0" },
	{ 0x0d34, "DEBUG_COUNTER_1" },
	{ 0x0d38, "DEBUG_COUNTER_2" },
	{ 0x0d3c, "DEBUG_COUNTER_3" },
	{ 0x0d40, "IP_BUSY_MONITOR_0" },
	{ 0x0d44, "IP_BUSY_MONITOR_1" },
	{ 0x0d48, "IP_BUSY_MONITOR_2" },
	{ 0x0d4c, "IP_BUSY_MONITOR_3" },
	{ 0x0d60, "IP_STALL_OUT_STATUS_0" },
	{ 0x0d64, "IP_STALL_OUT_STATUS_1" },
	{ 0x0d68, "IP_STALL_OUT_STATUS_2" },
	{ 0x0d6c, "IP_STALL_OUT_STATUS_3" },
	{ 0x0d80, "STOPEN_CRC_STOP_VALID_COUNT" },
	{ 0x0d88, "SFR_ACCESS_LOG_ENABLE" },
	{ 0x0d8c, "SFR_ACCESS_LOG_CLEAR" },
	{ 0x0d90, "SFR_ACCESS_LOG_0" },
	{ 0x0d94, "SFR_ACCESS_LOG_0_ADDRESS" },
	{ 0x0d98, "SFR_ACCESS_LOG_1" },
	{ 0x0d9c, "SFR_ACCESS_LOG_1_ADDRESS" },
	{ 0x0da0, "SFR_ACCESS_LOG_2" },
	{ 0x0da4, "SFR_ACCESS_LOG_2_ADDRESS" },
	{ 0x0da8, "SFR_ACCESS_LOG_3" },
	{ 0x0dac, "SFR_ACCESS_LOG_3_ADDRESS" },
	{ 0x0dd0, "IP_ROL_RESET" },
	{ 0x0dd4, "IP_ROL_MODE" },
	{ 0x0dd8, "IP_ROL_SELECT" },
	{ 0x0de0, "IP_INT_ON_COL_ROW" },
	{ 0x0de4, "IP_INT_ON_COL_ROW_POS" },
	{ 0x0de8, "FREEZE_FOR_DEBUG" },
	{ 0x0dec, "FREEZE_EXTENSION_ENABLE" },
	{ 0x0df0, "FREEZE_EN" },
	{ 0x0df4, "FREEZE_COL_ROW_POS" },
	{ 0x0df8, "FREEZE_CORRUPTED_ENABLE" },
	{ 0x0e00, "YUV_CINFIFO_ENABLE" },
	{ 0x0e04, "YUV_CINFIFO_CONFIG" },
	{ 0x0e08, "YUV_CINFIFO_STALL_CTRL" },
	{ 0x0e0c, "YUV_CINFIFO_INTERVAL_VBLANK" },
	{ 0x0e10, "YUV_CINFIFO_INTERVALS" },
	{ 0x0e14, "YUV_CINFIFO_STATUS" },
	{ 0x0e18, "YUV_CINFIFO_INPUT_CNT" },
	{ 0x0e1c, "YUV_CINFIFO_STALL_CNT" },
	{ 0x0e20, "YUV_CINFIFO_FIFO_FULLNESS" },
	{ 0x0e40, "YUV_CINFIFO_INT" },
	{ 0x0e44, "YUV_CINFIFO_INT_ENABLE" },
	{ 0x0e48, "YUV_CINFIFO_INT_STATUS" },
	{ 0x0e4c, "YUV_CINFIFO_INT_CLEAR" },
	{ 0x0e50, "YUV_CINFIFO_CORRUPTED_COND_ENABLE" },
	{ 0x0e54, "YUV_CINFIFO_ROL_SELECT" },
	{ 0x0e70, "YUV_CINFIFO_INTERVAL_VBLANK_AR" },
	{ 0x0e74, "YUV_CINFIFO_INTERVAL_HBLANK_AR" },
	{ 0x0e7c, "YUV_CINFIFO_STREAM_CRC" },
	{ 0x0e80, "STAT_CINFIFODUALLAYER_ENABLE" },
	{ 0x0e84, "STAT_CINFIFODUALLAYER_CONFIG" },
	{ 0x0e88, "STAT_CINFIFODUALLAYER_STALL_CTRL" },
	{ 0x0e8c, "STAT_CINFIFODUALLAYER_INTERVAL_VBLANK" },
	{ 0x0e90, "STAT_CINFIFODUALLAYER_INTERVALS" },
	{ 0x0e94, "STAT_CINFIFODUALLAYER_STATUS" },
	{ 0x0e98, "STAT_CINFIFODUALLAYER_INPUT_CNT" },
	{ 0x0e9c, "STAT_CINFIFODUALLAYER_STALL_CNT" },
	{ 0x0ea0, "STAT_CINFIFODUALLAYER_FIFO_FULLNESS" },
	{ 0x0ec0, "STAT_CINFIFODUALLAYER_INT" },
	{ 0x0ec4, "STAT_CINFIFODUALLAYER_INT_ENABLE" },
	{ 0x0ec8, "STAT_CINFIFODUALLAYER_INT_STATUS" },
	{ 0x0ecc, "STAT_CINFIFODUALLAYER_INT_CLEAR" },
	{ 0x0ed0, "STAT_CINFIFODUALLAYER_CORRUPTED_COND_ENABLE" },
	{ 0x0ed4, "STAT_CINFIFODUALLAYER_ROL_SELECT" },
	{ 0x0ef0, "STAT_CINFIFODUALLAYER_INTERVAL_VBLANK_AR" },
	{ 0x0ef4, "STAT_CINFIFODUALLAYER_INTERVAL_HBLANK_AR" },
	{ 0x0efc, "STAT_CINFIFODUALLAYER_STREAM_CRC" },
	{ 0x1600, "STAT_RDMACL_ENABLE" },
	{ 0x1604, "STAT_RDMACL_COMP_CTRL" },
	{ 0x1610, "STAT_RDMACL_DATA_FORMAT" },
	{ 0x1614, "STAT_RDMACL_MONO_MODE" },
	{ 0x1620, "STAT_RDMACL_WIDTH" },
	{ 0x1624, "STAT_RDMACL_HEIGHT" },
	{ 0x1628, "STAT_RDMACL_STRIDE_1P" },
	{ 0x1640, "STAT_RDMACL_MAX_MO" },
	{ 0x1644, "STAT_RDMACL_LINE_GAP" },
	{ 0x164c, "STAT_RDMACL_BUSINFO" },
	{ 0x1650, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0" },
	{ 0x1654, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1" },
	{ 0x1658, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2" },
	{ 0x165c, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3" },
	{ 0x1660, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4" },
	{ 0x1664, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5" },
	{ 0x1668, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6" },
	{ 0x166c, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7" },
	{ 0x1670, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0_LSB_4B" },
	{ 0x1674, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1_LSB_4B" },
	{ 0x1678, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2_LSB_4B" },
	{ 0x167c, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3_LSB_4B" },
	{ 0x1680, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4_LSB_4B" },
	{ 0x1684, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5_LSB_4B" },
	{ 0x1688, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6_LSB_4B" },
	{ 0x168c, "STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7_LSB_4B" },
	{ 0x1790, "STAT_RDMACL_IMG_CRC_1P" },
	{ 0x17b0, "STAT_RDMACL_MON_STATUS_0" },
	{ 0x17b4, "STAT_RDMACL_MON_STATUS_1" },
	{ 0x17b8, "STAT_RDMACL_MON_STATUS_2" },
	{ 0x17bc, "STAT_RDMACL_MON_STATUS_3" },
	{ 0x17e4, "STAT_RDMACL_AXI_DEBUG_CONTROL" },
	{ 0x2000, "YUV_WDMASC_W0_ENABLE" },
	{ 0x2004, "YUV_WDMASC_W0_COMP_CTRL" },
	{ 0x2010, "YUV_WDMASC_W0_DATA_FORMAT" },
	{ 0x2014, "YUV_WDMASC_W0_MONO_CTRL" },
	{ 0x2018, "YUV_WDMASC_W0_COMP_LOSSY_QUALITY_CONTROL" },
	{ 0x2020, "YUV_WDMASC_W0_WIDTH" },
	{ 0x2024, "YUV_WDMASC_W0_HEIGHT" },
	{ 0x2028, "YUV_WDMASC_W0_STRIDE_1P" },
	{ 0x202c, "YUV_WDMASC_W0_STRIDE_2P" },
	{ 0x2030, "YUV_WDMASC_W0_STRIDE_3P" },
	{ 0x2034, "YUV_WDMASC_W0_STRIDE_HEADER_1P" },
	{ 0x2038, "YUV_WDMASC_W0_STRIDE_HEADER_2P" },
	{ 0x203c, "YUV_WDMASC_W0_VOTF_EN" },
	{ 0x2040, "YUV_WDMASC_W0_MAX_MO" },
	{ 0x204c, "YUV_WDMASC_W0_BUSINFO" },
	{ 0x2050, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_0" },
	{ 0x2054, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_1" },
	{ 0x2058, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_2" },
	{ 0x205c, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_3" },
	{ 0x2060, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_4" },
	{ 0x2064, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_5" },
	{ 0x2068, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_6" },
	{ 0x206c, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_7" },
	{ 0x2070, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0" },
	{ 0x2074, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1" },
	{ 0x2078, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2" },
	{ 0x207c, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3" },
	{ 0x2080, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4" },
	{ 0x2084, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5" },
	{ 0x2088, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6" },
	{ 0x208c, "YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7" },
	{ 0x2090, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_0" },
	{ 0x2094, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_1" },
	{ 0x2098, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_2" },
	{ 0x209c, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_3" },
	{ 0x20a0, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_4" },
	{ 0x20a4, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_5" },
	{ 0x20a8, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_6" },
	{ 0x20ac, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_7" },
	{ 0x20b0, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0" },
	{ 0x20b4, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1" },
	{ 0x20b8, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2" },
	{ 0x20bc, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3" },
	{ 0x20c0, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4" },
	{ 0x20c4, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5" },
	{ 0x20c8, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6" },
	{ 0x20cc, "YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7" },
	{ 0x20d0, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_0" },
	{ 0x20d4, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_1" },
	{ 0x20d8, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_2" },
	{ 0x20dc, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_3" },
	{ 0x20e0, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_4" },
	{ 0x20e4, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_5" },
	{ 0x20e8, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_6" },
	{ 0x20ec, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_7" },
	{ 0x20f0, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0" },
	{ 0x20f4, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1" },
	{ 0x20f8, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2" },
	{ 0x20fc, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3" },
	{ 0x2100, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4" },
	{ 0x2104, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5" },
	{ 0x2108, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6" },
	{ 0x210c, "YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7" },
	{ 0x2110, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_0" },
	{ 0x2114, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_1" },
	{ 0x2118, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_2" },
	{ 0x211c, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_3" },
	{ 0x2120, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_4" },
	{ 0x2124, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_5" },
	{ 0x2128, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_6" },
	{ 0x212c, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_7" },
	{ 0x2130, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0" },
	{ 0x2134, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1" },
	{ 0x2138, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2" },
	{ 0x213c, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3" },
	{ 0x2140, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4" },
	{ 0x2144, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5" },
	{ 0x2148, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6" },
	{ 0x214c, "YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7" },
	{ 0x2150, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_0" },
	{ 0x2154, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_1" },
	{ 0x2158, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_2" },
	{ 0x215c, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_3" },
	{ 0x2160, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_4" },
	{ 0x2164, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_5" },
	{ 0x2168, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_6" },
	{ 0x216c, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_7" },
	{ 0x2170, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0" },
	{ 0x2174, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1" },
	{ 0x2178, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2" },
	{ 0x217c, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3" },
	{ 0x2180, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4" },
	{ 0x2184, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5" },
	{ 0x2188, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6" },
	{ 0x218c, "YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7" },
	{ 0x2190, "YUV_WDMASC_W0_IMG_CRC_1P" },
	{ 0x2194, "YUV_WDMASC_W0_IMG_CRC_2P" },
	{ 0x2198, "YUV_WDMASC_W0_IMG_CRC_3P" },
	{ 0x219c, "YUV_WDMASC_W0_HEADER_CRC_1P" },
	{ 0x21a0, "YUV_WDMASC_W0_HEADER_CRC_2P" },
	{ 0x21b0, "YUV_WDMASC_W0_MON_STATUS_0" },
	{ 0x21b4, "YUV_WDMASC_W0_MON_STATUS_1" },
	{ 0x21b8, "YUV_WDMASC_W0_MON_STATUS_2" },
	{ 0x21bc, "YUV_WDMASC_W0_MON_STATUS_3" },
	{ 0x21d0, "YUV_WDMASC_W0_BW_LIMIT_0" },
	{ 0x21d4, "YUV_WDMASC_W0_BW_LIMIT_1" },
	{ 0x21d8, "YUV_WDMASC_W0_BW_LIMIT_2" },
	{ 0x21e0, "YUV_WDMASC_W0_CACHE_CONTROL" },
	{ 0x21e4, "YUV_WDMASC_W0_DEBUG_CONTROL" },
	{ 0x21e8, "YUV_WDMASC_W0_DEBUG_0" },
	{ 0x21ec, "YUV_WDMASC_W0_DEBUG_1" },
	{ 0x21f0, "YUV_WDMASC_W0_DEBUG_2" },
	{ 0x21f8, "YUV_WDMASC_W0_FLIP_CONTROL" },
	{ 0x21fc, "YUV_WDMASC_W0_RGB_ALPHA" },
	{ 0x2200, "YUV_WDMASC_W1_ENABLE" },
	{ 0x2204, "YUV_WDMASC_W1_COMP_CTRL" },
	{ 0x2210, "YUV_WDMASC_W1_DATA_FORMAT" },
	{ 0x2214, "YUV_WDMASC_W1_MONO_CTRL" },
	{ 0x2218, "YUV_WDMASC_W1_COMP_LOSSY_QUALITY_CONTROL" },
	{ 0x2220, "YUV_WDMASC_W1_WIDTH" },
	{ 0x2224, "YUV_WDMASC_W1_HEIGHT" },
	{ 0x2228, "YUV_WDMASC_W1_STRIDE_1P" },
	{ 0x222c, "YUV_WDMASC_W1_STRIDE_2P" },
	{ 0x2230, "YUV_WDMASC_W1_STRIDE_3P" },
	{ 0x2234, "YUV_WDMASC_W1_STRIDE_HEADER_1P" },
	{ 0x2238, "YUV_WDMASC_W1_STRIDE_HEADER_2P" },
	{ 0x223c, "YUV_WDMASC_W1_VOTF_EN" },
	{ 0x2240, "YUV_WDMASC_W1_MAX_MO" },
	{ 0x224c, "YUV_WDMASC_W1_BUSINFO" },
	{ 0x2250, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_0" },
	{ 0x2254, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_1" },
	{ 0x2258, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_2" },
	{ 0x225c, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_3" },
	{ 0x2260, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_4" },
	{ 0x2264, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_5" },
	{ 0x2268, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_6" },
	{ 0x226c, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_7" },
	{ 0x2270, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0" },
	{ 0x2274, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1" },
	{ 0x2278, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2" },
	{ 0x227c, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3" },
	{ 0x2280, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4" },
	{ 0x2284, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5" },
	{ 0x2288, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6" },
	{ 0x228c, "YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7" },
	{ 0x2290, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_0" },
	{ 0x2294, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_1" },
	{ 0x2298, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_2" },
	{ 0x229c, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_3" },
	{ 0x22a0, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_4" },
	{ 0x22a4, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_5" },
	{ 0x22a8, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_6" },
	{ 0x22ac, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_7" },
	{ 0x22b0, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0" },
	{ 0x22b4, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1" },
	{ 0x22b8, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2" },
	{ 0x22bc, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3" },
	{ 0x22c0, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4" },
	{ 0x22c4, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5" },
	{ 0x22c8, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6" },
	{ 0x22cc, "YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7" },
	{ 0x22d0, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_0" },
	{ 0x22d4, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_1" },
	{ 0x22d8, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_2" },
	{ 0x22dc, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_3" },
	{ 0x22e0, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_4" },
	{ 0x22e4, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_5" },
	{ 0x22e8, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_6" },
	{ 0x22ec, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_7" },
	{ 0x22f0, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0" },
	{ 0x22f4, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1" },
	{ 0x22f8, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2" },
	{ 0x22fc, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3" },
	{ 0x2300, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4" },
	{ 0x2304, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5" },
	{ 0x2308, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6" },
	{ 0x230c, "YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7" },
	{ 0x2310, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_0" },
	{ 0x2314, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_1" },
	{ 0x2318, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_2" },
	{ 0x231c, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_3" },
	{ 0x2320, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_4" },
	{ 0x2324, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_5" },
	{ 0x2328, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_6" },
	{ 0x232c, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_7" },
	{ 0x2330, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0" },
	{ 0x2334, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1" },
	{ 0x2338, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2" },
	{ 0x233c, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3" },
	{ 0x2340, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4" },
	{ 0x2344, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5" },
	{ 0x2348, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6" },
	{ 0x234c, "YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7" },
	{ 0x2350, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_0" },
	{ 0x2354, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_1" },
	{ 0x2358, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_2" },
	{ 0x235c, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_3" },
	{ 0x2360, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_4" },
	{ 0x2364, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_5" },
	{ 0x2368, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_6" },
	{ 0x236c, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_7" },
	{ 0x2370, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0" },
	{ 0x2374, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1" },
	{ 0x2378, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2" },
	{ 0x237c, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3" },
	{ 0x2380, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4" },
	{ 0x2384, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5" },
	{ 0x2388, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6" },
	{ 0x238c, "YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7" },
	{ 0x2390, "YUV_WDMASC_W1_IMG_CRC_1P" },
	{ 0x2394, "YUV_WDMASC_W1_IMG_CRC_2P" },
	{ 0x2398, "YUV_WDMASC_W1_IMG_CRC_3P" },
	{ 0x239c, "YUV_WDMASC_W1_HEADER_CRC_1P" },
	{ 0x23a0, "YUV_WDMASC_W1_HEADER_CRC_2P" },
	{ 0x23b0, "YUV_WDMASC_W1_MON_STATUS_0" },
	{ 0x23b4, "YUV_WDMASC_W1_MON_STATUS_1" },
	{ 0x23b8, "YUV_WDMASC_W1_MON_STATUS_2" },
	{ 0x23bc, "YUV_WDMASC_W1_MON_STATUS_3" },
	{ 0x23d0, "YUV_WDMASC_W1_BW_LIMIT_0" },
	{ 0x23d4, "YUV_WDMASC_W1_BW_LIMIT_1" },
	{ 0x23d8, "YUV_WDMASC_W1_BW_LIMIT_2" },
	{ 0x23e0, "YUV_WDMASC_W1_CACHE_CONTROL" },
	{ 0x23e4, "YUV_WDMASC_W1_DEBUG_CONTROL" },
	{ 0x23e8, "YUV_WDMASC_W1_DEBUG_0" },
	{ 0x23ec, "YUV_WDMASC_W1_DEBUG_1" },
	{ 0x23f0, "YUV_WDMASC_W1_DEBUG_2" },
	{ 0x23f8, "YUV_WDMASC_W1_FLIP_CONTROL" },
	{ 0x23fc, "YUV_WDMASC_W1_RGB_ALPHA" },
	{ 0x2400, "YUV_WDMASC_W2_ENABLE" },
	{ 0x2410, "YUV_WDMASC_W2_DATA_FORMAT" },
	{ 0x2414, "YUV_WDMASC_W2_MONO_CTRL" },
	{ 0x2420, "YUV_WDMASC_W2_WIDTH" },
	{ 0x2424, "YUV_WDMASC_W2_HEIGHT" },
	{ 0x2428, "YUV_WDMASC_W2_STRIDE_1P" },
	{ 0x242c, "YUV_WDMASC_W2_STRIDE_2P" },
	{ 0x2430, "YUV_WDMASC_W2_STRIDE_3P" },
	{ 0x243c, "YUV_WDMASC_W2_VOTF_EN" },
	{ 0x2440, "YUV_WDMASC_W2_MAX_MO" },
	{ 0x244c, "YUV_WDMASC_W2_BUSINFO" },
	{ 0x2450, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_0" },
	{ 0x2454, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_1" },
	{ 0x2458, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_2" },
	{ 0x245c, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_3" },
	{ 0x2460, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_4" },
	{ 0x2464, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_5" },
	{ 0x2468, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_6" },
	{ 0x246c, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_7" },
	{ 0x2470, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0" },
	{ 0x2474, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1" },
	{ 0x2478, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2" },
	{ 0x247c, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3" },
	{ 0x2480, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4" },
	{ 0x2484, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5" },
	{ 0x2488, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6" },
	{ 0x248c, "YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7" },
	{ 0x2490, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_0" },
	{ 0x2494, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_1" },
	{ 0x2498, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_2" },
	{ 0x249c, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_3" },
	{ 0x24a0, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_4" },
	{ 0x24a4, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_5" },
	{ 0x24a8, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_6" },
	{ 0x24ac, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_7" },
	{ 0x24b0, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0" },
	{ 0x24b4, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1" },
	{ 0x24b8, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2" },
	{ 0x24bc, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3" },
	{ 0x24c0, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4" },
	{ 0x24c4, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5" },
	{ 0x24c8, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6" },
	{ 0x24cc, "YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7" },
	{ 0x24d0, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_0" },
	{ 0x24d4, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_1" },
	{ 0x24d8, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_2" },
	{ 0x24dc, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_3" },
	{ 0x24e0, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_4" },
	{ 0x24e4, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_5" },
	{ 0x24e8, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_6" },
	{ 0x24ec, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_7" },
	{ 0x24f0, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0" },
	{ 0x24f4, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1" },
	{ 0x24f8, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2" },
	{ 0x24fc, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3" },
	{ 0x2500, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4" },
	{ 0x2504, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5" },
	{ 0x2508, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6" },
	{ 0x250c, "YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7" },
	{ 0x2590, "YUV_WDMASC_W2_IMG_CRC_1P" },
	{ 0x2594, "YUV_WDMASC_W2_IMG_CRC_2P" },
	{ 0x2598, "YUV_WDMASC_W2_IMG_CRC_3P" },
	{ 0x25b0, "YUV_WDMASC_W2_MON_STATUS_0" },
	{ 0x25b4, "YUV_WDMASC_W2_MON_STATUS_1" },
	{ 0x25b8, "YUV_WDMASC_W2_MON_STATUS_2" },
	{ 0x25bc, "YUV_WDMASC_W2_MON_STATUS_3" },
	{ 0x25d0, "YUV_WDMASC_W2_BW_LIMIT_0" },
	{ 0x25d4, "YUV_WDMASC_W2_BW_LIMIT_1" },
	{ 0x25d8, "YUV_WDMASC_W2_BW_LIMIT_2" },
	{ 0x25e0, "YUV_WDMASC_W2_CACHE_CONTROL" },
	{ 0x25e4, "YUV_WDMASC_W2_DEBUG_CONTROL" },
	{ 0x25e8, "YUV_WDMASC_W2_DEBUG_0" },
	{ 0x25ec, "YUV_WDMASC_W2_DEBUG_1" },
	{ 0x25f0, "YUV_WDMASC_W2_DEBUG_2" },
	{ 0x25f8, "YUV_WDMASC_W2_FLIP_CONTROL" },
	{ 0x25fc, "YUV_WDMASC_W2_RGB_ALPHA" },
	{ 0x2600, "YUV_WDMASC_W3_ENABLE" },
	{ 0x2610, "YUV_WDMASC_W3_DATA_FORMAT" },
	{ 0x2614, "YUV_WDMASC_W3_MONO_CTRL" },
	{ 0x2620, "YUV_WDMASC_W3_WIDTH" },
	{ 0x2624, "YUV_WDMASC_W3_HEIGHT" },
	{ 0x2628, "YUV_WDMASC_W3_STRIDE_1P" },
	{ 0x262c, "YUV_WDMASC_W3_STRIDE_2P" },
	{ 0x2630, "YUV_WDMASC_W3_STRIDE_3P" },
	{ 0x263c, "YUV_WDMASC_W3_VOTF_EN" },
	{ 0x2640, "YUV_WDMASC_W3_MAX_MO" },
	{ 0x264c, "YUV_WDMASC_W3_BUSINFO" },
	{ 0x2650, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_0" },
	{ 0x2654, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_1" },
	{ 0x2658, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_2" },
	{ 0x265c, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_3" },
	{ 0x2660, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_4" },
	{ 0x2664, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_5" },
	{ 0x2668, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_6" },
	{ 0x266c, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_7" },
	{ 0x2670, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0" },
	{ 0x2674, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1" },
	{ 0x2678, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2" },
	{ 0x267c, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3" },
	{ 0x2680, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4" },
	{ 0x2684, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5" },
	{ 0x2688, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6" },
	{ 0x268c, "YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7" },
	{ 0x2690, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_0" },
	{ 0x2694, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_1" },
	{ 0x2698, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_2" },
	{ 0x269c, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_3" },
	{ 0x26a0, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_4" },
	{ 0x26a4, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_5" },
	{ 0x26a8, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_6" },
	{ 0x26ac, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_7" },
	{ 0x26b0, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0" },
	{ 0x26b4, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1" },
	{ 0x26b8, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2" },
	{ 0x26bc, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3" },
	{ 0x26c0, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4" },
	{ 0x26c4, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5" },
	{ 0x26c8, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6" },
	{ 0x26cc, "YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7" },
	{ 0x26d0, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_0" },
	{ 0x26d4, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_1" },
	{ 0x26d8, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_2" },
	{ 0x26dc, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_3" },
	{ 0x26e0, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_4" },
	{ 0x26e4, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_5" },
	{ 0x26e8, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_6" },
	{ 0x26ec, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_7" },
	{ 0x26f0, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0" },
	{ 0x26f4, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1" },
	{ 0x26f8, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2" },
	{ 0x26fc, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3" },
	{ 0x2700, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4" },
	{ 0x2704, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5" },
	{ 0x2708, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6" },
	{ 0x270c, "YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7" },
	{ 0x2790, "YUV_WDMASC_W3_IMG_CRC_1P" },
	{ 0x2794, "YUV_WDMASC_W3_IMG_CRC_2P" },
	{ 0x2798, "YUV_WDMASC_W3_IMG_CRC_3P" },
	{ 0x27b0, "YUV_WDMASC_W3_MON_STATUS_0" },
	{ 0x27b4, "YUV_WDMASC_W3_MON_STATUS_1" },
	{ 0x27b8, "YUV_WDMASC_W3_MON_STATUS_2" },
	{ 0x27bc, "YUV_WDMASC_W3_MON_STATUS_3" },
	{ 0x27d0, "YUV_WDMASC_W3_BW_LIMIT_0" },
	{ 0x27d4, "YUV_WDMASC_W3_BW_LIMIT_1" },
	{ 0x27d8, "YUV_WDMASC_W3_BW_LIMIT_2" },
	{ 0x27e0, "YUV_WDMASC_W3_CACHE_CONTROL" },
	{ 0x27e4, "YUV_WDMASC_W3_DEBUG_CONTROL" },
	{ 0x27e8, "YUV_WDMASC_W3_DEBUG_0" },
	{ 0x27ec, "YUV_WDMASC_W3_DEBUG_1" },
	{ 0x27f0, "YUV_WDMASC_W3_DEBUG_2" },
	{ 0x27f8, "YUV_WDMASC_W3_FLIP_CONTROL" },
	{ 0x27fc, "YUV_WDMASC_W3_RGB_ALPHA" },
	{ 0x2800, "YUV_WDMASC_W4_ENABLE" },
	{ 0x2810, "YUV_WDMASC_W4_DATA_FORMAT" },
	{ 0x2814, "YUV_WDMASC_W4_MONO_CTRL" },
	{ 0x2820, "YUV_WDMASC_W4_WIDTH" },
	{ 0x2824, "YUV_WDMASC_W4_HEIGHT" },
	{ 0x2828, "YUV_WDMASC_W4_STRIDE_1P" },
	{ 0x282c, "YUV_WDMASC_W4_STRIDE_2P" },
	{ 0x2830, "YUV_WDMASC_W4_STRIDE_3P" },
	{ 0x283c, "YUV_WDMASC_W4_VOTF_EN" },
	{ 0x2840, "YUV_WDMASC_W4_MAX_MO" },
	{ 0x284c, "YUV_WDMASC_W4_BUSINFO" },
	{ 0x2850, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_0" },
	{ 0x2854, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_1" },
	{ 0x2858, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_2" },
	{ 0x285c, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_3" },
	{ 0x2860, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_4" },
	{ 0x2864, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_5" },
	{ 0x2868, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_6" },
	{ 0x286c, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_7" },
	{ 0x2870, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0" },
	{ 0x2874, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1" },
	{ 0x2878, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2" },
	{ 0x287c, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3" },
	{ 0x2880, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4" },
	{ 0x2884, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5" },
	{ 0x2888, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6" },
	{ 0x288c, "YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7" },
	{ 0x2890, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_0" },
	{ 0x2894, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_1" },
	{ 0x2898, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_2" },
	{ 0x289c, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_3" },
	{ 0x28a0, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_4" },
	{ 0x28a4, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_5" },
	{ 0x28a8, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_6" },
	{ 0x28ac, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_7" },
	{ 0x28b0, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0" },
	{ 0x28b4, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1" },
	{ 0x28b8, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2" },
	{ 0x28bc, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3" },
	{ 0x28c0, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4" },
	{ 0x28c4, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5" },
	{ 0x28c8, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6" },
	{ 0x28cc, "YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7" },
	{ 0x28d0, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_0" },
	{ 0x28d4, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_1" },
	{ 0x28d8, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_2" },
	{ 0x28dc, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_3" },
	{ 0x28e0, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_4" },
	{ 0x28e4, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_5" },
	{ 0x28e8, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_6" },
	{ 0x28ec, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_7" },
	{ 0x28f0, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0" },
	{ 0x28f4, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1" },
	{ 0x28f8, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2" },
	{ 0x28fc, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3" },
	{ 0x2900, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4" },
	{ 0x2904, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5" },
	{ 0x2908, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6" },
	{ 0x290c, "YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7" },
	{ 0x2990, "YUV_WDMASC_W4_IMG_CRC_1P" },
	{ 0x2994, "YUV_WDMASC_W4_IMG_CRC_2P" },
	{ 0x2998, "YUV_WDMASC_W4_IMG_CRC_3P" },
	{ 0x29b0, "YUV_WDMASC_W4_MON_STATUS_0" },
	{ 0x29b4, "YUV_WDMASC_W4_MON_STATUS_1" },
	{ 0x29b8, "YUV_WDMASC_W4_MON_STATUS_2" },
	{ 0x29bc, "YUV_WDMASC_W4_MON_STATUS_3" },
	{ 0x29d0, "YUV_WDMASC_W4_BW_LIMIT_0" },
	{ 0x29d4, "YUV_WDMASC_W4_BW_LIMIT_1" },
	{ 0x29d8, "YUV_WDMASC_W4_BW_LIMIT_2" },
	{ 0x29e0, "YUV_WDMASC_W4_CACHE_CONTROL" },
	{ 0x29e4, "YUV_WDMASC_W4_DEBUG_CONTROL" },
	{ 0x29e8, "YUV_WDMASC_W4_DEBUG_0" },
	{ 0x29ec, "YUV_WDMASC_W4_DEBUG_1" },
	{ 0x29f0, "YUV_WDMASC_W4_DEBUG_2" },
	{ 0x29f8, "YUV_WDMASC_W4_FLIP_CONTROL" },
	{ 0x29fc, "YUV_WDMASC_W4_RGB_ALPHA" },
	{ 0x2f00, "YUV_WDMASC_W0_DITHER" },
	{ 0x2f04, "YUV_WDMASC_W0_RGB_CONV444_WEIGHT" },
	{ 0x2f08, "YUV_WDMASC_W0_PER_SUB_FRAME_EN" },
	{ 0x2f10, "YUV_WDMASC_W0_COMP_SRAM_START_ADDR" },
	{ 0x2f20, "YUV_WDMASC_W1_DITHER" },
	{ 0x2f24, "YUV_WDMASC_W1_RGB_CONV444_WEIGHT" },
	{ 0x2f28, "YUV_WDMASC_W1_PER_SUB_FRAME_EN" },
	{ 0x2f30, "YUV_WDMASC_W1_COMP_SRAM_START_ADDR" },
	{ 0x2f40, "YUV_WDMASC_W2_DITHER" },
	{ 0x2f44, "YUV_WDMASC_W2_RGB_CONV444_WEIGHT" },
	{ 0x2f48, "YUV_WDMASC_W2_PER_SUB_FRAME_EN" },
	{ 0x2f60, "YUV_WDMASC_W3_DITHER" },
	{ 0x2f64, "YUV_WDMASC_W3_RGB_CONV444_WEIGHT" },
	{ 0x2f68, "YUV_WDMASC_W3_PER_SUB_FRAME_EN" },
	{ 0x2f80, "YUV_WDMASC_W4_DITHER" },
	{ 0x2f84, "YUV_WDMASC_W4_RGB_CONV444_WEIGHT" },
	{ 0x2f88, "YUV_WDMASC_W4_PER_SUB_FRAME_EN" },
	{ 0x2fc0, "YUV_WDMASC_RGB_OFFSET" },
	{ 0x2fc4, "YUV_WDMASC_RGB_COEF_0" },
	{ 0x2fc8, "YUV_WDMASC_RGB_COEF_1" },
	{ 0x2fcc, "YUV_WDMASC_RGB_COEF_2" },
	{ 0x2fd0, "YUV_WDMASC_RGB_COEF_3" },
	{ 0x2fd4, "YUV_WDMASC_RGB_COEF_4" },
	{ 0x4000, "YUV_DJAG_CTRL" },
	{ 0x4004, "YUV_DJAG_IMG_SIZE" },
	{ 0x4008, "YUV_DJAG_PS_SRC_POS" },
	{ 0x400c, "YUV_DJAG_PS_SRC_SIZE" },
	{ 0x4010, "YUV_DJAG_PS_DST_SIZE" },
	{ 0x4014, "YUV_DJAGPS_PS_H_RATIO" },
	{ 0x4018, "YUV_DJAGPS_PS_V_RATIO" },
	{ 0x401c, "YUV_DJAGPS_PS_H_INIT_PHASE_OFFSET" },
	{ 0x4020, "YUV_DJAGPS_PS_V_INIT_PHASE_OFFSET" },
	{ 0x4024, "YUV_DJAGPS_PS_ROUND_MODE" },
	{ 0x4030, "YUV_DJAG_PS_STRIP_PRE_DST_SIZE" },
	{ 0x4034, "YUV_DJAG_PS_STRIP_IN_START_POS" },
	{ 0x4038, "YUV_DJAG_OUT_CROP_POS" },
	{ 0x403c, "YUV_DJAG_OUT_CROP_SIZE" },
	{ 0x4040, "YUV_DJAG_XFILTER_DEJAGGING_COEFF" },
	{ 0x4044, "YUV_DJAG_THRES_1X5_MATCHING" },
	{ 0x4048, "YUV_DJAG_THRES_SHOOTING_DETECT_0" },
	{ 0x404c, "YUV_DJAG_THRES_SHOOTING_DETECT_1" },
	{ 0x4050, "YUV_DJAGFILTER_LFSR_SEED_0" },
	{ 0x4054, "YUV_DJAGFILTER_LFSR_SEED_1" },
	{ 0x4058, "YUV_DJAGFILTER_LFSR_SEED_2" },
	{ 0x405c, "YUV_DJAGFILTER_DITHER_VALUE_04" },
	{ 0x4060, "YUV_DJAGFILTER_DITHER_VALUE_58" },
	{ 0x4064, "YUV_DJAGFILTER_DITHER_THRES" },
	{ 0x4068, "YUV_DJAGFILTER_CP_HF_THRES" },
	{ 0x406c, "YUV_DJAG_CP_ARBI" },
	{ 0x4070, "YUV_DJAG_DITHER_WB" },
	{ 0x4124, "YUV_DJAGPS_PS_Y_V_COEFF_0_01" },
	{ 0x4128, "YUV_DJAGPS_PS_Y_V_COEFF_0_23" },
	{ 0x412c, "YUV_DJAGPS_PS_Y_V_COEFF_1_01" },
	{ 0x4130, "YUV_DJAGPS_PS_Y_V_COEFF_1_23" },
	{ 0x4134, "YUV_DJAGPS_PS_Y_V_COEFF_2_01" },
	{ 0x4138, "YUV_DJAGPS_PS_Y_V_COEFF_2_23" },
	{ 0x413c, "YUV_DJAGPS_PS_Y_V_COEFF_3_01" },
	{ 0x4140, "YUV_DJAGPS_PS_Y_V_COEFF_3_23" },
	{ 0x4144, "YUV_DJAGPS_PS_Y_V_COEFF_4_01" },
	{ 0x4148, "YUV_DJAGPS_PS_Y_V_COEFF_4_23" },
	{ 0x414c, "YUV_DJAGPS_PS_Y_V_COEFF_5_01" },
	{ 0x4150, "YUV_DJAGPS_PS_Y_V_COEFF_5_23" },
	{ 0x4154, "YUV_DJAGPS_PS_Y_V_COEFF_6_01" },
	{ 0x4158, "YUV_DJAGPS_PS_Y_V_COEFF_6_23" },
	{ 0x415c, "YUV_DJAGPS_PS_Y_V_COEFF_7_01" },
	{ 0x4160, "YUV_DJAGPS_PS_Y_V_COEFF_7_23" },
	{ 0x4164, "YUV_DJAGPS_PS_Y_V_COEFF_8_01" },
	{ 0x4168, "YUV_DJAGPS_PS_Y_V_COEFF_8_23" },
	{ 0x416c, "YUV_DJAGPS_PS_Y_H_COEFF_0_01" },
	{ 0x4170, "YUV_DJAGPS_PS_Y_H_COEFF_0_23" },
	{ 0x4174, "YUV_DJAGPS_PS_Y_H_COEFF_0_45" },
	{ 0x4178, "YUV_DJAGPS_PS_Y_H_COEFF_0_67" },
	{ 0x417c, "YUV_DJAGPS_PS_Y_H_COEFF_1_01" },
	{ 0x4180, "YUV_DJAGPS_PS_Y_H_COEFF_1_23" },
	{ 0x4184, "YUV_DJAGPS_PS_Y_H_COEFF_1_45" },
	{ 0x4188, "YUV_DJAGPS_PS_Y_H_COEFF_1_67" },
	{ 0x418c, "YUV_DJAGPS_PS_Y_H_COEFF_2_01" },
	{ 0x4190, "YUV_DJAGPS_PS_Y_H_COEFF_2_23" },
	{ 0x4194, "YUV_DJAGPS_PS_Y_H_COEFF_2_45" },
	{ 0x4198, "YUV_DJAGPS_PS_Y_H_COEFF_2_67" },
	{ 0x419c, "YUV_DJAGPS_PS_Y_H_COEFF_3_01" },
	{ 0x41a0, "YUV_DJAGPS_PS_Y_H_COEFF_3_23" },
	{ 0x41a4, "YUV_DJAGPS_PS_Y_H_COEFF_3_45" },
	{ 0x41a8, "YUV_DJAGPS_PS_Y_H_COEFF_3_67" },
	{ 0x41ac, "YUV_DJAGPS_PS_Y_H_COEFF_4_01" },
	{ 0x41b0, "YUV_DJAGPS_PS_Y_H_COEFF_4_23" },
	{ 0x41b4, "YUV_DJAGPS_PS_Y_H_COEFF_4_45" },
	{ 0x41b8, "YUV_DJAGPS_PS_Y_H_COEFF_4_67" },
	{ 0x41bc, "YUV_DJAGPS_PS_Y_H_COEFF_5_01" },
	{ 0x41c0, "YUV_DJAGPS_PS_Y_H_COEFF_5_23" },
	{ 0x41c4, "YUV_DJAGPS_PS_Y_H_COEFF_5_45" },
	{ 0x41c8, "YUV_DJAGPS_PS_Y_H_COEFF_5_67" },
	{ 0x41cc, "YUV_DJAGPS_PS_Y_H_COEFF_6_01" },
	{ 0x41d0, "YUV_DJAGPS_PS_Y_H_COEFF_6_23" },
	{ 0x41d4, "YUV_DJAGPS_PS_Y_H_COEFF_6_45" },
	{ 0x41d8, "YUV_DJAGPS_PS_Y_H_COEFF_6_67" },
	{ 0x41dc, "YUV_DJAGPS_PS_Y_H_COEFF_7_01" },
	{ 0x41e0, "YUV_DJAGPS_PS_Y_H_COEFF_7_23" },
	{ 0x41e4, "YUV_DJAGPS_PS_Y_H_COEFF_7_45" },
	{ 0x41e8, "YUV_DJAGPS_PS_Y_H_COEFF_7_67" },
	{ 0x41ec, "YUV_DJAGPS_PS_Y_H_COEFF_8_01" },
	{ 0x41f0, "YUV_DJAGPS_PS_Y_H_COEFF_8_23" },
	{ 0x41f4, "YUV_DJAGPS_PS_Y_H_COEFF_8_45" },
	{ 0x41f8, "YUV_DJAGPS_PS_Y_H_COEFF_8_67" },
	{ 0x4224, "YUV_DJAGPS_PS_UV_V_COEFF_0_01" },
	{ 0x4228, "YUV_DJAGPS_PS_UV_V_COEFF_0_23" },
	{ 0x422c, "YUV_DJAGPS_PS_UV_V_COEFF_1_01" },
	{ 0x4230, "YUV_DJAGPS_PS_UV_V_COEFF_1_23" },
	{ 0x4234, "YUV_DJAGPS_PS_UV_V_COEFF_2_01" },
	{ 0x4238, "YUV_DJAGPS_PS_UV_V_COEFF_2_23" },
	{ 0x423c, "YUV_DJAGPS_PS_UV_V_COEFF_3_01" },
	{ 0x4240, "YUV_DJAGPS_PS_UV_V_COEFF_3_23" },
	{ 0x4244, "YUV_DJAGPS_PS_UV_V_COEFF_4_01" },
	{ 0x4248, "YUV_DJAGPS_PS_UV_V_COEFF_4_23" },
	{ 0x424c, "YUV_DJAGPS_PS_UV_V_COEFF_5_01" },
	{ 0x4250, "YUV_DJAGPS_PS_UV_V_COEFF_5_23" },
	{ 0x4254, "YUV_DJAGPS_PS_UV_V_COEFF_6_01" },
	{ 0x4258, "YUV_DJAGPS_PS_UV_V_COEFF_6_23" },
	{ 0x425c, "YUV_DJAGPS_PS_UV_V_COEFF_7_01" },
	{ 0x4260, "YUV_DJAGPS_PS_UV_V_COEFF_7_23" },
	{ 0x4264, "YUV_DJAGPS_PS_UV_V_COEFF_8_01" },
	{ 0x4268, "YUV_DJAGPS_PS_UV_V_COEFF_8_23" },
	{ 0x426c, "YUV_DJAGPS_PS_UV_H_COEFF_0_01" },
	{ 0x4270, "YUV_DJAGPS_PS_UV_H_COEFF_0_23" },
	{ 0x427c, "YUV_DJAGPS_PS_UV_H_COEFF_1_01" },
	{ 0x4280, "YUV_DJAGPS_PS_UV_H_COEFF_1_23" },
	{ 0x428c, "YUV_DJAGPS_PS_UV_H_COEFF_2_01" },
	{ 0x4290, "YUV_DJAGPS_PS_UV_H_COEFF_2_23" },
	{ 0x429c, "YUV_DJAGPS_PS_UV_H_COEFF_3_01" },
	{ 0x42a0, "YUV_DJAGPS_PS_UV_H_COEFF_3_23" },
	{ 0x42ac, "YUV_DJAGPS_PS_UV_H_COEFF_4_01" },
	{ 0x42b0, "YUV_DJAGPS_PS_UV_H_COEFF_4_23" },
	{ 0x42bc, "YUV_DJAGPS_PS_UV_H_COEFF_5_01" },
	{ 0x42c0, "YUV_DJAGPS_PS_UV_H_COEFF_5_23" },
	{ 0x42cc, "YUV_DJAGPS_PS_UV_H_COEFF_6_01" },
	{ 0x42d0, "YUV_DJAGPS_PS_UV_H_COEFF_6_23" },
	{ 0x42dc, "YUV_DJAGPS_PS_UV_H_COEFF_7_01" },
	{ 0x42e0, "YUV_DJAGPS_PS_UV_H_COEFF_7_23" },
	{ 0x42ec, "YUV_DJAGPS_PS_UV_H_COEFF_8_01" },
	{ 0x42f0, "YUV_DJAGPS_PS_UV_H_COEFF_8_23" },
	{ 0x4300, "YUV_DJAGPREFILTER_CTRL" },
	{ 0x4304, "YUV_DJAGPREFILTER_IMG_SIZE" },
	{ 0x4308, "YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF" },
	{ 0x430c, "YUV_DJAGPREFILTER_THRES_1X5_MATCHING" },
	{ 0x4310, "YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_0" },
	{ 0x4314, "YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_1" },
	{ 0x4318, "YUV_DJAGPREFILTER_FILTER_LFSR_SEED_0" },
	{ 0x431c, "YUV_DJAGPREFILTER_FILTER_LFSR_SEED_1" },
	{ 0x4320, "YUV_DJAGPREFILTER_FILTER_LFSR_SEED_2" },
	{ 0x4324, "YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_04" },
	{ 0x4328, "YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_58" },
	{ 0x432c, "YUV_DJAGPREFILTER_FILTER_DITHER_THRES" },
	{ 0x4330, "YUV_DJAGPREFILTER_FILTER_CP_HF_THRES" },
	{ 0x4334, "YUV_DJAGPREFILTER_CP_ARBI" },
	{ 0x4338, "YUV_DJAGPREFILTER_DITHER_WB" },
	{ 0x433c, "YUV_DJAGPREFILTER_HARRIS_DET" },
	{ 0x4340, "YUV_DJAGPREFILTER_BILATERAL_C" },
	{ 0x4400, "YUV_DUALLAYERRECOMP_CTRL" },
	{ 0x4404, "YUV_DUALLAYERRECOMP_WEIGHT" },
	{ 0x4408, "YUV_DUALLAYERRECOMP_IMG_SIZE" },
	{ 0x440c, "YUV_DUALLAYERRECOMP_IN_CROP_POS" },
	{ 0x4410, "YUV_DUALLAYERRECOMP_IN_CROP_SIZE" },
	{ 0x4414, "YUV_DUALLAYERRECOMP_RADIAL_CENTER" },
	{ 0x4418, "YUV_DUALLAYERRECOMP_RADIAL_BINNING" },
	{ 0x441c, "YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_A" },
	{ 0x4420, "YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_B" },
	{ 0x4424, "YUV_DUALLAYERRECOMP_RADIAL_GAIN_ENABLE" },
	{ 0x4428, "YUV_DUALLAYERRECOMP_RADIAL_GAIN_INCREMENT" },
	{ 0x442c, "YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_SCALE_SHIFT_ADDER" },
	{ 0x4430, "YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_A" },
	{ 0x4434, "YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_B" },
	{ 0x44fc, "YUV_DUALLAYERRECOMP_STREAM_CRC" },
	{ 0x5000, "YUV_POLY_SC0_CTRL" },
	{ 0x5004, "YUV_POLY_SC0_SRC_POS" },
	{ 0x5008, "YUV_POLY_SC0_SRC_SIZE" },
	{ 0x500c, "YUV_POLY_SC0_DST_SIZE" },
	{ 0x5010, "YUV_POLY_SC0_H_RATIO" },
	{ 0x5014, "YUV_POLY_SC0_V_RATIO" },
	{ 0x5018, "YUV_POLY_SC0_H_INIT_PHASE_OFFSET" },
	{ 0x501c, "YUV_POLY_SC0_V_INIT_PHASE_OFFSET" },
	{ 0x5020, "YUV_POLY_SC0_ROUND_MODE" },
	{ 0x5024, "YUV_POLY_SC0_Y_V_COEFF_0_01" },
	{ 0x5028, "YUV_POLY_SC0_Y_V_COEFF_0_23" },
	{ 0x502c, "YUV_POLY_SC0_Y_V_COEFF_1_01" },
	{ 0x5030, "YUV_POLY_SC0_Y_V_COEFF_1_23" },
	{ 0x5034, "YUV_POLY_SC0_Y_V_COEFF_2_01" },
	{ 0x5038, "YUV_POLY_SC0_Y_V_COEFF_2_23" },
	{ 0x503c, "YUV_POLY_SC0_Y_V_COEFF_3_01" },
	{ 0x5040, "YUV_POLY_SC0_Y_V_COEFF_3_23" },
	{ 0x5044, "YUV_POLY_SC0_Y_V_COEFF_4_01" },
	{ 0x5048, "YUV_POLY_SC0_Y_V_COEFF_4_23" },
	{ 0x504c, "YUV_POLY_SC0_Y_V_COEFF_5_01" },
	{ 0x5050, "YUV_POLY_SC0_Y_V_COEFF_5_23" },
	{ 0x5054, "YUV_POLY_SC0_Y_V_COEFF_6_01" },
	{ 0x5058, "YUV_POLY_SC0_Y_V_COEFF_6_23" },
	{ 0x505c, "YUV_POLY_SC0_Y_V_COEFF_7_01" },
	{ 0x5060, "YUV_POLY_SC0_Y_V_COEFF_7_23" },
	{ 0x5064, "YUV_POLY_SC0_Y_V_COEFF_8_01" },
	{ 0x5068, "YUV_POLY_SC0_Y_V_COEFF_8_23" },
	{ 0x506c, "YUV_POLY_SC0_Y_H_COEFF_0_01" },
	{ 0x5070, "YUV_POLY_SC0_Y_H_COEFF_0_23" },
	{ 0x5074, "YUV_POLY_SC0_Y_H_COEFF_0_45" },
	{ 0x5078, "YUV_POLY_SC0_Y_H_COEFF_0_67" },
	{ 0x507c, "YUV_POLY_SC0_Y_H_COEFF_1_01" },
	{ 0x5080, "YUV_POLY_SC0_Y_H_COEFF_1_23" },
	{ 0x5084, "YUV_POLY_SC0_Y_H_COEFF_1_45" },
	{ 0x5088, "YUV_POLY_SC0_Y_H_COEFF_1_67" },
	{ 0x508c, "YUV_POLY_SC0_Y_H_COEFF_2_01" },
	{ 0x5090, "YUV_POLY_SC0_Y_H_COEFF_2_23" },
	{ 0x5094, "YUV_POLY_SC0_Y_H_COEFF_2_45" },
	{ 0x5098, "YUV_POLY_SC0_Y_H_COEFF_2_67" },
	{ 0x509c, "YUV_POLY_SC0_Y_H_COEFF_3_01" },
	{ 0x50a0, "YUV_POLY_SC0_Y_H_COEFF_3_23" },
	{ 0x50a4, "YUV_POLY_SC0_Y_H_COEFF_3_45" },
	{ 0x50a8, "YUV_POLY_SC0_Y_H_COEFF_3_67" },
	{ 0x50ac, "YUV_POLY_SC0_Y_H_COEFF_4_01" },
	{ 0x50b0, "YUV_POLY_SC0_Y_H_COEFF_4_23" },
	{ 0x50b4, "YUV_POLY_SC0_Y_H_COEFF_4_45" },
	{ 0x50b8, "YUV_POLY_SC0_Y_H_COEFF_4_67" },
	{ 0x50bc, "YUV_POLY_SC0_Y_H_COEFF_5_01" },
	{ 0x50c0, "YUV_POLY_SC0_Y_H_COEFF_5_23" },
	{ 0x50c4, "YUV_POLY_SC0_Y_H_COEFF_5_45" },
	{ 0x50c8, "YUV_POLY_SC0_Y_H_COEFF_5_67" },
	{ 0x50cc, "YUV_POLY_SC0_Y_H_COEFF_6_01" },
	{ 0x50d0, "YUV_POLY_SC0_Y_H_COEFF_6_23" },
	{ 0x50d4, "YUV_POLY_SC0_Y_H_COEFF_6_45" },
	{ 0x50d8, "YUV_POLY_SC0_Y_H_COEFF_6_67" },
	{ 0x50dc, "YUV_POLY_SC0_Y_H_COEFF_7_01" },
	{ 0x50e0, "YUV_POLY_SC0_Y_H_COEFF_7_23" },
	{ 0x50e4, "YUV_POLY_SC0_Y_H_COEFF_7_45" },
	{ 0x50e8, "YUV_POLY_SC0_Y_H_COEFF_7_67" },
	{ 0x50ec, "YUV_POLY_SC0_Y_H_COEFF_8_01" },
	{ 0x50f0, "YUV_POLY_SC0_Y_H_COEFF_8_23" },
	{ 0x50f4, "YUV_POLY_SC0_Y_H_COEFF_8_45" },
	{ 0x50f8, "YUV_POLY_SC0_Y_H_COEFF_8_67" },
	{ 0x5100, "YUV_POLY_SC1_CTRL" },
	{ 0x5104, "YUV_POLY_SC1_SRC_POS" },
	{ 0x5108, "YUV_POLY_SC1_SRC_SIZE" },
	{ 0x510c, "YUV_POLY_SC1_DST_SIZE" },
	{ 0x5110, "YUV_POLY_SC1_H_RATIO" },
	{ 0x5114, "YUV_POLY_SC1_V_RATIO" },
	{ 0x5118, "YUV_POLY_SC1_H_INIT_PHASE_OFFSET" },
	{ 0x511c, "YUV_POLY_SC1_V_INIT_PHASE_OFFSET" },
	{ 0x5120, "YUV_POLY_SC1_ROUND_MODE" },
	{ 0x5124, "YUV_POLY_SC1_Y_V_COEFF_0_01" },
	{ 0x5128, "YUV_POLY_SC1_Y_V_COEFF_0_23" },
	{ 0x512c, "YUV_POLY_SC1_Y_V_COEFF_1_01" },
	{ 0x5130, "YUV_POLY_SC1_Y_V_COEFF_1_23" },
	{ 0x5134, "YUV_POLY_SC1_Y_V_COEFF_2_01" },
	{ 0x5138, "YUV_POLY_SC1_Y_V_COEFF_2_23" },
	{ 0x513c, "YUV_POLY_SC1_Y_V_COEFF_3_01" },
	{ 0x5140, "YUV_POLY_SC1_Y_V_COEFF_3_23" },
	{ 0x5144, "YUV_POLY_SC1_Y_V_COEFF_4_01" },
	{ 0x5148, "YUV_POLY_SC1_Y_V_COEFF_4_23" },
	{ 0x514c, "YUV_POLY_SC1_Y_V_COEFF_5_01" },
	{ 0x5150, "YUV_POLY_SC1_Y_V_COEFF_5_23" },
	{ 0x5154, "YUV_POLY_SC1_Y_V_COEFF_6_01" },
	{ 0x5158, "YUV_POLY_SC1_Y_V_COEFF_6_23" },
	{ 0x515c, "YUV_POLY_SC1_Y_V_COEFF_7_01" },
	{ 0x5160, "YUV_POLY_SC1_Y_V_COEFF_7_23" },
	{ 0x5164, "YUV_POLY_SC1_Y_V_COEFF_8_01" },
	{ 0x5168, "YUV_POLY_SC1_Y_V_COEFF_8_23" },
	{ 0x516c, "YUV_POLY_SC1_Y_H_COEFF_0_01" },
	{ 0x5170, "YUV_POLY_SC1_Y_H_COEFF_0_23" },
	{ 0x5174, "YUV_POLY_SC1_Y_H_COEFF_0_45" },
	{ 0x5178, "YUV_POLY_SC1_Y_H_COEFF_0_67" },
	{ 0x517c, "YUV_POLY_SC1_Y_H_COEFF_1_01" },
	{ 0x5180, "YUV_POLY_SC1_Y_H_COEFF_1_23" },
	{ 0x5184, "YUV_POLY_SC1_Y_H_COEFF_1_45" },
	{ 0x5188, "YUV_POLY_SC1_Y_H_COEFF_1_67" },
	{ 0x518c, "YUV_POLY_SC1_Y_H_COEFF_2_01" },
	{ 0x5190, "YUV_POLY_SC1_Y_H_COEFF_2_23" },
	{ 0x5194, "YUV_POLY_SC1_Y_H_COEFF_2_45" },
	{ 0x5198, "YUV_POLY_SC1_Y_H_COEFF_2_67" },
	{ 0x519c, "YUV_POLY_SC1_Y_H_COEFF_3_01" },
	{ 0x51a0, "YUV_POLY_SC1_Y_H_COEFF_3_23" },
	{ 0x51a4, "YUV_POLY_SC1_Y_H_COEFF_3_45" },
	{ 0x51a8, "YUV_POLY_SC1_Y_H_COEFF_3_67" },
	{ 0x51ac, "YUV_POLY_SC1_Y_H_COEFF_4_01" },
	{ 0x51b0, "YUV_POLY_SC1_Y_H_COEFF_4_23" },
	{ 0x51b4, "YUV_POLY_SC1_Y_H_COEFF_4_45" },
	{ 0x51b8, "YUV_POLY_SC1_Y_H_COEFF_4_67" },
	{ 0x51bc, "YUV_POLY_SC1_Y_H_COEFF_5_01" },
	{ 0x51c0, "YUV_POLY_SC1_Y_H_COEFF_5_23" },
	{ 0x51c4, "YUV_POLY_SC1_Y_H_COEFF_5_45" },
	{ 0x51c8, "YUV_POLY_SC1_Y_H_COEFF_5_67" },
	{ 0x51cc, "YUV_POLY_SC1_Y_H_COEFF_6_01" },
	{ 0x51d0, "YUV_POLY_SC1_Y_H_COEFF_6_23" },
	{ 0x51d4, "YUV_POLY_SC1_Y_H_COEFF_6_45" },
	{ 0x51d8, "YUV_POLY_SC1_Y_H_COEFF_6_67" },
	{ 0x51dc, "YUV_POLY_SC1_Y_H_COEFF_7_01" },
	{ 0x51e0, "YUV_POLY_SC1_Y_H_COEFF_7_23" },
	{ 0x51e4, "YUV_POLY_SC1_Y_H_COEFF_7_45" },
	{ 0x51e8, "YUV_POLY_SC1_Y_H_COEFF_7_67" },
	{ 0x51ec, "YUV_POLY_SC1_Y_H_COEFF_8_01" },
	{ 0x51f0, "YUV_POLY_SC1_Y_H_COEFF_8_23" },
	{ 0x51f4, "YUV_POLY_SC1_Y_H_COEFF_8_45" },
	{ 0x51f8, "YUV_POLY_SC1_Y_H_COEFF_8_67" },
	{ 0x5200, "YUV_POLY_SC2_CTRL" },
	{ 0x5204, "YUV_POLY_SC2_SRC_POS" },
	{ 0x5208, "YUV_POLY_SC2_SRC_SIZE" },
	{ 0x520c, "YUV_POLY_SC2_DST_SIZE" },
	{ 0x5210, "YUV_POLY_SC2_H_RATIO" },
	{ 0x5214, "YUV_POLY_SC2_V_RATIO" },
	{ 0x5218, "YUV_POLY_SC2_H_INIT_PHASE_OFFSET" },
	{ 0x521c, "YUV_POLY_SC2_V_INIT_PHASE_OFFSET" },
	{ 0x5220, "YUV_POLY_SC2_ROUND_MODE" },
	{ 0x5224, "YUV_POLY_SC2_Y_V_COEFF_0_01" },
	{ 0x5228, "YUV_POLY_SC2_Y_V_COEFF_0_23" },
	{ 0x522c, "YUV_POLY_SC2_Y_V_COEFF_1_01" },
	{ 0x5230, "YUV_POLY_SC2_Y_V_COEFF_1_23" },
	{ 0x5234, "YUV_POLY_SC2_Y_V_COEFF_2_01" },
	{ 0x5238, "YUV_POLY_SC2_Y_V_COEFF_2_23" },
	{ 0x523c, "YUV_POLY_SC2_Y_V_COEFF_3_01" },
	{ 0x5240, "YUV_POLY_SC2_Y_V_COEFF_3_23" },
	{ 0x5244, "YUV_POLY_SC2_Y_V_COEFF_4_01" },
	{ 0x5248, "YUV_POLY_SC2_Y_V_COEFF_4_23" },
	{ 0x524c, "YUV_POLY_SC2_Y_V_COEFF_5_01" },
	{ 0x5250, "YUV_POLY_SC2_Y_V_COEFF_5_23" },
	{ 0x5254, "YUV_POLY_SC2_Y_V_COEFF_6_01" },
	{ 0x5258, "YUV_POLY_SC2_Y_V_COEFF_6_23" },
	{ 0x525c, "YUV_POLY_SC2_Y_V_COEFF_7_01" },
	{ 0x5260, "YUV_POLY_SC2_Y_V_COEFF_7_23" },
	{ 0x5264, "YUV_POLY_SC2_Y_V_COEFF_8_01" },
	{ 0x5268, "YUV_POLY_SC2_Y_V_COEFF_8_23" },
	{ 0x526c, "YUV_POLY_SC2_Y_H_COEFF_0_01" },
	{ 0x5270, "YUV_POLY_SC2_Y_H_COEFF_0_23" },
	{ 0x5274, "YUV_POLY_SC2_Y_H_COEFF_0_45" },
	{ 0x5278, "YUV_POLY_SC2_Y_H_COEFF_0_67" },
	{ 0x527c, "YUV_POLY_SC2_Y_H_COEFF_1_01" },
	{ 0x5280, "YUV_POLY_SC2_Y_H_COEFF_1_23" },
	{ 0x5284, "YUV_POLY_SC2_Y_H_COEFF_1_45" },
	{ 0x5288, "YUV_POLY_SC2_Y_H_COEFF_1_67" },
	{ 0x528c, "YUV_POLY_SC2_Y_H_COEFF_2_01" },
	{ 0x5290, "YUV_POLY_SC2_Y_H_COEFF_2_23" },
	{ 0x5294, "YUV_POLY_SC2_Y_H_COEFF_2_45" },
	{ 0x5298, "YUV_POLY_SC2_Y_H_COEFF_2_67" },
	{ 0x529c, "YUV_POLY_SC2_Y_H_COEFF_3_01" },
	{ 0x52a0, "YUV_POLY_SC2_Y_H_COEFF_3_23" },
	{ 0x52a4, "YUV_POLY_SC2_Y_H_COEFF_3_45" },
	{ 0x52a8, "YUV_POLY_SC2_Y_H_COEFF_3_67" },
	{ 0x52ac, "YUV_POLY_SC2_Y_H_COEFF_4_01" },
	{ 0x52b0, "YUV_POLY_SC2_Y_H_COEFF_4_23" },
	{ 0x52b4, "YUV_POLY_SC2_Y_H_COEFF_4_45" },
	{ 0x52b8, "YUV_POLY_SC2_Y_H_COEFF_4_67" },
	{ 0x52bc, "YUV_POLY_SC2_Y_H_COEFF_5_01" },
	{ 0x52c0, "YUV_POLY_SC2_Y_H_COEFF_5_23" },
	{ 0x52c4, "YUV_POLY_SC2_Y_H_COEFF_5_45" },
	{ 0x52c8, "YUV_POLY_SC2_Y_H_COEFF_5_67" },
	{ 0x52cc, "YUV_POLY_SC2_Y_H_COEFF_6_01" },
	{ 0x52d0, "YUV_POLY_SC2_Y_H_COEFF_6_23" },
	{ 0x52d4, "YUV_POLY_SC2_Y_H_COEFF_6_45" },
	{ 0x52d8, "YUV_POLY_SC2_Y_H_COEFF_6_67" },
	{ 0x52dc, "YUV_POLY_SC2_Y_H_COEFF_7_01" },
	{ 0x52e0, "YUV_POLY_SC2_Y_H_COEFF_7_23" },
	{ 0x52e4, "YUV_POLY_SC2_Y_H_COEFF_7_45" },
	{ 0x52e8, "YUV_POLY_SC2_Y_H_COEFF_7_67" },
	{ 0x52ec, "YUV_POLY_SC2_Y_H_COEFF_8_01" },
	{ 0x52f0, "YUV_POLY_SC2_Y_H_COEFF_8_23" },
	{ 0x52f4, "YUV_POLY_SC2_Y_H_COEFF_8_45" },
	{ 0x52f8, "YUV_POLY_SC2_Y_H_COEFF_8_67" },
	{ 0x5300, "YUV_POLY_SC3_CTRL" },
	{ 0x5304, "YUV_POLY_SC3_SRC_POS" },
	{ 0x5308, "YUV_POLY_SC3_SRC_SIZE" },
	{ 0x530c, "YUV_POLY_SC3_DST_SIZE" },
	{ 0x5310, "YUV_POLY_SC3_H_RATIO" },
	{ 0x5314, "YUV_POLY_SC3_V_RATIO" },
	{ 0x5318, "YUV_POLY_SC3_H_INIT_PHASE_OFFSET" },
	{ 0x531c, "YUV_POLY_SC3_V_INIT_PHASE_OFFSET" },
	{ 0x5320, "YUV_POLY_SC3_ROUND_MODE" },
	{ 0x5324, "YUV_POLY_SC3_Y_V_COEFF_0_01" },
	{ 0x5328, "YUV_POLY_SC3_Y_V_COEFF_0_23" },
	{ 0x532c, "YUV_POLY_SC3_Y_V_COEFF_1_01" },
	{ 0x5330, "YUV_POLY_SC3_Y_V_COEFF_1_23" },
	{ 0x5334, "YUV_POLY_SC3_Y_V_COEFF_2_01" },
	{ 0x5338, "YUV_POLY_SC3_Y_V_COEFF_2_23" },
	{ 0x533c, "YUV_POLY_SC3_Y_V_COEFF_3_01" },
	{ 0x5340, "YUV_POLY_SC3_Y_V_COEFF_3_23" },
	{ 0x5344, "YUV_POLY_SC3_Y_V_COEFF_4_01" },
	{ 0x5348, "YUV_POLY_SC3_Y_V_COEFF_4_23" },
	{ 0x534c, "YUV_POLY_SC3_Y_V_COEFF_5_01" },
	{ 0x5350, "YUV_POLY_SC3_Y_V_COEFF_5_23" },
	{ 0x5354, "YUV_POLY_SC3_Y_V_COEFF_6_01" },
	{ 0x5358, "YUV_POLY_SC3_Y_V_COEFF_6_23" },
	{ 0x535c, "YUV_POLY_SC3_Y_V_COEFF_7_01" },
	{ 0x5360, "YUV_POLY_SC3_Y_V_COEFF_7_23" },
	{ 0x5364, "YUV_POLY_SC3_Y_V_COEFF_8_01" },
	{ 0x5368, "YUV_POLY_SC3_Y_V_COEFF_8_23" },
	{ 0x536c, "YUV_POLY_SC3_Y_H_COEFF_0_01" },
	{ 0x5370, "YUV_POLY_SC3_Y_H_COEFF_0_23" },
	{ 0x5374, "YUV_POLY_SC3_Y_H_COEFF_0_45" },
	{ 0x5378, "YUV_POLY_SC3_Y_H_COEFF_0_67" },
	{ 0x537c, "YUV_POLY_SC3_Y_H_COEFF_1_01" },
	{ 0x5380, "YUV_POLY_SC3_Y_H_COEFF_1_23" },
	{ 0x5384, "YUV_POLY_SC3_Y_H_COEFF_1_45" },
	{ 0x5388, "YUV_POLY_SC3_Y_H_COEFF_1_67" },
	{ 0x538c, "YUV_POLY_SC3_Y_H_COEFF_2_01" },
	{ 0x5390, "YUV_POLY_SC3_Y_H_COEFF_2_23" },
	{ 0x5394, "YUV_POLY_SC3_Y_H_COEFF_2_45" },
	{ 0x5398, "YUV_POLY_SC3_Y_H_COEFF_2_67" },
	{ 0x539c, "YUV_POLY_SC3_Y_H_COEFF_3_01" },
	{ 0x53a0, "YUV_POLY_SC3_Y_H_COEFF_3_23" },
	{ 0x53a4, "YUV_POLY_SC3_Y_H_COEFF_3_45" },
	{ 0x53a8, "YUV_POLY_SC3_Y_H_COEFF_3_67" },
	{ 0x53ac, "YUV_POLY_SC3_Y_H_COEFF_4_01" },
	{ 0x53b0, "YUV_POLY_SC3_Y_H_COEFF_4_23" },
	{ 0x53b4, "YUV_POLY_SC3_Y_H_COEFF_4_45" },
	{ 0x53b8, "YUV_POLY_SC3_Y_H_COEFF_4_67" },
	{ 0x53bc, "YUV_POLY_SC3_Y_H_COEFF_5_01" },
	{ 0x53c0, "YUV_POLY_SC3_Y_H_COEFF_5_23" },
	{ 0x53c4, "YUV_POLY_SC3_Y_H_COEFF_5_45" },
	{ 0x53c8, "YUV_POLY_SC3_Y_H_COEFF_5_67" },
	{ 0x53cc, "YUV_POLY_SC3_Y_H_COEFF_6_01" },
	{ 0x53d0, "YUV_POLY_SC3_Y_H_COEFF_6_23" },
	{ 0x53d4, "YUV_POLY_SC3_Y_H_COEFF_6_45" },
	{ 0x53d8, "YUV_POLY_SC3_Y_H_COEFF_6_67" },
	{ 0x53dc, "YUV_POLY_SC3_Y_H_COEFF_7_01" },
	{ 0x53e0, "YUV_POLY_SC3_Y_H_COEFF_7_23" },
	{ 0x53e4, "YUV_POLY_SC3_Y_H_COEFF_7_45" },
	{ 0x53e8, "YUV_POLY_SC3_Y_H_COEFF_7_67" },
	{ 0x53ec, "YUV_POLY_SC3_Y_H_COEFF_8_01" },
	{ 0x53f0, "YUV_POLY_SC3_Y_H_COEFF_8_23" },
	{ 0x53f4, "YUV_POLY_SC3_Y_H_COEFF_8_45" },
	{ 0x53f8, "YUV_POLY_SC3_Y_H_COEFF_8_67" },
	{ 0x5400, "YUV_POLY_SC4_CTRL" },
	{ 0x5404, "YUV_POLY_SC4_SRC_POS" },
	{ 0x5408, "YUV_POLY_SC4_SRC_SIZE" },
	{ 0x540c, "YUV_POLY_SC4_DST_SIZE" },
	{ 0x5410, "YUV_POLY_SC4_H_RATIO" },
	{ 0x5414, "YUV_POLY_SC4_V_RATIO" },
	{ 0x5418, "YUV_POLY_SC4_H_INIT_PHASE_OFFSET" },
	{ 0x541c, "YUV_POLY_SC4_V_INIT_PHASE_OFFSET" },
	{ 0x5420, "YUV_POLY_SC4_ROUND_MODE" },
	{ 0x5424, "YUV_POLY_SC4_Y_V_COEFF_0_01" },
	{ 0x5428, "YUV_POLY_SC4_Y_V_COEFF_0_23" },
	{ 0x542c, "YUV_POLY_SC4_Y_V_COEFF_1_01" },
	{ 0x5430, "YUV_POLY_SC4_Y_V_COEFF_1_23" },
	{ 0x5434, "YUV_POLY_SC4_Y_V_COEFF_2_01" },
	{ 0x5438, "YUV_POLY_SC4_Y_V_COEFF_2_23" },
	{ 0x543c, "YUV_POLY_SC4_Y_V_COEFF_3_01" },
	{ 0x5440, "YUV_POLY_SC4_Y_V_COEFF_3_23" },
	{ 0x5444, "YUV_POLY_SC4_Y_V_COEFF_4_01" },
	{ 0x5448, "YUV_POLY_SC4_Y_V_COEFF_4_23" },
	{ 0x544c, "YUV_POLY_SC4_Y_V_COEFF_5_01" },
	{ 0x5450, "YUV_POLY_SC4_Y_V_COEFF_5_23" },
	{ 0x5454, "YUV_POLY_SC4_Y_V_COEFF_6_01" },
	{ 0x5458, "YUV_POLY_SC4_Y_V_COEFF_6_23" },
	{ 0x545c, "YUV_POLY_SC4_Y_V_COEFF_7_01" },
	{ 0x5460, "YUV_POLY_SC4_Y_V_COEFF_7_23" },
	{ 0x5464, "YUV_POLY_SC4_Y_V_COEFF_8_01" },
	{ 0x5468, "YUV_POLY_SC4_Y_V_COEFF_8_23" },
	{ 0x546c, "YUV_POLY_SC4_Y_H_COEFF_0_01" },
	{ 0x5470, "YUV_POLY_SC4_Y_H_COEFF_0_23" },
	{ 0x5474, "YUV_POLY_SC4_Y_H_COEFF_0_45" },
	{ 0x5478, "YUV_POLY_SC4_Y_H_COEFF_0_67" },
	{ 0x547c, "YUV_POLY_SC4_Y_H_COEFF_1_01" },
	{ 0x5480, "YUV_POLY_SC4_Y_H_COEFF_1_23" },
	{ 0x5484, "YUV_POLY_SC4_Y_H_COEFF_1_45" },
	{ 0x5488, "YUV_POLY_SC4_Y_H_COEFF_1_67" },
	{ 0x548c, "YUV_POLY_SC4_Y_H_COEFF_2_01" },
	{ 0x5490, "YUV_POLY_SC4_Y_H_COEFF_2_23" },
	{ 0x5494, "YUV_POLY_SC4_Y_H_COEFF_2_45" },
	{ 0x5498, "YUV_POLY_SC4_Y_H_COEFF_2_67" },
	{ 0x549c, "YUV_POLY_SC4_Y_H_COEFF_3_01" },
	{ 0x54a0, "YUV_POLY_SC4_Y_H_COEFF_3_23" },
	{ 0x54a4, "YUV_POLY_SC4_Y_H_COEFF_3_45" },
	{ 0x54a8, "YUV_POLY_SC4_Y_H_COEFF_3_67" },
	{ 0x54ac, "YUV_POLY_SC4_Y_H_COEFF_4_01" },
	{ 0x54b0, "YUV_POLY_SC4_Y_H_COEFF_4_23" },
	{ 0x54b4, "YUV_POLY_SC4_Y_H_COEFF_4_45" },
	{ 0x54b8, "YUV_POLY_SC4_Y_H_COEFF_4_67" },
	{ 0x54bc, "YUV_POLY_SC4_Y_H_COEFF_5_01" },
	{ 0x54c0, "YUV_POLY_SC4_Y_H_COEFF_5_23" },
	{ 0x54c4, "YUV_POLY_SC4_Y_H_COEFF_5_45" },
	{ 0x54c8, "YUV_POLY_SC4_Y_H_COEFF_5_67" },
	{ 0x54cc, "YUV_POLY_SC4_Y_H_COEFF_6_01" },
	{ 0x54d0, "YUV_POLY_SC4_Y_H_COEFF_6_23" },
	{ 0x54d4, "YUV_POLY_SC4_Y_H_COEFF_6_45" },
	{ 0x54d8, "YUV_POLY_SC4_Y_H_COEFF_6_67" },
	{ 0x54dc, "YUV_POLY_SC4_Y_H_COEFF_7_01" },
	{ 0x54e0, "YUV_POLY_SC4_Y_H_COEFF_7_23" },
	{ 0x54e4, "YUV_POLY_SC4_Y_H_COEFF_7_45" },
	{ 0x54e8, "YUV_POLY_SC4_Y_H_COEFF_7_67" },
	{ 0x54ec, "YUV_POLY_SC4_Y_H_COEFF_8_01" },
	{ 0x54f0, "YUV_POLY_SC4_Y_H_COEFF_8_23" },
	{ 0x54f4, "YUV_POLY_SC4_Y_H_COEFF_8_45" },
	{ 0x54f8, "YUV_POLY_SC4_Y_H_COEFF_8_67" },
	{ 0x5524, "YUV_POLY_SC0_UV_V_COEFF_0_01" },
	{ 0x5528, "YUV_POLY_SC0_UV_V_COEFF_0_23" },
	{ 0x552c, "YUV_POLY_SC0_UV_V_COEFF_1_01" },
	{ 0x5530, "YUV_POLY_SC0_UV_V_COEFF_1_23" },
	{ 0x5534, "YUV_POLY_SC0_UV_V_COEFF_2_01" },
	{ 0x5538, "YUV_POLY_SC0_UV_V_COEFF_2_23" },
	{ 0x553c, "YUV_POLY_SC0_UV_V_COEFF_3_01" },
	{ 0x5540, "YUV_POLY_SC0_UV_V_COEFF_3_23" },
	{ 0x5544, "YUV_POLY_SC0_UV_V_COEFF_4_01" },
	{ 0x5548, "YUV_POLY_SC0_UV_V_COEFF_4_23" },
	{ 0x554c, "YUV_POLY_SC0_UV_V_COEFF_5_01" },
	{ 0x5550, "YUV_POLY_SC0_UV_V_COEFF_5_23" },
	{ 0x5554, "YUV_POLY_SC0_UV_V_COEFF_6_01" },
	{ 0x5558, "YUV_POLY_SC0_UV_V_COEFF_6_23" },
	{ 0x555c, "YUV_POLY_SC0_UV_V_COEFF_7_01" },
	{ 0x5560, "YUV_POLY_SC0_UV_V_COEFF_7_23" },
	{ 0x5564, "YUV_POLY_SC0_UV_V_COEFF_8_01" },
	{ 0x5568, "YUV_POLY_SC0_UV_V_COEFF_8_23" },
	{ 0x556c, "YUV_POLY_SC0_UV_H_COEFF_0_01" },
	{ 0x5570, "YUV_POLY_SC0_UV_H_COEFF_0_23" },
	{ 0x557c, "YUV_POLY_SC0_UV_H_COEFF_1_01" },
	{ 0x5580, "YUV_POLY_SC0_UV_H_COEFF_1_23" },
	{ 0x558c, "YUV_POLY_SC0_UV_H_COEFF_2_01" },
	{ 0x5590, "YUV_POLY_SC0_UV_H_COEFF_2_23" },
	{ 0x559c, "YUV_POLY_SC0_UV_H_COEFF_3_01" },
	{ 0x55a0, "YUV_POLY_SC0_UV_H_COEFF_3_23" },
	{ 0x55ac, "YUV_POLY_SC0_UV_H_COEFF_4_01" },
	{ 0x55b0, "YUV_POLY_SC0_UV_H_COEFF_4_23" },
	{ 0x55bc, "YUV_POLY_SC0_UV_H_COEFF_5_01" },
	{ 0x55c0, "YUV_POLY_SC0_UV_H_COEFF_5_23" },
	{ 0x55cc, "YUV_POLY_SC0_UV_H_COEFF_6_01" },
	{ 0x55d0, "YUV_POLY_SC0_UV_H_COEFF_6_23" },
	{ 0x55dc, "YUV_POLY_SC0_UV_H_COEFF_7_01" },
	{ 0x55e0, "YUV_POLY_SC0_UV_H_COEFF_7_23" },
	{ 0x55ec, "YUV_POLY_SC0_UV_H_COEFF_8_01" },
	{ 0x55f0, "YUV_POLY_SC0_UV_H_COEFF_8_23" },
	{ 0x5624, "YUV_POLY_SC1_UV_V_COEFF_0_01" },
	{ 0x5628, "YUV_POLY_SC1_UV_V_COEFF_0_23" },
	{ 0x562c, "YUV_POLY_SC1_UV_V_COEFF_1_01" },
	{ 0x5630, "YUV_POLY_SC1_UV_V_COEFF_1_23" },
	{ 0x5634, "YUV_POLY_SC1_UV_V_COEFF_2_01" },
	{ 0x5638, "YUV_POLY_SC1_UV_V_COEFF_2_23" },
	{ 0x563c, "YUV_POLY_SC1_UV_V_COEFF_3_01" },
	{ 0x5640, "YUV_POLY_SC1_UV_V_COEFF_3_23" },
	{ 0x5644, "YUV_POLY_SC1_UV_V_COEFF_4_01" },
	{ 0x5648, "YUV_POLY_SC1_UV_V_COEFF_4_23" },
	{ 0x564c, "YUV_POLY_SC1_UV_V_COEFF_5_01" },
	{ 0x5650, "YUV_POLY_SC1_UV_V_COEFF_5_23" },
	{ 0x5654, "YUV_POLY_SC1_UV_V_COEFF_6_01" },
	{ 0x5658, "YUV_POLY_SC1_UV_V_COEFF_6_23" },
	{ 0x565c, "YUV_POLY_SC1_UV_V_COEFF_7_01" },
	{ 0x5660, "YUV_POLY_SC1_UV_V_COEFF_7_23" },
	{ 0x5664, "YUV_POLY_SC1_UV_V_COEFF_8_01" },
	{ 0x5668, "YUV_POLY_SC1_UV_V_COEFF_8_23" },
	{ 0x566c, "YUV_POLY_SC1_UV_H_COEFF_0_01" },
	{ 0x5670, "YUV_POLY_SC1_UV_H_COEFF_0_23" },
	{ 0x567c, "YUV_POLY_SC1_UV_H_COEFF_1_01" },
	{ 0x5680, "YUV_POLY_SC1_UV_H_COEFF_1_23" },
	{ 0x568c, "YUV_POLY_SC1_UV_H_COEFF_2_01" },
	{ 0x5690, "YUV_POLY_SC1_UV_H_COEFF_2_23" },
	{ 0x569c, "YUV_POLY_SC1_UV_H_COEFF_3_01" },
	{ 0x56a0, "YUV_POLY_SC1_UV_H_COEFF_3_23" },
	{ 0x56ac, "YUV_POLY_SC1_UV_H_COEFF_4_01" },
	{ 0x56b0, "YUV_POLY_SC1_UV_H_COEFF_4_23" },
	{ 0x56bc, "YUV_POLY_SC1_UV_H_COEFF_5_01" },
	{ 0x56c0, "YUV_POLY_SC1_UV_H_COEFF_5_23" },
	{ 0x56cc, "YUV_POLY_SC1_UV_H_COEFF_6_01" },
	{ 0x56d0, "YUV_POLY_SC1_UV_H_COEFF_6_23" },
	{ 0x56dc, "YUV_POLY_SC1_UV_H_COEFF_7_01" },
	{ 0x56e0, "YUV_POLY_SC1_UV_H_COEFF_7_23" },
	{ 0x56ec, "YUV_POLY_SC1_UV_H_COEFF_8_01" },
	{ 0x56f0, "YUV_POLY_SC1_UV_H_COEFF_8_23" },
	{ 0x5724, "YUV_POLY_SC2_UV_V_COEFF_0_01" },
	{ 0x5728, "YUV_POLY_SC2_UV_V_COEFF_0_23" },
	{ 0x572c, "YUV_POLY_SC2_UV_V_COEFF_1_01" },
	{ 0x5730, "YUV_POLY_SC2_UV_V_COEFF_1_23" },
	{ 0x5734, "YUV_POLY_SC2_UV_V_COEFF_2_01" },
	{ 0x5738, "YUV_POLY_SC2_UV_V_COEFF_2_23" },
	{ 0x573c, "YUV_POLY_SC2_UV_V_COEFF_3_01" },
	{ 0x5740, "YUV_POLY_SC2_UV_V_COEFF_3_23" },
	{ 0x5744, "YUV_POLY_SC2_UV_V_COEFF_4_01" },
	{ 0x5748, "YUV_POLY_SC2_UV_V_COEFF_4_23" },
	{ 0x574c, "YUV_POLY_SC2_UV_V_COEFF_5_01" },
	{ 0x5750, "YUV_POLY_SC2_UV_V_COEFF_5_23" },
	{ 0x5754, "YUV_POLY_SC2_UV_V_COEFF_6_01" },
	{ 0x5758, "YUV_POLY_SC2_UV_V_COEFF_6_23" },
	{ 0x575c, "YUV_POLY_SC2_UV_V_COEFF_7_01" },
	{ 0x5760, "YUV_POLY_SC2_UV_V_COEFF_7_23" },
	{ 0x5764, "YUV_POLY_SC2_UV_V_COEFF_8_01" },
	{ 0x5768, "YUV_POLY_SC2_UV_V_COEFF_8_23" },
	{ 0x576c, "YUV_POLY_SC2_UV_H_COEFF_0_01" },
	{ 0x5770, "YUV_POLY_SC2_UV_H_COEFF_0_23" },
	{ 0x577c, "YUV_POLY_SC2_UV_H_COEFF_1_01" },
	{ 0x5780, "YUV_POLY_SC2_UV_H_COEFF_1_23" },
	{ 0x578c, "YUV_POLY_SC2_UV_H_COEFF_2_01" },
	{ 0x5790, "YUV_POLY_SC2_UV_H_COEFF_2_23" },
	{ 0x579c, "YUV_POLY_SC2_UV_H_COEFF_3_01" },
	{ 0x57a0, "YUV_POLY_SC2_UV_H_COEFF_3_23" },
	{ 0x57ac, "YUV_POLY_SC2_UV_H_COEFF_4_01" },
	{ 0x57b0, "YUV_POLY_SC2_UV_H_COEFF_4_23" },
	{ 0x57bc, "YUV_POLY_SC2_UV_H_COEFF_5_01" },
	{ 0x57c0, "YUV_POLY_SC2_UV_H_COEFF_5_23" },
	{ 0x57cc, "YUV_POLY_SC2_UV_H_COEFF_6_01" },
	{ 0x57d0, "YUV_POLY_SC2_UV_H_COEFF_6_23" },
	{ 0x57dc, "YUV_POLY_SC2_UV_H_COEFF_7_01" },
	{ 0x57e0, "YUV_POLY_SC2_UV_H_COEFF_7_23" },
	{ 0x57ec, "YUV_POLY_SC2_UV_H_COEFF_8_01" },
	{ 0x57f0, "YUV_POLY_SC2_UV_H_COEFF_8_23" },
	{ 0x5824, "YUV_POLY_SC3_UV_V_COEFF_0_01" },
	{ 0x5828, "YUV_POLY_SC3_UV_V_COEFF_0_23" },
	{ 0x582c, "YUV_POLY_SC3_UV_V_COEFF_1_01" },
	{ 0x5830, "YUV_POLY_SC3_UV_V_COEFF_1_23" },
	{ 0x5834, "YUV_POLY_SC3_UV_V_COEFF_2_01" },
	{ 0x5838, "YUV_POLY_SC3_UV_V_COEFF_2_23" },
	{ 0x583c, "YUV_POLY_SC3_UV_V_COEFF_3_01" },
	{ 0x5840, "YUV_POLY_SC3_UV_V_COEFF_3_23" },
	{ 0x5844, "YUV_POLY_SC3_UV_V_COEFF_4_01" },
	{ 0x5848, "YUV_POLY_SC3_UV_V_COEFF_4_23" },
	{ 0x584c, "YUV_POLY_SC3_UV_V_COEFF_5_01" },
	{ 0x5850, "YUV_POLY_SC3_UV_V_COEFF_5_23" },
	{ 0x5854, "YUV_POLY_SC3_UV_V_COEFF_6_01" },
	{ 0x5858, "YUV_POLY_SC3_UV_V_COEFF_6_23" },
	{ 0x585c, "YUV_POLY_SC3_UV_V_COEFF_7_01" },
	{ 0x5860, "YUV_POLY_SC3_UV_V_COEFF_7_23" },
	{ 0x5864, "YUV_POLY_SC3_UV_V_COEFF_8_01" },
	{ 0x5868, "YUV_POLY_SC3_UV_V_COEFF_8_23" },
	{ 0x586c, "YUV_POLY_SC3_UV_H_COEFF_0_01" },
	{ 0x5870, "YUV_POLY_SC3_UV_H_COEFF_0_23" },
	{ 0x587c, "YUV_POLY_SC3_UV_H_COEFF_1_01" },
	{ 0x5880, "YUV_POLY_SC3_UV_H_COEFF_1_23" },
	{ 0x588c, "YUV_POLY_SC3_UV_H_COEFF_2_01" },
	{ 0x5890, "YUV_POLY_SC3_UV_H_COEFF_2_23" },
	{ 0x589c, "YUV_POLY_SC3_UV_H_COEFF_3_01" },
	{ 0x58a0, "YUV_POLY_SC3_UV_H_COEFF_3_23" },
	{ 0x58ac, "YUV_POLY_SC3_UV_H_COEFF_4_01" },
	{ 0x58b0, "YUV_POLY_SC3_UV_H_COEFF_4_23" },
	{ 0x58bc, "YUV_POLY_SC3_UV_H_COEFF_5_01" },
	{ 0x58c0, "YUV_POLY_SC3_UV_H_COEFF_5_23" },
	{ 0x58cc, "YUV_POLY_SC3_UV_H_COEFF_6_01" },
	{ 0x58d0, "YUV_POLY_SC3_UV_H_COEFF_6_23" },
	{ 0x58dc, "YUV_POLY_SC3_UV_H_COEFF_7_01" },
	{ 0x58e0, "YUV_POLY_SC3_UV_H_COEFF_7_23" },
	{ 0x58ec, "YUV_POLY_SC3_UV_H_COEFF_8_01" },
	{ 0x58f0, "YUV_POLY_SC3_UV_H_COEFF_8_23" },
	{ 0x5924, "YUV_POLY_SC4_UV_V_COEFF_0_01" },
	{ 0x5928, "YUV_POLY_SC4_UV_V_COEFF_0_23" },
	{ 0x592c, "YUV_POLY_SC4_UV_V_COEFF_1_01" },
	{ 0x5930, "YUV_POLY_SC4_UV_V_COEFF_1_23" },
	{ 0x5934, "YUV_POLY_SC4_UV_V_COEFF_2_01" },
	{ 0x5938, "YUV_POLY_SC4_UV_V_COEFF_2_23" },
	{ 0x593c, "YUV_POLY_SC4_UV_V_COEFF_3_01" },
	{ 0x5940, "YUV_POLY_SC4_UV_V_COEFF_3_23" },
	{ 0x5944, "YUV_POLY_SC4_UV_V_COEFF_4_01" },
	{ 0x5948, "YUV_POLY_SC4_UV_V_COEFF_4_23" },
	{ 0x594c, "YUV_POLY_SC4_UV_V_COEFF_5_01" },
	{ 0x5950, "YUV_POLY_SC4_UV_V_COEFF_5_23" },
	{ 0x5954, "YUV_POLY_SC4_UV_V_COEFF_6_01" },
	{ 0x5958, "YUV_POLY_SC4_UV_V_COEFF_6_23" },
	{ 0x595c, "YUV_POLY_SC4_UV_V_COEFF_7_01" },
	{ 0x5960, "YUV_POLY_SC4_UV_V_COEFF_7_23" },
	{ 0x5964, "YUV_POLY_SC4_UV_V_COEFF_8_01" },
	{ 0x5968, "YUV_POLY_SC4_UV_V_COEFF_8_23" },
	{ 0x596c, "YUV_POLY_SC4_UV_H_COEFF_0_01" },
	{ 0x5970, "YUV_POLY_SC4_UV_H_COEFF_0_23" },
	{ 0x597c, "YUV_POLY_SC4_UV_H_COEFF_1_01" },
	{ 0x5980, "YUV_POLY_SC4_UV_H_COEFF_1_23" },
	{ 0x598c, "YUV_POLY_SC4_UV_H_COEFF_2_01" },
	{ 0x5990, "YUV_POLY_SC4_UV_H_COEFF_2_23" },
	{ 0x599c, "YUV_POLY_SC4_UV_H_COEFF_3_01" },
	{ 0x59a0, "YUV_POLY_SC4_UV_H_COEFF_3_23" },
	{ 0x59ac, "YUV_POLY_SC4_UV_H_COEFF_4_01" },
	{ 0x59b0, "YUV_POLY_SC4_UV_H_COEFF_4_23" },
	{ 0x59bc, "YUV_POLY_SC4_UV_H_COEFF_5_01" },
	{ 0x59c0, "YUV_POLY_SC4_UV_H_COEFF_5_23" },
	{ 0x59cc, "YUV_POLY_SC4_UV_H_COEFF_6_01" },
	{ 0x59d0, "YUV_POLY_SC4_UV_H_COEFF_6_23" },
	{ 0x59dc, "YUV_POLY_SC4_UV_H_COEFF_7_01" },
	{ 0x59e0, "YUV_POLY_SC4_UV_H_COEFF_7_23" },
	{ 0x59ec, "YUV_POLY_SC4_UV_H_COEFF_8_01" },
	{ 0x59f0, "YUV_POLY_SC4_UV_H_COEFF_8_23" },
	{ 0x5f00, "YUV_POLY_SC0_STRIP_PRE_DST_SIZE" },
	{ 0x5f04, "YUV_POLY_SC0_STRIP_IN_START_POS" },
	{ 0x5f08, "YUV_POLY_SC0_OUT_CROP_POS" },
	{ 0x5f0c, "YUV_POLY_SC0_OUT_CROP_SIZE" },
	{ 0x5f10, "YUV_POLY_SC1_STRIP_PRE_DST_SIZE" },
	{ 0x5f14, "YUV_POLY_SC1_STRIP_IN_START_POS" },
	{ 0x5f18, "YUV_POLY_SC1_OUT_CROP_POS" },
	{ 0x5f1c, "YUV_POLY_SC1_OUT_CROP_SIZE" },
	{ 0x5f20, "YUV_POLY_SC2_STRIP_PRE_DST_SIZE" },
	{ 0x5f24, "YUV_POLY_SC2_STRIP_IN_START_POS" },
	{ 0x5f28, "YUV_POLY_SC2_OUT_CROP_POS" },
	{ 0x5f2c, "YUV_POLY_SC2_OUT_CROP_SIZE" },
	{ 0x5f30, "YUV_POLY_SC3_STRIP_PRE_DST_SIZE" },
	{ 0x5f34, "YUV_POLY_SC3_STRIP_IN_START_POS" },
	{ 0x5f38, "YUV_POLY_SC3_OUT_CROP_POS" },
	{ 0x5f3c, "YUV_POLY_SC3_OUT_CROP_SIZE" },
	{ 0x5f40, "YUV_POLY_SC4_STRIP_PRE_DST_SIZE" },
	{ 0x5f44, "YUV_POLY_SC4_STRIP_IN_START_POS" },
	{ 0x5f48, "YUV_POLY_SC4_OUT_CROP_POS" },
	{ 0x5f4c, "YUV_POLY_SC4_OUT_CROP_SIZE" },
	{ 0x6000, "YUV_POST_PC0_CTRL" },
	{ 0x6004, "YUV_POST_PC0_IMG_SIZE" },
	{ 0x6008, "YUV_POST_PC0_DST_SIZE" },
	{ 0x600c, "YUV_POSTPC_PC0_H_RATIO" },
	{ 0x6010, "YUV_POSTPC_PC0_V_RATIO" },
	{ 0x6014, "YUV_POSTPC_PC0_H_INIT_PHASE_OFFSET" },
	{ 0x6018, "YUV_POSTPC_PC0_V_INIT_PHASE_OFFSET" },
	{ 0x601c, "YUV_POSTPC_PC0_ROUND_MODE" },
	{ 0x6030, "YUV_POST_PC0_STRIP_PRE_DST_SIZE" },
	{ 0x6034, "YUV_POST_PC0_STRIP_IN_START_POS" },
	{ 0x6038, "YUV_POST_PC0_OUT_CROP_POS" },
	{ 0x603c, "YUV_POST_PC0_OUT_CROP_SIZE" },
	{ 0x6040, "YUV_POST_PC0_CONV420_CTRL" },
	{ 0x6048, "YUV_POST_PC0_BCHS_CTRL" },
	{ 0x604c, "YUV_POST_PC0_BCHS_BC" },
	{ 0x6050, "YUV_POST_PC0_BCHS_HS1" },
	{ 0x6054, "YUV_POST_PC0_BCHS_HS2" },
	{ 0x6058, "YUV_POST_PC0_BCHS_CLAMP_Y" },
	{ 0x605c, "YUV_POST_PC0_BCHS_CLAMP_C" },
	{ 0x6100, "YUV_POST_PC1_CTRL" },
	{ 0x6104, "YUV_POST_PC1_IMG_SIZE" },
	{ 0x6108, "YUV_POST_PC1_DST_SIZE" },
	{ 0x610c, "YUV_POSTPC_PC1_H_RATIO" },
	{ 0x6110, "YUV_POSTPC_PC1_V_RATIO" },
	{ 0x6114, "YUV_POSTPC_PC1_H_INIT_PHASE_OFFSET" },
	{ 0x6118, "YUV_POSTPC_PC1_V_INIT_PHASE_OFFSET" },
	{ 0x611c, "YUV_POSTPC_PC1_ROUND_MODE" },
	{ 0x6130, "YUV_POST_PC1_STRIP_PRE_DST_SIZE" },
	{ 0x6134, "YUV_POST_PC1_STRIP_IN_START_POS" },
	{ 0x6138, "YUV_POST_PC1_OUT_CROP_POS" },
	{ 0x613c, "YUV_POST_PC1_OUT_CROP_SIZE" },
	{ 0x6140, "YUV_POST_PC1_CONV420_CTRL" },
	{ 0x6148, "YUV_POST_PC1_BCHS_CTRL" },
	{ 0x614c, "YUV_POST_PC1_BCHS_BC" },
	{ 0x6150, "YUV_POST_PC1_BCHS_HS1" },
	{ 0x6154, "YUV_POST_PC1_BCHS_HS2" },
	{ 0x6158, "YUV_POST_PC1_BCHS_CLAMP_Y" },
	{ 0x615c, "YUV_POST_PC1_BCHS_CLAMP_C" },
	{ 0x6200, "YUV_POST_PC2_CTRL" },
	{ 0x6204, "YUV_POST_PC2_IMG_SIZE" },
	{ 0x6208, "YUV_POST_PC2_DST_SIZE" },
	{ 0x620c, "YUV_POSTPC_PC2_H_RATIO" },
	{ 0x6210, "YUV_POSTPC_PC2_V_RATIO" },
	{ 0x6214, "YUV_POSTPC_PC2_H_INIT_PHASE_OFFSET" },
	{ 0x6218, "YUV_POSTPC_PC2_V_INIT_PHASE_OFFSET" },
	{ 0x621c, "YUV_POSTPC_PC2_ROUND_MODE" },
	{ 0x6230, "YUV_POST_PC2_STRIP_PRE_DST_SIZE" },
	{ 0x6234, "YUV_POST_PC2_STRIP_IN_START_POS" },
	{ 0x6238, "YUV_POST_PC2_OUT_CROP_POS" },
	{ 0x623c, "YUV_POST_PC2_OUT_CROP_SIZE" },
	{ 0x6240, "YUV_POST_PC2_CONV420_CTRL" },
	{ 0x6248, "YUV_POST_PC2_BCHS_CTRL" },
	{ 0x624c, "YUV_POST_PC2_BCHS_BC" },
	{ 0x6250, "YUV_POST_PC2_BCHS_HS1" },
	{ 0x6254, "YUV_POST_PC2_BCHS_HS2" },
	{ 0x6258, "YUV_POST_PC2_BCHS_CLAMP_Y" },
	{ 0x625c, "YUV_POST_PC2_BCHS_CLAMP_C" },
	{ 0x6300, "YUV_POST_PC3_CTRL" },
	{ 0x6340, "YUV_POST_PC3_CONV420_CTRL" },
	{ 0x6348, "YUV_POST_PC3_BCHS_CTRL" },
	{ 0x634c, "YUV_POST_PC3_BCHS_BC" },
	{ 0x6350, "YUV_POST_PC3_BCHS_HS1" },
	{ 0x6354, "YUV_POST_PC3_BCHS_HS2" },
	{ 0x6358, "YUV_POST_PC3_BCHS_CLAMP_Y" },
	{ 0x635c, "YUV_POST_PC3_BCHS_CLAMP_C" },
	{ 0x6400, "YUV_POST_PC4_CTRL" },
	{ 0x6440, "YUV_POST_PC4_CONV420_CTRL" },
	{ 0x6448, "YUV_POST_PC4_BCHS_CTRL" },
	{ 0x644c, "YUV_POST_PC4_BCHS_BC" },
	{ 0x6450, "YUV_POST_PC4_BCHS_HS1" },
	{ 0x6454, "YUV_POST_PC4_BCHS_HS2" },
	{ 0x6458, "YUV_POST_PC4_BCHS_CLAMP_Y" },
	{ 0x645c, "YUV_POST_PC4_BCHS_CLAMP_C" },
	{ 0x6524, "YUV_POSTPC_PC0_Y_V_COEFF_0_01" },
	{ 0x6528, "YUV_POSTPC_PC0_Y_V_COEFF_0_23" },
	{ 0x652c, "YUV_POSTPC_PC0_Y_V_COEFF_1_01" },
	{ 0x6530, "YUV_POSTPC_PC0_Y_V_COEFF_1_23" },
	{ 0x6534, "YUV_POSTPC_PC0_Y_V_COEFF_2_01" },
	{ 0x6538, "YUV_POSTPC_PC0_Y_V_COEFF_2_23" },
	{ 0x653c, "YUV_POSTPC_PC0_Y_V_COEFF_3_01" },
	{ 0x6540, "YUV_POSTPC_PC0_Y_V_COEFF_3_23" },
	{ 0x6544, "YUV_POSTPC_PC0_Y_V_COEFF_4_01" },
	{ 0x6548, "YUV_POSTPC_PC0_Y_V_COEFF_4_23" },
	{ 0x654c, "YUV_POSTPC_PC0_Y_V_COEFF_5_01" },
	{ 0x6550, "YUV_POSTPC_PC0_Y_V_COEFF_5_23" },
	{ 0x6554, "YUV_POSTPC_PC0_Y_V_COEFF_6_01" },
	{ 0x6558, "YUV_POSTPC_PC0_Y_V_COEFF_6_23" },
	{ 0x655c, "YUV_POSTPC_PC0_Y_V_COEFF_7_01" },
	{ 0x6560, "YUV_POSTPC_PC0_Y_V_COEFF_7_23" },
	{ 0x6564, "YUV_POSTPC_PC0_Y_V_COEFF_8_01" },
	{ 0x6568, "YUV_POSTPC_PC0_Y_V_COEFF_8_23" },
	{ 0x656c, "YUV_POSTPC_PC0_Y_H_COEFF_0_01" },
	{ 0x6570, "YUV_POSTPC_PC0_Y_H_COEFF_0_23" },
	{ 0x6574, "YUV_POSTPC_PC0_Y_H_COEFF_0_45" },
	{ 0x6578, "YUV_POSTPC_PC0_Y_H_COEFF_0_67" },
	{ 0x657c, "YUV_POSTPC_PC0_Y_H_COEFF_1_01" },
	{ 0x6580, "YUV_POSTPC_PC0_Y_H_COEFF_1_23" },
	{ 0x6584, "YUV_POSTPC_PC0_Y_H_COEFF_1_45" },
	{ 0x6588, "YUV_POSTPC_PC0_Y_H_COEFF_1_67" },
	{ 0x658c, "YUV_POSTPC_PC0_Y_H_COEFF_2_01" },
	{ 0x6590, "YUV_POSTPC_PC0_Y_H_COEFF_2_23" },
	{ 0x6594, "YUV_POSTPC_PC0_Y_H_COEFF_2_45" },
	{ 0x6598, "YUV_POSTPC_PC0_Y_H_COEFF_2_67" },
	{ 0x659c, "YUV_POSTPC_PC0_Y_H_COEFF_3_01" },
	{ 0x65a0, "YUV_POSTPC_PC0_Y_H_COEFF_3_23" },
	{ 0x65a4, "YUV_POSTPC_PC0_Y_H_COEFF_3_45" },
	{ 0x65a8, "YUV_POSTPC_PC0_Y_H_COEFF_3_67" },
	{ 0x65ac, "YUV_POSTPC_PC0_Y_H_COEFF_4_01" },
	{ 0x65b0, "YUV_POSTPC_PC0_Y_H_COEFF_4_23" },
	{ 0x65b4, "YUV_POSTPC_PC0_Y_H_COEFF_4_45" },
	{ 0x65b8, "YUV_POSTPC_PC0_Y_H_COEFF_4_67" },
	{ 0x65bc, "YUV_POSTPC_PC0_Y_H_COEFF_5_01" },
	{ 0x65c0, "YUV_POSTPC_PC0_Y_H_COEFF_5_23" },
	{ 0x65c4, "YUV_POSTPC_PC0_Y_H_COEFF_5_45" },
	{ 0x65c8, "YUV_POSTPC_PC0_Y_H_COEFF_5_67" },
	{ 0x65cc, "YUV_POSTPC_PC0_Y_H_COEFF_6_01" },
	{ 0x65d0, "YUV_POSTPC_PC0_Y_H_COEFF_6_23" },
	{ 0x65d4, "YUV_POSTPC_PC0_Y_H_COEFF_6_45" },
	{ 0x65d8, "YUV_POSTPC_PC0_Y_H_COEFF_6_67" },
	{ 0x65dc, "YUV_POSTPC_PC0_Y_H_COEFF_7_01" },
	{ 0x65e0, "YUV_POSTPC_PC0_Y_H_COEFF_7_23" },
	{ 0x65e4, "YUV_POSTPC_PC0_Y_H_COEFF_7_45" },
	{ 0x65e8, "YUV_POSTPC_PC0_Y_H_COEFF_7_67" },
	{ 0x65ec, "YUV_POSTPC_PC0_Y_H_COEFF_8_01" },
	{ 0x65f0, "YUV_POSTPC_PC0_Y_H_COEFF_8_23" },
	{ 0x65f4, "YUV_POSTPC_PC0_Y_H_COEFF_8_45" },
	{ 0x65f8, "YUV_POSTPC_PC0_Y_H_COEFF_8_67" },
	{ 0x6624, "YUV_POSTPC_PC1_Y_V_COEFF_0_01" },
	{ 0x6628, "YUV_POSTPC_PC1_Y_V_COEFF_0_23" },
	{ 0x662c, "YUV_POSTPC_PC1_Y_V_COEFF_1_01" },
	{ 0x6630, "YUV_POSTPC_PC1_Y_V_COEFF_1_23" },
	{ 0x6634, "YUV_POSTPC_PC1_Y_V_COEFF_2_01" },
	{ 0x6638, "YUV_POSTPC_PC1_Y_V_COEFF_2_23" },
	{ 0x663c, "YUV_POSTPC_PC1_Y_V_COEFF_3_01" },
	{ 0x6640, "YUV_POSTPC_PC1_Y_V_COEFF_3_23" },
	{ 0x6644, "YUV_POSTPC_PC1_Y_V_COEFF_4_01" },
	{ 0x6648, "YUV_POSTPC_PC1_Y_V_COEFF_4_23" },
	{ 0x664c, "YUV_POSTPC_PC1_Y_V_COEFF_5_01" },
	{ 0x6650, "YUV_POSTPC_PC1_Y_V_COEFF_5_23" },
	{ 0x6654, "YUV_POSTPC_PC1_Y_V_COEFF_6_01" },
	{ 0x6658, "YUV_POSTPC_PC1_Y_V_COEFF_6_23" },
	{ 0x665c, "YUV_POSTPC_PC1_Y_V_COEFF_7_01" },
	{ 0x6660, "YUV_POSTPC_PC1_Y_V_COEFF_7_23" },
	{ 0x6664, "YUV_POSTPC_PC1_Y_V_COEFF_8_01" },
	{ 0x6668, "YUV_POSTPC_PC1_Y_V_COEFF_8_23" },
	{ 0x666c, "YUV_POSTPC_PC1_Y_H_COEFF_0_01" },
	{ 0x6670, "YUV_POSTPC_PC1_Y_H_COEFF_0_23" },
	{ 0x6674, "YUV_POSTPC_PC1_Y_H_COEFF_0_45" },
	{ 0x6678, "YUV_POSTPC_PC1_Y_H_COEFF_0_67" },
	{ 0x667c, "YUV_POSTPC_PC1_Y_H_COEFF_1_01" },
	{ 0x6680, "YUV_POSTPC_PC1_Y_H_COEFF_1_23" },
	{ 0x6684, "YUV_POSTPC_PC1_Y_H_COEFF_1_45" },
	{ 0x6688, "YUV_POSTPC_PC1_Y_H_COEFF_1_67" },
	{ 0x668c, "YUV_POSTPC_PC1_Y_H_COEFF_2_01" },
	{ 0x6690, "YUV_POSTPC_PC1_Y_H_COEFF_2_23" },
	{ 0x6694, "YUV_POSTPC_PC1_Y_H_COEFF_2_45" },
	{ 0x6698, "YUV_POSTPC_PC1_Y_H_COEFF_2_67" },
	{ 0x669c, "YUV_POSTPC_PC1_Y_H_COEFF_3_01" },
	{ 0x66a0, "YUV_POSTPC_PC1_Y_H_COEFF_3_23" },
	{ 0x66a4, "YUV_POSTPC_PC1_Y_H_COEFF_3_45" },
	{ 0x66a8, "YUV_POSTPC_PC1_Y_H_COEFF_3_67" },
	{ 0x66ac, "YUV_POSTPC_PC1_Y_H_COEFF_4_01" },
	{ 0x66b0, "YUV_POSTPC_PC1_Y_H_COEFF_4_23" },
	{ 0x66b4, "YUV_POSTPC_PC1_Y_H_COEFF_4_45" },
	{ 0x66b8, "YUV_POSTPC_PC1_Y_H_COEFF_4_67" },
	{ 0x66bc, "YUV_POSTPC_PC1_Y_H_COEFF_5_01" },
	{ 0x66c0, "YUV_POSTPC_PC1_Y_H_COEFF_5_23" },
	{ 0x66c4, "YUV_POSTPC_PC1_Y_H_COEFF_5_45" },
	{ 0x66c8, "YUV_POSTPC_PC1_Y_H_COEFF_5_67" },
	{ 0x66cc, "YUV_POSTPC_PC1_Y_H_COEFF_6_01" },
	{ 0x66d0, "YUV_POSTPC_PC1_Y_H_COEFF_6_23" },
	{ 0x66d4, "YUV_POSTPC_PC1_Y_H_COEFF_6_45" },
	{ 0x66d8, "YUV_POSTPC_PC1_Y_H_COEFF_6_67" },
	{ 0x66dc, "YUV_POSTPC_PC1_Y_H_COEFF_7_01" },
	{ 0x66e0, "YUV_POSTPC_PC1_Y_H_COEFF_7_23" },
	{ 0x66e4, "YUV_POSTPC_PC1_Y_H_COEFF_7_45" },
	{ 0x66e8, "YUV_POSTPC_PC1_Y_H_COEFF_7_67" },
	{ 0x66ec, "YUV_POSTPC_PC1_Y_H_COEFF_8_01" },
	{ 0x66f0, "YUV_POSTPC_PC1_Y_H_COEFF_8_23" },
	{ 0x66f4, "YUV_POSTPC_PC1_Y_H_COEFF_8_45" },
	{ 0x66f8, "YUV_POSTPC_PC1_Y_H_COEFF_8_67" },
	{ 0x6724, "YUV_POSTPC_PC2_Y_V_COEFF_0_01" },
	{ 0x6728, "YUV_POSTPC_PC2_Y_V_COEFF_0_23" },
	{ 0x672c, "YUV_POSTPC_PC2_Y_V_COEFF_1_01" },
	{ 0x6730, "YUV_POSTPC_PC2_Y_V_COEFF_1_23" },
	{ 0x6734, "YUV_POSTPC_PC2_Y_V_COEFF_2_01" },
	{ 0x6738, "YUV_POSTPC_PC2_Y_V_COEFF_2_23" },
	{ 0x673c, "YUV_POSTPC_PC2_Y_V_COEFF_3_01" },
	{ 0x6740, "YUV_POSTPC_PC2_Y_V_COEFF_3_23" },
	{ 0x6744, "YUV_POSTPC_PC2_Y_V_COEFF_4_01" },
	{ 0x6748, "YUV_POSTPC_PC2_Y_V_COEFF_4_23" },
	{ 0x674c, "YUV_POSTPC_PC2_Y_V_COEFF_5_01" },
	{ 0x6750, "YUV_POSTPC_PC2_Y_V_COEFF_5_23" },
	{ 0x6754, "YUV_POSTPC_PC2_Y_V_COEFF_6_01" },
	{ 0x6758, "YUV_POSTPC_PC2_Y_V_COEFF_6_23" },
	{ 0x675c, "YUV_POSTPC_PC2_Y_V_COEFF_7_01" },
	{ 0x6760, "YUV_POSTPC_PC2_Y_V_COEFF_7_23" },
	{ 0x6764, "YUV_POSTPC_PC2_Y_V_COEFF_8_01" },
	{ 0x6768, "YUV_POSTPC_PC2_Y_V_COEFF_8_23" },
	{ 0x676c, "YUV_POSTPC_PC2_Y_H_COEFF_0_01" },
	{ 0x6770, "YUV_POSTPC_PC2_Y_H_COEFF_0_23" },
	{ 0x6774, "YUV_POSTPC_PC2_Y_H_COEFF_0_45" },
	{ 0x6778, "YUV_POSTPC_PC2_Y_H_COEFF_0_67" },
	{ 0x677c, "YUV_POSTPC_PC2_Y_H_COEFF_1_01" },
	{ 0x6780, "YUV_POSTPC_PC2_Y_H_COEFF_1_23" },
	{ 0x6784, "YUV_POSTPC_PC2_Y_H_COEFF_1_45" },
	{ 0x6788, "YUV_POSTPC_PC2_Y_H_COEFF_1_67" },
	{ 0x678c, "YUV_POSTPC_PC2_Y_H_COEFF_2_01" },
	{ 0x6790, "YUV_POSTPC_PC2_Y_H_COEFF_2_23" },
	{ 0x6794, "YUV_POSTPC_PC2_Y_H_COEFF_2_45" },
	{ 0x6798, "YUV_POSTPC_PC2_Y_H_COEFF_2_67" },
	{ 0x679c, "YUV_POSTPC_PC2_Y_H_COEFF_3_01" },
	{ 0x67a0, "YUV_POSTPC_PC2_Y_H_COEFF_3_23" },
	{ 0x67a4, "YUV_POSTPC_PC2_Y_H_COEFF_3_45" },
	{ 0x67a8, "YUV_POSTPC_PC2_Y_H_COEFF_3_67" },
	{ 0x67ac, "YUV_POSTPC_PC2_Y_H_COEFF_4_01" },
	{ 0x67b0, "YUV_POSTPC_PC2_Y_H_COEFF_4_23" },
	{ 0x67b4, "YUV_POSTPC_PC2_Y_H_COEFF_4_45" },
	{ 0x67b8, "YUV_POSTPC_PC2_Y_H_COEFF_4_67" },
	{ 0x67bc, "YUV_POSTPC_PC2_Y_H_COEFF_5_01" },
	{ 0x67c0, "YUV_POSTPC_PC2_Y_H_COEFF_5_23" },
	{ 0x67c4, "YUV_POSTPC_PC2_Y_H_COEFF_5_45" },
	{ 0x67c8, "YUV_POSTPC_PC2_Y_H_COEFF_5_67" },
	{ 0x67cc, "YUV_POSTPC_PC2_Y_H_COEFF_6_01" },
	{ 0x67d0, "YUV_POSTPC_PC2_Y_H_COEFF_6_23" },
	{ 0x67d4, "YUV_POSTPC_PC2_Y_H_COEFF_6_45" },
	{ 0x67d8, "YUV_POSTPC_PC2_Y_H_COEFF_6_67" },
	{ 0x67dc, "YUV_POSTPC_PC2_Y_H_COEFF_7_01" },
	{ 0x67e0, "YUV_POSTPC_PC2_Y_H_COEFF_7_23" },
	{ 0x67e4, "YUV_POSTPC_PC2_Y_H_COEFF_7_45" },
	{ 0x67e8, "YUV_POSTPC_PC2_Y_H_COEFF_7_67" },
	{ 0x67ec, "YUV_POSTPC_PC2_Y_H_COEFF_8_01" },
	{ 0x67f0, "YUV_POSTPC_PC2_Y_H_COEFF_8_23" },
	{ 0x67f4, "YUV_POSTPC_PC2_Y_H_COEFF_8_45" },
	{ 0x67f8, "YUV_POSTPC_PC2_Y_H_COEFF_8_67" },
	{ 0x6a24, "YUV_POSTPC_PC0_UV_V_COEFF_0_01" },
	{ 0x6a28, "YUV_POSTPC_PC0_UV_V_COEFF_0_23" },
	{ 0x6a2c, "YUV_POSTPC_PC0_UV_V_COEFF_1_01" },
	{ 0x6a30, "YUV_POSTPC_PC0_UV_V_COEFF_1_23" },
	{ 0x6a34, "YUV_POSTPC_PC0_UV_V_COEFF_2_01" },
	{ 0x6a38, "YUV_POSTPC_PC0_UV_V_COEFF_2_23" },
	{ 0x6a3c, "YUV_POSTPC_PC0_UV_V_COEFF_3_01" },
	{ 0x6a40, "YUV_POSTPC_PC0_UV_V_COEFF_3_23" },
	{ 0x6a44, "YUV_POSTPC_PC0_UV_V_COEFF_4_01" },
	{ 0x6a48, "YUV_POSTPC_PC0_UV_V_COEFF_4_23" },
	{ 0x6a4c, "YUV_POSTPC_PC0_UV_V_COEFF_5_01" },
	{ 0x6a50, "YUV_POSTPC_PC0_UV_V_COEFF_5_23" },
	{ 0x6a54, "YUV_POSTPC_PC0_UV_V_COEFF_6_01" },
	{ 0x6a58, "YUV_POSTPC_PC0_UV_V_COEFF_6_23" },
	{ 0x6a5c, "YUV_POSTPC_PC0_UV_V_COEFF_7_01" },
	{ 0x6a60, "YUV_POSTPC_PC0_UV_V_COEFF_7_23" },
	{ 0x6a64, "YUV_POSTPC_PC0_UV_V_COEFF_8_01" },
	{ 0x6a68, "YUV_POSTPC_PC0_UV_V_COEFF_8_23" },
	{ 0x6a6c, "YUV_POSTPC_PC0_UV_H_COEFF_0_01" },
	{ 0x6a70, "YUV_POSTPC_PC0_UV_H_COEFF_0_23" },
	{ 0x6a7c, "YUV_POSTPC_PC0_UV_H_COEFF_1_01" },
	{ 0x6a80, "YUV_POSTPC_PC0_UV_H_COEFF_1_23" },
	{ 0x6a8c, "YUV_POSTPC_PC0_UV_H_COEFF_2_01" },
	{ 0x6a90, "YUV_POSTPC_PC0_UV_H_COEFF_2_23" },
	{ 0x6a9c, "YUV_POSTPC_PC0_UV_H_COEFF_3_01" },
	{ 0x6aa0, "YUV_POSTPC_PC0_UV_H_COEFF_3_23" },
	{ 0x6aac, "YUV_POSTPC_PC0_UV_H_COEFF_4_01" },
	{ 0x6ab0, "YUV_POSTPC_PC0_UV_H_COEFF_4_23" },
	{ 0x6abc, "YUV_POSTPC_PC0_UV_H_COEFF_5_01" },
	{ 0x6ac0, "YUV_POSTPC_PC0_UV_H_COEFF_5_23" },
	{ 0x6acc, "YUV_POSTPC_PC0_UV_H_COEFF_6_01" },
	{ 0x6ad0, "YUV_POSTPC_PC0_UV_H_COEFF_6_23" },
	{ 0x6adc, "YUV_POSTPC_PC0_UV_H_COEFF_7_01" },
	{ 0x6ae0, "YUV_POSTPC_PC0_UV_H_COEFF_7_23" },
	{ 0x6aec, "YUV_POSTPC_PC0_UV_H_COEFF_8_01" },
	{ 0x6af0, "YUV_POSTPC_PC0_UV_H_COEFF_8_23" },
	{ 0x6b24, "YUV_POSTPC_PC1_UV_V_COEFF_0_01" },
	{ 0x6b28, "YUV_POSTPC_PC1_UV_V_COEFF_0_23" },
	{ 0x6b2c, "YUV_POSTPC_PC1_UV_V_COEFF_1_01" },
	{ 0x6b30, "YUV_POSTPC_PC1_UV_V_COEFF_1_23" },
	{ 0x6b34, "YUV_POSTPC_PC1_UV_V_COEFF_2_01" },
	{ 0x6b38, "YUV_POSTPC_PC1_UV_V_COEFF_2_23" },
	{ 0x6b3c, "YUV_POSTPC_PC1_UV_V_COEFF_3_01" },
	{ 0x6b40, "YUV_POSTPC_PC1_UV_V_COEFF_3_23" },
	{ 0x6b44, "YUV_POSTPC_PC1_UV_V_COEFF_4_01" },
	{ 0x6b48, "YUV_POSTPC_PC1_UV_V_COEFF_4_23" },
	{ 0x6b4c, "YUV_POSTPC_PC1_UV_V_COEFF_5_01" },
	{ 0x6b50, "YUV_POSTPC_PC1_UV_V_COEFF_5_23" },
	{ 0x6b54, "YUV_POSTPC_PC1_UV_V_COEFF_6_01" },
	{ 0x6b58, "YUV_POSTPC_PC1_UV_V_COEFF_6_23" },
	{ 0x6b5c, "YUV_POSTPC_PC1_UV_V_COEFF_7_01" },
	{ 0x6b60, "YUV_POSTPC_PC1_UV_V_COEFF_7_23" },
	{ 0x6b64, "YUV_POSTPC_PC1_UV_V_COEFF_8_01" },
	{ 0x6b68, "YUV_POSTPC_PC1_UV_V_COEFF_8_23" },
	{ 0x6b6c, "YUV_POSTPC_PC1_UV_H_COEFF_0_01" },
	{ 0x6b70, "YUV_POSTPC_PC1_UV_H_COEFF_0_23" },
	{ 0x6b7c, "YUV_POSTPC_PC1_UV_H_COEFF_1_01" },
	{ 0x6b80, "YUV_POSTPC_PC1_UV_H_COEFF_1_23" },
	{ 0x6b8c, "YUV_POSTPC_PC1_UV_H_COEFF_2_01" },
	{ 0x6b90, "YUV_POSTPC_PC1_UV_H_COEFF_2_23" },
	{ 0x6b9c, "YUV_POSTPC_PC1_UV_H_COEFF_3_01" },
	{ 0x6ba0, "YUV_POSTPC_PC1_UV_H_COEFF_3_23" },
	{ 0x6bac, "YUV_POSTPC_PC1_UV_H_COEFF_4_01" },
	{ 0x6bb0, "YUV_POSTPC_PC1_UV_H_COEFF_4_23" },
	{ 0x6bbc, "YUV_POSTPC_PC1_UV_H_COEFF_5_01" },
	{ 0x6bc0, "YUV_POSTPC_PC1_UV_H_COEFF_5_23" },
	{ 0x6bcc, "YUV_POSTPC_PC1_UV_H_COEFF_6_01" },
	{ 0x6bd0, "YUV_POSTPC_PC1_UV_H_COEFF_6_23" },
	{ 0x6bdc, "YUV_POSTPC_PC1_UV_H_COEFF_7_01" },
	{ 0x6be0, "YUV_POSTPC_PC1_UV_H_COEFF_7_23" },
	{ 0x6bec, "YUV_POSTPC_PC1_UV_H_COEFF_8_01" },
	{ 0x6bf0, "YUV_POSTPC_PC1_UV_H_COEFF_8_23" },
	{ 0x6c24, "YUV_POSTPC_PC2_UV_V_COEFF_0_01" },
	{ 0x6c28, "YUV_POSTPC_PC2_UV_V_COEFF_0_23" },
	{ 0x6c2c, "YUV_POSTPC_PC2_UV_V_COEFF_1_01" },
	{ 0x6c30, "YUV_POSTPC_PC2_UV_V_COEFF_1_23" },
	{ 0x6c34, "YUV_POSTPC_PC2_UV_V_COEFF_2_01" },
	{ 0x6c38, "YUV_POSTPC_PC2_UV_V_COEFF_2_23" },
	{ 0x6c3c, "YUV_POSTPC_PC2_UV_V_COEFF_3_01" },
	{ 0x6c40, "YUV_POSTPC_PC2_UV_V_COEFF_3_23" },
	{ 0x6c44, "YUV_POSTPC_PC2_UV_V_COEFF_4_01" },
	{ 0x6c48, "YUV_POSTPC_PC2_UV_V_COEFF_4_23" },
	{ 0x6c4c, "YUV_POSTPC_PC2_UV_V_COEFF_5_01" },
	{ 0x6c50, "YUV_POSTPC_PC2_UV_V_COEFF_5_23" },
	{ 0x6c54, "YUV_POSTPC_PC2_UV_V_COEFF_6_01" },
	{ 0x6c58, "YUV_POSTPC_PC2_UV_V_COEFF_6_23" },
	{ 0x6c5c, "YUV_POSTPC_PC2_UV_V_COEFF_7_01" },
	{ 0x6c60, "YUV_POSTPC_PC2_UV_V_COEFF_7_23" },
	{ 0x6c64, "YUV_POSTPC_PC2_UV_V_COEFF_8_01" },
	{ 0x6c68, "YUV_POSTPC_PC2_UV_V_COEFF_8_23" },
	{ 0x6c6c, "YUV_POSTPC_PC2_UV_H_COEFF_0_01" },
	{ 0x6c70, "YUV_POSTPC_PC2_UV_H_COEFF_0_23" },
	{ 0x6c7c, "YUV_POSTPC_PC2_UV_H_COEFF_1_01" },
	{ 0x6c80, "YUV_POSTPC_PC2_UV_H_COEFF_1_23" },
	{ 0x6c8c, "YUV_POSTPC_PC2_UV_H_COEFF_2_01" },
	{ 0x6c90, "YUV_POSTPC_PC2_UV_H_COEFF_2_23" },
	{ 0x6c9c, "YUV_POSTPC_PC2_UV_H_COEFF_3_01" },
	{ 0x6ca0, "YUV_POSTPC_PC2_UV_H_COEFF_3_23" },
	{ 0x6cac, "YUV_POSTPC_PC2_UV_H_COEFF_4_01" },
	{ 0x6cb0, "YUV_POSTPC_PC2_UV_H_COEFF_4_23" },
	{ 0x6cbc, "YUV_POSTPC_PC2_UV_H_COEFF_5_01" },
	{ 0x6cc0, "YUV_POSTPC_PC2_UV_H_COEFF_5_23" },
	{ 0x6ccc, "YUV_POSTPC_PC2_UV_H_COEFF_6_01" },
	{ 0x6cd0, "YUV_POSTPC_PC2_UV_H_COEFF_6_23" },
	{ 0x6cdc, "YUV_POSTPC_PC2_UV_H_COEFF_7_01" },
	{ 0x6ce0, "YUV_POSTPC_PC2_UV_H_COEFF_7_23" },
	{ 0x6cec, "YUV_POSTPC_PC2_UV_H_COEFF_8_01" },
	{ 0x6cf0, "YUV_POSTPC_PC2_UV_H_COEFF_8_23" },
	{ 0x7000, "YUV_HWFC_SWRESET" },
	{ 0x7004, "YUV_HWFC_MODE" },
	{ 0x7008, "YUV_HWFC_REGION_IDX_BIN" },
	{ 0x700c, "YUV_HWFC_REGION_IDX_GRAY" },
	{ 0x7010, "YUV_HWFC_CURR_REGION" },
	{ 0x7014, "YUV_HWFC_CONFIG_IMAGE_A" },
	{ 0x7018, "YUV_HWFC_TOTAL_IMAGE_BYTE0_A" },
	{ 0x701c, "YUV_HWFC_TOTAL_WIDTH_BYTE0_A" },
	{ 0x7020, "YUV_HWFC_TOTAL_IMAGE_BYTE1_A" },
	{ 0x7024, "YUV_HWFC_TOTAL_WIDTH_BYTE1_A" },
	{ 0x7028, "YUV_HWFC_TOTAL_IMAGE_BYTE2_A" },
	{ 0x702c, "YUV_HWFC_TOTAL_WIDTH_BYTE2_A" },
	{ 0x7030, "YUV_HWFC_CONFIG_IMAGE_B" },
	{ 0x7034, "YUV_HWFC_TOTAL_IMAGE_BYTE0_B" },
	{ 0x7038, "YUV_HWFC_TOTAL_WIDTH_BYTE0_B" },
	{ 0x703c, "YUV_HWFC_TOTAL_IMAGE_BYTE1_B" },
	{ 0x7040, "YUV_HWFC_TOTAL_WIDTH_BYTE1_B" },
	{ 0x7044, "YUV_HWFC_TOTAL_IMAGE_BYTE2_B" },
	{ 0x7048, "YUV_HWFC_TOTAL_WIDTH_BYTE2_B" },
	{ 0x704c, "YUV_HWFC_FRAME_START_SELECT" },
	{ 0x7050, "YUV_HWFC_INDEX_RESET" },
	{ 0x7054, "YUV_HWFC_ENABLE_AUTO_CLEAR" },
	{ 0x7058, "YUV_HWFC_CORE_RESET_INPUT_SEL" },
	{ 0x705c, "YUV_HWFC_MASTER_SEL" },
	{ 0x7060, "YUV_HWFC_IMAGE_HEIGHT" },
	{ 0x7800, "YUV_DTPOTFIN_BYPASS" },
	{ 0x7808, "YUV_DTPOTFIN_TEST_PATTERN_MODE" },
	{ 0x780c, "YUV_DTPOTFIN_TEST_DATA_Y" },
	{ 0x7810, "YUV_DTPOTFIN_TEST_DATA_U" },
	{ 0x7814, "YUV_DTPOTFIN_TEST_DATA_V" },
	{ 0x7818, "YUV_DTPOTFIN_YUV_STANDARD" },
	{ 0x78fc, "YUV_DTPOTFIN_STREAM_CRC" },
	{ 0x7a00, "STAT_DTPDUALLAYER_BYPASS" },
	{ 0x7a08, "STAT_DTPDUALLAYER_TEST_PATTERN_MODE" },
	{ 0x7a0c, "STAT_DTPDUALLAYER_TEST_DATA_Y" },
	{ 0x7a10, "STAT_DTPDUALLAYER_TEST_DATA_U" },
	{ 0x7a14, "STAT_DTPDUALLAYER_TEST_DATA_V" },
	{ 0x7a18, "STAT_DTPDUALLAYER_YUV_STANDARD" },
	{ 0x7afc, "STAT_DTPDUALLAYER_STREAM_CRC" },
	{ 0x7e00, "DBG_AXI_0" },
	{ 0x7e04, "DBG_AXI_1" },
	{ 0x7e08, "DBG_AXI_2" },
	{ 0x7e0c, "DBG_AXI_3" },
	{ 0x7e10, "DBG_AXI_4" },
	{ 0x7e14, "DBG_AXI_5" },
	{ 0x7e18, "DBG_AXI_6" },
	{ 0x7e1c, "DBG_AXI_7" },
	{ 0x7e20, "DBG_AXI_8" },
	{ 0x7e24, "DBG_AXI_9" },
	{ 0x7e28, "DBG_AXI_10" },
	{ 0x7e2c, "DBG_AXI_11" },
	{ 0x7e30, "DBG_AXI_12" },
	{ 0x7e34, "DBG_AXI_13" },
	{ 0x7e38, "DBG_AXI_14" },
	{ 0x7e3c, "DBG_AXI_15" },
	{ 0x7e40, "DBG_AXI_16" },
	{ 0x7e44, "DBG_AXI_17" },
	{ 0x7e48, "DBG_AXI_18" },
	{ 0x7e4c, "DBG_AXI_19" },
	{ 0x7e50, "DBG_AXI_20" },
	{ 0x7e54, "DBG_AXI_21" },
	{ 0x7e58, "DBG_AXI_22" },
	{ 0x7e5c, "DBG_AXI_23" },
	{ 0x7e60, "DBG_AXI_24" },
	{ 0x7e64, "DBG_AXI_25" },
	{ 0x7e68, "DBG_AXI_26" },
	{ 0x7e6c, "DBG_AXI_27" },
	{ 0x7e70, "DBG_AXI_28" },
	{ 0x7e74, "DBG_AXI_29" },
	{ 0x7e78, "DBG_AXI_30" },
	{ 0x7e7c, "DBG_AXI_31" },
	{ 0x7e80, "DBG_AXI_32" },
	{ 0x7e84, "DBG_AXI_33" },
	{ 0x7e88, "DBG_AXI_34" },
	{ 0x7e8c, "DBG_AXI_35" },
	{ 0x7e90, "DBG_AXI_36" },
	{ 0x7e94, "DBG_AXI_37" },
	{ 0x7e98, "DBG_AXI_38" },
	{ 0x7e9c, "DBG_AXI_39" },
	{ 0x7ea0, "DBG_AXI_40" },
	{ 0x7ea4, "DBG_AXI_41" },
	{ 0x7ea8, "DBG_AXI_42" },
	{ 0x7eac, "DBG_AXI_43" },
	{ 0x7eb0, "DBG_AXI_44" },
	{ 0x7eb4, "DBG_AXI_45" },
	{ 0x7eb8, "DBG_AXI_46" },
	{ 0x7ebc, "DBG_AXI_47" },
	{ 0x7ec0, "DBG_AXI_48" },
	{ 0x7ec4, "DBG_AXI_49" },
	{ 0x7ee8, "DBG_AXI_58" },
	{ 0x7eec, "DBG_AXI_59" },
	{ 0x7ef0, "DBG_AXI_60" },
	{ 0x7ef4, "DBG_AXI_61" },
	{ 0x7f00, "DBG_SC_0" },
	{ 0x7f04, "DBG_SC_1" },
	{ 0x7f08, "DBG_SC_2" },
	{ 0x7f0c, "DBG_SC_3" },
	{ 0x7f10, "DBG_SC_4" },
	{ 0x7f14, "DBG_SC_5" },
	{ 0x7f18, "DBG_SC_6" },
	{ 0x7f1c, "DBG_SC_7" },
	{ 0x7f20, "DBG_SC_8" },
	{ 0x7f24, "DBG_SC_9" },
	{ 0x7f28, "DBG_SC_10" },
	{ 0x7f2c, "DBG_SC_11" },
	{ 0x7f30, "DBG_SC_12" },
	{ 0x7f34, "DBG_SC_13" },
	{ 0x7f38, "DBG_SC_14" },
	{ 0x7f3c, "DBG_SC_15" },
	{ 0x7f40, "DBG_SC_16" },
	{ 0x7f44, "DBG_SC_17" },
	{ 0x7f48, "DBG_SC_18" },
	{ 0x7f4c, "DBG_SC_19" },
	{ 0x7f50, "DBG_SC_20" },
	{ 0x7f54, "DBG_SC_21" },
	{ 0x7f58, "DBG_SC_22" },
	{ 0x7f5c, "DBG_SC_23" },
	{ 0x7f60, "DBG_SC_24" },
	{ 0x7f64, "DBG_SC_25" },
	{ 0x7f68, "DBG_SC_26" },
	{ 0x7f6c, "DBG_SC_27" },
	{ 0x7f70, "DBG_SC_28" },
	{ 0x7f74, "DBG_SC_29" },
	{ 0x7f78, "DBG_SC_30" },
	{ 0x7f7c, "DBG_SC_31" },
	{ 0x7f80, "DBG_SC_32" },
	{ 0x7f84, "DBG_SC_33" },
	{ 0x7f88, "DBG_SC_34" },
	{ 0x7f8c, "CRC_RESULT_0" },
	{ 0x7f90, "CRC_RESULT_1" },
	{ 0x7f94, "CRC_RESULT_2" },
	{ 0x7f98, "CRC_RESULT_3" },
	{ 0x7f9c, "CRC_RESULT_4" },
	{ 0x7fa0, "CRC_RESULT_5" },
	{ 0x7fa4, "CRC_RESULT_6" },
	{ 0x7fa8, "CRC_RESULT_7" },
	{ 0x7fac, "CRC_RESULT_8" },
	{ 0x7fb0, "CRC_RESULT_9" },
	{ 0x7fb4, "CRC_RESULT_10" },
	{ 0x7fb8, "CRC_RESULT_11" },
	{ 0x7fbc, "CRC_RESULT_12" },
	{ 0x7fc0, "CRC_RESULT_13" },
	{ 0x7fc4, "DBG_SC_35" },
};

#define MCSC_R_CMDQ_ENABLE 0x0000
#define MCSC_R_CMDQ_STOP_CRPT_ENABLE 0x0008
#define MCSC_R_SW_RESET 0x0010
#define MCSC_R_SW_CORE_RESET 0x0014
#define MCSC_R_SW_APB_RESET 0x0018
#define MCSC_R_TRANS_STOP_REQ 0x001c
#define MCSC_R_TRANS_STOP_REQ_RDY 0x0020
#define MCSC_R_IP_APG_MODE 0x0028
#define MCSC_R_IP_CLOCK_DOWN_MODE 0x002c
#define MCSC_R_IP_PROCESSING 0x0030
#define MCSC_R_FORCE_INTERNAL_CLOCK 0x0034
#define MCSC_R_DEBUG_CLOCK_ENABLE 0x0038
#define MCSC_R_IP_POST_FRAME_GAP 0x003c
#define MCSC_R_IP_DRCG_ENABLE 0x0040
#define MCSC_R_AUTO_IGNORE_INTERRUPT_ENABLE 0x0050
#define MCSC_R_IP_USE_SW_FINISH_COND 0x0058
#define MCSC_R_SW_FINISH_COND_ENABLE 0x005c
#define MCSC_R_IP_CORRUPTED_COND_ENABLE 0x006c
#define MCSC_R_IP_USE_OTF_PATH_67 0x0074
#define MCSC_R_IP_USE_OTF_PATH_45 0x0078
#define MCSC_R_IP_USE_OTF_PATH_23 0x007c
#define MCSC_R_IP_USE_OTF_PATH_01 0x0080
#define MCSC_R_IP_USE_CINFIFO_NEW_FRAME_IN 0x0084
#define MCSC_R_SECU_CTRL_TZINFO_ICTRL 0x00c8
#define MCSC_R_ICTRL_CSUB_BASE_ADDR 0x00d0
#define MCSC_R_ICTRL_CSUB_RECV_TURN_OFF_MSG 0x00d4
#define MCSC_R_ICTRL_CSUB_RECV_IP_INFO_MSG 0x00d8
#define MCSC_R_ICTRL_CSUB_CONNECTION_TEST_MSG 0x00dc
#define MCSC_R_ICTRL_CSUB_MSG_SEND_ENABLE 0x00e0
#define MCSC_R_ICTRL_CSUB_INT0_EV_ENABLE 0x00e4
#define MCSC_R_ICTRL_CSUB_INT1_EV_ENABLE 0x00e8
#define MCSC_R_ICTRL_CSUB_IP_S_EV_ENABLE 0x00ec
#define MCSC_R_YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_WIDTH 0x0210
#define MCSC_R_YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_HEIGHT 0x0214
#define MCSC_R_YUV_MAIN_CTRL_FRO_EN 0x0240
#define MCSC_R_YUV_MAIN_CTRL_INPUT_MONO_EN 0x0244
#define MCSC_R_YUV_MAIN_CTRL_CRC_EN 0x02a0
#define MCSC_R_YUV_MAIN_CTRL_STALL_THROTTLE_CTRL 0x02a4
#define MCSC_R_YUV_MAIN_CTRL_DJAG_SC_HBI 0x02f4
#define MCSC_R_YUV_MAIN_CTRL_AXI_TRAFFIC_WR_SEL 0x02fc
#define MCSC_R_CMDQ_QUE_CMD_H 0x0400
#define MCSC_R_CMDQ_QUE_CMD_M 0x0404
#define MCSC_R_CMDQ_QUE_CMD_L 0x0408
#define MCSC_R_CMDQ_ADD_TO_QUEUE_0 0x040c
#define MCSC_R_CMDQ_AUTO_CONV_ENABLE 0x0418
#define MCSC_R_CMDQ_LOCK 0x0440
#define MCSC_R_CMDQ_CTRL_SETSEL_EN 0x0450
#define MCSC_R_CMDQ_SETSEL 0x0454
#define MCSC_R_CMDQ_FLUSH_QUEUE_0 0x0460
#define MCSC_R_CMDQ_SWAP_QUEUE_0 0x046c
#define MCSC_R_CMDQ_ROTATE_QUEUE_0 0x0478
#define MCSC_R_CMDQ_HOLD_MARK_QUEUE_0 0x0484
#define MCSC_R_CMDQ_DEBUG_STATUS_PRE_LOAD 0x0494
#define MCSC_R_CMDQ_VHD_CONTROL 0x049c
#define MCSC_R_CMDQ_FRAME_COUNTER_INC_TYPE 0x04a0
#define MCSC_R_CMDQ_FRAME_COUNTER_RESET 0x04a4
#define MCSC_R_CMDQ_FRAME_COUNTER 0x04a8
#define MCSC_R_CMDQ_FRAME_ID 0x04ac
#define MCSC_R_CMDQ_QUEUE_0_INFO 0x04b0
#define MCSC_R_CMDQ_QUEUE_0_RPTR_FOR_DEBUG 0x04b4
#define MCSC_R_CMDQ_DEBUG_QUE_0_CMD_H 0x04b8
#define MCSC_R_CMDQ_DEBUG_QUE_0_CMD_M 0x04bc
#define MCSC_R_CMDQ_DEBUG_QUE_0_CMD_L 0x04c0
#define MCSC_R_CMDQ_DEBUG_STATUS 0x04ec
#define MCSC_R_CMDQ_INT 0x04f0
#define MCSC_R_CMDQ_INT_ENABLE 0x04f4
#define MCSC_R_CMDQ_INT_STATUS 0x04f8
#define MCSC_R_CMDQ_INT_CLEAR 0x04fc
#define MCSC_R_C_LOADER_ENABLE 0x0500
#define MCSC_R_C_LOADER_RESET 0x0504
#define MCSC_R_C_LOADER_FAST_MODE 0x0508
#define MCSC_R_C_LOADER_REMAP_EN 0x050c
#define MCSC_R_C_LOADER_ACCESS_INTERVAL 0x0510
#define MCSC_R_C_LOADER_REMAP_00_ADDR 0x0540
#define MCSC_R_C_LOADER_REMAP_01_ADDR 0x0544
#define MCSC_R_C_LOADER_REMAP_02_ADDR 0x0548
#define MCSC_R_C_LOADER_REMAP_03_ADDR 0x054c
#define MCSC_R_C_LOADER_REMAP_04_ADDR 0x0550
#define MCSC_R_C_LOADER_REMAP_05_ADDR 0x0554
#define MCSC_R_C_LOADER_REMAP_06_ADDR 0x0558
#define MCSC_R_C_LOADER_REMAP_07_ADDR 0x055c
#define MCSC_R_C_LOADER_LOGICAL_OFFSET_EN 0x0580
#define MCSC_R_C_LOADER_LOGICAL_OFFSET 0x0584
#define MCSC_R_C_LOADER_DEBUG_STATUS 0x05c0
#define MCSC_R_C_LOADER_DEBUG_HEADER_REQ_COUNTER 0x05c4
#define MCSC_R_C_LOADER_DEBUG_HEADER_APB_COUNTER 0x05c8
#define MCSC_R_C_LOADER_HEADER_CRC_SEED 0x05e0
#define MCSC_R_C_LOADER_PAYLOAD_CRC_SEED 0x05e4
#define MCSC_R_C_LOADER_HEADER_CRC_RESULT 0x05f0
#define MCSC_R_C_LOADER_PAYLOAD_CRC_RESULT 0x05f4
#define MCSC_R_COREX_ENABLE 0x0600
#define MCSC_R_COREX_RESET 0x0604
#define MCSC_R_COREX_FAST_MODE 0x0608
#define MCSC_R_COREX_UPDATE_TYPE_0 0x060c
#define MCSC_R_COREX_UPDATE_TYPE_1 0x0610
#define MCSC_R_COREX_UPDATE_MODE_0 0x0614
#define MCSC_R_COREX_UPDATE_MODE_1 0x0618
#define MCSC_R_COREX_START_0 0x061c
#define MCSC_R_COREX_START_1 0x0620
#define MCSC_R_COREX_COPY_FROM_IP_0 0x0624
#define MCSC_R_COREX_COPY_FROM_IP_1 0x0628
#define MCSC_R_COREX_STATUS_0 0x062c
#define MCSC_R_COREX_STATUS_1 0x0630
#define MCSC_R_COREX_PRE_ADDR_CONFIG 0x0634
#define MCSC_R_COREX_PRE_DATA_CONFIG 0x0638
#define MCSC_R_COREX_POST_ADDR_CONFIG 0x063c
#define MCSC_R_COREX_POST_DATA_CONFIG 0x0640
#define MCSC_R_COREX_PRE_POST_CONFIG_EN 0x0644
#define MCSC_R_COREX_TYPE_WRITE 0x0648
#define MCSC_R_COREX_TYPE_WRITE_TRIGGER 0x064c
#define MCSC_R_COREX_TYPE_READ 0x0650
#define MCSC_R_COREX_TYPE_READ_OFFSET 0x0654
#define MCSC_R_COREX_INT 0x0658
#define MCSC_R_COREX_INT_STATUS 0x065c
#define MCSC_R_COREX_INT_CLEAR 0x0660
#define MCSC_R_COREX_INT_ENABLE 0x0664
#define MCSC_R_INT_REQ_INT0 0x0800
#define MCSC_R_INT_REQ_INT0_ENABLE 0x0804
#define MCSC_R_INT_REQ_INT0_STATUS 0x0808
#define MCSC_R_INT_REQ_INT0_CLEAR 0x080c
#define MCSC_R_INT_REQ_INT1 0x0810
#define MCSC_R_INT_REQ_INT1_ENABLE 0x0814
#define MCSC_R_INT_REQ_INT1_STATUS 0x0818
#define MCSC_R_INT_REQ_INT1_CLEAR 0x081c
#define MCSC_R_INT_HIST_CURINT0 0x0900
#define MCSC_R_INT_HIST_CURINT0_ENABLE 0x0904
#define MCSC_R_INT_HIST_CURINT0_STATUS 0x0908
#define MCSC_R_INT_HIST_CURINT1 0x090c
#define MCSC_R_INT_HIST_CURINT1_ENABLE 0x0910
#define MCSC_R_INT_HIST_CURINT1_STATUS 0x0914
#define MCSC_R_INT_HIST_00_FRAME_ID 0x0918
#define MCSC_R_INT_HIST_00_INT0 0x091c
#define MCSC_R_INT_HIST_00_INT1 0x0920
#define MCSC_R_INT_HIST_01_FRAME_ID 0x0924
#define MCSC_R_INT_HIST_01_INT0 0x0928
#define MCSC_R_INT_HIST_01_INT1 0x092c
#define MCSC_R_INT_HIST_02_FRAME_ID 0x0930
#define MCSC_R_INT_HIST_02_INT0 0x0934
#define MCSC_R_INT_HIST_02_INT1 0x0938
#define MCSC_R_INT_HIST_03_FRAME_ID 0x093c
#define MCSC_R_INT_HIST_03_INT0 0x0940
#define MCSC_R_INT_HIST_03_INT1 0x0944
#define MCSC_R_INT_HIST_04_FRAME_ID 0x0948
#define MCSC_R_INT_HIST_04_INT0 0x094c
#define MCSC_R_INT_HIST_04_INT1 0x0950
#define MCSC_R_INT_HIST_05_FRAME_ID 0x0954
#define MCSC_R_INT_HIST_05_INT0 0x0958
#define MCSC_R_INT_HIST_05_INT1 0x095c
#define MCSC_R_INT_HIST_06_FRAME_ID 0x0960
#define MCSC_R_INT_HIST_06_INT0 0x0964
#define MCSC_R_INT_HIST_06_INT1 0x0968
#define MCSC_R_INT_HIST_07_FRAME_ID 0x096c
#define MCSC_R_INT_HIST_07_INT0 0x0970
#define MCSC_R_INT_HIST_07_INT1 0x0974
#define MCSC_R_SECU_CTRL_SEQID 0x0b00
#define MCSC_R_SECU_CTRL_TZINFO_SEQID_0 0x0b10
#define MCSC_R_SECU_CTRL_TZINFO_SEQID_1 0x0b14
#define MCSC_R_SECU_CTRL_TZINFO_SEQID_2 0x0b18
#define MCSC_R_SECU_CTRL_TZINFO_SEQID_3 0x0b1c
#define MCSC_R_SECU_CTRL_TZINFO_SEQID_4 0x0b20
#define MCSC_R_SECU_CTRL_TZINFO_SEQID_5 0x0b24
#define MCSC_R_SECU_CTRL_TZINFO_SEQID_6 0x0b28
#define MCSC_R_SECU_CTRL_TZINFO_SEQID_7 0x0b2c
#define MCSC_R_SECU_OTF_SEQ_ID_PROT_ENABLE 0x0b58
#define MCSC_R_PERF_MONITOR_ENABLE 0x0c00
#define MCSC_R_PERF_MONITOR_CLEAR 0x0c04
#define MCSC_R_PERF_MONITOR_INT_USER_SEL 0x0c08
#define MCSC_R_PERF_MONITOR_INT_START 0x0c40
#define MCSC_R_PERF_MONITOR_INT_END 0x0c44
#define MCSC_R_PERF_MONITOR_INT_USER 0x0c48
#define MCSC_R_PERF_MONITOR_PROCESS_PRE_CONFIG 0x0c4c
#define MCSC_R_PERF_MONITOR_PROCESS_FRAME 0x0c50
#define MCSC_R_IP_VERSION 0x0d00
#define MCSC_R_COMMON_CTRL_VERSION 0x0d04
#define MCSC_R_QCH_STATUS 0x0d08
#define MCSC_R_IDLENESS_STATUS 0x0d0c
#define MCSC_R_DEBUG_COUNTER_SIG_SEL 0x0d2c
#define MCSC_R_DEBUG_COUNTER_0 0x0d30
#define MCSC_R_DEBUG_COUNTER_1 0x0d34
#define MCSC_R_DEBUG_COUNTER_2 0x0d38
#define MCSC_R_DEBUG_COUNTER_3 0x0d3c
#define MCSC_R_IP_BUSY_MONITOR_0 0x0d40
#define MCSC_R_IP_BUSY_MONITOR_1 0x0d44
#define MCSC_R_IP_BUSY_MONITOR_2 0x0d48
#define MCSC_R_IP_BUSY_MONITOR_3 0x0d4c
#define MCSC_R_IP_STALL_OUT_STATUS_0 0x0d60
#define MCSC_R_IP_STALL_OUT_STATUS_1 0x0d64
#define MCSC_R_IP_STALL_OUT_STATUS_2 0x0d68
#define MCSC_R_IP_STALL_OUT_STATUS_3 0x0d6c
#define MCSC_R_STOPEN_CRC_STOP_VALID_COUNT 0x0d80
#define MCSC_R_SFR_ACCESS_LOG_ENABLE 0x0d88
#define MCSC_R_SFR_ACCESS_LOG_CLEAR 0x0d8c
#define MCSC_R_SFR_ACCESS_LOG_0 0x0d90
#define MCSC_R_SFR_ACCESS_LOG_0_ADDRESS 0x0d94
#define MCSC_R_SFR_ACCESS_LOG_1 0x0d98
#define MCSC_R_SFR_ACCESS_LOG_1_ADDRESS 0x0d9c
#define MCSC_R_SFR_ACCESS_LOG_2 0x0da0
#define MCSC_R_SFR_ACCESS_LOG_2_ADDRESS 0x0da4
#define MCSC_R_SFR_ACCESS_LOG_3 0x0da8
#define MCSC_R_SFR_ACCESS_LOG_3_ADDRESS 0x0dac
#define MCSC_R_IP_ROL_RESET 0x0dd0
#define MCSC_R_IP_ROL_MODE 0x0dd4
#define MCSC_R_IP_ROL_SELECT 0x0dd8
#define MCSC_R_IP_INT_ON_COL_ROW 0x0de0
#define MCSC_R_IP_INT_ON_COL_ROW_POS 0x0de4
#define MCSC_R_FREEZE_FOR_DEBUG 0x0de8
#define MCSC_R_FREEZE_EXTENSION_ENABLE 0x0dec
#define MCSC_R_FREEZE_EN 0x0df0
#define MCSC_R_FREEZE_COL_ROW_POS 0x0df4
#define MCSC_R_FREEZE_CORRUPTED_ENABLE 0x0df8
#define MCSC_R_YUV_CINFIFO_ENABLE 0x0e00
#define MCSC_R_YUV_CINFIFO_CONFIG 0x0e04
#define MCSC_R_YUV_CINFIFO_STALL_CTRL 0x0e08
#define MCSC_R_YUV_CINFIFO_INTERVAL_VBLANK 0x0e0c
#define MCSC_R_YUV_CINFIFO_INTERVALS 0x0e10
#define MCSC_R_YUV_CINFIFO_STATUS 0x0e14
#define MCSC_R_YUV_CINFIFO_INPUT_CNT 0x0e18
#define MCSC_R_YUV_CINFIFO_STALL_CNT 0x0e1c
#define MCSC_R_YUV_CINFIFO_FIFO_FULLNESS 0x0e20
#define MCSC_R_YUV_CINFIFO_INT 0x0e40
#define MCSC_R_YUV_CINFIFO_INT_ENABLE 0x0e44
#define MCSC_R_YUV_CINFIFO_INT_STATUS 0x0e48
#define MCSC_R_YUV_CINFIFO_INT_CLEAR 0x0e4c
#define MCSC_R_YUV_CINFIFO_CORRUPTED_COND_ENABLE 0x0e50
#define MCSC_R_YUV_CINFIFO_ROL_SELECT 0x0e54
#define MCSC_R_YUV_CINFIFO_INTERVAL_VBLANK_AR 0x0e70
#define MCSC_R_YUV_CINFIFO_INTERVAL_HBLANK_AR 0x0e74
#define MCSC_R_YUV_CINFIFO_STREAM_CRC 0x0e7c
#define MCSC_R_STAT_CINFIFODUALLAYER_ENABLE 0x0e80
#define MCSC_R_STAT_CINFIFODUALLAYER_CONFIG 0x0e84
#define MCSC_R_STAT_CINFIFODUALLAYER_STALL_CTRL 0x0e88
#define MCSC_R_STAT_CINFIFODUALLAYER_INTERVAL_VBLANK 0x0e8c
#define MCSC_R_STAT_CINFIFODUALLAYER_INTERVALS 0x0e90
#define MCSC_R_STAT_CINFIFODUALLAYER_STATUS 0x0e94
#define MCSC_R_STAT_CINFIFODUALLAYER_INPUT_CNT 0x0e98
#define MCSC_R_STAT_CINFIFODUALLAYER_STALL_CNT 0x0e9c
#define MCSC_R_STAT_CINFIFODUALLAYER_FIFO_FULLNESS 0x0ea0
#define MCSC_R_STAT_CINFIFODUALLAYER_INT 0x0ec0
#define MCSC_R_STAT_CINFIFODUALLAYER_INT_ENABLE 0x0ec4
#define MCSC_R_STAT_CINFIFODUALLAYER_INT_STATUS 0x0ec8
#define MCSC_R_STAT_CINFIFODUALLAYER_INT_CLEAR 0x0ecc
#define MCSC_R_STAT_CINFIFODUALLAYER_CORRUPTED_COND_ENABLE 0x0ed0
#define MCSC_R_STAT_CINFIFODUALLAYER_ROL_SELECT 0x0ed4
#define MCSC_R_STAT_CINFIFODUALLAYER_INTERVAL_VBLANK_AR 0x0ef0
#define MCSC_R_STAT_CINFIFODUALLAYER_INTERVAL_HBLANK_AR 0x0ef4
#define MCSC_R_STAT_CINFIFODUALLAYER_STREAM_CRC 0x0efc
#define MCSC_R_STAT_RDMACL_ENABLE 0x1600
#define MCSC_R_STAT_RDMACL_COMP_CTRL 0x1604
#define MCSC_R_STAT_RDMACL_DATA_FORMAT 0x1610
#define MCSC_R_STAT_RDMACL_MONO_MODE 0x1614
#define MCSC_R_STAT_RDMACL_WIDTH 0x1620
#define MCSC_R_STAT_RDMACL_HEIGHT 0x1624
#define MCSC_R_STAT_RDMACL_STRIDE_1P 0x1628
#define MCSC_R_STAT_RDMACL_MAX_MO 0x1640
#define MCSC_R_STAT_RDMACL_LINE_GAP 0x1644
#define MCSC_R_STAT_RDMACL_BUSINFO 0x164c
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0 0x1650
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1 0x1654
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2 0x1658
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3 0x165c
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4 0x1660
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5 0x1664
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6 0x1668
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7 0x166c
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0_LSB_4B 0x1670
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1_LSB_4B 0x1674
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2_LSB_4B 0x1678
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3_LSB_4B 0x167c
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4_LSB_4B 0x1680
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5_LSB_4B 0x1684
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6_LSB_4B 0x1688
#define MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7_LSB_4B 0x168c
#define MCSC_R_STAT_RDMACL_IMG_CRC_1P 0x1790
#define MCSC_R_STAT_RDMACL_MON_STATUS_0 0x17b0
#define MCSC_R_STAT_RDMACL_MON_STATUS_1 0x17b4
#define MCSC_R_STAT_RDMACL_MON_STATUS_2 0x17b8
#define MCSC_R_STAT_RDMACL_MON_STATUS_3 0x17bc
#define MCSC_R_STAT_RDMACL_AXI_DEBUG_CONTROL 0x17e4
#define MCSC_R_YUV_WDMASC_W0_ENABLE 0x2000
#define MCSC_R_YUV_WDMASC_W0_COMP_CTRL 0x2004
#define MCSC_R_YUV_WDMASC_W0_DATA_FORMAT 0x2010
#define MCSC_R_YUV_WDMASC_W0_MONO_CTRL 0x2014
#define MCSC_R_YUV_WDMASC_W0_COMP_LOSSY_QUALITY_CONTROL 0x2018
#define MCSC_R_YUV_WDMASC_W0_WIDTH 0x2020
#define MCSC_R_YUV_WDMASC_W0_HEIGHT 0x2024
#define MCSC_R_YUV_WDMASC_W0_STRIDE_1P 0x2028
#define MCSC_R_YUV_WDMASC_W0_STRIDE_2P 0x202c
#define MCSC_R_YUV_WDMASC_W0_STRIDE_3P 0x2030
#define MCSC_R_YUV_WDMASC_W0_STRIDE_HEADER_1P 0x2034
#define MCSC_R_YUV_WDMASC_W0_STRIDE_HEADER_2P 0x2038
#define MCSC_R_YUV_WDMASC_W0_VOTF_EN 0x203c
#define MCSC_R_YUV_WDMASC_W0_MAX_MO 0x2040
#define MCSC_R_YUV_WDMASC_W0_BUSINFO 0x204c
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_0 0x2050
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_1 0x2054
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_2 0x2058
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_3 0x205c
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_4 0x2060
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_5 0x2064
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_6 0x2068
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_7 0x206c
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0 0x2070
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1 0x2074
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2 0x2078
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3 0x207c
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4 0x2080
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5 0x2084
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6 0x2088
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7 0x208c
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_0 0x2090
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_1 0x2094
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_2 0x2098
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_3 0x209c
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_4 0x20a0
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_5 0x20a4
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_6 0x20a8
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_7 0x20ac
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0 0x20b0
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1 0x20b4
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2 0x20b8
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3 0x20bc
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4 0x20c0
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5 0x20c4
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6 0x20c8
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7 0x20cc
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_0 0x20d0
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_1 0x20d4
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_2 0x20d8
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_3 0x20dc
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_4 0x20e0
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_5 0x20e4
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_6 0x20e8
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_7 0x20ec
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0 0x20f0
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1 0x20f4
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2 0x20f8
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3 0x20fc
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4 0x2100
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5 0x2104
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6 0x2108
#define MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7 0x210c
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_0 0x2110
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_1 0x2114
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_2 0x2118
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_3 0x211c
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_4 0x2120
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_5 0x2124
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_6 0x2128
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_7 0x212c
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0 0x2130
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1 0x2134
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2 0x2138
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3 0x213c
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4 0x2140
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5 0x2144
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6 0x2148
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7 0x214c
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_0 0x2150
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_1 0x2154
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_2 0x2158
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_3 0x215c
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_4 0x2160
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_5 0x2164
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_6 0x2168
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_7 0x216c
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0 0x2170
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1 0x2174
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2 0x2178
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3 0x217c
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4 0x2180
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5 0x2184
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6 0x2188
#define MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7 0x218c
#define MCSC_R_YUV_WDMASC_W0_IMG_CRC_1P 0x2190
#define MCSC_R_YUV_WDMASC_W0_IMG_CRC_2P 0x2194
#define MCSC_R_YUV_WDMASC_W0_IMG_CRC_3P 0x2198
#define MCSC_R_YUV_WDMASC_W0_HEADER_CRC_1P 0x219c
#define MCSC_R_YUV_WDMASC_W0_HEADER_CRC_2P 0x21a0
#define MCSC_R_YUV_WDMASC_W0_MON_STATUS_0 0x21b0
#define MCSC_R_YUV_WDMASC_W0_MON_STATUS_1 0x21b4
#define MCSC_R_YUV_WDMASC_W0_MON_STATUS_2 0x21b8
#define MCSC_R_YUV_WDMASC_W0_MON_STATUS_3 0x21bc
#define MCSC_R_YUV_WDMASC_W0_BW_LIMIT_0 0x21d0
#define MCSC_R_YUV_WDMASC_W0_BW_LIMIT_1 0x21d4
#define MCSC_R_YUV_WDMASC_W0_BW_LIMIT_2 0x21d8
#define MCSC_R_YUV_WDMASC_W0_CACHE_CONTROL 0x21e0
#define MCSC_R_YUV_WDMASC_W0_DEBUG_CONTROL 0x21e4
#define MCSC_R_YUV_WDMASC_W0_DEBUG_0 0x21e8
#define MCSC_R_YUV_WDMASC_W0_DEBUG_1 0x21ec
#define MCSC_R_YUV_WDMASC_W0_DEBUG_2 0x21f0
#define MCSC_R_YUV_WDMASC_W0_FLIP_CONTROL 0x21f8
#define MCSC_R_YUV_WDMASC_W0_RGB_ALPHA 0x21fc
#define MCSC_R_YUV_WDMASC_W1_ENABLE 0x2200
#define MCSC_R_YUV_WDMASC_W1_COMP_CTRL 0x2204
#define MCSC_R_YUV_WDMASC_W1_DATA_FORMAT 0x2210
#define MCSC_R_YUV_WDMASC_W1_MONO_CTRL 0x2214
#define MCSC_R_YUV_WDMASC_W1_COMP_LOSSY_QUALITY_CONTROL 0x2218
#define MCSC_R_YUV_WDMASC_W1_WIDTH 0x2220
#define MCSC_R_YUV_WDMASC_W1_HEIGHT 0x2224
#define MCSC_R_YUV_WDMASC_W1_STRIDE_1P 0x2228
#define MCSC_R_YUV_WDMASC_W1_STRIDE_2P 0x222c
#define MCSC_R_YUV_WDMASC_W1_STRIDE_3P 0x2230
#define MCSC_R_YUV_WDMASC_W1_STRIDE_HEADER_1P 0x2234
#define MCSC_R_YUV_WDMASC_W1_STRIDE_HEADER_2P 0x2238
#define MCSC_R_YUV_WDMASC_W1_VOTF_EN 0x223c
#define MCSC_R_YUV_WDMASC_W1_MAX_MO 0x2240
#define MCSC_R_YUV_WDMASC_W1_BUSINFO 0x224c
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_0 0x2250
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_1 0x2254
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_2 0x2258
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_3 0x225c
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_4 0x2260
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_5 0x2264
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_6 0x2268
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_7 0x226c
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0 0x2270
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1 0x2274
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2 0x2278
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3 0x227c
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4 0x2280
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5 0x2284
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6 0x2288
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7 0x228c
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_0 0x2290
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_1 0x2294
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_2 0x2298
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_3 0x229c
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_4 0x22a0
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_5 0x22a4
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_6 0x22a8
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_7 0x22ac
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0 0x22b0
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1 0x22b4
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2 0x22b8
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3 0x22bc
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4 0x22c0
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5 0x22c4
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6 0x22c8
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7 0x22cc
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_0 0x22d0
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_1 0x22d4
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_2 0x22d8
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_3 0x22dc
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_4 0x22e0
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_5 0x22e4
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_6 0x22e8
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_7 0x22ec
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0 0x22f0
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1 0x22f4
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2 0x22f8
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3 0x22fc
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4 0x2300
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5 0x2304
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6 0x2308
#define MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7 0x230c
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_0 0x2310
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_1 0x2314
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_2 0x2318
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_3 0x231c
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_4 0x2320
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_5 0x2324
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_6 0x2328
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_7 0x232c
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0 0x2330
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1 0x2334
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2 0x2338
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3 0x233c
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4 0x2340
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5 0x2344
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6 0x2348
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7 0x234c
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_0 0x2350
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_1 0x2354
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_2 0x2358
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_3 0x235c
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_4 0x2360
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_5 0x2364
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_6 0x2368
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_7 0x236c
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0 0x2370
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1 0x2374
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2 0x2378
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3 0x237c
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4 0x2380
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5 0x2384
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6 0x2388
#define MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7 0x238c
#define MCSC_R_YUV_WDMASC_W1_IMG_CRC_1P 0x2390
#define MCSC_R_YUV_WDMASC_W1_IMG_CRC_2P 0x2394
#define MCSC_R_YUV_WDMASC_W1_IMG_CRC_3P 0x2398
#define MCSC_R_YUV_WDMASC_W1_HEADER_CRC_1P 0x239c
#define MCSC_R_YUV_WDMASC_W1_HEADER_CRC_2P 0x23a0
#define MCSC_R_YUV_WDMASC_W1_MON_STATUS_0 0x23b0
#define MCSC_R_YUV_WDMASC_W1_MON_STATUS_1 0x23b4
#define MCSC_R_YUV_WDMASC_W1_MON_STATUS_2 0x23b8
#define MCSC_R_YUV_WDMASC_W1_MON_STATUS_3 0x23bc
#define MCSC_R_YUV_WDMASC_W1_BW_LIMIT_0 0x23d0
#define MCSC_R_YUV_WDMASC_W1_BW_LIMIT_1 0x23d4
#define MCSC_R_YUV_WDMASC_W1_BW_LIMIT_2 0x23d8
#define MCSC_R_YUV_WDMASC_W1_CACHE_CONTROL 0x23e0
#define MCSC_R_YUV_WDMASC_W1_DEBUG_CONTROL 0x23e4
#define MCSC_R_YUV_WDMASC_W1_DEBUG_0 0x23e8
#define MCSC_R_YUV_WDMASC_W1_DEBUG_1 0x23ec
#define MCSC_R_YUV_WDMASC_W1_DEBUG_2 0x23f0
#define MCSC_R_YUV_WDMASC_W1_FLIP_CONTROL 0x23f8
#define MCSC_R_YUV_WDMASC_W1_RGB_ALPHA 0x23fc
#define MCSC_R_YUV_WDMASC_W2_ENABLE 0x2400
#define MCSC_R_YUV_WDMASC_W2_DATA_FORMAT 0x2410
#define MCSC_R_YUV_WDMASC_W2_MONO_CTRL 0x2414
#define MCSC_R_YUV_WDMASC_W2_WIDTH 0x2420
#define MCSC_R_YUV_WDMASC_W2_HEIGHT 0x2424
#define MCSC_R_YUV_WDMASC_W2_STRIDE_1P 0x2428
#define MCSC_R_YUV_WDMASC_W2_STRIDE_2P 0x242c
#define MCSC_R_YUV_WDMASC_W2_STRIDE_3P 0x2430
#define MCSC_R_YUV_WDMASC_W2_VOTF_EN 0x243c
#define MCSC_R_YUV_WDMASC_W2_MAX_MO 0x2440
#define MCSC_R_YUV_WDMASC_W2_BUSINFO 0x244c
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_0 0x2450
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_1 0x2454
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_2 0x2458
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_3 0x245c
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_4 0x2460
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_5 0x2464
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_6 0x2468
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_7 0x246c
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0 0x2470
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1 0x2474
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2 0x2478
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3 0x247c
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4 0x2480
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5 0x2484
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6 0x2488
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7 0x248c
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_0 0x2490
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_1 0x2494
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_2 0x2498
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_3 0x249c
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_4 0x24a0
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_5 0x24a4
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_6 0x24a8
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_7 0x24ac
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0 0x24b0
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1 0x24b4
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2 0x24b8
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3 0x24bc
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4 0x24c0
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5 0x24c4
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6 0x24c8
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7 0x24cc
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_0 0x24d0
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_1 0x24d4
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_2 0x24d8
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_3 0x24dc
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_4 0x24e0
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_5 0x24e4
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_6 0x24e8
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_7 0x24ec
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0 0x24f0
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1 0x24f4
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2 0x24f8
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3 0x24fc
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4 0x2500
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5 0x2504
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6 0x2508
#define MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7 0x250c
#define MCSC_R_YUV_WDMASC_W2_IMG_CRC_1P 0x2590
#define MCSC_R_YUV_WDMASC_W2_IMG_CRC_2P 0x2594
#define MCSC_R_YUV_WDMASC_W2_IMG_CRC_3P 0x2598
#define MCSC_R_YUV_WDMASC_W2_MON_STATUS_0 0x25b0
#define MCSC_R_YUV_WDMASC_W2_MON_STATUS_1 0x25b4
#define MCSC_R_YUV_WDMASC_W2_MON_STATUS_2 0x25b8
#define MCSC_R_YUV_WDMASC_W2_MON_STATUS_3 0x25bc
#define MCSC_R_YUV_WDMASC_W2_BW_LIMIT_0 0x25d0
#define MCSC_R_YUV_WDMASC_W2_BW_LIMIT_1 0x25d4
#define MCSC_R_YUV_WDMASC_W2_BW_LIMIT_2 0x25d8
#define MCSC_R_YUV_WDMASC_W2_CACHE_CONTROL 0x25e0
#define MCSC_R_YUV_WDMASC_W2_DEBUG_CONTROL 0x25e4
#define MCSC_R_YUV_WDMASC_W2_DEBUG_0 0x25e8
#define MCSC_R_YUV_WDMASC_W2_DEBUG_1 0x25ec
#define MCSC_R_YUV_WDMASC_W2_DEBUG_2 0x25f0
#define MCSC_R_YUV_WDMASC_W2_FLIP_CONTROL 0x25f8
#define MCSC_R_YUV_WDMASC_W2_RGB_ALPHA 0x25fc
#define MCSC_R_YUV_WDMASC_W3_ENABLE 0x2600
#define MCSC_R_YUV_WDMASC_W3_DATA_FORMAT 0x2610
#define MCSC_R_YUV_WDMASC_W3_MONO_CTRL 0x2614
#define MCSC_R_YUV_WDMASC_W3_WIDTH 0x2620
#define MCSC_R_YUV_WDMASC_W3_HEIGHT 0x2624
#define MCSC_R_YUV_WDMASC_W3_STRIDE_1P 0x2628
#define MCSC_R_YUV_WDMASC_W3_STRIDE_2P 0x262c
#define MCSC_R_YUV_WDMASC_W3_STRIDE_3P 0x2630
#define MCSC_R_YUV_WDMASC_W3_VOTF_EN 0x263c
#define MCSC_R_YUV_WDMASC_W3_MAX_MO 0x2640
#define MCSC_R_YUV_WDMASC_W3_BUSINFO 0x264c
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_0 0x2650
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_1 0x2654
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_2 0x2658
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_3 0x265c
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_4 0x2660
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_5 0x2664
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_6 0x2668
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_7 0x266c
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0 0x2670
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1 0x2674
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2 0x2678
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3 0x267c
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4 0x2680
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5 0x2684
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6 0x2688
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7 0x268c
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_0 0x2690
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_1 0x2694
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_2 0x2698
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_3 0x269c
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_4 0x26a0
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_5 0x26a4
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_6 0x26a8
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_7 0x26ac
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0 0x26b0
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1 0x26b4
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2 0x26b8
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3 0x26bc
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4 0x26c0
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5 0x26c4
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6 0x26c8
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7 0x26cc
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_0 0x26d0
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_1 0x26d4
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_2 0x26d8
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_3 0x26dc
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_4 0x26e0
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_5 0x26e4
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_6 0x26e8
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_7 0x26ec
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0 0x26f0
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1 0x26f4
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2 0x26f8
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3 0x26fc
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4 0x2700
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5 0x2704
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6 0x2708
#define MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7 0x270c
#define MCSC_R_YUV_WDMASC_W3_IMG_CRC_1P 0x2790
#define MCSC_R_YUV_WDMASC_W3_IMG_CRC_2P 0x2794
#define MCSC_R_YUV_WDMASC_W3_IMG_CRC_3P 0x2798
#define MCSC_R_YUV_WDMASC_W3_MON_STATUS_0 0x27b0
#define MCSC_R_YUV_WDMASC_W3_MON_STATUS_1 0x27b4
#define MCSC_R_YUV_WDMASC_W3_MON_STATUS_2 0x27b8
#define MCSC_R_YUV_WDMASC_W3_MON_STATUS_3 0x27bc
#define MCSC_R_YUV_WDMASC_W3_BW_LIMIT_0 0x27d0
#define MCSC_R_YUV_WDMASC_W3_BW_LIMIT_1 0x27d4
#define MCSC_R_YUV_WDMASC_W3_BW_LIMIT_2 0x27d8
#define MCSC_R_YUV_WDMASC_W3_CACHE_CONTROL 0x27e0
#define MCSC_R_YUV_WDMASC_W3_DEBUG_CONTROL 0x27e4
#define MCSC_R_YUV_WDMASC_W3_DEBUG_0 0x27e8
#define MCSC_R_YUV_WDMASC_W3_DEBUG_1 0x27ec
#define MCSC_R_YUV_WDMASC_W3_DEBUG_2 0x27f0
#define MCSC_R_YUV_WDMASC_W3_FLIP_CONTROL 0x27f8
#define MCSC_R_YUV_WDMASC_W3_RGB_ALPHA 0x27fc
#define MCSC_R_YUV_WDMASC_W4_ENABLE 0x2800
#define MCSC_R_YUV_WDMASC_W4_DATA_FORMAT 0x2810
#define MCSC_R_YUV_WDMASC_W4_MONO_CTRL 0x2814
#define MCSC_R_YUV_WDMASC_W4_WIDTH 0x2820
#define MCSC_R_YUV_WDMASC_W4_HEIGHT 0x2824
#define MCSC_R_YUV_WDMASC_W4_STRIDE_1P 0x2828
#define MCSC_R_YUV_WDMASC_W4_STRIDE_2P 0x282c
#define MCSC_R_YUV_WDMASC_W4_STRIDE_3P 0x2830
#define MCSC_R_YUV_WDMASC_W4_VOTF_EN 0x283c
#define MCSC_R_YUV_WDMASC_W4_MAX_MO 0x2840
#define MCSC_R_YUV_WDMASC_W4_BUSINFO 0x284c
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_0 0x2850
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_1 0x2854
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_2 0x2858
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_3 0x285c
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_4 0x2860
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_5 0x2864
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_6 0x2868
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_7 0x286c
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0 0x2870
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1 0x2874
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2 0x2878
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3 0x287c
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4 0x2880
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5 0x2884
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6 0x2888
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7 0x288c
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_0 0x2890
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_1 0x2894
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_2 0x2898
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_3 0x289c
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_4 0x28a0
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_5 0x28a4
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_6 0x28a8
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_7 0x28ac
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0 0x28b0
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1 0x28b4
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2 0x28b8
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3 0x28bc
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4 0x28c0
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5 0x28c4
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6 0x28c8
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7 0x28cc
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_0 0x28d0
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_1 0x28d4
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_2 0x28d8
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_3 0x28dc
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_4 0x28e0
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_5 0x28e4
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_6 0x28e8
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_7 0x28ec
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0 0x28f0
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1 0x28f4
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2 0x28f8
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3 0x28fc
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4 0x2900
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5 0x2904
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6 0x2908
#define MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7 0x290c
#define MCSC_R_YUV_WDMASC_W4_IMG_CRC_1P 0x2990
#define MCSC_R_YUV_WDMASC_W4_IMG_CRC_2P 0x2994
#define MCSC_R_YUV_WDMASC_W4_IMG_CRC_3P 0x2998
#define MCSC_R_YUV_WDMASC_W4_MON_STATUS_0 0x29b0
#define MCSC_R_YUV_WDMASC_W4_MON_STATUS_1 0x29b4
#define MCSC_R_YUV_WDMASC_W4_MON_STATUS_2 0x29b8
#define MCSC_R_YUV_WDMASC_W4_MON_STATUS_3 0x29bc
#define MCSC_R_YUV_WDMASC_W4_BW_LIMIT_0 0x29d0
#define MCSC_R_YUV_WDMASC_W4_BW_LIMIT_1 0x29d4
#define MCSC_R_YUV_WDMASC_W4_BW_LIMIT_2 0x29d8
#define MCSC_R_YUV_WDMASC_W4_CACHE_CONTROL 0x29e0
#define MCSC_R_YUV_WDMASC_W4_DEBUG_CONTROL 0x29e4
#define MCSC_R_YUV_WDMASC_W4_DEBUG_0 0x29e8
#define MCSC_R_YUV_WDMASC_W4_DEBUG_1 0x29ec
#define MCSC_R_YUV_WDMASC_W4_DEBUG_2 0x29f0
#define MCSC_R_YUV_WDMASC_W4_FLIP_CONTROL 0x29f8
#define MCSC_R_YUV_WDMASC_W4_RGB_ALPHA 0x29fc
#define MCSC_R_YUV_WDMASC_W0_DITHER 0x2f00
#define MCSC_R_YUV_WDMASC_W0_RGB_CONV444_WEIGHT 0x2f04
#define MCSC_R_YUV_WDMASC_W0_PER_SUB_FRAME_EN 0x2f08
#define MCSC_R_YUV_WDMASC_W0_COMP_SRAM_START_ADDR 0x2f10
#define MCSC_R_YUV_WDMASC_W1_DITHER 0x2f20
#define MCSC_R_YUV_WDMASC_W1_RGB_CONV444_WEIGHT 0x2f24
#define MCSC_R_YUV_WDMASC_W1_PER_SUB_FRAME_EN 0x2f28
#define MCSC_R_YUV_WDMASC_W1_COMP_SRAM_START_ADDR 0x2f30
#define MCSC_R_YUV_WDMASC_W2_DITHER 0x2f40
#define MCSC_R_YUV_WDMASC_W2_RGB_CONV444_WEIGHT 0x2f44
#define MCSC_R_YUV_WDMASC_W2_PER_SUB_FRAME_EN 0x2f48
#define MCSC_R_YUV_WDMASC_W3_DITHER 0x2f60
#define MCSC_R_YUV_WDMASC_W3_RGB_CONV444_WEIGHT 0x2f64
#define MCSC_R_YUV_WDMASC_W3_PER_SUB_FRAME_EN 0x2f68
#define MCSC_R_YUV_WDMASC_W4_DITHER 0x2f80
#define MCSC_R_YUV_WDMASC_W4_RGB_CONV444_WEIGHT 0x2f84
#define MCSC_R_YUV_WDMASC_W4_PER_SUB_FRAME_EN 0x2f88
#define MCSC_R_YUV_WDMASC_RGB_OFFSET 0x2fc0
#define MCSC_R_YUV_WDMASC_RGB_COEF_0 0x2fc4
#define MCSC_R_YUV_WDMASC_RGB_COEF_1 0x2fc8
#define MCSC_R_YUV_WDMASC_RGB_COEF_2 0x2fcc
#define MCSC_R_YUV_WDMASC_RGB_COEF_3 0x2fd0
#define MCSC_R_YUV_WDMASC_RGB_COEF_4 0x2fd4
#define MCSC_R_YUV_DJAG_CTRL 0x4000
#define MCSC_R_YUV_DJAG_IMG_SIZE 0x4004
#define MCSC_R_YUV_DJAG_PS_SRC_POS 0x4008
#define MCSC_R_YUV_DJAG_PS_SRC_SIZE 0x400c
#define MCSC_R_YUV_DJAG_PS_DST_SIZE 0x4010
#define MCSC_R_YUV_DJAGPS_PS_H_RATIO 0x4014
#define MCSC_R_YUV_DJAGPS_PS_V_RATIO 0x4018
#define MCSC_R_YUV_DJAGPS_PS_H_INIT_PHASE_OFFSET 0x401c
#define MCSC_R_YUV_DJAGPS_PS_V_INIT_PHASE_OFFSET 0x4020
#define MCSC_R_YUV_DJAGPS_PS_ROUND_MODE 0x4024
#define MCSC_R_YUV_DJAG_PS_STRIP_PRE_DST_SIZE 0x4030
#define MCSC_R_YUV_DJAG_PS_STRIP_IN_START_POS 0x4034
#define MCSC_R_YUV_DJAG_OUT_CROP_POS 0x4038
#define MCSC_R_YUV_DJAG_OUT_CROP_SIZE 0x403c
#define MCSC_R_YUV_DJAG_XFILTER_DEJAGGING_COEFF 0x4040
#define MCSC_R_YUV_DJAG_THRES_1X5_MATCHING 0x4044
#define MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_0 0x4048
#define MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_1 0x404c
#define MCSC_R_YUV_DJAGFILTER_LFSR_SEED_0 0x4050
#define MCSC_R_YUV_DJAGFILTER_LFSR_SEED_1 0x4054
#define MCSC_R_YUV_DJAGFILTER_LFSR_SEED_2 0x4058
#define MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_04 0x405c
#define MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_58 0x4060
#define MCSC_R_YUV_DJAGFILTER_DITHER_THRES 0x4064
#define MCSC_R_YUV_DJAGFILTER_CP_HF_THRES 0x4068
#define MCSC_R_YUV_DJAG_CP_ARBI 0x406c
#define MCSC_R_YUV_DJAG_DITHER_WB 0x4070
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_0_01 0x4124
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_0_23 0x4128
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_1_01 0x412c
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_1_23 0x4130
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_2_01 0x4134
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_2_23 0x4138
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_3_01 0x413c
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_3_23 0x4140
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_4_01 0x4144
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_4_23 0x4148
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_5_01 0x414c
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_5_23 0x4150
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_6_01 0x4154
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_6_23 0x4158
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_7_01 0x415c
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_7_23 0x4160
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_8_01 0x4164
#define MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_8_23 0x4168
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_01 0x416c
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_23 0x4170
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_45 0x4174
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_67 0x4178
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_01 0x417c
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_23 0x4180
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_45 0x4184
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_67 0x4188
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_01 0x418c
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_23 0x4190
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_45 0x4194
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_67 0x4198
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_01 0x419c
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_23 0x41a0
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_45 0x41a4
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_67 0x41a8
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_01 0x41ac
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_23 0x41b0
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_45 0x41b4
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_67 0x41b8
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_01 0x41bc
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_23 0x41c0
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_45 0x41c4
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_67 0x41c8
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_01 0x41cc
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_23 0x41d0
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_45 0x41d4
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_67 0x41d8
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_01 0x41dc
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_23 0x41e0
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_45 0x41e4
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_67 0x41e8
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_01 0x41ec
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_23 0x41f0
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_45 0x41f4
#define MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_67 0x41f8
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_0_01 0x4224
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_0_23 0x4228
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_1_01 0x422c
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_1_23 0x4230
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_2_01 0x4234
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_2_23 0x4238
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_3_01 0x423c
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_3_23 0x4240
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_4_01 0x4244
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_4_23 0x4248
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_5_01 0x424c
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_5_23 0x4250
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_6_01 0x4254
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_6_23 0x4258
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_7_01 0x425c
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_7_23 0x4260
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_8_01 0x4264
#define MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_8_23 0x4268
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_0_01 0x426c
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_0_23 0x4270
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_1_01 0x427c
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_1_23 0x4280
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_2_01 0x428c
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_2_23 0x4290
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_3_01 0x429c
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_3_23 0x42a0
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_4_01 0x42ac
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_4_23 0x42b0
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_5_01 0x42bc
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_5_23 0x42c0
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_6_01 0x42cc
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_6_23 0x42d0
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_7_01 0x42dc
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_7_23 0x42e0
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_8_01 0x42ec
#define MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_8_23 0x42f0
#define MCSC_R_YUV_DJAGPREFILTER_CTRL 0x4300
#define MCSC_R_YUV_DJAGPREFILTER_IMG_SIZE 0x4304
#define MCSC_R_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF 0x4308
#define MCSC_R_YUV_DJAGPREFILTER_THRES_1X5_MATCHING 0x430c
#define MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_0 0x4310
#define MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_1 0x4314
#define MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_0 0x4318
#define MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_1 0x431c
#define MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_2 0x4320
#define MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_04 0x4324
#define MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_58 0x4328
#define MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_THRES 0x432c
#define MCSC_R_YUV_DJAGPREFILTER_FILTER_CP_HF_THRES 0x4330
#define MCSC_R_YUV_DJAGPREFILTER_CP_ARBI 0x4334
#define MCSC_R_YUV_DJAGPREFILTER_DITHER_WB 0x4338
#define MCSC_R_YUV_DJAGPREFILTER_HARRIS_DET 0x433c
#define MCSC_R_YUV_DJAGPREFILTER_BILATERAL_C 0x4340
#define MCSC_R_YUV_DUALLAYERRECOMP_CTRL 0x4400
#define MCSC_R_YUV_DUALLAYERRECOMP_WEIGHT 0x4404
#define MCSC_R_YUV_DUALLAYERRECOMP_IMG_SIZE 0x4408
#define MCSC_R_YUV_DUALLAYERRECOMP_IN_CROP_POS 0x440c
#define MCSC_R_YUV_DUALLAYERRECOMP_IN_CROP_SIZE 0x4410
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_CENTER 0x4414
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BINNING 0x4418
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_A 0x441c
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_B 0x4420
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_GAIN_ENABLE 0x4424
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_GAIN_INCREMENT 0x4428
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_SCALE_SHIFT_ADDER 0x442c
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_A 0x4430
#define MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_B 0x4434
#define MCSC_R_YUV_DUALLAYERRECOMP_STREAM_CRC 0x44fc
#define MCSC_R_YUV_POLY_SC0_CTRL 0x5000
#define MCSC_R_YUV_POLY_SC0_SRC_POS 0x5004
#define MCSC_R_YUV_POLY_SC0_SRC_SIZE 0x5008
#define MCSC_R_YUV_POLY_SC0_DST_SIZE 0x500c
#define MCSC_R_YUV_POLY_SC0_H_RATIO 0x5010
#define MCSC_R_YUV_POLY_SC0_V_RATIO 0x5014
#define MCSC_R_YUV_POLY_SC0_H_INIT_PHASE_OFFSET 0x5018
#define MCSC_R_YUV_POLY_SC0_V_INIT_PHASE_OFFSET 0x501c
#define MCSC_R_YUV_POLY_SC0_ROUND_MODE 0x5020
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_0_01 0x5024
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_0_23 0x5028
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_1_01 0x502c
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_1_23 0x5030
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_2_01 0x5034
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_2_23 0x5038
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_3_01 0x503c
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_3_23 0x5040
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_4_01 0x5044
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_4_23 0x5048
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_5_01 0x504c
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_5_23 0x5050
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_6_01 0x5054
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_6_23 0x5058
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_7_01 0x505c
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_7_23 0x5060
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_8_01 0x5064
#define MCSC_R_YUV_POLY_SC0_Y_V_COEFF_8_23 0x5068
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_01 0x506c
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_23 0x5070
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_45 0x5074
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_67 0x5078
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_01 0x507c
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_23 0x5080
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_45 0x5084
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_67 0x5088
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_01 0x508c
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_23 0x5090
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_45 0x5094
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_67 0x5098
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_01 0x509c
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_23 0x50a0
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_45 0x50a4
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_67 0x50a8
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_01 0x50ac
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_23 0x50b0
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_45 0x50b4
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_67 0x50b8
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_01 0x50bc
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_23 0x50c0
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_45 0x50c4
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_67 0x50c8
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_01 0x50cc
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_23 0x50d0
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_45 0x50d4
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_67 0x50d8
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_01 0x50dc
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_23 0x50e0
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_45 0x50e4
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_67 0x50e8
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_01 0x50ec
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_23 0x50f0
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_45 0x50f4
#define MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_67 0x50f8
#define MCSC_R_YUV_POLY_SC1_CTRL 0x5100
#define MCSC_R_YUV_POLY_SC1_SRC_POS 0x5104
#define MCSC_R_YUV_POLY_SC1_SRC_SIZE 0x5108
#define MCSC_R_YUV_POLY_SC1_DST_SIZE 0x510c
#define MCSC_R_YUV_POLY_SC1_H_RATIO 0x5110
#define MCSC_R_YUV_POLY_SC1_V_RATIO 0x5114
#define MCSC_R_YUV_POLY_SC1_H_INIT_PHASE_OFFSET 0x5118
#define MCSC_R_YUV_POLY_SC1_V_INIT_PHASE_OFFSET 0x511c
#define MCSC_R_YUV_POLY_SC1_ROUND_MODE 0x5120
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_0_01 0x5124
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_0_23 0x5128
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_1_01 0x512c
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_1_23 0x5130
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_2_01 0x5134
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_2_23 0x5138
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_3_01 0x513c
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_3_23 0x5140
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_4_01 0x5144
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_4_23 0x5148
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_5_01 0x514c
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_5_23 0x5150
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_6_01 0x5154
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_6_23 0x5158
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_7_01 0x515c
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_7_23 0x5160
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_8_01 0x5164
#define MCSC_R_YUV_POLY_SC1_Y_V_COEFF_8_23 0x5168
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_01 0x516c
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_23 0x5170
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_45 0x5174
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_67 0x5178
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_01 0x517c
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_23 0x5180
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_45 0x5184
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_67 0x5188
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_01 0x518c
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_23 0x5190
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_45 0x5194
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_67 0x5198
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_01 0x519c
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_23 0x51a0
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_45 0x51a4
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_67 0x51a8
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_01 0x51ac
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_23 0x51b0
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_45 0x51b4
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_67 0x51b8
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_01 0x51bc
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_23 0x51c0
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_45 0x51c4
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_67 0x51c8
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_01 0x51cc
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_23 0x51d0
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_45 0x51d4
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_67 0x51d8
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_01 0x51dc
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_23 0x51e0
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_45 0x51e4
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_67 0x51e8
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_01 0x51ec
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_23 0x51f0
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_45 0x51f4
#define MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_67 0x51f8
#define MCSC_R_YUV_POLY_SC2_CTRL 0x5200
#define MCSC_R_YUV_POLY_SC2_SRC_POS 0x5204
#define MCSC_R_YUV_POLY_SC2_SRC_SIZE 0x5208
#define MCSC_R_YUV_POLY_SC2_DST_SIZE 0x520c
#define MCSC_R_YUV_POLY_SC2_H_RATIO 0x5210
#define MCSC_R_YUV_POLY_SC2_V_RATIO 0x5214
#define MCSC_R_YUV_POLY_SC2_H_INIT_PHASE_OFFSET 0x5218
#define MCSC_R_YUV_POLY_SC2_V_INIT_PHASE_OFFSET 0x521c
#define MCSC_R_YUV_POLY_SC2_ROUND_MODE 0x5220
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_0_01 0x5224
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_0_23 0x5228
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_1_01 0x522c
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_1_23 0x5230
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_2_01 0x5234
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_2_23 0x5238
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_3_01 0x523c
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_3_23 0x5240
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_4_01 0x5244
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_4_23 0x5248
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_5_01 0x524c
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_5_23 0x5250
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_6_01 0x5254
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_6_23 0x5258
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_7_01 0x525c
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_7_23 0x5260
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_8_01 0x5264
#define MCSC_R_YUV_POLY_SC2_Y_V_COEFF_8_23 0x5268
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_01 0x526c
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_23 0x5270
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_45 0x5274
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_67 0x5278
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_01 0x527c
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_23 0x5280
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_45 0x5284
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_67 0x5288
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_01 0x528c
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_23 0x5290
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_45 0x5294
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_67 0x5298
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_01 0x529c
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_23 0x52a0
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_45 0x52a4
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_67 0x52a8
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_01 0x52ac
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_23 0x52b0
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_45 0x52b4
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_67 0x52b8
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_01 0x52bc
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_23 0x52c0
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_45 0x52c4
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_67 0x52c8
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_01 0x52cc
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_23 0x52d0
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_45 0x52d4
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_67 0x52d8
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_01 0x52dc
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_23 0x52e0
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_45 0x52e4
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_67 0x52e8
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_01 0x52ec
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_23 0x52f0
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_45 0x52f4
#define MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_67 0x52f8
#define MCSC_R_YUV_POLY_SC3_CTRL 0x5300
#define MCSC_R_YUV_POLY_SC3_SRC_POS 0x5304
#define MCSC_R_YUV_POLY_SC3_SRC_SIZE 0x5308
#define MCSC_R_YUV_POLY_SC3_DST_SIZE 0x530c
#define MCSC_R_YUV_POLY_SC3_H_RATIO 0x5310
#define MCSC_R_YUV_POLY_SC3_V_RATIO 0x5314
#define MCSC_R_YUV_POLY_SC3_H_INIT_PHASE_OFFSET 0x5318
#define MCSC_R_YUV_POLY_SC3_V_INIT_PHASE_OFFSET 0x531c
#define MCSC_R_YUV_POLY_SC3_ROUND_MODE 0x5320
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_0_01 0x5324
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_0_23 0x5328
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_1_01 0x532c
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_1_23 0x5330
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_2_01 0x5334
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_2_23 0x5338
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_3_01 0x533c
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_3_23 0x5340
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_4_01 0x5344
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_4_23 0x5348
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_5_01 0x534c
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_5_23 0x5350
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_6_01 0x5354
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_6_23 0x5358
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_7_01 0x535c
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_7_23 0x5360
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_8_01 0x5364
#define MCSC_R_YUV_POLY_SC3_Y_V_COEFF_8_23 0x5368
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_01 0x536c
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_23 0x5370
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_45 0x5374
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_67 0x5378
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_01 0x537c
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_23 0x5380
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_45 0x5384
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_67 0x5388
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_01 0x538c
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_23 0x5390
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_45 0x5394
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_67 0x5398
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_01 0x539c
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_23 0x53a0
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_45 0x53a4
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_67 0x53a8
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_01 0x53ac
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_23 0x53b0
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_45 0x53b4
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_67 0x53b8
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_01 0x53bc
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_23 0x53c0
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_45 0x53c4
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_67 0x53c8
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_01 0x53cc
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_23 0x53d0
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_45 0x53d4
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_67 0x53d8
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_01 0x53dc
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_23 0x53e0
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_45 0x53e4
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_67 0x53e8
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_01 0x53ec
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_23 0x53f0
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_45 0x53f4
#define MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_67 0x53f8
#define MCSC_R_YUV_POLY_SC4_CTRL 0x5400
#define MCSC_R_YUV_POLY_SC4_SRC_POS 0x5404
#define MCSC_R_YUV_POLY_SC4_SRC_SIZE 0x5408
#define MCSC_R_YUV_POLY_SC4_DST_SIZE 0x540c
#define MCSC_R_YUV_POLY_SC4_H_RATIO 0x5410
#define MCSC_R_YUV_POLY_SC4_V_RATIO 0x5414
#define MCSC_R_YUV_POLY_SC4_H_INIT_PHASE_OFFSET 0x5418
#define MCSC_R_YUV_POLY_SC4_V_INIT_PHASE_OFFSET 0x541c
#define MCSC_R_YUV_POLY_SC4_ROUND_MODE 0x5420
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_0_01 0x5424
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_0_23 0x5428
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_1_01 0x542c
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_1_23 0x5430
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_2_01 0x5434
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_2_23 0x5438
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_3_01 0x543c
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_3_23 0x5440
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_4_01 0x5444
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_4_23 0x5448
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_5_01 0x544c
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_5_23 0x5450
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_6_01 0x5454
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_6_23 0x5458
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_7_01 0x545c
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_7_23 0x5460
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_8_01 0x5464
#define MCSC_R_YUV_POLY_SC4_Y_V_COEFF_8_23 0x5468
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_01 0x546c
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_23 0x5470
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_45 0x5474
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_67 0x5478
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_01 0x547c
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_23 0x5480
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_45 0x5484
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_67 0x5488
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_01 0x548c
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_23 0x5490
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_45 0x5494
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_67 0x5498
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_01 0x549c
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_23 0x54a0
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_45 0x54a4
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_67 0x54a8
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_01 0x54ac
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_23 0x54b0
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_45 0x54b4
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_67 0x54b8
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_01 0x54bc
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_23 0x54c0
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_45 0x54c4
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_67 0x54c8
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_01 0x54cc
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_23 0x54d0
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_45 0x54d4
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_67 0x54d8
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_01 0x54dc
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_23 0x54e0
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_45 0x54e4
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_67 0x54e8
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_01 0x54ec
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_23 0x54f0
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_45 0x54f4
#define MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_67 0x54f8
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_0_01 0x5524
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_0_23 0x5528
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_1_01 0x552c
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_1_23 0x5530
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_2_01 0x5534
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_2_23 0x5538
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_3_01 0x553c
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_3_23 0x5540
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_4_01 0x5544
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_4_23 0x5548
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_5_01 0x554c
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_5_23 0x5550
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_6_01 0x5554
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_6_23 0x5558
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_7_01 0x555c
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_7_23 0x5560
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_8_01 0x5564
#define MCSC_R_YUV_POLY_SC0_UV_V_COEFF_8_23 0x5568
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_0_01 0x556c
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_0_23 0x5570
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_1_01 0x557c
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_1_23 0x5580
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_2_01 0x558c
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_2_23 0x5590
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_3_01 0x559c
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_3_23 0x55a0
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_4_01 0x55ac
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_4_23 0x55b0
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_5_01 0x55bc
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_5_23 0x55c0
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_6_01 0x55cc
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_6_23 0x55d0
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_7_01 0x55dc
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_7_23 0x55e0
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_8_01 0x55ec
#define MCSC_R_YUV_POLY_SC0_UV_H_COEFF_8_23 0x55f0
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_0_01 0x5624
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_0_23 0x5628
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_1_01 0x562c
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_1_23 0x5630
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_2_01 0x5634
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_2_23 0x5638
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_3_01 0x563c
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_3_23 0x5640
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_4_01 0x5644
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_4_23 0x5648
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_5_01 0x564c
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_5_23 0x5650
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_6_01 0x5654
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_6_23 0x5658
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_7_01 0x565c
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_7_23 0x5660
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_8_01 0x5664
#define MCSC_R_YUV_POLY_SC1_UV_V_COEFF_8_23 0x5668
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_0_01 0x566c
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_0_23 0x5670
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_1_01 0x567c
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_1_23 0x5680
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_2_01 0x568c
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_2_23 0x5690
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_3_01 0x569c
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_3_23 0x56a0
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_4_01 0x56ac
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_4_23 0x56b0
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_5_01 0x56bc
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_5_23 0x56c0
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_6_01 0x56cc
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_6_23 0x56d0
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_7_01 0x56dc
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_7_23 0x56e0
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_8_01 0x56ec
#define MCSC_R_YUV_POLY_SC1_UV_H_COEFF_8_23 0x56f0
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_0_01 0x5724
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_0_23 0x5728
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_1_01 0x572c
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_1_23 0x5730
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_2_01 0x5734
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_2_23 0x5738
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_3_01 0x573c
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_3_23 0x5740
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_4_01 0x5744
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_4_23 0x5748
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_5_01 0x574c
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_5_23 0x5750
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_6_01 0x5754
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_6_23 0x5758
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_7_01 0x575c
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_7_23 0x5760
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_8_01 0x5764
#define MCSC_R_YUV_POLY_SC2_UV_V_COEFF_8_23 0x5768
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_0_01 0x576c
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_0_23 0x5770
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_1_01 0x577c
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_1_23 0x5780
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_2_01 0x578c
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_2_23 0x5790
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_3_01 0x579c
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_3_23 0x57a0
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_4_01 0x57ac
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_4_23 0x57b0
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_5_01 0x57bc
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_5_23 0x57c0
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_6_01 0x57cc
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_6_23 0x57d0
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_7_01 0x57dc
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_7_23 0x57e0
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_8_01 0x57ec
#define MCSC_R_YUV_POLY_SC2_UV_H_COEFF_8_23 0x57f0
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_0_01 0x5824
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_0_23 0x5828
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_1_01 0x582c
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_1_23 0x5830
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_2_01 0x5834
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_2_23 0x5838
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_3_01 0x583c
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_3_23 0x5840
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_4_01 0x5844
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_4_23 0x5848
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_5_01 0x584c
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_5_23 0x5850
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_6_01 0x5854
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_6_23 0x5858
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_7_01 0x585c
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_7_23 0x5860
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_8_01 0x5864
#define MCSC_R_YUV_POLY_SC3_UV_V_COEFF_8_23 0x5868
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_0_01 0x586c
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_0_23 0x5870
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_1_01 0x587c
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_1_23 0x5880
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_2_01 0x588c
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_2_23 0x5890
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_3_01 0x589c
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_3_23 0x58a0
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_4_01 0x58ac
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_4_23 0x58b0
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_5_01 0x58bc
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_5_23 0x58c0
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_6_01 0x58cc
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_6_23 0x58d0
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_7_01 0x58dc
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_7_23 0x58e0
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_8_01 0x58ec
#define MCSC_R_YUV_POLY_SC3_UV_H_COEFF_8_23 0x58f0
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_0_01 0x5924
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_0_23 0x5928
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_1_01 0x592c
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_1_23 0x5930
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_2_01 0x5934
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_2_23 0x5938
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_3_01 0x593c
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_3_23 0x5940
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_4_01 0x5944
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_4_23 0x5948
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_5_01 0x594c
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_5_23 0x5950
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_6_01 0x5954
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_6_23 0x5958
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_7_01 0x595c
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_7_23 0x5960
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_8_01 0x5964
#define MCSC_R_YUV_POLY_SC4_UV_V_COEFF_8_23 0x5968
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_0_01 0x596c
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_0_23 0x5970
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_1_01 0x597c
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_1_23 0x5980
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_2_01 0x598c
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_2_23 0x5990
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_3_01 0x599c
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_3_23 0x59a0
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_4_01 0x59ac
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_4_23 0x59b0
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_5_01 0x59bc
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_5_23 0x59c0
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_6_01 0x59cc
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_6_23 0x59d0
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_7_01 0x59dc
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_7_23 0x59e0
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_8_01 0x59ec
#define MCSC_R_YUV_POLY_SC4_UV_H_COEFF_8_23 0x59f0
#define MCSC_R_YUV_POLY_SC0_STRIP_PRE_DST_SIZE 0x5f00
#define MCSC_R_YUV_POLY_SC0_STRIP_IN_START_POS 0x5f04
#define MCSC_R_YUV_POLY_SC0_OUT_CROP_POS 0x5f08
#define MCSC_R_YUV_POLY_SC0_OUT_CROP_SIZE 0x5f0c
#define MCSC_R_YUV_POLY_SC1_STRIP_PRE_DST_SIZE 0x5f10
#define MCSC_R_YUV_POLY_SC1_STRIP_IN_START_POS 0x5f14
#define MCSC_R_YUV_POLY_SC1_OUT_CROP_POS 0x5f18
#define MCSC_R_YUV_POLY_SC1_OUT_CROP_SIZE 0x5f1c
#define MCSC_R_YUV_POLY_SC2_STRIP_PRE_DST_SIZE 0x5f20
#define MCSC_R_YUV_POLY_SC2_STRIP_IN_START_POS 0x5f24
#define MCSC_R_YUV_POLY_SC2_OUT_CROP_POS 0x5f28
#define MCSC_R_YUV_POLY_SC2_OUT_CROP_SIZE 0x5f2c
#define MCSC_R_YUV_POLY_SC3_STRIP_PRE_DST_SIZE 0x5f30
#define MCSC_R_YUV_POLY_SC3_STRIP_IN_START_POS 0x5f34
#define MCSC_R_YUV_POLY_SC3_OUT_CROP_POS 0x5f38
#define MCSC_R_YUV_POLY_SC3_OUT_CROP_SIZE 0x5f3c
#define MCSC_R_YUV_POLY_SC4_STRIP_PRE_DST_SIZE 0x5f40
#define MCSC_R_YUV_POLY_SC4_STRIP_IN_START_POS 0x5f44
#define MCSC_R_YUV_POLY_SC4_OUT_CROP_POS 0x5f48
#define MCSC_R_YUV_POLY_SC4_OUT_CROP_SIZE 0x5f4c
#define MCSC_R_YUV_POST_PC0_CTRL 0x6000
#define MCSC_R_YUV_POST_PC0_IMG_SIZE 0x6004
#define MCSC_R_YUV_POST_PC0_DST_SIZE 0x6008
#define MCSC_R_YUV_POSTPC_PC0_H_RATIO 0x600c
#define MCSC_R_YUV_POSTPC_PC0_V_RATIO 0x6010
#define MCSC_R_YUV_POSTPC_PC0_H_INIT_PHASE_OFFSET 0x6014
#define MCSC_R_YUV_POSTPC_PC0_V_INIT_PHASE_OFFSET 0x6018
#define MCSC_R_YUV_POSTPC_PC0_ROUND_MODE 0x601c
#define MCSC_R_YUV_POST_PC0_STRIP_PRE_DST_SIZE 0x6030
#define MCSC_R_YUV_POST_PC0_STRIP_IN_START_POS 0x6034
#define MCSC_R_YUV_POST_PC0_OUT_CROP_POS 0x6038
#define MCSC_R_YUV_POST_PC0_OUT_CROP_SIZE 0x603c
#define MCSC_R_YUV_POST_PC0_CONV420_CTRL 0x6040
#define MCSC_R_YUV_POST_PC0_BCHS_CTRL 0x6048
#define MCSC_R_YUV_POST_PC0_BCHS_BC 0x604c
#define MCSC_R_YUV_POST_PC0_BCHS_HS1 0x6050
#define MCSC_R_YUV_POST_PC0_BCHS_HS2 0x6054
#define MCSC_R_YUV_POST_PC0_BCHS_CLAMP_Y 0x6058
#define MCSC_R_YUV_POST_PC0_BCHS_CLAMP_C 0x605c
#define MCSC_R_YUV_POST_PC1_CTRL 0x6100
#define MCSC_R_YUV_POST_PC1_IMG_SIZE 0x6104
#define MCSC_R_YUV_POST_PC1_DST_SIZE 0x6108
#define MCSC_R_YUV_POSTPC_PC1_H_RATIO 0x610c
#define MCSC_R_YUV_POSTPC_PC1_V_RATIO 0x6110
#define MCSC_R_YUV_POSTPC_PC1_H_INIT_PHASE_OFFSET 0x6114
#define MCSC_R_YUV_POSTPC_PC1_V_INIT_PHASE_OFFSET 0x6118
#define MCSC_R_YUV_POSTPC_PC1_ROUND_MODE 0x611c
#define MCSC_R_YUV_POST_PC1_STRIP_PRE_DST_SIZE 0x6130
#define MCSC_R_YUV_POST_PC1_STRIP_IN_START_POS 0x6134
#define MCSC_R_YUV_POST_PC1_OUT_CROP_POS 0x6138
#define MCSC_R_YUV_POST_PC1_OUT_CROP_SIZE 0x613c
#define MCSC_R_YUV_POST_PC1_CONV420_CTRL 0x6140
#define MCSC_R_YUV_POST_PC1_BCHS_CTRL 0x6148
#define MCSC_R_YUV_POST_PC1_BCHS_BC 0x614c
#define MCSC_R_YUV_POST_PC1_BCHS_HS1 0x6150
#define MCSC_R_YUV_POST_PC1_BCHS_HS2 0x6154
#define MCSC_R_YUV_POST_PC1_BCHS_CLAMP_Y 0x6158
#define MCSC_R_YUV_POST_PC1_BCHS_CLAMP_C 0x615c
#define MCSC_R_YUV_POST_PC2_CTRL 0x6200
#define MCSC_R_YUV_POST_PC2_IMG_SIZE 0x6204
#define MCSC_R_YUV_POST_PC2_DST_SIZE 0x6208
#define MCSC_R_YUV_POSTPC_PC2_H_RATIO 0x620c
#define MCSC_R_YUV_POSTPC_PC2_V_RATIO 0x6210
#define MCSC_R_YUV_POSTPC_PC2_H_INIT_PHASE_OFFSET 0x6214
#define MCSC_R_YUV_POSTPC_PC2_V_INIT_PHASE_OFFSET 0x6218
#define MCSC_R_YUV_POSTPC_PC2_ROUND_MODE 0x621c
#define MCSC_R_YUV_POST_PC2_STRIP_PRE_DST_SIZE 0x6230
#define MCSC_R_YUV_POST_PC2_STRIP_IN_START_POS 0x6234
#define MCSC_R_YUV_POST_PC2_OUT_CROP_POS 0x6238
#define MCSC_R_YUV_POST_PC2_OUT_CROP_SIZE 0x623c
#define MCSC_R_YUV_POST_PC2_CONV420_CTRL 0x6240
#define MCSC_R_YUV_POST_PC2_BCHS_CTRL 0x6248
#define MCSC_R_YUV_POST_PC2_BCHS_BC 0x624c
#define MCSC_R_YUV_POST_PC2_BCHS_HS1 0x6250
#define MCSC_R_YUV_POST_PC2_BCHS_HS2 0x6254
#define MCSC_R_YUV_POST_PC2_BCHS_CLAMP_Y 0x6258
#define MCSC_R_YUV_POST_PC2_BCHS_CLAMP_C 0x625c
#define MCSC_R_YUV_POST_PC3_CTRL 0x6300
#define MCSC_R_YUV_POST_PC3_CONV420_CTRL 0x6340
#define MCSC_R_YUV_POST_PC3_BCHS_CTRL 0x6348
#define MCSC_R_YUV_POST_PC3_BCHS_BC 0x634c
#define MCSC_R_YUV_POST_PC3_BCHS_HS1 0x6350
#define MCSC_R_YUV_POST_PC3_BCHS_HS2 0x6354
#define MCSC_R_YUV_POST_PC3_BCHS_CLAMP_Y 0x6358
#define MCSC_R_YUV_POST_PC3_BCHS_CLAMP_C 0x635c
#define MCSC_R_YUV_POST_PC4_CTRL 0x6400
#define MCSC_R_YUV_POST_PC4_CONV420_CTRL 0x6440
#define MCSC_R_YUV_POST_PC4_BCHS_CTRL 0x6448
#define MCSC_R_YUV_POST_PC4_BCHS_BC 0x644c
#define MCSC_R_YUV_POST_PC4_BCHS_HS1 0x6450
#define MCSC_R_YUV_POST_PC4_BCHS_HS2 0x6454
#define MCSC_R_YUV_POST_PC4_BCHS_CLAMP_Y 0x6458
#define MCSC_R_YUV_POST_PC4_BCHS_CLAMP_C 0x645c
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_0_01 0x6524
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_0_23 0x6528
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_1_01 0x652c
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_1_23 0x6530
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_2_01 0x6534
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_2_23 0x6538
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_3_01 0x653c
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_3_23 0x6540
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_4_01 0x6544
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_4_23 0x6548
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_5_01 0x654c
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_5_23 0x6550
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_6_01 0x6554
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_6_23 0x6558
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_7_01 0x655c
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_7_23 0x6560
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_8_01 0x6564
#define MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_8_23 0x6568
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_01 0x656c
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_23 0x6570
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_45 0x6574
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_67 0x6578
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_01 0x657c
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_23 0x6580
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_45 0x6584
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_67 0x6588
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_01 0x658c
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_23 0x6590
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_45 0x6594
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_67 0x6598
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_01 0x659c
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_23 0x65a0
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_45 0x65a4
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_67 0x65a8
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_01 0x65ac
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_23 0x65b0
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_45 0x65b4
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_67 0x65b8
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_01 0x65bc
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_23 0x65c0
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_45 0x65c4
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_67 0x65c8
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_01 0x65cc
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_23 0x65d0
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_45 0x65d4
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_67 0x65d8
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_01 0x65dc
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_23 0x65e0
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_45 0x65e4
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_67 0x65e8
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_01 0x65ec
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_23 0x65f0
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_45 0x65f4
#define MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_67 0x65f8
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_0_01 0x6624
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_0_23 0x6628
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_1_01 0x662c
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_1_23 0x6630
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_2_01 0x6634
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_2_23 0x6638
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_3_01 0x663c
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_3_23 0x6640
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_4_01 0x6644
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_4_23 0x6648
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_5_01 0x664c
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_5_23 0x6650
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_6_01 0x6654
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_6_23 0x6658
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_7_01 0x665c
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_7_23 0x6660
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_8_01 0x6664
#define MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_8_23 0x6668
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_01 0x666c
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_23 0x6670
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_45 0x6674
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_67 0x6678
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_01 0x667c
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_23 0x6680
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_45 0x6684
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_67 0x6688
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_01 0x668c
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_23 0x6690
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_45 0x6694
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_67 0x6698
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_01 0x669c
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_23 0x66a0
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_45 0x66a4
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_67 0x66a8
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_01 0x66ac
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_23 0x66b0
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_45 0x66b4
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_67 0x66b8
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_01 0x66bc
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_23 0x66c0
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_45 0x66c4
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_67 0x66c8
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_01 0x66cc
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_23 0x66d0
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_45 0x66d4
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_67 0x66d8
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_01 0x66dc
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_23 0x66e0
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_45 0x66e4
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_67 0x66e8
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_01 0x66ec
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_23 0x66f0
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_45 0x66f4
#define MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_67 0x66f8
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_0_01 0x6724
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_0_23 0x6728
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_1_01 0x672c
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_1_23 0x6730
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_2_01 0x6734
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_2_23 0x6738
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_3_01 0x673c
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_3_23 0x6740
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_4_01 0x6744
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_4_23 0x6748
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_5_01 0x674c
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_5_23 0x6750
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_6_01 0x6754
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_6_23 0x6758
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_7_01 0x675c
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_7_23 0x6760
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_8_01 0x6764
#define MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_8_23 0x6768
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_01 0x676c
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_23 0x6770
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_45 0x6774
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_67 0x6778
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_01 0x677c
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_23 0x6780
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_45 0x6784
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_67 0x6788
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_01 0x678c
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_23 0x6790
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_45 0x6794
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_67 0x6798
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_01 0x679c
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_23 0x67a0
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_45 0x67a4
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_67 0x67a8
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_01 0x67ac
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_23 0x67b0
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_45 0x67b4
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_67 0x67b8
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_01 0x67bc
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_23 0x67c0
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_45 0x67c4
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_67 0x67c8
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_01 0x67cc
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_23 0x67d0
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_45 0x67d4
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_67 0x67d8
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_01 0x67dc
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_23 0x67e0
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_45 0x67e4
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_67 0x67e8
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_01 0x67ec
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_23 0x67f0
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_45 0x67f4
#define MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_67 0x67f8
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_0_01 0x6a24
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_0_23 0x6a28
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_1_01 0x6a2c
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_1_23 0x6a30
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_2_01 0x6a34
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_2_23 0x6a38
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_3_01 0x6a3c
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_3_23 0x6a40
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_4_01 0x6a44
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_4_23 0x6a48
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_5_01 0x6a4c
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_5_23 0x6a50
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_6_01 0x6a54
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_6_23 0x6a58
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_7_01 0x6a5c
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_7_23 0x6a60
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_8_01 0x6a64
#define MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_8_23 0x6a68
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_0_01 0x6a6c
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_0_23 0x6a70
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_1_01 0x6a7c
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_1_23 0x6a80
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_2_01 0x6a8c
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_2_23 0x6a90
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_3_01 0x6a9c
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_3_23 0x6aa0
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_4_01 0x6aac
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_4_23 0x6ab0
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_5_01 0x6abc
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_5_23 0x6ac0
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_6_01 0x6acc
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_6_23 0x6ad0
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_7_01 0x6adc
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_7_23 0x6ae0
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_8_01 0x6aec
#define MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_8_23 0x6af0
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_0_01 0x6b24
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_0_23 0x6b28
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_1_01 0x6b2c
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_1_23 0x6b30
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_2_01 0x6b34
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_2_23 0x6b38
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_3_01 0x6b3c
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_3_23 0x6b40
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_4_01 0x6b44
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_4_23 0x6b48
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_5_01 0x6b4c
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_5_23 0x6b50
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_6_01 0x6b54
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_6_23 0x6b58
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_7_01 0x6b5c
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_7_23 0x6b60
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_8_01 0x6b64
#define MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_8_23 0x6b68
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_0_01 0x6b6c
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_0_23 0x6b70
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_1_01 0x6b7c
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_1_23 0x6b80
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_2_01 0x6b8c
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_2_23 0x6b90
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_3_01 0x6b9c
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_3_23 0x6ba0
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_4_01 0x6bac
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_4_23 0x6bb0
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_5_01 0x6bbc
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_5_23 0x6bc0
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_6_01 0x6bcc
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_6_23 0x6bd0
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_7_01 0x6bdc
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_7_23 0x6be0
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_8_01 0x6bec
#define MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_8_23 0x6bf0
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_0_01 0x6c24
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_0_23 0x6c28
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_1_01 0x6c2c
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_1_23 0x6c30
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_2_01 0x6c34
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_2_23 0x6c38
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_3_01 0x6c3c
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_3_23 0x6c40
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_4_01 0x6c44
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_4_23 0x6c48
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_5_01 0x6c4c
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_5_23 0x6c50
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_6_01 0x6c54
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_6_23 0x6c58
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_7_01 0x6c5c
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_7_23 0x6c60
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_8_01 0x6c64
#define MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_8_23 0x6c68
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_0_01 0x6c6c
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_0_23 0x6c70
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_1_01 0x6c7c
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_1_23 0x6c80
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_2_01 0x6c8c
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_2_23 0x6c90
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_3_01 0x6c9c
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_3_23 0x6ca0
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_4_01 0x6cac
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_4_23 0x6cb0
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_5_01 0x6cbc
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_5_23 0x6cc0
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_6_01 0x6ccc
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_6_23 0x6cd0
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_7_01 0x6cdc
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_7_23 0x6ce0
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_8_01 0x6cec
#define MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_8_23 0x6cf0
#define MCSC_R_YUV_HWFC_SWRESET 0x7000
#define MCSC_R_YUV_HWFC_MODE 0x7004
#define MCSC_R_YUV_HWFC_REGION_IDX_BIN 0x7008
#define MCSC_R_YUV_HWFC_REGION_IDX_GRAY 0x700c
#define MCSC_R_YUV_HWFC_CURR_REGION 0x7010
#define MCSC_R_YUV_HWFC_CONFIG_IMAGE_A 0x7014
#define MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE0_A 0x7018
#define MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE0_A 0x701c
#define MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE1_A 0x7020
#define MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE1_A 0x7024
#define MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE2_A 0x7028
#define MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE2_A 0x702c
#define MCSC_R_YUV_HWFC_CONFIG_IMAGE_B 0x7030
#define MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE0_B 0x7034
#define MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE0_B 0x7038
#define MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE1_B 0x703c
#define MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE1_B 0x7040
#define MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE2_B 0x7044
#define MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE2_B 0x7048
#define MCSC_R_YUV_HWFC_FRAME_START_SELECT 0x704c
#define MCSC_R_YUV_HWFC_INDEX_RESET 0x7050
#define MCSC_R_YUV_HWFC_ENABLE_AUTO_CLEAR 0x7054
#define MCSC_R_YUV_HWFC_CORE_RESET_INPUT_SEL 0x7058
#define MCSC_R_YUV_HWFC_MASTER_SEL 0x705c
#define MCSC_R_YUV_HWFC_IMAGE_HEIGHT 0x7060
#define MCSC_R_YUV_DTPOTFIN_BYPASS 0x7800
#define MCSC_R_YUV_DTPOTFIN_TEST_PATTERN_MODE 0x7808
#define MCSC_R_YUV_DTPOTFIN_TEST_DATA_Y 0x780c
#define MCSC_R_YUV_DTPOTFIN_TEST_DATA_U 0x7810
#define MCSC_R_YUV_DTPOTFIN_TEST_DATA_V 0x7814
#define MCSC_R_YUV_DTPOTFIN_YUV_STANDARD 0x7818
#define MCSC_R_YUV_DTPOTFIN_STREAM_CRC 0x78fc
#define MCSC_R_STAT_DTPDUALLAYER_BYPASS 0x7a00
#define MCSC_R_STAT_DTPDUALLAYER_TEST_PATTERN_MODE 0x7a08
#define MCSC_R_STAT_DTPDUALLAYER_TEST_DATA_Y 0x7a0c
#define MCSC_R_STAT_DTPDUALLAYER_TEST_DATA_U 0x7a10
#define MCSC_R_STAT_DTPDUALLAYER_TEST_DATA_V 0x7a14
#define MCSC_R_STAT_DTPDUALLAYER_YUV_STANDARD 0x7a18
#define MCSC_R_STAT_DTPDUALLAYER_STREAM_CRC 0x7afc
#define MCSC_R_DBG_AXI_0 0x7e00
#define MCSC_R_DBG_AXI_1 0x7e04
#define MCSC_R_DBG_AXI_2 0x7e08
#define MCSC_R_DBG_AXI_3 0x7e0c
#define MCSC_R_DBG_AXI_4 0x7e10
#define MCSC_R_DBG_AXI_5 0x7e14
#define MCSC_R_DBG_AXI_6 0x7e18
#define MCSC_R_DBG_AXI_7 0x7e1c
#define MCSC_R_DBG_AXI_8 0x7e20
#define MCSC_R_DBG_AXI_9 0x7e24
#define MCSC_R_DBG_AXI_10 0x7e28
#define MCSC_R_DBG_AXI_11 0x7e2c
#define MCSC_R_DBG_AXI_12 0x7e30
#define MCSC_R_DBG_AXI_13 0x7e34
#define MCSC_R_DBG_AXI_14 0x7e38
#define MCSC_R_DBG_AXI_15 0x7e3c
#define MCSC_R_DBG_AXI_16 0x7e40
#define MCSC_R_DBG_AXI_17 0x7e44
#define MCSC_R_DBG_AXI_18 0x7e48
#define MCSC_R_DBG_AXI_19 0x7e4c
#define MCSC_R_DBG_AXI_20 0x7e50
#define MCSC_R_DBG_AXI_21 0x7e54
#define MCSC_R_DBG_AXI_22 0x7e58
#define MCSC_R_DBG_AXI_23 0x7e5c
#define MCSC_R_DBG_AXI_24 0x7e60
#define MCSC_R_DBG_AXI_25 0x7e64
#define MCSC_R_DBG_AXI_26 0x7e68
#define MCSC_R_DBG_AXI_27 0x7e6c
#define MCSC_R_DBG_AXI_28 0x7e70
#define MCSC_R_DBG_AXI_29 0x7e74
#define MCSC_R_DBG_AXI_30 0x7e78
#define MCSC_R_DBG_AXI_31 0x7e7c
#define MCSC_R_DBG_AXI_32 0x7e80
#define MCSC_R_DBG_AXI_33 0x7e84
#define MCSC_R_DBG_AXI_34 0x7e88
#define MCSC_R_DBG_AXI_35 0x7e8c
#define MCSC_R_DBG_AXI_36 0x7e90
#define MCSC_R_DBG_AXI_37 0x7e94
#define MCSC_R_DBG_AXI_38 0x7e98
#define MCSC_R_DBG_AXI_39 0x7e9c
#define MCSC_R_DBG_AXI_40 0x7ea0
#define MCSC_R_DBG_AXI_41 0x7ea4
#define MCSC_R_DBG_AXI_42 0x7ea8
#define MCSC_R_DBG_AXI_43 0x7eac
#define MCSC_R_DBG_AXI_44 0x7eb0
#define MCSC_R_DBG_AXI_45 0x7eb4
#define MCSC_R_DBG_AXI_46 0x7eb8
#define MCSC_R_DBG_AXI_47 0x7ebc
#define MCSC_R_DBG_AXI_48 0x7ec0
#define MCSC_R_DBG_AXI_49 0x7ec4
#define MCSC_R_DBG_AXI_58 0x7ee8
#define MCSC_R_DBG_AXI_59 0x7eec
#define MCSC_R_DBG_AXI_60 0x7ef0
#define MCSC_R_DBG_AXI_61 0x7ef4
#define MCSC_R_DBG_SC_0 0x7f00
#define MCSC_R_DBG_SC_1 0x7f04
#define MCSC_R_DBG_SC_2 0x7f08
#define MCSC_R_DBG_SC_3 0x7f0c
#define MCSC_R_DBG_SC_4 0x7f10
#define MCSC_R_DBG_SC_5 0x7f14
#define MCSC_R_DBG_SC_6 0x7f18
#define MCSC_R_DBG_SC_7 0x7f1c
#define MCSC_R_DBG_SC_8 0x7f20
#define MCSC_R_DBG_SC_9 0x7f24
#define MCSC_R_DBG_SC_10 0x7f28
#define MCSC_R_DBG_SC_11 0x7f2c
#define MCSC_R_DBG_SC_12 0x7f30
#define MCSC_R_DBG_SC_13 0x7f34
#define MCSC_R_DBG_SC_14 0x7f38
#define MCSC_R_DBG_SC_15 0x7f3c
#define MCSC_R_DBG_SC_16 0x7f40
#define MCSC_R_DBG_SC_17 0x7f44
#define MCSC_R_DBG_SC_18 0x7f48
#define MCSC_R_DBG_SC_19 0x7f4c
#define MCSC_R_DBG_SC_20 0x7f50
#define MCSC_R_DBG_SC_21 0x7f54
#define MCSC_R_DBG_SC_22 0x7f58
#define MCSC_R_DBG_SC_23 0x7f5c
#define MCSC_R_DBG_SC_24 0x7f60
#define MCSC_R_DBG_SC_25 0x7f64
#define MCSC_R_DBG_SC_26 0x7f68
#define MCSC_R_DBG_SC_27 0x7f6c
#define MCSC_R_DBG_SC_28 0x7f70
#define MCSC_R_DBG_SC_29 0x7f74
#define MCSC_R_DBG_SC_30 0x7f78
#define MCSC_R_DBG_SC_31 0x7f7c
#define MCSC_R_DBG_SC_32 0x7f80
#define MCSC_R_DBG_SC_33 0x7f84
#define MCSC_R_DBG_SC_34 0x7f88
#define MCSC_R_CRC_RESULT_0 0x7f8c
#define MCSC_R_CRC_RESULT_1 0x7f90
#define MCSC_R_CRC_RESULT_2 0x7f94
#define MCSC_R_CRC_RESULT_3 0x7f98
#define MCSC_R_CRC_RESULT_4 0x7f9c
#define MCSC_R_CRC_RESULT_5 0x7fa0
#define MCSC_R_CRC_RESULT_6 0x7fa4
#define MCSC_R_CRC_RESULT_7 0x7fa8
#define MCSC_R_CRC_RESULT_8 0x7fac
#define MCSC_R_CRC_RESULT_9 0x7fb0
#define MCSC_R_CRC_RESULT_10 0x7fb4
#define MCSC_R_CRC_RESULT_11 0x7fb8
#define MCSC_R_CRC_RESULT_12 0x7fbc
#define MCSC_R_CRC_RESULT_13 0x7fc0
#define MCSC_R_DBG_SC_35 0x7fc4

enum mcsc_fields {
	MCSC_F_CMDQ_ENABLE,
	MCSC_F_CMDQ_STOP_CRPT_ENABLE,
	MCSC_F_SW_RESET,
	MCSC_F_SW_CORE_RESET,
	MCSC_F_SW_APB_RESET,
	MCSC_F_TRANS_STOP_REQ,
	MCSC_F_TRANS_STOP_REQ_RDY,
	MCSC_F_IP_APG_MODE,
	MCSC_F_IP_CLOCK_DOWN_MODE,
	MCSC_F_IP_PROCESSING,
	MCSC_F_FORCE_INTERNAL_CLOCK,
	MCSC_F_DEBUG_CLOCK_ENABLE,
	MCSC_F_IP_POST_FRAME_GAP,
	MCSC_F_IP_DRCG_ENABLE,
	MCSC_F_AUTO_IGNORE_INTERRUPT_ENABLE,
	MCSC_F_IP_USE_SW_FINISH_COND,
	MCSC_F_SW_FINISH_COND_ENABLE,
	MCSC_F_IP_CORRUPTED_COND_ENABLE,
	MCSC_F_IP_USE_OTF_IN_FOR_PATH_6,
	MCSC_F_IP_USE_OTF_IN_FOR_PATH_7,
	MCSC_F_IP_USE_OTF_OUT_FOR_PATH_6,
	MCSC_F_IP_USE_OTF_OUT_FOR_PATH_7,
	MCSC_F_IP_USE_OTF_IN_FOR_PATH_4,
	MCSC_F_IP_USE_OTF_IN_FOR_PATH_5,
	MCSC_F_IP_USE_OTF_OUT_FOR_PATH_4,
	MCSC_F_IP_USE_OTF_OUT_FOR_PATH_5,
	MCSC_F_IP_USE_OTF_IN_FOR_PATH_2,
	MCSC_F_IP_USE_OTF_IN_FOR_PATH_3,
	MCSC_F_IP_USE_OTF_OUT_FOR_PATH_2,
	MCSC_F_IP_USE_OTF_OUT_FOR_PATH_3,
	MCSC_F_IP_USE_OTF_IN_FOR_PATH_0,
	MCSC_F_IP_USE_OTF_IN_FOR_PATH_1,
	MCSC_F_IP_USE_OTF_OUT_FOR_PATH_0,
	MCSC_F_IP_USE_OTF_OUT_FOR_PATH_1,
	MCSC_F_IP_USE_CINFIFO_NEW_FRAME_IN,
	MCSC_F_SECU_CTRL_TZINFO_ICTRL,
	MCSC_F_ICTRL_CSUB_BASE_ADDR,
	MCSC_F_ICTRL_CSUB_RECV_TURN_OFF_MSG,
	MCSC_F_ICTRL_CSUB_RECV_IP_INFO_MSG,
	MCSC_F_ICTRL_CSUB_CONNECTION_TEST_MSG,
	MCSC_F_ICTRL_CSUB_MSG_SEND_ENABLE,
	MCSC_F_ICTRL_CSUB_INT0_EV_ENABLE,
	MCSC_F_ICTRL_CSUB_INT1_EV_ENABLE,
	MCSC_F_ICTRL_CSUB_IP_S_EV_ENABLE,
	MCSC_F_YUV_IN_IMG_SZ_WIDTH,
	MCSC_F_YUV_IN_IMG_SZ_HEIGHT,
	MCSC_F_YUV_FRO_ENABLE,
	MCSC_F_YUV_INPUT_MONO_EN,
	MCSC_F_YUV_CRC_SEED,
	MCSC_F_YUV_MAIN_CTRL_STALL_THROTTLE_EN,
	MCSC_F_YUV_MAIN_CTRL_DJAG_SC_HBI,
	MCSC_F_YUV_MAIN_CTRL_AXI_TRAFFIC_WR_SEL_0,
	MCSC_F_YUV_MAIN_CTRL_AXI_TRAFFIC_WR_SEL_1,
	MCSC_F_CMDQ_QUE_CMD_BASE_ADDR,
	MCSC_F_CMDQ_QUE_CMD_HEADER_NUM,
	MCSC_F_CMDQ_QUE_CMD_SETTING_MODE,
	MCSC_F_CMDQ_QUE_CMD_HOLD_MODE,
	MCSC_F_CMDQ_QUE_CMD_FRAME_ID,
	MCSC_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE,
	MCSC_F_CMDQ_QUE_CMD_FRO_INDEX,
	MCSC_F_CMDQ_ADD_TO_QUEUE_0,
	MCSC_F_CMDQ_AUTO_CONV_ENABLE,
	MCSC_F_CMDQ_POP_LOCK,
	MCSC_F_CMDQ_RELOAD_LOCK,
	MCSC_F_CMDQ_CTRL_SETSEL_EN,
	MCSC_F_CMDQ_SETSEL,
	MCSC_F_CMDQ_FLUSH_QUEUE_0,
	MCSC_F_CMDQ_SWAP_QUEUE_0_TRIG,
	MCSC_F_CMDQ_SWAP_QUEUE_0_POS_A,
	MCSC_F_CMDQ_SWAP_QUEUE_0_POS_B,
	MCSC_F_CMDQ_ROTATE_QUEUE_0_TRIG,
	MCSC_F_CMDQ_ROTATE_QUEUE_0_POS_S,
	MCSC_F_CMDQ_ROTATE_QUEUE_0_POS_E,
	MCSC_F_CMDQ_HM_QUEUE_0_TRIG,
	MCSC_F_CMDQ_HM_QUEUE_0,
	MCSC_F_CMDQ_HM_FRAME_ID_QUEUE_0,
	MCSC_F_CMDQ_CHARGED_FRAME_ID,
	MCSC_F_CMDQ_CHARGED_FOR_NEXT_FRAME,
	MCSC_F_CMDQ_VHD_VBLANK_QRUN_ENABLE,
	MCSC_F_CMDQ_VHD_STALL_ON_QSTOP_ENABLE,
	MCSC_F_CMDQ_FRAME_COUNTER_INC_TYPE,
	MCSC_F_CMDQ_FRAME_COUNTER_RESET,
	MCSC_F_CMDQ_FRAME_COUNTER,
	MCSC_F_CMDQ_PRE_FRAME_ID,
	MCSC_F_CMDQ_CURRENT_FRAME_ID,
	MCSC_F_CMDQ_QUEUE_0_FULLNESS,
	MCSC_F_CMDQ_QUEUE_0_WPTR,
	MCSC_F_CMDQ_QUEUE_0_RPTR,
	MCSC_F_CMDQ_QUEUE_0_RPTR_FOR_DEBUG,
	MCSC_F_CMDQ_DEBUG_QUE_0_CMD_H,
	MCSC_F_CMDQ_DEBUG_QUE_0_CMD_M,
	MCSC_F_CMDQ_DEBUG_QUE_0_CMD_L,
	MCSC_F_CMDQ_DEBUG_PROCESS,
	MCSC_F_CMDQ_DEBUG_HOLD,
	MCSC_F_CMDQ_DEBUG_PERIOD,
	MCSC_F_CMDQ_INT,
	MCSC_F_CMDQ_INT_ENABLE,
	MCSC_F_CMDQ_INT_STATUS,
	MCSC_F_CMDQ_INT_CLEAR,
	MCSC_F_C_LOADER_ENABLE,
	MCSC_F_C_LOADER_RESET,
	MCSC_F_C_LOADER_FAST_MODE,
	MCSC_F_C_LOADER_REMAP_TO_SHADOW_EN,
	MCSC_F_C_LOADER_REMAP_TO_DIRECT_EN,
	MCSC_F_C_LOADER_REMAP_SETSEL_EN,
	MCSC_F_C_LOADER_ACCESS_INTERVAL,
	MCSC_F_C_LOADER_REMAP_00_SETA_ADDR,
	MCSC_F_C_LOADER_REMAP_00_SETB_ADDR,
	MCSC_F_C_LOADER_REMAP_01_SETA_ADDR,
	MCSC_F_C_LOADER_REMAP_01_SETB_ADDR,
	MCSC_F_C_LOADER_REMAP_02_SETA_ADDR,
	MCSC_F_C_LOADER_REMAP_02_SETB_ADDR,
	MCSC_F_C_LOADER_REMAP_03_SETA_ADDR,
	MCSC_F_C_LOADER_REMAP_03_SETB_ADDR,
	MCSC_F_C_LOADER_REMAP_04_SETA_ADDR,
	MCSC_F_C_LOADER_REMAP_04_SETB_ADDR,
	MCSC_F_C_LOADER_REMAP_05_SETA_ADDR,
	MCSC_F_C_LOADER_REMAP_05_SETB_ADDR,
	MCSC_F_C_LOADER_REMAP_06_SETA_ADDR,
	MCSC_F_C_LOADER_REMAP_06_SETB_ADDR,
	MCSC_F_C_LOADER_REMAP_07_SETA_ADDR,
	MCSC_F_C_LOADER_REMAP_07_SETB_ADDR,
	MCSC_F_C_LOADER_LOGICAL_OFFSET_EN,
	MCSC_F_C_LOADER_LOGICAL_OFFSET_IP,
	MCSC_F_C_LOADER_LOGICAL_OFFSET_VOTF_R,
	MCSC_F_C_LOADER_LOGICAL_OFFSET_VOTF_W,
	MCSC_F_C_LOADER_BUSY,
	MCSC_F_C_LOADER_NUM_OF_HEADER_TO_REQ,
	MCSC_F_C_LOADER_NUM_OF_HEADER_REQED,
	MCSC_F_C_LOADER_NUM_OF_HEADER_APBED,
	MCSC_F_C_LOADER_NUM_OF_HEADER_SKIPED,
	MCSC_F_C_LOADER_HEADER_CRC_SEED,
	MCSC_F_C_LOADER_PAYLOAD_CRC_SEED,
	MCSC_F_C_LOADER_HEADER_CRC_RESULT,
	MCSC_F_C_LOADER_PAYLOAD_CRC_RESULT,
	MCSC_F_COREX_ENABLE,
	MCSC_F_COREX_RESET,
	MCSC_F_COREX_FAST_MODE,
	MCSC_F_COREX_UPDATE_TYPE_0,
	MCSC_F_COREX_UPDATE_TYPE_1,
	MCSC_F_COREX_UPDATE_MODE_0,
	MCSC_F_COREX_UPDATE_MODE_1,
	MCSC_F_COREX_START_0,
	MCSC_F_COREX_START_1,
	MCSC_F_COREX_COPY_FROM_IP_0,
	MCSC_F_COREX_COPY_FROM_IP_1,
	MCSC_F_COREX_BUSY_0,
	MCSC_F_COREX_IP_SET_0,
	MCSC_F_COREX_BUSY_1,
	MCSC_F_COREX_IP_SET_1,
	MCSC_F_COREX_PRE_ADDR_CONFIG,
	MCSC_F_COREX_PRE_DATA_CONFIG,
	MCSC_F_COREX_POST_ADDR_CONFIG,
	MCSC_F_COREX_POST_DATA_CONFIG,
	MCSC_F_COREX_PRE_CONFIG_EN,
	MCSC_F_COREX_POST_CONFIG_EN,
	MCSC_F_COREX_TYPE_WRITE,
	MCSC_F_COREX_TYPE_WRITE_TRIGGER,
	MCSC_F_COREX_TYPE_READ,
	MCSC_F_COREX_TYPE_READ_OFFSET,
	MCSC_F_COREX_INT,
	MCSC_F_COREX_INT_STATUS,
	MCSC_F_COREX_INT_CLEAR,
	MCSC_F_COREX_INT_ENABLE,
	MCSC_F_INT_REQ_INT0,
	MCSC_F_INT_REQ_INT0_ENABLE,
	MCSC_F_INT_REQ_INT0_STATUS,
	MCSC_F_INT_REQ_INT0_CLEAR,
	MCSC_F_INT_REQ_INT1,
	MCSC_F_INT_REQ_INT1_ENABLE,
	MCSC_F_INT_REQ_INT1_STATUS,
	MCSC_F_INT_REQ_INT1_CLEAR,
	MCSC_F_INT_HIST_CURINT0,
	MCSC_F_INT_HIST_CURINT0_ENABLE,
	MCSC_F_INT_HIST_CURINT0_STATUS,
	MCSC_F_INT_HIST_CURINT1,
	MCSC_F_INT_HIST_CURINT1_ENABLE,
	MCSC_F_INT_HIST_CURINT1_STATUS,
	MCSC_F_INT_HIST_00_FRAME_ID,
	MCSC_F_INT_HIST_00_INT0,
	MCSC_F_INT_HIST_00_INT1,
	MCSC_F_INT_HIST_01_FRAME_ID,
	MCSC_F_INT_HIST_01_INT0,
	MCSC_F_INT_HIST_01_INT1,
	MCSC_F_INT_HIST_02_FRAME_ID,
	MCSC_F_INT_HIST_02_INT0,
	MCSC_F_INT_HIST_02_INT1,
	MCSC_F_INT_HIST_03_FRAME_ID,
	MCSC_F_INT_HIST_03_INT0,
	MCSC_F_INT_HIST_03_INT1,
	MCSC_F_INT_HIST_04_FRAME_ID,
	MCSC_F_INT_HIST_04_INT0,
	MCSC_F_INT_HIST_04_INT1,
	MCSC_F_INT_HIST_05_FRAME_ID,
	MCSC_F_INT_HIST_05_INT0,
	MCSC_F_INT_HIST_05_INT1,
	MCSC_F_INT_HIST_06_FRAME_ID,
	MCSC_F_INT_HIST_06_INT0,
	MCSC_F_INT_HIST_06_INT1,
	MCSC_F_INT_HIST_07_FRAME_ID,
	MCSC_F_INT_HIST_07_INT0,
	MCSC_F_INT_HIST_07_INT1,
	MCSC_F_SECU_CTRL_SEQID,
	MCSC_F_SECU_CTRL_TZINFO_SEQID_0,
	MCSC_F_SECU_CTRL_TZINFO_SEQID_1,
	MCSC_F_SECU_CTRL_TZINFO_SEQID_2,
	MCSC_F_SECU_CTRL_TZINFO_SEQID_3,
	MCSC_F_SECU_CTRL_TZINFO_SEQID_4,
	MCSC_F_SECU_CTRL_TZINFO_SEQID_5,
	MCSC_F_SECU_CTRL_TZINFO_SEQID_6,
	MCSC_F_SECU_CTRL_TZINFO_SEQID_7,
	MCSC_F_SECU_OTF_SEQ_ID_PROT_ENABLE,
	MCSC_F_PERF_MONITOR_ENABLE,
	MCSC_F_PERF_MONITOR_CLEAR,
	MCSC_F_PERF_MONITOR_INT_USER_SEL,
	MCSC_F_PERF_MONITOR_INT_START,
	MCSC_F_PERF_MONITOR_INT_END,
	MCSC_F_PERF_MONITOR_INT_USER,
	MCSC_F_PERF_MONITOR_PROCESS_PRE_CONFIG,
	MCSC_F_PERF_MONITOR_PROCESS_FRAME,
	MCSC_F_IP_MICRO,
	MCSC_F_IP_MINOR,
	MCSC_F_IP_MAJOR,
	MCSC_F_CTRL_MICRO,
	MCSC_F_CTRL_MINOR,
	MCSC_F_CTRL_MAJOR,
	MCSC_F_QCH_STATUS,
	MCSC_F_IDLENESS_STATUS,
	MCSC_F_CHAIN_IDLENESS_STATUS,
	MCSC_F_DEBUG_COUNTER_0_SIG_SEL,
	MCSC_F_DEBUG_COUNTER_1_SIG_SEL,
	MCSC_F_DEBUG_COUNTER_2_SIG_SEL,
	MCSC_F_DEBUG_COUNTER_3_SIG_SEL,
	MCSC_F_DEBUG_COUNTER_0,
	MCSC_F_DEBUG_COUNTER_1,
	MCSC_F_DEBUG_COUNTER_2,
	MCSC_F_DEBUG_COUNTER_3,
	MCSC_F_IP_BUSY_MONITOR_0,
	MCSC_F_IP_BUSY_MONITOR_1,
	MCSC_F_IP_BUSY_MONITOR_2,
	MCSC_F_IP_BUSY_MONITOR_3,
	MCSC_F_IP_STALL_OUT_STATUS_0,
	MCSC_F_IP_STALL_OUT_STATUS_1,
	MCSC_F_IP_STALL_OUT_STATUS_2,
	MCSC_F_IP_STALL_OUT_STATUS_3,
	MCSC_F_STOPEN_CRC_STOP_VALID_COUNT,
	MCSC_F_SFR_ACCESS_LOG_ENABLE,
	MCSC_F_SFR_ACCESS_LOG_CLEAR,
	MCSC_F_SFR_ACCESS_LOG_0_ADJUST_RANGE,
	MCSC_F_SFR_ACCESS_LOG_0,
	MCSC_F_SFR_ACCESS_LOG_0_ADDRESS,
	MCSC_F_SFR_ACCESS_LOG_1_ADJUST_RANGE,
	MCSC_F_SFR_ACCESS_LOG_1,
	MCSC_F_SFR_ACCESS_LOG_1_ADDRESS,
	MCSC_F_SFR_ACCESS_LOG_2_ADJUST_RANGE,
	MCSC_F_SFR_ACCESS_LOG_2,
	MCSC_F_SFR_ACCESS_LOG_2_ADDRESS,
	MCSC_F_SFR_ACCESS_LOG_3_ADJUST_RANGE,
	MCSC_F_SFR_ACCESS_LOG_3,
	MCSC_F_SFR_ACCESS_LOG_3_ADDRESS,
	MCSC_F_IP_ROL_RESET,
	MCSC_F_IP_ROL_MODE,
	MCSC_F_IP_ROL_SELECT,
	MCSC_F_IP_INT_SRC_SEL,
	MCSC_F_IP_INT_COL_EN,
	MCSC_F_IP_INT_ROW_EN,
	MCSC_F_IP_INT_COL_POS,
	MCSC_F_IP_INT_ROW_POS,
	MCSC_F_FREEZE_FOR_DEBUG,
	MCSC_F_FREEZE_BY_SFR_ENABLE,
	MCSC_F_FREEZE_BY_INT1_ENABLE,
	MCSC_F_FREEZE_BY_INT0_ENABLE,
	MCSC_F_FREEZE_SRC_SEL,
	MCSC_F_FREEZE_EN,
	MCSC_F_FREEZE_COL_POS,
	MCSC_F_FREEZE_ROW_POS,
	MCSC_F_FREEZE_CORRUPTED_ENABLE,
	MCSC_F_YUV_CINFIFO_ENABLE,
	MCSC_F_YUV_CINFIFO_STALL_BEFORE_FRAME_START_EN,
	MCSC_F_YUV_CINFIFO_AUTO_RECOVERY_EN,
	MCSC_F_YUV_CINFIFO_ROL_EN,
	MCSC_F_YUV_CINFIFO_ROL_RESET_ON_FRAME_START,
	MCSC_F_YUV_CINFIFO_DEBUG_EN,
	MCSC_F_YUV_CINFIFO_STRGEN_MODE_EN,
	MCSC_F_YUV_CINFIFO_STRGEN_MODE_DATA_TYPE,
	MCSC_F_YUV_CINFIFO_STRGEN_MODE_DATA,
	MCSC_F_YUV_CINFIFO_STALL_THROTTLE_EN,
	MCSC_F_YUV_CINFIFO_INTERVAL_VBLANK,
	MCSC_F_YUV_CINFIFO_INTERVAL_HBLANK,
	MCSC_F_YUV_CINFIFO_INTERVAL_PIXEL,
	MCSC_F_YUV_CINFIFO_OP_STATE_MONITOR,
	MCSC_F_YUV_CINFIFO_ERROR_STATE_MONITOR,
	MCSC_F_YUV_CINFIFO_COL_CNT,
	MCSC_F_YUV_CINFIFO_ROW_CNT,
	MCSC_F_YUV_CINFIFO_STALL_CNT,
	MCSC_F_YUV_CINFIFO_FIFO_FULLNESS,
	MCSC_F_YUV_CINFIFO_INT,
	MCSC_F_YUV_CINFIFO_INT_ENABLE,
	MCSC_F_YUV_CINFIFO_INT_STATUS,
	MCSC_F_YUV_CINFIFO_INT_CLEAR,
	MCSC_F_YUV_CINFIFO_CORRUPTED_COND_ENABLE,
	MCSC_F_YUV_CINFIFO_ROL_SELECT,
	MCSC_F_CINFIFO_INTERVAL_VBLANK_AR,
	MCSC_F_CINFIFO_INTERVAL_HBLANK_AR,
	MCSC_F_YUV_CINFIFO_CRC_SEED,
	MCSC_F_YUV_CINFIFO_CRC_RESULT,
	MCSC_F_STAT_CINFIFODUALLAYER_ENABLE,
	MCSC_F_STAT_CINFIFODUALLAYER_STALL_BEFORE_FRAME_START_EN,
	MCSC_F_STAT_CINFIFODUALLAYER_AUTO_RECOVERY_EN,
	MCSC_F_STAT_CINFIFODUALLAYER_ROL_EN,
	MCSC_F_STAT_CINFIFODUALLAYER_ROL_RESET_ON_FRAME_START,
	MCSC_F_STAT_CINFIFODUALLAYER_DEBUG_EN,
	MCSC_F_STAT_CINFIFODUALLAYER_STRGEN_MODE_EN,
	MCSC_F_STAT_CINFIFODUALLAYER_STRGEN_MODE_DATA_TYPE,
	MCSC_F_STAT_CINFIFODUALLAYER_STRGEN_MODE_DATA,
	MCSC_F_STAT_CINFIFODUALLAYER_STALL_THROTTLE_EN,
	MCSC_F_STAT_CINFIFODUALLAYER_INTERVAL_VBLANK,
	MCSC_F_STAT_CINFIFODUALLAYER_INTERVAL_HBLANK,
	MCSC_F_STAT_CINFIFODUALLAYER_INTERVAL_PIXEL,
	MCSC_F_STAT_CINFIFODUALLAYER_OP_STATE_MONITOR,
	MCSC_F_STAT_CINFIFODUALLAYER_ERROR_STATE_MONITOR,
	MCSC_F_STAT_CINFIFODUALLAYER_COL_CNT,
	MCSC_F_STAT_CINFIFODUALLAYER_ROW_CNT,
	MCSC_F_STAT_CINFIFODUALLAYER_STALL_CNT,
	MCSC_F_STAT_CINFIFODUALLAYER_FIFO_FULLNESS,
	MCSC_F_STAT_CINFIFODUALLAYER_INT,
	MCSC_F_STAT_CINFIFODUALLAYER_INT_ENABLE,
	MCSC_F_STAT_CINFIFODUALLAYER_INT_STATUS,
	MCSC_F_STAT_CINFIFODUALLAYER_INT_CLEAR,
	MCSC_F_STAT_CINFIFODUALLAYER_CORRUPTED_COND_ENABLE,
	MCSC_F_STAT_CINFIFODUALLAYER_ROL_SELECT,
	MCSC_F_CINFIFODUALLAYER_INTERVAL_VBLANK_AR,
	MCSC_F_CINFIFODUALLAYER_INTERVAL_HBLANK_AR,
	MCSC_F_STAT_CINFIFODUALLAYER_CRC_SEED,
	MCSC_F_STAT_CINFIFODUALLAYER_CRC_RESULT,
	MCSC_F_STAT_RDMACL_EN,
	MCSC_F_STAT_RDMACL_CLK_ALWAYS_ON_EN,
	MCSC_F_STAT_RDMACL_SBWC_EN,
	MCSC_F_STAT_RDMACL_AFBC_EN,
	MCSC_F_STAT_RDMACL_SBWC_64B_ALIGN,
	MCSC_F_STAT_RDMACL_SBWC_TERACELL_ENABLE,
	MCSC_F_STAT_RDMACL_SBWC_128B_ALIGN,
	MCSC_F_STAT_RDMACL_DATA_FORMAT_BAYER,
	MCSC_F_STAT_RDMACL_DATA_FORMAT_YUV,
	MCSC_F_STAT_RDMACL_DATA_FORMAT_RGB,
	MCSC_F_STAT_RDMACL_DATA_FORMAT_MSBALIGN,
	MCSC_F_STAT_RDMACL_DATA_FORMAT_MSBALIGN_UNSIGN,
	MCSC_F_STAT_RDMACL_MONO_MODE,
	MCSC_F_STAT_RDMACL_WIDTH,
	MCSC_F_STAT_RDMACL_HEIGHT,
	MCSC_F_STAT_RDMACL_STRIDE_1P,
	MCSC_F_STAT_RDMACL_MAX_MO,
	MCSC_F_STAT_RDMACL_QURGENT_MO,
	MCSC_F_STAT_RDMACL_LINE_GAP,
	MCSC_F_STAT_RDMACL_BUSINFO,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0_LSB_4B,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1_LSB_4B,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2_LSB_4B,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3_LSB_4B,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4_LSB_4B,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5_LSB_4B,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6_LSB_4B,
	MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7_LSB_4B,
	MCSC_F_STAT_RDMACL_IMG_CRC_1P,
	MCSC_F_STAT_RDMACL_MON_STATUS_0,
	MCSC_F_STAT_RDMACL_MON_STATUS_1,
	MCSC_F_STAT_RDMACL_MON_STATUS_2,
	MCSC_F_STAT_RDMACL_MON_STATUS_3,
	MCSC_F_STAT_RDMACL_DEBUG_ENABLE,
	MCSC_F_YUV_WDMASC_W0_EN,
	MCSC_F_YUV_WDMASC_W0_CLK_ALWAYS_ON_EN,
	MCSC_F_YUV_WDMASC_W0_SBWC_EN,
	MCSC_F_YUV_WDMASC_W0_AFBC_EN,
	MCSC_F_YUV_WDMASC_W0_SBWC_64B_ALIGN,
	MCSC_F_YUV_WDMASC_W0_SBWC_TERACELL_ENABLE,
	MCSC_F_YUV_WDMASC_W0_SBWC_128B_ALIGN,
	MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_BAYER,
	MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_YUV,
	MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_RGB,
	MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_4B_SWAP,
	MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_TYPE,
	MCSC_F_YUV_WDMASC_W0_MONO_EN,
	MCSC_F_YUV_WDMASC_W0_COMP_LOSSY_QUALITY_CONTROL,
	MCSC_F_YUV_WDMASC_W0_WIDTH,
	MCSC_F_YUV_WDMASC_W0_HEIGHT,
	MCSC_F_YUV_WDMASC_W0_STRIDE_1P,
	MCSC_F_YUV_WDMASC_W0_STRIDE_2P,
	MCSC_F_YUV_WDMASC_W0_STRIDE_3P,
	MCSC_F_YUV_WDMASC_W0_STRIDE_HEADER_1P,
	MCSC_F_YUV_WDMASC_W0_STRIDE_HEADER_2P,
	MCSC_F_YUV_WDMASC_W0_VOTF_EN,
	MCSC_F_YUV_WDMASC_W0_MAX_MO,
	MCSC_F_YUV_WDMASC_W0_BUSINFO,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W0_IMG_CRC_1P,
	MCSC_F_YUV_WDMASC_W0_IMG_CRC_2P,
	MCSC_F_YUV_WDMASC_W0_IMG_CRC_3P,
	MCSC_F_YUV_WDMASC_W0_HEADER_CRC_1P,
	MCSC_F_YUV_WDMASC_W0_HEADER_CRC_2P,
	MCSC_F_YUV_WDMASC_W0_MON_STATUS_0,
	MCSC_F_YUV_WDMASC_W0_MON_STATUS_1,
	MCSC_F_YUV_WDMASC_W0_MON_STATUS_2,
	MCSC_F_YUV_WDMASC_W0_MON_STATUS_3,
	MCSC_F_YUV_WDMASC_W0_BW_LIMIT_EN,
	MCSC_F_YUV_WDMASC_W0_BW_LIMIT_FREQ_NUM_CYCLE,
	MCSC_F_YUV_WDMASC_W0_BW_LIMIT_AVG_BW,
	MCSC_F_YUV_WDMASC_W0_BW_LIMIT_SLOT_BW,
	MCSC_F_YUV_WDMASC_W0_BW_LIMIT_COMPENSATION_PERIOD,
	MCSC_F_YUV_WDMASC_W0_BW_LIMIT_COMPENSATION_BW,
	MCSC_F_YUV_WDMASC_W0_CACHE_WLU_ENABLE,
	MCSC_F_YUV_WDMASC_W0_CACHE_32B_PARTIAL_ALLOC_ENABLE,
	MCSC_F_YUV_WDMASC_W0_DEBUG_ENABLE,
	MCSC_F_YUV_WDMASC_W0_DEBUG_CAPTURE_ONCE,
	MCSC_F_YUV_WDMASC_W0_DEBUG_CLK_CNT,
	MCSC_F_YUV_WDMASC_W0_DEBUG_AXI_BLK_CNT,
	MCSC_F_YUV_WDMASC_W0_DEBUG_AXI_MO_CNT,
	MCSC_F_YUV_WDMASC_W0_DEBUG_AXI_AW_BLK_CNT,
	MCSC_F_YUV_WDMASC_W0_FLIP_CONTROL,
	MCSC_F_YUV_WDMASC_W0_RGB_ALPHA,
	MCSC_F_YUV_WDMASC_W1_EN,
	MCSC_F_YUV_WDMASC_W1_CLK_ALWAYS_ON_EN,
	MCSC_F_YUV_WDMASC_W1_SBWC_EN,
	MCSC_F_YUV_WDMASC_W1_AFBC_EN,
	MCSC_F_YUV_WDMASC_W1_SBWC_64B_ALIGN,
	MCSC_F_YUV_WDMASC_W1_SBWC_TERACELL_ENABLE,
	MCSC_F_YUV_WDMASC_W1_SBWC_128B_ALIGN,
	MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_BAYER,
	MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_YUV,
	MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_RGB,
	MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_4B_SWAP,
	MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_TYPE,
	MCSC_F_YUV_WDMASC_W1_MONO_EN,
	MCSC_F_YUV_WDMASC_W1_COMP_LOSSY_QUALITY_CONTROL,
	MCSC_F_YUV_WDMASC_W1_WIDTH,
	MCSC_F_YUV_WDMASC_W1_HEIGHT,
	MCSC_F_YUV_WDMASC_W1_STRIDE_1P,
	MCSC_F_YUV_WDMASC_W1_STRIDE_2P,
	MCSC_F_YUV_WDMASC_W1_STRIDE_3P,
	MCSC_F_YUV_WDMASC_W1_STRIDE_HEADER_1P,
	MCSC_F_YUV_WDMASC_W1_STRIDE_HEADER_2P,
	MCSC_F_YUV_WDMASC_W1_VOTF_EN,
	MCSC_F_YUV_WDMASC_W1_MAX_MO,
	MCSC_F_YUV_WDMASC_W1_BUSINFO,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W1_IMG_CRC_1P,
	MCSC_F_YUV_WDMASC_W1_IMG_CRC_2P,
	MCSC_F_YUV_WDMASC_W1_IMG_CRC_3P,
	MCSC_F_YUV_WDMASC_W1_HEADER_CRC_1P,
	MCSC_F_YUV_WDMASC_W1_HEADER_CRC_2P,
	MCSC_F_YUV_WDMASC_W1_MON_STATUS_0,
	MCSC_F_YUV_WDMASC_W1_MON_STATUS_1,
	MCSC_F_YUV_WDMASC_W1_MON_STATUS_2,
	MCSC_F_YUV_WDMASC_W1_MON_STATUS_3,
	MCSC_F_YUV_WDMASC_W1_BW_LIMIT_EN,
	MCSC_F_YUV_WDMASC_W1_BW_LIMIT_FREQ_NUM_CYCLE,
	MCSC_F_YUV_WDMASC_W1_BW_LIMIT_AVG_BW,
	MCSC_F_YUV_WDMASC_W1_BW_LIMIT_SLOT_BW,
	MCSC_F_YUV_WDMASC_W1_BW_LIMIT_COMPENSATION_PERIOD,
	MCSC_F_YUV_WDMASC_W1_BW_LIMIT_COMPENSATION_BW,
	MCSC_F_YUV_WDMASC_W1_CACHE_WLU_ENABLE,
	MCSC_F_YUV_WDMASC_W1_CACHE_32B_PARTIAL_ALLOC_ENABLE,
	MCSC_F_YUV_WDMASC_W1_DEBUG_ENABLE,
	MCSC_F_YUV_WDMASC_W1_DEBUG_CAPTURE_ONCE,
	MCSC_F_YUV_WDMASC_W1_DEBUG_CLK_CNT,
	MCSC_F_YUV_WDMASC_W1_DEBUG_AXI_BLK_CNT,
	MCSC_F_YUV_WDMASC_W1_DEBUG_AXI_MO_CNT,
	MCSC_F_YUV_WDMASC_W1_DEBUG_AXI_AW_BLK_CNT,
	MCSC_F_YUV_WDMASC_W1_FLIP_CONTROL,
	MCSC_F_YUV_WDMASC_W1_RGB_ALPHA,
	MCSC_F_YUV_WDMASC_W2_EN,
	MCSC_F_YUV_WDMASC_W2_CLK_ALWAYS_ON_EN,
	MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_BAYER,
	MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_YUV,
	MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_RGB,
	MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_4B_SWAP,
	MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_TYPE,
	MCSC_F_YUV_WDMASC_W2_MONO_EN,
	MCSC_F_YUV_WDMASC_W2_WIDTH,
	MCSC_F_YUV_WDMASC_W2_HEIGHT,
	MCSC_F_YUV_WDMASC_W2_STRIDE_1P,
	MCSC_F_YUV_WDMASC_W2_STRIDE_2P,
	MCSC_F_YUV_WDMASC_W2_STRIDE_3P,
	MCSC_F_YUV_WDMASC_W2_VOTF_EN,
	MCSC_F_YUV_WDMASC_W2_MAX_MO,
	MCSC_F_YUV_WDMASC_W2_BUSINFO,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W2_IMG_CRC_1P,
	MCSC_F_YUV_WDMASC_W2_IMG_CRC_2P,
	MCSC_F_YUV_WDMASC_W2_IMG_CRC_3P,
	MCSC_F_YUV_WDMASC_W2_MON_STATUS_0,
	MCSC_F_YUV_WDMASC_W2_MON_STATUS_1,
	MCSC_F_YUV_WDMASC_W2_MON_STATUS_2,
	MCSC_F_YUV_WDMASC_W2_MON_STATUS_3,
	MCSC_F_YUV_WDMASC_W2_BW_LIMIT_EN,
	MCSC_F_YUV_WDMASC_W2_BW_LIMIT_FREQ_NUM_CYCLE,
	MCSC_F_YUV_WDMASC_W2_BW_LIMIT_AVG_BW,
	MCSC_F_YUV_WDMASC_W2_BW_LIMIT_SLOT_BW,
	MCSC_F_YUV_WDMASC_W2_BW_LIMIT_COMPENSATION_PERIOD,
	MCSC_F_YUV_WDMASC_W2_BW_LIMIT_COMPENSATION_BW,
	MCSC_F_YUV_WDMASC_W2_CACHE_WLU_ENABLE,
	MCSC_F_YUV_WDMASC_W2_CACHE_32B_PARTIAL_ALLOC_ENABLE,
	MCSC_F_YUV_WDMASC_W2_DEBUG_ENABLE,
	MCSC_F_YUV_WDMASC_W2_DEBUG_CAPTURE_ONCE,
	MCSC_F_YUV_WDMASC_W2_DEBUG_CLK_CNT,
	MCSC_F_YUV_WDMASC_W2_DEBUG_AXI_BLK_CNT,
	MCSC_F_YUV_WDMASC_W2_DEBUG_AXI_MO_CNT,
	MCSC_F_YUV_WDMASC_W2_DEBUG_AXI_AW_BLK_CNT,
	MCSC_F_YUV_WDMASC_W2_FLIP_CONTROL,
	MCSC_F_YUV_WDMASC_W2_RGB_ALPHA,
	MCSC_F_YUV_WDMASC_W3_EN,
	MCSC_F_YUV_WDMASC_W3_CLK_ALWAYS_ON_EN,
	MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_BAYER,
	MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_YUV,
	MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_RGB,
	MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_4B_SWAP,
	MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_TYPE,
	MCSC_F_YUV_WDMASC_W3_MONO_EN,
	MCSC_F_YUV_WDMASC_W3_WIDTH,
	MCSC_F_YUV_WDMASC_W3_HEIGHT,
	MCSC_F_YUV_WDMASC_W3_STRIDE_1P,
	MCSC_F_YUV_WDMASC_W3_STRIDE_2P,
	MCSC_F_YUV_WDMASC_W3_STRIDE_3P,
	MCSC_F_YUV_WDMASC_W3_VOTF_EN,
	MCSC_F_YUV_WDMASC_W3_MAX_MO,
	MCSC_F_YUV_WDMASC_W3_BUSINFO,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W3_IMG_CRC_1P,
	MCSC_F_YUV_WDMASC_W3_IMG_CRC_2P,
	MCSC_F_YUV_WDMASC_W3_IMG_CRC_3P,
	MCSC_F_YUV_WDMASC_W3_MON_STATUS_0,
	MCSC_F_YUV_WDMASC_W3_MON_STATUS_1,
	MCSC_F_YUV_WDMASC_W3_MON_STATUS_2,
	MCSC_F_YUV_WDMASC_W3_MON_STATUS_3,
	MCSC_F_YUV_WDMASC_W3_BW_LIMIT_EN,
	MCSC_F_YUV_WDMASC_W3_BW_LIMIT_FREQ_NUM_CYCLE,
	MCSC_F_YUV_WDMASC_W3_BW_LIMIT_AVG_BW,
	MCSC_F_YUV_WDMASC_W3_BW_LIMIT_SLOT_BW,
	MCSC_F_YUV_WDMASC_W3_BW_LIMIT_COMPENSATION_PERIOD,
	MCSC_F_YUV_WDMASC_W3_BW_LIMIT_COMPENSATION_BW,
	MCSC_F_YUV_WDMASC_W3_CACHE_WLU_ENABLE,
	MCSC_F_YUV_WDMASC_W3_CACHE_32B_PARTIAL_ALLOC_ENABLE,
	MCSC_F_YUV_WDMASC_W3_DEBUG_ENABLE,
	MCSC_F_YUV_WDMASC_W3_DEBUG_CAPTURE_ONCE,
	MCSC_F_YUV_WDMASC_W3_DEBUG_CLK_CNT,
	MCSC_F_YUV_WDMASC_W3_DEBUG_AXI_BLK_CNT,
	MCSC_F_YUV_WDMASC_W3_DEBUG_AXI_MO_CNT,
	MCSC_F_YUV_WDMASC_W3_DEBUG_AXI_AW_BLK_CNT,
	MCSC_F_YUV_WDMASC_W3_FLIP_CONTROL,
	MCSC_F_YUV_WDMASC_W3_RGB_ALPHA,
	MCSC_F_YUV_WDMASC_W4_EN,
	MCSC_F_YUV_WDMASC_W4_CLK_ALWAYS_ON_EN,
	MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_BAYER,
	MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_YUV,
	MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_RGB,
	MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_4B_SWAP,
	MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_TYPE,
	MCSC_F_YUV_WDMASC_W4_MONO_EN,
	MCSC_F_YUV_WDMASC_W4_WIDTH,
	MCSC_F_YUV_WDMASC_W4_HEIGHT,
	MCSC_F_YUV_WDMASC_W4_STRIDE_1P,
	MCSC_F_YUV_WDMASC_W4_STRIDE_2P,
	MCSC_F_YUV_WDMASC_W4_STRIDE_3P,
	MCSC_F_YUV_WDMASC_W4_VOTF_EN,
	MCSC_F_YUV_WDMASC_W4_MAX_MO,
	MCSC_F_YUV_WDMASC_W4_BUSINFO,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_0,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_1,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_2,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_3,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_4,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_5,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_6,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_7,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6,
	MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7,
	MCSC_F_YUV_WDMASC_W4_IMG_CRC_1P,
	MCSC_F_YUV_WDMASC_W4_IMG_CRC_2P,
	MCSC_F_YUV_WDMASC_W4_IMG_CRC_3P,
	MCSC_F_YUV_WDMASC_W4_MON_STATUS_0,
	MCSC_F_YUV_WDMASC_W4_MON_STATUS_1,
	MCSC_F_YUV_WDMASC_W4_MON_STATUS_2,
	MCSC_F_YUV_WDMASC_W4_MON_STATUS_3,
	MCSC_F_YUV_WDMASC_W4_BW_LIMIT_EN,
	MCSC_F_YUV_WDMASC_W4_BW_LIMIT_FREQ_NUM_CYCLE,
	MCSC_F_YUV_WDMASC_W4_BW_LIMIT_AVG_BW,
	MCSC_F_YUV_WDMASC_W4_BW_LIMIT_SLOT_BW,
	MCSC_F_YUV_WDMASC_W4_BW_LIMIT_COMPENSATION_PERIOD,
	MCSC_F_YUV_WDMASC_W4_BW_LIMIT_COMPENSATION_BW,
	MCSC_F_YUV_WDMASC_W4_CACHE_WLU_ENABLE,
	MCSC_F_YUV_WDMASC_W4_CACHE_32B_PARTIAL_ALLOC_ENABLE,
	MCSC_F_YUV_WDMASC_W4_DEBUG_ENABLE,
	MCSC_F_YUV_WDMASC_W4_DEBUG_CAPTURE_ONCE,
	MCSC_F_YUV_WDMASC_W4_DEBUG_CLK_CNT,
	MCSC_F_YUV_WDMASC_W4_DEBUG_AXI_BLK_CNT,
	MCSC_F_YUV_WDMASC_W4_DEBUG_AXI_MO_CNT,
	MCSC_F_YUV_WDMASC_W4_DEBUG_AXI_AW_BLK_CNT,
	MCSC_F_YUV_WDMASC_W4_FLIP_CONTROL,
	MCSC_F_YUV_WDMASC_W4_RGB_ALPHA,
	MCSC_F_YUV_WDMASC_W0_DITHER_EN_C,
	MCSC_F_YUV_WDMASC_W0_DITHER_EN_Y,
	MCSC_F_YUV_WDMASC_W0_ROUND_EN,
	MCSC_F_YUV_WDMASC_W0_RGB_CONV444_WEIGHT,
	MCSC_F_YUV_WDMASC_W0_PER_SUB_FRAME_EN,
	MCSC_F_YUV_WDMASC_W0_COMP_SRAM_START_ADDR,
	MCSC_F_YUV_WDMASC_W1_DITHER_EN_C,
	MCSC_F_YUV_WDMASC_W1_DITHER_EN_Y,
	MCSC_F_YUV_WDMASC_W1_ROUND_EN,
	MCSC_F_YUV_WDMASC_W1_RGB_CONV444_WEIGHT,
	MCSC_F_YUV_WDMASC_W1_PER_SUB_FRAME_EN,
	MCSC_F_YUV_WDMASC_W1_COMP_SRAM_START_ADDR,
	MCSC_F_YUV_WDMASC_W2_DITHER_EN_C,
	MCSC_F_YUV_WDMASC_W2_DITHER_EN_Y,
	MCSC_F_YUV_WDMASC_W2_ROUND_EN,
	MCSC_F_YUV_WDMASC_W2_RGB_CONV444_WEIGHT,
	MCSC_F_YUV_WDMASC_W2_PER_SUB_FRAME_EN,
	MCSC_F_YUV_WDMASC_W3_DITHER_EN_C,
	MCSC_F_YUV_WDMASC_W3_DITHER_EN_Y,
	MCSC_F_YUV_WDMASC_W3_ROUND_EN,
	MCSC_F_YUV_WDMASC_W3_RGB_CONV444_WEIGHT,
	MCSC_F_YUV_WDMASC_W3_PER_SUB_FRAME_EN,
	MCSC_F_YUV_WDMASC_W4_DITHER_EN_C,
	MCSC_F_YUV_WDMASC_W4_DITHER_EN_Y,
	MCSC_F_YUV_WDMASC_W4_ROUND_EN,
	MCSC_F_YUV_WDMASC_W4_RGB_CONV444_WEIGHT,
	MCSC_F_YUV_WDMASC_W4_PER_SUB_FRAME_EN,
	MCSC_F_YUV_WDMASC_RGB_SRC_Y_OFFSET,
	MCSC_F_YUV_WDMASC_RGB_COEF_C10,
	MCSC_F_YUV_WDMASC_RGB_COEF_C00,
	MCSC_F_YUV_WDMASC_RGB_COEF_C01,
	MCSC_F_YUV_WDMASC_RGB_COEF_C20,
	MCSC_F_YUV_WDMASC_RGB_COEF_C21,
	MCSC_F_YUV_WDMASC_RGB_COEF_C11,
	MCSC_F_YUV_WDMASC_RGB_COEF_C12,
	MCSC_F_YUV_WDMASC_RGB_COEF_C02,
	MCSC_F_YUV_WDMASC_RGB_COEF_C22,
	MCSC_F_YUV_DJAGPS_PS_ENABLE,
	MCSC_F_YUV_DJAGFILTER_DJAG_BACKWARD_COMP,
	MCSC_F_YUV_DJAG_CLK_GATE_DISABLE_PS,
	MCSC_F_YUV_DJAG_CLK_GATE_DISABLE_PS_LB,
	MCSC_F_YUV_DJAGPS_PS_STRIP_ENABLE,
	MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_ENABLE,
	MCSC_F_YUV_DJAGFILTER_EZPOST_ENABLE,
	MCSC_F_YUV_DJAGPS_DUALLAYERNR_ENABLE,
	MCSC_F_YUV_DJAGPS_INPUT_IMG_VSIZE,
	MCSC_F_YUV_DJAGPS_INPUT_IMG_HSIZE,
	MCSC_F_YUV_DJAGPS_PS_SRC_VPOS,
	MCSC_F_YUV_DJAGPS_PS_SRC_HPOS,
	MCSC_F_YUV_DJAGPS_PS_SRC_VSIZE,
	MCSC_F_YUV_DJAGPS_PS_SRC_HSIZE,
	MCSC_F_YUV_DJAGPS_PS_DST_VSIZE,
	MCSC_F_YUV_DJAGPS_PS_DST_HSIZE,
	MCSC_F_YUV_DJAGPS_PS_H_RATIO,
	MCSC_F_YUV_DJAGPS_PS_V_RATIO,
	MCSC_F_YUV_DJAGPS_PS_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_DJAGPS_PS_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_DJAGPS_PS_ROUND_MODE,
	MCSC_F_YUV_DJAGPS_PS_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_DJAGPS_PS_STRIP_IN_START_POS_H,
	MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_POS_V,
	MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_POS_H,
	MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_SIZE_V,
	MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_SIZE_H,
	MCSC_F_YUV_DJAGFILTER_XFILTER_DEJAGGING_WEIGHT0,
	MCSC_F_YUV_DJAGFILTER_XFILTER_DEJAGGING_WEIGHT1,
	MCSC_F_YUV_DJAGFILTER_XFILTER_HF_BOOST_WEIGHT,
	MCSC_F_YUV_DJAGFILTER_CENTER_HF_BOOST_WEIGHT,
	MCSC_F_YUV_DJAGFILTER_DIAGONAL_HF_BOOST_WEIGHT,
	MCSC_F_YUV_DJAGFILTER_CENTER_WEIGHTED_MEAN_WEIGHT,
	MCSC_F_YUV_DJAGFILTER_THRES_1X5_MATCHING_SAD,
	MCSC_F_YUV_DJAGFILTER_THRES_1X5_ABSHF,
	MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_LLCRR,
	MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_LCR,
	MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_NEIGHBOR,
	MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_UUCDD,
	MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_UCD,
	MCSC_F_YUV_DJAGFILTER_MIN_MAX_WEIGHT,
	MCSC_F_YUV_DJAGFILTER_LFSR_SEED_0,
	MCSC_F_YUV_DJAGFILTER_LFSR_SEED_1,
	MCSC_F_YUV_DJAGFILTER_LFSR_SEED_2,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_0,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_1,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_2,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_3,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_4,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_5,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_6,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_7,
	MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_8,
	MCSC_F_YUV_DJAGFILTER_SAT_CTRL,
	MCSC_F_YUV_DJAGFILTER_DITHER_THRES,
	MCSC_F_YUV_DJAGFILTER_CP_HF_THRES,
	MCSC_F_YUV_DJAGFILTER_CP_ARBI_MAX_COV_OFFSET,
	MCSC_F_YUV_DJAGFILTER_CP_ARBI_MAX_COV_SHIFT,
	MCSC_F_YUV_DJAGFILTER_CP_ARBI_DENOM,
	MCSC_F_YUV_DJAGFILTER_CP_ARBI_MODE,
	MCSC_F_YUV_DJAGFILTER_DITHER_WB_THRES,
	MCSC_F_YUV_DJAGFILTER_DITHER_BLACK_LEVEL,
	MCSC_F_YUV_DJAGFILTER_DITHER_WHITE_LEVEL,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_0_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_0_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_0_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_0_3,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_1_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_1_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_1_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_1_3,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_2_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_2_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_2_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_2_3,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_3_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_3_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_3_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_3_3,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_4_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_4_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_4_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_4_3,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_5_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_5_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_5_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_5_3,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_6_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_6_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_6_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_6_3,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_7_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_7_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_7_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_7_3,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_8_0,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_8_1,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_8_2,
	MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_8_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_7,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_7,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_7,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_7,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_7,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_7,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_7,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_7,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_0,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_1,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_2,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_3,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_4,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_5,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_6,
	MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_7,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_0_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_0_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_0_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_0_3,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_1_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_1_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_1_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_1_3,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_2_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_2_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_2_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_2_3,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_3_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_3_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_3_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_3_3,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_3,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_5_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_5_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_5_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_5_3,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_6_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_6_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_6_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_6_3,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_7_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_7_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_7_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_7_3,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_8_0,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_8_1,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_8_2,
	MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_8_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_0_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_0_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_0_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_0_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_1_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_1_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_1_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_1_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_3_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_3_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_3_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_3_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_4_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_4_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_4_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_4_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_5_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_5_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_5_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_5_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_6_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_6_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_6_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_6_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_7_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_7_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_7_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_7_3,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_8_0,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_8_1,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_8_2,
	MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_8_3,
	MCSC_F_YUV_DJAGPREFILTER_EZPOST_ENABLE,
	MCSC_F_YUV_DJAGPREFILTER_CLK_GATE_DISABLE_PS,
	MCSC_F_YUV_DJAGPREFILTER_CLK_GATE_DISABLE_PS_LB,
	MCSC_F_YUV_DJAGPREFILTER_INPUT_IMG_VSIZE,
	MCSC_F_YUV_DJAGPREFILTER_INPUT_IMG_HSIZE,
	MCSC_F_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_WEIGHT0,
	MCSC_F_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_WEIGHT1,
	MCSC_F_YUV_DJAGPREFILTER_XFILTER_HF_BOOST_WEIGHT,
	MCSC_F_YUV_DJAGPREFILTER_CENTER_HF_BOOST_WEIGHT,
	MCSC_F_YUV_DJAGPREFILTER_DIAGONAL_HF_BOOST_WEIGHT,
	MCSC_F_YUV_DJAGPREFILTER_CENTER_WEIGHTED_MEAN_WEIGHT,
	MCSC_F_YUV_DJAGPREFILTER_THRES_1X5_MATCHING_SAD,
	MCSC_F_YUV_DJAGPREFILTER_THRES_1X5_ABSHF,
	MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_LLCRR,
	MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_LCR,
	MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_NEIGHBOR,
	MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_UUCDD,
	MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_UCD,
	MCSC_F_YUV_DJAGPREFILTER_MIN_MAX_WEIGHT,
	MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_0,
	MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_1,
	MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_2,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_0,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_1,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_2,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_3,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_4,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_5,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_6,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_7,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_8,
	MCSC_F_YUV_DJAGPREFILTER_SAT_CTRL,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_THRES,
	MCSC_F_YUV_DJAGPREFILTER_CP_HF_THRES,
	MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MAX_COV_OFFSET,
	MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MAX_COV_SHIFT,
	MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_DENOM,
	MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MODE,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_WB_THRES,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_BLACK_LEVEL,
	MCSC_F_YUV_DJAGPREFILTER_DITHER_WHITE_LEVEL,
	MCSC_F_YUV_DJAGPREFILTER_HARRIS_K,
	MCSC_F_YUV_DJAGPREFILTER_HARRIS_TH,
	MCSC_F_YUV_DJAGPREFILTER_BILATERAL_C,
	MCSC_F_YUV_DUALLAYERRECOMP_BYPASS,
	MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_ENABLE,
	MCSC_F_YUV_DUALLAYERRECOMP_WEIGHT,
	MCSC_F_YUV_DUALLAYERRECOMP_INPUT_IMG_VSIZE,
	MCSC_F_YUV_DUALLAYERRECOMP_INPUT_IMG_HSIZE,
	MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_POS_V,
	MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_POS_H,
	MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_SIZE_V,
	MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_SIZE_H,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_CENTER_Y,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_CENTER_X,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BINNING_Y,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BINNING_X,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_A,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_B,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_GAIN_ENABLE,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_GAIN_INCREMENT,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_SCALE_SHIFT_ADDER,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_A,
	MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_B,
	MCSC_F_YUV_DUALLAYERRECOMP_CRC_SEED,
	MCSC_F_YUV_DUALLAYERRECOMP_CRC_RESULT,
	MCSC_F_YUV_POLY_SC0_ENABLE,
	MCSC_F_YUV_POLY_SC0_BYPASS,
	MCSC_F_YUV_POLY_SC0_CLK_GATE_DISABLE,
	MCSC_F_YUV_POLY_SC0_STRIP_ENABLE,
	MCSC_F_YUV_POLY_SC0_OUT_CROP_ENABLE,
	MCSC_F_YUV_POLY_SC0_SRC_VPOS,
	MCSC_F_YUV_POLY_SC0_SRC_HPOS,
	MCSC_F_YUV_POLY_SC0_SRC_VSIZE,
	MCSC_F_YUV_POLY_SC0_SRC_HSIZE,
	MCSC_F_YUV_POLY_SC0_DST_VSIZE,
	MCSC_F_YUV_POLY_SC0_DST_HSIZE,
	MCSC_F_YUV_POLY_SC0_H_RATIO,
	MCSC_F_YUV_POLY_SC0_V_RATIO,
	MCSC_F_YUV_POLY_SC0_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC0_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC0_ROUND_MODE,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC0_Y_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_7,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_7,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_7,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_7,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_7,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_7,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_7,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_7,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_4,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_5,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_6,
	MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_7,
	MCSC_F_YUV_POLY_SC1_ENABLE,
	MCSC_F_YUV_POLY_SC1_BYPASS,
	MCSC_F_YUV_POLY_SC1_CLK_GATE_DISABLE,
	MCSC_F_YUV_POLY_SC1_STRIP_ENABLE,
	MCSC_F_YUV_POLY_SC1_OUT_CROP_ENABLE,
	MCSC_F_YUV_POLY_SC1_SRC_VPOS,
	MCSC_F_YUV_POLY_SC1_SRC_HPOS,
	MCSC_F_YUV_POLY_SC1_SRC_VSIZE,
	MCSC_F_YUV_POLY_SC1_SRC_HSIZE,
	MCSC_F_YUV_POLY_SC1_DST_VSIZE,
	MCSC_F_YUV_POLY_SC1_DST_HSIZE,
	MCSC_F_YUV_POLY_SC1_H_RATIO,
	MCSC_F_YUV_POLY_SC1_V_RATIO,
	MCSC_F_YUV_POLY_SC1_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC1_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC1_ROUND_MODE,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC1_Y_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_7,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_7,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_7,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_7,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_7,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_7,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_7,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_7,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_4,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_5,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_6,
	MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_7,
	MCSC_F_YUV_POLY_SC2_ENABLE,
	MCSC_F_YUV_POLY_SC2_BYPASS,
	MCSC_F_YUV_POLY_SC2_CLK_GATE_DISABLE,
	MCSC_F_YUV_POLY_SC2_STRIP_ENABLE,
	MCSC_F_YUV_POLY_SC2_OUT_CROP_ENABLE,
	MCSC_F_YUV_POLY_SC2_SRC_VPOS,
	MCSC_F_YUV_POLY_SC2_SRC_HPOS,
	MCSC_F_YUV_POLY_SC2_SRC_VSIZE,
	MCSC_F_YUV_POLY_SC2_SRC_HSIZE,
	MCSC_F_YUV_POLY_SC2_DST_VSIZE,
	MCSC_F_YUV_POLY_SC2_DST_HSIZE,
	MCSC_F_YUV_POLY_SC2_H_RATIO,
	MCSC_F_YUV_POLY_SC2_V_RATIO,
	MCSC_F_YUV_POLY_SC2_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC2_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC2_ROUND_MODE,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC2_Y_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_7,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_7,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_7,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_7,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_7,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_7,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_7,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_7,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_4,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_5,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_6,
	MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_7,
	MCSC_F_YUV_POLY_SC3_ENABLE,
	MCSC_F_YUV_POLY_SC3_BYPASS,
	MCSC_F_YUV_POLY_SC3_CLK_GATE_DISABLE,
	MCSC_F_YUV_POLY_SC3_STRIP_ENABLE,
	MCSC_F_YUV_POLY_SC3_OUT_CROP_ENABLE,
	MCSC_F_YUV_POLY_SC3_SRC_VPOS,
	MCSC_F_YUV_POLY_SC3_SRC_HPOS,
	MCSC_F_YUV_POLY_SC3_SRC_VSIZE,
	MCSC_F_YUV_POLY_SC3_SRC_HSIZE,
	MCSC_F_YUV_POLY_SC3_DST_VSIZE,
	MCSC_F_YUV_POLY_SC3_DST_HSIZE,
	MCSC_F_YUV_POLY_SC3_H_RATIO,
	MCSC_F_YUV_POLY_SC3_V_RATIO,
	MCSC_F_YUV_POLY_SC3_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC3_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC3_ROUND_MODE,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC3_Y_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_7,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_7,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_7,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_7,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_7,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_7,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_7,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_7,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_4,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_5,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_6,
	MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_7,
	MCSC_F_YUV_POLY_SC4_ENABLE,
	MCSC_F_YUV_POLY_SC4_BYPASS,
	MCSC_F_YUV_POLY_SC4_CLK_GATE_DISABLE,
	MCSC_F_YUV_POLY_SC4_STRIP_ENABLE,
	MCSC_F_YUV_POLY_SC4_OUT_CROP_ENABLE,
	MCSC_F_YUV_POLY_SC4_SRC_VPOS,
	MCSC_F_YUV_POLY_SC4_SRC_HPOS,
	MCSC_F_YUV_POLY_SC4_SRC_VSIZE,
	MCSC_F_YUV_POLY_SC4_SRC_HSIZE,
	MCSC_F_YUV_POLY_SC4_DST_VSIZE,
	MCSC_F_YUV_POLY_SC4_DST_HSIZE,
	MCSC_F_YUV_POLY_SC4_H_RATIO,
	MCSC_F_YUV_POLY_SC4_V_RATIO,
	MCSC_F_YUV_POLY_SC4_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC4_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POLY_SC4_ROUND_MODE,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC4_Y_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_7,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_7,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_7,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_7,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_7,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_7,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_7,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_7,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_4,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_5,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_6,
	MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_7,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC0_UV_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC0_UV_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC1_UV_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC1_UV_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC2_UV_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC2_UV_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC3_UV_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC3_UV_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_0_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_0_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_0_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_0_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_1_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_1_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_1_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_1_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_2_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_2_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_2_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_2_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_3_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_3_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_3_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_3_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_4_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_4_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_4_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_4_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_5_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_5_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_5_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_5_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_6_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_6_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_6_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_6_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_7_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_7_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_7_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_7_3,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_8_0,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_8_1,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_8_2,
	MCSC_F_YUV_POLY_SC4_UV_V_COEFF_8_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_0_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_0_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_0_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_0_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_1_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_1_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_1_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_1_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_2_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_2_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_2_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_2_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_3_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_3_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_3_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_3_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_4_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_4_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_4_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_4_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_5_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_5_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_5_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_5_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_6_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_6_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_6_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_6_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_7_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_7_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_7_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_7_3,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_8_0,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_8_1,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_8_2,
	MCSC_F_YUV_POLY_SC4_UV_H_COEFF_8_3,
	MCSC_F_YUV_POLY_SC0_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_POLY_SC0_STRIP_IN_START_POS_H,
	MCSC_F_YUV_POLY_SC0_OUT_CROP_POS_V,
	MCSC_F_YUV_POLY_SC0_OUT_CROP_POS_H,
	MCSC_F_YUV_POLY_SC0_OUT_CROP_SIZE_V,
	MCSC_F_YUV_POLY_SC0_OUT_CROP_SIZE_H,
	MCSC_F_YUV_POLY_SC1_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_POLY_SC1_STRIP_IN_START_POS_H,
	MCSC_F_YUV_POLY_SC1_OUT_CROP_POS_V,
	MCSC_F_YUV_POLY_SC1_OUT_CROP_POS_H,
	MCSC_F_YUV_POLY_SC1_OUT_CROP_SIZE_V,
	MCSC_F_YUV_POLY_SC1_OUT_CROP_SIZE_H,
	MCSC_F_YUV_POLY_SC2_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_POLY_SC2_STRIP_IN_START_POS_H,
	MCSC_F_YUV_POLY_SC2_OUT_CROP_POS_V,
	MCSC_F_YUV_POLY_SC2_OUT_CROP_POS_H,
	MCSC_F_YUV_POLY_SC2_OUT_CROP_SIZE_V,
	MCSC_F_YUV_POLY_SC2_OUT_CROP_SIZE_H,
	MCSC_F_YUV_POLY_SC3_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_POLY_SC3_STRIP_IN_START_POS_H,
	MCSC_F_YUV_POLY_SC3_OUT_CROP_POS_V,
	MCSC_F_YUV_POLY_SC3_OUT_CROP_POS_H,
	MCSC_F_YUV_POLY_SC3_OUT_CROP_SIZE_V,
	MCSC_F_YUV_POLY_SC3_OUT_CROP_SIZE_H,
	MCSC_F_YUV_POLY_SC4_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_POLY_SC4_STRIP_IN_START_POS_H,
	MCSC_F_YUV_POLY_SC4_OUT_CROP_POS_V,
	MCSC_F_YUV_POLY_SC4_OUT_CROP_POS_H,
	MCSC_F_YUV_POLY_SC4_OUT_CROP_SIZE_V,
	MCSC_F_YUV_POLY_SC4_OUT_CROP_SIZE_H,
	MCSC_F_YUV_POSTPC_PC0_ENABLE,
	MCSC_F_YUV_POST_PC0_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC0_CONV420_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC0_BCHS_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC0_LB_CTRL_CLK_GATE_DISABLE,
	MCSC_F_YUV_POSTPC_PC0_STRIP_ENABLE,
	MCSC_F_YUV_POSTPC_PC0_OUT_CROP_ENABLE,
	MCSC_F_YUV_POSTPC_PC0_IMG_VSIZE,
	MCSC_F_YUV_POSTPC_PC0_IMG_HSIZE,
	MCSC_F_YUV_POSTPC_PC0_DST_VSIZE,
	MCSC_F_YUV_POSTPC_PC0_DST_HSIZE,
	MCSC_F_YUV_POSTPC_PC0_H_RATIO,
	MCSC_F_YUV_POSTPC_PC0_V_RATIO,
	MCSC_F_YUV_POSTPC_PC0_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POSTPC_PC0_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POSTPC_PC0_ROUND_MODE,
	MCSC_F_YUV_POSTPC_PC0_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_POSTPC_PC0_STRIP_IN_START_POS_H,
	MCSC_F_YUV_POSTPC_PC0_OUT_CROP_POS_V,
	MCSC_F_YUV_POSTPC_PC0_OUT_CROP_POS_H,
	MCSC_F_YUV_POSTPC_PC0_OUT_CROP_SIZE_V,
	MCSC_F_YUV_POSTPC_PC0_OUT_CROP_SIZE_H,
	MCSC_F_YUV_POSTCONV_PC0_CONV420_ENABLE,
	MCSC_F_YUV_POSTCONV_PC0_CONV420_ODD_LINE,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_ENABLE,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_GAIN,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_OFFSET,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_00,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_01,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_10,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_11,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_CLAMP_MAX,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_CLAMP_MAX,
	MCSC_F_YUV_POSTPC_PC1_ENABLE,
	MCSC_F_YUV_POST_PC1_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC1_CONV420_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC1_BCHS_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC1_LB_CTRL_CLK_GATE_DISABLE,
	MCSC_F_YUV_POSTPC_PC1_STRIP_ENABLE,
	MCSC_F_YUV_POSTPC_PC1_OUT_CROP_ENABLE,
	MCSC_F_YUV_POSTPC_PC1_IMG_VSIZE,
	MCSC_F_YUV_POSTPC_PC1_IMG_HSIZE,
	MCSC_F_YUV_POSTPC_PC1_DST_VSIZE,
	MCSC_F_YUV_POSTPC_PC1_DST_HSIZE,
	MCSC_F_YUV_POSTPC_PC1_H_RATIO,
	MCSC_F_YUV_POSTPC_PC1_V_RATIO,
	MCSC_F_YUV_POSTPC_PC1_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POSTPC_PC1_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POSTPC_PC1_ROUND_MODE,
	MCSC_F_YUV_POSTPC_PC1_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_POSTPC_PC1_STRIP_IN_START_POS_H,
	MCSC_F_YUV_POSTPC_PC1_OUT_CROP_POS_V,
	MCSC_F_YUV_POSTPC_PC1_OUT_CROP_POS_H,
	MCSC_F_YUV_POSTPC_PC1_OUT_CROP_SIZE_V,
	MCSC_F_YUV_POSTPC_PC1_OUT_CROP_SIZE_H,
	MCSC_F_YUV_POSTCONV_PC1_CONV420_ENABLE,
	MCSC_F_YUV_POSTCONV_PC1_CONV420_ODD_LINE,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_ENABLE,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_GAIN,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_OFFSET,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_GAIN_00,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_GAIN_01,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_GAIN_10,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_GAIN_11,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_CLAMP_MAX,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_CLAMP_MAX,
	MCSC_F_YUV_POSTPC_PC2_ENABLE,
	MCSC_F_YUV_POST_PC2_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC2_CONV420_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC2_BCHS_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC2_LB_CTRL_CLK_GATE_DISABLE,
	MCSC_F_YUV_POSTPC_PC2_STRIP_ENABLE,
	MCSC_F_YUV_POSTPC_PC2_OUT_CROP_ENABLE,
	MCSC_F_YUV_POSTPC_PC2_IMG_VSIZE,
	MCSC_F_YUV_POSTPC_PC2_IMG_HSIZE,
	MCSC_F_YUV_POSTPC_PC2_DST_VSIZE,
	MCSC_F_YUV_POSTPC_PC2_DST_HSIZE,
	MCSC_F_YUV_POSTPC_PC2_H_RATIO,
	MCSC_F_YUV_POSTPC_PC2_V_RATIO,
	MCSC_F_YUV_POSTPC_PC2_H_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POSTPC_PC2_V_INIT_PHASE_OFFSET,
	MCSC_F_YUV_POSTPC_PC2_ROUND_MODE,
	MCSC_F_YUV_POSTPC_PC2_STRIP_PRE_DST_SIZE_H,
	MCSC_F_YUV_POSTPC_PC2_STRIP_IN_START_POS_H,
	MCSC_F_YUV_POSTPC_PC2_OUT_CROP_POS_V,
	MCSC_F_YUV_POSTPC_PC2_OUT_CROP_POS_H,
	MCSC_F_YUV_POSTPC_PC2_OUT_CROP_SIZE_V,
	MCSC_F_YUV_POSTPC_PC2_OUT_CROP_SIZE_H,
	MCSC_F_YUV_POSTCONV_PC2_CONV420_ENABLE,
	MCSC_F_YUV_POSTCONV_PC2_CONV420_ODD_LINE,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_ENABLE,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_GAIN,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_OFFSET,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_GAIN_00,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_GAIN_01,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_GAIN_10,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_GAIN_11,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_CLAMP_MAX,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_CLAMP_MAX,
	MCSC_F_YUV_POST_PC3_CONV420_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC3_BCHS_CLK_GATE_DISABLE,
	MCSC_F_YUV_POSTCONV_PC3_CONV420_ENABLE,
	MCSC_F_YUV_POSTCONV_PC3_CONV420_ODD_LINE,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_ENABLE,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_GAIN,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_OFFSET,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_GAIN_00,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_GAIN_01,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_GAIN_10,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_GAIN_11,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_CLAMP_MAX,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_CLAMP_MAX,
	MCSC_F_YUV_POST_PC4_CONV420_CLK_GATE_DISABLE,
	MCSC_F_YUV_POST_PC4_BCHS_CLK_GATE_DISABLE,
	MCSC_F_YUV_POSTCONV_PC4_CONV420_ENABLE,
	MCSC_F_YUV_POSTCONV_PC4_CONV420_ODD_LINE,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_ENABLE,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_GAIN,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_OFFSET,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_GAIN_00,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_GAIN_01,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_GAIN_10,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_GAIN_11,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_CLAMP_MAX,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_CLAMP_MIN,
	MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_CLAMP_MAX,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_7,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_7,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_7,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_7,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_7,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_7,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_7,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_7,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_4,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_5,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_6,
	MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_7,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_7,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_7,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_7,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_7,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_7,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_7,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_7,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_7,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_4,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_5,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_6,
	MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_7,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_7,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_7,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_7,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_7,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_7,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_7,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_7,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_7,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_4,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_5,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_6,
	MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_7,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_8_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_0_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_0_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_0_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_0_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_1_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_1_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_1_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_1_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_2_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_2_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_2_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_2_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_3_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_3_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_3_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_3_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_4_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_4_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_4_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_4_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_5_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_5_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_5_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_5_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_6_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_6_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_6_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_6_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_7_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_7_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_7_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_7_3,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_8_0,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_8_1,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_8_2,
	MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_8_3,
	MCSC_F_YUV_HWFC_SWRESET,
	MCSC_F_YUV_HWFC_MODE,
	MCSC_F_YUV_HWFC_REGION_IDX_BIN_A,
	MCSC_F_YUV_HWFC_REGION_IDX_BIN_B,
	MCSC_F_YUV_HWFC_REGION_IDX_GRAY_A,
	MCSC_F_YUV_HWFC_REGION_IDX_GRAY_B,
	MCSC_F_YUV_HWFC_CURR_REGION_A,
	MCSC_F_YUV_HWFC_CURR_REGION_B,
	MCSC_F_YUV_HWFC_PLANE_A,
	MCSC_F_YUV_HWFC_ID0_A,
	MCSC_F_YUV_HWFC_ID1_A,
	MCSC_F_YUV_HWFC_ID2_A,
	MCSC_F_YUV_HWFC_FORMAT_A,
	MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE0_A,
	MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE0_A,
	MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE1_A,
	MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE1_A,
	MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE2_A,
	MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE2_A,
	MCSC_F_YUV_HWFC_PLANE_B,
	MCSC_F_YUV_HWFC_ID0_B,
	MCSC_F_YUV_HWFC_ID1_B,
	MCSC_F_YUV_HWFC_ID2_B,
	MCSC_F_YUV_HWFC_FORMAT_B,
	MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE0_B,
	MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE0_B,
	MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE1_B,
	MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE1_B,
	MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE2_B,
	MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE2_B,
	MCSC_F_YUV_HWFC_FRAME_START_SELECT,
	MCSC_F_YUV_HWFC_INDEX_RESET,
	MCSC_F_YUV_HWFC_ENABLE_AUTO_CLEAR,
	MCSC_F_YUV_HWFC_CORE_RESET_INPUT_SEL_A,
	MCSC_F_YUV_HWFC_CORE_RESET_INPUT_SEL_B,
	MCSC_F_YUV_HWFC_MASTER_SEL_A,
	MCSC_F_YUV_HWFC_MASTER_SEL_B,
	MCSC_F_YUV_HWFC_IMAGE_HEIGHT_A,
	MCSC_F_YUV_HWFC_IMAGE_HEIGHT_B,
	MCSC_F_YUV_DTPOTFIN_BYPASS,
	MCSC_F_YUV_DTPOTFIN_TEST_PATTERN_MODE,
	MCSC_F_YUV_DTPOTFIN_TEST_DATA_Y,
	MCSC_F_YUV_DTPOTFIN_TEST_DATA_U,
	MCSC_F_YUV_DTPOTFIN_TEST_DATA_V,
	MCSC_F_YUV_DTPOTFIN_YUV_STANDARD,
	MCSC_F_YUV_DTPOTFIN_CRC_SEED,
	MCSC_F_YUV_DTPOTFIN_CRC_RESULT,
	MCSC_F_STAT_DTPDUALLAYER_BYPASS,
	MCSC_F_STAT_DTPDUALLAYER_TEST_PATTERN_MODE,
	MCSC_F_STAT_DTPDUALLAYER_TEST_DATA_Y,
	MCSC_F_STAT_DTPDUALLAYER_TEST_DATA_U,
	MCSC_F_STAT_DTPDUALLAYER_TEST_DATA_V,
	MCSC_F_STAT_DTPDUALLAYER_YUV_STANDARD,
	MCSC_F_STAT_DTPDUALLAYER_CRC_SEED,
	MCSC_F_STAT_DTPDUALLAYER_CRC_RESULT,
	MCSC_F_DBG_AXI_0,
	MCSC_F_DBG_AXI_1,
	MCSC_F_DBG_AXI_2,
	MCSC_F_DBG_AXI_3,
	MCSC_F_DBG_AXI_4,
	MCSC_F_DBG_AXI_5,
	MCSC_F_DBG_AXI_6,
	MCSC_F_DBG_AXI_7,
	MCSC_F_DBG_AXI_8,
	MCSC_F_DBG_AXI_9,
	MCSC_F_DBG_AXI_10,
	MCSC_F_DBG_AXI_11,
	MCSC_F_DBG_AXI_12,
	MCSC_F_DBG_AXI_13,
	MCSC_F_DBG_AXI_14,
	MCSC_F_DBG_AXI_15,
	MCSC_F_DBG_AXI_16,
	MCSC_F_DBG_AXI_17,
	MCSC_F_DBG_AXI_18,
	MCSC_F_DBG_AXI_19,
	MCSC_F_DBG_AXI_20,
	MCSC_F_DBG_AXI_21,
	MCSC_F_DBG_AXI_22,
	MCSC_F_DBG_AXI_23,
	MCSC_F_DBG_AXI_24,
	MCSC_F_DBG_AXI_25,
	MCSC_F_DBG_AXI_26,
	MCSC_F_DBG_AXI_27,
	MCSC_F_DBG_AXI_28,
	MCSC_F_DBG_AXI_29,
	MCSC_F_DBG_AXI_30,
	MCSC_F_DBG_AXI_31,
	MCSC_F_DBG_AXI_32,
	MCSC_F_DBG_AXI_33,
	MCSC_F_DBG_AXI_34,
	MCSC_F_DBG_AXI_35,
	MCSC_F_DBG_AXI_36,
	MCSC_F_DBG_AXI_37,
	MCSC_F_DBG_AXI_38,
	MCSC_F_DBG_AXI_39,
	MCSC_F_DBG_AXI_40,
	MCSC_F_DBG_AXI_41,
	MCSC_F_DBG_AXI_42,
	MCSC_F_DBG_AXI_43,
	MCSC_F_DBG_AXI_44,
	MCSC_F_DBG_AXI_45,
	MCSC_F_DBG_AXI_46,
	MCSC_F_DBG_AXI_47,
	MCSC_F_DBG_AXI_48,
	MCSC_F_DBG_AXI_49,
	MCSC_F_DBG_AXI_58,
	MCSC_F_DBG_AXI_59,
	MCSC_F_DBG_AXI_60,
	MCSC_F_DBG_AXI_61,
	MCSC_F_DBG_SC_0,
	MCSC_F_DBG_SC_1,
	MCSC_F_DBG_SC_2,
	MCSC_F_DBG_SC_3,
	MCSC_F_DBG_SC_4,
	MCSC_F_DBG_SC_5,
	MCSC_F_DBG_SC_6,
	MCSC_F_DBG_SC_7,
	MCSC_F_DBG_SC_8,
	MCSC_F_DBG_SC_9,
	MCSC_F_DBG_SC_10,
	MCSC_F_DBG_SC_11,
	MCSC_F_DBG_SC_12,
	MCSC_F_DBG_SC_13,
	MCSC_F_DBG_SC_14,
	MCSC_F_DBG_SC_15,
	MCSC_F_DBG_SC_16,
	MCSC_F_DBG_SC_17,
	MCSC_F_DBG_SC_18,
	MCSC_F_DBG_SC_19,
	MCSC_F_DBG_SC_20,
	MCSC_F_DBG_SC_21,
	MCSC_F_DBG_SC_22,
	MCSC_F_DBG_SC_23,
	MCSC_F_DBG_SC_24,
	MCSC_F_DBG_SC_25,
	MCSC_F_DBG_SC_26,
	MCSC_F_DBG_SC_27,
	MCSC_F_DBG_SC_28,
	MCSC_F_DBG_SC_29,
	MCSC_F_DBG_SC_30,
	MCSC_F_DBG_SC_31,
	MCSC_F_DBG_SC_32,
	MCSC_F_DBG_SC_33,
	MCSC_F_DBG_SC_34,
	MCSC_F_YUV_DJAGPS_CRC_TOP_IN_OTF,
	MCSC_F_YUV_DJAGPS_CRC_PS,
	MCSC_F_YUV_DJAGFILTER_CRC_EZPOST_OUT,
	MCSC_F_YUV_DJAGOUTCROP_CRC_DJAG_OUT,
	MCSC_F_YUV_POLY_SC0_CRC_POLY,
	MCSC_F_YUV_POLY_SC1_CRC_POLY,
	MCSC_F_YUV_POLY_SC2_CRC_POLY,
	MCSC_F_YUV_POLY_SC3_CRC_POLY,
	MCSC_F_YUV_POLY_SC4_CRC_POLY,
	MCSC_F_YUV_POSTPC_PC0_CRC_POST,
	MCSC_F_YUV_POSTPC_PC1_CRC_POST,
	MCSC_F_YUV_POSTPC_PC2_CRC_POST,
	MCSC_F_YUV_POSTPC_PC3_CRC_POST,
	MCSC_F_YUV_POSTPC_PC4_CRC_POST,
	MCSC_F_YUV_POSTCONV_PC0_CRC_CONV_LUMA,
	MCSC_F_YUV_POSTCONV_PC0_CRC_CONV_CHR,
	MCSC_F_YUV_POSTCONV_PC1_CRC_CONV_LUMA,
	MCSC_F_YUV_POSTCONV_PC1_CRC_CONV_CHR,
	MCSC_F_YUV_POSTCONV_PC2_CRC_CONV_LUMA,
	MCSC_F_YUV_POSTCONV_PC2_CRC_CONV_CHR,
	MCSC_F_YUV_POSTCONV_PC3_CRC_CONV_LUMA,
	MCSC_F_YUV_POSTCONV_PC3_CRC_CONV_CHR,
	MCSC_F_YUV_POSTCONV_PC4_CRC_CONV_LUMA,
	MCSC_F_YUV_POSTCONV_PC4_CRC_CONV_CHR,
	MCSC_F_YUV_POSTBCHS_PC0_CRC_BCHS_LUMA,
	MCSC_F_YUV_POSTBCHS_PC0_CRC_BCHS_CHR,
	MCSC_F_YUV_POSTBCHS_PC1_CRC_BCHS_LUMA,
	MCSC_F_YUV_POSTBCHS_PC1_CRC_BCHS_CHR,
	MCSC_F_YUV_POSTBCHS_PC2_CRC_BCHS_LUMA,
	MCSC_F_YUV_POSTBCHS_PC2_CRC_BCHS_CHR,
	MCSC_F_YUV_POSTBCHS_PC3_CRC_BCHS_LUMA,
	MCSC_F_YUV_POSTBCHS_PC3_CRC_BCHS_CHR,
	MCSC_F_YUV_POSTBCHS_PC4_CRC_BCHS_LUMA,
	MCSC_F_YUV_POSTBCHS_PC4_CRC_BCHS_CHR,
	MCSC_F_YUV_WDMASC_W0_CRC_CMN_WDMA_IN_LUMA,
	MCSC_F_YUV_WDMASC_W0_CRC_CMN_WDMA_IN_CHR,
	MCSC_F_YUV_WDMASC_W1_CRC_CMN_WDMA_IN_LUMA,
	MCSC_F_YUV_WDMASC_W1_CRC_CMN_WDMA_IN_CHR,
	MCSC_F_YUV_WDMASC_W2_CRC_CMN_WDMA_IN_LUMA,
	MCSC_F_YUV_WDMASC_W2_CRC_CMN_WDMA_IN_CHR,
	MCSC_F_YUV_WDMASC_W3_CRC_CMN_WDMA_IN_LUMA,
	MCSC_F_YUV_WDMASC_W3_CRC_CMN_WDMA_IN_CHR,
	MCSC_F_YUV_WDMASC_W4_CRC_CMN_WDMA_IN_LUMA,
	MCSC_F_YUV_WDMASC_W4_CRC_CMN_WDMA_IN_CHR,
	MCSC_F_CRC_AXI_RDMA_SFR,
	MCSC_F_DJAG_SIRC_0,
	MCSC_F_DBG_SC_35,
	MCSC_REG_FIELD_CNT
};

struct pmio_field_desc mcsc_field_descs[] = {
	[MCSC_F_CMDQ_ENABLE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_ENABLE, 0, 0),
	[MCSC_F_CMDQ_STOP_CRPT_ENABLE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_STOP_CRPT_ENABLE, 0, 0),
	[MCSC_F_SW_RESET] = PMIO_FIELD_DESC(MCSC_R_SW_RESET, 0, 0),
	[MCSC_F_SW_CORE_RESET] = PMIO_FIELD_DESC(MCSC_R_SW_CORE_RESET, 0, 0),
	[MCSC_F_SW_APB_RESET] = PMIO_FIELD_DESC(MCSC_R_SW_APB_RESET, 0, 0),
	[MCSC_F_TRANS_STOP_REQ] = PMIO_FIELD_DESC(MCSC_R_TRANS_STOP_REQ, 0, 0),
	[MCSC_F_TRANS_STOP_REQ_RDY] = PMIO_FIELD_DESC(MCSC_R_TRANS_STOP_REQ_RDY, 0, 0),
	[MCSC_F_IP_APG_MODE] = PMIO_FIELD_DESC(MCSC_R_IP_APG_MODE, 0, 1),
	[MCSC_F_IP_CLOCK_DOWN_MODE] = PMIO_FIELD_DESC(MCSC_R_IP_CLOCK_DOWN_MODE, 0, 0),
	[MCSC_F_IP_PROCESSING] = PMIO_FIELD_DESC(MCSC_R_IP_PROCESSING, 0, 0),
	[MCSC_F_FORCE_INTERNAL_CLOCK] = PMIO_FIELD_DESC(MCSC_R_FORCE_INTERNAL_CLOCK, 0, 0),
	[MCSC_F_DEBUG_CLOCK_ENABLE] = PMIO_FIELD_DESC(MCSC_R_DEBUG_CLOCK_ENABLE, 0, 0),
	[MCSC_F_IP_POST_FRAME_GAP] = PMIO_FIELD_DESC(MCSC_R_IP_POST_FRAME_GAP, 0, 31),
	[MCSC_F_IP_DRCG_ENABLE] = PMIO_FIELD_DESC(MCSC_R_IP_DRCG_ENABLE, 0, 0),
	[MCSC_F_AUTO_IGNORE_INTERRUPT_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_AUTO_IGNORE_INTERRUPT_ENABLE, 0, 0),
	[MCSC_F_IP_USE_SW_FINISH_COND] = PMIO_FIELD_DESC(MCSC_R_IP_USE_SW_FINISH_COND, 0, 0),
	[MCSC_F_SW_FINISH_COND_ENABLE] = PMIO_FIELD_DESC(MCSC_R_SW_FINISH_COND_ENABLE, 0, 5),
	[MCSC_F_IP_CORRUPTED_COND_ENABLE] = PMIO_FIELD_DESC(MCSC_R_IP_CORRUPTED_COND_ENABLE, 0, 18),
	[MCSC_F_IP_USE_OTF_IN_FOR_PATH_6] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_67, 0, 0),
	[MCSC_F_IP_USE_OTF_IN_FOR_PATH_7] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_67, 8, 8),
	[MCSC_F_IP_USE_OTF_OUT_FOR_PATH_6] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_67, 16, 16),
	[MCSC_F_IP_USE_OTF_OUT_FOR_PATH_7] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_67, 24, 24),
	[MCSC_F_IP_USE_OTF_IN_FOR_PATH_4] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_45, 0, 0),
	[MCSC_F_IP_USE_OTF_IN_FOR_PATH_5] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_45, 8, 8),
	[MCSC_F_IP_USE_OTF_OUT_FOR_PATH_4] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_45, 16, 16),
	[MCSC_F_IP_USE_OTF_OUT_FOR_PATH_5] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_45, 24, 24),
	[MCSC_F_IP_USE_OTF_IN_FOR_PATH_2] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_23, 0, 0),
	[MCSC_F_IP_USE_OTF_IN_FOR_PATH_3] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_23, 8, 8),
	[MCSC_F_IP_USE_OTF_OUT_FOR_PATH_2] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_23, 16, 16),
	[MCSC_F_IP_USE_OTF_OUT_FOR_PATH_3] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_23, 24, 24),
	[MCSC_F_IP_USE_OTF_IN_FOR_PATH_0] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_01, 0, 0),
	[MCSC_F_IP_USE_OTF_IN_FOR_PATH_1] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_01, 8, 8),
	[MCSC_F_IP_USE_OTF_OUT_FOR_PATH_0] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_01, 16, 16),
	[MCSC_F_IP_USE_OTF_OUT_FOR_PATH_1] = PMIO_FIELD_DESC(MCSC_R_IP_USE_OTF_PATH_01, 24, 24),
	[MCSC_F_IP_USE_CINFIFO_NEW_FRAME_IN] =
		PMIO_FIELD_DESC(MCSC_R_IP_USE_CINFIFO_NEW_FRAME_IN, 0, 0),
	[MCSC_F_SECU_CTRL_TZINFO_ICTRL] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_ICTRL, 0, 0),
	[MCSC_F_ICTRL_CSUB_BASE_ADDR] = PMIO_FIELD_DESC(MCSC_R_ICTRL_CSUB_BASE_ADDR, 0, 27),
	[MCSC_F_ICTRL_CSUB_RECV_TURN_OFF_MSG] =
		PMIO_FIELD_DESC(MCSC_R_ICTRL_CSUB_RECV_TURN_OFF_MSG, 0, 0),
	[MCSC_F_ICTRL_CSUB_RECV_IP_INFO_MSG] =
		PMIO_FIELD_DESC(MCSC_R_ICTRL_CSUB_RECV_IP_INFO_MSG, 0, 31),
	[MCSC_F_ICTRL_CSUB_CONNECTION_TEST_MSG] =
		PMIO_FIELD_DESC(MCSC_R_ICTRL_CSUB_CONNECTION_TEST_MSG, 0, 31),
	[MCSC_F_ICTRL_CSUB_MSG_SEND_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_ICTRL_CSUB_MSG_SEND_ENABLE, 0, 0),
	[MCSC_F_ICTRL_CSUB_INT0_EV_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_ICTRL_CSUB_INT0_EV_ENABLE, 0, 31),
	[MCSC_F_ICTRL_CSUB_INT1_EV_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_ICTRL_CSUB_INT1_EV_ENABLE, 0, 30),
	[MCSC_F_ICTRL_CSUB_IP_S_EV_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_ICTRL_CSUB_IP_S_EV_ENABLE, 0, 4),
	[MCSC_F_YUV_IN_IMG_SZ_WIDTH] =
		PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_WIDTH, 0, 15),
	[MCSC_F_YUV_IN_IMG_SZ_HEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_YUVP_IN_IMG_SZ_HEIGHT, 0, 15),
	[MCSC_F_YUV_FRO_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_FRO_EN, 0, 0),
	[MCSC_F_YUV_INPUT_MONO_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_INPUT_MONO_EN, 0, 0),
	[MCSC_F_YUV_CRC_SEED] = PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_CRC_EN, 0, 7),
	[MCSC_F_YUV_MAIN_CTRL_STALL_THROTTLE_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_STALL_THROTTLE_CTRL, 0, 0),
	[MCSC_F_YUV_MAIN_CTRL_DJAG_SC_HBI] =
		PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_DJAG_SC_HBI, 0, 9),
	[MCSC_F_YUV_MAIN_CTRL_AXI_TRAFFIC_WR_SEL_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_AXI_TRAFFIC_WR_SEL, 0, 2),
	[MCSC_F_YUV_MAIN_CTRL_AXI_TRAFFIC_WR_SEL_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_MAIN_CTRL_AXI_TRAFFIC_WR_SEL, 4, 6),
	[MCSC_F_CMDQ_QUE_CMD_BASE_ADDR] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUE_CMD_H, 0, 31),
	[MCSC_F_CMDQ_QUE_CMD_HEADER_NUM] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUE_CMD_M, 0, 11),
	[MCSC_F_CMDQ_QUE_CMD_SETTING_MODE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUE_CMD_M, 12, 13),
	[MCSC_F_CMDQ_QUE_CMD_HOLD_MODE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUE_CMD_M, 14, 15),
	[MCSC_F_CMDQ_QUE_CMD_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUE_CMD_M, 16, 31),
	[MCSC_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUE_CMD_L, 0, 7),
	[MCSC_F_CMDQ_QUE_CMD_FRO_INDEX] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUE_CMD_L, 8, 11),
	[MCSC_F_CMDQ_ADD_TO_QUEUE_0] = PMIO_FIELD_DESC(MCSC_R_CMDQ_ADD_TO_QUEUE_0, 0, 0),
	[MCSC_F_CMDQ_AUTO_CONV_ENABLE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_AUTO_CONV_ENABLE, 0, 0),
	[MCSC_F_CMDQ_POP_LOCK] = PMIO_FIELD_DESC(MCSC_R_CMDQ_LOCK, 0, 0),
	[MCSC_F_CMDQ_RELOAD_LOCK] = PMIO_FIELD_DESC(MCSC_R_CMDQ_LOCK, 8, 8),
	[MCSC_F_CMDQ_CTRL_SETSEL_EN] = PMIO_FIELD_DESC(MCSC_R_CMDQ_CTRL_SETSEL_EN, 0, 0),
	[MCSC_F_CMDQ_SETSEL] = PMIO_FIELD_DESC(MCSC_R_CMDQ_SETSEL, 0, 0),
	[MCSC_F_CMDQ_FLUSH_QUEUE_0] = PMIO_FIELD_DESC(MCSC_R_CMDQ_FLUSH_QUEUE_0, 0, 0),
	[MCSC_F_CMDQ_SWAP_QUEUE_0_TRIG] = PMIO_FIELD_DESC(MCSC_R_CMDQ_SWAP_QUEUE_0, 0, 0),
	[MCSC_F_CMDQ_SWAP_QUEUE_0_POS_A] = PMIO_FIELD_DESC(MCSC_R_CMDQ_SWAP_QUEUE_0, 8, 11),
	[MCSC_F_CMDQ_SWAP_QUEUE_0_POS_B] = PMIO_FIELD_DESC(MCSC_R_CMDQ_SWAP_QUEUE_0, 16, 19),
	[MCSC_F_CMDQ_ROTATE_QUEUE_0_TRIG] = PMIO_FIELD_DESC(MCSC_R_CMDQ_ROTATE_QUEUE_0, 0, 0),
	[MCSC_F_CMDQ_ROTATE_QUEUE_0_POS_S] = PMIO_FIELD_DESC(MCSC_R_CMDQ_ROTATE_QUEUE_0, 8, 11),
	[MCSC_F_CMDQ_ROTATE_QUEUE_0_POS_E] = PMIO_FIELD_DESC(MCSC_R_CMDQ_ROTATE_QUEUE_0, 16, 19),
	[MCSC_F_CMDQ_HM_QUEUE_0_TRIG] = PMIO_FIELD_DESC(MCSC_R_CMDQ_HOLD_MARK_QUEUE_0, 0, 0),
	[MCSC_F_CMDQ_HM_QUEUE_0] = PMIO_FIELD_DESC(MCSC_R_CMDQ_HOLD_MARK_QUEUE_0, 8, 9),
	[MCSC_F_CMDQ_HM_FRAME_ID_QUEUE_0] = PMIO_FIELD_DESC(MCSC_R_CMDQ_HOLD_MARK_QUEUE_0, 16, 31),
	[MCSC_F_CMDQ_CHARGED_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_CMDQ_DEBUG_STATUS_PRE_LOAD, 0, 15),
	[MCSC_F_CMDQ_CHARGED_FOR_NEXT_FRAME] =
		PMIO_FIELD_DESC(MCSC_R_CMDQ_DEBUG_STATUS_PRE_LOAD, 16, 16),
	[MCSC_F_CMDQ_VHD_VBLANK_QRUN_ENABLE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_VHD_CONTROL, 0, 0),
	[MCSC_F_CMDQ_VHD_STALL_ON_QSTOP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_VHD_CONTROL, 24, 24),
	[MCSC_F_CMDQ_FRAME_COUNTER_INC_TYPE] =
		PMIO_FIELD_DESC(MCSC_R_CMDQ_FRAME_COUNTER_INC_TYPE, 0, 0),
	[MCSC_F_CMDQ_FRAME_COUNTER_RESET] = PMIO_FIELD_DESC(MCSC_R_CMDQ_FRAME_COUNTER_RESET, 0, 0),
	[MCSC_F_CMDQ_FRAME_COUNTER] = PMIO_FIELD_DESC(MCSC_R_CMDQ_FRAME_COUNTER, 0, 31),
	[MCSC_F_CMDQ_PRE_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_CMDQ_FRAME_ID, 0, 15),
	[MCSC_F_CMDQ_CURRENT_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_CMDQ_FRAME_ID, 16, 31),
	[MCSC_F_CMDQ_QUEUE_0_FULLNESS] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUEUE_0_INFO, 0, 4),
	[MCSC_F_CMDQ_QUEUE_0_WPTR] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUEUE_0_INFO, 8, 11),
	[MCSC_F_CMDQ_QUEUE_0_RPTR] = PMIO_FIELD_DESC(MCSC_R_CMDQ_QUEUE_0_INFO, 16, 19),
	[MCSC_F_CMDQ_QUEUE_0_RPTR_FOR_DEBUG] =
		PMIO_FIELD_DESC(MCSC_R_CMDQ_QUEUE_0_RPTR_FOR_DEBUG, 0, 3),
	[MCSC_F_CMDQ_DEBUG_QUE_0_CMD_H] = PMIO_FIELD_DESC(MCSC_R_CMDQ_DEBUG_QUE_0_CMD_H, 0, 31),
	[MCSC_F_CMDQ_DEBUG_QUE_0_CMD_M] = PMIO_FIELD_DESC(MCSC_R_CMDQ_DEBUG_QUE_0_CMD_M, 0, 31),
	[MCSC_F_CMDQ_DEBUG_QUE_0_CMD_L] = PMIO_FIELD_DESC(MCSC_R_CMDQ_DEBUG_QUE_0_CMD_L, 0, 11),
	[MCSC_F_CMDQ_DEBUG_PROCESS] = PMIO_FIELD_DESC(MCSC_R_CMDQ_DEBUG_STATUS, 0, 5),
	[MCSC_F_CMDQ_DEBUG_HOLD] = PMIO_FIELD_DESC(MCSC_R_CMDQ_DEBUG_STATUS, 8, 9),
	[MCSC_F_CMDQ_DEBUG_PERIOD] = PMIO_FIELD_DESC(MCSC_R_CMDQ_DEBUG_STATUS, 16, 18),
	[MCSC_F_CMDQ_INT] = PMIO_FIELD_DESC(MCSC_R_CMDQ_INT, 0, 10),
	[MCSC_F_CMDQ_INT_ENABLE] = PMIO_FIELD_DESC(MCSC_R_CMDQ_INT_ENABLE, 0, 10),
	[MCSC_F_CMDQ_INT_STATUS] = PMIO_FIELD_DESC(MCSC_R_CMDQ_INT_STATUS, 0, 10),
	[MCSC_F_CMDQ_INT_CLEAR] = PMIO_FIELD_DESC(MCSC_R_CMDQ_INT_CLEAR, 0, 10),
	[MCSC_F_C_LOADER_ENABLE] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_ENABLE, 0, 0),
	[MCSC_F_C_LOADER_RESET] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_RESET, 0, 0),
	[MCSC_F_C_LOADER_FAST_MODE] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_FAST_MODE, 0, 0),
	[MCSC_F_C_LOADER_REMAP_TO_SHADOW_EN] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_EN, 0, 0),
	[MCSC_F_C_LOADER_REMAP_TO_DIRECT_EN] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_EN, 8, 8),
	[MCSC_F_C_LOADER_REMAP_SETSEL_EN] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_EN, 16, 16),
	[MCSC_F_C_LOADER_ACCESS_INTERVAL] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_ACCESS_INTERVAL, 0, 3),
	[MCSC_F_C_LOADER_REMAP_00_SETA_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_00_ADDR, 0, 15),
	[MCSC_F_C_LOADER_REMAP_00_SETB_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_00_ADDR, 16, 31),
	[MCSC_F_C_LOADER_REMAP_01_SETA_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_01_ADDR, 0, 15),
	[MCSC_F_C_LOADER_REMAP_01_SETB_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_01_ADDR, 16, 31),
	[MCSC_F_C_LOADER_REMAP_02_SETA_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_02_ADDR, 0, 15),
	[MCSC_F_C_LOADER_REMAP_02_SETB_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_02_ADDR, 16, 31),
	[MCSC_F_C_LOADER_REMAP_03_SETA_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_03_ADDR, 0, 15),
	[MCSC_F_C_LOADER_REMAP_03_SETB_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_03_ADDR, 16, 31),
	[MCSC_F_C_LOADER_REMAP_04_SETA_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_04_ADDR, 0, 15),
	[MCSC_F_C_LOADER_REMAP_04_SETB_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_04_ADDR, 16, 31),
	[MCSC_F_C_LOADER_REMAP_05_SETA_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_05_ADDR, 0, 15),
	[MCSC_F_C_LOADER_REMAP_05_SETB_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_05_ADDR, 16, 31),
	[MCSC_F_C_LOADER_REMAP_06_SETA_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_06_ADDR, 0, 15),
	[MCSC_F_C_LOADER_REMAP_06_SETB_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_06_ADDR, 16, 31),
	[MCSC_F_C_LOADER_REMAP_07_SETA_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_07_ADDR, 0, 15),
	[MCSC_F_C_LOADER_REMAP_07_SETB_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_REMAP_07_ADDR, 16, 31),
	[MCSC_F_C_LOADER_LOGICAL_OFFSET_EN] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_LOGICAL_OFFSET_EN, 0, 0),
	[MCSC_F_C_LOADER_LOGICAL_OFFSET_IP] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_LOGICAL_OFFSET, 0, 3),
	[MCSC_F_C_LOADER_LOGICAL_OFFSET_VOTF_R] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_LOGICAL_OFFSET, 8, 11),
	[MCSC_F_C_LOADER_LOGICAL_OFFSET_VOTF_W] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_LOGICAL_OFFSET, 16, 19),
	[MCSC_F_C_LOADER_BUSY] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_DEBUG_STATUS, 0, 0),
	[MCSC_F_C_LOADER_NUM_OF_HEADER_TO_REQ] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_DEBUG_HEADER_REQ_COUNTER, 0, 13),
	[MCSC_F_C_LOADER_NUM_OF_HEADER_REQED] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_DEBUG_HEADER_REQ_COUNTER, 16, 29),
	[MCSC_F_C_LOADER_NUM_OF_HEADER_APBED] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_DEBUG_HEADER_APB_COUNTER, 0, 13),
	[MCSC_F_C_LOADER_NUM_OF_HEADER_SKIPED] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_DEBUG_HEADER_APB_COUNTER, 16, 29),
	[MCSC_F_C_LOADER_HEADER_CRC_SEED] = PMIO_FIELD_DESC(MCSC_R_C_LOADER_HEADER_CRC_SEED, 0, 31),
	[MCSC_F_C_LOADER_PAYLOAD_CRC_SEED] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_PAYLOAD_CRC_SEED, 0, 31),
	[MCSC_F_C_LOADER_HEADER_CRC_RESULT] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_HEADER_CRC_RESULT, 0, 31),
	[MCSC_F_C_LOADER_PAYLOAD_CRC_RESULT] =
		PMIO_FIELD_DESC(MCSC_R_C_LOADER_PAYLOAD_CRC_RESULT, 0, 31),
	[MCSC_F_COREX_ENABLE] = PMIO_FIELD_DESC(MCSC_R_COREX_ENABLE, 0, 0),
	[MCSC_F_COREX_RESET] = PMIO_FIELD_DESC(MCSC_R_COREX_RESET, 0, 0),
	[MCSC_F_COREX_FAST_MODE] = PMIO_FIELD_DESC(MCSC_R_COREX_FAST_MODE, 0, 0),
	[MCSC_F_COREX_UPDATE_TYPE_0] = PMIO_FIELD_DESC(MCSC_R_COREX_UPDATE_TYPE_0, 0, 1),
	[MCSC_F_COREX_UPDATE_TYPE_1] = PMIO_FIELD_DESC(MCSC_R_COREX_UPDATE_TYPE_1, 0, 1),
	[MCSC_F_COREX_UPDATE_MODE_0] = PMIO_FIELD_DESC(MCSC_R_COREX_UPDATE_MODE_0, 0, 0),
	[MCSC_F_COREX_UPDATE_MODE_1] = PMIO_FIELD_DESC(MCSC_R_COREX_UPDATE_MODE_1, 0, 0),
	[MCSC_F_COREX_START_0] = PMIO_FIELD_DESC(MCSC_R_COREX_START_0, 0, 0),
	[MCSC_F_COREX_START_1] = PMIO_FIELD_DESC(MCSC_R_COREX_START_1, 0, 0),
	[MCSC_F_COREX_COPY_FROM_IP_0] = PMIO_FIELD_DESC(MCSC_R_COREX_COPY_FROM_IP_0, 0, 0),
	[MCSC_F_COREX_COPY_FROM_IP_1] = PMIO_FIELD_DESC(MCSC_R_COREX_COPY_FROM_IP_1, 0, 0),
	[MCSC_F_COREX_BUSY_0] = PMIO_FIELD_DESC(MCSC_R_COREX_STATUS_0, 0, 0),
	[MCSC_F_COREX_IP_SET_0] = PMIO_FIELD_DESC(MCSC_R_COREX_STATUS_0, 1, 1),
	[MCSC_F_COREX_BUSY_1] = PMIO_FIELD_DESC(MCSC_R_COREX_STATUS_1, 0, 0),
	[MCSC_F_COREX_IP_SET_1] = PMIO_FIELD_DESC(MCSC_R_COREX_STATUS_1, 1, 1),
	[MCSC_F_COREX_PRE_ADDR_CONFIG] = PMIO_FIELD_DESC(MCSC_R_COREX_PRE_ADDR_CONFIG, 0, 31),
	[MCSC_F_COREX_PRE_DATA_CONFIG] = PMIO_FIELD_DESC(MCSC_R_COREX_PRE_DATA_CONFIG, 0, 31),
	[MCSC_F_COREX_POST_ADDR_CONFIG] = PMIO_FIELD_DESC(MCSC_R_COREX_POST_ADDR_CONFIG, 0, 31),
	[MCSC_F_COREX_POST_DATA_CONFIG] = PMIO_FIELD_DESC(MCSC_R_COREX_POST_DATA_CONFIG, 0, 31),
	[MCSC_F_COREX_PRE_CONFIG_EN] = PMIO_FIELD_DESC(MCSC_R_COREX_PRE_POST_CONFIG_EN, 0, 0),
	[MCSC_F_COREX_POST_CONFIG_EN] = PMIO_FIELD_DESC(MCSC_R_COREX_PRE_POST_CONFIG_EN, 1, 1),
	[MCSC_F_COREX_TYPE_WRITE] = PMIO_FIELD_DESC(MCSC_R_COREX_TYPE_WRITE, 0, 31),
	[MCSC_F_COREX_TYPE_WRITE_TRIGGER] = PMIO_FIELD_DESC(MCSC_R_COREX_TYPE_WRITE_TRIGGER, 0, 0),
	[MCSC_F_COREX_TYPE_READ] = PMIO_FIELD_DESC(MCSC_R_COREX_TYPE_READ, 0, 31),
	[MCSC_F_COREX_TYPE_READ_OFFSET] = PMIO_FIELD_DESC(MCSC_R_COREX_TYPE_READ_OFFSET, 0, 8),
	[MCSC_F_COREX_INT] = PMIO_FIELD_DESC(MCSC_R_COREX_INT, 0, 8),
	[MCSC_F_COREX_INT_STATUS] = PMIO_FIELD_DESC(MCSC_R_COREX_INT_STATUS, 0, 8),
	[MCSC_F_COREX_INT_CLEAR] = PMIO_FIELD_DESC(MCSC_R_COREX_INT_CLEAR, 0, 8),
	[MCSC_F_COREX_INT_ENABLE] = PMIO_FIELD_DESC(MCSC_R_COREX_INT_ENABLE, 0, 8),
	[MCSC_F_INT_REQ_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_REQ_INT0, 0, 31),
	[MCSC_F_INT_REQ_INT0_ENABLE] = PMIO_FIELD_DESC(MCSC_R_INT_REQ_INT0_ENABLE, 0, 31),
	[MCSC_F_INT_REQ_INT0_STATUS] = PMIO_FIELD_DESC(MCSC_R_INT_REQ_INT0_STATUS, 0, 31),
	[MCSC_F_INT_REQ_INT0_CLEAR] = PMIO_FIELD_DESC(MCSC_R_INT_REQ_INT0_CLEAR, 0, 31),
	[MCSC_F_INT_REQ_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_REQ_INT1, 0, 31),
	[MCSC_F_INT_REQ_INT1_ENABLE] = PMIO_FIELD_DESC(MCSC_R_INT_REQ_INT1_ENABLE, 0, 31),
	[MCSC_F_INT_REQ_INT1_STATUS] = PMIO_FIELD_DESC(MCSC_R_INT_REQ_INT1_STATUS, 0, 31),
	[MCSC_F_INT_REQ_INT1_CLEAR] = PMIO_FIELD_DESC(MCSC_R_INT_REQ_INT1_CLEAR, 0, 31),
	[MCSC_F_INT_HIST_CURINT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_CURINT0, 0, 31),
	[MCSC_F_INT_HIST_CURINT0_ENABLE] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_CURINT0_ENABLE, 0, 31),
	[MCSC_F_INT_HIST_CURINT0_STATUS] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_CURINT0_STATUS, 0, 31),
	[MCSC_F_INT_HIST_CURINT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_CURINT1, 0, 31),
	[MCSC_F_INT_HIST_CURINT1_ENABLE] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_CURINT1_ENABLE, 0, 31),
	[MCSC_F_INT_HIST_CURINT1_STATUS] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_CURINT1_STATUS, 0, 31),
	[MCSC_F_INT_HIST_00_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_00_FRAME_ID, 0, 15),
	[MCSC_F_INT_HIST_00_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_00_INT0, 0, 31),
	[MCSC_F_INT_HIST_00_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_00_INT1, 0, 31),
	[MCSC_F_INT_HIST_01_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_01_FRAME_ID, 0, 15),
	[MCSC_F_INT_HIST_01_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_01_INT0, 0, 31),
	[MCSC_F_INT_HIST_01_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_01_INT1, 0, 31),
	[MCSC_F_INT_HIST_02_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_02_FRAME_ID, 0, 15),
	[MCSC_F_INT_HIST_02_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_02_INT0, 0, 31),
	[MCSC_F_INT_HIST_02_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_02_INT1, 0, 31),
	[MCSC_F_INT_HIST_03_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_03_FRAME_ID, 0, 15),
	[MCSC_F_INT_HIST_03_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_03_INT0, 0, 31),
	[MCSC_F_INT_HIST_03_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_03_INT1, 0, 31),
	[MCSC_F_INT_HIST_04_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_04_FRAME_ID, 0, 15),
	[MCSC_F_INT_HIST_04_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_04_INT0, 0, 31),
	[MCSC_F_INT_HIST_04_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_04_INT1, 0, 31),
	[MCSC_F_INT_HIST_05_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_05_FRAME_ID, 0, 15),
	[MCSC_F_INT_HIST_05_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_05_INT0, 0, 31),
	[MCSC_F_INT_HIST_05_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_05_INT1, 0, 31),
	[MCSC_F_INT_HIST_06_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_06_FRAME_ID, 0, 15),
	[MCSC_F_INT_HIST_06_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_06_INT0, 0, 31),
	[MCSC_F_INT_HIST_06_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_06_INT1, 0, 31),
	[MCSC_F_INT_HIST_07_FRAME_ID] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_07_FRAME_ID, 0, 15),
	[MCSC_F_INT_HIST_07_INT0] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_07_INT0, 0, 31),
	[MCSC_F_INT_HIST_07_INT1] = PMIO_FIELD_DESC(MCSC_R_INT_HIST_07_INT1, 0, 31),
	[MCSC_F_SECU_CTRL_SEQID] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_SEQID, 0, 2),
	[MCSC_F_SECU_CTRL_TZINFO_SEQID_0] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_SEQID_0, 0, 31),
	[MCSC_F_SECU_CTRL_TZINFO_SEQID_1] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_SEQID_1, 0, 31),
	[MCSC_F_SECU_CTRL_TZINFO_SEQID_2] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_SEQID_2, 0, 31),
	[MCSC_F_SECU_CTRL_TZINFO_SEQID_3] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_SEQID_3, 0, 31),
	[MCSC_F_SECU_CTRL_TZINFO_SEQID_4] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_SEQID_4, 0, 31),
	[MCSC_F_SECU_CTRL_TZINFO_SEQID_5] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_SEQID_5, 0, 31),
	[MCSC_F_SECU_CTRL_TZINFO_SEQID_6] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_SEQID_6, 0, 31),
	[MCSC_F_SECU_CTRL_TZINFO_SEQID_7] = PMIO_FIELD_DESC(MCSC_R_SECU_CTRL_TZINFO_SEQID_7, 0, 31),
	[MCSC_F_SECU_OTF_SEQ_ID_PROT_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_SECU_OTF_SEQ_ID_PROT_ENABLE, 0, 0),
	[MCSC_F_PERF_MONITOR_ENABLE] = PMIO_FIELD_DESC(MCSC_R_PERF_MONITOR_ENABLE, 0, 4),
	[MCSC_F_PERF_MONITOR_CLEAR] = PMIO_FIELD_DESC(MCSC_R_PERF_MONITOR_CLEAR, 0, 4),
	[MCSC_F_PERF_MONITOR_INT_USER_SEL] =
		PMIO_FIELD_DESC(MCSC_R_PERF_MONITOR_INT_USER_SEL, 0, 5),
	[MCSC_F_PERF_MONITOR_INT_START] = PMIO_FIELD_DESC(MCSC_R_PERF_MONITOR_INT_START, 0, 31),
	[MCSC_F_PERF_MONITOR_INT_END] = PMIO_FIELD_DESC(MCSC_R_PERF_MONITOR_INT_END, 0, 31),
	[MCSC_F_PERF_MONITOR_INT_USER] = PMIO_FIELD_DESC(MCSC_R_PERF_MONITOR_INT_USER, 0, 31),
	[MCSC_F_PERF_MONITOR_PROCESS_PRE_CONFIG] =
		PMIO_FIELD_DESC(MCSC_R_PERF_MONITOR_PROCESS_PRE_CONFIG, 0, 31),
	[MCSC_F_PERF_MONITOR_PROCESS_FRAME] =
		PMIO_FIELD_DESC(MCSC_R_PERF_MONITOR_PROCESS_FRAME, 0, 31),
	[MCSC_F_IP_MICRO] = PMIO_FIELD_DESC(MCSC_R_IP_VERSION, 0, 15),
	[MCSC_F_IP_MINOR] = PMIO_FIELD_DESC(MCSC_R_IP_VERSION, 16, 23),
	[MCSC_F_IP_MAJOR] = PMIO_FIELD_DESC(MCSC_R_IP_VERSION, 24, 31),
	[MCSC_F_CTRL_MICRO] = PMIO_FIELD_DESC(MCSC_R_COMMON_CTRL_VERSION, 0, 15),
	[MCSC_F_CTRL_MINOR] = PMIO_FIELD_DESC(MCSC_R_COMMON_CTRL_VERSION, 16, 23),
	[MCSC_F_CTRL_MAJOR] = PMIO_FIELD_DESC(MCSC_R_COMMON_CTRL_VERSION, 24, 31),
	[MCSC_F_QCH_STATUS] = PMIO_FIELD_DESC(MCSC_R_QCH_STATUS, 0, 3),
	[MCSC_F_IDLENESS_STATUS] = PMIO_FIELD_DESC(MCSC_R_IDLENESS_STATUS, 0, 0),
	[MCSC_F_CHAIN_IDLENESS_STATUS] = PMIO_FIELD_DESC(MCSC_R_IDLENESS_STATUS, 16, 16),
	[MCSC_F_DEBUG_COUNTER_0_SIG_SEL] = PMIO_FIELD_DESC(MCSC_R_DEBUG_COUNTER_SIG_SEL, 0, 7),
	[MCSC_F_DEBUG_COUNTER_1_SIG_SEL] = PMIO_FIELD_DESC(MCSC_R_DEBUG_COUNTER_SIG_SEL, 8, 15),
	[MCSC_F_DEBUG_COUNTER_2_SIG_SEL] = PMIO_FIELD_DESC(MCSC_R_DEBUG_COUNTER_SIG_SEL, 16, 23),
	[MCSC_F_DEBUG_COUNTER_3_SIG_SEL] = PMIO_FIELD_DESC(MCSC_R_DEBUG_COUNTER_SIG_SEL, 24, 31),
	[MCSC_F_DEBUG_COUNTER_0] = PMIO_FIELD_DESC(MCSC_R_DEBUG_COUNTER_0, 0, 31),
	[MCSC_F_DEBUG_COUNTER_1] = PMIO_FIELD_DESC(MCSC_R_DEBUG_COUNTER_1, 0, 31),
	[MCSC_F_DEBUG_COUNTER_2] = PMIO_FIELD_DESC(MCSC_R_DEBUG_COUNTER_2, 0, 31),
	[MCSC_F_DEBUG_COUNTER_3] = PMIO_FIELD_DESC(MCSC_R_DEBUG_COUNTER_3, 0, 31),
	[MCSC_F_IP_BUSY_MONITOR_0] = PMIO_FIELD_DESC(MCSC_R_IP_BUSY_MONITOR_0, 0, 31),
	[MCSC_F_IP_BUSY_MONITOR_1] = PMIO_FIELD_DESC(MCSC_R_IP_BUSY_MONITOR_1, 0, 31),
	[MCSC_F_IP_BUSY_MONITOR_2] = PMIO_FIELD_DESC(MCSC_R_IP_BUSY_MONITOR_2, 0, 31),
	[MCSC_F_IP_BUSY_MONITOR_3] = PMIO_FIELD_DESC(MCSC_R_IP_BUSY_MONITOR_3, 0, 31),
	[MCSC_F_IP_STALL_OUT_STATUS_0] = PMIO_FIELD_DESC(MCSC_R_IP_STALL_OUT_STATUS_0, 0, 31),
	[MCSC_F_IP_STALL_OUT_STATUS_1] = PMIO_FIELD_DESC(MCSC_R_IP_STALL_OUT_STATUS_1, 0, 31),
	[MCSC_F_IP_STALL_OUT_STATUS_2] = PMIO_FIELD_DESC(MCSC_R_IP_STALL_OUT_STATUS_2, 0, 31),
	[MCSC_F_IP_STALL_OUT_STATUS_3] = PMIO_FIELD_DESC(MCSC_R_IP_STALL_OUT_STATUS_3, 0, 31),
	[MCSC_F_STOPEN_CRC_STOP_VALID_COUNT] =
		PMIO_FIELD_DESC(MCSC_R_STOPEN_CRC_STOP_VALID_COUNT, 0, 27),
	[MCSC_F_SFR_ACCESS_LOG_ENABLE] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_ENABLE, 0, 0),
	[MCSC_F_SFR_ACCESS_LOG_CLEAR] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_CLEAR, 0, 0),
	[MCSC_F_SFR_ACCESS_LOG_0_ADJUST_RANGE] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_0, 0, 7),
	[MCSC_F_SFR_ACCESS_LOG_0] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_0, 8, 31),
	[MCSC_F_SFR_ACCESS_LOG_0_ADDRESS] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_0_ADDRESS, 0, 31),
	[MCSC_F_SFR_ACCESS_LOG_1_ADJUST_RANGE] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_1, 0, 7),
	[MCSC_F_SFR_ACCESS_LOG_1] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_1, 8, 31),
	[MCSC_F_SFR_ACCESS_LOG_1_ADDRESS] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_1_ADDRESS, 0, 31),
	[MCSC_F_SFR_ACCESS_LOG_2_ADJUST_RANGE] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_2, 0, 7),
	[MCSC_F_SFR_ACCESS_LOG_2] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_2, 8, 31),
	[MCSC_F_SFR_ACCESS_LOG_2_ADDRESS] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_2_ADDRESS, 0, 31),
	[MCSC_F_SFR_ACCESS_LOG_3_ADJUST_RANGE] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_3, 0, 7),
	[MCSC_F_SFR_ACCESS_LOG_3] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_3, 8, 31),
	[MCSC_F_SFR_ACCESS_LOG_3_ADDRESS] = PMIO_FIELD_DESC(MCSC_R_SFR_ACCESS_LOG_3_ADDRESS, 0, 31),
	[MCSC_F_IP_ROL_RESET] = PMIO_FIELD_DESC(MCSC_R_IP_ROL_RESET, 0, 0),
	[MCSC_F_IP_ROL_MODE] = PMIO_FIELD_DESC(MCSC_R_IP_ROL_MODE, 0, 1),
	[MCSC_F_IP_ROL_SELECT] = PMIO_FIELD_DESC(MCSC_R_IP_ROL_SELECT, 0, 18),
	[MCSC_F_IP_INT_SRC_SEL] = PMIO_FIELD_DESC(MCSC_R_IP_INT_ON_COL_ROW, 0, 1),
	[MCSC_F_IP_INT_COL_EN] = PMIO_FIELD_DESC(MCSC_R_IP_INT_ON_COL_ROW, 8, 8),
	[MCSC_F_IP_INT_ROW_EN] = PMIO_FIELD_DESC(MCSC_R_IP_INT_ON_COL_ROW, 16, 16),
	[MCSC_F_IP_INT_COL_POS] = PMIO_FIELD_DESC(MCSC_R_IP_INT_ON_COL_ROW_POS, 0, 15),
	[MCSC_F_IP_INT_ROW_POS] = PMIO_FIELD_DESC(MCSC_R_IP_INT_ON_COL_ROW_POS, 16, 31),
	[MCSC_F_FREEZE_FOR_DEBUG] = PMIO_FIELD_DESC(MCSC_R_FREEZE_FOR_DEBUG, 0, 0),
	[MCSC_F_FREEZE_BY_SFR_ENABLE] = PMIO_FIELD_DESC(MCSC_R_FREEZE_EXTENSION_ENABLE, 0, 0),
	[MCSC_F_FREEZE_BY_INT1_ENABLE] = PMIO_FIELD_DESC(MCSC_R_FREEZE_EXTENSION_ENABLE, 8, 8),
	[MCSC_F_FREEZE_BY_INT0_ENABLE] = PMIO_FIELD_DESC(MCSC_R_FREEZE_EXTENSION_ENABLE, 16, 16),
	[MCSC_F_FREEZE_SRC_SEL] = PMIO_FIELD_DESC(MCSC_R_FREEZE_EN, 0, 1),
	[MCSC_F_FREEZE_EN] = PMIO_FIELD_DESC(MCSC_R_FREEZE_EN, 8, 8),
	[MCSC_F_FREEZE_COL_POS] = PMIO_FIELD_DESC(MCSC_R_FREEZE_COL_ROW_POS, 0, 15),
	[MCSC_F_FREEZE_ROW_POS] = PMIO_FIELD_DESC(MCSC_R_FREEZE_COL_ROW_POS, 16, 31),
	[MCSC_F_FREEZE_CORRUPTED_ENABLE] = PMIO_FIELD_DESC(MCSC_R_FREEZE_CORRUPTED_ENABLE, 0, 0),
	[MCSC_F_YUV_CINFIFO_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_ENABLE, 0, 0),
	[MCSC_F_YUV_CINFIFO_STALL_BEFORE_FRAME_START_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CONFIG, 0, 0),
	[MCSC_F_YUV_CINFIFO_AUTO_RECOVERY_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CONFIG, 1, 1),
	[MCSC_F_YUV_CINFIFO_ROL_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CONFIG, 2, 2),
	[MCSC_F_YUV_CINFIFO_ROL_RESET_ON_FRAME_START] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CONFIG, 3, 3),
	[MCSC_F_YUV_CINFIFO_DEBUG_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CONFIG, 4, 4),
	[MCSC_F_YUV_CINFIFO_STRGEN_MODE_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CONFIG, 5, 5),
	[MCSC_F_YUV_CINFIFO_STRGEN_MODE_DATA_TYPE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CONFIG, 6, 6),
	[MCSC_F_YUV_CINFIFO_STRGEN_MODE_DATA] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CONFIG, 16, 31),
	[MCSC_F_YUV_CINFIFO_STALL_THROTTLE_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_STALL_CTRL, 0, 0),
	[MCSC_F_YUV_CINFIFO_INTERVAL_VBLANK] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INTERVAL_VBLANK, 0, 31),
	[MCSC_F_YUV_CINFIFO_INTERVAL_HBLANK] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INTERVALS, 0, 15),
	[MCSC_F_YUV_CINFIFO_INTERVAL_PIXEL] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INTERVALS, 16, 31),
	[MCSC_F_YUV_CINFIFO_OP_STATE_MONITOR] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_STATUS, 0, 8),
	[MCSC_F_YUV_CINFIFO_ERROR_STATE_MONITOR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_STATUS, 16, 24),
	[MCSC_F_YUV_CINFIFO_COL_CNT] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INPUT_CNT, 0, 15),
	[MCSC_F_YUV_CINFIFO_ROW_CNT] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INPUT_CNT, 16, 31),
	[MCSC_F_YUV_CINFIFO_STALL_CNT] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_STALL_CNT, 0, 31),
	[MCSC_F_YUV_CINFIFO_FIFO_FULLNESS] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_FIFO_FULLNESS, 0, 4),
	[MCSC_F_YUV_CINFIFO_INT] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INT, 0, 3),
	[MCSC_F_YUV_CINFIFO_INT_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INT_ENABLE, 0, 3),
	[MCSC_F_YUV_CINFIFO_INT_STATUS] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INT_STATUS, 0, 3),
	[MCSC_F_YUV_CINFIFO_INT_CLEAR] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INT_CLEAR, 0, 3),
	[MCSC_F_YUV_CINFIFO_CORRUPTED_COND_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_CORRUPTED_COND_ENABLE, 0, 3),
	[MCSC_F_YUV_CINFIFO_ROL_SELECT] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_ROL_SELECT, 0, 3),
	[MCSC_F_CINFIFO_INTERVAL_VBLANK_AR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INTERVAL_VBLANK_AR, 0, 31),
	[MCSC_F_CINFIFO_INTERVAL_HBLANK_AR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_INTERVAL_HBLANK_AR, 0, 15),
	[MCSC_F_YUV_CINFIFO_CRC_SEED] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_STREAM_CRC, 0, 7),
	[MCSC_F_YUV_CINFIFO_CRC_RESULT] = PMIO_FIELD_DESC(MCSC_R_YUV_CINFIFO_STREAM_CRC, 8, 15),
	[MCSC_F_STAT_CINFIFODUALLAYER_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_ENABLE, 0, 0),
	[MCSC_F_STAT_CINFIFODUALLAYER_STALL_BEFORE_FRAME_START_EN] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CONFIG, 0, 0),
	[MCSC_F_STAT_CINFIFODUALLAYER_AUTO_RECOVERY_EN] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CONFIG, 1, 1),
	[MCSC_F_STAT_CINFIFODUALLAYER_ROL_EN] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CONFIG, 2, 2),
	[MCSC_F_STAT_CINFIFODUALLAYER_ROL_RESET_ON_FRAME_START] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CONFIG, 3, 3),
	[MCSC_F_STAT_CINFIFODUALLAYER_DEBUG_EN] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CONFIG, 4, 4),
	[MCSC_F_STAT_CINFIFODUALLAYER_STRGEN_MODE_EN] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CONFIG, 5, 5),
	[MCSC_F_STAT_CINFIFODUALLAYER_STRGEN_MODE_DATA_TYPE] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CONFIG, 6, 6),
	[MCSC_F_STAT_CINFIFODUALLAYER_STRGEN_MODE_DATA] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CONFIG, 16, 31),
	[MCSC_F_STAT_CINFIFODUALLAYER_STALL_THROTTLE_EN] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_STALL_CTRL, 0, 0),
	[MCSC_F_STAT_CINFIFODUALLAYER_INTERVAL_VBLANK] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INTERVAL_VBLANK, 0, 31),
	[MCSC_F_STAT_CINFIFODUALLAYER_INTERVAL_HBLANK] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INTERVALS, 0, 15),
	[MCSC_F_STAT_CINFIFODUALLAYER_INTERVAL_PIXEL] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INTERVALS, 16, 31),
	[MCSC_F_STAT_CINFIFODUALLAYER_OP_STATE_MONITOR] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_STATUS, 0, 8),
	[MCSC_F_STAT_CINFIFODUALLAYER_ERROR_STATE_MONITOR] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_STATUS, 16, 24),
	[MCSC_F_STAT_CINFIFODUALLAYER_COL_CNT] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INPUT_CNT, 0, 15),
	[MCSC_F_STAT_CINFIFODUALLAYER_ROW_CNT] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INPUT_CNT, 16, 31),
	[MCSC_F_STAT_CINFIFODUALLAYER_STALL_CNT] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_STALL_CNT, 0, 31),
	[MCSC_F_STAT_CINFIFODUALLAYER_FIFO_FULLNESS] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_FIFO_FULLNESS, 0, 4),
	[MCSC_F_STAT_CINFIFODUALLAYER_INT] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INT, 0, 3),
	[MCSC_F_STAT_CINFIFODUALLAYER_INT_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INT_ENABLE, 0, 3),
	[MCSC_F_STAT_CINFIFODUALLAYER_INT_STATUS] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INT_STATUS, 0, 3),
	[MCSC_F_STAT_CINFIFODUALLAYER_INT_CLEAR] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INT_CLEAR, 0, 3),
	[MCSC_F_STAT_CINFIFODUALLAYER_CORRUPTED_COND_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_CORRUPTED_COND_ENABLE, 0, 3),
	[MCSC_F_STAT_CINFIFODUALLAYER_ROL_SELECT] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_ROL_SELECT, 0, 3),
	[MCSC_F_CINFIFODUALLAYER_INTERVAL_VBLANK_AR] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INTERVAL_VBLANK_AR, 0, 31),
	[MCSC_F_CINFIFODUALLAYER_INTERVAL_HBLANK_AR] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_INTERVAL_HBLANK_AR, 0, 15),
	[MCSC_F_STAT_CINFIFODUALLAYER_CRC_SEED] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_STREAM_CRC, 0, 7),
	[MCSC_F_STAT_CINFIFODUALLAYER_CRC_RESULT] =
		PMIO_FIELD_DESC(MCSC_R_STAT_CINFIFODUALLAYER_STREAM_CRC, 8, 15),
	[MCSC_F_STAT_RDMACL_EN] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_ENABLE, 0, 0),
	[MCSC_F_STAT_RDMACL_CLK_ALWAYS_ON_EN] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_ENABLE, 1, 1),
	[MCSC_F_STAT_RDMACL_SBWC_EN] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_COMP_CTRL, 0, 1),
	[MCSC_F_STAT_RDMACL_AFBC_EN] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_COMP_CTRL, 2, 2),
	[MCSC_F_STAT_RDMACL_SBWC_64B_ALIGN] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_COMP_CTRL, 3, 3),
	[MCSC_F_STAT_RDMACL_SBWC_TERACELL_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_COMP_CTRL, 4, 4),
	[MCSC_F_STAT_RDMACL_SBWC_128B_ALIGN] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_COMP_CTRL, 6, 6),
	[MCSC_F_STAT_RDMACL_DATA_FORMAT_BAYER] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_DATA_FORMAT, 0, 4),
	[MCSC_F_STAT_RDMACL_DATA_FORMAT_YUV] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_DATA_FORMAT, 8, 13),
	[MCSC_F_STAT_RDMACL_DATA_FORMAT_RGB] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_DATA_FORMAT, 16, 19),
	[MCSC_F_STAT_RDMACL_DATA_FORMAT_MSBALIGN] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_DATA_FORMAT, 21, 21),
	[MCSC_F_STAT_RDMACL_DATA_FORMAT_MSBALIGN_UNSIGN] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_DATA_FORMAT, 22, 22),
	[MCSC_F_STAT_RDMACL_MONO_MODE] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_MONO_MODE, 0, 0),
	[MCSC_F_STAT_RDMACL_WIDTH] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_WIDTH, 0, 16),
	[MCSC_F_STAT_RDMACL_HEIGHT] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_HEIGHT, 0, 13),
	[MCSC_F_STAT_RDMACL_STRIDE_1P] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_STRIDE_1P, 0, 18),
	[MCSC_F_STAT_RDMACL_MAX_MO] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_MAX_MO, 0, 8),
	[MCSC_F_STAT_RDMACL_QURGENT_MO] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_MAX_MO, 16, 24),
	[MCSC_F_STAT_RDMACL_LINE_GAP] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_LINE_GAP, 0, 15),
	[MCSC_F_STAT_RDMACL_BUSINFO] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_BUSINFO, 0, 10),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0, 0, 31),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1, 0, 31),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2, 0, 31),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3, 0, 31),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4, 0, 31),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5, 0, 31),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6, 0, 31),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7, 0, 31),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0_LSB_4B] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO0_LSB_4B, 0, 3),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1_LSB_4B] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO1_LSB_4B, 0, 3),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2_LSB_4B] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO2_LSB_4B, 0, 3),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3_LSB_4B] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO3_LSB_4B, 0, 3),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4_LSB_4B] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO4_LSB_4B, 0, 3),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5_LSB_4B] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO5_LSB_4B, 0, 3),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6_LSB_4B] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO6_LSB_4B, 0, 3),
	[MCSC_F_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7_LSB_4B] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7_LSB_4B, 0, 3),
	[MCSC_F_STAT_RDMACL_IMG_CRC_1P] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_IMG_CRC_1P, 0, 31),
	[MCSC_F_STAT_RDMACL_MON_STATUS_0] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_MON_STATUS_0, 0, 31),
	[MCSC_F_STAT_RDMACL_MON_STATUS_1] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_MON_STATUS_1, 0, 31),
	[MCSC_F_STAT_RDMACL_MON_STATUS_2] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_MON_STATUS_2, 0, 31),
	[MCSC_F_STAT_RDMACL_MON_STATUS_3] = PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_MON_STATUS_3, 0, 31),
	[MCSC_F_STAT_RDMACL_DEBUG_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_STAT_RDMACL_AXI_DEBUG_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W0_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_ENABLE, 0, 0),
	[MCSC_F_YUV_WDMASC_W0_CLK_ALWAYS_ON_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_ENABLE, 1, 1),
	[MCSC_F_YUV_WDMASC_W0_SBWC_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_COMP_CTRL, 0, 1),
	[MCSC_F_YUV_WDMASC_W0_AFBC_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_COMP_CTRL, 2, 2),
	[MCSC_F_YUV_WDMASC_W0_SBWC_64B_ALIGN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_COMP_CTRL, 3, 3),
	[MCSC_F_YUV_WDMASC_W0_SBWC_TERACELL_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_COMP_CTRL, 4, 4),
	[MCSC_F_YUV_WDMASC_W0_SBWC_128B_ALIGN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_COMP_CTRL, 6, 6),
	[MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_BAYER] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DATA_FORMAT, 0, 4),
	[MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_YUV] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DATA_FORMAT, 8, 13),
	[MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_RGB] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DATA_FORMAT, 16, 19),
	[MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_4B_SWAP] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DATA_FORMAT, 24, 24),
	[MCSC_F_YUV_WDMASC_W0_DATA_FORMAT_TYPE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DATA_FORMAT, 31, 31),
	[MCSC_F_YUV_WDMASC_W0_MONO_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_MONO_CTRL, 0, 0),
	[MCSC_F_YUV_WDMASC_W0_COMP_LOSSY_QUALITY_CONTROL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_COMP_LOSSY_QUALITY_CONTROL, 0, 1),
	[MCSC_F_YUV_WDMASC_W0_WIDTH] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_WIDTH, 0, 16),
	[MCSC_F_YUV_WDMASC_W0_HEIGHT] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEIGHT, 0, 13),
	[MCSC_F_YUV_WDMASC_W0_STRIDE_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_STRIDE_1P, 0, 18),
	[MCSC_F_YUV_WDMASC_W0_STRIDE_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_STRIDE_2P, 0, 18),
	[MCSC_F_YUV_WDMASC_W0_STRIDE_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_STRIDE_3P, 0, 18),
	[MCSC_F_YUV_WDMASC_W0_STRIDE_HEADER_1P] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_STRIDE_HEADER_1P, 0, 8),
	[MCSC_F_YUV_WDMASC_W0_STRIDE_HEADER_2P] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_STRIDE_HEADER_2P, 0, 8),
	[MCSC_F_YUV_WDMASC_W0_VOTF_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_VOTF_EN, 0, 1),
	[MCSC_F_YUV_WDMASC_W0_MAX_MO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_MAX_MO, 0, 8),
	[MCSC_F_YUV_WDMASC_W0_BUSINFO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_BUSINFO, 0, 10),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W0_IMG_CRC_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_CRC_1P, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_CRC_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_CRC_2P, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_IMG_CRC_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_IMG_CRC_3P, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_CRC_1P] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_CRC_1P, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_HEADER_CRC_2P] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_HEADER_CRC_2P, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_MON_STATUS_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_MON_STATUS_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_MON_STATUS_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_MON_STATUS_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_MON_STATUS_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_MON_STATUS_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_MON_STATUS_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_MON_STATUS_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_BW_LIMIT_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_BW_LIMIT_0, 0, 0),
	[MCSC_F_YUV_WDMASC_W0_BW_LIMIT_FREQ_NUM_CYCLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_BW_LIMIT_0, 16, 27),
	[MCSC_F_YUV_WDMASC_W0_BW_LIMIT_AVG_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_BW_LIMIT_1, 0, 15),
	[MCSC_F_YUV_WDMASC_W0_BW_LIMIT_SLOT_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_BW_LIMIT_1, 16, 31),
	[MCSC_F_YUV_WDMASC_W0_BW_LIMIT_COMPENSATION_PERIOD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_BW_LIMIT_2, 0, 11),
	[MCSC_F_YUV_WDMASC_W0_BW_LIMIT_COMPENSATION_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_BW_LIMIT_2, 16, 31),
	[MCSC_F_YUV_WDMASC_W0_CACHE_WLU_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_CACHE_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W0_CACHE_32B_PARTIAL_ALLOC_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_CACHE_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W0_DEBUG_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DEBUG_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W0_DEBUG_CAPTURE_ONCE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DEBUG_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W0_DEBUG_CLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DEBUG_CONTROL, 16, 31),
	[MCSC_F_YUV_WDMASC_W0_DEBUG_AXI_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DEBUG_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_DEBUG_AXI_MO_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DEBUG_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_DEBUG_AXI_AW_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DEBUG_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W0_FLIP_CONTROL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_FLIP_CONTROL, 0, 1),
	[MCSC_F_YUV_WDMASC_W0_RGB_ALPHA] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_RGB_ALPHA, 0, 7),
	[MCSC_F_YUV_WDMASC_W1_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_ENABLE, 0, 0),
	[MCSC_F_YUV_WDMASC_W1_CLK_ALWAYS_ON_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_ENABLE, 1, 1),
	[MCSC_F_YUV_WDMASC_W1_SBWC_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_COMP_CTRL, 0, 1),
	[MCSC_F_YUV_WDMASC_W1_AFBC_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_COMP_CTRL, 2, 2),
	[MCSC_F_YUV_WDMASC_W1_SBWC_64B_ALIGN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_COMP_CTRL, 3, 3),
	[MCSC_F_YUV_WDMASC_W1_SBWC_TERACELL_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_COMP_CTRL, 4, 4),
	[MCSC_F_YUV_WDMASC_W1_SBWC_128B_ALIGN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_COMP_CTRL, 6, 6),
	[MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_BAYER] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DATA_FORMAT, 0, 4),
	[MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_YUV] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DATA_FORMAT, 8, 13),
	[MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_RGB] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DATA_FORMAT, 16, 19),
	[MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_4B_SWAP] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DATA_FORMAT, 24, 24),
	[MCSC_F_YUV_WDMASC_W1_DATA_FORMAT_TYPE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DATA_FORMAT, 31, 31),
	[MCSC_F_YUV_WDMASC_W1_MONO_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_MONO_CTRL, 0, 0),
	[MCSC_F_YUV_WDMASC_W1_COMP_LOSSY_QUALITY_CONTROL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_COMP_LOSSY_QUALITY_CONTROL, 0, 1),
	[MCSC_F_YUV_WDMASC_W1_WIDTH] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_WIDTH, 0, 16),
	[MCSC_F_YUV_WDMASC_W1_HEIGHT] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEIGHT, 0, 13),
	[MCSC_F_YUV_WDMASC_W1_STRIDE_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_STRIDE_1P, 0, 18),
	[MCSC_F_YUV_WDMASC_W1_STRIDE_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_STRIDE_2P, 0, 18),
	[MCSC_F_YUV_WDMASC_W1_STRIDE_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_STRIDE_3P, 0, 18),
	[MCSC_F_YUV_WDMASC_W1_STRIDE_HEADER_1P] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_STRIDE_HEADER_1P, 0, 8),
	[MCSC_F_YUV_WDMASC_W1_STRIDE_HEADER_2P] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_STRIDE_HEADER_2P, 0, 8),
	[MCSC_F_YUV_WDMASC_W1_VOTF_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_VOTF_EN, 0, 1),
	[MCSC_F_YUV_WDMASC_W1_MAX_MO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_MAX_MO, 0, 8),
	[MCSC_F_YUV_WDMASC_W1_BUSINFO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_BUSINFO, 0, 10),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_1P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W1_IMG_CRC_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_CRC_1P, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_CRC_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_CRC_2P, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_IMG_CRC_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_IMG_CRC_3P, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_CRC_1P] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_CRC_1P, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_HEADER_CRC_2P] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_HEADER_CRC_2P, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_MON_STATUS_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_MON_STATUS_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_MON_STATUS_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_MON_STATUS_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_MON_STATUS_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_MON_STATUS_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_MON_STATUS_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_MON_STATUS_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_BW_LIMIT_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_BW_LIMIT_0, 0, 0),
	[MCSC_F_YUV_WDMASC_W1_BW_LIMIT_FREQ_NUM_CYCLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_BW_LIMIT_0, 16, 27),
	[MCSC_F_YUV_WDMASC_W1_BW_LIMIT_AVG_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_BW_LIMIT_1, 0, 15),
	[MCSC_F_YUV_WDMASC_W1_BW_LIMIT_SLOT_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_BW_LIMIT_1, 16, 31),
	[MCSC_F_YUV_WDMASC_W1_BW_LIMIT_COMPENSATION_PERIOD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_BW_LIMIT_2, 0, 11),
	[MCSC_F_YUV_WDMASC_W1_BW_LIMIT_COMPENSATION_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_BW_LIMIT_2, 16, 31),
	[MCSC_F_YUV_WDMASC_W1_CACHE_WLU_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_CACHE_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W1_CACHE_32B_PARTIAL_ALLOC_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_CACHE_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W1_DEBUG_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DEBUG_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W1_DEBUG_CAPTURE_ONCE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DEBUG_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W1_DEBUG_CLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DEBUG_CONTROL, 16, 31),
	[MCSC_F_YUV_WDMASC_W1_DEBUG_AXI_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DEBUG_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_DEBUG_AXI_MO_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DEBUG_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_DEBUG_AXI_AW_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DEBUG_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W1_FLIP_CONTROL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_FLIP_CONTROL, 0, 1),
	[MCSC_F_YUV_WDMASC_W1_RGB_ALPHA] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_RGB_ALPHA, 0, 7),
	[MCSC_F_YUV_WDMASC_W2_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_ENABLE, 0, 0),
	[MCSC_F_YUV_WDMASC_W2_CLK_ALWAYS_ON_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_ENABLE, 1, 1),
	[MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_BAYER] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DATA_FORMAT, 0, 4),
	[MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_YUV] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DATA_FORMAT, 8, 13),
	[MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_RGB] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DATA_FORMAT, 16, 19),
	[MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_4B_SWAP] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DATA_FORMAT, 24, 24),
	[MCSC_F_YUV_WDMASC_W2_DATA_FORMAT_TYPE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DATA_FORMAT, 31, 31),
	[MCSC_F_YUV_WDMASC_W2_MONO_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_MONO_CTRL, 0, 0),
	[MCSC_F_YUV_WDMASC_W2_WIDTH] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_WIDTH, 0, 16),
	[MCSC_F_YUV_WDMASC_W2_HEIGHT] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_HEIGHT, 0, 13),
	[MCSC_F_YUV_WDMASC_W2_STRIDE_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_STRIDE_1P, 0, 18),
	[MCSC_F_YUV_WDMASC_W2_STRIDE_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_STRIDE_2P, 0, 18),
	[MCSC_F_YUV_WDMASC_W2_STRIDE_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_STRIDE_3P, 0, 18),
	[MCSC_F_YUV_WDMASC_W2_VOTF_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_VOTF_EN, 0, 1),
	[MCSC_F_YUV_WDMASC_W2_MAX_MO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_MAX_MO, 0, 8),
	[MCSC_F_YUV_WDMASC_W2_BUSINFO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_BUSINFO, 0, 10),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W2_IMG_CRC_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_CRC_1P, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_CRC_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_CRC_2P, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_IMG_CRC_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_IMG_CRC_3P, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_MON_STATUS_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_MON_STATUS_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_MON_STATUS_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_MON_STATUS_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_MON_STATUS_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_MON_STATUS_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_MON_STATUS_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_MON_STATUS_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_BW_LIMIT_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_BW_LIMIT_0, 0, 0),
	[MCSC_F_YUV_WDMASC_W2_BW_LIMIT_FREQ_NUM_CYCLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_BW_LIMIT_0, 16, 27),
	[MCSC_F_YUV_WDMASC_W2_BW_LIMIT_AVG_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_BW_LIMIT_1, 0, 15),
	[MCSC_F_YUV_WDMASC_W2_BW_LIMIT_SLOT_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_BW_LIMIT_1, 16, 31),
	[MCSC_F_YUV_WDMASC_W2_BW_LIMIT_COMPENSATION_PERIOD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_BW_LIMIT_2, 0, 11),
	[MCSC_F_YUV_WDMASC_W2_BW_LIMIT_COMPENSATION_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_BW_LIMIT_2, 16, 31),
	[MCSC_F_YUV_WDMASC_W2_CACHE_WLU_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_CACHE_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W2_CACHE_32B_PARTIAL_ALLOC_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_CACHE_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W2_DEBUG_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DEBUG_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W2_DEBUG_CAPTURE_ONCE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DEBUG_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W2_DEBUG_CLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DEBUG_CONTROL, 16, 31),
	[MCSC_F_YUV_WDMASC_W2_DEBUG_AXI_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DEBUG_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_DEBUG_AXI_MO_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DEBUG_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_DEBUG_AXI_AW_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DEBUG_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W2_FLIP_CONTROL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_FLIP_CONTROL, 0, 1),
	[MCSC_F_YUV_WDMASC_W2_RGB_ALPHA] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_RGB_ALPHA, 0, 7),
	[MCSC_F_YUV_WDMASC_W3_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_ENABLE, 0, 0),
	[MCSC_F_YUV_WDMASC_W3_CLK_ALWAYS_ON_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_ENABLE, 1, 1),
	[MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_BAYER] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DATA_FORMAT, 0, 4),
	[MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_YUV] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DATA_FORMAT, 8, 13),
	[MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_RGB] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DATA_FORMAT, 16, 19),
	[MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_4B_SWAP] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DATA_FORMAT, 24, 24),
	[MCSC_F_YUV_WDMASC_W3_DATA_FORMAT_TYPE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DATA_FORMAT, 31, 31),
	[MCSC_F_YUV_WDMASC_W3_MONO_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_MONO_CTRL, 0, 0),
	[MCSC_F_YUV_WDMASC_W3_WIDTH] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_WIDTH, 0, 16),
	[MCSC_F_YUV_WDMASC_W3_HEIGHT] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_HEIGHT, 0, 13),
	[MCSC_F_YUV_WDMASC_W3_STRIDE_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_STRIDE_1P, 0, 18),
	[MCSC_F_YUV_WDMASC_W3_STRIDE_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_STRIDE_2P, 0, 18),
	[MCSC_F_YUV_WDMASC_W3_STRIDE_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_STRIDE_3P, 0, 18),
	[MCSC_F_YUV_WDMASC_W3_VOTF_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_VOTF_EN, 0, 1),
	[MCSC_F_YUV_WDMASC_W3_MAX_MO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_MAX_MO, 0, 8),
	[MCSC_F_YUV_WDMASC_W3_BUSINFO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_BUSINFO, 0, 10),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W3_IMG_CRC_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_CRC_1P, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_CRC_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_CRC_2P, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_IMG_CRC_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_IMG_CRC_3P, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_MON_STATUS_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_MON_STATUS_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_MON_STATUS_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_MON_STATUS_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_MON_STATUS_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_MON_STATUS_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_MON_STATUS_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_MON_STATUS_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_BW_LIMIT_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_BW_LIMIT_0, 0, 0),
	[MCSC_F_YUV_WDMASC_W3_BW_LIMIT_FREQ_NUM_CYCLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_BW_LIMIT_0, 16, 27),
	[MCSC_F_YUV_WDMASC_W3_BW_LIMIT_AVG_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_BW_LIMIT_1, 0, 15),
	[MCSC_F_YUV_WDMASC_W3_BW_LIMIT_SLOT_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_BW_LIMIT_1, 16, 31),
	[MCSC_F_YUV_WDMASC_W3_BW_LIMIT_COMPENSATION_PERIOD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_BW_LIMIT_2, 0, 11),
	[MCSC_F_YUV_WDMASC_W3_BW_LIMIT_COMPENSATION_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_BW_LIMIT_2, 16, 31),
	[MCSC_F_YUV_WDMASC_W3_CACHE_WLU_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_CACHE_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W3_CACHE_32B_PARTIAL_ALLOC_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_CACHE_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W3_DEBUG_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DEBUG_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W3_DEBUG_CAPTURE_ONCE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DEBUG_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W3_DEBUG_CLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DEBUG_CONTROL, 16, 31),
	[MCSC_F_YUV_WDMASC_W3_DEBUG_AXI_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DEBUG_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_DEBUG_AXI_MO_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DEBUG_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_DEBUG_AXI_AW_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DEBUG_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W3_FLIP_CONTROL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_FLIP_CONTROL, 0, 1),
	[MCSC_F_YUV_WDMASC_W3_RGB_ALPHA] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_RGB_ALPHA, 0, 7),
	[MCSC_F_YUV_WDMASC_W4_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_ENABLE, 0, 0),
	[MCSC_F_YUV_WDMASC_W4_CLK_ALWAYS_ON_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_ENABLE, 1, 1),
	[MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_BAYER] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DATA_FORMAT, 0, 4),
	[MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_YUV] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DATA_FORMAT, 8, 13),
	[MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_RGB] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DATA_FORMAT, 16, 19),
	[MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_4B_SWAP] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DATA_FORMAT, 24, 24),
	[MCSC_F_YUV_WDMASC_W4_DATA_FORMAT_TYPE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DATA_FORMAT, 31, 31),
	[MCSC_F_YUV_WDMASC_W4_MONO_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_MONO_CTRL, 0, 0),
	[MCSC_F_YUV_WDMASC_W4_WIDTH] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_WIDTH, 0, 16),
	[MCSC_F_YUV_WDMASC_W4_HEIGHT] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_HEIGHT, 0, 13),
	[MCSC_F_YUV_WDMASC_W4_STRIDE_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_STRIDE_1P, 0, 18),
	[MCSC_F_YUV_WDMASC_W4_STRIDE_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_STRIDE_2P, 0, 18),
	[MCSC_F_YUV_WDMASC_W4_STRIDE_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_STRIDE_3P, 0, 18),
	[MCSC_F_YUV_WDMASC_W4_VOTF_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_VOTF_EN, 0, 1),
	[MCSC_F_YUV_WDMASC_W4_MAX_MO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_MAX_MO, 0, 8),
	[MCSC_F_YUV_WDMASC_W4_BUSINFO] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_BUSINFO, 0, 10),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_1P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_2P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_4, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_5, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_6, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_0_7, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_0, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_1, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_2, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_3, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_4, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_5, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_6, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7, 0, 3),
	[MCSC_F_YUV_WDMASC_W4_IMG_CRC_1P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_CRC_1P, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_CRC_2P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_CRC_2P, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_IMG_CRC_3P] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_IMG_CRC_3P, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_MON_STATUS_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_MON_STATUS_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_MON_STATUS_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_MON_STATUS_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_MON_STATUS_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_MON_STATUS_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_MON_STATUS_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_MON_STATUS_3, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_BW_LIMIT_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_BW_LIMIT_0, 0, 0),
	[MCSC_F_YUV_WDMASC_W4_BW_LIMIT_FREQ_NUM_CYCLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_BW_LIMIT_0, 16, 27),
	[MCSC_F_YUV_WDMASC_W4_BW_LIMIT_AVG_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_BW_LIMIT_1, 0, 15),
	[MCSC_F_YUV_WDMASC_W4_BW_LIMIT_SLOT_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_BW_LIMIT_1, 16, 31),
	[MCSC_F_YUV_WDMASC_W4_BW_LIMIT_COMPENSATION_PERIOD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_BW_LIMIT_2, 0, 11),
	[MCSC_F_YUV_WDMASC_W4_BW_LIMIT_COMPENSATION_BW] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_BW_LIMIT_2, 16, 31),
	[MCSC_F_YUV_WDMASC_W4_CACHE_WLU_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_CACHE_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W4_CACHE_32B_PARTIAL_ALLOC_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_CACHE_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W4_DEBUG_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DEBUG_CONTROL, 0, 0),
	[MCSC_F_YUV_WDMASC_W4_DEBUG_CAPTURE_ONCE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DEBUG_CONTROL, 1, 1),
	[MCSC_F_YUV_WDMASC_W4_DEBUG_CLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DEBUG_CONTROL, 16, 31),
	[MCSC_F_YUV_WDMASC_W4_DEBUG_AXI_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DEBUG_0, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_DEBUG_AXI_MO_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DEBUG_1, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_DEBUG_AXI_AW_BLK_CNT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DEBUG_2, 0, 31),
	[MCSC_F_YUV_WDMASC_W4_FLIP_CONTROL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_FLIP_CONTROL, 0, 1),
	[MCSC_F_YUV_WDMASC_W4_RGB_ALPHA] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_RGB_ALPHA, 0, 7),
	[MCSC_F_YUV_WDMASC_W0_DITHER_EN_C] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DITHER, 0, 0),
	[MCSC_F_YUV_WDMASC_W0_DITHER_EN_Y] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DITHER, 1, 1),
	[MCSC_F_YUV_WDMASC_W0_ROUND_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_DITHER, 4, 4),
	[MCSC_F_YUV_WDMASC_W0_RGB_CONV444_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_RGB_CONV444_WEIGHT, 0, 4),
	[MCSC_F_YUV_WDMASC_W0_PER_SUB_FRAME_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_PER_SUB_FRAME_EN, 0, 15),
	[MCSC_F_YUV_WDMASC_W0_COMP_SRAM_START_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W0_COMP_SRAM_START_ADDR, 0, 13),
	[MCSC_F_YUV_WDMASC_W1_DITHER_EN_C] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DITHER, 0, 0),
	[MCSC_F_YUV_WDMASC_W1_DITHER_EN_Y] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DITHER, 1, 1),
	[MCSC_F_YUV_WDMASC_W1_ROUND_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_DITHER, 4, 4),
	[MCSC_F_YUV_WDMASC_W1_RGB_CONV444_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_RGB_CONV444_WEIGHT, 0, 4),
	[MCSC_F_YUV_WDMASC_W1_PER_SUB_FRAME_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_PER_SUB_FRAME_EN, 0, 15),
	[MCSC_F_YUV_WDMASC_W1_COMP_SRAM_START_ADDR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W1_COMP_SRAM_START_ADDR, 0, 13),
	[MCSC_F_YUV_WDMASC_W2_DITHER_EN_C] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DITHER, 0, 0),
	[MCSC_F_YUV_WDMASC_W2_DITHER_EN_Y] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DITHER, 1, 1),
	[MCSC_F_YUV_WDMASC_W2_ROUND_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_DITHER, 4, 4),
	[MCSC_F_YUV_WDMASC_W2_RGB_CONV444_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_RGB_CONV444_WEIGHT, 0, 4),
	[MCSC_F_YUV_WDMASC_W2_PER_SUB_FRAME_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W2_PER_SUB_FRAME_EN, 0, 15),
	[MCSC_F_YUV_WDMASC_W3_DITHER_EN_C] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DITHER, 0, 0),
	[MCSC_F_YUV_WDMASC_W3_DITHER_EN_Y] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DITHER, 1, 1),
	[MCSC_F_YUV_WDMASC_W3_ROUND_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_DITHER, 4, 4),
	[MCSC_F_YUV_WDMASC_W3_RGB_CONV444_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_RGB_CONV444_WEIGHT, 0, 4),
	[MCSC_F_YUV_WDMASC_W3_PER_SUB_FRAME_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W3_PER_SUB_FRAME_EN, 0, 15),
	[MCSC_F_YUV_WDMASC_W4_DITHER_EN_C] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DITHER, 0, 0),
	[MCSC_F_YUV_WDMASC_W4_DITHER_EN_Y] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DITHER, 1, 1),
	[MCSC_F_YUV_WDMASC_W4_ROUND_EN] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_DITHER, 4, 4),
	[MCSC_F_YUV_WDMASC_W4_RGB_CONV444_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_RGB_CONV444_WEIGHT, 0, 4),
	[MCSC_F_YUV_WDMASC_W4_PER_SUB_FRAME_EN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_W4_PER_SUB_FRAME_EN, 0, 15),
	[MCSC_F_YUV_WDMASC_RGB_SRC_Y_OFFSET] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_OFFSET, 0, 9),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C10] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_0, 0, 15),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C00] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_0, 16, 31),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C01] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_1, 0, 15),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C20] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_1, 16, 31),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C21] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_2, 0, 15),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C11] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_2, 16, 31),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C12] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_3, 0, 15),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C02] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_3, 16, 31),
	[MCSC_F_YUV_WDMASC_RGB_COEF_C22] = PMIO_FIELD_DESC(MCSC_R_YUV_WDMASC_RGB_COEF_4, 0, 15),
	[MCSC_F_YUV_DJAGPS_PS_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CTRL, 1, 1),
	[MCSC_F_YUV_DJAGFILTER_DJAG_BACKWARD_COMP] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CTRL, 2, 2),
	[MCSC_F_YUV_DJAG_CLK_GATE_DISABLE_PS] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CTRL, 3, 3),
	[MCSC_F_YUV_DJAG_CLK_GATE_DISABLE_PS_LB] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CTRL, 4, 4),
	[MCSC_F_YUV_DJAGPS_PS_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CTRL, 8, 8),
	[MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CTRL, 9, 9),
	[MCSC_F_YUV_DJAGFILTER_EZPOST_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CTRL, 10, 10),
	[MCSC_F_YUV_DJAGPS_DUALLAYERNR_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CTRL, 11, 11),
	[MCSC_F_YUV_DJAGPS_INPUT_IMG_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_IMG_SIZE, 0, 15),
	[MCSC_F_YUV_DJAGPS_INPUT_IMG_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_IMG_SIZE, 16, 31),
	[MCSC_F_YUV_DJAGPS_PS_SRC_VPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_PS_SRC_POS, 0, 15),
	[MCSC_F_YUV_DJAGPS_PS_SRC_HPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_PS_SRC_POS, 16, 31),
	[MCSC_F_YUV_DJAGPS_PS_SRC_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_PS_SRC_SIZE, 0, 15),
	[MCSC_F_YUV_DJAGPS_PS_SRC_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_PS_SRC_SIZE, 16, 31),
	[MCSC_F_YUV_DJAGPS_PS_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_PS_DST_SIZE, 0, 15),
	[MCSC_F_YUV_DJAGPS_PS_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_PS_DST_SIZE, 16, 31),
	[MCSC_F_YUV_DJAGPS_PS_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_H_RATIO, 0, 27),
	[MCSC_F_YUV_DJAGPS_PS_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_V_RATIO, 0, 27),
	[MCSC_F_YUV_DJAGPS_PS_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_DJAGPS_PS_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_DJAGPS_PS_ROUND_MODE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_DJAGPS_PS_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_PS_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_DJAGPS_PS_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_PS_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_DJAGOUTCROP_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_DJAGFILTER_XFILTER_DEJAGGING_WEIGHT0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_XFILTER_DEJAGGING_COEFF, 0, 3),
	[MCSC_F_YUV_DJAGFILTER_XFILTER_DEJAGGING_WEIGHT1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_XFILTER_DEJAGGING_COEFF, 4, 7),
	[MCSC_F_YUV_DJAGFILTER_XFILTER_HF_BOOST_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_XFILTER_DEJAGGING_COEFF, 8, 11),
	[MCSC_F_YUV_DJAGFILTER_CENTER_HF_BOOST_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_XFILTER_DEJAGGING_COEFF, 12, 14),
	[MCSC_F_YUV_DJAGFILTER_DIAGONAL_HF_BOOST_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_XFILTER_DEJAGGING_COEFF, 15, 17),
	[MCSC_F_YUV_DJAGFILTER_CENTER_WEIGHTED_MEAN_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_XFILTER_DEJAGGING_COEFF, 18, 20),
	[MCSC_F_YUV_DJAGFILTER_THRES_1X5_MATCHING_SAD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_THRES_1X5_MATCHING, 0, 9),
	[MCSC_F_YUV_DJAGFILTER_THRES_1X5_ABSHF] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_THRES_1X5_MATCHING, 10, 19),
	[MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_LLCRR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_0, 0, 7),
	[MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_LCR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_0, 8, 15),
	[MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_NEIGHBOR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_0, 16, 23),
	[MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_UUCDD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_1, 0, 7),
	[MCSC_F_YUV_DJAGFILTER_THRES_SHOOTING_UCD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_1, 8, 15),
	[MCSC_F_YUV_DJAGFILTER_MIN_MAX_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_THRES_SHOOTING_DETECT_1, 16, 19),
	[MCSC_F_YUV_DJAGFILTER_LFSR_SEED_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_LFSR_SEED_0, 0, 15),
	[MCSC_F_YUV_DJAGFILTER_LFSR_SEED_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_LFSR_SEED_1, 0, 15),
	[MCSC_F_YUV_DJAGFILTER_LFSR_SEED_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_LFSR_SEED_2, 0, 15),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_04, 0, 5),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_04, 6, 11),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_04, 12, 17),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_04, 18, 23),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_04, 24, 29),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_58, 0, 5),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_58, 6, 11),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_58, 12, 17),
	[MCSC_F_YUV_DJAGFILTER_DITHER_VALUE_8] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_VALUE_58, 18, 23),
	[MCSC_F_YUV_DJAGFILTER_SAT_CTRL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_THRES, 0, 3),
	[MCSC_F_YUV_DJAGFILTER_DITHER_THRES] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_DITHER_THRES, 14, 18),
	[MCSC_F_YUV_DJAGFILTER_CP_HF_THRES] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGFILTER_CP_HF_THRES, 0, 6),
	[MCSC_F_YUV_DJAGFILTER_CP_ARBI_MAX_COV_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CP_ARBI, 0, 15),
	[MCSC_F_YUV_DJAGFILTER_CP_ARBI_MAX_COV_SHIFT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CP_ARBI, 16, 19),
	[MCSC_F_YUV_DJAGFILTER_CP_ARBI_DENOM] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CP_ARBI, 20, 23),
	[MCSC_F_YUV_DJAGFILTER_CP_ARBI_MODE] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_CP_ARBI, 24, 25),
	[MCSC_F_YUV_DJAGFILTER_DITHER_WB_THRES] = PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_DITHER_WB, 0, 4),
	[MCSC_F_YUV_DJAGFILTER_DITHER_BLACK_LEVEL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_DITHER_WB, 8, 16),
	[MCSC_F_YUV_DJAGFILTER_DITHER_WHITE_LEVEL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAG_DITHER_WB, 20, 29),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_DJAGPREFILTER_EZPOST_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_CTRL, 0, 0),
	[MCSC_F_YUV_DJAGPREFILTER_CLK_GATE_DISABLE_PS] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_CTRL, 3, 3),
	[MCSC_F_YUV_DJAGPREFILTER_CLK_GATE_DISABLE_PS_LB] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_CTRL, 4, 4),
	[MCSC_F_YUV_DJAGPREFILTER_INPUT_IMG_VSIZE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_IMG_SIZE, 0, 15),
	[MCSC_F_YUV_DJAGPREFILTER_INPUT_IMG_HSIZE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_IMG_SIZE, 16, 31),
	[MCSC_F_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_WEIGHT0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF, 0, 3),
	[MCSC_F_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_WEIGHT1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF, 4, 7),
	[MCSC_F_YUV_DJAGPREFILTER_XFILTER_HF_BOOST_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF, 8, 11),
	[MCSC_F_YUV_DJAGPREFILTER_CENTER_HF_BOOST_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF, 12, 14),
	[MCSC_F_YUV_DJAGPREFILTER_DIAGONAL_HF_BOOST_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF, 15, 17),
	[MCSC_F_YUV_DJAGPREFILTER_CENTER_WEIGHTED_MEAN_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_XFILTER_DEJAGGING_COEFF, 18, 20),
	[MCSC_F_YUV_DJAGPREFILTER_THRES_1X5_MATCHING_SAD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_THRES_1X5_MATCHING, 0, 9),
	[MCSC_F_YUV_DJAGPREFILTER_THRES_1X5_ABSHF] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_THRES_1X5_MATCHING, 10, 19),
	[MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_LLCRR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_0, 0, 7),
	[MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_LCR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_0, 8, 15),
	[MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_NEIGHBOR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_0, 16, 23),
	[MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_UUCDD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_1, 0, 7),
	[MCSC_F_YUV_DJAGPREFILTER_THRES_SHOOTING_UCD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_1, 8, 15),
	[MCSC_F_YUV_DJAGPREFILTER_MIN_MAX_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_THRES_SHOOTING_DETECT_1, 16, 19),
	[MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_0, 0, 15),
	[MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_1, 0, 15),
	[MCSC_F_YUV_DJAGPREFILTER_LFSR_SEED_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_LFSR_SEED_2, 0, 15),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_04, 0, 5),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_04, 6, 11),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_04, 12, 17),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_04, 18, 23),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_04, 24, 29),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_58, 0, 5),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_58, 6, 11),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_58, 12, 17),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_VALUE_8] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_VALUE_58, 18, 23),
	[MCSC_F_YUV_DJAGPREFILTER_SAT_CTRL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_THRES, 0, 3),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_THRES] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_DITHER_THRES, 14, 18),
	[MCSC_F_YUV_DJAGPREFILTER_CP_HF_THRES] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_FILTER_CP_HF_THRES, 0, 6),
	[MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MAX_COV_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_CP_ARBI, 0, 15),
	[MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MAX_COV_SHIFT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_CP_ARBI, 16, 19),
	[MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_DENOM] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_CP_ARBI, 20, 23),
	[MCSC_F_YUV_DJAGPREFILTER_CP_ARBI_MODE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_CP_ARBI, 24, 25),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_WB_THRES] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_DITHER_WB, 0, 4),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_BLACK_LEVEL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_DITHER_WB, 8, 16),
	[MCSC_F_YUV_DJAGPREFILTER_DITHER_WHITE_LEVEL] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_DITHER_WB, 20, 29),
	[MCSC_F_YUV_DJAGPREFILTER_HARRIS_K] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_HARRIS_DET, 0, 4),
	[MCSC_F_YUV_DJAGPREFILTER_HARRIS_TH] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_HARRIS_DET, 5, 9),
	[MCSC_F_YUV_DJAGPREFILTER_BILATERAL_C] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DJAGPREFILTER_BILATERAL_C, 0, 9),
	[MCSC_F_YUV_DUALLAYERRECOMP_BYPASS] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_CTRL, 0, 0),
	[MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_CTRL, 1, 1),
	[MCSC_F_YUV_DUALLAYERRECOMP_WEIGHT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_WEIGHT, 0, 11),
	[MCSC_F_YUV_DUALLAYERRECOMP_INPUT_IMG_VSIZE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_IMG_SIZE, 0, 15),
	[MCSC_F_YUV_DUALLAYERRECOMP_INPUT_IMG_HSIZE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_IMG_SIZE, 16, 31),
	[MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_IN_CROP_POS, 0, 15),
	[MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_IN_CROP_POS, 16, 31),
	[MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_IN_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_DUALLAYERRECOMP_IN_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_IN_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_CENTER_Y] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_CENTER, 0, 15),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_CENTER_X] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_CENTER, 16, 31),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BINNING_Y] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BINNING, 0, 15),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BINNING_X] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BINNING, 16, 31),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_A, 0, 16),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_FACTOR_B, 0, 16),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_GAIN_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_GAIN_ENABLE, 0, 0),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_GAIN_INCREMENT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_GAIN_INCREMENT, 0, 0),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_SCALE_SHIFT_ADDER] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_BIQUAD_SCALE_SHIFT_ADDER, 0, 4),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_A, 0, 3),
	[MCSC_F_YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_RADIAL_ELLIPTIC_FACTOR_B, 0, 3),
	[MCSC_F_YUV_DUALLAYERRECOMP_CRC_SEED] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_STREAM_CRC, 0, 7),
	[MCSC_F_YUV_DUALLAYERRECOMP_CRC_RESULT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DUALLAYERRECOMP_STREAM_CRC, 8, 15),
	[MCSC_F_YUV_POLY_SC0_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_CTRL, 0, 0),
	[MCSC_F_YUV_POLY_SC0_BYPASS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_CTRL, 1, 1),
	[MCSC_F_YUV_POLY_SC0_CLK_GATE_DISABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_CTRL, 3, 3),
	[MCSC_F_YUV_POLY_SC0_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_CTRL, 8, 8),
	[MCSC_F_YUV_POLY_SC0_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_CTRL, 9, 9),
	[MCSC_F_YUV_POLY_SC0_SRC_VPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_SRC_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC0_SRC_HPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_SRC_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC0_SRC_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_SRC_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC0_SRC_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_SRC_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC0_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_DST_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC0_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC0_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_H_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC0_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_V_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC0_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC0_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC0_ROUND_MODE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_POLY_SC0_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_CTRL, 0, 0),
	[MCSC_F_YUV_POLY_SC1_BYPASS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_CTRL, 1, 1),
	[MCSC_F_YUV_POLY_SC1_CLK_GATE_DISABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_CTRL, 3, 3),
	[MCSC_F_YUV_POLY_SC1_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_CTRL, 8, 8),
	[MCSC_F_YUV_POLY_SC1_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_CTRL, 9, 9),
	[MCSC_F_YUV_POLY_SC1_SRC_VPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_SRC_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC1_SRC_HPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_SRC_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC1_SRC_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_SRC_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC1_SRC_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_SRC_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC1_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_DST_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC1_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC1_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_H_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC1_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_V_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC1_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC1_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC1_ROUND_MODE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_POLY_SC1_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_CTRL, 0, 0),
	[MCSC_F_YUV_POLY_SC2_BYPASS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_CTRL, 1, 1),
	[MCSC_F_YUV_POLY_SC2_CLK_GATE_DISABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_CTRL, 3, 3),
	[MCSC_F_YUV_POLY_SC2_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_CTRL, 8, 8),
	[MCSC_F_YUV_POLY_SC2_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_CTRL, 9, 9),
	[MCSC_F_YUV_POLY_SC2_SRC_VPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_SRC_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC2_SRC_HPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_SRC_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC2_SRC_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_SRC_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC2_SRC_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_SRC_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC2_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_DST_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC2_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC2_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_H_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC2_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_V_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC2_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC2_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC2_ROUND_MODE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_POLY_SC2_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_CTRL, 0, 0),
	[MCSC_F_YUV_POLY_SC3_BYPASS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_CTRL, 1, 1),
	[MCSC_F_YUV_POLY_SC3_CLK_GATE_DISABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_CTRL, 3, 3),
	[MCSC_F_YUV_POLY_SC3_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_CTRL, 8, 8),
	[MCSC_F_YUV_POLY_SC3_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_CTRL, 9, 9),
	[MCSC_F_YUV_POLY_SC3_SRC_VPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_SRC_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC3_SRC_HPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_SRC_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC3_SRC_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_SRC_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC3_SRC_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_SRC_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC3_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_DST_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC3_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC3_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_H_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC3_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_V_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC3_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC3_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC3_ROUND_MODE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_POLY_SC3_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_CTRL, 0, 0),
	[MCSC_F_YUV_POLY_SC4_BYPASS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_CTRL, 1, 1),
	[MCSC_F_YUV_POLY_SC4_CLK_GATE_DISABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_CTRL, 3, 3),
	[MCSC_F_YUV_POLY_SC4_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_CTRL, 8, 8),
	[MCSC_F_YUV_POLY_SC4_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_CTRL, 9, 9),
	[MCSC_F_YUV_POLY_SC4_SRC_VPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_SRC_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC4_SRC_HPOS] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_SRC_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC4_SRC_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_SRC_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC4_SRC_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_SRC_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC4_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_DST_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC4_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC4_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_H_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC4_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_V_RATIO, 0, 27),
	[MCSC_F_YUV_POLY_SC4_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC4_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POLY_SC4_ROUND_MODE] = PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_POLY_SC4_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC0_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC1_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC2_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC3_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POLY_SC4_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POLY_SC0_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC0_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC0_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC0_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC0_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC0_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC0_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC1_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC1_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC1_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC1_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC1_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC1_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC1_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC2_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC2_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC2_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC2_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC2_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC2_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC2_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC3_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC3_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC3_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC3_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC3_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC3_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC3_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC4_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POLY_SC4_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC4_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_POLY_SC4_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_POLY_SC4_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_POLY_SC4_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POLY_SC4_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC0_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CTRL, 0, 0),
	[MCSC_F_YUV_POST_PC0_CLK_GATE_DISABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CTRL, 1, 1),
	[MCSC_F_YUV_POST_PC0_CONV420_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CTRL, 2, 2),
	[MCSC_F_YUV_POST_PC0_BCHS_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CTRL, 3, 3),
	[MCSC_F_YUV_POST_PC0_LB_CTRL_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CTRL, 4, 4),
	[MCSC_F_YUV_POSTPC_PC0_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CTRL, 8, 8),
	[MCSC_F_YUV_POSTPC_PC0_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CTRL, 9, 9),
	[MCSC_F_YUV_POSTPC_PC0_IMG_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_IMG_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC0_IMG_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_IMG_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC0_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_DST_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC0_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC0_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_H_RATIO, 0, 27),
	[MCSC_F_YUV_POSTPC_PC0_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_V_RATIO, 0, 27),
	[MCSC_F_YUV_POSTPC_PC0_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POSTPC_PC0_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POSTPC_PC0_ROUND_MODE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_POSTPC_PC0_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC0_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_POSTPC_PC0_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_POSTPC_PC0_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_POSTPC_PC0_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC0_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_POSTCONV_PC0_CONV420_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CONV420_CTRL, 0, 0),
	[MCSC_F_YUV_POSTCONV_PC0_CONV420_ODD_LINE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_CONV420_CTRL, 1, 1),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_CTRL, 0, 0),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_GAIN] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_BC, 0, 13),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_BC, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_00] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_HS1, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_01] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_HS1, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_10] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_HS2, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_GAIN_11] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_HS2, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_CLAMP_Y, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_Y_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_CLAMP_Y, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_CLAMP_C, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC0_BCHS_C_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC0_BCHS_CLAMP_C, 16, 25),
	[MCSC_F_YUV_POSTPC_PC1_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CTRL, 0, 0),
	[MCSC_F_YUV_POST_PC1_CLK_GATE_DISABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CTRL, 1, 1),
	[MCSC_F_YUV_POST_PC1_CONV420_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CTRL, 2, 2),
	[MCSC_F_YUV_POST_PC1_BCHS_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CTRL, 3, 3),
	[MCSC_F_YUV_POST_PC1_LB_CTRL_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CTRL, 4, 4),
	[MCSC_F_YUV_POSTPC_PC1_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CTRL, 8, 8),
	[MCSC_F_YUV_POSTPC_PC1_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CTRL, 9, 9),
	[MCSC_F_YUV_POSTPC_PC1_IMG_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_IMG_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC1_IMG_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_IMG_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC1_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_DST_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC1_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC1_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_H_RATIO, 0, 27),
	[MCSC_F_YUV_POSTPC_PC1_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_V_RATIO, 0, 27),
	[MCSC_F_YUV_POSTPC_PC1_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POSTPC_PC1_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POSTPC_PC1_ROUND_MODE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_POSTPC_PC1_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC1_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_POSTPC_PC1_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_POSTPC_PC1_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_POSTPC_PC1_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC1_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_POSTCONV_PC1_CONV420_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CONV420_CTRL, 0, 0),
	[MCSC_F_YUV_POSTCONV_PC1_CONV420_ODD_LINE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_CONV420_CTRL, 1, 1),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_CTRL, 0, 0),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_GAIN] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_BC, 0, 13),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_BC, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_GAIN_00] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_HS1, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_GAIN_01] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_HS1, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_GAIN_10] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_HS2, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_GAIN_11] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_HS2, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_CLAMP_Y, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_Y_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_CLAMP_Y, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_CLAMP_C, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC1_BCHS_C_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC1_BCHS_CLAMP_C, 16, 25),
	[MCSC_F_YUV_POSTPC_PC2_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CTRL, 0, 0),
	[MCSC_F_YUV_POST_PC2_CLK_GATE_DISABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CTRL, 1, 1),
	[MCSC_F_YUV_POST_PC2_CONV420_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CTRL, 2, 2),
	[MCSC_F_YUV_POST_PC2_BCHS_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CTRL, 3, 3),
	[MCSC_F_YUV_POST_PC2_LB_CTRL_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CTRL, 4, 4),
	[MCSC_F_YUV_POSTPC_PC2_STRIP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CTRL, 8, 8),
	[MCSC_F_YUV_POSTPC_PC2_OUT_CROP_ENABLE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CTRL, 9, 9),
	[MCSC_F_YUV_POSTPC_PC2_IMG_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_IMG_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC2_IMG_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_IMG_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC2_DST_VSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_DST_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC2_DST_HSIZE] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC2_H_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_H_RATIO, 0, 27),
	[MCSC_F_YUV_POSTPC_PC2_V_RATIO] = PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_V_RATIO, 0, 27),
	[MCSC_F_YUV_POSTPC_PC2_H_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_H_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POSTPC_PC2_V_INIT_PHASE_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_V_INIT_PHASE_OFFSET, 0, 19),
	[MCSC_F_YUV_POSTPC_PC2_ROUND_MODE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_ROUND_MODE, 0, 0),
	[MCSC_F_YUV_POSTPC_PC2_STRIP_PRE_DST_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_STRIP_PRE_DST_SIZE, 16, 31),
	[MCSC_F_YUV_POSTPC_PC2_STRIP_IN_START_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_STRIP_IN_START_POS, 16, 31),
	[MCSC_F_YUV_POSTPC_PC2_OUT_CROP_POS_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_OUT_CROP_POS, 0, 15),
	[MCSC_F_YUV_POSTPC_PC2_OUT_CROP_POS_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_OUT_CROP_POS, 16, 31),
	[MCSC_F_YUV_POSTPC_PC2_OUT_CROP_SIZE_V] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_OUT_CROP_SIZE, 0, 15),
	[MCSC_F_YUV_POSTPC_PC2_OUT_CROP_SIZE_H] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_OUT_CROP_SIZE, 16, 31),
	[MCSC_F_YUV_POSTCONV_PC2_CONV420_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CONV420_CTRL, 0, 0),
	[MCSC_F_YUV_POSTCONV_PC2_CONV420_ODD_LINE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_CONV420_CTRL, 1, 1),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_CTRL, 0, 0),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_GAIN] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_BC, 0, 13),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_BC, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_GAIN_00] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_HS1, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_GAIN_01] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_HS1, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_GAIN_10] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_HS2, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_GAIN_11] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_HS2, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_CLAMP_Y, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_Y_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_CLAMP_Y, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_CLAMP_C, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC2_BCHS_C_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC2_BCHS_CLAMP_C, 16, 25),
	[MCSC_F_YUV_POST_PC3_CONV420_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_CTRL, 2, 2),
	[MCSC_F_YUV_POST_PC3_BCHS_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_CTRL, 3, 3),
	[MCSC_F_YUV_POSTCONV_PC3_CONV420_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_CONV420_CTRL, 0, 0),
	[MCSC_F_YUV_POSTCONV_PC3_CONV420_ODD_LINE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_CONV420_CTRL, 1, 1),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_CTRL, 0, 0),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_GAIN] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_BC, 0, 13),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_BC, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_GAIN_00] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_HS1, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_GAIN_01] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_HS1, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_GAIN_10] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_HS2, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_GAIN_11] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_HS2, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_CLAMP_Y, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_Y_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_CLAMP_Y, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_CLAMP_C, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC3_BCHS_C_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC3_BCHS_CLAMP_C, 16, 25),
	[MCSC_F_YUV_POST_PC4_CONV420_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_CTRL, 2, 2),
	[MCSC_F_YUV_POST_PC4_BCHS_CLK_GATE_DISABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_CTRL, 3, 3),
	[MCSC_F_YUV_POSTCONV_PC4_CONV420_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_CONV420_CTRL, 0, 0),
	[MCSC_F_YUV_POSTCONV_PC4_CONV420_ODD_LINE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_CONV420_CTRL, 1, 1),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_ENABLE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_CTRL, 0, 0),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_GAIN] = PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_BC, 0, 13),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_OFFSET] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_BC, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_GAIN_00] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_HS1, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_GAIN_01] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_HS1, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_GAIN_10] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_HS2, 0, 14),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_GAIN_11] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_HS2, 16, 30),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_CLAMP_Y, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_Y_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_CLAMP_Y, 16, 25),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_CLAMP_MIN] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_CLAMP_C, 0, 9),
	[MCSC_F_YUV_POSTBCHS_PC4_BCHS_C_CLAMP_MAX] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POST_PC4_BCHS_CLAMP_C, 16, 25),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_0_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_0_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_1_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_1_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_2_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_2_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_3_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_3_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_4_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_4_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_5_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_5_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_6_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_6_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_7_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_7_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_4] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_45, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_5] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_45, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_6] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_67, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_Y_H_COEFF_8_7] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_Y_H_COEFF_8_67, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC0_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC0_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_V_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_V_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_0_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_0_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_0_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_0_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_0_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_0_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_0_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_0_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_1_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_1_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_1_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_1_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_1_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_1_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_1_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_1_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_2_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_2_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_2_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_2_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_2_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_2_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_2_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_2_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_3_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_3_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_3_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_3_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_3_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_3_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_3_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_3_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_4_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_4_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_4_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_4_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_4_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_4_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_4_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_4_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_5_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_5_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_5_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_5_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_5_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_5_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_5_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_5_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_6_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_6_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_6_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_6_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_6_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_6_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_6_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_6_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_7_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_7_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_7_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_7_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_7_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_7_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_7_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_7_23, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_8_0] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_8_01, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_8_1] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_8_01, 16, 26),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_8_2] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_8_23, 0, 10),
	[MCSC_F_YUV_POSTPC_PC2_UV_H_COEFF_8_3] =
		PMIO_FIELD_DESC(MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_8_23, 16, 26),
	[MCSC_F_YUV_HWFC_SWRESET] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_SWRESET, 0, 0),
	[MCSC_F_YUV_HWFC_MODE] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_MODE, 0, 2),
	[MCSC_F_YUV_HWFC_REGION_IDX_BIN_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_REGION_IDX_BIN, 0, 11),
	[MCSC_F_YUV_HWFC_REGION_IDX_BIN_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_REGION_IDX_BIN, 16, 27),
	[MCSC_F_YUV_HWFC_REGION_IDX_GRAY_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_REGION_IDX_GRAY, 0, 11),
	[MCSC_F_YUV_HWFC_REGION_IDX_GRAY_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_REGION_IDX_GRAY, 16, 27),
	[MCSC_F_YUV_HWFC_CURR_REGION_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CURR_REGION, 0, 11),
	[MCSC_F_YUV_HWFC_CURR_REGION_B] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CURR_REGION, 16, 27),
	[MCSC_F_YUV_HWFC_PLANE_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_A, 0, 1),
	[MCSC_F_YUV_HWFC_ID0_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_A, 4, 9),
	[MCSC_F_YUV_HWFC_ID1_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_A, 12, 17),
	[MCSC_F_YUV_HWFC_ID2_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_A, 20, 25),
	[MCSC_F_YUV_HWFC_FORMAT_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_A, 28, 28),
	[MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE0_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE0_A, 0, 27),
	[MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE0_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE0_A, 0, 14),
	[MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE1_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE1_A, 0, 27),
	[MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE1_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE1_A, 0, 14),
	[MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE2_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE2_A, 0, 27),
	[MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE2_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE2_A, 0, 14),
	[MCSC_F_YUV_HWFC_PLANE_B] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_B, 0, 1),
	[MCSC_F_YUV_HWFC_ID0_B] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_B, 4, 9),
	[MCSC_F_YUV_HWFC_ID1_B] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_B, 12, 17),
	[MCSC_F_YUV_HWFC_ID2_B] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_B, 20, 25),
	[MCSC_F_YUV_HWFC_FORMAT_B] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CONFIG_IMAGE_B, 28, 28),
	[MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE0_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE0_B, 0, 27),
	[MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE0_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE0_B, 0, 14),
	[MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE1_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE1_B, 0, 27),
	[MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE1_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE1_B, 0, 14),
	[MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE2_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE2_B, 0, 27),
	[MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE2_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE2_B, 0, 14),
	[MCSC_F_YUV_HWFC_FRAME_START_SELECT] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_FRAME_START_SELECT, 0, 0),
	[MCSC_F_YUV_HWFC_INDEX_RESET] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_INDEX_RESET, 0, 0),
	[MCSC_F_YUV_HWFC_ENABLE_AUTO_CLEAR] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_ENABLE_AUTO_CLEAR, 0, 0),
	[MCSC_F_YUV_HWFC_CORE_RESET_INPUT_SEL_A] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CORE_RESET_INPUT_SEL, 0, 0),
	[MCSC_F_YUV_HWFC_CORE_RESET_INPUT_SEL_B] =
		PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_CORE_RESET_INPUT_SEL, 4, 4),
	[MCSC_F_YUV_HWFC_MASTER_SEL_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_MASTER_SEL, 0, 2),
	[MCSC_F_YUV_HWFC_MASTER_SEL_B] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_MASTER_SEL, 4, 6),
	[MCSC_F_YUV_HWFC_IMAGE_HEIGHT_A] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_IMAGE_HEIGHT, 0, 15),
	[MCSC_F_YUV_HWFC_IMAGE_HEIGHT_B] = PMIO_FIELD_DESC(MCSC_R_YUV_HWFC_IMAGE_HEIGHT, 16, 31),
	[MCSC_F_YUV_DTPOTFIN_BYPASS] = PMIO_FIELD_DESC(MCSC_R_YUV_DTPOTFIN_BYPASS, 0, 0),
	[MCSC_F_YUV_DTPOTFIN_TEST_PATTERN_MODE] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DTPOTFIN_TEST_PATTERN_MODE, 0, 1),
	[MCSC_F_YUV_DTPOTFIN_TEST_DATA_Y] = PMIO_FIELD_DESC(MCSC_R_YUV_DTPOTFIN_TEST_DATA_Y, 0, 11),
	[MCSC_F_YUV_DTPOTFIN_TEST_DATA_U] = PMIO_FIELD_DESC(MCSC_R_YUV_DTPOTFIN_TEST_DATA_U, 0, 11),
	[MCSC_F_YUV_DTPOTFIN_TEST_DATA_V] = PMIO_FIELD_DESC(MCSC_R_YUV_DTPOTFIN_TEST_DATA_V, 0, 11),
	[MCSC_F_YUV_DTPOTFIN_YUV_STANDARD] =
		PMIO_FIELD_DESC(MCSC_R_YUV_DTPOTFIN_YUV_STANDARD, 0, 0),
	[MCSC_F_YUV_DTPOTFIN_CRC_SEED] = PMIO_FIELD_DESC(MCSC_R_YUV_DTPOTFIN_STREAM_CRC, 0, 7),
	[MCSC_F_YUV_DTPOTFIN_CRC_RESULT] = PMIO_FIELD_DESC(MCSC_R_YUV_DTPOTFIN_STREAM_CRC, 8, 15),
	[MCSC_F_STAT_DTPDUALLAYER_BYPASS] = PMIO_FIELD_DESC(MCSC_R_STAT_DTPDUALLAYER_BYPASS, 0, 0),
	[MCSC_F_STAT_DTPDUALLAYER_TEST_PATTERN_MODE] =
		PMIO_FIELD_DESC(MCSC_R_STAT_DTPDUALLAYER_TEST_PATTERN_MODE, 0, 1),
	[MCSC_F_STAT_DTPDUALLAYER_TEST_DATA_Y] =
		PMIO_FIELD_DESC(MCSC_R_STAT_DTPDUALLAYER_TEST_DATA_Y, 0, 11),
	[MCSC_F_STAT_DTPDUALLAYER_TEST_DATA_U] =
		PMIO_FIELD_DESC(MCSC_R_STAT_DTPDUALLAYER_TEST_DATA_U, 0, 11),
	[MCSC_F_STAT_DTPDUALLAYER_TEST_DATA_V] =
		PMIO_FIELD_DESC(MCSC_R_STAT_DTPDUALLAYER_TEST_DATA_V, 0, 11),
	[MCSC_F_STAT_DTPDUALLAYER_YUV_STANDARD] =
		PMIO_FIELD_DESC(MCSC_R_STAT_DTPDUALLAYER_YUV_STANDARD, 0, 0),
	[MCSC_F_STAT_DTPDUALLAYER_CRC_SEED] =
		PMIO_FIELD_DESC(MCSC_R_STAT_DTPDUALLAYER_STREAM_CRC, 0, 7),
	[MCSC_F_STAT_DTPDUALLAYER_CRC_RESULT] =
		PMIO_FIELD_DESC(MCSC_R_STAT_DTPDUALLAYER_STREAM_CRC, 8, 15),
	[MCSC_F_DBG_AXI_0] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_0, 0, 23),
	[MCSC_F_DBG_AXI_1] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_1, 0, 23),
	[MCSC_F_DBG_AXI_2] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_2, 0, 23),
	[MCSC_F_DBG_AXI_3] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_3, 0, 23),
	[MCSC_F_DBG_AXI_4] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_4, 0, 23),
	[MCSC_F_DBG_AXI_5] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_5, 0, 23),
	[MCSC_F_DBG_AXI_6] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_6, 0, 23),
	[MCSC_F_DBG_AXI_7] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_7, 0, 23),
	[MCSC_F_DBG_AXI_8] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_8, 0, 23),
	[MCSC_F_DBG_AXI_9] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_9, 0, 23),
	[MCSC_F_DBG_AXI_10] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_10, 0, 23),
	[MCSC_F_DBG_AXI_11] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_11, 0, 23),
	[MCSC_F_DBG_AXI_12] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_12, 0, 23),
	[MCSC_F_DBG_AXI_13] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_13, 0, 23),
	[MCSC_F_DBG_AXI_14] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_14, 0, 23),
	[MCSC_F_DBG_AXI_15] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_15, 0, 23),
	[MCSC_F_DBG_AXI_16] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_16, 0, 23),
	[MCSC_F_DBG_AXI_17] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_17, 0, 23),
	[MCSC_F_DBG_AXI_18] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_18, 0, 23),
	[MCSC_F_DBG_AXI_19] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_19, 0, 23),
	[MCSC_F_DBG_AXI_20] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_20, 0, 23),
	[MCSC_F_DBG_AXI_21] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_21, 0, 23),
	[MCSC_F_DBG_AXI_22] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_22, 0, 23),
	[MCSC_F_DBG_AXI_23] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_23, 0, 23),
	[MCSC_F_DBG_AXI_24] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_24, 0, 23),
	[MCSC_F_DBG_AXI_25] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_25, 0, 23),
	[MCSC_F_DBG_AXI_26] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_26, 0, 23),
	[MCSC_F_DBG_AXI_27] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_27, 0, 23),
	[MCSC_F_DBG_AXI_28] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_28, 0, 23),
	[MCSC_F_DBG_AXI_29] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_29, 0, 23),
	[MCSC_F_DBG_AXI_30] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_30, 0, 23),
	[MCSC_F_DBG_AXI_31] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_31, 0, 23),
	[MCSC_F_DBG_AXI_32] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_32, 0, 23),
	[MCSC_F_DBG_AXI_33] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_33, 0, 23),
	[MCSC_F_DBG_AXI_34] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_34, 0, 23),
	[MCSC_F_DBG_AXI_35] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_35, 0, 23),
	[MCSC_F_DBG_AXI_36] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_36, 0, 23),
	[MCSC_F_DBG_AXI_37] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_37, 0, 23),
	[MCSC_F_DBG_AXI_38] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_38, 0, 23),
	[MCSC_F_DBG_AXI_39] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_39, 0, 23),
	[MCSC_F_DBG_AXI_40] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_40, 0, 23),
	[MCSC_F_DBG_AXI_41] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_41, 0, 23),
	[MCSC_F_DBG_AXI_42] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_42, 0, 23),
	[MCSC_F_DBG_AXI_43] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_43, 0, 23),
	[MCSC_F_DBG_AXI_44] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_44, 0, 23),
	[MCSC_F_DBG_AXI_45] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_45, 0, 23),
	[MCSC_F_DBG_AXI_46] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_46, 0, 23),
	[MCSC_F_DBG_AXI_47] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_47, 0, 23),
	[MCSC_F_DBG_AXI_48] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_48, 0, 23),
	[MCSC_F_DBG_AXI_49] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_49, 0, 23),
	[MCSC_F_DBG_AXI_58] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_58, 0, 23),
	[MCSC_F_DBG_AXI_59] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_59, 0, 23),
	[MCSC_F_DBG_AXI_60] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_60, 0, 23),
	[MCSC_F_DBG_AXI_61] = PMIO_FIELD_DESC(MCSC_R_DBG_AXI_61, 0, 23),
	[MCSC_F_DBG_SC_0] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_0, 0, 31),
	[MCSC_F_DBG_SC_1] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_1, 0, 31),
	[MCSC_F_DBG_SC_2] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_2, 0, 31),
	[MCSC_F_DBG_SC_3] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_3, 0, 31),
	[MCSC_F_DBG_SC_4] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_4, 0, 31),
	[MCSC_F_DBG_SC_5] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_5, 0, 31),
	[MCSC_F_DBG_SC_6] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_6, 0, 31),
	[MCSC_F_DBG_SC_7] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_7, 0, 31),
	[MCSC_F_DBG_SC_8] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_8, 0, 31),
	[MCSC_F_DBG_SC_9] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_9, 0, 31),
	[MCSC_F_DBG_SC_10] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_10, 0, 31),
	[MCSC_F_DBG_SC_11] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_11, 0, 31),
	[MCSC_F_DBG_SC_12] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_12, 0, 31),
	[MCSC_F_DBG_SC_13] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_13, 0, 31),
	[MCSC_F_DBG_SC_14] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_14, 0, 31),
	[MCSC_F_DBG_SC_15] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_15, 0, 31),
	[MCSC_F_DBG_SC_16] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_16, 0, 31),
	[MCSC_F_DBG_SC_17] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_17, 0, 31),
	[MCSC_F_DBG_SC_18] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_18, 0, 31),
	[MCSC_F_DBG_SC_19] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_19, 0, 31),
	[MCSC_F_DBG_SC_20] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_20, 0, 31),
	[MCSC_F_DBG_SC_21] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_21, 0, 31),
	[MCSC_F_DBG_SC_22] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_22, 0, 31),
	[MCSC_F_DBG_SC_23] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_23, 0, 31),
	[MCSC_F_DBG_SC_24] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_24, 0, 31),
	[MCSC_F_DBG_SC_25] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_25, 0, 31),
	[MCSC_F_DBG_SC_26] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_26, 0, 31),
	[MCSC_F_DBG_SC_27] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_27, 0, 31),
	[MCSC_F_DBG_SC_28] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_28, 0, 31),
	[MCSC_F_DBG_SC_29] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_29, 0, 31),
	[MCSC_F_DBG_SC_30] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_30, 0, 31),
	[MCSC_F_DBG_SC_31] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_31, 0, 31),
	[MCSC_F_DBG_SC_32] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_32, 0, 31),
	[MCSC_F_DBG_SC_33] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_33, 0, 31),
	[MCSC_F_DBG_SC_34] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_34, 0, 31),
	[MCSC_F_YUV_DJAGPS_CRC_TOP_IN_OTF] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_0, 0, 7),
	[MCSC_F_YUV_DJAGPS_CRC_PS] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_0, 8, 15),
	[MCSC_F_YUV_DJAGFILTER_CRC_EZPOST_OUT] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_0, 24, 31),
	[MCSC_F_YUV_DJAGOUTCROP_CRC_DJAG_OUT] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_1, 0, 7),
	[MCSC_F_YUV_POLY_SC0_CRC_POLY] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_1, 8, 15),
	[MCSC_F_YUV_POLY_SC1_CRC_POLY] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_1, 16, 23),
	[MCSC_F_YUV_POLY_SC2_CRC_POLY] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_1, 24, 31),
	[MCSC_F_YUV_POLY_SC3_CRC_POLY] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_2, 0, 7),
	[MCSC_F_YUV_POLY_SC4_CRC_POLY] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_2, 8, 15),
	[MCSC_F_YUV_POSTPC_PC0_CRC_POST] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_2, 16, 23),
	[MCSC_F_YUV_POSTPC_PC1_CRC_POST] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_2, 24, 31),
	[MCSC_F_YUV_POSTPC_PC2_CRC_POST] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_3, 0, 7),
	[MCSC_F_YUV_POSTPC_PC3_CRC_POST] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_3, 8, 15),
	[MCSC_F_YUV_POSTPC_PC4_CRC_POST] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_3, 16, 23),
	[MCSC_F_YUV_POSTCONV_PC0_CRC_CONV_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_3, 24, 31),
	[MCSC_F_YUV_POSTCONV_PC0_CRC_CONV_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_4, 0, 7),
	[MCSC_F_YUV_POSTCONV_PC1_CRC_CONV_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_4, 8, 15),
	[MCSC_F_YUV_POSTCONV_PC1_CRC_CONV_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_4, 16, 23),
	[MCSC_F_YUV_POSTCONV_PC2_CRC_CONV_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_4, 24, 31),
	[MCSC_F_YUV_POSTCONV_PC2_CRC_CONV_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_5, 0, 7),
	[MCSC_F_YUV_POSTCONV_PC3_CRC_CONV_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_5, 8, 15),
	[MCSC_F_YUV_POSTCONV_PC3_CRC_CONV_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_5, 16, 23),
	[MCSC_F_YUV_POSTCONV_PC4_CRC_CONV_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_5, 24, 31),
	[MCSC_F_YUV_POSTCONV_PC4_CRC_CONV_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_6, 0, 7),
	[MCSC_F_YUV_POSTBCHS_PC0_CRC_BCHS_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_6, 8, 15),
	[MCSC_F_YUV_POSTBCHS_PC0_CRC_BCHS_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_6, 16, 23),
	[MCSC_F_YUV_POSTBCHS_PC1_CRC_BCHS_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_6, 24, 31),
	[MCSC_F_YUV_POSTBCHS_PC1_CRC_BCHS_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_7, 0, 7),
	[MCSC_F_YUV_POSTBCHS_PC2_CRC_BCHS_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_7, 8, 15),
	[MCSC_F_YUV_POSTBCHS_PC2_CRC_BCHS_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_7, 16, 23),
	[MCSC_F_YUV_POSTBCHS_PC3_CRC_BCHS_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_7, 24, 31),
	[MCSC_F_YUV_POSTBCHS_PC3_CRC_BCHS_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_8, 0, 7),
	[MCSC_F_YUV_POSTBCHS_PC4_CRC_BCHS_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_8, 8, 15),
	[MCSC_F_YUV_POSTBCHS_PC4_CRC_BCHS_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_8, 16, 23),
	[MCSC_F_YUV_WDMASC_W0_CRC_CMN_WDMA_IN_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_8, 24, 31),
	[MCSC_F_YUV_WDMASC_W0_CRC_CMN_WDMA_IN_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_9, 0, 7),
	[MCSC_F_YUV_WDMASC_W1_CRC_CMN_WDMA_IN_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_9, 8, 15),
	[MCSC_F_YUV_WDMASC_W1_CRC_CMN_WDMA_IN_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_9, 16, 23),
	[MCSC_F_YUV_WDMASC_W2_CRC_CMN_WDMA_IN_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_9, 24, 31),
	[MCSC_F_YUV_WDMASC_W2_CRC_CMN_WDMA_IN_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_10, 0, 7),
	[MCSC_F_YUV_WDMASC_W3_CRC_CMN_WDMA_IN_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_10, 8, 15),
	[MCSC_F_YUV_WDMASC_W3_CRC_CMN_WDMA_IN_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_10, 16, 23),
	[MCSC_F_YUV_WDMASC_W4_CRC_CMN_WDMA_IN_LUMA] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_10, 24, 31),
	[MCSC_F_YUV_WDMASC_W4_CRC_CMN_WDMA_IN_CHR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_11, 0, 7),
	[MCSC_F_CRC_AXI_RDMA_SFR] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_12, 0, 7),
	[MCSC_F_DJAG_SIRC_0] = PMIO_FIELD_DESC(MCSC_R_CRC_RESULT_13, 0, 7),
	[MCSC_F_DBG_SC_35] = PMIO_FIELD_DESC(MCSC_R_DBG_SC_35, 0, 31),
};

static const struct pmio_range mcsc_readable_ranges[] = {
	pmio_reg_range(MCSC_R_CMDQ_ENABLE, MCSC_R_CMDQ_STOP_CRPT_ENABLE),
	pmio_reg_range(MCSC_R_TRANS_STOP_REQ, MCSC_R_ICTRL_CSUB_BASE_ADDR),
	pmio_reg_range(MCSC_R_ICTRL_CSUB_CONNECTION_TEST_MSG, MCSC_R_CMDQ_QUE_CMD_L),
	pmio_reg_range(MCSC_R_CMDQ_AUTO_CONV_ENABLE, MCSC_R_CMDQ_SETSEL),
	pmio_reg_range(MCSC_R_CMDQ_DEBUG_STATUS_PRE_LOAD, MCSC_R_CMDQ_FRAME_COUNTER_INC_TYPE),
	pmio_reg_range(MCSC_R_CMDQ_FRAME_COUNTER, MCSC_R_CMDQ_INT_STATUS),
	pmio_reg_range(MCSC_R_C_LOADER_ENABLE, MCSC_R_C_LOADER_ENABLE),
	pmio_reg_range(MCSC_R_C_LOADER_FAST_MODE, MCSC_R_COREX_ENABLE),
	pmio_reg_range(MCSC_R_COREX_FAST_MODE, MCSC_R_COREX_UPDATE_MODE_1),
	pmio_reg_range(MCSC_R_COREX_STATUS_0, MCSC_R_COREX_PRE_POST_CONFIG_EN),
	pmio_reg_range(MCSC_R_COREX_TYPE_READ, MCSC_R_COREX_INT_STATUS),
	pmio_reg_range(MCSC_R_COREX_INT_ENABLE, MCSC_R_INT_REQ_INT0_STATUS),
	pmio_reg_range(MCSC_R_INT_REQ_INT1, MCSC_R_INT_REQ_INT1_STATUS),
	pmio_reg_range(MCSC_R_INT_HIST_CURINT0, MCSC_R_PERF_MONITOR_ENABLE),
	pmio_reg_range(MCSC_R_PERF_MONITOR_INT_USER_SEL, MCSC_R_SFR_ACCESS_LOG_ENABLE),
	pmio_reg_range(MCSC_R_SFR_ACCESS_LOG_0, MCSC_R_SFR_ACCESS_LOG_3_ADDRESS),
	pmio_reg_range(MCSC_R_IP_ROL_MODE, MCSC_R_IP_INT_ON_COL_ROW_POS),
	pmio_reg_range(MCSC_R_FREEZE_EXTENSION_ENABLE, MCSC_R_YUV_CINFIFO_INT_STATUS),
	pmio_reg_range(
		MCSC_R_YUV_CINFIFO_CORRUPTED_COND_ENABLE, MCSC_R_STAT_CINFIFODUALLAYER_INT_STATUS),
	pmio_reg_range(MCSC_R_STAT_CINFIFODUALLAYER_CORRUPTED_COND_ENABLE,
		MCSC_R_YUV_POSTPC_PC2_UV_H_COEFF_8_23),
	pmio_reg_range(MCSC_R_YUV_HWFC_MODE, MCSC_R_YUV_HWFC_FRAME_START_SELECT),
};

static const struct pmio_access_table mcsc_readable_ranges_table = {
	.yes_ranges = mcsc_readable_ranges,
	.n_yes_ranges = ARRAY_SIZE(mcsc_readable_ranges),
	.no_ranges = mcsc_readable_ranges,
	.n_no_ranges = ARRAY_SIZE(mcsc_readable_ranges),
};

static const struct pmio_range mcsc_writable_ranges[] = {
	pmio_reg_range(MCSC_R_CMDQ_ENABLE, MCSC_R_TRANS_STOP_REQ),
	pmio_reg_range(MCSC_R_IP_APG_MODE, MCSC_R_CMDQ_CTRL_SETSEL_EN),
	pmio_reg_range(MCSC_R_CMDQ_FLUSH_QUEUE_0, MCSC_R_CMDQ_HOLD_MARK_QUEUE_0),
	pmio_reg_range(MCSC_R_CMDQ_VHD_CONTROL, MCSC_R_CMDQ_FRAME_COUNTER_RESET),
	pmio_reg_range(MCSC_R_CMDQ_QUEUE_0_RPTR_FOR_DEBUG, MCSC_R_CMDQ_QUEUE_0_RPTR_FOR_DEBUG),
	pmio_reg_range(MCSC_R_CMDQ_INT_ENABLE, MCSC_R_CMDQ_INT_ENABLE),
	pmio_reg_range(MCSC_R_CMDQ_INT_CLEAR, MCSC_R_C_LOADER_LOGICAL_OFFSET),
	pmio_reg_range(MCSC_R_C_LOADER_HEADER_CRC_SEED, MCSC_R_C_LOADER_PAYLOAD_CRC_SEED),
	pmio_reg_range(MCSC_R_COREX_ENABLE, MCSC_R_COREX_COPY_FROM_IP_1),
	pmio_reg_range(MCSC_R_COREX_PRE_ADDR_CONFIG, MCSC_R_COREX_TYPE_WRITE_TRIGGER),
	pmio_reg_range(MCSC_R_COREX_TYPE_READ_OFFSET, MCSC_R_COREX_TYPE_READ_OFFSET),
	pmio_reg_range(MCSC_R_COREX_INT_CLEAR, MCSC_R_COREX_INT_ENABLE),
	pmio_reg_range(MCSC_R_INT_REQ_INT0_ENABLE, MCSC_R_INT_REQ_INT0_ENABLE),
	pmio_reg_range(MCSC_R_INT_REQ_INT0_CLEAR, MCSC_R_INT_REQ_INT0_CLEAR),
	pmio_reg_range(MCSC_R_INT_REQ_INT1_ENABLE, MCSC_R_INT_REQ_INT1_ENABLE),
	pmio_reg_range(MCSC_R_INT_REQ_INT1_CLEAR, MCSC_R_INT_REQ_INT1_CLEAR),
	pmio_reg_range(MCSC_R_INT_HIST_CURINT0_ENABLE, MCSC_R_INT_HIST_CURINT0_ENABLE),
	pmio_reg_range(MCSC_R_INT_HIST_CURINT1_ENABLE, MCSC_R_INT_HIST_CURINT1_ENABLE),
	pmio_reg_range(MCSC_R_SECU_CTRL_SEQID, MCSC_R_PERF_MONITOR_INT_USER_SEL),
	pmio_reg_range(MCSC_R_DEBUG_COUNTER_SIG_SEL, MCSC_R_DEBUG_COUNTER_SIG_SEL),
	pmio_reg_range(MCSC_R_STOPEN_CRC_STOP_VALID_COUNT, MCSC_R_SFR_ACCESS_LOG_0),
	pmio_reg_range(MCSC_R_SFR_ACCESS_LOG_0_ADDRESS, MCSC_R_SFR_ACCESS_LOG_1),
	pmio_reg_range(MCSC_R_SFR_ACCESS_LOG_1_ADDRESS, MCSC_R_SFR_ACCESS_LOG_2),
	pmio_reg_range(MCSC_R_SFR_ACCESS_LOG_2_ADDRESS, MCSC_R_SFR_ACCESS_LOG_3),
	pmio_reg_range(MCSC_R_SFR_ACCESS_LOG_3_ADDRESS, MCSC_R_YUV_CINFIFO_INTERVALS),
	pmio_reg_range(MCSC_R_YUV_CINFIFO_INT_ENABLE, MCSC_R_YUV_CINFIFO_INT_ENABLE),
	pmio_reg_range(MCSC_R_YUV_CINFIFO_INT_CLEAR, MCSC_R_YUV_CINFIFO_STREAM_CRC),
	pmio_reg_range(MCSC_R_STAT_CINFIFODUALLAYER_ENABLE, MCSC_R_STAT_CINFIFODUALLAYER_INTERVALS),
	pmio_reg_range(
		MCSC_R_STAT_CINFIFODUALLAYER_INT_ENABLE, MCSC_R_STAT_CINFIFODUALLAYER_INT_ENABLE),
	pmio_reg_range(
		MCSC_R_STAT_CINFIFODUALLAYER_INT_CLEAR, MCSC_R_STAT_CINFIFODUALLAYER_STREAM_CRC),
	pmio_reg_range(MCSC_R_STAT_RDMACL_ENABLE, MCSC_R_STAT_RDMACL_IMG_BASE_ADDR_1P_FRO7_LSB_4B),
	pmio_reg_range(MCSC_R_STAT_RDMACL_AXI_DEBUG_CONTROL,
		MCSC_R_YUV_WDMASC_W0_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W0_BW_LIMIT_0, MCSC_R_YUV_WDMASC_W0_DEBUG_CONTROL),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W0_FLIP_CONTROL,
		MCSC_R_YUV_WDMASC_W1_HEADER_BASE_ADDR_2P_FRO_LSB_4B_0_7),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W1_BW_LIMIT_0, MCSC_R_YUV_WDMASC_W1_DEBUG_CONTROL),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W1_FLIP_CONTROL,
		MCSC_R_YUV_WDMASC_W2_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W2_BW_LIMIT_0, MCSC_R_YUV_WDMASC_W2_DEBUG_CONTROL),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W2_FLIP_CONTROL,
		MCSC_R_YUV_WDMASC_W3_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W3_BW_LIMIT_0, MCSC_R_YUV_WDMASC_W3_DEBUG_CONTROL),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W3_FLIP_CONTROL,
		MCSC_R_YUV_WDMASC_W4_IMG_BASE_ADDR_3P_FRO_LSB_4B_0_7),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W4_BW_LIMIT_0, MCSC_R_YUV_WDMASC_W4_DEBUG_CONTROL),
	pmio_reg_range(MCSC_R_YUV_WDMASC_W4_FLIP_CONTROL, MCSC_R_YUV_DUALLAYERRECOMP_STREAM_CRC),
	pmio_reg_range(MCSC_R_YUV_POLY_SC0_CTRL, MCSC_R_YUV_HWFC_MODE),
	pmio_reg_range(MCSC_R_YUV_HWFC_CONFIG_IMAGE_A, MCSC_R_YUV_DTPOTFIN_STREAM_CRC),
	pmio_reg_range(MCSC_R_STAT_DTPDUALLAYER_BYPASS, MCSC_R_STAT_DTPDUALLAYER_STREAM_CRC),
};

static const struct pmio_access_table mcsc_writable_ranges_table = {
	.yes_ranges = mcsc_writable_ranges,
	.n_yes_ranges = ARRAY_SIZE(mcsc_writable_ranges),
};

static const struct pmio_range mcsc_volatile_ranges[] = {
	pmio_reg_range(MCSC_R_SW_RESET, MCSC_R_SW_APB_RESET),
	pmio_reg_range(MCSC_R_STAT_RDMACL_ENABLE, MCSC_R_STAT_RDMACL_ENABLE),
};

static const struct pmio_access_table mcsc_volatile_ranges_table = {
	.yes_ranges = mcsc_volatile_ranges,
	.n_yes_ranges = ARRAY_SIZE(mcsc_volatile_ranges),
};

static const struct pmio_range mcsc_cacheable_ranges[] = {};

static const struct pmio_access_table mcsc_cacheable_ranges_table = {
	.yes_ranges = mcsc_cacheable_ranges,
	.n_yes_ranges = ARRAY_SIZE(mcsc_cacheable_ranges),
};

static const struct pmio_range mcsc_none_ranges[] = {};

static const struct pmio_access_table mcsc_none_ranges_table = {
	.yes_ranges = mcsc_none_ranges,
	.n_yes_ranges = ARRAY_SIZE(mcsc_none_ranges),
};

#endif
