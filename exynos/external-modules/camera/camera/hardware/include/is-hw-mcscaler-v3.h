/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_HW_MCSC_H
#define IS_HW_MCSC_H

#include "is-hw.h"
#include "is-hw-djag-v2.h"
#include "is-hw-cac.h"
#include "is-interface-library.h"
#include "is-param.h"

#define MCSC_ROUND_UP(x, d) \
	((d) * (((x) + ((d) - 1)) / (d)))

#define GET_MCSC_HW_CAP(hwip) \
	((hwip->priv_info) ? &((struct is_hw_mcsc *)hw_ip->priv_info)->cap : NULL)
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

enum mcsc_shadow_ctrl {
	SHADOW_WRITE_START = 0,
	SHADOW_WRITE_FINISH,
};

enum is_hw_mcsc_dbg_mode {
	MCSC_DBG_DUMP_REG = 0,
	MCSC_DBG_DUMP_REG_ONCE = 1,
	MCSC_DBG_S2D = 2,
	MCSC_DBG_SKIP_SETFILE = 3,
	MCSC_DBG_BYPASS = 4,
	MCSC_DBG_DTP_OTF = 5,
	MCSC_DBG_DTP_RDMA = 6,
	MCSC_DBG_STRGEN = 7,
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

struct scaler_setfile_contents {
	/* Brightness/Contrast control param */
	u32 y_offset;
	u32 y_gain;

	/* Hue/Saturation control param */
	u32 c_gain00;
	u32 c_gain01;
	u32 c_gain10;
	u32 c_gain11;
};

struct scaler_filter_h_coef_cfg {
	int h_coef_a[9];
	int h_coef_b[9];
	int h_coef_c[9];
	int h_coef_d[9];
	int h_coef_e[9];
	int h_coef_f[9];
	int h_coef_g[9];
	int h_coef_h[9];
};
struct scaler_filter_v_coef_cfg {
	int v_coef_a[9];
	int v_coef_b[9];
	int v_coef_c[9];
	int v_coef_d[9];
};
#ifdef USE_MCSC_H_V_COEFF
struct scaler_filter_coef_cfg {
	u32 ratio_x8_8;		/* x8/8, Ratio <= 1048576 (~8:8) */
	u32 ratio_x7_8;		/* x7/8, 1048576 < Ratio <= 1198373 (~7/8) */
	u32 ratio_x6_8;		/* x6/8, 1198373 < Ratio <= 1398101 (~6/8) */
	u32 ratio_x5_8;		/* x5/8, 1398101 < Ratio <= 1677722 (~5/8) */
	u32 ratio_x4_8;		/* x4/8, 1677722 < Ratio <= 2097152 (~4/8) */
	u32 ratio_x3_8;		/* x3/8, 2097152 < Ratio <= 2796203 (~3/8) */
	u32 ratio_x2_8;		/* x2/8, 2796203 < Ratio <= 4194304 (~2/8) */
};

struct scaler_coef_cfg {
	struct scaler_filter_coef_cfg poly_sc_coef[2]; /* [0] : horizontal [1] : vertical */
	struct scaler_filter_coef_cfg post_sc_coef[2]; /* [0] : horizontal [1] : vertical */
	bool use_poly_sc_coef;						/* 0 : default, 1 : use poly_sc_h_coef/poly_sc_v_coef */
	struct scaler_filter_h_coef_cfg poly_sc_h_coef;
	struct scaler_filter_v_coef_cfg poly_sc_v_coef;
};
#else
struct scaler_coef_cfg {
	u32 ratio_x8_8;		/* x8/8, Ratio <= 1048576 (~8:8) */
	u32 ratio_x7_8;		/* x7/8, 1048576 < Ratio <= 1198373 (~7/8) */
	u32 ratio_x6_8;		/* x6/8, 1198373 < Ratio <= 1398101 (~6/8) */
	u32 ratio_x5_8;		/* x5/8, 1398101 < Ratio <= 1677722 (~5/8) */
	u32 ratio_x4_8;		/* x4/8, 1677722 < Ratio <= 2097152 (~4/8) */
	u32 ratio_x3_8;		/* x3/8, 2097152 < Ratio <= 2796203 (~3/8) */
	u32 ratio_x2_8;		/* x2/8, 2796203 < Ratio <= 4194304 (~2/8) */
};
#endif
struct scaler_bchs_clamp_cfg {
	u32 y_max;
	u32 y_min;
	u32 c_max;
	u32 c_min;
};

struct hf_cfg_by_ni {
	u32 hf_weight;
};

struct hf_setfile_contents {
	u32	ni_max;
	u32	ni[HF_MAX_NI_DEPENDED_CFG];
	struct hf_cfg_by_ni	cfgs[HF_MAX_NI_DEPENDED_CFG];
};

struct hw_mcsc_setfile {
	u32 setfile_version;

	/* contents for Full/Narrow mode
	 * 0 : SCALER_OUTPUT_YUV_RANGE_FULL
	 * 1 : SCALER_OUTPUT_YUV_RANGE_NARROW
	 */
	struct scaler_setfile_contents	sc_base[2];
#ifdef MCSC_USE_DEJAG_TUNING_PARAM
	/* Setfile tuning parameters for DJAG (Lhotse)
	 * 0 : Scaling ratio = x1.0
	 * 1 : Scaling ratio = x1.1~x1.4
	 * 2 : Scaling ratio = x1.5~x2.0
	 * 3 : Scaling ratio = x2.1~
	 */
	struct djag_setfile_contents	djag;
#endif
	struct scaler_bchs_clamp_cfg	sc_bchs[2];	/* 0: YUV_FULL, 1: YUV_NARROW */
	struct scaler_coef_cfg		sc_coef;
	struct djag_wb_thres_cfg	djag_wb[MAX_SCALINGRATIOINDEX_DEPENDED_CONFIGS];
	struct cac_setfile_contents	cac;
	struct hf_setfile_contents	hf;
};

#define SUBBLK_TDNR	(0)
#define SUBBLK_CAC	(1)
#define SUBBLK_HF	(2)
#define SUBBLK_MAX	(3)
struct is_hw_mcsc_output {
	u32 output_id;
	struct list_head node;
};

struct is_hw_mcsc {
	struct hw_mcsc_setfile setfile[SENSOR_POSITION_MAX][IS_MAX_MCSC_SETFILE];
	struct	hw_mcsc_setfile *cur_setfile[SENSOR_POSITION_MAX][IS_STREAM_COUNT];
	struct	is_hw_mcsc_cap cap;

	u32	in_img_format;
	u32	out_img_format[MCSC_OUTPUT_MAX];
	bool	conv420_en[MCSC_OUTPUT_MAX];
	bool	rep_flag[IS_STREAM_COUNT];
	bool	tune_set[IS_STREAM_COUNT];
	int	yuv_range;
	u32	instance;
	ulong	out_en;		/* This flag save whether the capture video node of MCSC is opened or not. */
	u32	prev_hwfc_output_ids;
	/* noise_index also needs to use in TDNR, HF, CAC */
	u32			cur_ni[SUBBLK_MAX];
	/*
	 * Support flip an image exceeding line buffer size
	 * flip_output_id[i] = n;
	 * i : output_id
	 * n : output_id of output param use for setting (i)port
	 */
	u32 flip_output_id[MCSC_OUTPUT_MAX];
	u32 stripe_region;	/* Left, Middle, Right */
};

int is_hw_mcsc_update_param(struct is_hw_ip *hw_ip,
	struct mcs_param *param, u32 instance);
int is_hw_mcsc_reset(struct is_hw_ip *hw_ip, u32 instance);
int is_hw_mcsc_clear_interrupt(struct is_hw_ip *hw_ip, u32 instance);

int is_hw_mcsc_otf_input(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance);
int is_hw_mcsc_dma_input(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 instance);
int is_hw_mcsc_poly_phase(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, struct param_stripe_input *stripe_input,
	u32 output_id, u32 instance);
int is_hw_mcsc_post_chain(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_mcs_output *output, struct param_stripe_input *stripe_input,
	u32 output_id, u32 instance);
int is_hw_mcsc_flip(struct is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);
int is_hw_mcsc_dma_output(struct is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);
int is_hw_mcsc_output_yuvrange(struct is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);
int is_hw_mcsc_hwfc_mode(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	u32 hwfc_output_ids, u32 dma_output_ids, u32 instance);
int is_hw_mcsc_hwfc_output(struct is_hw_ip *hw_ip, struct param_mcs_output *output,
	u32 output_id, u32 instance);

int is_hw_mcsc_adjust_input_img_fmt(u32 format, u32 plane, u32 order, u32 *img_format);
int is_hw_mcsc_adjust_output_img_fmt(u32 bitwidth, u32 format, u32 plane,
					u32 order, u32 *img_format, bool *conv420_flag);
int is_hw_mcsc_check_format(enum mcsc_io_type type, u32 format, u32 bit_width,
	u32 width, u32 height);
u32 is_scaler_get_idle_status(void __iomem *base_addr, u32 hw_id);

void is_hw_mcsc_adjust_size_with_djag(struct is_hw_ip *hw_ip, struct param_mcs_input *input,
	struct param_stripe_input *stripe_input,
	struct is_hw_mcsc_cap *cap, u32 crop_type, u32 *x, u32 *y, u32 *width, u32 *height);
int is_hw_mcsc_update_djag_register(struct is_hw_ip *hw_ip,
		struct mcs_param *param,
		u32 instance);
int is_hw_mcsc_update_cac_register(struct is_hw_ip *hw_ip,
	struct is_frame *frame, u32 instance);
void is_hw_mcsc_s_debug_type(int type);
void is_hw_mcsc_c_debug_type(int type);

#ifdef DEBUG_HW_SIZE
#define hw_mcsc_check_size(hw_ip, input, output, instance, output_id) \
	is_hw_mcsc_check_size(hw_ip, input, output, instance, output_id)
#else
#define hw_mcsc_check_size(hw_ip, input, output, instance, output_id)
#endif
#endif
