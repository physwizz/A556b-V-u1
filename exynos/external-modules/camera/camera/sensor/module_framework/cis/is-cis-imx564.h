/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Samsung Exynos SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_CIS_IMX564_H
#define IS_CIS_IMX564_H

#include "is-cis.h"

#define IXM564_OB_MARGIN	(60)

#define AEB_IMX564_LUT0	0x0E20
#define AEB_IMX564_LUT1	0x0E50

#define AEB_IMX564_OFFSET_CIT		0x0
#define AEB_IMX564_OFFSET_AGAIN		0x2
#define AEB_IMX564_OFFSET_DGAIN		0x4
#define AEB_IMX564_OFFSET_FLL		0xE

enum sensor_imx564_mode_enum {
	SENSOR_IMX564_4000X3000_86FPS_10BIT = 0,
	SENSOR_IMX564_4000X3000_51FPS_DSG = 1,
	SENSOR_IMX564_4000X3000_53FPS_12BIT_LNFAST = 2,
	SENSOR_IMX564_4000X3000_53FPS_10BIT_LNFAST = 3,
	SENSOR_IMX564_4000X3000_30FPS_10BIT_LN4 = 4,
	SENSOR_IMX564_4000X2252_114FPS_10BIT = 5,
	SENSOR_IMX564_4000X2252_67FPS_DSG = 6,
	SENSOR_IMX564_4000X2252_69FPS_12BIT_LNFAST = 7,
	SENSOR_IMX564_4000X2252_69FPS_10BIT_LNFAST = 8,
	SENSOR_IMX564_4000X2252_39FPS_10BIT_LN4 = 9,
	SENSOR_IMX564_2000X1124_527FPS = 10,
	SENSOR_IMX564_2000X1500_157FPS = 11,
	SENSOR_IMX564_2800X2100_86FPS = 12,
	SENSOR_IMX564_4000X2252_141FPS_10BIT = 13,
	SENSOR_IMX564_4000X3000_60FPS_DSG_AEB = 14, /* AEB + DCG */
	SENSOR_IMX564_4000X3000_86FPS_12BIT = 15,
	SENSOR_IMX564_4000X3000_30FPS_12BIT_LN4 = 16,
	SENSOR_IMX564_MODE_MAX,
};

static const struct sensor_reg_addr sensor_imx564_reg_addr = {
	.fll = 0x0340,
	.fll_aeb_long = AEB_IMX564_LUT0 + AEB_IMX564_OFFSET_FLL,
	.fll_aeb_short = AEB_IMX564_LUT1 + AEB_IMX564_OFFSET_FLL,
	.fll_shifter = 0x3151,
	.cit = 0x0202,
	.cit_aeb_long = AEB_IMX564_LUT0 + AEB_IMX564_OFFSET_CIT,
	.cit_aeb_short = AEB_IMX564_LUT1 + AEB_IMX564_OFFSET_CIT,
	.cit_shifter = 0x3150,
	.again = 0x0204,
	.again_aeb_long = AEB_IMX564_LUT0 + AEB_IMX564_OFFSET_AGAIN,
	.again_aeb_short = AEB_IMX564_LUT1 + AEB_IMX564_OFFSET_AGAIN,
	.dgain = 0x020E,
	.dgain_secondary = 0x0218,
	.dgain_aeb_long = AEB_IMX564_LUT0 + AEB_IMX564_OFFSET_DGAIN,
	.dgain_aeb_short = AEB_IMX564_LUT1 + AEB_IMX564_OFFSET_DGAIN,
	.group_param_hold = 0x0104,
};

struct sensor_imx564_private_data {
	const struct sensor_regs global;
	const struct sensor_regs init_aeb_10;
	const struct sensor_regs init_aeb_12;
};

#define MODE_GROUP_NONE (-1)
enum sensor_imx564_mode_group_enum {
	SENSOR_IMX564_MODE_DEFAULT,
	SENSOR_IMX564_MODE_AEB,
	SENSOR_IMX564_MODE_IDCG,
	SENSOR_IMX564_MODE_AEB_IDCG,
	SENSOR_IMX564_MODE_LN2,
	SENSOR_IMX564_MODE_LN4,
	SENSOR_IMX564_MODE_MODE_GROUP_MAX
};
static u32 sensor_imx564_mode_groups[SENSOR_IMX564_MODE_MODE_GROUP_MAX];

/**
 * Register address & data
 */
#define DATA_IMX564_GPH_HOLD            (0x01)
#define DATA_IMX564_GPH_RELEASE         (0x00)

int sensor_imx564_cis_stream_on(struct v4l2_subdev *subdev);
int sensor_imx564_cis_stream_off(struct v4l2_subdev *subdev);
int sensor_imx564_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again);
int sensor_imx564_cis_update_seamless_mode(struct v4l2_subdev *subdev);
#endif
