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

#include "is-hw-rgbp.h"
#include "is-err.h"
#include "api/is-hw-api-rgbp-v2_0.h"
#include "is-votfmgr.h"
#include "is-votf-id-table.h"
#include "votf/pablo-votf.h"
#include "pablo-hw-helper.h"
#include "is-stripe.h"
#include "is-hw-param-debug.h"
#include "pablo-json.h"

static inline void *get_base(struct is_hw_ip *hw_ip) {
	if (IS_ENABLED(RGBP_USE_MMIO))
		return hw_ip->regs[REG_SETA];
	return hw_ip->pmio;
}

static int param_debug_rgbp_usage(char *buffer, const size_t buf_size)
{
	const char *usage_msg = "[value] bit value, set debug param value\n"
				"\tb[0] : dump sfr\n"
				"\tb[1] : enable DTP\n";

	return scnprintf(buffer, buf_size, usage_msg);
}

static struct pablo_debug_param debug_rgbp = {
	.type = IS_DEBUG_PARAM_TYPE_BIT,
	.max_value = 0x3,
	.ops.usage = param_debug_rgbp_usage,
};
module_param_cb(debug_rgbp, &pablo_debug_param_ops, &debug_rgbp, 0644);

static int is_hw_rgbp_handle_interrupt0(u32 id, void *context)
{
	struct is_hw_ip *hw_ip = (struct is_hw_ip *)context;
	struct is_hardware *hardware = hw_ip->hardware;
	struct is_hw_rgbp *hw_rgbp = NULL;
	struct rgbp_param_set *param_set;
	u32 status, instance, hw_fcount, hl = 0, vl = 0;
	bool f_err = false;

	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);

	if (!test_bit(HW_OPEN, &hw_ip->state)) {
		mserr_hw("invalid interrupt, hw_ip state(0x%lx)", instance, hw_ip, hw_ip->state);
		return 0;
	}

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	param_set = &hw_rgbp->param_set[instance];

	status = rgbp_hw_g_int0_state(get_base(hw_ip), true,
			(hw_ip->num_buffers & 0xffff), &hw_rgbp->irq_state[RGBP_INTR0])
			& rgbp_hw_g_int0_mask(get_base(hw_ip));

	msdbg_hw(2, "RGBP interrupt status(0x%x)\n", instance, hw_ip, status);

	if (test_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag)) {
		mserr_hw("During recovery : invalid interrupt", instance, hw_ip);
		return 0;
	}

	if (!test_bit(HW_RUN, &hw_ip->state)) {
		mserr_hw("HW disabled!! interrupt(0x%x)", instance, hw_ip, status);
		return 0;
	}

	if (rgbp_hw_is_occurred0(status, INTR0_SETTING_DONE_INT))
		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount, DEBUG_POINT_SETTING_DONE);

	if (rgbp_hw_is_occurred0(status, INTR0_FRAME_START_INT) && rgbp_hw_is_occurred0(status, INTR0_FRAME_END_INT))
		mswarn_hw("start/end overlapped!! (0x%x)", instance, hw_ip, status);

	if (rgbp_hw_is_occurred0(status, INTR0_FRAME_START_INT)) {
		atomic_add((hw_ip->num_buffers & 0xffff), &hw_ip->count.fs);
		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount,
				DEBUG_POINT_FRAME_START);
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			msdbg_hw(2, "[F:%d]F.S\n", instance, hw_ip, hw_fcount);

		CALL_HW_OPS(hw_ip, frame_start, hw_ip, instance);
	}

	if (rgbp_hw_is_occurred0(status, INTR0_FRAME_END_INT)) {
		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount,
				DEBUG_POINT_FRAME_END);
		atomic_add((hw_ip->num_buffers & 0xffff), &hw_ip->count.fe);

		CALL_HW_OPS(hw_ip, frame_done, hw_ip, NULL, -1, IS_HW_CORE_END,
				IS_SHOT_SUCCESS, true);

		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			msdbg_hw(2, "[F:%d]F.E\n", instance, hw_ip, hw_fcount);

		if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
			mserr_hw("fs(%d), fe(%d), dma(%d), status(0x%x)",
					instance, hw_ip,
					atomic_read(&hw_ip->count.fs),
					atomic_read(&hw_ip->count.fe),
					atomic_read(&hw_ip->count.dma),
					status);
		}
		wake_up(&hw_ip->status.wait_queue);
	}

	f_err = rgbp_hw_is_occurred0(status, INTR0_ERR0);

	if (f_err) {
		msinfo_hw("[F:%d] Ocurred error interrupt0 (%d,%d) status(0x%x)\n",
			instance, hw_ip, hw_fcount, hl, vl, status);
		rgbp_hw_dump(hw_ip->pmio, HW_DUMP_CR);
		rgbp_hw_int0_error_handle(get_base(hw_ip));
	}

	return 0;

}

static int is_hw_rgbp_handle_interrupt1(u32 id, void *context)
{
	struct is_hw_ip *hw_ip = (struct is_hw_ip *)context;
	struct is_hardware *hardware = hw_ip->hardware;
	struct is_hw_rgbp *hw_rgbp = NULL;
	u32 status, instance, hw_fcount, strip_index, hl = 0, vl = 0;
	bool f_err = false;

	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);

	if (!test_bit(HW_OPEN, &hw_ip->state)) {
		mserr_hw("invalid interrupt, hw_ip state(0x%lx)", instance, hw_ip, hw_ip->state);
		return 0;
	}

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	strip_index = atomic_read(&hw_rgbp->strip_index);

	status = rgbp_hw_g_int1_state(get_base(hw_ip), true,
			(hw_ip->num_buffers & 0xffff), &hw_rgbp->irq_state[RGBP_INTR1])
			& rgbp_hw_g_int1_mask(get_base(hw_ip));

	msdbg_hw(2, "RGBP interrupt status1(0x%x)\n", instance, hw_ip, status);

	if (test_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag)) {
		mserr_hw("During recovery : invalid interrupt", instance, hw_ip);
		return 0;
	}

	if (!test_bit(HW_RUN, &hw_ip->state)) {
		mserr_hw("HW disabled!! interrupt(0x%x)", instance, hw_ip, status);
		return 0;
	}

	f_err = rgbp_hw_is_occurred1(status, INTR1_ERR1);

	if (f_err) {
		msinfo_hw("[F:%d] Ocurred error interrupt1 (%d,%d) status(0x%x)\n",
			instance, hw_ip, hw_fcount, hl, vl, status);
		rgbp_hw_dump(hw_ip->pmio, HW_DUMP_CR);
	}

	return 0;

}

static int is_hw_rgbp_reset(struct is_hw_ip *hw_ip, u32 instance)
{
	msinfo_hw("reset\n", instance, hw_ip);

	if (rgbp_hw_s_reset(get_base(hw_ip))) {
		mserr_hw("sw reset fail", instance, hw_ip);
		return -ENODEV;
	}

	return 0;
}

static void is_hw_rgbp_prepare(struct is_hw_ip *hw_ip, u32 instance)
{
	rgbp_hw_s_init(get_base(hw_ip));
}

static int is_hw_rgbp_finish(struct is_hw_ip *hw_ip, u32 instance)
{
	int res;

	res = is_hw_rgbp_reset(hw_ip, instance);
	if (res)
		return res;

	res = rgbp_hw_wait_idle(get_base(hw_ip));
	if (res)
		mserr_hw("failed to rgbp_hw_wait_idle", instance, hw_ip);

	msinfo_hw("final finished rgbp\n", instance, hw_ip);

	return res;
}

static int is_hw_rgbp_set_cmdq(struct is_hw_ip *hw_ip, u32 instance, u32 set_id, u32 num_buffers, struct c_loader_buffer *clb)
{
	/*
	 * add to command queue
	 * 1. set corex_update_type_0_setX
	 * normal shot: SETA(wide), SETB(tele, front), SETC(uw)
	 * reprocessing shot: SETD
	 * 2. set cq_frm_start_type to 0. only at first time?
	 * 3. set ms_add_to_queue(set id, frame id).
	 */
	int ret = 0;

	FIMC_BUG(!hw_ip);

	/*
	 * TODO : enable corex
	ret = rgbp_hw_s_corex_update_type(get_base(hw_ip), set_id, COREX_COPY);
	if (ret)
		return ret;
	*/

	if (clb)
		rgbp_hw_s_cmdq(get_base(hw_ip), set_id, num_buffers, clb->header_dva, clb->num_of_headers);
	else
		rgbp_hw_s_cmdq(get_base(hw_ip), set_id, num_buffers, 0, 0);

	return ret;
}

static int __nocfi is_hw_rgbp_open(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct is_hw_rgbp *hw_rgbp = NULL;
	struct is_mem *mem;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, "HWRGBP");
	frame_manager_open(hw_ip->framemgr, IS_MAX_HW_FRAME, false);

	hw_ip->priv_info = vzalloc(sizeof(struct is_hw_rgbp));
	if (!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	hw_rgbp->instance = instance;

	ret = CALL_HW_HELPER_OPS(hw_ip, open, instance, &hw_rgbp->lib[instance],
				LIB_FUNC_RGBP);
	if (ret)
		goto err_chain_create;

	ret = CALL_HW_HELPER_OPS(hw_ip, alloc_iqset, rgbp_hw_g_reg_cnt());
	if (ret)
		goto err_iqset_alloc;

	hw_rgbp->rdma_max_cnt = rgbp_hw_g_rdma_max_cnt();
	hw_rgbp->wdma_max_cnt = rgbp_hw_g_wdma_max_cnt();
	hw_rgbp->rdma_param_max_cnt = rgbp_hw_g_rdma_cfg_max_cnt();
	hw_rgbp->wdma_param_max_cnt = rgbp_hw_g_wdma_cfg_max_cnt();
	hw_rgbp->rdma = vzalloc(sizeof(struct is_common_dma) * hw_rgbp->rdma_max_cnt);
	hw_rgbp->wdma = vzalloc(sizeof(struct is_common_dma) * hw_rgbp->wdma_max_cnt);

	atomic_set(&hw_ip->status.Vvalid, V_BLANK);

	mem = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_iommu_mem, GROUP_ID_RGBP);

	hw_rgbp->pb_c_loader_payload = CALL_PTR_MEMOP(mem, alloc, mem->priv,
							0x8000, NULL, 0);
	if (IS_ERR_OR_NULL(hw_rgbp->pb_c_loader_payload)) {
		err("failed to allocate buffer for c-loader payload");
		hw_rgbp->pb_c_loader_payload = NULL;
		return -ENOMEM;
	}
	hw_rgbp->kva_c_loader_payload = CALL_BUFOP(hw_rgbp->pb_c_loader_payload,
			kvaddr, hw_rgbp->pb_c_loader_payload);
	hw_rgbp->dva_c_loader_payload = CALL_BUFOP(hw_rgbp->pb_c_loader_payload,
			dvaddr, hw_rgbp->pb_c_loader_payload);

	hw_rgbp->pb_c_loader_header = CALL_PTR_MEMOP(mem, alloc,
			mem->priv, 0x2000,
			NULL, 0);
	if (IS_ERR_OR_NULL(hw_rgbp->pb_c_loader_header)) {
		err("failed to allocate buffer for c-loader header");
		hw_rgbp->pb_c_loader_header = NULL;
		return -ENOMEM;
	}
	hw_rgbp->kva_c_loader_header = CALL_BUFOP(hw_rgbp->pb_c_loader_header,
			kvaddr, hw_rgbp->pb_c_loader_header);
	hw_rgbp->dva_c_loader_header = CALL_BUFOP(hw_rgbp->pb_c_loader_header,
			dvaddr, hw_rgbp->pb_c_loader_header);

	set_bit(HW_OPEN, &hw_ip->state);

	msdbg_hw(2, "open: framemgr[%s]", instance, hw_ip, hw_ip->framemgr->name);

	return 0;

err_iqset_alloc:
	CALL_HW_HELPER_OPS(hw_ip, close, instance, &hw_rgbp->lib[instance]);

err_chain_create:
	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
err_alloc:
	frame_manager_close(hw_ip->framemgr);
	return ret;
}

static int is_hw_rgbp_init(struct is_hw_ip *hw_ip, u32 instance,
			bool flag, u32 f_type)
{
	int ret = 0;
	struct is_hw_rgbp *hw_rgbp = NULL;
	u32 idx;
	u32 rdma_max_cnt, wdma_max_cnt;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	if (!hw_rgbp) {
		mserr_hw("hw_rgbp is null ", instance, hw_ip);
		ret = -ENODATA;
		goto err;
	}

	ret = CALL_HW_HELPER_OPS(hw_ip, init, instance, &hw_rgbp->lib[instance],
				(u32)flag, f_type, LIB_FUNC_RGBP);
	if (ret)
		return ret;

	rdma_max_cnt = hw_rgbp->rdma_max_cnt;
	wdma_max_cnt = hw_rgbp->wdma_max_cnt;

	/* set RDMA */
	for (idx = 0; idx < rdma_max_cnt; idx++) {
		ret = rgbp_hw_rdma_create(&hw_rgbp->rdma[idx], get_base(hw_ip), idx);
		if (ret) {
			mserr_hw("rgbp_hw_rdma_create error[%d]", instance, hw_ip, idx);
			ret = -ENODATA;
			goto err;
		}
	}

	/* set WDMA */
	for (idx = 0; idx < wdma_max_cnt; idx++) {
		ret = rgbp_hw_wdma_create(&hw_rgbp->wdma[idx], get_base(hw_ip), idx);
		if (ret) {
			mserr_hw("rgbp_hw_wdma_create error[%d]", instance, hw_ip, idx);
			ret = -ENODATA;
			goto err;
		}
	}

	set_bit(HW_INIT, &hw_ip->state);
	return 0;

err:
	return ret;

}

static int is_hw_rgbp_deinit(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_rgbp *hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, deinit, instance, &hw_rgbp->lib[instance]);
}

static int is_hw_rgbp_close(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_rgbp *hw_rgbp = NULL;
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	CALL_HW_HELPER_OPS(hw_ip, close, instance, &hw_rgbp->lib[instance]);

	is_hw_rgbp_finish(hw_ip, instance);

	CALL_BUFOP(hw_rgbp->pb_c_loader_payload, free, hw_rgbp->pb_c_loader_payload);
	CALL_BUFOP(hw_rgbp->pb_c_loader_header, free, hw_rgbp->pb_c_loader_header);

	CALL_HW_HELPER_OPS(hw_ip, free_iqset);

	vfree(hw_rgbp->rdma);
	vfree(hw_rgbp->wdma);

	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;

	frame_manager_close(hw_ip->framemgr);
	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int is_hw_rgbp_enable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	if (test_bit(HW_RUN, &hw_ip->state))
		return 0;

	msdbg_hw(2, "enable: start\n", instance, hw_ip);

	is_hw_rgbp_reset(hw_ip, instance);
	if (!IS_ENABLED(RGBP_USE_MMIO)) {
		hw_ip->pmio_config.cache_type = PMIO_CACHE_FLAT;
		if (pmio_reinit_cache(hw_ip->pmio, &hw_ip->pmio_config)) {
			pmio_cache_set_bypass(hw_ip->pmio, true);
			err("failed to reinit PMIO cache, set bypass");
			return -EINVAL;
		}
	}
	is_hw_rgbp_prepare(hw_ip, instance);

	if (!IS_ENABLED(RGBP_USE_MMIO))
		pmio_cache_set_only(hw_ip->pmio, true);

	set_bit(HW_RUN, &hw_ip->state);
	msdbg_hw(2, "enable: done\n", instance, hw_ip);

	return ret;
}

static int is_hw_rgbp_disable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	long timetowait;
	struct is_hw_rgbp *hw_rgbp;
	struct rgbp_param_set *param_set;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	msinfo_hw("rgbp_disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	FIMC_BUG(!hw_ip->priv_info);
	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	timetowait = wait_event_timeout(hw_ip->status.wait_queue,
		!atomic_read(&hw_ip->status.Vvalid),
		IS_HW_STOP_TIMEOUT);

	if (!timetowait) {
		mserr_hw("wait FRAME_END timeout (%ld)", instance,
			hw_ip, timetowait);
		ret = -ETIME;
	}

	param_set = &hw_rgbp->param_set[instance];
	param_set->fcount = 0;

	CALL_HW_HELPER_OPS(hw_ip, disable, instance, &hw_rgbp->lib[instance]);

	if (hw_ip->run_rsc_state)
		return 0;

	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int __is_hw_rgbp_set_rdma(struct is_hw_ip *hw_ip, struct is_hw_rgbp *hw_rgbp, struct rgbp_param_set *param_set,
										u32 instance, u32 id, u32 set_id)
{
	pdma_addr_t *input_dva = NULL;
	u32 cmd, in_rgb = 0;
	u32 comp_sbwc_en, payload_size;
	u32 in_crop_size_x = 0;
	u32 cache_hint = 0x7;
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_rgbp);
	FIMC_BUG(!param_set);

	input_dva = rgbp_hw_g_input_dva(param_set, instance, id, &cmd, &in_rgb);

	msdbg_hw(2, "%s: %d\n", instance, hw_ip, hw_rgbp->rdma[id].name, cmd);

	rgbp_hw_s_rdma_corex_id(&hw_rgbp->rdma[id], set_id);

	ret = rgbp_hw_s_rdma_init(hw_ip, &hw_rgbp->rdma[id], param_set, cmd,
		in_crop_size_x, cache_hint,
		&comp_sbwc_en, &payload_size, in_rgb);
	if (ret) {
		mserr_hw("failed to initialize RGBP_DMA(%d)", instance, hw_ip, id);
		return -EINVAL;
	}

	if (cmd == DMA_INPUT_COMMAND_ENABLE) {
		ret = rgbp_hw_s_rdma_addr(&hw_rgbp->rdma[id], input_dva, 0, (hw_ip->num_buffers & 0xffff),
			0, comp_sbwc_en, payload_size, in_rgb);
		if (ret) {
			mserr_hw("failed to set RGBP_RDMA(%d) address", instance, hw_ip, id);
			return -EINVAL;
		}
	}

	return 0;
}

static int __is_hw_rgbp_set_wdma(struct is_hw_ip *hw_ip, struct is_hw_rgbp *hw_rgbp, struct rgbp_param_set *param_set,
							u32 instance, u32 id, u32 set_id, struct is_frame *dma_frame)
{
	pdma_addr_t *output_dva = NULL;
	u32 cmd, out_yuv = 0;
	u32 cache_hint = IS_LLC_CACHE_HINT_VOTF_TYPE;
	int ret = 0;
	struct rgbp_dma_cfg dma_cfg;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_rgbp);
	FIMC_BUG(!param_set);

	output_dva = rgbp_hw_g_output_dva(param_set, instance, id, &cmd, &out_yuv);

	msdbg_hw(2, "%s: %d\n", instance, hw_ip, hw_rgbp->wdma[id].name, cmd);

	rgbp_hw_s_wdma_corex_id(&hw_rgbp->wdma[id], set_id);

	dma_cfg.enable = cmd;
	dma_cfg.cache_hint = cache_hint;
	dma_cfg.out_yuv = out_yuv;
	dma_cfg.num_buffers = hw_ip->num_buffers & 0xffff;

	ret = rgbp_hw_s_wdma_init(&hw_rgbp->wdma[id], get_base(hw_ip), set_id, param_set, output_dva, &dma_cfg);
	if (ret) {
		mserr_hw("failed to initialize RGBP_DMA(%d)", instance, hw_ip, id);
		return -EINVAL;
	}

	return 0;
}


static void __is_hw_rgbp_check_size(struct is_hw_ip *hw_ip, struct rgbp_param_set *param_set, u32 instance)
{
	struct param_dma_output *output_dma = &param_set->dma_output_yuv;
	struct param_otf_input *input = &param_set->otf_input;
	struct param_otf_output *output = &param_set->otf_output;

	FIMC_BUG_VOID(!param_set);

	msdbgs_hw(2, "hw_rgbp_check_size >>>\n", instance, hw_ip);
	msdbgs_hw(2, "otf_input: pos(%d,%d),crop%dx%d),size(%dx%d)\n", instance, hw_ip,
		input->bayer_crop_offset_x, input->bayer_crop_offset_y,
		input->bayer_crop_width, input->bayer_crop_height,
		input->width, input->height);
	msdbgs_hw(2, "otf_output: pos(%d,%d),crop%dx%d),size(%dx%d)\n", instance, hw_ip,
		output->crop_offset_x, output->crop_offset_y,
		output->crop_width, output->crop_height,
		output->width, output->height);
	msdbgs_hw(2, "dma_output: format(%d),crop_size(%dx%d)\n", instance, hw_ip,
		output_dma->format, output_dma->dma_crop_width, output_dma->dma_crop_height);
	msdbgs_hw(2, "<<< hw_rgbp_check_size <<<\n", instance, hw_ip);
}

static int __is_hw_rgbp_bypass(struct is_hw_ip *hw_ip, u32 set_id)
{
	FIMC_BUG(!hw_ip);

	rgbp_hw_s_block_bypass(get_base(hw_ip), set_id);

	return 0;
}

static int __is_hw_rgbp_update_block_reg(struct is_hw_ip *hw_ip, struct rgbp_param_set *param_set,
									u32 instance, u32 set_id)
{
	struct is_hw_rgbp *hw_rgbp;
	int ret = 0;

	msdbg_hw(4, "%s\n", instance, hw_ip, __func__);

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	if (hw_rgbp->instance != instance) {
		msdbg_hw(2, "update_param: hw_ip->instance(%d)\n",
			instance, hw_ip, hw_rgbp->instance);
		hw_rgbp->instance = instance;
	}

	__is_hw_rgbp_check_size(hw_ip, param_set, instance);
	ret = __is_hw_rgbp_bypass(hw_ip, set_id);

	return ret;
}

static void __is_hw_rgbp_update_param(struct is_hw_ip *hw_ip, struct is_param_region *p_region,
					struct rgbp_param_set *param_set, IS_DECLARE_PMAP(pmap),
					u32 instance)
{
	struct rgbp_param *param;

	FIMC_BUG_VOID(!hw_ip);
	FIMC_BUG_VOID(!param_set);

	param = &p_region->rgbp;
	param_set->instance_id = instance;

	param_set->mono_mode = hw_ip->region[instance]->parameter.sensor.config.mono_mode;

	param_set->byrp_param = &p_region->byrp;

	rgbp_hw_update_param(param, pmap, param_set);
}

static int is_hw_rgbp_set_param(struct is_hw_ip *hw_ip, struct is_region *region,
		IS_DECLARE_PMAP(pmap), u32 instance, ulong hw_map)
{
	struct is_hw_rgbp *hw_rgbp;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!region);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	hw_ip->region[instance] = region;

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	hw_rgbp->instance = IS_STREAM_COUNT;

	return 0;
}

static void is_hw_rgbp_s_size_regs(struct is_hw_ip *hw_ip, struct rgbp_param_set *param_set,
	u32 instance, const struct is_frame *frame)
{
	struct is_hw_rgbp *hw;
	struct rgbp_grid_cfg grid_cfg;
	struct is_rgbp_config *cfg;
	u32 strip_enable, start_pos_x, start_pos_y;
	u32 sensor_full_width, sensor_full_height;
	u32 sensor_binning_x, sensor_binning_y;
	u32 sensor_crop_x, sensor_crop_y;
	u32 bns_binning_x, bns_binning_y;
	u32 sw_binning_x, sw_binning_y;

	hw = (struct is_hw_rgbp *)hw_ip->priv_info;
	cfg = &hw->config;

	/* GRID configuration for LSC */
	strip_enable = (param_set->stripe_input.total_count == 0) ? 0 : 1;

	sensor_full_width = frame->shot->udm.frame_info.sensor_size[0];
	sensor_full_height = frame->shot->udm.frame_info.sensor_size[1];
	sensor_crop_x = frame->shot->udm.frame_info.sensor_crop_region[0];
	sensor_crop_y = frame->shot->udm.frame_info.sensor_crop_region[1];
	sensor_binning_x = frame->shot->udm.frame_info.sensor_binning[0];
	sensor_binning_y = frame->shot->udm.frame_info.sensor_binning[1];
	bns_binning_x = frame->shot->udm.frame_info.bns_binning[0];
	bns_binning_y = frame->shot->udm.frame_info.bns_binning[1];
	if (frame->shot_ext) {
		sw_binning_x = frame->shot_ext->binning_ratio_x ?
					frame->shot_ext->binning_ratio_x : 1000;
		sw_binning_y = frame->shot_ext->binning_ratio_y ?
					frame->shot_ext->binning_ratio_y : 1000;
	} else {
		sw_binning_x = 1000;
		sw_binning_y = 1000;
	}

	/* Center */
	if (cfg->sensor_center_x == 0 || cfg->sensor_center_y == 0) {
		cfg->sensor_center_x = sensor_full_width >> 1;
		cfg->sensor_center_y = sensor_full_height >> 1;
		msdbg_hw(2, "%s: cal_center(0,0) from DDK. Fix to (%d,%d)",
				instance, hw_ip, __func__,
				cfg->sensor_center_x, cfg->sensor_center_y);
	}

	/* Total_binning = sensor_binning * csis_bns_binning */
	grid_cfg.binning_x = (1024ULL * sensor_binning_x * bns_binning_x * sw_binning_x) / 1000 / 1000 / 1000;
	grid_cfg.binning_y = (1024ULL * sensor_binning_y * bns_binning_y * sw_binning_y) / 1000 / 1000 / 1000;
	if (grid_cfg.binning_x == 0 || grid_cfg.binning_y == 0) {
		grid_cfg.binning_x = 1024;
		grid_cfg.binning_y = 1024;
		msdbg_hw(2, "%s:[LSC] calculated binning(0,0). Fix to (%d,%d)",
				instance, hw_ip, __func__, 1024, 1024);
	}

	/* Step */
	grid_cfg.step_x = cfg->lsc_step_x;
	grid_cfg.step_y = cfg->lsc_step_y;

	/* Total_crop = unbinned(Stripe_start_position + BYRP_bcrop) + unbinned(sensor_crop) */
	sensor_crop_x = frame->shot->udm.frame_info.sensor_crop_region[0];
	sensor_crop_y = frame->shot->udm.frame_info.sensor_crop_region[1];
	start_pos_x = (strip_enable) ? param_set->stripe_input.start_pos_x : 0;
	start_pos_x = (((start_pos_x + param_set->byrp_param->dma_input.bayer_crop_offset_x) *
			grid_cfg.binning_x) / 1024) +
			(sensor_crop_x * sensor_binning_x) / 1000;
	start_pos_y = ((param_set->byrp_param->dma_input.bayer_crop_offset_y * grid_cfg.binning_y) / 1024) +
			(sensor_crop_y * sensor_binning_y) / 1000;
	grid_cfg.crop_x = start_pos_x * grid_cfg.step_x;
	grid_cfg.crop_y = start_pos_y * grid_cfg.step_y;
	grid_cfg.crop_radial_x = (u16)((-1) * (cfg->sensor_center_x - start_pos_x));
	grid_cfg.crop_radial_y = (u16)((-1) * (cfg->sensor_center_y - start_pos_y));

	rgbp_hw_s_grid_cfg(get_base(hw_ip), &grid_cfg);

	msdbg_hw(2, "%s:[LSC] stripe: enable(%d), pos_x(%d), full_size(%dx%d)\n",
			instance, hw_ip, __func__,
			strip_enable, param_set->stripe_input.start_pos_x,
			param_set->stripe_input.full_width,
			param_set->stripe_input.full_height);

	msdbg_hw(2, "%s:[LSC] dbg: calibrated_size(%dx%d), cal_center(%d,%d), sensor_crop(%d,%d), start_pos(%d,%d)\n",
			instance, hw_ip, __func__,
			sensor_full_width, sensor_full_height,
			cfg->sensor_center_x,
			cfg->sensor_center_y,
			sensor_crop_x, sensor_crop_y,
			start_pos_x, start_pos_y);

	msdbg_hw(2, "%s:[LSC] sfr: binning(%dx%d), step(%dx%d), crop(%d,%d), crop_radial(%d,%d)\n",
			instance, hw_ip, __func__,
			grid_cfg.binning_x,
			grid_cfg.binning_y,
			grid_cfg.step_x,
			grid_cfg.step_y,
			grid_cfg.crop_x,
			grid_cfg.crop_y,
			grid_cfg.crop_radial_x,
			grid_cfg.crop_radial_y);
}

static int __is_hw_rgbp_set_size_regs(struct is_hw_ip *hw_ip, struct rgbp_param_set *param_set,
	u32 instance, u32 rgbp_strip_start_pos, const struct is_frame *frame, u32 set_id)
{
	struct is_hw_rgbp *hw_rgbp;
	struct is_rgbp_config *config;
	struct is_hw_rgbp_sc_size yuvsc_size, upsc_size;
	struct rgbp_radial_cfg radial_cfg;
	int ret = 0;
	u32 strip_enable;
	u32 stripe_index;
	u32 strip_start_pos_x;
	u32 in_width, in_height;
	u32 out_width, out_height, out_x;
	u32 vratio, hratio;
	u32 yuvsc_enable, yuvsc_bypass;
	u32 upsc_enable, upsc_bypass;
	u32 decomp_enable, decomp_bypass;
	u32 gamma_enable, gamma_bypass;
	u32 hf_mode;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param_set);

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	config = &hw_rgbp->config;

	in_width = param_set->otf_input.width;
	in_height = param_set->otf_input.height;
	if (param_set->otf_output.cmd) {
		out_x = 0;
		out_width = param_set->otf_output.width;
		out_height = param_set->otf_output.height;
	} else if (param_set->dma_output_yuv.cmd) {
		out_x = param_set->dma_output_yuv.dma_crop_offset_x;
		out_width = param_set->otf_output.width;
		out_height = param_set->otf_output.height;
	} else {
		out_x = 0;
		out_width = in_width;
		out_height = in_height;
	}

	rgbp_hw_s_chain_src_size(get_base(hw_ip), set_id, in_width, in_height);
	rgbp_hw_s_chain_dst_size(get_base(hw_ip), set_id, out_width, out_height);

	rgbp_hw_s_dtp_output_size(get_base(hw_ip), set_id, in_width, in_height);

	strip_enable = (param_set->stripe_input.total_count < 2) ? 0 : 1;
	stripe_index = param_set->stripe_input.index;
	strip_start_pos_x = (stripe_index) ?
		(param_set->stripe_input.start_pos_x - param_set->stripe_input.left_margin) : 0;

	radial_cfg.sensor_full_width = frame->shot->udm.frame_info.sensor_size[0];
	radial_cfg.sensor_full_height = frame->shot->udm.frame_info.sensor_size[1];
	radial_cfg.sensor_crop_x = frame->shot->udm.frame_info.sensor_crop_region[0];
	radial_cfg.sensor_crop_y = frame->shot->udm.frame_info.sensor_crop_region[1];
	radial_cfg.taa_crop_x = frame->shot->udm.frame_info.taa_in_crop_region[0];
	radial_cfg.taa_crop_y = frame->shot->udm.frame_info.taa_in_crop_region[1];
	radial_cfg.taa_crop_width = frame->shot->udm.frame_info.taa_in_crop_region[2];
	radial_cfg.taa_crop_height = frame->shot->udm.frame_info.taa_in_crop_region[3];
	radial_cfg.sensor_binning_x = frame->shot->udm.frame_info.sensor_binning[0];
	radial_cfg.sensor_binning_y = frame->shot->udm.frame_info.sensor_binning[1];
	radial_cfg.bns_binning_x = frame->shot->udm.frame_info.bns_binning[0];
	radial_cfg.bns_binning_y = frame->shot->udm.frame_info.bns_binning[1];

	rgbp_hw_s_dns_size(get_base(hw_ip), set_id, in_width, in_height,
			strip_enable, strip_start_pos_x, &radial_cfg, config);

	yuvsc_size.input_h_size = in_width;
	yuvsc_size.input_v_size = in_height;
	yuvsc_size.dst_h_size = out_width;
	yuvsc_size.dst_v_size = out_height;

	yuvsc_enable = 0x1;
	yuvsc_bypass = 0x1;
	if ((in_width != out_width) || (in_height != out_height)) {
		msdbgs_hw(2, "[F:%d][%s] YUVSC enable incrop(%d, %d), otcrop(%d, %d)\n",
					instance, hw_ip, frame->fcount, __func__,
					in_width, in_height, out_width, out_height);
		yuvsc_bypass = 0x0;
	}

	rgbp_hw_s_yuvsc_enable(get_base(hw_ip), set_id, yuvsc_enable, yuvsc_bypass);
	rgbp_hw_s_yuvsc_dst_size(get_base(hw_ip), set_id, yuvsc_size.dst_h_size, yuvsc_size.dst_v_size);

	hratio = ((u64)yuvsc_size.input_h_size << 20) / yuvsc_size.dst_h_size;
	vratio = ((u64)yuvsc_size.input_v_size << 20) / yuvsc_size.dst_v_size;

	rgbp_hw_s_yuvsc_scaling_ratio(get_base(hw_ip), set_id, hratio, vratio);

	/* set_crop */
	rgbp_hw_s_yuvsc_crop(get_base(hw_ip), param_set->dma_output_yuv.cmd, out_x, 0,
			param_set->dma_output_yuv.width, param_set->dma_output_yuv.height);

	upsc_enable = 0x0;
	upsc_bypass = 0x0;
	decomp_enable = 0x0;
	decomp_bypass = 0x1;
	gamma_enable = 0x0;
	gamma_bypass = 0x1;

	/* for HF */
	hf_mode = rgbp_hw_g_hf_mode(param_set);
	if (hf_mode) {
		if ((param_set->otf_input.width == param_set->otf_output.width) &&
			(param_set->otf_input.height == param_set->otf_output.height)) {
			mserr_hw("not support HF in the same otf in/output size\n", instance, hw_ip);
			goto hf_disable;
		}

		/* set UPSC */
		upsc_size.input_h_size = yuvsc_size.dst_h_size;
		upsc_size.input_v_size = yuvsc_size.dst_v_size;
		upsc_size.dst_h_size = yuvsc_size.input_h_size;
		upsc_size.dst_v_size = yuvsc_size.input_v_size;

		upsc_enable = 0x1;
		rgbp_hw_s_upsc_dst_size(get_base(hw_ip), set_id, upsc_size.dst_h_size, upsc_size.dst_v_size);

		hratio = ((u64)upsc_size.input_h_size << 20) / upsc_size.dst_h_size;
		vratio = ((u64)upsc_size.input_v_size << 20) / upsc_size.dst_v_size;

		rgbp_hw_s_upsc_scaling_ratio(get_base(hw_ip), set_id, hratio, vratio);
		rgbp_hw_s_upsc_coef(get_base(hw_ip), set_id, hratio, vratio);

		gamma_enable = 0x1;
		gamma_bypass = 0x0;

		/* set Decomp */
		decomp_enable = 0x1;
		decomp_bypass = 0x0;
		rgbp_hw_s_decomp_size(get_base(hw_ip), set_id, upsc_size.dst_h_size, upsc_size.dst_v_size);
	}

	if (hf_mode == HF_DMA)
		rgbp_hw_s_votf(get_base(hw_ip), set_id, 0x1, 0x1);
	else
		rgbp_hw_s_votf(get_base(hw_ip), set_id, 0x0, 0x1);

hf_disable:
	rgbp_hw_s_upsc_enable(get_base(hw_ip), set_id, upsc_enable, upsc_bypass);
	rgbp_hw_s_decomp_enable(get_base(hw_ip), set_id, decomp_enable, decomp_bypass);

	if (!IS_ENABLED(IRTA_CALL))
		gamma_bypass = 0x1;

	/* set gamma */
	rgbp_hw_s_gamma_enable(get_base(hw_ip), set_id, gamma_enable, gamma_bypass);

	return ret;
}

static int is_hw_rgbp_shot(struct is_hw_ip *hw_ip, struct is_frame *frame,
	ulong hw_map)
{
	struct is_hw_rgbp *hw_rgbp = NULL;
	struct is_param_region *param_region;
	struct rgbp_param_set *param_set = NULL;
	struct is_frame *dma_frame;
	struct param_dma_input *pdi;
	struct param_dma_output *pdo;
	u32 fcount, instance;
	u32 strip_start_pos = 0;
	u32 cur_idx;
	u32 set_id;
	int ret, i, batch_num;
	u32 strip_index, strip_total_count, strip_repeat_num, strip_repeat_idx;
	u32 rdma_max_cnt, wdma_max_cnt;
	u32 hf_mode;
	bool do_blk_cfg = true;
	bool skip = false;
	struct c_loader_buffer clb;
	pdma_addr_t *param_set_dva;
	dma_addr_t *dma_frame_dva;
	char *name;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	instance = frame->instance;
	msdbgs_hw(2, "[F:%d]shot(%d)\n", instance, hw_ip, frame->fcount, frame->cur_buf_index);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	if (!hw_ip->hardware) {
		mserr_hw("failed to get hardware", instance, hw_ip);
		return -EINVAL;
	}

	FIMC_BUG(!hw_ip->priv_info);
	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	param_set = &hw_rgbp->param_set[instance];
	param_region = frame->parameter;

	atomic_set(&hw_rgbp->strip_index, frame->stripe_info.region_id);
	fcount = frame->fcount;
	dma_frame = frame;
	rdma_max_cnt = hw_rgbp->rdma_max_cnt;
	wdma_max_cnt = hw_rgbp->wdma_max_cnt;

	FIMC_BUG(!frame->shot);

	if (hw_ip->internal_fcount[instance] != 0)
		hw_ip->internal_fcount[instance] = 0;

	if (frame->shot_ext) {
		if ((param_set->tnr_mode != frame->shot_ext->tnr_mode) &&
				!CHK_VIDEOHDR_MODE_CHANGE(param_set->tnr_mode, frame->shot_ext->tnr_mode))
			msinfo_hw("[F:%d] TNR mode is changed (%d -> %d)\n",
					instance, hw_ip, frame->fcount,
					param_set->tnr_mode, frame->shot_ext->tnr_mode);
		param_set->tnr_mode = frame->shot_ext->tnr_mode;
	} else {
		mswarn_hw("frame->shot_ext is null", instance, hw_ip);
		param_set->tnr_mode = TNR_PROCESSING_PREVIEW_POST_ON;
	}

	__is_hw_rgbp_update_param(hw_ip, param_region, param_set, frame->pmap, instance);

	/* DMA settings */
	cur_idx = frame->cur_buf_index;
	name = __getname();
	if (!name) {
		mserr_hw("Failed to get name buffer", instance, hw_ip);
		return -ENOMEM;
	}

	for (i = 0; i < hw_rgbp->rdma_param_max_cnt ; i++) {
		if (rgbp_hw_g_rdma_param_ptr(i, dma_frame, param_set,
			name, &dma_frame_dva, &pdi, &param_set_dva))
			continue;

		CALL_HW_OPS(hw_ip, dma_cfg, name, hw_ip,
			frame, cur_idx, frame->num_buffers,
			&pdi->cmd, pdi->plane, param_set_dva, dma_frame_dva);
	}

	for (i = 0; i < hw_rgbp->wdma_param_max_cnt ; i++) {
		if (rgbp_hw_g_wdma_param_ptr(i, dma_frame, param_set,
			name, &dma_frame_dva, &pdo, &param_set_dva))
			continue;

		CALL_HW_OPS(hw_ip, dma_cfg, name, hw_ip,
			frame, cur_idx, frame->num_buffers,
			&pdo->cmd, pdo->plane, param_set_dva, dma_frame_dva);
	}
	__putname(name);

	hf_mode = rgbp_hw_g_hf_mode(param_set);
	msdbg_hw(2, "[F:%d][%s] hf:%d, sf:%d, yuv:%d, rgb:%d\n",
		instance, hw_ip, dma_frame->fcount, __func__,
		hf_mode,
		param_set->dma_output_sf.cmd,
		param_set->dma_output_yuv.cmd,
		param_set->dma_output_rgb.cmd);

	param_set->instance_id = instance;
	param_set->fcount = fcount;
	/* multi-buffer */
	hw_ip->num_buffers = frame->num_buffers;
	batch_num = hw_ip->framemgr->batch_num;
	/* Update only for SW_FRO scenario */
	if (batch_num > 1 && hw_ip->num_buffers == 1) {
		hw_ip->num_buffers |= batch_num << SW_FRO_NUM_SHIFT;
		hw_ip->num_buffers |= cur_idx << CURR_INDEX_SHIFT;
	} else {
		batch_num = 0;
	}

	strip_index = param_set->stripe_input.index;
	strip_total_count = param_set->stripe_input.total_count;
	strip_repeat_num = param_set->stripe_input.repeat_num;
	strip_repeat_idx = param_set->stripe_input.repeat_idx;
	strip_start_pos = (strip_index) ?
		(param_set->stripe_input.start_pos_x - param_set->stripe_input.left_margin) : 0;

	if (IS_ENABLED(SKIP_ISP_SHOT_FOR_MULTI_SHOT)) {
		if (hw_rgbp->repeat_instance != instance)
			hw_rgbp->repeat_state = 0;

		if (batch_num > 1 || strip_total_count > 1 || strip_repeat_num > 1)
			hw_rgbp->repeat_state++;
		else
			hw_rgbp->repeat_state = 0;

		if (hw_rgbp->repeat_state > 1 &&
				(!pablo_is_first_shot(batch_num, cur_idx) ||
				 !pablo_is_first_shot(strip_total_count, strip_index) ||
				 !pablo_is_first_shot(strip_repeat_num, strip_repeat_idx)))
			skip = true;

		hw_rgbp->repeat_instance = instance;
	}

	msdbgs_hw(2, "[F:%d] repeat_state(%d), batch(%d, %d), strip(%d, %d), repeat(%d, %d), skip(%d)",
			instance, hw_ip, frame->fcount,
			hw_rgbp->repeat_state,
			batch_num, cur_idx,
			strip_total_count, strip_index,
			strip_repeat_num, strip_repeat_idx,
			skip);

	set_id = COREX_DIRECT;

	/* reset CLD buffer */
	clb.num_of_headers = 0;
	clb.num_of_values = 0;
	clb.num_of_pairs = 0;

	clb.header_dva = hw_rgbp->dva_c_loader_header;
	clb.payload_dva = hw_rgbp->dva_c_loader_payload;

	clb.clh = (struct c_loader_header *)hw_rgbp->kva_c_loader_header;
	clb.clp = (struct c_loader_payload *)hw_rgbp->kva_c_loader_payload;

	rgbp_hw_s_clock(get_base(hw_ip), true);
	rgbp_hw_s_core(get_base(hw_ip), frame->num_buffers, set_id,
		       &hw_rgbp->config, param_set);

	ret = CALL_HW_HELPER_OPS(hw_ip, lib_shot, instance, skip,
				frame, &hw_rgbp->lib[instance], param_set);
	if (ret)
		return ret;

	do_blk_cfg = CALL_HW_HELPER_OPS(hw_ip, set_rta_regs, instance, set_id, skip, frame, (void *)&clb);
	if (unlikely(do_blk_cfg))
		__is_hw_rgbp_update_block_reg(hw_ip, param_set, instance, set_id);

	is_hw_rgbp_s_size_regs(hw_ip, param_set, instance, frame);

	ret = __is_hw_rgbp_set_size_regs(hw_ip, param_set, instance,
			strip_start_pos, frame, set_id);
	if (ret) {
		mserr_hw("__is_hw_rgbp_set_size_regs is fail", instance, hw_ip);
		goto shot_fail_recovery;
	}

	if (unlikely(test_bit(RGBP_DBG_DTP, &debug_rgbp.value)))
		rgbp_hw_s_dtp(get_base(hw_ip), set_id, 2);

	for (i = 0; i < rdma_max_cnt; i++) {
		ret = __is_hw_rgbp_set_rdma(hw_ip, hw_rgbp, param_set, instance,
				i, set_id);
		if (ret) {
			mserr_hw("__is_hw_rgbp_set_rdma is fail", instance, hw_ip);
			goto shot_fail_recovery;
		}
	}

	for (i = 0; i < wdma_max_cnt; i++) {
		ret = __is_hw_rgbp_set_wdma(hw_ip, hw_rgbp, param_set, instance,
				i, set_id, dma_frame);
		if (ret) {
			mserr_hw("__is_hw_rgbp_set_wdma is fail", instance, hw_ip);
			goto shot_fail_recovery;
		}
	}

	if (param_region->rgbp.control.strgen == CONTROL_COMMAND_START) {
		msdbg_hw(2, "STRGEN input\n", instance, hw_ip);
		rgbp_hw_s_strgen(get_base(hw_ip), set_id);
	}

	if (!IS_ENABLED(RGBP_USE_MMIO)) {
		pmio_cache_fsync(hw_ip->pmio, (void *)&clb, PMIO_FORMATTER_PAIR);

		if (clb.num_of_pairs > 0)
			clb.num_of_headers++;

		CALL_BUFOP(hw_rgbp->pb_c_loader_payload, sync_for_device, hw_rgbp->pb_c_loader_payload,
				0, hw_rgbp->pb_c_loader_payload->size, DMA_TO_DEVICE);
		CALL_BUFOP(hw_rgbp->pb_c_loader_header, sync_for_device, hw_rgbp->pb_c_loader_header,
				0, hw_rgbp->pb_c_loader_header->size, DMA_TO_DEVICE);
	}

	CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, fcount, DEBUG_POINT_ADD_TO_CMDQ);
	ret = is_hw_rgbp_set_cmdq(hw_ip, instance, set_id, frame->num_buffers, &clb);
	if (ret) {
		mserr_hw("is_hw_rgbp_set_cmdq is fail", instance, hw_ip);
		goto shot_fail_recovery;
	}

	rgbp_hw_s_clock(get_base(hw_ip), false);

	if (unlikely(test_bit(RGBP_DBG_DUMP_REG, &debug_rgbp.value)))
		rgbp_hw_dump(hw_ip->pmio, HW_DUMP_CR);

	set_bit(HW_CONFIG, &hw_ip->state);

	return 0;

shot_fail_recovery:
	CALL_HW_HELPER_OPS(hw_ip, restore, instance, &hw_rgbp->lib[instance]);

	return ret;
}

static int is_hw_rgbp_get_meta(struct is_hw_ip *hw_ip, struct is_frame *frame,
	ulong hw_map)
{
	struct is_hw_rgbp *hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, get_meta, frame->instance,
				frame, &hw_rgbp->lib[frame->instance]);
}

static int is_hw_rgbp_frame_ndone(struct is_hw_ip *hw_ip, struct is_frame *frame,
		enum ShotErrorType done_type)
{
	int ret = 0;
	int output_id;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	output_id = IS_HW_CORE_END;
	if (test_bit(hw_ip->id, &frame->core_flag)) {
		ret = CALL_HW_OPS(hw_ip, frame_done, hw_ip, frame, -1,
			output_id, done_type, false);
	}

	return ret;
}


static int is_hw_rgbp_load_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_rgbp *hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, load_setfile, instance, &hw_rgbp->lib[instance]);
}


static int is_hw_rgbp_apply_setfile(struct is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	struct is_hw_rgbp *hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, apply_setfile, instance, &hw_rgbp->lib[instance],
				scenario);
}

static int is_hw_rgbp_delete_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_rgbp *hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, delete_setfile, instance, &hw_rgbp->lib[instance]);
}

int is_hw_rgbp_restore(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_rgbp *hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;

	is_hw_rgbp_reset(hw_ip, instance);
	if (!IS_ENABLED(RGBP_USE_MMIO)) {
		if (pmio_reinit_cache(hw_ip->pmio, &hw_ip->pmio_config)) {
			pmio_cache_set_bypass(hw_ip->pmio, true);
			err("failed to reinit PMIO cache, set bypass");
			return -EINVAL;
		}
	}

	is_hw_rgbp_prepare(hw_ip, instance);

	return CALL_HW_HELPER_OPS(hw_ip, restore, instance, &hw_rgbp->lib[instance]);
}

static int is_hw_rgbp_set_regs(struct is_hw_ip *hw_ip, u32 chain_id,
	u32 instance, u32 fcount, struct cr_set *regs, u32 regs_size)
{
	return CALL_HW_HELPER_OPS(hw_ip, set_regs, instance, regs, regs_size);
}

int is_hw_rgbp_dump_regs(struct is_hw_ip *hw_ip, u32 instance, u32 fcount,
	struct cr_set *regs, u32 regs_size, enum is_reg_dump_type dump_type)
{
	struct is_common_dma *dma;
	struct is_hw_rgbp *hw_rgbp = NULL;
	u32 i, rdma_max_cnt, wdma_max_cnt;

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	rdma_max_cnt = hw_rgbp->rdma_max_cnt;
	wdma_max_cnt = hw_rgbp->wdma_max_cnt;

	switch (dump_type) {
	case IS_REG_DUMP_TO_LOG:
		rgbp_hw_dump(hw_ip->pmio, HW_DUMP_CR);
		break;
	case IS_REG_DUMP_DMA:
		for (i = 0; i < rdma_max_cnt; i++) {
			dma = &hw_rgbp->rdma[i];
			CALL_DMA_OPS(dma, dma_print_info, 0);
		}

		for (i = 0; i < wdma_max_cnt; i++) {
			dma = &hw_rgbp->wdma[i];
			CALL_DMA_OPS(dma, dma_print_info, 0);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int is_hw_rgbp_set_config(struct is_hw_ip *hw_ip, u32 chain_id, u32 instance, u32 fcount, void *conf)
{
	struct is_hw_rgbp *hw_rgbp = NULL;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!conf);

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	hw_rgbp->config = *(struct is_rgbp_config *)conf;

	return 0;
}

static int is_hw_rgbp_notify_timeout(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_rgbp *hw_rgbp;

	hw_rgbp = (struct is_hw_rgbp *)hw_ip->priv_info;
	if (!hw_rgbp) {
		mserr_hw("failed to get HW RGBP", instance, hw_ip);
		return -ENODEV;
	}
	/* DEBUG */
	rgbp_hw_dump(hw_ip->pmio, HW_DUMP_DBG_STATE);

	return CALL_HW_HELPER_OPS(hw_ip, notify_timeout, instance, &hw_rgbp->lib[instance]);
}

static size_t is_hw_rgbp_dump_params(struct is_hw_ip *hw_ip, u32 instance, char *buf, size_t size)
{
	struct is_hw_rgbp *hw;
	struct rgbp_param_set *param;
	size_t rem = size;
	char *p = buf;

	hw = (struct is_hw_rgbp *)hw_ip->priv_info;
	param = &hw->param_set[instance];

	p = pablo_json_nstr(p, "hw name", hw_ip->name, strlen(hw_ip->name), &rem);
	p = pablo_json_uint(p, "hw id", hw_ip->id, &rem);

	p = dump_param_otf_input(p, "otf_input", &param->otf_input, &rem);
	p = dump_param_otf_output(p, "otf_output", &param->otf_output, &rem);
	p = dump_param_dma_input(p, "dma_input", &param->dma_input, &rem);
	p = dump_param_stripe_input(p, "stripe_input", &param->stripe_input, &rem);
	p = dump_param_dma_output(p, "dma_output_hf", &param->dma_output_hf, &rem);
	p = dump_param_dma_output(p, "dma_output_sf", &param->dma_output_sf, &rem);
	p = dump_param_dma_output(p, "dma_output_yuv", &param->dma_output_yuv, &rem);
	p = dump_param_dma_output(p, "dma_output_rgb", &param->dma_output_rgb, &rem);

	p = pablo_json_uint(p, "instance_id", param->instance_id, &rem);
	p = pablo_json_uint(p, "fcount", param->fcount, &rem);
	p = pablo_json_uint(p, "tnr_mode", param->tnr_mode, &rem);
	p = pablo_json_bool(p, "reprocessing", param->reprocessing, &rem);

	return WRITTEN(size, rem);
}

const struct is_hw_ip_ops is_hw_rgbp_ops = {
	.open			= is_hw_rgbp_open,
	.init			= is_hw_rgbp_init,
	.deinit			= is_hw_rgbp_deinit,
	.close			= is_hw_rgbp_close,
	.enable			= is_hw_rgbp_enable,
	.disable		= is_hw_rgbp_disable,
	.shot			= is_hw_rgbp_shot,
	.set_param		= is_hw_rgbp_set_param,
	.get_meta		= is_hw_rgbp_get_meta,
	.frame_ndone		= is_hw_rgbp_frame_ndone,
	.load_setfile		= is_hw_rgbp_load_setfile,
	.apply_setfile		= is_hw_rgbp_apply_setfile,
	.delete_setfile		= is_hw_rgbp_delete_setfile,
	.restore		= is_hw_rgbp_restore,
	.set_regs		= is_hw_rgbp_set_regs,
	.dump_regs		= is_hw_rgbp_dump_regs,
	.set_config		= is_hw_rgbp_set_config,
	.notify_timeout		= is_hw_rgbp_notify_timeout,
	.dump_params		= is_hw_rgbp_dump_params,
};

int is_hw_rgbp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name)
{
	int hw_slot;
	int ret = 0;

	hw_ip->ops  = &is_hw_rgbp_ops;

	hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, id);
	if (!valid_hw_slot_id(hw_slot)) {
		serr_hw("invalid hw_slot (%d)", hw_ip, hw_slot);
		return -EINVAL;
	}

	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &is_hw_rgbp_handle_interrupt0;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].id = INTR_HWIP1;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].valid = true;

	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].handler = &is_hw_rgbp_handle_interrupt1;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].id = INTR_HWIP2;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].valid = true;

	if (IS_ENABLED(RGBP_USE_MMIO))
		return ret;

	hw_ip->mmio_base = hw_ip->regs[REG_SETA];

	hw_ip->pmio_config.name = "rgbp";

	hw_ip->pmio_config.mmio_base = hw_ip->regs[REG_SETA];
	hw_ip->pmio_config.phys_base = hw_ip->regs_start[REG_SETA];

	hw_ip->pmio_config.cache_type = PMIO_CACHE_NONE;

	rgbp_hw_init_pmio_config(&hw_ip->pmio_config);

	hw_ip->pmio = pmio_init(NULL, NULL, &hw_ip->pmio_config);
	if (IS_ERR(hw_ip->pmio)) {
		err("failed to init rgbp PMIO: %ld", PTR_ERR(hw_ip->pmio));
		return -ENOMEM;
	}

	ret = pmio_field_bulk_alloc(hw_ip->pmio, &hw_ip->pmio_fields,
			hw_ip->pmio_config.fields,
			hw_ip->pmio_config.num_fields);
	if (ret) {
		err("failed to alloc rgbp PMIO fields: %d", ret);
		pmio_exit(hw_ip->pmio);
		return ret;

	}

	return 0;
}
EXPORT_SYMBOL_GPL(is_hw_rgbp_probe);

void is_hw_rgbp_remove(struct is_hw_ip *hw_ip)
{
	struct is_interface_ischain *itfc = hw_ip->itfc;
	int id = hw_ip->id;
	int hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, id);

	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].valid = false;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].valid = false;

	if (IS_ENABLED(RGBP_USE_MMIO))
		return;

	pmio_field_bulk_free(hw_ip->pmio, hw_ip->pmio_fields);
	pmio_exit(hw_ip->pmio);
}
EXPORT_SYMBOL_GPL(is_hw_rgbp_remove);
