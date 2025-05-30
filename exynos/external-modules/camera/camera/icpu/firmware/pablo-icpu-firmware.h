/* SPDX-License-Identifier: GPL */
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

#ifndef PABLO_ICPU_FIRMWARE_H
#define PABLO_ICPU_FIRMWARE_H

void *icpu_firmware_get_buf_bin(void);
void *icpu_firmware_get_buf_heap(bool secure_mode);
void *icpu_firmware_get_buf_log(void);
void *icpu_firmware_get_buf_dbg(void);
bool icpu_firmware_get_aarch64(void);
int is_firmware_loaded(void);
int load_firmware(void *dev, bool secure_mode);
int preload_firmware(void *dev, unsigned long flag);
void teardown_firmware(void);
int icpu_firmware_probe(void *dev);
void icpu_firmware_remove(void);

#endif
