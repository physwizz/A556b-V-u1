// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/videodev2_exynos_media.h>
#include "is-device-ischain.h"
#include "is-device-sensor.h"
#include "is-subdev-ctrl.h"
#include "is-votfmgr.h"
#include "votf/pablo-votf.h"
#include "is-votf-id-table.h"
#include "is-config.h"
#include "is-param.h"
#include "is-video.h"
#include "is-type.h"
#include "is-hw.h"

static void _rgbp_slot(struct camera2_node *node, u32 *pindex)
{
	switch (node->vid) {
	case IS_LVN_RGBP0_DRC:
	case IS_LVN_RGBP1_DRC:
	case IS_LVN_RGBP2_DRC:
	case IS_LVN_RGBP3_DRC:
		*pindex = PARAM_RGBP_DRC;
		break;
	case IS_LVN_RGBP0_SAT:
	case IS_LVN_RGBP1_SAT:
	case IS_LVN_RGBP2_SAT:
	case IS_LVN_RGBP3_SAT:
		*pindex = PARAM_RGBP_SAT;
		break;
	case IS_LVN_RGBP0_HIST:
	case IS_LVN_RGBP1_HIST:
	case IS_LVN_RGBP2_HIST:
	case IS_LVN_RGBP3_HIST:
		*pindex = PARAM_RGBP_HIST;
		break;
	default:
		break;
	}
}

static inline int _check_cropRegion(struct is_device_ischain *device, struct camera2_node *node)
{
	struct is_crop *incrop, *otcrop;

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

	return 0;
}

static int _rgbp_otf_in_cfg(struct is_device_ischain *device,
	struct is_subdev *leader,
	struct is_frame *ldr_frame,
	struct camera2_node *node,
	u32 pindex,
	IS_DECLARE_PMAP(pmap))
{
	int ret;
	struct param_otf_input *otf_input;
	struct param_otf_output *byrp_output;
	struct is_crop *incrop;
	struct is_group *group;
	struct param_sensor_config *ss_cfg;

	set_bit(pindex, pmap);

	group = container_of(leader, struct is_group, leader);

	otf_input = is_itf_g_param(device, ldr_frame, pindex);
	if (test_bit(IS_GROUP_OTF_INPUT, &group->state))
		otf_input->cmd = OTF_INPUT_COMMAND_ENABLE;
	else
		otf_input->cmd = OTF_INPUT_COMMAND_DISABLE;

	if (!otf_input->cmd)
		return 0;

	ret = _check_cropRegion(device, node);
	if (ret)
		return ret;

	incrop = (struct is_crop *)node->input.cropRegion;

	if (((otf_input->bayer_crop_width != incrop->w) ||
		    (otf_input->bayer_crop_height != incrop->h)))
		mlvinfo("[F%d]OTF bcrop[%d, %d, %d, %d]\n", device, node->vid, ldr_frame->fcount,
			incrop->x, incrop->y, incrop->w, incrop->h);

	byrp_output = is_itf_g_param(device, NULL, PARAM_BYRP_OTF_OUTPUT);

	ss_cfg = &device->is_region->parameter.sensor.config;

	otf_input->width = byrp_output->width;
	otf_input->height = byrp_output->height;
	otf_input->bayer_crop_offset_x = incrop->x;
	otf_input->bayer_crop_offset_y = incrop->y;
	otf_input->bayer_crop_width = incrop->w;
	otf_input->bayer_crop_height = incrop->h;
	otf_input->order = ss_cfg->pixel_order;

	if (!test_bit(IS_GROUP_REPROCESSING, &group->state)) {
		ldr_frame->shot->udm.frame_info.taa_in_crop_region[0] = incrop->x;
		ldr_frame->shot->udm.frame_info.taa_in_crop_region[1] = incrop->y;
		ldr_frame->shot->udm.frame_info.taa_in_crop_region[2] = incrop->w;
		ldr_frame->shot->udm.frame_info.taa_in_crop_region[3] = incrop->h;
	}

	return 0;
}

static int _rgbp_otf_out_cfg(struct is_device_ischain *device,
	struct is_subdev *leader,
	struct is_frame *ldr_frame,
	struct camera2_node *node,
	u32 pindex,
	IS_DECLARE_PMAP(pmap))
{
	int ret;
	struct param_otf_output *otf_output;
	struct is_crop *otcrop;
	struct is_group *group;

	set_bit(pindex, pmap);

	group = container_of(leader, struct is_group, leader);

	otf_output = is_itf_g_param(device, ldr_frame, pindex);
	if (test_bit(IS_GROUP_OTF_OUTPUT, &group->state))
		otf_output->cmd = OTF_OUTPUT_COMMAND_ENABLE;
	else
		otf_output->cmd = OTF_OUTPUT_COMMAND_DISABLE;

	ret = _check_cropRegion(device, node);
	if (ret)
		return ret;

	otcrop = (struct is_crop *)node->input.cropRegion;

	otf_output->width = otcrop->w;
	otf_output->height = otcrop->h;

	return 0;
}

static int _rgbp_dma_in_cfg(struct is_device_ischain *device,
	struct is_subdev *leader,
	struct is_frame *ldr_frame,
	struct camera2_node *node,
	u32 pindex,
	IS_DECLARE_PMAP(pmap))
{
	struct is_fmt *fmt;
	struct param_dma_input *dma_input;
	struct is_crop *incrop, *otcrop;
	struct is_group *group;
	u32 hw_format = DMA_INOUT_FORMAT_BAYER;
	u32 hw_bitwidth = DMA_INOUT_BIT_WIDTH_16BIT;
	u32 hw_sbwc = DMA_INPUT_SBWC_DISABLE;
	u32 hw_msb;
	u32 hw_order, flag_extra, flag_pixel_size;
	u32 width;
	struct is_crop incrop_cfg, otcrop_cfg;
	int ret = 0;

	set_bit(pindex, pmap);

	group = container_of(leader, struct is_group, leader);

	dma_input = is_itf_g_param(device, ldr_frame, pindex);

	if (pindex == PARAM_RGBP_DMA_INPUT && test_bit(IS_GROUP_OTF_INPUT, &group->state)) {
		dma_input->cmd = DMA_INPUT_COMMAND_DISABLE;
	} else {
		if (dma_input->cmd != node->request)
			mlvinfo("[F%d] RDMA enable: %d -> %d\n", device,
				node->vid, ldr_frame->fcount,
				dma_input->cmd, node->request);
		dma_input->cmd = node->request;

		if (!dma_input->cmd)
			return 0;
	}

	ret = _check_cropRegion(device, node);
	if (ret)
		return ret;

	incrop = (struct is_crop *)node->input.cropRegion;
	otcrop = (struct is_crop *)node->output.cropRegion;

	incrop_cfg = *incrop;
	otcrop_cfg = *otcrop;
	fmt = is_find_format(node->pixelformat, node->flags);
	if (!fmt) {
		merr("pixel format(0x%x) is not found", device, node->pixelformat);
		return -EINVAL;
	}

	hw_format = fmt->hw_format;
	hw_bitwidth = fmt->hw_bitwidth;
	hw_msb = fmt->bitsperpixel[0] - 1;
	hw_order = fmt->hw_order;
	/* pixel type [0:5] : pixel size, [6:7] : extra */
	flag_pixel_size = node->flags & PIXEL_TYPE_SIZE_MASK;
	flag_extra = (node->flags & PIXEL_TYPE_EXTRA_MASK)
			>> PIXEL_TYPE_EXTRA_SHIFT;

	if (hw_format == DMA_INOUT_FORMAT_BAYER_PACKED
		&& flag_pixel_size == CAMERA_PIXEL_SIZE_13BIT) {
		mdbgs_ischain(3, "in_crop[bitwidth: %d -> %d: 13 bit BAYER]\n",
				device,
				hw_bitwidth,
				DMA_INOUT_BIT_WIDTH_13BIT);
		hw_bitwidth = DMA_INOUT_BIT_WIDTH_13BIT;
		hw_msb = DMA_INOUT_BIT_WIDTH_13BIT - 1;
	} else if (hw_format == DMA_INOUT_FORMAT_BAYER) {
		hw_msb = fmt->bitsperpixel[0];	/* consider signed format only */
		mdbgs_ischain(3, "in_crop[unpack bitwidth: %d, msb: %d]\n",
				device,
				hw_bitwidth,
				hw_msb);
	}

	if (flag_extra)
		hw_sbwc = (SBWC_BASE_ALIGN_MASK | flag_extra);

	switch (node->vid) {
	case IS_VIDEO_RGBP0:
		hw_format = DMA_INOUT_FORMAT_BAYER_PACKED;
		hw_bitwidth = DMA_INOUT_BIT_WIDTH_10BIT;
		hw_msb = DMA_INOUT_BIT_WIDTH_10BIT - 1;
		hw_order = fmt->hw_order;
		break;
	default:
		break;
	}

	if (dma_input->cmd) {
		if (dma_input->format != hw_format)
			mlvinfo("[F%d]RDMA format: %d -> %d\n", device,
				node->vid, ldr_frame->fcount,
				dma_input->format, hw_format);
		if (dma_input->bitwidth != hw_bitwidth)
			mlvinfo("[F%d]RDMA bitwidth: %d -> %d\n", device,
				node->vid, ldr_frame->fcount, dma_input->bitwidth, hw_bitwidth);
		if (dma_input->sbwc_type != hw_sbwc)
			mlvinfo("[F%d]RDMA sbwc_type: %d -> %d\n", device,
				node->vid, ldr_frame->fcount,
				dma_input->sbwc_type, hw_sbwc);
		if ((dma_input->width != incrop->w) || (dma_input->height != incrop->h))
			mlvinfo("[F%d]RDMA incrop[%d, %d, %d, %d]\n", device,
				node->vid, ldr_frame->fcount,
				incrop->x, incrop->y, incrop->w, incrop->h);
	}

	width = incrop_cfg.w;

	dma_input->format = hw_format;
	dma_input->bitwidth = hw_bitwidth;
	dma_input->msb = hw_msb;
	dma_input->sbwc_type = hw_sbwc;
	dma_input->order = hw_order;
	dma_input->plane = fmt->hw_plane;
	dma_input->width = width;
	dma_input->height = incrop_cfg.h;
	dma_input->dma_crop_offset = (incrop_cfg.x << 16) | (incrop_cfg.y << 0);
	dma_input->dma_crop_width = incrop_cfg.w;
	dma_input->dma_crop_height = incrop_cfg.h;
	dma_input->bayer_crop_offset_x = 0;
	dma_input->bayer_crop_offset_y = 0;
	dma_input->bayer_crop_width = incrop_cfg.w;
	dma_input->bayer_crop_height = incrop_cfg.h;
	dma_input->stride_plane0 = incrop->w;
	dma_input->stride_plane1 = incrop->w;
	dma_input->stride_plane2 = incrop->w;
	/* VOTF of slave in is always disabled */
	dma_input->v_otf_enable = OTF_INPUT_COMMAND_DISABLE;

	return ret;
}

static int _rgbp_dma_out_cfg(struct is_device_ischain *device,
	struct is_subdev *leader,
	struct is_frame *frame,
	struct camera2_node *node,
	u32 pindex,
	IS_DECLARE_PMAP(pmap),
	int index)
{
	struct is_fmt *fmt;
	struct param_dma_output *dma_output;
	struct is_crop *incrop, *otcrop;
	struct is_group *group;
	struct is_crop otcrop_cfg;
	u32 hw_format = DMA_INOUT_FORMAT_BAYER;
	u32 hw_bitwidth = DMA_INOUT_BIT_WIDTH_16BIT;
	u32 hw_sbwc = DMA_INPUT_SBWC_DISABLE;
	u32 hw_plane, hw_msb, hw_order, flag_extra, flag_pixel_size;
	u32 width;
	int ret = 0;

	FIMC_BUG(!frame);
	FIMC_BUG(!node);

	dma_output = is_itf_g_param(device, frame, pindex);
	if (dma_output->cmd != node->request)
		mlvinfo("[F%d] WDMA enable: %d -> %d\n", device,
			node->vid, frame->fcount,
			dma_output->cmd, node->request);
	dma_output->cmd = DMA_OUTPUT_COMMAND_DISABLE;

	set_bit(pindex, pmap);

	if (!node->request)
		return 0;

	ret = _check_cropRegion(device, node);
	if (ret)
		return ret;

	incrop = (struct is_crop *)node->input.cropRegion;
	otcrop = (struct is_crop *)node->output.cropRegion;

	otcrop_cfg = *otcrop;
	width = otcrop_cfg.w;

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
	hw_msb = fmt->bitsperpixel[0] - 1;
	hw_order = fmt->hw_order;
	hw_plane = fmt->hw_plane;
	/* pixel type [0:5] : pixel size, [6:7] : extra */
	flag_pixel_size = node->flags & PIXEL_TYPE_SIZE_MASK;
	flag_extra = (node->flags & PIXEL_TYPE_EXTRA_MASK) >> PIXEL_TYPE_EXTRA_SHIFT;

	if (hw_format == DMA_INOUT_FORMAT_BAYER_PACKED
	    && flag_pixel_size == CAMERA_PIXEL_SIZE_13BIT) {
		hw_bitwidth = DMA_INOUT_BIT_WIDTH_10BIT;
		hw_msb = DMA_INOUT_BIT_WIDTH_10BIT - 1;
	} else if (hw_format == DMA_INOUT_FORMAT_BAYER) {
		/* NOTICE: consider signed format only */
		hw_msb = fmt->bitsperpixel[0];
	}

	if (flag_extra)
		hw_sbwc = (SBWC_BASE_ALIGN_MASK | flag_extra);

	switch (node->vid) {
	/* WDMA */
	default:
		break;
	}

	if (dma_output->format != hw_format)
		mlvinfo("[F%d]WDMA format: %d -> %d\n", device,
			node->vid, frame->fcount,
			dma_output->format, hw_format);
	if (dma_output->bitwidth != hw_bitwidth)
		mlvinfo("[F%d]WDMA bitwidth: %d -> %d\n", device,
			node->vid, frame->fcount,
			dma_output->bitwidth, hw_bitwidth);
	if (dma_output->sbwc_type != hw_sbwc)
		mlvinfo("[F%d]WDMA sbwc_type: %d -> %d\n", device,
				node->vid, frame->fcount,
				dma_output->sbwc_type, hw_sbwc);
	if ((dma_output->width != otcrop->w) || (dma_output->height != otcrop->h))
		mlvinfo("[F%d]WDMA otcrop[%d, %d, %d, %d]\n", device,
			node->vid, frame->fcount,
			otcrop->x, otcrop->y, otcrop->w, otcrop->h);

	dma_output->cmd = OTF_OUTPUT_COMMAND_ENABLE;

	group = container_of(leader, struct is_group, leader);
	if (test_bit(IS_GROUP_OTF_OUTPUT, &group->state)) {
		if (test_bit(IS_GROUP_VOTF_OUTPUT, &group->state))
			dma_output->v_otf_enable = OTF_OUTPUT_COMMAND_ENABLE;
		 else
			dma_output->v_otf_enable = OTF_OUTPUT_COMMAND_DISABLE;
	} else {
		dma_output->v_otf_enable = OTF_OUTPUT_COMMAND_DISABLE;
	}

	dma_output->format = hw_format;
	dma_output->bitwidth = hw_bitwidth;
	dma_output->msb = hw_msb;
	dma_output->sbwc_type = hw_sbwc;
	dma_output->order = hw_order;
	dma_output->plane = hw_plane;
	dma_output->width = otcrop_cfg.w;
	dma_output->height = otcrop_cfg.h;
	dma_output->dma_crop_offset_x = 0;
	dma_output->dma_crop_offset_y = 0;
	dma_output->dma_crop_width = otcrop_cfg.w;
	dma_output->dma_crop_height = otcrop_cfg.h;
	dma_output->stride_plane0 = node->width;
	dma_output->stride_plane1 = node->width;
	dma_output->stride_plane2 = node->width;

	mdbgs_ischain(4, "[F%d] width: %d, height: %d, format: %d, bitwidth %d, msb: %d, cmd : %d\n", device,
		frame->fcount, dma_output->width, dma_output->height, dma_output->format, dma_output->bitwidth,
		dma_output->msb, dma_output->cmd);

	return ret;
}

static void _rgbp_control_cfg(struct is_device_ischain *device,
	struct is_group *group,
	struct is_frame *frame,
	IS_DECLARE_PMAP(pmap))

{
	struct param_control *control;

	control = is_itf_g_param(device, frame, PARAM_RGBP_CONTROL);
	if (test_bit(IS_GROUP_STRGEN_INPUT, &group->state))
		control->strgen = CONTROL_COMMAND_START;
	else
		control->strgen = CONTROL_COMMAND_STOP;

	set_bit(PARAM_RGBP_CONTROL, pmap);
}

static int _rgbp_is_valid_vid(u32 l_vid, u32 c_vid)
{
	u32 min_vid, max_vid;

	switch (l_vid) {
	case IS_VIDEO_RGBP0:
		min_vid = IS_LVN_RGBP0_DRC;
		max_vid = IS_LVN_RGBP0_HIST;
		break;
	case IS_VIDEO_RGBP1:
		min_vid = IS_LVN_RGBP1_DRC;
		max_vid = IS_LVN_RGBP1_HIST;
		break;
	case IS_VIDEO_RGBP2:
		min_vid = IS_LVN_RGBP2_DRC;
		max_vid = IS_LVN_RGBP2_HIST;
		break;
	case IS_VIDEO_RGBP3:
		min_vid = IS_LVN_RGBP3_DRC;
		max_vid = IS_LVN_RGBP3_HIST;
		break;
	default:
		return 0;
	}

	return (c_vid >= min_vid && c_vid <= max_vid);
}

static int is_ischain_rgbp_tag(struct is_subdev *subdev,
	void *device_data,
	struct is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct is_group *group;
	struct rgbp_param *rgbp_param;
	struct is_crop *incrop, *otcrop;
	struct is_device_ischain *device;
	IS_DECLARE_PMAP(pmap);
	struct camera2_node *out_node = NULL;
	struct camera2_node *cap_node = NULL;
	u32 pindex = 0;
	int i;

	device = (struct is_device_ischain *)device_data;

	FIMC_BUG(!subdev);
	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!frame);

	mdbgs_ischain(4, "RGBP TAG\n", device);

	incrop = (struct is_crop *)node->input.cropRegion;
	otcrop = (struct is_crop *)node->output.cropRegion;

	group = container_of(subdev, struct is_group, leader);
	IS_INIT_PMAP(pmap);
	rgbp_param = &device->is_region->parameter.rgbp;

	_rgbp_control_cfg(device, group, frame, pmap);

	out_node = &frame->shot_ext->node_group.leader;

	ret = _rgbp_dma_in_cfg(device, subdev, frame, out_node,
				PARAM_RGBP_DMA_INPUT, pmap);
	if (ret) {
		mlverr("[F%d] dma_in_cfg fail. ret %d", device, node->vid,
				frame->fcount, ret);
		return ret;
	}

	ret = _rgbp_otf_in_cfg(device, subdev, frame, out_node,
				PARAM_RGBP_OTF_INPUT, pmap);
	if (ret) {
		mlverr("[F%d] otf_in_cfg fail. ret %d", device, node->vid,
				frame->fcount, ret);
		return ret;
	}

	ret = _rgbp_otf_out_cfg(device, subdev, frame, out_node,
				PARAM_RGBP_OTF_OUTPUT, pmap);
	if (ret) {
		mlverr("[F%d] otf_out_cfg fail. ret %d", device, node->vid,
				frame->fcount, ret);
		return ret;
	}

	out_node->result = 1;

	for (i = 0; i < CAPTURE_NODE_MAX; i++) {
		cap_node = &frame->shot_ext->node_group.capture[i];
		if (!_rgbp_is_valid_vid(subdev->vid, cap_node->vid))
			continue;

		_rgbp_slot(cap_node, &pindex);

		ret = _rgbp_dma_out_cfg(device, subdev, frame, cap_node,
					pindex, pmap, i);
		if (ret) {
			mlverr("[F%d] dma_out_cfg error\n", device, cap_node->vid,
					frame->fcount);
			return ret;
		}

		cap_node->result = 1;
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

static const struct is_subdev_ops is_subdev_rgbp_ops = {
	.bypass	= NULL,
	.cfg	= NULL,
	.tag	= is_ischain_rgbp_tag,
};

const struct is_subdev_ops *pablo_get_is_subdev_rgbp_ops(void)
{
	return &is_subdev_rgbp_ops;
}
KUNIT_EXPORT_SYMBOL(pablo_get_is_subdev_rgbp_ops);
