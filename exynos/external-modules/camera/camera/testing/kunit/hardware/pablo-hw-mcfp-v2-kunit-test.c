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
#include "is-hw-mcfp.h"

static struct pablo_hw_mcfp_kunit_test_ctx {
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
	struct mcfp_param	mcfp_param;
	struct is_region	region;
	void			*test_addr;
	struct is_hw_ip_ops	hw_ops;
	struct is_hw_ip_ops     *org_hw_ops;
	struct pablo_hw_helper_ops helper_ops;
} test_ctx;

static struct param_dma_input test_dma_input = {
	.cmd = DMA_INPUT_COMMAND_ENABLE,
	.format = DMA_INOUT_FORMAT_YUV422,
	.bitwidth = DMA_INOUT_BIT_WIDTH_12BIT,
	.plane = DMA_INOUT_PLANE_2,
	.width = 1920,
	.height = 1440,
	.dma_crop_offset = 0,
	.dma_crop_width = 1920,
	.dma_crop_height = 1440,
	.bayer_crop_width = 1920,
	.bayer_crop_height = 1440,
	.msb = DMA_INOUT_BIT_WIDTH_12BIT - 1,
};

/* Define the test cases. */

static void pablo_hw_mcfp_open_kunit_test(struct kunit *test)
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

static void pablo_hw_mcfp_handle_interrupt_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	struct is_interface_ischain *itfc = &test_ctx.itfc;
	int hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, hw_ip->id);

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler);

	/* not opened */
	ret = itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* overflow recorvery */
	set_bit(HW_OVERFLOW_RECOVERY, &hw_ip->hardware->hw_recovery_flag);
	ret = itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);
	clear_bit(HW_OVERFLOW_RECOVERY, &hw_ip->hardware->hw_recovery_flag);

	/* not run */
	ret = itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	set_bit(HW_RUN, &hw_ip->state);
	*(u32 *)(test_ctx.test_addr + 0x0800) = 0xFFFFFFFF;
	*(u32 *)(test_ctx.test_addr + 0x0804) = 0xFFFFFFFF;
	ret = itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler(0, hw_ip);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static bool __set_rta_regs(struct is_hw_ip *hw_ip, u32 instance, u32 set_id, bool skip,
		struct is_frame *frame, void *buf)
{
	return true;
}

static int __reset_stub(struct is_hw_ip *hw_ip, u32 instance)
{
	return 0;
}

static int __wait_idle_stub(struct is_hw_ip *hw_ip, u32 instance)
{
	return 0;
}

static void __open_init_setparam(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map,
				 struct kunit *test)
{
	int ret;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, init, instance, false, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, set_param, &test_ctx.region, test_ctx.frame.pmap, instance,
			    hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	hw_ip->region[instance] = &test_ctx.region;
}

static void __setup_frame_4_shot(void)
{
	test_ctx.frame.shot = &test_ctx.shot;
	test_ctx.frame.shot_ext = &test_ctx.shot_ext;
	test_ctx.frame.parameter = &test_ctx.parameter;

	set_bit(PARAM_MCFP_OTF_INPUT, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_OTF_OUTPUT, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_DMA_INPUT, test_ctx.frame.pmap);
	set_bit(CHAIN_STRIPE_PROCESSING, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_PREV_YUV, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_PREV_W, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_CUR_W, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_DRC, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_DP, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_MV, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_MVMIXER, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_SF, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_W, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_YUV, test_ctx.frame.pmap);
}

static void __deinit_close(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map, struct kunit *test)
{
	int ret;

	ret = CALL_HWIP_OPS(hw_ip, deinit, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_mcfp_shot_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	ulong hw_map = 0;
	struct is_hw_mcfp *hw_mcfp;
	struct is_mcfp_config *config;
	struct mcfp_param_set *param_set;
	struct is_param_region *param_region;
	enum is_hw_mcfp_subdev buf_id;

	set_bit(hw_ip->id, &hw_map);

	__open_init_setparam(hw_ip, instance, hw_map, test);

	hw_mcfp = (struct is_hw_mcfp *)hw_ip->priv_info;
	config = &hw_mcfp->config[instance];
	config->mixer_enable = 1;
	config->geomatch_bypass = 1;
	config->img_bit = 12;
	param_region = &test_ctx.parameter;
	param_region->mcfp.yuv.width = 320;
	param_region->mcfp.yuv.height = 240;

	__setup_frame_4_shot();

	is_hw_mcfp_s_debug_type(MCFP_DBG_DTP);

	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	is_hw_mcfp_c_debug_type(MCFP_DBG_DTP);

	is_hw_mcfp_s_debug_type(MCFP_DBG_TNR);

	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	config->mixer_mode = MCFP_TNR_MODE_FIRST;
	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	config->mixer_mode = MCFP_TNR_MODE_NORMAL;
	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	config->still_en = 1;
	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	for (buf_id = 0; buf_id < MCFP_SUBDEV_END; buf_id++)
		set_bit(PABLO_SUBDEV_ALLOC, &hw_mcfp->subdev[instance][buf_id].state);

	config->mixer_enable = 0;
	param_set = &hw_mcfp->param_set[instance];
	param_set->stripe_input.total_count = 2;

	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	is_hw_mcfp_c_debug_type(MCFP_DBG_TNR);

	__deinit_close(hw_ip, instance, hw_map, test);
}

static void pablo_hw_mcfp_enable_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	ulong hw_map = 0;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, init, instance, false, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	set_bit(hw_ip->id, &hw_map);

	is_set_debug_param(IS_DEBUG_PARAM_CRC_SEED, 55);

	ret = CALL_HWIP_OPS(hw_ip, enable, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, disable, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	is_set_debug_param(IS_DEBUG_PARAM_CRC_SEED, 0);

	ret = CALL_HWIP_OPS(hw_ip, enable, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, disable, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, deinit, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_mcfp_set_config_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 chain_id = 0;
	u32 instance = 0;
	u32 fcount = 0;
	struct is_mcfp_config conf;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, set_config, chain_id, instance, fcount, &conf);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_mcfp_dump_regs_kunit_test(struct kunit *test)
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

static void pablo_hw_mcfp_set_regs_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 chain_id = 0;
	u32 instance = 0;
	u32 fcount = 0;

	ret = CALL_HWIP_OPS(hw_ip, set_regs, chain_id, instance, fcount, NULL, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_mcfp_notify_timeout_kunit_test(struct kunit *test)
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

static void pablo_hw_mcfp_restore_kunit_test(struct kunit *test)
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

static void pablo_hw_mcfp_setfile_kunit_test(struct kunit *test)
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

static void pablo_hw_mcfp_get_meta_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	ulong hw_map = 0;

	ret = CALL_HWIP_OPS(hw_ip, get_meta, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_mcfp_get_cap_meta_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	ulong hw_map = 0;

	ret = CALL_HWIP_OPS(hw_ip, get_cap_meta, hw_map, 0, 0, 0x100,
		(ulong)test_ctx.test_addr);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_mcfp_set_param_kunit_test(struct kunit *test)
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

static void pablo_hw_mcfp_frame_ndone_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	enum ShotErrorType type = IS_SHOT_UNKNOWN;

	ret = CALL_HWIP_OPS(hw_ip, frame_ndone, &test_ctx.frame, type);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_hw_mcfp_param_get_debug_mcfp_kunit_test(struct kunit *test)
{
	const struct kernel_param *kp;
	int ret, len, argc;
	u32 value;
	char **argv;
	char usage_str[256];
	const char *query_str = "type : BIT\nmax : 127\nvalue : 1";
	char *buffer = (char *)kunit_kzalloc(test, PAGE_SIZE, GFP_KERNEL);

	kp = is_hw_mcfp_get_debug_kernel_param_kunit_wrapper();
	KUNIT_EXPECT_NOT_ERR_OR_NULL(test, kp);

	/* set/get debug_param */
	kp->ops->set("1", kp);
	ret = kp->ops->get(buffer, kp);
	KUNIT_EXPECT_GT(test, ret, 0);
	ret = kstrtouint(buffer, 0, &value);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, value, 1);

	/* get type of debug_param */
	kp->ops->set("type", kp);
	len = kp->ops->get(buffer, kp);
	KUNIT_EXPECT_GT(test, len, 0);
	buffer[len - 1] = '\0';
	KUNIT_EXPECT_STREQ(test, buffer, "BIT");

	/* get max of debug_param */
	kp->ops->set("max", kp);
	ret = kp->ops->get(buffer, kp);
	KUNIT_EXPECT_GT(test, ret, 0);

	/* the content of buffer is "max 127"(0x7F) */
	argv = argv_split(GFP_KERNEL, buffer, &argc);
	KUNIT_EXPECT_STREQ(test, argv[0], "max");

	/* check if debug_mcfp bit mask is 0x3F */
	ret = kstrtouint(argv[1], 0, &value);
	KUNIT_EXPECT_EQ(test, value, 0x7F);

	/* get type, max, value of debug_param with query command */
	kp->ops->set("query", kp);
	len = kp->ops->get(buffer, kp);
	KUNIT_EXPECT_EQ(test, len, strlen(query_str));
	KUNIT_EXPECT_STREQ(test, query_str, buffer);

	/* get usage of debug_param */
	kp->ops->set("usage", kp);
	len = kp->ops->get(buffer, kp);
	KUNIT_EXPECT_GT(test, len, 0);

	len = scnprintf(usage_str, sizeof(usage_str),
		"debug_mcfp usage: echo [value|option] > <target param>");
	/* compare only the first line of the acquired buf */
	buffer[len] = '\0';
	KUNIT_EXPECT_STREQ(test, usage_str, buffer);

	/* get value again after getting max, type, usage */
	ret = kp->ops->get(buffer, kp);
	ret = kstrtouint(buffer, 0, &value);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, value, 1);

	/* reset debug param */
	kp->ops->set("0", kp);

	kunit_kfree(test, buffer);
}

static void pablo_hw_mcfp_reset_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;

	hw_ip->ops = test_ctx.org_hw_ops;
	ret = CALL_HWIP_OPS(hw_ip, reset, instance);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);
}

static void pablo_hw_mcfp_rdma_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	u32 instance = 0;
	ulong hw_map = 0;
	struct is_hw_mcfp *hw_mcfp;
	struct is_param_region *param_region;
	u32 comp_sbwc_en = 0;
	int i;
	bool utc_flag = false;

	set_bit(hw_ip->id, &hw_map);

	__open_init_setparam(hw_ip, instance, hw_map, test);

	hw_mcfp = (struct is_hw_mcfp *)hw_ip->priv_info;
	param_region = &test_ctx.parameter;
	param_region->mcfp.dma_input = test_dma_input;

	__setup_frame_4_shot();

	for (i = NONE; i <= COMP_LOSS; i++) {
		param_region->mcfp.dma_input.sbwc_type = i; /* NONE == 0, COMP==1, COMP_LOSS==2 */
		ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
		KUNIT_EXPECT_EQ(test, ret, 0);
		if (!ret) {
			if (i == NONE)
				utc_flag = true;
			CALL_DMA_OPS(&hw_mcfp->rdma[MCFP_RDMA_CUR_IN_Y], dma_get_comp_sbwc_type,
				     &comp_sbwc_en);
			KUNIT_EXPECT_EQ(test, comp_sbwc_en, i);
			utc_flag = utc_flag && (comp_sbwc_en == i);
			CALL_DMA_OPS(&hw_mcfp->rdma[MCFP_RDMA_CUR_IN_UV], dma_get_comp_sbwc_type,
				     &comp_sbwc_en);
			KUNIT_EXPECT_EQ(test, comp_sbwc_en, i);
			utc_flag = utc_flag && (comp_sbwc_en == i);
		} else {
			utc_flag = false;
			break;
		}
	}

	__deinit_close(hw_ip, instance, hw_map, test);
	set_utc_result(KUTC_MCFP_RDMA_SBWC, UTC_ID_MCFP_RDMA_SBWC, utc_flag);
}

#if IS_ENABLED(ENABLE_RECURSIVE_NR)
static void pablo_hw_mcfp_recursive_nr_kunit_test(struct kunit *test)
{
	int ret;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	struct pablo_hw_helper_ops ops;
	u32 instance = 0;
	ulong hw_map = 0;
	struct is_hw_mcfp *hw_mcfp;
	struct is_mcfp_config *config;
	struct is_param_region *param_region;
	enum is_hw_mcfp_subdev buf_id;
	struct pablo_internal_subdev *sd;

	ret = CALL_HWIP_OPS(hw_ip, open, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, init, instance, false, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	set_bit(hw_ip->id, &hw_map);

	ret = CALL_HWIP_OPS(hw_ip, enable, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, set_param, &test_ctx.region, test_ctx.frame.pmap, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	test_ctx.frame.shot = &test_ctx.shot;
	test_ctx.frame.shot_ext = &test_ctx.shot_ext;
	test_ctx.frame.parameter = &test_ctx.parameter;
	ops.set_rta_regs = __set_rta_regs;
	hw_ip->help_ops = &ops;
	hw_ip->region[instance] = &test_ctx.region;

	hw_mcfp = (struct is_hw_mcfp *)hw_ip->priv_info;
	config = &hw_mcfp->config[instance];
	config->mixer_enable = 1;
	config->geomatch_bypass = 1;
	config->img_bit = 12;
	param_region = &test_ctx.parameter;
	param_region->mcfp.yuv.width = 320;
	param_region->mcfp.yuv.height = 240;

	set_bit(PARAM_MCFP_OTF_INPUT, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_OTF_OUTPUT, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_DMA_INPUT, test_ctx.frame.pmap);
	set_bit(CHAIN_STRIPE_PROCESSING, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_PREV_YUV, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_PREV_W, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_CUR_W, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_DRC, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_DP, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_MV, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_MVMIXER, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_SF, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_W, test_ctx.frame.pmap);
	set_bit(PARAM_MCFP_YUV, test_ctx.frame.pmap);

	is_hw_mcfp_s_debug_type(MCFP_DBG_TNR);

	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	config->mixer_mode = MCFP_TNR_MODE_FIRST;
	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	config->mixer_mode = MCFP_TNR_MODE_NORMAL;
	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* 2NR buffers are not yet allocated */
	for (buf_id = MCFP_SUBDEV_PREV_YUV_2NR; buf_id < MCFP_SUBDEV_MAX; buf_id++) {
		sd = &hw_mcfp->subdev[instance][buf_id];

		KUNIT_EXPECT_EQ(test, test_bit(PABLO_SUBDEV_ALLOC, &sd->state), 0);
	}

	/* 2nr test */
	test_ctx.frame.shot_ext->node_group.leader.recursiveNrType = NODE_RECURSIVE_NR_TYPE_2ND;
	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* 2NR buffers are allocated */
	for (buf_id = MCFP_SUBDEV_PREV_YUV_2NR; buf_id < MCFP_SUBDEV_MAX; buf_id++) {
		sd = &hw_mcfp->subdev[instance][buf_id];

		KUNIT_EXPECT_EQ(test, test_bit(PABLO_SUBDEV_ALLOC, &sd->state), 1);
	}

	test_ctx.frame.shot_ext->node_group.leader.iterationType = NODE_ITERATION_TYPE_NONE;
	config->mixer_mode = MCFP_TNR_MODE_FIRST;
	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* 2NR buffers are not yet freed */
	for (buf_id = MCFP_SUBDEV_PREV_YUV_2NR; buf_id < MCFP_SUBDEV_MAX; buf_id++) {
		sd = &hw_mcfp->subdev[instance][buf_id];

		KUNIT_EXPECT_EQ(test, test_bit(PABLO_SUBDEV_ALLOC, &sd->state), 1);
	}

	config->mixer_mode = MCFP_TNR_MODE_NORMAL;
	ret = CALL_HWIP_OPS(hw_ip, shot, &test_ctx.frame, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* 2NR buffers will not free until disable */
	for (buf_id = MCFP_SUBDEV_PREV_YUV_2NR; buf_id < MCFP_SUBDEV_MAX; buf_id++) {
		sd = &hw_mcfp->subdev[instance][buf_id];

		KUNIT_EXPECT_EQ(test, test_bit(PABLO_SUBDEV_ALLOC, &sd->state), 1);
	}

	is_hw_mcfp_c_debug_type(MCFP_DBG_TNR);

	ret = CALL_HWIP_OPS(hw_ip, disable, instance, hw_map);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* 2NR buffers freed at disable */
	for (buf_id = MCFP_SUBDEV_PREV_YUV_2NR; buf_id < MCFP_SUBDEV_MAX; buf_id++) {
		sd = &hw_mcfp->subdev[instance][buf_id];

		KUNIT_EXPECT_EQ(test, test_bit(PABLO_SUBDEV_ALLOC, &sd->state), 0);
	}

	ret = CALL_HWIP_OPS(hw_ip, deinit, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = CALL_HWIP_OPS(hw_ip, close, instance);
	KUNIT_EXPECT_EQ(test, ret, 0);
}
#endif

static struct kunit_case pablo_hw_mcfp_kunit_test_cases[] = {
	KUNIT_CASE(pablo_hw_mcfp_open_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_handle_interrupt_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_shot_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_enable_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_set_config_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_dump_regs_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_set_regs_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_notify_timeout_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_restore_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_setfile_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_get_meta_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_get_cap_meta_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_set_param_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_frame_ndone_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_param_get_debug_mcfp_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_reset_kunit_test),
	KUNIT_CASE(pablo_hw_mcfp_rdma_kunit_test),
#if IS_ENABLED(ENABLE_RECURSIVE_NR)
	KUNIT_CASE(pablo_hw_mcfp_recursive_nr_kunit_test),
#endif
	{},
};

static void __setup_hw_ip(struct kunit *test)
{
	int ret;
	enum is_hardware_id hw_id = DEV_HW_MCFP;
	struct is_interface *itf = NULL;
	struct is_hw_ip *hw_ip = &test_ctx.hw_ip;
	struct is_interface_ischain *itfc = &test_ctx.itfc;

	hw_ip->hardware = &test_ctx.hardware;

	ret = is_hw_mcfp_probe(hw_ip, itf, itfc, hw_id, "MCFP");
	KUNIT_ASSERT_EQ(test, ret, 0);

	hw_ip->id = hw_id;
	snprintf(hw_ip->name, sizeof(hw_ip->name), "MCFP");
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
	test_ctx.hw_ops.wait_idle = __wait_idle_stub;
	hw_ip->ops = &test_ctx.hw_ops;

	test_ctx.helper_ops.set_rta_regs = __set_rta_regs;
	hw_ip->help_ops = &test_ctx.helper_ops;
}

static int pablo_hw_mcfp_kunit_test_init(struct kunit *test)
{
	int ret;

	test_ctx.hardware = is_get_is_core()->hardware;

	test_ctx.test_addr = kunit_kzalloc(test, 0x8000, 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, test_ctx.test_addr);

	test_ctx.hw_ip.regs[REG_SETA] = test_ctx.test_addr;

	ret = is_mem_init(&test_ctx.mem, is_get_is_core()->pdev);
	KUNIT_ASSERT_EQ(test, ret, 0);

	ret = pablo_hw_chain_info_probe(&test_ctx.hardware);
	KUNIT_ASSERT_EQ(test, ret, 0);

	__setup_hw_ip(test);

	return 0;
}

static void pablo_hw_mcfp_kunit_test_exit(struct kunit *test)
{
	kunit_kfree(test, test_ctx.test_addr);

	memset(&test_ctx, 0, sizeof(test_ctx));
}

struct kunit_suite pablo_hw_mcfp_kunit_test_suite = {
	.name = "pablo-hw-mcfp-v2-kunit-test",
	.init = pablo_hw_mcfp_kunit_test_init,
	.exit = pablo_hw_mcfp_kunit_test_exit,
	.test_cases = pablo_hw_mcfp_kunit_test_cases,
};
define_pablo_kunit_test_suites(&pablo_hw_mcfp_kunit_test_suite);

MODULE_LICENSE("GPL");
