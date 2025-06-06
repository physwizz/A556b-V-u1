// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * MCFP HW control APIs
 *
 * Copyright (C) 2021 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_HW_API_MCFP_V11_0_H
#define IS_HW_API_MCFP_V11_0_H

#include "is-hw-mcfp.h"
#include "is-hw-common-dma.h"
#include "pablo-mmio.h"
#include "is-hw-api-type.h"

#define	MCFP_USE_MMIO	0

#define COREX_IGNORE			(0)
#define COREX_COPY			(1)
#define COREX_SWAP			(2)

#define HW_TRIGGER			(0)
#define SW_TRIGGER			(1)

enum set_status {
	SET_SUCCESS,
	SET_ERROR,
};

enum mcfp_event_type {
	INTR_FRAME_START,
	INTR_FRAME_END,
	INTR_COREX_END_0,
	INTR_COREX_END_1,
	INTR_SETTING_DONE,
	INTR_ERR,
};

enum mcfp_dtp_type {
	MCFP_DTP_BYPASS,
	MCFP_DTP_SOLID_IMAGE,
	MCFP_DTP_COLOR_BAR,
};

enum mcfp_dtp_color_bar {
	MCFP_DTP_COLOR_BAR_BT601,
	MCFP_DTP_COLOR_BAR_BT709,
};

enum mcfp_blk_check {
	MCFP_GEOMATCH_EN,
	MCFP_GEOMATCH_BYPASS,
	MCFP_GEOMATCH_MATCH_ENABLE,
	MCFP_YUV422TO444_BYPASS,
	MCFP_YUVTORGB_BYPASS,
	MCFP_GAMMARGB_BYPASS,
	MCFP_RGBTOYUV_BYPASS,
	MCFP_YUV444TO422_BYPASS,
	MCFP_BLK_BYPASS_MAX,
};

struct mcfp_radial_cfg {
	u32 sensor_full_width;
	u32 sensor_full_height;
	u32 sensor_binning_x;
	u32 sensor_binning_y;
	u32 sensor_crop_x;
	u32 sensor_crop_y;
	u32 bns_binning_x;
	u32 bns_binning_y;
	u32 sw_binning_x;
	u32 sw_binning_y;
	u32 taa_crop_x;
	u32 taa_crop_y;
	u32 taa_crop_width;
	u32 taa_crop_height;
};

int mcfp_hw_s_reset(struct pablo_mmio *base);
void mcfp_hw_s_init(struct pablo_mmio *base);
void mcfp_hw_s_clock(struct pablo_mmio *base, bool on);
u32 mcfp_hw_is_occurred(unsigned int status, enum mcfp_event_type type);
u32 mcfp_hw_is_occurred1(unsigned int status, enum mcfp_event_type type);
int mcfp_hw_wait_idle(struct pablo_mmio *base);
void mcfp_hw_dump(struct pablo_mmio *pmio, u32 mode);
void mcfp_hw_s_core(struct pablo_mmio *base, u32 set_id);
void mcfp_hw_dma_dump(struct is_common_dma *dma);
void mcfp_hw_s_dma_corex_id(struct is_common_dma *dma, u32 set_id);
int mcfp_hw_s_rdma_init(struct is_common_dma *dma, struct param_dma_input *dma_input,
	struct param_stripe_input *stripe_input,
	u32 frame_width, u32 frame_height, u32 *sbwc_en, u32 *payload_size,
	u32 *strip_offset, u32 *header_offset, struct is_mcfp_config *config);
int mcfp_hw_s_rdma_addr(struct is_common_dma *dma, pdma_addr_t *addr, u32 plane, u32 num_buffers,
	int buf_idx, u32 comp_sbwc_en, u32 payload_size, u32 strip_offset, u32 header_offset);
int mcfp_hw_s_wdma_init(struct is_common_dma *dma, struct param_dma_output *dma_output,
	struct param_stripe_input *stripe_input,
	u32 frame_width, u32 frame_height,
	u32 *sbwc_en, u32 *payload_size, u32 *strip_offset, u32 *header_offset,
	struct is_mcfp_config *config);
int mcfp_hw_s_wdma_addr(struct is_common_dma *dma, pdma_addr_t *addr, u32 plane, u32 num_buffers,
	int buf_idx, u32 comp_sbwc_en, u32 payload_size, u32 strip_offset, u32 header_offset);
void mcfp_hw_s_cmdq_queue(struct pablo_mmio *base, u32 num_buffers, dma_addr_t clh, u32 noh);
void mcfp_hw_s_cmdq_init(struct pablo_mmio *base);
void mcfp_hw_s_cmdq_enable(struct pablo_mmio *base, u32 enable);
unsigned int mcfp_hw_g_int_state(struct pablo_mmio *base, bool clear, u32 num_buffers, u32 irq_index, u32 *irq_state);
unsigned int mcfp_hw_g_int_mask(struct pablo_mmio *base, u32 irq_index);
void mcfp_hw_s_otf_input(struct pablo_mmio *base, u32 set_id, u32 enable);
void mcfp_hw_s_nr_otf_input(struct pablo_mmio *base, u32 set_id, u32 enable);
void mcfp_hw_s_otf_output(struct pablo_mmio *base, u32 set_id, u32 enable);
void mcfp_hw_s_input_size(struct pablo_mmio *base, u32 set_id, u32 width, u32 height);
int mcfp_hw_rdma_create(struct is_common_dma *dma, void *base, u32 input_id);
int mcfp_hw_wdma_create(struct is_common_dma *dma, void *base, u32 input_id);
void mcfp_hw_s_block_bypass(struct pablo_mmio *base, u32 set_id);
int mcfp_hw_g_block_bypass(struct pablo_mmio *base, u32 get_val[], size_t val_size);
void mcfp_hw_s_geomatch_size(struct pablo_mmio *base, u32 set_id,
				u32 frame_width, u32 dma_width, u32 height,
				bool strip_enable, u32 strip_start_pos,
				struct is_mcfp_config *mcfp_config);
void mcfp_hw_s_geomatch_bypass(struct pablo_mmio *base, u32 set_id, bool geomatch_bypass);
void mcfp_hw_s_mixer_size(struct pablo_mmio *base, u32 set_id,
		u32 frame_width, u32 dma_width, u32 height, bool strip_enable, u32 strip_start_pos,
		struct mcfp_radial_cfg *radial_cfg, struct is_mcfp_config *mcfp_config);
void mcfp_hw_s_crop_clean_img_otf(struct pablo_mmio *base, u32 set_id,
					u32 start_x, u32 width, u32 height,
					bool bypass);
void mcfp_hw_s_crop_wgt_otf(struct pablo_mmio *base, u32 set_id,
				u32 start_x, u32 width, u32 height,
				bool bypass);
void mcfp_hw_s_crop_clean_img_dma(struct pablo_mmio *base, u32 set_id,
					u32 start_x, u32 width, u32 height,
					bool bypass);
void mcfp_hw_s_crop_wgt_dma(struct pablo_mmio *base, u32 set_id,
					u32 start_x, u32 width, u32 height,
					bool bypass);
void mcfp_hw_s_img_bitshift(struct pablo_mmio *base, u32 set_id, u32 img_shift_bit);
void mcfp_hw_s_wgt_bitshift(struct pablo_mmio *base, u32 set_id, u32 wgt_shift_bit);
void mcfp_hw_g_img_bitshift(struct pablo_mmio *base, u32 set_id, u32 *shift,
	u32 *shift_chroma, u32 *offset, u32 *offset_chroma);
void mcfp_hw_g_wgt_bitshift(struct pablo_mmio *base, u32 set_id, u32 *shift);
void mcfp_hw_s_mono_mode(struct pablo_mmio *base, u32 set_id, bool enable);
void mcfp_hw_s_crc(struct pablo_mmio *base, u32 seed);

u32 mcfp_hw_g_reg_cnt(void);
void mcfp_hw_s_8x8_mv_mode(struct pablo_mmio *base, u32 set_id, u32 enable);
void mcfp_hw_s_dtp(struct pablo_mmio *base, u32 enable, enum mcfp_dtp_type type,
	u32 y, u32 u, u32 v, enum mcfp_dtp_color_bar cb);
void mcfp_hw_s_geomatch_mode(struct pablo_mmio *base, u32 set_id, u32 tnr_mode);
void mcfp_hw_s_mixer_mode(struct pablo_mmio *base, u32 set_id, u32 tnr_mode);
void mcfp_hw_s_strgen(struct pablo_mmio *base, u32 set_id);
void mcfp_hw_init_pmio_config(struct pmio_config *cfg);
void mcfp_hw_pmio_write_test(struct pablo_mmio *base, u32 set_id);
void mcfp_hw_pmio_read_test(struct pablo_mmio *base, u32 set_id);

void mcfp_hw_s_mixer_stitch_drc_map_width(struct pablo_mmio *base, u32 set_id, u32 width, u32 height);
void mcfp_hw_s_mixer_stitch_lme_map_width(struct pablo_mmio *base, u32 set_id,
		struct is_mcfp_config *mcfp_config, u32 width, u32 height);

#if IS_ENABLED(CONFIG_PABLO_DLFE)
void mcfp_hw_s_dlfe_path(struct pablo_mmio *base, u32 en);
#endif
#endif
