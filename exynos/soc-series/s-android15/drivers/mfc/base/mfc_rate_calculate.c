/*
 * drivers/media/platform/exynos/mfc/base/mfc_rate_calculate.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/sort.h>

#include "mfc_rate_calculate.h"
#include "mfc_utils.h"

#define COL_FRAME_RATE		0
#define COL_FRAME_INTERVAL	1

#define MFC_MAX_INTERVAL	(2 * USEC_PER_SEC)

/*
 * A framerate table determines framerate by the interval(us) of each frame.
 * Framerate is not accurate, just rough value to seperate overload section.
 * Base line of each section are selected from middle value.
 * 25fps(40000us), 40fps(25000us), 80fps(12500us)
 * 144fps(6940us), 205fps(4860us), 320fps(3125us)
 *
 * interval(us) | 0         3125          4860          6940          12500         25000        40000
 * framerate    |    480fps   |    240fps   |    180fps   |    120fps   |    60fps    |    30fps   |	24fps
 */
static unsigned long framerate_table[][2] = {
	{  24000, 40000 },
	{  30000, 25000 },
	{  60000, 12500 },
	{ 120000,  6940 },
	{ 180000,  4860 },
	{ 240000,  3125 },
	{ 480000,     0 },
};

/*
 * display_framerate_table determines framerate by the queued interval.
 * It supports 30fps, 60fps, 120fps as display framerate.
 * Base line of each section is selected from middle value.
 * 25fps(40000us), 40fps(25000us), 80fps(12500us)
 *
 * interval(us)     |          12500         25000        40000
 * disp framerate   |   120fps   |    60fps    |    30fps   |	24fps
 */
static unsigned long display_framerate_table[][2] = {
	{  24000, 40000 },
	{  30000, 25000 },
	{  60000, 12500 },
	{ 120000,     0 },
};

inline unsigned long mfc_rate_timespec64_diff(struct timespec64 *to,
					struct timespec64 *from)
{
	unsigned long interval_nsec = (to->tv_sec * NSEC_PER_SEC + to->tv_nsec)
		- (from->tv_sec * NSEC_PER_SEC + from->tv_nsec);

	if (interval_nsec <= 0)
		interval_nsec = MFC_MAX_INTERVAL * 1000;

	return interval_nsec / 1000;
}

static int __mfc_rate_ts_sort(const void *p0, const void *p1)
{
	const int *t0 = p0, *t1 = p1;

	/* ascending sort */
	if (*t0 < *t1)
		return -1;
	else if (*t0 > *t1)
		return 1;

	return 0;
}

static int __mfc_rate_get_ts_min_interval(struct mfc_ctx *ctx, struct mfc_ts_control *ts)
{
	int tmp[MAX_TIME_INDEX];

	if (!ts->ts_is_full)
		return 0;

	memcpy(&tmp[0], &ts->ts_interval_array[0], MAX_TIME_INDEX * sizeof(int));
	sort(tmp, MAX_TIME_INDEX, sizeof(int), __mfc_rate_ts_sort, NULL);

	return tmp[0];
}

static int __mfc_rate_get_ts_interval(struct mfc_ctx *ctx, struct mfc_ts_control *ts, int type)
{
	int tmp[MAX_TIME_INDEX];
	int n, i, val = 0, sum = 0;

	n = ts->ts_is_full ? MAX_TIME_INDEX : ts->ts_count;

	memcpy(&tmp[0], &ts->ts_interval_array[0], n * sizeof(int));
	sort(tmp, n, sizeof(int), __mfc_rate_ts_sort, NULL);

	if (type == MFC_TS_SRC) {
		/* apply median filter for selecting ts interval */
		val = (n <= 2) ? tmp[0] : tmp[n / 2];
	} else if ((type == MFC_TS_DST_Q) || (type == MFC_TS_SRC_Q) || (type == MFC_TS_DST_DQ)) {
		/* apply average for selecting ts interval except min,max */
		if (n < 3)
			return 0;
		for (i = 1; i < (n - 1); i++)
			sum += tmp[i];
		val = sum / (n - 2);
	} else {
		mfc_ctx_err("[TS] Wrong timestamp type %d\n", type);
	}

	if (ctx->dev->debugfs.debug_ts == 1) {
		mfc_ctx_info("==============[%s%s%s][TS] interval (sort)==============\n",
				(type & 0x1) ? "SRC" : "DST", (type & 0x2) ? "_Q" : "",
				(type & 0x4) ? "_DQ" : "");
		for (i = 0; i < n; i++)
			mfc_ctx_info("[%s%s%s][TS] interval [%d] = %d\n",
					(type & 0x1) ? "SRC" : "DST",
					(type & 0x2) ? "_Q" : "",
					(type & 0x4) ? "_DQ" : "", i, tmp[i]);
		mfc_ctx_info("[%s%s%s][TS] get interval %d\n",
					(type & 0x1) ? "SRC" : "DST",
					(type & 0x2) ? "_Q" : "",
					(type & 0x4) ? "_DQ" : "", val);
	}

	return val;
}

static unsigned long __mfc_rate_get_framerate_by_interval(int interval, int type)
{
	unsigned long (*table)[2];
	unsigned long i;
	int size;

	/* if the interval is too big (2sec), framerate set to 0 */
	if (interval > MFC_MAX_INTERVAL || interval <= 0)
		return 0;

	if (type == MFC_TS_SRC || type == MFC_TS_SRC_Q || type == MFC_TS_DST_DQ) {
		table = framerate_table;
		size = ARRAY_SIZE(framerate_table);
	} else if (type == MFC_TS_DST_Q) {
		table = display_framerate_table;
		size = ARRAY_SIZE(display_framerate_table);
	} else {
		return 0;
	}

	for (i = 0; i < size; i++) {
		if (interval > table[i][COL_FRAME_INTERVAL])
			return table[i][COL_FRAME_RATE];
	}

	return 0;
}

int mfc_rate_get_bps_section_by_bps(struct mfc_dev *dev, int Kbps, int max_Kbps)
{
	struct mfc_platdata *pdata = dev->pdata;
	int i, max_freq_idx;
	int freq_ratio, bps_interval, prev_interval = 0;

	max_freq_idx = pdata->num_mfc_freq - 1;
	if (Kbps > max_Kbps) {
		mfc_dev_debug(4, "[BPS] overspec bps %d > %d\n", Kbps, max_Kbps);
		return max_freq_idx;
	}

	for (i = 0; i < pdata->num_mfc_freq; i++) {
		freq_ratio = pdata->mfc_freqs[i] * 100 / pdata->mfc_freqs[max_freq_idx];
		bps_interval = max_Kbps * freq_ratio / 100;
		mfc_dev_debug(4, "[BPS] MFC freq lv%d, %uKHz covered: %d ~ %dKbps (now: %dKbps)\n",
				i, pdata->mfc_freqs[i], prev_interval, bps_interval, Kbps);
		if (Kbps <= bps_interval) {
			mfc_dev_debug(3, "[BPS] MFC freq lv%d, %uKHz is needed: %d ~ %dKbps\n",
					i, pdata->mfc_freqs[i],
					prev_interval, bps_interval);
			return i;
		}
		prev_interval = bps_interval;
	}

	/* Not changed the MFC freq according to BPS */
	return 0;
}

/* Return the minimum interval between previous and next entry */
static int __mfc_rate_get_interval(struct list_head *head, struct list_head *entry)
{
	unsigned long prev_interval = MFC_MAX_INTERVAL, next_interval = MFC_MAX_INTERVAL;
	struct mfc_timestamp *prev_ts, *next_ts, *curr_ts;
	int ret = 0;

	curr_ts = list_entry(entry, struct mfc_timestamp, list);

	if (entry->prev != head) {
		prev_ts = list_entry(entry->prev, struct mfc_timestamp, list);
		prev_interval = mfc_rate_timespec64_diff(&curr_ts->timestamp,
							&prev_ts->timestamp);
		if (prev_interval > MFC_MAX_INTERVAL)
			prev_interval = MFC_MAX_INTERVAL;
	}

	if (entry->next != head) {
		next_ts = list_entry(entry->next, struct mfc_timestamp, list);
		next_interval = mfc_rate_timespec64_diff(&next_ts->timestamp,
							&curr_ts->timestamp);
		if (next_interval > MFC_MAX_INTERVAL)
			next_interval = MFC_MAX_INTERVAL;
	}

	ret = (prev_interval < next_interval) ? prev_interval : next_interval;
	return ret;
}

static int __mfc_rate_add_timestamp(struct mfc_ctx *ctx, struct mfc_ts_control *ts,
			struct timespec64 *time, struct list_head *head)
{
	int replace_entry = 0;
	struct mfc_timestamp *curr_ts = &ts->ts_array[ts->ts_count];
	struct mfc_timestamp *adj_ts = NULL;

	if (ts->ts_is_full) {
		/* Replace the entry if list of array[ts_count] is same as entry */
		if (&curr_ts->list == head)
			replace_entry = 1;
		else
			list_del(&curr_ts->list);
	}

	memcpy(&curr_ts->timestamp, time, sizeof(*time));
	if (!replace_entry)
		list_add(&curr_ts->list, head);
	curr_ts->interval = __mfc_rate_get_interval(&ts->ts_list, &curr_ts->list);
	curr_ts->index = ts->ts_count;

	ts->ts_interval_array[ts->ts_count] = curr_ts->interval;
	ts->ts_count++;

	if (ts->ts_count == MAX_TIME_INDEX) {
		ts->ts_is_full = 1;
		ts->ts_count %= MAX_TIME_INDEX;
	}

	/*
	 * When timestamp is updated, the interval of adjacent timestamp can be changed.
	 * So, update this value of prev and next in list.
	 */
	if (curr_ts->list.next != &ts->ts_list) {
		adj_ts = list_entry(curr_ts->list.next, struct mfc_timestamp, list);
		adj_ts->interval = __mfc_rate_get_interval(&ts->ts_list, &adj_ts->list);
		ts->ts_interval_array[adj_ts->index] = adj_ts->interval;
	}
	if (curr_ts->list.prev != &ts->ts_list) {
		adj_ts = list_entry(curr_ts->list.prev, struct mfc_timestamp, list);
		adj_ts->interval = __mfc_rate_get_interval(&ts->ts_list, &adj_ts->list);
		ts->ts_interval_array[adj_ts->index] = adj_ts->interval;
	}

	return 0;
}

void mfc_rate_reset_ts_list(struct mfc_ts_control *ts)
{
	struct mfc_timestamp *temp_ts = NULL;
	unsigned long flags;

	spin_lock_irqsave(&ts->ts_lock, flags);

	/* empty the timestamp queue */
	while (!list_empty(&ts->ts_list)) {
		temp_ts = list_entry((&ts->ts_list)->next, struct mfc_timestamp, list);
		list_del(&temp_ts->list);
	}

	ts->ts_count = 0;
	ts->ts_is_full = 0;

	spin_unlock_irqrestore(&ts->ts_lock, flags);
}

static unsigned long __mfc_rate_get_fps_by_timestamp(struct mfc_ctx *ctx,
		struct mfc_ts_control *ts, struct timespec64 *time, int type)
{
	struct list_head *head = &ts->ts_list;
	struct mfc_timestamp *temp_ts;
	int found;
	int index = 0;
	int ts_is_full = ts->ts_is_full;
	int interval = MFC_MAX_INTERVAL;
	int min_interval = 0;
	int time_diff;
	unsigned long framerate;
	unsigned long flags;

	if (IS_BUFFER_BATCH_MODE(ctx)) {
		if (ctx->dev->debugfs.debug_ts == 1)
			mfc_ctx_info("[BUFCON][TS] Keep framerate if buffer batch mode is used, %ldfps\n",
					ctx->framerate);
		return ctx->framerate;
	}

	spin_lock_irqsave(&ts->ts_lock, flags);
	if (list_empty(&ts->ts_list)) {
		__mfc_rate_add_timestamp(ctx, ts, time, &ts->ts_list);
		spin_unlock_irqrestore(&ts->ts_lock, flags);
		return __mfc_rate_get_framerate_by_interval(0, type);
	} else {
		found = 0;
		list_for_each_entry_reverse(temp_ts, &ts->ts_list, list) {
			time_diff = __mfc_timespec64_compare(time, &temp_ts->timestamp);
			if (time_diff == 0) {
				/* Do not add if same timestamp already exists */
				found = 1;
				break;
			} else if (time_diff > 0) {
				/* Add this after temp_ts */
				__mfc_rate_add_timestamp(ctx, ts, time, &temp_ts->list);
				found = 1;
				break;
			}
		}

		if (!found)	/* Add this at first entry */
			__mfc_rate_add_timestamp(ctx, ts, time, &ts->ts_list);
	}
	spin_unlock_irqrestore(&ts->ts_lock, flags);

	interval = __mfc_rate_get_ts_interval(ctx, ts, type);
	framerate = __mfc_rate_get_framerate_by_interval(interval, type);

	if (ctx->dev->debugfs.debug_ts == 1) {
		spin_lock_irqsave(&ts->ts_lock, flags);
		/* Debug info */
		mfc_ctx_info("=================[%s%s%s][TS]===================\n",
				(type & 0x1) ? "SRC" : "DST", (type & 0x2) ? "_Q" : "",
				(type & 0x4) ? "_DQ" : "");
		mfc_ctx_info("[%s%s%s][TS] New timestamp = %lld.%06ld, count = %d\n",
			(type & 0x1) ? "SRC" : "DST",
			(type & 0x2) ? "_Q" : "",
			(type & 0x4) ? "_DQ" : "",
			time->tv_sec, time->tv_nsec, ts->ts_count);

		index = 0;
		list_for_each_entry(temp_ts, &ts->ts_list, list) {
			mfc_ctx_info("[%s%s][TS] [%d] timestamp [i:%d]: %lld.%06ld\n",
					(type & 0x1) ? "SRC" : "DST",
					(type & 0x2) ? "_Q" : "",
					index, temp_ts->index,
					temp_ts->timestamp.tv_sec,
					temp_ts->timestamp.tv_nsec);
			index++;
		}
		mfc_ctx_info("[%s%s%s][TS] Min interval = %d, It is %ld fps\n",
				(type & 0x1) ? "SRC" : "DST",
				(type & 0x2) ? "_Q" : "",
				(type & 0x4) ? "_DQ" : "",
				interval, framerate);
		spin_unlock_irqrestore(&ts->ts_lock, flags);
	}

	/* Calculation the last frame fps for drop control */
	if (ctx->type == MFCINST_ENCODER) {
		temp_ts = list_entry(head->prev, struct mfc_timestamp, list);
		if (temp_ts->interval > USEC_PER_SEC) {
			if (ts->ts_is_full)
				mfc_ctx_info("[TS] ts interval(%d) couldn't over 1sec(1fps)\n",
						temp_ts->interval);
			ts->ts_last_interval = 0;
		} else {
			ts->ts_last_interval = temp_ts->interval;
		}
	}

	if (!ts->ts_is_full) {
		if (ctx->dev->debugfs.debug_ts == 1)
			mfc_ctx_info("[TS] ts doesn't full, keep %ld fps\n", ctx->framerate);
		return ctx->framerate;
	}

	if (!ts_is_full && type == MFC_TS_SRC) {
		min_interval = __mfc_rate_get_ts_min_interval(ctx, ts);
		if (min_interval)
			ctx->max_framerate = __mfc_rate_get_framerate_by_interval(min_interval, type);
		else
			ctx->max_framerate = 0;
	}

	return framerate;
}

static int __mfc_rate_get_bps_section(struct mfc_ctx *ctx, u32 bytesused)
{
	struct mfc_dev *dev = ctx->dev;
	struct list_head *head = &ctx->bitrate_list;
	struct mfc_bitrate *temp_bitrate;
	struct mfc_bitrate *new_bitrate = &ctx->bitrate_array[ctx->bitrate_index];
	int max_Kbps;
	unsigned long sum_size = 0, avg_Kbits, fps;
	int count = 0;

	if (ctx->bitrate_is_full) {
		temp_bitrate = list_entry(head->next, struct mfc_bitrate, list);
		list_del(&temp_bitrate->list);
	}

	new_bitrate->bytesused = bytesused;
	list_add_tail(&new_bitrate->list, head);

	list_for_each_entry(temp_bitrate, head, list) {
		mfc_ctx_debug(4, "[BPS][%d] strm_size %d\n", count, temp_bitrate->bytesused);
		sum_size += temp_bitrate->bytesused;
		count++;
	}

	if (count == 0) {
		mfc_ctx_err("[BPS] There is no list for bps\n");
		return ctx->last_bps_section;
	}

	ctx->bitrate_index++;
	if (ctx->bitrate_index == MAX_TIME_INDEX) {
		ctx->bitrate_is_full = 1;
		ctx->bitrate_index %= MAX_TIME_INDEX;
	}

	/*
	 * When there is a value of ts_is_full,
	 * we can trust fps(trusted fps calculated by timestamp diff).
	 * When fps information becomes reliable,
	 * we will start QoS handling by obtaining bps section.
	 */
	if (!ctx->src_ts.ts_is_full)
		return 0;

	if (IS_MULTI_MODE(ctx))
		fps = ctx->last_framerate / 1000 / dev->num_core;
	else
		fps = ctx->last_framerate / 1000;
	avg_Kbits = ((sum_size * BITS_PER_BYTE) / count) / 1024;
	ctx->Kbps = (int)(avg_Kbits * fps);
	max_Kbps = dev->pdata->mfc_resource[ctx->codec_mode].max_Kbps;
	mfc_ctx_debug(3, "[BPS] %d Kbps, average %lu Kbits per frame\n", ctx->Kbps, avg_Kbits);

	return mfc_rate_get_bps_section_by_bps(dev, ctx->Kbps, max_Kbps);
}

void mfc_rate_update_bitrate(struct mfc_ctx *ctx, u32 bytesused)
{
	int bps_section;

	/* bitrate is updated */
	bps_section = __mfc_rate_get_bps_section(ctx, bytesused);
	if (ctx->last_bps_section != bps_section) {
		mfc_ctx_debug(2, "[BPS] bps section changed: %d -> %d\n",
				ctx->last_bps_section, bps_section);
		ctx->last_bps_section = bps_section;
		ctx->update_bitrate = true;
	}
}

static bool __mfc_rate_update_boost_mode(struct mfc_ctx *ctx, unsigned long framerate)
{
	u64 ktime, duration;

	/* 1) check no boosting condition */
	if (ctx->dst_q_ts.ts_is_full && !ctx->boosting_time)
		return false;

	ktime = ktime_get_ns();

	/* 2) check started boosting period */
	if (ctx->boosting_time && (ktime < ctx->boosting_time)) {
		mfc_ctx_debug(4, "[BOOST] seeking booster on-going %llu.%06llu until %llu.%06llu\n",
			(ktime / NSEC_PER_SEC),
			(ktime - ((ktime / NSEC_PER_SEC) * NSEC_PER_SEC)) / NSEC_PER_USEC,
			(ctx->boosting_time / NSEC_PER_SEC),
			(ctx->boosting_time - ((ctx->boosting_time / NSEC_PER_SEC) * NSEC_PER_SEC))
			/ NSEC_PER_USEC);
		return true;
	}

	/* 3) check boosting condition: seek */
	if (!ctx->dst_q_ts.ts_is_full && !ctx->boosting_time) {
		if (UNDER_FHD_RES(ctx) && (framerate <= MFC_MIN_FPS))
			duration = (MFC_BOOST_TIME / 2) * NSEC_PER_MSEC;
		else
			duration = MFC_BOOST_TIME * NSEC_PER_MSEC;
		if (ctx->dev->debugfs.boost_time)
			duration = ctx->dev->debugfs.boost_time * NSEC_PER_MSEC;
		ctx->boosting_time = ktime + duration;
		mfc_ctx_debug(2, "[BOOST] seeking booster start %llu.%06llu ~ %llu.%06llu (during %llums)\n",
			(ktime / NSEC_PER_SEC),
			(ktime - ((ktime / NSEC_PER_SEC) * NSEC_PER_SEC)) / NSEC_PER_USEC,
			(ctx->boosting_time / NSEC_PER_SEC),
			(ctx->boosting_time - ((ctx->boosting_time / NSEC_PER_SEC) * NSEC_PER_SEC))
			/ NSEC_PER_USEC, duration / NSEC_PER_MSEC);
		return true;
	}

	/* 4) boosting end */
	if (ctx->boosting_time)
		mfc_ctx_debug(2, "[BOOST] seeking booster terminated %llu.%06llu\n",
			(ktime / NSEC_PER_SEC),
			(ktime - ((ktime / NSEC_PER_SEC) * NSEC_PER_SEC)) / NSEC_PER_USEC);

	ctx->boosting_time = 0;

	return false;
}

void mfc_rate_update_framerate(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	unsigned long framerate;

#if IS_ENABLED(CONFIG_EXYNOS_THERMAL_V2)
	/* 1) check tmu for recording scenario */
	if (ctx->type == MFCINST_ENCODER && ctx->dev->tmu_fps) {
		if (ctx->framerate != ctx->dev->tmu_fps) {
			mfc_ctx_debug(2, "[QoS][TMU] forcibly fps changed %ld -> %d\n",
					ctx->framerate, ctx->dev->tmu_fps);
			ctx->framerate = ctx->dev->tmu_fps;
			ctx->update_framerate = true;
		}
		return;
	}
#endif

	/* 2) when src timestamp isn't full, only check operating framerate by user */
	if (!ctx->src_ts.ts_is_full) {
		if (ctx->operating_framerate && (ctx->operating_framerate > ctx->framerate)) {
			mfc_ctx_debug(2, "[QoS] operating fps changed: %ld\n", ctx->operating_framerate);
			mfc_rate_set_framerate(ctx, ctx->operating_framerate);
			ctx->update_framerate = true;
			return;
		}
	} else {
		/* 3) get src framerate */
		framerate = ctx->last_framerate;

		/* 4) check display framerate */
		if (dev->pdata->display_framerate &&
			ctx->dst_q_ts.ts_is_full && (ctx->dst_q_framerate > framerate)) {
			framerate = ctx->dst_q_framerate;
			if (framerate != ctx->framerate)
				mfc_ctx_debug(2, "[QoS] display fps %ld\n", framerate);
		}

		/* 5) check operating framerate by user */
		if (ctx->operating_framerate && (ctx->operating_framerate > framerate)) {
			framerate = ctx->operating_framerate;
			if (framerate != ctx->framerate)
				mfc_ctx_debug(2, "[QoS] operating fps %ld\n", framerate);
		}

		/* 6) check boosting mode */
		if ((ctx->type == MFCINST_DECODER) && (ctx->frame_cnt >= MFC_INITIAL_SKIP_FRAME)) {
			if (__mfc_rate_update_boost_mode(ctx, framerate)) {
				/* x8 boosting (30fps -> 240fps, 60fps -> 480fps) */
				if (dev->debugfs.boost_speed)
					framerate = framerate * dev->debugfs.boost_speed;
				else
					framerate = framerate * MFC_BOOST_SPEED;
				if (framerate != ctx->framerate)
					mfc_ctx_debug(2, "[QoS][BOOST] boosting fps %ld\n", framerate);
			}
		}

		/* 7) check non-real-time and undefined mode */
		if ((ctx->rt == MFC_NON_RT) || (ctx->rt == MFC_RT_UNDEFINED)) {
			if (framerate < ctx->src_q_framerate) {
				framerate = ctx->src_q_framerate;
				if (framerate != ctx->framerate)
					mfc_ctx_debug(2, "[QoS][PRIO] (default) NRT src_q fps %ld\n",
							framerate);
			}
		}

		if ((ctx->operating_framerate == ctx->framerate) && !ctx->check_src_ts_full) {
			mfc_ctx_debug(2, "[QoS] src ts is full, update framerate for load balancing\n");
			ctx->check_src_ts_full = true;
			ctx->update_framerate = true;
		}

		if (framerate && (framerate != ctx->framerate)) {
			mfc_ctx_debug(2, "[QoS] fps changed: %ld -> %ld, qos ratio: %d\n",
					ctx->framerate, framerate, ctx->qos_ratio);
			mfc_rate_set_framerate(ctx, framerate);
			ctx->update_framerate = true;
		}
	}
}

void mfc_rate_update_last_framerate(struct mfc_ctx *ctx, u64 timestamp)
{
	struct timespec64 time;

	time.tv_sec = timestamp / NSEC_PER_SEC;
	time.tv_nsec = (timestamp - (time.tv_sec * NSEC_PER_SEC));

	ctx->last_framerate = __mfc_rate_get_fps_by_timestamp(ctx, &ctx->src_ts, &time, MFC_TS_SRC);
	if (ctx->last_framerate > MFC_MAX_FPS)
		ctx->last_framerate = MFC_MAX_FPS;

	if (ctx->src_ts.ts_is_full)
		ctx->last_framerate = (ctx->qos_ratio * ctx->last_framerate) / 100;
}

void mfc_rate_update_bufq_framerate(struct mfc_ctx *ctx, int type)
{
	struct timespec64 time;
	u64 timestamp;

	timestamp = ktime_get_ns();
	time.tv_sec = timestamp / NSEC_PER_SEC;
	time.tv_nsec = timestamp - (time.tv_sec * NSEC_PER_SEC);

	if (type == MFC_TS_DST_Q) {
		ctx->dst_q_framerate = __mfc_rate_get_fps_by_timestamp(ctx, &ctx->dst_q_ts, &time,
				MFC_TS_DST_Q);
		mfc_ctx_debug(5, "[QoS] dst_q_framerate = %lu\n", ctx->dst_q_framerate);
	}
	if (type == MFC_TS_DST_DQ) {
		ctx->dst_dq_framerate = __mfc_rate_get_fps_by_timestamp(ctx, &ctx->dst_dq_ts, &time,
				MFC_TS_DST_DQ);
		mfc_ctx_debug(5, "[QoS] dst_dq_framerate = %lu\n", ctx->dst_dq_framerate);
	}
	if (type == MFC_TS_SRC_Q) {
		ctx->src_q_framerate = __mfc_rate_get_fps_by_timestamp(ctx, &ctx->src_q_ts, &time,
				MFC_TS_SRC_Q);
		mfc_ctx_debug(5, "[QoS] src_q_framerate = %lu\n", ctx->src_q_framerate);
	}
}

void mfc_rate_reset_bufq_framerate(struct mfc_ctx *ctx)
{
	if (ctx->boosting_time)
		mfc_ctx_debug(2, "[BOOST] seeking booster terminated\n");

	ctx->boosting_time = 0;
	ctx->dst_q_framerate = 0;
	ctx->dst_dq_framerate = 0;
	ctx->src_q_framerate = 0;

	mfc_rate_reset_ts_list(&ctx->dst_q_ts);
	mfc_rate_reset_ts_list(&ctx->src_q_ts);
}

int mfc_rate_check_perf_ctx(struct mfc_ctx *ctx, int max_runtime)
{
	struct mfc_ts_control *ts;
	struct timespec64 start_time, curr_time;
	u64 timestamp, interval;
	int op_fps, fps;
	unsigned long flags;

	if (ctx->dev->debugfs.sched_perf_disable)
		return 0;

	if (ctx->boosting_time)
		return 0;

	ts = &ctx->dst_dq_ts;
	if (!ts->ts_is_full)
		return 0;

	op_fps = ctx->operating_framerate;
	if (op_fps == 0) {
		if ((ctx->type == MFCINST_ENCODER) && (ctx->enc_priv->params.rc_framerate))
			op_fps = ctx->enc_priv->params.rc_framerate;
		else
			/*
			 * In case of non-real-time, check the buffer queueing rate
			 * because the non-real-time is best effort scenario. (ex. video editing)
			 */
			op_fps = ctx->src_q_framerate;
		mfc_ctx_debug(2, "[PRIO][rt %d] use fps: %d\n", ctx->rt, op_fps);
	}

	/* Calculate interval from start to current time */
	timestamp = ktime_get_ns();
	curr_time.tv_sec = timestamp / NSEC_PER_SEC;
	curr_time.tv_nsec = timestamp - (curr_time.tv_sec * NSEC_PER_SEC);

	spin_lock_irqsave(&ts->ts_lock, flags);
	start_time = ts->ts_array[ts->ts_count].timestamp;
	spin_unlock_irqrestore(&ts->ts_lock, flags);

	interval = mfc_rate_timespec64_diff(&curr_time, &start_time);
	interval += max_runtime;

	fps = (((MAX_TIME_INDEX - 1) * 1000) / (interval / 1000)) * 1000;
	mfc_ctx_debug(2, "[PRIO][rt %d] st %lld.%06ld, curr %lld.%06ld, interval %lld, fps %d\n",
			ctx->rt, start_time.tv_sec, start_time.tv_nsec,
			curr_time.tv_sec, curr_time.tv_nsec, interval, fps);

	if (fps > op_fps) {
		mfc_ctx_debug(2, "[PRIO] PERF enough fps: %d, op_fps: %d\n", fps, op_fps);
		return 1;
	}

	mfc_ctx_debug(2, "[PRIO] PERF insufficient fps: %d, op_fps: %d\n", fps, op_fps);
	return 0;
}
