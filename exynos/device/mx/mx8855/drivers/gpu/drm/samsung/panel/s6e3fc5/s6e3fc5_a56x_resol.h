/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fc5/s6e3fc5_a56x_resol.h
 *
 * Header file for Panel Driver
 *
 * Copyright (c) 2019 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FC5_A56X_RESOL_H__
#define __S6E3FC5_A56X_RESOL_H__

#include <dt-bindings/display/panel-display.h>
#include "../panel.h"
#include "s6e3fc5.h"
#include "s6e3fc5_dimming.h"

enum {
	S6E3FC5_A56X_DISPLAY_MODE_1080x2340_120HS,
	S6E3FC5_A56X_DISPLAY_MODE_1080x2340_60HS_120HS_TE_HW_SKIP_1,
	S6E3FC5_A56X_DISPLAY_MODE_1080x2340_60HS,
	S6E3FC5_A56X_DISPLAY_MODE_1080x2340_30NS,
	MAX_S6E3FC5_A56X_DISPLAY_MODE,
};

struct panel_vrr s6e3fc5_a56x_default_panel_vrr[] = {
	[S6E3FC5_VRR_120HS] = {
		.fps = 120,
		.te_sw_skip_count = 0,
		.te_hw_skip_count = 0,
		.mode = VRR_HS_MODE,
	},
	[S6E3FC5_VRR_60HS_120HS_TE_HW_SKIP_1] = {
		.fps = 120,
		.te_sw_skip_count = 0,
		.te_hw_skip_count = 1,
		.mode = VRR_HS_MODE,
	},
	[S6E3FC5_VRR_60HS] = {
		.fps = 60,
		.te_sw_skip_count = 0,
		.te_hw_skip_count = 0,
		.mode = VRR_HS_MODE,
	},
	[S6E3FC5_VRR_30NS] = {
		.fps = 30,
		.te_sw_skip_count = 0,
		.te_hw_skip_count = 0,
		.mode = VRR_NORMAL_MODE,
	},
};

static struct panel_vrr *s6e3fc5_a56x_default_vrrtbl[] = {
	&s6e3fc5_a56x_default_panel_vrr[S6E3FC5_VRR_120HS],
	&s6e3fc5_a56x_default_panel_vrr[S6E3FC5_VRR_60HS_120HS_TE_HW_SKIP_1],
	&s6e3fc5_a56x_default_panel_vrr[S6E3FC5_VRR_60HS],
	&s6e3fc5_a56x_default_panel_vrr[S6E3FC5_VRR_30NS],
};

static struct panel_resol s6e3fc5_a56x_default_resol[] = {
	[S6E3FC5_RESOL_1080x2340] = {
		.w = 1080,
		.h = 2340,
		.comp_type = PN_COMP_TYPE_DSC,
		.comp_param = {
			.dsc = {
				.slice_w = 540,
				.slice_h = 40,
			},
		},
		.available_vrr = s6e3fc5_a56x_default_vrrtbl,
		.nr_available_vrr = ARRAY_SIZE(s6e3fc5_a56x_default_vrrtbl),
	},
};

#if defined(CONFIG_USDM_PANEL_DISPLAY_MODE)
static struct common_panel_display_mode s6e3fc5_a56x_display_mode[] = {
	/* FHD */
	[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_120HS] = {
		.name = PANEL_DISPLAY_MODE_1080x2340_120HS,
		.resol = &s6e3fc5_a56x_default_resol[S6E3FC5_RESOL_1080x2340],
		.vrr = &s6e3fc5_a56x_default_panel_vrr[S6E3FC5_VRR_120HS],
	},
	[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_60HS_120HS_TE_HW_SKIP_1] = {
		.name = PANEL_DISPLAY_MODE_1080x2340_60HS_120HS_TE_SW_SKIP_1,
		.resol = &s6e3fc5_a56x_default_resol[S6E3FC5_RESOL_1080x2340],
		.vrr = &s6e3fc5_a56x_default_panel_vrr[S6E3FC5_VRR_60HS_120HS_TE_HW_SKIP_1],
	},
	[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_60HS] = {
		.name = PANEL_DISPLAY_MODE_1080x2340_60HS,
		.resol = &s6e3fc5_a56x_default_resol[S6E3FC5_RESOL_1080x2340],
		.vrr = &s6e3fc5_a56x_default_panel_vrr[S6E3FC5_VRR_60HS],
	},
	[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_30NS] = {
		.name = PANEL_DISPLAY_MODE_1080x2340_30NS,
		.resol = &s6e3fc5_a56x_default_resol[S6E3FC5_RESOL_1080x2340],
		.vrr = &s6e3fc5_a56x_default_panel_vrr[S6E3FC5_VRR_30NS],
	},
};

static struct common_panel_display_mode *s6e3fc5_a56x_display_mode_array[] = {
	[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_120HS] = &s6e3fc5_a56x_display_mode[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_120HS],
	[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_60HS_120HS_TE_HW_SKIP_1] = &s6e3fc5_a56x_display_mode[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_60HS_120HS_TE_HW_SKIP_1],
	[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_60HS] = &s6e3fc5_a56x_display_mode[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_60HS],
	[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_30NS] = &s6e3fc5_a56x_display_mode[S6E3FC5_A56X_DISPLAY_MODE_1080x2340_30NS],
};

static struct common_panel_display_modes s6e3fc5_a56x_display_modes = {
	.num_modes = ARRAY_SIZE(s6e3fc5_a56x_display_mode),
	.modes = (struct common_panel_display_mode **)&s6e3fc5_a56x_display_mode_array,
};
#endif /* CONFIG_USDM_PANEL_DISPLAY_MODE */
#endif /* __S6E3FC5_A56X_RESOL_H__ */
