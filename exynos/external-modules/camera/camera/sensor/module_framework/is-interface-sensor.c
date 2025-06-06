/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>

#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>

#include "is-control-sensor.h"
#include "is-device-sensor-peri.h"
#include "is-cis.h"
#include "is-interface-sensor.h"
#include "is-vendor-private.h"
#include "pablo-obte.h"
#include "pablo-fpsimd.h"
#include "exynos-is-module.h"
#include "is-device-csi.h"

static u8 rta_static_data[STATIC_DATA_SIZE];
static u8 ddk_static_data[STATIC_DATA_SIZE];

/* helper functions */
struct is_module_enum *get_subdev_module_enum(struct is_sensor_interface *itf)
{
	struct is_device_sensor_peri *sensor_peri;

	if (unlikely(!itf)) {
		err("invalid sensor interface");
		return NULL;
	}

	FIMC_BUG_NULL(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	return sensor_peri->module;
}

static inline struct is_device_sensor *get_device_sensor(struct is_sensor_interface *itf)
{
	struct is_module_enum *module;
	struct v4l2_subdev *subdev_module;

	module = get_subdev_module_enum(itf);
	if (unlikely(!module)) {
		err("failed to get sensor_peri's module");
		return NULL;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module's subdev was not probed");
		return NULL;
	}

	return (struct is_device_sensor *)
			v4l2_get_subdev_hostdata(subdev_module);
}

static inline struct is_device_csi *get_subdev_csi(struct is_sensor_interface *itf)
{
	struct is_device_sensor *device;

	device = get_device_sensor(itf);
	if (!device) {
		err("failed to get sensor device");
		return NULL;
	}

	return (struct is_device_csi *)
			v4l2_get_subdevdata(device->subdev_csi);
}

struct is_actuator *get_subdev_actuator(struct is_sensor_interface *itf)
{
	struct is_device_sensor_peri *sensor_peri;

	if (unlikely(!itf)) {
		err("invalid sensor interface");
		return NULL;
	}

	FIMC_BUG_NULL(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	return (struct is_actuator *)
			v4l2_get_subdevdata(sensor_peri->subdev_actuator);

}

int sensor_get_ctrl(struct is_sensor_interface *itf,
			u32 ctrl_id, u32 *val)
{
	int ret = 0;
	struct is_device_sensor *device = NULL;
	struct is_module_enum *module = NULL;
	struct v4l2_subdev *subdev_module = NULL;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct v4l2_control ctrl;

	if (unlikely(!itf)) {
		err("interface in is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!val);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module in is NULL");
		module = NULL;
		goto p_err;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module is not probed");
		subdev_module = NULL;
		goto p_err;
	}

	device = v4l2_get_subdev_hostdata(subdev_module);
	if (unlikely(!device)) {
		err("device in is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ctrl.id = ctrl_id;
	ctrl.value = -1;
	ret = is_sensor_g_ctrl(device, &ctrl);
	*val = (u32)ctrl.value;
	if (ret < 0) {
		err("err!!! ret(%d), return_value(%d)", ret, *val);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

/* if target param has only one value(such as CIS_SMIA mode or flash_intensity),
   then set value to long_val, will be ignored short_val */
int set_interface_param(struct is_sensor_interface *itf,
			enum itf_cis_interface mode,
			enum itf_param_type target,
			u32 index,
			enum is_exposure_gain_count num_data,
			u32 *val)
{
	int ret = 0;
	int exp_gain_type;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	if (mode != ITF_CIS_SMIA && mode != ITF_CIS_SMIA_WDR) {
		err("invalid mode (%d)", mode);
		return -EINVAL;
	}

	if (index >= NUM_FRAMES) {
		err("invalid frame index (%d)", index);
		return -EINVAL;
	}

	if (num_data <= EXPOSURE_GAIN_COUNT_INVALID || num_data >= EXPOSURE_GAIN_COUNT_END) {
		err("invalid num_data(%d)", num_data);
		return -EINVAL;
	}

	for (exp_gain_type = 0; exp_gain_type < num_data; exp_gain_type++) {
		switch (target) {
		case ITF_CIS_PARAM_TOTAL_GAIN:
			itf->total_gain[exp_gain_type][index] = val[exp_gain_type];
			dbg_sensor(1, "%s: total gain [T:%d][I:%d]: %d\n",
					__func__, exp_gain_type, index,
					itf->total_gain[exp_gain_type][index]);
			break;
		case ITF_CIS_PARAM_ANALOG_GAIN:
			itf->analog_gain[exp_gain_type][index] = val[exp_gain_type];
			dbg_sensor(1, "%s: again [T:%d][I:%d]: %d\n",
					__func__, exp_gain_type, index,
					itf->analog_gain[exp_gain_type][index]);
			break;
		case ITF_CIS_PARAM_DIGITAL_GAIN:
			itf->digital_gain[exp_gain_type][index] = val[exp_gain_type];
			dbg_sensor(1, "%s: dgain [T:%d][I:%d]: %d\n",
					__func__, exp_gain_type, index,
					itf->digital_gain[exp_gain_type][index]);
			break;
		case ITF_CIS_PARAM_EXPOSURE:
			itf->exposure[exp_gain_type][index] = val[exp_gain_type];
			dbg_sensor(1, "%s: exposure [T:%d][I:%d]: %d\n",
					__func__, exp_gain_type, index,
					itf->exposure[exp_gain_type][index]);
			break;
		case ITF_CIS_PARAM_FLASH_INTENSITY:
			itf->flash_intensity[index] = val[exp_gain_type];
			break;
		default:
			err("invalid target (%d)", target);
			ret = -EINVAL;
			goto p_err;
		}
	}

p_err:
	return ret;
}

int get_interface_param(struct is_sensor_interface *itf,
			enum itf_cis_interface mode,
			enum itf_param_type target,
			u32 index,
			enum is_exposure_gain_count num_data,
			u32 *val)
{
	int exp_gain_type;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	if (index >= NUM_FRAMES) {
		pr_err("[%s] invalid frame index (%d)\n", __func__, index);
		return -EINVAL;
	}

	if (num_data <= EXPOSURE_GAIN_COUNT_INVALID || num_data >= EXPOSURE_GAIN_COUNT_END) {
		err("invalid num_data(%d)", num_data);
		return -EINVAL;
	}

	for (exp_gain_type = 0; exp_gain_type < num_data; exp_gain_type++) {
		switch (target) {
		case ITF_CIS_PARAM_TOTAL_GAIN:
			val[exp_gain_type] = itf->total_gain[exp_gain_type][index];
			dbg_sensor(1, "%s: total gain [T:%d][I:%d]: %d\n",
					__func__, exp_gain_type, index,
					itf->total_gain[exp_gain_type][index]);
			break;
		case ITF_CIS_PARAM_ANALOG_GAIN:
			val[exp_gain_type] = itf->analog_gain[exp_gain_type][index];
			dbg_sensor(1, "%s: again [T:%d][I:%d]: %d\n",
					__func__, exp_gain_type, index,
					itf->analog_gain[exp_gain_type][index]);
			break;
		case ITF_CIS_PARAM_DIGITAL_GAIN:
			val[exp_gain_type] = itf->digital_gain[exp_gain_type][index];
			dbg_sensor(1, "%s: dgain [T:%d][I:%d]: %d\n",
					__func__, exp_gain_type, index,
					itf->digital_gain[exp_gain_type][index]);
			break;
		case ITF_CIS_PARAM_EXPOSURE:
			val[exp_gain_type] = itf->exposure[exp_gain_type][index];
			dbg_sensor(1, "%s: exposure [T:%d][I:%d]: %d\n",
					__func__, exp_gain_type, index,
					itf->exposure[exp_gain_type][index]);
			break;
		case ITF_CIS_PARAM_FLASH_INTENSITY:
			val[exp_gain_type] = itf->flash_intensity[index];
			break;
		default:
			err("invalid target (%d)", target);
			return -EINVAL;
		}
	}

	return 0;
}

u32 get_vsync_count(struct is_sensor_interface *itf);

u32 get_frame_count(struct is_sensor_interface *itf)
{
	u32 frame_count = 0;

	frame_count = get_vsync_count(itf);

	/* Frame count have to start at 1 */
	if (frame_count == 0)
		frame_count = 1;

	return frame_count;
}

struct is_sensor_ctl *get_sensor_ctl_from_module(struct is_sensor_interface *itf,
							u32 frame_count)
{
	u32 index = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_NULL(!itf);
	FIMC_BUG_NULL(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG_NULL(!sensor_peri);

	index = frame_count % CAM2P0_UCTL_LIST_SIZE;

	return &sensor_peri->cis.sensor_ctls[index];
}

camera2_sensor_uctl_t *get_sensor_uctl_from_module(struct is_sensor_interface *itf,
							u32 frame_count)
{
	struct is_sensor_ctl *sensor_ctl = NULL;

	FIMC_BUG_NULL(!itf);
	FIMC_BUG_NULL(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_ctl = get_sensor_ctl_from_module(itf, frame_count);

	/* TODO: will be moved to report_sensor_done() */
	sensor_ctl->sensor_frame_number = frame_count;

	return &sensor_ctl->cur_cam20_sensor_udctrl;
}

void set_sensor_uctl_valid(struct is_sensor_interface *itf,
						u32 frame_count)
{
	struct is_sensor_ctl *sensor_ctl = NULL;

	FIMC_BUG_VOID(!itf);
	FIMC_BUG_VOID(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_ctl = get_sensor_ctl_from_module(itf, frame_count);

	sensor_ctl->is_valid_sensor_udctrl = true;
}

int get_num_of_frame_per_one_3aa(struct is_sensor_interface *itf,
				u32 *num_of_frame);
int set_exposure(struct is_sensor_interface *itf,
		enum itf_cis_interface mode,
		enum is_exposure_gain_count num_data,
		u32 *exposure)
{
	int ret = 0;
	u32 frame_count = 0;
	camera2_sensor_uctl_t *sensor_uctl;
	u32 i = 0;
	u32 num_of_frame = 1;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	frame_count = get_frame_count(itf);
	ret = get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_uctl = get_sensor_uctl_from_module(itf, frame_count + i);
		FIMC_BUG(!sensor_uctl);

		is_sensor_ctl_update_exposure_to_uctl(sensor_uctl, num_data, exposure);

		set_sensor_uctl_valid(itf, frame_count);
	}

	return ret;
}

int set_gain_permile(struct is_sensor_interface *itf,
		enum itf_cis_interface mode,
		enum is_exposure_gain_count num_data,
		u32 *total_gain,
		u32 *analog_gain,
		u32 *digital_gain)
{
	int ret = 0;
	u32 frame_count = 0;
	camera2_sensor_uctl_t *sensor_uctl;
	u32 i = 0;
	u32 num_of_frame = 1;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!total_gain);
	FIMC_BUG(!analog_gain);
	FIMC_BUG(!digital_gain);

	frame_count = get_frame_count(itf);

	ret = get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_uctl = get_sensor_uctl_from_module(itf, frame_count + i);
		FIMC_BUG(!sensor_uctl);

		is_sensor_ctl_update_gain_to_uctl(sensor_uctl, num_data, analog_gain, digital_gain);

		set_sensor_uctl_valid(itf, frame_count);
	}

	return ret;
}

/* new APIs */
int request_reset_interface(struct is_sensor_interface *itf,
				u32 exposure,
				u32 total_gain,
				u32 analog_gain,
				u32 digital_gain)
{
	/* NOT USED */

	return 0;
}

int get_calibrated_size(struct is_sensor_interface *itf,
			u32 *width,
			u32 *height)
{
	int ret = 0;
	struct is_module_enum *module = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!width);
	FIMC_BUG(!height);

	module = get_subdev_module_enum(itf);
	FIMC_BUG(!module);

	*width = module->pixel_width;
	*height = module->pixel_height;

	pr_debug("%s, width(%d), height(%d)\n", __func__, *width, *height);

	return ret;
}

int get_bayer_order(struct is_sensor_interface *itf,
			u32 *bayer_order)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!bayer_order);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	*bayer_order = sensor_peri->cis.bayer_order;

	return ret;
}

u32 get_min_exposure_time(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 exposure = 0;

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MIN_EXPOSURE_TIME, &exposure);
	if (ret < 0 || exposure == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, exposure);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) min exp(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), exposure);

p_err:
	return exposure;
}

u32 get_max_exposure_time(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 exposure = 0;

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MAX_EXPOSURE_TIME, &exposure);
	if (ret < 0 || exposure == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, exposure);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) max exp(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), exposure);

p_err:
	return exposure;
}

u32 get_min_analog_gain(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 again = 0;

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MIN_ANALOG_GAIN, &again);
	if (ret < 0 || again == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, again);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) min analog gain(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), again);

p_err:
	return again;
}

u32 get_max_analog_gain(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 again = 0;

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MAX_ANALOG_GAIN, &again);
	if (ret < 0 || again == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, again);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) max analog gain(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), again);

p_err:
	return again;
}

u32 get_min_digital_gain(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 dgain = 0;

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MIN_DIGITAL_GAIN, &dgain);
	if (ret < 0 || dgain == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, dgain);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) min digital gain(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), dgain);

p_err:
	return dgain;
}

u32 get_max_digital_gain(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 dgain = 0;

	ret = sensor_get_ctrl(itf, V4L2_CID_SENSOR_GET_MAX_DIGITAL_GAIN, &dgain);
	if (ret < 0 || dgain == 0) {
		err("err!!! ret(%d), return_value(%d)", ret, dgain);
		goto p_err;
	}

	dbg_sensor(2, "%s:(%d:%d) max digital gain(%d)\n", __func__, get_vsync_count(itf), get_frame_count(itf), dgain);

p_err:
	return dgain;
}


u32 get_vsync_count(struct is_sensor_interface *itf)
{
	struct is_device_csi *csi;

	csi = get_subdev_csi(itf);
	if (unlikely(!csi))
		return 0;

	return atomic_read(&csi->fcount);
}

u32 get_vblank_count(struct is_sensor_interface *itf)
{
	struct is_device_csi *csi;

	csi = get_subdev_csi(itf);
	if (unlikely(!csi))
		return 0;

	return atomic_read(&csi->vblank_count);
}

bool is_vvalid_period(struct is_sensor_interface *itf)
{
	struct is_device_csi *csi;

	FIMC_BUG_FALSE(!itf);

	csi = get_subdev_csi(itf);
	FIMC_BUG_FALSE(!csi);

	return atomic_read(&csi->vvalid) <= 0 ? false : true;
}

int request_exposure(struct is_sensor_interface *itf,
	enum is_exposure_gain_count num_data, u32 *exposure)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	if (num_data <= EXPOSURE_GAIN_COUNT_INVALID || num_data >= EXPOSURE_GAIN_COUNT_END) {
		err("invalid num_data(%d)", num_data);
		return -EINVAL;
	}

	dbg_sensor(1, "[%s](%d:%d) cnt(%d): exposure(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		exposure[EXPOSURE_GAIN_LONG],
		exposure[EXPOSURE_GAIN_SHORT],
		exposure[EXPOSURE_GAIN_MIDDLE]);

	end_index = (itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA);

	i = (itf->vsync_flag == false ? 0 : end_index);
	for (; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, i, num_data, exposure);
		if (ret < 0) {
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}
	itf->vsync_flag = true;

	/* set exposure */
	if (itf->otf_flag_3aa == true) {
		ret = set_exposure(itf, itf->cis_mode, num_data, exposure);
		if (ret < 0) {
			pr_err("[%s] set_exposure fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}

	/* store exposure for use initial AE */
	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	if (sensor_peri->cis.use_initial_ae) {
		sensor_peri->cis.last_ae_setting.exposure = exposure[EXPOSURE_GAIN_LONG];
		sensor_peri->cis.last_ae_setting.long_exposure = exposure[EXPOSURE_GAIN_LONG];
		sensor_peri->cis.last_ae_setting.short_exposure = exposure[EXPOSURE_GAIN_SHORT];
		sensor_peri->cis.last_ae_setting.middle_exposure = exposure[EXPOSURE_GAIN_MIDDLE];
	}
p_err:
	return ret;
}

int adjust_exposure(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *exposure,
			u32 *available_exposure,
			is_sensor_adjust_direction adjust_direction)
{
	/* NOT IMPLEMENTED YET */
	int ret = -1;

	dbg_sensor(1, "[%s] NOT IMPLEMENTED YET\n", __func__);

	return ret;
}

int get_next_frame_timing(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *exposure,
			u32 *frame_period,
			u64 *line_period)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!exposure);
	FIMC_BUG(!frame_period);
	FIMC_BUG(!line_period);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, NEXT_FRAME, num_data, exposure);
	if (ret < 0)
		pr_err("[%s] get_interface_param EXPOSURE fail(%d)\n", __func__, ret);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*frame_period = sensor_peri->cis.cis_data->frame_time;
	*line_period = sensor_peri->cis.cis_data->line_readOut_time;

	dbg_sensor(2, "[%s](%d:%d) cnt(%d): exp(L(%d), S(%d), M(%d)), frame_period %d, line_period %lld\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		exposure[EXPOSURE_GAIN_LONG],
		exposure[EXPOSURE_GAIN_SHORT],
		exposure[EXPOSURE_GAIN_MIDDLE],
		*frame_period, *line_period);

	return ret;
}

int get_frame_timing(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *exposure,
			u32 *frame_period,
			u64 *line_period)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!exposure);
	FIMC_BUG(!frame_period);
	FIMC_BUG(!line_period);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, CURRENT_FRAME, num_data, exposure);
	if (ret < 0)
		pr_err("[%s] get_interface_param EXPOSURE fail(%d)\n", __func__, ret);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*frame_period = sensor_peri->cis.cis_data->frame_time;
	*line_period = sensor_peri->cis.cis_data->line_readOut_time;

	dbg_sensor(2, "[%s](%d:%d) cnt(%d): exp(L(%d), S(%d), M(%d)), frame_period %d, line_period %lld\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		exposure[EXPOSURE_GAIN_LONG],
		exposure[EXPOSURE_GAIN_SHORT],
		exposure[EXPOSURE_GAIN_MIDDLE],
		*frame_period, *line_period);

	return ret;
}

int request_analog_gain(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *analog_gain)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!analog_gain);

	dbg_sensor(1, "[%s](%d:%d) cnt(%d): analog_gain(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		analog_gain[EXPOSURE_GAIN_LONG],
		analog_gain[EXPOSURE_GAIN_SHORT],
		analog_gain[EXPOSURE_GAIN_MIDDLE]);

	end_index = (itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA);

	i = (itf->vsync_flag == false ? 0 : end_index);
	for (; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, i, num_data, analog_gain);
		if (ret < 0) {
			pr_err("[%s] set_interface_param EXPOSURE fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}

p_err:
	return ret;
}

int request_gain(struct is_sensor_interface *itf,
		enum is_exposure_gain_count num_data,
		u32 *total_gain,
		u32 *analog_gain,
		u32 *digital_gain)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!total_gain);
	FIMC_BUG(!analog_gain);
	FIMC_BUG(!digital_gain);

	dbg_sensor(1, "[%s](%d:%d) cnt(%d): total_gain(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		total_gain[EXPOSURE_GAIN_LONG],
		total_gain[EXPOSURE_GAIN_SHORT],
		total_gain[EXPOSURE_GAIN_MIDDLE]);
	dbg_sensor(1, "[%s](%d:%d) cnt(%d): analog_gain(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		analog_gain[EXPOSURE_GAIN_LONG],
		analog_gain[EXPOSURE_GAIN_SHORT],
		analog_gain[EXPOSURE_GAIN_MIDDLE]);
	dbg_sensor(1, "[%s](%d:%d) cnt(%d): digital_gain(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		digital_gain[EXPOSURE_GAIN_LONG],
		digital_gain[EXPOSURE_GAIN_SHORT],
		digital_gain[EXPOSURE_GAIN_MIDDLE]);

	end_index = (itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA);

	i = (itf->vsync_flag == false ? 0 : end_index);
	for (; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_TOTAL_GAIN, i, num_data, total_gain);
		if (ret < 0) {
			pr_err("[%s] set_interface_param total gain fail(%d)\n", __func__, ret);
			goto p_err;
		}
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, i, num_data, analog_gain);
		if (ret < 0) {
			pr_err("[%s] set_interface_param analog gain fail(%d)\n", __func__, ret);
			goto p_err;
		}
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN, i, num_data, digital_gain);
		if (ret < 0) {
			pr_err("[%s] set_interface_param digital gain fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}

	/* set gain permile */
	if (itf->otf_flag_3aa == true) {
		ret = set_gain_permile(itf, itf->cis_mode, num_data,
				total_gain, analog_gain, digital_gain);
		if (ret < 0) {
			pr_err("[%s] set_gain_permile fail(%d)\n", __func__, ret);
			goto p_err;
		}
	}

	/* store gain for use initial AE */
	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	if (sensor_peri->cis.use_initial_ae) {
		sensor_peri->cis.last_ae_setting.analog_gain = analog_gain[EXPOSURE_GAIN_LONG];
		sensor_peri->cis.last_ae_setting.digital_gain = digital_gain[EXPOSURE_GAIN_LONG];
		sensor_peri->cis.last_ae_setting.long_analog_gain = analog_gain[EXPOSURE_GAIN_LONG];
		sensor_peri->cis.last_ae_setting.long_digital_gain = digital_gain[EXPOSURE_GAIN_LONG];
		sensor_peri->cis.last_ae_setting.short_analog_gain = analog_gain[EXPOSURE_GAIN_SHORT];
		sensor_peri->cis.last_ae_setting.short_digital_gain = digital_gain[EXPOSURE_GAIN_SHORT];
		sensor_peri->cis.last_ae_setting.middle_analog_gain = analog_gain[EXPOSURE_GAIN_MIDDLE];
		sensor_peri->cis.last_ae_setting.middle_digital_gain = digital_gain[EXPOSURE_GAIN_MIDDLE];
	}
p_err:
	return ret;
}

int request_sensitivity(struct is_sensor_interface *itf,
		u32 sensitivity)
{
	int ret = 0;
	u32 frame_count = 0;
	camera2_sensor_uctl_t *sensor_uctl = NULL;
	u32 i = 0;
	u32 num_of_frame = 1;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	frame_count = get_frame_count(itf);

	ret = get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	/* set sensitivity  */
	if (itf->otf_flag_3aa == true) {
		for (i = 0; i < num_of_frame; i++) {
			sensor_uctl = get_sensor_uctl_from_module(itf, frame_count + i);
			FIMC_BUG(!sensor_uctl);

			sensor_uctl->sensitivity = sensitivity;
		}
	}

	dbg_sensor(1, "[%s] frame_count(%d), sensitivity(%d)\n", __func__, frame_count, sensitivity);

	return ret;
}

int set_previous_dm(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 frame_count = 0;
	u32 index = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_sensor_ctl *module_ctl = NULL;
	struct camera2_sensor_ctl *sensor_ctl = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	frame_count = get_frame_count(itf);

	/* set previous values */
	index = (frame_count - 1) % CAM2P0_UCTL_LIST_SIZE;
	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	module_ctl = &sensor_peri->cis.sensor_ctls[index];
	sensor_ctl = &module_ctl->cur_cam20_sensor_ctrl;

	index = (frame_count + 1) % EXPECT_DM_NUM;
	if(sensor_ctl->sensitivity != 0 && module_ctl->valid_sensor_ctrl == true) {
		if(sensor_ctl->sensitivity != sensor_peri->cis.expecting_sensor_dm[index].sensitivity) {
			sensor_peri->cis.expecting_sensor_dm[index].sensitivity = sensor_ctl->sensitivity;
		}
	}

	if(sensor_ctl->exposureTime != 0 && module_ctl->valid_sensor_ctrl == true) {
		if(sensor_ctl->exposureTime != sensor_peri->cis.expecting_sensor_dm[index].exposureTime) {
			sensor_peri->cis.expecting_sensor_dm[index].exposureTime = sensor_ctl->exposureTime;
		}
	}

	return ret;
}

int get_sensor_flag(struct is_sensor_interface *itf,
		enum is_sensor_stat_control *stat_control_type,
		enum is_sensor_hdr_mode *hdr_mode_type,
		enum is_sensor_12bit_mode *sensor_12bit_mode_type,
		u32 *exposure_count)
{
	int ex_mode, ret = 0;
	struct is_device_sensor *sensor = NULL;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_module_enum *module = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!stat_control_type);
	FIMC_BUG(!hdr_mode_type);
	FIMC_BUG(!sensor_12bit_mode_type);
	FIMC_BUG(!exposure_count);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	}

	module = get_subdev_module_enum(itf);
	if (!module) {
		err("failed to get module enum");
		return -ENODEV;
	}

	ex_mode = is_sensor_g_ex_mode(sensor);
	if (is_sensor_check_aeb_mode(sensor)) {
		*stat_control_type = SENSOR_STAT_NOTHING;
		*exposure_count = EXPOSURE_GAIN_COUNT_2;
		*hdr_mode_type = SENSOR_HDR_MODE_2AEB_2VC;
	} else if (sensor_peri->cis.cis_data->is_data.wdr_enable) {
		if (ex_mode == EX_3DHDR || ex_mode == EX_SEAMLESS_TETRA) {
			if (!strcmp(module->sensor_maker, "SLSI"))
				*stat_control_type = SENSOR_STAT_LSI_3DHDR;
			else if (!strcmp(module->sensor_maker, "SONY"))
				*stat_control_type = SENSOR_STAT_IMX_3DHDR;

			*exposure_count = EXPOSURE_GAIN_COUNT_3;
			*hdr_mode_type = SENSOR_HDR_MODE_3HDR;
		} else {
			*stat_control_type = SENSOR_STAT_NOTHING;
			*exposure_count = EXPOSURE_GAIN_COUNT_2;
			*hdr_mode_type = SENSOR_HDR_MODE_2HDR;
		}
	} else {
		*stat_control_type = SENSOR_STAT_NOTHING;
		*exposure_count = EXPOSURE_GAIN_COUNT_1;
		*hdr_mode_type = SENSOR_HDR_MODE_SINGLE;
	}

	if (sensor->cfg[sensor->nfi_toggle]->input[CSI_VIRTUAL_CH_0].hwformat == HW_FORMAT_RAW12) {
		*sensor_12bit_mode_type = SENSOR_12BIT_MODE_PSEUDO_REAL_SEAMLESS;
	} else {
		*sensor_12bit_mode_type = SENSOR_12BIT_MODE_NONE;
	}

	dbg_sensor(1, "[%s] stat_control_type: %d, exposure_count: %d, hdr_mode_type: %d\n", __func__,
			*stat_control_type, *exposure_count, *hdr_mode_type);
	dbg_sensor(1, "[%s] sensor_12bit_mode_type: %d\n", __func__, *sensor_12bit_mode_type);

	return ret;
}

int set_sensor_stat_control_mode_change(struct is_sensor_interface *itf,
		enum is_sensor_stat_control stat_control_type,
		void *stat_control)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!stat_control);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	if (stat_control_type == SENSOR_STAT_LSI_3DHDR) {
		sensor_peri->cis.lsi_sensor_stats
			= *(struct sensor_lsi_3hdr_stat_control_mode_change *)stat_control;
		sensor_peri->cis.sensor_stats = true;
	} else if (stat_control_type == SENSOR_STAT_IMX_3DHDR) {
		sensor_peri->cis.imx_sensor_stats
			= *(struct sensor_imx_3hdr_stat_control_mode_change *)stat_control;
		sensor_peri->cis.sensor_stats = true;
	} else {
		sensor_peri->cis.sensor_stats = false;
	}

	dbg_sensor(1, "[%s] sensor stat control mode change %s\n", __func__,
		sensor_peri->cis.sensor_stats ? "Enabled" : "Disabled");

	return ret;
}

int set_sensor_roi_control(struct is_sensor_interface *itf,
		enum is_sensor_stat_control stat_control_type,
		void *roi_control)
{
	u32 frame_count = 0;
	struct is_sensor_ctl *sensor_ctl = NULL;
	u32 num_of_frame = 1;
	u32 i = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!roi_control);

	frame_count = get_frame_count(itf);
	get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_ctl = get_sensor_ctl_from_module(itf, frame_count + i);
		BUG_ON(!sensor_ctl);

		if ((stat_control_type == SENSOR_STAT_LSI_3DHDR)
			|| (stat_control_type == SENSOR_STAT_IMX_3DHDR)) {
			sensor_ctl->roi_control = *(struct roi_setting_t *)roi_control;
			sensor_ctl->update_roi = true;
		} else {
			sensor_ctl->update_roi = false;
		}

		dbg_sensor(1, "[%s][F:%d]: %d\n", __func__,
			frame_count, sensor_ctl->update_roi);
	}

	return 0;
}

int set_sensor_stat_control_per_frame(struct is_sensor_interface *itf,
		enum is_sensor_stat_control stat_control_type,
		void *stat_control)
{
	u32 frame_count = 0;
	struct is_sensor_ctl *sensor_ctl = NULL;
	u32 num_of_frame = 1;
	u32 i = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!stat_control);

	frame_count = get_frame_count(itf);
	get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_ctl = get_sensor_ctl_from_module(itf, frame_count + i);
		BUG_ON(!sensor_ctl);

		if (stat_control_type == SENSOR_STAT_LSI_3DHDR) {
			sensor_ctl->lsi_stat_control
				= *(struct sensor_lsi_3hdr_stat_control_per_frame *)stat_control;
			sensor_ctl->update_3hdr_stat = true;
		} else if (stat_control_type == SENSOR_STAT_IMX_3DHDR) {
			sensor_ctl->imx_stat_control
				= *(struct sensor_imx_3hdr_stat_control_per_frame *)stat_control;
			sensor_ctl->update_3hdr_stat = true;
		} else {
			sensor_ctl->update_3hdr_stat = false;
		}

		dbg_sensor(1, "[%s][F:%d]: %d\n", __func__,
			frame_count, sensor_ctl->update_3hdr_stat);
	}

	return 0;
}

u32 set_sensor_12bit_state(struct is_sensor_interface *itf, enum is_sensor_12bit_state state)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	dbg_sensor(1, "ERR[%s][%d][%s] state(%d) need to use SS_CTRL_CIS_REQUEST_SEAMLESS_MODE_CHANGE\n",
		__func__, sensor_peri->cis.id, sensor_peri->module->sensor_name, state);

	return 0;
}

u32 get_sensor_12bit_state(struct is_sensor_interface *itf, enum is_sensor_12bit_state *state)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	WARN_ON(!cis_data);

	*state = cis_data->cur_12bit_mode;

	dbg_sensor(1, "[%s] cur_12bit_mode(%d)\n", __func__, cis_data->cur_12bit_mode);

	return 0;
}

u32 set_remosaic_zoom_ratio(struct is_sensor_interface *itf, u32 zoom_ratio)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	dbg_sensor(1, "ERR[%s][%d][%s] ratio(%d) need to use SS_CTRL_CIS_REQUEST_SEAMLESS_MODE_CHANGE\n",
		__func__, sensor_peri->cis.id, sensor_peri->module->sensor_name, zoom_ratio);

	return 0;
}

u32 get_remosaic_zoom_ratio(struct is_sensor_interface *itf, u32 fcount, u32 *zoom_ratio)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;
	u32 vsync_count = 0;
	u32 ref_history_index = 0;
	u32 actual_zoom_ratio = 0;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	if (IS_ENABLED(CONFIG_CAMERA_CIS_ZEBU_OBJ))
		return 0;

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	WARN_ON(!cis_data);

	vsync_count = (fcount > cis_data->sen_vsync_count) ? fcount : cis_data->sen_vsync_count;
	ref_history_index = (NUM_FRAMES + vsync_count - 1) % NUM_FRAMES;

	if (cis_data->seamless_history[ref_history_index].mode & SENSOR_MODE_CROPPED_RMS) {
		actual_zoom_ratio = cis_data->seamless_history[ref_history_index].remosaic_crop_zoom_ratio;
		actual_zoom_ratio = (actual_zoom_ratio >> 2) & 0xFF; /* 2 ~ 9 bit : Zoom Value */
	}

	*zoom_ratio = actual_zoom_ratio;

	dbg_sensor(1, "[%d][%s] zoom_ratio(%d)\n", sensor_peri->cis.id, __func__, *zoom_ratio);

	return 0;
}

int set_sensor_lsc_table_init(struct is_sensor_interface *itf,
		enum is_sensor_stat_control stat_control_type,
		void *lsc_table)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!lsc_table);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	if (stat_control_type == SENSOR_STAT_IMX_3DHDR) {
		sensor_peri->cis.imx_lsc_table_3hdr = *(struct sensor_imx_3hdr_lsc_table_init *)lsc_table;
		sensor_peri->cis.lsc_table_status = true;
	} else {
		sensor_peri->cis.lsc_table_status = false;
	}

	dbg_sensor(1, "[%s] sensor 3hdr lsc table %s\n", __func__,
			sensor_peri->cis.lsc_table_status ? "Loaded" : "Unloaded");

	return 0;
}

int set_sensor_tone_control(struct is_sensor_interface *itf,
		enum is_sensor_stat_control stat_control_type,
		void *tone_control)
{
	u32 frame_count = 0;
	struct is_sensor_ctl *sensor_ctl = NULL;
	u32 num_of_frame = 1;
	u32 i = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!tone_control);

	frame_count = get_frame_count(itf);
	get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_ctl = get_sensor_ctl_from_module(itf, frame_count + i);
		BUG_ON(!sensor_ctl);

		if (stat_control_type == SENSOR_STAT_IMX_3DHDR) {
			sensor_ctl->imx_tone_control = *(struct sensor_imx_3hdr_tone_control *)tone_control;
			sensor_ctl->update_tone = true;
		} else {
			sensor_ctl->update_tone = false;
		}

		dbg_sensor(1, "[%s][F:%d]: %d\n", __func__,
			frame_count, sensor_ctl->update_tone);
	}

	return 0;
}

int set_sensor_ev_control(struct is_sensor_interface *itf,
		enum is_sensor_stat_control stat_control_type,
		void *ev_control)
{
	u32 frame_count = 0;
	struct is_sensor_ctl *sensor_ctl = NULL;
	u32 num_of_frame = 1;
	u32 i = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!ev_control);

	frame_count = get_frame_count(itf);
	get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_ctl = get_sensor_ctl_from_module(itf, frame_count + i);
		BUG_ON(!sensor_ctl);

		if (stat_control_type == SENSOR_STAT_IMX_3DHDR) {
			sensor_ctl->imx_ev_control = *(struct sensor_imx_3hdr_ev_control *)ev_control;
			sensor_ctl->update_ev = true;
		} else {
			sensor_ctl->update_ev = false;
		}

		dbg_sensor(1, "[%s][F:%d]: %d\n", __func__, frame_count, sensor_ctl->update_ev);
	}

	return 0;
}

int adjust_analog_gain(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *desired_analog_gain,
			u32 *actual_gain,
			is_sensor_adjust_direction adjust_direction)
{
	/* NOT IMPLEMENTED YET */
	int ret = -1;

	dbg_sensor(1, "[%s] NOT IMPLEMENTED YET\n", __func__);

	return ret;
}

int get_next_analog_gain(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *analog_gain)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!analog_gain);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, NEXT_FRAME, num_data, analog_gain);
	if (ret < 0)
		err("get_interface_param ANALOG_GAIN fail(%d)", ret);

	dbg_sensor(2, "[%s](%d:%d) cnt(%d): analog_gain(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		analog_gain[EXPOSURE_GAIN_LONG],
		analog_gain[EXPOSURE_GAIN_SHORT],
		analog_gain[EXPOSURE_GAIN_MIDDLE]);

	return ret;
}

int get_analog_gain(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *analog_gain)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!analog_gain);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, CURRENT_FRAME, num_data, analog_gain);
	if (ret < 0)
		err("get_interface_param ANALOG_GAIN fail(%d)", ret);

	dbg_sensor(2, "[%s](%d:%d) cnt(%d): analog_gain(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		analog_gain[EXPOSURE_GAIN_LONG],
		analog_gain[EXPOSURE_GAIN_SHORT],
		analog_gain[EXPOSURE_GAIN_MIDDLE]);

	return ret;
}

int get_next_digital_gain(struct is_sensor_interface *itf,
				enum is_exposure_gain_count num_data,
				u32 *digital_gain)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!digital_gain);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN, NEXT_FRAME, num_data, digital_gain);
	if (ret < 0)
		err("get_interface_param DIGITAL_GAIN fail(%d)", ret);

	dbg_sensor(2, "[%s](%d:%d) cnt(%d): digital_gain(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		digital_gain[EXPOSURE_GAIN_LONG],
		digital_gain[EXPOSURE_GAIN_SHORT],
		digital_gain[EXPOSURE_GAIN_MIDDLE]);

	return ret;
}

int get_digital_gain(struct is_sensor_interface *itf,
				enum is_exposure_gain_count num_data,
				u32 *digital_gain)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!digital_gain);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN,
			CURRENT_FRAME, num_data, digital_gain);
	if (ret < 0)
		err("get_interface_param DIGITAL_GAIN fail(%d)", ret);

	dbg_sensor(2, "[%s](%d:%d) cnt(%d): digital_gain(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		digital_gain[EXPOSURE_GAIN_LONG],
		digital_gain[EXPOSURE_GAIN_SHORT],
		digital_gain[EXPOSURE_GAIN_MIDDLE]);

	return ret;
}

bool is_actuator_available(struct is_sensor_interface *itf)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	dbg_actuator("actuator available (%d)\n",
		test_bit(IS_SENSOR_ACTUATOR_AVAILABLE, &sensor_peri->peri_state));

	return test_bit(IS_SENSOR_ACTUATOR_AVAILABLE, &sensor_peri->peri_state);
}

bool is_flash_available(struct is_sensor_interface *itf)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	dbg_flash("flash available (%d)\n",
		test_bit(IS_SENSOR_FLASH_AVAILABLE, &sensor_peri->peri_state));
	return test_bit(IS_SENSOR_FLASH_AVAILABLE, &sensor_peri->peri_state);
}

int get_sensor_type(struct is_sensor_interface *itf)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_module_enum *module = NULL;
	int ret = 0;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module is NULL");
		return ret; /* SENSOR_TYPE_RGB default type */
	}

	if (unlikely(!module->pdata)) {
		err("pdata is NULL");
		return ret; /* SENSOR_TYPE_RGB default type */
	}

	dbg_sensor(2, "[%s] sensor_type: %d", __func__, module->pdata->sensor_module_type);

	return module->pdata->sensor_module_type;
}

bool is_ois_available(struct is_sensor_interface *itf)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	return test_bit(IS_SENSOR_OIS_AVAILABLE, &sensor_peri->peri_state);
}

bool is_aperture_available(struct is_sensor_interface *itf)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	return test_bit(IS_SENSOR_APERTURE_AVAILABLE, &sensor_peri->peri_state);
}

bool is_laser_af_available(struct is_sensor_interface *itf)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG_FALSE(!itf);
	FIMC_BUG_FALSE(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	return test_bit(IS_SENSOR_LASER_AF_AVAILABLE, &sensor_peri->peri_state);
}

bool is_tof_af_available(struct is_sensor_interface *itf){
#ifdef USE_TOF_AF
	return true;
#else
	return false;
#endif
}

int get_sensor_frame_timing(struct is_sensor_interface *itf,
			u32 *vvalid_time,
			u32 *vblank_time)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri;
	cis_shared_data *cis_data;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	cis_data = sensor_peri->cis.cis_data;

	*vvalid_time = cis_data->frame_valid_us_time;
	*vblank_time = 1000000U / cis_data->max_fps - *vvalid_time;

	dbg_sensor(2, "[%s](%d:%d) vvalid_time(%uus), vblank_time(%uus)\n",
		__func__, get_vsync_count(itf), get_frame_count(itf),
		*vvalid_time, *vblank_time);

	return ret;
}

int get_sensor_cur_size(struct is_sensor_interface *itf,
				u32 *cur_x,
				u32 *cur_y,
				u32 *cur_width,
				u32 *cur_height)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!cur_width);
	FIMC_BUG(!cur_height);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*cur_x = sensor_peri->cis.cis_data->crop_x;
	*cur_y = sensor_peri->cis.cis_data->crop_y;
	*cur_width = sensor_peri->cis.cis_data->cur_width;
	*cur_height = sensor_peri->cis.cis_data->cur_height;

	dbg_sensor(2, "[%s] x:%d, y:%d, w:%d, h:%d\n", __func__, *cur_x, *cur_y, *cur_width, *cur_height);

	return ret;
}

int get_sensor_max_fps(struct is_sensor_interface *itf,
			u32 *max_fps)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!max_fps);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*max_fps = sensor_peri->cis.cis_data->max_fps;

	return ret;
}

int get_sensor_cur_fps(struct is_sensor_interface *itf,
			u32 *cur_fps)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!cur_fps);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	if (sensor_peri->cis.cis_data->cur_frame_us_time != 0) {
		*cur_fps = (u32)((1 * 1000 * 1000) / sensor_peri->cis.cis_data->cur_frame_us_time);
	} else {
		pr_err("[%s] cur_frame_us_time is ZERO\n", __func__);
		ret = -1;
	}

	return ret;
}

int get_hdr_ratio_ctl_by_again(struct is_sensor_interface *itf,
			u32 *ctrl_by_again)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!ctrl_by_again);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	*ctrl_by_again = sensor_peri->cis.hdr_ctrl_by_again;

	return ret;
}

int get_sensor_use_dgain(struct is_sensor_interface *itf,
			u32 *use_dgain)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!use_dgain);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	*use_dgain = sensor_peri->cis.use_dgain;

	return ret;
}

int set_alg_reset_flag(struct is_sensor_interface *itf,
			bool executed)
{
	int ret = 0;
	struct is_sensor_ctl *sensor_ctl = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_ctl = get_sensor_ctl_from_module(itf, get_frame_count(itf));

	if (sensor_ctl == NULL) {
		err("get_sensor_ctl_from_module fail!!\n");
		return -1;
	}

	sensor_ctl->alg_reset_flag = executed;

	return ret;
}

/*
 * TODO: need to implement getting C2, C3 stat data
 * This sensor interface returns done status of getting sensor stat
 */
int get_sensor_hdr_stat(struct is_sensor_interface *itf,
		enum itf_cis_hdr_stat_status *status)
{
	int ret = 0;

	info("%s", __func__);

	return ret;
}

/*
 * TODO: For example, 3AA thumbnail result shuld be applied to sensor in case of IMX230
 */
int set_3a_alg_res_to_sens(struct is_sensor_interface *itf,
		struct is_3a_res_to_sensor *sensor_setting)
{
	int ret = 0;

	info("%s", __func__);

	return ret;
}

int get_sensor_initial_aperture(struct is_sensor_interface *itf,
			u32 *aperture)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(!aperture);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	*aperture = sensor_peri->cis.aperture_num;

	return ret;
}

int set_initial_exposure_of_setfile(struct is_sensor_interface *itf,
				u32 expo)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	dbg_sensor(1, "[%s] init expo (%d)\n", __func__, expo);

	sensor_peri->cis.cis_data->low_expo_start = expo;

	return ret;
}

int set_num_of_frame_per_one_3aa(struct is_sensor_interface *itf,
				u32 *num_of_frame)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!num_of_frame);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	sensor_peri->cis.cis_data->num_of_frame = *num_of_frame;

	dbg_sensor(1, "[%s] num_of_frame (%d)\n", __func__, *num_of_frame);

	return 0;
}

int get_num_of_frame_per_one_3aa(struct is_sensor_interface *itf,
				u32 *num_of_frame)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!num_of_frame);

	if (IS_ENABLED(CONFIG_CAMERA_CIS_ZEBU_OBJ))
		return 0;

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*num_of_frame = sensor_peri->cis.cis_data->num_of_frame;

	return 0;
}

int reserved0(struct is_sensor_interface *itf,
				bool reserved0)
{
	return 0;
}

int reserved1(struct is_sensor_interface *itf,
				u32 *reserved1)
{
	return 0;
}

int set_cur_uctl_list(struct is_sensor_interface *itf)
{

	/* NOT USED */

	return 0;
}

int apply_sensor_setting(struct is_sensor_interface *itf)
{
	struct is_device_sensor *device = NULL;
	struct is_module_enum *module = NULL;
	struct v4l2_subdev *subdev_module = NULL;
	struct is_device_sensor_peri *sensor_peri = NULL;
	int flag = 0;
	cis_shared_data *cis_data = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	if (in_atomic()) {
		is_fpsimd_put_func();
		flag = 1;
	}

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module in is NULL");
		module = NULL;
		goto p_err;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module is not probed");
		subdev_module = NULL;
		goto p_err;
	}

	cis_data = sensor_peri->cis.cis_data;

	device = v4l2_get_subdev_hostdata(subdev_module);
	FIMC_BUG(!device);

	mutex_lock(&sensor_peri->cis.stream_off_lock);
	if (cis_data->stream_on) {
		if (cis_data->sen_vsync_count <= cis_data->skip_control_vsync_count) {
			info("%s [%d][%s] SS_DBG skip is_sensor_ctl_frame_evt\n",
				__func__, cis_data->sen_vsync_count, sensor_peri->module->sensor_name);
		} else {
			/* sensor control */
			is_sensor_ctl_frame_evt(device);
		}
	}
	mutex_unlock(&sensor_peri->cis.stream_off_lock);

p_err:
	if (flag)
		is_fpsimd_get_func();

	return 0;
}

int request_reset_expo_gain(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *expo,
			u32 *tgain,
			u32 *again,
			u32 *dgain)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	ae_setting *auto_exposure;
	enum exynos_sensor_position module_position;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);
	FIMC_BUG(!expo);
	FIMC_BUG(!tgain);
	FIMC_BUG(!again);
	FIMC_BUG(!dgain);

	itf->cis_itf_ops.get_module_position(itf, &module_position);

	if (module_position == SENSOR_POSITION_REAR_TOF) {
		info("%s : reset_expo skip for ToF", __func__);
		return ret;
	}

	dbg_sensor(1, "[%s] cnt(%d): exposure(L(%d), S(%d), M(%d))\n", __func__, num_data,
		expo[EXPOSURE_GAIN_LONG], expo[EXPOSURE_GAIN_SHORT], expo[EXPOSURE_GAIN_MIDDLE]);
	dbg_sensor(1, "[%s] cnt(%d): total_gain(L(%d), S(%d), M(%d))\n", __func__, num_data,
		tgain[EXPOSURE_GAIN_LONG], tgain[EXPOSURE_GAIN_SHORT], tgain[EXPOSURE_GAIN_MIDDLE]);
	dbg_sensor(1, "[%s] cnt(%d): analog_gain(L(%d), S(%d), M(%d))\n", __func__, num_data,
		again[EXPOSURE_GAIN_LONG], again[EXPOSURE_GAIN_SHORT], again[EXPOSURE_GAIN_MIDDLE]);
	dbg_sensor(1, "[%s] cnt(%d): digital_gain(L(%d), S(%d), M(%d))\n", __func__, num_data,
		dgain[EXPOSURE_GAIN_LONG], dgain[EXPOSURE_GAIN_SHORT], dgain[EXPOSURE_GAIN_MIDDLE]);

	end_index = itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA;

	for (i = 0; i <= end_index; i++) {
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_TOTAL_GAIN, i, num_data, tgain);
		if (ret < 0)
			err("set_interface_param TOTAL_GAIN fail(%d)", ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN, i, num_data, again);
		if (ret < 0)
			err("set_interface_param ANALOG_GAIN fail(%d)", ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN, i, num_data, dgain);
		if (ret < 0)
			err("set_interface_param DIGITAL_GAIN fail(%d)", ret);
		ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, i, num_data, expo);
		if (ret < 0)
			err("set_interface_param EXPOSURE fail(%d)", ret);
	}

	itf->vsync_flag = true;

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	if(sensor_peri->use_sensor_work == false) {
		is_sensor_set_cis_uctrl_list(sensor_peri, num_data, expo, tgain, again, dgain);
	}

	auto_exposure = sensor_peri->cis.cis_data->auto_exposure;
	memset(auto_exposure, 0, sizeof(sensor_peri->cis.cis_data->auto_exposure));

	switch (num_data) {
	case EXPOSURE_GAIN_COUNT_1:
		auto_exposure[CURRENT_FRAME].exposure = expo[EXPOSURE_GAIN_LONG];
		auto_exposure[CURRENT_FRAME].analog_gain = again[EXPOSURE_GAIN_LONG];
		auto_exposure[CURRENT_FRAME].digital_gain = dgain[EXPOSURE_GAIN_LONG];
		break;
	case EXPOSURE_GAIN_COUNT_2:
		auto_exposure[CURRENT_FRAME].long_exposure = expo[EXPOSURE_GAIN_LONG];
		auto_exposure[CURRENT_FRAME].long_analog_gain = again[EXPOSURE_GAIN_LONG];
		auto_exposure[CURRENT_FRAME].long_digital_gain = dgain[EXPOSURE_GAIN_LONG];
		auto_exposure[CURRENT_FRAME].short_exposure = expo[EXPOSURE_GAIN_SHORT];
		auto_exposure[CURRENT_FRAME].short_analog_gain = again[EXPOSURE_GAIN_SHORT];
		auto_exposure[CURRENT_FRAME].short_digital_gain = dgain[EXPOSURE_GAIN_SHORT];
		break;
	case EXPOSURE_GAIN_COUNT_3:
		auto_exposure[CURRENT_FRAME].long_exposure = expo[EXPOSURE_GAIN_LONG];
		auto_exposure[CURRENT_FRAME].long_analog_gain = again[EXPOSURE_GAIN_LONG];
		auto_exposure[CURRENT_FRAME].long_digital_gain = dgain[EXPOSURE_GAIN_LONG];
		auto_exposure[CURRENT_FRAME].short_exposure = expo[EXPOSURE_GAIN_SHORT];
		auto_exposure[CURRENT_FRAME].short_analog_gain = again[EXPOSURE_GAIN_SHORT];
		auto_exposure[CURRENT_FRAME].short_digital_gain = dgain[EXPOSURE_GAIN_SHORT];
		auto_exposure[CURRENT_FRAME].middle_exposure = expo[EXPOSURE_GAIN_MIDDLE];
		auto_exposure[CURRENT_FRAME].middle_analog_gain = again[EXPOSURE_GAIN_MIDDLE];
		auto_exposure[CURRENT_FRAME].middle_digital_gain = dgain[EXPOSURE_GAIN_MIDDLE];
		break;
	default:
		err("invalid exp_gain_count(%d)", num_data);
		break;
	}

	memcpy(&auto_exposure[NEXT_FRAME], &auto_exposure[CURRENT_FRAME], sizeof(auto_exposure[NEXT_FRAME]));

	return ret;
}

int set_sensor_info_mode_change(struct is_sensor_interface *itf,
		enum is_exposure_gain_count num_data,
		u32 *expo,
		u32 *again,
		u32 *dgain)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	ae_setting *mode_chg;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	mode_chg = &sensor_peri->cis.mode_chg;

	memset(mode_chg, 0, sizeof(ae_setting));

	sensor_peri->cis.exp_gain_cnt = num_data;

	switch (num_data) {
	case EXPOSURE_GAIN_COUNT_1:
		mode_chg->exposure = expo[EXPOSURE_GAIN_LONG];
		mode_chg->analog_gain = again[EXPOSURE_GAIN_LONG];
		mode_chg->digital_gain = dgain[EXPOSURE_GAIN_LONG];
		break;
	case EXPOSURE_GAIN_COUNT_2:
		mode_chg->long_exposure = expo[EXPOSURE_GAIN_LONG];
		mode_chg->long_analog_gain = again[EXPOSURE_GAIN_LONG];
		mode_chg->long_digital_gain = dgain[EXPOSURE_GAIN_LONG];
		mode_chg->short_exposure = expo[EXPOSURE_GAIN_SHORT];
		mode_chg->short_analog_gain = again[EXPOSURE_GAIN_SHORT];
		mode_chg->short_digital_gain = dgain[EXPOSURE_GAIN_SHORT];
		break;
	case EXPOSURE_GAIN_COUNT_3:
		mode_chg->long_exposure = expo[EXPOSURE_GAIN_LONG];
		mode_chg->long_analog_gain = again[EXPOSURE_GAIN_LONG];
		mode_chg->long_digital_gain = dgain[EXPOSURE_GAIN_LONG];
		mode_chg->short_exposure = expo[EXPOSURE_GAIN_SHORT];
		mode_chg->short_analog_gain = again[EXPOSURE_GAIN_SHORT];
		mode_chg->short_digital_gain = dgain[EXPOSURE_GAIN_SHORT];
		mode_chg->middle_exposure = expo[EXPOSURE_GAIN_MIDDLE];
		mode_chg->middle_analog_gain = again[EXPOSURE_GAIN_MIDDLE];
		mode_chg->middle_digital_gain = dgain[EXPOSURE_GAIN_MIDDLE];
		break;
	default:
		err("invalid exp_gain_count(%d)", num_data);
		break;
	}

	dbg_sensor(1, "[%s] cnt(%d): exposure(L(%d), S(%d), M(%d))\n", __func__, num_data,
		expo[EXPOSURE_GAIN_LONG], expo[EXPOSURE_GAIN_SHORT], expo[EXPOSURE_GAIN_MIDDLE]);
	dbg_sensor(1, "[%s] cnt(%d): analog_gain(L(%d), S(%d), M(%d))\n", __func__, num_data,
		again[EXPOSURE_GAIN_LONG], again[EXPOSURE_GAIN_SHORT], again[EXPOSURE_GAIN_MIDDLE]);
	dbg_sensor(1, "[%s] cnt(%d): digital_gain(L(%d), S(%d), M(%d))\n", __func__, num_data,
		dgain[EXPOSURE_GAIN_LONG], dgain[EXPOSURE_GAIN_SHORT], dgain[EXPOSURE_GAIN_MIDDLE]);

	return ret;
}

int update_sensor_dynamic_meta(struct is_sensor_interface *itf,
		u32 frame_count,
		camera2_ctl_t *ctrl,
		camera2_dm_t *dm,
		camera2_udm_t *udm)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	u32 index = 0;
	enum camera_op_mode op_mode;

	FIMC_BUG(!itf);
	FIMC_BUG(!ctrl);
	FIMC_BUG(!dm);
	FIMC_BUG(!udm);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	index = frame_count % EXPECT_DM_NUM;
	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	op_mode = sensor_peri->cis.cis_data->is_data.op_mode;

	dm->aa.vendor_seamlessUpdateFrameNumber = sensor_peri->cis.cis_data->remosaic_crop_update_vsync_cnt;
	dbg_sensor(1, "[%s][%d][%d][%s] vendor_seamlessUpdateFrameNumber(%d)\n",
		__func__, sensor_peri->cis.id, frame_count, sensor_peri->module->sensor_name,
		dm->aa.vendor_seamlessUpdateFrameNumber);

	if (op_mode == CAMERA_OP_MODE_HAL3_TW
		|| op_mode == CAMERA_OP_MODE_HAL3_ATTACH
		|| op_mode == CAMERA_OP_MODE_HAL3_CAMERAX
		|| op_mode == CAMERA_OP_MODE_HAL3_UNIHAL_VIP) {
		dm->sensor.exposureTime = sensor_peri->cis.expecting_sensor_dm[index].exposureTime;
		dm->sensor.sensitivity = sensor_peri->cis.expecting_sensor_dm[index].sensitivity;
	} else {
		if (ctrl->aa.aeMode == AA_AEMODE_OFF
			|| ctrl->aa.mode == AA_CONTROL_OFF
			|| sensor_peri->cis.cur_sensor_uctrl.exposureTime == 0) {
			dm->sensor.exposureTime = sensor_peri->cis.expecting_sensor_dm[index].exposureTime;
			dm->sensor.sensitivity = sensor_peri->cis.expecting_sensor_dm[index].sensitivity;
		} else {
			dm->sensor.exposureTime = sensor_peri->cis.cur_sensor_uctrl.exposureTime;
			dm->sensor.sensitivity = sensor_peri->cis.cur_sensor_uctrl.sensitivity;
		}
	}

	dm->sensor.frameDuration = sensor_peri->cis.cis_data->cur_frame_us_time * 1000;
	dm->sensor.rollingShutterSkew = sensor_peri->cis.cis_data->rolling_shutter_skew;

	udm->sensor.analogGain = sensor_peri->cis.expecting_sensor_udm[index].analogGain;
	udm->sensor.digitalGain = sensor_peri->cis.expecting_sensor_udm[index].digitalGain;
	udm->sensor.longExposureTime = sensor_peri->cis.expecting_sensor_udm[index].longExposureTime;
	udm->sensor.shortExposureTime = sensor_peri->cis.expecting_sensor_udm[index].shortExposureTime;
	udm->sensor.midExposureTime = sensor_peri->cis.expecting_sensor_udm[index].midExposureTime;
	udm->sensor.longAnalogGain = sensor_peri->cis.expecting_sensor_udm[index].longAnalogGain;
	udm->sensor.shortAnalogGain = sensor_peri->cis.expecting_sensor_udm[index].shortAnalogGain;
	udm->sensor.midAnalogGain = sensor_peri->cis.expecting_sensor_udm[index].midAnalogGain;
	udm->sensor.longDigitalGain = sensor_peri->cis.expecting_sensor_udm[index].longDigitalGain;
	udm->sensor.shortDigitalGain = sensor_peri->cis.expecting_sensor_udm[index].shortDigitalGain;
	udm->sensor.midDigitalGain = sensor_peri->cis.expecting_sensor_udm[index].midDigitalGain;

	dbg_sensor(1, "[%s][%d][%d][%s] op_mode(%d), expo(%lld), duration(%lld), sensitivity(%d), rollingShutterSkew(%lld)\n",
		__func__, sensor_peri->cis.id, frame_count, sensor_peri->module->sensor_name,
		op_mode,
		dm->sensor.exposureTime,
		dm->sensor.frameDuration,
		dm->sensor.sensitivity,
		dm->sensor.rollingShutterSkew);
	dbg_sensor(1, "[%s][%d][%d][%s] udm_expo(L(%lld), S(%lld), M(%lld))\n",
		__func__, sensor_peri->cis.id, frame_count, sensor_peri->module->sensor_name,
		udm->sensor.longExposureTime,
		udm->sensor.shortExposureTime,
		udm->sensor.midExposureTime);
	dbg_sensor(1, "[%s][%d][%d][%s] udm_dgain(L(%d), S(%d), M(%d))\n",
		__func__, sensor_peri->cis.id, frame_count, sensor_peri->module->sensor_name,
		udm->sensor.longDigitalGain,
		udm->sensor.shortDigitalGain,
		udm->sensor.midDigitalGain);
	dbg_sensor(1, "[%s][%d][%d][%s] udm_again(L(%d), S(%d), M(%d))\n",
		__func__, sensor_peri->cis.id, frame_count, sensor_peri->module->sensor_name,
		udm->sensor.longAnalogGain,
		udm->sensor.shortAnalogGain,
		udm->sensor.midAnalogGain);

	return ret;
}

int copy_sensor_ctl(struct is_sensor_interface *itf,
			u32 frame_count,
			camera2_shot_t *shot)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	u32 index = 0;
	struct is_sensor_ctl *sensor_ctl;
	cis_shared_data *cis_data = NULL;
	struct is_flash *flash = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	index = frame_count % EXPECT_DM_NUM;
	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	sensor_ctl = &sensor_peri->cis.sensor_ctls[index];
	cis_data = sensor_peri->cis.cis_data;

	if (sensor_peri->flash)
		flash = sensor_peri->flash;

	FIMC_BUG(!cis_data);

	sensor_ctl->ctl_frame_number = 0;
	sensor_ctl->valid_sensor_ctrl = false;
	sensor_ctl->is_sensor_request = false;

	if (shot != NULL) {
		cis_data->is_data.paf_mode = shot->uctl.isModeUd.paf_mode;
		cis_data->is_data.wdr_mode = shot->uctl.isModeUd.wdr_mode;
		cis_data->is_data.disparity_mode = shot->uctl.isModeUd.disparity_mode;

		sensor_ctl->ctl_frame_number = shot->dm.request.frameCount;
		sensor_ctl->cur_cam20_sensor_ctrl = shot->ctl.sensor;
#if defined(CONFIG_CAMERA_USE_MCU) || defined(CONFIG_CAMERA_USE_INTERNAL_MCU)
		if (sensor_peri->mcu && sensor_peri->mcu->ois) {
			sensor_peri->mcu->ois->ois_mode = shot->ctl.lens.opticalStabilizationMode;
			sensor_peri->mcu->ois->coef = (u8)shot->uctl.lensUd.oisCoefVal;
		}
#endif
#ifdef CONFIG_CAMERA_VENDOR_MCD
		if (sensor_peri->laser_af) {
			sensor_peri->laser_af->rs_mode = shot->uctl.isModeUd.range_sensor_mode;
		}

		cis_data->is_data.scene_mode = shot->ctl.aa.sceneMode;
#endif
		cis_data->video_mode = shot->ctl.aa.vendor_videoMode;
		cis_data->is_data.flip_mode = shot->uctl.sensorFlip;
		cis_data->is_data.op_mode = shot->uctl.opMode;

#ifdef USE_CAMERA_ITF_CIS_SMIA_ONLY
		itf->cis_mode = ITF_CIS_SMIA;
		if (cis_data->is_data.wdr_enable) {
			if (shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_ON ||
					shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_AUTO ||
					shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_AUTO_LIKE)
				itf->cis_mode = ITF_CIS_SMIA_WDR;
		}
#else
		if (shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_ON ||
				shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_AUTO ||
				shot->uctl.isModeUd.wdr_mode == CAMERA_WDR_AUTO_LIKE)
			itf->cis_mode = ITF_CIS_SMIA_WDR;
		else
			itf->cis_mode = ITF_CIS_SMIA;
#endif

		/* set frame rate : Limit of max frame duration
		 * Frame duration is set by
		 * 1. Manual sensor control
		 *	 - For HAL3.2
		 *	 - AE_MODE is OFF and ctl.sensor.frameDuration is not 0.
		 * 2. Target FPS Range
		 *	 - For backward compatibility
		 *	 - In case of AE_MODE is not OFF and aeTargetFpsRange[0] is not 0,
		 *	   frame durtaion is 1000000us / aeTargetFpsRage[0]
		 */
		if ((shot->ctl.aa.aeMode == AA_AEMODE_OFF)
			|| (shot->ctl.aa.mode == AA_CONTROL_OFF)) {
			sensor_ctl->valid_sensor_ctrl = true;
			sensor_ctl->is_sensor_request = true;
		} else if (shot->ctl.aa.aeTargetFpsRange[1] != 0) {
			u32 duration_us = 1000000 / shot->ctl.aa.aeTargetFpsRange[1];
			sensor_ctl->cur_cam20_sensor_udctrl.frameDuration = is_sensor_convert_us_to_ns(duration_us);

			/* qbuf min, max fps value */
			sensor_peri->cis.min_fps = shot->ctl.aa.aeTargetFpsRange[0];
			sensor_peri->cis.max_fps = shot->ctl.aa.aeTargetFpsRange[1];
		}

		if (flash && flash->flash_data.aeflashMode != shot->ctl.aa.vendor_aeflashMode) {
			flash->flash_data.aeflashMode = shot->ctl.aa.vendor_aeflashMode;
			flash->flash_data.firingPower = shot->ctl.flash.firingPower;

			dbg_flash("[%s] aeflashMode(%d), firingPower(%d)\n",  __func__,
				flash->flash_data.aeflashMode, flash->flash_data.firingPower);
			if (flash->use_pre_config_work)
				schedule_work(&sensor_peri->flash->flash_data.flash_set_pre_config_work);
		}
	} else {
		sensor_ctl->cur_cam20_sensor_ctrl.testPatternMode =  0;
		dbg_sensor(1, "[%s] internal shot\n", __func__);
	}

	return ret;
}

int get_module_id(struct is_sensor_interface *itf, u32 *module_id)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_module_enum *module = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!module_id);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module in is NULL");
		module = NULL;
		goto p_err;
	}

	*module_id = module->sensor_id;

p_err:
	return 0;

}

int get_module_position(struct is_sensor_interface *itf,
				enum exynos_sensor_position *module_position)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_module_enum *module = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!module_position);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	module = sensor_peri->module;
	if (unlikely(!module)) {
		err("module in is NULL");
		module = NULL;
		goto p_err;
	}

	*module_position = module->position;

p_err:
	return 0;

}

int set_sensor_3a_mode(struct is_sensor_interface *itf,
				u32 mode)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	if (mode > 1) {
		err("invalid mode(%d)\n", mode);
		return -1;
	}

	/* 0: OTF, 1: M2M */
	itf->otf_flag_3aa = mode == 0 ? true : false;

	if (itf->otf_flag_3aa == false) {
		ret = is_sensor_init_sensor_thread(sensor_peri);
		if (ret) {
			err("is_sensor_init_sensor_thread is fail(%d)", ret);
			return ret;
		}
	}

	return 0;
}

int get_initial_exposure_gain_of_sensor(struct is_sensor_interface *itf,
	enum is_exposure_gain_count num_data,
	u32 *expo,
	u32 *again,
	u32 *dgain)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	int i;

	if (!itf) {
		err("is_sensor_interface is NULL");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	if (sensor_peri->cis.use_initial_ae) {
		switch (num_data) {
		case EXPOSURE_GAIN_COUNT_1:
			expo[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.exposure;
			again[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.analog_gain;
			dgain[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.digital_gain;
			break;
		case EXPOSURE_GAIN_COUNT_2:
			expo[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.long_exposure;
			again[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.long_analog_gain;
			dgain[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.long_digital_gain;
			expo[EXPOSURE_GAIN_SHORT] = sensor_peri->cis.init_ae_setting.short_exposure;
			again[EXPOSURE_GAIN_SHORT] = sensor_peri->cis.init_ae_setting.short_analog_gain;
			dgain[EXPOSURE_GAIN_SHORT] = sensor_peri->cis.init_ae_setting.short_digital_gain;
			expo[EXPOSURE_GAIN_MIDDLE] = sensor_peri->cis.init_ae_setting.middle_exposure;
			again[EXPOSURE_GAIN_MIDDLE] = sensor_peri->cis.init_ae_setting.middle_analog_gain;
			dgain[EXPOSURE_GAIN_MIDDLE] = sensor_peri->cis.init_ae_setting.middle_digital_gain;
			break;
		case EXPOSURE_GAIN_COUNT_3:
			expo[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.long_exposure;
			again[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.long_analog_gain;
			dgain[EXPOSURE_GAIN_LONG] = sensor_peri->cis.init_ae_setting.long_digital_gain;
			expo[EXPOSURE_GAIN_SHORT] = sensor_peri->cis.init_ae_setting.short_exposure;
			again[EXPOSURE_GAIN_SHORT] = sensor_peri->cis.init_ae_setting.short_analog_gain;
			dgain[EXPOSURE_GAIN_SHORT] = sensor_peri->cis.init_ae_setting.short_digital_gain;
			break;
		default:
			err("wrong exp_gain_count(%d)", num_data);
			break;
		}
	} else {
		for (i = 0; i < num_data; i++)
			expo[i] = again[i] = dgain[i] = 0;
		dbg_sensor(1, "%s: called at not enabled last_ae, set to 0", __func__);
	}

	dbg_sensor(1, "%s: [%s] long(%d-%d-%d), shot(%d-%d-%d), middle(%d-%d-%d)\n", __func__,
		sensor_peri->module->sensor_name,
		expo[EXPOSURE_GAIN_LONG], again[EXPOSURE_GAIN_LONG], dgain[EXPOSURE_GAIN_LONG],
		(num_data >= 2) ? expo[EXPOSURE_GAIN_SHORT] : 0,
		(num_data >= 2) ? again[EXPOSURE_GAIN_SHORT] : 0,
		(num_data >= 2) ? dgain[EXPOSURE_GAIN_SHORT] : 0,
		(num_data == 3) ? expo[EXPOSURE_GAIN_MIDDLE] : 0,
		(num_data == 3) ? again[EXPOSURE_GAIN_MIDDLE] : 0,
		(num_data == 3) ? dgain[EXPOSURE_GAIN_MIDDLE] : 0);

	return 0;
}

u32 get_sensor_frameid(struct is_sensor_interface *itf, u32 *frameid)
{
	u32 ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;

	if (unlikely(!itf) || (itf->magic != SENSOR_INTERFACE_MAGIC))
		goto p_err;

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	cis_data = sensor_peri->cis.cis_data;
	if (unlikely(!cis_data))
		goto p_err;

	dbg_sensor(1, "[%s] sen_frame_id(0x%x)\n", __func__, cis_data->sen_frame_id);
	*frameid = cis_data->sen_frame_id;

	return ret;
p_err:
	err("invalid sensor interface");
	return 0;
}

u32 set_adjust_sync(struct is_sensor_interface *itf, u32 setsync)
{
	u32 ret = 0;
	struct is_device_sensor *device = NULL;
	struct is_module_enum *module = NULL;
	struct v4l2_subdev *subdev_module = NULL;
	struct is_device_sensor_peri *sensor_peri = NULL;

	if (unlikely(!itf) || (itf->magic != SENSOR_INTERFACE_MAGIC)) {
		err("invalid sensor interface");
		return 0;
	}

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	module = sensor_peri->module;
	subdev_module = module->subdev;
	device = v4l2_get_subdev_hostdata(subdev_module);

	/* sensor control */
	is_sensor_ctl_adjust_sync(device, setsync);

	return ret;
}

u32 request_frame_length_line(struct is_sensor_interface *itf, u32 framelengthline)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;
	u32 frameDuration = 0;

	if (unlikely(!itf))
		goto p_err;

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	cis_data = sensor_peri->cis.cis_data;
	if (unlikely(!cis_data))
		goto p_err;

	frameDuration = is_sensor_convert_ns_to_us(sensor_peri->cis.cur_sensor_uctrl.frameDuration);
	cis_data->min_frame_us_time = MAX(frameDuration, cis_data->base_min_frame_us_time + framelengthline);

	dbg_sensor(1, "[%s] frameDuration(%d), framelengthline(%d), min_frame_us_time(%d)\n",
		__func__, frameDuration, framelengthline, cis_data->min_frame_us_time);

	return 0;

p_err:
	err("invalid sensor interface");
	return 0;
}

/* In order to change a current CIS mode when an user select the WDR (long and short exposure) mode or the normal AE mo */
int change_cis_mode(struct is_sensor_interface *itf,
		enum itf_cis_interface cis_mode)
{
	int ret = 0;

	return ret;
}

int start_of_frame(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "%s: !!!!!!!!!!!!!!!!!!!!!!!\n", __func__);
	end_index = itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA;

	for (i = 0; i < end_index; i++) {
		if (itf->cis_mode == ITF_CIS_SMIA) {
			itf->total_gain[EXPOSURE_GAIN_LONG][i] = itf->total_gain[EXPOSURE_GAIN_LONG][i + 1];
			itf->analog_gain[EXPOSURE_GAIN_LONG][i] = itf->analog_gain[EXPOSURE_GAIN_LONG][i + 1];
			itf->digital_gain[EXPOSURE_GAIN_LONG][i] = itf->digital_gain[EXPOSURE_GAIN_LONG][i + 1];
			itf->exposure[EXPOSURE_GAIN_LONG][i] = itf->exposure[EXPOSURE_GAIN_LONG][i + 1];
		} else if (itf->cis_mode == ITF_CIS_SMIA_WDR){
			itf->total_gain[EXPOSURE_GAIN_LONG][i] = itf->total_gain[EXPOSURE_GAIN_LONG][i + 1];
			itf->total_gain[EXPOSURE_GAIN_SHORT][i] = itf->total_gain[EXPOSURE_GAIN_SHORT][i + 1];
			itf->total_gain[EXPOSURE_GAIN_MIDDLE][i] = itf->total_gain[EXPOSURE_GAIN_MIDDLE][i + 1];
			itf->analog_gain[EXPOSURE_GAIN_LONG][i] = itf->analog_gain[EXPOSURE_GAIN_LONG][i + 1];
			itf->analog_gain[EXPOSURE_GAIN_SHORT][i] = itf->analog_gain[EXPOSURE_GAIN_SHORT][i + 1];
			itf->analog_gain[EXPOSURE_GAIN_MIDDLE][i] = itf->analog_gain[EXPOSURE_GAIN_MIDDLE][i + 1];
			itf->digital_gain[EXPOSURE_GAIN_LONG][i] = itf->digital_gain[EXPOSURE_GAIN_LONG][i + 1];
			itf->digital_gain[EXPOSURE_GAIN_SHORT][i] = itf->digital_gain[EXPOSURE_GAIN_SHORT][i + 1];
			itf->digital_gain[EXPOSURE_GAIN_MIDDLE][i] = itf->digital_gain[EXPOSURE_GAIN_MIDDLE][i + 1];
			itf->exposure[EXPOSURE_GAIN_LONG][i] = itf->exposure[EXPOSURE_GAIN_LONG][i + 1];
			itf->exposure[EXPOSURE_GAIN_SHORT][i] = itf->exposure[EXPOSURE_GAIN_SHORT][i + 1];
			itf->exposure[EXPOSURE_GAIN_MIDDLE][i] = itf->exposure[EXPOSURE_GAIN_MIDDLE][i + 1];
		} else {
			pr_err("[%s] in valid cis_mode (%d)\n", __func__, itf->cis_mode);
			ret = -EINVAL;
			goto p_err;
		}

		itf->flash_intensity[i] = itf->flash_intensity[i + 1];
		itf->flash_mode[i] = itf->flash_mode[i + 1];
		itf->flash_firing_duration[i] = itf->flash_firing_duration[i + 1];
	}

	itf->flash_mode[i] = CAM2_FLASH_MODE_OFF;
	itf->flash_intensity[end_index] = 0;
	itf->flash_firing_duration[i] = 0;

	/* Flash setting */
	ret =  set_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_FLASH_INTENSITY,
			end_index, 1, &itf->flash_intensity[end_index]);
	if (ret < 0)
		pr_err("[%s] set_interface_param FLASH_INTENSITY fail(%d)\n", __func__, ret);
	/* TODO */
	/*
	if (itf->flash_itf_ops) {
		(*itf->flash_itf_ops)->on_start_of_frame(itf->flash_itf_ops);
		(*itf->flash_itf_ops)->set_next_flash(itf->flash_itf_ops, itf->flash_intensity[NEXT_FRAME]);
	}
	*/

p_err:
	return ret;
}

int end_of_frame(struct is_sensor_interface *itf)
{
	int ret = 0;
	u32 end_index = 0;
	u32 total_gain[EXPOSURE_GAIN_MAX];
	u32 analog_gain[EXPOSURE_GAIN_MAX];
	u32 digital_gain[EXPOSURE_GAIN_MAX];
	u32 exposure[EXPOSURE_GAIN_MAX];
	enum is_exposure_gain_count num_data;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	dbg_sensor(1, "%s: !!!!!!!!!!!!!!!!!!!!!!!\n", __func__);
	end_index = itf->otf_flag_3aa == true ? NEXT_NEXT_FRAME_OTF : NEXT_NEXT_FRAME_DMA;

	if (itf->vsync_flag == true) {
		/* TODO: sensor timing test */

		if (itf->otf_flag_3aa == false) {
			sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
			FIMC_BUG(!sensor_peri);

			num_data = sensor_peri->cis.exp_gain_cnt;

			/* set gain */
			ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_TOTAL_GAIN,
						end_index, num_data, total_gain);
			if (ret < 0)
				pr_err("[%s] get TOTAL_GAIN fail(%d)\n", __func__, ret);
			ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_ANALOG_GAIN,
						end_index, num_data, analog_gain);
			if (ret < 0)
				pr_err("[%s] get ANALOG_GAIN fail(%d)\n", __func__, ret);
			ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_DIGITAL_GAIN,
						end_index, num_data, digital_gain);
			if (ret < 0)
				pr_err("[%s] get DIGITAL_GAIN fail(%d)\n", __func__, ret);

			ret = set_gain_permile(itf, itf->cis_mode, num_data,
						total_gain, analog_gain, digital_gain);
			if (ret < 0) {
				pr_err("[%s] set_gain_permile fail(%d)\n", __func__, ret);
				goto p_err;
			}

			/* set exposure */
			ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE,
						end_index, num_data, exposure);
			if (ret < 0)
				pr_err("[%s] get EXPOSURE fail(%d)\n", __func__, ret);

			ret = set_exposure(itf, itf->cis_mode, num_data, exposure);
			if (ret < 0) {
				pr_err("[%s] set_exposure fail(%d)\n", __func__, ret);
				goto p_err;
			}
		}
	}

	/* TODO */
	/*
	if (itf->flash_itf_ops) {
		(*itf->flash_itf_ops)->on_end_of_frame(itf->flash_itf_ops);
	}
	*/

p_err:
	return ret;
}

int apply_frame_settings(struct is_sensor_interface *itf)
{
	/* NOT IMPLEMENTED YET */
	int ret = -1;

	err("NOT IMPLEMENTED YET\n");

	return ret;
}

/* end of new APIs */

/* APERTURE interface */
int set_aperture_value(struct is_sensor_interface *itf, int value)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	dbg_aperture("[%s] aperture value(%d)\n", __func__, value);

	if (sensor_peri->mcu && sensor_peri->mcu->aperture) {
		sensor_peri->mcu->aperture->start_value = value;

		if (value != sensor_peri->mcu->aperture->cur_value) {
			sensor_peri->mcu->aperture->new_value = value;
			sensor_peri->mcu->aperture->step = APERTURE_STEP_PREPARE;
		}
	}

	return ret;
}

int get_aperture_value(struct is_sensor_interface *itf, struct is_apature_info_t *param)
{
	int ret = 0;
	int aperture_value = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	if (sensor_peri->mcu && sensor_peri->mcu->aperture)
		aperture_value = sensor_peri->mcu->aperture->cur_value;
	else
		aperture_value = 0;

	param->cur_value = aperture_value;

	dbg_aperture("[%s] aperture value(%d)\n", __func__, aperture_value);

	return ret;
}

/* Flash interface */
int set_flash(struct is_sensor_interface *itf,
		u32 frame_count, u32 flash_mode, u32 intensity, u32 time)
{
	int ret = 0;
	struct is_sensor_ctl *sensor_ctl = NULL;
	camera2_flash_uctl_t *flash_uctl = NULL;
	enum flash_mode mode = CAM2_FLASH_MODE_OFF;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_ctl = get_sensor_ctl_from_module(itf, frame_count);
	flash_uctl = &sensor_ctl->cur_cam20_flash_udctrl;

	sensor_ctl->flash_frame_number = frame_count;

	/*
	 * If use dual flash control, intensity value is ratio between warm and cool LED
	 * else if, use single flash, intensity value zero means flash off
	 */
	if (!IS_ENABLED(FLASH_CAL_DATA_ENABLE) && intensity == 0) {
		mode = CAM2_FLASH_MODE_OFF;
	} else {
		switch (flash_mode) {
		case CAM2_FLASH_MODE_OFF:
		case CAM2_FLASH_MODE_SINGLE:
		case CAM2_FLASH_MODE_TORCH:
			mode = flash_mode;
			break;
		default:
			err("unknown scene_mode(%d)", flash_mode);
			break;
		}
	}

	flash_uctl->flashMode = mode;
	flash_uctl->firingPower = intensity;
	flash_uctl->firingTime = time;

	dbg_flash("[%s] frame count %d,  mode %d, intensity %d, firing time %lld\n", __func__,
			frame_count,
			flash_uctl->flashMode,
			flash_uctl->firingPower,
			flash_uctl->firingTime);

	sensor_ctl->valid_flash_udctrl = true;

	return ret;
}

int request_flash(struct is_sensor_interface *itf,
				u32 mode,
				bool on,
				u32 intensity,
				u32 time)
{
	int ret = 0;
	u32 i = 0;
	u32 end_index = 0;
	u32 vsync_cnt = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_flash *flash;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	if (!sensor_peri->flash) {
		err("flash is NULL");
		return -ENXIO;
	}

	flash = sensor_peri->flash;

	vsync_cnt = get_vsync_count(itf);

	if (sensor_peri->cis.cis_data->video_mode) {
		if (mode == CAM2_FLASH_MODE_SINGLE)
			err("[%s](%d) mode(%d), on(%d), intensity(%d), time(%d), video_mode(%d)",
				sensor_peri->module->sensor_name, vsync_cnt,
				mode, on, intensity, time, sensor_peri->cis.cis_data->video_mode);
	}

	dbg_flash("[%s][%s](%d) mode(%d), on(%d), intensity(%d), time(%d)\n",
			__func__, sensor_peri->module->sensor_name, vsync_cnt, mode, on, intensity, time);

	ret = get_num_of_frame_per_one_3aa(itf, &end_index);
	if (ret < 0) {
		pr_err("[%s] get_num_of_frame_per_one_3aa fail(%d)\n", __func__, ret);
		goto p_err;
	}

	for (i = 0; i < end_index; i++) {
		if (mode == CAM2_FLASH_MODE_TORCH && on == false) {
			dbg_flash("[%s](%d) pre-flash off, mode(%d), on(%d), intensity(%d), time(%d)\n",
				__func__, vsync_cnt, mode, on, intensity, time);
			/* pre-flash off */
			flash->flash_ae.pre_fls_ae_reset = true;
			flash->flash_ae.frm_num_pre_fls = vsync_cnt + 1;
		} else if (mode == CAM2_FLASH_MODE_SINGLE && on == true) {
			dbg_flash("[%s](%d) main on-off, mode(%d), on(%d), intensity(%d), time(%d)\n",
				__func__, vsync_cnt, mode, on, intensity, time);

			flash->flash_data.mode = mode;
			flash->flash_data.intensity = intensity;
			flash->flash_data.firing_time_us = time;
			/* main-flash on off*/
			flash->flash_ae.main_fls_ae_reset = true;
			flash->flash_ae.frm_num_main_fls[0] = vsync_cnt + 1;
			flash->flash_ae.frm_num_main_fls[1] =
				(vsync_cnt + 1) + flash->flash_ae.flash_capture_cnt;
		} else {
			/* pre-flash on & flash off */
			ret = set_flash(itf, vsync_cnt + i, mode, intensity, time);
			if (ret < 0) {
				pr_err("[%s] set_flash fail(%d)\n", __func__, ret);
				goto p_err;
			}
		}
	}

p_err:
	return ret;
}

int request_direct_flash(struct is_sensor_interface *itf,
				u32 mode,
				bool on,
				u32 intensity,
				u32 time)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_flash *flash;
	u32 vsync_cnt = 0;
	int flag = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	if (!sensor_peri->flash) {
		err("flash is NULL");
		return -ENXIO;
	}

	flash = sensor_peri->flash;

	if (in_atomic()) {
		is_fpsimd_put_func();
		flag = 1;
	}

	vsync_cnt = get_vsync_count(itf);

	dbg_flash("[%s] mode(%d), on(%d), intensity(%d), time(%d)\n", __func__, mode, on, intensity, time);

	if (mode == CAM2_FLASH_MODE_SINGLE && on == true)
	{
		flash->flash_data.mode = mode;
		flash->flash_data.intensity = intensity;
		flash->flash_data.firing_time_us = time;
		flash->flash_data.high_resolution_flash = true;
#ifndef USE_HIGH_RES_FLASH_FIRE_BEFORE_STREAM_ON
		ret = is_sensor_flash_fire(sensor_peri, flash->flash_data.intensity);
		if (ret) {
			err("failed to turn off flash at flash expired handler\n");
		}
#endif
		set_flash(itf, vsync_cnt + 2, CAM2_FLASH_MODE_OFF, 0, time);
		if (ret < 0) {
			pr_err("[%s] set_flash fail(%d)\n", __func__, ret);
		}
	}
	else if (mode == CAM2_FLASH_MODE_TORCH && on == true)
	{
		flash->flash_data.mode = mode;
		flash->flash_data.intensity = intensity;
		flash->flash_data.firing_time_us = time;
		flash->flash_data.high_resolution_flash = false;

		ret = is_sensor_flash_fire(sensor_peri, flash->flash_data.intensity);
		if (ret) {
			err("failed to turn off flash at flash expired handler\n");
		}
	}
	else
	{
		flash->flash_data.mode = mode;
		flash->flash_data.intensity = 0;
		flash->flash_data.firing_time_us = time;
		flash->flash_data.high_resolution_flash = false;

		ret = is_sensor_flash_fire(sensor_peri, flash->flash_data.intensity);
		if (ret) {
			err("failed to turn off flash at flash expired handler\n");
		}
	}

	info("[%s] high_resolution_flash(%d)", __func__, flash->flash_data.high_resolution_flash);

	if (flag)
		is_fpsimd_get_func();

	return ret;
}


int request_flash_expo_gain(struct is_sensor_interface *itf,
			struct is_flash_expo_gain *flash_ae)
{
	int ret = 0;
	int i = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_flash *flash;

	FIMC_BUG(!itf);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	if (!sensor_peri->flash) {
		err("flash is NULL");
		return -ENXIO;
	}

	flash = sensor_peri->flash;
	flash->flash_ae.flash_capture_cnt = flash_ae->flash_capture_cnt;

	dbg_flash("[%s] capture cnt(%d)\n", __func__, flash_ae->flash_capture_cnt);

	for (i = 0; i < 2; i++) {
		flash->flash_ae.expo[i] = flash_ae->expo[i];
		flash->flash_ae.tgain[i] = flash_ae->tgain[i];
		flash->flash_ae.again[i] = flash_ae->again[i];
		flash->flash_ae.dgain[i] = flash_ae->dgain[i];
		flash->flash_ae.long_expo[i] = flash_ae->long_expo[i];
		flash->flash_ae.long_tgain[i] = flash_ae->long_tgain[i];
		flash->flash_ae.long_again[i] = flash_ae->long_again[i];
		flash->flash_ae.long_dgain[i] = flash_ae->long_dgain[i];
		flash->flash_ae.short_expo[i] = flash_ae->short_expo[i];
		flash->flash_ae.short_tgain[i] = flash_ae->short_tgain[i];
		flash->flash_ae.short_again[i] = flash_ae->short_again[i];
		flash->flash_ae.short_dgain[i] = flash_ae->short_dgain[i];
		dbg_flash("[%s] expo(%d, %d, %d), again(%d, %d, %d), dgain(%d, %d, %d)\n",
			__func__,
			flash->flash_ae.expo[i],
			flash->flash_ae.long_expo[i],
			flash->flash_ae.short_expo[i],
			flash->flash_ae.again[i],
			flash->flash_ae.long_again[i],
			flash->flash_ae.short_again[i],
			flash->flash_ae.dgain[i],
			flash->flash_ae.long_dgain[i],
			flash->flash_ae.short_dgain[i]);
	}

	return ret;
}

int update_flash_dynamic_meta(struct is_sensor_interface *itf,
		u32 frame_count,
		camera2_ctl_t *ctrl,
		camera2_dm_t *dm,
		camera2_udm_t *udm)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_flash *flash;

	FIMC_BUG(!itf);
	FIMC_BUG(!ctrl);
	FIMC_BUG(!dm);
	FIMC_BUG(!udm);
	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	if (!sensor_peri->flash) {
		err("flash is NULL");
		return -ENXIO;
	}

	flash = sensor_peri->flash;

	dm->flash.flashMode =
		flash->expecting_flash_dm[frame_count % EXPECT_DM_NUM].flashMode;
	dm->flash.firingPower =
		flash->expecting_flash_dm[frame_count % EXPECT_DM_NUM].firingPower;
	dm->flash.firingTime =
		flash->expecting_flash_dm[frame_count % EXPECT_DM_NUM].firingTime;
	dm->flash.flashState =
		flash->expecting_flash_dm[frame_count % EXPECT_DM_NUM].flashState;

	dbg_flash("[%s][F:%d]: mode(%d), power(%d), time(%lld), state(%d)\n",
			__func__, frame_count, dm->flash.flashMode,
			dm->flash.firingPower,
			dm->flash.firingTime,
			dm->flash.flashState);

	return ret;
}

struct is_framemgr *get_vc_framemgr(
	struct is_sensor_interface *itf, enum itf_vc_buf_data_type request_data_type)
{
	struct is_device_sensor *sensor;
	struct is_sensor_cfg *sensor_cfg;
	struct is_device_csi *csi;
	struct pablo_internal_subdev *subdev = NULL;
	int wdma_vc;
	int vc_type;
	struct is_pdp *pdp;
	struct is_device_sensor_peri *sensor_peri;

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return ERR_PTR(-ENODEV);
	}

	sensor_cfg = sensor->cfg[sensor->nfi_toggle];
	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	pdp = sensor_peri->pdp;

	/* converting stat type used at RTA to Driver */
	switch (request_data_type) {
	case VC_BUF_DATA_TYPE_SENSOR_STAT1:
		vc_type = VC_TAILPDAF;
		for (wdma_vc = DMA_VIRTUAL_CH_0; wdma_vc < DMA_VIRTUAL_CH_MAX; wdma_vc++) {
			if (sensor_cfg->output[0][wdma_vc].type == VC_EMBEDDED2) {
				vc_type = VC_EMBEDDED2;
				break;
			}
		}
		break;
	case VC_BUF_DATA_TYPE_SENSOR_STAT2:
		vc_type = VC_EMBEDDED;
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT1:
		vc_type = VC_PRIVATE;
		subdev = &pdp->i_subdev[sensor->instance][PDP_SUBDEV_STAT];
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT2:
		vc_type = VC_MIPISTAT;
		subdev = &pdp->i_subdev[sensor->instance][PDP_SUBDEV_STAT];
		break;
	case VC_BUF_DATA_TYPE_SENSOR_STAT3:
		vc_type = VC_VPDAF;
		break;
	default:
		err("invalid data type(%d)", request_data_type);
		return ERR_PTR(-EINVAL);
	}

	if (!subdev || !test_bit(PABLO_SUBDEV_ALLOC, &subdev->state)) {
		for (wdma_vc = DMA_VIRTUAL_CH_0; wdma_vc < DMA_VIRTUAL_CH_MAX; wdma_vc++) {
			if (sensor_cfg->output[0][wdma_vc].type == vc_type)
				break;
		}

		if (wdma_vc == DMA_VIRTUAL_CH_MAX) {
			merr("requested stat. type(%d) is not supported with current config",
								sensor, request_data_type);
			return ERR_PTR(-EINVAL);
		}

		csi = (struct is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
		if (!csi) {
			merr("failed to get csi device", sensor);
			return ERR_PTR(-ENODEV);
		}

		return GET_SUBDEV_I_FRAMEMGR(&csi->i_subdev[csi->dq_toggle][0][wdma_vc]);
	}

	return GET_SUBDEV_I_FRAMEMGR(subdev);
}

int get_vc_dma_buf_by_fcount(struct is_sensor_interface *itf,
	enum itf_vc_buf_data_type request_data_type, u32 frame_count, u32 *buf_index, u64 *buf_addr)
{
	struct is_device_sensor *sensor;
	struct is_framemgr *framemgr;
	struct is_frame *frame;
	unsigned long flags;
	int ret;

	WARN_ON(!buf_addr);
	WARN_ON(!buf_index);

	*buf_addr = 0;
	*buf_index = 0;

	framemgr = get_vc_framemgr(itf, request_data_type);
	if (IS_ERR_OR_NULL(framemgr)) {
		err("failed to get_vc_framemgr");
		return -ENODEV;
	}

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	} else if (!test_bit(IS_SENSOR_FRONT_START, &sensor->state)) {
		mwarn("[T%d][F%d]sensor is NOT working.", sensor,
				request_data_type, frame_count);
		return -EINVAL;
	}

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_30, flags);
	if (!framemgr->frames) {
		mserr("framemgr was already closed", sensor, framemgr);
		ret = -EINVAL;
		goto err_get_framemgr;
	}

	frame = find_frame(framemgr, FS_FREE, frame_fcount, (void *)(ulong)frame_count);
	if (frame) {
		*buf_addr = frame->pb_output->iova_ext;
		*buf_index = frame->index;
		trans_frame(framemgr, frame, FS_PROCESS);
	} else {
		mserr("failed to get a frame: fcount: %d", sensor, framemgr, frame_count);
		ret = -EINVAL;
		goto err_invalid_frame;
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_30, flags);

	dbg_sensor(2, "[%s]: [%d][%s] index: %d, framecount: %d, addr: 0x%llx\n", __func__,
		frame->instance, framemgr->name, *buf_index, frame_count, *buf_addr);

	return 0;

err_invalid_frame:
err_get_framemgr:
	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_30, flags);

	return ret;
}

int put_vc_dma_buf_by_fcount(struct is_sensor_interface *itf,
		enum itf_vc_buf_data_type request_data_type,
		u32 frame_count)
{
	struct is_device_sensor *sensor;
	struct is_framemgr *framemgr;
	struct is_frame *frame;
	unsigned long flags;
	int ret;

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	}

	framemgr = get_vc_framemgr(itf, request_data_type);
	if (IS_ERR_OR_NULL(framemgr)) {
		err("failed to get_vc_framemgr");
		return -ENODEV;
	}

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_31, flags);
	if (!framemgr->frames) {
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_31, flags);
		mserr("framemgr was already closed", sensor, framemgr);
		return -EINVAL;
	}

	frame = find_frame(framemgr, FS_PROCESS, frame_fcount, (void *)(ulong)frame_count);
	if (frame) {
		trans_frame(framemgr, frame, FS_FREE);
	} else {
		mserr("failed to get a frame: fcount: %d", sensor, framemgr, frame_count);
		ret = -EINVAL;
		goto err_invalid_frame;
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_31, flags);

	dbg_sensor(1, "[%s]: [%d][%s] index: %d\n", __func__, frame->instance, framemgr->name,
		frame->index);

	return 0;

err_invalid_frame:
	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_31, flags);

	return ret;
}

int get_vc_dma_buf_info(struct is_sensor_interface *itf,
		enum itf_vc_buf_data_type request_data_type,
		struct vc_buf_info_t *buf_info)
{
	int ret = 0;
	struct is_module_enum *module;
	struct v4l2_subdev *subdev_module;
	struct is_device_sensor *sensor;
	struct is_sensor_cfg *sensor_cfg;
	u32 stat_vc, ch = DMA_VIRTUAL_CH_MAX;

	memset(buf_info, 0, sizeof(struct vc_buf_info_t));
	buf_info->stat_type = VC_STAT_TYPE_INVALID;
	buf_info->sensor_mode = VC_SENSOR_MODE_INVALID;

	module = get_subdev_module_enum(itf);
	if (!module) {
		err("failed to get sensor_peri's module");
		return -ENODEV;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module's subdev was not probed");
		return -ENODEV;
	}

	sensor = v4l2_get_subdev_hostdata(subdev_module);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	}

	sensor_cfg = sensor->cfg[sensor->nfi_toggle];

	switch (request_data_type) {
	case VC_BUF_DATA_TYPE_SENSOR_STAT1:
		for (ch = DMA_VIRTUAL_CH_1; ch < DMA_VIRTUAL_CH_MAX; ch++) {
			if (sensor_cfg->output[0][ch].type == VC_TAILPDAF)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_SENSOR_STAT2:
		for (ch = DMA_VIRTUAL_CH_1; ch < DMA_VIRTUAL_CH_MAX; ch++) {
			if (sensor_cfg->output[0][ch].type == VC_EMBEDDED)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT1:
		for (ch = DMA_VIRTUAL_CH_1; ch < DMA_VIRTUAL_CH_MAX; ch++) {
			if (sensor_cfg->output[0][ch].type == VC_PRIVATE)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_GENERAL_STAT2:
		for (ch = DMA_VIRTUAL_CH_1; ch < DMA_VIRTUAL_CH_MAX; ch++) {
			if (sensor_cfg->output[0][ch].type == VC_MIPISTAT)
				break;
		}
		break;
	case VC_BUF_DATA_TYPE_SENSOR_STAT3:
		for (ch = DMA_VIRTUAL_CH_1; ch < DMA_VIRTUAL_CH_MAX; ch++) {
			if (sensor_cfg->output[0][ch].type == VC_VPDAF)
				break;
		}
		break;
	default:
		warn("invalid data type(%d)", request_data_type);
		break;
	}

	if (ch == DMA_VIRTUAL_CH_MAX)
		return -EINVAL;

	stat_vc = sensor_cfg->hpd_vc[CSI_OTF_OUT_SINGLE];
	buf_info->stat_type = module->vc_extra_info[request_data_type].stat_type;
	buf_info->sensor_mode = module->vc_extra_info[request_data_type].sensor_mode;
	buf_info->width = sensor_cfg->input[stat_vc].width;
	buf_info->height = sensor_cfg->input[stat_vc].height;
	buf_info->element_size = module->vc_extra_info[request_data_type].max_element;

	if (sensor->obte_config & BIT(OBTE_CONFIG_BIT_VC_STAT)) {
		buf_info->stat_type = VC_STAT_TYPE_INVALID;
		buf_info->sensor_mode = VC_SENSOR_MODE_INVALID;
		buf_info->width = 0;
		buf_info->height = 0;
		buf_info->element_size = 0;
	}

	dbg_sensor(2, "%s r_type %d stat_vc %d stat_type %d mode %d size %dx%d element %dB\n",
			__func__,
			request_data_type,
			stat_vc,
			buf_info->stat_type,
			buf_info->sensor_mode,
			buf_info->width,
			buf_info->height,
			buf_info->element_size);

	return ret;
}

int get_vc_dma_buf_max_size(struct is_sensor_interface *itf,
		enum itf_vc_buf_data_type request_data_type,
		u32 *width,
		u32 *height,
		u32 *element_size)
{
	struct is_sensor_vc_extra_info *vc_extra_info;
	struct is_module_enum *module;

	module = get_subdev_module_enum(itf);
	if (unlikely(!module)) {
		err("failed to get sensor_peri's module");
		return -ENODEV;
	}

	if ((request_data_type <= VC_BUF_DATA_TYPE_INVALID) ||
		(request_data_type >= VC_BUF_DATA_TYPE_MAX)) {
		err("invalid data type(%d)", request_data_type);
		return -EINVAL;
	}

	if (module->vc_extra_info[request_data_type].stat_type == VC_STAT_TYPE_INVALID) {
		return 0;
	}

	vc_extra_info = &module->vc_extra_info[request_data_type];

	*width = vc_extra_info->max_width;
	*height = vc_extra_info->max_height;
	*element_size = vc_extra_info->max_element;

	dbg_sensor(2, "VC max buf (type(%d), width(%d), height(%d),element(%d byte))\n",
		request_data_type, *width, *height, *element_size);

	return (*width) * (*height) * (*element_size);
}

int csi_reserved(struct is_sensor_interface *itf)
{
	return 0;
}

int set_long_term_expo_mode(struct is_sensor_interface *itf,
		struct is_long_term_expo_mode *long_term_expo_mode)
{
	int ret = 0;
	int i = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_device_sensor *sensor;
	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);
	WARN_ON(!long_term_expo_mode);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("%s, failed to get sensor device", __func__);
		return -1;
	}

	/* as this function called, always set true for operate */
	sensor_peri->cis.long_term_mode.sen_strm_off_on_enable = true;
	sensor_peri->cis.long_term_mode.frm_num_strm_off_on_interval = long_term_expo_mode->frm_num_strm_off_on_interval;
	sensor_peri->cis.long_term_mode.sen_strm_off_on_step = 0;

	for (i = 0; i < 2; i++) {
		sensor_peri->cis.long_term_mode.expo[i] = long_term_expo_mode->expo[i];
		sensor_peri->cis.long_term_mode.tgain[i] = long_term_expo_mode->tgain[i];
		sensor_peri->cis.long_term_mode.again[i] = long_term_expo_mode->again[i];
		sensor_peri->cis.long_term_mode.dgain[i] = long_term_expo_mode->dgain[i];
	}

	sensor_peri->cis.long_term_mode.frame_interval = long_term_expo_mode->frm_num_strm_off_on_interval;
	sensor_peri->cis.long_term_mode.lemode_set.lemode = sensor_peri->cis.long_term_mode.sen_strm_off_on_enable;

	/*
	 * If this function called during streaming, lte_work should be enabled.
	 * If not(during not streaming), set lte exp/gain before stream on
	 */
	if (test_bit(IS_SENSOR_FRONT_START, &sensor->state)) {
		sensor_peri->cis.lte_work_enable = true;
	} else {
		sensor_peri->cis.mode_chg.exposure = long_term_expo_mode->expo[0];
		sensor_peri->cis.mode_chg.analog_gain = long_term_expo_mode->again[0];
		sensor_peri->cis.mode_chg.digital_gain = long_term_expo_mode->dgain[0];

		sensor_peri->cis.lte_work_enable = false;
	}

	dbg_sensor(1, "[%s]: expo[0](%d), expo[1](%d), again[0](%d), again[1](%d), "
		KERN_CONT "dgain[0](%d), again[1](%d), interval(%d), lte_work_enable(%d)\n", __func__,
		long_term_expo_mode->expo[0], long_term_expo_mode->expo[1],
		long_term_expo_mode->again[0], long_term_expo_mode->again[1],
		long_term_expo_mode->dgain[0], long_term_expo_mode->dgain[1],
		long_term_expo_mode->frm_num_strm_off_on_interval,
		sensor_peri->cis.lte_work_enable);

	return ret;
}

int set_lte_multi_capture_mode(struct is_sensor_interface *itf, bool lte_multi_capture_mode)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	WARN_ON(!cis_data);

	cis_data->lte_multi_capture_mode = lte_multi_capture_mode;

	info("[%s] lte_multi_capture_mode(%d)\n", __func__, lte_multi_capture_mode);

	return 0;
}

int set_hdr_mode(struct is_sensor_interface *itf, u32 mode)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	dbg_sensor(1, "ERR[%s][%d][%s] mode(%d) need to use SS_CTRL_CIS_REQUEST_SEAMLESS_MODE_CHANGE\n",
		__func__, sensor_peri->cis.id, sensor_peri->module->sensor_name, mode);

	return 0;
}

char *get_seamless_mode_prefix_string(enum is_rta_sensor_mode mode)
{
	char *prefix_str = NULL;

	switch (mode) {
	case SENSOR_MODE_LN2:
		prefix_str = "LN2";
		break;
	case SENSOR_MODE_LN4:
		prefix_str = "LN4";
		break;
	case SENSOR_MODE_REAL_12BIT:
		prefix_str = "12BIT";
		break;
	case SENSOR_MODE_CROPPED_RMS:
		prefix_str = "RM";
		break;
	case SENSOR_MODE_AEB:
		prefix_str = "AEB";
		break;
	case SENSOR_MODE_ADC11:
		prefix_str = "ADC11";
		break;
	default:
		prefix_str = "";
	}

	return prefix_str;
}

static void print_changed_seamless_mode(struct is_device_sensor_peri *sensor_peri,
	cis_shared_data *cis_data)
{
	struct is_sensor_seamless_ctl *pre_seamless_mode
		= &cis_data->seamless_history[cis_data->sen_vsync_count % NUM_FRAMES];
	struct is_sensor_seamless_ctl *cur_seamless_mode
		= &cis_data->cur_seamless_ctl;
	int i = 0;

	if (pre_seamless_mode->mode == cur_seamless_mode->mode)
		return;

	for (i = SENSOR_MODE_LN2; i < SENSOR_MODE_MAX; i = i << 1) {
		if ((pre_seamless_mode->mode & i) != (cur_seamless_mode->mode & i)) {
			if (i == SENSOR_MODE_CROPPED_RMS)
				ssinfo("AE_CTL %s[%d -> %d][%d -> %d]\n",
					&sensor_peri->cis, get_seamless_mode_prefix_string(i),
					(pre_seamless_mode->mode & i), (cur_seamless_mode->mode & i),
					pre_seamless_mode->remosaic_crop_zoom_ratio,
					cur_seamless_mode->remosaic_crop_zoom_ratio);
			else
				ssinfo("AE_CTL %s[%d -> %d]\n",
					&sensor_peri->cis, get_seamless_mode_prefix_string(i),
					(pre_seamless_mode->mode & i), (cur_seamless_mode->mode & i));
		}
	}
}

void set_cis_data_seamless_mode(struct is_sensor_interface *itf,
	cis_shared_data *cis_data)
{
	struct is_device_sensor *sensor = get_device_sensor(itf);

	if (cis_data->cur_seamless_ctl.mode & SENSOR_MODE_LN2)
		cis_data->cur_lownoise_mode = IS_CIS_LN2;
	else if (cis_data->cur_seamless_ctl.mode & SENSOR_MODE_LN4)
		cis_data->cur_lownoise_mode = IS_CIS_LN4;
	else if (cis_data->cur_seamless_ctl.mode & SENSOR_MODE_ADC11)
		cis_data->cur_lownoise_mode = IS_CIS_ADC11;
	else
		cis_data->cur_lownoise_mode = IS_CIS_LNOFF;

	if (cis_data->cur_seamless_ctl.mode & SENSOR_MODE_REAL_12BIT)
		cis_data->cur_12bit_mode = SENSOR_12BIT_STATE_REAL_12BIT;
	else {
		if (sensor->ex_mode_format == EX_FORMAT_12BIT)
			cis_data->cur_12bit_mode = SENSOR_12BIT_STATE_PSEUDO_12BIT;
		else
			cis_data->cur_12bit_mode = SENSOR_12BIT_STATE_OFF;
	}

	if (cis_data->cur_seamless_ctl.mode & SENSOR_MODE_AEB)
		cis_data->cur_hdr_mode = SENSOR_HDR_MODE_2AEB_2VC;
	else
		cis_data->cur_hdr_mode = SENSOR_HDR_MODE_SINGLE;

	if (cis_data->cur_seamless_ctl.mode & SENSOR_MODE_CROPPED_RMS) {
		cis_data->cur_bayer_pattern =
			cis_data->cur_seamless_ctl.remosaic_crop_zoom_ratio & 0x03; /* 0:RGB 1:TETRA */
		cis_data->cur_remosaic_zoom_ratio =
			(cis_data->cur_seamless_ctl.remosaic_crop_zoom_ratio >> 2) & 0xFF; /* 2 ~ 9 bit : Zoom Value */
	} else {
		cis_data->cur_seamless_ctl.remosaic_crop_zoom_ratio = 0;
		cis_data->cur_bayer_pattern = 0;
		cis_data->cur_remosaic_zoom_ratio = 0;
	}
}

static void check_seamless_mode_constraint(struct is_device_sensor_peri *sensor_peri,
	cis_shared_data *cis_data)
{
	/* aeb off during flash operation and forced aeb off when stream is off */
	if (!cis_data->stream_on && (cis_data->cur_seamless_ctl.mode & SENSOR_MODE_AEB)) {
		ssinfo("skip & reset AEB control during stream off (pre:%d, cur:%d)\n",
			&sensor_peri->cis, cis_data->pre_hdr_mode, cis_data->cur_hdr_mode);
		sensor_cis_set_cur_seamless_ctl_aeb_off(cis_data);
	}
}

int set_seamless_mode(struct is_sensor_interface *itf, struct is_sensor_seamless_ctl *seamless_mode)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	cis_shared_data *cis_data = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	WARN_ON(!cis_data);

	dbg_sensor_seamless_mode(&sensor_peri->cis, seamless_mode);
	memcpy(&cis_data->cur_seamless_ctl, seamless_mode, sizeof(struct is_sensor_seamless_ctl));

	print_changed_seamless_mode(sensor_peri, cis_data);
	check_seamless_mode_constraint(sensor_peri, cis_data);
	set_cis_data_seamless_mode(itf, cis_data);

	return 0;
}

int set_low_noise_mode(struct is_sensor_interface *itf, u32 mode)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	WARN_ON(!sensor_peri);

	dbg_sensor(1, "ERR[%s][%d][%s] mode(%d) need to use SS_CTRL_CIS_REQUEST_SEAMLESS_MODE_CHANGE\n",
		__func__, sensor_peri->cis.id, sensor_peri->module->sensor_name, mode);

	return 0;
}

int get_sensor_max_dynamic_fps(struct is_sensor_interface *itf,
			u32 *max_dynamic_fps)
{
	int ret = 0;
	struct is_device_sensor *sensor = NULL;
	u32 ex_mode;

	FIMC_BUG(!itf);
	FIMC_BUG(!max_dynamic_fps);

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -ENODEV;
	}

	ex_mode = is_sensor_g_ex_mode(sensor);
	if (ex_mode == EX_DUALFPS_960)
		*max_dynamic_fps = 960;
	else if (ex_mode == EX_DUALFPS_480)
		*max_dynamic_fps = 480;
	else
		*max_dynamic_fps = 0;

	return ret;
}

int get_static_mem(int ctrl_id, void **mem, int *size) {
	int err = 0;

	switch(ctrl_id) {
	case ITF_CTRL_ID_DDK:
		*mem = (void *)ddk_static_data;
		*size = (int)sizeof(ddk_static_data);
		break;
	case ITF_CTRL_ID_RTA:
		*mem = (void *)rta_static_data;
		*size = (int)sizeof(rta_static_data);
		break;
	default:
		err("invalid itf ctrl id %d", ctrl_id);
		*mem = NULL;
		*size = 0;
		err = -EINVAL;
		break;
	}

	return err;
}

int get_open_close_hint(int* opening, int* closing) {
#ifdef CONFIG_CAMERA_VENDOR_MCD
	struct is_core *core = is_get_is_core();
#endif

	*opening = IS_OPENING_HINT_NONE;
	*closing = IS_CLOSING_HINT_NONE;

#ifdef CONFIG_CAMERA_VENDOR_MCD
	if (core) {
		*opening = core->vendor.opening_hint;
		*closing = core->vendor.closing_hint;
	}
#endif

	dbg_sensor(1, "[%s] opening(%d), closing(%d)\n", __func__, *opening, *closing);

	return 0;
}

int set_mainflash_duration(struct is_sensor_interface *itf, u32 mainflash_duration)
{
	int ret = 0;
	u32 vsync_cnt = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_flash *flash;

	WARN_ON(!itf);
	WARN_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	if (!sensor_peri->flash) {
		err("flash is NULL");
		return -ENXIO;
	}

	flash = sensor_peri->flash;

	vsync_cnt = get_vsync_count(itf);

	if(mainflash_duration < 1) {
		dbg_flash("[%s] duration(%d) is too short\n", __func__, mainflash_duration);
		ret = -1;
		goto p_err;
	}
	else if(mainflash_duration > 5) {
		dbg_flash("[%s] duration(%d) is too long\n", __func__, mainflash_duration);
		ret = -1;
		goto p_err;
	}

	flash->flash_ae.frm_num_main_fls[1] = vsync_cnt + mainflash_duration + 1;

	dbg_flash("[%s] duration(%d)\n", __func__, mainflash_duration);

p_err:
	return ret;
}

int get_sensor_state(struct is_sensor_interface *itf)
{
	struct is_device_sensor *sensor;

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -1;
	}

	dbg("%s: sstream(%d)\n", sensor->phsycal_id, __func__, sensor->sstream);

	return sensor->sstream;
}

int get_reuse_3a_state(struct is_sensor_interface *itf,
				u32 *position, u32 *ae_exposure, u32 *ae_deltaev, bool is_clear)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	if (sensor_peri->reuse_3a_value) {
		if (sensor_peri->actuator) {
			if (position != NULL)
				*position = sensor_peri->actuator->position;
		}

		if (ae_exposure != NULL)
			*ae_exposure = sensor_peri->cis.ae_exposure;

		if (ae_deltaev != NULL)
			*ae_deltaev = sensor_peri->cis.ae_deltaev;

		if (is_clear)
			sensor_peri->reuse_3a_value = false;

		return true;
	}

	return false;
}

int set_reuse_ae_exposure(struct is_sensor_interface *itf,
				u32 ae_exposure, u32 ae_deltaev)
{
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);

	dbg_sensor(1, "[%s] set ae_exposure (%d)\n", __func__, ae_exposure);

	sensor_peri->cis.ae_exposure = ae_exposure;
	sensor_peri->cis.ae_deltaev = ae_deltaev;

	return 0;
}

int dual_reserved_0(struct is_sensor_interface *itf)
{
	return 0;
}

int dual_reserved_1(struct is_sensor_interface *itf)
{
	return 0;
}

int set_paf_param(struct is_sensor_interface *itf,
		struct paf_setting_t *regs, u32 regs_size)
{
	struct is_device_sensor_peri *sensor_peri;
	int ret;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	/* PDP */
	if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
		if (!sensor_peri->pdp || !sensor_peri->subdev_pdp) {
			err("invalid PDP state");
			return -EINVAL;
		}

		ret = CALL_PDPOPS(sensor_peri->pdp, set_param,
				sensor_peri->subdev_pdp,
				(struct cr_set *)regs, regs_size);
	} else {
		err("no PAF HW");
		return -ENODEV;
	}

	return ret;
}

int get_paf_ready(struct is_sensor_interface *itf, u32 *ready)
{
	struct is_device_sensor_peri *sensor_peri;
	int ret;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	/* PDP */
	if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
		if (!sensor_peri->pdp || !sensor_peri->subdev_pdp) {
			err("invalid PDP state");
			return -EINVAL;
		}

		ret = CALL_PDPOPS(sensor_peri->pdp, get_ready,
				sensor_peri->subdev_pdp, ready);
	} else {
		err("no PAF HW");
		return -ENODEV;
	}

	return ret;
}

int register_vc_dma_notifier(struct is_sensor_interface *itf,
			enum itf_vc_stat_type type,
			vc_dma_notifier_t notifier, void *data)
{
	struct is_device_sensor_peri *sensor_peri;
	int ret;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	/* PDP */
	if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
		if (!sensor_peri->pdp || !sensor_peri->subdev_pdp) {
			err("invalid PDP state");
			return -EINVAL;
		}

		ret = CALL_PDPOPS(sensor_peri->pdp, register_notifier,
				sensor_peri->subdev_pdp,
				type, notifier, data);
	} else {
		err("no PAF HW");
		return -ENODEV;
	}

	return ret;
}

int unregister_vc_dma_notifier(struct is_sensor_interface *itf,
			enum itf_vc_stat_type type,
			vc_dma_notifier_t notifier)
{
	struct is_device_sensor_peri *sensor_peri;
	int ret;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	/* PDP */
	if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
		if (!sensor_peri->pdp || !sensor_peri->subdev_pdp) {
			err("invalid PDP state");
			return -EINVAL;
		}

		ret = CALL_PDPOPS(sensor_peri->pdp, unregister_notifier,
				sensor_peri->subdev_pdp,
				type, notifier);
	} else {
		err("no PAF HW");
		return -ENODEV;
	}

	return ret;
}

int paf_reserved(struct is_sensor_interface *itf)
{
	return -EINVAL;
}

int get_laser_distance(struct is_sensor_interface *itf, enum itf_laser_af_type *type,
		union itf_laser_af_data *data)
{
	struct is_device_sensor_peri *sensor_peri;
	struct is_laser_af *laser_af;
	int ret;
	u32 size = 0;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	laser_af = sensor_peri->laser_af;
	if (!laser_af) {
		err("failed to get laser_af");
		return -ENODEV;
	}

	dbg_actuator("%s", __func__);

	ret = CALL_LASEROPS(laser_af, get_distance, sensor_peri->subdev_laser_af,
			(void *)data, &size);
	if (ret)
		err("INVALID_LASER_DISTANCE");

	*type = laser_af->id;

	return ret;
}

#ifdef USE_TOF_AF
int get_tof_af_data(struct is_sensor_interface *itf, struct tof_data_t *data)
{
	int ret=1;
	struct is_core *core;
	struct is_vendor_private *vendor_priv;

	core = (struct is_core *)dev_get_drvdata(is_dev);
	vendor_priv = core->vendor.private_data;

	mutex_lock(&vendor_priv->tof_af_lock);

	memcpy(data, &vendor_priv->tof_af_data, sizeof(struct tof_data_t));

	mutex_unlock(&vendor_priv->tof_af_lock);
	return ret;
}
#endif

int request_wb_gain(struct is_sensor_interface *itf,
		u32 gr_gain, u32 r_gain, u32 b_gain, u32 gb_gain)
{
	struct is_device_sensor_peri *sensor_peri = NULL;
	struct is_device_sensor *sensor;
	struct is_sensor_ctl *sensor_ctl = NULL;
	int i;
	u32 frame_count = 0, num_of_frame = 1;

	BUG_ON(!itf);
	BUG_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -1;
	}

	if (!test_bit(IS_SENSOR_FRONT_START, &sensor->state)) {
		sensor_peri->cis.mode_chg_wb_gains.gr = gr_gain;
		sensor_peri->cis.mode_chg_wb_gains.r = r_gain;
		sensor_peri->cis.mode_chg_wb_gains.b = b_gain;
		sensor_peri->cis.mode_chg_wb_gains.gb = gb_gain;
	}

	frame_count = get_frame_count(itf);
	get_num_of_frame_per_one_3aa(itf, &num_of_frame);

	for (i = 0; i < num_of_frame; i++) {
		sensor_ctl = get_sensor_ctl_from_module(itf, frame_count + i);
		BUG_ON(!sensor_ctl);

		sensor_ctl->wb_gains.gr = gr_gain;
		sensor_ctl->wb_gains.r = r_gain;
		sensor_ctl->wb_gains.b = b_gain;
		sensor_ctl->wb_gains.gb = gb_gain;

		if (i == 0)
			sensor_ctl->update_wb_gains = true;
	}

	dbg_sensor(1, "[%s] stream %s, wb gains(gr:%d, r:%d, b:%d, gb:%d)\n",
		__func__,
		test_bit(IS_SENSOR_FRONT_START, &sensor->state) ? "on" : "off",
		gr_gain, r_gain, b_gain, gb_gain);

	return 0;
}

int set_sensor_info_mfhdr_mode_change(struct is_sensor_interface *itf,
		u32 count, u32 *long_expo, u32 *long_again, u32 *long_dgain,
		u32 *expo, u32 *again, u32 *dgain, u32 *sensitivity)
{
	struct is_device_sensor_peri *sensor_peri;
	struct is_device_sensor *sensor;
	camera2_sensor_uctl_t *sensor_uctl;
	struct is_sensor_ctl *sensor_ctl;
	int idx;

	BUG_ON(!itf);
	BUG_ON(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);

	sensor = get_device_sensor(itf);
	if (!sensor) {
		err("failed to get sensor device");
		return -1;
	}

	if (test_bit(IS_SENSOR_FRONT_START, &sensor->state))
		warn("called during stream on");

	if (count < 1) {
		err("wrong request count(%d)", count);
		return -1;
	}

	/* set index 0 values to mode_chg_xxx variables
	 * for applying values before stream on
	 */
	sensor_peri->cis.mode_chg.short_exposure = expo[0];
	sensor_peri->cis.mode_chg.short_analog_gain = again[0];
	sensor_peri->cis.mode_chg.short_digital_gain = dgain[0];

	sensor_peri->cis.mode_chg.exposure = long_expo[0];
	sensor_peri->cis.mode_chg.analog_gain = long_again[0];
	sensor_peri->cis.mode_chg.digital_gain = long_dgain[0];

	sensor_peri->cis.mode_chg.long_exposure = long_expo[0];
	sensor_peri->cis.mode_chg.long_analog_gain = long_again[0];
	sensor_peri->cis.mode_chg.long_digital_gain = long_dgain[0];

	/* set index 0 ~ cnt-1 values to sensor uctl variables
	 * for applying values during streaming.
	 * (index 0 is set for code clean(not updated during streaming)
	 * set "use_sensor_work" to true for call sensor_work_thread
	 * to apply sensor settings
	 */
	sensor_peri->use_sensor_work = true;
	for (idx = 0; idx < count; idx++) {
		sensor_ctl = get_sensor_ctl_from_module(itf, get_frame_count(itf) + idx);
		sensor_ctl->force_update = true;

		sensor_uctl = get_sensor_uctl_from_module(itf, get_frame_count(itf) + idx);
		BUG_ON(!sensor_uctl);

		sensor_uctl->exposureTime = is_sensor_convert_us_to_ns(long_expo[idx]);
		sensor_uctl->longExposureTime = is_sensor_convert_us_to_ns(long_expo[idx]);
		sensor_uctl->shortExposureTime = is_sensor_convert_us_to_ns(expo[idx]);

		sensor_uctl->sensitivity = sensitivity ? sensitivity[idx] : 0;
		sensor_uctl->analogGain = again[idx];
		sensor_uctl->digitalGain = dgain[idx];
		sensor_uctl->longAnalogGain = long_again[idx];
		sensor_uctl->shortAnalogGain = again[idx];
		sensor_uctl->longDigitalGain = long_dgain[idx];
		sensor_uctl->shortDigitalGain = dgain[idx];

		set_sensor_uctl_valid(itf, idx);

		dbg_sensor(1, "[%s][%d]: exp(%d), again(%d), dgain(%d), "
			KERN_CONT "long_exp(%d), long_again(%d), long_dgain(%d), sensitivity(%d)\n",
			__func__, idx,
			expo[idx], again[idx], dgain[idx],
			long_expo[idx], long_again[idx], long_dgain[idx],
			sensor_uctl->sensitivity);
	}

	return 0;
}

int get_sensor_min_frame_duration(struct is_sensor_interface *itf,
			u32 *min_frame_duration)
{
	int ret = 0;
	struct is_device_sensor_peri *sensor_peri = NULL;

	FIMC_BUG(!itf);
	FIMC_BUG(!min_frame_duration);

	sensor_peri = container_of(itf, struct is_device_sensor_peri, sensor_interface);
	FIMC_BUG(!sensor_peri);
	FIMC_BUG(!sensor_peri->cis.cis_data);

	*min_frame_duration = sensor_peri->cis.cis_data->base_min_frame_us_time;

	return ret;
}

int get_sensor_min_frame_duration_dummy(struct is_sensor_interface *itf, u32 *min_frame_duration)
{
	*min_frame_duration = 0;

	return 0;
}

int get_sensor_seamless_mode_info(struct is_sensor_interface *itf,
				struct is_sensor_mode_info *seamless_mode_info,
				u32 *seamless_mode_cnt)
{
	struct is_device_sensor_peri *sensor_peri;
	cis_shared_data *cis_data;
	int i;

	if (!itf) {
		err("invalid sensor interface");
		return -EINVAL;
	}

	FIMC_BUG(itf->magic != SENSOR_INTERFACE_MAGIC);

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	cis_data = sensor_peri->cis.cis_data;
	FIMC_BUG(!cis_data);

	*seamless_mode_cnt = cis_data->seamless_mode_cnt;

	memcpy(seamless_mode_info, cis_data->seamless_mode_info,
		sizeof(struct is_sensor_mode_info) * cis_data->seamless_mode_cnt);

	for (i = 0; i < cis_data->seamless_mode_cnt; i++)
		dbg_sensor(1, "%s [%d] mode[%d] expo[%d-%d] again[%d-%d] dgain[%d-%d] vvalid[%d] vblank[%d] max_fps[%d]\n",
			sensor_peri->module->sensor_name, i,
			seamless_mode_info[i].mode,
			seamless_mode_info[i].min_expo,
			seamless_mode_info[i].max_expo,
			seamless_mode_info[i].min_again,
			seamless_mode_info[i].max_again,
			seamless_mode_info[i].min_dgain,
			seamless_mode_info[i].max_dgain,
			seamless_mode_info[i].vvalid_time,
			seamless_mode_info[i].vblank_time,
			seamless_mode_info[i].max_fps);

	return 0;
}

int get_sensor_seamless_mode_info_dummy(struct is_sensor_interface *itf,
	struct is_sensor_mode_info *seamless_mode_info, u32 *seamless_mode_cnt)
{
	*seamless_mode_cnt = 0;

	return 0;
}

int get_sensor_applied_seamless_mode(struct is_sensor_interface *itf,
				struct is_sensor_seamless_ctl *applied_seamless_mode, u32 fcount)
{
	struct is_device_sensor_peri *sensor_peri;
	cis_shared_data *cis_data;
	unsigned int ref_history_index;
	unsigned int ctrl_delay = 0;
	u32 vsync_count;

	if (unlikely(!itf) || (itf->magic != SENSOR_INTERFACE_MAGIC))
		goto p_err;

	sensor_peri = container_of(itf, struct is_device_sensor_peri,
			sensor_interface);

	cis_data = sensor_peri->cis.cis_data;
	if (unlikely(!cis_data))
		goto p_err;

	vsync_count = (fcount > cis_data->sen_vsync_count) ? fcount : cis_data->sen_vsync_count;

	if (sensor_peri->cis.ctrl_delay == N_PLUS_ONE_FRAME)
		ctrl_delay = 1;
	else
		ctrl_delay = 2;

	ref_history_index = (NUM_FRAMES + vsync_count - ctrl_delay) % NUM_FRAMES;

	memcpy(applied_seamless_mode,
		&cis_data->seamless_history[ref_history_index],
		sizeof(struct is_sensor_seamless_ctl));

	if (!cis_data->stream_on)
		applied_seamless_mode->mode &= ~SENSOR_MODE_AEB;

	if (cis_data->seamless_update_vsync_cnt + ctrl_delay == vsync_count)
		ssinfo_seamless_mode(&sensor_peri->cis, applied_seamless_mode);
	else
		dbg_sensor_seamless_mode(&sensor_peri->cis, applied_seamless_mode);

	return 0;
p_err:
	err("invalid sensor interface");
	return 0;
}

int get_delayed_preflash_time(struct is_sensor_interface *itf, u32 *delayedTime)
{
	// Need to change
	return 0;
}

/* Dummy APIs for virtual(zebu) sensor env. */
static int get_next_frame_timing_dummy(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *exposure,
			u32 *frame_period,
			u64 *line_period)
{
	/* No ops */
	return 0;
}

static int get_frame_timing_dummy(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *exposure,
			u32 *frame_period,
			u64 *line_period)
{
	int ret = 0;

	FIMC_BUG(!itf);
	FIMC_BUG(!exposure);

	ret =  get_interface_param(itf, itf->cis_mode, ITF_CIS_PARAM_EXPOSURE, CURRENT_FRAME, num_data, exposure);
	if (ret < 0)
		pr_err("[%s] get_interface_param EXPOSURE fail(%d)\n", __func__, ret);

	dbg_sensor(2, "[%s](%d:%d) cnt(%d): exp(L(%d), S(%d), M(%d))\n", __func__,
		get_vsync_count(itf), get_frame_count(itf), num_data,
		exposure[EXPOSURE_GAIN_LONG],
		exposure[EXPOSURE_GAIN_SHORT],
		exposure[EXPOSURE_GAIN_MIDDLE]);

	return ret;
}

static int get_sensor_flag_dummy(struct is_sensor_interface *itf,
		enum is_sensor_stat_control *stat_control_type,
		enum is_sensor_hdr_mode *hdr_mode_type,
		enum is_sensor_12bit_mode *sensor_12bit_mode_type,
		u32 *exposure_count)
{
	FIMC_BUG(!stat_control_type);
	FIMC_BUG(!hdr_mode_type);
	FIMC_BUG(!exposure_count);

	*stat_control_type = SENSOR_STAT_NOTHING;
	*hdr_mode_type = SENSOR_HDR_MODE_SINGLE;
	*exposure_count = EXPOSURE_GAIN_COUNT_1;

	return 0;
}

static int get_sensor_frame_timing_dummy(struct is_sensor_interface *itf,
			u32 *vvalid_time,
			u32 *vblank_time)
{
	/* No ops */
	return 0;
}

static int get_sensor_cur_size_dummy(struct is_sensor_interface *itf,
				u32 *cur_x,
				u32 *cur_y,
				u32 *cur_width,
				u32 *cur_height)
{
	/* No ops */
	return 0;
}

static int get_sensor_max_fps_dummy(struct is_sensor_interface *itf,
			u32 *max_fps)
{
	/* No ops */
	return 0;
}

static int get_sensor_cur_fps_dummy(struct is_sensor_interface *itf,
			u32 *cur_fps)
{
	/* No ops */
	return 0;
}

static int set_initial_exposure_of_setfile_dummy(struct is_sensor_interface *itf,
				u32 expo)
{
	/* No ops */
	return 0;
}

static int set_num_of_frame_per_one_3aa_dummy(struct is_sensor_interface *itf,
				u32 *num_of_frame)
{
	/* No ops */
	return 0;
}

static int request_reset_expo_gain_dummy(struct is_sensor_interface *itf,
			enum is_exposure_gain_count num_data,
			u32 *expo,
			u32 *tgain,
			u32 *again,
			u32 *dgain)
{
	/* No ops */
	return 0;
}

static int update_sensor_dynamic_meta_dummy(struct is_sensor_interface *itf,
		u32 frame_count,
		camera2_ctl_t *ctrl,
		camera2_dm_t *dm,
		camera2_udm_t *udm)
{
	/* No ops */
	return 0;
}

static int copy_sensor_ctl_dummy(struct is_sensor_interface *itf,
			u32 frame_count,
			camera2_shot_t *shot)
{
	/* No ops */
	return 0;
}

static u32 request_frame_length_line_dummy(struct is_sensor_interface *itf,
		u32 framelengthline)
{
	/* No ops */
	return 0;
}

static int set_low_noise_mode_dummy(struct is_sensor_interface *itf, u32 mode)
{
	/* No ops */
	return 0;
}

static u32 get_sensor_12bit_state_dummy(struct is_sensor_interface *itf, enum is_sensor_12bit_state *state)
{
	/* No ops */
	return 0;
}

int init_sensor_interface(struct is_sensor_interface *itf)
{
	int ret = 0;

	itf->magic = SENSOR_INTERFACE_MAGIC;
	itf->vsync_flag = false;

	/* Default scenario is OTF */
	itf->otf_flag_3aa = true;
	/* TODO: check cis mode */
	itf->cis_mode = ITF_CIS_SMIA_WDR;
	/* OTF default is 3 frame delay */
	itf->diff_bet_sen_isp = itf->otf_flag_3aa ? DIFF_OTF_DELAY : DIFF_M2M_DELAY;

	/* struct is_cis_interface_ops */
	itf->cis_itf_ops.request_reset_interface = request_reset_interface;
	itf->cis_itf_ops.get_calibrated_size = get_calibrated_size;
	itf->cis_itf_ops.get_bayer_order = get_bayer_order;
	itf->cis_itf_ops.get_min_exposure_time = get_min_exposure_time;
	itf->cis_itf_ops.get_max_exposure_time = get_max_exposure_time;
	itf->cis_itf_ops.get_min_analog_gain = get_min_analog_gain;
	itf->cis_itf_ops.get_max_analog_gain = get_max_analog_gain;
	itf->cis_itf_ops.get_min_digital_gain = get_min_digital_gain;
	itf->cis_itf_ops.get_max_digital_gain = get_max_digital_gain;

	itf->cis_itf_ops.get_vsync_count = get_vsync_count;
	itf->cis_itf_ops.get_vblank_count = get_vblank_count;
	itf->cis_itf_ops.is_vvalid_period = is_vvalid_period;

	itf->cis_itf_ops.request_exposure = request_exposure;
	itf->cis_itf_ops.adjust_exposure = adjust_exposure;

	itf->cis_itf_ops.request_analog_gain = request_analog_gain;
	itf->cis_itf_ops.request_gain = request_gain;

	itf->cis_itf_ops.adjust_analog_gain = adjust_analog_gain;
	itf->cis_itf_ops.get_next_analog_gain = get_next_analog_gain;
	itf->cis_itf_ops.get_analog_gain = get_analog_gain;

	itf->cis_itf_ops.get_next_digital_gain = get_next_digital_gain;
	itf->cis_itf_ops.get_digital_gain = get_digital_gain;

	itf->cis_itf_ops.is_actuator_available = is_actuator_available;
	itf->cis_itf_ops.is_flash_available = is_flash_available;
	itf->cis_itf_ops.get_sensor_type = get_sensor_type;
	itf->cis_itf_ops.is_ois_available = is_ois_available;
	itf->cis_itf_ops.is_aperture_available = is_aperture_available;
	itf->cis_itf_ops.is_laser_af_available = is_laser_af_available;
	itf->cis_itf_ops.is_tof_af_available = is_tof_af_available;
	itf->cis_itf_ops.get_hdr_ratio_ctl_by_again = get_hdr_ratio_ctl_by_again;
	itf->cis_itf_ops.get_sensor_use_dgain = get_sensor_use_dgain;
	itf->cis_itf_ops.get_sensor_initial_aperture = get_sensor_initial_aperture;
	itf->cis_itf_ops.set_alg_reset_flag = set_alg_reset_flag;
	itf->cis_ext_itf_ops.get_sensor_hdr_stat = get_sensor_hdr_stat;
	itf->cis_ext_itf_ops.set_3a_alg_res_to_sens = set_3a_alg_res_to_sens;

	itf->cis_itf_ops.reserved0 = reserved0;
	itf->cis_itf_ops.reserved1 = reserved1;

	itf->cis_itf_ops.set_cur_uctl_list = set_cur_uctl_list;

	/* TODO: What is diff with apply_frame_settings at event_ops */
	itf->cis_itf_ops.apply_sensor_setting = apply_sensor_setting;

	itf->cis_itf_ops.set_sensor_info_mode_change = set_sensor_info_mode_change;
	itf->cis_itf_ops.get_module_id = get_module_id;
	itf->cis_itf_ops.get_module_position = get_module_position;
	itf->cis_itf_ops.set_sensor_3a_mode = set_sensor_3a_mode;
	itf->cis_itf_ops.get_initial_exposure_gain_of_sensor = get_initial_exposure_gain_of_sensor;
	itf->cis_itf_ops.get_sensor_frameid = get_sensor_frameid;
	itf->cis_ext_itf_ops.change_cis_mode = change_cis_mode;

	/* struct is_cis_event_ops */
	itf->cis_evt_ops.start_of_frame = start_of_frame;
	itf->cis_evt_ops.end_of_frame = end_of_frame;
	itf->cis_evt_ops.apply_frame_settings = apply_frame_settings;

	/* CIS ext2 interface */
	/* Long Term Exposure mode(LTE mode) interface */
	itf->cis_ext2_itf_ops.set_long_term_expo_mode = set_long_term_expo_mode;
	itf->cis_ext2_itf_ops.get_sensor_max_dynamic_fps = get_sensor_max_dynamic_fps;
	itf->cis_ext2_itf_ops.get_static_mem = get_static_mem;
	itf->cis_ext2_itf_ops.get_open_close_hint = get_open_close_hint;
	itf->cis_ext2_itf_ops.set_mainflash_duration = set_mainflash_duration;
	itf->cis_ext2_itf_ops.set_previous_dm = set_previous_dm;
	itf->cis_ext2_itf_ops.set_hdr_mode = set_hdr_mode;
	itf->cis_ext_itf_ops.set_adjust_sync = set_adjust_sync;
	itf->cis_ext_itf_ops.request_sensitivity = request_sensitivity;
	itf->cis_ext_itf_ops.set_sensor_stat_control_mode_change = set_sensor_stat_control_mode_change;
	itf->cis_ext_itf_ops.set_sensor_roi_control = set_sensor_roi_control;
	itf->cis_ext_itf_ops.set_sensor_stat_control_per_frame = set_sensor_stat_control_per_frame;
	itf->cis_ext_itf_ops.set_sensor_12bit_state = set_sensor_12bit_state;
	itf->cis_ext_itf_ops.get_sensor_12bit_state = get_sensor_12bit_state;
#ifdef SUPPORT_SENSOR_SEAMLESS_3HDR
	itf->cis_ext_itf_ops.set_sensor_lsc_table_init = set_sensor_lsc_table_init;
	itf->cis_ext_itf_ops.set_sensor_tone_control = set_sensor_tone_control;
	itf->cis_ext_itf_ops.set_sensor_ev_control = set_sensor_ev_control;
#endif

	itf->cis_ext2_itf_ops.request_wb_gain = request_wb_gain;
	itf->cis_ext2_itf_ops.set_sensor_info_mfhdr_mode_change = set_sensor_info_mfhdr_mode_change;
	itf->cis_ext2_itf_ops.request_direct_flash = request_direct_flash;
	itf->cis_ext2_itf_ops.set_lte_multi_capture_mode = set_lte_multi_capture_mode;
	itf->cis_ext2_itf_ops.get_delayed_preflash_time = get_delayed_preflash_time;
	itf->cis_ext2_itf_ops.set_remosaic_zoom_ratio = set_remosaic_zoom_ratio;
	itf->cis_ext2_itf_ops.get_sensor_min_frame_duration = get_sensor_min_frame_duration;
	itf->cis_ext2_itf_ops.get_sensor_seamless_mode_info = get_sensor_seamless_mode_info;
	itf->cis_ext2_itf_ops.get_sensor_applied_seamless_mode = get_sensor_applied_seamless_mode;
	/*
	 * For ZEBU environment, some CIS APIs could not be run
	 * because there is no actual CIS module.
	 */
	if (IS_ENABLED(CONFIG_CAMERA_CIS_ZEBU_OBJ)) {
		itf->cis_itf_ops.get_next_frame_timing = get_next_frame_timing_dummy;
		itf->cis_itf_ops.get_frame_timing = get_frame_timing_dummy;
		itf->cis_itf_ops.get_sensor_frame_timing = get_sensor_frame_timing_dummy;
		itf->cis_itf_ops.get_sensor_cur_size = get_sensor_cur_size_dummy;
		itf->cis_itf_ops.get_sensor_max_fps = get_sensor_max_fps_dummy;
		itf->cis_itf_ops.get_sensor_cur_fps = get_sensor_cur_fps_dummy;
		itf->cis_itf_ops.set_initial_exposure_of_setfile = set_initial_exposure_of_setfile_dummy;
		itf->cis_itf_ops.set_num_of_frame_per_one_3aa = set_num_of_frame_per_one_3aa_dummy;
		itf->cis_itf_ops.update_sensor_dynamic_meta = update_sensor_dynamic_meta_dummy;
		itf->cis_itf_ops.copy_sensor_ctl = copy_sensor_ctl_dummy;
		itf->cis_itf_ops.request_reset_expo_gain = request_reset_expo_gain_dummy;

		itf->cis_ext_itf_ops.get_sensor_flag = get_sensor_flag_dummy;
		itf->cis_ext_itf_ops.request_frame_length_line = request_frame_length_line_dummy;

		itf->cis_ext2_itf_ops.set_low_noise_mode = set_low_noise_mode_dummy;
		itf->cis_ext_itf_ops.get_sensor_12bit_state = get_sensor_12bit_state_dummy;
		itf->cis_ext2_itf_ops.get_sensor_seamless_mode_info =
			get_sensor_seamless_mode_info_dummy;
		itf->cis_ext2_itf_ops.get_sensor_min_frame_duration =
			get_sensor_min_frame_duration_dummy;
	} else {
		itf->cis_itf_ops.get_next_frame_timing = get_next_frame_timing;
		itf->cis_itf_ops.get_frame_timing = get_frame_timing;
		itf->cis_itf_ops.get_sensor_frame_timing = get_sensor_frame_timing;
		itf->cis_itf_ops.get_sensor_cur_size = get_sensor_cur_size;
		itf->cis_itf_ops.get_sensor_max_fps = get_sensor_max_fps;
		itf->cis_itf_ops.get_sensor_cur_fps = get_sensor_cur_fps;
		itf->cis_itf_ops.set_initial_exposure_of_setfile = set_initial_exposure_of_setfile;
		itf->cis_itf_ops.set_num_of_frame_per_one_3aa = set_num_of_frame_per_one_3aa;
		itf->cis_itf_ops.update_sensor_dynamic_meta = update_sensor_dynamic_meta;
		itf->cis_itf_ops.copy_sensor_ctl = copy_sensor_ctl;
		/* reset exposure and gain for flash */
		itf->cis_itf_ops.request_reset_expo_gain = request_reset_expo_gain;

		itf->cis_ext_itf_ops.get_sensor_flag = get_sensor_flag;
		itf->cis_ext_itf_ops.request_frame_length_line = request_frame_length_line;

		itf->cis_ext2_itf_ops.set_low_noise_mode = set_low_noise_mode;
		itf->cis_ext2_itf_ops.set_seamless_mode = set_seamless_mode;
		itf->cis_ext_itf_ops.get_sensor_12bit_state = get_sensor_12bit_state;
		itf->cis_ext2_itf_ops.get_sensor_seamless_mode_info = get_sensor_seamless_mode_info;
		itf->cis_ext2_itf_ops.get_sensor_min_frame_duration = get_sensor_min_frame_duration;
	}

	/* Actuator interface */
	itf->actuator_itf.soft_landing_table.enable = false;
	itf->actuator_itf.position_table.enable = false;
	itf->actuator_itf.initialized = false;

	itf->actuator_itf_ops.set_actuator_position_table = set_actuator_position_table;
	itf->actuator_itf_ops.set_soft_landing_config = set_soft_landing_config;
	itf->actuator_itf_ops.set_position = set_position;
	itf->actuator_itf_ops.get_cur_frame_position = get_cur_frame_position;
	itf->actuator_itf_ops.get_applied_actual_position = get_applied_actual_position;
	itf->actuator_itf_ops.get_prev_frame_position = get_prev_frame_position;
	itf->actuator_itf_ops.set_af_window_position = set_af_window_position; /* AF window value for M2M AF */
	itf->actuator_itf_ops.get_status = get_status;

	/* Flash interface */
	itf->flash_itf_ops.request_flash = request_flash;
	itf->flash_itf_ops.request_flash_expo_gain = request_flash_expo_gain;
	itf->flash_itf_ops.update_flash_dynamic_meta = update_flash_dynamic_meta;

	/* Aperture interface */
	itf->aperture_itf_ops.set_aperture_value = set_aperture_value;
	itf->aperture_itf_ops.get_aperture_value = get_aperture_value;

	itf->paf_itf_ops.set_paf_param = set_paf_param;
	itf->paf_itf_ops.get_paf_ready = get_paf_ready;
	itf->paf_itf_ops.reserved[0] = paf_reserved;
	itf->paf_itf_ops.reserved[1] = paf_reserved;
	itf->paf_itf_ops.reserved[2] = paf_reserved;
	itf->paf_itf_ops.reserved[3] = paf_reserved;

//	itf->laser_af_itf_ops.set_active = set_laser_active; // TEMP_OLYMPUS
	itf->laser_af_itf_ops.get_distance = get_laser_distance;

#ifdef USE_TOF_AF
	itf->tof_af_itf_ops.get_data = get_tof_af_data;
#endif
	/* MIPI-CSI interface */
	itf->csi_itf_ops.get_vc_dma_buf = get_vc_dma_buf_by_fcount;
	itf->csi_itf_ops.put_vc_dma_buf = put_vc_dma_buf_by_fcount;
	itf->csi_itf_ops.get_vc_dma_buf_info = get_vc_dma_buf_info;
	itf->csi_itf_ops.get_vc_dma_buf_max_size = get_vc_dma_buf_max_size;
	itf->csi_itf_ops.register_vc_dma_notifier = register_vc_dma_notifier;
	itf->csi_itf_ops.unregister_vc_dma_notifier = unregister_vc_dma_notifier;
	itf->csi_itf_ops.reserved[0] = csi_reserved;
	itf->csi_itf_ops.reserved[1] = csi_reserved;

	/* Sensor dual sceanrio interface */
	itf->dual_itf_ops.get_sensor_state = get_sensor_state;
	itf->dual_itf_ops.get_reuse_3a_state = get_reuse_3a_state;
	itf->dual_itf_ops.set_reuse_ae_exposure = set_reuse_ae_exposure;
	itf->dual_itf_ops.reserved[0] = dual_reserved_0;
	itf->dual_itf_ops.reserved[1] = dual_reserved_1;

	return ret;
}

