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

#include <linux/videodev2_exynos_media.h>
#include "is-device-ischain.h"
#include "is-device-sensor.h"
#include "is-subdev-ctrl.h"
#include "is-config.h"
#include "is-param.h"
#include "is-video.h"
#include "is-type.h"
#include "is-hw.h"

static int __lme_dma_cfg(struct is_device_ischain *device,
	struct is_subdev *leader,
	struct is_frame *ldr_frame,
	struct camera2_node *node,
	struct param_lme_dma *dma,
	u32 pindex,
	IS_DECLARE_PMAP(pmap))
{
	struct is_fmt *fmt;
	struct param_control *control;
	struct is_crop *incrop, *otcrop;
	u32 hw_msb, hw_order, hw_plane;
	u32 hw_bitwidth = DMA_INOUT_BIT_WIDTH_8BIT;
	int ret = 0;
	u32 i;
	u32 *dma_cmd;

	FIMC_BUG(!leader);
	FIMC_BUG(!device);
	FIMC_BUG(!ldr_frame);
	FIMC_BUG(!node);

	switch (node->vid) {
	case IS_VIDEO_LME: /* curr */
		dma_cmd = &dma->cur_input_cmd;
		break;
	case IS_LVN_LME_PREV: /* prev */
		dma_cmd = &dma->prev_input_cmd;
		break;
	case IS_LVN_LME_PURE: /* mv out */
		dma_cmd = &dma->output_cmd;
		break;
	case IS_LVN_LME_SAD: /* sad out */
		dma_cmd = &dma->sad_output_cmd;
		break;
	case IS_LVN_LME_PROCESSED: /* proc mv from rta */
		dma_cmd = &dma->processed_output_cmd;
		break;
	case IS_LVN_LME_PROCESSED_HDR: /* S/W LME out for stitch hdr */
	case IS_LVN_LME_RTA_INFO:
		return 0;
	default:
		merr("vid(%d) is invalid", device, node->vid);
		return -EINVAL;
	}

	if (*dma_cmd != node->request)
		mlvinfo("[F%d] DMA enable: %d -> %d\n", device,
			node->vid, ldr_frame->fcount,
			*dma_cmd, node->request);

	if (!node->request) {
		*dma_cmd = DMA_OUTPUT_COMMAND_DISABLE;

		if (node->vid == IS_LVN_LME_PURE) {
			for (i = 0; i < IS_MAX_PLANES; i++)
				ldr_frame->lmecKTargetAddress[i] = 0;
		}

		set_bit(PARAM_LME_DMA, pmap);

		return 0;
	}

	incrop = (struct is_crop *)node->input.cropRegion;
	if (IS_NULL_CROP(incrop)) {
		merr("incrop is NULL", device);
		return -EINVAL;
	}

	otcrop = (struct is_crop *)node->output.cropRegion;
	if (IS_NULL_CROP(otcrop)) {
		merr("otcrop is NULL", device);
		return -EINVAL;
	}

	fmt = is_find_format(node->pixelformat, node->flags);
	if (!fmt) {
		merr("pixel format(0x%x) is not found", device, node->pixelformat);
		return -EINVAL;
	}

	hw_msb = hw_bitwidth - 1;
	hw_order = fmt->hw_order;
	hw_plane = fmt->hw_plane;

	control = is_itf_g_param(device, ldr_frame, PARAM_LME_CONTROL);
	control->cmd = CONTROL_COMMAND_START;
	control->bypass = CONTROL_BYPASS_DISABLE;

	switch (node->vid) {
	case IS_VIDEO_LME:
		set_bit(PARAM_LME_CONTROL, pmap);
		dma->cur_input_cmd = DMA_INPUT_COMMAND_ENABLE;
		dma->input_format = DMA_INOUT_FORMAT_Y;
		dma->input_bitwidth = hw_bitwidth;
		dma->input_order = hw_order;
		dma->input_msb = hw_msb;
		dma->input_plane = hw_plane;
		dma->cur_input_width = incrop->w;
		dma->cur_input_height = incrop->h;
		set_bit(pindex, pmap);
		break;
	case IS_LVN_LME_PREV:
		dma->prev_input_cmd = DMA_INPUT_COMMAND_ENABLE;
		dma->prev_input_width = incrop->w;
		dma->prev_input_height = incrop->h;
		break;
	case IS_LVN_LME_PURE:
		dma->output_cmd = DMA_OUTPUT_COMMAND_ENABLE;
		dma->output_order = hw_order;
		dma->output_bitwidth = hw_bitwidth;
		dma->output_plane = hw_plane;
		dma->output_msb = hw_msb;
		dma->output_width = otcrop->w;
		dma->output_height = otcrop->h;
		break;
	case IS_LVN_LME_SAD:
		dma->sad_output_cmd = DMA_OUTPUT_COMMAND_ENABLE;
		dma->sad_output_plane = hw_plane;
		break;
	case IS_LVN_LME_PROCESSED:
		dma->processed_output_cmd = DMA_OUTPUT_COMMAND_ENABLE;
		break;
	}

	return ret;
}

static int is_ischain_lme_tag(struct is_subdev *subdev,
	void *device_data,
	struct is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct lme_param *lme_param;
	struct is_crop *incrop, *otcrop;
	struct is_subdev *leader;
	struct is_device_ischain *device;
	struct camera2_node *out_node = NULL;
	struct camera2_node *cap_node = NULL;
	struct param_lme_dma *dma = NULL;
	IS_DECLARE_PMAP(pmap);
	int i;

	device = (struct is_device_ischain *)device_data;

	FIMC_BUG(!subdev);
	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!frame);

	mdbgs_ischain(4, "LME TAG\n", device);

	incrop = (struct is_crop *)node->input.cropRegion;
	otcrop = (struct is_crop *)node->output.cropRegion;

	leader = subdev->leader;
	IS_INIT_PMAP(pmap);
	lme_param = &device->is_region->parameter.lme;

	if (IS_ENABLED(LOGICAL_VIDEO_NODE)) {
		out_node = &frame->shot_ext->node_group.leader;
		dma = is_itf_g_param(device, frame, PARAM_LME_DMA);
		ret = __lme_dma_cfg(device, subdev, frame, out_node, dma, PARAM_LME_DMA, pmap);
		if (ret) {
			mlverr("[F%d] lme_dma_cfg fail. ret %d", device, node->vid,
					frame->fcount, ret);
			return ret;
		}

		out_node->result = 1;

		for (i = 0; i < CAPTURE_NODE_MAX; i++) {
			cap_node = &frame->shot_ext->node_group.capture[i];
			if (!cap_node->vid)
				continue;

			ret = __lme_dma_cfg(device, subdev, frame, cap_node, dma, PARAM_LME_DMA, pmap);
			if (ret) {
				mlverr("[F%d] lme_dma_cfg fail. ret %d", device, node->vid,
						frame->fcount, ret);
				return ret;
			}

			cap_node->result = 1;
		}

		ret = is_itf_s_param(device, frame, pmap);
		if (ret) {
			mrerr("is_itf_s_param is fail(%d)", device, frame, ret);
			goto p_err;
		}

		return ret;
	}

p_err:
	return ret;
}

static const struct is_subdev_ops is_subdev_lme_ops = {
	.bypass	= NULL,
	.cfg	= NULL,
	.tag	= is_ischain_lme_tag,
};

const struct is_subdev_ops *pablo_get_is_subdev_lme_ops(void)
{
	return &is_subdev_lme_ops;
}
KUNIT_EXPORT_SYMBOL(pablo_get_is_subdev_lme_ops);
