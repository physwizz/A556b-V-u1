/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - Security Dump Manager support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <soc/samsung/exynos-smc.h>
#include <soc/samsung/exynos/debug-snapshot.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/panic_notifier.h>
#include <linux/of.h>
#include <linux/printk.h>

static bool sdm_enable;

static int exynos_sdm_dump_secure_region(void)
{
	int ret;

	ret = exynos_smc(SMC_CMD_DUMP_SECURE_REGION, 0, 0, 0);
	pr_info("%s: 0x%x\n", __func__, ret);

	return NOTIFY_OK;
}

int exynos_sdm_flush_secdram(void)
{
	return exynos_smc(SMC_CMD_FLUSH_SECDRAM, 0, 0, 0);
}

static int exynos_sdm_handler(struct notifier_block *nb, unsigned long l, void *buf)
{
	(void) l;
	(void) buf;

	if (sdm_enable && dbg_snapshot_is_scratch())
		return exynos_sdm_dump_secure_region();

	return 0;
}

static struct notifier_block sdm_dump = {
	.notifier_call = exynos_sdm_handler,
};

static int __init exynos_sdm_init(void)
{
	struct device_node *np = NULL;
	const char *sdm_enable_str;

	/* get information from device tree */
	np = of_find_compatible_node(NULL, NULL, "samsung,exynos-sdm");
	if (np == NULL) {
		pr_err("%s: Do not support exynos-sdm\n", __func__);
		return -ENODEV;
	}

	if (!of_property_read_string(np, "sdm_enable", &sdm_enable_str)) {
                if (!strcmp(sdm_enable_str, "true")) {
                        pr_info("SDM: SDM is enabled.\n");
			sdm_enable = true;
                } else {
                        pr_info("SDM: SDM is disabled.\n");
			sdm_enable = false;
                }
        }

	pr_info("SDM: register sdm to panic notifier\n");
	atomic_notifier_chain_register(&panic_notifier_list, &sdm_dump);

	pr_info("SDM: sdm driver init success.\n");

	return 0;
}
module_init(exynos_sdm_init);

MODULE_DESCRIPTION("Exynos Security Dump Manager driver");
MODULE_AUTHOR("<jiye.min@samsung.com>");
MODULE_LICENSE("GPL");
