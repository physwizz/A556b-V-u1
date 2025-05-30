// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * CHUB IF Driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 * Authors:
 *      Boojin Kim <boojin.kim@samsung.com>
 *      Sukwon Ryu <sw.ryoo@samsung.com>
 *
 */

#if IS_ENABLED(CONFIG_EXYNOS_SYSTEM_EVENT)
#include <soc/samsung/exynos/sysevent.h>
#endif
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
#include <soc/samsung/exynos/memlogger.h>
#endif
#if IS_ENABLED(CONFIG_EXYNOS_ITMON) || IS_ENABLED(CONFIG_EXYNOS_ITMON_V2)
#include <soc/samsung/exynos/exynos-itmon.h>
#endif

#include <linux/interrupt.h>
#include <linux/panic_notifier.h>

#include "chub.h"
#include "ipc_chub.h"
#include "chub_dbg.h"
#include "chub_exynos.h"

#if IS_ENABLED(CONFIG_SHUB)
#include "chub_shub.h"
#endif

void contexthub_check_time(void)
{
	struct rtc_device *chub_rtc = rtc_class_open(CONFIG_RTC_SYSTOHC_DEVICE);
	struct rtc_device *ap_rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	struct rtc_time chub_tm, ap_tm;
	time64_t chub_t;

	rtc_read_time(ap_rtc, &chub_tm);
	rtc_read_time(chub_rtc, &ap_tm);

	chub_t = rtc_tm_sub(&chub_tm, &ap_tm);

	if (chub_t) {
		nanohub_info("nanohub %s: diff_time: %llu\n", __func__, chub_t);
		rtc_set_time(chub_rtc, &ap_tm);
	};

	rtc_class_close(chub_rtc);
	rtc_class_close(ap_rtc);
}

static irqreturn_t contexthub_irq_handler(int irq, void *data)
{
	struct contexthub_ipc_info *chub = data;
	struct cipc_info *cipc;
	enum ipc_mb_id rcv_mb_id;
	unsigned int status;
	int start_index;
	int irq_num;
	int ret;

	cipc = chub->chub_ipc->cipc;
	rcv_mb_id = cipc->user_info[CIPC_USER_CHUB2AP].map_info.dst.mb_id;
	status = ipc_hw_read_int_status_reg_all(chub->iomem.mailbox, rcv_mb_id);
	start_index = ipc_hw_start_bit(rcv_mb_id);
	irq_num = IRQ_NUM_CHUB_ALIVE + start_index;

	if (atomic_read(&chub->atomic.chub_sleep)) {
		nanohub_dev_info(chub->dev, "%s before resume: status:0x%x\n", __func__, status);
		chub->misc.wakeup_by_chub_cnt++;
	}

	/* chub alive interrupt handle */
	if (status & (1 << irq_num)) {
		status &= ~(1 << irq_num);
		ipc_hw_clear_int_pend_reg(chub->iomem.mailbox, rcv_mb_id, irq_num);
		if ((atomic_read(&chub->atomic.chub_status) == CHUB_ST_POWER_ON) && chub->misc.multi_os) {
			schedule_work(&chub->debug_work);
			return IRQ_HANDLED;
		}

		if (ipc_hw_read_shared_reg(chub->iomem.mailbox, SR_3) == 0xFFFFFFFF) {
			//reset
			contexthub_handle_debug(chub, CHUB_ERR_FW_FAULT);
			return IRQ_HANDLED;
		}

		if (ipc_read_hw_value(IPC_VAL_HW_AP_STATUS) == CHUB_REBOOT_REQ) {
			nanohub_dev_err(chub->dev, "chub sends to request reboot\n");
			contexthub_handle_debug(chub, CHUB_ERR_FW_REBOOT);
		} else {
			/* set wakeup flag for chub_alive_lock */
			chub_wake_event(&chub->event.chub_alive_lock);
		}
	}

	if (contexthub_get_token(chub)) {
		nanohub_dev_err(chub->dev, "%s: in reset irq_status:%d\n", __func__, status);
		ipc_hw_clear_all_int_pend_reg(chub->iomem.mailbox, rcv_mb_id);
		return IRQ_HANDLED;
	}

	irq_num = IRQ_NUM_CHUB_LOG + start_index;
	if (status & (1 << irq_num)) {
		status &= ~(1 << irq_num);
		ipc_hw_clear_int_pend_reg(chub->iomem.mailbox, rcv_mb_id, irq_num);
	}

	if (status) {
		ret = cipc_irqhandler(CIPC_USER_CHUB2AP, status);
		if (ret)
			contexthub_handle_debug(chub, CHUB_ERR_ISR);
	}
	contexthub_put_token(chub);
	wake_up(&chub->log.log_kthread_wait);
	return IRQ_HANDLED;
}

static irqreturn_t contexthub_irq_wdt_handler(int irq, void *data)
{
	struct contexthub_ipc_info *chub = data;

	nanohub_dev_info(chub->dev, "%s called\n", __func__);
	disable_irq_nosync(chub->irq.irq_wdt);
	chub->irq.irq_wdt_disabled = 1;
	contexthub_handle_debug(chub, CHUB_ERR_FW_WDT);

	return IRQ_HANDLED;
}

static void __iomem *get_iomem_with_range(struct platform_device *pdev, const char *name,
					  phys_addr_t *base, resource_size_t *size)
{
	struct resource *res;
	void __iomem *ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	ret = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ret)) {
		nanohub_dev_err(&pdev->dev, "fails to ioremap resource %s\n", name);
		return ret;
	}

	if (base && size) {
		*base = res->start;
		*size = resource_size(res);
	}

	return ret;
}

static void __iomem *get_iomem(struct platform_device *pdev, const char *name)
{
	return get_iomem_with_range(pdev, name, NULL, NULL);
}

static int contexthub_dt_barac(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	int reg_cnt;
	int barac_cnt_idx, barac_val_idx;
	int barac_line_idx = 0;
	int barac_sizes[BARAC_MAX];
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	reg_cnt = of_property_count_strings(node, "barac,names");

	if (reg_cnt < 0) {
		nanohub_dev_err(chub->dev, "%s : barac count has wrong initial value : %d\n",
				__func__, reg_cnt);
		nanohub_dev_err(chub->dev, "%s : should not set anything\n", __func__);
		return -ENODEV;
	}

	of_property_read_u32_array(node, "barac,sizes", barac_sizes, reg_cnt);

	for (barac_cnt_idx = 0, barac_line_idx =0 ; barac_cnt_idx < reg_cnt ; barac_cnt_idx++) {
		of_property_read_string_index(node, "barac,names", barac_cnt_idx, &chub->iomem.chub_baracs[barac_cnt_idx].name);
		chub->iomem.chub_baracs[barac_cnt_idx].size = barac_sizes[barac_cnt_idx];
		chub->iomem.chub_baracs[barac_cnt_idx].addr = get_iomem(pdev, chub->iomem.chub_baracs[barac_cnt_idx].name);
		for (barac_val_idx = 0; barac_val_idx < barac_sizes[barac_cnt_idx] * 5 ; barac_val_idx++) {
			if (of_property_read_u32_index
					(node, "barac,values", barac_line_idx++,
					 &chub->iomem.chub_baracs[barac_cnt_idx].values[barac_val_idx]))
				break;
			nanohub_dev_debug(chub->dev, "%s: %s [%d]0x%x\n", __func__,
					chub->iomem.chub_baracs[barac_cnt_idx].name, barac_val_idx,
					chub->iomem.chub_baracs[barac_cnt_idx].values[barac_val_idx]);
		}
	}

	return 0;
}

static void contexthub_dt_multios(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	const char *tmp;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	tmp = of_get_property(node, "multi-os", NULL);
	if (tmp && !strcmp(tmp, "enabled")) {
		chub->misc.multi_os = true;
	}

	nanohub_dev_debug(dev, "%s: multi-os: %d(%s)\n", __func__, chub->misc.multi_os, tmp);

	chub->misc.os_load = 1;
}

static void contexthub_dt_dfs(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	const char *dfs;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	dfs = of_get_property(node, "chub-dfs-gov", NULL);
	if (dfs) {
		nanohub_dev_debug(dev, "%s: dfs %s\n", __func__, dfs);
		chub->misc.chub_dfs_gov = !strcmp(dfs, "enabled");
	}
}

#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
static int contexthub_dt_dvfs(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	const char *dvfs;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	dvfs = of_get_property(node, "chub-dvfs", NULL);
	if (dvfs) {
		nanohub_info("%s: dvfs %s\n", __func__, dvfs);
		chub->misc.chub_dvfs = !strcmp(dvfs, "enabled"); /* true if enabled */
		if (chub->misc.chub_dvfs) {
			if (of_property_read_u32_index(node, "chub-dvfs-level", 0, &chub->misc.chub_dvfs_level[0])) {
				nanohub_dev_err(chub->dev, "%s: fail to get dvfs level0\n", __func__);
				return -ENODEV;
			}
			if (of_property_read_u32_index(node, "chub-dvfs-level", 1, &chub->misc.chub_dvfs_level[1])) {
				nanohub_dev_err(chub->dev, "%s: fail to get dvfs level1\n", __func__);
				return -ENODEV;
			}
			nanohub_info("%s: dvfs level 0:%d level 1:%d\n", __func__,
					chub->misc.chub_dvfs_level[0],
					chub->misc.chub_dvfs_level[1]);
		}
	} else {
		chub->misc.chub_dvfs = false;
	}
	return 0;
}
#endif

static void contexthub_dt_alive_mct(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	chub->misc.alive_mct = of_property_read_bool(node, "use_alive_mct");

	nanohub_dev_debug(dev, "%s: use_alive_mct enabled: %d\n", __func__, chub->misc.alive_mct);
}

static void contexthub_dt_hardfault_debug(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	const char *prop;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	prop = of_get_property(node, "hardfault-debug", NULL);
	if (prop) {
		nanohub_dev_debug(dev, "%s: hardfault-debug %s\n", __func__, prop);
		chub->misc.hardfault_debug = !strcmp(prop, "enabled");
	} else {
		chub->misc.hardfault_debug = 0;
	}
}

static int contexthub_dt_mailbox_irq(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	int ret;

	chub->irq.irq_mailbox = irq_of_parse_and_map(node, 0);
	if (chub->irq.irq_mailbox < 0) {
		nanohub_dev_err(dev, "failed to get irq\n");
		return -EINVAL;
	}
	nanohub_dev_debug(dev, "%s: chub irq mailbox %d\n", __func__, chub->irq.irq_mailbox);

#if IS_ENABLED(CONFIG_SHUB)
	ret = devm_request_threaded_irq(dev, chub->irq.irq_mailbox, NULL, contexthub_irq_handler,
			       IRQF_ONESHOT, dev_name(dev), chub);
#else
	ret = devm_request_irq(dev, chub->irq.irq_mailbox, contexthub_irq_handler,
			       IRQ_TYPE_LEVEL_HIGH, dev_name(dev), chub);
#endif

	if (ret) {
		nanohub_dev_err(dev, "failed to request irq:%d, ret:%d\n",
			chub->irq.irq_mailbox, ret);
		return ret;
	}
	irq_set_irq_wake(chub->irq.irq_mailbox, 1);

	return 0;
}

static int contexthub_dt_irq_wdt(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	chub->irq.irq_wdt = irq_of_parse_and_map(node, 1);
	if (chub->irq.irq_wdt > 0) {
		ret = devm_request_irq(dev, chub->irq.irq_wdt, contexthub_irq_wdt_handler,
					IRQ_TYPE_LEVEL_HIGH, dev_name(dev), chub);
		if (ret) {
			dev_err(dev, "failed to request: %d\n", ret);
			return ret;
		}
		chub->irq.irq_wdt_disabled = 0;
	} else {
		nanohub_dev_info(dev, "WDT IRQ is not registered\n");
	}

	return 0;
}

static int contexthub_dt_iomem(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	int reg_cnt;
	int j = 0;
	const char *reg_names[50]; /*chub_usi_array*/
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	chub->iomem.mailbox = get_iomem(pdev, "mailbox");
	if (IS_ERR(chub->iomem.mailbox))
		return PTR_ERR(chub->iomem.mailbox);

	/* get SRAM base */
	chub->iomem.sram = get_iomem_with_range(pdev, "sram", &chub->iomem.sram_phys, &chub->iomem.sram_size);
	if (IS_ERR(chub->iomem.sram))
		return PTR_ERR(chub->iomem.sram);

	/* get chub gpr base */
	chub->iomem.chub_dumpgpr = get_iomem(pdev, "dumpgpr");
	if (IS_ERR(chub->iomem.chub_dumpgpr))
		return PTR_ERR(chub->iomem.chub_dumpgpr);

	contexthub_get_qch_base(chub);

	/* get chub sfr base to dump */
	chub->iomem.chub_dump_cmu = get_iomem(pdev, "chub_dump_cmu");
	chub->iomem.chub_dump_sys = get_iomem(pdev, "chub_dump_sys");
	chub->iomem.chub_dump_wdt = get_iomem(pdev, "chub_dump_wdt");
	chub->iomem.chub_dump_timer = get_iomem(pdev, "chub_dump_timer");
	chub->iomem.chub_dump_pwm = get_iomem(pdev, "chub_dump_pwm");
	chub->iomem.chub_dump_rtc = get_iomem(pdev, "chub_dump_rtc");

	/* get usi_array to dump */
	chub->iomem.usi_cnt = 0;
	reg_cnt = of_property_count_strings(node, "reg-names");

	if (reg_cnt < 0) {
		nanohub_dev_err(&pdev->dev, "%s : usi has wrong initial value. : %d\n",
				__func__, reg_cnt);
		return -EINVAL;
	}

	of_property_read_string_array(node, "reg-names", reg_names, reg_cnt);
	for (j = 0; j < reg_cnt; j++) {
		if (strstr(reg_names[j], "usi"))
			nanohub_dev_debug(&pdev->dev,
				 "%s: usi reg name %s\n",
				 __func__, reg_names[j]);
	}
	for (j = 0; j < reg_cnt; j++) {
		if (strstr(reg_names[j], "usi")) {
			if (chub->iomem.usi_cnt < MAX_USI_CNT) {
				chub->iomem.usi_array[chub->iomem.usi_cnt] = get_iomem(pdev, reg_names[j]);
				if (IS_ERR(chub->iomem.usi_array[chub->iomem.usi_cnt]))
					return PTR_ERR(chub->iomem.usi_array[chub->iomem.usi_cnt]);
				chub->iomem.usi_cnt++;
			} else {
				dev_notice(&pdev->dev,
					   "Number of USI regs > MAX Sitting Value(15)\n");
				break;
			}
		}
	}

	chub->iomem.upmu = get_iomem(pdev, "upmu");
	chub->iomem.chub_out = get_iomem(pdev, "chub_out");

	return 0;
}
static int contexthub_dt_baaw(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	int ret;
	int reg_cnt;
	//struct chub_baaw chub_baaws[BAAW_MAX];
	int baaw_cnt_idx, baaw_val_idx;
	int baaw_line_idx = 0;
	int baaw_sizes[BAAW_MAX];
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	reg_cnt = of_property_count_strings(node, "baaw,names");

	if (reg_cnt < 0) {
		nanohub_dev_err(chub->dev, "%s : baaw count has wrong initial value : %d\n",
				__func__, reg_cnt);
		nanohub_dev_err(chub->dev, "%s : should not set anything\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32_array(node, "baaw,sizes", baaw_sizes, reg_cnt);

	for (baaw_cnt_idx = 0, baaw_line_idx =0 ; baaw_cnt_idx < reg_cnt ; baaw_cnt_idx++) {
		of_property_read_string_index(node, "baaw,names", baaw_cnt_idx, &chub->iomem.chub_baaws[baaw_cnt_idx].name);
		chub->iomem.chub_baaws[baaw_cnt_idx].size = baaw_sizes[baaw_cnt_idx];
		chub->iomem.chub_baaws[baaw_cnt_idx].addr = get_iomem(pdev, chub->iomem.chub_baaws[baaw_cnt_idx].name);
		for (baaw_val_idx = 0; baaw_val_idx < baaw_sizes[baaw_cnt_idx] * 5 ; baaw_val_idx++) {
			if (of_property_read_u32_index
					(node, "baaw,values", baaw_line_idx++,
					 &chub->iomem.chub_baaws[baaw_cnt_idx].values[baaw_val_idx]))
				break;
			nanohub_dev_debug(chub->dev, "%s: %s [%d]0x%x\n", __func__,
					chub->iomem.chub_baaws[baaw_cnt_idx].name, baaw_val_idx,
					chub->iomem.chub_baaws[baaw_cnt_idx].values[baaw_val_idx]);
		}
	}

	return 0;
}

static void contexthub_dt_cpu_config(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{

#ifdef CHUB_CPU_CONFIGURATION
	struct device *dev = &pdev->dev;

	chub->iomem.pmu_chub_cpu = devm_ioremap(dev, CHUB_CPU_CONFIGURATION, 0x10);
	if (IS_ERR_OR_NULL(chub->iomem.pmu_chub_cpu))
		nanohub_dev_debug(dev, "%s: chub core reset not supported\n", __func__);
#endif
}

static int contexthub_dt_disable_irq(struct platform_device *pdev,
		struct contexthub_ipc_info *chub)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	int i;

	/* disable chub irq list (for sensor irq) */
	of_property_read_u32(node, "chub-irq-pin-len", &chub->irq.irq_pin_len);
	if (chub->irq.irq_pin_len) {
		if (chub->irq.irq_pin_len > ARRAY_SIZE(chub->irq.irq_pins)) {
			dev_err(&pdev->dev, "failed to get irq pin length %d, %ld\n",
				chub->irq.irq_pin_len, sizeof(chub->irq.irq_pins));
			chub->irq.irq_pin_len = 0;
			return -EINVAL;
		}

		nanohub_dev_debug(&pdev->dev, "get chub irq_pin len:%d\n", chub->irq.irq_pin_len);
		for (i = 0; i < chub->irq.irq_pin_len; i++) {
			chub->irq.irq_pins[i] = of_get_named_gpio(node, "chub-irq-pin", i);
			if (!gpio_is_valid(chub->irq.irq_pins[i])) {
				nanohub_dev_err(&pdev->dev, "get invalid chub irq_pin:%d\n",
					chub->irq.irq_pins[i]);
				return -EINVAL;
			}
		}
		contexthub_disable_pin(chub);
	}

	return 0;
}

static int contexthub_dt_address_remapper(struct platform_device *pdev,
					 struct contexthub_ipc_info *chub)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	chub->iomem.is_baaw = !of_property_read_bool(node, "chub-no-baaw");

	return chub->iomem.is_baaw ?
		contexthub_dt_baaw(pdev, chub) : contexthub_dt_barac(pdev, chub);
}

static int contexthub_dt_init(struct platform_device *pdev,
					 struct contexthub_ipc_info *chub)
{
	int ret;

	contexthub_dt_multios(pdev, chub);
	contexthub_dt_dfs(pdev, chub);
	contexthub_dt_alive_mct(pdev, chub);
	contexthub_dt_hardfault_debug(pdev, chub);

#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
	ret = contexthub_dt_dvfs(pdev, chub);
	if (ret)
		return ret;
#endif

	ret = contexthub_dt_mailbox_irq(pdev, chub);
	if (ret)
		return ret;

	ret = contexthub_dt_irq_wdt(pdev, chub);
	if (ret)
		return ret;

	ret = contexthub_dt_iomem(pdev, chub);
	if (ret)
		return ret;

	ret = contexthub_dt_address_remapper(pdev, chub);
	if (ret)
		return ret;

	contexthub_dt_cpu_config(pdev, chub);

	ret = contexthub_dt_disable_irq(pdev, chub);
	if (ret)
		return ret;

	contexthub_set_clk(chub);

	return 0;
}

#if IS_ENABLED(CONFIG_EXYNOS_AONCAM)
static int contexthub_aon_notifier(struct notifier_block *nb,
				   unsigned long action, void *nb_data)
{
	struct contexthub_ipc_info *chub = container_of(nb, struct contexthub_ipc_info, lpd_nb);
	int timeout;

	nanohub_info("%s: action[%lu]\n", __func__, action);
	if (chub) {
		switch (action) {
			case AON_NOTIFIER_START:
				contexthub_ipc_write_event(chub, MAILBOX_EVT_START_AON);
				chub->aon_flag = AON_WAIT_ACK;
				timeout = wait_event_timeout(chub->aon_wait
							     chub->aon_flag == AON_START_ACK,
							     msecs_to_jiffies(AON_WAIT_TIME));
				nanohub_info("%s: AON start ack timeout %d\n", __func__, timeout);
				return timeout > 0 ? NOTIFY_OK : NOTIFY_DONE;
				break;
			case AON_NOTIFIER_STOP:
				contexthub_ipc_write_event(chub, MAILBOX_EVT_STOP_AON);
				chub->lpd_flag = AON_WAIT_ACK;
				timeout = wait_event_timeout(chub->aon_wait,
							     chub->aon_flag == AON_STOP_ACK,
							     msecs_to_jiffies(AON_WAIT_TIME));
				nanohub_info("%s: AON stop ack timeout %d\n", __func__, timeout);
				return timeout > 0 ? NOTIFY_OK : NOTIFY_DONE;
				break;
			default:
				nanohub_err("%s: not defined\n", __func__);
				return NOTIFY_BAD;
				break;
		}
	} else
		nanohub_err("%s: chub not available!\n", __func__);

	return NOTIFY_BAD;
}
#endif

#if IS_ENABLED(CONFIG_EXYNOS_ITMON) || IS_ENABLED(CONFIG_EXYNOS_ITMON_V2)
static int contexthub_itmon_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct contexthub_ipc_info *data = container_of(nb, struct contexthub_ipc_info, ext.itmon_nb);
	struct itmon_notifier *itmon_data = nb_data;

	if (itmon_data && itmon_data->master &&
	   (strstr(itmon_data->master, "CHUB") || strstr(itmon_data->master, "SHUB"))) {
		nanohub_dev_info(data->dev, "%s: chub(%s) itmon detected: action:%d!!\n",
			__func__, itmon_data->master, action);

		if (atomic_read(&data->atomic.in_reset)) {
			nanohub_dev_info(data->dev,
				"%s: chub(%s) itmon occurs during reset processing: skip job.\n",
				__func__, itmon_data->master);
			return ITMON_SKIP_MASK;
		}

		if (atomic_read(&data->atomic.chub_status) == CHUB_ST_RUN) {
#if IS_ENABLED(CONFIG_SHUB)
			contexthub_notifier_call(data, CHUB_FW_ST_OFF);
#endif
			atomic_set(&data->atomic.chub_status, CHUB_ST_ITMON);
			chub_dbg_dump_hw(data, CHUB_ERR_ITMON);
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
			if (!data->misc.itmon_dumped) {
				if (data->log.mlog.memlog_sram_chub) {
					memlog_do_dump(data->log.mlog.memlog_sram_chub, MEMLOG_LEVEL_EMERG);
					data->misc.itmon_dumped = true;
				} else
					nanohub_dev_err(data->dev, "%s: memlog_sram_chub is NULL\n", __func__);
			}
#endif
			contexthub_shutdown(data);
			contexthub_handle_debug(data, CHUB_ERR_ITMON);
		}
		return ITMON_SKIP_MASK;
	}
	return NOTIFY_OK;
}
#endif

static int contexthub_panic_handler(struct notifier_block *nb,
				    unsigned long action, void *data)
{
	struct contexthub_ipc_info *chub = container_of(nb, struct contexthub_ipc_info, ext.panic_nb);

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER) {
		nanohub_info("%s : chub is in no power state, there is nothing to do.\n", __func__);
		return NOTIFY_OK;
	}

	contexthub_request_chub_flush_cache(chub);
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
	if (!chub->misc.itmon_dumped) {
		if (chub->log.mlog.memlog_sram_chub)
			memlog_do_dump(chub->log.mlog.memlog_sram_chub, MEMLOG_LEVEL_EMERG);
		else
			nanohub_err("%s: memlog_sram_chub is NULL\n", __func__);
	}
#endif
	nanohub_info("%s : dump chub sram to dram in panic handler\n", __func__);
	chub_dbg_dump_ram(chub, CHUB_ERR_KERNEL_PANIC);

	return NOTIFY_OK;
}

#if IS_ENABLED(CONFIG_EXYNOS_SYSTEM_EVENT)
static int contexthub_sysevent_shutdown(const struct sysevent_desc *desc, bool force_stop)
{
	(void) desc;
	nanohub_info("%s [%d]\n", __func__, force_stop);

	return 0;
}

static int contexthub_sysevent_powerup(const struct sysevent_desc *desc)
{
	(void) desc;
	nanohub_info("%s\n", __func__);
	return 0;
}

static int contexthub_sysevent_ramdump(int tmp, const struct sysevent_desc *desc)
{
	(void) desc;
	(void) tmp;
	nanohub_info("%s\n", __func__);
	return 0;
}

static void contexthub_sysevent_crash_shutdown(const struct sysevent_desc *desc)
{
	(void) desc;
	nanohub_info("%s\n", __func__);
}
#endif
/* CIPC Notification */
static int contexthub_notifier(struct contexthub_notifier_block *nb)
{
	nanohub_info("%s called!: subsys:%s, start_off:%x, end_off:%x",
			 __func__, nb->subsystem, nb->start_off, nb->end_off);
	return 0;
}

int contexthub_memlog_init(struct contexthub_ipc_info *chub)
{
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
	int ret;

	ret = memlog_register("CHB", chub->dev, &chub->log.mlog.memlog_chub);
	if (ret) {
		dev_err(chub->dev, "memlog chub desc registration fail ret:%d\n", ret);
		return ret;
	}

	/* error handling */
	chub->log.mlog.memlog_printf_file_chub = memlog_alloc_file(chub->log.mlog.memlog_chub,
							       "log-fil", SZ_512K, SZ_1M, 1000, 3);
	dev_info(chub->dev, "memlog printf file chub %s\n",
		 chub->log.mlog.memlog_printf_file_chub ? "pass" : "fail");
	if (chub->log.mlog.memlog_printf_file_chub) {
		memlog_obj_set_sysfs_mode(chub->log.mlog.memlog_printf_file_chub, true);
		/* error handling */
		chub->log.mlog.memlog_printf_chub =
		    memlog_alloc_printf(chub->log.mlog.memlog_chub, SZ_512K,
					chub->log.mlog.memlog_printf_file_chub, "log-mem", 0);
	}
	dev_info(chub->dev, "memlog printf chub %s\n",
		 chub->log.mlog.memlog_printf_chub ? "pass" : "fail");

	/* error handling */
	chub->log.mlog.memlog_sram_file_chub =
		memlog_alloc_file(chub->log.mlog.memlog_chub,
				  "srm-fil", chub->iomem.sram_size, chub->iomem.sram_size, 1000, 3);
	if (chub->log.mlog.memlog_sram_file_chub)
		/* error handling */
		chub->log.mlog.memlog_sram_chub =
		    memlog_alloc_dump(chub->log.mlog.memlog_chub, chub->iomem.sram_size, chub->iomem.sram_phys, false,
				      chub->log.mlog.memlog_sram_file_chub, "srm-dmp");
	dev_info(chub->dev, "memlog dump chub %s\n",
		 chub->log.mlog.memlog_sram_chub ? "pass" : "fail");
#endif
	return 0;
}

int contexthub_sync_memlogger(void *chub_p)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
	struct contexthub_ipc_info *chub = chub_p;

	if (chub->log.mlog.memlog_printf_chub)
		ret |= memlog_sync_to_file(chub->log.mlog.memlog_printf_chub);
#endif
	return ret;
}

static struct contexthub_notifi_info cipc_noti[IPC_OWN_MAX] = {
	{"EMP", 0, NULL},
	{"CHUB", IPC_OWN_CHUB, NULL},
	{"AP", IPC_OWN_AP, NULL},
	{"GNSS", IPC_OWN_GNSS, NULL},
	{"ABOX", IPC_OWN_ABOX, NULL},
	{"VTS", IPC_OWN_VTS, NULL},
};

int contexthub_notifier_call(struct contexthub_ipc_info *chub,
			     enum CHUB_STATE state)
{
	int ret = 0;
	int i;
	u32 cipc_start_offset = 0;
	u32 cipc_size = 0;
	u32 ipc_base_offset = 0;
	u32 cipc_base_offset = 0;
	bool find = 0;

	nanohub_dev_info(chub->dev, "%s enters: state:%d\n", __func__, state);
	if (state == CHUB_FW_ST_OFF || state == CHUB_FW_ST_ON) {
		for (i = IPC_OWN_HOST; i < IPC_OWN_MAX; i++) {
			/* Check Notifier call pointer exist */
			if (!cipc_noti[i].nb)
				continue;

			if (cipc_noti[i].nb->state != CHUB_FW_ST_INVAL) {
				nanohub_dev_info(chub->dev,
						 "%s (%d:%s) call notifier call on chub-reset\n",
						 __func__, i, cipc_noti[i].name);
				cipc_noti[i].nb->state = state;
				ret = cipc_noti[i].nb->notifier_call(cipc_noti[i].nb);
				if (ret)
					nanohub_dev_info(chub->dev,
							 "%s (%d:%s) fails to notifier ret[%x] --\n",
							 __func__, i, cipc_noti[i].name, ret);
			}
		}
		return 0;
	}

	/* find the cipc base of sussystem */
	for (i = IPC_OWN_HOST; i < IPC_OWN_MAX; i++) {
		nanohub_dev_info(chub->dev,
				 "%s (%d:%s) start to find start_off\n",
				 __func__, i, cipc_noti[i].name);
		if ((i == IPC_OWN_MASTER) || (i == IPC_OWN_HOST)) {
			nanohub_dev_info(chub->dev,
					 "%s (%d:%s) skip by master & host\n",
					 __func__, i, cipc_noti[i].name);
			continue;
		}
		if (!cipc_noti[i].nb) {
			nanohub_dev_info(chub->dev,
					 "%s (%d:%s) skip by no notifier\n",
					 __func__, i, cipc_noti[i].name);
			continue;
		}

		cipc_get_offset_owner(i, &cipc_start_offset, &cipc_size);
		if (cipc_start_offset) {
			nanohub_dev_info(chub->dev,
					 "%s (%d:%s) get start_off:+%x, size:%d\n",
					 __func__, i, cipc_noti[i].name,
					 cipc_start_offset, cipc_size);

			cipc_base_offset = cipc_get_base(CIPC_REG_CIPC_BASE) - chub->iomem.sram;
			ipc_base_offset = ipc_get_base(IPC_REG_IPC) - chub->iomem.sram;
			/* offset check: offset should be 4KB align. And it's bigger than cipc_base. and it's smaller than ipc_end */
			if (((cipc_start_offset % IPC_ALIGN_SIZE) == 0) &&
			    ((cipc_size % IPC_ALIGN_SIZE) == 0) &&
			    (cipc_base_offset <= cipc_start_offset) &&
			    ((cipc_start_offset + cipc_size) <=
			    (ipc_base_offset + ipc_get_size(IPC_REG_IPC)))) {
				cipc_noti[i].nb->start_off = cipc_start_offset;
				cipc_noti[i].nb->end_off = cipc_start_offset + cipc_size;
				nanohub_dev_info(chub->dev,
						 "%s (%d:%s) fill notifier:start_off:+%x, end_off:+%x\n",
						 __func__, i, cipc_noti[i].name,
						 cipc_noti[i].nb->start_off,
						 cipc_noti[i].nb->end_off);
				find = 1;
			} else {
				nanohub_dev_err(chub->dev,
						"%s (%d:%s) invalid start_off:+%x(align:%d), end_off:+%x(align:%d), cipc_base_off:+%x, ipc_end_off:+%x\n",
						__func__, i, cipc_noti[i].name,
						cipc_start_offset, cipc_start_offset % IPC_ALIGN_SIZE,
						cipc_start_offset + cipc_size,
						(cipc_start_offset + cipc_size) % IPC_ALIGN_SIZE,
						cipc_base_offset, ipc_base_offset + ipc_get_size((IPC_REG_IPC)));
			}
		}
	}

	/* call cipc notifiers */
	if (find) {
		for (i = IPC_OWN_HOST; i < IPC_OWN_MAX; i++) {
			/* Check Notifier call pointer exist */
			if (!cipc_noti[i].nb) {
				nanohub_dev_info(chub->dev,
						 "%s (%d:%s) doesn't have notifier call\n",
						 __func__, i, cipc_noti[i].name);
				continue;
			}
			nanohub_dev_info(chub->dev,
					 "%s (%d:%s) start_off:+%x, end_off:+%x\n",
					 __func__, i, cipc_noti[i].name,
					 cipc_noti[i].nb->start_off,
					 cipc_noti[i].nb->end_off);
			if (cipc_noti[i].nb->start_off) {
				cipc_noti[i].nb->state = state;
				ret = cipc_noti[i].nb->notifier_call(cipc_noti[i].nb);
				if (ret)
					nanohub_dev_info(chub->dev,
							 "%s (%d:%s) fails to notifier ret[%x] --\n",
							 __func__, i,
							 cipc_noti[i].name,
							 ret);
			}
		}
	}
	return ret;
}

int contexthub_notifier_register(struct contexthub_notifier_block *nb)
{
	int index;

	if (!nb) {
		pr_err("%s: subsystem notifier block is NULL pointer\n",
		       __func__);
		return -EINVAL;
	}
	if (!nb->subsystem || !nb->notifier_call) {
		pr_err("%s: subsystem is NULL pointer\n", __func__);
		return -EINVAL;
	}

	for (index = IPC_OWN_HOST; index < IPC_OWN_MAX; index++) {
		if (!strncmp
		    (cipc_noti[index].name, nb->subsystem, IPC_NAME_MAX))
			break;
	}

	if (index >= IPC_OWN_MAX) {
		pr_err("%s: can't find subsystem:%s\n",
		       __func__, nb->subsystem);
		return -EINVAL;
	}

	nb->start_off = 0;
	nb->end_off = 0;
	cipc_noti[index].nb = nb;
	pr_info("%s: (%s) register successful!\n", __func__, nb->subsystem);
	return 0;
}
EXPORT_SYMBOL(contexthub_notifier_register);

static int contexthub_ipc_probe(struct platform_device *pdev)
{
	struct contexthub_ipc_info *chub;
	int ret = 0;

	chub_dbg_get_memory(pdev->dev.of_node);

	chub = devm_kzalloc(&pdev->dev, sizeof(*chub), GFP_KERNEL);
	if (!chub)
		return -ENOMEM;

	chub->dev = &pdev->dev;
	platform_set_drvdata(pdev, chub);
	atomic_set(&chub->atomic.reset_ctrl, 1);

	ret = contexthub_log_init(chub);
	if (ret) {
		dev_err(&pdev->dev, "log init is fail with ret %d\n", ret);
		goto err_before_pmqos;
	}

	/* parse dt and hw init */
	ret = contexthub_dt_init(pdev, chub);
	if (ret)
		goto err_before_pmqos;

	chub_dbg_register_dump_to_dss(chub->iomem.sram_phys, chub->iomem.sram_size);
	ret = contexthub_memlog_init(chub);
	if (ret) {
		dev_err(chub->dev, "memlog not registered...\n");
		goto err_before_pmqos;
	}

	ret = contexthub_blk_poweron(chub);
	if (ret) {
		dev_err(&pdev->dev, "block poweron failed\n");
		goto err_before_pmqos;
	}
	else {
		dev_info(&pdev->dev, "block poweron success\n");
	}

#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
	if (chub->misc.chub_dvfs) {
		exynos_pm_qos_add_request(&chub->misc.pm_qos_lock, PM_QOS_CHUB_THROUGHPUT, 0);
		exynos_pm_qos_update_request(&chub->misc.pm_qos_lock, chub->misc.chub_dvfs_level[0]);
	}
#endif

	spin_lock_init(&chub->logout_lock);
	atomic_set(&chub->atomic.chub_status, CHUB_ST_NO_POWER);
#if IS_ENABLED(CONFIG_EXYNOS_ITMON) || IS_ENABLED(CONFIG_EXYNOS_ITMON_V2)
	/* itmon notifier */
	chub->ext.itmon_nb.notifier_call = contexthub_itmon_notifier;
	itmon_notifier_chain_register(&chub->ext.itmon_nb);
#endif
	/* panic notifier */
	chub->ext.panic_nb.notifier_call = contexthub_panic_handler;
	atomic_notifier_chain_register(&panic_notifier_list, &chub->ext.panic_nb);

	/* chub power & reset notifier */
	chub->ext.chub_cipc_nb.subsystem = "CHUB";
	chub->ext.chub_cipc_nb.notifier_call = contexthub_notifier;
	contexthub_notifier_register(&chub->ext.chub_cipc_nb);

#if defined(CONFIG_EXYNOS_AONCAM)
	chub->ext.aon_nb.notifier_call = aon_notifier_chub;
	aon_notifier_register(&chub->ext.aon_nb);
	init_waitqueue_head(&chub->ext.aon_wait);
#endif
#if IS_ENABLED(CONFIG_EXYNOS_SYSTEM_EVENT)
	/* sysevent register */
	chub->ext.sysevent_desc.name = "CHB";
	strcpy(chub->ext.sysevent_desc.fw_name, chub->misc.multi_os ? OS_IMAGE_MULTIOS : OS_IMAGE_DEFAULT);

	chub->ext.sysevent_desc.owner = THIS_MODULE;
	chub->ext.sysevent_desc.shutdown = contexthub_sysevent_shutdown;
	chub->ext.sysevent_desc.powerup = contexthub_sysevent_powerup;
	chub->ext.sysevent_desc.ramdump = contexthub_sysevent_ramdump;
	chub->ext.sysevent_desc.crash_shutdown = contexthub_sysevent_crash_shutdown;
	chub->ext.sysevent_desc.dev = &pdev->dev;
	chub->ext.sysevent_dev = sysevent_register(&chub->ext.sysevent_desc);
	chub->ext.sysevent_status = NULL;

	if (IS_ERR(chub->ext.sysevent_dev)) {
		ret = PTR_ERR(chub->ext.sysevent_dev);
		dev_err(&pdev->dev, "failed to register sysevent:%d\n", ret);
		goto err_after_pmqos;
	}
#endif

	ret = contexthub_ipc_if_init(chub);
	if (ret) {
		dev_err(&pdev->dev, "ipc_if init is fail with ret %d\n", ret);
		goto err_after_pmqos;
	}

	ret = contexthub_bootup_init(chub);
	if (ret) {
		dev_err(&pdev->dev, "bootup init is fail with ret %d\n", ret);
		goto err_after_pmqos;
	}

	ret = chub_dbg_create_attribute(chub);
	if (ret) {
		dev_err(&pdev->dev, "failed to create attributes. ret %d\n", ret);
		goto err_after_pmqos;
	}

	nanohub_dev_info(&pdev->dev, "Probe done. sensor_drv:%s\n", chub->data ? "on" : "off");

#if IS_ENABLED(CONFIG_SHUB)
	set_contexthub_info(chub);
#endif

	return 0;
err_after_pmqos:
#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
	if (chub->misc.chub_dvfs)
		exynos_pm_qos_remove_request(&chub->misc.pm_qos_lock);
#endif
err_before_pmqos:
	devm_kfree(&pdev->dev, chub);

	return ret;
}

static int contexthub_ipc_remove(struct platform_device *pdev)
{
	struct contexthub_ipc_info *chub = platform_get_drvdata(pdev);

	kthread_stop(chub->log.log_kthread);
#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
	if (chub->misc.chub_dvfs)
		exynos_pm_qos_remove_request(&chub->misc.pm_qos_lock);
#endif
	chub_wake_lock_destroy(chub->ws_reset);
	return 0;
}

static int contexthub_alive_noirq(struct contexthub_ipc_info *ipc)
{
	int cnt = 100;
	int start_index;
	unsigned int status;
	int irq_num;
	struct cipc_info *cipc;
	enum ipc_mb_id rcv_mb_id;
	void *base;

	cipc = ipc->chub_ipc->cipc;
	rcv_mb_id = cipc->user_info[CIPC_USER_CHUB2AP].map_info.dst.mb_id;
	status = ipc_hw_read_int_status_reg_all(ipc->iomem.mailbox, rcv_mb_id);
	start_index = ipc_hw_start_bit(rcv_mb_id);
	irq_num = IRQ_NUM_CHUB_ALIVE + start_index;
	base = cipc->user_info[CIPC_USER_CHUB2AP].mb_base;

	atomic_set(&ipc->event.chub_alive_lock.flag, 0);
	while(cnt--) {
		mdelay(1);
		status = ipc_hw_read_int_status_reg_all(base, rcv_mb_id);
		if (status & (1 << irq_num)) {
			if (ipc_hw_read_shared_reg(base, SR_3) == 0xFFFFFFFF) {
				nanohub_dev_info(ipc->dev, "chub crash\n");
				break;
			}

			ipc_hw_clear_int_pend_reg(base, rcv_mb_id, irq_num);
			ipc_hw_mask_clear_int_reg(base, rcv_mb_id, IRQ_NUM_CHUB_ALIVE);
			atomic_set(&ipc->event.chub_alive_lock.flag, 1);
			nanohub_dev_info(ipc->dev, "chub is alive\n");
			return 0;
		}
	}
	ipc_hw_mask_clear_int_reg(base, rcv_mb_id, IRQ_NUM_CHUB_ALIVE);
	nanohub_dev_info(ipc->dev, "chub is not alive\n");
	return -1;
}

static int contexthub_resume_noirq(struct device *dev)
{
	struct contexthub_ipc_info *ipc = dev_get_drvdata(dev);
	struct cipc_info *cipc;
	enum ipc_mb_id rcv_mb_id;
	void *base;

	if (atomic_read(&ipc->atomic.chub_status) != CHUB_ST_RUN) {
		nanohub_dev_info(dev, "%s : F/W status(%d) is not run. skip next action.\n",
				__func__, atomic_read(&ipc->atomic.chub_status));
		return 0;
	}

	cipc = ipc->chub_ipc->cipc;
	rcv_mb_id = cipc->user_info[CIPC_USER_CHUB2AP].map_info.dst.mb_id;
	base = cipc->user_info[CIPC_USER_CHUB2AP].mb_base;

#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
	if (ipc->misc.chub_dvfs)
		exynos_pm_qos_update_request(&ipc->misc.pm_qos_lock, ipc->misc.chub_dvfs_level[0]);
#endif
	disable_irq_wake(ipc->irq.irq_mailbox);
	ipc_write_hw_value(IPC_VAL_HW_AP_STATUS, AP_WAKE);
	ipc_hw_mask_set_int_reg(base, rcv_mb_id, IRQ_NUM_CHUB_ALIVE);
	ipc_hw_gen_interrupt(ipc->iomem.mailbox, ipc->chub_ipc->opp_mb_id, IRQ_NUM_CHUB_ALIVE);

	contexthub_alive_noirq(ipc);

	atomic_set(&ipc->atomic.chub_sleep, 0);
	nanohub_dev_info(dev, "%s\n", __func__);
	return 0;
}

static int contexthub_prepare(struct device *dev)
{
	struct contexthub_ipc_info *ipc = dev_get_drvdata(dev);

	if (atomic_read(&ipc->atomic.chub_status) != CHUB_ST_RUN) {
		nanohub_dev_info(dev, "%s : F/W status(%d) is not run. skip next action.\n",
				__func__, atomic_read(&ipc->atomic.chub_status));
		return 0;
	}

	ipc_write_hw_value(IPC_VAL_HW_AP_STATUS, AP_PREPARE);
	ipc_hw_gen_interrupt(ipc->iomem.mailbox, ipc->chub_ipc->opp_mb_id, IRQ_NUM_CHUB_ALIVE);

	nanohub_dev_info(dev, "%s: out\n", __func__);

	return 0;
}

static int contexthub_suspend_noirq(struct device *dev)
{
	struct contexthub_ipc_info *ipc = dev_get_drvdata(dev);

	if (atomic_read(&ipc->atomic.chub_status) != CHUB_ST_RUN) {
		nanohub_dev_info(dev, "%s : F/W status(%d) is not run. skip next action.\n",
				__func__, atomic_read(&ipc->atomic.chub_status));
		return 0;
	}

	enable_irq_wake(ipc->irq.irq_mailbox);

	nanohub_dev_info(dev, "%s: send ap sleep\n", __func__);

	atomic_set(&ipc->atomic.chub_sleep, 1);

	ipc_write_hw_value(IPC_VAL_HW_AP_STATUS, AP_SLEEP);
	ipc_hw_gen_interrupt(ipc->iomem.mailbox, ipc->chub_ipc->opp_mb_id, IRQ_NUM_CHUB_ALIVE);
#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
	if (ipc->misc.chub_dvfs)
		exynos_pm_qos_update_request(&ipc->misc.pm_qos_lock, ipc->misc.chub_dvfs_level[1]);
#endif

	return 0;
}

static int contexthub_suspend(struct device *dev)
{
	struct contexthub_ipc_info *chub = dev_get_drvdata(dev);
	struct ipc_info *chub_ipc = chub->chub_ipc;

	if (atomic_read(&chub->atomic.chub_status) != CHUB_ST_RUN) {
		nanohub_dev_info(dev, "%s : F/W status(%d) is not run. skip next action.\n",
				__func__, atomic_read(&chub->atomic.chub_status));
		return 0;
	}

#ifdef CONFIG_CONTEXTHUB_SENSOR_DEBUG
#ifdef ALIVE_WORK
	cancel_delayed_work(&chub->sensor_alive_work);
	chub->sensor_alive_work_run = false;
#endif
#endif
	nanohub_dev_info(dev, "%s: irq-pend:ap:%x,chub:%x\n", __func__,
		ipc_hw_read_int_status_reg_all(chub->iomem.mailbox, chub_ipc->my_mb_id),
		ipc_hw_read_int_status_reg_all(chub->iomem.mailbox, chub_ipc->opp_mb_id));

	if (atomic_read(&chub->atomic.in_log)) {
		nanohub_dev_info(dev, "%s: waiting log print done...\n", __func__);
		chub_wait_event(&chub->event.log_lock, 100);
	}

	nanohub_dev_info(dev, "%s: out\n", __func__);

	return 0;
}

static int contexthub_resume(struct device *dev)
{
	struct contexthub_ipc_info *chub = dev_get_drvdata(dev);
	struct ipc_info *chub_ipc = chub->chub_ipc;

	if (atomic_read(&chub->atomic.chub_status) != CHUB_ST_RUN) {
		nanohub_dev_info(dev, "%s : F/W status(%d) is not run. skip next action.\n",
				__func__, atomic_read(&chub->atomic.chub_status));
		return 0;
	}
#ifdef CONFIG_CONTEXTHUB_SENSOR_DEBUG
#ifdef ALIVE_WORK
	if (chub->sensor_alive_work_run == false) {
		dev_info(dev, "%s: schedule sensor_alive_work\n", __func__);
		schedule_delayed_work(&chub->sensor_alive_work, msecs_to_jiffies(1000));
	}
#endif
#endif
	nanohub_dev_info(dev, "%s: irq-pend:ap:%x,chub:%x\n", __func__,
		ipc_hw_read_int_status_reg_all(chub->iomem.mailbox, chub_ipc->my_mb_id),
		ipc_hw_read_int_status_reg_all(chub->iomem.mailbox, chub_ipc->opp_mb_id));
	return 0;
}

static void contexthub_complete(struct device *dev)
{
	struct contexthub_ipc_info *ipc = dev_get_drvdata(dev);

	if (atomic_read(&ipc->atomic.chub_status) != CHUB_ST_RUN) {
		nanohub_dev_info(dev, "%s : F/W status(%d) is not run. skip next action.\n",
				__func__, atomic_read(&ipc->atomic.chub_status));
		return;
	}

	ipc_write_hw_value(IPC_VAL_HW_AP_STATUS, AP_COMPLETE);
	ipc_hw_gen_interrupt(ipc->iomem.mailbox, ipc->chub_ipc->opp_mb_id, IRQ_NUM_CHUB_ALIVE);

	nanohub_dev_info(dev, "%s: out\n", __func__);
}

static const struct dev_pm_ops contexthub_pm_ops = {
	.prepare = contexthub_prepare,
	.suspend = contexthub_suspend,
	.suspend_noirq = contexthub_suspend_noirq,
	.resume = contexthub_resume,
	.resume_noirq = contexthub_resume_noirq,
	.complete = contexthub_complete,
};

static const struct of_device_id contexthub_ipc_match[] = {
	{.compatible = "samsung,exynos-nanohub"},
	{},
};

static struct platform_driver samsung_contexthub_ipc_driver = {
	.probe = contexthub_ipc_probe,
	.remove = contexthub_ipc_remove,
	.driver = {
		   .name = "nanohub-ipc",
		   .owner = THIS_MODULE,
		   .of_match_table = contexthub_ipc_match,
		   .pm = &contexthub_pm_ops,
	},
};

int contexthub_init(void)
{
#ifdef CONFIG_SENSOR_DRV
	nanohub_init();
#endif
	return platform_driver_register(&samsung_contexthub_ipc_driver);
}

static void __exit contexthub_cleanup(void)
{
#ifdef CONFIG_SENSOR_DRV
	nanohub_cleanup();
#endif
	platform_driver_unregister(&samsung_contexthub_ipc_driver);
}

module_init(contexthub_init);
module_exit(contexthub_cleanup);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Exynos contexthub mailbox Driver");
MODULE_AUTHOR("Boojin Kim <boojin.kim@samsung.com>");
