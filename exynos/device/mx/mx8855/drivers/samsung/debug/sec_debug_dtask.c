// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/rwsem.h>
#include <linux/rtmutex.h>
#include <linux/interrupt.h>
#include <linux/sec_debug.h>
#include <soc/samsung/exynos/debug-snapshot.h>

#include "sec_debug_internal.h"

#include <trace/hooks/dtask.h>

struct sec_debug_wait {
	long	type;
	void	*data;
};

#define BUF_SIZE	128

/* use ANDROID_OEM_DATA_ARRAY(1,2) */
static inline long __wait_type(struct task_struct *task)
{
	return (long)task->android_oem_data1[0];
}

static inline void *__wait_data(struct task_struct *task)
{
	return (void *)task->android_oem_data1[1];
}

static void secdbg_dtsk_set_data(long type, void *data)
{
	struct task_struct *tsk = current;

	tsk->android_oem_data1[0] = (u64)type;
	tsk->android_oem_data1[1] = (u64)data;
}

/* from kernel/locking/mutex.c */
#define MUTEX_FLAGS		0x07
static inline struct task_struct *__mutex_owner_task(unsigned long owner)
{
	return (struct task_struct *)(owner & ~MUTEX_FLAGS);
}

static void secdbg_print_mutex_info(struct task_struct *task, struct sec_debug_wait *winfo, bool raw)
{
	struct mutex *wmutex = (struct mutex *)winfo->data;
	struct task_struct *owner_task = __mutex_owner_task(atomic_long_read(&wmutex->owner));

	pr_info("Mutex: %pS", wmutex);
	if (owner_task) {
		if (raw)
			pr_cont(": owner[0x%px %s :%d]", owner_task, owner_task->comm, owner_task->pid);
		else
			pr_cont(": owner[%s :%d]", owner_task->comm, owner_task->pid);
	}
	pr_cont("\n");
}

static void secdbg_print_rtmutex_info(struct task_struct *task, struct sec_debug_wait *winfo, bool raw)
{
	struct rt_mutex_base *wmutex = (struct rt_mutex_base *)winfo->data;
	struct task_struct *owner_task = __mutex_owner_task((unsigned long)READ_ONCE(wmutex->owner));

	pr_info("RTmutex: %pS", wmutex);
	if (owner_task) {
		if (raw)
			pr_cont(": owner[0x%px %s :%d]", owner_task, owner_task->comm, owner_task->pid);
		else
			pr_cont(": owner[%s :%d]", owner_task->comm, owner_task->pid);
	}
	pr_cont("\n");
}

/* from kernel/locking/rwsem.c */
#define RWSEM_FLAGS		0x07
#define RWSEM_READER_OWNED	(1UL << 0)
#define RWSEM_WRITER_LOCKED	(1UL << 0)
#define RWSEM_READER_SHIFT	8

static inline struct task_struct *__rwsem_owner_task(unsigned long owner)
{
	return (struct task_struct *)(owner & ~RWSEM_FLAGS);
}

static inline bool __rwsem_writer_locked(struct rw_semaphore *sem)
{
	return (atomic_long_read(&sem->count) & RWSEM_WRITER_LOCKED);
}

static inline bool __rwsem_reader_owned(struct rw_semaphore *sem)
{
	return (atomic_long_read(&sem->owner) & RWSEM_READER_OWNED);
}

static inline long __rwsem_readers_count(struct rw_semaphore *sem)
{
	return atomic_long_read(&sem->count) >> RWSEM_READER_SHIFT;
}

static void secdbg_print_rwsem_info(struct task_struct *task, struct sec_debug_wait *winfo, bool raw)
{
	const char *owned_str;
	struct rw_semaphore *rwsem = (struct rw_semaphore *)winfo->data;
	struct task_struct *owner_task = __rwsem_owner_task(atomic_long_read(&rwsem->owner));
	char tmp[BUF_SIZE] = {0,};

	if (__rwsem_writer_locked(rwsem)) {
		owned_str = "owned by writer";
	} else if (__rwsem_reader_owned(rwsem)) {
		scnprintf(tmp, BUF_SIZE, "%ld readers", __rwsem_readers_count(rwsem));
		owned_str = tmp;
	} else {
		owned_str = "not owned";
	}

	/* it may not be the real owner nor one of the real owner */
	if (!__rwsem_writer_locked(rwsem) && !IS_ENABLED(CONFIG_DEBUG_RWSEMS))
		owner_task = NULL;

	pr_info("RWsem: %pS: %s", rwsem, owned_str);
	if (owner_task) {
		if (raw)
			pr_cont("[0x%px %s :%d]", owner_task,
					owner_task->comm, owner_task->pid);
		else
			pr_cont("[%s :%d]",
					owner_task->comm, owner_task->pid);
	}
	pr_cont("\n");
}

static void secdbg_print_syncirq_info(struct task_struct *task, struct sec_debug_wait *winfo, bool raw)
{
	struct irq_desc *desc = (struct irq_desc *)winfo->data;
	struct task_struct *irq_thread;
	const char *name;

	if (desc->action && desc->action->name)
		name = desc->action->name;
	else
		name = "???";

	if (desc->action && desc->action->thread)
		irq_thread = desc->action->thread;
	else
		irq_thread = NULL;

	pr_info("SyncIRQ: irq-%u %s", desc->irq_data.irq, name);
	if (irq_thread) {
		if (raw)
			pr_cont(": thread[0x%px %s :%d]", irq_thread,
					irq_thread->comm, irq_thread->pid);
		else
			pr_cont(": thread[%s :%d]",
					irq_thread->comm, irq_thread->pid);
	}
	pr_cont("\n");
}

static void secdbg_print_dpm_device_info(struct task_struct *task, struct sec_debug_wait *winfo, bool raw)
{
	struct device *dev = (struct device *)winfo->data;

	pr_info("DPM: %s %s\n", dev_driver_string(dev), dev_name(dev));
}

static void secdbg_dtsk_print_info(struct task_struct *task, bool raw)
{
	struct sec_debug_wait winfo;

	winfo.type = __wait_type(task);
	winfo.data = __wait_data(task);

	if (!(winfo.type && winfo.data))
		return;

	switch (winfo.type) {
	case DTYPE_MUTEX:
		secdbg_print_mutex_info(task, &winfo, raw);
		break;
	case DTYPE_RTMUTEX:
		secdbg_print_rtmutex_info(task, &winfo, raw);
		break;
	case DTYPE_RWSEM:
		secdbg_print_rwsem_info(task, &winfo, raw);
		break;
	case DTYPE_SYNCIRQ:
		secdbg_print_syncirq_info(task, &winfo, raw);
		break;
	case DTYPE_DPMDEV:
		secdbg_print_dpm_device_info(task, &winfo, raw);
		break;
	default:
		/* unknown type */
		break;
	}
}

static int secdbg_dtsk_dss_one_task(struct notifier_block *nb,
				    unsigned long val, void *task)
{
	secdbg_dtsk_print_info(task, true);

	return NOTIFY_DONE;
}

static struct notifier_block nb_dss_one_task_block = {
	.notifier_call = secdbg_dtsk_dss_one_task,
};

static void secdbg_dtsk_sched_show_task(void *ignore, struct task_struct *task)
{
	secdbg_dtsk_print_info(task, false);
}

static void secdbg_dtsk_mutex_start(void *ignore, struct mutex *lock)
{
	secdbg_dtsk_set_data(DTYPE_MUTEX, lock);
}

static void secdbg_dtsk_mutex_finish(void *ignore, struct mutex *lock)
{
	secdbg_dtsk_set_data(DTYPE_NONE, NULL);
}

static void secdbg_dtsk_rtmutex_start(void *ignore, struct rt_mutex_base *lock)
{
	secdbg_dtsk_set_data(DTYPE_RTMUTEX, lock);
}

static void secdbg_dtsk_rtmutex_finish(void *ignore, struct rt_mutex_base *lock)
{
	secdbg_dtsk_set_data(DTYPE_NONE, NULL);
}

static void secdbg_dtsk_rwsem_start(void *ignore, struct rw_semaphore *sem)
{
	secdbg_dtsk_set_data(DTYPE_RWSEM, sem);
}

static void secdbg_dtsk_rwsem_finish(void *ignore, struct rw_semaphore *sem)
{
	secdbg_dtsk_set_data(DTYPE_NONE, NULL);
}

static void secdbg_dtsk_dpm_wait_start(void *ignore, struct device *dev)
{
	secdbg_dtsk_set_data(DTYPE_DPMDEV, dev);
}

static void secdbg_dtsk_dpm_wait_finish(void *ignore, struct device *dev)
{
	secdbg_dtsk_set_data(DTYPE_NONE, NULL);
}

static void secdbg_dtsk_sync_irq_start(void *ignore, struct irq_desc *desc)
{
	secdbg_dtsk_set_data(DTYPE_SYNCIRQ, desc);
}

static void secdbg_dtsk_sync_irq_finish(void *ignore, struct irq_desc *desc)
{
	secdbg_dtsk_set_data(DTYPE_NONE, NULL);
}

static void secdbg_dtsk_flush_wq_start(void *ignore, struct workqueue_struct *wq)
{
	secdbg_dtsk_set_data(DTYPE_WQFLUSH, wq);
}

static void secdbg_dtsk_flush_wq_finish(void *ignore, struct workqueue_struct *wq)
{
	secdbg_dtsk_set_data(DTYPE_NONE, NULL);
}

static void secdbg_dtsk_flush_work_start(void *ignore, struct work_struct *work)
{
	secdbg_dtsk_set_data(DTYPE_WORK, work);
}

static void secdbg_dtsk_flush_work_finish(void *ignore, struct work_struct *work)
{
	secdbg_dtsk_set_data(DTYPE_NONE, NULL);
}

static __init_or_module int secdbg_dtsk_init(void)
{
	pr_info("%s: init\n", __func__);

#if defined(CONFIG_TRACEPOINTS) && defined(CONFIG_ANDROID_VENDOR_HOOKS)
	register_dump_one_task_notifier(&nb_dss_one_task_block);
	register_trace_android_vh_sched_show_task(secdbg_dtsk_sched_show_task, NULL);

	register_trace_android_vh_mutex_wait_start(secdbg_dtsk_mutex_start, NULL);
	register_trace_android_vh_mutex_wait_finish(secdbg_dtsk_mutex_finish, NULL);

	register_trace_android_vh_rtmutex_wait_start(secdbg_dtsk_rtmutex_start, NULL);
	register_trace_android_vh_rtmutex_wait_finish(secdbg_dtsk_rtmutex_finish, NULL);

	register_trace_android_vh_rwsem_read_wait_start(secdbg_dtsk_rwsem_start, NULL);
	register_trace_android_vh_rwsem_write_wait_start(secdbg_dtsk_rwsem_start, NULL);
	register_trace_android_vh_rwsem_read_wait_finish(secdbg_dtsk_rwsem_finish, NULL);
	register_trace_android_vh_rwsem_write_wait_finish(secdbg_dtsk_rwsem_finish, NULL);

	register_trace_android_vh_dpm_wait_start(secdbg_dtsk_dpm_wait_start, NULL);
	register_trace_android_vh_dpm_wait_finish(secdbg_dtsk_dpm_wait_finish, NULL);
	register_trace_android_vh_sync_irq_wait_start(secdbg_dtsk_sync_irq_start, NULL);
	register_trace_android_vh_sync_irq_wait_finish(secdbg_dtsk_sync_irq_finish, NULL);
	register_trace_android_vh_flush_wq_wait_start(secdbg_dtsk_flush_wq_start, NULL);
	register_trace_android_vh_flush_wq_wait_finish(secdbg_dtsk_flush_wq_finish, NULL);
	register_trace_android_vh_flush_work_wait_start(secdbg_dtsk_flush_work_start, NULL);
	register_trace_android_vh_flush_work_wait_finish(secdbg_dtsk_flush_work_finish, NULL);

#endif

	return 0;
}
subsys_initcall(secdbg_dtsk_init);

MODULE_DESCRIPTION("Samsung Debug Dtask driver");
MODULE_LICENSE("GPL v2");
