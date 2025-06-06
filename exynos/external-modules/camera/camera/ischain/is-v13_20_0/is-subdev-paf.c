// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-device-ischain.h"
#include "is-device-sensor.h"
#include "is-subdev-ctrl.h"
#include "is-config.h"
#include "is-param.h"
#include "is-video.h"
#include "is-type.h"
#include "pablo-debug.h"

static int is_ischain_paf_cfg(struct is_subdev *leader,
	void *device_data,
	struct is_frame *frame,
	struct is_crop *incrop,
	struct is_crop *otcrop,
	IS_DECLARE_PMAP(pmap))
{
	int ret = 0;
	struct is_group *group;
	struct is_queue *queue;
	struct param_dma_input *dma_input;
	struct param_control *control;
	struct is_device_ischain *device;
	struct is_device_sensor *sensor;
	u32 hw_format = DMA_INOUT_FORMAT_BAYER;
	u32 hw_bitwidth = DMA_INOUT_BIT_WIDTH_16BIT;
	struct is_crop incrop_cfg, otcrop_cfg;

	device = (struct is_device_ischain *)device_data;

	FIMC_BUG(!leader);
	FIMC_BUG(!device);
	FIMC_BUG(!device->sensor);
	FIMC_BUG(!incrop);

	sensor = device->sensor;
	group = container_of(leader, struct is_group, leader);
	incrop_cfg = *incrop;
	otcrop_cfg = *otcrop;

	queue = GET_SUBDEV_QUEUE(leader);
	if (!test_bit(IS_GROUP_OTF_INPUT, &group->state)) {
		if (!queue || !queue->framecfg.format) {
			merr("format is NULL", device);
			ret = -EINVAL;
			goto p_err;
		}

		hw_format = queue->framecfg.format->hw_format;
		hw_bitwidth = queue->framecfg.format->hw_bitwidth; /* memory width per pixel */
	}

	/* Configure Conrtol */
	if (!frame) {
		control = is_itf_g_param(device, NULL, PARAM_PAF_CONTROL);
		if (test_bit(IS_GROUP_START, &group->state)) {
			control->cmd = CONTROL_COMMAND_START;
			control->bypass = CONTROL_BYPASS_DISABLE;
		} else {
			control->cmd = CONTROL_COMMAND_STOP;
			control->bypass = CONTROL_BYPASS_DISABLE;
		}

		set_bit(PARAM_PAF_CONTROL, pmap);
	}

	dma_input = is_itf_g_param(device, frame, PARAM_PAF_DMA_INPUT);
	if (test_bit(IS_GROUP_OTF_INPUT, &group->state)) {
		if (test_bit(IS_GROUP_VOTF_INPUT, &group->state)) {
			dma_input->cmd = DMA_INPUT_COMMAND_ENABLE;
			dma_input->v_otf_enable = OTF_INPUT_COMMAND_ENABLE;
		} else {
			dma_input->cmd = DMA_INPUT_COMMAND_DISABLE;
			dma_input->v_otf_enable = OTF_INPUT_COMMAND_DISABLE;
		}
	} else {
		dma_input->cmd = DMA_INPUT_COMMAND_ENABLE;
		dma_input->v_otf_enable = OTF_INPUT_COMMAND_DISABLE;
	}

	dma_input->format = hw_format;
	dma_input->bitwidth = hw_bitwidth;
	dma_input->msb = sensor->image.format.bitsperpixel[0] - 1;/* msb zero padding by HW constraint */
	dma_input->order = DMA_INOUT_ORDER_GR_BG;
	dma_input->plane = 1;
	dma_input->width = leader->input.width;
	dma_input->height = leader->input.height;

	dma_input->dma_crop_offset = 0;
	dma_input->dma_crop_width = incrop_cfg.w;
	dma_input->dma_crop_height = incrop_cfg.h;
	dma_input->bayer_crop_offset_x = incrop_cfg.x;
	dma_input->bayer_crop_offset_y = incrop_cfg.y;
	dma_input->bayer_crop_width = incrop_cfg.w;
	dma_input->bayer_crop_height = incrop_cfg.h;

	set_bit(PARAM_PAF_DMA_INPUT, pmap);

	leader->input.crop = *incrop;

p_err:
	return ret;
}

static int is_ischain_paf_tag(struct is_subdev *subdev,
	void *device_data,
	struct is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct is_group *group;
	struct paf_rdma_param *paf_param;
	struct is_crop inparm;
	struct is_crop *incrop, *otcrop;
	struct is_subdev *leader;
	struct is_device_ischain *device;
	IS_DECLARE_PMAP(pmap);

	device = (struct is_device_ischain *)device_data;

	FIMC_BUG(!subdev);
	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!frame);

	mdbgs_ischain(4, "PAF TAG\n", device);

	incrop = (struct is_crop *)node->input.cropRegion;
	otcrop = (struct is_crop *)node->output.cropRegion;

	group = container_of(subdev, struct is_group, leader);
	leader = subdev->leader;
	IS_INIT_PMAP(pmap);
	paf_param = &device->is_region->parameter.paf;

	if (test_bit(IS_GROUP_OTF_INPUT, &group->state)) {
		inparm.x = 0;
		inparm.y = 0;
		inparm.w = paf_param->dma_input.width;
		inparm.h = paf_param->dma_input.height;
	} else {
		inparm.x = 0;
		inparm.y = 0;
		inparm.w = paf_param->dma_input.dma_crop_width;
		inparm.h = paf_param->dma_input.dma_crop_height;
	}

	if (IS_NULL_CROP(incrop))
		*incrop = inparm;

	/* not supported DMA input crop */
	if ((incrop->x != 0) || (incrop->y != 0))
		*incrop = inparm;

	if (!COMPARE_CROP(incrop, &inparm) || test_bit(IS_SUBDEV_FORCE_SET, &leader->state)) {
		ret = is_ischain_paf_cfg(subdev,
			device,
			frame,
			incrop,
			otcrop,
			pmap);
		if (ret) {
			merr("is_ischain_paf_cfg is fail(%d)", device, ret);
			goto p_err;
		}

		msrinfo("in_crop[%d, %d, %d, %d]\n", device, subdev, frame,
			incrop->x, incrop->y, incrop->w, incrop->h);
		msrinfo("ot_crop[%d, %d, %d, %d]\n", device, subdev, frame,
			otcrop->x, otcrop->y, otcrop->w, otcrop->h);
	}

	ret = is_itf_s_param(device, frame, pmap);
	if (ret) {
		mrerr("is_itf_s_param is fail(%d)", device, frame, ret);
		goto p_err;
	}

	node->result = 1;

p_err:
	return ret;
}

static const struct is_subdev_ops is_subdev_paf_ops = {
	.bypass	= NULL,
	.cfg	= is_ischain_paf_cfg,
	.tag	= is_ischain_paf_tag,
};

const struct is_subdev_ops *pablo_get_is_subdev_paf_ops(void)
{
	return &is_subdev_paf_ops;
}
KUNIT_EXPORT_SYMBOL(pablo_get_is_subdev_paf_ops);
