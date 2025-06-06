/*
 * drivers/media/video/exynos/fimc-is-mc2/interface/fimc-is-interface-ishcain.c
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *       http://www.samsung.com
 *
 * The header file related to camera
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/workqueue.h>
#include <linux/bug.h>
#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>

#include "exynos-is.h"
#include "is-core.h"
#include "is-err.h"
#include "is-groupmgr.h"

#include "is-interface.h"
#include "pablo-debug.h"
#include "pablo-work.h"

#include "pablo-interface-irta.h"
#include "pablo-icpu-adapter.h"
#include "is-interface-ischain.h"
#include "is-interface-library.h"
#include "../include/is-hw.h"
#include "pablo-irq.h"

static struct pablo_interface_ops itf_ops;
static void is_hw_free_irq(struct is_interface_hwip *itf_hwip)
{
	int irq_idx;

	for (irq_idx = 0; irq_idx < INTR_HWIP_MAX; irq_idx++) {
		if (itf_hwip->irq[irq_idx] > 0) {
			pablo_free_irq(itf_hwip->irq[irq_idx], itf_hwip);
			itf_hwip->handler[irq_idx].valid = false;
		}
	}
}

static int is_interface_hw_ip_probe(struct is_interface_ischain *itfc, int hw_id,
	struct platform_device *pdev, struct is_hardware *hardware)
{
	struct is_interface_hwip *itf;
	int ret;
	int hw_slot;
	struct is_hw_ip *hw_ip;

	hw_slot = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_slot_id, hw_id);
	if (!valid_hw_slot_id(hw_slot)) {
		err_itfc("invalid hw_slot (%d) ", hw_slot);
		return -EINVAL;
	}

	hw_ip = &(hardware->hw_ip[hw_slot]);
	itfc->itf_ip[hw_slot].hw_ip = hw_ip;

	itf = &itfc->itf_ip[hw_slot];
	itf->id = hw_id;

	ret = is_hw_get_address(itf, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_address failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = is_hw_get_irq(itf, pdev, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_get_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	ret = is_hw_request_irq(itf, hw_id);
	if (ret) {
		err_itfc("[ID:%2d] hw_request_irq failed (%d)", hw_id, ret);
		return -EINVAL;
	}

	dbg_itfc("[ID:%2d] probe done\n", hw_id);

	return ret;
}

static void is_interface_hw_ip_remove(struct is_interface_ischain *itfc, int hw_id,
	struct is_hardware *hardware)
{
	int hw_slot = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_slot_id, hw_id);
	struct is_interface_hwip *itf = &itfc->itf_ip[hw_slot];

	is_hw_free_irq(itf);

	dbg_itfc("[ID:%2d] remove done\n", hw_id);
}

int is_interface_ischain_probe(struct is_interface_ischain *this,
	struct is_hardware *hardware, struct platform_device *pdev)
{
	int ret, i;
	const int *hw_id = is_hw_get_ioresource_to_hw_id();

	this->minfo = is_get_is_minfo();

	is_interface_set_lib_support(this, pdev);

	for (i = 0; i < IORESOURCE_MAX; i++) {
		if (hw_id[i] >= DEV_HW_END)
			continue;

		ret = is_interface_hw_ip_probe(this, hw_id[i], pdev, hardware);
		if (ret) {
			err_itfc("interface probe fail (hw_id: %d)", hw_id[i]);
			return -EINVAL;
		}
	}

	dbg_itfc("interface ishchain probe done (hw_id: %d)", hw_id[i - 1]);

	return 0;
}

void is_interface_ischain_remove(struct is_interface_ischain *this,
	struct is_hardware *hardware)
{
	int i;
	const int *hw_id = is_hw_get_ioresource_to_hw_id();

	for (i = 0; i < IORESOURCE_MAX; i++)
		is_interface_hw_ip_remove(this, hw_id[i], hardware);
}

static void wq_func_group_xxx(struct is_group *group,
	struct is_framemgr *framemgr,
	struct is_frame *frame,
	struct is_video_ctx *vctx,
	u32 status)
{
	u32 done_state = VB2_BUF_STATE_DONE;

	FIMC_BUG_VOID(!vctx);
	FIMC_BUG_VOID(!framemgr);
	FIMC_BUG_VOID(!frame);

	/* Collecting sensor group NDONE status */
	if (test_bit(IS_GROUP_OTF_INPUT, &group->state))
		status = status ? status : frame->result;

	if (status) {
		mgrinfo("[ERR] NDONE(%d, E%d)\n", group, group, frame, frame->index, status);
		done_state = VB2_BUF_STATE_ERROR;
		frame->result = status;

		if (status == IS_SHOT_OVERFLOW) {
			if (IS_ENABLED(CONFIG_PANIC_ON_COTF_ERR))
				panic("G%d overflow", group->id);
			else
				err("G%d overflow", group->id);
		}
	} else {
		mgrdbgs(1, " DONE(%d)\n", group, group, frame, frame->index);
	}

	frame->stripe_info.region_id++;
	if (frame->stripe_info.region_num) {
		if (frame->stripe_info.region_id == frame->stripe_info.region_num) {
			frame->repeat_info.idx++;
			frame->stripe_info.region_id = 0;
		}
	} else {
		frame->repeat_info.idx++;
	}
	mgrdbgs(5, "%s : repeat num %d, repeat idx %d, repeat scn %d\n",
		group, group, frame, __func__, frame->repeat_info.num,
		frame->repeat_info.idx, frame->repeat_info.scenario);
	mgrdbgs(5, "%s : strip num %d, strip idx %d\n",	group, group, frame, __func__,
		frame->stripe_info.region_num, frame->stripe_info.region_id);
	clear_bit(group->leader.id, &frame->out_flag);
	CALL_GROUP_OPS(group, done, frame, done_state);

	/**
	 * Skip done when current frame is doing stripe process,
	 * and re-trigger the group shot for next stripe process.
	 */
	if (!status && frame->state == FS_REPEAT_PROCESS)
		return;

	frame->stripe_info.region_id = 0;
	frame->stripe_info.region_num  = 0;
	frame->repeat_info.idx = 0;
	frame->repeat_info.num = 0;

	trans_frame(framemgr, frame, FS_COMPLETE);
	CALL_VOPS(vctx, done, frame->index, done_state);
}

static int wq_func_group(struct is_device_ischain *device, struct is_group *group,
	struct is_framemgr *framemgr, struct is_frame *frame, struct is_video_ctx *vctx, u32 status,
	u32 fcount)
{
	FIMC_BUG(!group);
	FIMC_BUG(!framemgr);
	FIMC_BUG(!vctx);

	TIME_SHOT(TMS_SDONE);

	if (!frame) {
		err("frame is NULL");
		return -EINVAL;
	}

	/*
	 * complete count should be lower than 3 when
	 * buffer is queued or overflow can be occurred
	 */
	if (framemgr->queued_count[FS_COMPLETE] >= DIV_ROUND_UP(framemgr->num_frames, 2))
		mgwarn(" complete bufs : %d", device, group, (framemgr->queued_count[FS_COMPLETE] + 1));

	if (unlikely(fcount != frame->fcount)) {
		if (test_bit(IS_GROUP_OTF_INPUT, &group->state)) {
			while (frame) {
				if (fcount == frame->fcount) {
					wq_func_group_xxx(group, framemgr,
							frame, vctx, status);
					break;
				} else if (fcount > frame->fcount) {
					wq_func_group_xxx(group, framemgr,
							frame, vctx, SHOT_ERR_MISMATCH);

					/* get next leader frame */
					frame = peek_frame(framemgr, FS_PROCESS);
				} else {
					warn("%d shot done is ignored", fcount);
					return -EPERM;
				}
			}
		} else {
			wq_func_group_xxx(group, framemgr,
					frame, vctx, SHOT_ERR_MISMATCH);
		}
	} else {
		wq_func_group_xxx(group, framemgr, frame, vctx, status);
	}

	frame = peek_frame(framemgr, FS_PROCESS);
	while (frame) {
		if (frame->result != SHOT_ERR_CANCEL)
			break;

		wq_func_group_xxx(group, framemgr, frame, vctx, frame->result);
		frame = peek_frame(framemgr, FS_PROCESS);
	}

	return 0;
}

static void wq_func_shot(struct work_struct *data)
{
	struct is_device_ischain *device;
	struct is_interface *itf;
	struct is_msg *msg;
	struct is_framemgr *framemgr;
	struct is_frame *frame;
	struct is_group *group;
	struct is_group *head;
	struct is_work_list *work_list;
	struct is_work *work;
	struct is_video_ctx *vctx;
	ulong flags;
	u32 fcount, status;
	int instance;
	u64 group_slot;
	struct is_core *core;

	FIMC_BUG_VOID(!data);

	itf = container_of(data, struct is_interface, work_wq[WORK_SHOT_DONE]);
	work_list = &itf->work_list[WORK_SHOT_DONE];

	get_req_work(work_list, &work);
	while (work) {
		core = (struct is_core *)itf->core;
		instance = work->msg.instance;
		group_slot = work->msg.group;
		device = &((struct is_core *)itf->core)->ischain[instance];

		msg = &work->msg;
		fcount = msg->param1;
		status = msg->param2;

		if (group_slot < GROUP_SLOT_MAX) {
			group = device->group[group_slot];
		} else {
			merr("unresolved group_slot (%llu)", device, group_slot);
			goto remain;
		}

		head = group->head;
		if (!head) {
			merr("head is NULL", device);
			goto remain;
		}

		vctx = head->leader.vctx;
		if (!vctx) {
			merr("vctx is NULL", device);
			goto remain;
		}

		framemgr = GET_FRAMEMGR(vctx);
		if (!framemgr) {
			merr("framemgr is NULL", device);
			goto remain;
		}

		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_5, flags);

		frame = find_frame(framemgr, FS_REPEAT_PROCESS, frame_fcount, (void *)(ulong)fcount);
		if (!frame)
			frame = peek_frame(framemgr, FS_PROCESS);

		if (!wq_func_group(device, head, framemgr, frame, vctx, status, fcount)) {
#ifdef MEASURE_TIME
#ifdef EXTERNAL_TIME
			ktime_get_ts64(&frame->tzone[TM_SHOT_D]);
#endif
#endif
			/* clear bit for child group */
			if (group != head)
				clear_bit(group->leader.id, &frame->out_flag);
		} else {
			mgerr("invalid shot done(%d)", device, head, fcount);
			frame_manager_print_queues(framemgr);
		}

		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_5, flags);

remain:
		set_free_work(work_list, work);
		get_req_work(work_list, &work);
	}
}

static inline void print_framemgr_spinlock_usage(struct is_core *core)
{
	u32 i, j;
	struct is_device_ischain *ischain;
	struct is_device_sensor *sensor;
	struct is_framemgr *framemgr;
	struct is_subdev *subdev;

	for (i = 0; i < IS_SENSOR_COUNT; ++i) {
		sensor = &core->sensor[i];
		if (test_bit(IS_SENSOR_OPEN, &sensor->state)) {
			framemgr = sensor->vctx->queue ? &sensor->vctx->queue->framemgr : NULL;
			if (framemgr)
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);
		}
	}

	for (i = 0; i < IS_STREAM_COUNT; ++i) {
		ischain = &core->ischain[i];
		if (!test_bit(IS_ISCHAIN_OPEN, &ischain->state))
			continue;

		for (j = 0; j < GROUP_SLOT_MAX; j++) {
			if (!ischain->group[j])
				continue;

			subdev = &ischain->group[j]->leader;
			framemgr = GET_SUBDEV_FRAMEMGR(subdev);
			if (test_bit(IS_SUBDEV_OPEN, &subdev->state) && framemgr)
				info("[@] framemgr(%s) sindex : 0x%08lX\n", framemgr->name, framemgr->sindex);
		}
	}
}

IS_TIMER_FUNC(interface_timer)
{
	u32 shot_count, scount[GROUP_SLOT_MAX];
	u32 fcount, i;
	unsigned long flags;
	struct is_interface *itf = from_timer(itf, (struct timer_list *)data, timer);
	struct is_core *core;
	struct is_device_ischain *device;
	struct is_device_sensor *sensor;
	struct is_group *group;
	struct is_framemgr *framemgr;
	struct is_work_list *work_list;

	FIMC_BUG_VOID(!itf->core);

	if (!test_bit(IS_IF_STATE_OPEN, &itf->state)) {
		is_info("shot timer is terminated\n");
		return;
	}

	core = itf->core;

	for (i = 0; i < IS_STREAM_COUNT; ++i) {
		device = &core->ischain[i];
		shot_count = 0;
		memset(scount, 0x00, sizeof(scount));

		sensor = device->sensor;
		if (!sensor)
			continue;

		if (!test_bit(IS_SENSOR_FRONT_START, &sensor->state))
			continue;

		if (test_bit(IS_ISCHAIN_OPEN_STREAM, &device->state)) {
			spin_lock_irqsave(&itf->shot_check_lock, flags);
			if (atomic_read(&itf->shot_check[i])) {
				atomic_set(&itf->shot_check[i], 0);
				atomic_set(&itf->shot_timeout[i], 0);
				spin_unlock_irqrestore(&itf->shot_check_lock, flags);
				continue;
			}
			spin_unlock_irqrestore(&itf->shot_check_lock, flags);

			group = get_leader_group(i);
				while (group) {
				if (test_bit(IS_GROUP_START, &group->state)) {
					framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
					if (framemgr) {
						framemgr_e_barrier_irqs(framemgr, FMGR_IDX_6, flags);
						scount[group->slot] = framemgr->queued_count[FS_PROCESS];
						shot_count += scount[group->slot];
						framemgr_x_barrier_irqr(framemgr, FMGR_IDX_6, flags);
					} else {
						minfo("\n### group_slot%d framemgr is null ###\n", device, group->slot);
					}
				}
				group = group->gnext;
			};
		}

		if (shot_count) {
			atomic_inc(&itf->shot_timeout[i]);
			minfo("shot timer[%d] is increased to %d\n", device,
				i, atomic_read(&itf->shot_timeout[i]));
		}

		if (atomic_read(&itf->shot_timeout[i]) > TRY_TIMEOUT_COUNT) {
			merr("shot command is timeout(%d, %d)", device,
				atomic_read(&itf->shot_timeout[i]), shot_count);

			group = get_leader_group(i);
			while (group) {
				minfo("\n### group_slot%d framemgr info ###\n", device, group->slot);
				if (scount[group->slot]) {
					framemgr = GET_HEAD_GROUP_FRAMEMGR(group);
					if (framemgr) {
						framemgr_e_barrier_irqs(framemgr, 0, flags);
						frame_manager_print_queues(framemgr);
						framemgr_x_barrier_irqr(framemgr, 0, flags);
					} else {
						minfo("\n### group_slot%d framemgr is null ###\n", device, group->slot);
					}
				}
				group = group->gnext;
			}

			minfo("\n### work list info ###\n", device);
			work_list = &itf->work_list[WORK_SHOT_DONE];
			print_fre_work_list(work_list);
			print_req_work_list(work_list);

			/* framemgr spinlock check */
			print_framemgr_spinlock_usage(core);

			merr("[@] camera firmware panic!!!", device);
			is_debug_s2d(true, "IS_SHOT_CMD_TIMEOUT");

			return;
		}
	}

	for (i = 0; i < IS_SENSOR_COUNT; ++i) {
		sensor = &core->sensor[i];

		if (!test_bit(IS_SENSOR_BACK_START, &sensor->state)
			|| !test_bit(IS_SENSOR_FRONT_START, &sensor->state)) {
			atomic_set(&itf->sensor_check[i], 0);
			continue;
		}

		fcount = is_sensor_g_fcount(sensor);
		if (fcount == atomic_read(&itf->sensor_check[i])) {
			atomic_inc(&itf->sensor_timeout[i]);
			minfo("sensor timer[%d] is increased to %d(fcount : %d)\n", sensor, i,
				atomic_read(&itf->sensor_timeout[i]), fcount);
		} else {
			atomic_set(&itf->sensor_timeout[i], 0);
			atomic_set(&itf->sensor_check[i], fcount);
		}

#ifdef ESD_RECOVERY_ON_SENSOR_TIMEOUT
		if (atomic_read(&itf->sensor_timeout[i]) == 1) {
			set_bit(IS_SENSOR_ESD_RECOVERY, &sensor->state);
		}
#endif

		if (atomic_read(&itf->sensor_timeout[i]) > SENSOR_TIMEOUT_COUNT) {
			merr("sensor is timeout(%d, %d)", sensor,
				atomic_read(&itf->sensor_timeout[i]),
				atomic_read(&itf->sensor_check[i]));
			is_sensor_dump(sensor);

			/* framemgr spinlock check */
			print_framemgr_spinlock_usage(core);

#ifdef SENSOR_PANIC_ENABLE
			/* if panic happened, fw log dump should be happened by panic handler */
			mdelay(2000);
			panic("[@] camera sensor panic!!!");
#else
			is_resource_dump();
#endif
			return;
		}
	}

	mod_timer(&itf->timer, jiffies + (IS_COMMAND_TIMEOUT/TRY_TIMEOUT_COUNT));
}

int is_interface_probe(struct is_interface *this,
	void *core_data)
{
	int ret;

	is_load_ctrl_init();
	dbg_interface(1, "%s\n", __func__);

	spin_lock_init(&this->shot_check_lock);

	this->workqueue = alloc_workqueue("is/[H/U]", WQ_HIGHPRI | WQ_UNBOUND, 0);
	if (!this->workqueue)
		probe_warn("failed to alloc own workqueue, will be use global one");

	INIT_WORK(&this->work_wq[WORK_SHOT_DONE], wq_func_shot);
	init_work_list(&this->work_list[WORK_SHOT_DONE], WORK_SHOT_DONE, MAX_WORK_COUNT);

	this->core = core_data;
	this->ops = &itf_ops;

	clear_bit(IS_IF_STATE_OPEN, &this->state);
	clear_bit(IS_IF_STATE_START, &this->state);
	clear_bit(IS_IF_STATE_BUSY, &this->state);
	clear_bit(IS_IF_STATE_READY, &this->state);
	clear_bit(IS_IF_STATE_LOGGING, &this->state);

	ret = pablo_interface_irta_probe();
	if (ret) {
		probe_err("pablo_interface_irta_probe is fail(%d)", ret);
		return ret;
	}

	pablo_icpu_adt_probe();

	return ret;
}

void is_interface_remove(struct is_interface *this)
{
	pablo_icpu_adt_remove();
	pablo_interface_irta_remove();
	destroy_workqueue(this->workqueue);
}

static int is_interface_open(struct is_interface *this)
{
	int i;
	int ret = 0;

	if (test_bit(IS_IF_STATE_OPEN, &this->state)) {
		err("already open");
		ret = -EMFILE;
		goto exit;
	}

	dbg_interface(1, "%s\n", __func__);

	for (i = 0; i < IS_STREAM_COUNT; i++) {
		atomic_set(&this->shot_check[i], 0);
		atomic_set(&this->shot_timeout[i], 0);
	}
	for (i = 0; i < IS_SENSOR_COUNT; i++) {
		atomic_set(&this->sensor_check[i], 0);
		atomic_set(&this->sensor_timeout[i], 0);
	}

	clear_bit(IS_IF_STATE_START, &this->state);
	clear_bit(IS_IF_STATE_BUSY, &this->state);
	clear_bit(IS_IF_STATE_READY, &this->state);
	clear_bit(IS_IF_STATE_LOGGING, &this->state);

	timer_setup(&this->timer, (void (*)(struct timer_list *))interface_timer, 0);
	this->timer.expires = jiffies + (IS_COMMAND_TIMEOUT/TRY_TIMEOUT_COUNT);
	add_timer(&this->timer);

	set_bit(IS_IF_STATE_OPEN, &this->state);

exit:
	return ret;
}

static int is_interface_close(struct is_interface *this)
{
	int ret = 0;

	if (!test_bit(IS_IF_STATE_OPEN, &this->state)) {
		err("already close");
		ret = -EMFILE;
		goto exit;
	}

	del_timer_sync(&this->timer);

	dbg_interface(1, "%s\n", __func__);

	clear_bit(IS_IF_STATE_OPEN, &this->state);

exit:
	return ret;
}

static struct pablo_interface_ops itf_ops = {
	.open = is_interface_open,
	.close = is_interface_close,
};
