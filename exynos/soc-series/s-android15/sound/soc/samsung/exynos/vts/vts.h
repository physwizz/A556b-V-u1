/* SPDX-License-Identifier: GPL-2.0-or-later
 * sound/soc/samsung/vts/vts.h
 *
 * ALSA SoC - Samsung VTS driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_VTS_H
#define __SND_SOC_VTS_H

#include <linux/clk.h>
#include <sound/memalloc.h>
#include <soc/samsung/exynos/memlogger.h>
#include <soc/samsung/exynos/sysevent.h>
#include <sound/samsung/vts.h>

#include "vts_soc.h"
#include "vts_shared_info.h"

#if IS_ENABLED(CONFIG_EXYNOS_S2MPU)
#include <soc/samsung/exynos/exynos-s2mpu.h>
#endif
#define VTS_S2MPU_NAME	"VTS"

#define TEST_WAKEUP

#define VTS_TAG_IPC	("IPC")

#define VTS_IRQ_LIMIT			(32)

#define SOUND_MODEL_SIZE_MAX (SZ_32K)
#define SOUND_MODEL_COUNT (3)
#define VTS_MICCONF_FOR_RECORD (4)

/* VTS Model Binary Max buffer sizes */
#define VTS_MODEL_BIN_MAXSZ     (0x1D4C0)
#define CLK_MAX_CNT (20)
#define RECOVERY_MAX_CNT	(5)

enum ipc_state {
	IDLE,
	SEND_MSG,
	SEND_MSG_OK,
	SEND_MSG_FAIL,
};

/* poll_event_type
	triggered: event_type = EVENT_TRIGGERED + triggered_id
	stop polling: event_type = EVENT_STOP_POLLING
	error recoverty: event_type = EVENT_ERROR_RECOVERY
	retart: event_type = EVENT_RESTART
	EVENT_READY should be add(bit operation) whenever event occurs
*/
enum poll_event_type {
	EVENT_NONE              = 0,
	EVENT_TRIGGERED         = 0x10,
	EVENT_STOP_POLLING      = 0x20,
	EVENT_ERROR_RECOVERY    = 0x40,
	EVENT_RESTART		= 0x80,
	EVENT_READY             = 0x100,
	EVENT_BARGEIN             = 0x200,
};

enum vts_platform_type {
	PLATFORM_VTS_NONE = 0, /* skipping id */
	PLATFORM_VTS_NORMAL_RECORD,
	PLATFORM_VTS_TRIGGER_RECORD,
	PLATFORM_VTS_INTERNAL_RECORD,
	PLATFORM_VTS_NS_L_RECORD,
	PLATFORM_VTS_NS_R_RECORD,
};

enum executionmode {
	VTS_RECOGNIZE_STOP		= 0,
	VTS_RECOGNIZE_START		= 1,
};

enum vts_dump_type {
	KERNEL_PANIC_DUMP,
	VTS_FW_NOT_READY,
	VTS_IPC_TRANS_FAIL,
	VTS_FW_ERROR,
	VTS_ITMON_ERROR,
	RUNTIME_SUSPEND_DUMP,
	VTS_DUMP_LAST,
};

enum vts_command {
	VTS_START_SLIFDUMP	  = 0x00000010,
	VTS_STOP_SLIFDUMP	  = 0x00000020,
	VTS_KERNEL_TIME		  = 0x00000040,
	VTS_FORCE_TRIGGER	  = 0x00000080,
	VTS_AP_SUSPEND		  = 0x00000100,
	VTS_AP_RESUME		  = 0x00000200,
	VTS_SUSPEND_FORCE_TRIGGER = 0x00000400,
	VTS_MIC_INPUT_CH	  = 0x00000800,
	VTS_FORCE_ERROR		  = 0x00001000,
	VTS_SOL_STATUS		  = 0x00002000,
	VTS_DISABLE_LOGDUMP	  = 0x01000000,
	VTS_ENABLE_LOGDUMP	  = 0x02000000,
	VTS_DISABLE_AUDIODUMP	  = 0x04000000,
	VTS_ENABLE_AUDIODUMP	  = 0x08000000,
	VTS_DISABLE_DEBUGLOG	  = 0x10000000,
	VTS_ENABLE_DEBUGLOG	  = 0x20000000,
	VTS_ENABLE_SRAM_LOG	  = 0x80000000,
};

enum supported_mic_type {
	MIC_NUM1 = 1,
	MIC_NUM2,
};

struct vts_ipc_msg {
	int msg;
	u32 values[3];
};

enum vts_state_machine {
	VTS_STATE_NONE			= 0,	/* runtime_suspended state */
	VTS_STATE_VOICECALL		= 1,	/* sram L2Cache call state */
	VTS_STATE_RUNTIME_RESUMING	= 2,	/* runtime_resume started */
	VTS_STATE_RUNTIME_RESUMED	= 3,	/* runtime_resume done */
	VTS_STATE_RECOG_STARTED		= 4,	/* Recognization started */
	VTS_STATE_RECOG_TRIGGERED	= 5,	/* Recognize triggered */
	VTS_STATE_SEAMLESS_REC_STARTED	= 6,	/* seamless record started */
	VTS_STATE_SEAMLESS_REC_STOPPED	= 7,	/* seamless record stopped */
	VTS_STATE_RECOG_STOPPED		= 8,	/* Recognization stopped */
	VTS_STATE_RUNTIME_SUSPENDING	= 9,	/* runtime_suspend started */
	VTS_STATE_RUNTIME_SUSPENDED	= 10,	/* runtime_suspend done */
};

enum vts_rec_state_machine {
	VTS_REC_STATE_STOP  = 0,
	VTS_REC_STATE_START = 1,
};

enum vts_tri_state_machine {
	VTS_TRI_STATE_COPY_STOP  = 0,
	VTS_TRI_STATE_COPY_START = 1,
};

enum vts_status {
	NORMAL   = 0,
	ABNORMAL = 1,
};

enum vts_dma_id {
	VTS_TRIGGRE    = 0,
	VTS_RECORD     = 1,
	VTS_INTERNAL   = 2,
	VTS_NS_L       = 3,
	VTS_NS_R       = 4,

	/* should define before this */
	VTS_DMA_CNT,
};

/* These values are defined in the VTS DMIC_IF SFR Description,
   and when using these values, you must check the document to see
   if the values are correct. */
enum vts_dmic_if_sys_sel {
	DMIC_IF_SYS_SEL_512KHz = 0,
	DMIC_IF_SYS_SEL_768KHz = 1,
	DMIC_IF_SYS_SEL_384KHz = 2,
	DMIC_IF_SYS_SEL_2_048MHz = 3,
	DMIC_IF_SYS_SEL_3_072MHz = 4,
	DMIC_IF_SYS_SEL_4_608MHz = 6,
};

#define DMIC_IF_SYS_SEL_VTS DMIC_IF_SYS_SEL_768KHz
#define DMIC_IF_SYS_SEL_AUD DMIC_IF_SYS_SEL_3_072MHz

struct vts_model_bin_info {
	size_t actual_sz;
	size_t max_sz;
	int offset;
};

struct vts_dbg_fw_dump {
	unsigned long long sram_magic;
	char sram_dump[VTS_SRAM_SZ];
	unsigned long long pcm_magic;
	char pcm_dump[VTS_PCM_BUF_SZ];
};

struct vts_dbg_dump {
	long long time;
	enum vts_dump_type dbg_type;
	unsigned int gpr[25];
	char *sram_log;
	struct vts_dbg_fw_dump *fw_dump;
};

struct vtsfw_stored_data {
	int num_data[10];
};

struct vts_dram_request {
	unsigned int id;
	bool on;
	unsigned long long updated;
};

struct clk_src {
	struct clk *clk_src;
	bool enabled;
};

struct clk_path {
	int index;
	int value;
};

struct vts_data {
	struct platform_device *pdev;
	struct snd_soc_component *cmpnt;
	void __iomem *sfr_base;
	void __iomem *baaw_base;
	void __iomem *sram_base;
	void __iomem *dmic_if0_base;
	void __iomem *dmic_if1_base;
	void __iomem *dmic_ahb0_base;
	void __iomem *timer0_base;
	void __iomem *gpr_base;

	void __iomem *intmem_code;
	void __iomem *intmem_data;
	void __iomem *intmem_pcm;
	void __iomem *intmem_data1;
	void __iomem *sicd_base;
	void __iomem *sfr_slif_vts;
	void __iomem *cmu_vts;

	size_t sram_size;
	phys_addr_t sram_phys;
	struct regmap *regmap_dmic;
	struct clk_src clk_src_list[CLK_MAX_CNT];
	struct clk_path init_clk_path[CLK_MAX_CNT];
	struct clk_path alive_clk_path[CLK_MAX_CNT];
	struct clk_path sys_clk_path[CLK_MAX_CNT];
	struct clk_path tri_clk_path[CLK_MAX_CNT];
	struct clk_path aud_clk_path[CLK_MAX_CNT];
	char clk_name_list[CLK_MAX_CNT][50];
	struct pinctrl *pinctrl;
	struct mutex mutex_pin;
	unsigned int vtsfw_version;
	unsigned int vtsdetectlib_version;
	const struct firmware *firmware;
	unsigned int vtsdma_count;
	unsigned long syssel_rate;
	unsigned int target_sysclk;
	unsigned long sysclk_rate;
	struct platform_device *pdev_mailbox;
	struct platform_device *pdev_vtsdma[VTS_DMA_CNT];
	int irq[VTS_IRQ_LIMIT];
	enum ipc_state ipc_state_ap;
	wait_queue_head_t ipc_wait_queue;
	wait_queue_head_t poll_wait_queue;
	spinlock_t ipc_spinlock;
	struct mutex ipc_mutex;
	u32 dma_area_vts;
	struct snd_dma_buffer dmab;
	struct snd_dma_buffer dmab_rec;
	struct snd_dma_buffer dmab_internal;
	struct snd_dma_buffer dmab_ns_l;
	struct snd_dma_buffer dmab_ns_r;
	struct snd_dma_buffer dmab_log;
	struct snd_dma_buffer dmab_model;
	u32 target_size;
	int target_id;
	u32 current_active_ids;
	enum executionmode exec_mode;
	bool vts_ready;
	unsigned long sram_acquired;
	bool enabled;
	bool running;
	bool voicecall_enabled;
	struct snd_soc_card *card;
	int micclk_init_cnt;
	unsigned int mic_ready;
	enum vts_dmic_sel dmic_if;
	bool irq_state;
	u32 lpsdgain;
	u32 dmic_gain[2];
	u32 dmic_1db_gain[2];
	u32 amicgain;
	char *sramlog_baseaddr;
	u32 running_ipc;
	struct wakeup_source *wake_lock;
	unsigned int vts_state;
	unsigned int vts_tri_state;
	unsigned int vts_rec_state;
	u32 fw_logfile_enabled;
	u32 fw_logger_enabled;
	struct vts_model_bin_info sm_info;
	spinlock_t state_spinlock;
	struct notifier_block pm_nb;
	struct notifier_block itmon_nb;
	struct notifier_block bargein_nb;
	u32 kw_length;
	struct vts_dbg_dump p_dump;
	bool slif_dump_enabled;
	enum vts_clk_src clk_path;
	struct memlog *log_desc;
	struct memlog_obj *kernel_log_file_obj;
	struct memlog_obj *kernel_log_obj;
	struct memlog_obj *dump_file_obj;
	struct memlog_obj *dump_obj;
	struct memlog_obj *fw_log_file_obj;
	struct memlog_obj *fw_log_obj;
	struct sysevent_desc sysevent_desc;
	struct sysevent_device *sysevent_dev;
	struct vts_shared_info *shared_info;
	struct vts_dram_request dram_requests[16];
	u32 poll_event_type;
	char *sm_data;
	bool sm_loaded;
	uint32_t supported_mic_num;
	uint32_t sysclk_div;
	uint32_t mic_input_ch;
	/* info got from shared_info */
	uint32_t pcm_buf_offset;
	int google_version;
	char google_uuid[40];
	uint32_t vts_status;
	uint32_t silent_reset_support;
	uint32_t recovery_try_cnt;
	struct vtsfw_stored_data stored_data;
	struct mutex firmware_lock;
	struct mutex firmware_boot_lock;
#if IS_ENABLED(CONFIG_EXYNOS_S2MPU)
	struct s2mpu_notifier_block s2mpu_nb;
#endif
	uint32_t is_bargein;
	uint32_t bargein_gain;
	uint32_t sramlog_offset;
	uint32_t wfi_addr;
};

struct vts_dma_data {
	unsigned int id;
	struct platform_device *pdev_vts;
	struct device *dev;
	struct vts_data *vts_data;
	struct snd_pcm_substream *substream;
	enum vts_platform_type type;
	unsigned int pointer;
};

extern int vts_start_ipc_transaction(struct device *dev,
	struct vts_data *data, int msg, u32 (*values)[3], int atomic, int sync);
extern int vts_send_ipc_ack(struct vts_data *data, u32 result);
extern void vts_register_dma(struct platform_device *pdev_vts,
		struct platform_device *pdev_vts_dma, unsigned int id);
extern int vts_set_dmicctrl(struct platform_device *pdev,
	int micconf_type, bool enable);
extern int vts_sound_machine_drv_register(void);
extern bool is_vts(struct device *dev);
extern int vts_start_runtime_resume(struct device *dev, int skip_log);
extern struct vts_data *vts_get_data(struct device *dev);
extern struct vts_data *vts_get_vts_data(void);
extern int cmu_vts_rco_400_control(bool on);

extern void vts_intr_start_dump(struct device *dev, bool en);
extern bool vts_is_running(void);
#if defined(CONFIG_VTS_FG_MEMP_CTRL)
extern void vts_set_fg_mem_ctrl(struct platform_device *pdev, bool enable);
#endif
#endif /* __SND_SOC_VTS_H */
