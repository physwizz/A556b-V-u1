/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS Scaler driver
 *
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
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/pm_runtime.h>
#include <linux/iommu.h>
#include <linux/iosys-map.h>
#include <linux/dma-buf.h>
#include <linux/dma-fence.h>
#include <linux/sync_file.h>

#include <soc/samsung/exynos/exynos-soc.h>
#include <soc/samsung/exynos/exynos-hvc.h>

#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-sg.h>

#include <soc/samsung/bts.h>

#include <kunit/visibility.h>

#include "scaler.h"
#include "scaler-regs.h"

/* Protection IDs of Scaler are 1 and 2. */
#define SC_SMC_PROTECTION_ID(instance)	(1 + (instance))

int sc_log_level;
module_param_named(sc_log_level, sc_log_level, uint, 0644);

int sc_set_blur;
module_param_named(sc_set_blur, sc_set_blur, uint, 0644);

int sc_show_stat;
module_param_named(sc_show_stat, sc_show_stat, uint, 0644);

#define BUF_EXT_SIZE	512
#define BUF_WIDTH_ALIGN	128

VISIBLE_IF_KUNIT inline unsigned int sc_ext_buf_size(int width)
{
	return IS_ALIGNED(width, BUF_WIDTH_ALIGN) ? 0 : BUF_EXT_SIZE;
}
EXPORT_SYMBOL_IF_KUNIT(sc_ext_buf_size);

/*
 * If true, writes the latency of H/W operation to v4l2_buffer.reserved2
 * in the unit of nano seconds.  It must not be enabled with real use-case
 * because v4l2_buffer.reserved may be used for other purpose.
 * The latency is written to the destination buffer.
 */
int __measure_hw_latency;
module_param_named(measure_hw_latency, __measure_hw_latency, int, 0644);

/*
 * If true, the driver will write mark into systrace.
 * By systrace, profiling the performance of MSCL-HW wil be available.
 * Actually, to turn on systrace of this,
 * '/d/tracing/events/systrace/enable' should be enabled additionally.
 */
int __enable_systrace;
module_param_named(enable_systrace, __enable_systrace, int, 0644);

struct vb2_sc_buffer {
	struct v4l2_m2m_buffer mb;
	ktime_t ktime;

	struct dma_fence	*in_fence;
	int			state;
	struct dma_fence_cb	fence_cb;
	struct timer_list	fence_timer;
	struct work_struct	qbuf_work;

	struct sync_file	*out_sync_file;

	struct sc_tws		*tws;
};

static const struct sc_fmt sc_formats[] = {
	{
		.name		= "RGB565",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
		.cfg_val	= SCALER_CFG_FMT_RGB565,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "RGB1555",
		.pixelformat	= V4L2_PIX_FMT_RGB555X,
		.cfg_val	= SCALER_CFG_FMT_ARGB1555,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "ARGB4444",
		.pixelformat	= V4L2_PIX_FMT_RGB444,
		.cfg_val	= SCALER_CFG_FMT_ARGB4444,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {	/* swaps of ARGB32 in bytes in half word, half words in word */
		.name		= "RGBA8888",
		.pixelformat	= V4L2_PIX_FMT_RGB32,
		.cfg_val	= SCALER_CFG_FMT_RGBA8888 |
					SCALER_CFG_BYTE_HWORD_SWAP,
		.bitperpixel	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "BGRA8888",
		.pixelformat	= V4L2_PIX_FMT_BGR32,
		.cfg_val	= SCALER_CFG_FMT_ARGB8888,
		.bitperpixel	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "ARGB2101010",
		.pixelformat	= V4L2_PIX_FMT_ARGB2101010,
		.cfg_val	= SCALER_CFG_FMT_ARGB2101010,
		.bitperpixel	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "ABGR2101010",
		.pixelformat	= V4L2_PIX_FMT_ABGR2101010,
		.cfg_val	= SCALER_CFG_FMT_ABGR2101010,
		.bitperpixel	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "RGBA1010102",
		.pixelformat	= V4L2_PIX_FMT_RGBA1010102,
		.cfg_val	= SCALER_CFG_FMT_RGBA1010102,
		.bitperpixel	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "BGRA1010102",
		.pixelformat	= V4L2_PIX_FMT_BGRA1010102,
		.cfg_val	= SCALER_CFG_FMT_BGRA1010102,
		.bitperpixel	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21,
		.cfg_val	= SCALER_CFG_FMT_YCRCB420_2P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 non-contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12M,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 non-contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21M,
		.cfg_val	= SCALER_CFG_FMT_YCRCB420_2P,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 3-planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420,	/* I420 */
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YVU 4:2:0 contiguous 3-planar, Y/Cr/Cb",
		.pixelformat	= V4L2_PIX_FMT_YVU420,	/* YV12 */
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 non-contiguous 3-planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420M,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 8, 2, 2 },
		.num_planes	= 3,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YVU 4:2:0 non-contiguous 3-planar, Y/Cr/Cb",
		.pixelformat	= V4L2_PIX_FMT_YVU420M,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 8, 2, 2 },
		.num_planes	= 3,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 packed, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.cfg_val	= SCALER_CFG_FMT_YUYV,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 packed, CbYCrY",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
		.cfg_val	= SCALER_CFG_FMT_UYVY,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 packed, YCrYCb",
		.pixelformat	= V4L2_PIX_FMT_YVYU,
		.cfg_val	= SCALER_CFG_FMT_YVYU,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16,
		.cfg_val	= SCALER_CFG_FMT_YCBCR422_2P,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV61,
		.cfg_val	= SCALER_CFG_FMT_YCRCB422_2P,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 3-planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,
		.cfg_val	= SCALER_CFG_FMT_YCBCR422_3P,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 3,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12N,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous Y/CbCr 10-bit",
		.pixelformat	= V4L2_PIX_FMT_NV12N_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_10BIT_S10,
		.bitperpixel	= { 15 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 3-planar Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420N,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr 8+2 bit",
		.pixelformat	= V4L2_PIX_FMT_NV12M_S10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_10BIT_S10,
		.bitperpixel	= { 10, 5 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr 10-bit",
		.pixelformat	= V4L2_PIX_FMT_NV12M_P010,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_BYTE_SWAP |
					SCALER_CFG_10BIT_P010,
		.bitperpixel	= { 16, 8 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr 10-bit",
		.pixelformat	= V4L2_PIX_FMT_NV12_P010,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_BYTE_SWAP |
					SCALER_CFG_10BIT_P010,
		.bitperpixel	= { 24 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CbCr 8+2 bit",
		.pixelformat	= V4L2_PIX_FMT_NV16M_S10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR422_2P |
					SCALER_CFG_10BIT_S10,
		.bitperpixel	= { 10, 10 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CrCb 8+2 bit",
		.pixelformat	= V4L2_PIX_FMT_NV61M_S10B,
		.cfg_val	= SCALER_CFG_FMT_YCRCB422_2P |
					SCALER_CFG_10BIT_S10,
		.bitperpixel	= { 10, 10 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC 8 bit",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWC_8B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_SBWC_FORMAT,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC 10 bit",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWC_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC,
		.bitperpixel	= { 10, 5 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CrCb SBWC 8 bit",
		.pixelformat	= V4L2_PIX_FMT_NV21M_SBWC_8B,
		.cfg_val	= SCALER_CFG_FMT_YCRCB420_2P |
					SCALER_CFG_SBWC_FORMAT,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CrCb SBWC 10 bit",
		.pixelformat	= V4L2_PIX_FMT_NV21M_SBWC_10B,
		.cfg_val	= SCALER_CFG_FMT_YCRCB420_2P	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC,
		.bitperpixel	= { 10, 5 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr SBWC 8 bit",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWC_8B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_SBWC_FORMAT,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr SBWC 10 bit",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWC_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC,
		.bitperpixel	= { 10, 5 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC lossy 8 bit",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWCL_8B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT,
		.bitperpixel	= { 6, 3 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC lossy 10 bit",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWCL_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "Y mono",
		.pixelformat	= V4L2_PIX_FMT_GREY,
		.cfg_val	= SCALER_CFG_FMT_Y_MONO,
		.bitperpixel	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
	}, {
		.name		= "Y mono 10 bit",
		.pixelformat	= V4L2_PIX_FMT_Y10,
		.cfg_val	= SCALER_CFG_FMT_Y_MONO |
				  SCALER_CFG_10BIT_P010,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC v2.7 lossy 8 bit 32 align",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWCL_32_8B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_SBWC_BYTE_ALIGN(32),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC v2.7 lossy 8 bit 64 align",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWCL_64_8B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_SBWC_BYTE_ALIGN(64),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr SBWC v2.7 lossy 8 bit 32 align",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWCL_32_8B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_SBWC_BYTE_ALIGN(32),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC v2.7 lossy 8 bit 64 align",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWCL_64_8B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_SBWC_BYTE_ALIGN(64),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC v2.7 lossy 10 bit 32 align",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWCL_32_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC	|
					SCALER_CFG_SBWC_BYTE_ALIGN(32),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC v2.7 lossy 10 bit 64 align",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWCL_64_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC	|
					SCALER_CFG_SBWC_BYTE_ALIGN(64),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr SBWC v2.7 lossy 10 bit 32 align",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWCL_32_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC	|
					SCALER_CFG_SBWC_BYTE_ALIGN(32),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC v2.7 lossy 10 bit 64 align",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWCL_64_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC	|
					SCALER_CFG_SBWC_BYTE_ALIGN(64),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr with SBWC layout",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWC_DECOMP,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr 10-bit with SBWC layout",
		.pixelformat	= V4L2_PIX_FMT_P010N_SBWC_DECOMP,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_BYTE_SWAP |
					SCALER_CFG_10BIT_P010,
		.bitperpixel	= { 24 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr aligned by 256",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWC_256_8B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_SBWC_BYTE_ALIGN(128),
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr 10-bit aligned by 256",
		.pixelformat	= V4L2_PIX_FMT_NV12N_SBWC_256_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC	|
					SCALER_CFG_SBWC_BYTE_ALIGN(128),
		.bitperpixel	= { 24 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC v2.8 lossy FR 8 bit 64 align",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWCL_64_8B_FR,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_SBWC_LOSSY_FR_MODE |
					SCALER_CFG_SBWC_BYTE_ALIGN(64),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr SBWC v2.8 lossy FR 10 bit 64 align",
		.pixelformat	= V4L2_PIX_FMT_NV12M_SBWCL_64_10B_FR,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P	|
					SCALER_CFG_SBWC_LOSSY	|
					SCALER_CFG_SBWC_FORMAT	|
					SCALER_CFG_10BIT_SBWC	|
					SCALER_CFG_SBWC_LOSSY_FR_MODE |
					SCALER_CFG_SBWC_BYTE_ALIGN(64),
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	},
};

#define SCALE_RATIO_CONST(x, y) (u32)((1048576ULL * (x)) / (y))

#define SCALE_RATIO(x, y)						\
({									\
		u32 ratio;						\
		if (__builtin_constant_p(x) && __builtin_constant_p(y))	{\
			ratio = SCALE_RATIO_CONST(x, y);		\
		} else if ((x) < 2048) {				\
			ratio = (u32)((1048576UL * (x)) / (y));		\
		} else {						\
			unsigned long long dividend = 1048576ULL;	\
			dividend *= x;					\
			do_div(dividend, y);				\
			ratio = (u32)dividend;				\
		}							\
		ratio;							\
})

#define SCALE_RATIO_FRACT(x, y, z) (u32)(((x << 20) + SCALER_FRACT_VAL(y)) / z)

/* must specify in revers order of SCALER_VERSION(xyz) */
static const u32 sc_version_table[][2] = {
        { 0x10000101, SCALER_VERSION(10, 0, 1) }, /* SC_POLY */
	{ 0x10000000, SCALER_VERSION(10, 0, 0) }, /* SC_POLY */
	{ 0x09000101, SCALER_VERSION(9, 0, 1) }, /* SC_POLY */
	{ 0x09000000, SCALER_VERSION(9, 0, 0) }, /* SC_POLY */
	{ 0x08000000, SCALER_VERSION(8, 0, 0) }, /* SC_POLY */
        { 0x07000103, SCALER_VERSION(7, 0, 1) }, /*SC_POLY*/
	{ 0x07000101, SCALER_VERSION(7, 0, 1) }, /* SC_POLY */
	{ 0x07000100, SCALER_VERSION(7, 0, 1) }, /* SC_POLY */
	{ 0x07000000, SCALER_VERSION(7, 0, 1) }, /* SC_POLY */
	{ 0x06000100, SCALER_VERSION(6, 0, 1) }, /* SC_POLY */
	{ 0x06000000, SCALER_VERSION(5, 2, 0) }, /* SC_POLY */
	{ 0x05000100, SCALER_VERSION(5, 2, 0) }, /* SC_POLY */
	{ 0x05000000, SCALER_VERSION(5, 2, 0) }, /* SC_POLY */
	{ 0x04000001, SCALER_VERSION(5, 1, 0) }, /* SC_POLY */
	{ 0x04000000, SCALER_VERSION(5, 1, 0) }, /* SC_POLY */
	{ 0x02000100, SCALER_VERSION(5, 0, 1) }, /* SC_POLY */
	{ 0x02000000, SCALER_VERSION(5, 0, 0) },
	{ 0x80060007, SCALER_VERSION(4, 2, 0) }, /* SC_BI */
	{ 0x01200100, SCALER_VERSION(4, 2, 0) }, /* SC_BI */
	{ 0x0100000f, SCALER_VERSION(4, 0, 1) }, /* SC_POLY */
	{ 0xA0000013, SCALER_VERSION(4, 0, 1) },
	{ 0xA0000012, SCALER_VERSION(4, 0, 1) },
	{ 0x80050007, SCALER_VERSION(4, 0, 0) }, /* SC_POLY */
	{ 0xA000000B, SCALER_VERSION(3, 0, 2) },
	{ 0xA000000A, SCALER_VERSION(3, 0, 2) },
	{ 0x8000006D, SCALER_VERSION(3, 0, 1) },
	{ 0x80000068, SCALER_VERSION(3, 0, 0) },
	{ 0x8004000C, SCALER_VERSION(2, 2, 0) },
	{ 0x80000008, SCALER_VERSION(2, 1, 1) },
	{ 0x80000048, SCALER_VERSION(2, 1, 0) },
	{ 0x80010000, SCALER_VERSION(2, 0, 1) },
	{ 0x80000047, SCALER_VERSION(2, 0, 0) },
};

static const struct sc_variant sc_variant[] = {
	{
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 16384,
			.max_h		= 16384,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 16384,
			.max_h		= 16384,
		},
		.version		= SCALER_VERSION(7, 0, 1),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v6,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 16384,
			.max_h		= 16384,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 16384,
			.max_h		= 16384,
		},
		.version		= SCALER_VERSION(6, 0, 1),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v6,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(5, 2, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v5,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(5, 1, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v4,
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
		.minsize_srcplane	= 4096 + 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(5, 0, 1),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v4,
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
		.extra_buf		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(5, 0, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v4,
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
		.extra_buf		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(4, 2, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v3,
		.blending		= 1,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.is_bilinear		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(4, 0, 1),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 16),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v4,
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(4, 0, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v3,
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 0,
		.initphase		= 0,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(3, 0, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(16, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL_v3,
		.blending		= 0,
		.prescale		= 1,
		.ratio_20bit		= 1,
		.initphase		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(2, 2, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL,
		.blending		= 1,
		.prescale		= 0,
		.ratio_20bit		= 0,
		.initphase		= 0,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(2, 0, 1),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL,
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 0,
		.initphase		= 0,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 4096,
			.max_h		= 4096,
		},
		.version		= SCALER_VERSION(2, 0, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.int_en_mask		= SCALER_INT_EN_ALL,
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 0,
		.initphase		= 0,
	},
};

#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS) || IS_ENABLED(CONFIG_EXYNOS_PM_QOS_MODULE)
static void sc_pm_qos_update_devfreq(struct exynos_pm_qos_request *qos_req,
				int pm_qos_class, u32 freq)
{
	if (!exynos_pm_qos_request_active(qos_req))
		exynos_pm_qos_add_request(qos_req, pm_qos_class, 0);

	exynos_pm_qos_update_request(qos_req, freq);
}

static void sc_pm_qos_remove_devfreq(struct exynos_pm_qos_request *qos_req)
{
	if (exynos_pm_qos_request_active(qos_req))
		exynos_pm_qos_remove_request(qos_req);
}
#else
#define sc_pm_qos_update_devfreq(qos_req, pm_qos_class, freq) do { } while (0)
#define sc_pm_qos_remove_devfreq(qos_req) do { } while (0)
#endif

void sc_remove_devfreq(struct sc_ctx *ctx)
{
	__maybe_unused struct sc_qos_request *qos_req = &ctx->pm_qos;
	int bts_id = ctx->sc_dev->bts_id;

	if (bts_id >= 0) {
		__maybe_unused struct bts_bw bw = { 0 };

		bts_update_bw(bts_id, bw);
	}

	sc_pm_qos_remove_devfreq(&qos_req->mif_req);
	if (ctx->sc_dev->min_bus_int_table_cnt != 0)
		sc_pm_qos_remove_devfreq(&qos_req->bus_int_req);
	sc_pm_qos_remove_devfreq(&qos_req->dev_req);

	ctx->pm_qos_lv = -1;
	ctx->mif_freq_req = 0;
}

#define BIT_SZ_OF_HEADER_UNIT	(4)
unsigned int sc_calc_sbwc_bandwidth(struct sc_frame *frame, int framerate,
				    bool is_read, bool is_rotation)
{
	unsigned long bw_payload, bw_header, bw_total;
	unsigned long pixel_count = ALIGN(frame->crop.width, 32) *
				    ALIGN(frame->crop.height, 8);
	unsigned long bpp;
	unsigned long blocksize;

	switch (frame->sc_fmt->pixelformat) {
	case V4L2_PIX_FMT_NV12M_SBWC_8B:
	case V4L2_PIX_FMT_NV21M_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B_FR:
		// TODO : should update calculation of SBWC v2.7
		bpp = 12;
		blocksize = 32 * 4;
		/*
		 * In worst case, compression-rate is 60%.
		 */
		bw_payload = pixel_count * bpp * framerate * 6 / 10;
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_10B:
	case V4L2_PIX_FMT_NV21M_SBWC_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_10B:
	case V4L2_PIX_FMT_NV12M_SBWCL_10B:
	case V4L2_PIX_FMT_NV12M_SBWCL_32_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_10B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B_FR:
		// TODO : should update calculation of SBWC v2.7
		bpp = 15;
		blocksize = 32 * 5;
		/*
		 * In worst case, compression-rate is 50%.
		 */
		bw_payload = pixel_count * bpp * framerate * 5 / 10;
		break;
	default:
		bw_payload = 0;
		break;
	}

	if (bw_payload == 0)
		return 0;

	bw_header = pixel_count * bpp * framerate; /* the scale of bandwidth is bits/sec */
	bw_header *= BIT_SZ_OF_HEADER_UNIT; /* the size of a block of SBWC header is 4 bits */
	bw_header /= BITS_PER_BYTE * blocksize; /* header block is a metadata per payload block */

	bw_total = bw_payload + bw_header;

	if (is_read) {
		/*
		 * In read SBWC, there is overhead.
		 * In no rotaion, overhead is 8 bytes per unit of read.
		 * In rotaion, overhead is 12 bytes per unit of read.
		 *
		 * Size of unit of read is 128 bytes on Y-plane
		 *			and 64 bytes on CbCr-plane.
		 * By considering portion of each plane in total bandwidth,
		 * calculation of overhead is simplificated as like below.
		 */
		if (is_rotation)
			bw_total += bw_total / 8;
		else
			bw_total += bw_total / 12;
	}

	return (unsigned int)(bw_total / (SZ_1K * BITS_PER_BYTE));
}

unsigned int sc_calc_bandwidth(struct sc_ctx *ctx, bool is_read)
{
	struct sc_frame *frame = (is_read) ? &ctx->s_frame : &ctx->d_frame;
	int framerate = ctx->framerate;
	unsigned long bw = 0;
	bool is_rotation = (ctx->flip_rot_cfg & SCALER_ROT_90) ? 1: 0;

	if (sc_fmt_is_sbwc(frame->sc_fmt->pixelformat))
		return sc_calc_sbwc_bandwidth(frame, framerate,
					      is_read, is_rotation);

	{
		unsigned int bpp = 0;
		unsigned int width, height;
		int i;

		width = frame->crop.width;
		height = frame->crop.height;

		for (i = 0; i < frame->sc_fmt->num_planes; i++)
			bpp += frame->sc_fmt->bitperpixel[i];

		bw = (unsigned long)(width * height) * framerate * bpp;

		if (is_read) {
			/*
			 * In read, the unit-size of read is different by format and rotation.
			 * And, there is overhead in read.
			 * Below table shows,
			 * format	: no-rot     rotate
			 *		 < (unit size of read) (overhead of each read) >
			 * RGBA		: < 32  4 > < 128 8 >
			 * YUV420 - y	: < 128 3 > < 128 8 >
			 * YUV420 - cbcr: < 64  3 > < 128 8 >
			 * YUV422 - y	: < 128 3 > < 128 8 >
			 * YUV422 - cbcr: < 64  3 > < 128 8 >
			 *
			 * So, with considering upper table and portion of cbcr in bandwidth,
			 * the calculation of overhead can be simplificated as like below code.
			 */
			if (is_rotation) {
				bw += bw / 16;
			} else {
				if (frame->sc_fmt->is_rgb)
					bw += bw / 8;
				else if (sc_fmt_is_yuv420(frame->sc_fmt->pixelformat))
					bw += bw / 32;
				else if (sc_fmt_is_yuv422(frame->sc_fmt->pixelformat))
					bw += bw * 9 / 256;
			}
		}

		bw /= (SZ_1K * BITS_PER_BYTE);
	}

	return (unsigned int)bw;
}

void sc_get_bandwidth(struct sc_ctx *ctx, struct bts_bw *bw)
{
	bw->read = sc_calc_bandwidth(ctx, true);
	bw->write = sc_calc_bandwidth(ctx, false);
	bw->peak = (bw->read + bw->write) / 2;
}

unsigned int sc_get_mif_freq_by_bw(struct sc_ctx *ctx,
				   unsigned int bw_read, unsigned int bw_write)
{
	/*
	 * The reference bandwidth is the bandwidth of image processing of a specific workload.
	 * And the reference MIF frequency is the required MIF frequency
	 * when the workload satisfies its performance requirements.
	 *
	 * Calculate MIF by linear function of bandwidth.
	 * The function is based on bw_ref and mif_ref.
	 */
	int mif_ref = ctx->sc_dev->mif_ref;
	int bw_ref = ctx->sc_dev->bw_ref;
	unsigned long total_bw = bw_read + bw_write;

	return (unsigned int)(total_bw * mif_ref / bw_ref);
}

static int sc_get_min_bus_int(struct sc_dev *sc, struct sc_frame *frame);

void sc_request_devfreq(struct sc_ctx *ctx, int lv)
{
	struct sc_dev *sc = ctx->sc_dev;
	__maybe_unused struct sc_qos_request *qos_req = &ctx->pm_qos;
	__maybe_unused struct sc_qos_table *qos_table = sc->qos_table;
	int bts_id = sc->bts_id;
	struct bts_bw bw;
	unsigned int req_mif;

	if (lv < 0 || lv >= sc->qos_table_cnt) {
		dev_err(sc->dev, "%s: requested lv(%d) is invalid (0 ~ %d)\n",
			__func__, lv, sc->qos_table_cnt - 1);
		return;
	}

	sc_get_bandwidth(ctx, &bw);

	req_mif = sc_get_mif_freq_by_bw(ctx, bw.read, bw.write);

	/* No request is required if level is same. */
	if (req_mif != ctx->mif_freq_req) {
		if (bts_id >= 0)
			bts_update_bw(bts_id, bw);

		ctx->mif_freq_req = req_mif;

		sc_pm_qos_update_devfreq(&qos_req->mif_req, PM_QOS_BUS_THROUGHPUT,
					 req_mif);

		if (sc->min_bus_int_table_cnt != 0) {
			int min_bus_int;
			int src_min_bus_int, dst_min_bus_int;

			src_min_bus_int = sc_get_min_bus_int(sc, &ctx->s_frame);
			dst_min_bus_int = sc_get_min_bus_int(sc, &ctx->d_frame);

			min_bus_int = (src_min_bus_int > dst_min_bus_int) ?
				src_min_bus_int : dst_min_bus_int;

			sc_pm_qos_update_devfreq(&qos_req->bus_int_req,
						 PM_QOS_DEVICE_THROUGHPUT,
						 min_bus_int);
		}
	}
	if (lv != ctx->pm_qos_lv) {
		ctx->pm_qos_lv = lv;

		sc_pm_qos_update_devfreq(&qos_req->dev_req, sc->dvfs_class,
					 qos_table[lv].freq_int);
	}
}

/*
 * Required Clock (KHz) = (width * height) / (process time(ms) * ppc)
 */

/*
 * We cannot tell the effective bit-depth of SBWC since it is compressed.
 * So we have decided the bit-depth of SBWC as 100
 * to find the corresponding the hardware performance (pixels per clock)
 * specified in the device-tree.
 */
#define SC_BPP_OF_SBWC		100
static int sc_get_clock_khz(struct sc_ctx *ctx,
			struct sc_frame *frame, int process_time)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_ppc_table *ppc_table = sc->ppc_table;
	int ppc_idx = (ctx->flip_rot_cfg & SCALER_ROT_90) ? 1 : 0;
	int i, bpp = 0, ppc = 0;
	unsigned long clk;

	if (sc_fmt_is_sbwc(frame->sc_fmt->pixelformat))
		bpp = SC_BPP_OF_SBWC;
	else
		for (i = 0; i < frame->sc_fmt->num_planes; i++)
			bpp += frame->sc_fmt->bitperpixel[i];

	for (i = 0; i < sc->ppc_table_cnt; i++) {
		if (ppc_table[i].bpp == bpp) {
			ppc = ppc_table[i].ppc[ppc_idx];
			break;
		}
	}

	if (!ppc || !process_time)
		return 0;

	/*
	 * process_time is unit of microseconds,
	 * and ppc was already multiplied by 100 to eliminate the decimal point.
	 * So clk should be multiplied by 100000.
	 */
	clk = (unsigned long)(frame->crop.width * frame->crop.height) * 100000;
	clk /= (process_time * ppc);

	if (clk > INT_MAX)
		clk = INT_MAX;

	return (int)clk;
}

static int sc_get_min_bus_int(struct sc_dev *sc, struct sc_frame *frame)
{
	int bpp = 0;
	int i;

	if (sc_fmt_is_sbwc(frame->sc_fmt->pixelformat))
		bpp = SC_BPP_OF_SBWC;
	else
		for (i = 0; i < frame->sc_fmt->num_planes; i++)
			bpp += frame->sc_fmt->bitperpixel[i];

	for (i = 0 ; i < sc->min_bus_int_table_cnt; i++) {
		if (sc->min_bus_int_table[i].bpp == bpp)
			return sc->min_bus_int_table[i].min_bus_int;
	}

	return 0;
}

/* Find the matches format */
static const struct sc_fmt *sc_find_format(struct sc_dev *sc,
						u32 pixfmt, bool output_buf)
{
	const struct sc_fmt *sc_fmt;
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(sc_formats); ++i) {
		sc_fmt = &sc_formats[i];
		if (sc_fmt->pixelformat == pixfmt) {
			return &sc_formats[i];
		}
	}

	return NULL;
}

static int sc_v4l2_querycap(struct file *file, void *fh,
			     struct v4l2_capability *cap)
{
	strncpy(cap->driver, MODULE_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, MODULE_NAME, sizeof(cap->card) - 1);

	return 0;
}

static int sc_v4l2_g_fmt_mplane(struct file *file, void *fh,
			  struct v4l2_format *f)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	const struct sc_fmt *sc_fmt;
	struct sc_frame *frame;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	int i;

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	sc_fmt = frame->sc_fmt;

	pixm->width		= frame->width;
	pixm->height		= frame->height;
	pixm->pixelformat	= frame->pixelformat;
	pixm->field		= V4L2_FIELD_NONE;
	pixm->num_planes	= frame->sc_fmt->num_planes;
	pixm->colorspace	= 0;

	for (i = 0; i < pixm->num_planes; ++i) {
		pixm->plane_fmt[i].bytesperline = frame->stride[i];
		pixm->plane_fmt[i].sizeimage = frame->addr.size[i];

		v4l2_dbg(1, sc_log_level, &ctx->sc_dev->m2m.v4l2_dev,
				"[%d] plane: bytesperline %d, sizeimage %d\n",
				i, pixm->plane_fmt[i].bytesperline,
				pixm->plane_fmt[i].sizeimage);
	}

	return 0;
}

int sc_calc_s10b_planesize(u32 pixelformat, u32 width, u32 height,
		u32 *ysize, u32 *csize, bool only_8bit, u8 byte32num)
{
	int ret = 0;
	int c_padding = 0;

	switch (pixelformat) {
	case V4L2_PIX_FMT_NV12M_S10B:
		*ysize = NV12M_Y_SIZE(width, height);
		*csize = NV12M_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12N_10B:
		*ysize = NV12N_Y_SIZE(width, height);
		*csize = NV12N_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		*ysize = NV16M_Y_SIZE(width, height);
		*csize = NV16M_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_8B:
	case V4L2_PIX_FMT_NV21M_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_DECOMP:
		*ysize = SBWC_8B_Y_SIZE(width, height);
		*csize = SBWC_8B_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_10B:
	case V4L2_PIX_FMT_NV21M_SBWC_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_10B:
	case V4L2_PIX_FMT_P010N_SBWC_DECOMP:
		*ysize = SBWC_10B_Y_SIZE(width, height);
		*csize = SBWC_10B_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_10B:
		*ysize = SBWCL_Y_SIZE(width, height, byte32num);
		*csize = SBWCL_CBCR_SIZE(width, height, byte32num);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_32_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_10B:
		*ysize = SBWCL_32_Y_SIZE(width, height);
		*csize = SBWCL_32_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_10B:
		*ysize = SBWCL_64_Y_SIZE(width, height);
		*csize = SBWCL_64_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
		*ysize = SBWC_256_8B_Y_SIZE(width, height);
		*csize = SBWC_256_8B_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
		*ysize = SBWC_256_10B_Y_SIZE(width, height);
		*csize = SBWC_256_10B_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B_FR:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B_FR:
		*ysize = SBWCL_64_FR_Y_SIZE(width, height);
		*csize = SBWCL_64_FR_CBCR_SIZE(width, height);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret || only_8bit)
		return ret;

	switch (pixelformat) {
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV12N_10B:
		*ysize += NV12M_Y_2B_SIZE(width, height);
		*csize += NV12M_CBCR_2B_SIZE(width, height);
		c_padding = 256;
		break;
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
		*ysize += NV16M_Y_2B_SIZE(width, height);
		*csize += NV16M_CBCR_2B_SIZE(width, height);
		c_padding = 256;
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_8B:
	case V4L2_PIX_FMT_NV21M_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_8B:
	case V4L2_PIX_FMT_NV12N_SBWC_DECOMP:
		*ysize += SBWC_8B_Y_HEADER_SIZE(width, height);
		*csize += SBWC_8B_CBCR_HEADER_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWC_10B:
	case V4L2_PIX_FMT_NV21M_SBWC_10B:
	case V4L2_PIX_FMT_NV12N_SBWC_10B:
	case V4L2_PIX_FMT_P010N_SBWC_DECOMP:
		*ysize += SBWC_10B_Y_HEADER_SIZE(width, height);
		*csize += SBWC_10B_CBCR_HEADER_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_10B:
		/* Do nothing */
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_32_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_32_10B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_8B:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B:
	case V4L2_PIX_FMT_NV12N_SBWCL_64_10B:
		*ysize += SBWCL_Y_HEADER_SIZE(width, height);
		*csize += SBWCL_CBCR_HEADER_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
		*ysize += SBWC_256_8B_Y_HEADER_SIZE(width, height);
		*csize += SBWC_256_8B_CBCR_HEADER_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
		*ysize += SBWC_256_10B_Y_HEADER_SIZE(width, height);
		*csize += SBWC_256_10B_CBCR_HEADER_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV12M_SBWCL_64_8B_FR:
	case V4L2_PIX_FMT_NV12M_SBWCL_64_10B_FR:
		*ysize += SBWCL_Y_HEADER_SIZE(width, height);
		*csize += SBWCL_CBCR_HEADER_SIZE(width, height);
	}

	/* Do not consider extra size for 2bit CbCr (for S10B only) */
	*csize -= c_padding;

	return ret;
}

static int get_lossy_byte32num(const struct sc_fmt *sc_fmt, __u8 blocksize)
{
	int i;
	__u8 avail_size[] = { 0, 0, 64, 96, 128 };
	__u8 min = 2, max = 3;

	if ((sc_fmt->cfg_val & SCALER_CFG_10BIT_MASK) == SCALER_CFG_10BIT_SBWC)
		max = 4;

	for (i = min; i <= max; i++)
		if (avail_size[i] == blocksize)
			return i;

	return 0;
}

static int sc_calc_fmt_s10b_size(const struct sc_fmt *sc_fmt,
		struct v4l2_pix_format_mplane *pixm, unsigned int ext_size)
{
	__u8 byte32num = 0;
	u32 y_size, c_size;
	int i, ret;

	BUG_ON(sc_fmt->num_comp != 2);

	for (i = 0; i < pixm->num_planes; i++)
		pixm->plane_fmt[i].bytesperline = (pixm->width *
				sc_fmt->bitperpixel[i]) >> 3;

	if (sc_fmt_is_sbwc_lossy(sc_fmt->pixelformat)) {
		byte32num = get_lossy_byte32num(sc_fmt, pixm->flags);
		if (!byte32num) {
			pr_err("Wrong blocksize(%d) for lossy\n", pixm->flags);
			return -EINVAL;
		}
	}

	ret = sc_calc_s10b_planesize(sc_fmt->pixelformat,
				     pixm->width, pixm->height,
				     &y_size, &c_size, false, byte32num);
	if (ret) {
		pr_err("Scaler doesn't support %s format\n", sc_fmt->name);
		return ret;
	}

	if (pixm->num_planes == 1) {
		pixm->plane_fmt[0].sizeimage = y_size + c_size;
	} else if (pixm->num_planes == 2) {
		pixm->plane_fmt[0].sizeimage = y_size;
		pixm->plane_fmt[1].sizeimage = c_size;
	}

	for (i = 0; ext_size && i < pixm->num_planes; i++)
		pixm->plane_fmt[i].sizeimage += (i == 0) ? ext_size : ext_size/2;

	return ret;
}

static void sc_calc_ayv12_planesize(const __u32 y_stride, const __u32 c_stride,
				    const __u32 height,
				    u32 *y_plane, u32 *cb_plane, u32 *cr_plane)
{
	*y_plane = y_stride * height;
	*cb_plane = *cr_plane = c_stride * height / 2;
}

static int sc_calc_fmt_ayv12_planesize(const u32 fmt, const __u32 y_stride,
				       const __u32 c_stride, const __u32 height,
				       u32 *y_plane, u32 *cb_plane,
				       u32 *cr_plane)
{
	unsigned int y_size, cb_size, cr_size;

	sc_calc_ayv12_planesize(y_stride, c_stride, height,
				&y_size, &cb_size, &cr_size);

	if (fmt == V4L2_PIX_FMT_YVU420) {
		*y_plane = y_size + cb_size + cr_size;
	} else if (fmt == V4L2_PIX_FMT_YVU420M) {
		*y_plane = y_size;
		*cb_plane = cb_size;
		*cr_plane = cr_size;
	}

	return 0;
}

static void sc_calc_planesize(struct sc_frame *frame, unsigned int pixsize)
{
	int idx = frame->sc_fmt->num_planes;

	while (idx-- > 0)
		frame->addr.size[idx] =
			(pixsize * frame->sc_fmt->bitperpixel[idx]) / 8;
}

static void sc_calc_bufsize(struct sc_dev *sc, struct sc_frame *frame)
{
	unsigned int pixsize, bytesize;
	unsigned int ext_size = 0, i;
	u32 min_size = sc->variant->minsize_srcplane;

	pixsize = frame->width * frame->height;
	bytesize = (pixsize * frame->sc_fmt->bitperpixel[0]) >> 3;

	frame->addr.size[SC_PLANE_CB] = 0;
	frame->addr.size[SC_PLANE_CR] = 0;

	if (sc->variant->extra_buf)
		ext_size = sc_ext_buf_size(frame->width);

	switch (frame->sc_fmt->num_comp) {
	case 1: /* rgb, yuyv */
		frame->addr.size[SC_PLANE_Y] = bytesize;
		break;
	case 2:
		if (sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat) ||
		    sc_fmt_is_sbwc(frame->sc_fmt->pixelformat) ||
		    sc_fmt_with_sbwc_size(frame->sc_fmt->pixelformat)) {
			sc_calc_s10b_planesize(frame->sc_fmt->pixelformat,
					frame->width, frame->height,
					&frame->addr.size[SC_PLANE_Y],
					&frame->addr.size[SC_PLANE_CB],
					false, frame->byte32num);
		} else if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12N) {
			unsigned int w = frame->width;
			unsigned int h = frame->height;

			frame->addr.size[SC_PLANE_Y] = NV12N_Y_SIZE(w, h);
			frame->addr.size[SC_PLANE_CB] = NV12N_CBCR_SIZE(w, h);
		} else if (frame->sc_fmt->num_planes == 1) {
			if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12_P010)
				pixsize *= 2;
			frame->addr.size[SC_PLANE_Y] = pixsize;
			frame->addr.size[SC_PLANE_CB] = bytesize - pixsize;
		} else if (frame->sc_fmt->num_planes == 2) {
			sc_calc_planesize(frame, pixsize);
		}
		break;
	case 3:
		if (sc_fmt_is_ayv12(frame->sc_fmt->pixelformat)) {
			sc_calc_ayv12_planesize(frame->stride[0], frame->stride[1],
						frame->height,
						&frame->addr.size[SC_PLANE_Y],
						&frame->addr.size[SC_PLANE_CB],
						&frame->addr.size[SC_PLANE_CR]);
		} else if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_YUV420N) {
			unsigned int w = frame->width;
			unsigned int h = frame->height;

			frame->addr.size[SC_PLANE_Y] = YUV420N_Y_SIZE(w, h);
			frame->addr.size[SC_PLANE_CB] = YUV420N_CB_SIZE(w, h);
			frame->addr.size[SC_PLANE_CR] = YUV420N_CR_SIZE(w, h);
		} else if (frame->sc_fmt->num_planes == 1) {
			frame->addr.size[SC_PLANE_Y] = pixsize;
			frame->addr.size[SC_PLANE_CB] =
						(bytesize - pixsize) / 2;
			frame->addr.size[SC_PLANE_CR] =
						frame->addr.size[SC_PLANE_CB];
		} else if (frame->sc_fmt->num_planes == 3) {
			sc_calc_planesize(frame, pixsize);
		} else {
			dev_err(sc->dev, "Please check the num of comp\n");
		}

		break;
	default:
		break;
	}

	for (i = 0; ext_size && i < frame->sc_fmt->num_comp; i++)
		frame->addr.size[i] += (i == 0) ? ext_size : ext_size/2;

	for (i = 0; i < frame->sc_fmt->num_comp; i++) {
		if (frame->addr.size[i] < min_size)
			frame->addr.size[i] = min_size;
	}

}

static inline int sc_get_sbwc_span_bytes(const struct sc_fmt *fmt,
					int width, unsigned short ratio)
{
	if (fmt->cfg_val & SCALER_CFG_SBWC_BYTE_ALIGN(128)) {
		if ((fmt->cfg_val & SCALER_CFG_10BIT_MASK) == SCALER_CFG_10BIT_SBWC)
			return SBWC_256_10B_STRIDE(width);
		else
			return SBWC_256_8B_STRIDE(width);
	}

	/* Lossy case */
	if (fmt->cfg_val & SCALER_CFG_SBWC_LOSSY) {
		if (sc_fmt_is_sbwc_lossy_v2_7(fmt->pixelformat)) {
			if ((fmt->cfg_val & SCALER_CFG_SBWC_BYTE_ALIGN_MASK) ==
					SCALER_CFG_SBWC_BYTE_ALIGN(32))
				return SBWCL_32_STRIDE(width);
			else if ((fmt->cfg_val & SCALER_CFG_SBWC_BYTE_ALIGN_MASK) ==
					SCALER_CFG_SBWC_BYTE_ALIGN(64))
				return SBWCL_64_STRIDE(width);
		} else if (fmt->cfg_val & SCALER_CFG_SBWC_LOSSY_FR_MODE) {
			return SBWCL_64_FR_STRIDE(width);
		} else {
			return SBWCL_STRIDE(width, ratio);
		}
	}

	/* Lossless case */
	if ((fmt->cfg_val & SCALER_CFG_10BIT_MASK) == SCALER_CFG_10BIT_SBWC)
		return SBWC_10B_STRIDE(width);

	return SBWC_8B_STRIDE(width);
}

void sc_get_span(const struct sc_fmt *fmt, const __u32 width,
		 u32 *yspan, u32 *cspan, __u8 byte32num)
{
	if (sc_fmt_is_sbwc(fmt->pixelformat)) {
		*yspan = *cspan  = sc_get_sbwc_span_bytes(fmt, width, byte32num);
	} else {
		*yspan = width;

		if (fmt->num_comp == 2) {
			*cspan = width << fmt->cspan;
		} else if (fmt->num_comp == 3) {
			if (sc_fmt_is_ayv12(fmt->pixelformat))
				*cspan = ALIGN(width >> 1, 16);
			else if (fmt->cspan) /* YUV444 */
				*cspan = width;
			else
				*cspan = width >> 1;
		} else {
			*cspan = 0;
		}
	}
}

static int sc_v4l2_try_fmt_mplane(struct file *file, void *fh,
			    struct v4l2_format *f)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	const struct sc_fmt *sc_fmt;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	const struct sc_size_limit *limit;
	int i;
	int h_align = 0;
	int w_align = 0;
	unsigned int ext_size = 0;

	/*
	 * v4l2-ioctl checked whether the type is multi-planar format or not.
	 * So, there is no need to check it in here.
	 */

	sc_fmt = sc_find_format(ctx->sc_dev, f->fmt.pix_mp.pixelformat, V4L2_TYPE_IS_OUTPUT(f->type));
	if (!sc_fmt) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"not supported format type\n");
		return -EINVAL;
	}

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		limit = &ctx->sc_dev->variant->limit_input;
	else
		limit = &ctx->sc_dev->variant->limit_output;

	/*
	 * Y_SPAN - should even in interleaved YCbCr422
	 * C_SPAN - should even in YCbCr420 and YCbCr422
	 */
	w_align = sc_fmt->h_shift;
	h_align = sc_fmt->v_shift;

	/* Bound an image to have width and height in limit */
	v4l_bound_align_image(&pixm->width, limit->min_w, limit->max_w,
			w_align, &pixm->height, limit->min_h,
			limit->max_h, h_align, 0);

	pixm->num_planes = sc_fmt->num_planes;
	pixm->colorspace = 0;

	if (ctx->sc_dev->variant->extra_buf && V4L2_TYPE_IS_OUTPUT(f->type))
		ext_size = sc_ext_buf_size(pixm->width);

	if (sc_fmt_is_s10bit_yuv(sc_fmt->pixelformat) ||
	    sc_fmt_is_sbwc(sc_fmt->pixelformat) ||
	    sc_fmt_with_sbwc_size(sc_fmt->pixelformat))
		return sc_calc_fmt_s10b_size(sc_fmt, pixm, ext_size);
	else if (sc_fmt_is_ayv12(sc_fmt->pixelformat)) {
		if (pixm->plane_fmt[0].bytesperline == 0) {
			sc_get_span(sc_fmt, pixm->width,
				    &pixm->plane_fmt[SC_PLANE_Y].bytesperline,
				    &pixm->plane_fmt[SC_PLANE_CB].bytesperline,
				    0);
		}

		sc_calc_fmt_ayv12_planesize(sc_fmt->pixelformat,
					    pixm->plane_fmt[0].bytesperline,
					    pixm->plane_fmt[1].bytesperline,
					    pixm->height,
					    &pixm->plane_fmt[0].sizeimage,
					    &pixm->plane_fmt[1].sizeimage,
					    &pixm->plane_fmt[2].sizeimage);
	} else {
		for (i = 0; i < pixm->num_planes; ++i) {
			/*
			 * If user sets this value, it is meaning that there is some restriction about stride.
			 * If not, the driver will use the width in calculation of bytesperline.
			 * The value user sets will be the stride as pixel.
			 * So, the driver should calculate the bytesperline also.
			 */
			if (pixm->plane_fmt[i].bytesperline == 0)
				pixm->plane_fmt[i].bytesperline = pixm->width;

			pixm->plane_fmt[i].bytesperline *= sc_fmt->bitperpixel[i];
			pixm->plane_fmt[i].bytesperline >>= 3;

			pixm->plane_fmt[i].sizeimage =
				pixm->plane_fmt[i].bytesperline * pixm->height;

			v4l2_dbg(1, sc_log_level, &ctx->sc_dev->m2m.v4l2_dev,
				 "[%d] plane: bytesperline %d, sizeimage %d\n",
				 i, pixm->plane_fmt[i].bytesperline,
				 pixm->plane_fmt[i].sizeimage);
		}
	}

	for (i = 0; ext_size && i < pixm->num_planes; i++)
		pixm->plane_fmt[i].sizeimage += (i == 0) ? ext_size : ext_size/2;

	return 0;
}

static int sc_v4l2_s_fmt_mplane(struct file *file, void *fh,
				 struct v4l2_format *f)

{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	struct vb2_queue *vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	struct sc_frame *frame;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	__u32 stride[SC_MAX_PLANES] = {0};
	bool is_need_calc_stride = true;
	int i, ret = 0;

	if (vb2_is_streaming(vq)) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev, "device is busy\n");
		return -EBUSY;
	}

	if (!sc_fmt_is_sbwc(pixm->pixelformat)) {
		for (i = 0; i < SC_MAX_PLANES; i++) {
			if (pixm->plane_fmt[i].bytesperline != 0)
				stride[i] = pixm->plane_fmt[i].bytesperline;
			else
				break;

			is_need_calc_stride = false;
		}
	}

	ret = sc_v4l2_try_fmt_mplane(file, fh, f);
	if (ret < 0)
		return ret;

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	set_bit(CTX_PARAMS, &ctx->flags);

	/*
	 * format was validated by upper calling sc_v4l2_try_fmt_mplane().
	 * So, it is not necessary to check the returned value.
	 */
	frame->sc_fmt = sc_find_format(ctx->sc_dev, f->fmt.pix_mp.pixelformat, V4L2_TYPE_IS_OUTPUT(f->type));

	for (i = 0; i < frame->sc_fmt->num_planes; i++)
		frame->bytesused[i] = pixm->plane_fmt[i].sizeimage;

	if (pixm->flags == V4L2_PIX_FMT_FLAG_PREMUL_ALPHA &&
			ctx->sc_dev->version != SCALER_VERSION(4, 0, 0))
		frame->pre_multi = true;
	else
		frame->pre_multi = false;

	if (sc_fmt_is_sbwc(frame->sc_fmt->pixelformat)) {
		if (sc_fmt_is_sbwc_lossy(frame->sc_fmt->pixelformat)) {
			frame->byte32num = get_lossy_byte32num(frame->sc_fmt,
								pixm->flags);
			if (!frame->byte32num) {
				dev_err(ctx->sc_dev->dev,
					"%s: invalid lossy blocksize(%d)",
					__func__, pixm->flags);
				return -EINVAL;
			}
		}
	}

	frame->width = pixm->width;
	frame->height = pixm->height;
	frame->pixelformat = pixm->pixelformat;

	frame->crop.width = pixm->width;
	frame->crop.height = pixm->height;

	if (is_need_calc_stride) {
		sc_get_span(frame->sc_fmt, frame->width,
			    &frame->stride[SC_PLANE_Y],
			    &frame->stride[SC_PLANE_CB], frame->byte32num);
	} else {
		for (i = 0; i < frame->sc_fmt->num_comp; i++)
			frame->stride[i] = stride[i];
	}

	sc_calc_bufsize(ctx->sc_dev, frame);

	return 0;
}

static int sc_v4l2_reqbufs(struct file *file, void *fh,
			    struct v4l2_requestbuffers *reqbufs)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);

	return v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
}

#define sc_from_vb2_to_sc_buf(vb2_buf)					       \
({									       \
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb2_buf);	       \
	struct v4l2_m2m_buffer *m2m_buf =				       \
				container_of(v4l2_buf, typeof(*m2m_buf), vb);  \
	struct vb2_sc_buffer *sc_buf =					       \
				container_of(m2m_buf, typeof(*sc_buf), mb);    \
									       \
	sc_buf;								       \
})

static void __sc_vb2_buf_queue(struct v4l2_m2m_ctx *m2m_ctx,
			       struct vb2_v4l2_buffer *v4l2_buf);

static void sc_fence_cb(struct dma_fence *f, struct dma_fence_cb *cb)
{
	struct vb2_sc_buffer *sc_buf = container_of(cb, typeof(*sc_buf),
						    fence_cb);
	struct vb2_buffer *vb = &sc_buf->mb.vb.vb2_buf;
	struct sc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct dma_fence *fence;

	do {
		fence = sc_buf->in_fence;
	} while (cmpxchg(&sc_buf->in_fence, fence, NULL) != fence);

	if (!fence)
		return;

	del_timer(&sc_buf->fence_timer);

	if (fence->error) {
		dev_err(ctx->sc_dev->dev,
			"%s: in-fence: %s #%llu, error: %d\n",
		       __func__, fence->ops->get_driver_name(fence),
		       fence->seqno, fence->error);

		sc_buf->state = fence->error;
	}
	dma_fence_put(fence);

	schedule_work(&sc_buf->qbuf_work);
}

static void __sc_qbuf_work(struct work_struct *work)
{
	struct vb2_sc_buffer *sc_buf = container_of(work, typeof(*sc_buf),
						    qbuf_work);
	struct vb2_v4l2_buffer *v4l2_buf = &sc_buf->mb.vb;
	struct sc_ctx *ctx = vb2_get_drv_priv(v4l2_buf->vb2_buf.vb2_queue);

	__sc_vb2_buf_queue(ctx->m2m_ctx, v4l2_buf);
}

#define SC_FENCE_TIMEOUT	(3100)
static void sc_fence_timeout_handler(struct timer_list *t)
{
	struct vb2_sc_buffer *sc_buf = container_of(t, typeof(*sc_buf),
						    fence_timer);
	struct vb2_buffer *vb = &sc_buf->mb.vb.vb2_buf;
	struct sc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct dma_fence *fence;

	do {
		fence = sc_buf->in_fence;
	} while (cmpxchg(&sc_buf->in_fence, fence, NULL) != fence);

	if (!fence)
		return;

	dma_fence_remove_callback(fence, &sc_buf->fence_cb);

	dev_err(ctx->sc_dev->dev,
		"%s: timeout(%d ms) in-fence: %s #%llu (%s), error: %d\n",
		__func__, SC_FENCE_TIMEOUT,
		fence->ops->get_driver_name(fence), fence->seqno,
		dma_fence_is_signaled(fence) ? "signaled" : "active",
		fence->error);

	sc_buf->state = -ETIMEDOUT;

	dma_fence_put(fence);

	fence = sc_buf->out_sync_file->fence;
	if (fence)
		dev_err(ctx->sc_dev->dev,
			"%s: out-fence: #%llu\n", __func__, fence->seqno);

	__sc_vb2_buf_queue(ctx->m2m_ctx, &sc_buf->mb.vb);
}

static const char *sc_fence_get_driver_name(struct dma_fence *fence)
{
	return "mscl";
}

static bool sc_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static struct dma_fence_ops sc_fence_ops = {
	.get_driver_name	= sc_fence_get_driver_name,
	.get_timeline_name	= sc_fence_get_driver_name,
	.enable_signaling	= sc_fence_enable_signaling,
	.wait			= dma_fence_default_wait,
};

static int sc_setup_out_fence(struct sc_ctx *ctx, struct vb2_sc_buffer *sc_buf)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct dma_fence *fence;
	struct sync_file *sync_file;

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (!fence)
		return -ENOMEM;

	dma_fence_init(fence, &sc_fence_ops, &sc->fence_lock, sc->fence_context,
		       atomic_inc_return(&sc->fence_timeline));

	sync_file = sync_file_create(fence);
	dma_fence_put(fence);
	if (!sync_file)
		return -ENOMEM;

	sc_buf->out_sync_file = sync_file;

	return 0;
}

static void sc_remove_out_fence(struct vb2_sc_buffer *sc_buf)
{
	fput(sc_buf->out_sync_file->file);
	sc_buf->out_sync_file = NULL;
}

#define SC_V4L2_BUF_FLAG_IN_FENCE	0x00200000
#define SC_V4L2_BUF_FLAG_OUT_FENCE	0x00400000

#define SC_VOTF_REQUESTED(buf)		((buf)->m.planes[0].reserved[0] && \
					 !V4L2_TYPE_IS_OUTPUT((buf)->type))

static bool sc_check_votf_data(struct sc_dev *sc, unsigned int dpu_dma_idx,
			       unsigned int trs_idx, unsigned int buf_idx)
{
	if (dpu_dma_idx >= sc->votf_table_count)
		return false;

	if (trs_idx >= SC_NUM_TRS_IDX)
		return false;

	if (buf_idx >= SC_NUM_BUF_IDX)
		return false;

	return true;
}

static int sc_set_votf_data(struct sc_ctx *ctx, struct vb2_sc_buffer *sc_buf,
			    struct v4l2_buffer *buf)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_tws *tws;
	unsigned int dpu_dma_idx = buf->m.planes[0].reserved[1];
	unsigned int trs_idx = buf->m.planes[0].reserved[2];
	unsigned int buf_idx = buf->m.planes[0].reserved[3];
	unsigned long flags;

	sc_buf->tws = NULL;

	if (!SC_VOTF_REQUESTED(buf))
		return 0;

	if (ctx->num_int_frame != 0) {
		dev_err(sc->dev, "vOTF is not available with prescaling\n");
		return -EINVAL;
	}

	if (!sc->votf_regs) {
		dev_err(sc->dev, "vOTF is not supported\n");
		return -EINVAL;
	}

	if (!sc_check_votf_data(sc, dpu_dma_idx, trs_idx, buf_idx)) {
		dev_err(sc->dev,
			"vOTF data is invalid. dpu_dma(%u), trs(%u), buf(%u)\n",
			dpu_dma_idx, trs_idx, buf_idx);
		return -EINVAL;
	}

	spin_lock_irqsave(&sc->tws_lock, flags);

	if (list_empty(&sc->tws_avail_list)) {
		dev_err(sc->dev, "There is no tws available\n");
		spin_unlock_irqrestore(&sc->tws_lock, flags);
		return -ENODATA;
	}

	tws = list_first_entry(&sc->tws_avail_list, struct sc_tws, node);
	list_del_init(&tws->node);

	spin_unlock_irqrestore(&sc->tws_lock, flags);

	tws->sink.dpu_dma_idx = dpu_dma_idx;
	tws->sink.trs_idx = trs_idx;
	tws->sink.buf_idx = buf_idx;

	sc_buf->tws = tws;

	return 0;
}

static struct vb2_sc_buffer *sc_get_sc_buf_from_v4l2_buf(struct sc_ctx *ctx, struct v4l2_buffer *buf)
{
	struct vb2_queue *vq;
	struct vb2_buffer *vb;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, buf->type);
	if (!vq)
		return ERR_PTR(-EINVAL);

	if (buf->type != vq->type) {
		dev_err(ctx->sc_dev->dev,
			"buf->type(%x) is invalid\n", buf->type);
		return ERR_PTR(-EINVAL);
	}

	if (buf->index >= vq->num_buffers) {
		dev_err(ctx->sc_dev->dev,
			"%s: buf->index(%d) >= vq->num_buffers(%d)\n",
			__func__, buf->index, vq->num_buffers);
		return ERR_PTR(-EINVAL);
	}

	vb = vq->bufs[buf->index];
	if (vb == NULL) {
		dev_err(ctx->sc_dev->dev, "vb2_buffer is NULL\n");
		return ERR_PTR(-EFAULT);
	}

	if (buf->m.planes == NULL) {
		dev_err(ctx->sc_dev->dev, "the array of planes is invalid\n");
		return ERR_PTR(-EFAULT);
	}

	if (buf->length < vb->num_planes || buf->length > VB2_MAX_PLANES) {
		dev_err(ctx->sc_dev->dev,
			"%s: buf->length is expected %d, but got %d.\n",
			__func__, buf->length, vb->num_planes);
		return ERR_PTR(-EINVAL);
	}

	return sc_from_vb2_to_sc_buf(vb);
}

static s64 sc_calc_required_clk_of_ctx(struct sc_ctx *ctx, ktime_t cur_time)
{
	int src_clk, dst_clk;
	s64 i_frame_clk = 0, target_clk = 0;
	int ts_delta;
	int i;

	ts_delta = ktime_us_delta(ctx->ts_to_complete, cur_time);
	if (ts_delta <= 0)
		return -1;

	src_clk = sc_get_clock_khz(ctx, &ctx->s_frame, ts_delta);
	dst_clk = sc_get_clock_khz(ctx, &ctx->d_frame, ts_delta);

	i_frame_clk = 0;
	for (i = 0; i < ctx->num_int_frame; i++)
		i_frame_clk += sc_get_clock_khz(ctx, &ctx->i_frame[i]->frame, ts_delta);

	target_clk += max(src_clk, dst_clk);
	target_clk += i_frame_clk;

	return target_clk;
}

static void sc_calc_and_request_qos(struct sc_dev *sc, struct sc_ctx *ctx_queued)
{
	struct sc_ctx *ctx;
	ktime_t cur_time = ktime_get();
	s64 ret_clk, target_clk;
	int i;
	int qos_lv = -1;
	unsigned long flags;

	/* No need to calculate if no qos_table exists. */
	if (!sc->qos_table)
		return;

	if (ctx_queued == NULL) {
		dev_err(sc->dev, "%s: ctx_queued is NULL\n", __func__);
		return;
	}

	target_clk = 0;

	spin_lock_irqsave(&sc->ctxlist_qos_lock, flags);
	list_for_each_entry(ctx, &sc->ctxlist_qos, node_qos) {
		ret_clk = sc_calc_required_clk_of_ctx(ctx, cur_time);
		if (ret_clk < 0) {
			spin_unlock_irqrestore(&sc->ctxlist_qos_lock, flags);
			qos_lv = 0;
			goto complete_get_qos_lv;
		}

		target_clk += ret_clk;
	}

	spin_unlock_irqrestore(&sc->ctxlist_qos_lock, flags);

	for (i = sc->qos_table_cnt - 1; i > 0; i--) {
		if (sc->qos_table[i].freq_mscl >= target_clk)
			break;
	}
	qos_lv = i;

complete_get_qos_lv:
	mutex_lock(&ctx_queued->pm_qos_lock);
	sc_request_devfreq(ctx_queued, qos_lv);
	mod_delayed_work(system_wq, &ctx_queued->qos_work, msecs_to_jiffies(50));
	mutex_unlock(&ctx_queued->pm_qos_lock);
}

static int sc_v4l2_qbuf(struct file *file, void *fh,
			 struct v4l2_buffer *buf)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	struct vb2_sc_buffer *sc_buf;
	int out_fence_fd = -1;
	int ret;
	struct sc_dev *sc = ctx->sc_dev;
	unsigned long flags;

	sc_buf = sc_get_sc_buf_from_v4l2_buf(ctx, buf);
	if (IS_ERR(sc_buf))
		return PTR_ERR(sc_buf);

	if (buf->flags & SC_V4L2_BUF_FLAG_IN_FENCE) {
		sc_buf->in_fence = sync_file_get_fence(buf->reserved);
		if (!sc_buf->in_fence) {
			dev_err(ctx->sc_dev->dev,
				"%s: failed to get in_fence from fd(%d)\n",
				__func__, buf->reserved);
			return -EINVAL;
		}
	}
	sc_buf->state = 0;

	ret = sc_set_votf_data(ctx, sc_buf, buf);
	if (ret)
		return ret;

	if (buf->flags & SC_V4L2_BUF_FLAG_OUT_FENCE) {
		ret = sc_setup_out_fence(ctx, sc_buf);
		if (ret)
			goto err;

		ret = get_unused_fd_flags(O_CLOEXEC);
		if (ret < 0)
			goto err;
		out_fence_fd = ret;
	}

	ret = v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);
	if (ret)
		goto err;

	if (sc_buf->out_sync_file) {
		fd_install(out_fence_fd, get_file(sc_buf->out_sync_file->file));
		buf->reserved = out_fence_fd;
	}

	return ret;

err:
	spin_lock_irqsave(&sc->tws_lock, flags);
	if (sc_buf->tws) {
		list_add_tail(&sc_buf->tws->node, &sc->tws_avail_list);
		sc_buf->tws = NULL;
	}
	spin_unlock_irqrestore(&sc->tws_lock, flags);

	if (sc_buf->in_fence) {
		dma_fence_put(sc_buf->in_fence);
		sc_buf->in_fence = NULL;
	}
	if (sc_buf->out_sync_file)
		sc_remove_out_fence(sc_buf);
	if (out_fence_fd >= 0)
		put_unused_fd(out_fence_fd);

	return ret;
}

static int sc_v4l2_dqbuf(struct file *file, void *fh,
			  struct v4l2_buffer *buf)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	return v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);
}

static int sc_v4l2_streamon(struct file *file, void *fh,
			     enum v4l2_buf_type type)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	return v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
}

static int sc_v4l2_streamoff(struct file *file, void *fh,
			      enum v4l2_buf_type type)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	return v4l2_m2m_streamoff(file, ctx->m2m_ctx, type);
}

static int sc_get_fract_val(struct v4l2_rect *rect, struct sc_ctx *ctx)
{
	ctx->init_phase.yh = SC_CROP_GET_FR_VAL(rect->left);
	if (ctx->init_phase.yh)
		rect->left &= SC_CROP_INT_MASK;

	ctx->init_phase.yv = SC_CROP_GET_FR_VAL(rect->top);
	if (ctx->init_phase.yv)
		rect->top &= SC_CROP_INT_MASK;

	ctx->init_phase.w = SC_CROP_GET_FR_VAL(rect->width);
	if (ctx->init_phase.w) {
		rect->width &= SC_CROP_INT_MASK;
		rect->width += 1;
	}

	ctx->init_phase.h = SC_CROP_GET_FR_VAL(rect->height);
	if (ctx->init_phase.h) {
		rect->height &= SC_CROP_INT_MASK;
		rect->height += 1;
	}

	if (sc_fmt_is_yuv420(ctx->s_frame.sc_fmt->pixelformat)) {
		ctx->init_phase.ch = ctx->init_phase.yh / 2;
		ctx->init_phase.cv = ctx->init_phase.yv / 2;
	} else {
		ctx->init_phase.ch = ctx->init_phase.yh;
		ctx->init_phase.cv = ctx->init_phase.yv;
	}

	if ((ctx->init_phase.yh || ctx->init_phase.yv || ctx->init_phase.w
			|| ctx->init_phase.h) &&
		(!(sc_fmt_is_yuv420(ctx->s_frame.sc_fmt->pixelformat) ||
		sc_fmt_is_rgb888(ctx->s_frame.sc_fmt->pixelformat)))) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"%s format on real number is not supported",
			ctx->s_frame.sc_fmt->name);
		return -EINVAL;
	}

	v4l2_dbg(1, sc_log_level, &ctx->sc_dev->m2m.v4l2_dev,
				"src crop position (x,y,w,h) =	\
				(%d.%d, %d.%d, %d.%d, %d.%d) %d, %d\n",
				rect->left, ctx->init_phase.yh,
				rect->top, ctx->init_phase.yv,
				rect->width, ctx->init_phase.w,
				rect->height, ctx->init_phase.h,
				ctx->init_phase.ch, ctx->init_phase.cv);
	return 0;
}

static int sc_v4l2_s_selection(struct file *file, void *fh,
			       struct v4l2_selection *s)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_frame *frame;
	struct v4l2_rect rect = s->r;
	const struct sc_size_limit *limit = NULL;
	int w_align = 0;
	int h_align = 0;
	int ret = 0;

	frame = ctx_get_frame(ctx, s->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (!test_bit(CTX_PARAMS, &ctx->flags)) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"color format is not set\n");
		return -EINVAL;
	}

	if (s->r.left < 0 || s->r.top < 0) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"crop position is negative\n");
		return -EINVAL;
	}

	if (V4L2_TYPE_IS_OUTPUT(s->type)) {
		ret = sc_get_fract_val(&rect, ctx);
		if (ret < 0)
			return ret;
		limit = &sc->variant->limit_input;
		set_bit(CTX_SRC_FMT, &ctx->flags);
	} else {
		limit = &sc->variant->limit_output;
		set_bit(CTX_DST_FMT, &ctx->flags);
	}

	w_align = frame->sc_fmt->h_shift;
	h_align = frame->sc_fmt->v_shift;

	/* Bound an image to have crop width and height in limit */
	v4l_bound_align_image(&rect.width, limit->min_w, limit->max_w,
			w_align, &rect.height, limit->min_h,
			limit->max_h, h_align, 0);

	/* The crop position should be aligned in case of DST or YUYV format */
	if (!V4L2_TYPE_IS_OUTPUT(s->type) || (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_YUYV)) {
		/* Bound an image to have crop position in limit */
		v4l_bound_align_image(&rect.left, 0, frame->width - rect.width, w_align,
				      &rect.top, 0, frame->height - rect.height, h_align, 0);
	}

	if (!V4L2_TYPE_IS_OUTPUT(s->type) &&
			sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat))
		rect.width = ALIGN_DOWN(rect.width, 4);

	if ((rect.height > frame->height) || (rect.top > frame->height) ||
		(rect.width > frame->width) || (rect.left > frame->width)) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"Out of crop range: (%d,%d,%d,%d) from %dx%d\n",
			rect.left, rect.top, rect.width, rect.height,
			frame->width, frame->height);
		return -EINVAL;
	}

	frame->crop.top = rect.top;
	frame->crop.left = rect.left;
	frame->crop.height = rect.height;
	frame->crop.width = rect.width;

	return 0;
}

static uint32_t sc_bpp_to_v4l2format_for_ppc[][2] = {
	{ 12, V4L2_PIX_FMT_NV12M },
	{ 16, V4L2_PIX_FMT_NV16 },
	{ 24, V4L2_PIX_FMT_NV12M_P010 },
	{ 32, V4L2_PIX_FMT_RGB32 },
	{ 100, V4L2_PIX_FMT_NV12M_SBWC_8B },
};

#define MAX_ARG_PPC_IOCTL	10
struct sc_ioctl_ppc_arg {
	struct {
		uint32_t v4l2fmt;
		uint32_t ppc;
		uint32_t ppc_rot;
	} elem[MAX_ARG_PPC_IOCTL];
};

#define SC_CMD_G_PPC	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct sc_ioctl_ppc_arg)
#define SC_CMD_G_MAXCLK	_IOWR('V', BASE_VIDIOC_PRIVATE + 2, uint32_t)

static long sc_v4l2_default(struct file *file, void *fh, bool valid_prio,
			    unsigned int cmd, void *arg)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_ioctl_ppc_arg *ppc_arg = arg;
	int i, j;

	switch (cmd) {
	case SC_CMD_G_PPC:
		if (sc->ppc_table_cnt > MAX_ARG_PPC_IOCTL) {
			dev_err(sc->dev,
				"%s: elements of ppc_table is too many (num : %d)\n",
				__func__, sc->ppc_table_cnt);
			return -ENOMEM;
		}

		for (i = 0; i < sc->ppc_table_cnt; i++) {
			for (j = 0; j < ARRAY_SIZE(sc_bpp_to_v4l2format_for_ppc); j++) {
				if (sc_bpp_to_v4l2format_for_ppc[j][0] == sc->ppc_table[i].bpp) {
					ppc_arg->elem[i].v4l2fmt = sc_bpp_to_v4l2format_for_ppc[j][1];
					ppc_arg->elem[i].ppc = sc->ppc_table[i].ppc[0];
					ppc_arg->elem[i].ppc_rot = sc->ppc_table[i].ppc[1];

					break;
				}
			}
		}
		break;
	case SC_CMD_G_MAXCLK:
		if (!sc->qos_table)
			return -EINVAL;

		*((uint32_t*)arg) = sc->qos_table[0].freq_mscl;

		break;
	default:
		dev_err(sc->dev, "%s : invalid command %u\n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ioctl_ops sc_v4l2_ioctl_ops = {
	.vidioc_querycap		= sc_v4l2_querycap,

	.vidioc_g_fmt_vid_cap_mplane	= sc_v4l2_g_fmt_mplane,
	.vidioc_g_fmt_vid_out_mplane	= sc_v4l2_g_fmt_mplane,

	.vidioc_try_fmt_vid_cap_mplane	= sc_v4l2_try_fmt_mplane,
	.vidioc_try_fmt_vid_out_mplane	= sc_v4l2_try_fmt_mplane,

	.vidioc_s_fmt_vid_cap_mplane	= sc_v4l2_s_fmt_mplane,
	.vidioc_s_fmt_vid_out_mplane	= sc_v4l2_s_fmt_mplane,

	.vidioc_reqbufs			= sc_v4l2_reqbufs,

	.vidioc_qbuf			= sc_v4l2_qbuf,
	.vidioc_dqbuf			= sc_v4l2_dqbuf,

	.vidioc_streamon		= sc_v4l2_streamon,
	.vidioc_streamoff		= sc_v4l2_streamoff,

	.vidioc_s_selection		= sc_v4l2_s_selection,

	.vidioc_default			= sc_v4l2_default,
};

static int sc_ctx_stop_req(struct sc_ctx *ctx)
{
	struct sc_ctx *curr_ctx;
	struct sc_dev *sc = ctx->sc_dev;
	int ret = 0;

	curr_ctx = v4l2_m2m_get_curr_priv(sc->m2m.m2m_dev);
	if (!test_bit(CTX_RUN, &ctx->flags) || (curr_ctx != ctx))
		return 0;

	ret = wait_event_timeout(sc->wait,
			!test_bit(CTX_RUN, &ctx->flags), SC_TIMEOUT);

	/* TODO: How to handle case of timeout event */
	if (ret == 0) {
		dev_err(sc->dev, "device failed to stop request\n");
		ret = -EBUSY;
	}

	return ret;
}

static void free_intermediate_frame(struct sc_int_frame *i_frame)
{
	int i;

	if (i_frame == NULL)
		return;

	if (!i_frame->dma_buf[0])
		return;

	for(i = 0; i < 3; i++) {
		if (i_frame->dma_buf[i]) {
			dma_buf_unmap_attachment_unlocked(i_frame->attachment[i],
					i_frame->sgt[i], DMA_BIDIRECTIONAL);
			dma_buf_detach(i_frame->dma_buf[i], i_frame->attachment[i]);
			dma_heap_buffer_free(i_frame->dma_buf[i]);
		}
	}

	memset(&i_frame->dma_buf, 0, sizeof(struct dma_buf *) * 3);
	memset(&i_frame->frame.addr, 0, sizeof(struct sc_addr));
}

static void destroy_intermediate_frame(struct sc_ctx *ctx, unsigned int idx)
{
	if (ctx->i_frame[idx]) {
		free_intermediate_frame(ctx->i_frame[idx]);
		dma_heap_put(ctx->i_frame[idx]->dma_heap);
		kfree(ctx->i_frame[idx]);
		ctx->i_frame[idx] = NULL;
	}

	ctx->num_int_frame--;
}

static void destroy_intermediate_frame_all(struct sc_ctx *ctx)
{
	int i;

	for (i = ctx->num_int_frame - 1; i >= 0; i--) {
		destroy_intermediate_frame(ctx, i);
	}

	ctx->num_int_frame = 0;
}

static bool alloc_intermediate_buffer(struct device *dev,
				      struct sc_int_frame *iframe, int i, size_t size)
{
	iframe->dma_buf[i] = dma_heap_buffer_alloc(iframe->dma_heap, size, 0, 0);
	if (IS_ERR(iframe->dma_buf[i])) {
		dev_err(dev,
			"Failed to allocate intermediate buffer.%d (err %ld)",
			i, PTR_ERR(iframe->dma_buf[i]));
		goto err_dmabuf;
	}

	iframe->attachment[i] = dma_buf_attach(iframe->dma_buf[i], dev);
	if (IS_ERR(iframe->attachment[i])) {
		dev_err(dev,
			"Failed to attach from dma_buf of buffer.%d (err %ld)",
			i, PTR_ERR(iframe->attachment[i]));
		goto err_attach;
	}

	iframe->sgt[i] = dma_buf_map_attachment_unlocked(iframe->attachment[i],
						DMA_BIDIRECTIONAL);
	if (IS_ERR(iframe->sgt[i])) {
		dev_err(dev, "Failed to get sgt from buffer.%d (err %ld)",
			i, PTR_ERR(iframe->sgt[i]));
		goto err_map;
	}

	iframe->frame.addr.ioaddr[i] = sg_dma_address(iframe->sgt[i]->sgl);
	if (IS_ERR_VALUE(iframe->frame.addr.ioaddr[i])) {
		dev_err(dev,
			"Failed to allocate iova of buffer.%d (err %pa)",
			i, &iframe->frame.addr.ioaddr[i]);
		goto err_iaddr_alloc;
	}

	return true;

err_iaddr_alloc:
	iframe->frame.addr.ioaddr[i] = 0;
	dma_buf_unmap_attachment_unlocked(iframe->attachment[i], iframe->sgt[i],
				 DMA_BIDIRECTIONAL);
err_map:
	iframe->sgt[i] = NULL;
	dma_buf_detach(iframe->dma_buf[i], iframe->attachment[i]);
err_attach:
	iframe->attachment[i] = NULL;
	dma_heap_buffer_free(iframe->dma_buf[i]);
err_dmabuf:
	iframe->dma_buf[i] = NULL;
	return false;
}

static bool initialize_intermediate_frame(struct sc_ctx *ctx,
					  struct sc_int_frame *i_frame)
{
	struct sc_frame *frame;
	struct sc_dev *sc = ctx->sc_dev;
	char *heap_name;
	int i;

	frame = &i_frame->frame;

	frame->crop.top = 0;
	frame->crop.left = 0;
	if (!frame->width)
		frame->width = frame->crop.width;
	if (!frame->height)
		frame->height = frame->crop.height;

	sc_get_span(frame->sc_fmt, frame->width,
		    &frame->stride[SC_PLANE_Y], &frame->stride[SC_PLANE_CB],
		    frame->byte32num);
	/*
	 * Check if intermeidate frame is already initialized by a previous
	 * frame. If it is already initialized, intermediate buffer is no longer
	 * needed to be initialized because image setting is never changed
	 * while streaming continues.
	 */
	if (i_frame->dma_buf[0])
		return true;

	if (ctx->cp_enabled)
		heap_name = "vscaler-secure";
	else
		heap_name = "system-uncached";

	i_frame->dma_heap = dma_heap_find(heap_name);
	if (!i_frame->dma_heap) {
		dev_err(sc->dev,
			"%s:failed to get dma_heap(%s)\n", __func__, heap_name);
			return false;
	}

	sc_calc_bufsize(sc, &i_frame->frame);

	for (i = 0; i < SC_MAX_PLANES; i++) {
		if (!frame->addr.size[i])
			break;

		if (!alloc_intermediate_buffer(sc->dev, i_frame, i,
					       frame->addr.size[i]))
			goto err_ion_alloc;
	}

	return true;

err_ion_alloc:
	free_intermediate_frame(i_frame);
	dma_heap_put(i_frame->dma_heap);
	return false;
}

static bool allocate_intermediate_frame(struct sc_ctx *ctx, unsigned int idx)
{
	if (idx >= MAX_INT_FRAME) {
		dev_err(ctx->sc_dev->dev,
			"Invalid request to allocate i_frame[%d] (MAX: %d)\n",
			idx, MAX_INT_FRAME);
		return false;
	}

	if (idx > ctx->num_int_frame) {
		dev_err(ctx->sc_dev->dev,
			"Request to allocate i_frame should be sequentially.\
			(idx: %d, but ctx->num_int_frame: %d)\n",
			idx, ctx->num_int_frame);
		return false;
	}

	if (ctx->i_frame[idx] == NULL) {
		ctx->i_frame[idx] = kzalloc(sizeof(*ctx->i_frame[idx]), GFP_KERNEL);
		if (ctx->i_frame[idx] == NULL) {
			dev_err(ctx->sc_dev->dev,
				"Failed to allocate intermediate frame\n");
			return false;
		}

		ctx->num_int_frame++;
	}

	return true;
}

/* Zoom-out range: (x1/4, x1/16] */
static int sc_prepare_multi_scaling(struct sc_ctx *ctx,
				  __s32 src_width_in, __s32 src_height_in,
				  unsigned int *h_ratio_in, unsigned int *v_ratio_in)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct v4l2_rect crop = ctx->d_frame.crop;
	const struct sc_size_limit *limit;
	unsigned int halign = 0, walign = 0;
	unsigned short width = 0, height = 0;
	const struct sc_fmt *target_fmt = ctx->d_frame.sc_fmt;
	unsigned int sc_down_min = sc->variant->sc_down_min;
	unsigned int sc_up_max = sc->variant->sc_up_max;

	unsigned int idx = 0;
	unsigned int h_ratio = *h_ratio_in, v_ratio = *v_ratio_in;
	__s32 src_width = src_width_in, src_height = src_height_in;

	int num_int_frame;

	for (; idx < MAX_INT_FRAME; idx++) {
		if (!allocate_intermediate_frame(ctx, idx))
			return -ENOMEM;

		limit = &sc->variant->limit_input;
		if (v_ratio > SCALE_RATIO_CONST(4, 1)) {
			crop.height = ((src_height + 7) / 8) * 2;
			if (crop.height < limit->min_h)
				crop.height = limit->min_h;
		}

		if (h_ratio > SCALE_RATIO_CONST(4, 1)) {
			crop.width = ((src_width + 7) / 8) * 2;
			if (crop.width < limit->min_w)
				crop.width = limit->min_w;
		}

		if (v_ratio < SCALE_RATIO_CONST(1, 8)) {
			crop.height = src_height * 8;
			if (crop.height > limit->max_h)
				crop.height = limit->max_h;
		}

		if (h_ratio < SCALE_RATIO_CONST(1, 8)) {
			crop.width = src_width * 8;
			if (crop.width > limit->max_w)
				crop.width = limit->max_w;
		}

		walign = target_fmt->h_shift;
		halign = target_fmt->v_shift;

		limit = &sc->variant->limit_output;
		v4l_bound_align_image(&crop.width, limit->min_w, limit->max_w,
				      walign, &crop.height, limit->min_h,
				      limit->max_h, halign, 0);

		/* align up for scale down, align down for scale up */
		if (sc_fmt_is_sbwc(ctx->d_frame.sc_fmt->pixelformat)) {
			if (!IS_ALIGNED(crop.width, 32))
				width = ALIGN(crop.width, 32);
			if (!IS_ALIGNED(crop.height, 4))
				height = ALIGN(crop.height, 4);
		}

		if (sc_fmt_is_s10bit_yuv(ctx->d_frame.sc_fmt->pixelformat))
			crop.width = ALIGN(crop.width, 4);

		if ((ctx->i_frame[idx]->frame.sc_fmt != ctx->d_frame.sc_fmt) ||
		    memcmp(&crop, &ctx->i_frame[idx]->frame.crop, sizeof(crop)) ||
		    (ctx->cp_enabled != test_bit(CTX_INT_FRAME_CP, &ctx->flags))) {
			free_intermediate_frame(ctx->i_frame[idx]);

			memcpy(&ctx->i_frame[idx]->frame, &ctx->d_frame,
			       sizeof(ctx->d_frame));
			memcpy(&ctx->i_frame[idx]->frame.crop, &crop, sizeof(crop));

			ctx->i_frame[idx]->frame.width = width;
			ctx->i_frame[idx]->frame.height = height;

			if (!initialize_intermediate_frame(ctx, ctx->i_frame[idx])) {
				destroy_intermediate_frame_all(ctx);
				return -ENOMEM;
			}
		}

		h_ratio = SCALE_RATIO(crop.width, ctx->d_frame.crop.width);
		v_ratio = SCALE_RATIO(crop.height, ctx->d_frame.crop.height);

		if ((h_ratio <= sc_down_min) && (v_ratio <= sc_down_min)
			&& (h_ratio >= sc_up_max) && (v_ratio >= sc_up_max)) {
			break;
		}

		src_width = crop.width;
		src_height = crop.height;
	}

	/*
	 * In destroy_intermediate_frame(), ctx->num_int_frame will be decreased.
	 * So, save the value to destroy unnecessary i_frame.
	 */
	num_int_frame = ctx->num_int_frame;
	for (++idx; idx < num_int_frame; idx++)
		destroy_intermediate_frame(ctx, idx);

	*h_ratio_in = SCALE_RATIO(src_width_in, ctx->i_frame[0]->frame.crop.width);
	*v_ratio_in = SCALE_RATIO(src_height_in, ctx->i_frame[0]->frame.crop.height);

	if (ctx->cp_enabled)
		set_bit(CTX_INT_FRAME_CP, &ctx->flags);
	else
		clear_bit(CTX_INT_FRAME_CP, &ctx->flags);

	return 0;
}

static struct sc_dnoise_filter sc_filter_tab[4] = {
	{SC_FT_240,   426,  240},
	{SC_FT_480,   854,  480},
	{SC_FT_720,  1280,  720},
	{SC_FT_1080, 1920, 1080},
};

static int sc_find_filter_size(struct sc_ctx *ctx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sc_filter_tab); i++) {
		if (sc_filter_tab[i].strength == ctx->dnoise_ft.strength) {
			if (ctx->s_frame.width >= ctx->s_frame.height) {
				ctx->dnoise_ft.w = sc_filter_tab[i].w;
				ctx->dnoise_ft.h = sc_filter_tab[i].h;
			} else {
				ctx->dnoise_ft.w = sc_filter_tab[i].h;
				ctx->dnoise_ft.h = sc_filter_tab[i].w;
			}
			break;
		}
	}

	if (i == ARRAY_SIZE(sc_filter_tab)) {
		dev_err(ctx->sc_dev->dev,
			"%s: can't find filter size\n", __func__);
		return -EINVAL;
	}

	if (ctx->s_frame.crop.width < ctx->dnoise_ft.w ||
			ctx->s_frame.crop.height < ctx->dnoise_ft.h) {
		dev_err(ctx->sc_dev->dev,
			"%s: filter is over source size.(%dx%d -> %dx%d)\n",
			__func__, ctx->s_frame.crop.width,
			ctx->s_frame.crop.height, ctx->dnoise_ft.w,
			ctx->dnoise_ft.h);
		return -EINVAL;
	}
	return 0;
}

static int sc_prepare_denoise_filter(struct sc_ctx *ctx)
{
	unsigned int sc_down_min = ctx->sc_dev->variant->sc_down_min;

	if (ctx->dnoise_ft.strength <= SC_FT_BLUR)
		return 0;

	if (sc_find_filter_size(ctx))
		return -EINVAL;

	if (!allocate_intermediate_frame(ctx, 0))
		return -ENOMEM;

	memcpy(&ctx->i_frame[0]->frame, &ctx->d_frame, sizeof(ctx->d_frame));
	ctx->i_frame[0]->frame.crop.width = ctx->dnoise_ft.w;
	ctx->i_frame[0]->frame.crop.height = ctx->dnoise_ft.h;
	ctx->i_frame[0]->frame.width = 0;
	ctx->i_frame[0]->frame.height = 0;

	free_intermediate_frame(ctx->i_frame[0]);
	if (!initialize_intermediate_frame(ctx, ctx->i_frame[0])) {
		free_intermediate_frame(ctx->i_frame[0]);
		dev_err(ctx->sc_dev->dev,
			"%s: failed to initialize int_frame\n", __func__);
		return -ENOMEM;
	}

	ctx->h_ratio = SCALE_RATIO(ctx->s_frame.crop.width, ctx->dnoise_ft.w);
	ctx->v_ratio = SCALE_RATIO(ctx->s_frame.crop.height, ctx->dnoise_ft.h);

	if ((ctx->h_ratio > sc_down_min) ||
			(ctx->h_ratio < ctx->sc_dev->variant->sc_up_max)) {
		dev_err(ctx->sc_dev->dev,
			"filter can't support width scaling(%d -> %d)\n",
			ctx->s_frame.crop.width, ctx->dnoise_ft.w);
		goto err_ft;
	}

	if ((ctx->v_ratio > sc_down_min) ||
			(ctx->v_ratio < ctx->sc_dev->variant->sc_up_max)) {
		dev_err(ctx->sc_dev->dev,
			"filter can't support height scaling(%d -> %d)\n",
			ctx->s_frame.crop.height, ctx->dnoise_ft.h);
		goto err_ft;
	}

#if IS_ENABLED(CONFIG_VIDEO_EXYNOS_SCALER_PRESCALE)
	if (ctx->sc_dev->variant->prescale) {
		BUG_ON(sc_down_min != SCALE_RATIO_CONST(16, 1));

		if (ctx->h_ratio > SCALE_RATIO_CONST(8, 1))
			ctx->pre_h_ratio = 2;
		else if (ctx->h_ratio > SCALE_RATIO_CONST(4, 1))
			ctx->pre_h_ratio = 1;
		else
			ctx->pre_h_ratio = 0;

		if (ctx->v_ratio > SCALE_RATIO_CONST(8, 1))
			ctx->pre_v_ratio = 2;
		else if (ctx->v_ratio > SCALE_RATIO_CONST(4, 1))
			ctx->pre_v_ratio = 1;
		else
			ctx->pre_v_ratio = 0;

		if (ctx->pre_h_ratio || ctx->pre_v_ratio) {
			if (!IS_ALIGNED(ctx->s_frame.crop.width,
					1 << (ctx->pre_h_ratio +
					ctx->s_frame.sc_fmt->h_shift))) {
				dev_err(ctx->sc_dev->dev,
					"filter can't support not-aligned source(%d -> %d)\n",
					ctx->s_frame.crop.width, ctx->dnoise_ft.w);
				goto err_ft;
			} else if (!IS_ALIGNED(ctx->s_frame.crop.height,
					1 << (ctx->pre_v_ratio +
					ctx->s_frame.sc_fmt->v_shift))) {
				dev_err(ctx->sc_dev->dev,
					"filter can't support not-aligned source(%d -> %d)\n",
					ctx->s_frame.crop.height, ctx->dnoise_ft.h);
				goto err_ft;
			} else {
				ctx->h_ratio >>= ctx->pre_h_ratio;
				ctx->v_ratio >>= ctx->pre_v_ratio;
			}
		}
	}
#endif

	return 0;

err_ft:
	free_intermediate_frame(ctx->i_frame[0]);
	return -EINVAL;
}

static int sc_find_scaling_ratio(struct sc_ctx *ctx)
{
	__s32 src_width, src_height;
	unsigned int h_ratio, v_ratio;
	struct sc_dev *sc = ctx->sc_dev;
	unsigned int sc_down_min = sc->variant->sc_down_min;
	unsigned int sc_up_max = sc->variant->sc_up_max;

	if ((ctx->s_frame.crop.width == 0) ||
			(ctx->d_frame.crop.width == 0))
		return 0; /* s_fmt is not complete */

	src_width = ctx->s_frame.crop.width;
	src_height = ctx->s_frame.crop.height;
	if (!!(ctx->flip_rot_cfg & SCALER_ROT_90))
		swap(src_width, src_height);

	h_ratio = SCALE_RATIO(src_width, ctx->d_frame.crop.width);
	v_ratio = SCALE_RATIO(src_height, ctx->d_frame.crop.height);

	/*
	 * If the source crop width or height is fractional value
	 * calculate scaling ratio including it and calculate with original
	 * crop.width and crop.height value because they were rounded up.
	 */
	if (ctx->init_phase.w)
		h_ratio = SCALE_RATIO_FRACT((src_width - 1), ctx->init_phase.w,
				ctx->d_frame.crop.width);
	if (ctx->init_phase.h)
		v_ratio = SCALE_RATIO_FRACT((src_height - 1), ctx->init_phase.h,
				ctx->d_frame.crop.height);
	sc_dbg("Scaling ratio h_ratio %d, v_ratio %d\n", h_ratio, v_ratio);

#if IS_ENABLED(CONFIG_VIDEO_EXYNOS_SCALER_PRESCALE)
	if (sc->variant->prescale) {
		BUG_ON(sc_down_min != SCALE_RATIO_CONST(16, 1));

		if (h_ratio > SCALE_RATIO_CONST(8, 1)) {
			ctx->pre_h_ratio = 2;
		} else if (h_ratio > SCALE_RATIO_CONST(4, 1)) {
			ctx->pre_h_ratio = 1;
		} else {
			ctx->pre_h_ratio = 0;
		}

		if (v_ratio > SCALE_RATIO_CONST(8, 1)) {
			ctx->pre_v_ratio = 2;
		} else if (v_ratio > SCALE_RATIO_CONST(4, 1)) {
			ctx->pre_v_ratio = 1;
		} else {
			ctx->pre_v_ratio = 0;
		}

		/*
		 * If the source image resolution violates the constraints of
		 * pre-scaler, then performs poly-phase scaling twice
		 */
		if (ctx->pre_h_ratio || ctx->pre_v_ratio) {
			if (!IS_ALIGNED(src_width, 1 << (ctx->pre_h_ratio +
					ctx->s_frame.sc_fmt->h_shift)) ||
				!IS_ALIGNED(src_height, 1 << (ctx->pre_v_ratio +
					ctx->s_frame.sc_fmt->v_shift))) {
				sc_down_min = SCALE_RATIO_CONST(4, 1);
				ctx->pre_h_ratio = 0;
				ctx->pre_v_ratio = 0;
			} else {
				h_ratio >>= ctx->pre_h_ratio;
				v_ratio >>= ctx->pre_v_ratio;
			}
		}

		if (sc_down_min == SCALE_RATIO_CONST(4, 1)) {
			dev_info(sc->dev,
			"%s: Prepared 2nd polyphase scaler (%dx%d->%dx%d)\n",
			__func__,
			ctx->s_frame.crop.width, ctx->s_frame.crop.height,
			ctx->d_frame.crop.width, ctx->d_frame.crop.height);
		}
	}
#endif

	if ((h_ratio > sc_down_min) || (v_ratio > sc_down_min)
			|| (h_ratio < sc_up_max) || (v_ratio < sc_up_max)) {
		int ret;

		ret = sc_prepare_multi_scaling(ctx, src_width, src_height,
						&h_ratio, &v_ratio);
		if (ret)
			return ret;
	} else {
		destroy_intermediate_frame_all(ctx);
	}

	ctx->h_ratio = h_ratio;
	ctx->v_ratio = v_ratio;

	return 0;
}

#define sc_fmt_is_y_mono(x)	(((x) == V4L2_PIX_FMT_GREY) || \
				 ((x) == V4L2_PIX_FMT_Y10))
static bool sc_check_csc_valid(struct sc_ctx *ctx)
{
	if (sc_fmt_is_y_mono(ctx->s_frame.sc_fmt->pixelformat) !=
	    sc_fmt_is_y_mono(ctx->d_frame.sc_fmt->pixelformat))
		return false;

	return true;
}

static int sc_vb2_queue_setup(struct vb2_queue *vq,
		unsigned int *num_buffers, unsigned int *num_planes,
		unsigned int sizes[], struct device *alloc_devs[])
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	struct sc_frame *frame;
	int ret;
	int i;

	frame = ctx_get_frame(ctx, vq->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (!sc_check_csc_valid(ctx)) {
		dev_err(ctx->sc_dev->dev, "CSC(%s -> %s) is not available",
			ctx->s_frame.sc_fmt->name, ctx->d_frame.sc_fmt->name);
		return -EINVAL;
	}

	/* Get number of planes from format_list in driver */
	*num_planes = frame->sc_fmt->num_planes;
	for (i = 0; i < frame->sc_fmt->num_planes; i++) {
		sizes[i] = frame->bytesused[i];
		alloc_devs[i] = ctx->sc_dev->dev;
	}

	ret = sc_find_scaling_ratio(ctx);
	if (ret)
		return ret;

	ret = sc_prepare_denoise_filter(ctx);
	if (ret)
		return ret;

	return 0;
}

/*
 * This function should be used for source buffer only.
 * In case of destination buffer, frame->bytesused[] is not valid.
 */
static int sc_check_src_plane_size(struct sc_ctx *ctx, struct vb2_buffer *vb,
					 struct sc_frame *frame, u32 min_size)
{
	struct sg_table *sgt;
	int i, ret = 0;

	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type) || !min_size)
		return ret;

	for (i = 0; i < frame->sc_fmt->num_planes; i++) {
		if (frame->bytesused[i] < min_size) {
			sgt = (struct sg_table *)vb2_plane_cookie(vb, i);
			if (!sgt) {
				v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
					"invalid sgt in plane %d\n", i);
				return -EINVAL;
			}

			if (sg_nents_for_len(sgt->sgl, (u64)min_size) <= 0) {
				ret = -EINVAL;
				break;
			}
		}
	}

	if (ret)
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"plane%d size %d is smaller than %d\n",
			i, frame->bytesused[i], min_size);

	return ret;
}

static int sc_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct sc_frame *frame;
	u32 min_size = ctx->sc_dev->variant->minsize_srcplane;
	int i;

	frame = ctx_get_frame(ctx, vb->vb2_queue->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		/* Plane size checking is needed for source buffer */
		if (sc_check_src_plane_size(ctx, vb, frame, min_size))
			return -EINVAL;
	} else {
		for (i = 0; i < frame->sc_fmt->num_planes; i++)
			vb2_set_plane_payload(vb, i, frame->bytesused[i]);
	}

	return 0;
}

static void sc_vb2_buf_finish(struct vb2_buffer *vb)
{
	struct vb2_sc_buffer *sc_buf = sc_from_vb2_to_sc_buf(vb);
	struct dma_fence *fence;
	struct sc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long flags;

	do {
		fence = sc_buf->in_fence;
	} while (cmpxchg(&sc_buf->in_fence, fence, NULL) != fence);

	if (fence) {
		del_timer(&sc_buf->fence_timer);
		dma_fence_remove_callback(fence, &sc_buf->fence_cb);
		dma_fence_put(fence);
	} else if (work_busy(&sc_buf->qbuf_work)) {
		cancel_work_sync(&sc_buf->qbuf_work);
	}

	/*
	 * node_qos should be deleted before calling buf_finish().
	 * But, in case of calling __vb2_queue_cancel(),
	 * the buffer will be aborted between QUEUED and RUNNING.
	 * So, node_qos can be not deleted and it is handling this.
	 */
	spin_lock_irqsave(&ctx->sc_dev->ctxlist_qos_lock, flags);
	if (!list_empty(&ctx->node_qos))
		list_del_init(&ctx->node_qos);
	spin_unlock_irqrestore(&ctx->sc_dev->ctxlist_qos_lock, flags);

	if (sc_buf->out_sync_file)
		sc_remove_out_fence(sc_buf);
}

static void __sc_vb2_buf_queue(struct v4l2_m2m_ctx *m2m_ctx,
			       struct vb2_v4l2_buffer *v4l2_buf)
{
	struct vb2_v4l2_buffer *src, *dst;
	struct sc_ctx *ctx = m2m_ctx->priv;
	unsigned long flags;

	v4l2_m2m_buf_queue(m2m_ctx, v4l2_buf);

	src = v4l2_m2m_next_src_buf(m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(m2m_ctx);

	if (src && dst) {
		struct sc_ctx *tmp_ctx;
		bool skip_request_qos = false;
		struct vb2_buffer *src_vb, *dst_vb;
		struct vb2_sc_buffer *src_sc_buf, *dst_sc_buf;

		src_vb = &(src->vb2_buf);
		dst_vb = &(dst->vb2_buf);
		src_sc_buf = sc_from_vb2_to_sc_buf(src_vb);
		dst_sc_buf = sc_from_vb2_to_sc_buf(dst_vb);

		if (!src_sc_buf->state && !dst_sc_buf->state) {
			spin_lock_irqsave(&ctx->sc_dev->ctxlist_qos_lock, flags);

			if (list_empty(&ctx->node_qos)) {
				ctx->ts_to_complete = ktime_add_us(ktime_get(),
						USEC_PER_SEC / ((ctx->framerate != 0) ? ctx->framerate
								      : SC_FRAMERATE_DEFAULT));
				/*
				 * The jobs queued before this should be finished before this.
				 * So, change the timestamp as this.
				 */
				list_for_each_entry(tmp_ctx, &ctx->sc_dev->ctxlist_qos, node_qos) {
					if (tmp_ctx->ts_to_complete > ctx->ts_to_complete)
						tmp_ctx->ts_to_complete = ctx->ts_to_complete;
				}
				list_add(&ctx->node_qos, &ctx->sc_dev->ctxlist_qos);
			} else {
				skip_request_qos = true;
			}

			spin_unlock_irqrestore(&ctx->sc_dev->ctxlist_qos_lock, flags);

			if (!skip_request_qos)
				sc_calc_and_request_qos(ctx->sc_dev, ctx);
		}
	}

	v4l2_m2m_try_schedule(m2m_ctx);
}

static void sc_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);
	struct vb2_sc_buffer *sc_buf = sc_from_vb2_to_sc_buf(vb);
	int ret;

	if (sc_buf->tws) {
		ret = exynos_dpuf_set_votf(sc_buf->tws->sink.dpu_dma_idx, true);
		if (ret < 0) {
			dev_err(ctx->sc_dev->dev,
				"fail to enable sink device of vOTF(err:%d)\n", ret);

			sc_buf->state = ret;
		}
	}

	if (sc_buf->in_fence) {
		timer_setup(&sc_buf->fence_timer, sc_fence_timeout_handler, 0);
		mod_timer(&sc_buf->fence_timer,
			  jiffies + msecs_to_jiffies(SC_FENCE_TIMEOUT));

		ret = dma_fence_add_callback(sc_buf->in_fence,
					     &sc_buf->fence_cb, sc_fence_cb);
		if (ret) {
			struct dma_fence *fence;

			do {
				fence = sc_buf->in_fence;
			} while (cmpxchg(&sc_buf->in_fence, fence, NULL) != fence);

			if (!fence)
				return;

			del_timer(&sc_buf->fence_timer);
			dma_fence_put(fence);

			if (ret != -ENOENT) {
				dev_err(ctx->sc_dev->dev,
					"%s: failed to add fence_cb[err:%d]\n",
					__func__, ret);
				sc_buf->state = ret;
			}
		} else {
			return;
		}
	}

	__sc_vb2_buf_queue(ctx->m2m_ctx, v4l2_buf);
}

static void sc_vb2_buf_cleanup(struct vb2_buffer *vb)
{
}

static void sc_vb2_lock(struct vb2_queue *vq)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	mutex_lock(&ctx->sc_dev->lock);
}

static void sc_vb2_unlock(struct vb2_queue *vq)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	mutex_unlock(&ctx->sc_dev->lock);
}

static int sc_vb2_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	set_bit(CTX_STREAMING, &ctx->flags);

	return 0;
}

static void sc_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	int ret;

	ret = sc_ctx_stop_req(ctx);
	if (ret < 0)
		dev_err(ctx->sc_dev->dev, "wait timeout\n");

	clear_bit(CTX_STREAMING, &ctx->flags);

	if (atomic_read(&vq->owned_by_drv_count)) {
		unsigned int i;

		for (i = 0; i < vq->num_buffers; ++i) {
			if (vq->bufs[i]->state == VB2_BUF_STATE_ACTIVE) {
				dev_warn(ctx->sc_dev->dev,
					 "%s:vq->bufs[%d] is in active state\n",
					 V4L2_TYPE_IS_OUTPUT(vq->type) ? "SRC"
								       : "DST",
					 i);
				vb2_buffer_done(vq->bufs[i], VB2_BUF_STATE_ERROR);
			}
		}
		/* Must be zero now */
		WARN_ON(atomic_read(&vq->owned_by_drv_count));
	}
}

static int sc_vb2_buf_init(struct vb2_buffer *vb)
{
	struct vb2_sc_buffer *sc_buf = sc_from_vb2_to_sc_buf(vb);

	INIT_WORK(&sc_buf->qbuf_work, __sc_qbuf_work);
	return 0;
}

static struct vb2_ops sc_vb2_ops = {
	.queue_setup		= sc_vb2_queue_setup,
	.buf_init		= sc_vb2_buf_init,
	.buf_prepare		= sc_vb2_buf_prepare,
	.buf_finish		= sc_vb2_buf_finish,
	.buf_queue		= sc_vb2_buf_queue,
	.buf_cleanup		= sc_vb2_buf_cleanup,
	.wait_finish		= sc_vb2_lock,
	.wait_prepare		= sc_vb2_unlock,
	.start_streaming	= sc_vb2_start_streaming,
	.stop_streaming		= sc_vb2_stop_streaming,
};

static int queue_init(void *priv, struct vb2_queue *src_vq,
		      struct vb2_queue *dst_vq)
{
	struct sc_ctx *ctx = priv;
	int ret;

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->ops = &sc_vb2_ops;
	src_vq->mem_ops = &vb2_dma_sg_memops;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct vb2_sc_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	dst_vq->ops = &sc_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_sg_memops;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct vb2_sc_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	return vb2_queue_init(dst_vq);
}

static bool sc_configure_rotation_degree(struct sc_ctx *ctx, int degree)
{
	ctx->flip_rot_cfg &= ~SCALER_ROT_MASK;

	/*
	 * we expect that the direction of rotation is clockwise
	 * but the Scaler does in counter clockwise.
	 * Since the GScaler doest that in clockwise,
	 * the following makes the direction of rotation by the Scaler
	 * clockwise.
	 */
	if (degree == 270) {
		ctx->flip_rot_cfg |= SCALER_ROT_90;
	} else if (degree == 180) {
		ctx->flip_rot_cfg |= SCALER_ROT_180;
	} else if (degree == 90) {
		ctx->flip_rot_cfg |= SCALER_ROT_270;
	} else if (degree != 0) {
		dev_err(ctx->sc_dev->dev,
			"%s: Rotation of %d is not supported\n",
			__func__, degree);
		return false;
	}

	return true;
}

static int sc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sc_ctx *ctx;
	int ret = 0;

	sc_dbg("ctrl ID:%d, value:%d\n", ctrl->id, ctrl->val);
	ctx = container_of(ctrl->handler, struct sc_ctx, ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		if (ctrl->val)
			ctx->flip_rot_cfg |= SCALER_FLIP_X_EN;
		else
			ctx->flip_rot_cfg &= ~SCALER_FLIP_X_EN;
		break;
	case V4L2_CID_HFLIP:
		if (ctrl->val)
			ctx->flip_rot_cfg |= SCALER_FLIP_Y_EN;
		else
			ctx->flip_rot_cfg &= ~SCALER_FLIP_Y_EN;
		break;
	case V4L2_CID_ROTATE:
		if (!sc_configure_rotation_degree(ctx, ctrl->val))
			return -EINVAL;
		break;
	case V4L2_CID_GLOBAL_ALPHA:
		ctx->g_alpha = ctrl->val;
		break;
	case V4L2_CID_2D_BLEND_OP:
		if (!ctx->sc_dev->variant->blending && (ctrl->val > 0)) {
			dev_err(ctx->sc_dev->dev,
				"%s: blending is not supported from v2.2.0\n",
				__func__);
			return -EINVAL;
		}
		ctx->bl_op = ctrl->val;
		break;
	case V4L2_CID_2D_FMT_PREMULTI:
		ctx->pre_multi = ctrl->val;
		break;
	case V4L2_CID_2D_DITH:
		ctx->dith = ctrl->val;
		break;
	case V4L2_CID_CSC_EQ:
		ctx->csc.csc_eq = ctrl->val;
		break;
	case V4L2_CID_CSC_RANGE:
		ctx->csc.csc_range = ctrl->val;
		break;
	case V4L2_CID_CONTENT_PROTECTION:
		ctx->cp_enabled = !!ctrl->val;
		break;
	case SC_CID_DNOISE_FT:
		ctx->dnoise_ft.strength = ctrl->val;
		break;
	case SC_CID_FRAMERATE:
		ctx->framerate = ctrl->val;
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops sc_ctrl_ops = {
	.s_ctrl = sc_s_ctrl,
};

static const struct v4l2_ctrl_config sc_custom_ctrl[] = {
	{
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_GLOBAL_ALPHA,
		.name = "Set constant src alpha",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = 0,
		.max = 255,
		.def = 255,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_2D_BLEND_OP,
		.name = "set blend op",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = 0,
		.max = BL_OP_ADD,
		.def = 0,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_2D_DITH,
		.name = "set dithering",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = false,
		.max = true,
		.def = false,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_2D_FMT_PREMULTI,
		.name = "set pre-multiplied format",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = false,
		.max = true,
		.def = false,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_CSC_EQ,
		.name = "Set CSC equation",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = V4L2_COLORSPACE_DEFAULT,
		.max = V4L2_COLORSPACE_BT2020,
		.def = V4L2_COLORSPACE_DEFAULT,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_CSC_RANGE,
		.name = "Set CSC range",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = SC_CSC_NARROW,
		.max = SC_CSC_WIDE,
		.def = SC_CSC_NARROW,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_CONTENT_PROTECTION,
		.name = "Enable contents protection",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = 0,
		.max = 1,
		.def = 0,
	}, {
		.ops = &sc_ctrl_ops,
		.id = SC_CID_DNOISE_FT,
		.name = "Enable denoising filter",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.step = 1,
		.min = 0,
		.max = SC_FT_MAX,
		.def = 0,
	}, {
		.ops = &sc_ctrl_ops,
		.id = SC_CID_FRAMERATE,
		.name = "Frame rate setting",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.step = 1,
		.min = 0,
		.max = SC_FRAMERATE_MAX,
		.def = SC_FRAMERATE_DEFAULT,
	}
};

static int sc_add_ctrls(struct sc_ctx *ctx)
{
	unsigned long i;

	v4l2_ctrl_handler_init(&ctx->ctrl_handler, SC_MAX_CTRL_NUM);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &sc_ctrl_ops,
			V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &sc_ctrl_ops,
			V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &sc_ctrl_ops,
			V4L2_CID_ROTATE, 0, 270, 90, 0);

	for (i = 0; i < ARRAY_SIZE(sc_custom_ctrl); i++)
		v4l2_ctrl_new_custom(&ctx->ctrl_handler,
				&sc_custom_ctrl[i], NULL);
	if (ctx->ctrl_handler.error) {
		int err = ctx->ctrl_handler.error;
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"v4l2_ctrl_handler_init failed %d\n", err);
		v4l2_ctrl_handler_free(&ctx->ctrl_handler);
		return err;
	}

	v4l2_ctrl_handler_setup(&ctx->ctrl_handler);

	return 0;
}

static int sc_power_clk_enable(struct sc_dev *sc)
{
	int ret;

	if (in_interrupt())
		ret = pm_runtime_get(sc->dev);
	else
		ret = pm_runtime_get_sync(sc->dev);

	if (ret < 0) {
		dev_err(sc->dev,
			"%s=%d: Failed to enable local power\n", __func__, ret);
		return ret;
	}

	if (!IS_ERR(sc->pclk)) {
		ret = clk_enable(sc->pclk);
		if (ret) {
			dev_err(sc->dev, "%s: Failed to enable PCLK (err %d)\n",
				__func__, ret);
			goto err_pclk;
		}
	}

	if (!IS_ERR(sc->aclk)) {
		ret = clk_enable(sc->aclk);
		if (ret) {
			dev_err(sc->dev, "%s: Failed to enable ACLK (err %d)\n",
				__func__, ret);
			goto err_aclk;
		}
	}

	return 0;
err_aclk:
	if (!IS_ERR(sc->pclk))
		clk_disable(sc->pclk);
err_pclk:
	pm_runtime_put(sc->dev);
	return ret;
}

static void sc_clk_power_disable(struct sc_dev *sc)
{
	sc_clear_aux_power_cfg(sc);

	if (!IS_ERR(sc->aclk))
		clk_disable(sc->aclk);

	if (!IS_ERR(sc->pclk))
		clk_disable(sc->pclk);

	pm_runtime_put(sc->dev);
}

#if IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
static int sc_ctrl_protection(struct sc_dev *sc, struct sc_ctx *ctx, bool en)
{
	int ret;

	ret = exynos_hvc(HVC_PROTECTION_SET, 0,
			SC_SMC_PROTECTION_ID(sc->dev_id),
			(en ? HVC_PROTECTION_ENABLE : HVC_PROTECTION_DISABLE), 0);
	if (ret != 0) {
		dev_err(sc->dev,
			"fail to protection enable (%d)\n", ret);
	}
	return ret;
}
#endif

static void sc_timeout_qos_work(struct work_struct *work)
{
	struct sc_ctx *ctx = container_of(work, struct sc_ctx, qos_work.work);

	mutex_lock(&ctx->pm_qos_lock);
	sc_remove_devfreq(ctx);
	mutex_unlock(&ctx->pm_qos_lock);
}

static int sc_open(struct file *file)
{
	struct sc_dev *sc = video_drvdata(file);
	struct sc_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		dev_err(sc->dev, "no memory for open context\n");
		return -ENOMEM;
	}

	atomic_inc(&sc->m2m.in_use);

	ctx->context_type = SC_CTX_V4L2_TYPE;
	INIT_LIST_HEAD(&ctx->node);
	ctx->sc_dev = sc;

	v4l2_fh_init(&ctx->fh, sc->m2m.vfd);

	INIT_DELAYED_WORK(&ctx->qos_work, sc_timeout_qos_work);
	ctx->pm_qos_lv = -1;
	mutex_init(&ctx->pm_qos_lock);
	INIT_LIST_HEAD(&ctx->node_qos);

	ret = sc_add_ctrls(ctx);
	if (ret)
		goto err_fh;

	ctx->fh.ctrl_handler = &ctx->ctrl_handler;
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	/* Default color format */
	ctx->s_frame.sc_fmt = &sc_formats[0];
	ctx->d_frame.sc_fmt = &sc_formats[0];

	if (!IS_ERR(sc->pclk)) {
		ret = clk_prepare(sc->pclk);
		if (ret) {
			dev_err(sc->dev, "%s: failed to prepare PCLK(err %d)\n",
					__func__, ret);
			goto err_pclk_prepare;
		}
	}

	if (!IS_ERR(sc->aclk)) {
		ret = clk_prepare(sc->aclk);
		if (ret) {
			dev_err(sc->dev, "%s: failed to prepare ACLK(err %d)\n",
					__func__, ret);
			goto err_aclk_prepare;
		}
	}

	/* Setup the device context for mem2mem mode. */
	ctx->m2m_ctx = v4l2_m2m_ctx_init(sc->m2m.m2m_dev, ctx, queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = -EINVAL;
		goto err_ctx;
	}

	ctx->pid = current->pid;

	return 0;

err_ctx:
	if (!IS_ERR(sc->aclk))
		clk_unprepare(sc->aclk);
err_aclk_prepare:
	if (!IS_ERR(sc->pclk))
		clk_unprepare(sc->pclk);
err_pclk_prepare:
	v4l2_fh_del(&ctx->fh);
err_fh:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_exit(&ctx->fh);
	atomic_dec(&sc->m2m.in_use);
	kfree(ctx);

	return ret;
}

static int sc_release(struct file *file)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(file->private_data);
	struct sc_dev *sc = ctx->sc_dev;

	sc_dbg("refcnt= %d", atomic_read(&sc->m2m.in_use));

	atomic_dec(&sc->m2m.in_use);

	v4l2_m2m_ctx_release(ctx->m2m_ctx);

	destroy_intermediate_frame_all(ctx);

	if (work_busy(&ctx->qos_work.work))
		flush_delayed_work(&ctx->qos_work);

	if (!IS_ERR(sc->aclk))
		clk_unprepare(sc->aclk);
	if (!IS_ERR(sc->pclk))
		clk_unprepare(sc->pclk);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);

	return 0;
}

static unsigned int sc_poll(struct file *file,
			     struct poll_table_struct *wait)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(file->private_data);

	return v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
}

static int sc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(file->private_data);

	return v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);
}

static const struct v4l2_file_operations sc_v4l2_fops = {
	.owner		= THIS_MODULE,
	.open		= sc_open,
	.release	= sc_release,
	.poll		= sc_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= sc_mmap,
};

static void sc_buffer_done(struct vb2_v4l2_buffer *vb,
			   enum vb2_buffer_state state)
{
	struct vb2_sc_buffer *sc_buf;

	sc_buf = sc_from_vb2_to_sc_buf(&vb->vb2_buf);

	if (sc_buf->out_sync_file && !sc_buf->tws) {
		if (state == VB2_BUF_STATE_ERROR)
			dma_fence_set_error(sc_buf->out_sync_file->fence,
					    -EFAULT);
		dma_fence_signal(sc_buf->out_sync_file->fence);
	}

	v4l2_m2m_buf_done(vb, state);
}

static bool sc_clear_votf(struct sc_tws *tws)
{
	bool ret;

	ret = sc_hwset_clear_votf_clock_en(tws);
	if (!ret)
		return ret;

	exynos_dpuf_set_votf(tws->sink.dpu_dma_idx, false);

	return true;
}

static void sc_job_finish(struct sc_dev *sc, struct sc_ctx *ctx)
{
	unsigned long flags;
	bool ret;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;

	if (ctx->tws) {
		sc_hwset_tws_flush(ctx->tws);

		ret = sc_hwset_clear_votf_clock_en(ctx->tws);
		if (!ret)
			dev_err(sc->dev, "failed to clear VOTF_CLOCK_EN\n");

		spin_lock_irqsave(&sc->tws_lock, flags);
		list_add_tail(&ctx->tws->node, &sc->tws_avail_list);
		spin_unlock_irqrestore(&sc->tws_lock, flags);

		ctx->tws = NULL;
	}

	spin_lock_irqsave(&sc->slock, flags);

	if (ctx->context_type == SC_CTX_V4L2_TYPE) {
		ctx = v4l2_m2m_get_curr_priv(sc->m2m.m2m_dev);
		if (!ctx || !ctx->m2m_ctx) {
			dev_err(sc->dev, "current ctx is NULL\n");
			spin_unlock_irqrestore(&sc->slock, flags);
			return;

		}
		clear_bit(CTX_RUN, &ctx->flags);

#if IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
		if (test_bit(DEV_CP, &sc->state)) {
			sc_ctrl_protection(sc, ctx, false);
			clear_bit(DEV_CP, &sc->state);
		}
#endif
		src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		BUG_ON(!src_vb || !dst_vb);

		spin_lock_irqsave(&sc->ctxlist_qos_lock, flags);
		list_del_init(&ctx->node_qos);
		spin_unlock_irqrestore(&sc->ctxlist_qos_lock, flags);

		sc_buffer_done(src_vb, VB2_BUF_STATE_ERROR);
		sc_buffer_done(dst_vb, VB2_BUF_STATE_ERROR);

		v4l2_m2m_job_finish(sc->m2m.m2m_dev, ctx->m2m_ctx);
	}

	spin_unlock_irqrestore(&sc->slock, flags);
}

static void sc_watchdog(struct timer_list *t)
{
	struct sc_dev *sc = from_timer(sc, t, wdt.timer);
	struct sc_ctx *ctx;
	unsigned long flags;

	sc_dbg("timeout watchdog\n");
	if (atomic_read(&sc->wdt.cnt) >= SC_WDT_CNT) {
		sc_hwset_soft_reset(sc);

		atomic_set(&sc->wdt.cnt, 0);
		clear_bit(DEV_RUN, &sc->state);

		spin_lock_irqsave(&sc->ctxlist_lock, flags);
		ctx = sc->current_ctx;
		sc->current_ctx = NULL;
		spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

		BUG_ON(!ctx);
		sc_job_finish(sc, ctx);
		sc_clk_power_disable(sc);
		return;
	}

	if (test_bit(DEV_RUN, &sc->state)) {
		sc_hwregs_dump(sc);
		if (sc->current_ctx)
			sc_ctx_dump(sc->current_ctx);
		atomic_inc(&sc->wdt.cnt);
		dev_err(sc->dev, "scaler is still running\n");
		mod_timer(&sc->wdt.timer, jiffies + SC_TIMEOUT);
	} else {
		sc_dbg("scaler finished job\n");
	}

}

static void sc_set_csc_coef(struct sc_ctx *ctx)
{
	struct sc_frame *s_frame, *d_frame;
	struct sc_dev *sc;
	enum sc_csc_idx idx;

	sc = ctx->sc_dev;
	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	if (s_frame->sc_fmt->is_rgb == d_frame->sc_fmt->is_rgb)
		idx = NO_CSC;
	else if (s_frame->sc_fmt->is_rgb)
		idx = CSC_R2Y;
	else
		idx = CSC_Y2R;

	sc_hwset_csc_coef(sc, idx, &ctx->csc);
}

static bool sc_process_multi_stage(struct sc_dev *sc, struct sc_ctx *ctx)
{
	struct sc_frame *s_frame, *d_frame;
	const struct sc_size_limit *limit;
	unsigned int halign = 0, walign = 0;
	unsigned int pre_h_ratio = 0;
	unsigned int pre_v_ratio = 0;
	unsigned int h_ratio = SCALE_RATIO(1, 1);
	unsigned int v_ratio = SCALE_RATIO(1, 1);

	if (!test_bit(CTX_INT_FRAME, &ctx->flags))
		return false;

	s_frame = &ctx->i_frame[ctx->cur_int_frame]->frame;
	ctx->cur_int_frame++;
	if (ctx->cur_int_frame == ctx->num_int_frame)
		d_frame = &ctx->d_frame;
	else
		d_frame = &ctx->i_frame[ctx->cur_int_frame]->frame;

	walign = d_frame->sc_fmt->h_shift;
	halign = d_frame->sc_fmt->v_shift;

	limit = &sc->variant->limit_input;
	v4l_bound_align_image(&s_frame->crop.width, limit->min_w, limit->max_w,
			walign, &s_frame->crop.height, limit->min_h,
			limit->max_h, halign, 0);

	sc_hwset_src_image_format(sc, s_frame);
	sc_hwset_dst_image_format(sc, d_frame);
	sc_hwset_src_imgsize(sc, s_frame);
	sc_hwset_dst_imgsize(sc, d_frame);

	if ((ctx->flip_rot_cfg & SCALER_ROT_90) &&
		(ctx->dnoise_ft.strength > SC_FT_BLUR)) {
		h_ratio = SCALE_RATIO(s_frame->crop.height, d_frame->crop.width);
		v_ratio = SCALE_RATIO(s_frame->crop.width, d_frame->crop.height);
	} else {
		h_ratio = SCALE_RATIO(s_frame->crop.width, d_frame->crop.width);
		v_ratio = SCALE_RATIO(s_frame->crop.height, d_frame->crop.height);
	}

	pre_h_ratio = 0;
	pre_v_ratio = 0;

	if (!sc->variant->ratio_20bit) {
		/* No prescaler, 1/4 precision */
		BUG_ON(h_ratio > SCALE_RATIO(4, 1));
		BUG_ON(v_ratio > SCALE_RATIO(4, 1));

		h_ratio >>= 4;
		v_ratio >>= 4;
	}

	/* no rotation */

	sc_hwset_hratio(sc, h_ratio, pre_h_ratio);
	sc_hwset_vratio(sc, v_ratio, pre_v_ratio);

	if (!sc->variant->is_bilinear) {
		sc_hwset_polyphase_hcoef(sc, h_ratio, h_ratio, 0);
		sc_hwset_polyphase_vcoef(sc, v_ratio, v_ratio, 0);
	}

	sc_hwset_src_pos(sc, s_frame->crop.left, s_frame->crop.top,
			s_frame->sc_fmt->h_shift, s_frame->sc_fmt->v_shift);
	sc_hwset_src_wh(sc, s_frame->crop.width, s_frame->crop.height,
			pre_h_ratio, pre_v_ratio,
			s_frame->sc_fmt->h_shift, s_frame->sc_fmt->v_shift);

	sc_hwset_dst_pos(sc, d_frame->crop.left, d_frame->crop.top);
	sc_hwset_dst_wh(sc, d_frame->crop.width, d_frame->crop.height);

	sc_hwset_src_addr(sc, s_frame);
	sc_hwset_dst_addr(sc, d_frame);

	if ((ctx->flip_rot_cfg & SCALER_ROT_MASK) &&
		(ctx->dnoise_ft.strength > SC_FT_BLUR))
		sc_hwset_flip_rotation(sc, ctx->flip_rot_cfg);
	else
		sc_hwset_flip_rotation(sc, 0);

	sc_hwset_start(sc);

	if (ctx->cur_int_frame == ctx->num_int_frame)
		clear_bit(CTX_INT_FRAME, &ctx->flags);

	return true;
}

static void sc_set_dithering(struct sc_ctx *ctx)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_frame *s_frame = &ctx->s_frame;
	struct sc_frame *d_frame = &ctx->d_frame;
	unsigned int val = 0;
	bool is_bypass = false;

	if (ctx->dith)
		val = sc_dith_val(1, 1, 1);

	if (sc->variant->pixfmt_10bit) {
		/*
		 * Whole IP use dithering for dst of 8P2.
		 * So, with src of 8P2, it need to inverse dithering.
		 */
		if (sc_fmt_is_s10bit_yuv(s_frame->sc_fmt->pixelformat))
			val |= SCALER_DITH_SRC_INV;

		if ((s_frame->sc_fmt->cfg_val == d_frame->sc_fmt->cfg_val) &&
		    (ctx->h_ratio == SCALE_RATIO(1, 1)) &&
		    (ctx->v_ratio == SCALE_RATIO(1, 1)))
			is_bypass = true;

		/*
		 * dithering should be done in the case with these.
		 * 1. dst is not 10 bit.
		 * 2. except no csc
		 * 	(in this, csc means format is changed to whatever).
		 * 3. except no scaling.
		 * 4. dst is 8P2 format.
		 */
		if ((!sc_cfg_is_10bit_format(d_frame->sc_fmt->cfg_val) &&
		     !is_bypass) ||
		    sc_fmt_is_s10bit_yuv(d_frame->sc_fmt->pixelformat))
			val |= SCALER_DITH_DST_EN;

		if (sc->version >= SCALER_VERSION(5, 1, 0)) {
			/*
			 * In case of bypass(no csc, no scaling),
			 * SRC and DST should be identical.
			 * But, there is the logic to tune data while bit-extension.
			 * By this, the data can be changed.
			 * So, in this case,
			 * disable this and make HW run as previous version.
			 */
			if (!sc_cfg_is_10bit_format(d_frame->sc_fmt->cfg_val) &&
			    is_bypass)
				sc_hwset_csc_prev_ver(sc, true);
			else
				sc_hwset_csc_prev_ver(sc, false);
		}
	}

	sc_dbg("dither value is 0x%x\n", val);
	sc_hwset_dith(sc, val);
}

static void sc_set_initial_phase(struct sc_ctx *ctx)
{
	struct sc_dev *sc = ctx->sc_dev;

	/* TODO: need to check scaling, csc, rot according to H/W Goude  */
	sc_hwset_src_init_phase(sc, &ctx->init_phase);
}

static int sc_run_next_job(struct sc_dev *sc)
{
	unsigned long flags;
	struct sc_ctx *ctx;
	struct sc_frame *d_frame, *s_frame;
	unsigned int pre_h_ratio = 0;
	unsigned int pre_v_ratio = 0;
	unsigned int h_ratio = SCALE_RATIO(1, 1);
	unsigned int v_ratio = SCALE_RATIO(1, 1);
	unsigned int ch_ratio = SCALE_RATIO(1, 1);
	unsigned int cv_ratio = SCALE_RATIO(1, 1);
	unsigned int h_shift, v_shift;
	int ret;

	spin_lock_irqsave(&sc->ctxlist_lock, flags);

	if (sc->current_ctx || list_empty(&sc->context_list)) {
		/* a job is currently being processed or no job is to run */
		spin_unlock_irqrestore(&sc->ctxlist_lock, flags);
		return 0;
	}

	/*
	 * sc_run_next_job() must not reenter while sc->state is DEV_RUN.
	 * DEV_RUN is cleared when an operation is finished.
	 */
	BUG_ON(test_bit(DEV_RUN, &sc->state));

	set_bit(DEV_RUN, &sc->state);

	if (test_bit(DEV_SUSPEND, &sc->state)) {
		clear_bit(DEV_RUN, &sc->state);
		spin_unlock_irqrestore(&sc->ctxlist_lock, flags);
		return 0;
	}

	ctx = list_first_entry(&sc->context_list, struct sc_ctx, node);

	set_bit(CTX_RUN, &ctx->flags);
	list_del_init(&ctx->node);

	sc->current_ctx = ctx;

	spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

	ret = sc_power_clk_enable(sc);
	if (ret) {
		/*
		 * Failed to enable the power and the clock. Let's push the task
		 * again for the later retry.
		 */
		spin_lock_irqsave(&sc->ctxlist_lock, flags);

		list_add(&ctx->node, &sc->context_list);
		sc->current_ctx = NULL;

		clear_bit(CTX_RUN, &ctx->flags);

		spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

		clear_bit(DEV_RUN, &sc->state);

		/*
		 * V4L2 mem2mem assumes that the tasks in device_run() are
		 * always succeed in processing in H/W while m2m1shot accepts
		 * failure in device_run(). m2m1shot2 returns failure to the
		 * users if devce_run() fails. To prevent returning failure to
		 * users and losing a task to run, we should assume that
		 * processing a task always succeeds.
		 */
		return 0;
	}

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	sc_hwset_clk_request(sc, true);

	if (ctx->context_type == SC_CTX_EXT_TYPE)
		return sc_ext_run_job(ctx);

	if (ctx->num_int_frame != 0) {
		set_bit(CTX_INT_FRAME, &ctx->flags);
		ctx->cur_int_frame = 0;
		d_frame = &ctx->i_frame[ctx->cur_int_frame]->frame;
	}

	sc_hwset_src_image_format(sc, s_frame);
	sc_hwset_dst_image_format(sc, d_frame);

	sc_hwset_pre_multi_format(sc, s_frame->pre_multi, d_frame->pre_multi);

	sc_hwset_src_imgsize(sc, s_frame);
	sc_hwset_dst_imgsize(sc, d_frame);

	sc_set_csc_coef(ctx);

	h_ratio = ctx->h_ratio;
	v_ratio = ctx->v_ratio;
	pre_h_ratio = ctx->pre_h_ratio;
	pre_v_ratio = ctx->pre_v_ratio;

	if (!sc->variant->ratio_20bit) {

		/* No prescaler, 1/4 precision */
		BUG_ON(h_ratio > SCALE_RATIO(4, 1));
		BUG_ON(v_ratio > SCALE_RATIO(4, 1));

		h_ratio >>= 4;
		v_ratio >>= 4;
	}

	h_shift = s_frame->sc_fmt->h_shift;
	v_shift = s_frame->sc_fmt->v_shift;

	if (!!(ctx->flip_rot_cfg & SCALER_ROT_90)) {
		swap(pre_h_ratio, pre_v_ratio);
		swap(h_shift, v_shift);
	}

	if (h_shift < d_frame->sc_fmt->h_shift)
		ch_ratio = h_ratio * 2; /* chroma scaling down */
	else if (h_shift > d_frame->sc_fmt->h_shift)
		ch_ratio = h_ratio / 2; /* chroma scaling up */
	else
		ch_ratio = h_ratio;

	if (v_shift < d_frame->sc_fmt->v_shift)
		cv_ratio = v_ratio * 2; /* chroma scaling down */
	else if (v_shift > d_frame->sc_fmt->v_shift)
		cv_ratio = v_ratio / 2; /* chroma scaling up */
	else
		cv_ratio = v_ratio;

	sc_hwset_hratio(sc, h_ratio, pre_h_ratio);
	sc_hwset_vratio(sc, v_ratio, pre_v_ratio);

	if (!sc->variant->is_bilinear) {
		sc_hwset_polyphase_hcoef(sc, h_ratio, ch_ratio,
				ctx->dnoise_ft.strength);
		sc_hwset_polyphase_vcoef(sc, v_ratio, cv_ratio,
				ctx->dnoise_ft.strength);
	}

	sc_hwset_src_pos(sc, s_frame->crop.left, s_frame->crop.top,
			s_frame->sc_fmt->h_shift, s_frame->sc_fmt->v_shift);
	sc_hwset_src_wh(sc, s_frame->crop.width, s_frame->crop.height,
			pre_h_ratio, pre_v_ratio,
			s_frame->sc_fmt->h_shift, s_frame->sc_fmt->v_shift);

	sc_hwset_dst_pos(sc, d_frame->crop.left, d_frame->crop.top);
	sc_hwset_dst_wh(sc, d_frame->crop.width, d_frame->crop.height);

	if (sc->variant->initphase)
		sc_set_initial_phase(ctx);

	sc_hwset_src_addr(sc, s_frame);
	sc_hwset_dst_addr(sc, d_frame);

	sc_set_dithering(ctx);

	if (ctx->bl_op != BL_NO_OP)
		sc_hwset_blend(sc, ctx->bl_op, ctx->pre_multi, ctx->g_alpha);

	if (ctx->dnoise_ft.strength > SC_FT_BLUR)
		sc_hwset_flip_rotation(sc, 0);
	else
		sc_hwset_flip_rotation(sc, ctx->flip_rot_cfg);

	if (sc->votf_regs) {
		if (ctx->tws) {
			struct vb2_buffer *dst_vb;
			struct vb2_sc_buffer *dst_sc_buf;
			struct v4l2_m2m_ctx *m2m_ctx = ctx->m2m_ctx;

			sc_hwset_votf_ring_clk_en(sc);
			sc_hwset_init_votf(sc);

			sc_hwset_votf_en(sc, true);
			sc_hwset_votf(sc, ctx->tws);

			dst_vb = (struct vb2_buffer *)v4l2_m2m_next_dst_buf(m2m_ctx);
			dst_sc_buf = sc_from_vb2_to_sc_buf(dst_vb);

			if (dst_sc_buf->out_sync_file)
				dma_fence_signal(dst_sc_buf->out_sync_file->fence);
		} else {
			sc_hwset_votf_en(sc, false);
		}
	}

	sc_hwset_int_en(sc);

	if (sc_show_stat & 0x1)
		sc_hwregs_dump(sc);
	if (sc_show_stat & 0x2)
		sc_ctx_dump(ctx);

	mod_timer(&sc->wdt.timer, jiffies + SC_TIMEOUT);

	if (__enable_systrace)
		sc_tracing_mark_write(ctx, 'C', "MSCL_HW_RUN", true);
	if (__measure_hw_latency) {
		if (ctx->context_type == SC_CTX_V4L2_TYPE) {
			struct vb2_v4l2_buffer *vb =
					v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
			struct v4l2_m2m_buffer *mb =
					container_of(vb, typeof(*mb), vb);
			struct vb2_sc_buffer *svb =
					container_of(mb, typeof(*svb), mb);

			svb->ktime = ktime_get();
		}
	}
#if IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
	if (ctx->cp_enabled) {
		ret = sc_ctrl_protection(sc, ctx, true);
		if (!ret)
			set_bit(DEV_CP, &sc->state);
	}
#endif
	sc_hwset_start(sc);

	return 0;
}

static int sc_add_context_and_run(struct sc_dev *sc, struct sc_ctx *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&sc->ctxlist_lock, flags);
	list_add_tail(&ctx->node, &sc->context_list);
	spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

	return sc_run_next_job(sc);
}

static void sc_clear_tws_work(struct work_struct *work)
{
	struct sc_tws *tws = container_of(work, struct sc_tws, tws_work.work);
	struct sc_dev *sc = tws->sc_dev;
	bool ret;
	unsigned long flags;

	ret = sc_clear_votf(tws);
	if (!ret) {
		mod_delayed_work(system_wq, &tws->tws_work,
				 usecs_to_jiffies(100));
		return;
	}

	spin_lock_irqsave(&sc->tws_lock, flags);
	list_add_tail(&tws->node, &sc->tws_avail_list);
	spin_unlock_irqrestore(&sc->tws_lock, flags);

	sc_clk_power_disable(sc);
}

static irqreturn_t sc_irq_handler(int irq, void *priv)
{
	struct sc_dev *sc = priv;
	struct sc_ctx *ctx;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;
	u32 irq_status;
	struct sc_tws *tws = NULL;

	spin_lock(&sc->slock);

	/*
	 * ok to access sc->current_ctx withot ctxlist_lock held
	 * because it is not modified until sc_run_next_job() is called.
	 */
	ctx = sc->current_ctx;

	BUG_ON(!ctx);

	irq_status = sc_hwget_and_clear_irq_status(sc);

	if (SCALER_INT_OK(irq_status) && sc_process_multi_stage(sc, ctx))
		goto isr_unlock;

	if (ctx->context_type == SC_CTX_EXT_TYPE && SCALER_INT_OK(irq_status)) {
		if (!sc_ext_job_finished(ctx)) {
			sc_ext_run_job(ctx);
			goto isr_unlock;
		}
	}

	del_timer(&sc->wdt.timer);

	if (!SCALER_INT_OK(irq_status)) {
		sc_hwset_soft_reset(sc);
	} else {
		if (sc_hwget_is_sbwc(sc))
			sc_hwset_soft_reset_no_bus_idle(sc);
		else
			sc_hwset_clk_request(sc, false);
	}

#if IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
	if (test_bit(DEV_CP, &sc->state)) {
		sc_ctrl_protection(sc, ctx, false);
		clear_bit(DEV_CP, &sc->state);
	}
#endif

	clear_bit(DEV_RUN, &sc->state);
	clear_bit(CTX_RUN, &ctx->flags);

	if (ctx->context_type == SC_CTX_V4L2_TYPE) {
		BUG_ON(ctx != v4l2_m2m_get_curr_priv(sc->m2m.m2m_dev));

		src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		BUG_ON(!src_vb || !dst_vb);

		if (__enable_systrace)
			sc_tracing_mark_write(ctx, 'C', "MSCL_HW_RUN", false);
		if (__measure_hw_latency) {
			struct v4l2_m2m_buffer *mb =
					container_of(dst_vb, typeof(*mb), vb);
			struct vb2_sc_buffer *svb =
					container_of(mb, typeof(*svb), mb);

			dst_vb->vb2_buf.timestamp =
					ktime_sub(ktime_get(), svb->ktime);

			if (sc_show_stat & 0x4)
				dev_info(sc->dev, "H/W time : %ld us\n",
				(unsigned long)ktime_to_us(dst_vb->vb2_buf.timestamp));
		}

		if (ctx->tws) {
			tws = ctx->tws;
			ctx->tws = NULL;
		}

		spin_lock(&sc->ctxlist_qos_lock);
		list_del_init(&ctx->node_qos);
		spin_unlock(&sc->ctxlist_qos_lock);

		sc_buffer_done(src_vb,
			       SCALER_INT_OK(irq_status) ? VB2_BUF_STATE_DONE
							 : VB2_BUF_STATE_ERROR);
		sc_buffer_done(dst_vb,
			       SCALER_INT_OK(irq_status) ? VB2_BUF_STATE_DONE
							 : VB2_BUF_STATE_ERROR);

		v4l2_m2m_job_finish(sc->m2m.m2m_dev, ctx->m2m_ctx);
	} else {
		BUG_ON(ctx->context_type != SC_CTX_EXT_TYPE);

		/* TODO: time measure */
		sc_ext_current_task_finish(sc->xdev,
					SCALER_INT_OK(irq_status));
	}

	spin_lock(&sc->ctxlist_lock);
	sc->current_ctx = NULL;
	spin_unlock(&sc->ctxlist_lock);

	wake_up(&sc->wait);

	sc_run_next_job(sc);

	if (tws) {
		tws->ktime = ktime_get();
		mod_delayed_work(system_wq, &tws->tws_work,
				 usecs_to_jiffies(100));
	} else {
		sc_clk_power_disable(sc);
	}
isr_unlock:
	spin_unlock(&sc->slock);

	return IRQ_HANDLED;
}

static dma_addr_t sc_get_vb2_dma_addr(struct vb2_buffer *vb2buf, int plane_no)
{
	struct sg_table *sgt;

	sgt = vb2_dma_sg_plane_desc(vb2buf, plane_no);
	if (!sgt)
		return -ENOMEM;

	return sg_dma_address(sgt->sgl);
}

static int sc_get_bufaddr(struct sc_dev *sc, struct vb2_buffer *vb2buf,
		struct sc_frame *frame)
{
	int i;

	for (i = 0; i < SC_MAX_PLANES; i++) {
		if (i < frame->sc_fmt->num_planes)
			frame->addr.ioaddr[i] = sc_get_vb2_dma_addr(vb2buf, i);
		else if (i < frame->sc_fmt->num_comp)
			frame->addr.ioaddr[i] = frame->addr.ioaddr[i - 1] + frame->addr.size[i - 1];
		else
			frame->addr.ioaddr[i] = 0;
	}

	if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_YVU420 ||
			frame->sc_fmt->pixelformat == V4L2_PIX_FMT_YVU420M) {
		u32 t_cb = frame->addr.ioaddr[SC_PLANE_CB];
		frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_CR];
		frame->addr.ioaddr[SC_PLANE_CR] = t_cb;
	}

	sc_dbg("y addr %pa y size %#x\n", &frame->addr.ioaddr[SC_PLANE_Y], frame->addr.size[SC_PLANE_Y]);
	sc_dbg("cb addr %pa cb size %#x\n", &frame->addr.ioaddr[SC_PLANE_CB], frame->addr.size[SC_PLANE_CB]);
	sc_dbg("cr addr %pa cr size %#x\n", &frame->addr.ioaddr[SC_PLANE_CR], frame->addr.size[SC_PLANE_CR]);

	return 0;
}

static void sc_m2m_device_run(void *priv)
{
	struct sc_ctx *ctx = priv;
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_frame *s_frame, *d_frame;
	struct vb2_buffer *src_vb, *dst_vb;
	struct vb2_v4l2_buffer *src_vb_v4l2, *dst_vb_v4l2;
	struct vb2_sc_buffer *src_sc_buf, *dst_sc_buf;
	unsigned long flags;
	struct sc_tws *tws = NULL;

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	src_vb = (struct vb2_buffer *)v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	dst_vb = (struct vb2_buffer *)v4l2_m2m_next_dst_buf(ctx->m2m_ctx);

	src_sc_buf = sc_from_vb2_to_sc_buf(src_vb);
	dst_sc_buf = sc_from_vb2_to_sc_buf(dst_vb);

	if (src_sc_buf->state || dst_sc_buf->state) {
		src_vb_v4l2 = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		dst_vb_v4l2 = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		spin_lock_irqsave(&sc->tws_lock, flags);
		tws = dst_sc_buf->tws;
		dst_sc_buf->tws = NULL;
		spin_unlock_irqrestore(&sc->tws_lock, flags);
		if (tws) {
			spin_lock_irqsave(&sc->tws_lock, flags);
			list_add_tail(&tws->node, &sc->tws_avail_list);
			spin_unlock_irqrestore(&sc->tws_lock, flags);
			exynos_dpuf_set_votf(tws->sink.dpu_dma_idx, false);
		}
		spin_lock_irqsave(&sc->ctxlist_qos_lock, flags);
		list_del_init(&ctx->node_qos);
		spin_unlock_irqrestore(&sc->ctxlist_qos_lock, flags);

		sc_buffer_done(src_vb_v4l2, VB2_BUF_STATE_ERROR);
		sc_buffer_done(dst_vb_v4l2, VB2_BUF_STATE_ERROR);

		v4l2_m2m_job_finish(sc->m2m.m2m_dev, ctx->m2m_ctx);
		return;
	}

	if (dst_sc_buf->tws)
		ctx->tws = dst_sc_buf->tws;

	sc_get_bufaddr(sc, src_vb, s_frame);
	sc_get_bufaddr(sc, dst_vb, d_frame);

	sc_add_context_and_run(sc, ctx);
}

static void sc_m2m_job_abort(void *priv)
{
	struct sc_ctx *ctx = priv;
	int ret;

	ret = sc_ctx_stop_req(ctx);
	if (ret < 0)
		dev_err(ctx->sc_dev->dev, "wait timeout\n");
}

static struct v4l2_m2m_ops sc_m2m_ops = {
	.device_run	= sc_m2m_device_run,
	.job_abort	= sc_m2m_job_abort,
};

int sc_ext_device_run(struct sc_ctx *ctx)
{
	return sc_add_context_and_run(ctx->sc_dev, ctx);
}

static void sc_unregister_m2m_device(struct sc_dev *sc)
{
	video_unregister_device(sc->m2m.vfd);
	v4l2_m2m_release(sc->m2m.m2m_dev);
	v4l2_device_unregister(&sc->m2m.v4l2_dev);
}

#define SC_V4L2_CAP_FENCE		0x20000000
#define SC_V4L2_CAP_VOTF		0x40000000
static int sc_register_m2m_device(struct sc_dev *sc, int dev_id)
{
	struct v4l2_device *v4l2_dev;
	struct device *dev;
	struct video_device *vfd;
	int ret = 0;

	dev = sc->dev;
	v4l2_dev = &sc->m2m.v4l2_dev;

	scnprintf(v4l2_dev->name, sizeof(v4l2_dev->name), "%s.m2m",
			MODULE_NAME);

	ret = v4l2_device_register(dev, v4l2_dev);
	if (ret) {
		dev_err(sc->dev, "failed to register v4l2 device\n");
		return ret;
	}

	vfd = video_device_alloc();
	if (!vfd) {
		dev_err(sc->dev, "failed to allocate video device\n");
		goto err_v4l2_dev;
	}

	vfd->fops	= &sc_v4l2_fops;
	vfd->ioctl_ops	= &sc_v4l2_ioctl_ops;
	vfd->release	= video_device_release;
	vfd->lock	= &sc->lock;
	vfd->vfl_dir	= VFL_DIR_M2M;
	vfd->v4l2_dev	= v4l2_dev;
	vfd->device_caps =  V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING |
			    SC_V4L2_CAP_FENCE;
	if (sc->votf_regs)
		vfd->device_caps |= SC_V4L2_CAP_VOTF;
	scnprintf(vfd->name, sizeof(vfd->name), "%s:m2m", MODULE_NAME);

	video_set_drvdata(vfd, sc);

	sc->m2m.vfd = vfd;
	sc->m2m.m2m_dev = v4l2_m2m_init(&sc_m2m_ops);
	if (IS_ERR(sc->m2m.m2m_dev)) {
		dev_err(sc->dev, "failed to initialize v4l2-m2m device\n");
		ret = PTR_ERR(sc->m2m.m2m_dev);
		goto err_dev_alloc;
	}

	ret = video_register_device(vfd, VFL_TYPE_VIDEO, 50 + dev_id);
	if (ret) {
		dev_err(sc->dev, "failed to register video device (video%d)\n",
			50 + dev_id);
		goto err_m2m_dev;
	}

	return 0;

err_m2m_dev:
	v4l2_m2m_release(sc->m2m.m2m_dev);
err_dev_alloc:
	video_device_release(sc->m2m.vfd);
err_v4l2_dev:
	v4l2_device_unregister(v4l2_dev);

	return ret;
}

static int sc_sysmmu_fault_handler(struct iommu_fault *fault, void *data)
{
	struct sc_dev *sc = data;
	struct device *dev = sc->dev;
	unsigned long iova = fault->event.addr;

	if (test_bit(DEV_RUN, &sc->state)) {
		dev_info(dev, "System MMU fault called for IOVA %#lx\n", iova);
		sc_hwregs_dump(sc);
		if (sc->current_ctx)
			sc_ctx_dump(sc->current_ctx);
	}

	return 0;
}

static int sc_clk_get(struct sc_dev *sc)
{
	sc->aclk = devm_clk_get(sc->dev, "gate");
	if (IS_ERR(sc->aclk)) {
		if (PTR_ERR(sc->aclk) != -ENOENT) {
			dev_err(sc->dev, "Failed to get 'gate' clock: %ld",
				PTR_ERR(sc->aclk));
			return PTR_ERR(sc->aclk);
		}
		dev_info(sc->dev, "'gate' clock is not present\n");
	}

	sc->pclk = devm_clk_get(sc->dev, "gate2");
	if (IS_ERR(sc->pclk)) {
		if (PTR_ERR(sc->pclk) != -ENOENT) {
			dev_err(sc->dev, "Failed to get 'gate2' clock: %ld",
				PTR_ERR(sc->pclk));
			return PTR_ERR(sc->pclk);
		}
		dev_info(sc->dev, "'gate2' clock is not present\n");
	}

	sc->clk_chld = devm_clk_get(sc->dev, "mux_user");
	if (IS_ERR(sc->clk_chld)) {
		if (PTR_ERR(sc->clk_chld) != -ENOENT) {
			dev_err(sc->dev, "Failed to get 'mux_user' clock: %ld",
				PTR_ERR(sc->clk_chld));
			return PTR_ERR(sc->clk_chld);
		}
		dev_info(sc->dev, "'mux_user' clock is not present\n");
	}

	if (!IS_ERR(sc->clk_chld)) {
		sc->clk_parn = devm_clk_get(sc->dev, "mux_src");
		if (IS_ERR(sc->clk_parn)) {
			dev_err(sc->dev, "Failed to get 'mux_src' clock: %ld",
				PTR_ERR(sc->clk_parn));
			return PTR_ERR(sc->clk_parn);
		}
	} else {
		sc->clk_parn = ERR_PTR(-ENOENT);
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int sc_suspend(struct device *dev)
{
	struct sc_dev *sc = dev_get_drvdata(dev);
	int ret;

	set_bit(DEV_SUSPEND, &sc->state);

	ret = wait_event_timeout(sc->wait,
			!test_bit(DEV_RUN, &sc->state), SC_TIMEOUT);
	if (ret == 0)
		dev_err(sc->dev, "wait timeout\n");

	return 0;
}

static int sc_resume(struct device *dev)
{
	struct sc_dev *sc = dev_get_drvdata(dev);

	clear_bit(DEV_SUSPEND, &sc->state);

	sc_run_next_job(sc);

	return 0;
}
#endif

#ifdef CONFIG_PM
static int sc_runtime_resume(struct device *dev)
{
	struct sc_dev *sc = dev_get_drvdata(dev);

	if (!IS_ERR(sc->clk_chld) && !IS_ERR(sc->clk_parn)) {
		int ret = clk_set_parent(sc->clk_chld, sc->clk_parn);
		if (ret) {
			dev_err(sc->dev, "%s: Failed to setup MUX: %d\n",
				__func__, ret);
			return ret;
		}
	}

#if IS_ENABLED(CONFIG_SOC_S5E9935)
	/*
	 * There was some HW bug about failure of M2M block power off.
	 * This is about W/A to that bug.
	 * But, in some SOC, even though the bug is fixed by revision,
	 * it supports one-binary to revision.
	 * So, check the revision before this.
	 */
	if (exynos_soc_info.main_rev == 0)
		exynos_update_ip_idle_status(sc->idx_idle_ip, 0);
#endif

	return 0;
}

static int sc_runtime_suspend(struct device *dev)
{
	__maybe_unused struct sc_dev *sc = dev_get_drvdata(dev);

#if IS_ENABLED(CONFIG_SOC_S5E9935)
	if (exynos_soc_info.main_rev == 0)
		exynos_update_ip_idle_status(sc->idx_idle_ip, 1);
#endif

	return 0;
}
#endif

static const struct dev_pm_ops sc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sc_suspend, sc_resume)
	SET_RUNTIME_PM_OPS(NULL, sc_runtime_resume, sc_runtime_suspend)
};

/* sort qos table in descending order of freq_int */
static int sc_compare_qos_table_entries(const void *p1, const void *p2)
{
	const struct sc_qos_table *t1 = p1, *t2 = p2;

	if (t1->freq_int < t2->freq_int)
		return 1;
	else
		return -1;

	return 0;
}

static int sc_get_pm_qos_from_dt(struct sc_dev *sc)
{
	struct device *dev = sc->dev;
	struct sc_qos_table *qos_table;
	struct sc_ppc_table *ppc_table;
	int i, len;

	len = of_property_count_u32_elems(dev->of_node, "mscl_qos_table");
	if (len <= 0) {
		dev_info(dev, "No qos table for scaler\n");
		return 0;
	}

	sc->qos_table_cnt = len / 3;

	qos_table = devm_kcalloc(dev, sc->qos_table_cnt, sizeof(*qos_table),
				 GFP_KERNEL);
	if (!qos_table)
		return -ENOMEM;

	of_property_read_u32_array(dev->of_node, "mscl_qos_table",
				   (unsigned int *)qos_table, len);

	sort(qos_table, sc->qos_table_cnt, sizeof(*qos_table),
	     sc_compare_qos_table_entries, NULL);

	for (i = 0; i < sc->qos_table_cnt; i++) {
		dev_info(dev, "MSCL QoS Table[%d] mif : %u int : %u [%u]\n", i,
			 qos_table[i].freq_mif, qos_table[i].freq_int,
			 qos_table[i].freq_mscl);
	}

	sc->qos_table = qos_table;

	len = of_property_count_u32_elems(dev->of_node, "mscl_ppc_table");
	if (len <= 0) {
		dev_info(dev, "No ppc table for scaler\n");
		return -ENOENT;
	}

	sc->ppc_table_cnt = len / 3;

	ppc_table = devm_kcalloc(dev, sc->ppc_table_cnt, sizeof(*ppc_table),
				 GFP_KERNEL);
	if (!ppc_table)
		return -ENOMEM;

	of_property_read_u32_array(dev->of_node, "mscl_ppc_table",
				   (unsigned int *)ppc_table, len);

	for (i = 0; i < sc->ppc_table_cnt; i++) {
		dev_info(dev, "MSCL PPC Table[%d] bpp : %u ppc : %u/%u\n", i,
			 ppc_table[i].bpp,
			 ppc_table[i].ppc[0],
			 ppc_table[i].ppc[1]);
	}

	sc->ppc_table = ppc_table;

	if (of_property_read_u32(dev->of_node, "mscl_mif_ref", &sc->mif_ref)) {
		dev_err(dev, "mscl_mif_ref is not found\n");
		return -ENOENT;
	}
	if (of_property_read_u32(dev->of_node, "mscl_bw_ref", &sc->bw_ref)) {
		dev_err(dev, "mscl_bw_ref is not found\n");
		return -ENOENT;
	}

	if (of_property_read_u32(dev->of_node, "mscl_dvfs_class", &sc->dvfs_class)) {
		dev_info(dev, "mscl_dvfs_class is not found, so use INT as default\n");
		sc->dvfs_class = PM_QOS_DEVICE_THROUGHPUT;
	}
	dev_info(dev, "sc->dvfs_class is %d\n", sc->dvfs_class);

	len = of_property_count_u32_elems(dev->of_node, "mscl_min_bus_int_table");
	if (len <= 0) {
		dev_info(dev, "No min_bus_int_table for scaler\n");
	} else {
		struct sc_min_bus_int_table *min_bus_int_table;

		sc->min_bus_int_table_cnt = len / 2;

		min_bus_int_table = devm_kcalloc(dev, sc->min_bus_int_table_cnt,
						 sizeof(*min_bus_int_table),
						 GFP_KERNEL);
		if (!min_bus_int_table)
			return -ENOMEM;

		of_property_read_u32_array(dev->of_node,
					"mscl_min_bus_int_table",
					(unsigned int *)min_bus_int_table, len);

		sc->min_bus_int_table = min_bus_int_table;

		for (i = 0; i < sc->min_bus_int_table_cnt; i++)
			dev_info(dev,
				 "Min-bus-int Table[%d] bpp : %u int : %u\n",
				 i, min_bus_int_table[i].bpp,
				 min_bus_int_table[i].min_bus_int);
	}

	return 0;
}

static void sc_put_device_wrapper(void *arg)
{
	put_device((struct device *)arg);
}

#define SC_VOTF_NOT_SUPPORTED	1
static int sc_get_mscl_votf_of_dt(struct sc_dev *sc)
{
	const __be32 *votf_reg;
	int bytes;
	const void *prop;
	u32 cells;
	phys_addr_t votf_base;
	size_t votf_size;

	votf_reg = of_get_property(sc->dev->of_node, "mscl_votf_reg", &bytes);
	if (!votf_reg)
		/* absence of vOTF is not error, but it is not error. */
		return SC_VOTF_NOT_SUPPORTED;

	if (bytes != (sizeof(*votf_reg) * 2)) {
		dev_err(sc->dev, "mscl_votf_reg in DT is invalid\n");
		return -EINVAL;
	}
	prop = of_get_property(sc->dev->of_node,
			       "#mscl-votf-address-cells", NULL);
	if (!prop) {
		dev_err(sc->dev, "#mscl-votf-address-cells is missed\n");
		return -ENOENT;
	}
	cells = be32_to_cpup(prop);

	votf_base = of_read_number(votf_reg, cells);

	votf_reg += cells;

	prop = of_get_property(sc->dev->of_node, "#mscl-votf-size-cells", NULL);
	if (!prop) {
		dev_err(sc->dev, "#mscl-votf-size-cells is missed\n");
		return -ENOENT;
	}
	cells = be32_to_cpup(prop);

	votf_size = of_read_number(votf_reg, cells);

	sc->votf_regs = devm_ioremap(sc->dev, votf_base, votf_size);
	if (!sc->votf_regs)
		return -ENOMEM;

	sc->votf_base_pa = votf_base;

	return 0;
}

#define SC_NUM_ARGS_VOTF_TARGET		4
static int sc_get_sink_votf_of_dt(struct sc_dev *sc)
{
	int count;
	int ret;
	int i;

	count = of_count_phandle_with_args(sc->dev->of_node,
					   "samsung,mscl-votf_targets", NULL);
	if (count <= 0)
		/* absence of vOTF is not error, but it is not error. */
		return SC_VOTF_NOT_SUPPORTED;

	/* In that list of DT, the data contains one phandle and 4 arguments. */
	if ((count % (1 + SC_NUM_ARGS_VOTF_TARGET)) != 0) {
		dev_err(sc->dev,
			"Number of votf_targets(%d) is invalid\n", count);
		return -EINVAL;
	}
	count /= (1 + SC_NUM_ARGS_VOTF_TARGET);

	sc->votf_table = devm_kcalloc(sc->dev, count, sizeof(*sc->votf_table),
				      GFP_KERNEL);
	if (!sc->votf_table)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		struct platform_device *pdev_of_phandle;
		struct of_phandle_args pa;
		struct sc_votf_target *votf_target;

		votf_target = &sc->votf_table[i];

		ret = of_parse_phandle_with_fixed_args(sc->dev->of_node,
						"samsung,mscl-votf_targets",
						       SC_NUM_ARGS_VOTF_TARGET,
						       i, &pa);
		if (ret) {
			dev_err(sc->dev,
				"fail to get phandle for vOTF(err:%d)\n", ret);
			return ret;
		}

		pdev_of_phandle = of_find_device_by_node(pa.np);

		if (!pdev_of_phandle) {
			dev_err(sc->dev, "fail to get pdev from phandle\n");
			return -ENOENT;
		}

		votf_target->dev = get_device(&pdev_of_phandle->dev);
		ret = devm_add_action_or_reset(sc->dev, sc_put_device_wrapper,
					       votf_target->dev);
		if (ret)
			return ret;

		votf_target->regs = devm_ioremap(sc->dev,
						 pa.args[0], pa.args[1]);
		if (!votf_target->regs) {
			dev_err(sc->dev, "fail to ioremap vOTF_target\n");
			return -ENOMEM;
		}

		votf_target->votf_base_pa = pa.args[2];
	}

	sc->votf_table_count = count;
	return 0;
}

static int sc_init_tws_index(struct sc_dev *sc, unsigned int index)
{
	struct sc_tws *tws;

	tws = devm_kzalloc(sc->dev, sizeof(*tws), GFP_KERNEL);
	if (!tws) {
		dev_err(sc->dev, "no memory for tws index(%d)\n", index);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&tws->node);

	/* not need to use lock, because this list is not used yet */
	list_add_tail(&tws->node, &sc->tws_avail_list);

	INIT_DELAYED_WORK(&tws->tws_work, sc_clear_tws_work);

	tws->idx = index;
	tws->sc_dev = sc;

	return 0;
}

static int sc_get_votf_of_dt(struct sc_dev *sc)
{
	int ret_mscl, ret_sink;
	unsigned int i;
	int ret;

	ret_mscl = sc_get_mscl_votf_of_dt(sc);
	if (ret_mscl != 0 && ret_mscl != SC_VOTF_NOT_SUPPORTED)
		return ret_mscl;

	ret_sink = sc_get_sink_votf_of_dt(sc);
	if (ret_sink != 0 && ret_sink != SC_VOTF_NOT_SUPPORTED)
		return ret_sink;

	if (ret_mscl != ret_sink)
		return -EINVAL;

	spin_lock_init(&sc->tws_lock);
	INIT_LIST_HEAD(&sc->tws_avail_list);

	for (i = 0; i < SC_NUM_TWS_IDX; i++) {
		ret = sc_init_tws_index(sc, i);
		if (ret) {
			dev_err(sc->dev, "failed to init tws(%d)\n", i);
			return ret;
		}
	}

	dev_info(sc->dev, "vOTF of MSCL is supported\n");
	return 0;
}

static int sc_populate_dt(struct sc_dev *sc)
{
	int ret;

	ret = sc_get_pm_qos_from_dt(sc);
	if (ret)
		return ret;

	ret = sc_get_votf_of_dt(sc);
	if (ret)
		return ret;

	return 0;
}

#if IS_ENABLED(CONFIG_EXYNOS_ITMON)
static bool sc_itmon_check(struct sc_dev *sc, char *str_itmon, char *str_attr)
{
	const char *name = NULL;

	if (!str_itmon)
		return false;

	of_property_read_string(sc->dev->of_node, str_attr, &name);
	if (!name)
		return false;

	if (strncmp(str_itmon, name, strlen(name)) == 0)
		return true;

	return false;
}

static int sc_itmon_notifier(struct notifier_block *nb,
			     unsigned long action, void *nb_data)
{
	struct sc_dev *sc = container_of(nb, struct sc_dev, itmon_nb);
	struct itmon_notifier *itmon_info = nb_data;
	static int called_count;

	if (called_count != 0) {
		dev_info(sc->dev, "%s is called %d times, ignore it.\n",
			 __func__, called_count);
		return NOTIFY_DONE;
	}

	if (sc_itmon_check(sc, itmon_info->master, "itmon,master")) {
		if (test_bit(DEV_RUN, &sc->state)) {
			if (sc->current_ctx)
				sc_ctx_dump(sc->current_ctx);
			sc_hwregs_dump(sc);
		} else {
			dev_info(sc->dev, "MSCL is not running!\n");
		}
		called_count++;
	} else if (sc_itmon_check(sc, itmon_info->dest, "itmon,dest")) {
		if (test_bit(DEV_RUN, &sc->state)) {
			if (sc->current_ctx)
				sc_ctx_dump(sc->current_ctx);
			if (itmon_info->onoff)
				sc_hwregs_dump(sc);
			else
				dev_info(sc->dev, "MSCL power is disabled!\n");
		} else {
			dev_info(sc->dev, "MSCL is not running!\n");
		}
		called_count++;
	}

	return NOTIFY_DONE;
}
#endif

static int sc_probe(struct platform_device *pdev)
{
	struct sc_dev *sc;
	struct resource *res;
	int ret = 0;
	size_t ivar;
	u32 hwver;
	int irq_num;

	sc = devm_kzalloc(&pdev->dev, sizeof(struct sc_dev), GFP_KERNEL);
	if (!sc) {
		dev_err(&pdev->dev, "no memory for scaler device\n");
		return -ENOMEM;
	}

	sc->dev = &pdev->dev;

	spin_lock_init(&sc->ctxlist_lock);
	INIT_LIST_HEAD(&sc->context_list);
	spin_lock_init(&sc->slock);
	mutex_init(&sc->lock);
	init_waitqueue_head(&sc->wait);
	spin_lock_init(&sc->ctxlist_qos_lock);
	INIT_LIST_HEAD(&sc->ctxlist_qos);

	sc->fence_context = dma_fence_context_alloc(1);
	spin_lock_init(&sc->fence_lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sc->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(sc->regs))
		return PTR_ERR(sc->regs);

	irq_num = platform_get_irq(pdev, 0);
	if (irq_num < 0) {
		dev_err(&pdev->dev, "failed to get IRQ resource\n");
		return -ENOENT;
	}

	ret = devm_request_threaded_irq(&pdev->dev, irq_num,
					sc_irq_handler, NULL,
					0, pdev->name, sc);
	if (ret) {
		dev_err(&pdev->dev, "failed to install irq\n");
		return ret;
	}

	dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));

	atomic_set(&sc->wdt.cnt, 0);
	timer_setup(&sc->wdt.timer, sc_watchdog, 0);

	ret = sc_clk_get(sc);
	if (ret)
		return ret;

	if (pdev->dev.of_node)
		sc->dev_id = of_alias_get_id(pdev->dev.of_node, "scaler");
	else
		sc->dev_id = pdev->id;

	sc->xdev = create_scaler_ext_device(&pdev->dev);
	if (IS_ERR(sc->xdev)) {
		dev_err(&pdev->dev, "%s: Failed to create scaler-ext device\n",
			__func__);
		ret = PTR_ERR(sc->xdev);
		goto err_m2m;
	}

	platform_set_drvdata(pdev, sc);

#if IS_ENABLED(CONFIG_SOC_S5E9935)
	if (exynos_soc_info.main_rev == 0) {
	    sc->idx_idle_ip =
			exynos_get_idle_ip_index(dev_name(&pdev->dev), 1);
	    if (sc->idx_idle_ip < 0) {
		    ret = sc->idx_idle_ip;
		    goto err_ext;
	    }
    } else {
	    sc->idx_idle_ip = -1;
    }
#endif

	pm_runtime_enable(&pdev->dev);

	ret = sc_populate_dt(sc);
	if (ret)
		return ret;

#if defined(CONFIG_EXYNOS_BTS) || defined(CONFIG_EXYNOS_BTS_MODULE)
	sc->bts_id = bts_get_bwindex("MSCL");
#else
	sc->bts_id = -1;
#endif

	ret = sc_register_m2m_device(sc, sc->dev_id);
	if (ret) {
		dev_err(&pdev->dev, "failed to register m2m device\n");
		goto err_ext;
	}

	if (of_property_read_u32(pdev->dev.of_node, "mscl,cfw",
				(u32 *)&sc->cfw))
		sc->cfw = 0;

	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: failed to local power on (err %d)\n",
			__func__, ret);
		goto err_ver_rpm_get;
	}

	if (!IS_ERR(sc->pclk)) {
		ret = clk_prepare_enable(sc->pclk);
		if (ret) {
			dev_err(&pdev->dev,
				"%s: failed to enable PCLK (err %d)\n",
				__func__, ret);
			goto err_ver_pclk_get;
		}
	}

	if (!IS_ERR(sc->aclk)) {
		ret = clk_prepare_enable(sc->aclk);
		if (ret) {
			dev_err(&pdev->dev,
				"%s: failed to enable ACLK (err %d)\n",
				__func__, ret);
			goto err_ver_aclk_get;
		}
	}

	sc->version = SCALER_VERSION(2, 0, 0);

	hwver = __raw_readl(sc->regs + SCALER_VER);

	/* selects the lowest version number if no version is matched */
	for (ivar = 0; ivar < ARRAY_SIZE(sc_version_table); ivar++) {
		sc->version = sc_version_table[ivar][1];
		if (hwver == sc_version_table[ivar][0])
			break;
	}

	for (ivar = 0; ivar < ARRAY_SIZE(sc_variant); ivar++) {
		if (sc->version >= sc_variant[ivar].version) {
			sc->variant = &sc_variant[ivar];
			break;
		}
	}

	if (!IS_ERR(sc->aclk))
		clk_disable_unprepare(sc->aclk);
	if (!IS_ERR(sc->pclk))
		clk_disable_unprepare(sc->pclk);
	pm_runtime_put(&pdev->dev);

	iommu_register_device_fault_handler(&pdev->dev,
					    sc_sysmmu_fault_handler, sc);

#if IS_ENABLED(CONFIG_EXYNOS_ITMON)
	sc->itmon_nb.notifier_call = sc_itmon_notifier;
	itmon_notifier_chain_register(&sc->itmon_nb);
#endif

	dev_info(&pdev->dev,
		"Driver probed successfully(version: %08x(%x))\n",
		hwver, sc->version);

	return 0;

err_ver_aclk_get:
	if (!IS_ERR(sc->pclk))
		clk_disable_unprepare(sc->pclk);
err_ver_pclk_get:
	pm_runtime_put(&pdev->dev);
err_ver_rpm_get:
	sc_unregister_m2m_device(sc);
err_ext:
	destroy_scaler_ext_device(sc->xdev);
err_m2m:

	return ret;
}

static int sc_remove(struct platform_device *pdev)
{
	struct sc_dev *sc = platform_get_drvdata(pdev);

	if (timer_pending(&sc->wdt.timer))
		del_timer(&sc->wdt.timer);

	destroy_scaler_ext_device(sc->xdev);
	sc_unregister_m2m_device(sc);

	iommu_unregister_device_fault_handler(&pdev->dev);

	return 0;
}

static void sc_shutdown(struct platform_device *pdev)
{
	struct sc_dev *sc = platform_get_drvdata(pdev);

	set_bit(DEV_SUSPEND, &sc->state);

	wait_event(sc->wait,
			!test_bit(DEV_RUN, &sc->state));
}

static const struct of_device_id exynos_sc_match[] = {
	{
		.compatible = "samsung,exynos5-scaler",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_sc_match);

static struct platform_driver sc_driver = {
	.probe		= sc_probe,
	.remove		= sc_remove,
	.shutdown	= sc_shutdown,
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &sc_pm_ops,
		.of_match_table = of_match_ptr(exynos_sc_match),
	}
};

module_platform_driver(sc_driver);

MODULE_AUTHOR("Sunyoung, Kang <sy0816.kang@samsung.com>");
MODULE_DESCRIPTION("EXYNOS m2m scaler driver");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_LICENSE("GPL");
