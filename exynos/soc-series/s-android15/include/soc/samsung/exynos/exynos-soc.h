/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header for EXYNOS SoC Chipid support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_SOC_H
#define __EXYNOS_SOC_H

#define S5E9945_SOC_ID		0xE9945000
#define S5E9955_SOC_ID		0xE9955000
#define S5E8855_SOC_ID          0xE8855000

#define EXYNOS_SOC_MASK		0xFFFFF000
#define EXYNOS_SOC_MASK_V2	0x00FFFFFF
#define EXYNOS_LOTID_MASK	0x001FFFFF
#define EXYNOS_REV_MASK		0xF
#define EXYNOS_UNIQUEID_MASK	(0x1FFFFFFFFFFFULL)	// 0 ~ 44bit

#define EXYNOS_MAIN_REV_1	0x10
#define EXYNOS_MAIN_REV_2	0x20

#define EXYNOS_SUB_REV1		0x1
#define EXYNOS_SUB_REV2		0x2

struct exynos_chipid_variant {
	int product_ver;
	int unique_id_reg;
	int rev_reg;
	int main_rev_bit;
	int sub_rev_bit;
};

/**
 * Struct exynos_chipid_info
 * @soc_product_id: product id allocated to exynos SoC
 * @soc_revision: revision of exynos SoC
 */
struct exynos_chipid_info {
	void __iomem *reg;
	u32 product_id;
	u32 revision;
	u32 main_rev;
	u32 sub_rev;
	u32 lot_id;
	u64 unique_id;
	struct exynos_chipid_variant *drv_data;
	struct platform_device *pdev;
	struct device *svc_dev;
};

extern struct exynos_chipid_info exynos_soc_info;

#endif /* __EXYNOS_SOC_H */
