/******************************************************************************
 *
 * Copyright (c) 2014 - 2023 Samsung Electronics Co., Ltd. All rights reserved
 *
 *****************************************************************************/

#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <scsc/scsc_mx.h>
#include <scsc/scsc_mifram.h>
#include <linux/ktime.h>
#include <linux/rtc.h>
#include <linux/kthread.h>
#include <scsc/scsc_logring.h>
#include <scsc/scsc_warn.h>
#include <linux/cpu.h>
#include <linux/bitmap.h>
#include <linux/minmax.h>

#include "mgt.h"
#include "hip4.h"
#include "mbulk.h"
#include "dev.h"
#include "hip4_sampler.h"
#ifdef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
#include "load_manager.h"
#endif

#ifdef CONFIG_SCSC_WLAN_ANDROID
#include "scsc_wifilogger_rings.h"
#endif
#ifdef CONFIG_SCSC_WLAN_HOST_DPD
#include "dpd_mmap.h"
#endif

#include "debug.h"

#ifdef CONFIG_SLSI_WLAN_LPC
#include "local_packet_capture.h"
#endif

/* Global spinlock to serialize napi context with slsi_hip_deinit*/
static DEFINE_SPINLOCK(in_napi_context);

static bool hip4_system_wq;
module_param(hip4_system_wq, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_system_wq, "Use system wq instead of named workqueue. (default: N)");

#if IS_ENABLED(CONFIG_SCSC_LOGRING)
static bool hip4_dynamic_logging = true;
module_param(hip4_dynamic_logging, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_dynamic_logging, "Dynamic logging, logring is disabled if tput > hip4_qos_med_tput_in_mbps. (default: Y)");

static int hip4_dynamic_logging_tput_in_mbps = 150;
module_param(hip4_dynamic_logging_tput_in_mbps, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_dynamic_logging_tput_in_mbps, "throughput (in Mbps) to apply dynamic logring logging");
#endif

#ifdef CONFIG_SCSC_QOS
static bool hip4_qos_enable = true;
module_param(hip4_qos_enable, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_qos_enable, "enable HIP4 PM QoS. (default: Y)");

#if defined(CONFIG_SOC_S5E9925)
static int hip4_qos_max_tput_in_mbps = 150;
#elif defined(CONFIG_SOC_S5E8835) || defined(CONFIG_SOC_S5E8845)
static int hip4_qos_max_tput_in_mbps = 50;
#else
static int hip4_qos_max_tput_in_mbps = 250;
#endif
module_param(hip4_qos_max_tput_in_mbps, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_qos_max_tput_in_mbps, "throughput (in Mbps) to apply Max PM QoS");

#if defined(CONFIG_SOC_S5E9925)
static int hip4_qos_med_tput_in_mbps = 50;
#elif defined(CONFIG_SOC_S5E8835) || defined(CONFIG_SOC_S5E8845)
static int hip4_qos_med_tput_in_mbps = 30;
#else
static int hip4_qos_med_tput_in_mbps = 150;
#endif
module_param(hip4_qos_med_tput_in_mbps, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_qos_med_tput_in_mbps, "throughput (in Mbps) to apply Median PM QoS");
#endif

#ifdef CONFIG_SCSC_SMAPPER
static bool hip4_smapper_enable = true;
module_param(hip4_smapper_enable, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_smapper_enable, "enable HIP4 SMAPPER. (default: Y)");
static bool hip4_smapper_is_enabled;
#endif

#define SLSI_HIP_NAPI_STATE_ENABLED (0x1)
/* run NAPI poll on a specific CPU (preferably a big CPU if online) */
#ifdef CONFIG_SOC_EXYNOS7885
int napi_select_cpu = 7; /* Big CPU number */
#else
static int napi_select_cpu; /* CPU number */
#endif
module_param(napi_select_cpu, int, 0644);
MODULE_PARM_DESC(napi_select_cpu, "select a specific CPU to execute NAPI poll");

#if defined(CONFIG_SOC_S5E8825) || defined(CONFIG_SOC_S5E8535) || defined(CONFIG_SOC_S5E8835) || defined(CONFIG_SOC_S5E8845) \
|| defined(CONFIG_SOC_S5E8855)
static bool hip4_napi_rx_saturation_detection;
#else
static bool hip4_napi_rx_saturation_detection = true;
#endif
module_param(hip4_napi_rx_saturation_detection, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_napi_rx_saturation_detection, "enable Rx saturation detection");

static uint hip4_napi_num_rx_pkts = 80;
module_param(hip4_napi_num_rx_pkts, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_napi_num_rx_pkts, "number of packets in Rx queue to deem it as full");

static uint hip4_napi_num_rx_full = 3;
module_param(hip4_napi_num_rx_full, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hip4_napi_num_rx_full, "number of iterations of Rx queue full that signal to move to high performance");

static int max_buffered_frames = 10000;
module_param(max_buffered_frames, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(max_buffered_frames, "Maximum number of frames to buffer in the driver");

static ktime_t intr_received_fb;
static ktime_t bh_init_fb;
static ktime_t bh_end_fb;
static ktime_t intr_received_ctrl;
static ktime_t bh_init_ctrl;
static ktime_t bh_end_ctrl;
static ktime_t intr_received_data;
static ktime_t bh_init_data;
static ktime_t bh_end_data;
static ktime_t wdt;
static ktime_t send;
static ktime_t closing;

enum rw {
	widx,
	ridx,
};

static u8 hip4_read_index(struct slsi_hip *hip, u32 q, enum rw r_w);

/* Q mapping V3 - V4 */
/*offset of F/W owned indices */
#define FW_OWN_OFS      (64)
/**
 * HIP queue indices layout in the scoreboard (SC-505612-DD). v3
 *
 *             3        2        1        0
 *         +-----------------------------------+
 *     +0  |  Q3R   |   Q2R  |  Q1W   |  Q0W   |  Owned by the host
 *         +-----------------------------------+
 *     +4  |        |        |  Q5W   |  Q4R   |  Owned by the host
 *         +-----------------------------------+
 *
 *         +-----------------------------------+
 *    +64  |  Q3W   |   Q2W  |  Q1R   |  Q0R   |  Owned by the F/W
 *         +-----------------------------------+
 *    +68  |        |        |  Q5R   |  Q4W   |  Owned by the F/W
 *         +-----------------------------------+
 *
 * The queue indcies which owned by the host are only writable by the host.
 * F/W can only read them. And vice versa.
 */
static int q_idx_layout[6][2] = {
	{               0,  FW_OWN_OFS + 0},	/* mif_q_fh_ctl : 0 */
	{               1,  FW_OWN_OFS + 1},	/* mif_q_fh_dat : 1 */
	{  FW_OWN_OFS + 2,               2},	/* mif_q_fh_rfb : 2 */
	{  FW_OWN_OFS + 3,               3},    /* mif_q_th_ctl : 3 */
	{  FW_OWN_OFS + 4,               4},	/* mif_q_th_dat : 4 */
	{               5,  FW_OWN_OFS + 5}	/* mif_q_th_rfb : 5 */
};

/*offset of F/W owned VIF Status */
#define FW_OWN_VIF      (96)
/**
 * HIP Pause state VIF. v4. 2 bits per PEER
 *
 *         +-----------------------------------+
 *    +96  |        VIF[0] Peers [15-1]        |  Owned by the F/W
 *         +-----------------------------------+
 *    +100 |        VIF[0] Peers [31-16]       |  Owned by the F/W
 *         +-----------------------------------+
 *    +104 |        VIF[1] Peers [15-1]        |  Owned by the F/W
 *         +-----------------------------------+
 *    +108 |        VIF[1] Peers [31-16]       |  Owned by the F/W
 *         +-----------------------------------+
 *    +112 |        VIF[2] Peers [15-1]        |  Owned by the F/W
 *         +-----------------------------------+
 *    +116 |        VIF[2] Peers [31-16]       |  Owned by the F/W
 *         +-----------------------------------+
 *    +120 |        VIF[3] Peers [15-1]        |  Owned by the F/W
 *         +-----------------------------------+
 *    +124 |        VIF[3] Peers [31-16]       |  Owned by the F/W
 *         +-----------------------------------+
 *
 */

/* MAX_STORM. Max Interrupts allowed when platform is in suspend */
#define MAX_STORM            5

/* Timeout for Wakelocks in HIP  */
#define SLSI_HIP_WAKELOCK_TIME_OUT_IN_MS   (1000)

#ifdef CONFIG_SCSC_WLAN_DEBUG

static u64 histogram_1;
static u64 histogram_2;
static u64 histogram_3;
static u64 histogram_4;
static u64 histogram_5;
static u64 histogram_6;
static u64 max_jitter;

#define HISTO_1		1000 /* 1 us */
#define HISTO_2		10000 /* 10 us */
#define HISTO_3		100000 /* 100 us */
#define HISTO_4		1000000 /* 1ms */
#define HISTO_5		10000000 /* 10ms */

static u64 histogram_1_data;
static u64 histogram_2_data;
static u64 histogram_3_data;
static u64 histogram_4_data;
static u64 histogram_5_data;
static u64 histogram_6_data;
static u64 max_data;

#define HISTO_1_DATA	50 /* 50 th data packets  */
#define HISTO_2_DATA	100/* 100 th data packets */
#define HISTO_3_DATA	150/* 150 th data packets */
#define HISTO_4_DATA	200/* 200 th data packets */
#define HISTO_5_DATA	250/* 250 th data packets */

/* MAX_HISTORY_RECORDS should be power of two */
#define MAX_HISTORY_RECORDS  32

#define FH                   0
#define TH                   1

struct hip4_history {
	bool    dir;
	u32     signal;
	u32     cnt;
	ktime_t last_time;
} hip4_signal_history[MAX_HISTORY_RECORDS];

static u32 history_record;

/* This function should be called from atomic context */
static void hip4_history_record_add(bool dir, u32 signal_id)
{
	struct hip4_history record;

	record = hip4_signal_history[history_record];

	if (record.signal == signal_id && record.dir == dir) {
		/* If last signal and direction is the same, increment counter */
		record.last_time = ktime_get();
		record.cnt += 1;
		hip4_signal_history[history_record] = record;
		return;
	}

	history_record = (history_record + 1) & (MAX_HISTORY_RECORDS - 1);

	record = hip4_signal_history[history_record];
	record.dir = dir;
	record.signal = signal_id;
	record.cnt = 1;
	record.last_time = ktime_get();
	hip4_signal_history[history_record] = record;
}

#define HIP4_HISTORY(in_seq_file, m, fmt, arg ...)       \
	do {                                             \
		if (in_seq_file)                         \
			seq_printf(m, fmt, ## arg);      \
		else                                     \
			SLSI_ERR_NODEV(fmt, ## arg);     \
	} while (0)

static void hip4_history_record_print(bool in_seq_file, struct seq_file *m)
{
	struct hip4_history record;
	u32 i, pos;
	ktime_t old;

	old = ktime_set(0, 0);

	/* Start with the Next record to print history in order */
	pos = (history_record + 1) & (MAX_HISTORY_RECORDS - 1);

	HIP4_HISTORY(in_seq_file, m, "dir\t signal\t cnt\t last_time(ns) \t\t gap(ns)\n");
	HIP4_HISTORY(in_seq_file, m, "-----------------------------------------------------------------------------\n");
	for (i = 0; i < MAX_HISTORY_RECORDS; i++) {
		record = hip4_signal_history[pos];
		/*next pos*/
		if (record.cnt) {
			HIP4_HISTORY(in_seq_file, m, "%s\t 0x%04x\t %d\t %lld \t%lld\n", record.dir ? "<--TH" : "FH-->",
				     record.signal, record.cnt, ktime_to_ns(record.last_time), ktime_to_ns(ktime_sub(record.last_time, old)));
		}
		old = record.last_time;
		pos = (pos + 1) & (MAX_HISTORY_RECORDS - 1);
	}
}

static int hip4_proc_show_history(struct seq_file *m, void *v)
{
	hip4_history_record_print(true, m);
	return 0;
}

static int hip4_proc_history_open(struct inode *inode, struct file *file)
{
	return single_open(file, hip4_proc_show_history, PDE_DATA(inode));
}

static const struct file_operations hip4_procfs_history_fops = {
	.owner   = THIS_MODULE,
	.open    = hip4_proc_history_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int hip4_proc_show(struct seq_file *m, void *v)
{
	struct slsi_hip *hip = m->private;
	struct hip4_hip_control *hip_control;
	struct slsi_dev         *sdev = container_of(hip, struct slsi_dev, hip);
	u8 i;

	u32 conf_hip4_ver = 0;
	void *hip_ptr;

	if (!hip->hip_priv) {
		seq_puts(m, "HIP4 not active\n");
		return 0;
	}

	conf_hip4_ver = scsc_wifi_get_hip_config_version(&hip->hip_control->init);
	/* Check if the version is supported. And get the index */
	/* This is hardcoded and may change in future versions */
	if (conf_hip4_ver != 4 && conf_hip4_ver != 3) {
		SLSI_ERR_NODEV("FW Version %d not supported or Hip has not been set up\n", conf_hip4_ver);
		return 0;
	}

	/* hip_ref contains the reference of the start of shared memory allocated for WLAN */
	/* hip_ptr is the kernel address of hip_ref*/
	hip_ptr = scsc_mx_service_mif_addr_to_ptr(sdev->service, hip->hip_ref);
	/* Get hip_control pointer on shared memory  */
	hip_control = (struct hip4_hip_control *)(hip_ptr +
		      HIP4_WLAN_CONFIG_OFFSET);

	seq_puts(m, "-----------------------------------------\n");
	seq_puts(m, "HIP4 CONFIG:\n");
	seq_puts(m, "-----------------------------------------\n");
	seq_printf(m, "config kernel addr  = %p\n", hip_control);
	if (conf_hip4_ver == 4) {
		seq_printf(m, "hip4_version_4 addr   = 0x%p\n", &hip_control->config_v4);
		seq_printf(m, "magic_number          = 0x%x\n", hip_control->config_v4.magic_number);
		seq_printf(m, "hip_config_ver        = 0x%x\n", hip_control->config_v4.hip_config_ver);
		seq_printf(m, "config_len            = 0x%x\n", hip_control->config_v4.config_len);
		seq_printf(m, "compat_flag           = 0x%x\n", hip_control->config_v4.compat_flag);
		seq_printf(m, "sap_mlme_ver          = 0x%x\n", hip_control->config_v4.sap_mlme_ver);
		seq_printf(m, "sap_ma_ver            = 0x%x\n", hip_control->config_v4.sap_ma_ver);
		seq_printf(m, "sap_debug_ver         = 0x%x\n", hip_control->config_v4.sap_debug_ver);
		seq_printf(m, "sap_test_ver          = 0x%x\n", hip_control->config_v4.sap_test_ver);
		seq_printf(m, "fw_build_id           = 0x%x\n", hip_control->config_v4.fw_build_id);
		seq_printf(m, "fw_patch_id           = 0x%x\n", hip_control->config_v4.fw_patch_id);
		seq_printf(m, "unidat_req_headroom   = 0x%x\n", hip_control->config_v4.unidat_req_headroom);
		seq_printf(m, "unidat_req_tailroom   = 0x%x\n", hip_control->config_v4.unidat_req_tailroom);
		seq_printf(m, "bulk_buffer_align     = 0x%x\n", hip_control->config_v4.bulk_buffer_align);
		seq_printf(m, "host_cache_line       = 0x%x\n", hip_control->config_v4.host_cache_line);
		seq_printf(m, "host_buf_loc          = 0x%x\n", hip_control->config_v4.host_buf_loc);
		seq_printf(m, "host_buf_sz           = 0x%x\n", hip_control->config_v4.host_buf_sz);
		seq_printf(m, "fw_buf_loc            = 0x%x\n", hip_control->config_v4.fw_buf_loc);
		seq_printf(m, "fw_buf_sz             = 0x%x\n", hip_control->config_v4.fw_buf_sz);
		seq_printf(m, "mib_buf_loc           = 0x%x\n", hip_control->config_v4.mib_loc);
		seq_printf(m, "mib_buf_sz            = 0x%x\n", hip_control->config_v4.mib_sz);
		seq_printf(m, "log_config_loc        = 0x%x\n", hip_control->config_v4.log_config_loc);
		seq_printf(m, "log_config_sz         = 0x%x\n", hip_control->config_v4.log_config_sz);
		seq_printf(m, "mif_fh_int_n          = 0x%x\n", hip_control->config_v4.mif_fh_int_n);
		seq_printf(m, "mif_th_int_n[FH_CTRL] = 0x%x\n", hip_control->config_v4.mif_th_int_n[HIP4_MIF_Q_FH_CTRL]);
		seq_printf(m, "mif_th_int_n[FH_DAT]  = 0x%x\n", hip_control->config_v4.mif_th_int_n[HIP4_MIF_Q_FH_DAT]);
		seq_printf(m, "mif_th_int_n[FH_RFB]  = 0x%x\n", hip_control->config_v4.mif_th_int_n[HIP4_MIF_Q_FH_RFB]);
		seq_printf(m, "mif_th_int_n[TH_CTRL] = 0x%x\n", hip_control->config_v4.mif_th_int_n[HIP4_MIF_Q_TH_CTRL]);
		seq_printf(m, "mif_th_int_n[TH_DAT]  = 0x%x\n", hip_control->config_v4.mif_th_int_n[HIP4_MIF_Q_TH_DAT]);
		seq_printf(m, "mif_th_int_n[TH_RFB]  = 0x%x\n", hip_control->config_v4.mif_th_int_n[HIP4_MIF_Q_TH_RFB]);
		seq_printf(m, "scbrd_loc             = 0x%x\n", hip_control->config_v4.scbrd_loc);
		seq_printf(m, "q_num                 = 0x%x\n", hip_control->config_v4.q_num);
		seq_printf(m, "q_len                 = 0x%x\n", hip_control->config_v4.q_len);
		seq_printf(m, "q_idx_sz              = 0x%x\n", hip_control->config_v4.q_idx_sz);
		for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++)
			seq_printf(m, "q_loc[%d]            = 0x%x\n", i, hip_control->config_v4.q_loc[i]);
	} else if (conf_hip4_ver == 5) {
		seq_printf(m, "hip4_version_5 addr = 0x%p\n", &hip_control->config_v5);
		seq_printf(m, "magic_number        = 0x%x\n", hip_control->config_v5.magic_number);
		seq_printf(m, "hip_config_ver      = 0x%x\n", hip_control->config_v5.hip_config_ver);
		seq_printf(m, "config_len          = 0x%x\n", hip_control->config_v5.config_len);
		seq_printf(m, "compat_flag         = 0x%x\n", hip_control->config_v5.compat_flag);
		seq_printf(m, "sap_mlme_ver        = 0x%x\n", hip_control->config_v5.sap_mlme_ver);
		seq_printf(m, "sap_ma_ver          = 0x%x\n", hip_control->config_v5.sap_ma_ver);
		seq_printf(m, "sap_debug_ver       = 0x%x\n", hip_control->config_v5.sap_debug_ver);
		seq_printf(m, "sap_test_ver        = 0x%x\n", hip_control->config_v5.sap_test_ver);
		seq_printf(m, "fw_build_id         = 0x%x\n", hip_control->config_v5.fw_build_id);
		seq_printf(m, "fw_patch_id         = 0x%x\n", hip_control->config_v5.fw_patch_id);
		seq_printf(m, "unidat_req_headroom = 0x%x\n", hip_control->config_v5.unidat_req_headroom);
		seq_printf(m, "unidat_req_tailroom = 0x%x\n", hip_control->config_v5.unidat_req_tailroom);
		seq_printf(m, "bulk_buffer_align   = 0x%x\n", hip_control->config_v5.bulk_buffer_align);
		seq_printf(m, "host_cache_line     = 0x%x\n", hip_control->config_v5.host_cache_line);
		seq_printf(m, "host_buf_loc        = 0x%x\n", hip_control->config_v5.host_buf_loc);
		seq_printf(m, "host_buf_sz         = 0x%x\n", hip_control->config_v5.host_buf_sz);
		seq_printf(m, "fw_buf_loc          = 0x%x\n", hip_control->config_v5.fw_buf_loc);
		seq_printf(m, "fw_buf_sz           = 0x%x\n", hip_control->config_v5.fw_buf_sz);
		seq_printf(m, "mib_buf_loc         = 0x%x\n", hip_control->config_v5.mib_loc);
		seq_printf(m, "mib_buf_sz          = 0x%x\n", hip_control->config_v5.mib_sz);
		seq_printf(m, "log_config_loc      = 0x%x\n", hip_control->config_v5.log_config_loc);
		seq_printf(m, "log_config_sz       = 0x%x\n", hip_control->config_v5.log_config_sz);
		seq_printf(m, "mif_fh_int_n        = 0x%x\n", hip_control->config_v5.mif_fh_int_n);
		seq_printf(m, "mif_th_int_n        = 0x%x\n", hip_control->config_v5.mif_th_int_n);
		seq_printf(m, "scbrd_loc           = 0x%x\n", hip_control->config_v5.scbrd_loc);
		seq_printf(m, "q_num               = 0x%x\n", hip_control->config_v5.q_num);
		seq_printf(m, "q_len               = 0x%x\n", hip_control->config_v5.q_len);
		seq_printf(m, "q_idx_sz            = 0x%x\n", hip_control->config_v5.q_idx_sz);
		for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++)
			seq_printf(m, "q_loc[%d]            = 0x%x\n", i, hip_control->config_v5.q_loc[i]);
	}
	seq_puts(m, "\n-----------------------------------------\n");
	seq_puts(m, "HIP4 SCOREBOARD INDEXES:\n");
	seq_puts(m, "-----------------------------------------\n");
	seq_printf(m, "ktime start %lld (ns)\n", ktime_to_ns(hip->hip_priv->stats.start));
	seq_printf(m, "ktime now   %lld (ns)\n\n", ktime_to_ns(ktime_get()));

	seq_printf(m, "rx_intr_tohost 0x%x\n", hip->hip_priv->intr_tohost);
	seq_printf(m, "rx_intr_fromhost 0x%x\n\n", hip->hip_priv->intr_fromhost);

	/* HIP statistics */
	seq_printf(m, "HIP IRQs: %u\n", atomic_read(&hip->hip_priv->stats.irqs));
	seq_printf(m, "HIP IRQs spurious: %u\n", atomic_read(&hip->hip_priv->stats.spurious_irqs));
	seq_printf(m, "FW debug-inds: %u\n\n", atomic_read(&sdev->debug_inds));

	seq_puts(m, "Queue\tIndex\tFrames\n");
	seq_puts(m, "-----\t-----\t------\n");
	/* Print scoreboard */
	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++) {
		seq_printf(m, "Q%dW\t0x%x\t\n", i, hip4_read_index(hip, i, widx));
		seq_printf(m, "Q%dR\t0x%x\t%d\n", i, hip4_read_index(hip, i, ridx), hip->hip_priv->stats.q_num_frames[i]);
	}
	seq_puts(m, "\n");
	return 0;
}

static int hip4_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hip4_proc_show, PDE_DATA(inode));
}

static const struct file_operations hip4_procfs_stats_fops = {
	.owner   = THIS_MODULE,
	.open    = hip4_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int hip4_proc_jitter_show(struct seq_file *m, void *v)
{
	seq_puts(m, "Values in ns\n");
	seq_printf(m, "<%d(ns)\t\t\t\t %lld\n", HISTO_1, histogram_1);
	seq_printf(m, "%d(ns)-%d(ns)\t\t\t %lld\n", HISTO_1, HISTO_2, histogram_2);
	seq_printf(m, "%d(ns)-%d(ns)\t\t\t %lld\n", HISTO_2, HISTO_3, histogram_3);
	seq_printf(m, "%d(ns)-%d(ns)\t\t\t %lld\n", HISTO_3, HISTO_4, histogram_4);
	seq_printf(m, "%d(ns)-%d(ns)\t\t %lld\n", HISTO_4, HISTO_5, histogram_5);
	seq_printf(m, ">%d(ns)\t\t\t\t %lld\n", HISTO_5, histogram_6);
	seq_printf(m, "max jitter(ns)\t\t\t\t %lld\n", max_jitter);
	seq_puts(m, "--------------------------\n");
	seq_puts(m, "Packets in TH DATA Q\n");
	seq_printf(m, "<%d\t\t%lld\n", HISTO_1_DATA, histogram_1_data);
	seq_printf(m, "%d-%d\t\t%lld\n", HISTO_1_DATA, HISTO_2_DATA, histogram_2_data);
	seq_printf(m, "%d-%d\t\t%lld\n", HISTO_2_DATA, HISTO_3_DATA, histogram_3_data);
	seq_printf(m, "%d-%d\t\t%lld\n", HISTO_3_DATA, HISTO_4_DATA, histogram_4_data);
	seq_printf(m, "%d-%d\t\t%lld\n", HISTO_4_DATA, HISTO_5_DATA, histogram_5_data);
	seq_printf(m, ">%d\t\t%lld\n", HISTO_5_DATA, histogram_6_data);
	seq_printf(m, "max data\t%lld\n", max_data);
	return 0;
}

static int hip4_proc_jitter_open(struct inode *inode, struct file *file)
{
	return single_open(file, hip4_proc_jitter_show, PDE_DATA(inode));
}

static ssize_t hip4_proc_jitter_clear(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	SLSI_INFO_NODEV("Clear Histogram\n");

	histogram_1 = 0;
	histogram_2 = 0;
	histogram_3 = 0;
	histogram_4 = 0;
	histogram_5 = 0;
	histogram_6 = 0;
	max_jitter = 0;

	histogram_1_data = 0;
	histogram_2_data = 0;
	histogram_3_data = 0;
	histogram_4_data = 0;
	histogram_5_data = 0;
	histogram_6_data = 0;
	max_data = 0;

	return count;
}

static const struct file_operations hip4_procfs_jitter_fops = {
	.owner   = THIS_MODULE,
	.open    = hip4_proc_jitter_open,
	.write   = hip4_proc_jitter_clear,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};
#endif

#define FB_NO_SPC_NUM_RET    100
#define FB_NO_SPC_SLEEP_MS   10
#define FB_NO_SPC_DELAY_US   1000

/* Update scoreboard index */
/* Function can be called from BH context */
static void hip4_update_index(struct slsi_hip *hip, u32 q, enum rw r_w, u8 value)
{
	struct hip_priv    *hip_priv = hip->hip_priv;

	write_lock_bh(&hip_priv->rw_scoreboard);
	if (hip->hip_priv->version == 5 || hip->hip_priv->version == 4) {
		*((u8 *)(hip->hip_priv->scbrd_base + q_idx_layout[q][r_w])) = value;
	} else {
		SLSI_ERR_NODEV("Incorrect version\n");
		goto error;
	}

	/* Memory barrier when updating shared mailbox/memory */
	smp_wmb();
	SCSC_HIP4_SAMPLER_Q(hip_priv->minor, q, r_w, value, 0);
error:
	write_unlock_bh(&hip_priv->rw_scoreboard);
}

/* Read scoreboard index */
/* Function can be called from BH context */
static u8 hip4_read_index(struct slsi_hip *hip, u32 q, enum rw r_w)
{
	struct hip_priv    *hip_priv = hip->hip_priv;
	u32                 value = 0;

	read_lock_bh(&hip_priv->rw_scoreboard);
	if (hip->hip_priv->version == 5 || hip->hip_priv->version == 4) {
		value = *((u8 *)(hip->hip_priv->scbrd_base + q_idx_layout[q][r_w]));
	} else {
		SLSI_ERR_NODEV("Incorrect version\n");
		goto error;
	}

	/* Memory barrier when reading shared mailbox/memory */
	smp_rmb();
error:
	read_unlock_bh(&hip_priv->rw_scoreboard);
	return value;
}

void slsi_hip_from_host_intr_set(struct scsc_service *service, struct slsi_hip *hip)
{
	scsc_service_mifintrbit_bit_set(service, hip->hip_priv->intr_fromhost, SCSC_MIFINTR_TARGET_WLAN);
}

static void hip4_dump_dbg(struct slsi_hip *hip, struct mbulk *m, struct sk_buff *skb, struct scsc_service *service)
{
	unsigned int        i = 0;
	scsc_mifram_ref     ref;

	SLSI_ERR_NODEV("intr_tohost_fb 0x%x\n", hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_FH_RFB]);
	SLSI_ERR_NODEV("intr_tohost_ctrl 0x%x\n", hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_CTRL]);
	SLSI_ERR_NODEV("intr_tohost_dat 0x%x\n", hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
	SLSI_ERR_NODEV("intr_fromhost 0x%x\n", hip->hip_priv->intr_fromhost);

	/* Print scoreboard */
	for (i = 0; i < 6; i++) {
		SLSI_ERR_NODEV("Q%dW 0x%x\n", i, hip4_read_index(hip, i, widx));
		SLSI_ERR_NODEV("Q%dR 0x%x\n", i, hip4_read_index(hip, i, ridx));
	}

	if (service)
		scsc_mx_service_mif_dump_registers(service);

	if (m && service) {
		if (scsc_mx_service_mif_ptr_to_addr(service, m, &ref))
			return;
		SLSI_ERR_NODEV("m: %p 0x%x\n", m, ref);
		print_hex_dump(KERN_ERR, SCSC_PREFIX "mbulk ", DUMP_PREFIX_NONE, 16, 1, m, sizeof(struct mbulk), 0);
	}
	if (m && mbulk_has_signal(m))
		print_hex_dump(KERN_ERR, SCSC_PREFIX "sig   ", DUMP_PREFIX_NONE, 16, 1, mbulk_get_signal(m),
			       MBULK_SEG_SIG_BUFSIZE(m), 0);
	if (skb)
		print_hex_dump(KERN_ERR, SCSC_PREFIX "skb   ", DUMP_PREFIX_NONE, 16, 1, skb->data, skb->len > 0xff ? 0xff : skb->len, 0);

	SLSI_ERR_NODEV("time: wdt     %lld\n", ktime_to_ns(wdt));
	SLSI_ERR_NODEV("time: send    %lld\n", ktime_to_ns(send));
	SLSI_ERR_NODEV("time: intr_fb      %lld\n", ktime_to_ns(intr_received_fb));
	SLSI_ERR_NODEV("time: bh_init_fb   %lld\n", ktime_to_ns(bh_init_fb));
	SLSI_ERR_NODEV("time: bh_end_fb    %lld\n", ktime_to_ns(bh_end_fb));
	SLSI_ERR_NODEV("time: intr_ctrl    %lld\n", ktime_to_ns(intr_received_ctrl));
	SLSI_ERR_NODEV("time: bh_init_ctrl %lld\n", ktime_to_ns(bh_init_ctrl));
	SLSI_ERR_NODEV("time: bh_end_ctrl  %lld\n", ktime_to_ns(bh_end_ctrl));
	SLSI_ERR_NODEV("time: intr_data    %lld\n", ktime_to_ns(intr_received_data));
	SLSI_ERR_NODEV("time: bh_init_data %lld\n", ktime_to_ns(bh_init_data));
	SLSI_ERR_NODEV("time: bh_end_data  %lld\n", ktime_to_ns(bh_end_data));
	SLSI_ERR_NODEV("time: closing %lld\n", ktime_to_ns(closing));
#ifdef CONFIG_SCSC_WLAN_DEBUG
	/* Discard noise if it is a mbulk/skb issue */
	if (!skb && !m)
		hip4_history_record_print(false, NULL);
#endif
}

/* Transform skb to mbulk (fapi_signal + payload) */
static struct mbulk *hip4_skb_to_mbulk(struct scsc_service *service, struct hip_priv *hip,
				       struct sk_buff *skb, bool ctrl_packet, mbulk_colour colour)
{
	struct mbulk        *m = NULL, *head = NULL;
	void                *sig = NULL, *b_data = NULL;
	size_t              payload = 0, memcpy_len = 0;
	u8                  pool_id = ctrl_packet ? MBULK_POOL_ID_CTRL : MBULK_POOL_ID_DATA;
	u8                  headroom = 0, tailroom = 0;
	enum mbulk_class    clas = ctrl_packet ? MBULK_CLASS_FROM_HOST_CTL : MBULK_CLASS_FROM_HOST_DAT;
	struct slsi_skb_cb *cb = slsi_skb_cb_get(skb);
	int skb_offset = 0;
	scsc_mifram_ref m_chain_next = 0;
#ifdef CONFIG_SCSC_WLAN_SG
	u32                 linear_data;
	u32                 offset;
	u8                  i;
#endif

	payload = skb->len - cb->sig_length;

	/* Get headroom/tailroom */
	headroom = hip->unidat_req_headroom;
	tailroom = hip->unidat_req_tailroom;

	/* Allocate mbulk */
	if (payload > 0) {
		/* If signal include payload, add headroom and tailroom */
		head = mbulk_with_signal_alloc_by_pool(service, pool_id, colour, clas, cb->sig_length + 4,
						    payload + headroom + tailroom);
		if (!head) {
			SLSI_ERR_NODEV("payload is:%d, mbulk alloc failed\n", payload);
			return NULL;
		}
		if (!mbulk_reserve_head(head, headroom)) {
			mbulk_chain_free(head, service);
			return NULL;
		}
	} else {
		/* If it is only a signal do not add headroom */
		head = mbulk_with_signal_alloc_by_pool(service, pool_id, colour, clas, cb->sig_length + 4, 0);
		if (!head)
			return NULL;
	}

	m = head;

	/* Get signal handler if it is head_mbulk or a single mbulk */
	sig = mbulk_get_signal(m);
	if (!sig) {
		SLSI_ERR_NODEV("no sig in mbulk/chained_head\n");
		mbulk_chain_free(head, service);
		return NULL;
	}

	/* Copy signal */
	/* 4Bytes offset is required for FW fapi header */
	memcpy(sig + 4, skb->data, cb->sig_length);
	skb_offset += cb->sig_length;
	while (m) {
		/* Copy payload */
		/* If the signal has payload memcpy the data */
		if (m->dat_bufsz > 0) {
			/* Get head pointer */
			b_data = mbulk_dat_rw(m);
			if (!b_data) {
				SLSI_ERR_NODEV("head pointer is NULL\n");
				mbulk_chain_free(head, service);
				return NULL;
			}
			if (!payload) {
				m->flag |= MBULK_F_OBOUND;
				SLSI_WARN_NODEV("m->dat_bufsz is %d but payload is 0\n", m->dat_bufsz);
				break;
			}

#ifdef CONFIG_SCSC_WLAN_SG
			/* The amount of non-paged data at skb->data can be calculated as skb->len - skb->data_len.
			 * Helper routine: skb_headlen() .
			 */
			linear_data = skb_headlen(skb) - cb->sig_length;

			offset = 0;
			/* Copy the linear data */
			if (linear_data > 0) {
				/* Copy the linear payload skipping the signal data */
				memcpy(b_data, skb->data + cb->sig_length, linear_data);
				offset = linear_data;
			}

			/* Traverse fragments and copy in to linear DRAM memory */
			for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
				skb_frag_t *frag = NULL;
				void *frag_va_data;
				unsigned int frag_size;

				frag = &skb_shinfo(skb)->frags[i];
				WLBT_WARN_ON(!frag);
				if (!frag)
					continue;
				frag_va_data = skb_frag_address_safe(frag);
				WLBT_WARN_ON(!frag_va_data);
				if (!frag_va_data)
					continue;
				frag_size = skb_frag_size(frag);
				/* Copy the fragmented data */
				memcpy(b_data + offset, frag_va_data, frag_size);
				offset += frag_size;
			}

			/* Check whether the driver should perform the checksum */
			if (skb->ip_summed == CHECKSUM_PARTIAL) {
				SLSI_DBG3_NODEV(SLSI_HIP, "CHECKSUM_PARTIAL. Driver performing checksum\n");
				if (skb->protocol == htons(ETH_P_IP)) {
					struct ethhdr *mach = (struct ethhdr *)b_data;
					struct iphdr *iph = (struct iphdr *)((char *)b_data + sizeof(*mach));
					unsigned int len = payload - sizeof(*mach) - (iph->ihl << 2);

					if (iph->protocol == IPPROTO_TCP) {
						struct tcphdr *th = (struct tcphdr *)((char *)b_data + sizeof(*mach) +
								    (iph->ihl << 2));
						th->check = 0;
						th->check = csum_tcpudp_magic(iph->saddr, iph->daddr, len,
									      IPPROTO_TCP,
							csum_partial((char *)th, len, 0));
						SLSI_DBG3_NODEV(SLSI_HIP, "th->check 0x%x\n", ntohs(th->check));
					} else if (iph->protocol == IPPROTO_UDP) {
						struct udphdr *uh = (struct udphdr *)((char *)b_data + sizeof(*mach) +
								    (iph->ihl << 2));
						uh->check = 0;
						uh->check = csum_tcpudp_magic(iph->saddr, iph->daddr, len,
									      IPPROTO_UDP,
							csum_partial((char *)uh, len, 0));
						SLSI_DBG3_NODEV(SLSI_HIP, "uh->check 0x%x\n", ntohs(uh->check));
					}
				}
			}
#else
			/* Copy payload skipping the signal data
			   If it is a chained mbulk, then consider for the headroom when copying the payload
			   for head mbulk as the m->dat_bufsz will be 2028 bytes (pool-segsiz - m->sigsz).
			*/
			if ((m->flag & MBULK_F_CHAIN_HEAD) == MBULK_F_CHAIN_HEAD) {
				memcpy_len = umin(m->dat_bufsz - headroom, payload);
			} else {
				memcpy_len = umin(m->dat_bufsz, payload);
			}
			memcpy(b_data, skb->data + skb_offset, memcpy_len);
			if (!mbulk_append_tail(m, memcpy_len))
				SLSI_WARN_NODEV("mbulk_append_tail failed\n");
			payload -= memcpy_len;
			skb_offset += memcpy_len;
#endif
		}
		m->flag |= MBULK_F_OBOUND;
		m_chain_next = mbulk_chain_next(m);
		if (!m_chain_next)
			break;
		m = scsc_mx_service_mif_addr_to_ptr(service, m_chain_next);

#ifdef CONFIG_SCSC_SMAPPER
		/* Clear smapper field */
		cb->skb_addr = NULL;
#endif
	}
	return head;
}

/* Transform mbulk to skb (fapi_signal + payload) */
static struct sk_buff *hip4_mbulk_to_skb(struct scsc_service *service, struct hip_priv *hip_priv, struct mbulk *m, scsc_mifram_ref *to_free, bool atomic)
{
	struct slsi_skb_cb        *cb;
	struct mbulk              *next_mbulk[MBULK_MAX_CHAIN];
	struct sk_buff            *skb = NULL;
	scsc_mifram_ref           ref;
	scsc_mifram_ref           m_chain_next;
	u8                        free = 0;
	u8                        i = 0, j = 0;
	u8                        *p;
	size_t                    bytes_to_alloc = 0;

	/* Get the mif ref pointer, check for incorrect mbulk */
	if (scsc_mx_service_mif_ptr_to_addr(service, m, &ref)) {
		SLSI_ERR_NODEV("mbulk address conversion failed\n");
		return NULL;
	}

	/* Track mbulk that should be freed */
	to_free[free++] = ref;

	bytes_to_alloc += m->sig_bufsz - 4;
	bytes_to_alloc += m->len;

	/* Detect Chained mbulk to start building the chain */
	if ((MBULK_SEG_IS_CHAIN_HEAD(m)) && (MBULK_SEG_IS_CHAINED(m))) {
		m_chain_next = mbulk_chain_next(m);
		if (!m_chain_next) {
			SLSI_ERR_NODEV("Mbulk is set MBULK_F_CHAIN_HEAD and MBULK_F_CHAIN but m_chain_next is NULL\n");
			goto cont;
		}
		while (1) {
			/* increase number mbulks in chain */
			i++;
			/* Get next_mbulk kernel address space pointer  */
			next_mbulk[i - 1] = scsc_mx_service_mif_addr_to_ptr(service, m_chain_next);
			if (!next_mbulk[i - 1]) {
				SLSI_ERR_NODEV("First Mbulk is set as MBULK_F_CHAIN but next_mbulk is NULL\n");
				return NULL;
			}
			/* Track mbulk to be freed */
			to_free[free++] = m_chain_next;
			bytes_to_alloc += next_mbulk[i - 1]->len;
			if (MBULK_SEG_IS_CHAINED(next_mbulk[i - 1])) {
				/* continue traversing the chain */
				m_chain_next = mbulk_chain_next(next_mbulk[i - 1]);
				if (!m_chain_next)
					break;

				if (i >= MBULK_MAX_CHAIN) {
					SLSI_ERR_NODEV("Max number of chained MBULK reached\n");
					return NULL;
				}
			} else {
				break;
			}
		}
	}

cont:
	if (atomic) {
		skb = alloc_skb(bytes_to_alloc, GFP_ATOMIC);
	} else {
		spin_unlock_bh(&hip_priv->rx_lock);
		skb = alloc_skb(bytes_to_alloc, GFP_KERNEL);
		spin_lock_bh(&hip_priv->rx_lock);
	}
	if (!skb) {
		SLSI_ERR_NODEV("Error allocating skb %d bytes\n", bytes_to_alloc);
		return NULL;
	}

	cb = slsi_skb_cb_init(skb);
	cb->sig_length = m->sig_bufsz - 4;
	/* fapi_data_append adds to the data_length */
	cb->data_length = cb->sig_length;

	p = mbulk_get_signal(m);
	if (!p) {
		SLSI_ERR_NODEV("No signal in Mbulk\n");
		print_hex_dump(KERN_ERR, SCSC_PREFIX "mbulk ", DUMP_PREFIX_NONE, 16, 1, m, sizeof(struct mbulk), 0);
#ifdef CONFIG_SCSC_SMAPPER
		hip4_smapper_free_mapped_skb(skb);
#endif
		kfree_skb(skb);
		return NULL;
	}
	/* Remove 4Bytes offset coming from FW */
	p += 4;

	/* Don't need to copy the 4Bytes header coming from the FW */
	memcpy(skb_put(skb, cb->sig_length), p, cb->sig_length);

	if (m->len)
		fapi_append_data(skb, mbulk_dat_r(m), m->len);
	for (j = 0; j < i; j++)
		fapi_append_data(skb, mbulk_dat_r(next_mbulk[j]), next_mbulk[j]->len);

	return skb;
}

/* Add signal reference (offset in shared memory) in the selected queue */
/* This function should be called in atomic context. Callers should supply proper locking mechanism */
static int hip4_q_add_signal(struct slsi_hip *hip, enum hip4_hip_q_conf conf, scsc_mifram_ref phy_m, struct scsc_service *service)
{
	struct hip4_hip_control *ctrl = hip->hip_control;
	struct hip_priv        *hip_priv = hip->hip_priv;
	u8                      idx_w;
	u8                      idx_r;

	/* Read the current q write pointer */
	idx_w = hip4_read_index(hip, conf, widx);
	/* Read the current q read pointer */
	idx_r = hip4_read_index(hip, conf, ridx);
	SCSC_HIP4_SAMPLER_Q(hip_priv->minor, conf, widx, idx_w, 1);
	SCSC_HIP4_SAMPLER_Q(hip_priv->minor, conf, ridx, idx_r, 1);

	/* Queueu is full */
	if (idx_r == ((idx_w + 1) & (MAX_NUM - 1)))
		return -ENOSPC;

	/* Update array */
	ctrl->q[conf].array[idx_w] = phy_m;
	/* Memory barrier before updating shared mailbox */
	smp_wmb();
	SCSC_HIP4_SAMPLER_QREF(hip_priv->minor, phy_m, conf);
#ifdef CONFIG_SCSC_WLAN_DEBUG
	hip->hip_priv->stats.q_num_frames[conf] = hip->hip_priv->stats.q_num_frames[conf] + 1;
#endif

	/* Increase index */
	idx_w++;
	idx_w &= (MAX_NUM - 1);

	/* Update the scoreboard */
	hip4_update_index(hip, conf, widx, idx_w);

#ifdef CONFIG_SCSC_WLAN_TX_API
	if (conf == HIP4_MIF_Q_FH_DAT)
		return 0;
#endif
	send = ktime_get();
	scsc_service_mifintrbit_bit_set(service, hip_priv->intr_fromhost, SCSC_MIFINTR_TARGET_WLAN);

	return 0;
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void hip4_watchdog(struct timer_list *t)
#else
static void hip4_watchdog(unsigned long data)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct hip_priv        *priv = from_timer(priv, t, watchdog);
	struct slsi_hip        *hip = priv->hip;
#else
	struct slsi_hip        *hip = (struct slsi_hip *)data;
#endif
	struct slsi_dev         *sdev = container_of(hip, struct slsi_dev, hip);
	struct scsc_service     *service;
	ktime_t                 intr_ov;
	unsigned long           flags;
	bool retrigger_watchdog = true;
	u8 i;

	if (!hip || !sdev || !sdev->service || !hip->hip_priv)
		return;

	spin_lock_irqsave(&hip->hip_priv->watchdog_lock, flags);
	if (!atomic_read(&hip->hip_priv->watchdog_timer_active))
		goto exit;

	wdt = ktime_get();

	/* if intr_received > wdt skip as intr has been unblocked */
	if (test_and_clear_bit(HIP4_MIF_Q_FH_RFB, hip->hip_priv->irq_bitmap)) {
		intr_ov = ktime_add_ms(intr_received_fb, jiffies_to_msecs(HZ));
		if ((ktime_compare(intr_ov, wdt) < 0))
			retrigger_watchdog = false;
	}
	if (test_and_clear_bit(HIP4_MIF_Q_TH_CTRL, hip->hip_priv->irq_bitmap)) {
		intr_ov = ktime_add_ms(intr_received_ctrl, jiffies_to_msecs(HZ));
		if ((ktime_compare(intr_ov, wdt) < 0))
			retrigger_watchdog = false;
	}
	if (test_and_clear_bit(HIP4_MIF_Q_TH_DAT, hip->hip_priv->irq_bitmap)) {
		intr_ov = ktime_add_ms(intr_received_data, jiffies_to_msecs(HZ));
		if ((ktime_compare(intr_ov, wdt) < 0))
			retrigger_watchdog = false;
	}
	if (retrigger_watchdog) {
		wdt = ktime_set(0, 0);
		/* Retrigger WDT to check flags again in the future */
		mod_timer(&hip->hip_priv->watchdog, jiffies + HZ / 2);
		goto exit;
	}

	/* Unlock irq to avoid __local_bh_enable_ip warning */
	spin_unlock_irqrestore(&hip->hip_priv->watchdog_lock, flags);
	hip4_dump_dbg(hip, NULL, NULL, sdev->service);
	spin_lock_irqsave(&hip->hip_priv->watchdog_lock, flags);

	service = sdev->service;

	SLSI_INFO_NODEV("Hip4 watchdog triggered\n");

	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++) {
		if (hip->hip_priv->intr_tohost_mul[i] == MIF_NO_IRQ)
			continue;
		if (scsc_service_mifintrbit_bit_mask_status_get(service) & (1 << hip->hip_priv->intr_tohost_mul[i])) {
			/* Interrupt might be pending! */
			SLSI_INFO_NODEV("%d: Interrupt Masked. Unmask to restart Interrupt processing\n", i);
			scsc_service_mifintrbit_bit_unmask(service, hip->hip_priv->intr_tohost_mul[i]);
		}
	}
exit:
	spin_unlock_irqrestore(&hip->hip_priv->watchdog_lock, flags);
}

int slsi_hip_wlan_get_rtc_time(struct rtc_time *tm)
{
	struct rtc_device *rtc = NULL;
	int err;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (!rtc) {
		SLSI_ERR_NODEV("Can not find RTC Device\n");
		return -ENXIO;
	}
	err = rtc_read_time(rtc, tm);

	rtc_class_close(rtc);

	if (err < 0)
		return err;

	return 0;
}

#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
void slsi_hip_napi_cpu_set(struct slsi_hip *hip, u8 napi_cpu, bool perf_mode)
{
	struct hip_priv        *hip_priv;
	struct slsi_dev         *sdev = container_of(hip, struct slsi_dev, hip);
	unsigned long flags;

	if (!hip)
		return;

	hip_priv = hip->hip_priv;

	if (!hip_priv)
		return;

	if (!cpu_online(napi_cpu)) {
		SLSI_ERR_NODEV("CPU%d is offline.\n", napi_cpu);
		scsc_service_mifintrbit_bit_unmask(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
		return;
	}

	slsi_wake_lock(&hip->hip_priv->hip4_wake_lock_data);
	if (napi_cpu != napi_select_cpu) {
		spin_lock_irqsave(&hip->hip_priv->napi_cpu_lock, flags);
		scsc_service_mifintrbit_bit_mask(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
		if (!test_bit(SLSI_HIP_NAPI_STATE_ENABLED, &hip->hip_priv->napi_state)) {
			slsi_wake_unlock(&hip->hip_priv->hip4_wake_lock_data);
			SLSI_INFO_NODEV("NAPI is already disabled on CPU%d\n", napi_select_cpu);
			spin_unlock_irqrestore(&hip->hip_priv->napi_cpu_lock, flags);
			return;
		}

		SLSI_INFO_NODEV("disable NAPI on CPU%d\n", napi_select_cpu);
		clear_bit(SLSI_HIP_NAPI_STATE_ENABLED, &hip->hip_priv->napi_state);
		napi_select_cpu = napi_cpu;

		/* napi_disable may sleep, so release the lock */
		spin_unlock_irqrestore(&hip->hip_priv->napi_cpu_lock, flags);
		napi_disable(&hip->hip_priv->napi);

#if defined(CONFIG_SCSC_QOS)
#if defined(CONFIG_SOC_S5E9815) || defined(CONFIG_SOC_S5E9925)
		/**
		 * In case where irq affinity set is failed,
		 * we allow that IRQ and napi are scheduled in different core.
		 */
#if defined(CONFIG_SCSC_PCIE_CHIP)
		/* MSI affinity is not supported (yet), msi
		* argument will be ignored */
		if (scsc_service_set_affinity_cpu(sdev->service, 0, napi_select_cpu) != 0)
#else
		if (scsc_service_set_affinity_cpu(sdev->service, napi_select_cpu) != 0)
#endif
			SLSI_ERR_NODEV("failed to change IRQ affinity (CPU%d)\n", napi_select_cpu);
#endif
#endif

		local_bh_disable();
		spin_lock_irqsave(&hip->hip_priv->napi_cpu_lock, flags);

		hip_priv->napi_perf_mode = perf_mode;
		if (!test_and_set_bit(SLSI_HIP_NAPI_STATE_ENABLED, &hip->hip_priv->napi_state)) {
			SLSI_INFO_NODEV("enable NAPI on CPU%d\n", napi_select_cpu);
			napi_enable(&hip->hip_priv->napi);

			if (napi_select_cpu == smp_processor_id()) {
				napi_schedule(&hip->hip_priv->napi);
			} else {
				if (napi_select_cpu && cpu_online(napi_select_cpu))
					schedule_work_on(napi_select_cpu, &hip->hip_priv->intr_wq_napi_cpu_switch);
				else
					napi_schedule(&hip->hip_priv->napi);
			}
		}

		spin_unlock_irqrestore(&hip->hip_priv->napi_cpu_lock, flags);
		local_bh_enable();
	}
	scsc_service_mifintrbit_bit_unmask(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
	slsi_wake_unlock(&hip->hip_priv->hip4_wake_lock_data);
}
#endif
static void hip4_tl_fb(unsigned long data)

{
	struct slsi_hip        *hip = (void *)data;
	struct hip_priv        *hip_priv = hip->hip_priv;
	struct hip4_hip_control *ctrl;
	struct scsc_service     *service;
	struct slsi_dev         *sdev;
	bool                    no_change = true;
	u8                      idx_r;
	u8                      idx_w;
	scsc_mifram_ref         ref;
	void                    *mem;

	if (!hip_priv || !hip_priv->hip) {
		SLSI_ERR_NODEV("hip_priv or hip_priv->hip is Null\n");
		return;
	}

	hip = hip_priv->hip;
	ctrl = hip->hip_control;

	if (!ctrl) {
		SLSI_ERR_NODEV("hip->hip_control is Null\n");
		return;
	}
	sdev = container_of(hip, struct slsi_dev, hip);

	if (!sdev || !sdev->service) {
		SLSI_ERR_NODEV("sdev or sdev->service is Null\n");
		return;
	}

	spin_lock_bh(&hip_priv->rx_lock);
	service = sdev->service;
	SCSC_HIP4_SAMPLER_INT_BH(hip->hip_priv->minor, 2);
	bh_init_fb = ktime_get();
	clear_bit(HIP4_MIF_Q_FH_RFB, hip->hip_priv->irq_bitmap);

	idx_r = hip4_read_index(hip, HIP4_MIF_Q_FH_RFB, ridx);
	idx_w = hip4_read_index(hip, HIP4_MIF_Q_FH_RFB, widx);

#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
	if (idx_r != idx_w) {
		SCSC_HIP4_SAMPLER_Q(hip_priv->minor, HIP4_MIF_Q_FH_RFB, ridx, idx_r, 1);
		SCSC_HIP4_SAMPLER_Q(hip_priv->minor, HIP4_MIF_Q_FH_RFB, widx, idx_w, 1);
	}
#endif
	while (idx_r != idx_w) {
		struct mbulk *m;
		mbulk_colour colour;

		no_change = false;
		ref = ctrl->q[HIP4_MIF_Q_FH_RFB].array[idx_r];
#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
		SCSC_HIP4_SAMPLER_QREF(hip_priv->minor, ref, HIP4_MIF_Q_FH_RFB);
#endif
		mem = scsc_mx_service_mif_addr_to_ptr(service, ref);
		m = (struct mbulk *)mem;

		if (!m) {
			SLSI_ERR_NODEV("FB: Mbulk is NULL\n");
			goto consume_fb_mbulk;
		}
		/* Account ONLY for data RFB */
		if ((m->pid & 0x1) == MBULK_POOL_ID_DATA) {
			colour = mbulk_get_colour(MBULK_POOL_ID_DATA, m);
#if defined(CONFIG_SCSC_WLAN_HIP4_PROFILING) && !defined(CONFIG_SCSC_WLAN_TX_API)
			SCSC_HIP4_SAMPLER_VIF_PEER(hip->hip_priv->minor,
						   0,
						   SLSI_MBULK_COLOUR_GET_IFNUM(colour),
						   SLSI_MBULK_COLOUR_GET_PEER_IDX(colour));
			/* to profile round-trip */
			{
				u16 host_tag;
				u8 *get_host_tag;
				/* This is a nasty way of getting the host_tag without involving mbulk processing
				 * This hostag value should also be include in the cb descriptor which goes to
				 * mbulk descriptor (no room left at the moment)
				 */
				get_host_tag = (u8 *)m;
				host_tag = get_host_tag[37] << 8 | get_host_tag[36];
				SCSC_HIP4_SAMPLER_PKT_TX_FB(hip->hip_priv->minor, host_tag);
			}
#endif
			/* Ignore return value */
#ifdef CONFIG_SCSC_WLAN_TX_API
			slsi_hip_tx_done(sdev, colour, (idx_r + 1 & (MAX_NUM - 1)) != idx_w);
#else
			slsi_hip_tx_done(sdev,
					 SLSI_MBULK_COLOUR_GET_IFNUM(colour),
					 SLSI_MBULK_COLOUR_GET_PEER_IDX(colour),
					 SLSI_MBULK_COLOUR_GET_AC(colour));
#endif
		}
		mbulk_free_virt_host(m);
consume_fb_mbulk:
		/* Increase index */
		idx_r++;
		idx_r &= (MAX_NUM - 1);
		hip4_update_index(hip, HIP4_MIF_Q_FH_RFB, ridx, idx_r);
	}

	if (no_change)
		atomic_inc(&hip->hip_priv->stats.spurious_irqs);

	if (!atomic_read(&hip->hip_priv->closing)) {
		atomic_set(&hip->hip_priv->watchdog_timer_active, 0);
		scsc_service_mifintrbit_bit_unmask(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_FH_RFB]);
	}
	SCSC_HIP4_SAMPLER_INT_OUT_BH(hip->hip_priv->minor, 2);

	if (slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_tx)) {
		slsi_wake_unlock(&hip->hip_priv->hip4_wake_lock_tx);
#ifdef CONFIG_SCSC_WLAN_ANDROID
		SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_RELEASED, "hip4_wake_lock_tx", WL_REASON_RX);
#endif
	}

	bh_end_fb = ktime_get();
	spin_unlock_bh(&hip_priv->rx_lock);
}

static void hip4_irq_handler_fb(int irq, void *data)
{
	struct slsi_hip    *hip = (struct slsi_hip *)data;
	struct slsi_dev     *sdev = container_of(hip, struct slsi_dev, hip);

	SCSC_HIP4_SAMPLER_INT(hip->hip_priv->minor, 2);
	intr_received_fb = ktime_get();

	if (!slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_tx)) {
		slsi_wake_lock_timeout(&hip->hip_priv->hip4_wake_lock_tx, msecs_to_jiffies(SLSI_HIP_WAKELOCK_TIME_OUT_IN_MS));
#ifdef CONFIG_SCSC_WLAN_ANDROID
		SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_TAKEN, "hip4_wake_lock_tx", WL_REASON_RX);
#endif
	}

	if (!atomic_read(&hip->hip_priv->watchdog_timer_active)) {
		atomic_set(&hip->hip_priv->watchdog_timer_active, 1);
		mod_timer(&hip->hip_priv->watchdog, jiffies + HZ);
	}
	set_bit(HIP4_MIF_Q_FH_RFB, hip->hip_priv->irq_bitmap);

	scsc_service_mifintrbit_bit_mask(sdev->service, irq);
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	tasklet_hi_schedule(&hip->hip_priv->intr_tl_fb);
#else
	slsi_lbm_run_bh(hip->hip_priv->bh_rfb);
#endif
	/* Clear interrupt */
	scsc_service_mifintrbit_bit_clear(sdev->service, irq);
	SCSC_HIP4_SAMPLER_INT_OUT(hip->hip_priv->minor, 2);
}

#ifdef CONFIG_SLSI_WLAN_LPC
static void hip4_lpc_work(struct work_struct *data)
{
	struct hip_priv        *hip_priv = slsi_lbm_get_hip_priv_from_work(data);
	struct slsi_hip        *hip;
	struct slsi_dev        *sdev;
	int                     ret;

	if (!hip_priv || !hip_priv->hip) {
		SLSI_ERR_NODEV("hip_priv or hip_priv->hip is Null\n");
		return;
	}

	hip = hip_priv->hip;
	sdev = container_of(hip, struct slsi_dev, hip);

	if (!sdev || !sdev->service) {
		SLSI_ERR_NODEV("sdev or sdev->service is Null\n");
		return;
	}

	ret = slsi_lpc_get_packets_info(sdev);

	if (ret <= 0) {
		SLSI_INFO_NODEV("There are still packets to process.\n");
		slsi_lbm_run_bh(hip->hip_priv->bh_lpc);
	}
}
#endif
static void hip4_wq_ctrl(struct work_struct *data)
{
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	struct hip_priv        *hip_priv = container_of(data, struct hip_priv, intr_wq_ctrl);
#else
	struct hip_priv        *hip_priv = slsi_lbm_get_hip_priv_from_work(data);
#endif
	struct slsi_hip        *hip;
	struct hip4_hip_control *ctrl;
	struct scsc_service     *service;
	struct slsi_dev         *sdev;
	u8						retry;
	bool                    no_change = true;
	u8                      idx_r;
	u8                      idx_w;
	scsc_mifram_ref         ref;
	void                    *mem;
	struct mbulk            *m;
#if defined(CONFIG_SCSC_WLAN_DEBUG) || defined(CONFIG_SCSC_WLAN_HIP4_PROFILING)
	int                     id;
#endif

	if (!hip_priv || !hip_priv->hip) {
		SLSI_ERR_NODEV("hip_priv or hip_priv->hip is Null\n");
		return;
	}

	hip = hip_priv->hip;
	ctrl = hip->hip_control;

	if (!ctrl) {
		SLSI_ERR_NODEV("hip->hip_control is Null\n");
		return;
	}
	sdev = container_of(hip, struct slsi_dev, hip);

	if (!sdev || !sdev->service) {
		SLSI_ERR_NODEV("sdev or sdev->service is Null\n");
		return;
	}

	spin_lock_bh(&hip_priv->rx_lock);
	service = sdev->service;
	SCSC_HIP4_SAMPLER_INT_BH(hip->hip_priv->minor, 1);
	bh_init_ctrl = ktime_get();
	clear_bit(HIP4_MIF_Q_TH_CTRL, hip->hip_priv->irq_bitmap);

	idx_r = hip4_read_index(hip, HIP4_MIF_Q_TH_CTRL, ridx);
	idx_w = hip4_read_index(hip, HIP4_MIF_Q_TH_CTRL, widx);

#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
	if (idx_r != idx_w) {
		SCSC_HIP4_SAMPLER_Q(hip_priv->minor, HIP4_MIF_Q_TH_CTRL, ridx, idx_r, 1);
		SCSC_HIP4_SAMPLER_Q(hip_priv->minor, HIP4_MIF_Q_TH_CTRL, widx, idx_w, 1);
	}
#endif
	while (idx_r != idx_w) {
		struct sk_buff *skb;
		/* TODO: currently the max number to be freed is 2. In future
		 * implementations (i.e. AMPDU) this number may be bigger
		 * list of mbulks to be freedi
		 */
		scsc_mifram_ref to_free[MBULK_MAX_CHAIN + 1] = { 0 };
		u8              i = 0;

		no_change = false;
		/* Catch-up with idx_w */
		ref = ctrl->q[HIP4_MIF_Q_TH_CTRL].array[idx_r];
#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
		SCSC_HIP4_SAMPLER_QREF(hip_priv->minor, ref, HIP4_MIF_Q_TH_CTRL);
#endif
		mem = scsc_mx_service_mif_addr_to_ptr(service, ref);
		m = (struct mbulk *)(mem);
		/* Process Control Signal */
		skb = hip4_mbulk_to_skb(service, hip_priv, m, to_free, false);
		if (!skb) {
			SLSI_ERR_NODEV("Ctrl: Error parsing or allocating skb\n");
			hip4_dump_dbg(hip, m, skb, service);
			goto consume_ctl_mbulk;
		}

		if (m->flag & MBULK_F_WAKEUP) {
			SLSI_INFO(sdev, "WIFI wakeup by MLME frame 0x%x:\n", fapi_get_sigid(skb));
			SCSC_BIN_TAG_INFO(BIN_WIFI_PM, skb->data, skb->len > 128 ? 128 : skb->len);
			slsi_skb_cb_get(skb)->wakeup = true;
		}

#if defined(CONFIG_SCSC_WLAN_DEBUG) || defined(CONFIG_SCSC_WLAN_HIP4_PROFILING)
		id = fapi_get_sigid(skb);
#endif
#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
		/* log control signal, not unidata not debug  */
		if (fapi_is_mlme(skb))
			SCSC_HIP4_SAMPLER_SIGNAL_CTRLRX(hip_priv->minor, (id & 0xff00) >> 8, id & 0xff);
#endif
#ifdef CONFIG_SCSC_WLAN_DEBUG
		hip4_history_record_add(TH, id);
#endif
		if (slsi_hip_rx(sdev, skb) < 0) {
			SLSI_ERR_NODEV("Ctrl: Error detected slsi_hip_rx\n");
			hip4_dump_dbg(hip, m, skb, service);
#ifdef CONFIG_SCSC_SMAPPER
			hip4_smapper_free_mapped_skb(skb);
#endif
			kfree_skb(skb);
		}
consume_ctl_mbulk:
		/* Increase index */
		idx_r++;
		idx_r &= (MAX_NUM - 1);

		/* Go through the list of references to free */
		while ((ref = to_free[i++])) {
			/* Set the number of retries */
			retry = FB_NO_SPC_NUM_RET;
			/* return to the firmware */
			while (hip4_q_add_signal(hip, HIP4_MIF_Q_TH_RFB, ref, service) && (!atomic_read(&hip->hip_priv->closing)) && (retry > 0)) {
				SLSI_WARN_NODEV("Ctrl: Not enough space in FB, retry of %d/%d\n",
						((FB_NO_SPC_NUM_RET + 1) - retry), FB_NO_SPC_NUM_RET);
				spin_unlock_bh(&hip_priv->rx_lock);
				msleep(FB_NO_SPC_SLEEP_MS);
				spin_lock_bh(&hip_priv->rx_lock);
				retry--;
				if (retry == 0)
					SLSI_ERR_NODEV("Ctrl: FB has not been freed for %d ms\n", FB_NO_SPC_NUM_RET * FB_NO_SPC_SLEEP_MS);
				SCSC_HIP4_SAMPLER_QFULL(hip_priv->minor, HIP4_MIF_Q_TH_RFB);
			}
		}
		hip4_update_index(hip, HIP4_MIF_Q_TH_CTRL, ridx, idx_r);
	}

	if (no_change)
		atomic_inc(&hip->hip_priv->stats.spurious_irqs);

	if (!atomic_read(&hip->hip_priv->closing)) {
		atomic_set(&hip->hip_priv->watchdog_timer_active, 0);
		scsc_service_mifintrbit_bit_unmask(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_CTRL]);
	}
	SCSC_HIP4_SAMPLER_INT_OUT_BH(hip->hip_priv->minor, 1);

	if (slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_ctrl)) {
		slsi_wake_unlock(&hip->hip_priv->hip4_wake_lock_ctrl);
#ifdef CONFIG_SCSC_WLAN_ANDROID
		SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_RELEASED, "hip4_wake_lock_ctrl", WL_REASON_RX);
#endif
	}

	bh_end_ctrl = ktime_get();
	spin_unlock_bh(&hip_priv->rx_lock);
}

static void hip4_irq_handler_ctrl(int irq, void *data)
{
	struct slsi_hip    *hip = (struct slsi_hip *)data;
	struct slsi_dev     *sdev = container_of(hip, struct slsi_dev, hip);

	SCSC_HIP4_SAMPLER_INT(hip->hip_priv->minor, 1);
	intr_received_ctrl = ktime_get();

	if (!slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_ctrl)) {
		slsi_wake_lock_timeout(&hip->hip_priv->hip4_wake_lock_ctrl, msecs_to_jiffies(SLSI_HIP_WAKELOCK_TIME_OUT_IN_MS));
#ifdef CONFIG_SCSC_WLAN_ANDROID
		SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_TAKEN, "hip4_wake_lock_ctrl", WL_REASON_RX);
#endif
	}

	if (!atomic_read(&hip->hip_priv->watchdog_timer_active)) {
		atomic_set(&hip->hip_priv->watchdog_timer_active, 1);
		mod_timer(&hip->hip_priv->watchdog, jiffies + HZ);
	}
	set_bit(HIP4_MIF_Q_TH_CTRL, hip->hip_priv->irq_bitmap);

	scsc_service_mifintrbit_bit_mask(sdev->service, irq);
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	if (hip4_system_wq)
		schedule_work(&hip->hip_priv->intr_wq_ctrl);
	else
		queue_work(hip->hip_priv->hip4_workq, &hip->hip_priv->intr_wq_ctrl);
#else
	slsi_lbm_run_bh(hip->hip_priv->bh_ctl);
#endif
	/* Clear interrupt */
	scsc_service_mifintrbit_bit_clear(sdev->service, irq);
	SCSC_HIP4_SAMPLER_INT_OUT(hip->hip_priv->minor, 1);
}

void slsi_hip_sched_wq_ctrl(struct slsi_hip *hip)
{
	struct slsi_dev     *sdev = container_of(hip, struct slsi_dev, hip);

	if (!hip || !sdev || !sdev->service || !hip->hip_priv)
		return;

	if (atomic_read(&hip->hip_priv->closing))
		return;

	hip4_dump_dbg(hip, NULL, NULL, sdev->service);

	if (!slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_ctrl)) {
		slsi_wake_lock_timeout(&hip->hip_priv->hip4_wake_lock_ctrl, msecs_to_jiffies(SLSI_HIP_WAKELOCK_TIME_OUT_IN_MS));
#ifdef CONFIG_SCSC_WLAN_ANDROID
		SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_TAKEN, "hip4_wake_lock_ctrl", WL_REASON_RX);
#endif
	}

	SLSI_DBG1(sdev, SLSI_HIP, "Trigger wq for skipped ctrl BH\n");

#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	if (hip4_system_wq)
		schedule_work(&hip->hip_priv->intr_wq_ctrl);
	else
		if(!queue_work(hip->hip_priv->hip4_workq, &hip->hip_priv->intr_wq_ctrl))
			SLSI_DBG1(sdev, SLSI_HIP, "hip4_wq_ctrl is already scheduled\n");
#else
	slsi_lbm_run_bh(hip->hip_priv->bh_ctl);
#endif
}

static int hip4_napi_poll(struct napi_struct *napi, int budget)
{
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	struct hip_priv        *hip_priv = container_of(napi, struct hip_priv, napi);
#else
	struct hip_priv        *hip_priv = slsi_lbm_get_hip_priv_from_napi(napi);
#endif
	struct slsi_hip        *hip;
	struct hip4_hip_control *ctrl;
	struct scsc_service     *service;
	struct slsi_dev         *sdev;
#ifdef CONFIG_SCSC_WLAN_TPUT_MONITOR
	struct net_device       *dev;
	struct napi_stat        *stat;
#endif

#ifdef CONFIG_SCSC_WLAN_DEBUG
	int                     id;
#endif
	u8                      idx_r;
	u8                      idx_w;
	scsc_mifram_ref         ref;
	void                    *mem;
	struct mbulk            *m;
	u8                      retry;
	u8                      todo = 0;
	int			work_done = 0;
	int			hip_state;

	spin_lock_bh(&in_napi_context);
	if (!hip_priv || !hip_priv->hip) {
		SLSI_ERR_NODEV("hip_priv or hip_priv->hip is Null\n");
		spin_unlock_bh(&in_napi_context);
		return 0;
	}

	hip = hip_priv->hip;
	if (!hip || !hip->hip_priv) {
		SLSI_ERR_NODEV("either hip or hip->hip_priv is Null\n");
		spin_unlock_bh(&in_napi_context);
		return 0;
	}

	ctrl = hip->hip_control;

	if (!ctrl) {
		SLSI_ERR_NODEV("hip->hip_control is Null\n");
		spin_unlock_bh(&in_napi_context);
		return 0;
	}
	sdev = container_of(hip, struct slsi_dev, hip);

	if (!sdev || !sdev->service) {
		SLSI_ERR_NODEV("sdev or sdev->service is Null\n");
		spin_unlock_bh(&in_napi_context);
		return 0;
	}

	hip_state = atomic_read(&sdev->hip.hip_state);

	if (hip_state != SLSI_HIP_STATE_STARTED) {
		SLSI_DBG3(sdev, SLSI_RX, "hip cycles can not run(hip state:%d)\n", hip_state);
		napi_complete(napi);
		spin_unlock_bh(&in_napi_context);
		return 0;
	}

	spin_lock_bh(&hip_priv->rx_lock);
	SCSC_HIP4_SAMPLER_INT_BH(hip->hip_priv->minor, 0);
	if (ktime_compare(bh_init_data, bh_end_data) <= 0) {
		bh_init_data = ktime_get();
		if (!atomic_read(&hip->hip_priv->closing))
			atomic_set(&hip->hip_priv->watchdog_timer_active, 0);
	}
	clear_bit(HIP4_MIF_Q_TH_DAT, hip->hip_priv->irq_bitmap);

	idx_r = hip4_read_index(hip, HIP4_MIF_Q_TH_DAT, ridx);
	idx_w = hip4_read_index(hip, HIP4_MIF_Q_TH_DAT, widx);

	service = sdev->service;

	if (idx_r == idx_w) {
		SLSI_DBG3(sdev, SLSI_RX, "nothing to do, NAPI Complete\n");
		hip->hip_priv->napi_rx_full_cnt = 0;
		hip_priv->napi_rx_saturated = 0;
		bh_end_data = ktime_get();
		napi_complete(napi);
		if (!atomic_read(&hip->hip_priv->closing)) {
			/* Nothing more to drain, unmask interrupt */
			scsc_service_mifintrbit_bit_unmask(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
		}

		if (slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_data)) {
			slsi_wake_unlock(&hip->hip_priv->hip4_wake_lock_data);
#ifdef CONFIG_SCSC_WLAN_ANDROID
			SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_RELEASED, "hip4_wake_lock_data", WL_REASON_RX);
#endif
		}

		goto end;
	}

#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
	if (idx_r != idx_w) {
		SCSC_HIP4_SAMPLER_Q(hip_priv->minor, HIP4_MIF_Q_TH_DAT, ridx, idx_r, 1);
		SCSC_HIP4_SAMPLER_Q(hip_priv->minor, HIP4_MIF_Q_TH_DAT, widx, idx_w, 1);
	}
#endif

	todo = ((idx_w - idx_r) & 0xff);
	SLSI_DBG3(sdev, SLSI_RX, "todo:%hhu\n", todo);

#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	/* check if Rx queue is saturating and if so move to performance mode */
	if (hip4_napi_rx_saturation_detection && !napi_select_cpu && !hip_priv->napi_perf_mode && !hip_priv->napi_rx_saturated) {
		if (todo > hip4_napi_num_rx_pkts)
		{
			if (++hip_priv->napi_rx_full_cnt >= hip4_napi_num_rx_full) {
				SLSI_INFO(sdev, "Rx queue saturating.. move to performance mode\n");
				hip_priv->napi_rx_saturated = 1;
				/* signal to traffic monitor to notify clients */
				slsi_traffic_mon_override(sdev);
			}
		} else {
			hip_priv->napi_rx_full_cnt = 0;
		}
	}
#endif
	while (idx_r != idx_w) {
		struct sk_buff *skb;
		/* TODO: currently the max number to be freed is 2. In future
		 * implementations (i.e. AMPDU) this number may be bigger
		 */
		/* list of mbulks to be freed */
		scsc_mifram_ref to_free[MBULK_MAX_CHAIN + 1] = { 0 };
		u8              i = 0;

		/* Catch-up with idx_w */
		ref = ctrl->q[HIP4_MIF_Q_TH_DAT].array[idx_r];
#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
		SCSC_HIP4_SAMPLER_QREF(hip_priv->minor, ref, HIP4_MIF_Q_TH_DAT);
#endif
		mem = scsc_mx_service_mif_addr_to_ptr(service, ref);
		m = (struct mbulk *)(mem);
		skb = hip4_mbulk_to_skb(service, hip_priv, m, to_free, true);
		if (!skb) {
			SLSI_ERR_NODEV("Dat: Error parsing or allocating skb\n");
			hip4_dump_dbg(hip, m, skb, service);
			goto consume_dat_mbulk;
		}

		if (m->flag & MBULK_F_WAKEUP) {
			SLSI_INFO(sdev, "WIFI wakeup by DATA frame vif:%d, pid:%d, sigid:0x%x\n",
				  fapi_get_vif(skb), fapi_get_u16(skb, receiver_pid), fapi_get_sigid(skb));
			slsi_skb_cb_get(skb)->wakeup = true;
			slsi_dump_eth_packet(sdev, skb);
		}

#ifdef CONFIG_SCSC_WLAN_DEBUG
		id = fapi_get_sigid(skb);
		hip4_history_record_add(TH, id);
#endif
#ifdef CONFIG_SCSC_SMAPPER
		/* Check SMAPPER flag */
		if (m->flag & MBULK_F_SMAPPER) {
			slsi_skb_cb_get(skb)->smapper_linked = true;
		}
#endif
		if (slsi_hip_rx(sdev, skb) < 0) {
			SLSI_ERR_NODEV("Dat: Error detected slsi_hip_rx\n");
			hip4_dump_dbg(hip, m, skb, service);
#ifdef CONFIG_SCSC_SMAPPER
			hip4_smapper_free_mapped_skb(skb);
#endif
			kfree_skb(skb);
		}
consume_dat_mbulk:
		/* Increase index */
		idx_r++;
		idx_r &= (MAX_NUM - 1);

		while ((ref = to_free[i++])) {
			/* Set the number of retries */
			retry = FB_NO_SPC_NUM_RET;
			while (hip4_q_add_signal(hip, HIP4_MIF_Q_TH_RFB, ref, service) && (!atomic_read(&hip->hip_priv->closing)) && (retry > 0)) {
				SLSI_WARN_NODEV("Data: Not enough space in FB, retry of %d/%d\n",
						((FB_NO_SPC_NUM_RET + 1) - retry), FB_NO_SPC_NUM_RET);
				udelay(FB_NO_SPC_DELAY_US);
				retry--;

				if (retry == 0)
					SLSI_ERR_NODEV("Dat: FB has not been freed for %d us\n", FB_NO_SPC_NUM_RET * FB_NO_SPC_DELAY_US);
#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
				SCSC_HIP4_SAMPLER_QFULL(hip_priv->minor, HIP4_MIF_Q_TH_RFB);
#endif
			}
		}

		work_done++;
		if (budget == work_done) {
			bh_end_data = ktime_get();
			/* We have consumed all the budget */
			break;
		}
	}

	hip4_update_index(hip, HIP4_MIF_Q_TH_DAT, ridx, idx_r);

	if (work_done < budget) {
		SLSI_DBG3(sdev, SLSI_RX, "NAPI complete (work_done:%d)\n", work_done);
		bh_end_data = ktime_get();
		hip->hip_priv->napi_rx_full_cnt = 0;
		hip_priv->napi_rx_saturated = 0;
		napi_complete(napi);
		if (!atomic_read(&hip->hip_priv->closing)) {
			/* Nothing more to drain, unmask interrupt */
			scsc_service_mifintrbit_bit_unmask(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
		}
		if (slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_data)) {
			slsi_wake_unlock(&hip->hip_priv->hip4_wake_lock_data);
#ifdef CONFIG_SCSC_WLAN_ANDROID
			SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_RELEASED, "hip4_wake_lock_data", WL_REASON_RX);
#endif
		}
	}
end:
#ifdef CONFIG_SCSC_WLAN_TPUT_MONITOR
	rcu_read_lock();
	dev = slsi_get_netdev_rcu(sdev, SLSI_NET_INDEX_WLAN);
	stat = &((struct netdev_vif *)netdev_priv(dev))->rx_stat;
	stat->napi_cnt += 1;
	stat->napi_todo += todo;
	stat->napi_done += work_done;
	stat->napi_time = ktime_add(ktime_sub(bh_end_data, bh_init_data), stat->napi_time);
	rcu_read_unlock();
#endif
	SLSI_DBG3(sdev, SLSI_RX, "work done:%d\n", work_done);
	SCSC_HIP4_SAMPLER_INT_OUT_BH(hip->hip_priv->minor, 0);
	spin_unlock_bh(&hip_priv->rx_lock);
	spin_unlock_bh(&in_napi_context);
	return work_done;
}

#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
static void hip4_irq_data_napi_switch_work(struct work_struct *work)
{
	struct hip_priv *hip_priv = container_of(work, struct hip_priv, intr_wq_napi_cpu_switch);
	local_bh_disable();
	napi_schedule(&hip_priv->napi);
	local_bh_enable();
}
#endif

static void hip4_irq_handler_dat(int irq, void *data)
{
	struct slsi_hip    *hip = (struct slsi_hip *)data;
	struct slsi_dev     *sdev = container_of(hip, struct slsi_dev, hip);
	unsigned long flags;

	if (!hip || !sdev || !sdev->service || !hip->hip_priv)
		return;

	SCSC_HIP4_SAMPLER_INT(hip->hip_priv->minor, 0);
	intr_received_data = ktime_get();

	if (!slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_data)) {
		slsi_wake_lock_timeout(&hip->hip_priv->hip4_wake_lock_data, msecs_to_jiffies(SLSI_HIP_WAKELOCK_TIME_OUT_IN_MS));
#ifdef CONFIG_SCSC_WLAN_ANDROID
		SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_TAKEN, "hip4_wake_lock_data", WL_REASON_RX);
#endif
	}

	if (!atomic_read(&hip->hip_priv->watchdog_timer_active)) {
		atomic_set(&hip->hip_priv->watchdog_timer_active, 1);
		mod_timer(&hip->hip_priv->watchdog, jiffies + HZ);
	}
	set_bit(HIP4_MIF_Q_TH_DAT, hip->hip_priv->irq_bitmap);

	/* Mask interrupt to avoid interrupt storm and let BH run */
	scsc_service_mifintrbit_bit_mask(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);

	spin_lock_irqsave(&hip->hip_priv->napi_cpu_lock, flags);
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	if (test_bit(SLSI_HIP_NAPI_STATE_ENABLED, &hip->hip_priv->napi_state)) {
		if (napi_select_cpu && (napi_select_cpu != smp_processor_id()) && cpu_online(napi_select_cpu))
			/* queue work on system_wq. Do not use hip4_workq as
			 * it is single thread wq and WQ_UNBOUND wouldnt be
			 * set. What it means? its not garunteed to run on
			 * intended CPU if wq is created as single threaded
			 * wq.
			 */
			schedule_work_on(napi_select_cpu, &hip->hip_priv->intr_wq_napi_cpu_switch);
		else
			napi_schedule(&hip->hip_priv->napi);

		scsc_service_mifintrbit_bit_clear(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
	}
#else
#ifdef CONFIG_SLSI_WLAN_LPC
	if (slsi_lpc_is_lpc_enabled())
		slsi_lbm_run_bh(hip->hip_priv->bh_lpc);
#endif
	if (test_bit(SLSI_HIP_NAPI_STATE_ENABLED, &hip->hip_priv->bh_dat->bh_priv.napi.napi_state)) {
		slsi_lbm_run_bh(hip->hip_priv->bh_dat);
		scsc_service_mifintrbit_bit_clear(sdev->service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
	}
#endif
	spin_unlock_irqrestore(&hip->hip_priv->napi_cpu_lock, flags);
	SCSC_HIP4_SAMPLER_INT_OUT(hip->hip_priv->minor, 0);
}

#ifdef CONFIG_SCSC_QOS
static void hip4_pm_qos_work(struct work_struct *data)
{
	struct hip_priv        *hip_priv = container_of(data, struct hip_priv, pm_qos_work);
	struct slsi_hip        *hip = hip_priv->hip;
	struct slsi_dev         *sdev = container_of(hip, struct slsi_dev, hip);
	u8 state;

	if (!sdev || !sdev->service) {
		WLBT_WARN_ON(1);
		return;
	}

	if (atomic_read(&hip->hip_priv->closing))
		return;

	SLSI_DBG1(sdev, SLSI_HIP, "update to state %d\n", hip_priv->pm_qos_state);
	spin_lock_bh(&hip_priv->pm_qos_lock);
	state = hip_priv->pm_qos_state;
	spin_unlock_bh(&hip_priv->pm_qos_lock);
#if IS_ENABLED(CONFIG_SCSC_WLAN_SEPARATED_CLUSTER_FREQUENCY)
	scsc_service_pm_qos_update_cluster_request(sdev->service, state);
#else
	scsc_service_pm_qos_update_request(sdev->service, state);
#endif
}

void hip4_traffic_monitor_cb(void *client_ctx, u32 state, u32 tput_tx, u32 tput_rx)
{
	struct slsi_hip *hip = (struct slsi_hip *)client_ctx;
	struct slsi_dev *sdev;
	u8 state_before;

	if (!hip || !hip->hip_priv) {
		SLSI_WARN_NODEV("invalid HIP instance\n");
		return;
	}

	sdev = container_of(hip, struct slsi_dev, hip);
	if (!sdev)
		return;

	if (atomic_read(&hip->hip_priv->closing))
		return;

	spin_lock_bh(&hip->hip_priv->pm_qos_lock);
	state_before = hip->hip_priv->pm_qos_state;

	SLSI_DBG1(sdev, SLSI_HIP, "state:%u --> event (state:%u, tput_tx:%u Mbps, tput_rx:%u Mbps)\n",
		state_before,
		state,
		SLSI_TRAFFIC_MON_BYTES_TO_MBPS(tput_tx),
		SLSI_TRAFFIC_MON_BYTES_TO_MBPS(tput_rx));

	if (state == TRAFFIC_MON_CLIENT_STATE_HIGH || state == TRAFFIC_MON_CLIENT_STATE_OVERRIDE)
		hip->hip_priv->pm_qos_state = SCSC_QOS_MAX;
	else if (state == TRAFFIC_MON_CLIENT_STATE_MID)
		hip->hip_priv->pm_qos_state = SCSC_QOS_MED;
	else
		hip->hip_priv->pm_qos_state = SCSC_QOS_DISABLED;

	spin_unlock_bh(&hip->hip_priv->pm_qos_lock);

	if (state_before != hip->hip_priv->pm_qos_state)
		schedule_work(&hip->hip_priv->pm_qos_work);
}
#endif

#if IS_ENABLED(CONFIG_SCSC_LOGRING)
static void hip4_traffic_monitor_logring_cb(void *client_ctx, u32 state, u32 tput_tx, u32 tput_rx)
{
	struct hip_priv *hip_priv = (struct hip_priv *)client_ctx;
	struct slsi_hip *hip = hip_priv->hip;
	struct slsi_dev *sdev = container_of(hip, struct slsi_dev, hip);

	if (!sdev)
		return;

	SLSI_DBG1(sdev, SLSI_HIP, "event (state:%u, tput_tx:%u Mbps, tput_rx:%u Mbps)\n",
		state,
		SLSI_TRAFFIC_MON_BYTES_TO_MBPS(tput_tx),
		SLSI_TRAFFIC_MON_BYTES_TO_MBPS(tput_rx));
	if (state == TRAFFIC_MON_CLIENT_STATE_HIGH || state == TRAFFIC_MON_CLIENT_STATE_MID || state == TRAFFIC_MON_CLIENT_STATE_OVERRIDE) {
		if (hip4_dynamic_logging)
			scsc_logring_enable(false);
	} else {
		scsc_logring_enable(true);
	}
}
#endif

#ifdef CONFIG_SCSC_WLAN_HOST_DPD
int slsi_hip_from_host_dpd_intr_set(struct scsc_service *service, struct slsi_hip *hip)
{
	if (!hip || !service || !hip->hip_priv) {
		SLSI_WARN_NODEV("invalid HIP instance\n");
		return -ENODEV;
	}
	scsc_service_mifintrbit_bit_set(service, hip->hip_priv->intr_from_host_dpd, SCSC_MIFINTR_TARGET_WLAN);
	return 0;
}

static void hip4_irq_handler_dpd(int irq, void *data) {
	struct slsi_hip    *hip = (struct slsi_hip *)data;
	struct slsi_dev    *sdev = container_of(hip, struct slsi_dev, hip);

	if (!hip || !sdev || !sdev->service || !hip->hip_priv) {
		SLSI_WARN_NODEV("invalid HIP instance\n");
		return;
	}
	/* Clear interrupt */
	scsc_service_mifintrbit_bit_mask(sdev->service, irq);
	scsc_service_mifintrbit_bit_clear(sdev->service, irq);

	slsi_wlan_dpd_mmap_user_space_event(SLSI_WLAN_DPD_DRV_MSG_ID_WLAN_INTR);
	scsc_service_mifintrbit_bit_unmask(sdev->service, irq);
}
#endif

int slsi_hip_init(struct slsi_hip *hip)
{
	void                    *hip_ptr;
	struct hip4_hip_control *hip_control;
	struct scsc_service     *service;
	struct slsi_dev         *sdev = container_of(hip, struct slsi_dev, hip);
	scsc_mifram_ref         ref, ref_scoreboard;
	int                     i;
	int                     ret;
	u32                     total_mib_len;
	u32                     mib_file_offset;
	struct net_device       *dev;
#ifdef CONFIG_SCSC_WLAN_TPUT_MONITOR
	struct napi_stat        *stat;
#endif

	if (!sdev || !sdev->service)
		return -EINVAL;

	hip->hip_priv = kzalloc(sizeof(*hip->hip_priv), GFP_KERNEL);
	if (!hip->hip_priv)
		return -ENOMEM;

	SLSI_INFO_NODEV("HIP4_WLAN_CONFIG_SIZE (%d)\n", HIP4_WLAN_CONFIG_SIZE);
	SLSI_INFO_NODEV("HIP4_WLAN_MIB_SIZE (%d)\n", HIP4_WLAN_MIB_SIZE);
	SLSI_INFO_NODEV("HIP4_WLAN_TX_DAT_SIZE (%d)\n", HIP4_WLAN_TX_DAT_SIZE);
	SLSI_INFO_NODEV("HIP4_WLAN_TX_CTL_SIZE (%d)\n", HIP4_WLAN_TX_CTL_SIZE);
	SLSI_INFO_NODEV("HIP4_WLAN_RX_SIZE (%d)\n", HIP4_WLAN_RX_SIZE);
#ifdef CONFIG_SCSC_WLAN_HOST_DPD
	SLSI_INFO_NODEV("HIP4_WLAN_DPD_BUF_SIZE (%d)\n", HIP4_WLAN_DPD_BUF_SIZE);
#endif
	SLSI_INFO_NODEV("HIP4_WLAN_TOTAL_MEM (%d)\n", HIP4_WLAN_TOTAL_MEM);
	SLSI_INFO_NODEV("HIP4_DAT_SLOTS (%d)\n", HIP4_DAT_SLOTS);
	SLSI_INFO_NODEV("HIP4_CTL_SLOTS (%d)\n", HIP4_CTL_SLOTS);
#ifdef CONFIG_SCSC_WLAN_DEBUG
	memset(&hip->hip_priv->stats, 0, sizeof(hip->hip_priv->stats));
	hip->hip_priv->stats.start = ktime_get();
	hip->hip_priv->stats.procfs_dir = proc_mkdir("driver/hip4", NULL);
	if (hip->hip_priv->stats.procfs_dir) {
		proc_create_data("info", S_IRUSR | S_IRGRP,
				 hip->hip_priv->stats.procfs_dir, &hip4_procfs_stats_fops, hip);
		proc_create_data("history", S_IRUSR | S_IRGRP,
				 hip->hip_priv->stats.procfs_dir, &hip4_procfs_history_fops, hip);
		proc_create_data("jitter", S_IRUSR | S_IRGRP,
				 hip->hip_priv->stats.procfs_dir, &hip4_procfs_jitter_fops, hip);
	}

	hip->hip_priv->minor = hip4_sampler_register_hip(sdev->maxwell_core);
	if (hip->hip_priv->minor < SCSC_HIP4_INTERFACES) {
		SLSI_DBG1_NODEV(SLSI_HIP, "registered with minor %d\n", hip->hip_priv->minor);
		sdev->minor_prof = hip->hip_priv->minor;
	} else {
		SLSI_DBG1_NODEV(SLSI_HIP, "hip4_sampler is not enabled\n");
	}
#endif

	/* Used in the workqueue */
	hip->hip_priv->hip = hip;

	service = sdev->service;

	hip->hip_priv->host_pool_id_dat = MBULK_POOL_ID_DATA;
	hip->hip_priv->host_pool_id_ctl = MBULK_POOL_ID_CTRL;

	/* hip_ref contains the reference of the start of shared memory allocated for WLAN */
	/* hip_ptr is the kernel address of hip_ref*/
	hip_ptr = scsc_mx_service_mif_addr_to_ptr(service, hip->hip_ref);

#ifdef CONFIG_SCSC_WLAN_DEBUG
	/* Configure mbulk allocator - Data QUEUES */
	ret = mbulk_pool_add(MBULK_POOL_ID_DATA, hip_ptr + HIP4_WLAN_TX_DAT_OFFSET,
			     hip_ptr + HIP4_WLAN_TX_DAT_OFFSET + HIP4_WLAN_TX_DAT_SIZE,
			     (HIP4_WLAN_TX_DAT_SIZE / HIP4_DAT_SLOTS) - sizeof(struct mbulk), 5,
			     hip->hip_priv->minor);
	if (ret)
		return ret;

	/* Configure mbulk allocator - Control QUEUES */
	ret = mbulk_pool_add(MBULK_POOL_ID_CTRL, hip_ptr + HIP4_WLAN_TX_CTL_OFFSET,
			     hip_ptr + HIP4_WLAN_TX_CTL_OFFSET + HIP4_WLAN_TX_CTL_SIZE,
			     (HIP4_WLAN_TX_CTL_SIZE / HIP4_CTL_SLOTS) - sizeof(struct mbulk), 0,
			     hip->hip_priv->minor);
	if (ret)
		return ret;
#else
	/* Configure mbulk allocator - Data QUEUES */
	ret = mbulk_pool_add(MBULK_POOL_ID_DATA, hip_ptr + HIP4_WLAN_TX_DAT_OFFSET,
			     hip_ptr + HIP4_WLAN_TX_DAT_OFFSET + HIP4_WLAN_TX_DAT_SIZE,
			     (HIP4_WLAN_TX_DAT_SIZE / HIP4_DAT_SLOTS) - sizeof(struct mbulk), 5);
	if (ret)
		return ret;

	/* Configure mbulk allocator - Control QUEUES */
	ret = mbulk_pool_add(MBULK_POOL_ID_CTRL, hip_ptr + HIP4_WLAN_TX_CTL_OFFSET,
			     hip_ptr + HIP4_WLAN_TX_CTL_OFFSET + HIP4_WLAN_TX_CTL_SIZE,
			    (HIP4_WLAN_TX_CTL_SIZE / HIP4_CTL_SLOTS) - sizeof(struct mbulk), 0);
	if (ret)
		return ret;
#endif

	/* Reset hip_control table */
	memset(hip_ptr, 0, sizeof(struct hip4_hip_control));

	/* Reset Sample q values sending 0xff */
	SCSC_HIP4_SAMPLER_RESET(hip->hip_priv->minor);

	/***** VERSION 4 *******/
	/* TOHOST Handler allocator */
	/* Q0 FH CTRL */
	hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_FH_CTRL] = MIF_NO_IRQ;
	/* Q1 FH DATA */
	hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_FH_DAT] = MIF_NO_IRQ;
	/* Q5 TH RFB */
	hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_RFB] = MIF_NO_IRQ;
	/* Q2 FH FB */
	hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_FH_RFB] =
		scsc_service_mifintrbit_register_tohost(service, hip4_irq_handler_fb, hip, SCSC_MIFINTR_TARGET_WLAN, HIP4_IRQ_HANDLER_FB_TYPE);
	scsc_service_mifintrbit_bit_mask(service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_FH_RFB]);
	/* Q3 TH CTRL */
	hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_CTRL] =
		scsc_service_mifintrbit_register_tohost(service, hip4_irq_handler_ctrl, hip, SCSC_MIFINTR_TARGET_WLAN, HIP4_IRQ_HANDLER_CTRL_TYPE);
	scsc_service_mifintrbit_bit_mask(service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_CTRL]);
	/* Q4 TH DAT */
	hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT] =
		scsc_service_mifintrbit_register_tohost(service, hip4_irq_handler_dat, hip, SCSC_MIFINTR_TARGET_WLAN, HIP4_IRQ_HANDLER_DATA_TYPE);
	scsc_service_mifintrbit_bit_mask(service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);

	rcu_read_lock();
	/* one NAPI instance is ok for multiple netdev devices */
	dev = slsi_get_netdev_rcu(sdev, SLSI_NET_INDEX_WLAN);
	if (!dev) {
		SLSI_ERR(sdev, "netdev No longer exists\n");
		rcu_read_unlock();
		return -EINVAL;
	}
	rcu_read_unlock();

#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	netif_napi_add(dev, &hip->hip_priv->napi, hip4_napi_poll);
#else
	netif_napi_add(dev, &hip->hip_priv->napi, hip4_napi_poll, NAPI_POLL_WEIGHT);
#endif
#else
#ifdef CONFIG_SLSI_WLAN_LPC
	hip->hip_priv->bh_lpc = slsi_lbm_register_workqueue(sdev, hip4_lpc_work, -1);
	if (!hip->hip_priv->bh_lpc) {
		SLSI_ERR_NODEV("Error allocating bh_lpc\n");
		return -ENOMEM;
	}
#endif
	hip->hip_priv->bh_dat = slsi_lbm_register_napi(sdev, hip4_napi_poll,
						       hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT], NP_RX_0);
	if (hip->hip_priv->bh_dat) {
		slsi_lbm_register_cpu_affinity_control(hip->hip_priv->bh_dat, NP_RX_0);
		slsi_lbm_register_io_saturation_control(hip->hip_priv->bh_dat);
	} else {
		SLSI_ERR_NODEV("Error allocating bh\n");
		return -ENOMEM;
	}
#endif
#ifdef CONFIG_SCSC_WLAN_TPUT_MONITOR
	SLSI_ERR_NODEV("initialize traffic_monitor stats\n");
	stat = &((struct netdev_vif *)netdev_priv(dev))->rx_stat;
	memset(stat, 0, sizeof(struct napi_stat));
#endif

	/* FROMHOST Handler allocator */
	hip->hip_priv->intr_fromhost =
		scsc_service_mifintrbit_alloc_fromhost(service, SCSC_MIFINTR_TARGET_WLAN);

#ifdef CONFIG_SCSC_WLAN_HOST_DPD
	/* TOHOST interrupt for DPD */
	hip->hip_priv->intr_to_host_dpd =
		scsc_service_mifintrbit_register_tohost(service, hip4_irq_handler_dpd, hip, SCSC_MIFINTR_TARGET_WLAN, HIP4_IRQ_HANDLER_DPD_TYPE);
	scsc_service_mifintrbit_bit_mask(service, hip->hip_priv->intr_to_host_dpd);

	/* FROMHOST interrupt for DPD */
	hip->hip_priv->intr_from_host_dpd =
		scsc_service_mifintrbit_alloc_fromhost(service, SCSC_MIFINTR_TARGET_WLAN);
#endif

	/* Get hip_control pointer on shared memory  */
	hip_control = (struct hip4_hip_control *)(hip_ptr +
		      HIP4_WLAN_CONFIG_OFFSET);

	/* Initialize scoreboard */
	if (scsc_mx_service_mif_ptr_to_addr(service, &hip_control->scoreboard, &ref_scoreboard))
		return -EFAULT;

	/* Calculate total space used by wlan*.hcf files */
	for (i = 0, total_mib_len = 0; i < SLSI_WLAN_MAX_MIB_FILE; i++)
		total_mib_len += sdev->mib[i].mib_len;

	/* Copy MIB content in shared memory if any */
	/* Clear the area to avoid picking up old values */
	memset(hip_ptr + HIP4_WLAN_MIB_OFFSET, 0, HIP4_WLAN_MIB_SIZE);

	if (total_mib_len > HIP4_WLAN_MIB_SIZE) {
		SLSI_ERR_NODEV("MIB size (%d), is bigger than the MIB AREA (%d). Aborting memcpy\n", total_mib_len, HIP4_WLAN_MIB_SIZE);
		hip_control->config_v4.mib_loc      = 0;
		hip_control->config_v4.mib_sz       = 0;
		hip_control->config_v5.mib_loc      = 0;
		hip_control->config_v5.mib_sz       = 0;
		total_mib_len = 0;
	} else if (total_mib_len) {
		SLSI_INFO_NODEV("Loading MIB into shared memory, size (%d)\n", total_mib_len);
		/* Load each MIB file into shared DRAM region */
		for (i = 0, mib_file_offset = 0;
		     i < SLSI_WLAN_MAX_MIB_FILE && sdev->mib[i].mib_file_name;
		     i++) {
			SLSI_INFO_NODEV("Loading MIB %d into shared memory, offset (%d), size (%d), total (%d)\n", i, mib_file_offset, sdev->mib[i].mib_len, total_mib_len);
			if (sdev->mib[i].mib_len) {
				memcpy((u8 *)hip_ptr + HIP4_WLAN_MIB_OFFSET + mib_file_offset, sdev->mib[i].mib_data, sdev->mib[i].mib_len);
				mib_file_offset += sdev->mib[i].mib_len;
			}
		}
		hip_control->config_v4.mib_loc      = hip->hip_ref + HIP4_WLAN_MIB_OFFSET;
		hip_control->config_v4.mib_sz       = total_mib_len;
		hip_control->config_v5.mib_loc      = hip->hip_ref + HIP4_WLAN_MIB_OFFSET;
		hip_control->config_v5.mib_sz       = total_mib_len;
	} else {
		hip_control->config_v4.mib_loc      = 0;
		hip_control->config_v4.mib_sz       = 0;
		hip_control->config_v5.mib_loc      = 0;
		hip_control->config_v5.mib_sz       = 0;
	}

	/* Initialize hip_control table for version 4 */
	/***** VERSION 4 *******/
	hip_control->config_v4.magic_number = 0xcaba0401;
	hip_control->config_v4.hip_config_ver = 4;
	hip_control->config_v4.config_len = sizeof(struct hip4_hip_config_version_4);
	hip_control->config_v4.host_cache_line = 64;
	hip_control->config_v4.host_buf_loc = hip->hip_ref + HIP4_WLAN_TX_OFFSET;
	hip_control->config_v4.host_buf_sz  = HIP4_WLAN_TX_SIZE;
	hip_control->config_v4.fw_buf_loc   = hip->hip_ref + HIP4_WLAN_RX_OFFSET;
	hip_control->config_v4.fw_buf_sz    = HIP4_WLAN_RX_SIZE;
	hip_control->config_v4.log_config_loc = 0;
#ifdef CONFIG_SCSC_WLAN_HOST_DPD
	hip_control->config_v4.dpd_buf_loc   = hip->hip_ref + HIP4_WLAN_DPD_BUF_OFFSET;
	hip_control->config_v4.dpd_buf_sz    = HIP4_WLAN_DPD_BUF_SIZE;
	hip_control->config_v4.intr_from_host_dpd = hip->hip_priv->intr_from_host_dpd;
	hip_control->config_v4.intr_to_host_dpd = hip->hip_priv->intr_to_host_dpd;
	slsi_wlan_dpd_mmap_set_buffer(sdev, (hip_ptr + HIP4_WLAN_DPD_BUF_OFFSET), HIP4_WLAN_DPD_BUF_SIZE);
#endif
	hip_control->config_v4.mif_fh_int_n = hip->hip_priv->intr_fromhost;
	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++) {
		hip_control->config_v4.mif_th_int_n[i] = hip->hip_priv->intr_tohost_mul[i];
	}

	hip_control->config_v4.scbrd_loc = (u32)ref_scoreboard;
	hip_control->config_v4.q_num = 6;
	hip_control->config_v4.q_len = 256;
	hip_control->config_v4.q_idx_sz = 1;
	/* Initialize q relative positions */
	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++) {
		if (scsc_mx_service_mif_ptr_to_addr(service, &hip_control->q[i].array, &ref))
			return -EFAULT;
		hip_control->config_v4.q_loc[i] = (u32)ref;
	}
	/***** END VERSION 4 *******/

	/* Initialize hip_control table for version 5 */
	/***** VERSION 5 *******/
	hip_control->config_v5.magic_number = 0xcaba0401;
	hip_control->config_v5.hip_config_ver = 5;
	hip_control->config_v5.config_len = sizeof(struct hip4_hip_config_version_5);
	hip_control->config_v5.host_cache_line = 64;
	hip_control->config_v5.host_buf_loc = hip->hip_ref + HIP4_WLAN_TX_OFFSET;
	hip_control->config_v5.host_buf_sz  = HIP4_WLAN_TX_SIZE;
	hip_control->config_v5.fw_buf_loc   = hip->hip_ref + HIP4_WLAN_RX_OFFSET;
	hip_control->config_v5.fw_buf_sz    = HIP4_WLAN_RX_SIZE;
	hip_control->config_v5.log_config_loc = 0;
	hip_control->config_v5.mif_fh_int_n = hip->hip_priv->intr_fromhost;
	hip_control->config_v5.mif_th_int_n = hip->hip_priv->intr_tohost;
	hip_control->config_v5.q_num = 6;
	hip_control->config_v5.q_len = 256;
	hip_control->config_v5.q_idx_sz = 1;
	hip_control->config_v5.scbrd_loc = (u32)ref_scoreboard; /* scoreborad location */

	/* Initialize q relative positions */
	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++) {
		if (scsc_mx_service_mif_ptr_to_addr(service, &hip_control->q[i].array, &ref))
			return -EFAULT;
		hip_control->config_v5.q_loc[i] = (u32)ref;
	}
	/***** END VERSION 5 *******/

	/* Initialzie hip_init configuration */
	hip_control->init.magic_number = 0xcaaa0400;
	if (scsc_mx_service_mif_ptr_to_addr(service, &hip_control->config_v4, &ref))
		return -EFAULT;
	hip_control->init.version_a_ref = ref;

	if (scsc_mx_service_mif_ptr_to_addr(service, &hip_control->config_v5, &ref))
		return -EFAULT;
	hip_control->init.version_b_ref = ref;
	/* End hip_init configuration */

	hip->hip_control = hip_control;
	hip->hip_priv->scbrd_base = &hip_control->scoreboard;

	spin_lock_init(&hip->hip_priv->rx_lock);
	atomic_set(&hip->hip_priv->in_rx, 0);
	spin_lock_init(&hip->hip_priv->tx_lock);
	atomic_set(&hip->hip_priv->in_tx, 0);

	slsi_wake_lock_init(NULL, &hip->hip_priv->hip4_wake_lock_tx, "hip4_wake_lock_tx");
	slsi_wake_lock_init(NULL, &hip->hip_priv->hip4_wake_lock_ctrl, "hip4_wake_lock_ctrl");
	slsi_wake_lock_init(NULL, &hip->hip_priv->hip4_wake_lock_data, "hip4_wake_lock_data");

	/* Init work structs */
	hip->hip_priv->hip4_workq = create_singlethread_workqueue("hip4_work");
	if (!hip->hip_priv->hip4_workq) {
		SLSI_ERR_NODEV("Error creating singlethread_workqueue\n");
		return -ENOMEM;
	}
	spin_lock_init(&hip->hip_priv->napi_cpu_lock);
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	INIT_WORK(&hip->hip_priv->intr_wq_napi_cpu_switch, hip4_irq_data_napi_switch_work);
	INIT_WORK(&hip->hip_priv->intr_wq_ctrl, hip4_wq_ctrl);
	tasklet_init(&hip->hip_priv->intr_tl_fb, hip4_tl_fb, (unsigned long)hip);
#else
	hip->hip_priv->bh_ctl = slsi_lbm_register_workqueue(sdev, hip4_wq_ctrl, -1);
	hip->hip_priv->bh_rfb = slsi_lbm_register_tasklet(sdev, hip4_tl_fb, -1);
	if (!hip->hip_priv->bh_ctl || !hip->hip_priv->bh_rfb) {
		SLSI_ERR_NODEV("Error allocating bh\n");
		return -ENOMEM;
	}
#endif
	rwlock_init(&hip->hip_priv->rw_scoreboard);

	/* Setup watchdog timer */
	atomic_set(&hip->hip_priv->watchdog_timer_active, 0);
	spin_lock_init(&hip->hip_priv->watchdog_lock);
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	timer_setup(&hip->hip_priv->watchdog, hip4_watchdog, 0);
#else
	setup_timer(&hip->hip_priv->watchdog, hip4_watchdog, (unsigned long)hip);
#endif

	atomic_set(&hip->hip_priv->gmod, HIP4_DAT_SLOTS);
	atomic_set(&hip->hip_priv->gactive, 1);
	spin_lock_init(&hip->hip_priv->gbot_lock);
	hip->hip_priv->saturated = 0;

#ifdef CONFIG_SCSC_SMAPPER
	/* Init SMAPPER */
	if (hip4_smapper_enable) {
		if (hip4_smapper_init(sdev, hip)) {
			SLSI_ERR_NODEV("Error on hip4_smapper init\n");
			hip4_smapper_is_enabled = false;
		} else {
			hip4_smapper_is_enabled = true;
		}
	}
#endif
#ifdef CONFIG_SCSC_QOS
	/* setup for PM QoS */
	spin_lock_init(&hip->hip_priv->pm_qos_lock);

	if (hip4_qos_enable) {
		if (!scsc_service_pm_qos_add_request(service, SCSC_QOS_DISABLED)) {
			/* register to traffic monitor for throughput events */
			hip->qos_client = slsi_traffic_mon_client_register(sdev,
									   hip,
									   TRAFFIC_MON_CLIENT_MODE_EVENTS,
									   SLSI_TRAFFIC_MON_MBPS_TO_BYTES_PER_SEC(hip4_qos_med_tput_in_mbps),
									   SLSI_TRAFFIC_MON_MBPS_TO_BYTES_PER_SEC(hip4_qos_max_tput_in_mbps),
									   TRAFFIC_MON_DIR_DEFAULT,
									   hip4_traffic_monitor_cb);

			if (!(hip->qos_client))
				SLSI_WARN(sdev, "failed to add PM QoS client to traffic monitor\n");
			else
				INIT_WORK(&hip->hip_priv->pm_qos_work, hip4_pm_qos_work);
		} else {
			SLSI_WARN(sdev, "failed to add PM QoS request\n");
		}
	}
#endif
#if IS_ENABLED(CONFIG_SCSC_LOGRING)
	/* register to traffic monitor for dynamic logring logging */
	hip->hip_priv->logring_client = slsi_traffic_mon_client_register(sdev,
									 hip->hip_priv,
									 TRAFFIC_MON_CLIENT_MODE_EVENTS,
									 0,
									 SLSI_TRAFFIC_MON_MBPS_TO_BYTES_PER_SEC(hip4_dynamic_logging_tput_in_mbps),
									 TRAFFIC_MON_DIR_DEFAULT,
									 hip4_traffic_monitor_logring_cb);
	if (!(hip->hip_priv->logring_client))
		SLSI_WARN(sdev, "failed to add Logring client to traffic monitor\n");
#endif
	return 0;
}

/**
 * This function returns the number of free slots available to
 * transmit control packet.
 */
int slsi_hip_free_control_slots_count(struct slsi_hip *hip)
{
	return mbulk_pool_get_free_count(MBULK_POOL_ID_CTRL);
}

static bool slsi_hip_validate_size(struct sk_buff *skb, bool ctrl_packet, u8 head_tail_room)
{
	size_t payload, total_size;
	struct slsi_skb_cb *cb = slsi_skb_cb_get(skb);

	payload = skb->len - cb->sig_length;
	if (payload)
		total_size = cb->sig_length + head_tail_room;
	else
		total_size = cb->sig_length;

	if (mbulk_pool_seg_size(ctrl_packet ? MBULK_POOL_ID_CTRL : MBULK_POOL_ID_DATA) < total_size)
		return false;
	return true;
}

/**
 * This function is in charge to transmit a frame through the HIP.
 * It does NOT take ownership of the SKB unless it successfully transmit it;
 * as a consequence skb is NOT freed on error.
 * We return ENOSPC on queue related troubles in order to trigger upper
 * layers of kernel to requeue/retry.
 * We free ONLY locally-allocated stuff.
 *
 * the vif_index, peer_index, priority fields are valid for data packets only
 */
int slsi_hip_transmit_frame(struct slsi_hip *hip, struct sk_buff *skb, bool ctrl_packet, u8 vif_index, u8 peer_index, u8 priority)
{
	struct scsc_service       *service;
	scsc_mifram_ref           offset = 0;
	struct mbulk              *m = NULL;
	mbulk_colour              colour = 0;
	struct slsi_dev           *sdev = container_of(hip, struct slsi_dev, hip);
	struct fapi_signal_header *fapi_header;
	int                       ret = 0;
	struct netdev_vif         *ndev_vif;

	if (!hip || !sdev || !sdev->service || !skb || !hip->hip_priv)
		return -EINVAL;

	spin_lock_bh(&hip->hip_priv->tx_lock);
	atomic_set(&hip->hip_priv->in_tx, 1);

	if (!slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_tx)) {
		slsi_wake_lock_timeout(&hip->hip_priv->hip4_wake_lock_tx, msecs_to_jiffies(SLSI_HIP_WAKELOCK_TIME_OUT_IN_MS));
#ifdef CONFIG_SCSC_WLAN_ANDROID
		SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_TAKEN, "hip4_wake_lock_tx", WL_REASON_TX);
#endif
	}

	service = sdev->service;
	fapi_header = (struct fapi_signal_header *)skb->data;

	if (fapi_is_ma(skb)) {
		u8 vif_index_colour = vif_index;

		if (skb->dev) {
			ndev_vif = netdev_priv(skb->dev);
			if (ndev_vif->iftype != NL80211_IFTYPE_AP_VLAN)
				vif_index_colour = ndev_vif->ifnum;
		}
#ifndef CONFIG_SCSC_WLAN_TX_API
		SLSI_MBULK_COLOUR_SET(colour, vif_index_colour, peer_index, priority);
#else
		SLSI_MBULK_COLOUR_SET(colour, vif_index_colour, priority);
#endif
	}
	m = hip4_skb_to_mbulk(service, hip->hip_priv, skb, ctrl_packet, colour);
	if (!m) {
		SCSC_HIP4_SAMPLER_MFULL(hip->hip_priv->minor);
		if (!slsi_hip_validate_size(skb, ctrl_packet,
					    hip->hip_priv->unidat_req_headroom + hip->hip_priv->unidat_req_tailroom)) {
			ret = -ENOSPC;
			SLSI_ERR_NODEV("mbulk is NULL\n");
		} else {
			ret = -EINVAL;
			SLSI_ERR_NODEV("High payload length\n");
		}
		goto error;
	}

	if (scsc_mx_service_mif_ptr_to_addr(service, m, &offset) < 0) {
		mbulk_chain_free(m, service);
		ret = -EFAULT;
		SLSI_ERR_NODEV("Incorrect reference memory\n");
		goto error;
	}

	if (hip4_q_add_signal(hip, ctrl_packet ? HIP4_MIF_Q_FH_CTRL : HIP4_MIF_Q_FH_DAT, offset, service)) {
		SCSC_HIP4_SAMPLER_QFULL(hip->hip_priv->minor, ctrl_packet ? HIP4_MIF_Q_FH_CTRL : HIP4_MIF_Q_FH_DAT);
		mbulk_chain_free(m, service);
		ret = -ENOSPC;
		SLSI_ERR_NODEV("No space\n");
		goto error;
	}

#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
	if (ctrl_packet) {
		/* Record control signal */
		SCSC_HIP4_SAMPLER_SIGNAL_CTRLTX(hip->hip_priv->minor, (fapi_header->id & 0xff00) >> 8, fapi_header->id & 0xff);
	} else {
#if 0
		SCSC_HIP4_SAMPLER_PKT_TX_HIP4(hip->hip_priv->minor, fapi_get_u16(skb, u.ma_unitdata_req.host_tag));
#endif
		SCSC_HIP4_SAMPLER_VIF_PEER(hip->hip_priv->minor, 1, vif_index, peer_index);
	}
#endif
#ifdef CONFIG_SCSC_WLAN_DEBUG
	hip4_history_record_add(FH, fapi_header->id);
#endif

	/* Here we push a copy of the bare skb TRANSMITTED data also to the logring
	 * as a binary record. Note that bypassing UDI subsystem as a whole
	 * means we are losing:
	 *   UDI filtering / UDI Header INFO / UDI QueuesFrames Throttling /
	 *   UDI Skb Asynchronous processing
	 * We keep separated DATA/CTRL paths.
	 */
#ifndef CONFIG_SCSC_USER_IMAGE
	if (ctrl_packet)
		SCSC_BIN_TAG_DEBUG(BIN_WIFI_CTRL_TX, skb->data, skb_headlen(skb));
	else
		SCSC_BIN_TAG_DEBUG(BIN_WIFI_DATA_TX, skb->data, skb_headlen(skb));
#endif
	/* slsi_log_clients_log_signal_fast: skb is copied to all the log clients */
	slsi_log_clients_log_signal_fast(sdev, &sdev->log_clients, skb, SLSI_LOG_DIRECTION_FROM_HOST);
	consume_skb(skb);
	atomic_set(&hip->hip_priv->in_tx, 0);
	spin_unlock_bh(&hip->hip_priv->tx_lock);
	return 0;

error:
	if (slsi_wake_lock_active(&hip->hip_priv->hip4_wake_lock_tx)) {
		slsi_wake_unlock(&hip->hip_priv->hip4_wake_lock_tx);
#ifdef CONFIG_SCSC_WLAN_ANDROID
		SCSC_WLOG_WAKELOCK(WLOG_LAZY, WL_RELEASED, "hip4_wake_lock_tx", WL_REASON_TX);
#endif
	}
	atomic_set(&hip->hip_priv->in_tx, 0);
	spin_unlock_bh(&hip->hip_priv->tx_lock);
	return ret;
}

/* HIP4 has been initialize, setup with values
 * provided by FW
 */
int slsi_hip_setup(struct slsi_hip *hip)
{
	struct slsi_dev     *sdev = container_of(hip, struct slsi_dev, hip);
	struct scsc_service *service;
	u32 conf_hip4_ver = 0;

	if (!sdev || !sdev->service)
		return -EIO;

	if (atomic_read(&sdev->hip.hip_state) != SLSI_HIP_STATE_STARTED &&
	    atomic_read(&sdev->hip.hip_state) != SLSI_HIP_STATE_BLOCKED)
		return -EIO;

	service = sdev->service;

	/* Get the Version reported by the FW */
	conf_hip4_ver = scsc_wifi_get_hip_config_version(&hip->hip_control->init);
	/* Check if the version is supported. And get the index */
	/* This is hardcoded and may change in future versions */
	if (conf_hip4_ver != 4 && conf_hip4_ver != 5) {
		SLSI_ERR_NODEV("FW HIP config version %d not supported\n", conf_hip4_ver);
		return -EIO;
	}

	if (conf_hip4_ver == 4) {
		hip->hip_priv->unidat_req_headroom =
			scsc_wifi_get_hip_config_u8(&hip->hip_control, unidat_req_headroom, 4);
		hip->hip_priv->unidat_req_tailroom =
			scsc_wifi_get_hip_config_u8(&hip->hip_control, unidat_req_tailroom, 4);
		hip->hip_priv->version = 4;

		hip->hip_priv->napi_rx_full_cnt = 0;
		hip->hip_priv->napi_rx_saturated = 0;
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
		if (!test_and_set_bit(SLSI_HIP_NAPI_STATE_ENABLED, &hip->hip_priv->napi_state))
			napi_enable(&hip->hip_priv->napi);
#endif
	} else {
		/* version 5 */
		hip->hip_priv->unidat_req_headroom =
			scsc_wifi_get_hip_config_u8(&hip->hip_control, unidat_req_headroom, 5);
		hip->hip_priv->unidat_req_tailroom =
			scsc_wifi_get_hip_config_u8(&hip->hip_control, unidat_req_tailroom, 5);
		hip->hip_priv->version = 5;
	}
	/* Unmask interrupts - now host should handle them */
	atomic_set(&hip->hip_priv->stats.irqs, 0);
	atomic_set(&hip->hip_priv->stats.spurious_irqs, 0);
	atomic_set(&sdev->debug_inds, 0);

	atomic_set(&hip->hip_priv->closing, 0);

	scsc_service_mifintrbit_bit_unmask(service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_FH_RFB]);
	scsc_service_mifintrbit_bit_unmask(service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_CTRL]);
	scsc_service_mifintrbit_bit_unmask(service, hip->hip_priv->intr_tohost_mul[HIP4_MIF_Q_TH_DAT]);
#ifdef CONFIG_SCSC_WLAN_HOST_DPD
	scsc_service_mifintrbit_bit_unmask(service, hip->hip_priv->intr_to_host_dpd);
	slsi_wlan_dpd_mmap_user_space_event(SLSI_WLAN_DPD_DRV_MSG_ID_WLAN_ON);
#endif
	SLSI_INFO_NODEV("hip4 setup %d done\n", conf_hip4_ver);
	return 0;
}

/* On suspend hip4 needs to ensure that TH interrupts *are* unmasked */
void slsi_hip_suspend(struct slsi_hip *hip)
{
	struct slsi_dev *sdev;
	struct scsc_service *service;
	u8 i;

	if (!hip || !hip->hip_priv)
		return;

	sdev = container_of(hip, struct slsi_dev, hip);
	if (!sdev || !sdev->service)
		return;

	if (atomic_read(&sdev->hip.hip_state) != SLSI_HIP_STATE_STARTED)
		return;

	memset(&sdev->suspend_tm, 0, sizeof(struct rtc_time));
	if (slsi_hip_wlan_get_rtc_time(&sdev->suspend_tm))
		SLSI_ERR_NODEV("Error reading rtc\n");

	service = sdev->service;

	slsi_log_client_msg(sdev, UDI_DRV_SUSPEND_IND, 0, NULL);
	SCSC_HIP4_SAMPLER_SUSPEND(hip->hip_priv->minor);

	atomic_set(&hip->hip_priv->in_suspend, 1);

	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++)
		if (hip->hip_priv->intr_tohost_mul[i] != MIF_NO_IRQ)
			scsc_service_mifintrbit_bit_unmask(service, hip->hip_priv->intr_tohost_mul[i]);
}

void slsi_hip_resume(struct slsi_hip *hip)
{
	struct slsi_dev *sdev;
	struct scsc_service *service;
	u8 i;

	if (!hip || !hip->hip_priv)
		return;

	sdev = container_of(hip, struct slsi_dev, hip);
	if (!sdev || !sdev->service)
		return;

	if (atomic_read(&sdev->hip.hip_state) != SLSI_HIP_STATE_STARTED)
		return;

	service = sdev->service;

	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++)
		if (hip->hip_priv->intr_tohost_mul[i] != MIF_NO_IRQ)
			scsc_service_mifintrbit_bit_unmask(service, hip->hip_priv->intr_tohost_mul[i]);
	slsi_log_client_msg(sdev, UDI_DRV_RESUME_IND, 0, NULL);
	SCSC_HIP4_SAMPLER_RESUME(hip->hip_priv->minor);
	atomic_set(&hip->hip_priv->in_suspend, 0);
}

void slsi_hip_freeze(struct slsi_hip *hip)
{
	struct slsi_dev *sdev;
	struct scsc_service *service;
	u8 i;

#ifdef CONFIG_SCSC_WLAN_HOST_DPD
	slsi_wlan_dpd_mmap_user_space_event(SLSI_WLAN_DPD_DRV_MSG_ID_WLAN_OFF);
#endif
	if (!hip || !hip->hip_priv)
		return;

	sdev = container_of(hip, struct slsi_dev, hip);
	if (!sdev || !sdev->service)
		return;

	if (atomic_read(&sdev->hip.hip_state) != SLSI_HIP_STATE_STARTED)
		return;

	service = sdev->service;

	closing = ktime_get();
	atomic_set(&hip->hip_priv->closing, 1);

	hip4_dump_dbg(hip, NULL, NULL, service);
	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++)
		if (hip->hip_priv->intr_tohost_mul[i] != MIF_NO_IRQ)
			scsc_service_mifintrbit_bit_mask(service, hip->hip_priv->intr_tohost_mul[i]);

	hip->hip_priv->napi_rx_full_cnt = 0;
	hip->hip_priv->napi_rx_saturated = 0;
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	if (test_and_clear_bit(SLSI_HIP_NAPI_STATE_ENABLED, &hip->hip_priv->napi_state))
		napi_disable(&hip->hip_priv->napi);

	cancel_work_sync(&hip->hip_priv->intr_wq_napi_cpu_switch);
	cancel_work_sync(&hip->hip_priv->intr_wq_ctrl);
	tasklet_kill(&hip->hip_priv->intr_tl_fb);
#endif
#ifdef CONFIG_SCSC_QOS
	cancel_work_sync(&hip->hip_priv->pm_qos_work);
	if (hip->hip_priv->pm_qos_state != SCSC_QOS_DISABLED)
		scsc_service_pm_qos_update_request(sdev->service, SCSC_QOS_DISABLED);
#endif
	atomic_set(&hip->hip_priv->watchdog_timer_active, 0);

	/* Deactive the wd timer prior its expiration */
	del_timer_sync(&hip->hip_priv->watchdog);
	SLSI_INFO_NODEV("hip4 freeze done\n");
}

void slsi_hip_deinit(struct slsi_hip *hip)
{
	struct slsi_dev     *sdev;
	struct scsc_service *service;
	u8 i;

	if (!hip || !hip->hip_priv)
		return;

	sdev = container_of(hip, struct slsi_dev, hip);
	if (!sdev || !sdev->service)
		return;

	service = sdev->service;

#if IS_ENABLED(CONFIG_SCSC_LOGRING)
	slsi_traffic_mon_client_unregister(sdev, hip->hip_priv->logring_client);
	/* Reenable logring in case was disabled */
	scsc_logring_enable(true);
#endif
#ifdef CONFIG_SCSC_QOS
	/* de-register with traffic monitor */
	slsi_traffic_mon_client_unregister(sdev, hip->qos_client);
	scsc_service_pm_qos_remove_request(service);
#endif

#ifdef CONFIG_SCSC_SMAPPER
	/* Init SMAPPER */
	if (hip4_smapper_is_enabled) {
		hip4_smapper_is_enabled = false;
		hip4_smapper_deinit(sdev, hip);
	}
#endif

	closing = ktime_get();
	atomic_set(&hip->hip_priv->closing, 1);

	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++)
		if (hip->hip_priv->intr_tohost_mul[i] != MIF_NO_IRQ)
			scsc_service_mifintrbit_bit_mask(service, hip->hip_priv->intr_tohost_mul[i]);

	hip->hip_priv->napi_rx_full_cnt = 0;
	hip->hip_priv->napi_rx_saturated = 0;
#ifndef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	if (test_and_clear_bit(SLSI_HIP_NAPI_STATE_ENABLED, &hip->hip_priv->napi_state))
		napi_disable(&hip->hip_priv->napi);
	cancel_work_sync(&hip->hip_priv->intr_wq_napi_cpu_switch);
	cancel_work_sync(&hip->hip_priv->intr_wq_ctrl);
	tasklet_kill(&hip->hip_priv->intr_tl_fb);

	netif_napi_del(&hip->hip_priv->napi);
#else
	slsi_lbm_unregister_bh(hip->hip_priv->bh_dat);
	slsi_lbm_unregister_bh(hip->hip_priv->bh_ctl);
	slsi_lbm_unregister_bh(hip->hip_priv->bh_rfb);
	hip->hip_priv->bh_dat = NULL;
	hip->hip_priv->bh_ctl = NULL;
	hip->hip_priv->bh_rfb = NULL;
#ifdef CONFIG_SLSI_WLAN_LPC
	slsi_lbm_unregister_bh(hip->hip_priv->bh_lpc);
	hip->hip_priv->bh_lpc = NULL;
#endif
#endif
	for (i = 0; i < MIF_HIP_CFG_Q_NUM; i++)
		if (hip->hip_priv->intr_tohost_mul[i] != MIF_NO_IRQ)
			scsc_service_mifintrbit_unregister_tohost(service, hip->hip_priv->intr_tohost_mul[i], SCSC_MIFINTR_TARGET_WLAN);
#ifdef CONFIG_SCSC_QOS
	cancel_work_sync(&hip->hip_priv->pm_qos_work);
#endif
	flush_workqueue(hip->hip_priv->hip4_workq);
	destroy_workqueue(hip->hip_priv->hip4_workq);
	scsc_service_mifintrbit_free_fromhost(service, hip->hip_priv->intr_fromhost, SCSC_MIFINTR_TARGET_WLAN);

#ifdef CONFIG_SCSC_WLAN_HOST_DPD
	scsc_service_mifintrbit_bit_mask(service, hip->hip_priv->intr_to_host_dpd);
	scsc_service_mifintrbit_unregister_tohost(service, hip->hip_priv->intr_to_host_dpd, SCSC_MIFINTR_TARGET_WLAN);
	scsc_service_mifintrbit_free_fromhost(service, hip->hip_priv->intr_from_host_dpd, SCSC_MIFINTR_TARGET_WLAN);
	slsi_wlan_dpd_mmap_set_buffer(NULL, NULL, 0);
	slsi_wlan_dpd_mmap_user_space_event(SLSI_WLAN_DPD_DRV_MSG_ID_WLAN_OFF);
#endif

	slsi_wake_lock_destroy(&hip->hip_priv->hip4_wake_lock_tx);
	slsi_wake_lock_destroy(&hip->hip_priv->hip4_wake_lock_ctrl);
	slsi_wake_lock_destroy(&hip->hip_priv->hip4_wake_lock_data);

	/* If we get to that point with rx_lock/tx_lock claimed, trigger BUG() */
	WLBT_WARN_ON(atomic_read(&hip->hip_priv->in_tx));
	WLBT_WARN_ON(atomic_read(&hip->hip_priv->in_rx));

	atomic_set(&hip->hip_priv->watchdog_timer_active, 0);
	/* Deactive the wd timer prior its expiration */
	del_timer_sync(&hip->hip_priv->watchdog);

#ifdef CONFIG_SCSC_WLAN_DEBUG
	if (hip->hip_priv->stats.procfs_dir) {
		remove_proc_entry("driver/hip4/jitter", NULL);
		remove_proc_entry("driver/hip4/info", NULL);
		remove_proc_entry("driver/hip4/history", NULL);
		remove_proc_entry("driver/hip4", NULL);
	}
#endif

	spin_lock_bh(&in_napi_context);
	kfree(hip->hip_priv);

	hip->hip_priv = NULL;
	spin_unlock_bh(&in_napi_context);

	/* remove the pools */
	mbulk_pool_remove(MBULK_POOL_ID_DATA);
	mbulk_pool_remove(MBULK_POOL_ID_CTRL);

	SLSI_INFO_NODEV("hip4 deinit done\n");
}
