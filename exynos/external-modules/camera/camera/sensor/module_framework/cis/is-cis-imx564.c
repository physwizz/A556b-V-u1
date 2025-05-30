// SPDX-License-Identifier: GPL-2.0-or-later
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

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-is-sensor.h>
#include "is-hw.h"
#include "is-core.h"
#include "is-param.h"
#include "is-device-sensor.h"
#include "is-device-sensor-peri.h"
#include "is-resourcemgr.h"
#include "is-dt.h"
#include "is-cis-imx564.h"
#ifndef USE_CAMERA_IMX564_FF
#include "is-cis-imx564-setA-19p2.h"
#else
#include "is-cis-imx564-setB-19p2.h"
#endif
#include "is-helper-ixc.h"

#define SENSOR_NAME "IMX564"

void sensor_imx564_cis_set_mode_group(u32 mode)
{
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_DEFAULT] = mode;
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_AEB] = MODE_GROUP_NONE;
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG] = MODE_GROUP_NONE;
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_AEB_IDCG] = MODE_GROUP_NONE;
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2] = MODE_GROUP_NONE;
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] = MODE_GROUP_NONE;

	switch (mode) {
	case SENSOR_IMX564_4000X3000_86FPS_10BIT:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_AEB] = SENSOR_IMX564_4000X3000_86FPS_10BIT;
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2] = SENSOR_IMX564_4000X3000_53FPS_10BIT_LNFAST;
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] = SENSOR_IMX564_4000X3000_30FPS_10BIT_LN4;
		break;
	case SENSOR_IMX564_4000X3000_53FPS_12BIT_LNFAST:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG] = SENSOR_IMX564_4000X3000_51FPS_DSG;
		break;
	case SENSOR_IMX564_4000X2252_69FPS_12BIT_LNFAST:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG] = SENSOR_IMX564_4000X2252_67FPS_DSG;
		break;
	case SENSOR_IMX564_4000X3000_30FPS_10BIT_LN4:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2] = SENSOR_IMX564_4000X3000_53FPS_10BIT_LNFAST;
		break;
	case SENSOR_IMX564_4000X2252_114FPS_10BIT:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2] = SENSOR_IMX564_4000X2252_69FPS_10BIT_LNFAST;
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] = SENSOR_IMX564_4000X2252_39FPS_10BIT_LN4;
		break;
	case SENSOR_IMX564_4000X2252_39FPS_10BIT_LN4:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2] = SENSOR_IMX564_4000X2252_69FPS_10BIT_LNFAST;
		break;
	case SENSOR_IMX564_4000X3000_53FPS_10BIT_LNFAST:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] = SENSOR_IMX564_4000X3000_30FPS_10BIT_LN4;
		break;
	case SENSOR_IMX564_4000X2252_69FPS_10BIT_LNFAST:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] = SENSOR_IMX564_4000X2252_39FPS_10BIT_LN4;
		break;
	case SENSOR_IMX564_4000X3000_86FPS_12BIT:
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_AEB] = SENSOR_IMX564_4000X3000_86FPS_12BIT;
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] = SENSOR_IMX564_4000X3000_30FPS_12BIT_LN4;
		break;
	}

	info("[%s] default(%d) aeb(%d) idcg(%d) aeb_idcg(%d) ln2(%d) ln4(%d)\n", __func__,
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_DEFAULT],
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_AEB],
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG],
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_AEB_IDCG],
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2],
		sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4]);
}

#ifdef USE_CAMERA_EMBEDDED_HEADER
#define SENSOR_IMX564_PAGE_LENGTH 205
#define SENSOR_IMX564_VALID_TAG 0xA5
#define SENSOR_IMX564_FRAME_ID_PAGE 0
#define SENSOR_IMX564_FRAME_ID_OFFSET 27
#define SENSOR_IMX564_FRAME_COUNT_PAGE 0
#define SENSOR_IMX564_FRAME_COUNT_OFFSET 7

static u32 frame_id_idx = (SENSOR_IMX564_PAGE_LENGTH * SENSOR_IMX564_FRAME_ID_PAGE) + SENSOR_IMX564_FRAME_ID_OFFSET;
static u32 frame_count_idx = (SENSOR_IMX564_PAGE_LENGTH * SENSOR_IMX564_FRAME_COUNT_PAGE) + SENSOR_IMX564_FRAME_COUNT_OFFSET;

static int sensor_imx564_cis_get_frame_id(struct v4l2_subdev *subdev, u8 *embedded_buf, u16 *frame_id)
{
	int ret = 0;

	if (embedded_buf[frame_id_idx-1] == SENSOR_IMX564_VALID_TAG) {
		*frame_id = embedded_buf[frame_id_idx];

		dbg_sensor(1, "%s - frame_count(%d)", __func__, embedded_buf[frame_count_idx]);
		dbg_sensor(1, "%s - frame_id(%d)", __func__, *frame_id);

	} else {
		err("%s : invalid valid tag(%x)", __func__, embedded_buf[frame_id_idx-1]);
		*frame_id = 1;
	}

	return ret;
}
#endif

/*************************************************
 *  [imx564 Analog gain formular]
 *
 *  m0: [0x008c:0x008d] fixed to 0
 *  m1: [0x0090:0x0091] fixed to -1
 *  c0: [0x008e:0x008f] fixed to 1024
 *  c1: [0x0092:0x0093] fixed to 1024
 *  X : [0x0204:0x0205] Analog gain setting value
 *
 *  Analog Gain = (m0 * X + c0) / (m1 * X + c1)
 *              = 1024 / (1024 - X)
 *
 *  Analog Gain Range = 0 to 896 (0dB to 18dB)
 *
 *************************************************/

u32 sensor_imx564_cis_calc_again_code(u32 permille)
{
	return 1024 - (1024000 / permille);
}

u32 sensor_imx564_cis_calc_again_permile(u32 code)
{
	return 1024000 / (1024 - code);
}

/* CIS OPS */
int sensor_imx564_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	ktime_t st = ktime_get();

#if !defined(CONFIG_CAMERA_VENDOR_MCD)
	memset(cis->cis_data, 0, sizeof(cis_shared_data));

	ret = sensor_cis_check_rev(cis);
	if (ret < 0) {
		warn("sensor_imx564_check_rev is fail when cis init");
		return -EINVAL;
	}
#endif

	cis->cis_data->stream_on = false;
	cis->cis_data->cur_width = cis->sensor_info->max_width;
	cis->cis_data->cur_height = cis->sensor_info->max_height;
	cis->cis_data->low_expo_start = 33000;
	cis->cis_data->remosaic_mode = false;
	cis->cis_data->pre_12bit_mode = SENSOR_12BIT_STATE_OFF;
	cis->cis_data->cur_12bit_mode = SENSOR_12BIT_STATE_OFF;
	cis->cis_data->pre_lownoise_mode = IS_CIS_LNOFF;
	cis->cis_data->cur_lownoise_mode = IS_CIS_LNOFF;
	cis->cis_data->pre_hdr_mode = SENSOR_HDR_MODE_SINGLE;
	cis->cis_data->cur_hdr_mode = SENSOR_HDR_MODE_SINGLE;
	cis->need_mode_change = false;
	cis->long_term_mode.sen_strm_off_on_step = 0;
	cis->long_term_mode.sen_strm_off_on_enable = false;
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;
	cis->cis_data->cur_pattern_mode = SENSOR_TEST_PATTERN_MODE_OFF;

	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_DEFAULT] = MODE_GROUP_NONE;
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG] = MODE_GROUP_NONE;
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2] = MODE_GROUP_NONE;
	sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] = MODE_GROUP_NONE;

	cis->cis_data->sens_config_index_pre = SENSOR_IMX564_MODE_MAX;
	cis->cis_data->sens_config_index_cur = 0;
	CALL_CISOPS(cis, cis_data_calculation, subdev, cis->cis_data->sens_config_index_cur);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %lldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx564_cis_deinit(struct v4l2_subdev *subdev)
{
	int ret = 0;

	return ret;
}

static const struct is_cis_log log_imx564[] = {
	{I2C_READ, 16, 0x0A20, 0, "model_id"},
	{I2C_READ, 8, 0x0018, 0, "revision_number"},
	{I2C_READ, 8, 0x0005, 0, "frame_count"},
	{I2C_READ, 8, 0x0100, 0, "0x0100"},
	{I2C_READ, 16, 0x0202, 0, "0x0202"},
	{I2C_READ, 16, 0x0340, 0, "0x0340"},
};

int sensor_imx564_cis_log_status(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = sensor_cis_get_cis(subdev);

	sensor_cis_log_status(cis, log_imx564, ARRAY_SIZE(log_imx564), (char *)__func__);

	return ret;
}

int sensor_imx564_cis_init_aeb(struct is_cis *cis, u32 hwformat)
{
	int ret = 0;
	struct sensor_imx564_private_data *priv = (struct sensor_imx564_private_data *)cis->sensor_info->priv;

	if (hwformat == HW_FORMAT_RAW10) {
		ret |= sensor_cis_write_registers(cis->subdev, priv->init_aeb_10);
		info("[%s] init_aeb_10bit done. ret=%d", __func__, ret);
	} else if (hwformat == HW_FORMAT_RAW12) {
		ret |= sensor_cis_write_registers(cis->subdev, priv->init_aeb_12);
		info("[%s] init_aeb_12bit done. ret=%d", __func__, ret);
	}

	if (ret < 0)
		err("sensor_imx564_cis_init_aeb fail!! %d", ret);

	return ret;
}

int sensor_imx564_cis_check_aeb(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_device_sensor *device;
	struct is_cis *cis = sensor_cis_get_cis(subdev);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	if (cis->cis_data->stream_on == false
		|| is_sensor_g_ex_mode(device) != EX_AEB
		|| sensor_imx564_mode_groups[SENSOR_IMX564_MODE_AEB] == MODE_GROUP_NONE) {
		if (cis->cis_data->cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC) {
			info("%s : current AEB status is enabled. need AEB disable\n", __func__);
			cis->cis_data->pre_hdr_mode = SENSOR_HDR_MODE_SINGLE;
			cis->cis_data->cur_hdr_mode = SENSOR_HDR_MODE_SINGLE;
			/* AEB disable */
			cis->ixc_ops->write8(cis->client, 0x0E00, 0x00);
			info("%s : disable AEB in not support mode\n", __func__);
		}
		return -1; //continue;
	}

	if (cis->cis_data->cur_hdr_mode == cis->cis_data->pre_hdr_mode) {
		if (cis->cis_data->cur_hdr_mode != SENSOR_HDR_MODE_2AEB_2VC)
			return -2; //continue; OFF->OFF
		else
			return 0; //return; ON->ON
	}

	if (cis->cis_data->cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC) {
		info("[%s] enable 2AEB 2VC\n", __func__);
		cis->ixc_ops->write8(cis->client, 0x0E00, 0x02);
	} else {
		info("[%s] disable AEB\n", __func__);
		cis->ixc_ops->write8(cis->client, 0x0E00, 0x00);
	}

	cis->cis_data->pre_hdr_mode = cis->cis_data->cur_hdr_mode;

	info("[%s] done\n", __func__);

	return ret;
}

int sensor_imx564_cis_check_12bit(cis_shared_data *cis_data, u32 *next_mode)
{
	int ret = 0;

	if (sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG] == MODE_GROUP_NONE)
		return -1;

	switch (cis_data->cur_12bit_mode) {
	case SENSOR_12BIT_STATE_REAL_12BIT:
		*next_mode = sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG];
		break;
	case SENSOR_12BIT_STATE_PSEUDO_12BIT:
	default:
		ret = -1;
		break;
	}

	return ret;
}

int sensor_imx564_cis_check_lownoise(cis_shared_data *cis_data, u32 *next_mode)
{
	int ret = 0;
	u32 temp_mode = MODE_GROUP_NONE;

	if ((sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2] == MODE_GROUP_NONE
		&& sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] == MODE_GROUP_NONE))
		return -1;

	switch (cis_data->cur_lownoise_mode) {
	case IS_CIS_LN2:
		temp_mode = sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2];
		break;
	case IS_CIS_LN4:
		temp_mode = sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4];
		break;
	case IS_CIS_LNOFF:
	default:
		break;
	}

	if (temp_mode == MODE_GROUP_NONE)
		ret = -1;

	if (ret == 0)
		*next_mode = temp_mode;

	return ret;
}

int sensor_imx564_cis_update_seamless_mode(struct v4l2_subdev *subdev)
{
	int ret = 0;
	unsigned int mode = 0;
	unsigned int next_mode = 0;
	const struct sensor_cis_mode_info *next_mode_info;
	struct is_cis *cis = sensor_cis_get_cis(subdev);

	mode = cis->cis_data->sens_config_index_cur;

	next_mode = sensor_imx564_mode_groups[SENSOR_IMX564_MODE_DEFAULT];
	if (next_mode == MODE_GROUP_NONE) {
		err("mode group is none");
		return -1;
	}

	if (cis->cis_data->cur_pattern_mode != SENSOR_TEST_PATTERN_MODE_OFF) {
		dbg_sensor(1, "[%s] cur_pattern_mode (%d)", __func__, cis->cis_data->cur_pattern_mode);
		return ret;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	if (sensor_imx564_cis_check_aeb(subdev) == 0)
		goto p_i2c_unlock;

	sensor_imx564_cis_check_lownoise(cis->cis_data, &next_mode);
	sensor_imx564_cis_check_12bit(cis->cis_data, &next_mode);

	if (mode == next_mode || next_mode == MODE_GROUP_NONE)
		goto p_i2c_unlock;

	info("[%s] sensor mode(%d)\n", __func__, next_mode);
	next_mode_info = cis->sensor_info->mode_infos[next_mode];
	ret |= sensor_cis_write_registers(subdev, next_mode_info->setfile);
	if (ret < 0) {
		err("sensor_imx564_set_registers fail!!");
		goto p_i2c_unlock;
	}

	cis->cis_data->sens_config_index_pre = cis->cis_data->sens_config_index_cur;
	cis->cis_data->sens_config_index_cur = next_mode;

	CALL_CISOPS(cis, cis_data_calculation, subdev, next_mode);

	info("[%s] pre(%d)->cur(%d), 12bit[%d] LN[%d] AEB[%d]\n",
		__func__,
		cis->cis_data->sens_config_index_pre, cis->cis_data->sens_config_index_cur,
		cis->cis_data->cur_12bit_mode,
		cis->cis_data->cur_lownoise_mode,
		cis->cis_data->cur_hdr_mode);

	cis->cis_data->pre_12bit_mode = cis->cis_data->cur_12bit_mode;
	cis->cis_data->pre_lownoise_mode = cis->cis_data->cur_lownoise_mode;

p_i2c_unlock:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx564_cis_get_seamless_mode_info(struct v4l2_subdev *subdev)
{
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	cis_shared_data *cis_data = cis->cis_data;
	int ret = 0, cnt = 0;

	if (sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2] != MODE_GROUP_NONE) {
		cis_data->seamless_mode_info[cnt].mode = SENSOR_MODE_LN2;
		sensor_cis_get_mode_info(subdev, sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN2],
			&cis_data->seamless_mode_info[cnt]);
		cnt++;
	}

	if (sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4] != MODE_GROUP_NONE) {
		cis_data->seamless_mode_info[cnt].mode = SENSOR_MODE_LN4;
		sensor_cis_get_mode_info(subdev, sensor_imx564_mode_groups[SENSOR_IMX564_MODE_LN4],
			&cis_data->seamless_mode_info[cnt]);
		cnt++;
	}

	if (sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG] != MODE_GROUP_NONE) {
		cis_data->seamless_mode_info[cnt].mode = SENSOR_MODE_REAL_12BIT;
		sensor_cis_get_mode_info(subdev, sensor_imx564_mode_groups[SENSOR_IMX564_MODE_IDCG],
			&cis_data->seamless_mode_info[cnt]);
		cnt++;
	}

	cis_data->seamless_mode_cnt = cnt;

	return ret;
}

int sensor_imx564_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	u32 ex_mode;
	struct is_device_sensor *device;
	const struct sensor_cis_mode_info *mode_info;
	struct is_cis *cis = sensor_cis_get_cis(subdev);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	if (mode >= cis->sensor_info->mode_count) {
		err("invalid mode(%d)!!", mode);
		return -EINVAL;
	}

	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ex_mode = is_sensor_g_ex_mode(device);
	sensor_imx564_cis_set_mode_group(mode);

	info("[%s] sensor mode(%d) ex_mode(%d)\n", __func__, mode, ex_mode);

	mode_info = cis->sensor_info->mode_infos[mode];

	ret |= sensor_cis_write_registers(subdev, mode_info->setfile);
	if (ret < 0) {
		err("sensor_imx564_set_registers fail!!");
		goto p_err_i2c_unlock;
	}

	cis->cis_data->sens_config_index_pre = mode;

	cis->cis_data->pre_12bit_mode = mode_info->state_12bit;
	cis->cis_data->cur_12bit_mode = mode_info->state_12bit;

	cis->cis_data->pre_lownoise_mode = IS_CIS_LNOFF;
	cis->cis_data->cur_lownoise_mode = IS_CIS_LNOFF;

	cis->cis_data->pre_hdr_mode = SENSOR_HDR_MODE_SINGLE;
	cis->cis_data->cur_hdr_mode = SENSOR_HDR_MODE_SINGLE;
	/* AEB disable */
	ret |= cis->ixc_ops->write8(cis->client, 0x0E00, 0x00);

	info("[%s] mode[%d] 12bit[%d] LN[%d] AEB[%d]\n",
		__func__, mode,
		cis->cis_data->cur_12bit_mode,
		cis->cis_data->cur_lownoise_mode,
		cis->cis_data->cur_hdr_mode);

p_err_i2c_unlock:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	sensor_imx564_cis_update_seamless_mode(subdev);
	sensor_imx564_cis_get_seamless_mode_info(subdev);

	/* sensor_imx564_cis_log_status(subdev); */
	info("[%s] X\n", __func__);

	return ret;
}

int sensor_imx564_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	struct sensor_imx564_private_data *priv = (struct sensor_imx564_private_data *)cis->sensor_info->priv;

	info("[%s] global setting start\n", __func__);
	ret = sensor_cis_write_registers_locked(subdev, priv->global);

	if (ret < 0) {
		err("sensor_imx564_set_registers fail!!");
		return ret;
	}

	info("[%s] global setting done\n", __func__);

	return ret;
}

int sensor_imx564_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u32 hwformat = 0;
	struct is_device_sensor *device;
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	ktime_t st = ktime_get();

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	is_vendor_set_mipi_clock(device);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	/* EMB OFF */
	cis->ixc_ops->write8(cis->client, 0x3101, 0x00);

	/* init_aeb */
	if (is_sensor_g_ex_mode(device) == EX_AEB) {
		hwformat = device->cfg[device->nfi_toggle]->input[CSI_VIRTUAL_CH_0].hwformat;
		sensor_imx564_cis_init_aeb(cis, hwformat);
	}
	/* Sensor stream on */
	info("%s\n", __func__);
	cis->ixc_ops->write8(cis->client, 0x0100, 0x01);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis->cis_data->stream_on = true;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %lldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx564_cis_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u8 cur_frame_count = 0;
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	ktime_t st = ktime_get();

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	if (cis->cis_data->cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC
		&& cis->cis_data->pre_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC) {
		info("[%s] current AEB status is enabled. need AEB disable\n", __func__);
		cis->cis_data->pre_hdr_mode = cis->cis_data->cur_hdr_mode = SENSOR_HDR_MODE_SINGLE;

		/* AEB disable */
		cis->ixc_ops->write8(cis->client, 0x0E00, 0x00);
		info("[%s] disable AEB\n", __func__);
	}

	cis->ixc_ops->read8(cis->client, 0x0005, &cur_frame_count);
	info("%s: frame_count(0x%x)\n", __func__, cur_frame_count);

	cis->ixc_ops->write8(cis->client, 0x0100, 0x00);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis->cis_data->stream_on = false;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %lldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

void sensor_imx564_cis_data_calculation(struct v4l2_subdev *subdev, u32 mode)
{
	u32 frame_valid_us = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	const struct sensor_cis_mode_info *mode_info;
	u64 pclk_hz, valid_height;
	u16 llp;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	if (mode >= cis->sensor_info->mode_count) {
		err("invalid mode(%d)!", mode);
		return;
	}

	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	mode_info = cis->sensor_info->mode_infos[mode];

	pclk_hz = mode_info->pclk;
	llp = mode_info->line_length_pck;

	sensor_cis_data_calculation(subdev, mode);

	if (mode_info->state_12bit == SENSOR_12BIT_STATE_REAL_12BIT) {
		valid_height = (u64)(cis_data->cur_height + IXM564_OB_MARGIN) * 2;

		frame_valid_us = valid_height * llp * 1000 * 1000 / pclk_hz;
		cis_data->frame_valid_us_time = (unsigned int)frame_valid_us;

		cis_data->frame_time
			= (cis_data->line_readOut_time * valid_height / 1000);
		cis_data->rolling_shutter_skew
			= (valid_height - 1) * cis_data->line_readOut_time;

		dbg_sensor(1, "modified - frame_valid_us (%d us), min_frame_us_time(%d us)\n",
			frame_valid_us, cis_data->min_frame_us_time);
		dbg_sensor(1, "modified - frame_time(%d), rolling_shutter_skew(%lld)\n",
			cis_data->frame_time, cis_data->rolling_shutter_skew);
	}
}

int sensor_imx564_cis_set_test_pattern(struct v4l2_subdev *subdev, camera2_sensor_ctl_t *sensor_ctl)
{
	int ret = 0;
	struct is_cis *cis = sensor_cis_get_cis(subdev);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_pattern_mode(%d), testPatternMode(%d)\n", cis->id, __func__,
		cis->cis_data->cur_pattern_mode, sensor_ctl->testPatternMode);

	if (cis->cis_data->cur_pattern_mode != sensor_ctl->testPatternMode) {
		if (sensor_ctl->testPatternMode == SENSOR_TEST_PATTERN_MODE_OFF) {
			info("%s: set DEFAULT pattern! (mode : %d)\n", __func__, sensor_ctl->testPatternMode);

			IXC_MUTEX_LOCK(cis->ixc_lock);
			cis->ixc_ops->write16(cis->client, 0x0600, 0x0000);
			IXC_MUTEX_UNLOCK(cis->ixc_lock);

			cis->cis_data->cur_pattern_mode = sensor_ctl->testPatternMode;
		} else if (sensor_ctl->testPatternMode == SENSOR_TEST_PATTERN_MODE_BLACK) {
			info("%s: set BLACK pattern! (mode :%d), Data : 0x(%x, %x, %x, %x)\n",
				__func__, sensor_ctl->testPatternMode,
				(unsigned short)sensor_ctl->testPatternData[0],
				(unsigned short)sensor_ctl->testPatternData[1],
				(unsigned short)sensor_ctl->testPatternData[2],
				(unsigned short)sensor_ctl->testPatternData[3]);

			IXC_MUTEX_LOCK(cis->ixc_lock);
			cis->ixc_ops->write16(cis->client, 0x0600, 0x0001);
			cis->ixc_ops->write8(cis->client, 0x357E, 0x02);
			IXC_MUTEX_UNLOCK(cis->ixc_lock);

			cis->cis_data->cur_pattern_mode = sensor_ctl->testPatternMode;
		}
	}

	return ret;
}

static struct is_cis_ops cis_ops_imx564 = {
	.cis_init = sensor_imx564_cis_init,
	.cis_deinit = sensor_imx564_cis_deinit,
	.cis_log_status = sensor_imx564_cis_log_status,
	.cis_group_param_hold = sensor_cis_set_group_param_hold,
	.cis_set_global_setting = sensor_imx564_cis_set_global_setting,
	.cis_mode_change = sensor_imx564_cis_mode_change,
	.cis_stream_on = sensor_imx564_cis_stream_on,
	.cis_stream_off = sensor_imx564_cis_stream_off,
	.cis_data_calculation = sensor_imx564_cis_data_calculation,
	.cis_set_exposure_time = sensor_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_cis_get_max_exposure_time,
	.cis_adjust_frame_duration = sensor_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_cis_set_frame_rate,
	.cis_adjust_analog_gain = sensor_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_cis_get_max_analog_gain,
	.cis_calc_again_code = sensor_imx564_cis_calc_again_code, /* imx564 has own code */
	.cis_calc_again_permile = sensor_imx564_cis_calc_again_permile, /* imx564 has own code */
	.cis_set_digital_gain = sensor_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_cis_get_max_digital_gain,
	.cis_calc_dgain_code = sensor_cis_calc_dgain_code,
	.cis_calc_dgain_permile = sensor_cis_calc_dgain_permile,
	.cis_compensate_gain_for_extremely_br = sensor_cis_compensate_gain_for_extremely_br,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_wait_streamon = sensor_cis_wait_streamon,
	.cis_check_rev_on_init = sensor_cis_check_rev_on_init,
	.cis_set_initial_exposure = sensor_cis_set_initial_exposure,
//	.cis_recover_stream_on = sensor_cis_recover_stream_on,
//	.cis_recover_stream_off = sensor_cis_recover_stream_off,
#ifdef USE_CAMERA_EMBEDDED_HEADER
	.cis_get_frame_id = sensor_imx564_cis_get_frame_id,
#endif
	.cis_set_test_pattern = sensor_imx564_cis_set_test_pattern,
	.cis_update_seamless_mode = sensor_imx564_cis_update_seamless_mode,
	.cis_set_flip_mode = sensor_cis_set_flip_mode,
};

DEFINE_I2C_DRIVER_PROBE(cis_imx564_probe_i2c)
{
	int ret;
	u32 mclk_freq_khz;
	struct is_cis *cis;
	struct is_device_sensor_peri *sensor_peri;
	char const *setfile;
	struct device_node *dnode = client->dev.of_node;

	ret = sensor_cis_probe(client, &(client->dev), &sensor_peri, I2C_TYPE);
	if (ret) {
		probe_info("%s: sensor_cis_probe ret(%d)\n", __func__, ret);
		return ret;
	}

	cis = &sensor_peri->cis;
	cis->ctrl_delay = N_PLUS_TWO_FRAME;
	cis->cis_ops = &cis_ops_imx564;
	/* belows are depend on sensor cis. MUST check sensor spec */
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_RG_GB;
	cis->reg_addr = &sensor_imx564_reg_addr;

	ret = of_property_read_string(dnode, "setfile", &setfile);
	if (ret) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	mclk_freq_khz = sensor_peri->module->pdata->mclk_freq_khz;

	if (mclk_freq_khz == 19200) {
#ifndef USE_CAMERA_IMX564_FF
		if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0)
			probe_info("%s setfile_A mclk: 19.2MHz\n", __func__);
		else
			err("%s setfile index out of bound, take default (setfile_A mclk: 19.2MHz)", __func__);

		cis->sensor_info = &sensor_imx564_info_A_19p2;
#else
		if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setB") == 0)
			probe_info("%s setfile_B mclk: 19.2MHz\n", __func__);
		else
			err("%s setfile index out of bound, take default (setfile_B mclk: 19.2MHz)", __func__);

		cis->sensor_info = &sensor_imx564_info_B_19p2;

#endif
	}

	is_vendor_set_mipi_mode(cis);

	probe_info("%s done\n", __func__);

	return ret;
}

PKV_DEFINE_I2C_DRIVER_REMOVE(cis_imx564_remove_i2c)
{
	PKV_DEFINE_I2C_DRIVER_REMOVE_RETURN;
}

static const struct of_device_id sensor_cis_imx564_match[] = {
	{
		.compatible = "samsung,exynos-is-cis-imx564",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx564_match);

static const struct i2c_device_id sensor_cis_imx564_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx564_driver = {
	.probe	= cis_imx564_probe_i2c,
	.remove	= cis_imx564_remove_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx564_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx564_idt
};

#ifdef MODULE
module_driver(sensor_cis_imx564_driver, i2c_add_driver,
	i2c_del_driver)
#else
static int __init sensor_cis_imx564_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_imx564_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_imx564_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx564_init);
#endif

MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: fimc-is");
