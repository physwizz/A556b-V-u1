/* SPDX-License-Identifier: GPL-2.0-only
 *
 * linux/drivers/gpu/drm/samsung/exynos_drm_dsim.h
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Headef file for Samsung MIPI DSI Master driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_DRM_DSI_H__
#define __EXYNOS_DRM_DSI_H__

/* Add header */
#include <drm/drm_encoder.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_property.h>
#include <drm/drm_panel.h>
#include <video/videomode.h>

#include <exynos_drm_drv.h>
#include <exynos_drm_crtc.h>
#include <exynos_drm_encoder.h>
#include <dsim_cal.h>

enum dsim_state {
	DSIM_STATE_INIT,	/* already enable-state from LK display */
	DSIM_STATE_HSCLKEN,	/* enable-state */
	DSIM_STATE_ULPS,	/* ulps-state by hibernation */
	DSIM_STATE_SUSPEND,	/* disable-state */
};

enum dphy_pll_sleep_ctrl {
	DPHY_SLEEP_CTRL_UNSUPPORTED,	/* NOT-Supported */
	DPHY_SLEEP_CTRL_LINK,		/* pll sleep control with DECON */
	DPHY_SLEEP_CTRL_DECON,		/* pll sleep control with DSI TX link */
};

enum dsim_underrun_debug {
	DSIM_UNDERRUN_DEBUG_NONE = 0,
	DSIM_UNDERRUN_DEBUG_S2D,
	DSIM_UNDERRUN_DEBUG_PANIC,
	DSIM_UNDERRUN_DEBUG_MAX,
};

struct dsim_pll_params {
	int curr_idx;
	unsigned int num_modes;
	struct dsim_pll_param *params;
};

struct dsim_resources {
	void __iomem *regs;
	void __iomem *phy_regs;
	void __iomem *phy_regs_ex;
	void __iomem *ss_reg_base;
	struct phy *phy;
	struct phy *phy_ex;
	struct clk *oscclk_dsim;
};

struct dsim_fb_handover {
	/* true  - fb reserved     */
	/* false - fb not reserved */
	bool reserved;
	phys_addr_t phys_addr;
	size_t phys_size;
	struct reserved_mem *rmem;
	int lk_fb_win_id;
};

struct dsim_burst_cmd {
	bool init_done;
	u32 pl_count;
	u32 line_count;
};

struct dsim_underrun_config {
	u32 continuous_count;
	int prev_linecnt;
};

#if defined(CONFIG_EXYNOS_DMA_DSIMFC)
#define to_dsim_fcmd(msg)	container_of(msg, struct dsim_fcmd, msg)

struct dsim_fcmd {
	u32 xfer_unit;
	struct mipi_dsi_msg msg;
};

struct dsim_dma_buf_data {
	struct dma_buf			*dma_buf;
	struct dma_buf_attachment	*attachment;
	struct sg_table			*sg_table;
	dma_addr_t			dma_addr;
};
#endif

struct dsim_freq_hop;
struct dsim_sync_cmd;
struct dsim_device {
	struct exynos_drm_encoder *encoder;
	struct mipi_dsi_host dsi_host;
	struct device *dev;
	struct drm_bridge *panel_bridge;
	struct mipi_dsi_device *dsi_device;

	enum exynos_drm_output_type output_type;

	struct dsim_resources res;
	struct clk **clks;
	struct dsim_pll_params *pll_params;
	int irq;
	int id;
	spinlock_t slock;
	struct mutex cmd_lock;
	struct completion ph_wr_comp;
	struct completion rd_comp;
	struct timer_list cmd_timer;
	struct work_struct cmd_work;

#if defined(CONFIG_EXYNOS_DMA_DSIMFC)
	struct dsimfc_device *dsimfc;
	struct completion pl_wr_comp;
	struct timer_list fcmd_timer;
	struct dsim_dma_buf_data fcmd_buf_data;
	struct dma_buf *fcmd_buf;
	void *fcmd_buf_vaddr;
	bool fcmd_buf_allocated;
	struct work_struct fcmd_work;
#endif

	enum dsim_state state;
	bool lp_mode_state;

	/* set bist mode by sysfs */
	unsigned int bist_mode;

	/* FIXME: dsim cal structure */
	struct dsim_reg_config config;
	struct dsim_clks clk_param;

	struct dsim_freq_hop *freq_hop;
	int idle_ip_index;
	struct dsim_fb_handover fb_handover;

	struct dsim_burst_cmd burst_cmd;
	bool emul_mode;

	enum dphy_pll_sleep_ctrl pll_sleep;
	struct dsim_underrun_config underrun_config;

	struct dsim_sync_cmd *sync_cmd;
	int g_cmd_sram;
	int s_cmd_sram;
};

extern struct dsim_device *dsim_drvdata[MAX_DSI_CNT];

static inline struct dsim_device *get_dsim_drvdata(u32 id)
{
	if (id < MAX_DSI_CNT)
		return dsim_drvdata[id];

	return NULL;
}

#define host_to_dsi(host) container_of(host, struct dsim_device, dsi_host)

#if !IS_ENABLED(CONFIG_DRM_MCD_COMMON)
#if defined(CONFIG_BOARD_EMULATOR)
#define EMUL_DISP_SLOW_DEGREE                   (1500)
#define MIPI_WR_TIMEOUT		msecs_to_jiffies(EMUL_DISP_SLOW_DEGREE * 50)
#define MIPI_RD_TIMEOUT		msecs_to_jiffies(EMUL_DISP_SLOW_DEGREE * 100)
#else
#define MIPI_WR_TIMEOUT				msecs_to_jiffies(50)
#define MIPI_RD_TIMEOUT				msecs_to_jiffies(100)
#endif
#else
#define MIPI_WR_TIMEOUT				msecs_to_jiffies(300)
#define MIPI_RD_TIMEOUT				msecs_to_jiffies(300)
#endif

struct decon_device;

static inline struct drm_crtc *dsim_get_crtc(const struct dsim_device *dsim)
{
	/* TODO: change to &drm_connector_state.crtc */
	struct exynos_drm_encoder *encoder;

	encoder = dsim->encoder;
	if (!encoder)
		return NULL;

	return encoder->base.crtc;
}

static inline struct exynos_drm_crtc *
dsim_get_exynos_crtc(const struct dsim_device *dsim)
{
	const struct drm_crtc *crtc;

	crtc = dsim_get_crtc(dsim);
	if (!crtc)
		return NULL;

	return to_exynos_crtc(crtc);
}

static inline const struct decon_device *
dsim_get_decon(const struct dsim_device *dsim)
{
	const struct drm_crtc *crtc;

	crtc = dsim_get_crtc(dsim);
	if (!crtc)
		return NULL;

	return to_exynos_crtc(crtc)->ctx;
}

static inline bool dsim_is_fb_reserved(const struct dsim_device *dsim)
{
	return dsim != NULL && dsim->fb_handover.reserved;
}

static inline bool is_dsim_enabled(const struct dsim_device *dsim)
{
	return	dsim->state == DSIM_STATE_HSCLKEN || dsim->state == DSIM_STATE_INIT;
}

static inline bool is_dsim_doze_suspended(const struct dsim_device *dsim)
{
	return	dsim->state == DSIM_STATE_SUSPEND && dsim->lp_mode_state;
}

int dsim_exit_pll_sleep(struct dsim_device *dsim);
void dsim_allow_pll_sleep(struct dsim_device *dsim);
void dsim_stop_pll_sleep(struct dsim_device *dsim, bool stop);
void dsim_enter_ulps_locked(struct dsim_device *dsim);
void dsim_exit_ulps_locked(struct dsim_device *dsim);
void dsim_exit_ulps_locked(struct dsim_device *dsim);
void dsim_enable_locked(struct exynos_drm_encoder *exynos_encoder);
void dsim_disable_locked(struct exynos_drm_encoder *exynos_encoder);
void dsim_check_cmd_transfer_mode(struct dsim_device *dsim,
		const struct mipi_dsi_msg *msg);

void dsim_wait_pending_vblank(struct dsim_device *dsim);
void dsim_dump(struct dsim_device *dsim);
int dsim_free_fb_resource(struct dsim_device *dsim);

#if defined(CONFIG_EXYNOS_DMA_DSIMFC)
ssize_t dsim_host_fcmd_transfer(struct mipi_dsi_host *host,
			    const struct mipi_dsi_msg *msg);
#endif
#endif /* __EXYNOS_DRM_DSI_H__ */
