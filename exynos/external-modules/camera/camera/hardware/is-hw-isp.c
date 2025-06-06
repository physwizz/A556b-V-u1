/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-hw-3aa.h"
#include "is-hw-mcscaler-v3.h"
#include "is-hw-isp.h"
#include "is-err.h"
#include "is-votfmgr.h"
#include "is-hw-yuvpp.h"

static int __nocfi is_hw_isp_open(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct is_hw_isp *hw_isp = NULL;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, "HWISP");
	frame_manager_open(hw_ip->framemgr, IS_MAX_HW_FRAME, false);

	hw_ip->priv_info = vzalloc(sizeof(struct is_hw_isp));
	if (!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	ret = CALL_HW_HELPER_OPS(hw_ip, open, instance, &hw_isp->lib[instance],
				LIB_FUNC_ISP);
	if (ret)
		goto err_chain_create;

	set_bit(HW_OPEN, &hw_ip->state);
	msdbg_hw(2, "open: framemgr[%s]", instance, hw_ip, hw_ip->framemgr->name);

	return 0;

err_chain_create:
	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
err_alloc:
	frame_manager_close(hw_ip->framemgr);
	return ret;
}

static int is_hw_isp_init(struct is_hw_ip *hw_ip, u32 instance,
			bool flag, u32 f_type)
{
	int ret = 0;
	struct is_hw_isp *hw_isp;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);

	hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	hw_isp->param_set[instance].reprocessing = flag;

	ret = CALL_HW_HELPER_OPS(hw_ip, init, instance, &hw_isp->lib[instance],
				(u32)flag, f_type, LIB_FUNC_ISP);
	if (ret)
		return ret;

	set_bit(HW_INIT, &hw_ip->state);
	return ret;
}

static int is_hw_isp_deinit(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_isp *hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, deinit, instance, &hw_isp->lib[instance]);
}

static int is_hw_isp_close(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct is_hw_isp *hw_isp;

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	CALL_HW_HELPER_OPS(hw_ip, close, instance, &hw_isp->lib[instance]);
	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
	frame_manager_close(hw_ip->framemgr);

	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int is_hw_isp_enable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	set_bit(HW_RUN, &hw_ip->state);

	return ret;
}

static int is_hw_isp_disable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	int ret = 0;
	long timetowait;
	struct is_hw_isp *hw_isp;
	struct isp_param_set *param_set;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	msinfo_hw("isp_disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	FIMC_BUG(!hw_ip->priv_info);
	hw_isp = (struct is_hw_isp *)hw_ip->priv_info;
	param_set = &hw_isp->param_set[instance];

	timetowait = wait_event_timeout(hw_ip->status.wait_queue,
		!atomic_read(&hw_ip->status.Vvalid),
		IS_HW_STOP_TIMEOUT);

	if (!timetowait) {
		mserr_hw("wait FRAME_END timeout (%ld)", instance,
			hw_ip, timetowait);
		ret = -ETIME;
	}

	param_set->fcount = 0;
	CALL_HW_HELPER_OPS(hw_ip, disable, instance, &hw_isp->lib[instance]);

	if (hw_ip->run_rsc_state)
		return 0;

	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

int is_hw_isp_set_yuv_range(struct is_hw_ip *hw_ip,
	struct isp_param_set *param_set, u32 fcount, ulong hw_map)
{
	int ret = 0;
	struct is_hw_ip *hw_ip_mcsc = NULL;
	struct is_hw_mcsc *hw_mcsc = NULL;
	enum is_hardware_id hw_id = DEV_HW_END;
	int yuv_range = 0; /* 0: FULL, 1: NARROW */

#if !defined(USE_YUV_RANGE_BY_ISP)
	return 0;
#endif
	if (test_bit(DEV_HW_MCSC0, &hw_map))
		hw_id = DEV_HW_MCSC0;
	else if (test_bit(DEV_HW_MCSC1, &hw_map))
		hw_id = DEV_HW_MCSC1;

	hw_ip_mcsc = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_ip, hw_id);
	if (hw_ip_mcsc) {
		FIMC_BUG(!hw_ip_mcsc->priv_info);
		hw_mcsc = (struct is_hw_mcsc *)hw_ip_mcsc->priv_info;
		yuv_range = hw_mcsc->yuv_range;
	}

	if (yuv_range == SCALER_OUTPUT_YUV_RANGE_NARROW) {
		switch (param_set->otf_output.format) {
		case OTF_OUTPUT_FORMAT_YUV444:
			param_set->otf_output.format = OTF_OUTPUT_FORMAT_YUV444_TRUNCATED;
			break;
		case OTF_OUTPUT_FORMAT_YUV422:
			param_set->otf_output.format = OTF_OUTPUT_FORMAT_YUV422_TRUNCATED;
			break;
		default:
			break;
		}

		switch (param_set->dma_output_yuv.format) {
		case DMA_INOUT_FORMAT_YUV444:
			param_set->dma_output_yuv.format = DMA_INOUT_FORMAT_YUV444_TRUNCATED;
			break;
		case DMA_INOUT_FORMAT_YUV422:
			param_set->dma_output_yuv.format = DMA_INOUT_FORMAT_YUV422_TRUNCATED;
			break;
		default:
			break;
		}
	}

	dbg_hw(2, "[%d][F:%d]%s: OTF[%d]%s(%d), DMA[%d]%s(%d)\n",
		param_set->instance_id, fcount, __func__,
		param_set->otf_output.cmd,
		(param_set->otf_output.format >= OTF_OUTPUT_FORMAT_YUV444_TRUNCATED ? "N" : "W"),
		param_set->otf_output.format,
		param_set->dma_output_yuv.cmd,
		(param_set->dma_output_yuv.format >= DMA_INOUT_FORMAT_YUV444_TRUNCATED ? "N" : "W"),
		param_set->dma_output_yuv.format);

	return ret;
}

static void is_hw_isp_update_param(struct is_hw_ip *hw_ip, struct is_param_region *p_region,
	struct isp_param_set *param_set, IS_DECLARE_PMAP(pmap), u32 instance)
{
	struct isp_param *param;

	param = &p_region->isp;
	param_set->instance_id = instance;

	/* check input */
	if (test_bit(PARAM_ISP_OTF_INPUT, pmap))
		memcpy(&param_set->otf_input, &param->otf_input,
				sizeof(struct param_otf_input));

	if (test_bit(PARAM_ISP_VDMA1_INPUT, pmap))
		memcpy(&param_set->dma_input, &param->vdma1_input,
				sizeof(struct param_dma_input));

#if IS_ENABLED(SOC_TNR_MERGER)
	if (test_bit(PARAM_ISP_VDMA2_INPUT, pmap))
		memcpy(&param_set->prev_wgt_dma_input, &param->vdma2_input,
				sizeof(struct param_dma_input));
#if IS_ENABLED(USE_MCFP_MOTION_INTERFACE)
	if (test_bit(PARAM_ISP_MOTION_DMA_INPUT, pmap))
		memcpy(&param_set->motion_dma_input, &param->motion_dma_input,
				sizeof(struct param_dma_input));

	if (test_bit(PARAM_ISP_DRC_INPUT, pmap))
		memcpy(&param_set->dma_input_drcgrid, &param->drc_input,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_NRB_OUTPUT, pmap)) {
		/* TODO: add */
	}
#endif
#if defined(ENABLE_RGB_REPROCESSING)
	if (test_bit(PARAM_ISP_RGB_INPUT, pmap))
		memcpy(&param_set->dma_input_rgb, &param->rgb_input,
				sizeof(struct param_dma_input));

	if (test_bit(PARAM_ISP_NOISE_INPUT, pmap))
		memcpy(&param_set->dma_input_noise, &param->noise_input,
				sizeof(struct param_dma_input));

	if (test_bit(PARAM_ISP_NOISE_REP_OUTPUT, pmap))
		memcpy(&param_set->dma_output_noise_rep, &param->noise_rep_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_RGB_OUTPUT, pmap))
		memcpy(&param_set->dma_output_rgb, &param->rgb_output,
				sizeof(struct param_dma_output));
#endif
#if defined(ENABLE_SC_MAP)
	if (test_bit(PARAM_ISP_SCMAP_INPUT, pmap))
		memcpy(&param_set->dma_input_scmap, &param->scmap_input,
				sizeof(struct param_dma_input));
#endif
#if IS_ENABLED(SOC_YPP)
	if (test_bit(PARAM_ISP_VDMA4_OUTPUT, pmap))
		memcpy(&param_set->dma_output_yuv, &param->vdma4_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_VDMA5_OUTPUT, pmap))
		memcpy(&param_set->dma_output_rgb, &param->vdma5_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_NRDS_OUTPUT, pmap))
		memcpy(&param_set->dma_output_nrds, &param->nrds_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_NOISE_OUTPUT, pmap))
		memcpy(&param_set->dma_output_noise, &param->noise_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_DRC_OUTPUT, pmap))
		memcpy(&param_set->dma_output_drc, &param->drc_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_HIST_OUTPUT, pmap))
		memcpy(&param_set->dma_output_hist, &param->hist_output,
				sizeof(struct param_dma_output));
#else
	if (test_bit(PARAM_ISP_VDMA4_OUTPUT, pmap))
		memcpy(&param_set->dma_output_chunk, &param->vdma4_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_VDMA5_OUTPUT, pmap))
		memcpy(&param_set->dma_output_yuv, &param->vdma5_output,
				sizeof(struct param_dma_output));
#endif
	if (test_bit(PARAM_ISP_VDMA6_OUTPUT, pmap))
		memcpy(&param_set->dma_output_tnr_prev, &param->vdma6_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_VDMA7_OUTPUT, pmap))
		memcpy(&param_set->dma_output_tnr_wgt, &param->vdma7_output,
				sizeof(struct param_dma_output));

#if IS_ENABLED(USE_HF_INTERFACE)
	if (test_bit(PARAM_ISP_HF_OUTPUT, pmap))
		memcpy(&param_set->dma_output_hf, &param->hf_output,
				sizeof(struct param_dma_output));
#endif
#else
	if (test_bit(PARAM_ISP_VDMA4_OUTPUT, pmap))
		memcpy(&param_set->dma_output_chunk, &param->vdma4_output,
				sizeof(struct param_dma_output));

	if (test_bit(PARAM_ISP_VDMA5_OUTPUT, pmap))
		memcpy(&param_set->dma_output_yuv, &param->vdma5_output,
				sizeof(struct param_dma_output));
#endif /* SOC_TNR_MERGER */

	if (test_bit(PARAM_ISP_VDMA3_INPUT, pmap))
		memcpy(&param_set->prev_dma_input, &param->vdma3_input,
				sizeof(struct param_dma_input));

	/* check output*/
	if (test_bit(PARAM_ISP_OTF_OUTPUT, pmap))
		memcpy(&param_set->otf_output, &param->otf_output,
				sizeof(struct param_otf_output));

	if (IS_ENABLED(CHAIN_STRIPE_PROCESSING) && test_bit(PARAM_ISP_STRIPE_INPUT, pmap))
		memcpy(&param_set->stripe_input, &param->stripe_input,
				sizeof(struct param_stripe_input));
}

static int is_hw_isp_shot(struct is_hw_ip *hw_ip, struct is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	int cur_idx, batch_num;
	struct is_hw_isp *hw_isp;
	struct isp_param_set *param_set;
	struct is_param_region *p_region;
	struct isp_param *param;
	struct is_frame *dma_frame;
	u32 fcount, instance;
	u32 cmd_input, cmd_tnr_prev_in, cmd_yuv, cmd_rgb_out;
#if IS_ENABLED(SOC_TNR_MERGER)
	u32 cmd_tnr_wgt_in, cmd_tnr_prev_out, cmd_tnr_wgt_out;
#if IS_ENABLED(USE_MCFP_MOTION_INTERFACE)
	u32 cmd_motion, cmd_drcgrid;
#endif
#if defined(ENABLE_RGB_REPROCESSING)
	u32 cmd_rgb_in, cmd_noise_in, cmd_noise_rep;
#endif
#if defined(ENABLE_SC_MAP)
	u32 cmd_scmap;
#endif
#if IS_ENABLED(SOC_YPP)
	struct is_group *group;
	u32 en_votf;
	u32 cmd_nrds, cmd_noise_out, cmd_drc, cmd_hist, cmd_hf;
#endif
#endif /* SOC_TNR_MERGER */

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	instance = frame->instance;
	msdbgs_hw(2, "[F:%d]shot\n", instance, hw_ip, frame->fcount);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	set_bit(hw_ip->id, &frame->core_flag);

	FIMC_BUG(!hw_ip->priv_info);
	hw_isp = (struct is_hw_isp *)hw_ip->priv_info;
	param_set = &hw_isp->param_set[instance];
	p_region = frame->parameter;

	param = &p_region->isp;
	fcount = frame->fcount;
	cur_idx = frame->cur_buf_index;

	if (frame->type == SHOT_TYPE_INTERNAL) {
		/* OTF INPUT case */
		cmd_input = param_set->dma_input.cmd;
		param_set->dma_input.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->input_dva[0] = 0x0;

#if IS_ENABLED(SOC_TNR_MERGER)
		cmd_tnr_prev_in = param_set->prev_dma_input.cmd;
		param_set->prev_dma_input.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->input_dva_tnr_prev[0] = 0x0;

		cmd_tnr_wgt_in = param_set->prev_wgt_dma_input.cmd;
		param_set->prev_wgt_dma_input.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->input_dva_tnr_wgt[0] = 0x0;

		cmd_tnr_prev_out = param_set->dma_output_tnr_prev.cmd;
		param_set->dma_output_tnr_prev.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->output_dva_tnr_prev[0] = 0x0;

		cmd_tnr_wgt_out = param_set->dma_output_tnr_wgt.cmd;
		param_set->prev_wgt_dma_input.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->input_dva_tnr_wgt[0] = 0x0;
#if IS_ENABLED(USE_MCFP_MOTION_INTERFACE)
		cmd_motion = param_set->motion_dma_input.cmd;
		param_set->motion_dma_input.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->input_dva_motion[0] = 0x0;

		cmd_drcgrid = param_set->dma_input_drcgrid.cmd;
		param_set->dma_input_drcgrid.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->input_dva_drcgrid[0] = 0x0;
#endif
#if defined(ENABLE_RGB_REPROCESSING)
		cmd_rgb_in = param_set->dma_input_rgb.cmd;
		param_set->dma_input_rgb.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->input_dva_rgb[0] = 0x0;

		cmd_noise_in = param_set->dma_input_noise.cmd;
		param_set->dma_input_noise.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->input_dva_noise[0] = 0x0;

		cmd_noise_rep = param_set->dma_output_noise_rep.cmd;
		param_set->dma_output_noise_rep.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->output_dva_noise_rep[0] = 0x0;
#endif
#if defined(ENABLE_SC_MAP)
		cmd_scmap = param_set->dma_input_scmap.cmd;
		param_set->dma_input_scmap.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->input_dva_scmap[0] = 0x0;
#endif
#if IS_ENABLED(SOC_YPP)
		cmd_nrds = param_set->dma_output_nrds.cmd;
		param_set->dma_output_nrds.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_nrds[0] = 0x0;

		cmd_noise_out = param_set->dma_output_noise.cmd;
		param_set->dma_output_noise.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_noise[0] = 0x0;

		cmd_drc = param_set->dma_output_drc.cmd;
		param_set->dma_output_drc.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_drc[0] = 0x0;

		cmd_hist = param_set->dma_output_hist.cmd;
		param_set->dma_output_hist.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_hist[0] = 0x0;

		cmd_hf = param_set->dma_output_hf.cmd;
		param_set->dma_output_hf.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_hf[0] = 0x0;

		cmd_rgb_out = param_set->dma_output_rgb.cmd;
		param_set->dma_output_rgb.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_rgb[0] = 0x0;

#else
		cmd_rgb_out = param_set->dma_output_chunk.cmd;
		param_set->dma_output_chunk.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_chunk[0] = 0x0;
#endif

		param_set->tnr_mode = TNR_PROCESSING_PREVIEW_POST_ON;
#else
		cmd_tnr_prev_in = param_set->prev_dma_input.cmd;
		param_set->prev_dma_input.cmd = DMA_INPUT_COMMAND_DISABLE;
		param_set->tnr_prev_input_dva[0] = 0x0;

		cmd_rgb_out = param_set->dma_output_chunk.cmd;
		param_set->dma_output_chunk.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_chunk[0] = 0x0;
#endif /* SOC_TNR_MERGER */
		cmd_yuv = param_set->dma_output_yuv.cmd;
		param_set->dma_output_yuv.cmd  = DMA_OUTPUT_COMMAND_DISABLE;
		param_set->output_dva_yuv[0] = 0x0;


		hw_ip->internal_fcount[instance] = fcount;
		goto config;
	} else {
		FIMC_BUG(!frame->shot);

		if (hw_ip->internal_fcount[instance] != 0) {
			hw_ip->internal_fcount[instance] = 0;
#if IS_ENABLED(SOC_TNR_MERGER) && IS_ENABLED(SOC_YPP)
			param_set->dma_output_yuv.cmd = param->vdma4_output.cmd;
			param_set->dma_output_rgb.cmd  = param->vdma5_output.cmd;
#else
			param_set->dma_output_chunk.cmd = param->vdma4_output.cmd;
			param_set->dma_output_yuv.cmd  = param->vdma5_output.cmd;
#endif
		}

		/*set TNR operation mode */
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

		if (param_set->otf_input.cmd == OTF_INPUT_COMMAND_ENABLE) {
			struct is_hw_ip *hw_ip_3aa = NULL;
			struct is_hw_3aa *hw_3aa = NULL;
			enum is_hardware_id hw_id = DEV_HW_END;

			if (test_bit(DEV_HW_3AA0, &hw_map))
				hw_id = DEV_HW_3AA0;
			else if (test_bit(DEV_HW_3AA1, &hw_map))
				hw_id = DEV_HW_3AA1;
			else if (test_bit(DEV_HW_3AA2, &hw_map))
				hw_id = DEV_HW_3AA2;
			else if (test_bit(DEV_HW_3AA3, &hw_map))
				hw_id = DEV_HW_3AA3;

			hw_ip_3aa = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_ip, hw_id);
			if (hw_ip_3aa) {
				FIMC_BUG(!hw_ip_3aa->priv_info);
				hw_3aa = (struct is_hw_3aa *)hw_ip_3aa->priv_info;
				param_set->taa_param = &hw_3aa->param_set[instance];
				/*
				 * When the ISP shot is requested, DDK needs to know the size fo 3AA.
				 * This is because DDK calculates the position of the cropped image
				 * from the 3AA size.
				 */
				is_hw_3aa_update_param(hw_ip,
						p_region, param_set->taa_param,
						frame->pmap, instance);
			}
		}
	}

	is_hw_isp_update_param(hw_ip, p_region, param_set, frame->pmap, instance);

	/* DMA settings */
	dma_frame = frame;

	cmd_input = CALL_HW_OPS(hw_ip, dma_cfg, "ixs", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_input.cmd,
			param_set->dma_input.plane,
			param_set->input_dva,
			dma_frame->dvaddr_buffer);

	/* Slave I/O */
#if IS_ENABLED(SOC_TNR_MERGER)
	/* TNR prev image input */
	cmd_tnr_prev_in = CALL_HW_OPS(hw_ip, dma_cfg, "tnr_prev_in", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->prev_dma_input.cmd,
			param_set->prev_dma_input.plane,
			param_set->input_dva_tnr_prev,
			dma_frame->ixtTargetAddress);

	/* TNR prev weight input */
	cmd_tnr_wgt_in = CALL_HW_OPS(hw_ip, dma_cfg, "tnr_wgt_in", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->prev_wgt_dma_input.cmd,
			param_set->prev_wgt_dma_input.plane,
			param_set->input_dva_tnr_wgt,
			dma_frame->ixgTargetAddress);

	cmd_tnr_prev_out = CALL_HW_OPS(hw_ip, dma_cfg, "tnr_prev_out", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_tnr_prev.cmd,
			param_set->dma_output_tnr_prev.plane,
			param_set->output_dva_tnr_prev,
			dma_frame->ixvTargetAddress);

	cmd_tnr_wgt_out = CALL_HW_OPS(hw_ip, dma_cfg, "tnr_wgt_out", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_tnr_wgt.cmd,
			param_set->dma_output_tnr_wgt.plane,
			param_set->output_dva_tnr_wgt,
			dma_frame->ixwTargetAddress);
#if IS_ENABLED(USE_MCFP_MOTION_INTERFACE)
	cmd_motion = CALL_HW_OPS(hw_ip, dma_cfg, "motion", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->motion_dma_input.cmd,
			param_set->motion_dma_input.plane,
			param_set->input_dva_motion,
			dma_frame->ixmTargetAddress);

	if (CHECK_NEED_KVADDR_LVN_ID(IS_VIDEO_IMM_NUM))
		CALL_HW_OPS(hw_ip, dma_cfg_kva64, "motion", hw_ip,
			frame, cur_idx,
			&param_set->motion_dma_input.cmd,
			param_set->motion_dma_input.plane,
			param_set->input_kva_motion,
			dma_frame->ixmKTargetAddress);

	cmd_drcgrid = CALL_HW_OPS(hw_ip, dma_cfg, "drcgrid", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_input_drcgrid.cmd,
			param_set->dma_input_drcgrid.plane,
			param_set->input_dva_drcgrid,
			dma_frame->ixdgrTargetAddress);
#endif
#if defined(ENABLE_RGB_REPROCESSING)
	cmd_rgb_in = CALL_HW_OPS(hw_ip, dma_cfg, "rgb_in", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_input_rgb.cmd,
			param_set->dma_input_rgb.plane,
			param_set->input_dva_rgb,
			dma_frame->ixrrgbTargetAddress);

	cmd_noise_in = CALL_HW_OPS(hw_ip, dma_cfg, "noise_in", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_input_noise.cmd,
			param_set->dma_input_noise.plane,
			param_set->input_dva_noise,
			dma_frame->ixnoirTargetAddress);

	cmd_noise_rep = CALL_HW_OPS(hw_ip, dma_cfg, "noise_rep", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_noise_rep.cmd,
			param_set->dma_output_noise_rep.plane,
			param_set->output_dva_noise_rep,
			dma_frame->ixnoirwTargetAddress);
#endif
#if defined(ENABLE_SC_MAP)
	cmd_scmap = CALL_HW_OPS(hw_ip, dma_cfg, "scmap", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_input_scmap.cmd,
			param_set->dma_input_scmap.plane,
			param_set->input_dva_scmap,
			dma_frame->ixscmapTargetAddress);
#endif
#if IS_ENABLED(SOC_YPP)
	group = hw_ip->group[instance];
	en_votf = param_set->dma_output_yuv.v_otf_enable;
	if (en_votf == OTF_INPUT_COMMAND_ENABLE)
		dma_frame = is_votf_get_frame(group, TWS,
				group->next->leader.id, 0);

	cmd_yuv = CALL_HW_OPS(hw_ip, dma_cfg, "yuv", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_yuv.cmd,
			param_set->dma_output_yuv.plane,
			param_set->output_dva_yuv,
			dma_frame->ixcTargetAddress);

	cmd_nrds = CALL_HW_OPS(hw_ip, dma_cfg, "nrds", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_nrds.cmd,
			param_set->dma_output_nrds.plane,
			param_set->output_dva_nrds,
			dma_frame->ixnrdsTargetAddress);

	cmd_noise_out = CALL_HW_OPS(hw_ip, dma_cfg, "noise_out", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_noise.cmd,
			param_set->dma_output_noise.plane,
			param_set->output_dva_noise,
			dma_frame->ixnoiTargetAddress);

	cmd_drc = CALL_HW_OPS(hw_ip, dma_cfg, "drc", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_drc.cmd,
			param_set->dma_output_drc.plane,
			param_set->output_dva_drc,
			dma_frame->ixdgaTargetAddress);

	cmd_hist = CALL_HW_OPS(hw_ip, dma_cfg, "hist", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_hist.cmd,
			param_set->dma_output_hist.plane,
			param_set->output_dva_hist,
			dma_frame->ixsvhistTargetAddress);

	dma_frame = frame;
	cmd_hf = CALL_HW_OPS(hw_ip, dma_cfg, "hf", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_hf.cmd,
			param_set->dma_output_hf.plane,
			param_set->output_dva_hf,
			dma_frame->ixhfTargetAddress);

	cmd_rgb_out = CALL_HW_OPS(hw_ip, dma_cfg, "rgb_out", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_rgb.cmd,
			param_set->dma_output_rgb.plane,
			param_set->output_dva_rgb,
			dma_frame->ixwrgbTargetAddress);

	if (en_votf) {
		struct is_hw_ip *hw_ip_ypp;
		struct is_hw_ypp *hw_ypp;

		hw_ip_ypp = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_ip, DEV_HW_YPP);

		FIMC_BUG(!hw_ip_ypp->priv_info);
		hw_ypp = (struct is_hw_ypp *)hw_ip_ypp->priv_info;

		if (hw_ypp->param_set[instance].dma_input_lv2.cmd == DMA_INPUT_COMMAND_DISABLE)
			param_set->dma_output_nrds.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		else
			param_set->dma_output_nrds.cmd = DMA_OUTPUT_COMMAND_ENABLE;

		if (hw_ypp->param_set[instance].dma_input_drc.cmd == DMA_INPUT_COMMAND_DISABLE)
			param_set->dma_output_drc.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		else
			param_set->dma_output_drc.cmd = DMA_OUTPUT_COMMAND_ENABLE;

		if (hw_ypp->param_set[instance].dma_input_hist.cmd == DMA_INPUT_COMMAND_DISABLE)
			param_set->dma_output_hist.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		else
			param_set->dma_output_hist.cmd = DMA_OUTPUT_COMMAND_ENABLE;

		if (hw_ypp->param_set[instance].dma_input_noise.cmd == DMA_INPUT_COMMAND_DISABLE)
			param_set->dma_output_noise.cmd = DMA_OUTPUT_COMMAND_DISABLE;
		else
			param_set->dma_output_noise.cmd = DMA_OUTPUT_COMMAND_ENABLE;
	}
#else
	cmd_yuv = CALL_HW_OPS(hw_ip, dma_cfg, "yuv", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_yuv.cmd,
			param_set->dma_output_yuv.plane,
			param_set->output_dva_yuv,
			dma_frame->ixpTargetAddress);

	cmd_rgb_out = CALL_HW_OPS(hw_ip, dma_cfg, "chk", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_chunk.cmd,
			param_set->dma_output_chunk.plane,
			param_set->output_dva_chunk,
			dma_frame->ixcTargetAddress);
#endif

#else
	cmd_tnr_prev_in = CALL_HW_OPS(hw_ip, dma_cfg, "tnr_prev_in", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->prev_dma_input.cmd,
			param_set->prev_dma_input.plane,
			param_set->tnr_prev_input_dva,
			dma_frame->ixtTargetAddress);

	cmd_yuv = CALL_HW_OPS(hw_ip, dma_cfg, "yuv", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_yuv.cmd,
			param_set->dma_output_yuv.plane,
			param_set->output_dva_yuv,
			dma_frame->ixpTargetAddress);

	cmd_rgb_out = CALL_HW_OPS(hw_ip, dma_cfg, "chk", hw_ip,
			frame, cur_idx, frame->num_buffers,
			&param_set->dma_output_chunk.cmd,
			param_set->dma_output_chunk.plane,
			param_set->output_dva_chunk,
			dma_frame->ixcTargetAddress);
#endif /* SOC_TNR_MERGER */

config:
	param_set->instance_id = instance;
	param_set->fcount = fcount;

	/* multi-buffer */
	hw_ip->num_buffers = frame->num_buffers;
	batch_num = hw_ip->framemgr->batch_num;
	/* Update only for SW_FRO scenario */
	if (batch_num > 1 && hw_ip->num_buffers == 1) {
		hw_ip->num_buffers |= batch_num << SW_FRO_NUM_SHIFT;
		hw_ip->num_buffers |= cur_idx << CURR_INDEX_SHIFT;
	}

	if (frame->shot) {
		ret = is_lib_isp_set_ctrl(hw_ip, &hw_isp->lib[instance], frame);
		if (ret)
			mserr_hw("set_ctrl fail", instance, hw_ip);
	}

	ret = is_hw_isp_set_yuv_range(hw_ip, param_set, frame->fcount, hw_map);
	ret |= is_lib_isp_shot(hw_ip, &hw_isp->lib[instance], param_set, frame->shot);

	/* Restore CMDs in param_set. */
	param_set->dma_input.cmd = cmd_input;
	param_set->prev_dma_input.cmd = cmd_tnr_prev_in;
#if IS_ENABLED(SOC_TNR_MERGER)
	param_set->prev_wgt_dma_input.cmd = cmd_tnr_wgt_in;
	param_set->dma_output_tnr_prev.cmd = cmd_tnr_prev_out;
	param_set->dma_output_tnr_wgt.cmd = cmd_tnr_wgt_out;
#if IS_ENABLED(USE_MCFP_MOTION_INTERFACE)
	param_set->motion_dma_input.cmd = cmd_motion;
	param_set->dma_input_drcgrid.cmd = cmd_drcgrid;
#endif
#if defined(ENABLE_RGB_REPROCESSING)
	param_set->dma_input_rgb.cmd = cmd_rgb_in;
	param_set->dma_input_noise.cmd = cmd_noise_in;
	param_set->dma_output_noise_rep.cmd = cmd_noise_rep;
#endif
#if defined(ENABLE_SC_MAP)
	param_set->dma_input_scmap.cmd = cmd_scmap;
#endif
#if IS_ENABLED(SOC_YPP)
	param_set->dma_output_nrds.cmd = cmd_nrds;
	param_set->dma_output_noise.cmd = cmd_noise_out;
	param_set->dma_output_drc.cmd = cmd_drc;
	param_set->dma_output_hist.cmd = cmd_hist;
	param_set->dma_output_hf.cmd = cmd_hf;
	param_set->dma_output_rgb.cmd = cmd_rgb_out;
#else
	param_set->dma_output_chunk.cmd = cmd_rgb_out;
#endif
#else
	param_set->dma_output_chunk.cmd = cmd_rgb_out;
#endif /* SOC_TNR_MERGER */
	param_set->dma_output_yuv.cmd = cmd_yuv;

	set_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static int is_hw_isp_set_param(struct is_hw_ip *hw_ip, struct is_region *region,
		IS_DECLARE_PMAP(pmap), u32 instance, ulong hw_map)
{
	struct is_hw_isp *hw_isp;
	struct isp_param_set *param_set;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	FIMC_BUG(!hw_ip->priv_info);
	hw_isp = (struct is_hw_isp *)hw_ip->priv_info;
	param_set = &hw_isp->param_set[instance];

	hw_ip->region[instance] = region;

	is_hw_isp_update_param(hw_ip, &region->parameter, param_set, pmap, instance);

	return 0;
}

static int is_hw_isp_get_meta(struct is_hw_ip *hw_ip, struct is_frame *frame,
	ulong hw_map)
{
	int ret;
	struct is_hw_isp *hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	ret = CALL_HW_HELPER_OPS(hw_ip, get_meta, frame->instance,
				frame, &hw_isp->lib[frame->instance]);
	if (frame->shot) {
		msdbg_hw(2, "%s: [F:%d], %d,%d,%d\n", frame->instance, hw_ip, __func__,
			frame->fcount,
			frame->shot->udm.ni.currentFrameNoiseIndex,
			frame->shot->udm.ni.nextFrameNoiseIndex,
			frame->shot->udm.ni.nextNextFrameNoiseIndex);
	}

	return ret;
}

static int is_hw_isp_frame_ndone(struct is_hw_ip *hw_ip, struct is_frame *frame,
		enum ShotErrorType done_type)
{
	int output_id = 0;
	int ret = 0;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	if (test_bit(hw_ip->id, &frame->core_flag))
		output_id = IS_HW_CORE_END;

	ret = CALL_HW_OPS(hw_ip, frame_done, hw_ip, frame, -1,
			output_id, done_type, false);

	return ret;
}

static int is_hw_isp_load_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_isp *hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, load_setfile, instance, &hw_isp->lib[instance]);
}

static int is_hw_isp_apply_setfile(struct is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	struct is_hw_isp *hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, apply_setfile, instance, &hw_isp->lib[instance],
				scenario);
}

static int is_hw_isp_delete_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_isp *hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	return CALL_HW_HELPER_OPS(hw_ip, delete_setfile, instance, &hw_isp->lib[instance]);
}


int is_hw_isp_restore(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct is_hw_isp *hw_isp = NULL;

	BUG_ON(!hw_ip);
	BUG_ON(!hw_ip->priv_info);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return -EINVAL;

	hw_isp = (struct is_hw_isp *)hw_ip->priv_info;

	ret = is_lib_isp_reset_recovery(hw_ip, &hw_isp->lib[instance], instance);
	if (ret) {
		mserr_hw("is_lib_isp_reset_recovery fail ret(%d)",
				instance, hw_ip, ret);
	}

	return ret;
}

static int is_hw_isp_notify_timeout(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_hw_isp *hw_isp;

	hw_isp = (struct is_hw_isp *)hw_ip->priv_info;
	if (!hw_isp) {
		mserr_hw("failed to get HW ISP", instance, hw_ip);
		return -ENODEV;
	}

	return CALL_HW_HELPER_OPS(hw_ip, notify_timeout, instance, &hw_isp->lib[instance]);
}

const struct is_hw_ip_ops is_hw_isp_ops = {
	.open			= is_hw_isp_open,
	.init			= is_hw_isp_init,
	.deinit			= is_hw_isp_deinit,
	.close			= is_hw_isp_close,
	.enable			= is_hw_isp_enable,
	.disable		= is_hw_isp_disable,
	.shot			= is_hw_isp_shot,
	.set_param		= is_hw_isp_set_param,
	.get_meta		= is_hw_isp_get_meta,
	.frame_ndone		= is_hw_isp_frame_ndone,
	.load_setfile		= is_hw_isp_load_setfile,
	.apply_setfile		= is_hw_isp_apply_setfile,
	.delete_setfile		= is_hw_isp_delete_setfile,
	.restore		= is_hw_isp_restore,
	.notify_timeout		= is_hw_isp_notify_timeout,
};

int is_hw_isp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name)
{
	int ret = 0;

	hw_ip->ops  = &is_hw_isp_ops;

	return ret;
}

void is_hw_isp_remove(struct is_hw_ip *hw_ip)	{ }
