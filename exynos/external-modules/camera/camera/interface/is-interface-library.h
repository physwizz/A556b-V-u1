/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_API_COMMON_H
#define IS_API_COMMON_H

#include <linux/vmalloc.h>
#include <linux/of_reserved_mem.h>

#include "is-core.h"
#include "pablo-debug.h"
#include "pablo-binary.h"
#include "is-type.h"
#include "is-hw-config.h"

#if !IS_ENABLED(SKIP_LIB_LOAD)
/* #define LIB_MEM_TRACK */
#endif

#if defined(CONFIG_STACKTRACE) && defined(LIB_MEM_TRACK)
#define LIB_MEM_TRACK_CALLSTACK
#endif

#define LIB_ISP_OFFSET		(0x00000080)
#define LIB_ISP_CODE_SIZE	(0x00340000 + IS_RCHECKER_SIZE_RO)

#define LIB_VRA_OFFSET		(0x00000400)
#define LIB_VRA_CODE_SIZE	(0x00080000)

#define LIB_RTA_OFFSET		(0x00000000)
#define LIB_RTA_CODE_SIZE	(0x00200000)

#define LIB_MAX_TASK		(IS_MAX_TASK)

#define TASK_TYPE_DDK	(1)
#define TASK_TYPE_RTA	(2)
#define TASK_TYPE_EXT	(3)

/* depends on FIMC-IS version */
enum task_priority_magic {
	TASK_PRIORITY_BASE = 10,
	TASK_PRIORITY_1ST,	/* highest */
	TASK_PRIORITY_2ND,
	TASK_PRIORITY_3RD,
	TASK_PRIORITY_4TH,
	TASK_PRIORITY_5TH,	/* lowest */
	TASK_PRIORITY_6TH,	/* for RTA */
	TASK_PRIORITY_7TH,	/* for EXT1 */
	TASK_PRIORITY_8TH,	/* for EXT2 */
	TASK_PRIORITY_9TH,	/* for EXT3 */
	TASK_PRIORITY_10TH,	/* for EXT4 */
	TASK_PRIORITY_11TH,	/* for EXT5 */
	TASK_PRIORITY_12TH	/* for EXT6 */
};

/* Task index */
enum task_index {
	TASK_OTF = 0,
	TASK_AF,
	TASK_ISP_DMA,
	TASK_3AA_DMA,
	TASK_AA,
	TASK_RTA,
	TASK_EXT1,
	TASK_EXT2,
	TASK_EXT3,
	TASK_EXT4,
	TASK_EXT5,
	TASK_EXT6,
	TASK_INDEX_MAX
};
#define TASK_VRA		(TASK_INDEX_MAX)

/* Task affinity */
#define TASK_OTF_AFFINITY		(3)
#define TASK_AF_AFFINITY		(1)
#define TASK_ISP_DMA_AFFINITY		(2)
#define TASK_3AA_DMA_AFFINITY		(TASK_ISP_DMA_AFFINITY)
#define TASK_AA_AFFINITY		(TASK_AF_AFFINITY)
/* #define TASK_RTA_AFFINITY		(1) */ /* There is no need to set of cpu affinity for RTA task */
#define TASK_VRA_AFFINITY		(2)

#define CAMERA_BINARY_VRA_DATA_OFFSET   0x080000
#define CAMERA_BINARY_VRA_DATA_SIZE     0x040000
#ifdef USE_ONE_BINARY
#define CAMERA_BINARY_DDK_CODE_OFFSET   VRA_LIB_SIZE
#define CAMERA_BINARY_DDK_DATA_OFFSET   (VRA_LIB_SIZE + LIB_ISP_CODE_SIZE)
#else
#define CAMERA_BINARY_DDK_CODE_OFFSET   0x000000
#define CAMERA_BINARY_DDK_DATA_OFFSET   LIB_ISP_CODE_SIZE
#endif
#define CAMERA_BINARY_RTA_DATA_OFFSET   LIB_RTA_CODE_SIZE

enum BinLoadType{
    BINARY_LOAD_ALL,
    BINARY_LOAD_DATA
};

#define BINARY_LOAD_DDK_DONE		0x01
#define BINARY_LOAD_RTA_DONE		0x02
#define BINARY_LOAD_ALL_DONE	(BINARY_LOAD_DDK_DONE | BINARY_LOAD_RTA_DONE)

typedef u32 (*task_func)(void *params);

#if IS_ENABLED(DYNAMIC_HEAP_FOR_DDK_RTA)
typedef u32 (*start_up_func_t)(void **func, void *args);
#else
typedef u32 (*start_up_func_t)(void **func);
#endif
typedef u32 (*rta_start_up_func_t)(void *bootargs, void **func);
typedef void(*os_system_func_t)(void);

#define DDK_SHUT_DOWN_FUNC_ADDR	(DDK_LIB_ADDR + 0x100)
typedef int (*ddk_shut_down_func_t)(void *data);
#define RTA_SHUT_DOWN_FUNC_ADDR	(RTA_LIB_ADDR + 0x100)
typedef int (*rta_shut_down_func_t)(void *data);

#define DDK_GET_ENV_ADDR	(DDK_LIB_ADDR + 0x140)
typedef int (*ddk_get_env_func_t)(void *args);
#define RTA_GET_ENV_ADDR	(RTA_LIB_ADDR + 0x140)
typedef int (*rta_get_env_func_t)(void *args);

#define SET_FRAME_INFO_FUNC_ADDR       (DDK_LIB_ADDR + 0x180)
typedef int (*set_frame_info_func_t)(int instance, int frame_num, void *data);

#define TRIGGER_IMAGE_RTA_FUNC_ADDR    (DDK_LIB_ADDR + 0x1C0)
typedef int (*trigger_image_rta_func_t)(int instance, int frame_num, void *data);

/* DEVICE SENSOR INTERFACE */
#define SENSOR_REGISTER_FUNC_ADDR	(DDK_LIB_ADDR + 0x40)
#define SENSOR_REGISTER_FUNC_ADDR_RTA	(RTA_LIB_ADDR + 0x40)
typedef int (*register_sensor_interface)(void *itf);

struct is_task_work {
	struct kthread_work		work;
	task_func			func;
	void				*params;
};

struct is_lib_task {
	struct kthread_worker		worker;
	struct task_struct		*task;
	spinlock_t			work_lock;
	u32				work_index;
	struct is_task_work		work[LIB_MAX_TASK];
#ifdef VH_FPSIMD_API
	struct is_kernel_fpsimd_state	fp_state[LIB_MAX_TASK];
#endif
};

enum memory_track_type {
	/* each O/S facility */
	MT_TYPE_SEMA	= 0x01,
	MT_TYPE_MUTEX,
	MT_TYPE_TIMER,
	MT_TYPE_SPINLOCK,

	/* memory block */
	MT_TYPE_MB_HEAP	= 0x10,
	MT_TYPE_MB_DMA_TAAISP,
	MT_TYPE_MB_DMA_MEDRC,
	MT_TYPE_MB_DMA_TNR,
	MT_TYPE_MB_DMA_TNR_S,
	MT_TYPE_MB_DMA_ORBMCH,
	MT_TYPE_MB_DMA_CLAHE,

	MT_TYPE_MB_VRA	= 0x20,
	MT_TYPE_MB_VRA_NETARR = 0x21,

	/* etc */
	MT_TYPE_DMA	= 0x30,
};

#ifdef LIB_MEM_TRACK
#define MT_STATUS_ALLOC	0x1
#define MT_STATUS_FREE	0x2

#define MEM_TRACK_COUNT	5000
#define MEM_TRACK_ADDRS_COUNT 16

struct _lib_mem_track {
	ulong			lr;
#ifdef LIB_MEM_TRACK_CALLSTACK
	ulong			addrs[MEM_TRACK_ADDRS_COUNT];
#endif
	int			cpu;
	int			pid;
	unsigned long long	when;
};

struct lib_mem_track {
	int			type;
	ulong			addr;
	size_t			size;
	int			status;
	struct _lib_mem_track	alloc;
	struct _lib_mem_track	free;
};

struct lib_mem_tracks {
	struct list_head	list;
	int			num_of_track;
	struct lib_mem_track	track[MEM_TRACK_COUNT];
};
#endif

#define MEM_BLOCK_NAME_LEN	16
struct lib_mem_block {
	spinlock_t		lock;
	struct list_head	list;
	u32			used;	/* size was allocated totally */
	u32			end;	/* last allocation position */

	size_t			align;
	ulong			kva_base;
	dma_addr_t		dva_base;

	struct is_priv_buf *pb;

	char			name[MEM_BLOCK_NAME_LEN];
	u32			type;
};

struct lib_buf {
	u32			size;
	ulong			kva;
	dma_addr_t		dva;
	struct list_head	list;
	void			*priv;
};

struct general_intr_handler {
	int irq;
	int id;
	int (*intr_func)(void);
};

enum general_intr_id {
	ID_GENERAL_INTR_PREPROC_PDAF = 0,
	ID_GENERAL_INTR_PDP0_STAT,
	ID_GENERAL_INTR_PDP1_STAT,
	ID_GENERAL_INTR_MAX
};

enum ddk_timer_id {
	ID_TIMER_0 = 0,
	ID_TIMER_1,
	ID_TIMER_2,
	ID_TIMER_3,
	ID_TIMER_MAX
};

typedef int (*timer_cb_t)(void *data);

struct lib_timer {
	struct hrtimer dtimer;
	timer_cb_t cb;
	void *data;
};

struct is_dump_set  {
	u32 frame_cnt;
	char chain_name[IS_STR_LEN];
	bool valid;
	u32 reg_value[SFR_DUMP_SIZE / 4];
	struct kthread_work work;
};

struct is_dump_to_file {
	int dump_set_size;
	atomic_t dump_set_idx;
	struct task_struct *task;
	struct kthread_worker worker;
	struct is_dump_set *is_dump_set;
};

#define SIZE_LIB_LOG_PREFIX	18	/* [%5lu.%06lu] [%d]  */
#define SIZE_LIB_LOG_BUF	256
#define MAX_LIB_LOG_BUF		(SIZE_LIB_LOG_PREFIX + SIZE_LIB_LOG_BUF)
struct is_lib_support {
	struct is_minfo			*minfo;

	struct is_interface_ischain	*itfc;
	struct hwip_intr_handler		*intr_handler_taaisp[ID_3AAISP_MAX][INTR_HWIP_MAX];
	struct is_lib_task			task_taaisp[TASK_INDEX_MAX];

	bool					binary_load_flg;
	int					binary_load_fatal;
	int					binary_code_load_flg;

	/* memory blocks */
	struct lib_mem_block			mb_heap_rta;
	struct lib_mem_block			mb_dma_taaisp;
	struct lib_mem_block			mb_dma_medrc;
	struct lib_mem_block			mb_dma_tnr;
	struct lib_mem_block			mb_dma_tnr_s;
	struct lib_mem_block			mb_dma_orbmch;
	struct lib_mem_block			mb_dma_clahe;
	struct lib_mem_block			mb_vra;
	struct lib_mem_block			mb_vra_net_array;
	/* non-memory block */
	spinlock_t				slock_nmb;
	struct list_head			list_of_nmb;

	/* for log */
	spinlock_t				slock_debug;
	ulong					log_ptr;
	char					log_buf[MAX_LIB_LOG_BUF];
	char					string[MAX_LIB_LOG_BUF];

#ifdef ENABLE_DBG_EVENT
	char					debugmsg[SIZE_LIB_LOG_BUG];
#endif
	/* for library load */
	struct platform_device			*pdev;
#ifdef LIB_MEM_TRACK
	spinlock_t				slock_mem_track;
	struct list_head			list_of_tracks;
	struct lib_mem_tracks			*cur_tracks;
#endif
	struct general_intr_handler		intr_handler[ID_GENERAL_INTR_MAX];
	char		fw_name[50];		/* DDK */
	char		rta_fw_name[50];	/* RTA */

	struct lib_timer			timer[ID_TIMER_MAX];
	ulong					timer_state;

	struct is_binary			lib_file;
	struct is_dump_to_file			dump_to_file;
};

struct is_memory_attribute {
	pgprot_t state;
	int numpages;
	ulong vaddr;
};

struct is_memory_change_data {
	pgprot_t set_mask;
	pgprot_t clear_mask;
};

/* TODO: remove below */
/* Fast FDAE & FDAF */
 struct fd_point32 {
	int x;
	int y;
};

struct fd_rectangle {
	u32 width;
	u32 height;
};

struct fd_areabyrect {
	struct fd_point32 top_left;
	struct fd_rectangle span;
};

struct fd_faceinfo {
	struct fd_areabyrect face_area;
	int faceId;
	int score;
	u32 rotation;
};

struct fd_info {
	struct fd_faceinfo face[MAX_FACE_COUNT];
	int face_num;
	u32 frame_count;
};

#if IS_ENABLED(CONFIG_PABLO_HW_HELPER_V1)
struct is_lib_support *is_get_lib_support(void);
void is_interface_set_irq_handler(struct is_hw_ip *hw_ip, int handler_id,
					struct is_interface_hwip *itf_ip);
void is_interface_set_lib_support(struct is_interface_ischain *itfc,
					struct platform_device *pdev);

int is_load_bin(void);
void is_load_clear(void);
void is_interface_lib_shut_down(struct is_minfo *minfo);
int is_init_ddk_thread(void);
void is_set_ddk_thread_affinity(void);
void is_flush_ddk_thread(void);
void check_lib_memory_leak(void);

int is_log_write(const char *str, ...);
int is_log_write_console(char *str);
int is_lib_logdump(void);

void is_load_ctrl_init(void);
int pablo_lib_init_reserved_mem(struct reserved_mem *rmem);
void pablo_lib_deinit_reserved_mem(void);
int pablo_lib_init_mem(struct is_mem *mem);
int is_sensor_register_itf(struct is_device_sensor *device);
#else
#define is_interface_set_irq_handler(hw_ip, handler_id, itf_ip)
#define is_interface_set_lib_support(itfc, pdev)

#define is_load_bin() 0
#define is_load_clear()
#define is_interface_lib_shut_down(minfo)
#define is_init_ddk_thread() 0
#define is_set_ddk_thread_affinity()
#define is_flush_ddk_thread()
#define check_lib_memory_leak()
#define is_log_write(str, ...) 0
#define is_log_write_console(str) 0
#define is_lib_logdump() 0
#define is_load_ctrl_init()
#define pablo_lib_init_reserved_mem(rmem) 0
#define pablo_lib_deinit_reserved_mem() 0
#define pablo_lib_init_mem(rscmgr) 0
#define is_sensor_register_itf(device) 0
#endif

int is_spin_lock_init(void **slock);
int is_spin_lock_finish(void *slock_lib);
int is_spin_lock(void *slock_lib);
int is_spin_unlock(void *slock_lib);
int is_spin_lock_irq(void *slock_lib);
int is_spin_unlock_irq(void *slock_lib);
ulong is_spin_lock_irqsave(void *slock_lib);
int is_spin_unlock_irqrestore(void *slock_lib, ulong flag);

void *is_alloc_vra(u32 size);
void is_free_vra(void *buf);
int is_dva_vra(ulong kva, u32 *dva);
void is_inv_vra(ulong kva, u32 size);
void is_clean_vra(ulong kva, u32 size);

void *is_alloc_vra_net_array(u32 size);
void is_free_vra_net_array(void *buf);
void is_inv_vra_net_array(ulong kva, u32 size);
int is_dva_vra_net_array(ulong kva, u32 *dva);

bool is_lib_in_irq(void);

int is_load_bin_on_boot(void);
void is_load_ctrl_unlock(void);
void is_load_ctrl_lock(void);
int is_set_fw_names(char *fw_name, char *rta_fw_name);

int is_timer_create(int timer_id, void *func, void *data);
int is_timer_delete(int timer_id);
int is_timer_reset(void *timer, ulong expires);
int is_timer_query(void *timer, ulong timerValue);
int is_timer_enable(int timer_id, u32 timeout_ms);
int is_timer_disable(int timer_id);

int is_debug_attr(char *cmd, int *param1, int *param2, int *param3);

#if IS_ENABLED(USE_DDK_HWIP_INTERFACE)
int is_set_regs_array(void *this, u32 chain_id, int instance, u32 fcount, struct cr_set *regs, u32 regs_size);
int is_dump_regs_array(int target_hw_id, int instance, u32 fcount, struct cr_set *regs, u32 regs_size);
int is_dump_regs_log(void *this, u32 chain_id, const char *str);
int is_dump_regs_addr_value(void *this, u32 chain_id, u32 **addr_array, u32 **value_array);
int is_dump_init_reg_dump(u32 count);
int is_dump_regs_file(void *this, u32 chain_id, const char *filepath);
int is_set_config(void *this, u32 chain_id, int instance, u32 fcount, void *config);
#else
int is_set_regs_array(int target_hw_id, int instance, u32 fcount, struct cr_set *regs, u32 regs_size);
int is_dump_regs_array(int target_hw_id, int instance, u32 fcount, struct cr_set *regs, u32 regs_size);
#endif
int is_file_read(const char *ppath, const char *pfname, void **pdata, u32 *size);

unsigned long _uh_call2(u64 app_id, u64 command, u64 arg0, u64 arg1, u64 arg2, u64 arg3);
static inline void uh_call2(u64 app_id, u64 command, u64 arg0, u64 arg1, u64 arg2, u64 arg3)
{
	_uh_call2(app_id | command, arg0, arg1, arg2, arg3, 0);
}

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
#define KUNIT_TEST_LIB_ISP 0
#define KUNIT_TEST_LIB_RTA 1
#define KUNIT_TEST_LIB_VRA 2

os_system_func_t pablo_kunit_get_os_system_func(int type, int index);
void pablo_kunit_mblk_init(struct lib_mem_block *mblk, struct is_priv_buf *pb,
	u32 type, const char *name);
#endif

#endif
