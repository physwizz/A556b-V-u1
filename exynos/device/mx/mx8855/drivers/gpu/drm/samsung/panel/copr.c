// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of_gpio.h>
#include <linux/sec_panel_notifier_v2.h>
#include "panel.h"
#include "panel_drv.h"
#include "panel_debug.h"
#include "copr.h"

static struct copr_reg_info copr_reg_v0_list[] = {
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v0, copr_gamma) },
	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v0, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v0, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v0, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v0, copr_eb) },
	{ .name = "copr_roi_on=", .offset = offsetof(struct copr_reg_v0, roi_on) },
	{ .name = "copr_roi_x_s=", .offset = offsetof(struct copr_reg_v0, roi_xs) },
	{ .name = "copr_roi_y_s=", .offset = offsetof(struct copr_reg_v0, roi_ys) },
	{ .name = "copr_roi_x_e=", .offset = offsetof(struct copr_reg_v0, roi_xe) },
	{ .name = "copr_roi_y_e=", .offset = offsetof(struct copr_reg_v0, roi_ye) },
};

static struct copr_reg_info copr_reg_v1_list[] = {
	{ .name = "copr_cnt_re=", .offset = offsetof(struct copr_reg_v1, cnt_re) },
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v1, copr_gamma) },
	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v1, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v1, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v1, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v1, copr_eb) },
	{ .name = "copr_max_cnt=", .offset = offsetof(struct copr_reg_v1, max_cnt) },
	{ .name = "copr_roi_on=", .offset = offsetof(struct copr_reg_v1, roi_on) },
	{ .name = "copr_roi_x_s=", .offset = offsetof(struct copr_reg_v1, roi_xs) },
	{ .name = "copr_roi_y_s=", .offset = offsetof(struct copr_reg_v1, roi_ys) },
	{ .name = "copr_roi_x_e=", .offset = offsetof(struct copr_reg_v1, roi_xe) },
	{ .name = "copr_roi_y_e=", .offset = offsetof(struct copr_reg_v1, roi_ye) },
};

static struct copr_reg_info copr_reg_v2_list[] = {
	{ .name = "copr_cnt_re=", .offset = offsetof(struct copr_reg_v2, cnt_re) },
	{ .name = "copr_ilc=", .offset = offsetof(struct copr_reg_v2, copr_ilc) },
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v2, copr_gamma) },

	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v2, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v2, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v2, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v2, copr_eb) },
	{ .name = "copr_erc=", .offset = offsetof(struct copr_reg_v2, copr_erc) },
	{ .name = "copr_egc=", .offset = offsetof(struct copr_reg_v2, copr_egc) },
	{ .name = "copr_ebc=", .offset = offsetof(struct copr_reg_v2, copr_ebc) },
	{ .name = "copr_max_cnt=", .offset = offsetof(struct copr_reg_v2, max_cnt) },
	{ .name = "copr_roi_on=", .offset = offsetof(struct copr_reg_v2, roi_on) },
	{ .name = "copr_roi_x_s=", .offset = offsetof(struct copr_reg_v2, roi_xs) },
	{ .name = "copr_roi_y_s=", .offset = offsetof(struct copr_reg_v2, roi_ys) },
	{ .name = "copr_roi_x_e=", .offset = offsetof(struct copr_reg_v2, roi_xe) },
	{ .name = "copr_roi_y_e=", .offset = offsetof(struct copr_reg_v2, roi_ye) },
};

static struct copr_reg_info copr_reg_v3_list[] = {
	{ .name = "copr_mask=", .offset = offsetof(struct copr_reg_v3, copr_mask) },
	{ .name = "copr_cnt_re=", .offset = offsetof(struct copr_reg_v3, cnt_re) },
	{ .name = "copr_ilc=", .offset = offsetof(struct copr_reg_v3, copr_ilc) },
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v3, copr_gamma) },

	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v3, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v3, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v3, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v3, copr_eb) },
	{ .name = "copr_erc=", .offset = offsetof(struct copr_reg_v3, copr_erc) },
	{ .name = "copr_egc=", .offset = offsetof(struct copr_reg_v3, copr_egc) },
	{ .name = "copr_ebc=", .offset = offsetof(struct copr_reg_v3, copr_ebc) },
	{ .name = "copr_max_cnt=", .offset = offsetof(struct copr_reg_v3, max_cnt) },
	{ .name = "copr_roi_ctrl=", .offset = offsetof(struct copr_reg_v3, roi_on) },
	/* ROI1 */
	{ .name = "copr_roi1_x_s=", .offset = offsetof(struct copr_reg_v3, roi[0].roi_xs) },
	{ .name = "copr_roi1_y_s=", .offset = offsetof(struct copr_reg_v3, roi[0].roi_ys) },
	{ .name = "copr_roi1_x_e=", .offset = offsetof(struct copr_reg_v3, roi[0].roi_xe) },
	{ .name = "copr_roi1_y_e=", .offset = offsetof(struct copr_reg_v3, roi[0].roi_ye) },
	/* ROI2 */
	{ .name = "copr_roi2_x_s=", .offset = offsetof(struct copr_reg_v3, roi[1].roi_xs) },
	{ .name = "copr_roi2_y_s=", .offset = offsetof(struct copr_reg_v3, roi[1].roi_ys) },
	{ .name = "copr_roi2_x_e=", .offset = offsetof(struct copr_reg_v3, roi[1].roi_xe) },
	{ .name = "copr_roi2_y_e=", .offset = offsetof(struct copr_reg_v3, roi[1].roi_ye) },
	/* ROI3 */
	{ .name = "copr_roi3_x_s=", .offset = offsetof(struct copr_reg_v3, roi[2].roi_xs) },
	{ .name = "copr_roi3_y_s=", .offset = offsetof(struct copr_reg_v3, roi[2].roi_ys) },
	{ .name = "copr_roi3_x_e=", .offset = offsetof(struct copr_reg_v3, roi[2].roi_xe) },
	{ .name = "copr_roi3_y_e=", .offset = offsetof(struct copr_reg_v3, roi[2].roi_ye) },
	/* ROI4 */
	{ .name = "copr_roi4_x_s=", .offset = offsetof(struct copr_reg_v3, roi[3].roi_xs) },
	{ .name = "copr_roi4_y_s=", .offset = offsetof(struct copr_reg_v3, roi[3].roi_ys) },
	{ .name = "copr_roi4_x_e=", .offset = offsetof(struct copr_reg_v3, roi[3].roi_xe) },
	{ .name = "copr_roi4_y_e=", .offset = offsetof(struct copr_reg_v3, roi[3].roi_ye) },
	/* ROI5 */
	{ .name = "copr_roi5_x_s=", .offset = offsetof(struct copr_reg_v3, roi[4].roi_xs) },
	{ .name = "copr_roi5_y_s=", .offset = offsetof(struct copr_reg_v3, roi[4].roi_ys) },
	{ .name = "copr_roi5_x_e=", .offset = offsetof(struct copr_reg_v3, roi[4].roi_xe) },
	{ .name = "copr_roi5_y_e=", .offset = offsetof(struct copr_reg_v3, roi[4].roi_ye) },
	/* ROI6 */
	{ .name = "copr_roi6_x_s=", .offset = offsetof(struct copr_reg_v3, roi[5].roi_xs) },
	{ .name = "copr_roi6_y_s=", .offset = offsetof(struct copr_reg_v3, roi[5].roi_ys) },
	{ .name = "copr_roi6_x_e=", .offset = offsetof(struct copr_reg_v3, roi[5].roi_xe) },
	{ .name = "copr_roi6_y_e=", .offset = offsetof(struct copr_reg_v3, roi[5].roi_ye) },
};

static struct copr_reg_info copr_reg_v5_list[] = {
	{ .name = "copr_mask=", .offset = offsetof(struct copr_reg_v5, copr_mask) },
	{ .name = "copr_cnt_re=", .offset = offsetof(struct copr_reg_v5, cnt_re) },
	{ .name = "copr_ilc=", .offset = offsetof(struct copr_reg_v5, copr_ilc) },
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v5, copr_gamma) },

	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v5, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v5, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v5, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v5, copr_eb) },
	{ .name = "copr_erc=", .offset = offsetof(struct copr_reg_v5, copr_erc) },
	{ .name = "copr_egc=", .offset = offsetof(struct copr_reg_v5, copr_egc) },
	{ .name = "copr_ebc=", .offset = offsetof(struct copr_reg_v5, copr_ebc) },
	{ .name = "copr_max_cnt=", .offset = offsetof(struct copr_reg_v5, max_cnt) },
	{ .name = "copr_roi_ctrl=", .offset = offsetof(struct copr_reg_v5, roi_on) },
	/* ROI1 */
	{ .name = "copr_roi1_x_s=", .offset = offsetof(struct copr_reg_v5, roi[0].roi_xs) },
	{ .name = "copr_roi1_y_s=", .offset = offsetof(struct copr_reg_v5, roi[0].roi_ys) },
	{ .name = "copr_roi1_x_e=", .offset = offsetof(struct copr_reg_v5, roi[0].roi_xe) },
	{ .name = "copr_roi1_y_e=", .offset = offsetof(struct copr_reg_v5, roi[0].roi_ye) },
	/* ROI2 */
	{ .name = "copr_roi2_x_s=", .offset = offsetof(struct copr_reg_v5, roi[1].roi_xs) },
	{ .name = "copr_roi2_y_s=", .offset = offsetof(struct copr_reg_v5, roi[1].roi_ys) },
	{ .name = "copr_roi2_x_e=", .offset = offsetof(struct copr_reg_v5, roi[1].roi_xe) },
	{ .name = "copr_roi2_y_e=", .offset = offsetof(struct copr_reg_v5, roi[1].roi_ye) },
	/* ROI3 */
	{ .name = "copr_roi3_x_s=", .offset = offsetof(struct copr_reg_v5, roi[2].roi_xs) },
	{ .name = "copr_roi3_y_s=", .offset = offsetof(struct copr_reg_v5, roi[2].roi_ys) },
	{ .name = "copr_roi3_x_e=", .offset = offsetof(struct copr_reg_v5, roi[2].roi_xe) },
	{ .name = "copr_roi3_y_e=", .offset = offsetof(struct copr_reg_v5, roi[2].roi_ye) },
	/* ROI4 */
	{ .name = "copr_roi4_x_s=", .offset = offsetof(struct copr_reg_v5, roi[3].roi_xs) },
	{ .name = "copr_roi4_y_s=", .offset = offsetof(struct copr_reg_v5, roi[3].roi_ys) },
	{ .name = "copr_roi4_x_e=", .offset = offsetof(struct copr_reg_v5, roi[3].roi_xe) },
	{ .name = "copr_roi4_y_e=", .offset = offsetof(struct copr_reg_v5, roi[3].roi_ye) },
	/* ROI5 */
	{ .name = "copr_roi5_x_s=", .offset = offsetof(struct copr_reg_v5, roi[4].roi_xs) },
	{ .name = "copr_roi5_y_s=", .offset = offsetof(struct copr_reg_v5, roi[4].roi_ys) },
	{ .name = "copr_roi5_x_e=", .offset = offsetof(struct copr_reg_v5, roi[4].roi_xe) },
	{ .name = "copr_roi5_y_e=", .offset = offsetof(struct copr_reg_v5, roi[4].roi_ye) },
};

static struct copr_reg_info copr_reg_v6_list[] = {
	{ .name = "copr_mask=", .offset = offsetof(struct copr_reg_v6, copr_mask) },
	{ .name = "copr_pwr=", .offset = offsetof(struct copr_reg_v6, copr_pwr) },
	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v6, copr_en) },
	{ .name = "copr_roi_ctrl=", .offset = offsetof(struct copr_reg_v6, roi_on) },
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v6, copr_gamma) },
	{ .name = "copr_frame_count=", .offset = offsetof(struct copr_reg_v6, copr_frame_count) },
	/* ROI1 */
	{ .name = "copr_roi1_er=", .offset = offsetof(struct copr_reg_v6, roi[0].roi_er) },
	{ .name = "copr_roi1_eg=", .offset = offsetof(struct copr_reg_v6, roi[0].roi_eg) },
	{ .name = "copr_roi1_eb=", .offset = offsetof(struct copr_reg_v6, roi[0].roi_eb) },
	/* ROI2 */
	{ .name = "copr_roi2_er=", .offset = offsetof(struct copr_reg_v6, roi[1].roi_er) },
	{ .name = "copr_roi2_eg=", .offset = offsetof(struct copr_reg_v6, roi[1].roi_eg) },
	{ .name = "copr_roi2_eb=", .offset = offsetof(struct copr_reg_v6, roi[1].roi_eb) },
	/* ROI3 */
	{ .name = "copr_roi3_er=", .offset = offsetof(struct copr_reg_v6, roi[2].roi_er) },
	{ .name = "copr_roi3_eg=", .offset = offsetof(struct copr_reg_v6, roi[2].roi_eg) },
	{ .name = "copr_roi3_eb=", .offset = offsetof(struct copr_reg_v6, roi[2].roi_eb) },
	/* ROI4 */
	{ .name = "copr_roi4_er=", .offset = offsetof(struct copr_reg_v6, roi[3].roi_er) },
	{ .name = "copr_roi4_eg=", .offset = offsetof(struct copr_reg_v6, roi[3].roi_eg) },
	{ .name = "copr_roi4_eb=", .offset = offsetof(struct copr_reg_v6, roi[3].roi_eb) },
	/* ROI5 */
	{ .name = "copr_roi5_er=", .offset = offsetof(struct copr_reg_v6, roi[4].roi_er) },
	{ .name = "copr_roi5_eg=", .offset = offsetof(struct copr_reg_v6, roi[4].roi_eg) },
	{ .name = "copr_roi5_eb=", .offset = offsetof(struct copr_reg_v6, roi[4].roi_eb) },

	/* ROI1 */
	{ .name = "copr_roi1_x_s=", .offset = offsetof(struct copr_reg_v6, roi[0].roi_xs) },
	{ .name = "copr_roi1_y_s=", .offset = offsetof(struct copr_reg_v6, roi[0].roi_ys) },
	{ .name = "copr_roi1_x_e=", .offset = offsetof(struct copr_reg_v6, roi[0].roi_xe) },
	{ .name = "copr_roi1_y_e=", .offset = offsetof(struct copr_reg_v6, roi[0].roi_ye) },
	/* ROI2 */
	{ .name = "copr_roi2_x_s=", .offset = offsetof(struct copr_reg_v6, roi[1].roi_xs) },
	{ .name = "copr_roi2_y_s=", .offset = offsetof(struct copr_reg_v6, roi[1].roi_ys) },
	{ .name = "copr_roi2_x_e=", .offset = offsetof(struct copr_reg_v6, roi[1].roi_xe) },
	{ .name = "copr_roi2_y_e=", .offset = offsetof(struct copr_reg_v6, roi[1].roi_ye) },
	/* ROI3 */
	{ .name = "copr_roi3_x_s=", .offset = offsetof(struct copr_reg_v6, roi[2].roi_xs) },
	{ .name = "copr_roi3_y_s=", .offset = offsetof(struct copr_reg_v6, roi[2].roi_ys) },
	{ .name = "copr_roi3_x_e=", .offset = offsetof(struct copr_reg_v6, roi[2].roi_xe) },
	{ .name = "copr_roi3_y_e=", .offset = offsetof(struct copr_reg_v6, roi[2].roi_ye) },
	/* ROI4 */
	{ .name = "copr_roi4_x_s=", .offset = offsetof(struct copr_reg_v6, roi[3].roi_xs) },
	{ .name = "copr_roi4_y_s=", .offset = offsetof(struct copr_reg_v6, roi[3].roi_ys) },
	{ .name = "copr_roi4_x_e=", .offset = offsetof(struct copr_reg_v6, roi[3].roi_xe) },
	{ .name = "copr_roi4_y_e=", .offset = offsetof(struct copr_reg_v6, roi[3].roi_ye) },
	/* ROI5 */
	{ .name = "copr_roi5_x_s=", .offset = offsetof(struct copr_reg_v6, roi[4].roi_xs) },
	{ .name = "copr_roi5_y_s=", .offset = offsetof(struct copr_reg_v6, roi[4].roi_ys) },
	{ .name = "copr_roi5_x_e=", .offset = offsetof(struct copr_reg_v6, roi[4].roi_xe) },
	{ .name = "copr_roi5_y_e=", .offset = offsetof(struct copr_reg_v6, roi[4].roi_ye) },
};
static struct copr_reg_info copr_reg_v0_1_list[] = {
	{ .name = "copr_mask=", .offset = offsetof(struct copr_reg_v0_1, copr_mask) },
	{ .name = "copr_pwr=", .offset = offsetof(struct copr_reg_v0_1, copr_pwr) },
	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v0_1, copr_en) },
	{ .name = "copr_roi_ctrl=", .offset = offsetof(struct copr_reg_v0_1, copr_roi_ctrl) },
	{ .name = "copr_gamma_ctrl=", .offset = offsetof(struct copr_reg_v0_1, copr_gamma_ctrl) },
	/* ROI1 */
	{ .name = "copr_roi1_er=", .offset = offsetof(struct copr_reg_v0_1, roi[0].roi_er) },
	{ .name = "copr_roi1_eg=", .offset = offsetof(struct copr_reg_v0_1, roi[0].roi_eg) },
	{ .name = "copr_roi1_eb=", .offset = offsetof(struct copr_reg_v0_1, roi[0].roi_eb) },
	/* ROI2 */
	{ .name = "copr_roi2_er=", .offset = offsetof(struct copr_reg_v0_1, roi[1].roi_er) },
	{ .name = "copr_roi2_eg=", .offset = offsetof(struct copr_reg_v0_1, roi[1].roi_eg) },
	{ .name = "copr_roi2_eb=", .offset = offsetof(struct copr_reg_v0_1, roi[1].roi_eb) },
	/* ROI1 */
	{ .name = "copr_roi1_x_s=", .offset = offsetof(struct copr_reg_v0_1, roi[0].roi_xs) },
	{ .name = "copr_roi1_y_s=", .offset = offsetof(struct copr_reg_v0_1, roi[0].roi_ys) },
	{ .name = "copr_roi1_x_e=", .offset = offsetof(struct copr_reg_v0_1, roi[0].roi_xe) },
	{ .name = "copr_roi1_y_e=", .offset = offsetof(struct copr_reg_v0_1, roi[0].roi_ye) },
	/* ROI2 */
	{ .name = "copr_roi2_x_s=", .offset = offsetof(struct copr_reg_v0_1, roi[1].roi_xs) },
	{ .name = "copr_roi2_y_s=", .offset = offsetof(struct copr_reg_v0_1, roi[1].roi_ys) },
	{ .name = "copr_roi2_x_e=", .offset = offsetof(struct copr_reg_v0_1, roi[1].roi_xe) },
	{ .name = "copr_roi2_y_e=", .offset = offsetof(struct copr_reg_v0_1, roi[1].roi_ye) },
};

static int get_copr_ver(struct copr_info *copr)
{
	return copr->props.version;
}

static void SET_COPR_REG_GAMMA(struct copr_info *copr, unsigned int copr_gamma)
{
	u32 version = get_copr_ver(copr);

	if (version == COPR_VER_0)
		copr->props.reg.v0.copr_gamma = copr_gamma;
	else if (version == COPR_VER_1)
		copr->props.reg.v1.copr_gamma = copr_gamma;
	else if (version == COPR_VER_2)
		copr->props.reg.v2.copr_gamma = copr_gamma;
	else if (version == COPR_VER_3)
		copr->props.reg.v3.copr_gamma = copr_gamma;
	else if (version == COPR_VER_5)
		copr->props.reg.v5.copr_gamma = copr_gamma;
	else if (version == COPR_VER_6)
		copr->props.reg.v6.copr_gamma = copr_gamma;
	else
		panel_warn("unsupprted in ver%d\n", version);
}

static void SET_COPR_REG_E(struct copr_info *copr, int r, int g, int b)
{
	u32 version = get_copr_ver(copr);

	if (version == COPR_VER_0) {
		copr->props.reg.v0.copr_er = r;
		copr->props.reg.v0.copr_eg = g;
		copr->props.reg.v0.copr_eb = b;
	} else if (version == COPR_VER_1) {
		copr->props.reg.v1.copr_er = r;
		copr->props.reg.v1.copr_eg = g;
		copr->props.reg.v1.copr_eb = b;
	} else if (version == COPR_VER_2) {
		copr->props.reg.v2.copr_er = r;
		copr->props.reg.v2.copr_eg = g;
		copr->props.reg.v2.copr_eb = b;
	} else if (version == COPR_VER_3) {
		copr->props.reg.v3.copr_er = r;
		copr->props.reg.v3.copr_eg = g;
		copr->props.reg.v3.copr_eb = b;
	} else if (version == COPR_VER_5) {
		copr->props.reg.v5.copr_er = r;
		copr->props.reg.v5.copr_eg = g;
		copr->props.reg.v5.copr_eb = b;
	} else if ((version == COPR_VER_6) || (version == COPR_VER_0_1)) {
		panel_warn("unsupprted in ver%d\n", version);
	} else {
		panel_warn("unsupprted in ver%d\n", version);
	}
}

static void SET_COPR_REG_EC(struct copr_info *copr, int r, int g, int b)
{
	u32 version = get_copr_ver(copr);

	if (version == COPR_VER_0) {
		panel_warn("unsupprted in ver%d\n", version);
	} else if (version == COPR_VER_1) {
		panel_warn("unsupprted in ver%d\n", version);
	} else if (version == COPR_VER_2) {
		copr->props.reg.v2.copr_erc = r;
		copr->props.reg.v2.copr_egc = g;
		copr->props.reg.v2.copr_ebc = b;
	} else if (version == COPR_VER_3) {
		copr->props.reg.v3.copr_erc = r;
		copr->props.reg.v3.copr_egc = g;
		copr->props.reg.v3.copr_ebc = b;
	} else if (version == COPR_VER_5) {
		copr->props.reg.v5.copr_erc = r;
		copr->props.reg.v5.copr_egc = g;
		copr->props.reg.v5.copr_ebc = b;
	} else if ((version == COPR_VER_6) || (version == COPR_VER_0_1)) {
		panel_warn("unsupprted in ver%d\n", version);
	} else {
		panel_warn("unsupprted in ver%d\n", version);
	}
}

#if 0
static void SET_COPR_REG_CNT_RE(struct copr_info *copr, int cnt_re)
{
	u32 version = get_copr_ver(copr);

	if (version == COPR_VER_0)
		panel_warn("unsupprted in ver%d\n", version);
	else if (version == COPR_VER_1)
		copr->props.reg.v1.cnt_re = cnt_re;
	else if (version == COPR_VER_2)
		copr->props.reg.v2.cnt_re = cnt_re;
	else if (version == COPR_VER_3)
		copr->props.reg.v3.cnt_re = cnt_re;
	else if (version == COPR_VER_5)
		copr->props.reg.v5.cnt_re = cnt_re;
	else if (version == COPR_VER_6)
		panel_warn("unsupprted in ver%d\n", version);
	else
		panel_warn("unsupprted in ver%d\n", version);
}
#endif

static void SET_COPR_REG_ROI(struct copr_info *copr, struct copr_roi *roi, int nr_roi)
{
	u32 version = get_copr_ver(copr);
	struct copr_properties *props = &copr->props;
	int i;

	if (version == COPR_VER_2) {
		if (roi == NULL) {
			props->reg.v2.roi_xs = 0;
			props->reg.v2.roi_ys = 0;
			props->reg.v2.roi_xe = 0;
			props->reg.v2.roi_ye = 0;
			props->reg.v2.roi_on = 0;
		} else {
			props->reg.v2.roi_xs = roi[0].roi_xs;
			props->reg.v2.roi_ys = roi[0].roi_ys;
			props->reg.v2.roi_xe = roi[0].roi_xe;
			props->reg.v2.roi_ye = roi[0].roi_ye;
			props->reg.v2.roi_on = true;
		}
	} else if (version == COPR_VER_3) {
		if (roi == NULL) {
			props->reg.v3.roi_on = 0;
			memset(props->reg.v3.roi, 0, sizeof(props->reg.v3.roi));
		} else {
			props->reg.v3.roi_on = 0;
			for (i = 0; i < min_t(int, ARRAY_SIZE(props->reg.v3.roi), nr_roi); i++) {
				props->reg.v3.roi[i].roi_xs = roi[i].roi_xs;
				props->reg.v3.roi[i].roi_ys = roi[i].roi_ys;
				props->reg.v3.roi[i].roi_xe = roi[i].roi_xe;
				props->reg.v3.roi[i].roi_ye = roi[i].roi_ye;
				props->reg.v3.roi_on |= 0x1 << i;
			}
		}
	} else if (version == COPR_VER_5) {
		if (roi == NULL) {
			props->reg.v5.roi_on = 0;
			memset(props->reg.v5.roi, 0, sizeof(props->reg.v5.roi));
		} else {
			props->reg.v5.roi_on = 0;
			for (i = 0; i < min_t(int, ARRAY_SIZE(props->reg.v5.roi), nr_roi); i++) {
				props->reg.v5.roi[i].roi_xs = roi[i].roi_xs;
				props->reg.v5.roi[i].roi_ys = roi[i].roi_ys;
				props->reg.v5.roi[i].roi_xe = roi[i].roi_xe;
				props->reg.v5.roi[i].roi_ye = roi[i].roi_ye;
				props->reg.v5.roi_on |= 0x1 << i;
			}
		}
	} else if (version == COPR_VER_6) {
		if (roi == NULL) {
			props->reg.v6.roi_on = 0;
			memset(props->reg.v6.roi, 0, sizeof(props->reg.v6.roi));
		} else {
			props->reg.v6.roi_on = 0;
			for (i = 0; i < min_t(int, ARRAY_SIZE(props->reg.v6.roi), nr_roi); i++) {
				props->reg.v6.roi[i].roi_er = roi[i].roi_er;
				props->reg.v6.roi[i].roi_eg = roi[i].roi_eg;
				props->reg.v6.roi[i].roi_eb = roi[i].roi_eb;
				props->reg.v6.roi[i].roi_xs = roi[i].roi_xs;
				props->reg.v6.roi[i].roi_ys = roi[i].roi_ys;
				props->reg.v6.roi[i].roi_xe = roi[i].roi_xe;
				props->reg.v6.roi[i].roi_ye = roi[i].roi_ye;
				props->reg.v6.roi_on |= 0x1 << i;
			}
		}
	} else if (version == COPR_VER_0_1) {
		if (roi == NULL) {
			props->reg.v0_1.copr_roi_ctrl = 0;
			memset(props->reg.v0_1.roi, 0, sizeof(props->reg.v0_1.roi));
		} else {
			props->reg.v0_1.copr_roi_ctrl = 0;
			for (i = 0; i < min_t(int, ARRAY_SIZE(props->reg.v0_1.roi), nr_roi); i++) {
				props->reg.v0_1.roi[i].roi_er = roi[i].roi_er;
				props->reg.v0_1.roi[i].roi_eg = roi[i].roi_eg;
				props->reg.v0_1.roi[i].roi_eb = roi[i].roi_eb;
				props->reg.v0_1.roi[i].roi_xs = roi[i].roi_xs;
				props->reg.v0_1.roi[i].roi_ys = roi[i].roi_ys;
				props->reg.v0_1.roi[i].roi_xe = roi[i].roi_xe;
				props->reg.v0_1.roi[i].roi_ye = roi[i].roi_ye;
				props->reg.v0_1.copr_roi_ctrl |= 0x1 << i;
			}
		}
	}
}

int get_copr_reg_copr_en(struct copr_info *copr)
{
	u32 version = get_copr_ver(copr);
	int copr_en = COPR_REG_OFF;

	if (version == COPR_VER_0)
		copr_en = copr->props.reg.v0.copr_en;
	else if (version == COPR_VER_1)
		copr_en = copr->props.reg.v1.copr_en;
	else if (version == COPR_VER_2)
		copr_en = copr->props.reg.v2.copr_en;
	else if (version == COPR_VER_3)
		copr_en = copr->props.reg.v3.copr_en;
	else if (version == COPR_VER_5)
		copr_en = copr->props.reg.v5.copr_en;
	else if (version == COPR_VER_6)
		copr_en = copr->props.reg.v6.copr_en;
	else if (version == COPR_VER_0_1)
		copr_en = copr->props.reg.v0_1.copr_en;
	else
		panel_warn("unsupprted in ver%d\n", version);

	return copr_en;
}

int get_copr_reg_size(int version)
{
	if (version == COPR_VER_0)
		return ARRAY_SIZE(copr_reg_v0_list);
	else if (version == COPR_VER_1)
		return ARRAY_SIZE(copr_reg_v1_list);
	else if (version == COPR_VER_2)
		return ARRAY_SIZE(copr_reg_v2_list);
	else if (version == COPR_VER_3)
		return ARRAY_SIZE(copr_reg_v3_list);
	else if (version == COPR_VER_5)
		return ARRAY_SIZE(copr_reg_v5_list);
	else if (version == COPR_VER_6)
		return ARRAY_SIZE(copr_reg_v6_list);
	else if (version == COPR_VER_0_1)
		return ARRAY_SIZE(copr_reg_v0_1_list);

	else
		return 0;
}
EXPORT_SYMBOL(get_copr_reg_size);

int get_copr_reg_packed_size(int version)
{
	if (version == COPR_VER_5)
		return COPR_V5_CTRL_REG_SIZE;
	else if (version == COPR_VER_6)
		return COPR_V6_CTRL_REG_SIZE;
	else
		return 0;
}
EXPORT_SYMBOL(get_copr_reg_packed_size);

const char *get_copr_reg_name(int version, int index)
{
	if (version == COPR_VER_0)
		return copr_reg_v0_list[index].name;
	else if (version == COPR_VER_1)
		return copr_reg_v1_list[index].name;
	else if (version == COPR_VER_2)
		return copr_reg_v2_list[index].name;
	else if (version == COPR_VER_3)
		return copr_reg_v3_list[index].name;
	else if (version == COPR_VER_5)
		return copr_reg_v5_list[index].name;
	else if (version == COPR_VER_6)
		return copr_reg_v6_list[index].name;
	else if (version == COPR_VER_0_1)
		return copr_reg_v0_1_list[index].name;
	else
		return NULL;
}
EXPORT_SYMBOL(get_copr_reg_name);

int get_copr_reg_offset(int version, int index)
{
	if (version == COPR_VER_0)
		return copr_reg_v0_list[index].offset;
	else if (version == COPR_VER_1)
		return copr_reg_v1_list[index].offset;
	else if (version == COPR_VER_2)
		return copr_reg_v2_list[index].offset;
	else if (version == COPR_VER_3)
		return copr_reg_v3_list[index].offset;
	else if (version == COPR_VER_5)
		return copr_reg_v5_list[index].offset;
	else if (version == COPR_VER_6)
		return copr_reg_v6_list[index].offset;
	else if (version == COPR_VER_0_1)
		return copr_reg_v0_1_list[index].offset;
	else
		return -EINVAL;
}
EXPORT_SYMBOL(get_copr_reg_offset);

u32 *get_copr_reg_ptr(struct copr_reg *reg, int version, int index)
{
	int offset = get_copr_reg_offset(version, index);

	if (offset < 0)
		return NULL;

	if (version == COPR_VER_0)
		return (u32 *)((void *)&reg->v0 + offset);
	else if (version == COPR_VER_1)
		return (u32 *)((void *)&reg->v1 + offset);
	else if (version == COPR_VER_2)
		return (u32 *)((void *)&reg->v2 + offset);
	else if (version == COPR_VER_3)
		return (u32 *)((void *)&reg->v3 + offset);
	else if (version == COPR_VER_5)
		return (u32 *)((void *)&reg->v5 + offset);
	else if (version == COPR_VER_6)
		return (u32 *)((void *)&reg->v6 + offset);
	else if (version == COPR_VER_0_1)
		return (u32 *)((void *)&reg->v0_1 + offset);
	else
		return NULL;
}
EXPORT_SYMBOL(get_copr_reg_ptr);

int find_copr_reg_by_name(int version, char *s)
{
	int i;
	const char *name;

	if (s == NULL)
		return -EINVAL;

	for (i = 0; i < get_copr_reg_size(version); i++) {
		name = get_copr_reg_name(version, i);
		if (name == NULL)
			continue;

		if (!strncmp(name, s, strlen(name)))
			return i;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(find_copr_reg_by_name);

int copr_reg_to_byte_array(struct copr_reg *reg, int version, unsigned char *byte_array)
{
	int i, offset = 0;

	if (version == COPR_VER_5) {
		struct copr_reg_v5 *r = &reg->v5;

		byte_array[offset++] = (r->copr_mask << 5) | (r->cnt_re << 4) |
			(r->copr_ilc << 3) | (r->copr_gamma << 1) | r->copr_en;
		byte_array[offset++] = ((r->copr_er >> 8) & 0x3) << 4 |
			((r->copr_eg >> 8) & 0x3) << 2 | ((r->copr_eb >> 8) & 0x3);
		byte_array[offset++] = ((r->copr_erc >> 8) & 0x3) << 4 |
			((r->copr_egc >> 8) & 0x3) << 2 | ((r->copr_ebc >> 8) & 0x3);
		byte_array[offset++] = r->copr_er;
		byte_array[offset++] = r->copr_eg;
		byte_array[offset++] = r->copr_eb;
		byte_array[offset++] = r->copr_erc;
		byte_array[offset++] = r->copr_egc;
		byte_array[offset++] = r->copr_ebc;
		byte_array[offset++] = (r->max_cnt >> 8) & 0xFF;
		byte_array[offset++] = r->max_cnt & 0xFF;
		byte_array[offset++] = r->roi_on;
		for (i = 0; i < 5; i++) {
			byte_array[offset++] = (r->roi[i].roi_xs >> 8) & 0x7;
			byte_array[offset++] = r->roi[i].roi_xs & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_ys >> 8) & 0xF;
			byte_array[offset++] = r->roi[i].roi_ys & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_xe >> 8) & 0x7;
			byte_array[offset++] = r->roi[i].roi_xe & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_ye >> 8) & 0xF;
			byte_array[offset++] = r->roi[i].roi_ye & 0xFF;
		}
	} else if (version == COPR_VER_6) {
		struct copr_reg_v6 *r = &reg->v6;

		byte_array[offset++] = ((r->copr_mask & 0x1) << 4) |
			((r->copr_pwr & 0x1) << 1) |
			(r->copr_en & 0x1);
		byte_array[offset++] = (r->roi_on & 0x1F);
		byte_array[offset++] = (r->copr_gamma & 0x1F);
		byte_array[offset++] = (r->copr_frame_count >> 8) & 0x3;
		byte_array[offset++] = (r->copr_frame_count & 0xFF);

		/* COPR_ROI_ER/G/B */
		for (i = 0; i < 5; i++) {
			byte_array[offset++] = (r->roi[i].roi_er >> 8) & 0x3;
			byte_array[offset++] = r->roi[i].roi_er & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_eg >> 8) & 0x3;
			byte_array[offset++] = r->roi[i].roi_eg & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_eb >> 8) & 0x3;
			byte_array[offset++] = r->roi[i].roi_eb & 0xFF;
		}

		/* COPR_ROI_XS/YS/XE/YE */
		for (i = 0; i < 5; i++) {
			byte_array[offset++] = (r->roi[i].roi_xs >> 8) & 0x7;
			byte_array[offset++] = r->roi[i].roi_xs & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_ys >> 8) & 0xF;
			byte_array[offset++] = r->roi[i].roi_ys & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_xe >> 8) & 0x7;
			byte_array[offset++] = r->roi[i].roi_xe & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_ye >> 8) & 0xF;
			byte_array[offset++] = r->roi[i].roi_ye & 0xFF;
		}
	} else if (version == COPR_VER_0_1) {
		struct copr_reg_v0_1 *r = &reg->v0_1;

		byte_array[offset++] = (r->copr_mask << 4) | (r->copr_pwr << 1) | r->copr_en;
		byte_array[offset++] = ((r->copr_gamma_ctrl & 0x3) << 4) | (r->copr_roi_ctrl & 0x3);

		for (i = 0; i < 2; i++) {
			byte_array[offset++] = (r->roi[i].roi_er >> 8) & 0x03;
			byte_array[offset++] = r->roi[i].roi_er & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_eg >> 8) & 0x03;
			byte_array[offset++] = r->roi[i].roi_eg & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_eb >> 8) & 0x03;
			byte_array[offset++] = r->roi[i].roi_eb & 0xFF;
		}

		for (i = 0; i < 2; i++) {
			byte_array[offset++] = (r->roi[i].roi_xs >> 8) & 0x3;
			byte_array[offset++] = r->roi[i].roi_xs & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_ys >> 8) & 0x3;
			byte_array[offset++] = r->roi[i].roi_ys & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_xe >> 8) & 0x3;
			byte_array[offset++] = r->roi[i].roi_xe & 0xFF;
			byte_array[offset++] = (r->roi[i].roi_ye >> 8) & 0x3;
			byte_array[offset++] = r->roi[i].roi_ye & 0xFF;
		}

	}

	return 0;
}
EXPORT_SYMBOL(copr_reg_to_byte_array);

ssize_t copr_reg_show(struct copr_info *copr, char *buf)
{
	int i, len = 0, size;
	const char *name;
	u32 *ptr;
	u32 version = get_copr_ver(copr);

	size = get_copr_reg_size(version);
	for (i = 0; i < size; i++) {
		name = get_copr_reg_name(version, i);
		ptr = get_copr_reg_ptr(&copr->props.reg, version, i);
		if (name != NULL && ptr != NULL)
			len += snprintf(buf + len, PAGE_SIZE - len, "%s%d\n", name, *ptr);
	}

	return len;
}

int copr_reg_store(struct copr_info *copr, int index, u32 value)
{
	const char *name;
	u32 *ptr;
	u32 version = get_copr_ver(copr);
	int size = get_copr_reg_size(version);

	if (index >= size)
		return -EINVAL;

	name = get_copr_reg_name(version, index);
	ptr = get_copr_reg_ptr(&copr->props.reg, version, index);
	if (name != NULL && ptr != NULL)
		*ptr = value;
	else
		return -EINVAL;

	return 0;
}

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
static inline void panel_send_coprstate_notify(int display_idx, u32 state)
{
	struct panel_notifier_event_data evt_data = {
		.display_index = display_idx,
		.state = state,
	};

	panel_notifier_call_chain(PANEL_EVENT_COPR_STATE_CHANGED, &evt_data);
	panel_info("call EVENT_COPR_STATE notifier %d\n", evt_data.state);
}
#endif

struct seqinfo *find_copr_sequence(struct copr_info *copr, char *seqname)
{
	if (!copr) {
		panel_err("copr is null\n");
		return NULL;
	}

	return find_panel_seq_by_name(to_panel_device(copr), seqname);
}

int copr_do_sequence_nolock(struct copr_info *copr, char *seqname)
{
	struct seqinfo *seq;

	if (!copr) {
		panel_err("copr is null\n");
		return -EINVAL;
	}

	seq = find_copr_sequence(copr, seqname);
	if (!seq)
		return -EINVAL;

	return execute_sequence_nolock(to_panel_device(copr), seq);
}

int copr_do_sequence(struct copr_info *copr, char *seqname)
{
	int ret;
	struct panel_device *panel;

	if (!copr)
		return -EINVAL;

	panel = to_panel_device(copr);
	panel_mutex_lock(&panel->op_lock);
	ret = copr_do_sequence_nolock(copr, seqname);
	if (ret < 0) {
		ret = -EIO;
		goto err;
	}

err:
	panel_mutex_unlock(&panel->op_lock);

	return ret;
}

static int panel_set_copr(struct copr_info *copr)
{
	int ret;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	ret = copr_do_sequence(copr, COPR_SET_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("failed to run sequence(%s)\n",
				COPR_SET_SEQ);
		return ret;
	}

	msleep(34);
	if (get_copr_reg_copr_en(copr))
		copr->props.state = COPR_REG_ON;
	else
		copr->props.state = COPR_REG_OFF;

	return 0;
}

#ifdef CONFIG_SUPPORT_COPR_AVG
static int panel_clear_copr(struct copr_info *copr)
{
	int ret;

	ret = copr_do_sequence(copr, COPR_CLR_CNT_ON_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("failed to run sequence(%s)\n",
				COPR_CLR_CNT_ON_SEQ);
		return ret;
	}

	msleep(34);

	ret = copr_do_sequence(copr, COPR_CLR_CNT_OFF_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("failed to run sequence(%s)\n",
				COPR_CLR_CNT_OFF_SEQ);
		return ret;
	}

	msleep(34);

	panel_dbg("copr clear seq\n");

	return 0;
}
#endif

#ifdef CONFIG_USDM_COPR_SPI
static int panel_read_copr_spi(struct copr_info *copr)
{
	u8 *buf = NULL;
	int i, c, index, ret, size;
	struct panel_device *panel = to_panel_device(copr);
	struct panel_info *panel_data;
	struct copr_properties *props = &copr->props;
	u32 version = get_copr_ver(copr);
	int max_color = (version == COPR_VER_6 || version == COPR_VER_0_1) ?
		MAX_RGBW_COLOR : MAX_COLOR;

	if (unlikely(!panel)) {
		panel_err("panel is null\n");
		return -ENODEV;
	}
	panel_data = &panel->panel_data;

	ret = copr_do_sequence(copr, COPR_SPI_GET_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("failed to run sequence(%s)\n",
				COPR_SPI_GET_SEQ);
		goto get_copr_error;
	}

	size = get_panel_resource_size(panel, "copr_spi");
	if (size < 0) {
		panel_err("failed to get copr size (ret %d)\n", size);
		ret = -EINVAL;
		goto get_copr_error;
	}

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto get_copr_error;
	}

	ret = panel_resource_copy(panel, (u8 *)buf, "copr_spi");
	if (ret < 0) {
		panel_err("failed to get copr (ret %d)\n", ret);
		goto get_copr_error;
	}

	if (version == COPR_VER_6) {
		for (i = 0; i < 5; i++) {
			for (c = 0; c < max_color; c++) {
				index = i * (max_color * 2) + c * 2;
				if (i == 4 && c == RGBW_WHITE) {
					/* COPR_ROI5_W:8bit */
					props->copr_roi_r[i][c] = buf[index];
				} else {
					/* COPR_ROI1~5 RGBW:10bit */
					props->copr_roi_r[i][c] = (buf[index] << 8) | buf[index + 1];
				}
			}
			panel_dbg("copr_dsi: copr_roi_r[%d] %d %d %d %d\n",
					i, props->copr_roi_r[i][RGBW_RED],
					props->copr_roi_r[i][RGBW_GREEN],
					props->copr_roi_r[i][RGBW_BLUE],
					props->copr_roi_r[i][RGBW_WHITE]);
		}
	} else if (version == COPR_VER_0_1) {
		for (i = 0; i < 2; i++) {
			for (c = 0; c < max_color; c++) {
				index = i * (max_color * 2) + c * 2;
				if (i == 4 && c == RGBW_WHITE) {
					/* COPR_ROI5_W:8bit */
					props->copr_roi_r[i][c] = buf[index];
				} else {
					/* COPR_ROI1~5 RGBW:10bit */
					props->copr_roi_r[i][c] = (buf[index] << 8) | buf[index + 1];
				}
			}
			panel_dbg("copr_dsi: copr_roi_r[%d] %d %d %d %d\n",
					i, props->copr_roi_r[i][RGBW_RED],
					props->copr_roi_r[i][RGBW_GREEN],
					props->copr_roi_r[i][RGBW_BLUE],
					props->copr_roi_r[i][RGBW_WHITE]);
		}
	} else if (version == COPR_VER_3 || version == COPR_VER_5) {
		props->copr_ready = buf[0] & 0x01;
		props->cur_cnt = (buf[1] << 8) | buf[2];
		props->cur_copr = (buf[3] << 8) | buf[4];
		props->avg_copr = (buf[5] << 8) | buf[6];
		props->s_cur_cnt = (buf[7] << 8) | buf[8];
		props->s_avg_copr = (buf[9] << 8) | buf[10];
		for (i = 0; i < 5; i++) {
			for (c = 0; c < max_color; c++) {
				index = 11 + i * (max_color * 2) + c * 2;
				props->copr_roi_r[i][c] = (buf[index] << 8) | buf[index + 1];
			}
			panel_dbg("copr_spi: copr_roi_r[%d] %d %d %d\n",
					i, props->copr_roi_r[i][RED],
					props->copr_roi_r[i][GREEN],
					props->copr_roi_r[i][BLUE]);
		}
		panel_dbg("copr_spi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d, comp_copr %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready, props->comp_copr);
	} else if (version == COPR_VER_2) {
		props->copr_ready = ((buf[0] & 0x80) ? 1 : 0);
		props->cur_cnt = ((buf[0] & 0x7F) << 9) |
			(buf[1] << 1) | ((buf[2] & 0x80) ? 1 : 0);
		props->cur_copr = ((buf[2] & 0x7F) << 2) | ((buf[3] & 0xC0) >> 6);
		props->avg_copr = ((buf[3] & 0x3F) << 3) | ((buf[4] & 0xE0) >> 5);
		props->s_cur_cnt = ((buf[4] & 0x1F) << 11) |
			(buf[5] << 3) | ((buf[6] & 0xE0) >> 5);
		props->s_avg_copr = ((buf[6] & 0x1F) << 4) | ((buf[7] & 0xF0) >> 4);
		props->comp_copr = ((buf[7] & 0x0F) << 5) | ((buf[8] & 0xF8) >> 3);
		panel_dbg("copr_spi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d, comp_copr %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready, props->comp_copr);
	} else if (version == COPR_VER_1) {
		props->copr_ready = ((buf[0] & 0x80) ? 1 : 0);
		props->cur_cnt = ((buf[0] & 0x7F) << 9) |
			(buf[1] << 1) | ((buf[2] & 0x80) ? 1 : 0);
		props->cur_copr = ((buf[2] & 0x7F) << 2) | ((buf[3] & 0xC0) >> 6);
		props->avg_copr = ((buf[3] & 0x3F) << 3) | ((buf[4] & 0xE0) >> 5);
		props->s_cur_cnt = ((buf[4] & 0x1F) << 11) |
			(buf[5] << 3) | ((buf[6] & 0xE0) >> 5);
		props->s_avg_copr = ((buf[6] & 0x1F) << 4) | ((buf[7] & 0xF0) >> 4);
		panel_dbg("copr_spi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready);
	} else if (version == COPR_VER_0) {
		props->cur_copr = buf[0];
	}

get_copr_error:
	kfree(buf);

	return ret;
}
#else
static int panel_read_copr_dsi(struct copr_info *copr)
{
	u8 *buf = NULL;
	int i, c, index, ret, size;
	struct panel_device *panel = to_panel_device(copr);
	struct panel_info *panel_data;
	struct copr_properties *props = &copr->props;
	u32 version = get_copr_ver(copr);
	int max_color = (version == COPR_VER_6 || version == COPR_VER_0_1) ?
		MAX_RGBW_COLOR : MAX_COLOR;

	if (unlikely(!panel)) {
		panel_err("panel is null\n");
		return -ENODEV;
	}
	panel_data = &panel->panel_data;

	ret = copr_do_sequence(copr, COPR_DSI_GET_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("failed to run sequence(%s)\n",
				COPR_DSI_GET_SEQ);
		goto get_copr_error;
	}

	size = get_panel_resource_size(panel, "copr_dsi");
	if (size < 0) {
		panel_err("failed to get copr size (ret %d)\n", size);
		ret = -EINVAL;
		goto get_copr_error;
	}

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto get_copr_error;
	}

	ret = panel_resource_copy(panel, (u8 *)buf, "copr_dsi");
	if (ret < 0) {
		panel_err("failed to get copr (ret %d)\n", ret);
		goto get_copr_error;
	}

	if (version == COPR_VER_6) {
		for (i = 0; i < 5; i++) {
			for (c = 0; c < max_color; c++) {
				index = i * (max_color * 2) + c * 2;
				if (i == 4 && c == RGBW_WHITE) {
					/* COPR_ROI5_W:8bit */
					props->copr_roi_r[i][c] = buf[index];
				} else {
					/* COPR_ROI1~5 RGBW:10bit */
					props->copr_roi_r[i][c] = (buf[index] << 8) | buf[index + 1];
				}
			}
			panel_dbg("copr_dsi: copr_roi_r[%d] %d %d %d %d\n",
					i, props->copr_roi_r[i][RGBW_RED],
					props->copr_roi_r[i][RGBW_GREEN],
					props->copr_roi_r[i][RGBW_BLUE],
					props->copr_roi_r[i][RGBW_WHITE]);
		}
	} else if (version == COPR_VER_0_1) {
		for (i = 0; i < 2; i++) {
			for (c = 0; c < max_color; c++) {
				index = i * (max_color * 2) + c * 2;
				if (i == 4 && c == RGBW_WHITE) {
					/* COPR_ROI5_W:8bit */
					props->copr_roi_r[i][c] = buf[index];
				} else {
					/* COPR_ROI1~5 RGBW:10bit */
					props->copr_roi_r[i][c] = (buf[index] << 8) | buf[index + 1];
				}
			}
			panel_dbg("copr_dsi: copr_roi_r[%d] %d %d %d %d\n",
					i, props->copr_roi_r[i][RGBW_RED],
					props->copr_roi_r[i][RGBW_GREEN],
					props->copr_roi_r[i][RGBW_BLUE],
					props->copr_roi_r[i][RGBW_WHITE]);
		}
	} else if (version == COPR_VER_3 || version == COPR_VER_5) {
		props->copr_ready = buf[0] & 0x01;
		props->cur_cnt = (buf[1] << 8) | buf[2];
		props->cur_copr = (buf[3] << 8) | buf[4];
		props->avg_copr = (buf[5] << 8) | buf[6];
		props->s_cur_cnt = (buf[7] << 8) | buf[8];
		props->s_avg_copr = (buf[9] << 8) | buf[10];
		for (i = 0; i < 5; i++) {
			for (c = 0; c < max_color; c++) {
				index = 11 + i * (max_color * 2) + c * 2;
				props->copr_roi_r[i][c] = (buf[index] << 8) | buf[index + 1];
			}
			panel_dbg("copr_dsi: copr_roi_r[%d] %d %d %d\n",
					i, props->copr_roi_r[i][RED],
					props->copr_roi_r[i][GREEN],
					props->copr_roi_r[i][BLUE]);
		}
		panel_dbg("copr_dsi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready);
	} else if (version == COPR_VER_2) {
		props->copr_ready = ((buf[10] & 0x80) ? 1 : 0);
		props->cur_cnt = (buf[0] << 8) | buf[1];
		props->cur_copr = (buf[2] << 8) | buf[3];
		props->avg_copr = (buf[4] << 8) | buf[5];
		props->s_cur_cnt = (buf[6] << 8) | buf[7];
		props->s_avg_copr = (buf[8] << 8) | buf[9];
		props->comp_copr = (((buf[10] & 0x01) ? 1 : 0) << 8) | buf[11];
		panel_dbg("copr_dsi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d, comp_copr %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready, props->comp_copr);
	} else if (version == COPR_VER_1) {
		props->copr_ready = ((buf[8] & 0x80) ? 1 : 0);
		props->cur_cnt = (buf[0] << 8) | buf[1];
		props->cur_copr = (buf[2] << 8) | buf[3];
		props->avg_copr = (buf[4] << 8) | buf[5];
		props->s_cur_cnt = (buf[6] << 8) | buf[7];
		props->s_avg_copr = (buf[8] << 8) | buf[9];
		panel_dbg("copr_dsi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready);
	} else {
		props->cur_copr = buf[0];
	}

get_copr_error:
	kfree(buf);

	return ret;
}
#endif

static int panel_get_copr(struct copr_info *copr)
{
	struct timespec64 cur_ts, last_ts, delta_ts;
	struct copr_properties *props = &copr->props;
	s64 elapsed_usec;
	int ret;

	if (unlikely(!props->support))
		return -ENODEV;

	ktime_get_ts64(&cur_ts);
	if (props->state != COPR_REG_ON) {
		panel_dbg("copr reg is not on state %d\n", props->state);
		ret = -EINVAL;
		goto get_copr_error;
	}

	if (atomic_read(&copr->stop)) {
		panel_warn("copr_stop\n");
		ret = -EINVAL;
		goto get_copr_error;
	}

#ifdef CONFIG_USDM_COPR_SPI
	panel_read_copr_spi(copr);
#else
	panel_read_copr_dsi(copr);
#endif

	ktime_get_ts64(&last_ts);

	delta_ts = timespec64_sub(last_ts, cur_ts);
	elapsed_usec = timespec64_to_ns(&delta_ts) / 1000;
	panel_dbg("elapsed_usec %lld usec (%lld.%lld %lld.%lld)\n",
			elapsed_usec, timespec64_to_ns(&cur_ts) / 1000000000,
			(timespec64_to_ns(&cur_ts) % 1000000000) / 1000,
			timespec64_to_ns(&last_ts) / 1000000000,
			(timespec64_to_ns(&last_ts) % 1000000000) / 1000);

	return 0;

get_copr_error:
	return ret;
}

bool copr_is_enabled(struct copr_info *copr)
{
	return (copr->props.support && get_copr_reg_copr_en(copr) && copr->props.enable);
}

static int copr_res_init(struct copr_info *copr)
{
	timenval_init(&copr->res);

	return 0;
}

static int copr_res_start(struct copr_info *copr, int value, struct timespec64 cur_ts)
{
	timenval_start(&copr->res, value, cur_ts);

	return 0;
}

int copr_update_start(struct copr_info *copr, int count)
{
	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (atomic_read(&copr->wq.count) < count)
		atomic_set(&copr->wq.count, count);
	wake_up_interruptible_all(&copr->wq.wait);

	return 0;
}

int copr_update_average(struct copr_info *copr)
{
	struct copr_properties *props = &copr->props;
	struct timespec64 cur_ts;
	int ret;
	int cur_copr;
	u32 version = get_copr_ver(copr);

	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (!copr_is_enabled(copr)) {
		panel_dbg("copr disabled\n");
		return -EIO;
	}

	ktime_get_ts64(&cur_ts);
	if (props->state == COPR_UNINITIALIZED) {
		panel_set_copr(copr);
		panel_info("copr register updated\n");
	}

	ret = panel_get_copr(copr);
	if (ret < 0) {
		panel_err("failed to get copr (ret %d)\n", ret);
		return -EINVAL;
	}

	if (version == COPR_VER_2 ||
		version == COPR_VER_3 ||
		version == COPR_VER_5 ||
		version == COPR_VER_6 ||
		version == COPR_VER_0_1) {
#ifdef CONFIG_SUPPORT_COPR_AVG
		ret = panel_clear_copr(copr);
		if (unlikely(ret < 0))
			panel_err("failed to reset copr\n");
		cur_copr = props->avg_copr;
		timenval_update_average(&copr->res, cur_copr, cur_ts);
#else
		cur_copr = props->cur_copr;
		timenval_update_snapshot(&copr->res, cur_copr, cur_ts);
#endif
	} else {
		cur_copr = props->cur_copr;
		timenval_update_snapshot(&copr->res, cur_copr, cur_ts);
	}

	return 0;
}

int copr_get_value(struct copr_info *copr)
{
	struct copr_properties *props = &copr->props;
	int ret;
	int cur_copr;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	panel_mutex_lock(&copr->lock);
	if (!copr_is_enabled(copr)) {
		panel_dbg("copr disabled\n");
		panel_mutex_unlock(&copr->lock);
		return -EIO;
	}

	if (props->state == COPR_UNINITIALIZED) {
		panel_set_copr(copr);
		panel_info("copr register updated\n");
	}

	ret = panel_get_copr(copr);
	if (ret < 0) {
		panel_err("failed to get copr (ret %d)\n", ret);
		panel_mutex_unlock(&copr->lock);
		return -EINVAL;
	}
	cur_copr = props->cur_copr;

	panel_mutex_unlock(&copr->lock);

	return cur_copr;
}

int copr_iter_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out)
{
	struct copr_properties *props = &copr->props;
	struct timespec64 cur_ts;
	int ret;
	int i, c, cur_copr;
	struct copr_reg reg;
	u32 version = get_copr_ver(copr);

	if (unlikely(!copr->props.support))
		return -ENODEV;

	/* update using last value or avg_copr */
	panel_mutex_lock(&copr->lock);
	ret = copr_update_average(copr);
	if (ret < 0) {
		panel_err("failed to update average(ret %d)\n", ret);
		panel_mutex_unlock(&copr->lock);
		return ret;
	}

	if (!copr_is_enabled(copr)) {
		panel_dbg("copr disabled\n");
		panel_mutex_unlock(&copr->lock);
		return -EIO;
	}

	panel_dbg("set roi\n");
	memcpy(&reg, &copr->props.reg, sizeof(reg));
	SET_COPR_REG_GAMMA(copr, 0);
	if (version == COPR_VER_2) {
		for (i = 0; i < size; i++) {
			SET_COPR_REG_ROI(copr, &roi[i], 1);
			SET_COPR_REG_E(copr, 0x300, 0, 0);
			SET_COPR_REG_EC(copr, 0, 0x300, 0);
			panel_set_copr(copr);
			ret = panel_get_copr(copr);
			if (ret < 0) {
				panel_err("failed to get copr (ret %d)\n", ret);
				/* restore r/g/b efficiency & roi */
				memcpy(&copr->props.reg, &reg, sizeof(copr->props.reg));
				panel_mutex_unlock(&copr->lock);
				return -EINVAL;
			}
			out[i * 3 + 0] = props->cur_copr;
			out[i * 3 + 1] = props->comp_copr;

			SET_COPR_REG_E(copr, 0, 0, 0x300);
			SET_COPR_REG_EC(copr, 0, 0, 0);
			panel_set_copr(copr);
			ret = panel_get_copr(copr);
			if (ret < 0) {
				panel_err("failed to get copr (ret %d)\n", ret);
				/* restore r/g/b efficiency & roi */
				memcpy(&copr->props.reg, &reg, sizeof(copr->props.reg));
				panel_mutex_unlock(&copr->lock);
				return -EINVAL;
			}
			out[i * 3 + 2] = props->cur_copr;
		}
	} else if (version == COPR_VER_1) {
		for (i = 0; i < size; i++) {
			for (c = 0; c < MAX_COLOR; c++) {
				SET_COPR_REG_ROI(copr, &roi[i], 1);
				if (c == 0)
					SET_COPR_REG_E(copr, 0x300, 0, 0);
				else if (c == 1)
					SET_COPR_REG_E(copr, 0, 0x300, 0);
				else if (c == 2)
					SET_COPR_REG_E(copr, 0, 0, 0x300);

				panel_set_copr(copr);
				ret = panel_get_copr(copr);
				if (ret < 0) {
					panel_err("failed to get copr (ret %d)\n", ret);
					/* restore r/g/b efficiency & roi */
					memcpy(&copr->props.reg, &reg, sizeof(copr->props.reg));
					panel_mutex_unlock(&copr->lock);
					return -EINVAL;
				}
				out[i * 3 + c] = props->cur_copr;
			}
		}
	}

	/* restore r/g/b efficiency & roi */
	memcpy(&copr->props.reg, &reg, sizeof(copr->props.reg));
#ifdef CONFIG_SUPPORT_COPR_AVG
	if (version == COPR_VER_2 ||
		version == COPR_VER_1) {
		ret = copr_do_sequence(copr, COPR_CLR_CNT_ON_SEQ);
		if (unlikely(ret < 0))
			panel_err("failed to run sequence(%s)\n",
					COPR_CLR_CNT_ON_SEQ);
		msleep(34);
	}
#endif
	panel_set_copr(copr);
	msleep(34);
	ret = panel_get_copr(copr);
	if (ret < 0) {
		panel_err("failed to get copr (ret %d)\n", ret);
		panel_mutex_unlock(&copr->lock);
		return -EINVAL;
	}

	panel_dbg("restore roi\n");

	/*
	 * exclude elapsed time of copr roi snapshot reading
	 * in copr_sum_update. so update last_ts as cur_ts.
	 */
	ktime_get_ts64(&cur_ts);
	cur_copr = props->cur_copr;
	copr_res_start(copr, cur_copr, cur_ts);
	panel_mutex_unlock(&copr->lock);

	return 0;
}

int copr_cur_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out)
{
	struct copr_properties *props = &copr->props;
	int i, c, max_size = 5, ret;
	int max_color = (props->version == COPR_VER_6 || props->version == COPR_VER_0_1) ?
		MAX_RGBW_COLOR : MAX_COLOR;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	panel_mutex_lock(&copr->lock);
	if (!copr_is_enabled(copr)) {
		panel_dbg("copr disabled\n");
		panel_mutex_unlock(&copr->lock);
		return -EIO;
	}

	if (props->state == COPR_UNINITIALIZED) {
		SET_COPR_REG_ROI(copr, roi, (int)min(size, max_size));
		panel_set_copr(copr);
		panel_info("copr register updated\n");
	}

	ret = panel_get_copr(copr);
	if (ret < 0) {
		panel_err("failed to get copr (ret %d)\n", ret);
		panel_mutex_unlock(&copr->lock);
		return -EINVAL;
	}


	for (i = 0; i < (int)min(size, max_size); i++)
		for (c = 0; c < max_color; c++)
			out[i * max_color + c] = props->copr_roi_r[i][c];

	panel_mutex_unlock(&copr->lock);

	return 0;
}

int copr_roi_set_value(struct copr_info *copr, struct copr_roi *roi, int size)
{
	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (!copr_is_enabled(copr)) {
		panel_dbg("copr disabled\n");
		return -EIO;
	}

	panel_mutex_lock(&copr->lock);
	SET_COPR_REG_ROI(copr, roi, (int)min(size, 6));
	panel_set_copr(copr);
	panel_mutex_unlock(&copr->lock);

	return 0;
}

/**
 * copr_roi_get_value - get copr value of each roi
 *
 * @ copr : copr_info
 * @ roi : a pointer of roi array.
 * @ size : size of roi array.
 * @ out : a pointer roi output value to be stored.
 *
 * Get copr snapshot for each roi's r/g/b.
 * This function returns 0 if the average is valid.
 * If not this function returns -ERRNO.
 */
int copr_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out)
{
	u32 version = get_copr_ver(copr);

	if (version > COPR_VER_2 ||
		version == COPR_VER_5 ||
		version == COPR_VER_6 ||
		version == COPR_VER_0_1)
		return copr_cur_roi_get_value(copr, roi, size, out);
	else
		return copr_iter_roi_get_value(copr, roi, size, out);
}

int copr_clear_average(struct copr_info *copr)
{
	timenval_clear_average(&copr->res);

	return 0;
}

/**
 * copr_get_average_and_clear - get copr average.
 *
 * @ copr : copr_info
 * @ index : a index of copr result.
 *
 * Get copr average from sum and elapsed_msec.
 * And clears copr_res's sum and elapsed_msec variables.
 * This function returns 0 if the average is valid.
 * If not this function returns -ERRNO.
 */
int copr_get_average_and_clear(struct copr_info *copr)
{
	int avg;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	panel_mutex_lock(&copr->lock);
	copr_update_average(copr);
	avg = copr->res.avg;
	copr_clear_average(copr);
	panel_mutex_unlock(&copr->lock);

	return avg;
}

#ifdef CONFIG_USDM_COPR_SPI
static int set_spi_gpios(struct panel_device *panel, int en)
{
	int err_num = 0;
	struct spi_device *spi = panel->spi;
	struct copr_spi_gpios *gpio_info = &panel->spi_gpio;

	if (!spi) {
		panel_dbg("spi is null\n");
		return 0;
	}

	panel_info("en : %d\n", en);
	if (en) {
		if (gpio_direction_output(gpio_info->gpio_sck, 0))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_miso))
			goto set_exit;
		err_num++;

		if (gpio_direction_output(gpio_info->gpio_mosi, 0))
			goto set_exit;
		err_num++;

		if (gpio_direction_output(gpio_info->gpio_cs, 0))
			goto set_exit;
	} else {
		if (gpio_direction_input(gpio_info->gpio_sck))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_miso))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_mosi))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_cs))
			goto set_exit;
	}
	return 0;

set_exit:
	panel_err("failed to gpio:%d\n", err_num);
	return -EIO;
}

static int get_spi_gpios_dt(struct panel_device *panel)
{
	int ret = 0;
	struct device_node *np;
	struct spi_device *spi = panel->spi;
	struct copr_spi_gpios *gpio_info = &panel->spi_gpio;

	if (!spi) {
		panel_err("spi or gpio_info is null\n");
		goto error_dt;
	}

	np = spi->master->dev.of_node;
	if (!np) {
		panel_err("dev_of_node is null\n");
		goto error_dt;
	}

	gpio_info->gpio_sck = of_get_named_gpio(np, "gpio-sck", 0);
	if (gpio_info->gpio_sck < 0) {
		panel_err("failed to get gpio_sck from dt\n");
		goto error_dt;
	}

	gpio_info->gpio_miso = of_get_named_gpio(np, "gpio-miso", 0);
	if (gpio_info->gpio_miso < 0) {
		panel_err("failed to get miso from dt\n");
		goto error_dt;
	}

	gpio_info->gpio_mosi = of_get_named_gpio(np, "gpio-mosi", 0);
	if (gpio_info->gpio_mosi < 0) {
		panel_err("failed to get mosi from dt\n");
		goto error_dt;
	}

	gpio_info->gpio_cs = of_get_named_gpio(np, "cs-gpios", 0);
	if (gpio_info->gpio_cs < 0) {
		panel_err("failed to get cs from dt\n");
		goto error_dt;
	}

error_dt:
	return ret;
}
#endif

int copr_enable(struct copr_info *copr)
{
	struct panel_device *panel = to_panel_device(copr);
	struct panel_state *state = &panel->state;
	struct copr_properties *props = &copr->props;
	int ret;
	u32 version = get_copr_ver(copr);

	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (copr_is_enabled(copr)) {
		panel_info("already enabled\n");
		return 0;
	}

#ifdef CONFIG_USDM_COPR_SPI
	if (set_spi_gpios(panel, 1))
		panel_err("failed to set spio gpio\n");
#endif

	panel_info("+\n");
	atomic_set(&copr->stop, 0);
	panel_mutex_lock(&copr->lock);
	copr->props.enable = true;
	if (state->disp_on == PANEL_DISPLAY_ON) {
		/*
		 * TODO : check whether "copr-set" is includued in "init-seq".
		 * If COPR_SET_SEQ is included in INIT_SEQ, set state COPR_REG_ON.
		 * If not, copr state should be COPR_UNINITIALIZED.
		 */
		props->state = COPR_REG_ON;
	}
	if (copr->props.options.check_avg) {
		copr_res_init(copr);
#ifdef CONFIG_SUPPORT_COPR_AVG
		if (version == COPR_VER_1 ||
			version == COPR_VER_2 ||
			version == COPR_VER_3 ||
			version == COPR_VER_5 ||
			version == COPR_VER_6 ||
			version == COPR_VER_0_1) {
			ret = panel_clear_copr(copr);
			if (unlikely(ret < 0))
				panel_err("failed to reset copr\n");
		}
#endif
		copr_update_average(copr);
	}
	panel_mutex_unlock(&copr->lock);

	panel_info("-\n");
#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	panel_send_coprstate_notify(panel->id, PANEL_EVENT_COPR_STATE_ENABLED);
#endif
	return 0;
}

int copr_disable(struct copr_info *copr)
{
#if defined(CONFIG_USDM_COPR_SPI) || IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	struct panel_device *panel = to_panel_device(copr);
#endif
	struct copr_properties *props = &copr->props;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (!copr_is_enabled(copr)) {
		panel_info("already disabled\n");
		return 0;
	}

	panel_info("+\n");
	atomic_set(&copr->stop, 1);
	panel_mutex_lock(&copr->lock);
	if (copr->props.options.check_avg) {
		if (get_copr_ver(copr) < COPR_VER_2)
			copr_update_average(copr);
	}
	if (props->enable) {
		props->enable = false;
		props->state = COPR_UNINITIALIZED;
	}
	panel_mutex_unlock(&copr->lock);
#ifdef CONFIG_USDM_COPR_SPI
	if (set_spi_gpios(panel, 0))
		panel_err("failed to set spio gpio\n");
#endif
	panel_info("-\n");
#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	panel_send_coprstate_notify(panel->id, PANEL_EVENT_COPR_STATE_DISABLED);
#endif
	return 0;
}

static int copr_thread(void *data)
{
	struct copr_info *copr = data;
	int last_value;
	int ret;
	bool should_stop = false;
#ifdef CONFIG_SUPPORT_COPR_AVG
	bool runnable = (get_copr_ver(copr) < COPR_VER_2);
#else
	bool runnable = true;
#endif

	last_value = copr->res.last_value;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(copr->wq.wait,
				(should_stop = copr->wq.should_stop || kthread_should_stop()) ||
				((atomic_read(&copr->wq.count) > 0) &&
				copr_is_enabled(copr) && runnable));

		if (should_stop)
			break;

		if (last_value != copr->res.last_value)
			atomic_dec(&copr->wq.count);

		if (!ret) {
			panel_mutex_lock(&copr->lock);
			copr_update_average(copr);
			last_value = copr->res.last_value;
			panel_mutex_unlock(&copr->lock);
			usleep_range(16660, 16670);
		}
	}

	return 0;
}

static int copr_create_thread(struct copr_info *copr)
{
	if (unlikely(!copr->props.support)) {
		panel_warn("copr unsupported\n");
		return 0;
	}

	copr->wq.should_stop = false;
	copr->wq.thread = kthread_run(copr_thread, copr, "copr-thread");
	if (IS_ERR_OR_NULL(copr->wq.thread)) {
		panel_err("failed to run copr thread\n");
		copr->wq.thread = NULL;
		return PTR_ERR(copr->wq.thread);
	}

	return 0;
}

static int copr_destroy_thread(struct copr_info *copr)
{
	if (unlikely(!copr->props.support)) {
		panel_warn("copr unsupported\n");
		return 0;
	}

	if (IS_ERR_OR_NULL(copr->wq.thread))
		return 0;

	copr->wq.should_stop = true;
	/* wake up waitqueue to stop */
	wake_up_interruptible_all(&copr->wq.wait);
	/* kthread_should_stop() == true */
	kthread_stop(copr->wq.thread);

	return 0;
}

static int copr_fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct copr_info *copr;
	struct fb_event *evdata = data;
	int fb_blank;
	int early_blank;

	switch (event) {
	case FB_EVENT_BLANK:
		early_blank = 0;
		break;
	default:
		return 0;
	}

	copr = container_of(self, struct copr_info, fb_notif);

	fb_blank = *(int *)evdata->data;
	panel_dbg("fb_blank:%d\n", fb_blank);

	if (evdata->info->node != 0)
		return 0;
	if (unlikely(!copr->props.support))
		return 0;

	return 0;
}

static int copr_register_fb(struct copr_info *copr)
{
	memset(&copr->fb_notif, 0, sizeof(copr->fb_notif));
	copr->fb_notif.notifier_call = copr_fb_notifier_callback;
	return fb_register_client(&copr->fb_notif);
}

static int copr_unregister_fb(struct copr_info *copr)
{
	fb_unregister_client(&copr->fb_notif);
	copr->fb_notif.notifier_call = NULL;

	return 0;
}

int copr_prepare(struct panel_device *panel, struct panel_copr_data *copr_data)
{
	struct copr_info *copr;
	int ret;

	if (!panel)
		return -EINVAL;

	if (!copr_data) {
		panel_info("copr is not supported\n");
		return 0;
	}

	copr = &panel->copr;

	panel_mutex_lock(&copr->lock);
	atomic_set(&copr->stop, 0);
	memcpy(&copr->props.reg, &copr_data->reg, sizeof(struct copr_reg));
	copr->props.version = copr_data->version;
	memcpy(&copr->props.options, &copr_data->options, sizeof(struct copr_options));
	memcpy(&copr->props.roi, &copr_data->roi, sizeof(copr->props.roi));
	copr->props.nr_roi = copr_data->nr_roi;

	ret = panel_add_command_from_initdata_maptbl(copr_data->maptbl,
			copr_data->nr_maptbl, &panel->command_initdata_list);
	if (ret < 0) {
		panel_err("failed to panel_add_command_from_initdata_maptbl\n");
		panel_mutex_unlock(&copr->lock);
		return ret;
	}

	ret = panel_add_command_from_initdata_seqtbl(copr_data->seqtbl,
			copr_data->nr_seqtbl, &panel->command_initdata_list);
	if (ret < 0) {
		panel_err("failed to panel_add_command_from_initdata_seqtbl\n");
		panel_mutex_unlock(&copr->lock);
		return ret;
	}

	panel_mutex_unlock(&copr->lock);

	return 0;
}

int copr_unprepare(struct panel_device *panel)
{
	return 0;
}

int copr_probe(struct panel_device *panel, struct panel_copr_data *copr_data)
{
	struct copr_info *copr;

	if (!panel) {
		panel_err("panel(%p) not exist\n", panel);
		return -EINVAL;
	}

	if (!copr_data) {
		panel_warn("copr is not supported\n");
		return 0;
	}

	copr = &panel->copr;

	panel_mutex_lock(&copr->lock);
	init_waitqueue_head(&copr->wq.wait);
	copr->props.support = true;
	copr_register_fb(copr);
#ifdef CONFIG_USDM_COPR_SPI
	get_spi_gpios_dt(panel);
#endif

	if (IS_PANEL_ACTIVE(panel) &&
			get_copr_reg_copr_en(copr)) {
		copr->props.enable = true;
		copr->props.state = COPR_UNINITIALIZED;
		panel_set_copr(copr);
		atomic_set(&copr->wq.count, 5);
		if (copr->props.options.thread_on)
			copr_create_thread(copr);
	}
	panel_mutex_unlock(&copr->lock);

	panel_info("registered successfully\n");

	return 0;
}

int copr_remove(struct panel_device *panel)
{
	struct copr_info *copr;

	if (!panel) {
		panel_err("panel(%p) not exist\n", panel);
		return -EINVAL;
	}

	copr = &panel->copr;
	if (copr->props.options.thread_on)
		copr_destroy_thread(copr);
	panel_mutex_lock(&copr->lock);
	copr->props.support = false;
	copr_unregister_fb(copr);
	panel_mutex_unlock(&copr->lock);

	return 0;
}
