// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ALSA SoC - Samsung Abox driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include <linux/iommu.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/workqueue.h>
#include <linux/smc.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/panic_notifier.h>
#include <linux/sched/clock.h>
#include <linux/pinctrl/consumer.h>

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/samsung/abox.h>
#include <sound/samsung/vts.h>

#include <soc/samsung/exynos/debug-snapshot.h>
#include <soc/samsung/exynos-pmu-if.h>
#include <soc/samsung/exynos/exynos-itmon.h>
#include <soc/samsung/exynos-sci.h>
#if IS_ENABLED(CONFIG_SHM_IPC)
#include <soc/samsung/shm_ipc.h>
#endif

#include "abox_util.h"
#include "abox_dbg.h"
#include "abox_proc.h"
#include "abox_log.h"
#include "abox_dump.h"
#include "abox_gic.h"
#include "abox_failsafe.h"
#include "abox_if.h"
#include "abox_dma.h"
#include "abox_vdma.h"
#include "abox_shm.h"
#include "abox_effect.h"
#include "abox_cmpnt.h"
#include "abox_tplg.h"
#include "abox_core.h"
#include "abox_ipc.h"
#include "abox_memlog.h"
#include "abox_vss.h"
#include "abox.h"
#include "abox_pci.h"
#include "abox_oem.h"

#include <soc/samsung/exynos-smc.h>
#define EXYNOS_SET_CONN_TZPC (0)

#define ABOX_ARAM_CTRL			(0x040c)

#define BOOT_DONE_TIMEOUT_MS		(10000)
#define DRAM_TIMEOUT_NS			(10000000)
#define IPC_RETRY			(10)

#define BARGEIN_TRIGGERED_TIMEOUT_MS    (5000)

/* For only external static functions */
static struct abox_data *p_abox_data;

struct abox_data *abox_get_data(struct device *dev)
{
	while (dev && !is_abox(dev))
		dev = dev->parent;

	return dev ? dev_get_drvdata(dev) : NULL;
}

static int abox_iommu_fault_handler(struct iommu_fault *fault, void *token)
{
	struct abox_data *data = token;

	abox_dbg_print_gpr(data->dev, data);

	return 0;
}

static void abox_cpu_power(bool on);
static int abox_cpu_enable(bool enable);
static int abox_cpu_pm_ipc(struct abox_data *data, bool resume);
static void abox_boot_done(struct device *dev, unsigned int version);
static int abox_load_config_file(struct abox_data *data);

void abox_set_magic(struct abox_data *data, unsigned int val)
{
	writel(val, data->sram_base + SRAM_FIRMWARE_SIZE - sizeof(u32));
}

static void exynos_abox_panic_handler(void)
{
	static bool has_run;
	struct abox_data *data = p_abox_data;
	struct device *dev = data ? (data->dev ? data->dev : NULL) : NULL;

	abox_dbg(dev, "%s\n", __func__);

	if (abox_is_on() && dev) {
		if (has_run) {
			abox_info(dev, "already dumped\n");
			return;
		}
		has_run = true;

		abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_KERNEL, "panic");
		abox_set_magic(data, 0x504E4943);
		abox_cpu_enable(false);
		abox_cpu_power(false);
		abox_cpu_power(true);
		abox_cpu_enable(true);
		mdelay(100);
		abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_KERNEL, "panic");
	} else {
		abox_info(dev, "%s: dump is skipped due to no power\n",
				__func__);
	}
}

static int abox_panic_handler(struct notifier_block *nb,
			       unsigned long action, void *data)
{
	exynos_abox_panic_handler();
	return NOTIFY_OK;
}

static struct notifier_block abox_panic_notifier = {
	.notifier_call	= abox_panic_handler,
	.next		= NULL,
	.priority	= 0	/* priority: INT_MAX >= x >= 0 */
};

static void abox_wdt_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct abox_data *data = container_of(dwork, struct abox_data,
			wdt_work);
	struct device *dev_abox = data->dev;

	abox_dbg_print_gpr(dev_abox, data);
	abox_dbg_dump_mem(dev_abox, data, ABOX_DBG_DUMP_KERNEL, "watchdog");
	abox_failsafe_report(dev_abox, true);
}

static DECLARE_DELAYED_WORK(abox_wdt_work, abox_wdt_work_func);

static irqreturn_t abox_wdt_handler(int irq, void *dev_id)
{
	struct abox_data *data = dev_id;
	struct device *dev = data->dev;

	abox_err(dev, "abox watchdog timeout\n");

	if (abox_is_on()) {
		abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_KERNEL, "watchdog");
		writel(0x504E4943, data->sram_base + SRAM_FIRMWARE_SIZE -
				sizeof(u32));
		abox_cpu_enable(false);
		abox_cpu_power(false);
		abox_cpu_power(true);
		abox_cpu_enable(true);
		schedule_delayed_work(&data->wdt_work, msecs_to_jiffies(100));
	} else {
		abox_info(dev, "%s: no power\n", __func__);
	}

	return IRQ_HANDLED;
}

void abox_enable_wdt(struct abox_data *data)
{
	abox_gic_target_ap(data->dev_gic, IRQ_WDT);
	abox_gic_enable(data->dev_gic, IRQ_WDT, true);
}

static struct platform_driver samsung_abox_driver;

bool is_abox(struct device *dev)
{
	return (&samsung_abox_driver.driver) == dev->driver;
}

static BLOCKING_NOTIFIER_HEAD(abox_power_nh);

int abox_power_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&abox_power_nh, nb);
}

int abox_power_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&abox_power_nh, nb);
}

static int abox_power_notifier_call_chain(struct abox_data *data, bool en)
{
	return blocking_notifier_call_chain(&abox_power_nh, en, data);
}

static void abox_probe_quirks(struct abox_data *data, struct device_node *np)
{
	#define QUIRKS "samsung,quirks"
	#define DEC_MAP(id) {ABOX_QUIRK_STR_##id, ABOX_QUIRK_BIT_##id}

	static const struct {
		const char *str;
		unsigned int bit;
	} map[] = {
		DEC_MAP(ARAM_MODE),
		DEC_MAP(INT_SKEW),
		DEC_MAP(SILENT_RESET),
	};

	int i, ret;

	for (i = 0; i < ARRAY_SIZE(map); i++) {
		ret = of_property_match_string(np, QUIRKS, map[i].str);
		if (ret >= 0)
			data->quirks |= map[i].bit;
	}
}

void abox_enable_mclk(unsigned int on)
{
	struct abox_data *data = p_abox_data;
	int ret = 0;

	if (!abox_is_on()) {
		dev_info(data->dev, "%s(abox_is_off)\n", __func__);
		return;
	}

	abox_uaif0_cfg_gpio(on);
	ret = abox_disable_qchannel(data->dev, data, ABOX_BCLK_UAIF0, !!on);
	if(ret < 0)
		abox_warn(data->dev, "%s(%#x) failed: %d\n", __func__, ABOX_QCHANNEL_DISABLE, ret);
	dev_info(data->dev, "%s:ABOX_QCHANNEL_DISABLE=%08x\n", __func__,
			({regmap_read(data->regmap, ABOX_QCHANNEL_DISABLE, &on); on;}));
}
EXPORT_SYMBOL(abox_enable_mclk);

phys_addr_t abox_addr_to_phys_addr(struct abox_data *data, unsigned int addr)
{
	phys_addr_t ret;

	/* 0 is translated to 0 for convenience. */
	if (addr == 0)
		ret = 0;
	else if (addr < IOVA_DRAM_PARAMETER)
		ret = data->sram_phys + addr;
	else
		ret = iommu_iova_to_phys(data->iommu_domain, addr);

	return ret;
}

int abox_write_sysreg(unsigned int value, unsigned int offset)
{
	struct abox_data *data = p_abox_data;

	if (!data)
		return -ENODEV;

	if (offset >= data->sysreg_size)
		return -EINVAL;

	writel(value, data->sysreg_base + offset);

	return 0;
}
EXPORT_SYMBOL(abox_write_sysreg);

int abox_read_sysreg(unsigned int *value, unsigned int offset)
{
	struct abox_data *data = p_abox_data;

	if (!data)
		return -ENODEV;

	if (offset >= data->sysreg_size)
		return -EINVAL;

	*value = readl(data->sysreg_base + offset);

	return 0;
}
EXPORT_SYMBOL(abox_read_sysreg);

static void *abox_dram_addr_to_kernel_addr(struct abox_data *data,
		unsigned long iova)
{
	struct abox_iommu_mapping *m;
	unsigned long flags;
	void *ret = 0;

	spin_lock_irqsave(&data->iommu_lock, flags);
	list_for_each_entry(m, &data->iommu_maps, list) {
		if (m->iova <= iova && iova < m->iova + m->bytes) {
			ret = m->area + (iova - m->iova);
			break;
		}
	}
	spin_unlock_irqrestore(&data->iommu_lock, flags);

	return ret;
}

void *abox_addr_to_kernel_addr(struct abox_data *data, unsigned int addr)
{
	void *ret;

	/* 0 is translated to 0 for convenience. */
	if (addr == 0)
		ret = 0;
	else if (addr < IOVA_DRAM_PARAMETER)
		ret = data->sram_base + addr;
	else
		ret = abox_dram_addr_to_kernel_addr(data, addr);

	return ret;
}

phys_addr_t abox_iova_to_phys(struct device *dev, unsigned long iova)
{
	return abox_addr_to_phys_addr(dev_get_drvdata(dev), iova);
}
EXPORT_SYMBOL(abox_iova_to_phys);

void *abox_iova_to_virt(struct device *dev, unsigned long iova)
{
	return abox_addr_to_kernel_addr(dev_get_drvdata(dev), iova);
}
EXPORT_SYMBOL(abox_iova_to_virt);

int abox_of_get_addr(struct abox_data *data, struct device_node *np,
		const char *name, void **addr, dma_addr_t *dma, size_t *size)
{
	struct device *dev = data->dev;
	unsigned int area[3];
	int ret;

	abox_dbg(dev, "%s(%s)\n", __func__, name);

	ret = of_property_read_u32_array(np, name, area, ARRAY_SIZE(area));
	if (ret < 0) {
		abox_warn(dev, "Failed to read %s: %d\n", name, ret);
		goto out;
	}

	switch (area[0]) {
	case 0:
		*addr = data->sram_base;
		if (dma)
			*dma = area[1];
		break;
	default:
		abox_warn(dev, "%s: invalid area: %d\n", __func__, area[0]);
		fallthrough;
	case 1:
		*addr = data->dram_base;
		if (dma)
			*dma = area[1] + IOVA_DRAM_FIRMWARE;
		break;
	case 2:
#if IS_ENABLED(CONFIG_SHM_IPC)
		*addr = shm_get_vss_region();
#else
		*addr = 0;
#endif
		if (dma)
			*dma = area[1] + IOVA_VSS_FIRMWARE;
		break;
	case 3:
		*addr = 0;
		if (dma)
			*dma = area[1];
		break;
	}
	*addr += area[1];
	if (size)
		*size = area[2];
out:
	return ret;
}

static void abox_notify_bargein_detect_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data, notify_bargein_detect_work);
	struct device *dev = data->dev;

	dev_info(dev, "%s: bargein detection\n", __func__);
	vts_bargein_call_notifier();

	pm_wakeup_event(dev, BARGEIN_TRIGGERED_TIMEOUT_MS);
}

static bool __abox_ipc_queue_empty(struct abox_data *data)
{
	return (data->ipc_queue_end == data->ipc_queue_start);
}

static bool __abox_ipc_queue_full(struct abox_data *data)
{
	size_t length = ARRAY_SIZE(data->ipc_queue);

	return (((data->ipc_queue_end + 1) % length) == data->ipc_queue_start);
}

static int abox_ipc_queue_put(struct abox_data *data, struct device *dev,
		int hw_irq, const ABOX_IPC_MSG *msg, size_t size)
{
	spinlock_t *lock = &data->ipc_queue_lock;
	size_t length = ARRAY_SIZE(data->ipc_queue);
	struct abox_ipc *ipc;
	unsigned long flags;
	int ret;

	if (unlikely(sizeof(ipc->msg) < size))
		return -EINVAL;

	spin_lock_irqsave(lock, flags);
	if (!__abox_ipc_queue_full(data)) {
		ipc = &data->ipc_queue[data->ipc_queue_end];
		ipc->dev = dev;
		ipc->hw_irq = hw_irq;
		ipc->put_time = sched_clock();
		ipc->get_time = 0;
		memcpy(&ipc->msg, msg, size);
		ipc->size = size;
		data->ipc_queue_end = (data->ipc_queue_end + 1) % length;
		ret = 0;
	} else {
		ret = -EBUSY;
	}
	spin_unlock_irqrestore(lock, flags);

	return ret;
}

static int abox_ipc_queue_get(struct abox_data *data, struct abox_ipc *ipc)
{
	spinlock_t *lock = &data->ipc_queue_lock;
	size_t length = ARRAY_SIZE(data->ipc_queue);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(lock, flags);
	if (!__abox_ipc_queue_empty(data)) {
		struct abox_ipc *tmp;

		tmp = &data->ipc_queue[data->ipc_queue_start];
		tmp->get_time = sched_clock();
		*ipc = *tmp;
		data->ipc_queue_start = (data->ipc_queue_start + 1) % length;

		ret = 0;
	} else {
		ret = -ENODATA;
	}
	spin_unlock_irqrestore(lock, flags);

	return ret;
}

static bool abox_can_calliope_ipc(struct device *dev,
		struct abox_data *data)
{
	bool ret = true;

	switch (data->calliope_state) {
	case CALLIOPE_DISABLING:
	case CALLIOPE_ENABLED:
		break;
	case CALLIOPE_ENABLING:
		wait_event_timeout(data->ipc_wait_queue,
				data->calliope_state == CALLIOPE_ENABLED,
				abox_get_waiting_jiffies(true));
		if (data->calliope_state == CALLIOPE_ENABLED)
			break;
		fallthrough;
	case CALLIOPE_DISABLED:
	default:
		abox_warn(dev, "Invalid calliope state: %d\n",
				data->calliope_state);
		ret = false;
	}

	abox_dbg(dev, "%s: %d\n", __func__, ret);

	return ret;
}

static void abox_process_ipc(struct work_struct *work)
{
	static struct abox_ipc ipc;
	struct abox_data *data = container_of(work, struct abox_data, ipc_work);
	struct device *dev = data->dev;

	abox_dbg(dev, "%s: %d %d\n", __func__, data->ipc_queue_start,
			data->ipc_queue_end);

	pm_runtime_get_sync(dev);

	if (abox_can_calliope_ipc(dev, data)) {
		while (abox_ipc_queue_get(data, &ipc) == 0) {
			if (abox_ipc_send(dev, &ipc.msg, ipc.size, NULL, 0) < 0)
				abox_failsafe_report(dev, true);
		}
	}

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
}

static int abox_schedule_ipc(struct device *dev, struct abox_data *data,
		int hw_irq, const ABOX_IPC_MSG *msg, size_t size,
		bool atomic, bool sync)
{
	int retry = 0;
	int ret;

	abox_dbg(dev, "%s(%d, %zu, %d, %d)\n", __func__, hw_irq,
			size, atomic, sync);

	do {
		ret = abox_ipc_queue_put(data, dev, hw_irq, msg, size);
		queue_work(data->ipc_workqueue, &data->ipc_work);
		if (!atomic && sync)
			flush_work(&data->ipc_work);
		if (ret >= 0 || ret == -EINVAL)
			break;

		if (!atomic) {
			abox_info(dev, "%s: flush(%d)\n", __func__, retry);
			flush_work(&data->ipc_work);
		} else {
			abox_info(dev, "%s: delay(%d)\n", __func__, retry);
			mdelay(1);
		}
	} while (retry++ < IPC_RETRY);

	if (ret == -EINVAL) {
		abox_err(dev, "%s(%d): invalid message\n", __func__, hw_irq);
	} else if (ret < 0) {
		abox_err(dev, "%s(%d): ipc queue overflow\n", __func__, hw_irq);
		abox_failsafe_report(dev, true);
	}

	return ret;
}

int abox_request_ipc(struct device *dev,
		int hw_irq, const ABOX_IPC_MSG *msg,
		size_t size, int atomic, int sync)
{
	struct abox_data *data = dev_get_drvdata(dev);
	int ret;

	if (atomic && sync) {
		ret = abox_ipc_send_atomic(dev, msg, size, NULL, 0);
	} else {
		ret = abox_schedule_ipc(dev, data, hw_irq, msg, size,
				!!atomic, !!sync);
	}

	return ret;
}
EXPORT_SYMBOL(abox_request_ipc);

bool abox_is_on(void)
{
	return p_abox_data && p_abox_data->enabled;
}
EXPORT_SYMBOL(abox_is_on);

int abox_register_bclk_usage(struct device *dev, struct abox_data *data,
		enum abox_dai dai_id, unsigned int rate, unsigned int channels,
		unsigned int width)
{
	unsigned long target_pll, audif_rate;
	int id = dai_id - ABOX_UAIF0;
	int ret = 0;
	int i;

	abox_dbg(dev, "%s(%d, %d)\n", __func__, id, rate);

	if (id < 0 || id >= ABOX_DAI_COUNT) {
		abox_err(dev, "invalid dai_id: %d\n", dai_id);
		return -EINVAL;
	}

	if (rate == 0) {
		data->audif_rates[id] = 0;
		return 0;
	}

	target_pll = ((rate % 44100) == 0) ? AUD_PLL_RATE_HZ_FOR_44100 :
			AUD_PLL_RATE_HZ_FOR_48000;

	if (abs(target_pll - clk_get_rate(data->clk_pll)) > 100) {
		abox_warn(dev, "current pll_aud: %lu(target: %lu)\n",
			clk_get_rate(data->clk_pll), target_pll);
	}

	if (data->uaif_max_div <= 16) {
		/*LSI codec uses 24.576Mhz*/
		audif_rate = 49152000;
	} else if (data->uaif_max_div <= 32) {
		if ((rate % 44100) == 0)
			audif_rate = ((rate > 176400) ? 352800 : 176400) *
					width * 2;
		else
			audif_rate = ((rate > 192000) ? 384000 : 192000) *
					width * 2;

		while (audif_rate / rate / channels / width >
				data->uaif_max_div)
			audif_rate /= 2;
	} else {
		unsigned long clk_width = 96; /* LCM of 24 and 32 */
		unsigned long clk_channels = 2;

		if ((rate % 44100) == 0)
			audif_rate = 352800 * clk_width * clk_channels;
		else
			audif_rate = 384000 * clk_width * clk_channels;

		if (audif_rate < rate * width * channels)
			audif_rate = rate * width * channels;
	}

	data->audif_rates[id] = audif_rate;

	for (i = 0; i < ARRAY_SIZE(data->audif_rates); i++) {
		if (data->audif_rates[i] > 0 &&
				data->audif_rates[i] > audif_rate) {
			audif_rate = data->audif_rates[i];
		}
	}

	ret = clk_set_rate(data->clk_audif, audif_rate);
	if (ret < 0)
		abox_err(dev, "Failed to set audif clock: %d\n", ret);

	abox_info(dev, "audif clock: %lu\n", clk_get_rate(data->clk_audif));

	return ret;
}

static bool abox_timer_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ABOX_TIMER_CTRL0(0):
	case ABOX_TIMER_CTRL1(0):
	case ABOX_TIMER_PRESET_LSB(0):
	case ABOX_TIMER_CTRL0(1):
	case ABOX_TIMER_CTRL1(1):
	case ABOX_TIMER_PRESET_LSB(1):
	case ABOX_TIMER_CTRL0(2):
	case ABOX_TIMER_CTRL1(2):
	case ABOX_TIMER_PRESET_LSB(2):
	case ABOX_TIMER_CTRL0(3):
	case ABOX_TIMER_CTRL1(3):
	case ABOX_TIMER_PRESET_LSB(3):
		return true;
	default:
		return false;
	}
}

static bool abox_timer_readable_reg(struct device *dev, unsigned int reg)
{
	return abox_timer_volatile_reg(dev, reg);
}

static bool abox_timer_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ABOX_TIMER_CTRL0(0):
	case ABOX_TIMER_CTRL1(0):
	case ABOX_TIMER_CTRL0(1):
	case ABOX_TIMER_CTRL1(1):
	case ABOX_TIMER_CTRL0(2):
	case ABOX_TIMER_CTRL1(2):
	case ABOX_TIMER_CTRL0(3):
	case ABOX_TIMER_CTRL1(3):
		return true;
	default:
		return false;
	}
}

static struct regmap_config abox_timer_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = ABOX_TIMER_MAX_REGISTERS,
	.volatile_reg = abox_timer_volatile_reg,
	.readable_reg = abox_timer_readable_reg,
	.writeable_reg = abox_timer_writeable_reg,
	.cache_type = REGCACHE_RBTREE,
	.fast_io = true,
};

int abox_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params)
{
	return abox_cmpnt_hw_params_fixup_helper(rtd, params);
}
EXPORT_SYMBOL(abox_hw_params_fixup_helper);

unsigned int abox_get_requiring_int_freq_in_khz(void)
{
	struct abox_data *data = p_abox_data;

	if (data == NULL)
		return 0;

	return abox_qos_get_target(data->dev, ABOX_QOS_INT);
}
EXPORT_SYMBOL(abox_get_requiring_int_freq_in_khz);

unsigned int abox_get_requiring_mif_freq_in_khz(void)
{
	struct abox_data *data = p_abox_data;

	if (data == NULL)
		return 0;

	return abox_qos_get_target(data->dev, ABOX_QOS_MIF);
}
EXPORT_SYMBOL(abox_get_requiring_mif_freq_in_khz);

unsigned int abox_get_requiring_aud_freq_in_khz(void)
{
	struct abox_data *data = p_abox_data;

	if (data == NULL)
		return 0;

	return abox_qos_get_target(data->dev, ABOX_QOS_AUD);
}
EXPORT_SYMBOL(abox_get_requiring_aud_freq_in_khz);

unsigned int abox_get_routing_path(void)
{
	struct abox_data *data = p_abox_data;

	if (data == NULL)
		return 0;

	return data->sound_type;
}
EXPORT_SYMBOL(abox_get_routing_path);

bool abox_cpu_gear_idle(struct device *dev, unsigned int id)
{
	return !abox_qos_is_active(dev, ABOX_QOS_AUD, id);
}

static bool __maybe_unused abox_is_clearable(struct device *dev,
		struct abox_data *data)
{
	return abox_cpu_gear_idle(dev, ABOX_CPU_GEAR_ABSOLUTE) &&
			data->audio_mode != MODE_IN_CALL;
}

bool abox_get_call_state(void)
{
	struct abox_data *data = p_abox_data;

	if (data == NULL)
		return 0;

	return (abox_qos_is_active(data->dev, ABOX_QOS_AUD,
				ABOX_CPU_GEAR_ABSOLUTE) ||
			abox_qos_is_active(data->dev, ABOX_QOS_AUD,
				ABOX_CPU_GEAR_CALL_VSS) ||
			abox_qos_is_active(data->dev, ABOX_QOS_AUD,
				ABOX_CPU_GEAR_CALL_KERNEL));
}
EXPORT_SYMBOL(abox_get_call_state);

static void abox_check_cpu_request(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int old_val, unsigned int val)
{
	struct device *dev_abox = data->dev;

	if (id != ABOX_CPU_GEAR_BOOT)
		return;

	if (data->calliope_state == CALLIOPE_ENABLING)
		abox_boot_done(dev_abox, data->calliope_version);

	if (!old_val && val) {
		abox_dbg(dev, "%s(%#x): on\n", __func__, id);
		pm_wakeup_event(dev_abox, BOOT_DONE_TIMEOUT_MS);
	} else if (old_val && !val) {
		abox_dbg(dev, "%s(%#x): off\n", __func__, id);
		pm_relax(dev_abox);
	}
}

static void abox_notify_cpu_gear(struct abox_data *data)
{
	struct device *dev = data->dev;
	int aud = abox_qos_get_request(dev, ABOX_QOS_AUD);
	int max = abox_qos_get_request(dev, ABOX_QOS_AUD_MAX);
	unsigned long long time = sched_clock();
	unsigned long rem = do_div(time, NSEC_PER_SEC);
	unsigned int freq = min_not_zero(aud, max);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	abox_info(dev, "pm qos aud: %dkHz, max: %dkHz(at %llu.%lu)\n", aud, max, time, rem);

	switch (data->calliope_state) {
	case CALLIOPE_ENABLING:
	case CALLIOPE_ENABLED:
		abox_dbg(dev, "%s(%u)\n", __func__, freq);

		msg.ipcid = IPC_SYSTEM;
		system_msg->msgtype = ABOX_CHANGED_GEAR;
		system_msg->param1 = (int)freq;
		system_msg->param2 = (int)time; /* SEC */
		system_msg->param3 = (int)rem; /* NSEC */
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
		break;
	case CALLIOPE_DISABLING:
	case CALLIOPE_DISABLED:
	default:
		/* notification to passing by context is not needed */
		break;
	}
}

static int abox_quirk_aud_int_skew(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int val, const char *name)
{
	const unsigned int SKEW_INT_VAL = 200000;
	const unsigned int SKEW_INT_ID = DEFAULT_INT_FREQ_ID + 1;
	const char *SKEW_INT_NAME = "skew";
	unsigned int aud_max = data->pm_qos_aud[0];
	unsigned int old_target, new_target;
	int ret;

	abox_dbg(dev, "%s(%u, %u, %s)\n", __func__, id, val, name);

	old_target = abox_qos_get_target(dev, ABOX_QOS_AUD);
	if (old_target < aud_max && val >= aud_max)
		abox_qos_request_int(dev, SKEW_INT_ID, SKEW_INT_VAL,
				SKEW_INT_NAME);

	ret = abox_qos_request_aud(dev, id, val, name);

	new_target = abox_qos_get_target(dev, ABOX_QOS_AUD);
	if (old_target >= aud_max && new_target < aud_max)
		abox_qos_request_int(dev, SKEW_INT_ID, 0, SKEW_INT_NAME);

	return ret;
}

int abox_request_cpu_gear(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int level, const char *name)
{
	unsigned int old_val, val;
	int ret;

	if (level < 1)
		val = 0;
	else if (level <= data->cpu_gear_min)
		val = data->pm_qos_aud[level - 1];
	else if (level <= ABOX_CPU_GEAR_MIN)
		val = 0;
	else
		val = level;

	old_val = abox_qos_get_value(dev, ABOX_QOS_AUD, id);
	if (abox_test_quirk(data, ABOX_QUIRK_BIT_INT_SKEW))
		ret = abox_quirk_aud_int_skew(dev, data, id, val, name);
	else
		ret = abox_qos_request_aud(dev, id, val, name);
	abox_check_cpu_request(dev, data, id, old_val, val);

	return ret;
}

void abox_cpu_gear_barrier(void)
{
	abox_qos_complete();
}

int abox_request_cpu_gear_sync(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int level, const char *name)
{
	int ret = abox_request_cpu_gear(dev, data, id, level, name);

	abox_cpu_gear_barrier();
	return ret;
}

static void abox_set_boost_qos(struct abox_data *data)
{
	abox_qos_apply_aud_base(data->dev, data->pm_qos_aud[0]);
}

static void abox_set_base_qos(struct abox_data *data)
{
	abox_qos_apply_aud_base(data->dev, data->pm_qos_stable_min ? : 0);
}

static void abox_clear_base_qos(struct abox_data *data)
{
	abox_qos_apply_aud_base(data->dev, 0);
}

int abox_request_power(struct device *dev, unsigned int id, const char *name)
{
	struct abox_data *data = dev_get_drvdata(dev);

	return abox_request_cpu_gear(dev, data, id, data->cpu_gear_min, name);
}
EXPORT_SYMBOL(abox_request_power);

int abox_release_power(struct device *dev, unsigned int id, const char *name)
{
	struct abox_data *data = dev_get_drvdata(dev);

	return abox_request_cpu_gear(dev, data, id, 0, name);
}
EXPORT_SYMBOL(abox_release_power);

void abox_synchronize_power(void)
{
	abox_cpu_gear_barrier();
}
EXPORT_SYMBOL(abox_synchronize_power);

void abox_clear_cpu_gear_requests(struct device *dev)
{
	abox_qos_clear(dev, ABOX_QOS_AUD);
}

void abox_clear_mif_requests(struct device *dev)
{
	abox_qos_clear(dev, ABOX_QOS_MIF);
}

static int abox_check_dram_status(struct device *dev, struct abox_data *data,
		bool on)
{
	struct regmap *regmap = data->regmap;
	u64 timeout = local_clock() + DRAM_TIMEOUT_NS;
	unsigned int val = 0;
	int ret;

	abox_dbg(dev, "%s(%d)\n", __func__, on);

	do {
		ret = regmap_read(regmap, ABOX_SYSPOWER_STATUS, &val);
		val &= ABOX_SYSPOWER_STATUS_MASK;
		if (local_clock() > timeout) {
			abox_warn(dev, "syspower status timeout: %u\n", val);
			abox_dbg_dump_simple(dev, data,
					"syspower status timeout");
			ret = -EPERM;
			break;
		}
	} while ((ret < 0) || ((!!val) != on));

	return ret;
}

void abox_request_dram_on(struct device *dev, unsigned int id, bool on)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct regmap *regmap = data->regmap;
	struct abox_dram_request *request;
	size_t len = ARRAY_SIZE(data->dram_requests);
	unsigned int val = 0;
	int ret;

	abox_dbg(dev, "%s(%#x, %d)\n", __func__, id, on);

	for (request = data->dram_requests; request - data->dram_requests <
			len && request->id && request->id != id; request++) {
	}

	if (request - data->dram_requests >= len) {
		abox_err(dev, "too many dram requests: %#x, %d\n", id, on);
		return;
	}

	request->on = on;
	request->id = id;
	request->updated = local_clock();

	for (request = data->dram_requests; request - data->dram_requests <
			len && request->id; request++) {
		if (request->on) {
			val = ABOX_SYSPOWER_CTRL_MASK;
			break;
		}
	}

	ret = regmap_update_bits(regmap, ABOX_SYSPOWER_CTRL, ABOX_SYSPOWER_CTRL_MASK, val);
	if (ret < 0) {
		abox_err(dev, "syspower write failed: %d\n", ret);
		return;
	}

	abox_check_dram_status(dev, data, on);
}
EXPORT_SYMBOL(abox_request_dram_on);

static int abox_iommu_map_prot(struct device *dev, unsigned long iova,
		phys_addr_t addr, size_t bytes, void *area, int prot)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_iommu_mapping *mapping;
	unsigned long flags;
	int ret;

	abox_dbg(dev, "%s(%#lx, %#zx)\n", __func__, iova, bytes);

	if (!area) {
		abox_warn(dev, "%s(%#lx): no virtual address\n", __func__, iova);
		area = phys_to_virt(addr);
	}

	list_for_each_entry(mapping, &data->iommu_maps, list) {
		if (iova == mapping->iova) {
			abox_info(dev, "already mapped(%#lx)\n", iova);
			ret = 0;
			goto out;
		}
	}

	mapping = devm_kmalloc(dev, sizeof(*mapping), GFP_KERNEL);
	if (!mapping) {
		ret = -ENOMEM;
		goto out;
	}

	ret = iommu_map(data->iommu_domain, iova, addr, bytes, prot, GFP_KERNEL);
	if (ret < 0) {
		devm_kfree(dev, mapping);
		abox_err(dev, "iommu_map(%#lx) fail: %d\n", iova, ret);
		goto out;
	}

	mapping->iova = iova;
	mapping->addr = addr;
	mapping->area = area;
	mapping->bytes = bytes;

	spin_lock_irqsave(&data->iommu_lock, flags);
	list_add(&mapping->list, &data->iommu_maps);
	spin_unlock_irqrestore(&data->iommu_lock, flags);
out:
	return ret;
}

int abox_iommu_map(struct device *dev, unsigned long iova,
		phys_addr_t addr, size_t bytes, void *area)
{
	return abox_iommu_map_prot(dev, iova, addr, bytes, area, 0);
}
EXPORT_SYMBOL(abox_iommu_map);

int abox_iommu_map_cache(struct device *dev, unsigned long iova,
		phys_addr_t addr, size_t bytes, void *area)
{
	return abox_iommu_map_prot(dev, iova, addr, bytes, area, IOMMU_CACHE);
}
EXPORT_SYMBOL(abox_iommu_map_cache);

int abox_iommu_map_sg(struct device *dev, unsigned long iova,
		struct scatterlist *sg, unsigned int nents,
		int prot, size_t bytes, void *area)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_iommu_mapping *mapping;
	unsigned long flags;
	int ret;

	abox_dbg(dev, "%s(%#lx, %#zx)\n", __func__, iova, bytes);

	if (!area)
		abox_warn(dev, "%s(%#lx): no virtual address\n", __func__, iova);

	list_for_each_entry(mapping, &data->iommu_maps, list) {
		if (iova == mapping->iova) {
			abox_info(dev, "already mapped(%#lx)\n", iova);
			ret = 0;
			goto out;
		}
	}

	mapping = devm_kmalloc(dev, sizeof(*mapping), GFP_KERNEL);
	if (!mapping) {
		ret = -ENOMEM;
		goto out;
	}

	ret = iommu_map_sg(data->iommu_domain, iova, sg, nents, prot, GFP_KERNEL);
	if (ret < 0) {
		devm_kfree(dev, mapping);
		abox_err(dev, "iommu_map_sg(%#lx) fail: %d\n", iova, ret);
		goto out;
	}

	mapping->iova = iova;
	mapping->addr = 0;
	mapping->area = area;
	mapping->bytes = bytes;

	spin_lock_irqsave(&data->iommu_lock, flags);
	list_add(&mapping->list, &data->iommu_maps);
	spin_unlock_irqrestore(&data->iommu_lock, flags);
out:
	return ret;
}
EXPORT_SYMBOL(abox_iommu_map_sg);

int abox_iommu_unmap(struct device *dev, unsigned long iova)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct iommu_domain *domain = data->iommu_domain;
	struct abox_iommu_mapping *mapping;
	unsigned long flags;
	int ret = 0;

	abox_dbg(dev, "%s(%#lx)\n", __func__, iova);

	spin_lock_irqsave(&data->iommu_lock, flags);
	list_for_each_entry(mapping, &data->iommu_maps, list) {
		if (iova != mapping->iova)
			continue;

		list_del(&mapping->list);

		ret = iommu_unmap(domain, iova, mapping->bytes);
		if (ret < 0)
			abox_err(dev, "iommu_unmap(%#lx) fail: %d\n", iova, ret);

		devm_kfree(dev, mapping);
		break;
	}
	spin_unlock_irqrestore(&data->iommu_lock, flags);

	return ret;
}
EXPORT_SYMBOL(abox_iommu_unmap);

int abox_register_ipc_handler(struct device *dev, int ipc_id,
		abox_ipc_handler_t ipc_handler, void *dev_id)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_ipc_action *action = NULL;
	bool new_handler = true;

	if (ipc_id >= IPC_ID_COUNT)
		return -EINVAL;

	list_for_each_entry(action, &data->ipc_actions, list) {
		if (action->ipc_id == ipc_id && action->data == dev_id) {
			new_handler = false;
			break;
		}
	}

	if (new_handler) {
		action = devm_kzalloc(dev, sizeof(*action), GFP_KERNEL);
		if (IS_ERR_OR_NULL(action)) {
			abox_err(dev, "%s: kmalloc fail\n", __func__);
			return -ENOMEM;
		}
		action->dev = dev;
		action->ipc_id = ipc_id;
		action->data = dev_id;
		list_add_tail(&action->list, &data->ipc_actions);
	}

	action->handler = ipc_handler;

	return 0;
}
EXPORT_SYMBOL(abox_register_ipc_handler);

static int abox_component_control_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	struct ABOX_COMPONENT_CONTROL *control = value->control;
	int i;
	const char *c;

	abox_dbg(dev, "%s(%s)\n", __func__, kcontrol->id.name);

	switch (control->type) {
	case ABOX_CONTROL_INT:
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
		uinfo->count = control->count;
		uinfo->value.integer.min = control->min;
		uinfo->value.integer.max = control->max;
		break;
	case ABOX_CONTROL_ENUM:
		uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
		uinfo->count = control->count;
		uinfo->value.enumerated.items = control->max;
		if (uinfo->value.enumerated.item >= control->max)
			uinfo->value.enumerated.item = control->max - 1;
		for (c = control->texts.addr, i = 0; i <
				uinfo->value.enumerated.item; ++i, ++c) {
			for (; *c != '\0'; ++c)
				;
		}
		strlcpy(uinfo->value.enumerated.name, c,
			sizeof(uinfo->value.enumerated.name));
		break;
	case ABOX_CONTROL_BYTE:
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
		uinfo->count = control->count;
		break;
	default:
		abox_err(dev, "%s(%s): invalid type:%d\n", __func__,
				kcontrol->id.name, control->type);
		return -EINVAL;
	}

	return 0;
}

static ABOX_IPC_MSG abox_component_control_get_msg;

static int abox_component_control_get_cache(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	int i;

	for (i = 0; i < value->control->count; i++) {
		switch (value->control->type) {
		case ABOX_CONTROL_INT:
			ucontrol->value.integer.value[i] = value->cache[i];
			break;
		case ABOX_CONTROL_ENUM:
			ucontrol->value.enumerated.item[i] = value->cache[i];
			break;
		case ABOX_CONTROL_BYTE:
			ucontrol->value.bytes.data[i] = value->cache[i];
			break;
		default:
			abox_err(dev, "%s(%s): invalid type:%d\n", __func__,
					kcontrol->id.name,
					value->control->type);
			break;
		}
	}

	return 0;
}

static int abox_component_control_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	ABOX_IPC_MSG *msg = &abox_component_control_get_msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg->msg.system;
	int i, ret;

	abox_dbg(dev, "%s\n", __func__);

	if (value->cache_only) {
		abox_component_control_get_cache(kcontrol, ucontrol);
		return 0;
	}

	msg->ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_REQUEST_COMPONENT_CONTROL;
	system_msg->param1 = value->desc->id;
	system_msg->param2 = value->control->id;
	ret = abox_request_ipc(dev, msg->ipcid, msg, sizeof(*msg), 0, 0);
	if (ret < 0)
		return ret;

	ret = wait_event_timeout(data->ipc_wait_queue,
			system_msg->msgtype == ABOX_REPORT_COMPONENT_CONTROL,
			abox_get_waiting_jiffies(true));
	if (system_msg->msgtype != ABOX_REPORT_COMPONENT_CONTROL)
		return -ETIME;

	for (i = 0; i < value->control->count; i++) {
		switch (value->control->type) {
		case ABOX_CONTROL_INT:
			ucontrol->value.integer.value[i] =
					system_msg->bundle.param_s32[i];
			break;
		case ABOX_CONTROL_ENUM:
			ucontrol->value.enumerated.item[i] =
					system_msg->bundle.param_s32[i];
			break;
		case ABOX_CONTROL_BYTE:
			ucontrol->value.bytes.data[i] =
					system_msg->bundle.param_bundle[i];
			break;
		default:
			abox_err(dev, "%s(%s): invalid type:%d\n", __func__,
					kcontrol->id.name,
					value->control->type);
			break;
		}
	}

	return 0;
}

static int abox_component_control_put_ipc(struct device *dev,
		struct abox_component_kcontrol_value *value)
{
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	int i;

	abox_dbg(dev, "%s\n", __func__);

	for (i = 0; i < value->control->count; i++) {
		int val = value->cache[i];
		char *name = value->control->name;

		switch (value->control->type) {
		case ABOX_CONTROL_INT:
			system_msg->bundle.param_s32[i] = val;
			abox_dbg(dev, "%s: %s[%d] = %d", __func__, name, i, val);
			break;
		case ABOX_CONTROL_ENUM:
			system_msg->bundle.param_s32[i] = val;
			abox_dbg(dev, "%s: %s[%d] = %d", __func__, name, i, val);
			break;
		case ABOX_CONTROL_BYTE:
			system_msg->bundle.param_bundle[i] = val;
			abox_dbg(dev, "%s: %s[%d] = %d", __func__, name, i, val);
			break;
		default:
			abox_err(dev, "%s(%s): invalid type:%d\n", __func__,
					name, value->control->type);
			break;
		}
	}

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_UPDATE_COMPONENT_CONTROL;
	system_msg->param1 = value->desc->id;
	system_msg->param2 = value->control->id;

	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int abox_component_control_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	int i;

	abox_dbg(dev, "%s\n", __func__);

	for (i = 0; i < value->control->count; i++) {
		int val = (int)ucontrol->value.integer.value[i];
		char *name = kcontrol->id.name;

		if (val < value->control->min || val > value->control->max) {
			abox_err(dev, "%s: value[%d]=%d, min=%d, max=%d\n",
					kcontrol->id.name, i, val,
					value->control->min,
					value->control->max);
			return -EINVAL;
		}

		value->cache[i] = val;
		abox_dbg(dev, "%s: %s[%d] <= %d", __func__, name, i, val);
	}

	return abox_component_control_put_ipc(dev, value);
}

#define ABOX_COMPONENT_KCONTROL(xname, xdesc, xcontrol)	\
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = abox_component_control_info, \
	.get = abox_component_control_get, \
	.put = abox_component_control_put, \
	.private_value = \
		(unsigned long)&(struct abox_component_kcontrol_value) \
		{.desc = xdesc, .control = xcontrol} }

struct snd_kcontrol_new abox_component_kcontrols[] = {
	ABOX_COMPONENT_KCONTROL(NULL, NULL, NULL),
};

static int abox_register_control(struct device *dev,
		struct abox_data *data, struct abox_component *com,
		struct ABOX_COMPONENT_CONTROL *control)
{
	struct ABOX_COMPONENT_DESCRIPTIOR *desc = com->desc;
	struct abox_component_kcontrol_value *value;
	int len, ret;

	value = devm_kzalloc(dev, sizeof(*value) + (control->count *
			sizeof(value->cache[0])), GFP_KERNEL);
	if (!value)
		return -ENOMEM;

	if (control->texts.aaddr) {
		char *texts, *addr;
		size_t size;

		texts = abox_addr_to_kernel_addr(data, control->texts.aaddr);
		size = strlen(texts) + 1;
		addr = devm_kmalloc(dev, size + 1, GFP_KERNEL);
		texts = memcpy(addr, texts, size);
		texts[size] = '\0';
		for (len = 0; strsep(&addr, ","); len++)
			;
		if (control->max > len) {
			abox_dbg(dev, "%s: incomplete enumeration. expected=%d, received=%d\n",
					control->name, control->max, len);
			control->max = len;
		}
		control->texts.addr = texts;
	}

	value->desc = desc;
	value->control = control;

	if (strlen(desc->name))
		abox_component_kcontrols[0].name = kasprintf(GFP_KERNEL,
				"%s %s", desc->name, control->name);
	else
		abox_component_kcontrols[0].name = kasprintf(GFP_KERNEL,
				"%s", control->name);
	abox_component_kcontrols[0].private_value = (unsigned long)value;
	if (data->cmpnt && data->cmpnt->card) {
		ret = snd_soc_add_component_controls(data->cmpnt,
				abox_component_kcontrols, 1);
		if (ret >= 0)
			list_add_tail(&value->list, &com->value_list);
		else
			devm_kfree(dev, value);
	} else {
		ret = 0;
		devm_kfree(dev, value);
	}
	kfree_const(abox_component_kcontrols[0].name);

	return ret;
}

static void abox_register_component_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			register_component_work);
	struct device *dev = data->dev;
	struct abox_component *com;
	size_t len = ARRAY_SIZE(data->components);
	int ret;

	abox_dbg(dev, "%s\n", __func__);

	for (com = data->components; com - data->components < len; com++) {
		struct ABOX_COMPONENT_DESCRIPTIOR *desc = com->desc;
		struct ABOX_COMPONENT_CONTROL *ctrl, *first;
		size_t ctrl_len, size;

		if (!desc || com->registered)
			continue;

		size = sizeof(*desc) + (sizeof(desc->controls[0]) *
				desc->control_count);
		com->desc = devm_kmemdup(dev, desc, size, GFP_KERNEL);
		if (!com->desc) {
			abox_err(dev, "%s: %s: alloc fail\n", __func__,
					desc->name);
			com->desc = desc;
			continue;
		}
		devm_kfree(dev, desc);
		desc = com->desc;
		first = desc->controls;
		ctrl_len = desc->control_count;

		for (ctrl = first; ctrl - first < ctrl_len; ctrl++) {
			ret = abox_register_control(dev, data, com, ctrl);
			if (ret < 0)
				abox_err(dev, "%s: %s register failed (%d)\n",
						__func__, ctrl->name, ret);
		}

		WRITE_ONCE(com->registered, true);
		abox_info(dev, "%s: %s registered\n", __func__, desc->name);
	}
}

static int abox_register_component(struct device *dev,
		struct ABOX_COMPONENT_DESCRIPTIOR *desc)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_component *com;
	size_t len = ARRAY_SIZE(data->components);
	int ret = 0;

	abox_dbg(dev, "%s(%d, %s)\n", __func__, desc->id, desc->name);

	for (com = data->components; com - data->components < len; com++) {
		struct ABOX_COMPONENT_DESCRIPTIOR *temp;
		size_t size;

		if (com->desc && !strcmp(com->desc->name, desc->name))
			break;

		if (com->desc)
			continue;

		size = sizeof(*desc) + (sizeof(desc->controls[0]) *
				desc->control_count);
		temp = devm_kmemdup(dev, desc, size, GFP_ATOMIC);
		if (!temp) {
			abox_err(dev, "%s: %s register fail\n", __func__,
					desc->name);
			ret = -ENOMEM;
		}
		INIT_LIST_HEAD(&com->value_list);
		WRITE_ONCE(com->desc, temp);
		schedule_work(&data->register_component_work);
		break;
	}

	return ret;
}

static void abox_restore_component_control(struct device *dev,
		struct abox_component_kcontrol_value *value)
{
	int count = value->control->count;
	int *val;

	for (val = value->cache; val - value->cache < count; val++) {
		if (*val) {
			abox_component_control_put_ipc(dev, value);
			break;
		}
	}
	value->cache_only = false;
}

static void abox_restore_components(struct device *dev, struct abox_data *data)
{
	struct abox_component *com;
	struct abox_component_kcontrol_value *value;
	size_t len = ARRAY_SIZE(data->components);

	abox_dbg(dev, "%s\n", __func__);

	for (com = data->components; com - data->components < len; com++) {
		if (!com->registered)
			break;
		list_for_each_entry(value, &com->value_list, list) {
			abox_restore_component_control(dev, value);
		}
	}
}

static void abox_cache_components(struct device *dev, struct abox_data *data)
{
	struct abox_component *com;
	struct abox_component_kcontrol_value *value;
	size_t len = ARRAY_SIZE(data->components);

	abox_dbg(dev, "%s\n", __func__);

	for (com = data->components; com - data->components < len; com++) {
		if (!com->registered)
			break;
		list_for_each_entry(value, &com->value_list, list) {
			value->cache_only = true;
		}
	}
}

static bool abox_is_calliope_incompatible(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	memcpy_toio(&msg, data->sram_base + 0x30040, 0x3C);

	return ((system_msg->param3 >> 24) == 'A');
}

static int abox_set_profiling_ipc(struct abox_data *data)
{
	struct device *dev = data->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	bool en = !data->no_profiling;

	abox_dbg(dev, "%s profiling\n", en ? "enable" : "disable");

	/* Profiler is enabled by default. */
	if (en)
		return 0;

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_REQUEST_DEBUG;
	system_msg->param1 = en;
	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

int abox_set_profiling(int en)
{
	struct abox_data *data = p_abox_data;

	if (!data)
		return -EAGAIN;

	data->no_profiling = !en;
	return abox_set_profiling_ipc(data);
}
EXPORT_SYMBOL(abox_set_profiling);

unsigned long abox_get_waiting_ns(bool coarse)
{
	unsigned long wait;

	if (!p_abox_data)
		return 0UL;

	if (p_abox_data->error)
		wait = 0UL;
	else
		wait = coarse ? 1000000000UL : 100000000UL; /* 1000ms or 100ms */

	return wait;
}
EXPORT_SYMBOL_GPL(abox_get_waiting_ns);

static void abox_sram_vts_request_work_func(struct work_struct *work)
{
	struct abox_sram_vts *sram_vts = container_of(work,
			struct abox_sram_vts, request_work);
	struct abox_data *data = container_of(sram_vts,
			struct abox_data, sram_vts);
	ABOX_IPC_MSG msg;
	int ret;

	if (sram_vts->enable) {
		if (sram_vts->enabled)
			return;

		sram_vts->enabled = true;
		ret = pm_runtime_get_sync(sram_vts->dev);
		if (ret < 0) {
			abox_err(data->dev, "Falied to get %s: %d\n",
					"vts sram", ret);
			return;
		}
		msg.ipcid = IPC_SYSTEM;
		msg.msg.system.msgtype = ABOX_READY_SRAM;
		abox_request_ipc(data->dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
		abox_info(data->dev, "request %s\n", "vts sram");
	} else {
		if (!sram_vts->enabled)
			return;

		sram_vts->enabled = false;
		ret = pm_runtime_put(sram_vts->dev);
		if (ret < 0) {
			abox_err(data->dev, "Falied to put %s: %d\n",
					"vts sram", ret);
			return;
		}
		abox_info(data->dev, "release %s\n", "vts sram");
	}
}

static void abox_request_sram_vts(struct abox_data *data)
{
	if (!data->sram_vts.dev)
		return;

	data->sram_vts.enable = true;
	queue_work(system_highpri_wq, &data->sram_vts.request_work);
}

static void abox_release_sram_vts(struct abox_data *data)
{
	if (!data->sram_vts.dev)
		return;

	data->sram_vts.enable = false;
	queue_work(system_highpri_wq, &data->sram_vts.request_work);
}

static int abox_probe_sram_vts(struct abox_data *data)
{
	struct abox_sram_vts *sram_vts = &data->sram_vts;
	struct device *dev = data->dev;
	struct device_node *np = dev->of_node;
	struct device_node *np_tmp;
	struct platform_device *pdev_tmp;

	np_tmp = of_parse_phandle(np, "samsung,abox-vts", 0);
	if (!np_tmp)
		return 0;

	pdev_tmp = of_find_device_by_node(np_tmp);
	of_node_put(np_tmp);
	if (!pdev_tmp) {
		abox_err(dev, "Failed to get abox-vts platform device\n");
		return -EPROBE_DEFER;
	}

	sram_vts->dev = &pdev_tmp->dev;
	put_device(&pdev_tmp->dev);
	INIT_WORK(&sram_vts->request_work, abox_sram_vts_request_work_func);
	abox_info(dev, "%s initialized\n", "vts sram");
	return 0;
}

static void abox_pcmc_request_work_func(struct work_struct *work)
{
	struct abox_pcmc *pcmc = container_of(work, struct abox_pcmc, request_work);
	struct abox_data *data = container_of(pcmc, struct abox_data, pcmc);
	struct device *dev = data->dev;
	enum mux_pcmc next = pcmc->next;
	ABOX_IPC_MSG msg;

	switch (pcmc->cur) {
	case ABOX_PCMC_CP:
		clk_disable(pcmc->clk_aud_pcmc);
		clk_disable(pcmc->clk_cp_pcmc);
		clk_set_rate(pcmc->clk_cp_pcmc, pcmc->rate_osc);
		break;
	case ABOX_PCMC_AUD:
		clk_disable(pcmc->clk_aud_pcmc);
		clk_set_rate(pcmc->clk_aud_pcmc, pcmc->rate_osc);
		break;
	default:
		/* nothing to do */
		break;
	}

	switch (next) {
	case ABOX_PCMC_CP:
		clk_set_rate(pcmc->clk_cp_pcmc, pcmc->rate_cp_pcmc);
		clk_enable(pcmc->clk_cp_pcmc);
		clk_enable(pcmc->clk_aud_pcmc);
		msg.ipcid = IPC_SYSTEM;
		msg.msg.system.msgtype = ABOX_READY_PCMC;
		msg.msg.system.param1 = next;
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
		abox_info(data->dev, "request pcmc %s\n", "cp");
		break;
	case ABOX_PCMC_AUD:
		clk_set_rate(pcmc->clk_aud_pcmc, pcmc->rate_aud_pcmc);
		clk_enable(pcmc->clk_aud_pcmc);
		msg.ipcid = IPC_SYSTEM;
		msg.msg.system.msgtype = ABOX_READY_PCMC;
		msg.msg.system.param1 = next;
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
		abox_info(data->dev, "request pcmc %s\n", "aud");
		break;
	default:
		abox_info(data->dev, "request pcmc %s\n", "osc");
		break;
	}

	pcmc->cur = next;
}

static void abox_request_pcmc(struct abox_data *data, enum mux_pcmc mux)
{
	if (!data->pcmc.clk_cp_pcmc || !data->pcmc.clk_aud_pcmc)
		return;

	data->pcmc.next = mux;
	queue_work(system_highpri_wq, &data->pcmc.request_work);
}

static void abox_release_pcmc(struct abox_data *data)
{
	abox_request_pcmc(data, ABOX_PCMC_OSC);
}

static int abox_probe_pcmc(struct abox_data *data)
{
	struct abox_pcmc *pcmc = &data->pcmc;
	struct device *dev = data->dev;

	pcmc->clk_cp_pcmc = devm_clk_get(dev, "cp_pcmc");
	if (IS_ERR(pcmc->clk_cp_pcmc))
		return -EINVAL;
	clk_prepare(pcmc->clk_cp_pcmc);

	pcmc->clk_aud_pcmc = devm_clk_get(dev, "aud_pcmc");
	if (IS_ERR(pcmc->clk_aud_pcmc))
		return -EINVAL;
	clk_prepare(pcmc->clk_aud_pcmc);

	pcmc->rate_osc = data->rate_pcmc[ABOX_PCMC_OSC];
	pcmc->rate_cp_pcmc = data->rate_pcmc[ABOX_PCMC_CP];
	pcmc->rate_aud_pcmc = data->rate_pcmc[ABOX_PCMC_AUD];

	INIT_WORK(&pcmc->request_work, abox_pcmc_request_work_func);

	abox_info(dev, "%s initialized\n", "pcmc");
	return 0;
}

static void abox_remove_pcmc(struct abox_data *data)
{
	if (!IS_ERR_OR_NULL(data->pcmc.clk_cp_pcmc))
		clk_unprepare(data->pcmc.clk_cp_pcmc);
	if (!IS_ERR_OR_NULL(data->pcmc.clk_aud_pcmc))
		clk_unprepare(data->pcmc.clk_aud_pcmc);
}

static void abox_restore_data(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);

	abox_dbg(dev, "%s\n", __func__);

	abox_restore_components(dev, data);
	abox_tplg_restore(dev);
	abox_cmpnt_restore(dev);
	abox_effect_restore();
	data->restored = true;
	wake_up_all(&data->wait_queue);
}

void abox_wait_restored(struct abox_data *data)
{
	long timeout = abox_get_waiting_jiffies(true);

	wait_event_timeout(data->wait_queue, data->restored == true, timeout);
}

static void abox_restore_data_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			restore_data_work);
	struct device *dev = data->dev;

	abox_restore_data(dev);
	abox_set_profiling_ipc(data);
}

static void abox_boot_clear_work_func(struct work_struct *work)
{
	static unsigned long boot_clear_count;
	struct delayed_work *dwork = to_delayed_work(work);
	struct abox_data *data = container_of(dwork, struct abox_data,
			boot_clear_work);
	struct device *dev = data->dev;

	abox_dbg(dev, "%s\n", __func__);

	if (!abox_cpu_gear_idle(dev, ABOX_CPU_GEAR_BOOT)) {
		abox_warn(dev, "boot clear activated\n");
		abox_request_cpu_gear(dev, data, ABOX_CPU_GEAR_BOOT,
				0, "boot_clear");
		boot_clear_count++;
	}
}

static void abox_boot_done_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			boot_done_work);
	struct device *dev = data->dev;

	abox_dbg(dev, "%s\n", __func__);

	abox_set_base_qos(data);
	abox_cpu_pm_ipc(data, true);
	/* notify current cpu gear level */
	abox_notify_cpu_gear(data);
	abox_request_cpu_gear(dev, data, DEFAULT_CPU_GEAR_ID, 0, "boot_done");
	abox_request_pcmc(data, ABOX_PCMC_AUD);
}

long abox_wait_for_boot(struct abox_data *data, unsigned long jiffies)
{
	return wait_event_timeout(data->boot_wait_queue,
			data->calliope_state == CALLIOPE_ENABLED,
			jiffies);
}

long abox_wait_boot_timeout(struct device *dev, unsigned long jiffies)
{
	return abox_wait_for_boot(dev_get_drvdata(dev), jiffies);
}
EXPORT_SYMBOL(abox_wait_boot_timeout);

static void abox_boot_done(struct device *dev, unsigned int version)
{
	struct abox_data *data = dev_get_drvdata(dev);
	char ver_char[4];

	abox_dbg(dev, "%s\n", __func__);

	data->calliope_version = version;
	memcpy(ver_char, &version, sizeof(ver_char));
	abox_info(dev, "Calliope is ready to sing (version:%c%c%c%c)\n",
			ver_char[3], ver_char[2], ver_char[1], ver_char[0]);
	data->calliope_state = CALLIOPE_ENABLED;
	schedule_work(&data->boot_done_work);
	cancel_delayed_work(&data->boot_clear_work);
	schedule_delayed_work(&data->boot_clear_work, HZ);
	wake_up_all(&data->boot_wait_queue);
	wake_up(&data->ipc_wait_queue);
	abox_dump_restore(data);
}

static irqreturn_t abox_dma_irq_handler(int irq, struct abox_data *data)
{
	struct device *dev = data->dev;
	int id;
	struct device **dev_dma;
	struct abox_dma_data *dma_data;

	abox_dbg(dev, "%s(%d)\n", __func__, irq);

	switch (irq) {
	case RDMA0_BUF_EMPTY:
	case RDMA1_BUF_EMPTY:
	case RDMA2_BUF_EMPTY:
	case RDMA3_BUF_EMPTY:
		id = irq - RDMA0_BUF_EMPTY;
		dev_dma = data->dev_rdma;
		break;
	case WDMA0_BUF_FULL:
	case WDMA1_BUF_FULL:
		id = irq - WDMA0_BUF_FULL;
		dev_dma = data->dev_wdma;
		break;
	default:
		abox_warn(dev, "unknown dma irq: irq=%d\n", irq);
		return IRQ_NONE;
	}

	if (unlikely(!dev_dma[id])) {
		abox_err(dev, "spurious dma irq: irq=%d id=%d\n", irq, id);
		return IRQ_HANDLED;
	}

	dma_data = dev_get_drvdata(dev_dma[id]);
	if (unlikely(!dma_data)) {
		abox_err(dev, "dma irq with null data: irq=%d id=%d\n", irq, id);
		return IRQ_HANDLED;
	}

	dma_data->pointer = 0;
	snd_pcm_period_elapsed(dma_data->substream);

	return IRQ_HANDLED;
}

static irqreturn_t abox_registered_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg, bool broadcast)
{
	struct abox_ipc_action *action;
	int ipc_id = msg->ipcid;
	irqreturn_t ret = IRQ_NONE;

	abox_dbg(dev, "%s: ipc_id=%d\n", __func__, ipc_id);

	list_for_each_entry(action, &data->ipc_actions, list) {
		if (action->ipc_id != ipc_id)
			continue;

		ret = action->handler(ipc_id, action->data, msg);
		if (!broadcast && ret == IRQ_HANDLED)
			break;
	}

	return ret;
}

static void abox_system_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_SYSTEM_MSG *system_msg = &msg->msg.system;
	phys_addr_t addr;
	void *area;
	int ret;

	abox_dbg(dev, "msgtype=%d\n", system_msg->msgtype);

	switch (system_msg->msgtype) {
	case ABOX_BOOT_DONE:
		if (abox_is_calliope_incompatible(dev))
			abox_err(dev, "Calliope is not compatible with the driver\n");

		abox_boot_done(dev, system_msg->param3);
		abox_registered_ipc_handler(dev, data, msg, true);
		break;
	case ABOX_CHANGE_GEAR:
		abox_request_cpu_gear(dev, data, system_msg->param2,
				system_msg->param1, "calliope");
		break;
	case ABOX_REQUEST_SYSCLK:
		switch (system_msg->param2) {
		default:
			/* fall through */
		case 0:
			abox_qos_request_mif(dev, system_msg->param3,
					system_msg->param1, "calliope");
			break;
		case 1:
			abox_qos_request_int(dev, system_msg->param3,
					system_msg->param1, "calliope");
			break;
		}
		break;
	case ABOX_RECORD_GEAR:
		abox_qos_request_fw0(dev, system_msg->param2, system_msg->param1, "");
		break;
	case ABOX_REPORT_LOG:
		area = abox_addr_to_kernel_addr(data, system_msg->param2);
		ret = abox_log_register_buffer(dev, system_msg->param1, area);
		if (ret < 0) {
			abox_err(dev, "log buffer registration failed: %u, %u\n",
					system_msg->param1, system_msg->param2);
		}
		break;
	case ABOX_FLUSH_LOG:
		break;
	case ABOX_REPORT_DUMP:
		area = abox_addr_to_kernel_addr(data, system_msg->param2);
		addr = abox_addr_to_phys_addr(data, system_msg->param2);
		ret = abox_dump_register_legacy(data, system_msg->param1,
				system_msg->bundle.param_bundle, area, addr,
				system_msg->param3);
		if (ret < 0) {
			abox_err(dev, "dump buffer registration failed: %u, %u\n",
					system_msg->param1, system_msg->param2);
		}
		break;
	case ABOX_COPY_DUMP:
		break;
	case ABOX_TRANSFER_DUMP:
		break;
	case ABOX_FLUSH_DUMP:
		abox_dump_period_elapsed(system_msg->param1,
				system_msg->param2);
		break;
	case ABOX_REPORT_COMPONENT:
		area = abox_addr_to_kernel_addr(data, system_msg->param1);
		abox_register_component(dev, area);
		break;
	case ABOX_REPORT_COMPONENT_CONTROL:
		abox_component_control_get_msg = *msg;
		wake_up(&data->ipc_wait_queue);
		break;
	case ABOX_UPDATED_COMPONENT_CONTROL:
		/* nothing to do */
		break;
	case ABOX_VSS_DISABLED:
		data->vss_disabled = !!system_msg->param1;
		abox_vss_notify_status(!data->vss_disabled);
		break;
	case ABOX_REQUEST_SRAM:
		abox_request_sram_vts(data);
		break;
	case ABOX_RELEASE_SRAM:
		abox_release_sram_vts(data);
		break;
	case ABOX_REQUEST_PCMC:
		abox_request_pcmc(data, system_msg->param1);
		break;
	case ABOX_RELEASE_PCMC:
		abox_release_pcmc(data);
		break;
	case ABOX_REPORT_FAULT:
	{
		const char *type;
		const char *memory_area;
		switch (system_msg->bundle.param_s32[2]) {
		case 0:
			memory_area = "framework";
			break;
		case 1:
			memory_area = "solution";
			break;
		case 2:
			memory_area = "vss";
			break;
		default:
			memory_area = "";
			break;
		}

		switch (system_msg->param1) {
		case 1:
			type = "data abort";
			break;
		case 2:
			type = "prefetch abort";
			break;
		case 3:
			type = "os error";
			break;
		case 4:
			type = "vss error";
			break;
		case 5:
			type = "undefined exception";
			break;
		case 6:
			type = "debug assert";
			break;
		default:
			type = "unknown error";
			break;
		}

		switch (system_msg->param1) {
		case 1:
		case 2:
		case 5:
		case 6:
			abox_err(dev, "%s(FAR:%#x, PC:%#x) on core%d is reported from calliope %s\n",
					type, system_msg->param2, system_msg->param3,
					system_msg->bundle.param_s32[1],
					memory_area);
			area = abox_addr_to_kernel_addr(data,
					system_msg->bundle.param_s32[0]);
			abox_dbg_print_gpr_from_addr(dev, data, area);
			abox_dbg_dump_gpr_from_addr(dev, data, area,
					ABOX_DBG_DUMP_FIRMWARE, type);
			abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_FIRMWARE,
					type);
			break;
		default:
			abox_err(dev, "%s(FAR:%#x, PC:%#x) on core%d is reported from calliope %s\n",
					type, system_msg->param2, system_msg->param3,
					system_msg->bundle.param_s32[1],
					memory_area);
			abox_dbg_print_gpr(dev, data);
			abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_FIRMWARE,
					type);
			abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_FIRMWARE,
					type);
			break;
		}
#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
		abox_debug_string_update(system_msg->param1, system_msg->param2,
				system_msg->param3, system_msg->bundle.param_s32[1],
				data->audio_mode, data->audio_mode_time);
#endif
		if (data->debug_mode == DEBUG_MODE_WDT)
			dbg_snapshot_expire_watchdog();
		abox_failsafe_report(dev, true);
		break;
	}
	case ABOX_REPORT_NOISE:
		abox_dbg(dev, "DMA error is deteced, p1:%d, p2:%d, p3:%d\n",
			system_msg->param1, system_msg->param2, system_msg->param3);
		if (system_msg->param3)
			abox_oem_noise_detect(data, ABOX_OEM_DMA_INTERVAL);
		else
			abox_oem_noise_detect(data, ABOX_OEM_DMA_UNDERRUN);
		break;
	case ABOX_UDMA_FLUSH:
		data->udma_stop = true;
		wake_up(&data->udma_fade_done);
		break;
	default:
		abox_warn(dev, "Redundant system message: %d(%d, %d, %d)\n",
				system_msg->msgtype, system_msg->param1,
				system_msg->param2, system_msg->param3);
		break;
	}
}

static void abox_pcm_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	irqreturn_t ret;

	abox_dbg(dev, "msgtype=%d\n", pcmtask_msg->msgtype);

	ret = abox_registered_ipc_handler(dev, data, msg, false);
	if (ret != IRQ_HANDLED)
		abox_err(dev, "not handled pcm ipc: %d, %d, %d\n", msg->ipcid,
				pcmtask_msg->channel_id, pcmtask_msg->msgtype);
}

static void abox_offload_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_OFFLOADTASK_MSG *offloadtask_msg = &msg->msg.offload;
	int id = offloadtask_msg->channel_id;
	struct abox_dma_data *dma_data = dev_get_drvdata(data->dev_rdma[id]);

	if (dma_data->compr_data.isr_handler)
		dma_data->compr_data.isr_handler(data->dev_rdma[id]);
	else
		abox_warn(dev, "Redundant offload message on rdma[%d]", id);
}

static irqreturn_t abox_irq_handler(int irq, void *dev_id)
{
	struct device *dev = dev_id;
	struct abox_data *data = dev_get_drvdata(dev);

	return abox_dma_irq_handler(irq, data);
}

static irqreturn_t abox_ipc_handler(int ipc, void *dev_id, ABOX_IPC_MSG *msg)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);

	abox_dbg(dev, "%s: ipc=%d, ipcid=%d\n", __func__, ipc, msg->ipcid);

	switch (ipc) {
	case IPC_SYSTEM:
		abox_system_ipc_handler(dev, data, msg);
		break;
	case IPC_PCMPLAYBACK:
	case IPC_PCMCAPTURE:
		abox_pcm_ipc_handler(dev, data, msg);
		break;
	case IPC_OFFLOAD:
		abox_offload_ipc_handler(dev, data, msg);
		break;
	default:
		abox_registered_ipc_handler(dev, data, msg, false);
		break;
	}

	abox_log_schedule_flush_all(dev);

	abox_dbg(dev, "%s: exit\n", __func__);
	return IRQ_HANDLED;
}

int abox_register_extra_sound_card(struct device *dev,
		struct snd_soc_card *card, unsigned int idx)
{
	static DEFINE_MUTEX(lock);
	static struct snd_soc_card *cards[SNDRV_CARDS];
	int i, ret;

	if (idx >= ARRAY_SIZE(cards))
		return -EINVAL;

	for (i = 1; i < idx; i++) {
		if (!cards[i])
			return -EAGAIN;
	}

	mutex_lock(&lock);
	abox_dbg(dev, "%s(%s, %u)\n", __func__, card ? card->name : "(null)", idx);

	for (i = ARRAY_SIZE(cards) - 1; i >= idx; i--)
		if (cards[i])
			snd_soc_unregister_card(cards[i]);

	if (card)
		cards[idx] = card;

	for (i = idx; i < ARRAY_SIZE(cards); i++) {
		if (cards[i]) {
			ret = snd_soc_register_card(cards[i]);
			if (ret < 0)
				cards[idx] = NULL;
		}
	}

	mutex_unlock(&lock);
	return 0;
}

static int abox_cpu_suspend_complete(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct regmap *regmap = data->timer_regmap;
	unsigned int value = 1;
	u64 limit;
	int ret = 0;

	abox_dbg(dev, "%s\n", __func__);

	limit = local_clock() + abox_get_waiting_ns(false);

	while (regmap_read(regmap, ABOX_TIMER_PRESET_LSB(1), &value) >= 0) {
		if (value == 0xFFFFFFFE) /* -2 means ABOX enters WFI */
			break;

		if (local_clock() > limit) {
			abox_err(dev, "%s: timeout\n", __func__);
			if (data->test_mode == TEST_MODE_SLT) {
				abox_info(dev, "TEST SLT MODE\n");
				abox_failsafe_report(dev, true);
			}
			ret = -ETIME;
			break;
		}
	}

	return ret;
}

void abox_silent_reset(struct abox_data *data, bool reset)
{
	static bool last;

	if (!abox_test_quirk(data, ABOX_QUIRK_BIT_SILENT_RESET))
		return;

	if (last == reset)
		return;

	if (data->pmu_silent_rst[OFFSET] && data->pmu_silent_rst[MASK]) {
		last = reset;
		if (reset) {
			abox_info(data->dev, "silent reset\n");
			exynos_pmu_write(data->pmu_silent_rst[OFFSET], data->pmu_silent_rst[MASK]);
#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
			send_adsp_silent_reset_ev();
#endif
		} else {
			exynos_pmu_write(data->pmu_silent_rst[OFFSET], 0x0);
		}
	} else {
		abox_warn(data->dev, "not support silent reset\n");
	}
}

static void abox_control_acp(struct abox_data *data, bool enable)
{
	unsigned int sysreg_ca32_con1, mask_ainact, value;

	if (data->sys_acp_con[OFFSET] == 0 || data->sys_acp_con[MASK] == 0)
		return;

	sysreg_ca32_con1 = data->sys_acp_con[OFFSET];
	mask_ainact = data->sys_acp_con[MASK];
	value = readl(data->sysreg_base + sysreg_ca32_con1);
	value &= ~mask_ainact;
	value |= enable ? 0x0 : mask_ainact;
	writel(value, data->sysreg_base + sysreg_ca32_con1);
	abox_dbg(data->dev, "%s(%d): %#x <= %#x\n", __func__,
			enable, sysreg_ca32_con1, value);
}

static void abox_control_qch_bypass(struct abox_data *data, bool enable)
{
	unsigned int sysreg_qch_bypass, mask_ainact, value;

	if (data->sys_qch_bypass[OFFSET] == 0 || data->sys_qch_bypass[MASK] == 0)
		return;

	sysreg_qch_bypass = data->sys_qch_bypass[OFFSET];
	mask_ainact = data->sys_qch_bypass[MASK];
	value = readl(data->sysreg_base + sysreg_qch_bypass);
	abox_info(data->dev, "%s(%#x)\n", __func__, value);
	value &= ~mask_ainact;
	value |= enable ? mask_ainact : 0x0;
	writel(value, data->sysreg_base + sysreg_qch_bypass);
	value = readl(data->sysreg_base + sysreg_qch_bypass);
	abox_info(data->dev, "%s(%d): %#x <= %#x\n", __func__,
			enable, sysreg_qch_bypass, value);

}

static int abox_cpu_pm_ipc(struct abox_data *data, bool resume)
{
	struct device *dev = data->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system = &msg.msg.system;
	unsigned long long ktime, atime;
	int suspend, standby, ret;

	abox_dbg(dev, "%s\n", __func__);

	ktime = sched_clock();
	atime = readq_relaxed(data->timer_base + ABOX_TIMER_CURVALUD_LSB(1));
	/* clock to ns */
	atime *= NSEC_PER_SEC / TIMER_MOD;
	do_div(atime, TIMER_RATE / TIMER_MOD);

	msg.ipcid = IPC_SYSTEM;
	system->msgtype = resume ? ABOX_RESUME : ABOX_SUSPEND;
	system->bundle.param_u64[0] = ktime;
	system->bundle.param_u64[1] = atime;
	ret = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 1);
	if (!resume) {
		suspend = abox_cpu_suspend_complete(dev);
		standby = abox_core_standby();
		if (suspend < 0 || standby < 0)
			abox_silent_reset(data, true);
	}

	return ret;
}

static int abox_pm_ipc(struct abox_data *data, bool resume)
{
	struct device *dev = data->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system = &msg.msg.system;
	unsigned long long ktime, atime;
	int ret;

	abox_dbg(dev, "%s\n", __func__);

	ktime = sched_clock();
	atime = readq_relaxed(data->timer_base + ABOX_TIMER_CURVALUD_LSB(1));
	/* clock to ns */
	atime *= NSEC_PER_SEC / TIMER_MOD;
	do_div(atime, TIMER_RATE / TIMER_MOD);

	msg.ipcid = IPC_SYSTEM;
	system->msgtype = resume ? ABOX_AP_RESUME : ABOX_AP_SUSPEND;
	system->bundle.param_u64[0] = ktime;
	system->bundle.param_u64[1] = atime;
	ret = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 1);

	return ret;
}

static void abox_pad_retention(struct abox_data *data, bool retention)
{
	if (!data->pad_ret_skip && !retention)
		exynos_pmu_update(data->pmu_pad_ret[OFFSET], data->pmu_pad_ret[MASK],
				data->pmu_pad_ret[MASK]);
	else
		abox_info(data->dev, "Skip audio pad retention control\n");
}

static void abox_cpu_power(bool on)
{
	abox_core_power(on);
}

#if !IS_ENABLED(CONFIG_SOC_S5E8855)
static void abox_pmu_aud_reset(struct abox_data *data, bool reset)
{
	if (!IS_ENABLED(CONFIG_SOC_S5E9955))
		return;

	abox_dbg(data->dev, "%s(%d)\n", __func__, reset);

	if (reset) {
		updatel_phys(data->pmu_aud_gresetn_cpu[MASK],
				data->pmu_aud_gresetn_cpu[OFFSET]);
		updatel_phys(data->pmu_aud_mask[MASK],
				data->pmu_aud_mask[OFFSET]);
		updatel_phys(data->pmu_aud_cnt_en_wdt[MASK],
				data->pmu_aud_cnt_en_wdt[OFFSET]);
	} else {
		updatel_phys(0x0, data->pmu_aud_cnt_en_wdt[OFFSET]);
		updatel_phys(0x0, data->pmu_aud_gresetn_cpu[OFFSET]);
	}
}
#else
static void abox_pmu_aud_reset(struct abox_data *data, bool reset)
{
	abox_info(data->dev, "%s(%d)\n", __func__, reset);
}
#endif
static int abox_cpu_enable(bool enable)
{
	/* reset core only if disable */
	if (!enable)
		abox_core_enable(enable);

	return 0;
}

static void abox_save_register(struct abox_data *data)
{
	regcache_cache_only(data->regmap, true);
	regcache_mark_dirty(data->regmap);
}

static void abox_restore_register(struct abox_data *data)
{
	regcache_cache_only(data->regmap, false);
	regcache_sync(data->regmap);
}

static int abox_ext_bin_request(struct device *dev,
		struct abox_extra_firmware *efw)
{
	int ret;

	abox_dbg(dev, "%s\n", __func__);

	mutex_lock(&efw->lock);

	release_firmware(efw->firmware);
	ret = request_firmware_direct(&efw->firmware, efw->name, dev);
	if (ret == -ENOENT)
		abox_warn(dev, "%s doesn't exist\n", efw->name);
	else if (ret < 0)
		abox_err(dev, "%s request fail: %d\n", efw->name, ret);
	else
		abox_info(dev, "%s is loaded\n", efw->name);

	mutex_unlock(&efw->lock);

	return ret;
}

static void abox_ext_bin_download(struct abox_data *data,
		struct abox_extra_firmware *efw)
{
	struct device *dev = data->dev;
	void __iomem *base;
	size_t size;

	abox_dbg(dev, "%s\n", __func__);

	mutex_lock(&efw->lock);
	if (!efw->firmware)
		goto unlock;

	switch (efw->area) {
	case 0:
		base = data->sram_base;
		size = SRAM_FIRMWARE_SIZE;
		efw->iova = efw->offset;
		break;
	case 1:
		base = data->dram_para_base;
		size = DRAM_PARAMETER_SIZE;
		efw->iova = efw->offset + IOVA_DRAM_PARAMETER;
		break;
	case 2:
		base = data->dram_base;
		size = DRAM_FIRMWARE_SIZE;
		efw->iova = efw->offset + IOVA_DRAM_FIRMWARE;
		break;
	case 3:
#if IS_ENABLED(CONFIG_SHM_IPC)
		base = shm_get_vss_region();
		size = shm_get_vss_size();
		efw->iova = efw->offset + IOVA_VSS_FIRMWARE;
#else
		abox_err(dev, "%s: invalied area %s, %u, %#x\n", __func__,
				efw->name, efw->area, efw->offset);
		goto unlock;
#endif
		break;
	default:
		abox_err(dev, "%s: invalied area %s, %u, %#x\n", __func__,
				efw->name, efw->area, efw->offset);
		goto unlock;
	}

	if (efw->offset + efw->firmware->size > size) {
		abox_err(dev, "%s: too large firmware %s, %u, %#x\n", __func__,
				efw->name, efw->area, efw->offset);
		goto unlock;
	}

	memcpy(base + efw->offset, efw->firmware->data, efw->firmware->size);
	abox_dbg(dev, "%s is downloaded %u, %#x\n", efw->name,
			efw->area, efw->offset);
unlock:
	mutex_unlock(&efw->lock);
}

static int abox_ext_bin_name_get(struct snd_kcontrol *kcontrol,
		unsigned int __user *bytes, unsigned int size)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_bytes_ext *params = (void *)kcontrol->private_value;
	struct abox_extra_firmware *efw = params->dobj.private;
	unsigned long res = size;

	abox_dbg(dev, "%s(%s)\n", __func__, kcontrol->id.name);

	res = copy_to_user(bytes, efw->name, min(sizeof(efw->name), res));
	if (res)
		abox_warn(dev, "%s: size=%u, res=%lu\n", __func__, size, res);

	return 0;
}

static int abox_ext_bin_name_put(struct snd_kcontrol *kcontrol,
		const unsigned int __user *bytes, unsigned int size)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_bytes_ext *params = (void *)kcontrol->private_value;
	struct abox_extra_firmware *efw = params->dobj.private;
	const struct firmware *test = NULL;
	char *name;
	unsigned long res;
	int ret;

	abox_dbg(dev, "%s(%s)\n", __func__, kcontrol->id.name);

	if (!efw->changeable)
		return -EINVAL;

	if (size >= sizeof(efw->name))
		return -EINVAL;

	name = kzalloc(size + 1, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	res = copy_from_user(name, bytes, size);
	if (res)
		abox_warn(dev, "%s: size=%u, res=%zu\n", __func__, size, res);

	ret = request_firmware(&test, name, dev);
	release_firmware(test);
	if (ret >= 0) {
		strlcpy(efw->name, name, sizeof(efw->name));
		abox_ext_bin_request(dev, efw);
	}

	kfree(name);
	return ret;
}

static int abox_ext_bin_reload_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;

	abox_dbg(dev, "%s(%s)\n", __func__, kcontrol->id.name);

	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int abox_ext_bin_reload_put_ipc(struct abox_data *data,
		struct abox_extra_firmware *efw)
{
	struct device *dev = data->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	abox_dbg(dev, "%s(%s)\n", __func__, efw->name);

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_RELOAD_AREA;
	system_msg->param1 = efw->iova;
	system_msg->param2 = (int)(efw->firmware ? efw->firmware->size : 0);
	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int abox_ext_bin_reload_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int value = (unsigned int)ucontrol->value.integer.value[0];
	struct abox_extra_firmware *efw = mc->dobj.private;

	abox_dbg(dev, "%s(%s, %u)\n", __func__, kcontrol->id.name, value);

	if (value < mc->min || value > mc->max)
		return -EINVAL;

	if (!efw->changeable)
		return -EINVAL;

	if (value) {
		abox_ext_bin_request(dev, efw);
		abox_ext_bin_download(data, efw);
		abox_ext_bin_reload_put_ipc(data, efw);
	}

	return 0;
}

static const struct snd_kcontrol_new abox_ext_bin_controls[] = {
	SND_SOC_BYTES_TLV("EXT BIN%d NAME", 64,
			abox_ext_bin_name_get, abox_ext_bin_name_put),
	SOC_SINGLE_EXT("EXT BIN%d RELOAD", 0, 0, 1, 0,
			abox_ext_bin_reload_get, abox_ext_bin_reload_put),
};

static int abox_ext_bin_reload_all_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	abox_ext_bin_reload_get(kcontrol, ucontrol);

	return 0;
}

static int abox_ext_bin_reload_all_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
			(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int value = (unsigned int)ucontrol->value.integer.value[0];
	struct abox_extra_firmware *efw;

	abox_dbg(dev, "%s(%s, %u)\n", __func__, kcontrol->id.name, value);

	if (value < mc->min || value > mc->max)
		return -EINVAL;

	if (value) {
		list_for_each_entry(efw, &data->firmware_extra, list) {
			if (efw->changeable) {
				abox_ext_bin_request(dev, efw);
				abox_ext_bin_download(data, efw);
				abox_ext_bin_reload_put_ipc(data, efw);
			}
		}
	}

	return 0;
}

static const struct snd_kcontrol_new abox_ext_bin_reload_all_control =
	SOC_SINGLE_EXT("EXT BIN RELOAD ALL", 0, 0, 1, 0,
			abox_ext_bin_reload_all_get,
			abox_ext_bin_reload_all_put);

static int abox_ext_bin_add_controls(struct snd_soc_component *cmpnt,
		struct abox_extra_firmware *efw)
{
	struct device *dev = cmpnt->dev;
	struct snd_kcontrol_new *control, *controls;
	struct soc_bytes_ext *be;
	struct soc_mixer_control *mc;
	char *name;
	int num_controls = ARRAY_SIZE(abox_ext_bin_controls);
	int ret;

	controls = kmemdup(&abox_ext_bin_controls,
			sizeof(abox_ext_bin_controls), GFP_KERNEL);
	if (!controls)
		return -ENOMEM;

	for (control = controls; control - controls < num_controls; control++) {
		name = kasprintf(GFP_KERNEL, control->name, efw->idx);
		if (!name) {
			ret = -ENOMEM;
			goto err;
		}
		control->name = name;

		if (control->info == snd_soc_bytes_info_ext) {
			be = (void *)control->private_value;
			be = devm_kmemdup(dev, be, sizeof(*be), GFP_KERNEL);
			if (!be) {
				ret = -ENOMEM;
				goto err;
			}
			be->dobj.private = efw;
			control->private_value = (unsigned long)be;
		} else if (control->info == snd_soc_info_volsw) {
			mc = (void *)control->private_value;
			mc = devm_kmemdup(dev, mc, sizeof(*mc), GFP_KERNEL);
			if (!mc) {
				ret = -ENOMEM;
				goto err;
			}
			mc->dobj.private = efw;
			control->private_value = (unsigned long)mc;
		} else {
			ret = -EINVAL;
			goto err;
		}
	}

	ret = snd_soc_add_component_controls(cmpnt, controls, num_controls);
err:
	for (control = controls; control - controls < num_controls; control++)
		kfree_const(control->name);
	kfree(controls);
	return ret;
}

static void abox_add_extra_firmware_controls_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			add_extra_firmware_controls_work);
	struct device *dev = data->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct abox_extra_firmware *efw;
	int ret;

	abox_info(dev, "%s\n", __func__);

	if (!cmpnt) {
		abox_err(dev, "failed to add extra firmware controls\n");
		return;
	}

	snd_soc_add_component_controls(cmpnt, &abox_ext_bin_reload_all_control, 1);

	list_for_each_entry(efw, &data->firmware_extra, list) {
		if (efw->changeable) {
			ret = abox_ext_bin_add_controls(cmpnt, efw);
			if (ret < 0)
				abox_err(dev, "failed to add ext bin controls : %d\n", ret);
		}
	}
}

static struct abox_extra_firmware *abox_find_extra_firmware(
		struct abox_data *data, const char *name)
{
	struct abox_extra_firmware *efw;

	list_for_each_entry(efw, &data->firmware_extra, list) {
		if (strncmp(name, efw->name, sizeof(efw->name)) == 0)
			return efw;
	}

	return NULL;
}

int abox_add_extra_firmware(struct device *dev, struct abox_data *data,
		const char *name, unsigned int area,
		unsigned int offset, bool changeable)
{
	struct abox_extra_firmware *efw;
	int idx;

	efw = abox_find_extra_firmware(data, name);
	if (efw) {
		abox_info(dev, "update firmware: name=%s, area=%u, offset=%#x\n",
				name, area, offset);
		mutex_lock(&efw->lock);
		strlcpy(efw->name, name, sizeof(efw->name));
		efw->area = area;
		efw->offset = offset;
		efw->changeable = changeable;
		mutex_unlock(&efw->lock);
		return 0;
	}

	idx = 0;
	list_for_each_entry(efw, &data->firmware_extra, list)
		if (idx <= efw->idx)
			idx = efw->idx + 1;

	efw = devm_kzalloc(dev, sizeof(*efw), GFP_KERNEL);
	if (!efw)
		return -ENOMEM;

	abox_info(dev, "new firmware: name=%s, area=%u, offset=%#x\n",
				name, area, offset);
	strlcpy(efw->name, name, sizeof(efw->name));
	efw->idx = idx;
	efw->area = area;
	efw->offset = offset;
	efw->changeable = changeable;
	mutex_init(&efw->lock);
	list_add_tail(&efw->list, &data->firmware_extra);
	return 0;
}

static void abox_reload_extra_firmware(struct abox_data *data, const char *name)
{
	struct device *dev = data->dev;
	struct abox_extra_firmware *efw;

	abox_dbg(dev, "%s(%s)\n", __func__, name);

	list_for_each_entry(efw, &data->firmware_extra, list) {
		if (strncmp(efw->name, name, sizeof(efw->name)))
			continue;

		abox_ext_bin_request(dev, efw);
	}
}

static void abox_request_extra_firmware(struct abox_data *data)
{
	struct device *dev = data->dev;
	struct abox_extra_firmware *efw;

	abox_dbg(dev, "%s\n", __func__);

	list_for_each_entry(efw, &data->firmware_extra, list) {
		if (efw->firmware)
			continue;

		abox_ext_bin_request(dev, efw);
	}
}

static void abox_download_extra_firmware(struct abox_data *data)
{
	struct device *dev = data->dev;
	struct abox_extra_firmware *efw;

	abox_dbg(dev, "%s\n", __func__);

	list_for_each_entry(efw, &data->firmware_extra, list)
		abox_ext_bin_download(data, efw);
}

static void abox_parse_extra_firmware(struct abox_data *data)
{
	struct device *dev = data->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct abox_extra_firmware *efw;
	const char *name;
	unsigned int idx, area, offset;
	bool changeable, kcontrol;
	int ret;

	abox_dbg(dev, "%s\n", __func__);

	idx = 0;
	for_each_available_child_of_node(np, child_np) {
		if (!of_device_is_compatible(child_np, "samsung,abox-ext-bin"))
			continue;

		ret = of_samsung_property_read_string(dev, child_np, "name",
				&name);
		if (ret < 0)
			continue;

		ret = of_samsung_property_read_u32(dev, child_np, "area",
				&area);
		if (ret < 0)
			continue;

		ret = of_samsung_property_read_u32(dev, child_np, "offset",
				&offset);
		if (ret < 0)
			continue;

		kcontrol = of_samsung_property_read_bool(dev, child_np,
				"mixer-control");

		changeable = of_samsung_property_read_bool(dev, child_np,
				"changeable");

		efw = devm_kzalloc(dev, sizeof(*efw), GFP_KERNEL);
		if (!efw)
			continue;

		abox_info(dev, "%s: name=%s, area=%u, offset=%#x\n", __func__, name, area, offset);

		strlcpy(efw->name, name, sizeof(efw->name));
		efw->idx = idx++;
		efw->area = area;
		efw->offset = offset;
		efw->changeable = changeable;
		mutex_init(&efw->lock);
		list_add_tail(&efw->list, &data->firmware_extra);
	}
}

static int abox_download_firmware(struct device *dev)
{
	static bool requested;
	struct abox_data *data = dev_get_drvdata(dev);
	int ret;

	ret = abox_load_config_file(data);
	if (ret)
		return ret;

	ret = abox_core_download_firmware();
	if (ret)
		return ret;

	/* Requesting missing extra firmware is waste of time. */
	if (!requested) {
		abox_request_extra_firmware(data);
		requested = true;
	}
	abox_download_extra_firmware(data);

	return 0;
}

static const char *conf_node_get_value_optional(struct abox_conf_node *node, const char *name)
{
	struct abox_conf_tuple *tuple;

	list_for_each_entry(tuple, &node->tuples, list) {
		if (strcmp(name, tuple->name) == 0)
			return tuple->value;
	}

	return NULL;
}

static bool conf_node_get_value_bool(struct abox_conf_node *node, const char *name)
{
	struct abox_conf_tuple *tuple;

	list_for_each_entry(tuple, &node->tuples, list) {
		if (strcmp(name, tuple->name) == 0)
			return 1;
	}

	return 0;
}

static const char *conf_node_get_value(struct abox_data *data, struct abox_conf_node *node,
		const char *name)
{
	const char *value;

	value = conf_node_get_value_optional(node, name);
	if (!value)
		abox_err(data->dev, "Couldn't get %s for %s\n", name, node->name);

	return value;
}

static void conf_node_get_name_address_size(struct abox_data *data, struct abox_conf_node *node,
	const char **name, const char **address, const char **size) {

	*name = conf_node_get_value(data, node, "name");
	*address = conf_node_get_value(data, node, "address");
	*size = conf_node_get_value_optional(node, "size");
}

static int parse_conf_tuple(char **inp, char **name, char **value)
{
	*value = strsep(inp, ";");
	if (!*value)
		return -EINVAL;

	*name = strim(strsep(value, "="));
	if (*value)
		*value = strim(*value);

	return 0;
}

static void put_conf_node(struct abox_data *data, struct abox_conf_node *node)
{
	struct abox_conf_tuple *tuple;

	if (!node)
		return;

	list_for_each_entry(tuple, &node->tuples, list)
		devm_kfree(data->dev, tuple);
	devm_kfree(data->dev, node);
}

static struct abox_conf_node *get_conf_node(struct abox_data *data, char **inp)
{
	static const char name_compatible[] = "compatible";
	struct device *dev = data->dev;
	struct abox_conf_node *node = NULL;
	char *buffer, *name, *value;

	buffer = strsep(inp, "}");
	if (!*inp)
		return ERR_PTR(-EINVAL);

	/* skip ; after } */
	strsep(inp, ";");

	node = devm_kmalloc(dev, sizeof(*node), GFP_KERNEL);
	if (!node)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&node->tuples);
	name = strsep(&buffer, "{");
	if (name)
		node->name = strim(name);

	while (buffer) {
		struct abox_conf_tuple *tuple;

		if (parse_conf_tuple(&buffer, &name, &value) < 0)
			break;

		if (strncmp(name, name_compatible, sizeof(name_compatible)) == 0) {
			node->compatible = value;
			continue;
		}

		tuple = devm_kmalloc(dev, sizeof(*tuple), GFP_KERNEL);
		if (!tuple) {
			put_conf_node(data, node);
			return ERR_PTR(-ENOMEM);
		}

		tuple->name = name;
		tuple->value = value;
		list_add(&tuple->list, &node->tuples);
	}

	return node;
}

static int process_conf_node_ext_bin(struct abox_data *data, struct abox_conf_node *node)
{
	const char *name, *address, *size;
	unsigned int iova, area, offset;
	bool changeable = false;
	bool disabled = false;

	conf_node_get_name_address_size(data, node, &name, &address, &size);
	changeable = conf_node_get_value_bool(node, "changeable");
	disabled = conf_node_get_value_bool(node, "disabled");

	if (disabled) {
		abox_info(data->dev, "%s is disabled\n", name);
		return 0;
	}

	if (kstrtouint(address, 0, &iova) < 0)
		return -EINVAL;

	if (iova < IOVA_DRAM_PARAMETER) {
		area = 0;
		offset = iova;
	} else if (iova < IOVA_DRAM_FIRMWARE) {
		area = 1;
		offset = iova - IOVA_DRAM_PARAMETER;
	} else if (iova < IOVA_VSS_FIRMWARE) {
		area = 2;
		offset = iova - IOVA_DRAM_FIRMWARE;
	} else {
		area = 3;
		offset = iova - IOVA_VSS_FIRMWARE;
	}

	return abox_add_extra_firmware(data->dev, data, name, area, offset, changeable);
}

static int complete_conf_node_ext_bin(struct abox_data *data)
{
	schedule_work(&data->add_extra_firmware_controls_work);

	return 0;
}

static int process_conf_node_ipc(struct abox_data *data, struct abox_conf_node *node)
{
	const char *tx_address, *rx_address, *tx_size, *rx_size;
	unsigned int tx_iova, rx_iova, tx_sz, rx_sz;

	tx_address = conf_node_get_value(data, node, "ipc-tx-address");
	tx_size = conf_node_get_value(data, node, "ipc-tx-size");
	rx_address = conf_node_get_value(data, node, "ipc-rx-address");
	rx_size = conf_node_get_value(data, node, "ipc-rx-size");

	if (kstrtouint(tx_address, 0, &tx_iova) < 0)
		return -EINVAL;

	if (kstrtouint(tx_size, 0, &tx_sz) < 0)
		return -EINVAL;

	if (kstrtouint(rx_address, 0, &rx_iova) < 0)
		return -EINVAL;

	if (kstrtouint(rx_size, 0, &rx_sz) < 0)
		return -EINVAL;

	data->ipc_tx_addr = abox_addr_to_kernel_addr(data, tx_iova);
	data->ipc_tx_size = tx_sz;
	data->ipc_rx_addr = abox_addr_to_kernel_addr(data, rx_iova);
	data->ipc_rx_size = rx_sz;

	return 0;
}

static int complete_conf_node_ipc(struct abox_data *data)
{
	return abox_ipc_init(data->dev, data->ipc_tx_addr, data->ipc_tx_size,
		data->ipc_rx_addr, data->ipc_rx_size);
}

static int process_conf_node_log(struct abox_data *data, struct abox_conf_node *node)
{
	const char *name, *address, *size;
	unsigned int iova;
	static const char str_log[] = "log-area";
	static const char str_log_major[] = "log-major-area";

	conf_node_get_name_address_size(data, node, &name, &address, &size);

	if (kstrtouint(address, 0, &iova) < 0)
		return -EINVAL;

	if (!strncmp(str_log, name, sizeof(str_log) - 1)) {
		data->log_addr = iova - IOVA_DRAM_FIRMWARE;
		abox_info(data->dev, "log offest: 0x%x\n", iova);
	} else if (!strncmp(str_log_major, name, sizeof(str_log_major) - 1)) {
		data->log_major_addr = iova - IOVA_DRAM_FIRMWARE;
		abox_info(data->dev, "log major offest: 0x%x\n", iova);
	}

	return 0;
}

static int complete_conf_node_log(struct abox_data *data)
{
	if(!data->log_addr)
		data->log_addr = ABOX_LOG_OFFSET;
	if (!data->log_major_addr)
		data->log_major_addr = ABOX_LOG_MAJOR_OFFSET;

	abox_dbg_create(data);

	return 0;
}

struct abox_conf_loader {
	const char *compatible;
	int (*load)(struct abox_data *data, struct abox_conf_node *node);
	int (*complete)(struct abox_data *data);
};

static const struct abox_conf_loader abox_conf_loaders[] = {
	{ "abox-ext-bin",  process_conf_node_ext_bin, complete_conf_node_ext_bin },
	{ "abox-ipc-area",  process_conf_node_ipc, complete_conf_node_ipc },
	{ "abox-log-area",  process_conf_node_log, complete_conf_node_log },
};

static int process_conf_loader(struct abox_data *data, const struct abox_conf_loader *loader)
{
	struct abox_conf *conf = &data->conf;
	struct abox_conf_node *node;
	int res, ret = 0;

	list_for_each_entry(node, &conf->nodes, list) {
		if (!strcmp(loader->compatible, node->compatible)) {
			res = loader->load(data, node);
			if (res < 0) {
				abox_warn(data->dev, "failed to process conf %s(%s): %d\n",
					node->name, node->compatible, res);
				ret = res;
			}
		}
	}

	return ret;
}

static int abox_process_config(struct abox_data *data)
{
	const struct abox_conf_loader *loaders = abox_conf_loaders;
	const struct abox_conf_loader *loader;
	size_t len_loaders = ARRAY_SIZE(abox_conf_loaders);
	int res, ret = 0;

	for (loader = loaders; loader - loaders < len_loaders; loader++) {
		res = process_conf_loader(data, loader);
		if (res < 0)
			ret = res;
	}

	return ret;
}

static int abox_complete_config(struct abox_data *data)
{
	const struct abox_conf_loader *loaders = abox_conf_loaders;
	const struct abox_conf_loader *loader;
	size_t len_loaders = ARRAY_SIZE(abox_conf_loaders);
	int res, ret = 0;

	for (loader = loaders; loader - loaders < len_loaders; loader++) {
		if (!loader->complete)
			continue;

		res = loader->complete(data);
		if (res < 0)
			ret = res;
	}

	return ret;
}

static int abox_parse_config(struct abox_data *data, char *buffer, size_t size)
{
	struct device *dev = data->dev;
	struct abox_conf *conf = &data->conf;
	struct abox_conf_node *node;

	conf->buffer = buffer;

	while (buffer) {
		node = get_conf_node(data, &buffer);
		if (IS_ERR_OR_NULL(node))
			continue;

		list_add_tail(&node->list, &conf->nodes);
		abox_info(dev, "parsed config %s\n", node->name ? : "(no name)");
	}

	return 0;
}

static void abox_free_config(struct abox_data *data)
{
	struct abox_conf *conf = &data->conf;
	struct abox_conf_node *node, *n;

	list_for_each_entry_safe(node, n, &conf->nodes, list)
		put_conf_node(data, node);

	devm_kfree(data->dev, conf->buffer);
	conf->buffer = NULL;
}

static void abox_init_config(struct abox_data *data)
{
	struct abox_conf *conf = &data->conf;

	conf->buffer = NULL;
	INIT_LIST_HEAD(&conf->nodes);
}

static void abox_parse_config_file(struct abox_data *data, const struct firmware *fw)
{
	struct device *dev = data->dev;
	size_t size;
	char *buffer;

	if (!fw) {
		abox_err(data->dev, "failed to load config file\n");
		return;
	}

	size = fw->size;

	buffer = devm_kmalloc(dev, size + 1, GFP_KERNEL);
	if (!buffer)
		return;

	abox_info(data->dev, "%s\n", __func__);

	memcpy(buffer, fw->data, size);
	buffer[size] = '\0';

	abox_parse_config(data, buffer, size);
}

static int abox_load_config_file(struct abox_data *data)
{
	const struct firmware *fw = NULL;
	int ret, i;

	if (!list_empty(&data->conf.nodes))
		return 0;

	for(i = 0; i < ARRAY_SIZE(data->file_name); i++) {
		abox_init_config(data);
		ret = request_firmware_direct(&fw, data->file_name[i], data->dev);
		if (ret < 0) {
			abox_err(data->dev, "%s request fail: %d\n", data->file_name[i], ret);
			return -EAGAIN;
		}
		abox_parse_config_file(data, fw);
		abox_process_config(data);
		release_firmware(fw);
	}
	abox_complete_config(data);

	return 0;
}

static void abox_set_calliope_bootargs(struct abox_data *data)
{
	if (!data->bootargs_offset || !data->bootargs)
		return;

	abox_info(data->dev, "bootargs: %#x, %s\n", data->bootargs_offset,
			data->bootargs);

	memcpy_toio(data->sram_base + data->bootargs_offset, data->bootargs,
			strnlen(data->bootargs, SZ_512) + 1);
}

static void abox_set_calliope_slogargs(struct abox_data *data)
{
	char args[8];
	int len;

	if (!data->slogargs_offset)
		return;

	len = snprintf(args, sizeof(args), "%zu", data->slog_size / (1024 * 1024));
	memcpy_toio(data->sram_base + data->slogargs_offset, args, len + 1);
	abox_info(data->dev, "slogargs: %#x, %s\n", data->slogargs_offset, args);
}

static bool abox_is_timer_set(struct abox_data *data)
{
	unsigned int val;
	int ret;

	ret = regmap_read(data->timer_regmap, ABOX_TIMER_PRESET_LSB(1), &val);
	if (ret < 0)
		val = 0;

	return !!val;
}

static void abox_start_timer(struct abox_data *data)
{
	struct regmap *regmap = data->timer_regmap;

	regmap_write(regmap, ABOX_TIMER_CTRL0(0), 1 << ABOX_TIMER_START_L);
	regmap_write(regmap, ABOX_TIMER_CTRL0(2), 1 << ABOX_TIMER_START_L);
}

static void abox_update_suspend_wait_flag(struct abox_data *data, bool suspend)
{
	const unsigned int HOST_SUSPEND_HOLDING_FLAG = 0x74736F68; /* host */
	const unsigned int HOST_SUSPEND_RELEASE_FLAG = 0x656B6177; /* wake */
	unsigned int flag;

	flag = suspend ? HOST_SUSPEND_HOLDING_FLAG : HOST_SUSPEND_RELEASE_FLAG;
	WRITE_ONCE(data->hndshk_tag->suspend_wait_flag, flag);
}

static void abox_cleanup_sidetone(struct abox_data *data)
{
	writel(0x0, data->sfr_base + ABOX_SIDETONE_CTRL);
}

static void abox_cleanup_atune_bqf(struct abox_data *data)
{
	if (IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_ABOX_ATUNE)) {
		writel_relaxed(0x0, data->sfr_base + ATUNE_SPUS_BQF_CTRL(0));
		writel_relaxed(0x0, data->sfr_base + ATUNE_SPUS_BQF_CTRL(1));
		writel_relaxed(0x0, data->sfr_base + ATUNE_SPUM_BQF_CTRL(0));
		writel(0x0, data->sfr_base + ATUNE_SPUM_BQF_CTRL(1));
	}
}

static void abox_cleanup_sifs_cnt_val(struct abox_data *data)
{
	const int r = 11; /* RDMA */
	const int u = 5; /* UAIF */
	const unsigned long long tout_ns = 100 * NSEC_PER_MSEC;
	struct device *dev = data->dev;
	void __iomem *b = data->sfr_base;
	bool dirty = false;
	unsigned long long time;
	unsigned int val;
	int i;

	if (!abox_addr_to_kernel_addr(data, IOVA_RDMA_BUFFER(r)))
		return;

	for (i = 0; i < ARRAY_SIZE(data->sifs_cnt_dirty); i++) {
		if (data->sifs_cnt_dirty[i]) {
			dirty = true;
			break;
		}
	}
	if (!dirty)
		return;

	/* disable rdma11 irq */
	abox_gic_enable(data->dev_gic, IRQ_RDMA11_BUF_EMPTY, 0);
	/* set up rdma11 */
	writel_relaxed(IOVA_RDMA_BUFFER(r), b + ABOX_RDMA_BUF_STR(r));
	writel_relaxed(IOVA_RDMA_BUFFER(r) + SZ_4K, b + ABOX_RDMA_BUF_END(r));
	writel_relaxed(SZ_1K, b + ABOX_RDMA_BUF_OFFSET(r));
	writel_relaxed(IOVA_RDMA_BUFFER(r), b + ABOX_RDMA_STR_POINT(r));
	writel_relaxed(0x1 << ABOX_DMA_DST_BIT_WIDTH_L,
			b + ABOX_RDMA_BIT_CTRL(r));
	/* format of mixp and stmix */
	writel_relaxed(0x00090009, b + ABOX_SPUS_CTRL_MIXP_FORMAT);
	/* set up & enable uaif5 */
	writel_relaxed(0x0984f000, b + ABOX_UAIF_CTRL1(u));
	writel_relaxed(0x01880015, b + ABOX_UAIF_CTRL0(u));

	for (i = ARRAY_SIZE(data->sifs_cnt_dirty) - 1; i >= 0; i--) {
		if (!data->sifs_cnt_dirty[i])
			continue;

		data->sifs_cnt_dirty[i] = false;
		abox_info(dev, "reset sifs%d_cnt_val\n", i);

		/* set up spus */
		writel_relaxed((i ? (i << 1) : 0x1) <<
				ABOX_FUNC_CHAIN_SRC_OUT_L(r),
				b + ABOX_SPUS_CTRL_FC_SRC(r));
		/* set up sifs_out_sel */
		if (i > 0)
			writel_relaxed(r << ABOX_SIFS_OUT_SEL_L(i),
					b + ABOX_SPUS_CTRL_SIFS_OUT_SEL(i));
		/* set up uaif5_spk */
		writel_relaxed((i + 1) << ABOX_ROUTE_UAIF_SPK_L(5),
				b + ABOX_ROUTE_CTRL_UAIF_SPK(5));
		/* set sifs_cnt_val */
		writel_relaxed(ABOX_SIFS_CNT_VAL_MASK(i),
				b + ABOX_SPUS_CTRL_SIFS_CNT_VAL(i));
		abox_dbg(dev, "sifs%d_cnt_val: %#x\n", i, readl_relaxed(b +
				ABOX_SPUS_CTRL_SIFS_CNT_VAL(i)));
		/* enable rdma11 */
		writel(0xb0200901, b + ABOX_RDMA_CTRL0(r));
		time = sched_clock();
		do {
			val = readl(b + ABOX_RDMA_STATUS(r));
			val &= ABOX_RDMA_PROGRESS_MASK;
			if (val)
				break;
			udelay(1);
		} while (sched_clock() - time < tout_ns);
		if (!val)
			abox_err(dev, "RDMA enable timeout\n");
		abox_dbg(dev, "rdma status: %#x\n",
				readl_relaxed(b + ABOX_RDMA_STATUS(r)));
		/* wait for low aclk_cnt_ack */
		udelay(1);
		/* clear sifs_cnt_val */
		writel(0, b + ABOX_SPUS_CTRL_SIFS_CNT_VAL(i));
		abox_dbg(dev, "sifs%d_cnt_val: %#x\n", i, readl_relaxed(b +
				ABOX_SPUS_CTRL_SIFS_CNT_VAL(i)));
		/* disable rdma11 */
		writel(0xb0200900, b + ABOX_RDMA_CTRL0(r));
		time = sched_clock();
		do {
			val = readl(b + ABOX_RDMA_STATUS(r));
			val &= ABOX_RDMA_PROGRESS_MASK;
			if (!val)
				break;
			udelay(1);
		} while (sched_clock() - time < tout_ns);
		if (val)
			abox_err(dev, "RDMA disable timeout\n");
		abox_dbg(dev, "rdma status: %#x\n",
				readl_relaxed(b + ABOX_RDMA_STATUS(r)));
	}
	/* disable uaif5 */
	writel(0x01880014, b + ABOX_UAIF_CTRL0(u));
	/* restore rdma11 irq */
	abox_gic_enable(data->dev_gic, IRQ_RDMA11_BUF_EMPTY, 1);
}

static void abox_cleanup_dummy_start(struct abox_data *data)
{
	unsigned int i, val;

	for (i = data->rdma_count; i; i--) {
		val = readl_relaxed(data->sfr_base + ABOX_RDMA_CTRL(i));
		val &= ~ABOX_DMA_DUMMY_START_MASK;
		writel_relaxed(val, data->sfr_base + ABOX_RDMA_CTRL(i));
	}
	writel(0x0, data->sfr_base + ABOX_SPUS_LATENCY_CTRL0);
}

static void abox_cleanup_spus_flush(struct abox_data *data)
{
	writel(ABOX_MIXP_LD_FLUSH_MASK | ABOX_MIXP_FLUSH_MASK |
			ABOX_STMIX_LD_FLUSH_MASK | ABOX_STMIX_FLUSH_MASK |
			ABOX_SIFS_FLUSH_MASK(1) | ABOX_SIFS_FLUSH_MASK(2) |
			ABOX_SIFS_FLUSH_MASK(3) | ABOX_SIFS_FLUSH_MASK(4) |
			ABOX_SIFS_FLUSH_MASK(5) | ABOX_SIFS_FLUSH_MASK(6),
			data->sfr_base + ABOX_SPUS_CTRL_FLUSH);
}

#if !IS_ENABLED(CONFIG_SOC_S5E9955) && !IS_ENABLED(CONFIG_SOC_S5E8855)
static void abox_cleanup_qchannel_disable(struct abox_data *data, enum qchannel qchannel, bool disable)
{
	unsigned int val, mask = BIT_MASK(qchannel);

	val = readl(data->sfr_base + ABOX_QCHANNEL_DISABLE);
	val &= ~mask;
	val |= disable ? mask : 0x0;
	writel(val, data->sfr_base + ABOX_QCHANNEL_DISABLE);
}
#endif

static void abox_cleanup_pcmc(struct abox_data *data)
{
	abox_release_pcmc(data);
	flush_work(&data->pcmc.request_work);
}

static void abox_early_cleanup(struct abox_data *data)
{
	switch (CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION) {
	case ABOX_SOC_VERSION(4, 0, 0):
		abox_cleanup_sifs_cnt_val(data);
		break;
	case ABOX_SOC_VERSION(4, 0, 1):
	case ABOX_SOC_VERSION(4, 0x20, 0):
	case ABOX_SOC_VERSION(4, 0x22, 0):
	case ABOX_SOC_VERSION(4, 0x23, 0):
	case ABOX_SOC_VERSION(4, 1, 0):
	case ABOX_SOC_VERSION(4, 2, 0):
	case ABOX_SOC_VERSION(4, 0x30, 0):
#if !IS_ENABLED(CONFIG_SOC_S5E9955) && !IS_ENABLED(CONFIG_SOC_S5E8855)
		abox_cleanup_qchannel_disable(data, ABOX_CCLK_ACP, true);
#endif
		break;
	default:
		/* ignore */
		break;
	}
}

static void abox_cleanup(struct abox_data *data)
{
	abox_release_sram_vts(data);
	abox_cleanup_pcmc(data);
	abox_cleanup_sidetone(data);

	switch (CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION) {
	case ABOX_SOC_VERSION(3, 1, 0):
		abox_cleanup_atune_bqf(data);
		fallthrough;
	case ABOX_SOC_VERSION(3, 1, 1):
		abox_cleanup_sifs_cnt_val(data);
		fallthrough;
	case ABOX_SOC_VERSION(4, 0, 0):
		abox_cleanup_dummy_start(data);
		abox_cleanup_spus_flush(data);
		break;
	case ABOX_SOC_VERSION(4, 0, 1):
	case ABOX_SOC_VERSION(4, 0x20, 0):
	case ABOX_SOC_VERSION(4, 0x22, 0):
	case ABOX_SOC_VERSION(4, 0x23, 0):
	case ABOX_SOC_VERSION(4, 1, 0):
	case ABOX_SOC_VERSION(4, 2, 0):
	case ABOX_SOC_VERSION(4, 0x30, 0):
#if !IS_ENABLED(CONFIG_SOC_S5E9955) && !IS_ENABLED(CONFIG_SOC_S5E8855)
		abox_cleanup_qchannel_disable(data, ABOX_CCLK_ACP, false);
#endif
		abox_cleanup_dummy_start(data);
		break;
	case ABOX_SOC_VERSION(4, 0x24, 0):
	case ABOX_SOC_VERSION(4, 4, 0):
		abox_cleanup_dummy_start(data);
		break;
	}
}

static void abox_mask_ext_interrupt(struct abox_data *data, bool en)
{
	struct regmap *regmap = data->regmap;
	int ret;

	ret = regmap_update_bits(regmap, ABOX_SYSPOWER_CTRL, ABOX_EXT_INTRRUPT_MASK_MASK,
			(unsigned int)en << ABOX_EXT_INTRRUPT_MASK_L);
	if (ret < 0) {
		abox_err(data->dev, "EXT_INTERRUPT_MASK write failed: %d\n", ret);
		return;
	}
	data->mask_ext_int = en;
}

static int abox_enable(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	bool has_reset;
	int ret = 0;

	abox_info(dev, "%s\n", __func__);

	/*Check the probe status of the abox module includes sub drivers */
	if (!data->probed) {
		abox_warn(dev, "abox has not been probed yet\n");
		goto error;
	}

	if (!data->cmpnt || !data->cmpnt->card->snd_card) {
		abox_warn(dev, "sound card has not been created yet\n");
		goto error;
	}

	abox_mask_ext_interrupt(data, true);

	abox_set_boost_qos(data);

	if (abox_test_quirk(data, ABOX_QUIRK_BIT_ARAM_MODE))
		writel(0x0, data->sysreg_base + ABOX_ARAM_CTRL);

	abox_silent_reset(data, false);
	abox_control_acp(data, true);
	abox_control_qch_bypass(data, true);

	abox_power_notifier_call_chain(data, true);
	abox_gic_enable_irq(data->dev_gic);
	abox_enable_wdt(data);

	abox_request_cpu_gear(dev, data, DEFAULT_CPU_GEAR_ID,
			ABOX_CPU_GEAR_MAX, "enable");

	if (data->clk_cpu) {
		ret = clk_enable(data->clk_cpu);
		if (ret < 0) {
			abox_err(dev, "Failed to enable cpu clock: %d\n", ret);
			goto error;
		}
	}

	ret = clk_enable(data->clk_audif);
	if (ret < 0) {
		abox_err(dev, "Failed to enable audif clock: %d\n", ret);
		goto error;
	}

	abox_pad_retention(data, false);
	abox_restore_register(data);
	has_reset = !abox_is_timer_set(data);
	if (!has_reset) {
		abox_info(dev, "wakeup from WFI\n");
		abox_update_suspend_wait_flag(data, false);
		abox_start_timer(data);
	} else {
		abox_gic_init_gic(data->dev_gic);

		ret = abox_download_firmware(dev);
		if (ret < 0) {
			if (ret != -EAGAIN)
				abox_err(dev, "Failed to download firmware\n");
			else
				ret = 0;

			abox_set_base_qos(data);
			abox_request_cpu_gear(dev, data, DEFAULT_CPU_GEAR_ID,
					0, "enable");
			goto error;
		}
	}

	abox_set_calliope_bootargs(data);
	abox_set_calliope_slogargs(data);
	abox_request_dram_on(dev, DEFAULT_SYS_POWER_ID, true);
	data->calliope_state = CALLIOPE_ENABLING;
	if (has_reset) {
		abox_cpu_power(true);
		abox_cpu_enable(true);
	}

	abox_pmu_aud_reset(data, true);
	data->enabled = true;
	if (has_reset)
		pm_wakeup_event(dev, BOOT_DONE_TIMEOUT_MS);
	else
		abox_boot_done(dev, data->calliope_version);

	schedule_work(&data->restore_data_work);
error:
	return ret;
}

void abox_force_enable(void)
{
	int ret;

	if(list_empty(&p_abox_data->conf.nodes)) {
		abox_info(p_abox_data->dev, "%s\n", __func__);
		if (p_abox_data->rebooting) {
			abox_info(p_abox_data->dev, "%s: skip due to power down\n", __func__);
			return;
		}
		pm_runtime_get_sync(p_abox_data->dev);
		if (!p_abox_data->enabled) {
			ret = abox_enable(p_abox_data->dev);
			if(ret < 0)
				abox_err(p_abox_data->dev, "Failed to enable abox, ret=%d\n", ret);
		}
		pm_runtime_put(p_abox_data->dev);
	}
}

#if IS_ENABLED(CONFIG_SOC_S5E9955)
void dma_check(struct abox_data *data)
{
	unsigned int val = 0;
	unsigned int ctrl_val = 0;
	u64 timeout;
	int i = 0;

	abox_info(data->dev, "%s\n", __func__);

	/*rdma check*/
	for(i = 0; i < data->rdma_count; i++) {
		timeout = local_clock() + abox_get_waiting_ns(false);
		regmap_read(data->regmap, ABOX_RDMA_STATUS(i), &val);
		while (!!(val & ABOX_RDMA_PROGRESS_MASK)) {
			if (local_clock() > timeout) {
				abox_info(data->dev, "%s(%d) timeout\n", __func__, i);
				dbg_snapshot_expire_watchdog();
				break;
			}

			regmap_read(data->regmap, ABOX_RDMA_CTRL0(i), &ctrl_val);
			abox_info(data->dev, "%s rdma(%d):0x%x / 0x%x\n", __func__,
					i, val, ctrl_val);
			regmap_update_bits(data->regmap, ABOX_RDMA_CTRL0(i),
					ABOX_DMA_ENABLE_MASK, 0);
			regmap_read(data->regmap, ABOX_RDMA_CTRL0(i), &ctrl_val);
			abox_info(data->dev, "%s d(0x%x)\n", __func__, ctrl_val);
		}
	}

	/*wdma check*/
	for(i = 0; i < data->wdma_count; i++) {
		timeout = local_clock() + abox_get_waiting_ns(false);
		regmap_read(data->regmap, ABOX_WDMA_STATUS(i), &val);
		while (!!(val & ABOX_WDMA_PROGRESS_MASK)) {
			if (local_clock() > timeout) {
				abox_info(data->dev, "%s(%d) timeout\n", __func__, i);
				dbg_snapshot_expire_watchdog();
				break;
			}

			regmap_read(data->regmap, ABOX_WDMA_CTRL0(i), &ctrl_val);
			abox_info(data->dev, "%s wdma(%d):0x%x / 0x%x\n", __func__,
					i, val, ctrl_val);
			regmap_update_bits(data->regmap, ABOX_WDMA_CTRL0(i),
					ABOX_DMA_ENABLE_MASK, 0);
			regmap_read(data->regmap, ABOX_WDMA_CTRL0(i), &ctrl_val);
			abox_info(data->dev, "%s d(0x%x)\n", __func__, ctrl_val);
		}
	}
	abox_info(data->dev, "%s done\n", __func__);
}
#endif

static int abox_disable(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	enum calliope_state state = data->calliope_state;

	abox_info(dev, "%s\n", __func__);

#if IS_ENABLED(CONFIG_SOC_S5E9955)
		dma_check(data);
#endif

	abox_update_suspend_wait_flag(data, true);
	data->calliope_state = CALLIOPE_DISABLING;
	abox_cache_components(dev, data);
	flush_work(&data->boot_done_work);
	abox_early_cleanup(data);
	abox_control_acp(data, false);
	if (state != CALLIOPE_DISABLED)
		abox_cpu_pm_ipc(data, false);
	data->calliope_state = CALLIOPE_DISABLED;
	abox_log_drain_all(dev);
	abox_request_dram_on(dev, DEFAULT_SYS_POWER_ID, false);
	abox_mask_ext_interrupt(data, false);
	abox_save_register(data);
	abox_pad_retention(data, true);
	data->enabled = false;
	data->restored = false;
	if (data->clk_cpu)
		clk_disable(data->clk_cpu);
	abox_gic_disable_irq(data->dev_gic);
	abox_failsafe_report_reset(dev);
	if (data->debug_mode != DEBUG_MODE_NONE)
		abox_dbg_dump_suspend(dev, data);
	abox_power_notifier_call_chain(data, false);
	abox_cleanup(data);
	abox_clear_base_qos(data);
	if(!abox_core_status())
		abox_pmu_aud_reset(data, false);

	return 0;
}

static int abox_runtime_suspend(struct device *dev)
{
	abox_dbg(dev, "%s\n", __func__);
	abox_sysevent_put(dev);

	return abox_disable(dev);
}

static int abox_runtime_resume(struct device *dev)
{
	abox_dbg(dev, "%s\n", __func__);
	abox_sysevent_get(dev);

	return abox_enable(dev);
}

static int abox_suspend(struct device *dev)
{
	abox_info(dev, "%s\n", __func__);

	if (pm_runtime_active(dev))
		abox_pm_ipc(dev_get_drvdata(dev), false);

	return 0;
}

static int abox_resume(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int val, reg;

	/*AUD_STATUS*/
	exynos_pmu_read(AUD_STATUS, &reg);

	abox_info(dev, "%s(%d - %d)\n", __func__, dev->power.runtime_status, reg);

	if (reg && 0x1) {
		val = readl(data->sfr_base + ABOX_SYSPOWER_CTRL);
		val &= ~ABOX_EXT_INTRRUPT_MASK_MASK;
		val |= data->mask_ext_int ? ABOX_EXT_INTRRUPT_MASK_MASK : 0x0;
		writel(val, data->sfr_base + ABOX_SYSPOWER_CTRL);
	}

	if (pm_runtime_active(dev)) {
		abox_pm_ipc(dev_get_drvdata(dev), true);
	}

	return 0;
}

static int abox_qos_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, qos_nb);
	struct device *dev = data->dev;

	if (!pm_runtime_active(dev))
		return NOTIFY_DONE;

	abox_notify_cpu_gear(data);
	abox_cmpnt_update_cnt_val(dev);
	abox_cmpnt_update_asrc_tick(dev);

	return NOTIFY_DONE;
}

int abox_print_power_usage(struct device *dev, void *data)
{
	abox_dbg(dev, "%s\n", __func__);

	if (pm_runtime_enabled(dev) && pm_runtime_active(dev)) {
		abox_info(dev, "usage_count:%d\n",
				atomic_read(&dev->power.usage_count));
		device_for_each_child(dev, data, abox_print_power_usage);
	}

	return 0;
}

static int abox_pm_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, pm_nb);
	struct device *dev = data->dev;
	unsigned long long audio_mode_time;
	unsigned long time;
	int ret;

	abox_dbg(dev, "%s(%lu)\n", __func__, action);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		pm_runtime_barrier(dev);
		abox_cpu_gear_barrier();
		flush_workqueue(data->ipc_workqueue);
		if (abox_is_clearable(dev, data)) {
			ret = pm_runtime_suspend(dev);
			if (ret < 0) {
				abox_info(dev, "runtime suspend: %d\n", ret);
				if (atomic_read(&dev->power.child_count) < 1) {
					abox_qos_print(dev, ABOX_QOS_AUD);
					abox_qos_print(dev, ABOX_QOS_FW0);
				}
				abox_print_power_usage(dev, NULL);
				audio_mode_time = data->audio_mode_time;
				time = do_div(audio_mode_time, NSEC_PER_SEC);
				abox_info(dev, "audio_mode: %d, updated at %llu.%06lus\n",
						data->audio_mode,
						audio_mode_time, time);
				return NOTIFY_BAD;
			}
		}
		break;
	default:
		/* Nothing to do */
		break;
	}
	return NOTIFY_DONE;
}

int abox_notify_modem_event(enum abox_modem_event event)
{
	struct abox_data *data = p_abox_data;
	struct device *dev = data->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	abox_info(dev, "%s(%d)\n", __func__, event);

	if (!data->cmpnt)
		return NOTIFY_DONE;

	msg.ipcid = IPC_SYSTEM;
	switch (event) {
	case ABOX_MODEM_EVENT_RESET:
		system_msg->msgtype = ABOX_RESET_VSS;
		break;
	case ABOX_MODEM_EVENT_EXIT:
		system_msg->msgtype = ABOX_STOP_VSS;
		if (abox_is_on()) {
			abox_dbg_print_gpr(dev, data);
			abox_failsafe_report(dev, false);
		}
		break;
	case ABOX_MODEM_EVENT_ONLINE:
		data->vss_disabled = false;
		system_msg->msgtype = abox_is_on() ? ABOX_START_VSS : 0;
		break;
	case ABOX_MODEM_EVENT_WATCHDOG:
		system_msg->msgtype = ABOX_WATCHDOG_VSS;
		break;
	default:
		system_msg->msgtype = 0;
		break;
	}

	if (system_msg->msgtype)
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 0);

	return NOTIFY_DONE;
}
EXPORT_SYMBOL(abox_notify_modem_event);

static int abox_itmon_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	const char keyword[] = "AUD";
	struct abox_data *data = container_of(nb, struct abox_data, itmon_nb);
	struct device *dev = data->dev;
	struct itmon_notifier *itmon_data = nb_data;

	if (!itmon_data)
		return NOTIFY_DONE;

	if (itmon_data->port && strstr(itmon_data->port, keyword)) {
		abox_info(dev, "%s(%lu)\n", __func__, action);
		abox_dbg_print_gpr(dev, data);
		abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_KERNEL, "itmon");
		abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_KERNEL, "itmon");
		data->enabled = false;
		return NOTIFY_BAD;
	}

	if (itmon_data->dest && strstr(itmon_data->dest, keyword)) {
		abox_info(dev, "%s(%lu)\n", __func__, action);
		data->enabled = false;
		return NOTIFY_BAD;
	}

	return NOTIFY_DONE;
}

static ssize_t calliope_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int version = be32_to_cpu(data->calliope_version);

	memcpy(buf, &version, sizeof(version));
	buf[4] = '\n';
	buf[5] = '\0';

	return 6;
}

static ssize_t calliope_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ABOX_IPC_MSG msg = {0,};
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	int ret;

	abox_dbg(dev, "%s\n", __func__);

	if (!pm_runtime_active(dev))
		return count;

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_REQUEST_DEBUG;
	ret = sscanf(buf, "%10d,%10d,%10d,%739s", &system_msg->param1,
			&system_msg->param2, &system_msg->param3,
			system_msg->bundle.param_bundle);
	if (ret < 0)
		return ret;

	ret = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t calliope_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	static const char cmd_reload_ext_bin[] = "RELOAD EXT BIN";
	static const char cmd_failsafe[] = "FAILSAFE";
	static const char cmd_cpu_gear[] = "CPU GEAR";
	static const char cmd_cpu_max[] = "CPU MAX";
	static const char cmd_test_ipc[] = "TEST IPC";
	static const char cmd_mux_pcmc[] = "MUX PCMC";
	static const char cmd_wdt[] = "WDT";
	static const char cmd_slog_size[] = "SLOG SIZE";
	static const char cmd_extra_card[] = "EXTRA CARD";
	static const char cmd_test_panic[] = "TEST PANIC";
	static const char cmd_test_wdt[] = "TEST WDT";
	static const char cmd_test_itmon[] = "TEST ITMON";
	static const char cmd_test_iommu_fault[] = "TEST IOMMU FAULT";
	static const char cmd_test_system_sleep[] = "TEST SYSTEM SLEEP";
	static const char cmd_test_cp_crash[] = "TEST CP CRASH";
	static const char cmd_test_exported_api[] = "TEST EXPORTED API";
	static const char cmd_load_topology[] = "LOAD TOPOLOGY";
	static const char cmd_load_calliope[] = "LOAD CALLIOPE";
	static const char cmd_load_slt[] = "TEST SLT";
	struct abox_data *data = dev_get_drvdata(dev);
	char name[80];
	int ret;

	abox_dbg(dev, "%s(%s)\n", __func__, buf);
	if (!strncmp(cmd_reload_ext_bin, buf, sizeof(cmd_reload_ext_bin) - 1)) {
		abox_dbg(dev, "reload ext bin\n");
		if (sscanf(buf, "RELOAD EXT BIN:%63s", name) == 1)
			abox_reload_extra_firmware(data, name);
	} else if (!strncmp(cmd_failsafe, buf, sizeof(cmd_failsafe) - 1)) {
		abox_dbg(dev, "failsafe\n");
		abox_failsafe_report(dev, true);
	} else if (!strncmp(cmd_cpu_gear, buf, sizeof(cmd_cpu_gear) - 1)) {
		unsigned int gear;

		abox_info(dev, "set clk\n");
		ret = kstrtouint(buf + sizeof(cmd_cpu_gear), 10, &gear);
		if (!ret) {
			abox_info(dev, "gear = %u\n", gear);
			pm_runtime_get_sync(dev);
			abox_request_cpu_gear(dev, data, TEST_CPU_GEAR_ID,
					gear, "calliope_cmd");
			pm_runtime_mark_last_busy(dev);
			pm_runtime_put_autosuspend(dev);
		}
	} else if (!strncmp(cmd_cpu_max, buf, sizeof(cmd_cpu_max) - 1)) {
		unsigned int id = 0, max = 0;

		abox_info(dev, "set clk max\n");
		ret = sscanf(buf, "CPU MAX=%u %u", &max, &id);
		if (ret > 0) {
			if (ret < 2)
				id = TEST_CPU_GEAR_ID;
			abox_info(dev, "max = %u, id = %#x\n", max, id);
			pm_runtime_get_sync(dev);
			abox_qos_request_aud_max(dev, id, max, "calliope_cmd");
			pm_runtime_mark_last_busy(dev);
			pm_runtime_put_autosuspend(dev);
		}
	} else if (!strncmp(cmd_test_ipc, buf, sizeof(cmd_test_ipc) - 1)) {
		unsigned int size;

		abox_info(dev, "test ipc\n");
		ret = kstrtouint(buf + sizeof(cmd_test_ipc), 10, &size);
		if (!ret) {
			ABOX_IPC_MSG *msg;
			size_t msg_size;
			int i;

			abox_info(dev, "size = %u\n", size);
			msg_size = offsetof(ABOX_IPC_MSG, msg.system.bundle)
					+ (size * 4);
			msg = kmalloc(msg_size, GFP_KERNEL);
			if (msg) {
				msg->ipcid = IPC_SYSTEM;
				msg->msg.system.msgtype = ABOX_PRINT_BUNDLE;
				msg->msg.system.param1 = size;
				for (i = 0; i < size; i++)
					msg->msg.system.bundle.param_s32[i] = i;
				abox_request_ipc(dev, IPC_SYSTEM, msg, msg_size,
						0, 0);
				kfree(msg);
			}
		}
	} else if (!strncmp(cmd_load_slt, buf, sizeof(cmd_load_slt) - 1)) {
		unsigned int timeout;

		abox_info(dev, "TEST SLT MODE\n");

		ret = kstrtouint(buf + sizeof(cmd_load_slt), 10, &timeout);
		if (!ret) {
			abox_info(dev, "suspend time = %u\n", timeout);
			pm_runtime_set_autosuspend_delay(dev, timeout);
			data->test_mode = TEST_MODE_SLT;
		} else {
			pm_runtime_set_autosuspend_delay(dev, 500);
			data->test_mode = TEST_MODE_NONE;
		}
	} else if (!strncmp(cmd_mux_pcmc, buf, sizeof(cmd_mux_pcmc) - 1)) {
		enum mux_pcmc mux = ABOX_PCMC_OSC;

		ret = sscanf(buf, "MUX PCMC: %d", &mux);
		if (ret > 0)
			abox_request_pcmc(data, mux);
	} else if (!strncmp(cmd_wdt, buf, sizeof(cmd_wdt) - 1)) {
		dbg_snapshot_expire_watchdog();
	} else if (!strncmp(cmd_slog_size, buf, sizeof(cmd_slog_size) - 1)) {
		unsigned int size;

		ret = kstrtouint(buf + sizeof(cmd_slog_size), 0, &size);
		if (!ret) {
			abox_info(dev, "slog size = %u\n", size);
			data->slog_size = size;
		}
	} else if (!strncmp(cmd_extra_card, buf, sizeof(cmd_extra_card) - 1)) {
		int idx = SNDRV_CARDS;

		ret = kstrtouint(buf + sizeof(cmd_extra_card), 0, &idx);
		if (ret >= 0)
			abox_register_extra_sound_card(dev, NULL, idx);
	} else if (!strncmp(cmd_test_panic, buf, sizeof(cmd_test_panic) - 1)) {
		abox_panic_handler(&abox_panic_notifier, 0, NULL);
		abox_clear_cpu_gear_requests(dev);
	} else if (!strncmp(cmd_test_wdt, buf, sizeof(cmd_test_wdt) - 1)) {
		abox_wdt_handler(IRQ_WDT, data);
		abox_clear_cpu_gear_requests(dev);
	} else if (!strncmp(cmd_test_itmon, buf, sizeof(cmd_test_itmon) - 1)) {
		struct itmon_notifier itmon_data = { 0, };

		itmon_data.port = "AUD";
		itmon_data.dest = "";
		abox_itmon_notifier(&data->itmon_nb, 0, &itmon_data);

		itmon_data.port = "";
		itmon_data.dest = "AUD";
		abox_itmon_notifier(&data->itmon_nb, 0, &itmon_data);

		abox_clear_cpu_gear_requests(dev);
	} else if (!strncmp(cmd_test_iommu_fault, buf, sizeof(cmd_test_iommu_fault) - 1)) {
		abox_iommu_fault_handler(NULL, data);
	} else if (!strncmp(cmd_test_system_sleep, buf, sizeof(cmd_test_system_sleep) - 1)) {
		ret = abox_pm_notifier(&data->pm_nb, PM_SUSPEND_PREPARE, NULL);
		abox_info(dev, "pm notifier: %d\n", ret);
		abox_suspend(dev);
		msleep(1000);
		abox_resume(dev);
	} else if (!strncmp(cmd_test_cp_crash, buf, sizeof(cmd_test_cp_crash) - 1)) {
		abox_notify_modem_event(ABOX_MODEM_EVENT_EXIT);
		msleep(1000);
		abox_notify_modem_event(ABOX_MODEM_EVENT_ONLINE);
	} else if (!strncmp(cmd_test_exported_api, buf, sizeof(cmd_test_exported_api) - 1)) {
		unsigned int value;

		value = abox_get_requiring_int_freq_in_khz();
		abox_info(dev, "abox_get_requiring_int_freq_in_khz=%u\n", value);
		value = abox_get_requiring_mif_freq_in_khz();
		abox_info(dev, "abox_get_requiring_mif_freq_in_khz=%u\n", value);
		value = abox_get_requiring_aud_freq_in_khz();
		abox_info(dev, "abox_get_requiring_aud_freq_in_khz=%u\n", value);
		value = abox_get_routing_path();
		abox_info(dev, "abox_get_routing_path=%u\n", value);
		value = abox_get_call_state();
		abox_info(dev, "abox_get_call_state=%u\n", value);
	} else if (!strncmp(cmd_load_topology, buf, sizeof(cmd_load_topology) - 1)) {
		if (sscanf(buf + sizeof(cmd_load_topology), "%79s", name) == 1)
			abox_tplg_load_extra_firmware(data->cmpnt, name);
	} else if (!strncmp(cmd_load_calliope, buf, sizeof(cmd_load_calliope) - 1)) {
		abox_core_change_firmware();
	}

	return count;
}

static DEVICE_ATTR_RO(calliope_version);
static DEVICE_ATTR_WO(calliope_debug);
static DEVICE_ATTR_WO(calliope_cmd);

static struct reserved_mem *abox_rmem;
static int __init abox_rmem_setup(struct reserved_mem *rmem)
{
	pr_info("%s: size=%pa\n", __func__, &rmem->size);
	abox_rmem = rmem;
	return 0;
}
RESERVEDMEM_OF_DECLARE(abox_rmem, "exynos,abox_rmem", abox_rmem_setup);

static void abox_memlog_register(struct abox_data *data)
{
	int ret;

	ret = memlog_register("@box", data->dev, &data->drvlog_desc);
	if (ret) {
		dev_err(data->dev, "Failed to register abox memlog\n");
		return;
	}

	data->drv_log_file_obj = memlog_alloc_file(data->drvlog_desc,
			"abox-file", SZ_512K, SZ_2M, 200, 10);
	if (!data->drv_log_file_obj) {
		dev_err(data->dev, "%s : %d : Failed to allocate a file for driver log\n",
				__func__, __LINE__);
		return;
	}

	data->drv_log_obj = memlog_alloc_printf(data->drvlog_desc, SZ_512K,
			data->drv_log_file_obj, "abox-mem", 0);
	if (!data->drv_log_obj) {
		dev_err(data->dev, "%s : %d : Failed to allocate memory for driver log\n",
				__func__, __LINE__);
		return;
	}
	data->drv_log_obj->log_level = MEMLOG_LEVEL_CAUTION;
}

static int abox_sysevent_powerup(const struct sysevent_desc *sysevent)
{
	dev_info(sysevent->dev, "%s: powerup callback\n", __func__);
	return 0;
}

static int abox_sysevent_shutdown(const struct sysevent_desc *sysevent, bool force_stop)
{
	dev_info(sysevent->dev, "%s: shutdown callback\n", __func__);
	return 0;
}

static void abox_sysevent_crash_shutdown(const struct sysevent_desc *sysevent)
{
	dev_info(sysevent->dev, "%s: crash callback\n", __func__);
}

static void abox_sysevent_register(struct abox_data *data)
{
	int ret;

	data->sysevent_dev = NULL;
	data->sysevent_desc.name = "ABOX";
	data->sysevent_desc.owner = THIS_MODULE;
	data->sysevent_desc.shutdown = abox_sysevent_shutdown;
	data->sysevent_desc.powerup = abox_sysevent_powerup;
	data->sysevent_desc.crash_shutdown = abox_sysevent_crash_shutdown;
	data->sysevent_desc.dev = data->dev;
	data->sysevent_dev = sysevent_register(&data->sysevent_desc);
	if (IS_ERR(data->sysevent_dev)) {
		ret = PTR_ERR(data->sysevent_dev);
		dev_err(data->dev, "ABOX: sysevent_register failed :%d\n", ret);
	} else {
		dev_info(data->dev, "ABOX: sysevent_register success\n");
	}
}

/* sub-driver list */
static struct platform_driver *abox_sub_drivers[] = {
	&samsung_abox_debug_driver,
	&samsung_abox_core_driver,
	&samsung_abox_dump_driver,
	&samsung_abox_dma_driver,
	&samsung_abox_vdma_driver,
	&samsung_abox_wdma_driver,
	&samsung_abox_rdma_driver,
	&samsung_abox_if_driver,
	&samsung_abox_vss_driver,
	&samsung_abox_effect_driver,
	&samsung_abox_tplg_driver,
};

static int samsung_abox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *np_tmp;
	struct platform_device *pdev_tmp;
	struct abox_data *data;
	phys_addr_t paddr;
	size_t size;
	void *addr;
	int ret, i;

	dev_info(dev, "%s\n", __func__);

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(36));
	set_dma_ops(dev, NULL); /* ABOX driver use manual iommu mapping */
	platform_set_drvdata(pdev, data);
	data->dev = dev;
	p_abox_data = data;

	data->probed = false;

	abox_probe_quirks(data, np);
	init_waitqueue_head(&data->ipc_wait_queue);
	init_waitqueue_head(&data->boot_wait_queue);
	init_waitqueue_head(&data->wait_queue);
	init_waitqueue_head(&data->offline_poll_wait);
	init_waitqueue_head(&data->udma_fade_done);
	spin_lock_init(&data->ipc_queue_lock);
	spin_lock_init(&data->iommu_lock);
	device_init_wakeup(dev, true);
	if (IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_ABOX_DEBUG)) {
		data->debug_mode = DEBUG_MODE_DRAM;
		abox_info(dev, "debug mode enabled\n");
	}
	data->cpu_gear_min = 3; /* default value from kangchen */
	for (i = 0; i < ARRAY_SIZE(data->sif_rate); i++) {
		data->sif_rate[i] = 48000;
		data->sif_format[i] = SNDRV_PCM_FORMAT_S16;
		data->sif_channels[i] = 2;
	}
	INIT_WORK(&data->ipc_work, abox_process_ipc);
	INIT_WORK(&data->add_extra_firmware_controls_work,
			abox_add_extra_firmware_controls_work_func);
	INIT_WORK(&data->register_component_work,
			abox_register_component_work_func);
	INIT_WORK(&data->restore_data_work, abox_restore_data_work_func);
	INIT_WORK(&data->boot_done_work, abox_boot_done_work_func);
	INIT_DEFERRABLE_WORK(&data->boot_clear_work, abox_boot_clear_work_func);
	INIT_DELAYED_WORK(&data->wdt_work, abox_wdt_work_func);
	INIT_LIST_HEAD(&data->firmware_extra);
	INIT_LIST_HEAD(&data->ipc_actions);
	INIT_LIST_HEAD(&data->iommu_maps);
	INIT_WORK(&data->notify_bargein_detect_work, abox_notify_bargein_detect_work_func);

	for (i = 10; i; i--) {
		data->iommu_domain = iommu_get_domain_for_dev(dev);
		if (data->iommu_domain)
			break;
		abox_warn(dev, "Unable to get iommu domain\n");
		msleep(100);
	}
	if (!data->iommu_domain) {
		abox_err(dev, "Unable to get iommu domain\n");
		return -EPROBE_DEFER;
	}

	np_tmp = of_parse_phandle(np, "samsung,abox-gic", 0);
	if (!np_tmp) {
		abox_err(dev, "Failed to get abox-gic device node\n");
		return -EPROBE_DEFER;
	}
	pdev_tmp = of_find_device_by_node(np_tmp);
	if (!pdev_tmp) {
		abox_err(dev, "Failed to get abox-gic platform device\n");
		return -EPROBE_DEFER;
	}
	data->dev_gic = &pdev_tmp->dev;

	data->ipc_workqueue = alloc_workqueue("%s", WQ_HIGHPRI | WQ_UNBOUND |
			WQ_MEM_RECLAIM | WQ_SYSFS, 1, "abox_ipc");
	if (!data->ipc_workqueue) {
		abox_err(dev, "Couldn't create workqueue %s\n", "abox_ipc");
		return -ENOMEM;
	}

	data->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(data->pinctrl)) {
		abox_dbg(dev, "Couldn't get pins (%li)\n",
				PTR_ERR(data->pinctrl));
		data->pinctrl = NULL;
	}

	data->sfr_base = devm_get_request_ioremap(pdev, "sfr",
			&data->sfr_phys, &data->sfr_size);
	if (IS_ERR(data->sfr_base))
		return PTR_ERR(data->sfr_base);

	data->sysreg_base = devm_get_request_ioremap(pdev, "sysreg",
			&data->sysreg_phys, &data->sysreg_size);
	if (IS_ERR(data->sysreg_base))
		return PTR_ERR(data->sysreg_base);

	data->sram_base = devm_get_request_ioremap(pdev, "sram",
			&data->sram_phys, NULL);
	if (IS_ERR(data->sram_base))
		return PTR_ERR(data->sram_base);

	data->timer_base = devm_get_request_ioremap(pdev, "timer",
			NULL, NULL);
	if (IS_ERR(data->timer_base))
		return PTR_ERR(data->timer_base);

	if (!abox_rmem) {
		np_tmp = of_parse_phandle(np, "memory-region", 0);
		if (np_tmp)
			abox_rmem = of_reserved_mem_lookup(np_tmp);
	}

	if (abox_rmem) {
		data->dram_phys = abox_rmem->base;
		data->dram_base = rmem_vmap(abox_rmem);
		abox_info(dev, "%s(%#x) alloc: rmem\n", "dram firmware", DRAM_FIRMWARE_SIZE);
	} else {
		data->dram_base = dmam_alloc_coherent(dev, DRAM_FIRMWARE_SIZE,
				&data->dram_phys, GFP_KERNEL);
		if (!data->dram_base) {
			dev_err(dev, "%s: no memory\n", "dram firmware");
			return -ENOMEM;
		}
		abox_info(dev, "%s(%#x) alloc: dmam_alloc_coherent\n", "dram firmware", DRAM_FIRMWARE_SIZE);
	}

	abox_info(dev, "%s(%#x) alloc\n", "dram firmware", DRAM_FIRMWARE_SIZE);
	abox_iommu_map(dev, IOVA_DRAM_FIRMWARE, data->dram_phys,
			DRAM_FIRMWARE_SIZE, data->dram_base);

	if (CONFIG_SND_SOC_SAMSUNG_ABOX_PARAM_SIZE) {
		data->dram_para_phys = data->dram_phys + DRAM_FIRMWARE_SIZE;
		data->dram_para_base = data->dram_base + DRAM_FIRMWARE_SIZE;
		abox_info(dev, "%s(%#x) alloc\n", "dram parameter", DRAM_PARAMETER_SIZE);
		abox_iommu_map(dev, IOVA_DRAM_PARAMETER, data->dram_para_phys,
				DRAM_PARAMETER_SIZE, data->dram_para_base);
	}

	of_property_read_u32(np, "samsung,abox-bootargs-offset",
			&data->bootargs_offset);
	of_property_read_string(np, "samsung,abox-bootargs", &data->bootargs);
	abox_info(dev, "bootargs: %#x, %s\n", data->bootargs_offset,
			data->bootargs ? data->bootargs : "");

	of_property_read_u32(np, "samsung,abox-slogargs-offset",
			&data->slogargs_offset);

#if IS_ENABLED(CONFIG_SHM_IPC)
	paddr = shm_get_vss_base();
#else
	paddr = 0;
#endif
	if (paddr) {
#if IS_ENABLED(CONFIG_SHM_IPC)
		abox_info(dev, "%s(%#x) alloc\n", "vss firmware",
				shm_get_vss_size());
		abox_iommu_map(dev, IOVA_VSS_FIRMWARE, paddr,
				shm_get_vss_size(), shm_get_vss_region());
#endif
	} else {
		if (data->bootargs == NULL) {
			/*for bring up*/
			abox_info(dev, "%s(%#x) alloc\n", "vss firmware virtual",
					PHSY_VSS_SIZE);
			addr = dmam_alloc_coherent(dev, PHSY_VSS_SIZE, &paddr,
					GFP_KERNEL);
			memset(addr, 0x0, PHSY_VSS_SIZE);
			abox_iommu_map(dev, IOVA_VSS_FIRMWARE, paddr, PHSY_VSS_SIZE,
					addr);
		} else {
			/*for wifi only model*/
			abox_info(dev, "%s(%#x--%#x) alloc\n", "vss firmware",
			IOVA_VSS_FIRMWARE_WIFI,	SZ_1M);
				addr = dmam_alloc_coherent(dev, SZ_1M, &paddr,
					GFP_KERNEL);
			memset(addr, 0x0, SZ_1M);
			abox_iommu_map(dev, IOVA_VSS_FIRMWARE_WIFI, paddr, SZ_1M,
					addr);
		}
	}

#if IS_ENABLED(CONFIG_SHM_IPC)
	paddr = shm_get_vparam_base();
	abox_info(dev, "%s(%#x) alloc\n", "vss parameter",
			shm_get_vparam_size());
	abox_iommu_map(dev, IOVA_VSS_PARAMETER, paddr, shm_get_vparam_size(),
			shm_get_vparam_region());
#endif
	//abox_iommu_map(dev, 0x10000000, 0x10000000, PAGE_SIZE, 0);

	if (get_resource_mem(pdev, "mailbox_apm", &paddr, &size) >= 0) {
		abox_info(dev, "mapping %s\n", "mailbox_apm");
		abox_iommu_map(dev, paddr, paddr, size, 0);
	}

	if (get_resource_mem(pdev, "cmu_top", &paddr, &size) >= 0) {
		abox_info(dev, "mapping %s\n", "cmu_top");
		abox_iommu_map(dev, paddr, paddr, size, 0);
	}

	iommu_register_device_fault_handler(dev, abox_iommu_fault_handler,
			data);

	data->clk_pll = devm_clk_get_and_prepare(pdev, "pll");
	if (IS_ERR(data->clk_pll))
		return PTR_ERR(data->clk_pll);

	data->clk_pll1 = devm_clk_get_and_prepare(pdev, "pll1");
	if (IS_ERR(data->clk_pll1)) {
		data->clk_pll1 = NULL;
		dev_info(dev, "failed to get pll1\n");
	}

	data->clk_audif = devm_clk_get_and_prepare(pdev, "audif");
	if (IS_ERR(data->clk_audif))
		return PTR_ERR(data->clk_audif);

	data->clk_cpu = devm_clk_get_and_prepare(pdev, "cpu");
	if (IS_ERR(data->clk_cpu)) {
		if (IS_ENABLED(CONFIG_SOC_EXYNOS8895))
			return PTR_ERR(data->clk_cpu);

		data->clk_cpu = NULL;
	}

	data->clk_dmic = devm_clk_get_and_prepare(pdev, "dmic");
	if (IS_ERR(data->clk_dmic))
		data->clk_dmic = NULL;

	data->clk_bus = devm_clk_get_and_prepare(pdev, "bus");
	if (IS_ERR(data->clk_bus))
		return PTR_ERR(data->clk_bus);

	data->clk_cnt = devm_clk_get_and_prepare(pdev, "cnt");
	if (IS_ERR(data->clk_cnt))
		data->clk_cnt = NULL;

	data->clk_sclk = devm_clk_get_and_prepare(pdev, "sclk");
	if (!IS_ERR(data->clk_sclk)) {
		ret = clk_set_rate(data->clk_sclk, clk_get_rate(data->clk_audif));
		if (ret < 0)
			dev_err(dev, "%s: failed to set rate: %d\n", "sclk", ret);
	}

	ret = of_samsung_property_read_u32(dev, np, "uaif-max-div",
			&data->uaif_max_div);
	if (ret < 0)
		data->uaif_max_div = 32;

	of_samsung_property_read_u32_array(dev, np, "pm-qos-int",
			data->pm_qos_int, ARRAY_SIZE(data->pm_qos_int));

	ret = of_samsung_property_read_variable_u32_array(dev, np, "pm-qos-aud",
			data->pm_qos_aud, 1, ARRAY_SIZE(data->pm_qos_aud));
	if (ret >= 0) {
		for (i = 0; i < ARRAY_SIZE(data->pm_qos_aud); i++) {
			if (!data->pm_qos_aud[i]) {
				data->cpu_gear_min = i;
				break;
			}
		}
	}

	of_samsung_property_read_u32(dev, np, "pm-qos-stable-min",
			&data->pm_qos_stable_min);

#if !IS_ENABLED(CONFIG_SOC_S5E8855)
	of_samsung_property_read_u32_array(dev, np, "pmu-aud-mask",
			data->pmu_aud_mask, ARRAY_SIZE(data->pmu_aud_mask));
	of_samsung_property_read_u32_array(dev, np, "pmu-aud-gresetn-cpu",
			data->pmu_aud_gresetn_cpu, ARRAY_SIZE(data->pmu_aud_gresetn_cpu));
	of_samsung_property_read_u32_array(dev, np, "pmu-aud-cnt-en-wdt",
			data->pmu_aud_cnt_en_wdt, ARRAY_SIZE(data->pmu_aud_cnt_en_wdt));
#endif

	of_samsung_property_read_u32_array(dev, np, "pmu-pad-ret",
			data->pmu_pad_ret, ARRAY_SIZE(data->pmu_pad_ret));
	of_samsung_property_read_u32_array(dev, np, "pmu-silent-rst",
			data->pmu_silent_rst, ARRAY_SIZE(data->pmu_silent_rst));
	of_samsung_property_read_u32_array(dev, np, "sys-acp-con",
			data->sys_acp_con, ARRAY_SIZE(data->sys_acp_con));
	of_samsung_property_read_u32_array(dev, np, "sys-qch-bypass",
			data->sys_qch_bypass, ARRAY_SIZE(data->sys_qch_bypass));
	of_samsung_property_read_u32_array(dev, np, "rate-pcmc",
			data->rate_pcmc, ARRAY_SIZE(data->rate_pcmc));

	data->pad_ret_skip = of_samsung_property_read_bool(dev, np,
		"pmu-pad-ret-skip");
	if (data->pad_ret_skip)
		abox_info(dev, "Do not control PAD retention at ABOX\n");

	for (i = 0; i < ARRAY_SIZE(data->file_name); i++) {
		of_property_read_string_index(np, "samsung,conf-file-name", i, &data->file_name[i]);
		dev_info(dev, "conf file[%d]: %s\n", i, data->file_name[i]);
	}

	abox_memlog_register(data);
	abox_sysevent_register(data);

	abox_gic_register_irq_handler(data->dev_gic, IRQ_WDT, abox_wdt_handler,
			data);
	for (i = 0; i < SGI_ABOX_MSG; i++)
		abox_gic_register_irq_handler(data->dev_gic, i,
				abox_irq_handler, dev);

	abox_of_get_addr(data, np, "samsung,ipc-tx-area", &data->ipc_tx_addr,
			NULL, &data->ipc_tx_size);
	abox_of_get_addr(data, np, "samsung,ipc-rx-area", &data->ipc_rx_addr,
			NULL, &data->ipc_rx_size);
	abox_of_get_addr(data, np, "samsung,handshake-area",
			(void **)&data->hndshk_tag, NULL, NULL);
	ret = abox_of_get_addr(data, np, "samsung,shm-area", &data->shm_addr,
			NULL, &data->shm_size);
	if (ret == 0)
		abox_shm_init(data->shm_addr, data->shm_size);
	else
		abox_shm_alloc(dev);

	abox_ipc_init_dev(dev);
	for (i = 0; i < IPC_ID_COUNT; i++)
		abox_ipc_register_handler(dev, i, abox_ipc_handler, pdev);

	abox_init_config(data);
	abox_parse_extra_firmware(data);

	ret = abox_probe_sram_vts(data);
	if (ret < 0)
		dev_info(dev, "failed to set sram vts: %d\n", ret);

	ret = abox_probe_pcmc(data);
	if (ret < 0)
		dev_info(dev, "failed to set pcmc: %d\n", ret);

	abox_proc_probe();

	data->regmap = abox_soc_get_regmap(dev);

	data->timer_regmap = devm_regmap_init_mmio(dev,
			data->timer_base,
			&abox_timer_regmap_config);

	abox_qos_init(dev);

	pm_runtime_enable(dev);
	pm_runtime_set_autosuspend_delay(dev, 500);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_get(dev);

	data->qos_nb.notifier_call = abox_qos_notifier;
	abox_qos_add_notifier(ABOX_QOS_AUD, &data->qos_nb);
	abox_qos_add_notifier(ABOX_QOS_AUD_MAX, &data->qos_nb);

	data->pm_nb.notifier_call = abox_pm_notifier;
	register_pm_notifier(&data->pm_nb);

	data->itmon_nb.notifier_call = abox_itmon_notifier;
	itmon_notifier_chain_register(&data->itmon_nb);

	abox_failsafe_init(dev);

	ret = device_create_file(dev, &dev_attr_calliope_version);
	if (ret < 0)
		abox_warn(dev, "Failed to create file: %s\n", "version");

	ret = device_create_file(dev, &dev_attr_calliope_debug);
	if (ret < 0)
		abox_warn(dev, "Failed to create file: %s\n", "debug");

	ret = device_create_file(dev, &dev_attr_calliope_cmd);
	if (ret < 0)
		abox_warn(dev, "Failed to create file: %s\n", "cmd");

	atomic_notifier_chain_register(&panic_notifier_list,
			&abox_panic_notifier);

	ret = abox_cmpnt_register(dev);
	if (ret < 0)
		abox_err(dev, "component register failed: %d\n", ret);

	abox_vdma_init(dev);

	platform_register_drivers(abox_sub_drivers, ARRAY_SIZE(abox_sub_drivers));
	abox_pci_register_driver();
	of_platform_populate(np, NULL, NULL, dev);

	data->probed = true;

	data->rebooting = false;

	data->test_mode = TEST_MODE_NONE;

	pm_runtime_put(dev);

	/*Hack patch to access CHUB gpio with non-secure*/
//	if (IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E9945))
//		exynos_smc(SMC_CMD_CONN_IF, (2ull << 32) | EXYNOS_SET_CONN_TZPC, 0, 0);

#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
#if IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	data->debug_mode = DEBUG_MODE_NONE;
	abox_info(dev, "debug_mode NONE\n");
#else
	data->debug_mode = DEBUG_MODE_FILE;
	abox_info(dev, "debug_mode FILE\n");
#endif
#endif

	abox_info(dev, "%s: probe complete\n", __func__);
	return 0;
}

static int samsung_abox_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);

	abox_info(dev, "%s\n", __func__);

	abox_free_config(data);
	abox_remove_pcmc(data);
	abox_proc_remove();
	pm_runtime_disable(dev);
#ifndef CONFIG_PM
	abox_runtime_suspend(dev);
#endif
	device_init_wakeup(dev, false);
	destroy_workqueue(data->ipc_workqueue);
	snd_soc_unregister_component(dev);
	abox_iommu_unmap(dev, IOVA_DRAM_FIRMWARE);

	return 0;
}

static void samsung_abox_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	abox_info(dev, "%s\n", __func__);

	if (data)
		data->rebooting = true;

	if (data && data->regmap)
		abox_save_register(data);
	pm_runtime_disable(dev);
}

static const struct of_device_id samsung_abox_match[] = {
	{
		.compatible = "samsung,abox",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_match);

static const struct dev_pm_ops samsung_abox_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(abox_suspend, abox_resume)
	SET_RUNTIME_PM_OPS(abox_runtime_suspend, abox_runtime_resume, NULL)
};

static struct platform_driver samsung_abox_driver = {
	.probe  = samsung_abox_probe,
	.remove = samsung_abox_remove,
	.shutdown = samsung_abox_shutdown,
	.driver = {
		.name = "abox",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_match),
		.pm = &samsung_abox_pm,
	},
};

module_platform_driver(samsung_abox_driver);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box Driver");
MODULE_ALIAS("platform:abox");
MODULE_LICENSE("GPL");
