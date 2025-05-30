/* SPDX-License-Identifier: GPL-2.0-only
 *
 * linux/drivers/gpu/drm/samsung/exynos_drm_decon.h
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Headef file for Samsung MIPI DSI Master driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_DRM_DECON_H__
#define __EXYNOS_DRM_DECON_H__

#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <soc/samsung/exynos_pm_qos.h>
#include <soc/samsung/exynos/debug-snapshot.h>
#include <drm/drm_device.h>
#include <video/videomode.h>

#include <exynos_drm_dpp.h>
#include <exynos_drm_drv.h>
#include <exynos_drm_crtc.h>
#include <exynos_drm_fb.h>
#include <exynos_drm_dsim.h>
#include <exynos_drm_hibernation.h>
#include <exynos_drm_writeback.h>
#include <exynos_drm_encoder.h>
#include <exynos_drm_recovery.h>

#include <decon_cal.h>

enum decon_state {
	DECON_STATE_INIT = 0,
	DECON_STATE_ON,
	DECON_STATE_HIBERNATION,
	DECON_STATE_OFF,
};

struct decon_resources {
	struct pinctrl *pinctrl;
	struct pinctrl_state *te_on;
	struct pinctrl_state *te_off;
	struct clk *aclk;
	struct clk *aclk_disp;
};

struct decon_device {
	u32				id;
	enum decon_state		state;
	struct decon_regs		regs;
	struct device			*dev;
	int				pd_cnt;
	char				**pd_names;
	struct drm_device		*drm_dev;
	struct exynos_drm_crtc		*crtc;
	/* dpp information saved in dpp channel number order */
	struct dpp_device		*dpp[MAX_WIN_PER_DECON];
	struct dpp_device		*rcd;
	u32				dpp_cnt;
	u32				win_cnt;
	enum exynos_drm_output_type	con_type;
	struct decon_config		config;
	struct decon_resources		res;
	struct dpu_bts			bts;
	struct completion		framestart_done;
	struct exynos_recovery		recovery;
	const struct decon_restrict	restriction;

	u32				irq_fs;	/* frame start irq number*/
	u32				irq_fd;	/* frame done irq number*/
	u32				irq_ext;/* extra irq number*/
	int				irq_sramc_d;/* sramc_d irq number*/
	int				irq_sramc1_d;/* sramc1_d irq number*/
	u32				irq_te;
	u32				pnum_te;

	spinlock_t			slock;
	ktime_t				timestamp_s;

	bool busy;
	bool dimming;
	bool emul_mode;
	wait_queue_head_t framedone_wait;
	struct work_struct off_work;

	struct mutex trig_mask_lock;
	atomic_t trig_mask_count;
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
	atomic_t pll_sleep_masked;
#endif

#if IS_ENABLED(CONFIG_DRM_MCD_COMMON)
#if defined(CONFIG_USDM_PANEL_DPUI)
	struct notifier_block dpui_notif;
#endif
#endif
};

extern struct decon_device *decon_drvdata[MAX_DECON_CNT];

static inline struct decon_device *get_decon_drvdata(u32 id)
{
	if (id < MAX_DECON_CNT)
		return decon_drvdata[id];

	return NULL;
}

void decon_dump(struct exynos_drm_crtc *crtc);

static inline struct drm_encoder*
decon_get_encoder(const struct decon_device *decon, u32 encoder_type)
{
	const struct drm_crtc *crtc = &decon->crtc->base;

	if (!crtc->state)
		return NULL;

	return crtc_get_encoder(crtc->state, encoder_type);
}

static inline struct dsim_device*
decon_get_dsim(struct decon_device *decon)
{
	struct drm_encoder *encoder;
	struct exynos_drm_encoder *exynos_encoder;

	encoder = decon_get_encoder(decon, DRM_MODE_ENCODER_DSI);
	if (!encoder)
		return NULL;

	exynos_encoder = to_exynos_encoder(encoder);
	return exynos_encoder->ctx;
}

static inline struct writeback_device*
decon_get_wb(struct decon_device *decon)
{
	struct drm_encoder *encoder;
	struct drm_writeback_connector *wb_connector;

	encoder = decon_get_encoder(decon, DRM_MODE_ENCODER_VIRTUAL);
	if (!encoder)
		return NULL;

	wb_connector = container_of(encoder, struct drm_writeback_connector,
			encoder);

	return container_of(wb_connector, struct writeback_device, writeback);
}

#if defined(CONFIG_EXYNOS_PLL_SLEEP)
static inline
void decon_set_pll_sleep_masked(struct decon_device *decon, bool on)
{
	atomic_set(&decon->pll_sleep_masked, !!on);
}

static inline
bool decon_get_pll_sleep_masked(struct decon_device *decon)
{
	return atomic_read(&decon->pll_sleep_masked);
}
#endif

int decon_trigger_recovery(struct exynos_drm_crtc *exynos_crtc, char *mode);
bool decon_read_recovering(struct exynos_drm_crtc *exynos_crtc);

static inline bool IS_DECON_OFF_STATE(struct decon_device *decon)
{
	return decon->state == DECON_STATE_HIBERNATION ||
		decon->state == DECON_STATE_OFF;
}

static inline bool IS_DECON_HIBER_STATE(struct exynos_drm_crtc *exynos_crtc)
{
	struct decon_device *decon = exynos_crtc->ctx;

	return decon->state == DECON_STATE_HIBERNATION;
}

bool __is_vdo_to_cmd(struct decon_device *decon);
bool __is_cmd_to_vdo(struct decon_device *decon);

#if IS_ENABLED(CONFIG_DRM_MCD_COMMON)
#if defined(CONFIG_USDM_PANEL_DPUI)
void log_decon_bigdata(struct decon_device *decon);
#endif
#endif

#endif /* __EXYNOS_DRM_DECON_H__ */
