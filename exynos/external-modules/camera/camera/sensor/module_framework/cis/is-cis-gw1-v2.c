/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
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
#include <linux/syscalls.h>
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
#include "is-cis-gw1-v2.h"
#include "is-cis-gw1-setC.h"

#include "is-helper-ixc.h"

#include "interface/is-interface-library.h"

#define SENSOR_NAME "S5KGW1_V2"
/* #define DEBUG_GW1_PLL */

static const u32 *sensor_gw1_global;
static u32 sensor_gw1_global_size;
static const u32 **sensor_gw1_setfiles;
static const u32 *sensor_gw1_setfile_sizes;
static u32 sensor_gw1_max_setfile_num;
static const struct sensor_pll_info_compact **sensor_gw1_pllinfos;
static u8 cit_lshift_val;
static const struct sensor_crop_xy_info *sensor_gw1_crop_xy;

static int current_mode;

static void sensor_gw1_cis_data_calculation(const struct sensor_pll_info_compact *pll_info_compact, cis_shared_data *cis_data)
{
	u64 vt_pix_clk_hz;
	u32 frame_rate, frame_valid_us;
	u64 max_fps;

	FIMC_BUG_VOID(!pll_info_compact);

	/* 1. get pclk value from pll info */
	vt_pix_clk_hz = pll_info_compact->pclk;

	dbg_sensor(1, "ext_clock(%d), mipi_datarate(%llu), pclk(%llu)\n",
			pll_info_compact->ext_clk, pll_info_compact->mipi_datarate, pll_info_compact->pclk);

	/* 2. the time of processing one frame calculation (us) */
	cis_data->min_frame_us_time = (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck
					/ (vt_pix_clk_hz / (1000 * 1000)));
	cis_data->cur_frame_us_time = cis_data->min_frame_us_time;

	/* 3. FPS calculation */
	frame_rate = vt_pix_clk_hz / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	dbg_sensor(1, "frame_rate (%d) = vt_pix_clk_hz(%llu) / "
		KERN_CONT "(pll_info_compact->frame_length_lines(%d) * pll_info_compact->line_length_pck(%d))\n",
		frame_rate, vt_pix_clk_hz, pll_info_compact->frame_length_lines, pll_info_compact->line_length_pck);

	/* calculate max fps */
	max_fps = ((u64)vt_pix_clk_hz * 10) / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	max_fps = (max_fps % 10 >= 5 ? frame_rate + 1 : frame_rate);

	cis_data->pclk = vt_pix_clk_hz;
	cis_data->max_fps = (u32)max_fps;
	cis_data->frame_length_lines = pll_info_compact->frame_length_lines;
	cis_data->line_length_pck = pll_info_compact->line_length_pck;
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
	cis_data->min_fine_integration_time = SENSOR_GW1_FINE_INTEGRATION_TIME_MIN;
	cis_data->max_fine_integration_time = cis_data->cur_width;
	cis_data->min_coarse_integration_time = SENSOR_GW1_COARSE_INTEGRATION_TIME_MIN;
	cis_data->max_margin_coarse_integration_time = SENSOR_GW1_COARSE_INTEGRATION_TIME_MAX_MARGIN;
}

static void sensor_gw1_cis_get_crop_xy(const struct sensor_crop_xy_info *crop_xy_info, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!crop_xy_info);

	cis_data->crop_x = crop_xy_info->crop_x;
	cis_data->crop_y = crop_xy_info->crop_y;
}

static int sensor_gw1_cis_check_rev(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u8 rev = 0;
	struct is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	memset(cis->cis_data, 0, sizeof(cis_shared_data));
	cis->rev_flag = false;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = cis->ixc_ops->write16(cis->client, 0x6028, 0x4000);
	if (ret < 0) {
		ret = -EAGAIN;
	}

	ret = cis->ixc_ops->read8(cis->client, 0x0000, &rev);
	if (ret < 0) {
		ret = -EAGAIN;
	} else {
		pr_info("%s : model_id(0x%x) \n", __func__, rev);
	}

	ret = cis->ixc_ops->read8(cis->client, 0x0002, &rev);
	if (ret < 0) {
		cis->rev_flag = true;
		ret = -EAGAIN;
	} else {
		cis->cis_data->cis_rev = rev;
		pr_info("%s : Rev. 0x%X\n", __func__, rev);
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	if (rev == 0XA2) {
		probe_info("%s EVT0.2\n", __func__);
		sensor_gw1_global = sensor_gw1_setfile_C_EVT0P2_Global;
		sensor_gw1_global_size = ARRAY_SIZE(sensor_gw1_setfile_C_EVT0P2_Global);
    	}
	return ret;
}

/* CIS OPS */
static int sensor_gw1_cis_init(struct v4l2_subdev *subdev)
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
	memset(cis->cis_data, 0, sizeof(cis_shared_data));

	cis->cis_data->cur_width = SENSOR_GW1_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_GW1_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;

	sensor_gw1_cis_data_calculation(sensor_gw1_pllinfos[setfile_index], cis->cis_data);
	sensor_gw1_cis_get_crop_xy(&sensor_gw1_crop_xy[setfile_index], cis->cis_data);

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
	setinfo.return_value = 0;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

static int sensor_gw1_cis_log_status(struct v4l2_subdev *subdev)
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

	sensor_cis_dump_registers(subdev, sensor_gw1_setfiles[0], sensor_gw1_setfile_sizes[0]);

	pr_err("[SEN:DUMP] *******************************\n");

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

#if USE_GROUP_PARAM_HOLD
static int sensor_gw1_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
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
static inline int sensor_gw1_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{
	return 0;
}
#endif

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
static int sensor_gw1_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_gw1_cis_group_param_hold_func(subdev, hold);
	if (ret < 0)
		goto p_err;

p_err:

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

static int sensor_gw1_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	current_mode = -1;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_cis_set_registers(subdev, sensor_gw1_global, sensor_gw1_global_size);

	if (ret < 0) {
		err("%s sensor_cis_set_registers fail!!", __func__);
		goto p_err;
	}

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

static int sensor_gw1_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (mode >= sensor_gw1_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	if (mode == current_mode)
		goto p_err;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_cis_set_registers(subdev, sensor_gw1_setfiles[mode], sensor_gw1_setfile_sizes[mode]);
	if (ret < 0) {
		err("%s sensor_cis_set_registers fail!!", __func__);
		goto p_err_mutex;
	}

	cis->cis_data->frame_time = (cis->cis_data->line_readOut_time * cis->cis_data->cur_height / 1000);
	cis->cis_data->rolling_shutter_skew = (cis->cis_data->cur_height - 1) * cis->cis_data->line_readOut_time;
	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
		cis->cis_data->frame_time, cis->cis_data->rolling_shutter_skew);

	dbg_sensor(1, "[%s] mode changed(%d)\n", __func__, mode);

	current_mode = mode;

p_err_mutex:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

/* Deprecated */
static int sensor_gw1_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
{
	return 0;
}

static int sensor_gw1_cis_stream_on(struct v4l2_subdev *subdev)
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

	if (cis->stream_state != CIS_STREAM_SET_DONE) {
		err("%s: cis stream state id %d", __func__, cis->stream_state);
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream on");

#ifdef DEBUG_GW1_PLL
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
	cis->ixc_ops->write16(cis->client, 0x6028, 0x4000);
	cis->ixc_ops->write16(cis->client, 0x6214, 0xF9F0);
	cis->ixc_ops->write16(cis->client, 0x6218, 0xF9F0);
	cis->ixc_ops->write8(cis->client, 0x0100, 0x01);

	/* WDR */
	if (is_vendor_wdr_mode_on(cis_data))
		cis->ixc_ops->write8(cis->client, 0x021E, 0x01);
	else
		cis->ixc_ops->write8(cis->client, 0x021E, 0x00);

	cis_data->stream_on = true;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

static int sensor_gw1_cis_stream_off(struct v4l2_subdev *subdev)
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

	ret = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream off");

	/* Sensor stream off */
	cis->ixc_ops->write16(cis->client, 0x6028, 0x4000);
	cis->ixc_ops->write8(cis->client, 0x0100, 0x00);

	cis_data->stream_on = false;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

static int sensor_gw1_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
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
	u64 _long_coarse_int = 0;
	u64 _short_coarse_int = 0;
	u64 numerator;
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

	numerator = (u64)cis_data->pclk * target_exposure->long_val;
	_long_coarse_int = (numerator - min_fine_int)
		/(1000 * 1000) / line_length_pck / (1 << cit_lshift_val);
	numerator = (u64)cis_data->pclk * target_exposure->short_val;
	_short_coarse_int = (numerator - min_fine_int)
		/(1000 * 1000) / line_length_pck / (1 << cit_lshift_val);

	long_coarse_int = _long_coarse_int & 0xffff;
	short_coarse_int = _short_coarse_int & 0xffff;

	if (long_coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) max(%d)\n",
			   cis->id, __func__, cis_data->sen_vsync_count,
			   long_coarse_int, cis_data->max_coarse_integration_time);
		long_coarse_int = cis_data->max_coarse_integration_time;
	}

	if (short_coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) max(%d)\n",
			   cis->id, __func__, cis_data->sen_vsync_count,
			   short_coarse_int, cis_data->max_coarse_integration_time);
		short_coarse_int = cis_data->max_coarse_integration_time;
	}

	if (long_coarse_int < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n",
			   cis->id, __func__, cis_data->sen_vsync_count,
			   long_coarse_int, cis_data->min_coarse_integration_time);
		long_coarse_int = cis_data->min_coarse_integration_time;
	}

	if (short_coarse_int < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) min(%d)\n",
			   cis->id, __func__, cis_data->sen_vsync_count,
			   short_coarse_int, cis_data->min_coarse_integration_time);
		short_coarse_int = cis_data->min_coarse_integration_time;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_mutex;
	}

	/* Short exposure */
	ret = cis->ixc_ops->write16(cis->client, 0x0202, short_coarse_int);
	if (ret < 0)
		goto p_err_mutex;

	/* Long exposure */
	if (is_vendor_wdr_mode_on(cis_data)) {
		ret = cis->ixc_ops->write16(cis->client, 0x0226, long_coarse_int);
		if (ret < 0)
			goto p_err_mutex;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), vt_pic_clk_freq_mhz (%d), line_length_pck(%d), min_fine_int (%d)\n",
		cis->id, __func__, cis_data->sen_vsync_count, vt_pic_clk_freq_mhz, line_length_pck, min_fine_int);
	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), frame_length_lines(%#x), long_coarse_int %#x, short_coarse_int %#x\n",
		cis->id, __func__, cis_data->sen_vsync_count, cis_data->frame_length_lines,
		long_coarse_int, short_coarse_int);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_mutex:
	if (hold > 0) {
		hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

static int sensor_gw1_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
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

static int sensor_gw1_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
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

	/* TODO: Is this values update here? */
	cis_data->max_margin_fine_integration_time = max_fine_margin;
	cis_data->max_coarse_integration_time = max_coarse;

	dbg_sensor(1, "[%s] max integration time %d, max margin fine integration %d, max coarse integration %d\n",
			__func__, max_integration_time, cis_data->max_margin_fine_integration_time,
			cis_data->max_coarse_integration_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

static int sensor_gw1_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
						u32 input_exposure_time,
						u32 *target_duration)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	u64 frame_duration = 0;
	u64 numerator;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_duration);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000 * 1000);
	line_length_pck = cis_data->line_length_pck;
	numerator = (u64)cis_data->pclk * input_exposure_time;
	frame_length_lines = (u16)(((numerator / (1000 * 1000)) / line_length_pck)
				   / (1 << cit_lshift_val));
	frame_length_lines += cis_data->max_margin_coarse_integration_time;

	numerator = (u64)frame_length_lines * line_length_pck;
	frame_duration = numerator / vt_pic_clk_freq_mhz
		* (1 << cit_lshift_val);

	dbg_sensor(1, "[%s](vsync cnt = %d) input exp(%d), adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, cis_data->sen_vsync_count, input_exposure_time,
			frame_duration, cis_data->min_frame_us_time);
	dbg_sensor(1, "[%s](vsync cnt = %d) adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, cis_data->sen_vsync_count, frame_duration, cis_data->min_frame_us_time);

	*target_duration = MAX(frame_duration, cis_data->min_frame_us_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

static int sensor_gw1_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 line_length_pck = 0;
	u16 frame_length_lines = 0;
	u64 numerator;
	u32 max_coarse_integration_time = 0;
	u64 _frame_length_lines = 0;
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

	line_length_pck = cis_data->line_length_pck;
	numerator = (u64)cis_data->pclk * frame_duration;

	_frame_length_lines = (u64)((numerator / line_length_pck) / (1000 * 1000)
				    / (1 << cit_lshift_val));
	frame_length_lines = _frame_length_lines & 0xffff;

	dbg_sensor(1, "[MOD:D:%d] %s, vt_pic_clk(%#x) frame_duration = %d us,"
		KERN_CONT "(line_length_pck%#x), frame_length_lines(%#x)\n",
		cis->id, __func__, cis_data->pclk, frame_duration, line_length_pck, frame_length_lines);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_mutex;
	}

	/* NOTE: if cis is working as master, the frame duration need to be larger than slave cis. */
	if (cis->dual_sync_mode == DUAL_SYNC_MASTER)
		frame_length_lines += 2;

	ret = cis->ixc_ops->write16(cis->client, 0x0340, frame_length_lines);
	if (ret < 0)
		goto p_err_mutex;

	cis_data->cur_frame_us_time = frame_duration;
	cis_data->frame_length_lines = frame_length_lines;

	max_coarse_integration_time = cis_data->frame_length_lines - cis_data->max_margin_coarse_integration_time;
	cis_data->max_coarse_integration_time = max_coarse_integration_time;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_mutex:
	if (hold > 0) {
		hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

static int sensor_gw1_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
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

	ret = sensor_gw1_cis_set_frame_duration(subdev, frame_duration);
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

static int sensor_gw1_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
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

	if (again_code > cis_data->max_analog_gain[0])
		again_code = cis_data->max_analog_gain[0];
	else if (again_code < cis_data->min_analog_gain[0])
		again_code = cis_data->min_analog_gain[0];

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

static int sensor_gw1_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
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

	if (analog_gain < cis->cis_data->min_analog_gain[0])
		analog_gain = cis->cis_data->min_analog_gain[0];

	if (analog_gain > cis->cis_data->max_analog_gain[0])
		analog_gain = cis->cis_data->max_analog_gain[0];

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_again = %d us, analog_gain(%#x)\n",
		cis->id, __func__, cis->cis_data->sen_vsync_count, again->val, analog_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_mutex;
	}

	ret = cis->ixc_ops->write16(cis->client, 0x0204, analog_gain);
	if (ret < 0)
		goto p_err_mutex;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_mutex:
	if (hold > 0) {
		hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

static int sensor_gw1_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
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

	hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_mutex;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x0204, &analog_gain);
	if (ret < 0)
		goto p_err_mutex;

	*again = sensor_cis_calc_again_permile(analog_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_again = %d us, analog_gain(%#x)\n",
			cis->id, __func__, *again, analog_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_mutex:
	if (hold > 0) {
		hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

static int sensor_gw1_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
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

	cis_data->min_analog_gain[0] = SENSOR_GW1_V2_MIN_ANALOG_GAIN;

	cis_data->min_analog_gain[1] = sensor_cis_calc_again_permile(SENSOR_GW1_V2_MIN_ANALOG_GAIN);

	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->min_analog_gain[0], cis_data->min_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));p_err:

	return ret;
}

static int sensor_gw1_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
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

	cis_data->max_analog_gain[0] = SENSOR_GW1_V2_MAX_ANALOG_GAIN;

	cis_data->max_analog_gain[1] = sensor_cis_calc_again_permile(SENSOR_GW1_V2_MAX_ANALOG_GAIN);

	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->max_analog_gain[0], cis_data->max_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:

	return ret;
}

static int sensor_gw1_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u16 long_gain = 0;
	u16 short_gain = 0;
	u16 dgains[4] = {0};\
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

	if (long_gain < cis->cis_data->min_digital_gain[0])
		long_gain = cis->cis_data->min_digital_gain[0];

	if (long_gain > cis->cis_data->max_digital_gain[0])
		long_gain = cis->cis_data->max_digital_gain[0];

	if (short_gain < cis->cis_data->min_digital_gain[0])
		short_gain = cis->cis_data->min_digital_gain[0];

	if (short_gain > cis->cis_data->max_digital_gain[0])
		short_gain = cis->cis_data->max_digital_gain[0];

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_dgain = %d/%d us, long_gain(%#x), short_gain(%#x)\n",
		cis->id, __func__, cis->cis_data->sen_vsync_count, dgain->long_val,
		dgain->short_val, long_gain, short_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_mutex;
	}

	dgains[0] = dgains[1] = dgains[2] = dgains[3] = short_gain;
	/* Short digital gain */
	ret = cis->ixc_ops->write16_array(cis->client, 0x020E, dgains, 4);
	if (ret < 0)
		goto p_err_mutex;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_mutex:
	if (hold > 0) {
		hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

static int sensor_gw1_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
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

	hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_mutex;
	}

	ret = cis->ixc_ops->read16(cis->client, 0x020E, &digital_gain);
	if (ret < 0)
		goto p_err_mutex;

	*dgain = sensor_cis_calc_dgain_permile(digital_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_dgain = %d us, digital_gain(%#x)\n",
			cis->id, __func__, *dgain, digital_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_mutex:
	if (hold > 0) {
		hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

static int sensor_gw1_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
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

	cis_data->min_digital_gain[0] = SENSOR_GW1_V2_MIN_DIGITAL_GAIN;

	cis_data->min_digital_gain[1] = sensor_cis_calc_dgain_permile(SENSOR_GW1_V2_MIN_DIGITAL_GAIN);

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_digital_gain[0], cis_data->min_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));p_err:

	return ret;
}

static int sensor_gw1_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
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

	cis_data->max_digital_gain[0] = SENSOR_GW1_V2_MAX_DIGITAL_GAIN;

	cis_data->max_digital_gain[1] = sensor_cis_calc_dgain_permile(SENSOR_GW1_V2_MAX_DIGITAL_GAIN);

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_digital_gain[0], cis_data->max_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));p_err:

	return ret;
}

static int sensor_gw1_cis_long_term_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_long_term_expo_mode *lte_mode;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	lte_mode = &cis->long_term_mode;

	cit_lshift_val = 0;
	/* LTE mode or normal mode set */
	if (lte_mode->sen_strm_off_on_enable &&
	    lte_mode->expo[0] > 500000) {
		cit_lshift_val = 7;
	} else {
		cit_lshift_val = 0;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret |= cis->ixc_ops->write8(cis->client, 0x0702, cit_lshift_val);
	ret |= cis->ixc_ops->write8(cis->client, 0x0704, cit_lshift_val);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	info("%s enable(%d) %d %d", __func__, lte_mode->sen_strm_off_on_enable, cit_lshift_val, lte_mode->expo[0]);

	if (ret < 0) {
		pr_err("ERR[%s]: LTE register setting fail\n", __func__);
		return ret;
	}

	return ret;
}

static int sensor_gw1_cis_set_dual_setting(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis;
	const u32 *sensor_dual_setting;
	u32 sensor_dual_setting_size;

	info("%s E %d", __func__, mode);
	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);

	switch (mode) {
	case DUAL_SYNC_MASTER:
		sensor_dual_setting_size = sensor_gw1_cis_dual_master_settings_size;
		sensor_dual_setting = sensor_gw1_cis_dual_master_settings;
		break;
	case DUAL_SYNC_SLAVE:
		sensor_dual_setting_size = sensor_gw1_cis_dual_slave_settings_size;
		sensor_dual_setting = sensor_gw1_cis_dual_slave_settings;
		break;
	case DUAL_SYNC_STANDALONE:
		sensor_dual_setting_size = sensor_gw1_cis_dual_standalone_settings_size;
		sensor_dual_setting = sensor_gw1_cis_dual_standalone_settings;
		break;
	default:
		err("invalid cis->dual_sync_mode(%d)\n", cis->dual_sync_mode);
		ret = -EINVAL;
		goto p_err;
	}

	ret = sensor_cis_set_registers(subdev, sensor_dual_setting, sensor_dual_setting_size);
p_err:
	info("%s X %d", __func__, ret);
	return ret;
}

static void sensor_gw1_cis_data_calc(struct v4l2_subdev *subdev, u32 mode)
{
	struct is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (mode >= sensor_gw1_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return;
	}

	sensor_gw1_cis_data_calculation(sensor_gw1_pllinfos[mode], cis->cis_data);
	sensor_gw1_cis_get_crop_xy(&sensor_gw1_crop_xy[mode], cis->cis_data);
}

static int sensor_gw1_cis_set_wb_gain(struct v4l2_subdev *subdev, struct wb_gains wb_gains)
{
	const u16 addr_page = SENSOR_GW1_ABS_GAIN_PAGE;
	const u16 addr_offset = SENSOR_GW1_ABS_GAIN_OFFSET;
	struct is_cis *cis;
	int i, ret = 0;
	int hold = 0;
	int mode = 0;
	u16 abs_gains[3] = {0, }; /* R, G, B */
	u32 avg_g = 0, div = 0;
	ktime_t st = ktime_get();

	dbg_sensor(1, "[%s] E\n", __func__);
	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	if (!cis->use_wb_gain)
		return 0;

	mode = cis->cis_data->sens_config_index_cur;

	if (wb_gains.gr != wb_gains.gb) {
		err("gr, gb not euqal"); /* check DDK layer */
		return -EINVAL;
	}

	if (wb_gains.gr == 1024)
		div = 4;
	else if (wb_gains.gr == 2048)
		div = 8;
	else {
		err("invalid gr,gb %d", wb_gains.gr); /* check DDK layer */
		return -EINVAL;
	}

	dbg_sensor(1, "[SEN:%d]%s: DDK vlaue: wb_gain_gr(%d), wb_gain_r(%d), wb_gain_b(%d), wb_gain_gb(%d)\n",
		cis->id, __func__, wb_gains.gr, wb_gains.r, wb_gains.b, wb_gains.gb);

	avg_g = (wb_gains.gr + wb_gains.gb) / 2;
	abs_gains[0] = (u16)((wb_gains.r / div) & 0xFFFF);
	abs_gains[1] = (u16)((avg_g / div) & 0xFFFF);
	abs_gains[2] = (u16)((wb_gains.b / div) & 0xFFFF);

	dbg_sensor(1, "[SEN:%d]%s: abs_gain_r(0x%4X), abs_gain_g(0x%4X), abs_gain_b(0x%4X)\n",
		cis->id, __func__, abs_gains[0], abs_gains[1], abs_gains[2]);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = cis->ixc_ops->write16(cis->client, SENSOR_GW1_W_DIR_PAGE, addr_page);
	if (ret < 0) {
		err("i2c fail addr(%x, %x) ,ret = %d\n", SENSOR_GW1_W_DIR_PAGE, addr_page, ret);
		goto p_err;
	}
	for (i = 0; i < ARRAY_SIZE(abs_gains); i++) {
		ret = cis->ixc_ops->write16(cis->client, addr_offset + (i * 2), abs_gains[i]);
		dbg_sensor(2, "[wbgain] %d: 0x%04X, 0x%04X\n", i, addr_offset + (i * 2), abs_gains[i]);
		if (ret < 0) {
			err("i2c fail addr(%x), val(%04x), ret = %d\n",
				addr_offset + (i * 2), abs_gains[i], ret);
			goto p_err;
		}
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_gw1_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	dbg_sensor(1, "[%s] X (%d)\n", __func__, ret);
	return ret;
}

static struct is_cis_ops cis_ops = {
	.cis_init				= sensor_gw1_cis_init,
	.cis_log_status				= sensor_gw1_cis_log_status,
	.cis_group_param_hold			= sensor_gw1_cis_group_param_hold,
	.cis_set_global_setting			= sensor_gw1_cis_set_global_setting,
	.cis_mode_change			= sensor_gw1_cis_mode_change,
	.cis_set_size				= sensor_gw1_cis_set_size,
	.cis_stream_on				= sensor_gw1_cis_stream_on,
	.cis_stream_off				= sensor_gw1_cis_stream_off,
	.cis_set_exposure_time			= sensor_gw1_cis_set_exposure_time,
	.cis_get_min_exposure_time		= sensor_gw1_cis_get_min_exposure_time,
	.cis_get_max_exposure_time		= sensor_gw1_cis_get_max_exposure_time,
	.cis_adjust_frame_duration		= sensor_gw1_cis_adjust_frame_duration,
	.cis_set_frame_duration			= sensor_gw1_cis_set_frame_duration,
	.cis_set_frame_rate			= sensor_gw1_cis_set_frame_rate,
	.cis_adjust_analog_gain			= sensor_gw1_cis_adjust_analog_gain,
	.cis_set_analog_gain			= sensor_gw1_cis_set_analog_gain,
	.cis_get_analog_gain			= sensor_gw1_cis_get_analog_gain,
	.cis_get_min_analog_gain		= sensor_gw1_cis_get_min_analog_gain,
	.cis_get_max_analog_gain		= sensor_gw1_cis_get_max_analog_gain,
	.cis_set_digital_gain			= sensor_gw1_cis_set_digital_gain,
	.cis_get_digital_gain			= sensor_gw1_cis_get_digital_gain,
	.cis_get_min_digital_gain		= sensor_gw1_cis_get_min_digital_gain,
	.cis_get_max_digital_gain		= sensor_gw1_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br	= sensor_cis_compensate_gain_for_extremely_br,
	.cis_wait_streamoff			= sensor_cis_wait_streamoff,
	.cis_wait_streamon			= sensor_cis_wait_streamon,
	.cis_set_initial_exposure		= sensor_cis_set_initial_exposure,
	.cis_check_rev_on_init			= sensor_gw1_cis_check_rev,
	.cis_set_long_term_exposure		= sensor_gw1_cis_long_term_exposure,
	.cis_set_dual_setting			= sensor_gw1_cis_set_dual_setting,
	.cis_data_calculation			= sensor_gw1_cis_data_calc,
	.cis_get_bayer_order            = sensor_cis_get_bayer_order,
	.cis_set_wb_gains            = sensor_gw1_cis_set_wb_gain,
};

static int cis_gw1_probe_i2c(struct i2c_client *client,
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
	cis->cis_ops = &cis_ops;
	/* belows are depend on sensor cis. MUST check sensor spec */
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;
	cis->use_wb_gain = true;

	ret = of_property_read_string(dnode, "setfile", &setfile);
	if (ret) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setC") == 0) {
		sensor_gw1_global = sensor_gw1_setfile_C_Global;
		sensor_gw1_global_size = ARRAY_SIZE(sensor_gw1_setfile_C_Global);
		sensor_gw1_setfiles = sensor_gw1_setfiles_C;
		sensor_gw1_setfile_sizes = sensor_gw1_setfile_C_sizes;
		sensor_gw1_pllinfos = sensor_gw1_pllinfos_C;
		sensor_gw1_max_setfile_num = ARRAY_SIZE(sensor_gw1_setfiles_C);
		sensor_gw1_crop_xy = sensor_gw1_crop_xy_C;
		sensor_gw1_binning_ratio = sensor_gw1_binning_ratio_C;
	}

	current_mode = -1;

	probe_info("%s done\n", __func__);

	return ret;
}

static const struct of_device_id sensor_cis_gw1_match[] = {
	{
		.compatible = "samsung,exynos-is-cis-gw1-v2",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_gw1_match);

static const struct i2c_device_id sensor_cis_gw1_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_gw1_driver = {
	.probe	= cis_gw1_probe_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_gw1_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_gw1_idt
};

#ifdef MODULE
builtin_i2c_driver(sensor_cis_gw1_driver);
#else
static int __init sensor_cis_gw1_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_gw1_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_gw1_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_gw1_init);
#endif

MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: fimc-is");
