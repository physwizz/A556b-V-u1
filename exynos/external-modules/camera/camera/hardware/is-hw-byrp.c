// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-hw-byrp.h"
#include "is-err.h"
#include "api/is-hw-api-byrp.h"
#include "pablo-hw-helper.h"
#include "is-stripe.h"

#define LIB_MODE BYRP_USE_DRIVER

static int debug_byrp;
module_param(debug_byrp, int, 0644);

static int is_hw_byrp_handle_interrupt(u32 id, void *context)
{
	struct is_hardware *hardware = NULL;
	struct is_hw_ip *hw_ip = NULL;
	struct is_hw_byrp *hw_byrp = NULL;
	struct byrp_param_set *param_set;
	u32 status, instance, hw_fcount, strip_index, strip_total_count, hl = 0, vl = 0;
	bool f_err = false;
	u32 cur_idx, batch_num;
	bool is_first_shot = true, is_last_shot = true;

	hw_ip = (struct is_hw_ip *)context;
	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);
	param_set = &hw_byrp->param_set[instance];
	strip_index = param_set->stripe_input.index;
	strip_total_count = param_set->stripe_input.total_count;

	cur_idx = (hw_ip->num_buffers >> CURR_INDEX_SHIFT) & 0xff;
	batch_num = (hw_ip->num_buffers >> SW_FRO_NUM_SHIFT) & 0xff;

	if (IS_ENABLED(SKIP_ISP_SHOT_FOR_MULTI_SHOT)) {
		/* SW batch mode */
		if (!pablo_is_first_shot(batch_num, cur_idx)) is_first_shot = false;
		if (!pablo_is_last_shot(batch_num, cur_idx)) is_last_shot = false;
		/* Stripe processing */
		if (!pablo_is_first_shot(strip_total_count, strip_index)) is_first_shot = false;
		if (!pablo_is_last_shot(strip_total_count, strip_index)) is_last_shot = false;
		/* Repeat processing */
		if (!pablo_is_first_shot(param_set->stripe_input.repeat_num,
			param_set->stripe_input.repeat_idx)) is_first_shot = false;
		if (!pablo_is_last_shot(param_set->stripe_input.repeat_num,
			param_set->stripe_input.repeat_idx)) is_last_shot = false;
	}

	status = byrp_hw_g_int0_state(hw_ip->regs[REG_SETA], true,
			(hw_ip->num_buffers & 0xffff), &hw_byrp->irq_state[BYRP_INT0])
			& byrp_hw_g_int0_mask(hw_ip->regs[REG_SETA]);

	msdbg_hw(2, "BYRP interrupt status(0x%x)\n", instance, hw_ip, status);

	if (!test_bit(HW_OPEN, &hw_ip->state)) {
		mserr_hw("invalid interrupt: 0x%x", instance, hw_ip, status);
		return 0;
	}

	if (test_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag)) {
		mserr_hw("During recovery : invalid interrupt", instance, hw_ip);
		return 0;
	}

	if (!test_bit(HW_RUN, &hw_ip->state)) {
		mserr_hw("HW disabled!! interrupt(0x%x)", instance, hw_ip, status);
		return 0;
	}

	if (byrp_hw_is_occurred(status, INTR_SETTING_DONE))
		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount, DEBUG_POINT_SETTING_DONE);

	if (byrp_hw_is_occurred(status, INTR_FRAME_START) && byrp_hw_is_occurred(status, INTR_FRAME_END))
		mswarn_hw("start/end overlapped!! (0x%x)", instance, hw_ip, status);

	if (byrp_hw_is_occurred(status, INTR_FRAME_START)) {
		msdbg_hw(2, "[F:%d]F.S\n", instance, hw_ip, hw_fcount);

		if (IS_ENABLED(BYRP_DDK_LIB_CALL) && is_first_shot) {
			is_lib_isp_event_notifier(hw_ip, &hw_byrp->lib[instance],
					instance, hw_fcount, BYRP_EVENT_FRAME_START,
					strip_index, NULL);
		} else {
			atomic_add((hw_ip->num_buffers & 0xffff), &hw_ip->count.fs);
			CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount,
					DEBUG_POINT_FRAME_START);
			if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
				msdbg_hw(2, "[F:%d]F.S\n", instance, hw_ip, hw_fcount);

			CALL_HW_OPS(hw_ip, frame_start, hw_ip, instance);
		}

		/* byrp_hw_dump(hw_ip->regs[REG_SETA]); */
	}

	if (byrp_hw_is_occurred(status, INTR_FRAME_END)) {
		msdbg_hw(2, "[F:%d]F.E\n", instance, hw_ip, hw_fcount);

		if (IS_ENABLED(BYRP_DDK_LIB_CALL) && is_last_shot) {
			is_lib_isp_event_notifier(hw_ip, &hw_byrp->lib[instance],
					instance, hw_fcount, BYRP_EVENT_FRAME_END,
					strip_index, NULL);
		} else {
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

		/* byrp_hw_dump(hw_ip->regs[REG_SETA]); */
	}

	/* COREX Interrupt */
	/*
	 * if (byrp_hw_is_occurred(status, INTR_COREX_END_0)) {}
	 *
	 * if (byrp_hw_is_occurred(status, INTR_COREX_END_1)) {}
	 */

	f_err = byrp_hw_is_occurred(status, INTR_ERR);

	if (f_err) {
		msinfo_hw("[F:%d] Ocurred error interrupt (%d,%d) status(0x%x)\n",
			instance, hw_ip, hw_fcount, hl, vl, status);
	}

	return 0;
}

static int __is_hw_byrp_s_reset(struct is_hw_ip *hw_ip, struct is_hw_byrp *hw_byrp)
{
	int i = 0;

	for (i = 0; i < COREX_MAX; i++)
		hw_ip->cur_hw_iq_set[i].size = 0;

	return byrp_hw_s_reset(hw_ip->regs[REG_SETA]);
}

static int __is_hw_byrp_s_common_reg(struct is_hw_ip *hw_ip, u32 instance)
{
	int res = 0;
	struct is_hw_byrp *hw_byrp;

	FIMC_BUG(!hw_ip);

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	if (hw_ip) {
		msinfo_hw("reset\n", instance, hw_ip);

		res = __is_hw_byrp_s_reset(hw_ip, hw_byrp);
		if (res != 0) {
			mserr_hw("sw reset fail", instance, hw_ip);
			return -ENODEV;
		}

		/* TODO: Enable Corex */
		/* byrp_hw_s_corex_init(hw_ip->regs[REG_SETA], 0); */
		byrp_hw_s_init(hw_ip->regs[REG_SETA], 0);
	}

	msinfo_hw("clear interrupt\n", instance, hw_ip);

	return 0;
}

static int __is_hw_byrp_clear_common(struct is_hw_ip *hw_ip, u32 instance)
{
	int res = 0;
	struct is_hw_byrp *hw_byrp;

	FIMC_BUG(!hw_ip);

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	res = __is_hw_byrp_s_reset(hw_ip, hw_byrp);
	if (res != 0) {
		mserr_hw("sw reset fail", instance, hw_ip);
		return -ENODEV;
	}
	res = byrp_hw_wait_idle(hw_ip->regs[REG_SETA]);

	if (res)
		mserr_hw("failed to byrp_hw_wait_idle", instance, hw_ip);

	msinfo_hw("final finished byrp\n", instance, hw_ip);

	return res;
}

static int is_hw_byrp_set_cmdq(struct is_hw_ip *hw_ip, u32 instance, u32 set_id)
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
	 * ret = byrp_hw_s_corex_update_type(hw_ip->regs[REG_SETA], set_id, COREX_COPY);
	 * if (ret)
	 *	return ret;
	 */

	byrp_hw_s_cmdq(hw_ip->regs[REG_SETA], set_id);

	return ret;
}

static int __nocfi is_hw_byrp_open(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct is_hw_byrp *hw_byrp = NULL;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, "HWBYRP");
	frame_manager_open(hw_ip->framemgr, IS_MAX_HW_FRAME, false);

	hw_ip->priv_info = vzalloc(sizeof(struct is_hw_byrp));
	if (!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
	hw_byrp->instance = instance;

	ret = CALL_HW_HELPER_OPS(hw_ip, open, instance, &hw_byrp->lib[instance],
				LIB_FUNC_BYRP);
	if (ret)
		goto err_chain_create;

	ret = CALL_HW_HELPER_OPS(hw_ip, alloc_iqset, byrp_hw_g_reg_cnt());
	if (ret)
		goto err_iqset_alloc;

	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	set_bit(HW_OPEN, &hw_ip->state);

	msdbg_hw(2, "open: framemgr[%s]", instance, hw_ip, hw_ip->framemgr->name);

	return 0;

err_iqset_alloc:
	CALL_HW_HELPER_OPS(hw_ip, close, instance, &hw_byrp->lib[instance]);

err_chain_create:
	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
err_alloc:
	frame_manager_close(hw_ip->framemgr);
	return ret;
}

static int is_hw_byrp_init(struct is_hw_ip *hw_ip, u32 instance,
			bool flag, u32 f_type)
{
	int ret = 0;
	struct is_hw_byrp *hw_byrp = NULL;
	u32 input_id;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
	if (!hw_byrp) {
		mserr_hw("hw_byrp is null ", instance, hw_ip);
		ret = -ENODATA;
		goto err;
	}

	hw_ip->lib_mode = LIB_MODE;

	ret = CALL_HW_HELPER_OPS(hw_ip, init, instance, &hw_byrp->lib[instance],
				(u32)flag, f_type, LIB_FUNC_BYRP);
	if (ret)
		return ret;

	for (input_id = BYRP_RDMA_IMG; input_id < BYRP_RDMA_MAX; input_id++) {
		ret = byrp_hw_rdma_create(&hw_byrp->rdma[input_id], hw_ip->regs[REG_SETA], input_id);
		if (ret) {
			mserr_hw("byrp_hw_rdma_create error[%d]", instance, hw_ip, input_id);
			ret = -ENODATA;
			goto err;
		}
	}

	for (input_id = BYRP_WDMA_BYR; input_id < BYRP_WDMA_MAX; input_id++) {
		ret = byrp_hw_wdma_create(&hw_byrp->wdma[input_id], hw_ip->regs[REG_SETA], input_id);
		if (ret) {
			mserr_hw("byrp_hw_wdma_create error[%d]", instance, hw_ip, input_id);
			ret = -ENODATA;
			goto err;
		}
	}

	set_bit(HW_INIT, &hw_ip->state);
	return 0;

err:
	return ret;
}

static int is_hw_byrp_deinit(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_byrp *hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, deinit, instance, &hw_byrp->lib[instance]);
}

static int is_hw_byrp_close(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_byrp *hw_byrp = NULL;
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	CALL_HW_HELPER_OPS(hw_ip, close, instance, &hw_byrp->lib[instance]);

	if (hw_ip->lib_mode == BYRP_USE_DRIVER)
		__is_hw_byrp_clear_common(hw_ip, instance);

	CALL_HW_HELPER_OPS(hw_ip, free_iqset);

	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;

	frame_manager_close(hw_ip->framemgr);
	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int is_hw_byrp_enable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_byrp *hw_byrp = NULL;
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

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	__is_hw_byrp_s_common_reg(hw_ip, instance);

	set_bit(HW_RUN, &hw_ip->state);
	msdbg_hw(2, "enable: done\n", instance, hw_ip);

	return ret;
}

static int is_hw_byrp_disable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	long timetowait;
	struct is_hw_byrp *hw_byrp;
	struct byrp_param_set *param_set;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	msinfo_hw("byrp_disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	FIMC_BUG(!hw_ip->priv_info);
	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	timetowait = wait_event_timeout(hw_ip->status.wait_queue,
		!atomic_read(&hw_ip->status.Vvalid),
		IS_HW_STOP_TIMEOUT);

	if (!timetowait) {
		mserr_hw("wait FRAME_END timeout (%ld)", instance,
			hw_ip, timetowait);
		ret = -ETIME;
	}

	param_set = &hw_byrp->param_set[instance];
	param_set->fcount = 0;

	CALL_HW_HELPER_OPS(hw_ip, disable, instance, &hw_byrp->lib[instance]);

	if (hw_ip->run_rsc_state)
		return 0;

	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int byrp_hw_g_rdma_offset(u32 instance, u32 id, struct param_dma_input *dma_input,
	struct param_stripe_input *stripe_input, struct is_crop *dma_crop_cfg,
	u32 *img_offset, u32 *header_offset)
{
	u32 hwformat, memory_bitwidth, pixelsize, sbwc_type;
	u32 sbwc_en;
	u32 comp_64b_align, lossy_byte32num = 0;
	u32 strip_enable;
	u32 full_width;
	u32 start_pos_x;
	u32 image_stride_1p, header_stride_1p;
	u32 crop_x;

	hwformat = dma_input->format;
	sbwc_type = dma_input->sbwc_type;
	memory_bitwidth = dma_input->bitwidth;
	pixelsize = dma_input->msb + 1;

	*img_offset = 0;
	*header_offset = 0;

	strip_enable = (stripe_input->total_count == 0) ? 0 : 1;
	full_width = (strip_enable) ? stripe_input->full_width : dma_input->width;
	start_pos_x = (strip_enable) ? stripe_input->start_pos_x : 0;
	crop_x = dma_crop_cfg->x + start_pos_x;

	sbwc_en = is_hw_dma_get_comp_sbwc_en(sbwc_type, &comp_64b_align);

	switch (id) {
	case BYRP_RDMA_IMG:
		if (sbwc_en == NONE) {
			image_stride_1p = is_hw_dma_get_img_stride(memory_bitwidth, pixelsize,
									hwformat, full_width,
									32, true);
			*img_offset = (image_stride_1p * dma_crop_cfg->y) +
					is_hw_dma_get_img_stride(memory_bitwidth, pixelsize,
									hwformat, crop_x, 16, true);
		} else if (sbwc_en == COMP || sbwc_en == COMP_LOSS) {
			image_stride_1p = is_hw_dma_get_payload_stride(sbwc_en, pixelsize,
									full_width, comp_64b_align,
									lossy_byte32num,
									BYRP_COMP_BLOCK_WIDTH,
									BYRP_COMP_BLOCK_HEIGHT);
			*img_offset = (image_stride_1p * dma_crop_cfg->y) +
					is_hw_dma_get_payload_stride(sbwc_en, pixelsize, crop_x,
									comp_64b_align,
									lossy_byte32num,
									BYRP_COMP_BLOCK_WIDTH,
									BYRP_COMP_BLOCK_HEIGHT);
		} else {
			merr_hw("sbwc_en(%d) is not valid", instance, sbwc_en);
			return -1;
		}

		if (sbwc_en == COMP) {
			header_stride_1p = is_hw_dma_get_header_stride(full_width,
									BYRP_COMP_BLOCK_WIDTH,
									32);
			*header_offset = (header_stride_1p * dma_crop_cfg->y) +
					is_hw_dma_get_header_stride(crop_x, BYRP_COMP_BLOCK_WIDTH,
									0);
		}

		break;
	default:
		break;
	}

	return 0;
}

static int byrp_hw_g_wdma_offset(u32 instance, u32 id, struct param_dma_output *dma_output,
	struct param_stripe_input *stripe_input, struct is_crop *dma_crop_cfg,
	u32 *img_offset, u32 *header_offset)
{
	u32 hwformat, memory_bitwidth, pixelsize, sbwc_type;
	u32 sbwc_en;
	u32 comp_64b_align;
	u32 strip_enable;
	u32 full_width;
	u32 start_pos_x;
	u32 image_stride_1p;
	u32 crop_x;

	hwformat = dma_output->format;
	sbwc_type = dma_output->sbwc_type;
	memory_bitwidth = dma_output->bitwidth;
	pixelsize = dma_output->msb + 1;

	*img_offset = 0;
	*header_offset = 0;

	strip_enable = (stripe_input->total_count == 0) ? 0 : 1;
	full_width = (strip_enable) ? stripe_input->full_width : dma_output->width;
	start_pos_x = (strip_enable) ? stripe_input->start_pos_x : 0;
	crop_x = dma_crop_cfg->x + start_pos_x + stripe_input->left_margin;

	sbwc_en = is_hw_dma_get_comp_sbwc_en(sbwc_type, &comp_64b_align);

	switch (id) {
	case BYRP_WDMA_BYR:
		image_stride_1p = is_hw_dma_get_img_stride(memory_bitwidth, pixelsize,
				hwformat, full_width,
				32, true);
		*img_offset = (image_stride_1p * dma_crop_cfg->y) +
			is_hw_dma_get_img_stride(memory_bitwidth, pixelsize,
					hwformat, crop_x, 16, true);
		break;
	default:
		break;
	}

	return 0;
}

static int __is_hw_byrp_set_rdma(struct is_hw_ip *hw_ip, struct is_hw_byrp *hw_byrp,
	struct byrp_param_set *param_set,
	u32 instance, u32 id, struct is_crop *dma_crop_cfg, u32 set_id)
{
	u32 *input_dva = NULL;
	u32 cmd;
	u32 comp_sbwc_en, payload_size;
	u32 cache_hint = 0;
	u32 rdma_width = 0, rdma_height = 0;
	int ret = 0;
	u32 image_addr_offset;
	u32 header_addr_offset;
	struct param_dma_input *dma_input;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_byrp);
	FIMC_BUG(!param_set);

	switch (id) {
	case BYRP_RDMA_IMG:
		input_dva = param_set->input_dva;
		dma_input = &param_set->dma_input;
		cache_hint = 0x7; /* 111: last-access-read */
		break;
	case BYRP_RDMA_HDR:
		input_dva = param_set->input_dva_hdr;
		dma_input = &param_set->dma_input_hdr;
		cache_hint = 0x3; /* 011: cache-noalloc-type */
		break;
	default:
		merr_hw("invalid ID (%d)", instance,  id);
		return -EINVAL;
	}

	msdbg_hw(2, "[HW][%s] %s cmd: %d, %dx%d, format: %d, bitwidth: %d\n",
		instance, hw_ip, __func__,
		(id == BYRP_RDMA_IMG) ? "RDMA_IMG" : "RDMA_HDR",
		dma_input->cmd, dma_input->width, dma_input->height,
		dma_input->format, dma_input->bitwidth);

	cmd = dma_input->cmd;

	msdbg_hw(2, "%s: %d\n", instance, hw_ip, hw_byrp->rdma[id].name, cmd);

	byrp_hw_s_rdma_corex_id(&hw_byrp->rdma[id], set_id);

	rdma_width = dma_input->width;
	rdma_height = dma_input->height;

	ret = byrp_hw_s_rdma_init(&hw_byrp->rdma[id], param_set,
					cmd, dma_crop_cfg, cache_hint,
					&comp_sbwc_en, &payload_size);

	if (ret) {
		mserr_hw("failed to initialize BYRP_RDMA(%d)", instance, hw_ip, id);
		return -EINVAL;
	}

	if (cmd == DMA_INPUT_COMMAND_ENABLE) {
		byrp_hw_g_rdma_offset(instance, id, dma_input, &param_set->stripe_input,
					dma_crop_cfg, &image_addr_offset, &header_addr_offset);

		ret = byrp_hw_s_rdma_addr(&hw_byrp->rdma[id], input_dva, 0,
						(hw_ip->num_buffers & 0xffff),
						0, comp_sbwc_en, payload_size,
						image_addr_offset,
						header_addr_offset);

		if (ret) {
			mserr_hw("failed to set BYRP_RDMA(%d) address", instance, hw_ip, id);
			return -EINVAL;
		}
	}

	return 0;
}

static int __is_hw_byrp_set_wdma(struct is_hw_ip *hw_ip, struct is_hw_byrp *hw_byrp,
	struct byrp_param_set *param_set, u32 instance, u32 id, u32 set_id)
{
	u32 *output_dva = NULL;
	u32 cmd;
	u32 comp_sbwc_en, payload_size;
	u32 out_crop_size_x = 0;
	u32 cache_hint = 0;
	u32 image_addr_offset;
	u32 header_addr_offset;
	int ret = 0;
	struct is_crop dma_crop_cfg;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_byrp);
	FIMC_BUG(!param_set);

	msdbg_hw(2, "[HW][%s] WDMA cmd: %d, %dx%d, format: %d, bitwidth: %d\n",
		instance, hw_ip, __func__,
		param_set->dma_output_byr.cmd,
		param_set->dma_output_byr.width, param_set->dma_output_byr.height,
		param_set->dma_output_byr.format, param_set->dma_output_byr.bitwidth);

	switch (id) {
	case BYRP_WDMA_BYR:
		output_dva = param_set->output_dva_byr;
		cmd = param_set->dma_output_byr.cmd;
		break;
	default:
		merr_hw("invalid ID (%d)", instance,  id);
		return -EINVAL;
	}

	msdbg_hw(2, "%s: %d\n", instance, hw_ip, hw_byrp->wdma[id].name, cmd);

	byrp_hw_s_wdma_corex_id(&hw_byrp->wdma[id], set_id);

	ret = byrp_hw_s_wdma_init(&hw_byrp->wdma[id], param_set,
					hw_ip->regs[REG_SETA], set_id,
					cmd, out_crop_size_x, cache_hint,
					&comp_sbwc_en, &payload_size);

	if (ret) {
		mserr_hw("failed to initialize BYRP_WDMA(%d)", instance, hw_ip, id);
		return -EINVAL;
	}

	if (cmd == DMA_OUTPUT_COMMAND_ENABLE) {
		dma_crop_cfg.x = 0;
		dma_crop_cfg.y = 0;
		dma_crop_cfg.w = param_set->dma_input.width;
		dma_crop_cfg.h = param_set->dma_input.height;

		byrp_hw_g_wdma_offset(instance, id, &param_set->dma_output_byr, &param_set->stripe_input,
					&dma_crop_cfg, &image_addr_offset, &header_addr_offset);

		ret = byrp_hw_s_wdma_addr(&hw_byrp->wdma[id], output_dva, 0,
			(hw_ip->num_buffers & 0xffff),
			0, comp_sbwc_en, payload_size, image_addr_offset, header_addr_offset);
		if (ret) {
			mserr_hw("failed to set BYRP_WDMA(%d) address", instance, hw_ip, id);
			return -EINVAL;
		}
	}

	return 0;
}

static void __is_hw_byrp_check_size(struct is_hw_ip *hw_ip,
	struct byrp_param_set *param_set, u32 instance)
{
	struct param_dma_input *input = &param_set->dma_input;
	struct param_otf_output *output = &param_set->otf_output;

	FIMC_BUG_VOID(!param_set);

	msdbgs_hw(2, "hw_byrp_check_size >>>\n", instance, hw_ip);
	msdbgs_hw(2, "dma_input: format(%d),crop_size(%dx%d)\n", instance, hw_ip,
		input->format, input->dma_crop_width, input->dma_crop_height);
	msdbgs_hw(2, "otf_output: otf_cmd(%d),format(%d)\n", instance, hw_ip,
		output->cmd, output->format);
	msdbgs_hw(2, "otf_output: pos(%d,%d),crop%dx%d),size(%dx%d)\n", instance, hw_ip,
		output->crop_offset_x, output->crop_offset_y,
		output->crop_width, output->crop_height,
		output->width, output->height);
	msdbgs_hw(2, "<<< hw_byrp_check_size\n", instance, hw_ip);
}

static int __is_hw_byrp_bypass(struct is_hw_ip *hw_ip, u32 set_id)
{
	FIMC_BUG(!hw_ip);

	byrp_hw_s_block_bypass(hw_ip->regs[REG_SETA], set_id);

	return 0;
}

static int __is_hw_byrp_update_block_reg(struct is_hw_ip *hw_ip,
	struct byrp_param_set *param_set, u32 instance, u32 set_id)
{
	struct is_hw_byrp *hw_byrp;
	int ret = 0;

	msdbg_hw(4, "%s\n", instance, hw_ip, __func__);

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	if (hw_byrp->instance != instance) {
		msdbg_hw(2, "update_param: hw_ip->instance(%d)\n",
			instance, hw_ip, hw_byrp->instance);
		hw_byrp->instance = instance;
	}

	__is_hw_byrp_check_size(hw_ip, param_set, instance);

	ret = __is_hw_byrp_bypass(hw_ip, set_id);

	return ret;
}

static void __is_hw_byrp_update_param(struct is_hw_ip *hw_ip, struct is_param_region *p_region,
					struct byrp_param_set *param_set, IS_DECLARE_PMAP(pmap),
					u32 instance)
{
	struct byrp_param *param;

	FIMC_BUG_VOID(!hw_ip);
	FIMC_BUG_VOID(!param_set);

	param = &p_region->byrp;
	param_set->instance_id = instance;

	/* check input */
	if (test_bit(PARAM_BYRP_DMA_INPUT, pmap))
		memcpy(&param_set->dma_input, &param->dma_input,
				sizeof(struct param_dma_input));

	if (test_bit(PARAM_BYRP_HDR, pmap))
		memcpy(&param_set->dma_input_hdr, &param->hdr,
				sizeof(struct param_dma_input));

	/* check output*/
	if (test_bit(PARAM_BYRP_BYR, pmap))
		memcpy(&param_set->dma_output_byr, &param->byr,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_BYRP_STRIPE_INPUT, pmap))
		memcpy(&param_set->stripe_input, &param->stripe_input,
				sizeof(struct param_stripe_input));
}

static int is_hw_byrp_set_param(struct is_hw_ip *hw_ip, struct is_region *region,
		IS_DECLARE_PMAP(pmap), u32 instance, ulong hw_map)
{
	struct is_hw_byrp *hw_byrp;

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

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
	hw_byrp->instance = IS_STREAM_COUNT;

	return 0;
}

static void is_hw_byrp_s_size_regs(struct is_hw_ip *hw_ip, struct byrp_param_set *param_set,
	u32 instance, const struct is_frame *frame,
	struct is_crop *dma_crop_cfg, struct is_crop *bcrop_cfg)
{
	struct is_hw_byrp *hw;
	void __iomem *base;
	u32 dng_en = 0;
	struct byrp_grid_cfg grid_cfg;
	struct is_byrp_config *cfg;
	u32 strip_enable, start_pos_x, start_pos_y;
	u32 sensor_full_width, sensor_full_height;
	u32 sensor_binning_x, sensor_binning_y;
	u32 sensor_crop_x, sensor_crop_y;
	u32 byrp_crop_offset_x, byrp_crop_offset_y;
	u32 bns_binning_x, bns_binning_y;

	hw = (struct is_hw_byrp *)hw_ip->priv_info;
	base = hw_ip->regs[REG_SETA];
	cfg = &hw->config;
	dng_en = param_set->dma_output_byr.cmd;

	if (dng_en) {
		/* In DNG scenario, use BcropRgb in the end of chain */
		dma_crop_cfg->x = 0;
		dma_crop_cfg->y = 0;
		dma_crop_cfg->w = param_set->dma_input.width;
		dma_crop_cfg->h = param_set->dma_input.height;
		bcrop_cfg->x = param_set->dma_input.bayer_crop_offset_x;
		bcrop_cfg->y = param_set->dma_input.bayer_crop_offset_y;
		bcrop_cfg->w = param_set->dma_input.bayer_crop_width;
		bcrop_cfg->h = param_set->dma_input.bayer_crop_height;
	} else {
		/* In Normal scenario, use RDMA crop + BcropByr */
		bcrop_cfg->x = param_set->dma_input.bayer_crop_offset_x % 512;
		bcrop_cfg->y = 0;
		bcrop_cfg->w = param_set->dma_input.bayer_crop_width;
		bcrop_cfg->h = param_set->dma_input.bayer_crop_height;

		dma_crop_cfg->x = param_set->dma_input.bayer_crop_offset_x - bcrop_cfg->x;
		dma_crop_cfg->y = param_set->dma_input.bayer_crop_offset_y;
		dma_crop_cfg->w = ALIGN(param_set->dma_input.bayer_crop_width + bcrop_cfg->x, 512);
		/* when dma crop size is larger than dma input size */
		if (dma_crop_cfg->w > param_set->dma_input.width - dma_crop_cfg->x)
			dma_crop_cfg->w = param_set->dma_input.width - dma_crop_cfg->x;
		dma_crop_cfg->h = param_set->dma_input.bayer_crop_height;
	}

	msdbg_hw(2, "[HW][%s] rdma info w: %d, h: %d (%d, %d, %dx%d)\n",
		instance, hw_ip, __func__,
		param_set->dma_input.width, param_set->dma_input.height,
		param_set->dma_input.bayer_crop_offset_x,
		param_set->dma_input.bayer_crop_offset_y,
		param_set->dma_input.bayer_crop_width,
		param_set->dma_input.bayer_crop_height);
	msdbg_hw(2, "[HW][%s] rdma crop (%d %d, %dx%d), bcrop(%s, %d %d, %dx%d)\n",
		instance, hw_ip, __func__,
		dma_crop_cfg->x, dma_crop_cfg->y, dma_crop_cfg->w, dma_crop_cfg->h,
		dng_en ? "BCROP_RGB" : "BCROP_BYR",
		bcrop_cfg->x, bcrop_cfg->y, bcrop_cfg->w, bcrop_cfg->h);

	/* GRID configuration for CGRAS */
	strip_enable = (param_set->stripe_input.total_count == 0) ? 0 : 1;

	sensor_full_width = frame->shot->udm.frame_info.sensor_size[0];
	sensor_full_height = frame->shot->udm.frame_info.sensor_size[1];
	sensor_crop_x = frame->shot->udm.frame_info.sensor_crop_region[0];
	sensor_crop_y = frame->shot->udm.frame_info.sensor_crop_region[1];
	sensor_binning_x = frame->shot->udm.frame_info.sensor_binning[0];
	sensor_binning_y = frame->shot->udm.frame_info.sensor_binning[1];
	bns_binning_x = frame->shot->udm.frame_info.bns_binning[0];
	bns_binning_y = frame->shot->udm.frame_info.bns_binning[1];

	/* Center */
	if (cfg->sensor_center_x == 0 || cfg->sensor_center_y == 0) {
		cfg->sensor_center_x = sensor_full_width >> 1;
		cfg->sensor_center_y = sensor_full_height >> 1;
		msdbg_hw(2, "%s: cal_center(0,0) from DDK. Fix to (%d,%d)",
				instance, hw_ip, __func__,
				cfg->sensor_center_x, cfg->sensor_center_y);
	}

	/* Total_binning = sensor_binning * csis_bns_binning */
	grid_cfg.binning_x = (1024ULL * sensor_binning_x * bns_binning_x) / 1000 / 1000;
	grid_cfg.binning_y = (1024ULL * sensor_binning_y * bns_binning_y) / 1000 / 1000;
	if (grid_cfg.binning_x == 0 || grid_cfg.binning_y == 0) {
		grid_cfg.binning_x = 1024;
		grid_cfg.binning_y = 1024;
		msdbg_hw(2, "%s:[CGR] calculated binning(0,0). Fix to (%d,%d)",
				instance, hw_ip, __func__, 1024, 1024);
	}

	/* Step */
	grid_cfg.step_x = cfg->lsc_step_x;
	grid_cfg.step_y = cfg->lsc_step_y;

	/* Total_crop = unbinned(Stripe_start_position + BYRP_bcrop) + unbinned(sensor_crop) */
	start_pos_x = (strip_enable) ? param_set->stripe_input.start_pos_x : 0;

	if (dng_en) { /* No crop */
		byrp_crop_offset_x = 0;
		byrp_crop_offset_y = 0;
	} else { /* DMA crop + Bayer crop */
		byrp_crop_offset_x = param_set->dma_input.bayer_crop_offset_x;
		byrp_crop_offset_y = param_set->dma_input.bayer_crop_offset_y;
	}

	start_pos_x = (((start_pos_x + byrp_crop_offset_x) * grid_cfg.binning_x) / 1024) +
			(sensor_crop_x * sensor_binning_x) / 1000;
	start_pos_y = ((byrp_crop_offset_y * grid_cfg.binning_y) / 1024) +
			(sensor_crop_y * sensor_binning_y) / 1000;
	grid_cfg.crop_x = start_pos_x * grid_cfg.step_x;
	grid_cfg.crop_y = start_pos_y * grid_cfg.step_y;
	grid_cfg.crop_radial_x = (u16)((-1) * (cfg->sensor_center_x - start_pos_x));
	grid_cfg.crop_radial_y = (u16)((-1) * (cfg->sensor_center_y - start_pos_y));

	byrp_hw_s_grid_cfg(base, &grid_cfg);

	msdbg_hw(2, "%s:[CGR] stripe: enable(%d), pos_x(%d), full_size(%dx%d)\n",
			instance, hw_ip, __func__,
			strip_enable, param_set->stripe_input.start_pos_x,
			param_set->stripe_input.full_width,
			param_set->stripe_input.full_height);

	msdbg_hw(2, "%s:[CGR] dbg: calibrated_size(%dx%d), cal_center(%d,%d), sensor_crop(%d,%d), start_pos(%d,%d)\n",
			instance, hw_ip, __func__,
			sensor_full_width, sensor_full_height,
			cfg->sensor_center_x,
			cfg->sensor_center_y,
			sensor_crop_x, sensor_crop_y,
			start_pos_x, start_pos_y);

	msdbg_hw(2, "%s:[CGR] sfr: binning(%dx%d), step(%dx%d), crop(%d,%d), crop_radial(%d,%d)\n",
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

static int __is_hw_byrp_set_size_regs(struct is_hw_ip *hw_ip, struct byrp_param_set *param_set,
	u32 instance, const struct is_frame *frame, struct is_crop *bcrop_cfg, u32 set_id)
{
	struct is_hw_byrp *hw_byrp;
	struct is_hw_size_config size_config;
	int ret = 0;
	bool dng_en;
	bool hdr_en;
	u32 strip_enable;
	u32 rdma_width, rdma_height;
	u32 frame_width;
	u32 region_id;
	u32 bcrop_x, bcrop_y, bcrop_w, bcrop_h;
	u32 pipe_w, pipe_h;
	u32 crop_x, crop_y, crop_w, crop_h;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param_set);

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	strip_enable = (param_set->stripe_input.total_count == 0) ? 0 : 1;

	region_id = param_set->stripe_input.index;
	rdma_width = param_set->dma_input.width;
	rdma_height = param_set->dma_input.height;

	/* Bayer crop, belows are configured for detail cropping */
	dng_en = param_set->dma_output_byr.cmd;
	hdr_en = param_set->dma_input_hdr.cmd;
	bcrop_x = bcrop_cfg->x;
	bcrop_y = bcrop_cfg->y;
	bcrop_w = bcrop_cfg->w;
	bcrop_h = bcrop_cfg->h;
	frame_width = (strip_enable) ? param_set->stripe_input.full_width : rdma_width;

	/* Belows are for block configuration */
	size_config.sensor_calibrated_width = frame->shot->udm.frame_info.sensor_size[0];
	size_config.sensor_calibrated_height = frame->shot->udm.frame_info.sensor_size[1];
	size_config.sensor_binning_x = frame->shot->udm.frame_info.sensor_binning[0];
	size_config.sensor_binning_y = frame->shot->udm.frame_info.sensor_binning[1];
	size_config.sensor_crop_x = frame->shot->udm.frame_info.sensor_crop_region[0];
	size_config.sensor_crop_y = frame->shot->udm.frame_info.sensor_crop_region[1];
	size_config.sensor_crop_width = frame->shot->udm.frame_info.sensor_crop_region[2];
	size_config.sensor_crop_height = frame->shot->udm.frame_info.sensor_crop_region[3];

	if (dng_en) {
		pipe_w = rdma_width;
		pipe_h = rdma_height;
		byrp_hw_s_sdc_size(hw_ip->regs[REG_SETA], set_id, rdma_width, rdma_height);
		byrp_hw_s_mpc_size(hw_ip->regs[REG_SETA], set_id, rdma_width, rdma_height);
		byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_BYR,
					0, 0, rdma_width, rdma_height);
		byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_RGB,
					bcrop_x, bcrop_y, bcrop_w, bcrop_h);
		byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_STRP,
					bcrop_x, bcrop_y, bcrop_w, bcrop_h);
		if (strip_enable) {
			crop_x = param_set->stripe_input.left_margin;
			crop_y = 0;
			crop_w = rdma_width - param_set->stripe_input.left_margin -
						param_set->stripe_input.right_margin;
			crop_h = rdma_height;

			byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_ZSL,
					crop_x, crop_y, crop_w, crop_h);
		} else
			byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_ZSL,
					0, 0, pipe_w, pipe_h);
	} else {
		pipe_w = bcrop_w;
		pipe_h = bcrop_h;
		byrp_hw_s_sdc_size(hw_ip->regs[REG_SETA], set_id, bcrop_w + bcrop_x * 2, pipe_h + bcrop_h * 2);
		byrp_hw_s_mpc_size(hw_ip->regs[REG_SETA], set_id, bcrop_w + bcrop_x * 2, pipe_h + bcrop_h * 2);
		byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_BYR,
					bcrop_x, bcrop_y, bcrop_w, bcrop_h);
		byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_RGB,
					0, 0, bcrop_w, bcrop_h);
		byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_STRP,
					0, 0, bcrop_w, bcrop_h);
		byrp_hw_s_bcrop_size(hw_ip->regs[REG_SETA], set_id, BYRP_BCROP_ZSL, 0, 0, pipe_w, pipe_h);
	}

	byrp_hw_s_chain_size(hw_ip->regs[REG_SETA], set_id, pipe_w, pipe_h);
	byrp_hw_s_bpc_size(hw_ip->regs[REG_SETA], set_id, pipe_w, pipe_h);
	byrp_hw_s_disparity_size(hw_ip->regs[REG_SETA], set_id, &size_config);
	byrp_hw_s_susp_size(hw_ip->regs[REG_SETA], set_id, pipe_w, pipe_h);
	byrp_hw_s_ggc_size(hw_ip->regs[REG_SETA], set_id, pipe_w, pipe_h);
	byrp_hw_s_otf_hdr_size(hw_ip->regs[REG_SETA], hdr_en, set_id, pipe_h);

	return ret;
}

static int is_hw_byrp_shot(struct is_hw_ip *hw_ip, struct is_frame *frame,
	ulong hw_map)
{
	struct is_hw_byrp *hw_byrp = NULL;
	struct is_param_region *param_region;
	struct byrp_param_set *param_set = NULL;
	struct is_frame *dma_frame;
	u32 fcount, instance;
	u32 cur_idx;
	u32 set_id;
	int ret = 0, i = 0, batch_num = 0;
	u32 cmd_input, cmd_output;
	struct is_crop dma_crop_cfg, bcrop_cfg;
	bool do_blk_cfg;
	u32 strip_index, strip_total_count;
	bool skip = false;

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
	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
	param_set = &hw_byrp->param_set[instance];
	param_region = frame->parameter;

	atomic_set(&hw_byrp->strip_index, frame->stripe_info.region_id);
	fcount = frame->fcount;
	dma_frame = frame;

	FIMC_BUG(!frame->shot);

	if (hw_ip->internal_fcount[instance] != 0)
		hw_ip->internal_fcount[instance] = 0;

	__is_hw_byrp_update_param(hw_ip, param_region, param_set, frame->pmap, instance);

	/* DMA settings */
	cur_idx = frame->cur_buf_index;

	cmd_input = CALL_HW_OPS(hw_ip, dma_cfg, "byrp_img", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_input.cmd,
			param_set->dma_input.plane,
			param_set->input_dva,
			dma_frame->dvaddr_buffer);

	cmd_input = CALL_HW_OPS(hw_ip, dma_cfg, "byrp_hdr", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_input_hdr.cmd,
			param_set->dma_input_hdr.plane,
			param_set->input_dva_hdr,
			dma_frame->dva_byrp_hdr);

	cmd_output = CALL_HW_OPS(hw_ip, dma_cfg, "byrp_wdma", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_byr.cmd,
			param_set->dma_output_byr.plane,
			param_set->output_dva_byr,
			dma_frame->ixcTargetAddress);

	param_set->instance_id = instance;
	param_set->fcount = fcount;
	/* multi-buffer */
	hw_ip->num_buffers = dma_frame->num_buffers;
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

	if (IS_ENABLED(SKIP_ISP_SHOT_FOR_MULTI_SHOT)) {
		if (!pablo_is_first_shot(batch_num, cur_idx)) skip = true;
		if (!pablo_is_first_shot(strip_total_count, strip_index)) skip = true;
		if (!pablo_is_first_shot(param_set->stripe_input.repeat_num,
			param_set->stripe_input.repeat_idx)) skip = true;
	}

	/* TODO: change to use COREX setA/B */
	set_id = COREX_DIRECT;

	byrp_hw_s_core(hw_ip->regs[REG_SETA], frame->num_buffers, set_id, param_set);

	ret = CALL_HW_HELPER_OPS(hw_ip, lib_shot, instance, skip,
				frame, &hw_byrp->lib[instance], param_set);
	if (ret)
		return ret;

	do_blk_cfg = CALL_HW_HELPER_OPS(hw_ip, set_rta_regs, instance, set_id, skip, frame, NULL);
	if (unlikely(do_blk_cfg))
		__is_hw_byrp_update_block_reg(hw_ip, param_set, instance, set_id);

	is_hw_byrp_s_size_regs(hw_ip, param_set, instance, frame, &dma_crop_cfg, &bcrop_cfg);

	ret = __is_hw_byrp_set_size_regs(hw_ip, param_set, instance, frame, &bcrop_cfg, set_id);
	if (ret) {
		mserr_hw("__is_hw_byrp_set_size_regs is fail", instance, hw_ip);
		goto shot_fail_recovery;
	}

	for (i = BYRP_RDMA_IMG; i < BYRP_RDMA_MAX; i++) {
		ret = __is_hw_byrp_set_rdma(hw_ip, hw_byrp, param_set, instance,
						i, &dma_crop_cfg, set_id);
		if (ret) {
			mserr_hw("__is_hw_byrp_set_rdma is fail", instance, hw_ip);
			goto shot_fail_recovery;
		}
	};

	for (i = BYRP_WDMA_BYR; i < BYRP_WDMA_MAX; i++) {
		ret = __is_hw_byrp_set_wdma(hw_ip, hw_byrp, param_set, instance,
						i, set_id);
		if (ret) {
			mserr_hw("__is_hw_byrp_set_wdma is fail", instance, hw_ip);
			goto shot_fail_recovery;
		}
	};

	if (param_region->byrp.control.strgen == CONTROL_COMMAND_START) {
		msdbg_hw(2, "STRGEN input\n", instance, hw_ip);
		byrp_hw_s_strgen(hw_ip->regs[REG_SETA], set_id);
	}

	CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, fcount, DEBUG_POINT_ADD_TO_CMDQ);
	ret = is_hw_byrp_set_cmdq(hw_ip, instance, set_id);
	if (ret < 0) {
		mserr_hw("is_hw_byrp_set_cmdq is fail", instance, hw_ip);
		goto shot_fail_recovery;
	}

	if (unlikely(debug_byrp))
		byrp_hw_dump(hw_ip->regs[REG_SETA]);

	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;

shot_fail_recovery:
	CALL_HW_HELPER_OPS(hw_ip, restore, instance, &hw_byrp->lib[instance]);

	return ret;
}

static int is_hw_byrp_get_meta(struct is_hw_ip *hw_ip, struct is_frame *frame,
	ulong hw_map)
{
	struct is_hw_byrp *hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, get_meta, frame->instance,
				frame, &hw_byrp->lib[frame->instance]);
}

static int is_hw_byrp_get_cap_meta(struct is_hw_ip *hw_ip, ulong hw_map,
	u32 instance, u32 fcount, u32 size, ulong addr)
{
	struct is_hw_byrp *hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, get_cap_meta, instance,
				&hw_byrp->lib[instance], fcount, size, addr);
}

static int is_hw_byrp_frame_ndone(struct is_hw_ip *hw_ip, struct is_frame *frame,
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

static int is_hw_byrp_load_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_byrp *hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, load_setfile, instance, &hw_byrp->lib[instance]);
}

static int is_hw_byrp_apply_setfile(struct is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	struct is_hw_byrp *hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, apply_setfile, instance, &hw_byrp->lib[instance],
				scenario);
}

static int is_hw_byrp_delete_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_byrp *hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, delete_setfile, instance, &hw_byrp->lib[instance]);
}

int is_hw_byrp_restore(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_byrp *hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return -EINVAL;

	if (hw_ip->lib_mode == BYRP_USE_DRIVER) {
		msinfo_hw("hw_byrp_reset\n", instance, hw_ip);

		if (__is_hw_byrp_s_common_reg(hw_ip, instance)) {
			mserr_hw("sw reset fail", instance, hw_ip);
			return -ENODEV;
		}
	}

	return CALL_HW_HELPER_OPS(hw_ip, restore, instance, &hw_byrp->lib[instance]);
}

static int is_hw_byrp_set_regs(struct is_hw_ip *hw_ip, u32 chain_id,
	u32 instance, u32 fcount, struct cr_set *regs, u32 regs_size)
{
	return CALL_HW_HELPER_OPS(hw_ip, set_regs, instance, regs, regs_size);
}

int is_hw_byrp_dump_regs(struct is_hw_ip *hw_ip, u32 instance, u32 fcount,
	struct cr_set *regs, u32 regs_size, enum is_reg_dump_type dump_type)
{
	void __iomem *base = 0;
	struct is_common_dma *dma;
	struct is_hw_byrp *hw_byrp = NULL;
	u32 i;

	switch (dump_type) {
	case IS_REG_DUMP_TO_LOG:
		base = hw_ip->regs[REG_SETA];
		byrp_hw_dump(base);
		break;
	case IS_REG_DUMP_DMA:
		for (i = BYRP_RDMA_IMG; i < BYRP_RDMA_MAX; i++) {
			hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
			dma = &hw_byrp->rdma[i];
			CALL_DMA_OPS(dma, dma_print_info, 0);
		}

		for (i = BYRP_WDMA_BYR; i < BYRP_WDMA_MAX; i++) {
			hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
			dma = &hw_byrp->wdma[i];
			CALL_DMA_OPS(dma, dma_print_info, 0);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int is_hw_byrp_set_config(struct is_hw_ip *hw_ip, u32 chain_id, u32 instance, u32 fcount, void *config)
{
	struct is_hw_byrp *hw_byrp = NULL;
	struct is_byrp_config *conf = (struct is_byrp_config *)config;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!conf);

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
	hw_byrp->config = *conf;

	return 0;
}

static int is_hw_byrp_notify_timeout(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_byrp *hw_byrp;

	hw_byrp = (struct is_hw_byrp *)hw_ip->priv_info;
	if (!hw_byrp) {
		mserr_hw("failed to get HW BYRP", instance, hw_ip);
		return -ENODEV;
	}
	/* DEBUG */
	byrp_hw_dump(hw_ip->regs[REG_SETA]);

	return CALL_HW_HELPER_OPS(hw_ip, notify_timeout, instance, &hw_byrp->lib[instance]);
}

const struct is_hw_ip_ops is_hw_byrp_ops = {
	.open			= is_hw_byrp_open,
	.init			= is_hw_byrp_init,
	.deinit			= is_hw_byrp_deinit,
	.close			= is_hw_byrp_close,
	.enable			= is_hw_byrp_enable,
	.disable		= is_hw_byrp_disable,
	.shot			= is_hw_byrp_shot,
	.set_param		= is_hw_byrp_set_param,
	.get_meta		= is_hw_byrp_get_meta,
	.get_cap_meta		= is_hw_byrp_get_cap_meta,
	.frame_ndone		= is_hw_byrp_frame_ndone,
	.load_setfile		= is_hw_byrp_load_setfile,
	.apply_setfile		= is_hw_byrp_apply_setfile,
	.delete_setfile		= is_hw_byrp_delete_setfile,
	.restore		= is_hw_byrp_restore,
	.set_regs		= is_hw_byrp_set_regs,
	.dump_regs		= is_hw_byrp_dump_regs,
	.set_config		= is_hw_byrp_set_config,
	.notify_timeout		= is_hw_byrp_notify_timeout,
};

int is_hw_byrp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name)
{
	int hw_slot;

	hw_ip->ops  = &is_hw_byrp_ops;

	hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, id);
	if (!valid_hw_slot_id(hw_slot)) {
		serr_hw("invalid hw_slot (%d)", hw_ip, hw_slot);
		return -EINVAL;
	}

	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &is_hw_byrp_handle_interrupt;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].id = INTR_HWIP1;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].valid = true;

	return 0;
}

void is_hw_byrp_remove(struct is_hw_ip *hw_ip)
{
	struct is_interface_ischain *itfc = hw_ip->itfc;
	int id = hw_ip->id;
	int hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, id);

	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].valid = false;
}
