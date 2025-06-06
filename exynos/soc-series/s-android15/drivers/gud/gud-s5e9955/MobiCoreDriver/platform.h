/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2013-2022 TRUSTONIC LIMITED
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
/*
 * Header file of MobiCore Driver Kernel Module Platform
 * specific structures
 *
 * Internal structures of the McDrvModule
 *
 * Header file the MobiCore Driver Kernel Module,
 * its internal structures and defines.
 */
#ifndef MC_DRV_PLATFORM_H
#define MC_DRV_PLATFORM_H

/* MobiCore Interrupt. */
#if defined(CONFIG_SOC_EXYNOS7870)
#define MC_INTR_SSIQ	255
/* Notify Interrupt cannot wakeup the core on this platform */
#define MC_DISABLE_IRQ_WAKEUP
#elif defined(CONFIG_SOC_EXYNOSAUTO9)
#define MC_DEVICE_PROPNAME "samsung,exynos-tee"
#endif

/* Force usage of xenbus_map_ring_valloc as of Linux v4.1 */
#define MC_XENBUS_MAP_RING_VALLOC_4_1

#if defined(CONFIG_SOC_EXYNOSAUTO9_EVT1)
/* Force core pin to #4 due to SIREX cache restriction */
#define PLAT_DEFAULT_TEE_AFFINITY_MASK 0x10
/*
 * Use PMEM allocator for PMDs and PTEs
 */
/* #define MC_USE_VLX_PMEM */
#endif

/* Probe TEE driver even if node not defined in Device Tree */
/* #define MC_PROBE_WITHOUT_DEVICE_TREE */

#endif /* MC_DRV_PLATFORM_H */
