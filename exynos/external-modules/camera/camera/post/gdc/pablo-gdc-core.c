// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS ISPP GDC driver
 * (FIMC-IS PostProcessing Generic Distortion Correction driver)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>

#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-sg.h>
#include <media/v4l2-ioctl.h>

#if IS_ENABLED(CONFIG_EXYNOS_SCI)
#include <soc/samsung/exynos-sci.h>
#endif

#include "votf/pablo-votf.h"
#include "pablo-gdc.h"
#include "pablo-hw-api-gdc.h"
#include "pablo-kernel-variant.h"
#include "is-common-enum.h"
#include "is-video.h"
#include "pmio.h"
#include "pablo-device-iommu-group.h"
#include "pablo-irq.h"

/* Flags that are set by us */
#define V4L2_BUFFER_MASK_FLAGS                                                                     \
	(V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_QUEUED | V4L2_BUF_FLAG_DONE | V4L2_BUF_FLAG_ERROR |  \
	 V4L2_BUF_FLAG_PREPARED | V4L2_BUF_FLAG_IN_REQUEST | V4L2_BUF_FLAG_REQUEST_FD |            \
	 V4L2_BUF_FLAG_TIMESTAMP_MASK)
/* Output buffer flags that should be passed on to the driver */
#define V4L2_BUFFER_OUT_FLAGS                                                                      \
	(V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME | V4L2_BUF_FLAG_KEYFRAME |                    \
	 V4L2_BUF_FLAG_TIMECODE)

#define GDC_V4L2_DEVICE_CAPS (V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING)

#if PKV_VER_GE(6, 1, 0)
#define EXYNOS_GDC_SUPPORT_V4L2_PIX_FMT_P010
#endif

static int gdc_log_level;
module_param_named(gdc_log_level, gdc_log_level, uint, 0644);

static ulong debug_gdc;
static int param_get_debug_gdc(char *buffer, const struct kernel_param *kp)
{
	int ret;

	ret = sprintf(buffer, "GDC debug features\n");
	ret += sprintf(buffer + ret, "\tb[0] : Dump SFR (0x1)\n");
	ret += sprintf(buffer + ret, "\tb[1] : Dump SFR Once (0x2)\n");
	ret += sprintf(buffer + ret, "\tb[2] : S2D (0x4)\n");
	ret += sprintf(buffer + ret, "\tb[3] : Shot & H/W latency (0x8)\n");
	ret += sprintf(buffer + ret, "\tb[4] : PMIO APB-DIRECT (0x10)\n");
	ret += sprintf(buffer + ret, "\tb[5] : Dump PMIO Cache Buffer (0x20)\n");
	ret += sprintf(buffer + ret, "\tcurrent value : 0x%lx\n", debug_gdc);

	return ret;
}

static const struct kernel_param_ops param_ops_debug_gdc = {
	.set = param_set_ulong,
	.get = param_get_debug_gdc,
};

module_param_cb(debug_gdc, &param_ops_debug_gdc, &debug_gdc, 0644);

static u32 gdc_driver_version;
module_param_named(driver_version, gdc_driver_version, uint, 0444);
static void gdc_set_driver_version(u32 gdc_version)
{
	gdc_driver_version = gdc_version;
}

static int gdc_suspend(struct device *dev);
static int gdc_resume(struct device *dev);

struct vb2_gdc_buffer {
	struct v4l2_m2m_buffer mb;
	struct gdc_ctx *ctx;
	ktime_t ktime;
};

static const struct gdc_fmt gdc_formats[] = {
	{
		.name = "YUV 4:2:0 non-contiguous 2-planar, Y/CbCr",
		.pixelformat = V4L2_PIX_FMT_NV12M,
		.bitperpixel = { 8, 4 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "YUV 4:2:0 non-contiguous 2-planar, Y/CrCb",
		.pixelformat = V4L2_PIX_FMT_NV21M,
		.bitperpixel = { 8, 4 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "YUV 4:2:0 contiguous 2-planar, Y/CbCr",
		.pixelformat = V4L2_PIX_FMT_NV12,
		.bitperpixel = { 8 },
		.num_planes = 1,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "YUV 4:2:0 contiguous 2-planar, Y/CrCb",
		.pixelformat = V4L2_PIX_FMT_NV21,
		.bitperpixel = { 8 },
		.num_planes = 1,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "YUV 4:2:2 packed, YCrYCb",
		.pixelformat = V4L2_PIX_FMT_YVYU,
		.bitperpixel = { 16 },
		.num_planes = 1,
		.num_comp = 1,
		.h_shift = 1,
	},
	{
		.name = "YUV 4:2:2 packed, YCbYCr",
		.pixelformat = V4L2_PIX_FMT_YUYV,
		.bitperpixel = { 16 },
		.num_planes = 1,
		.num_comp = 1,
		.h_shift = 1,
	},
	{
		.name = "YUV 4:2:2 contiguous 2-planar, Y/CbCr",
		.pixelformat = V4L2_PIX_FMT_NV16,
		.bitperpixel = { 8 },
		.num_planes = 1,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "YUV 4:2:2 contiguous 2-planar, Y/CrCb",
		.pixelformat = V4L2_PIX_FMT_NV61,
		.bitperpixel = { 8 },
		.num_planes = 1,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "YUV 4:2:2 non-contiguous 2-planar, Y/CbCr",
		.pixelformat = V4L2_PIX_FMT_NV16M,
		.bitperpixel = { 8, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "YUV 4:2:2 non-contiguous 2-planar, Y/CrCb",
		.pixelformat = V4L2_PIX_FMT_NV61M,
		.bitperpixel = { 8, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "P010_16B",
		.pixelformat = V4L2_PIX_FMT_NV12M_P010,
		.bitperpixel = { 16, 16 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "P010_16B",
		.pixelformat = V4L2_PIX_FMT_NV21M_P010,
		.bitperpixel = { 16, 16 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "P210_16B",
		.pixelformat = V4L2_PIX_FMT_NV16M_P210,
		.bitperpixel = { 16, 16 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "P210_16B",
		.pixelformat = V4L2_PIX_FMT_NV61M_P210,
		.bitperpixel = { 16, 16 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "YUV422 2P 10bit(8+2)",
		.pixelformat = V4L2_PIX_FMT_NV16M_S10B,
		.bitperpixel = { 8, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "YUV422 2P 10bit(8+2)",
		.pixelformat = V4L2_PIX_FMT_NV61M_S10B,
		.bitperpixel = { 8, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "YUV420 2P 10bit(8+2)",
		.pixelformat = V4L2_PIX_FMT_NV12M_S10B,
		.bitperpixel = { 8, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "YUV420 2P 10bit(8+2)",
		.pixelformat = V4L2_PIX_FMT_NV21M_S10B,
		.bitperpixel = { 8, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
	},
	{
		.name = "NV12M SBWC LOSSY 8bit",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWCL_8B,
		.bitperpixel = { 8, 4 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M SBWC LOSSY 10bit",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWCL_10B,
		.bitperpixel = { 16, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M SBWC LOSSY 32B Align 8bit",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWCL_32_8B,
		.bitperpixel = { 8, 4 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M SBWC LOSSY 32B Align 10bit",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWCL_32_10B,
		.bitperpixel = { 16, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M SBWC LOSSY 64B Align 8bit",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWCL_64_8B,
		.bitperpixel = { 8, 4 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M SBWC LOSSY 64B Align 10bit",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWCL_64_10B,
		.bitperpixel = { 16, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M 8bit SBWC Lossy 64B align footprint reduction",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWCL_64_8B_FR,
		.bitperpixel = { 8, 4 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M 10bit SBWC Lossy 64B align footprint reduction",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWCL_64_10B_FR,
		.bitperpixel = { 16, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M SBWC LOSSLESS 8bit",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWC_8B,
		.bitperpixel = { 8, 4 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "NV12M SBWC LOSSLESS 10bit",
		.pixelformat = V4L2_PIX_FMT_NV12M_SBWC_10B,
		.bitperpixel = { 16, 8 },
		.num_planes = 2,
		.num_comp = 2,
		.h_shift = 1,
		.v_shift = 1,
	},
	{
		.name = "Y 8bit",
		.pixelformat = V4L2_PIX_FMT_GREY,
		.bitperpixel = { 8 },
		.num_planes = 1,
		.num_comp = 1,
		.h_shift = 1,
		.v_shift = 1,
	},
#ifdef EXYNOS_GDC_SUPPORT_V4L2_PIX_FMT_P010
	{
		.name = "P010 contiguous 2-planar, Y/CbCr",
		.pixelformat = V4L2_PIX_FMT_P010,
		.bitperpixel = { 16 },
		.num_planes = 1,
		.num_comp = 2,
		.h_shift = 1,
	},
#endif
};

static const char * const gdc_out_mode_names[] = {
	"M2M",
	"VOTF",
	"OTF",
	"NONE",
};

static inline const char *gdc_get_out_mode_name(enum gdc_out_mode out_mode)
{
	if (out_mode < GDC_OUT_M2M || out_mode > GDC_OUT_NONE)
		return "INVALID";

	return gdc_out_mode_names[out_mode];
}

int gdc_get_log_level(void)
{
	return gdc_log_level;
}

static int gdc_votf_set_service_config(int txs, struct gdc_dev *gdc)
{
	int ret = 0;
	int id = 0;
	struct votf_info vinfo;
	struct votf_service_cfg cfg;

	memset(&vinfo, 0, sizeof(struct votf_info));
	memset(&cfg, 0, sizeof(struct votf_service_cfg));

	if (txs) {
		/* TRS: Slave */
	} else {
		/* TWS: Master */
		for (id = 0; id < GDC_VOTF_ADDR_MAX; id++) {
			vinfo.service = TWS;
			vinfo.ip = gdc->votf_src_dest[GDC_SRC_ADDR] >> 16;
			vinfo.id = id;

			cfg.enable = 0x1;
			cfg.limit = 0x1;
			cfg.token_size = 0x1;
			cfg.connected_ip = gdc->votf_src_dest[GDC_DST_ADDR] >> 16;
			cfg.connected_id = id;
			cfg.option = 0;

			ret = votfitf_set_service_cfg(&vinfo, &cfg);
			if (ret < 0) {
				ret = -EINVAL;
				gdc_dev_dbg(gdc->dev,
					"votfitf_set_service_cfg for TWS is fail (src(%d, %d), dst(%d, %d))",
					vinfo.ip, vinfo.id, cfg.connected_ip, cfg.connected_id);
			} else
				gdc_dev_dbg(gdc->dev,
					"votfitf_set_service_cfg success (src(0x%x, %d), dst(0x%x, %d))",
					vinfo.ip, vinfo.id, cfg.connected_ip, cfg.connected_id);
		}
	}

	return ret;
}

static int gdc_votfitf_set_flush(struct gdc_dev *gdc)
{
	int ret = 0;
	int id = 0;
	struct votf_info vinfo;

	memset(&vinfo, 0, sizeof(struct votf_info));

	for (id = 0; id < GDC_VOTF_ADDR_MAX; id++) {
		vinfo.service = TWS;
		vinfo.ip = gdc->votf_src_dest[GDC_SRC_ADDR] >> 16;
		vinfo.id = id;

		ret = votfitf_set_flush(&vinfo);
		if (ret < 0)
			gdc_dev_dbg(gdc->dev,
				"votfitf_set_flush failed (src(0x%x, %d)", vinfo.ip, vinfo.id);
	}
	return ret;
}

/* Find the matches format */
static const struct gdc_fmt *gdc_find_format(struct gdc_dev *gdc, u32 pixfmt, bool output_buf)
{
	const struct gdc_fmt *gdc_fmt;
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(gdc_formats); ++i) {
		gdc_fmt = &gdc_formats[i];
		if (gdc_fmt->pixelformat == pixfmt)
			return &gdc_formats[i];
	}

	return NULL;
}

static int gdc_v4l2_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	strscpy(cap->driver, GDC_MODULE_NAME, sizeof(cap->driver));
	strscpy(cap->card, GDC_MODULE_NAME, sizeof(cap->card));

	cap->capabilities =
		V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE;
	cap->capabilities |= V4L2_CAP_DEVICE_CAPS;
	cap->device_caps = GDC_V4L2_DEVICE_CAPS;

	return 0;
}

static unsigned int gdc_get_sizeimage_of_plain_format(
		const struct gdc_fmt *gdc_fmt,
		const int plane_index,
		const struct v4l2_pix_format_mplane *pixm)
{
	switch (gdc_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV21M_S10B:
		if (plane_index == 0)
			return NV12M_Y_SIZE(pixm->width, pixm->height) +
				NV12M_Y_2B_SIZE(pixm->width, pixm->height);
		else
			return NV12M_CBCR_SIZE(pixm->width, pixm->height) +
				NV12M_CBCR_2B_SIZE(pixm->width, pixm->height);

	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		if (plane_index == 0)
			return NV16M_Y_SIZE(pixm->width, pixm->height) +
				NV16M_Y_2B_SIZE(pixm->width, pixm->height);
		else
			return NV16M_CBCR_SIZE(pixm->width, pixm->height) +
				NV16M_CBCR_2B_SIZE(pixm->width, pixm->height);

	case V4L2_PIX_FMT_NV12M_P010:
	case V4L2_PIX_FMT_NV21M_P010:
		if (plane_index == 0)
			return ALIGN(pixm->plane_fmt[plane_index].bytesperline, 16) *
				pixm->height;
		else
			return (ALIGN(pixm->plane_fmt[plane_index].bytesperline, 16) *
				pixm->height) >> 1;

	case V4L2_PIX_FMT_NV16M_P210:
	case V4L2_PIX_FMT_NV61M_P210:
		return ALIGN(pixm->plane_fmt[plane_index].bytesperline, 16) *
			pixm->height;

#ifdef EXYNOS_GDC_SUPPORT_V4L2_PIX_FMT_P010
	/* single-fd 420 10-bit formats */
	case V4L2_PIX_FMT_P010:
		return (ALIGN(pixm->plane_fmt[plane_index].bytesperline, 16) *
			pixm->height * 3) >> 1;
#endif
	/* single-fd 420 8-bit formats */
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		return (ALIGN(pixm->plane_fmt[plane_index].bytesperline, 16) *
			pixm->height * 3) >> 1;

	/* single-fd 422 8-bit formats */
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
		return ALIGN(pixm->plane_fmt[plane_index].bytesperline, 16) *
			pixm->height * 2;

	default:
		return pixm->plane_fmt[plane_index].bytesperline * pixm->height;
	}

}

static int gdc_fill_sizeimage(
	const struct gdc_ctx *ctx,
	const struct gdc_fmt *gdc_fmt,
	struct gdc_frame *frame,
	struct v4l2_pix_format_mplane *pixm)
{
	int i, sbwc_size;

	/* The pixm->plane_fmt[i].sizeimage for the plane which
	 * contains the src blend data has to be calculated as per the
	 * size of the actual width and actual height of the src blend
	 * buffer
	 */
	for (i = 0; i < pixm->num_planes; ++i) {
		if (gdc_fmt_is_ayv12(gdc_fmt->pixelformat)) {
			unsigned int y_size, c_span;

			y_size = pixm->width * pixm->height;
			c_span = ALIGN(pixm->width >> 1, 16);
			pixm->plane_fmt[i].sizeimage = y_size + (c_span * pixm->height >> 1) * 2;
		} else if (!camerapp_hw_check_sbwc_fmt(gdc_fmt->pixelformat)) {
			if (i == 0)
				sbwc_size = camerapp_hw_get_comp_buf_size(
					ctx->gdc_dev, frame, pixm->width, pixm->height,
					gdc_fmt->pixelformat, GDC_PLANE_LUMA,
					GDC_SBWC_SIZE_ALL);
			else /* i == 1 */
				sbwc_size = camerapp_hw_get_comp_buf_size(
					ctx->gdc_dev, frame, pixm->width, pixm->height,
					gdc_fmt->pixelformat, GDC_PLANE_CHROMA,
					GDC_SBWC_SIZE_ALL);

			if (sbwc_size >= 0) {
				pixm->plane_fmt[i].sizeimage = sbwc_size;
			} else {
				v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
						"not supported format type\n");
				return sbwc_size;
			}
		} else {
			pixm->plane_fmt[i].sizeimage =
				gdc_get_sizeimage_of_plain_format(gdc_fmt, i, pixm);
		}

		v4l2_dbg(1, gdc_log_level, &ctx->gdc_dev->m2m.v4l2_dev,
			 "[%d] plane: bytesperline %d, sizeimage %d\n", i,
			 pixm->plane_fmt[i].bytesperline, pixm->plane_fmt[i].sizeimage);
	}

	return 0;
}

static int gdc_v4l2_g_fmt_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	const struct gdc_fmt *gdc_fmt;
	struct gdc_frame *frame;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	int i;
	int ret = 0;

	gdc_dev_dbg(ctx->gdc_dev->dev, "gdc g_fmt_mplane\n");

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	gdc_fmt = frame->gdc_fmt;

	pixm->width = frame->width;
	pixm->height = frame->height;
	pixm->pixelformat = frame->pixelformat;
	pixm->field = V4L2_FIELD_NONE;
	pixm->num_planes = frame->gdc_fmt->num_planes;
	pixm->colorspace = 0;

	for (i = 0; i < pixm->num_planes; ++i)
		pixm->plane_fmt[i].bytesperline = (pixm->width * gdc_fmt->bitperpixel[i]) >> 3;

	ret = gdc_fill_sizeimage(ctx, gdc_fmt, frame, pixm);
	if (ret)
		return ret;

	return 0;
}

static int gdc_v4l2_try_fmt_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	const struct gdc_fmt *gdc_fmt;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	const struct gdc_size_limit *limit;
	struct gdc_frame *frame;
	int i;
	int h_align = 0;
	int w_align = 0;
	int ret = 0;

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev, "not supported v4l2 type\n");
		return -EINVAL;
	}

	gdc_fmt = gdc_find_format(ctx->gdc_dev, f->fmt.pix_mp.pixelformat,
				  V4L2_TYPE_IS_OUTPUT(f->type));
	if (!gdc_fmt) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			 "not supported format type, pixelformat(%c%c%c%c)\n",
			 (char)((f->fmt.pix_mp.pixelformat >> 0) & 0xFF),
			 (char)((f->fmt.pix_mp.pixelformat >> 8) & 0xFF),
			 (char)((f->fmt.pix_mp.pixelformat >> 16) & 0xFF),
			 (char)((f->fmt.pix_mp.pixelformat >> 24) & 0xFF));
		return -EINVAL;
	}

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		limit = &ctx->gdc_dev->variant->limit_input;
	else
		limit = &ctx->gdc_dev->variant->limit_output;

	/* TODO: check */
	w_align = gdc_fmt->h_shift;
	h_align = gdc_fmt->v_shift;

	/* Bound an image to have width and height in limit */
	v4l_bound_align_image(&pixm->width, limit->min_w, limit->max_w, w_align, &pixm->height,
			      limit->min_h, limit->max_h, h_align, 0);

	for (i = 0; i < gdc_fmt->num_planes; ++i)
		pixm->plane_fmt[i].bytesperline = (pixm->width * gdc_fmt->bitperpixel[i]) >> 3;

	ret = gdc_fill_sizeimage(ctx, gdc_fmt, frame, pixm);
	if (ret)
		return ret;

	return 0;
}

static int gdc_image_bound_check(struct gdc_ctx *ctx, enum v4l2_buf_type type,
				 struct v4l2_pix_format_mplane *pixm)
{
	const struct gdc_size_limit *limitout = &ctx->gdc_dev->variant->limit_input;
	const struct gdc_size_limit *limitcap = &ctx->gdc_dev->variant->limit_output;

	if (V4L2_TYPE_IS_OUTPUT(type) &&
	    ((pixm->width > limitout->max_w) || (pixm->height > limitout->max_h))) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			 "%dx%d of source image is not supported: too large\n", pixm->width,
			 pixm->height);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_OUTPUT(type) &&
	    ((pixm->width > limitcap->max_w) || (pixm->height > limitcap->max_h))) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			 "%dx%d of target image is not supported: too large\n", pixm->width,
			 pixm->height);
		return -EINVAL;
	}

	if (V4L2_TYPE_IS_OUTPUT(type) &&
	    ((pixm->width < limitout->min_w) || (pixm->height < limitout->min_h))) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			 "%dx%d of source image is not supported: too small\n", pixm->width,
			 pixm->height);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_OUTPUT(type) &&
	    ((pixm->width < limitcap->min_w) || (pixm->height < limitcap->min_h))) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			 "%dx%d of target image is not supported: too small\n", pixm->width,
			 pixm->height);
		return -EINVAL;
	}

	return 0;
}

static int gdc_v4l2_s_fmt_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	struct vb2_queue *vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	struct gdc_frame *frame;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	int i, ret = 0;

	gdc_dev_dbg(ctx->gdc_dev->dev, "gdc s_fmt_mplane\n");

	if (vb2_is_streaming(vq)) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev, "device is busy\n");
		return -EBUSY;
	}

	ret = gdc_v4l2_try_fmt_mplane(file, fh, f);
	if (ret < 0)
		return ret;

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	set_bit(CTX_PARAMS, &ctx->flags);

	frame->gdc_fmt = gdc_find_format(ctx->gdc_dev, f->fmt.pix_mp.pixelformat,
					 V4L2_TYPE_IS_OUTPUT(f->type));
	if (!frame->gdc_fmt) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev, "not supported format values\n");
		return -EINVAL;
	}

	frame->num_planes = frame->gdc_fmt->num_planes;

	for (i = 0; i < frame->num_planes; i++)
		frame->bytesused[i] = pixm->plane_fmt[i].sizeimage;

	ret = gdc_image_bound_check(ctx, f->type, pixm);
	if (ret)
		return ret;

	frame->width = pixm->width;
	frame->height = pixm->height;
	frame->pixelformat = pixm->pixelformat;

	/* Set the SBWC flag */
	frame->pixel_size = (pixm->flags & PIXEL_TYPE_SIZE_MASK) >> PIXEL_TYPE_SIZE_SHIFT;
	frame->extra = (pixm->flags & PIXEL_TYPE_EXTRA_MASK) >> PIXEL_TYPE_EXTRA_SHIFT;
	gdc_dev_dbg(ctx->gdc_dev->dev,
		"pixelformat(%c%c%c%c) size(%dx%d) pixel_size(%d) extra(%d)\n",
		(char)((frame->gdc_fmt->pixelformat >> 0) & 0xFF),
		(char)((frame->gdc_fmt->pixelformat >> 8) & 0xFF),
		(char)((frame->gdc_fmt->pixelformat >> 16) & 0xFF),
		(char)((frame->gdc_fmt->pixelformat >> 24) & 0xFF),
		frame->width, frame->height, frame->pixel_size, frame->extra);

	/* Check constraints for SBWC */
	/* Remark : plain type is filtered at first condition. */
	if (frame->extra && camerapp_hw_get_sbwc_constraint(frame->gdc_fmt->pixelformat,
							    frame->width, frame->height, f->type))
		return -EINVAL;

	return 0;
}

static int gdc_v4l2_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *reqbufs)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);

	gdc_dev_dbg(ctx->gdc_dev->dev, "v4l2_reqbuf\n");
	return v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
}

static int gdc_v4l2_querybuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);

	gdc_dev_dbg(ctx->gdc_dev->dev, "v4l2_querybuf\n");
	return v4l2_m2m_querybuf(file, ctx->m2m_ctx, buf);
}

static int gdc_check_vb2_qbuf(struct vb2_queue *q, struct v4l2_buffer *b)
{
	struct vb2_buffer *vb;
	struct vb2_plane planes[VB2_MAX_PLANES];
	int plane;
	int ret = 0;

	if (q->fileio) {
		gdc_info("file io in progress\n");
		ret = -EBUSY;
		goto q_err;
	}

	if (b->type != q->type) {
		gdc_info("buf type is invalid(%d != %d)\n", b->type, q->type);
		ret = -EINVAL;
		goto q_err;
	}

	if (b->index >= q->num_buffers) {
		gdc_info("buffer index out of range b_index(%d) q_num_buffers(%d)\n", b->index,
			 q->num_buffers);
		ret = -EINVAL;
		goto q_err;
	}

	if (q->bufs[b->index] == NULL) {
		/* Should never happen */
		gdc_info("buffer is NULL\n");
		ret = -EINVAL;
		goto q_err;
	}

	if (b->memory != q->memory) {
		gdc_info("invalid memory type b_mem(%d) q_mem(%d)\n", b->memory, q->memory);
		ret = -EINVAL;
		goto q_err;
	}

	vb = q->bufs[b->index];
	if (!vb) {
		gdc_info("vb is NULL");
		ret = -EINVAL;
		goto q_err;
	}

	if (V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
		/* Is memory for copying plane information present? */
		if (b->m.planes == NULL) {
			gdc_info("multi-planar buffer passed but planes array not provided\n");
			ret = -EINVAL;
			goto q_err;
		}

		if (b->length < vb->num_planes || b->length > VB2_MAX_PLANES) {
			gdc_info("incorrect planes array length, expected %d, got %d\n",
				 vb->num_planes, b->length);
			ret = -EINVAL;
			goto q_err;
		}
	}

	if ((b->flags & V4L2_BUF_FLAG_REQUEST_FD) && vb->state != VB2_BUF_STATE_DEQUEUED) {
		gdc_info("buffer is not in dequeued state\n");
		ret = -EINVAL;
		goto q_err;
	}

	/* for detect vb2 framework err, operate some vb2 functions */
	memset(planes, 0, sizeof(planes[0]) * vb->num_planes);
	/* Copy relevant information provided by the userspace */
	ret = call_bufop(vb->vb2_queue, fill_vb2_buffer, vb, planes);
	if (ret) {
		gdc_info("vb2_fill_vb2_v4l2_buffer failed (%d)\n", ret);
		goto q_err;
	}

	for (plane = 0; plane < vb->num_planes; ++plane) {
		struct dma_buf *dbuf;

		dbuf = dma_buf_get(planes[plane].m.fd);
		if (IS_ERR_OR_NULL(dbuf)) {
			gdc_info("invalid dmabuf fd(%d) for plane %d\n", planes[plane].m.fd, plane);
			ret = -EINVAL;
			goto q_err;
		}

		if (planes[plane].length == 0)
			planes[plane].length = (unsigned int)dbuf->size;

		if (planes[plane].length < vb->planes[plane].min_length) {
			gdc_info("invalid dmabuf length %u for plane %d, minimum length %u\n",
				 planes[plane].length, plane, vb->planes[plane].min_length);
			ret = -EINVAL;
			dma_buf_put(dbuf);
			goto q_err;
		}
		dma_buf_put(dbuf);
	}
q_err:
	return ret;
}

static int gdc_check_qbuf(struct file *file, struct v4l2_m2m_ctx *m2m_ctx, struct v4l2_buffer *buf)
{
	struct vb2_queue *vq;

	vq = v4l2_m2m_get_vq(m2m_ctx, buf->type);
	if (!V4L2_TYPE_IS_OUTPUT(vq->type) && (buf->flags & V4L2_BUF_FLAG_REQUEST_FD)) {
		gdc_info("requests cannot be used with capture buffers\n");
		return -EPERM;
	}
	return gdc_check_vb2_qbuf(vq, buf);
}

static int gdc_v4l2_qbuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	int ret;

#if IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON)
#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)
	/* Save flags for cache sync of V4L2_MEMORY_DMABUF */
	if (!V4L2_TYPE_IS_OUTPUT(buf->type))
		ctx->hdr10p_ctx[GDC_BUF_IDX_TO_PARAM_IDX(buf->index)].flags = buf->flags;
#endif
#endif
	gdc_dev_dbg(ctx->gdc_dev->dev, "buf->type=%d, buf->index=%d\n", buf->type, buf->index);

	ret = v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);
	if (ret) {
		gdc_dev_err(ctx->gdc_dev->dev, "v4l2_m2m_qbuf failed ret(%d) check(%d)\n", ret,
			gdc_check_qbuf(file, ctx->m2m_ctx, buf));
	}
	return ret;
}

static int gdc_v4l2_dqbuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	int ret = 0;

	ret = v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);
	if (ret)
		gdc_dev_err(ctx->gdc_dev->dev, "v4l2_m2m_dqbuf failed ret(%d)\n", ret);

	gdc_dev_dbg(ctx->gdc_dev->dev, "buf->type=%d, buf->index=%d\n", buf->type, buf->index);
	return ret;
}

static int gdc_power_clk_enable(struct gdc_dev *gdc)
{
	int ret;

	if (in_interrupt())
		ret = pm_runtime_get(gdc->dev);
	else
		ret = pm_runtime_get_sync(gdc->dev);

	if (ret < 0) {
		gdc_dev_err(gdc->dev, "Failed to enable local power (err %d)\n", ret);
		return ret;
	}

	if (!IS_ERR(gdc->pclk)) {
		ret = clk_prepare_enable(gdc->pclk);
		if (ret) {
			gdc_dev_err(gdc->dev, "Failed to enable PCLK (err %d)\n", ret);
			goto err_pclk;
		}
	}

	if (!IS_ERR(gdc->aclk)) {
		ret = clk_prepare_enable(gdc->aclk);
		if (ret) {
			gdc_dev_err(gdc->dev, "Failed to enable ACLK (err %d)\n", ret);
			goto err_aclk;
		}
	}
	camerapp_hw_gdc_cfg_after_poweron(gdc);
	return 0;
err_aclk:
	if (!IS_ERR(gdc->pclk))
		clk_disable_unprepare(gdc->pclk);
err_pclk:
	pm_runtime_put(gdc->dev);
	return ret;
}

static void gdc_power_clk_disable(struct gdc_dev *gdc)
{
	camerapp_hw_gdc_stop(gdc->pmio);

	if (!IS_ERR(gdc->aclk))
		clk_disable_unprepare(gdc->aclk);

	if (!IS_ERR(gdc->pclk))
		clk_disable_unprepare(gdc->pclk);

	pm_runtime_put(gdc->dev);
}

static struct pablo_gdc_v4l2_ops gdc_v4l2_ops = {
	.m2m_streamon = v4l2_m2m_streamon,
	.m2m_streamoff = v4l2_m2m_streamoff,
};

static int gdc_v4l2_streamon(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	struct gdc_dev *gdc = ctx->gdc_dev;
	int ret;

	gdc_dev_dbg(gdc->dev, "v4l2_stream_on\n");

	if (gdc->stalled) {
		gdc->stalled = 0;
		gdc_dev_dbg(gdc->dev, "stall state clear\n");
	}

	if (!V4L2_TYPE_IS_OUTPUT(type)) {
		ret = gdc_power_clk_enable(gdc);
		if (ret)
			return ret;
		gdc_dev_dbg(gdc->dev, "gdc clk enable\n");
	}

	return gdc->v4l2_ops->m2m_streamon(file, ctx->m2m_ctx, type);
}

static int gdc_v4l2_streamoff(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	struct gdc_dev *gdc = ctx->gdc_dev;

	gdc_dev_dbg(gdc->dev, "v4l2_stream_off\n");

	if (!V4L2_TYPE_IS_OUTPUT(type)) {
		if (atomic_read(&gdc->m2m.in_use) == 1)
			gdc->hw_gdc_ops->sw_reset(gdc->pmio);

		gdc_power_clk_disable(gdc);
	}

	return gdc->v4l2_ops->m2m_streamoff(file, ctx->m2m_ctx, type);
}

static int gdc_v4l2_s_ctrl(struct file *file, void *priv, struct v4l2_control *ctrl)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);
	struct gdc_crop_param *crop_param = ctx->crop_param;
	int ret = 0;

	gdc_dev_dbg(ctx->gdc_dev->dev, "v4l2_s_ctrl = %d (%d)\n", ctrl->id, ctrl->value);

	switch (ctrl->id) {
	case V4L2_CID_CAMERAPP_GDC_GRID_CROP_START:
		crop_param->crop_start_x = (ctrl->value & 0xFFFF0000) >> 16;
		crop_param->crop_start_y = (ctrl->value & 0x0000FFFF);
		break;
	case V4L2_CID_CAMERAPP_GDC_GRID_CROP_SIZE:
		crop_param->crop_width = (ctrl->value & 0xFFFF0000) >> 16;
		crop_param->crop_height = (ctrl->value & 0x0000FFFF);
		break;
	case V4L2_CID_CAMERAPP_GDC_GRID_SENSOR_SIZE:
		crop_param->sensor_width = (ctrl->value & 0xFFFF0000) >> 16;
		crop_param->sensor_height = (ctrl->value & 0x0000FFFF);
		break;
	case V4L2_CID_CAMERAPP_SENSOR_NUM:
		crop_param->sensor_num = ctrl->value;
		gdc_dev_dbg(ctx->gdc_dev->dev, "sensor number = %d\n", crop_param->sensor_num);
		break;
	default:
		ret = -EINVAL;
		gdc_dev_dbg(ctx->gdc_dev->dev, "Err: Invalid ioctl id(%d)\n", ctrl->id);
		break;
	}

	return ret;
}

static struct pablo_gdc_sys_ops gdc_sys_ops = {
	.copy_from_user = copy_from_user,
};

static int gdc_v4l2_s_ext_ctrls(struct file *file, void *priv, struct v4l2_ext_controls *ctrls)
{
	int ret = 0;
	int i;
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);
	struct gdc_dev *gdc = ctx->gdc_dev;
	struct gdc_crop_param *crop_param;

	struct v4l2_ext_control *ext_ctrl;
	struct v4l2_control ctrl;
	u32 param_index;

	gdc_dev_dbg(gdc->dev, "v4l2_s_ext_ctrl\n");

	BUG_ON(!ctx);

	for (i = 0; i < ctrls->count; i++) {
		ext_ctrl = (ctrls->controls + i);

		gdc_dev_dbg(gdc->dev, "ctrl ID:%d\n", ext_ctrl->id);
		switch (ext_ctrl->id) {
		case V4L2_CID_CAMERAPP_GDC_GRID_CONTROL: {
			ret = gdc->sys_ops->copy_from_user(ctx->crop_param, ext_ctrl->ptr,
							   sizeof(struct gdc_crop_param));
			/*
			 * Copy memory to keep the data for multi-buffer
			 * To support both of multi-buffer and legacy,
			 * crop_param[buffer index + 1] will be used.
			 * e.g.
			 * (multi-buffer) buffer index is 0...n and use crop_param[1...n+1]
			 * (not multi-buffer) buffer index is 0 and use crop_param[1]
			 */
			param_index = GDC_BUF_IDX_TO_PARAM_IDX(ctx->crop_param->buf_index);
			gdc_dev_dbg(gdc->dev, "buf_index=%u, param_index=%u\n",
				ctx->crop_param->buf_index, param_index);
			crop_param = (struct gdc_crop_param *)&ctx->crop_param[param_index];
			memcpy(crop_param, &ctx->crop_param, sizeof(struct gdc_crop_param));

			crop_param->is_crop_dzoom = false;

			if ((crop_param->crop_width != crop_param->sensor_width) ||
			    (crop_param->crop_height != crop_param->sensor_height))
				crop_param->is_crop_dzoom = true;
		} break;

#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)
		case V4L2_CID_CAMERAPP_GDC_HDR10P_CONTROL: {
			gdc_hdr10p_set_ext_ctrls(ctx, ext_ctrl);
		} break;
#endif
		default:
			ctrl.id = ext_ctrl->id;
			ctrl.value = ext_ctrl->value;

			ret = gdc_v4l2_s_ctrl(file, ctx, &ctrl);
			if (ret) {
				gdc_dev_dbg(gdc->dev, "gdc_v4l2_s_ctrl is fail(%d)\n", ret);
				goto p_err;
			}
			break;
		}
	}

p_err:
	return ret;
}

static const struct v4l2_ioctl_ops gdc_v4l2_ioctl_ops = {
	.vidioc_querycap = gdc_v4l2_querycap,

	.vidioc_g_fmt_vid_cap_mplane = gdc_v4l2_g_fmt_mplane,
	.vidioc_g_fmt_vid_out_mplane = gdc_v4l2_g_fmt_mplane,

	.vidioc_try_fmt_vid_cap_mplane = gdc_v4l2_try_fmt_mplane,
	.vidioc_try_fmt_vid_out_mplane = gdc_v4l2_try_fmt_mplane,

	.vidioc_s_fmt_vid_cap_mplane = gdc_v4l2_s_fmt_mplane,
	.vidioc_s_fmt_vid_out_mplane = gdc_v4l2_s_fmt_mplane,

	.vidioc_reqbufs = gdc_v4l2_reqbufs,
	.vidioc_querybuf = gdc_v4l2_querybuf,

	.vidioc_qbuf = gdc_v4l2_qbuf,
	.vidioc_dqbuf = gdc_v4l2_dqbuf,

	.vidioc_streamon = gdc_v4l2_streamon,
	.vidioc_streamoff = gdc_v4l2_streamoff,

	.vidioc_s_ctrl = gdc_v4l2_s_ctrl,
	.vidioc_s_ext_ctrls = gdc_v4l2_s_ext_ctrls,
};

static int gdc_ctx_stop_req(struct gdc_ctx *ctx)
{
	struct gdc_ctx *curr_ctx;
	struct gdc_dev *gdc = ctx->gdc_dev;
	int ret = 0;

	curr_ctx = v4l2_m2m_get_curr_priv(gdc->m2m.m2m_dev);
	if (!test_bit(CTX_RUN, &ctx->flags) || (curr_ctx != ctx))
		return 0;

	set_bit(CTX_ABORT, &ctx->flags);

	ret = wait_event_timeout(gdc->wait, !test_bit(CTX_RUN, &ctx->flags), GDC_TIMEOUT);

	/* TODO: How to handle case of timeout event */
	if (ret == 0) {
		gdc_dev_err(gdc->dev, "device failed to stop request\n");
		ret = -EBUSY;
	}

	return ret;
}

static int gdc_vb2_queue_setup(struct vb2_queue *vq, unsigned int *num_buffers,
			       unsigned int *num_planes, unsigned int sizes[],
			       struct device *alloc_devs[])
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);
	struct gdc_frame *frame;
	int i;

	gdc_dev_dbg(ctx->gdc_dev->dev, "gdc queue setup\n");
	frame = ctx_get_frame(ctx, vq->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	/* Get number of planes from format_list in driver */
	*num_planes = frame->num_planes;
	for (i = 0; i < *num_planes; i++) {
		sizes[i] = frame->bytesused[i];
		alloc_devs[i] = ctx->gdc_dev->dev;
	}

	return 0;
}

static int gdc_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct gdc_frame *frame;
	int i, ret = 0;

	frame = ctx_get_frame(ctx, vb->vb2_queue->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		for (i = 0; i < frame->gdc_fmt->num_planes; i++)
			vb2_set_plane_payload(vb, i, frame->bytesused[i]);
	}

#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)
	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type))
		ret = gdc_hdr10p_buf_prepare(ctx, vb->index);
#endif

	return ret;
}

static void gdc_vb2_buf_finish(struct vb2_buffer *vb)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type) &&
	    unlikely(test_bit(GDC_DBG_TIME, &debug_gdc))) {
		gdc_dev_info(ctx->gdc_dev->dev,
			"index(%d) shot_time %lld us, hw_time %lld us\n",
			vb->index, ctx->time_dbg.shot_time_stamp, ctx->time_dbg.hw_time_stamp);
	}

#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)
	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type))
		gdc_hdr10p_buf_finish(ctx, vb->index);
#endif
}

static void gdc_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);

	gdc_dev_dbg(ctx->gdc_dev->dev, "gdc buf_queue\n");

	if (ctx->m2m_ctx)
		v4l2_m2m_buf_queue(ctx->m2m_ctx, v4l2_buf);
}

static void gdc_vb2_buf_cleanup(struct vb2_buffer *vb)
{
	/* No operation */
}

static void gdc_vb2_lock(struct vb2_queue *vq)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);

	mutex_lock(&ctx->gdc_dev->lock);
}

static void gdc_vb2_unlock(struct vb2_queue *vq)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);

	mutex_unlock(&ctx->gdc_dev->lock);
}

static void gdc_cleanup_queue(struct gdc_ctx *ctx)
{
	struct vb2_v4l2_buffer *src_vb, *dst_vb;

	while (v4l2_m2m_num_src_bufs_ready(ctx->m2m_ctx) > 0) {
		src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		v4l2_m2m_buf_done(src_vb, VB2_BUF_STATE_ERROR);
		gdc_dev_dbg(ctx->gdc_dev->dev, "src_index(%d)\n", src_vb->vb2_buf.index);
	}

	while (v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx) > 0) {
		dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		v4l2_m2m_buf_done(dst_vb, VB2_BUF_STATE_ERROR);
		gdc_dev_dbg(ctx->gdc_dev->dev, "dst_index(%d)\n", dst_vb->vb2_buf.index);
	}
}

static int gdc_vb2_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);

	set_bit(CTX_STREAMING, &ctx->flags);

	return 0;
}

static void gdc_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);
	int ret;

	ret = gdc_ctx_stop_req(ctx);
	if (ret < 0)
		gdc_dev_err(ctx->gdc_dev->dev, "wait timeout\n");

	clear_bit(CTX_STREAMING, &ctx->flags);

	/* release all queued buffers in multi-buffer scenario*/
	gdc_cleanup_queue(ctx);
}

static const struct vb2_ops gdc_vb2_ops = {
	.queue_setup = gdc_vb2_queue_setup,
	.buf_prepare = gdc_vb2_buf_prepare,
	.buf_finish = gdc_vb2_buf_finish,
	.buf_queue = gdc_vb2_buf_queue,
	.buf_cleanup = gdc_vb2_buf_cleanup,
	.wait_finish = gdc_vb2_lock,
	.wait_prepare = gdc_vb2_unlock,
	.start_streaming = gdc_vb2_start_streaming,
	.stop_streaming = gdc_vb2_stop_streaming,
};

static void set_vb2_queue(struct vb2_queue *vq, unsigned int type, struct gdc_ctx *ctx)
{
	memset(vq, 0, sizeof(*vq));
	vq->type = type;
	vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	vq->ops = &gdc_vb2_ops;
	vq->mem_ops = &vb2_dma_sg_memops;
	vq->drv_priv = ctx;
	vq->buf_struct_size = (unsigned int)sizeof(struct vb2_gdc_buffer);
	vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
}

static int queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	struct gdc_ctx *ctx = priv;
	int ret;

	set_vb2_queue(src_vq, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, ctx);

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	set_vb2_queue(dst_vq, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, ctx);

	return vb2_queue_init(dst_vq);
}

static struct is_mem *__get_iommu_mem(struct gdc_dev *gdc)
{
	struct is_mem *mem;
	struct pablo_device_iommu_group *iommu_group;

	if (gdc->use_cloader_iommu_group) {
		iommu_group = pablo_iommu_group_get(gdc->cloader_iommu_group_id);
		mem = &iommu_group->mem;
	} else {
		mem = &gdc->mem;
	}

	return mem;
}

static int gdc_alloc_pmio_mem(struct gdc_dev *gdc)
{
	struct is_mem *mem;

	mem = __get_iommu_mem(gdc);

	gdc->pb_c_loader_payload = CALL_PTR_MEMOP(mem, alloc, mem->priv, 0x8000, NULL, 0);
	if (IS_ERR_OR_NULL(gdc->pb_c_loader_payload)) {
		gdc_dev_err(gdc->dev, "failed to allocate buffer for c-loader payload");
		gdc->pb_c_loader_payload = NULL;
		return -ENOMEM;
	}

	gdc->kva_c_loader_payload =
		CALL_BUFOP(gdc->pb_c_loader_payload, kvaddr, gdc->pb_c_loader_payload);
	gdc->dva_c_loader_payload =
		CALL_BUFOP(gdc->pb_c_loader_payload, dvaddr, gdc->pb_c_loader_payload);

	gdc->pb_c_loader_header = CALL_PTR_MEMOP(mem, alloc, mem->priv, 0x2000, NULL, 0);
	if (IS_ERR_OR_NULL(gdc->pb_c_loader_header)) {
		gdc_dev_err(gdc->dev, "failed to allocate buffer for c-loader header");
		gdc->pb_c_loader_header = NULL;
		CALL_BUFOP(gdc->pb_c_loader_payload, free, gdc->pb_c_loader_payload);
		return -ENOMEM;
	}

	gdc->kva_c_loader_header =
		CALL_BUFOP(gdc->pb_c_loader_header, kvaddr, gdc->pb_c_loader_header);
	gdc->dva_c_loader_header =
		CALL_BUFOP(gdc->pb_c_loader_header, dvaddr, gdc->pb_c_loader_header);

	gdc_dev_info(gdc->dev, "payload_dva(0x%llx) header_dva(0x%llx)\n",
		gdc->dva_c_loader_payload, gdc->dva_c_loader_header);

	return 0;
}

static void gdc_free_pmio_mem(struct gdc_dev *gdc)
{
	if (!IS_ERR_OR_NULL(gdc->pb_c_loader_payload))
		CALL_BUFOP(gdc->pb_c_loader_payload, free, gdc->pb_c_loader_payload);
	if (!IS_ERR_OR_NULL(gdc->pb_c_loader_header))
		CALL_BUFOP(gdc->pb_c_loader_header, free, gdc->pb_c_loader_header);
}

static int gdc_open(struct file *file)
{
	struct gdc_dev *gdc = video_drvdata(file);
	struct gdc_ctx *ctx;
	int ret = 0;

	ctx = vzalloc(sizeof(struct gdc_ctx));

	if (!ctx)
		return -ENOMEM;

	mutex_lock(&gdc->m2m.lock);

	gdc_dev_info(gdc->dev, "gdc open refcnt = %d\n", atomic_read(&gdc->m2m.in_use));

	if (!atomic_read(&gdc->m2m.in_use) && gdc->pmio_en) {
		ret = gdc_alloc_pmio_mem(gdc);
		if (ret) {
			gdc_dev_err(gdc->dev, "PMIO mem alloc failed\n");
			mutex_unlock(&gdc->m2m.lock);
			goto err_alloc_pmio_mem;
		}
	}

	atomic_inc(&gdc->m2m.in_use);

	mutex_unlock(&gdc->m2m.lock);

	INIT_LIST_HEAD(&ctx->node);
	ctx->gdc_dev = gdc;

	v4l2_fh_init(&ctx->fh, gdc->m2m.vfd);
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	/* Default color format */
	ctx->s_frame.gdc_fmt = &gdc_formats[0];
	ctx->d_frame.gdc_fmt = &gdc_formats[0];
	ctx->use_mfc_votf_ops = false;

	if (!IS_ERR(gdc->pclk)) {
		ret = clk_prepare(gdc->pclk);
		if (ret) {
			gdc_dev_err(gdc->dev, "Failed to prepare PCLK(err %d)\n", ret);
			goto err_pclk_prepare;
		}
	}

	if (!IS_ERR(gdc->aclk)) {
		ret = clk_prepare(gdc->aclk);
		if (ret) {
			gdc_dev_err(gdc->dev, "Failed to prepare ACLK(err %d)\n", ret);
			goto err_aclk_prepare;
		}
	}

	/* Setup the device context for mem2mem mode. */
	ctx->m2m_ctx = v4l2_m2m_ctx_init(gdc->m2m.m2m_dev, ctx, queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = -EINVAL;
		gdc_dev_err(gdc->dev, "Failed to v4l2_m2m_ctx_init(ret=%p)\n", ctx->m2m_ctx);
		goto err_ctx;
	}

	gdc_dev_info(gdc->dev, "completed.\n");
	return 0;

err_ctx:
	if (!IS_ERR(gdc->aclk))
		clk_unprepare(gdc->aclk);
err_aclk_prepare:
	if (!IS_ERR(gdc->pclk))
		clk_unprepare(gdc->pclk);
err_pclk_prepare:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	atomic_dec(&gdc->m2m.in_use);
	gdc_free_pmio_mem(gdc);
err_alloc_pmio_mem:
	vfree(ctx);

	gdc_dev_info(gdc->dev, "completed with error.(%d)\n", ret);
	return ret;
}

void gdc_job_finish(struct gdc_dev *gdc, struct gdc_ctx *ctx)
{
	unsigned long flags;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;

	spin_lock_irqsave(&gdc->slock, flags);

	ctx = v4l2_m2m_get_curr_priv(gdc->m2m.m2m_dev);
	if (!ctx || !ctx->m2m_ctx) {
		gdc_dev_err(gdc->dev, "current ctx is NULL\n");
		spin_unlock_irqrestore(&gdc->slock, flags);
		return;
	}
	clear_bit(CTX_RUN, &ctx->flags);

	src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

	BUG_ON(!src_vb || !dst_vb);

	v4l2_m2m_buf_done(src_vb, VB2_BUF_STATE_ERROR);
	v4l2_m2m_buf_done(dst_vb, VB2_BUF_STATE_ERROR);

	v4l2_m2m_job_finish(gdc->m2m.m2m_dev, ctx->m2m_ctx);

	spin_unlock_irqrestore(&gdc->slock, flags);
}

static int gdc_release(struct file *file)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);
	struct gdc_dev *gdc = ctx->gdc_dev;
	struct vb2_queue *src_vq = v4l2_m2m_get_vq(ctx->m2m_ctx,
					V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	struct vb2_queue *dst_vq = v4l2_m2m_get_vq(ctx->m2m_ctx,
					V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	if (src_vq && vb2_is_streaming(src_vq)) {
		gdc_dev_info(gdc->dev, "SRC is in stream-on state\n");
		gdc_v4l2_streamoff(file, (void *)(&ctx->fh), V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	}

	if (dst_vq && vb2_is_streaming(dst_vq)) {
		gdc_dev_info(gdc->dev, "DST is in stream-on state\n");
		gdc_v4l2_streamoff(file, (void *)(&ctx->fh), V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	}

	if (ctx->use_mfc_votf_ops)
		gdc_clear_mfc_votf_ops(gdc);

	mutex_lock(&gdc->m2m.lock);

	gdc_dev_info(gdc->dev, "gdc close refcnt = %d\n", atomic_read(&gdc->m2m.in_use));

	atomic_dec(&gdc->m2m.in_use);

	if (!atomic_read(&gdc->m2m.in_use) && test_bit(DEV_RUN, &gdc->state)) {
		gdc_dev_err(gdc->dev, "gdc is still running\n");
		gdc_suspend(gdc->dev);
	}

	if (!atomic_read(&gdc->m2m.in_use) && gdc->pmio_en)
		gdc_free_pmio_mem(gdc);

	mutex_unlock(&gdc->m2m.lock);

	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	if (!IS_ERR(gdc->aclk))
		clk_unprepare(gdc->aclk);
	if (!IS_ERR(gdc->pclk))
		clk_unprepare(gdc->pclk);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	vfree(ctx);

	gdc_dev_info(gdc->dev, "completed\n");

	return 0;
}

static unsigned int gdc_poll(struct file *file, struct poll_table_struct *wait)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);

	return v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
}

static int gdc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);

	return v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);
}

static const struct v4l2_file_operations gdc_v4l2_fops = {
	.owner = THIS_MODULE,
	.open = gdc_open,
	.release = gdc_release,
	.poll = gdc_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = gdc_mmap,
};

static int gdc_get_out_mode(struct gdc_ctx *ctx)
{
	struct vb2_buffer *src_vb = (struct vb2_buffer *)v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	u32 param_index;
	struct gdc_crop_param *crop_param;
	int ret;

	if (!src_vb) {
		gdc_dev_err(ctx->gdc_dev->dev, "src_vb is null\n");
		return -EPERM;
	}

	param_index = GDC_BUF_IDX_TO_PARAM_IDX(
		ctx->crop_param[GDC_BUF_IDX_TO_PARAM_IDX(src_vb->index)].buf_index);

	gdc_dev_dbg(ctx->gdc_dev->dev,
		"buf_index : %u, param_index :%u\n", src_vb->index, param_index);

	crop_param = &ctx->crop_param[param_index];

	if (crop_param->out_mode)
		ret = (int)crop_param->out_mode;
	else
		ret = crop_param->votf_en ? GDC_OUT_VOTF : GDC_OUT_M2M;

	return ret;
}

static void gdc_watchdog(struct timer_list *t)
{
	struct gdc_wdt *wdt = from_timer(wdt, t, timer);
	struct gdc_dev *gdc = container_of(wdt, typeof(*gdc), wdt);
	struct gdc_crop_param *crop_param;
	struct gdc_ctx *ctx;
	unsigned long flags;
	int out_mode;

	if (!test_bit(DEV_RUN, &gdc->state)) {
		gdc_dev_info(gdc->dev, "GDC is not running\n");
		return;
	}

	spin_lock_irqsave(&gdc->ctxlist_lock, flags);
	ctx = gdc->current_ctx;
	if (!ctx) {
		gdc_dev_info(gdc->dev, "ctx is empty\n");
		spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);
		return;
	}

	crop_param = &ctx->crop_param[ctx->cur_index];
	out_mode = gdc_get_out_mode(ctx);

	if (atomic_read(&gdc->wdt.cnt) >= GDC_WDT_CNT) {
		gdc_dev_dbg(gdc->dev, "final timeout\n");
		is_debug_s2d(true, "GDC watchdog s2d");
		if (gdc->has_votf_mfc == 1 && out_mode == GDC_OUT_VOTF) {
			gdc->stalled = 1;
			gdc_votfitf_set_flush(gdc);
			if (votfitf_wrapper_reset(gdc->votf_base))
				gdc_dev_err(gdc->dev, "gdc votf wrapper reset fail\n");
		}
		if (camerapp_hw_gdc_sw_reset(gdc->pmio))
			gdc_dev_err(gdc->dev, "gdc sw reset fail\n");

		atomic_set(&gdc->wdt.cnt, 0);
		clear_bit(DEV_RUN, &gdc->state);
		gdc->current_ctx = NULL;
		gdc->votf_ctx = NULL;
		spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);
		gdc_job_finish(gdc, ctx);
		return;
	}

	if (gdc->has_votf_mfc == 1 && out_mode == GDC_OUT_VOTF) {
		if (atomic_read(&gdc->wdt.cnt) >= 1) {
			if (!votfitf_check_votf_ring(gdc->mfc_votf_base, C2SERV)) {
				gdc_dev_dbg(gdc->dev, "mfc off state\n");
				gdc->stalled = 1;
				/* Do not votf_flush when MFC is turned off */
				if (votfitf_wrapper_reset(gdc->votf_base))
					gdc_dev_err(gdc->dev, "gdc votf wrapper reset fail\n");

				if (camerapp_hw_gdc_sw_reset(gdc->pmio))
					gdc_dev_err(gdc->dev, "gdc sw reset fail\n");

				atomic_set(&gdc->wdt.cnt, 0);
				clear_bit(DEV_RUN, &gdc->state);
				gdc->current_ctx = NULL;
				gdc->votf_ctx = NULL;
				spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);
				gdc_job_finish(gdc, ctx);
				return;
			}
			gdc_dev_dbg(gdc->dev, "Waiting for MFC operation to end\n");
		}
	}
	spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);

	if (test_bit(DEV_RUN, &gdc->state)) {
		if (!atomic_read(&gdc->wdt.cnt))
			camerapp_gdc_sfr_dump(gdc->regs_base);
		atomic_inc(&gdc->wdt.cnt);
		gdc_dev_err(gdc->dev, "gdc is still running\n");
		mod_timer(&gdc->wdt.timer, jiffies + GDC_TIMEOUT);
	} else {
		gdc_dev_dbg(gdc->dev, "gdc finished job\n");
	}
}

static int gdc_votf_create_link(struct gdc_dev *gdc)
{
	int ret = 0;

	votfitf_votf_create_ring(gdc->votf_base, gdc->votf_src_dest[GDC_SRC_ADDR] >> 16, C2SERV);
	votfitf_votf_set_sel_reg(gdc->votf_base, 0x1, 0x1);

	ret = gdc_votf_set_service_config(TWS, gdc);
	if (ret < 0) {
		gdc_dev_dbg(gdc->dev, "create link error (ret:%d)\n", ret);
		return ret;
	}
	return ret;
}

static void gdc_pmio_config(struct gdc_dev *gdc, struct c_loader_buffer *clb)
{
	if (unlikely(test_bit(GDC_DBG_PMIO_MODE, &debug_gdc))) {
		/* APB-DIRECT */
		pmio_cache_sync(gdc->pmio);
		clb->clh = NULL;
		clb->num_of_headers = 0;
	} else {
		/* C-LOADER */
		clb->num_of_headers = 0;
		clb->num_of_values = 0;
		clb->num_of_pairs = 0;
		clb->header_dva = gdc->dva_c_loader_header;
		clb->payload_dva = gdc->dva_c_loader_payload;
		clb->clh = (struct c_loader_header *)gdc->kva_c_loader_header;
		clb->clp = (struct c_loader_payload *)gdc->kva_c_loader_payload;

		pmio_cache_fsync(gdc->pmio, (void *)clb, PMIO_FORMATTER_PAIR);

		if (clb->num_of_pairs > 0)
			clb->num_of_headers++;

		if (unlikely(test_bit(GDC_DBG_DUMP_PMIO_CACHE, &debug_gdc))) {
			gdc_dev_info(gdc->dev, "payload_dva(%pad) header_dva(%pad)\n",
				&clb->payload_dva, &clb->header_dva);
			gdc_dev_info(gdc->dev, "number of headers: %d\n", clb->num_of_headers);
			gdc_dev_info(gdc->dev, "number of pairs: %d\n", clb->num_of_pairs);

			print_hex_dump(KERN_INFO, "header  ", DUMP_PREFIX_OFFSET, 16, 4, clb->clh,
				       clb->num_of_headers * 16, true);

			print_hex_dump(KERN_INFO, "payload ", DUMP_PREFIX_OFFSET, 16, 4, clb->clp,
				       clb->num_of_headers * 64, true);
		}
	}

	CALL_BUFOP(gdc->pb_c_loader_payload, sync_for_device, gdc->pb_c_loader_payload, 0,
		   gdc->pb_c_loader_payload->size, DMA_TO_DEVICE);
	CALL_BUFOP(gdc->pb_c_loader_header, sync_for_device, gdc->pb_c_loader_header, 0,
		   gdc->pb_c_loader_header->size, DMA_TO_DEVICE);
}

#if IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON)
static void gdc_disable_S2MPU(void)
{
	/* SYSMMU_D1_LME_S2 */
	void __iomem *reg;

	gdc_dbg("[GDC]  S2MPU disable (SYSMMU_S0_LME_S2)\n");
	reg = ioremap(0x1D0C0054, 0x4);
	writel(0xFF, reg);
	iounmap(reg);
}
#endif

static int gdc_run_next_job(struct gdc_dev *gdc)
{
	unsigned long flags;
	struct gdc_ctx *ctx;
	struct gdc_crop_param *crop_param;
	struct c_loader_buffer clb;
	int ret;
	int out_mode;

	spin_lock_irqsave(&gdc->ctxlist_lock, flags);

	if (gdc->current_ctx || list_empty(&gdc->context_list)) {
		/* a job is currently being processed or no job is to run */
		spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);
		return 0;
	}

	ctx = list_first_entry(&gdc->context_list, struct gdc_ctx, node);

	list_del_init(&ctx->node);

	clb.header_dva = 0;
	clb.num_of_headers = 0;
	gdc->current_ctx = ctx;
	crop_param = &ctx->crop_param[ctx->cur_index];

	if (gdc->stalled) {
		gdc_dev_err(gdc->dev, "gdc hw stalled!!\n");
		gdc->current_ctx = NULL;
		gdc->votf_ctx = NULL;
		spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);
		gdc_job_finish(gdc, ctx);
		return 0;
	}

	spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);

	/*
	 * gdc_run_next_job() must not reenter while gdc->state is DEV_RUN.
	 * DEV_RUN is cleared when an operation is finished.
	 */
	BUG_ON(test_bit(DEV_RUN, &gdc->state));

	if (gdc->pmio_en)
		pmio_cache_set_only(gdc->pmio, false);

	gdc_dev_dbg(gdc->dev, "gdc hw setting\n");
	ctx->time_dbg.shot_time = ktime_get();
	out_mode = gdc_get_out_mode(gdc->current_ctx);
	gdc_dev_dbg(gdc->dev, "out_mode:%d(%s)\n",
		out_mode, gdc_get_out_mode_name((enum gdc_out_mode)out_mode));

	if (gdc->has_votf_mfc == 1 && out_mode == GDC_OUT_VOTF)
		votfitf_wrapper_reset(gdc->votf_base);

	gdc->hw_gdc_ops->sw_reset(gdc->pmio);
	gdc_dev_dbg(gdc->dev, "gdc sw reset\n");

#if IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON)
	gdc_disable_S2MPU();
#endif

#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)
	gdc_dev_dbg(gdc->dev, "gdc->has_hdr10p : %d", gdc->has_hdr10p);
	gdc_dev_dbg(gdc->dev, "ctx->cur_index : %d", ctx->cur_index);
	gdc_dev_dbg(gdc->dev, "ctx->hdr10p_ctx[ctx->cur_index].param.en : %d",
		ctx->hdr10p_ctx[ctx->cur_index].param.en);

	if (gdc->has_hdr10p && ctx->hdr10p_ctx[ctx->cur_index].param.en)
		camerapp_hw_gdc_hdr10p_init(gdc->pmio);
#endif

	if (gdc->has_votf_mfc == 1 && out_mode == GDC_OUT_VOTF) {
		ret = gdc_votf_create_link(gdc);
		if (ret < 0) {
			gdc_dev_err(gdc->dev, "gdc votf create link fail (ret:%d)\n", ret);

			if (gdc->pmio_en)
				pmio_cache_set_only(gdc->pmio, true);

			return ret;
		}
	}

	gdc->hw_gdc_ops->set_initialization(gdc->pmio);

	if (gdc->pmio_en) {
		pmio_reset_cache(gdc->pmio);
		pmio_cache_set_only(gdc->pmio, true);
	}

	ret = gdc->hw_gdc_ops->update_param(gdc->pmio, gdc);
	if (ret) {
		gdc_dev_err(gdc->dev, "Failed to update gdc param(%d)", ret);
		spin_lock_irqsave(&gdc->ctxlist_lock, flags);
		gdc->current_ctx = NULL;
		gdc->votf_ctx = NULL;
		spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);
		gdc_job_finish(gdc, ctx);
		return ret;
	}
	gdc_dev_dbg(gdc->dev, "gdc param update done\n");

	if (gdc->pmio_en)
		gdc_pmio_config(gdc, &clb);

	set_bit(DEV_RUN, &gdc->state);
	set_bit(CTX_RUN, &ctx->flags);
	mod_timer(&gdc->wdt.timer, jiffies + GDC_TIMEOUT);

	gdc->hw_gdc_ops->start(gdc->pmio, &clb);

	ctx->time_dbg.shot_time_stamp = ktime_us_delta(ktime_get(), ctx->time_dbg.shot_time);

	if (unlikely(test_bit(GDC_DBG_DUMP_REG, &debug_gdc) ||
		     test_and_clear_bit(GDC_DBG_DUMP_REG_ONCE, &debug_gdc)))
		gdc->hw_gdc_ops->sfr_dump(gdc->regs_base);

	ctx->time_dbg.hw_time = ktime_get();

	return 0;
}

int gdc_add_context_and_run(struct gdc_dev *gdc, struct gdc_ctx *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&gdc->ctxlist_lock, flags);
	list_add_tail(&ctx->node, &gdc->context_list);
	spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);

	return gdc_run_next_job(gdc);
}

static irqreturn_t gdc_irq_handler(int irq, void *priv)
{
	struct gdc_dev *gdc = priv;
	struct gdc_ctx *ctx;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;
	u32 irq_status;

	spin_lock(&gdc->slock);

	irq_status = camerapp_hw_gdc_get_intr_status_and_clear(gdc->pmio);
	if (gdc->stalled) {
		gdc_dev_dbg(gdc->dev, "stalled intr = %x\n", irq_status);
		spin_unlock(&gdc->slock);
		return IRQ_HANDLED;
	}

	/*
	 * ok to access gdc->current_ctx withot ctxlist_lock held
	 * because it is not modified until gdc_run_next_job() is called.
	 */
	ctx = gdc->current_ctx;
	BUG_ON(!ctx);

	if (irq_status & camerapp_hw_gdc_get_int_frame_start()) {
		ctx->time_dbg.hw_time = ktime_get();
		gdc_dev_dbg(gdc->dev, "gdc frame start (0x%x)\n", irq_status);
	}

	if (irq_status & camerapp_hw_gdc_get_int_frame_end()) {
		ctx->time_dbg.hw_time_stamp = ktime_us_delta(ktime_get(), ctx->time_dbg.hw_time);

		clear_bit(DEV_RUN, &gdc->state);
		del_timer(&gdc->wdt.timer);
		atomic_set(&gdc->wdt.cnt, 0);

		clear_bit(CTX_RUN, &ctx->flags);

		BUG_ON(ctx != v4l2_m2m_get_curr_priv(gdc->m2m.m2m_dev));

		src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		BUG_ON(!src_vb || !dst_vb);

		gdc_dev_dbg(gdc->dev, "gdc frame end (0x%x)\n", irq_status);

		v4l2_m2m_buf_done(src_vb, VB2_BUF_STATE_DONE);
		v4l2_m2m_buf_done(dst_vb, VB2_BUF_STATE_DONE);

		/* Wake up from CTX_ABORT state */
		clear_bit(CTX_ABORT, &ctx->flags);

		spin_lock(&gdc->ctxlist_lock);
		gdc->current_ctx = NULL;
		gdc->votf_ctx = NULL;
		spin_unlock(&gdc->ctxlist_lock);

		v4l2_m2m_job_finish(gdc->m2m.m2m_dev, ctx->m2m_ctx);

		gdc_resume(gdc->dev);
		wake_up(&gdc->wait);

		if (unlikely(test_bit(GDC_DBG_DUMP_S2D, &debug_gdc)))
			is_debug_s2d(true, "GDC_DBG_DUMP_S2D");
	}

	spin_unlock(&gdc->slock);

	return IRQ_HANDLED;
}

static inline void gdc_init_frame(struct gdc_frame *frame)
{
	frame->addr.cb = 0;
	frame->addr.cr = 0;
	frame->addr.y_2bit = 0;
	frame->addr.cbcr_2bit = 0;
	frame->addr.cbsize = 0;
	frame->addr.crsize = 0;
	frame->addr.ysize_2bit = 0;
	frame->addr.cbcrsize_2bit = 0;
}

static inline void gdc_get_bufaddr_8_plus_2_fmt(struct gdc_frame *frame, unsigned int w,
						unsigned int h)
{
	if ((frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV16M_S10B) ||
	    (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV61M_S10B)) {
		frame->addr.ysize_2bit = NV16M_Y_SIZE(w, h);
		frame->addr.y_2bit = frame->addr.y + NV16M_Y_SIZE(w, h);
		frame->addr.cbcrsize_2bit = NV16M_CBCR_SIZE(w, h);
		frame->addr.cbcr_2bit = frame->addr.cb + NV16M_CBCR_SIZE(w, h);
	} else if ((frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV12M_S10B) ||
		   (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV21M_S10B)) {
		frame->addr.ysize_2bit = NV12M_Y_SIZE(w, h);
		frame->addr.y_2bit = frame->addr.y + NV12M_Y_SIZE(w, h);
		frame->addr.cbcrsize_2bit = NV12M_CBCR_SIZE(w, h);
		frame->addr.cbcr_2bit = frame->addr.cb + NV12M_CBCR_SIZE(w, h);
	}
}

static int gdc_get_bufaddr_frame_fmt(struct gdc_frame *frame, unsigned int pixsize,
				     unsigned int bytesize, struct vb2_buffer *vb2buf)
{
	if (frame->gdc_fmt->num_planes == 1) {
		if (gdc_fmt_is_ayv12(frame->gdc_fmt->pixelformat)) {
			unsigned int c_span;

			c_span = ALIGN(frame->width >> 1, 16);
			frame->addr.ysize = pixsize;
			frame->addr.cbsize = c_span * (frame->height >> 1);
			frame->addr.crsize = frame->addr.cbsize;
			frame->addr.cb = frame->addr.y + pixsize;
			frame->addr.cr = frame->addr.cb + frame->addr.cbsize;
		} else if (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_YUV420N) {
			unsigned int w = frame->width;
			unsigned int h = frame->height;

			frame->addr.ysize = YUV420N_Y_SIZE(w, h);
			frame->addr.cbsize = YUV420N_CB_SIZE(w, h);
			frame->addr.crsize = YUV420N_CR_SIZE(w, h);
			frame->addr.cb = YUV420N_CB_BASE(frame->addr.y, w, h);
			frame->addr.cr = YUV420N_CR_BASE(frame->addr.y, w, h);
		} else {
			frame->addr.ysize = pixsize;
			frame->addr.cbsize = (bytesize - pixsize) / 2;
			frame->addr.crsize = frame->addr.cbsize;
			frame->addr.cb = frame->addr.y + pixsize;
			frame->addr.cr = frame->addr.cb + frame->addr.cbsize;
		}
	} else if (frame->gdc_fmt->num_planes == 3) {
		frame->addr.cb = gdc_get_dma_address(vb2buf, 1);
		if (!frame->addr.cb)
			return -EINVAL;

		frame->addr.cr = gdc_get_dma_address(vb2buf, 2);
		if (!frame->addr.cr)
			return -EINVAL;
		frame->addr.ysize = pixsize * frame->gdc_fmt->bitperpixel[0] >> 3;
		frame->addr.cbsize = pixsize * frame->gdc_fmt->bitperpixel[1] >> 3;
		frame->addr.crsize = pixsize * frame->gdc_fmt->bitperpixel[2] >> 3;
	}

	return 0;
}

static int gdc_get_bufaddr(struct gdc_dev *gdc, struct gdc_ctx *ctx, struct vb2_buffer *vb2buf,
			   struct gdc_frame *frame)
{
	unsigned int pixsize, ysize;
	unsigned int w = frame->width;
	unsigned int h = frame->height;
	int ret = 0;

	frame->addr.y = gdc_get_dma_address(vb2buf, 0);
	if (!frame->addr.y)
		return -EINVAL;

	gdc_init_frame(frame);

	switch (frame->gdc_fmt->num_comp) {
	case 1: /* rgb, yuyv */
		pixsize = frame->width * frame->height;
		ysize = (pixsize * frame->gdc_fmt->bitperpixel[0]) >> 3;
		frame->addr.ysize = ysize;
		break;
	case 2:
		if (frame->gdc_fmt->num_planes == 1) {
			ysize = frame->stride * frame->height;
			gdc_dev_dbg(gdc->dev, "pixsize(%dx%d) stride(%d) ysize(%u)\n",
				frame->width, frame->height, frame->stride, ysize);

			/* single-fd yuv 420 formats */
			if (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV12
				|| frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV21
#ifdef EXYNOS_GDC_SUPPORT_V4L2_PIX_FMT_P010
				|| frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_P010
#endif
				) {
				frame->addr.ysize = ysize;
				frame->addr.cb = frame->addr.y + ysize;
				frame->addr.cbsize = (ysize >> 1);
			/* single-fd yuv 422 formats */
			} else if (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV16 ||
					frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV61) {
				frame->addr.ysize = ysize;
				frame->addr.cb = frame->addr.y + ysize;
				frame->addr.cbsize = ysize;
			} else {
				gdc_dev_err(gdc->dev, "Please check frame->gdc_fmt->pixelformat\n");
			}
		} else if (frame->gdc_fmt->num_planes == 2) {
			/* V4L2_PIX_FMT_NV21M, V4L2_PIX_FMT_NV12M */
			/* V4L2_PIX_FMT_NV61M, V4L2_PIX_FMT_NV16M */
			/* V4L2_PIX_FMT_NV12M_P010, V4L2_PIX_FMT_NV16M_P210 */
			/* V4L2_PIX_FMT_NV21M_P010, V4L2_PIX_FMT_NV61M_P210 */
			frame->addr.cb = gdc_get_dma_address(vb2buf, 1);
			if (!frame->addr.cb)
				return -EINVAL;

			/* 8+2 format */
			gdc_get_bufaddr_8_plus_2_fmt(frame, w, h);

			/* SBWC format */
			if (frame->extra && camerapp_hw_gdc_has_comp_header(frame->extra)) {
				/*
				 * When SBWC is on,
				 * Buffer is consist of payload(before) + header(after)
				 * Header base address = payload base address + payload memory size
				 */
				frame->addr.y_2bit =
					frame->addr.y +
					camerapp_hw_get_comp_buf_size(gdc, frame, w, h,
								      frame->gdc_fmt->pixelformat,
								      GDC_PLANE_LUMA,
								      GDC_SBWC_SIZE_PAYLOAD);
				frame->addr.cbcr_2bit =
					frame->addr.cb +
					camerapp_hw_get_comp_buf_size(gdc, frame, w, h,
								      frame->gdc_fmt->pixelformat,
								      GDC_PLANE_CHROMA,
								      GDC_SBWC_SIZE_PAYLOAD);
			}
		} else {
			gdc_dev_err(gdc->dev, "Please check frame->gdc_fmt->pixelformat\n");
		}
		break;
	case 3:
		pixsize = frame->width * frame->height;
		ysize = (pixsize * frame->gdc_fmt->bitperpixel[0]) >> 3;
		ret = gdc_get_bufaddr_frame_fmt(frame, pixsize, ysize, vb2buf);
		if (ret)
			return ret;
		break;
	default:
		break;
	}

	if (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_YVU420 ||
	    frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_YVU420M) {
		u32 t_cb = frame->addr.cb;

		frame->addr.cb = frame->addr.cr;
		frame->addr.cr = t_cb;
	}

#ifdef ENABLE_PRINT_GDC_BUFADDR
	gdc_dev_info(gdc->dev, "y addr %pa y size %#x\n", &frame->addr.y, frame->addr.ysize);
	gdc_dev_info(gdc->dev, "cb addr %pa cb size %#x\n", &frame->addr.cb, frame->addr.cbsize);
	gdc_dev_info(gdc->dev, "cr addr %pa cr size %#x\n", &frame->addr.cr, frame->addr.crsize);
#endif

	return 0;
}

void gdc_fill_curr_frame(struct gdc_dev *gdc, struct gdc_ctx *ctx)
{
	struct gdc_frame *s_frame, *d_frame;
	struct vb2_buffer *src_vb = (struct vb2_buffer *)v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	struct vb2_buffer *dst_vb = (struct vb2_buffer *)v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	u32 param_index = GDC_BUF_IDX_TO_PARAM_IDX(
		ctx->crop_param[GDC_BUF_IDX_TO_PARAM_IDX(src_vb->index)].buf_index);
	struct gdc_crop_param *crop_param = &ctx->crop_param[param_index];

	gdc_dev_dbg(gdc->dev, "buf_index : %u, param_index :%u\n", src_vb->index, param_index);

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	/* Check input crop boundary */
	ctx->cur_index = param_index;
	if (crop_param->crop_start_x + crop_param->crop_width > s_frame->width ||
	    crop_param->crop_start_y + crop_param->crop_height > s_frame->height) {
		gdc_dev_err(gdc->dev, "Invalid input crop size %d,%d %dx%d. Input size %dx%d\n",
			crop_param->crop_start_x, crop_param->crop_start_y, crop_param->crop_width,
			crop_param->crop_height, s_frame->width, s_frame->height);
		crop_param->crop_start_x = 0;
		crop_param->crop_start_y = 0;
		crop_param->crop_width = s_frame->width;
		crop_param->crop_height = s_frame->height;
	}

	d_frame->io_mode = crop_param->out_mode;

	s_frame->stride = (unsigned short)max(crop_param->src_bytesperline[0],
			(u32)(s_frame->width * s_frame->gdc_fmt->bitperpixel[0]) >> 3);
	d_frame->stride = (unsigned short)max(crop_param->dst_bytesperline[0],
			(u32)(d_frame->width * d_frame->gdc_fmt->bitperpixel[0]) >> 3);

	gdc_get_bufaddr(gdc, ctx, src_vb, s_frame);
	gdc_get_bufaddr(gdc, ctx, dst_vb, d_frame);

	gdc_dev_dbg(gdc->dev, "gdc_src : pixelformat = %c%c%c%c, w(%d), h(%d), s(%d)\n",
		(char)((s_frame->gdc_fmt->pixelformat >> 0) & 0xFF),
		(char)((s_frame->gdc_fmt->pixelformat >> 8) & 0xFF),
		(char)((s_frame->gdc_fmt->pixelformat >> 16) & 0xFF),
		(char)((s_frame->gdc_fmt->pixelformat >> 24) & 0xFF),
		s_frame->width, s_frame->height, s_frame->stride);
	gdc_dev_dbg(gdc->dev, "gdc_dst : pixelformat = %c%c%c%c, w(%d), h(%d), s(%d)\n",
		(char)((d_frame->gdc_fmt->pixelformat >> 0) & 0xFF),
		(char)((d_frame->gdc_fmt->pixelformat >> 8) & 0xFF),
		(char)((d_frame->gdc_fmt->pixelformat >> 16) & 0xFF),
		(char)((d_frame->gdc_fmt->pixelformat >> 24) & 0xFF),
		d_frame->width, d_frame->height, d_frame->stride);

	gdc_dev_dbg(gdc->dev, "gdc_crop_param[%d] : crop_x:%d, crop_y:%d crop_width:%d crop_height:%d out_mode:%d(%s) grid:%d bypass:%d\n",
		param_index, crop_param->crop_start_x, crop_param->crop_start_y,
		crop_param->crop_width, crop_param->crop_height, crop_param->out_mode,
		gdc_get_out_mode_name((enum gdc_out_mode)crop_param->out_mode),
		crop_param->is_grid_mode, crop_param->is_bypass_mode);

#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)
	gdc_hdr10p_fill_curr_frame(gdc, ctx, param_index);
#endif
}

static void gdc_m2m_device_run(void *priv)
{
	struct gdc_ctx *ctx = priv;
	struct gdc_dev *gdc = ctx->gdc_dev;
	int out_mode;

	gdc_dev_dbg(gdc->dev, "gdc m2m device run\n");

	if (test_bit(DEV_SUSPEND, &gdc->state)) {
		gdc_dev_err(gdc->dev, "GDC is in suspend state\n");
		return;
	}

	if (test_bit(CTX_ABORT, &ctx->flags)) {
		gdc_dev_err(gdc->dev, "aborted GDC device run\n");
		return;
	}

	out_mode = gdc_get_out_mode(ctx);
	if (out_mode == GDC_OUT_VOTF || out_mode == GDC_OUT_OTF) {
		gdc_out_votf_otf(gdc, ctx);
		/* GDC is not started immediately in GDC-MFC vOTF/OTF mode */
		return;
	}

	gdc_fill_curr_frame(gdc, ctx);
	gdc_add_context_and_run(gdc, ctx);
}

static void gdc_m2m_job_abort(void *priv)
{
	struct gdc_ctx *ctx = priv;
	int ret;

	ret = gdc_ctx_stop_req(ctx);
	if (ret < 0)
		gdc_dev_err(ctx->gdc_dev->dev, "wait timeout\n");
}

static struct v4l2_m2m_ops gdc_m2m_ops = {
	.device_run = gdc_m2m_device_run,
	.job_abort = gdc_m2m_job_abort,
};

static void gdc_unregister_m2m_device(struct gdc_dev *gdc)
{
	video_unregister_device(gdc->m2m.vfd);
	v4l2_m2m_release(gdc->m2m.m2m_dev);
	v4l2_device_unregister(&gdc->m2m.v4l2_dev);
}

static struct v4l2_m2m_dev *gdc_v4l2_m2m_init(void)
{
	return v4l2_m2m_init(&gdc_m2m_ops);
}

static void gdc_v4l2_m2m_release(struct v4l2_m2m_dev *m2m_dev)
{
	v4l2_m2m_release(m2m_dev);
}

static int gdc_register_m2m_device(struct gdc_dev *gdc, int dev_id)
{
	struct v4l2_device *v4l2_dev;
	struct device *dev;
	struct video_device *vfd;
	int ret = 0;
	u32 node_num = CAMERAPP_VIDEONODE_GDC + dev_id;

	dev = gdc->dev;
	v4l2_dev = &gdc->m2m.v4l2_dev;

	scnprintf(v4l2_dev->name, sizeof(v4l2_dev->name), "%s.m2m", GDC_MODULE_NAME);

	ret = v4l2_device_register(dev, v4l2_dev);
	if (ret) {
		gdc_dev_err(gdc->dev, "failed to register v4l2 device\n");
		return ret;
	}

	vfd = video_device_alloc();
	if (!vfd) {
		gdc_dev_err(gdc->dev, "failed to allocate video device\n");
		goto err_v4l2_dev;
	}

	vfd->fops = &gdc_v4l2_fops;
	vfd->ioctl_ops = &gdc_v4l2_ioctl_ops;
	vfd->release = video_device_release;
	vfd->lock = &gdc->lock;
	vfd->vfl_dir = VFL_DIR_M2M;
	vfd->v4l2_dev = v4l2_dev;
	vfd->device_caps = GDC_V4L2_DEVICE_CAPS;
	scnprintf(vfd->name, sizeof(vfd->name), "%s:m2m", GDC_MODULE_NAME);

	video_set_drvdata(vfd, gdc);

	gdc->m2m.vfd = vfd;
	gdc->m2m.m2m_dev = gdc_v4l2_m2m_init();
	if (IS_ERR(gdc->m2m.m2m_dev)) {
		gdc_dev_err(gdc->dev, "failed to initialize v4l2-m2m device\n");
		ret = PTR_ERR(gdc->m2m.m2m_dev);
		goto err_dev_alloc;
	}

	ret = video_register_device(vfd, VFL_TYPE_PABLO, EXYNOS_VIDEONODE_CAMERAPP(node_num));
	if (ret) {
		gdc_dev_err(gdc->dev, "failed to register video device\n");
		goto err_m2m_dev;
	}

	gdc_dev_info(gdc->dev, "video node register: %d\n", EXYNOS_VIDEONODE_CAMERAPP(node_num));

	return 0;

err_m2m_dev:
	gdc_v4l2_m2m_release(gdc->m2m.m2m_dev);
err_dev_alloc:
	video_device_release(gdc->m2m.vfd);
err_v4l2_dev:
	v4l2_device_unregister(v4l2_dev);

	return ret;
}

#ifdef CONFIG_EXYNOS_IOVMM
static int __maybe_unused gdc_sysmmu_fault_handler(struct iommu_domain *domain,
		struct device *dev, unsigned long iova, int flags, void *token)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);
#else
static int gdc_sysmmu_fault_handler(struct iommu_fault *fault, void *data)
{
	struct gdc_dev *gdc = data;
	struct device *dev = gdc->dev;
	unsigned long iova = fault->event.addr;
#endif
	if (test_bit(DEV_RUN, &gdc->state)) {
		gdc_dev_info(dev, "System MMU fault called for IOVA %#lx\n", iova);
		camerapp_gdc_sfr_dump(gdc->regs_base);
	}

	return 0;
}

static int gdc_clk_get(struct gdc_dev *gdc)
{
	gdc->aclk = devm_clk_get(gdc->dev, "gate");
	if (IS_ERR(gdc->aclk)) {
		if (PTR_ERR(gdc->aclk) != -ENOENT) {
			gdc_dev_err(gdc->dev,
				"Failed to get 'gate' clock: %ld", PTR_ERR(gdc->aclk));
			return PTR_ERR(gdc->aclk);
		}
		gdc_dev_info(gdc->dev, "'gate' clock is not present\n");
	}

	gdc->pclk = devm_clk_get(gdc->dev, "gate2");
	if (IS_ERR(gdc->pclk)) {
		if (PTR_ERR(gdc->pclk) != -ENOENT) {
			gdc_dev_err(gdc->dev,
				"Failed to get 'gate2' clock: %ld", PTR_ERR(gdc->pclk));
			return PTR_ERR(gdc->pclk);
		}
		gdc_dev_info(gdc->dev, "'gate2' clock is not present\n");
	}

	gdc->clk_chld = devm_clk_get(gdc->dev, "mux_user");
	if (IS_ERR(gdc->clk_chld)) {
		if (PTR_ERR(gdc->clk_chld) != -ENOENT) {
			gdc_dev_err(gdc->dev, "Failed to get 'mux_user' clock: %ld",
				PTR_ERR(gdc->clk_chld));
			return PTR_ERR(gdc->clk_chld);
		}
		gdc_dev_info(gdc->dev, "'mux_user' clock is not present\n");
	}

	if (!IS_ERR(gdc->clk_chld)) {
		gdc->clk_parn = devm_clk_get(gdc->dev, "mux_src");
		if (IS_ERR(gdc->clk_parn)) {
			gdc_dev_err(gdc->dev, "Failed to get 'mux_src' clock: %ld",
				PTR_ERR(gdc->clk_parn));
			return PTR_ERR(gdc->clk_parn);
		}
	} else {
		gdc->clk_parn = ERR_PTR(-ENOENT);
	}

	return 0;
}

static void gdc_clk_put(struct gdc_dev *gdc)
{
	if (!IS_ERR(gdc->clk_parn))
		devm_clk_put(gdc->dev, gdc->clk_parn);

	if (!IS_ERR(gdc->clk_chld))
		devm_clk_put(gdc->dev, gdc->clk_chld);

	if (!IS_ERR(gdc->pclk))
		devm_clk_put(gdc->dev, gdc->pclk);

	if (!IS_ERR(gdc->aclk))
		devm_clk_put(gdc->dev, gdc->aclk);
}

#ifdef CONFIG_PM_SLEEP
static int gdc_suspend(struct device *dev)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);
	int ret;

	set_bit(DEV_SUSPEND, &gdc->state);

	ret = wait_event_timeout(gdc->wait, !test_bit(DEV_RUN, &gdc->state),
				 GDC_TIMEOUT * 50); /*  2sec */
	if (ret == 0)
		gdc_dev_err(gdc->dev, "wait timeout\n");

	return 0;
}

static int gdc_resume(struct device *dev)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);

	clear_bit(DEV_SUSPEND, &gdc->state);

	return 0;
}
#endif

#ifdef CONFIG_PM
static int gdc_runtime_resume(struct device *dev)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);

	if (!IS_ERR(gdc->clk_chld) && !IS_ERR(gdc->clk_parn)) {
		int ret = clk_set_parent(gdc->clk_chld, gdc->clk_parn);

		if (ret) {
			gdc_dev_err(gdc->dev, "Failed to setup MUX: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int gdc_runtime_suspend(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops gdc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(gdc_suspend, gdc_resume)
	SET_RUNTIME_PM_OPS(gdc_runtime_suspend, gdc_runtime_resume, NULL)
};

static int gdc_pmio_init(struct gdc_dev *gdc)
{
	int ret;

	camerapp_hw_gdc_init_pmio_config(gdc);

	gdc->pmio = pmio_init(gdc, NULL, &gdc->pmio_config);
	if (IS_ERR(gdc->pmio)) {
		gdc_dev_err(gdc->dev, "failed to init gdc PMIO: %ld", PTR_ERR(gdc->pmio));
		return -ENOMEM;
	}

	if (!gdc->pmio_en)
		return 0;

	ret = pmio_field_bulk_alloc(gdc->pmio, &gdc->pmio_fields, gdc->pmio_config.fields,
				    gdc->pmio_config.num_fields);
	if (ret) {
		gdc_dev_err(gdc->dev, "failed to alloc gdc PMIO fields: %d", ret);
		pmio_exit(gdc->pmio);
		return ret;
	}

	return 0;
}

static void gdc_pmio_exit(struct gdc_dev *gdc)
{
	if (gdc->pmio) {
		if (gdc->pmio_fields)
			pmio_field_bulk_free(gdc->pmio, gdc->pmio_fields);

		pmio_exit(gdc->pmio);
	}
}

static int gdc_probe(struct platform_device *pdev)
{
	struct gdc_dev *gdc;
	struct resource *rsc;
	struct device_node *np;
	struct device_node *votf_np = NULL;
	int ret = 0;
	const char *interrupt_names[2] = { NULL, NULL };

	gdc = devm_kzalloc(&pdev->dev, sizeof(struct gdc_dev), GFP_KERNEL);
	if (!gdc)
		return -ENOMEM;

	gdc->dev = &pdev->dev;
	np = gdc->dev->of_node;

	dev_set_drvdata(&pdev->dev, gdc);

	spin_lock_init(&gdc->ctxlist_lock);
	INIT_LIST_HEAD(&gdc->context_list);
	spin_lock_init(&gdc->slock);
	mutex_init(&gdc->lock);
	init_waitqueue_head(&gdc->wait);

	rsc = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!rsc) {
		gdc_dev_err(&pdev->dev, "Failed to get io memory region\n");
		ret = -ENOMEM;
		goto err_get_mem_res;
	}

	gdc->regs_base = devm_ioremap(&pdev->dev, rsc->start, resource_size(rsc));
	if (IS_ERR_OR_NULL(gdc->regs_base)) {
		gdc_dev_err(&pdev->dev, "Failed to ioremap for reg_base\n");
		ret = ENOMEM;
		goto err_get_mem_res;
	}
	gdc->regs_rsc = rsc;

	of_property_read_string_array(np, "interrupt-names", interrupt_names, 2);

	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		gdc_dev_err(&pdev->dev, "failed to get IRQ-0 (%d)\n", ret);
		goto err_get_irq_res;
	}
	gdc->irq0 = ret;

	if (interrupt_names[0]) {
		ret = devm_request_irq(&pdev->dev, gdc->irq0, gdc_irq_handler, 0,
				       interrupt_names[0], gdc);
	} else {
		ret = devm_request_irq(&pdev->dev, gdc->irq0, gdc_irq_handler, 0, "GDC0", gdc);
	}

	if (ret) {
		gdc_dev_err(&pdev->dev, "failed to install irq\n");
		goto err_get_irq_res;
	}

	// apply multi target irq
	pablo_set_affinity_irq(gdc->irq0, true);

	ret = platform_get_irq(pdev, 1);
	if (ret < 0) {
		gdc_dev_info(&pdev->dev, "there's no IRQ-1 resource\n");
	} else {
		gdc->irq1 = ret;

		if (interrupt_names[1]) {
			ret = devm_request_irq(&pdev->dev, gdc->irq1, gdc_irq_handler, 0,
					       interrupt_names[1], gdc);
		} else {
			ret = devm_request_irq(&pdev->dev, gdc->irq1, gdc_irq_handler, 0, "GDC1",
					       gdc);
		}

		if (ret) {
			gdc_dev_err(&pdev->dev, "failed to install irq\n");
			goto err_req_irq1;
		}

		// apply multi target irq
		pablo_set_affinity_irq(gdc->irq1, true);
	}

	votf_np = of_get_child_by_name(np, "votf_axi");
	if (votf_np) {
		ret = of_property_read_u32_array(votf_np, "votf_src_dest", gdc->votf_src_dest, 2);
		if (ret) {
			gdc->has_votf_mfc = 0;
			gdc_dev_err(&pdev->dev, "failed to get votf src dest ID (ret:%d)\n", ret);
			goto err_votf_axi;
		}

		gdc_set_device_for_votf(&pdev->dev);
		gdc->has_votf_mfc = 1;
		gdc->votf_base = ioremap(gdc->votf_src_dest[GDC_SRC_ADDR], 0x10000);
		gdc->mfc_votf_base = ioremap(gdc->votf_src_dest[GDC_DST_ADDR], 0x10000);
		pkv_iommu_dma_reserve_iova_map(gdc->dev, gdc->votf_src_dest[GDC_DST_ADDR], 0x10000);
		gdc_clear_mfc_votf_ops(gdc);
	} else {
		gdc->votf_src_dest[GDC_SRC_ADDR] = 0;
		gdc->votf_src_dest[GDC_DST_ADDR] = 0;
		gdc->has_votf_mfc = 0;
	}

	gdc->has_hdr10p = of_property_read_bool(np, "hdr10p");

	gdc->use_cloader_iommu_group = of_property_read_bool(np, "iommu_group_for_cloader");
	if (gdc->use_cloader_iommu_group) {
		ret = of_property_read_u32(np, "iommu_group_for_cloader",
					   &gdc->cloader_iommu_group_id);
		if (ret)
			gdc_dev_err(&pdev->dev,
				"fail to get iommu group id for cloader, ret(%d)", ret);
	}

	atomic_set(&gdc->wdt.cnt, 0);
	timer_setup(&gdc->wdt.timer, gdc_watchdog, 0);

	ret = gdc_clk_get(gdc);
	if (ret)
		goto err_wq;

	if (pdev->dev.of_node)
		gdc->dev_id = of_alias_get_id(pdev->dev.of_node, "pablo-gdc");
	else
		gdc->dev_id = pdev->id;

	if (gdc->dev_id < 0)
		gdc->dev_id = 0;

	platform_set_drvdata(pdev, gdc);

	pm_runtime_enable(&pdev->dev);

	ret = gdc_register_m2m_device(gdc, gdc->dev_id);
	if (ret) {
		gdc_dev_err(&pdev->dev, "failed to register m2m device\n");
		goto err_reg_m2m_dev;
	}

#ifdef CONFIG_EXYNOS_IOVMM
	ret = iovmm_activate(gdc->dev);
	if (ret) {
		gdc_dev_err(&pdev->dev, "failed to attach iommu\n");
		goto err_iommu;
	}
#endif

	ret = gdc_power_clk_enable(gdc);
	if (ret)
		goto err_power_clk;

#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_set_fault_handler(&pdev->dev, gdc_sysmmu_fault_handler, gdc);
#else
	iommu_register_device_fault_handler(&pdev->dev, gdc_sysmmu_fault_handler, gdc);
#endif

	ret = is_mem_init(&gdc->mem, pdev);
	if (ret) {
		gdc_dev_err(gdc->dev, "gdc_mem_probe is fail(%d)", ret);
		goto err_mem_init;
	}

	gdc->pmio_en = false;
	ret = gdc_pmio_init(gdc);
	if (ret) {
		gdc_dev_err(&pdev->dev, "gdc pmio initialization failed(%d)", ret);
		goto err_pmio_init;
	}

	mutex_init(&gdc->m2m.lock);

	gdc->variant = camerapp_hw_gdc_get_size_constraints(gdc->pmio);
	gdc->version = gdc->variant->version;
	gdc->stalled = 0;
	gdc->v4l2_ops = &gdc_v4l2_ops;
	gdc->sys_ops = &gdc_sys_ops;
	gdc->hw_gdc_ops = pablo_get_hw_gdc_ops();

	gdc_set_driver_version(gdc->version);

	gdc_power_clk_disable(gdc);

	gdc_dev_info(&pdev->dev, "Driver probed successfully(version: %08x)\n", gdc->version);

	return 0;

err_pmio_init:
err_mem_init:
#ifndef CONFIG_EXYNOS_IOVMM
	iommu_unregister_device_fault_handler(&pdev->dev);
#endif
	gdc_power_clk_disable(gdc);
err_power_clk:
#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_deactivate(gdc->dev);
err_iommu:
#endif
	gdc_unregister_m2m_device(gdc);
err_reg_m2m_dev:
	gdc_clk_put(gdc);
err_wq:
	if (gdc->has_votf_mfc) {
		iounmap(gdc->votf_base);
		iounmap(gdc->mfc_votf_base);
		pkv_iommu_dma_reserve_iova_unmap(&pdev->dev, gdc->votf_src_dest[GDC_DST_ADDR],
						 0x10000);
	}
err_votf_axi:
	if (gdc->irq1) {
		pablo_set_affinity_irq(gdc->irq1, false);
		devm_free_irq(&pdev->dev, gdc->irq1, gdc);
	}
err_req_irq1:
	pablo_set_affinity_irq(gdc->irq0, false);
	devm_free_irq(&pdev->dev, gdc->irq0, gdc);
err_get_irq_res:
	devm_iounmap(&pdev->dev, gdc->regs_base);
err_get_mem_res:
	devm_kfree(&pdev->dev, gdc);

	return ret;
}

static int gdc_remove(struct platform_device *pdev)
{
	struct gdc_dev *gdc = platform_get_drvdata(pdev);

	gdc_pmio_exit(gdc);

	pablo_mem_deinit(&gdc->mem);

#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_set_fault_handler(&pdev->dev, NULL, gdc);
	iovmm_deactivate(gdc->dev);
#else
	iommu_unregister_device_fault_handler(&pdev->dev);
#endif

	gdc_unregister_m2m_device(gdc);

	pm_runtime_disable(&pdev->dev);

	gdc_clk_put(gdc);

	if (timer_pending(&gdc->wdt.timer))
		del_timer(&gdc->wdt.timer);

	if (gdc->has_votf_mfc) {
		gdc_clear_mfc_votf_ops(gdc);
		iounmap(gdc->votf_base);
		iounmap(gdc->mfc_votf_base);
		pkv_iommu_dma_reserve_iova_unmap(&pdev->dev, gdc->votf_src_dest[GDC_DST_ADDR],
						 0x10000);
	}

	if (gdc->irq1) {
		pablo_set_affinity_irq(gdc->irq1, false);
		devm_free_irq(&pdev->dev, gdc->irq1, gdc);
	}
	pablo_set_affinity_irq(gdc->irq0, false);
	devm_free_irq(&pdev->dev, gdc->irq0, gdc);
	devm_iounmap(&pdev->dev, gdc->regs_base);
	devm_kfree(&pdev->dev, gdc);

	return 0;
}

static void gdc_shutdown(struct platform_device *pdev)
{
	struct gdc_dev *gdc = platform_get_drvdata(pdev);

	set_bit(DEV_SUSPEND, &gdc->state);

	wait_event(gdc->wait, !test_bit(DEV_RUN, &gdc->state));

#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_deactivate(gdc->dev);
#endif
}

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
static void gdc_set_log_level(int level)
{
	gdc_log_level = level;
}

static struct pkt_gdc_ops gdc_ops = {
	.get_log_level = gdc_get_log_level,
	.set_log_level = gdc_set_log_level,
	.get_debug = param_get_debug_gdc,
	.votf_set_service_cfg = gdc_votf_set_service_config,
	.votfitf_set_flush = gdc_votfitf_set_flush,
	.find_format = gdc_find_format,
	.v4l2_querycap = gdc_v4l2_querycap,
	.v4l2_g_fmt_mplane = gdc_v4l2_g_fmt_mplane,
	.v4l2_try_fmt_mplane = gdc_v4l2_try_fmt_mplane,
	.image_bound_check = gdc_image_bound_check,
	.v4l2_s_fmt_mplane = gdc_v4l2_s_fmt_mplane,
	.v4l2_reqbufs = gdc_v4l2_reqbufs,
	.v4l2_querybuf = gdc_v4l2_querybuf,
	.vb2_qbuf = gdc_check_vb2_qbuf,
	.check_qbuf = gdc_check_qbuf,
	.v4l2_qbuf = gdc_v4l2_qbuf,
	.v4l2_dqbuf = gdc_v4l2_dqbuf,
	.power_clk_enable = gdc_power_clk_enable,
	.power_clk_disable = gdc_power_clk_disable,
	.v4l2_streamon = gdc_v4l2_streamon,
	.v4l2_streamoff = gdc_v4l2_streamoff,
	.v4l2_s_ctrl = gdc_v4l2_s_ctrl,
	.v4l2_s_ext_ctrls = gdc_v4l2_s_ext_ctrls,
	.v4l2_m2m_init = gdc_v4l2_m2m_init,
	.v4l2_m2m_release = gdc_v4l2_m2m_release,
	.ctx_stop_req = gdc_ctx_stop_req,
	.vb2_queue_setup = gdc_vb2_queue_setup,
	.vb2_buf_prepare = gdc_vb2_buf_prepare,
	.vb2_buf_finish = gdc_vb2_buf_finish,
	.vb2_buf_queue = gdc_vb2_buf_queue,
	.vb2_lock = gdc_vb2_lock,
	.vb2_unlock = gdc_vb2_unlock,
	.cleanup_queue = gdc_cleanup_queue,
	.vb2_start_streaming = gdc_vb2_start_streaming,
	.vb2_stop_streaming = gdc_vb2_stop_streaming,
	.queue_init = queue_init,
	.pmio_init = gdc_pmio_init,
	.pmio_exit = gdc_pmio_exit,
	.pmio_config = gdc_pmio_config,
	.run_next_job = gdc_run_next_job,
	.add_context_and_run = gdc_add_context_and_run,
	.m2m_device_run = gdc_m2m_device_run,
	.poll = gdc_poll,
	.mmap = gdc_mmap,
	.m2m_job_abort = gdc_m2m_job_abort,
	.clk_get = gdc_clk_get,
	.clk_put = gdc_clk_put,
	.sysmmu_fault_handler = gdc_sysmmu_fault_handler,
	.shutdown = gdc_shutdown,
	.suspend = gdc_suspend,
	.runtime_resume = gdc_runtime_resume,
	.runtime_suspend = gdc_runtime_suspend,
	.alloc_pmio_mem = gdc_alloc_pmio_mem,
	.free_pmio_mem = gdc_free_pmio_mem,
	.job_finish = gdc_job_finish,
	.register_m2m_device = gdc_register_m2m_device,
	.init_frame = gdc_init_frame,
	.get_bufaddr_8_plus_2_fmt = gdc_get_bufaddr_8_plus_2_fmt,
	.get_bufaddr_frame_fmt = gdc_get_bufaddr_frame_fmt,
	.get_bufaddr = gdc_get_bufaddr,
};

struct pkt_gdc_ops *pablo_kunit_get_gdc(void)
{
	return &gdc_ops;
}
KUNIT_EXPORT_SYMBOL(pablo_kunit_get_gdc);

ulong pablo_get_dbg_gdc(void)
{
	return debug_gdc;
}
KUNIT_EXPORT_SYMBOL(pablo_get_dbg_gdc);

void pablo_set_dbg_gdc(ulong dbg)
{
	debug_gdc = dbg;
}
KUNIT_EXPORT_SYMBOL(pablo_set_dbg_gdc);

#endif

static const struct of_device_id exynos_gdc_match[] = {
	{
		.compatible = "samsung,exynos-is-gdc",
	},
	{
		.compatible = "samsung,exynos-is-gdc-o",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_gdc_match);

static struct platform_driver gdc_driver = {
	.probe		= gdc_probe,
	.remove		= gdc_remove,
	.shutdown	= gdc_shutdown,
	.driver = {
		.name	= GDC_MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &gdc_pm_ops,
		.of_match_table = of_match_ptr(exynos_gdc_match),
	}
};
module_driver(gdc_driver, platform_driver_register, platform_driver_unregister)

MODULE_AUTHOR("SamsungLSI Camera");
MODULE_DESCRIPTION("EXYNOS CameraPP GDC driver");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_LICENSE("GPL");
