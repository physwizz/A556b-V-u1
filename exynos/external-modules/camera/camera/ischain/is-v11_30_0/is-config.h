// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_CONFIG_H
#define IS_CONFIG_H

#include <linux/version.h>
#include "is-common-config.h"
#include "is-vendor-config.h"

/*
 * =================================================================================================
 * CONFIG - GLOBAL OPTIONS
 * =================================================================================================
 */
#define IS_SENSOR_COUNT		6
#define IS_STREAM_COUNT		10

/*
 * =================================================================================================
 * CONFIG - Chain IP configurations
 * =================================================================================================
 */
/* Sensor VC */
#define SOC_SSVC0	1
#define SOC_SSVC1	1
#define SOC_SSVC2	1
#define SOC_SSVC3	1

/* CSTAT */
#define SOC_CSTAT0		1
#define SOC_CSTAT1		1
#define SOC_CSTAT2		1
#define SOC_CSTAT3		1
#define SOC_CSTAT_SVHIST	0

/* BYRP */
#define SOC_BYRP	1

/* RGBP */
#define SOC_RGBP	1

/* MCFP */
#define SOC_MCFP	1

/* YUVP */
#define SOC_YUVP	1

/* MCSC */
#define SOC_MCS		1

/* LME */
#define SOC_LME0	1

#define CSIS_COMP_BLOCK_WIDTH		256
#define CSIS_COMP_BLOCK_HEIGHT		1

#define CSTAT_COMP_BLOCK_WIDTH		256
#define CSTAT_COMP_BLOCK_HEIGHT		1

#define BYRP_COMP_BLOCK_WIDTH		256
#define BYRP_COMP_BLOCK_HEIGHT		1

#define RGBP_COMP_BLOCK_WIDTH		32
#define RGBP_COMP_BLOCK_HEIGHT		4

#define MCFP_COMP_BLOCK_WIDTH		32
#define MCFP_COMP_BLOCK_HEIGHT		4

#define YUVP_COMP_BLOCK_WIDTH		32
#define YUVP_COMP_BLOCK_HEIGHT		4

#define MCSC_COMP_BLOCK_WIDTH		32
#define MCSC_COMP_BLOCK_HEIGHT		4
#define MCSC_HF_COMP_BLOCK_WIDTH	256
#define MCSC_HF_COMP_BLOCK_HEIGHT	1

#define GDC_COMP_BLOCK_WIDTH		32
#define GDC_COMP_BLOCK_HEIGHT		4

#define COMP_LOSSYBYTE32NUM_32X4_U8	2
#define COMP_LOSSYBYTE32NUM_32X4_U10	2
#define COMP_LOSSYBYTE32NUM_32X4_U12	4
#define COMP_LOSSYBYTE32NUM_256X1_U10	5
#define COMP_LOSSYBYTE32NUM_256X1_U12	9

#define DEV_HW_ISP0_ID		DEV_HW_BYRP

#define CSIS_PIXEL_PER_CLK		CSIS_PPC_4
#define CSIS_OTF_CH_LC_NUM 4

/*
 * =================================================================================================
 * CONFIG - SW configurations
 * =================================================================================================
 */
#define CHAIN_STRIPE_PROCESSING	1
#define MCFP_WIDTH_ALIGN		128
#define STRIPE_MARGIN_WIDTH		768
#define STRIPE_MCSC_MARGIN_WIDTH	512
#define STRIPE_WIDTH_ALIGN		512
#define STRIPE_RATIO_PRECISION		1000
#define ENABLE_LMEDS
#define ENABLE_LMEDS1
#define ENABLE_BYRP_HDR
#define ENABLE_10BIT_MCSC
#define ENABLE_DJAG_IN_MCSC
#define ENABLE_MCSC_CENTER_CROP
#define USE_MCSC_STRIP_OUT_CROP		/* use for MCSC stripe */
#define USE_DJAG_COEFF_CONFIG		0
#define USE_MCSC_H_V_COEFF		/* support different u,v coefficient */
#define LME_SAD_DATA_SIZE		2 /* byte */
#define DDK_INTERFACE_VER		0x1010
#define DISABLE_MCFP_MV_MIXER	1

#define USE_ONE_BINARY
#define USE_RTA_BINARY
#define DISABLE_DDK_HEAP_FREE	1
#define USE_BINARY_PADDING_DATA_ADDED	/* for DDK signature */
#define USE_DDK_SHUT_DOWN_FUNC
#define ENABLE_IRQ_MULTI_TARGET
#define USE_DVA_36BIT		0
/* #define ENABLE_MODECHANGE_CAPTURE */

#define LOGICAL_VIDEO_NODE	1
/* #define CONFIG_SENSOR_GROUP_LVN	1 */
/* #define ENABLE_SKIP_PER_FRAME_MAP */
#define CRTA_CALL		1
#define IRTA_CALL		1

#define CAMERA_2LD_MIRROR_FLIP	1

#define SUPPORT_DJAG_PRE_FILTER 0

/*
 * PDP0: RDMA
 * PDP1: RDMA
 * PDP2: RDMA
 * 3AA0: 3AA0, ZSL/STRIP DMA0, MEIP0
 * 3AA1: 3AA1, ZSL/STRIP DMA1, MEIP1
 * 3AA2: 3AA2, ZSL/STRIP DMA2, MEIP2
 * ITP0: TNR, ITP0, DNS0
 * MCSC0:
 *
 */
#define HW_SLOT_MAX            (16)
/* #define SKIP_SETFILE_LOAD */
#define SKIP_LIB_LOAD			1
#define IS_MAX_MCSC_SETFILE 128

#define DYNAMIC_HEAP_FOR_DDK_RTA	1

/* FIMC-IS task priority setting */
#define TASK_SENSOR_WORK_PRIO		(IS_MAX_PRIO - 48) /* 52 */
#define TASK_GRP_OTF_INPUT_PRIO		(IS_MAX_PRIO - 49) /* 51 */
#define TASK_GRP_DMA_INPUT_PRIO		(IS_MAX_PRIO - 50) /* 50 */
#define TASK_MSHOT_WORK_PRIO		(IS_MAX_PRIO - 43) /* 57 */
#define TASK_LIB_OTF_PRIO		(IS_MAX_PRIO - 44) /* 56 */
#define TASK_LIB_AF_PRIO		(IS_MAX_PRIO - 45) /* 55 */
#define TASK_LIB_ISP_DMA_PRIO		(IS_MAX_PRIO - 46) /* 54 */
#define TASK_LIB_3AA_DMA_PRIO		(IS_MAX_PRIO - 47) /* 53 */
#define TASK_LIB_AA_PRIO		(IS_MAX_PRIO - 48) /* 52 */
#define TASK_LIB_RTA_PRIO		(IS_MAX_PRIO - 49) /* 51 */

/* EXTRA chain for 3AA and ITP
 * Each IP has 4 slot
 * 3AA: 0~3	ITP: 0~3
 * MEIP: 4~7	MCFP0: 4~7
 * DMA: 8~11	DNS: 8~11
 * LME: 8~11	MCFP1: 8~11
 */
#define LIC_CHAIN_NUM		(2)
#define LIC_CHAIN_OFFSET_NUM	(8)

/* 0x0: 32B_ALIGN, 0x4: 64B_ALIGN */
#define SBWC_BASE_ALIGN_MASK_LLC_OFF	(0x0)
#define SBWC_BASE_ALIGN_MASK		(0x4)

#if IS_ENABLED(CLOG_RESERVED_MEM)
#undef CLOG_RESERVED_MEM
#endif

/*
 * =================================================================================================
 * CONFIG - FEATURE ENABLE
 * =================================================================================================
 */
#define IS_MAX_TASK		(40)

#define HW_TIMEOUT_PANIC_ENABLE
#ifdef HW_TIMEOUT_PANIC_ENABLE
#define CHECK_TIMEOUT_PANIC_HW(id)	((id == DEV_HW_BYRP) || \
				(id == DEV_HW_RGBP) || \
				(id == DEV_HW_MCFP) || \
				(id == DEV_HW_YPP) || \
				(id == DEV_HW_LME0))
#endif
#if defined(CONFIG_ARM_EXYNOS_DEVFREQ)
#define CONFIG_IS_BUS_DEVFREQ
#endif

#if IS_ENABLED(CONFIG_PANIC_ON_COTF_ERR)
#define DDK_OVERFLOW_RECOVERY		(0)	/* 0: do not execute recovery, 1: execute recovery */
#else
#define DDK_OVERFLOW_RECOVERY		(1)	/* 0: do not execute recovery, 1: execute recovery */
#endif

#define CAPTURE_NODE_MAX		17
#define OTF_YUV_FORMAT			(OTF_INPUT_FORMAT_YUV422)
#define MSB_OF_3AA_DMA_OUT		(11)
#define MSB_OF_DNG_DMA_OUT		(9)
/* #define USE_YUV_RANGE_BY_ISP */

/* #define ENABLE_HWFC */
#define USE_IXC_LOCK
#define SENSOR_REQUEST_DELAY		2

#ifdef ENABLE_IRQ_MULTI_TARGET
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
#define IS_HW_IRQ_FLAG     0
#else
#define IS_HW_IRQ_FLAG     IRQF_GIC_MULTI_TARGET
#endif
#define IRQ_MULTI_TARGET_CL0 1
#else
#define IS_HW_IRQ_FLAG     0
#endif

/* #define ENABLE_EARLY_SHOT */
#define SUPPORT_HW_FRO(id)	(((id >= DEV_HW_PAF0) && \
				(id <= DEV_HW_PAF3)) || \
				(id == DEV_HW_BYRP))
#define FULL_OTF_TAIL_GROUP_ID		GROUP_ID_MCS0

#define SKIP_ISP_SHOT_FOR_MULTI_SHOT	1

#if defined(USE_IXC_LOCK)
#define IXC_MUTEX_LOCK(lock)	\
	({if (!in_atomic())	\
		mutex_lock(lock);})
#define IXC_MUTEX_UNLOCK(lock)	\
	({if (!in_atomic())	\
		mutex_unlock(lock);})
#else
#define IXC_MUTEX_LOCK(lock)
#define IXC_MUTEX_UNLOCK(lock)
#endif

/* #define ENABLE_DBG_CLK_PRINT */

#ifdef SECURE_CAMERA_IRIS
#undef SECURE_CAMERA_IRIS
#endif

#define SECURE_CAMERA_FACE		1 /* For face Detection and face authentication */
#define SECURE_CAMERA_CH		((1 << CSI_ID_C) | (1 << CSI_ID_E))
#define SECURE_CAMERA_HEAP_ID		(11)
#define SECURE_CAMERA_MEM_SHARE		1
#define SECURE_CAMERA_MEM_ADDR		(0x96000000)	/* secure_camera_heap */
#define SECURE_CAMERA_MEM_SIZE		(0x03000000)
#define NON_SECURE_CAMERA_MEM_ADDR	(0x0)	/* camera_heap */
#define NON_SECURE_CAMERA_MEM_SIZE	(0x0)
#define ION_EXYNOS_FLAG_PROTECTED	BIT(16)
#define ION_EXYNOS_FLAG_IOVA_EXTENSION	BIT(20)

#undef SECURE_CAMERA_FACE_SEQ_CHK	/* To check sequence before applying secure protection */

#define ENABLE_HWACG_CONTROL

#define USE_PDP_LINE_INTR_FOR_PDAF	0

/* use CIS global work for enhance launching time */
#define USE_CIS_GLOBAL_WORK	1

#define USE_CAMIF_FIX_UP	0
#define CHAIN_TAG_SENSOR_IN_SOFTIRQ_CONTEXT	0
#define CHAIN_TAG_VC0_DMA_IN_HARDIRQ_CONTEXT	1

#define CONFIG_SKIP_DUMP_LIC_OVERFLOW 1
#define USE_CSTAT_LIC_RECOVERY		0

#define CONFIG_VOTF_ONESHOT	0	/* oneshot mode is used when using VOTF in PDP input.  */
#define VOTF_BACK_FIRST_OFF	1
#define USE_VOTF_AXI_APB
#define USE_YPP_VOTF
#define USE_YPP_SHARED_INTERNAL_BUFFER		1

/* BTS */
/* #define DISABLE_BTS_CALC	*/ /* This is only for v8.1. next AP don't have to use this */

#define CONFIG_THROTTLING_MIF_ENABLE 1
#define CONFIG_THROTTLING_INT_ENABLE 0
#define CONFIG_THROTTLING_INTCAM_ENABLE 1

#define AP_PHY_LDO_ALL_SAME  	1

#define SYNC_SHOT_ALWAYS	/* This is a feature for reducing late shot. */

#define CHECK_NEED_KVADDR_LVN_ID(vid)                                                              \
	((0) || ((vid) == IS_LVN_MCFP_MV) || ((vid) == IS_LVN_BYRP_RTA_INFO) ||                    \
	 ((vid) == IS_LVN_RGBP_RTA_INFO) || ((vid) == IS_LVN_MCFP_RTA_INFO) ||                     \
	 ((vid) == IS_LVN_YUVP_RTA_INFO) || ((vid) == IS_LVN_LME_RTA_INFO) ||                      \
	 (is_get_debug_param(IS_DEBUG_PARAM_LVN) && (vid) == IS_VIDEO_IMM_NUM))

#define CHECK_NEED_KVADDR_ID(vid) (0)

#define MCSC_HF_ID	(IS_VIDEO_M5P_NUM)

#define CHECK_YPP_NOISE_RDMA_DISALBE(exynos_soc_info)	\
		(exynos_soc_info.main_rev >= 1 && exynos_soc_info.sub_rev >= 1)

#define HW_RUNNING_FPS		1

/*
 * ======================================================
 * CONFIG - Interface version configs
 * ======================================================
 */

#define SETFILE_VERSION_INFO_HEADER1	(0xF85A20B4)
#define SETFILE_VERSION_INFO_HEADER2	(0xCA539ADF)

#define USE_DDK_INTF_LIC_OFFSET
#define USE_DDK_HWIP_INTERFACE		1

#define USE_DDK_INTF_CAP_META		0
#endif /* #ifndef IS_CONFIG_H */
