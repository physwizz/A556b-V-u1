/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <asm/barrier.h>

#include "npu-log.h"
#include "npu-util-autosleepthr.h"
#include "npu-util-common.h"

const char *DEFAULT_THREAD_PRINT_NAME = "Anon. auto_sleep_thread";
const int DEFAULT_NO_ACTIVITY_THRESHOLD = 4;
const int AUTO_SLEEP_THREAD_TIMEOUT = 1000;

static const u8 auto_sleep_thread_state_transition[][THREAD_STATE_INVALID+1] = {

	/* From    -   To	NOT_INITIALIZED	INITIALIZED	RUNNING		SLEEPING	TERMINATED	INVALID		*/
	/* NOT_INITIALIZED */ {		0,		1,		0,		0,		0,		0 },
	/* INITIALIZED     */ {		0,		0,		1,		0,		1,		0 },
	/* RUNNING         */ {		0,		0,		0,		1,		1,		0 },
	/* SLEEPING        */ {		0,		0,		1,		0,		1,		0 },
	/* TERMINATED      */ {		0,		0,		1,		0,		0,		0 },
	/* INVALID         */ {		0,		0,		0,		0,		0,		0 }
};

static inline auto_sleep_thread_state_e auto_sleep_thread_set_state(struct auto_sleep_thread *thrctx, auto_sleep_thread_state_e new_state)
{
	int old_state;

	old_state = atomic_xchg(&(thrctx->thr_state), new_state);

	/* Check after transition is made - To ensure atomicity */
	if (!auto_sleep_thread_state_transition[old_state][new_state]) {
		npu_err("NPU: auto sleep thread: invalid transition (%d) -> (%d)\n",
			old_state, new_state);
	}
	return old_state;
}

static void dump_auto_sleep_thread(const struct auto_sleep_thread *ctx)
{
#if !IS_ENABLED(CONFIG_NPU_BRINGUP_NOTDONE)
	npu_dbg("--Auto sleep thread at %pK --\n", ctx);
	npu_dbg("thread_ref = %pK {\n", ctx->thread_ref);
	npu_dbg("pid = %u\n", ctx->thread_ref->pid);
	npu_dbg("tgid = %u\n", ctx->thread_ref->tgid);
	npu_dbg("set_child_tid = %pK\n", ctx->thread_ref->set_child_tid);
	npu_dbg("name = %s\n", ctx->name);
	npu_dbg("do_task = %pK\n", ctx->do_task);
	npu_dbg("task_param = {.data = %pK}\n", ctx->task_param.data);
	npu_dbg("no_activity_threshold = %d\n", ctx->no_activity_threshold);
	npu_dbg("thr_state = %d\n", atomic_read(&(ctx->thr_state)));
#endif
}

static inline int wakeup_check(struct auto_sleep_thread *thrctx)
{
	smp_mb();	/* Ensure the atomic veriable is properly synchronized */

	/* If there is a work to do (check_work) or terminating thread is requested(kthread_should_stop())
	  -> Wake-up the thread */
	if (kthread_should_stop()) {
		npu_dbg("Stop flag asserted.\n");
		return 1;
	}
	if (thrctx->check_work(&thrctx->task_param)) {
		npu_dbg("check_work() returns true. resuming thread.\n");
		return 1;
	}
	/* Nothing to do */
	return 0;
}

static int auto_sleep_thread_thrfunc(void *data)
{
	int	num_activity;
	s64	idle_duration_ns;
	int	no_activity_cnt = 0;
	struct auto_sleep_thread *thrctx = (struct auto_sleep_thread *)data;

	npu_info("ASThread[%s] thrfunc is initiated. ctx = %pK\n"
		 , thrctx->name, thrctx);

	thrctx->idle_start_ns = 0;
	while (!kthread_should_stop()) {
		/* Execute thread task */
		if (!(thrctx->do_task)) {
			npu_trace("no do_task defined. terminating AST. thrctx(%pK)\n", thrctx);
			return 0;
		}
		num_activity = thrctx->do_task(&(thrctx->task_param));

		if (num_activity == 0) {
			/* No activity */
			no_activity_cnt++;

			// No activity more than threshold -> Go to sleep
			if (no_activity_cnt >= thrctx->no_activity_threshold) {
				if (thrctx->idle_start_ns) {
					idle_duration_ns = npu_get_time_ns() - thrctx->idle_start_ns;
				} else {
					/* First idle state */
					idle_duration_ns = 0;
					thrctx->idle_start_ns = npu_get_time_ns();
				}
				npu_trace("ASThread[%s] goes into sleep. no_activity_cnt = %d, idle duration = %lld\n",
					thrctx->name, no_activity_cnt, idle_duration_ns);

				/* Invoke idle callback if available */
				if (thrctx->on_idle)
					thrctx->on_idle(&(thrctx->task_param), idle_duration_ns);

				auto_sleep_thread_set_state(thrctx, THREAD_STATE_SLEEPING);
				wait_event_interruptible_timeout(thrctx->wq,
					wakeup_check(thrctx), thrctx->period);

				auto_sleep_thread_set_state(thrctx, THREAD_STATE_RUNNING);
				no_activity_cnt = 0;
				npu_trace("ASThread[%s] wakeup.\n", thrctx->name);
			}
		} else {
			no_activity_cnt = 0;
			thrctx->idle_start_ns = 0;
		}
	}

	npu_info("ASThread(%s) terminated\n", thrctx->name);
	return 0;
}

int auto_sleep_thread_create(struct auto_sleep_thread *newthr, const char *print_name,
	int (*do_task)(struct auto_sleep_thread_param *data),
	int (*check_work)(struct auto_sleep_thread_param *data),
	void (*on_idle)(struct auto_sleep_thread_param *data, s64 idle_duration_ns),
	unsigned int period)
{
	atomic_set(&(newthr->thr_state), THREAD_STATE_NOT_INITIALIZED);

	npu_dbg("start in creating auto sleep thread\n");
	/* Setting print name */
	if (print_name == NULL) {
		// Use default name
		strncpy(newthr->name, DEFAULT_THREAD_PRINT_NAME, PRINT_NAME_LEN);
	} else {
		strncpy(newthr->name, print_name, PRINT_NAME_LEN);
	}
	newthr->name[PRINT_NAME_LEN] = '\0';

	/* Initialize other fields */
	newthr->thread_ref = NULL;
	newthr->do_task = (do_task);
	newthr->check_work = (check_work);
	newthr->on_idle = (on_idle);
	newthr->no_activity_threshold = DEFAULT_NO_ACTIVITY_THRESHOLD;
	if (period == 0)
		newthr->period = AUTO_SLEEP_THREAD_TIMEOUT;
	else
		newthr->period = period;
	auto_sleep_thread_set_state(newthr, THREAD_STATE_INITIALIZED);

	npu_info("Creating Auto Sleep Thread(%s): Completed - newthr(%pK), do_task(%pK)\n",
		 newthr->name, newthr, newthr->do_task);
	return 0;
}


int auto_sleep_thread_start(struct auto_sleep_thread *thrctx, struct auto_sleep_thread_param param)
{
	npu_info("Starting Autho Sleep Thread[%s] : Starting - newthr = %pK, do_task = %pK\n",
		 thrctx->name, thrctx, thrctx->do_task);

	/* Initialize companion objects */
	init_waitqueue_head(&(thrctx->wq));
	thrctx->task_param = param;

	auto_sleep_thread_set_state(thrctx, THREAD_STATE_RUNNING);

	npu_info("calling kthread_run for (%s)...\n", thrctx->name);
	thrctx->thread_ref = kthread_run(auto_sleep_thread_thrfunc, thrctx, "%s", thrctx->name);
	if (IS_ERR(thrctx->thread_ref)) {
		npu_err("NPU: kthread_run failed(%pK) err(%ld) [%s]\n",
			thrctx->thread_ref, PTR_ERR(thrctx->thread_ref), thrctx->name);
		return -EFAULT;
	}
	dump_auto_sleep_thread(thrctx);
	npu_info("Starting Auto Sleep Thread[%s] : Completed - newthr = %pK, do_task = %pK\n",
		 thrctx->name, thrctx, thrctx->do_task);
	return 0;
}

/*
 * Terminate the auto_sleep_thread and returns return value of
 * thread function, or returns -EINTR if wake_up_process was neve called
 * (Please refer the description of 'kthread_stop (..) for EINTR error)
 */
int auto_sleep_thread_terminate(struct auto_sleep_thread *thrctx)
{
	int ret = 0;

	npu_info("terminating auto sleep thread(%s) : starting - newthr(%pK), do_task(%pK)\n",
		 thrctx->name, thrctx, thrctx->do_task);

	dump_auto_sleep_thread(thrctx);

	/* Wake-up the thread because sleeping thread would not check the stop flag */
	wake_up_interruptible(&(thrctx->wq));

	npu_info("wait for thread termination of (%s)...\n", thrctx->name);

	/* Now terminating the thread */
	ret = kthread_stop(thrctx->thread_ref);

	auto_sleep_thread_set_state(thrctx, THREAD_STATE_TERMINATED);

	npu_info("terminating auto sleep thread(%s) : Completed\n",
		 thrctx->name);
	/* Prepare for re-start */
	thrctx->thread_ref = NULL;

	return ret;
}

void auto_sleep_thread_signal(struct auto_sleep_thread *thrctx)
{
	if (!thrctx) {
		npu_warn("invalid thrctx\n");
		return;
	} else if (!thrctx->thread_ref) {
		npu_warn("invalid in thrctx->thread_ref\n");
		return;
	}

	npu_dbg("sending wakeup signal on ASThread (%s)...\n", thrctx->name);

	// Wake up tasks on the wait_queue
	wake_up_interruptible(&(thrctx->wq));
}

#ifndef CONFIG_NPU_KUNIT_TEST
void auto_sleep_thread_set_period(struct auto_sleep_thread *thrctx,
		unsigned int period)
{
	if (period == 0)
		thrctx->period = AUTO_SLEEP_THREAD_TIMEOUT;
	else
		thrctx->period = period;

	npu_dbg("set AST (%s) period as %d\n", thrctx->name, thrctx->period);

	/* immediately restart to re-set period */
	auto_sleep_thread_signal(thrctx);
}
#endif
