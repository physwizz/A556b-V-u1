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
#include "is-cis-imx616.h"
#include "is-cis-imx616-setA.h"
#include "is-cis-imx616-setB.h"

#include "is-helper-ixc.h"
#ifdef CONFIG_CAMERA_VENDOR_MCD_V2
#include "is-sec-define.h"
#endif

#include "interface/is-interface-library.h"

#define SENSOR_NAME "IMX616"

static const u32 *sensor_imx616_global;
static u32 sensor_imx616_global_size;
static const u32 **sensor_imx616_setfiles;
static const u32 *sensor_imx616_setfile_sizes;
static u32 sensor_imx616_max_setfile_num;
static const struct sensor_pll_info_compact **sensor_imx616_pllinfos;

static bool sensor_imx616_cal_write_flag;

int sensor_imx616_cis_check_rev(struct is_cis *cis)
{
	int ret = 0;
	u8 rev = 0, status = 0;


	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	/* Specify OTP Page Address for READ - Page127(dec) */
	ret = cis->ixc_ops->write8(cis->client, REG(OTP_PAGE_SETUP), 0x7F);
	if (ret < 0)
		err("is_sensor_read8 fail (ret %d)", ret);

	/* Turn ON OTP Read MODE */
	ret = cis->ixc_ops->write8(cis->client, REG(OTP_READ_TRANSFER_MODE), 0x01);
	if (ret < 0)
		err("is_sensor_write8 fail (ret %d)", ret);

	/* Check status - 0x01 : read ready*/
	ret = cis->ixc_ops->read8(cis->client, REG(OTP_STATUS_REGISTER), &status);
	if (ret < 0)
		err("is_sensor_read8 fail (ret %d)", ret);

	if ((status & 0x1) == false)
		err("status fail, (%d)", status);

	/* CHIP REV 0x0018 */
	ret = cis->ixc_ops->read8(cis->client, REG(OTP_CHIP_REVISION), &rev);
	if (ret < 0)
		err("is_sensor_read8 fail (ret %d)", ret);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis->cis_data->cis_rev = rev;

	probe_info("imx616 rev:%x", rev);

	return ret;
}

static void sensor_imx616_set_integration_max_margin(u32 mode, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!cis_data);

	cis_data->max_margin_coarse_integration_time = VAL(COARSE_INTEG_TIME_MAX_MARGIN);
	dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n",
		cis_data->max_margin_coarse_integration_time);
}

static void sensor_imx616_set_integration_min(u32 mode, cis_shared_data *cis_data)
{
	FIMC_BUG_VOID(!cis_data);

	cis_data->min_coarse_integration_time = VAL(COARSE_INTEG_TIME_MIN);
	dbg_sensor(1, "min_coarse_integration_time(%d)\n",
		cis_data->min_coarse_integration_time);
}

static void sensor_imx616_cis_data_calculation(const struct sensor_pll_info_compact *pll_info, cis_shared_data *cis_data)
{
	u64 pixel_rate;
	u32 frame_rate, max_fps, frame_valid_us;

	FIMC_BUG_VOID(!pll_info);

	/* 1. get pclk value from pll info */
	pixel_rate = pll_info->pclk * TOTAL_NUM_OF_IVTPX_CHANNEL;

	/* 2. FPS calculation */
	frame_rate = pixel_rate / (pll_info->frame_length_lines * pll_info->line_length_pck);

	/* 3. the time of processing one frame calculation (us) */
	cis_data->min_frame_us_time = (1 * 1000 * 1000) / frame_rate;
	cis_data->cur_frame_us_time = cis_data->min_frame_us_time;

	dbg_sensor(1, "frame_duration(%d) - frame_rate (%d) = pixel_rate(%llu) / "
		KERN_CONT "(pll_info->frame_length_lines(%d) * pll_info->line_length_pck(%d))\n",
		cis_data->min_frame_us_time, frame_rate, pixel_rate, pll_info->frame_length_lines, pll_info->line_length_pck);

	/* calculate max fps */
	max_fps = (pixel_rate * 10) / (pll_info->frame_length_lines * pll_info->line_length_pck);
	max_fps = (max_fps % 10 >= 5 ? frame_rate + 1 : frame_rate);

	cis_data->pclk = pixel_rate;
	cis_data->max_fps = max_fps;
	cis_data->frame_length_lines = pll_info->frame_length_lines;
	cis_data->line_length_pck = pll_info->line_length_pck;
	cis_data->line_readOut_time = (u64)cis_data->line_length_pck * 1000
					* 1000 * 1000 / cis_data->pclk;
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	/* Frame valid time calcuration */
	frame_valid_us = (u64)cis_data->cur_height * cis_data->line_length_pck
				* 1000 * 1000 / cis_data->pclk;
	cis_data->frame_valid_us_time = (int)frame_valid_us;

	dbg_sensor(1, "%s\n", __func__);
	dbg_sensor(1, "Sensor size(%d x %d) setting: SUCCESS!\n", cis_data->cur_width, cis_data->cur_height);
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
	cis_data->min_fine_integration_time = VAL(FINE_INTEG_TIME);
	cis_data->max_fine_integration_time = VAL(FINE_INTEG_TIME);
	info("%s: done", __func__);
}

#if SENSOR_IMX616_EBD_LINE_DISABLE
static int sensor_imx616_set_ebd(struct is_cis *cis)
{
	int ret = 0;
	u8 enabled_ebd = 0;


	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

#if SENSOR_IMX616_DEBUG_INFO
	ret = cis->ixc_ops->read8(cis->client, SENSOR_IMX616_EBD_CONTROL_ADDR, &enabled_ebd);
	if (ret < 0) {
		err("is_sensor_read is fail, (ret %d)", ret);
		ret = -EAGAIN;
		goto p_err;
	} else {
		pr_info("%s : before set, EBD Control = %d\n", __func__, enabled_ebd);
	}
#endif

	enabled_ebd = 0;	//default disabled
	ret = cis->ixc_ops->write8(cis->client, SENSOR_IMX616_EBD_CONTROL_ADDR, enabled_ebd);
	if (ret < 0) {
		err("is_sensor_read is fail, (ret %d)", ret);
		ret = -EAGAIN;
		goto p_err;
	}

#if SENSOR_IMX616_DEBUG_INFO
	ret = cis->ixc_ops->read8(cis->client, SENSOR_IMX616_EBD_CONTROL_ADDR, &enabled_ebd);
	if (ret < 0) {
		err("is_sensor_read is fail, (ret %d)", ret);
		ret = -EAGAIN;
		goto p_err;
	} else {
		pr_info("%s : after set,  EBD Control = %d\n", __func__, enabled_ebd);
	}
#endif

p_err:
	return ret;
}
#endif

#if SENSOR_IMX616_CAL_DEBUG
int sensor_imx616_cis_cal_dump(char* name, char *buf, size_t size)
{
	int ret = 0;
#ifdef USE_KERNEL_VFS_READ_WRITE
	struct file *fp;

	fp = filp_open(name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0666);
	if (IS_ERR_OR_NULL(fp)) {
		ret = PTR_ERR(fp);
		err("%s(): open file error(%s), error(%d)\n", __func__, name, ret);
		goto p_err;
	}

	ret = kernel_write(fp, buf, size, &fp->f_pos);
	if (ret < 0) {
		err("%s(): file write fail(%s) ret(%d)", __func__,
				name, ret);
		goto p_err;
	}

p_err:
	if (!IS_ERR_OR_NULL(fp))
		filp_close(fp, NULL);

#else
	err("not support %s due to kernel_write!", __func__);
	ret = -EINVAL;
#endif
p_err:
	return ret;
}
#endif

int sensor_imx616_cis_QuadSensCal_write(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_device_sensor_peri *sensor_peri = NULL;
	int position;
	u16 start_addr;
	u16 data_size;

	ulong cal_addr;
	u8 cal_data[SENSOR_IMX616_QUAD_SENS_CAL_SIZE] = {0, };

#ifdef CONFIG_CAMERA_VENDOR_MCD_V2
	char *rom_cal_buf = NULL;
#else
	struct is_minfo *minfo = is_get_is_minfo();
#endif

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	FIMC_BUG(!sensor_peri);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		return -EINVAL;
	}

	position = sensor_peri->module->position;

#ifdef CONFIG_CAMERA_VENDOR_MCD_V2
	ret = is_sec_get_cal_buf(position, &rom_cal_buf);

	if (ret < 0) {
		goto p_err;
	}

	cal_addr = (ulong)rom_cal_buf;
	 if (position == SENSOR_POSITION_FRONT) {
		cal_addr += SENSOR_IMX616_QUAD_SENS_CAL_BASE_FRONT;
	} else {
		err("cis_imx616 position(%d) is invalid!\n", position);
		goto p_err;
	}
#else
	if (position == SENSOR_POSITION_FRONT){
		cal_addr = minfo->kvaddr_cal[position] + SENSOR_IMX616_QUAD_SENS_CAL_BASE_FRONT;
	}else {
		err("cis_imx616 position(%d) is invalid!\n", position);
		goto p_err;
	}
#endif

	memcpy(cal_data, (u16 *)cal_addr, SENSOR_IMX616_QUAD_SENS_CAL_SIZE);

#if SENSOR_IMX616_CAL_DEBUG
	ret = sensor_imx616_cis_cal_dump(SENSOR_IMX616_QSC_DUMP_NAME, (char *)cal_data, (size_t)SENSOR_IMX616_QUAD_SENS_CAL_SIZE);
	CHECK_ERR_GOTO(ret < 0, p_err, "cis_imx616 QSC Cal dump fail(%d)!\n", ret);
#endif

	start_addr = REG(QUAD_SENS_REG);
	data_size = SENSOR_IMX616_QUAD_SENS_CAL_SIZE;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = cis->ixc_ops->write8_sequential(client, start_addr, cal_data, data_size);
	if (ret < 0) {
		err("cis_imx616 QSC write Error(%d)\n", ret);
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

/*************************************************
 *  [IMX616 Analog gain formular]
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
 *  Analog Gain Range = 112 to 1008 (1dB to 26dB)
 *
 *************************************************/

u32 sensor_imx616_cis_calc_again_code(u32 permille)
{
	return 1024 - (1024000 / permille);
}

u32 sensor_imx616_cis_calc_again_permile(u32 code)
{
	return 1024000 / (1024 - code);
}

u32 sensor_imx616_cis_calc_dgain_code(u32 permile)
{
	u8 buf[2] = {0, 0};
	buf[0] = (permile / 1000) & 0x0F;
	buf[1] = (((permile - (buf[0] * 1000)) * 256) / 1000);

	return (buf[0] << 8 | buf[1]);
}

u32 sensor_imx616_cis_calc_dgain_permile(u32 code)
{
	return (((code & 0x0F00) >> 8) * 1000) + ((code & 0x00FF) * 1000 / 256);
}

int sensor_imx616_cis_get_fineIntegTime(struct is_cis *cis)
{
	int ret = 0;
	u16 fine_integ_time = 0;

	FIMC_BUG(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	/* FINE_INTEG_TIME ADDR [0x0200:0x0201] */
	ret = cis->ixc_ops->read16(cis->client, REG(FINE_INTEG_TIME), &fine_integ_time);
	if (ret < 0) {
		err("is_sensor_read16 fail (ret %d)", ret);
		IXC_MUTEX_UNLOCK(cis->ixc_lock);
		return ret;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	info("%s: read fine_integ_time = %#x\n", __func__, fine_integ_time);

	return ret;
}

/* CIS OPS */
int sensor_imx616_cis_init(struct v4l2_subdev *subdev)
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

	info("[%s] init %s\n", __func__, cis->use_3hdr ? "(Use 3HDR)" : "");
	cis->rev_flag = false;

/***********************************************************************
***** Check that QSC and DPC Cal is written for Remosaic Capture.
***** false : Not yet write the QSC and DPC
***** true  : Written the QSC and DPC Or Skip
***********************************************************************/
	sensor_imx616_cal_write_flag = false;

	ret = sensor_imx616_cis_check_rev(cis);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
		if (sensor_peri)
			is_sec_get_hw_param(&hw_param, sensor_peri->module->position);
		if (hw_param)
			hw_param->i2c_sensor_err_cnt++;
#endif
		warn("sensor_imx616_check_rev is fail when cis init");
		cis->rev_flag = true;
		ret = 0;
	}

	cis->cis_data->cur_width = SENSOR_IMX616_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_IMX616_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
	cis->long_term_mode.sen_strm_off_on_step = 0;

	cis->cis_data->stream_on = false;

	sensor_imx616_cis_data_calculation(sensor_imx616_pllinfos[setfile_index], cis->cis_data);
	sensor_imx616_set_integration_max_margin(setfile_index, cis->cis_data);
	sensor_imx616_set_integration_min(setfile_index, cis->cis_data);

#if SENSOR_IMX616_DEBUG_INFO
	ret = sensor_imx616_cis_get_fineIntegTime(cis);
	CHECK_ERR_GOTO(ret < 0, p_err, "sensor_imx616_cis_get_fineIntegTime fail!! (%d)", ret);

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
#endif

#if SENSOR_IMX616_SENSOR_CAL_FOR_REMOSAIC
	if (sensor_imx616_cal_write_flag == false) {
		sensor_imx616_cal_write_flag = true;

		info("[%s] mode is QBC Remosaic Mode! Write QSC data.\n", __func__);

		ret = sensor_imx616_cis_QuadSensCal_write(subdev);
		CHECK_ERR_GOTO(ret < 0, p_err, "sensor_imx616_Quad_Sens_Cal_write fail!! (%d)", ret);
	}
#endif

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_imx616_cis_log_status(struct v4l2_subdev *subdev)
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

	sensor_cis_dump_registers(subdev, sensor_imx616_setfiles[0], sensor_imx616_setfile_sizes[0]);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	pr_err("[SEN:DUMP] *******************************\n");

p_err:
	return ret;
}

static int sensor_imx616_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{
#if USE_GROUP_PARAM_HOLD
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
#else

	return 0;
#endif
}

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_imx616_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_imx616_cis_group_param_hold_func(subdev, hold);
	if (ret < 0)
		goto p_err;

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_imx616_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	/* setfile global setting is at camera entrance */
	info("[%s] global setting start\n", __func__);
	ret = sensor_cis_set_registers(subdev, sensor_imx616_global, sensor_imx616_global_size);
	CHECK_ERR_GOTO(ret < 0, p_err, "sensor_imx616_set_registers fail!!");

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	/* Check that QSC and DPC Cal is written for Remosaic Capture.
	   false : Not yet write the QSC and DPC
	   true  : Written the QSC and DPC */
	sensor_imx616_cal_write_flag = false;
	return ret;
}

#ifdef SUPPORT_SENSOR_SEAMLESS_3HDR
/**
 * @brief sensor_imx616_cis_get_seamless_trasition_mode
 *  : get simplified tansition mode matched with recieved mode.
 * @return
 *  : transtion mode if success.
 */
int sensor_imx616_cis_get_seamless_trasition_mode(u32 mode)
{
	int pair_mode = -1;

	switch (mode) {
	case IMX616_MODE_QBCHDR_SEAMLESS_3264x1836_30FPS:
		pair_mode = IMX616_MODE_TRANSITION_2X2BIN_to_QBCHDR;
		break;
	case IMX616_MODE_2X2BIN_SEAMLESS_3264x1836_30FPS:
		pair_mode = IMX616_MODE_TRANSITION_QBCHDR_to_2X2BIN;
		break;
	default:
		err("invalid mode %d", mode);
		break;
	}

	if (pair_mode >= 0)
		info("[imx616] seamless trasition mode %d\n", mode);

	return pair_mode;
}
#endif

int sensor_imx616_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	u16 aehist_linear_threth[] = { VAL(AEHIST_LINEAR_LO_THRETH),
				VAL(AEHIST_LINEAR_UP_THRETH) };
	u16 aehist_log_threth[] = { VAL(AEHIST_LOG_LO_THRETH),
				VAL(AEHIST_LOG_UP_THRETH) };
	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	if (mode > sensor_imx616_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	/* If check_rev(Sensor ID in OTP) of IMX616 fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_imx616_cis_check_rev(cis);
		CHECK_ERR_GOTO(ret < 0, p_err, "sensor_imx616_check_rev is fail");
		info("[%s] cis_rev=%#x\n", __func__, cis->cis_data->cis_rev);
	}

	sensor_imx616_set_integration_max_margin(mode, cis->cis_data);
	sensor_imx616_set_integration_min(mode, cis->cis_data);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	info("[%s] mode=%d, mode change setting start\n", __func__, mode);
	ret = sensor_cis_set_registers(subdev, sensor_imx616_setfiles[mode], sensor_imx616_setfile_sizes[mode]);
	CHECK_ERR_GOTO(ret < 0, p_i2c_err, "sensor_imx616_set_registers fail!!");

	/* Set AEHIST manual */
	if (IS_3HDR_MODE(cis)) {
		info("[imx616] set AEHIST manual\n");
		ret = cis->ixc_ops->write8(cis->client, REG(AEHIST_LN_AUTO_THRETH), 0x0);
		CHECK_GOTO(ret < 0, p_i2c_err);
		ret = cis->ixc_ops->write16_array(cis->client, REG(AEHIST_LN_THRETH_START),
					aehist_linear_threth, ARRAY_SIZE(aehist_linear_threth));
		CHECK_ERR_GOTO(ret < 0, p_i2c_err, "failed to set linear");

		ret = cis->ixc_ops->write8(cis->client, REG(AEHIST_LOG_AUTO_THRETH), 0x0);
		CHECK_GOTO(ret < 0, p_i2c_err);
		ret = cis->ixc_ops->write16_array(cis->client, REG(AEHIST_LOG_THRETH_START),
					aehist_log_threth, ARRAY_SIZE(aehist_log_threth));
		CHECK_ERR_GOTO(ret < 0, p_i2c_err, "failed to set log");

		/* Addtional Settings */
		ret = sensor_cis_set_registers(subdev, sensor_imx616_exposure_nr_on,
					ARRAY_SIZE(sensor_imx616_exposure_nr_on));
		CHECK_ERR_GOTO(ret < 0, p_i2c_err, "set exp fail!!");
	} else {
		/* Addtional Settings */
		ret = sensor_cis_set_registers(subdev, sensor_imx616_exposure_nr_off,
					ARRAY_SIZE(sensor_imx616_exposure_nr_off));
		CHECK_ERR_GOTO(ret < 0, p_i2c_err, "set exp fail!!");
	}

#if SENSOR_IMX616_EBD_LINE_DISABLE
	/* To Do :  if the mode is needed EBD, add the condition with mode. */
	ret = sensor_imx616_set_ebd(cis);
	if (ret < 0) {
		err("sensor_imx616_set_registers fail!!");
		goto p_i2c_err;
	}
#endif

	dbg_sensor(1, "[%s] mode changed(%d)\n", __func__, mode);

p_i2c_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

#ifdef SUPPORT_SENSOR_SEAMLESS_3HDR
int sensor_imx616_cis_mode_change_seamless(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	int hold = 0;
	int transition_mode = -1;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	if ( !IS_3HDR(cis, mode) || mode > sensor_imx616_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return -EINVAL;
	}

	/*sensor_imx616_set_integration_max_margin(mode, cis->cis_data);
	sensor_imx616_set_integration_min(mode, cis->cis_data); */

	info("[%s] mode=%d, seamless mode change start\n", __func__, mode);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c;
	}

	/* Transition to 3HDR or seamless-tetra mode */
	if (IS_3HDR_SEAMLESS(cis, mode)) {
		/* info("%s: 0x3020, 0x01\n", __func__);*/
		ret = cis->ixc_ops->write8(cis->client, 0x3020, 0x01);
		CHECK_ERR_GOTO(ret < 0, p_err_i2c, "transition cmd fail!!");
	} else {
		/* info("%s: 0x3020, 0x00\n", __func__);*/
		ret = cis->ixc_ops->write8(cis->client, 0x3020, 0x00);
		CHECK_ERR_GOTO(ret < 0, p_err_i2c, "transition cmd fail!!");
	}

	transition_mode = sensor_imx616_cis_get_seamless_trasition_mode(mode);
	CHECK_ERR_GOTO(transition_mode < 0, p_err_i2c, "get transition mode fail!!");

	ret = sensor_cis_set_registers(subdev, sensor_imx616_setfiles[transition_mode],
					sensor_imx616_setfile_sizes[transition_mode]);
	CHECK_ERR_GOTO(ret < 0, p_err_i2c, "seamless transition settings fail!!");

#if SENSOR_IMX616_EBD_LINE_DISABLE
	/* To Do :  if the mode is needed EBD, add the condition with mode. */
	ret = sensor_imx616_set_ebd(cis);
	if (ret < 0) {
		err("sensor_imx616_set_registers fail!!");
		goto p_err_i2c;
	}
#endif

	dbg_sensor(1, "[%s] mode changed(%d)\n", __func__, mode);

p_err_i2c:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_mode_change_wrap(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	if (cis->cis_data->stream_on) {
		if (!IS_3HDR(cis, mode)) {
			err("not allowed to change mode(%d) during stream on", mode);
			return -EPERM;
		}

		ret = sensor_imx616_cis_mode_change_seamless(subdev, mode);
	} else {
		ret = sensor_imx616_cis_mode_change(subdev, mode);
	}

	return ret;
}
#endif

int sensor_imx616_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
{
	int ret = 0;
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

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:

	return ret;
}

int sensor_imx616_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int hold = 0;
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


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	info("[imx616] stream on\n");
	if (IS_3HDR_MODE(cis))
		info("[imx616] 3hdr mode start\n");

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0)
		err("group_param_hold fail (hold %d)", hold);

#ifdef SENSOR_IMX616_DEBUG_INFO
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
	cis->ixc_ops->read16(cis->client, 0x030a, &pll);
	dbg_sensor(1, "______ op_sys_clk_div(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x030c, &pll);
	dbg_sensor(1, "______ op_prepllck_div(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x030e, &pll);
	dbg_sensor(1, "______ op_pll_multiplier(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0310, &pll);
	dbg_sensor(1, "______ pll_mult_driv(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0340, &pll);
	dbg_sensor(1, "______ frame_length_lines(%x)\n", pll);
	cis->ixc_ops->read16(cis->client, 0x0342, &pll);
	dbg_sensor(1, "______ line_length_pck(%x)\n", pll);
	}
#endif

#ifdef SUPPORT_SENSOR_SEAMLESS_3HDR
	if (IS_3HDR_MODE(cis)) {
		if (IS_3HDR_SEAMLESS_MODE(cis)) {
			/* info("%s: 0x3020, 0x01\n", __func__); */
			ret = cis->ixc_ops->write8(cis->client, 0x3020, 0x01);
		} else {
			/* info("%s: 0x3020, 0x00\n", __func__); */
			ret = cis->ixc_ops->write8(cis->client, 0x3020, 0x00);
		}

		ret |= cis->ixc_ops->write8(cis->client, 0x3021, 0x01);
		if (ret)
			err("seamless 3hdr enable fail!!");
	}
#endif
	/* Sensor stream on */
	cis->ixc_ops->write8(cis->client, 0x0100, 0x01);

	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
	if (hold < 0)
		err("group_param_hold fail (hold %d)", hold);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = true;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	cis_data = cis->cis_data;

	/*dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__); */
	info("[imx616] stream off\n");

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
	if (hold < 0)
		err("group_param_hold fail (hold %d)", hold);

#ifdef SUPPORT_SENSOR_SEAMLESS_3HDR
	if (IS_3HDR_MODE(cis)) {
		ret = cis->ixc_ops->write8(cis->client, 0x3020, 0x00);
		ret |= cis->ixc_ops->write8(cis->client, 0x3021, 0x00);
		if (ret)
			err("seamless 3hdr off fail!!");
	}
#endif

	/* stream off */
	ret = cis->ixc_ops->write8(cis->client, 0x0100, 0x00);
	if (ret < 0)
		err("%s fail (ret %d)",__func__, ret);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->stream_on = false;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
						u32 input_exposure_time,
						u32 *target_duration)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 coarse_integ_time = 0;
	u32 frame_length_lines = 0;
	u32 frame_duration = 0;
	u32 max_frame_us_time = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_duration);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	coarse_integ_time = (u32)((pix_rate_freq_khz * input_exposure_time) / (line_length_pck * 1000));
	frame_length_lines = coarse_integ_time + cis_data->max_margin_coarse_integration_time;

	frame_duration = (u32)((((u64)frame_length_lines * line_length_pck) / pix_rate_freq_khz) * 1000);
	max_frame_us_time = 1000000/cis->min_fps;

	dbg_sensor(1, "[%s](vsync cnt = %d) input exp(%d), adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, cis_data->sen_vsync_count, input_exposure_time, frame_duration, cis_data->min_frame_us_time);
	dbg_sensor(1, "[%s](vsync cnt = %d) adj duration, frame duraion(%d), min_frame_us(%d), max_frame_us_time(%d)\n",
			__func__, cis_data->sen_vsync_count, frame_duration, cis_data->min_frame_us_time, max_frame_us_time);

	dbg_sensor(1, "[%s] requested min_fps(%d), max_fps(%d) from HAL\n", __func__, cis->min_fps, cis->max_fps);

	*target_duration = MAX(frame_duration, cis_data->min_frame_us_time);
	if(cis->min_fps == cis->max_fps) {
		*target_duration = MIN(frame_duration, max_frame_us_time);
	}

	dbg_sensor(1, "[%s] calcurated frame_duration(%d), adjusted frame_duration(%d)\n", __func__, frame_duration, *target_duration);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;
	u16 frame_length_lines = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	cis_data = cis->cis_data;

	if (frame_duration < cis_data->min_frame_us_time) {
		dbg_sensor(1, "frame duration(%d) is less than min(%d)\n", frame_duration, cis_data->min_frame_us_time);
		frame_duration = cis_data->min_frame_us_time;
	}

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;

	frame_length_lines = (u16)((pix_rate_freq_khz * frame_duration) / (line_length_pck * 1000));

	dbg_sensor(1, "[MOD:D:%d] %s, pix_rate_freq_khz(%#x) frame_duration = %d us,"
			KERN_CONT "(line_length_pck%#x), frame_length_lines(%#x)\n",
			cis->id, __func__, pix_rate_freq_khz, frame_duration,
			line_length_pck, frame_length_lines);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	ret = cis->ixc_ops->write16(cis->client, REG(FRAME_LENGTH_LINE), frame_length_lines);
	CHECK_GOTO(ret < 0, p_i2c_err);

	cis_data->cur_frame_us_time = frame_duration;
	cis_data->frame_length_lines = frame_length_lines;
	cis_data->max_coarse_integration_time = cis_data->frame_length_lines - cis_data->max_margin_coarse_integration_time;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
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
		err("[MOD:D:%d] %s, request FPS is too high(%d), set to max_fps(%d)\n",
			cis->id, __func__, min_fps, cis_data->max_fps);
		min_fps = cis_data->max_fps;
	}

	if (min_fps == 0) {
		err("[MOD:D:%d] %s, request FPS is 0, set to min FPS(1)\n", cis->id, __func__);
		min_fps = 1;
	}

	frame_duration = (1 * 1000 * 1000) / min_fps;

	dbg_sensor(1, "[MOD:D:%d] %s, set FPS(%d), frame duration(%d)\n",
			cis->id, __func__, min_fps, frame_duration);

	ret = sensor_imx616_cis_set_frame_duration(subdev, frame_duration);
	CHECK_ERR_GOTO(ret < 0, p_err, "[MOD:D:%d] %s, set frame duration is fail(%d)\n",
			cis->id, __func__, ret);

	cis_data->min_frame_us_time = frame_duration;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:

	return ret;
}

int sensor_imx616_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u64 pix_rate_freq_khz = 0;
	u16 long_coarse_int = 0;
	u16 short_coarse_int = 0;
	u16 middle_coarse_int = 0;
	u32 line_length_pck = 0;
	u32 min_fine_int = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!target_exposure);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	if ((target_exposure->long_val <= 0) || (target_exposure->short_val <= 0)) {
		err("[%s] invalid target exposure(%d, %d)", __func__,
				target_exposure->long_val, target_exposure->short_val);
		return -EINVAL;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt %d): target long(%d), short(%d), middle(%d)\n",
		cis->id, __func__, cis_data->sen_vsync_count,
		target_exposure->long_val, target_exposure->short_val, target_exposure->middle_val);

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;

	long_coarse_int = (u16)(((target_exposure->long_val * pix_rate_freq_khz) - min_fine_int) / (line_length_pck * 1000));
	short_coarse_int = (u16)(((target_exposure->short_val * pix_rate_freq_khz) - min_fine_int) / (line_length_pck * 1000));
	middle_coarse_int = (u16)(((target_exposure->middle_val * pix_rate_freq_khz) - min_fine_int) / (line_length_pck * 1000));


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

	if (middle_coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), middle coarse(%d) max(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, middle_coarse_int, cis_data->max_coarse_integration_time);
		middle_coarse_int = cis_data->max_coarse_integration_time;
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

	if (middle_coarse_int < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), middle coarse(%d) min(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, middle_coarse_int, cis_data->min_coarse_integration_time);
		middle_coarse_int = cis_data->min_coarse_integration_time;
	}

	cis_data->cur_exposure_coarse = long_coarse_int;
	cis_data->cur_long_exposure_coarse = long_coarse_int;
	cis_data->cur_short_exposure_coarse = short_coarse_int;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	ret = cis->ixc_ops->write16(cis->client, REG(COARSE_INTEG_TIME), long_coarse_int);
	CHECK_GOTO(ret < 0, p_i2c_err);

	if (IS_3HDR_MODE(cis)) {
		if (cis_data->is_data.wdr_mode == CAMERA_WDR_OFF)
			err("Check me: now 3hdr mode, but WDR_OFF"); /* check again later */

		ret = cis->ixc_ops->write16(cis->client, REG(MID_COARSE_INTEG_TIME), middle_coarse_int);
		CHECK_GOTO(ret < 0, p_i2c_err);

		ret = cis->ixc_ops->write16(cis->client, REG(ST_COARSE_INTEG_TIME), short_coarse_int);
		CHECK_GOTO(ret < 0, p_i2c_err);
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt %d): pix_rate_freq_khz (%d),"
		KERN_CONT "line_length_pck(%d), frame_length_lines(%#x)\n", cis->id, __func__,
		cis_data->sen_vsync_count, pix_rate_freq_khz, line_length_pck, cis_data->frame_length_lines);
	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt %d): 3hdr(%d), coarse long(%#x), short(%#x), middle(%#x)\n",
		cis->id, __func__, cis_data->sen_vsync_count, IS_3HDR_MODE(cis),
		long_coarse_int, short_coarse_int, middle_coarse_int);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	cis_shared_data *cis_data = NULL;
	u32 min_integration_time = 0;
	u32 min_coarse = 0;
	u32 min_fine = 0;
	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	min_coarse = cis_data->min_coarse_integration_time;
	min_fine = cis_data->min_fine_integration_time;

	min_integration_time = (u32)(((line_length_pck * min_coarse) + min_fine) * 1000 / pix_rate_freq_khz);
	*min_expo = min_integration_time;

	dbg_sensor(1, "[%s] min integration time %d\n", __func__, min_integration_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 max_integration_time = 0;
	u32 max_coarse_margin = 0;
	u32 max_coarse = 0;
	u32 max_fine = 0;
	u64 pix_rate_freq_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	pix_rate_freq_khz = cis_data->pclk / 1000;
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = cis_data->frame_length_lines;
	max_coarse_margin = cis_data->max_margin_coarse_integration_time;
	max_coarse = frame_length_lines - max_coarse_margin;
	max_fine = cis_data->max_fine_integration_time;

	max_integration_time = (u32)(((line_length_pck * max_coarse) + max_fine) * 1000 / pix_rate_freq_khz);

	*max_expo = max_integration_time;

	/* TODO: Is this values update hear? */
	cis_data->max_coarse_integration_time = max_coarse;

	dbg_sensor(1, "[%s] max integration time %d, max coarse integration %d\n",
			__func__, max_integration_time, cis_data->max_coarse_integration_time);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
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

	again_code = sensor_imx616_cis_calc_again_code(input_again);

	if (again_code > cis_data->max_analog_gain[0]) {
		again_code = cis_data->max_analog_gain[0];
	} else if (again_code < cis_data->min_analog_gain[0]) {
		again_code = cis_data->min_analog_gain[0];
	}

	again_permile = sensor_imx616_cis_calc_again_permile(again_code);

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

int sensor_imx616_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 long_gain = 0;
	u16 short_gain = 0;
	u16 middle_gain = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	cis_data = cis->cis_data;

	long_gain = (u16)sensor_imx616_cis_calc_again_code(again->long_val);
	short_gain = (u16)sensor_imx616_cis_calc_again_code(again->short_val);
	middle_gain = (u16)sensor_imx616_cis_calc_again_code(again->middle_val);

	if (long_gain < cis_data->min_analog_gain[0])
		long_gain = cis_data->min_analog_gain[0];

	if (long_gain > cis_data->max_analog_gain[0])
		long_gain = cis_data->max_analog_gain[0];

	if (short_gain < cis_data->min_analog_gain[0])
		short_gain = cis_data->min_analog_gain[0];

	if (short_gain > cis_data->max_analog_gain[0])
		short_gain = cis_data->max_analog_gain[0];

	if (middle_gain < cis_data->min_analog_gain[0])
		middle_gain = cis_data->min_analog_gain[0];

	if (middle_gain > cis_data->max_analog_gain[0])
		middle_gain = cis_data->max_analog_gain[0];

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt %d): input_again = %d/%d/%d us,"
		KERN_CONT "long_gain(%#x), short_gain(%#x), middle_gain(%#x)\n",
			cis->id, __func__, cis_data->sen_vsync_count,
			again->long_val, again->short_val, again->middle_val,
			long_gain, short_gain, middle_gain);

	if (IS_3HDR_MODE(cis) && !IS_3HDR_SEAMLESS_MODE(cis)) {
		ASSERT(long_gain >= VAL(QBCHDR_MIN_AGAIN) && long_gain <= VAL(QBCHDR_MAX_AGAIN),
			"long_gain %#X", long_gain);
		ASSERT(short_gain >= VAL(QBCHDR_MIN_AGAIN) && short_gain <= VAL(QBCHDR_MAX_AGAIN),
			"short_gain %#X", short_gain);
		ASSERT(middle_gain >= VAL(QBCHDR_MIN_AGAIN) && middle_gain <= VAL(QBCHDR_MAX_AGAIN),
			"middle_gain %#X", middle_gain);
	} else {
		ASSERT(long_gain >= VAL(MIN_AGAIN) && long_gain <= VAL(MAX_AGAIN),
			"long_gain %#X", long_gain);
	}

	long_gain &= 0x03FF;
	short_gain &= 0x03FF;
	middle_gain &= 0x03FF;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	ret = cis->ixc_ops->write16(cis->client, SENSOR_IMX616_AGAIN_ADDR, long_gain);
	CHECK_GOTO(ret < 0, p_i2c_err);

	if (IS_3HDR_MODE(cis)) {
		if (cis_data->is_data.wdr_mode == CAMERA_WDR_OFF)
			err("Check me: now 3hdr mode, but WDR_OFF"); /* check again later */

		ret = cis->ixc_ops->write16(cis->client, REG(MID_AGAIN), middle_gain);
		CHECK_GOTO(ret < 0, p_i2c_err);

		ret = cis->ixc_ops->write16(cis->client, REG(ST_AGAIN), short_gain);
		CHECK_GOTO(ret < 0, p_i2c_err);
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
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


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	ret = cis->ixc_ops->read16(cis->client, REG(AGAIN), &analog_gain);
	CHECK_GOTO(ret < 0, p_i2c_err);

	analog_gain &= 0x03FF;
	*again = sensor_imx616_cis_calc_again_permile(analog_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_again = %d us, analog_gain(%#x)\n",
			cis->id, __func__, *again, analog_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 min_again_code = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	if (IS_3HDR_MODE(cis))
		min_again_code = VAL(QBCHDR_MIN_AGAIN);
	else
		min_again_code = VAL(MIN_AGAIN);

	cis_data->min_analog_gain[0] = min_again_code;
	cis_data->min_analog_gain[1] = sensor_imx616_cis_calc_again_permile(min_again_code);
	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] min_again_code %d, main_again_permile %d\n", __func__,
		cis_data->min_analog_gain[0], cis_data->min_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 max_again_code = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	if (IS_3HDR_MODE(cis) && !IS_3HDR_SEAMLESS_MODE(cis))
		max_again_code = VAL(QBCHDR_MAX_AGAIN);
	else
		max_again_code = VAL(MAX_AGAIN);

	cis_data->max_analog_gain[0] = max_again_code;
	cis_data->max_analog_gain[1] = sensor_imx616_cis_calc_again_permile(max_again_code);
	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] max_again_code %d, max_again_permile %d\n", __func__,
		cis_data->max_analog_gain[0], cis_data->max_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 long_gain = 0;
	u16 short_gain = 0;
	u16 middle_gain = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	cis_data = cis->cis_data;

	long_gain = (u16)sensor_cis_calc_dgain_code(dgain->long_val);
	short_gain = (u16)sensor_cis_calc_dgain_code(dgain->short_val);
	middle_gain = (u16)sensor_cis_calc_dgain_code(dgain->middle_val);

	if (long_gain < cis_data->min_digital_gain[0])
		long_gain = cis_data->min_digital_gain[0];

	if (long_gain > cis_data->max_digital_gain[0])
		long_gain = cis_data->max_digital_gain[0];

	if (short_gain < cis_data->min_digital_gain[0])
		short_gain = cis_data->min_digital_gain[0];

	if (short_gain > cis_data->max_digital_gain[0])
		short_gain = cis_data->max_digital_gain[0];

	if (middle_gain < cis_data->min_digital_gain[0])
		middle_gain = cis_data->min_digital_gain[0];

	if (middle_gain > cis_data->max_digital_gain[0])
		middle_gain = cis_data->max_digital_gain[0];

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt %d): input_dgain = %d/%d/%d us,"
		KERN_CONT "long_gain(%#x), short_gain(%#x), middle_gain(%#x)\n",
			cis->id, __func__, cis_data->sen_vsync_count,
			dgain->long_val, dgain->short_val, dgain->middle_val,
			long_gain, short_gain, middle_gain);

	if (IS_3HDR_MODE(cis) && !IS_3HDR_SEAMLESS_MODE(cis)) {
		ASSERT(long_gain >= VAL(QBCHDR_MIN_DGAIN) && long_gain <= VAL(QBCHDR_MAX_DGAIN),
			"long_gain %#X", long_gain);
		ASSERT(short_gain >= VAL(QBCHDR_MIN_DGAIN) && short_gain <= VAL(QBCHDR_MAX_DGAIN),
			"short_gain %#X", short_gain);
		ASSERT(middle_gain >= VAL(QBCHDR_MIN_DGAIN) && middle_gain <= VAL(QBCHDR_MAX_DGAIN),
			"middle_gain %#X", middle_gain);
	} else {
		ASSERT(long_gain >= VAL(MIN_DGAIN) && long_gain <= VAL(MAX_DGAIN),
			"long_gain %#X", long_gain);
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	ret = cis->ixc_ops->write16(cis->client, REG(DGAIN), long_gain);
	CHECK_GOTO(ret < 0, p_i2c_err);

	if (IS_3HDR_MODE(cis)) {
		if (cis_data->is_data.wdr_mode == CAMERA_WDR_OFF)
			err("Check me: now 3hdr mode, but WDR_OFF"); /* check again later */

		ret = cis->ixc_ops->write16(cis->client, REG(MID_DGAIN), middle_gain);
		CHECK_GOTO(ret < 0, p_i2c_err);

		ret = cis->ixc_ops->write16(cis->client, REG(ST_DGAIN), short_gain);
		CHECK_GOTO(ret < 0, p_i2c_err);
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
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


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	ret = cis->ixc_ops->read16(cis->client, REG(DGAIN), &digital_gain);
	CHECK_GOTO(ret < 0, p_i2c_err);

	*dgain = sensor_cis_calc_dgain_permile(digital_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, dgain_permile = %d, dgain_code(%#x)\n",
			cis->id, __func__, *dgain, digital_gain);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 min_dgain_code = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!min_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	if (IS_3HDR_MODE(cis))
		min_dgain_code = VAL(QBCHDR_MIN_DGAIN);
	else
		min_dgain_code = VAL(MIN_DGAIN);

	cis_data->min_digital_gain[0] = min_dgain_code;
	cis_data->min_digital_gain[1] = sensor_imx616_cis_calc_dgain_permile(min_dgain_code);

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] min_dgain_code %d, min_dgain_permile %d\n", __func__,
		cis_data->min_digital_gain[0], cis_data->min_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 max_dgain_code = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!max_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	cis_data = cis->cis_data;

	if (IS_3HDR_MODE(cis) && !IS_3HDR_SEAMLESS_MODE(cis))
		max_dgain_code = VAL(QBCHDR_MAX_DGAIN);
	else
		max_dgain_code = VAL(MAX_DGAIN);

	cis_data->max_digital_gain[0] = max_dgain_code;
	cis_data->max_digital_gain[1] = sensor_imx616_cis_calc_dgain_permile(max_dgain_code);

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] max_dgain_code %d, max_dgain_permile %d\n", __func__,
		cis_data->max_digital_gain[0], cis_data->max_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_imx616_cis_set_wb_gain(struct v4l2_subdev *subdev, struct wb_gains wb_gains)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 abs_gains[4] = {0, };	//[0]=gr, [1]=r, [2]=b, [3]=gb
	u32 div = 0;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	if (!IS_REMOSAIC_MODE(cis) && !IS_3HDR_MODE(cis))
		return 0;

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

	dbg_sensor(1, "[SEN:%d]%s:DDK vlaue: wb_gain_gr(%d), wb_gain_r(%d), wb_gain_b(%d), wb_gain_gb(%d)\n",
		cis->id, __func__, wb_gains.gr, wb_gains.r, wb_gains.b, wb_gains.gb);

	abs_gains[0] = (u16)((wb_gains.gr / div) & 0xFFFF);
	abs_gains[1] = (u16)((wb_gains.r / div) & 0xFFFF);
	abs_gains[2] = (u16)((wb_gains.b / div) & 0xFFFF);
	abs_gains[3] = (u16)((wb_gains.gb / div) & 0xFFFF);

	dbg_sensor(1, "[SEN:%d]%s, abs_gain_gr(0x%4X), abs_gain_r(0x%4X), abs_gain_b(0x%4X), abs_gain_gb(0x%4X)\n",
		cis->id, __func__, abs_gains[0], abs_gains[1], abs_gains[2], abs_gains[3]);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	ret = cis->ixc_ops->write16_array(cis->client, REG(ABS_GAIN_GR_SET), abs_gains, 4);
	CHECK_ERR_GOTO(ret < 0, p_i2c_err, "failed to write abs_gain");

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

void sensor_imx616_cis_data_calc(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	FIMC_BUG_VOID(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG_VOID(!cis);
	FIMC_BUG_VOID(!cis->cis_data);

	if (mode > sensor_imx616_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true && cis->cis_data->stream_on == false) {
		cis->rev_flag = false;
		ret = sensor_imx616_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_imx616_check_rev is fail: ret(%d)", ret);
			return;
		}
	}

	sensor_imx616_cis_data_calculation(sensor_imx616_pllinfos[mode], cis->cis_data);
}

#ifdef SUPPORT_SENSOR_SEAMLESS_3HDR
/**
 * sensor_imx616_cis_set_3hdr_flk_roi
 * : set flicker roi
 */
int sensor_imx616_cis_set_3hdr_flk_roi(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 roi[4];
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");
	CHECK_ERR_RET(!IS_3HDR_MODE(cis), 0, "can not set in none-3hdr");

	roi[0] = 0;
	roi[1] = 0;
	roi[2] = cis->cis_data->cur_width / NR_FLKER_BLK_W;
	roi[3] = cis->cis_data->cur_height / NR_FLKER_BLK_H;
	info("%s: (%d, %d, %d, %d) -> (%d, %d, %d, %d)", __func__,
		roi[0], roi[1], cis->cis_data->cur_width, cis->cis_data->cur_height,
		roi[0], roi[1], roi[2], roi[3]);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err_i2c;
	}

	ret = cis->ixc_ops->write16_array(cis->client, REG(FLK_AREA), roi, ARRAY_SIZE(roi));
	CHECK_ERR_GOTO(ret < 0, p_err_i2c, "failed to set flk_roi reg");

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err_i2c:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_set_3hdr_roi(struct v4l2_subdev *subdev, struct roi_setting_t rois)
{
	int i, ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 roi[4];
	u16 addrs[NR_ROI_AREAS] = {REG(AEHIST1), REG(AEHIST2)};
	u32 width, height;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);
	FIMC_BUG(NR_ROI_AREAS > ARRAY_SIZE(rois.roi_start_x));


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");
	CHECK_ERR_RET(!IS_3HDR_MODE(cis), 0, "can not set in none-3hdr");

	width = cis->cis_data->cur_width;
	height = cis->cis_data->cur_height;

	for (i = 0; i < NR_ROI_AREAS; i++) {
		dbg_sensor(1, "%s: [MOD:%d] roi_control[%d] (start_x:%d, start_y:%d, end_x:%d, end_y:%d)\n",
			__func__, cis->id, i, rois.roi_start_x[i], rois.roi_start_y[i],
					rois.roi_end_x[i], rois.roi_end_y[i]);
		ASSERT(rois.roi_start_x[i] < width, "%d:start_x %d < %d", i, rois.roi_start_x[i], width);
		ASSERT(rois.roi_start_y[i] < height, "%d:start_y %d < %d", i, rois.roi_start_y[i], height);
		ASSERT(rois.roi_end_x[i] > 0 && rois.roi_end_x[i] <= width,
			"%d:end_x %d / w %d", i, rois.roi_end_x[i], width);
		ASSERT(rois.roi_end_y[i] > 0 && rois.roi_end_y[i] <= height,
			"%d:end_y %d / h %d", i, rois.roi_end_y[i], height);
		ASSERT(rois.roi_start_x[i] + rois.roi_end_x[i] <= width,
			"%d: %d + %d <= w%d", i, rois.roi_start_x[i], rois.roi_end_x[i], width);
		ASSERT(rois.roi_start_y[i] + rois.roi_end_y[i] <= height,
			"%d: %d + %d <= h%d", i, rois.roi_start_y[i], rois.roi_end_y[i], height);
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	for (i = 0; i < NR_ROI_AREAS; i++) {
		roi[0] = rois.roi_start_x[i] >= width ? width - 1 : rois.roi_start_x[i];
		roi[1] = rois.roi_start_y[i] >= height ? height - 1 : rois.roi_start_y[i];
		roi[2] = rois.roi_end_x[i] > width ? width : rois.roi_end_x[i];
		roi[3] = rois.roi_end_y[i] > height ? height : rois.roi_end_y[i];
		/*info("%s: set [%d](%d, %d, %d, %d)\n", __func__, i, roi[0], roi[1], roi[2], roi[3]);*/

		ret = cis->ixc_ops->write16_array(cis->client, addrs[i], roi, ARRAY_SIZE(roi));
		CHECK_ERR_GOTO(ret < 0, p_err, "failed to set roi reg");
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_set_roi_stat(struct v4l2_subdev *subdev, struct roi_setting_t roi_control)
{
	return sensor_imx616_cis_set_3hdr_roi(subdev, roi_control);
}

int sensor_imx616_cis_set_3hdr_stat(struct v4l2_subdev *subdev, bool streaming, void *data)
{
	int ret = 0;
	/*int hold = 0;
	struct is_cis *cis;
	*/
	struct sensor_imx_3hdr_stat_control_per_frame *per_frame_stat = NULL;
	struct sensor_imx_3hdr_stat_control_mode_change *mode_change_stat = NULL;
	struct roi_setting_t *roi = NULL;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!data);

	/*dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_dgain = %d/%d/%d us,"
		KERN_CONT "long_gain(%#x), short_gain(%#x), middle_gain(%#x)\n",
			cis->id, __func__, cis_data->sen_vsync_count,
			dgain->long_val, dgain->short_val, dgain->middle_val,
			long_gain, short_gain, middle_gain);*/
	/* IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	} */

	if (streaming) {
		per_frame_stat = (struct sensor_imx_3hdr_stat_control_per_frame *)data;
	} else {
		mode_change_stat = (struct sensor_imx_3hdr_stat_control_mode_change *)data;
		roi = &mode_change_stat->y_sum_roi;
		info("[imx616] 3hdr_stat: roi (%d, %d, %d, %d), (%d, %d, %d, %d)\n",
			roi->roi_start_x[0], roi->roi_start_y[0], roi->roi_end_x[0], roi->roi_end_y[0],
			roi->roi_start_x[1], roi->roi_start_y[1], roi->roi_end_x[1], roi->roi_end_y[1]);
		ret = sensor_imx616_cis_set_3hdr_roi(subdev, mode_change_stat->y_sum_roi);
		CHECK_ERR_RET(ret < 0 , ret, "failed to 3hdr_roi");

		ret = sensor_imx616_cis_set_3hdr_flk_roi(subdev);
		CHECK_ERR_RET(ret < 0 , ret, "failed to flk_roi");
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	/*
p_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	*/

	return ret;
}
#endif /* SUPPORT_SENSOR_SEAMLESS_3HDR */

void sensor_imx616_cis_check_wdr_mode(struct v4l2_subdev *subdev, u32 mode_idx)
{
	struct is_cis *cis;

	FIMC_BUG_VOID(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG_VOID(!cis);
	FIMC_BUG_VOID(!cis->cis_data);

	/* check wdr mode */
	if (IS_3HDR(cis, mode_idx))
		cis->cis_data->is_data.wdr_enable = true;
	else
		cis->cis_data->is_data.wdr_enable = false;

	dbg_sensor(1, "[%s] wdr_enable: %d\n", __func__,
				cis->cis_data->is_data.wdr_enable);
}

/**
 * sensor_imx616_cis_init_3hdr_lsc_table: set LSC table on init
 */
int sensor_imx616_cis_init_3hdr_lsc_table(struct v4l2_subdev *subdev, void *data)
{
	int i, ret = 0;
	int hold = 0;
	struct is_cis *cis = NULL;

	struct sensor_imx_3hdr_lsc_table_init *lsc = NULL;
	/* const u16 ram_addr[] = {REG(KNOT_TAB_GR), REG(KNOT_TAB_GB)}; */
	u32 table_len = RAMTABLE_LEN * 2;
	u16 addr, val;
	ktime_t st = ktime_get();

	FIMC_BUG(!subdev);
	FIMC_BUG(!data);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");

	if (!IS_3HDR_MODE(cis) || IS_3HDR_SEAMLESS_MODE(cis)) {
		err("can not set in none-3hdr");
		return 0;
	}

	lsc = (struct sensor_imx_3hdr_lsc_table_init *)data;
	CHECK_ERR_RET(ARRAY_SIZE(lsc->ram_table) < table_len,
			0, "ramtable size smaller than %d", table_len);

	info("[imx616] set LSC table\n");

	for (i = 0; i < table_len; i++) {
		if (lsc->ram_table[i] > VAL(LSC_MAX_GAIN)) {
			err("ramtable[%d]: invalid gain %#X", i, lsc->ram_table[i]);
			lsc->ram_table[i] = VAL(LSC_MAX_GAIN);
		}
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	/* In 3hdr and seamless tetra mode, LSC enabled by setfile */

	/* Write RATE, RATE_Y */
	val = (VAL(LSC_APP_RATE) << 8) | VAL(LSC_APP_RATE_Y);
	ret = cis->ixc_ops->write16(cis->client, REG(LSC_APP_RATE), val);
	CHECK_GOTO(ret < 0, p_i2c_err);

	/* Write MODE, TABLE_SEL */
	val = (VAL(CALC_MODE_MANUAL) << 8) | VAL(TABLE_SEL_1);
	ret = cis->ixc_ops->write16(cis->client, REG(CALC_MODE), val);
	CHECK_GOTO(ret < 0, p_i2c_err);

	for (i = 0; i < table_len; i++) {
		addr = REG(KNOT_TAB_GR) + (i * 2);
		/* info("[RAMTABLE] %03d: %#X, %#X\n", i, addr, lsc->ram_table[i]); */
		cis->ixc_ops->write16(cis->client, addr, lsc->ram_table[i]);
		CHECK_ERR_GOTO(ret < 0, p_i2c_err, "failed to write table");
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_set_3hdr_tone(struct v4l2_subdev *subdev, struct sensor_imx_3hdr_tone_control tc)
{
	int i, ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u8 nr_transit_frames[4] = {0, };
	u16 blend2_addr[5] = { REG(BLD2_TC_RATIO_1), REG(BLD2_TC_RATIO_2),
				REG(BLD2_TC_RATIO_3), REG(BLD2_TC_RATIO_4),
				REG(BLD2_TC_RATIO_5) };
	u16 blend1_ratio, blend2_ratio, blend3_ratio;
	u16 hdrtc_ratio[5] = {0, };
	u8 blend3_ratio_1_5[5] = {0, };
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	cis_data = cis->cis_data;


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");
	CHECK_ERR_RET(!IS_3HDR_MODE(cis), 0, "can not set in none-3hdr");

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt %d): gmtc2 on(%d), gmtc2 ratio %#x, "
		"(%#x, %#x) - (%#x, %#x), manual tc ratio %#x, ltc ratio %#x, "
		"hdr_tc1 %#x, hdr_tc2 %#x, hdr_tc3 %#x, hdr_tc4 %#x, hdr_tc5 %#x\n",
		cis->id, __func__, cis_data->sen_vsync_count, tc.gmt_tc2_enable, tc.gmt_tc2_ratio,
		tc.manual21_frame_p1, tc.manual21_frame_p2, tc.manual12_frame_p1, tc.manual12_frame_p2,
		tc.manual_tc_ratio, tc.ltc_ratio, tc.hdr_tc_ratio_1, tc.hdr_tc_ratio_2, tc.hdr_tc_ratio_3,
		tc.hdr_tc_ratio_4, tc.hdr_tc_ratio_5);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	/* Blend1 */
	ret = cis->ixc_ops->write8(cis->client, REG(BLD1_GMTC2_EN), tc.gmt_tc2_enable ? 0x01 : 0x00);
	CHECK_GOTO(ret < 0, p_i2c_err);

	blend1_ratio = tc.gmt_tc2_ratio > 0x10 ? 0x10 : tc.gmt_tc2_ratio;
	ret = cis->ixc_ops->write8(cis->client, REG(BLD1_GMTC2_RATIO), blend1_ratio);
	CHECK_GOTO(ret < 0, p_i2c_err);

	nr_transit_frames[0] = tc.manual21_frame_p1 & 0x7F;
	nr_transit_frames[1] = tc.manual21_frame_p2 & 0x7F;
	nr_transit_frames[2] = tc.manual12_frame_p1 & 0x7F;
	nr_transit_frames[3] = tc.manual12_frame_p2 & 0x7F;

	ret = cis->ixc_ops->write8_array(client, REG(BLD1_GMTC_NR_TRANSIT_FRM),
					nr_transit_frames, ARRAY_SIZE(nr_transit_frames));
	CHECK_GOTO(ret < 0, p_i2c_err);

	/* Blend2 */
	blend2_ratio = tc.manual_tc_ratio > 0x10 ? 0x10 : tc.manual_tc_ratio;
	for (i = 0; i < ARRAY_SIZE(blend2_addr); i++) {
		ret = cis->ixc_ops->write8(cis->client, blend2_addr[i], blend2_ratio);
		CHECK_GOTO(ret < 0, p_i2c_err);
	}

	/* Blend3 */
	blend3_ratio = tc.ltc_ratio > 0x20 ? 0x20 : tc.ltc_ratio;
	for (i = 0; i < ARRAY_SIZE(blend3_ratio_1_5); i++) {
		blend3_ratio_1_5[i] = blend3_ratio;
	}

	ret = cis->ixc_ops->write8_array(client, REG(BLD3_LTC_RATIO_START),
					blend3_ratio_1_5, ARRAY_SIZE(blend3_ratio_1_5));
	ret |= cis->ixc_ops->write8(cis->client, REG(BLD3_LTC_RATIO_6), blend3_ratio);
	CHECK_GOTO(ret < 0, p_i2c_err);

	if (tc.hdr_tc_ratio_4 != tc.hdr_tc_ratio_5) {
		/* DDK needs to be checked */
		err("not same: hdr_tc4 %d, hdr_tc5 %d", tc.hdr_tc_ratio_4, tc.hdr_tc_ratio_5);
	}

	/* Blend4 */
	i = 0;
	hdrtc_ratio[i++] = tc.hdr_tc_ratio_1 > 0x100 ? 0x100 : tc.hdr_tc_ratio_1;
	hdrtc_ratio[i++] = tc.hdr_tc_ratio_2 > 0x100 ? 0x100 : tc.hdr_tc_ratio_2;
	hdrtc_ratio[i++] = tc.hdr_tc_ratio_3 > 0x100 ? 0x100 : tc.hdr_tc_ratio_3;
	hdrtc_ratio[i++] = tc.hdr_tc_ratio_4 > 0x100 ? 0x100 : tc.hdr_tc_ratio_4;
	hdrtc_ratio[i] = tc.hdr_tc_ratio_5 > 0x100 ? 0x100 : tc.hdr_tc_ratio_5;

	/*
	i = 0;
	hdrtc_addr[i++] = REG(BLD4_HDR_TC_RATIO1_UP);
	hdrtc_addr[i++] = REG(BLD4_HDR_TC_RATIO2_UP);
	hdrtc_addr[i++] = REG(BLD4_HDR_TC_RATIO3_UP);
	hdrtc_addr[i++] = REG(BLD4_HDR_TC_RATIO4_UP);
	hdrtc_addr[i] = REG(BLD4_HDR_TC_RATIO5_UP); */

	ret = cis->ixc_ops->write16_array(cis->client, REG(BLD4_HDR_TC_RATIO_START),
					hdrtc_ratio, ARRAY_SIZE(hdrtc_ratio));
	CHECK_GOTO(ret < 0, p_i2c_err);

	ASSERT((tc.gmt_tc2_ratio == blend1_ratio) && (tc.manual_tc_ratio == blend2_ratio) && (tc.ltc_ratio == blend3_ratio)
		&& (tc.manual21_frame_p1 == nr_transit_frames[0]) && (tc.manual21_frame_p2 == nr_transit_frames[1])
		&& (tc.manual12_frame_p1 == nr_transit_frames[2]) && (tc.manual12_frame_p2 == nr_transit_frames[3])
		&& (tc.hdr_tc_ratio_1 == hdrtc_ratio[0]) && (tc.hdr_tc_ratio_2 == hdrtc_ratio[1]) && (tc.hdr_tc_ratio_3 == hdrtc_ratio[2])
		&& (tc.hdr_tc_ratio_4 == hdrtc_ratio[3]) && (tc.hdr_tc_ratio_5 == hdrtc_ratio[4]),
		"changed tc values were set!");

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt %d): gmtc2 on(%d), gmtc2 ratio %#x, "
		"(%#x, %#x) - (%#x, %#x), manual tc ratio %#x, ltc ratio %#x, "
		"hdr_tc1 %#x, hdr_tc2 %#x, hdr_tc3 %#x, hdr_tc4 %#x, hdr_tc5 %#x\n",
		cis->id, __func__, cis_data->sen_vsync_count, tc.gmt_tc2_enable, blend1_ratio,
		nr_transit_frames[0], nr_transit_frames[1], nr_transit_frames[2], nr_transit_frames[3],
		blend2_ratio, blend3_ratio, hdrtc_ratio[0], hdrtc_ratio[1], hdrtc_ratio[2],
		hdrtc_ratio[3], hdrtc_ratio[4]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

int sensor_imx616_cis_set_3hdr_ev(struct v4l2_subdev *subdev, struct sensor_imx_3hdr_ev_control ev_control)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	int pgain = 0, ngain = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	cis_data = cis->cis_data;


	CHECK_ERR_RET(!client, -EINVAL, "client is NULL");
	CHECK_ERR_RET(!IS_3HDR_MODE(cis), 0, "can not set in none-3hdr");

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt %d): evc_pgain %#x, evc_ngain %#x\n",
		cis->id, __func__, cis_data->sen_vsync_count, ev_control.evc_pgain, ev_control.evc_ngain);

	if (ev_control.evc_pgain && ev_control.evc_ngain) {
		err("invalid ev control. pgain %d, ngain %d", ev_control.evc_pgain, ev_control.evc_ngain);
		return -EINVAL;
	}

	pgain = ev_control.evc_pgain > 0x10 ? 0x10: ev_control.evc_pgain;
	ngain = ev_control.evc_ngain > 0x10 ? 0x10: ev_control.evc_ngain;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_i2c_err;
	}

	ret = cis->ixc_ops->write8(cis->client, REG(EVC_PGAIN), pgain);
	CHECK_GOTO(ret < 0, p_i2c_err);

	ret = cis->ixc_ops->write8(cis->client, REG(EVC_NGAIN), ngain);
	CHECK_GOTO(ret < 0, p_i2c_err);

	if (pgain != ev_control.evc_pgain || ngain != ev_control.evc_ngain) {
		err("pgain %d -> %d, ngain %d -> %d", ev_control.evc_pgain, pgain,
						ev_control.evc_ngain, ngain);
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_i2c_err:
	if (hold > 0) {
		hold = sensor_imx616_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}

static struct is_cis_ops cis_ops_imx616 = {
	.cis_init = sensor_imx616_cis_init,
	.cis_log_status = sensor_imx616_cis_log_status,
	.cis_group_param_hold = sensor_imx616_cis_group_param_hold,
	.cis_set_global_setting = sensor_imx616_cis_set_global_setting,
	.cis_set_size = sensor_imx616_cis_set_size,
#ifdef SUPPORT_SENSOR_SEAMLESS_3HDR
	.cis_mode_change = sensor_imx616_cis_mode_change_wrap,
#else
	.cis_mode_change = sensor_imx616_cis_mode_change,
#endif
	.cis_stream_on = sensor_imx616_cis_stream_on,
	.cis_stream_off = sensor_imx616_cis_stream_off,
	.cis_wait_streamon = sensor_cis_wait_streamon,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_adjust_frame_duration = sensor_imx616_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_imx616_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_imx616_cis_set_frame_rate,
	.cis_set_exposure_time = sensor_imx616_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_imx616_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_imx616_cis_get_max_exposure_time,
	.cis_adjust_analog_gain = sensor_imx616_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_imx616_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_imx616_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_imx616_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_imx616_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_imx616_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_imx616_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_imx616_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_imx616_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_cis_compensate_gain_for_extremely_br,
	.cis_set_wb_gains = sensor_imx616_cis_set_wb_gain,
	.cis_data_calculation = sensor_imx616_cis_data_calc,
#ifdef SUPPORT_SENSOR_SEAMLESS_3HDR
	.cis_set_roi_stat = sensor_imx616_cis_set_roi_stat,
	.cis_set_3hdr_stat = sensor_imx616_cis_set_3hdr_stat,
#endif
	.cis_check_wdr_mode = sensor_imx616_cis_check_wdr_mode,
	.cis_init_3hdr_lsc_table = sensor_imx616_cis_init_3hdr_lsc_table,
	.cis_set_tone_stat = sensor_imx616_cis_set_3hdr_tone,
	.cis_set_ev_stat = sensor_imx616_cis_set_3hdr_ev,
};

int cis_imx616_probe_i2c(struct i2c_client *client,
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
	cis->cis_ops = &cis_ops_imx616;
	/* belows are depend on sensor cis. MUST check sensor spec */
	cis->bayer_order = OTF_INPUT_ORDER_BAYER_RG_GB;
	cis->use_wb_gain = true;

	cis->use_3hdr = of_property_read_bool(dnode, "use_3hdr");
	probe_info("%s use_3hdr(%d)\n", __func__, cis->use_3hdr);

	ret = of_property_read_string(dnode, "setfile", &setfile);
	if (ret) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	if (strcmp(setfile, "default") == 0 || strcmp(setfile, "setA") == 0) {
		probe_info("%s setfile_A\n", __func__);
		sensor_imx616_global = sensor_imx616_setfile_A_Global;
		sensor_imx616_global_size = ARRAY_SIZE(sensor_imx616_setfile_A_Global);
		sensor_imx616_setfiles = sensor_imx616_setfiles_A;
		sensor_imx616_setfile_sizes = sensor_imx616_setfile_A_sizes;
		sensor_imx616_pllinfos = sensor_imx616_pllinfos_A;
		sensor_imx616_max_setfile_num = ARRAY_SIZE(sensor_imx616_setfiles_A);
	} else if (strcmp(setfile, "setB") == 0) {
		probe_info("%s setfile_B\n", __func__);
		sensor_imx616_global = sensor_imx616_setfile_B_Global;
		sensor_imx616_global_size = ARRAY_SIZE(sensor_imx616_setfile_B_Global);
		sensor_imx616_setfiles = sensor_imx616_setfiles_B;
		sensor_imx616_setfile_sizes = sensor_imx616_setfile_B_sizes;
		sensor_imx616_pllinfos = sensor_imx616_pllinfos_B;
		sensor_imx616_max_setfile_num = ARRAY_SIZE(sensor_imx616_setfiles_B);
	} else {
		err("%s setfile index out of bound, take default (setfile_A)", __func__);
		sensor_imx616_global = sensor_imx616_setfile_A_Global;
		sensor_imx616_global_size = ARRAY_SIZE(sensor_imx616_setfile_A_Global);
		sensor_imx616_setfiles = sensor_imx616_setfiles_A;
		sensor_imx616_setfile_sizes = sensor_imx616_setfile_A_sizes;
		sensor_imx616_pllinfos = sensor_imx616_pllinfos_A;
		sensor_imx616_max_setfile_num = ARRAY_SIZE(sensor_imx616_setfiles_A);
	}

	probe_info("%s done\n", __func__);

	return ret;
}

static const struct of_device_id sensor_cis_imx616_match[] = {
	{
		.compatible = "samsung,exynos-is-cis-imx616",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_imx616_match);

static const struct i2c_device_id sensor_cis_imx616_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_imx616_driver = {
	.probe	= cis_imx616_probe_i2c,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_imx616_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_imx616_idt
};

static int __init sensor_cis_imx616_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_imx616_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_imx616_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_imx616_init);

MODULE_LICENSE("GPL");
