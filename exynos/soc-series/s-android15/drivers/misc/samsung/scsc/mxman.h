/****************************************************************************
 *
 * Copyright (c) 2014 - 2022 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef _MAXWELL_MANAGER_H
#define _MAXWELL_MANAGER_H
#include <linux/workqueue.h>
#include "fwhdr.h"
#include "mxmgmt_transport.h"
#include <linux/version.h>
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
#include <linux/kfifo.h>
#endif
#include "mxproc.h"
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#include "mxman_res.h"
#include "mxman_if.h"
#endif
#if IS_ENABLED(CONFIG_EXYNOS_SYSTEM_EVENT)
#include "mxman_sysevent.h"
#endif
#include <scsc/scsc_mx.h>
#ifdef CONFIG_SCSC_COMMON_ANDROID
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <scsc/scsc_wakelock.h>
#else
#include <linux/wakelock.h>
#endif
#endif
struct mxman;

void mxman_init(struct mxman *mxman, struct scsc_mx *mx);
void mxman_deinit(struct mxman *mxman);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
int mxman_open(struct mxman *mxman, enum scsc_subsystem sub, void *data, size_t data_sz);
void mxman_close(struct mxman *mxman, enum scsc_subsystem sub);
#else
int mxman_open(struct mxman *mxman);
void mxman_close(struct mxman *mxman);
#endif
#if IS_ENABLED(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
void mxman_fail(struct mxman *mxman, u16 scsc_panic_code, const char *resason, enum scsc_subsystem id);
int mxman_force_panic(struct mxman *mxman, enum scsc_subsystem id);
#else
void mxman_fail(struct mxman *mxman, u16 scsc_panic_code, const char *reason);
int mxman_force_panic(struct mxman *mxman);
#endif
void mxman_syserr(struct mxman *mxman, struct mx_syserr_decode *syserr);
void mxman_freeze(struct mxman *mxman);
int mxman_suspend(struct mxman *mxman);
void mxman_resume(struct mxman *mxman);
void mxman_show_last_panic(struct mxman *mxman);
void mxman_subserv_recovery(struct mxman *mxman, struct mx_syserr_decode *syserr_decode);
bool mxman_is_failed(void);
bool mxman_is_frozen(void);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
int mxman_get_state(struct mxman *mxman);
u64 mxman_get_last_panic_time(struct mxman *mxman);
u32 mxman_get_panic_code(struct mxman *mxman);
u32 *mxman_get_last_panic_rec(struct mxman *mxman);
u16 mxman_get_last_panic_rec_sz(struct mxman *mxman);
void mxman_reinit_completion(struct mxman *mxman);
int mxman_wait_for_completion_timeout(struct mxman *mxman, u32 ms);
void mxman_set_syserr_recovery_in_progress(struct mxman *mxman, bool value);
void mxman_set_last_syserr_recovery_time(struct mxman *mxman, enum scsc_subsystem sub, unsigned long value);
bool mxman_get_syserr_recovery_in_progress(struct mxman *mxman);
u16 mxman_get_last_syserr_subsys(struct mxman *mxman);

#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
void mxman_recover_subsystem(struct mxman *mxman, u16 failure_source, const char *reason, enum scsc_subsystem sub);
void mxman_indep_recovery_err_completion(struct mxman *mxman);
int mxman_check_start_failure_during_indep_recovery(struct mxman *mxman);
bool mxman_warm_reset_in_progress(void);
bool mxman_check_recovery_state_none(void);
bool mxman_check_recovery_state_error_in_progress(void);
int mxman_set_recovery_mode(struct mxman *mxman, bool enable_indep);
#endif
bool mxman_subsys_in_failed_state(struct mxman *mxman, enum scsc_subsystem sub);
bool mxman_get_panic_in_progress(struct mxman *mxman);
bool mxman_subsys_active(struct mxman *mxman, enum scsc_subsystem sub);
bool mxman_users_active(struct mxman *mxman);
#if IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
void mxman_scan_dump_mode(void);
#endif
#endif

void mxman_control_suspend_gpio(struct mxman *mxman, u8 value);

#if IS_ENABLED(CONFIG_SCSC_FM)
void mxman_fm_on_halt_ldos_on(void);
void mxman_fm_on_halt_ldos_off(void);
int mxman_fm_set_params(struct wlbt_fm_params *params);
#endif
int mxman_lerna_send(struct mxman *mxman, void *data, u32 size);

#ifdef CONFIG_HDM_WLBT_SUPPORT
int mxman_get_hdm_wlan_support(void);
int mxman_get_hdm_bt_support(void);
#endif

#if !defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
enum mxman_state {
	MXMAN_STATE_STOPPED,
	MXMAN_STATE_STARTING,
	MXMAN_STATE_STARTED,
	MXMAN_STATE_FAILED,
	MXMAN_STATE_FROZEN,
};
#endif

#define SCSC_FAILURE_REASON_LEN 256

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#define MAX_NUM_SUBSYS		(2)
#define SYSERR_SUBSYS_RESET_WPAN (SYSERR_SUBSYS_BT - 1)		// 0
#define SYSERR_SUBSYS_RESET_WLAN (SYSERR_SUBSYS_WLAN - 1)	// 1
#endif

#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
enum recovery_state {
	RCVRY_STATE_NONE = 0,
	RCVRY_STATE_CHIP_RESET,
	RCVRY_STATE_DUMP_LOGGING,
	RCVRY_STATE_SUBSYSTEM_RESET,
	RCVRY_STATE_WAITING_START,
	RCVRY_STATE_ERROR_IN_PROGRESS,

	RCVRY_STATE_MAX,
};

enum recovery_event {
	RCVRY_EVT_ERROR = 0,

	RCVRY_EVT_RECOVER_WLAN,
	RCVRY_EVT_RECOVER_WPAN,
	RCVRY_EVT_RECOVER_HOST,
	RCVRY_EVT_RECOVER_REJECT,

	RCVRY_EVT_DUMP_END,

	RCVRY_EVT_RESET_DONE,
	RCVRY_EVT_RESET_ERR,
	RCVRY_EVT_RESET_TIMEOUT,

	RCVRY_EVT_START_SUCCESS,
	RCVRY_EVT_START_FAILURE,

	RCVRY_EVT_MAX
};

enum indep_sub_state {
	NO_INDEP = 0x00,

	WLAN_INDEP_NORMAL = 0x10,
	WLAN_INDEP_ERR,
	WLAN_INDEP_ERR_START_FAIL,

	WPAN_INDEP_NORMAL = 0x20,
	WPAN_INDEP_ERR,
	WPAN_INDEP_ERR_START_FAIL,
};

struct rcvry_fsm_thread {
	spinlock_t 			kfifo_lock;
	struct task_struct 	*task;
	wait_queue_head_t 	evt_wait_q;
	struct completion	reset_work_completion;
	struct completion 	reset_completion;
	struct completion 	delay_setting_completion;
	struct completion	indep_err_work_comletion;
	struct kfifo 		evt_queue;
	struct mutex		thread_lock;

	bool				pmu_monitor_mode;
	enum scsc_subsystem target_sub;
	enum indep_sub_state	indep_substate;

	enum scsc_subsystem last_panic_sub;
	u16 				last_panic_code;
	u8      			last_panic_level;
	char				last_failure_reason[SCSC_FAILURE_REASON_LEN];
};
#endif

struct mxman {
	struct scsc_mx          *mx;
	int                     users;
	void                    *start_dram;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	size_t                  size_dram;
	int			users_wpan;
#endif
	struct workqueue_struct *fw_crc_wq;
	struct delayed_work     fw_crc_work;
	struct workqueue_struct *failure_wq; /* For recovery from chip restart */
	struct work_struct      failure_work; /* For recovery from chip restart */
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	struct work_struct      failure_work_wlan; /* For single subsystem recovery */
	struct work_struct      failure_work_wpan; /* For single subsystem recovery */
#endif
	struct workqueue_struct *syserr_recovery_wq; /* For recovery from syserr sub-system restart */
	struct work_struct      syserr_recovery_work; /* For recovery from syserr sub-system restart */
	char                    *fw;
	u32                     fw_image_size;
	struct completion       mm_msg_start_ind_completion;
	struct completion       mm_msg_halt_rsp_completion;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	struct fwhdr_if 	*fw_wlan;
	struct fwhdr_if 	*fw_wpan;
#else
	struct fwhdr		fwhdr;
#endif
	struct mxconf           *mxconf;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	struct mxconf           *mxconf_wpan;
	void                    *data_mxconf;
	void                    *data_mxconf_wpan;
	bool			wpan_present;
#endif
	enum mxman_state        mxman_state;
	enum mxman_state        mxman_next_state;
	struct mutex            mxman_mutex;
	/* Syserr sub-sytem and full chip restart co-ordination */
	struct mutex            mxman_recovery_mutex;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM) && !defined(CONFIG_WLBT_SPLIT_RECOVERY)
	bool                    panic_in_progress;
#endif
	struct mxproc           mxproc;
	int			suspended;
	atomic_t		suspend_count;
	atomic_t                recovery_count;
	atomic_t		boot_count;
	atomic_t		boot_count_wlan;
	atomic_t		boot_count_wpan;
	atomic_t		cancel_resume;
	bool			check_crc;
	char                    fw_build_id[FW_BUILD_ID_SZ]; /* Defined in SC-505846-SW */
	struct completion       recovery_completion;
#ifdef CONFIG_SCSC_COMMON_ANDROID
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct scsc_wake_lock	failure_recovery_wake_lock; /* For recovery from chip restart */
	struct scsc_wake_lock	syserr_recovery_wake_lock; /* For recovery from syserr sub-system restart */
#else
	struct wake_lock	failure_recovery_wake_lock; /* For recovery from chip restart */
	struct wake_lock	syserr_recovery_wake_lock; /* For recovery from syserr sub-system restart */
#endif
#endif
	u32			rf_hw_ver;
	u16			scsc_panic_code;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	u16			scsc_panic_code_wpan;
	enum scsc_subsystem 	scsc_panic_sub;
#endif
	u64			last_panic_time;
	u32			last_panic_rec_r[PANIC_RECORD_SIZE]; /* Must be at least SCSC_R4_V2_MINOR_54 */
	u16			last_panic_rec_sz;
	u32			last_panic_stack_rec_r[PANIC_STACK_RECORD_SIZE]; /* Must be at least SCSC_R4_V2_MINOR_54 */
	u16			last_panic_stack_rec_sz;
	struct mx_syserr_decode	last_syserr;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	unsigned long		last_syserr_recovery_time[MAX_NUM_SUBSYS]; /* In jiffies */
#else
	unsigned long		last_syserr_recovery_time; /* In jiffies */
#endif
	unsigned long		last_syserr_level7_recovery_time; /* In jiffies */
	bool			notify;
	bool			syserr_recovery_in_progress;
#if IS_ENABLED(CONFIG_SCSC_FM)
	u32			on_halt_ldos_on;
#endif
	char			failure_reason[SCSC_FAILURE_REASON_LEN]; /* previous failure reason */
	struct wlbt_fm_params	fm_params;		/* FM freq info */
	int			fm_params_pending;	/* FM freq info waiting to be delivered to FW */

	char                    fw_ttid[FW_TTID_SZ]; /* Defined in SC-505846-SW */

#if IS_ENABLED(CONFIG_EXYNOS_SYSTEM_EVENT)
	/* System Event */
	struct sysevent_desc sysevent_desc;
	struct sysevent_device *sysevent_dev;
	struct notifier_block sysevent_nb;
#endif

#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
	struct rcvry_fsm_thread	rcvry_thread;
	struct workqueue_struct *reset_subsystem_wq;
	struct work_struct      reset_subsystem_work;

	struct workqueue_struct *delay_recovery_setting_wq;
	struct work_struct      delay_recovery_setting_work;

	struct workqueue_struct *indep_recovery_err_wq;
	struct work_struct      indep_recovery_err_work;
#endif

#if defined(CONFIG_SCSC_XO_CDAC_CON)
	bool is_dcxo_set;
#endif
};

#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
int mxman_send_rcvry_evt_to_fsm(struct mxman *mxman, enum recovery_event event);
#endif
void mxman_register_gdb_channel(struct scsc_mx *mx, mxmgmt_channel_handler handler, void *data);
void mxman_send_gdb_channel(struct scsc_mx *mx, void *data, size_t length);

#ifdef CONFIG_SCSC_CHV_SUPPORT
#define SCSC_CHV_ARGV_ADDR_OFFSET 0x200008
extern int chv_run;
#endif

#define SCSC_SYSERR_HOST_SERVICE_SHIFT 4

#endif
