// SPDX-License-Identifier: GPL
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/reboot.h>

#include <linux/iommu.h>

#include "pablo-icpu.h"
#include "pablo-icpu-core.h"
#include "pablo-icpu-hw-itf.h"
#include "firmware/pablo-icpu-firmware.h"
#include "mbox/pablo-icpu-mbox.h"
#include "pablo-icpu-debug.h"
#include "pablo-icpu-sysfs.h"
#include "mem/pablo-icpu-mem.h"
#include "pablo-icpu-enum.h"

#include "pablo-kernel-variant.h"
#include "pablo-debug.h"
#include "pablo-internal-subdev-ctrl.h"

static struct icpu_core *core;
static struct icpu_logger _log = {
	.level = LOGLEVEL_INFO,
	.prefix = "[ICPU-CORE]",
};

static ulong debug_icpu_aarch;
module_param(debug_icpu_aarch, ulong, 0644);

struct icpu_logger *get_icpu_core_log(void)
{
	return &_log;
}

struct icpu_core *get_icpu_core(void)
{
	return core;
}

void *get_icpu_dev(struct icpu_core *core)
{
	return (core && core->pdev) ? &core->pdev->dev : NULL;
}

static void *__get_pdata(struct icpu_core *core)
{
	return (core && core->pdev) ? dev_get_platdata(&core->pdev->dev) : NULL;
}

static inline void __pablo_icpu_teardown_hw(void)
{
	int ret = pablo_icpu_hw_wait_teardown_timeout(dev_get_platdata(&core->pdev->dev), 300);

	if (ret) {
		ICPU_ERR("hw_wait_teardown fail, ret = %d", ret);
		pablo_icpu_hw_print_debug_reg(dev_get_platdata(&core->pdev->dev));
		pablo_icpu_debug_fw_log_dump();
		pablo_icpu_hw_force_powerdown(dev_get_platdata(&core->pdev->dev));
	}
}

static int pablo_icpu_config_debug(void *pdata)
{
	int ret = 0;
	void *log_buf;
	struct pablo_icpu_buf_info log_buf_info;
	size_t log_size;
	size_t printk_size;

	log_buf = icpu_firmware_get_buf_log();
	if (log_buf) {
		log_buf_info = pablo_icpu_mem_get_buf_info(log_buf);

		pablo_icpu_hw_set_debug_reg(pdata, 3, log_buf_info.dva);

		log_size = pablo_icpu_get_log_size();
		pablo_icpu_hw_set_mbox_debug_reg(pdata, 38, log_size);

		printk_size = log_buf_info.size - log_size;
		if (printk_size > 0)
			pablo_icpu_hw_set_mbox_debug_reg(pdata, 39, printk_size);

		ret = pablo_icpu_debug_config((void *)log_buf_info.kva, log_buf_info.size);
		if (ret) {
			ICPU_ERR("icpu debug config fail, kva:0x%lx, size:%zu", log_buf_info.kva,
				 log_buf_info.size);
			pablo_icpu_debug_reset();
		}
	}

	return ret;
}

int pablo_icpu_boot(bool secure_mode)
{
	int ret;
	void *pdata;
	void *icpubuf;
	struct pablo_icpu_buf_info fw_info;
	bool aarch64;

	if (!core)
		return -ENODEV;

	pdata = dev_get_platdata(get_icpu_dev(core));
	if (!pdata)
		return -ENOMEM;

#if defined(CONFIG_PM)
	ICPU_INFO("pm_runtime_get_sync: E");
	ret = pm_runtime_get_sync(get_icpu_dev(core));
	if (ret < 0) {
		ICPU_ERR("pm_runtime_get_sync() return error: %d", ret);
		return ret;
	}
	ICPU_INFO("pm_runtime_get_sync: X");
#endif

	ret = load_firmware(get_icpu_dev(core), secure_mode);
	if (ret) {
		ICPU_ERR("icpu load firmware fail: %d", ret);
		goto err_load_firmware;
	}

	pablo_icpu_hw_set_sw_reset(pdata);

	/* Prepare some hw dependencies */
	pablo_icpu_hw_misc_prepare(pdata);

	icpubuf = icpu_firmware_get_buf_bin();
	fw_info = pablo_icpu_mem_get_buf_info(icpubuf);
	pablo_icpu_hw_set_base_address(pdata, fw_info.dva);

	if (unlikely(debug_icpu_aarch == 32))
		aarch64 = false;
	else if (unlikely(debug_icpu_aarch == 64))
		aarch64 = true;
	else
		aarch64 = icpu_firmware_get_aarch64();
	pablo_icpu_hw_set_aarch64(pdata, aarch64);

	icpubuf = icpu_firmware_get_buf_heap(secure_mode);
	if (icpubuf) {
		fw_info = pablo_icpu_mem_get_buf_info(icpubuf);
		pablo_icpu_hw_set_debug_reg(pdata, 2, fw_info.dva);
	}

	icpubuf = icpu_firmware_get_buf_dbg();
	if (icpubuf) {
		fw_info = pablo_icpu_mem_get_buf_info(icpubuf);
		pablo_icpu_hw_set_debug_reg(pdata, 15, fw_info.dva);
		pablo_icpu_debug_trace_dump_info((void *)fw_info.kva, fw_info.size);
	}

	if (pablo_icpu_config_debug(pdata))
		goto err_config_debug;

	ret = pablo_icpu_hw_release_reset(pdata, 1);
	if (ret)
		goto err_hw_release_reset;

	return 0;

err_hw_release_reset:
err_config_debug:
	teardown_firmware();

err_load_firmware:
	ICPU_INFO("pm_runtime_put_sync: E");
	if (pm_runtime_put_sync(&core->pdev->dev))
		ICPU_ERR("pm_runtime_put_sync is fail(%d)", ret);
	ICPU_INFO("pm_runtime_put_sync: X");

	return ret;
}
EXPORT_SYMBOL_GPL(pablo_icpu_boot);

bool pablo_icpu_boot_done(u32 id)
{
	u32 num_core;
	unsigned long core_ready;
	unsigned long flags;
	bool complete;

	if (!core) {
		ICPU_ERR("core is NULL");
		return false;
	}

	num_core = pablo_icpu_hw_num_cores(__get_pdata(core));

	spin_lock_irqsave(&core->icpu_state_slock, flags);
	set_bit(id, &core->icpu_boot_state);
	core_ready = hweight_long(core->icpu_boot_state);
	complete = core_ready == num_core ? true : false;
	spin_unlock_irqrestore(&core->icpu_state_slock, flags);

	ICPU_INFO("core%d ready, [%lu/%d]", id, core_ready, num_core);

	return complete;

}
EXPORT_SYMBOL_GPL(pablo_icpu_boot_done);

static inline void __pablo_icpu_dbg_trace_dump(void)
{
	if (!pablo_icpu_debug_trace_enabled())
		return;

	pablo_icpu_debug_trace_dump();
}

void pablo_icpu_power_down(void)
{
	int ret;
	unsigned long flags;

	if (!core)
		return;

	__pablo_icpu_teardown_hw();

	spin_lock_irqsave(&core->icpu_state_slock, flags);
	core->icpu_boot_state = 0;
	spin_unlock_irqrestore(&core->icpu_state_slock, flags);

	pablo_icpu_debug_reset();

	__pablo_icpu_dbg_trace_dump();

	teardown_firmware();

#if defined(CONFIG_PM)
	ICPU_INFO("pm_runtime_put_sync: E");
	ret = pm_runtime_put_sync(&core->pdev->dev);
	if (ret)
		ICPU_ERR("pm_runtime_put_sync is fail(%d)", ret);
	ICPU_INFO("pm_runtime_put_sync: X");
#endif
}
EXPORT_SYMBOL_GPL(pablo_icpu_power_down);

int pablo_icpu_preload_fw(unsigned long flag)
{
	int ret;

	if (is_firmware_loaded())
		return 0;

#if defined(CONFIG_PM)
	if (!pm_runtime_status_suspended(&core->pdev->dev)) {
		ICPU_ERR("ICPU is not suspended");
		return -EBUSY;
	}

	ICPU_INFO("pm_runtime_get_sync: E");
	ret = pm_runtime_get_sync(get_icpu_dev(core));
	if (ret < 0) {
		ICPU_ERR("pm_runtime_get_sync() return error: %d", ret);
		return ret;
	}
	ICPU_INFO("pm_runtime_get_sync: X");
#endif

	ret = preload_firmware(get_icpu_dev(core), flag);
	if (ret)
		goto exit_put_sync;

	if (test_bit(PABLO_ICPU_PRELOAD_FLAG_FORCE_RELEASE, &flag)) {
		ICPU_INFO("Force release after preload, flag(%lx)", flag);
		teardown_firmware();
	}

exit_put_sync:
#if defined(CONFIG_PM)
	ICPU_INFO("pm_runtime_put_sync: E");
	if (pm_runtime_put_sync(get_icpu_dev(core)))
		ICPU_ERR("pm_runtime_put_sync is fail");
	ICPU_INFO("pm_runtime_put_sync: X");
#endif

	return ret;
}

void pablo_icpu_print_debug_reg(void)
{
	struct device *dev;

	pablo_icpu_hw_print_debug_reg(dev_get_platdata(&core->pdev->dev));

#ifdef CONFIG_PM
	dev = get_icpu_dev(core);
	if (dev)
		ICPU_INFO("power.usage_count: %d", atomic_read(&dev->power.usage_count));
#endif
}

static struct icpu_mbox_controller *__get_mbox_by_state(enum icpu_mbox_chan_type type,
		enum icpu_mbox_state state)
{
	int i;
	void *pdata;
	struct icpu_mbox_controller *mbox = NULL;
	struct icpu_mbox_controller **mbox_list = NULL;
	u32 num_mbox = 0;

	pdata = __get_pdata(core);
	if (!pdata) {
		ICPU_ERR("icpu core is not probed");
		return NULL;
	}

	/* TODO: If more than one mboxes are supported for each rx and tx,
	 * we need mbox management to search free mbox and match to channel
	 */
	if (type == ICPU_MBOX_CHAN_TX) {
		num_mbox = pablo_icpu_hw_get_num_tx_mbox(pdata);
		mbox_list = core->tx_mboxes;
	} else if (type == ICPU_MBOX_CHAN_RX) {
		num_mbox = pablo_icpu_hw_get_num_rx_mbox(pdata);
		mbox_list = core->rx_mboxes;
	}

	for (i = 0; i < num_mbox; i++) {
		if (!mbox_list[i])
			continue;

		if (mbox_list[i]->state == state) {
			mbox = mbox_list[i];
			break;
		}
	}

	return mbox;
}

static struct pablo_icpu_mbox_chan *__get_free_channel(void)
{
	int i;
	void *pdata;
	struct pablo_icpu_mbox_chan *chan = NULL;

	pdata = __get_pdata(core);
	if (!pdata) {
		ICPU_ERR("icpu core is not probed");
		return NULL;
	}

	for (i = 0; i < pablo_icpu_hw_get_num_channels(pdata); i++) {
		if (core->chans[i].cl == NULL) {
			chan = &core->chans[i];
			break;
		}
	}

	return chan;
}

u32 pablo_icpu_get_num_rx_mbox(void)
{
	return pablo_icpu_hw_get_num_rx_mbox(dev_get_platdata(&core->pdev->dev));
}
KUNIT_EXPORT_SYMBOL(pablo_icpu_get_num_rx_mbox);

struct pablo_icpu_mbox_chan *pablo_icpu_request_mbox_chan(struct icpu_mbox_client *cl,
		enum icpu_mbox_chan_type type)
{
	int ret;
	struct pablo_icpu_mbox_chan *chan = NULL;

	if (!cl || type >= ICPU_MBOX_CHAN_MAX) {
		ICPU_ERR("Invalid argument");
		return NULL;
	}

	chan = __get_free_channel();
	if (!chan) {
		ICPU_ERR("fail to acquire free channel");
		return NULL;
	}

	chan->mbox = __get_mbox_by_state(type, ICPU_MBOX_STATE_INIT);
	if (!chan->mbox) {
		ICPU_ERR("Cannot acquire mbox for type(%d)", type);
		return NULL;
	}

	if (chan->mbox->ops->startup) {
		ret = chan->mbox->ops->startup(chan);
		if (ret) {
			ICPU_ERR("Cannot startup channel, type(%d), ret(%d)", type, ret);
			return NULL;
		}
	}

	chan->cl = cl;

	ICPU_DEBUG("request mbox channel(type=%d) done", type);

	return chan;
}
EXPORT_SYMBOL_GPL(pablo_icpu_request_mbox_chan);

void pablo_icpu_free_mbox_chan(struct pablo_icpu_mbox_chan *chan)
{
	if (!core || !chan)
		return;

	if (chan->mbox->ops->shutdown)
		chan->mbox->ops->shutdown(chan);

	memset(chan, 0x0, sizeof(struct pablo_icpu_mbox_chan));

	ICPU_INFO("free mbox channel done");
}
EXPORT_SYMBOL_GPL(pablo_icpu_free_mbox_chan);

void pablo_icpu_handle_msg(u32 *rx_data, u32 len)
{
	if (!rx_data)
		return;

	ICPU_INFO("Do nothing for msg 0x%x(len:%d)", rx_data[0], len);
}
EXPORT_SYMBOL_GPL(pablo_icpu_handle_msg);

static int pablo_icpu_suspend(struct device *dev)
{
	ICPU_DEBUG("pablo-icpu Suspend\n");

	return 0;
}

static int pablo_icpu_resume(struct device *dev)
{
	ICPU_DEBUG("pablo-icpu Resume\n");

	return 0;
}

static int pablo_icpu_ischain_runtime_suspend(struct device *dev)
{
	ICPU_DEBUG("pablo-icpu runtime suspend\n");

	return 0;
}

static int pablo_icpu_ischain_runtime_resume(struct device *dev)
{
	ICPU_DEBUG("pablo-icpu runtime resume\n");

	return 0;
}

static const struct dev_pm_ops pablo_icpu_pm_ops = {
	.suspend		= pablo_icpu_suspend,
	.resume			= pablo_icpu_resume,
	.runtime_suspend	= pablo_icpu_ischain_runtime_suspend,
	.runtime_resume		= pablo_icpu_ischain_runtime_resume,
};

static const struct of_device_id pablo_icpu_match[] = {
	{
		.compatible = "samsung,exynos-isp-cpu",
	},
	{},
};
MODULE_DEVICE_TABLE(of, pablo_icpu_match);

static void __hw_panic_handler(void)
{
	if (!core || !core->pdev)
		return;

	if (pm_runtime_status_suspended(&core->pdev->dev))
		return;

	pablo_icpu_hw_panic_handler(dev_get_platdata(&core->pdev->dev));
}

static struct icpu_panic_handlers {
	icpu_panic_handler_t itf_handler;
} _icpu_panic_handlers;

void pablo_icpu_register_panic_handler(icpu_panic_handler_t hnd)
{
	_icpu_panic_handlers.itf_handler = hnd;
}

static void _notify_panic(void)
{
	if (_icpu_panic_handlers.itf_handler)
		_icpu_panic_handlers.itf_handler();
}

static void __panic_handler_impl(void)
{
	_notify_panic();

	__hw_panic_handler();

	is_debug_lock();
	pablo_icpu_debug_fw_log_dump();
	is_debug_unlock();
}

static int __iommu_fault_handler(struct iommu_fault *fault, void *token)
{
	ICPU_INFO("in");

	__panic_handler_impl();

	ICPU_INFO("out");

	return 0;
}

static void __teardown_core(struct platform_device *pdev);
static int __reboot_handler(struct notifier_block *nb, ulong l, void *buf)
{
	ICPU_INFO("in");

	__panic_handler_impl();

	ICPU_INFO("out");

	return 0;
}

static struct notifier_block __notify_reboot_block = {
	.notifier_call = __reboot_handler,
};

static int __panic_handler(struct notifier_block *nb, ulong l, void *buf)
{
	ICPU_INFO("in");

	__panic_handler_impl();

	ICPU_INFO("out");

	return 0;
}

static struct notifier_block __notify_panic_block = {
	.notifier_call = __panic_handler,
};

static void __s2d_handler(void)
{
	ICPU_INFO("in");

	__panic_handler_impl();

	ICPU_INFO("out");
}

void *__get_fw_loginfo(u32 *size)
{
	struct pablo_icpu_buf_info fw_info;

	fw_info = pablo_icpu_mem_get_buf_info(icpu_firmware_get_buf_log());
	*size = fw_info.size;

	return (void *)fw_info.kva;
}

static void __teardown_core(struct platform_device *pdev)
{
	void *pdata;
	int i;

	if (!core)
		return;

	pdata = dev_get_platdata(&pdev->dev);

	if (core->tx_mboxes) {
		for (i = 0; i < pablo_icpu_hw_get_num_tx_mbox(pdata); i++)
			if (core->tx_mboxes[i]) {
				pablo_icpu_mbox_free(core->tx_mboxes[i]);
				core->tx_mboxes[i] = NULL;
			}

		kfree(core->tx_mboxes);
		core->tx_mboxes = NULL;
	}

	if (core->rx_mboxes) {
		for (i = 0; i < pablo_icpu_hw_get_num_rx_mbox(pdata); i++)
			if (core->rx_mboxes[i]) {
				pablo_icpu_mbox_free(core->rx_mboxes[i]);
				core->rx_mboxes[i] = NULL;
			}

		kfree(core->rx_mboxes);
		core->rx_mboxes = NULL;
	}

	kfree(core->chans);
	core->chans = NULL;

	pablo_icpu_mem_remove();

	pdev->dev.platform_data = NULL;
	kfree(pdata);

	kfree(core);
	core = NULL;
}

static struct pablo_icpu_operations is_debug_ops = {
	.s2d_handler = __s2d_handler,
	.get_fw_loginfo = __get_fw_loginfo,
};

static int pablo_icpu_probe(struct platform_device *pdev)
{
	int ret;
	void *pdata;
	int i;
	u32 num_chans;
	u32 num_tx_mbox;
	u32 num_rx_mbox;

	core = kzalloc(sizeof(struct icpu_core), GFP_KERNEL);
	if (!core)
		return -ENOMEM;

	core->pdev = pdev;

	ret = pablo_icpu_hw_probe(pdev);
	if (ret)
		goto probe_fail;

	pdata = dev_get_platdata(&pdev->dev);

#if defined(CONFIG_PM)
	pm_runtime_enable(&pdev->dev);
#endif

	ret = pablo_icpu_mem_probe(pdev);
	if (ret) {
		ICPU_ERR("icpu_mem_probe fail");
		goto probe_fail;
	}

	ret = icpu_firmware_probe(&pdev->dev);
	if (ret) {
		ICPU_ERR("pablo_icpu_firmware_probe fail, ret(%d)", ret);
		goto probe_fail;
	}

	num_chans = pablo_icpu_hw_get_num_channels(pdata);
	num_tx_mbox = pablo_icpu_hw_get_num_tx_mbox(pdata);
	num_rx_mbox = pablo_icpu_hw_get_num_rx_mbox(pdata);

	core->chans = kzalloc(sizeof(struct pablo_icpu_mbox_chan) * num_chans, GFP_KERNEL);
	if (!core->chans)
		goto probe_fail;

	/* TODO: check usage of this lock */
	for (i = 0; i < num_chans; i++)
		spin_lock_init(&core->chans[i].lock);

	core->tx_mboxes = kzalloc(sizeof(void *) * num_tx_mbox, GFP_KERNEL);
	if (!core->tx_mboxes)
		goto probe_fail;

	core->rx_mboxes = kzalloc(sizeof(void *) * num_rx_mbox, GFP_KERNEL);
	if (!core->rx_mboxes)
		goto probe_fail;

	for (i = 0; i < num_tx_mbox; i++) {
		core->tx_mboxes[i] = pablo_icpu_mbox_request(ICPU_MBOX_MODE_TX, pablo_icpu_hw_get_tx_info(pdata, i));
		if (!core->tx_mboxes[i])
			goto probe_fail;
	}

	for (i = 0; i < num_rx_mbox; i++) {
		core->rx_mboxes[i] = pablo_icpu_mbox_request(ICPU_MBOX_MODE_RX, pablo_icpu_hw_get_rx_info(pdata, i));
		if (!core->rx_mboxes[i])
			goto probe_fail;
	}

	ret = pablo_icpu_debug_probe();
	if (ret) {
		ICPU_ERR("pablo_icpu_debug_probe fail");
		goto probe_fail;
	}

	ret = pablo_icpu_sysfs_probe(&pdev->dev);
	if (ret) {
		ICPU_ERR("pablo_icpu_sysfs_probe fail");
		goto probe_fail;
	}

	dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));

	iommu_register_device_fault_handler(&pdev->dev, __iommu_fault_handler, NULL);

	atomic_notifier_chain_register(&panic_notifier_list, &__notify_panic_block);
	register_reboot_notifier(&__notify_reboot_block);

	is_debug_register_icpu_ops(&is_debug_ops);

	spin_lock_init(&core->icpu_state_slock);

	pablo_i_subdev_set_icpu_dev(&core->pdev->dev);

	ICPU_INFO("pablo icpu probe done");

	return 0;

probe_fail:
	__teardown_core(pdev);

#if defined(CONFIG_PM)
	pm_runtime_disable(&pdev->dev);
#endif

	return -ENOMEM;
}

static int pablo_icpu_remove(struct platform_device *pdev)
{
	__teardown_core(pdev);

#if defined(CONFIG_PM)
	pm_runtime_disable(&pdev->dev);
#endif

	pablo_icpu_hw_remove(dev_get_platdata(&pdev->dev));

	pablo_icpu_debug_remove();

	atomic_notifier_chain_unregister(&panic_notifier_list, &__notify_panic_block);
	unregister_reboot_notifier(&__notify_reboot_block);
	iommu_unregister_device_fault_handler(&pdev->dev);

	pablo_icpu_sysfs_remove(&pdev->dev);

	icpu_firmware_remove();

	ICPU_INFO("pablo icpu remove done");

	return 0;
}

static struct platform_driver pablo_icpu_driver = {
	.probe		= pablo_icpu_probe,
	.remove		= pablo_icpu_remove,
	.driver = {
		.name	= "pablo-icpu",
		.owner	= THIS_MODULE,
		.pm	= &pablo_icpu_pm_ops,
		.of_match_table = pablo_icpu_match,
	}
};

struct platform_driver *pablo_icpu_get_platform_driver(void)
{
	return &pablo_icpu_driver;
}
KUNIT_EXPORT_SYMBOL(pablo_icpu_get_platform_driver);

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
/* set new pdata and return original */
void *pablo_icpu_test_set_platform_data(void *pdata)
{
	void *data;

	if (!core)
		return NULL;

	data = dev_get_platdata(&core->pdev->dev);

	core->pdev->dev.platform_data = pdata;

	return data;
}
KUNIT_EXPORT_SYMBOL(pablo_icpu_test_set_platform_data);

const struct dev_pm_ops *pablo_icpu_test_get_pm_ops(void)
{
	return &pablo_icpu_pm_ops;
}
KUNIT_EXPORT_SYMBOL(pablo_icpu_test_get_pm_ops);

struct notifier_block *pablo_icpu_test_get_panic_handler(void)
{
	return &__notify_panic_block;
}
KUNIT_EXPORT_SYMBOL(pablo_icpu_test_get_panic_handler);

struct notifier_block *pablo_icpu_test_get_reboot_handler(void)
{
	return &__notify_reboot_block;
}
KUNIT_EXPORT_SYMBOL(pablo_icpu_test_get_reboot_handler);

void *pablo_icpu_test_get_iommu_fault_handler(void)
{
	return __iommu_fault_handler;
}
KUNIT_EXPORT_SYMBOL(pablo_icpu_test_get_iommu_fault_handler);
#endif
