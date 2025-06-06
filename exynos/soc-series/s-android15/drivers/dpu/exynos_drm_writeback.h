/* SPDX-License-Identifier: GPL-2.0-only
 * exynos_drm_writeback.h
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 * Authors:
 *	Wonyeong Choi <won0.choi@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _EXYNOS_DRM_WRTIEBACK_H_
#define _EXYNOS_DRM_WRTIEBACK_H_

#include <drm/drm_writeback.h>
#include <samsung_drm.h>
#include <exynos_drm_drv.h>

#include <linux/dma-buf.h>
#include <media/exynos_repeater.h>

#include <dpp_cal.h>

enum writeback_state {
	WB_STATE_OFF = 0,
	WB_STATE_ON,
};

struct repeater_dma_buf {
	bool 				votf_supported;
	bool 				active;
	int				buf_idx;
	struct shared_buffer_info 	buf_info;
	struct dma_buf_attachment	*attachment[MAX_SHARED_BUF_NUM];
	struct sg_table			*sg_table[MAX_SHARED_BUF_NUM];
	dma_addr_t			dma_addr[MAX_SHARED_BUF_NUM];
};

struct dpuf;
struct writeback_device {
	struct device *dev;
	u32 id;
	u32 port;
	enum writeback_state state;
	unsigned long attr;
	int odma_irq;
	uint32_t *pixel_formats;
	unsigned int num_pixel_formats;

	spinlock_t odma_slock;

	struct dpp_regs	regs;
	struct dpp_params_info win_config;
	struct drm_writeback_connector writeback;

	enum exynos_drm_output_type output_type;

	struct repeater_dma_buf rdb;
	bool protection;
	struct dpuf *dpuf;
};

struct exynos_drm_writeback_state {
	struct drm_connector_state base;
	uint32_t standard;
	uint32_t range;
	bool use_repeater_buffer;
	uint32_t repeater_buf_idx;
	enum exynos_drm_writeback_type type;
};

#define to_wb_dev(wb_conn)		\
	container_of(wb_conn, struct writeback_device, writeback)
#define to_exynos_wb_state(state)	\
	container_of(state, struct exynos_drm_writeback_state, base)

#define conn_to_wb_conn(conn)		\
	container_of(conn, struct drm_writeback_connector, base)
#define conn_to_wb_dev(conn)	to_wb_dev(conn_to_wb_conn(conn))

#define enc_to_wb_conn(enc)		\
	container_of(enc, struct drm_writeback_connector, encoder)
#define enc_to_wb_dev(enc)	to_wb_dev(enc_to_wb_conn(enc))

static inline struct exynos_drm_crtc *
wb_get_exynos_crtc(const struct writeback_device *wb)
{
	const struct drm_connector_state *conn_state = wb->writeback.base.state;

	if (!conn_state || !conn_state->crtc)
		return NULL;

	return to_exynos_crtc(conn_state->crtc);
}

static inline const struct decon_device *
wb_get_decon(const struct writeback_device *wb)
{
	const struct drm_connector_state *conn_state = wb->writeback.base.state;

	if (!conn_state || !conn_state->crtc)
		return NULL;

	return to_exynos_crtc(conn_state->crtc)->ctx;
}

static inline bool wb_check_job(const struct drm_connector_state *conn_state)
{
	return (conn_state->writeback_job && conn_state->writeback_job->fb);
}

#if IS_ENABLED(CONFIG_DRM_SAMSUNG_WB)
void wb_dump(struct writeback_device *wb);
bool wb_notify_error(struct writeback_device *wb);
void repeater_buf_release(const struct drm_connector *conn,
			  const struct drm_connector_state *new_conn_state);
#else
static inline void wb_dump(struct writeback_device *wb) { return; }
static inline bool wb_notify_error(struct writeback_device *wb) { return false; }
static inline void repeater_buf_release(const struct drm_connector *conn,
			  const struct drm_connector_state *new_conn_state) { return; }
#endif

#if IS_ENABLED(CONFIG_VIDEO_EXYNOS_REPEATER)
extern int repeater_request_buffer(struct shared_buffer_info *info, int owner);
extern int repeater_get_valid_buffer(int *buf_idx);
extern int repeater_set_valid_buffer(int buf_idx, int capture_idx);
extern int repeater_dpu_dpms_off(void);
extern int repeater_notify_error(int);
#else
static inline int repeater_request_buffer(struct shared_buffer_info *info, int owner) { return 0; }
static inline int repeater_get_valid_buffer(int *buf_idx) { return 0; }
static inline int repeater_set_valid_buffer(int buf_idx, int capture_idx) { return 0; }
static inline int repeater_dpu_dpms_off(void) { return 0; }
static inline int repeater_notify_error(int owner) { return 0; }
#endif

#if IS_ENABLED(CONFIG_VIDEO_EXYNOS_TSMUX)
extern int tsmux_blending_start(int32_t index);
extern int tsmux_blending_end(int32_t index);
#else
static inline int tsmux_blending_start(int32_t index) { return 0; }
static inline int tsmux_blending_end(int32_t index) { return 0; }
#endif
#endif
