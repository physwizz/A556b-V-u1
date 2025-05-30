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
#include <linux/of_platform.h>
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
#include "is-helper-ixc.h"
#include "is-cis.h"
#include "is-device-csi.h"

#define IS_ENABLED_SEAMLESS_LOG(cis) ({ \
	(cis->sensor_info->use_debug_seamless_control) \
	&& ((cis->cis_data->seamless_update_vsync_cnt == cis->cis_data->sen_vsync_count) \
	|| (cis->cis_data->seamless_update_vsync_cnt + 1 == cis->cis_data->sen_vsync_count) \
	|| (cis->cis_data->seamless_update_vsync_cnt + 2 == cis->cis_data->sen_vsync_count)); \
})

#define IS_ENABLED_PRINT_CONTROL_LOG(cis) ({ \
	IS_ENABLED_SEAMLESS_LOG(cis) \
	|| (cis->cis_data->use_print_control_log); \
})

extern int sensor_module_power_reset(struct v4l2_subdev *subdev, struct is_device_sensor *device);

u32 sensor_cis_do_div64(u64 num, u32 den) {
	u64 res = 0;

	if (den != 0) {
		res = num;
		do_div(res, den);
	} else {
		err("Divide by zero!!!\n");
		WARN_ON(1);
	}

	return (u32)res;
}
EXPORT_SYMBOL_GPL(sensor_cis_do_div64);

int sensor_cis_write_registers_locked(struct v4l2_subdev *subdev, const struct sensor_regs regset)
{
	int ret = 0;
	struct is_cis *cis;

	cis = sensor_cis_get_cis(subdev);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = sensor_cis_write_registers(subdev, regset);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_write_registers_locked);

int sensor_cis_write_registers(struct v4l2_subdev *subdev, const struct sensor_regs regset)
{
	int ret = 0;
	int i = 0, j = 0;
	int delay_time = 0;
	struct is_cis *cis;
	int index_str = 0, index_next = 0;
	int start_data = 0;
	int burst_num = 1;
	u8 *burst_data;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		return -EINVAL;
	}

	if (unlikely(!cis->client)) {
		err("client is NULL");
		return -EINVAL;
	}

	dbg_sensor(1, "[%s][%d] sensor setting start\n", __func__, cis->cis_data->sen_vsync_count);
	cis->stream_state = CIS_STREAM_INIT;

	for (i = 0; i < regset.size; i += IXC_NEXT) {
		switch (regset.regs[i + IXC_BYTE]) {
		case IXC_MODE_BURST_ADDR_16BIT:
			index_str = i;
			break;

		case IXC_MODE_BURST_DATA_16BIT:
			index_next = i + IXC_NEXT + IXC_BYTE;
			if ((index_next < regset.size) && (regset.regs[index_next] == IXC_MODE_BURST_DATA_16BIT)) {
				burst_num++;
				break;
			}

			burst_data = vmalloc(burst_num * 2);
			start_data = index_str + IXC_NEXT;
			for (j = 0; j < burst_num; j++) {
				burst_data[j * 2] = (regset.regs[j * IXC_NEXT + start_data] & 0xFF00) >> 8;
				burst_data[j * 2 + 1] = (regset.regs[j * IXC_NEXT + start_data] & 0xFF);
			}
			ret = cis->ixc_ops->write8_sequential(cis->client,
				regset.regs[index_str], burst_data, burst_num * 2);

			if (ret < 0) {
				err("write8_sequential(burst) fail, ret(%d), addr(%#x), data(%#x)",
						ret, regset.regs[index_str], burst_data[0]);
				vfree(burst_data);
				goto p_err;
			}
			vfree(burst_data);
			burst_num = 1;
			break;

		case IXC_MODE_BURST_DATA_8BIT:
			index_next = i + IXC_NEXT + IXC_BYTE;
			if ((index_next < regset.size) && (regset.regs[index_next] == IXC_MODE_BURST_DATA_8BIT)) {
				burst_num++;
				break;
			}

			burst_data = vmalloc(burst_num);
			start_data = index_str + IXC_NEXT;
			for (j = 0; j < burst_num; j++) {
				burst_data[j] = regset.regs[j * IXC_NEXT + start_data];
			}
			ret = cis->ixc_ops->write8_sequential(cis->client,
				regset.regs[index_str], burst_data, burst_num);

			if (ret < 0) {
				err("write8_sequential(burst) fail, ret(%d), addr(%#x), data(%#x)",
						ret, regset.regs[index_str], burst_data[0]);
				vfree(burst_data);
				goto p_err;
			}
			vfree(burst_data);
			burst_num = 1;
			break;

		case IXC_MODE_DELAY_16BIT:
			delay_time = (regset.regs[i + IXC_ADDR] * 1000) + regset.regs[i + IXC_DATA];
			usleep_range(delay_time, delay_time + 10);
			break;

		default:
			if (regset.regs[i + IXC_BYTE] == 0x1) {
				ret = cis->ixc_ops->write8(cis->client,
					regset.regs[i + IXC_ADDR], regset.regs[i + IXC_DATA]);
				if (ret < 0) {
					err("is_sensor_write8 fail, ret(%d), addr(%#x), data(%#x)",
							ret, regset.regs[i + IXC_ADDR], regset.regs[i + IXC_DATA]);
					goto p_err;
				}
			} else if (regset.regs[i + IXC_BYTE] == 0x2) {
				ret = cis->ixc_ops->write16(cis->client,
					regset.regs[i + IXC_ADDR], regset.regs[i + IXC_DATA]);
				if (ret < 0) {
					err("is_sensor_write16 fail, ret(%d), addr(%#x), data(%#x)",
						ret, regset.regs[i + IXC_ADDR], regset.regs[i + IXC_DATA]);
					goto p_err;
				}
			}
		}
	}

#if (CIS_TEST_PATTERN_MODE != 0)
	ret = cis->ixc_ops->write8(cis->client, 0x0601, CIS_TEST_PATTERN_MODE);
#endif
	cis->stream_state = CIS_STREAM_SET_DONE;
	dbg_sensor(1, "[%s][%d] sensor setting done\n", __func__, cis->cis_data->sen_vsync_count);

p_err:
	if (ret) {
		if (cis)
			cis->stream_state = CIS_STREAM_SET_ERR;
		err("[%s] global/mode setting fail(%d)", __func__, ret);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_write_registers);

int sensor_cis_set_registers(struct v4l2_subdev *subdev, const u32 *regs, const u32 size)
{
	int ret = 0;
	int i = 0;
	struct is_cis *cis;
	int index_str = 0, index_next = 0;
	int burst_num = 1;
	u16 *addr_str = NULL;

	WARN_ON(!subdev);
	WARN_ON(!regs);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		return -EINVAL;
	}

	if (unlikely(!cis->client)) {
		err("client is NULL");
		return -EINVAL;
	}

	dbg_sensor(1, "[%s][%d] sensor setting start\n", __func__, cis->cis_data->sen_vsync_count);
	cis->stream_state = CIS_STREAM_INIT;

	for (i = 0; i < size; i += IXC_NEXT) {
		switch (regs[i + IXC_ADDR]) {
		case IXC_MODE_BURST_ADDR:
			index_str = i;
			break;
		case IXC_MODE_BURST_DATA:
			index_next = i + IXC_NEXT;
			if ((index_next < size) && (IXC_MODE_BURST_DATA == regs[index_next + IXC_ADDR])) {
				burst_num++;
				break;
			}

			addr_str = (u16 *)&regs[index_str + IXC_NEXT + IXC_DATA];
			ret = cis->ixc_ops->write16_burst(cis->client, regs[index_str + IXC_DATA], addr_str, burst_num);
			if (ret < 0) {
				err("is_sensor_write16_burst fail, ret(%d), addr(%#x), data(%#x)",
						ret, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
				goto p_err;
			}
			burst_num = 1;
			break;
		case IXC_MODE_DELAY:
			usleep_range(regs[i + IXC_DATA], regs[i + IXC_DATA] + 10);
			break;
		default:
			if (regs[i + IXC_BYTE] == 0x1) {
				ret = cis->ixc_ops->write8(cis->client, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
				if (ret < 0) {
					err("is_sensor_write8 fail, ret(%d), addr(%#x), data(%#x)",
							ret, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
					goto p_err;
				}
			} else if (regs[i + IXC_BYTE] == 0x2) {
				ret = cis->ixc_ops->write16(cis->client, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
				if (ret < 0) {
					err("is_sensor_write16 fail, ret(%d), addr(%#x), data(%#x)",
						ret, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
					goto p_err;
				}
			}
		}
	}

#if (CIS_TEST_PATTERN_MODE != 0)
	ret = cis->ixc_ops->write8(cis->client, 0x0601, CIS_TEST_PATTERN_MODE);
#endif
	cis->stream_state = CIS_STREAM_SET_DONE;
	dbg_sensor(1, "[%s][%d] sensor setting done\n", __func__, cis->cis_data->sen_vsync_count);

p_err:
	if (ret) {
		if (cis)
			cis->stream_state = CIS_STREAM_SET_ERR;
		err("[%s] global/mode setting fail(%d)", __func__, ret);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_registers);

int sensor_cis_set_registers_addr8(struct v4l2_subdev *subdev, const u32 *regs, const u32 size)
{
	int ret = 0;
	int i = 0;
	struct is_cis *cis;
	int index_str = 0, index_next = 0;
	int burst_num = 1;
	u16 *addr_str = NULL;

	WARN_ON(!subdev);
	WARN_ON(!regs);

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

	dbg_sensor(1, "[%s][%d] sensor setting start\n", __func__, cis->cis_data->sen_vsync_count);

	for (i = 0; i < size; i += IXC_NEXT) {
		switch (regs[i + IXC_ADDR]) {
		case IXC_MODE_BURST_ADDR:
			index_str = i;
			break;
		case IXC_MODE_BURST_DATA:
			index_next = i + IXC_NEXT;
			if ((index_next < size) && (IXC_MODE_BURST_DATA == regs[index_next + IXC_ADDR])) {
				burst_num++;
				break;
			}

			addr_str = (u16 *)&regs[index_str + IXC_NEXT + IXC_DATA];
			ret = cis->ixc_ops->write16_burst(cis->client, regs[index_str + IXC_DATA], addr_str, burst_num);
			if (ret < 0) {
				err("is_sensor_write16_burst fail, ret(%d), addr(%#x), data(%#x)",
						ret, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
			}
			burst_num = 1;
			break;
		case IXC_MODE_DELAY:
			usleep_range(regs[i + IXC_DATA], regs[i + IXC_DATA] + 10);
			break;
		default:
			if (regs[i + IXC_BYTE] == 0x1) {
				ret = cis->ixc_ops->addr8_write8(cis->client, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
				if (ret < 0) {
					err("is_sensor_write8 fail, ret(%d), addr(%#x), data(%#x)",
							ret, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
				}
			} else if (regs[i + IXC_BYTE] == 0x2) {
				ret = cis->ixc_ops->write16(cis->client, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
				if (ret < 0) {
					err("is_sensor_write16 fail, ret(%d), addr(%#x), data(%#x)",
						ret, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
				}
			}
		}
	}

#if (CIS_TEST_PATTERN_MODE != 0)
	ret = cis->ixc_ops->write8(cis->client, 0x0601, CIS_TEST_PATTERN_MODE);
#endif

	dbg_sensor(1, "[%s][%d] sensor setting done\n", __func__, cis->cis_data->sen_vsync_count);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_registers_addr8);

struct is_cis *sensor_cis_get_cis(struct v4l2_subdev *subdev)
{
	struct is_cis *cis;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);
	WARN_ON(!cis->client);

	return cis;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_cis);

void sensor_cis_set_retention_mode(struct v4l2_subdev *subdev, u32 retention_mode)
{
	struct is_cis *cis;
	struct is_module_enum *module;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;

	cis = sensor_cis_get_cis(subdev);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	WARN_ON(!sensor_peri);

	module = sensor_peri->module;
	WARN_ON(!module);

	ext_info = &module->ext;
	WARN_ON(!ext_info);

	ext_info->use_retention_mode = retention_mode;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_retention_mode);

u32 sensor_cis_get_retention_mode(struct v4l2_subdev *subdev)
{
	struct is_cis *cis;
	struct is_module_enum *module;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;

	cis = sensor_cis_get_cis(subdev);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	WARN_ON(!sensor_peri);

	module = sensor_peri->module;
	WARN_ON(!module);

	ext_info = &module->ext;
	WARN_ON(!ext_info);

	return ext_info->use_retention_mode;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_retention_mode);

bool sensor_cis_support_retention_mode(struct v4l2_subdev *subdev)
{
	return (sensor_cis_get_retention_mode(subdev) != SENSOR_RETENTION_UNSUPPORTED);
}
EXPORT_SYMBOL_GPL(sensor_cis_support_retention_mode);

int sensor_cis_check_rev_on_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
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

	ret = sensor_cis_check_rev(cis);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_check_rev_on_init);

int sensor_cis_check_rev(struct is_cis *cis)
{
	int ret = 0;
	u8 rev8 = 0;
	u16 rev16 = 0;
	int i;
	u32 wait_cnt = 0, time_out_cnt = 5;

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	do {
		IXC_MUTEX_LOCK(cis->ixc_lock);

		if (cis->rev_byte == 2) { // 2 BYTE read
			ret = cis->ixc_ops->read16(cis->client, cis->rev_addr, &rev16);
			cis->cis_data->cis_rev = rev16;
		} else {
			ret = cis->ixc_ops->read8(cis->client, cis->rev_addr, &rev8);	//default
			cis->cis_data->cis_rev = rev8;
		}

		IXC_MUTEX_UNLOCK(cis->ixc_lock);

		if (ret < 0) {
			usleep_range(10000, 10010);
			wait_cnt++;
			if (wait_cnt >= time_out_cnt) {
				err("[MOD:D:%d] is_sensor_read failed wait_cnt(%d) > time_out_cnt(%d)", cis->id, wait_cnt, time_out_cnt);
				ret = -EAGAIN;
				goto p_err;
			}
		}
	} while (ret < 0);

	if (cis->rev_valid_count) {
		for (i = 0; i < cis->rev_valid_count; i++) {
			if (cis->cis_data->cis_rev == cis->rev_valid_values[i]) {
				info("%s : [%d] Sensor version. Rev(addr:0x%X). 0x%X\n",
					__func__, cis->id, cis->rev_addr, cis->cis_data->cis_rev);
				break;
			}
		}

		if (i == cis->rev_valid_count) {
			info("%s : [%d] Wrong sensor version. Rev(addr:0x%X). 0x%X\n",
				__func__, cis->id, cis->rev_addr, cis->cis_data->cis_rev);
#if defined(USE_CAMERA_CHECK_SENSOR_REV)
			ret = -EINVAL;
#endif
		}
	} else {
		info("%s : [%d] Skip rev checking. Rev. 0x%X\n",
			__func__, cis->id, cis->cis_data->cis_rev);
	}

	if (cis->sensor_info && cis->sensor_info->version)
		info("[%d] Sensor setting version : %s\n", cis->id, cis->sensor_info->version);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_check_rev);

u32 sensor_cis_calc_again_code(u32 permile)
{
	return (permile * 32 + 500) / 1000;
}
EXPORT_SYMBOL_GPL(sensor_cis_calc_again_code);

u32 sensor_cis_calc_again_permile(u32 code)
{
	return (code * 1000 + 16) / 32;
}
EXPORT_SYMBOL_GPL(sensor_cis_calc_again_permile);

u32 sensor_cis_calc_dgain_code(u32 permile)
{
	u8 buf[2] = {0, 0};
	buf[0] = permile / 1000;
	buf[1] = (((permile - (buf[0] * 1000)) * 256) / 1000);

	return (buf[0] << 8 | buf[1]);
}
EXPORT_SYMBOL_GPL(sensor_cis_calc_dgain_code);

u32 sensor_cis_calc_dgain_permile(u32 code)
{
	return (((code & 0xFF00) >> 8) * 1000) + ((code & 0xFF) * 1000 / 256);
}
EXPORT_SYMBOL_GPL(sensor_cis_calc_dgain_permile);

void sensor_cis_data_calculation(struct v4l2_subdev *subdev, u32 mode)
{
	struct is_cis *cis;
	const struct sensor_cis_mode_info *mode_info;
	cis_shared_data *cis_data;
	struct is_device_sensor_peri *sensor_peri = NULL;
	u64 pclk_hz;
	u32 frame_rate, max_fps, frame_valid_us;
	u16 fll, llp, lli;
	u32 fll_x_llp;
	u32 min_exp = 0, max_exp = 0;
	u32 min_again = 0, max_again = 0;
	u32 min_dgain = 0, max_dgain = 0;
	u32 mclk_freq_khz;
	u16 readout_factor;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	if (mode >= cis->sensor_info->mode_count) {
		err("invalid mode(%d)!", mode);
		return;
	}

	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	WARN_ON(!sensor_peri);

	cis_data = cis->cis_data;
	mode_info = cis->sensor_info->mode_infos[mode];

	/* get pclk value from pll info */
	pclk_hz = mode_info->pclk;
	fll = mode_info->frame_length_lines;
	llp = mode_info->line_length_pck;
	lli = mode_info->line_length_inck;
	mclk_freq_khz = sensor_peri->module->pdata->mclk_freq_khz;
	fll_x_llp = fll * llp;

	/* the time of processing one frame calculation (us) */
	cis_data->min_frame_us_time = (u64)fll_x_llp * 1000 * 1000 / pclk_hz;

	/* if requested fps is lower than selected fps, adjust min_frame_us_time */
	if (cis_data->request_min_frame_us_time > cis_data->min_frame_us_time) {
		dbg_sensor(1, "%s : request_min_frame_us_time(%d), min_frame_us_time(%d)\n",
				__func__, cis_data->request_min_frame_us_time, cis_data->min_frame_us_time);
		cis_data->min_frame_us_time = cis_data->request_min_frame_us_time;
	}

	cis_data->base_min_frame_us_time = cis_data->min_frame_us_time;
	cis_data->cur_frame_us_time = cis_data->min_frame_us_time;

	frame_rate = pclk_hz / fll_x_llp;
	max_fps = frame_rate;

	/* calculate max fps with rounding */
	if (((pclk_hz * 10) / fll_x_llp) % 10 >= 5)
		max_fps++;

	cis_data->pclk = pclk_hz;
	cis_data->max_fps = max_fps;
	cis_data->pre_frame_length_lines = cis_data->frame_length_lines;
	cis_data->frame_length_lines = fll;
	cis_data->pre_line_length_pck = cis_data->line_length_pck;
	cis_data->line_length_pck = llp;
	cis_data->line_length_inck = lli;

	if (cis->sensor_info->readout_factor > 1)
		readout_factor = cis->sensor_info->readout_factor;
	else
		readout_factor = 1;

	if (lli > 0)
		cis_data->line_readOut_time = (u64)lli * 1000 * 1000  / mclk_freq_khz;
	else
		cis_data->line_readOut_time = (u64)llp * readout_factor * 1000 * 1000 * 1000 / pclk_hz;

	frame_valid_us = (u64)cis_data->cur_height * cis_data->line_readOut_time / 1000;
	cis_data->frame_valid_us_time = (unsigned int)frame_valid_us;

	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	if (mode_info->fine_integration_time > 0) {
		cis_data->min_fine_integration_time = mode_info->fine_integration_time;
		cis_data->max_fine_integration_time = mode_info->fine_integration_time;
	} else {
		cis_data->min_fine_integration_time = cis->sensor_info->fine_integration_time;
		cis_data->max_fine_integration_time = cis->sensor_info->fine_integration_time;
	}

	cis_data->min_coarse_integration_time = mode_info->min_cit;
	cis_data->max_margin_coarse_integration_time = mode_info->max_cit_margin;

	if (cis->reg_addr->fll_divider > 0)
		cis_data->max_coarse_integration_time = cis_data->frame_length_lines / cis->reg_addr->fll_divider - mode_info->max_cit_margin;
	else
		cis_data->max_coarse_integration_time = cis_data->frame_length_lines - mode_info->max_cit_margin;

	dbg_sensor(1, "%s [%d][%s] mode[%d], fps[%d] = pclk[%lld] / (fll[%d:0x%x] * llp[%d:0x%x]), max fps=%d\n",
		__func__, cis->id, cis->sensor_info->name, mode, frame_rate, pclk_hz, fll, fll, llp, llp, max_fps);
	dbg_sensor(1, "line_readOut_time(%d ns), frame_valid_us (%d us), min_frame_us_time(%d us)\n",
		cis_data->line_readOut_time, frame_valid_us, cis_data->min_frame_us_time);
	dbg_sensor(1, "frame_time(%d), rolling_shutter_skew(%lld)\n",
		cis_data->frame_time, cis_data->rolling_shutter_skew);
	dbg_sensor(1, "min cit(%d), max cit margin(%d) max cit(%d)\n",
		cis_data->min_coarse_integration_time, cis_data->max_margin_coarse_integration_time,
		cis_data->max_coarse_integration_time);

	/* to set proper cis_data for current mode */
	CALL_CISOPS(cis, cis_get_min_exposure_time, subdev, &min_exp);
	CALL_CISOPS(cis, cis_get_max_exposure_time, subdev, &max_exp);
	dbg_sensor(1, "%s [%d][%s] min_exp[%d.%03dms] max_exp[%d.%03dms]\n", __func__, cis->id,
		cis->sensor_info->name, (min_exp / 1000), (min_exp % 1000), (max_exp / 1000), (max_exp % 1000));

	CALL_CISOPS(cis, cis_get_min_analog_gain, subdev, &min_again);
	CALL_CISOPS(cis, cis_get_max_analog_gain, subdev, &max_again);
	dbg_sensor(1, "%s [%d][%s] min_again[x%d.%03d] max_again[x%d.%03d]\n", __func__, cis->id,
		cis->sensor_info->name, (min_again / 1000), (min_again % 1000), (max_again / 1000), (max_again % 1000));

	CALL_CISOPS(cis, cis_get_min_digital_gain, subdev, &min_dgain);
	CALL_CISOPS(cis, cis_get_max_digital_gain, subdev, &max_dgain);
	dbg_sensor(1, "%s [%d][%s] min_dgain[x%d.%03d] max_dgain[x%d.%03d]\n", __func__, cis->id,
		cis->sensor_info->name, (min_dgain / 1000), (min_dgain % 1000), (max_dgain / 1000), (max_dgain % 1000));
}
EXPORT_SYMBOL_GPL(sensor_cis_data_calculation);

u8 sensor_cis_get_duration_shifter(struct is_cis *cis, u32 input_duration)
{
	u8 shifter = 0;
	u32 shifted_val = input_duration;
	u64 max_duration_16bit = (u64)0xFF00 * 1000000 * cis->cis_data->line_length_pck / cis->cis_data->pclk;

	if (input_duration > max_duration_16bit) {
		shifted_val /=  max_duration_16bit;
		while (shifted_val) {
			shifted_val >>= 1;
			shifter++;
		}
	}

	dbg_sensor(1, "%s [%d][%s] input_duration(%d), max_duration(%llu), shifter(%d)\n",
		__func__, cis->id, cis->sensor_info->name, input_duration, max_duration_16bit, shifter);

	return shifter;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_duration_shifter);

int sensor_cis_long_term_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_long_term_expo_mode *lte_mode;
	u8 shifter = 0;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	lte_mode = &cis->long_term_mode;

	/* LTE mode or normal mode set */
	if (lte_mode->sen_strm_off_on_enable) {
		shifter = sensor_cis_get_duration_shifter(cis, lte_mode->expo[0]);
		lte_mode->expo[0] >>= shifter;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);
	if (cis->reg_addr->fll_shifter > 0)
		ret |= cis->ixc_ops->write8(cis->client, cis->reg_addr->fll_shifter, shifter);
	if (cis->reg_addr->cit_shifter > 0)
		ret |= cis->ixc_ops->write8(cis->client, cis->reg_addr->cit_shifter, shifter);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	info("%s [%d][%s] enable(%d) shift_count(%d) exp(%d)\n",
		__func__, cis->id, cis->sensor_info->name,
		lte_mode->sen_strm_off_on_enable, shifter, lte_mode->expo[0]);

	if (ret < 0)
		err("LTE register setting fail");

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_long_term_exposure);

int sensor_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	int ret = 0;
	struct is_cis *cis;
	const struct sensor_cis_mode_info *mode_info;
	struct is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data;
	unsigned int mode = 0;
	unsigned int max_margin_cit;
	u64 pclk_khz = 0;
	u16 cit_long = 0;
	u16 cit_short = 0;
	u16 fll;
	u32 line_length_pck = 0;
	u32 line_length_inck = 0;
	u32 min_fine_int = 0;
	u32 input_duration;
	u8 cit_shifter = 0;
	u64 mclk_freq_khz;
	struct ae_param exp;
	u64 line_readOut_time_ns;

	WARN_ON(!subdev);
	WARN_ON(!target_exposure);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	WARN_ON(!sensor_peri);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if ((target_exposure->long_val <= 0) || (target_exposure->short_val <= 0)) {
		err("invalid target exposure(%d, %d)", target_exposure->long_val, target_exposure->short_val);
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	cis_data->last_exp = *target_exposure;
	exp = *target_exposure;

	mode = cis_data->sens_config_index_cur;
	mode_info = cis->sensor_info->mode_infos[mode];

	input_duration = MAX(exp.long_val, exp.short_val);
	cit_shifter = sensor_cis_get_duration_shifter(cis, input_duration);

	cit_shifter = MAX(cit_shifter, cis_data->frame_length_lines_shifter);
	exp.long_val >>= cit_shifter;
	exp.short_val >>= cit_shifter;

	pclk_khz = cis_data->pclk / (1000);
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;
	line_length_inck = cis_data->line_length_inck;
	mclk_freq_khz = (u64)sensor_peri->module->pdata->mclk_freq_khz;

	if (line_length_inck > 0)
		line_readOut_time_ns = (u64)line_length_inck * 1000 * 1000 / mclk_freq_khz;
	else
		line_readOut_time_ns = (u64)line_length_pck * 1000 * 1000 / pclk_khz;

	cit_long = ((exp.long_val * 1000) / line_readOut_time_ns) - (min_fine_int / line_length_pck);
	CALL_CISOPS(cis, cis_check_cit_contraint, subdev, &cit_long);
	cit_long = ALIGN_DOWN(cit_long, mode_info->align_cit);
	if (cit_long < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "%s [%d][%d][%s] cit_long(%d) min(%d)\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			cit_long, cis_data->min_coarse_integration_time);
		cit_long = cis_data->min_coarse_integration_time;
	}

	cit_short = ((exp.short_val * 1000) / line_readOut_time_ns) - (min_fine_int / line_length_pck);
	CALL_CISOPS(cis, cis_check_cit_contraint, subdev, &cit_short);
	cit_short = ALIGN_DOWN(cit_short, mode_info->align_cit);
	if (cit_short < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "%s [%d][%d][%s] cit_short(%d) min(%d)\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			cit_short, cis_data->min_coarse_integration_time);
		cit_short = cis_data->min_coarse_integration_time;
	}

	if (cit_long > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "%s [%d][%d][%s] cit_long(%d) max(%d)\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			cit_long, cis_data->max_coarse_integration_time);
		cit_long = cis_data->max_coarse_integration_time;
	}

	if (cit_short > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "%s [%d][%d][%s] cit_short(%d) max(%d)\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			cit_short, cis_data->max_coarse_integration_time);
		cit_short = cis_data->max_coarse_integration_time;
	}

	cis_data->cur_long_exposure_coarse = cit_long;
	cis_data->cur_short_exposure_coarse = cit_short;
	max_margin_cit = cis_data->max_margin_coarse_integration_time;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	if (cis->cis_data->cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC) {
		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->cit_aeb_long, cit_long);
		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->cit_aeb_short, cit_short);

		if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
			info("[%d][%d][%s] SS_DBG CIT_L[%dus]=>[0x%04X,0x%04X], CIT_S[%dus]=>[0x%04X,0x%04X]\n",
				cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
				exp.long_val, cis->reg_addr->cit_aeb_long, cit_long,
				exp.short_val, cis->reg_addr->cit_aeb_short, cit_short);

		if (cis_data->fll_long < cit_long + max_margin_cit) {
			cis_data->fll_long = cit_long + max_margin_cit;
			if (mode_info->align_fll > 1)
				cis_data->fll_long = ALIGN_UP(cis_data->fll_long, mode_info->align_fll);

			ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->fll_aeb_long, cis_data->fll_long);

			if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
				info("[%d][%d][%s] SS_DBG FLL_L rewrite[0x%04X,0x%04X]\n",
					cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
					cis->reg_addr->fll_aeb_long, cis_data->fll_long);
		}

		if (cis_data->fll_short < cit_short + max_margin_cit) {
			cis_data->fll_short = cit_short + max_margin_cit;
			if (mode_info->align_fll > 1)
				cis_data->fll_short = ALIGN_UP(cis_data->fll_short, mode_info->align_fll);

			ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->fll_aeb_short, cis_data->fll_short);

			if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
				info("[%d][%d][%s] SS_DBG FLL_S rewrite[0x%04X,0x%04X]\n",
					cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
					cis->reg_addr->fll_aeb_short, cis_data->fll_short);
		}
	} else {
		ret = cis->ixc_ops->write16(cis->client, cis->reg_addr->cit, cit_long);

		if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
			info("[%d][%d][%s] SS_DBG CIT[%dus]=>[0x%04X,0x%04X]\n",
				cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
				exp.long_val, cis->reg_addr->cit, cit_long);

		if (cis->reg_addr->cit_shifter > 0) {
			ret |= cis->ixc_ops->write8(cis->client, cis->reg_addr->cit_shifter, cit_shifter);

			if (cit_shifter > 0 && IS_ENABLED_PRINT_CONTROL_LOG(cis))
				info("[%d][%d][%s] SS_DBG CIT_SH[%d]=>[0x%04X,0x%02X]\n",
					cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
					cit_shifter, cis->reg_addr->cit_shifter, cit_shifter);
		}

		if (cis_data->use_fast_first_frame) {
			if (cit_long + max_margin_cit < mode_info->frame_length_lines) {
				fll = mode_info->frame_length_lines;
				ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->fll, fll);

				info("[%d][%d][%s] SS_DBG fast frame FLL=>[0x%04X,0x%02X]\n",
					cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
					cis->reg_addr->fll, fll);
			}

			cis_data->use_fast_first_frame = false;
		}
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	if (exp.long_val != target_exposure->long_val || exp.short_val != target_exposure->short_val)
		dbg_sensor(1, "%s [%d][%d][%s] SS_CTL input - L[%d] S[%d]\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			target_exposure->long_val, target_exposure->short_val);

	dbg_sensor(1, "%s [%d][%d][%s] SS_CTL L[%d] S[%d] => FLL[%#x] CIT_L[%#x] CIT_S[%#x] CIT_SH[%#x]\n",
		__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
		exp.long_val, exp.short_val,
		cis_data->frame_length_lines, cit_long, cit_short, cit_shifter);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_exposure_time);

int sensor_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
{
	struct is_cis *cis = NULL;
	cis_shared_data *cis_data = NULL;
	u32 min_integration_time = 0;
	u32 min_coarse = 0;
	u32 min_fine = 0;
	u64 pclk_khz;
	u32 line_length_pck = 0;

	WARN_ON(!subdev);
	WARN_ON(!min_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	pclk_khz = cis_data->pclk / (1000);
	if (pclk_khz == 0) {
		err("[MOD:D:%d] Invalid pclk_khz(0)", cis->id);
		return 0;
	}

	line_length_pck = cis_data->line_length_pck;
	min_coarse = cis_data->min_coarse_integration_time;
	min_fine = cis_data->min_fine_integration_time;

	min_integration_time = (u32)((u64)((line_length_pck * min_coarse) + min_fine) * 1000 / pclk_khz);
	*min_expo = min_integration_time;

	dbg_sensor(1, "%s [%d][%s] min integration time %d\n",
		__func__, cis->id, cis->sensor_info->name, min_integration_time);

	return 0;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_min_exposure_time);

int sensor_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
{
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 max_integ_time = 0;
	u32 max_coarse_margin = 0;
	u32 max_fine_margin = 0;
	u32 max_coarse = 0;
	u32 max_fine = 0;
	u64 pclk_khz;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;

	WARN_ON(!subdev);
	WARN_ON(!max_expo);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	pclk_khz = cis_data->pclk / (1000);
	if (pclk_khz == 0) {
		err("[MOD:D:%d] Invalid pclk_khz(0)", cis->id);
		return 0;
	}
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = cis_data->frame_length_lines;

	max_coarse_margin = cis_data->max_margin_coarse_integration_time;
	max_fine_margin = ZERO_IF_NEG(line_length_pck - cis_data->min_fine_integration_time);

	if (cis->reg_addr->fll_divider > 0)
		max_coarse = ZERO_IF_NEG(frame_length_lines / cis->reg_addr->fll_divider - max_coarse_margin);
	else
		max_coarse = ZERO_IF_NEG(frame_length_lines - max_coarse_margin);
	max_fine = cis_data->max_fine_integration_time;

	max_integ_time = (u32)((u64)((line_length_pck * max_coarse) + max_fine) * 1000 / pclk_khz);

	*max_expo = max_integ_time;

	cis_data->max_margin_fine_integration_time = max_fine_margin;
	cis_data->max_coarse_integration_time = max_coarse;

	dbg_sensor(1, "%s [%d][%s] max integ time %d, max margin fine integ %d, max cit %d\n",
			__func__, cis->id, cis->sensor_info->name, max_integ_time,
			cis_data->max_margin_fine_integration_time, cis_data->max_coarse_integration_time);

	return 0;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_max_exposure_time);

u32 sensor_cis_get_margin_frame_us_time(struct is_cis *cis)
{
	cis_shared_data *cis_data;

	u64 pclk_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_duration = 0;
	u32 max_margin_cit = 0;

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	pclk_khz = cis_data->pclk / (1000);
	line_length_pck = cis_data->line_length_pck;
	max_margin_cit = cis_data->max_margin_coarse_integration_time;

	frame_duration = (u32)(((u64)max_margin_cit * line_length_pck) * 1000 / pclk_khz);

	dbg_sensor(2, "[%s][%s][%d] max_margin_cit(%d)->(%dms)\n", __func__,
		cis->sensor_info->name, cis_data->sen_vsync_count,
		max_margin_cit, frame_duration);

	return frame_duration;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_margin_frame_us_time);

int sensor_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
	u32 input_exposure_time,
	u32 *target_duration)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	struct is_device_sensor *device;

	u64 pclk_khz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	u32 frame_duration = 0;

	WARN_ON(!subdev);
	WARN_ON(!target_duration);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	if (input_exposure_time == 0) {
		input_exposure_time  = cis_data->min_frame_us_time;
		info("%s [%d][%s] wrong exposure time from AE, apply min frame duration forcely! (0 -> %d)\n",
			__func__, cis->id, cis->sensor_info->name, cis_data->min_frame_us_time);
	}

	pclk_khz = cis_data->pclk / (1000);
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = (u32)(((pclk_khz * input_exposure_time) / 1000
						- cis_data->min_fine_integration_time) / line_length_pck);
	frame_duration = (u32)(((u64)frame_length_lines * line_length_pck) * 1000 / pclk_khz);

	if (cis->dual_sync_type == DUAL_SYNC_TYPE_SW)
		frame_duration += is_sensor_peri_get_max_margin_for_frame_sync(device);
	else
		frame_duration += sensor_cis_get_margin_frame_us_time(cis);

	*target_duration = MAX(frame_duration, cis_data->min_frame_us_time);

	dbg_sensor(1, "%s [%d][%d][%s] SS_CTL input exp(%d), frame_duration(%d), min_frame_us(%d) target_duration(%d)\n",
		__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
		input_exposure_time, frame_duration, cis_data->min_frame_us_time, *target_duration);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_adjust_frame_duration);

int sensor_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u64 pclk_khz = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	u32 line_length_pck = 0;
	u32 line_length_inck = 0;
	u16 fll, fll_short, fll_long, min_fll;
	u8 fll_shifter = 0;
	u64 mclk_freq_khz;
	u32 frame_duration_long, frame_duration_short;
	u32 mode;
	const struct sensor_cis_mode_info *mode_info;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	WARN_ON(!sensor_peri);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	mode = cis_data->sens_config_index_cur;
	mode_info = cis->sensor_info->mode_infos[mode];

	if (frame_duration < cis_data->min_frame_us_time) {
		dbg_sensor(1, "frame duration is less than min(%d)\n", frame_duration);
		frame_duration = cis_data->min_frame_us_time;
	}

	cis_data->cur_frame_us_time = frame_duration;
	cis_data->last_frame_duration = frame_duration;

	fll_shifter = sensor_cis_get_duration_shifter(cis, frame_duration);
	frame_duration >>= fll_shifter;

	pclk_khz = cis_data->pclk / (1000);
	line_length_pck = cis_data->line_length_pck;
	line_length_inck = cis_data->line_length_inck;
	mclk_freq_khz = (u64)sensor_peri->module->pdata->mclk_freq_khz;

	if (cis_data->fll_margin_ratio > 1000) {
		frame_duration = (frame_duration * cis_data->fll_margin_ratio / 1000);
		cis_data->fll_margin_ratio = 0;
	}

	if (line_length_inck > 0)
		fll = (u16)((mclk_freq_khz * frame_duration) / (line_length_inck * 1000));
	else
		fll = (u16)((pclk_khz * frame_duration) / (line_length_pck * 1000));

	if (mode_info->align_fll > 1)
		fll = ALIGN_UP(fll, mode_info->align_fll);

	min_fll = mode_info->frame_length_lines;
	if (min_fll > fll && fll_shifter == 0)
		fll = min_fll;

	IXC_MUTEX_LOCK(cis->ixc_lock);

	if (cis_data->cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC) {
		frame_duration_long = frame_duration_short = (frame_duration / 2);

		if (cis_data->fll_margin_ratio_aeb > 1000) {
			frame_duration_long = (frame_duration_long * cis_data->fll_margin_ratio_aeb / 1000);
			cis_data->fll_margin_ratio_aeb = 0;
		}

		if (line_length_inck > 0) {
			fll_long = (u16)((mclk_freq_khz * frame_duration_long) / (line_length_inck * 1000));
			fll_short = (u16)((mclk_freq_khz * frame_duration_short) / (line_length_inck * 1000));
		} else {
			fll_long = (u16)((pclk_khz * frame_duration_long) / (line_length_pck * 1000));
			fll_short = (u16)((pclk_khz * frame_duration_short) / (line_length_pck * 1000));
		}

		if (mode_info->align_fll > 1) {
			fll_long = ALIGN_UP(fll_long, mode_info->align_fll);
			fll_short = ALIGN_UP(fll_short, mode_info->align_fll);
		}

		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->fll_aeb_long, fll_long);
		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->fll_aeb_short, fll_short);

		if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
			info("[%d][%d][%s] SS_DBG FLL_L[%dus]=>[0x%04X,0x%04X], FLL_S[%dus]=>[0x%04X,0x%04X]\n",
				cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
				frame_duration_long, cis->reg_addr->fll_aeb_long, fll_long,
				frame_duration_short, cis->reg_addr->fll_aeb_short, fll_short);

		cis_data->fll_long = fll_long;
		cis_data->fll_short = fll_short;
	} else {
		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->fll, fll);

		if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
			info("[%d][%d][%s] SS_DBG FLL[%dus]=>[0x%04X,0x%04X]\n",
				cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
				frame_duration, cis->reg_addr->fll, fll);

		if (cis->reg_addr->fll_shifter > 0) {
			ret |= cis->ixc_ops->write8(cis->client, cis->reg_addr->fll_shifter, fll_shifter);

			if (fll_shifter > 0 && IS_ENABLED_PRINT_CONTROL_LOG(cis))
				info("[%d][%d][%s] SS_DBG FLL_SH[%d]=>[0x%04X,0x%02X]\n",
					cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
					fll_shifter, cis->reg_addr->fll_shifter, fll_shifter);
		}
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	dbg_sensor(1, "%s [%d][%d][%s] SS_CTL Duration[%dus]->[%dus] => FLL[%#x] FLL_SH[%#x]\n",
		__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
		cis_data->cur_frame_us_time, frame_duration, fll, fll_shifter);

	cis_data->pre_frame_length_lines = cis_data->frame_length_lines;
	cis_data->frame_length_lines = fll;

	if (cis->reg_addr->fll_divider > 0)
		cis_data->max_coarse_integration_time
			= cis_data->frame_length_lines / cis->reg_addr->fll_divider - cis_data->max_margin_coarse_integration_time;
	else
		cis_data->max_coarse_integration_time
			= cis_data->frame_length_lines - cis_data->max_margin_coarse_integration_time;

	cis_data->frame_length_lines_shifter = fll_shifter;

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_frame_duration);

int sensor_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	u32 frame_duration = 0;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	if (min_fps > cis_data->max_fps) {
		err("[MOD:D:%d] request FPS is too high(%d), set to max(%d)",
			cis->id, min_fps, cis_data->max_fps);
		min_fps = cis_data->max_fps;
	}

	if (min_fps == 0) {
		err("[MOD:D:%d] request FPS is 0, set to min FPS(1)", cis->id);
		min_fps = 1;
	}

	frame_duration = (1 * 1000 * 1000) / min_fps;

	dbg_sensor(1, "%s [%d][%s] set FPS(%d), frame duration(%d)\n",
			__func__, cis->id, cis->sensor_info->name, min_fps, frame_duration);

	ret = CALL_CISOPS(cis, cis_set_frame_duration, subdev, frame_duration);
	if (ret < 0) {
		err("[MOD:D:%d] set frame duration is fail(%d)", cis->id, ret);
		goto p_err;
	}

	cis_data->min_frame_us_time = MAX(frame_duration, cis_data->base_min_frame_us_time);

p_err:

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_frame_rate);

int sensor_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 again_code = 0;
	u32 again_permile = 0;

	WARN_ON(!subdev);
	WARN_ON(!target_permile);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	again_code = CALL_CISOPS(cis, cis_calc_again_code, input_again);

	if (again_code > cis_data->max_analog_gain[0])
		again_code = cis_data->max_analog_gain[0];
	else if (again_code < cis_data->min_analog_gain[0])
		again_code = cis_data->min_analog_gain[0];

	again_permile = CALL_CISOPS(cis, cis_calc_again_permile, again_code);

	dbg_sensor(1, "%s [%d][%s] min again(%d), max(%d), input_again(%d), code(%d), permile(%d)\n",
			__func__, cis->id, cis->sensor_info->name,
			cis_data->max_analog_gain[0],
			cis_data->min_analog_gain[0],
			input_again, again_code, again_permile);

	*target_permile = again_permile;

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_adjust_analog_gain);

int sensor_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 long_again = 0;
	u16 short_again = 0;

	WARN_ON(!subdev);
	WARN_ON(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data->last_again = *again;

	/* again->long_val & again->val are union values*/
	long_again = (u16)CALL_CISOPS(cis, cis_calc_again_code, again->long_val);
	short_again = (u16)CALL_CISOPS(cis, cis_calc_again_code, again->short_val);

	if (long_again < cis_data->min_analog_gain[0]) {
		info("%s [%d][%d][%s] wrong long_again, reset [%d:%d => %d]\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			again->long_val, long_again, cis_data->min_analog_gain[0]);
		long_again = cis_data->min_analog_gain[0];
	}

	if (long_again > cis_data->max_analog_gain[0]) {
		info("%s [%d][%d][%s] wrong long_again, reset [%d:%d => %d]\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			again->long_val, long_again, cis_data->max_analog_gain[0]);
		long_again = cis_data->max_analog_gain[0];
	}

	if (short_again < cis_data->min_analog_gain[0]) {
		info("%s [%d][%d][%s] wrong short_again, reset [%d:%d => %d]\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			again->short_val, short_again, cis_data->min_analog_gain[0]);
		short_again = cis_data->min_analog_gain[0];
	}

	if (short_again > cis_data->max_analog_gain[0]) {
		info("%s [%d][%d][%s] wrong short_again, reset [%d:%d => %d]\n",
			__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			again->short_val, short_again, cis_data->max_analog_gain[0]);
		short_again = cis_data->max_analog_gain[0];
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	if (cis_data->cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC) {
		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->again_aeb_long, long_again);
		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->again_aeb_short, short_again);

		if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
			info("[%d][%d][%s] SS_DBG A_GAIN_L[%d]=>[0x%04X,0x%04X], A_GAIN_S[%d]=>[0x%04X,0x%04X]\n",
				cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
				again->long_val, cis->reg_addr->again_aeb_long, long_again,
				again->short_val, cis->reg_addr->again_aeb_short, short_again);
	} else {
		ret = cis->ixc_ops->write16(cis->client, cis->reg_addr->again, long_again);

		if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
			info("[%d][%d][%s] SS_DBG A_GAIN[%d]=>[0x%04X,0x%04X]\n",
				cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
				again->long_val, cis->reg_addr->again, long_again);
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	dbg_sensor(1, "%s [%d][%d][%s] SS_CTL L[%d] S[%d] => A_GAIN_L[%#x] A_GAIN_S[%#x]\n",
		__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
		again->long_val, again->short_val, long_again, short_again);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_analog_gain);

int sensor_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
{
	int ret = 0;
	struct is_cis *cis;
	u16 analog_gain = 0;

	WARN_ON(!subdev);
	WARN_ON(!again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = cis->ixc_ops->read16(cis->client, cis->reg_addr->again, &analog_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

	*again = CALL_CISOPS(cis, cis_calc_again_permile, analog_gain);

	dbg_sensor(1, "%s [%d][%s] cur_again = x%d.%03d, analog_gain(%#x)\n",
			__func__, cis->id, cis->sensor_info->name, (*again / 1000), (*again % 1000), analog_gain);

p_err_i2c_unlock:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_analog_gain);

int sensor_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	const struct sensor_cis_mode_info *mode_info;
	u32 mode;

	WARN_ON(!subdev);
	WARN_ON(!min_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	mode = cis_data->sens_config_index_cur;
	mode_info = cis->sensor_info->mode_infos[mode];

	if (mode_info->use_mode_analog_gain)
		cis_data->min_analog_gain[0] = mode_info->min_analog_gain;
	else
		cis_data->min_analog_gain[0] = cis->sensor_info->min_analog_gain;

	cis_data->min_analog_gain[1] = CALL_CISOPS(cis, cis_calc_again_permile, cis_data->min_analog_gain[0]);

	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "%s [%d][%s] code %d, permile %d\n", __func__, cis->id,
		cis->sensor_info->name, cis_data->min_analog_gain[0], cis_data->min_analog_gain[1]);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_min_analog_gain);

int sensor_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	struct is_cis *cis;
	const struct sensor_cis_mode_info *mode_info;
	cis_shared_data *cis_data;
	u32 mode;

	WARN_ON(!subdev);
	WARN_ON(!max_again);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	mode = cis_data->sens_config_index_cur;
	mode_info = cis->sensor_info->mode_infos[mode];

	cis_data->max_analog_gain[0] = mode_info->max_analog_gain;
	cis_data->max_analog_gain[1] = CALL_CISOPS(cis, cis_calc_again_permile, cis_data->max_analog_gain[0]);

	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "%s [%d][%s] code %d, permile %d\n", __func__, cis->id,
		cis->sensor_info->name, cis_data->max_analog_gain[0], cis_data->max_analog_gain[1]);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_max_analog_gain);

int sensor_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u16 long_dgain = 0;
	u16 short_dgain = 0;

	WARN_ON(!subdev);
	WARN_ON(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data->last_dgain = *dgain;

	/* dgain->long_val & dgain->val are union values*/
	long_dgain = (u16)CALL_CISOPS(cis, cis_calc_dgain_code, dgain->long_val);
	short_dgain = (u16)CALL_CISOPS(cis, cis_calc_dgain_code, dgain->short_val);

	if (long_dgain < cis_data->min_digital_gain[0]) {
		info("[%s] not proper long_gain value, reset to min_digital_gain\n", __func__);
		long_dgain = cis_data->min_digital_gain[0];
	}

	if (long_dgain > cis_data->max_digital_gain[0]) {
		info("[%s] not proper long_gain value, reset to max_digital_gain\n", __func__);
		long_dgain = cis_data->max_digital_gain[0];
	}

	if (short_dgain < cis_data->min_digital_gain[0]) {
		info("[%s] not proper short_gain value, reset to min_digital_gain\n", __func__);
		short_dgain = cis_data->min_digital_gain[0];
	}

	if (short_dgain > cis_data->max_digital_gain[0]) {
		info("[%s] not proper short_gain value, reset to max_digital_gain\n", __func__);
		short_dgain = cis_data->max_digital_gain[0];
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	if (cis->cis_data->cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC) {
		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->dgain_aeb_long, long_dgain);
		ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->dgain_aeb_short, short_dgain);

		if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
			info("[%d][%d][%s] SS_DBG D_GAIN_L[%d]=>[0x%04X,0x%04X], D_GAIN_S[%d]=>[0x%04X,0x%04X]\n",
				cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
				dgain->long_val, cis->reg_addr->dgain_aeb_long, long_dgain,
				dgain->short_val, cis->reg_addr->dgain_aeb_short, short_dgain);
	} else {
		ret = cis->ixc_ops->write16(cis->client, cis->reg_addr->dgain, long_dgain);

		if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
			info("[%d][%d][%s] SS_DBG D_GAIN[%d]=>[0x%04X,0x%04X]\n",
				cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
				dgain->long_val, cis->reg_addr->dgain, long_dgain);

		if (cis->reg_addr->dgain_secondary)
			ret |= cis->ixc_ops->write16(cis->client, cis->reg_addr->dgain_secondary, long_dgain);
	}

	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	dbg_sensor(1, "%s [%d][%d][%s] SS_CTL L[%d] S[%d] => D_GAIN_L[%#x] D_GAIN_S[%#x]\n",
		__func__, cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
		dgain->long_val, dgain->short_val, long_dgain, short_dgain);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_digital_gain);

int sensor_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
{
	int ret = 0;
	struct is_cis *cis;
	u16 digital_gain = 0;

	WARN_ON(!subdev);
	WARN_ON(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(cis->ixc_lock);

	ret = cis->ixc_ops->read16(cis->client, cis->reg_addr->dgain, &digital_gain);
	if (ret < 0)
		goto p_err_i2c_unlock;

	*dgain = CALL_CISOPS(cis, cis_calc_dgain_permile, digital_gain);

	dbg_sensor(1, "%s [%d][%s] cur_dgain = x%d.%03d, digital_gain(%#x)\n",
			__func__, cis->id, cis->sensor_info->name, (*dgain / 1000), (*dgain % 1000), digital_gain);

p_err_i2c_unlock:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_digital_gain);

int sensor_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;

	WARN_ON(!subdev);
	WARN_ON(!min_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	cis_data->min_digital_gain[0] = cis->sensor_info->min_digital_gain;
	cis_data->min_digital_gain[1] = CALL_CISOPS(cis, cis_calc_dgain_permile, cis_data->min_digital_gain[0]);

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "%s [%d][%s] code %d, permile %d\n", __func__, cis->id,
		cis->sensor_info->name, cis_data->min_digital_gain[0], cis_data->min_digital_gain[1]);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_min_digital_gain);

int sensor_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	struct is_cis *cis;
	const struct sensor_cis_mode_info *mode_info;
	cis_shared_data *cis_data;
	u32 mode;

	WARN_ON(!subdev);
	WARN_ON(!max_dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;
	mode = cis_data->sens_config_index_cur;
	mode_info = cis->sensor_info->mode_infos[mode];

	cis_data->max_digital_gain[0] = mode_info->max_digital_gain;
	cis_data->max_digital_gain[1] = CALL_CISOPS(cis, cis_calc_dgain_permile, cis_data->max_digital_gain[0]);

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "%s [%d][%s] code %d, permile %d\n", __func__, cis->id,
		cis->sensor_info->name, cis_data->max_digital_gain[0], cis_data->max_digital_gain[1]);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_max_digital_gain);

int sensor_cis_compensate_gain_for_extremely_br(struct v4l2_subdev *subdev, u32 expo, u32 *again, u32 *dgain)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	struct is_device_sensor_peri *sensor_peri = NULL;
	const struct sensor_cis_mode_info *mode_info;
	u32 mode;
	u64 pclk_khz = 0;
	u64 mclk_freq_khz;
	u32 llp = 0;
	u32 lli = 0;
	u32 min_fine_int = 0;
	u32 cit = 0;
	u32 again_input, again_comp = 0;
	u32 dgain_input, dgain_comp = 0;
	u16 cit_compensation_threshold = 15; /* default value for legacy driver */
	u64 expected_exp, real_cit_exp;
	u64 line_readOut_time_ns;

	WARN_ON(!subdev);
	WARN_ON(!again);
	WARN_ON(!dgain);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	WARN_ON(!sensor_peri);

	cis_data = cis->cis_data;

	pclk_khz = cis_data->pclk / 1000;
	llp = cis_data->line_length_pck;
	mclk_freq_khz = (u64)sensor_peri->module->pdata->mclk_freq_khz;
	lli = cis_data->line_length_inck;
	min_fine_int = cis_data->min_fine_integration_time;

	if (llp <= 0) {
		err("[%s] invalid line_length_pck(%d)\n", __func__, llp);
		goto p_err;
	}

	if (lli > 0)
		line_readOut_time_ns = (u64)lli * 1000 * 1000 / mclk_freq_khz;
	else
		line_readOut_time_ns = (u64)llp * 1000 * 1000 / pclk_khz;

	cit = ((expo * 1000) / line_readOut_time_ns) - (min_fine_int / llp);

	/* keep backward compatibility for legacy driver */
	if (cis->sensor_info) {
		mode = cis_data->sens_config_index_cur;
		mode_info = cis->sensor_info->mode_infos[mode];
		cit = ALIGN_DOWN(cit, mode_info->align_cit);
		cit_compensation_threshold = cis->sensor_info->cit_compensation_threshold * mode_info->align_cit;
	}

	if (cit < cis_data->min_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, cit, cis_data->min_coarse_integration_time);
		cit = cis_data->min_coarse_integration_time;
	}

	if (cit <= cit_compensation_threshold) {
		CALL_CISOPS(cis, cis_adjust_cit_for_extremely_br, subdev, &cit);

		again_input = *again;
		dgain_input = *dgain;

		if (lli > 0 && line_readOut_time_ns > 0) {
			expected_exp = ((u64)expo * llp * 1000) / line_readOut_time_ns - min_fine_int;
			real_cit_exp = llp * cit;
		} else {
			expected_exp = (expo * pclk_khz) / 1000 - min_fine_int;
			real_cit_exp = llp * cit;
		}

		if (again_input >= cis_data->max_analog_gain[1]) {
			again_comp = cis_data->max_analog_gain[1];
			dgain_comp = dgain_input * expected_exp / real_cit_exp;
		} else {
			again_comp = again_input * expected_exp / real_cit_exp;
			dgain_comp = dgain_input;

			if (again_comp < cis_data->min_analog_gain[1]) {
				again_comp = cis_data->min_analog_gain[1];
			} else if (again_comp >= cis_data->max_analog_gain[1]) {
				dgain_comp = dgain_input * again_comp / cis_data->max_analog_gain[1];
				again_comp = cis_data->max_analog_gain[1];
			}
		}

		*again = again_comp;
		*dgain = dgain_comp;

		dbg_sensor(1, "[%s] exp[%d] again[%d] dgain[%d] cit[%d] => compensated again[%d] dgain[%d]\n",
			__func__, expo, again_input, dgain_input, cit, again_comp, dgain_comp);
	}

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_compensate_gain_for_extremely_br);

int sensor_cis_set_totalgain(struct v4l2_subdev *subdev, struct ae_param *target_exposure,
	struct ae_param *again, struct ae_param *dgain)
{
	int ret = 0;
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	cis_shared_data *cis_data = NULL;
	struct ae_param total_dgain;
	u32 again_code = 0;
	u32 sensor_again = 0;
	u32 cal_digital = 0;

	WARN_ON(!subdev);
	WARN_ON(!target_exposure);
	WARN_ON(!again);
	WARN_ON(!dgain);

	cis_data = cis->cis_data;

	/* Set Exposure Time */
	ret = CALL_CISOPS(cis, cis_set_exposure_time, subdev, target_exposure);
	if (ret < 0)
		err("cis_set_exposure_time fail");

	/* Set Analog Gain */
	ret = CALL_CISOPS(cis, cis_set_analog_gain, subdev, again);
	if (ret < 0)
		err("cis_set_analog_gain fail");

	/* Set Digital Gain */
	again_code = CALL_CISOPS(cis, cis_calc_again_code, again->val);

	if (again_code < cis_data->max_analog_gain[0]) {
		sensor_again = CALL_CISOPS(cis, cis_calc_again_permile, again_code);
		if (sensor_again > 0)
			cal_digital = (again->val * 1000) / sensor_again;

		if (cal_digital < 1000)
			cal_digital = 1000;

		total_dgain.long_val = cal_digital;
		total_dgain.short_val = cal_digital;

		dbg_sensor(1, "%s [%d][%s] VCNT[%d] A_GAIN[%d] => Sensor_A_GAIN[%d], Total_GAIN[%d]\n",
			__func__, cis->id, cis->sensor_info->name, cis_data->sen_vsync_count,
			again->val, sensor_again, cal_digital);
	} else {
		total_dgain.long_val = dgain->val;
		total_dgain.short_val = dgain->val;
	}

	ret = CALL_CISOPS(cis, cis_set_digital_gain, subdev, &total_dgain);
	if (ret < 0)
		err("sensor_cis_set_digital_gain fail");

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_totalgain);

int sensor_cis_init_state(struct v4l2_subdev *subdev, int mode)
{
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	cis_shared_data *cis_data = cis->cis_data;
	const struct sensor_cis_mode_info *mode_info;
	unsigned int target_history_index;
	struct is_device_sensor *device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);

	WARN_ON(!device);

	if (mode >= cis->sensor_info->mode_count) {
		err("invalid mode(%d)!!", mode);
		return -EINVAL;
	}

	mode_info = cis->sensor_info->mode_infos[mode];
	cis_data->remosaic_mode = mode_info->remosaic_mode;
	cis_data->pre_12bit_mode = mode_info->state_12bit;
	cis_data->cur_12bit_mode = mode_info->state_12bit;
	cis_data->pre_lownoise_mode = mode_info->lownoise_mode;
	cis_data->cur_lownoise_mode = mode_info->lownoise_mode;

	cis_data->stream_on = false;
	cis->long_term_mode.sen_strm_off_on_step = 0;
	cis->long_term_mode.sen_strm_off_on_enable = false;
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;


	cis_data->cur_pattern_mode = SENSOR_TEST_PATTERN_MODE_OFF;
	cis_data->pre_remosaic_zoom_ratio = 0;
	cis_data->cur_remosaic_zoom_ratio = 0;
	cis_data->pre_bayer_pattern = 0;
	cis_data->cur_bayer_pattern = 0;
	cis_data->pre_hdr_mode = SENSOR_HDR_MODE_SINGLE;
	cis_data->cur_hdr_mode = SENSOR_HDR_MODE_SINGLE;
	cis_data->seamless_update_vsync_cnt = 0;
	cis_data->remosaic_crop_update_vsync_cnt = 0;
	cis_data->skip_control_vsync_count = 0;
	cis_data->last_seamless_history_vsync_cnt = 0;

	memset(cis_data->seamless_history, 0, sizeof(struct is_sensor_seamless_ctl) * NUM_FRAMES);

	sensor_cis_set_cur_seamless_ctl(cis_data);

	if (cis->ctrl_delay == N_PLUS_ONE_FRAME)
		target_history_index = (cis_data->sen_vsync_count % NUM_FRAMES);
	else
		target_history_index = ((NUM_FRAMES + cis_data->sen_vsync_count - 1) % NUM_FRAMES);

	memcpy(&cis_data->seamless_history[target_history_index],
		&cis_data->cur_seamless_ctl, sizeof(struct is_sensor_seamless_ctl));
	/* N-2 history needs to be initialized after mode change */
	memcpy(&cis_data->seamless_history[(NUM_FRAMES + target_history_index - 1) % NUM_FRAMES],
		&cis_data->cur_seamless_ctl, sizeof(struct is_sensor_seamless_ctl));

	ssinfo_seamless_mode(cis, &cis_data->cur_seamless_ctl);

	cis_data->seamless_mode_changed = true; /* to update first frame */

	is_sensor_peri_update_seamless_state(device);

	return 0;
}
EXPORT_SYMBOL_GPL(sensor_cis_init_state);

int sensor_cis_update_seamless_history(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	cis_shared_data *cis_data = cis->cis_data;

	/* if gph is on, the seamless state cannot be updated because perframe control is still in progress */
	if (cis_data->stream_on
		&& (!cis_data->seamless_mode_changed || cis_data->group_param_hold)) {

		/* if N-1 frame drop occurs, N-1 history should be copied from N-2 */
		if (cis_data->sen_vsync_count > 1 &&
			cis_data->last_seamless_history_vsync_cnt > 0 &&
			cis_data->last_seamless_history_vsync_cnt + 2 == cis_data->sen_vsync_count) {
			memcpy(&cis_data->seamless_history[(cis_data->sen_vsync_count - 1) % NUM_FRAMES],
				&cis_data->seamless_history[(cis_data->sen_vsync_count - 2) % NUM_FRAMES],
				sizeof(struct is_sensor_seamless_ctl));

			ssinfo("N-1[%d] frame drop occurred, copy N-2 frame history to N-1\n",
				cis, cis_data->last_seamless_history_vsync_cnt + 1);
		}

		if (cis_data->sen_vsync_count > 0)
			memcpy(&cis_data->seamless_history[cis_data->sen_vsync_count % NUM_FRAMES],
				&cis_data->seamless_history[(cis_data->sen_vsync_count - 1) % NUM_FRAMES],
				sizeof(struct is_sensor_seamless_ctl));

		cis_data->last_seamless_history_vsync_cnt = cis_data->sen_vsync_count;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_update_seamless_history);

int sensor_cis_get_mode_info(struct v4l2_subdev *subdev, u32 mode, struct is_sensor_mode_info *mode_info)
{
	int ret = 0;
	struct is_cis *cis;
	const struct sensor_cis_mode_info *cis_mode_info;
	cis_shared_data *cis_data;

	u16 fll, llp;
	u32 frame_rate, max_fps, frame_valid_us;
	u32 min_coarse = 0, min_fine = 0;
	u32 max_coarse_margin = 0, max_coarse = 0, max_fine = 0;
	u64 pclk_hz, pclk_khz;
	unsigned int min_frame_us_time;
	unsigned int cur_frame_us_time;
	unsigned int min_fine_integration_time;
	unsigned int max_fine_integration_time;
	unsigned int min_coarse_integration_time;
	unsigned int max_margin_coarse_integration_time;

	WARN_ON(!subdev);
	WARN_ON(!mode_info);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_mode_info = cis->sensor_info->mode_infos[mode];
	cis_data = cis->cis_data;

	/* get pclk value from pll info */
	pclk_hz = cis_mode_info->pclk;
	pclk_khz = pclk_hz / (1000);
	fll = cis_mode_info->frame_length_lines;
	llp = cis_mode_info->line_length_pck;

	/* the time of processing one frame calculation (us) */
	min_frame_us_time = (u64)fll * llp * 1000 * 1000 / pclk_hz;
	cur_frame_us_time = min_frame_us_time;

	frame_rate = pclk_hz / ((u64)fll * (u64)llp);
	max_fps = frame_rate;

	/* calculate max fps with rounding */
	if (((pclk_hz * 10) / ((u64)fll * (u64)llp)) % 10 >= 5)
		max_fps++;

	frame_valid_us = (u64)cis_data->cur_height * llp * 1000 * 1000 / pclk_hz;

	if (cis_mode_info->fine_integration_time > 0) {
		min_fine_integration_time = cis_mode_info->fine_integration_time;
		max_fine_integration_time = cis_mode_info->fine_integration_time;
	} else {
		min_fine_integration_time = cis->sensor_info->fine_integration_time;
		max_fine_integration_time = cis->sensor_info->fine_integration_time;
	}

	min_coarse_integration_time = cis_mode_info->min_cit;
	max_margin_coarse_integration_time = cis_mode_info->max_cit_margin;

	/* calculate max_expo */
	max_coarse_margin = max_margin_coarse_integration_time;
	max_coarse = ZERO_IF_NEG(fll - max_coarse_margin);
	max_fine = max_fine_integration_time;

	/* calculate min_expo */
	min_coarse = min_coarse_integration_time;
	min_fine = min_fine_integration_time;

	mode_info->min_expo = (u32)((u64)((llp * min_coarse) + min_fine) * 1000 / pclk_khz);
	mode_info->max_expo = (u32)((u64)((llp * max_coarse) + max_fine) * 1000 / pclk_khz);
	if (cis_mode_info->use_mode_analog_gain)
		mode_info->min_again = CALL_CISOPS(cis, cis_calc_again_permile, cis_mode_info->min_analog_gain);
	else
		mode_info->min_again = CALL_CISOPS(cis, cis_calc_again_permile, cis->sensor_info->min_analog_gain);
	mode_info->max_again = CALL_CISOPS(cis, cis_calc_again_permile, cis_mode_info->max_analog_gain);
	mode_info->min_dgain = CALL_CISOPS(cis, cis_calc_dgain_permile, cis->sensor_info->min_digital_gain);
	mode_info->max_dgain = CALL_CISOPS(cis, cis_calc_dgain_permile, cis_mode_info->max_digital_gain);
	mode_info->vvalid_time = frame_valid_us;
	mode_info->vblank_time = 1000000U / max_fps - frame_valid_us;
	mode_info->max_fps = max_fps;

	dbg_sensor(1, "[%s][%d][%s] mode(%d), min_expo(%d), max_expo(%d), min_again(%d), max_again(%d), "
		KERN_CONT "min_dgain(%d), max_dgain(%d), vvalid_time(%d), vblank_time(%d), max_fps(%d)\n",
		__func__, cis->id, cis->sensor_info->name, mode_info->mode,
		mode_info->min_expo, mode_info->max_expo,
		mode_info->min_again, mode_info->max_again,
		mode_info->min_dgain, mode_info->max_dgain,
		mode_info->vvalid_time, mode_info->vblank_time,
		mode_info->max_fps);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_mode_info);

int sensor_cis_recover_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;
	cis_shared_data *cis_data;
	struct is_device_sensor *device;
#if IS_ENABLED(USE_CAMERA_SENSOR_RETENTION)
	struct is_module_enum *module;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;
#endif

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	cis_data = cis->cis_data;
	WARN_ON(!cis_data);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

#if IS_ENABLED(USE_CAMERA_SENSOR_RETENTION)
	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	WARN_ON(!ext_info);

	if (ext_info->use_retention_mode != SENSOR_RETENTION_UNSUPPORTED)
		ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
#endif

	info("%s [%d][%s] start\n", __func__, cis->id, cis->sensor_info->name);

	ret = CALL_CISOPS(cis, cis_set_global_setting, subdev);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_mode_change, subdev, cis_data->sens_config_index_cur);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_set_frame_duration, subdev, cis_data->cur_frame_us_time);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_set_analog_gain, subdev, &cis_data->last_again);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_set_digital_gain, subdev, &cis_data->last_dgain);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_set_exposure_time, subdev, &cis_data->last_exp);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_stream_on, subdev);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_wait_streamon, subdev);
	if (ret < 0)
		goto p_err;

	info("%s [%d][%s] end\n", __func__, cis->id, cis->sensor_info->name);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_recover_stream_on);

int sensor_cis_recover_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis = NULL;
#if IS_ENABLED(USE_CAMERA_SENSOR_RETENTION)
	struct is_module_enum *module;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;
#endif

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

#if IS_ENABLED(USE_CAMERA_SENSOR_RETENTION)
	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	WARN_ON(!ext_info);

	if (ext_info->use_retention_mode != SENSOR_RETENTION_UNSUPPORTED)
		ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
#endif

	info("%s [%d][%s] start\n", __func__, cis->id, cis->sensor_info->name);

	ret = CALL_CISOPS(cis, cis_set_global_setting, subdev);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_stream_off, subdev);
	if (ret < 0)
		goto p_err;

	ret = CALL_CISOPS(cis, cis_wait_streamoff, subdev);
	if (ret < 0)
		goto p_err;

	info("%s [%d][%s] end\n", __func__, cis->id, cis->sensor_info->name);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_recover_stream_off);

int sensor_cis_dump_registers(struct v4l2_subdev *subdev, const u32 *regs, const u32 size)
{
	int ret = 0;
	int i = 0;
	struct is_cis *cis;
	u8 data8 = 0;
	u16 data16 = 0;

	WARN_ON(!subdev);
	WARN_ON(!regs);

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

	for (i = 0; i < size; i += IXC_NEXT) {
		if (regs[i + IXC_BYTE] == 0x2 && regs[i + IXC_ADDR] == 0x6028) {
			ret = cis->ixc_ops->write16(cis->client, regs[i + IXC_ADDR], regs[i + IXC_DATA]);
			if (ret < 0) {
				err("is_sensor_write16 fail, ret(%d), addr(%#x)",
						ret, regs[i + IXC_ADDR]);
			}
		}

		if (regs[i + IXC_BYTE] == 0x1) {
			ret = cis->ixc_ops->read8(cis->client, regs[i + IXC_ADDR], &data8);
			if (ret < 0) {
				err("is_sensor_read8 fail, ret(%d), addr(%#x)",
						ret, regs[i + IXC_ADDR]);
			}
			pr_err("[SEN:DUMP] [0x%04X, 0x%04X\n", regs[i + IXC_ADDR], data8);
		} else {
			ret = cis->ixc_ops->read16(cis->client, regs[i + IXC_ADDR], &data16);
			if (ret < 0) {
				err("is_sensor_read6 fail, ret(%d), addr(%#x)",
						ret, regs[i + IXC_ADDR]);
			}
			pr_err("[SEN:DUMP] [0x%04X, 0x%04X\n", regs[i + IXC_ADDR], data16);
		}
	}

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_dump_registers);

int sensor_cis_parse_dt(struct device *dev, struct v4l2_subdev *subdev)
{
	int ret;
	int i = 0;
	struct is_cis *cis;
	struct device_node *dnode;
	const u32 *rev_reg_spec;
	u32 rev_reg[IS_CIS_REV_MAX_LIST+2];
	u32 rev_reg_len;
	bool use_pdaf = false;

	WARN_ON(!dev);
	WARN_ON(!dev->of_node);
	WARN_ON(!subdev);

	dnode = dev->of_node;

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		return -EINVAL;
	}

	rev_reg_spec = of_get_property(dnode, "rev_reg", &rev_reg_len);
	if (rev_reg_spec) {
		rev_reg_len /= (unsigned int)sizeof(*rev_reg_spec);
		BUG_ON(rev_reg_len > (IS_CIS_REV_MAX_LIST + 2));

		if (!of_property_read_u32_array(dnode, "rev_reg", rev_reg, rev_reg_len)) {
			cis->rev_valid_count = rev_reg_len - 2;
			cis->rev_addr = rev_reg[0];
			cis->rev_byte = rev_reg[1];

			for (i = 2; i < rev_reg_len; i++)
				cis->rev_valid_values[i-2] = rev_reg[i];
		}
	} else {
		info("rev_reg read is fail\n");
		cis->rev_addr = 0x0002;	// default 1byte 0x0002 read
		cis->rev_byte = 1;
		cis->rev_valid_count = 0;
	}

	if (of_property_read_bool(dnode, "use_pdaf"))
		use_pdaf = true;

	cis->use_pdaf = use_pdaf;
	probe_info("%s use_pdaf %d\n", __func__, use_pdaf);

	ret = of_property_read_u32(dnode, "sensor_f_number", &cis->aperture_num);
	if (ret) {
		warn("f-number read is fail(%d), use default value", ret);
		cis->aperture_num = F2_4;
	}
	probe_info("%s f-number %d\n", __func__, cis->aperture_num);

	cis->use_initial_ae = of_property_read_bool(dnode, "use_initial_ae");
	probe_info("%s use initial_ae(%d)\n", __func__, cis->use_initial_ae);

	cis->vendor_use_adaptive_mipi = of_property_read_bool(dnode, "vendor_use_adaptive_mipi");
	probe_info("%s vendor_use_adaptive_mipi(%d)\n", __func__, cis->vendor_use_adaptive_mipi);

	cis->vendor_use_nfi = of_property_read_bool(dnode, "vendor_use_nfi");
	probe_info("%s vendor_use_nfi(%d)\n", __func__, cis->vendor_use_nfi);
	return 0;
}
EXPORT_SYMBOL_GPL(sensor_cis_parse_dt);

#if defined(USE_RECOVER_I2C_TRANS)
void sensor_cis_recover_i2c_fail(struct v4l2_subdev *subdev_cis)
{
	struct is_device_sensor *device;
	struct v4l2_subdev *subdev_module;

	FIMC_BUG_VOID(!subdev_cis);

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev_cis);
	FIMC_BUG_VOID(!device);

	subdev_module = device->subdev_module;
	FIMC_BUG_VOID(!subdev_module);

	sensor_module_power_reset(subdev_module, device);
}
#endif

int sensor_cis_wait_streamoff(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 wait_cnt = 0, time_out_cnt = 250;
	u8 sensor_fcount = 0;
	u32 i2c_fail_cnt = 0, i2c_fail_max_cnt = 5;

	WARN_ON(!subdev);

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
	ret = cis->ixc_ops->read8(cis->client, 0x0005, &sensor_fcount);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	if (ret < 0)
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", 0x0005, sensor_fcount, ret);

	/*
	 * Read sensor frame counter (sensor_fcount address = 0x0005)
	 * stream on (0x00 ~ 0xFE), stream off (0xFF)
	 */
	while (sensor_fcount != 0xFF) {
		IXC_MUTEX_LOCK(cis->ixc_lock);
		ret = cis->ixc_ops->read8(cis->client, 0x0005, &sensor_fcount);
		IXC_MUTEX_UNLOCK(cis->ixc_lock);
		if (ret < 0) {
			i2c_fail_cnt++;
			err("i2c transfer fail addr(%x), val(%x), try(%d), ret = %d\n",
				0x0005, sensor_fcount, i2c_fail_cnt, ret);

			if (i2c_fail_cnt >= i2c_fail_max_cnt) {
				err("[MOD:D:%d] %s, i2c fail, i2c_fail_cnt(%d) >= i2c_fail_max_cnt(%d), sensor_fcount(%d)",
						cis->id, __func__, i2c_fail_cnt, i2c_fail_max_cnt, sensor_fcount);
				ret = -EINVAL;
				goto p_err;
			}
		}
#if defined(USE_RECOVER_I2C_TRANS)
		if (i2c_fail_cnt >= USE_RECOVER_I2C_TRANS) {
			sensor_cis_recover_i2c_fail(subdev);
			err("[mod:d:%d] %s, i2c transfer, forcely power down/up",
				cis->id, __func__);
			ret = -EINVAL;
			goto p_err;
		}
#endif
		usleep_range(CIS_STREAM_OFF_WAIT_TIME, CIS_STREAM_OFF_WAIT_TIME + 10);
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

	/* aeb off during flash operation and forced aeb off when stream is off */
	if (cis_data->pre_hdr_mode != SENSOR_HDR_MODE_SINGLE
		|| cis_data->cur_hdr_mode != SENSOR_HDR_MODE_SINGLE) {
		info("[%s][%s] reset AEB state (pre:%d, cur:%d)\n", __func__, cis->sensor_info->name,
			cis_data->pre_hdr_mode, cis_data->cur_hdr_mode);
		sensor_cis_set_cur_seamless_ctl_aeb_off(cis->cis_data);
	}

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_wait_streamoff);

int sensor_cis_wait_streamoff_mipi_end(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;
	struct is_device_sensor_peri *sensor_peri;
	u32 wait_cnt = 0, time_out_cnt = 250;
	struct is_device_sensor *device;
	struct is_device_csi *csi;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (unlikely(!cis)) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);
	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	csi = v4l2_get_subdevdata(device->subdev_csi);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	/* wait stream off by waiting vblank state */
	do {
		usleep_range(CIS_STREAM_OFF_WAIT_TIME, CIS_STREAM_OFF_WAIT_TIME + 1);
		wait_cnt++;

		if (wait_cnt >= time_out_cnt) {
			err("[MOD:D:%d] %s, time out, wait_limit(%d) > time_out(%d)",
					cis->id, __func__, wait_cnt, time_out_cnt);
			ret = -EINVAL;
			goto p_err;
		}

		dbg_sensor(1, "[MOD:D:%d] %s, wait_limit(%d) < time_out(%d)\n",
				cis->id, __func__, wait_cnt, time_out_cnt);
	} while (csi->sw_checker == EXPECT_FRAME_END);

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_wait_streamoff_mipi_end);

int sensor_cis_wait_streamon(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int ret_err = 0;
	struct is_cis *cis;
	cis_shared_data *cis_data;
	u32 wait_cnt = 0, time_out_cnt = 250;
	u8 sensor_fcount = 0;
	u32 i2c_fail_cnt = 0;

	WARN_ON(!subdev);

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

	if (cis_data->cur_frame_us_time > 300000 && cis_data->cur_frame_us_time < 2000000)
		time_out_cnt = (cis_data->cur_frame_us_time / CIS_STREAM_ON_WAIT_TIME) + 100; // for Hyperlapse night mode

	if (cis_data->dual_slave == true || cis_data->remosaic_mode == true)
		time_out_cnt = time_out_cnt * 6;

	if (cis_data->remosaic_mode == true
		&& (cis_data->cur_frame_us_time > 300000 && cis_data->cur_frame_us_time < 31000000))
		time_out_cnt = (cis_data->cur_frame_us_time / CIS_STREAM_ON_WAIT_TIME) + 100; // for remosaic LTE

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = cis->ixc_ops->read8(cis->client, 0x0005, &sensor_fcount);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);
	if (ret < 0)
		err("i2c transfer fail addr(%x), val(%x), ret = %d\n", 0x0005, sensor_fcount, ret);

	/*
	 * Read sensor frame counter (sensor_fcount address = 0x0005)
	 * stream on (0x00 ~ 0xFE), stream off (0xFF)
	 */
	while (sensor_fcount == 0xff) {
		usleep_range(CIS_STREAM_ON_WAIT_TIME, CIS_STREAM_ON_WAIT_TIME);
		wait_cnt++;

		IXC_MUTEX_LOCK(cis->ixc_lock);
		ret = cis->ixc_ops->read8(cis->client, 0x0005, &sensor_fcount);
		IXC_MUTEX_UNLOCK(cis->ixc_lock);
		if (ret < 0) {
			i2c_fail_cnt++;
			err("i2c transfer fail addr(%x), val(%x), try(%d), ret = %d\n",
				0x0005, sensor_fcount, i2c_fail_cnt, ret);
		}
#if defined(USE_RECOVER_I2C_TRANS)
		if (i2c_fail_cnt >= USE_RECOVER_I2C_TRANS) {
			sensor_cis_recover_i2c_fail(subdev);
			err("[mod:d:%d] %s, i2c transfer, forcely power down/up",
				cis->id, __func__);
			ret = -EINVAL;
			goto p_err;
		}
#endif
		if (wait_cnt >= time_out_cnt) {
			err("[MOD:D:%d] %s, stream on wait failed (%d), wait_cnt(%d) > time_out_cnt(%d)",
				cis->id, __func__, cis_data->cur_frame_us_time, wait_cnt, time_out_cnt);
			err("framecount(%x), dual_slave(%d), remosaic_mode(%d)",
				sensor_fcount, cis_data->dual_slave, cis_data->remosaic_mode);
			ret = -EINVAL;
			goto p_err;
		}

		dbg_sensor(1, "[MOD:D:%d] %s, sensor_fcount(%d), (wait_limit(%d) < time_out(%d))\n",
				cis->id, __func__, sensor_fcount, wait_cnt, time_out_cnt);
	}

	return 0;

p_err:
	CALL_CISOPS(cis, cis_log_status, subdev);

	ret_err = CALL_CISOPS(cis, cis_stream_off, subdev);
	if (ret_err < 0) {
		err("[MOD:D:%d] stream off fail", cis->id);
	} else {
		ret_err = CALL_CISOPS(cis, cis_wait_streamoff, subdev);
		if (ret_err < 0)
			err("[MOD:D:%d] sensor wait stream off fail", cis->id);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_wait_streamon);

int sensor_cis_set_initial_exposure(struct v4l2_subdev *subdev)
{
	struct is_cis *cis;

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (unlikely(!cis)) {
		err("cis is NULL");
		return -EINVAL;
	}

	if (cis->use_initial_ae) {
		cis->init_ae_setting = cis->last_ae_setting;

		dbg_sensor(1, "[MOD:D:%d] %s short(exp:%u/again:%d/dgain:%d), long(exp:%u/again:%d/dgain:%d)\n",
			cis->id, __func__, cis->init_ae_setting.exposure, cis->init_ae_setting.analog_gain,
			cis->init_ae_setting.digital_gain, cis->init_ae_setting.long_exposure,
			cis->init_ae_setting.long_analog_gain, cis->init_ae_setting.long_digital_gain);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_initial_exposure);

int sensor_cis_active_test(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_cis *cis;

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	if (unlikely(!cis)) {
		err("cis is NULL");
		return -EINVAL;
	}

	/* sensor mode setting */
	ret = CALL_CISOPS(cis, cis_set_global_setting, subdev);
	if (ret < 0) {
		err("[%s] cis global setting fail\n", __func__);
		return ret;
	}

	ret = CALL_CISOPS(cis, cis_mode_change, subdev, 0);
	if (ret < 0) {
		err("[%s] cis mode setting(0) fail\n", __func__);
		return ret;
	}

	/* sensor stream on */
	ret = CALL_CISOPS(cis, cis_stream_on, subdev);
	if (ret < 0) {
		err("[%s] stream on fail\n", __func__);
		return ret;
	}

	ret = CALL_CISOPS(cis, cis_wait_streamon, subdev);
	if (ret < 0) {
		err("[%s] sensor wait stream on fail\n", __func__);
		return ret;
	}

	msleep(100);

	/* Sensor stream off */
	ret = CALL_CISOPS(cis, cis_stream_off, subdev);
	if (ret < 0) {
		err("[%s] stream off fail\n", __func__);
		return ret;
	}

	ret = CALL_CISOPS(cis, cis_wait_streamoff, subdev);
	if (ret < 0) {
		err("[%s] stream off fail\n", __func__);
		return ret;
	}

	info("[MOD:D:%d] %s: %d\n", cis->id, __func__, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_active_test);

int sensor_cis_get_bayer_order(struct v4l2_subdev *subdev, u32 *bayer_order)
{

	struct is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	*bayer_order = cis->bayer_order;
	info("[MOD:D:%d] %s cis->bayer_order: %d\n", cis->id, __func__, cis->bayer_order);
	return 0;
}
EXPORT_SYMBOL_GPL(sensor_cis_get_bayer_order);

void sensor_cis_log_status(struct is_cis *cis,
	const struct is_cis_log *log, unsigned int num, char *fn)
{
	int ret;
	int i;
	u16 data;

	warn("[%s] *******************************", fn);

	IXC_MUTEX_LOCK(cis->ixc_lock);

	for (i = 0; i < num; i++) {
		data = 0;
		if  (log[i].type == I2C_WRITE) {
			if  (log[i].size == 8)
				ret = cis->ixc_ops->write8(cis->client, log[i].addr, log[i].data);
			else
				ret = cis->ixc_ops->write16(cis->client, log[i].addr, log[i].data);

			if (!ret && log[i].name)
				warn("%s", log[i].name);
		} else {
			if  (log[i].size == 8)
				ret = cis->ixc_ops->read8(cis->client, log[i].addr, (u8 *)&data);
			else
				ret = cis->ixc_ops->read16(cis->client, log[i].addr, &data);

			if (!ret)
				warn("%s(0x%x)", log[i].name, data);
		}

		if (ret) {
			err("%s: i2c %s fail", log[i].name,
				(log[i].type == I2C_WRITE) ? "write" : "read");
			goto i2c_err;
		}
	}

i2c_err:
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	warn("[%s] *******************************", fn);
}
EXPORT_SYMBOL_GPL(sensor_cis_log_status);

int sensor_cis_set_test_pattern(struct v4l2_subdev *subdev, camera2_sensor_ctl_t *sensor_ctl)
{
	int ret = 0;
	struct is_cis *cis;

	cis = (struct is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (unlikely(!cis->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, cur_pattern_mode(%d), testPatternMode(%d)\n", cis->id, __func__,
			cis->cis_data->cur_pattern_mode, sensor_ctl->testPatternMode);

	if (cis->cis_data->cur_pattern_mode != sensor_ctl->testPatternMode) {
		if (sensor_ctl->testPatternMode == SENSOR_TEST_PATTERN_MODE_OFF) {
			info("%s: set DEFAULT pattern! (testpatternmode : %d)\n", __func__, sensor_ctl->testPatternMode);

			IXC_MUTEX_LOCK(cis->ixc_lock);
			cis->ixc_ops->write16(cis->client, 0x0600, 0x0000);
			IXC_MUTEX_UNLOCK(cis->ixc_lock);

			cis->cis_data->cur_pattern_mode = sensor_ctl->testPatternMode;
		} else if (sensor_ctl->testPatternMode == SENSOR_TEST_PATTERN_MODE_BLACK) {
			info("%s: set BLACK pattern! (testpatternmode :%d), Data : 0x(%x, %x, %x, %x)\n",
				__func__, sensor_ctl->testPatternMode,
				(unsigned short)sensor_ctl->testPatternData[0],
				(unsigned short)sensor_ctl->testPatternData[1],
				(unsigned short)sensor_ctl->testPatternData[2],
				(unsigned short)sensor_ctl->testPatternData[3]);

			IXC_MUTEX_LOCK(cis->ixc_lock);
			cis->ixc_ops->write16(cis->client, 0x0600, 0x0001);
			IXC_MUTEX_UNLOCK(cis->ixc_lock);

			cis->cis_data->cur_pattern_mode = sensor_ctl->testPatternMode;
		}
	}

p_err:
	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_test_pattern);

int sensor_cis_complete_probe(struct is_device_sensor_peri *sensor_peri)
{
	int ret;
	struct is_module_enum *module = sensor_peri->module;

	/* i3c_probe was completed. */
	if (delayed_work_pending(&module->hotjoin_chk_dwork))
		cancel_delayed_work_sync(&module->hotjoin_chk_dwork);

	ret = module->pdata->gpio_cfg(module, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF);
	if (ret) {
		err("gpio_cfg is fail(%d)", ret);
		return ret;
	}
	clear_bit(IS_MODULE_GPIO_ON, &module->state);
	module->pdata->cis_i3c_probed = true;

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_complete_probe);

int sensor_cis_wait_delayed_gph_release(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_device_sensor *device;
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	cis_shared_data *cis_data = cis->cis_data;
	bool use_delayed_gph_release = false;
	u64 cur_vsync_ts, delay_ts_us;
	unsigned int base_vsync;
	int retry;

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	if (cis->sensor_info->use_delayed_gph_release_aeb_off
		&& cis_data->pre_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC
		&& cis_data->cur_hdr_mode != SENSOR_HDR_MODE_2AEB_2VC) {
		use_delayed_gph_release = true;
	}

	if (use_delayed_gph_release) {
		base_vsync = cis_data->sen_vsync_count;
		cur_vsync_ts = device->timestamp[base_vsync % IS_TIMESTAMP_HASH_KEY];
		delay_ts_us = (ktime_get_ns() - cur_vsync_ts) / 1000;

		info("[%d][%d][%s] SS_DBG delayed gph release, delay_ts_us[%lldms] vsync[%d]\n",
			cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			delay_ts_us, base_vsync);

		retry = 0;
		while ((base_vsync == cis_data->sen_vsync_count) && retry < 33) {
			usleep_range(1000, 1010);
			retry++;
		}

		info("[%d][%d][%s] SS_DBG delayed gph release, vsync[%d]\n",
			cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			cis_data->sen_vsync_count);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_wait_delayed_gph_release);

int sensor_cis_set_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	cis_shared_data *cis_data = cis->cis_data;

	if (!cis->sensor_info->use_group_param_hold)
		return ret;

	if (cis_data->is_data.scene_mode == AA_SCENE_MODE_FAST_AE)
		return ret;

	if (hold == cis_data->group_param_hold) {
		dbg_sensor(1, "%s : already group_param_hold (%d)\n", __func__, cis_data->group_param_hold);
		return ret;
	}

	if (cis_data->stream_on == false && hold == true) {
		dbg_sensor(1, "%s : already stream off\n", __func__);
		return ret;
	}

	if (!hold)
		sensor_cis_wait_delayed_gph_release(subdev);

	IXC_MUTEX_LOCK(cis->ixc_lock);
	ret = cis->ixc_ops->write8(cis->client, cis->reg_addr->group_param_hold, hold);
	IXC_MUTEX_UNLOCK(cis->ixc_lock);

	if (IS_ENABLED_PRINT_CONTROL_LOG(cis))
		info("[%d][%d][%s] SS_DBG GPH[%d]=>[0x%04X,0x%02X]\n",
			cis->id, cis_data->sen_vsync_count, cis->sensor_info->name,
			hold, cis->reg_addr->group_param_hold, hold);

	dbg_sensor(1, "[%s][%d][%s] GPH[%d], ret[%d]\n",
		__func__, cis_data->sen_vsync_count, cis->sensor_info->name, hold, ret);
	if (ret < 0)
		return ret;

	cis_data->group_param_hold = hold;
	ret = hold;

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_group_param_hold);

int sensor_cis_probe(void *client, struct device *client_dev,
		struct is_device_sensor_peri **sensor_peri, enum ixc_type type)
{
	struct is_device_sensor_peri *sp;
	struct device *dev;
	struct device_node *dnode;
	u32 module_phandle;
	struct device_node *module_node;
	struct platform_device *module_pdev;
	int ret;
	struct is_core *core;
	struct v4l2_subdev *subdev_cis;
	struct is_cis *cis;
	struct is_device_sensor *device;
	u32 sensor_dev_id;

	core = is_get_is_core();
	if (!core) {
		probe_info("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = client_dev;
	dnode = dev->of_node;

	sp = dev_get_platdata(client_dev);
	if (sp) {
		*sensor_peri = sp;
		return 0;
	}

	ret = of_property_read_u32(dnode, "sensor-module", &module_phandle);
	if (ret) {
		probe_err("There is no sensor-module in DT (%d)", ret);
		return -EINVAL;
	}

	module_node = of_find_node_by_phandle(module_phandle);
	if (!module_node) {
		probe_err("It cannot find module node in DT");
		return -EINVAL;
	}

	module_pdev = of_find_device_by_node(module_node);
	if (!module_pdev) {
		probe_err("There is no module platform_device of module");
		return -ENODEV;
	}

	sp = platform_get_drvdata(module_pdev);
	if (!sp) {
		probe_info("%s, sensor peri is not yet probed", __func__);
		return -EPROBE_DEFER;
	}

	*sensor_peri = sp;

	ret = of_property_read_u32(dnode, "id", &sensor_dev_id);
	if (ret) {
		err("sensor id read is fail(%d)", ret);
		return -EINVAL;
	}

	probe_info("%s: sensor dev id %d\n", __func__, sensor_dev_id);

	device = &core->sensor[sensor_dev_id];

	cis = &sp->cis;
	subdev_cis = devm_kzalloc(dev, sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_cis) {
		probe_err("subdev_cis is NULL");
		return -ENOMEM;
	}

	cis->subdev = subdev_cis;
	cis->id = sensor_dev_id;
	cis->client = client;
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;
	cis->cis_data = devm_kzalloc(dev, sizeof(cis_shared_data), GFP_KERNEL);
	if (!cis->cis_data) {
		err("cis_data is NULL");
		ret = -ENOMEM;
		goto err_alloc_cis_data;
	}

	cis->use_dgain = true;
	cis->hdr_ctrl_by_again = false;

	switch(type) {
	case I2C_TYPE:
		cis->ixc_ops = pablo_get_i2c();
		break;
	case I3C_TYPE:
		cis->ixc_ops = pablo_get_i3c();
		break;
	default:
		break;
	}

	v4l2_set_subdevdata(subdev_cis, cis);
	v4l2_set_subdev_hostdata(subdev_cis, device);
	snprintf(subdev_cis->name, V4L2_SUBDEV_NAME_SIZE, "cis-subdev.%d", cis->id);

	sensor_cis_parse_dt(dev, cis->subdev);

	sp->subdev_cis = subdev_cis;
	sp->module->client = cis->client;

	return 0;

err_alloc_cis_data:
	kfree(subdev_cis);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_probe);

int sensor_cis_set_flip_mode(struct v4l2_subdev *subdev)
{
	int ret;
	struct is_device_sensor *device;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_cis *cis = sensor_cis_get_cis(subdev);

	enum camera_flip_mode flip_mode;

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	sensor_peri = container_of(cis, struct is_device_sensor_peri, cis);

	flip_mode = cis->cis_data->is_data.flip_mode;

	info("[%s][%d][%s] flip_mode(%d)\n",
		__func__, cis->id, cis->sensor_info->name, flip_mode);

	if (flip_mode < 0 || flip_mode >= CAM_FLIP_MODE_MAX) {
		err("[%s] flip_mode(%d) is out of range", __func__, flip_mode);
		return -EINVAL;
	}

	ret = sensor_cis_write_registers_locked(subdev, cis->sensor_info->filp_mode[flip_mode]);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_flip_mode);

int sensor_cis_seamless_ctl_before_stream(struct v4l2_subdev *subdev,
	enum is_rta_sensor_mode sensor_mode, u32 value)
{
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	cis_shared_data *cis_data = cis->cis_data;
	struct is_device_sensor *device = NULL;
	struct is_sensor_interface *itf = NULL;

	device = (struct is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!device);

	itf = is_sensor_get_sensor_interface(device);
	WARN_ON(!itf);

	switch (sensor_mode) {
	case SENSOR_MODE_LN2:
		break;
	case SENSOR_MODE_LN4:
		break;
	case SENSOR_MODE_REAL_12BIT:
		break;
	case SENSOR_MODE_CROPPED_RMS:
		cis_data->seamless_ctl_before_stream.remosaic_crop_zoom_ratio = value;
		itf->cis_ext2_itf_ops.set_remosaic_zoom_ratio(itf, value);
		break;
	case SENSOR_MODE_AEB:
		break;
	default:
		err("[%d][%s] not support sensor_mode(%d), value[%d]\n",
			cis_data->sen_vsync_count, cis->sensor_info->name,
			sensor_mode, value);
		return 0;
	}

	cis_data->seamless_ctl_before_stream.mode |= sensor_mode;

	ssinfo_seamless_mode(cis, &cis_data->seamless_ctl_before_stream);

	return 0;
}
EXPORT_SYMBOL_GPL(sensor_cis_seamless_ctl_before_stream);

int sensor_cis_wait_seamless_update_delay(struct v4l2_subdev *subdev)
{
	struct is_cis *cis = sensor_cis_get_cis(subdev);
	cis_shared_data *cis_data = cis->cis_data;
	unsigned int seamless_vsync_done_cnt;
	u32 wait_interval = 10;
	u32 wait_cnt = 0;
	u32 max_cnt = 1000;

	if (cis_data->seamless_update_vsync_cnt == 0)
		return 0;

	seamless_vsync_done_cnt
		= cis_data->seamless_update_vsync_cnt + cis->sensor_info->seamless_wait_cnt;

	if (seamless_vsync_done_cnt > cis_data->sen_vsync_count) {
		do {
			info("[%s][%s] seamless_vsync_done_cnt(%d) cur_fs_count(%d)\n",
				__func__, cis->sensor_info->name,
				seamless_vsync_done_cnt, cis_data->sen_vsync_count);
			if (wait_cnt >= max_cnt) {
				err("fail wait_seamless_update_delay");
				return -EINVAL;
			}

			msleep(wait_interval);
			wait_cnt++;
		} while (seamless_vsync_done_cnt > cis_data->sen_vsync_count);
	}

	cis_data->seamless_update_vsync_cnt = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(sensor_cis_wait_seamless_update_delay);

void sensor_cis_set_cur_seamless_ctl(cis_shared_data *cis_data)
{
	memset(&cis_data->cur_seamless_ctl, 0, sizeof(struct is_sensor_seamless_ctl));

	if (cis_data->cur_12bit_mode == SENSOR_12BIT_STATE_REAL_12BIT)
		cis_data->cur_seamless_ctl.mode |= SENSOR_MODE_REAL_12BIT;

	if (cis_data->cur_lownoise_mode == IS_CIS_LN2)
		cis_data->cur_seamless_ctl.mode |= SENSOR_MODE_LN2;
	else if (cis_data->cur_lownoise_mode == IS_CIS_LN4)
		cis_data->cur_seamless_ctl.mode |= SENSOR_MODE_LN4;
	else if (cis_data->cur_lownoise_mode == IS_CIS_ADC11)
		cis_data->cur_seamless_ctl.mode |= SENSOR_MODE_ADC11;

	if (cis_data->cur_remosaic_zoom_ratio > 0) {
		cis_data->cur_seamless_ctl.mode |= SENSOR_MODE_CROPPED_RMS;
		cis_data->cur_seamless_ctl.remosaic_crop_zoom_ratio = cis_data->cur_remosaic_zoom_ratio;
	}

	if (cis_data->cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC)
		cis_data->cur_seamless_ctl.mode |= SENSOR_MODE_AEB;
}
EXPORT_SYMBOL_GPL(sensor_cis_set_cur_seamless_ctl);

void sensor_cis_set_cur_seamless_ctl_aeb_off(cis_shared_data *cis_data)
{
	cis_data->pre_hdr_mode = cis_data->cur_hdr_mode = SENSOR_HDR_MODE_SINGLE;
	cis_data->cur_seamless_ctl.mode &= ~SENSOR_MODE_AEB;

	memcpy(&cis_data->seamless_history[cis_data->sen_vsync_count % NUM_FRAMES],
		&cis_data->cur_seamless_ctl, sizeof(struct is_sensor_seamless_ctl));
}
EXPORT_SYMBOL_GPL(sensor_cis_set_cur_seamless_ctl_aeb_off);

