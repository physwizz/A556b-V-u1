// SPDX-License-Identifier: GPL-2.0-only
/* exynos_drm_connector.h
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _EXYNOS_DRM_CONNECTOR_H_
#define _EXYNOS_DRM_CONNECTOR_H_

#include <drm/drm_atomic.h>
#include <drm/drm_connector.h>
#include "exynos_drm_dsc.h"

/* bit mask for modeset_flags */
#define SEAMLESS_MODESET_MRES		BIT(0)
#define SEAMLESS_MODESET_VREF		BIT(1)
#define SEAMLESS_MODESET_LP		BIT(2)
#define SEAMLESS_MODESET_PSCALER	BIT(3)
#define SEAMLESS_MODESET_OPMS		BIT(4)

/* bit mask for hdr formats */
#define HDR_DOLBY_VISION	BIT(1)
#define HDR_HDR10		BIT(2)
#define HDR_HLG			BIT(3)
#define HDR_HDR10_PLUS		BIT(4)

struct exynos_drm_connector;

struct exynos_display_dsc {
	bool enabled;
	unsigned int dsc_count;
	unsigned int slice_count;
	unsigned int slice_height;
};

#define MAX_STEPPING_PATTERNS	5
struct exynos_freq_step {
	u32 idx;
	u32 cnt;
	u32 dur[MAX_STEPPING_PATTERNS];
	u32 num_repeat[MAX_STEPPING_PATTERNS];
};

#define EXYNOS_MODE(flag, b, lp_mode, dsc_en, dsc_cnt, slice_cnt, slice_h)	\
	.mode_flags = flag, .bpc = b, .is_lp_mode = lp_mode,			\
	.dsc = { .enabled = dsc_en, .dsc_count = dsc_cnt,			\
		.slice_count = slice_cnt, .slice_height = slice_h }		\

/* struct exynos_display_mode - exynos display specific info */
struct exynos_display_mode {
	/* @dsc: DSC parameters for the selected mode */
	struct exynos_display_dsc dsc;
	struct drm_dsc_config dsc_cfg;
	/* DSI mode flags in drm_mipi_dsi.h */
	unsigned long mode_flags;
	/* command: TE pulse time, video: vbp+vfp time */
	unsigned int vblank_usec;
	/* @bpc: display bits per component */
	unsigned int bpc;
	/* @is_lp_mode: boolean, if true it means this mode is a Low Power mode */
	bool is_lp_mode;
	/* @vscan_coef: scanning N times, in one frame */
	u16 vscan_coef;
	/* @bts_fps: fps value to calculate the DPU BW */
	unsigned int bts_fps;
	/* @plane_cnt: restriction plane count for this display mode */
	int plane_cnt;

	u16 desired_hdisplay;
	u16 desired_vdisplay;
	u16 scaler_type;

	/* @vhm: boolean, video hybrid mode */
	bool vhm;
	u32 esync_hz;
};

/* struct exynos_drm_connector_state - mutable connector state */
struct exynos_drm_connector_state {
	/* @base: base connector state */
	struct drm_connector_state base;
	/* @bypass: boolean, whether to bypass panel and regulator control */
	bool bypass_panel : 1;
	/* @brightness_level_changed: boolean, whether to brightness_level changed */
	bool brightness_level_changed: 1;
	/*
	 * @requested_panel_recovery : boolean, if true then panel requested
	 *  on & off during the recovery
	 */
	bool requested_panel_recovery;
	unsigned int panel_brightness_level;
};

#define to_exynos_connector_state(c)		\
	container_of((c), struct exynos_drm_connector_state, base)
#define to_exynos_connector(c)			\
	container_of((c), struct exynos_drm_connector, base)

struct exynos_drm_connector_funcs {
	enum drm_connector_status (*detect)(
			struct exynos_drm_connector *exynos_conn, bool force);
	void (*destroy)(struct exynos_drm_connector *exynos_conn);
	void (*atomic_print_state)(struct drm_printer *p,
			const struct exynos_drm_connector_state *state);
	int (*atomic_set_property)(struct exynos_drm_connector *exynos_conn,
			struct exynos_drm_connector_state *exynos_conn_state,
			struct drm_property *property, uint64_t val);
	int (*atomic_get_property)(struct exynos_drm_connector *exynos_conn,
			const struct exynos_drm_connector_state *exynos_conn_state,
			struct drm_property *property, uint64_t *val);
	void (*query_status)(struct exynos_drm_connector *exynos_conn);
	bool (*check_connection)(struct exynos_drm_connector *exynos_conn);
	void (*update_fps_config)(struct exynos_drm_connector *exynos_conn);
	int (*update_brightness)(struct exynos_drm_connector *exynos_conn, bool cmap);
	const struct exynos_freq_step *
		(*get_freq_step)(struct exynos_drm_connector *exynos_conn,
			     u32 frame_interval_ns);
	void (*set_freq_step)(struct exynos_drm_connector *exynos_conn,
                                 u32 frame_interval_ns);
	void (*update_panel)(struct exynos_drm_connector *exynos_conn);
};

struct exynos_drm_connector {
	struct drm_connector base;
	const struct exynos_drm_connector_funcs *funcs;
	u32 boost_bts_fps;
	ktime_t boost_expire_time;
};

void exynos_drm_boost_bts_fps(struct exynos_drm_connector *exynos_connector,
		u32 fps, ktime_t expire_time);
bool is_exynos_drm_connector(const struct drm_connector *connector);
int exynos_drm_connector_init(struct drm_device *dev,
		struct exynos_drm_connector *exynos_connector,
		const struct exynos_drm_connector_funcs *funcs,
		int connector_type);
int exynos_drm_connector_create_properties(struct drm_device *dev);
#endif /* _EXYNOS_DRM_CONNECTOR_H */
