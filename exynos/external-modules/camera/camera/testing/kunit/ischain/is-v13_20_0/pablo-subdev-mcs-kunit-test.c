// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "pablo-kunit-test.h"
#include "pablo-kunit-test-utils.h"
#include "votf/pablo-votf.h"
#include "is-core.h"
#include "is-hw-chain.h"
#include "pablo-lib.h"

extern struct pablo_kunit_subdev_mcs_func *pablo_kunit_get_subdev_mcs_func(void);
static struct pablo_kunit_subdev_mcs_func *fn;
static struct is_device_ischain *idi;
static struct is_frame *frame;

static struct pablo_kunit_test_ctx {
	struct votf_dev *votf_dev_mock[VOTF_DEV_NUM];
	struct votf_dev *votf_dev[VOTF_DEV_NUM];
} pkt_ctx;

static void pablo_subdev_mcs_kunit_test_init_votfdev(struct kunit *test)
{
	struct votf_dev *dev, *org_dev;
	u32 dev_id, ip;
	void *votf_addr;

	for (dev_id = 0; dev_id < VOTF_DEV_NUM; dev_id++) {
		org_dev = get_votf_dev(dev_id);
		if (!org_dev)
			continue;

		/* Copy original dev infos */
		dev = kunit_kzalloc(test, sizeof(struct votf_dev), 0);
		KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);
		memcpy(dev, org_dev, sizeof(struct votf_dev));

		/* Set dummy VOTF addr region */
		for (ip = 0; ip < IP_MAX; ip++) {
			votf_addr = kunit_kzalloc(test, 0x10000, 0);
			KUNIT_ASSERT_NOT_ERR_OR_NULL(test, votf_addr);
			dev->votf_addr[ip] = votf_addr;
		}


		pkt_ctx.votf_dev[dev_id] = org_dev;
		pkt_ctx.votf_dev_mock[dev_id] = dev;

		set_votf_dev(dev_id, dev);
	}
}

static void pablo_subdev_mcs_kunit_test_deinit_votfdev(struct kunit *test)
{
	struct votf_dev *dev;
	u32 dev_id, ip;
	void *votf_addr;

	for (dev_id = 0; dev_id < VOTF_DEV_NUM; dev_id++) {
		dev = pkt_ctx.votf_dev_mock[dev_id];
		if (!dev)
			continue;

		for (ip = 0; ip < IP_MAX; ip++) {
			votf_addr = dev->votf_addr[ip];
			if (!votf_addr)
				continue;

			kunit_kfree(test, votf_addr);
			dev->votf_addr[ip] = NULL;
		}

		set_votf_dev(dev_id, pkt_ctx.votf_dev[dev_id]);
		kunit_kfree(test, pkt_ctx.votf_dev_mock[dev_id]);

		pkt_ctx.votf_dev[dev_id] = NULL;
		pkt_ctx.votf_dev_mock[dev_id] = NULL;
	}
}

static int pablo_subdev_mcs_kunit_test_init(struct kunit *test)
{
	fn = pablo_kunit_get_subdev_mcs_func();
	idi = kunit_kzalloc(test, sizeof(struct is_device_ischain), 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, idi);

	idi->is_region = kunit_kzalloc(test, sizeof(struct is_region), 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, idi->is_region);

	frame = kunit_kzalloc(test, sizeof(struct is_frame), 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, frame);

	frame->parameter = kunit_kzalloc(test, sizeof(struct is_param_region), 0);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, frame->parameter);

	pablo_subdev_mcs_kunit_test_init_votfdev(test);

	return 0;
}

static void pablo_subdev_mcs_kunit_test_exit(struct kunit *test)
{
	pablo_subdev_mcs_kunit_test_deinit_votfdev(test);
	kunit_kfree(test, frame->parameter);
	kunit_kfree(test, frame);
	kunit_kfree(test, idi->is_region);
	kunit_kfree(test, idi);
}

static struct kunit_case pablo_subdev_mcs_kunit_test_cases[] = {
	{},
};

struct kunit_suite pablo_subdev_mcs_kunit_test_suite = {
	.name = "pablo-subdev-mcs-kunit-test",
	.init = pablo_subdev_mcs_kunit_test_init,
	.exit = pablo_subdev_mcs_kunit_test_exit,
	.test_cases = pablo_subdev_mcs_kunit_test_cases,
};
define_pablo_kunit_test_suites(&pablo_subdev_mcs_kunit_test_suite);

MODULE_LICENSE("GPL v2");
