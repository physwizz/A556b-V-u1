/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_BL_H__
#define __PANEL_BL_H__

#include "timenval.h"
#include "panel_mutex.h"

#ifdef CONFIG_USDM_PANEL_DIMMING
#include "dimming.h"
#include "panel_dimming.h"
#endif

#include "panel_kunit.h"

struct panel_info;
struct panel_device;
struct panel_irc_info;

#undef DEBUG_PAC
#undef CONFIG_PANEL_BL_USE_BRT_CACHE

#define PANEL_BL_PROPERTY_BRIGHTNESS ("brightness")
#define PANEL_BL_PROPERTY_PREV_BRIGHTNESS ("prev_brightness")
#define PANEL_BL_PROPERTY_SMOOTH_TRANSITION ("smooth_transition")
#define PANEL_BL_PROPERTY_ACL_OPR ("acl_opr")
#define PANEL_BL_PROPERTY_ACL_PWRSAVE ("acl_pwrsave")
#define PANEL_BL_PROPERTY_NIGHT_DIM ("night_dim")
#define PANEL_BL_PROPERTY_LOCAL_HBM ("local_hbm")
#define PANEL_BL_PROPERTY_LOCAL_HBM_CIRLCE ("local_hbm_circle")

#define MAX_PANEL_BL_NAME_SIZE (32)

#ifdef CONFIG_USDM_PANEL_BACKLIGHT_PAC_3_0
#define BRT_SCALE	(10)
#define PANEL_BACKLIGHT_PAC_STEPS	(512)
#else
#define PANEL_BACKLIGHT_PAC_STEPS	(256)
#define BRT_SCALE	(1)
#endif
#define BRT_SCALE_UP(_val_)	((_val_) * BRT_SCALE)
#define BRT_SCALE_DN(_val_)	((_val_) / BRT_SCALE)
#define BRT_TBL_INDEX(_val_)	(BRT_SCALE_DN((_val_) + ((BRT_SCALE) - 1)))
#define BRT(brt)			(BRT_SCALE_UP(brt))
#define BRT_LT(brt)			(BRT_SCALE_UP(brt) - 1)
#define BRT_GT(brt)			(BRT_SCALE_UP(brt) + 1)
#define BRT_USER(brt)		(BRT_SCALE_DN(brt))

#define CALC_SCALE	(100)

#define UI_MIN_BRIGHTNESS			(BRT(0))
#define UI_DEF_BRIGHTNESS			(BRT(128))
#define UI_MAX_BRIGHTNESS			(BRT(255))
#define UI_BRIGHTNESS_STEPS		(UI_MAX_BRIGHTNESS - UI_MIN_BRIGHTNESS + 1)
#define IS_UI_BRIGHTNESS(br)            (((br) <= UI_MAX_BRIGHTNESS) ? 1 : 0)

#define AOR_TO_RATIO(aor, vtotal) \
	(((aor) * disp_pow(10, 5) / (vtotal) + 5 * disp_pow(10, 0)) / disp_pow(10, 1))

#define VIRTUAL_BASE_LUMINANCE(lum, aor_ratio) \
	((lum) * disp_pow(10, 6) / (disp_pow(10, 4) - (aor_ratio)))

enum SEARCH_TYPE {
	SEARCH_TYPE_EXACT,
	SEARCH_TYPE_LOWER,
	SEARCH_TYPE_UPPER,
	MAX_SEARCH_TYPE,
};

enum DIMTYPE {
	A_DIMMING = 0,
	S_DIMMING,
	MAX_DIMMING_TYPE,
};

enum {
	IRC_R_64 = 8,
	IRC_G_64,
	IRC_B_64,
	IRC_R_128,
	IRC_G_128,
	IRC_B_128,
	IRC_R_192,
	IRC_G_192,
	IRC_B_192,
	MAX_IRC_PARAM,
};

enum brightness_control_type {
	BRIGHTNESS_CONTROL_TYPE_GAMMA_MODE1,
	BRIGHTNESS_CONTROL_TYPE_GAMMA_MODE2,
	MAX_BRIGHTNESS_CONTROL_TYPE
};

enum dim_type {
	DIM_TYPE_STR_TABLE,
	DIM_TYPE_STR_FLASH,
	DIM_TYPE_STR_GM2,
	MAX_DIM_TYPE_STR,
};

struct brightness_table {
	int control_type;
	/* brightness to step count between brightness */
	u32 *step_cnt;
	u32 sz_step_cnt;
	/* brightness to step mapping table */
	u32 *brt_to_step;
	u32 sz_brt_to_step;
	/* brightness to brightness mapping table */
	u32 (*brt_to_brt)[2];
	u32 sz_brt_to_brt;
	/* brightness table */
	u32 *brt;
	u32 sz_brt;
	u32 sz_ui_brt;
	u32 sz_hbm_brt;
	/* actual brightness table */
	u32 *lum;
	u32 sz_lum;

	/*
	 * luminance table has 3 (ui, hbm, ext_hbm) regions.
	 * to determine current luminance's region, these sizes are necessary.
	 * ui luminance : ui brightness region. (typ:0 ~ 255)
	 * hbm luminance : hbm interpolation brightness region
	 * ext hbm luminance : extended hbm brightness
	 */
	u32 sz_ui_lum;
	u32 sz_hbm_lum;
	u32 sz_ext_hbm_lum;

	u32 sz_panel_dim_ui_lum;
	u32 sz_panel_dim_hbm_lum;
	u32 sz_panel_dim_ext_hbm_lum;
	u32 vtotal;
};

enum panel_bl_hw_type {
	PANEL_BL_HW_TYPE_TFT,
	PANEL_BL_HW_TYPE_OCTA,
	MAX_PANEL_BL_HW_TYPE,
};

enum panel_bl_subdev_type {
	PANEL_BL_SUBDEV_TYPE_DISP,
#ifdef CONFIG_USDM_PANEL_HMD
	PANEL_BL_SUBDEV_TYPE_HMD,
#endif
#ifdef CONFIG_USDM_PANEL_AOD_BL
	PANEL_BL_SUBDEV_TYPE_AOD,
#endif
	MAX_PANEL_BL_SUBDEV,
};

enum {
	SKIP_CMD = 0,
	SEND_CMD = 1,
};

struct panel_bl_properties {
	int id;
	int prev_brightness;
	int brightness;
	int actual_brightness;
	int actual_brightness_index;
	int actual_brightness_intrp;
	int step;
	int brightness_of_step;
	int acl_pwrsave;
	int acl_opr;
	int aor_ratio;
	int smooth_transition;
	int smooth_transition_sysfs_req;
	int smooth_transition_mask_layer_req;
	int night_dim;
	int local_hbm;
	int local_hbm_circle;
	int local_hbm_sysfs;
#ifdef CONFIG_USDM_PANEL_MASK_LAYER
	int mask_layer_br_target;
	int mask_layer_br_actual;
	int mask_layer_br_hook;
	int fp_green_circle;
#endif
	atomic_t brightness_set_count;
	atomic_t brightness_non_zero_set_count;
	bool saved;
};

struct panel_bl_sub_dev {
	const char *name;
	enum panel_bl_hw_type hw_type;
	enum panel_bl_subdev_type subdev_type;
	struct brightness_table brt_tbl;
	int brightness;
#if defined(CONFIG_PANEL_BL_USE_BRT_CACHE)
	int *brt_cache_tbl;
	int sz_brt_cache_tbl;
#endif
};

struct panel_bl_wq {
	wait_queue_head_t wait;
	atomic_t count;
	struct task_struct *thread;
	bool should_stop;
};

struct panel_bl_device {
	char name[MAX_PANEL_BL_NAME_SIZE];
	struct backlight_device *bd;
	void *bd_data;
	struct panel_mutex ops_lock;
	struct panel_mutex lock;
	struct panel_bl_properties props;
	struct panel_bl_sub_dev subdev[MAX_PANEL_BL_SUBDEV];
	struct timenval tnv[2];
	struct panel_bl_wq wq;
};

static inline void panel_bl_set_saved_flag(struct panel_bl_device *panel_bl, bool saved)
{
	if (!panel_bl)
		return;

	panel_bl->props.saved = saved;
};

static inline bool panel_bl_get_saved_flag(struct panel_bl_device *panel_bl)
{
	if (!panel_bl)
		return false;

	return panel_bl->props.saved;
};

int panel_bl_set_property(struct panel_bl_device *panel_bl,
		int *property, unsigned int value);
int panel_bl_init(struct panel_bl_device *panel_bl);
int panel_bl_exit(struct panel_bl_device *panel_bl);
int panel_bl_probe(struct panel_bl_device *panel_bl);
int panel_bl_remove(struct panel_bl_device *panel_bl);
int panel_bl_set_brightness(struct panel_bl_device *panel_bl, int id, u32 send_cmd);
int panel_update_brightness(struct panel_device *panel);
int panel_update_brightness_cmd_skip(struct panel_device *panel);
int panel_update_brightness_cmd_skip_nolock(struct panel_device *panel);
int max_brt_tbl(struct brightness_table *brt_tbl);
int get_max_brightness(struct panel_bl_device *panel_bl);
int get_brightness_pac_step_by_subdev_id(struct panel_bl_device *panel_bl, int id, int brightness);
int get_brightness_pac_step(struct panel_bl_device *panel_bl, int brightness);
int get_brightness_of_brt_to_step(struct panel_bl_device *panel_bl, int id, int brightness);
int get_actual_brightness(struct panel_bl_device *panel_bl, int brightness);
int get_subdev_actual_brightness(struct panel_bl_device *panel_bl, int id, int brightness);
int get_actual_brightness_index(struct panel_bl_device *panel_bl, int brightness);
int get_subdev_actual_brightness_index(struct panel_bl_device *panel_bl,
		int id, int brightness);
int get_actual_brightness_interpolation(struct panel_bl_device *panel_bl, int brightness);
int get_subdev_actual_brightness_interpolation(struct panel_bl_device *panel_bl, int id, int brightness);
int panel_bl_get_acl_pwrsave(struct panel_bl_device *panel_bl);
int panel_bl_get_acl_opr(struct panel_bl_device *panel_bl);
int panel_bl_get_smooth_transition(struct panel_bl_device *panel_bl);
bool is_hbm_brightness(struct panel_bl_device *panel_bl, int brightness);
bool is_ext_hbm_brightness(struct panel_bl_device *panel_bl, int brightness);
int panel_bl_set_subdev(struct panel_bl_device *panel_bl, int id);

int panel_bl_update_average(struct panel_bl_device *panel_bl, size_t index);
int panel_bl_clear_average(struct panel_bl_device *panel_bl, size_t index);
int panel_bl_get_average_and_clear(struct panel_bl_device *panel_bl, size_t index);
int aor_interpolation(unsigned int *brt_tbl, unsigned int *lum_tbl,
		u8 (*aor_tbl)[2], int size, int size_ui_lum, u32 vtotal, int brightness);
int panel_bl_aor_interpolation(struct panel_bl_device *panel_bl,
		int id, u8 (*aor_tbl)[2]);
int panel_bl_aor_interpolation_2(struct panel_bl_device *panel_bl,
		int id, u8 (*aor_tbl)[2]);
int panel_bl_irc_interpolation(struct panel_bl_device *panel_bl, int id,
	struct panel_irc_info *irc_info);
int search_tbl(int *tbl, int sz, enum SEARCH_TYPE type, int value);
int panel_bl_get_brightness_set_count(struct panel_bl_device *panel_bl);
void panel_bl_update_acl_state(struct panel_bl_device *panel_bl);
int panel_update_subdev_brightness(struct panel_device *panel, u32 subdev_id, u32 brightness);
void panel_bl_clear_brightness_non_zero_set_count(struct panel_bl_device *panel_bl);
int panel_bl_get_smooth_dim_request(struct panel_bl_device *panel_bl);
int panel_bl_get_smooth_transition_sysfs_req(struct panel_bl_device *panel_bl);
#endif /* __PANEL_BL_H__ */
