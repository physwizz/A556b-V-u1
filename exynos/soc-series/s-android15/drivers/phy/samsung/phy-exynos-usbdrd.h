/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *        http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PHY_EXYNOS_USBDRD_H__
#define __PHY_EXYNOS_USBDRD_H__

#include "phy-samsung-usb-cal.h"
#include "phy-exynos-usb3p1.h"
#include "phy-exynos-usbdp-gen2.h"
#include "phy-exynos-usbdp-gen2-v4.h"

#define EXYNOS_USBPHY_VER_02_0_0	0x0200	/* Lhotse - USBDP Combo PHY */

/* 9810 PMU register offset */
#define EXYNOS_USBDP_PHY_CONTROL	(0x704)
#define EXYNOS_USB2_PHY_CONTROL	(0x72C)
/* PMU register offset for USB */
#define EXYNOS_USBDEV_PHY_CONTROL	(0x704)
#define EXYNOS_USBDRD_ENABLE		BIT(0)
#define EXYNOS_USBHOST_ENABLE		BIT(1)

/* USB dwc3 driver uses 0 to 0x20 */
#define PHY_MODE_BUS_SUSPEND		0x30
#define PHY_MODE_BUS_RESUME		0x31
#define PHY_MODE_ABOX_POWER		0x32
#define PHY_MODE_CALL_ENTER		0x33
#define PHY_MODE_CALL_EXIT		0x34
#define PHY_MODE_SUSPEND_BYPASS		0x35
#define PHY_MODE_RESUME_BYPASS		0x36

/* Exynos USB PHY registers */
#define EXYNOS_FSEL_9MHZ6		0x0
#define EXYNOS_FSEL_10MHZ		0x1
#define EXYNOS_FSEL_12MHZ		0x2
#define EXYNOS_FSEL_19MHZ2		0x1
#define EXYNOS_FSEL_20MHZ		0x4
#define EXYNOS_FSEL_24MHZ		0x5
#define EXYNOS_FSEL_26MHZ		0x82
#define EXYNOS_FSEL_50MHZ		0x7

/* EXYNOS: USB DRD PHY registers */
#define EXYNOS_DRD_LINKSYSTEM			0x04

#define LINKSYSTEM_FLADJ_MASK			(0x3f << 1)
#define LINKSYSTEM_FLADJ(_x)			((_x) << 1)

#define EXYNOS_DRD_PHYUTMI			0x08

#define EXYNOS_DRD_PHYPIPE			0x0c

#define PHYPIPE_PHY_CLOCK_SEL				(0x1 << 4)

#define EXYNOS_DRD_PHYCLKRST			0x10

#define PHYCLKRST_SSC_REFCLKSEL_MASK		(0xff << 23)
#define PHYCLKRST_SSC_REFCLKSEL(_x)		((_x) << 23)

#define PHYCLKRST_SSC_RANGE_MASK		(0x03 << 21)
#define PHYCLKRST_SSC_RANGE(_x)			((_x) << 21)

#define PHYCLKRST_MPLL_MULTIPLIER_MASK		(0x7f << 11)
#define PHYCLKRST_MPLL_MULTIPLIER_100MHZ_REF	(0x19 << 11)
#define PHYCLKRST_MPLL_MULTIPLIER_50M_REF	(0x32 << 11)
#define PHYCLKRST_MPLL_MULTIPLIER_24MHZ_REF	(0x68 << 11)
#define PHYCLKRST_MPLL_MULTIPLIER_20MHZ_REF	(0x7d << 11)
#define PHYCLKRST_MPLL_MULTIPLIER_19200KHZ_REF	(0x02 << 11)

#define PHYCLKRST_FSEL_UTMI_MASK		(0x7 << 5)
#define PHYCLKRST_FSEL_PIPE_MASK		(0x7 << 8)
#define PHYCLKRST_FSEL(_x)			((_x) << 5)
#define PHYCLKRST_FSEL_PAD_100MHZ		(0x27 << 5)
#define PHYCLKRST_FSEL_PAD_24MHZ		(0x2a << 5)
#define PHYCLKRST_FSEL_PAD_20MHZ		(0x31 << 5)
#define PHYCLKRST_FSEL_PAD_19_2MHZ		(0x38 << 5)

#define PHYCLKRST_REFCLKSEL_MASK		(0x03 << 2)
#define PHYCLKRST_REFCLKSEL_PAD_REFCLK		(0x2 << 2)
#define PHYCLKRST_REFCLKSEL_EXT_REFCLK		(0x3 << 2)

#define EXYNOS_DRD_PHYREG0			0x14
#define EXYNOS_DRD_PHYREG1			0x18

#define EXYNOS_DRD_PHYPARAM0			0x1c

#define PHYPARAM0_REF_LOSLEVEL_MASK		(0x1f << 26)
#define PHYPARAM0_REF_LOSLEVEL			(0x9 << 26)

#define EXYNOS_DRD_PHYPARAM1			0x20

#define PHYPARAM1_PCS_TXDEEMPH_MASK		(0x1f << 0)
#define PHYPARAM1_PCS_TXDEEMPH			(0x1c)

#define EXYNOS_DRD_PHYTERM			0x24

#define EXYNOS_DRD_PHYTEST			0x28

#define EXYNOS_DRD_PHYADP			0x2c

#define EXYNOS_DRD_PHYUTMICLKSEL		0x30

#define PHYUTMICLKSEL_UTMI_CLKSEL		BIT(2)

#define EXYNOS_DRD_PHYRESUME			0x34
#define EXYNOS_DRD_LINKPORT			0x44

#define KHZ	1000
#define MHZ	(KHZ * KHZ)

#define EXYNOS_DRD_MAX_TUNEPARAM_NUM		32

enum exynos_usbdrd_phy_id {
	EXYNOS_DRDPHY_UTMI,
	EXYNOS_DRDPHY_PIPE3,
	EXYNOS_DRDPHYS_NUM,
};

struct phy_usb_instance;
struct exynos_usbdrd_phy;

struct exynos_usbdrd_phy_config {
	u32 id;
	void (*phy_isol)(struct phy_usb_instance *inst, u32 on, unsigned int);
	void (*phy_init)(struct exynos_usbdrd_phy *phy_drd);
	void (*phy_exit)(struct exynos_usbdrd_phy *phy_drd);
	void (*phy_tune)(struct exynos_usbdrd_phy *phy_drd, int);
	int (*phy_vendor_set)(struct exynos_usbdrd_phy *phy_drd, int, int);
	void (*phy_ilbk)(struct exynos_usbdrd_phy *phy_drd);
	void (*phy_set)(struct exynos_usbdrd_phy *phy_drd, int, void *);
	int (*set_refclk)(struct phy_usb_instance *inst);
};

struct exynos_usbdrd_phy_drvdata {
	const struct exynos_usbdrd_phy_config *phy_cfg;
};

/**
 * struct exynos_usbdrd_phy - driver data for USB DRD PHY
 * @dev: pointer to device instance of this platform device
 * @reg_phy: usb phy controller register memory base
 * @clk: phy clock for register access
 * @drv_data: pointer to SoC level driver data structure
 * @phys[]: array for 'EXYNOS_DRDPHYS_NUM' number of PHY
 *	    instances each with its 'phy' and 'phy_cfg'.
 * @extrefclk: frequency select settings when using 'separate
 *	       reference clocks' for SS and HS operations
 * @ref_clk: reference clock to PHY block from which PHY's
 *	     operational clocks are derived
 * @usbphy_info; Phy main control info
 * @usbphy_sub_info; USB3.0 phy control info
 */
struct exynos_usbdrd_phy {
	struct device *dev;
	void __iomem *reg_phy;
	void __iomem *reg_phy2;
	void __iomem *reg_phy3;
	void __iomem *reg_link;
	void __iomem *reg_eusb_ctrl;
	void __iomem *reg_eusb_phy;
	void __iomem *reg_dpphy_ctrl;
	void __iomem *reg_dpphy_tca;
	struct clk **clocks;
	struct clk **phy_clocks;
	const struct exynos_usbdrd_phy_drvdata *drv_data;
	struct phy_usb_instance {
		struct phy *phy;
		u32 index;
		struct regmap *reg_pmu;
		u32 pmu_offset;
		u32 pmu_offset_dp;
		u32 pmu_mask;
		u32 pmu_offset_tcxobuf;
		u32 pmu_mask_tcxobuf;
		const struct exynos_usbdrd_phy_config *phy_cfg;
	} phys[EXYNOS_DRDPHYS_NUM];
	u32 extrefclk;
	bool use_phy_umux;
	struct clk *ref_clk;
	struct regulator *vbus;
	struct regulator	*vdd075_usb;
	struct regulator	*vdd12_usb;
	struct regulator	*vdd18_usb;
	struct regulator	*vdd33_usb;
	struct exynos_usbphy_info usbphy_info;
	struct exynos_usbphy_info usbphy_sub_info;
	struct exynos_usbphy_info usbphy_blkcon_info;
	struct exynos_usbphy_ss_tune ss_value[2];
	struct exynos_usbphy_hs_tune hs_value[2];
	int hs_tune_param_value[EXYNOS_DRD_MAX_TUNEPARAM_NUM][2];
	int ss_tune_param_value[EXYNOS_DRD_MAX_TUNEPARAM_NUM][2];

	u32 ip_type;
#if IS_ENABLED(CONFIG_EXYNOS_OTP)
#define OTP_SUPPORT_USBPHY_NUMBER	2
#define OTP_USB3PHY_INDEX		0
#define OTP_USB2PHY_INDEX		1
	u8 otp_type[OTP_SUPPORT_USBPHY_NUMBER];
	u8 otp_index[OTP_SUPPORT_USBPHY_NUMBER];
	struct tune_bits *otp_data[OTP_SUPPORT_USBPHY_NUMBER];
#endif
	int irq_wakeup;
	int usb3_irq_wakeup;
	int irq_conn;
	int is_conn;
	int is_irq_enabled;
	int idle_ip_idx;
	struct notifier_block	pm_nb;
	struct completion	resume_cmpl;
	int			phy_usbdrd_suspended;
	u32 phy_port;
	u32 reverse_phy_port;
	spinlock_t lock;
	struct mutex mutex;
	u32 use_default_tune_val;
	int in_shutdown;
	int is_ldo_on;
	int phy_port_test_en;
#if defined(CONFIG_OTG_CDP_SUPPORT)
	int cdp_check;
#endif
};

void __iomem *phy_exynos_usbdp_get_address(void);

enum samsung_cpu_type {
	TYPE_S3C64XX,
	TYPE_EXYNOS4210,
	TYPE_EXYNOS5250,
	TYPE_EXYNOS5430,
	TYPE_EXYNOS7420,
	TYPE_EXYNOS7580,
	TYPE_EXYNOS8890,
	TYPE_EXYNOS8895,
};

enum samsung_usb_ip_type {
	TYPE_USB3DRD = 0,
	TYPE_USB3HOST,
	TYPE_USB2DRD,
	TYPE_USB2HOST,
};

enum samsung_phy_set_option {
	SET_DPPULLUP_ENABLE,
	SET_DPPULLUP_DISABLE,
	SET_DPDM_PULLDOWN,
};

void exynos_usb_pm_noti_init(struct exynos_usbdrd_phy *phy_drd);
#if IS_ENABLED(CONFIG_PHY_EXYNOS_EUSB_REPEATER)
extern int eusb_repeater_power_on(void);
extern int eusb_repeater_power_off(void);
#endif
extern int get_idle_ip_index(void);
extern int exynos_usb_scenario_info(void);
#endif	/* __PHY_EXYNOS_USBDRD_H__ */
