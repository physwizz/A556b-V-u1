// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 * Pablo v13.20 specific functions
 *
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_HW_CHAIN_H
#define IS_HW_CHAIN_H

#include "is-hw-config.h"
#include "is-config.h"

enum sysreg_csis_reg_name {
	SYSREG_R_CSIS_MIPI_PHY_CON,
	SYSREG_CSIS_REG_CNT
};

enum sysreg_csis_reg_field {
	SYSREG_F_CSIS_MIPI_DPHY_CONFIG,
	SYSREG_F_CSIS_MIPI_RESETN_DPHY_S4,
	SYSREG_F_CSIS_MIPI_RESETN_DPHY_S3,
	SYSREG_F_CSIS_MIPI_RESETN_DCPHY_S2,
	SYSREG_F_CSIS_MIPI_RESETN_DCPHY_S1,
	SYSREG_F_CSIS_MIPI_RESETN_DCPHY_S,
	SYSREG_CSIS_REG_FIELD_CNT
};

enum s2pmu_list {
	SYSMMU_CSIS_S0_S2 = 0,
	SYSMMU_D_CSTAT_S2,
	SYSMMU_RGBP_S0_S2,
	SYSMMU_YUVP_S0_S2,
	SYSMMU_M2M_S0_S2,
	SYSMMU_ICPU_S0_S2,
	SYSMMU_DMAX_S2
};

enum sysreg_reg_name {
	AXAC_CSTAT0 = 0,
	AXAC_CSTAT1,
	AXAC_CSTAT2,
	AXAC_CSTAT3,
	AXAC_CSTAT4,
	AXAC_CSTAT5,
	AXIC_CSTAT,
	AXAC_MCSC0,
	AXAC_MCSC1,
	AXAC_MCSC2,
	AXIC_BYRP_MCSC,
	AXIC_1,
	SYSREG_REG_NUM,
};

enum sysreg_reg_field {
	AW_AC_CSTAT0 = 0,
	AW_AC_CSTAT1,
	AW_AC_CSTAT2,
	AW_AC_CSTAT3,
	AW_AC_CSTAT4,
	AW_AC_CSTAT5,
	AW_IC_TARGET_0,
	AW_AC_MCSC0,
	AR_AC_MCSC0,
	AW_AC_MCSC1,
	AW_AC_MCSC2,
	AW_IC_TARGET_2,
	AW_IC_1,
	SYSREG_REG_FIELD_NUM,
};

enum sysreg_reg_type {
	SYSREG_CSTAT = 0,
	SYSREG_M2M,
};

#define GROUP_HW_MAX (GROUP_SLOT_MAX)

#define IORESOURCE_CSTAT0	0
#define IORESOURCE_CSTAT1	1
#define IORESOURCE_CSTAT2	2
#define IORESOURCE_LME		3
#define IORESOURCE_BYRP	4
#define IORESOURCE_RGBP	5
#define IORESOURCE_MCFP	6
#define IORESOURCE_YUVP	7
#define IORESOURCE_MCSC	8
#define IORESOURCE_MAX 9

#define GROUP_SENSOR_MAX_WIDTH	16376
#define GROUP_SENSOR_MAX_HEIGHT	12288
#define GROUP_PDP_MAX_WIDTH	16376
#define GROUP_PDP_MAX_HEIGHT	12288
#define GROUP_3AA_MAX_WIDTH	16376
#define GROUP_3AA_MAX_HEIGHT	12288
#define GROUP_LME_MAX_WIDTH	2016
#define GROUP_LME_MAX_HEIGHT	1920
#define GROUP_MCFP_MAX_WIDTH	4880
#define GROUP_MCFP_MAX_HEIGHT	4320
#define GROUP_BYRP_MAX_WIDTH	4880
#define GROUP_BYRP_MAX_HEIGHT	9000

/* RTA HEAP: 6MB */
#define IS_RESERVE_LIB_SIZE	(0x00600000)

/* ORBMCH DMA: Moved to user space */
#define TAAISP_ORBMCH_SIZE	(0)

/* DDK DMA: Moved into driver */
#define IS_TAAISP_SIZE		(0)

/* TNR DMA: Moved into driver*/
#define TAAISP_TNR_SIZE		(0)

/* Secure TNR DMA: Moved into driver */
#define TAAISP_TNR_S_SIZE	(0)

/* DDK HEAP: 90MB */
#define IS_HEAP_SIZE		(0x05A00000)

/* config_level */
#define QE_CFG_LEVEL (2)

/* Rule checker size for DDK */
#define IS_RCHECKER_SIZE_RO	(SZ_4M + SZ_1M)
#define IS_RCHECKER_SIZE_RW	(SZ_256K)

#define SYSREG_CSIS_BASE_ADDR	(0x17420000)
#define SYSREG_CSTAT_BASE_ADDR	(0x17820000)
#define SYSREG_M2M_BASE_ADDR	(0x16020000)

enum ext_chain_id {
	ID_ORBMCH_0 = 0,
	ID_ORBMCH_1 = 1,
};

#define INTR_ID_BASE_OFFSET	(INTR_HWIP_MAX)
#define GET_IRQ_ID(y, x)	(x - (INTR_ID_BASE_OFFSET * y))
#define valid_3aaisp_intr_index(intr_index) \
	(intr_index >= 0 && intr_index < INTR_HWIP_MAX)

/* TODO: update below for 9830 */
/* Specific interrupt map belonged to each IP */

/* MC-Scaler */
#define USE_DMA_BUFFER_INDEX		(0) /* 0 ~ 7 */
#define MCSC_OFFSET_ALIGN		(2)
#define MCSC_WIDTH_ALIGN		(2)
#define MCSC_HEIGHT_ALIGN		(2)
#define MCSC_PRECISION			(20)
#define MCSC_POLY_RATIO_UP		(25)
#define MCSC_POLY_QUALITY_RATIO_DOWN	(4)
#define MCSC_POLY_RATIO_DOWN		(16)
#define MCSC_POLY_MAX_RATIO_DOWN	(256)
#define MCSC_POST_RATIO_DOWN		(16)
#define MCSC_POST_MAX_WIDTH		(1220)
/* #define MCSC_POST_WA */
/* #define MCSC_POST_WA_SHIFT	(8)*/	/* 256 = 2^8 */
#define MCSC_USE_DEJAG_TUNING_PARAM	(true)
/* #define MCSC_DNR_USE_TUNING		(true) */	/* DNR and DJAG TUNING PARAM are used exclusively. */
#if IS_ENABLED(SUPPORT_DJAG_PRE_FILTER)
#define MCSC_SETFILE_VERSION		(0x14027437)
#else
#define MCSC_SETFILE_VERSION		(0x14027435)
#endif
#define MCSC_DJAG_ENABLE_SENSOR_BRATIO	(2000)
#define MCSC_LINE_BUF_SIZE		(4880)
#define MCSC_DMA_MIN_WIDTH		(8)
#define HWFC_DMA_ID_OFFSET		(8)
#define ENTRY_HF			ENTRY_M5P	/* Subdev ID of MCSC port for High Frequency */
#define CAC_G2_VERSION			1

#define CSIS0_QCH_EN_ADDR		(0x17480004)
#define CSIS1_QCH_EN_ADDR		(0x17490004)
#define CSIS2_QCH_EN_ADDR		(0x174A0004)
#define CSIS3_QCH_EN_ADDR		(0x174B0004)
#define CSIS4_QCH_EN_ADDR		(0x174C0004)

/* LME */
#define LME_IMAGE_MAX_WIDTH		1664
#define LME_IMAGE_MAX_HEIGHT		1248
#define LME_TNR_MODE_MIN_BUFFER_NUM	1

int exynos991_is_dump_clk(struct device *dev);

#define IS_LLC_CACHE_HINT_SHIFT 4
#define IS_32B_WRITE_ALLOC_SHIFT 10

enum is_llc_cache_hint {
	IS_LLC_CACHE_HINT_INVALID = 0,
	IS_LLC_CACHE_HINT_BYPASS_TYPE,
	IS_LLC_CACHE_HINT_CACHE_ALLOC_TYPE,
	IS_LLC_CACHE_HINT_CACHE_NOALLOC_TYPE,
	IS_LLC_CACHE_HINT_VOTF_TYPE,
	IS_LLC_CACHE_HINT_LAST_BUT_SHARED,
	IS_LLC_CACHE_HINT_NOT_USED_FAR,
	IS_LLC_CACHE_HINT_LAST_ACCESS,
	IS_LLC_CACHE_HINT_MAX
};

enum is_llc_sn {
	IS_LLC_SN_DEFAULT = 0,
	IS_LLC_SN_FHD,
	IS_LLC_SN_UHD,
	IS_LLC_SN_8K,
	IS_LLC_SN_PREVIEW,
	IS_LLC_SN_END,
};

struct is_llc_way_num {
	int votf;
	int mcfp;
	int icpu;
};

const struct is_subdev_ops *pablo_get_is_subdev_sensor_ops(void);
#define pablo_get_is_subdev_ssvc_ops() (NULL)
const struct is_subdev_ops *pablo_get_is_subdev_paf_ops(void);
const struct is_subdev_ops *pablo_get_is_subdev_cstat_ops(void);
const struct is_subdev_ops *pablo_get_is_subdev_lme_ops(void);
const struct is_subdev_ops *pablo_get_is_subdev_byrp_ops(void);
const struct is_subdev_ops *pablo_get_is_subdev_rgbp_ops(void);
const struct is_subdev_ops *pablo_get_is_subdev_mcfp_ops(void);
const struct is_subdev_ops *pablo_get_is_subdev_yuvp_ops(void);
const struct is_subdev_ops *pablo_get_is_subdev_mcs_ops(void);

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
#define DEV_HW_ISP_BYRP DEV_HW_BYRP

struct is_device_ischain;
struct is_frame;
struct pablo_kunit_subdev_mcs_func {
	int (*mcs_tag_hf)(struct is_device_ischain *device,
				struct is_frame *frame);
};
#endif

#endif
