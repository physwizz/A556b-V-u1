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
#include "is-cis-3j1.h"
#include "is-vendor.h"
#ifndef CONFIG_CAMERA_VENDOR_MCD
#include "is-cis-3j1-setA.h"
#include "is-cis-3j1-setB.h"
#endif
#include "is-cis-3j1-setA-19p2.h"
#include "is-helper-ixc.h"

#define SENSOR_NAME "S5K3J1"

static const struct v4l2_subdev_ops subdev_ops;

static const u32 *sensor_3j1_global;
static u32 sensor_3j1_global_size;
static const u32 **sensor_3j1_setfiles;
static const u32 *sensor_3j1_setfile_sizes;
static const struct sensor_pll_info_compact **sensor_3j1_pllinfos;
static u32 sensor_3j1_max_setfile_num;
/* settings for initial setfile */
static u32 sensor_3j1_post_init_setting_list[SENSOR_3J1_POST_INIT_SETTING_MAX];
static u32 sensor_3j1_post_init_setting_list_len = 0;

static const u32 *sensor_3j1_dualsync_slave;
static u32 sensor_3j1_dualsync_slave_size;

static const u32 *sensor_3j1_dualsync_single;
static u32 sensor_3j1_dualsync_single_size;

static u32 sensor_3j1_multicam_slave = 0;

int sensor_3j1_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps);

static bool sensor_3j1_cis_is_wdr_mode_on(cis_shared_data *cis_data)
{
	/* TODO: WDR always off for bring-up */
	unsigned int mode = cis_data->sens_config_index_cur;

	if (is_vendor_wdr_mode_on(cis_data) &&
		(mode == SENSOR_3J1_3648X2736_30FPS
		|| mode == SENSOR_3J1_2736X2736_30FPS
		|| mode == SENSOR_3J1_3968X2232_30FPS
		|| mode == SENSOR_3J1_2944X2208_30FPS
		|| mode == SENSOR_3J1_3216X1808_30FPS
		|| mode == SENSOR_3J1_2208X2208_30FPS))
		return true;

	return false;
}

static void sensor_3j1_cis_data_calculation(const struct sensor_pll_info_compact *pll_info_compact, cis_shared_data *cis_data)
{
	u64 vt_pix_clk_hz;
	u32 frame_rate, max_fps, frame_valid_us;

	FIMC_BUG_VOID(!pll_info_compact);

	/* 1. get pclk value from pll info */
	vt_pix_clk_hz = pll_info_compact->pclk;

	dbg_sensor(1, "ext_clock(%d), mipi_datarate(%llu), pclk(%llu)\n",
			pll_info_compact->ext_clk, pll_info_compact->mipi_datarate, pll_info_compact->pclk);

	/* 2. the time of processing one frame calculation (us) */
	cis_data->min_frame_us_time = (((u64)pll_info_compact->frame_length_lines) * pll_info_compact->line_length_pck * 1000
					/ (vt_pix_clk_hz / 1000));
	cis_data->cur_frame_us_time = cis_data->min_frame_us_time;
	cis_data->base_min_frame_us_time = cis_data->min_frame_us_time;

	/* 3. FPS calculation */
	frame_rate = vt_pix_clk_hz / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	dbg_sensor(1, "frame_rate (%d) = vt_pix_clk_hz(%llu) / "
		KERN_CONT "(pll_info_compact->frame_length_lines(%d) * pll_info_compact->line_length_pck(%d))\n",
		frame_rate, vt_pix_clk_hz, pll_info_compact->frame_length_lines, pll_info_compact->line_length_pck);

	/* calculate max fps */
	max_fps = (vt_pix_clk_hz * 10) / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	max_fps = (max_fps % 10 >= 5 ? frame_rate + 1 : frame_rate);

	cis_data->pclk = vt_pix_clk_hz;
	cis_data->max_fps = max_fps;
	cis_data->frame_length_lines = pll_info_compact->frame_length_lines;
	cis_data->line_length_pck = pll_info_compact->line_length_pck;
	cis_data->line_readOut_time = (u64)cis_data->line_length_pck * 1000
					* 1000 * 1000 / cis_data->pclk;
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	/* Frame valid time calcuration */
	frame_valid_us = (u64)cis_data->cur_height * cis_data->line_length_pck
				* 1000 * 1000 / cis_data->pclk;
	cis_data->frame_valid_us_time = (unsigned int)frame_valid_us;

	dbg_sensor(1, "%s\n", __func__);
	dbg_sensor(1, "Sensor size(%d x %d) setting: SUCCESS!\n",
					cis_data->cur_width, cis_data->cur_height);
	dbg_sensor(1, "Frame Valid(us): %d\n", frame_valid_us);
	dbg_sensor(1, "rolling_shutter_skew: %lld\n", cis_data->rolling_shutter_skew);

	dbg_sensor(1, "Fps: %d, max fps(%d)\n", frame_rate, cis_data->max_fps);
	dbg_sensor(1, "min_frame_time(%d us)\n", cis_data->min_frame_us_time);
	dbg_sensor(1, "Pixel rate(Kbps): %d\n", cis_data->pclk / 1000);

	/* Frame period calculation */
	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
		cis_data->frame_time, cis_data->rolling_shutter_skew);

	/* Constant values */
	cis_data->min_fine_integration_time = SENSOR_3J1_FINE_INTEGRATION_TIME_MIN;
	cis_data->max_fine_integration_time = SENSOR_3J1_FINE_INTEGRATION_TIME_MAX;
	cis_data->min_coarse_integration_time = SENSOR_3J1_COARSE_INTEGRATION_TIME_MIN;
	cis_data->max_margin_coarse_integration_time = SENSOR_3J1_COARSE_INTEGRATION_TIME_MAX_MARGIN;
}

static int sensor_3j1_wait_stream_off_status(cis_shared_data *cis_data)
{
	int ret = 0;
	u32 timeout = 0;

	FIMC_BUG(!cis_data);

#define STREAM_OFF_WAIT_TIME 250
	while (timeout < STREAM_OFF_WAIT_TIME) {
		if (cis_data->is_active_area == false &&
				cis_data->stream_on == false) {
	}
		timeout++;
	}

	if (timeout == STREAM_OFF_WAIT_TIME) {
		pr_err("actual stream off wait timeout\n");
		ret = -1;
	}

	return ret;
}

/* CIS OPS */
int sensor_3j1_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	u32 setfile_index = 0;
	cis_setting_info setinfo;
	ktime_t st = ktime_get();

	setinfo.param = NULL;
	setinfo.return_value = 0;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	FIMC_BUG(!cis->cis_data);
#if !defined(CONFIG_CAMERA_VENDOR_MCD)
	memset(cis->cis_data, 0, sizeof(cis_shared_data));

	ret = sensor_cis_check_rev(cis);
	if (ret < 0) {
		warn("sensor_3j1_check_rev is fail when cis init, ret(%d)", ret);
		ret = -EINVAL;
		goto p_err;
	}
#endif

	cis->cis_data->cur_width = SENSOR_3J1_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_3J1_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
#ifdef USE_CAMERA_ADAPTIVE_MIPI
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;
#endif

	sensor_3j1_cis_data_calculation(sensor_3j1_pllinfos[setfile_index], cis->cis_data);

	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_exposure_time, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min exposure time : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_exposure_time, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max exposure time : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_analog_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min again : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_analog_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max again : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_digital_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min dgain : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_digital_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max dgain : %d\n", __func__, setinfo.return_value);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

static const struct is_cis_log log_3j1[] = {
	{I2C_READ, 16, 0x0000, 0, "model_id"},
	{I2C_READ, 16, 0x00DC, 0, "0x00DC"},
	{I2C_READ, 16, 0x00EA, 0, "0x00EA"},
	{I2C_READ, 8, 0x0002, 0, "revision_number"},
	{I2C_READ, 8, 0x0005, 0, "frame_count"},
	{I2C_READ, 8, 0x0100, 0, "0x0100"},
	{I2C_READ, 16, 0x0202, 0, "0x0202"},
	{I2C_READ, 16, 0x0204, 0, "0x0204"},
	{I2C_READ, 16, 0x0340, 0, "0x0340"},
};

int sensor_3j1_cis_log_status(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	sensor_cis_log_status(cis, log_3j1,
			ARRAY_SIZE(log_3j1), (char *)__func__);
p_err:
	return ret;
}

#if USE_GROUP_PARAM_HOLD
static int sensor_3j1_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (hold == cis->cis_data->group_param_hold) {
		pr_debug("already group_param_hold (%d)\n", cis->cis_data->group_param_hold);
		goto p_err;
	}

	ret = cis->ixc_ops->write8(cis->client, 0x0104, hold);
	if (ret < 0)
		goto p_err;

	cis->cis_data->group_param_hold = hold;
	ret = 1;
p_err:
	return ret;
}
#else
static inline int sensor_3j1_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{ return 0; }
#endif

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_3j1_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_3j1_cis_group_param_hold_func(subdev, hold);
	if (ret < 0)
		goto p_err;

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_3j1_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int i;
	struct is_cis *cis = NULL;
	u32 post_set_address;
	u32 post_set_value;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* setfile global setting is at camera entrance */
	info("[%d][%s] global setting enter\n", cis->id, __func__);
	ret = sensor_cis_set_registers(subdev, sensor_3j1_global, sensor_3j1_global_size);
	if (ret < 0) {
		err("[%d]sensor_3j1_set_registers fail!!", cis->id);
		goto p_err;
	}
	info("[%d][%s] global setting done\n", cis->id, __func__);

	for (i = 0; i < sensor_3j1_post_init_setting_list_len; i += 2) {
		post_set_address = sensor_3j1_post_init_setting_list[i];
		post_set_value = sensor_3j1_post_init_setting_list[i+1];

		ret = cis->ixc_ops->write16(cis->client, post_set_address, post_set_value);
		if (ret < 0)
			goto p_err;
	}

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3j1_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	struct is_device_sensor *device;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (unlikely(!device)) {
		err("device sensor is null");
		return -EINVAL;
	}

	if (mode >= sensor_3j1_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	sensor_3j1_cis_data_calculation(sensor_3j1_pllinfos[mode], cis->cis_data);

	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_cis_set_registers(subdev, sensor_3j1_setfiles[mode], sensor_3j1_setfile_sizes[mode]);
	if (ret < 0) {
		err("[%d]sensor_3j1_set_registers fail!!", cis->id);
		goto p_err_i2c_unlock;
	}

	info("[%d][%s] mode changed(%d)\n", cis->id, __func__, mode);

	/* EMB Header off */
	ret = cis->ixc_ops->write8(cis->client, 0x0118, 0x00);
	if (ret < 0){
		err("[%d]EMB header off fail", cis->id);
	}

p_err_i2c_unlock:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	/* make first frame to 24fps for live focus sync issue */
	if (device->image.framerate == 24) {
		sensor_3j1_cis_set_frame_rate(subdev, 24);
	}

	return ret;
}

/* TODO: Sensor set size sequence(sensor done, sensor stop, 3AA done in FW case */
int sensor_3j1_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
{
	int ret = 0;
	bool binning = false;
	u32 ratio_w = 0, ratio_h = 0, start_x = 0, start_y = 0, end_x = 0, end_y = 0;
	u32 even_x= 0, odd_x = 0, even_y = 0, odd_y = 0;
	struct is_cis *cis = NULL;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	if (unlikely(!cis_data)) {
		err("cis data is NULL");
		if (unlikely(!cis->cis_data)) {
			ret = -EINVAL;
			goto p_err;
		} else {
			cis_data = cis->cis_data;
		}
	}

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* Wait actual stream off */
	ret = sensor_3j1_wait_stream_off_status(cis_data);
	if (ret) {
		err("Must stream off\n");
		ret = -EINVAL;
		goto p_err;
	}

	binning = cis_data->binning;
	if (binning) {
		ratio_w = (SENSOR_3J1_MAX_WIDTH / cis_data->cur_width);
		ratio_h = (SENSOR_3J1_MAX_HEIGHT / cis_data->cur_height);
	} else {
		ratio_w = 1;
		ratio_h = 1;
	}

	if (((cis_data->cur_width * ratio_w) > SENSOR_3J1_MAX_WIDTH) ||
		((cis_data->cur_height * ratio_h) > SENSOR_3J1_MAX_HEIGHT)) {
		err("Config max sensor size over~!!\n");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	/* 1. page_select */
	ret = cis->ixc_ops->write16(cis->client, 0x6028, 0x2000);
	if (ret < 0)
		 goto p_err_i2c_unlock;

	/* 2. pixel address region setting */
	start_x = ((SENSOR_3J1_MAX_WIDTH - cis_data->cur_width * ratio_w) / 2) & (~0x1);
	start_y = ((SENSOR_3J1_MAX_HEIGHT - cis_data->cur_height * ratio_h) / 2) & (~0x1);
	end_x = start_x + (cis_data->cur_width * ratio_w - 1);
	end_y = start_y + (cis_data->cur_height * ratio_h - 1);

	if (!(end_x & (0x1)) || !(end_y & (0x1))) {
		err("Sensor pixel end address must odd\n");
		ret = -EINVAL;
		goto p_err_i2c_unlock;
	}

	ret = cis->ixc_ops->write16(cis->client, 0x0344, start_x);
	if (ret < 0)
		 goto p_err_i2c_unlock;
	ret = cis->ixc_ops->write16(cis->client, 0x0346, start_y);
	if (ret < 0)
		 goto p_err_i2c_unlock;
	ret = cis->ixc_ops->write16(cis->client, 0x0348, end_x);
	if (ret < 0)
		 goto p_err_i2c_unlock;
	ret = cis->ixc_ops->write16(cis->client, 0x034A, end_y);
	if (ret < 0)
		 goto p_err_i2c_unlock;

	/* 3. output address setting */
	ret = cis->ixc_ops->write16(cis->client, 0x034C, cis_data->cur_width);
	if (ret < 0)
		 goto p_err_i2c_unlock;
	ret = cis->ixc_ops->write16(cis->client, 0x034E, cis_data->cur_height);
	if (ret < 0)
		 goto p_err_i2c_unlock;

	/* If not use to binning, sensor image should set only crop */
	if (!binning) {
		dbg_sensor(1, "Sensor size set is not binning\n");
		goto p_err_i2c_unlock;
	}

	/* 4. sub sampling setting */
	even_x = 1;	/* 1: not use to even sampling */
	even_y = 1;
	odd_x = (ratio_w * 2) - even_x;
	odd_y = (ratio_h * 2) - even_y;

	ret = cis->ixc_ops->write16(cis->client, 0x0380, even_x);
	if (ret < 0)
		 goto p_err_i2c_unlock;
	ret = cis->ixc_ops->write16(cis->client, 0x0382, odd_x);
	if (ret < 0)
		 goto p_err_i2c_unlock;
	ret = cis->ixc_ops->write16(cis->client, 0x0384, even_y);
	if (ret < 0)
		 goto p_err_i2c_unlock;
	ret = cis->ixc_ops->write16(cis->client, 0x0386, odd_y);
	if (ret < 0)
		 goto p_err_i2c_unlock;

	/* 5. binnig setting */
	ret = cis->ixc_ops->write8(cis->client, 0x0900, binning);	/* 1:  binning enable, 0: disable */
	if (ret < 0)
		goto p_err_i2c_unlock;
	ret = cis->ixc_ops->write8(cis->client, 0x0901, (ratio_w << 4) | ratio_h);
	if (ret < 0)
		goto p_err_i2c_unlock;

	/* 6. scaling setting: but not use */
	/* scaling_mode (0: No scaling, 1: Horizontal, 2: Full) */
	ret = cis->ixc_ops->write16(cis->client, 0x0400, 0x0000);
	if (ret < 0)
		goto p_err_i2c_unlock;
	/* down_scale_m: 1 to 16 upwards (scale_n: 16(fixed))
	down scale factor = down_scale_m / down_scale_n */
	ret = cis->ixc_ops->write16(cis->client, 0x0404, 0x0010);
	if (ret < 0)
		goto p_err_i2c_unlock;

	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis->cis_data->rolling_shutter_skew = (cis->cis_data->cur_height - 1) * cis->cis_data->line_readOut_time;
	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
		cis->cis_data->frame_time, cis->cis_data->rolling_shutter_skew);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_i2c_unlock:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_device_sensor *device;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (unlikely(!device)) {
		err("device sensor is null");
		return -EINVAL;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	is_vendor_set_mipi_clock(device);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_3j1_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream on");

	msleep(8);

	/* Sensor stream on */
	if (sensor_3j1_multicam_slave == 1) { /* Mirror Flip */
		if (0) {// always single mode if (test_bit(cis->ixc_ops->FRONT_START, &(core->sensor[0].state))) {
			info("[%d][%s] dual sync slave mode\n", cis->id, __func__);
			ret = sensor_cis_set_registers(subdev, sensor_3j1_dualsync_slave, sensor_3j1_dualsync_slave_size);
			cis->cis_data->dual_slave = true;
		} else {
			info("[%d][%s] dual sync single mode\n", cis->id, __func__);
			ret = sensor_cis_set_registers(subdev, sensor_3j1_dualsync_single, sensor_3j1_dualsync_single_size);
			cis->cis_data->dual_slave = false;
		}

		if (ret < 0) {
			err("[%d]3j1 dual sync fail", cis->id);
			goto p_err;
		}
		cis->ixc_ops->write16(cis->client, 0x0100, 0x103);
	} else {
		/* Dual sync Master setting start */
		cis->ixc_ops->write16(cis->client, 0x0A70, 0x0001);
		cis->ixc_ops->write16(cis->client, 0x0A72, 0x0100);
		cis->ixc_ops->write8(cis->client, 0x0100, 0x01);
	}

	if (sensor_3j1_cis_is_wdr_mode_on(cis_data))
		cis->ixc_ops->write8(cis->client, 0x021E, 0x01);
	else
		cis->ixc_ops->write8(cis->client, 0x021E, 0x00);

	info("[%d]%s\n", cis->id, __func__);

	cis_data->stream_on = true;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_3j1_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream off");

	/* Sensor stream off */
	cis->ixc_ops->write8(cis->client, 0x0100, 0x00);

	info("[%d]%s\n", cis->id, __func__);

	cis_data->stream_on = false;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u64 vt_pic_clk_freq_khz = 0;
	u16 long_coarse_int = 0;
	u16 short_coarse_int = 0;
	u32 line_length_pck = 0;
	u32 min_fine_int = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_exposure);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if ((target_exposure->long_val <= 0) || (target_exposure->short_val <= 0)) {
		err("[%d][%s] invalid target exposure(%d, %d)\n", cis->id, __func__,
				target_exposure->long_val, target_exposure->short_val);
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), target long(%d), short(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, target_exposure->long_val, target_exposure->short_val);

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;

	dbg_sensor(1, "[MOD:D:%d] %s, vt_pic_clk_freq_khz (%d), line_length_pck(%d), min_fine_int (%d)\n",
		cis->id, __func__, vt_pic_clk_freq_khz, line_length_pck, min_fine_int);

	long_coarse_int = ((target_exposure->long_val * vt_pic_clk_freq_khz) / 1000 - min_fine_int) / line_length_pck;
	short_coarse_int = ((target_exposure->short_val * vt_pic_clk_freq_khz) / 1000 - min_fine_int) / line_length_pck;

	if (long_coarse_int > cis_data->max_coarse_integration_time) {
		long_coarse_int = cis_data->max_coarse_integration_time;
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) max\n", cis->id, __func__,
			cis_data->sen_vsync_count, long_coarse_int);
	}

	if (short_coarse_int > cis_data->max_coarse_integration_time) {
		short_coarse_int = cis_data->max_coarse_integration_time;
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) max\n", cis->id, __func__,
			cis_data->sen_vsync_count, short_coarse_int);
	}

	if (long_coarse_int < cis_data->min_coarse_integration_time) {
		long_coarse_int = cis_data->min_coarse_integration_time;
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min\n", cis->id, __func__,
			cis_data->sen_vsync_count, long_coarse_int);
	}

	if (short_coarse_int < cis_data->min_coarse_integration_time) {
		short_coarse_int = cis_data->min_coarse_integration_time;
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) min\n", cis->id, __func__,
			cis_data->sen_vsync_count, short_coarse_int);
	}

	if (short_coarse_int > long_coarse_int) {
		dbg_sensor(1, "[MOD:D:%d] %s, long coarse(%d), short coarse(%d) max\n", cis->id, __func__,
			long_coarse_int, short_coarse_int);
		long_coarse_int = short_coarse_int;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, frame_length_lines(%#x), long_coarse_int %#x, short_coarse_int %#x\n",
		cis->id, __func__, cis_data->frame_length_lines, long_coarse_int, short_coarse_int);

	cis_data->cur_long_exposure_coarse = long_coarse_int;
	cis_data->cur_short_exposure_coarse = short_coarse_int;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	/* WDR mode */
	if (sensor_3j1_cis_is_wdr_mode_on(cis_data)) {
		cis->ixc_ops->write8(cis->client, 0x021E, 0x01);
	} else {
		cis->ixc_ops->write8(cis->client, 0x021E, 0x00);
	}

	/* Short exposure */
	ret = cis->ixc_ops->write16(cis->client, 0x0202, short_coarse_int);
	if (ret < 0)
		goto p_err_i2c_unlock;

	/* Long exposure */
	if (sensor_3j1_cis_is_wdr_mode_on(cis_data)) {
		ret = cis->ixc_ops->write16(cis->client, 0x0226, long_coarse_int);
		if (ret < 0)
			goto p_err_i2c_unlock;
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	cis_shared_data *cis_data = NULL;
	u32 min_integration_time = 0;
	u32 min_coarse = 0;
	u32 min_fine = 0;
	u64 vt_pic_clk_freq_khz = 0;
	u32 line_length_pck = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	if (vt_pic_clk_freq_khz == 0) {
		pr_err("[%d] %s, Invalid vt_pic_clk_freq_khz(%d)\n", cis->id, __func__, vt_pic_clk_freq_khz);
		goto p_err;
	}
	line_length_pck = cis_data->line_length_pck;
	min_coarse = cis_data->min_coarse_integration_time;
	min_fine = cis_data->min_fine_integration_time;

	min_integration_time = (u32)((u64)((line_length_pck * min_coarse) + min_fine) * 1000 / vt_pic_clk_freq_khz);
	*min_expo = min_integration_time;

	dbg_sensor(1, "[%s] min integration time %d\n", __func__, min_integration_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_3j1_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 max_integration_time = 0;
	u32 max_coarse_margin = 0;
	u32 max_fine_margin = 0;
	u32 max_coarse = 0;
	u32 max_fine = 0;
	u64 vt_pic_clk_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	if (vt_pic_clk_freq_khz == 0) {
		pr_err("[MOD:D:%d] %s, Invalid vt_pic_clk_freq_khz(%d)\n", cis->id, __func__, vt_pic_clk_freq_khz);
		goto p_err;
	}
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = cis_data->frame_length_lines;

	max_coarse_margin = cis_data->max_margin_coarse_integration_time;
	max_fine_margin = line_length_pck - cis_data->min_fine_integration_time;
	max_coarse = frame_length_lines - max_coarse_margin;
	max_fine = cis_data->max_fine_integration_time;

	max_integration_time = (u32)((u64)((line_length_pck * max_coarse) + max_fine) * 1000 / vt_pic_clk_freq_khz);

	*max_expo = max_integration_time;

	/* TODO: Is this values update hear? */
	cis_data->max_margin_fine_integration_time = max_fine_margin;
	cis_data->max_coarse_integration_time = max_coarse;

	dbg_sensor(1, "[%s] max integration time %d, max margin fine integration %d, max coarse integration %d\n",
			__func__, max_integration_time, cis_data->max_margin_fine_integration_time, cis_data->max_coarse_integration_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_3j1_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
						u32 input_exposure_time,
						u32 *target_duration)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u64 vt_pic_clk_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	u32 frame_duration = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_duration);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	if (input_exposure_time == 0) {
		input_exposure_time  = cis_data->min_frame_us_time;
		info("[%d][%s] Not proper exposure time(0), so apply min frame duration to exposure time forcely!!!(%d)\n",
			cis->id, __func__, cis_data->min_frame_us_time);
	}

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = (((vt_pic_clk_freq_khz * input_exposure_time) / 1000) / line_length_pck);
	frame_length_lines += cis_data->max_margin_coarse_integration_time;

	frame_duration = (u32)(((u64)frame_length_lines * line_length_pck) * 1000 / vt_pic_clk_freq_khz);

	dbg_sensor(1, "[%s] input exp(%d), adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, input_exposure_time, frame_duration, cis_data->min_frame_us_time);
	dbg_sensor(1, "[%s] adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, frame_duration, cis_data->min_frame_us_time);

	*target_duration = MAX(frame_duration, cis_data->min_frame_us_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3j1_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u64 vt_pic_clk_freq_khz = 0;
	u32 line_length_pck = 0;
	u16 frame_length_lines = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	if (frame_duration < cis_data->min_frame_us_time) {
		dbg_sensor(1, "frame duration is less than min(%d)\n", frame_duration);
		frame_duration = cis_data->min_frame_us_time;
	}

	vt_pic_clk_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;

	frame_length_lines = (u16)((vt_pic_clk_freq_khz * frame_duration) / (line_length_pck * 1000));

	dbg_sensor(1, "[MOD:D:%d] %s, vt_pic_clk_freq_khz(%#x) frame_duration = %d us,"
			KERN_CONT "(line_length_pck%#x), frame_length_lines(%#x)\n",
			cis->id, __func__, vt_pic_clk_freq_khz, frame_duration,
			line_length_pck, frame_length_lines);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	ret = cis->ixc_ops->write16(cis->client, 0x0340, frame_length_lines);
	if (ret < 0)
		goto p_err_i2c_unlock;

	cis_data->cur_frame_us_time = frame_duration;
	cis_data->frame_length_lines = frame_length_lines;
	cis_data->max_coarse_integration_time = cis_data->frame_length_lines - cis_data->max_margin_coarse_integration_time;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 frame_duration = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	if (min_fps > cis_data->max_fps) {
		err("[%d] %s, request FPS is too high(%d), set to max(%d)\n",
			cis->id, __func__, min_fps, cis_data->max_fps);
		min_fps = cis_data->max_fps;
	}

	if (min_fps == 0) {
		err("[%d] %s, request FPS is 0, set to min FPS(1)\n",
			cis->id, __func__);
		min_fps = 1;
	}

	frame_duration = (1 * 1000 * 1000) / min_fps;

	dbg_sensor(1, "[MOD:D:%d] %s, set FPS(%d), frame duration(%d)\n",
			cis->id, __func__, min_fps, frame_duration);

	ret = sensor_3j1_cis_set_frame_duration(subdev, frame_duration);
	if (ret < 0) {
		err("[%d] %s, set frame duration is fail(%d)\n",
			cis->id, __func__, ret);
		goto p_err;
	}

	cis_data->min_frame_us_time = MAX(frame_duration, cis_data->base_min_frame_us_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:

	return ret;
}

int sensor_3j1_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 again_code = 0;
	u32 again_permile = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_permile);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	again_code = sensor_cis_calc_again_code(input_again);

	if (again_code > cis_data->max_analog_gain[0]) {
		again_code = cis_data->max_analog_gain[0];
	} else if (again_code < cis_data->min_analog_gain[0]) {
		again_code = cis_data->min_analog_gain[0];
	}

	again_permile = sensor_cis_calc_again_permile(again_code);

	dbg_sensor(1, "[%s] min again(%d), max(%d), input_again(%d), code(%d), permile(%d)\n", __func__,
			cis_data->max_analog_gain[0],
			cis_data->min_analog_gain[0],
			input_again,
			again_code,
			again_permile);

	*target_permile = again_permile;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3j1_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 short_gain = 0;
	u16 long_gain = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	short_gain = (u16)sensor_cis_calc_again_code(again->short_val);
	long_gain = (u16)sensor_cis_calc_again_code(again->long_val);

	if (long_gain < cis->cis_data->min_analog_gain[0]) {
		info("[%d][%s] not proper long_gain value, reset to min_analog_gain\n", cis->id, __func__);
		long_gain = cis->cis_data->min_analog_gain[0];
	}
	if (long_gain > cis->cis_data->max_analog_gain[0]) {
		info("[%d][%s] not proper long_gain value, reset to max_analog_gain\n", cis->id, __func__);
		long_gain = cis->cis_data->max_analog_gain[0];
	}

	if (short_gain < cis->cis_data->min_analog_gain[0]) {
		info("[%d][%s] not proper short_gain value, reset to min_analog_gain\n", cis->id, __func__);
		short_gain = cis->cis_data->min_analog_gain[0];
	}
	if (short_gain > cis->cis_data->max_analog_gain[0]) {
		info("[%d][%s] not proper short_gain value, reset to max_analog_gain\n", cis->id, __func__);
		short_gain = cis->cis_data->max_analog_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d),"
		KERN_CONT "input short gain = %d us, input long gain = %d us,"
		KERN_CONT "analog short gain(%#x), analog short gain(%#x)\n",
		cis->id, __func__, cis->cis_data->sen_vsync_count,
		again->short_val, again->long_val,
		short_gain, long_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	/* Short analog gain */
	ret = cis->ixc_ops->write16(cis->client, 0x0204, short_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

	/*
	 * 3j1 sensor have to setting a long analog gain
	 * This function check a sensor guide by sensor development team
	 */
	/* Long analog gain */
	if (sensor_3j1_cis_is_wdr_mode_on(cis_data)) {
		if (long_gain != short_gain)	/* Request it by IQ team*/
			long_gain = short_gain;
		ret = cis->ixc_ops->write16(cis->client, 0x0206, long_gain);
		if (ret < 0)
			goto p_err_i2c_unlock;
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 analog_gain = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x0204, &analog_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

	*again = sensor_cis_calc_again_permile(analog_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_again = %d us, analog_gain(%#x)\n",
			cis->id, __func__, *again, analog_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	WARN_ON(!subdev);
	WARN_ON(!min_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->min_analog_gain[0] = 0x20; /* x1, gain=x/0x20 */
	cis_data->min_analog_gain[1] = sensor_cis_calc_again_permile(cis_data->min_analog_gain[0]);

	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_analog_gain[0], cis_data->min_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3j1_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	WARN_ON(!subdev);
	WARN_ON(!max_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->max_analog_gain[0] = 0x200; /* x16, gain=x/0x20 */
	cis_data->max_analog_gain[1] = sensor_cis_calc_again_permile(cis_data->max_analog_gain[0]);

	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_analog_gain[0], cis_data->max_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3j1_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 long_gain = 0;
	u16 short_gain = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	long_gain = (u16)sensor_cis_calc_dgain_code(dgain->long_val);
	short_gain = (u16)sensor_cis_calc_dgain_code(dgain->short_val);

	if (long_gain < cis->cis_data->min_digital_gain[0]) {
		info("[%d][%s] not proper long_gain value, reset to min_digital_gain\n", cis->id, __func__);
		long_gain = cis->cis_data->min_digital_gain[0];
	}
	if (long_gain > cis->cis_data->max_digital_gain[0]) {
		info("[%d][%s] not proper long_gain value, reset to max_digital_gain\n", cis->id, __func__);
		long_gain = cis->cis_data->max_digital_gain[0];
	}

	if (short_gain < cis->cis_data->min_digital_gain[0]) {
		info("[%d][%s] not proper short_gain value, reset to min_digital_gain\n", cis->id, __func__);
		short_gain = cis->cis_data->min_digital_gain[0];
	}
	if (short_gain > cis->cis_data->max_digital_gain[0]) {
		info("[%d][%s] not proper short_gain value, reset to max_digital_gain\n", cis->id, __func__);
		short_gain = cis->cis_data->max_digital_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s, input_dgain = %d/%d us, long_gain(%#x), short_gain(%#x)\n",
			cis->id, __func__, dgain->long_val, dgain->short_val, long_gain, short_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	/* Short digital gain */
	ret = cis->ixc_ops->write16(cis->client, 0x020E, short_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

	/* Long digital gain */
	if (sensor_3j1_cis_is_wdr_mode_on(cis_data)) {
		ret = cis->ixc_ops->write16(cis->client, 0x0230, long_gain);
		if (ret < 0)
			goto p_err_i2c_unlock;
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 digital_gain = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c_unlock;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x020E, &digital_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

	*dgain = sensor_cis_calc_dgain_permile(digital_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_dgain = %d us, digital_gain(%#x)\n",
			cis->id, __func__, *dgain, digital_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_i2c_unlock:
	if (hold > 0) {
		hold = sensor_3j1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_3j1_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	WARN_ON(!subdev);
	WARN_ON(!min_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->min_digital_gain[0] = 0x100;
	cis_data->min_digital_gain[1] = sensor_cis_calc_dgain_permile(cis_data->min_digital_gain[0]);

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_digital_gain[0], cis_data->min_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3j1_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	WARN_ON(!subdev);
	WARN_ON(!max_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->max_digital_gain[0] = 0x1000;
	cis_data->max_digital_gain[1] = sensor_cis_calc_dgain_permile(cis_data->max_digital_gain[0]);

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_digital_gain[0], cis_data->max_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus\n", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

static struct is_cis_ops cis_ops = {
	.cis_init = sensor_3j1_cis_init,
	.cis_log_status = sensor_3j1_cis_log_status,
	.cis_group_param_hold = sensor_3j1_cis_group_param_hold,
	.cis_set_global_setting = sensor_3j1_cis_set_global_setting,
	.cis_mode_change = sensor_3j1_cis_mode_change,
	.cis_set_size = sensor_3j1_cis_set_size,
	.cis_stream_on = sensor_3j1_cis_stream_on,
	.cis_stream_off = sensor_3j1_cis_stream_off,
	.cis_set_exposure_time = sensor_3j1_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_3j1_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_3j1_cis_get_max_exposure_time,
	.cis_adjust_frame_duration = sensor_3j1_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_3j1_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_3j1_cis_set_frame_rate,
	.cis_adjust_analog_gain = sensor_3j1_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_3j1_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_3j1_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_3j1_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_3j1_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_3j1_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_3j1_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_3j1_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_3j1_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_cis_compensate_gain_for_extremely_br,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_wait_streamon = sensor_cis_wait_streamon,
	.cis_check_rev_on_init = sensor_cis_check_rev_on_init,
	.cis_set_initial_exposure = sensor_cis_set_initial_exposure,
};

static int cis_3j1_probe_i2c(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	u32 mclk_freq_khz;
	struct is_cis *cis;
	struct is_device_sensor_peri *sensor_peri;
	const u32 *post_init_setting_list_spec;
	char const *setfile;
	struct device_node *dnode = client->dev.of_node;
	int i;
	int index;
	const int *verify_sensor_mode = NULL;
	int verify_sensor_mode_size = 0;

	ret = sensor_cis_probe(client, &(client->dev), &sensor_peri, I2C_TYPE);
	if (ret) {
		probe_info("%s: sensor_cis_probe ret(%d)\n", __func__, ret);
		return ret;
	}

	ret = of_property_read_u32(dnode, "multicam_slave", &sensor_3j1_multicam_slave);
	if (ret) {
		sensor_3j1_multicam_slave = 0;
		probe_info("3j1 is master (%d)", ret);
		ret = 0;
	} else {
		probe_info("sensor_3j1_multicam_slave %d", sensor_3j1_multicam_slave);
	}

	cis = &sensor_peri->cis;
	cis->ctrl_delay = N_PLUS_TWO_FRAME;
	cis->cis_ops = &cis_ops;
	/* belows are depend on sensor cis. MUST check sensor spec */
	if (sensor_3j1_multicam_slave == 1) {
		cis->bayer_order = OTF_INPUT_ORDER_BAYER_GB_RG;
	} else {
		cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;
	}

	if (of_property_read_string(dnode, "setfile", &setfile)) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	post_init_setting_list_spec = of_get_property(dnode, "post_init_setting_list",
		&sensor_3j1_post_init_setting_list_len);
	if (post_init_setting_list_spec) {
		sensor_3j1_post_init_setting_list_len /= (unsigned int)sizeof(*post_init_setting_list_spec);

		BUG_ON(sensor_3j1_post_init_setting_list_len > SENSOR_3J1_POST_INIT_SETTING_MAX);

		ret = of_property_read_u32_array(dnode, "post_init_setting_list",
			sensor_3j1_post_init_setting_list, sensor_3j1_post_init_setting_list_len);
		if (ret)
			err("sensor_3j1_post_init_setting_list read is fail(%d)", ret);
	}

	mclk_freq_khz = sensor_peri->module->pdata->mclk_freq_khz;

	if (mclk_freq_khz == 19200) {
		if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0)
			probe_info("%s setfile_A mclk: 19.2MHz\n", __func__);
		else
			err("%s setfile index out of bound, take default (setfile_A mclk: 19.2MHz)", __func__);

		sensor_3j1_global = sensor_3j1_setfile_A_Global_19p2;
		sensor_3j1_global_size = ARRAY_SIZE(sensor_3j1_setfile_A_Global_19p2);
		sensor_3j1_setfiles = sensor_3j1_setfiles_A_19p2;
		sensor_3j1_setfile_sizes = sensor_3j1_setfile_A_sizes_19p2;
		sensor_3j1_pllinfos = sensor_3j1_pllinfos_A_19p2;
		sensor_3j1_max_setfile_num = ARRAY_SIZE(sensor_3j1_setfiles_A_19p2);
		sensor_3j1_dualsync_slave = sensor_3j1_dual_slave_settings_19p2;
		sensor_3j1_dualsync_slave_size = ARRAY_SIZE(sensor_3j1_dual_slave_settings_19p2);
		sensor_3j1_dualsync_single = sensor_3j1_dual_single_settings_19p2;
		sensor_3j1_dualsync_single_size = ARRAY_SIZE(sensor_3j1_dual_single_settings_19p2);
	}
#ifndef CONFIG_CAMERA_VENDOR_MCD
	else {
		if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0) {
			probe_info("%s setfile_A mclk: 26MHz\n", __func__);
			sensor_3j1_global = sensor_3j1_setfile_A_Global;
			sensor_3j1_global_size = ARRAY_SIZE(sensor_3j1_setfile_A_Global);
			sensor_3j1_setfiles = sensor_3j1_setfiles_A;
			sensor_3j1_setfile_sizes = sensor_3j1_setfile_A_sizes;
			sensor_3j1_pllinfos = sensor_3j1_pllinfos_A;
			sensor_3j1_max_setfile_num = ARRAY_SIZE(sensor_3j1_setfiles_A);
			sensor_3j1_dualsync_slave = sensor_3j1_dual_slave_settings;
			sensor_3j1_dualsync_slave_size = ARRAY_SIZE(sensor_3j1_dual_slave_settings);
			sensor_3j1_dualsync_single = sensor_3j1_dual_single_settings;
			sensor_3j1_dualsync_single_size = ARRAY_SIZE(sensor_3j1_dual_single_settings);
			verify_sensor_mode = sensor_3j1_setfile_A_verify_sensor_mode;
			verify_sensor_mode_size = ARRAY_SIZE(sensor_3j1_setfile_A_verify_sensor_mode);
		} else if (strcmp(setfile, "setB") == 0) {
			/* setB for 3J1 folded zoom (TELE) */
			probe_info("%s setfile_B\n", __func__);
			sensor_3j1_global = sensor_3j1_setfile_B_Global;
			sensor_3j1_global_size = ARRAY_SIZE(sensor_3j1_setfile_B_Global);
			sensor_3j1_setfiles = sensor_3j1_setfiles_B;
			sensor_3j1_setfile_sizes = sensor_3j1_setfile_B_sizes;
			sensor_3j1_pllinfos = sensor_3j1_pllinfos_B;
			sensor_3j1_max_setfile_num = ARRAY_SIZE(sensor_3j1_setfiles_B);
			sensor_3j1_dualsync_slave = sensor_3j1_dual_slave_settings;
			sensor_3j1_dualsync_slave_size = ARRAY_SIZE(sensor_3j1_dual_slave_settings);
			sensor_3j1_dualsync_single = sensor_3j1_dual_single_settings;
			sensor_3j1_dualsync_single_size = ARRAY_SIZE(sensor_3j1_dual_single_settings);
			verify_sensor_mode = sensor_3j1_setfile_B_verify_sensor_mode;
			verify_sensor_mode_size = ARRAY_SIZE(sensor_3j1_setfile_B_verify_sensor_mode);
		} else {
			err("%s setfile index out of bound, take default (setfile_A)", __func__);
			sensor_3j1_global = sensor_3j1_setfile_A_Global;
			sensor_3j1_global_size = ARRAY_SIZE(sensor_3j1_setfile_A_Global);
			sensor_3j1_setfiles = sensor_3j1_setfiles_A;
			sensor_3j1_setfile_sizes = sensor_3j1_setfile_A_sizes;
			sensor_3j1_pllinfos = sensor_3j1_pllinfos_A;
			sensor_3j1_max_setfile_num = ARRAY_SIZE(sensor_3j1_setfiles_A);
			sensor_3j1_dualsync_slave = sensor_3j1_dual_slave_settings;
			sensor_3j1_dualsync_slave_size = ARRAY_SIZE(sensor_3j1_dual_slave_settings);
			verify_sensor_mode = sensor_3j1_setfile_A_verify_sensor_mode;
			verify_sensor_mode_size = ARRAY_SIZE(sensor_3j1_setfile_A_verify_sensor_mode);
		}
	}
#endif

	if (cis->vendor_use_adaptive_mipi) {
		for (i = 0; i < verify_sensor_mode_size; i++) {
			index = verify_sensor_mode[i];

			if (index >= cis->mipi_sensor_mode_size || index < 0) {
				panic("wrong mipi_sensor_mode index");
				break;
			}
		}
	}

	probe_info("%s done\n", __func__);

	return ret;
}

static const struct of_device_id sensor_cis_3j1_match[] = {
	{
		.compatible = "samsung,exynos-is-cis-3j1",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_3j1_match);

static const struct i2c_device_id sensor_cis_3j1_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_3j1_driver = {
	.probe	= cis_3j1_probe_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_3j1_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_3j1_idt
};

#ifdef MODULE
builtin_i2c_driver(sensor_cis_3j1_driver);
#else
static int __init sensor_cis_3j1_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_3j1_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_3j1_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_3j1_init);
#endif

MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: fimc-is");
