/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Samsung Exynos SoC series dsp driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 */

#ifndef __DL_DSP_PM_MANAGER_H__
#define __DL_DSP_PM_MANAGER_H__

#include "dsp-common.h"
#include "dsp-tlsf-allocator.h"
#include "dsp-lib-manager.h"

extern unsigned long dsp_pm_start_addr;

int dsp_pm_manager_init(unsigned long start_addr, size_t size,
	unsigned int pm_offset);
void dsp_pm_manager_free(void);
#ifdef CONFIG_NPU_KUNIT_TEST
#define dsp_pm_manager_print() do {} while (0)
#else
void dsp_pm_manager_print(void);
#endif

int dsp_pm_manager_alloc_libs(struct dsp_lib **libs, int libs_size,
	int *pm_inv);

int dsp_pm_alloc(size_t size, struct dsp_lib *lib, int *pm_inv);
void dsp_pm_free(struct dsp_lib *lib);

void dsp_pm_print(struct dsp_lib *lib);

#endif
