/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
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

#include "is-core.h"
#include "is-cmd.h"
#include "is-err.h"

ulong frame_fcount(struct is_frame *frame, void *data)
{
	return (ulong)frame->fcount - (ulong)data;
}
EXPORT_SYMBOL_GPL(frame_fcount);

int put_frame(struct is_framemgr *this, struct is_frame *frame,
			enum is_frame_state state)
{
	if (state == FS_INVALID)
		return -EINVAL;

	if (!frame) {
		err("invalid frame");
		return -EFAULT;
	}

	frame->state = state;

	list_add_tail(&frame->list, &this->queued_list[state]);
	this->queued_count[state]++;

#ifdef TRACE_FRAME
	print_frame_queue(this, state);
#endif

	return 0;
}
EXPORT_SYMBOL_GPL(put_frame);

struct is_frame *get_frame(struct is_framemgr *this,
			enum is_frame_state state)
{
	struct is_frame *frame;

	if (state == FS_INVALID)
		return NULL;

	if (!this->queued_count[state])
		return NULL;

	frame = list_first_entry(&this->queued_list[state],
						struct is_frame, list);
	list_del(&frame->list);
	this->queued_count[state]--;

	frame->state = FS_INVALID;

	return frame;
}
EXPORT_SYMBOL_GPL(get_frame);

int trans_frame(struct is_framemgr *this, struct is_frame *frame,
			enum is_frame_state state)
{
	if (!frame) {
		err("invalid frame");
		return -EFAULT;
	}

	if ((frame->state == FS_INVALID) || (state == FS_INVALID))
		return -EINVAL;

	if (!this->queued_count[frame->state]) {
		err("%s frame queue is empty (%s)", frame_state_name[frame->state],
							this->name);
		return -EINVAL;
	}

	list_del(&frame->list);
	this->queued_count[frame->state]--;

	return put_frame(this, frame, state);
}
EXPORT_SYMBOL_GPL(trans_frame);

struct is_frame *peek_frame(struct is_framemgr *this,
			enum is_frame_state state)
{
	if (state == FS_INVALID)
		return NULL;

	if (!this->queued_count[state])
		return NULL;

	return list_first_entry(&this->queued_list[state],
						struct is_frame, list);
}
EXPORT_SYMBOL_GPL(peek_frame);

struct is_frame *peek_frame_tail(struct is_framemgr *this,
			enum is_frame_state state)
{
	if (state == FS_INVALID)
		return NULL;

	if (!this->queued_count[state])
		return NULL;

	return list_last_entry(&this->queued_list[state],
						struct is_frame, list);
}
EXPORT_SYMBOL_GPL(peek_frame_tail);

struct is_frame *find_frame(struct is_framemgr *this,
		enum is_frame_state state,
		ulong (*fn)(struct is_frame *, void *), void *data)
{
	struct is_frame *frame;

	if (state == FS_INVALID)
		return NULL;

	if (!this->queued_count[state])
		return NULL;

	list_for_each_entry(frame, &this->queued_list[state], list) {
		if (!fn(frame, data))
			return frame;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(find_frame);

void print_frame_queue(struct is_framemgr *this,
			enum is_frame_state state)
{
	struct is_frame *frame, *temp;

	is_info("[FRM] %s(%s, %d) :", frame_state_name[state],
					this->name, this->queued_count[state]);

	list_for_each_entry_safe(frame, temp, &this->queued_list[state], list)
		pr_cont("%d[%d]->", frame->index, frame->fcount);

	pr_cont("X\n");
}
EXPORT_SYMBOL_GPL(print_frame_queue);

static void print_frame_info_queue(struct is_framemgr *this,
			enum is_frame_state state)
{
	unsigned long long when[MAX_FRAME_INFO];
	unsigned long usec[MAX_FRAME_INFO];
	struct is_frame *frame, *temp;

	is_info("[FRM_INFO] %s(%s, %d) :", hw_frame_state_name[state],
					this->name, this->queued_count[state]);

	list_for_each_entry_safe(frame, temp, &this->queued_list[state], list) {
		when[INFO_FRAME_START]    = frame->frame_info[INFO_FRAME_START].when;
		when[INFO_CONFIG_LOCK]    = frame->frame_info[INFO_CONFIG_LOCK].when;
		when[INFO_FRAME_END_PROC] = frame->frame_info[INFO_FRAME_END_PROC].when;

		usec[INFO_FRAME_START]    = do_div(when[INFO_FRAME_START], NSEC_PER_SEC);
		usec[INFO_CONFIG_LOCK]    = do_div(when[INFO_CONFIG_LOCK], NSEC_PER_SEC);
		usec[INFO_FRAME_END_PROC] = do_div(when[INFO_FRAME_END_PROC], NSEC_PER_SEC);

		pr_cont("%d[%d][%d]([%5lu.%06lu],[%5lu.%06lu],[%5lu.%06lu][C:0x%lx])->",
			frame->index, frame->fcount, frame->type,
			(unsigned long)when[INFO_FRAME_START],    usec[INFO_FRAME_START] / NSEC_PER_USEC,
			(unsigned long)when[INFO_CONFIG_LOCK],    usec[INFO_CONFIG_LOCK] / NSEC_PER_USEC,
			(unsigned long)when[INFO_FRAME_END_PROC], usec[INFO_FRAME_END_PROC] / NSEC_PER_USEC,
			frame->core_flag);
	}

	pr_cont("X\n");
}

int frame_manager_probe(struct is_framemgr *this, const char *name)
{
	snprintf(this->name, sizeof(this->name), "%s", name);
	spin_lock_init(&this->slock);
	this->frames = NULL;

	return 0;
}
EXPORT_SYMBOL_GPL(frame_manager_probe);

static inline u32 __get_num_repeat(struct is_frame *frame)
{
	u32 num_repeat, region_num;

	region_num = frame->stripe_info.region_num;
	num_repeat = frame->repeat_info.num;

	if (!num_repeat)
		num_repeat = region_num;
	else if (region_num)
		num_repeat *= region_num;

	return num_repeat;
}

static inline void frame_work_fn(struct is_frame *frame)
{
	struct is_group *group;
	u32 num_repeat;

	group = frame->group;

	if (!IS_ERR_OR_NULL(group))
		atomic_dec(&group->rcount);

	num_repeat = __get_num_repeat(frame);

#ifdef ENABLE_STRIPE_SYNC_PROCESSING
	if (!is_vendor_check_remosaic_mode_change(frame) && num_repeat) {
#else
	if (num_repeat) {
#endif
		/* Prevent other frame comming while stripe processing */
		while (num_repeat--)
			CALL_GROUP_OPS(group, shot, frame);
	} else {
		CALL_GROUP_OPS(group, shot, frame);
	}
}

static void default_frame_work_fn(struct kthread_work *work)
{
	struct is_frame *frame = container_of(work, struct is_frame, work);

	frame_work_fn(frame);
}

static void default_frame_dwork_fn(struct kthread_work *work)
{
	struct kthread_delayed_work *dwork = container_of(work, struct kthread_delayed_work, work);
	struct is_frame *frame = container_of(dwork, struct is_frame, dwork);

	frame_work_fn(frame);
}

int frame_manager_open(struct is_framemgr *this, u32 buffers, bool need_param)
{
	u32 i;
	unsigned long flag;

	/*
	 * We already have frames allocated, so we should free them first.
	 * reqbufs(n) could be called multiple times from userspace after
	 * each video node was opened.
	 */
	if (this->frames)
		vfree(this->frames);

	this->frames = vzalloc(array_size(sizeof(struct is_frame), buffers));
	if (!this->frames) {
		err("failed to allocate frames");
		return -ENOMEM;
	}

	if (need_param) {
		this->parameters = vzalloc(array_size(sizeof(struct is_param_region), buffers));
		if (!this->parameters) {
			err("failed to allocate parameters");
			vfree(this->frames);
			this->frames = NULL;
			return -ENOMEM;
		}
	}

	spin_lock_irqsave(&this->slock, flag);

	this->num_frames = buffers;

	for (i = 0; i < NR_FRAME_STATE; i++) {
		this->queued_count[i] = 0;
		INIT_LIST_HEAD(&this->queued_list[i]);
	}

	for (i = 0; i < buffers; ++i) {
		this->frames[i].index = i;
		put_frame(this, &this->frames[i], FS_FREE);
		kthread_init_work(&this->frames[i].work, default_frame_work_fn);
		kthread_init_delayed_work(&this->frames[i].dwork, default_frame_dwork_fn);
		if (this->parameters)
			this->frames[i].parameter = &this->parameters[i];
	}

	spin_unlock_irqrestore(&this->slock, flag);

	return 0;
}
EXPORT_SYMBOL_GPL(frame_manager_open);

int frame_manager_close(struct is_framemgr *this)
{
	u32 i;
	unsigned long flag;

	spin_lock_irqsave(&this->slock, flag);

	if (this->frames) {
		is_vfree_atomic(this->frames);
		this->frames = NULL;
	}

	this->num_frames = 0;

	if (this->parameters) {
		is_vfree_atomic(this->parameters);
		this->parameters = NULL;
	}

	for (i = 0; i < NR_FRAME_STATE; i++) {
		this->queued_count[i] = 0;
		INIT_LIST_HEAD(&this->queued_list[i]);
	}

	spin_unlock_irqrestore(&this->slock, flag);

	return 0;
}
EXPORT_SYMBOL_GPL(frame_manager_close);

int frame_manager_flush(struct is_framemgr *this)
{
	unsigned long flag;
	struct is_frame *frame, *temp;
	enum is_frame_state i;

	spin_lock_irqsave(&this->slock, flag);

	for (i = FS_REQUEST; i < FS_INVALID; i++) {
		list_for_each_entry_safe(frame, temp, &this->queued_list[i], list)
			trans_frame(this, frame, FS_FREE);
	}

	spin_unlock_irqrestore(&this->slock, flag);

	FIMC_BUG(this->queued_count[FS_FREE] != this->num_frames);

	return 0;
}
EXPORT_SYMBOL_GPL(frame_manager_flush);

void frame_manager_print_queues(struct is_framemgr *this)
{
	int i;

	if (!this->num_frames)
		return;

	for (i = 0; i < NR_FRAME_STATE; i++)
		print_frame_queue(this, (enum is_frame_state)i);
}
EXPORT_SYMBOL_GPL(frame_manager_print_queues);

void dump_frame_queue(struct is_framemgr *this,
			enum is_frame_state state)
{
	struct is_frame *frame, *temp;

	cinfo("[FRM] %s(%s, %d) :", frame_state_name[state],
					this->name, this->queued_count[state]);

	list_for_each_entry_safe(frame, temp, &this->queued_list[state], list)
		cinfo("%d[%d]->", frame->index, frame->fcount);

	cinfo("X\n");
}

void frame_manager_dump_queues(struct is_framemgr *this)
{
	int i;

	if (!this->num_frames)
		return;

	for (i = 0; i < NR_FRAME_STATE; i++)
		dump_frame_queue(this, (enum is_frame_state)i);
}
EXPORT_SYMBOL_GPL(frame_manager_dump_queues);

void frame_manager_print_info_queues(struct is_framemgr *this)
{
	int i;

	for (i = 0; i < NR_FRAME_STATE; i++)
		print_frame_info_queue(this, (enum is_frame_state)i);
}
EXPORT_SYMBOL_GPL(frame_manager_print_info_queues);

int frame_manager_reinit(struct is_framemgr *this)
{
	int i;

	for (i = 0; i < this->num_frames; ++i) {
		kthread_init_work(&this->frames[i].work, default_frame_work_fn);
		kthread_init_delayed_work(&this->frames[i].dwork, default_frame_dwork_fn);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(frame_manager_reinit);
