/*
 * drivers/media/platform/exynos/mfc/mfc_common.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_COMMON_H
#define __MFC_COMMON_H __FILE__

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sched/clock.h>
#include <media/videobuf2-dma-sg.h>
#include <asm/cacheflush.h>
#include <soc/samsung/exynos/debug-snapshot.h>

#include "mfc_data_struct.h"
#include "mfc_regs.h"
#include "mfc_macros.h"
#include "mfc_debug.h"
#include "mfc_media.h"

#define MFC_DRIVER_INFO		230330

#define MFC_MAX_REF_BUFS	2
#define MFC_FRAME_PLANES	2
#define MFC_INFO_INIT_FD	-1

/* MFC video node */
#define EXYNOS_VIDEONODE_MFC_DEC               6
#define EXYNOS_VIDEONODE_MFC_ENC               7
#define EXYNOS_VIDEONODE_MFC_DEC_DRM           8
#define EXYNOS_VIDEONODE_MFC_ENC_DRM           9
#define EXYNOS_VIDEONODE_MFC_ENC_OTF           10
#define EXYNOS_VIDEONODE_MFC_ENC_OTF_DRM       11

/* Core information */
#define MFC_DEC_DEFAULT_CORE	0
#define MFC_ENC_DEFAULT_CORE	0
#define MFC_SURPLUS_CORE	1

/* Interrupt timeout */
#define MFC_INT_TIMEOUT		4000
/* Interrupt short timeout */
#define MFC_INT_SHORT_TIMEOUT	800
/* hwlock timeout */
#define MFC_HWLOCK_TIMEOUT	12000
/* Busy wait timeout */
#define MFC_BW_TIMEOUT		500
/* MMCache invalidation timeout */
#define MMCACHE_INVAL_TIMEOUT	1000
/* Interrupt timeout count*/
#define MFC_INT_TIMEOUT_CNT	3
/* The initial skip frame for boost */
#define MFC_INITIAL_SKIP_FRAME	60
/* The MAX I frame interval for boosting is 2000ms(2sec) */
#define MFC_BOOST_TIME			(2000)
/* The boost mode is maintained at least 20msec */
#define MFC_BOOST_OFF_TIME		((MFC_BOOST_TIME * NSEC_PER_MSEC) - (20 * NSEC_PER_MSEC))
/* The boost speed is multiplied by the framerate */
#define MFC_BOOST_SPEED		(8)

#define MFC_DEFAULT_RUNTIME	(33000)

/*
 * This value guarantees 299.4msec ~ 2.25sec according to MFC clock (668MHz ~ 89MHz)
 * releated with MFC_REG_TIMEOUT_VALUE
 */
#define MFC_TIMEOUT_VALUE	200000000
/* 250ms is the mfc firmware timeout value*/
#define MFC_TIMEOUT_VALUE_IN_MSEC 250

#define NUM_MPEG4_LF_BUF	2

#define FRAME_RATE_RESOLUTION	10000

#define DEFAULT_TAG		(0xE05)
#define IGNORE_TAG		(0xD5C) /* ex) encoder DRC */
#define HEADER_TAG		(0xC5D)
#define UNUSED_TAG		(-1)

#define MFC_NO_INSTANCE_SET	-1

#define MFC_ENC_CAP_PLANE_COUNT	1
#define MFC_ENC_OUT_PLANE_COUNT	2

#define MFC_NAME_LEN		16
#define MFC_FW_NAME		"mfc_fw.bin"

#define STUFF_BYTE		4
#define MFC_EXTRA_DPB		5
#define MFC_ALL_AVAILABLE_DPB	0xffffffffffffffff

#define MFC_BASE_MASK		((1 << 17) - 1)

/* Error & Warning */
#define mfc_get_err(x)		(((x) >> MFC_REG_ERR_STATUS_SHIFT)	\
						& MFC_REG_ERR_STATUS_MASK)
#define mfc_get_warn(x)		(((x) >> MFC_REG_WARN_STATUS_SHIFT)	\
						& MFC_REG_WARN_STATUS_MASK)

/* MFC conceal color is black */
#define MFC_CONCEAL_COLOR	0x8020000

#define vb_to_mfc_buf(x)		\
	container_of(x, struct mfc_buf, vb.vb2_buf)

#define fh_to_mfc_ctx(x)		\
	container_of(x, struct mfc_ctx, fh)

#define call_bop(ctx, op, args...)	(((ctx)->b_ops->op) ? ((ctx)->b_ops->op(args)) : 0)
#define call_cop(ctx, op, args...)	(((ctx)->c_ops->op) ? ((ctx)->c_ops->op(args)) : 0)
#define call_dop(d, op, args...)	(((d)->dump_ops->op) ? ((d)->dump_ops->op(args)) : 0)

#define	MFC_CTRL_TYPE_GET	(MFC_CTRL_TYPE_GET_SRC | MFC_CTRL_TYPE_GET_DST)
#define	MFC_CTRL_TYPE_SET	(MFC_CTRL_TYPE_SET_SRC | MFC_CTRL_TYPE_SET_DST)
#define	MFC_CTRL_TYPE_SRC	(MFC_CTRL_TYPE_SET_SRC | MFC_CTRL_TYPE_GET_SRC)
#define	MFC_CTRL_TYPE_DST	(MFC_CTRL_TYPE_SET_DST | MFC_CTRL_TYPE_GET_DST)

#define MFC_FMT_STREAM		(1 << 0)
#define MFC_FMT_FRAME		(1 << 1)
#define MFC_FMT_10BIT		(1 << 2)
#define MFC_FMT_422		(1 << 3)
#define MFC_FMT_RGB		(1 << 4)
#define MFC_FMT_SBWC		(1 << 5)
#define MFC_FMT_SBWCL		(1 << 6)
#define MFC_FMT_DEC		(1 << 7)
#define MFC_FMT_ENC		(1 << 8)
#define MFC_FMT_SBWCLH		(1 << 9)
#define MFC_FMT_SBWCLHFR	(1 << 10)

/* node check */
#define IS_DEC_NODE(n)		((n == EXYNOS_VIDEONODE_MFC_DEC) ||	\
				(n == EXYNOS_VIDEONODE_MFC_DEC_DRM))
#define IS_ENC_NODE(n)		((n == EXYNOS_VIDEONODE_MFC_ENC) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_DRM) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_OTF) ||	\
				(n == EXYNOS_VIDEONODE_MFC_ENC_OTF_DRM))

/* Decoder codec mode check */
#define IS_H264_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_H264_DEC)
#define IS_H264_MVC_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_H264_MVC_DEC)
#define IS_MPEG4_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_MPEG4_DEC)
#define IS_FIMV1_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_FIMV1_DEC)
#define IS_FIMV2_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_FIMV2_DEC)
#define IS_FIMV3_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_FIMV3_DEC)
#define IS_FIMV4_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_FIMV4_DEC)
#define IS_VC1_DEC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VC1_DEC)
#define IS_VC1_RCV_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_VC1_RCV_DEC)
#define IS_MPEG2_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_MPEG2_DEC)
#define IS_HEVC_DEC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_HEVC_DEC)
#define IS_VP8_DEC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VP8_DEC)
#define IS_VP9_DEC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VP9_DEC)
#define IS_AV1_DEC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_AV1_DEC)
#define IS_MV_HEVC_DEC(ctx, profile)	((ctx)->codec_mode == MFC_REG_CODEC_HEVC_DEC &&	\
				profile == MFC_REG_D_PROFILE_MULTIVIEW_HEVC_MAIN)

/* Encoder codec mode check */
#define IS_H264_ENC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_H264_ENC)
#define IS_MPEG4_ENC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_MPEG4_ENC)
#define IS_H263_ENC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_H263_ENC)
#define IS_VP8_ENC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VP8_ENC)
#define IS_HEVC_ENC(ctx)	((ctx)->codec_mode == MFC_REG_CODEC_HEVC_ENC)
#define IS_VP9_ENC(ctx)		((ctx)->codec_mode == MFC_REG_CODEC_VP9_ENC)

#define CODEC_NOT_CODED(ctx)	(IS_MPEG4_DEC(ctx) || IS_VC1_DEC(ctx) ||	\
				IS_VC1_RCV_DEC(ctx) || IS_VP9_DEC(ctx))
#define CODEC_INTERLACED(ctx)	(IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) ||	\
				IS_MPEG2_DEC(ctx) || IS_MPEG4_DEC(ctx) ||	\
				IS_VC1_DEC(ctx) || IS_VC1_RCV_DEC(ctx))
#define CODEC_MBAFF(ctx)	(IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx))
#define CODEC_MULTIFRAME(ctx)	(IS_MPEG4_DEC(ctx) || IS_VP9_DEC(ctx) ||	\
				IS_FIMV2_DEC(ctx) || IS_FIMV3_DEC(ctx) || IS_FIMV4_DEC(ctx) || IS_AV1_DEC(ctx))
#define CODEC_10BIT_DEC(ctx)	(IS_HEVC_DEC(ctx) || IS_VP9_DEC(ctx) ||	IS_AV1_DEC(ctx))
#define CODEC_10BIT_ENC(ctx)	(IS_HEVC_ENC(ctx) || IS_VP9_ENC(ctx))
#define CODEC_422FORMAT(ctx)	(IS_HEVC_DEC(ctx) || IS_HEVC_ENC(ctx) ||	\
				IS_VP9_DEC(ctx) || IS_VP9_ENC(ctx))
#define ON_RES_CHANGE(ctx)	(((ctx)->state >= MFCINST_RES_CHANGE_INIT) &&	\
				 ((ctx)->state <= MFCINST_RES_CHANGE_END))
#define IS_NO_ERROR(err)	((err) == 0 ||		\
				(mfc_get_warn(err)	\
				 == MFC_REG_ERR_SYNC_POINT_NOT_RECEIVED))
#define CODEC_HAS_IDR(ctx)	(IS_H264_DEC(ctx) || IS_H264_MVC_DEC(ctx) || IS_HEVC_DEC(ctx) ||  \
				IS_H264_ENC(ctx) || IS_HEVC_ENC(ctx))

// Buffer container
#define IS_BUFFER_BATCH_MODE(ctx)	((ctx)->batch_mode == 1)
#define IS_NO_HEADER_GENERATE(ctx, p)			\
	((p->seq_hdr_mode == V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME) || \
	((IS_VP8_ENC(ctx) || IS_VP9_ENC(ctx)) && p->ivf_header_disable))

#define HDR10_PLUS_ADDR(base, idx)	(base + ((idx) * HDR10_PLUS_DATA_SIZE))

/*
 levels with maximum property values
 level	Maximum frame size in MB (MB width x MB height)
 LV5		22,080
 LV5.1		36,864
 LV6		139,264
 */
#define LV51_MB_MIN		(22080)
#define LV51_MB_MAX		(36864)
#define LV60_MB_MAX		(139264)

#define IS_LV51_MB(mb)		(((mb) > LV51_MB_MIN) && ((mb) <= LV51_MB_MAX))
#define IS_LV60_MB(mb)		(((mb) > LV51_MB_MAX) && ((mb) <= LV60_MB_MAX))

static struct mfc_resolution mfc_res[] = {
	{
		.width = 0,
		.height = 0,
	},
	{
		.width = 1920,
		.height = 1088,
	},
	{
		.width = 4096,
		.height = 2176,
	},
	{
		.width = 8192,
		.height = 4352,
	},
};

/* 8K resolution */
#define MFC_8K_RES		(7680 * 4320)
#define IS_8K_RES(ctx)		(((ctx)->crop_width * (ctx)->crop_height) >= MFC_8K_RES)
/* For max h/w performance */
#define IS_8K_PERF(ctx)		(((ctx)->crop_width * (ctx)->crop_height) >= (MFC_8K_RES / 2))
#define UNDER_8K_RES(ctx)	(((ctx)->crop_width * (ctx)->crop_height) < MFC_8K_RES)

/* 4K resolution */
/* VP9 needs 64 alignment because of min MB(64x64) */
#define MFC_4K_RES		(4096 * 2160)
#define MFC_4K_RES_64ALIGN	(4096 * 2176)
#define UNDER_4K_RES(ctx)	(((ctx)->crop_width * (ctx)->crop_height) < MFC_4K_RES_64ALIGN)
#define IS_MULTI_MODE_RES(ctx)	(((ctx)->img_width * (ctx)->img_height) >= (2 * MFC_4K_RES))

/* UHD resolution */
#define MFC_UHD_RES		(3840 * 2160)
#define OVER_UHD_RES(ctx)	(((ctx)->crop_width * (ctx)->crop_height) >= MFC_UHD_RES)
#define UNDER_UHD_RES(ctx)	(((ctx)->crop_width * (ctx)->crop_height) <= MFC_4K_RES_64ALIGN)

/* FHD resolution */
#define MFC_FHD_RES		(1920 * 1088)
#define MFC_1080P_RES		(1920 * 1080)
#define MFC_FHD_RES_MB		(((1920 + 15) / 16) * ((1088 + 15) / 16))
#define UNDER_FHD_RES(ctx)	(((ctx)->crop_width * (ctx)->crop_height) <= MFC_FHD_RES)
#define UNDER_1080P_RES(ctx)	(((ctx)->crop_width * (ctx)->crop_height) < MFC_1080P_RES)

/* HD resolution */
#define MFC_HD_RES_MB		(((1280 + 15) / 16) * ((720 + 15) / 16))

/*
 * If it is a resolution that requires max performance,
 * the performance degradation option is not available.
 * (2-ref, B frame)
 */
#define IS_MFC_MAX_PERF(ctx, fps)	(IS_8K_PERF(ctx) || (OVER_UHD_RES(ctx) && (fps > 60)))
#define IS_MFC_HEAVY_PERF(ctx, fps)	(IS_8K_PERF(ctx) || (OVER_UHD_RES(ctx) && (fps >= 60)))

#define IS_MULTI_MODE_ENC_RES(ctx)	(IS_MULTI_MODE_RES(ctx) || \
					 (OVER_UHD_RES(ctx) && (ctx)->operating_framerate >= 60000) || \
					 (OVER_UHD_RES(ctx) && mfc_is_enc_bframe(ctx)))

#define IS_MULTI_MODE_ENC_CONDITION(ctx)	((IS_HEVC_ENC(ctx) && IS_MULTI_MODE_ENC_RES(ctx)))

#define IS_BLACKBAR_OFF(ctx)	((ctx)->crop_height > 2160)
#define IS_SUPER64_BFRAME(ctx, size, type)	((ctx->is_10bit) && (size >= 2) && (type == 3))

#define IS_SINGLE_FD(ctx, fmt)	((!(ctx)->rgb_bpp) && ((fmt)->mem_planes == 1))
#define IS_SBWC_8B(fmt)		((((fmt)->fourcc) == V4L2_PIX_FMT_NV12M_SBWC_8B) ||	\
				(((fmt)->fourcc) == V4L2_PIX_FMT_NV21M_SBWC_8B) || \
				(((fmt)->fourcc) == V4L2_PIX_FMT_NV12N_SBWC_8B) || \
				(((fmt)->fourcc) == V4L2_PIX_FMT_NV12N_SBWC_256_8B))
#define IS_SBWC_10B(fmt)	((((fmt)->fourcc) == V4L2_PIX_FMT_NV12M_SBWC_10B) || \
				(((fmt)->fourcc) == V4L2_PIX_FMT_NV21M_SBWC_10B) || \
				(((fmt)->fourcc) == V4L2_PIX_FMT_NV12N_SBWC_10B) || \
				(((fmt)->fourcc) == V4L2_PIX_FMT_NV12N_SBWC_256_10B))
#define IS_SBWC_256ALIGN(fmt)	((((fmt)->fourcc) == V4L2_PIX_FMT_NV12N_SBWC_256_8B) || \
				(((fmt)->fourcc) == V4L2_PIX_FMT_NV12N_SBWC_256_10B))
#define IS_SBWC_FMT(fmt)	(IS_SBWC_8B(fmt) || IS_SBWC_10B(fmt))

/* 2bit needs S10B YUV 10bit, SBWC and SBWC Lossy header format */
#define IS_2BIT_NEED(ctx)	(((ctx)->is_10bit && !(ctx)->mem_type_10bit &&	\
				!(ctx)->is_sbwc_lossy) || (ctx)->is_sbwc ||	\
				((ctx)->is_sbwc_lossy == MFC_SBWCL_HEADER) ||	\
				((ctx)->is_sbwc_lossy == MFC_SBWCL_HEADER_FR))
#define IS_SBWC_DPB(ctx)	((ctx)->is_sbwc || (ctx)->is_sbwc_lossy ||	\
				((ctx)->enc_priv->sbwc_option == 2))

#define IS_MULTI_CORE_DEVICE(dev)	((dev)->num_core > 1)
#define IS_SINGLE_MODE(ctx)	   ((ctx)->op_mode == MFC_OP_SINGLE)
#define IS_TWO_MODE1(ctx)	   ((ctx)->op_mode == MFC_OP_TWO_MODE1)
#define IS_TWO_MODE2(ctx)	   ((ctx)->op_mode == MFC_OP_TWO_MODE2)
#define IS_MULTI_MODE(ctx)	   (((ctx)->op_mode == MFC_OP_TWO_MODE1) || \
					((ctx)->op_mode == MFC_OP_TWO_MODE2))
#define IS_SWITCH_SINGLE_MODE(ctx) (((ctx)->op_mode == MFC_OP_SWITCH_TO_SINGLE) || \
					((ctx)->op_mode == MFC_OP_SWITCH_BUT_MODE2))
#define IS_MODE_SWITCHING(ctx)	   ((ctx)->op_mode == MFC_OP_SWITCHING)
#define IS_HDR10(ctx, p, t, m)									\
	((ctx)->is_10bit &&									\
	(p == MFC_PRIMARIES_BT2020) &&								\
	((m == MFC_MATRIX_COEFF_BT2020) || (m == MFC_MATRIX_COEFF_BT2020_CONSTANT)) &&		\
	((t == MFC_TRANSFER_SMPTE_170M) || (t == MFC_TRANSFER_ST2084) || (t == MFC_TRANSFER_HLG)))

#define IS_NO_INFOLOG(ctx)		((ctx)->dec_priv && (ctx)->dec_priv->is_multiframe)

/* Extra information for Decoder */
#define	DEC_SET_DUAL_DPB		(1 << 0)
#define	DEC_SET_DYNAMIC_DPB		(1 << 1)
#define	DEC_SET_LAST_FRAME_INFO		(1 << 2)
#define	DEC_SET_SKYPE_FLAG		(1 << 3)
#define	DEC_SET_HDR10_PLUS		(1 << 4)
/* new C2_INTERFACE: DISPLAY_DELAY, FRAME_POC */
#define	DEC_SET_C2_INTERFACE		(1 << 6)
#define DEC_SET_FRAME_ERR_TYPE		(1 << 7)
#define DEC_SET_OPERATING_FPS		(1 << 8)
#define DEC_SET_HDR10_PLUS_FULL		(1 << 9)
#define DEC_SET_BUF_FLAG_CTRL		(1 << 16)
#define DEC_SET_PRIORITY		(1 << 23)

/* Extra information for Encoder */
#define	ENC_SET_RGB_INPUT		(1 << 0)
#define	ENC_SET_SPARE_SIZE		(1 << 1)
#define	ENC_SET_TEMP_SVC_CH		(1 << 2)
#define	ENC_SET_SKYPE_FLAG		(1 << 3)
#define	ENC_SET_ROI_CONTROL		(1 << 4)
#define	ENC_SET_QP_BOUND_PB		(1 << 5)
#define	ENC_SET_FIXED_SLICE		(1 << 6)
#define	ENC_SET_PVC_MODE		(1 << 7)
#define	ENC_SET_RATIO_OF_INTRA		(1 << 8)
#define	ENC_SET_COLOR_ASPECT		(1 << 9)
#define	ENC_SET_HP_BITRATE_CONTROL	(1 << 10)
#define	ENC_SET_STATIC_INFO		(1 << 11)
#define	ENC_SET_HDR10_PLUS		(1 << 12)
#define	ENC_SET_VP9_PROFILE_LEVEL	(1 << 13)
#define	ENC_SET_DROP_CONTROL		(1 << 14)
#define	ENC_SET_CHROMA_QP_CONTROL	(1 << 15)
#define ENC_SET_BUF_FLAG_CTRL		(1 << 16)
#define ENC_SET_GDC_VOTF		(1 << 17)
#define ENC_SET_OPERATING_FPS		(1 << 18)
#define ENC_SET_AVERAGE_QP		(1 << 19)
#define ENC_SET_MV_SEARCH_MODE		(1 << 20)
#define ENC_SET_GOP_CTRL		(1 << 21)
#define	ENC_SET_HDR10_PLUS_STAT_INFO	(1 << 22)
#define ENC_SET_PRIORITY		(1 << 23)
#define ENC_SET_CAPABILITY		(1 << 24)
#define ENC_SET_IFRAME_SIZE		(1 << 25)
#define ENC_SET_TIMING_INFO_ENABLE	(1 << 26)

#define MFC_FEATURE_SUPPORT(dev, f)	((f).support && ((dev)->fw_date >= (f).version))

/* Low memory check */
#define IS_LOW_MEM			(totalram_pages() <= ((SZ_1G + SZ_512M) >> PAGE_SHIFT))
#define SZ_600M				(6 * 1024 * 1024)

static inline int mfc_core_get_pwr_ref_cnt(struct mfc_core *core)
{
	return atomic_read(&core->pm.pwr_ref);
}

static inline int mfc_core_get_clk_ref_cnt(struct mfc_core *core)
{
	if (core->dev->pdata->support_hwacg == MFC_HWACG_HWFW_CTRL)
		return 1;

	return atomic_read(&core->clk_ref);
}

#endif /* __MFC_COMMON_H */
