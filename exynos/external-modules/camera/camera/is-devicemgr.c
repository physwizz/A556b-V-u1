/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * exynos5 fimc-is group manager functions
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>
#include <linux/v4l2-mediabus.h>
#include <linux/bug.h>

#include "pablo-lib.h"
#include "is-hw.h"
#include "is-core.h"
#include "is-type.h"
#include "is-err.h"
#include "is-video.h"
#include "pablo-framemgr.h"
#include "is-groupmgr.h"
#include "is-devicemgr.h"
#include "is-device-ischain.h"
#include "interface/is-interface-library.h"

static struct is_devicemgr devicemgr;

static void do_sensor_tag(unsigned long data)
{
	int ret = 0;
	u32 stream;
	unsigned long framemgr_flag;
	struct is_framemgr *framemgr;
	struct camera2_node ldr_node = {0, };
	struct is_device_sensor *sensor;
	struct devicemgr_sensor_tag_data *tag_data;
	struct is_group *group;
	struct is_frame *frame;
	struct is_group_task *gtask;

	tag_data = (struct devicemgr_sensor_tag_data *)data;
	stream = tag_data->stream;
	sensor = tag_data->devicemgr->sensor[stream];
	group = tag_data->group;
	gtask = get_group_task(group->id);

	mgrdbgs(5, "start sensor tag(%s)\n", group->device, group, tag_data,
			in_softirq() ? "S" : "H");

	if (unlikely(test_bit(IS_GROUP_FORCE_STOP, &group->state))) {
		mgwarn(" cancel by fstop", group, group);
		goto p_err;
	}

	if (unlikely(test_bit(IS_GTASK_REQUEST_STOP, &gtask->state))) {
		mgerr(" cancel by gstop", group, group);
		goto p_err;
	}

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", group);
		return;
	}

	framemgr_e_barrier_irqs(framemgr, 0, framemgr_flag);
	frame = find_frame(framemgr, FS_PROCESS, frame_fcount, (void *)(ulong)tag_data->fcount);

	if (!frame) {
		frame_manager_print_queues(framemgr);
		merr("[F%d] There's no frame in processing." \
			"Can't sync sensor and ischain buffer anymore..",
			group, tag_data->fcount);
		framemgr_x_barrier_irqr(framemgr, 0, framemgr_flag);
		return;
	}
	framemgr_x_barrier_irqr(framemgr, 0, framemgr_flag);

	ldr_node = frame->shot_ext->node_group.leader;

	ret = CALL_DEVICE_SENSOR_OPS(sensor, group_tag, sensor, frame, &ldr_node);
	if (ret) {
		merr("is_sensor_group_tag is fail(%d)", group, ret);
		goto p_err;
	}

	mgrdbgs(5, "finish sensor tag(%s)\n", group->device, group, tag_data,
			in_softirq() ? "S" : "H");

p_err:
	return;
}

int is_devicemgr_open(void *device, enum is_device_type type)
{
	int ret = 0;
	u32 stream = 0;
	u32 group_id;
	struct is_core *core;
	struct is_group *group = NULL;
	struct is_device_sensor *sensor;
	struct is_device_ischain *ischain;

	FIMC_BUG(!device);

	switch (type) {
	case IS_DEVICE_SENSOR:
		sensor = (struct is_device_sensor *)device;
		group = &sensor->group_sensor;
		core = sensor->private_data;

		FIMC_BUG(!core);
		FIMC_BUG(!group);
		FIMC_BUG(!sensor->vctx);
		FIMC_BUG(!GET_VIDEO(sensor->vctx));

		/* get the stream id */
		for (stream = 0; stream < IS_STREAM_COUNT; ++stream) {
			ischain = &core->ischain[stream];
			if (!test_bit(IS_ISCHAIN_OPEN, &ischain->state))
				break;
		}

		FIMC_BUG(stream >= IS_STREAM_COUNT);

		/* init group's device information */
		pablo_lib_set_stream_prefix(stream, "%s_P", sensor->sensor_name);
		sensor->stm = group->stm = pablo_lib_get_stream_prefix(stream);

		devicemgr.sensor[stream] = sensor;
		sensor->instance = stream;
		group->instance = stream;
		group->device = ischain;
		group_id = GROUP_ID_SS0 + GET_SSX_ID(GET_VIDEO(sensor->vctx));

		ret = CALL_GROUP_OPS(group, open, group_id, sensor->vctx);
		if (ret) {
			merr("is_group_open is fail(%d)", ischain, ret);
			ret = -EINVAL;
			goto p_err;
		}

		sensor->vctx->next_device = ischain;
		break;
	case IS_DEVICE_ISCHAIN:
		ischain = (struct is_device_ischain *)device;
		stream = ischain->instance;

		ischain->stm = pablo_lib_get_stream_prefix(stream);
		break;
	default:
		err("device type(%d) is invalid", type);
		BUG();
		break;
	}

p_err:
	return ret;
}

int is_devicemgr_binding(struct is_device_sensor *sensor,
		struct is_device_ischain *ischain,
		enum is_device_type type)
{
	int ret = 0;
	struct is_group *group = NULL;
	struct is_group *child_group;

	switch (type) {
	case IS_DEVICE_SENSOR:
		if (test_bit(IS_SENSOR_ONLY, &sensor->state)) {
			/* in case of sensor driving */
			ischain->sensor = sensor;
			sensor->ischain = ischain;

			/* Clear ischain device all state */
			ischain->state = 0;

			/*
			 * Forcely set the ischain's state to "Opened"
			 * Because if it's sensor driving mode, ischain was not opened from HAL.
			 */
			set_bit(IS_ISCHAIN_OPEN, &ischain->state);
			set_bit(ischain->instance, &ischain->ginstance_map);

			ret = is_groupmgr_init(ischain);
			if (ret) {
				merr("is_groupmgr_init is fail(%d)", ischain, ret);
				ret = -EINVAL;
				goto p_err;
			}
		}
		break;
	case IS_DEVICE_ISCHAIN:
		if (sensor &&
			!test_bit(IS_ISCHAIN_REPROCESSING, &ischain->state)) {
			group = &sensor->group_sensor;
			FIMC_BUG(group->instance != ischain->instance);

			child_group = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);
			if (child_group) {
				info("[%d/%d] sensor otf output set\n",
					sensor->device_id, ischain->instance);
				set_bit(IS_SENSOR_OTF_OUTPUT, &sensor->state);
			}
		}
		break;
	default:
		err("device type(%d) is invalid", type);
		BUG();
		break;
	}

p_err:
	return ret;
}

int is_devicemgr_start(void *device, enum is_device_type type)
{
	int ret = 0;
	struct is_group *group;
	struct is_device_sensor *sensor;

	switch (type) {
	case IS_DEVICE_SENSOR:
		sensor = (struct is_device_sensor *)device;
		group = &sensor->group_sensor;

		if (!test_bit(IS_SENSOR_ONLY, &sensor->state) && sensor->ischain) {
			ret = is_ischain_start_wrap(sensor->ischain, group);
			if (ret) {
				merr("is_ischain_start_wrap is fail(%d)", sensor->ischain, ret);
				ret = -EINVAL;
				goto p_err;
			}
		} else {
			ret = is_groupmgr_start(group->device);
			if (ret) {
				merr("is_groupmgr_start is fail(%d)", group->device, ret);
				ret = -EINVAL;
				goto p_err;
			}
		}

		if (IS_ENABLED(CHAIN_TAG_SENSOR_IN_SOFTIRQ_CONTEXT)) {
			struct is_group *child_group
				= GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);

			/* only in case of OTF case, uses tasklet. */
			if (sensor->ischain && child_group)
				tasklet_init(&devicemgr.tasklet[group->instance], do_sensor_tag,
						(unsigned long)&devicemgr.tag_data[group->instance]);
		}
		break;
	case IS_DEVICE_ISCHAIN:
		break;
	default:
		err("device type(%d) is invalid", type);
		BUG();
		break;
	}

p_err:
	return ret;
}

int is_devicemgr_stop(void *device, enum is_device_type type)
{
	int ret = 0;
	struct is_group *group;
	struct is_device_sensor *sensor;

	switch (type) {
	case IS_DEVICE_SENSOR:
		sensor = (struct is_device_sensor *)device;
		group = &sensor->group_sensor;

		if (IS_ENABLED(CHAIN_TAG_SENSOR_IN_SOFTIRQ_CONTEXT)) {
			struct is_group *child_group
				= GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);

			if (sensor->ischain && child_group)
				tasklet_kill(&devicemgr.tasklet[group->instance]);
		}

		if (!test_bit(IS_SENSOR_ONLY, &sensor->state) && sensor->ischain) {
			ret = is_ischain_stop_wrap(sensor->ischain, group);
			if (ret) {
				merr("is_ischain_stop_wrap is fail(%d)", sensor->ischain, ret);
				ret = -EINVAL;
				goto p_err;
			}
		} else {
			is_groupmgr_stop(group->device);
		}
		break;
	case IS_DEVICE_ISCHAIN:
		break;
	default:
		err("device type(%d) is invalid", type);
		BUG();
		break;
	}

p_err:
	return ret;
}

int is_devicemgr_close(void *device, enum is_device_type type)
{
	int ret = 0;
	struct is_group *group;
	struct is_device_sensor *sensor;

	switch (type) {
	case IS_DEVICE_SENSOR:
		sensor = (struct is_device_sensor *)device;

		FIMC_BUG(!sensor);

		group = &sensor->group_sensor;

		ret = CALL_GROUP_OPS(group, close);
		if (ret)
			merr("is_group_close is fail", sensor);

		/*
		 * Forcely set the ischain's state to "Not Opened"
		 * Because if it's sensor driving mode, ischain was not opened from HAL.
		 */
		if (test_bit(IS_SENSOR_ONLY, &sensor->state) && sensor->ischain)
			clear_bit(IS_ISCHAIN_OPEN, &sensor->ischain->state);
		break;
	case IS_DEVICE_ISCHAIN:
		break;
	default:
		err("device type(%d) is invalid", type);
		BUG();
		break;
	}

	return ret;
}

int __nocfi is_devicemgr_shot_callback(struct is_group *group,
		struct is_frame *frame,
		u32 fcount,
		enum is_device_type type)
{
	int ret = 0;
#if !IS_ENABLED(CONFIG_SENSOR_GROUP_LVN)
	unsigned long flags = 0;
#endif
	struct is_group *child_group;
	struct camera2_node ldr_node = {0, };
	struct devicemgr_sensor_tag_data *tag_data;
	u32 stream;
	struct is_device_sensor *sensor;

	switch (type) {
	case IS_DEVICE_SENSOR:
		child_group = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);

		PROGRAM_COUNT(9);

		mgrdbgs(1, " DEVICE SHOT CALLBACK(%d) %s\n", group->device,
			       group, frame, frame->index, child_group ? "OTF" : "M2M");

		/* OTF */
		/* IS_SENSOR_OTF_OUTPUT bit needs to check also.
		 * because this bit can be cleared for not operate child_group_shot at remosaic sensor mode
		 */
		if (child_group && test_bit(IS_SENSOR_OTF_OUTPUT, &group->device->sensor->state)) {
			ret = child_group->shot_callback(child_group->device, child_group, frame);
			if (ret) {
				mgerr("child_group->shot_callback(%d)", child_group, child_group, ret);
				goto p_err;
			}
		/* M2M */
		} else {
			sensor = group->device->sensor;
			ret = CALL_DEVICE_SENSOR_OPS(sensor, group_tag, sensor, frame, &ldr_node);
			if (ret) {
				merr("is_sensor_group_tag is fail(%d)", group, ret);
#if !IS_ENABLED(CONFIG_SENSOR_GROUP_LVN)
				mginfo("[F%d] Start CANCEL Other subdev frame\n", group->device, group, frame->fcount);
				ret = CALL_GROUP_OPS(group, lock, group->device_type, true, &flags);
				if (ret)
					goto p_err;
				CALL_GROUP_OPS(group, subdev_cancel, frame, group->device_type,
					       FS_PROCESS, true);
				CALL_GROUP_OPS(group, subdev_cancel, frame, group->device_type,
					       FS_REQUEST, false);
				CALL_GROUP_OPS(group, unlock, flags, group->device_type, true);
				mginfo("[F%d] End CANCEL Other subdev frame\n", group->device, group, frame->fcount);
#endif
				ret = -EINVAL;
				goto p_err;
			}
		}

		PROGRAM_COUNT(10);

		break;
	case IS_DEVICE_ISCHAIN:
		sensor = group->head->sensor;
		is_sensor_s_otf_out(sensor);

		/*
		 * Sensor Tag Pre-conditions
		 * 1. Current shot is EXTERNAL with single buffer.
		 * 2. Current shot is MULTI,
		 *	but it's the first buffer of batch buffer.
		 */
		if (frame->cur_buf_index > 0
			|| (frame->type != SHOT_TYPE_EXTERNAL
				&& frame->type != SHOT_TYPE_MULTI))
			break;

		stream = sensor->instance;

		tag_data = &devicemgr.tag_data[stream];
		tag_data->fcount = fcount;
		tag_data->devicemgr = &devicemgr;
		tag_data->group = &devicemgr.sensor[stream]->group_sensor;
		tag_data->stream = stream;

		if (IS_ENABLED(CHAIN_TAG_SENSOR_IN_SOFTIRQ_CONTEXT)) {
			mgrdbgs(1, "schedule sensor tag tasklet\n", group->device, group, frame);
			tasklet_schedule(&devicemgr.tasklet[stream]);
		} else { /* (hard)IRQ context */
			do_sensor_tag((unsigned long)tag_data);
		}

		break;
	default:
		mgerr("device type(%d) is invalid", group, group, group->device_type);
		BUG();
		break;
	}

p_err:
	return ret;
}

#if !IS_ENABLED(CONFIG_SENSOR_GROUP_LVN)
int is_devicemgr_shot_done(struct is_group *group,
		struct is_frame *ldr_frame,
		u32 status)
{
	int ret = 0;
	unsigned long flags = 0;

	/* skip in case of the sensor -> 3AA M2M case */
	if (group->device_type == IS_DEVICE_ISCHAIN)
		return ret;

	/*
	 * When error happens, cancel the sensor's subdev frames
	 *  - CONFIG_LOCK_DELAY: Skip cancel because the sensor subdev has already finished its processing.
	 *  - LATE_FRAME: FS_REQUEST frame is handled by 'is_devicemgr_late_shot_handle()'.
	 */
	if (status == IS_SHOT_LATE_FRAME) {
		mginfo("[F%d] Start CANCEL Other subdev frame\n", group->device, group, ldr_frame->fcount);
		ret = CALL_GROUP_OPS(group, lock, group->device_type, false, &flags);
		if (ret)
			return ret;
		CALL_GROUP_OPS(group, subdev_cancel, ldr_frame, group->device_type, FS_PROCESS,
			       true);
		CALL_GROUP_OPS(group, unlock, flags, group->device_type, false);
		mginfo("[F%d] End CANCEL Other subdev frame\n", group->device, group, ldr_frame->fcount);
	}

	return ret;
}

void is_devicemgr_late_shot_handle(struct is_group *group, struct is_frame *frame, u32 status)
{
	unsigned long flags = 0;
	struct is_framemgr *framemgr;
	unsigned long framemgr_flag;
	struct is_frame *ldr_frame;

	info("%s: fcount(%d)", __func__, frame->fcount);

	if (group->device_type == IS_DEVICE_ISCHAIN)
		return;

	framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
	if (!framemgr) {
		merr("framemgr is NULL", group);
		return;
	}

	framemgr_e_barrier_irqs(framemgr, 0, framemgr_flag);
	ldr_frame = find_frame(framemgr, FS_PROCESS, frame_fcount, (void *)(ulong)frame->fcount);

	if (!ldr_frame) {
		frame_manager_print_queues(framemgr);
		merr("[F%d] There's no frame in processing." \
				"Can't sync sensor and ischain buffer anymore..",
				group, frame->fcount);
		framemgr_x_barrier_irqr(framemgr, 0, framemgr_flag);
		return;
	}
	framemgr_x_barrier_irqr(framemgr, 0, framemgr_flag);

	/* if error happened, cancel the sensor's subdev frames */
	if (status) {
		mginfo("[F%d] Start CANCEL Other subdev frame\n", group->device, group, ldr_frame->fcount);
		if (CALL_GROUP_OPS(group, lock, group->device_type, false, &flags))
			return;
		CALL_GROUP_OPS(group, subdev_cancel, ldr_frame, group->device_type, FS_REQUEST,
			       false);
		CALL_GROUP_OPS(group, unlock, flags, group->device_type, false);
		mginfo("[F%d] End CANCEL Other subdev frame\n", group->device, group, ldr_frame->fcount);
	}

	return;
}
#endif

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
static struct pablo_kunit_devicemgr_ops pablo_devicemgr_ops = {
	.sensor_tag = do_sensor_tag,
	.shot_callback = is_devicemgr_shot_callback,
#if !IS_ENABLED(CONFIG_SENSOR_GROUP_LVN)
	.shot_done = is_devicemgr_shot_done,
	.late_shot_handle = is_devicemgr_late_shot_handle,
#endif
};

struct pablo_kunit_devicemgr_ops *pablo_kunit_get_devicemgr(void)
{
	return &pablo_devicemgr_ops;
}
KUNIT_EXPORT_SYMBOL(pablo_kunit_get_devicemgr);
#endif
