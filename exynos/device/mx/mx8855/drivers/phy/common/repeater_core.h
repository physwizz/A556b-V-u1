// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2021 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung USB repeater driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __USB_REPEATER_H__
#define __USB_REPEATER_H__

#include <linux/device.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

struct repeater_data {
	int (*enable)(void *data, bool en);
	int (*set_bypass)(void *data, bool enable);
	void *private_data;
};

extern int repeater_enable(bool en);
extern int eusb_repeater_set_bypass(bool en);
extern int repeater_core_register(struct repeater_data *data);
extern void repeater_core_unregister(void);
#if	IS_ENABLED(CONFIG_PHY_SUPPORT_SEC_REPEATER_CB)
extern int register_repeater_enable(int (*repeater_enable) (bool en));
#endif

#endif	/* __USB_REPEATER_H__ */
