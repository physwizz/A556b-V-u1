/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Headef file for DPU debug.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_DRM_DEBUG_H__
#define __EXYNOS_DRM_DEBUG_H__

#include <linux/iommu.h>
#include <drm/drm_rect.h>
#include <exynos_drm_modifier.h>
#include <exynos_drm_bts.h>

#include <soc/samsung/exynos/memlogger.h>

/**
 * Display Subsystem event management status.
 *
 * These status labels are used internally by the DECON to indicate the
 * current status of a device with operations.
 */

/**
 * FENCE_FMT - printk string for &struct dma_fence
 */
#define FENCE_FMT "name:%-15s timeline:%-20s ctx:%-5llu seqno:%-5llu flags:%#lx"
/**
 * FENCE_ARG - printk arguments for &struct dma_fence
 * @f: dma_fence struct
 */
#define FENCE_ARG(f)	(f)->ops->get_driver_name((f)),		\
			(f)->ops->get_timeline_name((f)),	\
			(f)->context, (f)->seqno, (f)->flags

/**
 * STATE_FMT - printk string for &struct drm_crtc_state
 */
#define STATE_FMT "en(%d) act(%d) sr_act(%d) changed[p:%d m:%d a:%d conn:%d color:%d]"
/**
 * STATE_ARG - printk arguments for &struct drm_crtc_state
 * @s: drm_crtc_state struct
 */
#define STATE_ARG(s) (s)->enable, (s)->active, (s)->self_refresh_active,\
	(s)->planes_changed, (s)->mode_changed, (s)->active_changed, \
	(s)->connectors_changed, (s)->color_mgmt_changed

/**
 * FREQ_FMT - printk string for &struct dpu_bts to print mif, int and disp clk
 */
#define FREQ_FMT "mif(%lu) int(%lu) disp(%lu)"
/**
 * FREQ_ARG - printk arguments for &struct dpu_bts to print mif, int and disp clk
 * @b: dpu_bts struct
 */
#define FREQ_ARG(b)						\
	exynos_devfreq_get_domain_freq((b)->df_mif_idx),	\
	exynos_devfreq_get_domain_freq((b)->df_int_idx),	\
	exynos_devfreq_get_domain_freq((b)->df_disp_idx)	\

/**
 * KTIME_SEC_FMT - printk string for ktime_t
 */
#define KTIME_SEC_FMT "[%lld.%03lldsec]"
/**
 * KTIME_SEC_ARG - printk arguments for ktime_t
 */
#define KTIME_SEC_ARG(nsec)			\
	(nsec) / NSEC_PER_SEC, ktime_to_ms((nsec) % NSEC_PER_SEC)

/**
 * KTIME_MSEC_FMT - printk string for ktime_t
 */
#define KTIME_MSEC_FMT "[%lld.%03lldmsec]"
/**
 * KTIME_MSEC_ARG - printk arguments for ktime_t
 */
#define KTIME_MSEC_ARG(nsec)			\
	(nsec) / NSEC_PER_MSEC, ktime_to_us((nsec) % NSEC_PER_MSEC)

/**
 * VBLANK_FMT - printk string for vblank
 */
#define VBLANK_FMT "vbl_cnt[%lld] time"KTIME_SEC_FMT" period"KTIME_MSEC_FMT
/**
 * VBLANK_ARG - printk arguments for vblank
 */
#define VBLANK_ARG(cnt, nsec, period_nsec)			\
	(cnt), KTIME_SEC_ARG(nsec), KTIME_MSEC_ARG(period_nsec)


typedef size_t (*dpu_data_to_string)(void *src, void *buf, size_t remained);

struct drm_crtc;
struct exynos_drm_crtc;
int dpu_init_debug(struct exynos_drm_crtc *exynos_crtc);

size_t dpu_rsc_ch_to_string(void *src, void *buf, size_t remained);
size_t dpu_rsc_win_to_string(void *src, void *buf, size_t remained);
size_t dpu_config_to_string(void *src, void *buf, size_t remained);
size_t dpu_dsi_packet_to_string(void *src, void *buf, size_t remained);
#define EVENT_FLAG_REPEAT	(1 << 0)
#define EVENT_FLAG_ERROR	(1 << 1)
#define EVENT_FLAG_FENCE	(1 << 2)
#define EVENT_FLAG_LONG		(1 << 3)
#define EVENT_FLAG_MASK		(EVENT_FLAG_REPEAT | EVENT_FLAG_ERROR |\
				EVENT_FLAG_FENCE | EVENT_FLAG_LONG)
void DPU_EVENT_LOG(const char *name, struct exynos_drm_crtc *exynos_crtc,
		u32 flag, void *fmt, ...);

void dpu_print_eint_state(struct drm_crtc *crtc);
void dpu_check_panel_status(struct drm_crtc *crtc);
void dpu_profile_hiber_enter(struct exynos_drm_crtc *exynos_crtc);
void dpu_profile_hiber_exit(struct exynos_drm_crtc *exynos_crtc);
void dpu_profile_hiber_inc_frame_cnt(struct exynos_drm_crtc *exynos_crtc);

static __always_inline const char *get_comp_src_name(u64 comp_src)
{
	if (comp_src == AFBC_FORMAT_MOD_SOURCE_GPU)
		return "GPU";
	else if (comp_src == AFBC_FORMAT_MOD_SOURCE_G2D)
		return "G2D";
	else
		return "";
}

struct memlog;
struct memlog_obj;

#define DPU_EVENT_MAX_LEN	(25)
#define DPU_EVENT_KEEP_CNT	(10)
struct dpu_memlog_event {
	struct memlog_obj *obj;
	spinlock_t slock;
	size_t last_event_len;
	char last_event[DPU_EVENT_MAX_LEN];
	char prefix[DPU_EVENT_MAX_LEN];
	u32 repeat_cnt;
};

struct dpu_memlog {
	struct memlog *desc;
	struct dpu_memlog_event event_log;
	struct dpu_memlog_event fevent_log;
};

struct dpu_debug {
	/* ioremap region to read TE PEND register */
	void __iomem *eint_pend;

	/* ioremap region to read TE level register */
	void __iomem *te_lvl;

	/* count of underrun interrupt */
	u32 underrun_cnt;

	u32 err_event_cnt;

	struct dpu_memlog memlog;

	/* devfreq index to get current domain freq */
	u32 df_mif_idx;
	u32 df_int_idx;
	u32 df_disp_idx;

#if IS_ENABLED(CONFIG_EXYNOS_ITMON) || IS_ENABLED(CONFIG_EXYNOS_ITMON_V2)
	struct notifier_block itmon_nb;
	bool itmon_notified;
#endif
};

#if IS_ENABLED(CONFIG_EXYNOS_DRM_BUFFER_SANITY_CHECK)
void
exynos_atomic_commit_prepare_buf_sanity(struct drm_atomic_state *old_state);
bool exynos_atomic_commit_check_buf_sanity(struct drm_atomic_state *old_state);
#else
static inline void
exynos_atomic_commit_prepare_buf_sanity(struct drm_atomic_state *old_state) { }
static inline void
exynos_atomic_commit_check_buf_sanity(struct drm_atomic_state *old_state) { }
#endif

extern struct memlog_obj *g_log_obj;
extern struct memlog_obj *g_errlog_obj;
#define DPU_PR_PREFIX		""
#define DPU_PR_FMT		"%s[%d]: %s:%d "
#define DPU_PR_ARG(name, id)	(name), (id), __func__, __LINE__

#define drv_name(d) (((d) && (d)->dev && (d)->dev->driver) ?			\
			(d)->dev->driver->name : "unregistered")
#define dpu_pr_memlog(handle, name, id, memlog_lv, fmt, ...)			\
	do {									\
		if ((handle) && ((handle)->enabled))				\
			memlog_write_printf((handle), (memlog_lv),		\
				DPU_PR_PREFIX DPU_PR_FMT fmt,			\
				DPU_PR_ARG((name), (id)), ##__VA_ARGS__);	\
	} while (0)

#define dpu_pr_err(name, id, log_lv, fmt, ...)					\
	do {									\
		if ((log_lv) >= 3)						\
			pr_err(DPU_PR_PREFIX DPU_PR_FMT fmt,			\
				DPU_PR_ARG((name), (id)), ##__VA_ARGS__);	\
										\
		dpu_pr_memlog((g_log_obj), (name), (id), MEMLOG_LEVEL_ERR,	\
				fmt, ##__VA_ARGS__);				\
		dpu_pr_memlog((g_errlog_obj), (name), (id), MEMLOG_LEVEL_ERR,	\
				fmt, ##__VA_ARGS__);				\
	} while (0)

#define dpu_pr_warn(name, id, log_lv, fmt, ...)					\
	do {									\
		if ((log_lv) >= 5)						\
			pr_warn(DPU_PR_PREFIX DPU_PR_FMT fmt,			\
				DPU_PR_ARG((name), (id)), ##__VA_ARGS__);	\
										\
		dpu_pr_memlog((g_log_obj), (name), (id), MEMLOG_LEVEL_CAUTION,	\
				fmt, ##__VA_ARGS__);				\
	} while (0)

#define dpu_pr_info(name, id, log_lv, fmt, ...)					\
	do {									\
		if ((log_lv) >= 6)						\
			pr_info(DPU_PR_PREFIX DPU_PR_FMT fmt,			\
				DPU_PR_ARG((name), (id)), ##__VA_ARGS__);	\
										\
		dpu_pr_memlog((g_log_obj), (name), (id), MEMLOG_LEVEL_INFO,	\
				fmt, ##__VA_ARGS__);				\
	} while (0)

#define dpu_pr_debug(name, id, log_lv, fmt, ...)				\
	do {									\
		if ((log_lv) >= 7)						\
			pr_info(DPU_PR_PREFIX DPU_PR_FMT fmt,			\
				DPU_PR_ARG((name), (id)), ##__VA_ARGS__);	\
										\
		dpu_pr_memlog((g_log_obj), (name), (id), MEMLOG_LEVEL_DEBUG,	\
				fmt, ##__VA_ARGS__);				\
	} while (0)

typedef int (*dpu_fault_handler_t)(void *);
void *exynos_drm_create_dpu_fault_context(struct drm_device *drm_dev);
int exynos_drm_register_dpu_fault_handler(struct drm_device *drm_dev,
					dpu_fault_handler_t handler,
					void *ctx);
void exynos_drm_report_dpu_fault(struct exynos_drm_crtc *exynos_crtc);
#endif /* __EXYNOS_DRM_DEBUG_H__ */
