/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ALSA SoC - Samsung Abox Core driver
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_CORE_H
#define __SND_SOC_ABOX_CORE_H

/**
 * abox cores status check
 */
extern bool abox_core_status(void);

/**
 * flush caches for abox cores
 */
extern void abox_core_flush(void);

/**
 * power abox cores on or off
 * @param[in]	on	1 for on, 0 for off
 */
extern void abox_core_power(int on);

/**
 * enable abox cores
 * @param[in]	enable	1 for enable, 0 for disable
 */
extern void abox_core_enable(int enable);

/**
 * wait for standby
 * @return		0 or error code
 */
extern int abox_core_standby(void);

/**
 * print gpr values from gpr dump sfr to kernel log
 */
extern void abox_core_print_gpr(void);

/**
 * print gpr values from dump to kernel log
 * @param[in]	dump	address of gpr dump
 */
extern void abox_core_print_gpr_dump(unsigned int *dump);

/**
 * print gpr values from dump to kernel log
 * @param[in]	tgt	address of target buffer
 */
extern void abox_core_dump_gpr(unsigned int *tgt);

/**
 * print gpr values from dump to kernel log
 * @param[in]	tgt	address of target buffer
 * @param[in]	dump	address of gpr dump
 */
extern void abox_core_dump_gpr_dump(unsigned int *tgt, unsigned int *dump);

/**
 * print gpr values from gpr dump sfr to buffer
 * @param[in]	buf	buffer to print gpr
 * @return		number of characters written into @buf
 */
extern int abox_core_show_gpr(char *buf);

/**
 * download firmware
 * @return		error code
 */
extern int abox_core_download_firmware(void);

/**
 * change firmware
 */
extern void abox_core_change_firmware(void);

#endif /* __SND_SOC_ABOX_CORE_H */
