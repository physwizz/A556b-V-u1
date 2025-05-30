/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_HW_MCSC_H_V5
#define IS_HW_MCSC_H_V5

#include "is-hw.h"
#include "is-interface-library.h"
#include "is-param.h"
#include "is-hw-common-dma.h"
#include "is-votf-id-table.h"
#include "pablo-hw-api-common-ctrl.h"
#include "pablo-internal-subdev-ctrl.h"

#define MCSC_ROUND_UP(x, d) \
	((d) * (((x) + ((d) - 1)) / (d)))

#define GET_MCSC_HW_CAP(hwip)                                                                      \
	((hwip->priv_info) ? &((struct pablo_hw_mcsc *)hw_ip->priv_info)->cap : NULL)
#define GET_ENTRY_FROM_OUTPUT_ID(output_id) \
	(output_id + ENTRY_M0P)
#define GET_DJAG_ZOOM_RATIO(in, out) (u32)(((in * 1000 / out) << MCSC_PRECISION) / 1000)

#define GET_CORE_NUM(id)	((id) - DEV_HW_MCSC0)

/* NI from DDK needs to adjust scale factor (by multipling 10) */
#define MULTIPLIED_10(value)		(10 * (value))
#define INTRPL_SHFT_VAL			(12)
#define SUB(a, b)		(int)(((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#define LSHFT(a)		((int)((a) << INTRPL_SHFT_VAL))
#define RSHFT(a)		((int)((a) >> INTRPL_SHFT_VAL))
#define NUMERATOR(Y1, Y2, DXn)			(((Y2) - (Y1)) * (DXn))
#define CALC_LNR_INTRPL(Y1, Y2, X1, X2, X)	(LSHFT(NUMERATOR(Y1, Y2, SUB(X, X1))) / SUB(X2, X1) + LSHFT(Y1))
#define GET_LNR_INTRPL(Y1, Y2, X1, X2, X)	RSHFT(SUB(X2, X1) ? CALC_LNR_INTRPL(Y1, Y2, X1, X2, X) : LSHFT(Y1))

/* This should be same with CAC_MAX_NI_DEPENDED_CFG */
#define HF_MAX_NI_DEPENDED_CFG 12
#define RDMAHF_LLC_CACHE_HINT  IS_LLC_CACHE_HINT_LAST_ACCESS

#define MCSC_DJAG_PRESCALE_INDEX_1 0 /* x1.0 */
#define MCSC_DJAG_PRESCALE_INDEX_2 1 /* x1.1~x1.4 */
#define MCSC_DJAG_PRESCALE_INDEX_3 2 /* x1.5~x2.0 */
#define MCSC_DJAG_PRESCALE_INDEX_4 3 /* x2.1~ */

#define MAX_SCALINGRATIOINDEX_DEPENDED_CONFIGS (4)
#define MAX_DITHER_VALUE_CONFIGS (9)

enum mcsc_img_format {
	MCSC_YUV422_1P_YUYV = 0,
	MCSC_YUV422_1P_YVYU,
	MCSC_YUV422_1P_UYVY,
	MCSC_YUV422_1P_VYUY,
	MCSC_YUV422_2P_UFIRST,
	MCSC_YUV422_2P_VFIRST,
	MCSC_YUV422_3P,
	MCSC_YUV420_2P_UFIRST,
	MCSC_YUV420_2P_VFIRST,
	MCSC_YUV420_3P,
	MCSC_YUV422_2P_UFIRST_P210 = 16,
	MCSC_YUV422_2P_VFIRST_P210,
	MCSC_YUV422_2P_UFIRST_8P2,
	MCSC_YUV422_2P_VFIRST_8P2,
	MCSC_YUV422_2P_UFIRST_PACKED10,
	MCSC_YUV422_2P_VFIRST_PACKED10,
	MCSC_YUV420_2P_UFIRST_P010 = 32,
	MCSC_YUV420_2P_VFIRST_P010,
	MCSC_YUV420_2P_UFIRST_8P2,
	MCSC_YUV420_2P_VFIRST_8P2,
	MCSC_YUV420_2P_UFIRST_PACKED10,
	MCSC_YUV420_2P_VFIRST_PACKED10,
	MCSC_YUV444_1P = 48,
	MCSC_YUV444_3P,
	MCSC_YUV444_1P_PACKED10,
	MCSC_YUV444_3P_PACKED10,
	MCSC_YUV444_1P_UNPACKED,
	MCSC_YUV444_3P_UNPACKED,
	MCSC_RGB_RGBA8888 = 60,
	MCSC_RGB_ABGR8888,
	MCSC_RGB_ARGB8888,
	MCSC_RGB_BGRA8888,
	MCSC_RGB_RGBA1010102 = 67,
	MCSC_RGB_ABGR1010102,
	MCSC_MONO_Y8,
};

enum mcsc_io_type {
	HW_MCSC_OTF_INPUT,
	HW_MCSC_OTF_OUTPUT,
	HW_MCSC_DMA_INPUT,
	HW_MCSC_DMA_OUTPUT,
};

enum pablo_hw_mcsc_dbg_mode {
	MCSC_DBG_DUMP_REG = 0,
	MCSC_DBG_DUMP_REG_ONCE,
	MCSC_DBG_S2D_FS,
	MCSC_DBG_S2D_FE,
	MCSC_DBG_SKIP_RTA,
	MCSC_DBG_BYPASS,
	MCSC_DBG_DTP_OTF_IMG,
	MCSC_DBG_DTP_OTF_DL,
	MCSC_DBG_STRGEN,
	MCSC_DBG_CHECK_SIZE,
	MCSC_DBG_APB_DIRECT,
};

enum mcsc_stripe_region_type {
	MCSC_STRIPE_REGION_L,
	MCSC_STRIPE_REGION_M,
	MCSC_STRIPE_REGION_R,
};

struct ref_ni {
	u32 min;
	u32 max;
};

#define SUBBLK_TDNR	(0)
#define SUBBLK_CAC	(1)
#define SUBBLK_HF	(2)
#define SUBBLK_MAX	(3)
struct pablo_hw_mcsc_output {
	u32 output_id;
	struct list_head node;
};

struct pablo_hw_mcsc {
	struct is_mcsc_config config;
	struct is_hw_mcsc_cap cap;

	u32 in_img_format;
	u32 out_img_format[MCSC_OUTPUT_MAX];
	bool conv420_en[MCSC_OUTPUT_MAX];
	bool rep_flag[IS_STREAM_COUNT];
	bool tune_set[IS_STREAM_COUNT];
	int yuv_range;
	u32 instance;
	ulong out_en; /* This flag save whether the capture video node of MCSC is opened or not. */
	u32 prev_hwfc_output_ids;
	/* noise_index also needs to use in TDNR, HF, CAC */
	u32 cur_ni[SUBBLK_MAX];
	/*
	 * Support flip an image exceeding line buffer size
	 * flip_output_id[i] = n;
	 * i : output_id
	 * n : output_id of output param use for setting (i)port
	 */
	u32 flip_output_id[MCSC_OUTPUT_MAX];
	/* Left, Middle, Right */
	u32 stripe_region;

	struct is_common_dma *rdma;
	struct is_common_dma *wdma;
	u32 rdma_max_cnt;
	u32 wdma_max_cnt;

	struct pablo_internal_subdev subdev_cloader;
	struct pablo_common_ctrl *pcc;
};

int pablo_hw_mcsc_update_param(struct is_hw_ip *hw_ip, struct mcs_param *param, u32 instance,
	struct pablo_common_ctrl_frame_cfg *frame_cfg);
int pablo_hw_mcsc_reset(struct is_hw_ip *hw_ip, u32 instance);

int pablo_hw_mcsc_otf_input(struct is_hw_ip *hw_ip, struct param_control *control,
	struct param_mcs_input *input, u32 instance, struct pablo_common_ctrl_frame_cfg *frame_cfg);
int pablo_hw_mcsc_dma_input(struct is_hw_ip *hw_ip, struct param_mcs_input *input, u32 instance);
int pablo_hw_mcsc_poly_phase(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, struct param_stripe_input *stripe_input, u32 output_id,
	u32 instance);
int pablo_hw_mcsc_post_chain(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, struct param_stripe_input *stripe_input, u32 output_id,
	u32 instance);
int pablo_hw_mcsc_flip(
	struct is_hw_ip *hw_ip, struct param_mcs_output *output, u32 output_id, u32 instance);
int pablo_hw_mcsc_dma_output(
	struct is_hw_ip *hw_ip, struct param_mcs_output *output, u32 output_id, u32 instance);
int pablo_hw_mcsc_output_yuvrange(
	struct is_hw_ip *hw_ip, struct param_mcs_output *output, u32 output_id, u32 instance);
int pablo_hw_mcsc_hwfc_mode(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 hwfc_output_ids, u32 dma_output_ids, u32 instance);
int pablo_hw_mcsc_hwfc_output(
	struct is_hw_ip *hw_ip, struct param_mcs_output *output, u32 output_id, u32 instance);

int pablo_hw_mcsc_adjust_input_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format);
int pablo_hw_mcsc_adjust_output_img_fmt(
	u32 bitwidth, u32 format, u32 plane, u32 order, u32 *img_format, bool *conv420_flag);
int pablo_hw_mcsc_check_format(
	enum mcsc_io_type type, u32 format, u32 bit_width, u32 width, u32 height);
void is_hw_mcsc_adjust_size_with_djag(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_stripe_input *stripe_input,
	struct is_hw_mcsc_cap *cap, u32 crop_type, u32 *x, u32 *y, u32 *width, u32 *height);
int is_hw_mcsc_update_djag_register(struct is_hw_ip *hw_ip,
		struct mcs_param *param,
		u32 instance);
int is_hw_mcsc_update_cac_register(struct is_hw_ip *hw_ip,
	struct is_frame *frame, u32 instance);
void pablo_hw_mcsc_s_debug_type(int type);
void pablo_hw_mcsc_c_debug_type(int type);
const struct kernel_param *pablo_hw_mcsc_get_debug_kernel_param_kunit_wrapper(void);
void pablo_hw_mcsc_check_size(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, u32 instance, u32 output_id);
#endif
