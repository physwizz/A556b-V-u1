/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_INTERFACE_ISCHAIN_H
#define IS_INTERFACE_ISCHAIN_H
#include "is-resourcemgr.h"
#include "is-hw-config.h"
#define IRQ_NAME_LENGTH 	16

struct is_hardware;

typedef int (*hwip_handler)(u32 id, void *context);

struct hwip_intr_handler {
	u32 valid;
	u32 priority;
	u32 id;
	void *ctx;
	hwip_handler handler;
	u32 chain_id;
};

struct is_interface_hwip {
	int				id;

	/* interrupt */
	int				irq[INTR_HWIP_MAX];
	char				irq_name[INTR_HWIP_MAX][IRQ_NAME_LENGTH];
	struct hwip_intr_handler	handler[INTR_HWIP_MAX];

	struct is_hw_ip		*hw_ip;
};

/**
 * struct is_interface_ischain - Sub IPs in ischain interrupt interface structure
 * @state: is chain interface state
 */
struct is_interface_ischain {
	struct is_interface_hwip	itf_ip[HW_SLOT_MAX];

	struct is_minfo		*minfo;
};

int is_interface_ischain_probe(struct is_interface_ischain *this,
	struct is_hardware *hardware, struct platform_device *pdev);
void is_interface_ischain_remove(struct is_interface_ischain *this,
	struct is_hardware *hardware);
#endif
