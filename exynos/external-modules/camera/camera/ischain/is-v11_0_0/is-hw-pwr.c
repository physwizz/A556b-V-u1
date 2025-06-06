// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "is-config.h"
#include "is-hw.h"

/* sensor */
int is_sensor_runtime_resume_pre(struct device *dev)
{
	return 0;
}

int is_sensor_runtime_suspend_pre(struct device *dev)
{
	return 0;
}

int is_ischain_runtime_resume_post(struct device *dev)
{
	struct is_core *core = (struct is_core *)dev_get_drvdata(dev);

	info("%s\n", __func__);

	is_hw_ischain_enable(core);

	return 0;
}

int is_ischain_runtime_suspend_post(struct device *dev)
{
	info("%s\n", __func__);

	return 0;
}

int is_runtime_suspend_post(struct device *dev)
{
	return 0;
}
