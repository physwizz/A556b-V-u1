// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/moduleparam.h>
#include <linux/kthread.h>

#include "pablo-kunit-test.h"
#include "pablo-icpu-itf.h"
#include "icpu/pablo-icpu-selftest.h"

static const struct kernel_param_ops *__control_param_ops;
static const struct kernel_param_ops *__message_param_ops;

static struct task_struct *test_task_async;

/* Define the test cases. */

static void pablo_icpu_selftest_get_param_negative_kunit_test(struct kunit *test)
{
	int ret;

	ret = pablo_icpu_selftest_get_param_ops(NULL, NULL);
	KUNIT_ASSERT_EQ(test, ret, -EINVAL);
}

static void pablo_icpu_selftest_control_set_kunit_test(struct kunit *test)
{
	int ret;

	ret = __control_param_ops->set("target board", NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = __control_param_ops->set("scenario", NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* TODO: need to test boot on */

	ret = __control_param_ops->set("boot off", NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_icpu_selftest_control_set_negative_kunit_test(struct kunit *test)
{
	int ret;

	ret = __control_param_ops->set(NULL, NULL);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	ret = __control_param_ops->set("invalid", NULL);
	KUNIT_EXPECT_EQ(test, ret, -1);

	ret = __control_param_ops->set("target invalid", NULL);
	KUNIT_EXPECT_EQ(test, ret, -1);

	ret = __control_param_ops->set("msg all", NULL);
	KUNIT_EXPECT_EQ(test, ret, -1);
}

static void pablo_icpu_selftest_control_get_kunit_test(struct kunit *test)
{
	int ret;
	char *buf;

	ret = __control_param_ops->get(NULL, NULL);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	buf = kunit_kzalloc(test, 4 * 1024, 0);
	ret = __control_param_ops->get(buf, NULL);
	KUNIT_EXPECT_NE(test, ret, 0);

	kunit_kfree(test, buf);
}

static void pablo_icpu_selftest_message_set_kunit_test(struct kunit *test)
{
	int ret;

	ret = __message_param_ops->set("3[1,2,3]", NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = __message_param_ops->set("c", NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void pablo_icpu_selftest_message_set_negative_kunit_test(struct kunit *test)
{
	int ret;
	int i;

	ret = __message_param_ops->set(NULL, NULL);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	ret = __message_param_ops->set("invalid", NULL);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	for (i = 0; i < 64; i++) {
		ret = __message_param_ops->set("3[1,2,3]", NULL);
		KUNIT_EXPECT_EQ(test, ret, 0);
	}
	ret = __message_param_ops->set("3[1,2,3]", NULL);
	KUNIT_EXPECT_EQ(test, ret, -1);

	ret = __message_param_ops->set("c", NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);

	ret = __message_param_ops->set("17[1,2,3]", NULL);
	KUNIT_EXPECT_EQ(test, ret, -1);

	ret = __message_param_ops->set("3[a,2,3]", NULL);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	ret = __message_param_ops->set("3[b{},2,3]", NULL);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void pablo_icpu_selftest_message_get_kunit_test(struct kunit *test)
{
	int ret;
	char *buf;

	ret = __message_param_ops->get(NULL, NULL);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);

	buf = kunit_kzalloc(test, 4 * 1024, 0);
	ret = __message_param_ops->get(buf, NULL);
	KUNIT_EXPECT_NE(test, ret, 0);

	kunit_kfree(test, buf);
}

static void pablo_icpu_selftest_rsp_cb_kunit_test(struct kunit *test)
{
	u32 data[2] = { 1, 2, };

	pablo_icpu_selftest_rsp_cb(NULL, NULL, data);
}

static int __respose_task_func(void *data)
{
	icpu_itf_callback_func_t callback = data;
	u32 d[16] = { 1, 2, };

	if (callback)
		callback(NULL, NULL, d);

	return 0;
}

static void pablo_icpu_selftest_profile_kunit_test(struct kunit *test)
{
	int ret;

	ret = __control_param_ops->set("profile start 3[0x86,b[8096:],8096]", NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);

	if (test_task_async) {
		kthread_stop(test_task_async);
		test_task_async = NULL;
	}

	ret = __control_param_ops->set("profile stop 1[0x87]", NULL);
	KUNIT_EXPECT_EQ(test, ret, 0);

	if (test_task_async) {
		kthread_stop(test_task_async);
		test_task_async = NULL;
	}
}

static struct kunit_case pablo_icpu_selftest_kunit_test_cases[] = {
	KUNIT_CASE(pablo_icpu_selftest_get_param_negative_kunit_test),
	KUNIT_CASE(pablo_icpu_selftest_control_set_kunit_test),
	KUNIT_CASE(pablo_icpu_selftest_control_set_negative_kunit_test),
	KUNIT_CASE(pablo_icpu_selftest_control_get_kunit_test),
	KUNIT_CASE(pablo_icpu_selftest_message_set_kunit_test),
	KUNIT_CASE(pablo_icpu_selftest_message_set_negative_kunit_test),
	KUNIT_CASE(pablo_icpu_selftest_message_get_kunit_test),
	KUNIT_CASE(pablo_icpu_selftest_rsp_cb_kunit_test),
	KUNIT_CASE(pablo_icpu_selftest_profile_kunit_test),
	{},
};

static int __pablo_icpu_selftest_send_message_wrap(void *sender, void *cookie,
		icpu_itf_callback_func_t callback,
		enum icpu_msg_priority msg_prio, unsigned int num, u32 *data)
{
	if (callback)
		test_task_async = kthread_run(__respose_task_func,
				callback, "thread_rsp");

	return 0;
}

struct pablo_icpu_itf_api test_itf  = {
	.send_message = __pablo_icpu_selftest_send_message_wrap,
};

static int pablo_icpu_selftest_kunit_test_init(struct kunit *test)
{
	int ret;

	ret = pablo_icpu_selftest_get_param_ops(&__control_param_ops, &__message_param_ops);
	KUNIT_ASSERT_EQ(test, ret, 0);

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, __control_param_ops);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, __control_param_ops->set);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, __control_param_ops->get);

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, __message_param_ops);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, __message_param_ops->set);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, __message_param_ops->get);

	pablo_icpu_selftest_register_test_itf(&test_itf);

	return 0;
}

static void pablo_icpu_selftest_kunit_test_exit(struct kunit *test)
{
	__control_param_ops = NULL;
	__message_param_ops = NULL;
	pablo_icpu_selftest_register_test_itf(NULL);
}

struct kunit_suite pablo_icpu_selftest_kunit_test_suite = {
	.name = "pablo-icpu-selftest-kunit-test",
	.init = pablo_icpu_selftest_kunit_test_init,
	.exit = pablo_icpu_selftest_kunit_test_exit,
	.test_cases = pablo_icpu_selftest_kunit_test_cases,
};
define_pablo_kunit_test_suites(&pablo_icpu_selftest_kunit_test_suite);

MODULE_LICENSE("GPL");
