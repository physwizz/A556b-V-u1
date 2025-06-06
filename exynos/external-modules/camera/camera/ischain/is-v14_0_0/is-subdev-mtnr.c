// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2023 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/videodev2_exynos_media.h>
#include "is-device-ischain.h"
#include "is-device-sensor.h"
#include "is-subdev-ctrl.h"
#include "is-config.h"
#include "is-param.h"
#include "is-video.h"
#include "is-type.h"
#include "is-stripe.h"
#include "is-hw.h"
#include "pablo-hw-chain-info.h"
#include "is-core.h"

static int __ischain_mtnr_slot(struct camera2_node *node, u32 *pindex)
{
	int ret;

	switch (node->vid) {
	/* MTNR0 */
	case IS_VIDEO_MTNR:
		*pindex = PARAM_MTNR_RDMA_CUR_L0;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_CUR_L4:
		*pindex = PARAM_MTNR_RDMA_CUR_L4;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_PREV_L0:
		*pindex = PARAM_MTNR_RDMA_PREV_L0;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_PREV_L0_WGT:
		*pindex = PARAM_MTNR_RDMA_PREV_L0_WGT;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_MV_GEOMATCH:
		*pindex = PARAM_MTNR_RDMA_MV_GEOMATCH;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_SEG_L0:
		*pindex = PARAM_MTNR_RDMA_SEG_L0;
		ret = 1;
		break;
	case IS_LVN_MTNR_CAPTURE_PREV_L0:
		*pindex = PARAM_MTNR_WDMA_PREV_L0;
		ret = 2;
		break;
	case IS_LVN_MTNR_CAPTURE_PREV_L0_WGT:
		*pindex = PARAM_MTNR_WDMA_PREV_L0_WGT;
		ret = 2;
		break;
	/* MTNR1 */
	case IS_LVN_MTNR_OUTPUT_CUR_L1:
		*pindex = PARAM_MTNR_RDMA_CUR_L1;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_CUR_L2:
		*pindex = PARAM_MTNR_RDMA_CUR_L2;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_CUR_L3:
		*pindex = PARAM_MTNR_RDMA_CUR_L3;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_PREV_L1:
		*pindex = PARAM_MTNR_RDMA_PREV_L1;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_PREV_L2:
		*pindex = PARAM_MTNR_RDMA_PREV_L2;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_PREV_L3:
		*pindex = PARAM_MTNR_RDMA_PREV_L3;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_PREV_L4:
		*pindex = PARAM_MTNR_RDMA_PREV_L4;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_PREV_L1_WGT:
		*pindex = PARAM_MTNR_RDMA_PREV_L1_WGT;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_SEG_L1:
		*pindex = PARAM_MTNR_RDMA_SEG_L1;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_SEG_L2:
		*pindex = PARAM_MTNR_RDMA_SEG_L2;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_SEG_L3:
		*pindex = PARAM_MTNR_RDMA_SEG_L3;
		ret = 1;
		break;
	case IS_LVN_MTNR_OUTPUT_SEG_L4:
		*pindex = PARAM_MTNR_RDMA_SEG_L4;
		ret = 1;
		break;
	case IS_LVN_MTNR_CAPTURE_PREV_L1:
		*pindex = PARAM_MTNR_WDMA_PREV_L1;
		ret = 2;
		break;
	case IS_LVN_MTNR_CAPTURE_PREV_L2:
		*pindex = PARAM_MTNR_WDMA_PREV_L2;
		ret = 2;
		break;
	case IS_LVN_MTNR_CAPTURE_PREV_L3:
		*pindex = PARAM_MTNR_WDMA_PREV_L3;
		ret = 2;
		break;
	case IS_LVN_MTNR_CAPTURE_PREV_L4:
		*pindex = PARAM_MTNR_WDMA_PREV_L4;
		ret = 2;
		break;
	case IS_LVN_MTNR_CAPTURE_PREV_L1_WGT:
		*pindex = PARAM_MTNR_WDMA_PREV_L1_WGT;
		ret = 2;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

static inline int __check_cropRegion(struct is_device_ischain *device, struct camera2_node *node)
{
	if (IS_NULL_CROP((struct is_crop *)node->input.cropRegion)) {
		mlverr("incrop is NULL", device, node->vid);
		return -EINVAL;
	}

	if (IS_NULL_CROP((struct is_crop *)node->output.cropRegion)) {
		mlverr("otcrop is NULL", device, node->vid);
		return -EINVAL;
	}

	return 0;
}

static int __mtnr_otf_out_cfg(struct is_device_ischain *device, struct is_subdev *leader,
	struct is_frame *ldr_frame, struct camera2_node *node, u32 pindex, IS_DECLARE_PMAP(pmap))
{
	int ret;
	struct param_otf_output *otf_output;
	struct is_crop *otcrop;
	struct is_group *group;
	struct is_crop otcrop_cfg;
	u32 width;
	u32 level = 0;

	set_bit(pindex, pmap);

	group = container_of(leader, struct is_group, leader);

	otf_output = is_itf_g_param(device, ldr_frame, pindex);
	if (test_bit(IS_GROUP_OTF_OUTPUT, &group->state))
		otf_output->cmd = OTF_OUTPUT_COMMAND_ENABLE;
	else
		otf_output->cmd = OTF_OUTPUT_COMMAND_DISABLE;

	if (!otf_output->cmd)
		return 0;

	ret = __check_cropRegion(device, node);
	if (ret)
		return ret;

	otcrop = (struct is_crop *)node->output.cropRegion;
	otcrop_cfg = *otcrop;

	switch (node->vid) {
	case IS_LVN_MTNR_OUTPUT_CUR_L1:
	case IS_LVN_MTNR_OUTPUT_CUR_L2:
	case IS_LVN_MTNR_OUTPUT_CUR_L3:
	case IS_LVN_MTNR_OUTPUT_CUR_L4:
		level = ZERO_IF_NEG(node->vid - IS_LVN_MTNR_OUTPUT_CUR_L1 + 1);
		break;
	default:
		break;
	}

	if (!ldr_frame->stripe_info.region_num &&
		((otf_output->width != otcrop->w) || (otf_output->height != otcrop->h)))
		mlvinfo("[F%d]OTF otcrop[%d, %d, %d, %d]\n", device, IS_VIDEO_MTNR,
			ldr_frame->fcount, otcrop->x, otcrop->y, otcrop->w, otcrop->h);

	if (IS_ENABLED(CHAIN_STRIPE_PROCESSING) && ldr_frame->stripe_info.region_num)
		width = ALIGN(ldr_frame->stripe_info.out.crop_width >> level, 2);
	else
		width = otcrop_cfg.w;

	otf_output->width = width;
	otf_output->height = otcrop_cfg.h;
	otf_output->format = OTF_YUV_FORMAT;
	otf_output->bitwidth = OTF_OUTPUT_BIT_WIDTH_12BIT;
	otf_output->order = OTF_OUTPUT_FORMAT_YUV422;

	return 0;
}

static int __mtnr_stripe_in_cfg(struct is_device_ischain *device, struct is_subdev *leader,
	struct is_frame *frame, struct camera2_node *node, u32 pindex, IS_DECLARE_PMAP(pmap))
{
	int ret;
	struct param_stripe_input *stripe_input;
	struct is_crop *otcrop;
	int i;

	set_bit(pindex, pmap);

	ret = __check_cropRegion(device, node);
	if (ret)
		return ret;

	otcrop = (struct is_crop *)node->output.cropRegion;

	stripe_input = is_itf_g_param(device, frame, pindex);
	if (frame->stripe_info.region_num) {
		stripe_input->index = frame->stripe_info.region_id;
		stripe_input->total_count = frame->stripe_info.region_num;
		if (!frame->stripe_info.region_id) {
			stripe_input->stripe_roi_start_pos_x[0] = 0;
			for (i = 1; i < frame->stripe_info.region_num; ++i)
				stripe_input->stripe_roi_start_pos_x[i] =
					frame->stripe_info.h_pix_num[i - 1];
		}

		stripe_input->left_margin = frame->stripe_info.out.left_margin;
		stripe_input->right_margin = frame->stripe_info.out.right_margin;
		stripe_input->full_width = otcrop->w;
		stripe_input->full_height = otcrop->h;
		stripe_input->start_pos_x =
			frame->stripe_info.out.crop_x + frame->stripe_info.out.offset_x;

#if defined(MTNR_STRIP_OTF_MARGIN_CROP)
		if (stripe_input->left_margin > MTNR_STRIP_OTF_MARGIN_CROP &&
			frame->stripe_info.out.crop_width > MTNR_STRIP_OTF_MARGIN_CROP) {
			frame->stripe_info.out.left_margin -= MTNR_STRIP_OTF_MARGIN_CROP;
			frame->stripe_info.out.crop_width -= MTNR_STRIP_OTF_MARGIN_CROP;
			frame->stripe_info.out.crop_x += MTNR_STRIP_OTF_MARGIN_CROP;
		}

		if (stripe_input->right_margin > MTNR_STRIP_OTF_MARGIN_CROP &&
			frame->stripe_info.out.crop_width > MTNR_STRIP_OTF_MARGIN_CROP) {
			frame->stripe_info.out.right_margin -= MTNR_STRIP_OTF_MARGIN_CROP;
			frame->stripe_info.out.crop_width -= MTNR_STRIP_OTF_MARGIN_CROP;
		}
#endif

	} else {
		stripe_input->index = 0;
		stripe_input->total_count = 0;
		stripe_input->left_margin = 0;
		stripe_input->right_margin = 0;
		stripe_input->full_width = 0;
		stripe_input->full_height = 0;
	}

	pablo_update_repeat_param(frame, stripe_input);

	return 0;
}

static int __mtnr_dma_in_cfg(struct is_device_ischain *device, struct is_subdev *leader,
	struct is_frame *ldr_frame, struct camera2_node *node, u32 pindex, IS_DECLARE_PMAP(pmap))
{
	int ret;
	struct is_fmt *fmt;
	struct param_dma_input *dma_input;
	struct is_crop *otcrop;
	struct is_group *group;
	u32 hw_format = DMA_INOUT_FORMAT_YUV422;
	u32 hw_bitwidth = DMA_INOUT_BIT_WIDTH_12BIT;
	u32 hw_sbwc = DMA_INPUT_SBWC_DISABLE;
	u32 hw_order, flag_extra, flag_pixel_size, hw_msb;
	u32 width;
	u32 level = 0;
	struct is_crop otcrop_cfg;

	set_bit(pindex, pmap);

	group = container_of(leader, struct is_group, leader);

	dma_input = is_itf_g_param(device, ldr_frame, pindex);

	if (pindex == PARAM_MTNR_RDMA_CUR_L0 && test_bit(IS_GROUP_OTF_INPUT, &group->state)) {
		dma_input->cmd = DMA_INPUT_COMMAND_DISABLE;
	} else {
		if (dma_input->cmd != node->request)
			mlvinfo("[F%d] RDMA enable: %d -> %d\n", device, node->vid,
				ldr_frame->fcount, dma_input->cmd, node->request);
		dma_input->cmd = node->request;

		if (!dma_input->cmd)
			return 0;
	}

	ret = __check_cropRegion(device, node);
	if (ret)
		return ret;

	otcrop = (struct is_crop *)node->output.cropRegion;
	otcrop_cfg = *otcrop;

	fmt = is_find_format(node->pixelformat, node->flags);
	if (!fmt) {
		merr("pixel format(0x%x) is not found", device, node->pixelformat);
		return -EINVAL;
	}

	hw_format = fmt->hw_format;
	hw_bitwidth = fmt->hw_bitwidth;
	hw_msb = ZERO_IF_NEG(fmt->bitsperpixel[0] - 1);
	hw_order = fmt->hw_order;
	/* pixel type [0:5] : pixel size, [6:7] : extra */
	flag_pixel_size = node->flags & PIXEL_TYPE_SIZE_MASK;
	flag_extra = (node->flags & PIXEL_TYPE_EXTRA_MASK) >> PIXEL_TYPE_EXTRA_SHIFT;

	if (flag_extra)
		hw_sbwc = (SBWC_BASE_ALIGN_MASK | flag_extra);

	switch (node->vid) {
	case IS_LVN_MTNR_OUTPUT_CUR_L1:
	case IS_LVN_MTNR_OUTPUT_CUR_L2:
	case IS_LVN_MTNR_OUTPUT_CUR_L3:
	case IS_LVN_MTNR_OUTPUT_CUR_L4:
		level = ZERO_IF_NEG(node->vid - IS_LVN_MTNR_OUTPUT_CUR_L1 + 1);
		break;
	case IS_LVN_MTNR_OUTPUT_MV_GEOMATCH:
	case IS_LVN_MTNR_OUTPUT_SEG_L0:
	case IS_LVN_MTNR_OUTPUT_SEG_L1:
	case IS_LVN_MTNR_OUTPUT_SEG_L2:
	case IS_LVN_MTNR_OUTPUT_SEG_L3:
	case IS_LVN_MTNR_OUTPUT_SEG_L4:
		hw_format = DMA_INOUT_FORMAT_Y;
		hw_bitwidth = DMA_INOUT_BIT_WIDTH_8BIT;
		hw_msb = ZERO_IF_NEG(hw_bitwidth - 1);
		hw_order = DMA_INOUT_ORDER_NO;
		break;
	default:
		break;
	}

	if (IS_ENABLED(CHAIN_STRIPE_PROCESSING) && ldr_frame->stripe_info.region_num &&
		(node->vid == IS_VIDEO_MTNR))
		is_ischain_g_stripe_cfg(ldr_frame, node, &otcrop_cfg, &otcrop_cfg, &otcrop_cfg);

	if (dma_input->cmd) {
		if (dma_input->format != hw_format)
			mlvinfo("[F%d]RDMA format: %d -> %d\n", device, node->vid,
				ldr_frame->fcount, dma_input->format, hw_format);
		if (dma_input->bitwidth != hw_bitwidth)
			mlvinfo("[F%d]RDMA bitwidth: %d -> %d\n", device, node->vid,
				ldr_frame->fcount, dma_input->bitwidth, hw_bitwidth);
		if (dma_input->sbwc_type != hw_sbwc)
			mlvinfo("[F%d]RDMA sbwc_type: %d -> %d\n", device, node->vid,
				ldr_frame->fcount, dma_input->sbwc_type, hw_sbwc);
		if (!ldr_frame->stripe_info.region_num &&
			((dma_input->width != otcrop_cfg.w) || (dma_input->height != otcrop_cfg.h)))
			mlvinfo("[F%d]RDMA incrop[%d, %d, %d, %d]\n", device, node->vid,
				ldr_frame->fcount, otcrop_cfg.x, otcrop_cfg.y, otcrop_cfg.w,
				otcrop_cfg.h);
	}

	if (IS_ENABLED(CHAIN_STRIPE_PROCESSING) && ldr_frame->stripe_info.region_num)
		width = ALIGN(ldr_frame->stripe_info.out.crop_width >> level, 2);
	else
		width = otcrop_cfg.w;

	dma_input->format = hw_format;
	dma_input->bitwidth = hw_bitwidth;
	dma_input->msb = hw_msb;
	dma_input->sbwc_type = hw_sbwc;
	dma_input->order = hw_order;
	dma_input->plane = fmt->hw_plane;
	dma_input->width = width;
	dma_input->height = otcrop_cfg.h;
	dma_input->dma_crop_offset = (otcrop_cfg.x << 16) | (otcrop_cfg.y << 0);
	dma_input->dma_crop_width = otcrop_cfg.w;
	dma_input->dma_crop_height = otcrop_cfg.h;
	dma_input->bayer_crop_offset_x = 0;
	dma_input->bayer_crop_offset_y = 0;
	dma_input->bayer_crop_width = otcrop_cfg.w;
	dma_input->bayer_crop_height = otcrop_cfg.h;
	dma_input->stride_plane0 = otcrop_cfg.w;
	dma_input->stride_plane1 = otcrop_cfg.w;
	dma_input->stride_plane2 = otcrop_cfg.w;

	return 0;
}

static inline bool __mtnr_check_wdma_prev(u32 pindex)
{
	bool ret = false;

	if (pindex == PARAM_MTNR_WDMA_PREV_L0 ||
		pindex == PARAM_MTNR_WDMA_PREV_L1 ||
		pindex == PARAM_MTNR_WDMA_PREV_L2 ||
		pindex == PARAM_MTNR_WDMA_PREV_L3 ||
		pindex == PARAM_MTNR_WDMA_PREV_L4)
		ret = true;

	return ret;
}

static inline void __mtnr_get_prev_l_size(u32 pindex, u32 w, u32 h, u32 *tw, u32 *th)
{
	switch (pindex) {
	case PARAM_MTNR_WDMA_PREV_L0:
		*tw = w;
		*th = h;
		break;
	case PARAM_MTNR_WDMA_PREV_L1:
		*tw = ALIGN(w / 2, 2);
		*th = ALIGN(h / 2, 2);
		break;
	case PARAM_MTNR_WDMA_PREV_L2:
		*tw = ALIGN(w / 4, 2);
		*th = ALIGN(h / 4, 2);
		break;
	case PARAM_MTNR_WDMA_PREV_L3:
		*tw = ALIGN(w / 8, 2);
		*th = ALIGN(h / 8, 2);
		break;
	case PARAM_MTNR_WDMA_PREV_L4:
		*tw = ALIGN(w / 16, 2);
		*th = ALIGN(h / 16, 2);
		break;
	}
}

static int __mtnr_dma_out_cfg(struct is_device_ischain *device, struct is_subdev *leader,
	struct is_frame *frame, struct camera2_node *node, u32 pindex, IS_DECLARE_PMAP(pmap),
	int index)
{
	struct is_fmt *fmt;
	struct param_dma_output *dma_output;
	struct is_crop *otcrop;
	struct is_crop otcrop_cfg;
	u32 hw_format = DMA_INOUT_FORMAT_YUV422;
	u32 hw_bitwidth = DMA_INOUT_BIT_WIDTH_12BIT;
	u32 hw_sbwc = DMA_INPUT_SBWC_DISABLE;
	u32 hw_plane, hw_msb, hw_order, flag_extra, flag_pixel_size;
	u32 width, height;

	dma_output = is_itf_g_param(device, frame, pindex);
	if (dma_output->cmd != node->request)
		mlvinfo("[F%d] WDMA enable: %d -> %d\n", device, node->vid, frame->fcount,
			dma_output->cmd, node->request);
	dma_output->cmd = DMA_OUTPUT_COMMAND_DISABLE;

	set_bit(pindex, pmap);

	if (!node->request)
		return 0;

	otcrop = (struct is_crop *)node->output.cropRegion;
	if (IS_NULL_CROP(otcrop)) {
		mlverr("[F%d][%d] otcrop is NULL (%d, %d, %d, %d), disable DMA", device, node->vid,
			frame->fcount, pindex, otcrop->x, otcrop->y, otcrop->w, otcrop->h);
		otcrop->x = otcrop->y = 0;
		otcrop->w = leader->input.width;
		otcrop->h = leader->input.height;
		dma_output->cmd = DMA_OUTPUT_COMMAND_DISABLE;
	}
	otcrop_cfg = *otcrop;
	width = otcrop_cfg.w;
	height = otcrop_cfg.h;

	if (__mtnr_check_wdma_prev(pindex)) {
		if (device->yuv_max_width && device->yuv_max_height) {
			__mtnr_get_prev_l_size(pindex,
					device->yuv_max_width, device->yuv_max_height,
					&width, &height);
		} else {
			__mtnr_get_prev_l_size(pindex,
					leader->input.width, leader->input.height,
					&width, &height);
		}
	}

	fmt = is_find_format(node->pixelformat, node->flags);
	if (!fmt) {
		merr("pixelformat(%c%c%c%c) is not found", device,
			(char)((node->pixelformat >> 0) & 0xFF),
			(char)((node->pixelformat >> 8) & 0xFF),
			(char)((node->pixelformat >> 16) & 0xFF),
			(char)((node->pixelformat >> 24) & 0xFF));
		return -EINVAL;
	}
	hw_format = fmt->hw_format;
	hw_bitwidth = fmt->hw_bitwidth;
	hw_msb = ZERO_IF_NEG(hw_bitwidth - 1);
	hw_order = fmt->hw_order;
	hw_plane = fmt->hw_plane;
	/* pixel type [0:5] : pixel size, [6:7] : extra */
	flag_pixel_size = node->flags & PIXEL_TYPE_SIZE_MASK;
	flag_extra = (node->flags & PIXEL_TYPE_EXTRA_MASK) >> PIXEL_TYPE_EXTRA_SHIFT;

	if (flag_extra)
		hw_sbwc = (SBWC_BASE_ALIGN_MASK | flag_extra);

	if (dma_output->format != hw_format)
		mlvinfo("[F%d]WDMA format: %d -> %d\n", device, node->vid, frame->fcount,
			dma_output->format, hw_format);
	if (dma_output->bitwidth != hw_bitwidth)
		mlvinfo("[F%d]WDMA bitwidth: %d -> %d\n", device, node->vid, frame->fcount,
			dma_output->bitwidth, hw_bitwidth);
	if (dma_output->sbwc_type != hw_sbwc)
		mlvinfo("[F%d]WDMA sbwc_type: %d -> %d\n", device, node->vid, frame->fcount,
			dma_output->sbwc_type, hw_sbwc);
	if (!frame->stripe_info.region_num &&
		((dma_output->width != width) || (dma_output->height != height)))
		mlvinfo("[F%d]WDMA otcrop[%d, %d, %d, %d]\n", device, node->vid, frame->fcount,
			otcrop->x, otcrop->y, otcrop->w, otcrop->h);

	dma_output->cmd = DMA_OUTPUT_COMMAND_ENABLE;
	dma_output->v_otf_enable = OTF_OUTPUT_COMMAND_DISABLE;

	dma_output->format = hw_format;
	dma_output->bitwidth = hw_bitwidth;
	dma_output->msb = hw_msb;
	dma_output->sbwc_type = hw_sbwc;
	dma_output->order = hw_order;
	dma_output->plane = hw_plane;
	dma_output->width = width;
	dma_output->height = height;
	dma_output->dma_crop_offset_x = 0;
	dma_output->dma_crop_offset_y = 0;
	dma_output->dma_crop_width = width;
	dma_output->dma_crop_height = height;

	return 0;
}

static int is_ischain_mtnr_tag(struct is_subdev *subdev, void *device_data, struct is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct is_crop *incrop, *otcrop;
	struct is_device_ischain *device;
	IS_DECLARE_PMAP(pmap);
	struct camera2_node *out_node = NULL;
	struct camera2_node *cap_node = NULL;
	u32 pindex = 0;
	int i, dma_type;

	device = (struct is_device_ischain *)device_data;

	mdbgs_ischain(4, "MTNR TAG\n", device);

	incrop = (struct is_crop *)node->input.cropRegion;
	otcrop = (struct is_crop *)node->output.cropRegion;

	IS_INIT_PMAP(pmap);

	out_node = &frame->shot_ext->node_group.leader;
	ret = __mtnr_dma_in_cfg(device, subdev, frame, out_node, PARAM_MTNR_RDMA_CUR_L0, pmap);
	if (ret) {
		mlverr("[F%d] dma_in_cfg fail. ret %d", device, node->vid, frame->fcount, ret);
		return ret;
	}

	out_node->result = 1;

	ret = __mtnr_otf_out_cfg(device, subdev, frame, out_node, PARAM_MTNR_COUT_MSNR_L0, pmap);
	if (ret) {
		mlverr("[F%d] otf_out_cfg fail. ret %d", device, node->vid, frame->fcount, ret);
		return ret;
	}

	for (i = 0; i < CAPTURE_NODE_MAX; i++) {
		cap_node = &frame->shot_ext->node_group.capture[i];

		if (!cap_node->vid)
			continue;
		dma_type = __ischain_mtnr_slot(cap_node, &pindex);
		if (dma_type == 1) {
			switch (pindex) {
			case PARAM_MTNR_RDMA_CUR_L1:
				ret = __mtnr_otf_out_cfg(device, subdev, frame, cap_node,
					PARAM_MTNR_COUT_MSNR_L1, pmap);
				break;
			case PARAM_MTNR_RDMA_CUR_L2:
				ret = __mtnr_otf_out_cfg(device, subdev, frame, cap_node,
					PARAM_MTNR_COUT_MSNR_L2, pmap);
				break;
			case PARAM_MTNR_RDMA_CUR_L3:
				ret = __mtnr_otf_out_cfg(device, subdev, frame, cap_node,
					PARAM_MTNR_COUT_MSNR_L3, pmap);
				break;
			case PARAM_MTNR_RDMA_CUR_L4:
				ret = __mtnr_otf_out_cfg(device, subdev, frame, cap_node,
					PARAM_MTNR_COUT_MSNR_L4, pmap);
				break;
			default:
				break;
			}
			if (ret) {
				mlverr("[F%d] otf_out_cfg fail(pindex %d). ret %d", device,
					node->vid, frame->fcount, pindex, ret);
				return ret;
			}

			ret = __mtnr_dma_in_cfg(device, subdev, frame, cap_node, pindex, pmap);
		} else if (dma_type == 2) {
			ret = __mtnr_dma_out_cfg(device, subdev, frame, cap_node, pindex, pmap, i);
		} else {
			if (IS_ENABLED(IRTA_CALL) &&
				(cap_node->vid == IS_LVN_MTNR0_CR ||
					cap_node->vid == IS_LVN_MTNR1_CR) &&
				!cap_node->request) {
				mlverr("[F%d] rta info node is disabled", device, cap_node->vid,
					frame->fcount);
				ret = -EINVAL;
				goto p_err;
			}
		}

		if (ret) {
			mlverr("[F%d] dma_%s_cfg error\n", device, cap_node->vid, frame->fcount,
				(dma_type == 1) ? "in" : "out");
			return ret;
		}

		cap_node->result = 1;
	}

	ret = __mtnr_stripe_in_cfg(device, subdev, frame, out_node, PARAM_MTNR_STRIPE_INPUT, pmap);
	if (ret) {
		mlverr("[F%d] strip_in_cfg fail. ret %d", device, node->vid, frame->fcount, ret);
		return ret;
	}

	ret = is_itf_s_param(device, frame, pmap);
	if (ret) {
		mrerr("is_itf_s_param is fail(%d)", device, frame, ret);
		goto p_err;
	}

	return 0;

p_err:
	return ret;
}

static inline void is_ischain_mtnr_get_config(struct is_subdev *subdev,
	struct is_frame *frame, struct is_mtnr0_config **mtnr0_config)
{
	int i, ret;
	struct is_sub_frame *sframe;
	dma_addr_t *dva;
	u64 *kva;
	void *config = NULL;
	struct is_core *core = is_get_is_core();
	struct is_hardware *hw;

	hw = &core->hardware;
	for (i = 0; i < CAPTURE_NODE_MAX; i++) {
		sframe = &frame->cap_node.sframe[i];
		if (!sframe->id)
			continue;

		if (sframe->id == IS_LVN_MTNR0_CR) {
			ret = is_hw_get_capture_slot(frame, &dva, &kva, sframe->id);
			if (ret) {
				mserr("Invalid capture slot(%d)", subdev, subdev, sframe->id);
				return;
			}

			if (dva)
				memcpy(dva, sframe->dva, sizeof(dma_addr_t) * sframe->num_planes);

			if (kva)
				memcpy(kva, sframe->kva, sizeof(u64) * sframe->num_planes);

			config = CALL_HW_CHAIN_INFO_OPS(hw, get_config, DEV_HW_MTNR0, frame);
			*mtnr0_config = (struct is_mtnr0_config *)(config + sizeof(u32));

			return;
		}
	}
}

static int is_ischain_mtnr_get(struct is_subdev *subdev, struct is_frame *frame,
	enum pablo_subdev_get_type type, void *result)
{
	struct camera2_node *node;
	struct is_crop *incrop, *outcrop;
	struct is_mtnr0_config *mtnr0_config = NULL;

	msrdbgs(1, "GET type: %d\n", subdev, subdev, frame, type);

	switch (type) {
	case PSGT_REGION_NUM:
		node = &frame->shot_ext->node_group.leader;
		incrop = (struct is_crop *)node->input.cropRegion;
		outcrop = (struct is_crop *)node->output.cropRegion;
		is_ischain_mtnr_get_config(subdev, frame, &mtnr0_config);
		subdev->constraints_width = (mtnr0_config && mtnr0_config->L0_bypass) ? 8192 : 4880;

		*(int *)result = is_calc_region_num(incrop, outcrop, subdev);
		break;
	default:
		break;
	}

	return 0;
}

static const struct is_subdev_ops is_subdev_mtnr_ops = {
	.bypass = NULL,
	.cfg = NULL,
	.tag = is_ischain_mtnr_tag,
	.get = is_ischain_mtnr_get,
};

const struct is_subdev_ops *pablo_get_is_subdev_mtnr_ops(void)
{
	return &is_subdev_mtnr_ops;
}
KUNIT_EXPORT_SYMBOL(pablo_get_is_subdev_mtnr_ops);
