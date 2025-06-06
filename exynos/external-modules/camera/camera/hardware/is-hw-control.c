/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>

#include "exynos-is.h"
#include "videodev2_exynos_camera.h"
#include "pablo-hw-helper.h"
#include "is-err.h"
#include "is-core.h"
#include "is-hw-control.h"
#include "is-hw-dm.h"
#include "pablo-work.h"
#include "is-hw-ip.h"
#include "pablo-dvfs.h"
#include "pablo-obte.h"
#include "pablo-fpsimd.h"
#include "is-interface-sensor.h"
#include "is-interface-ddk.h"
#include "pablo-mmio.h"
#include "pablo-icpu-adapter.h"
#include "pablo-crta-bufmgr.h"
#include "is-device-csi.h"
#include "is-devicemgr.h"
#include "pmio.h"

#define INTERNAL_SHOT_EXIST	(1)

static int param_debug_time_hw_usage(char *buffer, const size_t buf_size)
{
	const char *usage_msg = "[value] number, set hw_ip id for debugging\n"
				"\t0 : turn off debug\n"
				"\tN : set hw_ip id for hw time debugging\n";

	return scnprintf(buffer, buf_size, usage_msg);
}

static struct pablo_debug_param debug_time_hw = {
	.type = IS_DEBUG_PARAM_TYPE_NUM,
	.max_value = DEV_HW_END,
	.ops.usage = param_debug_time_hw_usage,
};
module_param_cb(debug_time_hw, &pablo_debug_param_ops, &debug_time_hw, 0644);

static const struct is_hardware_ops is_hardware_ops;

static inline void wq_func_schedule(struct is_interface *itf,
	struct work_struct *work_wq)
{
	if (itf->workqueue)
		queue_work(itf->workqueue, work_wq);
	else
		schedule_work(work_wq);
}

static void prepare_sfr_dump(struct is_hardware *hardware)
{
	int hw_slot = -1;
	int reg_size = 0;
	struct is_hw_ip *hw_ip = NULL;
	int i;

	if (!hardware) {
		err_hw("hardware is null\n");
		return;
	}

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];

		if (hw_ip->id == DEV_HW_END || hw_ip->id == 0)
		       continue;

		hw_ip->sfr_dump_flag = false;

		for (i = 0; i < REG_SET_MAX; i++) {
			if (IS_ERR_OR_NULL(hw_ip->regs[i]) ||
				(hw_ip->regs_start[i] == 0) ||
				(hw_ip->regs_end[i] == 0))
				continue;

			reg_size = (hw_ip->regs_end[i] - hw_ip->regs_start[i] + 1);
			hw_ip->sfr_dump[i] = kzalloc(reg_size, GFP_KERNEL);
			if (IS_ERR_OR_NULL(hw_ip->sfr_dump[i]))
				serr_hw("sfr %d dump memory alloc fail", hw_ip, i);
			else
				sinfo_hw("sfr %d dump memory (V/P/S):(%lx/%lx/0x%X)[0x%llX~0x%llX]\n", hw_ip,
					i, (ulong)hw_ip->sfr_dump[i], (ulong)virt_to_phys(hw_ip->sfr_dump[i]),
					reg_size, hw_ip->regs_start[i], hw_ip->regs_end[i]);
		}
	}
}

static void free_sfr_dump(struct is_hardware *hardware)
{
	int hw_slot;
	struct is_hw_ip *hw_ip;
	int i;

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];

		if (hw_ip->id == DEV_HW_END || hw_ip->id == 0)
			continue;

		for (i = 0; i < REG_SET_MAX; i++) {
			if (IS_ERR_OR_NULL(hw_ip->regs[i]) || (hw_ip->regs_start[i] == 0) ||
				(hw_ip->regs_end[i] == 0))
				continue;

			kfree(hw_ip->sfr_dump[i]);
		}
	}
}

static void _is_hardware_sfr_dump_splits(
	struct is_hw_ip *hw_ip, u32 reg_id, void *sfr_dump, bool flag_print_log)
{
	struct is_hw_sfr_dump_region *split_list;
	resource_size_t pa_start, pa_end, ofs_start, ofs_end, size, total_size;
	u32 split_id = 0;
	void *va_src, *va_dst;

	split_list = (struct is_hw_sfr_dump_region *)hw_ip->dump_region[reg_id];

	pa_start = hw_ip->regs_start[reg_id];
	pa_end = hw_ip->regs_end[reg_id];
	total_size = pa_end - pa_start + 1;

	do {
		ofs_start = split_list[split_id].start;
		ofs_end = split_list[split_id].end;
		if (ofs_end < ofs_start)
			break;
		else if (!ofs_end)
			ofs_end = total_size - 1;

		va_src = hw_ip->regs[reg_id] + ofs_start;
		va_dst = sfr_dump + ofs_start;
		size = ofs_end - ofs_start + 1;

		memcpy_fromio(va_dst, va_src, size);
		sinfo_hw("##### SFR %d-%d DUMP(V/P/S):(0x%lx/0x%lx/0x%llX)[0x%llX~0x%llX]\n", hw_ip,
			reg_id, split_id, (ulong)va_dst, (ulong)virt_to_phys(va_dst), size,
			pa_start + ofs_start, pa_start + ofs_end);

		if (flag_print_log)
			print_hex_dump(
				KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4, va_dst, size, false);
	} while (++split_id < DUMP_SPLIT_MAX && ofs_end < total_size - 1);
}

static void _is_hardware_sfr_dump_list(
	struct is_hw_ip *hw_ip, u32 reg_id, void *sfr_dump, bool flag_print_log)
{
	resource_size_t pa_start, pa_end, total_size;
	void *va_src, *va_dst;
	u32 i, ofs;

	pa_start = hw_ip->regs_start[reg_id];
	pa_end = hw_ip->regs_end[reg_id];
	total_size = pa_end - pa_start + 1;

	for (i = 0; i < hw_ip->dump_reg_list_size; i++) {
		ofs = hw_ip->dump_for_each_reg[i].sfr_offset;
		va_src = hw_ip->regs[reg_id] + ofs;
		va_dst = sfr_dump + ofs;

		*(u32 *)va_dst = readl_relaxed(va_src);
	}

	sinfo_hw("##### SFR %d DUMP(V/P/S):(0x%lx/0x%lx/0x%llX)[0x%llX~0x%llX]\n", hw_ip, reg_id,
		(ulong)sfr_dump, (ulong)virt_to_phys(sfr_dump), total_size, pa_start, pa_end);

	if (flag_print_log)
		print_hex_dump(
			KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4, sfr_dump, total_size, false);
}

static void _is_hardware_sfr_dump_pmio(struct is_hw_ip *hw_ip, void *sfr_dump, bool flag_print_log)
{
	struct pablo_mmio *pmio = hw_ip->pmio;
	const struct pmio_access_table *rd_table;
	resource_size_t size, total_size;
	resource_size_t ofs_start = 0;
	void *va_dst;
	u32 i;
	bool org_cache_only, org_cache_bypass;

	total_size = pmio->max_register + 4;
	rd_table = pmio->rd_table;
	org_cache_only = pmio->cache_only;
	org_cache_bypass = pmio->cache_bypass;

	pmio->cache_only = false;
	pmio->cache_bypass = true;

	for (i = 0; rd_table && i < rd_table->n_no_ranges; i++) {
		va_dst = sfr_dump + ofs_start;
		size = rd_table->no_ranges[i].min - ofs_start;

		pmio_raw_read(pmio, ofs_start, va_dst, size);

		ofs_start = rd_table->no_ranges[i].max + 1;
	}

	va_dst = sfr_dump + ofs_start;
	size = total_size - ofs_start;

	pmio_raw_read(pmio, ofs_start, va_dst, size);

	pmio->cache_only = org_cache_only;
	pmio->cache_bypass = org_cache_bypass;

	sinfo_hw("##### SFR DUMP(V/P/S):(0x%lx/0x%lx/0x%llX)[0x%llX~0x%llX]\n", hw_ip,
		(ulong)sfr_dump, (ulong)virt_to_phys(sfr_dump), total_size, pmio->phys_base,
		pmio->phys_base + total_size - 1);

	if (flag_print_log)
		print_hex_dump(
			KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4, sfr_dump, total_size, false);
}

static void _is_hardware_sfr_dump(struct is_hw_ip *hw_ip, bool flag_print_log)
{
	enum base_reg_index i;
	void *sfr_dump;
	resource_size_t total_size;

	if (!test_bit(HW_OPEN, &hw_ip->state)) {
		swarn_hw("IP is not opend", hw_ip);
		return;
	}

#ifdef ENABLE_PANIC_SFR_PRINT
	flag_print_log = true;
#endif

	for (i = 0; i < REG_SET_MAX; i++) {
		sfr_dump = hw_ip->sfr_dump[i];
		if (IS_ERR_OR_NULL(sfr_dump))
			continue;

		total_size = (hw_ip->regs_end[i] - hw_ip->regs_start[i] + 1);

		if (hw_ip->sfr_dump_flag) {
			sinfo_hw(
				"already done: SFR %d DUMP(V/P/S):(0x%lx/0x%lx/0x%llX)[0x%llX~0x%llX]\n",
				hw_ip, i, (ulong)sfr_dump, (ulong)virt_to_phys(sfr_dump),
				total_size, hw_ip->regs_start[i], hw_ip->regs_end[i]);
			if (flag_print_log) {
				print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
						sfr_dump, total_size, false);
			}
			continue;
		}

		/* dump reg */
		if (hw_ip->dump_reg_list_size)
			_is_hardware_sfr_dump_list(hw_ip, i, sfr_dump, flag_print_log);
		else if (hw_ip->pmio)
			_is_hardware_sfr_dump_pmio(hw_ip, sfr_dump, flag_print_log);
		else
			_is_hardware_sfr_dump_splits(hw_ip, i, sfr_dump, flag_print_log);

		is_clean_dcache_area(sfr_dump, total_size);
	}

	hw_ip->sfr_dump_flag = true;
}

static inline void _is_hw_print_debug_trace(struct is_hw_ip *hw_ip, u32 index, u32 dbg_e, u32 dbg_x, const char *name)
{
	u32 instance = atomic_read(&hw_ip->instance);

	if (unlikely(test_bit(hw_ip->id, &debug_time_hw.value))) {
		msinfo_hw("TIME %s F%d: %05llu us\n", instance, hw_ip,
			   name, hw_ip->debug_info[index].fcount,
			   (hw_ip->debug_info[index].time[dbg_x] -
			   hw_ip->debug_info[index].time[dbg_e]) / 1000);
	}
}

void _is_hw_frame_dbg_trace(struct is_hw_ip *hw_ip, u32 fcount, u32 dbg_pts)
{
	u32 index, instance, debug_e;
	char name[20] = { 0, };

	FIMC_BUG_VOID(!hw_ip);

	debug_e = DEBUG_POINT_MAX;

	switch (dbg_pts) {
	case DEBUG_POINT_HW_SHOT_E:
		index = fcount % DEBUG_FRAME_COUNT;
		instance = atomic_read(&hw_ip->instance);
		hw_ip->debug_index[0] = fcount;
		hw_ip->debug_info[index].fcount = fcount;
		hw_ip->debug_info[index].instance = instance;
		break;
	case DEBUG_POINT_HW_SHOT_X:
		index = fcount % DEBUG_FRAME_COUNT;
		debug_e = DEBUG_POINT_HW_SHOT_E;
		snprintf(name, sizeof(name), "HW_SHOT");
		break;
	case DEBUG_POINT_FRAME_START:
		index = hw_ip->debug_index[0] % DEBUG_FRAME_COUNT;
		hw_ip->debug_index[1] = index;
		break;
	case DEBUG_POINT_FRAME_END:
		index = hw_ip->debug_index[1];
		debug_e = DEBUG_POINT_FRAME_START;
		snprintf(name, sizeof(name), "HW");
		break;
	case DEBUG_POINT_LIB_SHOT_E:
	case DEBUG_POINT_RTA_REGS_E:
	case DEBUG_POINT_ADD_TO_CMDQ:
	case DEBUG_POINT_CONFIG_LOCK_E:
	case DEBUG_POINT_SHOT_MSG:
		index = fcount % DEBUG_FRAME_COUNT;
		break;
	case DEBUG_POINT_LIB_SHOT_X:
		index = fcount % DEBUG_FRAME_COUNT;
		debug_e = DEBUG_POINT_LIB_SHOT_E;
		snprintf(name, sizeof(name), "LIB_SHOT");
		break;
	case DEBUG_POINT_RTA_REGS_X:
		index = fcount % DEBUG_FRAME_COUNT;
		debug_e = DEBUG_POINT_RTA_REGS_E;
		snprintf(name, sizeof(name), "RTA_REGS");
		break;
	case DEBUG_POINT_SETTING_DONE:
		index = fcount % DEBUG_FRAME_COUNT;
		debug_e = DEBUG_POINT_ADD_TO_CMDQ;
		snprintf(name, sizeof(name), "PRE_CONFIG");
		break;
	case DEBUG_POINT_CONFIG_LOCK_X:
		index = fcount % DEBUG_FRAME_COUNT;
		debug_e = DEBUG_POINT_CONFIG_LOCK_E;
		snprintf(name, sizeof(name), "CONFIG_LOCK");
		break;
	case DEBUG_POINT_SHOT_CALLBACK:
		index = fcount % DEBUG_FRAME_COUNT;
		debug_e = DEBUG_POINT_SHOT_MSG;
		snprintf(name, sizeof(name), "SHOT~CALLBACK");
		break;
	default:
		merr_hw("Invalid event (%d)\n",
			atomic_read(&hw_ip->instance), dbg_pts);
		return;
	}

	hw_ip->debug_info[index].cpuid[dbg_pts] = raw_smp_processor_id();
	hw_ip->debug_info[index].time[dbg_pts] = local_clock();
	if (debug_e != DEBUG_POINT_MAX)
		_is_hw_print_debug_trace(hw_ip, index, debug_e, dbg_pts, name);
}

void _is_hw_frame_dbg_ext_trace(struct is_hw_ip *hw_ip, u32 fcount, u32 dbg_pts, u32 ext_id)
{
	u32 index;

	FIMC_BUG_VOID(!hw_ip);

	switch (dbg_pts) {
	case DEBUG_POINT_FRAME_START:
		hw_ip->debug_ext_info[ext_id].fcount = fcount;
		hw_ip->debug_ext_info[ext_id].cpuid[dbg_pts] = raw_smp_processor_id();
		hw_ip->debug_ext_info[ext_id].time[dbg_pts] = cpu_clock(raw_smp_processor_id());
		break;
	case DEBUG_POINT_FRAME_END:
		hw_ip->debug_ext_info[ext_id].cpuid[dbg_pts] = raw_smp_processor_id();
		hw_ip->debug_ext_info[ext_id].time[dbg_pts] = cpu_clock(raw_smp_processor_id());
		if (unlikely(test_bit(hw_ip->id, &debug_time_hw.value))) {
			index = fcount % DEBUG_FRAME_COUNT;
			msinfo_hw("TIME %s F%d: %05llu us\n", atomic_read(&hw_ip->instance), hw_ip,
				"CINROW_END", fcount,
				(hw_ip->debug_ext_info[ext_id].time[dbg_pts] -
				hw_ip->debug_info[index].time[DEBUG_POINT_CONFIG_LOCK_E]) / 1000);
		}
		break;
	default:
		merr_hw("Invalid event (%d)\n",
			atomic_read(&hw_ip->instance), dbg_pts);
		break;
	}

}

void print_hw_frame_count(struct is_hw_ip *hw_ip)
{
	int f_index, p_index, ext_index;
	struct hw_debug_info *debug_info;
	struct hw_debug_info *debug_ext_info;
	unsigned long long time[DEBUG_POINT_MAX];
	ulong usec[DEBUG_POINT_MAX];
	u32 instance;
	struct is_group *group;
	struct is_device_sensor *sensor;
	struct is_device_csi *csi;

	if (!hw_ip) {
		err_hw("hw_ip is null\n");
		return;
	}

	/* skip printing frame count, if hw_ip wasn't opened */
	if (!test_bit(HW_OPEN, &hw_ip->state))
		return;

	/* csis interrupt debug */
	instance = atomic_read(&hw_ip->instance);
	group = hw_ip->group[instance];

	if (!group)
		goto exit;

	if (group->prev && group->prev->device_type == IS_DEVICE_SENSOR) {
		if (!group->device) {
			err_hw("device is NULL");
			goto exit;
		}
		sensor = group->device->sensor;
		if (!sensor) {
			err_hw("sensor is NULL");
			goto exit;
		}

		csi = v4l2_get_subdevdata(sensor->subdev_csi);
		if (!csi) {
			err_hw("CSI is NULL");
			goto exit;
		}

		info("[HW:CSI%d]\n", csi->otf_info.csi_ch);
		for (f_index = 0; f_index < DEBUG_FRAME_COUNT; f_index++) {
			debug_info = &csi->debug_info[f_index];
			for (p_index = 0 ; p_index < DEBUG_POINT_MAX; p_index++) {
				time[p_index]  = debug_info->time[p_index];
				usec[p_index]  = do_div(time[p_index], NSEC_PER_SEC);
			}

			info("[%d][F:%d] shot[%5lu.%06lu], fs[c%d][%5lu.%06lu], fe[c%d][%5lu.%06lu], dma[c%d][%5lu.%06lu]\n",
				f_index, debug_info->fcount,
				(ulong)time[DEBUG_POINT_HW_SHOT_E], usec[DEBUG_POINT_HW_SHOT_E] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_START],
				(ulong)time[DEBUG_POINT_FRAME_START], usec[DEBUG_POINT_FRAME_START] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_END],
				(ulong)time[DEBUG_POINT_FRAME_END], usec[DEBUG_POINT_FRAME_END] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_DMA_END],
				(ulong)time[DEBUG_POINT_FRAME_DMA_END], usec[DEBUG_POINT_FRAME_DMA_END] / NSEC_PER_USEC);
		}
	}

exit:
	sinfo_hw("fs(%d), cl(%d), fe(%d), dma(%d)\n", hw_ip,
			atomic_read(&hw_ip->count.fs),
			atomic_read(&hw_ip->count.cl),
			atomic_read(&hw_ip->count.fe),
			atomic_read(&hw_ip->count.dma));

	for (f_index = 0; f_index < DEBUG_FRAME_COUNT; f_index++) {
		debug_info = &hw_ip->debug_info[f_index];
		for (p_index = 0 ; p_index < DEBUG_POINT_MAX; p_index++) {
			time[p_index]  = debug_info->time[p_index];
			usec[p_index]  = do_div(time[p_index], NSEC_PER_SEC);
		}

		info_hw("[%d][F:%d] shot[%5lu.%06lu], fs[c%d][%5lu.%06lu], fe[c%d][%5lu.%06lu], dma[c%d][%5lu.%06lu] (%d) \n",
				f_index, debug_info->fcount,
				(ulong)time[DEBUG_POINT_HW_SHOT_E], usec[DEBUG_POINT_HW_SHOT_E] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_START],
				(ulong)time[DEBUG_POINT_FRAME_START], usec[DEBUG_POINT_FRAME_START] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_END],
				(ulong)time[DEBUG_POINT_FRAME_END], usec[DEBUG_POINT_FRAME_END] / NSEC_PER_USEC,
				debug_info->cpuid[DEBUG_POINT_FRAME_DMA_END],
				(ulong)time[DEBUG_POINT_FRAME_DMA_END], usec[DEBUG_POINT_FRAME_DMA_END] / NSEC_PER_USEC,
				debug_info->instance);
	}

	for (ext_index = 0; ext_index < DEBUG_EXT_MAX; ext_index++) {
		debug_ext_info =  &hw_ip->debug_ext_info[ext_index];
		if (debug_ext_info->time[DEBUG_POINT_FRAME_START]) {
			for (p_index = DEBUG_POINT_FRAME_START; p_index <= DEBUG_POINT_FRAME_END; p_index++) {
				time[p_index]  = debug_ext_info->time[p_index];
				usec[p_index]  = do_div(time[p_index], NSEC_PER_SEC);
			}
			info_hw("[EXT%d][F:%d] fs[c%d][%5lu.%06lu], fe[c%d][%5lu.%06lu]\n",
				ext_index, debug_ext_info->fcount,
				debug_ext_info->cpuid[DEBUG_POINT_FRAME_START],
				(ulong)time[DEBUG_POINT_FRAME_START],
				usec[DEBUG_POINT_FRAME_START] / NSEC_PER_USEC,
				debug_ext_info->cpuid[DEBUG_POINT_FRAME_END],
				(ulong)time[DEBUG_POINT_FRAME_END],
				usec[DEBUG_POINT_FRAME_END] / NSEC_PER_USEC);
		}
	}
}

void print_all_hw_frame_count(struct is_hardware *hardware)
{
	int hw_slot = -1;
	struct is_hw_ip *_hw_ip = NULL;

	if (!hardware) {
		err_hw("hardware is null\n");
		return;
	}

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		_hw_ip = &hardware->hw_ip[hw_slot];
		print_hw_frame_count(_hw_ip);
	}
}

void is_hardware_flush_frame(struct is_hw_ip *hw_ip,
	enum is_frame_state state,
	enum ShotErrorType done_type)
{
	int ret = 0;
	struct is_framemgr *framemgr;
	struct is_frame *frame;
	ulong flags = 0;
	int retry;

	FIMC_BUG_VOID(!hw_ip);

	framemgr = hw_ip->framemgr;

	framemgr_e_barrier_irqs(framemgr, 0, flags);
	while (state <  FS_HW_WAIT_DONE) {
		frame = peek_frame(framemgr, state);
		while (frame) {
			trans_frame(framemgr, frame, state + 1);
			frame = peek_frame(framemgr, state);
		}
		state++;
	}
	frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
	framemgr_x_barrier_irqr(framemgr, 0, flags);

	retry = IS_MAX_HW_FRAME;
	while (frame && retry--) {
		if (done_type == IS_SHOT_TIMEOUT)
			mserr_hw("[F:%d]hardware is timeout", frame->instance, hw_ip, frame->fcount);

		ret = is_hardware_frame_ndone(hw_ip, frame, done_type);
		if (ret)
			mserr_hw("%s: hardware_frame_ndone fail",
				atomic_read(&hw_ip->instance), hw_ip, __func__);

		framemgr_e_barrier_irqs(framemgr, 0, flags);
		frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
		framemgr_x_barrier_irqr(framemgr, 0, flags);
	}

	if (retry == 0)
		err_hw("frame flush is not completed");
}

static void __update_hw_slot_id_2nr(struct is_frame *hw_frame, struct is_hardware *hardware)
{
	int i;
	struct is_hw_ip *hw_ip;

	if (!hardware->chain_info_ops || !hardware->chain_info_ops->get_dynamic_hw_slot)
		return;

	for (i = 0; i < HW_SLOT_MAX; i++) {
		if (hw_frame->hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[hw_frame->hw_slot_id[i]];

		hw_frame->hw_slot_id[i] =
			CALL_HW_CHAIN_INFO_OPS(hardware, get_dynamic_hw_slot, hw_ip->id, HW_SLOT_SCENARIO_2NR);

		if (hw_frame->hw_slot_id[i] >= HW_SLOT_MAX)
			mdbg_hw(1, "Skip hw_ip name(%s), id(%d), i(%d), hw_slot_id(%d)",
					hw_frame->instance,
					hw_ip->name, hw_ip->id, i, hw_frame->hw_slot_id[i]);

	}
}

#if IS_ENABLED(ENABLE_RECURSIVE_NR)
#define IS_RNR_2ND(shot_ext)                                                                      \
	(shot_ext && (shot_ext)->node_group.leader.recursiveNrType == NODE_RECURSIVE_NR_TYPE_2ND)
#else
#define IS_RNR_2ND(shot_ext)	false
#endif

void is_hardware_fill_frame_info(u32 instance,
	struct is_frame *hw_frame,
	struct is_frame *frame,
	struct is_hardware *hardware,
	bool reset)
{
	int i;
	struct is_hw_ip *hw_ip;

	memcpy(hw_frame->hw_slot_id, frame->hw_slot_id, sizeof(frame->hw_slot_id));
	hw_frame->group		= frame->group;
	hw_frame->shot_ext	= frame->shot_ext;
	hw_frame->shot		= frame->shot;
	hw_frame->shot_dva	= frame->shot_dva;
	hw_frame->shot_size	= frame->shot_size;
	hw_frame->vbuf		= frame->vbuf;
	hw_frame->fcount	= frame->fcount;
	hw_frame->rcount	= frame->rcount;
	hw_frame->core_flag	= 0;
	hw_frame->result	= 0;
	hw_frame->parameter	= frame->parameter;
	IS_COPY_PMAP(hw_frame->pmap, frame->pmap);
	memcpy(hw_frame->dvaddr_buffer, frame->dvaddr_buffer, sizeof(frame->dvaddr_buffer));
	memcpy(hw_frame->kvaddr_buffer, frame->kvaddr_buffer, sizeof(frame->kvaddr_buffer));
	memcpy(&hw_frame->stripe_info, &frame->stripe_info, sizeof(frame->stripe_info));

	hw_frame->ldr_hw_slot_id = hw_frame->hw_slot_id[0];
	if (IS_RNR_2ND(hw_frame->shot_ext))
		__update_hw_slot_id_2nr(hw_frame, hardware);

	for (i = 0; i < HW_SLOT_MAX; i++) {
		if (frame->hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[frame->hw_slot_id[i]];
		is_hw_fill_target_address(hw_ip->id, hw_frame, frame, reset);
	}

	hw_frame->instance = instance;
}

int is_hardware_set_param(struct is_hardware *hardware, u32 instance,
	struct is_region *region, IS_DECLARE_PMAP(pmap), ulong hw_map)
{
	int ret = 0;
	int hw_slot = -1;
	struct is_hw_ip *hw_ip = NULL;

	FIMC_BUG(!hardware);
	FIMC_BUG(!region);

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];

		if (!test_bit_variables(hw_ip->id, &hw_map))
			continue;

		ret = CALL_HWIP_OPS(hw_ip, set_param, region, pmap, instance, hw_map);
		if (ret) {
			mserr_hw("set_param fail (%d)", instance, hw_ip, hw_slot);
			return -EINVAL;
		}
	}

	msdbg_hw(1, "set_param hw_map[0x%lx]\n", instance, hw_ip, hw_map);

	return ret;
}
PST_EXPORT_SYMBOL(is_hardware_set_param);

IS_TIMER_FUNC(is_hardware_shot_timer)
{
	struct is_hw_ip *hw_ip = from_timer(hw_ip, (struct timer_list *)data, shot_timer);
	u32 instance;
	struct is_group *group, *child;
	struct is_hw_ip *hw_ip_chd;
	struct is_hardware *hardware;
	int hw_list[GROUP_HW_MAX], hw_maxnum, hw_index;
	int f_index;
	struct hw_debug_info *debug_info;
	unsigned long long start, end, otf_end, dma_end, shot;
	bool timeout = false;

	instance = atomic_read(&hw_ip->instance);
	group = hw_ip->group[instance];
	if (!group) {
		mserr_hw("There is no group.", instance, hw_ip);
		timeout = true;
		goto flush_frame;
	}

	child = group->tail;

	hardware = hw_ip->hardware;

	while (child && (child->device_type == IS_DEVICE_ISCHAIN)) {
		hw_maxnum = is_get_hw_list(child->id, hw_list);
		for (hw_index = hw_maxnum - 1; hw_index >= 0; hw_index--) {
			hw_ip_chd = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_list[hw_index]);
			if (!hw_ip_chd) {
				merr_hw("invalid id (%d)", instance, hw_list[hw_index]);
				timeout = true;
				goto flush_frame;
			}

			for (f_index = 0; f_index < DEBUG_FRAME_COUNT; f_index++) {
				debug_info = &hw_ip_chd->debug_info[f_index];

				shot = debug_info->time[DEBUG_POINT_HW_SHOT_E];
				start = debug_info->time[DEBUG_POINT_FRAME_START];
				otf_end = debug_info->time[DEBUG_POINT_FRAME_END];
				dma_end = debug_info->time[DEBUG_POINT_FRAME_DMA_END];
				end = max(otf_end, dma_end);

				if (time_after(jiffies, hw_ip->shot_timer.expires)) {
					if (start > end) {
						msinfo_hw("[F:%d] timeout: start_time > end_time", instance, hw_ip_chd,
								debug_info->fcount);
						timeout = true;
					} else if (shot > start) {
						msinfo_hw("[F:%d] timeout: shot_time > start_time", instance, hw_ip_chd,
								debug_info->fcount);
						timeout = true;
					}
				}
			}
		}

		child = child->parent;
	}

flush_frame:

	if (timeout) {
		print_all_hw_frame_count(hw_ip->hardware);
		is_hardware_flush_frame(hw_ip, FS_HW_REQUEST, IS_SHOT_TIMEOUT);
	} else {
		msinfo_hw(" false alarm timeout", instance, hw_ip);
		print_all_hw_frame_count(hw_ip->hardware);
	}
}

static inline void is_hardware_suspend(struct is_hardware *hardware,
		struct is_group *group)
{
	struct is_hw_ip *hw_ip;
	int hw_list[GROUP_HW_MAX], hw_index, hw_num;
	u32 instance;

	while (group) {
		hw_num = is_get_hw_list(group->id, hw_list);
		instance = group->instance;

		for (hw_index = 0; hw_index < hw_num; hw_index++) {
			hw_ip = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_list[hw_index]);
			if (!hw_ip) {
				merr_hw("invalid id (%d)", instance, hw_list[hw_index]);
				continue;
			}

			CALL_HWIP_OPS(hw_ip, suspend, instance);

			del_timer(&hw_ip->shot_timer);
		}

		group = group->child;
	}
}

static inline int _is_hardware_shot(struct is_hardware *hardware,
	struct is_group *group, struct is_frame *frame,
	struct is_framemgr *framemgr, u32 framenum)
{
	int ret = 0, i, j;
	struct is_hw_ip *hw_ip, *ldr_hw_ip;
	struct is_group *head = NULL;
	ulong hw_map;
	u32 instance, fcount;
	struct pablo_crta_bufmgr *crta_bufmgr;
	struct pablo_crta_buf_info pcfi_buf = { 0, };
	struct is_priv_buf *pb;

	head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);
	instance = group->instance;
	fcount = frame->fcount;

	ldr_hw_ip = is_get_hw_ip(head->id, hardware);
	if (!ldr_hw_ip) {
		mgerr("invalid ldr_hw_ip", head, head);
		return -EINVAL;
	}

	if (CALL_HW_CHAIN_INFO_OPS(hardware, check_crta_hw, ldr_hw_ip->id)) {
		crta_bufmgr = pablo_get_crta_bufmgr(PABLO_CRTA_BUF_PCFI, instance, ldr_hw_ip->ch);
		CALL_CRTA_BUFMGR_OPS(crta_bufmgr, get_process_buf, framenum, &pcfi_buf);
	}

	if (!IS_ENABLED(ICPU_IO_COHERENCY)) {
		if (pcfi_buf.frame) {
			pb = pcfi_buf.frame->pb_output;
			CALL_BUFOP(pb, sync_for_cpu, pb, 0, pb->size, DMA_FROM_DEVICE);
		}
	}

	is_dvfs_get_freq(&group->device->resourcemgr->dvfs_ctrl, hardware->dvfs_freq);
	is_hw_update_frame_info(group, frame);

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (group->hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[group->hw_slot_id[i]];

		hw_ip->framemgr = framemgr;
		hw_map = hardware->hw_map[instance];

		if (pcfi_buf.kva)
			is_hw_config(hw_ip, &pcfi_buf);

		if (atomic_read(&hw_ip->instance) != instance
			|| atomic_read(&hw_ip->fcount) < frame->fcount)
			carve_hw_shot_info(hw_ip, frame, instance);

		if (!test_and_set_bit(HW_OTF, &hw_ip->state))
			msinfo_hw("[F%d] Set OTF mode", instance, hw_ip, fcount);

		if (hardware->fake_otf_frame_start)
			hardware->fake_otf_frame_start(hw_ip, instance);
		else
			ret = CALL_HWIP_OPS(hw_ip, shot, frame, hw_map);
		_is_hw_frame_dbg_trace(hw_ip, fcount, DEBUG_POINT_HW_SHOT_X);
		if (ret) {
			mserr_hw("shot fail [F:%d]", instance, hw_ip,
				frame->fcount);
			goto shot_err_cancel;
		}
	}

	mdbg_adt(1, "[F%d(SF%d)][G:%s][H:%s][S:%s]hw_shot [C:0x%lx]\n", instance,
			fcount, atomic_read(&head->head->sensor_fcount),
			group_id_name[head->id], ldr_hw_ip->name,
			hw_frame_state_name[frame->state],
			frame->core_flag);

	if (pcfi_buf.kva)
		CALL_CRTA_BUFMGR_OPS(crta_bufmgr, put_buf, &pcfi_buf);

	return ret;

shot_err_cancel:
	mswarn_hw("[F:%d] Canceled by hardware shot err", instance, ldr_hw_ip, fcount);

	ret = is_hardware_frame_ndone(hw_ip, frame, IS_SHOT_UNPROCESSED);
	if (ret)
		mserr_hw("failed to process as NDONE", instance, hw_ip);

	for (j = HW_SLOT_MAX - 1; j > i; j--) {
		if (group->hw_slot_id[j] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[group->hw_slot_id[j]];

		if (CALL_HWIP_OPS(hw_ip, restore, instance))
			mserr_hw("reset & restore fail", instance, hw_ip);
	}

	return ret;
}

static void is_hardware_update_sensor_info(struct is_group *grp, struct is_frame *frame)
{
	struct is_device_sensor *sensor = grp->device->sensor;
	ulong rms_crop_state = sensor->rms_crop_state;
	int b_ratio;
	u32 sensor_w, sensor_h;
	u32 rms_crop_ofs_x, rms_crop_ofs_y;

	if (test_bit(IS_SENSOR_RMS_CROP_OFF, &rms_crop_state)) {
		frame->sensor_rms_crop_ratio = 0;
		return;
	} else {
		get_remosaic_zoom_ratio(is_sensor_get_sensor_interface(sensor), frame->fcount,
					&frame->sensor_rms_crop_ratio);
	}

	b_ratio = is_sensor_g_updated_bratio(sensor, frame->fcount);

	dbg_hw(2, "[%s][F%d]%s: binning %d -> %d\n",
	       group_id_name[grp->id], frame->fcount, __func__,
	       frame->shot->udm.frame_info.sensor_binning[0], b_ratio);

	sensor_w = frame->shot->udm.frame_info.sensor_size[0] * 1000 / b_ratio;
	sensor_h = frame->shot->udm.frame_info.sensor_size[1] * 1000 / b_ratio;
	rms_crop_ofs_x = (sensor_w - frame->shot->udm.frame_info.sensor_crop_region[2]) >> 1;
	rms_crop_ofs_y = (sensor_h - frame->shot->udm.frame_info.sensor_crop_region[3]) >> 1;
	rms_crop_ofs_x = ALIGN_DOWN(rms_crop_ofs_x, 2);
	rms_crop_ofs_y = ALIGN_DOWN(rms_crop_ofs_y, 2);

	frame->shot->udm.frame_info.sensor_binning[0] = b_ratio;
	frame->shot->udm.frame_info.sensor_binning[1] = b_ratio;
	frame->shot->udm.frame_info.sensor_crop_region[0] = rms_crop_ofs_x;
	frame->shot->udm.frame_info.sensor_crop_region[1] = rms_crop_ofs_y;

	dbg_hw(2, "[%s][F%d]%s: cal_size %dx%d binning %d bns %d crop %d,%d %dx%d\n",
	       group_id_name[grp->id], frame->fcount, __func__,
	       frame->shot->udm.frame_info.sensor_size[0],
	       frame->shot->udm.frame_info.sensor_size[1],
	       frame->shot->udm.frame_info.sensor_binning[0],
	       frame->shot->udm.frame_info.bns_binning[0],
	       frame->shot->udm.frame_info.sensor_crop_region[0],
	       frame->shot->udm.frame_info.sensor_crop_region[1],
	       frame->shot->udm.frame_info.sensor_crop_region[2],
	       frame->shot->udm.frame_info.sensor_crop_region[3]);
}

int is_hardware_shot_prepare(struct is_hardware *hardware,
	struct is_group *group, struct is_frame *frame)
{
	int ret, i;
	u32 instance, fcount;
	struct pablo_icpu_adt *icpu_adt;
	struct camera2_shot *shot;
	struct pablo_crta_bufmgr *crta_bufmgr;
	struct pablo_crta_buf_info shot_buf = { 0, }, pcfi_buf = { 0, };
	struct is_group *head;
	struct is_hw_ip *head_hw_ip, *hw_ip;

	icpu_adt = hardware->icpu_adt;
	shot = frame->shot;
	instance = group->instance;
	fcount = frame->fcount;

	head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);
	head_hw_ip = is_get_hw_ip(head->id, hardware);
	if (!head_hw_ip) {
		mgerr("invalid head_hw_ip", head, head);
		return -EINVAL;
	}

	if (!test_bit(IS_GROUP_USE_MULTI_CH, &head->state) &&
		frame->type == SHOT_TYPE_EXTERNAL)
		is_hardware_update_sensor_info(head, frame);

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (group->hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[group->hw_slot_id[i]];

		carve_hw_shot_info(hw_ip, frame, instance);
	}

	crta_bufmgr = pablo_get_crta_bufmgr(PABLO_CRTA_BUF_PCFI, instance, head_hw_ip->ch);
	ret = CALL_CRTA_BUFMGR_OPS(crta_bufmgr, get_free_buf, fcount, true, &pcfi_buf);
	if (!ret)
		is_hw_update_pcfi(hardware, head, frame, &pcfi_buf);
	else
		merr_adt("[F%d][PCFI]failed to get_free_buf", instance, fcount);

	mdbg_adt(1, "[F%d][G:%s][S:%s]shot_prepare [C:0x%lx]\n", instance,
			frame->fcount, group_id_name[group->id],
			hw_frame_state_name[frame->state],
			frame->core_flag);

	/* get shot va */
	shot_buf.kva = shot;
	shot_buf.dva = frame->shot_dva;

	if (!test_bit(IS_GROUP_RNR_2ND, &group->state))
		CALL_ADT_MSG_OPS(icpu_adt, send_msg_shot, instance, (void *)group, (void *)frame,
			fcount, &shot_buf, &pcfi_buf);
	_is_hw_frame_dbg_trace(head_hw_ip, fcount, DEBUG_POINT_SHOT_MSG);

	return 0;
}

static int _is_hardware_shot_callback(void *user, void *ctx, void *rsp_msg)
{
	int ret;
	u32 framenum, instance, ss_fcount;
	struct pablo_icpu_adt_rsp_msg *msg;
	struct is_hardware *hardware;
	struct is_hw_ip *head_hw_ip, *hw_ip;
	struct is_frame *frame;
	struct is_group *group, *head;
	struct is_framemgr *framemgr;
	struct is_device_sensor *sensor;

	if (!user || !ctx || !rsp_msg) {
		err_hw("invalid callback: user(%p), ctx(%p), msg(%p)",
			user, ctx, rsp_msg);
		return -EINVAL;
	}

	msg = (struct pablo_icpu_adt_rsp_msg *)rsp_msg;
	frame = (struct is_frame *)ctx;

	if (msg->rsp)
		merr_hw("shot fail from icpu: msg_ret(%d)", msg->instance, msg->rsp);

	group = (struct is_group *)user;
	if (!group) {
		merr_hw("[F%d]group is null", msg->instance, msg->fcount);
		return -EINVAL;
	}

	hardware = &is_get_is_core()->hardware;
	instance = group->instance;
	framenum = frame->fcount;
	sensor = group->device->sensor;
	ss_fcount = sensor->fcount;

	if ((msg->fcount != framenum) || (msg->instance != instance)) {
		merr_hw("fcount, instance is not matched(S%d!=S%d, F%d!=F%d)", instance,
			msg->instance, instance, msg->fcount, framenum);
		return -EINVAL;
	}

	head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);
	head_hw_ip = hw_ip = &hardware->hw_ip[group->hw_slot_id[0]];

	if (!head_hw_ip || !test_bit(instance, &head_hw_ip->run_rsc_state)) {
		merr_hw("[F%d]ignore shot_callback after process stop", instance, framenum);
		return -EINVAL;
	}

	/* Multi channel stream refers the framemgr of main stream */
	if (test_bit(IS_GROUP_USE_MULTI_CH, &group->state)) {
		head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group->head->tail);
		hw_ip = is_get_hw_ip(head->id, hardware);
	}

	/* Drop frame when shot_callback has been delayed too much compared to sensor fcount. */
	if (test_bit(IS_GROUP_OTF_INPUT, &head->state)) {
		if (frame->state == FS_HW_FREE || frame->fcount < (ss_fcount + 1)) {
			mswarn_hw("[F%d] LATE_SHOT_CALLBACK [S:%s][C:0x%lx]", instance, head_hw_ip,
				framenum, hw_frame_state_name[frame->state], frame->core_flag);
			frame->result = IS_SHOT_CONFIG_LOCK_DELAY;
			return 0;
		}
	}

	framemgr = &hardware->framemgr[hw_ip->id];

	_is_hw_frame_dbg_trace(head_hw_ip, framenum, DEBUG_POINT_SHOT_CALLBACK);

	mdbg_adt(1, "[F%d][G:%s][S:%s]shot_callback [C:0x%lx]\n", instance,
			framenum, group_id_name[group->id],
			hw_frame_state_name[frame->state],
			frame->core_flag);

	ret = _is_hardware_shot(hardware, group, frame, framemgr, framenum);
	if (ret) {
		err_hw("_is_hardware_shot fail");
		return ret;
	}

	return ret;

}

static inline int is_hardware_shot(struct is_hardware *hardware, u32 instance,
	struct is_group *group, struct is_frame *frame,
	struct is_framemgr *framemgr, u32 framenum)
{
	struct is_device_sensor *sensor;
	struct is_group *pnext;
	struct is_hw_ip *hw_ip;
	int ret = 0;
	ulong flags = 0;

	FIMC_BUG(!hardware);
	FIMC_BUG(!frame);

	sensor = group->device->sensor;
	pnext = group->head->pnext;

	framemgr_e_barrier_common(framemgr, 0, flags);
	if (!test_bit(IS_GROUP_USE_MULTI_CH, &group->state))
		trans_frame(framemgr, frame, FS_HW_CONFIGURE);
	framemgr_x_barrier_common(framemgr, 0, flags);

	if (test_and_clear_bit(IS_SKIP_CHAIN_SHOT, &sensor->aeb_state) &&
	    test_bit(IS_GROUP_USE_MULTI_CH, &group->state)) {
		/* Trigger HW be in IDLE state because there will be no more shot */
		is_hardware_suspend(hardware, group);
		goto skip_chain_shot;
	} else if (test_and_clear_bit(IS_DO_SHORT_CHAIN_S_PARAM, &sensor->aeb_state)) {
		/* Set initial parameter for short stream. */
		if (CALL_HW_GRP_OPS(pnext, s_param, hardware, pnext->instance, pnext, frame)) {
			mgerr("[F%d]grp_s_param fail", pnext, pnext, frame->fcount);
			return -EINVAL;
		}
	}

	hw_ip = is_get_hw_ip(group->id, hardware);
	if (CALL_HW_CHAIN_INFO_OPS(hardware, check_crta_hw, hw_ip->id)) {
		is_hardware_shot_prepare(hardware, group, frame);

		if (test_and_clear_bit(IS_DO_SHORT_CHAIN_SHOT, &sensor->aeb_state)) {
			/* Generate init_shot without CRTA shot_callback to do initial HW congirutaion. */
			_is_hardware_shot(hardware, pnext, frame, framemgr, framenum);
			is_hardware_shot_prepare(hardware, pnext, frame);
		}

		goto skip_chain_shot;
	}

	ret = _is_hardware_shot(hardware, group, frame, framemgr, framenum);
	if (ret) {
		err_hw("_is_hardware_shot fail");
		return ret;
	}

	if (test_and_clear_bit(IS_DO_SHORT_CHAIN_SHOT, &sensor->aeb_state)) {
		ret = _is_hardware_shot(hardware, group->head->pnext, frame, framemgr, framenum);
		if (ret) {
			err_hw("_is_hardware_shot fail");
			return ret;
		}
	}

skip_chain_shot:
	if (test_and_clear_bit(IS_SKIP_SENSOR_SHOT, &sensor->aeb_state))
		goto skip_sensor_shot;
	/*
	 * do the other device's group shot
	 * In case of VOTF, a user buffer should be set later than internal buffer.
	 * So, shot_callback of sensor group should be called after calling shot for PDP-3AA.
	 */
	ret = is_devicemgr_shot_callback(group, frame, frame->fcount, IS_DEVICE_ISCHAIN);
	if (ret) {
		err_hw("[F%d] is_devicemgr_shot_callback fail", frame->fcount);
		return -EINVAL;
	}

skip_sensor_shot:
	return ret;
}

static inline int is_hardware_get_meta(struct is_hw_ip *hw_ip, struct is_frame *frame,
	u32 instance, ulong hw_map)
{
	int ret;

	ret = CALL_HWIP_OPS(hw_ip, get_meta, frame, hw_map);
	if (ret) {
		mserr_hw("[F:%d] get_meta fail", instance, hw_ip, frame->fcount);
		return 0;
	}

	switch (hw_ip->id) {
	case DEV_HW_3AA0:
	case DEV_HW_3AA1:
	case DEV_HW_3AA2:
	case DEV_HW_3AA3:
		is_hw_mcsc_set_ni(hw_ip->hardware, frame, instance);
		break;
	default:
		break;
	}

	msdbg_hw(1, "[F:%d]get_meta\n", instance, hw_ip, frame->fcount);

	return 0;
}

int check_shot_exist(struct is_framemgr *framemgr, u32 fcount, struct is_frame **frame)
{
	if (framemgr->queued_count[FS_HW_WAIT_DONE]) {
		*frame = find_frame(framemgr, FS_HW_WAIT_DONE, frame_fcount,
					(void *)(ulong)fcount);
		if (*frame) {
			if ((*frame)->type == SHOT_TYPE_INTERNAL)
				info_hw("[F:%d]is in complete_list\n", fcount);
			return INTERNAL_SHOT_EXIST;
		}
	}

	if (framemgr->queued_count[FS_HW_CONFIGURE]) {
		*frame = find_frame(framemgr, FS_HW_CONFIGURE, frame_fcount,
					(void *)(ulong)fcount);
		if (*frame) {
			if ((*frame)->type == SHOT_TYPE_INTERNAL)
				info_hw("[F:%d]is in process_list\n", fcount);
			return INTERNAL_SHOT_EXIST;
		}
	}

	return 0;
}

void is_set_hw_count(struct is_hardware *hardware, u32 *hw_slot_id, u32 instance, u32 fcount)
{
	struct is_hw_ip *hw_ip;
	u32 fs, cl, fe, dma;
	int i;

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[hw_slot_id[i]];

		fs = atomic_read(&hw_ip->count.fs);
		cl = atomic_read(&hw_ip->count.cl);
		fe = atomic_read(&hw_ip->count.fe);
		dma = atomic_read(&hw_ip->count.dma);
		atomic_set(&hw_ip->count.fs, (fcount - 1));
		atomic_set(&hw_ip->count.cl, (fcount - 1));
		atomic_set(&hw_ip->count.fe, (fcount - 1));
		atomic_set(&hw_ip->count.dma, (fcount - 1));
		msdbg_hw(1, "[F:%d]count clear, fs(%d->%d), fe(%d->%d), dma(%d->%d)\n",
				instance, hw_ip, fcount,
				fs, atomic_read(&hw_ip->count.fs),
				fe, atomic_read(&hw_ip->count.fe),
				dma, atomic_read(&hw_ip->count.dma));
	}
}

static inline int is_hw_otf_grp_set_hw_ops(struct is_hardware *hardware, struct is_frame *frame)
{
	int i;
	struct is_hw_ip *hw_ip;

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (frame->hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[frame->hw_slot_id[i]];
		hw_ip->hw_ops = &is_hardware_ops;
	}

	return 0;
}

static int is_hw_otf_grp_shot(struct is_hardware *hardware, u32 instance, struct is_frame *frame)
{
	int ret = 0;
	struct is_device_sensor *sensor;
	struct is_hw_ip *hw_ip;
	struct is_frame *hw_frame;
	struct is_framemgr *framemgr;
	struct is_group *head, *ss_grp;
	ulong flags = 0;
	int num_buffers;
	bool reset;

	FIMC_BUG(!hardware);
	FIMC_BUG(!frame);
	FIMC_BUG(instance >= IS_STREAM_COUNT);

	if (frame->hw_slot_id[0] >= HW_SLOT_MAX) {
		merr_hw("invalid hw_slot_id(%d)", instance, frame->hw_slot_id[0]);
		return -EINVAL;
	}

	is_hw_otf_grp_set_hw_ops(hardware, frame);

	hw_ip = &hardware->hw_ip[frame->hw_slot_id[0]];
	head = hw_ip->group[instance];

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
	hw_ip->hw_fro_en = (num_buffers > 1) ? true : false;

	put_frame(framemgr, hw_frame, FS_HW_REQUEST);

	msdbg_hw(2, "ischain batch_num(%d), HW FRO(%d)\n", instance, hw_ip,
		num_buffers, hw_ip->hw_fro_en);

	ss_grp = head->head;
	sensor = ss_grp->sensor;

	if (!atomic_read(&ss_grp->scount) || hardware->fake_otf_frame_start) {
		hw_frame = peek_frame(framemgr, FS_HW_REQUEST);

		msinfo_hw("OTF start [F:%d]\n", instance, hw_ip, frame->fcount);

		if (is_sensor_g_aeb_mode(sensor))
			sensor->aeb_state = BIT(IS_SENSOR_SINGLE_MODE)
						| BIT(IS_DO_SHORT_CHAIN_S_PARAM);
	} else {
		atomic_set(&hw_ip->hardware->log_count, 0);
		framemgr_x_barrier_irqr(framemgr, 0, flags);

		return ret;
	}

	framemgr_x_barrier_irqr(framemgr, 0, flags);

	is_set_hw_count(hardware, frame->hw_slot_id, instance, frame->fcount);
	ret = is_hardware_shot(hardware, instance, head, hw_frame, framemgr, frame->fcount);
	if (ret) {
		mserr_hw("hardware_shot fail", instance, hw_ip);
		return -EINVAL;
	}

	return ret;
}

static int is_hw_otf_grp_open(struct is_hardware *hardware, u32 instance, u32 *hw_slot_id)
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
		hw_ip->hw_ops = &is_hardware_ops;

		/* Once */
		atomic_inc(&hardware->slot_rsccount[hw_ip->group[instance]->slot]);
	}

	return 0;
}

static int is_hw_otf_grp_close(struct is_hardware *hardware, u32 instance, u32 *hw_slot_id)
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

		/* Once */
		atomic_dec(&hardware->slot_rsccount[hw_ip->group[instance]->slot]);
	}

	return 0;
}

static int is_hw_otf_grp_s_param(struct is_hardware *hardware, u32 instance,
		struct is_group *grp, struct is_frame *frame)
{
	struct is_hw_ip *hw_ip;
	struct is_region *region;
	ulong hw_map = hardware->hw_map[instance];

	mdbg_hw(1, "[F%d]grp_s_param:hw_map 0x%lx\n", instance, frame->fcount, hw_map);

	while (grp) {
		hw_ip = is_get_hw_ip(grp->id, hardware);
		region = hw_ip->region[instance];
		if (!region) {
			mserr_hw("No valid is_region", instance, hw_ip);
			return -EINVAL;
		}

		if (CALL_HWIP_OPS(hw_ip, set_param, region, frame->pmap, instance, hw_map))
			mserr_hw("set_param fail", instance, hw_ip);

		grp = grp->child;
	}

	return 0;
}

static int flush_frames_in_instance(struct is_hw_ip *hw_ip,
	struct is_framemgr *framemgr, u32 instance,
	enum is_frame_state state, enum ShotErrorType done_type)
{
	int retry = 150;
	struct is_frame *frame;
	int ret = 0;
	ulong flags = 0;
	u32 queued_count = 0;

	framemgr_e_barrier_irqs(framemgr, 0, flags);
	queued_count = framemgr->queued_count[state];
	framemgr_x_barrier_irqr(framemgr, 0, flags);

	while (--retry && queued_count) {
		framemgr_e_barrier_irqs(framemgr, 0, flags);
		frame = peek_frame(framemgr, state);
		if (!frame) {
			framemgr_x_barrier_irqr(framemgr, 0, flags);
			break;
		}

		if (frame->instance != instance) {
			msinfo_hw("different instance's frame was detected\n",
				instance, hw_ip);
			info_hw("\t frame's instance: %d, queued count: %d\n",
				frame->instance, framemgr->queued_count[state]);

			/* FIXME: consider mixing frames among instances */
			framemgr_x_barrier_irqr(framemgr, 0, flags);
			break;
		}

		info_hw("frame info: %s(queued count: %d) [F:%d][T:%d][C:0x%lx]",
			hw_frame_state_name[frame->state], framemgr->queued_count[state],
			frame->fcount, frame->type, frame->core_flag);

		/* Core_flag need to be cleared in case,
		 * Other IPs are currently using framemgr of current IP.
		 */
		frame->core_flag = 0;
		set_bit(hw_ip->id, &frame->core_flag);

		framemgr_x_barrier_irqr(framemgr, 0, flags);
		ret = is_hardware_frame_ndone(hw_ip, frame, done_type);
		if (ret) {
			mserr_hw("failed to process as NDONE", instance, hw_ip);
			break;
		}

		framemgr_e_barrier_irqs(framemgr, 0, flags);
		warn_hw("flushed a frame in %s, queued count: %d ",
			hw_frame_state_name[frame->state], framemgr->queued_count[state]);

		queued_count = framemgr->queued_count[state];
		framemgr_x_barrier_irqr(framemgr, 0, flags);

		if (queued_count > 0)
			usleep_range(1000, 1000);
	}

	return ret;
}

static void is_hardware_force_stop(struct is_hardware *hardware,
	struct is_hw_ip *hw_ip, u32 instance)
{
	int ret = 0;
	struct is_framemgr *framemgr;
	enum is_frame_state state;

	FIMC_BUG_VOID(!hw_ip);

	framemgr = hw_ip->framemgr;
	msinfo_hw("frame manager queued count (%s: %d)(%s: %d)(%s: %d)\n",
		instance, hw_ip,
		hw_frame_state_name[FS_HW_WAIT_DONE],
		framemgr->queued_count[FS_HW_WAIT_DONE],
		hw_frame_state_name[FS_HW_CONFIGURE],
		framemgr->queued_count[FS_HW_CONFIGURE],
		hw_frame_state_name[FS_HW_REQUEST],
		framemgr->queued_count[FS_HW_REQUEST]);

	/* reverse order */
	for (state = FS_HW_WAIT_DONE; state > FS_HW_FREE; state--) {
		ret = flush_frames_in_instance(hw_ip, framemgr, instance, state, IS_SHOT_UNPROCESSED);
		if (ret) {
			mserr_hw("failed to flush frames in %s", instance, hw_ip,
				hw_frame_state_name[state]);
			return;
		}
	}
}

static void is_hw_otf_grp_wait_frame(struct is_hw_ip *hw_ip, u32 instance)
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

	state = FS_HW_WAIT_DONE;
	framemgr_e_barrier_common(framemgr, 0, flags);
	frame = peek_frame(framemgr, state);
	framemgr_x_barrier_common(framemgr, 0, flags);
	if (frame && frame->instance != instance) {
		msinfo_hw("frame->instance(%d), queued_count(%s(%d))\n", instance, hw_ip,
			  frame->instance, hw_frame_state_name[state],
			  framemgr->queued_count[state]);
	} else {
		retry = 10;
		while (--retry && framemgr->queued_count[state]) {
			mswarn_hw("%s(%d) com waiting...", instance, hw_ip,
				  hw_frame_state_name[state], framemgr->queued_count[state]);
			usleep_range(5000, 5500);
		}
		if (!retry)
			mswarn_hw("waiting(until frame empty) is fail", instance, hw_ip);
	}
}

static int is_hw_otf_grp_stop(struct is_hardware *hardware, u32 instance, u32 mode, u32 *hw_slot_id)
{
	int ret, i;
	ulong hw_map = hardware->hw_map[instance];
	struct is_hw_ip *hw_ip;
	bool wait_done = false;

	mdbg_hw(1, "otf_grp_stop hw_map[0x%lx] mode(%d)\n", instance, hw_map, mode);

	for (i = HW_SLOT_MAX - 1; i >= 0; i--) {
		if (hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		hw_ip = &hardware->hw_ip[hw_slot_id[i]];

		if (!wait_done) {
			wait_done = true;
			is_hw_otf_grp_wait_frame(hw_ip, instance);
		}

		if (!test_and_clear_bit(instance, &hw_ip->run_rsc_state))
			mswarn_hw("try to disable disabled instance", instance, hw_ip);

		ret = CALL_HWIP_OPS(hw_ip, disable, instance, hw_map);
		if (ret == -EWOULDBLOCK)
			continue;
		else if (ret)
			mserr_hw("disable fail", instance, hw_ip);

		if (!test_bit(HW_RUN, &hw_ip->state)) {
			atomic_set(&hw_ip->fcount, 0);
			del_timer_sync(&hw_ip->shot_timer);
			msinfo_hw("del timer\n", instance, hw_ip);
		}

		if (mode == 0)
			continue;

		is_hardware_force_stop(hardware, hw_ip, instance);
		hw_ip->internal_fcount[instance] = 0;
	}

	return 0;
}

static const struct is_hw_group_ops is_hw_group_ops_otf = {
	.shot = is_hw_otf_grp_shot,
	.open = is_hw_otf_grp_open,
	.close = is_hw_otf_grp_close,
	.stop = is_hw_otf_grp_stop,
	.s_param = is_hw_otf_grp_s_param,
};

const struct is_hw_group_ops *is_hw_get_otf_group_ops(void)
{
	return &is_hw_group_ops_otf;
}
EXPORT_SYMBOL_GPL(is_hw_get_otf_group_ops);

static int make_internal_shot(struct is_hw_ip *hw_ip, u32 instance, u32 fcount,
		struct is_framemgr *framemgr, u32 buf_index)
{
	int ret, i;
	struct is_frame *frame;
	struct is_group *group;

	if (framemgr->queued_count[FS_HW_FREE] < 3) {
		mswarn_hw("Free frame is less than 3", instance, hw_ip);
		frame_manager_print_info_queues(framemgr);
		check_hw_bug_count(hw_ip->hardware, 10);
	}

	ret = check_shot_exist(framemgr, fcount, &frame);
	if (ret == INTERNAL_SHOT_EXIST)
		return ret;

	frame = get_frame(framemgr, FS_HW_FREE);
	if (!frame) {
		merr_hw("config_lock: frame(null)", instance);
		return -EINVAL;
	}

	group = hw_ip->group[instance];
	if (!group) {
		merr_hw("config_lock: group(null)", instance);
		return -EINVAL;
	}

	memcpy(frame->hw_slot_id, group->hw_slot_id, sizeof(frame->hw_slot_id));
	frame->group		= NULL;
	frame->shot_ext		= NULL;
	frame->shot		= NULL;
	frame->shot_size	= 0;
	frame->shot_dva 	= 0;
	frame->vbuf		= NULL;
	frame->fcount		= fcount;
	frame->rcount		= 0;
	frame->core_flag	= 0;
	frame->result		= 0;
	/* multi-buffer */
	frame->planes		= 0;
	if (hw_ip->hw_fro_en)
		frame->num_buffers = hw_ip->num_buffers;
	else
		frame->num_buffers = 1;

	for (i = 0; i < IS_MAX_PLANES; i++)
		frame->dvaddr_buffer[i]	= 0;

	frame->type = SHOT_TYPE_INTERNAL;
	frame->instance = instance;
	frame->cur_buf_index = buf_index;

	put_frame(framemgr, frame, FS_HW_REQUEST);

	return ret;
}

static int is_hardware_config_lock(struct is_hw_ip *hw_ip, u32 instance, u32 framenum)
{
	int ret = 0;
	struct is_frame *frame;
	struct is_framemgr *framemgr;
	struct is_hardware *hardware;
	struct is_device_sensor *sensor;
	u32 sensor_fcount, hw_fcount, shot_fcount;
	struct is_group *group, *head, *dev_head;
	struct is_hw_ip *hw_ip_ldr;
	struct is_device_csi *csi;
	int i;
	struct is_hw_ip *tmp_hw_ip;

	FIMC_BUG(!hw_ip);

	group = hw_ip->group[instance];
	head = group->head;
	dev_head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);

	framemgr = hw_ip->framemgr;
	hardware = hw_ip->hardware;
	sensor = group->device->sensor;
	sensor_fcount = sensor->fcount;
	hw_fcount = atomic_read(&hw_ip->fcount);

	for (i = 0; i < HW_SLOT_MAX; i++) {
		if (group->hw_slot_id[i] >= HW_SLOT_MAX)
			continue;

		tmp_hw_ip = &hardware->hw_ip[group->hw_slot_id[i]];

		CALL_HWIP_OPS(tmp_hw_ip, recover, hw_fcount);
	}

	if (sensor_fcount > hw_fcount)
		shot_fcount = sensor_fcount + 1;
	else
		shot_fcount = hw_fcount + 1;
	csi = (struct is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (csi)
		atomic_set(&csi->chain_fcount, shot_fcount);

	msdbg_hw(1, "C.L [SF:%d][HF:%d]->[F:%d]\n", instance, hw_ip,
		sensor_fcount, hw_fcount, shot_fcount);

	/* leader shot timer set in OTF used HW */
	hw_ip_ldr = is_get_hw_ip(dev_head->id, hw_ip->hardware);
	if (!hw_ip_ldr) {
		mserr_hw("[F%d] Failed to get leader hw_ip", instance, hw_ip, shot_fcount);
		return -EINVAL;
	}

	mod_timer(&hw_ip_ldr->shot_timer,
		  jiffies + msecs_to_jiffies(is_get_shot_timeout(&is_get_is_core()->resourcemgr)));

retry_get_frame:
	framemgr_e_barrier(framemgr, 0);

	/* Sub stream context for 2EXP_MODE */
	if (hw_ip->frame_type == LIB_FRAME_HDR_SHORT) {
		/* Re-use main stream frame that has been processed right before */
		frame = peek_frame_tail(framemgr, FS_HW_CONFIGURE);
		if (!frame) {
			framemgr_x_barrier(framemgr, 0);
			mserr_hw("[F%d] Failed to get main frame. aeb_state 0x%lx",
					instance, hw_ip, shot_fcount,
					sensor->aeb_state);
			return -EINVAL;
		}

		goto shot_exist_frame;
	}

	if (!framemgr->queued_count[FS_HW_REQUEST]) {
		u32 num_buffers = hw_ip_ldr->hw_fro_en ? 1 : framemgr->batch_num;
		u32 buf_index;

		/* There is no request. Generate internal shot. */
		for (buf_index = 0; buf_index < num_buffers; buf_index++) {
			ret = make_internal_shot(hw_ip_ldr, instance, shot_fcount, framemgr,
						buf_index);
			if (ret == INTERNAL_SHOT_EXIST) {
				framemgr_x_barrier(framemgr, 0);

				return ret;
			} else if (ret) {
				framemgr_x_barrier(framemgr, 0);
				print_all_hw_frame_count(hardware);

				FIMC_BUG(1);
			}
		}
	}

	frame = peek_frame(framemgr, FS_HW_REQUEST);
	if (!frame) {
		framemgr_x_barrier(framemgr, 0);
		mserr_hw("frame is null", instance, hw_ip);

		return -EINVAL;
	} else if (frame->type == SHOT_TYPE_INTERNAL) {
		u32 log_count = atomic_read(&hardware->log_count);

		if ((log_count <= 20) || !(log_count % 100))
			msinfo_hw("config_lock: INTERNAL_SHOT [F:%d](%d) count(%d)\n",
					instance, hw_ip,
					frame->fcount, frame->index, log_count);
	} else if (frame->fcount < shot_fcount) {
		/* It's too old frame. Flush it */
		msinfo_hw("LATE_SHOT (%d)[F:%d][SF:%d][HF:%d]->[F:%d][C:0x%lx]\n",
				instance, hw_ip,
				hw_ip->internal_fcount[instance], frame->fcount,
				sensor_fcount, hw_fcount, shot_fcount,
				frame->core_flag);
		frame->type = SHOT_TYPE_LATE;
		frame->result = IS_SHOT_LATE_FRAME;
#if !IS_ENABLED(CONFIG_SENSOR_GROUP_LVN)
		is_devicemgr_late_shot_handle(&sensor->group_sensor, frame, frame->type);
#endif
		trans_frame(framemgr, frame, FS_HW_WAIT_DONE);
		framemgr_x_barrier(framemgr, 0);

		goto retry_get_frame;
	}

	frame->frame_info[INFO_CONFIG_LOCK].cpu = raw_smp_processor_id();
	frame->frame_info[INFO_CONFIG_LOCK].pid = current->pid;
	frame->frame_info[INFO_CONFIG_LOCK].when = local_clock();

shot_exist_frame:
	framemgr_x_barrier(framemgr, 0);

	ret = is_hardware_shot(hardware, instance, dev_head, frame, framemgr, frame->fcount);
	if (ret) {
		mserr_hw("hardware_shot fail", instance, hw_ip);
		return -EINVAL;
	}

	return ret;
}

struct is_hw_ip *is_get_hw_ip(u32 group_id, struct is_hardware *hardware)
{
	int hw_list[GROUP_HW_MAX];
	int hw_maxnum = 0;

	hw_maxnum = is_get_hw_list(group_id, hw_list);

	return CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_list[0]);
}

static struct is_frame *find_start_frame(struct is_hw_ip *hw_ip, struct is_framemgr *framemgr,
					 u32 instance, u32 hw_fcount)
{
	struct is_frame *frame, *next;
	struct list_head *head;

	/* Find frame in WAIT_DONE state, that has core_flag of current hw_ip, first. */
	head = &framemgr->queued_list[FS_HW_WAIT_DONE];
	list_for_each_entry_safe(frame, next, head, list) {
		if (test_bit_variables(hw_ip->id, &frame->core_flag))
			goto exit;
	}

	/* Get next CONFIGURE frame */
	head = &framemgr->queued_list[FS_HW_CONFIGURE];
	list_for_each_entry_safe(frame, next, head, list) {
		if (framemgr->queued_count[FS_HW_CONFIGURE] > 1 && frame->fcount < hw_fcount) {
			/* Flush old CONFIGURE frames */
			msinfo_hw("[F%d][HWF%d][CF%d]flush CONFIGURE frame\n", instance, hw_ip,
				  frame->fcount, hw_fcount,
				  framemgr->queued_count[FS_HW_CONFIGURE]);

			frame_manager_print_info_queues(framemgr);

			trans_frame(framemgr, frame, FS_HW_WAIT_DONE);
		} else if (frame->fcount <= hw_fcount) {
			break;
		}
	}

	/* Return NULL when current frame is invalid */
	if (list_entry_is_head(frame, head, list) ||
	    !test_bit_variables(hw_ip->id, &frame->core_flag))
		frame = NULL;

exit:
	return frame;
}

static inline void is_hardware_aeb_change(struct is_group *group, u32 fcount)
{
	struct is_device_sensor *sensor;
	u32 cur_hdr_mode;
	ulong prev_state;

	sensor = group->device->sensor;
	if (!is_sensor_g_aeb_mode(sensor))
		return;

	/* Read HDR mode from sensor module */
	cur_hdr_mode = is_sensor_g_hdr_mode(sensor, fcount,
					test_bit(IS_GROUP_USE_MULTI_CH, &group->state));

	prev_state = sensor->aeb_state;

	/* Keep AEB_ERR state until sensor turns off AEB mode. */
	if (test_bit(IS_SENSOR_AEB_ERR, &sensor->aeb_state)) {
		if (cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC)
			goto skip_transition;
		else
			clear_bit(IS_SENSOR_AEB_ERR, &sensor->aeb_state);
	}

	/* IS-chain shot control for AEB mode */
	if (!test_bit(IS_GROUP_USE_MULTI_CH, &group->state)) {
		/* Long frame shot context */
		if (test_bit(IS_SENSOR_SINGLE_MODE, &sensor->aeb_state)) {
			if (cur_hdr_mode == SENSOR_HDR_MODE_2AEB_2VC) {
				if (!test_bit(IS_SENSOR_AEB_SWITCHING, &sensor->aeb_state))
					/* 1st frame: Wait one more long shot */
					sensor->aeb_state |= BIT(IS_SENSOR_AEB_SWITCHING) |
							     BIT(IS_DO_SHORT_CHAIN_SHOT) |
							     BIT(IS_DO_SENSOR_OTF_OUT_CFG);
				else
					/* 2nd frame: Skip sensor shot */
					set_bit(IS_SKIP_SENSOR_SHOT, &sensor->aeb_state);
			}
		} else if (test_bit(IS_SENSOR_2EXP_MODE, &sensor->aeb_state)) {
			if (test_and_clear_bit(IS_SENSOR_AEB_SWITCHING, &sensor->aeb_state)) {
				/*
				 * Long frame shot after turning off AEB mode
				 * It's now on SINGLE mode
				 */
				sensor->aeb_state &= ~IS_SENSOR_AEB_MODE_MASK;
				sensor->aeb_state |= BIT(IS_SENSOR_SINGLE_MODE);
			} else if (cur_hdr_mode == SENSOR_HDR_MODE_SINGLE) {
				/*
				 * Long frame shot to turn off AEB mode
				 * Wait next short shot
				 */
				sensor->aeb_state |= BIT(IS_SENSOR_AEB_SWITCHING)
							| BIT(IS_DO_SENSOR_OTF_OUT_CFG);
			}

			/*
			 * Sensor shot control for AEB mode
			 * Long shot should not trigger sensor shot on AEB on
			 */
			if (!test_bit(IS_SENSOR_SINGLE_MODE, &sensor->aeb_state))
				set_bit(IS_SKIP_SENSOR_SHOT, &sensor->aeb_state);

		}
	} else {
		/* Short frame shot context */
		if (test_bit(IS_SENSOR_2EXP_MODE, &sensor->aeb_state)) {
			if (test_bit(IS_SENSOR_AEB_SWITCHING, &sensor->aeb_state) ||
					cur_hdr_mode == SENSOR_HDR_MODE_SINGLE)
				/*
				 * Short frame to turn off AEB mode
				 * Skip next Short frame shot
				 */
				sensor->aeb_state |= BIT(IS_SENSOR_AEB_SWITCHING)
							| BIT(IS_SKIP_CHAIN_SHOT)
							| BIT(IS_DO_SENSOR_OTF_OUT_CFG);
		} else {
			if (test_and_clear_bit(IS_SENSOR_AEB_SWITCHING, &sensor->aeb_state)) {
				/**
				 * 1st short frame shot to enter AEB mode
				 * Move to AEB mode.
				 */
				sensor->aeb_state &= ~IS_SENSOR_AEB_MODE_MASK;
				sensor->aeb_state |= BIT(IS_SENSOR_2EXP_MODE);
			} else {
				/**
				 * Short frame shot conext in SINGLE mode
				 * This is abnormal sensor operation! Restore the AEB modes.
				 *  1. Return to SINGLE mode.
				 *  2. Don't trigger any additional shot for short stream.
				 *  3. Reset the sensor OTF out mode
				 */
				mgerr("[F%d] Invalid sensor output on SINGLE mode. aeb_state 0x%lx",
						group, group, fcount, sensor->aeb_state);

				sensor->aeb_state &= ~IS_SENSOR_AEB_MODE_MASK;
				sensor->aeb_state |= IS_SENSOR_AEB_INVALID_SHORT_OUT_MASK;
			}
		}
	}

skip_transition:
	dbg_hw(2, "[%s][F%d]%s: %s_shot cur_hdr_mode %d aeb_state 0x%lx\n",
			group_id_name[group->id],
			fcount,
			__func__,
			test_bit(IS_GROUP_USE_MULTI_CH, &group->state) ? "SHORT" : "LONG",
			cur_hdr_mode,
			sensor->aeb_state);

	if ((prev_state ^ sensor->aeb_state) & IS_SENSOR_AEB_MODE_MASK)
		mginfo("[F%d] Change aeb_state 0x%lx -> 0x%lx\n",
				group, group, fcount,
				prev_state, sensor->aeb_state);
}

static inline void is_hardware_rms_crop_state_change(struct is_group *grp, u32 fcount)
{
	struct is_device_sensor *sensor = grp->device->sensor;
	ulong prev_state = sensor->rms_crop_state;
	int b_ratio = is_sensor_g_bratio(sensor);
	int new_b_ratio = is_sensor_g_updated_bratio(sensor, fcount);

	/* For now, SWITCHING state is only required to print state transition log. */
	if (test_bit(IS_SENSOR_RMS_CROP_SWITCHING, &prev_state))
		clear_bit(IS_SENSOR_RMS_CROP_SWITCHING, &sensor->rms_crop_state);

	if (test_bit(IS_SENSOR_RMS_CROP_OFF, &prev_state)) {
		if (new_b_ratio != b_ratio)
			sensor->rms_crop_state = BIT(IS_SENSOR_RMS_CROP_ON)
						| BIT(IS_SENSOR_RMS_CROP_SWITCHING);
	} else if (test_bit(IS_SENSOR_RMS_CROP_ON, &prev_state)) {
		if (new_b_ratio == b_ratio)
			sensor->rms_crop_state = BIT(IS_SENSOR_RMS_CROP_OFF)
						| BIT(IS_SENSOR_RMS_CROP_SWITCHING);
	}

	if (test_bit(IS_SENSOR_RMS_CROP_SWITCHING, &sensor->rms_crop_state))
		mginfo("[F%d] Change rms_crop_state 0x%lx -> 0x%lx\n",
		       grp, grp, fcount,
		       prev_state, sensor->rms_crop_state);

	dbg_hw(2, "[%s][F%d]%s: b_ratio %d rms_crop_state 0x%lx\n",
	       group_id_name[grp->id], fcount, __func__,
	       b_ratio, sensor->rms_crop_state);
}

static void is_hardware_frame_start(struct is_hw_ip *hw_ip, u32 instance)
{
	struct is_frame *frame = NULL;
	struct is_framemgr *framemgr;
	struct is_group *group, *head;
	u32 hw_fcount;
	struct is_sysfs_debug *sysfs_debug;
	int ret;

	framemgr = hw_ip->framemgr;
	group = hw_ip->group[instance];
	head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);
	FIMC_BUG_VOID(!head);

	sysfs_debug = is_get_sysfs_debug();
	if (unlikely(sysfs_debug->pattern_en) && (group->id == head->id))
		sysfs_debug->emulate_irq(instance, V_VALID);

	hw_fcount = atomic_read(&hw_ip->fcount);

	framemgr_e_barrier(framemgr, 0);

	/* AEB state transition is required for short stream, too. */
	if (hw_ip->frame_type == LIB_FRAME_HDR_SHORT)
		is_hardware_aeb_change(group, hw_fcount);

	frame = find_start_frame(hw_ip, framemgr, instance, hw_fcount);
	if (!frame) {
		/* error happened..print the frame info */
		mserr_hw("FSTART frame null. internal_fcount(%d)", instance, hw_ip,
			 hw_ip->internal_fcount[instance]);
		frame_manager_print_info_queues(framemgr);
		print_all_hw_frame_count(hw_ip->hardware);
		goto skip_start;
	} else if (frame->state == FS_HW_WAIT_DONE) {
		msdbgs_hw(2, "[F:%d] already processed to HW_WAIT_DONE state", instance, hw_ip,
			  frame->fcount);
		goto skip_start;
	}

	/* Below code must be run only once for each frame */
	is_hardware_aeb_change(group, hw_fcount);
	is_hardware_rms_crop_state_change(group, hw_fcount);

	/* TODO: multi-instance */
	frame->frame_info[INFO_FRAME_START].cpu = raw_smp_processor_id();
	frame->frame_info[INFO_FRAME_START].pid = current->pid;
	frame->frame_info[INFO_FRAME_START].when = local_clock();

	if (framemgr->batch_num == 1 && frame->fcount != atomic_read(&hw_ip->count.fs)) {
		/* error handling */
		info_hw("frame_start_isr (%d, %d)\n", frame->fcount,
				atomic_read(&hw_ip->count.fs));
		atomic_set(&hw_ip->count.fs, frame->fcount);
	}

	trans_frame(framemgr, frame, FS_HW_WAIT_DONE);

skip_start:
	framemgr_x_barrier(framemgr, 0);
	if (atomic_read(&hw_ip->status.Vvalid) == V_VALID)
		msdbg_hw(1, "[F%d][HF%d] already in VValid", instance, hw_ip,
				frame ? frame->fcount : -1, hw_fcount);
	else
		atomic_set(&hw_ip->status.Vvalid, V_VALID);

	if (!hw_ip->hardware->fake_otf_frame_start && frame) {
		ret = CALL_HWIP_OPS(hw_ip, cmp_fcount, frame->fcount);
		if (ret > 0) {
			mswarn_hw("[F%d] LATE_FRAME ret(%d)", instance, hw_ip, frame->fcount, ret);
			frame->result = IS_SHOT_LATE_FRAME;
		}
	}

	is_dvfs_get_freq(&group->device->resourcemgr->dvfs_ctrl, hw_ip->hardware->dvfs_freq);
}

int is_hardware_sensor_start(struct is_hardware *hardware, u32 instance,
	ulong hw_map, struct is_group *group)
{
	int ret = 0;
	struct is_hw_ip *hw_ip;
	struct is_group *head, *child;
	int hw_list[GROUP_HW_MAX], hw_maxnum, hw_index;
	struct is_device_sensor *sensor;
	ulong streaming_state = BIT(HW_SENSOR_STREAMING);

	FIMC_BUG(!hardware);

	head = group;

	child = head->tail;
	while (child && (child->device_type == IS_DEVICE_ISCHAIN)) {
		hw_maxnum = is_get_hw_list(child->id, hw_list);
		for (hw_index = hw_maxnum - 1; hw_index >= 0; hw_index--) {
			hw_ip = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_list[hw_index]);
			if (!hw_ip) {
				merr_hw("invalid id (%d)", instance, hw_list[hw_index]);
				return -EINVAL;
			}

			ret = CALL_HWIP_OPS(hw_ip, sensor_start, instance);
			if (ret) {
				mserr_hw("sensor_start fail", instance, hw_ip);
				return -EINVAL;
			}
			msdbg_hw(2, "hw_sensor_start [P:0x%lx]\n", instance, hw_ip, hw_map);
		}
		child = child->parent;
	}

	sensor = group->device->sensor;
	if (sensor && test_bit(IS_SENSOR_OTF_OUTPUT, &sensor->state))
		streaming_state |= BIT(HW_ISCHAIN_STREAMING);

	atomic_set(&hardware->streaming[hardware->sensor_position[instance]], streaming_state);
	atomic_set(&hardware->bug_count, 0);
	atomic_set(&hardware->log_count, 0);
	clear_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag);

	return ret;
}

int is_hardware_sensor_stop(struct is_hardware *hardware, u32 instance,
	ulong hw_map, struct is_group *group)
{
	int ret = 0;
	int retry, hw_slot;
	struct is_frame *frame;
	struct is_framemgr *framemgr;
	struct is_group *head;
	struct is_hw_ip *hw_ip = NULL, *wait_hw_ip;
	ulong flags = 0;
	bool wait;

	FIMC_BUG(!hardware);

	head = group;

	hw_ip = is_get_hw_ip(head->id, hardware);
	if (!hw_ip) {
		mgerr("invalid hw_ip", head, head);
		return -EINVAL;
	}

	ret = CALL_HWIP_OPS(hw_ip, sensor_stop, instance);
	if (ret)
		mserr_hw("sensor_stop fail", instance, hw_ip);

	msdbg_hw(2, "hw_sensor_stop [P:0x%lx]\n", instance, hw_ip, hw_map);

	if (hw_ip->frame_type == LIB_FRAME_HDR_SHORT) {
		mginfo("Skip sensor_stop for multi_ch group", head, head);
		return 0;
	}

	atomic_set(&hardware->streaming[hardware->sensor_position[instance]], 0);
	atomic_set(&hardware->bug_count, 0);
	atomic_set(&hardware->log_count, 0);
	clear_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag);

	/* decrease lic_update state if used */
	if (atomic_read(&hardware->lic_updated) > 0)
		atomic_dec(&hardware->lic_updated);

	framemgr = hw_ip->framemgr;
	retry = 99;
	while (--retry) {
		framemgr_e_barrier_irqs(framemgr, 0, flags);
		if (!framemgr->queued_count[FS_HW_WAIT_DONE]) {
			framemgr_x_barrier_irqr(framemgr, 0, flags);
			break;
		}

		frame = peek_frame(framemgr, FS_HW_WAIT_DONE);
		if (frame == NULL) {
			framemgr_x_barrier_irqr(framemgr, 0, flags);
			break;
		}

		msinfo_hw("hw_sensor_stop: com_list: [F:%d][%d][C:0x%lx][(%d)", instance, hw_ip,
			  frame->fcount, frame->type, frame->core_flag,
			  framemgr->queued_count[FS_HW_WAIT_DONE]);

		wait = false;
		for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
			wait_hw_ip = &hardware->hw_ip[hw_slot];
			if (!test_bit(wait_hw_ip->id, &hardware->hw_map[instance]))
				continue;

			if (test_bit(wait_hw_ip->id, &frame->core_flag)) {
				wait = true;
				break;
			}
		}

		if (frame->num_buffers > 1 || !wait) {
			retry = 0;
			framemgr_x_barrier_irqr(framemgr, 0, flags);
			break;
		}

		mswarn_hw(" %d com waiting...", instance, hw_ip,
			  framemgr->queued_count[FS_HW_WAIT_DONE]);

		framemgr_x_barrier_irqr(framemgr, 0, flags);
		usleep_range(1000, 1001);
	}

	if (!retry) {
		frame = NULL;
		framemgr_e_barrier_irqs(framemgr, 0, flags);

		frame = peek_frame(framemgr, FS_HW_WAIT_DONE);

		framemgr_x_barrier_irqr(framemgr, 0, flags);

		if (frame) {
			ret = is_hardware_frame_ndone(hw_ip, frame, IS_SHOT_UNPROCESSED);
			if (ret)
				mserr_hw("hardware_frame_ndone fail", instance, hw_ip);
		}
	}

	/* for last fcount */
	print_all_hw_frame_count(hardware);

	msinfo_hw("hw_sensor_stop: done[P:0x%lx]\n", instance, hw_ip, hw_map);

	return ret;
}

int is_hardware_process_start(struct is_hardware *hardware, u32 instance)
{
	int ret, i;
	ulong hw_map;
	struct is_hw_ip *hw_ip;

	FIMC_BUG(!hardware);

	hw_map = hardware->hw_map[instance];
	mdbg_hw(1, "process_start hw_map[0x%lx]\n", instance, hw_map);

	for (i = 0; i < HW_SLOT_MAX; i++) {
		hw_ip = &hardware->hw_ip[i];
		if(!test_bit(hw_ip->id, &hw_map))
			continue;

		if (test_and_set_bit(instance, &hw_ip->run_rsc_state))
			mswarn_hw("try to enable enabled instance", instance, hw_ip);

		ret = CALL_HWIP_OPS(hw_ip, enable, instance, hw_map);
		if (ret) {
			mserr_hw("enable fail", instance, hw_ip);
			return ret;
		}

		hw_ip->internal_fcount[instance] = 0;
	}

	return 0;
}
PST_EXPORT_SYMBOL(is_hardware_process_start);

static void pablo_hardware_open_crta_bufmgr(u32 instance, u32 hw_id)
{
	u32 type;
	struct pablo_crta_bufmgr *bufmgr;

	for (type = PABLO_CRTA_BUF_BASE; type <= PABLO_CRTA_BUF_CDAF_MW; type++) {
		bufmgr = pablo_get_crta_bufmgr(type, instance, hw_id);
		CALL_CRTA_BUFMGR_OPS(bufmgr, open);
	}

}

static void pablo_hardware_close_crta_bufmgr(u32 instance, u32 hw_id)
{
	u32 type;
	struct pablo_crta_bufmgr *bufmgr;

	for (type = PABLO_CRTA_BUF_BASE; type <= PABLO_CRTA_BUF_CDAF_MW; type++) {
		bufmgr = pablo_get_crta_bufmgr(type, instance, hw_id);
		CALL_CRTA_BUFMGR_OPS(bufmgr, close);
	}
}

static void is_hardware_deinit(struct is_hardware *hardware, u32 instance)
{
	int i;
	struct is_hw_ip *hw_ip;

	for (i = 0; i < HW_SLOT_MAX; i++) {
		hw_ip = &hardware->hw_ip[i];
		if(!test_bit(hw_ip->id, &hardware->hw_map[instance]))
			continue;

		if (CALL_HWIP_OPS(hw_ip, deinit, instance))
			mserr_hw("deinit fail", instance, hw_ip);
	}
}

static void is_hardware_close_by_rsccount(struct is_hardware *hardware, u32 instance)
{
	int i;
	ulong hw_map;
	struct is_hw_ip *hw_ip;
	bool use_crta = false;
	u32 hw_id;

	/*
	 * The logical_hw_map is opened IP originally and hw_map is changed IP dynamically.
	 * So, both IPs should be closed because rsccount was increased at change_chain function.
	 */
	hw_map = hardware->logical_hw_map[instance] | hardware->hw_map[instance];

	for (i = 0; i < HW_SLOT_MAX; i++) {
		hw_ip = &hardware->hw_ip[i];
		if(!test_bit(hw_ip->id, &hw_map))
			continue;

		msinfo_hw("close. rsccnt %d\n", instance, hw_ip, atomic_read(&hw_ip->rsccount));

		if (atomic_dec_return(&hw_ip->rsccount) > 0)
			continue;

		hw_ip->framemgr = &hardware->framemgr[hw_ip->id];

		del_timer_sync(&hw_ip->shot_timer);

		/* Before close hw_ip, flush frames if remained */
		is_hardware_force_stop(hardware, hw_ip, instance);

		if (!use_crta) {
			use_crta = CALL_HW_CHAIN_INFO_OPS(hardware, check_crta_hw, hw_ip->id);
			hw_id = hw_ip->ch;
		}

		if (CALL_HWIP_OPS(hw_ip, close, instance))
			mserr_hw("close fail", instance, hw_ip);

		memset(hw_ip->debug_info, 0x00, sizeof(struct hw_debug_info) * DEBUG_FRAME_COUNT);
		memset(hw_ip->debug_ext_info, 0x00, sizeof(struct hw_debug_info) * DEBUG_EXT_MAX);
		hw_ip->debug_index[0] = 0;
		hw_ip->debug_index[1] = 0;
		clear_bit(HW_OPEN, &hw_ip->state);
		clear_bit(HW_INIT, &hw_ip->state);
		clear_bit(HW_CONFIG, &hw_ip->state);
		clear_bit(HW_RUN, &hw_ip->state);
		clear_bit(HW_TUNESET, &hw_ip->state);
		atomic_set(&hw_ip->instance, 0);
		hw_ip->run_rsc_state = 0;
	}

	if (use_crta)
		pablo_hardware_close_crta_bufmgr(instance, hw_id);
}

int is_hardware_close(struct is_hardware *hardware, u32 instance)
{
	FIMC_BUG(!hardware);

	mdbg_hw(1, "%s\n", instance, __func__);

	is_hardware_deinit(hardware, instance);

	is_hardware_close_by_rsccount(hardware, instance);

	if (CALL_ADT_MSG_OPS(hardware->icpu_adt, register_response_msg_cb, instance,
			PABLO_HIC_SHOT, NULL))
		err_hw("icpu_adt register_response_msg_cb fail");

	return 0;
}
PST_EXPORT_SYMBOL(is_hardware_close);

static int is_hardware_init(struct is_hardware *hardware, u32 instance, bool rep_flag)
{
	int ret, i;
	struct is_hw_ip *hw_ip;
	u32 f_type = is_sensor_get_frame_type(instance);

	for (i = 0; i < HW_SLOT_MAX; i++) {
		hw_ip = &hardware->hw_ip[i];
		if(!test_bit(hw_ip->id, &hardware->hw_map[instance]))
			continue;

		ret = CALL_HWIP_OPS(hw_ip, init, instance, rep_flag, f_type);
		if (ret) {
			mserr_hw("init fail", instance, hw_ip);
			goto err_init;
		}
	}
	return 0;

err_init:
	while (i-- > 0) {
		hw_ip = &hardware->hw_ip[i];
		if(!test_bit(hw_ip->id, &hardware->hw_map[instance]))
			continue;

		if (CALL_HWIP_OPS(hw_ip, deinit, instance))
			mserr_hw("deinit fail", instance, hw_ip);
	}

	return ret;
}

static int is_hardware_open_by_rsccount(struct is_hardware *hardware, u32 instance)
{
	int ret, i;
	struct is_hw_ip *hw_ip;
	bool use_crta = false;
	u32 hw_id;

	for (i = 0; i < HW_SLOT_MAX; i++) {
		hw_ip = &hardware->hw_ip[i];
		if(!test_bit(hw_ip->id, &hardware->hw_map[instance]))
			continue;

		hw_ip->stm[instance] = pablo_lib_get_stream_prefix(instance);

		msinfo_hw("open. rsccnt %d\n", instance, hw_ip, atomic_read(&hw_ip->rsccount));

		if (atomic_inc_return(&hw_ip->rsccount) > 1)
			continue;

		hw_ip->hardware = hardware;
		hw_ip->framemgr = &hardware->framemgr[hw_ip->id];
		memset(hw_ip->debug_info, 0x00, sizeof(struct hw_debug_info) * DEBUG_FRAME_COUNT);
		memset(hw_ip->debug_ext_info, 0x00, sizeof(struct hw_debug_info) * DEBUG_EXT_MAX);
		hw_ip->applied_scenario = -1;
		hw_ip->debug_index[0] = 0;
		hw_ip->debug_index[1] = 0;
		hw_ip->sfr_dump_flag = false;
		atomic_set(&hw_ip->count.fs, 0);
		atomic_set(&hw_ip->count.cl, 0);
		atomic_set(&hw_ip->count.fe, 0);
		atomic_set(&hw_ip->count.dma, 0);
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		hw_ip->run_rsc_state = 0;
		timer_setup(&hw_ip->shot_timer, (void (*)(struct timer_list *))is_hardware_shot_timer, 0);

		ret = CALL_HWIP_OPS(hw_ip, open, instance);
		if (ret) {
			mserr_hw("open fail", instance, hw_ip);
			atomic_dec(&hw_ip->rsccount);
			goto err_open;
		}

		if (!use_crta) {
			use_crta = CALL_HW_CHAIN_INFO_OPS(hardware, check_crta_hw, hw_ip->id);
			hw_id = hw_ip->ch;
		}
	}

	if (use_crta)
		pablo_hardware_open_crta_bufmgr(instance, hw_id);

	return 0;

err_open:
	while (i-- > 0) {
		hw_ip = &hardware->hw_ip[i];
		if(!test_bit(hw_ip->id, &hardware->hw_map[instance]))
			continue;

		if (atomic_dec_return(&hw_ip->rsccount) == 0)
			CALL_HWIP_OPS(hw_ip, close, instance);
	}

	return ret;
}

int is_hardware_open(struct is_hardware *hardware, u32 instance, bool rep_flag)
{
	int ret;

	FIMC_BUG(!hardware);

	mdbg_hw(1, "%s\n", instance, __func__);

	ret = CALL_ADT_MSG_OPS(hardware->icpu_adt, register_response_msg_cb, instance,
			PABLO_HIC_SHOT, _is_hardware_shot_callback);
	if (ret) {
		merr_adt("icpu_adt register_response_msg_cb fail", instance);
		return ret;
	}

	ret = is_hardware_open_by_rsccount(hardware, instance);
	if (ret) {
		merr_hw("is_hardware_open_by_rsccount fail", instance);
		return ret;
	}

	ret = is_hardware_init(hardware, instance, rep_flag);
	if (ret) {
		merr_hw("init fail", instance);
		goto err_init;
	}

	return 0;

err_init:
	is_hardware_close_by_rsccount(hardware, instance);

	return ret;
}
PST_EXPORT_SYMBOL(is_hardware_open);

int is_hardware_change_chain(struct is_hardware *hardware,
	struct is_group *group, u32 instance, u32 next_id)
{
	int ret = 0;
	struct is_group *child;
	struct is_hw_ip *hw_ip;

	FIMC_BUG(!hardware);

	child = group->tail;
	while (child && (child->device_type == IS_DEVICE_ISCHAIN)) {
		hw_ip = is_get_hw_ip(child->id, hardware);
		if (!hw_ip) {
			mgerr("invalid hw_ip", child, child);
			return -EINVAL;
		}

		ret = CALL_HWIP_OPS(hw_ip, change_chain, instance, next_id, hardware);
		if (ret) {
			mserr_hw("change_chain callback is fail", instance, hw_ip);
			return ret;
		}

		msinfo_hw("change_chain done (map: L(0x%lx) P(0x%lx))", instance, hw_ip,
			hardware->logical_hw_map[instance], hardware->hw_map[instance]);

		child = child->parent;
	}

	return ret;
}

int is_hardware_shot_done(struct is_hw_ip *hw_ip, struct is_frame *frame,
	struct is_framemgr *framemgr, enum ShotErrorType done_type)
{
	int ret = 0;
	struct work_struct *work_wq;
	struct is_work_list *work_list;
	struct is_work *work;
	struct is_group *head;
	u32  instance;

	instance = frame->instance;
	if (hw_ip->frame_type == LIB_FRAME_HDR_SHORT)
		instance = atomic_read(&hw_ip->instance);

	if (frame->type == SHOT_TYPE_INTERNAL || frame->type == SHOT_TYPE_MULTI)
		goto free_frame;

	head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, hw_ip->group[instance]);
	if (!head) {
		mserr_hw("Failed to get HEAD group", instance, hw_ip);
		ret = -EINVAL;
		goto exit;
	}

	msdbgs_hw(2, "shot_done [F:%d][C:0x%lx]\n", frame->instance, hw_ip, frame->fcount,
		  frame->core_flag);

	work_wq   = &hw_ip->itf->work_wq[WORK_SHOT_DONE];
	work_list = &hw_ip->itf->work_list[WORK_SHOT_DONE];

	get_free_work(work_list, &work);
	if (work) {
		work->msg.id		= 0;
		work->msg.command	= IS_FRAME_DONE;
		work->msg.instance	= frame->instance;
		work->msg.group		= head->slot;
		work->msg.param1	= frame->fcount;
		work->msg.param2	= done_type; /* status: enum ShotErrorType */
		work->msg.param3	= 0;
		work->msg.param4	= 0;

		work->fcount = work->msg.param1;
		set_req_work(work_list, work);

		if (!work_pending(work_wq))
			wq_func_schedule(hw_ip->itf, work_wq);
	} else {
		mserr_hw("free work item is empty\n", frame->instance, hw_ip);

		/* WQ debug info */
		print_fre_work_list(work_list);
		print_req_work_list(work_list);
	}

free_frame:
	if (done_type) {
		msinfo_hw("SHOT_NDONE [E%d][F:%d]\n", frame->instance, hw_ip,
			done_type, frame->fcount);
		goto exit;
	}

	if (frame->type == SHOT_TYPE_INTERNAL) {
		msdbg_hw(1, "INTERNAL_SHOT_DONE [F:%d]\n",
			frame->instance, hw_ip, frame->fcount);
		atomic_inc(&hw_ip->hardware->log_count);
	} else if (frame->type == SHOT_TYPE_MULTI) {
		mswarn_hw("SHOT_TYPE_MULTI [F:%d]\n",
			frame->instance, hw_ip, frame->fcount);
	} else {
		msdbg_hw(1, "SHOT_DONE [F:%d]\n",
			frame->instance, hw_ip, frame->fcount);
		atomic_set(&hw_ip->hardware->log_count, 0);
	}
exit:
	if (framemgr->queued_count[FS_HW_FREE] > 10)
		atomic_set(&hw_ip->hardware->bug_count, 0);

	return ret;
}

static struct is_frame *find_done_frame(struct is_hw_ip *hw_ip, struct is_framemgr *framemgr)
{
	struct is_frame *frame, *next;
	struct list_head *head = &framemgr->queued_list[FS_HW_WAIT_DONE];
	ulong flags = 0;

	framemgr_e_barrier_common(framemgr, 0, flags);

	list_for_each_entry_safe(frame, next, head, list) {
		if (frame->type == SHOT_TYPE_LATE) {
			/* Flush LATE_SHOT frames */
			trans_frame(framemgr, frame, FS_HW_FREE);
			is_hardware_shot_done(hw_ip, frame, framemgr, IS_SHOT_LATE_FRAME);
		} else if (!test_bit_variables(hw_ip->id, &frame->core_flag)) {
			msinfo_hw("invalid core_flag [F:%d][C:0x%lx]", frame->instance,
				  hw_ip, frame->fcount, frame->core_flag);

			/* Flush too old frame in WAIT_DONE state */
			if (!list_entry_is_head(next, head, list)) {
				trans_frame(framemgr, frame, FS_HW_FREE);
				is_hardware_shot_done(hw_ip, frame, framemgr, IS_SHOT_DROP);
			}
		} else {
			break;
		}
	}

	/* Return NULL when current frame is invalid */
	if (list_entry_is_head(frame, head, list) ||
	    !test_bit_variables(hw_ip->id, &frame->core_flag))
		frame = NULL;

	framemgr_x_barrier_common(framemgr, 0, flags);

	return frame;
}

static int is_hardware_frame_done(struct is_hw_ip *hw_ip, struct is_frame *frame,
	int wq_id, u32 output_id, enum ShotErrorType done_type, bool get_meta)
{
	struct is_framemgr *framemgr = hw_ip->framemgr;
	struct is_group *group, *head;
	u32 hw_fe_cnt = atomic_read(&hw_ip->fcount);
	struct is_sysfs_debug *sysfs_debug;
	ulong flags = 0;
	u32 instance;

	switch (done_type) {
	case IS_SHOT_SUCCESS:
		if (frame == NULL)
			frame = find_done_frame(hw_ip, framemgr);
		else
			sdbg_hw(2, "frame NOT null!!(%d)", hw_ip, done_type);

		if (IS_RUNNING_TUNING_SYSTEM() && frame)
			pablo_obte_regdump(frame->instance, hw_ip->id,
					frame->stripe_info.region_id,
					frame->stripe_info.region_num);

		break;
	case IS_SHOT_LATE_FRAME:
	case IS_SHOT_UNPROCESSED:
	case IS_SHOT_OVERFLOW:
	case IS_SHOT_INVALID_FRAMENUMBER:
	case IS_SHOT_TIMEOUT:
	case IS_SHOT_CONFIG_LOCK_DELAY:
	case IS_SHOT_DROP:
		break;
	default:
		serr_hw("invalid done_type(%d)", hw_ip, done_type);
		return -EINVAL;
	}

	if (frame == NULL) {
		serr_hw("[F:%d]frame_done: frame(null)!!(%d)(0x%x)", hw_ip,
			hw_fe_cnt, done_type, output_id);
		framemgr_e_barrier_common(framemgr, 0, flags);
		frame_manager_print_info_queues(framemgr);
		print_all_hw_frame_count(hw_ip->hardware);
		framemgr_x_barrier_common(framemgr, 0, flags);
		return -EINVAL;
	}

	instance = frame->instance;
	group = hw_ip->group[instance];
	head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);

	sysfs_debug = is_get_sysfs_debug();
	if (unlikely(sysfs_debug->pattern_en) && head && (group->id == head->id))
		sysfs_debug->emulate_irq(instance, V_BLANK);

	if (hw_ip->frame_type == LIB_FRAME_HDR_SHORT)
		instance = atomic_read(&hw_ip->instance);

	msdbgs_hw(2, "[0x%x]frame_done [F:%d][HWF:%d][C:0x%lx]\n", frame->instance, hw_ip,
		  output_id, frame->fcount, hw_fe_cnt, frame->core_flag);


	if (frame->shot && get_meta)
		is_hardware_get_meta(hw_ip, frame,
			instance, hw_ip->hardware->hw_map[instance]);

	if (atomic_read(&hw_ip->status.Vvalid) == V_BLANK)
		msdbg_hw(1, "[F%d][HF%d] already in VBlank", frame->instance, hw_ip,
				frame->fcount, hw_fe_cnt);
	else
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);

	framemgr_e_barrier_common(framemgr, 0, flags);
	clear_bit(hw_ip->id, &frame->core_flag);
	if (!frame->core_flag) {
		trans_frame(framemgr, frame, FS_HW_FREE);
		framemgr_x_barrier_common(framemgr, 0, flags);

		frame->frame_info[INFO_FRAME_END_PROC].cpu = raw_smp_processor_id();
		frame->frame_info[INFO_FRAME_END_PROC].pid = current->pid;
		frame->frame_info[INFO_FRAME_END_PROC].when = local_clock();

		if (frame->result == IS_SHOT_LATE_FRAME)
			done_type = IS_SHOT_LATE_FRAME;

		return is_hardware_shot_done(hw_ip, frame, framemgr, done_type);
	}
	framemgr_x_barrier_common(framemgr, 0, flags);

	return 0;
}

int is_hardware_frame_ndone(struct is_hw_ip *ldr_hw_ip,
	struct is_frame *frame, enum ShotErrorType done_type)
{
	int ret = 0;
	struct is_hw_ip *hw_ip;
	struct is_group *chain_head;
	struct is_group *sensor_head;
	struct is_group *child;
	struct is_group *pnext;
	struct is_hardware *hardware;
	enum is_hardware_id hw_id = DEV_HW_END;
	int hw_list[GROUP_HW_MAX], hw_index;
	int hw_maxnum = 0;
	u32 instance;

	if (!frame) {
		serr_hw("ndone frame is NULL(%d)", ldr_hw_ip, done_type);
		return -EINVAL;
	}

	instance = frame->instance;
	if (ldr_hw_ip->frame_type == LIB_FRAME_HDR_SHORT)
		instance = atomic_read(&ldr_hw_ip->instance);

	chain_head = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, ldr_hw_ip->group[instance]);
	if (!chain_head) {
		mserr_hw("Failed to get HEAD group", instance, ldr_hw_ip);
		return -EINVAL;
	}

	sensor_head = chain_head->head;
	hardware = ldr_hw_ip->hardware;

	/* SFR dump */
	child = chain_head;
	pnext = sensor_head->pnext;
	while (child) {
		hw_maxnum = is_get_hw_list(child->id, hw_list);
		for (hw_index = 0; hw_index < hw_maxnum; hw_index++) {
			hw_id = hw_list[hw_index];
			hw_ip = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_id);
			if (!hw_ip) {
				err_hw("hw_ip is NULL");
				continue;
			}

			if (done_type == IS_SHOT_TIMEOUT) {
				is_hardware_sfr_dump(hardware, hw_id, false);
				ret = CALL_HWIP_OPS(hw_ip, notify_timeout, child->instance);
				if (ret) {
					mserr_hw("notify_timeout fail", child->instance, hw_ip);
					break;
				}
			}
		}
		child = child->child;

		if (pnext && !child) {
			child = pnext;
			pnext = NULL;
		}
	}
#if defined(HW_TIMEOUT_PANIC_ENABLE)
	if ((CHECK_TIMEOUT_PANIC_HW(ldr_hw_ip->id) ||
		    test_bit(IS_GROUP_REPROCESSING, &chain_head->state)) &&
		(done_type == IS_SHOT_TIMEOUT))
		is_debug_s2d(true, "IS_SHOT_TIMEOUT");
#endif

	/* force set core_flag for not done callbak, but it will be deprecated. */
	set_bit(ldr_hw_ip->id, &frame->core_flag);

	/* Not done */
	child = chain_head;
	pnext = sensor_head->pnext;
	while (child) {
		hw_maxnum = is_get_hw_list(child->id, hw_list);
		for (hw_index = 0; hw_index < hw_maxnum; hw_index++) {
			hw_ip = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_list[hw_index]);
			if (!hw_ip) {
				merr_hw("invalid id (%d)", instance, hw_list[hw_index]);
				return -EINVAL;
			}

			ret = CALL_HWIP_OPS(hw_ip, frame_ndone, frame, done_type);
			if (ret) {
				mserr_hw("frame_ndone fail", child->instance, hw_ip);
				return -EINVAL;
			}
		}
		child = child->child;

		if (pnext && !child) {
			child = pnext;
			pnext = NULL;
		}
	}

	return ret;
}

static int parse_setfile_header(ulong addr, struct is_setfile_header *header)
{
	union __setfile_header *file_header;

	/* 1. check setfile version */
	/* 2. load version specific header information */
	file_header = (union __setfile_header *)addr;
	if (file_header->magic_number == (SET_FILE_MAGIC_NUMBER - 1)) {
		header->version = SETFILE_V2;

		header->num_ips = file_header->ver_2.subip_num;
		header->num_scenarios = file_header->ver_2.scenario_num;

		header->scenario_table_base = sizeof(struct __setfile_header_ver_2);
		header->setfile_entries_base = file_header->ver_2.setfile_offset;

		header->designed_bits = 0;
		memset(header->version_code, 0, 5);
		memset(header->revision_code, 0, 5);
	} else if (file_header->magic_number == SET_FILE_MAGIC_NUMBER) {
		header->version = SETFILE_V3;

		header->num_ips = file_header->ver_3.subip_num;
		header->num_scenarios = file_header->ver_3.scenario_num;

		header->scenario_table_base = sizeof(struct __setfile_header_ver_3);
		header->setfile_entries_base = file_header->ver_3.setfile_offset;

		header->designed_bits = file_header->ver_3.designed_bit;
		memcpy(header->version_code, file_header->ver_3.version_code, 4);
		header->version_code[4] = 0;
		memcpy(header->revision_code, file_header->ver_3.revision_code, 4);
		header->revision_code[4] = 0;
	} else {
		err_hw("invalid magic number[0x%08x]", file_header->magic_number);
		return -EINVAL;
	}

	/* 3. process more header information */
	header->num_setfile_base = header->scenario_table_base
		+ (header->num_ips * header->num_scenarios * sizeof(u32));
	header->setfile_table_base = header->num_setfile_base
		+ (header->num_ips * sizeof(u32));

	info_hw("%s: version(%d)(%s)\n", __func__, header->version, header->revision_code);

	dbg_hw(1, "%s: number of IPs: %d\n", __func__, header->num_ips);
	dbg_hw(1, "%s: number of scenario: %d\n", __func__, header->num_scenarios);
	dbg_hw(1, "%s: scenario table base: 0x%lx\n", __func__, addr + header->scenario_table_base);
	dbg_hw(1, "%s: number of setfile base: 0x%lx\n", __func__, addr + header->num_setfile_base);
	dbg_hw(1, "%s: setfile table base: 0x%lx\n", __func__, addr + header->setfile_table_base);
	dbg_hw(1, "%s: setfile entries base: 0x%lx\n", __func__, addr + header->setfile_entries_base);

	return 0;
}

static int parse_setfile_info(struct is_hw_ip *hw_ip,
	ulong addr,
	struct is_setfile_header header,
	u32 sensor_position,
	u32 num_ips,
	struct __setfile_table_entry *setfile_table_entry)
{
	unsigned long base;
	size_t blk_size;
	u32 idx;
	struct is_hw_ip_setfile *setfile;
	struct is_core *core;
	struct is_mem *mem;
	int setfile_size;
	int offset;

	setfile = &hw_ip->setfile[sensor_position];

	/* skip setfile parsing if it alreay parsed at each sensor position */
	if (setfile->using_count > 0)
		return 0;

	/* set version */
	setfile->version = header.version;

	/* set what setfile index is used at each scenario */
	base = header.scenario_table_base + addr;
	blk_size = header.num_scenarios * sizeof(u32);
	memcpy(setfile->index, (void *)(base + (num_ips * blk_size)), blk_size);

	/* fill out-of-range index for each not-used scenario to check sanity */
	memset((u32 *)&setfile->index[header.num_scenarios],
		0xff, (IS_MAX_SCENARIO - header.num_scenarios) * sizeof(u32));
	for (idx = 0; idx < header.num_scenarios; idx++)
		sdbg_hw(1, "scenario table [%d:%d]\n", hw_ip,
			idx, setfile->index[idx]);

	/* set the number of setfile at each sub IP */
	base = header.num_setfile_base + addr;
	blk_size = sizeof(u32);
	setfile->using_count = (u32)*(ulong *)(base + (num_ips * blk_size));

	if (setfile->using_count > IS_MAX_SETFILE) {
		serr_hw("too many setfile entries: %d", hw_ip, setfile->using_count);
		return -EINVAL;
	}

	sdbg_hw(1, "number of setfile: %d\n", hw_ip, setfile->using_count);
	core = is_get_is_core();
	mem = &core->resourcemgr.mem;

	if (!CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, skip_setfile_load, hw_ip->id)) {
		setfile_size = setfile->using_count * setfile_table_entry[0].size;
		hw_ip->pb_setfile[sensor_position] = CALL_PTR_MEMOP(mem, alloc, mem->priv,
								setfile_size, NULL, 0);
		if (IS_ERR(hw_ip->pb_setfile[sensor_position])) {
			err("failed to allocate buffer for setfile, size: 0x%x", setfile_size);
			hw_ip->pb_setfile[sensor_position] = NULL;

			return -ENOMEM;
		}

		hw_ip->kvaddr_setfile[sensor_position] = CALL_BUFOP(hw_ip->pb_setfile[sensor_position], kvaddr,
										hw_ip->pb_setfile[sensor_position]);

		memcpy((void *)hw_ip->kvaddr_setfile[sensor_position],
			(void *)addr + header.setfile_entries_base + setfile_table_entry[0].offset,
			setfile_size);
		/* set each setfile address and size */
		for (idx = 0; idx < setfile->using_count; idx++) {
			offset = setfile_table_entry[idx].offset - setfile_table_entry[0].offset;
			setfile->table[idx].addr = hw_ip->kvaddr_setfile[sensor_position] + offset;
			setfile->table[idx].size = setfile_table_entry[idx].size;

			sdbg_hw(1, "setfile[%d] addr: 0x%lx, size: %x\n",
				hw_ip, idx,
				setfile->table[idx].addr,
				setfile->table[idx].size);
		}
	}

	return 0;
}

static void set_hw_slots_bit(struct is_hardware *hw, unsigned long *slots, int nslots, int hw_id)
{
	int hw_slot;

	switch (hw_id) {
	/* setfile chain (3AA0, 3AA1, ISP0, ISP1) */
	case DEV_HW_3AA0:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_3AA1;
		fallthrough;
	case DEV_HW_3AA1:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_3AA2;
		fallthrough;
	case DEV_HW_3AA2:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_3AA3;
		fallthrough;
	case DEV_HW_3AA3:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_ISP0_ID;
		fallthrough;
	case DEV_HW_ISP0_ID:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_ISP1;
		break;
	/* setfile chain (MCSC0, MCSC1) */
	case DEV_HW_MCSC0:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_MCSC1;
		break;

	/* setfile chain (LME0, LME1) */
	case DEV_HW_LME0:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_LME1;
		break;
	/* setfile chain (ORB0, ORB1) */
	case DEV_HW_ORB0:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		hw_id = DEV_HW_ORB1;
		break;
	}

	switch (hw_id) {
	/* every leaf of each setfile chain */
	case DEV_HW_ISP1:
	case DEV_HW_VRA:
	case DEV_HW_MCSC1:
	case DEV_HW_LME1:
	case DEV_HW_ORB1:
		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, hw_id);
		if (valid_hw_slot_id(hw_slot))
			set_bit(hw_slot, slots);
		break;
	}
}

static void get_setfile_hw_slots_no_hint(struct is_hardware *hw, unsigned long *slots, int ip, u32 num_ips)
{
	int hw_id = 0;

	bitmap_zero(slots, HW_SLOT_MAX);

	if (num_ips == 3) {
		/* ISP, DRC, VRA */
		switch (ip) {
		case 0:
			hw_id = DEV_HW_3AA0;
			break;
		case 2:
			hw_id = DEV_HW_VRA;
			break;
		}
	} else if (num_ips == 4) {
		/* ISP, DRC, TDNR, VRA */
		switch (ip) {
		case 0:
			hw_id = DEV_HW_3AA0;
			break;
		case 3:
			hw_id = DEV_HW_VRA;
			break;
		}
	} else if (num_ips == 5) {
		/* ISP, DRC, DIS, TDNR, VRA */
		switch (ip) {
		case 0:
			hw_id = DEV_HW_3AA0;
			break;
		case 4:
			hw_id = DEV_HW_VRA;
			break;
		}
	} else if (num_ips == 6) {
		/* ISP, DRC, DIS, TDNR, MCSC, VRA */
		switch (ip) {
		case 0:
			hw_id = DEV_HW_3AA0;
			break;
		case 4:
			hw_id = DEV_HW_MCSC0;
			break;
		case 5:
			hw_id = DEV_HW_VRA;
			break;
		}
	}

	dbg_hw(1, "%s: hw_id: %d, IP: %d, number of IPs: %d\n", __func__, hw_id, ip, num_ips);

	if (hw_id > 0)
		set_hw_slots_bit(hw, slots, HW_SLOT_MAX, hw_id);
}

static void get_setfile_hw_slots(struct is_hardware *hw, unsigned long *slots, unsigned long *hint)
{
	dbg_hw(1, "%s: designed bits(0x%lx) ", __func__, *hint);

	bitmap_zero(slots, HW_SLOT_MAX);

	if (test_and_clear_bit(SETFILE_DESIGN_BIT_3AA_ISP, hint)) {
		set_hw_slots_bit(hw, slots, HW_SLOT_MAX, DEV_HW_3AA0);

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_DRC, hint)) {
		/* deprecated */
		/* set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_DRC); */

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_SCC, hint)) {
		/* not supported yet */
		/* set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_SCC); */

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_ODC, hint)) {
		/* not supported yet */
		/* set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_ODC); */

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_VDIS, hint)) {
		/* deprecated */
		/* set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_DIS); */

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_TDNR, hint)) {
		/* deprecated */
		/* set_hw_slots_bit(slots, HW_SLOT_MAX, DEV_HW_3DNR); */

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_SCX_MCSC, hint)) {
		set_hw_slots_bit(hw, slots, HW_SLOT_MAX, DEV_HW_MCSC0);

	} else if (test_and_clear_bit(SETFILE_DESIGN_BIT_FD_VRA, hint)) {
		set_hw_slots_bit(hw, slots, HW_SLOT_MAX, DEV_HW_VRA);

	}

	dbg_hw(1, "              -> (0x%lx)\n", *hint);
}

int is_hardware_parse_setfile(struct is_hardware *hardware, ulong addr,
	u32 sensor_position)
{
	struct is_setfile_header header;
	struct __setfile_table_entry *setfile_table_entry;
	unsigned long slots[DIV_ROUND_UP(HW_SLOT_MAX, BITS_PER_LONG)];
	struct is_hw_ip *hw_ip;
	unsigned long hw_slot;
	unsigned long hint;
	u32 ip;
	ulong setfile_table_idx = 0;
	int ret = 0;
	struct is_priv_buf *ipb;

	ret = parse_setfile_header(addr, &header);
	if (ret) {
		err_hw("failed to parse setfile header(%d)", ret);
		return ret;
	}

	if (header.num_scenarios > IS_MAX_SCENARIO) {
		err_hw("too many scenarios: %d", header.num_scenarios);
		return -EINVAL;
	}

	hint = header.designed_bits;
	setfile_table_entry = (struct __setfile_table_entry *)(header.setfile_table_base + addr);

	for (ip = 0; ip < header.num_ips; ip++) {
		if (header.version == SETFILE_V3)
			get_setfile_hw_slots(hardware, slots, &hint);
		else
			get_setfile_hw_slots_no_hint(hardware, slots, ip, header.num_ips);

		hw_ip = NULL;

		hw_slot = find_first_bit(slots, HW_SLOT_MAX);

		if (hw_slot == HW_SLOT_MAX) {
			unsigned long base = header.num_setfile_base + addr;
			size_t blk_size = sizeof(u32);

			setfile_table_idx = (u32)*(ulong *)(base + (ip * blk_size));
			setfile_table_entry += setfile_table_idx;

			dbg_hw(1, "skip parsing at not supported ip.\n");

			continue;
		}

		while (hw_slot < HW_SLOT_MAX) {
			hw_ip = &hardware->hw_ip[hw_slot];

			clear_bit(hw_slot, slots);
			hw_slot = find_first_bit(slots, HW_SLOT_MAX);

			ret = parse_setfile_info(hw_ip, addr, header, sensor_position, ip, setfile_table_entry);
			if (ret) {
				serr_hw("parse setfile info failed\n", hw_ip);
				goto err_parse_setfile;
			}

			/* set setfile table idx for next setfile_table base */
			setfile_table_idx = (ulong)hw_ip->setfile[sensor_position].using_count;
		}

		/* increase setfile table base even though there is no valid HW slot */
		if (hw_ip)
			setfile_table_entry += setfile_table_idx;
		else
			setfile_table_entry++;
	}

	return ret;

err_parse_setfile:
	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];
		ipb = hw_ip->pb_setfile[sensor_position];
		if (ipb)
			CALL_VOID_BUFOP(ipb, free, ipb);

		hw_ip->pb_setfile[sensor_position] = NULL;
	}

	return ret;
}

int is_hardware_load_setfile(struct is_hardware *hardware, u32 instance, ulong hw_map)
{
	struct is_hw_ip *hw_ip;
	int hw_slot;
	int ret = 0;

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];
		if(!test_bit(hw_ip->id, &hardware->hw_map[instance]))
			continue;

		ret = CALL_HWIP_OPS(hw_ip, load_setfile, instance, hw_map);
		if (ret) {
			mserr_hw("failed to load setfile(%d)", instance, hw_ip, ret);
			return ret;
		}
	}

	return ret;
};

int is_hardware_apply_setfile(struct is_hardware *hardware, u32 instance,
	u32 scenario, ulong hw_map)
{
	struct is_hw_ip *hw_ip = NULL;
	int hw_id = 0;
	int ret = 0;
	int hw_slot = -1;
	enum exynos_sensor_position sensor_position;

	FIMC_BUG(!hardware);

	if (IS_MAX_SCENARIO <= scenario) {
		merr_hw("invalid scenario id: scenario(%d)", instance, scenario);
		return -EINVAL;
	}

	minfo_hw("apply_setfile: hw_map (0x%lx)\n", instance, hw_map);

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];
		hw_id = hw_ip->id;
		if(!test_bit(hw_id, &hardware->hw_map[instance]))
			continue;

		ret = CALL_HWIP_OPS(hw_ip, apply_setfile, scenario, instance, hw_map);
		if (ret) {
			mserr_hw("apply_setfile fail (%d)", instance, hw_ip, ret);
			return -EINVAL;
		}

		sensor_position = hardware->sensor_position[instance];
		hw_ip->applied_scenario = scenario;
	}

	return ret;
}

int is_hardware_delete_setfile(struct is_hardware *hardware, u32 instance,
	ulong hw_map)
{
	int ret = 0;
	int hw_slot;
	struct is_hw_ip *hw_ip;

	FIMC_BUG(!hardware);

	minfo_hw("delete_setfile: hw_map (0x%lx)\n", instance, hw_map);
	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];

		if (!test_bit_variables(hw_ip->id, &hw_map))
			continue;

		ret = CALL_HWIP_OPS(hw_ip, delete_setfile, instance, hw_map);
		if (ret) {
			mserr_hw("delete_setfile fail", instance, hw_ip);
			return -EINVAL;
		}
	}

	return ret;
}

int is_hardware_capture_meta_request(struct is_hardware *hardware,
	struct is_group *group, u32 instance, u32 fcount, u32 size, ulong addr)
{
	int ret;
	struct is_hw_ip *hw_ip;
	int hw_list[GROUP_HW_MAX], hw_index;
	int hw_maxnum;
	ulong hw_map;

	hw_map = hardware->hw_map[instance];
	hw_maxnum = is_get_hw_list(group->id, hw_list);
	for (hw_index = hw_maxnum - 1; hw_index >= 0; hw_index--) {
		hw_ip = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_list[hw_index]);
		if (!hw_ip) {
			merr_hw("invalid id (%d)", instance, hw_list[hw_index]);
			return -EINVAL;
		}

		ret = CALL_HWIP_OPS(hw_ip, get_cap_meta, hw_map, instance, fcount, size, addr);
		if (ret) {
			mserr_hw("get_cap_meta fail (%d)", instance, hw_ip, ret);
			return -EINVAL;
		}
	}

	return 0;
}

void is_hardware_sfr_dump(struct is_hardware *hardware, u32 hw_id, bool flag_print_log)
{
	int hw_slot = -1;
	struct is_hw_ip *hw_ip = NULL;

	if (!hardware) {
		err_hw("hardware is null\n");
		return;
	}

	if (hw_id == DEV_HW_END) {
		for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
			hw_ip = &hardware->hw_ip[hw_slot];
			_is_hardware_sfr_dump(hw_ip, flag_print_log);
		}
	} else {
		hw_ip = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_id);
		if (hw_ip)
			_is_hardware_sfr_dump(hw_ip, flag_print_log);
	}
}

void is_hardware_dma_cfg_kva64(char *name, struct is_hw_ip *hw_ip,
			struct is_frame *frame, int cur_idx,
			u32 *cmd, u32 plane,
			u64 *dst_kva, u64 *src_kva)
{
	int buf_i, plane_i, src_kva_i, dst_kva_i, src_p_i, dst_p_i;
	int level;
	u32 i;

	if (!(*cmd)) {
		for (i = 0; i < IS_MAX_PLANES; i++)
			dst_kva[i] = 0;
		return;
	}

	/* Iterator for single buffer with HW_FRO */
	for (buf_i = 0; buf_i < frame->num_buffers; buf_i++) {
		/* Single buffer DVA idx based on cur_idx in src_kva array */
		src_kva_i = (cur_idx + buf_i) * plane;
		dst_kva_i = buf_i * plane;

		if (!src_kva[src_kva_i]) {
			*cmd = 0; /* COMMAND_DISABLE */
			level = (src_kva_i == 0) ? 0 : 2;
			msdbg_hw(level, "[F:%d]%s_src_kva[%d] is zero\n",
				frame->instance, hw_ip,
				frame->fcount, name, src_kva_i);

			continue;
		}

		for (plane_i = 0; plane_i < plane; plane_i++) {
			src_p_i = src_kva_i + plane_i;
			dst_p_i = dst_kva_i + plane_i;
			dst_kva[dst_p_i] = src_kva[src_p_i];

			msdbg_hw(2, "[F:%d]%s_src_kva[%d] is 0x%llx\n",
				frame->instance, hw_ip,
				frame->fcount, name, plane_i, src_kva[src_p_i]);
		}
	}
}
PST_EXPORT_SYMBOL(is_hardware_dma_cfg_kva64);

u32 is_hardware_dma_cfg(char *name, struct is_hw_ip *hw_ip,
			struct is_frame *frame, int cur_idx, u32 num_buffers,
			u32 *cmd, u32 plane,
			pdma_addr_t *dst_dva, dma_addr_t *src_dva)
{
	int buf_i, plane_i, src_dva_i, dst_dva_i, src_p_i, dst_p_i;
	u32 org_cmd = *cmd;
	int level = 0;
	u32 i;

	if (!org_cmd) {
		for (i = 0; i < IS_MAX_PLANES; i++)
			dst_dva[i] = 0;
		goto exit;
	}

	msdbg_hw(3, "dma_cfg of %s, num plane %d\n", 0, hw_ip, name, plane);

	if (!plane) {
		mswarn_hw("plane is zero", frame->instance, hw_ip);
		*cmd = 0; /* COMMAND_DISABLE */
		goto exit;
	}

	/* Iterator for single buffer with HW_FRO */
	for (buf_i = 0; buf_i < frame->num_buffers; buf_i++) {
		/* Single buffer DVA idx based on cur_idx in src_dva array */
		src_dva_i = (cur_idx + (buf_i % num_buffers)) * plane;
		dst_dva_i = buf_i * plane;

		if (!src_dva[src_dva_i]) {
			*cmd = 0; /* COMMAND_DISABLE */
			level = (src_dva_i == 0) ? 0 : 2;
			msdbg_hw(level, "[F:%d]%s_src_dva[%d] is zero\n",
				frame->instance, hw_ip,
				frame->fcount, name, src_dva_i);

			continue;
		}

		for (plane_i = 0; plane_i < plane; plane_i++) {
			src_p_i = src_dva_i + plane_i;
			dst_p_i = dst_dva_i + plane_i;
			dst_dva[dst_p_i] = (pdma_addr_t) src_dva[src_p_i];
		}
	}

exit:
	return org_cmd;
}
PST_KUNIT_EXPORT_SYMBOL(is_hardware_dma_cfg);

void is_hardware_debug_otf(struct is_hardware *hardware, struct is_group *group)
{
	struct is_group *child;
	struct is_hw_ip *hw_ip_paf = NULL;
	struct is_hw_ip *hw_ip_3aa = NULL;

	child = GET_HEAD_GROUP_IN_DEVICE(IS_DEVICE_ISCHAIN, group);
	while (child) {
		if (child->slot == GROUP_SLOT_PAF)
			hw_ip_paf = is_get_hw_ip(child->id, hardware);
		else if (child->slot == GROUP_SLOT_3AA)
			hw_ip_3aa = is_get_hw_ip(child->id, hardware);

		child = child->child;
	}

	if ((hw_ip_paf && hw_ip_3aa)
			&& atomic_read(&hw_ip_3aa->count.fe) > atomic_read(&hw_ip_paf->count.fe)) {
		err_hw("hw_paf(%d) fe_cnt(%d) < hw_3aa(%d) fe_cnt(%d)",
				hw_ip_paf->id, atomic_read(&hw_ip_paf->count.fe),
				hw_ip_3aa->id, atomic_read(&hw_ip_3aa->count.fe));

		CALL_HWIP_OPS(hw_ip_paf, show_status, group->instance);
	}
}

static const struct is_hardware_ops is_hardware_ops = {
	.frame_start = is_hardware_frame_start,
	.config_lock = is_hardware_config_lock,
	.frame_done = is_hardware_frame_done,
	.frame_ndone = is_hardware_frame_ndone,
	.flush_frame = is_hardware_flush_frame,
	.dbg_trace = _is_hw_frame_dbg_trace,
	.dma_cfg = is_hardware_dma_cfg,
	.dma_cfg_kva64 = is_hardware_dma_cfg_kva64,
	.clear_crc_status = clear_gather_crc_status,
	.sfr_dump = is_hardware_sfr_dump,
};

static int is_hardware_ip_probe(struct is_hardware *hardware, struct is_interface *itf,
	struct is_interface_ischain *itfc, enum is_hardware_id ip_id, int ip_num, char *ip_name,
	hw_ip_probe_fn_t probe_fn, hw_ip_remove_fn_t remove_fn)
{
	int ret;
	struct is_hw_ip *hw_ip, *locomotive;
	int i;
	enum is_hardware_id hw_id;

	for (i = 0; i < ip_num; i++) {
		hw_id = ip_id + i;
		hw_ip = CALL_HW_CHAIN_INFO_OPS(hardware, get_hw_ip, hw_id);
		if (!hw_ip) {
			err_hw("invalid id (%d)", hw_id);
			return -EINVAL;
		}

		hw_ip->hardware = hardware;

		ret = probe_fn(hw_ip, itf, itfc, hw_id, ip_name);
		if (ret) {
			err_hw("%s%d probe fail (%d)", ip_name, i, hw_id);
			return ret;
		}

		/* common */
		hw_ip->id = hw_id;
		snprintf(hw_ip->name, sizeof(hw_ip->name), "%s%d", ip_name, i);
		hw_ip->itf = itf;
		hw_ip->itfc = itfc;
		atomic_set(&hw_ip->fcount, 0);
		atomic_set(&hw_ip->status.Vvalid, V_BLANK);
		atomic_set(&hw_ip->rsccount, 0);
		init_waitqueue_head(&hw_ip->status.wait_queue);
		hw_ip->state = 0;
		hw_ip->ip_num = ip_num;

		pablo_hw_helper_probe(hw_ip);

		if (i == 0)
			locomotive = hw_ip;
		hw_ip->locomotive = locomotive;
		hw_ip->remove = remove_fn;
		sinfo_hw("probe done\n", hw_ip);
	}

	return 0;
}

void is_hardware_dump(void)
{
	struct is_core *core = is_get_is_core();
	struct is_hardware *hardware = &core->hardware;
	struct is_debug_event *dbg_event = pablo_debug_get_event();
	struct is_hw_ip *hw_ip;
	u32 hw_slot;

	info("### %s dump start ###\n", __func__);

	if (atomic_read(&dbg_event->event_cnt[IS_EVENT_OVERFLOW_IBUF].cnt) <= 0)
		goto skip_hardware_dump;

	for (hw_slot = 0; hw_slot < HW_SLOT_MAX; hw_slot++) {
		hw_ip = &hardware->hw_ip[hw_slot];
		if (test_bit(HW_OPEN, &hw_ip->state) && test_bit(HW_OTF, &hw_ip->state))
			CALL_HWIP_OPS(hw_ip, notify_timeout, atomic_read(&hw_ip->instance));
	}

skip_hardware_dump:
	info("### %s dump end ###\n", __func__);
}

int is_hardware_probe(struct is_hardware *hardware,
	struct is_interface *itf, struct is_interface_ischain *itfc,
	struct platform_device *pdev)
{
	int ret = 0;
	int i;
	struct device *dev;
	struct exynos_platform_is *pdata;
	struct is_core *core;

	FIMC_BUG(!hardware);
	FIMC_BUG(!itf);
	FIMC_BUG(!itfc);

	for (i = 0; i < HW_SLOT_MAX; i++) {
		hardware->hw_ip[i].id = DEV_HW_END;
		hardware->hw_ip[i].priv_info = NULL;
		hardware->hw_ip[i].dump_for_each_reg = NULL;
		hardware->hw_ip[i].dump_reg_list_size = 0;
		hardware->hw_ip[i].remove = NULL;
	}

	dev = &pdev->dev;
	pdata = dev->platform_data;

	if (IS_ENABLED(SOC_CSTAT0))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_3AA0, pdata->num_of_ip.taa,
				"CSTAT", is_hw_cstat_probe, is_hw_cstat_remove);

	else if (IS_ENABLED(SOC_30S))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_3AA0, pdata->num_of_ip.taa,
				"3AA", is_hw_3aa_probe, is_hw_3aa_remove);

	if (IS_ENABLED(SOC_I0S))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_ISP0, pdata->num_of_ip.isp,
				"ISP", is_hw_isp_probe, is_hw_isp_remove);

	if (IS_ENABLED(SOC_BYRP))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_BYRP, pdata->num_of_ip.byrp,
				"BYRP", is_hw_byrp_probe, is_hw_byrp_remove);

	if (IS_ENABLED(SOC_RGBP))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_RGBP, pdata->num_of_ip.rgbp,
				"RGBP", is_hw_rgbp_probe, is_hw_rgbp_remove);

	ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_MCSC0, pdata->num_of_ip.mcsc,
				"MCS", is_hw_mcsc_probe, is_hw_mcsc_remove);

	if (IS_ENABLED(SOC_VRA))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_VRA, pdata->num_of_ip.vra,
				"VRA", is_hw_vra_probe, is_hw_vra_remove);

	if (IS_ENABLED(SOC_YPP))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_YPP, pdata->num_of_ip.ypp,
				"YPP", is_hw_ypp_probe, is_hw_ypp_remove);

	if (IS_ENABLED(SOC_YUVP))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_YPP, pdata->num_of_ip.ypp,
				"YUVP", is_hw_yuvp_probe, is_hw_yuvp_remove);

	if (IS_ENABLED(SOC_SHRP))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_SHRP, pdata->num_of_ip.shrp,
				"SHRP", is_hw_shrp_probe, is_hw_shrp_remove);

	if (IS_ENABLED(SOC_MCFP))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_MCFP, pdata->num_of_ip.mcfp,
				"MCFP", is_hw_mcfp_probe, is_hw_mcfp_remove);

	if (IS_ENABLED(CONFIG_PABLO_RGBP))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_RGBP, pdata->num_of_ip.rgbp,
				"RGBP", pablo_hw_rgbp_probe, pablo_hw_rgbp_remove);

	if (IS_ENABLED(CONFIG_PABLO_MTNR)) {
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_MTNR0, pdata->num_of_ip.mtnr0,
				"MTNR0", pablo_hw_mtnr0_probe, pablo_hw_mtnr0_remove);
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_MTNR1, pdata->num_of_ip.mtnr1,
				"MTNR1", pablo_hw_mtnr1_probe, pablo_hw_mtnr1_remove);
	}

	if (IS_ENABLED(CONFIG_PABLO_MSNR))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_MSNR, pdata->num_of_ip.msnr,
				"MSNR", pablo_hw_msnr_probe, pablo_hw_msnr_remove);

	if (IS_ENABLED(CONFIG_PABLO_DLFE))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_DLFE, pdata->num_of_ip.dlfe,
				"DLFE", pablo_hw_dlfe_probe, pablo_hw_dlfe_remove);

	if (IS_ENABLED(SOC_LME0))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_LME0, pdata->num_of_ip.lme,
				"LME", is_hw_lme_probe, is_hw_lme_remove);

	if (IS_ENABLED(SOC_ORBMCH))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_ORB0, pdata->num_of_ip.orbmch,
				"ORBMCH", is_hw_orbmch_probe, is_hw_orbmch_remove);

	if (IS_ENABLED(CONFIG_PABLO_YUVSC))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_YUVSC0, pdata->num_of_ip.yuvsc,
				"YUVSC", pablo_hw_yuvsc_probe, pablo_hw_yuvsc_remove);

	if (IS_ENABLED(CONFIG_PABLO_MLSC))
		ret |= is_hardware_ip_probe(hardware, itf, itfc,
				DEV_HW_MLSC0, pdata->num_of_ip.mlsc,
				"MLSC", pablo_hw_mlsc_probe, pablo_hw_mlsc_remove);

	if (ret) {
		err_hw("failed to is_hardware_ip_probe()");
		return ret;
	}

	hardware->icpu_adt = pablo_get_icpu_adt();

	core = is_get_is_core();
	for (i = 0; i < IS_STREAM_COUNT; i++) {
		hardware->hw_map[i] = 0;
		hardware->sensor_position[i] = 0;
	}

	for (i = 0; i < SENSOR_POSITION_MAX; i++)
		atomic_set(&hardware->streaming[i], 0);

	for (i = 0; i < GROUP_SLOT_MAX; i++)
		atomic_set(&hardware->slot_rsccount[i], 0);

	mutex_init(&hardware->itf_lock);

	atomic_set(&hardware->rsccount, 0);
	atomic_set(&hardware->bug_count, 0);
	atomic_set(&hardware->log_count, 0);
	clear_bit(HW_OVERFLOW_RECOVERY, &hardware->hw_recovery_flag);

	prepare_sfr_dump(hardware);
	is_debug_register_dump_func(is_hardware_dump);

	return 0;
}

void is_hardware_remove(struct is_hardware *hardware)
{
	int i;
	struct is_hw_ip *hw_ip;

	free_sfr_dump(hardware);

	for (i = 0; i < HW_SLOT_MAX; i++) {
		hw_ip = &hardware->hw_ip[i];
		if (hw_ip->remove) {
			hw_ip->remove(hw_ip);
			sinfo_hw("remove done\n", hw_ip);
		}
	}
}

static int crc_flag;
int clear_gather_crc_status(u32 instance, struct is_hw_ip *hw_ip)
{
	struct is_device_ischain *device;
	struct is_device_sensor *sensor;
	struct is_device_csi *csi;

	device = is_get_ischain_device(instance);
	if (!device) {
		mserr_hw("failed to get devcie", instance, hw_ip);
		return -ENODEV;
	}

	sensor = device->sensor;
	if (!sensor) {
		mserr_hw("failed to get sensor", instance, hw_ip);
		return -ENODEV;
	}

	csi = (struct is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		mserr_hw("csi is null\n", instance, hw_ip);
		return -ENODEV;
	}

	if (csi->crc_flag)
		crc_flag |= 1 << instance;
	else
		crc_flag &= ~(1 << instance);

	csi->crc_flag = false;

	return crc_flag;
}
