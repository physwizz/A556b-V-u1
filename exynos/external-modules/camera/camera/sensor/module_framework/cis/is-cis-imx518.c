/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
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
#include "is-cis-imx518.h"
#include "is-cis-imx518-setA.h"
#include "is-vendor-private.h"
#include "is-helper-ixc.h"

#define SENSOR_NAME "IMX518"

#define SENSOR_EXTAREA_INTG_0_A_0 0x2124
#define SENSOR_EXTAREA_INTG_0_A_1 0x2125
#define SENSOR_EXTAREA_INTG_0_A_2 0x2126
#define SENSOR_EXTAREA_INTG_0_A_3 0x2127

#define SENSOR_EXTAREA_INTG_0_B_0 0x2128
#define SENSOR_EXTAREA_INTG_0_B_1 0x2129
#define SENSOR_EXTAREA_INTG_0_B_2 0x212A
#define SENSOR_EXTAREA_INTG_0_B_3 0x212B

#define SENSOR_LASER_CONTROL_REG_1 0x2144
#define SENSOR_LASER_CONTROL_REG_2 0x2145
#define SENSOR_LASER_OFF  0xAA
#define SENSOR_LASER_ON   0x00

#define SENSOR_HMAX_UPPER 0x0800
#define SENSOR_HMAX_LOWER 0x0801

#define SENSOR_DEPTH_MAP_NUMBER 0x151C
#define SENSOR_FPS_30		33333333
#define SENSOR_FPS_24		41666666
#define SENSOR_FPS_20		50000000
#define SENSOR_FPS_15		66666666
#define SENSOR_FPS_10		100000000
#define SENSOR_FPS_5		200000000
#define SENOSR_FPS_REGISTER_1	0x2112
#define SENOSR_FPS_REGISTER_2	0x2113

static const u32 *sensor_imx518_global;
static u32 sensor_imx518_global_size;
static const u32 ***sensor_imx518_setfiles;
static const u32 **sensor_imx518_laser_setting;
static const u32 *sensor_imx518_mode_id;
static u32 sensor_imx518_max_mode_id_num;
static const u32 **sensor_imx518_setfile_sizes;
static const u32 *sensor_imx518_laser_setting_sizes;
static const u16 ***sensor_imx518_setfiles_fps;

int sensor_imx518_mode = -1;
u16 sensor_imx518_HMAX;
u32 sensor_imx518_frame_duration;
int sensor_imx518_laser_mode = LASER_MODE_NORMAL;

#ifndef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
#define REAR_TX_DEFAULT_FREQ 100 /* MHz */
#else
/* because of solution pre-set, initial value need to be 0*/
#define REAR_TX_DEFAULT_FREQ 0 /* MHz */
static const struct cam_tof_sensor_mode *sensor_imx518_tx_freq_sensor_mode;
u32 sensor_imx518_rear_tx_freq = REAR_TX_DEFAULT_FREQ;
u32 sensor_imx518_rear_tx_freq_fixed_index = -1;

int sensor_imx518_cis_get_mode_id_index(struct is_cis *cis, u32 mode, u32 *mode_id_index);

static int sensor_imx518_cis_set_tx_clock_init(struct v4l2_subdev *subdev)
{
	struct is_cis *cis = NULL;
	const struct cam_tof_sensor_mode *cur_tx_sensor_mode;
	int found = -1;

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (cis == NULL) {
		err("cis is NULL");
		return -1;
	}

	cur_tx_sensor_mode = &sensor_imx518_tx_freq_sensor_mode[0];

	if (cur_tx_sensor_mode->mipi_channel_size == 0 ||
		cur_tx_sensor_mode->mipi_channel == NULL) {
		dbg_sensor(1, "skip select mipi channel\n");
		return -1;
	}

	if(sensor_imx518_rear_tx_freq_fixed_index != -1) {
		found = sensor_imx518_rear_tx_freq_fixed_index;
	} else {
		found = is_vendor_select_mipi_by_rf_channel(cur_tx_sensor_mode->mipi_channel,
					cur_tx_sensor_mode->mipi_channel_size);
	}

	if (found != -1) {
		if (found < cur_tx_sensor_mode->sensor_setting_size) {
			sensor_imx518_rear_tx_freq = sensor_imx518_supported_tx_freq[found];
			if(sensor_imx518_rear_tx_freq_fixed_index != -1)
				info("%s - tx setting fixed to %d\n", __func__, sensor_imx518_rear_tx_freq);
			else
				info("%s - tx setting %d freq %d\n", __func__, found, sensor_imx518_rear_tx_freq);
		} else {
			err("sensor setting size is out of bound");
		}
	}

	return 0;
}

static int sensor_imx518_cis_set_tx_clock_apply(struct v4l2_subdev *subdev)
{
	const struct cam_tof_sensor_mode *cur_tx_sensor_mode;
	int i, found = -1;

	cur_tx_sensor_mode = &sensor_imx518_tx_freq_sensor_mode[sensor_imx518_mode];

	if (cur_tx_sensor_mode->mipi_channel_size == 0 ||
		cur_tx_sensor_mode->mipi_channel == NULL) {
		dbg_sensor(1, "skip select mipi channel\n");
		return -1;
	}

	for(i = 0; i < ARRAY_SIZE(sensor_imx518_supported_tx_freq); i++){
		if (sensor_imx518_rear_tx_freq == sensor_imx518_supported_tx_freq[i]){
			found = i;
			break;
		}
	}

	if (found != -1) {
		if (found < cur_tx_sensor_mode->sensor_setting_size) {
			sensor_cis_set_registers(subdev,
				cur_tx_sensor_mode->sensor_setting[found].setting,
				cur_tx_sensor_mode->sensor_setting[found].setting_size);
			if(sensor_imx518_rear_tx_freq_fixed_index != -1)
				info("%s - tx setting fixed to %d\n", __func__, sensor_imx518_rear_tx_freq);
			else
				info("%s - tx setting %d freq %d\n", __func__, found, sensor_imx518_rear_tx_freq);
		} else {
			err("sensor setting size is out of bound");
		}
	}

	return 0;
}
#endif

int sensor_imx518_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 duration);

int sensor_imx518_cis_get_mode_id_index(struct is_cis *cis, u32 mode, u32 *mode_id_index)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	u32 sensor_mode_id = 0;
	int i = 0;

	if (mode >= SENSOR_IMX518_MODE_MAX || mode == -1) {
		err("sensor mode is out of bound (%d)", mode);
		return -1;
	}

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);

	for (i = 0; i < sensor_imx518_max_mode_id_num; i++) {
		if (sensor_mode_id == sensor_imx518_mode_id[i]) {
			*mode_id_index = i;
			break;
		}
	}

	if (i == sensor_imx518_max_mode_id_num) {
		err("sensor_imx518_cis_mode_change can't find mode_id(mode_id : 0x%x) mode (%d)", sensor_mode_id, mode);
		return -1;
	}

	return 0;
}

int sensor_imx518_cis_check_rev(struct is_cis *cis)
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

	ret = cis->ixc_ops->read8(cis->client, 0x0019,&rev);
	if (ret < 0)
		ret = cis->ixc_ops->read8(cis->client, 0x0019,&rev);
	info("sensor_imx518_cis_check_rev 0x0019 %x ", rev);
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
int sensor_imx518_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_device_sensor_peri *sensor_peri = NULL;
	cis_setting_info setinfo;
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
#endif
	ktime_t st = ktime_get();
#ifdef TOF_MODULE_CHECK_ID
	struct is_core *core;
	struct is_vendor_private *vendor_priv;
	u32 sensor_mode_id;

	core = (struct is_core *)dev_get_drvdata(is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		return -1;
	}
#endif

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

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);

#ifdef TOF_MODULE_CHECK_ID
	vendor_priv = core->vendor.private_data;
	if (sensor_peri->module->position == SENSOR_POSITION_REAR_TOF) {
		sensor_mode_id = vendor_priv->rear_tof_mode_id;
	} else {  // SENSOR_POSITION_FRONT_TOF
		sensor_mode_id = vendor_priv->front_tof_mode_id;
	}

	if (sensor_mode_id != TOF_MODULE_CHECK_ID) {
		err("invalid module!(0x%X)", sensor_mode_id);
		return -1;
	}
#endif

	ret = sensor_imx518_cis_check_rev(cis);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		if (sensor_peri)
			is_sec_get_hw_param(&hw_param, sensor_peri->module->position);
		if (hw_param)
			hw_param->i2c_sensor_err_cnt++;
#endif
		warn("sensor_imx518_check_rev is fail when cis init");
		cis->rev_flag = true;
	}

	cis->cis_data->cur_width = SENSOR_IMX518_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_IMX518_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
	sensor_imx518_frame_duration = 0;
	sensor_imx518_laser_mode = LASER_MODE_NORMAL;
	sensor_imx518_mode = -1;

#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
	sensor_imx518_rear_tx_freq = REAR_TX_DEFAULT_FREQ;
	sensor_imx518_cis_set_tx_clock_init(subdev);
#endif
	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx518_cis_log_status(struct v4l2_subdev *subdev)
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
	pr_err("[SEN:DUMP] IMX518 *************************\n");
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

int sensor_imx518_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	/* setfile global setting is at camera entrance */
	info("sensor_imx518_cis_set_global_setting");
	ret = sensor_cis_set_registers(subdev, sensor_imx518_global, sensor_imx518_global_size);
	if (ret < 0) {
		err("sensor_imx518_set_registers fail!!");
		goto p_err;
	}

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx518_get_HMAX_value(const u32 *regs, const u32 size)
{
	int i = 0;

	FIMC_BUG(!regs);

	sensor_imx518_HMAX = 0;
	for (i = 0; i < size; i += IXC_NEXT) {
		if (regs[i + IXC_ADDR] == SENSOR_HMAX_UPPER) {
			sensor_imx518_HMAX |= (regs[i + IXC_DATA] << 8);
		} else if (regs[i + IXC_ADDR] == SENSOR_HMAX_LOWER) {
			sensor_imx518_HMAX |= regs[i + IXC_DATA];
			return 0;
		}
	}
	return 0;
}

int sensor_imx518_cis_set_laser_control(struct v4l2_subdev *subdev, u32 onoff);
int sensor_imx518_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	struct is_device_sensor_peri *sensor_peri = NULL;
	u32 sensor_mode_id = 0;
	u32 mode_id_index = 0;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (mode >= SENSOR_IMX518_MODE_MAX) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		return ret;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx518_check_rev is fail");
			return ret;
		}
	}

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);

	ret = sensor_imx518_cis_get_mode_id_index(cis, mode, &mode_id_index);
	if (ret < 0) {
		return ret;
	}

	info("sensor_imx518_cis_mode_change pos %d sensor_mode_id %x mode %d index %d 0x%x",
		sensor_peri->module->position, sensor_mode_id, mode, mode_id_index, sensor_imx518_mode_id[mode_id_index]);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_cis_set_registers(subdev, sensor_imx518_setfiles[mode_id_index][mode], sensor_imx518_setfile_sizes[mode_id_index][mode]);
	if (ret < 0) {
		err("sensor_imx518_set_registers fail!!");
		goto p_err;
	}
	/* laser setting */
	ret = sensor_cis_set_registers(subdev, sensor_imx518_laser_setting[mode_id_index], sensor_imx518_laser_setting_sizes[mode_id_index]);

	if (mode == SENSOR_IMX518_320x480_30FPS) {	/* PD read disble in AF mode */
		ret |= cis->ixc_ops->write8(cis->client, 0x0403, 0x20);	/* Send1_ctrl_devsel */
		ret |= cis->ixc_ops->write8(cis->client, 0x0405, 0x00);	/* Send1_divide */
		ret |= cis->ixc_ops->write8(cis->client, 0x0407, 0x00);	/* Send1_tx_buf_no */
		ret |= cis->ixc_ops->write8(cis->client, 0x0500, 0x02);
		ret |= cis->ixc_ops->write8(cis->client, 0x0501, 0x00);	/* Address of RST_DIS, APC_XEN */
		ret |= cis->ixc_ops->write8(cis->client, 0x0502, 0x1C);	/* To disable APC_check pulse */
		ret |= cis->ixc_ops->write8(cis->client, 0x0401, 0x01);	/* Send1_enable */
		ret |= cis->ixc_ops->write8(cis->client, 0x0400, 0x01);	/* Send1_Trigger */
		ret |= cis->ixc_ops->write8(cis->client, 0x0401, 0x00);	/* Send1_disable */
	}

	if (ret < 0) {
		err("sensor_imx518_set_registers fail!!");
		goto p_err;
	}

	sensor_imx518_get_HMAX_value(sensor_imx518_setfiles[mode_id_index][mode], sensor_imx518_setfile_sizes[mode_id_index][mode]);
	sensor_imx518_mode = mode;

#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
	sensor_imx518_cis_set_tx_clock_apply(subdev);
#endif
p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	if (sensor_imx518_laser_mode == LASER_MODE_LIVE_FOCUS) {
		info("%s - Laser off at LiveFocus entrance",__func__);
		sensor_imx518_cis_set_laser_control(subdev, false);
	}
	return ret;
}

int sensor_imx518_cis_wait_streamoff(struct v4l2_subdev *subdev)
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

	if (ret < 0) {
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * stream on (0x00 ~ 0xFE), stream off (0xFF)
	 */
	while (sensor_fcount != 0xFF) {
		IXC_MUTEX_LOCK(cis->ixc_lock);
		ret = cis->ixc_ops->read8(cis->client, SENSOR_DEPTH_MAP_NUMBER, &sensor_fcount);
		IXC_MUTEX_UNLOCK(cis->ixc_lock);

		if (ret < 0) {
			err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);
			ret = -EINVAL;
			goto p_err;
		}

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

int sensor_imx518_cis_wait_streamon(struct v4l2_subdev *subdev)
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
	if (ret < 0) {
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);
		ret = -EINVAL;
		goto p_err;
	}

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

		if (ret < 0) {
			err("i2c transfer fail addr(%x), val(%x), ret = %d\n", SENSOR_DEPTH_MAP_NUMBER, sensor_fcount, ret);
			ret = -EINVAL;
			goto p_err;
		}

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

int sensor_imx518_cis_stream_on(struct v4l2_subdev *subdev)
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
	if (sensor_imx518_frame_duration != 0)
		sensor_imx518_cis_set_frame_rate(subdev, sensor_imx518_frame_duration);

	/* Sensor stream on */
	info("sensor_imx518_cis_stream_on %d", module->position);

	IXC_MUTEX_LOCK(cis->ixc_lock);
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

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx518_cis_stream_off(struct v4l2_subdev *subdev)
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
	info("sensor_imx518_cis_stream_off");
	cis->ixc_ops->write8(cis->client, 0x1001, 0x00);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = false;

	if (sensor_imx518_laser_mode == LASER_MODE_LIVE_FOCUS) {
		sensor_imx518_cis_set_laser_control(subdev, true);
		sensor_imx518_laser_mode = LASER_MODE_NORMAL;
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx518_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
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

	if ((int)(target_exposure->short_val) <= 0) {
		info("%s initial setting is skip, shor_val(%d)", __func__, target_exposure->short_val);
		goto p_err;
	}

	value_32 = target_exposure->short_val * 120;
	if (value_32 > AE_MAX) {
		value_32 = AE_MAX;
	} else if (value_32 < sensor_imx518_HMAX) {
		value_32 = sensor_imx518_HMAX;
	}

	info("sensor_imx518_cis_set_exposure_time org %d value %x sensor_imx518_HMAX %x",
		target_exposure->short_val, value_32, sensor_imx518_HMAX);

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

int sensor_imx518_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 duration)
{
	int ret = 0;
	struct is_cis *cis;
	u16 cur_fps = 0;
	const u16 *fps_list = NULL;
	u8 fps = 0;
	u32 mode_id_index = 0;

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

	if (sensor_imx518_mode == -1) {
		sensor_imx518_frame_duration = duration;
		warn("need sensor setting (%d)", duration);
		goto p_err;
	} else if (sensor_imx518_mode == SENSOR_IMX518_320x480_30FPS) {
		info("%s - not support fps ctrl in AF mode", __func__);
		goto p_err;
	}

	sensor_imx518_frame_duration = duration;
	duration = (1000 * 1000 * 1000) / duration;

	ret = sensor_imx518_cis_get_mode_id_index(cis, sensor_imx518_mode, &mode_id_index);
	if (ret < 0) {
		return ret;
	}

	fps_list = sensor_imx518_setfiles_fps[mode_id_index][sensor_imx518_mode];
	IXC_MUTEX_LOCK(cis->ixc_lock);
	cis->ixc_ops->write8(cis->client, 0x0102, 0x01);

	info("%s : set %d", __func__, duration);
	switch (duration) {
		case SENSOR_FPS_5:
			cur_fps = fps_list[5];
			fps = cur_fps & 0xFF;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_10:
			cur_fps = fps_list[4];
			fps = cur_fps & 0xFF;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_15:
			cur_fps = fps_list[3];
			fps = cur_fps & 0xFF;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_20:
			cur_fps = fps_list[2];
			fps = cur_fps & 0xFF;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_24:
			cur_fps = fps_list[1];
			fps = cur_fps & 0xFF;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_1, fps);
			break;
		case SENSOR_FPS_30:
			cur_fps = fps_list[0];
			fps = cur_fps & 0xFF;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_2, fps);
			fps = cur_fps >> 8;
			cis->ixc_ops->write8(cis->client, SENOSR_FPS_REGISTER_1, fps);
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

int sensor_imx518_cis_set_laser_control(struct v4l2_subdev *subdev, u32 onoff)
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

	info("sensor_imx518_cis_set_laser_control %d", onoff);

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

int sensor_imx518_cis_set_laser_mode(struct v4l2_subdev *subdev, u32 mode)
{
	sensor_imx518_laser_mode = mode;
	return 0;
}

int sensor_imx518_cis_set_vcsel_current(struct v4l2_subdev *subdev, u32 value)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	struct is_module_enum *module;
	struct is_device_sensor_peri *sensor_peri = NULL;
	u8 value8 = (u8)value;
	u8 err_flag[2] = { 0 };

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);
	module = sensor_peri->module;

	cis_data = cis->cis_data;
	if (cis_data->stream_on == false) {
		err("sensor_imx518_cis_set_vcsel_current fail!! sensor is off ");
		goto p_err;
	}

	info("sensor_imx518_cis_set_vcsel_current %d", value);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	/* Current test cause OC_EM error, check error flag before set current */
	cis->ixc_ops->read8(cis->client, 0x0590, &err_flag[0]);	/* 0x0590 for CSA4026 error flag(0x27) */
	cis->ixc_ops->read8(cis->client, 0x0591, &err_flag[1]);	/* 0x0591 for CSA4026 error flag(0x28) */

	/* for setting IPD_OFFSET */
	/* DIF_ERR_SEL set */
	cis->ixc_ops->write8(cis->client, 0x0403, 0x20);
	cis->ixc_ops->write8(cis->client, 0x0405, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0407, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0500, 0x02);
	cis->ixc_ops->write8(cis->client, 0x0501, 0x0f);
	cis->ixc_ops->write8(cis->client, 0x0502, 0xC4);
	cis->ixc_ops->write8(cis->client, 0x0401, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0400, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0401, 0x00);

	/* release DIFF2 Error Mask */
	cis->ixc_ops->write8(cis->client, 0x0403, 0x20);
	cis->ixc_ops->write8(cis->client, 0x0405, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0407, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0500, 0x02);
	cis->ixc_ops->write8(cis->client, 0x0501, 0x14);
	cis->ixc_ops->write8(cis->client, 0x0502, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0401, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0400, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0401, 0x00);

	/* for setting VCSEL current */
	cis->ixc_ops->write8(cis->client, 0x0403, 0x20);
	cis->ixc_ops->write8(cis->client, 0x0405, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0407, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0500, 0x02);
	cis->ixc_ops->write8(cis->client, 0x0501, 0x08);
	cis->ixc_ops->write8(cis->client, 0x0502, value8);
	cis->ixc_ops->write8(cis->client, 0x0401, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0400, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0401, 0x00);

	/* Wait 70ms(2 frame) */
	usleep_range(70000, 70000);

	/* for setting VCSEL current */
	cis->ixc_ops->write8(cis->client, 0x0403, 0x20);
	cis->ixc_ops->write8(cis->client, 0x0405, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0407, 0x00);
	cis->ixc_ops->write8(cis->client, 0x0500, 0x02);
	cis->ixc_ops->write8(cis->client, 0x0501, 0x08);
	cis->ixc_ops->write8(cis->client, 0x0502, value8);
	cis->ixc_ops->write8(cis->client, 0x0401, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0400, 0x01);
	cis->ixc_ops->write8(cis->client, 0x0401, 0x00);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	usleep_range(5000, 5000);

	info("%s- TX STATUS: 0x0590(0x27)(%x) 0x0591(0x27)(%x)", __func__, err_flag[0], err_flag[1]);
p_err:
	return ret;
}

int sensor_imx518_cis_set_laser_error(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis;
	int i=0;
	struct sensor_imx518_laser_test test;

	FIMC_BUG(!subdev);
	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->ixc_lock);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	for(i=0; i < sizeof(sensor_imx518_laser_testcases)/sizeof(sensor_imx518_laser_testcases[0]); i++){
		test = sensor_imx518_laser_testcases[i];

		if(test.error == mode){
			IXC_MUTEX_LOCK(cis->ixc_lock);

			cis->ixc_ops->write8(cis->client, 0x0403, 0x20);
			cis->ixc_ops->write8(cis->client, 0x0405, 0x00);
			cis->ixc_ops->write8(cis->client, 0x0407, 0x00);		/*send buf CXA4026 config */
			cis->ixc_ops->write8(cis->client, 0x0500, 0x02);
			cis->ixc_ops->write8(cis->client, 0x0501, test.addr);		/* CXA4026 addr */
			cis->ixc_ops->write8(cis->client, 0x0502, test.data);		/* Data to send */
			cis->ixc_ops->write8(cis->client, 0x0401, 0x01);		/* Send start*/
			cis->ixc_ops->write8(cis->client, 0x0400, 0x01);
			cis->ixc_ops->write8(cis->client, 0x0401, 0x00);		/* Send end */

			IXC_MUTEX_UNLOCK(cis->ixc_lock);

			info("%s - Set err: %d (0x%x):0x%x", __func__, test.error, test.addr, test.data);
		}
	}
p_err:
	return ret;
}

int sensor_imx518_cis_get_laser_error_flag(struct v4l2_subdev *subdev, int *value)
{
	int ret = 0;
	struct is_cis *cis;
	u8 err_flag[2] = { 0 };

	FIMC_BUG(!subdev);
	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->ixc_lock);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	cis->ixc_ops->read8(cis->client, 0x0590, &err_flag[0]);	/* 0x0590 for CSA4026 error flag(0x27) */
	cis->ixc_ops->read8(cis->client, 0x0591, &err_flag[1]);	/* 0x0591 for CSA4026 error flag(0x28) */

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	*value = (((int)err_flag[0] << 6) | err_flag[1]);
	info("%s - err : %x (err[0] : %x, err[1] : %x)", __func__, *value, err_flag[0], err_flag[1]);

p_err:
	return ret;
}

int sensor_imx518_cis_laser_error_req(struct v4l2_subdev *subdev, u32 mode, int *value){
	int ret = 0;
	if(mode)
		ret =sensor_imx518_cis_set_laser_error(subdev, mode);
	else
		ret = sensor_imx518_cis_get_laser_error_flag(subdev, value);
	return ret;
}

int sensor_imx518_cis_get_vcsel_photo_diode(struct v4l2_subdev *subdev, u16 *value)
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

	IXC_MUTEX_LOCK(cis->ixc_lock);

	cis->ixc_ops->read8(cis->client, 0x0588, &value8_1); 	/*0x058B for APC2_CHECK_DATA[9:8]*/
	cis->ixc_ops->read8(cis->client, 0x058F, &value8_0); 	/* 0x0582 for APC2_CHECK_DATA[7:0] */

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	*value = (((value8_1 & 0x30) << 4) | value8_0);
	info("%s - value8_1 0x%x value8_0 0x%x => (0x%x)%d", __func__, value8_1, value8_0, *value, *value);

p_err:
	return ret;
}

#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
int sensor_imx518_cis_set_tx_freq(struct v4l2_subdev *subdev, u32 value)
{
	int i=0;
	int freq_size = ARRAY_SIZE(sensor_imx518_supported_tx_freq);

	sensor_imx518_rear_tx_freq_fixed_index = -1;
	for(i=0;i < freq_size;i++){
		if(value == sensor_imx518_supported_tx_freq[i]) {
			sensor_imx518_rear_tx_freq_fixed_index = i;
			break;
		}
	}
	if(sensor_imx518_rear_tx_freq_fixed_index == -1){
		info("%s - not support freq",__func__);
		return -1;
	}
	info("%s - tx freq fixed : %d", __func__,
			sensor_imx518_supported_tx_freq[sensor_imx518_rear_tx_freq_fixed_index]);
	return sensor_imx518_supported_tx_freq[sensor_imx518_rear_tx_freq_fixed_index];
}
#endif
#if defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION) || defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE)
int sensor_imx518_cis_get_tx_freq(struct v4l2_subdev *subdev, u32 *value)
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

	if (module->position == SENSOR_POSITION_REAR_TOF) {
#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
		info("%s rear tx freq %d", __func__, sensor_imx518_rear_tx_freq);
		*value = sensor_imx518_rear_tx_freq;
#else
		info("%s freq variation disabled, rear tx freq %d", __func__, REAR_TX_DEFAULT_FREQ);
		*value = REAR_TX_DEFAULT_FREQ;
#endif
	} else { //SENSOR_POSITION_FRONT_TOF
		info("%s front tx freq none ", __func__);
	}

	return ret;
}
#endif

static struct is_cis_ops cis_ops_imx518 = {
	.cis_init = sensor_imx518_cis_init,
	.cis_log_status = sensor_imx518_cis_log_status,
	.cis_set_global_setting = sensor_imx518_cis_set_global_setting,
	.cis_mode_change = sensor_imx518_cis_mode_change,
	.cis_stream_on = sensor_imx518_cis_stream_on,
	.cis_stream_off = sensor_imx518_cis_stream_off,
	.cis_set_exposure_time = sensor_imx518_cis_set_exposure_time,
	.cis_wait_streamoff = sensor_imx518_cis_wait_streamoff,
	.cis_wait_streamon = sensor_imx518_cis_wait_streamon,
	.cis_set_laser_control = sensor_imx518_cis_set_laser_control,
	.cis_set_laser_mode = sensor_imx518_cis_set_laser_mode,
	.cis_set_laser_current = sensor_imx518_cis_set_vcsel_current,
	.cis_get_laser_photo_diode = sensor_imx518_cis_get_vcsel_photo_diode,
	.cis_set_frame_rate = sensor_imx518_cis_set_frame_rate,
	.cis_get_tof_laser_error_flag = sensor_imx518_cis_laser_error_req,
#if defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION) || defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE)
	.cis_get_tof_tx_freq = sensor_imx518_cis_get_tx_freq,
#endif
#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
	.cis_set_tof_tx_freq = sensor_imx518_cis_set_tx_freq,
#endif
};

static int cis_imx518_probe_i2c(struct i2c_client *client,
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
	cis->cis_ops = &cis_ops_imx518;
	/* belows are depend on sensor cis. MUST check sensor spec */
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;

	if (of_property_read_string(dnode, "setfile", &setfile)) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0) {
		probe_info("%s setfile_A\n", __func__);
		sensor_imx518_global = sensor_imx518_setfile_A_Global;
		sensor_imx518_global_size = ARRAY_SIZE(sensor_imx518_setfile_A_Global);
		sensor_imx518_setfiles = sensor_imx518_setfiles_A;
		sensor_imx518_mode_id = sensor_imx518_setfiles_mode_id;
		sensor_imx518_max_mode_id_num = sensor_imx518_setfiles_mode_id_num;
		sensor_imx518_setfile_sizes = sensor_imx518_setfile_A_sizes;
		sensor_imx518_setfiles_fps = sensor_imx518_setfile_FPS_A;
		sensor_imx518_laser_setting = sensor_imx518_laser_setting_A;
		sensor_imx518_laser_setting_sizes = sensor_imx518_laser_setting_A_sizes;
#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
		sensor_imx518_tx_freq_sensor_mode = sensor_imx518_setfile_A_tof_sensor_mode;
#endif
	} else {
		err("%s setfile index out of bound, take default (setfile_A)", __func__);
		sensor_imx518_global = sensor_imx518_setfile_A_Global;
		sensor_imx518_global_size = ARRAY_SIZE(sensor_imx518_setfile_A_Global);
		sensor_imx518_setfiles = sensor_imx518_setfiles_A;
		sensor_imx518_mode_id = sensor_imx518_setfiles_mode_id;
		sensor_imx518_max_mode_id_num = sensor_imx518_setfiles_mode_id_num;
		sensor_imx518_setfile_sizes = sensor_imx518_setfile_A_sizes;
	}

#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
	{
		probe_info("%s verify mipi_channel",__func__);
	}
#endif
	probe_info("%s done\n", __func__);

	return ret;
}

static const struct of_device_id sensor_cis_imx518_match[] = {
	{
		.compatible = "samsung,exynos-is-cis-imx518",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx518_match);

static const struct i2c_device_id sensor_cis_imx518_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx518_driver = {
	.probe	= cis_imx518_probe_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx518_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx518_idt
};

static int __init sensor_cis_imx518_init(void)
{
	int ret;

	probe_info("sensor_cis_imx518_init");
	ret = i2c_add_driver(&sensor_cis_imx518_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_imx518_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx518_init);

MODULE_LICENSE("GPL");
