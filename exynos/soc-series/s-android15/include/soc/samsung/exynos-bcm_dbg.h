/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __EXYNOS_BCM_DBG_H_
#define __EXYNOS_BCM_DBG_H_

#include <dt-bindings/soc/samsung/exynos-bcm_dbg.h>

#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG_GNR)
#define BCM_BIN_SIZE				(SZ_128K)
#define BCM_BIN_NAME				"/system/vendor/firmware/fimc_is_lib.bin"
#endif

#define ERRCODE_ITMON_SLVERR			(0)
#define ERRCODE_ITMON_TIMEOUT			(6)

#define EXYNOS_BCM_DBG_MODULE_NAME		"exynos-bcm_dbg"
#define BCM_DSS_NAME				"log_bcm"

#define EXYNOS_BCM_CMD_LOW_MASK			(0x0000FFFF)
#define EXYNOS_BCM_CMD_HIGH_MASK		(0xFFFF0000)
#define EXYNOS_BCM_U64_LOW_MASK			(0x00000000FFFFFFFF)
#define EXYNOS_BCM_U64_HIGH_MASK		(0xFFFFFFFF00000000)
#define EXYNOS_BCM_32BIT_SHIFT			(32)
#define EXYNOS_BCM_KTIME_SIZE			(0x8)

#define CMD_DATA_MAX				(4)
#define BCM_DEFAULT_TIMER_PERIOD                (1)
#define BCM_TIMER_PERIOD_MAX                    (5000)
#define BCM_TIMER_PERIOD_MIN                    (1)

/* IPC common definition */
#define BCM_ONE_BIT_MASK			(0x1)
#define BCM_ERR_MASK				(0x7)
#define BCM_ERR_SHIFT				(13)
#define BCM_CMD_ID_MASK                         (0xF)
#define BCM_CMD_ID_SHIFT                        (0)

/* IPC_AP_BCM_PD definition */
#define BCM_PD_INFO_MAX				(36)
#define BCM_PD_INFO_MASK			(0x3F)
#define BCM_PD_INFO_SHIFT			(0)
#define BCM_PD_ON_SHIFT				(6)

/* IPC_AP_BCM_EVENT definition */
#define BCM_EVT_EVENT_MAX			(8)
#define BCM_EVT_ID_MASK				(0x1F)
#define BCM_EVT_ID_SHIFT			(0)
#define BCM_EVT_DIR_SHIFT			(5)
#define BCM_EVT_PRE_DEFINE_MASK			(0x7)
#define BCM_EVT_PRE_DEFINE_SHIFT		(6)
#define BCM_IP_RANGE_SHIFT			(9)
#define BCM_IP_MASK				(0x7F)
#define BCM_IP_SHIFT				(10)
#define BCM_EVT_EVENT_MASK			(0xFF)
#define BCM_EVT_EVENT_SHIFT(x)			(((x) >= 4) ? (((x) * 8) - 32) : ((x) * 8))
#define BCM_EVT_FLT_ACT_SHIFT(x)		(x)
#define BCM_EVT_FLT_OTHR_MAX			(2)
#define BCM_EVT_FLT_OTHR_TYPE_MASK		(0x1F)
#define BCM_EVT_FLT_OTHR_MASK_MASK		(0x1F)
#define BCM_EVT_FLT_OTHR_VALUE_MASK		(0x1F)
#define BCM_EVT_FLT_OTHR_TYPE_SHIFT(x)		(((x) == 1) ? 26 : 10)
#define BCM_EVT_FLT_OTHR_MASK_SHIFT(x)		(((x) == 1) ? 21 : 5)
#define BCM_EVT_FLT_OTHR_VALUE_SHIFT(x)		(((x) == 1) ? 16 : 0)
#define BCM_EVT_RUN_CONT_SHIFT			(7)
#define BCM_EVT_RUN_CONT_MASK			(0x3)
#define BCM_EVT_PERIOD_CONT_MASK		(0xFFFFF)
#define BCM_EVT_PERIOD_CONT_SHIFT		(0)
#define BCM_EVT_MODE_CONT_MASK			(0x7)
#define BCM_EVT_MODE_CONT_SHIFT			(7)
#define BCM_EVT_STR_STATE_SHIFT			BCM_EVT_RUN_CONT_SHIFT
#define BCM_EVT_IP_CONT_SHIFT			BCM_EVT_RUN_CONT_SHIFT
#define BCM_EVT_GLBAUTO_SHIFT			BCM_EVT_RUN_CONT_SHIFT
/* ESCA parameter */
#define BCM_EVT_IPCTYPE_SHIFT			(17)

/* 2.7c, 2.7d */
#define BCM_IP_D0_INDEX				7
#define BCM_IP_D1_INDEX				8
#define BCM_IP_D2_INDEX				9
#define BCM_IP_D3_INDEX				10
#define BCM_IP_DPUF0D0_INDEX			13
#define BCM_IP_DPUF0D1_INDEX			14
#define BCM_IP_DPUF1D0_INDEX			15
#define BCM_IP_DPUF1D1_INDEX			16

#define BCM_CMD_GET(cmd_data, mask, shift)	((cmd_data & (mask << shift)) >> shift)
#define BCM_CMD_CLEAR(mask, shift)		(~(mask << shift))
#define BCM_CMD_SET(data, mask, shift)		((data & mask) << shift)

#undef BCM_DBGGEN
#ifdef BCM_DBGGEN
#define BCM_DBG(x...)	pr_info("bcm_dbg: " x)
#else
#define BCM_DBG(x...)	do {} while (0)
#endif

#define BCM_INFO(x...)	pr_info("bcm_info: " x)
#define BCM_ERR(x...)	pr_err("bcm_err: " x)

enum exynos_bcm_dbg_ipc_type {
	IPC_BCM_DBG_EVENT = 0,
	IPC_BCM_DBG_PD,
	IPC_BCM_DBG_MAX
};

enum exynos_bcm_err_code {
	E_OK = 0,
	E_INVAL,
	E_BUSY,
};

enum exynos_bcm_event_dir {
	BCM_EVT_GET = 0,
	BCM_EVT_SET,
};

enum exynos_bcm_event_id {
	BCM_EVT_PRE_DEFINE = 0,
	BCM_EVT_EVENT,
	BCM_EVT_RUN_CONT,
	BCM_EVT_IP_CONT,
	BCM_EVT_PERIOD_CONT,
	BCM_EVT_MODE_CONT,
	BCM_EVT_EVENT_FLT_ID,
	BCM_EVT_EVENT_FLT_OTHERS,
	BCM_EVT_EVENT_SAMPLE_ID,
	BCM_EVT_STR_STATE,
	BCM_EVT_DUMP_ADDR,
	BCM_EVT_GLBAUTO_CONT,
#if !IS_ENABLED(CONFIG_EXYNOS_BCM_DBG_GNR)
	BCM_EVT_KTIME_SYNC,
	BCM_EVT_RTC_SYNC,
#endif
	BCM_EVT_FLOW_CTRL,
	BCM_EVT_MAX,
};

enum exynos_bcm_ip_range {
	BCM_EACH = 0,
	BCM_ALL,
	BCM_RANGE_MAX,
};

enum bcm_flow_state {
	BCM_FLOW_ONGOING = 0,
	BCM_FLOW_PRE_PAUSE,
	BCM_FLOW_PAUSED,
};


struct exynos_bcm_dump_addr {
	u32				p_addr;
	u32				p_size;
	u32				buff_size;
	void __iomem			*v_addr;
};

struct exynos_bcm_ipc_base_info {
	enum exynos_bcm_event_dir	direction;
	enum exynos_bcm_event_id	event_id;
	enum exynos_bcm_ip_range	ip_range;
};

struct exynos_bcm_pd_info {
	char				*pd_name;
	unsigned int			cal_pdid;
	unsigned int			pd_index;
	bool				on;
};

struct exynos_bcm_filter_id {
	unsigned int			sm_id_mask;
	unsigned int			sm_id_value;
	unsigned int			sm_id_active[BCM_EVT_EVENT_MAX];
};

struct exynos_bcm_filter_others {
	unsigned int			sm_other_type[BCM_EVT_FLT_OTHR_MAX];
	unsigned int			sm_other_mask[BCM_EVT_FLT_OTHR_MAX];
	unsigned int			sm_other_value[BCM_EVT_FLT_OTHR_MAX];
	unsigned int			sm_other_active[BCM_EVT_EVENT_MAX];
};

struct exynos_bcm_sample_id {
	unsigned int			peak_mask;
	unsigned int			peak_id;
	unsigned int			peak_enable[BCM_EVT_EVENT_MAX];
};

struct exynos_bcm_event {
	unsigned int			index;
	unsigned int			event[BCM_EVT_EVENT_MAX];
};

struct exynos_bcm_dbg_data {
	struct device			*dev;
	struct mutex			lock;

	struct exynos_bcm_dump_addr	dump_addr;
	bool				dump_klog;

	struct device_node		*ipc_node;
	unsigned int			ipc_ch_num;
	unsigned int			ipc_size;
	struct device_node		*ipc_noti_node;
	unsigned int			ipc_noti_ch_num;
	unsigned int			ipc_noti_size;
	struct exynos_bcm_pd_info	*pd_info[BCM_PD_INFO_MAX];
	unsigned int			pd_size;

	struct exynos_bcm_event		define_event[PRE_DEFINE_EVT_MAX];
	unsigned int			default_define_event;
	unsigned int			define_event_max;

	struct exynos_bcm_filter_id	define_filter_id[PRE_DEFINE_EVT_MAX];
	struct exynos_bcm_filter_others define_filter_others[PRE_DEFINE_EVT_MAX];
	struct exynos_bcm_sample_id	define_sample_id[PRE_DEFINE_EVT_MAX];

	unsigned int			bcm_ip_nr;
	unsigned int			bcm_ip_print_nr;
	unsigned int			initial_bcm_run;
	unsigned int			initial_period;
	unsigned int			initial_bcm_mode;
	unsigned int			*initial_run_ip;
	unsigned int			glb_auto_en;

	unsigned int			bcm_run_state;
	bool				available_stop_owner[STOP_OWNER_MAX];
	bool				available_halt_owner[STOP_OWNER_MAX];
#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG_GNR)
	bool				bcm_load_bin;
	struct hrtimer			bcm_hrtimer;
	unsigned int			period;
	unsigned int			bcm_mode;
#endif
	unsigned int			bcm_cnt_nr;
	struct notifier_block		itmon_notifier;
#if defined(CONFIG_CPU_IDLE)
	unsigned int			idle_ip_index;
#endif
#if !IS_ENABLED(CONFIG_EXYNOS_BCM_DBG_GNR)
	void __iomem			*rtc_addr;
#endif
	struct completion		flow_ctrl_completion;
};

struct exynos_bcm_bw_data {
	u64 seq_no;
	u64 dump_time;
	u64 ccnt;
	u64 pmcnt[BCM_EVT_EVENT_MAX];
};

struct exynos_bcm_bw {
	int measure_time;
	unsigned int num_ip;
	unsigned int *ip_idx;
	struct exynos_bcm_bw_data *bw_data;
};

#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG_GNR)
struct cmd_data {
	unsigned int raw_cmd;
	unsigned int cmd[CMD_DATA_MAX];
};

struct bin_system_func {
	int (*send_data)(struct cmd_data *);
	int (*timer_event)(void);
};

struct os_system_func {
	void *(*ioremap)(u64 phys_addr, size_t size);
	void (*iounmap)(volatile void *addr);
	int (*snprint)(char *buf, size_t size, const char *fmt, ...);
	int (*print)(const char *, ...);
	u64 (*sched_clock)(void);
};
#endif

#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG)
int exynos_bcm_dbg_pd_sync(unsigned int cal_pdid, bool on);
void exynos_bcm_dbg_start(void);
void exynos_bcm_dbg_stop(unsigned int bcm_stop_owner);
void exynos_bcm_dbg_get_data(void *bcm);

/* For MIF profiler */
void exynos_bcm_get_data(u64 *rwdata_transfer, u64 *rdata_transfer_cnt, u64
		*rdata_latency_sum, u64 *output_bw);
u64 exynos_bcm_get_ccnt(unsigned int idx);
#else
#define exynos_bcm_dbg_pd(a, b) do {} while (0)
#define exynos_bcm_dbg_ipc_send_data(a, b, c) do {} while (0)
#define exynos_bcm_dbg_start() do {} while (0)
#define exynos_bcm_dbg_stop(a) do {} while (0)
#define exynos_bcm_get_data(a, b, c, d) do {} while (0)
#endif

#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG_GNR)
int __nocfi exynos_bcm_dbg_load_bin(void);
#else
#define exynos_bcm_dbg_load_bin() do {} while (0)
#endif

#endif	/* __EXYNOS_BCM_DBG_H_ */
