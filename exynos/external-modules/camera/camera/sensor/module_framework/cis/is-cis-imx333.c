/*
 * Samsung Exynos5 SoC series Sensor driver
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
#include "is-cis-imx333.h"
#include "is-cis-imx333-setA.h"
#include "is-cis-imx333-setB.h"

#include "is-helper-ixc.h"

#define SENSOR_NAME "IMX333"
/* #define DEBUG_IMX333_PLL */

static const u32 *sensor_imx333_global;
static u32 sensor_imx333_global_size;
static const u32 **sensor_imx333_setfiles;
static const u32 *sensor_imx333_setfile_sizes;
static const struct sensor_pll_info_compact **sensor_imx333_pllinfos;
static u32 sensor_imx333_max_setfile_num;

static void sensor_imx333_set_integration_max_margin(u32 mode, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!cis_data);

	switch (mode) {
		case SENSOR_IMX333_8064X3024_30FPS:
		case SENSOR_IMX333_8064X2268_30FPS:
		case SENSOR_IMX333_6048X3024_30FPS:
		case SENSOR_IMX333_4032X2268_60FPS:
		case SENSOR_IMX333_8064X1960_30FPS:
			cis_data->max_margin_coarse_integration_time = 0x14;
			dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n",
				cis_data->max_margin_coarse_integration_time);
			break;
		case SENSOR_IMX333_2016X1134_120FPS:
		case SENSOR_IMX333_2016X1134_30FPS:
			cis_data->max_margin_coarse_integration_time = 0x0E;
                        break;
		case SENSOR_IMX333_2016X1134_240FPS:
		case SENSOR_IMX333_1008X756_120FPS:
		case SENSOR_IMX333_2016X1512_30FPS:
		case SENSOR_IMX333_1504X1504_30FPS:
			cis_data->max_margin_coarse_integration_time = 0x0A;
			dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n",
				cis_data->max_margin_coarse_integration_time);
			break;
		default:
			err("[%s] Unsupport imx333 sensor mode\n", __func__);
			cis_data->max_margin_coarse_integration_time = SENSOR_IMX333_COARSE_INTEGRATION_TIME_MAX_MARGIN;
			dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n",
				cis_data->max_margin_coarse_integration_time);
			break;
	}
}

static void sensor_imx333_set_integration_min(u32 mode, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!cis_data);

	switch (mode) {
		case SENSOR_IMX333_8064X3024_30FPS:
		case SENSOR_IMX333_8064X2268_30FPS:
		case SENSOR_IMX333_6048X3024_30FPS:
		case SENSOR_IMX333_8064X1960_30FPS:
		        cis_data->min_coarse_integration_time = 2;
			dbg_sensor(1, "min_coarse_integration_time(%d)\n",
				cis_data->min_coarse_integration_time);
			break;
		case SENSOR_IMX333_4032X2268_60FPS:
			cis_data->min_coarse_integration_time = 4;
			break;
		case SENSOR_IMX333_2016X1134_120FPS:
		case SENSOR_IMX333_2016X1134_30FPS:
		        cis_data->min_coarse_integration_time = 2;
                        break;
		case SENSOR_IMX333_2016X1134_240FPS:
		case SENSOR_IMX333_1008X756_120FPS:
		case SENSOR_IMX333_2016X1512_30FPS:
		case SENSOR_IMX333_1504X1504_30FPS:
		        cis_data->min_coarse_integration_time = 4;
			dbg_sensor(1, "min_coarse_integration_time(%d)\n",
				cis_data->min_coarse_integration_time);
			break;
		default:
			err("[%s] Unsupport imx333 sensor mode\n", __func__);
		        cis_data->min_coarse_integration_time = 2;
			dbg_sensor(1, "min_coarse_integration_time(%d)\n",
				cis_data->min_coarse_integration_time);
			break;
	}
}

static void sensor_imx333_set_integration_time_step(u32 mode, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!cis_data);

	switch (mode) {
		case SENSOR_IMX333_8064X3024_30FPS:
		case SENSOR_IMX333_8064X2268_30FPS:
		case SENSOR_IMX333_6048X3024_30FPS:
		case SENSOR_IMX333_8064X1960_30FPS:
		        cis_data->cur_coarse_integration_time_step = SENSOR_IMX333_COARSE_STEP_2N;
			dbg_sensor(1, "cur_coarse_integration_time_step(%d)\n",
				cis_data->cur_coarse_integration_time_step);
			break;
		case SENSOR_IMX333_4032X2268_60FPS:
			cis_data->cur_coarse_integration_time_step = SENSOR_IMX333_COARSE_STEP_4N;
			break;
		case SENSOR_IMX333_2016X1134_120FPS:
		case SENSOR_IMX333_2016X1134_30FPS:
		        cis_data->cur_coarse_integration_time_step = SENSOR_IMX333_COARSE_STEP_2N;
			dbg_sensor(1, "cur_coarse_integration_time_step(%d)\n",
				cis_data->cur_coarse_integration_time_step);
                        break;
		case SENSOR_IMX333_2016X1134_240FPS:
		case SENSOR_IMX333_1008X756_120FPS:
		case SENSOR_IMX333_2016X1512_30FPS:
		case SENSOR_IMX333_1504X1504_30FPS:
		        cis_data->cur_coarse_integration_time_step = SENSOR_IMX333_COARSE_STEP_4N;
			dbg_sensor(1, "cur_coarse_integration_time_step(%d)\n",
				cis_data->cur_coarse_integration_time_step);
			break;
		default:
			err("[%s] Unsupport imx333 sensor mode\n", __func__);
		        cis_data->cur_coarse_integration_time_step = SENSOR_IMX333_COARSE_STEP_2N;
			dbg_sensor(1, "cur_coarse_integration_time_step(%d)\n",
				cis_data->cur_coarse_integration_time_step);
			break;
	}
}

static void sensor_imx333_cis_data_calculation(const struct sensor_pll_info_compact *pll_info, cis_shared_data *cis_data)
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
	dbg_sensor(1, "Pixel rate(Mbps): %d\n", cis_data->pclk / 1000000);

        /* Frame period calculation */
        cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
        cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
		cis_data->frame_time, cis_data->rolling_shutter_skew);

        /* Constant values */
        cis_data->min_fine_integration_time = SENSOR_IMX333_FINE_INTEGRATION_TIME_MIN;
        cis_data->max_fine_integration_time = SENSOR_IMX333_FINE_INTEGRATION_TIME_MAX;
        info("%s: done", __func__);
}

void sensor_imx333_cis_data_calc(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG_VOID(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG_VOID(!cis);
	FIMC_BUG_VOID(!cis->cis_data);

	if (mode > sensor_imx333_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx333_check_rev is fail: ret(%d)", ret);
			return;
		}
	}

	sensor_imx333_cis_data_calculation(sensor_imx333_pllinfos[mode], cis->cis_data);
}

static int sensor_imx333_wait_stream_off_status(cis_shared_data *cis_data)
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

int sensor_imx333_cis_check_rev(struct is_cis *cis)
{
	int ret = 0;
	u8 rev = 0, status=0;

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	/* Specify OPT Page Address for READ */
	cis->ixc_ops->write8(cis->client, 0x0A02,  0x7F);

	/* Turn ON OTP Read MODE */
	cis->ixc_ops->write8(cis->client, 0x0A00,  0x01);

	cis->ixc_ops->read8(cis->client, 0x0A01,  &status);
	if ((status & 0x1) == false)
		err("status fail, (%d)", status);

	ret = cis->ixc_ops->read8(cis->client, 0x0A21, &rev);
	if (ret < 0) {
		err("is_sensor_read8 fail (ret %d)", ret);
		IXC_MUTEX_UNLOCK(cis->ixc_lock);
		return ret;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis->cis_data->cis_rev = rev;
	probe_info("imx333 rev:%x", rev);

	return 0;
}

/* CIS OPS */
int sensor_imx333_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	u32 setfile_index = 0;
	cis_setting_info setinfo;
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct is_device_sensor_peri *sensor_peri = NULL;
#endif
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
	memset(cis->cis_data, 0, sizeof(cis_shared_data));
	cis->rev_flag = false;

	ret = sensor_imx333_cis_check_rev(cis);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
		if (sensor_peri)
			is_sec_get_hw_param(&hw_param, sensor_peri->module->position);
		if (hw_param)
			hw_param->i2c_sensor_err_cnt++;
#endif
		warn("sensor_imx333_check_rev is fail when cis init, ret(%d)", ret);
		cis->rev_flag = true;
		goto p_err;
	}

	if (cis->cis_data->cis_rev < 0x12) {
		probe_info("%s setfile_A\n", __func__);
		sensor_imx333_global = sensor_imx333_setfile_A_Global;
		sensor_imx333_global_size = sizeof(sensor_imx333_setfile_A_Global) / sizeof(sensor_imx333_setfile_A_Global[0]);
		sensor_imx333_setfiles = sensor_imx333_setfiles_A;
		sensor_imx333_setfile_sizes = sensor_imx333_setfile_A_sizes;
		sensor_imx333_pllinfos = sensor_imx333_pllinfos_A;
		sensor_imx333_max_setfile_num = sizeof(sensor_imx333_setfiles_A) / sizeof(sensor_imx333_setfiles_A[0]);
	} else {
		probe_info("%s setfile_B (retention)\n", __func__);
		sensor_imx333_global = sensor_imx333_setfile_B_Global;
		sensor_imx333_global_size = sizeof(sensor_imx333_setfile_B_Global) / sizeof(sensor_imx333_setfile_B_Global[0]);
		sensor_imx333_setfiles = sensor_imx333_setfiles_B;
		sensor_imx333_setfile_sizes = sensor_imx333_setfile_B_sizes;
		sensor_imx333_pllinfos = sensor_imx333_pllinfos_B;
		sensor_imx333_max_setfile_num = sizeof(sensor_imx333_setfiles_B) / sizeof(sensor_imx333_setfiles_B[0]);
	}

	cis->cis_data->cur_width = SENSOR_IMX333_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_IMX333_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
	cis->long_term_mode.sen_strm_off_on_step = 0;
	cis->long_term_mode.sen_strm_off_on_enable = false;

	sensor_imx333_cis_data_calculation(sensor_imx333_pllinfos[setfile_index], cis->cis_data);
	sensor_imx333_set_integration_max_margin(setfile_index, cis->cis_data);
	sensor_imx333_set_integration_min(setfile_index, cis->cis_data);
	sensor_imx333_set_integration_time_step(setfile_index, cis->cis_data);

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

int sensor_imx333_cis_log_status(struct v4l2_subdev *subdev)
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
	pr_err("[SEN:DUMP] *******************************\n");
	cis->ixc_ops->read16(cis->client, 0x0000, &data16);
	pr_err("[SEN:DUMP] model_id(%x)\n", data16);
	cis->ixc_ops->read8(cis->client, 0x0002, &data8);
	pr_err("[SEN:DUMP] revision_number(%x)\n", data8);
	cis->ixc_ops->read8(cis->client, 0x0005, &data8);
	pr_err("[SEN:DUMP] frame_count(%x)\n", data8);
	cis->ixc_ops->read8(cis->client, 0x0100, &data8);
	pr_err("[SEN:DUMP] mode_select(%x)\n", data8);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	pr_err("[SEN:DUMP] *******************************\n");

p_err:
	return ret;
}

#if USE_GROUP_PARAM_HOLD
static int sensor_imx333_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
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
static inline int sensor_imx333_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{ return 0; }
#endif

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_imx333_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_imx333_cis_group_param_hold_func(subdev, hold);
	if (ret < 0)
		goto p_err;

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx333_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	struct is_module_enum *module;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	/* setfile global setting is at camera entrance */
	ret = sensor_cis_set_registers(subdev, sensor_imx333_global, sensor_imx333_global_size);
	if (ret < 0) {
		err("sensor_imx333_set_registers fail!!");
		goto p_err;
	}

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx333_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_module_enum *module;
	struct sensor_open_extended *ext_info;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (mode > sensor_imx333_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx333_check_rev is fail");
			goto p_err;
		}
	}

	sensor_imx333_set_integration_max_margin(mode, cis->cis_data);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_cis_set_registers(subdev, sensor_imx333_setfiles[mode], sensor_imx333_setfile_sizes[mode]);
	if (ret < 0) {
		err("sensor_imx333_set_registers fail!!");
		goto p_err;
	}

	dbg_sensor(1, "[%s] mode changed(%d)\n", __func__, mode);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

/* TODO: Sensor set size sequence(sensor done, sensor stop, 3AA done in FW case */
int sensor_imx333_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
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
	ret = sensor_imx333_wait_stream_off_status(cis_data);
	if (ret) {
		err("Must stream off\n");
		ret = -EINVAL;
		goto p_err;
	}

	binning = cis_data->binning;
	if (binning) {
		ratio_w = (SENSOR_IMX333_MAX_WIDTH / cis_data->cur_width);
		ratio_h = (SENSOR_IMX333_MAX_HEIGHT / cis_data->cur_height);
	} else {
		ratio_w = 1;
		ratio_h = 1;
	}

	if (((cis_data->cur_width * ratio_w) > SENSOR_IMX333_MAX_WIDTH) ||
		((cis_data->cur_height * ratio_h) > SENSOR_IMX333_MAX_HEIGHT)) {
		err("Config max sensor size over~!!\n");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	/* 1. page_select */
	ret = cis->ixc_ops->write16(cis->client, 0x6028, 0x2000);
	if (ret < 0)
		 goto p_err;

	/* 2. pixel address region setting */
	start_x = ((SENSOR_IMX333_MAX_WIDTH - cis_data->cur_width * ratio_w) / 2) & (~0x1);
	start_y = ((SENSOR_IMX333_MAX_HEIGHT - cis_data->cur_height * ratio_h) / 2) & (~0x1);
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
	/* scaling_mode (0: No scaling, 1: Horizontal, 2: Full) */
	ret = cis->ixc_ops->write16(cis->client, 0x0400, 0x0000);
	if (ret < 0)
		goto p_err;
	/* down_scale_m: 1 to 16 upwards (scale_n: 16(fixed))
	down scale factor = down_scale_m / down_scale_n */
	ret = cis->ixc_ops->write16(cis->client, 0x0404, 0x0010);
	if (ret < 0)
		goto p_err;

	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis->cis_data->rolling_shutter_skew = (cis->cis_data->cur_height - 1) * cis->cis_data->line_readOut_time;
	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__, cis->cis_data->frame_time,
                cis->cis_data->rolling_shutter_skew);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx333_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	struct is_device_sensor_peri *sensor_peri = NULL;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	sensor_imx333_cis_group_param_hold_func(subdev, 0x01);

#ifdef DEBUG_IMX333_PLL
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

	/*
	 * If a companion is used,
	 * then 8 ms waiting is needed before the StreamOn of a sensor (IMX333).
	 */
	if (test_bit(IS_SENSOR_PREPROCESSOR_AVAILABLE, &sensor_peri->peri_state)) {
		mdelay(8);
	}

	/* Sensor stream on */
	cis->ixc_ops->write8(cis->client, 0x0100, 0x01);

	sensor_imx333_cis_group_param_hold_func(subdev, 0x00);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = true;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx333_cis_stream_off(struct v4l2_subdev *subdev)
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
	ret = sensor_imx333_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream off");

	cis->ixc_ops->write8(cis->client, 0x0100, 0x00);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = false;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx333_cis_compensate_gain_under_ext_br(struct v4l2_subdev *subdev, u32 expo, u32 *again, u32 *dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 ratio;
	u32 vt_pic_clk_freq_mhz = 0;
	u32 inflated_coarse_int = 0;
	u32 coarse_int = 0;
	u32 line_length_pck = 0;
	u32 min_fine_int = 0;
	u32 expected_again = 0;

	FIMC_BUG(!subdev);
	FIMC_BUG(!again);
	FIMC_BUG(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}
	cis_data = cis->cis_data;

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000 * 1000);
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;

	if (line_length_pck <= 0) {
		err("[%s] invalid line_length_pck(%d)\n", __func__, line_length_pck);
		goto p_err;
	}

	inflated_coarse_int = (u32)((u64)(expo * vt_pic_clk_freq_mhz - min_fine_int)
		* SENSOR_IMX333_DUMMY_MULTIPLIER / line_length_pck);
	coarse_int = (u32)(inflated_coarse_int / SENSOR_IMX333_DUMMY_MULTIPLIER);

	if (coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long_coarse(%d) max(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, coarse_int, cis_data->max_coarse_integration_time);
		inflated_coarse_int = cis_data->max_coarse_integration_time * SENSOR_IMX333_DUMMY_MULTIPLIER;
	}

	if (coarse_int < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long_coarse(%d) min(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, coarse_int, cis_data->min_coarse_integration_time);
		inflated_coarse_int = cis_data->min_coarse_integration_time * SENSOR_IMX333_DUMMY_MULTIPLIER;
	}

	ratio = inflated_coarse_int / coarse_int;

	if(coarse_int <= SENSOR_IMX333_BRIGHT_ILLUMINATION_COMPENSATE_THR)
	{
		if(cis_data->cur_coarse_integration_time_step == SENSOR_IMX333_COARSE_STEP_2N)
		{
			if(coarse_int % SENSOR_IMX333_COARSE_STEP_2N == 1)
			{
				if((coarse_int % SENSOR_IMX333_COARSE_STEP_2N) != 0)
				{
					ratio = inflated_coarse_int / (coarse_int - 1);
				}
			}
		}
		else if(cis_data->cur_coarse_integration_time_step == SENSOR_IMX333_COARSE_STEP_4N)
		{
			if((coarse_int % SENSOR_IMX333_COARSE_STEP_4N) != 0)
			{
				if((coarse_int % SENSOR_IMX333_COARSE_STEP_4N) == 1)
				{
					ratio = inflated_coarse_int / (coarse_int - 1);
				}
				else if((coarse_int % SENSOR_IMX333_COARSE_STEP_4N) == 2)
				{
					ratio = inflated_coarse_int / (coarse_int - 2);
				}
				else if((coarse_int % SENSOR_IMX333_COARSE_STEP_4N) == 3)
				{
					ratio = inflated_coarse_int / (coarse_int - 3);
				}
			}
		}
		else
		{
			err("[%s] Unsupported Coarse Integration Time Step Value:%d\n", __func__, cis_data->cur_coarse_integration_time_step);
		}

		expected_again = (u32)((*again) * ratio) / SENSOR_IMX333_DUMMY_MULTIPLIER;
		if(expected_again < cis_data->min_analog_gain[1])
		{
			*again = cis_data->min_analog_gain[1];
		}
		else if(expected_again > cis_data->max_analog_gain[1])
		{
			*dgain = (u32)((*dgain) * ratio) / SENSOR_IMX333_DUMMY_MULTIPLIER;
		}
		else
		{
			*again = expected_again;
		}
	}
p_err:
	return ret;
}

static void sensor_imx333_cis_calc_integration_time_by_step(u16 *coarse_int, u32 int_time_step)
{
	if(int_time_step == SENSOR_IMX333_COARSE_STEP_2N)
	{
		if((*coarse_int % SENSOR_IMX333_COARSE_STEP_2N) != 0)
		{
			*coarse_int -= 1;
		}

		if(*coarse_int <= SENSOR_IMX333_COARSE_STEP_2N)
		{
			*coarse_int = SENSOR_IMX333_COARSE_STEP_2N;
		}
	}
	else if(int_time_step == SENSOR_IMX333_COARSE_STEP_4N)
	{
		if((*coarse_int % SENSOR_IMX333_COARSE_STEP_4N) != 0)
		{
			if((*coarse_int % SENSOR_IMX333_COARSE_STEP_4N) == 1)
			{
				*coarse_int -= 1;
			}
			else if((*coarse_int % SENSOR_IMX333_COARSE_STEP_4N) == 2)
			{
				*coarse_int -= 2;
			}
			else if((*coarse_int % SENSOR_IMX333_COARSE_STEP_4N) == 3)
			{
				*coarse_int -= 3;
			}
		}

		if(*coarse_int <= SENSOR_IMX333_COARSE_STEP_4N)
		{
			*coarse_int = SENSOR_IMX333_COARSE_STEP_4N;
		}
	}
	else
	{
		info("[%s] Unsupported Coarse Integration Time Step: %d\n", __func__, int_time_step);
	}

	if(*coarse_int <= SENSOR_IMX333_COARSE_STEP_2N)
	{
		*coarse_int = SENSOR_IMX333_COARSE_STEP_2N;
	}
}

int sensor_imx333_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
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
        u8 arrayBuf[4];
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

	sensor_imx333_cis_calc_integration_time_by_step(&long_coarse_int, cis_data->cur_coarse_integration_time_step);
	sensor_imx333_cis_calc_integration_time_by_step(&short_coarse_int, cis_data->cur_coarse_integration_time_step);

        cis_data->cur_long_exposure_coarse = long_coarse_int;
        cis_data->cur_short_exposure_coarse = short_coarse_int;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/* WDR mode off */
	if (is_vendor_wdr_mode_on(cis_data)) {
                cis->ixc_ops->write8(cis->client, 0x0220, 0x61);
        } else {
                cis->ixc_ops->write8(cis->client, 0x0220, 0x00);
        }

        /* Long exposure */
        arrayBuf[0] = (cis_data->cur_long_exposure_coarse & 0xFF00) >> 8;
        arrayBuf[1] = cis_data->cur_long_exposure_coarse & 0xFF;
	ret = cis->ixc_ops->write8_array(client, 0x0202, arrayBuf, 2);
	if (ret < 0)
		goto p_err;

	/* Short exposure */
	/* if (is_vendor_wdr_mode_on(cis_data)) */
	{
		arrayBuf[0] = (cis_data->cur_short_exposure_coarse & 0xFF00) >> 8;
		arrayBuf[1] = cis_data->cur_short_exposure_coarse & 0xFF;
		ret = cis->ixc_ops->write8_array(client, 0x0224, arrayBuf, 2);
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
		hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx333_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
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

int sensor_imx333_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
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

int sensor_imx333_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
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
	frame_length_lines = ((vt_pic_clk_freq_mhz * input_exposure_time) / line_length_pck);
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

int sensor_imx333_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
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
			cis->id, __func__, vt_pic_clk_freq_mhz, frame_duration,
			line_length_pck, frame_length_lines);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x01);
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
		hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx333_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
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

	ret = sensor_imx333_cis_set_frame_duration(subdev, frame_duration);
	if (ret < 0) {
		err("[MOD:D:%d] %s, set frame duration is fail(%d)\n",
			cis->id, __func__, ret);
		goto p_err;
	}

	cis_data->min_frame_us_time = frame_duration;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:

	return ret;
}

u32 sensor_imx333_cis_calc_again_code(u32 permille)
{
	return 512 - (512000 / permille);
}

u32 sensor_imx333_cis_calc_again_permile(u32 code)
{
	return 512000 / (512 - code);
}

int sensor_imx333_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
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

	again_permile = sensor_imx333_cis_calc_again_permile(again_code);

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

int sensor_imx333_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 analog_gain = 0;
        u8 arrayBuf[2];
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

	analog_gain = (u16)sensor_imx333_cis_calc_again_code(again->val);

	if (analog_gain < cis->cis_data->min_analog_gain[0]) {
		analog_gain = cis->cis_data->min_analog_gain[0];
	}

	if (analog_gain > cis->cis_data->max_analog_gain[0]) {
		analog_gain = cis->cis_data->max_analog_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_again = %d us, analog_gain(%#x)\n",
		cis->id, __func__, cis->cis_data->sen_vsync_count, again->val, analog_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

        /* Analog gain */
        arrayBuf[0] = (analog_gain & 0xFF00) >> 8;
        arrayBuf[1] = analog_gain & 0xFF;
	ret = cis->ixc_ops->write8_array(client, 0x0204, arrayBuf, 2);
	if (ret < 0)
		goto p_err;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx333_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
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
	hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x0204, &analog_gain);
	if (ret < 0)
		goto p_err;

	*again = sensor_imx333_cis_calc_again_permile(analog_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_again = %d us, analog_gain(%#x)\n",
			cis->id, __func__, *again, analog_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx333_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
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

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

        cis_data->min_analog_gain[0] = 0x0;
        cis_data->min_analog_gain[1] = sensor_imx333_cis_calc_again_permile(cis_data->min_analog_gain[0]);
        *min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->min_analog_gain[0],
		cis_data->min_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx333_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
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

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
        cis_data->max_analog_gain[0] = 0x1C0;
        cis_data->max_analog_gain[1] = sensor_imx333_cis_calc_again_permile(cis_data->max_analog_gain[0]);
        *max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->max_analog_gain[0],
		cis_data->max_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx333_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 long_gain = 0;
	u16 short_gain = 0;
	u8 dgains[2] = {0};
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
		long_gain = cis->cis_data->min_digital_gain[0];
	}
	if (long_gain > cis->cis_data->max_digital_gain[0]) {
		long_gain = cis->cis_data->max_digital_gain[0];
	}

	if (short_gain < cis->cis_data->min_digital_gain[0]) {
		short_gain = cis->cis_data->min_digital_gain[0];
	}
	if (short_gain > cis->cis_data->max_digital_gain[0]) {
		short_gain = cis->cis_data->max_digital_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_dgain = %d/%d us,"
			KERN_CONT "long_gain(%#x), short_gain(%#x)\n",
			cis->id, __func__, cis->cis_data->sen_vsync_count,
			dgain->long_val, dgain->short_val, long_gain, short_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/* Set current short & long digitial gain */
	/* 0x0226 ~ 0x0227 : ST_DIG_GAIN_GLOBAL */
	/* 0x020E ~ 0x020F : DIG_GAIN_GLOBAL */
	/* if (is_vendor_wdr_mode_on(cis_data)) */
	{
		dgains[0] = (short_gain & 0xFF00) >> 8;
		dgains[1] = short_gain & 0xFF;
		ret = cis->ixc_ops->write8_array(client, 0x0226, dgains, 2);
		if (ret < 0)
			goto p_err;
	}
	dgains[0] = (long_gain & 0xFF00) >> 8;
	dgains[1] = long_gain & 0xFF;
	ret = cis->ixc_ops->write8_array(client, 0x020E, dgains, 2);
	if (ret < 0)
		goto p_err;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx333_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
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
	hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x01);
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
		hold = sensor_imx333_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx333_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
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

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
        cis_data->min_digital_gain[0] = 0x100;
        cis_data->min_digital_gain[1] = sensor_cis_calc_dgain_permile(cis_data->min_digital_gain[0]);

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->min_digital_gain[0],
		cis_data->min_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx333_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
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

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	cis_data->max_digital_gain[0] = 0xfff;
	cis_data->max_digital_gain[1] = sensor_cis_calc_dgain_permile(cis_data->max_digital_gain[0]);

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->max_digital_gain[0],
		cis_data->max_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx333_cis_long_term_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_long_term_expo_mode *lte_mode;
	unsigned char cit_lshift_val = 0;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	lte_mode = &cis->long_term_mode;

	/* LTE mode or normal mode set */
	if (lte_mode->sen_strm_off_on_enable) {
		if (lte_mode->expo[0] > 500000) {
			cit_lshift_val = (unsigned char)(lte_mode->expo[0] / 500000);
			cit_lshift_val = cit_lshift_val / 2;
			lte_mode->expo[0] = 500000;
			ret |= cis->ixc_ops->write8(cis->client, 0x3008, cit_lshift_val);
		}
	} else {
		cit_lshift_val = 0;
		ret |= cis->ixc_ops->write8(cis->client, 0x3008, cit_lshift_val);
	}

	info("%s enable(%d) %d %d", __func__, lte_mode->sen_strm_off_on_enable, cit_lshift_val, lte_mode->expo[0]);

	if (ret < 0) {
		pr_err("ERR[%s]: LTE register setting fail\n", __func__);
		return ret;
	}

	return ret;
}
static struct is_cis_ops cis_ops_imx333 = {
	.cis_init = sensor_imx333_cis_init,
	.cis_log_status = sensor_imx333_cis_log_status,
	.cis_group_param_hold = sensor_imx333_cis_group_param_hold,
	.cis_set_global_setting = sensor_imx333_cis_set_global_setting,
	.cis_mode_change = sensor_imx333_cis_mode_change,
	.cis_set_size = sensor_imx333_cis_set_size,
	.cis_stream_on = sensor_imx333_cis_stream_on,
	.cis_stream_off = sensor_imx333_cis_stream_off,
	.cis_set_exposure_time = sensor_imx333_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_imx333_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_imx333_cis_get_max_exposure_time,
	.cis_adjust_frame_duration = sensor_imx333_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_imx333_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_imx333_cis_set_frame_rate,
	.cis_adjust_analog_gain = sensor_imx333_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_imx333_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_imx333_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_imx333_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_imx333_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_imx333_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_imx333_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_imx333_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_imx333_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_imx333_cis_compensate_gain_under_ext_br,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_data_calculation = sensor_imx333_cis_data_calc,
	.cis_set_long_term_exposure = sensor_imx333_cis_long_term_exposure,
};

static int cis_imx333_probe_i2c(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
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
	cis->cis_ops = &cis_ops_imx333;
	/* belows are depend on sensor cis. MUST check sensor spec */
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;

	ret = of_property_read_string(dnode, "setfile", &setfile);
	if (ret) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	probe_info("%s done\n", __func__);

	return ret;
}

static const struct of_device_id sensor_cis_imx333_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-cis-imx333",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx333_match);

static const struct i2c_device_id sensor_cis_imx333_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx333_driver = {
	.probe	= cis_imx333_probe_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx333_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx333_idt
};

static int __init sensor_cis_imx333_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_imx333_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_imx333_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx333_init);
