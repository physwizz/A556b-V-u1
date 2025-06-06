/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <soc/samsung/exynos/exynos-soc.h>

#include <linux/iommu.h>
#include <linux/pm_runtime.h>

#include "include/npu-config.h"
#include "npu-clock.h"
#include "npu-log.h"
#include "npu-hw-device.h"
#include "npu-debug.h"
#include "npu-protodrv.h"
#include "npu-util-memdump.h"
#include "npu-util-regs.h"
#include "include/npu-stm-soc.h"
#include "include/vs4l.h"
#include "npu-afm.h"

/* FIXME: global variables */
u32 g_hwdev_num;
static struct npu_hw_device *g_hwdev_list[NPU_HWDEVICE_MAX_DEVICES] = {NULL,};

struct npu_hw_device *npu_get_hdev(char *name)
{
	int i;

	for (i = 0; i < NPU_HWDEVICE_MAX_DEVICES; i++) {
		/* no hw device */
		if (!g_hwdev_list[i])
			return NULL;
		if (!strcmp(g_hwdev_list[i]->name, name))
			return g_hwdev_list[i];
	}

	return NULL;
}

struct npu_hw_device *npu_get_hdev_by_id(int id)
{
	int i;

	for (i = 0; i < NPU_HWDEVICE_MAX_DEVICES; i++) {
		/* no hw device */
		if (!g_hwdev_list[i])
			return NULL;
		if (g_hwdev_list[i]->id == id)
			return g_hwdev_list[i];
	}

	return NULL;
}


/* ops implementation */
static int npu_hwdev_default_boot(struct npu_hw_device *hdev, bool on)
{
	int ret = 0;

	if (on) {
		if (hdev->type & NPU_HWDEV_TYPE_PWRCTRL) {
			/* power control */
			ret = pm_runtime_get_sync(hdev->dev);
			if (ret)
				npu_err("fail in runtime resume(%d)\n", ret);
			npu_info("%s power %s (%d)\n", hdev->name, on ? "on" : "off", ret);
		}
		if (hdev->type & NPU_HWDEV_TYPE_CLKCTRL) {
			/* clock control */
			ret = npu_clk_prepare_enable(&hdev->clks);
			if (ret)
				npu_err("fail to prepare/enable npu_clk(%d)\n", ret);
			npu_info("%s clock %s (%d)\n", hdev->name, on ? "on" : "off", ret);
		}
		hdev->status = NPU_HWDEV_STATUS_ACTIVE << 16 | NPU_HWDEV_STATUS_PWR_CLK_ON;
	}	else {
		if (hdev->type & NPU_HWDEV_TYPE_CLKCTRL) {
			/* clock control */
			npu_clk_disable_unprepare(&hdev->clks);
			npu_info("%s clock %s (%d)\n", hdev->name, on ? "on" : "off", ret);
		}
		if (hdev->type & NPU_HWDEV_TYPE_PWRCTRL) {
			/* power control */
			ret = pm_runtime_put_sync(hdev->dev);
			if (ret)
				npu_err("fail in runtime suspend(%d)\n", ret);
			npu_info("%s power %s (%d)\n", hdev->name, on ? "on" : "off", ret);
		}
		hdev->status = NPU_HWDEV_STATUS_PWR_CLK_OFF;
	}
	npu_info("%s (%d)\n", on ? "on" : "off", ret);
	return ret;
}

int npu_hwdev_hwacg(struct npu_system *system, u32 hids, bool on)
{
	int ret = 0;
	struct npu_hw_device *hdev;

#if IS_ENABLED(CONFIG_SOC_S5E9945)
	if (exynos_soc_info.main_rev == 0) {
		npu_info("Skip hwacg setting\n");
		return ret;
	}
#else
	return ret;
#endif

	npu_dbg("start %s\n", on?"on":"off");
	if (on) {
		switch (hids) {
		case NPU_HWDEV_ID_DNC:
			break;
		case NPU_HWDEV_ID_NPU:
			ret = npu_cmd_map(system, "acgenall");
			if (ret) {
				npu_err("fail(%d) in npu_cmd_map for acgenall\n", ret);
				goto err_hwacg;
			}
			break;
		case NPU_HWDEV_ID_DSP:
			ret = npu_cmd_map(system, "acgendsp");
			if (ret) {
				npu_err("fail(%d) in npu_cmd_map for acgendsp\n", ret);
				goto err_hwacg;
			}
			break;
		default:
			npu_err("HWACG %d cmd not registered\n", hids);
			break;
		}
	} else {
		switch (hids) {
		case NPU_HWDEV_ID_DNC:
			break;
		case NPU_HWDEV_ID_NPU:
			ret = npu_cmd_map(system, "acgenall");
			if (ret) {
				npu_err("fail(%d) in npu_cmd_map for acgenall\n", ret);
				goto err_hwacg;
			}
			break;
		case NPU_HWDEV_ID_DSP:
			ret = npu_cmd_map(system, "acgendsp");
			if (ret) {
				npu_err("fail(%d) in npu_cmd_map for acgendsp\n", ret);
				goto err_hwacg;
			}
			break;
		default:
			npu_err("HWACG %d cmd not registered\n", hids);
			break;
		}
	}

	hdev = npu_get_hdev("DNC");
	if (atomic_read(&hdev->boot_cnt.refcount) == 1) {
		if (on) {
			if (exynos_soc_info.sub_rev < 2) {
				ret = npu_cmd_map(system, "hwacgdislh");
			} else { // exynos_sec_info.sub_rev >= 2
				ret = npu_cmd_map(system, "hwacgdislh2");
			}
		} else { // off
			if (exynos_soc_info.sub_rev < 2) {
				ret = npu_cmd_map(system, "hwacgenlh");
			} else { // exynos_sec_info.sub_rev >= 2
				ret = npu_cmd_map(system, "hwacgenlh2");
			}
		}
	}
err_hwacg:
	npu_dbg("done %s\n", on?"on":"off");
	return ret;
}

static void npu_hwdev_power_gating(struct npu_system *system, u32 hids, bool on)
{
	int ret = 0;

	npu_info("Now, NPU APG is disabled\n");
	return;

	npu_dbg("start %s\n", on?"on":"off");
	if (on) {
		switch (hids) {
		case NPU_HWDEV_ID_NPU:
			ret = npu_cmd_map(system, "npupgon");
			if (ret) {
				npu_err("fail(%d) in npu_cmd_map for npupgon\n", ret);
				goto err_pg;
			}
			break;
		case NPU_HWDEV_ID_DSP:
			ret = npu_cmd_map(system, "dsppgon");
			if (ret) {
				npu_err("fail(%d) in npu_cmd_map for dsppgon\n", ret);
				goto err_pg;
			}
			break;
		default:
			npu_err("Power gating %d cmd not registered\n", hids);
			break;
		}
	} else {
		switch (hids) {
		case NPU_HWDEV_ID_NPU:
			ret = npu_cmd_map(system, "npupgoff");
			if (ret) {
				npu_err("fail(%d) in npu_cmd_map for npupgoff\n", ret);
				goto err_pg;
			}
			break;
		case NPU_HWDEV_ID_DSP:
			ret = npu_cmd_map(system, "dsppgoff");
			if (ret) {
				npu_err("fail(%d) in npu_cmd_map for dsppgoff\n", ret);
				goto err_pg;
			}
			break;
		default:
			npu_err("Power gating %d cmd not registered\n", hids);
			break;
		}
	}

err_pg:
	npu_dbg("done %s\n", on?"on":"off");
}

static int npu_hwdev_npu_init(struct npu_hw_device *hdev, bool on)
{
	int ret = 0;
	struct npu_device *device = hdev->device;
	struct npu_system *system = &device->system;

	if (on) {
		ret = npu_stm_enable(system, hdev->id);
		if (ret)
			npu_err("fail(%d) in npu_stm_enable(%u)\n", ret, hdev->id);
		npu_hwdev_hwacg(system, hdev->id, true);
		npu_hwdev_power_gating(system, hdev->id, true);
		npu_afm_open(system, hdev->id);
	} else {
		ret = npu_stm_disable(system, hdev->id);
		if (ret)
			npu_err("fail(%d) in npu_stm_disable(%u)\n", ret, hdev->id);

		npu_afm_close(system, hdev->id);
		npu_hwdev_power_gating(system, hdev->id, false);
		npu_hwdev_hwacg(system, hdev->id, false);
	}

	npu_info("%s (%d)\n", on ? "on" : "off", ret);
	return ret;
}

static int npu_hwdev_dnc_init(struct npu_hw_device *hdev, bool on)
{
	int ret = 0;
	struct npu_device *device = hdev->device;

	if (on) {
		ret = npu_device_bootup(device);
		if (check_emergency(device)) {
			npu_err("start for emergency recovery\n");
			npu_device_shutdown(device);
			npu_device_recovery_close(device);
			ret = -EWOULDBLOCK;
			goto p_err;
		}

		if (ret) {
			npu_err("fail(%d) in npu_device_bootup", ret);
			npu_device_shutdown(device);
			goto p_err;
		}
	} else {
		ret = npu_device_shutdown(device);
		if (ret) {
			npu_err("fail(%d) in npu_device_shutdown\n", ret);
			goto p_err;
		}
	}

p_err:
	npu_info("%s (%d)\n", on ? "on" : "off", ret);
	return ret;
}

static int npu_hwdev_dsp_init(struct npu_hw_device *hdev, bool on)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_DSP_USE_VS4L)
	struct npu_device *device = hdev->device;
	struct npu_system *system = &device->system;

	/* kernel loading for init */
	if (on) {
		ret = dsp_system_load_binary(system);
		if (ret) {
			probe_err("fail(%d) in dsp_system_load_binary\n", ret);
			goto err_kernel;
		}

		ret = dsp_kernel_manager_open(system, &hdev->device->kmgr);
		if (ret) {
			npu_err("fail(%d) in dsp_kernel_manager_open\n", ret);
			goto err_kernel;
		}

		ret = npu_stm_enable(system, hdev->id);
		if (ret)
			npu_err("fail(%d) in npu_stm_enable(%u)\n", ret, hdev->id);
		npu_hwdev_hwacg(system, hdev->id, true);
		npu_hwdev_power_gating(system, hdev->id, true);
		npu_afm_open(system, hdev->id);
	} else {
		dsp_kernel_manager_close(&hdev->device->kmgr, 1);

		ret = npu_stm_disable(&hdev->device->system, hdev->id);
		if (ret)
			npu_err("fail(%d) in npu_stm_disable(%u)\n", ret, hdev->id);

		npu_afm_close(system, hdev->id);
		npu_hwdev_power_gating(system, hdev->id, false);
		npu_hwdev_hwacg(system, hdev->id, false);
	}

err_kernel:
#endif
	npu_info("%s (%d)\n", on ? "on" : "off", ret);
	return ret;
}

static int npu_hwdev_probe(struct platform_device *pdev)
{
	int ret = 0, hi;
	struct device *dev;
	struct npu_hw_device *hdev;

	BUG_ON(!pdev);

	dev = &pdev->dev;

	hi = g_hwdev_num;

	hdev = devm_kzalloc(dev, sizeof(*hdev), GFP_KERNEL);
	if (!hdev) {
		probe_err("fail in devm_kzalloc\n");
		ret = -ENOMEM;
		goto err_exit;
	}
	hdev->dev = dev;

	/* get hw device name */
	ret = of_property_read_string(dev->of_node,
			"samsung,npuhwdev-name",
			(const char **)&hdev->name);
	if (ret) {
		probe_err("failed to read hwdev name %d from %s node : %d\n",
				hi, dev->of_node->name, ret);
		goto err_exit;
	}

	/* get hw device's parent name */
	ret = of_property_read_string(dev->of_node,
			"samsung,npuhwdev-parent",
			(const char **)&hdev->parent);
	if (ret)
		probe_warn("no parent for %s hwdev node : %d\n",
				dev->of_node->name, ret);

	ret = of_property_read_u32(dev->of_node, "samsung,npuhwdev-type", &hdev->type);
	if (ret) {
		probe_err("failed to read hwdev type %d from %s node : %d\n",
				hi, dev->of_node->name, ret);
		goto err_exit;
	}

	ret = of_property_read_u32(dev->of_node, "samsung,npuhwdev-id", &hdev->id);
	if (ret) {
		probe_err("failed to read hwdev id %d from %s node : %d\n",
				hi, dev->of_node->name, ret);
		goto err_exit;
	}

	/* links ops */
	if (!strcmp(hdev->name, "DSP")) {
		hdev->ops.boot = npu_hwdev_default_boot;
		hdev->ops.init = npu_hwdev_dsp_init;
	} else if (!strcmp(hdev->name, "DNC")) {
		hdev->ops.boot = npu_hwdev_default_boot;
		hdev->ops.init = npu_hwdev_dnc_init;
	} else if (!strcmp(hdev->name, "NPU")) {
		hdev->ops.boot = npu_hwdev_default_boot;
		hdev->ops.init = npu_hwdev_npu_init;
	} else {
		probe_warn("no boot/init function for %s\n", hdev->name);
		hdev->ops.boot = NULL;
		hdev->ops.init = NULL;
	}

	npu_hw_ref_setup(&hdev->boot_cnt, hdev,
			npu_hw_ref_open, npu_hw_ref_close);
	npu_hw_ref_setup(&hdev->init_cnt, hdev,
			npu_hw_ref_init, npu_hw_ref_deinit);

	/* init parameters */
	hdev->idle_load = 0;
	hdev->status = NPU_HWDEV_STATUS_PWR_CLK_OFF;

	ret = npu_clk_get(&hdev->clks, dev);
	if (ret)
		probe_warn("fail(%d) in npu_clk_get\n", ret);

	pm_runtime_enable(dev);
	dev_set_drvdata(dev, hdev);

	ret = 0;
	g_hwdev_list[g_hwdev_num] = hdev;
	g_hwdev_num++;
	probe_info("loading %s (id: 0x%x) completed (%d)\n",
			hdev->name, hdev->id, g_hwdev_num - 1);

	goto ok_exit;
err_exit:
	probe_err("error (%d)\n", ret);
ok_exit:
	return ret;
}

int npu_hwdev_register_hwdevs(struct npu_device *device, struct npu_system *system)
{
	int i;

	system->hwdev_num = g_hwdev_num;
	system->hwdev_list = g_hwdev_list;

	for (i = 0; i < g_hwdev_num; i++)
		g_hwdev_list[i]->device = device;

	return 0;
}

#ifndef CONFIG_NPU_KUNIT_TEST
int npu_hwdev_open(struct npu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	npu_dbg(" (%d)\n", ret);
	return ret;
}

int npu_hwdev_close(struct npu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	npu_dbg(" (%d)\n", ret);
	return ret;
}
#endif

int npu_hwdev_bootup(struct npu_device *device, __u32 hids)
{
	int ret = 0;
	int hi;
	struct npu_hw_device *hdev;

	BUG_ON(!device);

	/* hw-device-wide bootup : for each network */
	for (hi = 0; hi < g_hwdev_num; hi++) {
		hdev = g_hwdev_list[hi];
		if (hdev && (hids & hdev->id)) {
			npu_hw_ref_get(device, &hdev->boot_cnt);
			npu_info("boot %s (%d)\n", hdev->name, hdev->id);
		}
	}

	for (hi = 0; hi < g_hwdev_num; hi++) {
		hdev = g_hwdev_list[hi];
		if (hdev && (hids & hdev->id)) {
			npu_hw_ref_get(device, &hdev->init_cnt);
			npu_info("init %s (%d)\n", hdev->name, hdev->id);
		}
	}

	npu_info("(%d)\n", ret);
	return ret;
}

int npu_hwdev_shutdown(struct npu_device *device, __u32 hids)
{
	int ret = 0;
	int hi;
	struct npu_hw_device *hdev;

	BUG_ON(!device);

	/* hw-device-wide shutdown : for each network */
	for (hi = g_hwdev_num - 1; hi >= 0; hi--) {
		hdev = g_hwdev_list[hi];
		if (hdev && (hids & hdev->id)) {
			npu_hw_ref_put(device, &hdev->init_cnt);
			npu_info("init %s (%d)\n", hdev->name, hdev->id);
		}
	}

	for (hi = g_hwdev_num - 1; hi >= 0; hi--) {
		hdev = g_hwdev_list[hi];
		if (hdev && (hids & hdev->id)) {
			npu_hw_ref_put(device, &hdev->boot_cnt);
			npu_info("boot %s (%d)\n", hdev->name, hdev->id);
		}
	}

	npu_info("(%d)\n", ret);
	return ret;
}

int npu_hwdev_recovery_shutdown(struct npu_device *device)
{
	int ret = 0;
	int hi, isleft;
	struct npu_hw_device *hdev;

	BUG_ON(!device);

	/* hw-device-wide shutdown : for each network */
	isleft = 1;
	while (isleft) {
		isleft = 0;
		for (hi = g_hwdev_num - 1; hi >= 0; hi--) {
			hdev = g_hwdev_list[hi];
			if (hdev && strcmp(hdev->name, "DNC") && hdev->status) {
				npu_hw_ref_put(device, &hdev->init_cnt);
				npu_dbg("%s deinit\n", hdev->name);
				if (atomic_read(&hdev->init_cnt.refcount))
					isleft = 1;

			}
		}
	}

	isleft = 1;
	while (isleft) {
		isleft = 0;
		for (hi = g_hwdev_num - 1; hi >= 0; hi--) {
			hdev = g_hwdev_list[hi];
			if (hdev && strcmp(hdev->name, "DNC") && hdev->status) {
				npu_hw_ref_put(device, &hdev->boot_cnt);
				npu_dbg("%s shutdown\n", hdev->name);
				if (atomic_read(&hdev->boot_cnt.refcount))
					isleft = 1;
			}
		}
	}

	npu_info("(%d)\n", ret);
	return ret;
}

#if IS_ENABLED(CONFIG_PM_SLEEP)
static int npu_hwdev_suspend(struct device *dev)
{
	npu_dbg("called\n");
	return 0;
}

static int npu_hwdev_resume(struct device *dev)
{
	npu_dbg("called\n");
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_PM)
static int npu_hwdev_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct npu_hw_device *hdev;

	hdev = dev_get_drvdata(dev);

	/* default core power down, clock off */

	/* hwdev power down, clock off */

	npu_info("%s (%d)\n", hdev->name, ret);
	return ret;
}

static int npu_hwdev_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct npu_hw_device *hdev;

	hdev = dev_get_drvdata(dev);

	/* hwdev power up, clock up */

	/* default core power up, clock up */

	npu_info("%s (%d)\n", hdev->name, ret);
	return ret;
}
#endif

static int npu_hwdev_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
	struct npu_hw_device *hdev;

	BUG_ON(!pdev);

	dev = &pdev->dev;
	hdev = dev_get_drvdata(dev);
	pm_runtime_disable(dev);
	npu_clk_put(&hdev->clks, dev);

	probe_info("complete in %s\n", __func__);
	return ret;
}

static const struct dev_pm_ops npu_hwdev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(npu_hwdev_suspend, npu_hwdev_resume)
	SET_RUNTIME_PM_OPS(npu_hwdev_runtime_suspend, npu_hwdev_runtime_resume, NULL)
};

#if IS_ENABLED(CONFIG_OF)
const static struct of_device_id exynos_npu_hwdev_match[] = {
	{
		.compatible = "samsung,exynos-npu-hwdev"
	},
	{}
};
MODULE_DEVICE_TABLE(of, exynos_npu_hwdev_match);
#endif

static struct platform_driver npu_hwdev_driver = {
	.probe	= npu_hwdev_probe,
	.remove = npu_hwdev_remove,
	.driver = {
		.name	= "exynos-npu-hwdev",
		.owner	= THIS_MODULE,
		.pm	= &npu_hwdev_pm_ops,
		.of_match_table = of_match_ptr(exynos_npu_hwdev_match),
	},
};

int __init npu_hwdev_register(void)
{
	int ret = 0;

	ret = platform_driver_register(&npu_hwdev_driver);
	if (ret) {
		probe_err("error(%d) in platform_driver_register\n", ret);
		goto err_exit;
	}

	probe_info("success in %s\n", __func__);
	ret = 0;
	goto ok_exit;

err_exit:
	// necessary clean-up

ok_exit:
	return ret;
}

void __exit npu_hwdev_unregister(void)
{
	platform_driver_unregister(&npu_hwdev_driver);

	probe_info("success in %s\n", __func__);
}

