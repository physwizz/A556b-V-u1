/*
 * copyright (c) 2017 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com
 *
 * Core file for Samsung EXYNOS TSMUX driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/pm_runtime.h>

#include <media/wfd_logger.h>
#include "repeater_dev.h"
#include "repeater_dbg.h"

#define REPEATER_CUR_CONTEXTS_NUM		0

static struct repeater_device *g_repeater_device;
static spinlock_t repeater_spinlock;

#define ENCODING_PERIOD_TIME_US			1000000
#define MAX_WAIT_TIME_DUMP				10000000

#define MAX_WAIT_TIME_GET_STATUS 1000000

#define MAX_NO_BUF_ERROR_COUNT	100
#define MAX_LOGGING_FRAME_COUNT 30

struct wfd_logger_info *g_repeater_logger = NULL;
int g_repeater_log_level;
int g_repeater_debug_level;
module_param(g_repeater_debug_level, int, 0600);

int g_repeater_test_error;
module_param(g_repeater_test_error, int, 0600);

static inline void increase_log_level(struct repeater_context *ctx, int log_level)
{
	ctx->remain_logging_frame = MAX_LOGGING_FRAME_COUNT;
	if (g_repeater_log_level < log_level)
		g_repeater_log_level = log_level;
}

/*
 * If fps is 60, MFC encoding is called when repeater_set_valid_buffer is called
 */

int repeater_request_buffer(struct shared_buffer_info *info, int owner)
{
	int ret = 0;
	struct repeater_context *ctx;
	int i = 0;
	int *buf_fd;
	int idx = REPEATER_CUR_CONTEXTS_NUM;
	unsigned long flags;
	int (*repeater_drc_cb)(void);

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_EXT_INFO, "++\n");

	if (!g_repeater_device || !info) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater %pK, info %pK\n",
			g_repeater_device, info);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx = g_repeater_device->ctx[idx];
	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (ctx->ctx_status.buffer_status != REPEATER_CTX_MAP) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status(%d) is invalid\n",
			ctx->ctx_status.buffer_status);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (owner != REQ_BUF_OWNER_DPU && owner != REQ_BUF_OWNER_MFC) {
		print_repeater_wfdlogger(RPT_ERROR, "owner(%d) is invalid\n", owner);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (g_repeater_test_error == REPEATER_ERR_REQ_BUFFER) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater_test_error\n");
		increase_log_level(ctx, RPT_EXT_INFO);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	info->pixel_format = ctx->info.pixel_format;
	info->width = ctx->info.width;
	info->height = ctx->info.height;
	if (ctx->info.buffer_count > MAX_SHARED_BUF_NUM) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_count is invalid %d",
			ctx->info.buffer_count);
		ctx->info.buffer_count = MAX_SHARED_BUF_NUM;
	}
	info->buffer_count = ctx->info.buffer_count;
	buf_fd = ctx->info.buf_fd;

	for (i = 0; i < info->buffer_count; i++) {
		if (IS_ERR(ctx->dmabufs[i])) {
			print_repeater_wfdlogger(RPT_ERROR, "dmabufs is error\n");
			spin_unlock_irqrestore(&repeater_spinlock, flags);
			return -EFAULT;
		}
		get_dma_buf(ctx->dmabufs[i]);
		info->bufs[i] = ctx->dmabufs[i];
	}

	print_repeater_wfdlogger(RPT_EXT_INFO,
		"f 0x%x, w %d, h %d, buf_cnt %d\n", info->pixel_format,
		info->width, info->height, info->buffer_count);
	for (i = 0; i < info->buffer_count; i++)
		print_repeater_wfdlogger(RPT_EXT_INFO,
			"owner %d, dma_buf[%d] %pK\n", owner, i, info->bufs[i]);

	if (owner == REQ_BUF_OWNER_DPU && ctx->dpu_dpms_off == true) {
		init_shared_buffer(&ctx->shared_bufs, MAX_SHARED_BUF_NUM);
		/* DPU d/d calls repeater_request_buffer */
		ctx->dpu_dpms_off = false;
		/* MFC d/d will call repeater_request_buffer */
		if (ctx->repeater_drc_cb) {
			repeater_drc_cb = ctx->repeater_drc_cb;
			spin_unlock_irqrestore(&repeater_spinlock, flags);
			repeater_drc_cb();
			spin_lock_irqsave(&repeater_spinlock, flags);
		} else {
			print_repeater_wfdlogger(RPT_ERROR, "repeater_drc_cb is invalid\n");
		}
	}

	print_repeater_wfdlogger(RPT_EXT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL(repeater_request_buffer);

int repeater_dpu_dpms_off(void)
{
	int ret = 0;
	struct repeater_context *ctx;
	int idx = REPEATER_CUR_CONTEXTS_NUM;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_EXT_INFO, "++\n");

	if (!g_repeater_device) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater_device is null\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx = g_repeater_device->ctx[idx];
	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_INIT) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status(%d) is invalid\n",
			ctx->ctx_status.buffer_status);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx->dpu_dpms_off = true;

	print_repeater_wfdlogger(RPT_EXT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL(repeater_dpu_dpms_off);

int repeater_get_valid_buffer(int *buf_idx)
{
	int ret = 0;
	struct repeater_context *ctx;
	struct shared_buffer *shr_bufs;
	int idx = REPEATER_CUR_CONTEXTS_NUM;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_EXT_INFO, "++\n");

	if (!g_repeater_device || !buf_idx) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater %pK, buf_idx %pK\n",
			g_repeater_device, buf_idx);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx = g_repeater_device->ctx[idx];
	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_INIT || ctx->repeater_encode_cb == NULL) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx_status.buffer_status %d, %s\n",
			ctx->ctx_status.buffer_status, ctx->repeater_encode_cb == NULL ? "cb is NULL" : "cb is not NULL");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	shr_bufs = &ctx->shared_bufs;
	ret = get_capturing_buf_idx(shr_bufs, buf_idx);
	if (ret || g_repeater_test_error == REPEATER_ERR_GET_CAPTURING_BUF) {
		increase_log_level(ctx, RPT_SHR_BUF_INFO);
		ctx->error = -REPEATER_ERR_GET_CAPTURING_BUF;
		wake_up_interruptible(&ctx->wait_queue_error);
		print_repeater_wfdlogger(RPT_ERROR, "get_capturing_buf_idx() return %d, g_repeater_test_error %d\n",
			g_repeater_test_error, ret);
	}

	print_repeater_wfdlogger(RPT_EXT_INFO, "ret %d, buf_idx %d\n", ret, *buf_idx);

	print_repeater_wfdlogger(RPT_EXT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL(repeater_get_valid_buffer);

int repeater_set_valid_buffer(int buf_idx, int capture_idx)
{
	int ret = 0;
	struct repeater_context *ctx;
	struct shared_buffer *shr_bufs;
	int idx = REPEATER_CUR_CONTEXTS_NUM;
	unsigned long flags;
	ktime_t cur_ktime;
	uint64_t cur_timestamp;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_EXT_INFO, "++\n");

	print_repeater_wfdlogger(RPT_EXT_INFO, "buf_idx %d, capture_idx %d\n",
		buf_idx, capture_idx);

	if (!g_repeater_device) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater_device is null\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx = g_repeater_device->ctx[idx];
	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_INIT) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx_status.buffer_status(%d) is invalid\n",
			ctx->ctx_status.buffer_status);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (ctx->start_mode == REPEATER_START_ONESHOT && ctx->enc_param.time_stamp) {
		print_repeater_wfdlogger(RPT_ERROR, "repeater is one shot mode and already one frame encoded\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return ret;
	}

	shr_bufs = &ctx->shared_bufs;

	ret = set_captured_buf_idx(shr_bufs, buf_idx, capture_idx);
	if (ret == 0) {
		ctx->repeated_frame_count = 0;
	}
	if (ret || g_repeater_test_error == REPEATER_ERR_SET_CAPTURED_BUF) {
		increase_log_level(ctx, RPT_SHR_BUF_INFO);
		ctx->error = -REPEATER_ERR_SET_CAPTURED_BUF;
		wake_up_interruptible(&ctx->wait_queue_error);
		print_repeater_wfdlogger(RPT_ERROR, "set_captured_buf_idx() return %d, g_repeater_test_error %d\n",
			ret, g_repeater_test_error);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (ctx->info.fps == 60 && ctx->repeater_encode_cb
			&& ctx->ctx_status.status == REPEATER_CTX_START) {
		cur_ktime = ktime_get();
		cur_timestamp = ktime_to_us(cur_ktime);
		if (ret == 0) {
			ctx->enc_param.time_stamp = cur_timestamp;
			set_encoding_start(shr_bufs, buf_idx);
			ret = ctx->repeater_encode_cb(buf_idx, &ctx->enc_param);
			if (ret) {
				print_repeater_wfdlogger(RPT_ERROR,
					"repeater_encode_cb failed %d\n", ret);
				ret = set_encoding_done(shr_bufs);
			}
		}
	}

	print_repeater_wfdlogger(RPT_EXT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL(repeater_set_valid_buffer);

int repeater_register_encode_cb(
	int (*repeater_encode_cb)(int, struct repeater_encoding_param *param))
{
	int ret = 0;
	struct repeater_context *ctx;
	int idx = REPEATER_CUR_CONTEXTS_NUM;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_EXT_INFO, "++\n");

	if (!g_repeater_device) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater_device is null\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx = g_repeater_device->ctx[idx];
	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx->repeater_encode_cb = repeater_encode_cb;

	print_repeater_wfdlogger(RPT_EXT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL(repeater_register_encode_cb);

int repeater_register_drc_cb(
	int (*repeater_drc_cb)(void))
{
	int ret = 0;
	struct repeater_context *ctx;
	int idx = REPEATER_CUR_CONTEXTS_NUM;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_EXT_INFO, "++\n");

	if (!g_repeater_device) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater_device is null\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx = g_repeater_device->ctx[idx];
	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx->repeater_drc_cb = repeater_drc_cb;

	print_repeater_wfdlogger(RPT_EXT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL(repeater_register_drc_cb);

int repeater_encoding_done(int encoding_ret)
{
	int ret = 0;
	struct repeater_context *ctx;
	struct shared_buffer *shr_bufs;
	int idx = REPEATER_CUR_CONTEXTS_NUM;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_EXT_INFO, "++\n");

	if (!g_repeater_device) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater_device is null\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx = g_repeater_device->ctx[idx];
	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	shr_bufs = &ctx->shared_bufs;
	ret = set_encoding_done(shr_bufs);
	if (ret) {
		// shared buffer isn't init on DRC
		print_repeater_wfdlogger(RPT_EXT_INFO, "set_encoding_done() return %d\n", ret);
	}

	print_repeater_wfdlogger(RPT_EXT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL(repeater_encoding_done);

int repeater_notify_error(int owner)
{
	int ret = 0;
	struct repeater_context *ctx;
	int idx = REPEATER_CUR_CONTEXTS_NUM;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_EXT_INFO, "++\n");

	if (!g_repeater_device) {
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater_device is null\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ctx = g_repeater_device->ctx[idx];
	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	increase_log_level(ctx, RPT_SHR_BUF_INFO);

	ctx->error = -REPEATER_ERR_NOTIFY;
	wake_up_interruptible(&ctx->wait_queue_error);
	print_repeater_wfdlogger(RPT_ERROR, "owner %d\n", owner);

	print_repeater_wfdlogger(RPT_EXT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}
EXPORT_SYMBOL(repeater_notify_error);

void encoding_work_handler(struct work_struct *work)
{
	struct repeater_context *ctx;
	struct shared_buffer *shr_bufs;
	int32_t buf_idx = -1;
	int32_t ret;
	ktime_t cur_ktime;
	uint64_t target_timestamp;
	uint64_t cur_timestamp;
	int64_t required_delay;
	unsigned long flags;
	struct delayed_work *enc_work;
	bool is_encoding_frame = false;
	struct repeater_encoding_param enc_param;
	int (*repeater_encode_cb)(int, struct repeater_encoding_param *param);

	print_repeater(RPT_INT_INFO, "++\n");

	enc_work = container_of(work, struct delayed_work, work);
	if (!enc_work) {
		print_repeater_wfdlogger(RPT_ERROR, "no enc_work\n");
		return;
	}

	spin_lock_irqsave(&repeater_spinlock, flags);
	print_repeater_wfdlogger(RPT_INT_INFO, "++++\n");

	ctx = container_of(enc_work, struct repeater_context, encoding_work);

	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return;
	}

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_INIT) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status(%d) is invalid \n",
			ctx->ctx_status.buffer_status);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return;
	}

	if (ctx->ctx_status.status != REPEATER_CTX_START) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx_status.status(%d) is invalid \n",
			ctx->ctx_status.status);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return;
	}

	cur_ktime = ktime_get();
	cur_timestamp = ktime_to_us(cur_ktime);

	target_timestamp = ctx->encoding_start_timestamp +
		ctx->video_frame_count * ctx->encoding_period_us + ctx->paused_time;
	required_delay = target_timestamp - cur_timestamp;
	print_repeater_wfdlogger(RPT_EXT_INFO, "usleep_range %lld\n", required_delay);

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	if (required_delay > 1000 && required_delay < (1000000 / ctx->info.fps))
		usleep_range(required_delay, required_delay + 1);

	spin_lock_irqsave(&repeater_spinlock, flags);

	if (!ctx) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return;
	}

	if (ctx->ctx_status.status != REPEATER_CTX_START) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx_status.status %d\n",
			ctx->ctx_status.status);
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return;
	}

	if (ctx->remain_logging_frame > 0)
		ctx->remain_logging_frame--;
	else
		g_repeater_log_level = 0;

	shr_bufs = &ctx->shared_bufs;
	ret = get_latest_captured_buf_idx(shr_bufs, &buf_idx);
	if (ret == -EBUSY || g_repeater_test_error == REPEATER_ERR_GET_LATEST_CAPTURED_BUF_BUSY) {
		increase_log_level(ctx, RPT_SHR_BUF_INFO);
		ctx->error = -REPEATER_ERR_GET_LATEST_CAPTURED_BUF_BUSY;
		wake_up_interruptible(&ctx->wait_queue_error);
		print_repeater_wfdlogger(RPT_ERROR, "get_latest_captured_buf_idx() return -EBUSY\n");
		print_repeater_wfdlogger(RPT_ERROR, "g_repeater_test_error %d\n", g_repeater_test_error);
	} else if (ret == -ENOBUFS || g_repeater_test_error == REPEATER_ERR_GET_LATEST_CAPTURED_BUF_NO) {
		ctx->nobuf_err_count++;
		if (ctx->nobuf_err_count >= MAX_NO_BUF_ERROR_COUNT) {
			/* After a DRC, a situation with no blended frames can persist for a long time. */
			/* So, it doesn't switch to GGWFD just because there are no blended frames. */
			/* Example) Gallary UHD video playback (DLNA on) */
			increase_log_level(ctx, RPT_SHR_BUF_INFO);
			print_repeater_wfdlogger(RPT_ERROR, "get_latest_captured_buf_idx() return -ENOBUFS\n");
			print_repeater_wfdlogger(RPT_ERROR, "g_repeater_test_error %d\n", g_repeater_test_error);
		}
	} else {
		ctx->nobuf_err_count = 0;
	}

	cur_ktime = ktime_get();
	cur_timestamp = ktime_to_us(cur_ktime);
	if (ctx->encoding_start_timestamp == 0)
		ctx->encoding_start_timestamp = cur_timestamp;
	ctx->enc_param.time_stamp = ctx->encoding_start_timestamp +
		ctx->video_frame_count * ctx->encoding_period_us + ctx->paused_time;
	print_repeater_wfdlogger(RPT_EXT_INFO, "time_stamp %lld\n",
	ctx->enc_param.time_stamp);

	print_repeater_wfdlogger(RPT_EXT_INFO, "video_frame_count %lld, repeated_frame_count %lld\n",
		ctx->video_frame_count, ctx->repeated_frame_count);

	if (ctx->repeated_frame_count >= ctx->max_skipped_frame)
		ctx->repeated_frame_count = 0;

	if (ctx->repeated_frame_count == 0)
		is_encoding_frame = true;

	if (ctx->dpu_dpms_off)
		is_encoding_frame = false;
	print_repeater_wfdlogger(RPT_EXT_INFO, "dpu_dpms_off %d\n", ctx->dpu_dpms_off);

	print_repeater_wfdlogger(RPT_EXT_INFO, "buf_idx %d, is_encoding_frame %d\n",
		buf_idx, is_encoding_frame);
	if (ret == 0 && buf_idx >= 0 && ctx->repeater_encode_cb && is_encoding_frame) {
		print_repeater_wfdlogger(RPT_EXT_INFO, "buf_idx %d\n", buf_idx);
		ctx->buf_idx_dump = buf_idx;
		wake_up_interruptible(&ctx->wait_queue_dump);
		set_encoding_start(shr_bufs, buf_idx);
		memcpy(&enc_param, &ctx->enc_param, sizeof(struct repeater_encoding_param));
		repeater_encode_cb = ctx->repeater_encode_cb;
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		ret = repeater_encode_cb(buf_idx, &enc_param);
		spin_lock_irqsave(&repeater_spinlock, flags);
		if (!ctx) {
			print_repeater_wfdlogger(RPT_ERROR, "ctx is NULL\n");
			spin_unlock_irqrestore(&repeater_spinlock, flags);
			return;
		}
		if (ret) {
			print_repeater_wfdlogger(RPT_ERROR,
				"repeater_encode_cb failed %d\n", ret);
			ret = set_encoding_done(shr_bufs);
		}
	} else {
		if (ctx->repeated_frame_count == 0) {
			print_repeater_wfdlogger(RPT_ERROR,
					"skip encoding, ret %d, buf_idx %d, %s, is_encoding_frame %d\n",
					ret, buf_idx, ctx->repeater_encode_cb == NULL ? "cb is NULL" : "cb is not NULL",
					is_encoding_frame);
		}
	}
	ctx->repeated_frame_count++;

	do {
		ctx->video_frame_count++;
		target_timestamp = ctx->encoding_start_timestamp +
			ctx->video_frame_count * ctx->encoding_period_us + ctx->paused_time;
		required_delay = target_timestamp - cur_timestamp;
	} while (required_delay < 0);

	print_repeater_wfdlogger(RPT_EXT_INFO, "timestamp cur %lld, target %lld\n",
		cur_timestamp, target_timestamp);

	if (ctx->ctx_status.status == REPEATER_CTX_START) {
		required_delay = required_delay / 2;
		print_repeater_wfdlogger(RPT_EXT_INFO, "schedule_delayed_work() %lld\n",
			required_delay);
		schedule_delayed_work(&ctx->encoding_work,
			usecs_to_jiffies(required_delay));
	} else {
		print_repeater_wfdlogger(RPT_ERROR, "ctx_status.status is invalid %d",
			ctx->ctx_status.status);
	}

	print_repeater_wfdlogger(RPT_INT_INFO, "----\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	print_repeater(RPT_INT_INFO, "--\n");
}

int repeater_ioctl_map_buf(
	struct repeater_context *ctx, struct repeater_info *info)
{
	int ret = 0;
	unsigned long flags;
	int i = 0;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_MAP) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status(%d) is invalid",
			ctx->ctx_status.buffer_status);
		print_repeater_wfdlogger(RPT_INT_INFO, "--\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	memcpy(&ctx->info, info, sizeof(struct repeater_info));

	if (ctx->info.buffer_count > MAX_SHARED_BUF_NUM) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_count is invalid %d",
			ctx->info.buffer_count);
		ctx->info.buffer_count = MAX_SHARED_BUF_NUM;
	}

	if (ctx->info.fps != 30 && ctx->info.fps != 60) {
		print_repeater_wfdlogger(RPT_ERROR, "fps is invalid %d",
			ctx->info.fps);
		ctx->info.fps = 30;
	}

	for (i = 0; i < ctx->info.buffer_count; i++) {
		ctx->dmabufs[i] = dma_buf_get(ctx->info.buf_fd[i]);
		if (!IS_ERR(ctx->dmabufs[i])) {
			print_repeater_wfdlogger(RPT_INT_INFO,
				"dmabufs[%d] %pK\n", i, ctx->dmabufs[i]);
		}
	}
	ctx->ctx_status.buffer_status = REPEATER_CTX_MAP;
	ctx->encoding_period_us = ENCODING_PERIOD_TIME_US / ctx->info.fps;

	memcpy(info, &ctx->info, sizeof(struct repeater_info));

	print_repeater_wfdlogger(RPT_INT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}

int repeater_ioctl_unmap_buf(struct repeater_context *ctx)
{
	int ret = 0;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");

	if (ctx->ctx_status.buffer_status != REPEATER_CTX_MAP) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status(%d) is invalid",
			ctx->ctx_status.buffer_status);
		print_repeater_wfdlogger(RPT_INT_INFO, "--\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	if (ctx->info.buffer_count > MAX_SHARED_BUF_NUM) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_count is invalid %d",
			ctx->info.buffer_count);
		ctx->info.buffer_count = MAX_SHARED_BUF_NUM;
	}

	for (i = 0; i < ctx->info.buffer_count; i++) {
		if (!IS_ERR_OR_NULL(ctx->dmabufs[i])) {
			dma_buf_put(ctx->dmabufs[i]);
			ctx->dmabufs[i] = 0;
		}
	}
	ctx->ctx_status.buffer_status = REPEATER_CTX_UNMAP;

	print_repeater_wfdlogger(RPT_INT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}

int repeater_ioctl_start(struct repeater_context *ctx, int start_mode)
{
	int ret = 0;
	unsigned long flags;
	struct shared_buffer *shr_bufs;

	spin_lock_irqsave(&repeater_spinlock, flags);
	ctx->start_mode = start_mode;

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");
	print_repeater_wfdlogger(RPT_INT_INFO, "ctx_status.status %d, fps %d, start_mode %d\n",
		ctx->ctx_status.status, ctx->info.fps, start_mode);

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_UNMAP) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status is unmap %d\n",
			ctx->ctx_status.buffer_status);
	}

	INIT_DELAYED_WORK(&ctx->encoding_work, encoding_work_handler);
	shr_bufs = &ctx->shared_bufs;
	init_shared_buffer(shr_bufs, MAX_SHARED_BUF_NUM);
	ctx->ctx_status.status = REPEATER_CTX_START;
	if (ctx->info.fps == 30) {
		ret = schedule_delayed_work(&ctx->encoding_work,
			usecs_to_jiffies(1));
		print_repeater_wfdlogger(RPT_INT_INFO,
			"schedule_delayed_work ret %d\n", ret);
	}

	print_repeater_wfdlogger(RPT_INT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}

int repeater_ioctl_stop(struct repeater_context *ctx)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");
	print_repeater_wfdlogger(RPT_INT_INFO, "ctx_status.status %d\n",
		ctx->ctx_status.status);

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_UNMAP) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status is unmap %d\n",
			ctx->ctx_status.buffer_status);
	}

	ctx->ctx_status.status = REPEATER_CTX_STOP;
	ctx->repeater_encode_cb = NULL;
	wake_up_interruptible(&ctx->wait_queue_error);
	print_repeater_wfdlogger(RPT_INT_INFO, "----\n");
	spin_unlock_irqrestore(&repeater_spinlock, flags);

	ret = cancel_delayed_work_sync(&ctx->encoding_work);
	print_repeater(RPT_INT_INFO,
		"cancel_delayed_work_sync ret %d\n", ret);

	print_repeater(RPT_INT_INFO, "--\n");

	return ret;
}

int repeater_ioctl_pause(struct repeater_context *ctx)
{
	int ret = 0;
	ktime_t cur_ktime;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");
	print_repeater_wfdlogger(RPT_INT_INFO, "ctx_status.status %d\n",
		ctx->ctx_status.status);

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_UNMAP) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status is unmap %d\n",
			ctx->ctx_status.buffer_status);
	}

	cur_ktime = ktime_get();
	ctx->pause_time = ktime_to_us(cur_ktime);
	ctx->ctx_status.status = REPEATER_CTX_PAUSE;
	print_repeater_wfdlogger(RPT_INT_INFO, "----\n");
	spin_unlock_irqrestore(&repeater_spinlock, flags);

	ret = cancel_delayed_work_sync(&ctx->encoding_work);
	print_repeater(RPT_INT_INFO,
		"cancel_delayed_work_sync ret %d\n", ret);

	print_repeater(RPT_INT_INFO, "--\n");

	return ret;
}

int repeater_ioctl_resume(struct repeater_context *ctx)
{
	int ret = 0;
	ktime_t cur_ktime;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");
	print_repeater_wfdlogger(RPT_INT_INFO, "ctx_status.status %d\n",
		ctx->ctx_status.status);

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_UNMAP) {
		print_repeater_wfdlogger(RPT_ERROR, "buffer_status is unmap %d\n",
			ctx->ctx_status.buffer_status);
	}

	INIT_DELAYED_WORK(&ctx->encoding_work, encoding_work_handler);
	cur_ktime = ktime_get();
	ctx->resume_time = ktime_to_us(cur_ktime);
	ctx->paused_time += ctx->resume_time - ctx->pause_time;
	ctx->ctx_status.status = REPEATER_CTX_START;
	ret = schedule_delayed_work(&ctx->encoding_work,
		usecs_to_jiffies(1));
	print_repeater_wfdlogger(RPT_INT_INFO,
		"schedule_delayed_work ret %d\n", ret);

	print_repeater_wfdlogger(RPT_INT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}

int repeater_ioctl_dump(struct repeater_context *ctx)
{
	int ret = 0;
	unsigned long jiffies_from_usec = usecs_to_jiffies(MAX_WAIT_TIME_DUMP);
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");

	if (ctx->ctx_status.status != REPEATER_CTX_START) {
		print_repeater_wfdlogger(RPT_ERROR, "ctx_status is invalid %d\n",
			ctx->ctx_status.status);
		print_repeater_wfdlogger(RPT_INT_INFO, "--\n");
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return -EFAULT;
	}

	ret = wait_event_interruptible_timeout(ctx->wait_queue_dump,
		ctx->buf_idx_dump >= 0, jiffies_from_usec);

	print_repeater_wfdlogger(RPT_INT_INFO,
		"wait_event_interruptible_timeout(%lu) ret %d, ctx->buf_idx_dump %d\n",
		jiffies_from_usec, ret, ctx->buf_idx_dump);

	print_repeater_wfdlogger(RPT_INT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}

int repeater_ioctl_set_max_skipped_frame(
	struct repeater_context *ctx, int max_skipped_frame)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");

	if (max_skipped_frame >= 0) {
		ctx->max_skipped_frame = max_skipped_frame;
		ret = 0;
	} else {
		print_repeater_wfdlogger(RPT_ERROR, "max_skipped_frame is invalid %d\n",
			max_skipped_frame);
		ret = -EFAULT;
	}

	print_repeater_wfdlogger(RPT_INT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}

static int repeater_ioctl_get_status(
	struct repeater_context *ctx, int *status)
{
	unsigned long jiffies_from_usec = usecs_to_jiffies(MAX_WAIT_TIME_GET_STATUS);
	unsigned long flags;
	int wait_time = 0;
	int ret = 0;

	spin_lock_irqsave(&repeater_spinlock, flags);

	print_repeater_wfdlogger(RPT_INT_INFO, "++\n");

	print_repeater_wfdlogger(RPT_INT_INFO, "ctx->ctx_status.status %d, ctx->error %d\n",
		ctx->ctx_status.status, ctx->error);

	if (ctx->ctx_status.status != REPEATER_CTX_START ||
		g_repeater_test_error == REPEATER_ERR_BAD_STATUS) {
		increase_log_level(ctx, RPT_INT_INFO);
		print_repeater_wfdlogger(RPT_ERROR, "repeater_ioctl_get_status(), status is %d, test error %d\n",
			ctx->ctx_status.status, g_repeater_test_error);
		*status = -REPEATER_ERR_BAD_STATUS;
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return ret;
	}
	if (ctx->error) {
		print_repeater_wfdlogger(RPT_ERROR, "repeater_ioctl_get_status(), ctx->error is %d\n",
			ctx->ctx_status.status);
		*status = ctx->error;
		spin_unlock_irqrestore(&repeater_spinlock, flags);
		return ret;
	}
	spin_unlock_irqrestore(&repeater_spinlock, flags);

	wait_time = wait_event_interruptible_timeout(ctx->wait_queue_error,
		false, jiffies_from_usec);

	print_repeater_wfdlogger(RPT_INT_INFO,
		"wait_event_interruptible_timeout(wait_queue_error), wait_time %d\n", wait_time);

	spin_lock_irqsave(&repeater_spinlock, flags);
	*status = ctx->error;

	print_repeater_wfdlogger(RPT_INT_INFO, "--\n");

	spin_unlock_irqrestore(&repeater_spinlock, flags);

	return ret;
}

static int repeater_ioctl_get_log_info(struct repeater_context *ctx, char* log)
{
	int log_size = 0;
	int err = 0;

	if (g_repeater_logger == NULL) {
		print_repeater(RPT_ERROR, "g_repeater_logger is NULL\n");
		return -EFAULT;
	}

	print_repeater(RPT_INT_INFO, "g_repeater_logger rewind %d, offset %d, size %d\n",
		g_repeater_logger->rewind, g_repeater_logger->offset, g_repeater_logger->size);

	if (g_repeater_logger->rewind) {
		log_size = g_repeater_logger->size;
		if (copy_to_user((char __user *)log, g_repeater_logger->log_buf + g_repeater_logger->offset,
			g_repeater_logger->size - g_repeater_logger->offset)) {
			err = -EFAULT;
		}
		if (copy_to_user((char __user *)log + g_repeater_logger->size - g_repeater_logger->offset,
			g_repeater_logger->log_buf, g_repeater_logger->offset)) {
			err = -EFAULT;
		}
	} else {
		log_size = g_repeater_logger->offset;
		if (copy_to_user((char __user *)log, g_repeater_logger->log_buf, g_repeater_logger->offset)) {
			err = -EFAULT;
		}
	}

	if (err) {
		print_repeater(RPT_ERROR, "fail to copy_to_user\n");
		return err;
	} else {
		return log_size;
	}
}

static int repeater_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct repeater_device *repeater_dev = container_of(filp->private_data,
		struct repeater_device, misc_dev);
	struct repeater_context *ctx;
	struct shared_buffer *shr_bufs;
	unsigned long flags;
	struct wfd_logger_info *wfd_logger_temp;
	int ctx_num;

	print_repeater(RPT_INT_INFO, "++\n");

	ctx_num = atomic_fetch_add_unless(&repeater_dev->ctx_num, 1, REPEATER_MAX_CONTEXTS_NUM);
	if (ctx_num >= REPEATER_MAX_CONTEXTS_NUM) {
		print_repeater(RPT_ERROR, "too many context\n");
		ret = -EBUSY;
		return ret;
	}

	ctx = kzalloc(sizeof(struct repeater_context), GFP_KERNEL);
	if (!ctx) {
		atomic_dec(&repeater_dev->ctx_num);
		ret = -ENOMEM;
		return ret;
	}

	ctx->repeater_dev = repeater_dev;
	repeater_dev->ctx[ctx_num] = ctx;

	filp->private_data = ctx;

	ctx->ctx_status.buffer_status = REPEATER_CTX_INIT;
	ctx->ctx_status.status = REPEATER_CTX_STOP;

	shr_bufs = &ctx->shared_bufs;
	init_shared_buffer(shr_bufs, MAX_SHARED_BUF_NUM);
	ctx->enc_param.time_stamp = 0;
	ctx->encoding_start_timestamp = 0;
	ctx->video_frame_count = 0;
	ctx->paused_time = 0;
	ctx->pause_time = 0;
	ctx->resume_time = 0;
	ctx->time_stamp_us = 33333;
	ctx->last_encoding_time_us = 0;
	ctx->dpu_dpms_off = false;
	ctx->error = 0;
	ctx->nobuf_err_count = 0;

	wfd_logger_temp = wfd_logger_init();
	spin_lock_irqsave(&repeater_spinlock, flags);
	g_repeater_device = repeater_dev;
	g_repeater_logger = wfd_logger_temp;
	spin_unlock_irqrestore(&repeater_spinlock, flags);

	init_waitqueue_head(&ctx->wait_queue_error);

	init_waitqueue_head(&ctx->wait_queue_dump);
	ctx->buf_idx_dump = -1;

	pm_runtime_get_sync(repeater_dev->dev);

	ctx->repeated_frame_count = 0;
	ctx->max_skipped_frame = 0;
	g_repeater_test_error = 0;

	g_repeater_log_level = 0;
	ctx->remain_logging_frame = 0;

	print_repeater(RPT_INT_INFO, "--\n");

	return ret;
}

static int repeater_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct repeater_context *ctx = filp->private_data;
	struct repeater_device *repeater_dev = ctx->repeater_dev;
	unsigned long flags;

	print_repeater(RPT_INT_INFO, "++\n");

	if (ctx->ctx_status.status == REPEATER_CTX_START ||
			ctx->ctx_status.status == REPEATER_CTX_PAUSE)
		repeater_ioctl_stop(ctx);

	if (ctx->ctx_status.buffer_status == REPEATER_CTX_MAP)
		repeater_ioctl_unmap_buf(ctx);

	pm_runtime_put_sync(repeater_dev->dev);

	spin_lock_irqsave(&repeater_spinlock, flags);
	print_repeater_wfdlogger(RPT_INT_INFO, "++++\n");

	kfree(ctx);
	filp->private_data = NULL;

	g_repeater_device = NULL;
	print_repeater_wfdlogger(RPT_INT_INFO, "----\n");
	wfd_logger_deinit(g_repeater_logger);
	g_repeater_logger = NULL;
	spin_unlock_irqrestore(&repeater_spinlock, flags);

	atomic_dec(&repeater_dev->ctx_num);
	print_repeater(RPT_INT_INFO, "--\n");
	return ret;
}

static long repeater_ioctl(struct file *filp,
	unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct repeater_context *ctx;
	struct repeater_info info;
	int max_skipped_frame = 0;
	int status;
	int start_mode;

	print_repeater(RPT_INT_INFO, "++\n");

	ctx = filp->private_data;
	if (!ctx) {
		print_repeater(RPT_ERROR, "ctx is NULL\n");
		ret = -ENOTTY;
		return ret;
	}

	switch (cmd) {
	case REPEATER_IOCTL_MAP_BUF:
		if (copy_from_user(&info,
			(struct repeater_info __user *)arg,
			sizeof(struct repeater_info))) {
			ret = -EFAULT;
			break;
		}

		ret = repeater_ioctl_map_buf(ctx, &info);

		if (copy_to_user((struct repeater_info __user *)arg,
			&info,
			sizeof(struct repeater_info))) {
			ret = -EFAULT;
			break;
		}
	break;

	case REPEATER_IOCTL_UNMAP_BUF:
		ret = repeater_ioctl_unmap_buf(ctx);
	break;

	case REPEATER_IOCTL_START:
		if (copy_from_user(&start_mode,
			(int *)arg,
			sizeof(int))) {
			ret = -EFAULT;
			break;
		}
		ret = repeater_ioctl_start(ctx, start_mode);
	break;

	case REPEATER_IOCTL_STOP:
		ret = repeater_ioctl_stop(ctx);
	break;

	case REPEATER_IOCTL_PAUSE:
		ret = repeater_ioctl_pause(ctx);
	break;

	case REPEATER_IOCTL_RESUME:
		ret = repeater_ioctl_resume(ctx);
	break;

	case REPEATER_IOCTL_DUMP:
		ret = repeater_ioctl_dump(ctx);
		if (copy_to_user((int __user *)arg,
			&ctx->buf_idx_dump,
			sizeof(int))) {
			ret = -EFAULT;
			break;
		}
		ctx->buf_idx_dump = -1;
	break;

	case REPEATER_IOCTL_SET_MAX_SKIPPED_FRAME:
		if (copy_from_user(&max_skipped_frame,
			(int __user *)arg, sizeof(int))) {
			ret = -EFAULT;
			break;
		}
		ret = repeater_ioctl_set_max_skipped_frame(ctx, max_skipped_frame);
	break;

	case REPEATER_IOCTL_GET_STATUS:
		ret = repeater_ioctl_get_status(ctx, &status);
		if (copy_to_user((int __user *)arg, &status, sizeof(int))) {
			ret = -EFAULT;
			break;
		}
	break;

	case REPEATER_IOCTL_GET_LOG_INFO:
		ret = repeater_ioctl_get_log_info(ctx, (char __user *)arg);
	break;

	default:
		ret = -ENOTTY;
	}

	print_repeater(RPT_INT_INFO, "--\n");
	return ret;

}

static const struct file_operations repeater_fops = {
	.owner          = THIS_MODULE,
	.open           = repeater_open,
	.release        = repeater_release,
	.unlocked_ioctl = repeater_ioctl,
	.compat_ioctl = repeater_ioctl,
};

static int repeater_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct repeater_device *repeater_dev;

	print_repeater(RPT_INT_INFO, "++\n");

	repeater_dev = devm_kzalloc(&pdev->dev, sizeof(struct repeater_device),
					GFP_KERNEL);
	if (!repeater_dev)
		return -ENOMEM;

	atomic_set(&repeater_dev->ctx_num, 0);
	repeater_dev->dev = &pdev->dev;
	repeater_dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
	repeater_dev->misc_dev.fops = &repeater_fops;
	repeater_dev->misc_dev.name = NODE_NAME;
	ret = misc_register(&repeater_dev->misc_dev);
	if (ret)
		goto err_misc_register;

	platform_set_drvdata(pdev, repeater_dev);

	pm_runtime_enable(&pdev->dev);

	spin_lock_init(&repeater_spinlock);
	g_repeater_debug_level = RPT_ERROR;

	print_repeater(RPT_INT_INFO, "--\n");

	return ret;

err_misc_register:
	print_repeater(RPT_ERROR, "--, err_misc_dev\n");

	return ret;
}

static int repeater_remove(struct platform_device *pdev)
{
	struct repeater_device *repeater_dev = platform_get_drvdata(pdev);

	print_repeater(RPT_INT_INFO, "++\n");

	pm_runtime_disable(&pdev->dev);

	if (repeater_dev) {
		misc_deregister(&repeater_dev->misc_dev);
		kfree(repeater_dev);
	}

	print_repeater(RPT_INT_INFO, "--\n");

	return 0;
}

static void repeater_shutdown(struct platform_device *pdev)
{
	print_repeater(RPT_INT_INFO, "++\n");

	print_repeater(RPT_INT_INFO, "--\n");
}

static const struct of_device_id exynos_repeater_match[] = {
	{
		.compatible = "samsung,exynos-repeater",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_repeater_match);

static struct platform_driver repeater_driver = {
	.probe		= repeater_probe,
	.remove		= repeater_remove,
	.shutdown	= repeater_shutdown,
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_repeater_match),
	}
};

static int exynos_repeater_register(void)
{
	platform_driver_register(&repeater_driver);
	return 0;
}

static void exynos_repeater_unregister(void)
{
	platform_driver_unregister(&repeater_driver);
}

module_init(exynos_repeater_register);
module_exit(exynos_repeater_unregister);
MODULE_AUTHOR("Shinwon Lee <shinwon.lee@samsung.com>");
MODULE_DESCRIPTION("EXYNOS repeater driver");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_LICENSE("GPL");
