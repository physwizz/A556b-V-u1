/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com
 *
 * Header file for Exynos REPEATER driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REPEATER_DEV_H_
#define _REPEATER_DEV_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/dma-buf.h>
#include <linux/timer.h>

#include <media/exynos_repeater.h>

#include "repeater.h"
#include "repeater_buf.h"

#define REPEATER_MAX_CONTEXTS_NUM		1

enum repeater_buffer_status {
	REPEATER_CTX_INIT,
	REPEATER_CTX_MAP,
	REPEATER_CTX_UNMAP,
};

enum repeater_status {
	REPEATER_CTX_STOP,
	REPEATER_CTX_START,
	REPEATER_CTX_PAUSE,
};

struct repeater_context_status {
	enum repeater_buffer_status buffer_status;
	enum repeater_status status;
};

struct repeater_device {
	struct miscdevice misc_dev;
	struct device *dev;
	atomic_t ctx_num;
	struct repeater_context *ctx[REPEATER_MAX_CONTEXTS_NUM];
};

struct repeater_context {
	struct repeater_device *repeater_dev;
	struct repeater_info info;
	struct dma_buf *dmabufs[MAX_SHARED_BUF_NUM];

	struct shared_buffer shared_bufs;
	struct repeater_encoding_param enc_param;
	struct timer_list encoding_timer;
	uint64_t encoding_period_us;
	uint64_t last_encoding_time_us;
	uint64_t time_stamp_us;
	struct repeater_context_status ctx_status;
	int (*repeater_encode_cb)(int, struct repeater_encoding_param *param);
	int (*repeater_drc_cb)(void);

	struct delayed_work encoding_work;
	uint64_t encoding_start_timestamp;
	uint64_t video_frame_count;
	uint64_t paused_time;
	uint64_t pause_time;
	uint64_t resume_time;

	/* LLWFD DRC */
	bool dpu_dpms_off;
	bool last_frame_encoding;

	/* frame skipping */
	uint64_t repeated_frame_count;
	uint64_t max_skipped_frame;

	wait_queue_head_t wait_queue_error;
	int error;
	int nobuf_err_count;

	/* To tsmux otf unit test */
	int start_mode;

	/* To dump data */
	wait_queue_head_t wait_queue_dump;
	int buf_idx_dump;

	/* log level */
	int remain_logging_frame;
};

#define NODE_NAME		"repeater"
#define MODULE_NAME		"exynos-repeater"

#endif /* _REPEATER_DEV_H_ */
