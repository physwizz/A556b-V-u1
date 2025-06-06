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
#include <asm/byteorder.h>

#include <exynos-is-sensor.h>
#include "is-hw.h"
#include "is-core.h"
#include "is-param.h"
#include "is-device-sensor.h"
#include "is-device-sensor-peri.h"
#include "is-resourcemgr.h"
#include "is-dt.h"
#include "is-cis-2x5.h"
#include "is-cis-2x5-setA.h"
#include "is-cis-2x5-setB.h"

#include "is-helper-ixc.h"
#if defined(SUPPORT_2X5_SENSOR_VARIATION) || defined(SENSOR_2X5_CAL_UPLOAD)
#ifdef CONFIG_CAMERA_VENDOR_MCD_V2
#include "fimc-is-sec-define.h"
#endif
static bool last_version = false;
#endif
static bool cal_done = false;

#define SENSOR_NAME "S5K2X5"
/* #define DEBUG_2X5_PLL */

static const u32 *sensor_2x5_global;
static u32 sensor_2x5_global_size;
static const u32 **sensor_2x5_setfiles;
static const u32 *sensor_2x5_setfile_sizes;
static u32 sensor_2x5_max_setfile_num;
static const u32 *sensor_2x5_precal;
static u32 sensor_2x5_precal_size;
static const u32 *sensor_2x5_postcal;
static u32 sensor_2x5_postcal_size;
static const u32 **sensor_2x5_tetra_isp;
static const u32 *sensor_2x5_tetra_isp_sizes;
static const struct sensor_pll_info_compact **sensor_2x5_pllinfos;

static void sensor_2x5_cis_data_calculation(const struct sensor_pll_info_compact *pll_info_compact, cis_shared_data *cis_data)
{
	u64 vt_pix_clk_hz;
	u32 frame_rate, max_fps, frame_valid_us;

	BUG_ON(!pll_info_compact);

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
	max_fps = (vt_pix_clk_hz * 10) / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	max_fps = (max_fps % 10 >= 5 ? frame_rate + 1 : frame_rate);

	cis_data->pclk = vt_pix_clk_hz;
	cis_data->max_fps = max_fps;
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
	cis_data->min_fine_integration_time = SENSOR_2X5_FINE_INTEGRATION_TIME_MIN;
	cis_data->max_fine_integration_time = SENSOR_2X5_FINE_INTEGRATION_TIME_MAX;
	cis_data->min_coarse_integration_time = SENSOR_2X5_COARSE_INTEGRATION_TIME_MIN;
}

#ifdef SENSOR_2X5_CAL_UPLOAD
int sensor_2x5_cis_write_cal(struct v4l2_subdev *subdev)
{
	int i, ret = 0;
	struct is_cis *cis = NULL;
	const int position = SENSOR_POSITION_FRONT;
	u16 cal_page_xtalk, cal_page_lsc, cal_offset_xtalk, cal_offset_lsc;
	u16 *xtalk_buf = NULL, *lsc_buf = NULL;
	char *cal_buf = NULL;
#ifdef CONFIG_CAMERA_VENDOR_MCD_V2
	static struct is_rom_info *finfo = NULL;
#else
	err("write cal not implemented!");
	return -ENOSYS;
#endif

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		return -EINVAL;
	}

#ifdef CONFIG_CAMERA_VENDOR_MCD_V2
	is_sec_get_sysfs_finfo_by_position(position, &finfo);
	is_sec_get_cal_buf(position, &cal_buf);
	dbg_sensor(1, "%s: sensor cal start addr1 0x%X, end addr1 0x%X, addr2 0x%X, end addr2 0x%X\n",
		__func__, finfo->sensor_cal_data_start_addr, finfo->sensor_cal_data_end_addr,
		*((u32 *)&cal_buf[0x18]), *((u32 *)&cal_buf[0x1C]));
#endif
	xtalk_buf = (u16 *)&cal_buf[finfo->sensor_cal_data_start_addr];
	lsc_buf = (u16 *)&cal_buf[finfo->sensor_cal_data_start_addr + SENSOR_2X5_CAL_XTALK_SIZE];

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_cis_set_registers(subdev, sensor_2x5_precal, sensor_2x5_precal_size);
	if (ret < 0) {
		err("set_registers(precal) fail!!");
		goto p_err;
	}
	dbg_sensor(1, "[set_setfile]: sensor_2x5_precal\n");

#ifdef SUPPORT_2X5_SENSOR_VARIATION
	if (last_version == false) {
		cal_page_xtalk = SENSOR_2X5_EVT0_XTALK_ADDR_PAGE;
		cal_offset_xtalk = SENSOR_2X5_EVT0_XTALK_ADDR_OFFSET;
		cal_page_lsc = SENSOR_2X5_EVT0_LSC_ADDR_PAGE;
		cal_offset_lsc = SENSOR_2X5_EVT0_LSC_ADDR_OFFSET;
	} else
#endif
	{
		cal_page_xtalk = SENSOR_2X5_XTALK_ADDR_PAGE;
		cal_offset_xtalk = SENSOR_2X5_XTALK_ADDR_OFFSET;
		cal_page_lsc = SENSOR_2X5_LSC_ADDR_PAGE;
		cal_offset_lsc = SENSOR_2X5_LSC_ADDR_OFFSET;
	}

	/* Write LSC */
	dbg_sensor(1, "%s: Write Cal LSC\n", __func__);
	ret = cis->ixc_ops->write16(cis->client, SENSOR_2X5_W_DIR_PAGE, cal_page_lsc);
	/*dbg_sensor(1, "[   lsc] 0x%X, 0x%X\n", SENSOR_2X5_W_DIR_PAGE, cal_page_lsc); */
	if (ret < 0) {
		err("[lsc] i2c fail addr(%x, %x) ,ret = %d\n", SENSOR_2X5_W_DIR_PAGE, cal_page_lsc, ret);
		goto p_err;
	}
	for (i = 0; i < SENSOR_2X5_CAL_LSC_SIZE / 2; i++) {
		ret = cis->ixc_ops->write16(cis->client, cal_offset_lsc + (i * 2), cpu_to_be16(lsc_buf[i]));
		/*dbg_sensor(2, "[   lsc] %4d: 0x%X, 0x%04X\n", i, cal_offset_lsc + (i * 2), cpu_to_be16(lsc_buf[i])); */
		if (ret < 0) {
			err("[lsc] i2c fail addr(%x), val(%04x), ret = %d\n",
				cal_offset_lsc + (i * 2), cpu_to_be16(lsc_buf[i]), ret);
			goto p_err;
		}
	}

	/* Write X-talk */
	dbg_sensor(1, "%s: Write Cal Xtalk\n", __func__);
	for (i = 0; i < SENSOR_2X5_CAL_XTALK_SIZE / 2; i++) {
		ret = cis->ixc_ops->write16(cis->client, cal_offset_xtalk + (i * 2), cpu_to_be16(xtalk_buf[i]));
		/*dbg_sensor(2, "[xtalk] %4d: 0x%X, 0x%04X\n", i, cal_offset_xtalk + (i * 2), cpu_to_be16(xtalk_buf[i])); */
		if (ret < 0) {
			err("[xtalk] i2c fail addr(%x), val(%04x), ret = %d\n",
				cal_offset_xtalk + (i * 2), cpu_to_be16(xtalk_buf[i]), ret);
			goto p_err;
		}
	}

	ret = sensor_cis_set_registers(subdev, sensor_2x5_postcal, sensor_2x5_postcal_size);
	if (ret < 0) {
		err("set_registers(postcal) fail!!");
		goto p_err;
	}
	dbg_sensor(1, "[set_setfile]: sensor_2x5_postcal\n");

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	info("[2x5] sensor cal done!\n");

	return 0;

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}
#endif

/* CIS OPS */
int sensor_2x5_cis_init(struct v4l2_subdev *subdev)
{
#ifdef SUPPORT_2X5_SENSOR_VARIATION
#ifdef CONFIG_CAMERA_VENDOR_MCD_V2
	static struct is_rom_info *finfo = NULL;
#endif
	u8 version_id = 0;
#endif
	struct is_cis *cis;
	u32 setfile_index = 0;
	cis_setting_info setinfo = {NULL, 0};
	int ret = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	BUG_ON(!cis->cis_data);
	memset(cis->cis_data, 0, sizeof(cis_shared_data));
	cis->rev_flag = false;

	ret = sensor_cis_check_rev(cis);
	if (ret < 0) {
		warn("sensor_2x5_check_rev is fail when cis init");
		cis->rev_flag = true;
		ret = 0;
	}

	cis->cis_data->cur_width = SENSOR_2X5_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_2X5_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->need_mode_change = false;
	cal_done = false;

#ifdef SUPPORT_2X5_SENSOR_VARIATION
#ifdef CONFIG_CAMERA_VENDOR_MCD_V2
	is_sec_get_sysfs_finfo_by_position(SENSOR_POSITION_FRONT, &finfo);
	version_id = finfo->rom_sensor_id[8];
#endif
	if (version_id == SENSOR_2X5_VER_SP13) {
		last_version = true;
		info("%s : EVT1 setfile_B\n", __func__);
		sensor_2x5_global = sensor_2x5_setfile_B_Global;
		sensor_2x5_global_size = ARRAY_SIZE(sensor_2x5_setfile_B_Global);
		sensor_2x5_setfiles = sensor_2x5_setfiles_B;
		sensor_2x5_setfile_sizes = sensor_2x5_setfile_B_sizes;
		sensor_2x5_max_setfile_num = ARRAY_SIZE(sensor_2x5_setfiles_B);
		sensor_2x5_precal = sensor_2x5_setfile_B_pre_cal;
		sensor_2x5_precal_size = ARRAY_SIZE(sensor_2x5_setfile_B_pre_cal);
		sensor_2x5_postcal = sensor_2x5_setfile_B_post_cal;
		sensor_2x5_postcal_size = ARRAY_SIZE(sensor_2x5_setfile_B_post_cal);
		sensor_2x5_pllinfos = sensor_2x5_pllinfos_B;

	} else if (version_id == SENSOR_2X5_VER_SP03) {
		last_version = false;
		info("%s : EVT0 setfile_A\n", __func__);
		sensor_2x5_global = sensor_2x5_setfile_A_Global;
		sensor_2x5_global_size = ARRAY_SIZE(sensor_2x5_setfile_A_Global);
		sensor_2x5_setfiles = sensor_2x5_setfiles_A;
		sensor_2x5_setfile_sizes = sensor_2x5_setfile_A_sizes;
		sensor_2x5_max_setfile_num = ARRAY_SIZE(sensor_2x5_setfiles_A);
		sensor_2x5_precal = sensor_2x5_setfile_A_pre_cal;
		sensor_2x5_precal_size = ARRAY_SIZE(sensor_2x5_setfile_A_pre_cal);
		sensor_2x5_postcal = sensor_2x5_setfile_A_post_cal;
		sensor_2x5_postcal_size = ARRAY_SIZE(sensor_2x5_setfile_A_post_cal);
		sensor_2x5_tetra_isp = sensor_2x5_tetra_isp_A;
		sensor_2x5_tetra_isp_sizes = sensor_2x5_tetra_isp_A_sizes;
		sensor_2x5_pllinfos = sensor_2x5_pllinfos_A;
	} else
#endif /* SUPPORT_2X5_SENSOR_VARIATION */
	{
		/* We assume last version if we couldn't get sersor version info */
		last_version = true;
#ifdef SUPPORT_2X5_SENSOR_VARIATION
		err("invalid version ID 0x%02X. Check module!!\n", __func__, version_id);
#endif
		info("%s : EVT1 setfile_B\n", __func__);
		sensor_2x5_global = sensor_2x5_setfile_B_Global;
		sensor_2x5_global_size = ARRAY_SIZE(sensor_2x5_setfile_B_Global);
		sensor_2x5_setfiles = sensor_2x5_setfiles_B;
		sensor_2x5_setfile_sizes = sensor_2x5_setfile_B_sizes;
		sensor_2x5_max_setfile_num = ARRAY_SIZE(sensor_2x5_setfiles_B);
		sensor_2x5_precal = sensor_2x5_setfile_B_pre_cal;
		sensor_2x5_precal_size = ARRAY_SIZE(sensor_2x5_setfile_B_pre_cal);
		sensor_2x5_postcal = sensor_2x5_setfile_B_post_cal;
		sensor_2x5_postcal_size = ARRAY_SIZE(sensor_2x5_setfile_B_post_cal);
		sensor_2x5_pllinfos = sensor_2x5_pllinfos_B;
	}

	sensor_2x5_cis_data_calculation(sensor_2x5_pllinfos[setfile_index], cis->cis_data);

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

int sensor_2x5_cis_log_status(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	u8 data8 = 0;
	u16 data16 = 0;

	BUG_ON(!subdev);

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

	pr_err("[SEN:DUMP] *******************************\n");
	ret = cis->ixc_ops->read16(cis->client, 0x0000, &data16);
	if (unlikely(!ret))
		printk("[SEN:DUMP] model_id(%x)\n", data16);
	ret = cis->ixc_ops->read8(cis->client, 0x0002, &data8);
	if (unlikely(!ret))
		printk("[SEN:DUMP] revision_number(%x)\n", data8);
	ret = cis->ixc_ops->read8(cis->client, 0x0005, &data8);
	if (unlikely(!ret))
		printk("[SEN:DUMP] frame_count(%x)\n", data8);
	ret = cis->ixc_ops->read8(cis->client, 0x0100, &data8);
	if (unlikely(!ret))
		printk("[SEN:DUMP] mode_select(%x)\n", data8);

	sensor_cis_dump_registers(subdev, sensor_2x5_setfiles[0], sensor_2x5_setfile_sizes[0]);
	pr_err("[SEN:DUMP] *******************************\n");

p_err:
	return ret;
}

#if USE_GROUP_PARAM_HOLD
static int sensor_2x5_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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
static inline int sensor_2x5_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{ return 0; }
#endif

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_2x5_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	ret = sensor_2x5_cis_group_param_hold_func(subdev, hold);
	if (ret < 0)
		goto p_err;

p_err:
	return ret;
}

int sensor_2x5_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	BUG_ON(!cis);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	/* setfile global setting is at camera entrance */
	ret = sensor_cis_set_registers(subdev, sensor_2x5_global, sensor_2x5_global_size);
	if (ret < 0) {
		err("sensor_2x5_set_registers(global) fail!!");
		goto p_err;
	}
	dbg_sensor(1, "[set_setfile]: sensor_2x5_global\n");

	dbg_sensor(1, "[%s] global setting done\n", __func__);

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_2x5_cis_mode_change_evt0(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	int isp_mode = TETRA_ISP_24MP; /* default 24MP, for SP03 version */
	struct is_cis *cis = NULL;

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	if (mode > sensor_2x5_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return -EINVAL;
	} else if (mode < SENSOR_2X5_MODE_REMOSAIC_START) {
		isp_mode = TETRA_ISP_6MP;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_2x5_check_rev is fail");
			return ret;
		}
	}

	/* DSLIM: Check below */
	sensor_2x5_cis_data_calculation(sensor_2x5_pllinfos[mode], cis->cis_data);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = sensor_cis_set_registers(subdev, sensor_2x5_setfiles[mode], sensor_2x5_setfile_sizes[mode]);
	if (ret < 0) {
		err("sensor_2x5_set_registers fail!!");
		goto p_err;
	}
	dbg_sensor(1, "[set_setfile]: sensor_2x5_setfiles mode\n", mode);

	/* Setting Tetra ISP in old revision */
	info("%s: Tetra ISP mode %d (EVT0)\n", __func__, isp_mode);
	ret = sensor_cis_set_registers(subdev, sensor_2x5_tetra_isp[isp_mode], sensor_2x5_tetra_isp_sizes[isp_mode]);
	if (ret < 0) {
		err("sensor_2x5_tetra_isp fail!!");
		goto p_err;
	}
	dbg_sensor(1, "[set_setfile]: sensor_2x5_tetra_isp %d\n", isp_mode);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	info("%s: mode changed(%d) EVT0\n", __func__, mode);
	/*dbg_sensor(1, "[%s] mode changed(%d)\n", __func__, mode);*/

	return 0;

p_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}


int sensor_2x5_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	bool cal = cal_done;

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (mode > sensor_2x5_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return -EINVAL;
	}

	/* If check_rev fail when cis_init, one more check_rev in mode_change */
	if (cis->rev_flag == true) {
		cis->rev_flag = false;
		ret = sensor_cis_check_rev(cis);
		if (ret < 0) {
			err("sensor_2x5_check_rev is fail");
			return ret;
		}
	}

	/* DSLIM: Check below */
	sensor_2x5_cis_data_calculation(sensor_2x5_pllinfos[mode], cis->cis_data);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	if (cal_done == true) {
		ret = cis->ixc_ops->write16(cis->client, SENSOR_2X5_W_DIR_PAGE, 0x4000);
		if (ret < 0) {
			err("i2c fail addr(%x, 0x4000) ,ret = %d\n", SENSOR_2X5_W_DIR_PAGE, ret);
			goto p_err_i2c;
		}
		ret = cis->ixc_ops->write16(cis->client, 0x6000, 0x0005);
		if (ret < 0) {
			err("i2c fail addr(0x6000, 0x0005) ,ret = %d\n", ret);
			goto p_err_i2c;
		}
	}

	ret = sensor_cis_set_registers(subdev, sensor_2x5_setfiles[mode], sensor_2x5_setfile_sizes[mode]);
	if (ret < 0) {
		err("sensor_2x5_set_registers fail!!");
		goto p_err_i2c;
	}
	dbg_sensor(1, "[set_setfile]: sensor_2x5_setfiles mode\n", mode);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

#ifdef SENSOR_2X5_CAL_UPLOAD
	if (cal_done == true) {
		IXC_MUTEX_LOCK(cis->ixc_lock);
		ret = cis->ixc_ops->write16(cis->client, SENSOR_2X5_W_DIR_PAGE, 0x4000);
		if (ret < 0) {
			err("i2c fail addr(%x, 0x4000) ,ret = %d\n", SENSOR_2X5_W_DIR_PAGE, ret);
			goto p_err_i2c;
		}
		ret = cis->ixc_ops->write16(cis->client, 0x6000, 0x0085);
		if (ret < 0) {
			err("i2c fail addr(0x6000, 0x0085) ,ret = %d\n", ret);
			goto p_err_i2c;
		}
		IXC_MUTEX_UNLOCK(cis->ixc_lock);
	} else {
		/* Write Sensor cal */
		ret = sensor_2x5_cis_write_cal(subdev);
		if (ret < 0) {
			err("set_registers(cal) fail!!");
			goto p_err;
		}
		cal_done = true;
	}
#endif /* SENSOR_2X5_CAL_UPLOAD */

	info("%s: mode changed(%d)%s\n", __func__, mode, (cal != cal_done) ? " with CAL": "");
	/*dbg_sensor(1, "[%s] mode changed(%d)\n", __func__, mode);*/

	return 0;

p_err_i2c:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}

int sensor_2x5_cis_mode_change_wrap(struct v4l2_subdev *subdev, u32 mode)
{
	if (last_version)
		return sensor_2x5_cis_mode_change(subdev, mode);
	else
		return sensor_2x5_cis_mode_change_evt0(subdev, mode); /* for EVT0 */
}

/* TODO: Sensor set size sequence(sensor done, sensor stop, 3AA done in FW case */
int sensor_2x5_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
{
	int ret = 0;

	err("not implemented!!");

	return ret;
}

int sensor_2x5_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	info("[2x5] start\n");
	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("[%s] sensor_2x5_cis_group_param_hold_func fail\n", __func__);

#ifdef DEBUG_2X5_PLL
	{
	u16 pll;
	ret = cis->ixc_ops->read16(cis->client, 0x0300, &pll);
	dbg_sensor(1, "______ vt_pix_clk_div(%x)\n", pll);
	ret = cis->ixc_ops->read16(cis->client, 0x0302, &pll);
	dbg_sensor(1, "______ vt_sys_clk_div(%x)\n", pll);
	ret = cis->ixc_ops->read16(cis->client, 0x0304, &pll);
	dbg_sensor(1, "______ pre_pll_clk_div(%x)\n", pll);
	ret = cis->ixc_ops->read16(cis->client, 0x0306, &pll);
	dbg_sensor(1, "______ pll_multiplier(%x)\n", pll);
	ret = cis->ixc_ops->read16(cis->client, 0x0308, &pll);
	dbg_sensor(1, "______ op_pix_clk_div(%x)\n", pll);
	ret = cis->ixc_ops->read16(cis->client, 0x030a, &pll);
	dbg_sensor(1, "______ op_sys_clk_div(%x)\n", pll);

	ret = cis->ixc_ops->read16(cis->client, 0x030c, &pll);
	dbg_sensor(1, "______ secnd_pre_pll_clk_div(%x)\n", pll);
	ret = cis->ixc_ops->read16(cis->client, 0x030e, &pll);
	dbg_sensor(1, "______ secnd_pll_multiplier(%x)\n", pll);
	ret = cis->ixc_ops->read16(cis->client, 0x0340, &pll);
	dbg_sensor(1, "______ frame_length_lines(%x)\n", pll);
	ret = cis->ixc_ops->read16(cis->client, 0x0342, &pll);
	dbg_sensor(1, "______ line_length_pck(%x)\n", pll);
	}
#endif

	/* Sensor stream on */
	ret = cis->ixc_ops->write16(cis->client, 0x0100, 0x0100);
	if (ret < 0)
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", 0x0100, 0x0100, ret);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	cis_data->stream_on = true;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_2x5_cis_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	/*dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);*/
	info("[2x5] stop\n");

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("[%s] sensor_2x5_cis_group_param_hold_func fail\n", __func__);

	/* Sensor stream off */
	ret |= cis->ixc_ops->write16(cis->client, 0x0100, 0x0000);
	if (ret < 0)
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", 0x0100, 0x00, ret);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	cis_data->stream_on = false;

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_2x5_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
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

	BUG_ON(!subdev);
	BUG_ON(!target_exposure);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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
	hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/* Short exposure */
	ret = cis->ixc_ops->write16(cis->client, 0x0202, short_coarse_int);
	if (ret < 0)
		goto p_err;

#if defined(USE_SENSOR_WDR)
	/* Long exposure */
	if (is_vendor_wdr_mode_on(cis_data)) {
		ret = cis->ixc_ops->write16(cis->client, 0x021E, long_coarse_int);
		if (ret < 0)
			goto p_err;
	}
#endif

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
		hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_2x5_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
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

	BUG_ON(!subdev);
	BUG_ON(!min_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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

int sensor_2x5_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
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

	BUG_ON(!subdev);
	BUG_ON(!max_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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

int sensor_2x5_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
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

	BUG_ON(!subdev);
	BUG_ON(!target_duration);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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

int sensor_2x5_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;
	u16 frame_length_lines = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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
	hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x01);
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
		hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_2x5_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 frame_duration = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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

	ret = sensor_2x5_cis_set_frame_duration(subdev, frame_duration);
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

int sensor_2x5_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 again_code = 0;
	u32 again_permile = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!target_permile);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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

int sensor_2x5_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 analog_gain = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);

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
	hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x01);
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
		hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_2x5_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 analog_gain = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x01);
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
		hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_2x5_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 read_value = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!min_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = cis->ixc_ops->read16(cis->client, 0x0084, &read_value);
	if (ret < 0)
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", 0x0084, read_value, ret);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->min_analog_gain[0] = read_value;
	cis_data->min_analog_gain[1] = sensor_cis_calc_again_permile(read_value);

	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_analog_gain[0], cis_data->min_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_2x5_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 read_value = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!max_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = cis->ixc_ops->read16(cis->client, 0x0086, &read_value);
	if (ret < 0)
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", 0x0086, read_value, ret);

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	cis_data->max_analog_gain[0] = read_value;
	cis_data->max_analog_gain[1] = sensor_cis_calc_again_permile(read_value);

	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_analog_gain[0], cis_data->max_analog_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	return ret;
}

int sensor_2x5_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u16 long_gain = 0;
	u16 short_gain = 0;
	u16 dgains[4] = {0};
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

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
			cis->id, __func__, cis->cis_data->sen_vsync_count, dgain->long_val, dgain->short_val, long_gain, short_gain);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/* Short digital gain */
	dgains[0] = dgains[1] = dgains[2] = dgains[3] = short_gain;
	ret = cis->ixc_ops->write16(cis->client, 0x020E, short_gain);
	if (ret < 0)
		goto p_err;

	/* Long digital gain */
	if (false /* is_vendor_wdr_mode_on(cis_data)*/) {
		dgains[0] = dgains[1] = dgains[2] = dgains[3] = long_gain;
		ret = cis->ixc_ops->write16(cis->client, 0x0C82, long_gain);
		if (ret < 0)
			goto p_err;
	}

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

p_err:
	if (hold > 0) {
		hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_2x5_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
{
	int ret = 0;
	int hold = 0;
	struct is_cis *cis;
	u16 digital_gain = 0;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x01);
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
		hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}

int sensor_2x5_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!min_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	/* 2X5 cannot read min/max digital gain */
	cis_data->min_digital_gain[0] = 0x0100;

	cis_data->min_digital_gain[1] = 1000;

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->min_digital_gain[0], cis_data->min_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_2x5_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	ktime_t st = ktime_get();

	BUG_ON(!subdev);
	BUG_ON(!max_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	BUG_ON(!cis);
	BUG_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	/* 2X5 cannot read min/max digital gain */
	cis_data->max_digital_gain[0] = 0x1000;

	cis_data->max_digital_gain[1] = 16000;

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__,
		cis_data->max_digital_gain[0], cis_data->max_digital_gain[1]);

	if (IS_ENABLED(DEBUG_SENSOR_TIME))
		dbg_sensor(1, "[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));

	return ret;
}

int sensor_2x5_cis_set_wb_gain(struct v4l2_subdev *subdev, struct wb_gains wb_gains)
{
	const u16 addr_page = SENSOR_2X5_ABS_GAIN_PAGE;
	const u16 addr_offset = SENSOR_2X5_ABS_GAIN_OFFSET;
	struct is_cis *cis;
	int i, ret = 0;
	int hold = 0;
	int mode = 0;
	u16 abs_gains[3] = {0, }; /* R, G, B */
	u32 avg_g = 0, div = 0;
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

	if (!cis->use_wb_gain || !last_version)
		return 0;

	mode = cis->cis_data->sens_config_index_cur;

	if ((mode < SENSOR_2X5_MODE_REMOSAIC_START) || (mode >= SENSOR_2X5_MODE_3DHDR_START))
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

	dbg_sensor(1, "[SEN:%d]%s: DDK vlaue: wb_gain_gr(%d), wb_gain_r(%d), wb_gain_b(%d), wb_gain_gb(%d)\n",
		cis->id, __func__, wb_gains.gr, wb_gains.r, wb_gains.b, wb_gains.gb);

	avg_g = (wb_gains.gr + wb_gains.gb) / 2;
	abs_gains[0] = (u16)((wb_gains.r / div) & 0xFFFF);
	abs_gains[1] = (u16)((avg_g / div) & 0xFFFF);
	abs_gains[2] = (u16)((wb_gains.b / div) & 0xFFFF);

	dbg_sensor(1, "[SEN:%d]%s: abs_gain_r(0x%4X), abs_gain_g(0x%4X), abs_gain_b(0x%4X)\n",
		cis->id, __func__, abs_gains[0], abs_gains[1], abs_gains[2]);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = cis->ixc_ops->write16(cis->client, SENSOR_2X5_W_DIR_PAGE, addr_page);
	if (ret < 0) {
		err("i2c fail addr(%x, %x) ,ret = %d\n", SENSOR_2X5_W_DIR_PAGE, addr_page, ret);
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
		hold = sensor_2x5_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	return ret;
}


static struct is_cis_ops cis_ops = {
	.cis_init = sensor_2x5_cis_init,
	.cis_log_status = sensor_2x5_cis_log_status,
	.cis_group_param_hold = sensor_2x5_cis_group_param_hold,
	.cis_set_global_setting = sensor_2x5_cis_set_global_setting,
	.cis_mode_change = sensor_2x5_cis_mode_change_wrap,
	.cis_set_size = sensor_2x5_cis_set_size,
	.cis_stream_on = sensor_2x5_cis_stream_on,
	.cis_stream_off = sensor_2x5_cis_stream_off,
	.cis_set_exposure_time = sensor_2x5_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_2x5_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_2x5_cis_get_max_exposure_time,
	.cis_adjust_frame_duration = sensor_2x5_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_2x5_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_2x5_cis_set_frame_rate,
	.cis_adjust_analog_gain = sensor_2x5_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_2x5_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_2x5_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_2x5_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_2x5_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_2x5_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_2x5_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_2x5_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_2x5_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_cis_compensate_gain_for_extremely_br,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_wait_streamon = sensor_cis_wait_streamon,
	.cis_set_wb_gains = sensor_2x5_cis_set_wb_gain,
};

int cis_2x5_probe_i2c(struct i2c_client *client,
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
	cis->use_pdaf = false; /* cis->use_pdaf =  use_pdaf */

	ret = of_property_read_string(dnode, "setfile", &setfile);
	if (ret) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

	probe_info("%s done\n", __func__);

	return ret;
}

static int cis_2x5_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

static const struct of_device_id exynos_is_cis_2x5_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-cis-2x5",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_is_cis_2x5_match);

static const struct i2c_device_id cis_2x5_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver cis_2x5_driver = {
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_is_cis_2x5_match
	},
	.probe	= cis_2x5_probe_i2c,
	.remove	= cis_2x5_remove,
	.id_table = cis_2x5_idt
};
module_i2c_driver(cis_2x5_driver);

MODULE_LICENSE("GPL");
