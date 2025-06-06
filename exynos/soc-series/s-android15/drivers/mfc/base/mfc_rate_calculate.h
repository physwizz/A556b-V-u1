/*
 * drivers/media/platform/exynos/mfc/base/mfc_rate_calculate.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_RATE_CALCULATE_H
#define __MFC_RATE_CALCULATE_H __FILE__

#include "mfc_common.h"

#define MFC_MIN_FPS			(30000)
#define MFC_MAX_FPS			(480000)
#define DEC_DEFAULT_FPS			(480000)
#define ENC_DEFAULT_FPS			(480000)
#define ENC_DEFAULT_CAM_CAPTURE_FPS	(60000)

inline unsigned long mfc_rate_timespec64_diff(struct timespec64 *to,
					struct timespec64 *from);
void mfc_rate_reset_ts_list(struct mfc_ts_control *ts);
int mfc_rate_get_bps_section_by_bps(struct mfc_dev *dev, int Kbps, int max_Kbps);
void mfc_rate_update_bitrate(struct mfc_ctx *ctx, u32 bytesused);
void mfc_rate_update_framerate(struct mfc_ctx *ctx);
void mfc_rate_update_last_framerate(struct mfc_ctx *ctx, u64 timestamp);
void mfc_rate_update_bufq_framerate(struct mfc_ctx *ctx, int type);
void mfc_rate_reset_bufq_framerate(struct mfc_ctx *ctx);
int mfc_rate_check_perf_ctx(struct mfc_ctx *ctx, int max_runtime);

static inline int __mfc_timespec64_compare(const struct timespec64 *lhs, const struct timespec64 *rhs)
{
	if (lhs->tv_sec < rhs->tv_sec)
		return -1;
	if (lhs->tv_sec > rhs->tv_sec)
		return 1;
	return lhs->tv_nsec - rhs->tv_nsec;
}

static inline void mfc_rate_reset_framerate(struct mfc_ctx *ctx)
{
	if (ctx->type == MFCINST_DECODER)
		ctx->framerate = DEC_DEFAULT_FPS;
	else if (ctx->type == MFCINST_ENCODER)
		ctx->framerate = ENC_DEFAULT_FPS;

	mfc_ctx_debug(3, "[QoS] reset ctx->framrate: %lu\n", ctx->framerate);
}

static inline void mfc_rate_reset_last_framerate(struct mfc_ctx *ctx)
{
	ctx->last_framerate = 0;
}

static inline void mfc_rate_set_framerate(struct mfc_ctx *ctx, int rate)
{
	ctx->framerate = rate;

	mfc_ctx_debug(3, "[QoS] set ctx->framerate: %lu\n", ctx->framerate);
}

static inline int mfc_rate_get_op_framerate(struct mfc_ctx *ctx)
{
	return ctx->framerate;
}

static inline int mfc_rate_get_framerate(struct mfc_ctx *ctx)
{
	unsigned long framerate = ctx->last_framerate;

	if (!framerate)
		framerate = mfc_rate_get_op_framerate(ctx);

	if (ctx->operating_framerate > framerate)
		return ctx->operating_framerate;
	else
		return framerate;
}

static inline unsigned long mfc_rate_get_rt_framerate(struct mfc_ctx *ctx, enum mfc_real_time rt)
{
	unsigned long framerate;

	framerate = ctx->operating_framerate;

	if (rt == MFC_RT) {
		if ((framerate == 0) && (ctx->type == MFCINST_ENCODER) &&
				(ctx->enc_priv->params.rc_framerate))
			framerate = ctx->enc_priv->params.rc_framerate;
	} else if (rt == MFC_RT_UNDEFINED || rt == MFC_NON_RT) {
		framerate = mfc_rate_get_op_framerate(ctx);
	} else {
		if (ctx->src_ts.ts_is_full)
			framerate = mfc_rate_get_framerate(ctx);
	}

	if (framerate == 0)
		framerate = MFC_MIN_FPS;
	else if (framerate > MFC_MAX_FPS)
		framerate = MFC_MAX_FPS;

	mfc_ctx_debug(3, "[QoS] rt framerate: %lu\n", framerate);

	return framerate;
}

#endif /* __MFC_RATE_CALCULATE_H */
