/*
 * drivers/media/platform/exynos/mfc/mfc_utils.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <soc/samsung/exynos-smc.h>

#include "mfc_utils.h"
#include "mfc_mem.h"
#include "mfc_queue.h"

int mfc_check_vb_with_fmt(struct mfc_fmt *fmt, struct vb2_buffer *vb)
{
	struct mfc_ctx *ctx = vb->vb2_queue->drv_priv;
	int mem_planes;

	if (!fmt)
		return -EINVAL;

	if (fmt->type & MFC_FMT_FRAME)
		mem_planes = ctx->num_fd_frame;
	else
		mem_planes = fmt->mem_planes;

	if (mem_planes != vb->num_planes) {
		mfc_ctx_err("plane number is different (%d != %d)\n",
			    mem_planes, vb->num_planes);
		return -EINVAL;
	}

	return 0;
}

int mfc_check_resolution(struct mfc_ctx *ctx)
{
	int check_res = ctx->dev->pdata->support_check_res;
	int max_width, max_height;

	if (!check_res)
		return 0;

	max_width = mfc_res[check_res].width;
	max_height = mfc_res[check_res].height;

	if (((ctx->crop_width > max_width) && (ctx->crop_height > max_height)) ||
			((ctx->crop_width > max_height) && (ctx->crop_height > max_width))) {
		mfc_ctx_err("Resolution is too big (%dx%d > %dx%d or %dx%d)\n",
				ctx->crop_width, ctx->crop_height,
				max_width, max_height, max_height, max_width);
		return -EINVAL;
	}

	return 0;
}

unsigned int mfc_get_uncomp_format(struct mfc_ctx *ctx, u32 org_fmt)
{
	u32 uncomp_pixfmt = 0;

	switch (org_fmt) {
	case V4L2_PIX_FMT_NV12M_SBWC_8B:
		uncomp_pixfmt = V4L2_PIX_FMT_NV12M;
		break;
	case V4L2_PIX_FMT_NV21M_SBWC_8B:
		uncomp_pixfmt = V4L2_PIX_FMT_NV21M;
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
		if (ctx->type == MFCINST_DECODER)
			uncomp_pixfmt = V4L2_PIX_FMT_NV12N;
		else
			uncomp_pixfmt = V4L2_PIX_FMT_NV12M;
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_10B:
		uncomp_pixfmt = V4L2_PIX_FMT_NV12M_P010;
		break;
	case V4L2_PIX_FMT_NV21M_SBWC_10B:
		uncomp_pixfmt = V4L2_PIX_FMT_NV21M_P010;
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
		if (ctx->type == MFCINST_DECODER)
			uncomp_pixfmt = V4L2_PIX_FMT_NV12N_P010;
		else
			uncomp_pixfmt = V4L2_PIX_FMT_NV12M_P010;
		break;
	default:
		mfc_ctx_err("[SBWC] Cannot find uncomp format: %d\n", org_fmt);
		break;
	}

	return uncomp_pixfmt;
}

static void __mfc_set_dec_stride(struct mfc_ctx *ctx, struct mfc_raw_info *raw, struct mfc_fmt *fmt)
{
	int stride_align, y_stride, stride_type;

	stride_align = ctx->dev->pdata->stride_align;
	y_stride = ALIGN(ctx->img_width, stride_align);
	stride_type = ctx->dev->pdata->stride_type;

	switch (fmt->fourcc) {
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YUV420N:
	case V4L2_PIX_FMT_YVU420M:
		raw->stride[0] = y_stride;
		raw->stride[1] = ALIGN(y_stride >> 1, stride_align);
		raw->stride[2] = ALIGN(y_stride >> 1, stride_align);
		break;
	case V4L2_PIX_FMT_NV12MT_16X16:
	case V4L2_PIX_FMT_NV12MT:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV61M:
		raw->stride[0] = y_stride;
		raw->stride[1] = y_stride;
		raw->stride[2] = 0;
		break;
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV12N_10B:
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		raw->stride[0] = S10B_8B_STRIDE(ctx->img_width);
		raw->stride[1] = S10B_8B_STRIDE(ctx->img_width);
		raw->stride[2] = 0;
		raw->stride_2bits[0] = S10B_2B_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = S10B_2B_STRIDE(ctx->img_width);
		raw->stride_2bits[2] = 0;
		break;
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV61M_P210:
	case V4L2_PIX_FMT_NV16M_P210:
		if (ctx->dev->pdata->stride_type) {
			raw->stride[0] = ALIGN(ctx->img_width * 2, stride_align);
			raw->stride[1] = ALIGN(ctx->img_width * 2, stride_align);
		} else {
			raw->stride[0] = y_stride * 2;
			raw->stride[1] = y_stride * 2;
		}
		raw->stride[2] = 0;
		raw->stride_2bits[0] = 0;
		raw->stride_2bits[1] = 0;
		raw->stride_2bits[2] = 0;
		break;
	/* non-contiguous single fd format without extra */
	case V4L2_PIX_FMT_NV12N:
		if (stride_align < 64) {
			mfc_ctx_info("[FRAME] Forced to change stride align %d -> %dbyte\n",
					stride_align, 64);
			stride_align = 64;
		}
		raw->stride[0] = ALIGN(ctx->img_width, stride_align);
		raw->stride[1] = ALIGN(ctx->img_width, stride_align);
		break;
	case V4L2_PIX_FMT_NV12N_P010:
		if (stride_align < 128) {
			mfc_ctx_info("[FRAME] Forced to change stride align %d -> %dbyte\n",
					stride_align, 128);
			stride_align = 128;
		}
		raw->stride[0] = ALIGN(ctx->img_width * 2, stride_align);
		raw->stride[1] = ALIGN(ctx->img_width * 2, stride_align);
		break;
	/* for compress format (SBWC) */
	case V4L2_PIX_FMT_NV12M_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_8B:
		raw->stride[0] = SBWC_8B_STRIDE(ctx->img_width);
		raw->stride[1] = SBWC_8B_STRIDE(ctx->img_width);
		raw->stride[2] = 0;
		raw->stride_2bits[0] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[2] = 0;
		mfc_ctx_debug(2, "[SBWC] 8B stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
		raw->stride[0] = SBWC_256_8B_STRIDE(ctx->img_width);
		raw->stride[1] = SBWC_256_8B_STRIDE(ctx->img_width);
		raw->stride[2] = 0;
		raw->stride_2bits[0] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[2] = 0;
		mfc_ctx_debug(2, "[SBWC] 8B stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_10B:
		raw->stride[0] = SBWC_10B_STRIDE(ctx->img_width);
		raw->stride[1] = SBWC_10B_STRIDE(ctx->img_width);
		raw->stride[2] = 0;
		raw->stride_2bits[0] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[2] = 0;
		mfc_ctx_debug(2, "[SBWC] 10B stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
		raw->stride[0] = SBWC_256_10B_STRIDE(ctx->img_width);
		raw->stride[1] = SBWC_256_10B_STRIDE(ctx->img_width);
		raw->stride[2] = 0;
		raw->stride_2bits[0] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[2] = 0;
		mfc_ctx_debug(2, "[SBWC] 10B stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	default:
		mfc_ctx_err("Invalid pixelformat : %s\n", fmt->name);
		break;
	}
}

static void __mfc_set_enc_stride(struct mfc_ctx *ctx, struct mfc_raw_info *raw, struct mfc_fmt *fmt)
{
	int i, y_stride, stride_align = 16;

	y_stride = ctx->bytesperline[0];
	if (!y_stride)
		y_stride = ALIGN(ctx->img_width, stride_align);

	for (i = 0; i < MFC_MAX_PLANES; i++) {
		raw->stride[i] = 0;
		raw->stride_2bits[i] = 0;
	}

	switch (fmt->fourcc) {
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YUV420N:
	case V4L2_PIX_FMT_YVU420M:
	case V4L2_PIX_FMT_NV12MT_16X16:
	case V4L2_PIX_FMT_NV12MT:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV61M:
	case V4L2_PIX_FMT_NV12N:
		/* use user stride */
		for (i = 0; i < ctx->src_fmt->num_planes; i++) {
			raw->stride[i] = ctx->bytesperline[i];
			if (!raw->stride[i])
				raw->stride[i] = ALIGN(ctx->img_width, stride_align);
		}
		break;
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_RGB32X:
	case V4L2_PIX_FMT_BGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_RGB32:
		raw->stride[0] = y_stride * (ctx->rgb_bpp / 8);
		break;
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV12N_P010:
	case V4L2_PIX_FMT_NV21M_P010:
	case V4L2_PIX_FMT_NV61M_P210:
	case V4L2_PIX_FMT_NV16M_P210:
		raw->stride[0] = y_stride * 2;
		raw->stride[1] = y_stride * 2;
		break;
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV12N_10B:
	case V4L2_PIX_FMT_NV21M_S10B:
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		raw->stride[0] = S10B_8B_STRIDE(ctx->img_width);
		raw->stride[1] = S10B_8B_STRIDE(ctx->img_width);
		raw->stride_2bits[0] = S10B_2B_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = S10B_2B_STRIDE(ctx->img_width);
		break;
	/* for compress format (SBWC) */
	case V4L2_PIX_FMT_NV12M_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_8B:
		raw->stride[0] = SBWC_8B_STRIDE(ctx->img_width);
		raw->stride[1] = SBWC_8B_STRIDE(ctx->img_width);
		raw->stride_2bits[0] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWC_HEADER_STRIDE(ctx->img_width);
		mfc_ctx_debug(2, "[SBWC] 8B stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
		raw->stride[0] = SBWC_256_8B_STRIDE(ctx->img_width);
		raw->stride[1] = SBWC_256_8B_STRIDE(ctx->img_width);
		raw->stride[2] = 0;
		raw->stride_2bits[0] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[2] = 0;
		mfc_ctx_debug(2, "[SBWC] 8B stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_10B:
		raw->stride[0] = SBWC_10B_STRIDE(ctx->img_width);
		raw->stride[1] = SBWC_10B_STRIDE(ctx->img_width);
		raw->stride_2bits[0] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWC_HEADER_STRIDE(ctx->img_width);
		mfc_ctx_debug(2, "[SBWC] 10B stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
		raw->stride[0] = SBWC_256_10B_STRIDE(ctx->img_width);
		raw->stride[1] = SBWC_256_10B_STRIDE(ctx->img_width);
		raw->stride[2] = 0;
		raw->stride_2bits[0] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWC_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[2] = 0;
		mfc_ctx_debug(2, "[SBWC] 10B stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	/* for compress lossy format (SBWCL) */
	case V4L2_PIX_FMT_NV12M_SBWCL_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_8B:
		raw->stride[0] = SBWCL_8B_STRIDE(ctx->img_width, ctx->sbwcl_ratio);
		raw->stride[1] = SBWCL_8B_STRIDE(ctx->img_width, ctx->sbwcl_ratio);
		mfc_ctx_debug(2, "[SBWCL] 8B stride [0] %d [1] %d\n", raw->stride[0], raw->stride[1]);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_10B:
		raw->stride[0] = SBWCL_10B_STRIDE(ctx->img_width, ctx->sbwcl_ratio);
		raw->stride[1] = SBWCL_10B_STRIDE(ctx->img_width, ctx->sbwcl_ratio);
		mfc_ctx_debug(2, "[SBWCL] 10B stride [0] %d [1] %d\n", raw->stride[0], raw->stride[1]);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_32_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_10B:
		raw->stride[0] = SBWCL_32_STRIDE(ctx->img_width);
		raw->stride[1] = SBWCL_32_STRIDE(ctx->img_width);
		raw->stride_2bits[0] = SBWCL_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWCL_HEADER_STRIDE(ctx->img_width);
		mfc_ctx_debug(2, "[SBWCL] 32 stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_10B:
		raw->stride[0] = SBWCL_64_STRIDE(ctx->img_width);
		raw->stride[1] = SBWCL_64_STRIDE(ctx->img_width);
		raw->stride_2bits[0] = SBWCL_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWCL_HEADER_STRIDE(ctx->img_width);
		mfc_ctx_debug(2, "[SBWCL] 64 stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B_FR:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B_FR:
		raw->stride[0] = SBWCL_64_FR_STRIDE(ctx->img_width);
		raw->stride[1] = SBWCL_64_FR_STRIDE(ctx->img_width);
		raw->stride_2bits[0] = SBWCL_HEADER_STRIDE(ctx->img_width);
		raw->stride_2bits[1] = SBWCL_HEADER_STRIDE(ctx->img_width);
		mfc_ctx_debug(2, "[SBWCL] 64 FR stride [0] %d [1] %d header [0] %d [1] %d\n",
				raw->stride[0], raw->stride[1], raw->stride_2bits[0], raw->stride_2bits[1]);
		break;
	default:
		mfc_ctx_err("Invalid pixelformat : %s\n", fmt->name);
		break;
	}

	/*
	 * Check only HW limitation
	 * - default: 16byte aligned stride
	 * - single 8bit: 64byte aligned stride
	 * - single 10bit: 128byte aligned stride
	 */
	if (fmt->fourcc == V4L2_PIX_FMT_NV12N)
		stride_align = 64;
	else if (fmt->fourcc == V4L2_PIX_FMT_NV12N_P010)
		stride_align = 128;

	for (i = 0; i < ctx->src_fmt->num_planes; i++) {
		if ((raw->stride[i] % stride_align) != 0) {
			mfc_ctx_err("[FRAME] Forced to change stride[%d] %d for %dbyte alignment\n",
					i, raw->stride[i], stride_align);
			raw->stride[i] = ALIGN(raw->stride[i], stride_align);
		}
	}
}

void mfc_set_linear_stride_size(struct mfc_ctx *ctx, struct mfc_raw_info *raw, struct mfc_fmt *fmt)
{
	/*
	 * Decoder: Use stride alignment value defiend in DT.
	 *	    (Largest limitation among SoC IPs)
	 * Encoder: Use the stride value that the user set when s_fmt.
	 */
	if (ctx->type == MFCINST_DECODER)
		__mfc_set_dec_stride(ctx, raw, fmt);
	else
		__mfc_set_enc_stride(ctx, raw, fmt);
}

void mfc_dec_calc_dpb_size(struct mfc_ctx *ctx, struct mfc_raw_info *raw, struct mfc_fmt *fmt)
{
	int i;
	int extra = MFC_LINEAR_BUF_SIZE;
	int check_min_dpb_size = 1;

	mfc_set_linear_stride_size(ctx, raw, fmt);

	raw->total_plane_size = 0;

	for (i = 0; i < raw->num_planes; i++) {
		raw->plane_size[i] = 0;
		raw->plane_size_2bits[i] = 0;
	}

	switch (fmt->fourcc) {
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV21M_S10B:
		raw->plane_size[0] = NV12M_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = NV12M_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = NV12M_Y_2B_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = NV12M_CBCR_2B_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2 + extra;
		break;
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_P010:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2 + extra;
		break;
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YVU420M:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2 + extra;
		raw->plane_size[2] = raw->stride[2] * ALIGN(ctx->img_height, 16) / 2 + extra;
		break;
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		raw->plane_size[0] = NV16M_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = NV16M_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = NV16M_Y_2B_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = NV16M_CBCR_2B_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV61M:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) + extra;
		break;
	case V4L2_PIX_FMT_NV16M_P210:
	case V4L2_PIX_FMT_NV61M_P210:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) + extra;
		break;
	/* non-contiguous single fd format */
	case V4L2_PIX_FMT_NV12N_10B:
		raw->plane_size[0] = NV12N_10B_Y_8B_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = NV12N_10B_CBCR_8B_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = NV12N_10B_Y_2B_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = NV12N_10B_CBCR_2B_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_YUV420N:
		raw->plane_size[0] = YUV420N_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = YUV420N_CB_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[2] = YUV420N_CR_SIZE(ctx->img_width, ctx->img_height);
		break;
	/* non-contiguous single fd format without extra */
	case V4L2_PIX_FMT_NV12N:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16);
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2;
		break;
	case V4L2_PIX_FMT_NV12N_P010:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16);
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2;
		break;
	/* for compress format (SBWC) */
	case V4L2_PIX_FMT_NV12M_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_8B:
		raw->plane_size[0] = SBWC_8B_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWC_8B_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWC_8B_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWC_8B_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
		raw->plane_size[0] = SBWC_256_8B_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWC_256_8B_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWC_256_8B_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWC_256_8B_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_10B:
		raw->plane_size[0] = SBWC_10B_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWC_10B_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWC_10B_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWC_10B_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
		raw->plane_size[0] = SBWC_256_10B_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWC_256_10B_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWC_256_10B_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWC_256_10B_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	default:
		mfc_ctx_err("Invalid pixelformat : %s\n", fmt->name);
		break;
	}

	/*
	 * The min DPB size returned by firmware may be larger than
	 * the DPB size calculated by the driver in the following situation.
	 * - Change 10bit mem_type at INIT_BUF.
	 * - Change SBWC format at INIT_BUF.
	 * - Use single-fd format without extra bytes.
	 * In the above case, if the driver forcibly changes the DPB size,
	 * it fails due to buffer size error at V4L2 Qbuf.
	 * And when F/W really needs min DPB size in scenario like VP9 interframe DRC,
	 * if the driver does not force change the DPB size,
	 * No.57(INSUFFICIENT_DPB_SIZE) error occurs in F/W.
	 */
	if (ctx->is_10bit || ctx->sbwc_disabled || (fmt->mem_planes == 1))
		check_min_dpb_size = 0;

	if (check_min_dpb_size) {
		for (i = 0; i < raw->num_planes; i++) {
			if (raw->plane_size[i] < ctx->min_dpb_size[i]) {
				mfc_ctx_info("[FRAME] plane[%d] size %d / min size %d\n",
						i, raw->plane_size[i], ctx->min_dpb_size[i]);
				raw->plane_size[i] = ctx->min_dpb_size[i];
			}
			if (IS_2BIT_NEED(ctx) &&
					(raw->plane_size_2bits[i] < ctx->min_dpb_size_2bits[i])) {
				mfc_ctx_info("[FRAME] 2bit plane[%d] size %d / min size %d\n",
						i, raw->plane_size_2bits[i],
						ctx->min_dpb_size_2bits[i]);
				raw->plane_size_2bits[i] = ctx->min_dpb_size_2bits[i];
			}
		}
	}

	for (i = 0; i < raw->num_planes; i++) {
		raw->total_plane_size += raw->plane_size[i];
		mfc_ctx_debug(2, "[FRAME] Plane[%d] size = %d, stride = %d\n",
			i, raw->plane_size[i], raw->stride[i]);
	}
	if (IS_2BIT_NEED(ctx)) {
		for (i = 0; i < raw->num_planes; i++) {
			raw->total_plane_size += raw->plane_size_2bits[i];
			mfc_ctx_debug(2, "[FRAME]%s%s Plane[%d] 2bit size = %d, stride = %d\n",
					(ctx->is_10bit ? "[10BIT]" : ""),
					(ctx->is_sbwc ? "[SBWC]" : ""),
					i, raw->plane_size_2bits[i],
					raw->stride_2bits[i]);
		}
	}
	mfc_ctx_debug(2, "[FRAME] total plane size: %d\n", raw->total_plane_size);

	if (IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx)) {
		ctx->mv_size = DEC_MV_SIZE_MB(ctx->img_width, ctx->img_height);
		ctx->mv_size = ALIGN(ctx->mv_size, 32);
	} else if (IS_HEVC_DEC(ctx)) {
		ctx->mv_size = DEC_HEVC_MV_SIZE(ctx->img_width, ctx->img_height);
		ctx->mv_size = ALIGN(ctx->mv_size, 32);
	} else if (IS_AV1_DEC(ctx)) {
		ctx->mv_size = DEC_AV1_MV_SIZE(ctx->img_width, ctx->img_height);
		ctx->mv_size = ALIGN(ctx->mv_size, 32);
	} else {
		ctx->mv_size = 0;
	}
}

void mfc_enc_calc_src_size(struct mfc_ctx *ctx)
{
	struct mfc_raw_info *raw;
	int i, extra;

	raw = &ctx->raw_buf;
	raw->total_plane_size = 0;
	extra = MFC_LINEAR_BUF_SIZE;

	mfc_set_linear_stride_size(ctx, raw, ctx->src_fmt);

	for (i = 0; i < raw->num_planes; i++) {
		raw->plane_size[i] = 0;
		raw->plane_size_2bits[i] = 0;
	}

	switch (ctx->src_fmt->fourcc) {
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YUV420N:
	case V4L2_PIX_FMT_YVU420M:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2 + extra;
		raw->plane_size[2] = raw->stride[2] * ALIGN(ctx->img_height, 16) / 2 + extra;
		break;
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV21M_S10B:
		raw->plane_size[0] = NV12M_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = NV12M_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = NV12M_Y_2B_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = NV12M_CBCR_2B_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12MT_16X16:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2 + extra;
		break;
	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_P010:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2 + extra;
		break;
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		raw->plane_size[0] = NV16M_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = NV16M_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = NV16M_Y_2B_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = NV16M_CBCR_2B_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV61M:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) + extra;
		break;
	case V4L2_PIX_FMT_NV16M_P210:
	case V4L2_PIX_FMT_NV61M_P210:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16) + extra;
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) + extra;
		break;
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_RGB32X:
	case V4L2_PIX_FMT_BGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_RGB32:
		raw->plane_size[0] = raw->stride[0] * ctx->img_height;
		break;
	/* non-contiguous single fd format without extra */
	case V4L2_PIX_FMT_NV12N:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16);
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2;
		break;
	case V4L2_PIX_FMT_NV12N_P010:
		raw->plane_size[0] = raw->stride[0] * ALIGN(ctx->img_height, 16);
		raw->plane_size[1] = raw->stride[1] * ALIGN(ctx->img_height, 16) / 2;
		break;
	/* for compress format (SBWC) */
	case V4L2_PIX_FMT_NV12M_SBWC_8B:
	case V4L2_PIX_FMT_NV21M_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_8B:
		raw->plane_size[0] = SBWC_8B_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWC_8B_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWC_8B_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWC_8B_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
		raw->plane_size[0] = SBWC_256_8B_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWC_256_8B_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWC_256_8B_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWC_256_8B_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_10B:
	case V4L2_PIX_FMT_NV21M_SBWC_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_10B:
		raw->plane_size[0] = SBWC_10B_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWC_10B_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWC_10B_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWC_10B_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
		raw->plane_size[0] = SBWC_256_10B_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWC_256_10B_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWC_256_10B_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWC_256_10B_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	/* for compress lossy format (SBWCL) */
	case V4L2_PIX_FMT_NV12M_SBWCL_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_8B:
		raw->plane_size[0] = SBWCL_8B_Y_SIZE(ctx->img_width, ctx->img_height, ctx->sbwcl_ratio);
		raw->plane_size[1] = SBWCL_8B_CBCR_SIZE(ctx->img_width, ctx->img_height, ctx->sbwcl_ratio);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_10B:
		raw->plane_size[0] = SBWCL_10B_Y_SIZE(ctx->img_width, ctx->img_height, ctx->sbwcl_ratio);
		raw->plane_size[1] = SBWCL_10B_CBCR_SIZE(ctx->img_width, ctx->img_height, ctx->sbwcl_ratio);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_32_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_10B:
		raw->plane_size[0] = SBWCL_32_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWCL_32_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWCL_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWCL_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_10B:
		raw->plane_size[0] = SBWCL_64_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWCL_64_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWCL_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWCL_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B_FR:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B_FR:
		raw->plane_size[0] = SBWCL_64_FR_Y_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size[1] = SBWCL_64_FR_CBCR_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[0] = SBWCL_Y_HEADER_SIZE(ctx->img_width, ctx->img_height);
		raw->plane_size_2bits[1] = SBWCL_CBCR_HEADER_SIZE(ctx->img_width, ctx->img_height);
		break;
	default:
		mfc_ctx_err("Invalid pixel format(%d)\n", ctx->src_fmt->fourcc);
		break;
	}

	for (i = 0; i < raw->num_planes; i++) {
		raw->total_plane_size += raw->plane_size[i];
		mfc_ctx_debug(2, "[FRAME] Plane[%d] size = %d, stride = %d\n",
			i, raw->plane_size[i], raw->stride[i]);
	}
	if (IS_2BIT_NEED(ctx)) {
		for (i = 0; i < raw->num_planes; i++) {
			raw->total_plane_size += raw->plane_size_2bits[i];
			mfc_ctx_debug(2, "[FRAME]%s%s Plane[%d] 2bit size = %d, stride = %d\n",
					(ctx->is_10bit ? "[10BIT]" : ""),
					(ctx->is_sbwc ? "[SBWC]" : ""),
					i, raw->plane_size_2bits[i],
					raw->stride_2bits[i]);
		}
	}

	mfc_ctx_debug(2, "[FRAME] total plane size: %d\n", raw->total_plane_size);
}

void mfc_calc_base_addr(struct mfc_ctx *ctx, struct vb2_buffer *vb,
				struct mfc_fmt *fmt)
{
	struct mfc_buf *buf = vb_to_mfc_buf(vb);
	struct mfc_raw_info *raw;
	dma_addr_t start_raw;
	int i, idx, max_idx;

	raw = &ctx->raw_buf;

	if ((fmt->type & MFC_FMT_FRAME) && ctx->multi_view_enable)
		max_idx = MFC_MV_BUF_IDX_MAX;
	else
		max_idx = 1;

	for (i = 0; i < max_idx; i++) {
		/* It means there is no plane in the buffer. */
		if (ctx->view_buf_info[i].num_fd == 0)
			continue;

		if (i == MFC_MV_BUF_IDX_VIEW0 || i == MFC_MV_BUF_IDX_VIEW1) {
			start_raw = mfc_mem_get_daddr_vb(vb, ctx->view_buf_info[i].offset);

			switch (fmt->fourcc) {
			case V4L2_PIX_FMT_NV12N_10B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = NV12N_10B_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height);
				break;
			case V4L2_PIX_FMT_YUV420N:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = YUV420N_CB_BASE(start_raw, ctx->img_width, ctx->img_height);
				buf->addr[i][2] = YUV420N_CR_BASE(start_raw, ctx->img_width, ctx->img_height);
				break;
			case V4L2_PIX_FMT_NV12N:
			case V4L2_PIX_FMT_NV12N_P010:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = start_raw + raw->plane_size[0];
				break;
			case V4L2_PIX_FMT_NV12N_SBWC_8B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = SBWC_8B_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height);
				break;
			case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = SBWC_256_8B_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height);
				break;
			case V4L2_PIX_FMT_NV12N_SBWC_10B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = SBWC_10B_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height);
				break;
			case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = SBWC_256_10B_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height);
				break;
			case V4L2_PIX_FMT_NV12N_SBWCL_8B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = SBWCL_8B_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height, ctx->sbwcl_ratio);
				break;
			case V4L2_PIX_FMT_NV12N_SBWCL_10B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = SBWCL_10B_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height, ctx->sbwcl_ratio);
				break;
			case V4L2_PIX_FMT_NV12N_SBWCL_32_8B:
			case V4L2_PIX_FMT_NV12N_SBWCL_32_10B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = SBWCL_32_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height);
				break;
			case V4L2_PIX_FMT_NV12N_SBWCL_64_8B:
			case V4L2_PIX_FMT_NV12N_SBWCL_64_10B:
				buf->addr[i][0] = start_raw;
				buf->addr[i][1] = SBWCL_64_CBCR_BASE(start_raw, ctx->img_width, ctx->img_height);
				break;
			default:
				for (idx = 0; idx < ctx->view_buf_info[i].num_fd; idx++)
					buf->addr[i][idx] = mfc_mem_get_daddr_vb(vb, ctx->view_buf_info[i].offset + idx);
				break;
			}
		} else {
			for (idx = 0; idx < ctx->view_buf_info[i].num_fd; idx++)
				buf->addr[i][idx] = mfc_mem_get_daddr_vb(vb, ctx->view_buf_info[i].offset + idx);
		}
	}

	for (i = 0; i < fmt->num_planes; i++)
		mfc_ctx_debug(2, "[MEMINFO] plane[%d] addr %#llx\n", i, buf->addr[0][i]);
}

void mfc_set_view_buf_info(struct mfc_ctx *ctx, int mem_planes, int num_fd_depth_map, int num_fd_sub_view_meta) {
	int offset = 0;

	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW0].offset = offset;
	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW0].num_fd = mem_planes;
	offset += mem_planes;

	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW0_DEPTH].offset = offset;
	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW0_DEPTH].num_fd = num_fd_depth_map;
	offset += num_fd_depth_map;

	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW1].offset = offset;
	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW1].num_fd = mem_planes;
	offset += mem_planes;

	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW1_DEPTH].offset = offset;
	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW1_DEPTH].num_fd = num_fd_depth_map;
	offset += num_fd_depth_map;

	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW1_META].offset = offset;
	ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW1_META].num_fd = num_fd_sub_view_meta;
	offset += num_fd_sub_view_meta;

	return;
}

void mfc_assign_internal_dpb(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_raw_info *raw;
	int index, i;
	int offset;

	mutex_lock(&dec->dpb_mutex);

	raw = &ctx->internal_raw_buf;
	for (index = 0; index < dec->total_dpb_count; index++) {
		offset = 0;
		for (i = 0; i < raw->num_planes; i++) {
			dec->dpb[index].addr[i] = dec->internal_dpb[index].daddr + offset;
			offset += raw->plane_size[i];
			if (IS_2BIT_NEED(ctx))
				offset += raw->plane_size_2bits[i];
			mfc_ctx_debug(2, "[PLUGIN] dpb[%d] plane[%d] daddr %#llx offset %d (%d + %d)\n",
					index, i, dec->dpb[index].addr[i], offset,
					raw->plane_size[i], raw->plane_size_2bits[i]);
		}
		dec->dpb[index].paddr = dec->internal_dpb[index].paddr;
		dec->dpb[index].mapcnt = 1;
		dec->dpb[index].queued = 1;
	}

	mutex_unlock(&dec->dpb_mutex);
}

int mfc_find_buf_dpb_table(struct mfc_ctx *ctx, dma_addr_t dspl_y_addr)
{
	struct mfc_dec *dec = ctx->dec_priv;
	int i;

	for (i = 0; i < dec->total_dpb_count; i++) {
		if (dec->internal_dpb[i].daddr == dspl_y_addr) {
			mfc_ctx_debug(2, "[PLUGIN] internal dpb index: %d, addr: %#llx\n",
					i, dspl_y_addr);
			return i;
		}
	}

	mfc_ctx_err("[PLUGIN] couldn't find internal dpb addr: %#llx\n", dspl_y_addr);

	return -1;
}

void mfc_core_meerkat_tick(struct timer_list *t)
{
	struct mfc_core *core = from_timer(core, t, meerkat_timer);

	mfc_core_debug(5, "meerkat is ticking!\n");

	if (core->state == MFCCORE_ERROR) {
		atomic_set(&core->meerkat_tick_running, 0);
		atomic_set(&core->meerkat_tick_cnt, 0);
		mfc_core_info("[MSR] meerkat timer is now stopped! It's Error state\n");
		return;
	}

	if (atomic_read(&core->meerkat_tick_running))
		atomic_inc(&core->meerkat_tick_cnt);
	else
		atomic_set(&core->meerkat_tick_cnt, 0);

	if (atomic_read(&core->meerkat_tick_cnt) >= MEERKAT_TICK_CNT_TO_START_MEERKAT) {
		/* This means that hw is busy and no interrupts were
		 * generated by hw for the Nth time of running this
		 * meerkat timer. This usually means a serious hw
		 * error. Now it is time to kill all instances and
		 * reset the MFC. */
		mfc_core_err("[%d] Time out during waiting for HW\n",
				atomic_read(&core->meerkat_tick_cnt));
		queue_work(core->meerkat_wq, &core->meerkat_work);
	}

	mod_timer(&core->meerkat_timer, jiffies + msecs_to_jiffies(MEERKAT_TICK_INTERVAL));
}

void mfc_core_meerkat_start_tick(struct mfc_core *core)
{
	if (atomic_read(&core->meerkat_tick_running)) {
		mfc_core_debug(2, "meerkat timer was already started!\n");
	} else {
		mfc_core_debug(2, "meerkat timer is now started!\n");
		atomic_set(&core->meerkat_tick_running, 1);
	}

	/* Reset the timeout meerkat */
	atomic_set(&core->meerkat_tick_cnt, 0);
}

void mfc_core_meerkat_stop_tick(struct mfc_core *core)
{
	if (atomic_read(&core->meerkat_tick_running)) {
		mfc_core_debug(2, "meerkat timer is now stopped!\n");
		atomic_set(&core->meerkat_tick_running, 0);
	} else {
		mfc_core_debug(2, "meerkat timer was already stopped!\n");
	}

	/* Reset the timeout meerkat */
	atomic_set(&core->meerkat_tick_cnt, 0);
}

void mfc_core_meerkat_reset_tick(struct mfc_core *core)
{
	mfc_core_debug(2, "meerkat timer reset!\n");

	/* Reset the timeout meerkat */
	atomic_set(&core->meerkat_tick_cnt, 0);
}

void mfc_core_idle_checker(struct timer_list *t)
{
	struct mfc_core *core = from_timer(core, t, mfc_idle_timer);
	struct mfc_dev *dev = core->dev;

	mfc_core_debug(5, "[MFCIDLE] MFC HW idle checker is ticking!\n");

	if (dev->debugfs.perf_boost_mode) {
		mfc_core_info("[QoS][BOOST][MFCIDLE] skip control\n");
		return;
	}

	if (dev->move_ctx_cnt) {
		MFC_TRACE_RM("[MFCIDLE] migration working\n");
		mfc_core_idle_checker_start_tick(core);
		return;
	}

	if (atomic_read(&core->qos_req_cur) == 0) {
		mfc_core_debug(6, "[MFCIDLE] MFC QoS not started yet\n");
		mfc_core_idle_checker_start_tick(core);
		return;
	}

	if (core->sched->is_work(core)) {
		MFC_TRACE_CORE("[MFCIDLE] there is work to do\n");
		mfc_core_debug(6, "[MFCIDLE] there is work to do\n");
		core->sched->queue_work(core);
		mfc_core_idle_checker_start_tick(core);
		return;
	}

	if (!atomic_read(&core->hw_run_bits) && !atomic_read(&core->dev->queued_bits))
		mfc_core_change_idle_mode(core, MFC_IDLE_MODE_RUNNING);

#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
	queue_work(core->mfc_idle_wq, &core->mfc_idle_work);
#endif
}