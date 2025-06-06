// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "pablo-kunit-test.h"
#include "hardware/api/is-hw-api-mcscaler-v4.h"
#include "hardware/sfr/is-sfr-mcsc-v13_20.h"
#include "hardware/sfr/is-sfr-common-dma-v2.h"

#define MCSC_GET_F(base, R, F) PMIO_GET_F(base, R, F)
#define MCSC_GET_V(reg_val, F) is_hw_get_field_value(reg_val, &dma_fields[F])

static struct pablo_hw_api_mcsc_kunit_test_ctx {
	struct scaler_coef_cfg sc_coef, sc_uv_coef;
	void *addr;
	struct pmio_config pmio_config;
	struct pablo_mmio *pmio;
	struct pmio_field *pmio_fields;
} test_ctx;

/* Define the test cases. */

static void pablo_hw_api_mcsc_dump_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;

	is_scaler_dump(tctx->pmio, HW_DUMP_CR);
	is_scaler_dump(tctx->pmio, HW_DUMP_DBG_STATE);
	is_scaler_dump(tctx->pmio, HW_DUMP_MODE_NUM);
}

static void pablo_hw_api_mcsc_djag_strip_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 pre_dst_h = 0, start_pos_h = 0;
	u32 test_result;

	is_scaler_set_djag_strip_config(tctx->pmio, 0, pre_dst_h, start_pos_h);

	is_scaler_get_djag_strip_config(tctx->pmio, 0, &pre_dst_h, &start_pos_h);
	KUNIT_EXPECT_EQ(test, pre_dst_h, (u32)0);
	KUNIT_EXPECT_EQ(test, start_pos_h, (u32)0);

	test_result = is_scaler_get_djag_strip_enable(tctx->pmio, 0);
	KUNIT_EXPECT_EQ(test, test_result, (u32)0);
}

static void pablo_hw_api_mcsc_set_hf_config_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	int ret;
	struct param_mcs_output hf_param = { 0 };
	struct hf_setfile_contents hf_setfile = { 0 };
	struct hf_cfg_by_ni hf_cfg = { 0 };
	u32 payload_size = 0;
	struct is_common_dma dma;
	struct mcsc_radial_cfg radial_cfg = { 0 };

	dma.base = tctx->addr;
	ret = is_scaler_set_hf_config(&dma, tctx->pmio, 0, false, &hf_param, &radial_cfg,
				      &hf_setfile, &payload_size);
	KUNIT_EXPECT_EQ(test, ret, 0);

	is_scaler_set_djag_hf_cfg(tctx->pmio, &hf_cfg);
}

static void pablo_hw_api_mcsc_poly_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 output_id = 0;
	u32 w = 0, h = 0;
	u32 pos_x = 0, pos_y = 0;
	u32 pre_dst_h = 0, start_pos_h = 0;
	u32 enable;

	is_scaler_set_poly_out_crop_size(tctx->pmio, output_id, pos_x, pos_y, w, h);

	is_scaler_get_poly_out_crop_size(tctx->pmio, output_id, &w, &h);
	KUNIT_EXPECT_EQ(test, w, (u32)0);
	KUNIT_EXPECT_EQ(test, h, (u32)0);

	is_scaler_get_poly_src_size(tctx->pmio, output_id, &w, &h);
	KUNIT_EXPECT_EQ(test, w, (u32)0);
	KUNIT_EXPECT_EQ(test, h, (u32)0);

	is_scaler_set_poly_strip_config(tctx->pmio, output_id, pre_dst_h, start_pos_h);
	is_scaler_get_poly_strip_config(tctx->pmio, output_id, &pre_dst_h, &start_pos_h);
	KUNIT_EXPECT_EQ(test, pre_dst_h, (u32)0);
	KUNIT_EXPECT_EQ(test, start_pos_h, (u32)0);

	enable = is_scaler_get_poly_strip_enable(tctx->pmio, output_id);
	KUNIT_EXPECT_EQ(test, enable, (u32)0);
}

static void pablo_hw_api_mcsc_post_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 output_id = 0;
	u32 w = 0, h = 0;
	u32 pos_x = 0, pos_y = 0;
	u32 pre_dst_h = 0, start_pos_h = 0;
	u32 enable;

	is_scaler_get_post_img_size(tctx->pmio, output_id, &w, &h);
	KUNIT_EXPECT_EQ(test, w, (u32)0);
	KUNIT_EXPECT_EQ(test, h, (u32)0);

	is_scaler_set_post_out_crop_size(tctx->pmio, output_id, pos_x, pos_y, w, h);

	is_scaler_get_post_out_crop_size(tctx->pmio, output_id, &w, &h);
	KUNIT_EXPECT_EQ(test, w, (u32)0);
	KUNIT_EXPECT_EQ(test, h, (u32)0);

	is_scaler_set_post_strip_config(tctx->pmio, output_id, pre_dst_h, start_pos_h);
	is_scaler_get_post_strip_config(tctx->pmio, output_id, &pre_dst_h, &start_pos_h);
	KUNIT_EXPECT_EQ(test, pre_dst_h, (u32)0);
	KUNIT_EXPECT_EQ(test, start_pos_h, (u32)0);

	enable = is_scaler_get_post_strip_enable(tctx->pmio, output_id);
	KUNIT_EXPECT_EQ(test, enable, (u32)0);
}

static void pablo_hw_api_mcsc_rdma_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;

	is_scaler_clear_rdma_addr(tctx->pmio);

	is_scaler_set_rdma_10bit_type(tctx->pmio, 0);

	is_scaler_set_rdma_frame_seq(tctx->pmio, 0);
}

static u32 pablo_hw_get_mcsc_dma(struct is_common_dma *dma, u32 reg)
{
	unsigned int corex_offset = GET_COREX_OFFSET(dma->set_id);

	return PMIO_GET_R(dma->base, corex_offset + dma->reg_ofs + dma_regs[reg].sfr_offset);
}

static void pablo_hw_api_mcsc_wdma_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 output_id = 0;
	u32 plane = 0;
	u32 out_fmt = MCSC_RGB_ARGB8888;
	struct is_common_dma dma;
	u32 y_addr, cb_addr, cr_addr;
	u32 format_type, mono_mode;
	u32 reg_val, val;
	u32 utc_flag;

	is_scaler_clear_rdma_addr(tctx->pmio);

	dma.base = tctx->pmio;
	output_id = MCSC_OUTPUT2;
	is_scaler_set_wdma_format(&dma, tctx->pmio, 0, output_id, plane, out_fmt);

	is_scaler_get_wdma_addr(tctx->pmio, output_id, &y_addr, &cb_addr, &cr_addr, 0);
	KUNIT_EXPECT_EQ(test, y_addr, (u32)0);
	KUNIT_EXPECT_EQ(test, cb_addr, (u32)0);
	KUNIT_EXPECT_EQ(test, cr_addr, (u32)0);

	is_scaler_set_mono_ctrl(tctx->pmio, 0, MCSC_MONO_Y8);
	dma.available_yuv_format_map = 0xFFFFFFFFFF & IS_YUV_FORMAT_MASK;
	pmio_dma_set_ops(&dma);
	is_scaler_set_wdma_format(&dma, tctx->pmio, 0, output_id, plane, out_fmt);

	format_type = is_scaler_get_wdma_fmt_info(&out_fmt, &mono_mode, 0, output_id, tctx->pmio);
	reg_val = pablo_hw_get_mcsc_dma(&dma, DMA_R_MONO_MODE);
	KUNIT_EXPECT_EQ(test, reg_val, mono_mode);
	utc_flag = reg_val == format_type;
	reg_val = pablo_hw_get_mcsc_dma(&dma, DMA_R_DATA_FORMAT);
	val = MCSC_GET_V(reg_val, DMA_F_DATA_FORMAT_YUV);
	KUNIT_EXPECT_EQ(test, val, out_fmt);
	utc_flag = utc_flag ? val == out_fmt : 0;
	val = MCSC_GET_V(reg_val, DMA_F_DATA_FORMAT_TYPE);
	KUNIT_EXPECT_EQ(test, val, format_type);
	utc_flag = utc_flag ? val == out_fmt : 0;
}

static void pablo_hw_api_mcsc_hwfc_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 idx;
	u32 output_id = 0;

	is_scaler_s_init(tctx->pmio);
	is_scaler_set_hwfc_mode(tctx->pmio, output_id);

	idx = is_scaler_get_hwfc_idx_bin(tctx->pmio, output_id);
	KUNIT_EXPECT_EQ(test, idx, (u32)0);

	idx = is_scaler_get_hwfc_cur_idx(tctx->pmio, output_id);
	KUNIT_EXPECT_EQ(test, idx, (u32)0);
}

static void pablo_hw_api_mcsc_get_idle_status_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 status;
	u32 hw_id = 0;

	status = is_scaler_get_idle_status(tctx->pmio, hw_id);
	KUNIT_EXPECT_EQ(test, status, (u32)0);
}

static void pablo_hw_api_mcsc_get_version_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 version;

	version = is_scaler_get_version(tctx->pmio);
	KUNIT_EXPECT_EQ(test, version, (u32)0);
}

static void pablo_hw_api_mcsc_input_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 hw_id = 0;
	u32 w = 0, h = 0;

	is_scaler_set_input_img_size(tctx->pmio, hw_id, w, h);
	is_scaler_get_input_img_size(tctx->pmio, hw_id, &w, &h);
	KUNIT_EXPECT_EQ(test, w, (u32)0);
	KUNIT_EXPECT_EQ(test, h, (u32)0);

	is_scaler_get_input_source(tctx->addr, hw_id);
}

static void pablo_hw_api_mcsc_cac_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	struct cac_cfg_by_ni cfg = { 0 };

	is_scaler_set_cac_enable(tctx->pmio, 0);
	is_scaler_set_cac_map_crt_thr(tctx->pmio, &cfg);
}

static void pablo_hw_api_mcsc_set_crc_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 seed = 0;

	pablo_kunit_scaler_hw_set_crc(tctx->pmio, seed);
}

static void pablo_hw_api_mcsc_set_poly_scaler_coef_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 output_id = 0;
	u32 hratio, vratio;
	struct scaler_coef_cfg *sc_coefs[2];
	u32 src_width = 320, src_height = 240;
	u32 dst_width = src_width, dst_height = src_height;
	u32 val;
	bool utc_flag;

	tctx->sc_coef.use_poly_sc_coef = 0;
	tctx->sc_uv_coef.use_poly_sc_coef = 0;
	sc_coefs[0] = &tctx->sc_coef;
	sc_coefs[1] = &tctx->sc_uv_coef;

	hratio = GET_ZOOM_RATIO(src_width, dst_width);
	vratio = GET_ZOOM_RATIO(src_height, dst_height);
	is_scaler_set_poly_scaler_coef(tctx->pmio, output_id, hratio, vratio, sc_coefs, 2, SP_REAR);

	/* check one of the horizontal Y coefficients randomly (4th tap of phase 1) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_POLY_SC0_Y_H_COEFF_1_23,
			 MCSC_F_YUV_POLY_SC0_Y_H_COEFF_1_3);
	KUNIT_EXPECT_EQ(test, (u32)509, val);
	utc_flag = val == 509;

	dst_width = 240;
	dst_height = 180; /* zoom ratio x6_8 */
	hratio = GET_ZOOM_RATIO(src_width, dst_width);
	vratio = GET_ZOOM_RATIO(src_height, dst_height);
	output_id = 1;
	is_scaler_set_poly_scaler_coef(tctx->pmio, output_id, hratio, vratio, sc_coefs, 2, SP_REAR);

	/* check one of the horizontal UV coefficients randomly (2nd tap of phase 2) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_POLY_SC1_UV_H_COEFF_2_01,
			 MCSC_F_YUV_POLY_SC1_UV_H_COEFF_2_1);
	KUNIT_EXPECT_EQ(test, (u32)383, val);
	utc_flag = utc_flag ? val == 383 : 0;

	dst_width = 160;
	dst_height = 120; /* zoom ratio x4_8 */
	hratio = GET_ZOOM_RATIO(src_width, dst_width);
	vratio = GET_ZOOM_RATIO(src_height, dst_height);
	output_id = 2;
	is_scaler_set_poly_scaler_coef(tctx->pmio, output_id, hratio, vratio, sc_coefs, 2, SP_REAR);

	/* check one of the vertical Y coefficients randomly (3rd tap of phase 3) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_POLY_SC2_Y_V_COEFF_3_23,
			 MCSC_F_YUV_POLY_SC2_Y_V_COEFF_3_2);
	KUNIT_EXPECT_EQ(test, (u32)153, val);
	utc_flag = utc_flag ? val == 153 : 0;

	dst_width = 80;
	dst_height = 60; /* zoom ratio x2_8 */
	hratio = GET_ZOOM_RATIO(src_width, dst_width);
	vratio = GET_ZOOM_RATIO(src_height, dst_height);
	output_id = 4;
	is_scaler_set_poly_scaler_coef(tctx->pmio, output_id, hratio, vratio, sc_coefs, 2, SP_REAR);

	set_utc_result(KUTC_MCSC_POLY_COEF, UTC_ID_MCSC_POLY_COEF, utc_flag);
}

static void pablo_hw_api_mcsc_set_post_scaler_coef_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 output_id = 0;
	u32 hratio, vratio;
	u32 src_width = 320, src_height = 240;
	u32 dst_width = src_width, dst_height = src_height; /* zoom ratio x8_8 */
	u32 val;
	struct scaler_coef_cfg *sc_coefs[2];

	sc_coefs[0] = &tctx->sc_coef;
	sc_coefs[1] = &tctx->sc_uv_coef;

	hratio = GET_ZOOM_RATIO(src_width, dst_width);
	vratio = GET_ZOOM_RATIO(src_height, dst_height);
	is_scaler_set_post_scaler_coef(tctx->pmio, output_id, hratio, vratio, sc_coefs, 2);

	/* check one of the horizontal Y coefficients randomly (4th tap of phase 1) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_POSTPC_PC0_Y_H_COEFF_1_23,
			 MCSC_F_YUV_POSTPC_PC0_Y_H_COEFF_1_3);
	KUNIT_EXPECT_EQ(test, (u32)509, val);

	dst_width = 240;
	dst_height = 180; /* zoom ratio x6_8 */
	hratio = GET_ZOOM_RATIO(src_width, dst_width);
	vratio = GET_ZOOM_RATIO(src_height, dst_height);
	output_id = 1;
	is_scaler_set_post_scaler_coef(tctx->pmio, output_id, hratio, vratio, sc_coefs, 2);

	/* check one of the horizontal UV coefficients randomly (2nd tap of phase 2) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_POSTPC_PC1_UV_H_COEFF_2_01,
			 MCSC_F_YUV_POSTPC_PC1_UV_H_COEFF_2_1);
	KUNIT_EXPECT_EQ(test, (u32)383, val);

	dst_width = 160;
	dst_height = 120; /* zoom ratio x4_8 */
	hratio = GET_ZOOM_RATIO(src_width, dst_width);
	vratio = GET_ZOOM_RATIO(src_height, dst_height);
	output_id = 2;
	is_scaler_set_post_scaler_coef(tctx->pmio, output_id, hratio, vratio, sc_coefs, 2);

	/* check one of the vertical Y coefficients randomly (3rd tap of phase 3) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_POSTPC_PC2_Y_V_COEFF_3_23,
			 MCSC_F_YUV_POSTPC_PC2_Y_V_COEFF_3_2);
	KUNIT_EXPECT_EQ(test, (u32)153, val);

	dst_width = 80;
	dst_height = 60; /* zoom ratio x2_8 */
	hratio = GET_ZOOM_RATIO(src_width, dst_width);
	vratio = GET_ZOOM_RATIO(src_height, dst_height);
	output_id = 0;
	is_scaler_set_post_scaler_coef(tctx->pmio, output_id, hratio, vratio, sc_coefs, 2);

	/* check one of the vertical UV coefficients randomly (4th tap of phase 4) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_POSTPC_PC0_UV_V_COEFF_4_23,
			 MCSC_F_YUV_POSTPC_PC0_UV_V_COEFF_4_3);
	KUNIT_EXPECT_EQ(test, (u32)21, val);
}

#if IS_ENABLED(USE_DJAG_COEFF_CONFIG)
static void pablo_hw_api_mcsc_set_djag_scaler_coef_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	struct djag_scaling_ratio_depended_config djag_tune;
	u32 val;

	djag_tune.djag_sc_coef.use_djag_sc_coef = 0;
	is_scaler_set_djag_tuning_param(tctx->pmio, &djag_tune);

	/* check one of the horizontal Y coefficients randomly(4th tap of phase 1) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_DJAG_SC_H_COEFF_1_23,
			 MCSC_F_YUV_DJAG_SC_H_COEFF_1_3);
	KUNIT_EXPECT_EQ(test, (u32)509, val);

	/* check one of the horizontal UV coefficients randomly(2nd tap of phase 2) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_DJAGPS_PS_UV_H_COEFF_2_01,
			 MCSC_F_YUV_DJAGPS_PS_UV_H_COEFF_2_1);
	KUNIT_EXPECT_EQ(test, (u32)495, val);

	/* check one of the vertical Y coefficients randomly(3rd tap of phase 3) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_DJAG_SC_V_COEFF_3_23,
			 MCSC_F_YUV_DJAG_SC_V_COEFF_3_2);
	KUNIT_EXPECT_EQ(test, (u32)75, val);

	/* check one of the vertical UV coefficients randomly(3rd tap of phase 4) */
	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_DJAGPS_PS_UV_V_COEFF_4_23,
			 MCSC_F_YUV_DJAGPS_PS_UV_V_COEFF_4_2);
	KUNIT_EXPECT_EQ(test, (u32)110, val);
}
#endif

static void pablo_hw_api_mcsc_set_hwfc_config_kunit_test(struct kunit *test)
{
	struct pablo_hw_api_mcsc_kunit_test_ctx *tctx = test->priv;
	u32 output_id = MCSC_OUTPUT1;
	u32 format = DMA_INOUT_FORMAT_YUV420;
	u32 plane = 2;
	u32 width = 320, height = 240;
	u32 val;

	is_scaler_set_hwfc_config(tctx->pmio, output_id, format, plane,
				  output_id * HWFC_DMA_ID_OFFSET, width, height, 1);

	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_HWFC_TOTAL_WIDTH_BYTE1_A,
			 MCSC_F_YUV_HWFC_TOTAL_WIDTH_BYTE1_A);
	KUNIT_EXPECT_EQ(test, width, val);

	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_HWFC_IMAGE_HEIGHT, MCSC_F_YUV_HWFC_IMAGE_HEIGHT_A);
	KUNIT_EXPECT_EQ(test, height, val);

	output_id = MCSC_OUTPUT2;
	format = DMA_INOUT_FORMAT_YUV422;
	plane = 1;
	is_scaler_set_hwfc_config(tctx->pmio, output_id, format, plane,
				  output_id * HWFC_DMA_ID_OFFSET, width, height, 1);

	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_HWFC_TOTAL_IMAGE_BYTE0_B,
			 MCSC_F_YUV_HWFC_TOTAL_IMAGE_BYTE0_B);
	KUNIT_EXPECT_EQ(test, (width * height) << 1, val);

	val = MCSC_GET_F(tctx->pmio, MCSC_R_YUV_HWFC_IMAGE_HEIGHT, MCSC_F_YUV_HWFC_IMAGE_HEIGHT_B);
	KUNIT_EXPECT_EQ(test, height, val);
}

static void pablo_hw_api_mcsc_get_payload_size_kunit_test(struct kunit *test)
{
	struct param_mcs_output output = { 0 };
	u32 payload, i, stride, height, result, plane = 2;

	/* sbwc */
	output.sbwc_type = DMA_INPUT_SBWC_LOSSY_32B;
	output.dma_format = DMA_INOUT_FORMAT_YUV420;
	output.dma_bitwidth = 8;
	output.full_output_width = 320;
	output.height = 240;
	stride = ALIGN(((output.full_output_width + 31) / 32) * 96, 32);

	for (i = 0; i < plane; i++) {
		height = i ? output.height / 2 : output.height;
		result = (height + MCSC_COMP_BLOCK_HEIGHT - 1) / MCSC_COMP_BLOCK_HEIGHT * stride;
		payload = is_scaler_get_payload_size(&output, i);
		KUNIT_EXPECT_EQ(test, payload, result);
	}

	/* normal */
	output.sbwc_type = DMA_INPUT_SBWC_DISABLE;
	for (i = 0; i < plane; i++) {
		payload = is_scaler_get_payload_size(&output, i);
		KUNIT_EXPECT_EQ(test, payload, (u32)0);
	}
}

static void pablo_hw_api_mcsc_set_clock_kunit_test(struct kunit *test)
{
	u32 set_val, expected_val;
	struct pablo_hw_api_mcsc_kunit_test_ctx *ctx = test->priv;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, ctx);

	is_scaler_set_clock(ctx->pmio, true);

	set_val = MCSC_GET_F(ctx->pmio, MCSC_R_IP_PROCESSING, MCSC_F_IP_PROCESSING);
	expected_val = 1;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);

	is_scaler_set_clock(ctx->pmio, false);

	set_val = MCSC_GET_F(ctx->pmio, MCSC_R_IP_PROCESSING, MCSC_F_IP_PROCESSING);
	expected_val = 0;
	KUNIT_EXPECT_EQ(test, set_val, expected_val);
}

void __pablo_hw_api_init_ratio_idx(struct scaler_filter_coef_cfg *coef)
{
	coef->ratio_x8_8 = 0;
	coef->ratio_x7_8 = 1;
	coef->ratio_x6_8 = 2;
	coef->ratio_x5_8 = 3;
	coef->ratio_x4_8 = 4;
	coef->ratio_x3_8 = 5;
	coef->ratio_x2_8 = 6;
}

static int __pablo_hw_api_mcsc_pmio_init(struct kunit *test)
{
	int ret;

	test_ctx.pmio_config.name = "mcsc";

	test_ctx.pmio_config.mmio_base = test_ctx.addr;

	test_ctx.pmio_config.cache_type = PMIO_CACHE_NONE;

	mcsc_hw_init_pmio_config(&test_ctx.pmio_config);

	test_ctx.pmio = pmio_init(NULL, NULL, &test_ctx.pmio_config);

	if (IS_ERR(test_ctx.pmio)) {
		err("failed to init mcsc PMIO: %ld", PTR_ERR(test_ctx.pmio));
		return -ENOMEM;
	}

	ret = pmio_field_bulk_alloc(test_ctx.pmio, &test_ctx.pmio_fields,
				    test_ctx.pmio_config.fields, test_ctx.pmio_config.num_fields);
	if (ret) {
		err("failed to alloc mcsc PMIO fields: %d", ret);
		pmio_exit(test_ctx.pmio);
		return ret;
	}

	return ret;
}

static void __pablo_hw_api_mcsc_pmio_deinit(struct kunit *test)
{
	pmio_field_bulk_free(test_ctx.pmio, test_ctx.pmio_fields);
	pmio_exit(test_ctx.pmio);
}

static int pablo_hw_api_mcsc_kunit_test_init(struct kunit *test)
{
	int idx, ret;

	test_ctx.sc_coef.use_poly_sc_coef = 0;
	test_ctx.sc_uv_coef.use_poly_sc_coef = 0;
	for (idx = 0; idx < 2; idx++) {
		__pablo_hw_api_init_ratio_idx(&test_ctx.sc_coef.poly_sc_coef[idx]);
		__pablo_hw_api_init_ratio_idx(&test_ctx.sc_uv_coef.poly_sc_coef[idx]);
		__pablo_hw_api_init_ratio_idx(&test_ctx.sc_coef.post_sc_coef[idx]);
		__pablo_hw_api_init_ratio_idx(&test_ctx.sc_uv_coef.post_sc_coef[idx]);
	}

	test_ctx.addr = kunit_kzalloc(test, 0x10000, 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, test_ctx.addr);
	test->priv = &test_ctx;

	ret = __pablo_hw_api_mcsc_pmio_init(test);
	KUNIT_ASSERT_EQ(test, ret, 0);

	return 0;
}

static void pablo_hw_api_mcsc_kunit_test_exit(struct kunit *test)
{
	__pablo_hw_api_mcsc_pmio_deinit(test);

	kunit_kfree(test, test_ctx.addr);
	memset(&test_ctx, 0, sizeof(struct pablo_hw_api_mcsc_kunit_test_ctx));
}

static struct kunit_case pablo_hw_api_mcsc_kunit_test_cases[] = {
	KUNIT_CASE(pablo_hw_api_mcsc_dump_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_djag_strip_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_set_hf_config_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_poly_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_post_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_rdma_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_wdma_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_hwfc_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_get_idle_status_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_get_version_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_input_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_cac_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_set_crc_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_set_poly_scaler_coef_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_set_post_scaler_coef_kunit_test),
#if IS_ENABLED(USE_DJAG_COEFF_CONFIG)
	KUNIT_CASE(pablo_hw_api_mcsc_set_djag_scaler_coef_kunit_test),
#endif
	KUNIT_CASE(pablo_hw_api_mcsc_set_hwfc_config_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_get_payload_size_kunit_test),
	KUNIT_CASE(pablo_hw_api_mcsc_set_clock_kunit_test),
	{},
};

struct kunit_suite pablo_hw_api_mcsc_kunit_test_suite = {
	.name = "pablo-hw-api-mcsc-v13_20-kunit-test",
	.init = pablo_hw_api_mcsc_kunit_test_init,
	.exit = pablo_hw_api_mcsc_kunit_test_exit,
	.test_cases = pablo_hw_api_mcsc_kunit_test_cases,
};
define_pablo_kunit_test_suites(&pablo_hw_api_mcsc_kunit_test_suite);

MODULE_LICENSE("GPL");
