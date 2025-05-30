// SPDX-License-Identifier: GPL-2.0
/**
 * otg.c - DesignWare USB3 DRD Controller OTG
 *
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Authors: Ido Shayevitz <idos@codeaurora.org>
 *	    Anton Tikhomirov <av.tikhomirov@samsung.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_USB_DWC3_OTG_H
#define __LINUX_USB_DWC3_OTG_H
#include <linux/pm_wakeup.h>
#include <linux/usb/otg-fsm.h>
#include <linux/usb/otg.h>
#include <linux/pm_qos.h>
#include <soc/samsung/exynos_pm_qos.h>
#ifdef CONFIG_SND_EXYNOS_USB_AUDIO
#include "../../../sound/usb/exynos_usb_audio.h"
#endif
#include "dwc3-exynos.h"

#define CHG_CONNECTED_DELAY_TIME	(HZ * 20) /* 20s */
#define MAX_ENUM_RETRY_CNT		3
#define REMOVED_RETRY_CNT		99
#define MAX_PLATFORM_WAITING_CNT	40

#define CONNECTION_FAIL_CLEAR		0
#define CONNECTION_FAIL_SET		1
#define CONNECTION_FAIL_DUMP		2

struct dwc3_ext_otg_ops {
	int	(*setup)(struct device *dev, struct otg_fsm *fsm);
	void	(*exit)(struct device *dev);
	int	(*start) (struct device *dev);
	void	(*stop)(struct device *dev);
};

/**
 * struct dwc3_otg: OTG driver data. Shared by HCD and DCD.
 * @otg: USB OTG Transceiver structure.
 * @fsm: OTG Final State Machine.
 * @dwc: pointer to our controller context structure.
 * @irq: IRQ number assigned for HSUSB controller.
 * @regs: ioremapped register base address.
 * @wakelock: prevents the system from entering suspend while
 *		host or peripheral mode is active.
 * @vbus_reg: Vbus regulator.
 * @ready: is one when OTG is ready for operation.
 * @ext_otg_ops: external OTG engine ops.
 */
struct dwc3_otg {
	struct usb_otg          otg;
	struct otg_fsm		fsm;
	struct dwc3             *dwc;
	struct dwc3_exynos      *exynos;
	int                     irq;
	void __iomem            *regs;
	struct wakeup_source	*wakelock;
	struct wakeup_source	*reconn_wakelock;

	unsigned		ready:1;

	struct regulator	*vbus_reg;


	struct exynos_pm_qos_request	pm_qos_hsi0_req;
	int			pm_qos_hsi0_val;

	struct exynos_pm_qos_request	pm_qos_int_req;
	int			pm_qos_int_val;

	struct dwc3_ext_otg_ops *ext_otg_ops;
#if defined(CONFIG_OTG_DEFAULT)
	struct intf_typec	*typec;
	struct delayed_work	typec_work;
#endif
	struct notifier_block	pm_nb;
	struct completion	resume_cmpl;
	int			dwc3_suspended;
	int			fsm_reset;

	struct mutex lock;
	u8 combo_phy_control;
};

static inline int dwc3_ext_otg_setup(struct dwc3_otg *dotg)
{
	struct device *dev = dotg->exynos->dev;

	pr_info("%s, fsm = %8llx\n", __func__, (u64)&dotg->fsm);

	if (!dotg->ext_otg_ops->setup)
		return -EOPNOTSUPP;
	return dotg->ext_otg_ops->setup(dev, &dotg->fsm);
}

static inline int dwc3_ext_otg_exit(struct dwc3_otg *dotg)
{
	struct device *dev = dotg->exynos->dev;

	if (!dotg->ext_otg_ops->exit)
		return -EOPNOTSUPP;
	dotg->ext_otg_ops->exit(dev);
	return 0;
}

static inline int dwc3_ext_otg_start(struct dwc3_otg *dotg)
{
	struct device *dev = dotg->exynos->dev;

	pr_info("%s\n", __func__);

	if (!dotg->ext_otg_ops->start)
		return -EOPNOTSUPP;
	return dotg->ext_otg_ops->start(dev);
}

static inline int dwc3_ext_otg_stop(struct dwc3_otg *dotg)
{
	struct device *dev = dotg->exynos->dev;

	if (!dotg->ext_otg_ops->stop)
		return -EOPNOTSUPP;
	dotg->ext_otg_ops->stop(dev);
	return 0;
}

int dwc3_exynos_otg_init(struct dwc3 *dwc, struct dwc3_exynos *exynos);
void dwc3_exynos_otg_exit(struct dwc3 *dwc, struct dwc3_exynos *exynos);
int dwc3_otg_start(struct dwc3 *dwc, struct dwc3_exynos *exynos);
void dwc3_otg_stop(struct dwc3 *dwc, struct dwc3_exynos *exynos);

void usb_power_notify_control(int owner, int on);
extern void usb_dr_role_control(int on);

/* DP API */
int dwc3_exynos_otg_is_usb_ready(void);
int dwc3_exynos_otg_inform_dp_use(int use, int lane_cnt);

#ifdef CONFIG_SND_EXYNOS_USB_AUDIO
extern struct exynos_usb_audio *usb_audio;
extern int otg_connection;
//extern int usb_audio_connection;
#endif

extern void __iomem *phycon_base_addr;
extern int exynos_usbdrd_pipe3_enable(struct phy *phy);
extern int exynos_usbdrd_pipe3_disable(struct phy *phy);
extern void usb_power_notify_control(int owner, int on);

#endif /* __LINUX_USB_DWC3_OTG_H */
