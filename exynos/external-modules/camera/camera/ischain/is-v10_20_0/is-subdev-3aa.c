// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-devicemgr.h"
#include "is-device-sensor.h"
#include "is-subdev-ctrl.h"
#include "is-config.h"
#include "is-param.h"
#include "is-video.h"
#include "is-type.h"
#include "is-hw.h"

void is_ischain_3aa_stripe_cfg(struct is_subdev *subdev,
		struct is_frame *frame,
		u32 *full_w, u32 *full_h,
		struct is_crop *incrop,
		struct is_crop *otcrop)
{
	u32 stripe_x, stripe_w;
	u32 region_id = frame->stripe_info.region_id;
	/* Input crop configuration */
	if (!region_id) {
		/**
		 * Left region
		 * The stripe width should be in h_pix_num.
		 */
		stripe_x = otcrop->x;
		stripe_w = ALIGN_DOWN(frame->stripe_info.in.h_pix_num[region_id] - stripe_x, STRIPE_WIDTH_ALIGN);
		*full_w = frame->stripe_info.in.h_pix_num[region_id];

		frame->stripe_info.in.h_pix_ratio = stripe_w * STRIPE_RATIO_PRECISION / otcrop->w;
		frame->stripe_info.in.h_pix_num[region_id] = stripe_w + stripe_x;
	} else {
		/* Right region */
		stripe_x = 0;
		stripe_w = otcrop->w - (frame->stripe_info.in.h_pix_num[region_id - 1] - otcrop->x);
		*full_w = *full_w - frame->stripe_info.in.h_pix_num[region_id - 1];
	}

	/* Add stripe processing horizontal margin into each region. */
	stripe_w += STRIPE_MARGIN_WIDTH;
	*full_w += STRIPE_MARGIN_WIDTH;

	incrop->x = stripe_x;
	incrop->y = otcrop->y;
	incrop->w = stripe_w;
	incrop->h = otcrop->h;

	/* Output crop configuration */
	if (!frame->stripe_info.region_id) {
		/* Left region */
		stripe_x = 0;
		stripe_w = frame->stripe_info.in.h_pix_num[region_id] - otcrop->x;

		frame->stripe_info.out.h_pix_ratio = stripe_w * STRIPE_RATIO_PRECISION / otcrop->w;
		frame->stripe_info.out.h_pix_num[region_id] = stripe_w;
	} else {
		/* Right region */
		stripe_x = 0;
		stripe_w = otcrop->w - frame->stripe_info.out.h_pix_num[region_id - 1];
	}

	/* Add stripe processing horizontal margin into each region. */
	stripe_w += STRIPE_MARGIN_WIDTH;

	otcrop->x = stripe_x;
	otcrop->y = 0;
	otcrop->w = stripe_w;

	msrdbgs(3, "stripe_in_crop[%d][%d, %d, %d, %d, %d, %d]\n", subdev, subdev, frame,
			region_id,
			incrop->x, incrop->y, incrop->w, incrop->h, *full_w, *full_h);
	msrdbgs(3, "stripe_ot_crop[%d][%d, %d, %d, %d]\n", subdev, subdev, frame,
			region_id,
			otcrop->x, otcrop->y, otcrop->w, otcrop->h);
}

void __tap_out_stripe_cfg(struct is_frame *ldr_frame,
		struct camera2_node *node,
		struct is_crop *otcrop,
		u32 bitwidth,
		int index)
{
	struct is_stripe_info *stripe_info;
	u32 stripe_x, stripe_w, dma_offset = 0;
	u32 region_id = ldr_frame->stripe_info.region_id;

	stripe_info = &ldr_frame->cap_node.sframe[index].stripe_info;
	/* Output crop & WDMA offset configuration */
	if (!ldr_frame->stripe_info.region_id) {
		/* Left region */
		stripe_x = otcrop->x;
		stripe_w = ldr_frame->stripe_info.out.h_pix_num[region_id];

		stripe_info->out.h_pix_ratio = stripe_w * STRIPE_RATIO_PRECISION / otcrop->w;
		stripe_info->out.h_pix_num[region_id] = stripe_w;
	} else {
		/* Right region */
		stripe_x = 0;
		stripe_w = otcrop->w - stripe_info->out.h_pix_num[region_id - 1];

		/**
		 * 3AA writes the right region with stripe margin.
		 * Add horizontal & vertical DMA offset.
		 */
		dma_offset = stripe_info->out.h_pix_num[region_id - 1] + STRIPE_MARGIN_WIDTH;
		dma_offset = dma_offset * node->pixelsize / BITS_PER_BYTE;
		dma_offset *= otcrop->h;
	}

	stripe_w += STRIPE_MARGIN_WIDTH;

	otcrop->x = stripe_x;
	otcrop->w = stripe_w;

	ldr_frame->cap_node.sframe[index].dva[0] += dma_offset;

	mlvdbgs(3, "[F%d] stripe_ot_crop[%d][%d, %d, %d, %d] offset %x\n", ldr_frame, node->vid,
			ldr_frame->fcount, region_id,
			otcrop->x, otcrop->y, otcrop->w, otcrop->h, dma_offset);
}

static int __ischain_taa_slot(struct camera2_node *node, u32 *pindex)
{
	int ret = 0;

	switch (node->vid) {
	case IS_VIDEO_30C_NUM:
	case IS_VIDEO_31C_NUM:
	case IS_VIDEO_32C_NUM:
		*pindex = PARAM_3AA_VDMA4_OUTPUT;
		ret = 2;
		break;
	case IS_VIDEO_30P_NUM:
	case IS_VIDEO_31P_NUM:
	case IS_VIDEO_32P_NUM:
		*pindex = PARAM_3AA_VDMA2_OUTPUT;
		ret = 2;
		break;
	case IS_VIDEO_30F_NUM:
	case IS_VIDEO_31F_NUM:
	case IS_VIDEO_32F_NUM:
		*pindex = PARAM_3AA_FDDMA_OUTPUT;
		ret = 2;
		break;
	case IS_VIDEO_30G_NUM:
	case IS_VIDEO_31G_NUM:
	case IS_VIDEO_32G_NUM:
		*pindex = PARAM_3AA_MRGDMA_OUTPUT;
		ret = 2;
		break;
	case IS_VIDEO_30D_NUM:
	case IS_VIDEO_31D_NUM:
	case IS_VIDEO_32D_NUM:
		*pindex = PARAM_3AA_DRCG_OUTPUT;
		ret = 2;
		break;
	case IS_VIDEO_30O_NUM:
	case IS_VIDEO_31O_NUM:
	case IS_VIDEO_32O_NUM:
		*pindex = PARAM_3AA_ORBDS_OUTPUT;
		ret = 2;
		break;
	default:
		*pindex = 0;
		ret = 0;
		break;
	}

	return ret;
}

static int __taa_dma_in_cfg(struct is_device_ischain *device,
	struct is_subdev *leader,
	struct is_frame *ldr_frame,
	struct camera2_node *node,
	u32 pindex,
	IS_DECLARE_PMAP(pmap))
{
	struct is_fmt *fmt;
	struct param_dma_input *dma_input;
	struct param_otf_input *otf_input;
	struct param_otf_output *otf_output;
	struct param_stripe_input *stripe_input;
	struct param_sensor_config *sensor_config;
	struct is_crop *incrop, *otcrop;
	struct param_control *control;
	struct is_group *parent;
	struct is_group *group;
	struct is_device_sensor *sensor;
	struct is_crop incrop_cfg, otcrop_cfg;
	u32 in_width, in_height;
	int ret = 0;
	struct is_queue *queue;
	u32 otf_input_bitwidth = OTF_INPUT_BIT_WIDTH_10BIT;

	FIMC_BUG(!device);
	FIMC_BUG(!ldr_frame);
	FIMC_BUG(!node);
	FIMC_BUG(!leader);

	sensor = device->sensor;
	group = container_of(leader, struct is_group, leader);

	dma_input = is_itf_g_param(device, ldr_frame, pindex);
	if (test_bit(IS_GROUP_OTF_INPUT, &group->state)) {
		dma_input->cmd = DMA_INPUT_COMMAND_DISABLE;
	} else {
		if (dma_input->cmd != node->request)
			mlvinfo("[F%d] RDMA enable: %d -> %d\n", device,
				node->vid, ldr_frame->fcount,
				dma_input->cmd, node->request);
		dma_input->cmd = DMA_INPUT_COMMAND_ENABLE;
	}

	set_bit(pindex, pmap);

	if (!node->request)
		return 0;

	incrop = (struct is_crop *)node->input.cropRegion;
	if (IS_NULL_CROP(incrop)) {
		mlverr("incrop is NULL", device, node->vid);
		return -EINVAL;
	}

	otcrop = (struct is_crop *)node->output.cropRegion;
	if (IS_NULL_CROP(otcrop)) {
		mlverr("otcrop is NULL", device, node->vid);
		return -EINVAL;
	}
	incrop_cfg = *incrop;
	otcrop_cfg = *otcrop;

	queue = GET_SUBDEV_QUEUE(leader);
	if (!queue) {
		merr("queue is NULL", device);
		ret = -EINVAL;
	}

	if ((sensor->cfg[sensor->nfi_toggle] &&
		    sensor->cfg[sensor->nfi_toggle]->input[CSI_VIRTUAL_CH_0].hwformat ==
			    HW_FORMAT_RAW12) ||
		(sensor->obte_config & BIT(14))) /* Deprecated definition: EX_12BIT == 14 */
		otf_input_bitwidth = OTF_INPUT_BIT_WIDTH_14BIT;

	/* Configure Conrtol */
	control = is_itf_g_param(device, NULL, PARAM_3AA_CONTROL);
	if (test_bit(IS_GROUP_START, &group->state)) {
		control->cmd = CONTROL_COMMAND_START;
		control->bypass = CONTROL_BYPASS_DISABLE;
	} else {
		control->cmd = CONTROL_COMMAND_STOP;
		control->bypass = CONTROL_BYPASS_DISABLE;
	}

	set_bit(PARAM_3AA_CONTROL, pmap);

	/*
	 * When 3AA is belonged at Reprocessing stream
	 * it should refer pdp rdma input size
	 * When 3AA is belonged at Preview stream
	 * it should refer sensor input size
	 */
	if (test_bit(IS_ISCHAIN_REPROCESSING, &device->state)) {
		in_width = leader->input.width;
		in_height = leader->input.height;
	} else {
		in_width = is_sensor_g_bns_width(device->sensor);
		in_height = is_sensor_g_bns_height(device->sensor);
	}

	if (IS_ENABLED(CHAIN_STRIPE_PROCESSING) &&
		ldr_frame && ldr_frame->stripe_info.region_num)
		is_ischain_3aa_stripe_cfg(leader, ldr_frame,
				&in_width, &in_height,
				&incrop_cfg, &otcrop_cfg);

	fmt = is_find_format(node->pixelformat, node->flags);
	if (!fmt) {
		mlverr("pixel format(0x%x) is not found", device,
			node->vid, node->pixelformat);
		return -EINVAL;
	}

	if (!test_bit(IS_GROUP_OTF_INPUT, &group->state)) {
		if (dma_input->format != fmt->hw_format)
			mlvinfo("[F%d]RDMA format: %d -> %d\n", device,
				node->vid, ldr_frame->fcount,
				dma_input->format, fmt->hw_format);
		if (dma_input->bitwidth != fmt->hw_bitwidth)
			mlvinfo("[F%d]RDMA bitwidth: %d -> %d\n", device,
				node->vid, ldr_frame->fcount,
				dma_input->bitwidth, fmt->hw_bitwidth);
		if ((dma_input->dma_crop_width != incrop->w) ||
			(dma_input->dma_crop_height != incrop->h))
			mlvinfo("[F%d]RDMA bcrop[%d, %d, %d, %d]\n",
				device, node->vid, ldr_frame->fcount,
				incrop->x, incrop->y, incrop->w, incrop->h);
	}

	dma_input->format = fmt->hw_format;
	dma_input->bitwidth = fmt->hw_bitwidth;
	dma_input->msb = fmt->bitsperpixel[0] - 1;/* msb zero padding by HW constraint */
	dma_input->order = DMA_INOUT_ORDER_GR_BG;
	dma_input->plane = 1;
	dma_input->width = in_width;
	dma_input->height = in_height;
#if defined(ENABLE_3AA_DMA_CROP)
	dma_input->dma_crop_offset = (incrop_cfg.x << 16) | (incrop_cfg.y << 0);
	dma_input->dma_crop_width = incrop_cfg.w;
	dma_input->dma_crop_height = incrop_cfg.h;
	dma_input->bayer_crop_offset_x = 0;
	dma_input->bayer_crop_offset_y = 0;
#else
	dma_input->dma_crop_offset = 0;
	dma_input->dma_crop_width = leader->input.width;
	dma_input->dma_crop_height = leader->input.height;
	dma_input->bayer_crop_offset_x = incrop_cfg.x;
	dma_input->bayer_crop_offset_y = incrop_cfg.y;
#endif
	dma_input->bayer_crop_width = incrop_cfg.w;
	dma_input->bayer_crop_height = incrop_cfg.h;

	if (pindex != PARAM_3AA_VDMA1_INPUT)
		goto p_skip;

	sensor_config = is_itf_g_param(device, NULL, PARAM_SENSOR_CONFIG);
	otf_input = is_itf_g_param(device, ldr_frame, PARAM_3AA_OTF_INPUT);
	if ((otf_input->bayer_crop_width != incrop->w) ||
		(otf_input->bayer_crop_height != incrop->h))
		mlvinfo("[F%d]OTF in bcrop[%d, %d, %d, %d]\n",
			device, node->vid, ldr_frame->fcount,
			incrop->x, incrop->y,
			incrop->w, incrop->h);

	parent = group->parent;
	if (parent) {
		if (test_bit(IS_GROUP_OTF_INPUT, &parent->state)) {
			if (test_bit(IS_GROUP_VOTF_INPUT, &parent->state)) {
				if (otf_input->source != OTF_INPUT_PAF_VOTF_PATH)
					mlvinfo("[F%d] otf_input->source: %d -> %d\n",
						device, node->vid, ldr_frame->fcount,
						otf_input->source, OTF_INPUT_PAF_VOTF_PATH);
				otf_input->source = OTF_INPUT_PAF_VOTF_PATH;
			} else {
				if (otf_input->source != OTF_INPUT_SENSOR_PATH)
					mlvinfo("[F%d] otf_input->source: %d -> %d\n",
						device, node->vid, ldr_frame->fcount,
						otf_input->source, OTF_INPUT_SENSOR_PATH);
				otf_input->source = OTF_INPUT_SENSOR_PATH;
			}
		} else {
			otf_input->source = OTF_INPUT_PAF_RDMA_PATH;
		}
	}
	/*
	 * bayer crop = bcrop1 + bcrop3
	 * hal should set full size input including cac margin
	 * and then full size is decreased as cac margin by driver internally
	 * size of 3AP take over only setting for BDS
	 */
	otf_input->cmd = OTF_INPUT_COMMAND_ENABLE;	/* in 9830, 3AA has only OTF input */
	otf_input->width = in_width;
	otf_input->height = in_height;
	otf_input->format = OTF_INPUT_FORMAT_BAYER;
	otf_input->bitwidth = otf_input_bitwidth;
	otf_input->order = sensor_config->pixel_order;
	otf_input->bayer_crop_offset_x = incrop_cfg.x;
	otf_input->bayer_crop_offset_y = incrop_cfg.y;
	otf_input->bayer_crop_width = incrop_cfg.w;
	otf_input->bayer_crop_height = incrop_cfg.h;

	set_bit(PARAM_3AA_OTF_INPUT, pmap);

	otf_output = is_itf_g_param(device, ldr_frame, PARAM_3AA_OTF_OUTPUT);
	if ((otf_output->width != otcrop->w) ||
		(otf_output->height != otcrop->h))
		mlvinfo("[F%d]OTF out bcrop[%d, %d, %d, %d]\n",
			device, node->vid, ldr_frame->fcount,
			otcrop->x, otcrop->y,
			otcrop->w, otcrop->h);

	/* TODO: The size of OTF_OUTPUT and DMA_OUTPUT(BDS) should be same,
	 * when the OTF_OUTPUT and DMA_OUTPUT(BDS) are enabled.  */
	if (test_bit(IS_GROUP_OTF_OUTPUT, &group->state))
		otf_output->cmd = OTF_OUTPUT_COMMAND_ENABLE;
	else
		otf_output->cmd = OTF_OUTPUT_COMMAND_DISABLE;
#ifdef USE_3AA_CROP_AFTER_BDS
	if (!test_bit(IS_ISCHAIN_REPROCESSING, &device->state)) {
		otf_output->width = otcrop_cfg.w;
		otf_output->height = otcrop_cfg.h;
		otf_output->crop_enable = 0;
	} else {
		otf_output->width = incrop_cfg.w;
		otf_output->height = incrop_cfg.h;
		otf_output->crop_offset_x = otcrop_cfg.x;
		otf_output->crop_offset_y = otcrop_cfg.y;
		otf_output->crop_width = otcrop_cfg.w;
		otf_output->crop_height = otcrop_cfg.h;
		otf_output->crop_enable = 1;
	}
#else
	otf_output->width = otcrop_cfg.w;
	otf_output->height = otcrop_cfg.h;
	otf_output->crop_enable = 0;
#endif
	otf_output->format = OTF_OUTPUT_FORMAT_BAYER;
	otf_output->bitwidth = OTF_INPUT_BIT_WIDTH_12BIT;
	otf_output->order = OTF_OUTPUT_ORDER_BAYER_GR_BG;

	set_bit(PARAM_3AA_OTF_OUTPUT, pmap);

	stripe_input = is_itf_g_param(device, ldr_frame, PARAM_3AA_STRIPE_INPUT);
	if (ldr_frame && ldr_frame->stripe_info.region_num) {
		stripe_input->index = ldr_frame->stripe_info.region_id;
		stripe_input->total_count = ldr_frame->stripe_info.region_num;
		if (!ldr_frame->stripe_info.region_id) {
			stripe_input->left_margin = 0;
			stripe_input->right_margin = STRIPE_MARGIN_WIDTH;
		} else {
			stripe_input->left_margin = STRIPE_MARGIN_WIDTH;
			stripe_input->right_margin = 0;
		}
		stripe_input->full_width = leader->input.width;
		stripe_input->full_height = leader->input.height;
	} else {
		stripe_input->index = 0;
		stripe_input->total_count = 0;
		stripe_input->left_margin = 0;
		stripe_input->right_margin = 0;
		stripe_input->full_width = 0;
		stripe_input->full_height = 0;
	}

	set_bit(PARAM_3AA_STRIPE_INPUT, pmap);

p_skip:
	leader->input.crop = *incrop;

	return ret;
}

static int __taa_dma_out_cfg(struct is_device_ischain *device,
	struct is_subdev *leader,
	struct is_frame *ldr_frame,
	struct camera2_node *node,
	u32 pindex,
	IS_DECLARE_PMAP(pmap),
	int index)
{
	struct is_fmt *fmt;
	struct param_dma_output *dma_output;
	struct param_otf_input *otf_input;
	struct is_crop *incrop, *otcrop;
	u32 hw_format = DMA_INOUT_FORMAT_BAYER;
	u32 hw_bitwidth = DMA_INOUT_BIT_WIDTH_16BIT;
	u32 hw_sbwc = DMA_INPUT_SBWC_DISABLE;
	u32 hw_msb, hw_order, flag_extra, flag_pixel_size;
	u32 hw_channel = 0;
	struct is_crop otcrop_cfg;
	int ret = 0;

	FIMC_BUG(!ldr_frame);
	FIMC_BUG(!node);

	dma_output = is_itf_g_param(device, ldr_frame, pindex);
	if (dma_output->cmd != node->request)
		mlvinfo("[F%d][%d] WDMA enable: %d -> %d\n", device,
			node->vid, ldr_frame->fcount, pindex,
			dma_output->cmd, node->request);
	dma_output->cmd = node->request;

	set_bit(pindex, pmap);

	incrop = (struct is_crop *)node->input.cropRegion;
	otcrop = (struct is_crop *)node->output.cropRegion;
	if (IS_NULL_CROP(otcrop)) {
		mlverr("[F%d][%d] otcrop is NULL (%d, %d, %d, %d), disable DMA",
			device, node->vid, ldr_frame->fcount, pindex,
			otcrop->x, otcrop->y, otcrop->w, otcrop->h);
		otcrop->x = otcrop->y = 0;
		otcrop->w = leader->input.width;
		otcrop->h = leader->input.height;
		dma_output->cmd = DMA_OUTPUT_COMMAND_DISABLE;
	}

	if (IS_NULL_CROP(incrop)) {
		mlverr("[F%d][%d] incrop is NULL (%d, %d, %d, %d), disable DMA",
			device, node->vid, ldr_frame->fcount, pindex,
			incrop->x, incrop->y, incrop->w, incrop->h);
		incrop->x = incrop->y = 0;
		incrop->w = leader->input.width;
		incrop->h = leader->input.height;
		dma_output->cmd = DMA_OUTPUT_COMMAND_DISABLE;
	}
	otcrop_cfg = *otcrop;

	fmt = is_find_format(node->pixelformat, node->flags);
	if (!fmt) {
		mlverr("[F%d][%d] pixelformat(%c%c%c%c) is not found",
			device, node->vid, ldr_frame->fcount, pindex,
			(char)((node->pixelformat >> 0) & 0xFF),
			(char)((node->pixelformat >> 8) & 0xFF),
			(char)((node->pixelformat >> 16) & 0xFF),
			(char)((node->pixelformat >> 24) & 0xFF));
		return -EINVAL;
	}
	hw_format = fmt->hw_format;
	hw_bitwidth = fmt->hw_bitwidth;
	hw_msb = fmt->bitsperpixel[0] - 1;
	hw_order = fmt->hw_order;

	/* pixel type [0:5] : pixel size, [6:7] : extra */
	flag_pixel_size = node->flags & PIXEL_TYPE_SIZE_MASK;
	flag_extra = (node->flags & PIXEL_TYPE_EXTRA_MASK) >> PIXEL_TYPE_EXTRA_SHIFT;

	if (hw_format == DMA_INOUT_FORMAT_BAYER_PACKED
		&& flag_pixel_size == CAMERA_PIXEL_SIZE_13BIT) {
		hw_bitwidth = DMA_INOUT_BIT_WIDTH_13BIT;
		hw_msb = DMA_INOUT_BIT_WIDTH_13BIT - 1;
	} else if (hw_format == DMA_INOUT_FORMAT_BAYER) {
		/* Except TAG node, it uses signed bayer format. */
		if (!CHECK_TAG_ID(node->vid))
			hw_msb = fmt->bitsperpixel[0];
	}

	if (flag_extra)
		hw_sbwc = (SBWC_BASE_ALIGN_MASK | flag_extra);

	/* DRC and HF WDMA have own format enum */
	if (CHECK_TAD_ID(node->vid)) {
		hw_format = DMA_INOUT_FORMAT_DRCGRID;
		hw_bitwidth = DMA_INOUT_BIT_WIDTH_32BIT;
		hw_msb = DMA_INOUT_BIT_WIDTH_32BIT - 1;
		hw_order = DMA_INOUT_ORDER_NO;
	} else if (CHECK_TAH_ID(node->vid)) {
		hw_format = DMA_INOUT_FORMAT_HF;
		hw_bitwidth = DMA_INOUT_BIT_WIDTH_8BIT;
		hw_msb = DMA_INOUT_BIT_WIDTH_8BIT - 1;
		hw_order = DMA_INOUT_ORDER_NO;
		hw_channel = node->vid - IS_VIDEO_30H_NUM;
	} else if (CHECK_TAP_ID(node->vid)) {
		if (IS_ENABLED(CHAIN_STRIPE_PROCESSING) &&
				ldr_frame->stripe_info.region_num)
			__tap_out_stripe_cfg(ldr_frame, node,
				&otcrop_cfg, hw_bitwidth, index);
	}

	if (dma_output->format != hw_format)
		mlvinfo("[F%d][%d] WDMA format: %d -> %d\n", device,
			node->vid, ldr_frame->fcount, pindex,
			dma_output->format, hw_format);
	if (dma_output->bitwidth != hw_bitwidth)
		mlvinfo("[F%d][%d] WDMA bitwidth: %d -> %d\n", device,
			node->vid, ldr_frame->fcount, pindex,
			dma_output->bitwidth, hw_bitwidth);
	if (dma_output->sbwc_type != hw_sbwc)
		mlvinfo("[F%d][%d] WDMA sbwc_type: %d -> %d\n", device,
			node->vid, ldr_frame->fcount, pindex,
			dma_output->sbwc_type, hw_sbwc);
	if (!ldr_frame->stripe_info.region_num && ((dma_output->width != otcrop->w) ||
		(dma_output->height != otcrop->h)) && !CHECK_TAF_ID(node->vid))
		mlvinfo("[F%d][%d] WDMA otcrop[%d, %d, %d, %d]\n", device,
			node->vid, ldr_frame->fcount, pindex,
			otcrop->x, otcrop->y, otcrop->w, otcrop->h);

	otf_input = &ldr_frame->parameter->taa.otf_input;
	if (CHECK_TAP_ID(node->vid) || CHECK_TAF_ID(node->vid)) {
		if ((otcrop_cfg.w > otf_input->bayer_crop_width) ||
			(otcrop_cfg.h > otf_input->bayer_crop_height)) {
			merr("bds output size is invalid((%d, %d) != (%d, %d))",
				device,
				otcrop_cfg.w,
				otcrop_cfg.h,
				otf_input->bayer_crop_width,
				otf_input->bayer_crop_height);
			return -EINVAL;
		}
	} else if (CHECK_TAC_ID(node->vid)) {
		if ((otcrop_cfg.w != otf_input->bayer_crop_width) ||
			(otcrop_cfg.h != otf_input->bayer_crop_height)) {
			merr("bds output size is invalid((%d, %d) != (%d, %d))",
				device,
				otcrop_cfg.w,
				otcrop_cfg.h,
				otf_input->bayer_crop_width,
				otf_input->bayer_crop_height);
			return -EINVAL;
		}
	}

	/*
	 * 3AA BDS ratio limitation on width, height
	 * ratio = input * 256 / output
	 * real output = input * 256 / ratio
	 * real output &= ~1
	 * real output is same with output crop
	 */
	dma_output->format = hw_format;
	dma_output->bitwidth = hw_bitwidth;
	dma_output->msb = hw_msb;
	dma_output->order = hw_order;
	dma_output->plane = fmt->hw_plane;
	dma_output->sbwc_type = hw_sbwc;
	dma_output->channel = hw_channel;
#ifdef USE_3AA_CROP_AFTER_BDS
	if (!test_bit(IS_ISCHAIN_REPROCESSING, &device->state)) {
		if (otcrop->x || otcrop->y)
			mwarn("crop pos(%d, %d) is ignored", device, otcrop->x, otcrop->y);

		dma_output->width = otcrop_cfg.w;
		dma_output->height = otcrop_cfg.h;
		dma_output->stride_plane0 = max(node->width, otcrop_cfg.w);
		dma_output->stride_plane1 = max(node->width, otcrop_cfg.w);
		dma_output->stride_plane2 = max(node->width, otcrop_cfg.w);
		if (CHECK_TAF_ID(node->vid)) {
			dma_output->crop_enable = 1;
			dma_output->dma_crop_offset_x = incrop->x;
			dma_output->dma_crop_offset_y = incrop->y;
			dma_output->dma_crop_width = incrop->w;
			dma_output->dma_crop_height = incrop->h;
		} else {
			dma_output->crop_enable = 0;
		}
	} else {
		if (CHECK_TAP_ID(node->vid)) {
			dma_output->width = otf_input->bayer_crop_width;
			dma_output->height = otf_input->bayer_crop_height;
			dma_output->crop_enable = 1;
			dma_output->dma_crop_offset_x = otcrop_cfg.x;
			dma_output->dma_crop_offset_y = otcrop_cfg.y;
			dma_output->dma_crop_width = otcrop_cfg.w;
			dma_output->dma_crop_height = otcrop_cfg.h;
		} else {
			dma_output->width = otcrop_cfg.w;
			dma_output->height = otcrop_cfg.h;
			dma_output->crop_enable = 0;
			dma_output->dma_crop_offset_x = incrop->x;
			dma_output->dma_crop_offset_y = incrop->y;
			dma_output->dma_crop_width = incrop->w;
			dma_output->dma_crop_height = incrop->h;
			dma_output->stride_plane0 = max(node->width, incrop->w);
			dma_output->stride_plane1 = max(node->width, incrop->w);
			dma_output->stride_plane2 = max(node->width, incrop->w);
		}
	}
#else
	if (otcrop->x || otcrop->y)
		mwarn("crop pos(%d, %d) is ignored", device, otcrop->x, otcrop->y);

	dma_output->width = otcrop_cfg.w;
	dma_output->height = otcrop_cfg.h;
	dma_output->crop_enable = 0;
#endif

	return ret;
}

static int is_ischain_3aa_tag(struct is_subdev *subdev,
	void *device_data,
	struct is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct taa_param *taa_param;
	struct is_crop *incrop, *otcrop;
	struct is_subdev *leader;
	struct is_device_ischain *device;
	IS_DECLARE_PMAP(pmap);
	struct is_queue *leader_queue, *subdev_queue;
	struct is_fmt *leader_format, *subdev_format;
	bool chg_fmt_size = false;
	struct is_queue *queue;
	struct camera2_node *out_node = NULL;
	struct camera2_node *cap_node = NULL;
	struct is_sub_frame *sframe;
	u32 pindex = 0;
	int i, dma_type;
	struct is_param_region *param_region;
	u32 calibrated_width, calibrated_height;
	u32 sensor_binning_ratio_x, sensor_binning_ratio_y;
	u32 binned_sensor_width, binned_sensor_height;

	device = (struct is_device_ischain *)device_data;

	FIMC_BUG(!subdev);
	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!frame);

	mdbgs_ischain(4, "3AA TAG\n", device);

	incrop = (struct is_crop *)node->input.cropRegion;
	otcrop = (struct is_crop *)node->output.cropRegion;

	leader = subdev->leader;
	IS_INIT_PMAP(pmap);
	taa_param = &device->is_region->parameter.taa;

	if (test_bit(IS_ISCHAIN_REPROCESSING, &device->state)) {
		leader_queue = GET_SUBDEV_QUEUE(leader);
		leader_format = leader_queue->framecfg.format;
		subdev_queue = GET_SUBDEV_QUEUE(subdev);
		subdev_format = subdev_queue->framecfg.format;

		if (!subdev_format || (leader_format != subdev_format) ||
			(leader_queue->framecfg.hw_pixeltype != subdev_queue->framecfg.hw_pixeltype)) {
			subdev_queue->framecfg.format = leader_format;
			subdev_queue->framecfg.hw_pixeltype = leader_queue->framecfg.hw_pixeltype;
			chg_fmt_size = true;
		}
	}

	if (IS_ENABLED(LOGICAL_VIDEO_NODE)) {
		queue = GET_SUBDEV_QUEUE(leader);
		if (!queue) {
			merr("queue is NULL", device);
			ret = -EINVAL;
			goto p_err;
		}

		out_node = &frame->shot_ext->node_group.leader;
		ret = __taa_dma_in_cfg(device, subdev, frame, out_node,
				PARAM_3AA_VDMA1_INPUT, pmap);
		if (ret) {
			mrerr("__taa_dma_in_cfg is fail(%d)", device, frame, ret);
			return ret;
		}

		out_node->result = 1;

		for (i = 0; i < CAPTURE_NODE_MAX; i++) {
			cap_node = &frame->shot_ext->node_group.capture[i];
			if (!cap_node->vid)
				continue;

			/* TODO: consider Sensor and PDP */
			sframe = &frame->cap_node.sframe[i];
			if ((sframe->id >= IS_VIDEO_SS0VC0_NUM) &&
					(sframe->id <= IS_VIDEO_PAF3S_NUM))
				continue;

			dma_type = __ischain_taa_slot(cap_node, &pindex);
			if (dma_type == 1) {
				ret = __taa_dma_in_cfg(device, subdev, frame, cap_node,
						pindex, pmap);
				if (ret) {
					mrerr("__taa_dma_in_cfg is fail(%d)", device, frame, ret);
					return ret;
				}
			} else if (dma_type == 2) {
				ret = __taa_dma_out_cfg(device, subdev, frame, cap_node,
						pindex, pmap, i);
				if (ret) {
					mrerr("__taa_dma_out_cfg is fail(%d)", device, frame, ret);
					return ret;
				}
			}

			cap_node->result = 1;
		}

		ret = is_itf_s_param(device, frame, pmap);
		if (ret) {
			mrerr("is_itf_s_param is fail(%d)", device, frame, ret);
			goto p_err;
		}

		param_region = &device->is_region->parameter;

		calibrated_width = param_region->sensor.config.calibrated_width;
		calibrated_height = param_region->sensor.config.calibrated_height;
		sensor_binning_ratio_x = param_region->sensor.config.sensor_binning_ratio_x;
		sensor_binning_ratio_y = param_region->sensor.config.sensor_binning_ratio_y;
		binned_sensor_width = calibrated_width * 1000 / sensor_binning_ratio_x;
		binned_sensor_height = calibrated_height * 1000 / sensor_binning_ratio_y;

		frame->shot->udm.frame_info.sensor_size[0] = calibrated_width;
		frame->shot->udm.frame_info.sensor_size[1] = calibrated_height;
		frame->shot->udm.frame_info.sensor_binning[0] = sensor_binning_ratio_x;
		frame->shot->udm.frame_info.sensor_binning[1] = sensor_binning_ratio_y;
		frame->shot->udm.frame_info.sensor_crop_region[0] =
			((binned_sensor_width - param_region->sensor.config.width) >> 1) & (~0x1);
		frame->shot->udm.frame_info.sensor_crop_region[1] =
			((binned_sensor_height - param_region->sensor.config.height) >> 1) & (~0x1);
		frame->shot->udm.frame_info.sensor_crop_region[2] = param_region->sensor.config.width;
		frame->shot->udm.frame_info.sensor_crop_region[3] = param_region->sensor.config.height;
		frame->shot->udm.frame_info.taa_in_crop_region[0] = incrop->x;
		frame->shot->udm.frame_info.taa_in_crop_region[1] = incrop->y;
		frame->shot->udm.frame_info.taa_in_crop_region[2] = incrop->w;
		frame->shot->udm.frame_info.taa_in_crop_region[3] = incrop->h;

		return ret;
	}

p_err:
	return ret;
}

static const struct is_subdev_ops is_subdev_3aa_ops = {
	.bypass			= NULL,
	.cfg			= NULL,
	.tag			= is_ischain_3aa_tag,
};

const struct is_subdev_ops *pablo_get_is_subdev_3aa_ops(void)
{
	return &is_subdev_3aa_ops;
}
KUNIT_EXPORT_SYMBOL(pablo_get_is_subdev_3aa_ops);
