/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-hw-vra.h"
#include "../interface/is-interface-ischain.h"
#include "is-err.h"

void is_hw_vra_save_debug_info(struct is_hw_ip *hw_ip,
	struct is_lib_vra *lib_vra, int debug_point)
{
	struct is_hardware *hardware;
	u32 hw_fcount, index, instance;

	FIMC_BUG_VOID(!hw_ip);

	hardware = hw_ip->hardware;
	instance = atomic_read(&hw_ip->instance);
	hw_fcount = atomic_read(&hw_ip->fcount);

	switch (debug_point) {
	case DEBUG_POINT_FRAME_START:
		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount, DEBUG_POINT_FRAME_START);

		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]])
			|| test_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state))
			msinfo_hw("[F:%d]F.S\n", instance, hw_ip, hw_fcount);
		break;
	case DEBUG_POINT_FRAME_END:
		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, hw_fcount, DEBUG_POINT_FRAME_DMA_END);

		index = hw_ip->debug_index[1];
		dbg_isr_hw("[F:%d][S-E] %05llu us\n", hw_ip, hw_fcount,
			(hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_END] -
			hw_ip->debug_info[index].time[DEBUG_POINT_FRAME_START]) / 1000);

		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]]))
			msinfo_hw("[F:%d]F.E\n", instance, hw_ip, hw_fcount);

		if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
			mserr_hw("fs(%d), fe(%d), dma(%d)", instance, hw_ip,
				atomic_read(&hw_ip->count.fs),
				atomic_read(&hw_ip->count.fe),
				atomic_read(&hw_ip->count.dma));
		}
		break;
	default:
		break;
	}
	return;
}

static int is_hw_vra_ch0_handle_interrupt(u32 id, void *context)
{
	int ret = 0;
	u32 status, intr_mask, intr_status;
	struct is_hw_ip *hw_ip = NULL;
	struct is_hw_vra *hw_vra = NULL;
	struct is_lib_vra *lib_vra;
	u32 hw_fcount;

	FIMC_BUG(!context);
	hw_ip = (struct is_hw_ip *)context;
	hw_fcount = atomic_read(&hw_ip->fcount);

	FIMC_BUG(!hw_ip->priv_info);
	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		serr_hw("[ch0]: hw_vra is null", hw_ip);
		return -EINVAL;
	}
	lib_vra = &hw_vra->lib_vra;

	intr_status = is_vra_chain0_get_status_intr(hw_ip->regs[REG_SETA]);
	intr_mask   = is_vra_chain0_get_enable_intr(hw_ip->regs[REG_SETA]);
	status = (intr_mask) & intr_status;
	sdbg_hw(2, "isr[ch0]: id(%d), status(0x%x), mask(0x%x), status(0x%x)\n",
		hw_ip, id, intr_status, intr_mask, status);

	ret = is_lib_vra_handle_interrupt(&hw_vra->lib_vra, id);
	if (ret) {
		serr_hw("isr[ch0]: lib_vra_handle_interrupt is fail (%d)\n" \
			"status(0x%x), mask(0x%x), status(0x%x)", hw_ip,
			ret, intr_status, intr_mask, status);
	}

	if (status & (1 << CH0INT_END_FRAME)) {
		status &= (~(1 << CH0INT_END_FRAME));
		sdbg_hw(2, "[ch0]: END_FRAME\n", hw_ip);
		atomic_inc(&hw_ip->count.fe);
		hw_ip->cur_e_int++;
		if (hw_ip->cur_e_int >= hw_ip->num_buffers) {
			/* VRA stop register do not support shadowing */
			if (test_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state)) {
				sinfo_hw("[ch0]: END_FRAME: VRA_LIB_STOP [F:%d]\n", hw_ip, hw_fcount);
				ret = is_lib_vra_stop(lib_vra);
				if (ret)
					serr_hw("lib_vra_stop is fail (%d)", hw_ip, ret);
				clear_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state);
				atomic_set(&hw_vra->ch1_count, 0);
			}
			atomic_set(&hw_ip->status.Vvalid, V_BLANK);
			is_hw_vra_save_debug_info(hw_ip, lib_vra, DEBUG_POINT_FRAME_END);
#if !defined(VRA_DMA_TEST_BY_IMAGE)
			CALL_HW_OPS(hw_ip, frame_done, hw_ip, NULL, -1,
				IS_HW_CORE_END, IS_SHOT_SUCCESS, true);
#endif
			wake_up(&hw_ip->status.wait_queue);
		}
	}

	if (status & (1 << CH0INT_CIN_FR_ST)) {
		status &= (~(1 << CH0INT_CIN_FR_ST));
		sdbg_hw(2, "[ch0]: CIN_FR_ST\n", hw_ip);
		atomic_inc(&hw_ip->count.fs);
		hw_ip->cur_s_int++;
		if (hw_ip->cur_s_int == 1) {
			is_hw_vra_save_debug_info(hw_ip, lib_vra, DEBUG_POINT_FRAME_START);
			atomic_set(&hw_ip->status.Vvalid, V_VALID);
			clear_bit(HW_CONFIG, &hw_ip->state);
		}

		if (hw_ip->cur_s_int < hw_ip->num_buffers) {
			ret = is_lib_vra_new_frame(lib_vra, NULL, NULL, atomic_read(&hw_ip->instance));
			if (ret)
				serr_hw("lib_vra_new_frame is fail (%d)", hw_ip, ret);
		}
	}

	if (status)
		sinfo_hw("[ch0]: error! status(0x%x)\n", hw_ip, intr_status);

	return ret;
}

static int is_hw_vra_ch1_handle_interrupt(u32 id, void *context)
{
	int ret = 0;
	u32 status, intr_mask, intr_status;
	struct is_hardware *hardware;
	struct is_hw_ip *hw_ip = NULL;
	struct is_hw_vra *hw_vra = NULL;
	struct is_lib_vra *lib_vra;
	u32 instance;
	bool has_vra_ch1_only = false;
	u32 ch1_mode;
	u32 hw_fcount;

	FIMC_BUG(!context);
	hw_ip = (struct is_hw_ip *)context;
	hardware = hw_ip->hardware;
	instance = atomic_read(&hw_ip->instance);
	hw_fcount = atomic_read(&hw_ip->fcount);

	ret = is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);

	FIMC_BUG(!hw_ip->priv_info);
	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		serr_hw("isr[ch1]: hw_vra is null", hw_ip);
		return -EINVAL;
	}
	lib_vra = &hw_vra->lib_vra;

	ch1_mode = is_vra_chain1_get_image_mode(hw_ip->regs[REG_SETA]);
	intr_status = is_vra_chain1_get_status_intr(hw_ip->regs[REG_SETA]);
	intr_mask   = is_vra_chain1_get_enable_intr(hw_ip->regs[REG_SETA]);
	status = (intr_mask) & intr_status;
	msdbg_hw(2, "isr[ch1]: id(%d), status(0x%x), mask(0x%x), status(0x%x)\n",
		instance, hw_ip, id, intr_status, intr_mask, status);

	ret = is_lib_vra_handle_interrupt(&hw_vra->lib_vra, id);
	if (ret) {
		mserr_hw("[ch1]: lib_vra_handle_interrupt is fail (%d)\n" \
			"status(0x%x), mask(0x%x), status(0x%x), ch1_count(%d)", instance, hw_ip,
			ret, intr_status, intr_mask, status, atomic_read(&hw_vra->ch1_count));
	}

	if (status & (1 << CH1INT_IN_START_OF_CONTEXT)) {
		status &= (~(1 << CH1INT_IN_START_OF_CONTEXT));
		msdbg_hw(2, "[ch1]: START_OF_CONTEXT\n", instance, hw_ip);
		if (has_vra_ch1_only && test_bit(HW_VRA_CH1_START, &hw_ip->state)) {
			clear_bit(HW_VRA_CH1_START, &hw_ip->state);
			atomic_inc(&hw_ip->count.fs);
			hw_ip->cur_s_int++;
			if (hw_ip->cur_s_int == 1) {
				CALL_HW_OPS(hw_ip, frame_start, hw_ip, instance);
				is_hw_vra_save_debug_info(hw_ip, lib_vra, DEBUG_POINT_FRAME_START);
				atomic_set(&hw_ip->status.Vvalid, V_VALID);
				clear_bit(HW_CONFIG, &hw_ip->state);
#ifdef ENABLE_REPROCESSING_FD
				clear_bit(instance, &lib_vra->done_vra_callback_out_ready);
				clear_bit(instance, &lib_vra->done_vra_hw_intr);
#endif
			}

			if (hw_ip->cur_s_int < hw_ip->num_buffers) {
				if (hw_ip->mframe) {
					struct is_frame *mframe = hw_ip->mframe;
					unsigned char *buffer_kva = NULL;
					unsigned char *buffer_dva = NULL;
					mframe->cur_buf_index = hw_ip->cur_s_int;
					/* TODO: It is required to support other YUV format */
					/* Y-plane index of NV21 */
					buffer_kva = (unsigned char *)
						(mframe->kvaddr_buffer[mframe->cur_buf_index * 2]);
					buffer_dva = (unsigned char *)
						(mframe->dvaddr_buffer[mframe->cur_buf_index * 2]);
					ret = is_lib_vra_new_frame(lib_vra, buffer_kva, buffer_dva, atomic_read(&hw_ip->instance));
					if (ret)
						mserr_hw("lib_vra_new_frame is fail (%d)",
							instance, hw_ip, ret);
				} else {
					mserr_hw("mframe is null(s:%d, e:%d, t:%d)", instance, hw_ip,
						hw_ip->cur_s_int, hw_ip->cur_e_int, hw_ip->num_buffers);
				}
			}
		}
	}

	if (status & (1 << CH1INT_IN_END_OF_CONTEXT)) {
		status &= (~(1 << CH1INT_IN_END_OF_CONTEXT));
		if (!atomic_read(&hardware->streaming[hardware->sensor_position[instance]])
			|| test_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state))
			msinfo_hw("[ch1]: END_OF_CONTEXT [F:%d](%d)\n", instance, hw_ip,
				atomic_read(&hw_ip->fcount),
				(atomic_read(&hw_vra->ch1_count) % VRA_CH1_INTR_CNT_PER_FRAME));
		atomic_inc(&hw_ip->count.dma);
		atomic_inc(&hw_vra->ch1_count);

		msdbg_hw(2, "[ch1]: END_OF_CONTEXT\n", instance, hw_ip);
		if (has_vra_ch1_only && (ch1_mode == VRA_CH1_DETECT_MODE)) {
			/* detect mode */

			set_bit(HW_VRA_CH1_START, &hw_ip->state);
			atomic_inc(&hw_ip->count.fe);
			hw_ip->cur_e_int++;
			if (hw_ip->cur_e_int >= hw_ip->num_buffers) {
				atomic_set(&hw_ip->status.Vvalid, V_BLANK);
				is_hw_vra_save_debug_info(hw_ip, lib_vra, DEBUG_POINT_FRAME_END);
#ifdef ENABLE_REPROCESSING_FD
				set_bit(instance, &lib_vra->done_vra_hw_intr);

				spin_lock(&lib_vra->reprocess_fd_lock);
				if (test_bit(instance, &lib_vra->done_vra_hw_intr)
					&& test_bit(instance, &lib_vra->done_vra_callback_out_ready)) {
					clear_bit(instance, &lib_vra->done_vra_callback_out_ready);
					clear_bit(instance, &lib_vra->done_vra_hw_intr);
					spin_unlock(&lib_vra->reprocess_fd_lock);

					CALL_HW_OPS(hw_ip, frame_done, hw_ip, NULL, -1,
						IS_HW_CORE_END, IS_SHOT_SUCCESS, true);
				} else {
					spin_unlock(&lib_vra->reprocess_fd_lock);
				}
#else
				CALL_HW_OPS(hw_ip, frame_done, hw_ip, NULL, -1,
					IS_HW_CORE_END, IS_SHOT_SUCCESS, true);
#endif
				wake_up(&hw_ip->status.wait_queue);

				hw_ip->mframe = NULL;
			}
		}
	}

	if (status)
		msinfo_hw("[ch1]: error! status(0x%x)\n", instance, hw_ip, intr_status);

	return ret;
}

static void is_hw_vra_reset(struct is_hw_ip *hw_ip)
{
	u32 all_intr;
	bool has_vra_ch1_only = false;
	int ret = 0;

	/* Interrupt clear */
	ret = is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);
	if (!has_vra_ch1_only) {
		all_intr = is_vra_chain0_get_all_intr(hw_ip->regs[REG_SETA]);
		is_vra_chain0_set_clear_intr(hw_ip->regs, all_intr);
	}

	all_intr = is_vra_chain1_get_all_intr(hw_ip->regs[REG_SETA]);
	is_vra_chain1_set_clear_intr(hw_ip->regs[REG_SETA], all_intr);
}

static int __nocfi is_hw_vra_open(struct is_hw_ip *hw_ip, u32 instance,
	struct is_group *group)
{
	int ret = 0;
	int ret_err = 0;
	ulong dma_addr;
	struct is_hw_vra *hw_vra = NULL;
	struct is_subdev *leader;
	enum is_lib_vra_input_type input_type;

	FIMC_BUG(!hw_ip);

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, "HWVRA");
	frame_manager_open(hw_ip->framemgr, IS_MAX_HW_FRAME, false);

	hw_ip->priv_info = vzalloc(sizeof(struct is_hw_vra));
	if(!hw_ip->priv_info) {
		mserr_hw("hw_ip->priv_info(null)", instance, hw_ip);
		ret = -ENOMEM;
		goto err_alloc;
	}

	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;

	leader = &group->leader;
	if (leader->id == ENTRY_VRA)
		input_type = VRA_INPUT_MEMORY;
	else
		input_type = VRA_INPUT_OTF;

	is_hw_vra_reset(hw_ip);

	is_fpsimd_get_func();
	get_lib_vra_func((void *)&hw_vra->lib_vra.itf_func);
	is_fpsimd_put_func();

	msdbg_hw(2, "library interface was initialized\n", instance, hw_ip);

	dma_addr = group->device->minfo->kvaddr_vra;
	ret = is_lib_vra_alloc_memory(&hw_vra->lib_vra, dma_addr);
	if (ret) {
		mserr_hw("failed to alloc. memory", instance, hw_ip);
		goto err_vra_alloc_memory;
	}

	ret = is_lib_vra_frame_work_init(&hw_vra->lib_vra, hw_ip->regs[REG_SETA], input_type);
	if (ret) {
		mserr_hw("failed to init. framework (%d)", instance, hw_ip, ret);
		goto err_vra_frame_work_init;
	}

	ret = is_lib_vra_init_task(&hw_vra->lib_vra);
	if (ret) {
		mserr_hw("failed to init. task(%d)", instance, hw_ip, ret);
		goto err_vra_init_task;
	}
#ifdef ENABLE_REPROCESSING_FD
		hw_vra->lib_vra.hw_ip = hw_ip;
#endif

#if defined(VRA_DMA_TEST_BY_IMAGE)
	ret = is_lib_vra_test_image_load(&hw_vra->lib_vra);
	if (ret) {
		mserr_hw("failed to load test image (%d)", instance, hw_ip, ret);
		return ret;
	}
#endif

	atomic_set(&hw_vra->ch1_count, 0);

	/* lib_vra spinlock init */
	spin_lock_init(&hw_vra->lib_vra.slock);

	set_bit(HW_OPEN, &hw_ip->state);
	msdbg_hw(2, "open: [G:0x%lx], framemgr[%s]", instance, hw_ip,
		GROUP_ID(group->id), hw_ip->framemgr->name);

	return 0;

err_vra_init_task:
	ret_err = is_lib_vra_frame_work_final(&hw_vra->lib_vra);
	if (ret_err)
		mserr_hw("lib_vra_destory_object is fail (%d)", instance, hw_ip, ret_err);
err_vra_frame_work_init:
	ret_err = is_lib_vra_free_memory(&hw_vra->lib_vra);
	if (ret_err)
		mserr_hw("lib_vra_free_memory is fail (%d)", instance, hw_ip, ret_err);
err_vra_alloc_memory:
	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
err_alloc:
	frame_manager_close(hw_ip->framemgr);
	return ret;
}

static int is_hw_vra_init(struct is_hw_ip *hw_ip, u32 instance,
	struct is_group *group, bool flag)
{
	int ret = 0;
	struct is_hw_vra *hw_vra = NULL;
	struct is_lib_vra *lib_vra;
	struct is_subdev *leader;
	enum is_lib_vra_input_type input_type;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!hw_ip->priv_info);
	FIMC_BUG(!group);

	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;
	lib_vra = &hw_vra->lib_vra;

	leader = &group->leader;
	if (leader->id == ENTRY_VRA)
		input_type = VRA_INPUT_MEMORY;
	else
		input_type = VRA_INPUT_OTF;

	if (input_type != lib_vra->fr_work_init.dram_input) {
		mserr_hw("input type is not matched - instance: %s, framework: %s",
				instance, hw_ip, input_type ? "M2M" : "OTF",
				lib_vra->fr_work_init.dram_input ? "M2M": "OTF");
		return -EINVAL;
	}

	ret = is_lib_vra_frame_desc_init(&hw_vra->lib_vra, instance);
	if (ret) {
		mserr_hw("failed to init frame desc. (%d)", instance, hw_ip, ret);
		return ret;
	}

	set_bit(HW_INIT, &hw_ip->state);
	msdbg_hw(2, "ready to start\n", instance, hw_ip);

	return 0;
}

static int is_hw_vra_deinit(struct is_hw_ip *hw_ip, u32 instance)
{
	return 0;
}

static int is_hw_vra_close(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct is_hw_vra *hw_vra = NULL;

	FIMC_BUG(!hw_ip);

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		mserr_hw("priv_info is NULL", instance, hw_ip);
		return -EINVAL;
	}

	ret = is_lib_vra_frame_work_final(&hw_vra->lib_vra);
	if (ret) {
		mserr_hw("lib_vra_destory_object is fail (%d)", instance, hw_ip, ret);
		return ret;
	}

	ret = is_lib_vra_free_memory(&hw_vra->lib_vra);
	if (ret) {
		mserr_hw("lib_vra_free_memory is fail", instance, hw_ip);
		return ret;
	}

	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;
	frame_manager_close(hw_ip->framemgr);

	clear_bit(HW_OPEN, &hw_ip->state);

	return ret;
}

static int is_hw_vra_enable(struct is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	set_bit(HW_RUN, &hw_ip->state);
	set_bit(HW_VRA_CH1_START, &hw_ip->state);

	return ret;
}

static int is_hw_vra_disable(struct is_hw_ip *hw_ip, u32 instance,
	ulong hw_map)
{
	int ret = 0;
	long timetowait;
	struct is_hw_vra *hw_vra = NULL;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		mserr_hw("priv_info is NULL", instance, hw_ip);
		return -EINVAL;
	}

	ret = is_lib_vra_stop_instance(&hw_vra->lib_vra, instance);
	if (ret) {
		mserr_hw("lib_vra_stop_instance is fail (%d)", instance, hw_ip, ret);
		return ret;
	}

	if (hw_ip->run_rsc_state)
		return 0;

	msinfo_hw("vra_disable: Vvalid(%d)\n", instance, hw_ip,
		atomic_read(&hw_ip->status.Vvalid));

	if (test_bit(HW_RUN, &hw_ip->state)) {
		timetowait = wait_event_timeout(hw_ip->status.wait_queue,
			!atomic_read(&hw_ip->status.Vvalid),
			IS_HW_STOP_TIMEOUT);

		if (!timetowait)
			mserr_hw("wait FRAME_END timeout (%ld)", instance, hw_ip, timetowait);

		ret = is_lib_vra_stop(&hw_vra->lib_vra);
		if (ret) {
			mserr_hw("lib_vra_stop is fail (%d)", instance, hw_ip, ret);
			return ret;
		}

		is_hw_vra_reset(hw_ip);
		atomic_set(&hw_vra->ch1_count, 0);
		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
	} else {
		msdbg_hw(2, "already disabled\n", instance, hw_ip);
	}

	return 0;
}

static int is_hw_vra_update_param(struct is_hw_ip *hw_ip,
	struct vra_param *param, IS_DECLARE_PMAP(pmap), u32 instance, u32 fcount)
{
	int ret = 0;
	struct is_hw_vra *hw_vra;
	struct is_lib_vra *lib_vra;

	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!param);

	lib_vra = &hw_vra->lib_vra;

#ifdef VRA_DMA_TEST_BY_IMAGE
	ret = is_lib_vra_test_input(lib_vra, instance);
	if (ret) {
		mserr_hw("test_input is fail (%d)", instance, hw_ip, ret);
		return ret;
	}
	return 0;
#endif

	if (param->otf_input.cmd == OTF_INPUT_COMMAND_ENABLE) {
		if (test_bit(PARAM_FD_OTF_INPUT, pmap)) {
			ret = is_lib_vra_otf_input(lib_vra, param, instance, fcount);
			if (ret) {
				mserr_hw("otf_input is fail (%d)", instance, hw_ip, ret);
				return ret;
			}
		}
	} else if (param->dma_input.cmd == DMA_INPUT_COMMAND_ENABLE) {
		if (test_bit(PARAM_FD_DMA_INPUT, pmap)) {
			ret = is_lib_vra_dma_input(lib_vra, param, instance, fcount);
			if (ret) {
				mserr_hw("dma_input is fail (%d)", instance, hw_ip, ret);
				return ret;
			}
		}
	} else {
		mserr_hw("param setting is wrong! otf_input.cmd(%d), dma_input.cmd(%d)",
			instance, hw_ip, param->otf_input.cmd, param->dma_input.cmd);
		return -EINVAL;
	}

	return ret;
}


static int is_hw_vra_shot(struct is_hw_ip *hw_ip, struct is_frame *frame,
	ulong hw_map)
{
	int ret = 0;
	struct is_group *head;
	struct is_hw_vra *hw_vra = NULL;
	struct vra_param *param;
	struct is_lib_vra *lib_vra;
	u32 instance;
	unsigned char *buffer_kva = NULL;
	unsigned char *buffer_dva = NULL;
	bool has_vra_ch1_only = false;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	/* multi-buffer */
	hw_ip->cur_s_int = 0;
	hw_ip->cur_e_int = 0;
	hw_ip->num_buffers = frame->num_buffers;

	instance = frame->instance;
	msdbgs_hw(2, "[F:%d] shot\n", instance, hw_ip, frame->fcount);

	hw_vra  = (struct is_hw_vra *)hw_ip->priv_info;
	param   = &hw_ip->region[instance]->parameter.vra;
	lib_vra = &hw_vra->lib_vra;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		msdbg_hw(2, "not initialized\n", instance, hw_ip);
		return -EINVAL;
	}

	if (frame->type == SHOT_TYPE_INTERNAL) {
		msdbg_hw(2, "request not exist\n", instance, hw_ip);
		goto new_frame;
	}

	FIMC_BUG(!frame->shot);

#ifdef ENABLE_HYBRID_FD
	if (frame->shot_ext) {
		lib_vra->post_detection_enable[instance] = frame->shot_ext->hfd.hfd_enable;
		ret = is_lib_vra_set_post_detect_output(lib_vra,
					lib_vra->post_detection_enable[instance], instance);
		if (ret) {
			mserr_hw("lib_vra_set_post_detect_output_enable is fail (%d)",
					instance, hw_ip, ret);
			return ret;
		}
	}
#endif

#if !defined(VRA_DMA_TEST_BY_IMAGE)
	ret = is_lib_vra_set_orientation(lib_vra,
			frame->shot->uctl.scalerUd.orientation, instance);
	if (ret) {
		mserr_hw("lib_vra_set_orientation is fail (%d)",
				instance, hw_ip, ret);
		return ret;
	}

	ret = is_hw_vra_update_param(hw_ip, param, frame->pmap, instance, frame->fcount);
	if (ret) {
		mserr_hw("lib_vra_update_param is fail (%d)", instance, hw_ip, ret);
		return ret;
	}
#endif

new_frame:
	head = hw_ip->group[frame->instance]->head;

	ret = is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);
	if(!has_vra_ch1_only && param->control.cmd == CONTROL_COMMAND_STOP) {
		if (!test_bit(VRA_LIB_FWALGS_ABORT, &lib_vra->state)) {
			if (test_bit(IS_GROUP_OTF_INPUT, &head->state)) {
				msinfo_hw("shot: VRA_LIB_BYPASS_REQUESTED [F:%d](%d)\n",
					instance, hw_ip, frame->fcount, param->control.cmd);
				set_bit(VRA_LIB_BYPASS_REQUESTED, &lib_vra->state);
			} else {
				msinfo_hw("shot: lib_vra_stop [F:%d](%d)\n",
					instance, hw_ip, frame->fcount, param->control.cmd);
				ret = is_lib_vra_stop(lib_vra);
				if (ret)
					mserr_hw("lib_vra_stop is fail (%d)", instance, hw_ip, ret);

				atomic_set(&hw_vra->ch1_count, 0);
			}
		}

		return ret;
	}
	/*
	 * OTF mode: the buffer value is null.
	 * DMA mode: the buffer value is VRA input DMA address.
	 * ToDo: DMA input buffer set by buffer hiding
	 */
	/* Add for CH1 DMA input */
	if (lib_vra->fr_work_init.dram_input) {
		/* TODO: It is required to support other YUV format */
		/* Y-plane index of NV21 */
		buffer_kva = (unsigned char *)
			(frame->kvaddr_buffer[frame->cur_buf_index * 2]);
		buffer_dva = (unsigned char *)
			(frame->dvaddr_buffer[frame->cur_buf_index * 2]);
		hw_ip->mframe = frame;
	}
	msdbg_hw(2, "[F:%d]lib_vra_new_frame\n", instance, hw_ip, frame->fcount);
	lib_vra->fr_index = frame->fcount;
	ret = is_lib_vra_new_frame(lib_vra, buffer_kva, buffer_dva, instance);
	if (ret) {
		mserr_hw("lib_vra_new_frame is fail (%d)", instance, hw_ip, ret);
		msinfo_hw("count fs(%d), fe(%d), dma(%d)\n", instance, hw_ip,
			atomic_read(&hw_ip->count.fs),
			atomic_read(&hw_ip->count.fe),
			atomic_read(&hw_ip->count.dma));
		return ret;
	}

#if defined(VRA_DMA_TEST_BY_IMAGE)
	clear_bit(hw_ip->id, &frame->core_flag);
#endif
	set_bit(HW_CONFIG, &hw_ip->state);

	return 0;
}

static int is_hw_vra_set_param(struct is_hw_ip *hw_ip, struct is_region *region,
		IS_DECLARE_PMAP(pmap), u32 instance, ulong hw_map)
{
	int ret;
	struct is_hw_vra *hw_vra;
	struct vra_param *param;
	struct is_lib_vra *lib_vra;
	u32 fcount;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!region);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	fcount = atomic_read(&hw_ip->fcount);

	hw_ip->region[instance] = region;

	param   = &region->parameter.vra;
	hw_vra  = (struct is_hw_vra *)hw_ip->priv_info;
	lib_vra = &hw_vra->lib_vra;

#ifndef ENABLE_VRA_CHANGE_SETFILE_PARSING
	if (!test_bit(VRA_INST_APPLY_TUNE_SET, &lib_vra->inst_state[instance])) {
		ret = is_lib_vra_apply_tune(lib_vra, NULL, instance);
		if (ret) {
			mserr_hw("set_tune is fail (%d)", instance, hw_ip, ret);
			return ret;
		}
	}
#endif

	msdbg_hw(2, "set_param \n", instance, hw_ip);
	ret = is_hw_vra_update_param(hw_ip, param,
			pmap, instance, fcount);
	if (ret) {
		mserr_hw("set_param is fail (%d)", instance, hw_ip, ret);
		return ret;
	}

	return ret;
}

static int is_hw_vra_frame_ndone(struct is_hw_ip *hw_ip,
	struct is_frame *frame, u32 instance, enum ShotErrorType done_type)
{
	int ret = 0;
	int wq_id;
	int output_id;

	FIMC_BUG(!hw_ip);
	FIMC_BUG(!frame);

	wq_id     = -1;
	output_id = IS_HW_CORE_END;
	if (test_bit_variables(hw_ip->id, &frame->core_flag))
		ret = CALL_HW_OPS(hw_ip, frame_done, hw_ip, frame, wq_id, output_id,
				done_type, false);

	return ret;
}

#ifdef ENABLE_VRA_CHANGE_SETFILE_PARSING
static int is_hw_vra_load_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_vra *hw_vra = NULL;
	struct is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;
	ulong addr;
	u32 size, index;
	int flag = 0, ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		mserr_hw("priv_info is NULL", instance, hw_ip);
		return -EINVAL;
	}
	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	switch (setfile->version) {
	case SETFILE_V2:
		flag = false;
		break;
	case SETFILE_V3:
		flag = true;
		break;
	default:
		mserr_hw("invalid version (%d)", instance, hw_ip,
				 setfile->version);
		return -EINVAL;
	}

	for (index = 0; index < setfile->using_count; index++) {
		addr = setfile->table[index].addr;
		size = setfile->table[index].size;
		ret = is_lib_vra_copy_tune_set(&hw_vra->lib_vra,
			addr, size, index, flag, instance);

		set_bit(index, &hw_vra->lib_vra.tune_count);
	}

	set_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

static int is_hw_vra_apply_setfile(struct is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	struct is_hw_ip_setfile *setfile;
	struct is_hw_vra *hw_vra;
	struct is_lib_vra *lib_vra;
	enum exynos_sensor_position sensor_position;
	u32 setfile_index;
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	hw_vra  = (struct is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		mserr_hw("priv_info is NULL", instance, hw_ip);
		return -EINVAL;
	}

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile    = &hw_ip->setfile[sensor_position];
	lib_vra = &hw_vra->lib_vra;
	clear_bit(VRA_INST_APPLY_TUNE_SET, &lib_vra->inst_state[instance]);

	setfile_index = setfile->index[scenario];
	if (setfile_index >= setfile->using_count) {
		mserr_hw("setfile index is out-of-range, [%d:%d]",
				instance, hw_ip, scenario, setfile_index);
		return -EINVAL;
	}

	msinfo_hw("setfile (%d) scenario (%d)\n", instance, hw_ip,
		setfile_index, scenario);

	ret = is_lib_vra_apply_tune_set(&hw_vra->lib_vra, setfile_index, instance);
	if (ret) {
		mserr_hw("apply_tune is fail (%d)", instance, hw_ip, ret);
		return ret;
	}

	lib_vra->max_face_num = MAX_FACE_COUNT;
	if (sensor_position == SENSOR_POSITION_FRONT)
		lib_vra->orientation[instance] = VRA_FRONT_ORIENTATION;
	else
		lib_vra->orientation[instance] = VRA_REAR_ORIENTATION;

	return 0;
}

#else
static int is_hw_vra_load_setfile(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct is_hw_vra *hw_vra = NULL;
	struct is_hw_ip_setfile *setfile;
	enum exynos_sensor_position sensor_position;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	if (!unlikely(hw_ip->priv_info)) {
		mserr_hw("priv_info is NULL", instance, hw_ip);
		return -EINVAL;
	}
	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;
	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile = &hw_ip->setfile[sensor_position];

	switch (setfile->version) {
	case SETFILE_V2:
		break;
	case SETFILE_V3:
		break;
	default:
		mserr_hw("invalid version (%d)", instance, hw_ip,
				 setfile->version);
		return -EINVAL;
	}

	set_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

static int is_hw_vra_apply_setfile(struct is_hw_ip *hw_ip, u32 scenario,
	u32 instance, ulong hw_map)
{
	struct is_hw_ip_setfile *setfile;
	struct is_hw_vra_setfile *setfile_vra;
	struct is_hw_vra *hw_vra;
	struct is_lib_vra *lib_vra;
	struct is_lib_vra_tune_data tune;
	enum exynos_sensor_position sensor_position;
	u32 setfile_index;
	u32 p_rot_mask;
	int ret = 0;

	FIMC_BUG(!hw_ip);

	if (!test_bit_variables(hw_ip->id, &hw_map)) {
		msdbg_hw(2, "%s: hw_map(0x%lx)\n", instance, hw_ip, __func__, hw_map);
		return 0;
	}

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -ESRCH;
	}

	hw_vra  = (struct is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		mserr_hw("priv_info is NULL", instance, hw_ip);
		return -EINVAL;
	}

	sensor_position = hw_ip->hardware->sensor_position[instance];
	setfile    = &hw_ip->setfile[sensor_position];
	lib_vra = &hw_vra->lib_vra;
	clear_bit(VRA_INST_APPLY_TUNE_SET, &lib_vra->inst_state[instance]);

	setfile_index = setfile->index[scenario];
	if (setfile_index >= setfile->using_count) {
		mserr_hw("setfile index is out-of-range, [%d:%d]",
				instance, hw_ip, scenario, setfile_index);
		return -EINVAL;
	}

	msinfo_hw("setfile (%d) scenario (%d)\n", instance, hw_ip,
		setfile_index, scenario);

	hw_vra->setfile = *(struct is_hw_vra_setfile *)setfile->table[setfile_index].addr;
	setfile_vra = &hw_vra->setfile;

	if (setfile_vra->setfile_version != VRA_SETFILE_VERSION)
		mserr_hw("setfile version is wrong:(%#x) expected version (%#x)",
			instance, hw_ip, setfile_vra->setfile_version, VRA_SETFILE_VERSION);

	tune.api_tune.tracking_mode   = setfile_vra->tracking_mode;
	tune.api_tune.enable_features = setfile_vra->enable_features;
	tune.api_tune.min_face_size   = setfile_vra->min_face_size;
	tune.api_tune.max_face_count  = setfile_vra->max_face_count;
	tune.api_tune.face_priority   = setfile_vra->face_priority;
	tune.api_tune.disable_frontal_rot_mask =
		(setfile_vra->limit_rotation_angles & 0xFF);

	if (setfile_vra->disable_profile_detection)
		p_rot_mask = (setfile_vra->limit_rotation_angles >> 8) | (0xFF);
	else
		p_rot_mask = (setfile_vra->limit_rotation_angles >> 8) | (0xFE);

	tune.api_tune.disable_profile_rot_mask = p_rot_mask;
	tune.api_tune.working_point       = setfile_vra->boost_dr_vs_fpr;
	tune.api_tune.tracking_smoothness = setfile_vra->tracking_smoothness;
	tune.api_tune.selfie_working_point = setfile_vra->lock_frame_number;
	tune.api_tune.sensor_position      = sensor_position;

	tune.dir = setfile_vra->front_orientation;

	tune.frame_lock.lock_frame_num         = setfile_vra->lock_frame_number;
	tune.frame_lock.init_frames_per_lock   = setfile_vra->init_frames_per_lock;
	tune.frame_lock.normal_frames_per_lock = setfile_vra->normal_frames_per_lock;

	if (lib_vra->fr_work_init.dram_input)
		tune.api_tune.full_frame_detection_freq =
				setfile_vra->init_frames_per_lock;
	else
		tune.api_tune.full_frame_detection_freq =
				setfile_vra->normal_frames_per_lock;

	ret = is_lib_vra_apply_tune(lib_vra, &tune, instance);
	if (ret) {
		mserr_hw("apply_tune is fail (%d)", instance, hw_ip, ret);
		return ret;
	}

	return 0;
}
#endif

static int is_hw_vra_delete_setfile(struct is_hw_ip *hw_ip, u32 instance,
		ulong hw_map)
{
	clear_bit(HW_TUNESET, &hw_ip->state);

	return 0;
}

static int is_hw_vra_get_meta(struct is_hw_ip *hw_ip,
		struct is_frame *frame, unsigned long hw_map)
{
	int ret = 0;
	struct is_hw_vra *hw_vra;

	if (unlikely(!frame)) {
		mserr_hw("get_meta: frame is null", atomic_read(&hw_ip->instance), hw_ip);
		return 0;
	}

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	hw_vra = (struct is_hw_vra *)hw_ip->priv_info;
	if (unlikely(!hw_vra)) {
		mserr_hw("priv_info is NULL", frame->instance, hw_ip);
		return -EINVAL;
	}

	ret = is_lib_vra_get_meta(&hw_vra->lib_vra, frame);
	if (ret)
		mserr_hw("get_meta is fail (%d)", frame->instance, hw_ip, ret);

	return ret;
}

const struct is_hw_ip_ops is_hw_vra_ops = {
	.open			= is_hw_vra_open,
	.init			= is_hw_vra_init,
	.deinit			= is_hw_vra_deinit,
	.close			= is_hw_vra_close,
	.enable			= is_hw_vra_enable,
	.disable		= is_hw_vra_disable,
	.shot			= is_hw_vra_shot,
	.set_param		= is_hw_vra_set_param,
	.get_meta		= is_hw_vra_get_meta,
	.frame_ndone		= is_hw_vra_frame_ndone,
	.load_setfile		= is_hw_vra_load_setfile,
	.apply_setfile		= is_hw_vra_apply_setfile,
	.delete_setfile		= is_hw_vra_delete_setfile,
};

int is_hw_vra_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name)
{
	int ret = 0;
	int hw_slot;
	bool has_vra_ch1_only = false;

	hw_ip->ops  = &is_hw_vra_ops;

	/* set fd sfr base address */
	hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, id);

	/* set vra interrupt handler */
	ret = is_hw_g_ctrl(NULL, 0, HW_G_CTRL_HAS_VRA_CH1_ONLY, (void *)&has_vra_ch1_only);

	if (!has_vra_ch1_only)
		itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &is_hw_vra_ch0_handle_interrupt;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].handler = &is_hw_vra_ch1_handle_interrupt;

	return ret;
}

void is_hw_vra_remove(struct is_hw_ip *hw_ip)	{ }
