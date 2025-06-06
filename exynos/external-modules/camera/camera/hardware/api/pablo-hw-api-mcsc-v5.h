/*
 * Samsung Exynos SoC series Pablo driver
 *
 * MCSC HW control functions
 *
 * Copyright (C) 2019 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_HW_API_MCSC_V5_H
#define IS_HW_API_MCSC_V5_H

#include "pablo-hw-api-common.h"
#include "pablo-hw-mcsc-v5.h"
#include "is-hw-common-dma.h"
#include "pablo-mmio.h"
#include "is-hw-api-type.h"
#include "is-param.h"

#define RATIO_X8_8	1048576
#define RATIO_X7_8	1198373
#define RATIO_X6_8	1398101
#define RATIO_X5_8	1677722
#define RATIO_X4_8	2097152
#define RATIO_X3_8	2796203
#define RATIO_X2_8	4194304

#define DJAG_PRE_LINE_BUF_SIZE_4880 4880
#define DJAG_PRE_LINE_BUF_SIZE_8192 8192
static int DJAG_PRE_LINE_BUF_SIZE = DJAG_PRE_LINE_BUF_SIZE_8192;

#define MCSC_SET_F(base, R, F, val)		PMIO_SET_F(base, R, F, val)
#define MCSC_SET_R(base, R, val)		PMIO_SET_R(base, R, val)
#define MCSC_SET_R_DIRECT(base, R, val)		PMIO_SET_R(base, R, val)
#define MCSC_SET_V(base, reg_val, F, val)	PMIO_SET_V(base, reg_val, F, val)

#define MCSC_GET_F(base, R, F)			PMIO_GET_F(base, R, F)
#define MCSC_GET_R(base, R)			PMIO_GET_R(base, R)

#define REG_OFFSET 4

#define FISRT_TAP	1
#define SECOND_TAP	2
#define FOURTH_TAP	4
#define SIXTH_TAP	6

#define MCSC_PRE_SC_SET_SKIP	2

enum mcsc_cotf_in_id {
	MCSC_COTF_IN_IMG,
	MCSC_COTF_IN_DLNR,
};

enum set_status {
	SET_SUCCESS,
	SET_ERROR,
};

enum mcsc_wdma_priority {
	MCSC_WDMA_OUTPUT0_Y = 0,
	MCSC_WDMA_OUTPUT0_U = 1,
	MCSC_WDMA_OUTPUT0_V = 2,
	MCSC_WDMA_OUTPUT1_Y = 3,
	MCSC_WDMA_OUTPUT1_U = 4,
	MCSC_WDMA_OUTPUT1_V = 5,
	MCSC_WDMA_OUTPUT2_Y = 6,
	MCSC_WDMA_OUTPUT2_U = 7,
	MCSC_WDMA_OUTPUT2_V = 8,
	MSCC_WDMA_PRI_A_MAX,

	MCSC_WDMA_OUTPUT3_Y = 0,
	MCSC_WDMA_OUTPUT3_U = 1,
	MCSC_WDMA_OUTPUT3_V = 2,
	MCSC_WDMA_OUTPUT4_Y = 3,
	MCSC_WDMA_OUTPUT4_U = 4,
	MCSC_WDMA_OUTPUT4_V = 5,
	MCSC_WDMA_OUTPUT5_Y = 6,	/* SSB */
	MSCC_WDMA_PRI_B_MAX
};

enum mcsc_filter_coeff {
	MCSC_COEFF_x8_8 = 0,	/* A (8/8 ~ ) */
	MCSC_COEFF_x7_8 = 1,	/* B (7/8 ~ ) */
	MCSC_COEFF_x6_8 = 2,	/* C (6/8 ~ ) */
	MCSC_COEFF_x5_8 = 3,	/* D (5/8 ~ ) */
	MCSC_COEFF_x4_8 = 4,	/* E (4/8 ~ ) */
	MCSC_COEFF_x3_8 = 5,	/* F (3/8 ~ ) */
	MCSC_COEFF_x2_8 = 6,	/* G (2/8 ~ ) */
	MCSC_COEFF_MAX
};

enum mcsc_event_type {
	INTR_FRAME_START,
	INTR_FRAME_END,
	INTR_OVERFLOW,
	INTR_OUTSTALL,
	INTR_INPUT_VERTICAL_UNF,
	INTR_INPUT_VERTICAL_OVF,
	INTR_INPUT_HORIZONTAL_UNF,
	INTR_INPUT_HORIZONTAL_OVF,
	INTR_WDMA_FINISH,
	INTR_COREX_END_0,
	INTR_COREX_END_1,
	INTR_SETTING_DONE,
	INTR_ERR,
};

enum mcsc_dtp_id {
	MCSC_DTP_OTF_IMG,
	MCSC_DTP_OTF_DL,
};

enum mcsc_dtp_type {
	MCSC_DTP_BYPASS,
	MCSC_DTP_SOLID_IMAGE,
	MCSC_DTP_COLOR_BAR,
};

enum mcsc_dtp_color_bar {
	MCSC_DTP_COLOR_BAR_BT601,
	MCSC_DTP_COLOR_BAR_BT709,
};

enum mcsc_scaler_coef_plane {
	MCSC_SC_COEF_Y,
	MCSC_SC_COEF_UV,
};

struct mcsc_v_coef {
	int v_coef_a[9];
	int v_coef_b[9];
	int v_coef_c[9];
	int v_coef_d[9];
};

struct mcsc_h_coef {
	int h_coef_a[9];
	int h_coef_b[9];
	int h_coef_c[9];
	int h_coef_d[9];
	int h_coef_e[9];
	int h_coef_f[9];
	int h_coef_g[9];
	int h_coef_h[9];
};

struct mcsc_radial_cfg {
	u32 sensor_full_width;
	u32 sensor_full_height;
	u32 sensor_binning_x;
	u32 sensor_binning_y;
	u32 sensor_crop_x;
	u32 sensor_crop_y;
	u32 bns_binning_x;
	u32 bns_binning_y;
	u32 taa_crop_x;
	u32 taa_crop_y;
	u32 taa_crop_width;
	u32 taa_crop_height;
};

enum mcsc_int_type {
	INT_FRAME_START,
	INT_FRAME_END,
	INT_COREX_END,
	INT_SETTING_DONE,
	INT_ERR0,
	INT_ERR1,
	INT_TYPE_NUM,
};

u32 is_scaler_sw_reset(struct pablo_mmio *base_addr, u32 hw_id, u32 global, u32 partial);
void is_scaler_s_cloader(struct pablo_mmio *base);

void is_scaler_set_input_source(struct pablo_mmio *base_addr, u32 hw_id, u32 rdma, bool strgen);
u32 is_scaler_get_input_source(struct pablo_mmio *base_addr, u32 hw_id);
void is_scaler_set_dither(struct pablo_mmio *base_addr, u32 hw_id, bool dither_en);
void is_scaler_set_input_img_size(struct pablo_mmio *base_addr, u32 hw_id, u32 width, u32 height);
void is_scaler_get_input_img_size(struct pablo_mmio *base_addr, u32 hw_id, u32 *width, u32 *height);

u32 is_scaler_get_scaler_path(struct pablo_mmio *base_addr, u32 hw_id, u32 output_id);
void is_scaler_set_poly_scaler_enable(
	struct pablo_mmio *base_addr, u32 hw_id, u32 output_id, u32 enable);
void is_scaler_set_poly_src_size(struct pablo_mmio *base_addr, u32 output_id, u32 pos_x, u32 pos_y,
	u32 width, u32 height);
void is_scaler_get_poly_src_size(struct pablo_mmio *base_addr, u32 output_id, u32 *width, u32 *height);
void is_scaler_set_poly_dst_size(struct pablo_mmio *base_addr, u32 output_id, u32 width, u32 height);
void is_scaler_get_poly_dst_size(struct pablo_mmio *base_addr, u32 output_id, u32 *width, u32 *height);
void is_scaler_set_poly_scaling_ratio(struct pablo_mmio *base_addr, u32 output_id, u32 hratio, u32 vratio);
void is_scaler_set_poly_scaler_coef(struct pablo_mmio *base_addr, u32 output_id, u32 hratio,
	u32 vratio, struct scaler_coef_config *sc_coefs[], int num_coef);
void is_scaler_set_poly_round_mode(struct pablo_mmio *base_addr, u32 output_id, u32 mode);
void is_scaler_set_post_scaler_enable(struct pablo_mmio *base_addr, u32 output_id, u32 enable);
void is_scaler_set_post_img_size(struct pablo_mmio *base_addr, u32 output_id, u32 width, u32 height);
void is_scaler_get_post_img_size(struct pablo_mmio *base_addr, u32 output_id, u32 *width, u32 *height);
void is_scaler_set_post_dst_size(struct pablo_mmio *base_addr, u32 output_id, u32 width, u32 height);
void is_scaler_get_post_dst_size(struct pablo_mmio *base_addr, u32 output_id, u32 *width, u32 *height);
void is_scaler_set_post_scaling_ratio(struct pablo_mmio *base_addr, u32 output_id, u32 hratio, u32 vratio);
void is_scaler_set_post_scaler_coef(struct pablo_mmio *base_addr, u32 output_id, u32 hratio,
	u32 vratio, struct scaler_coef_config *sc_coefs[], int num_coef);
void is_scaler_set_post_round_mode(struct pablo_mmio *base_addr, u32 output_id, u32 mode);

void is_scaler_set_420_conversion(struct pablo_mmio *base_addr, u32 output_id, u32 conv420_weight, u32 conv420_en);
void is_scaler_set_bchs_enable(struct pablo_mmio *base_addr, u32 output_id, bool bchs_en);
void is_scaler_set_b_c(struct pablo_mmio *base_addr, u32 output_id, u32 y_offset, u32 y_gain);
void is_scaler_set_h_s(struct pablo_mmio *base_addr, u32 output_id,
	u32 c_gain00, u32 c_gain01, u32 c_gain10, u32 c_gain11);
void is_scaler_set_bchs_clamp(struct pablo_mmio *base_addr, u32 output_id, u32 y_max, u32 y_min, u32 c_max, u32 c_min);
int mcsc_hw_rdma_create(struct is_common_dma *dma, void *base, u32 input_id);
int mcsc_hw_wdma_create(struct is_common_dma *dma, void *base, u32 input_id);
u32 is_scaler_get_dma_out_enable(struct pablo_mmio *base_addr, u32 output_id);

void is_scaler_set_mono_ctrl(struct pablo_mmio *base_addr, u32 hw_id, u32 in_fmt);
void is_scaler_set_rdma_format(struct pablo_mmio *base_addr, u32 hw_id, u32 dma_in_format);
void is_scaler_set_wdma_format(struct is_common_dma *dma, struct pablo_mmio *base_addr, u32 hw_id,
	u32 output_id, u32 plane, u32 dma_out_format);
void is_scaler_set_flip_mode(struct pablo_mmio *base_addr, u32 output_id, u32 flip);
void is_scaler_get_rdma_size(struct pablo_mmio *base_addr, u32 *width, u32 *height);
void is_scaler_get_wdma_size(struct pablo_mmio *base_addr, u32 output_id, u32 *width, u32 *height);
void is_scaler_get_rdma_stride(struct pablo_mmio *base_addr, u32 *y_stride, u32 *uv_stride);
void is_scaler_get_wdma_stride(struct pablo_mmio *base_addr, u32 output_id, u32 *y_stride, u32 *uv_stride);
void is_scaler_set_rdma_frame_seq(struct pablo_mmio *base_addr, u32 frame_seq);
void is_scaler_get_wdma_addr(struct pablo_mmio *base_addr, u32 output_id,
	u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index);
void is_scaler_clear_rdma_addr(struct pablo_mmio *base_addr);
void is_scaler_clear_wdma_addr(struct pablo_mmio *base_addr, u32 output_id);
void is_scaler_set_rdma_10bit_type(struct pablo_mmio *base_addr, u32 dma_in_10bit_type);
void is_scaler_set_wdma_dither(struct pablo_mmio *base_addr, u32 output_id, u32 fmt, u32 bitwidth, u32 plane);
void is_scaler_set_wdma_per_sub_frame_en(struct pablo_mmio *base_addr, u32 output_id, u32 sub_frame_en);
u32 is_scaler_g_dma_offset(struct param_mcs_output *output, u32 start_pos_x, u32 plane_i,
	u32 *strip_header_offset_in_byte);
u32 is_scaler_get_payload_size(struct param_mcs_output *output, u8 plane);

/* for hwfc */
void is_scaler_set_hwfc_mode(struct pablo_mmio *base_addr, u32 hwfc_output_ids);
void is_scaler_set_hwfc_config(struct pablo_mmio *base_addr,
		u32 output_id, u32 format, u32 plane, u32 dma_idx, u32 width, u32 height, u32 enable);
u32 is_scaler_get_hwfc_idx_bin(struct pablo_mmio *base_addr, u32 output_id);
u32 is_scaler_get_hwfc_cur_idx(struct pablo_mmio *base_addr, u32 output_id);

/* djag */
void is_scaler_set_djag_enable(struct pablo_mmio *base_addr, u32 djag_enable);
void is_scaler_set_pre_enable(struct pablo_mmio *base_addr, u32 presc_enable, u32 predjag_enable);
void is_scaler_set_djag_input_size(struct pablo_mmio *base_addr, u32 width, u32 height);
void is_scaler_set_djag_src_size(struct pablo_mmio *base_addr, u32 pos_x, u32 pos_y, u32 width, u32 height);
void is_scaler_set_djag_dst_size(struct pablo_mmio *base_addr, u32 width, u32 height);
void is_scaler_set_djag_scaling_ratio(struct pablo_mmio *base_addr, u32 hratio, u32 vratio);
void is_scaler_set_djag_init_phase_offset(struct pablo_mmio *base_addr, u32 h_offset, u32 v_offset);
void is_scaler_set_djag_round_mode(struct pablo_mmio *base_addr, u32 round_enable);
void is_scaler_set_djag_tuning_param(
	struct pablo_mmio *base_addr, const struct djag_config *djag_tune, u32 scale_index);
void is_scaler_set_predjag_tuning_param(
	struct pablo_mmio *base_addr, const struct predjag_config *djag_tune, u32 scale_index);
void is_scaler_set_djag_dither_wb(
	struct pablo_mmio *base_addr, struct djag_dither_config *dither_config);
void is_scaler_set_predjag_dither_wb(
	struct pablo_mmio *base_addr, struct djag_dither_config *dither_config);
void is_scaler_check_predjag_condition(struct pablo_mmio *base_addr, u32 in_width, u32 in_height,
	u32 *out_width, u32 *out_height);

/* FRO */
void is_scaler_set_lfro_mode_enable(struct pablo_mmio *base_addr, u32 hw_id, u32 lfro_enable, u32 lfro_total_fnum);

/* strip */
u32 is_scaler_get_djag_strip_enable(struct pablo_mmio *base_addr, u32 output_id);
void is_scaler_get_djag_strip_config(struct pablo_mmio *base_addr, u32 output_id, u32 *pre_dst_h, u32 *start_pos_h);
void is_scaler_set_djag_strip_enable(struct pablo_mmio *base_addr, u32 output_id, u32 enable);
void is_scaler_set_djag_strip_config(struct pablo_mmio *base_addr, u32 output_id, u32 pre_dst_h, u32 start_pos_h);

u32 is_scaler_get_poly_strip_enable(struct pablo_mmio *base_addr, u32 output_id);
void is_scaler_get_poly_strip_config(struct pablo_mmio *base_addr, u32 output_id, u32 *pre_dst_h, u32 *start_pos_h);
void is_scaler_set_poly_strip_enable(struct pablo_mmio *base_addr, u32 output_id, u32 enable);
void is_scaler_set_poly_strip_config(struct pablo_mmio *base_addr, u32 output_id, u32 pre_dst_h, u32 start_pos_h);

u32 is_scaler_get_poly_out_crop_enable(struct pablo_mmio *base_addr, u32 output_id);
void is_scaler_get_poly_out_crop_size(struct pablo_mmio *base_addr, u32 output_id, u32 *width, u32 *height);
void is_scaler_set_poly_out_crop_enable(struct pablo_mmio *base_addr, u32 output_id, u32 enable);
void is_scaler_set_poly_out_crop_size(struct pablo_mmio *base_addr, u32 output_id, u32 pos_x, u32 pos_y, u32 width, u32 height);

u32 is_scaler_get_post_strip_enable(struct pablo_mmio *base_addr, u32 output_id);
void is_scaler_get_post_strip_config(struct pablo_mmio *base_addr, u32 output_id, u32 *pre_dst_h, u32 *start_pos_h);
u32 is_scaler_get_post_out_crop_enable(struct pablo_mmio *base_addr, u32 output_id);
void is_scaler_get_post_out_crop_size(struct pablo_mmio *base_addr, u32 output_id, u32 *width, u32 *height);

void is_scaler_set_post_strip_enable(struct pablo_mmio *base_addr, u32 output_id, u32 enable);
void is_scaler_set_post_strip_config(struct pablo_mmio *base_addr, u32 output_id, u32 pre_dst_h, u32 start_pos_h);
void is_scaler_set_post_out_crop_enable(struct pablo_mmio *base_addr, u32 output_id, u32 enable);
void is_scaler_set_post_out_crop_size(struct pablo_mmio *base_addr, u32 output_id, u32 pos_x, u32 pos_y, u32 width, u32 height);

u32 is_scaler_get_stripe_align(u32 sbwc_type);

/* HF */
void is_scaler_set_djag_hf_enable(struct pablo_mmio *base_addr, u32 hf_enable);
void is_scaler_set_djag_hf_cfg(struct pablo_mmio *base_addr, struct hf_config *hf_cfg);
void is_scaler_clear_intr_src(struct pablo_mmio *base_addr, u32 hw_id, u32 status);
u32 is_scaler_get_intr_mask(struct pablo_mmio *base_addr, u32 hw_id);
u32 is_scaler_get_intr_status(struct pablo_mmio *base_addr, u32 hw_id);
void is_scaler_set_hf_cinfifo_ctrl(struct pablo_mmio *base, u32 en, bool strgen);
void is_scaler_set_hf_radial_config(struct pablo_mmio *base, struct mcsc_radial_cfg *radial_cfg,
	struct hf_config *hf_cfg, u32 width, u32 height);

/* sbwc */
void is_scaler_set_wdma_sbwc_config(struct is_common_dma *dma, struct pablo_mmio *base_addr,
	struct param_mcs_output *output, u32 output_id,
	u32 *y_stride, u32 *uv_stride, u32 *y_header_stride, u32 *uv_header_stride);

bool mcsc_hw_is_occurred(u32 status, ulong type);
void mcsc_hw_g_int_en(u32 *int_en);
u32 mcsc_hw_g_int_grp_en(void);

u32 is_scaler_get_version(struct pablo_mmio *base_addr);
void mcsc_hw_dump(struct pablo_mmio *pmio, u32 mode);
void mcsc_hw_s_dtp(struct pablo_mmio *base, enum mcsc_dtp_id id, u32 enable,
	enum mcsc_dtp_type type, u32 y, u32 u, u32 v, enum mcsc_dtp_color_bar cb);
u32 mcsc_hw_g_reg_cnt(void);
void mcsc_hw_g_pmio_cfg(struct pmio_config *cfg);

int mcsc_hw_g_rdma_max_cnt(void);
int mcsc_hw_g_wdma_max_cnt(void);
u32 is_scaler_get_idle_status(struct pablo_mmio *base_addr, u32 hw_id);

void is_hw_set_crc(struct pablo_mmio *base, u32 seed);

inline int mcsc_hw_g_djag_line_buffer(void);

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
/* CRC */
void pablo_kunit_scaler_hw_set_crc(struct pablo_mmio *base_addr, u32 seed);
#endif
#endif
