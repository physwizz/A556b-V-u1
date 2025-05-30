/* SPDX-License-Identifier: GPL-2.0-only
 * exynos_drm_encoder.h
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 * Authors:
 *	Won Yeong Choi <won0.choi@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _EXYNOS_DRM_ENCODER_H_
#define _EXYNOS_DRM_ENCODER_H_

#include <exynos_drm_drv.h>
#include <drm/drm_encoder.h>

#define to_exynos_encoder(e) \
	container_of((e), struct exynos_drm_encoder, base)

/*
 * Exynos drm encoder ops
 *
 * @mode_valid: specific driver callback for mode validation
 * @atomic_mode_set: specific driver callback for enabling vblank interrupt.
 * @enable: enable the device
 * @disable: disable the device
 * @atomic_enable: enable the device with atomic commit.
 * @atomic_activate: activate the device with atomic commit.
 * 	The callback depend on the panel reset timing.
 * @atomic_disable: disable the device with atomic commit.
 * @atomic_enter_ulps: enter the device to ulps state with atomic commit.
 * @atomic_exit_ulps: exit the device from ulps state with atomic commit.
 * @atomic_check: validate state
 */
struct exynos_drm_encoder;
struct exynos_drm_encoder_funcs {
	enum drm_mode_status (*mode_valid)(struct exynos_drm_encoder *exynos_encoder,
			const struct drm_display_mode *mode);
	void (*atomic_mode_set)(struct exynos_drm_encoder *exynos_encoder,
			struct drm_crtc_state *crtc_state,
			struct drm_connector_state *conn_state);
	void (*enable)(struct exynos_drm_encoder *exynos_encoder);
	void (*disable)(struct exynos_drm_encoder *exynos_encoder);
	void (*atomic_enable)(struct exynos_drm_encoder *exynos_encoder,
			      struct drm_atomic_state *state);
	void (*atomic_activate)(struct exynos_drm_encoder *exynos_encoder);
	void (*atomic_disable)(struct exynos_drm_encoder *exynos_encoder,
			       struct drm_atomic_state *state);
	void (*atomic_enter_ulps)(struct exynos_drm_encoder *exynos_encoder);
	void (*atomic_exit_ulps)(struct exynos_drm_encoder *exynos_encoder);
	int (*atomic_check)(struct exynos_drm_encoder *exynos_encoder,
			struct drm_crtc_state *crtc_state,
			struct drm_connector_state *conn_state);
};

/*
 * Exynos specific encoder structure.
 *
 * @base: encoder object.
 * @funcs: callback fuctions.
 * @output_type: output type to determine possible crtcs.
 */
struct exynos_drm_encoder {
	struct drm_encoder	base;
	void			*ctx;
	const struct exynos_drm_encoder_funcs *funcs;
	enum exynos_drm_output_type output_type;
};

struct drm_connector *exynos_encoder_get_conn(struct exynos_drm_encoder *exynos_encoder,
					      struct drm_atomic_state *state);
struct exynos_drm_encoder *
exynos_drm_encoder_create(struct drm_device *drm_dev,
			  const struct exynos_drm_encoder_funcs *funcs,
			  int encoder_type, enum exynos_drm_output_type output_type,
			  void *ctx);
void exynos_drm_encoder_activate(struct drm_encoder *encoder,
				 struct drm_atomic_state *state);
void exynos_drm_encoder_set_possible_mask(struct drm_device *dev);
void exynos_drm_encoder_lastclose(struct drm_device *dev);
#endif
