/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Authors: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef __EXYNOS_DRM_PLANE_H__
#define __EXYNOS_DRM_PLANE_H__

#include <drm/drm_device.h>
#include <exynos_drm_drv.h>

#define EXYNOS_PLANE_ALPHA_MAX          0xff

enum {
	EXYNOS_STANDARD_UNSPECIFIED,
	EXYNOS_STANDARD_BT709,
	EXYNOS_STANDARD_BT601_625,
	EXYNOS_STANDARD_BT601_625_UNADJUSTED,
	EXYNOS_STANDARD_BT601_525,
	EXYNOS_STANDARD_BT601_525_UNADJUSTED,
	EXYNOS_STANDARD_BT2020,
	EXYNOS_STANDARD_BT2020_CONSTANT_LUMINANCE,
	EXYNOS_STANDARD_BT470M,
	EXYNOS_STANDARD_FILM,
	EXYNOS_STANDARD_DCI_P3,
	EXYNOS_STANDARD_ADOBE_RGB,
};

enum {
	EXYNOS_TRANSFER_UNSPECIFIED,
	EXYNOS_TRANSFER_LINEAR,
	EXYNOS_TRANSFER_SRGB,
	EXYNOS_TRANSFER_SMPTE_170M,
	EXYNOS_TRANSFER_GAMMA2_2,
	EXYNOS_TRANSFER_GAMMA2_6,
	EXYNOS_TRANSFER_GAMMA2_8,
	EXYNOS_TRANSFER_ST2084,
	EXYNOS_TRANSFER_HLG,
};

enum {
	EXYNOS_RANGE_UNSPECIFIED,
	EXYNOS_RANGE_FULL,
	EXYNOS_RANGE_LIMITED,
	EXYNOS_RANGE_EXTENDED,
};

#define to_exynos_plane(x)	container_of(x, struct exynos_drm_plane, base)

/*
 * Exynos drm plane ops
 *
 * @atomic_check: validate state.
 * @atomic_update: enabling the plane device.
 * @atomic_disable: disabling the plane device.
 * @atomic_print_state: printing additional driver specific data.
 */

struct exynos_drm_plane;
struct exynos_drm_plane_state;
struct exynos_drm_plane_ops {
	int (*atomic_check)(struct exynos_drm_plane *exynos_plane,
			    struct drm_atomic_state *state);
	int (*atomic_update)(struct exynos_drm_plane *exynos_plane,
			     const struct exynos_drm_plane_state *state);
	int (*atomic_disable)(struct exynos_drm_plane *exynos_plane);
	void (*atomic_print_state)(struct drm_printer *p,
			const struct exynos_drm_plane *exynos_plane);
};

struct exynos_drm_rect {
	unsigned int x, y;
	unsigned int w, h;
};

struct exynos_block_rect {
	int x;
	int y;
	u32 w;
	u32 h;
};

struct exynos_original_size {
	uint32_t src_w;
	uint32_t src_h;
	uint32_t dst_w;
	uint32_t dst_h;
};

struct exynos_split {
	uint32_t position;
	struct exynos_original_size orig_size;
	bool has_fraction;
};

/*
 * Exynos drm plane state structure.
 *
 * @base: plane_state object (contains drm_framebuffer pointer)
 * @standard:
 * @transfer:
 * @range:
 * @colormap:
 * @split:
 * @hdr_fd:
 *
 * this structure consists plane state data that will be applied to hardware
 * specific overlay info.
 */

struct exynos_drm_plane_state {
	struct drm_plane_state base;
	uint32_t standard;
	uint32_t transfer;
	uint32_t range;
	uint32_t colormap;
	struct drm_property_blob *split;
	bool need_scaler_pos;
	int64_t hdr_fd;
	void *hdr_ctx;
	bool hdr_en;
	bool hdr_depre_en;
	struct drm_property_blob *block;
#if IS_ENABLED(CONFIG_EXYNOS_DRM_BUFFER_SANITY_CHECK)
	char fence_info[256];
#endif
};

static inline struct exynos_drm_plane_state *
to_exynos_plane_state(const struct drm_plane_state *state)
{
	return container_of(state, struct exynos_drm_plane_state, base);
}

/*
 * Exynos drm common overlay structure.
 *
 * @base: plane object
 * @index: hardware index of the overlay layer
 *
 * this structure is common to exynos SoC and its contents would be copied
 * to hardware specific overlay info.
 */

struct exynos_drm_plane {
	struct drm_plane		base;
	unsigned int			index;
	const struct exynos_drm_plane_ops *ops;
	void				*ctx;
	/*
	 * comp_src means compression source of input buffer compressed by
	 * AFBC encoder of G2D or GPU.
	 *
	 * comp_src in dpu_bts_win_config structure is the information of
	 * the buffer requested by user-side.
	 *
	 * But comp_src in dpp_device structure is the information of the buffer
	 * already applied to the HW.
	 */
	u64 				comp_src;
	u32 				recovery_cnt;
	unsigned int 			win_id;	/* connected window id */
					/* Is dpp connected to window ? */
	bool 				is_win_connected;
	bool 				is_rcd;
};

#define EXYNOS_DRM_PLANE_CAP_FLIP	BIT(2)
#define EXYNOS_DRM_PLANE_CAP_ROT	BIT(3)
#define EXYNOS_DRM_PLANE_CAP_CSC	BIT(4)
#define EXYNOS_DRM_PLANE_CAP_HDR	BIT(6)
#define EXYNOS_DRM_PLANE_CAP_RCD	BIT(7)

/*
 * Exynos drm plane configuration structure.
 *
 * @zpos: initial z-position of the plane.
 * @max_zpos: maximal possible z-position of the plane.
 * @type: type of the plane (primary, cursor or overlay).
 * @pixel_formats: supported pixel formats.
 * @num_pixel_formats: number of elements in 'pixel_formats'.
 * @modifiers: supported modifiers.
 * @capabilities: supported features (see EXYNOS_DRM_PLANE_CAP_*).
 * @res: dpp chanel restriction.
 * @ctx: a pointer to the dpp device.
 *
 * The configuration structure used to create exynos plane structure.
 */

struct exynos_drm_plane_config {
	unsigned int zpos;
	unsigned int max_zpos;
	enum drm_plane_type type;
	const uint32_t *pixel_formats;
	unsigned int num_pixel_formats;
	const uint64_t *modifiers;
	unsigned int capabilities;
	const struct dpp_restrict *res;
	void *ctx;
};

int exynos_drm_plane_create_properties(struct drm_device *drm_dev);
struct exynos_drm_plane *
exynos_drm_plane_create(struct drm_device *dev, unsigned int index,
		       const struct exynos_drm_plane_config *config,
		       const struct exynos_drm_plane_ops *ops);

#endif /* __EXYNOS_DRM_PLANE_H__ */
