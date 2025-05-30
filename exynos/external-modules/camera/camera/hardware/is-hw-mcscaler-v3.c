/*
 * Samsung Exynos SoC series Pablo driver
 *
 * MCSC HW control functions
 *
 * Copyright (C) 2019 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-hw-mcscaler-v3.h"
#include "api/is-hw-api-mcscaler-v3.h"
#include "../interface/is-interface-ischain.h"
#include "is-param.h"
#include "is-err.h"
#include <linux/videodev2_exynos_media.h>
#include "pablo-lib.h"

static ulong debug_mcsc;

static int param_get_debug_mcsc(char *buffer, const struct kernel_param *kp)
{
	int ret;

	ret = sprintf(buffer, "MCSC debug features\n");
	ret += sprintf(buffer + ret, "\tb[0] : dump sfr\n");
	ret += sprintf(buffer + ret, "\tb[1] : dump sfr once\n");
	ret += sprintf(buffer + ret, "\tb[2] : s2d\n");
	ret += sprintf(buffer + ret, "\tb[3] : skip setfile\n");
	ret += sprintf(buffer + ret, "\tb[4] : bypass\n");
	ret += sprintf(buffer + ret, "\tb[5] : DTP OTF\n");
	ret += sprintf(buffer + ret, "\tb[6] : DTP RDMA\n");
	ret += sprintf(buffer + ret, "\tb[7] : STRGEN\n");
	ret += sprintf(buffer + ret, "\tcurrent value : %ld\n", debug_mcsc);

	return ret;
}

static const struct kernel_param_ops param_ops_debug_mcsc = {
	.set = param_set_ulong,
	.get = param_get_debug_mcsc,
};

module_param_cb(debug_mcsc, &param_ops_debug_mcsc, &debug_mcsc, S_IRUGO | S_IWUSR);

static int is_hw_mcsc_rdma_cfg(struct is_hw_ip *hw_ip, struct is_frame *frame,
				struct param_mcs_input *input);
static void is_hw_mcsc_wdma_cfg(struct is_hw_ip *hw_ip, struct is_frame *frame,
				struct mcs_param *mcs_param);
static void is_hw_mcsc_size_dump(struct is_hw_ip *hw_ip);

void is_hw_mcsc_s_debug_type(int type)
{
	set_bit(type, &debug_mcsc);
}
KUNIT_EXPORT_SYMBOL(is_hw_mcsc_s_debug_type);

void is_hw_mcsc_c_debug_type(int type)
{
	clear_bit(type, &debug_mcsc);
}
KUNIT_EXPORT_SYMBOL(is_hw_mcsc_c_debug_type);

static int is_hw_mcsc_handle_interrupt(u32 id, void *context)
{
	struct is_hardware *hardware;
	struct is_hw_ip *hw_ip = NULL;
	u32 status, mask, instance, hw_fcount, hl = 0, vl = 0;
	bool f_err = false;
	int ret = 0;

	hw_ip = (struct is_hw_ip *)context;
	hardware = hw_ip->hardware;
	hw_fcount = atomic_read(&hw_ip->fcount);
	instance = atomic_read(&hw_ip->instance);

	is_scaler_get_input_status(hw_ip->regs[REG_SETA], hw_ip->id, &hl, &vl);
	/* read interrupt status register (sc_intr_status) */
	mask = is_scaler_get_intr_mask(hw_ip->regs[REG_SETA], hw_ip->id);
	status = is_scaler_get_intr_status(hw_ip->regs[REG_SETA], hw_ip->id);
	status = (~mask) & status;

	is_scaler_clear_intr_src(hw_ip->regs[REG_SETA], hw_ip->id, status);

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
		goto p_err;
	}

	if (is_scaler_intr_occurred0(status, INTR_OVERFLOW)) {
		mserr_hw("Overflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (is_scaler_intr_occurred0(status, INTR_OUTSTALL)) {
		mserr_hw("Output Block BLOCKING!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (is_scaler_intr_occurred0(status, INTR_INPUT_VERTICAL_UNF)) {
		mserr_hw("Input OTF Vertical Underflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (is_scaler_intr_occurred0(status, INTR_INPUT_VERTICAL_OVF)) {
		mserr_hw("Input OTF Vertical Overflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (is_scaler_intr_occurred0(status, INTR_INPUT_HORIZONTAL_UNF)) {
		mserr_hw("Input OTF Horizontal Underflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (is_scaler_intr_occurred0(status, INTR_INPUT_HORIZONTAL_OVF)) {
		mserr_hw("Input OTF Horizontal Overflow!! (0x%x)", instance, hw_ip, status);
		f_err = true;
	}

	if (is_scaler_intr_occurred0(status, INTR_WDMA_FINISH))
		mserr_hw("Disabeld interrupt occurred! WDAM FINISH!! (0x%x)", instance, hw_ip, status);

	if (is_scaler_intr_occurred0(status, INTR_SETTING_DONE))
		 CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount, DEBUG_POINT_SETTING_DONE);

	if (is_scaler_intr_occurred0(status, INTR_FRAME_START) && is_scaler_intr_occurred0(status, INTR_FRAME_END))
		mswarn_hw("start/end overlapped!! (0x%x)", instance, hw_ip, status);

	if (is_scaler_intr_occurred0(status, INTR_FRAME_START)) {
		atomic_add(1, &hw_ip->count.fs);

		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount, DEBUG_POINT_FRAME_START);

		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			msinfo_hw("[F:%d]F.S\n", instance, hw_ip, hw_fcount);

		CALL_HW_OPS(hw_ip, frame_start, hw_ip, instance);
	}

	if (is_scaler_intr_occurred0(status, INTR_FRAME_END)) {
		atomic_add(1, &hw_ip->count.fe);

		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount, DEBUG_POINT_FRAME_END);

		CALL_HW_OPS(hw_ip, frame_done, hw_ip, NULL, -1, IS_HW_CORE_END,
				IS_SHOT_SUCCESS, true);

		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			msinfo_hw("[F:%d]F.E\n", instance, hw_ip, hw_fcount);

		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
			mserr_hw("fs(%d), fe(%d), dma(%d), status(0x%x)", instance, hw_ip,
				atomic_read(&hw_ip->count.fs),
				atomic_read(&hw_ip->count.fe),
				atomic_read(&hw_ip->count.dma), status);
		}

		wake_up(&hw_ip->status.wait_queue);

		if (unlikely(test_bit(MCSC_DBG_S2D, &debug_mcsc)))
			is_debug_s2d(true, "MCSC_DBG_S2D");
	}

	/* for handle chip dependant intr */
	f_err |= is_scaler_handle_extended_intr(status);

	if (f_err) {
		msinfo_hw("[F:%d] Ocurred error interrupt (%d,%d) status(0x%x)\n",
			instance, hw_ip, hw_fcount, hl, vl, status);
		is_scaler_dump(hw_ip->regs[REG_SETA]);
	}

p_err:
	return ret;
}

static void is_hw_mcsc_hw_info(struct is_hw_ip *hw_ip, u32 instance, struct is_hw_mcsc_cap *cap)
{
	int i = 0;

	if (!(hw_ip && cap))
		return;

	msinfo_hw("==== h/w info(ver:0x%X) ====\n", instance, hw_ip, cap->hw_ver);
	msinfo_hw("[IN] out:%d, in(otf:%d/dma:%d), hwfc:%d, djag:%d, ds_vra:%d, ysum:%d\n",
		instance, hw_ip,
		cap->max_output, cap->in_otf, cap->in_dma, cap->hwfc,
		cap->djag, cap->ds_vra, cap->ysum);
	for (i = MCSC_OUTPUT0; i < cap->max_output; i++)
		minfo_hw("[OUT%d] out(otf/dma):%d/%d, hwfc:%d\n", instance, i,
			cap->out_otf[i], cap->out_dma[i], cap->out_hwfc[i]);
	msinfo_hw("========================\n", instance, hw_ip);
}

static int is_hw_mcsc_open(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0, i, j;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, "HWMCS");
	frame_manager_open(hw_ip->framemgr, IS_MAX_HW_FRAME, false);

	hw_ip->priv_info = vzalloc(sizeof(struct is_hw_mcsc));
	if (!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	hw_mcsc->instance = IS_STREAM_COUNT;

	cap = GET_MCSC_HW_CAP(hw_ip);
	CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, query_cap, DEV_HW_MCSC0, (void *)cap);
	is_hw_mcsc_hw_info(hw_ip, instance, cap);

	atomic_set(&hw_ip->status.Vvalid, V_BLANK);
	set_bit(HW_OPEN, &hw_ip->state);
	msdbg_hw(2, "open: framemgr[%s]", instance, hw_ip, hw_ip->framemgr->name);

	for (i = 0; i < SENSOR_POSITION_MAX; i++) {
		for (j = 0; j < IS_STREAM_COUNT; j++)
			hw_mcsc->cur_setfile[i][j] = NULL;
	}

	if (unlikely(test_bit(MCSC_DBG_DTP_OTF, &debug_mcsc)))
		mcsc_hw_s_dtp(hw_ip->regs[REG_SETA], MCSC_DTP_OTF, 1, MCSC_DTP_COLOR_BAR,
			0, 0, 0, MCSC_DTP_COLOR_BAR_BT601);
	else
		mcsc_hw_s_dtp(hw_ip->regs[REG_SETA], MCSC_DTP_OTF, 0, MCSC_DTP_BYPASS,
			0, 0, 0, MCSC_DTP_COLOR_BAR_BT601);

	if (unlikely(test_bit(MCSC_DBG_DTP_RDMA, &debug_mcsc)))
		mcsc_hw_s_dtp(hw_ip->regs[REG_SETA], MCSC_DTP_RDMA, 1, MCSC_DTP_COLOR_BAR,
			0, 0, 0, MCSC_DTP_COLOR_BAR_BT601);
	else
		mcsc_hw_s_dtp(hw_ip->regs[REG_SETA], MCSC_DTP_RDMA, 0, MCSC_DTP_BYPASS,
			0, 0, 0, MCSC_DTP_COLOR_BAR_BT601);

	msinfo_hw("mcsc_open: done\n", instance, hw_ip);

	return 0;

err_alloc:
	frame_manager_close(hw_ip->framemgr);

	return ret;
}

static int is_hw_mcsc_init(struct is_hw_ip *hw_ip, u32 instance,
			bool flag, u32 f_type)
{
	int ret = 0;
	u32 output_id;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap;

	FIMC_BUG(!hw_ip);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	if (!hw_mcsc) {
		mserr_hw("hw_mcsc is null ", instance, hw_ip);
		ret = -ENODATA;
		goto err_priv_info;
	}

	hw_mcsc->rep_flag[instance] = flag;

	cap = GET_MCSC_HW_CAP(hw_ip);
	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		if (test_bit(output_id, &hw_mcsc->out_en)) {
			msdbg_hw(2, "already set output(%d)\n", instance, hw_ip, output_id);
			continue;
		}

		set_bit(output_id, &hw_mcsc->out_en);
	}

	set_bit(HW_INIT, &hw_ip->state);
	msdbg_hw(2, "init: out_en[0x%lx]\n", instance, hw_ip, hw_mcsc->out_en);

err_priv_info:

	return ret;
}

static int is_hw_mcsc_deinit(struct is_hw_ip *hw_ip, u32 instance)
{
	return 0;
}

static int is_hw_mcsc_close(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
	frame_manager_close(hw_ip->framemgr);

	clear_bit(HW_OPEN, &hw_ip->state);

	/* For safe power off */
	ret = is_scaler_sw_reset(hw_ip->regs[REG_SETA], hw_ip->id, 0, 0);
	if (ret != 0)
		mserr_hw("sw reset fail", instance, hw_ip);

	return ret;
}

static int is_hw_mcsc_enable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
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
		return ret;

	msdbg_hw(2, "enable: start\n", instance, hw_ip);

	ret = is_hw_mcsc_reset(hw_ip, instance);
	if (ret != 0) {
		mserr_hw("MCSC sw reset fail", instance, hw_ip);
		return -ENODEV;
	}

	ret = is_hw_mcsc_clear_interrupt(hw_ip, instance);
	if (ret != 0) {
		mserr_hw("MCSC sw reset fail", instance, hw_ip);
		return -ENODEV;
	}

	msdbg_hw(2, "enable: done\n", instance, hw_ip);

	set_bit(HW_RUN, &hw_ip->state);

	return ret;
}

static void is_hw_mcsc_free_buffer(void)
{
	struct pablo_lib_platform_data *plpd = pablo_lib_get_platform_data();
	struct pablo_internal_subdev *sd = &plpd->sd_votf[PABLO_LIB_I_SUBDEV_VOTF_HF];

	if (!test_bit(PABLO_SUBDEV_ALLOC, &sd->state))
		return;

	if (CALL_I_SUBDEV_OPS(sd, free, sd))
		mserr("failed to free", sd, sd);

	msinfo("%s\n", sd, sd, __func__);
}

static int is_hw_mcsc_disable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	u32 output_id;
	long timetowait;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	msinfo_hw("mcsc_disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	timetowait = wait_event_timeout(hw_ip->status.wait_queue,
			!atomic_read(&hw_ip->status.Vvalid),
			IS_HW_STOP_TIMEOUT);

	if (!timetowait) {
		mserr_hw("wait FRAME_END timeout (%ld)", instance,
			hw_ip, timetowait);
		return -ETIME;
	}

	if (hw_ip->run_rsc_state)
		return 0;

	ret = is_hw_mcsc_clear_interrupt(hw_ip, instance);
	if (ret != 0) {
		mserr_hw("MCSC sw clear_interrupt fail", instance, hw_ip);
		return -ENODEV;
	}

	is_scaler_stop(hw_ip->regs[REG_SETA], hw_ip->id);

	/* disable MCSC */
	if (cap->in_dma == MCSC_CAP_SUPPORT)
		is_scaler_clear_rdma_addr(hw_ip->regs[REG_SETA]);

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		msinfo_hw("[OUT:%d]hw_mcsc_disable: clear_wdma_addr\n", instance, hw_ip, output_id);
		is_scaler_clear_wdma_addr(hw_ip->regs[REG_SETA], output_id);
	}

	is_scaler_clear_shadow_ctrl(hw_ip->regs[REG_SETA], hw_ip->id);
	is_hw_mcsc_free_buffer();

	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);

	msinfo_hw("mcsc_disable: done\n", instance, hw_ip);

	return ret;
}

static int is_hw_mcsc_rdma_cfg(struct is_hw_ip *hw_ip, struct is_frame *frame,
	struct param_mcs_input *input)
{
	int ret = 0;
	u8 buf_i, dva_i;
	u32 cmd_input;
	pdma_addr_t dst_dva[IS_MAX_PLANES] = {0};
	pdma_addr_t rdma_addr[3] = {0};
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	/* can't support this function */
	if (cap->in_dma != MCSC_CAP_SUPPORT)
		return ret;

	if (input->dma_cmd == DMA_INPUT_COMMAND_DISABLE)
		return ret;

	cmd_input = CALL_HW_OPS(hw_ip, dma_cfg, "mcsc_in", hw_ip, frame,
			frame->cur_buf_index, frame->num_buffers,
			&input->dma_cmd, input->plane,
			dst_dva, frame->dvaddr_buffer);

	/* DMA in */
	msdbg_hw(2, "[F:%d]rdma_cfg [addr: %x]\n",
		frame->instance, hw_ip, frame->fcount, dst_dva[0]);

	if (!dst_dva[0]) {
		mserr_hw("Wrong dst_dva(%x)\n",
			frame->instance, hw_ip, dst_dva[0]);
		msdbg_hw(2, "[F:%d]rdma disabled\n",
			frame->instance, hw_ip, frame->fcount);
		is_scaler_clear_rdma_addr(hw_ip->regs[REG_SETA]);
		is_scaler_set_rdma_enable(hw_ip->regs[REG_SETA], hw_ip->id, false);
		ret = -EINVAL;
		return ret;
	}

	/* use only one buffer (per-frame) */
	is_scaler_set_rdma_frame_seq(hw_ip->regs[REG_SETA], 0x1 << USE_DMA_BUFFER_INDEX);

	for (buf_i = 0; buf_i < frame->num_buffers; buf_i++) {
		dva_i = buf_i * input->plane;
		memset(rdma_addr, 0x0, sizeof(rdma_addr));

		switch (input->plane) {
		case DMA_INOUT_PLANE_4:
			/* 8+2(10bit) format */
			rdma_addr[0] = dst_dva[dva_i];
			rdma_addr[1] = dst_dva[1 + dva_i];
			is_scaler_set_rdma_2bit_addr(hw_ip->regs[REG_SETA],
				dst_dva[2 + dva_i],
				dst_dva[3 + dva_i],
				buf_i);
			break;
		case DMA_INOUT_PLANE_3:
			rdma_addr[2] = dst_dva[2 + dva_i];
		case DMA_INOUT_PLANE_2:
			rdma_addr[1] = dst_dva[1 + dva_i];
		case DMA_INOUT_PLANE_1:
			rdma_addr[0] = dst_dva[dva_i];
			break;
		default:
			mserr_hw("Invalid plane_num(%d)\n",
				frame->instance, hw_ip, input->plane);
			ret = -EINVAL;
			return ret;
		}

		is_scaler_set_rdma_addr(hw_ip->regs[REG_SETA],
			rdma_addr[0], rdma_addr[1], rdma_addr[2],
			buf_i);
	}
	msdbg_hw(2, "[F:%d]rdma enabled\n", frame->instance, hw_ip, frame->fcount);
	is_scaler_set_rdma_enable(hw_ip->regs[REG_SETA], hw_ip->id, true);

	input->dma_cmd = cmd_input;

	return ret;
}

static dma_addr_t *hw_mcsc_get_target_addr(u32 out_id, struct is_frame *frame)
{
	dma_addr_t *addr = NULL;

	switch (out_id) {
	case MCSC_OUTPUT0:
		addr = frame->sc0TargetAddress;
		break;
	case MCSC_OUTPUT1:
		addr = frame->sc1TargetAddress;
		break;
	case MCSC_OUTPUT2:
		addr = frame->sc2TargetAddress;
		break;
	case MCSC_OUTPUT3:
		addr = frame->sc3TargetAddress;
		break;
	case MCSC_OUTPUT4:
		addr = frame->sc4TargetAddress;
		break;
	case MCSC_OUTPUT5:
		addr = frame->sc5TargetAddress;
		break;
	default:
		panic("[F:%d] invalid output id(%d)", frame->fcount, out_id);
		break;
	}

	return addr;
}

static void hw_mcsc_set_wdma_addr(struct is_hw_ip *hw_ip, dma_addr_t *wdma_addr,
	u32 out_id, u32 plane, u32 buf_idx)
{
	u32 addr[4];

	if (plane == DMA_INOUT_PLANE_4) {
		/* addr_2bit_y, addr_2bit_uv */
		addr[0] = wdma_addr[0];
		addr[1] = wdma_addr[1];
		addr[2] = 0;
		is_scaler_set_wdma_2bit_addr(hw_ip->regs[REG_SETA], out_id,
				wdma_addr[2], wdma_addr[3], buf_idx);
	} else {
		addr[0] = wdma_addr[0];
		addr[1] = wdma_addr[1];
		addr[2] = wdma_addr[2];
	}
	is_scaler_set_wdma_addr(hw_ip->regs[REG_SETA], out_id,
		addr[0], addr[1], addr[2], buf_idx);
}

static void is_hw_mcsc_wdma_clear(struct is_hw_ip *hw_ip, struct is_frame *frame,
	struct mcs_param *param, u32 out_id)
{
	u32 wdma_enable = 0;

	wdma_enable = is_scaler_get_dma_out_enable(hw_ip->regs[REG_SETA], out_id);

	if (wdma_enable) {
		is_scaler_set_dma_out_enable(hw_ip->regs[REG_SETA], out_id, false);
		is_scaler_clear_wdma_addr(hw_ip->regs[REG_SETA], out_id);
		msdbg_hw(2, "[OUT:%d]shot: dma_out disabled\n",
			frame->instance, hw_ip, out_id);

	}
	msdbg_hw(2, "[OUT:%d]mcsc_wdma_clear: en(%d)[F:%d][T:%d][cmd:%d][addr:0x0]\n",
		frame->instance, hw_ip, out_id, wdma_enable, frame->fcount, frame->type,
		param->output[out_id].dma_cmd);
}

static int is_hw_mcsc_calculate_wdma_offset(struct is_hw_ip *hw_ip, struct is_frame *frame,
						struct mcs_param *mcs_param,
						u32 plane_idx, u32 out_id)
{
	u32 stride, shift, offset = 0;

	/* It is used only for NV21M format */
	if (mcs_param->output[out_id].dma_format == DMA_INOUT_FORMAT_YUV420 &&
	    mcs_param->output[out_id].plane == DMA_INOUT_PLANE_2) {
		stride = (plane_idx == 0) ? mcs_param->output[out_id].dma_stride_y :
						mcs_param->output[out_id].dma_stride_c;
		shift = (plane_idx == 0) ? 0 : 1;
		offset = mcs_param->output[out_id].offset_x +
			((mcs_param->output[out_id].offset_y * stride) >> shift);
	}
	return offset;
}

static void is_hw_mcsc_wdma_cfg(struct is_hw_ip *hw_ip, struct is_frame *frame,
				struct mcs_param *mcs_param)
{
	struct param_mcs_output *output;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	struct is_hw_mcsc *hw_mcsc;
	dma_addr_t dst_dva[4] = {0}, *src_dva = NULL;
	u8 buf_i, plane_i, src_dva_i, src_p_i;
	u32 plane, out_id, offset = 0;
	u32 target_out_id;
#ifdef USE_MCSC_STRIP_OUT_CROP
	u32 stripe_dma_offset;
	u32 roi_x;
#endif
#ifdef MCSC_FLIP_WA
	u32 otcrop_x, otcrop_w;
#endif
	struct is_hardware *hardware;
	bool en;
	bool use_sub_flip;
	bool hw_fro_en;

	FIMC_BUG_VOID(!cap);
	FIMC_BUG_VOID(!hw_ip->priv_info);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	hardware = hw_ip->hardware;
	hw_fro_en = (hw_ip->num_buffers) > 1 ? true : false;

	for (out_id = MCSC_OUTPUT0; out_id < cap->max_output; out_id++) {
		use_sub_flip = false;
		if ((cap->out_dma[out_id] != MCSC_CAP_SUPPORT)
			|| !test_bit(out_id, &hw_mcsc->out_en))
			continue;
#ifdef MCSC_FLIP_WA
		target_out_id = hw_mcsc->flip_output_id[out_id];
		output = &mcs_param->output[target_out_id];
		if ((output->flip & 1) && output->width > MCSC_LINE_BUF_SIZE
			&& !mcs_param->stripe_input.total_count) {
			output->full_output_width = output->width;
			if (target_out_id == out_id) {
				output->stripe_roi_end_pos_x = MCSC_LINE_BUF_SIZE;
				otcrop_x = 0;
				otcrop_w = MCSC_LINE_BUF_SIZE;
			} else {
				use_sub_flip = true;
				output->stripe_roi_end_pos_x = output->width;
				otcrop_x = MCSC_LINE_BUF_SIZE;
				otcrop_w = output->width - MCSC_LINE_BUF_SIZE;
			}
			is_scaler_set_poly_out_crop_enable(hw_ip->regs[REG_SETA], out_id, 1);
			is_scaler_set_poly_out_crop_size(hw_ip->regs[REG_SETA], out_id, otcrop_x, 0, otcrop_w, output->height);
			is_scaler_set_wdma_size(hw_ip->regs[REG_SETA], out_id, otcrop_w, output->height);
			msdbg_hw(2, "[OUT:%d] wdma_cfg: %s flip use outcrop(output : %dx%d -> output crop : %d, %d, %dx%d)\n",
				frame->instance, hw_ip, out_id,
				(target_out_id == out_id ? "main" : "sub"),
				output->width, output->height,
				otcrop_x, 0, otcrop_w, output->height);
		}
#else
		target_out_id = out_id;
		output = &mcs_param->output[target_out_id];
#endif
		src_dva = hw_mcsc_get_target_addr(target_out_id, frame);
		if (!src_dva) {
			mserr_hw("[OUT%d][F%d] Failed to get src_dva", frame->instance, hw_ip,
					target_out_id, frame->fcount);
			continue;
		}

		msdbg_hw(2, "[F:%d]wdma_cfg [T:%d][addr%d: %llx]\n", frame->instance, hw_ip,
			frame->fcount, frame->type, out_id, src_dva[0]);

		if (use_sub_flip || (mcs_param->output[out_id].dma_cmd != DMA_OUTPUT_COMMAND_DISABLE
			&& src_dva && src_dva[0]
			&& frame->type != SHOT_TYPE_INTERNAL)) {
			msdbg_hw(2, "[OUT:%d]dma_out enabled\n", frame->instance, hw_ip, out_id);

			plane = output->plane;
#ifdef USE_MCSC_STRIP_OUT_CROP
			stripe_dma_offset = 0;
			roi_x = 0;
			if (is_scaler_get_poly_out_crop_enable(hw_ip->regs[REG_SETA], out_id)
			|| is_scaler_get_post_out_crop_enable(hw_ip->regs[REG_SETA], out_id)) {
				if (output->flip & 1) /* X flip */
					roi_x = output->full_output_width - output->stripe_roi_end_pos_x;
				else
					roi_x = output->stripe_roi_start_pos_x;
			}
#endif

			for (buf_i = 0; buf_i < frame->num_buffers; buf_i++) {
				src_dva_i = (frame->cur_buf_index + buf_i)
					* plane;

				for (plane_i = 0; plane_i < plane; plane_i++) {
					src_p_i = src_dva_i + plane_i;

					/*
					 * offset_x and offset_y are initialized to non-zero values
					 * when supporting bypass offset for hifi solution.
					 */
					if (output->offset_x || output->offset_y)
						offset =
						is_hw_mcsc_calculate_wdma_offset(
							hw_ip, frame, mcs_param,
							plane_i, out_id);

					/*
					 * If the number of buffers is not same between leader and subdev,
					 * wdma addresses are forcibly set as the same address of first buffer.
					 */
					if (!hw_fro_en && !src_dva[src_p_i]) {
						en = false;
						break;
					}
					en = true;

					dst_dva[plane_i] = src_dva[src_p_i] ?
						src_dva[src_p_i] :
						src_dva[plane_i];
#ifdef USE_MCSC_STRIP_OUT_CROP
					stripe_dma_offset = is_scaler_g_dma_offset(output, roi_x, plane_i);
					dst_dva[plane_i] += stripe_dma_offset;
					dbg_hw(2, "M%dP(i:%d)(offset:%d:%d)\n",
						out_id, src_p_i,
						stripe_dma_offset, offset);
#endif
					dst_dva[plane_i] += offset;
					dbg_hw(2, "M%dP(i:%d)(A:%pad)\n",
						out_id, src_p_i,
						&dst_dva[plane_i]);
				}

				hw_mcsc_set_wdma_addr(hw_ip, dst_dva,
						out_id, plane, buf_i);
			}
			is_scaler_set_dma_out_enable(hw_ip->regs[REG_SETA], out_id, en);

		} else {
			is_hw_mcsc_wdma_clear(hw_ip, frame, mcs_param, out_id);
		}
	}
}

void hw_mcsc_find_ni_idx(struct is_hw_ip *hw_ip,
	u32 *ni, u32 ni_max, u32 cur_ni, struct ref_ni *ret_idx)
{
	struct ref_ni ni_range;
	u32 ni_idx;

	ni_range.min = MULTIPLIED_10(ni[0]);
	ni_range.max = MULTIPLIED_10(ni[ni_max - 1]);
	if (cur_ni < ni_range.min) {
		sdbg_hw(2, "[HF] cur_ni(%d) < ni_range.min(%d)\n", hw_ip, cur_ni, ni_range.min);
		ret_idx->min = 0;
		ret_idx->max = 0;
	} else if (cur_ni > ni_range.max) {
		sdbg_hw(2, "[HF] cur_ni(%d) > ni_range.max(%d)\n", hw_ip, cur_ni, ni_range.max);
		ret_idx->min = ni_max - 1;
		ret_idx->max = ni_max - 1;
	} else {
		for (ni_idx = 0; ni_idx < ni_max - 1; ni_idx++) {
			ni_range.min = MULTIPLIED_10(ni[ni_idx]);
			ni_range.max = MULTIPLIED_10(ni[ni_idx + 1]);

			if (ni_range.min < cur_ni && cur_ni < ni_range.max) {
				ret_idx->min = ni_idx;
				ret_idx->max = ni_idx + 1;
				break;
			} else if (cur_ni == ni_range.min) {
				ret_idx->min = ni_idx;
				ret_idx->max = ni_idx;
				break;
			} else if (cur_ni == ni_range.max) {
				ret_idx->min = ni_idx + 1;
				ret_idx->max = ni_idx + 1;
				break;
			}
		}
	}

	sdbg_hw(2, "[HF] find_ni_idx: cur_ni %d idx %d,%d range %d,%d\n", hw_ip, cur_ni,
		ret_idx->min, ret_idx->max,
		MULTIPLIED_10(ni[ret_idx->min]), MULTIPLIED_10(ni[ret_idx->max]));
}

void hw_mcsc_calc_hf_param_by_ni(struct is_hw_ip *hw_ip,
	struct hf_setfile_contents *hf, u32 cur_ni, struct hf_cfg_by_ni *hf_cfg)
{
	struct ref_ni ni_idx, ni;
	struct hf_cfg_by_ni cfg_max, cfg_min;

	hw_mcsc_find_ni_idx(hw_ip, hf->ni, hf->ni_max, cur_ni, &ni_idx);

	cfg_min = hf->cfgs[ni_idx.min];
	cfg_max = hf->cfgs[ni_idx.max];
	ni.min = MULTIPLIED_10(hf->ni[ni_idx.min]);
	ni.max = MULTIPLIED_10(hf->ni[ni_idx.max]);

	hf_cfg->hf_weight = GET_LNR_INTRPL(
			cfg_min.hf_weight, cfg_max.hf_weight,
			ni.min, ni.max, cur_ni);
}

int is_hw_mcsc_update_hf_register(struct is_hw_ip *hw_ip,
	struct mcs_param *param, struct is_frame *frame, u32 instance)
{
	int ret = 0;
	struct is_hw_mcsc *hw_mcsc;
	struct param_mcs_output *hf_param = &param->output[MCSC_INPUT_HF];
	enum exynos_sensor_position sensor_position;
	struct hw_mcsc_setfile *setfile;
	struct scaler_coef_cfg *sc_coef;
	struct hf_setfile_contents *hf_setfile;
	struct hf_cfg_by_ni hf_cfg;
	dma_addr_t *rdma_addr, dva, hdr_dva;
	u32 b, payload_size = 0;
	u32 djag_stripe_pre_dst_x = 0, djag_stripe_start_pos_x;
	u32 ni, core_num, ni_index;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);
	FIMC_BUG(!frame);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];
	hf_setfile = &setfile->hf;
	sc_coef = &setfile->sc_coef;

	if (param->output[MCSC_OUTPUT5].dma_cmd == DMA_OUTPUT_VOTF_ENABLE)
		is_scaler_set_votf_config(hw_ip->regs[REG_SETA], hw_ip->id, true);

	rdma_addr = hw_mcsc_get_target_addr(MCSC_INPUT_HF, frame);

	if (hf_param->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE || !rdma_addr[0]) {
		is_scaler_set_rdma_enable(hw_ip->regs[REG_SETA], hw_ip->id, false);
		is_scaler_set_djag_hf_enable(hw_ip->regs[REG_SETA], false, false);
		msdbg_hw(2, "[F:%d]high frequency: hf disabled, hf_param->dma_cmd(%d), rdma_addr[0](%pad)\n",
			instance, hw_ip, atomic_read(&hw_ip->fcount), hf_param->dma_cmd, &rdma_addr[0]);

		if (hf_param->dma_cmd == DMA_OUTPUT_COMMAND_ENABLE && !rdma_addr[0])
			mswarn_hw("[F:%d]high frequency: addr is 0, disable DMA",
				instance, hw_ip, atomic_read(&hw_ip->fcount));

		return ret;
	}

	/* Set HF tuning parameters for noise index */
	core_num = GET_CORE_NUM(hw_ip->id);
	ni_index = frame->fcount % NI_BACKUP_MAX;
	ni = hw_ip->hardware->ni_udm[core_num][ni_index].currentFrameNoiseIndex;
	if (!hf_setfile) {
		mserr_hw("[F:%d]high frequency: hf setfile is null\n",
				instance, hw_ip, frame->fcount);
	} else{
		if (hw_mcsc->cur_ni[SUBBLK_HF] != ni) {
			hw_mcsc_calc_hf_param_by_ni(hw_ip, hf_setfile, ni, &hf_cfg);
			is_scaler_set_djag_hf_cfg(hw_ip->regs[REG_SETA], &hf_cfg);
			msdbg_hw(2, "[F:%d]high frequency: ni(%d)\n",
				instance, hw_ip, frame->fcount, ni);
		}
	}
	hw_mcsc->cur_ni[SUBBLK_HF] = ni;

	ret = is_scaler_set_hf_config(hw_ip->regs[REG_SETA], hw_ip->id,
		true, hf_param,
		sc_coef, sensor_position, &payload_size);
	if (ret) {
		mserr_hw("[F:%d]high frequency: configuration failed, hf disabled\n",
				instance, hw_ip, frame->fcount);
		return ret;
	}

	msdbg_hw(2, "[F:%d]high frequency: hf enabled\n",
		instance, hw_ip, atomic_read(&hw_ip->fcount));

	/* FIX-ME: djag_stripe_pre_dst_x is no meaning */
	is_scaler_get_djag_strip_config(hw_ip->regs[REG_SETA], 0, &djag_stripe_pre_dst_x, &djag_stripe_start_pos_x);

	for (b = 0; b < frame->num_buffers; b++) {
		dva = rdma_addr[b] + djag_stripe_pre_dst_x;
		hdr_dva = dva + payload_size;
		is_scaler_set_rdma_addr(hw_ip->regs[REG_SETA], dva, 0, 0, b);
		is_scaler_set_rdma_sbwc_header_addr(hw_ip->regs[REG_SETA], hdr_dva, b);

		msdbg_hw(2, "[F:%d]high frequency: rdma_cfg [T:%d][addr:%pad][haddr:%pad], stripe_start_pos_x(%d)\n",
					frame->instance, hw_ip, frame->fcount, frame->type,
					&dva, &hdr_dva, djag_stripe_pre_dst_x);
	}
	return ret;
}

static int is_hw_mcsc_shot(struct is_hw_ip *hw_ip, struct is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	int ret_internal = 0;
	struct is_hardware *hardware;
	struct is_hw_mcsc *hw_mcsc;
	struct is_param_region *param_region;
	struct mcs_param *mcs_param;
	u32 instance;
	bool hw_fro_en;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	hardware = hw_ip->hardware;
	instance = frame->instance;

	msdbgs_hw(2, "[F:%d]shot\n", instance, hw_ip, frame->fcount);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		msdbg_hw(2, "not initialized\n", instance, hw_ip);
		return -EINVAL;
	}

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	param_region = frame->parameter;
	mcs_param = &param_region->mcs;

	if (mcs_param->control.cmd == CONTROL_COMMAND_STOP) {
		msdbg_hw(2, "Stop MCSC\n", instance, hw_ip);
		return 0;
	}

	if (frame->type == SHOT_TYPE_INTERNAL) {
		msdbg_hw(2, "request not exist\n", instance, hw_ip);
		goto config;
	}

	is_hw_mcsc_update_param(hw_ip, mcs_param, instance);

	msdbg_hw(2, "[F:%d]shot [T:%d]\n", instance, hw_ip, frame->fcount, frame->type);

config:
	/* multi-buffer */
	hw_ip->num_buffers = frame->num_buffers;
	hw_fro_en = (hw_ip->num_buffers) > 1 ? true : false;

	/* RDMA cfg */
	ret = is_hw_mcsc_rdma_cfg(hw_ip, frame, &mcs_param->input);
	if (ret) {
		mserr_hw("[F:%d]mcsc rdma_cfg failed\n",
				instance, hw_ip, frame->fcount);
		return ret;
	}

	/* WDMA cfg */
	is_hw_mcsc_wdma_cfg(hw_ip, frame, mcs_param);
	is_scaler_set_lfro_mode_enable(hw_ip->regs[REG_SETA], hw_ip->id, hw_fro_en, frame->num_buffers);

	ret_internal = is_hw_mcsc_update_cac_register(hw_ip, frame, instance);
	ret_internal = is_hw_mcsc_update_hf_register(hw_ip, mcs_param, frame, instance);

	if (IS_ENABLED(SOC_ITSC))
		is_itsc_start(hw_ip->regs[REG_EXT1], hw_ip->id);

	CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, frame->fcount, DEBUG_POINT_ADD_TO_CMDQ);
	is_scaler_start(hw_ip->regs[REG_SETA], hw_ip->id);

	msdbg_hw(2, "shot\n", instance, hw_ip);

	if (unlikely(test_bit(MCSC_DBG_DUMP_REG, &debug_mcsc)
		|| test_bit(MCSC_DBG_DUMP_REG_ONCE, &debug_mcsc))) {
		is_scaler_dump(hw_ip->regs[REG_SETA]);
		if (test_bit(MCSC_DBG_DUMP_REG_ONCE, &debug_mcsc))
			clear_bit(MCSC_DBG_DUMP_REG_ONCE, &debug_mcsc);
	}

	/* FIXME: Remove memcpy for sync of strip parameter */
	memcpy(&hw_ip->region[instance]->parameter.mcs.stripe_input, &mcs_param->stripe_input,
		sizeof(struct param_stripe_input));

	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int is_hw_mcsc_set_param(struct is_hw_ip *hw_ip, struct is_region *region,
		IS_DECLARE_PMAP(pmap), u32 instance, ulong hw_map)
{
	int ret = 0;
	struct is_hw_mcsc *hw_mcsc;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!region);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	hw_ip->region[instance] = region;

	/* To execute update_register function in hw_mcsc_shot(),
	 * the value of hw_mcsc->instance is set as default.
	 */
	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	hw_mcsc->instance = IS_STREAM_COUNT;

	return ret;
}

void is_hw_mcsc_check_size(struct is_hw_ip *hw_ip,
	struct param_mcs_input *input, struct param_mcs_output *output,
	u32 instance, u32 output_id)
{
	minfo_hw("[OUT:%d]>>> hw_mcsc_check_size >>>\n", instance, output_id);
	info_hw("otf_input: format(%d),size(%dx%d)\n",
		input->otf_format, input->width, input->height);
	info_hw("dma_input: format(%d),crop_size(%dx%d)\n",
		input->dma_format, input->dma_crop_width, input->dma_crop_height);
	info_hw("output: otf_cmd(%d),dma_cmd(%d),format(%d),stride(y:%d,c:%d)\n",
		output->otf_cmd, output->dma_cmd, output->dma_format,
		output->dma_stride_y, output->dma_stride_c);
	info_hw("output: pos(%d,%d),crop%dx%d),size(%dx%d)\n",
		output->crop_offset_x, output->crop_offset_y,
		output->crop_width, output->crop_height,
		output->width, output->height);
	info_hw("[%d]<<< hw_mcsc_check_size <<<\n", instance);
}

int is_hw_mcsc_update_register(struct is_hw_ip *hw_ip,
	struct param_mcs_input *input, struct param_mcs_output *output,
	struct param_stripe_input *stripe_input,
	u32 output_id, u32 instance)
{
	int ret = 0;

	hw_mcsc_check_size(hw_ip, input, output, instance, output_id);
	ret = is_hw_mcsc_poly_phase(hw_ip, input,
			output, stripe_input, output_id, instance);
	ret = is_hw_mcsc_post_chain(hw_ip, input,
			output, stripe_input, output_id, instance);
	ret = is_hw_mcsc_flip(hw_ip, output, output_id, instance);
	ret = is_hw_mcsc_dma_output(hw_ip, output, output_id, instance);
	ret = is_hw_mcsc_output_yuvrange(hw_ip, output, output_id, instance);
	ret = is_hw_mcsc_hwfc_output(hw_ip, output, output_id, instance);

	return ret;
}

int is_hw_mcsc_set_flip_output(struct is_hw_ip *hw_ip,
	struct mcs_param *param, u32 instance) {
	int i;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_output output_node[MCSC_OUTPUT_MAX];
	struct is_hw_mcsc_output *sub_output_node, *main_output_node;
	struct list_head main_flip_list, sub_flip_list;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);
	FIMC_BUG(!cap);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	/* Flip SW W/A */
	INIT_LIST_HEAD(&main_flip_list);
	INIT_LIST_HEAD(&sub_flip_list);
	for (i = MCSC_OUTPUT0; i < cap->max_output; i++) {
		hw_mcsc->flip_output_id[i] = i;
		output_node[i].output_id = i;
		INIT_LIST_HEAD(&output_node[i].node);
		if (param->output[i].dma_cmd == DMA_OUTPUT_COMMAND_ENABLE) {
			if ((param->output[i].flip & 1) && param->output[i].width > MCSC_LINE_BUF_SIZE)
				list_add_tail(&output_node[i].node, &main_flip_list);
		} else {
			list_add_tail(&output_node[i].node, &sub_flip_list);
		}
	}
	list_for_each_entry(main_output_node, &main_flip_list, node) {
		if (list_empty(&sub_flip_list) || param->stripe_input.total_count) {
			mserr_hw("set_flip_output: cannot flip an image that exceeds %d size due to insufficient number of ports or stripe(%d)",
				instance, hw_ip,
				MCSC_LINE_BUF_SIZE, param->stripe_input.total_count);
			return -EINVAL;
		}
		sub_output_node = list_first_entry(&sub_flip_list, struct is_hw_mcsc_output, node);
		hw_mcsc->flip_output_id[sub_output_node->output_id] = main_output_node->output_id;
		msdbg_hw(2, "set_flip_output: main flip output(%d), sub flip output(%d)\n", instance, hw_ip,
			main_output_node->output_id, sub_output_node->output_id);
		list_del(&sub_output_node->node);
	}
	return 0;
}

int is_hw_mcsc_update_param(struct is_hw_ip *hw_ip,
	struct mcs_param *param, u32 instance)
{
	int i = 0;
	int ret = 0;
	u32 out_id;
	u32 dma_output_ids = 0;
	u32 hwfc_output_ids = 0;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);
	FIMC_BUG(!cap);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	if (hw_mcsc->instance != instance) {
		msdbg_hw(2, "update_param: hw_ip->instance(%d)\n",
			instance, hw_ip, hw_mcsc->instance);
		hw_mcsc->instance = instance;
	}

	ret |= is_hw_mcsc_otf_input(hw_ip, &param->input, instance);
	ret |= is_hw_mcsc_dma_input(hw_ip, &param->input, instance);

	if (!IS_ENABLED(SKIP_SETFILE_LOAD) && !test_bit(MCSC_DBG_SKIP_SETFILE, &debug_mcsc))
		is_hw_mcsc_update_djag_register(hw_ip, param, instance);	/* for DZoom */

#ifdef MCSC_FLIP_WA
	is_hw_mcsc_set_flip_output(hw_ip, param, instance);
#endif
	for (i = MCSC_OUTPUT0; i < cap->max_output; i++) {
#ifdef MCSC_FLIP_WA
		out_id = hw_mcsc->flip_output_id[i];
#else
		out_id = i;
#endif
		ret |= is_hw_mcsc_update_register(hw_ip, &param->input, &param->output[out_id],
					&param->stripe_input, i, instance);

		/* check the hwfc enable in all output */
		if (param->output[i].hwfc)
			hwfc_output_ids |= (1 << i);

		if (param->output[i].dma_cmd == DMA_OUTPUT_COMMAND_ENABLE)
			dma_output_ids |= (1 << i);
	}

	/* setting for hwfc */
	ret |= is_hw_mcsc_hwfc_mode(hw_ip, &param->input, hwfc_output_ids, dma_output_ids, instance);

	hw_mcsc->prev_hwfc_output_ids = hwfc_output_ids;

	if (ret)
		is_hw_mcsc_size_dump(hw_ip);

	return ret;
}
KUNIT_EXPORT_SYMBOL(is_hw_mcsc_update_param);

int is_hw_mcsc_reset(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	u32 output_id;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!cap);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	if (hw_ip) {
		msinfo_hw("hw_mcsc_reset: out_en[0x%lx]\n", instance, hw_ip, hw_mcsc->out_en);
		if (IS_ENABLED(SOC_ITSC)) {
			ret = is_itsc_sw_reset(hw_ip->regs[REG_EXT1], 0);
			if (ret != 0) {
				mserr_hw("ITSC sw reset fail", instance, hw_ip);
				return -ENODEV;
			}
		}

		ret = is_scaler_sw_reset(hw_ip->regs[REG_SETA], hw_ip->id, 0, 0);
		if (ret != 0) {
			mserr_hw("sw reset fail", instance, hw_ip);
			return -ENODEV;
		}

		/* shadow ctrl register clear */
		is_scaler_clear_shadow_ctrl(hw_ip->regs[REG_SETA], hw_ip->id);
	}

	is_scaler_set_rdma_enable(hw_ip->regs[REG_SETA], hw_ip->id, false);
	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		msinfo_hw("[OUT:%d]set output clear\n", instance, hw_ip, output_id);
		is_scaler_set_poly_scaler_enable(hw_ip->regs[REG_SETA], hw_ip->id, output_id, 0);
		is_scaler_set_post_scaler_enable(hw_ip->regs[REG_SETA], output_id, 0);
		is_scaler_set_dma_out_enable(hw_ip->regs[REG_SETA], output_id, false);
	}

	return ret;
}

int is_hw_mcsc_clear_interrupt(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	msinfo_hw("hw_mcsc_clear_interrupt\n", instance, hw_ip);

	is_scaler_clear_intr_all(hw_ip->regs[REG_SETA], hw_ip->id);
	is_scaler_disable_intr(hw_ip->regs[REG_SETA], hw_ip->id);
	is_scaler_mask_intr(hw_ip->regs[REG_SETA], hw_ip->id,
		is_scaler_get_mask_intr());

	return ret;
}

static int is_hw_mcsc_load_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	struct is_hw_ip_setfile *setfile;
	struct hw_mcsc_setfile *setfile_addr;
	enum exynos_sensor_position sensor_position;
	struct is_hw_mcsc *hw_mcsc = NULL;
	int setfile_index = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		mserr_hw("priv_info is NULL", instance, hw_ip);
		return -EINVAL;
	}
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	switch (setfile->version) {
	case SETFILE_V2:
		break;
	case SETFILE_V3:
		break;
	default:
		mserr_hw("invalid version (%d)", instance, hw_ip,
			setfile->version);
		return -EINVAL;
	}

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	if (setfile->using_count > IS_MAX_MCSC_SETFILE) {
		serr_hw("too many setfile entries: %d", hw_ip, setfile->using_count);
		return -EINVAL;
	}

	if (setfile->table[0].size != sizeof(struct hw_mcsc_setfile))
		mswarn_hw("tuneset size(%x) is not matched to setfile structure size(%lx)",
			instance, hw_ip, setfile->table[0].size,
			sizeof(struct hw_mcsc_setfile));

	/* copy MCSC setfile set */
	setfile_addr = (struct hw_mcsc_setfile *)setfile->table[0].addr;
	memcpy(hw_mcsc->setfile[sensor_position], setfile_addr,
		sizeof(struct hw_mcsc_setfile) * setfile->using_count);

	/* check each setfile Magic numbers */
	for (setfile_index = 0; setfile_index < setfile->using_count; setfile_index++) {
		setfile_addr = &hw_mcsc->setfile[sensor_position][setfile_index];

		if (setfile_addr->setfile_version != MCSC_SETFILE_VERSION) {
			mserr_hw("setfile[%d] version(0x%x) is incorrect",
				instance, hw_ip, setfile_index,
				setfile_addr->setfile_version);
			return -EINVAL;
		}
	}
	hw_mcsc->tune_set[instance] = true;

	return ret;
}

static int is_hw_mcsc_apply_setfile(struct is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	int ret = 0;
	struct is_hw_mcsc *hw_mcsc = NULL;
	struct is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	u32 setfile_index = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		mserr_hw("priv info is NULL", instance, hw_ip);
		return -EINVAL;
	}

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	setfile_index = setfile->index[scenario];
	if (setfile_index >= setfile->using_count) {
		mserr_hw("setfile index is out-of-range, [%d:%d]",
				instance, hw_ip, scenario, setfile_index);
		return -EINVAL;
	}

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	hw_mcsc->cur_setfile[sensor_position][instance] =
		&hw_mcsc->setfile[sensor_position][setfile_index];

	msinfo_hw("setfile (%d/%d) scenario (%d)\n", instance, hw_ip, setfile_index,
		setfile->using_count, scenario);

	return ret;
}

static int is_hw_mcsc_delete_setfile(struct is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	struct is_hw_mcsc *hw_mcsc;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	if (hw_mcsc->tune_set[instance] == false) {
		msdbg_hw(2, "setfile already deleted", instance, hw_ip);
		return 0;
	}

	hw_mcsc->tune_set[instance] = false;

	return 0;
}

static int is_hw_mcsc_frame_ndone(struct is_hw_ip *hw_ip, struct is_frame *frame,
		enum ShotErrorType done_type)
{
	return CALL_HW_OPS(hw_ip, frame_done, hw_ip, frame, -1,
				IS_HW_CORE_END, done_type, false);
}

int is_hw_mcsc_otf_input(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance)
{
	int ret = 0;
	u32 width, height;
	u32 format, bit_width;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!cap);

	/* can't support this function */
	if (cap->in_otf != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	msdbg_hw(2, "otf_input_setting cmd(%d)\n", instance, hw_ip, input->otf_cmd);
	width  = input->width;
	height = input->height;
	format = input->otf_format;
	bit_width = input->otf_bitwidth;

	if (input->otf_cmd == OTF_INPUT_COMMAND_DISABLE)
		return ret;

	ret = is_hw_mcsc_check_format(HW_MCSC_OTF_INPUT,
		format, bit_width, width, height);
	if (ret) {
		mserr_hw("Invalid OTF Input format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, format, bit_width, width, height);
		return ret;
	}

	if (unlikely(test_bit(MCSC_DBG_STRGEN, &debug_mcsc)))
		is_scaler_set_input_source(hw_ip->regs[REG_SETA], hw_ip->id, input->otf_cmd, true);
	else
		is_scaler_set_input_source(hw_ip->regs[REG_SETA], hw_ip->id, input->otf_cmd, false);

	if (IS_ENABLED(SOC_ITSC))
		is_itsc_set_input_img_size(hw_ip->regs[REG_EXT1], hw_ip->id,
				width, height);

	is_scaler_set_input_img_size(hw_ip->regs[REG_SETA], hw_ip->id, width, height);
	is_scaler_set_mono_ctrl(hw_ip->regs[REG_SETA], hw_ip->id, format);

	return ret;
}

int is_hw_mcsc_dma_input(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance)
{
	int ret = 0;
	u32 width, height;
	u32 format, bit_width, plane, order;
	u32 y_stride, uv_stride;
	u32 img_format;
	u32 y_2bit_stride = 0, uv_2bit_stride = 0;
	u32 img_10bit_type = 0;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!cap);

	/* can't support this function */
	if (cap->in_dma != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	msdbg_hw(2, "dma_input_setting cmd(%d)\n", instance, hw_ip, input->dma_cmd);
	width  = input->dma_crop_width;
	height = input->dma_crop_height;
	format = input->dma_format;
	bit_width = input->dma_bitwidth;
	plane = input->plane;
	order = input->dma_order;
	y_stride = input->dma_stride_y;
	uv_stride = input->dma_stride_c;

	if (input->dma_cmd == DMA_INPUT_COMMAND_DISABLE) {
		is_scaler_set_rdma_enable(hw_ip->regs[REG_SETA], hw_ip->id, false);
		return ret;
	}

	ret = is_hw_mcsc_check_format(HW_MCSC_DMA_INPUT,
		format, bit_width, width, height);
	if (ret) {
		mserr_hw("Invalid DMA Input format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, format, bit_width, width, height);
		return ret;
	}

	is_scaler_set_input_source(hw_ip->regs[REG_SETA], hw_ip->id, input->dma_cmd + 1, false);
	if (IS_ENABLED(SOC_ITSC))
		is_itsc_set_input_img_size(hw_ip->regs[REG_EXT1], hw_ip->id,
				width, height);

	is_scaler_set_input_img_size(hw_ip->regs[REG_SETA], hw_ip->id, width, height);
	is_scaler_set_rdma_stride(hw_ip->regs[REG_SETA], y_stride, uv_stride);
	ret = is_hw_mcsc_adjust_input_img_fmt(format, plane, order, &img_format);

	if (ret < 0) {
		mswarn_hw("Invalid rdma image format", instance, hw_ip);
		img_format = hw_mcsc->in_img_format;
	} else {
		hw_mcsc->in_img_format = img_format;
	}

	is_scaler_set_rdma_size(hw_ip->regs[REG_SETA], width, height);
	is_scaler_set_rdma_format(hw_ip->regs[REG_SETA], hw_ip->id, img_format);

	if (bit_width == DMA_INOUT_BIT_WIDTH_16BIT)
		img_10bit_type = 2;
	else if (bit_width == DMA_INOUT_BIT_WIDTH_10BIT)
		if (plane == DMA_INOUT_PLANE_4)
			img_10bit_type = 1;
		else
			img_10bit_type = 3;
	else
		img_10bit_type = 0;
	is_scaler_set_rdma_10bit_type(hw_ip->regs[REG_SETA], img_10bit_type);

	if (input->plane == DMA_INOUT_PLANE_4) {
		y_2bit_stride = ALIGN(width / 4, 16);
		uv_2bit_stride = ALIGN(width / 4, 16);
		is_scaler_set_rdma_2bit_stride(hw_ip->regs[REG_SETA], y_2bit_stride, uv_2bit_stride);
	}

	return ret;
}
#ifdef USE_MCSC_STRIP_OUT_CROP
int is_hw_mcsc_calc_stripe_position(struct is_hw_ip *hw_ip, u32 instance, u32 output_id,
		struct param_mcs_input *input, struct param_mcs_output *output,
		struct param_stripe_input *stripe_input,
		u32 *x, u32 *width)
{
	u32 full_crop_offset_x, full_crop_width;
	/* Stripe position of Poly input */
	u32 input_in_start_pos_x = input->stripe_in_start_pos_x;
	u32 input_roi_start_pos_x = input->stripe_roi_start_pos_x;
	u32 input_roi_end_pos_x = input->stripe_roi_end_pos_x;
	u32 input_in_end_pos_x = input->stripe_in_end_pos_x;
	/* Stripe position of Poly input crop */
	u32 out_in_start_pos_x;
	u32 out_roi_start_pos_x, out_roi_end_pos_x;
	struct is_hw_mcsc *hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	hw_mcsc->stripe_region = MCSC_STRIPE_REGION_M;
	/* Do stripe processing only for horizontal axis*/
	full_crop_offset_x = *x;
	full_crop_width = *width;
	output->full_input_width = *width;

	/* Mcscaler bypassed when cropped stripe region is not included in input stripe region */
	if (full_crop_offset_x >= input_roi_end_pos_x ||
		full_crop_offset_x + full_crop_width <= input_roi_start_pos_x) {
		output->otf_cmd = OTF_OUTPUT_COMMAND_DISABLE;
		output->dma_cmd = DMA_OUTPUT_COMMAND_DISABLE;
		msdbg_hw(3, "[OUT:%d] Skip current stripe[#%d] region because cropped region is not included in input region\n",
			instance, hw_ip,
			output_id, stripe_input->index);
		return -EAGAIN;
	}

	/* Stripe x when crop offset x starts in input region */
	if (full_crop_offset_x > input_in_start_pos_x  &&
		full_crop_offset_x < input_in_end_pos_x) {
		*x = full_crop_offset_x - input_in_start_pos_x;
		out_in_start_pos_x = full_crop_offset_x;
		if (full_crop_offset_x >= input_roi_start_pos_x  &&
			full_crop_offset_x < input_roi_end_pos_x) {
				hw_mcsc->stripe_region = MCSC_STRIPE_REGION_L;
				out_roi_start_pos_x = full_crop_offset_x;
		} else
			out_roi_start_pos_x = input_roi_start_pos_x;
	} else {
		*x = 0;
		out_in_start_pos_x = input_in_start_pos_x;
		out_roi_start_pos_x = input_roi_start_pos_x;
	}

	/* Stripe width when crop region ends in input region */
	if (full_crop_offset_x + full_crop_width > input_in_start_pos_x  &&
		full_crop_offset_x + full_crop_width < input_in_end_pos_x) {
		*width = full_crop_offset_x + full_crop_width - out_in_start_pos_x;
		if (full_crop_offset_x + full_crop_width > input_roi_start_pos_x  &&
			full_crop_offset_x + full_crop_width <= input_roi_end_pos_x) {
				hw_mcsc->stripe_region = MCSC_STRIPE_REGION_R;
				out_roi_end_pos_x = full_crop_offset_x + full_crop_width;
			}
		else
			out_roi_end_pos_x = input_roi_end_pos_x;
	} else {
		*width = input_in_end_pos_x - out_in_start_pos_x;
		out_roi_end_pos_x = input_roi_end_pos_x;
	}
	output->stripe_roi_start_pos_x = out_roi_start_pos_x - full_crop_offset_x;
	output->stripe_roi_end_pos_x = out_roi_end_pos_x - full_crop_offset_x;
	output->stripe_in_start_pos_x = out_in_start_pos_x - full_crop_offset_x;
	msdbg_hw(2, "[OUT:%d]poly_phase: full crop offset_x(%d) width(%d) -> stripe crop offset_x(%d) width(%d)\n",
			instance, hw_ip, output_id, full_crop_offset_x, full_crop_width, *x, *width);

	return 0;
}
#endif
void is_hw_mcsc_calc_poly_dst_size(struct is_hw_ip *hw_ip, u32 instance,
	u32 src_length, u32 dst_length, u32 *poly_dst_length,
	u32 poly_ratio_down, bool check_dst_size, u32 align, bool *post_en)
{
	u32 temp_dst_length = 0;
	char is_type_length[2] = {'h','w'};
	/*
	 * w/ POST - x1/MCSC_POLY_QUALITY_RATIO_DOWN <= POLY RATIO <= xMCSC_POLY_RATIO_UP
	 * w/o POST - x1/MCSC_POLY_MAX_RATIO_DOWN <= POLY RATIO <= xMCSC_POLY_RATIO_UP
	 */
	if ((src_length <= (dst_length * poly_ratio_down))
		&& (dst_length <= (src_length * MCSC_POLY_RATIO_UP))) {
		temp_dst_length = dst_length;
		*post_en = false;
	}
	/*
	 * POLY RATIO = x1/MCSC_POLY_QUALITY_RATIO_DOWN
	 * POST RATIO = ~ x1/MCSC_POST_RATIO_DOWN
	 */
	else if ((src_length <= (dst_length * MCSC_POLY_QUALITY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((dst_length * MCSC_POLY_QUALITY_RATIO_DOWN) < src_length)) {
		temp_dst_length = MCSC_ROUND_UP(src_length / MCSC_POLY_QUALITY_RATIO_DOWN, align);
		*post_en = true;
	}
	/*
	 * POLY RATIO = x1/MCSC_POLY_RATIO_DOWN ~ x1/MCSC_POLY_QUALITY_RATIO_DOWN
	 * POST RATIO = x1/MCSC_POST_RATIO_DOWN
	 */
	else if ((src_length <= (dst_length * MCSC_POLY_RATIO_DOWN * MCSC_POST_RATIO_DOWN))
		&& ((dst_length * MCSC_POLY_QUALITY_RATIO_DOWN * MCSC_POST_RATIO_DOWN) < src_length)) {
		temp_dst_length = dst_length * MCSC_POST_RATIO_DOWN;
		*post_en = true;
	}
	/* POLY RATIO = x1/MCSC_POLY_RATIO_DOWN */
	else {
		mserr_hw("hw_mcsc_poly_phase: Unsupported ratio, (%d)->(%d) w or h?(%c)\n",
			instance, hw_ip, src_length, dst_length, is_type_length[check_dst_size]);
		temp_dst_length = MCSC_ROUND_UP(src_length / MCSC_POLY_RATIO_DOWN, align);
		*post_en = true;
	}

	/* line buffer size check */
	if (check_dst_size && *post_en) {
		if (dst_length > MCSC_POST_MAX_WIDTH) {
			msinfo_hw("%s: do not use pos_chain, dst_width(%d) > MCSC_POST_MAX_WIDTH(%d)\n",
					instance, hw_ip, __func__, dst_length,
					MCSC_POST_MAX_WIDTH);
			temp_dst_length = dst_length;
			*post_en = false;
		} else if (temp_dst_length > MCSC_POST_MAX_WIDTH) {
			temp_dst_length = MCSC_POST_MAX_WIDTH;
		}
	}

	*poly_dst_length = temp_dst_length;
}

int is_hw_mcsc_poly_phase(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, struct param_stripe_input *stripe_input, u32 output_id, u32 instance)
{
	int ret = 0;
	u32 crop_type, src_pos_x, src_pos_y, src_width, src_height;
	u32 poly_dst_width, poly_dst_height;
	u32 out_width, out_height;
	ulong temp_width, temp_height;
	u32 hratio, vratio;
	u32 input_id = 0;
	u32 poly_ratio_down = MCSC_POLY_QUALITY_RATIO_DOWN;
	u32 stripe_align;
	bool post_en = false;
	bool round_mode_en = true;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	enum exynos_sensor_position sensor_position;
	struct hw_mcsc_setfile *setfile;
	struct scaler_coef_cfg *sc_coefs[2];
#ifdef USE_MCSC_STRIP_OUT_CROP
	int otcrop_pos_x = 0, otcrop_width = 0;
	u32 otcrop_pos_y = 0, otcrop_height = 0;
	u32 full_in_width = 0, full_out_width = 0;
	u32 poly_dst_full_width = 0;
	u32 roi_start_x = 0, roi_end_x = 0, roi_start_w = 0, roi_end_w = 0;
	u32 temp_stripe_start_pos_x = 0, temp_stripe_pre_dst_x = 0;
	u32 use_out_crop = 0;
	u32 h_phase_offset = 0;
	ulong offset;
#endif

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!output);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]poly_phase_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->otf_cmd, output->dma_cmd);

	input_id = is_scaler_get_scaler_path(hw_ip->regs[REG_SETA], hw_ip->id, output_id);

#ifdef USE_MCSC_STRIP_OUT_CROP
	use_out_crop = (output->cmd & BIT(MCSC_OUT_CROP)) >> MCSC_OUT_CROP;
#endif
	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE
		&& output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		is_scaler_set_poly_scaler_enable(hw_ip->regs[REG_SETA], hw_ip->id, output_id, 0);
		return ret;
	}

	is_scaler_set_poly_scaler_enable(hw_ip->regs[REG_SETA], hw_ip->id, output_id, 1);

	crop_type = (output->cmd & BIT(MCSC_CROP_TYPE)) >> MCSC_CROP_TYPE;
	src_pos_x = output->crop_offset_x;
	src_pos_y = output->crop_offset_y;
	src_width = output->crop_width;
	src_height = output->crop_height;

	is_hw_mcsc_adjust_size_with_djag(hw_ip, input, stripe_input, cap, crop_type,
		&src_pos_x, &src_pos_y, &src_width, &src_height);
#ifdef USE_MCSC_STRIP_OUT_CROP
	if (use_out_crop) {
		ret = is_hw_mcsc_calc_stripe_position(hw_ip, instance, output_id,
			input, output, stripe_input,
			&src_pos_x, &src_width);
		if (ret) {
			is_scaler_set_poly_scaler_enable(hw_ip->regs[REG_SETA], hw_ip->id, output_id, 0);
			return 0;
		}
		full_in_width = output->full_input_width;
		full_out_width = output->full_output_width;
		roi_start_x = output->stripe_roi_start_pos_x;
		roi_end_x = output->stripe_roi_end_pos_x;
	}
#endif
	out_width = output->width;
	out_height = output->height;

	if (src_pos_x % MCSC_OFFSET_ALIGN) {
		mswarn_hw("[OUT:%d] hw_mcsc_poly_phase: src_pos_x(%d) should be multiple of %d\n",
			instance, hw_ip, output_id, src_pos_x, MCSC_OFFSET_ALIGN);
	}
	if (src_width % MCSC_WIDTH_ALIGN) {
		mswarn_hw("[OUT:%d] hw_mcsc_poly_phase: src_width(%d) should be multiple of %d\n",
			instance, hw_ip, output_id, src_width, MCSC_WIDTH_ALIGN);
	}

	is_scaler_set_poly_src_size(hw_ip->regs[REG_SETA], output_id, src_pos_x, src_pos_y,
		src_width, src_height);

	/* Allow higher ratio down of poly scaler if no post scaler in output. */
	if (cap->out_post[output_id] == MCSC_CAP_NOT_SUPPORT)
		poly_ratio_down = MCSC_POLY_MAX_RATIO_DOWN;

	temp_width  = (ulong)src_width;
	temp_height = (ulong)src_height;
#ifdef USE_MCSC_STRIP_OUT_CROP
	/* Use ratio of full size if stripe processing  */
	if (use_out_crop) {
		is_hw_mcsc_calc_poly_dst_size(hw_ip, instance, full_in_width,
			full_out_width, &poly_dst_full_width, poly_ratio_down,
			false, MCSC_WIDTH_ALIGN, &post_en);
		output->full_input_width = poly_dst_full_width;
		hratio = GET_ZOOM_RATIO(full_in_width, poly_dst_full_width);
		poly_dst_width = ALIGN_DOWN(GET_SCALED_SIZE(src_width, hratio), MCSC_WIDTH_ALIGN);

		roi_start_w = ALIGN_DOWN(GET_SCALED_SIZE(roi_start_x, hratio), MCSC_WIDTH_ALIGN);
		roi_end_w = ALIGN_DOWN(GET_SCALED_SIZE(roi_end_x, hratio), MCSC_WIDTH_ALIGN);
		/* Stripe start position to be set for calculating dma offset */
		output->stripe_roi_start_pos_x =  roi_start_w;
		output->stripe_roi_end_pos_x = roi_end_w;
	}
	else
#endif
	{
		/* W Ratio */
		is_hw_mcsc_calc_poly_dst_size(hw_ip, instance, src_width, out_width, &poly_dst_width,
			poly_ratio_down, true, MCSC_WIDTH_ALIGN, &post_en);
		hratio = GET_ZOOM_RATIO(temp_width, poly_dst_width);
	}
	/* H Ratio */
	is_hw_mcsc_calc_poly_dst_size(hw_ip, instance, src_height, out_height, &poly_dst_height,
		poly_ratio_down, false, 1, &post_en);

	/* if need post scaler, need to check line buffer size */
	if (post_en && poly_dst_width > MCSC_POST_MAX_WIDTH) {
		poly_dst_height = out_height;
		post_en = false;
	}

	vratio = GET_ZOOM_RATIO(temp_height, poly_dst_height);

#if defined(MCSC_POST_WA)
	/* The post scaler guarantee the quality of image          */
	/*  in case the scaling ratio equals to multiple of x1/256 */
	if ((post_en && ((poly_dst_width << MCSC_POST_WA_SHIFT) % out_width))
		|| (post_en && ((poly_dst_height << MCSC_POST_WA_SHIFT) % out_height))) {
		u32 multiple_hratio = 1;
		u32 multiple_vratio = 1;
		u32 shift = 0;

		for (shift = 0; shift <= MCSC_POST_WA_SHIFT; shift++) {
			if (((out_width % (1 << (MCSC_POST_WA_SHIFT-shift))) == 0)
				&& (out_height % (1 << (MCSC_POST_WA_SHIFT-shift)) == 0)) {
				multiple_hratio = out_width  / (1 << (MCSC_POST_WA_SHIFT-shift));
				multiple_vratio = out_height / (1 << (MCSC_POST_WA_SHIFT-shift));
				break;
			}
		}
		msdbg_hw(2, "[OUT:%d]poly_phase: shift(%d), ratio(%d,%d),"
			"size(%dx%d) before calculation\n",
			instance, hw_ip, output_id, shift, multiple_hratio, multiple_hratio,
			poly_dst_width, poly_dst_height);
		poly_dst_width  = MCSC_ROUND_UP(poly_dst_width, multiple_hratio);
		poly_dst_height = MCSC_ROUND_UP(poly_dst_height, multiple_vratio);
		if (poly_dst_width % 2) {
			poly_dst_width  = poly_dst_width  + multiple_hratio;
			poly_dst_height = poly_dst_height + multiple_vratio;
		}
		msdbg_hw(2, "[OUT:%d]poly_phase: size(%dx%d) after calculation\n",
			instance, hw_ip, output_id, poly_dst_width, poly_dst_height);
	}
#endif

	if (poly_dst_width % MCSC_WIDTH_ALIGN) {
		mswarn_hw("[OUT:%d] hw_mcsc_poly_phase: poly_dst_width(%d) should be multiple of %d\n",
			instance, hw_ip, output_id, poly_dst_width, MCSC_WIDTH_ALIGN);
	}

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];

	sc_coefs[0] = &setfile->sc_coef;
	sc_coefs[1] = &setfile->sc_coef;	/* &setfile->sc_coef_uv */
	stripe_align = is_scaler_get_stripe_align(output->sbwc_type);

#ifdef USE_MCSC_STRIP_OUT_CROP
	if (use_out_crop) {
		/* Strip configuration */
		temp_stripe_start_pos_x = output->stripe_in_start_pos_x;
		/*
		 * From 2nd stripe region,
		 * stripe_pre_dst_x should be rounded up to gurantee the data for first interpolated pixel.
		 */
		if (stripe_input->index == 0)
			temp_stripe_pre_dst_x = ALIGN_DOWN(GET_SCALED_SIZE(temp_stripe_start_pos_x, hratio), MCSC_WIDTH_ALIGN);
		else
			temp_stripe_pre_dst_x = ALIGN_DOWN(GET_SCALED_SIZE(temp_stripe_start_pos_x, hratio) + (MCSC_WIDTH_ALIGN - 1), MCSC_WIDTH_ALIGN);
		is_scaler_set_poly_strip_enable(hw_ip->regs[REG_SETA], output_id, 1);
		is_scaler_set_poly_strip_config(hw_ip->regs[REG_SETA], output_id, temp_stripe_pre_dst_x, temp_stripe_start_pos_x);

		msdbg_hw(2, "[OUT:%d] poly_phase: stripe input pos_x(%d) scaled output pos_x(%d) full in width(%d) full out width(%d)\n",
			instance, hw_ip, output_id,
			temp_stripe_start_pos_x, temp_stripe_pre_dst_x,
			full_in_width, poly_dst_full_width);

		offset = ((ulong)temp_stripe_pre_dst_x * hratio + (ulong)h_phase_offset)
						- ((ulong)temp_stripe_start_pos_x << MCSC_PRECISION);
		poly_dst_width = ALIGN_DOWN((u32)((((ulong)src_width << MCSC_PRECISION) - offset) / hratio), MCSC_WIDTH_ALIGN);

		/* Use out crop */
		if (!post_en) {
			if (stripe_input->index == 0) {
				/* LEFT */
				roi_end_w = ALIGN_DOWN(roi_end_w, stripe_align);
				output->stripe_roi_end_pos_x = roi_end_w;
			} else if (stripe_input->index == stripe_input->total_count - 1) {
				/* RIGHT */
				roi_start_w = ALIGN_DOWN(roi_start_w, stripe_align);
				output->stripe_roi_start_pos_x = roi_start_w;
			} else {
				/* MIDDLE */
				roi_start_w = ALIGN_DOWN(roi_start_w, stripe_align);
				roi_end_w = ALIGN_DOWN(roi_end_w, stripe_align);
				output->stripe_roi_start_pos_x = roi_start_w;
				output->stripe_roi_end_pos_x = roi_end_w;
			}

			otcrop_pos_x = roi_start_w - temp_stripe_pre_dst_x;
			otcrop_pos_y = 0;
			otcrop_width = roi_end_w - roi_start_w;
			otcrop_height = poly_dst_height;
			if ((otcrop_pos_x < 0 || otcrop_width < 0) ||
				otcrop_pos_x + otcrop_width > poly_dst_width) {
				mserr_hw("[OUT:%d] poly_phase: invalid outcrop region(output : %dx%d < output crop : %d, %d, %dx%d)\n",
					instance, hw_ip, output_id,
					poly_dst_width, poly_dst_height,
					otcrop_pos_x, otcrop_pos_y,
					otcrop_width, otcrop_height);
				otcrop_pos_x = 0;
				otcrop_width = poly_dst_width;
			}
			if (otcrop_width == 0) {
				mswarn_hw("[OUT:%d] poly_phase: stripe region w/o margin is zero(output : %dx%d < output crop : %d, %d, %dx%d), disable output port\n",
					instance, hw_ip, output_id,
					poly_dst_width, poly_dst_height,
					otcrop_pos_x, otcrop_pos_y,
					otcrop_width, otcrop_height);
				output->otf_cmd = OTF_OUTPUT_COMMAND_DISABLE;
				output->dma_cmd = DMA_OUTPUT_COMMAND_DISABLE;
				is_scaler_set_poly_scaler_enable(hw_ip->regs[REG_SETA], hw_ip->id, output_id, 0);
				is_scaler_set_poly_out_crop_enable(hw_ip->regs[REG_SETA], output_id, 0);
				is_scaler_set_poly_strip_enable(hw_ip->regs[REG_SETA], output_id, 0);

				return ret;
			} else if (otcrop_width < MCSC_DMA_MIN_WIDTH) {
				mswarn_hw("[OUT:%d] poly_phase: otcrop width(%d) is less than dma min width(%d)",
					instance, hw_ip, output_id,
					otcrop_width, MCSC_DMA_MIN_WIDTH);
				if (hw_mcsc->stripe_region == MCSC_STRIPE_REGION_L)
					otcrop_width = MCSC_DMA_MIN_WIDTH;
				else if (hw_mcsc->stripe_region == MCSC_STRIPE_REGION_R) {
					otcrop_pos_x = otcrop_pos_x - (MCSC_DMA_MIN_WIDTH - otcrop_width);
					output->stripe_roi_start_pos_x = roi_start_w - (MCSC_DMA_MIN_WIDTH - otcrop_width);
					otcrop_width = MCSC_DMA_MIN_WIDTH;
				}
			}
			is_scaler_set_poly_out_crop_enable(hw_ip->regs[REG_SETA], output_id, 1);
			is_scaler_set_poly_out_crop_size(hw_ip->regs[REG_SETA], output_id, otcrop_pos_x, 0, otcrop_width, poly_dst_height);

			msdbg_hw(2, "[OUT:%d] poly_phase: use outcrop(output : %dx%d -> output crop : %d, %d, %dx%d)\n",
				instance, hw_ip, output_id,
				poly_dst_width, poly_dst_height,
				otcrop_pos_x, otcrop_pos_y,
				otcrop_width, otcrop_height);
		} else {
			is_scaler_set_poly_out_crop_enable(hw_ip->regs[REG_SETA], output_id, 0);
		}
	} else {
		is_scaler_set_poly_out_crop_enable(hw_ip->regs[REG_SETA], output_id, 0);
		is_scaler_set_poly_strip_enable(hw_ip->regs[REG_SETA], output_id, 0);
	}
#endif

	is_scaler_set_poly_scaling_ratio(hw_ip->regs[REG_SETA], output_id, hratio, vratio);
	if (!IS_ENABLED(SKIP_SETFILE_LOAD) && !test_bit(MCSC_DBG_SKIP_SETFILE, &debug_mcsc))
		is_scaler_set_poly_scaler_coef(hw_ip->regs[REG_SETA], output_id, hratio, vratio,
			sc_coefs, 2, sensor_position);

	is_scaler_set_poly_round_mode(hw_ip->regs[REG_SETA], output_id, round_mode_en);
	is_scaler_set_poly_dst_size(hw_ip->regs[REG_SETA], output_id,
		poly_dst_width, poly_dst_height);

	msdbg_hw(2, "[OUT:%d] poly_phase: input(%d,%d,%dx%d) -> output(%d,%d,%dx%d)\n",
				instance, hw_ip, output_id,
				src_pos_x, src_pos_y, src_width, src_height,
				0, 0, poly_dst_width, poly_dst_height);
	return ret;
}

int is_hw_mcsc_post_chain(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, struct param_stripe_input *stripe_input, u32 output_id, u32 instance)
{
	int ret = 0;
	u32 img_width, img_height;
	u32 dst_width, dst_height;
	ulong temp_width, temp_height;
	u32 hratio, vratio;
	u32 input_id = 0;
	u32 stripe_align;
	bool post_en = true;
	bool round_mode_en = true;
	struct is_hw_mcsc *hw_mcsc;
	enum exynos_sensor_position sensor_position;
	struct hw_mcsc_setfile *setfile;
	struct scaler_coef_cfg *sc_coefs[2];
#ifdef USE_MCSC_STRIP_OUT_CROP
	bool poly_otcrop_en = false;
	int otcrop_pos_x = 0, otcrop_width = 0;
	u32 otcrop_pos_y = 0, otcrop_height = 0;
	u32 use_out_crop = 0;
	u32 full_in_width = 0, full_out_width = 0;
	u32 roi_start_x = 0, roi_end_x = 0, roi_start_w = 0, roi_end_w = 0;
	u32 temp_stripe_start_pos_x = 0, temp_stripe_pre_dst_x = 0;
	u32 h_phase_offset = 0;
	ulong offset;
#endif

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!output);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]post_chain_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->otf_cmd, output->dma_cmd);

	input_id = is_scaler_get_scaler_path(hw_ip->regs[REG_SETA], hw_ip->id, output_id);

#ifdef USE_MCSC_STRIP_OUT_CROP
	use_out_crop = (output->cmd & BIT(MCSC_OUT_CROP)) >> MCSC_OUT_CROP;
	full_in_width = output->full_input_width;
	full_out_width = output->full_output_width;
	roi_start_x = output->stripe_roi_start_pos_x;
	roi_end_x = output->stripe_roi_end_pos_x;
#endif

	if (output->otf_cmd == OTF_OUTPUT_COMMAND_DISABLE
		&& output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		is_scaler_set_post_scaler_enable(hw_ip->regs[REG_SETA], output_id, 0);
		return ret;
	}

#ifdef USE_MCSC_STRIP_OUT_CROP
	poly_otcrop_en = is_scaler_get_poly_out_crop_enable(hw_ip->regs[REG_SETA], output_id);
	if (poly_otcrop_en) {
		is_scaler_get_poly_out_crop_size(hw_ip->regs[REG_SETA], output_id, &img_width, &img_height);
		output->width = img_width;
		output->height = img_height;
	} else
#endif
	{
		is_scaler_get_poly_dst_size(hw_ip->regs[REG_SETA], output_id, &img_width, &img_height);
	}

	dst_width = output->width;
	dst_height = output->height;

	if (img_width % MCSC_WIDTH_ALIGN) {
		mswarn_hw("[OUT:%d] hw_mcsc_post_chain: img_width(%d) should be multiple of %d\n",
			instance, hw_ip, output_id, img_width, MCSC_WIDTH_ALIGN);
	}
	if (dst_width % MCSC_WIDTH_ALIGN) {
		mswarn_hw("[OUT:%d] hw_mcsc_post_chain: dst_width(%d) should be multiple of %d\n",
			instance, hw_ip, output_id, dst_width, MCSC_WIDTH_ALIGN);
	}

	/* if x1 ~ x1/4 scaling, post scaler bypassed */
	if ((img_width == dst_width) && (img_height == dst_height))
		post_en = false;

	is_scaler_set_post_img_size(hw_ip->regs[REG_SETA], output_id, img_width, img_height);

	temp_width  = (ulong)img_width;
	temp_height = (ulong)img_height;
#ifdef USE_MCSC_STRIP_OUT_CROP
	/* Use ratio of full size if stripe processing  */
	if (use_out_crop) {
		hratio = GET_ZOOM_RATIO(full_in_width, full_out_width);
		dst_width = ALIGN_DOWN(GET_SCALED_SIZE(img_width, hratio), MCSC_WIDTH_ALIGN);

		roi_start_w = ALIGN_DOWN(GET_SCALED_SIZE(roi_start_x, hratio), MCSC_WIDTH_ALIGN);
		roi_end_w = ALIGN_DOWN(GET_SCALED_SIZE(roi_end_x, hratio), MCSC_WIDTH_ALIGN);
		/* Stripe start position to be set for calculating dma offset */
		output->stripe_roi_start_pos_x = roi_start_w;
		output->stripe_roi_end_pos_x = roi_end_w;
	}
	else
#endif
		hratio = GET_ZOOM_RATIO(temp_width, dst_width);
	vratio = GET_ZOOM_RATIO(temp_height, dst_height);

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];

	sc_coefs[0] = &setfile->sc_coef;
	sc_coefs[1] = &setfile->sc_coef;	/* &setfile->sc_coef_uv */
	stripe_align = is_scaler_get_stripe_align(output->sbwc_type);

#ifdef USE_MCSC_STRIP_OUT_CROP
	if (!poly_otcrop_en && use_out_crop) {
		/* Strip configuration */
		if (is_scaler_get_poly_strip_enable(hw_ip->regs[REG_SETA], output_id))
			is_scaler_get_poly_strip_config(hw_ip->regs[REG_SETA],
					output_id,
					&temp_stripe_start_pos_x,
					&temp_stripe_pre_dst_x);
		else
			temp_stripe_start_pos_x = output->stripe_in_start_pos_x;
		/*
		 * From 2nd stripe region,
		 * stripe_pre_dst_x should be rounded up to gurantee the data for first interpolated pixel.
		 */
		if (stripe_input->index == 0)
			temp_stripe_pre_dst_x = ALIGN_DOWN(GET_SCALED_SIZE(temp_stripe_start_pos_x, hratio), MCSC_WIDTH_ALIGN);
		else
			temp_stripe_pre_dst_x = ALIGN_DOWN(GET_SCALED_SIZE(temp_stripe_start_pos_x, hratio) + (MCSC_WIDTH_ALIGN - 1), MCSC_WIDTH_ALIGN);
		is_scaler_set_post_strip_enable(hw_ip->regs[REG_SETA], output_id, 1);
		is_scaler_set_post_strip_config(hw_ip->regs[REG_SETA], output_id, temp_stripe_pre_dst_x, temp_stripe_start_pos_x);

		msdbg_hw(2, "[OUT:%d] post_chain: stripe input pos_x(%d) scaled output pos_x(%d) full in width(%d) full out width(%d)\n",
			instance, hw_ip, output_id,
			temp_stripe_start_pos_x, temp_stripe_pre_dst_x,
			full_in_width, full_out_width);

		offset = ((ulong)temp_stripe_pre_dst_x * hratio + (ulong)h_phase_offset)
						- ((ulong)temp_stripe_start_pos_x << MCSC_PRECISION);
		dst_width = ALIGN_DOWN((u32)((((ulong)img_width << MCSC_PRECISION) - offset) / hratio), MCSC_WIDTH_ALIGN);

		/* Use Out crop */
		if (stripe_input->index == 0) {
			/* LEFT */
			roi_end_w = ALIGN_DOWN(roi_end_w, stripe_align);
			output->stripe_roi_end_pos_x = roi_end_w;
		} else if (stripe_input->index == stripe_input->total_count - 1) {
			/* RIGHT */
			roi_start_w = ALIGN_DOWN(roi_start_w, stripe_align);
			output->stripe_roi_start_pos_x = roi_start_w;
		} else {
			/* MIDDLE */
			roi_start_w = ALIGN_DOWN(roi_start_w, stripe_align);
			roi_end_w = ALIGN_DOWN(roi_end_w, stripe_align);
			output->stripe_roi_start_pos_x = roi_start_w;
			output->stripe_roi_end_pos_x = roi_end_w;
		}

		otcrop_pos_x = roi_start_w - temp_stripe_pre_dst_x;
		otcrop_pos_y = 0;
		otcrop_width = roi_end_w - roi_start_w;
		otcrop_height = dst_height;
		if ((otcrop_pos_x < 0 || otcrop_width < 0) ||
			otcrop_pos_x + otcrop_width > dst_width) {
			mserr_hw("[OUT:%d] post_chain: invalid outcrop region(output : %dx%d < output crop : %d, %d, %dx%d)\n",
				instance, hw_ip, output_id,
				dst_width, dst_height,
				otcrop_pos_x, otcrop_pos_y,
				otcrop_width, otcrop_height);
			otcrop_pos_x = 0;
			otcrop_width = dst_width;
		}
		if (otcrop_width == 0) {
			mswarn_hw("[OUT:%d] post_chain: stripe region w/o margin is zero(output : %dx%d < output crop : %d, %d, %dx%d), disable output port\n",
				instance, hw_ip, output_id,
				dst_width, dst_height,
				otcrop_pos_x, otcrop_pos_y,
				otcrop_width, otcrop_height);
			output->otf_cmd = OTF_OUTPUT_COMMAND_DISABLE;
			output->dma_cmd = DMA_OUTPUT_COMMAND_DISABLE;
			is_scaler_set_post_scaler_enable(hw_ip->regs[REG_SETA], output_id, 0);
			is_scaler_set_post_out_crop_enable(hw_ip->regs[REG_SETA], output_id, 0);
			is_scaler_set_post_strip_enable(hw_ip->regs[REG_SETA], output_id, 0);

			return ret;
		} else if (otcrop_width < MCSC_DMA_MIN_WIDTH) {
			mswarn_hw("[OUT:%d] post_chain: otcrop width(%d) is less than dma min width(%d)",
					instance, hw_ip, output_id,
					otcrop_width, MCSC_DMA_MIN_WIDTH);
			if (hw_mcsc->stripe_region == MCSC_STRIPE_REGION_L)
				otcrop_width = MCSC_DMA_MIN_WIDTH;
			else if (hw_mcsc->stripe_region == MCSC_STRIPE_REGION_R) {
				otcrop_pos_x = otcrop_pos_x - (MCSC_DMA_MIN_WIDTH - otcrop_width);
				output->stripe_roi_start_pos_x = roi_start_w - (MCSC_DMA_MIN_WIDTH - otcrop_width);
				otcrop_width = MCSC_DMA_MIN_WIDTH;
			}
		}
		is_scaler_set_post_out_crop_enable(hw_ip->regs[REG_SETA], output_id, 1);
		is_scaler_set_post_out_crop_size(hw_ip->regs[REG_SETA], output_id, otcrop_pos_x, 0, otcrop_width, dst_height);

		msdbg_hw(2, "[OUT:%d] post_chain: use outcrop(output : %dx%d -> output crop : %d, %d, %dx%d)\n",
			instance, hw_ip, output_id,
			dst_width, dst_height,
			otcrop_pos_x, otcrop_pos_y,
			otcrop_width, otcrop_height);
	} else {
		is_scaler_set_post_out_crop_enable(hw_ip->regs[REG_SETA], output_id, 0);
		is_scaler_set_post_strip_enable(hw_ip->regs[REG_SETA], output_id, 0);
	}
#endif

	is_scaler_set_post_scaling_ratio(hw_ip->regs[REG_SETA], output_id, hratio, vratio);
	if (!IS_ENABLED(SKIP_SETFILE_LOAD) && !test_bit(MCSC_DBG_SKIP_SETFILE, &debug_mcsc))
		is_scaler_set_post_scaler_coef(hw_ip->regs[REG_SETA], output_id,
		hratio, vratio, sc_coefs, 2);

	is_scaler_set_post_round_mode(hw_ip->regs[REG_SETA], output_id, round_mode_en);
	is_scaler_set_post_dst_size(hw_ip->regs[REG_SETA], output_id, dst_width, dst_height);
	is_scaler_set_post_scaler_enable(hw_ip->regs[REG_SETA], output_id, post_en);
	/* Print size information if post scaler is enabled */
	if (post_en)
		msdbg_hw(2, "[OUT:%d] post_chain: input(%d,%d,%dx%d) -> output(%d,%d,%dx%d)\n",
					instance, hw_ip, output_id,
					0, 0, img_width, img_height,
					0, 0, dst_width, dst_height);
	return ret;
}

int is_hw_mcsc_flip(struct is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	struct is_hw_mcsc *hw_mcsc;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]flip_setting flip(%d),cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->flip, output->otf_cmd, output->dma_cmd);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE)
		return ret;

	is_scaler_set_flip_mode(hw_ip->regs[REG_SETA], output_id, output->flip);

	return ret;
}

int is_hw_mcsc_dma_output(struct is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	u32 out_width, out_height, scaled_width = 0, scaled_height = 0;
	u32 format, plane, order, bitwidth;
	u32 y_stride, uv_stride;
	u32 y_2bit_stride = 0, uv_2bit_stride = 0;
	u32 img_format;
	u32 input_id = 0;
	bool conv420_en = false;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	u32 conv420_weight = 0;
#ifdef USE_MCSC_STRIP_OUT_CROP
	bool poly_otcrop_en = false, post_otcrop_en = false;
#endif
	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->out_dma[output_id] != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	msdbg_hw(2, "[OUT:%d]dma_output_setting cmd(O:%d,D:%d)\n",
		instance, hw_ip, output_id, output->otf_cmd, output->dma_cmd);

	input_id = is_scaler_get_scaler_path(hw_ip->regs[REG_SETA], hw_ip->id, output_id);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		is_scaler_set_dma_out_enable(hw_ip->regs[REG_SETA], output_id, false);
		return ret;
	}

#ifdef USE_MCSC_STRIP_OUT_CROP
	poly_otcrop_en = is_scaler_get_poly_out_crop_enable(hw_ip->regs[REG_SETA], output_id);
	post_otcrop_en = is_scaler_get_post_out_crop_enable(hw_ip->regs[REG_SETA], output_id);
	if (poly_otcrop_en){
		is_scaler_get_poly_out_crop_size(hw_ip->regs[REG_SETA], output_id, &scaled_width, &scaled_height);
		output->width = scaled_width;
		output->height = scaled_height;
	} else if (post_otcrop_en){
		is_scaler_get_post_out_crop_size(hw_ip->regs[REG_SETA], output_id, &scaled_width, &scaled_height);
		output->width = scaled_width;
		output->height = scaled_height;
	} else
#endif
	{
		is_scaler_get_post_dst_size(hw_ip->regs[REG_SETA], output_id, &scaled_width, &scaled_height);
	}
	out_width  = output->width;
	out_height = output->height;
	format     = output->dma_format;
	plane      = output->plane;
	order      = output->dma_order;
	bitwidth   = output->dma_bitwidth;
	y_stride   = output->dma_stride_y;
	uv_stride  = output->dma_stride_c;

	ret = is_hw_mcsc_check_format(HW_MCSC_DMA_OUTPUT,
		format, bitwidth, out_width, out_height);
	if (ret) {
		mserr_hw("[OUT:%d]Invalid MCSC DMA Output format: fmt(%d),bit(%d),size(%dx%d)",
			instance, hw_ip, output_id, format, bitwidth, out_width, out_height);
		return ret;
	}

	ret = is_hw_mcsc_adjust_output_img_fmt(bitwidth, format, plane, order,
			&img_format, &conv420_en);
	if (ret < 0) {
		mswarn_hw("Invalid dma image format", instance, hw_ip);
		img_format = hw_mcsc->out_img_format[output_id];
		conv420_en = hw_mcsc->conv420_en[output_id];
	} else {
		hw_mcsc->out_img_format[output_id] = img_format;
		hw_mcsc->conv420_en[output_id] = conv420_en;
	}

	is_scaler_set_wdma_format(hw_ip->regs[REG_SETA], hw_ip->id, output_id, plane, img_format);
	/* HW Guide : conv420_weight is set to 0 */
	if (out_width > MCSC_LINE_BUF_SIZE)
		conv420_weight = 16;
	is_scaler_set_420_conversion(hw_ip->regs[REG_SETA], output_id, conv420_weight, conv420_en);

	if ((scaled_width != 0) && (scaled_height != 0)) {
		if ((scaled_width != out_width) || (scaled_height != out_height)) {
			msdbg_hw(2, "Invalid output[%d] scaled size (%d/%d)(%d/%d)\n",
				instance, hw_ip, output_id, scaled_width, scaled_height,
				out_width, out_height);
			return -EINVAL;
		}
	}

	is_scaler_set_wdma_sbwc_config(hw_ip->regs[REG_SETA],
		output, output_id,
		&y_stride, &uv_stride, &y_2bit_stride, &uv_2bit_stride);

	if (output->plane == DMA_INOUT_PLANE_4) {
		y_2bit_stride = ALIGN(out_width / 4, 16);
		uv_2bit_stride = ALIGN(out_width / 4, 16);
	}

	is_scaler_set_wdma_size(hw_ip->regs[REG_SETA], output_id, out_width, out_height);
	is_scaler_set_wdma_stride(hw_ip->regs[REG_SETA], output_id, y_stride, uv_stride);
	is_scaler_set_wdma_2bit_stride(hw_ip->regs[REG_SETA], output_id, y_2bit_stride, uv_2bit_stride);
	is_scaler_set_wdma_dither(hw_ip->regs[REG_SETA], output_id, format, bitwidth, plane);

	return ret;
}

int is_hw_mcsc_hwfc_mode(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 hwfc_output_ids, u32 dma_output_ids, u32 instance)
{
	int ret = 0;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	u32 input_id = 0, output_id;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!input);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->hwfc != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	/* skip hwfc mode when..
	 *  - one core share Preview, Reprocessing
	 *  - at Preview stream, DMA output port shared to previous reprocessing port
	 *  ex> at 1x3 scaler,
	 *     1. preview - output 0 used, reprocessing - output 1, 2 used
	 *        above output is not overlapped
	 *        at this case, when CAPTURE -> PREVIEW, preview shot should not set hwfc_mode
	 *        due to not set hwfc_mode off during JPEG is operating.
	 *     2. preview - output 0, 1 used (1: preview callback), reprocessing - output 1, 2 used
	 *        above output is overlapped "output 1"
	 *        at this case, when CAPTURE -> PREVIEW, preview shot shuold set hwfc_mode to "0"
	 *        for avoid operate at Preview stream.
	 */
	if (!hw_mcsc->rep_flag[instance] && !(hw_mcsc->prev_hwfc_output_ids & dma_output_ids))
		return ret;

	msdbg_hw(2, "hwfc_mode_setting output[0x%08X]\n", instance, hw_ip, hwfc_output_ids);

	for (output_id = MCSC_OUTPUT0; output_id < cap->max_output; output_id++) {
		input_id = is_scaler_get_scaler_path(hw_ip->regs[REG_SETA], hw_ip->id, output_id);

		if ((hwfc_output_ids & (1 << output_id))
			|| (is_scaler_get_dma_out_enable(hw_ip->regs[REG_SETA], output_id))) {
			msdbg_hw(2, "hwfc_mode_setting hwfc_output_ids(0x%x)\n",
				instance, hw_ip, hwfc_output_ids);
			is_scaler_set_hwfc_mode(hw_ip->regs[REG_SETA], hwfc_output_ids);
			break;
		}
	}

	return ret;
}

int is_hw_mcsc_hwfc_output(struct is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	u32 width, height, format, plane;
	struct is_hw_mcsc *hw_mcsc;
	struct is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!cap);
	FIMC_BUG(!hw_ip->priv_info);

	/* can't support this function */
	if (cap->out_hwfc[output_id] != MCSC_CAP_SUPPORT)
		return ret;

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	width  = output->width;
	height = output->height;
	format = output->dma_format;
	plane = output->plane;

	msdbg_hw(2, "hwfc_config_setting mode(%dx%d, %d, %d)\n", instance, hw_ip,
			width, height, format, plane);

	if (output->hwfc)
		is_scaler_set_hwfc_config(hw_ip->regs[REG_SETA], output_id, format, plane,
			(output_id * HWFC_DMA_ID_OFFSET), width, height);

	return ret;
}

void is_hw_bchs_range(void __iomem *base_addr, u32 output_id, int yuv)
{
	u32 y_ofs, y_gain, c_gain00, c_gain01, c_gain10, c_gain11;

#ifdef ENABLE_10BIT_MCSC
	if (yuv == SCALER_OUTPUT_YUV_RANGE_FULL) {
		/* Y range - [0:1024], U/V range - [0:1024] */
		y_ofs = 0;
		y_gain = 1024;
		c_gain00 = 1024;
		c_gain01 = 0;
		c_gain10 = 0;
		c_gain11 = 1024;
	} else {
		/* YUV_RANGE_NARROW */
		/* Y range - [64:940], U/V range - [64:960] */
		y_ofs = 0;
		y_gain = 1024;
		c_gain00 = 1024;
		c_gain01 = 0;
		c_gain10 = 0;
		c_gain11 = 1024;
	}
#else
	if (yuv == SCALER_OUTPUT_YUV_RANGE_FULL) {
		/* Y range - [0:255], U/V range - [0:255] */
		y_ofs = 0;
		y_gain = 256;
		c_gain00 = 256;
		c_gain01 = 0;
		c_gain10 = 0;
		c_gain11 = 256;
	} else {
		/* YUV_RANGE_NARROW */
		/* Y range - [16:235], U/V range - [16:239] */
		y_ofs = 16;
		y_gain = 220;
		c_gain00 = 224;
		c_gain01 = 0;
		c_gain10 = 0;
		c_gain11 = 224;
	}
#endif
	is_scaler_set_b_c(base_addr, output_id, y_ofs, y_gain);
	is_scaler_set_h_s(base_addr, output_id, c_gain00, c_gain01, c_gain10, c_gain11);
}

void is_hw_bchs_clamp(void __iomem *base_addr, u32 output_id, int yuv,
	struct scaler_bchs_clamp_cfg *sc_bchs)
{
	u32 y_max, y_min, c_max, c_min;

#ifdef ENABLE_10BIT_MCSC
	if (yuv == SCALER_OUTPUT_YUV_RANGE_FULL) {
		y_max = 1023;
		y_min = 0;
		c_max = 1023;
		c_min = 0;
	} else {
		/* YUV_RANGE_NARROW */
		y_max = 940;
		y_min = 64;
		c_max = 960;
		c_min = 64;
	}
#else
	if (yuv == SCALER_OUTPUT_YUV_RANGE_FULL) {
		y_max = 255;
		y_min = 0;
		c_max = 255;
		c_min = 0;
	} else {
		/* YUV_RANGE_NARROW */
		y_max = 235;
		y_min = 16;
		c_max = 240;
		c_min = 16;
	}
#endif
	if (sc_bchs) {
		is_scaler_set_bchs_clamp(base_addr, output_id,
			sc_bchs->y_max, sc_bchs->y_min,
			sc_bchs->c_max, sc_bchs->c_min);
		dbg_hw(2, "[Y:max(%d),min(%d)][C:max(%d),min(%d)]\n",
			sc_bchs->y_max, sc_bchs->y_min,
			sc_bchs->c_max, sc_bchs->c_min);
	} else {
		is_scaler_set_bchs_clamp(base_addr, output_id, y_max, y_min, c_max, c_min);
	}
}

int is_hw_mcsc_output_yuvrange(struct is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance)
{
	int ret = 0;
	int yuv = 0;
	u32 input_id = 0;
	struct is_hw_mcsc *hw_mcsc = NULL;
	struct hw_mcsc_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	struct scaler_bchs_clamp_cfg *sc_bchs;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!output);
	FIMC_BUG(!hw_ip->priv_info);

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;

	if (!test_bit(output_id, &hw_mcsc->out_en))
		return ret;

	input_id = is_scaler_get_scaler_path(hw_ip->regs[REG_SETA], hw_ip->id, output_id);

	if (output->dma_cmd == DMA_OUTPUT_COMMAND_DISABLE) {
		is_scaler_set_bchs_enable(hw_ip->regs[REG_SETA], output_id, 0);
		return ret;
	}

	yuv = output->yuv_range;
	hw_mcsc->yuv_range = yuv; /* save for ISP */

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = hw_mcsc->cur_setfile[sensor_position][instance];

	sc_bchs = &setfile->sc_bchs[yuv];
	if (!IS_ENABLED(SKIP_SETFILE_LOAD) && !test_bit(MCSC_DBG_SKIP_SETFILE, &debug_mcsc)) {
		is_scaler_set_bchs_enable(hw_ip->regs[REG_SETA], output_id, 1);
#if !defined(USE_YUV_RANGE_BY_ISP)
	if (hw_mcsc->tune_set[instance]) {
		/* set yuv range config value by scaler_param yuv_range mode */
		is_scaler_set_b_c(hw_ip->regs[REG_SETA], output_id,
			setfile->sc_base[yuv].y_offset, setfile->sc_base[yuv].y_gain);
		is_scaler_set_h_s(hw_ip->regs[REG_SETA], output_id,
			setfile->sc_base[yuv].c_gain00, setfile->sc_base[yuv].c_gain01,
			setfile->sc_base[yuv].c_gain10, setfile->sc_base[yuv].c_gain11);
		msdbg_hw(2, "set YUV range(%d) by setfile parameter\n",
			instance, hw_ip, yuv);
		msdbg_hw(2, "[OUT:%d]output_yuv_setting: yuv(%d), cmd(O:%d,D:%d)\n",
			instance, hw_ip, output_id, yuv, output->otf_cmd, output->dma_cmd);
		dbg_hw(2, "[Y:offset(%d),gain(%d)][C:gain00(%d),01(%d),10(%d),11(%d)]\n",
			setfile->sc_base[yuv].y_offset, setfile->sc_base[yuv].y_gain,
			setfile->sc_base[yuv].c_gain00, setfile->sc_base[yuv].c_gain01,
			setfile->sc_base[yuv].c_gain10, setfile->sc_base[yuv].c_gain11);
	} else {
		is_hw_bchs_range(hw_ip->regs[REG_SETA], output_id, yuv);
		msdbg_hw(2, "YUV range set default settings\n", instance, hw_ip);
	}
	is_hw_bchs_clamp(hw_ip->regs[REG_SETA], output_id, yuv, sc_bchs);
#else
		is_hw_bchs_range(hw_ip->regs[REG_SETA], output_id, yuv);
		is_hw_bchs_clamp(hw_ip->regs[REG_SETA], output_id, yuv, sc_bchs);
#endif
	}
	return ret;
}

int is_hw_mcsc_adjust_input_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format)
{
	int ret = 0;

	switch (format) {
	case DMA_INOUT_FORMAT_YUV420:
		switch (plane) {
		case 2:
		case 4:
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				*img_format = MCSC_YUV420_2P_UFIRST;
				break;
			case DMA_INOUT_ORDER_CrCb:
				*img_format = MCSC_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("input order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV420_3P;
			break;
		default:
			err_hw("input plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_INOUT_FORMAT_YUV422:
		switch (plane) {
		case 1:
			switch (order) {
			case DMA_INOUT_ORDER_CrYCbY:
				*img_format = MCSC_YUV422_1P_VYUY;
				break;
			case DMA_INOUT_ORDER_CbYCrY:
				*img_format = MCSC_YUV422_1P_UYVY;
				break;
			case DMA_INOUT_ORDER_YCrYCb:
				*img_format = MCSC_YUV422_1P_YVYU;
				break;
			case DMA_INOUT_ORDER_YCbYCr:
				*img_format = MCSC_YUV422_1P_YUYV;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 2:
		case 4:
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				*img_format = MCSC_YUV422_2P_UFIRST;
				break;
			case DMA_INOUT_ORDER_CrCb:
				*img_format = MCSC_YUV422_2P_VFIRST;
				break;
			default:
				err_hw("img order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case 3:
			*img_format = MCSC_YUV422_3P;
			break;
		default:
			err_hw("img plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	case DMA_INOUT_FORMAT_Y:
		*img_format = MCSC_MONO_Y8;
		break;
	default:
		err_hw("img format error - (%d/%d/%d)", format, order, plane);
		ret = -EINVAL;
		break;
	}

	return ret;
}


int is_hw_mcsc_adjust_output_img_fmt(u32 bitwidth, u32 format, u32 plane, u32 order,
					u32 *img_format, bool *conv420_flag)
{
	int ret = 0;

	switch (bitwidth) {
	/* bitwidth : 8 bit */
	case DMA_INOUT_BIT_WIDTH_8BIT:
		switch (format) {
		case DMA_INOUT_FORMAT_YUV420:
			if (conv420_flag)
				*conv420_flag = true;
			if (plane == 3) {
				*img_format = MCSC_YUV420_3P;
				break;
			}
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				*img_format = MCSC_YUV420_2P_UFIRST;
				break;
			case DMA_INOUT_ORDER_CrCb:
				*img_format = MCSC_YUV420_2P_VFIRST;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_YUV422:
			switch (plane) {
			case 1:
				switch (order) {
				case DMA_INOUT_ORDER_CrYCbY:
					*img_format = MCSC_YUV422_1P_VYUY;
					break;
				case DMA_INOUT_ORDER_CbYCrY:
					*img_format = MCSC_YUV422_1P_UYVY;
					break;
				case DMA_INOUT_ORDER_YCrYCb:
					*img_format = MCSC_YUV422_1P_YVYU;
					break;
				case DMA_INOUT_ORDER_YCbYCr:
					*img_format = MCSC_YUV422_1P_YUYV;
					break;
				default:
					err_hw("output order error - (%d/%d/%d)", format, order, plane);
					ret = -EINVAL;
					break;
				}
				break;
			case 2:
				switch (order) {
				case DMA_INOUT_ORDER_CbCr:
					*img_format = MCSC_YUV422_2P_UFIRST;
					break;
				case DMA_INOUT_ORDER_CrCb:
					*img_format = MCSC_YUV422_2P_VFIRST;
					break;
				default:
					err_hw("output order error - (%d/%d/%d)", format, order, plane);
					ret = -EINVAL;
					break;
				}
				break;
			case 3:
				*img_format = MCSC_YUV422_3P;
				break;
			default:
				err_hw("img plane error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_RGB:
			switch (order) {
			case DMA_INOUT_ORDER_ARGB:
				*img_format = MCSC_RGB_ARGB8888;
				break;
			case DMA_INOUT_ORDER_BGRA:
				*img_format = MCSC_RGB_BGRA8888;
				break;
			case DMA_INOUT_ORDER_RGBA:
				*img_format = MCSC_RGB_RGBA8888;
				break;
			case DMA_INOUT_ORDER_ABGR:
				*img_format = MCSC_RGB_ABGR8888;
				break;
			default:
				*img_format = MCSC_RGB_RGBA8888;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_YUV444:
			switch (plane) {
			case 1:
				*img_format = MCSC_YUV444_1P;
				break;
			case 3:
				*img_format = MCSC_YUV444_3P;
				break;
			default:
				err_hw("img plane error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_Y:
			*img_format = MCSC_MONO_Y8;
			break;
		default:
			err_hw("output format error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	/* bitwidth : 10 bit */
	case DMA_INOUT_BIT_WIDTH_10BIT:
		switch (format) {
		case DMA_INOUT_FORMAT_YUV420:
			if (conv420_flag)
			*conv420_flag = true;
			switch (plane) {
			case 2:
				switch (order) {
				case DMA_INOUT_ORDER_CbCr:
					*img_format = MCSC_YUV420_2P_UFIRST_PACKED10;
					break;
				case DMA_INOUT_ORDER_CrCb:
					*img_format = MCSC_YUV420_2P_VFIRST_PACKED10;
					break;
				default:
					err_hw("output order error - (%d/%d/%d)", format, order, plane);
					ret = -EINVAL;
					break;
				}
				break;
			case 4:
				switch (order) {
				case DMA_INOUT_ORDER_CbCr:
					*img_format = MCSC_YUV420_2P_UFIRST_8P2;
					break;
				case DMA_INOUT_ORDER_CrCb:
					*img_format = MCSC_YUV420_2P_VFIRST_8P2;
					break;
				default:
					err_hw("output order error - (%d/%d/%d)", format, order, plane);
					ret = -EINVAL;
					break;
				}
				break;
			default:
				err_hw("img plane error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_YUV422:
			switch (plane) {
			case 2:
				switch (order) {
				case DMA_INOUT_ORDER_CbCr:
					*img_format = MCSC_YUV422_2P_UFIRST_PACKED10;
					break;
				case DMA_INOUT_ORDER_CrCb:
					*img_format = MCSC_YUV422_2P_VFIRST_PACKED10;
					break;
				default:
					err_hw("output order error - (%d/%d/%d)", format, order, plane);
					ret = -EINVAL;
					break;
				}
				break;
			case 4:
				switch (order) {
				case DMA_INOUT_ORDER_CbCr:
					*img_format = MCSC_YUV422_2P_UFIRST_8P2;
					break;
				case DMA_INOUT_ORDER_CrCb:
					*img_format = MCSC_YUV422_2P_VFIRST_8P2;
					break;
				default:
					err_hw("output order error - (%d/%d/%d)", format, order, plane);
					ret = -EINVAL;
					break;
				}
				break;
			default:
				err_hw("img plane error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_RGB:
			switch (order) {
			case DMA_INOUT_ORDER_RGBA:
				*img_format = MCSC_RGB_RGBA1010102;
				break;
			case DMA_INOUT_ORDER_ABGR:
				*img_format = MCSC_RGB_ABGR1010102;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_YUV444:
			switch (plane) {
			case 1:
				*img_format = MCSC_YUV444_1P_PACKED10;
				break;
			case 3:
				*img_format = MCSC_YUV444_3P_PACKED10;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		default:
			err_hw("img plane error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	/* bitwidth : 16 bit */
	case DMA_INOUT_BIT_WIDTH_16BIT:
		switch (format) {
		case DMA_INOUT_FORMAT_YUV420:
			if (conv420_flag)
				*conv420_flag = true;
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				*img_format = MCSC_YUV420_2P_UFIRST_P010;
				break;
			case DMA_INOUT_ORDER_CrCb:
				*img_format = MCSC_YUV420_2P_VFIRST_P010;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_YUV422:
			switch (order) {
			case DMA_INOUT_ORDER_CbCr:
				*img_format = MCSC_YUV422_2P_UFIRST_P210;
				break;
			case DMA_INOUT_ORDER_CrCb:
				*img_format = MCSC_YUV422_2P_VFIRST_P210;
				break;
			default:
				err_hw("output order error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		case DMA_INOUT_FORMAT_YUV444:
			switch (plane) {
			case 1:
				*img_format = MCSC_YUV444_1P_UNPACKED;
				break;
			case 3:
				*img_format = MCSC_YUV444_3P_UNPACKED;
				break;
			default:
				err_hw("img plane error - (%d/%d/%d)", format, order, plane);
				ret = -EINVAL;
				break;
			}
			break;
		default:
			err_hw("output format error - (%d/%d/%d)", format, order, plane);
			ret = -EINVAL;
			break;
		}
		break;
	default:
		err_hw("output bitwidth error - (%d/%d/%d)", format, order, plane);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int is_hw_mcsc_check_format(enum mcsc_io_type type, u32 format, u32 bit_width,
	u32 width, u32 height)
{
	int ret = 0;

	switch (type) {
	case HW_MCSC_OTF_INPUT:
		/* check otf input */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input width(%d)", width);
		}

		if (height < 16 || height > 9072) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input height(%d)", height);
		}

		if (!(format == OTF_INPUT_FORMAT_YUV422
			|| format == OTF_INPUT_FORMAT_Y)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input format(%d)", format);
		}

		if (bit_width != OTF_INPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Input format(%d)", bit_width);
		}
		break;
	case HW_MCSC_OTF_OUTPUT:
		/* check otf output */
		if (width < 16 || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output width(%d)", width);
		}

		if (height < 16 || height > 9072) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output height(%d)", height);
		}

		if (format != OTF_OUTPUT_FORMAT_YUV422) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output format(%d)", format);
		}

		if (bit_width != OTF_OUTPUT_BIT_WIDTH_8BIT) {
			ret = -EINVAL;
			err_hw("Invalid MCSC OTF Output format(%d)", bit_width);
		}
		break;
	case HW_MCSC_DMA_INPUT:
		/* check dma input */
		if (width < MCSC_DMA_MIN_WIDTH || width > 8192) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input width(%d)", width);
		}

		if (height < 16 || height > 9072) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input height(%d)", height);
		}

		if (!(format == DMA_INOUT_FORMAT_YUV422
			|| format == DMA_INOUT_FORMAT_Y)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input format(%d)", format);
		}

		if (!(bit_width == DMA_INOUT_BIT_WIDTH_8BIT ||
			bit_width == DMA_INOUT_BIT_WIDTH_10BIT ||
			bit_width == DMA_INOUT_BIT_WIDTH_16BIT)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Input format(%d)", bit_width);
		}
		break;
	case HW_MCSC_DMA_OUTPUT:
		/* check dma output */
		if (width < MCSC_DMA_MIN_WIDTH) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output width(%d)", width);
		}
		if (height < 16) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output height(%d)", height);
		}

		if (!(format == DMA_INOUT_FORMAT_YUV444
			|| format == DMA_INOUT_FORMAT_YUV422
			|| format == DMA_INOUT_FORMAT_YUV420
			|| format == DMA_INOUT_FORMAT_Y
			|| format == DMA_INOUT_FORMAT_RGB)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output format(%d)", format);
		}

		if (!(bit_width == DMA_INOUT_BIT_WIDTH_8BIT ||
			bit_width == DMA_INOUT_BIT_WIDTH_10BIT ||
			bit_width == DMA_INOUT_BIT_WIDTH_16BIT ||
			bit_width == DMA_INOUT_BIT_WIDTH_32BIT)) {
			ret = -EINVAL;
			err_hw("Invalid MCSC DMA Output bit_width(%d)", bit_width);
		}
		break;
	default:
		err_hw("Invalid MCSC type(%d)", type);
		break;
	}

	return ret;
}

static void is_hw_mcsc_size_dump(struct is_hw_ip *hw_ip)
{
	int i;
	u32 input_src = 0;
	u32 in_w, in_h = 0;
	u32 rdma_w = 0, rdma_h = 0;
	u32 poly_src_w = 0, poly_src_h = 0;
	u32 poly_dst_w = 0, poly_dst_h = 0;
	u32 post_in_w = 0, post_in_h = 0;
	u32 post_out_w = 0, post_out_h = 0;
	u32 wdma_enable = 0;
	u32 wdma_w = 0, wdma_h = 0;
	u32 rdma_y_stride = 0, rdma_uv_stride = 0;
	u32 wdma_y_stride = 0, wdma_uv_stride = 0;
	struct is_hw_mcsc_cap *cap;

	FIMC_BUG_VOID(!hw_ip);

	/* skip size dump, if hw_ip wasn't opened */
	if (!test_bit(HW_OPEN, &hw_ip->state))
		return;

	cap = GET_MCSC_HW_CAP(hw_ip);
	if (!cap) {
		err_hw("failed to get hw_mcsc_cap(%p)", cap);
		return;
	}

	input_src = is_scaler_get_input_source(hw_ip->regs[REG_SETA], hw_ip->id);
	is_scaler_get_input_img_size(hw_ip->regs[REG_SETA], hw_ip->id, &in_w, &in_h);
	is_scaler_get_rdma_size(hw_ip->regs[REG_SETA], &rdma_w, &rdma_h);
	is_scaler_get_rdma_stride(hw_ip->regs[REG_SETA], &rdma_y_stride, &rdma_uv_stride);

	sdbg_hw(2, "=SIZE=====================================\n"
		"[INPUT] SRC:%d(0:OTF_0, 1:OTF_1, 2:DMA), SIZE:%dx%d\n"
		"[RDMA] SIZE:%dx%d, STRIDE: Y:%d, UV:%d\n",
		hw_ip, input_src, in_w, in_h,
		rdma_w, rdma_h, rdma_y_stride, rdma_uv_stride);

	for (i = MCSC_OUTPUT0; i < cap->max_output; i++) {
		is_scaler_get_poly_src_size(hw_ip->regs[REG_SETA], i, &poly_src_w, &poly_src_h);
		is_scaler_get_poly_dst_size(hw_ip->regs[REG_SETA], i, &poly_dst_w, &poly_dst_h);
		is_scaler_get_post_img_size(hw_ip->regs[REG_SETA], i, &post_in_w, &post_in_h);
		is_scaler_get_post_dst_size(hw_ip->regs[REG_SETA], i, &post_out_w, &post_out_h);
		is_scaler_get_wdma_size(hw_ip->regs[REG_SETA], i, &wdma_w, &wdma_h);
		is_scaler_get_wdma_stride(hw_ip->regs[REG_SETA], i, &wdma_y_stride, &wdma_uv_stride);
		wdma_enable = is_scaler_get_dma_out_enable(hw_ip->regs[REG_SETA], i);

		dbg_hw(2, "[POLY%d] SRC:%dx%d, DST:%dx%d\n"
			"[POST%d] SRC:%dx%d, DST:%dx%d\n"
			"[WDMA%d] ENABLE:%d, SIZE:%dx%d, STRIDE: Y:%d, UV:%d\n",
			i, poly_src_w, poly_src_h, poly_dst_w, poly_dst_h,
			i, post_in_w, post_in_h, post_out_w, post_out_h,
			i, wdma_enable, wdma_w, wdma_h, wdma_y_stride, wdma_uv_stride);
	}
	sdbg_hw(2, "==========================================\n", hw_ip);
}

void is_hw_mcsc_set_ni(struct is_hardware *hardware, struct is_frame *frame,
	u32 instance)
{
	u32 index, core_num;
	struct is_hw_ip *hw_ip;

	FIMC_BUG_VOID(!frame);

	index = frame->fcount % NI_BACKUP_MAX;
	core_num = GET_CORE_NUM(DEV_HW_MCSC0);

	hw_ip = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, DEV_HW_MCSC0);
	if (hw_ip && frame->shot) {
		hardware->ni_udm[core_num][index] = frame->shot->udm.ni;
		msdbg_hw(2, "set_ni: [F:%d], %d,%d,%d -> %d,%d,%d\n",
				instance, hw_ip, frame->fcount,
				frame->shot->udm.ni.currentFrameNoiseIndex,
				frame->shot->udm.ni.nextFrameNoiseIndex,
				frame->shot->udm.ni.nextNextFrameNoiseIndex,
				hardware->ni_udm[core_num][index].currentFrameNoiseIndex,
				hardware->ni_udm[core_num][index].nextFrameNoiseIndex,
				hardware->ni_udm[core_num][index].nextNextFrameNoiseIndex);
	}
}
EXPORT_SYMBOL_GPL(is_hw_mcsc_set_ni);

static int is_hw_mcsc_get_meta(struct is_hw_ip *hw_ip,
		struct is_frame *frame, unsigned long hw_map)
{
	int ret = 0;
	struct is_hw_mcsc *hw_mcsc;

	if (unlikely(!frame)) {
		mserr_hw("get_meta: frame is null", atomic_read(&hw_ip->instance), hw_ip);
		return 0;
	}

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	hw_mcsc = (struct is_hw_mcsc *)hw_ip->priv_info;
	if (unlikely(!hw_mcsc)) {
		mserr_hw("priv_info is NULL", frame->instance, hw_ip);
		return -EINVAL;
	}

	return ret;
}

int is_hw_mcsc_restore(struct is_hw_ip *hw_ip, u32 instance)
{
	if (!test_bit(HW_OPEN, &hw_ip->state))
		return -EINVAL;

	msinfo_hw("hw_mcsc_reset\n", instance, hw_ip);

	is_hw_mcsc_reset(hw_ip, instance);
	is_hw_mcsc_clear_interrupt(hw_ip, instance);

	return 0;
}

static int is_hw_mcsc_notify_timeout(struct is_hw_ip *hw_ip, u32 instance)
{
	FIMC_BUG(!hw_ip);

	is_scaler_dump(hw_ip->regs[REG_SETA]);

	return 0;
}

int is_hw_mcsc_dump_regs(struct is_hw_ip *hw_ip, u32 instance, u32 fcount,
	struct cr_set *regs, u32 regs_size, enum is_reg_dump_type dump_type)
{
	is_scaler_dump(hw_ip->regs[REG_SETA]);
	return 0;
}

const struct is_hw_ip_ops is_hw_mcsc_ops = {
	.open			= is_hw_mcsc_open,
	.init			= is_hw_mcsc_init,
	.deinit			= is_hw_mcsc_deinit,
	.close			= is_hw_mcsc_close,
	.enable			= is_hw_mcsc_enable,
	.disable		= is_hw_mcsc_disable,
	.shot			= is_hw_mcsc_shot,
	.set_param		= is_hw_mcsc_set_param,
	.get_meta		= is_hw_mcsc_get_meta,
	.frame_ndone		= is_hw_mcsc_frame_ndone,
	.load_setfile		= is_hw_mcsc_load_setfile,
	.apply_setfile		= is_hw_mcsc_apply_setfile,
	.delete_setfile		= is_hw_mcsc_delete_setfile,
	.size_dump		= is_hw_mcsc_size_dump,
	.restore		= is_hw_mcsc_restore,
	.dump_regs		= is_hw_mcsc_dump_regs,
	.notify_timeout		= is_hw_mcsc_notify_timeout
};
KUNIT_EXPORT_SYMBOL(is_hw_mcsc_ops);

int is_hw_mcsc_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name)
{
	int ret = 0;
	int hw_slot;

	hw_ip->ops = &is_hw_mcsc_ops;

	hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, id);
	if (!valid_hw_slot_id(hw_slot)) {
		serr_hw("invalid hw_slot (%d)", hw_ip, hw_slot);
		return -EINVAL;
	}

	/* set mcsc interrupt handler */
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &is_hw_mcsc_handle_interrupt;

	return ret;
}
EXPORT_SYMBOL_GPL(is_hw_mcsc_probe);

void is_hw_mcsc_remove(struct is_hw_ip *hw_ip)	{ }
EXPORT_SYMBOL_GPL(is_hw_mcsc_remove);
