/****************************************************************************
 *
 * Copyright (c) 2014 - 2022 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __PLATFORM_MIF_H
#define __PLATFORM_MIF_H

#ifdef CONFIG_WLBT_REFACTORY
#ifdef CONFIG_SCSC_PCIE_CHIP
#include "pcie/platform_mif.h"
#else
#include "modap/platform_mif.h"
#endif

#else
#include "scsc_mif_abs.h"

enum wlbt_irqs {
       PLATFORM_MIF_MBOX,
#if defined(CONFIG_SOC_S5E5515) || defined(CONFIG_SOC_S5E8825) || defined(CONFIG_SOC_S5E8535) \
       || defined(CONFIG_SOC_S5E8835) || defined(CONFIG_SOC_S5E8845) || defined(CONFIG_SOC_S5E5535)
       PLATFORM_MIF_MBOX_WPAN,
#endif
       PLATFORM_MIF_ALIVE,
       PLATFORM_MIF_WDOG,
#if defined(CONFIG_SOC_EXYNOS9610) || defined(CONFIG_SOC_EXYNOS9630) \
	|| defined(CONFIG_SOC_EXYNOS3830) || defined(CONFIG_SOC_S5E9815) || \
	defined(CONFIG_SOC_S5E5515) || defined(CONFIG_SOC_S5E8825) || defined(CONFIG_SOC_S5E8535) \
       || defined(CONFIG_SOC_S5E8835) || defined(CONFIG_SOC_S5E8845) || defined(CONFIG_SOC_S5E5535)
       PLATFORM_MIF_CFG_REQ,
#endif
#if defined(CONFIG_WLBT_PMU2AP_MBOX)
       PLATFORM_MIF_MBOX_PMU,
#endif
       /* must be last */
       PLATFORM_MIF_NUM_IRQS
};

#if IS_ENABLED(CONFIG_EXYNOS_ITMON) || IS_ENABLED(CONFIG_EXYNOS_ITMON_V2)
#define ITMON_ERRCODE_DECERR       (1)
#endif

struct platform_device;

struct scsc_mif_abs    *platform_mif_create(struct platform_device *pdev);
void platform_mif_destroy_platform(struct platform_device *pdev, struct scsc_mif_abs *interface);
struct platform_device *platform_mif_get_platform_dev(struct scsc_mif_abs *interface);
struct device          *platform_mif_get_dev(struct scsc_mif_abs *interface);
int platform_mif_suspend(struct scsc_mif_abs *interface);
void platform_mif_resume(struct scsc_mif_abs *interface);
#if defined(CONFIG_SCSC_PCIE_CHIP)
int wlbt_rc_pm_notifier(struct notifier_block *nb, unsigned long mode, void *_unused);
#endif

#endif
#endif
