// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2023 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "pablo-kunit-test.h"

#include "is-type.h"
#include "pablo-hw-api-common.h"
#include "hardware/api/is-hw-api-rgbp-v2_0.h"
#include "hardware/sfr/is-sfr-rgbp-v2_20.h"

#include "hardware/is-hw-control.h"
#include "include/is-hw.h"

#define HBLANK_CYCLE 0x2D
#define VBLANK_CYCLE 0xA
#define RGBP_LUT_REG_CNT 1650

#define RGBP_SET_F(base, R, F, val) PMIO_SET_F(base, R, F, val)
#define RGBP_SET_R(base, R, val) PMIO_SET_R(base, R, val)
#define RGBP_SET_V(base, reg_val, F, val) PMIO_SET_V(base, reg_val, F, val)

#define RGBP_GET_F(base, R, F) PMIO_GET_F(base, R, F)
#define RGBP_GET_R(base, R) PMIO_GET_R(base, R)

#define RGBP_SIZE_TEST_SET 3

static struct rgbp_test_ctx {
	void *test_addr;
	struct pmio_config pmio_config;
	struct pablo_mmio *pmio;
	struct pmio_field *pmio_fields;
	struct is_common_dma dma;
	struct rgbp_param_set param_set;
	struct rgbp_param rgbp_param;
	struct is_frame frame;
} test_ctx;

static struct is_rectangle rgbp_size_test_config[RGBP_SIZE_TEST_SET] = {
	{ 128, 28 }, /* min size */
	{ 8192, 12248 }, /* max size */
	{ 4032, 3024 }, /* customized size */
};

static void pablo_hw_api_rgbp_hw_s_dtp_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 set_val, expected_val;

	rgbp_hw_s_dtp(test_ctx.pmio, set_id, 0);

	set_val = RGBP_GET_R(test_ctx.pmio, RGBP_R_BYR_DTP_TEST_PATTERN_MODE);
	expected_val = 0;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_hw_is_occurred0_kunit_test(struct kunit *test)
{
	u32 ret, status;
	int test_idx;
	u32 err_interrupt_list[] = {
		INTR0_RGBP_CMDQ_ERROR_INT,
		INTR0_RGBP_C_LOADER_ERROR_INT,
		INTR0_RGBP_COREX_ERROR_INT,
		INTR0_RGBP_CINFIFO_0_OVERFLOW_ERROR_INT,
		INTR0_RGBP_CINFIFO_0_OVERLAP_ERROR_INT,
		INTR0_RGBP_CINFIFO_0_PIXEL_CNT_ERROR_INT,
		INTR0_RGBP_CINFIFO_0_INPUT_PROTOCOL_ERROR_INT,
		INTR0_RGBP_COUTFIFO_0_PIXEL_CNT_ERROR_INT,
		INTR0_RGBP_COUTFIFO_0_INPUT_PROTOCOL_ERROR_INT,
		INTR0_RGBP_COUTFIFO_0_OVERLAP_ERROR_INT,
		INTR0_RGBP_VOTF_GLOBAL_ERROR_INT,
		INTR0_RGBP_VOTFLOSTCONNECTION_INT,
		INTR0_RGBP_OTF_SEQ_ID_ERROR_INT,
	};

	status = 1 << INTR0_RGBP_FRAME_START_INT;
	ret = rgbp_hw_is_occurred0(status, INTR0_FRAME_START_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR0_RGBP_FRAME_END_INT;
	ret = rgbp_hw_is_occurred0(status, INTR0_FRAME_END_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR0_RGBP_CMDQ_HOLD_INT;
	ret = rgbp_hw_is_occurred0(status, INTR0_CMDQ_HOLD_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR0_RGBP_SETTING_DONE_INT;
	ret = rgbp_hw_is_occurred0(status, INTR0_SETTING_DONE_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR0_RGBP_C_LOADER_END_INT;
	ret = rgbp_hw_is_occurred0(status, INTR0_C_LOADER_END_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR0_RGBP_COREX_END_INT_0;
	ret = rgbp_hw_is_occurred0(status, INTR0_COREX_END_INT_0);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR0_RGBP_COREX_END_INT_1;
	ret = rgbp_hw_is_occurred0(status, INTR0_COREX_END_INT_1);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR0_RGBP_ROW_COL_INT;
	ret = rgbp_hw_is_occurred0(status, INTR0_ROW_COL_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR0_RGBP_TRANS_STOP_DONE_INT;
	ret = rgbp_hw_is_occurred0(status, INTR0_TRANS_STOP_DONE_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	for (test_idx = 0; test_idx < ARRAY_SIZE(err_interrupt_list); test_idx++) {
		status = 1 << err_interrupt_list[test_idx];
		ret = rgbp_hw_is_occurred0(status, INTR0_ERR0);
		KUNIT_EXPECT_EQ(test, ret, status);
	}
}

static void pablo_hw_api_rgbp_hw_is_occurred1_kunit_test(struct kunit *test)
{
	u32 ret, status;
	int test_idx;
	u32 err_interrupt_list[] = {
		INTR1_RGBP_VOTFLOSTFLUSH_INT,
		INTR1_RGBP_VOTF0RDMAGLOBALERROR_INT,
		INTR1_RGBP_VOTF0RDMALOSTCONNECTION_INT,
		INTR1_RGBP_VOTF0RDMALOSTFLUSH_INT,
		INTR1_RGBP_VOTF1WDMALOSTFLUSH_INT,
		INTR1_RGBP_DTP_DBG_CNT_ERROR_INT,
		INTR1_RGBP_TDMSC_DBG_CNT_ERROR_INT,
		INTR1_RGBP_DMSC_DBG_CNT_ERROR_INT,
		INTR1_RGBP_POST_GAMMA_DBG_CNT_ERROR_INT,
		INTR1_RGBP_LSC_DBG_CNT_ERROR_INT,
		INTR1_RGBP_GTM_DBG_CNT_ERROR_INT,
		INTR1_RGBP_CCM_DBG_CNT_ERROR_INT,
		INTR1_RGBP_RGB_GAMMA_DBG_CNT_ERROR_INT,
		INTR1_RGBP_RGB2YUV_DBG_CNT_ERROR_INT,
		INTR1_RGBP_YUV444TO422_DBG_CNT_ERROR_INT,
		INTR1_RGBP_SATFLAG_DBG_CNT_ERROR_INT,
		INTR1_RGBP_DECOMP_DBG_CNT_ERROR_INT,
		INTR1_RGBP_YUVSC_DBG_CNT_ERROR_INT,
		INTR1_RGBP_PPC_CONV_DBG_CNT_ERROR_INT,
		INTR1_RGBP_SYNC_MERGE_COUTFIFO_DBG_CNT_ERROR_INT,
	};

	status = 1 << INTR1_RGBP_VOTFLOSTFLUSH_INT;
	ret = rgbp_hw_is_occurred1(status, INTR1_VOTFLOSTFLUSH_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR1_RGBP_VOTF0RDMALOSTCONNECTION_INT;
	ret = rgbp_hw_is_occurred1(status, INTR1_VOTF0RDMALOSTCONNECTION_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR1_RGBP_VOTF0RDMALOSTFLUSH_INT;
	ret = rgbp_hw_is_occurred1(status, INTR1_VOTF0RDMALOSTFLUSH_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	status = 1 << INTR1_RGBP_VOTF1WDMALOSTFLUSH_INT;
	ret = rgbp_hw_is_occurred1(status, INTR1_VOTF1WDMALOSTFLUSH_INT);
	KUNIT_EXPECT_EQ(test, ret, status);

	for (test_idx = 0; test_idx < ARRAY_SIZE(err_interrupt_list); test_idx++) {
		status = 1 << err_interrupt_list[test_idx];
		ret = rgbp_hw_is_occurred1(status, INTR1_ERR1);
		KUNIT_EXPECT_EQ(test, ret, status);
	}
}

static void pablo_hw_api_rgbp_hw_s_reset_kunit_test(struct kunit *test)
{
	int ret;

	ret = rgbp_hw_s_reset(test_ctx.pmio);

	KUNIT_EXPECT_EQ(test, ret, RGBP_TRY_COUNT + 1); /* timeout */
}

static void pablo_hw_api_rgbp_hw_s_init_kunit_test(struct kunit *test)
{
	u32 set_val, expected_val;

	rgbp_hw_s_init(test_ctx.pmio);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_L,
			     RGBP_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE);
	expected_val = RGBP_INT_GRP_EN_MASK;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val =
		RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_M, RGBP_F_CMDQ_QUE_CMD_SETTING_MODE);
	expected_val = 3;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_R(test_ctx.pmio, RGBP_R_CMDQ_ENABLE);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_hw_wait_idle_kunit_test(struct kunit *test)
{
	int ret;

	ret = rgbp_hw_wait_idle(test_ctx.pmio);

	KUNIT_EXPECT_EQ(test, ret, -ETIME);
}

static void pablo_hw_api_rgbp_hw_dump_kunit_test(struct kunit *test)
{
	rgbp_hw_dump(test_ctx.pmio, HW_DUMP_CR);
	rgbp_hw_dump(test_ctx.pmio, HW_DUMP_DBG_STATE);
	rgbp_hw_dump(test_ctx.pmio, HW_DUMP_MODE_NUM);
}

static void pablo_hw_api_rgbp_check_cin_fifo(struct kunit *test)
{
	u32 set_val, expected_val;

	set_val =
		RGBP_GET_F(test_ctx.pmio, RGBP_R_IP_USE_OTF_PATH, RGBP_F_IP_USE_OTF_OUT_FOR_PATH_0);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_INTERVALS,
			     RGBP_F_YUV_COUTFIFO_INTERVAL_HBLANK);
	expected_val = HBLANK_CYCLE;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_CONFIG,
			     RGBP_F_YUV_COUTFIFO_VVALID_RISE_AT_FIRST_DATA_EN);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val =
		RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_CONFIG, RGBP_F_YUV_COUTFIFO_DEBUG_EN);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_CONFIG,
			     RGBP_F_YUV_COUTFIFO_BACK_STALL_EN);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_ENABLE, RGBP_F_YUV_COUTFIFO_ENABLE);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_check_cout_fifo(struct kunit *test)
{
	struct rgbp_param_set *rgbp_param_set = &test_ctx.param_set;
	u32 set_val, expected_val;
	u32 otf_output = rgbp_param_set->otf_output.cmd;

	set_val =
		RGBP_GET_F(test_ctx.pmio, RGBP_R_IP_USE_OTF_PATH, RGBP_F_IP_USE_OTF_OUT_FOR_PATH_0);
	KUNIT_EXPECT_EQ(test, set_val, otf_output);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_INTERVALS,
			     RGBP_F_YUV_COUTFIFO_INTERVAL_HBLANK);
	expected_val = HBLANK_CYCLE;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_INTERVAL_VBLANK,
			     RGBP_F_YUV_COUTFIFO_INTERVAL_VBLANK);
	expected_val = VBLANK_CYCLE;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_CONFIG,
			     RGBP_F_YUV_COUTFIFO_VVALID_RISE_AT_FIRST_DATA_EN);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val =
		RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_CONFIG, RGBP_F_YUV_COUTFIFO_DEBUG_EN);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_CONFIG,
			     RGBP_F_YUV_COUTFIFO_BACK_STALL_EN);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_COUTFIFO_ENABLE, RGBP_F_YUV_COUTFIFO_ENABLE);
	KUNIT_EXPECT_EQ(test, set_val, otf_output);
}

static void pablo_hw_api_rgbp_check_common(struct kunit *test)
{
	u32 set_val;

	set_val = RGBP_GET_R(test_ctx.pmio, RGBP_R_AUTO_IGNORE_INTERRUPT_ENABLE);
	KUNIT_EXPECT_EQ(test, set_val, 1);
}

static void pablo_hw_api_rgbp_check_int_mask(struct kunit *test)
{
	u32 set_val, expected_val;

	set_val = RGBP_GET_R(test_ctx.pmio, RGBP_R_INT_REQ_INT0_ENABLE);
	expected_val = RGBP_INT0_EN_MASK;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_R(test_ctx.pmio, RGBP_R_INT_REQ_INT1_ENABLE);
	expected_val = RGBP_INT1_EN_MASK;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_check_path(struct kunit *test)
{
	u32 set_val, expected_val;

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_MUX_SELECT, RGBP_F_MUX_DTP_SELECT);
	expected_val = 0;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_MUX_SELECT, RGBP_F_MUX_POSTGAMMA_SELECT);
	expected_val = 0;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_MUX_SELECT, RGBP_F_MUX_WDMA_REP_SELECT);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_DEMUX_ENABLE, RGBP_F_DEMUX_DMSC_ENABLE);
	expected_val = 3;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_DEMUX_ENABLE, RGBP_F_DEMUX_YUVSC_ENABLE);
	expected_val = 3;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val =
		RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_SATFLAG_ENABLE, RGBP_F_CHAIN_SATFLAG_ENABLE);
	expected_val = 0;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_check_pixel_order(struct kunit *test)
{
	u32 set_val, expected_val = 0;

	set_val = RGBP_GET_R(test_ctx.pmio, RGBP_R_BYR_DTP_PIXEL_ORDER);
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_R(test_ctx.pmio, RGBP_R_BYR_DMSC_PIXEL_ORDER);
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_R(test_ctx.pmio, RGBP_R_BYR_SATFLAG_PIXEL_ORDER);
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_check_secure_id(struct kunit *test)
{
	u32 set_val, expected_val;

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_SECU_CTRL_SEQID, RGBP_F_SECU_CTRL_SEQID);
	expected_val = 0;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_hw_s_core_kunit_test(struct kunit *test)
{
	u32 num_buffers = 1;
	u32 set_id = COREX_DIRECT;
	struct is_rgbp_config config;
	struct rgbp_param_set *rgbp_param_set = &test_ctx.param_set;

	rgbp_param_set->otf_input.cmd = 1;
	rgbp_param_set->otf_output.cmd = 1;

	is_set_debug_param(IS_DEBUG_PARAM_CRC_SEED, 55);

	rgbp_hw_s_core(test_ctx.pmio, num_buffers, set_id, &config, rgbp_param_set);

	is_set_debug_param(IS_DEBUG_PARAM_CRC_SEED, 0);

	/* rgbp_hw_s_cin_fifo() */
	pablo_hw_api_rgbp_check_cin_fifo(test);
	/* rgbp_hw_s_cout_fifo() */
	pablo_hw_api_rgbp_check_cout_fifo(test);
	/* rgbp_hw_s_common() */
	pablo_hw_api_rgbp_check_common(test);
	/* rgbp_hw_s_int_mask() */
	pablo_hw_api_rgbp_check_int_mask(test);
	/* rgbp_hw_s_path() */
	pablo_hw_api_rgbp_check_path(test);
	/* rgbp_hw_s_pixel_order() */
	pablo_hw_api_rgbp_check_pixel_order(test);
	/* rgbp_hw_s_secure_id() */
	pablo_hw_api_rgbp_check_secure_id(test);
}

static void pablo_hw_api_rgbp_hw_dma_dump_kunit_test(struct kunit *test)
{
	rgbp_hw_dma_dump(&test_ctx.dma);
}

static void pablo_hw_api_rgbp_hw_g_output_dva_kunit_test(struct kunit *test)
{
	struct rgbp_param_set *param_set = &test_ctx.param_set;
	pdma_addr_t *output_dva;
	u32 instance = 0;
	u32 id;
	u32 cmd;
	u32 out_yuv;

	id = RGBP_WDMA_HF;
	output_dva = rgbp_hw_g_output_dva(param_set, instance, id, &cmd, &out_yuv);
	KUNIT_EXPECT_EQ(test, cmd, param_set->dma_output_hf.cmd);
	KUNIT_EXPECT_PTR_EQ(test, output_dva, (pdma_addr_t *)param_set->output_dva_hf);

	id = RGBP_WDMA_SF;
	output_dva = rgbp_hw_g_output_dva(param_set, instance, id, &cmd, &out_yuv);
	KUNIT_EXPECT_EQ(test, cmd, param_set->dma_output_sf.cmd);
	KUNIT_EXPECT_PTR_EQ(test, output_dva, (pdma_addr_t *)param_set->output_dva_sf);

	id = RGBP_WDMA_Y;
	param_set->dma_output_yuv.cmd = 1;
	output_dva = rgbp_hw_g_output_dva(param_set, instance, id, &cmd, &out_yuv);
	KUNIT_EXPECT_EQ(test, cmd, param_set->dma_output_yuv.cmd);
	KUNIT_EXPECT_PTR_EQ(test, output_dva, (pdma_addr_t *)param_set->output_dva_yuv);

	id = RGBP_WDMA_UV;
	output_dva = rgbp_hw_g_output_dva(param_set, instance, id, &cmd, &out_yuv);
	KUNIT_EXPECT_EQ(test, cmd, param_set->dma_output_yuv.cmd);
	KUNIT_EXPECT_PTR_EQ(test, output_dva, (pdma_addr_t *)param_set->output_dva_yuv);

	id = RGBP_WDMA_REPB;
	output_dva = rgbp_hw_g_output_dva(param_set, instance, id, &cmd, &out_yuv);
	KUNIT_EXPECT_EQ(test, cmd, param_set->dma_output_rgb.cmd);
	KUNIT_EXPECT_PTR_EQ(test, output_dva, (pdma_addr_t *)param_set->output_dva_rgb);

	id = RGBP_WDMA_MAX;
	output_dva = rgbp_hw_g_output_dva(param_set, instance, id, &cmd, &out_yuv);
	KUNIT_EXPECT_PTR_EQ(test, output_dva, (pdma_addr_t *)NULL);
}

static void pablo_hw_api_rgbp_hw_s_rdma_corex_id_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;

	rgbp_hw_s_rdma_corex_id(&test_ctx.dma, set_id);
}

static void pablo_hw_api_rgbp_hw_rdma_create_kunit_test(struct kunit *test)
{
	int ret;
	u32 dma_id = 0;
	int test_idx;

	for (test_idx = RGBP_RDMA_BYR; test_idx < RGBP_RDMA_MAX; test_idx++) {
		dma_id = test_idx;

		ret = rgbp_hw_rdma_create(&test_ctx.dma, test_ctx.pmio, dma_id);
		KUNIT_EXPECT_EQ(test, ret, 0);
	}

	/* DMA id err test */
	dma_id = -1;
	ret = rgbp_hw_rdma_create(&test_ctx.dma, test_ctx.pmio, dma_id);
	KUNIT_EXPECT_EQ(test, ret, SET_ERROR);
}

static void pablo_hw_api_rgbp_hw_s_rdma_addr_kunit_test(struct kunit *test)
{
	int ret;
	pdma_addr_t addr = 0;
	u32 plane = 0;
	u32 num_buffers = 0;
	int buf_idx = 0;
	u32 comp_sbwc_en = 0;
	u32 payload_size = 0;
	u32 in_rgb = 0;

	ret = rgbp_hw_s_rdma_addr(&test_ctx.dma, &addr, plane, num_buffers, buf_idx, comp_sbwc_en,
				  payload_size, in_rgb);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_api_rgbp_hw_s_wdma_corex_id_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;

	rgbp_hw_s_wdma_corex_id(&test_ctx.dma, set_id);
}

static void pablo_hw_api_rgbp_hw_wdma_create_kunit_test(struct kunit *test)
{
	int ret;
	u32 dma_id = 0;
	int test_idx;

	for (test_idx = RGBP_WDMA_HF; test_idx < RGBP_WDMA_MAX; test_idx++) {
		dma_id = test_idx;

		ret = rgbp_hw_wdma_create(&test_ctx.dma, test_ctx.pmio, dma_id);
		KUNIT_EXPECT_EQ(test, ret, 0);
	}

	/* DMA id err test */
	dma_id = -1;
	ret = rgbp_hw_rdma_create(&test_ctx.dma, test_ctx.pmio, dma_id);
	KUNIT_EXPECT_EQ(test, ret, SET_ERROR);
}

static void pablo_hw_api_rgbp_hw_s_wdma_addr_kunit_test(struct kunit *test)
{
	int ret;
	struct is_common_dma dma = {
		0,
	};
	pdma_addr_t addr = 0;
	u32 num_buffers = 0;
	struct rgbp_dma_addr_cfg dma_addr_cfg;

	dma_addr_cfg.sbwc_en = 0;
	dma_addr_cfg.payload_size = 0;
	dma_addr_cfg.out_yuv = 0;

	ret = rgbp_hw_s_wdma_addr(&dma, &addr, num_buffers, &dma_addr_cfg);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_api_rgbp_hw_s_corex_update_type_kunit_test(struct kunit *test)
{
	int ret;
	u32 set_id = COREX_DIRECT;
	u32 type = 0;

	ret = rgbp_hw_s_corex_update_type(test_ctx.pmio, set_id, type);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_api_rgbp_hw_s_cmdq_kunit_test(struct kunit *test)
{
	dma_addr_t clh = 0x12345678;
	u32 set_id = COREX_DIRECT;
	u32 num_buffers = 3;
	u32 noh = 2;
	u32 idx, val;

	rgbp_hw_s_cmdq(test_ctx.pmio, set_id, num_buffers, clh, noh);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_L,
			 RGBP_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE);
	KUNIT_EXPECT_EQ(test, val, RGBP_INT_GRP_EN_MASK_FRO_LAST);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_H, RGBP_F_CMDQ_QUE_CMD_BASE_ADDR);
	KUNIT_EXPECT_EQ(test, val, DVA_36BIT_HIGH(clh));
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_M, RGBP_F_CMDQ_QUE_CMD_HEADER_NUM);
	KUNIT_EXPECT_EQ(test, val, noh);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_M, RGBP_F_CMDQ_QUE_CMD_SETTING_MODE);
	KUNIT_EXPECT_EQ(test, val, 1);
	idx = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_L, RGBP_F_CMDQ_QUE_CMD_FRO_INDEX);
	KUNIT_EXPECT_EQ(test, idx, num_buffers - 1);
	val = RGBP_GET_R(test_ctx.pmio, RGBP_R_CMDQ_ADD_TO_QUEUE_0);
	KUNIT_EXPECT_EQ(test, val, 1);
	val = RGBP_GET_R(test_ctx.pmio, RGBP_R_CMDQ_ENABLE);
	KUNIT_EXPECT_EQ(test, val, 1);

	noh = 0;
	num_buffers = 1;
	rgbp_hw_s_cmdq(test_ctx.pmio, set_id, num_buffers, clh, noh);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_L,
			 RGBP_F_CMDQ_QUE_CMD_INT_GROUP_ENABLE);
	KUNIT_EXPECT_EQ(test, val, RGBP_INT_GRP_EN_MASK);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_M, RGBP_F_CMDQ_QUE_CMD_SETTING_MODE);
	KUNIT_EXPECT_EQ(test, val, 3);
	idx = RGBP_GET_F(test_ctx.pmio, RGBP_R_CMDQ_QUE_CMD_L, RGBP_F_CMDQ_QUE_CMD_FRO_INDEX);
	KUNIT_EXPECT_EQ(test, idx, num_buffers - 1);
	val = RGBP_GET_R(test_ctx.pmio, RGBP_R_CMDQ_ADD_TO_QUEUE_0);
	KUNIT_EXPECT_EQ(test, val, 1);
	val = RGBP_GET_R(test_ctx.pmio, RGBP_R_CMDQ_ENABLE);
	KUNIT_EXPECT_EQ(test, val, 1);
}

static void pablo_hw_api_rgbp_hw_s_corex_init_kunit_test(struct kunit *test)
{
	bool enable = false;

	rgbp_hw_s_corex_init(test_ctx.pmio, enable);
}

static void pablo_hw_api_rgbp_hw_wait_corex_idle_kunit_test(struct kunit *test)
{
	rgbp_hw_wait_corex_idle(test_ctx.pmio);
}

static void pablo_hw_api_rgbp_hw_s_corex_start_kunit_test(struct kunit *test)
{
	bool enable = false;

	rgbp_hw_s_corex_start(test_ctx.pmio, enable);
}

static void pablo_hw_api_rgbp_hw_g_int0_state_kunit_test(struct kunit *test)
{
	unsigned int state;
	bool clear = false;
	u32 num_buffers = 0;
	u32 irq_state = 0;

	state = rgbp_hw_g_int0_state(test_ctx.pmio, clear, num_buffers, &irq_state);
	KUNIT_EXPECT_EQ(test, state, (unsigned int)0);
}

static void pablo_hw_api_rgbp_hw_g_int0_mask_kunit_test(struct kunit *test)
{
	unsigned int val;

	RGBP_SET_R(test_ctx.pmio, RGBP_R_INT_REQ_INT0_ENABLE, 0xFF);
	val = rgbp_hw_g_int0_mask(test_ctx.pmio);
	KUNIT_EXPECT_EQ(test, val, 0xFF);
}

static void pablo_hw_api_rgbp_hw_g_int1_state_kunit_test(struct kunit *test)
{
	unsigned int state;
	bool clear = true;
	u32 num_buffers = 0;
	u32 irq_state = 0;
	u32 irq = 0xFFFFFFFF;
	u32 val;

	RGBP_SET_R(test_ctx.pmio, RGBP_R_INT_REQ_INT1, irq);

	state = rgbp_hw_g_int1_state(test_ctx.pmio, clear, num_buffers, &irq_state);
	KUNIT_EXPECT_EQ(test, state, irq);
	val = RGBP_GET_R(test_ctx.pmio, RGBP_R_INT_REQ_INT1_CLEAR);
	KUNIT_EXPECT_EQ(test, val, irq);
	KUNIT_EXPECT_EQ(test, irq_state, irq);
}

static void pablo_hw_api_rgbp_hw_g_int1_mask_kunit_test(struct kunit *test)
{
	unsigned int mask;

	mask = rgbp_hw_g_int1_mask(test_ctx.pmio);
	KUNIT_EXPECT_EQ(test, mask, 0);
}

static void pablo_hw_api_rgbp_hw_s_chain_src_size_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 width, height;
	u32 g_width, g_height;
	int test_idx;

	for (test_idx = 0; test_idx < RGBP_SIZE_TEST_SET; test_idx++) {
		width = rgbp_size_test_config[test_idx].w;
		height = rgbp_size_test_config[test_idx].h;

		rgbp_hw_s_chain_src_size(test_ctx.pmio, set_id, width, height);

		g_width = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_SRC_IMG_SIZE,
				     RGBP_F_CHAIN_SRC_IMG_WIDTH);
		g_height = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_SRC_IMG_SIZE,
				      RGBP_F_CHAIN_SRC_IMG_HEIGHT);

		KUNIT_EXPECT_EQ(test, width, g_width);
		KUNIT_EXPECT_EQ(test, height, g_height);
	}
}

static void pablo_hw_api_rgbp_hw_s_chain_dst_size_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 width, height;
	u32 g_width, g_height;
	int test_idx;

	for (test_idx = 0; test_idx < RGBP_SIZE_TEST_SET; test_idx++) {
		width = rgbp_size_test_config[test_idx].w;
		height = rgbp_size_test_config[test_idx].h;

		rgbp_hw_s_chain_dst_size(test_ctx.pmio, set_id, width, height);

		g_width = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_DST_IMG_SIZE,
				     RGBP_F_CHAIN_DST_IMG_WIDTH);
		g_height = RGBP_GET_F(test_ctx.pmio, RGBP_R_CHAIN_DST_IMG_SIZE,
				      RGBP_F_CHAIN_DST_IMG_HEIGHT);

		KUNIT_EXPECT_EQ(test, width, g_width);
		KUNIT_EXPECT_EQ(test, height, g_height);
	}
}

static void pablo_hw_api_rgbp_hw_s_dtp_output_size_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 width, height;
	u32 g_width, g_height;
	int test_idx;

	for (test_idx = 0; test_idx < RGBP_SIZE_TEST_SET; test_idx++) {
		width = rgbp_size_test_config[test_idx].w;
		height = rgbp_size_test_config[test_idx].h;

		rgbp_hw_s_dtp_output_size(test_ctx.pmio, set_id, width, height);

		g_width = RGBP_GET_F(test_ctx.pmio, RGBP_R_BYR_DTP_X_OUTPUT_SIZE,
				     RGBP_F_BYR_DTP_X_OUTPUT_SIZE);
		g_height = RGBP_GET_F(test_ctx.pmio, RGBP_R_BYR_DTP_Y_OUTPUT_SIZE,
				      RGBP_F_BYR_DTP_Y_OUTPUT_SIZE);

		KUNIT_EXPECT_EQ(test, width, g_width);
		KUNIT_EXPECT_EQ(test, height, g_height);
	}
}

static void pablo_hw_api_rgbp_hw_s_decomp_frame_size_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 width, height;
	u32 g_width, g_height;
	int test_idx;

	for (test_idx = 0; test_idx < RGBP_SIZE_TEST_SET; test_idx++) {
		width = rgbp_size_test_config[test_idx].w;
		height = rgbp_size_test_config[test_idx].h;

		rgbp_hw_s_decomp_frame_size(test_ctx.pmio, set_id, width, height);

		g_width = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_DECOMP_FRAME_SIZE,
				     RGBP_F_Y_DECOMP_FRAME_WIDTH);
		g_height = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_DECOMP_FRAME_SIZE,
				      RGBP_F_Y_DECOMP_FRAME_HEIGHT);

		KUNIT_EXPECT_EQ(test, width, g_width);
		KUNIT_EXPECT_EQ(test, height, g_height);
	}
}

static void pablo_hw_api_rgbp_hw_s_sc_enable_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 enable = 0;
	u32 bypass = 0;
	u32 set_val, expected_val;

	rgbp_hw_s_yuvsc_enable(test_ctx.pmio, set_id, enable, bypass);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_CTRL0, RGBP_F_YUV_SC_TOP_BYPASS);
	expected_val = bypass;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_CTRL0, RGBP_F_YUV_SC_BYPASS);
	expected_val = bypass;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	set_val =
		RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_CTRL1, RGBP_F_YUV_SC_LR_HR_MERGE_SPLIT_ON);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_hw_s_sc_dst_size_size_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 width, height;
	u32 g_width, g_height;
	int test_idx;

	for (test_idx = 0; test_idx < RGBP_SIZE_TEST_SET; test_idx++) {
		width = rgbp_size_test_config[test_idx].w;
		height = rgbp_size_test_config[test_idx].h;

		rgbp_hw_s_sc_dst_size_size(test_ctx.pmio, set_id, width, height);

		g_width =
			RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_DST_SIZE, RGBP_F_YUV_SC_DST_HSIZE);
		g_height =
			RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_DST_SIZE, RGBP_F_YUV_SC_DST_VSIZE);

		KUNIT_EXPECT_EQ(test, width, g_width);
		KUNIT_EXPECT_EQ(test, height, g_height);
	}
}

static void pablo_hw_api_rgbp_hw_s_block_bypass_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 val;
	bool utc_flag;

	rgbp_hw_s_block_bypass(test_ctx.pmio, set_id);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_BYR_DMSC_BYPASS, RGBP_F_BYR_DMSC_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 0);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_POSTGAMMA_BYPASS, RGBP_F_RGB_POSTGAMMA_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = val == 1;
	val = RGBP_GET_F(
		test_ctx.pmio, RGBP_R_RGB_LUMAADAPLSC_BYPASS, RGBP_F_RGB_LUMAADAPLSC_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_GAMMA_BYPASS, RGBP_F_RGB_GAMMA_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_GTM_BYPASS, RGBP_F_RGB_GTM_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_RGBTOYUV_BYPASS, RGBP_F_RGB_RGBTOYUV_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 0);
	utc_flag = (utc_flag && !val);
	val = RGBP_GET_F(
		test_ctx.pmio, RGBP_R_YUV_YUV444TO422_ISP_BYPASS, RGBP_F_YUV_YUV444TO422_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 0);
	utc_flag = (utc_flag && !val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_BYR_SATFLAG_BYPASS, RGBP_F_BYR_SATFLAG_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_DECOMP_BYPASS, RGBP_F_Y_DECOMP_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_GAMMALR_BYPASS, RGBP_F_Y_GAMMALR_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_GAMMAHR_BYPASS, RGBP_F_Y_GAMMAHR_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_CTRL0, RGBP_F_YUV_SC_TOP_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_OTF_CROP_CTRL, RGBP_F_RGB_DMSCCROP_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_DMA_CROP_CTRL, RGBP_F_STAT_DECOMPCROP_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_DMA_CROP_CTRL, RGBP_F_STAT_SATFLGCROP_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_DMA_CROP_CTRL, RGBP_F_YUV_SCCROP_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_TETRA_TDMSC_BYPASS, RGBP_F_TETRA_TDMSC_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_CCM33_BYPASS, RGBP_F_RGB_CCM33_BYPASS);
	KUNIT_EXPECT_EQ(test, val, 1);
	utc_flag = (utc_flag && val);

	set_utc_result(KUTC_RGBP_BYPASS, UTC_ID_RGBP_BYPASS, utc_flag);
}

static void pablo_hw_api_rgbp_is_scaler_get_yuvsc_dst_size_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 h_size = 0;
	u32 v_size = 0;

	rgbp_hw_s_yuvsc_dst_size(test_ctx.pmio, set_id, 1920, 1080);
	rgbp_hw_g_yuvsc_dst_size(test_ctx.pmio, set_id, &h_size, &v_size);
	KUNIT_EXPECT_EQ(test, h_size, 1920);
	KUNIT_EXPECT_EQ(test, v_size, 1080);
}

static void pablo_hw_api_rgbp_hw_s_yuvsc_scaling_ratio_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 hratio = 1;
	u32 vratio = 2;
	u32 val;

	rgbp_hw_s_yuvsc_scaling_ratio(test_ctx.pmio, set_id, hratio, vratio);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_H_RATIO, RGBP_F_YUV_SC_H_RATIO);
	KUNIT_EXPECT_EQ(test, val, 1);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_V_RATIO, RGBP_F_YUV_SC_V_RATIO);
	KUNIT_EXPECT_EQ(test, val, 2);
}

static void pablo_hw_api_rgbp_hw_s_yuvsc_coef_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 val;

	rgbp_hw_s_yuvsc_coef(test_ctx.pmio, set_id, RGBP_RATIO_X8_8, RGBP_RATIO_X8_8);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_H_INIT_PHASE_OFFSET,
			 RGBP_F_YUV_SC_H_INIT_PHASE_OFFSET);
	KUNIT_EXPECT_EQ(test, val, 0);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_V_INIT_PHASE_OFFSET,
			 RGBP_F_YUV_SC_V_INIT_PHASE_OFFSET);
	KUNIT_EXPECT_EQ(test, val, 0);
}

static void pablo_hw_api_rgbp_hw_s_upsc_dst_size_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 width, height;
	u32 h_size, v_size;
	int test_idx;

	for (test_idx = 0; test_idx < RGBP_SIZE_TEST_SET; test_idx++) {
		width = rgbp_size_test_config[test_idx].w;
		height = rgbp_size_test_config[test_idx].h;

		rgbp_hw_s_upsc_dst_size(test_ctx.pmio, set_id, width, height);
		rgbp_hw_g_upsc_dst_size(test_ctx.pmio, set_id, &h_size, &v_size);
		KUNIT_EXPECT_EQ(test, width, h_size);
		KUNIT_EXPECT_EQ(test, height, v_size);
	}
}

static void pablo_hw_api_rgbp_hw_s_upsc_scaling_ratio_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 hratio = 1;
	u32 vratio = 1;
	u32 val;

	rgbp_hw_s_upsc_scaling_ratio(test_ctx.pmio, set_id, hratio, vratio);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_UPSC_H_RATIO, RGBP_F_Y_UPSC_H_RATIO);
	KUNIT_EXPECT_EQ(test, val, hratio);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_UPSC_V_RATIO, RGBP_F_Y_UPSC_V_RATIO);
	KUNIT_EXPECT_EQ(test, val, vratio);
}

static void pablo_hw_api_rgbp_hw_s_h_init_phase_offset_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 offset = 0xFF;
	u32 val;

	rgbp_hw_s_h_init_phase_offset(test_ctx.pmio, set_id, offset);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_H_INIT_PHASE_OFFSET,
			 RGBP_F_YUV_SC_H_INIT_PHASE_OFFSET);
	KUNIT_EXPECT_EQ(test, val, offset);
}

static void pablo_hw_api_rgbp_hw_s_v_init_phase_offset_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 offset = 0xFF;
	u32 val;

	rgbp_hw_s_v_init_phase_offset(test_ctx.pmio, set_id, offset);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_YUV_SC_V_INIT_PHASE_OFFSET,
			 RGBP_F_YUV_SC_V_INIT_PHASE_OFFSET);
	KUNIT_EXPECT_EQ(test, val, offset);
}

static void pablo_hw_api_rgbp_hw_s_upsc_enable_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 enable = 1;
	u32 val;

	rgbp_hw_s_upsc_enable(test_ctx.pmio, set_id, enable, enable);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_UPSC_CTRL0, RGBP_F_Y_UPSC_ENABLE);
	KUNIT_EXPECT_EQ(test, val, enable);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_UPSC_CTRL0, RGBP_F_Y_UPSC_BYPASS);
	KUNIT_EXPECT_EQ(test, val, enable);
}

static void pablo_hw_api_rgbp_hw_s_upsc_coef_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 hratio = 0;
	u32 vratio = 0;

	rgbp_hw_s_upsc_coef(test_ctx.pmio, set_id, hratio, vratio);
}

static void pablo_hw_api_rgbp_hw_s_gamma_enable_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 enable = 1;
	u32 val;

	rgbp_hw_s_gamma_enable(test_ctx.pmio, set_id, enable, enable);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_GAMMALR_BYPASS, RGBP_F_Y_GAMMALR_BYPASS);
	KUNIT_EXPECT_EQ(test, val, enable);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_GAMMAHR_BYPASS, RGBP_F_Y_GAMMAHR_BYPASS);
	KUNIT_EXPECT_EQ(test, val, enable);
}

static void pablo_hw_api_rgbp_hw_s_decomp_enable_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 bypass = 1;
	u32 val;

	rgbp_hw_s_decomp_enable(test_ctx.pmio, set_id, bypass, bypass);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_DECOMP_BYPASS, RGBP_F_Y_DECOMP_BYPASS);
	KUNIT_EXPECT_EQ(test, val, bypass);
}

static void pablo_hw_api_rgbp_hw_s_decomp_size_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 width, height;
	int test_idx;
	u32 g_width, g_height;

	for (test_idx = 0; test_idx < RGBP_SIZE_TEST_SET; test_idx++) {
		width = rgbp_size_test_config[test_idx].w;
		height = rgbp_size_test_config[test_idx].h;

		rgbp_hw_s_decomp_size(test_ctx.pmio, set_id, width, height);

		g_width = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_DECOMP_FRAME_SIZE,
				     RGBP_F_Y_DECOMP_FRAME_WIDTH);
		g_height = RGBP_GET_F(test_ctx.pmio, RGBP_R_Y_DECOMP_FRAME_SIZE,
				      RGBP_F_Y_DECOMP_FRAME_HEIGHT);

		KUNIT_EXPECT_EQ(test, width, g_width);
		KUNIT_EXPECT_EQ(test, height, g_height);
	}
}

static void pablo_hw_api_rgbp_hw_s_grid_cfg_kunit_test(struct kunit *test)
{
	struct rgbp_grid_cfg cfg = { 0, 0, 0, 0, 0, 0, 0, 0 };
	u32 val;

	rgbp_hw_s_grid_cfg(test_ctx.pmio, &cfg);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_LUMAADAPLSC_BINNING,
			 RGBP_F_RGB_LUMAADAPLSC_BINNING_X);
	KUNIT_EXPECT_EQ(test, val, cfg.binning_x);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_LUMAADAPLSC_BINNING,
			 RGBP_F_RGB_LUMAADAPLSC_BINNING_Y);
	KUNIT_EXPECT_EQ(test, val, cfg.binning_y);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_LUMAADAPLSC_CROP_START_X,
			 RGBP_F_RGB_LUMAADAPLSC_CROP_START_X);
	KUNIT_EXPECT_EQ(test, val, cfg.crop_x);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_LUMAADAPLSC_CROP_START_Y,
			 RGBP_F_RGB_LUMAADAPLSC_CROP_START_Y);
	KUNIT_EXPECT_EQ(test, val, cfg.crop_y);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_LUMAADAPLSC_CROP_START_REAL,
			 RGBP_F_RGB_LUMAADAPLSC_CROP_RADIAL_X);
	KUNIT_EXPECT_EQ(test, val, cfg.crop_radial_x);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_RGB_LUMAADAPLSC_CROP_START_REAL,
			 RGBP_F_RGB_LUMAADAPLSC_CROP_RADIAL_Y);
	KUNIT_EXPECT_EQ(test, val, cfg.crop_radial_y);
}

static void pablo_hw_api_rgbp_hw_s_votf_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 enable = 1, stall = 1;
	u32 val;

	rgbp_hw_s_votf(test_ctx.pmio, set_id, enable, stall);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_STAT_WDMADECOMP_VOTF_EN,
			 RGBP_F_STAT_WDMADECOMP_VOTF_EN);
	KUNIT_EXPECT_EQ(test, val & 0x1, enable);
	KUNIT_EXPECT_EQ(test, (val >> 1) & 0x1, stall);
}

static void pablo_hw_api_rgbp_hw_s_sbwc_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	bool enable = false;

	rgbp_hw_s_sbwc(test_ctx.pmio, set_id, enable);
}

static void pablo_hw_api_rgbp_hw_s_crop_kunit_test(struct kunit *test)
{
	rgbp_hw_s_yuvsc_crop(test_ctx.pmio, false, 0, 0, 0, 0);
}

static void pablo_hw_api_rgbp_hw_g_reg_cnt_kunit_test(struct kunit *test)
{
	int ret;

	ret = rgbp_hw_g_reg_cnt();

	KUNIT_EXPECT_EQ(test, ret, RGBP_REG_CNT + RGBP_LUT_REG_CNT);
}

static void pablo_hw_api_rgbp_hw_s_strgen_kunit_test(struct kunit *test)
{
	u32 set_id = COREX_DIRECT;
	u32 val;

	rgbp_hw_s_strgen(test_ctx.pmio, set_id);

	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_BYR_CINFIFO_CONFIG,
			 RGBP_F_BYR_CINFIFO_STRGEN_MODE_EN);
	KUNIT_EXPECT_EQ(test, val, 1);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_BYR_CINFIFO_CONFIG,
			 RGBP_F_BYR_CINFIFO_STRGEN_MODE_DATA_TYPE);
	KUNIT_EXPECT_EQ(test, val, 1);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_BYR_CINFIFO_CONFIG,
			 RGBP_F_BYR_CINFIFO_STRGEN_MODE_DATA);
	KUNIT_EXPECT_EQ(test, val, 255);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_IP_USE_OTF_PATH, RGBP_F_IP_USE_OTF_IN_FOR_PATH_0);
	KUNIT_EXPECT_EQ(test, val, 1);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_IP_USE_CINFIFO_NEW_FRAME_IN,
			 RGBP_F_IP_USE_CINFIFO_NEW_FRAME_IN);
	KUNIT_EXPECT_EQ(test, val, 0);
	val = RGBP_GET_F(test_ctx.pmio, RGBP_R_BYR_CINFIFO_ENABLE, RGBP_F_BYR_CINFIFO_ENABLE);
	KUNIT_EXPECT_EQ(test, val, 1);
}

static void pablo_hw_api_rgbp_hw_s_clock_kunit_test(struct kunit *test)
{
	u32 set_val, expected_val;

	rgbp_hw_s_clock(test_ctx.pmio, true);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_IP_PROCESSING, RGBP_F_IP_PROCESSING);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	rgbp_hw_s_clock(test_ctx.pmio, false);

	set_val = RGBP_GET_F(test_ctx.pmio, RGBP_R_IP_PROCESSING, RGBP_F_IP_PROCESSING);
	expected_val = 0;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

static void pablo_hw_api_rgbp_hw_g_dma_max_cnt_kunit_test(struct kunit *test)
{
	u32 ret;

	ret = rgbp_hw_g_rdma_max_cnt();
	KUNIT_EXPECT_EQ(test, ret, RGBP_RDMA_MAX);

	ret = rgbp_hw_g_wdma_max_cnt();
	KUNIT_EXPECT_EQ(test, ret, RGBP_WDMA_MAX);
}

static void pablo_hw_api_rgbp_hw_s_dma_init_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = NULL;
	u32 enable = 1;
	u32 in_crop_size_x = 0;
	u32 cache_hint = 0;
	u32 sbwc_en = 0;
	u32 payload_size = 0;
	u32 in_rgb = 0;
	u32 out_yuv = 0;
	u32 set_id = 0;
	struct rgbp_dma_cfg dma_cfg;
	pdma_addr_t addr = 0;

	ret = rgbp_hw_s_rdma_init(hw_ip, &test_ctx.dma, &test_ctx.param_set,
				  /* instance, */ enable, in_crop_size_x, cache_hint, &sbwc_en,
				  &payload_size, in_rgb);
	KUNIT_EXPECT_EQ(test, ret, 1);

	dma_cfg.enable = enable;
	dma_cfg.cache_hint = cache_hint;
	dma_cfg.out_yuv = out_yuv;
	dma_cfg.num_buffers = 0;
	ret = rgbp_hw_s_wdma_init(&test_ctx.dma, test_ctx.pmio, set_id, &test_ctx.param_set, &addr,
				  &dma_cfg);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void pablo_hw_api_rgbp_hw_g_dma_param_ptr_kunit_test(struct kunit *test)
{
	int ret;
	u32 rdma_param_max_cnt, wdma_param_max_cnt;
	char *name;
	struct param_dma_input *pdi;
	struct param_dma_output *pdo;
	dma_addr_t *dma_frame_dva;
	pdma_addr_t *param_set_dva;
	struct is_frame *dma_frame = &test_ctx.frame;
	struct rgbp_param_set *param_set = &test_ctx.param_set;

	name = __getname();
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, name);

	/* RDMA */
	rdma_param_max_cnt = rgbp_hw_g_rdma_cfg_max_cnt();
	KUNIT_EXPECT_EQ(test, rdma_param_max_cnt, RGBP_RDMA_CFG_MAX);

	ret = rgbp_hw_g_rdma_param_ptr(RGBP_RDMA_CFG_IMG, dma_frame, param_set, name,
				       &dma_frame_dva, &pdi, &param_set_dva);
	KUNIT_EXPECT_EQ(test, ret, (int)0);
	KUNIT_EXPECT_PTR_EQ(test, (dma_addr_t *)dma_frame_dva,
			    (dma_addr_t *)dma_frame->dvaddr_buffer);
	KUNIT_EXPECT_PTR_EQ(test, (struct param_dma_input *)pdi,
			    (struct param_dma_input *)&param_set->dma_input);
	KUNIT_EXPECT_PTR_EQ(test, (pdma_addr_t *)param_set_dva,
			    (pdma_addr_t *)param_set->input_dva);

	ret = rgbp_hw_g_rdma_param_ptr(RGBP_RDMA_CFG_MAX, dma_frame, param_set, name,
				       &dma_frame_dva, &pdi, &param_set_dva);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	/* WDMA */
	wdma_param_max_cnt = rgbp_hw_g_wdma_cfg_max_cnt();
	KUNIT_EXPECT_EQ(test, wdma_param_max_cnt, RGBP_WDMA_CFG_MAX);

	ret = rgbp_hw_g_wdma_param_ptr(RGBP_WDMA_CFG_HF, dma_frame, param_set, name, &dma_frame_dva,
				       &pdo, &param_set_dva);
	KUNIT_EXPECT_EQ(test, ret, (int)0);
	KUNIT_EXPECT_PTR_EQ(test, (dma_addr_t *)dma_frame_dva,
			    (dma_addr_t *)dma_frame->dva_rgbp_hf);
	KUNIT_EXPECT_PTR_EQ(test, (struct param_dma_output *)pdo,
			    (struct param_dma_output *)&param_set->dma_output_hf);
	KUNIT_EXPECT_PTR_EQ(test, (pdma_addr_t *)param_set_dva,
			    (pdma_addr_t *)param_set->output_dva_hf);

	ret = rgbp_hw_g_wdma_param_ptr(RGBP_WDMA_CFG_SF, dma_frame, param_set, name, &dma_frame_dva,
				       &pdo, &param_set_dva);
	KUNIT_EXPECT_EQ(test, ret, (int)0);
	KUNIT_EXPECT_PTR_EQ(test, (dma_addr_t *)dma_frame_dva,
			    (dma_addr_t *)dma_frame->dva_rgbp_sf);
	KUNIT_EXPECT_PTR_EQ(test, (struct param_dma_output *)pdo,
			    (struct param_dma_output *)&param_set->dma_output_sf);
	KUNIT_EXPECT_PTR_EQ(test, (pdma_addr_t *)param_set_dva,
			    (pdma_addr_t *)param_set->output_dva_sf);

	ret = rgbp_hw_g_wdma_param_ptr(RGBP_WDMA_CFG_YUV, dma_frame, param_set, name,
				       &dma_frame_dva, &pdo, &param_set_dva);
	KUNIT_EXPECT_EQ(test, ret, (int)0);
	KUNIT_EXPECT_PTR_EQ(test, (dma_addr_t *)dma_frame_dva,
			    (dma_addr_t *)dma_frame->dva_rgbp_yuv);
	KUNIT_EXPECT_PTR_EQ(test, (struct param_dma_output *)pdo,
			    (struct param_dma_output *)&param_set->dma_output_yuv);
	KUNIT_EXPECT_PTR_EQ(test, (pdma_addr_t *)param_set_dva,
			    (pdma_addr_t *)param_set->output_dva_yuv);

	ret = rgbp_hw_g_wdma_param_ptr(RGBP_WDMA_CFG_RGB, dma_frame, param_set, name,
				       &dma_frame_dva, &pdo, &param_set_dva);
	KUNIT_EXPECT_EQ(test, ret, (int)0);
	KUNIT_EXPECT_PTR_EQ(test, (dma_addr_t *)dma_frame_dva,
			    (dma_addr_t *)dma_frame->dva_rgbp_rgb);
	KUNIT_EXPECT_PTR_EQ(test, (struct param_dma_output *)pdo,
			    (struct param_dma_output *)&param_set->dma_output_rgb);
	KUNIT_EXPECT_PTR_EQ(test, (pdma_addr_t *)param_set_dva,
			    (pdma_addr_t *)param_set->output_dva_rgb);

	ret = rgbp_hw_g_wdma_param_ptr(RGBP_WDMA_CFG_MAX, dma_frame, param_set, name,
				       &dma_frame_dva, &pdo, &param_set_dva);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	__putname(name);
}

static void pablo_hw_api_rgbp_hw_update_param_kunit_test(struct kunit *test)
{
	struct rgbp_param_set *rgbp_param_set = &test_ctx.param_set;
	struct rgbp_param *rgbp_param = &test_ctx.rgbp_param;

	IS_DECLARE_PMAP(pmap);

	rgbp_param->otf_input.cmd = 1;
	rgbp_param->otf_output.cmd = 1;
	rgbp_param->yuv.cmd = 1;
	rgbp_param->hf.cmd = 1;
	rgbp_param->sf.cmd = 1;
	rgbp_param->stripe_input.total_count = 3;
	rgbp_param->dma_input.cmd = 1;
	rgbp_param->rgb.cmd = 1;

	rgbp_hw_update_param(rgbp_param, pmap, rgbp_param_set);
	KUNIT_EXPECT_NE(test, rgbp_param_set->otf_input.cmd, rgbp_param->otf_input.cmd);
	KUNIT_EXPECT_NE(test, rgbp_param_set->otf_output.cmd, rgbp_param->otf_output.cmd);
	KUNIT_EXPECT_NE(test, rgbp_param_set->dma_output_yuv.cmd, rgbp_param->yuv.cmd);
	KUNIT_EXPECT_NE(test, rgbp_param_set->dma_output_hf.cmd, rgbp_param->hf.cmd);
	KUNIT_EXPECT_NE(test, rgbp_param_set->dma_output_sf.cmd, rgbp_param->sf.cmd);
	KUNIT_EXPECT_NE(test, rgbp_param_set->stripe_input.total_count,
			rgbp_param->stripe_input.total_count);
	KUNIT_EXPECT_NE(test, rgbp_param_set->dma_input.cmd, rgbp_param->dma_input.cmd);
	KUNIT_EXPECT_NE(test, rgbp_param_set->dma_output_rgb.cmd, rgbp_param->rgb.cmd);

	set_bit(PARAM_RGBP_OTF_INPUT, pmap);
	set_bit(PARAM_RGBP_OTF_OUTPUT, pmap);
	set_bit(PARAM_RGBP_DMA_INPUT, pmap);
	set_bit(PARAM_RGBP_STRIPE_INPUT, pmap);
	set_bit(PARAM_RGBP_HF, pmap);
	set_bit(PARAM_RGBP_SF, pmap);
	set_bit(PARAM_RGBP_YUV, pmap);
	set_bit(PARAM_RGBP_RGB, pmap);

	rgbp_hw_update_param(rgbp_param, pmap, rgbp_param_set);
	KUNIT_EXPECT_EQ(test, rgbp_param_set->otf_input.cmd, rgbp_param->otf_input.cmd);
	KUNIT_EXPECT_EQ(test, rgbp_param_set->otf_output.cmd, rgbp_param->otf_output.cmd);
	KUNIT_EXPECT_EQ(test, rgbp_param_set->dma_output_yuv.cmd, rgbp_param->yuv.cmd);
	KUNIT_EXPECT_EQ(test, rgbp_param_set->dma_output_hf.cmd, rgbp_param->hf.cmd);
	KUNIT_EXPECT_EQ(test, rgbp_param_set->dma_output_sf.cmd, rgbp_param->sf.cmd);
	KUNIT_EXPECT_EQ(test, rgbp_param_set->stripe_input.total_count,
			rgbp_param->stripe_input.total_count);
	KUNIT_EXPECT_EQ(test, rgbp_param_set->dma_input.cmd, rgbp_param->dma_input.cmd);
	KUNIT_EXPECT_EQ(test, rgbp_param_set->dma_output_rgb.cmd, rgbp_param->rgb.cmd);
}

static struct kunit_case pablo_hw_api_rgbp_kunit_test_cases[] = {
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_dtp_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_is_occurred0_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_is_occurred1_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_reset_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_init_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_wait_idle_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_dump_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_core_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_dma_dump_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_g_output_dva_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_rdma_corex_id_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_rdma_create_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_rdma_addr_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_wdma_corex_id_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_wdma_create_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_wdma_addr_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_corex_update_type_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_cmdq_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_corex_init_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_wait_corex_idle_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_corex_start_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_g_int0_state_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_g_int0_mask_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_g_int1_state_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_g_int1_mask_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_chain_src_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_chain_dst_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_dtp_output_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_decomp_frame_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_sc_enable_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_sc_dst_size_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_block_bypass_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_is_scaler_get_yuvsc_dst_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_yuvsc_coef_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_upsc_dst_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_upsc_scaling_ratio_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_h_init_phase_offset_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_v_init_phase_offset_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_upsc_enable_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_upsc_coef_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_gamma_enable_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_decomp_enable_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_decomp_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_grid_cfg_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_votf_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_sbwc_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_crop_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_g_reg_cnt_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_strgen_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_clock_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_g_dma_max_cnt_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_dma_init_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_s_yuvsc_scaling_ratio_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_g_dma_param_ptr_kunit_test),
	KUNIT_CASE(pablo_hw_api_rgbp_hw_update_param_kunit_test),
	{},
};

static int __pablo_hw_api_rgbp_pmio_init(struct kunit *test)
{
	int ret;

	test_ctx.pmio_config.name = "rgbp";
	test_ctx.pmio_config.mmio_base = test_ctx.test_addr;
	test_ctx.pmio_config.cache_type = PMIO_CACHE_NONE;

	rgbp_hw_init_pmio_config(&test_ctx.pmio_config);

	test_ctx.pmio = pmio_init(NULL, NULL, &test_ctx.pmio_config);
	if (IS_ERR(test_ctx.pmio)) {
		err("failed to init rgbp PMIO: %ld", PTR_ERR(test_ctx.pmio));
		return -ENOMEM;
	}

	ret = pmio_field_bulk_alloc(test_ctx.pmio, &test_ctx.pmio_fields,
				    test_ctx.pmio_config.fields, test_ctx.pmio_config.num_fields);
	if (ret) {
		err("failed to alloc rgbp PMIO fields: %d", ret);
		pmio_exit(test_ctx.pmio);
		return ret;
	}

	return ret;
}

static void __pablo_hw_api_rgbp_pmio_deinit(struct kunit *test)
{
	pmio_field_bulk_free(test_ctx.pmio, test_ctx.pmio_fields);
	pmio_exit(test_ctx.pmio);
}

static int pablo_hw_api_rgbp_kunit_test_init(struct kunit *test)
{
	int ret;

	test_ctx.test_addr = kunit_kzalloc(test, 0x8000, 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, test_ctx.test_addr);

	ret = __pablo_hw_api_rgbp_pmio_init(test);
	KUNIT_ASSERT_EQ(test, ret, 0);

	return 0;
}

static void pablo_hw_api_rgbp_kunit_test_exit(struct kunit *test)
{
	__pablo_hw_api_rgbp_pmio_deinit(test);

	kunit_kfree(test, test_ctx.test_addr);
	memset(&test_ctx, 0, sizeof(struct rgbp_test_ctx));
}

struct kunit_suite pablo_hw_api_rgbp_kunit_test_suite = {
	.name = "pablo-hw-api-rgbp-v3_0-kunit-test",
	.init = pablo_hw_api_rgbp_kunit_test_init,
	.exit = pablo_hw_api_rgbp_kunit_test_exit,
	.test_cases = pablo_hw_api_rgbp_kunit_test_cases,
};
define_pablo_kunit_test_suites(&pablo_hw_api_rgbp_kunit_test_suite);

MODULE_LICENSE("GPL");
