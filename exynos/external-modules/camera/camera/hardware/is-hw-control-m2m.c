// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo Image Subsystem functions
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-hw-control.h"
#include "is-core.h"
#include "pablo-obte.h"

static void is_hardware_frame_start(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_frame *frame, *check_frame;
	struct is_framemgr *framemgr = hw_ip->framemgr;

	framemgr_e_barrier(framemgr, 0);
	frame = peek_frame(framemgr, FS_HW_CONFIGURE);

	if (!frame) {
		check_frame = find_frame(framemgr, FS_HW_WAIT_DONE,
			frame_fcount, (void *)(ulong)atomic_read(&hw_ip->fcount));
		if (check_frame) {
			msdbgs_hw(2, "[F:%d] already processed to HW_WAIT_DONE state",
					instance, hw_ip, check_frame->fcount);

			framemgr_x_barrier(framemgr, 0);
		} else {
			/* error happened..print the frame info */
			frame_manager_print_info_queues(framemgr);
			print_all_hw_frame_count(hw_ip->hardware);
			framemgr_x_barrier(framemgr, 0);
			mserr_hw("FSTART frame null", instance, hw_ip);
		}
		atomic_set(&hw_ip->status.Vvalid, V_VALID);
		return;
	}

	frame->frame_info[INFO_FRAME_START].cpu = raw_smp_processor_id();
	frame->frame_info[INFO_FRAME_START].pid = current->pid;
	frame->frame_info[INFO_FRAME_START].when = local_clock();

	trans_frame(framemgr, frame, FS_HW_WAIT_DONE);
	framemgr_x_barrier(framemgr, 0);
}

static int is_hardware_frame_done(struct is_hw_ip *hw_ip, struct is_frame *frame,
	int wq_id, u32 output_id, enum ShotErrorType done_type, bool get_meta)
{
	struct is_framemgr *framemgr = hw_ip->framemgr;
	u32 hw_fe_cnt = atomic_read(&hw_ip->fcount);
	ulong flags = 0;
	struct is_hw_ip *ldr_hw_ip;
	u32 instance;
	bool last_end;

	switch (done_type) {
	case IS_SHOT_SUCCESS:
		if (!frame) {
			framemgr_e_barrier_common(framemgr, 0, flags);
			frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
			framemgr_x_barrier_common(framemgr, 0, flags);
		} else {
			sdbg_hw(2, "frame NOT null!!(%d)", hw_ip, done_type);
		}

		if (IS_RUNNING_TUNING_SYSTEM() && frame)
			pablo_obte_regdump(frame->instance, hw_ip->id,
					frame->stripe_info.region_id,
					frame->stripe_info.region_num);
		break;
	case IS_SHOT_UNPROCESSED:
	case IS_SHOT_TIMEOUT:
		break;
	default:
		serr_hw("invalid done_type(%d)", hw_ip, done_type);
		return -EINVAL;
	}

	if (!frame) {
		serr_hw("[F:%d]frame_done: frame(null)!!(%d)(0x%x)", hw_ip,
			hw_fe_cnt, done_type, output_id);
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame_manager_print_info_queues(framemgr);
		print_all_hw_frame_count(hw_ip->hardware);
		framemgr_x_barrier_common(framemgr, 0, flags);
		return -EINVAL;
	}

	instance = frame->instance;

	msdbgs_hw(2, "[0x%x]frame_done [F:%d][HWF:%d][C:0x%lx]\n",
		instance, hw_ip, output_id, frame->fcount, hw_fe_cnt, frame->core_flag);

	if (frame->shot)
		CALL_HWIP_OPS(hw_ip, get_meta, frame, hw_ip->hardware->hw_map[instance]);

	if (atomic_read(&hw_ip->status.Vvalid) == V_BLANK)
		msdbg_hw(1, "[F%d][HF%d] already in VBlank", frame->instance, hw_ip,
				frame->fcount, hw_fe_cnt);
	else
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);

	framemgr_e_barrier_common(framemgr, 0, flags);
	clear_bit(hw_ip->id, &frame->core_flag);
	last_end = !frame->core_flag;
	framemgr_x_barrier_common(framemgr, 0, flags);

	if (!last_end)
		return 0;

	frame->frame_info[INFO_FRAME_END_PROC].cpu = raw_smp_processor_id();
	frame->frame_info[INFO_FRAME_END_PROC].pid = current->pid;
	frame->frame_info[INFO_FRAME_END_PROC].when = local_clock();

	if (frame->ldr_hw_slot_id < HW_SLOT_MAX) {
		ldr_hw_ip = &hw_ip->hardware->hw_ip[frame->ldr_hw_slot_id];

		del_timer(&ldr_hw_ip->shot_timer);

		framemgr_e_barrier_common(framemgr, 0, flags);
		trans_frame(framemgr, frame, FS_HW_FREE);
		framemgr_x_barrier_common(framemgr, 0, flags);

		if (frame->type == SHOT_TYPE_MULTI) {
			schedule_work(&ldr_hw_ip->wq_shot);
			return 0;
		}
		if (M2M_ASYNC_SHOTS > 1)
			schedule_work(&ldr_hw_ip->wq_shot);
	}

	return is_hardware_shot_done(hw_ip, frame, framemgr, done_type);
}

static const struct is_hardware_ops is_hardware_m2m_ops = {
	.frame_start	= is_hardware_frame_start,
	.frame_done	= is_hardware_frame_done,
	.frame_ndone	= is_hardware_frame_ndone,
	.flush_frame	= is_hardware_flush_frame,
	.dbg_trace	= _is_hw_frame_dbg_trace,
	.dma_cfg	= is_hardware_dma_cfg,
	.dma_cfg_kva64	= is_hardware_dma_cfg_kva64,
};

static inline int _is_hardware_shot(struct is_hardware *hardware, u32 instance,
	struct is_hw_ip *hw_ip, struct is_frame *frame, struct is_framemgr *framemgr)
{
	int ret = 0;
	struct is_group *group = hw_ip->group[instance];
	int i, j;

	is_set_hw_count(hardware, frame->hw_slot_id, instance, frame->fcount);
	mod_timer(&hw_ip->shot_timer,
		jiffies + msecs_to_jiffies(is_get_shot_timeout(&is_get_is_core()->resourcemgr)));

	if (group && CALL_HW_CHAIN_INFO_OPS(hardware, check_crta_hw, hw_ip->id))
		return is_hardware_shot_prepare(hardware, group, frame);

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (frame->hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[frame->hw_slot_id[i]];

		hw_ip->framemgr = framemgr;

		carve_hw_shot_info(hw_ip, frame, instance);

		if (test_and_clear_bit(HW_OTF, &hw_ip->state))
			msinfo_hw("[F%d] Set M2M mode", instance, hw_ip, frame->fcount);

		if (hardware->fake_m2m_frame_start)
			hardware->fake_m2m_frame_start(hw_ip, instance);
		else
			ret = CALL_HWIP_OPS(hw_ip, shot, frame, hardware->hw_map[instance]);

		_is_hw_frame_dbg_trace(hw_ip, frame->fcount, DEBUG_POINT_HW_SHOT_X);
		if (ret) {
			mserr_hw("shot fail [F:%d]", instance, hw_ip, frame->fcount);
			goto shot_err_cancel;
		}
	}

	if (hardware->fake_m2m_frame_end) {
		for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
			if (frame->hw_slot_id[i] >= HW_SLOT_MAX)
				continue;

			hw_ip = &hardware->hw_ip[frame->hw_slot_id[i]];

			hardware->fake_m2m_frame_end(hw_ip, instance);
		}
	}

	return ret;

shot_err_cancel:
	mswarn_hw("[F:%d] Canceled by hardware shot err", instance, hw_ip, frame->fcount);

	for (j = HW_SLOT_MAX - 1; j > i; j--) {
		if (frame->hw_slot_id[j] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[frame->hw_slot_id[j]];

		if (CALL_HWIP_OPS(hw_ip, restore, instance))
			mserr_hw("reset & restore fail", instance, hw_ip);
	}

	return ret;
}

static inline int is_hw_m2m_grp_set_hw_ops(struct is_hardware *hardware, struct is_frame *frame)
{
	int i;
	struct is_hw_ip *hw_ip;

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (frame->hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[frame->hw_slot_id[i]];
		hw_ip->hw_ops = &is_hardware_m2m_ops;
	}

	return 0;
}

static int is_hw_m2m_grp_shot(struct is_hardware *hardware, u32 instance, struct is_frame *frame)
{
	int ret;
	struct is_hw_ip *hw_ip;
	struct is_frame *hw_frame;
	struct is_framemgr *framemgr;
	ulong flags = 0;
	int num_buffers;
	bool reset;
	int i;

	FIMC_BUG(!hardware);
	FIMC_BUG(!frame);
	FIMC_BUG(instance >= IS_STREAM_COUNT);

	if (frame->hw_slot_id[0] >= HW_SLOT_MAX) {
		merr_hw("invalid hw_slot_id(%d)", instance, frame->hw_slot_id[0]);
		return -EINVAL;
	}

	is_hw_m2m_grp_set_hw_ops(hardware, frame);

	hw_ip = &hardware->hw_ip[frame->hw_slot_id[0]];

	if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
		msinfo_hw("grp_shot [F:%d][dva:%pad]\n",
			instance, hw_ip, frame->fcount, &frame->dvaddr_buffer[0]);

	hw_ip->framemgr = &hardware->framemgr[hw_ip->id];
	framemgr = hw_ip->framemgr;

	framemgr_e_barrier_irqs(framemgr, 0, flags);

	hw_frame = get_frame(framemgr, FS_HW_FREE);
	if (hw_frame == NULL) {
		framemgr_x_barrier_irqr(framemgr, 0, flags);
		mserr_hw("free_head(NULL)", instance, hw_ip);
		return -EINVAL;
	}

	num_buffers = frame->num_buffers;
	reset = (num_buffers > 1) ? 0 : 1;
	is_hardware_fill_frame_info(instance, hw_frame, frame, hardware, reset);
	frame->type = SHOT_TYPE_EXTERNAL;
	hw_frame->type = frame->type;

	/* multi-buffer */
	hw_frame->planes	= frame->planes;
	hw_frame->num_buffers	= num_buffers;
	hw_frame->cur_buf_index	= 0;
	framemgr->batch_num = num_buffers;

	put_frame(framemgr, hw_frame, FS_HW_REQUEST);

	if (num_buffers > 1) {
		if (SUPPORT_HW_FRO(hw_ip->id)) {
			hw_ip->hw_fro_en = true;
		} else {
			hw_ip->hw_fro_en = false;
			hw_frame->type = SHOT_TYPE_MULTI;
			hw_frame->planes = 1;
			hw_frame->num_buffers = 1;
			hw_frame->cur_buf_index = 0;

			for (i = 1; i < num_buffers; i++) {
				hw_frame = get_frame(framemgr, FS_HW_FREE);
				if (hw_frame == NULL) {
					framemgr_x_barrier_irqr(framemgr, 0, flags);
					err_hw("[F%d]free_head(NULL)", frame->fcount);
					return -EINVAL;
				}

				reset = (i < (num_buffers - 1)) ? 0 : 1;
				is_hardware_fill_frame_info(instance, hw_frame, frame,
								hardware, reset);
				hw_frame->type = SHOT_TYPE_MULTI;
				hw_frame->planes = 1;
				hw_frame->num_buffers = 1;
				hw_frame->cur_buf_index = i;

				put_frame(framemgr, hw_frame, FS_HW_REQUEST);
			}
			hw_frame->type = frame->type; /* last buffer */
		}
	} else {
		hw_ip->hw_fro_en = false;
	}

	framemgr_x_barrier_irqr(framemgr, 0, flags);

	msdbg_hw(2, "ischain batch_num(%d), HW FRO(%d)\n", instance, hw_ip,
		num_buffers, hw_ip->hw_fro_en);

	if (M2M_ASYNC_SHOTS > 1) {
		schedule_work(&hw_ip->wq_shot);
		return 0;
	} else {
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame = peek_frame(framemgr, FS_HW_REQUEST);
		if (!frame) {
			framemgr_x_barrier_common(framemgr, 0, flags);
			merr_hw("request_head(NULL)", instance);
			return -EINVAL;
		}

		trans_frame(framemgr, frame, FS_HW_CONFIGURE);
		framemgr_x_barrier_common(framemgr, 0, flags);
		ret = _is_hardware_shot(hardware, instance, hw_ip, frame, framemgr);
		if (ret) {
			framemgr_e_barrier_common(framemgr, 0, flags);
			trans_frame(framemgr, frame, FS_HW_FREE);
			framemgr_x_barrier_common(framemgr, 0, flags);
			mserr_hw("hardware_shot fail", instance, hw_ip);
		}
		return ret;
	}
}

static void is_hw_m2m_shot_work_fn(struct work_struct *data)
{
	u32 instance;
	int ret = 0;
	struct is_hw_ip *hw_ip;
	struct is_hardware *hardware;
	struct is_framemgr *framemgr;
	struct is_frame *frame;
	ulong flags = 0;

	hw_ip = container_of(data, struct is_hw_ip, wq_shot);

	hardware = hw_ip->hardware;
	framemgr = hw_ip->framemgr;

	framemgr_e_barrier_common(framemgr, 0, flags);
	if (framemgr->queued_count[FS_HW_CONFIGURE] || framemgr->queued_count[FS_HW_WAIT_DONE]) {
		framemgr_x_barrier_common(framemgr, 0, flags);
		return;
	}

	frame = peek_frame(framemgr, FS_HW_REQUEST);
	if (!frame) {
		framemgr_x_barrier_common(framemgr, 0, flags);
		return;
	}
	trans_frame(framemgr, frame, FS_HW_CONFIGURE);
	framemgr_x_barrier_common(framemgr, 0, flags);

	instance = frame->instance;

	msdbgs_hw(2, "%s\n", instance, hw_ip, __func__);

	ret = _is_hardware_shot(hardware, instance, hw_ip, frame, framemgr);
	if (ret) {
		/* TODO: It should call "is_hardware_frame_ndone" for error handling of nested shot */
		framemgr_e_barrier_common(framemgr, 0, flags);
		trans_frame(framemgr, frame, FS_HW_FREE);
		framemgr_x_barrier_common(framemgr, 0, flags);
		mserr_hw("hardware_shot fail", instance, hw_ip);
	}
}

static int is_hw_m2m_grp_open(struct is_hardware *hardware, u32 instance, u32 *hw_slot_id)
{
	int i;
	struct is_hw_ip *hw_ip;

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[hw_slot_id[i]];

		/* Per instance */

		if (atomic_read(&hw_ip->rsccount))
			continue;

		/* initial setting */
		hw_ip->hw_ops = &is_hardware_m2m_ops;

		if (!i) /* leader IP */
			INIT_WORK(&hw_ip->wq_shot, is_hw_m2m_shot_work_fn);
	}

	return 0;
}

static int is_hw_m2m_grp_close(struct is_hardware *hardware, u32 instance, u32 *hw_slot_id)
{
	return 0;
}

static void is_hw_m2m_grp_wait_frame(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_framemgr *framemgr = hw_ip->framemgr;
	struct is_frame *frame;
	int retry;
	ulong flags = 0;
	u32 state;

	if (!framemgr) {
		mswarn_hw("framemgr is NULL", instance, hw_ip);
		return;
	}

	/*
	 * If SW batch is used, all buffers in one batch shot must be retuend at once.
	 * So, all frames that are in FS_HW_REQUEST in one batch shot
	 * must be waited untill done.
	 * that's because all frames in one batch shot is changed to FS_HW_REQUEST
	 * at is_hardware_grp_shot() function,
	 */
	if (hw_ip->hw_fro_en == false && framemgr->batch_num > 1)
		state = FS_HW_REQUEST;
	else
		state = FS_HW_CONFIGURE;

	for (; state <= FS_HW_WAIT_DONE; state++) {
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame = peek_frame(framemgr, state);
		framemgr_x_barrier_common(framemgr, 0, flags);
		if (frame && frame->instance != instance) {
			msinfo_hw("frame->instance(%d), queued_count(%s(%d))\n",
					instance, hw_ip, frame->instance,
					hw_frame_state_name[state],
					framemgr->queued_count[state]);
		} else {
			retry = 10;
			while (--retry && framemgr->queued_count[state]) {
				mswarn_hw("%s(%d) com waiting...", instance, hw_ip,
						hw_frame_state_name[state],
						framemgr->queued_count[state]);
				usleep_range(5000, 5000);
			}
			if (!retry)
				mswarn_hw("waiting(until frame empty) is fail", instance, hw_ip);
		}
	}
}

static int is_hw_m2m_grp_stop(struct is_hardware *hardware, u32 instance, u32 mode, u32 *hw_slot_id)
{
	int ret, i;
	ulong hw_map = hardware->hw_map[instance];
	struct is_hw_ip *hw_ip;
	bool wait_done = false;

	mdbg_hw(1, "m2m_grp_stop hw_map[0x%lx] mode(%d)\n", instance, hw_map, mode);

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[hw_slot_id[i]];

		if (!wait_done) {
			wait_done = true;
			is_hw_m2m_grp_wait_frame(hw_ip, instance);
		}

		if (!test_and_clear_bit(instance, &hw_ip->run_rsc_state))
			mswarn_hw("try to disable disabled instance", instance, hw_ip);

		ret = CALL_HWIP_OPS(hw_ip, disable, instance, hw_map);
		if (ret)
			mserr_hw("disable fail", instance, hw_ip);

		if (!test_bit(HW_RUN, &hw_ip->state))
			atomic_set(&hw_ip->fcount, 0);
	}

	return 0;
}

static const struct is_hw_group_ops is_hw_group_ops_m2m = {
	.shot = is_hw_m2m_grp_shot,
	.open = is_hw_m2m_grp_open,
	.close = is_hw_m2m_grp_close,
	.stop = is_hw_m2m_grp_stop,
};

const struct is_hw_group_ops *is_hw_get_m2m_group_ops(void)
{
	return &is_hw_group_ops_m2m;
}
EXPORT_SYMBOL_GPL(is_hw_get_m2m_group_ops);
