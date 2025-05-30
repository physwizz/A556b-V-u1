// SPDX-License-Identifier: GPL-2.0
/*
 * tracepoint hook handling
 *
 * Copyright (C) 2021 Samsung Electronics Co., Ltd
 *
 */

#include <linux/poll.h>
#include <net/sock.h>
#include <trace/hooks/sched.h>

#define TASK_VENDOR		0x2000
#ifndef WF_ANDROID_VENDOR
/* Copied from kernel/sched/sched.h */
#define WF_ANDROID_VENDOR	0x1000
#endif

/******************************************************************************
 * tracepoint of Android vendor hook                                          *
 ******************************************************************************/
static void cpif_hook_do_wake_up_sync(void *data,
		struct wait_queue_head *wq_head, int *done, struct sock *sk)
{
#ifdef WF_ANDROID_VENDOR
	if (sk->sk_family == AF_INET6 || sk->sk_family == AF_INET) {
		*done = 1;
		__wake_up_sync_key(wq_head, TASK_INTERRUPTIBLE | TASK_VENDOR,
			poll_to_key(EPOLLIN | EPOLLPRI | EPOLLRDNORM | EPOLLRDBAND));
	}
#endif
}

static void cpif_hook_set_wake_flags(void *data,
		int *wake_flags, unsigned int *mode)
{
#ifdef WF_ANDROID_VENDOR
	if (*mode & TASK_VENDOR) {
		*mode &= ~TASK_VENDOR;
		*wake_flags = WF_ANDROID_VENDOR;
	}
#endif
}

int hook_init(void)
{
	int ret;

	ret = register_trace_android_vh_do_wake_up_sync(cpif_hook_do_wake_up_sync, NULL);
	if (ret)
		return ret;

	ret = register_trace_android_vh_set_wake_flags(cpif_hook_set_wake_flags, NULL);
	if (ret)
		return ret;

	return 0;
}
EXPORT_SYMBOL(hook_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung CPIF vendor hook driver");
