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
#include "is-cis-imx316.h"
#include "is-cis-imx316-setA.h"
#include "is-cis-imx316-setB.h"
#include "is-vendor-private.h"
#include "is-helper-ixc.h"

#define SENSOR_NAME "IMX316"
/* #define DEBUG_imx316_PLL */

#define SENSOR_EXTAREA_INTG_0_A_0 0x2110
#define SENSOR_EXTAREA_INTG_0_A_1 0x2111
#define SENSOR_EXTAREA_INTG_0_A_2 0x2112
#define SENSOR_EXTAREA_INTG_0_A_3 0x2113

#define SENSOR_EXTAREA_INTG_0_B_0 0x2114
#define SENSOR_EXTAREA_INTG_0_B_1 0x2115
#define SENSOR_EXTAREA_INTG_0_B_2 0x2116
#define SENSOR_EXTAREA_INTG_0_B_3 0x2117

#define SENSOR_LASER_CONTROL_REG_1 0x2134
#define SENSOR_LASER_CONTROL_REG_2 0x2135
#define SENSOR_LASER_OFF  0xAA
#define SENSOR_LASER_ON   0x00

#define SENSOR_HMAX_UPPER 0x0800
#define SENSOR_HMAX_LOWER 0x0801

#define SENSOR_DEPTH_MAP_NUMBER 0x152C
#define SENSOR_FPS_30		33333333
#define SENSOR_FPS_20		50000000
#define SENSOR_FPS_15		66666666
#define SENSOR_FPS_10		100000000
#define SENSOR_FPS_5		200000000

static const u32 *sensor_imx316_global;
static u32 sensor_imx316_global_size;
static const u32 **sensor_imx316_setfiles;
static const u32 *sensor_imx316_uid;
static u32 sensor_imx316_max_uid_num;
static const u32 *sensor_imx316_setfile_sizes;
static u32 sensor_imx316_max_setfile_num;
static const u32 *sensor_imx316_vcsel_current_setting;
static u32 sensor_imx316_vcsel_current_setting_size;

int sensor_imx316_mode = -1;
u16 sensor_imx316_HMAX;
bool sensor_imx316_enable_i2c_control;
u32 sensor_imx316_frame_duration;

#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
#define FRONT_TX_DEFAULT_FREQ 80

static const struct cam_tof_sensor_mode *sensor_imx316_tx_freq_sensor_mode;
static u32 sensor_imx316_tx_freq_sensor_mode_size;
static const int *sensor_imx316_verify_sensor_mode;
static int sensor_imx316_verify_sensor_mode_size;
u32 sensor_imx316_front_tx_freq = FRONT_TX_DEFAULT_FREQ;

static int sensor_imx316_cis_set_tx_clock(struct v4l2_subdev *subdev)
{
	struct is_cis *cis = NULL;
	struct is_device_sensor *device;
	const struct cam_tof_sensor_mode *cur_tx_sensor_mode;
	int found = -1;

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (device == NULL) {
		err("device is NULL");
		return -1;
	}

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (cis == NULL) {
		err("cis is NULL");
		return -1;
	}

	if (sensor_imx316_mode >= sensor_imx316_tx_freq_sensor_mode_size) {
		err("sensor mode is out of bound");
		return -1;
	}

	cur_tx_sensor_mode = &sensor_imx316_tx_freq_sensor_mode[sensor_imx316_mode];

	if (cur_tx_sensor_mode->mipi_channel_size == 0 ||
		cur_tx_sensor_mode->mipi_channel == NULL) {
		dbg_sensor(1, "skip select mipi channel\n");
		return -1;
	}

	found = is_vendor_select_mipi_by_rf_channel(cur_tx_sensor_mode->mipi_channel,
				cur_tx_sensor_mode->mipi_channel_size);

	if (found != -1) {
		if (found < cur_tx_sensor_mode->sensor_setting_size) {
			sensor_cis_set_registers(subdev,
				cur_tx_sensor_mode->sensor_setting[found].setting,
				cur_tx_sensor_mode->sensor_setting[found].setting_size);
			sensor_imx316_front_tx_freq = cur_tx_sensor_mode->sensor_setting[found].tx_freq;
			info("%s - tx setting %d freq %d\n", __func__, found, sensor_imx316_front_tx_freq);
		} else {
			err("sensor setting size is out of bound");
		}
	}

	return 0;
}
#endif

int sensor_imx316_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 duration);

int sensor_imx316_cis_check_rev(struct is_cis *cis)
{
	int ret = 0;
	u8 rev = 0;

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	info("sensor_imx316_cis_check_rev start ");
	ret = cis->ixc_ops->read8(cis->client, 0x0000, &rev);
	info("sensor_imx316_cis_check_rev 0x0000 %x ", rev);
	ret = cis->ixc_ops->read8(cis->client, 0x0001, &rev);
	info("sensor_imx316_cis_check_rev 0x0001 %x ", rev);
	ret = cis->ixc_ops->read8(cis->client, 0x0002, &rev);
	info("sensor_imx316_cis_check_rev 0x0002 %x ", rev);
	ret = cis->ixc_ops->read8(cis->client, 0x0003, &rev);
	info("sensor_imx316_cis_check_rev 0x0003 %x ", rev);
	ret = cis->ixc_ops->read8(cis->client, 0x0019,&rev);
	info("sensor_imx316_cis_check_rev 0x0019 %x ", rev);
	if (ret < 0) {
		err("is_sensor_read8 fail, (ret %d)", ret);
		ret = -EAGAIN;
	} else {
		cis->cis_data->cis_rev = rev;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

/* CIS OPS */
int sensor_imx316_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
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

	ret = sensor_imx316_cis_check_rev(cis);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
		if (sensor_peri)
			is_sec_get_hw_param(&hw_param, sensor_peri->module->position);
		if (hw_param)
			hw_param->i2c_sensor_err_cnt++;
#endif
		warn("sensor_imx316_check_rev is fail when cis init");
		cis->rev_flag = true;
	}

	cis->cis_data->cur_width = SENSOR_IMX316_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_IMX316_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
	sensor_imx316_enable_i2c_control = false;
	sensor_imx316_frame_duration = 0;
	sensor_imx316_mode = -1;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx316_cis_log_status(struct v4l2_subdev *subdev)
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
	pr_err("[SEN:DUMP] IMX316 *************************\n");
	cis->ixc_ops->read16(cis->client, 0x0001, &data16);
	pr_err("[SEN:DUMP] manufacturer ID(%x)\n", data16);
	cis->ixc_ops->read8(cis->client, SENSOR_DEPTH_MAP_NUMBER, &data8);
	pr_err("[SEN:DUMP] frame counter (%x)\n", data8);
	cis->ixc_ops->read8(cis->client, 0x1001, &data8);
	pr_err("[SEN:DUMP] mode_select(%x)\n", data8);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	pr_err("[SEN:DUMP] *******************************\n");

p_err:
	return ret;
}

int sensor_imx316_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	/* setfile global setting is at camera entrance */
	info("sensor_imx316_cis_set_global_setting");
	ret = sensor_cis_set_registers(subdev, sensor_imx316_global, sensor_imx316_global_size);
	if (ret < 0) {
		err("sensor_imx316_set_registers fail!!");
		goto p_err;
	}

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx316_get_HMAX_value(const u32 *regs, const u32 size)
{
	int i = 0;

	FIMC_BUG(!regs);

	sensor_imx316_HMAX = 0;
	for (i = 0; i < size; i += IXC_NEXT) {
		if (regs[i + IXC_ADDR] == SENSOR_HMAX_UPPER) {
			sensor_imx316_HMAX |= (regs[i + IXC_DATA] << 8);
		} else if (regs[i + IXC_ADDR] == SENSOR_HMAX_LOWER) {
			sensor_imx316_HMAX |= regs[i + IXC_DATA];
			return 0;
		}
	}
	return 0;
}

int sensor_imx316_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_core *core;
	struct is_cis *cis = NULL;
	//int index = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	u32 sensor_uid = 0;
	int i;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (mode > sensor_imx316_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		return ret;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx316_check_rev is fail");
			ret = -EINVAL;
			return ret;
		}
	}

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);

	core = is_get_is_core();

#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
	sensor_imx316_front_tx_freq = FRONT_TX_DEFAULT_FREQ;
#endif

	for (i = 0; i < sensor_imx316_max_uid_num; i++) {
		if (sensor_uid == sensor_imx316_uid[i]) {
			mode = i;
			break;
		}
	}

	sensor_imx316_mode = mode;
	if (i == sensor_imx316_max_uid_num) {
		err("sensor_imx316_cis_mode_change can't find uid set");
		ret = -EINVAL;
		return ret;
	}

	info("sensor_imx316_cis_mode_change pos %d sensor_uid %x mode %d 0x%x",
		sensor_peri->module->position, sensor_uid, mode, sensor_imx316_uid[mode]);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_cis_set_registers(subdev, sensor_imx316_setfiles[mode], sensor_imx316_setfile_sizes[mode]);
	if (ret < 0) {
		err("sensor_imx316_set_registers fail!!");
		goto p_err;
	}

	sensor_imx316_get_HMAX_value(sensor_imx316_setfiles[mode], sensor_imx316_setfile_sizes[mode]);
p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx316_cis_wait_streamoff(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 wait_cnt = 0, time_out_cnt = 250;
	u8 sensor_fcount = 0;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (unlikely(!cis)) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	if (unlikely(!cis_data)) {
		err("cis_data is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	usleep_range(33000, 33000);
	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = cis->ixc_ops->read8(cis->client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	if (ret < 0)
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);

	/*
	 * Read sensor frame counter (sensor_fcount address = 0x0005)
	 * stream on (0x00 ~ 0xFE), stream off (0xFF)
	 */
	while (sensor_fcount != 0xFF) {
		IXC_MUTEX_LOCK(cis->ixc_lock);
		ret = cis->ixc_ops->read8(cis->client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
		IXC_MUTEX_UNLOCK(cis->ixc_lock);

		if (ret < 0)
			err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);

		usleep_range(CIS_STREAM_OFF_WAIT_TIME, CIS_STREAM_OFF_WAIT_TIME);
		wait_cnt++;

		if (wait_cnt >= time_out_cnt) {
			err("[MOD:D:%d] %s, time out, wait_limit(%d) > time_out(%d), sensor_fcount(%d)",
					cis->id, __func__, wait_cnt, time_out_cnt, sensor_fcount);
			ret = -EINVAL;
			goto p_err;
		}

		dbg_sensor(1, "[MOD:D:%d] %s, sensor_fcount(%d), (wait_limit(%d) < time_out(%d))\n",
				cis->id, __func__, sensor_fcount, wait_cnt, time_out_cnt);
	}

p_err:
	return ret;
}

int sensor_imx316_cis_wait_streamon(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 wait_cnt = 0, time_out_cnt = 250;
	u8 sensor_fcount = 0;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (unlikely(!cis)) {
	    err("cis is NULL");
	    ret = -EINVAL;
	    goto p_err;
	}

	cis_data = cis->cis_data;
	if (unlikely(!cis_data)) {
	    err("cis_data is NULL");
	    ret = -EINVAL;
	    goto p_err;
	}

	if (unlikely(!cis->client)) {
	    err("client is NULL");
	    ret = -EINVAL;
	    goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = cis->ixc_ops->read8(cis->client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	if (ret < 0)
	    err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);

	/*
	 * Read sensor frame counter (sensor_fcount address = 0x0005)
	 * stream on (0x00 ~ 0xFE), stream off (0xFF)
	 */
	while (sensor_fcount == 0xff) {
		usleep_range(CIS_STREAM_ON_WAIT_TIME, CIS_STREAM_ON_WAIT_TIME);
		wait_cnt++;

		IXC_MUTEX_LOCK(cis->ixc_lock);
		ret = cis->ixc_ops->read8(cis->client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
		IXC_MUTEX_UNLOCK(cis->ixc_lock);

		if (ret < 0)
		    err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);

		if (wait_cnt >= time_out_cnt) {
			err("[MOD:D:%d] %s, Don't sensor stream on and time out, wait_limit(%d) > time_out(%d), sensor_fcount(%d)",
				cis->id, __func__, wait_cnt, time_out_cnt, sensor_fcount);
			ret = -EINVAL;
			goto p_err;
		}

		dbg_sensor(1, "[MOD:D:%d] %s, sensor_fcount(%d), (wait_limit(%d) < time_out(%d))\n",
				cis->id, __func__, sensor_fcount, wait_cnt, time_out_cnt);
	}

p_err:
	return ret;
}

int sensor_imx316_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_module_enum *module;
	cis_shared_data *cis_data;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct exynos_platform_is_module *pdata;
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
	module = sensor_peri->module;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);
	/* fps setting */
	if (sensor_imx316_frame_duration != 0)
		sensor_imx316_cis_set_frame_duration(subdev, sensor_imx316_frame_duration);

	/* Sensor stream on */
	info("sensor_imx316_cis_stream_on %d", module->position);

	IXC_MUTEX_LOCK(cis->ixc_lock);
#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
	if (module->position == SENSOR_POSITION_FRONT_TOF) {
		sensor_imx316_cis_set_tx_clock(subdev);
	}
#endif
	cis->ixc_ops->write8(cis->client, 0x1001, 0x01);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = true;

	/* VCSEL ON  */
	pdata = module->pdata;
	if (!pdata) {
		clear_bit(IS_MODULE_GPIO_ON, &module->state);
		err("pdata is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!pdata->gpio_cfg) {
		clear_bit(IS_MODULE_GPIO_ON, &module->state);
		err("gpio_cfg is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	usleep_range(200, 200);
	ret = pdata->gpio_cfg(module, SENSOR_SCENARIO_ADDITIONAL_POWER, GPIO_SCENARIO_ON);
	if (ret) {
		clear_bit(IS_MODULE_GPIO_ON, &module->state);
		err("gpio_cfg is fail(%d)", ret);
		goto p_err;
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx316_cis_stream_off(struct v4l2_subdev *subdev)
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
	/* Sensor stream off */
	info("sensor_imx316_cis_stream_off");
	cis->ixc_ops->write8(cis->client, 0x1001, 0x00);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = false;
	sensor_imx316_enable_i2c_control = false;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx316_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	#define AE_MAX 0x1D4C0
	int ret = 0;
	struct is_cis *cis;
	u8 value;
	u32 value_32;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (target_exposure->long_val) {
		info("initial setting is skip");
		goto p_err;
	}

	value_32 = target_exposure->short_val * 120;
	if (value_32 > AE_MAX) {
		value_32 = AE_MAX;
	} else if (value_32 < sensor_imx316_HMAX) {
		value_32 = sensor_imx316_HMAX;
	}

	info("sensor_imx316_cis_set_exposure_time org %d value %x sensor_imx316_HMAX %x",
		target_exposure->short_val, value_32, sensor_imx316_HMAX);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	value = value_32 & 0xFF;
	cis->ixc_ops->write8(cis->client, SENSOR_EXTAREA_INTG_0_A_3, value);
	cis->ixc_ops->write8(cis->client, SENSOR_EXTAREA_INTG_0_B_3, value);
	value = (value_32 & 0xFF00) >> 8;
	cis->ixc_ops->write8(cis->client, SENSOR_EXTAREA_INTG_0_A_2, value);
	cis->ixc_ops->write8(cis->client, SENSOR_EXTAREA_INTG_0_B_2, value);
	value = (value_32 & 0xFF0000) >> 16;
	cis->ixc_ops->write8(cis->client, SENSOR_EXTAREA_INTG_0_A_1, value);
	cis->ixc_ops->write8(cis->client, SENSOR_EXTAREA_INTG_0_B_1, value);
	value = (value_32 & 0xFF000000) >> 24;
	cis->ixc_ops->write8(cis->client, SENSOR_EXTAREA_INTG_0_A_0, value);
	cis->ixc_ops->write8(cis->client, SENSOR_EXTAREA_INTG_0_B_0, value);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx316_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 duration)
{
	int ret = 0;
	struct is_cis *cis;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (duration == 0) {
		err("duration is 0");
		return ret;
	}

	if (sensor_imx316_mode == -1) {
		sensor_imx316_frame_duration = duration;
		err("need sensor setting (%d)", duration);
		ret = -EINVAL;
		goto p_err;
	}

	sensor_imx316_frame_duration = duration;
	IXC_MUTEX_LOCK(cis->ixc_lock);
	cis->ixc_ops->write8(cis->client, 0x0102, 0x01);

	info("%s : set %d", __func__, duration);
	switch (duration) {
		case SENSOR_FPS_5:
			cis->ixc_ops->write8(cis->client, 0x2154, 0x36);
			cis->ixc_ops->write8(cis->client, 0x2155, 0x3C);
			break;
		case SENSOR_FPS_10:
			cis->ixc_ops->write8(cis->client, 0x2154, 0x17);
			cis->ixc_ops->write8(cis->client, 0x2155, 0xBC);
			break;
		case SENSOR_FPS_15:
			cis->ixc_ops->write8(cis->client, 0x2154, 0x0D);
			cis->ixc_ops->write8(cis->client, 0x2155, 0x92);
			break;
		case SENSOR_FPS_20:
			cis->ixc_ops->write8(cis->client, 0x2154, 0x08);
			cis->ixc_ops->write8(cis->client, 0x2155, 0x7E);
			break;
		case SENSOR_FPS_30:
			cis->ixc_ops->write8(cis->client, 0x2154, 0x03);
			cis->ixc_ops->write8(cis->client, 0x2155, 0x68);
			break;
		default:
			err("Unsupported fps : %d", duration);
			break;
	}

	cis->ixc_ops->write8(cis->client, 0x0102, 0x00);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx316_cis_set_laser_control(struct v4l2_subdev *subdev, u32 onoff)
{
	int ret = 0;
	struct is_cis *cis;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	info("sensor_imx316_cis_set_laser_control %d", onoff);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	if (onoff) {
		cis->ixc_ops->write8(cis->client, SENSOR_LASER_CONTROL_REG_1, SENSOR_LASER_ON);
		cis->ixc_ops->write8(cis->client, SENSOR_LASER_CONTROL_REG_2, SENSOR_LASER_ON);
	} else {
		cis->ixc_ops->write8(cis->client, SENSOR_LASER_CONTROL_REG_1, SENSOR_LASER_OFF);
		cis->ixc_ops->write8(cis->client, SENSOR_LASER_CONTROL_REG_2, SENSOR_LASER_OFF);
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_imx316_cis_set_vcsel_current(struct v4l2_subdev *subdev, u32 value)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	struct is_module_enum *module;
	struct is_device_sensor_peri *sensor_peri = NULL;
	u8 value8 = (u8)value;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);
	module = sensor_peri->module;

	cis_data = cis->cis_data;
	if (cis_data->stream_on == false) {
		err("sensor_imx316_cis_set_vcsel_current fail!! sensor is off ");
		ret = -EINVAL;
		return ret;
	}

	info("sensor_imx316_cis_set_vcsel_current %d", value);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	if (sensor_imx316_enable_i2c_control == false) {
		ret = sensor_cis_set_registers(subdev, sensor_imx316_vcsel_current_setting, sensor_imx316_vcsel_current_setting_size);
		if (ret < 0) {
			err("sensor_imx316_cis_set_vcsel_current fail!!");
			goto p_err;
		}
		sensor_imx316_enable_i2c_control = true;
	}

	cis->ixc_ops->write8(cis->client, 0x0436, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0437, 0x00);
	cis->ixc_ops->write8(cis->client, 0x043A, 0x02);
	cis->ixc_ops->write8(cis->client, 0x0500, 0x02);
	cis->ixc_ops->write8(cis->client, 0x0501, 0x08);
	cis->ixc_ops->write8(cis->client, 0x0502, value8);

	cis->ixc_ops->write8(cis->client, 0x0431, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0430, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0431, 0x00);

	if (module->position == SENSOR_POSITION_REAR_TOF) {  /* for setting IPD_OFFSET */
		cis->ixc_ops->write8(cis->client, 0x0436, 0x00);
		cis->ixc_ops->write8(cis->client, 0x0437, 0x00);
		cis->ixc_ops->write8(cis->client, 0x043A, 0x02);
		cis->ixc_ops->write8(cis->client, 0x0500, 0x02);
		cis->ixc_ops->write8(cis->client, 0x0501, 0x0D);
		cis->ixc_ops->write8(cis->client, 0x0502, 0xC3);
		cis->ixc_ops->write8(cis->client, 0x0431, 0x01);
		cis->ixc_ops->write8(cis->client, 0x0430, 0x01);
		cis->ixc_ops->write8(cis->client, 0x0431, 0x00);
	}

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx316_cis_get_vcsel_photo_diode(struct v4l2_subdev *subdev, u16 *value)
{
	int ret = 0;
	struct is_cis *cis;
	u8 value8_1 = 0, value8_0 = 0;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!sensor_imx316_enable_i2c_control) {
		ret = -EINVAL;
		err("sensor_imx316_cis_get_vcsel_photo_diode not available!!");
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	cis->ixc_ops->write8(cis->client, 0x0421, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0436, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0437, 0x00);
	cis->ixc_ops->write8(cis->client, 0x043A, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0500, 0x11);
	cis->ixc_ops->write8(cis->client, 0x0501, 0x90);
	cis->ixc_ops->write8(cis->client, 0x0431, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0430, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0431, 0x00);

	cis->ixc_ops->read8(cis->client, 0x058B, &value8_1); /*0x058B for APC2_CHECK_DATA[9:8]*/

	cis->ixc_ops->write8(cis->client, 0x0423, 0xA0);
	cis->ixc_ops->write8(cis->client, 0x0436, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0437, 0x00);
	cis->ixc_ops->write8(cis->client, 0x043A, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0500, 0x07);
	cis->ixc_ops->write8(cis->client, 0x0501, 0xA0);
	cis->ixc_ops->write8(cis->client, 0x0431, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0430, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0431, 0x00);
	cis->ixc_ops->read8(cis->client, 0x0582, &value8_0); /* 0x0582 for APC2_CHECK_DATA[7:0] */

	*value = (((value8_1 & 0x30) << 4) | value8_0);
	info("sensor_imx316_cis_get_vcsel_photo_diode value8_1 0x%x value8_0 0x%x => (0x%x)%d", value8_1, value8_0, *value, *value);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
int sensor_imx316_cis_get_tx_freq(struct v4l2_subdev *subdev, u32 *value)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_module_enum *module;
	struct is_device_sensor_peri *sensor_peri = NULL;
	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);
	module = sensor_peri->module;

	if (module->position == SENSOR_POSITION_FRONT_TOF) {
		info("%s front tx freq %d", __func__, sensor_imx316_front_tx_freq);
		*value = sensor_imx316_front_tx_freq;
	} else {
		info("%s rear tx freq none ", __func__);
	}

	return ret;
}
#endif

static struct is_cis_ops cis_ops_imx316 = {
	.cis_init = sensor_imx316_cis_init,
	.cis_log_status = sensor_imx316_cis_log_status,
	.cis_set_global_setting = sensor_imx316_cis_set_global_setting,
	.cis_mode_change = sensor_imx316_cis_mode_change,
	.cis_stream_on = sensor_imx316_cis_stream_on,
	.cis_stream_off = sensor_imx316_cis_stream_off,
	.cis_set_exposure_time = sensor_imx316_cis_set_exposure_time,
	.cis_wait_streamoff = sensor_imx316_cis_wait_streamoff,
	.cis_wait_streamon = sensor_imx316_cis_wait_streamon,
	.cis_set_laser_control = sensor_imx316_cis_set_laser_control,
	.cis_set_laser_current = sensor_imx316_cis_set_vcsel_current,
	.cis_get_laser_photo_diode = sensor_imx316_cis_get_vcsel_photo_diode,
	.cis_set_frame_duration = sensor_imx316_cis_set_frame_duration,
#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
	.cis_get_tof_tx_freq = sensor_imx316_cis_get_tx_freq,
#endif
};

static int cis_imx316_probe_i2c(struct i2c_client *client,
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
	cis->cis_ops = &cis_ops_imx316;
	/* belows are depend on sensor cis. MUST check sensor spec */
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;

	if (of_property_read_string(dnode, "setfile", &setfile)) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0) { /* KOR */
		probe_info("%s setfile_A\n", __func__);
		sensor_imx316_global = sensor_imx316_setfile_A_Global;
		sensor_imx316_global_size = ARRAY_SIZE(sensor_imx316_setfile_A_Global);
		sensor_imx316_setfiles = sensor_imx316_setfiles_A;
		sensor_imx316_uid = sensor_imx316_setfiles_A_uid;
		sensor_imx316_max_uid_num = ARRAY_SIZE(sensor_imx316_setfiles_A_uid);
		sensor_imx316_setfile_sizes = sensor_imx316_setfile_A_sizes;
		sensor_imx316_max_setfile_num = ARRAY_SIZE(sensor_imx316_setfiles_A);
		sensor_imx316_vcsel_current_setting = sensor_imx316_vcsel_current_setting_A;
		sensor_imx316_vcsel_current_setting_size = sensor_imx316_vcsel_current_setting_A_size;
#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
		sensor_imx316_verify_sensor_mode = sensor_imx316_setfile_A_verify_sensor_mode;
		sensor_imx316_verify_sensor_mode_size = ARRAY_SIZE(sensor_imx316_setfile_A_verify_sensor_mode);
#endif
	}
	else if (strcmp(setfile, "setB") == 0) { /* EUR OPEN */
		probe_info("%s setfile_B\n", __func__);
		sensor_imx316_global = sensor_imx316_setfile_B_Global;
		sensor_imx316_global_size = ARRAY_SIZE(sensor_imx316_setfile_B_Global);
		sensor_imx316_setfiles = sensor_imx316_setfiles_B;
		sensor_imx316_uid = sensor_imx316_setfiles_B_uid;
		sensor_imx316_max_uid_num = ARRAY_SIZE(sensor_imx316_setfiles_B_uid);
		sensor_imx316_setfile_sizes = sensor_imx316_setfile_B_sizes;
		sensor_imx316_max_setfile_num = ARRAY_SIZE(sensor_imx316_setfiles_B);
		sensor_imx316_vcsel_current_setting = sensor_imx316_vcsel_current_setting_B;
		sensor_imx316_vcsel_current_setting_size = sensor_imx316_vcsel_current_setting_B_size;
#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
		sensor_imx316_verify_sensor_mode = sensor_imx316_setfile_B_verify_sensor_mode;
		sensor_imx316_verify_sensor_mode_size = ARRAY_SIZE(sensor_imx316_setfile_B_verify_sensor_mode);
#endif

	}
	else {
		err("%s setfile index out of bound, take default (setfile_A)", __func__);
		probe_info("%s setfile_A\n", __func__);
		sensor_imx316_global = sensor_imx316_setfile_A_Global;
		sensor_imx316_global_size = ARRAY_SIZE(sensor_imx316_setfile_A_Global);
		sensor_imx316_setfiles = sensor_imx316_setfiles_A;
		sensor_imx316_uid = sensor_imx316_setfiles_A_uid;
		sensor_imx316_max_uid_num = ARRAY_SIZE(sensor_imx316_setfiles_A_uid);
		sensor_imx316_setfile_sizes = sensor_imx316_setfile_A_sizes;
		sensor_imx316_max_setfile_num = ARRAY_SIZE(sensor_imx316_setfiles_A);
		sensor_imx316_vcsel_current_setting = sensor_imx316_vcsel_current_setting_A;
		sensor_imx316_vcsel_current_setting_size = sensor_imx316_vcsel_current_setting_A_size;
#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
		sensor_imx316_verify_sensor_mode = sensor_imx316_setfile_A_verify_sensor_mode;
		sensor_imx316_verify_sensor_mode_size = ARRAY_SIZE(sensor_imx316_setfile_A_verify_sensor_mode);
#endif
	}

#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
	{
		int i = 0, j = 0, index = 0;
		for (i = 0; i < sensor_imx316_verify_sensor_mode_size; i++) {
			for (j = 0; j < sensor_imx316_max_uid_num; j++) {
				if (sensor_imx316_verify_sensor_mode[i] == sensor_imx316_uid[j]) {
					index = j;
					break;
				}
			}
		}
	}
#endif
	probe_info("%s done\n", __func__);

	return ret;
}

static const struct of_device_id sensor_cis_imx316_match[] = {
	{
		.compatible = "samsung,exynos-is-cis-imx316",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx316_match);

static const struct i2c_device_id sensor_cis_imx316_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx316_driver = {
	.probe	= cis_imx316_probe_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx316_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx316_idt
};

static int __init sensor_cis_imx316_init(void)
{
	int ret;

	probe_info("sensor_cis_imx316_init");
	ret = i2c_add_driver(&sensor_cis_imx316_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_imx316_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx316_init);

MODULE_LICENSE("GPL");
