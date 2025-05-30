/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ALSA SoC - Samsung VTS driver
 *
 * Copyright (c) 2022 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_VTS_DEP_SOC_H
#define __SND_SOC_VTS_DEP_SOC_H

#define VTS_SOC_VERSION(m, n, r) (((m) << 16) | ((n) << 8) | (r))

#if (VTS_SOC_VERSION(5, 1, 0) <= CONFIG_SND_SOC_SAMSUNG_VTS_VERSION)
#include "vts_soc_v5_1.h"
#elif (VTS_SOC_VERSION(5, 0, 0) <= CONFIG_SND_SOC_SAMSUNG_VTS_VERSION)
#include "vts_soc_v5.h"
#elif (VTS_SOC_VERSION(4, 1, 0) <= CONFIG_SND_SOC_SAMSUNG_VTS_VERSION)
#include "vts_soc_v4_1.h"
#elif (VTS_SOC_VERSION(4, 0, 0) <= CONFIG_SND_SOC_SAMSUNG_VTS_VERSION)
#include "vts_soc_v4.h"
#elif (VTS_SOC_VERSION(3, 1, 0) <= CONFIG_SND_SOC_SAMSUNG_VTS_VERSION)
#include "vts_soc_v3_1.h"
#elif (VTS_SOC_VERSION(3, 0, 0) <= CONFIG_SND_SOC_SAMSUNG_VTS_VERSION)
#include "vts_soc_v3.h"
#else
#include "vts_soc_v3.h"
#endif

#define VTS_PCM_BUF_SZ (96000)

#include "vts.h"

/* interface functions */
void vts_cpu_power(struct device *dev, bool on);
int vts_cpu_power_chk(struct device *dev);
int vts_cpu_enable(struct device *dev, bool enable);
void vts_reset_cpu(struct device *dev);
void vts_disable_fw_ctrl(struct device *dev);

void vts_pad_retention(bool retention);
u32 vts_set_baaw(void __iomem *sfr_base, u64 base, u32 size);

int vts_soc_runtime_resume(struct device *dev);
int vts_soc_runtime_suspend(struct device *dev);

int vts_soc_cmpnt_probe(struct device *dev);
int vts_soc_probe(struct device *dev);
void vts_soc_remove(struct device *dev);

#if defined(CONFIG_VTS_SFR_DUMP)
ssize_t vts_soc_sfr_dump_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t vts_soc_dmic_if_dump_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t vts_soc_slif_dump_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t vts_soc_gpio_dump_show(struct device *dev, struct device_attribute *attr, char *buf);
#endif
#endif /* __SND_SOC_VTS_DEP_SOC_H */
