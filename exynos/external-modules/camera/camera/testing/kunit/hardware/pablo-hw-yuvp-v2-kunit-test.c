// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "pablo-kunit-test.h"
#include "is-hw-ip.h"
#include "is-hw-yuvp-v2.h"
#include "is-hw-api-yuvp-v2_0.h"
static struct pablo_hw_yuvp_kunit_test_ctx {
	struct is_hw_ip		hw_ip;
	struct is_hardware	hardware;
	struct is_interface_ischain	itfc;
	struct is_framemgr	framemgr;
	struct is_frame		frame;
	struct camera2_shot_ext	shot_ext;
	struct camera2_shot	shot;
	struct is_param_region	parameter;
	struct is_mem		mem;
	struct is_mem_ops	memops;
	struct yuvp_param	yuvp_param;
	struct is_region	region;
	void			*test_addr;
	struct is_hw_ip_ops	hw_ops;
	struct is_hw_ip_ops     *org_hw_ops;
} test_ctx;

static struct param_otf_input test_otf_input = {
	.cmd = OTF_INPUT_COMMAND_ENABLE,
	.format = OTF_INPUT_FORMAT_YUV422,
	.bitwidth = 0,
	.order = 0,
	.width = 1920,
	.height = 1440,
	.bayer_crop_offset_x = 0,
	.bayer_crop_offset_y = 0,
	.bayer_crop_width = 1920,
	.bayer_crop_height = 1440,
	.source = 0,
	.physical_width = 1920,
	.physical_height = 1440,
	.offset_x = 0,
	.offset_y = 0,
};

static struct param_dma_input test_dma_input = {
	.cmd = DMA_INPUT_COMMAND_ENABLE,
	.format = DMA_INOUT_FORMAT_YUV422,
	.bitwidth = DMA_INOUT_BIT_WIDTH_10BIT,
	.plane = DMA_INOUT_PLANE_2,
	.width = 1920,
	.height = 1440,
	.dma_crop_offset = 0,
	.dma_crop_width = 1920,
	.dma_crop_height = 1440,
	.bayer_crop_width = 1920,
	.bayer_crop_height = 1440,
	.msb = DMA_INOUT_BIT_WIDTH_10BIT - 1,
	.stride_plane0 = 1920,
	.stride_plane1 = 1920,
	.stride_plane2 = 1920,
};

static struct param_otf_output test_output = {
	.cmd = OTF_OUTPUT_COMMAND_ENABLE,
	.format = OTF_INPUT_FORMAT_BAYER,
	.width = 1920,
	.height = 1440,
};

/* Define the test cases. */

static void pablo_hw_yuvp_open_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, init, instance, false, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, deinit, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_handle_interrupt0_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	struct is_interface_ischain *itfc = &test_ctx.itfc;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, itfc->itf_ip[9].handler[INTR_HWIP1].handler);

	/* not opened */
	ret = itfc->itf_ip[9].handler[INTR_HWIP1].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* overflow recorvery */
	set_bit(HW_OVERFLOW_RECOVERY, &hw_ip->hardware->hw_recovery_flag);
	ret = itfc->itf_ip[9].handler[INTR_HWIP1].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);
	clear_bit(HW_OVERFLOW_RECOVERY, &hw_ip->hardware->hw_recovery_flag);

	/* not run */
	ret = itfc->itf_ip[9].handler[INTR_HWIP1].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	set_bit(HW_RUN, &hw_ip->state);
	*(u32 *)(test_ctx.test_addr + 0x0800) = 0xFFFFFFFF;
	*(u32 *)(test_ctx.test_addr + 0x0804) = 0xFFFFFFFF;
	ret = itfc->itf_ip[9].handler[INTR_HWIP1].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_handle_interrupt1_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	struct is_interface_ischain *itfc = &test_ctx.itfc;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, itfc->itf_ip[9].handler[INTR_HWIP2].handler);

	/* not opened */
	ret = itfc->itf_ip[9].handler[INTR_HWIP2].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* overflow recorvery */
	set_bit(HW_OVERFLOW_RECOVERY, &hw_ip->hardware->hw_recovery_flag);
	ret = itfc->itf_ip[9].handler[INTR_HWIP2].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);
	clear_bit(HW_OVERFLOW_RECOVERY, &hw_ip->hardware->hw_recovery_flag);

	/* not run */
	ret = itfc->itf_ip[9].handler[INTR_HWIP2].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	set_bit(HW_RUN, &hw_ip->state);
	*(u32 *)(test_ctx.test_addr + 0x0800) = 0xFFFFFFFF;
	*(u32 *)(test_ctx.test_addr + 0x0804) = 0xFFFFFFFF;
	ret = itfc->itf_ip[9].handler[INTR_HWIP2].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static bool __set_rta_regs_false(struct is_hw_ip *hw_ip, u32 instance, u32 set_id, bool skip,
		struct is_frame *frame, void *buf)
{
	return false;
}

static bool __set_rta_regs_true(struct is_hw_ip *hw_ip, u32 instance, u32 set_id, bool skip,
		struct is_frame *frame, void *buf)
{
	return true;
}

static int __reset_stub(struct is_hw_ip *hw_ip, u32 instance)
{
	return 0;
}

static void pablo_hw_yuvp_check_perframe(struct is_hw_ip *hw_ip, ulong hw_map, struct kunit *test)
{
	struct is_param_region *param_region = &test_ctx.parameter;
	u32 resolution[][2] = {
		{ 1920, 1440 },
		{ 1000, 750 },
		{ 500, 375 },
	};
	u32 width, height;
	int ret, i;
	bool utc_flag = false;

	param_region->yuvp.otf_input = test_otf_input;
	param_region->yuvp.otf_output = test_output;

	for (i = 0; i < ARRAY_SIZE(resolution); i++) {
		param_region->yuvp.otf_input.width = resolution[i][0];
		param_region->yuvp.otf_input.height = resolution[i][1];
		ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
		KUNIT_EXPECT_EQ(test, ret, 0);
		if (i == 0)
			utc_flag = ret == 0;
		else
			utc_flag = utc_flag && !ret;
		yuvp_hw_g_size(hw_ip->pmio, COREX_DIRECT, &width, &height);
		ret = (width == resolution[i][0]) && (height == resolution[i][1]);
		KUNIT_EXPECT_TRUE(test, ret);
		utc_flag = utc_flag && ret;
	}

	set_utc_result(KUTC_YUVP_PER_FRM_CTRL, UTC_ID_YUVP_PER_FRM_CTRL, utc_flag);
	set_utc_result(KUTC_YUVP_META_INF, UTC_ID_YUVP_META_INF, utc_flag);
}

static int pablo_hw_yuvp_interface_yuv_otf_input(struct is_hw_ip *hw_ip, ulong hw_map)
{
	int ret, cmp;
	struct is_param_region *param_region = &test_ctx.parameter;
	struct is_hw_yuvp *hw_yuvp = (struct is_hw_yuvp *)hw_ip->priv_info;
	struct param_otf_input *set_param = &hw_yuvp->param_set[0].otf_input;

	param_region->yuvp.otf_input = test_otf_input;

	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	ret = ret == 0;
	cmp = memcmp(&param_region->yuvp.otf_input, set_param, sizeof(struct param_otf_input));
	ret = ret ? !cmp : 0;

	return ret;
}

static int pablo_hw_yuvp_interface_yuv_m2m_input(struct is_hw_ip *hw_ip, ulong hw_map)
{
	int ret, cmp;
	struct is_param_region *param_region = &test_ctx.parameter;
	struct is_hw_yuvp *hw_yuvp = (struct is_hw_yuvp *)hw_ip->priv_info;
	struct param_dma_input *set_param = &hw_yuvp->param_set[0].dma_input;

	param_region->yuvp.dma_input = test_dma_input;

	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	ret = ret == 0;
	cmp = memcmp(&param_region->yuvp.dma_input, set_param, sizeof(struct param_dma_input));
	ret = ret ? !cmp : 0;

	return ret;
}

static void pablo_hw_yuvp_interface_yuv_input(
	struct is_hw_ip *hw_ip, ulong hw_map, struct kunit *test)
{
	int ret;
	bool utc_flag;

	ret = pablo_hw_yuvp_interface_yuv_otf_input(hw_ip, hw_map);
	KUNIT_EXPECT_TRUE(test, ret);
	utc_flag = ret == true;

	ret = pablo_hw_yuvp_interface_yuv_m2m_input(hw_ip, hw_map);
	KUNIT_EXPECT_TRUE(test, ret);
	utc_flag = utc_flag ? ret == true : 0;

	set_utc_result(KUTC_YUVP_SET_YUVIN, UTC_ID_YUVP_SET_YUVIN, utc_flag);
}

static void pablo_hw_yuvp_shot_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	struct pablo_hw_helper_ops ops;
	u32 instance = 0;
	ulong hw_map = 0;
	struct is_param_region *param_region;
	struct is_hw_yuvp *hw_yuvp;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, init, instance, false, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	set_bit(hw_ip->id, &hw_map);
	test_ctx.frame.shot = &test_ctx.shot;
	test_ctx.frame.shot_ext = &test_ctx.shot_ext;
	test_ctx.frame.parameter = &test_ctx.parameter;
	ops.set_rta_regs = __set_rta_regs_false;
	hw_ip->help_ops = &ops;
	hw_ip->region[instance] = &test_ctx.region;

	set_bit(PARAM_YUVP_OTF_INPUT, test_ctx.frame.pmap);
	set_bit(PARAM_YUVP_DMA_INPUT, test_ctx.frame.pmap);
	set_bit(PARAM_YUVP_DRC, test_ctx.frame.pmap);
	set_bit(PARAM_YUVP_CLAHE, test_ctx.frame.pmap);
	set_bit(PARAM_YUVP_SEG, test_ctx.frame.pmap);
	set_bit(PARAM_YUVP_OTF_OUTPUT, test_ctx.frame.pmap);
	set_bit(PARAM_YUVP_YUV, test_ctx.frame.pmap);
	set_bit(PARAM_YUVP_STRIPE_INPUT, test_ctx.frame.pmap);
	set_bit(PARAM_YUVP_SVHIST, test_ctx.frame.pmap);

	param_region = &test_ctx.parameter;
	param_region->yuvp.clahe.cmd = 1;
	param_region->yuvp.clahe.bitwidth = 16;
	param_region->yuvp.clahe.msb = 7;
	param_region->yuvp.svhist.cmd = 1;
	param_region->yuvp.svhist.bitwidth = 16;
	param_region->yuvp.svhist.msb = 7;
	param_region->yuvp.drc.cmd = 1;
	param_region->yuvp.drc.bitwidth = 16;
	param_region->yuvp.drc.msb = 7;
	param_region->yuvp.seg.cmd = 1;
	param_region->yuvp.seg.bitwidth = 16;
	param_region->yuvp.seg.msb = 7;
	param_region->yuvp.otf_output.cmd = 1;
	param_region->yuvp.stripe_input.total_count = 0;

	hw_yuvp = (struct is_hw_yuvp *)hw_ip->priv_info;
	hw_yuvp->config.yuvnr_contents_aware_isp_en = 1;
	hw_yuvp->config.ccm_contents_aware_isp_en = 1;
	hw_yuvp->config.sharpen_contents_aware_isp_en = 1;

	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ops.set_rta_regs = __set_rta_regs_true;
	hw_yuvp->config.yuvnr_contents_aware_isp_en = 0;
	hw_yuvp->config.ccm_contents_aware_isp_en = 0;
	hw_yuvp->config.sharpen_contents_aware_isp_en = 0;
	param_region->yuvp.stripe_input.total_count = 2;

	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	pablo_hw_yuvp_check_perframe(hw_ip, hw_map, test);

	pablo_hw_yuvp_interface_yuv_input(hw_ip, hw_map, test);

	ret = CALL_HWIP_OPS(hw_ip, deinit, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_enable_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	ulong hw_map = 0;
	bool utc_flag;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, init, instance, false, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	set_bit(hw_ip->id, &hw_map);

	ret = CALL_HWIP_OPS(hw_ip, enable, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);
	utc_flag = ret == 0;

	ret = CALL_HWIP_OPS(hw_ip, disable, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);
	utc_flag = utc_flag ? ret == 0 : 0;

	ret = CALL_HWIP_OPS(hw_ip, deinit, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	set_utc_result(KUTC_YUVP_EN_DIS, UTC_ID_YUVP_EN_DIS, utc_flag);
}

static void pablo_hw_yuvp_set_config_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 chain_id = 0;
	u32 instance = 0;
	u32 fcount = 0;
	struct is_yuvp_config conf;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, set_config, chain_id, instance, fcount, &conf);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_dump_regs_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	u32 fcount = 0;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, dump_regs, instance, fcount, NULL, 0, IS_REG_DUMP_TO_LOG);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, dump_regs, instance, fcount, NULL, 0, IS_REG_DUMP_DMA);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, dump_regs, instance, fcount, NULL, 0, IS_REG_DUMP_TO_ARRAY);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_set_regs_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 chain_id = 0;
	u32 instance = 0;
	u32 fcount = 0;

	ret = CALL_HWIP_OPS(hw_ip, set_regs, chain_id, instance, fcount, NULL, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_notify_timeout_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;

	ret = CALL_HWIP_OPS(hw_ip, notify_timeout, instance);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, notify_timeout, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_restore_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, restore, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_setfile_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 scenario = 0;
	u32 instance = 0;
	ulong hw_map = 0;

	ret = CALL_HWIP_OPS(hw_ip, load_setfile, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, apply_setfile, scenario, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, delete_setfile, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_get_meta_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	ulong hw_map = 0;

	ret = CALL_HWIP_OPS(hw_ip, get_meta, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_set_param_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	ulong hw_map = 0;
	IS_DECLARE_PMAP(pmap);

	set_bit(hw_ip->id, &hw_map);

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, init, instance, false, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, set_param, &test_ctx.region, pmap, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, deinit, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_frame_ndone_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	enum ShotErrorType type = IS_SHOT_UNKNOWN;

	ret = CALL_HWIP_OPS(hw_ip, frame_ndone, &test_ctx.frame, type);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_yuvp_reset_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;

	hw_ip->ops = test_ctx.org_hw_ops;

	ret = CALL_HWIP_OPS(hw_ip, reset, instance);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);
}

static void pablo_hw_yuvp_dma_cfg_kunit_test(struct kunit *test)
{
	struct is_hw_ip *hw_ip = &test_ctx.hardware.hw_ip[0];
	u32 dma_cmd = 1;
	u32 plane = DMA_INOUT_PLANE_4;
	pdma_addr_t dst_dva[IS_MAX_PLANES] = { 0 };
	dma_addr_t sc0TargetAddress[IS_MAX_PLANES];
	int ret;

	test_ctx.frame.num_buffers = 1;
	ret = is_hardware_dma_cfg("", hw_ip, &test_ctx.frame, 0, test_ctx.frame.num_buffers,
		&dma_cmd, plane, dst_dva, sc0TargetAddress);
	KUNIT_EXPECT_EQ(test, dma_cmd, 0);
	set_utc_result(KUTC_YUVP_DIS_DMA, UTC_ID_YUVP_DIS_DMA, dma_cmd == 0);
}

static struct kunit_case pablo_hw_yuvp_kunit_test_cases[] = {
	KUNIT_CASE(pablo_hw_yuvp_open_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_handle_interrupt0_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_handle_interrupt1_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_shot_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_enable_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_set_config_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_dump_regs_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_set_regs_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_notify_timeout_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_restore_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_setfile_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_get_meta_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_set_param_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_frame_ndone_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_reset_kunit_test),
	KUNIT_CASE(pablo_hw_yuvp_dma_cfg_kunit_test),
	{},
};

static void __setup_hw_ip(struct kunit *test)
{
	int ret;
	enum is_hardware_id hw_id = DEV_HW_YPP;
	struct is_interface *itf = NULL;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	struct is_interface_ischain *itfc = &test_ctx.itfc;

	hw_ip->hardware = &test_ctx.hardware;

	ret = is_hw_yuvp_probe(hw_ip, itf, itfc, hw_id, "YUVP");
	KUNIT_ASSERT_EQ(test, ret, 0);

	hw_ip->id = hw_id;
	snprintf(hw_ip->name, sizeof(hw_ip->name), "YUVP");
	hw_ip->itf = itf;
	hw_ip->itfc = itfc;
	atomic_set(&hw_ip->fcount, 0);
	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	atomic_set(&hw_ip->rsccount, 0);
	init_waitqueue_head(&hw_ip->status.wait_queue);
	hw_ip->state = 0;

	hw_ip->framemgr = &test_ctx.framemgr;

	test_ctx.org_hw_ops = (struct is_hw_ip_ops *)hw_ip->ops;
	test_ctx.hw_ops = *(hw_ip->ops);
	test_ctx.hw_ops.reset = __reset_stub;
	hw_ip->ops = &test_ctx.hw_ops;
}

static int pablo_hw_yuvp_kunit_test_init(struct kunit *test)
{
	int ret;

	test_ctx.hardware = is_get_is_core()->hardware;

	test_ctx.test_addr = kunit_kzalloc(test, 0x8000, 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, test_ctx.test_addr);

	test_ctx.hw_ip.regs[REG_SETA] = test_ctx.test_addr;

	ret = is_mem_init(&test_ctx.mem, is_get_is_core()->pdev);
	KUNIT_ASSERT_EQ(test, ret, 0);

	__setup_hw_ip(test);

	return 0;
}

static void pablo_hw_yuvp_kunit_test_exit(struct kunit *test)
{
	kunit_kfree(test, test_ctx.test_addr);
	memset(&test_ctx, 0, sizeof(test_ctx));
}

struct kunit_suite pablo_hw_yuvp_kunit_test_suite = {
	.name = "pablo-hw-yuvp-v2-kunit-test",
	.init = pablo_hw_yuvp_kunit_test_init,
	.exit = pablo_hw_yuvp_kunit_test_exit,
	.test_cases = pablo_hw_yuvp_kunit_test_cases,
};
define_pablo_kunit_test_suites(&pablo_hw_yuvp_kunit_test_suite);

MODULE_LICENSE("GPL");
