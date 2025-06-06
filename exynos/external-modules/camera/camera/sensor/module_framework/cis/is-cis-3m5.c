/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
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
#include "is-cis-3m5.h"
#include "is-cis-3m5-setA.h"
#if 0
#include "is-cis-3m5-setB.h"
#endif
#include "is-cis-3m5-setC.h"

#include "is-helper-ixc.h"
#ifdef CAMERA_REAR2_SENSOR_SHIFT_CROP
#include "is-sec-define.h"
#endif

#define SENSOR_NAME "S5K3M5"
/* #define DEBUG_3M5_PLL */

static const u32 *sensor_3m5_global;
static u32 sensor_3m5_global_size;
static const u32 **sensor_3m5_setfiles;
static const u32 *sensor_3m5_setfile_sizes;
static const struct sensor_pll_info_compact **sensor_3m5_pllinfos;
static u32 sensor_3m5_max_setfile_num;

static const u32 *sensor_3m5_dualsync_slave;
static u32 sensor_3m5_dualsync_slave_size;

#ifdef CAMERA_REAR2_SENSOR_SHIFT_CROP
static struct is_efs_info efs_info;
#endif

static void sensor_3m5_set_integration_max_margin(u32 mode, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!cis_data);

	switch (mode) {
#if 0
		case SENSOR_3M5_4000X3000_30FPS:
		case SENSOR_3M5_4000X2252_30FPS:
		case SENSOR_3M5_2000x1128_30FPS:
		case SENSOR_3M5_2000x1500_30FPS:
		case SENSOR_3M5_1504x1504_30FPS:
		case SENSOR_3M5_1344X756_120FPS:
#endif
		case SENSOR_3M5_2592X1944_30FPS:
			cis_data->max_margin_coarse_integration_time = SENSOR_3M5_COARSE_INTEGRATION_TIME_MAX_MARGIN;
			dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n",
				cis_data->max_margin_coarse_integration_time);
			break;
		default:
			err("[%s] Unsupport 3m5 sensor mode\n", __func__);
			cis_data->max_margin_coarse_integration_time = SENSOR_3M5_COARSE_INTEGRATION_TIME_MAX_MARGIN;
			dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n",
				cis_data->max_margin_coarse_integration_time);
			break;
	}
}

static void sensor_3m5_cis_data_calculation(const struct sensor_pll_info_compact *pll_info, cis_shared_data *cis_data)
{
	u64 vt_pix_clk_hz;
	u32 frame_rate, max_fps, frame_valid_us;

	FIMC_BUG_VOID(!pll_info);

	/* 1. get pclk value from pll info */
	vt_pix_clk_hz = pll_info->pclk;

	/* 2. the time of processing one frame calculation (us) */
	cis_data->min_frame_us_time = (pll_info->frame_length_lines * pll_info->line_length_pck
	                / (vt_pix_clk_hz / (1000 * 1000)));
	cis_data->cur_frame_us_time = cis_data->min_frame_us_time;
	cis_data->base_min_frame_us_time = cis_data->min_frame_us_time;

	/* 3. FPS calculation */
	frame_rate = vt_pix_clk_hz / (pll_info->frame_length_lines * pll_info->line_length_pck);
	dbg_sensor(1, "frame_rate (%d) = vt_pix_clk_hz(%llu) / "
		KERN_CONT "(pll_info->frame_length_lines(%d) * pll_info->line_length_pck(%d))\n",
		frame_rate, vt_pix_clk_hz, pll_info->frame_length_lines, pll_info->line_length_pck);

	/* calculate max fps */
	max_fps = (vt_pix_clk_hz * 10) / (pll_info->frame_length_lines * pll_info->line_length_pck);
	max_fps = (max_fps % 10 >= 5 ? frame_rate + 1 : frame_rate);

	cis_data->pclk = vt_pix_clk_hz;
	cis_data->max_fps = max_fps;
	cis_data->frame_length_lines = pll_info->frame_length_lines;
	cis_data->line_length_pck = pll_info->line_length_pck;
	cis_data->line_readOut_time = (u64)cis_data->line_length_pck * 1000
					* 1000 * 1000 / cis_data->pclk;
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;
	cis_data->stream_on = false;

	/* Frame valid time calcuration */
	frame_valid_us = (u64)cis_data->cur_height * cis_data->line_length_pck
				* 1000 * 1000 / cis_data->pclk;
	cis_data->frame_valid_us_time = (int)frame_valid_us;

	dbg_sensor(1, "%s\n", __func__);
	dbg_sensor(1, "Sensor size(%d x %d) setting: SUCCESS!\n",
	                cis_data->cur_width, cis_data->cur_height);
	dbg_sensor(1, "Frame Valid(us): %d\n", frame_valid_us);
	dbg_sensor(1, "rolling_shutter_skew: %lld\n", cis_data->rolling_shutter_skew);

	dbg_sensor(1, "Fps: %d, max fps(%d)\n", frame_rate, cis_data->max_fps);
	dbg_sensor(1, "min_frame_time(%d us)\n", cis_data->min_frame_us_time);
	dbg_sensor(1, "Pixel rate(Mbps): %llu\n", cis_data->pclk / 1000000);

	/* Frame period calculation */
	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
		cis_data->frame_time, cis_data->rolling_shutter_skew);

	/* Constant values */
	cis_data->min_fine_integration_time = SENSOR_3M5_FINE_INTEGRATION_TIME_MIN;
	cis_data->max_fine_integration_time = SENSOR_3M5_FINE_INTEGRATION_TIME_MAX;
	cis_data->min_coarse_integration_time = SENSOR_3M5_COARSE_INTEGRATION_TIME_MIN;
	cis_data->max_margin_coarse_integration_time= SENSOR_3M5_COARSE_INTEGRATION_TIME_MAX_MARGIN;
}

void sensor_3m5_cis_data_calc(struct v4l2_subdev *subdev, u32 mode)
{
	//int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG_VOID(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG_VOID(!cis);
	FIMC_BUG_VOID(!cis->cis_data);

	if (mode >= sensor_3m5_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return;
	}

	sensor_3m5_cis_data_calculation(sensor_3m5_pllinfos[mode], cis->cis_data);
}

static int sensor_3m5_wait_stream_off_status(cis_shared_data *cis_data)
{
	int ret = 0;
	u32 timeout = 0;

	FIMC_BUG(!cis_data);

#define STREAM_OFF_WAIT_TIME 250
	while (timeout < STREAM_OFF_WAIT_TIME) {
		if (cis_data->is_active_area == false &&
				cis_data->stream_on == false) {
			pr_debug("actual stream off\n");
			break;
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
int sensor_3m5_cis_init(struct v4l2_subdev *subdev)
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
		warn("sensor_3m5_check_rev is fail when cis init");
		ret = -EINVAL;
		goto p_err;
	}
#endif

	cis->cis_data->cur_width = SENSOR_3M5_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_3M5_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;

	sensor_3m5_cis_data_calculation(sensor_3m5_pllinfos[setfile_index], cis->cis_data);
	sensor_3m5_set_integration_max_margin(setfile_index, cis->cis_data);

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
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_3m5_cis_log_status(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	u8 data8 = 0;
	u16 data16 = 0;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	pr_info("[%s] *******************************\n", __func__);
	ret = cis->ixc_ops->read16(cis->client, 0x0000, &data16);
	if (unlikely(!ret))
		pr_info("model_id(0x%x)\n", data16);
	else
		goto i2c_err;
	ret = cis->ixc_ops->read8(cis->client, 0x0002, &data8);
	if (unlikely(!ret))
		pr_info("revision_number(0x%x)\n", data8);
	else
		goto i2c_err;
	ret = cis->ixc_ops->read8(cis->client, 0x0005, &data8);
	if (unlikely(!ret))
		pr_info("frame_count(0x%x)\n", data8);
	else
		goto i2c_err;
	ret = cis->ixc_ops->read8(cis->client, 0x0100, &data8);
	if (unlikely(!ret))
		pr_info("0x0100(0x%x)\n", data8);
	else
		goto i2c_err;
	ret = cis->ixc_ops->read16(cis->client, 0x0136, &data16);
	if (unlikely(!ret))
		pr_info("0x0136(0x%x)\n", data16);
	else
		goto i2c_err;
	ret = cis->ixc_ops->read16(cis->client, 0x0202, &data16);
	if (unlikely(!ret))
		pr_info("0x0202(0x%x)\n", data16);
	else
		goto i2c_err;
	ret = cis->ixc_ops->read16(cis->client, 0x0204, &data16);
	if (unlikely(!ret))
		pr_info("0x0204(0x%x)\n", data16);
	else
		goto i2c_err;
	ret = cis->ixc_ops->read16(cis->client, 0x0340, &data16);
	if (unlikely(!ret))
		pr_info("0x0340(0x%x)\n", data16);
	else
		goto i2c_err;
	ret = cis->ixc_ops->read8(cis->client, 0x3000, &data8);
	if (unlikely(!ret))
		pr_info("0x3000(0x%x)\n", data8);
	else
		goto i2c_err;
	pr_info("[%s] *******************************\n", __func__);

i2c_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
p_err:
	return ret;
}

#if USE_GROUP_PARAM_HOLD
static int sensor_3m5_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
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
static inline int sensor_3m5_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{ return 0; }
#endif

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_3m5_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	ret = sensor_3m5_cis_group_param_hold_func(subdev, hold);
	if (ret < 0)
		goto p_err;

p_err:
	return ret;
}

int sensor_3m5_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	/* setfile global setting is at camera entrance */
	info("[%s] global setting enter\n", __func__);
	ret = sensor_cis_set_registers(subdev, sensor_3m5_global, sensor_3m5_global_size);
	if (ret < 0) {
		err("sensor_3m5_set_registers fail!!");
		goto p_err;
	}
	info("[%s] global setting done\n", __func__);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

#ifdef CAMERA_REAR2_SENSOR_SHIFT_CROP
int sensor_3m5_cis_update_crop_region(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u16 x_start = 0;
	u16 y_start = 0;
	u16 x_end = 0;
	u16 y_end = 0;
	char *cal_buf;
	struct is_cis *cis = NULL;
	struct is_device_sensor *device;
	s16 delta_x, delta_y;
	struct is_rom_info *finfo = NULL;
	u8 *buf = NULL;
	long efs_size = 0;
	int rom_id = 0;
	s16 temp_delta = 0;
	u32 mode;

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (device == NULL) {
		err("device is NULL");
		return -1;
	}

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		return -1;
	}

	mode = device->cfg[device->nfi_toggle]->mode;
	if (mode == SENSOR_3M5_1344X756_120FPS || mode == SENSOR_3M5_2000x1128_30FPS ||
		mode == SENSOR_3M5_2000x1500_30FPS || mode == SENSOR_3M5_1504x1504_30FPS) {
		warn("skip crop shift in fast ae sensor or binning mode");
		return 0;
	}

	rom_id = is_vendor_get_rom_id_from_position(device->position);
	is_sec_get_cal_buf(&cal_buf, rom_id);
	is_sec_get_sysfs_finfo(&finfo, rom_id);

	buf = vmalloc(MAX_EFS_DATA_LENGTH);
	if (!buf) {
		err("memory alloc failed.");
		return 0;
	}

	efs_size = is_vendor_read_efs(CROP_SHIFT_VALUE_FROM_EFS, buf, MAX_EFS_DATA_LENGTH);
	if (efs_size) {
		efs_info.crop_shift_delta_x = *((s16 *)&buf[CROP_SHIFT_ADDR_X]);
		efs_info.crop_shift_delta_y = *((s16 *)&buf[CROP_SHIFT_ADDR_Y]);
		set_bit(IS_EFS_STATE_READ, &efs_info.efs_state);
	} else {
		clear_bit(IS_EFS_STATE_READ, &efs_info.efs_state);
	}

	if (buf)
		vfree(buf);

	if (!test_bit(IS_EFS_STATE_READ, &efs_info.efs_state)) {
		delta_x = *((s16 *)&cal_buf[finfo->rom_dualcal_slave1_cropshift_x_addr]);
		delta_y = *((s16 *)&cal_buf[finfo->rom_dualcal_slave1_cropshift_y_addr]);
		info("[%s] Read from eeprom. delta_x[%d], delta_y[%d]", __func__, delta_x, delta_y);
	} else {
		delta_x = efs_info.crop_shift_delta_x;
		delta_y = efs_info.crop_shift_delta_y;
		info("[%s] Read from efs. delta_x[%d], delta_y[%d]", __func__, delta_x, delta_y);
	}

	if (delta_x > CROP_SHIFT_DELTA_MAX_X)
		delta_x = CROP_SHIFT_DELTA_MAX_X;
	else if (delta_x < CROP_SHIFT_DELTA_MIN_X)
		delta_x = CROP_SHIFT_DELTA_MIN_X;

	if (delta_y > CROP_SHIFT_DELTA_MAX_Y)
		delta_y = CROP_SHIFT_DELTA_MAX_Y;
	else if (delta_y < CROP_SHIFT_DELTA_MIN_Y)
		delta_y = CROP_SHIFT_DELTA_MIN_Y;

	if (delta_x % CROP_SHIFT_DELTA_ALIGN) {
		temp_delta = delta_x / CROP_SHIFT_DELTA_ALIGN;
		delta_x = temp_delta * CROP_SHIFT_DELTA_ALIGN;
		info("[%s] temp_delta = %d, delta_x = %d", __func__, temp_delta, delta_x);
	}

	if (delta_y % CROP_SHIFT_DELTA_ALIGN) {
		temp_delta = delta_y / CROP_SHIFT_DELTA_ALIGN;
		delta_y = temp_delta * CROP_SHIFT_DELTA_ALIGN;
		info("[%s] temp_delta = %d, delta_y = %d", __func__, temp_delta, delta_y);
	}

	info("[%s] Applied delta_x[%d], delta_y[%d]", __func__, delta_x, delta_y);

	ret = cis->ixc_ops->read16(cis->client, 0x0344, &x_start);
	ret = cis->ixc_ops->read16(cis->client, 0x0346, &y_start);
	ret = cis->ixc_ops->read16(cis->client, 0x0348, &x_end);
	ret = cis->ixc_ops->read16(cis->client, 0x034A, &y_end);

	x_start += delta_x;
	y_start += delta_y;
	x_end += delta_x;
	y_end += delta_y;

	ret = cis->ixc_ops->write16(cis->client, 0x0344, x_start);
	ret = cis->ixc_ops->write16(cis->client, 0x0346, y_start);
	ret = cis->ixc_ops->write16(cis->client, 0x0348, x_end);
	ret = cis->ixc_ops->write16(cis->client, 0x034A, y_end);

	info("[%s] x_start(%d), y_start(%d), x_end(%d), y_end(%d)\n",
		__func__, x_start, y_start, x_end, y_end);

	return ret;
}

static int sensor_3m5_cis_update_pdaf_tail_size(struct v4l2_subdev *subdev, struct is_sensor_cfg *select)
{
	u16 crop_num = 0;
	u32 width = 0, height = 0;

	if (select->mode == SENSOR_3M5_1344X756_120FPS) {
		warn("skip crop shift in fast ae sensor mode");
		return 0;
	}

	switch (select->mode) {
	case SENSOR_3M5_4000X3000_30FPS:
		width = 496;
		height = 744;
		break;
	case SENSOR_3M5_4000X2252_30FPS:
	case SENSOR_3M5_2000x1128_30FPS:
		width = 496;
		height = 552;
		break;
	case SENSOR_3M5_2000x1500_30FPS:
		width = 496;
		height = 744;
		break;
	case SENSOR_3M5_1504x1504_30FPS:
		width = 376;
		height = 744;
		break;
	default:
		warn("[%s] Don't change pdaf tail size\n", __func__);
		break;
	}

	select->input[CSI_VIRTUAL_CH_1].width = width;
	select->input[CSI_VIRTUAL_CH_1].height = height;
	select->output[CSI_VIRTUAL_CH_1].width = width;
	select->output[CSI_VIRTUAL_CH_1].height = height;

	info("[%s] PDAF tail size (%d x %d), crop_num = %x\n",
		__func__, width, height, crop_num);

	return 0;
}
#endif

int sensor_3m5_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	struct is_device_sensor *device;
	u32 setfile;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (unlikely(!device)) {
		err("device sensor is null");
		return -EINVAL;
	}

	if (mode >= sensor_3m5_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	/* always slave mode */
	info("[%s]dual sync slave mode\n", __func__);
	ret = sensor_cis_set_registers(subdev, sensor_3m5_dualsync_slave, sensor_3m5_dualsync_slave_size);
	if (ret < 0) {
		err("sensor_3m5_set_registers fail!!");
		goto p_err;
	}

	ret = sensor_cis_set_registers(subdev, sensor_3m5_setfiles[mode], sensor_3m5_setfile_sizes[mode]);
	if (ret < 0) {
		err("sensor_3m5_set_registers fail!!");
		goto p_err;
	}

#ifdef CAMERA_REAR2_SENSOR_SHIFT_CROP
	if (device->position == SENSOR_POSITION_REAR2) {
		ret = sensor_3m5_cis_update_crop_region(subdev);
		if (ret < 0) {
			err("sensor_3m5_cis_update_crop_region fail!!");
			goto p_err;
		}
	}
#endif

	if (device->ischain) {
	setfile = device->ischain->setfile;
	} else {
		err("device->ischain is null");
		ret = -EINVAL;
		goto p_err;
	}

	info("[3M5] current scenario: 0x%x (rev:0x%x), mode(%d)", setfile, cis->cis_data->cis_rev, mode);

	dbg_sensor(1, "[%s] mode changed(%d)\n", __func__, mode);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

/* TODO: Sensor set size sequence(sensor done, sensor stop, 3AA done in FW case */
int sensor_3m5_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
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
	ret = sensor_3m5_wait_stream_off_status(cis_data);
	if (ret) {
		err("Must stream off\n");
		ret = -EINVAL;
		goto p_err;
	}

	binning = cis_data->binning;
	if (binning) {
		ratio_w = (SENSOR_3M5_MAX_WIDTH / cis_data->cur_width);
		ratio_h = (SENSOR_3M5_MAX_HEIGHT / cis_data->cur_height);
	} else {
		ratio_w = 1;
		ratio_h = 1;
	}

	if (((cis_data->cur_width * ratio_w) > SENSOR_3M5_MAX_WIDTH) ||
		((cis_data->cur_height * ratio_h) > SENSOR_3M5_MAX_HEIGHT)) {
		err("Config max sensor size over~!!\n");
		ret = -EINVAL;
		goto p_err;
	}

	/* 1. page_select */
	ret = cis->ixc_ops->write16(cis->client, 0x6028, 0x2000);
	if (ret < 0)
		 goto p_err;

	/* 2. pixel address region setting */
	start_x = ((SENSOR_3M5_MAX_WIDTH - cis_data->cur_width * ratio_w) / 2) & (~0x1);
	start_y = ((SENSOR_3M5_MAX_HEIGHT - cis_data->cur_height * ratio_h) / 2) & (~0x1);
	end_x = start_x + (cis_data->cur_width * ratio_w - 1);
	end_y = start_y + (cis_data->cur_height * ratio_h - 1);

	if (!(end_x & (0x1)) || !(end_y & (0x1))) {
		err("Sensor pixel end address must odd\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = cis->ixc_ops->write16(cis->client, 0x0344, start_x);
	if (ret < 0)
		 goto p_err;
	ret = cis->ixc_ops->write16(cis->client, 0x0346, start_y);
	if (ret < 0)
		 goto p_err;
	ret = cis->ixc_ops->write16(cis->client, 0x0348, end_x);
	if (ret < 0)
		 goto p_err;
	ret = cis->ixc_ops->write16(cis->client, 0x034A, end_y);
	if (ret < 0)
		 goto p_err;

	/* 3. output address setting */
	ret = cis->ixc_ops->write16(cis->client, 0x034C, cis_data->cur_width);
	if (ret < 0)
		 goto p_err;
	ret = cis->ixc_ops->write16(cis->client, 0x034E, cis_data->cur_height);
	if (ret < 0)
		 goto p_err;

	/* If not use to binning, sensor image should set only crop */
	if (!binning) {
		dbg_sensor(1, "Sensor size set is not binning\n");
		goto p_err;
	}

	/* 4. sub sampling setting */
	even_x = 1;	/* 1: not use to even sampling */
	even_y = 1;
	odd_x = (ratio_w * 2) - even_x;
	odd_y = (ratio_h * 2) - even_y;

	ret = cis->ixc_ops->write16(cis->client, 0x0380, even_x);
	if (ret < 0)
		 goto p_err;
	ret = cis->ixc_ops->write16(cis->client, 0x0382, odd_x);
	if (ret < 0)
		 goto p_err;
	ret = cis->ixc_ops->write16(cis->client, 0x0384, even_y);
	if (ret < 0)
		 goto p_err;
	ret = cis->ixc_ops->write16(cis->client, 0x0386, odd_y);
	if (ret < 0)
		 goto p_err;

	/* 5. binnig setting */
	ret = cis->ixc_ops->write8(cis->client, 0x0900, binning);	/* 1:  binning enable, 0: disable */
	if (ret < 0)
		goto p_err;
	ret = cis->ixc_ops->write8(cis->client, 0x0901, (ratio_w << 4) | ratio_h);
	if (ret < 0)
		goto p_err;

	/* 6. scaling setting: but not use */
	/* scaling_digital_scaling */
	ret = cis->ixc_ops->write16(cis->client, 0x0402, 0x1010);
	if (ret < 0)
		goto p_err;
	/* scaling_hbin_digital_binning_factor */
	ret = cis->ixc_ops->write16(cis->client, 0x0404, 0x0010);
	if (ret < 0)
		goto p_err;
	/* scaling_tetracell_digital_binning_factor */
	ret = cis->ixc_ops->write16(cis->client, 0x0400, 0x1010);
	if (ret < 0)
		goto p_err;

	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis->cis_data->rolling_shutter_skew = (cis->cis_data->cur_height - 1) * cis->cis_data->line_readOut_time;
	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
		cis->cis_data->frame_time, cis->cis_data->rolling_shutter_skew);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_3m5_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	struct is_device_sensor *device;
	u32 mode;
	ktime_t st = ktime_get();

	WARN_ON(!subdev);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

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

	is_vendor_set_mipi_clock(device);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_3m5_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream on");

#ifdef DEBUG_3m5_PLL
	{
	u16 pll;
	cis->ixc_ops->read16(cis->client, 0x0300, &pll);
	dbg_sensor(1, "______ vt_pix_clk_div(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0302, &pll);
	dbg_sensor(1, "______ vt_sys_clk_div(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0304, &pll);
	dbg_sensor(1, "______ pre_pll_clk_div(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0306, &pll);
	dbg_sensor(1, "______ pll_multiplier(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0308, &pll);
	dbg_sensor(1, "______ op_pix_clk_div(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x030a, &pll);
	dbg_sensor(1, "______ op_sys_clk_div(%x)\n", pll);

	cis->ixc_ops->read16(cis->client, 0x030c, &pll);
	dbg_sensor(1, "______ secnd_pre_pll_clk_div(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x030e, &pll);
	dbg_sensor(1, "______ secnd_pll_multiplier(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0340, &pll);
	dbg_sensor(1, "______ frame_length_lines(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0342, &pll);
	dbg_sensor(1, "______ line_length_pck(%x)\n", pll);
	}
#endif

	/* Sensor stream on */
#ifdef CAMERA_USE_TELE_VFLIP
	cis->ixc_ops->write16(cis->client, 0x0100, 0x0102);
#else
	cis->ixc_ops->write16(cis->client, 0x0100, 0x0100);
#endif

	info("%s\n", __func__);

	/* TODO: WDR */

	cis_data->stream_on = true;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	mode = cis_data->sens_config_index_cur;
	dbg_sensor(1, "[%s] sens_config_index_cur=%d\n", __func__, mode);

	switch (mode) {
	case SENSOR_3M5_4000X3000_30FPS:
	case SENSOR_3M5_4000X2252_30FPS:
	case SENSOR_3M5_2000x1128_30FPS:
	case SENSOR_3M5_2000x1500_30FPS:
	case SENSOR_3M5_1504x1504_30FPS:
	case SENSOR_3M5_2592X1944_30FPS:
		cis->cis_data->base_min_frame_us_time = cis->cis_data->min_frame_us_time = 33333;
		break;
	default:
		break;
	}

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_stream_off(struct v4l2_subdev *subdev)
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

	ret = sensor_3m5_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream off");

	/* Sensor stream off */
	cis->ixc_ops->write8(cis->client, 0x0100, 0x00);

	info("%s\n", __func__);

	cis_data->stream_on = false;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 vt_pic_clk_freq_mhz = 0;
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
		err("[%s] invalid target exposure(%d, %d)\n", __func__,
				target_exposure->long_val, target_exposure->short_val);
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), target long(%d), short(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, target_exposure->long_val, target_exposure->short_val);

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000 * 1000);
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;

	long_coarse_int = ((target_exposure->long_val * vt_pic_clk_freq_mhz) - min_fine_int) / line_length_pck;
	short_coarse_int = ((target_exposure->short_val * vt_pic_clk_freq_mhz) - min_fine_int) / line_length_pck;

	if (long_coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) max(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, long_coarse_int, cis_data->max_coarse_integration_time);
		long_coarse_int = cis_data->max_coarse_integration_time;
	}

	if (short_coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) max(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, short_coarse_int, cis_data->max_coarse_integration_time);
		short_coarse_int = cis_data->max_coarse_integration_time;
	}

	if (long_coarse_int < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, long_coarse_int, cis_data->min_coarse_integration_time);
		long_coarse_int = cis_data->min_coarse_integration_time;
	}

	if (short_coarse_int < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) min(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, short_coarse_int, cis_data->min_coarse_integration_time);
		short_coarse_int = cis_data->min_coarse_integration_time;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/* WDR mode */
	if (is_vendor_wdr_mode_on(cis_data)) {
		cis->ixc_ops->write16(cis->client, 0x021E, 0x0100);
	} else {
		cis->ixc_ops->write16(cis->client, 0x021E, 0x0000);
	}

	/* Short exposure */
	ret = cis->ixc_ops->write16(cis->client, 0x0202, short_coarse_int);
	if (ret < 0)
		goto p_err;

	/* Long exposure */
	if (is_vendor_wdr_mode_on(cis_data)) {
		ret = cis->ixc_ops->write16(cis->client, 0x0226, long_coarse_int);
		if (ret < 0)
			goto p_err;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), vt_pic_clk_freq_mhz (%d),"
		KERN_CONT "line_length_pck(%d), min_fine_int (%d)\n", cis->id, __func__,
		cis_data->sen_vsync_count, vt_pic_clk_freq_mhz, line_length_pck, min_fine_int);
	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), frame_length_lines(%#x),"
		KERN_CONT "long_coarse_int %#x, short_coarse_int %#x\n", cis->id, __func__,
		cis_data->sen_vsync_count, cis_data->frame_length_lines, long_coarse_int, short_coarse_int);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	cis_shared_data *cis_data = NULL;
	u32 min_integration_time = 0;
	u32 min_coarse = 0;
	u32 min_fine = 0;
	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000 * 1000);
	if (vt_pic_clk_freq_mhz == 0) {
		pr_err("[MOD:D:%d] %s, Invalid vt_pic_clk_freq_mhz(%d)\n", cis->id, __func__, vt_pic_clk_freq_mhz);
		goto p_err;
	}
	line_length_pck = cis_data->line_length_pck;
	min_coarse = cis_data->min_coarse_integration_time;
	min_fine = cis_data->min_fine_integration_time;

	min_integration_time = ((line_length_pck * min_coarse) + min_fine) / vt_pic_clk_freq_mhz;
	*min_expo = min_integration_time;

	dbg_sensor(1, "[%s] min integration time %d\n", __func__, min_integration_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_3m5_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 max_integration_time = 0;
	u32 max_coarse_margin = 0;
	u32 max_fine_margin = 0;
	u32 max_coarse = 0;
	u32 max_fine = 0;
	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000 * 1000);
	if (vt_pic_clk_freq_mhz == 0) {
		pr_err("[MOD:D:%d] %s, Invalid vt_pic_clk_freq_mhz(%d)\n", cis->id, __func__, vt_pic_clk_freq_mhz);
		goto p_err;
	}
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = cis_data->frame_length_lines;

	max_coarse_margin = cis_data->max_margin_coarse_integration_time;
	max_fine_margin = line_length_pck - cis_data->min_fine_integration_time;
	max_coarse = frame_length_lines - max_coarse_margin;
	max_fine = cis_data->max_fine_integration_time;

	max_integration_time = ((line_length_pck * max_coarse) + max_fine) / vt_pic_clk_freq_mhz;

	*max_expo = max_integration_time;

	/* TODO: Is this values update hear? */
	cis_data->max_margin_fine_integration_time = max_fine_margin;
	cis_data->max_coarse_integration_time = max_coarse;

	dbg_sensor(1, "[%s] max integration time %d, max margin fine integration %d, max coarse integration %d\n",
			__func__, max_integration_time, cis_data->max_margin_fine_integration_time, cis_data->max_coarse_integration_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:

	return ret;
}

int sensor_3m5_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
						u32 input_exposure_time,
						u32 *target_duration)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 vt_pic_clk_freq_mhz = 0;
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

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000 * 1000);
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = (u32)((((u64)(vt_pic_clk_freq_mhz) * input_exposure_time)
						- cis_data->min_fine_integration_time) / line_length_pck);
	frame_length_lines += cis_data->max_margin_coarse_integration_time;

	frame_duration = (frame_length_lines * line_length_pck) / vt_pic_clk_freq_mhz;

	dbg_sensor(1, "[%s](vsync cnt = %d) input exp(%d), adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, cis_data->sen_vsync_count, input_exposure_time, frame_duration, cis_data->min_frame_us_time);
	dbg_sensor(1, "[%s](vsync cnt = %d) adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, cis_data->sen_vsync_count, frame_duration, cis_data->min_frame_us_time);

	*target_duration = MAX(frame_duration, cis_data->min_frame_us_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3m5_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 vt_pic_clk_freq_mhz = 0;
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

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000 * 1000);
	line_length_pck = cis_data->line_length_pck;

	frame_length_lines = (u16)((vt_pic_clk_freq_mhz * frame_duration) / line_length_pck);

	dbg_sensor(1, "[MOD:D:%d] %s, vt_pic_clk_freq_mhz(%#x) frame_duration = %d us,"
		KERN_CONT "(line_length_pck%#x), frame_length_lines(%#x)\n",
		cis->id, __func__, vt_pic_clk_freq_mhz, frame_duration, line_length_pck, frame_length_lines);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = cis->ixc_ops->write16(cis->client, 0x0340, frame_length_lines);
	if (ret < 0)
		goto p_err;

	cis_data->cur_frame_us_time = frame_duration;
	cis_data->frame_length_lines = frame_length_lines;
	cis_data->max_coarse_integration_time = cis_data->frame_length_lines - cis_data->max_margin_coarse_integration_time;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
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
		err("[MOD:D:%d] %s, request FPS is too high(%d), set to max(%d)\n",
			cis->id, __func__, min_fps, cis_data->max_fps);
		min_fps = cis_data->max_fps;
	}

	if (min_fps == 0) {
		err("[MOD:D:%d] %s, request FPS is 0, set to min FPS(1)\n",
			cis->id, __func__);
		min_fps = 1;
	}

	frame_duration = (1 * 1000 * 1000) / min_fps;

	dbg_sensor(1, "[MOD:D:%d] %s, set FPS(%d), frame duration(%d)\n",
			cis->id, __func__, min_fps, frame_duration);

	ret = sensor_3m5_cis_set_frame_duration(subdev, frame_duration);
	if (ret < 0) {
		err("[MOD:D:%d] %s, set frame duration is fail(%d)\n",
			cis->id, __func__, ret);
		goto p_err;
	}

	cis_data->min_frame_us_time = MAX(frame_duration, cis_data->base_min_frame_us_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:

	return ret;
}

int sensor_3m5_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
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
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3m5_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
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

	analog_gain = (u16)sensor_cis_calc_again_code(again->val);

	if (analog_gain < cis->cis_data->min_analog_gain[0]) {
		info("[%s] not proper analog_gain value, reset to min_analog_gain\n", __func__);
		analog_gain = cis->cis_data->min_analog_gain[0];
	}

	if (analog_gain > cis->cis_data->max_analog_gain[0]) {
		info("[%s] not proper analog_gain value, reset to max_analog_gain\n", __func__);
		analog_gain = cis->cis_data->max_analog_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_again = %d us, analog_gain(%#x)\n",
		cis->id, __func__, cis->cis_data->sen_vsync_count, again->val, analog_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = cis->ixc_ops->write16(cis->client, 0x0204, analog_gain);
	if (ret < 0)
		goto p_err;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
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

	hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x0204, &analog_gain);
	if (ret < 0)
		goto p_err;

	*again = sensor_cis_calc_again_permile(analog_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_again = %d us, analog_gain(%#x)\n",
			cis->id, __func__, *again, analog_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	cis_data->min_analog_gain[0] = 0x20; /* x1, gain=x/0x20 */
	cis_data->min_analog_gain[1] = sensor_cis_calc_again_permile(cis_data->min_analog_gain[0]);

	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_analog_gain[0], cis_data->min_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3m5_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	cis_data->max_analog_gain[0] = 0x200; /* x16, gain=x/0x20 */
	cis_data->max_analog_gain[1] = sensor_cis_calc_again_permile(cis_data->max_analog_gain[0]);

	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_analog_gain[0], cis_data->max_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3m5_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
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
		info("[%s] not proper long_gain value, reset to min_digital_gain\n", __func__);
		long_gain = cis->cis_data->min_digital_gain[0];
	}
	if (long_gain > cis->cis_data->max_digital_gain[0]) {
		info("[%s] not proper long_gain value, reset to max_digital_gain\n", __func__);
		long_gain = cis->cis_data->max_digital_gain[0];
	}

	if (short_gain < cis->cis_data->min_digital_gain[0]) {
		info("[%s] not proper short_gain value, reset to min_digital_gain\n", __func__);
		short_gain = cis->cis_data->min_digital_gain[0];
	}
	if (short_gain > cis->cis_data->max_digital_gain[0]) {
		info("[%s] not proper short_gain value, reset to max_digital_gain\n", __func__);
		short_gain = cis->cis_data->max_digital_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_dgain = %d/%d us, long_gain(%#x), short_gain(%#x)\n",
			cis->id, __func__, cis->cis_data->sen_vsync_count, dgain->long_val, dgain->short_val, long_gain, short_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/* DGain WDR Mode */
	if (is_vendor_wdr_mode_on(cis_data)) {
		cis->ixc_ops->write16(cis->client, 0x020D, 0x0000);
	} else {
		cis->ixc_ops->write16(cis->client, 0x020D, 0x0002);
	}

	/* Short digital gain */
	ret = cis->ixc_ops->write16(cis->client, 0x020E, short_gain);
	if (ret < 0)
		goto p_err;

	/* Long digital gain */
	if (is_vendor_wdr_mode_on(cis_data)) {
		ret = cis->ixc_ops->write16(cis->client, 0x0230, long_gain);
		if (ret < 0)
			goto p_err;
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
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

	hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x020E, &digital_gain);
	if (ret < 0)
		goto p_err;

	*dgain = sensor_cis_calc_dgain_permile(digital_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_dgain = %d us, digital_gain(%#x)\n",
			cis->id, __func__, *dgain, digital_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_3m5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	/* 3m5 cannot read min/max digital gain */
	cis_data->min_digital_gain[0] = 0x0100;
	cis_data->min_digital_gain[1] = 1000;

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_digital_gain[0], cis_data->min_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3m5_cis_set_adjust_sync(struct v4l2_subdev *subdev, u32 sync)
{
	int ret = 0;
	struct is_cis *cis;

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* Adjust Sync */
	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = cis->ixc_ops->write8(cis->client, 0x0B32, 0x01); // same with dual sync enable setting

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_3m5_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	/* 3m5 cannot read min/max digital gain */
	cis_data->max_digital_gain[0] = 0x1000;
	cis_data->max_digital_gain[1] = 16000;

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_digital_gain[0], cis_data->max_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_3m5_cis_recover_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	info("%s start\n", __func__);

	ret = sensor_3m5_cis_set_global_setting(subdev);
	if (ret < 0) goto p_err;
	ret = sensor_3m5_cis_mode_change(subdev, cis->cis_data->sens_config_index_cur);
	if (ret < 0) goto p_err;
	ret = sensor_3m5_cis_stream_on(subdev);
	if (ret < 0) goto p_err;
	ret = sensor_cis_wait_streamon(subdev);
	if (ret < 0) goto p_err;

	info("%s end\n", __func__);
p_err:
	return ret;
}

int sensor_3m5_cis_recover_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	info("%s start\n", __func__);

	ret = sensor_3m5_cis_stream_off(subdev);
	if (ret < 0) goto p_err;
	ret = sensor_cis_wait_streamoff(subdev);
	if (ret < 0) goto p_err;

	info("%s end\n", __func__);
p_err:
	return ret;
}

static struct is_cis_ops cis_ops = {
	.cis_init = sensor_3m5_cis_init,
	.cis_log_status = sensor_3m5_cis_log_status,
	.cis_group_param_hold = sensor_3m5_cis_group_param_hold,
	.cis_set_global_setting = sensor_3m5_cis_set_global_setting,
	.cis_mode_change = sensor_3m5_cis_mode_change,
	.cis_set_size = sensor_3m5_cis_set_size,
	.cis_stream_on = sensor_3m5_cis_stream_on,
	.cis_stream_off = sensor_3m5_cis_stream_off,
	.cis_set_exposure_time = sensor_3m5_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_3m5_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_3m5_cis_get_max_exposure_time,
	.cis_adjust_frame_duration = sensor_3m5_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_3m5_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_3m5_cis_set_frame_rate,
	.cis_adjust_analog_gain = sensor_3m5_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_3m5_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_3m5_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_3m5_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_3m5_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_3m5_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_3m5_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_3m5_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_3m5_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_cis_compensate_gain_for_extremely_br,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_wait_streamon = sensor_cis_wait_streamon,
	.cis_data_calculation = sensor_3m5_cis_data_calc,
	.cis_set_adjust_sync = sensor_3m5_cis_set_adjust_sync,
#ifdef CAMERA_REAR2_SENSOR_SHIFT_CROP
	.cis_update_pdaf_tail_size = sensor_3m5_cis_update_pdaf_tail_size,
#endif
	.cis_check_rev_on_init = sensor_cis_check_rev_on_init,
	.cis_set_initial_exposure = sensor_cis_set_initial_exposure,
	.cis_recover_stream_on = sensor_3m5_cis_recover_stream_on,
	.cis_recover_stream_off = sensor_3m5_cis_recover_stream_off,
};

static int cis_3m5_probe_i2c(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	struct is_cis *cis;
	struct is_device_sensor_peri *sensor_peri;
	char const *setfile;
	struct device_node *dnode = client->dev.of_node;
	int i, index;
	const int *verify_sensor_mode = NULL;
	int verify_sensor_mode_size = 0;

	ret = sensor_cis_probe(client, &(client->dev), &sensor_peri, I2C_TYPE);
	if (ret) {
		probe_info("%s: sensor_cis_probe ret(%d)\n", __func__, ret);
		return ret;
	}

	cis = &sensor_peri->cis;
	cis->ctrl_delay = N_PLUS_TWO_FRAME;
	cis->cis_ops = &cis_ops;
	/* belows are depend on sensor cis. MUST check sensor spec */
#ifdef CAMERA_USE_TELE_VFLIP
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_BG_GR;
#else
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;
#endif

	if (of_property_read_string(dnode, "setfile", &setfile)) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0) {
		probe_info("%s setfile_A\n", __func__);
		sensor_3m5_global = sensor_3m5_setfile_A_Global;
		sensor_3m5_global_size = ARRAY_SIZE(sensor_3m5_setfile_A_Global);
		sensor_3m5_setfiles = sensor_3m5_setfiles_A;
		sensor_3m5_setfile_sizes = sensor_3m5_setfile_A_sizes;
		sensor_3m5_pllinfos = sensor_3m5_pllinfos_A;
		sensor_3m5_max_setfile_num = ARRAY_SIZE(sensor_3m5_setfiles_A);
		sensor_3m5_dualsync_slave = sensor_3m5_dual_slave_A_settings;
		sensor_3m5_dualsync_slave_size = ARRAY_SIZE(sensor_3m5_dual_slave_A_settings);
		verify_sensor_mode = sensor_3m5_setfile_A_verify_sensor_mode;
		verify_sensor_mode_size = ARRAY_SIZE(sensor_3m5_setfile_A_verify_sensor_mode);
	}	else if (strcmp(setfile, "setC") == 0) {
		probe_info("%s setfile_C\n", __func__);
		sensor_3m5_global = sensor_3m5_setfile_C_Global;
		sensor_3m5_global_size = ARRAY_SIZE(sensor_3m5_setfile_C_Global);
		sensor_3m5_setfiles = sensor_3m5_setfiles_C;
		sensor_3m5_setfile_sizes = sensor_3m5_setfile_C_sizes;
		sensor_3m5_pllinfos = sensor_3m5_pllinfos_C;
		sensor_3m5_max_setfile_num = ARRAY_SIZE(sensor_3m5_setfiles_C);
		sensor_3m5_dualsync_slave = sensor_3m5_setfile_C_dualsync_Slave;
		sensor_3m5_dualsync_slave_size = ARRAY_SIZE(sensor_3m5_setfile_C_dualsync_Slave);
		verify_sensor_mode = sensor_3m5_setfile_A_verify_sensor_mode;
		verify_sensor_mode_size = ARRAY_SIZE(sensor_3m5_setfile_A_verify_sensor_mode);
	} else {
		err("%s setfile index out of bound, take default (setfile_A)", __func__);
		sensor_3m5_global = sensor_3m5_setfile_A_Global;
		sensor_3m5_global_size = ARRAY_SIZE(sensor_3m5_setfile_A_Global);
		sensor_3m5_setfiles = sensor_3m5_setfiles_A;
		sensor_3m5_setfile_sizes = sensor_3m5_setfile_A_sizes;
		sensor_3m5_pllinfos = sensor_3m5_pllinfos_A;
		sensor_3m5_max_setfile_num = ARRAY_SIZE(sensor_3m5_setfiles_A);
		sensor_3m5_dualsync_slave = sensor_3m5_dual_slave_A_settings;
		sensor_3m5_dualsync_slave_size = ARRAY_SIZE(sensor_3m5_dual_slave_A_settings);
		verify_sensor_mode = sensor_3m5_setfile_A_verify_sensor_mode;
		verify_sensor_mode_size = ARRAY_SIZE(sensor_3m5_setfile_A_verify_sensor_mode);
	}

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

static const struct of_device_id sensor_cis_3m5_match[] = {
	{
		.compatible = "samsung,exynos-is-cis-3m5",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_3m5_match);

static const struct i2c_device_id sensor_cis_3m5_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_3m5_driver = {
	.probe	= cis_3m5_probe_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_3m5_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_3m5_idt
};

#ifdef MODULE
builtin_i2c_driver(sensor_cis_3m5_driver);
#else
static int __init sensor_cis_3m5_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_3m5_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_3m5_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_3m5_init);
#endif

MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: fimc-is");
