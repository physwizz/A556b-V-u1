/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * drivers/debug/sec_debug_internal.h
 *
 * COPYRIGHT(C) 2020 Samsung Electronics Co., Ltd. All Right Reserved.
 *
 */

#ifndef __SEC_DEBUG_INTERNAL_H__
#define __SEC_DEBUG_INTERNAL_H__

#include <linux/sec_debug.h>
#include <linux/sizes.h>

/* TODO: SoC dependent offset, get them from LSI code ? */
#define EXYNOS_PMU_INFORM2 0x00B8
#define EXYNOS_PMU_INFORM3 0x00BC

/*
 * SEC DEBUG - DEBUG SNAPSHOT BASE HOOKING
 */
enum {
	DSS_KEVENT_TASK,
	DSS_KEVENT_WORK,
	DSS_KEVENT_IRQ,
	DSS_KEVENT_FREQ,
	DSS_KEVENT_IDLE,
	DSS_KEVENT_THRM,
	DSS_KEVENT_ACPM,
	DSS_KEVENT_MFRQ,
	DSS_KEVENT_PMIC,
	DSS_KEVENT_MAX,
};

#define SD_ESSINFO_KEY_SIZE	(32)

struct ess_info_offset {
	char key[SD_ESSINFO_KEY_SIZE];
	unsigned long base;
	unsigned long last;
	unsigned int nr;
	unsigned int size;
	unsigned int per_core;
};

/* AP SFR to send some information from kernel to bootloader */
#define SEC_DEBUG_MAGIC_INFORM		(EXYNOS_PMU_INFORM2)
#define SEC_DEBUG_PANIC_INFORM		(EXYNOS_PMU_INFORM3)

/* RESET REASON */
enum sec_debug_reset_reason_t {
	RR_S = 1,
	RR_W = 2,
	RR_D = 3,
	RR_K = 4,
	RR_M = 5,
	RR_P = 6,
	RR_R = 7,
	RR_B = 8,
	RR_N = 9,
	RR_T = 10,
	RR_C = 11,
};

/* sec debug buffer format */
struct outbuf {
	char buf[SZ_1K];
	int index;
	int already;
};

/* for sub data-structure of SDN */
struct sec_debug_ksyms {
	uint32_t magic;
	uint32_t kallsyms_all;
	uint64_t addresses_pa;
	uint64_t names_pa;
	uint64_t num_syms;
	uint64_t token_table_pa;
	uint64_t token_index_pa;
	uint64_t markers_pa;
	struct ksect {
		uint64_t sinittext;
		uint64_t einittext;
		uint64_t stext;
		uint64_t etext;
		uint64_t end;
	} sect;
	uint64_t relative_base;
	uint64_t offsets_pa;
	uint64_t kimage_voffset;
	uint64_t reserved[4];
};

/* kcnst has some kernel constant (offset) data for bootloader */
struct basic_type_int {
	uint64_t pa;	/* physical address of the variable */
	uint32_t size;	/* size of basic type. eg sizeof(unsigned long) goes here */
	uint32_t count;	/* for array types */
};

struct sec_debug_kcnst {
	uint64_t nr_cpus;
	struct basic_type_int per_cpu_offset;

	uint64_t phys_offset;
	uint64_t phys_mask;
	uint64_t page_offset;
	uint64_t page_mask;
	uint64_t page_shift;

	uint64_t va_bits;
	uint64_t kimage_vaddr;
	uint64_t kimage_voffset;

	uint64_t pa_swapper;
	uint64_t pgdir_shift;
	uint64_t pud_shift;
	uint64_t pmd_shift;

	uint64_t ptrs_per_pgd;
	uint64_t ptrs_per_pud;
	uint64_t ptrs_per_pmd;
	uint64_t ptrs_per_pte;

	uint64_t kconfig_base;
	uint64_t kconfig_size;

	uint64_t pa_text;
	uint64_t pa_start_rodata;

	uint64_t target_dprm_mask;

	uint64_t reserved[3];
};

struct member_type {
	uint16_t size;
	uint16_t offset;
};

typedef struct member_type member_type_int;
typedef struct member_type member_type_long;
typedef struct member_type member_type_longlong;
typedef struct member_type member_type_ptr;
typedef struct member_type member_type_str;

struct struct_thread_info {
	uint32_t struct_size;
	member_type_long flags;
	member_type_ptr task;
	member_type_int cpu;
	member_type_long rrk;
};

struct struct_task_struct {
	uint32_t struct_size;
	member_type_long state;
	member_type_long exit_state;
	member_type_ptr stack;
	member_type_int flags;
	member_type_int on_cpu;
	member_type_int on_rq;
	member_type_int cpu;
	member_type_int pid;
	member_type_str comm;
	member_type_ptr tasks_next;
	member_type_ptr thread_group_next;
	member_type_long fp;
	member_type_long sp;
	member_type_long pc;
	member_type_long sched_info__pcount;
	member_type_longlong sched_info__run_delay;
	member_type_longlong sched_info__last_arrival;
	member_type_longlong sched_info__last_queued;
	member_type_int ssdbg_wait__type;
	member_type_ptr ssdbg_wait__data;
};

struct irq_stack_info {
	uint64_t pcpu_stack;	/* IRQ_STACK_PTR(0) */
	uint64_t size;		/* IRQ_STACK_SIZE */
	uint64_t start_sp;	/* IRQ_STACK_START_SP */
};

/* task_struct offset data */
struct sec_debug_task {
	uint64_t stack_size; /* THREAD_SIZE */
	uint64_t start_sp; /* TRHEAD_START_SP */
	struct struct_thread_info ti;
	struct struct_task_struct ts;
	uint64_t init_task;
	struct irq_stack_info irq_stack;
};

#define SD_NR_ESSINFO_ITEMS	(16)
/* Exynos Debug Snapshot offset data */
struct sec_debug_ess_info {
	struct ess_info_offset item[SD_NR_ESSINFO_ITEMS];
};

/* Watchdog driver data */
struct watchdogd_info {
	struct task_struct *tsk;
	struct thread_info *thr;
	struct rtc_time *tm;

	unsigned long long last_ping_time;
	int last_ping_cpu;
	bool init_done;

	unsigned long emerg_addr;
};

struct bad_stack_info {
	unsigned long magic;
	unsigned long esr;
	unsigned long far;
	unsigned long spel0;
	unsigned long cpu;
	unsigned long tsk_stk;
	unsigned long irq_stk;
	unsigned long ovf_stk;
};

struct suspend_dev_info {
	uint64_t suspend_func;
	uint64_t suspend_device;
	uint64_t shutdown_func;
	uint64_t shutdown_device;
};

struct sec_debug_kernel_data {
	uint64_t task_in_pm_suspend;
	uint64_t task_in_sys_reboot;
	uint64_t task_in_sys_shutdown;
	uint64_t task_in_dev_shutdown;
	uint64_t task_in_sysrq_crash;
	uint64_t task_in_soft_lockup;
	uint64_t cpu_in_soft_lockup;
	uint64_t task_in_hard_lockup;
	uint64_t cpu_in_hard_lockup;
	uint64_t unfrozen_task;
	uint64_t unfrozen_task_count;
	uint64_t sync_irq_task;
	uint64_t sync_irq_num;
	uint64_t sync_irq_name;
	uint64_t sync_irq_desc;
	uint64_t sync_irq_thread;
	uint64_t sync_irq_threads_active;
	uint64_t dev_shutdown_start;
	uint64_t dev_shutdown_end;
	uint64_t dev_shutdown_duration;
	uint64_t dev_shutdown_func;
	unsigned long sysrq_ptr;
	struct watchdogd_info wddinfo;
	struct bad_stack_info bsi;
	struct suspend_dev_info sdi;
};

/* some buffers to use in sec debug module */
enum sdn_map {
	SDN_MAP_DUMP_SUMMARY,
	SDN_MAP_AUTO_COMMENT,
	SDN_MAP_EXTRA_INFO,
	SDN_MAP_AUTO_ANALYSIS,
	SDN_MAP_INITTASK_LOG,
	SDN_MAP_DEBUG_PARAM,
	SDN_MAP_FIRST2M_LOG,
	SDN_MAP_SPARED_BUFFER,
	NR_SDN_MAP,
};

struct sec_debug_buf {
	unsigned long base;
	unsigned long size;
};

struct sec_debug_map {
	struct sec_debug_buf buf[NR_SDN_MAP];
};

/* macro to initialize kernel data structure offset data */
#define SET_MEMBER_TYPE_INFO(PTR, TYPE, MEMBER) \
	{ \
		(PTR)->size = sizeof(((TYPE *)0)->MEMBER); \
		(PTR)->offset = offsetof(TYPE, MEMBER); \
	}

struct sec_debug_memtab {
	uint64_t table_start_pa;
	uint64_t table_end_pa;
	uint64_t reserved[4];
};

#define THREAD_START_SP		(THREAD_SIZE - 16)
#define IRQ_STACK_START_SP	THREAD_START_SP

#define SEC_DEBUG_MAGIC0	(0x11221133)
#define SEC_DEBUG_MAGIC1	(0x12121313)

/* TODO: sdn needs extra info data structure to define it normally, but ... */
/* ------------------------------------------------
 * SEC DEBUG EXTRA INFO
 * ------------------------------------------------ */
enum shared_buffer_slot {
	SLOT_32,
	SLOT_64,
	SLOT_256,
	SLOT_1024,
	SLOT_MAIN_END = SLOT_1024,
	NR_MAIN_SLOT = 4,
	SLOT_BK_32 = NR_MAIN_SLOT,
	SLOT_BK_64,
	SLOT_BK_256,
	SLOT_BK_1024,
	SLOT_END = SLOT_BK_1024,
	NR_SLOT = 8,
};

struct sec_debug_sb_index {
	unsigned int paddr;		/* physical address of slot */
	unsigned int size;		/* size of a item */
	unsigned int nr;		/* number of items in slot */
	unsigned int cnt;		/* number of used items in slot */

	/* map to indicate which items are added by bootloader */
	unsigned long blmark;
};

struct sec_debug_shared_buffer {
	/* initial magic code */
	unsigned int magic[4];

	/* shared buffer index */
	struct sec_debug_sb_index sec_debug_sbidx[NR_SLOT];
};

/* TODO: sdn needs auto comment data structure to define it normally, but ... */
/* ------------------------------------------------
 * SEC DEBUG AUTO COMMENT
 * ------------------------------------------------ */
#define SEC_DEBUG_AUTO_COMM_BUF_SIZE 10

struct sec_debug_auto_comm_buf {
	int reserved_0;
	int reserved_1;
	int reserved_2;
	unsigned int offset;
	char buf[SZ_4K];
};

struct sec_debug_auto_comment {
	int header_magic;
	int fault_flag;
	int lv5_log_cnt;
	u64 lv5_log_order;
	int order_map_cnt;
	int order_map[SEC_DEBUG_AUTO_COMM_BUF_SIZE];
	struct sec_debug_auto_comm_buf auto_comm_buf[SEC_DEBUG_AUTO_COMM_BUF_SIZE];

	int tail_magic;
};

/* ------------------------------------------------
 * SEC DEBUG GEN3 (Compatible with SEC DEBUG NEXT)
 * ------------------------------------------------ */
#define SEC_DEBUG_GEN3_MAGIC0	(0xc3a50421)
#define SEC_DEBUG_GEN3_MAGIC1	(0xe2b42021)

#define LEN_SECDBG_OFFSET_NAME		(64)

struct sec_debug_version {
	unsigned int magic[2];
	unsigned int version[2];
};

#define NR_GEN3_LOGBUF		32
#define NR_GEN3_KEVENT		32

/* LV1 */
struct secdbg_extra_info {
	struct sec_debug_shared_buffer data;

	char name[LEN_SECDBG_OFFSET_NAME];
};

/* LV1 */
struct secdbg_auto_comment {
	struct sec_debug_auto_comment data;

	char name[LEN_SECDBG_OFFSET_NAME];
};

/* LV1 */
struct secdbg_kernel_data {
	struct sec_debug_kernel_data data;

	char name[LEN_SECDBG_OFFSET_NAME];
};

/* LV1 */
struct secdbg_snapshot_offset {
	struct ess_info_offset data[NR_GEN3_KEVENT];

	char name[LEN_SECDBG_OFFSET_NAME];
};

/* LV1 */
struct secdbg_task_struct {
	struct sec_debug_task data;

	char name[LEN_SECDBG_OFFSET_NAME];
};

/* LV1 */
struct secdbg_kernel_constant {
	struct sec_debug_kcnst data;

	char name[LEN_SECDBG_OFFSET_NAME];
};

#define LEN_LOGBUF_NAME	(32)
/* LV2 (under secdbg_logbuf_list) */
struct secdbg_logbuf {
	uint64_t base;
	uint64_t size;
	char name[LEN_LOGBUF_NAME];
	uint32_t is_storage;
	uint32_t offset;
	uint32_t partition;
};

/* LV1 */
struct secdbg_logbuf_list {
	struct secdbg_logbuf data[NR_GEN3_LOGBUF];

	char name[LEN_SECDBG_OFFSET_NAME];
};

/* LV1 */
struct secdbg_memtab {
	struct sec_debug_memtab data;

	char name[LEN_SECDBG_OFFSET_NAME];
};

/* Offset for LV1 */
struct secdbg_lv1_member {
	char name[LEN_SECDBG_OFFSET_NAME];
	uint64_t size;
	uint64_t addr;
};

enum gen3_lv1_item {
	SDN_LV1_LOGBUF_MAP,
	SDN_LV1_MEMTAB,
	SDN_LV1_KERNEL_SYMBOL,
	SDN_LV1_KERNEL_CONSTANT,
	SDN_LV1_TASK_STRUCT,
	SDN_LV1_SNAPSHOT,
	SDN_LV1_SPINLOCK,	/* reserved */
	SDN_LV1_KERNEL_DATA,
	SDN_LV1_AUTO_COMMENT,
	SDN_LV1_EXTRA_INFO,
};

/* increase if sec_debug_next is not changed and other feature is upgraded */
#define SEC_DEBUG_KERNEL_UPPER_VERSION		(0x1001)
/* increase if sec_debug_next is changed */
#define SEC_DEBUG_KERNEL_LOWER_VERSION		(0x1002)

/* SEC DEBUG NEXT DEFINITION */
struct sec_debug_gen3 {
	unsigned int magic[2];
	unsigned int version[2];
	unsigned int used_offset;
	unsigned int end_addr;
	unsigned int reserved[4];

	struct secdbg_lv1_member lv1_data[64];
};

struct sec_debug_base_param {
	void *sdn_vaddr;
	bool init_sdn_done;
};

/* SEC DEBUG HARDLOCUP INFO */
enum ehld_types {
	NO_INSTRET,
	NO_INSTRUN,
	MAX_ETYPES
};

/* CONFIG_SEC_DEBUG_DTASK */
enum {
	DTYPE_NONE,
	DTYPE_MUTEX,
	DTYPE_RWSEM,
	DTYPE_WORK,
	DTYPE_CPUHP,
	DTYPE_KTHREAD,
	DTYPE_RTMUTEX,
	DTYPE_WQFLUSH,
	DTYPE_SYNCIRQ,
	DTYPE_DPMDEV,
};

/* function for external call */
extern void *secdbg_base_get_debug_base(int type);
extern unsigned long secdbg_base_get_buf_base(int type);
extern unsigned long secdbg_base_get_buf_size(int type);
extern void *secdbg_base_get_ncva(unsigned long pa);
extern unsigned long secdbg_base_get_end_addr(void);
extern void *secdbg_base_get_kcnst_base(void);
extern void *secdbg_base_get_kernd_base(void);
extern int secdbg_part_init_bdev_path(struct device *dev);

#if IS_ENABLED(CONFIG_SEC_DEBUG_LOCKUP_INFO)
void secdbg_base_set_info_hard_lockup(unsigned int cpu, struct task_struct *task);
#endif

/* SEC DEBUG EXTAR INFO */
#define MAX_ITEM_KEY_LEN		(16)
#define MAX_ITEM_VAL_LEN		(1008)

#define SEC_DEBUG_SHARED_MAGIC0 0xFFFFFFFF
#define SEC_DEBUG_SHARED_MAGIC1 0x95308180
#define SEC_DEBUG_SHARED_MAGIC2 0x15001500
#define SEC_DEBUG_SHARED_MAGIC3 0x00010001

extern int id_get_asb_ver(void);
extern int id_get_product_line(void);

extern char *get_bk_item_val(const char *key);
extern void get_bk_item_val_as_string(const char *key, char *buf);

extern void secdbg_exin_set_hwid(int asb_ver, int psite, const char *dramstr);

#if IS_ENABLED(CONFIG_SEC_DEBUG_STACKTRACE)
extern void secdbg_stra_show_callstack_auto(struct task_struct *tsk);
#else
static inline void secdbg_stra_show_callstack_auto(struct task_struct *tsk) {}
#endif

#if IS_ENABLED(CONFIG_SEC_DEBUG_UNFROZEN_TASK)
void secdbg_base_set_unfrozen_task(struct task_struct *task, uint64_t count);
#else
static inline void secdbg_base_set_unfrozen_task(struct task_struct *task, uint64_t count) {}
#endif /* CONFIG_SEC_DEBUG_UNFROZEN_TASK */

typedef void (*secdbg_handle_bad_stack_hook_fn)(void *data, struct pt_regs *regs,
		unsigned long esr, unsigned long far);

#if IS_ENABLED(CONFIG_SEC_DEBUG_BAD_STACK_INFO)
void secdbg_register_hook_handle_bad_stack(secdbg_handle_bad_stack_hook_fn fn);
#else
static inline void secdbg_register_hook_handle_bad_stack(secdbg_handle_bad_stack_hook_fn fn) {}
#endif /* CONFIG_SEC_DEBUG_BAD_STACK_INFO */

#endif /* __SEC_DEBUG_INTERNAL_H__ */
