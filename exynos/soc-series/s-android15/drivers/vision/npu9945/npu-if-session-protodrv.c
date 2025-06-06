/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>

#define NPU_LOG_TAG	"if-session-protodrv"

#include "include/npu-config.h"
#include "include/npu-common.h"
#include "npu-if-session-protodrv.h"
#include "npu-util-llq.h"
#include "npu-log.h"
#include "npu-queue.h"
#include "npu-session.h"
#include "npu-device.h"

extern int npu_session_save_result(struct npu_session *session, struct nw_result nw_result);
extern void npu_session_queue_done(struct npu_queue *queue, struct npu_queue_list *inclist, struct npu_queue_list *otclist, unsigned long flag);

/* Context information */
static struct npu_if_session_protodrv_ctx ctx = {
	.is_opened = ATOMIC_INIT(0),
};

/* TODO: Link appripirate function in Session manager */
const struct npu_if_session_protodrv_ops npu_if_session_protodrv_ops = {
	.queue_done = npu_session_queue_done,
};

/* Initializer and deallocator */
struct npu_if_session_protodrv_ctx *npu_if_session_protodrv_ctx_open(void)
{
	spin_lock_init(&ctx.buffer_q_lock);
	spin_lock_init(&ctx.ncp_mgmt_lock);

	INIT_KFIFO(ctx.buffer_q_list);
	INIT_KFIFO(ctx.ncp_mgmt_list);

	atomic_set(&ctx.is_opened, 1);
	return &ctx;
}

void npu_if_session_protodrv_ctx_close(void)
{
	atomic_set(&ctx.is_opened, 0);
	kfifo_reset(&ctx.buffer_q_list);
	kfifo_reset(&ctx.ncp_mgmt_list);
}

/* Return 1 if the npu_if_session_protodrv is opened state
 * Otherwise, returns 0
 */
int npu_if_session_protodrv_is_opened(void)
{
	return atomic_read(&ctx.is_opened);
}

/* Operations for buffer_q */
int npu_buffer_q_get(struct npu_frame *frame)
{
	BUG_ON(!frame);
	return kfifo_out_spinlocked(&ctx.buffer_q_list, frame, 1, &ctx.buffer_q_lock);
}

int npu_buffer_q_put(const struct npu_frame *frame)
{
	int ret;

	BUG_ON(!frame);
	ret = kfifo_in_spinlocked(&ctx.buffer_q_list, frame, 1, &ctx.buffer_q_lock);
	if (likely(ret > 0)) {
		if (likely(ctx.buffer_q_callback)) {
			ctx.buffer_q_callback();
		}
	}
	return ret;
}

#ifndef CONFIG_NPU_KUNIT_TEST
void npu_buffer_q_restart(void)
{
	if (likely(ctx.buffer_q_callback))
		ctx.buffer_q_callback();
}
#endif

int npu_buffer_q_is_available(void)
{
	return !kfifo_is_empty(&ctx.buffer_q_list);
}

void npu_buffer_q_register_cb(LLQ_task_t cb)
{
	npu_dbg("callback for buffer_q: [%pK]\n", cb);
	BUG_ON(!cb);
	ctx.buffer_q_callback = cb;
}

void npu_buffer_q_notify_done(const struct npu_frame *frame)
{
	unsigned long flags = 0;

	set_bit(VS4L_CL_FLAG_DONE, &flags);
	if (unlikely(frame->result_code == NPU_ERR_NPU_TIMEOUT)) {
		/* Timeout occurred - Assert fw timeout flag */
		npu_ufwarn("FW timeout flag asserted.\n", frame);
		set_bit(VS4L_CL_FLAG_FW_TIMEOUT, &flags);
	} else if (unlikely(frame->result_code == NPU_ERR_NPU_HW_TIMEOUT_RECOVERED)) {
		/* Error occurred - Assert HW_Timeout recovered flag */
		npu_ufwarn("NDONE HW Timeout recovered flag asserted.\n", frame);
		set_bit(VS4L_CL_FLAG_HW_TIMEOUT_RECOVERED, &flags);
	} else if (unlikely(frame->result_code == NPU_ERR_NPU_HW_TIMEOUT_NOTRECOVERABLE)) {
		/* Error occurred - Assert HW_Timeout notrecoverable flag */
		npu_ufwarn("NDONE HW Timeout notrecoverable flag asserted.\n", frame);
		set_bit(VS4L_CL_FLAG_HW_TIMEOUT_NOTRECOVERABLE, &flags);
	} else if (unlikely(frame->result_code)) {
		/* Error occurred - Assert invalid flag */
		npu_ufwarn("NDONE flag asserted.\n", frame);
		set_bit(VS4L_CL_FLAG_INVALID, &flags);
	}

	if (likely(npu_if_session_protodrv_ops.queue_done)) {
		if (likely(frame->src_queue && frame->input && frame->output)) {
			npu_uftrace("Calling npu_queue_done, frame_id = [%u]\n", frame, frame->frame_id);
			npu_if_session_protodrv_ops.queue_done(
				frame->src_queue, frame->input, frame->output, flags);
			npu_ufdbg("Succeeded frame_id = [%u]\n", frame, frame->frame_id);
		} else
			npu_ufdbg("Skip calling queue_done[queue=%pK, in=%pK, out=%pK]\n",
				frame, frame->src_queue, frame->input, frame->output);
	} else {
		npu_warn("not defined: queue_done\n");
	}
}

/* Operations for ncp_mgmt */
int npu_ncp_mgmt_get(struct npu_nw *frame)
{
	BUG_ON(!frame);
	return kfifo_out_spinlocked(&ctx.ncp_mgmt_list, frame, 1, &ctx.ncp_mgmt_lock);
}

int npu_ncp_mgmt_put(const struct npu_nw *frame)
{
	int ret;

	BUG_ON(!frame);
	ret = kfifo_in_spinlocked(&ctx.ncp_mgmt_list, frame, 1, &ctx.ncp_mgmt_lock);
	if (likely(ret > 0)) {
		if (likely(ctx.ncp_mgmt_callback))
			ctx.ncp_mgmt_callback();
	}
	return ret;
}

int npu_ncp_mgmt_is_available(void)
{
	return !kfifo_is_empty(&ctx.ncp_mgmt_list);
}

void npu_ncp_mgmt_register_cb(LLQ_task_t cb)
{
	npu_dbg("callback for ncp_mgmt(%pK)\n", cb);
	BUG_ON(!cb);
	ctx.ncp_mgmt_callback = cb;
}

int npu_ncp_mgmt_save_result(
	save_result_func notify_func,
	struct npu_session *sess,
	struct nw_result result)
{
	if (likely(notify_func)) {
		npu_dbg("notify_func invoked. sess=%pK, result=0x%08x\n",
			sess, result.result_code);
		return notify_func(sess, result);
	} else {
		npu_warn("not defined: save_result function\n");
		return 0;
	}
}
