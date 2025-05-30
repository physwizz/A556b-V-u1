/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for SAMSUNG DECON CAL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_DECON_CAL_H__
#define __SAMSUNG_DECON_CAL_H__

#include <exynos_panel.h>
#include <regs-decon.h>
#include <regs-dpp.h>
#include <cal_config.h>
#include "../exynos_drm_dsc.h"

#define DECON_BLENDING_PREMULT		0
#define DECON_BLENDING_COVERAGE		1
#define DECON_BLENDING_NONE		2

#define MAX_SRAMC_D_REGS		(2)

#define DECON_COLORMAP_BLACK		(0x00000000)
#define DECON_COLORMAP_WHITE		(0x00FFFFFF)

enum decon_dsi_mode {
	DSI_MODE_SINGLE = 0,
	DSI_MODE_DUAL_DSI,
	DSI_MODE_DUAL_DISPLAY,
	DSI_MODE_NONE
};

enum decon_data_path {
	/* No comp - OUTFIFO0 DSIM_IF0 */
	DPATH_NOCOMP_OUTFIFO0_DSIMIF0			= 0x001,
	/* No comp - FF0 - FORMATTER1 - DSIM_IF1 */
	DPATH_NOCOMP_OUTFIFO0_DSIMIF1			= 0x002,
	/* No comp - SPLITTER - FF0/1 - FORMATTER0/1 - DSIM_IF0/1 */
	DPATH_NOCOMP_SPLITTER_OUTFIFO01_DSIMIF01	= 0x003,
	/* No comp - OUTFIFO0/1/2 - WBIF */
	DPATH_NOCOMP_OUTFIFO0_WBIF			= 0x004,

	/* DSC_ENC0 - OUTFIFO0 - DSIM_IF0 */
	DPATH_DSCENC0_OUTFIFO0_DSIMIF0		= 0x011,
	/* DSC_ENC0 - OUTFIFO0 - DSIM_IF1 */
	DPATH_DSCENC0_OUTFIFO0_DSIMIF1		= 0x012,
	/* DSC_ENC0 - OUTFIFO0 - WBIF */
	DPATH_DSCENC0_OUTFIFO0_WBIF		= 0x014,

	/* DSCC,DSC_ENC0/1 - OUTFIFO01 DSIM_IF0 */
	DPATH_DSCC_DSCENC01_OUTFIFO01_DSIMIF0	= 0x0B1,
	/* DSCC,DSC_ENC0/1 - OUTFIFO01 DSIM_IF1 */
	DPATH_DSCC_DSCENC01_OUTFIFO01_DSIMIF1	= 0x0B2,
	/* DSCC,DSC_ENC0/1 - OUTFIFO01 DSIM_IF0/1 */
	DPATH_DSCC_DSCENC01_OUTFIFO01_DSIMIF01	= 0x0B3,
	/* DSCC,DSC_ENC0/1 - OUTFIFO01 - WBIF */
	DPATH_DSCC_DSCENC01_OUTFIFO01_WBIF	= 0x0B4,

	/* No comp - OUTFIFO0 DSIM_IF0 */
	DECON1_NOCOMP_OUTFIFO1_DSIMIF0		= 0x001,
	/* No comp - OUTFIFO0 DP_IF */
	DECON1_NOCOMP_OUTFIFO1_DPIF		= 0x008,
	/* DSC_ENC1 - OUTFIFO0 - DSIM_IF0 */
	DECON1_DSCENC1_OUTFIFO1_DSIMIF0		= 0x021,
	/* DSC_ENC1 - OUTFIFO0 - DP_IF */
	DECON1_DSCENC1_OUTFIFO1_DPIF		= 0x028,
	/* DSC_ENC1 - OUTFIFO1 - WBIF */
	DECON1_DSCENC1_OUTFIFO1_WBIF		= 0x024,

	/* No comp - OUTFIFO2 DP_IF */
	DECON2_NOCOMP_OUTFIFO2_DPIF		= 0x008,
	/* DSC_ENC2 - OUTFIFO2 - DP_IF0 */
	DECON2_DSCENC2_OUTFIFO2_DPIF		= 0x048,
	/* DSC_ENC2 - OUTFIFO2 - WBIF */
	DECON2_DSCENC2_OUTFIFO2_WBIF		= 0x044,
};

enum decon_enhance_path {
	ENHANCEPATH_ENHANCE_ALL_OFF	= 0x0,
	ENHANCEPATH_DITHER_ON		= 0x1,
	ENHANCEPATH_DQE_ON		= 0x2,
	ENHANCEPATH_DQE_DITHER_ON	= 0x3, /* post ENH stage is 8bpc */
};

enum decon_path_cfg {
	PATH_CON_ID_DSIM_IF0 = 0,
	PATH_CON_ID_DSIM_IF1 = 1,
	PATH_CON_ID_WB = 2,
	PATH_CON_ID_DP = 3,
	PATH_CON_ID_DUAL_DSC = 4,
	PATH_CON_ID_DSC0 = 4,
	PATH_CON_ID_DSC1 = 5,
	PATH_CON_ID_DSC2 = 6,
	PATH_CON_ID_DSCC_EN = 7,
};

enum decon_op_mode {
	DECON_VIDEO_MODE = 0,
	DECON_COMMAND_MODE = 1,
	/* TODO: ADD DP command mode */
};

enum decon_lp_mode {
	DECON_NORMAL = 0,
	DECON_ENTER_LP = 1,
}; /* TODO: needed HIBER_LP_STATE?? */

enum decon_opmode_option {
	DECON_CMD_LEGACY = 0,
	DECON_CMD_SVSYNC = 1,
	DECON_VDO_LEGACY = 0,
	DECON_VDO_LEGACY_C2V = 1,
	DECON_VDO_ADAPTIVE = 2,
	DECON_VDO_ADAPTIVE_C2V = 3,
	DECON_VDO_ONESHOT = 4,
	DECON_VDO_ONESHOT_C2V = 5,
	DECON_VDO_ONESHOT_ALIGN = 6,
	DECON_VDO_ONESHOT_ALIGN_C2V = 7,
	DECON_VDO_TE_DRIVEN = 8,
};

enum decon_trig_mode {
	DECON_HW_TRIG = 0,
	DECON_SW_TRIG
};

enum decon_out_type {
	DECON_OUT_DSI0 = 1 << 0,
	DECON_OUT_DSI1 = 1 << 1,
	DECON_OUT_DSI  = DECON_OUT_DSI0 | DECON_OUT_DSI1,

	DECON_OUT_DP0 = 1 << 4,
	DECON_OUT_DP1 = 1 << 5,
	DECON_OUT_DP  = DECON_OUT_DP0 | DECON_OUT_DP1,

	DECON_OUT_WB  = 1 << 8,
};

enum decon_rgb_order {
	DECON_RGB = 0x0,
	DECON_GBR = 0x1,
	DECON_BRG = 0x2,
	DECON_BGR = 0x4,
	DECON_RBG = 0x5,
	DECON_GRB = 0x6,
};

enum decon_win_func {
	PD_FUNC_CLEAR			= 0x0,
	PD_FUNC_COPY			= 0x1,
	PD_FUNC_DESTINATION		= 0x2,
	PD_FUNC_SOURCE_OVER		= 0x3,
	PD_FUNC_DESTINATION_OVER	= 0x4,
	PD_FUNC_SOURCE_IN		= 0x5,
	PD_FUNC_DESTINATION_IN		= 0x6,
	PD_FUNC_SOURCE_OUT		= 0x7,
	PD_FUNC_DESTINATION_OUT		= 0x8,
	PD_FUNC_SOURCE_A_TOP		= 0x9,
	PD_FUNC_DESTINATION_A_TOP	= 0xa,
	PD_FUNC_XOR			= 0xb,
	PD_FUNC_PLUS			= 0xc,
	PD_FUNC_USER_DEFINED		= 0xd,
};

enum decon_set_trig {
	DECON_TRIG_MASK = 0,
	DECON_TRIG_UNMASK
};

enum decon_te_from {
	DECON_TE_FROM_DDI0 = 0,
	DECON_TE_FROM_DDI1 = 1,
	DECON_TE_FROM_DDI2 = 2,
	MAX_DECON_TE_FROM_DDI = 3,
};

struct decon_vmc_mode {
	bool en;
	bool sync_shd_up;
};

enum decon_terminal_unit {
	DECON_TERMINAL_UNIT_DQE = 0,
	DECON_TERMINAL_UNIT_AIQE,
	DECON_TERMINAL_UNIT_MAX,
};

struct decon_mode {
	enum decon_op_mode op_mode;
	enum decon_dsi_mode dsi_mode;
	enum decon_trig_mode trig_mode;
	enum decon_lp_mode lp_mode;
	struct decon_vmc_mode vmc_mode;
};

struct decon_urgent {
	u32 rd_en;
	u32 rd_hi_thres;
	u32 rd_lo_thres;
	u32 rd_wait_cycle;
	u32 wr_en;
	u32 wr_hi_thres;
	u32 wr_lo_thres;
	u32 dta_en;
	u32 dta_hi_thres;
	u32 dta_lo_thres;
};

struct decon_vendor_pps {
	unsigned int initial_xmit_delay;
	unsigned int initial_dec_delay;
	unsigned int scale_increment_interval;
	unsigned int final_offset;
	unsigned int comp_cfg;
};

// #if IS_ENABLED(CONFIG_DRM_MCD_COMMON)
// /* Type for SVSYNC, depending on panel */
// enum svsync_type {
// 	SVSYNC_NONE = 0,
// 	SVSYNC_VFP,
// 	SVSYNC_TE_SHIFT,
// };
// #endif

struct decon_config {
	struct ip_version	version;
	enum decon_out_type	out_type;
	enum decon_te_from	te_from;
	unsigned int		image_height;
	unsigned int		image_width;
	struct decon_mode	mode;
	struct decon_urgent	urgent;
	struct exynos_dsc	dsc;
	struct drm_dsc_config	dsc_cfg;
	unsigned int		out_bpc;
	unsigned int		in_bpc;
	unsigned int		fps;
	unsigned int		rcd_en;
	u32			svsync_time;
	u32			svsync_on_fps;
// #if IS_ENABLED(CONFIG_DRM_MCD_COMMON)
// 	enum svsync_type	svsync_type;
// #endif
	bool vendor_pps_en;
	bool vote_overlap_bw;
	struct decon_vendor_pps vendor_pps;

	bool pscaler;
	unsigned int desired_width;
	unsigned int desired_height;

	bool support_opmode_switch;
	bool svsync_block;

	enum decon_terminal_unit terminal_unit;
};

struct decon_regs {
	void __iomem *regs;
	void __iomem *win_regs;
	void __iomem *sub_regs;
	void __iomem *wincon_regs;
	void __iomem *sramc_d_regs[MAX_SRAMC_D_REGS];
};

struct decon_window_regs {
	u32 wincon;
	u32 start_pos;
	u32 end_pos;
	u32 colormap;
	u32 start_time;
	u32 pixel_count;
	u32 whole_w;
	u32 whole_h;
	u32 offset_x;
	u32 offset_y;
	u32 winmap_state;
	int ch;
	int plane_alpha;
	u32 blend;
};

struct decon_dsc {
	unsigned int version;
	unsigned int comp_cfg;
	unsigned int bit_per_pixel;
	unsigned int bits_per_component;
	unsigned int line_buf_depth;
	unsigned int pic_height;
	unsigned int pic_width;
	unsigned int slice_height;
	unsigned int slice_width;
	unsigned int chunk_size;
	unsigned int initial_xmit_delay;
	unsigned int initial_dec_delay;
	unsigned int initial_scale_value;
	unsigned int scale_increment_interval;
	unsigned int scale_decrement_interval;
	unsigned int first_line_bpg_offset;
	unsigned int nfl_bpg_offset;
	unsigned int slice_bpg_offset;
	unsigned int initial_offset;
	unsigned int final_offset;
	unsigned int rc_range_parameters;
	unsigned int overlap_w;
	unsigned int width_per_enc;
	unsigned char *dec_pps_t;
};

void decon_regs_desc_init(void __iomem *regs, const char *name,
		enum decon_regs_type type, unsigned int id);

/*************** DECON CAL APIs exposed to DECON driver ***************/
/* DECON control */
int decon_reg_init(u32 id, struct decon_config *config);
int decon_reg_start(u32 id, struct decon_config *config);
int decon_reg_stop(u32 id, struct decon_config *config, bool rst, u32 fps);
void decon_reg_set_bpc_and_dither(u32 id, struct decon_config *config);

/* DECON window control */
void decon_reg_set_win_enable(u32 id, u32 win_idx, u32 en);
void decon_reg_win_enable_and_update(u32 id, u32 win_idx, u32 en);
void decon_reg_set_window_control(u32 id, int win_idx,
		struct decon_window_regs *regs, u32 winmap_en);

/* DECON shadow update and trigger control */
void decon_reg_set_trigger(u32 id, struct decon_mode *mode,
		enum decon_set_trig trig);
bool decon_reg_get_trigger_mask(u32 id);
void decon_reg_set_hw_trigger(u32 id, enum decon_set_trig trig);
int decon_reg_wait_update_done_timeout(u32 id, unsigned long timeout_us);
void decon_reg_init_trigger(u32 id, struct decon_config *cfg);

/* For window update and multi resolution feature */
int decon_reg_wait_idle_status_timeout(u32 id, unsigned long timeout);
void decon_reg_set_mres(u32 id, struct decon_config *config);
#if defined(CONFIG_DRM_SAMSUNG_EWR)
void decon_reg_update_ewr_control(u32 id, u32 fps);
#else
void decon_reg_update_ewr_control(u32 id, u32 fps) { }
#endif

#if IS_ENABLED(CONFIG_EXYNOS_CMD_SVSYNC_ONOFF)
void decon_reg_set_svsync_enable(u32 id, struct decon_config *cfg, bool en);
#endif

/* For operation mode switching */
void decon_reg_opmode_switch(u32 id, struct decon_config *config);
void decon_reg_svsync_block(u32 id, struct decon_config *config, bool block);

/* For writeback configuration */
void decon_reg_config_wb_size(u32 id, u32 height, u32 width);
void decon_reg_set_cwb_enable(u32 id, u32 en);

/* Dual Blender */
void decon_reg_set_win_idx_base(u32 id, u32 win_id_0, u32 win_id_1);
static inline u32 is_dual_blender(struct decon_config *config)
{
	return ((config->out_type & DECON_OUT_DP) && (config->image_width > 5120)) ? 1 : 0;
}

/* DECON interrupt control */
int decon_reg_get_interrupt_and_clear(u32 id, u32 *ext_irq);
void sramc_d_reg_get_irq_and_clear(u32 id);
void decon_reg_set_all_interrupts(u32 en, bool is_video_mode[]);

/* DECON SFR dump */
void __decon_dump(u32 id, struct decon_regs *regs, bool dsc_en);

void decon_reg_set_start_crc(u32 id, u32 en);
void decon_reg_get_crc_data(u32 id, u32 crc_data[3]);

/* For DQE configuration */
void decon_reg_set_dqe_enable(u32 id, bool en);
void decon_reg_set_dither_enable(u32 id, bool en);
void decon_reg_set_scaler_enable(u32 id, bool en);
void decon_reg_set_aiqe_enable(u32 id, bool en);
void decon_reg_set_aiqe_scaler_enable(u32 id, bool en);
void decon_reg_update_req_dqe(u32 id);
void decon_reg_set_rcd_enable(u32 id, bool en);

/* DPU hw limitation check */
struct decon_device;

/* PLL sleep related functions */
void decon_reg_set_pll_sleep(u32 id, u32 en);
void decon_reg_set_pll_wakeup(u32 id, u32 en);

u32 decon_reg_get_frame_cnt(u32 id);

u64 decon_reg_get_rsc_ch(u32 id);
u64 decon_reg_get_rsc_win(u32 id);

u32 decon_reg_get_run_status(u32 id);
u32 decon_reg_get_idle_status(u32 id);

u32 is_decon_using_ch(u64 rsc_ch, u32 ch);
u32 is_decon_using_win(u64 rsc_win, u32 win);

/* For Blender Bit Extension */
void decon_reg_set_blender_ext(u32 id, bool en);

/* For Recovery */
bool decon_reg_check_th_error(u32 id);

/* For Sync Command Transfer */
void decon_reg_set_shd_up_option(u32 id, bool unmask, struct decon_mode *mode);

/* For checking whether there is an enabled window or not */
u32 decon_reg_get_enable_windows(u32 id);
/*********************************************************************/

#endif /* __SAMSUNG_DECON_CAL_H__ */
