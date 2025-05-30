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

#include <linux/module.h>

#include "punit-test-hw-ip.h"
#include "punit-test-file-io.h"
#include "pablo-framemgr.h"
#include "is-hw.h"
#include "is-core.h"
#include "is-device-ischain.h"

static int pst_set_hw_byrp(const char *val, const struct kernel_param *kp);
static int pst_get_hw_byrp(char *buffer, const struct kernel_param *kp);
static const struct kernel_param_ops pablo_param_ops_hw_byrp = {
	.set = pst_set_hw_byrp,
	.get = pst_get_hw_byrp,
};
module_param_cb(test_hw_byrp, &pablo_param_ops_hw_byrp, NULL, 0644);

#define NUM_OF_BYRP_PARAM (PARAM_BYRP_BYR - PARAM_BYRP_CONTROL + 1)

static struct is_frame *frame_byrp;
static u32 byrp_param[NUM_OF_BYRP_PARAM][PARAMETER_MAX_MEMBER];
static struct is_priv_buf *pb[NUM_OF_BYRP_PARAM];
static struct size_cr_set byrp_cr_set;

static const struct byrp_param byrp_param_preset_grp[] = {
	/* Param set[0]: Default */
	[0].control.cmd = CONTROL_COMMAND_START,
	[0].control.bypass = 0,
	[0].control.strgen = CONTROL_COMMAND_STOP,

	[0].otf_output.cmd = OTF_OUTPUT_COMMAND_DISABLE,
	[0].otf_output.format = 2,
	[0].otf_output.bitwidth = 12,
	[0].otf_output.order = 0,
	[0].otf_output.width = 4032,
	[0].otf_output.height = 3024,
	[0].otf_output.crop_offset_x = 0,
	[0].otf_output.crop_offset_y = 0,
	[0].otf_output.crop_width = 0,
	[0].otf_output.crop_height = 0,
	[0].otf_output.crop_enable = 0,

	[0].dma_input.cmd = DMA_INPUT_COMMAND_ENABLE,
	[0].dma_input.format = DMA_INOUT_FORMAT_BAYER_PACKED,
	[0].dma_input.bitwidth = DMA_INOUT_BIT_WIDTH_10BIT,
	[0].dma_input.order = DMA_INOUT_ORDER_NO,
	[0].dma_input.plane = DMA_INOUT_PLANE_1,
	[0].dma_input.width = 4032,
	[0].dma_input.height = 3024,
	[0].dma_input.dma_crop_offset = 0,
	[0].dma_input.dma_crop_width = 4032,
	[0].dma_input.dma_crop_height = 3024,
	[0].dma_input.bayer_crop_offset_x = 0,
	[0].dma_input.bayer_crop_offset_y = 0,
	[0].dma_input.bayer_crop_width = 4032,
	[0].dma_input.bayer_crop_height = 3024,
	[0].dma_input.scene_mode = 0,
	[0].dma_input.msb = DMA_INOUT_BIT_WIDTH_10BIT - 1,
	[0].dma_input.stride_plane0 = 4032,
	[0].dma_input.stride_plane1 = 4032,
	[0].dma_input.stride_plane2 = 4032,
	[0].dma_input.v_otf_enable = OTF_INPUT_COMMAND_DISABLE,
	[0].dma_input.orientation = 0,
	[0].dma_input.strip_mode = 0,
	[0].dma_input.overlab_width = 0,
	[0].dma_input.strip_count = 0,
	[0].dma_input.strip_max_count = 0,
	[0].dma_input.sequence_id = 0,
	[0].dma_input.sbwc_type = NONE,

	[0].dma_output_byr.cmd = DMA_OUTPUT_COMMAND_DISABLE,
};

static const struct byrp_param byrp_param_preset[] = {
	/* Param set[1]: RDMA(SBWC OFF) + BYR WDMA(SBWC OFF) - DNG */
	[0].control.cmd = CONTROL_COMMAND_START,
	[0].control.bypass = 0,
	[0].control.strgen = CONTROL_COMMAND_STOP,

	[0].otf_output.cmd = OTF_OUTPUT_COMMAND_DISABLE,

	[0].dma_input.cmd = DMA_INPUT_COMMAND_ENABLE,
	[0].dma_input.format = DMA_INOUT_FORMAT_BAYER,
	[0].dma_input.bitwidth = DMA_INOUT_BIT_WIDTH_16BIT,
	[0].dma_input.order = OTF_INPUT_ORDER_BAYER_GR_BG,
	[0].dma_input.plane = DMA_INOUT_PLANE_1,
	[0].dma_input.width = 320,
	[0].dma_input.height = 240,
	[0].dma_input.dma_crop_offset = 0,
	[0].dma_input.dma_crop_width = 320,
	[0].dma_input.dma_crop_height = 240,
	[0].dma_input.bayer_crop_offset_x = 0,
	[0].dma_input.bayer_crop_offset_y = 0,
	[0].dma_input.bayer_crop_width = 320,
	[0].dma_input.bayer_crop_height = 240,
	[0].dma_input.scene_mode = 0,
	[0].dma_input.msb = DMA_INOUT_BIT_WIDTH_10BIT - 1,
	[0].dma_input.stride_plane0 = 0,
	[0].dma_input.stride_plane1 = 0,
	[0].dma_input.stride_plane2 = 0,
	[0].dma_input.v_otf_enable = OTF_INPUT_COMMAND_DISABLE,
	[0].dma_input.orientation = 0,
	[0].dma_input.strip_mode = 0,
	[0].dma_input.overlab_width = 0,
	[0].dma_input.strip_count = 0,
	[0].dma_input.strip_max_count = 0,
	[0].dma_input.sequence_id = 0,
	[0].dma_input.sbwc_type = NONE,

	[0].dma_output_byr.cmd = DMA_OUTPUT_COMMAND_ENABLE,
	[0].dma_output_byr.format = DMA_INOUT_FORMAT_BAYER,
	[0].dma_output_byr.bitwidth = DMA_INOUT_BIT_WIDTH_16BIT,
	[0].dma_output_byr.order = OTF_INPUT_ORDER_BAYER_GR_BG,
	[0].dma_output_byr.plane = DMA_INOUT_PLANE_1,
	[0].dma_output_byr.width = 320,
	[0].dma_output_byr.height = 240,
	[0].dma_output_byr.dma_crop_offset_x = 0,
	[0].dma_output_byr.dma_crop_offset_y = 0,
	[0].dma_output_byr.dma_crop_width = 320,
	[0].dma_output_byr.dma_crop_height = 240,
	[0].dma_output_byr.crop_enable = 0,
	[0].dma_output_byr.msb = DMA_INOUT_BIT_WIDTH_14BIT - 1,
	[0].dma_output_byr.stride_plane0 = 0,
	[0].dma_output_byr.stride_plane1 = 0,
	[0].dma_output_byr.stride_plane2 = 0,
	[0].dma_output_byr.v_otf_enable = OTF_OUTPUT_COMMAND_DISABLE,
	[0].dma_output_byr.sbwc_type = NONE,
};

static DECLARE_BITMAP(result, ARRAY_SIZE(byrp_param_preset));

static void pst_set_size_byrp(void *in_param, void *out_param)
{
	struct byrp_param *p = (struct byrp_param *)byrp_param;
	struct  param_size_byrp2yuvp *in = (struct param_size_byrp2yuvp *)in_param;
	struct  param_size_byrp2yuvp *out = (struct param_size_byrp2yuvp *)out_param;

	if (!in || !out)
		return;

	p->otf_output.width = out->w_byrp;
	p->otf_output.height = out->h_byrp;

	p->dma_input.width = in->w_start;
	p->dma_input.height = in->h_start;
	p->dma_input.bayer_crop_offset_x = out->x_byrp,
	p->dma_input.bayer_crop_offset_y = out->y_byrp,
	p->dma_input.bayer_crop_width = out->w_byrp;
	p->dma_input.bayer_crop_height = out->h_byrp;

	p->dma_output_byr.width = out->w_byrp;
	p->dma_output_byr.height = out->h_byrp;
}

static enum pst_blob_node pst_get_blob_node_byrp(u32 idx)
{
	enum pst_blob_node bn;

	switch (PARAM_BYRP_CONTROL + idx) {
	case PARAM_BYRP_DMA_INPUT:
		bn = PST_BLOB_BYRP_DMA_INPUT;
		break;
	case PARAM_BYRP_BYR:
		bn = PST_BLOB_BYRP_BYR;
		break;
	default:
		bn = PST_BLOB_NODE_MAX;
		break;
	}

	return bn;
}

static void pst_set_buf_byrp(struct is_frame *frame, u32 param_idx)
{
	size_t size[IS_MAX_PLANES];
	u32 align = 32;
	u32 block_w = BYRP_COMP_BLOCK_WIDTH;
	u32 block_h = BYRP_COMP_BLOCK_HEIGHT;
	dma_addr_t *dva;

	memset(size, 0x0, sizeof(size));

	switch (PARAM_BYRP_CONTROL + param_idx) {
	case PARAM_BYRP_DMA_INPUT:
		dva = frame->dvaddr_buffer;
		pst_get_size_of_dma_input(&byrp_param[param_idx], align, block_w, block_h, size);
		break;
	case PARAM_BYRP_BYR:
		dva = frame->dva_byrp_byr;
		pst_get_size_of_dma_output(&byrp_param[param_idx], align, size);
		break;
	case PARAM_BYRP_CONTROL:
	case PARAM_BYRP_OTF_INPUT:
	case PARAM_BYRP_OTF_OUTPUT:
		return;
	default:
		pr_err("%s: invalid param_idx(%d)", __func__, param_idx);
		return;
	}

	if (size[0]) {
		pb[param_idx] = pst_set_dva(frame, dva, size, GROUP_ID_BYRP);
		pst_blob_inject(pst_get_blob_node_byrp(param_idx), pb[param_idx]);
	}
}

static void pst_init_param_byrp(unsigned int index, enum pst_hw_ip_type type)
{
	int i = 0;
	const struct byrp_param *preset;

	if (type == PST_HW_IP_SINGLE)
		preset = byrp_param_preset;
	else
		preset = byrp_param_preset_grp;

	memcpy(byrp_param[i++], (u32 *)&preset[index].control, PARAMETER_MAX_SIZE);
	memcpy(byrp_param[i++], (u32 *)&preset[index].otf_input, PARAMETER_MAX_SIZE);
	memcpy(byrp_param[i++], (u32 *)&preset[index].dma_input, PARAMETER_MAX_SIZE);
	memcpy(byrp_param[i++], (u32 *)&preset[index].otf_output, PARAMETER_MAX_SIZE);
	memcpy(byrp_param[i++], (u32 *)&preset[index].dma_output_byr, PARAMETER_MAX_SIZE);
}

static void pst_set_param_byrp(struct is_frame *frame)
{
	int i;

	for (i = 0; i < NUM_OF_BYRP_PARAM; i++) {
		pst_set_param(frame, byrp_param[i], PARAM_BYRP_CONTROL + i);
		pst_set_buf_byrp(frame, i);
	}
}

static void pst_clr_param_byrp(struct is_frame *frame)
{
	int i;

	for (i = 0; i < NUM_OF_BYRP_PARAM; i++) {
		if (!pb[i])
			continue;

		pst_blob_dump(pst_get_blob_node_byrp(i), pb[i]);

		pst_clr_dva(pb[i]);
		pb[i] = NULL;
	}
}

static const struct pst_callback_ops pst_cb_byrp = {
	.init_param = pst_init_param_byrp,
	.set_param = pst_set_param_byrp,
	.clr_param = pst_clr_param_byrp,
	.set_rta_info = NULL,
	.set_size = pst_set_size_byrp,
};

const struct pst_callback_ops *pst_get_hw_byrp_cb(void)
{
	return &pst_cb_byrp;
}

static int pst_set_hw_byrp(const char *val, const struct kernel_param *kp)
{
	return pst_set_hw_ip(val,
			DEV_HW_BYRP,
			frame_byrp,
			byrp_param,
			&byrp_cr_set,
			ARRAY_SIZE(byrp_param_preset),
			result,
			&pst_cb_byrp);
}

static int pst_get_hw_byrp(char *buffer, const struct kernel_param *kp)
{
	return pst_get_hw_ip(buffer, "BYRP", ARRAY_SIZE(byrp_param_preset), result);
}
