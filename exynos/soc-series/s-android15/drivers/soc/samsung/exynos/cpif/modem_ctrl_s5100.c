// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/pci.h>
#include <linux/regulator/consumer.h>
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
#include <soc/samsung/esca_mfd.h>
#else
#include <soc/samsung/acpm_mfd.h>
#endif
#include <linux/reboot.h>
#include <linux/suspend.h>

#include <linux/exynos-pci-ctrl.h>
#include <soc/samsung/shm_ipc.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_ctrl.h"
#include "modem_notifier.h"
#include "link_device.h"
#include "link_device_memory.h"
#include "s51xx_pcie.h"

#if IS_ENABLED(CONFIG_SUSPEND_DURING_VOICE_CALL)
#include <sound/samsung/abox.h>
#endif
#if IS_ENABLED(CONFIG_LINK_DEVICE_PCIE_S2MPU)
#include <soc/samsung/exynos-s2mpu.h>
#endif
#if IS_ENABLED(CONFIG_LINK_DEVICE_PCIE_IOMMU)
#include "link_device_pcie_iommu.h"
#endif
#if IS_ENABLED(CONFIG_CPIF_DIRECT_DM)
#include "direct_dm.h"
#endif

#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)

#define RUNTIME_PM_AFFINITY_CORE 2

static int s5100_poweroff_pcie(struct modem_ctl *mc, bool force_off);

static int s5100_reboot_handler(struct notifier_block *nb,
				    unsigned long l, void *p)
{
	struct modem_ctl *mc = container_of(nb, struct modem_ctl, reboot_nb);

	mif_info("Now is device rebooting..\n");

	mutex_lock(&mc->pcie_check_lock);
	mc->device_reboot = true;
	mutex_unlock(&mc->pcie_check_lock);

	return 0;
}

static void print_mc_state(struct modem_ctl *mc)
{
	int pwr = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_PWR], false);
	int reset = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_AP2CP_NRESET], false);
	int pshold = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_PS_HOLD], false);

	int ap_wakeup = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP], false);
	int cp_wakeup = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_AP2CP_WAKEUP], false);

	int dump = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_AP2CP_DUMP_NOTI], false);
	int ap_status = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], false);
	int phone_active = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_CP_ACTIVE], false);

	mif_info("%s: %ps:GPIO pwr:%d rst:%d phd:%d c2aw:%d a2cw:%d dmp:%d ap_act:%d cp_act:%d\n",
		 mc->name, CALLER, pwr, reset, pshold, ap_wakeup, cp_wakeup, dump,
		 ap_status, phone_active);
}

static void pcie_clean_dislink(struct modem_ctl *mc)
{
#if IS_ENABLED(CONFIG_SUSPEND_DURING_VOICE_CALL)
	if (mc->pcie_voice_call_on) {
		modem_notify_event(MODEM_EVENT_RESET, mc);
		mc->pcie_voice_call_on = false;
	} else {
		modem_notify_event(MODEM_EVENT_OFFLINE, mc);
	}
#endif

	if (mc->pcie_powered_on)
		s5100_poweroff_pcie(mc, true);

	if (!mc->pcie_powered_on)
		mif_err("Link is disconnected!!!\n");
}

static void cp2ap_wakeup_work(struct work_struct *work)
{
	struct modem_ctl *mc = container_of(work, struct modem_ctl, wakeup_work);

	if (mc->phone_state == STATE_CRASH_EXIT)
		return;

	s5100_poweron_pcie(mc, false);
}

static void cp2ap_suspend_work(struct work_struct *work)
{
	struct modem_ctl *mc = container_of(work, struct modem_ctl, suspend_work);

	if (mc->phone_state == STATE_CRASH_EXIT)
		return;

	s5100_poweroff_pcie(mc, false);
}

#if IS_ENABLED(CONFIG_SUSPEND_DURING_VOICE_CALL)
static void voice_call_on_work(struct work_struct *work)
{
	struct modem_ctl *mc = container_of(work, struct modem_ctl, call_on_work);

	mutex_lock(&mc->pcie_check_lock);
	if (!mc->pcie_voice_call_on)
		goto exit;

	if (mc->pcie_powered_on &&
			(s51xx_check_pcie_link_status(mc->pcie_ch_num) != 0)) {
		if (cpif_wake_lock_active(mc->ws))
			cpif_wake_unlock(mc->ws);
	}

	modem_voice_call_notify_event(MODEM_VOICE_CALL_ON);

exit:
	mif_info("wakelock active = %d, voice status = %d\n",
		cpif_wake_lock_active(mc->ws), mc->pcie_voice_call_on);
	mutex_unlock(&mc->pcie_check_lock);
}

static void voice_call_off_work(struct work_struct *work)
{
	struct modem_ctl *mc = container_of(work, struct modem_ctl, call_off_work);

	mutex_lock(&mc->pcie_check_lock);
	if (mc->pcie_voice_call_on)
		goto exit;

	if (mc->pcie_powered_on &&
			(s51xx_check_pcie_link_status(mc->pcie_ch_num) != 0)) {
		if (!cpif_wake_lock_active(mc->ws))
			cpif_wake_lock(mc->ws);
	}

	modem_voice_call_notify_event(MODEM_VOICE_CALL_OFF);

exit:
	mif_info("wakelock active = %d, voice status = %d\n",
		cpif_wake_lock_active(mc->ws), mc->pcie_voice_call_on);
	mutex_unlock(&mc->pcie_check_lock);
}
#endif

/* It means initial GPIO level. */
static int check_link_order = 1;
static irqreturn_t ap_wakeup_handler(int irq, void *data)
{
	struct modem_ctl *mc = (struct modem_ctl *)data;
	int gpio_val = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP], true);
	unsigned long flags;

	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);

	if (mc->device_reboot) {
		mif_err("skip : device is rebooting..!!!\n");
		return IRQ_HANDLED;
	}

	if (gpio_val == check_link_order)
		mif_err("cp2ap_wakeup val is the same with before : %d\n", gpio_val);
	check_link_order = gpio_val;

	spin_lock_irqsave(&mc->pcie_pm_lock, flags);
	if (mc->pcie_pm_suspended) {
		if (gpio_val == 1) {
			/* try to block system suspend */
			if (!cpif_wake_lock_active(mc->ws))
				cpif_wake_lock(mc->ws);
		}

		mif_err("cp2ap_wakeup work pending. gpio_val : %d\n", gpio_val);
		mc->pcie_pm_resume_wait = true;
		mc->pcie_pm_resume_gpio_val = gpio_val;

		spin_unlock_irqrestore(&mc->pcie_pm_lock, flags);
		return IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&mc->pcie_pm_lock, flags);

	mc->apwake_irq_chip->irq_set_type(
		irq_get_irq_data(mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP].num),
		(gpio_val == 1 ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH));
	mif_enable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);

	queue_work_on(RUNTIME_PM_AFFINITY_CORE, mc->wakeup_wq,
			(gpio_val == 1 ? &mc->wakeup_work : &mc->suspend_work));

	return IRQ_HANDLED;
}

static irqreturn_t cp_active_handler(int irq, void *data)
{
	struct modem_ctl *mc = (struct modem_ctl *)data;
	struct link_device *ld;
	struct mem_link_device *mld;
	int cp_active;
	enum modem_state old_state;
	enum modem_state new_state;

	if (mc == NULL) {
		mif_err_limited("modem_ctl is NOT initialized - IGNORING interrupt\n");
		goto irq_done;
	}

	ld = get_current_link(mc->iod);
	mld = to_mem_link_device(ld);

	if (mc->s51xx_pdev == NULL) {
		mif_err_limited("S5100 is NOT initialized - IGNORING interrupt\n");
		goto irq_done;
	}

	if (mc->phone_state != STATE_ONLINE) {
		mif_err_limited("Phone_state is NOT ONLINE - IGNORING interrupt\n");
		goto irq_done;
	}

	cp_active = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_CP_ACTIVE], true);
	mif_err("[PHONE_ACTIVE Handler] state:%s cp_active:%d\n",
			cp_state_str(mc->phone_state), cp_active);

	if (cp_active == 1) {
		mif_err("ERROR - cp_active is not low, state:%s cp_active:%d\n",
				cp_state_str(mc->phone_state), cp_active);
		return IRQ_HANDLED;
	}

	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);

	mif_stop_logging();

	old_state = mc->phone_state;
	new_state = STATE_CRASH_EXIT;

	if (ld->crash_reason.type == CRASH_REASON_NONE)
		ld->crash_reason.type = CRASH_REASON_CP_ACT_CRASH;

	mc->s5100_cp_reset_required = false;
	mif_info("Set s5100_cp_reset_required to %u\n", mc->s5100_cp_reset_required);

	if (old_state != new_state) {
		mif_err("new_state = %s\n", cp_state_str(new_state));

		if (old_state == STATE_ONLINE)
			modem_notify_event(MODEM_EVENT_EXIT, mc);

		change_modem_state(mc, new_state);
	}

	atomic_set(&mld->forced_cp_crash, 0);

irq_done:
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE]);

	return IRQ_HANDLED;
}

static int register_phone_active_interrupt(struct modem_ctl *mc)
{
	int ret;

	if (mc == NULL)
		return -EINVAL;

	if (mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE].registered)
		return 0;

	mif_info("Register PHONE ACTIVE interrupt.\n");
	mif_init_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE],
		     mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE].num,
		     "phone_active", IRQF_TRIGGER_LOW);

	ret = mif_request_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE], cp_active_handler, mc);
	if (ret) {
		mif_err("%s: ERR! request_irq(%s#%d) fail (%d)\n",
			mc->name, mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE].name,
			mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE].num, ret);
		return ret;
	}

	return ret;
}

static int register_cp2ap_wakeup_interrupt(struct modem_ctl *mc)
{
	int ret;

	if (mc == NULL)
		return -EINVAL;

	if (mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP].registered) {
		mif_info("Set IRQF_TRIGGER_LOW to cp2ap_wakeup gpio\n");
		check_link_order = 1;
		ret = mc->apwake_irq_chip->irq_set_type(
				irq_get_irq_data(mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP].num),
				IRQF_TRIGGER_LOW);
		return ret;
	}

	mif_info("Register CP2AP WAKEUP interrupt.\n");
	mif_init_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP],
		     mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP].num,
		     "cp2ap_wakeup", IRQF_TRIGGER_LOW);

	ret = mif_request_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP], ap_wakeup_handler, mc);
	if (ret) {
		mif_err("%s: ERR! request_irq(%s#%d) fail (%d)\n",
			mc->name, mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP].name,
			mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP].num, ret);
		return ret;
	}

	return ret;
}

static int ds_detect = 2;
module_param(ds_detect, int, 0664);
MODULE_PARM_DESC(ds_detect, "Dual SIM detect");

static ssize_t ds_detect_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", ds_detect);
}

static ssize_t ds_detect_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int value;

	ret = kstrtoint(buf, 0, &value);
	if (ret != 0) {
		mif_err("invalid value:%d with %d\n", value, ret);
		return -EINVAL;
	}

	ds_detect = value;
	mif_info("set ds_detect: %d\n", ds_detect);

	return count;
}
static DEVICE_ATTR_RW(ds_detect);

static struct attribute *sim_attrs[] = {
	&dev_attr_ds_detect.attr,
	NULL,
};

static const struct attribute_group sim_group = {
	.attrs = sim_attrs,
	.name = "sim",
};

static int get_ds_detect(void)
{
	if (ds_detect > 2 || ds_detect < 1)
		ds_detect = 2;

	mif_info("Dual SIM detect = %d\n", ds_detect);
	return ds_detect - 1;
}

static int init_control_messages(struct modem_ctl *mc)
{
	struct modem_data *modem = mc->mdm_data;
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ds_det;

	if (modem->offset_cmsg_offset)
		iowrite32(modem->cmsg_offset, mld->cmsg_offset);
	if (modem->offset_srinfo_offset)
		iowrite32(modem->srinfo_offset, mld->srinfo_offset);
	if (ld->capability_check && modem->offset_capability_offset)
		iowrite32(modem->capability_offset, mld->capability_offset);

	set_ctrl_msg(&mld->ap2cp_united_status, 0);
	set_ctrl_msg(&mld->cp2ap_united_status, 0);

	if (ld->capability_check) {
		int part;

		for (part = 0; part < AP_CP_CAP_PARTS; part++) {
			iowrite32(0, mld->ap_capability_offset[part]);
			iowrite32(0, mld->cp_capability_offset[part]);
		}
	}

	ds_det = get_ds_detect();
	if (ds_det < 0) {
		mif_err("ds_det error:%d\n", ds_det);
		return -EINVAL;
	}

	update_ctrl_msg(&mld->ap2cp_united_status, ds_det, mc->sbi_ds_det_mask,
			mc->sbi_ds_det_pos);
	mif_info("ds_det:%d\n", ds_det);

	return 0;
}

static void set_pcie_msi_int(struct link_device *ld, bool enabled)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	int irq;
	bool *irq_wake;
#if IS_ENABLED(CONFIG_CP_PKTPROC)
	struct pktproc_adaptor *ppa = &mld->pktproc;
	unsigned int q_idx = 0;
#endif

	if (!mld->msi_irq_base)
		return;

	irq = mld->msi_irq_base;
	irq_wake = &mld->msi_irq_base_wake;

	do {
		if (enabled) {
			int err;

			if (!mld->msi_irq_enabled)
				enable_irq(irq);

			if (!*irq_wake) {
				err = enable_irq_wake(irq);
				*irq_wake = !err;
			}
		} else {
			if (mld->msi_irq_enabled)
				disable_irq(irq);

			if (*irq_wake) {
				disable_irq_wake(irq);
				*irq_wake = false;
			}
		}

		irq = 0;
#if IS_ENABLED(CONFIG_CP_PKTPROC)
		if (q_idx < ppa->num_queue) {
			struct pktproc_queue *q = ppa->q[q_idx];

			irq = q->irq;
			irq_wake = &q->msi_irq_wake;
			q_idx++;
		}
#endif
	} while (irq);

	mld->msi_irq_enabled = enabled;
}

static int request_pcie_int(struct link_device *ld, struct platform_device *pdev)
{
#define DOORBELL_INT_MASK(x)	((x) | 0x10000)

	int ret, base_irq;
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct device *dev = &pdev->dev;
	struct modem_ctl *mc = ld->mc;
	struct modem_data *modem = mc->mdm_data;
	int irq_offset = 0;

	/* Doorbell */
	mld->intval_ap2cp_msg = DOORBELL_INT_MASK(modem->mbx->int_ap2cp_msg);
	mld->intval_ap2cp_pcie_link_ack = DOORBELL_INT_MASK(modem->mbx->int_ap2cp_pcie_link_ack);

	/* MSI */
	base_irq = s51xx_pcie_request_msi_int(mc->s51xx_pdev, 4);
	if (base_irq <= 0) {
		mif_err("Can't get MSI IRQ!!!\n");
		return -EFAULT;
	}
	mif_info("MSI base_irq(%d)\n", base_irq);

	ret = devm_request_irq(dev, base_irq + irq_offset, shmem_irq_handler,
			       IRQF_SHARED, "mif_cp2ap_msg", mld);
	if (ret) {
		mif_err("Can't request cp2ap_msg interrupt!!!\n");
		return -EIO;
	}
	irq_offset++;

	ret = devm_request_irq(dev, base_irq + irq_offset, shmem_tx_state_handler,
			       IRQF_SHARED, "mif_cp2ap_status", mld);
	if (ret) {
		mif_err("Can't request cp2ap_status interrupt!!!\n");
		return -EIO;
	}
	irq_offset++;

#if IS_ENABLED(CONFIG_CPIF_DIRECT_DM)
	ret = devm_request_irq(dev, base_irq + irq_offset, direct_dm_irq_handler,
			       IRQF_SHARED, "mif_cp2ap_ddm", direct_dm_get_dc());
	if (ret) {
		mif_err("Can't request cp2ap_ddm interrupt!!!\n");
		return -EIO;
	}
	irq_offset++;
#endif

#if IS_ENABLED(CONFIG_CP_PKTPROC)
	if (mld->pktproc.use_exclusive_irq) {
		struct pktproc_adaptor *ppa = &mld->pktproc;
		unsigned int i;

		for (i = 0; i < ppa->num_queue; i++) {
			struct pktproc_queue *q = ppa->q[i];

			ret = register_separated_msi_vector(mc->pcie_ch_num, q->irq_handler, q,
							    &q->irq);
			if (ret < 0) {
				mif_err("register_separated_msi_vector for pktproc q[%u] err:%d\n",
					i, ret);
				q->irq = 0;
				return -EIO;
			}
		}
	}
#endif

	mld->msi_irq_base = base_irq;
	mld->msi_irq_enabled = true;
	set_pcie_msi_int(ld, true);

	return base_irq;
}

static int register_pcie(struct link_device *ld)
{
	struct modem_ctl *mc = ld->mc;
	struct platform_device *pdev = to_platform_device(mc->dev);
	static int is_registered;
	struct mem_link_device *mld = to_mem_link_device(ld);

	mif_info("CP EP driver initialization start.\n");

	if (!mld->msi_reg_base && (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE))
		mld->msi_reg_base = cp_shmem_get_region(SHMEM_MSI);

#if IS_ENABLED(CONFIG_LINK_DEVICE_PCIE_S2MPU)
	if (!mc->s5100_s2mpu_enabled) {
		int ret;
		u32 shmem_idx;

		mc->s5100_s2mpu_enabled = true;

		for (shmem_idx = 0 ; shmem_idx < MAX_CP_SHMEM ; shmem_idx++) {
			if (shmem_idx == SHMEM_MSI && !(mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE))
				continue;

			if (cp_shmem_get_base(shmem_idx)) {
				ret = (int)exynos_set_dev_stage2_ap("hsi2", 0,
					cp_shmem_get_base(shmem_idx),
					cp_shmem_get_size(shmem_idx), ATTR_RW);
				mif_info("pcie s2mpu idx:%d - addr:0x%08lx size:0x%08x ret:%d\n",
					 shmem_idx,
					 cp_shmem_get_base(shmem_idx),
					 cp_shmem_get_size(shmem_idx), ret);
			}
		}
	}
#endif

#if IS_ENABLED(CONFIG_LINK_DEVICE_PCIE_IOMMU)
	cpif_pcie_iommu_enable_regions(mld);
#endif

	msleep(200);

	s5100_poweron_pcie(mc, !!(mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE));

	if (is_registered == 0) {
		/* initialize the pci_dev for modem_ctl */
		mif_info("s51xx_pcie_init start\n");
		s51xx_pcie_init(mc);
		if (!mc->s51xx_pdev) {
			mif_err("s51xx_pdev is NULL. Check if CP wake up is done.\n");
			return -EINVAL;
		}

		/* debug: check MSI 32bit or 64bit - should be set as 32bit before this point*/
		// debug: pci_read_config_dword(s51xx_pcie.s51xx_pdev, 0x50, &msi_val);
		// debug: mif_err("MSI Control Reg : 0x%x\n", msi_val);

		request_pcie_int(ld, pdev);
		first_save_s51xx_status(mc->s51xx_pdev);

		is_registered = 1;
	} else {
		if (mc->phone_state == STATE_CRASH_RESET) {
			print_msi_register(mc->s51xx_pdev);
			enable_irq(mld->msi_irq_base);
		}
	}

	print_msi_register(mc->s51xx_pdev);
	mc->pcie_registered = true;

	mif_info("CP EP driver initialization end.\n");

	return 0;
}

static void gpio_power_off_cp(struct modem_ctl *mc)
{
#if IS_ENABLED(CONFIG_CP_WRESET_WA)
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_NRESET], 0, 50);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_PWR], 0, 0);
#else
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_NRESET], 0, 0);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_WRST_N], 0, 0);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_PWR], 0, 30);
#endif
}

static void gpio_power_offon_cp(struct modem_ctl *mc)
{
	gpio_power_off_cp(mc);

#if IS_ENABLED(CONFIG_CP_WRESET_WA)
	udelay(50);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_PWR], 1, 50);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_NRESET], 1, 50);
#else
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_PWR], 1, 10);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_NRESET], 1, 10);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_WRST_N], 1, 0);
#endif
}

static void gpio_power_wreset_cp(struct modem_ctl *mc)
{
#if !IS_ENABLED(CONFIG_CP_WRESET_WA)
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_WRST_N], 0, 50);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_CP_WRST_N], 1, 50);
#endif
}

static int power_on_cp(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	struct modem_data __maybe_unused *modem = mc->mdm_data;
	struct mem_link_device *mld = to_mem_link_device(ld);

	mif_info("%s: +++\n", mc->name);

	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE]);
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);
	drain_workqueue(mc->wakeup_wq);

	print_mc_state(mc);

	if (!cpif_wake_lock_active(mc->ws))
		cpif_wake_lock(mc->ws);

	if (mc->phone_state != STATE_OFFLINE) {
		change_modem_state(mc, STATE_RESET);
		msleep(STATE_RESET_INTERVAL_MS);
	}
	change_modem_state(mc, STATE_OFFLINE);

	pcie_clean_dislink(mc);

	mc->pcie_registered = false;

	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 1, 0);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_DUMP_NOTI], 0, 0);

	/* Clear shared memory */
	init_ctrl_msg(&mld->ap2cp_msg);
	init_ctrl_msg(&mld->cp2ap_msg);

	print_mc_state(mc);
	gpio_power_offon_cp(mc);
	mif_info("GPIO status after S5100 Power on\n");
	print_mc_state(mc);

	mif_info("---\n");

	return 0;
}

static int power_off_cp(struct modem_ctl *mc)
{
	mif_info("%s: +++\n", mc->name);

	if (mc->phone_state == STATE_OFFLINE)
		goto exit;

	change_modem_state(mc, STATE_OFFLINE);

	pcie_clean_dislink(mc);

	gpio_power_off_cp(mc);
	print_mc_state(mc);

exit:
	mif_info("---\n");

	return 0;
}

static int power_shutdown_cp(struct modem_ctl *mc)
{
	int i;

	mif_err("%s: +++\n", mc->name);

	if (mc->phone_state == STATE_OFFLINE)
		goto exit;

	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE]);
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);
	drain_workqueue(mc->wakeup_wq);

	/* wait for cp_active for 3 seconds */
	for (i = 0; i < 150; i++) {
		if (mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_CP_ACTIVE], false) == 1) {
			mif_err("PHONE_ACTIVE pin is HIGH...\n");
			break;
		}
		msleep(20);
	}

#if IS_ENABLED(CONFIG_SEC_MODEM_S5400)
	exynos_pcie_host_v1_poweroff(mc->pcie_ch_num);
#endif

	gpio_power_off_cp(mc);
	print_mc_state(mc);

#if !IS_ENABLED(CONFIG_SEC_MODEM_S5400)
	pcie_clean_dislink(mc);
#endif

exit:
	mif_err("---\n");
	return 0;
}

static int power_reset_dump_cp(struct modem_ctl *mc)
{
	struct s51xx_pcie *s51xx_pcie = NULL;
#if IS_ENABLED(CONFIG_LINK_DEVICE_WITH_SBD_ARCH)
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (ld->sbd_ipc && hrtimer_active(&mld->sbd_print_timer))
		hrtimer_cancel(&mld->sbd_print_timer);
#endif
	mif_info("%s: +++\n", mc->name);

	mc->phone_state = STATE_CRASH_EXIT;
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE]);
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);
	drain_workqueue(mc->wakeup_wq);
	pcie_clean_dislink(mc);

	if (mc->s51xx_pdev != NULL)
		s51xx_pcie = pci_get_drvdata(mc->s51xx_pdev);

	if (s51xx_pcie && s51xx_pcie->link_status == 1) {
		mif_info("link_satus:%d\n", s51xx_pcie->link_status);
		s51xx_pcie_save_state(mc->s51xx_pdev);
		pcie_clean_dislink(mc);
	}

#if IS_ENABLED(CONFIG_LINK_DEVICE_PCIE_GPIO_WA)
	if (mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_DUMP_NOTI], 1, 10))
		mif_gpio_toggle_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 50);
#else
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_DUMP_NOTI], 1, 0);
#endif

	mif_info("s5100_cp_reset_required:%d\n", mc->s5100_cp_reset_required);
	if (mc->s5100_cp_reset_required)
		gpio_power_offon_cp(mc);
	else
		gpio_power_wreset_cp(mc);

	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 1, 0);
	print_mc_state(mc);

	mif_info("---\n");

	return 0;
}

static int check_cp_status(struct modem_ctl *mc, unsigned int count, bool check_msi)
{
#define STATUS_NAME(msi) (msi ? "boot_stage" : "CP2AP_WAKEUP")

	struct link_device *ld = get_current_link(mc->bootd);
	struct mem_link_device *mld = to_mem_link_device(ld);
	bool check_done = false;
	int cnt = 0;
	int val;

	do {
		/* ensure that CP updates the value */
		msleep(20);

		if (check_msi) {
			val = (int)ioread32(mld->msi_reg_base +
				offsetof(struct msi_reg_type, boot_stage));
			if (val == BOOT_STAGE_DONE_MASK) {
				check_done = true;
				break;
			}
		} else {
			val = mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP], false);
			if (val == 1) {
				check_done = true;
				break;
			}
		}

		mif_info_limited("%s == 0x%X (cnt %d)\n", STATUS_NAME(check_msi), val, cnt);
	} while (++cnt < count);

	if (!check_done) {
		mif_err("ERR! %s == 0x%X (cnt %d)\n", STATUS_NAME(check_msi), val, cnt);
		return -EFAULT;
	}

	mif_info("%s == 0x%X (cnt %d)\n", STATUS_NAME(check_msi), val, cnt);
	if (cnt == 0)
		msleep(10);

	return 0;
}

#if IS_ENABLED(CONFIG_SEC_MODEM_S5400)
static int check_boot_status(struct modem_ctl *mc, unsigned int count, bool check_bl1)
{
	struct link_device *ld = get_current_link(mc->bootd);
	struct mem_link_device *mld = to_mem_link_device(ld);
	bool check_done = false;
	int cnt = 0;
	int val;

	do {
		/* ensure that CP updates the value */
		msleep(20);

		val = (int)ioread32(mld->msi_reg_base +
			offsetof(struct msi_reg_type, boot_stage));
		if (val == (check_bl1 ? BOOT_STAGE_BL1_DOWNLOAD_DONE_MASK : BOOT_STAGE_DONE_MASK)) {
			check_done = true;
			break;
		}

		mif_info_limited("boot_stage == 0x%X (cnt %d)\n", val, cnt);
	} while (++cnt < count);

	if (!check_done) {
		mif_err("ERR! boot_stage == 0x%X (cnt %d)\n", val, cnt);
		return -EFAULT;
	}

	mif_info("boot_stage == 0x%X (cnt %d)\n", val, cnt);
	if (cnt == 0)
		msleep(10);

	return 0;
}
#endif

static int set_cp_rom_boot_img(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long boot_img_addr;

	if (!(mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE)) {
		mif_err("Invalid attr:0x%lx\n", mld->attrs);
		return -EPERM;
	}

	if (!mld->msi_reg_base) {
		mif_err("MSI region is not assigned yet\n");
		return -EINVAL;
	}

	boot_img_addr = cp_shmem_get_base(SHMEM_IPC) + mld->boot_img_offset;

#if !IS_ENABLED(CONFIG_SEC_MODEM_S5400)
	iowrite32(0,
		  mld->msi_reg_base + offsetof(struct msi_reg_type, boot_stage));
#endif
	iowrite32(PADDR_LO(boot_img_addr),
		  mld->msi_reg_base + offsetof(struct msi_reg_type, img_addr_lo));
	iowrite32(PADDR_HI(boot_img_addr),
		  mld->msi_reg_base + offsetof(struct msi_reg_type, img_addr_hi));
	iowrite32(mld->boot_img_size,
		  mld->msi_reg_base + offsetof(struct msi_reg_type, img_size));

	mif_info("boot_img addr:0x%lX size:0x%X\n", boot_img_addr, mld->boot_img_size);

	s51xx_pcie_send_doorbell_int(mc->s51xx_pdev, mld->intval_ap2cp_msg);

	return 0;
}

static void debug_cp_rom_boot_img(struct mem_link_device *mld)
{
	unsigned char str[64 * 3];
	u8 __iomem *img_base;
	u32 img_size;

	if (!(mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE)) {
		mif_err("Invalid attr:0x%lx\n", mld->attrs);
		return;
	}

	img_base = mld->base + mld->boot_img_offset;
	img_size = ioread32(mld->msi_reg_base + offsetof(struct msi_reg_type, img_size));

	mif_err("boot_stage:0x%X err_report:0x%X img_lo:0x%X img_hi:0x%X img_size:0x%X\n",
		ioread32(mld->msi_reg_base + offsetof(struct msi_reg_type, boot_stage)),
		ioread32(mld->msi_reg_base + offsetof(struct msi_reg_type, err_report)),
		ioread32(mld->msi_reg_base + offsetof(struct msi_reg_type, img_addr_lo)),
		ioread32(mld->msi_reg_base + offsetof(struct msi_reg_type, img_addr_hi)),
		img_size);

	if (img_size > 64)
		img_size = 64;

	dump2hex(str, (img_size ? img_size * 3 : 1), img_base, img_size);
	mif_err("img_content:%s\n", str);
}

static int start_normal_boot(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret = 0;

	mif_info("+++\n");

	if (init_control_messages(mc))
		mif_err("Failed to initialize control messages\n");

	/* 2cp dump WA */
	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);
	atomic_set(&mld->forced_cp_crash, 0);

	mif_info("Set link mode to LINK_MODE_BOOT.\n");

	if (ld->link_prepare_normal_boot)
		ld->link_prepare_normal_boot(ld, mc->bootd);

	change_modem_state(mc, STATE_BOOTING);

	mif_info("Disable phone actvie interrupt.\n");
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE]);

	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 1, 0);
	mc->phone_state = STATE_BOOTING;

	if (ld->link_start_normal_boot) {
		mif_info("link_start_normal_boot\n");
		ld->link_start_normal_boot(ld, mc->iod);
	}

	ret = modem_ctrl_check_offset_data(mc);
	if (ret) {
		mif_err("modem_ctrl_check_offset_data() error:%d\n", ret);
		return ret;
	}

	if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE) {
		register_pcie(ld);
		if (mc->s51xx_pdev && mc->pcie_registered)
			set_cp_rom_boot_img(mld);

		ret = check_cp_status(mc, 200, true);
		if (ret < 0)
			goto status_error;

		s5100_poweroff_pcie(mc, false);

		ret = check_cp_status(mc, 200, false);
		if (ret < 0)
			goto status_error;

		s5100_poweron_pcie(mc, false);
	} else {
		ret = check_cp_status(mc, 200, false);
		if (ret < 0)
			goto status_error;

		register_pcie(ld);
	}

status_error:
	if (ret < 0) {
		mif_err("ERR! check_cp_status fail (err %d)\n", ret);
		if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE)
			debug_cp_rom_boot_img(mld);
		if (cpif_wake_lock_active(mc->ws))
			cpif_wake_unlock(mc->ws);

		return ret;
	}

	mif_info("---\n");
	return 0;
}

static int complete_normal_boot(struct modem_ctl *mc)
{
	int err = 0;
	unsigned long remain;

	mif_info("+++\n");

	reinit_completion(&mc->init_cmpl);
	remain = wait_for_completion_timeout(&mc->init_cmpl, MIF_INIT_TIMEOUT);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		err = -EAGAIN;
		goto exit;
	}

	/* Enable L1.2 after CP boot */
	s51xx_pcie_l1ss_ctrl(1);

	/* Read cp_active before enabling irq */
	mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_CP_ACTIVE], true);

	err = register_phone_active_interrupt(mc);
	if (err)
		mif_err("Err: register_phone_active_interrupt:%d\n", err);
	mif_enable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE]);

	err = register_cp2ap_wakeup_interrupt(mc);
	if (err)
		mif_err("Err: register_cp2ap_wakeup_interrupt:%d\n", err);
	mif_enable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);

	print_mc_state(mc);

	mc->device_reboot = false;

	change_modem_state(mc, STATE_ONLINE);

	print_mc_state(mc);

	mif_info("---\n");

exit:
	return err;
}

#if IS_ENABLED(CONFIG_SEC_MODEM_S5400)
static int start_normal_boot_bl1(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret = 0;

	mif_info("+++\n");

	if (init_control_messages(mc))
		mif_err("Failed to initialize control messages\n");

	/* 2cp dump WA */
	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);
	atomic_set(&mld->forced_cp_crash, 0);

	mif_info("Set link mode to LINK_MODE_BOOT.\n");

	if (ld->link_prepare_normal_boot)
		ld->link_prepare_normal_boot(ld, mc->bootd);

	change_modem_state(mc, STATE_BOOTING);

	mif_info("Disable phone actvie interrupt.\n");
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_CP_ACTIVE]);

	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 1, 0);
	mc->phone_state = STATE_BOOTING;

	if (ld->link_start_normal_boot) {
		mif_info("link_start_normal_boot\n");
		ld->link_start_normal_boot(ld, mc->iod);
	}

	ret = modem_ctrl_check_offset_data(mc);
	if (ret) {
		mif_err("modem_ctrl_check_offset_data() error:%d\n", ret);
		return ret;
	}

	if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE) {
		register_pcie(ld);
		if (mc->s51xx_pdev && mc->pcie_registered)
			set_cp_rom_boot_img(mld);

		ret = check_boot_status(mc, 200, true);
		if (ret < 0)
			goto status_error;
	} else {
		mif_err("ERR! LINK_ATTR_XMIT_BTDLR_PCIE is not set\n");
		return -EFAULT;
	}

status_error:
	if (ret < 0) {
		mif_err("ERR! check_cp_status fail (err %d)\n", ret);
		if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE)
			debug_cp_rom_boot_img(mld);
		if (cpif_wake_lock_active(mc->ws))
			cpif_wake_unlock(mc->ws);

		return ret;
	}

	mif_info("---\n");
	return 0;
}

static int start_normal_boot_bootloader(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret = 0;
	int val;

	mif_info("+++\n");

	if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE) {
		if (mc->s51xx_pdev && mc->pcie_registered)
			set_cp_rom_boot_img(mld);

		ret = check_boot_status(mc, 200, false);
		if (ret < 0)
			goto status_error;

		iowrite32(0, mld->msi_reg_base + offsetof(struct msi_reg_type, boot_stage));
		val = (int)ioread32(mld->msi_reg_base + offsetof(struct msi_reg_type, boot_stage));
		mif_info("Clear boot_stage == 0x%X\n", val);

		s5100_poweroff_pcie(mc, false);

		ret = check_cp_status(mc, 200, false);
		if (ret < 0)
			goto status_error;

		s5100_poweron_pcie(mc, false);
	} else {
		mif_err("ERR! LINK_ATTR_XMIT_BTDLR_PCIE is not set\n");
		return -EFAULT;
	}

status_error:
	if (ret < 0) {
		mif_err("ERR! check_cp_status fail (err %d)\n", ret);
		if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE)
			debug_cp_rom_boot_img(mld);
		if (cpif_wake_lock_active(mc->ws))
			cpif_wake_unlock(mc->ws);

		return ret;
	}

	mif_info("---\n");
	return 0;
}
#endif /* __SEC_MODEM_S5400_H__ */

static void ap2cp_dump_noti_work(struct work_struct *ws)
{
	struct modem_ctl *mc = container_of(ws, struct modem_ctl, dump_noti_work);

	mif_info("+++\n");

	if (mc->device_reboot) {
		mif_err("skip cp crash : device is rebooting..!!!\n");
		goto exit;
	}

	print_mc_state(mc);

	if (mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_CP_ACTIVE], true) == 1) {
#if IS_ENABLED(CONFIG_LINK_DEVICE_PCIE_GPIO_WA)
		if (atomic_inc_return(&mc->dump_toggle_issued) > 1) {
			atomic_dec(&mc->dump_toggle_issued);
			goto exit;
		}

		if (mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_DUMP_NOTI], 1, 10))
			mif_gpio_toggle_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 50);

		atomic_dec(&mc->dump_toggle_issued);
#else
		mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_DUMP_NOTI], 1, 0);
#endif
	} else {
		mif_err("do not need to set dump_noti\n");
	}

exit:
	mif_info("---\n");
}

void s5100_send_dump_noti(struct modem_ctl *mc)
{
	queue_work(mc->dump_noti_wq, &mc->dump_noti_work);
}

static int start_dump_boot(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int err = 0;

	mif_err("+++\n");

	/* Change phone state to CRASH_EXIT */
	mc->phone_state = STATE_CRASH_EXIT;

	if (!ld->link_start_dump_boot) {
		mif_err("%s: link_start_dump_boot is null\n", ld->name);
		return -EFAULT;
	}

	err = ld->link_start_dump_boot(ld, mc->bootd);
	if (err)
		return err;

	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 1, 0);
	/* do not handle cp2ap_wakeup irq during dump process */
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);

	if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE) {
		register_pcie(ld);
		if (mc->s51xx_pdev && mc->pcie_registered)
			set_cp_rom_boot_img(mld);

		err = check_cp_status(mc, 200, true);
		if (err < 0)
			goto status_error;

		s5100_poweroff_pcie(mc, false);

		err = check_cp_status(mc, 200, false);
		if (err < 0)
			goto status_error;

		s5100_poweron_pcie(mc, false);
	} else {
		err = check_cp_status(mc, 200, false);
		if (err < 0)
			goto status_error;

		register_pcie(ld);
	}

status_error:
	if (err < 0) {
		mif_err("ERR! check_cp_status fail (err %d)\n", err);
		if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE)
			debug_cp_rom_boot_img(mld);
		return err;
	}

	mif_err("---\n");
	return err;
}

#if IS_ENABLED(CONFIG_SEC_MODEM_S5400)
static int start_dump_boot_bl1(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int err = 0;

	mif_err("+++\n");

	/* Change phone state to CRASH_EXIT */
	mc->phone_state = STATE_CRASH_EXIT;

	if (!ld->link_start_dump_boot) {
		mif_err("%s: link_start_dump_boot is null\n", ld->name);
		return -EFAULT;
	}

	err = ld->link_start_dump_boot(ld, mc->bootd);
	if (err)
		return err;

	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 1, 0);
	/* do not handle cp2ap_wakeup irq during dump process */
	mif_disable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);

	if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE) {
		register_pcie(ld);
		if (mc->s51xx_pdev && mc->pcie_registered)
			set_cp_rom_boot_img(mld);

		err = check_boot_status(mc, 200, true);
		if (err < 0)
			goto status_error;
	} else {
		mif_err("ERR! LINK_ATTR_XMIT_BTDLR_PCIE is not set\n");
		return -EFAULT;
	}

status_error:
	if (err < 0) {
		mif_err("ERR! check_cp_status fail (err %d)\n", err);
		if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE)
			debug_cp_rom_boot_img(mld);
		return err;
	}

	mif_err("---\n");
	return err;
}

static int start_dump_boot_bootloader(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int err = 0;
	int val;

	mif_err("+++\n");

	if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE) {
		if (mc->s51xx_pdev && mc->pcie_registered)
			set_cp_rom_boot_img(mld);

		err = check_boot_status(mc, 200, false);
		if (err < 0)
			goto status_error;

		iowrite32(0, mld->msi_reg_base + offsetof(struct msi_reg_type, boot_stage));
		val = (int)ioread32(mld->msi_reg_base + offsetof(struct msi_reg_type, boot_stage));
		mif_info("Clear boot_stage == 0x%X\n", val);

		s5100_poweroff_pcie(mc, false);

		err = check_cp_status(mc, 200, false);
		if (err < 0)
			goto status_error;

		s5100_poweron_pcie(mc, false);
	} else {
		mif_err("ERR! LINK_ATTR_XMIT_BTDLR_PCIE is not set\n");
		return -EFAULT;
	}

status_error:
	if (err < 0) {
		mif_err("ERR! check_cp_status fail (err %d)\n", err);
		if (mld->attrs & LINK_ATTR_XMIT_BTDLR_PCIE)
			debug_cp_rom_boot_img(mld);
		return err;
	}

	mif_err("---\n");
	return err;
}
#endif /* __SEC_MODEM_S5400_H__ */

static int s5100_poweroff_pcie(struct modem_ctl *mc, bool force_off)
{
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	bool force_crash = false;
	bool in_pcie_recovery = false;
	unsigned long flags;

	mutex_lock(&mc->pcie_onoff_lock);
	mutex_lock(&mc->pcie_check_lock);
	mif_info("+++\n");

	if (!mc->pcie_powered_on &&
			(s51xx_check_pcie_link_status(mc->pcie_ch_num) == 0)) {
		mif_err("skip pci power off : already powered off\n");
		goto exit;
	}

	/* CP reads Tx RP (or tail) after CP2AP_WAKEUP = 1.
	 * skip pci power off if CP2AP_WAKEUP = 1 or Tx pending.
	 */
	if (!force_off) {
		spin_lock_irqsave(&mc->pcie_tx_lock, flags);
		/* wait Tx done if it is running */
		spin_unlock_irqrestore(&mc->pcie_tx_lock, flags);
		msleep(30);
		if (check_mem_link_tx_pending(mld) ||
			mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP], true) == 1) {
			mif_err("skip pci power off : condition not met\n");
			goto exit;
		}
	}

	set_pcie_msi_int(ld, false);

	if (mc->device_reboot) {
		mif_err("skip pci power off : device is rebooting..!!!\n");
		goto exit;
	}

	/* recovery status is not valid after PCI link down requests from CP */
	if (mc->pcie_cto_retry_cnt > 0) {
		mif_info("clear cto_retry_cnt(%d)..!!!\n", mc->pcie_cto_retry_cnt);
		mc->pcie_cto_retry_cnt = 0;
	}

	if (exynos_pcie_rc_get_cpl_timeout_state(mc->pcie_ch_num)) {
		exynos_pcie_rc_set_cpl_timeout_state(mc->pcie_ch_num, false);
		in_pcie_recovery = true;
	}

	mc->pcie_powered_on = false;

	if (mc->s51xx_pdev != NULL && (mc->phone_state == STATE_ONLINE ||
				mc->phone_state == STATE_BOOTING)) {
		mif_info("save s5100_status - phone_state:%d\n",
				mc->phone_state);
		s51xx_pcie_save_state(mc->s51xx_pdev);
	} else
		mif_info("ignore save_s5100_status - phone_state:%d\n",
				mc->phone_state);

	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_WAKEUP], 0, 5);
	print_mc_state(mc);

	exynos_pcie_host_v1_poweroff(mc->pcie_ch_num);

	if (cpif_wake_lock_active(mc->ws))
		cpif_wake_unlock(mc->ws);

exit:
	mif_info("---\n");
	mutex_unlock(&mc->pcie_check_lock);
	mutex_unlock(&mc->pcie_onoff_lock);

	spin_lock_irqsave(&mc->pcie_tx_lock, flags);
	if (in_pcie_recovery && !mc->reserve_doorbell_int && check_mem_link_tx_pending(mld))
		mc->reserve_doorbell_int = true;

	if ((mc->s51xx_pdev != NULL) && !mc->device_reboot && mc->reserve_doorbell_int) {
		mif_info("DBG: doorbell_reserved = %d\n", mc->reserve_doorbell_int);
		if (mc->pcie_powered_on) {
			mc->reserve_doorbell_int = false;
			if (s51xx_pcie_send_doorbell_int(mc->s51xx_pdev,
						mld->intval_ap2cp_msg) != 0)
				force_crash = true;
		} else
			s5100_try_gpio_cp_wakeup(mc);
	}
	spin_unlock_irqrestore(&mc->pcie_tx_lock, flags);

	if (unlikely(force_crash))
		modem_force_crash_exit(mc);

	return 0;
}

int s5100_poweron_pcie(struct modem_ctl *mc, bool boot_on)
{
	struct link_device *ld;
	struct mem_link_device *mld;
	bool force_crash = false;
	unsigned long flags;

	if (mc == NULL) {
		mif_err("Skip pci power on : mc is NULL\n");
		return 0;
	}

	ld = get_current_link(mc->iod);
	mld = to_mem_link_device(ld);

	if (mc->phone_state == STATE_OFFLINE) {
		mif_err("Skip pci power on : phone_state is OFFLINE\n");
		return 0;
	}

	mutex_lock(&mc->pcie_onoff_lock);
	mutex_lock(&mc->pcie_check_lock);
	mif_info("+++\n");
	if (mc->pcie_powered_on &&
			(s51xx_check_pcie_link_status(mc->pcie_ch_num) != 0)) {
		mif_err("skip pci power on : already powered on\n");
		goto exit;
	}

	if (!boot_on &&
	    mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP], true) == 0) {
		mif_err("skip pci power on : condition not met\n");
		goto exit;
	}

	if (mc->device_reboot) {
		mif_err("skip pci power on : device is rebooting..!!!\n");
		goto exit;
	}

	if (!cpif_wake_lock_active(mc->ws))
		cpif_wake_lock(mc->ws);

	if (!boot_on)
		mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_WAKEUP], 1, 5);
	print_mc_state(mc);

	spin_lock_irqsave(&mc->pcie_tx_lock, flags);
	/* wait Tx done if it is running */
	spin_unlock_irqrestore(&mc->pcie_tx_lock, flags);

	if (exynos_pcie_rc_get_cpl_timeout_state(mc->pcie_ch_num))
		exynos_pcie_set_ready_cto_recovery(mc->pcie_ch_num);

#if IS_ENABLED(CONFIG_SEC_MODEM_S5400)
	/* Temporary request GEN3 (from GEN4) */
	if (exynos_pcie_host_v1_poweron(mc->pcie_ch_num, (boot_on ? 1 : 3)) != 0)
		goto exit;
#else
	if (exynos_pcie_host_v1_poweron(mc->pcie_ch_num, (boot_on ? 1 : 3)) != 0)
		goto exit;
#endif

	mc->pcie_powered_on = true;

	if (mc->s51xx_pdev != NULL) {
		s51xx_pcie_restore_state(mc->s51xx_pdev, boot_on);

		/* DBG: check MSI sfr setting values */
		print_msi_register(mc->s51xx_pdev);
	} else {
		mif_err("DBG: MSI sfr not set up, yet(s5100_pdev is NULL)");
	}

	set_pcie_msi_int(ld, true);

	if ((mc->s51xx_pdev != NULL) && mc->pcie_registered && (mc->phone_state != STATE_CRASH_EXIT)) {
		/* DBG */
		mif_info("DBG: doorbell: pcie_registered = %d\n", mc->pcie_registered);
		if (s51xx_pcie_send_doorbell_int(mc->s51xx_pdev,
						 mld->intval_ap2cp_pcie_link_ack) != 0) {
			/* DBG */
			mif_err("DBG: s5100pcie_send_doorbell_int() func. is failed !!!\n");
			modem_force_crash_exit(mc);
		}
	}

#if IS_ENABLED(CONFIG_SUSPEND_DURING_VOICE_CALL)
	if (mc->pcie_voice_call_on && (mc->phone_state != STATE_CRASH_EXIT)) {
		if (cpif_wake_lock_active(mc->ws))
			cpif_wake_unlock(mc->ws);

		mif_info("wakelock active = %d, voice status = %d\n",
			cpif_wake_lock_active(mc->ws), mc->pcie_voice_call_on);
	}
#endif

exit:
	mif_info("---\n");
	mutex_unlock(&mc->pcie_check_lock);
	mutex_unlock(&mc->pcie_onoff_lock);

	spin_lock_irqsave(&mc->pcie_tx_lock, flags);
	if ((mc->s51xx_pdev != NULL) && mc->pcie_powered_on && mc->reserve_doorbell_int) {
		mif_info("DBG: doorbell: doorbell_reserved = %d\n", mc->reserve_doorbell_int);
		mc->reserve_doorbell_int = false;
		if (s51xx_pcie_send_doorbell_int(mc->s51xx_pdev, mld->intval_ap2cp_msg) != 0)
			force_crash = true;
	}
	spin_unlock_irqrestore(&mc->pcie_tx_lock, flags);

	if (unlikely(force_crash))
		modem_force_crash_exit(mc);

	return 0;
}

void s5100_set_pcie_irq_affinity(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
#if IS_ENABLED(CONFIG_CP_PKTPROC)
	struct pktproc_adaptor *ppa = &mld->pktproc;
	unsigned int num_queue = 1;
	unsigned int i;

	if (ppa->use_exclusive_irq)
		num_queue = ppa->num_queue;

	for (i = 0; i < num_queue; i++) {
		if (!ppa->q[i]->irq)
			break;

		irq_set_affinity_hint(ppa->q[i]->irq, cpumask_of(mld->msi_irq_q_cpu[i]));
	}
#endif

	if (mld->msi_irq_base)
		irq_set_affinity_hint(mld->msi_irq_base, cpumask_of(mld->msi_irq_base_cpu));
}

int s5100_set_outbound_atu(struct modem_ctl *mc, struct cp_btl *btl, loff_t *pos, u32 map_size)
{
	int ret = 0;
	u32 atu_grp = (*pos) / map_size;

	if (atu_grp != btl->last_pcie_atu_grp) {
		ret = exynos_pcie_rc_set_outbound_atu(
			mc->pcie_ch_num, btl->mem.cp_p_base, (atu_grp * map_size), map_size);
		btl->last_pcie_atu_grp = atu_grp;
	}

	return ret;
}

static int suspend_cp(struct modem_ctl *mc)
{
	if (!mc)
		return 0;

	do {
#if IS_ENABLED(CONFIG_SUSPEND_DURING_VOICE_CALL)
		if (mc->pcie_voice_call_on)
			break;
#endif

		if (mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP], true) == 1) {
			mif_err("abort suspend\n");
			return -EBUSY;
		}
	} while (0);

	modem_ctrl_set_kerneltime(mc);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 0, 0);
	mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], true);

	return 0;
}

static int resume_cp(struct modem_ctl *mc)
{
	if (!mc)
		return 0;

	s5100_set_pcie_irq_affinity(mc);

	modem_ctrl_set_kerneltime(mc);
	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], 1, 0);
	mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE], true);

	return 0;
}

static int s5100_pm_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	struct modem_ctl *mc;
	unsigned long flags;
	int gpio_val;

	mc = container_of(notifier, struct modem_ctl, pm_notifier);

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		mif_info("Suspend prepare\n");

		spin_lock_irqsave(&mc->pcie_pm_lock, flags);
		mc->pcie_pm_suspended = true;
		spin_unlock_irqrestore(&mc->pcie_pm_lock, flags);
		break;
	case PM_POST_SUSPEND:
		mif_info("Resume done\n");

		spin_lock_irqsave(&mc->pcie_pm_lock, flags);
		mc->pcie_pm_suspended = false;
		if (mc->pcie_pm_resume_wait) {
			mc->pcie_pm_resume_wait = false;
			gpio_val = mc->pcie_pm_resume_gpio_val;

			mif_err("cp2ap_wakeup work resume. gpio_val : %d\n", gpio_val);

			mc->apwake_irq_chip->irq_set_type(
				irq_get_irq_data(mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP].num),
				(gpio_val == 1 ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH));
			mif_enable_irq(&mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP]);

			queue_work_on(RUNTIME_PM_AFFINITY_CORE, mc->wakeup_wq,
				(gpio_val == 1 ? &mc->wakeup_work : &mc->suspend_work));
		}
		spin_unlock_irqrestore(&mc->pcie_pm_lock, flags);
		break;
	default:
		mif_info("pm_event %lu\n", pm_event);
		break;
	}

	return NOTIFY_OK;
}

int s5100_try_gpio_cp_wakeup(struct modem_ctl *mc)
{
	if ((mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_AP2CP_WAKEUP], false) == 0) &&
	    (mif_gpio_get_value(&mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP], false) == 0) &&
	    (s51xx_check_pcie_link_status(mc->pcie_ch_num) == 0)) {
		mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_WAKEUP], 1, 0);
		return 0;
	}
	return -EPERM;
}

static void s5100_get_ops(struct modem_ctl *mc)
{
	mc->ops.power_on = power_on_cp;
	mc->ops.power_off = power_off_cp;
	mc->ops.power_shutdown = power_shutdown_cp;
	mc->ops.power_reset = NULL;
	mc->ops.power_reset_dump = power_reset_dump_cp;

	mc->ops.start_normal_boot = start_normal_boot;
	mc->ops.complete_normal_boot = complete_normal_boot;
#if IS_ENABLED(CONFIG_SEC_MODEM_S5400)
	mc->ops.start_normal_boot_bl1 = start_normal_boot_bl1;
	mc->ops.start_normal_boot_bootloader = start_normal_boot_bootloader;
	mc->ops.start_dump_boot_bl1 = start_dump_boot_bl1;
	mc->ops.start_dump_boot_bootloader = start_dump_boot_bootloader;
#endif

	mc->ops.start_dump_boot = start_dump_boot;

	mc->ops.suspend = suspend_cp;
	mc->ops.resume = resume_cp;
}

static int s5100_get_pdata(struct modem_ctl *mc, struct modem_data *pdata)
{
	struct platform_device *pdev = to_platform_device(mc->dev);
	struct device_node *np = pdev->dev.of_node;
	unsigned int i;

	/* label */
	mc->cp_gpio[CP_GPIO_AP2CP_CP_PWR].label = "AP2CP_CP_PWR";
	mc->cp_gpio[CP_GPIO_AP2CP_NRESET].label = "AP2CP_NRESET";
	mc->cp_gpio[CP_GPIO_AP2CP_WAKEUP].label = "AP2CP_WAKEUP";
	mc->cp_gpio[CP_GPIO_AP2CP_DUMP_NOTI].label = "AP2CP_DUMP_NOTI";
	mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE].label = "AP2CP_AP_ACTIVE";
#if !IS_ENABLED(CONFIG_CP_WRESET_WA)
	mc->cp_gpio[CP_GPIO_AP2CP_CP_WRST_N].label = "AP2CP_CP_WRST_N";
#endif
	mc->cp_gpio[CP_GPIO_CP2AP_PS_HOLD].label = "CP2AP_PS_HOLD";
	mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP].label = "CP2AP_WAKEUP";
	mc->cp_gpio[CP_GPIO_CP2AP_CP_ACTIVE].label = "CP2AP_CP_ACTIVE";

	/* node name */
	mc->cp_gpio[CP_GPIO_AP2CP_CP_PWR].node_name = "gpio_ap2cp_cp_pwr_on";
	mc->cp_gpio[CP_GPIO_AP2CP_NRESET].node_name = "gpio_ap2cp_nreset_n";
	mc->cp_gpio[CP_GPIO_AP2CP_WAKEUP].node_name = "gpio_ap2cp_wake_up";
	mc->cp_gpio[CP_GPIO_AP2CP_DUMP_NOTI].node_name = "gpio_ap2cp_dump_noti";
	mc->cp_gpio[CP_GPIO_AP2CP_AP_ACTIVE].node_name = "gpio_ap2cp_pda_active";
#if !IS_ENABLED(CONFIG_CP_WRESET_WA)
	mc->cp_gpio[CP_GPIO_AP2CP_CP_WRST_N].node_name = "gpio_ap2cp_cp_wrst_n";
#endif
	mc->cp_gpio[CP_GPIO_CP2AP_PS_HOLD].node_name = "gpio_cp2ap_cp_ps_hold";
	mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP].node_name = "gpio_cp2ap_wake_up";
	mc->cp_gpio[CP_GPIO_CP2AP_CP_ACTIVE].node_name = "gpio_cp2ap_phone_active";

	/* irq */
	mc->cp_gpio[CP_GPIO_CP2AP_WAKEUP].irq_type = CP_GPIO_IRQ_CP2AP_WAKEUP;
	mc->cp_gpio[CP_GPIO_CP2AP_CP_ACTIVE].irq_type = CP_GPIO_IRQ_CP2AP_CP_ACTIVE;

	/* gpio */
	for (i = 0; i < CP_GPIO_MAX; i++) {
		mc->cp_gpio[i].num =
			of_get_named_gpio(np, mc->cp_gpio[i].node_name, 0);

		if (!gpio_is_valid(mc->cp_gpio[i].num))
			continue;

		mc->cp_gpio[i].valid = true;

		gpio_request(mc->cp_gpio[i].num, mc->cp_gpio[i].label);
		if (!strncmp(mc->cp_gpio[i].label, "AP2CP", 5))
			gpio_direction_output(mc->cp_gpio[i].num, 0);
		else
			gpio_direction_input(mc->cp_gpio[i].num);

		if (mc->cp_gpio[i].irq_type != CP_GPIO_IRQ_NONE) {
			mc->cp_gpio_irq[mc->cp_gpio[i].irq_type].num =
				gpio_to_irq(mc->cp_gpio[i].num);
		}
	}

	/* validate */
	for (i = 0; i < CP_GPIO_MAX; i++) {
		if (!mc->cp_gpio[i].valid) {
			mif_err("Missing some of GPIOs\n");
			return -EINVAL;
		}
	}

	/* Get PCIe Channel Number */
	mif_dt_read_u32(np, "pci_ch_num", mc->pcie_ch_num);
	mif_info("PCIe Channel Number:%d\n", mc->pcie_ch_num);

	mc->sbi_crash_type_mask = pdata->sbi_crash_type_mask;
	mc->sbi_crash_type_pos = pdata->sbi_crash_type_pos;

	mc->sbi_tx_flowctl_mask = pdata->sbi_tx_flowctl_mask;
	mc->sbi_tx_flowctl_pos = pdata->sbi_tx_flowctl_pos;

	mc->sbi_ds_det_mask = pdata->sbi_ds_det_mask;
	mc->sbi_ds_det_pos = pdata->sbi_ds_det_pos;

	return 0;
}

#if IS_ENABLED(CONFIG_SUSPEND_DURING_VOICE_CALL)
static int s5100_abox_call_state_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct modem_ctl *mc = container_of(nb, struct modem_ctl, abox_call_state_nb);

	mif_info("call event = %lu\n", action);

	switch (action) {
	case ABOX_CALL_EVENT_OFF:
		mc->pcie_voice_call_on = false;
		queue_work_on(RUNTIME_PM_AFFINITY_CORE, mc->wakeup_wq,
			&mc->call_off_work);
		break;
	case ABOX_CALL_EVENT_ON:
		mc->pcie_voice_call_on = true;
		queue_work_on(RUNTIME_PM_AFFINITY_CORE, mc->wakeup_wq,
			&mc->call_on_work);
		break;
	default:
		mif_err("undefined call event = %lu\n", action);
		break;
	}

	return NOTIFY_DONE;
}
#endif

int s5100_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(mc->dev);

	mif_err("+++\n");

	s5100_get_ops(mc);
	if (s5100_get_pdata(mc, pdata)) {
		mif_err("DT error: failed to parse\n");
		return -EINVAL;
	}
	dev_set_drvdata(mc->dev, mc);

	mc->ws = cpif_wake_lock_register(&pdev->dev, "s5100_wake_lock");
	if (mc->ws == NULL) {
		mif_err("s5100_wake_lock: wakeup_source_register fail\n");
		return -EINVAL;
	}
	mutex_init(&mc->pcie_onoff_lock);
	mutex_init(&mc->pcie_check_lock);
	spin_lock_init(&mc->pcie_tx_lock);
	spin_lock_init(&mc->pcie_pm_lock);
#if IS_ENABLED(CONFIG_LINK_DEVICE_PCIE_GPIO_WA)
	atomic_set(&mc->dump_toggle_issued, 0);
#endif

	mif_gpio_set_value(&mc->cp_gpio[CP_GPIO_AP2CP_NRESET], 0, 0);

	mif_info("Register GPIO interrupts\n");
	mc->apwake_irq_chip = irq_get_chip(mc->cp_gpio_irq[CP_GPIO_IRQ_CP2AP_WAKEUP].num);
	if (mc->apwake_irq_chip == NULL) {
		mif_err("Can't get irq_chip structure!!!!\n");
		return -EINVAL;
	}

	mc->wakeup_wq = create_singlethread_workqueue("cp2ap_wakeup_wq");
	if (!mc->wakeup_wq) {
		mif_err("%s: ERR! fail to create wakeup_wq\n", mc->name);
		return -EINVAL;
	}
	INIT_WORK(&mc->wakeup_work, cp2ap_wakeup_work);
	INIT_WORK(&mc->suspend_work, cp2ap_suspend_work);

	mc->dump_noti_wq = create_singlethread_workqueue("dump_noti_wq");
	if (!mc->dump_noti_wq) {
		mif_err("%s: ERR! fail to create crash_wq\n", mc->name);
		return -EINVAL;
	}
	INIT_WORK(&mc->dump_noti_work, ap2cp_dump_noti_work);

	mc->reboot_nb.notifier_call = s5100_reboot_handler;
	register_reboot_notifier(&mc->reboot_nb);

	/* Register PM notifier_call */
	mc->pm_notifier.notifier_call = s5100_pm_notifier;
	ret = register_pm_notifier(&mc->pm_notifier);
	if (ret) {
		mif_err("failed to register PM notifier_call\n");
		return ret;
	}

#if IS_ENABLED(CONFIG_SUSPEND_DURING_VOICE_CALL)
	INIT_WORK(&mc->call_on_work, voice_call_on_work);
	INIT_WORK(&mc->call_off_work, voice_call_off_work);

	mc->abox_call_state_nb.notifier_call = s5100_abox_call_state_notifier;
	register_abox_call_event_notifier(&mc->abox_call_state_nb);
#endif

	if (sysfs_create_group(&pdev->dev.kobj, &sim_group))
		mif_err("failed to create sysfs node related sim\n");

	mif_err("---\n");

	return 0;
}
