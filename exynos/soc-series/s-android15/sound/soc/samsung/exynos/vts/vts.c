// SPDX-License-Identifier: GPL-2.0-or-later
/* sound/soc/samsung/vts/vts.c
 *
 * ALSA SoC - Samsung VTS driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/sched/clock.h>
#include <linux/pinctrl/consumer.h>
#include <linux/suspend.h>
#include <linux/panic_notifier.h>

#include <asm-generic/delay.h>

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/samsung/vts.h>
#include <sound/exynos/sounddev_vts.h>

#include <sound/samsung/vts.h>
#include <soc/samsung/exynos-pmu-if.h>
#include <soc/samsung/exynos-el3_mon.h>
#if IS_ENABLED(CONFIG_EXYNOS_ITMON) || IS_ENABLED(CONFIG_EXYNOS_ITMON_V2)
#include <soc/samsung/exynos/exynos-itmon.h>
#endif
#include <soc/samsung/exynos/exynos-s2mpu.h>
#include <soc/samsung/exynos/debug-snapshot.h>

#include "vts.h"
#include "vts_res.h"
#include "vts_log.h"
/* #include "vts_s_lif_dump.h" */
#include "vts_dbg.h"
/* For DEF_VTS_PCM_DUMP */
#include "vts_pcm_dump.h"
#include "vts_proc.h"
#include "vts_util.h"
#include "vts_ipc.h"
#include "vts_misc.h"
#include "vts_ctrl.h"

#undef EMULATOR
#ifdef EMULATOR
static void __iomem *pmu_alive;
#define set_mask_value(id, mask, value)	(id = ((id & ~mask) | (value & mask)))

static void update_mask_value(void __iomem *sfr,
	unsigned int mask, unsigned int value)
{
	unsigned int sfr_value = readl(sfr);

	set_mask_value(sfr_value, mask, value);
	writel(sfr_value, sfr);
}
#endif

#define clear_bit(data, offset) ((data) &= ~(0x1<<(offset)))
#define clear_bits(data, value, offset) ((data) &= ~((value)<<(offset)))
#define set_bit(data, offset) ((data) |= (0x1<<(offset)))
#define set_bits(data, value, offset) ((data) |= ((value)<<(offset)))
#define check_bit(data, offset)	(((data)>>(offset)) & (0x1))

static BLOCKING_NOTIFIER_HEAD(vts_notifier);


/* For only external static functions */
static struct vts_data *p_vts_data;
static void vts_dbg_dump_fw_gpr(struct device *dev, struct vts_data *data,
			unsigned int dbg_type);

static void vts_check_version(struct device *dev);

static struct platform_driver samsung_vts_driver;

#if IS_ENABLED(CONFIG_EXYNOS_S2MPU)
static int vts_s2mpu_fault_handler(struct s2mpu_notifier_block *nb,
						struct s2mpu_notifier_info *ni)
{
	struct vts_data *data = container_of(nb, struct vts_data, s2mpu_nb);
	struct device *dev = &data->pdev->dev;

	vts_dev_info(dev, "%s : we get an s2mpu notifier block\n", __func__);
	vts_dbg_dump_fw_gpr(dev, data, VTS_FW_ERROR);

	return S2MPU_NOTIFY_BAD;
}
#endif

struct vts_data *vts_get_vts_data(void)
{
	return p_vts_data;
}

static void vts_print_fw_status(struct device *dev, const char *func_name)
{
	struct vts_data *data = dev_get_drvdata(dev);

	if (vts_is_running()) {
		vts_dev_info(dev, "%s: shared_info vendor_data:0x%x %d %d, trigger_mode:0x%x",
			func_name,
			data->shared_info->vendor_data[0],
			data->shared_info->vendor_data[1],
			data->shared_info->vendor_data[2],
			data->shared_info->config_fw.trigger_mode);
	}
}

static int vts_ipc_wait_for_ack(struct device *dev, int msg, u32 *ack_value)
{
	struct vts_data *data = dev_get_drvdata(dev);
	enum ipc_state *state = &data->ipc_state_ap;
	int i;

	for (i = 200; i && (*state != SEND_MSG_OK) &&
			(*state != SEND_MSG_FAIL) &&
			(*ack_value != (0x1 << msg)); i--) {
		vts_mailbox_read_shared_register(data->pdev_mailbox,
						ack_value, 3, 1);
		vts_dev_dbg(dev, "%s ACK-value: 0x%08x\n", VTS_TAG_IPC,
			*ack_value);
		mdelay(1);
	}
	if (!i) {
		vts_dev_warn(dev, "Transaction timeout!! Ack_value:0x%x\n",
				*ack_value);
		vts_dbg_dump_fw_gpr(dev, data, VTS_IPC_TRANS_FAIL);
		vts_print_fw_status(dev, __func__);
		return -EINVAL;
	}

	return 0;
}

static int vts_start_ipc_transaction_atomic(
	struct device *dev, struct vts_data *data,
	int msg, u32 (*values)[3], int sync)
{
	unsigned long flag;
	long result = 0;
	u32 ack_value = 0;
	enum ipc_state *state = &data->ipc_state_ap;

	vts_dev_info(dev, "%s:[+%d][0x%x,0x%x,0x%x]\n",
		VTS_TAG_IPC, msg, (*values)[0],
		(*values)[1], (*values)[2]);

	/* Check VTS state before processing IPC,
	 * in VTS_STATE_RUNTIME_SUSPENDING state only Power Down IPC
	 * can be processed
	 */
	spin_lock_irqsave(&data->state_spinlock, flag);
	if ((data->vts_state == VTS_STATE_RUNTIME_SUSPENDING &&
		msg != VTS_IRQ_AP_POWER_DOWN) ||
		data->vts_state == VTS_STATE_RUNTIME_SUSPENDED ||
		data->vts_state == VTS_STATE_VOICECALL ||
		data->vts_state == VTS_STATE_NONE) {
		vts_dev_warn(dev, "%s: VTS IP %s state\n", __func__,
			(data->vts_state == VTS_STATE_VOICECALL ?
			"VoiceCall" : "Suspended"));
		spin_unlock_irqrestore(&data->state_spinlock, flag);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&data->state_spinlock, flag);

	if (pm_runtime_suspended(dev)) {
		vts_dev_warn(dev, "%s: VTS IP is in suspended state, IPC cann't be processed\n",
			VTS_TAG_IPC);
		return -EINVAL;
	}

	if (!data->vts_ready) {
		vts_dev_warn(dev, "%s: VTS Firmware Not running\n", VTS_TAG_IPC);
		return -EINVAL;
	}

	spin_lock(&data->ipc_spinlock);

	*state = SEND_MSG;
	vts_mailbox_write_shared_register(data->pdev_mailbox, *values, 0, 3);
	vts_mailbox_generate_interrupt(data->pdev_mailbox, msg);
	data->running_ipc = msg;

	if (sync) {
		result = vts_ipc_wait_for_ack(dev, msg, &ack_value);

		if (result < 0) {
			vts_dev_warn(dev, "%s: Failed to wait for ack, ack_value:0x%x\n",
					__func__, ack_value);
		}

		if (*state == SEND_MSG_OK || ack_value == (0x1 << msg)) {
			vts_dev_dbg(dev, "Transaction success Ack_value:0x%x\n", ack_value);

			if (ack_value == (0x1 << VTS_IRQ_AP_COMMAND)
			    && ((*values)[0] == VTS_ENABLE_DEBUGLOG)) {
				u32 ackvalues[3] = {0, 0, 0};

				vts_mailbox_read_shared_register(data->pdev_mailbox,
					ackvalues, 4, 2);
				vts_dev_info(dev, "%s: offset: 0x%x size:0x%x\n",
					VTS_TAG_IPC, ackvalues[0], ackvalues[1]);
				if ((*values)[0] == VTS_ENABLE_DEBUGLOG) {
					/* Register debug log buffer */
					vts_register_log_buffer(dev,
						ackvalues[0], ackvalues[1]);
					vts_dev_dbg(dev, "%s: Log buffer\n",
						VTS_TAG_IPC);
				}
			} else if (ack_value == (0x1 << VTS_IRQ_AP_GET_VERSION)) {
				u32 version;
				vts_mailbox_read_shared_register(data->pdev_mailbox,
					&version, 2, 1);
				vts_dev_dbg(dev, "google version(%d) : %d %d",
					__LINE__, version, data->google_version);
			}
		} else {
			vts_dev_err(dev, "Transaction failed\n");
		}
		result = (*state == SEND_MSG_OK || ack_value) ? 0 : -EIO;
	}

	/* Clear running IPC & ACK value */
	ack_value = 0x0;
	vts_mailbox_write_shared_register(data->pdev_mailbox,
						&ack_value, 3, 1);
	data->running_ipc = 0;
	*state = IDLE;

	spin_unlock(&data->ipc_spinlock);
	vts_dev_info(dev, "%s:[-%d]\n", VTS_TAG_IPC, msg);

	return (int)result;
}

int vts_start_ipc_transaction(struct device *dev, struct vts_data *data,
		int msg, u32 (*values)[3], int atomic, int sync)
{
	return vts_start_ipc_transaction_atomic(dev, data, msg, values, sync);
}
EXPORT_SYMBOL(vts_start_ipc_transaction);

int vts_send_ipc_ack(struct vts_data *data, u32 result) {
	struct device *dev = &data->pdev->dev;

	return vts_ipc_ack(dev, result);
}

int vts_mem_setup(struct device *dev, phys_addr_t *fw_phys_base,
		size_t *fw_bin_size, size_t *fw_mem_size) {
	struct vts_data *data = dev_get_drvdata(dev);

	vts_dev_info(dev, "%s: enter\n", __func__);

	if (!data) {
		vts_dev_err(dev, "vts data is null\n");
		return -EINVAL;
	}

	if (!data->firmware) {
		vts_dev_err(dev, "fw is not loaded\n");
		return -EINVAL;
	}

	memcpy(data->sram_base, data->firmware->data, data->firmware->size);
	*fw_phys_base = data->sram_phys;
	*fw_bin_size = data->firmware->size;
	*fw_mem_size = data->sram_size;
	vts_dev_info(dev, "fw is downloaded(size=%zu)\n",
		data->firmware->size);
	return 0;
}

int vts_verify_fw(struct device *dev, phys_addr_t fw_phys_base,
		size_t fw_bin_size, size_t fw_mem_size) {
	uint64_t ret64 = 0;

	if (!IS_ENABLED(CONFIG_EXYNOS_S2MPU)) {
		vts_dev_warn(dev, "%s: H-Arx is not enabled\n", __func__);
		return 0;
	}

	ret64 = exynos_verify_subsystem_fw("VTS", 0,
					fw_phys_base, fw_bin_size, fw_mem_size);
	vts_dev_info(dev, "%s: verify fw(size:%zu)\n", __func__, fw_bin_size);

	if (ret64) {
		vts_dev_warn(dev, "%s: Failed F/W verification, ret=%llu\n",
			__func__, ret64);
		return -EIO;
	}
	vts_cpu_power(dev, true);
	ret64 = exynos_request_fw_stage2_ap("VTS");
	if (ret64) {
		vts_dev_warn(dev, "%s: Failed F/W verification to S2MPU, ret=%llu\n",
			__func__, ret64);
		return -EIO;
	}
	return 0;
}

static int vts_wait_for_fw_ready(struct device *dev) {
	struct vts_data *data = dev_get_drvdata(dev);
	int result;

	result = wait_event_timeout(data->ipc_wait_queue, data->vts_ready, msecs_to_jiffies(3000));

	if (data->vts_ready) {
		result = 0;
	} else {
		vts_dev_err(dev, "%s: VTS Firmware is not ready\n", __func__);
		vts_dbg_dump_fw_gpr(dev, data, VTS_FW_NOT_READY);
		result = -ETIME;
	}

	return result;
}


bool vts_is_on(void) {
	vts_info("%s : %d\n", __func__, (p_vts_data && p_vts_data->enabled));

	return p_vts_data && p_vts_data->enabled;
}
EXPORT_SYMBOL(vts_is_on);

bool vts_is_running(void)
{
	return p_vts_data && p_vts_data->running;
}
EXPORT_SYMBOL(vts_is_running);

int vts_chk_dmic_clk_mode(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);
	int ret = 0;
	u32 values[3] = {0, 0, 0};

	vts_dev_info(dev, "%s: state(%d) tri_state(%d) rec_state(%d) clk_path(%d) micclk_init_cnt(%d)\n",
		__func__, data->vts_state, data->vts_tri_state, data->vts_rec_state, data->clk_path, data->micclk_init_cnt);

	/* already started recognization mode and dmic_if clk is 3.072MHz*/
	/* VTS worked + Serial LIF */
	if (data->clk_path == VTS_CLK_SRC_RCO)
		data->syssel_rate = DMIC_IF_SYS_SEL_VTS;
	else
		data->syssel_rate = DMIC_IF_SYS_SEL_AUD;

	if (data->vts_state == VTS_STATE_RECOG_STARTED) {
		/* restart VTS to update mic and clock setting */
		data->poll_event_type |= EVENT_RESTART|EVENT_READY;
		wake_up(&data->poll_wait_queue);
	}

	if (data->vts_tri_state == VTS_TRI_STATE_COPY_START || data->vts_rec_state == VTS_REC_STATE_START) {
		mutex_lock(&data->firmware_lock);
		if (data->vts_tri_state == VTS_TRI_STATE_COPY_START) {
			ret = vts_start_ipc_transaction(dev, data, VTS_IRQ_AP_STOP_COPY,
				&values, 1, 1);
			if (ret < 0)
				vts_dev_err(dev, "%s: vts ipc VTS_IRQ_AP_STOP_COPY failed: %d\n",
					__func__, ret);
		}
		if(data->vts_rec_state == VTS_REC_STATE_START) {
			ret = vts_start_ipc_transaction(dev, data, VTS_IRQ_AP_STOP_REC,
				&values, 1, 1);
			if (ret < 0)
				vts_dev_err(dev, "%s: vts ipc VTS_IRQ_AP_STOP_REC failed: %d\n",
					__func__, ret);
		}

		if (data->syssel_rate == DMIC_IF_SYS_SEL_VTS) {
			vts_clk_set_rate(dev, data->tri_clk_path);
		} else {
			vts_clk_set_rate(dev, data->aud_clk_path);
		}
		if (data->micclk_init_cnt)
			data->micclk_init_cnt--;
		ret = vts_set_dmicctrl(data->pdev, data->target_id, true);
		if (ret < 0)
			vts_dev_err(dev, "%s: MIC control failed\n", __func__);

		values[0] = VTS_MIC_INPUT_CH;
		values[1] = data->mic_input_ch;
		values[2] = 1;
		ret = vts_start_ipc_transaction(dev, data,
				VTS_IRQ_AP_COMMAND,
				&values, 0, 1);
		if (ret < 0) {
			vts_dev_err(dev, "%s: vts ipc VTS_IRQ_AP_COMMAND failed: %d\n",
				__func__, ret);
		} else {
			vts_dev_info(dev, "%s: sent mic_input_ch(%d)\n", __func__, data->mic_input_ch);
		}

		if (data->vts_tri_state == VTS_TRI_STATE_COPY_START) {
			vts_dev_info(dev, "%s: VTS Copying : Change SYS_SEL : %lu\n",
				__func__, data->syssel_rate);
			ret = vts_start_ipc_transaction(dev, data, VTS_IRQ_AP_START_COPY,
				&values, 1, 1);
		}
		if(data->vts_rec_state == VTS_REC_STATE_START) {
			vts_dev_info(dev, "%s: VTS Recording : Change SYS_SEL : %lu\n",
				__func__, data->syssel_rate);
			ret = vts_start_ipc_transaction(dev, data, VTS_IRQ_AP_START_REC,
				&values, 1, 1);
		}

		mutex_unlock(&data->firmware_lock);
	}
	return ret;
}
EXPORT_SYMBOL(vts_chk_dmic_clk_mode);

int vts_set_dmicctrl(struct platform_device *pdev, int micconf_type, bool enable)
{
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	int ctrl_dmicif = 0;
	int select_dmicclk = 0;
	u32 values[3] = {0, 0, 0};
	int result;

	vts_dev_dbg(dev, "%s-- flag: %d mictype: %d micusagecnt: %d\n",
			 __func__, enable, micconf_type, data->micclk_init_cnt);

	if (!data->vts_ready) {
		vts_dev_warn(dev, "%s: VTS Firmware Not running\n", __func__);
		return -EINVAL;
	}

	if (enable) {
		values[0] = (u32)data->is_bargein;
		values[1] = (u32)data->bargein_gain;
		values[2] = 0;
		result = vts_start_ipc_transaction(dev, data,
				VTS_IRQ_AP_SET_BARGEIN_MODE,
				&values, 0, 1);
		if (result < 0) {
			vts_dev_err(dev, "%s: vts ipc VTS_IRQ_AP_SET_BARGEIN_MODE failed: %d\n",
				__func__, result);
		}
	}

	if (data->is_bargein) {
		vts_dev_info(dev, "%s: It's bargein mode. Skip turning on/off the mic\n",
				__func__);

		return 0;
	}

	if (enable) {
		if (!data->micclk_init_cnt) {
			ctrl_dmicif = readl(data->dmic_if0_base + VTS_DMIC_CONTROL_DMIC_IF);

			/* s5e9925, 8825, 9935 sets below configuration,
			   so don't need condition to set for now */
			clear_bits(ctrl_dmicif, 7, VTS_DMIC_SYS_SEL_OFFSET);
			set_bits(ctrl_dmicif, data->syssel_rate, VTS_DMIC_SYS_SEL_OFFSET);
			vts_dev_info(dev, "%s DMIC IF SYS_SEL : %lu\n", 	__func__, data->syssel_rate);

			if (data->dmic_if == APDM) {
				vts_port_cfg(dev, VTS_PORT_PAD0,
						VTS_PORT_ID_VTS, APDM, true);
				set_bit(select_dmicclk,
					VTS_ENABLE_CLK_GEN_OFFSET);
				set_bit(select_dmicclk,
					VTS_SEL_EXT_DMIC_CLK_OFFSET);
				set_bit(select_dmicclk,
					VTS_ENABLE_CLK_CLK_GEN_OFFSET);

				/* Set AMIC VTS Gain */
				set_bits(ctrl_dmicif, data->dmic_if,
					VTS_DMIC_DMIC_SEL_OFFSET);
				set_bits(ctrl_dmicif, data->amicgain,
					VTS_DMIC_GAIN_OFFSET);
			} else {
				vts_port_cfg(dev, VTS_PORT_PAD0,
						VTS_PORT_ID_VTS, DPDM, true);

				if (data->supported_mic_num == MIC_NUM2)
					vts_port_cfg(dev, VTS_PORT_PAD1, VTS_PORT_ID_VTS, DPDM, true);

				clear_bit(select_dmicclk,
					VTS_ENABLE_CLK_GEN_OFFSET);
				clear_bit(select_dmicclk,
					VTS_SEL_EXT_DMIC_CLK_OFFSET);
				clear_bit(select_dmicclk,
					VTS_ENABLE_CLK_CLK_GEN_OFFSET);

				/* Set DMIC VTS Gain */
				set_bits(ctrl_dmicif, data->dmic_if,
					VTS_DMIC_DMIC_SEL_OFFSET);
				set_bits(ctrl_dmicif,
					data->dmic_gain[0], VTS_DMIC_GAIN_OFFSET);
				set_bits(ctrl_dmicif,
					data->dmic_1db_gain[0], VTS_DMIC_1DB_GAIN_OFFSET);
			}
			writel(select_dmicclk,
				data->sfr_base + DMIC_CLK_CON);

			writel(ctrl_dmicif,
				data->dmic_if0_base + VTS_DMIC_CONTROL_DMIC_IF);

			if (data->supported_mic_num == MIC_NUM2) {
				set_bits(ctrl_dmicif, data->dmic_gain[1], VTS_DMIC_GAIN_OFFSET);
				set_bits(ctrl_dmicif, data->dmic_1db_gain[1], VTS_DMIC_1DB_GAIN_OFFSET);
 				writel(ctrl_dmicif, data->dmic_if1_base + VTS_DMIC_CONTROL_DMIC_IF);
			}

		}

		/* check whether Mic is already configure or not based on VTS
		 * option type for MIC configuration book keeping
		 */
		if (!(data->mic_ready & (0x1 << micconf_type))) {
			data->micclk_init_cnt++;
			data->mic_ready |= (0x1 << micconf_type);
			vts_dev_info(dev, "%s Micclk ENABLED for mic_ready ++ %d\n",
				 __func__, data->mic_ready);
		}
	} else {
		if (data->micclk_init_cnt)
			data->micclk_init_cnt--;

		if (!data->micclk_init_cnt) {
			if (data->clk_path != VTS_CLK_SRC_AUD0) {
				vts_port_cfg(dev, VTS_PORT_PAD0,
						VTS_PORT_ID_VTS, DPDM, false);

				if (data->supported_mic_num == MIC_NUM2)
					vts_port_cfg(dev, VTS_PORT_PAD1, VTS_PORT_ID_VTS, DPDM, false);
			}

			vts_dev_info(dev, "%s Micclk setting DISABLED\n",
				__func__);
		}

		/* MIC configuration book keeping */
		if (data->mic_ready & (0x1 << micconf_type)) {
			data->mic_ready &= ~(0x1 << micconf_type);
			vts_dev_info(dev, "%s Micclk DISABLED for mic_ready -- %d\n",
				 __func__, data->mic_ready);
		}
	}

	return 0;
}
EXPORT_SYMBOL(vts_set_dmicctrl);

#if defined(CONFIG_VTS_FG_MEMP_CTRL)
void vts_set_fg_mem_ctrl(struct platform_device *pdev, bool enable)
{
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	unsigned int ctrl_intmem = 0;
	unsigned int ctrl_cmu_vts = 0;

	vts_dev_info(dev, "[vts] %s: %d \n", __func__, enable);

	if(enable) {
		/* INTMEM */
		ctrl_intmem = readl(data->intmem_pcm + FG_MEMP_CON);
		set_bits(ctrl_intmem, FG_MEMP_CON_EN_SIZE, FG_MEMP_CON_EN_OFFSET);
		writel(ctrl_intmem, data->intmem_pcm + FG_MEMP_CON);
		ctrl_intmem = readl(data->intmem_code+ FG_MEMP_CON);
		set_bits(ctrl_intmem, FG_MEMP_CON_EN_SIZE, FG_MEMP_CON_EN_OFFSET);
		writel(ctrl_intmem, data->intmem_code + FG_MEMP_CON);
		ctrl_intmem = readl(data->intmem_data+ FG_MEMP_CON);
		set_bits(ctrl_intmem, FG_MEMP_CON_EN_SIZE, FG_MEMP_CON_EN_OFFSET);
		writel(ctrl_intmem, data->intmem_data + FG_MEMP_CON);
		ctrl_intmem = readl(data->intmem_data1+ FG_MEMP_CON);
		set_bits(ctrl_intmem, FG_MEMP_CON_EN_SIZE, FG_MEMP_CON_EN_OFFSET);
		writel(ctrl_intmem, data->intmem_data1 + FG_MEMP_CON);

		/* CMU_VTS */
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_CODE_0);
		set_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_CODE_0);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_3);
		set_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_3);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_4);
		set_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_4);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_5);
		set_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_5);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_6);
		set_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_6);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_7);
		set_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_7);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_PCM_1);
		set_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_PCM_1);
	} else {
		/* INTMEM */
		ctrl_intmem = readl(data->intmem_pcm + FG_MEMP_CON);
		clear_bits(ctrl_intmem, FG_MEMP_CON_EN_SIZE, FG_MEMP_CON_EN_OFFSET);
		writel(ctrl_intmem, data->intmem_pcm + FG_MEMP_CON);
		ctrl_intmem = readl(data->intmem_code+ FG_MEMP_CON);
		clear_bits(ctrl_intmem, FG_MEMP_CON_EN_SIZE, FG_MEMP_CON_EN_OFFSET);
		writel(ctrl_intmem, data->intmem_code + FG_MEMP_CON);
		ctrl_intmem = readl(data->intmem_data+ FG_MEMP_CON);
		clear_bits(ctrl_intmem, FG_MEMP_CON_EN_SIZE, FG_MEMP_CON_EN_OFFSET);
		writel(ctrl_intmem, data->intmem_data + FG_MEMP_CON);
		ctrl_intmem = readl(data->intmem_data1+ FG_MEMP_CON);
		clear_bits(ctrl_intmem, FG_MEMP_CON_EN_SIZE, FG_MEMP_CON_EN_OFFSET);
		writel(ctrl_intmem, data->intmem_data1 + FG_MEMP_CON);

		/* CMU_VTS */
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_CODE_0);
		clear_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_CODE_0);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_3);
		clear_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_3);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_4);
		clear_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_4);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_5);
		clear_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_5);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_6);
		clear_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_6);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_DATA0_7);
		clear_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_DATA0_7);
		ctrl_cmu_vts = readl(data->cmu_vts + MEMPG_CON_INTMEM_PCM_1);
		clear_bits(ctrl_cmu_vts, USE_PG_COUNTER_SIZE, USE_PG_COUNTER_OFFSET);
		writel(ctrl_cmu_vts, data->cmu_vts + MEMPG_CON_INTMEM_PCM_1);
	}
}
EXPORT_SYMBOL(vts_set_fg_mem_ctrl);
#endif

static void vts_update_kernel_time(struct device *dev) {
	struct vts_data *data = dev_get_drvdata(dev);
	unsigned long long kernel_time = sched_clock();

	data->shared_info->kernel_sec = (u32)(kernel_time / 1000000000);
	data->shared_info->kernel_msec = (u32)(kernel_time % 1000000000 / 1000000);
}

static irqreturn_t vts_error_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	u32 error_cfsr, error_hfsr, error_dfsr, error_afsr, error_bfsr;
	u32 error_code;

	vts_mailbox_read_shared_register(data->pdev_mailbox,
						&error_code, 3, 1);

	switch (error_code) {
	case VTS_ERR_HARD_FAULT:
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_cfsr, 1, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_hfsr, 2, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_dfsr, 4, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_afsr, 5, 1);
		vts_dev_err(dev, "HardFault CFSR 0x%x\n", (int)error_cfsr);
		vts_dev_err(dev, "HardFault HFSR 0x%x\n", (int)error_hfsr);
		vts_dev_err(dev, "HardFault DFSR 0x%x\n", (int)error_dfsr);
		vts_dev_err(dev, "HardFault AFSR 0x%x\n", (int)error_afsr);

		break;
	case VTS_ERR_BUS_FAULT:
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_cfsr, 1, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_hfsr, 2, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_dfsr, 4, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_bfsr, 5, 1);
		vts_dev_err(dev, "BusFault CFSR 0x%x\n", (int)error_cfsr);
		vts_dev_err(dev, "BusFault HFSR 0x%x\n", (int)error_hfsr);
		vts_dev_err(dev, "BusFault DFSR 0x%x\n", (int)error_dfsr);
		vts_dev_err(dev, "BusFault BFSR 0x%x\n", (int)error_bfsr);

		break;
	default:
		vts_dev_warn(dev, "undefined error_code: %d\n", error_code);

		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_cfsr, 1, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_hfsr, 2, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_dfsr, 4, 1);
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							&error_afsr, 5, 1);
		vts_dev_err(dev, "0x%x 0x%x 0x%x 0x%x\n",
				(int)error_cfsr, (int)error_hfsr,
				(int)error_dfsr, (int)error_afsr);
		break;
	}
	vts_ipc_ack(dev, 1);
	vts_dev_err(dev, "Error occurred on VTS: 0x%x\n", (int)error_code);
#ifdef VTS_SICD_CHECK
	vts_dev_err(dev, "SOC down : %d, MIF down : %d",
		readl(data->sicd_base + SICD_SOC_DOWN_OFFSET),
		readl(data->sicd_base + SICD_MIF_DOWN_OFFSET));
#endif
	vts_print_fw_status(dev, __func__);

	/* Dump VTS GPR register & SRAM */
	vts_dbg_dump_fw_gpr(dev, data, VTS_FW_ERROR);

	switch (error_code) {
	case VTS_ERR_HARD_FAULT:
	case VTS_ERR_BUS_FAULT:
		if (data->silent_reset_support) {
			data->vts_status = ABNORMAL;
			data->recovery_try_cnt++;
			vts_disable_fw_ctrl(dev);
			vts_dev_info(dev, "%s: try to recover vts(%d)\n",
				__func__, data->recovery_try_cnt);
			data->poll_event_type |= EVENT_ERROR_RECOVERY|EVENT_READY;
			wake_up(&data->poll_wait_queue);
			__pm_wakeup_event(data->wake_lock, VTS_TRIGGERED_TIMEOUT_MS);
		} else {
			dbg_snapshot_expire_watchdog();
		}
		break;
	default:
		vts_reset_cpu(dev);
		break;
	}

	return IRQ_HANDLED;
}

static irqreturn_t vts_boot_completed_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	uint32_t ret = 0;

	data->vts_ready = 1;

	vts_ipc_ack(dev, 1);
	wake_up(&data->ipc_wait_queue);

	vts_pcm_dump_reinit(PCM_DUMP_NODE_REC);
	vts_pcm_dump_reinit(PCM_DUMP_NODE_TRI);
	vts_pcm_dump_reinit(PCM_DUMP_NODE_INTERNAL);
	vts_pcm_dump_reinit(PCM_DUMP_NODE_NS_L);
	vts_pcm_dump_reinit(PCM_DUMP_NODE_NS_R);

	if (data->silent_reset_support == 0)
		goto exit;

	ret = vts_cpu_power_chk(dev);
	if (ret == 1) {
		/* If VTS CPU power is on normally, recovery_cnt is 0. */
		data->recovery_try_cnt = 0;
		data->vts_status = NORMAL;
		data->p_dump.fw_dump->sram_magic = 0;
	} else {
		vts_dev_err(dev, "%s: vts status is %x\n", __func__, ret);
	}

exit:
	vts_dev_info(dev, "%s: VTS boot completed\n", __func__);

	return IRQ_HANDLED;
}
#if 0  /* Don't need this as driver checks ipc received in other way */
static irqreturn_t vts_ipc_received_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	u32 ret;

	vts_mailbox_read_shared_register(data->pdev_mailbox, &ret, 3, 1);
	vts_dev_info(dev, "VTS received IPC: 0x%x\n", ret);

	switch (data->ipc_state_ap) {
	case SEND_MSG:
		if (ret  == (0x1 << data->running_ipc)) {
			vts_dev_dbg(dev, "IPC transaction completed\n");
			data->ipc_state_ap = SEND_MSG_OK;
		} else {
			vts_dev_err(dev, "IPC transaction error\n");
			data->ipc_state_ap = SEND_MSG_FAIL;
		}
		break;
	default:
		vts_dev_warn(dev, "State fault: %d Ack_value:0x%x\n",
				data->ipc_state_ap, ret);
		break;
	}

	return IRQ_HANDLED;
}
#endif
static irqreturn_t vts_voice_triggered_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	u32 id[2], score, frame_count;

	vts_dev_info(dev, "%s: enter\n", __func__);

	vts_mailbox_read_shared_register(data->pdev_mailbox, &id[0], 3, 2);
	vts_ipc_ack(dev, 1);

	frame_count = (u32)(id[0] & GENMASK(15, 0));
	score = (u32)((id[0] & GENMASK(27, 16)) >> 16);
	id[0] >>= 28;
	data->kw_length = id[1];

	if ((data->mic_ready & (1 << id[0])) || data->is_bargein) {
		vts_dev_info(dev, "VTS triggered: id = %u, score = %u\n",
				id[0], score);
		vts_dev_info(dev, "VTS triggered: frame_count = %u, kw_length: %u\n",
				 frame_count, data->kw_length);

		/* trigger event should be sent with triggered id */
		data->poll_event_type |= (EVENT_TRIGGERED|EVENT_READY) + id[0];
		wake_up(&data->poll_wait_queue);
		__pm_wakeup_event(data->wake_lock, VTS_TRIGGERED_TIMEOUT_MS);
		data->vts_state = VTS_STATE_RECOG_TRIGGERED;
	}

	return IRQ_HANDLED;
}

static irqreturn_t vts_trigger_period_elapsed_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	struct vts_dma_data *dma_data =
		platform_get_drvdata(data->pdev_vtsdma[VTS_TRIGGRE]);
	u32 pointer;

	if ((data->mic_ready && (data->mic_ready & ~(1 << VTS_MICCONF_FOR_RECORD))) || data->is_bargein) {
		vts_mailbox_read_shared_register(data->pdev_mailbox,
					 &pointer, 2, 1);
		vts_dev_dbg(dev, "%s:[%s] Base: %08x pointer:%08x\n",
			 __func__, (dma_data->id ? "VTS-RECORD" :
			 "VTS-TRIGGER"), data->dma_area_vts + DMA_BUF_TRI_OFFSET, pointer);

		if (pointer)
			dma_data->pointer = (pointer -
					 (data->dma_area_vts + DMA_BUF_TRI_OFFSET));
		vts_ipc_ack(dev, 1);
#ifdef DEF_VTS_PCM_DUMP
		if (vts_pcm_dump_get_file_started(PCM_DUMP_NODE_TRI))
			vts_pcm_dump_period_elapsed(PCM_DUMP_NODE_TRI, dma_data->pointer);
#endif
		snd_pcm_period_elapsed(dma_data->substream);
	}

	return IRQ_HANDLED;
}

static irqreturn_t vts_record_period_elapsed_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	struct vts_dma_data *dma_rec =
		platform_get_drvdata(data->pdev_vtsdma[VTS_RECORD]);
	struct vts_dma_data *data_ns_l =
		platform_get_drvdata(data->pdev_vtsdma[VTS_NS_L]);
	struct vts_dma_data *data_ns_r =
		platform_get_drvdata(data->pdev_vtsdma[VTS_NS_R]);
	u32 base_addr_rec = data->dma_area_vts + DMA_BUF_REC_OFFSET;
	u32 base_addr_l   = data->dma_area_vts + DMA_BUF_NS_L_OFFSET;
	u32 base_addr_r   = data->dma_area_vts + DMA_BUF_NS_R_OFFSET;
	u32 pointer[3];

	if (data->vts_state == VTS_STATE_RUNTIME_SUSPENDING ||
		data->vts_state == VTS_STATE_RUNTIME_SUSPENDED ||
		data->vts_state == VTS_STATE_NONE) {
		vts_dev_warn(dev, "%s: VTS wrong state\n", __func__);
		return IRQ_HANDLED;
	}

	if ((data->mic_ready & (0x1 << VTS_MICCONF_FOR_RECORD)) || data->is_bargein) {
		vts_mailbox_read_shared_register(data->pdev_mailbox,
						 &pointer[0], 1, 1);

		/*pointer[1] and pointer[2] is use for NS pcm dump*/
		pointer[1] = data->shared_info->ns_l_dump_adr;
		pointer[2] = data->shared_info->ns_r_dump_adr;

		vts_dev_dbg(dev, "%s: 0: 0x%x, 1:0x%x, 2:0x%x\n",
			 __func__,
			 pointer[0], pointer[1], pointer[2]);

		if (pointer[0])
			dma_rec->pointer = pointer[0] - base_addr_rec;
		if (pointer[1])
			data_ns_l->pointer = pointer[1] - base_addr_l;
		if (pointer[2])
			data_ns_r->pointer = pointer[2] - base_addr_r;

		vts_ipc_ack(dev, 1);
#ifdef DEF_VTS_PCM_DUMP
		if (vts_pcm_dump_get_file_started(PCM_DUMP_NODE_REC))
			vts_pcm_dump_period_elapsed(PCM_DUMP_NODE_REC, dma_rec->pointer);
	#ifdef DEF_NS_PCM_DUMP
		if (pointer[1] && vts_pcm_dump_get_file_started(PCM_DUMP_NODE_NS_L))
			vts_pcm_dump_period_elapsed(PCM_DUMP_NODE_NS_L, data_ns_l->pointer);
		if (pointer[2] && vts_pcm_dump_get_file_started(PCM_DUMP_NODE_NS_R))
			vts_pcm_dump_period_elapsed(PCM_DUMP_NODE_NS_R, data_ns_r->pointer);
	#endif
#endif
		snd_pcm_period_elapsed(dma_rec->substream);
	}

	return IRQ_HANDLED;
}

static irqreturn_t vts_debuglog_bufzero_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;

	if (!vts_is_running()) {
		vts_dev_err(dev, "%s: wrong status\n", __func__);
		return IRQ_HANDLED;
	}
	vts_dev_dbg(dev, "%s LogBuffer Index: %d\n", __func__, 0);

	/* schedule log dump */
	vts_log_schedule_flush(dev, 0);

	return IRQ_HANDLED;
}

static irqreturn_t vts_internal_rec_elapsed_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);
	u32 pointer = 0x0;
	u32 base_addr = data->dma_area_vts + DMA_BUF_INTERNAL_OFFSET;

	if (!data->running) {
		vts_dev_err(dev, "%s: wrong status\n", __func__);
		return IRQ_HANDLED;
	}

	pointer = data->shared_info->internel_dump_adr;

	if (pointer)
		pointer = pointer - base_addr;

	vts_ipc_ack(dev, 1);

	if (vts_pcm_dump_get_file_started(PCM_DUMP_NODE_INTERNAL))
		vts_pcm_dump_period_elapsed(PCM_DUMP_NODE_INTERNAL, pointer);

	return IRQ_HANDLED;
}

void vts_register_dma(struct platform_device *pdev_vts,
		struct platform_device *pdev_vts_dma, unsigned int id)
{
	struct vts_data *data = platform_get_drvdata(pdev_vts);

	if (id < ARRAY_SIZE(data->pdev_vtsdma)) {
		data->pdev_vtsdma[id] = pdev_vts_dma;
		if (id > data->vtsdma_count)
			data->vtsdma_count = id + 1;
		vts_dev_info(&data->pdev->dev, "%s: VTS-DMA id(%u)Registered\n",
			__func__, id);
	} else {
		vts_dev_err(&data->pdev->dev, "%s: invalid id(%u)\n",
			__func__, id);
	}
}
EXPORT_SYMBOL(vts_register_dma);

static int vts_suspend(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);
	u32 values[3] = {0, 0, 0};
	int result = 0;

	if (data->vts_ready) {
		if (vts_is_running() &&
			data->vts_state == VTS_STATE_RECOG_TRIGGERED) {
			result = vts_start_ipc_transaction(dev, data,
					VTS_IRQ_AP_RESTART_RECOGNITION,
					&values, 0, 1);
			if (result < 0) {
				vts_dev_err(dev, "%s restarted trigger failed\n",
					__func__);
				goto error_ipc;
			}
			data->vts_state = VTS_STATE_RECOG_STARTED;
		}

		if (vts_is_running()) {
			values[0] = VTS_AP_SUSPEND;
			result = vts_start_ipc_transaction(dev, data,
					VTS_IRQ_AP_COMMAND,
					&values, 0, 1);
			if (result < 0)
				vts_dev_err(dev, "%s send vts suspend failed\n", __func__);
		}

		/* enable vts wakeup source interrupts */
		enable_irq_wake(data->irq[VTS_IRQ_VTS_VOICE_TRIGGERED]);
		enable_irq_wake(data->irq[VTS_IRQ_VTS_ERROR]);
#ifdef TEST_WAKEUP
		enable_irq_wake(data->irq[VTS_IRQ_VTS_CP_WAKEUP]);
#endif
		vts_print_fw_status(dev, __func__);

		vts_dev_info(dev, "%s: Enable VTS Wakeup source irqs\n", __func__);
	}
	vts_dev_info(dev, "%s: TEST\n", __func__);
error_ipc:
	return result;
}

static int vts_resume(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);
	u32 values[3] = {0, 0, 0};
	int result = 0;

	if (data->vts_ready) {
		/* disable vts wakeup source interrupts */
		vts_print_fw_status(dev, __func__);

		if (vts_is_running()) {
			values[0] = VTS_AP_RESUME;
			result = vts_start_ipc_transaction(dev, data,
					VTS_IRQ_AP_COMMAND,
					&values, 0, 1);
			if (result < 0)
				vts_dev_err(dev, "%s send vts resume failed\n", __func__);
		}

		disable_irq_wake(data->irq[VTS_IRQ_VTS_VOICE_TRIGGERED]);
		disable_irq_wake(data->irq[VTS_IRQ_VTS_ERROR]);
#ifdef TEST_WAKEUP
		disable_irq_wake(data->irq[VTS_IRQ_VTS_CP_WAKEUP]);
#endif
		vts_dev_info(dev, "%s: Disable VTS Wakeup source irqs\n", __func__);
	}
	return 0;
}

static void vts_irq_enable(struct platform_device *pdev, bool enable)
{
	struct vts_data *data = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int irqidx;

	vts_dev_info(dev, "%s IRQ Enable: [%s]\n", __func__,
			(enable ? "TRUE" : "FALSE"));

	for (irqidx = 0; irqidx < VTS_IRQ_COUNT; irqidx++) {
		if (enable)
			enable_irq(data->irq[irqidx]);
		else
			disable_irq(data->irq[irqidx]);
	}
#ifdef TEST_WAKEUP
	if (enable)
		enable_irq(data->irq[VTS_IRQ_VTS_CP_WAKEUP]);
	else
		disable_irq(data->irq[VTS_IRQ_VTS_CP_WAKEUP]);
#endif
}

static void vts_save_register(struct vts_data *data)
{
	regcache_cache_only(data->regmap_dmic, true);
	regcache_mark_dirty(data->regmap_dmic);
}

static void vts_restore_register(struct vts_data *data)
{
	regcache_cache_only(data->regmap_dmic, false);
	regcache_sync(data->regmap_dmic);
}

static void vts_memlog_sync_to_file(struct device *dev, struct vts_data *data)
{
	if (data->kernel_log_obj)
		vts_dev_info(dev, "%s kernel_log_obj sync : %d",
			__func__, memlog_sync_to_file(data->kernel_log_obj));

	if (data->fw_log_obj)
		vts_dev_info(dev, "%s fw_log_obj sync : %d",
			__func__, memlog_sync_to_file(data->fw_log_obj));
}

void vts_dbg_dump_fw_gpr(struct device *dev, struct vts_data *data, unsigned int dbg_type)
{
	char *ver = (char *)(&data->vtsfw_version);
	struct vts_dbg_fw_dump *p_fw_dump;
	int i;

	vts_dev_dbg(dev, "%s: enter with type(%d)\n", __func__, dbg_type);
	vts_dev_info(dev,"%s: fw version: %c%c%c%c\n", __func__, ver[3], ver[2], ver[1], ver[0]);

	switch (dbg_type) {
	case RUNTIME_SUSPEND_DUMP:
		if (!vts_is_on() || !vts_is_running()) {
			vts_dev_info(dev, "%s is skipped due to no power\n",
				__func__);
			return;
		}
#if defined(CONFIG_SOC_S5E9925)
		if (data->dump_obj)
			memlog_do_dump(data->dump_obj, MEMLOG_LEVEL_INFO);
#endif
		vts_memlog_sync_to_file(dev, data);

		/* Save VTS firmware log msgs */
		if (!IS_ERR_OR_NULL(data->p_dump.sram_log)
		    && !IS_ERR_OR_NULL(data->sramlog_baseaddr)) {
			memcpy(data->p_dump.sram_log, VTS_LOG_DUMP_MAGIC,
				sizeof(VTS_LOG_DUMP_MAGIC));
			memcpy_fromio(data->p_dump.sram_log +	sizeof(VTS_LOG_DUMP_MAGIC),
				data->sramlog_baseaddr, VTS_SRAMLOG_SIZE_MAX);
		}
		break;
	case KERNEL_PANIC_DUMP:
	case VTS_ITMON_ERROR:
		if (!vts_is_running()) {
			vts_dev_info(dev, "%s is skipped due to not running\n",
				__func__);
			return;
		}
		fallthrough;
	case VTS_FW_NOT_READY:
	case VTS_IPC_TRANS_FAIL:
	case VTS_FW_ERROR:
		p_fw_dump = data->p_dump.fw_dump;
		if (!p_fw_dump) {
			vts_dev_warn(dev, "%s: memory is not allocated for firmware dump\n", __func__);
			break;
		}
#if defined(CONFIG_SOC_S5E9935) || defined(CONFIG_SOC_S5E8835) || defined(CONFIG_SOC_S5E9945) || defined(CONFIG_SOC_S5E8845) || defined(CONFIG_SOC_S5E9955) || defined(CONFIG_SOC_S5E8855)
		for (i = 0; i <= GPR_DUMP_CNT; i++) {
			vts_dev_info(dev, "R%d: %x\n", i, readl(data->gpr_base + VTS_CM_R(i)));
			data->p_dump.gpr[i] = readl(data->gpr_base + VTS_CM_R(i));
		}

		vts_dev_info(dev, "PC: %x\n", readl(data->gpr_base + VTS_CM_PC));
		data->p_dump.gpr[i++] = readl(data->gpr_base + VTS_CM_PC);
#else
		vts_dev_info(dev, "PC: 0x%x\n", readl(data->gpr_base + 0x0));
		data->p_dump.gpr[0] = readl(data->gpr_base + 0x0);
#endif
		if (data->dump_obj && (dbg_type == VTS_IPC_TRANS_FAIL || dbg_type == VTS_FW_ERROR)) {
			memlog_do_dump(data->dump_obj, MEMLOG_LEVEL_ERR);
			vts_memlog_sync_to_file(dev, data);
		}

		vts_dev_info(dev, "dump\n");

		/* Save VTS Sram all */
		p_fw_dump->sram_magic = VTS_SRAM_DUMP_MAGIC;
		memcpy_fromio(p_fw_dump->sram_dump, data->sram_base, VTS_SRAM_SZ);
		p_fw_dump->pcm_magic = VTS_SRAM_PCM_MAGIC;
		memcpy_fromio(p_fw_dump->pcm_dump, data->sram_base + data->pcm_buf_offset,
			VTS_PCM_BUF_SZ);

		/* log dump */
		print_hex_dump(KERN_ERR, "vts-fw-log", DUMP_PREFIX_OFFSET, 32, 4,
			data->sramlog_baseaddr, VTS_SRAM_EVENTLOG_SIZE_MAX, true);
		print_hex_dump(KERN_ERR, "vts-time-log", DUMP_PREFIX_OFFSET, 32, 4,
			data->sramlog_baseaddr + VTS_SRAM_EVENTLOG_SIZE_MAX,
				VTS_SRAM_TIMELOG_SIZE_MAX, true);
		break;
	default:
		vts_dev_info(dev, "%s is skipped due to invalid debug type\n",
			__func__);
		return;
	}
	data->p_dump.time = sched_clock();
	data->p_dump.dbg_type = dbg_type;
}

static void exynos_vts_panic_handler(void)
{
	struct vts_data *data = p_vts_data;
	struct device *dev =
		data ? (data->pdev ? &data->pdev->dev : NULL) : NULL;

	vts_dev_dbg(dev, "%s\n", __func__);

	if (vts_is_on() && dev) {
		/* Dump VTS GPR register & SRAM */
		vts_dbg_dump_fw_gpr(dev, data, KERNEL_PANIC_DUMP);
	} else {
		vts_dev_info(dev, "%s: dump is skipped due to no power\n",
			__func__);
	}
}

static int vts_panic_handler(struct notifier_block *nb,
			       unsigned long action, void *data)
{
	exynos_vts_panic_handler();
	return NOTIFY_OK;
}

static struct notifier_block vts_panic_notifier = {
	.notifier_call	= vts_panic_handler,
	.next		= NULL,
	.priority	= 0	/* priority: INT_MAX >= x >= 0 */
};

#define SLIF_SEL_VTS	(0x824) /* 0x15510824 */
static int vts_do_reg(struct device *dev, bool enable, int cmd) {
	struct vts_data *data = dev_get_drvdata(dev);
	int ret = 0;


	if (IS_ENABLED(CONFIG_SOC_S5E8825) || IS_ENABLED(CONFIG_SOC_S5E9935) || IS_ENABLED(CONFIG_SOC_S5E8835) || IS_ENABLED(CONFIG_SOC_S5E9945) || IS_ENABLED(CONFIG_SOC_S5E8845) || IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E8855)) {
		if (enable)
			writel(0x1, data->gpr_base);  /* enable DUMP_GPR */

		return ret;
	}

	if (!IS_ENABLED(CONFIG_SOC_S5E9925_EVT0))
		return ret;

	/* HACK */
	if (enable) {
		/* SYSREG_VTS + 0x824 */
		/*0 : SERIAL_LIF, 1 : DMIC_AHB */
		writel(0x0, data->sfr_base + SLIF_SEL_VTS);

		/* SL_INT_EN_SET : 0x040 */
		/* [9:8] Interrupt ENABLE SL_READ_POINTER_SET0 & */
		/* SL_READ_POINTER_SET1 */
		/* writel(0x300, data->sfr_slif_vts + 0x40); */
		/* INPUT_EN: EN0, EN1 */
		/* writel(0x3, data->sfr_slif_vts + 0x114); */
		/* CONFIG_MASTER: 16bit, 2ch */
		/* writel(0x00102009, data->sfr_slif_vts + 0x104); */
		/* CHANNEL_MAP: 0 1 0 1 0 1 0 1 */
		/* writel(0x20202020, data->sfr_slif_vts + 0x138); */
		/* writel(0x1111, data->sfr_slif_vts + 0x100); */
	} else {
		writel(0x0, data->sfr_base + SLIF_SEL_VTS);
		/* writel(0x0, data->sfr_slif_vts + 0x40); */
		/* writel(0x0, data->sfr_slif_vts + 0x114); */
		/* writel(0x00102009, data->sfr_slif_vts + 0x104); */
		/* writel(0x76543210, data->sfr_slif_vts + 0x138); */
		/* writel(0x0, data->sfr_slif_vts + 0x100); */

		/* writel(0x0, data->sfr_slif_vts + 0x100); */
		/* writel(0x0, data->sfr_base + 0x1000); */
	}

	return ret;
}

static void vts_update_config(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);
	int ret = 0;
	int sys_clk_index = data->sys_clk_path[0].index;

	/* update forcely, if you want */
	if (data->target_sysclk != data->sys_clk_path[0].value)
		data->sys_clk_path[0].value = data->target_sysclk;

	data->pcm_buf_offset = data->shared_info->pcm_buf_offset;
	vts_dev_info(dev, "%s: pcm buf offset(%x)\n", __func__, data->pcm_buf_offset);

	ret = vts_clk_set_rate(dev, data->sys_clk_path);
	if (ret < 0)
		vts_dev_err(dev, "failed set vts_sys_clk: %d\n", ret);

	vts_dev_dbg(dev, "System Clock target:%lu current:%lu\n",
			data->target_sysclk, clk_get_rate(data->clk_src_list[sys_clk_index].clk_src));
}

static void vts_check_version(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);
	u32 values[3];
	u32 ret_value[3];
	char *ver;
	int ret = 0;

	if (data->google_version == 0) {
		values[0] = 2;
		values[1] = 0;
		values[2] = 0;
		ret = vts_start_ipc_transaction(dev, data,
			VTS_IRQ_AP_GET_VERSION, &values, 0, 2);
		if (ret < 0)
			dev_err(dev, "VTS_IRQ_AP_GET_VERSION ipc failed\n");
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							ret_value, 0, 3);
		vts_dev_dbg(dev, "get version 0x%x 0x%x 0x%x\n",
			ret_value[0], ret_value[1], ret_value[2]);

		data->google_version = ret_value[2];
		memcpy(data->google_uuid, data->shared_info->hotword_id, 40);
	} else {
		vts_dev_dbg(dev, "google version = %d\n",
				data->google_version);
	}

	if (data->vtsdetectlib_version == 0) {
		values[0] = 1;
		values[1] = 0;
		values[2] = 0;

		ret = vts_start_ipc_transaction(dev, data,
			VTS_IRQ_AP_GET_VERSION, &values, 0, 2);
		if (ret < 0)
			dev_err(dev, "VTS_IRQ_AP_GET_VERSION ipc failed\n");
		vts_mailbox_read_shared_register(data->pdev_mailbox,
							ret_value, 0, 3);

		vts_dev_dbg(dev, "get version 0x%x 0x%x 0x%x\n",
			ret_value[0], ret_value[1], ret_value[2]);

		data->vtsfw_version = ret_value[0];
		data->vtsdetectlib_version = ret_value[2];
	}

	ver = (char *)(&data->vtsfw_version);
	vts_dev_info(dev, "firmware version: (%c%c%c%c) lib: 0x%x 0x%x\n",
		ver[3], ver[2], ver[1], ver[0],
		data->vtsdetectlib_version,
		data->google_version);
}

static int vts_runtime_suspend_locked(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct vts_data *data = dev_get_drvdata(dev);
	u32 values[3] = {0, 0, 0};
	int result = 0;
	unsigned long flag;

	vts_dev_info(dev, "%s\n", __func__);
	if (data->sysevent_dev)
		sysevent_put((void *)data->sysevent_dev);

	if (vts_is_running()) {
		vts_dev_info(dev, "RUNNING %s :%d\n", __func__, __LINE__);

		if (data->fw_logfile_enabled || data->fw_logger_enabled) {
			values[0] = VTS_DISABLE_DEBUGLOG;
			values[1] = 0;
			values[2] = 0;
			result = vts_start_ipc_transaction(dev, data,
					VTS_IRQ_AP_COMMAND,
					&values, 0, 1);
			if (result < 0)
				vts_dev_warn(dev, "Disable_debuglog ipc transaction failed\n");
			/* reset VTS SRAM debug log buffer */
			vts_register_log_buffer(dev, 0, 0);
		}
	}

	spin_lock_irqsave(&data->state_spinlock, flag);
	data->vts_state = VTS_STATE_RUNTIME_SUSPENDING;
	spin_unlock_irqrestore(&data->state_spinlock, flag);

	vts_dev_info(dev, "%s save register :%d\n", __func__, __LINE__);
	vts_save_register(data);
	if (vts_is_running()) {
		if (data->vts_status == ABNORMAL) {
			// to avoid spending time for meaningless waiting
			vts_dev_err(dev, "%s: skip POWER_DOWN ipc\n", __func__);
			goto skip_ipc;
		}

		values[0] = 0;
		values[1] = 0;
		values[2] = 0;
		result = vts_start_ipc_transaction(dev,
			data, VTS_IRQ_AP_POWER_DOWN, &values, 0, 1);
		if (result < 0) {
			vts_dev_warn(dev, "POWER_DOWN IPC transaction Failed\n");
			result = vts_start_ipc_transaction(dev,
				data, VTS_IRQ_AP_POWER_DOWN, &values, 0, 1);
			if (result < 0)
				vts_dev_warn(dev, "POWER_DOWN IPC transaction 2nd Failed\n");
		}

skip_ipc:
		/* Dump VTS GPR register & Log messages */
		vts_dbg_dump_fw_gpr(dev, data, RUNTIME_SUSPEND_DUMP);
		vts_print_fw_status(dev, __func__);

		vts_cpu_enable(dev, false);

		if (data->irq_state) {
			vts_irq_enable(pdev, false);
			data->irq_state = false;
		}

		vts_do_reg(dev, false, 0);

		result = vts_soc_runtime_suspend(dev);
		if (result < 0) {
			vts_dev_warn(dev, "vts_soc_rt_suspend %d\n", result);
		}

		vts_log_flush(&data->pdev->dev);

		spin_lock_irqsave(&data->state_spinlock, flag);
		data->vts_state = VTS_STATE_RUNTIME_SUSPENDED;
		spin_unlock_irqrestore(&data->state_spinlock, flag);

		vts_cpu_power(dev, false);
		data->running = false;
	} else {
		spin_lock_irqsave(&data->state_spinlock, flag);
		data->vts_state = VTS_STATE_RUNTIME_SUSPENDED;
		spin_unlock_irqrestore(&data->state_spinlock, flag);
	}

	vts_port_cfg(dev, VTS_PORT_PAD0, VTS_PORT_ID_VTS, DPDM, false);

	vts_mailbox_update_vts_is_on(false);
	data->enabled = false;
	data->exec_mode = VTS_RECOGNIZE_STOP;
	data->current_active_ids = 0;
	data->target_size = 0;
	/* reset micbias setting count */
	data->micclk_init_cnt = 0;
	data->mic_ready = 0;
	data->vts_ready = 0;

	vts_dev_info(dev, "%s Exit\n", __func__);
	return 0;
}

static int vts_runtime_suspend(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&data->firmware_boot_lock);
	ret = vts_runtime_suspend_locked(dev);
	mutex_unlock(&data->firmware_boot_lock);

	return ret;
}

int vts_start_runtime_resume_locked(struct device *dev, int skip_log)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct vts_data *data = dev_get_drvdata(dev);
	u32 values[3];
	int result;
	unsigned long long kernel_time;

	if (vts_is_running()) {
		if (!skip_log)
			vts_dev_info(dev, "SKIP %s\n", __func__);
		return 0;
	}
	vts_dev_info(dev, "%s\n", __func__);

	result = vts_soc_runtime_resume(dev);
	if (result < 0) {
		vts_dev_warn(dev, "vts_soc_rt_resume %d\n", result);
		goto error_clk;
	}

	data->vts_state = VTS_STATE_RUNTIME_RESUMING;
	data->enabled = true;
	vts_mailbox_update_vts_is_on(true);

	vts_restore_register(data);
	vts_port_cfg(dev, VTS_PORT_PAD0, VTS_PORT_ID_VTS, DPDM, true);
	vts_port_cfg(dev, VTS_PORT_PAD0, VTS_PORT_ID_VTS, DPDM, false);
	vts_pad_retention(false);

	if (!data->irq_state) {
		vts_irq_enable(pdev, true);
		data->irq_state = true;
	}

	phys_addr_t fw_phys_base = 0;
	size_t fw_bin_size = 0;
	size_t fw_mem_size = 0;

	result |= vts_mem_setup(dev, &fw_phys_base, &fw_bin_size, &fw_mem_size);

	result |= vts_verify_fw(dev, fw_phys_base, fw_bin_size, fw_mem_size);

	if (result < 0) {
		vts_dev_err(dev, "Failed to download firmware\n");
		data->enabled = false;
		vts_mailbox_update_vts_is_on(false);
		goto error_firmware;
	}

	vts_cpu_power(dev, true);

	vts_dev_info(dev, "GPR DUMP BASE Enable\n");

	result = vts_wait_for_fw_ready(dev);
	/* ADD FW READY STATUS */
	if (result < 0) {
		vts_dev_err(dev, "Failed to vts_wait_for_fw_ready\n");
		data->enabled = false;
		vts_mailbox_update_vts_is_on(false);
		goto error_firmware;
	}

	vts_update_config(dev);
	vts_check_version(dev);
	vts_do_reg(dev, true, 0);

	/* Configure select sys clock rate */
	if (data->syssel_rate == DMIC_IF_SYS_SEL_VTS) {
		vts_clk_set_rate(dev, data->tri_clk_path);
	} else {
		vts_clk_set_rate(dev, data->aud_clk_path);
	}

	data->dma_area_vts = vts_set_baaw(data->baaw_base,
			data->dmab.addr, DMA_BUF_BYTES_MAX);

	values[0] = data->dma_area_vts;
	values[1] = VTS_BUF_PERIOD_SIZE;
	values[2] = VTS_BUF_PERIOD_COUNT;
	result = vts_start_ipc_transaction(dev,
		data, VTS_IRQ_AP_SET_DRAM_BUFFER, &values, 0, 1);
	if (result < 0) {
		vts_dev_err(dev, "DRAM_BUFFER Setting IPC transaction Failed\n");
		goto error_firmware;
	}

	data->exec_mode = VTS_RECOGNIZE_STOP;

	values[0] = VTS_ENABLE_SRAM_LOG;
	values[1] = 0;
	values[2] = 0;
	result = vts_start_ipc_transaction(dev,
		data, VTS_IRQ_AP_COMMAND, &values, 0, 1);
	if (result < 0) {
		vts_dev_err(dev, "Enable_SRAM_log ipc transaction failed\n");
		goto error_firmware;
	}

	if (data->fw_logfile_enabled || data->fw_logger_enabled) {
		values[0] = VTS_ENABLE_DEBUGLOG;
		values[1] = 0;
		values[2] = 0;
		result = vts_start_ipc_transaction(dev,
			data, VTS_IRQ_AP_COMMAND, &values, 0, 1);
		if (result < 0) {
			vts_dev_err(dev, "Enable_debuglog ipc transaction failed\n");
			goto error_firmware;
		}
	}

	vts_update_kernel_time(dev);
	/* Send Kernel Time */
	kernel_time = sched_clock();
	values[0] = VTS_KERNEL_TIME;
	values[1] = (u32)(kernel_time / 1000000000);
	values[2] = (u32)(kernel_time % 1000000000 / 1000000);
	vts_dev_info(dev, "Time : %d.%d\n", values[1], values[2]);
	result = vts_start_ipc_transaction(dev, data,
		VTS_IRQ_AP_COMMAND, &values, 0, 2);
	if (result < 0)
		vts_dev_err(dev, "VTS_KERNEL_TIME ipc failed\n");
	vts_dev_dbg(dev, "%s DRAM-setting and VTS-Mode is completed\n",
		__func__);
	vts_dev_info(dev, "%s Exit\n", __func__);
	data->running = true;
	data->vts_state = VTS_STATE_RUNTIME_RESUMED;

	if(data->firmware) {
		release_firmware(data->firmware);
		data->firmware = NULL;
	}
	return 0;

error_firmware:
	vts_cpu_power(dev, false);
	vts_soc_runtime_suspend(dev);
	if(data->firmware) {
		release_firmware(data->firmware);
		data->firmware = NULL;
	}

error_clk:
	if (data->irq_state) {
		vts_irq_enable(pdev, false);
		data->irq_state = false;
	}
	data->running = false;
	data->enabled = false;
	vts_mailbox_update_vts_is_on(false);
	return 0;
}

int vts_start_runtime_resume(struct device *dev, int skip_log)
{
	struct vts_data *data = dev_get_drvdata(dev);
	int ret = 0;

	vts_dev_info(dev, "%s: target_id:0x%x\n", __func__, data->target_id);

	mutex_lock(&data->firmware_boot_lock);
	ret = vts_start_runtime_resume_locked(dev, skip_log);
	mutex_unlock(&data->firmware_boot_lock);

	return ret;
}
EXPORT_SYMBOL(vts_start_runtime_resume);

static int vts_runtime_resume(struct device *dev)
{
	struct vts_data *data = dev_get_drvdata(dev);
	void *retval = NULL;

	if (IS_ENABLED(CONFIG_SOC_EXYNOS2100)) {
		cmu_vts_rco_400_control(1);
		vts_dev_info(dev, "%s RCO400 ON\n", __func__);
	}
	if (data->sysevent_dev) {
		retval = sysevent_get(data->sysevent_desc.name);
		if (!retval)
			vts_dev_err(dev, "fail in sysevent_get\n");
	}
	data->enabled = true;
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static const struct dev_pm_ops samsung_vts_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(vts_suspend, vts_resume)
	SET_RUNTIME_PM_OPS(vts_runtime_suspend, vts_runtime_resume, NULL)
};

static const struct of_device_id exynos_vts_of_match[] = {
	{
		.compatible = "samsung,vts",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_vts_of_match);

static ssize_t vtsfw_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vts_data *data = dev_get_drvdata(dev);
	unsigned int version = data->vtsfw_version;

	buf[0] = ((version >> 24) & 0xFF);
	buf[1] = ((version >> 16) & 0xFF);
	buf[2] = ((version >> 8) & 0xFF);
	buf[3] = (version & 0xFF);
	buf[4] = '\n';
	buf[5] = '\0';

	return 6;
}

static ssize_t vtsdetectlib_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct vts_data *data = dev_get_drvdata(dev);
	unsigned int version = data->vtsdetectlib_version;

	buf[0] = (((version >> 24) & 0xFF) + '0');
	buf[1] = '.';
	buf[2] = (((version >> 16) & 0xFF) + '0');
	buf[3] = '.';
	buf[4] = ((version & 0xFF) + '0');
	buf[5] = '\n';
	buf[6] = '\0';

	return 7;
}

static ssize_t vts_cmd_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	static const char cmd_test_fwlog[] = "TESTFWLOG";
	struct vts_data *data = dev_get_drvdata(dev);

	vts_dev_info(dev, "%s: (%s)\n", __func__, buf);

	if (!strncmp(cmd_test_fwlog, buf, sizeof(cmd_test_fwlog) - 1)) {
               vts_dev_info(dev, "%s: %s\n", __func__, buf);

               if (vts_is_on()) {
			/* log dump */
			print_hex_dump(KERN_ERR, "vts-fw-log",
				DUMP_PREFIX_OFFSET, 32, 4, data->sramlog_baseaddr,
				VTS_SRAM_EVENTLOG_SIZE_MAX, true);

			print_hex_dump(KERN_ERR, "vts-time-log",
				DUMP_PREFIX_OFFSET, 32, 4,
				data->sramlog_baseaddr + VTS_SRAM_EVENTLOG_SIZE_MAX,
				VTS_SRAM_TIMELOG_SIZE_MAX, true);
               } else {
                       vts_dev_info(dev, "%s: no power\n", __func__);
               }
       }

	return count;
}

#if defined(CONFIG_VTS_SFR_DUMP)
static ssize_t vts_sfr_dump_show(struct device *dev,
               struct device_attribute *attr, char *buf)
{
	if(vts_is_on()) {
		return vts_soc_sfr_dump_show(dev, attr, buf);
	} else {
		char *pbuf = buf;
		pbuf += sprintf(pbuf, "Warning: VTS SFR registers only can dump when vts is on.\n");

		return pbuf - buf;
	}
}

static ssize_t vts_dmic_if_dump_show(struct device *dev,
               struct device_attribute *attr, char *buf)
{
	if(vts_is_on()) {
		return vts_soc_dmic_if_dump_show(dev, attr, buf);
	} else {
		char *pbuf = buf;
		pbuf += sprintf(pbuf, "Warning: VTS Dmic_if registers only can dump when vts is on.\n");

		return pbuf - buf;
	}
}

#if defined(CONFIG_SOC_S5E9925) || defined(CONFIG_SOC_S5E9935) || defined(CONFIG_SOC_S5E9945) || defined(CONFIG_SOC_S5E9955)
static ssize_t vts_slif_dump_show(struct device *dev,
               struct device_attribute *attr, char *buf)
{
	if(vts_is_on()) {
		return vts_soc_slif_dump_show(dev, attr, buf);
	} else {
		char *pbuf = buf;
		pbuf += sprintf(pbuf, "Warning: VTS Slif registers only can dump when vts is on.\n");

		return pbuf - buf;
	}
}
#endif

static ssize_t vts_gpio_dump_show(struct device *dev,
               struct device_attribute *attr, char *buf)
{
	if(vts_is_on()) {
		return vts_soc_gpio_dump_show(dev, attr, buf);
	} else {
		char *pbuf = buf;
		pbuf += sprintf(pbuf, "Warning: VTS GPIO registers only can dump when vts is on.\n");

		return pbuf - buf;
	}
}
#endif

static DEVICE_ATTR_RO(vtsfw_version);
static DEVICE_ATTR_RO(vtsdetectlib_version);
static DEVICE_ATTR_WO(vts_cmd);

#if defined(CONFIG_VTS_SFR_DUMP)
static DEVICE_ATTR_RO(vts_sfr_dump);
static DEVICE_ATTR_RO(vts_dmic_if_dump);
#if defined(CONFIG_SOC_S5E9925) || defined(CONFIG_SOC_S5E9935) || defined(CONFIG_SOC_S5E9945) || defined(CONFIG_SOC_S5E9955)
static DEVICE_ATTR_RO(vts_slif_dump);
#endif
static DEVICE_ATTR_RO(vts_gpio_dump);
#endif

static void __iomem *samsung_vts_membase_request_and_map(
	struct platform_device *pdev, const char *name, const char *size)
{
	const __be32 *prop;
	unsigned int len = 0;
	unsigned int data_base = 0;
	unsigned int data_size = 0;
	void __iomem *result = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	prop = of_get_property(np, name, &len);
	if (!prop) {
		vts_dev_err(&pdev->dev, "Failed to of_get_property %s\n", name);
		return ERR_PTR(-EFAULT);
	}
	data_base = be32_to_cpup(prop);
	prop = of_get_property(np, size, &len);
	if (!prop) {
		vts_dev_err(&pdev->dev, "Failed to of_get_property %s\n", size);
		return ERR_PTR(-EFAULT);
	}
	data_size = be32_to_cpup(prop);
	if (data_base && data_size) {
		result = ioremap(data_base, data_size);
		if (IS_ERR_OR_NULL(result)) {
			vts_dev_err(&pdev->dev, "Failed to map %s\n", name);
			return ERR_PTR(-EFAULT);
		}
	}
	return result;
}

static const struct reg_default vts_dmic_reg_defaults[] = {
	{0x0000, 0x00030000},
	{0x0004, 0x00000000},
};

static const struct regmap_config vts_component_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = VTS_DMIC_CONTROL_DMIC_IF,
	.reg_defaults = vts_dmic_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(vts_dmic_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
	.fast_io = true,
};

static int vts_memlog_file_completed(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int vts_memlog_status_notify(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int vts_memlog_level_notify(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int vts_memlog_enable_notify(struct memlog_obj *obj, u32 flags)
{
	struct vts_data *data = p_vts_data;
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	u32 values[3] = {0, 0, 0};
	int result = 0;

	if (data->kernel_log_obj)
		vts_dev_info(dev, "vts kernel_log_obj enable : %d",
			data->kernel_log_obj->enabled);

	if (data->fw_log_obj) {
		data->fw_logger_enabled = data->fw_log_obj->enabled;
		vts_dev_info(dev, "firmware logger enabled : %d",
			data->fw_logger_enabled);
		if (data->vts_ready) {
			if (data->fw_logger_enabled)
				values[0] = VTS_ENABLE_DEBUGLOG;
			else {
				values[0] = VTS_DISABLE_DEBUGLOG;
				vts_register_log_buffer(dev, 0, 0);
			}
			values[1] = 0;
			values[2] = 0;
			result = vts_start_ipc_transaction(dev, data,
				VTS_IRQ_AP_COMMAND, &values, 0, 1);
			if (result < 0)
				vts_dev_err(dev, "%s debuglog ipc transaction failed\n",
					__func__);
		}
	}
	return 0;
}

static const struct memlog_ops vts_memlog_ops = {
	.file_ops_completed = vts_memlog_file_completed,
	.log_status_notify = vts_memlog_status_notify,
	.log_level_notify = vts_memlog_level_notify,
	.log_enable_notify = vts_memlog_enable_notify,
};

static int vts_pm_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct vts_data *data = container_of(nb, struct vts_data, pm_nb);
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;

	vts_dev_info(dev, "%s(%lu)\n", __func__, action);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		/* Nothing to do */
		break;
	default:
		/* Nothing to do */
		break;
	}
	return NOTIFY_DONE;
}

#if IS_ENABLED(CONFIG_EXYNOS_ITMON) || IS_ENABLED(CONFIG_EXYNOS_ITMON_V2)
static int vts_itmon_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data) {
	struct vts_data *data = container_of(nb, struct vts_data, itmon_nb);
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	struct itmon_notifier *itmon_data = nb_data;

	if (!itmon_data)
		return NOTIFY_DONE;

	/* You can find VTS itmon full name in bootloader/platform/prjName/debug/itmon.c */
	if ((strncmp(itmon_data->master, VTS_ITMON_NAME, sizeof(VTS_ITMON_NAME) - 1) == 0)
		|| (strncmp(itmon_data->dest, VTS_ITMON_NAME, sizeof(VTS_ITMON_NAME) - 1) == 0)) {

		vts_dev_info(dev, "%s: action:%ld, master:%s, dest:%s\n",
				__func__, action, itmon_data->master, itmon_data->dest);


		vts_print_fw_status(dev, __func__);

		/* Dump VTS GPR register & SRAM. Master mode can dump sram area */
		if (strncmp(itmon_data->master, VTS_ITMON_NAME, sizeof(VTS_ITMON_NAME) - 1) == 0)
			vts_dbg_dump_fw_gpr(dev, data, VTS_ITMON_ERROR);

		data->enabled = false;
		vts_mailbox_update_vts_is_on(false);

		if (data->silent_reset_support) {
			if (data->recovery_try_cnt > RECOVERY_MAX_CNT)
				return ITMON_WATCHDOG_MASK;
			else
				return ITMON_SKIP_MASK;
		} else {
			return ITMON_WATCHDOG_MASK;
		}
	}
	return NOTIFY_DONE;
}
#endif

void vts_bargein_call_notifier(void)
{
	struct vts_data *data = p_vts_data;
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;

	vts_dev_info(dev, "%s\n", __func__);

	blocking_notifier_call_chain(&vts_notifier, 0, NULL);
}
EXPORT_SYMBOL_GPL(vts_bargein_call_notifier);


static int vts_bargein_notifier_handler(struct notifier_block *nb,
	unsigned long action, void *nb_data)
{
	struct vts_data *data = container_of(nb, struct vts_data, bargein_nb);
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;

	vts_dev_info(dev, "%s: enter\n", __func__);
	data->poll_event_type |= (EVENT_TRIGGERED|EVENT_READY|EVENT_BARGEIN);
	wake_up(&data->poll_wait_queue);

	return NOTIFY_DONE;
}

static int vts_sysevent_ramdump(
	int enable, const struct sysevent_desc *sysevent)
{
	struct vts_data *data = p_vts_data;
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	/*
	 * This function is called in syseventtem_put / restart
	 * TODO: Ramdump related
	 */
	vts_dev_info(dev, "%s: call-back function\n", __func__);
	return 0;
}

static int vts_sysevent_powerup(const struct sysevent_desc *sysevent)
{
	struct vts_data *data = p_vts_data;
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	/*
	 * This function is called in syseventtem_get / restart
	 * TODO: Power up related
	 */
	vts_dev_info(dev, "%s: call-back function\n", __func__);
	return 0;
}

static int vts_sysevent_shutdown(
	const struct sysevent_desc *sysevent, bool force_stop)
{
	struct vts_data *data = p_vts_data;
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	/*
	 * This function is called in syseventtem_get / restart
	 * TODO: Shutdown related
	 */
	vts_dev_info(dev, "%s: call-back function\n", __func__);
	return 0;
}

static void vts_sysevent_crash_shutdown(const struct sysevent_desc *sysevent)
{
	struct vts_data *data = p_vts_data;
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	/*
	 * This function is called in syseventtem_get / restart
	 * TODO: Crash Shutdown related
	 */
	vts_dev_info(dev, "%s: call-back function\n", __func__);
}

static struct platform_driver * const vts_sub_drivers[] = {
	&samsung_vts_dma_driver,
};

static int get_clk_path(struct device *dev, char *name, struct clk_path* path)
{
	struct device_node *np = dev->of_node;
	struct property *prop;
	const __be32 *p;
	int value;
	int prop_len;
	int i;

	p = NULL;
	prop = of_find_property(np, name, &prop_len);
	prop_len = prop_len / sizeof(int);
	vts_dev_info(dev, "%s: name: %s prop_len: %d\n", __func__, name, prop_len);

	if (!prop) {
		vts_dev_warn(dev, "%s: %s DT property missing\n", __func__, name);
		path[0].index = -1;
	} else {
		for (i = 0; i < prop_len / 2; i++) {
			p = of_prop_next_u32(prop, p, &value);
			if (!p) {
				vts_dev_warn(dev, "%s: %s index parsing error (%d)\n", __func__, name, i);
				path[i].index = -1;
				return 0;
			}
			path[i].index = value;

			p = of_prop_next_u32(prop, p, &value);
			if (!p) {
				vts_dev_warn(dev, "%s: %s value parsing error (%d)\n", __func__, name, i);
				return 0;
			}
			path[i].value = value;
		}
		path[i].index = -1;
	}

	return 0;
}

static int get_clk_name_list(struct device *dev)
{
	struct vts_data *data = p_vts_data;
	struct device_node *np = dev->of_node;
	struct property *prop;
	const char *cur = NULL;
	int i = 0;

	prop = of_find_property(np, "clk-name-list", NULL);

	if (prop) {
		for (i = 0; ((cur = of_prop_next_string(prop, cur)) != NULL) && (i < CLK_MAX_CNT); i++) {
			strcpy(data->clk_name_list[i], cur);
			vts_dev_info(dev, "%s: clk_name_list: %s\n", __func__, data->clk_name_list[i]);
		}
	}

	return 0;
}

static int samsung_vts_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct vts_data *data;
	int i, result;

	vts_dev_info(dev, "%s\n", __func__);
	data = devm_kzalloc(dev, sizeof(struct vts_data), GFP_KERNEL);
	if (!data) {
		vts_dev_err(dev, "Failed to allocate memory\n");
		result = -ENOMEM;
		goto error;
	}

	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(36));

	vts_port_init(dev);

	/* Model binary memory allocation */
	data->google_version = 0;
	result = of_property_read_u32(np, "samsung,max-model-size", (u32 *)(&data->sm_info.max_sz));
	if (result) {
		vts_dev_info(dev, "%s: get max-model-size error :%d, set default value\n",
				__func__, result);
		data->sm_info.max_sz = VTS_MODEL_BIN_MAXSZ;
	}
	vts_dev_info(dev, "%s: sound model's max size: 0x%x\n",
			__func__, (u32)data->sm_info.max_sz);

	data->sm_info.actual_sz = 0;
	data->sm_info.offset = 0;
	data->sm_loaded = false;
	data->sm_data = vmalloc(data->sm_info.max_sz);
	if (!data->sm_data) {
		vts_dev_err(dev, "%s Failed to allocate Grammar Bin memory\n",
			__func__);
		result = -ENOMEM;
		goto error;
	}

	/* initialize device structure members */
	data->target_id = 0;

	/* initialize micbias setting count */
	data->micclk_init_cnt = 0;
	data->mic_ready = 0;
	data->vts_state = VTS_STATE_NONE;
	data->vts_rec_state = VTS_REC_STATE_STOP;
	data->vts_tri_state = VTS_TRI_STATE_COPY_STOP;

	platform_set_drvdata(pdev, data);
	data->pdev = pdev;
	p_vts_data = data;

	init_waitqueue_head(&data->ipc_wait_queue);
	init_waitqueue_head(&data->poll_wait_queue);
	spin_lock_init(&data->ipc_spinlock);
	spin_lock_init(&data->state_spinlock);
	mutex_init(&data->ipc_mutex);
	mutex_init(&data->mutex_pin);
	mutex_init(&data->firmware_lock);
	mutex_init(&data->firmware_boot_lock);
	data->wake_lock = wakeup_source_register(dev, "vts");
	data->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(data->pinctrl)) {
		vts_dev_err(dev, "Couldn't get pins (%li)\n",
				PTR_ERR(data->pinctrl));
		data->pinctrl = NULL;
	}

	data->sfr_base = vts_devm_get_request_ioremap(pdev,
		"sfr", NULL, NULL);
	if (IS_ERR(data->sfr_base)) {
		result = PTR_ERR(data->sfr_base);
		goto error;
	}

	data->baaw_base = vts_devm_get_request_ioremap(pdev,
		"baaw", NULL, NULL);
	if (IS_ERR(data->baaw_base)) {
		result = PTR_ERR(data->baaw_base);
		goto error;
	}

	data->sram_base = vts_devm_get_request_ioremap(pdev,
		"sram", &data->sram_phys, &data->sram_size);
	if (IS_ERR(data->sram_base)) {
		result = PTR_ERR(data->sram_base);
		goto error;
	}

	data->dmic_if0_base = vts_devm_get_request_ioremap(pdev,
		"dmic", NULL, NULL);
	if (IS_ERR(data->dmic_if0_base)) {
		result = PTR_ERR(data->dmic_if0_base);
		goto error;
	}

	data->dmic_if1_base = vts_devm_get_request_ioremap(pdev,
		"dmic1", NULL, NULL);
	if (IS_ERR(data->dmic_if1_base)) {
		result = PTR_ERR(data->dmic_if1_base);
		goto error;
	}
#if defined(CONFIG_SOC_S5E9935) || defined(CONFIG_SOC_S5E8835) || defined(CONFIG_SOC_S5E9945) || defined(CONFIG_SOC_S5E8845) || defined(CONFIG_SOC_S5E9955) || defined(CONFIG_SOC_S5E8855)
	data->gpr_base = vts_devm_get_request_ioremap(pdev,
		"gpr", NULL, NULL);
	if (IS_ERR(data->gpr_base)) {
		result = PTR_ERR(data->gpr_base);
		goto error;
	}
#else
	data->gpr_base = data->sfr_base + VTS_SYSREG_YAMIN_DUMP;
	if (IS_ERR(data->gpr_base)) {
		result = PTR_ERR(data->gpr_base);
		goto error;
	}
#endif

	data->sicd_base = samsung_vts_membase_request_and_map(pdev,
		"sicd-base", "sicd-size");
	if (IS_ERR(data->sicd_base)) {
		result = PTR_ERR(data->sicd_base);
		goto error;
	}
#if defined(CONFIG_SOC_S5E9925) || defined(CONFIG_SOC_S5E9935) || defined(CONFIG_SOC_S5E9945) || defined(CONFIG_SOC_S5E9955)
	data->intmem_code = vts_devm_get_request_ioremap(pdev,
		"intmem_code", NULL, NULL);
	if (IS_ERR(data->intmem_code)) {
		result = PTR_ERR(data->intmem_code);
		goto error;
	}
	data->intmem_data = vts_devm_get_request_ioremap(pdev,
		"intmem_data", NULL, NULL);
	if (IS_ERR(data->intmem_data)) {
		result = PTR_ERR(data->intmem_data);
		goto error;
	}
	data->intmem_pcm = vts_devm_get_request_ioremap(pdev,
		"intmem_pcm", NULL, NULL);
	if (IS_ERR(data->intmem_pcm)) {
		result = PTR_ERR(data->intmem_pcm);
		goto error;
	}

	data->intmem_data1 = vts_devm_get_request_ioremap(pdev,
		"intmem_data1", NULL, NULL);
	if (IS_ERR(data->intmem_data1)) {
		result = PTR_ERR(data->intmem_data1);
		goto error;
	}

	data->sfr_slif_vts = vts_devm_get_request_ioremap(pdev,
		"slif_vts", NULL, NULL);
	if (IS_ERR(data->sfr_slif_vts)) {
		result = PTR_ERR(data->sfr_slif_vts);
		goto error;
	}
#endif

#if defined(CONFIG_SOC_S5E9945) || defined(CONFIG_SOC_S5E9955)
	data->cmu_vts = vts_devm_get_request_ioremap(pdev,
		"cmu_vts", NULL, NULL);
	if (IS_ERR(data->cmu_vts)) {
		result = PTR_ERR(data->cmu_vts);
		goto error;
	}
#endif

data->timer0_base = vts_devm_get_request_ioremap(pdev,
	"timer0", NULL, NULL);
if (IS_ERR(data->timer0_base)) {
	result = PTR_ERR(data->timer0_base);
	goto error;
}

#if defined(CONFIG_SOC_S5E8835) || defined(CONFIG_SOC_S5E8845) || defined(CONFIG_SOC_S5E8855)
	data->dmic_ahb0_base = vts_devm_get_request_ioremap(pdev,
		"dmic_ahb0", NULL, NULL);
	if (IS_ERR(data->dmic_ahb0_base)) {
		result = PTR_ERR(data->dmic_ahb0_base);
		goto error;
	}
#endif
	data->lpsdgain = 0;
	data->dmic_gain[0] = 0;
	data->dmic_gain[1] = 0;
	data->dmic_1db_gain[0] = 0;
	data->dmic_1db_gain[1] = 0;
	data->amicgain = 0;

	/* read tunned VTS gain values */
	of_property_read_u32(np, "lpsd-gain", &data->lpsdgain);
	of_property_read_u32(np, "amic-gain", &data->amicgain);

	vts_dev_info(dev, "VTS Tunned Gain value LPSD: %d AMIC: %d\n",
			data->lpsdgain, data->amicgain);

	vts_dev_info(dev, "VTS: use dmam_alloc_coherent\n");
	data->dmab.area = dmam_alloc_coherent(dev,
		DMA_BUF_BYTES_MAX, &data->dmab.addr, GFP_KERNEL);
	if (data->dmab.area == NULL) {
		result = -ENOMEM;
		goto error;
	}
	data->dmab.bytes = DMA_BUF_SIZE;
	data->dmab.dev.dev = dev;
	data->dmab.dev.type = SNDRV_DMA_TYPE_DEV;

	data->dmab_rec.area = (data->dmab.area + DMA_BUF_REC_OFFSET);
	data->dmab_rec.addr = (data->dmab.addr + DMA_BUF_REC_OFFSET);
	data->dmab_rec.bytes = DMA_BUF_SIZE;
	data->dmab_rec.dev.dev = dev;
	data->dmab_rec.dev.type = SNDRV_DMA_TYPE_DEV;

	data->dmab_internal.area = (data->dmab.area + DMA_BUF_INTERNAL_OFFSET);
	data->dmab_internal.addr = (data->dmab.addr + DMA_BUF_INTERNAL_OFFSET);
	data->dmab_internal.bytes = DMA_BUF_SIZE;
	data->dmab_internal.dev.dev = dev;

	data->dmab_ns_l.area = (data->dmab.area + DMA_BUF_NS_L_OFFSET);
	data->dmab_ns_l.addr = (data->dmab.addr + DMA_BUF_NS_L_OFFSET);
	data->dmab_ns_l.bytes = DMA_BUF_SIZE;
	data->dmab_ns_l.dev.dev = dev;
	data->dmab_ns_l.dev.type = SNDRV_DMA_TYPE_DEV;

	data->dmab_ns_r.area = (data->dmab.area + DMA_BUF_NS_R_OFFSET);
	data->dmab_ns_r.addr = (data->dmab.addr + DMA_BUF_NS_R_OFFSET);
	data->dmab_ns_r.bytes = DMA_BUF_SIZE;
	data->dmab_ns_r.dev.dev = dev;
	data->dmab_ns_r.dev.type = SNDRV_DMA_TYPE_DEV;

	data->dmab_log.area = dmam_alloc_coherent(dev, LOG_BUFFER_BYTES_MAX,
				&data->dmab_log.addr, GFP_KERNEL);
	if (data->dmab_log.area == NULL) {
		result = -ENOMEM;
		goto error;
	}
	data->dmab_log.bytes = LOG_BUFFER_BYTES_MAX;
	data->dmab_log.dev.dev = dev;
	data->dmab_log.dev.type = SNDRV_DMA_TYPE_DEV;

	data->dmab_model.area = dmam_alloc_coherent(dev,
		data->sm_info.max_sz,
		&data->dmab_model.addr, GFP_KERNEL);
	if (data->dmab_model.area == NULL) {
		result = -ENOMEM;
		goto error;
	}
	data->dmab_model.bytes = data->sm_info.max_sz;
	data->dmab_model.dev.dev = dev;
	data->dmab_model.dev.type = SNDRV_DMA_TYPE_DEV;

	for (i = 0; i < CLK_MAX_CNT; i++) {
		char clk_name[20];
		sprintf(clk_name, "clk_src%d", i);
		data->clk_src_list[i].clk_src = vts_devm_clk_get_and_prepare(dev, clk_name);
		data->clk_src_list[i].enabled = false;
		if (IS_ERR(data->clk_src_list[i].clk_src)) {
			vts_dev_warn(dev, "clk_src[%d] is NULL\n", i);
			data->clk_src_list[i].clk_src = NULL;
		}
	}

	result = get_clk_path(dev, "vts-init-clk", data->init_clk_path);
	if (result) {
		vts_dev_err(dev, "%s: get vts_init_clk property error\n", __func__);
		goto error;
	}

	result = get_clk_path(dev, "vts-alive-clk", data->alive_clk_path);
	if (result) {
		vts_dev_err(dev, "%s: get vts_alive_clk property error\n", __func__);
		goto error;
	}

	result = get_clk_path(dev, "vts-sys-clk", data->sys_clk_path);
	if (result) {
		vts_dev_err(dev, "%s: get vts_sys_clk property error\n", __func__);
		goto error;
	}

	result = get_clk_path(dev, "vts-tri-clk", data->tri_clk_path);
	if (result) {
		vts_dev_err(dev, "%s: get vts_tri_clk property error\n", __func__);
		goto error;
	}

	result = get_clk_path(dev, "vts-aud-clk", data->aud_clk_path);
	if (result) {
		vts_dev_err(dev, "%s: get vts_aud_clk property error\n", __func__);
		goto error;
	}

	result = get_clk_name_list(dev);
	if (result) {
		vts_dev_warn(dev, "%s: get_clk_name_list error\n", __func__);
	}

	if (vts_soc_probe(dev))
		vts_dev_warn(dev, "Failed to vts_soc_probe\n");
	else
		vts_dev_dbg(dev, "vts_soc_probe ok");

	result = vts_devm_request_threaded_irq(dev, "error",
		VTS_IRQ_VTS_ERROR, vts_error_handler);
	if (result < 0)
		goto error;

	result = vts_devm_request_threaded_irq(dev, "boot_completed",
		VTS_IRQ_VTS_BOOT_COMPLETED, vts_boot_completed_handler);
	if (result < 0)
		goto error;

	result = vts_devm_request_threaded_irq(dev,
		"voice_triggered",
		VTS_IRQ_VTS_VOICE_TRIGGERED, vts_voice_triggered_handler);
	if (result < 0)
		goto error;

	result = vts_devm_request_threaded_irq(dev,
		"trigger_period_elapsed",
		VTS_IRQ_VTS_PERIOD_ELAPSED, vts_trigger_period_elapsed_handler);
	if (result < 0)
		goto error;

	result = vts_devm_request_threaded_irq(dev,
		"record_period_elapsed",
		VTS_IRQ_VTS_REC_PERIOD_ELAPSED,
		vts_record_period_elapsed_handler);
	if (result < 0)
		goto error;

	result = vts_devm_request_threaded_irq(dev, "debuglog_bufzero",
		VTS_IRQ_VTS_DBGLOG_BUFZERO, vts_debuglog_bufzero_handler);
	if (result < 0)
		goto error;

	result = vts_devm_request_threaded_irq(dev, "internal_rec_elapsed",
		VTS_IRQ_VTS_INTERNAL_REC_ELAPSED, vts_internal_rec_elapsed_handler);
	if (result < 0)
		goto error;

	data->irq_state = true;

	result = vts_irq_register(dev);
	if (result < 0)
		goto error;

	data->pdev_mailbox = of_find_device_by_node(of_parse_phandle(np,
		"mailbox", 0));
	if (!data->pdev_mailbox) {
		vts_dev_err(dev, "Failed to get mailbox\n");
		result = -EPROBE_DEFER;
		goto error;
	}

	data->regmap_dmic = devm_regmap_init_mmio_clk(dev,
			NULL,
			data->dmic_if0_base,
			&vts_component_regmap_config);

	result = vts_ctrl_register(dev);
	if (result < 0) {
		vts_dev_err(dev, "Failed to register ASoC component\n");
		goto error;
	}

#ifdef EMULATOR
	pmu_alive = ioremap(0x16480000, 0x10000);
#endif

	data->pm_nb.notifier_call = vts_pm_notifier;
	register_pm_notifier(&data->pm_nb);

#if IS_ENABLED(CONFIG_EXYNOS_ITMON) || IS_ENABLED(CONFIG_EXYNOS_ITMON_V2)
	data->itmon_nb.notifier_call = vts_itmon_notifier;
	itmon_notifier_chain_register(&data->itmon_nb);
#endif

	/* Notifier initialization for barge-in*/
	data->bargein_nb.notifier_call = vts_bargein_notifier_handler,
	blocking_notifier_chain_register(&vts_notifier, &data->bargein_nb);

	data->sysevent_dev = NULL;
	data->sysevent_desc.name = pdev->name;
	data->sysevent_desc.owner = THIS_MODULE;
	data->sysevent_desc.ramdump = vts_sysevent_ramdump;
	data->sysevent_desc.powerup = vts_sysevent_powerup;
	data->sysevent_desc.shutdown = vts_sysevent_shutdown;
	data->sysevent_desc.crash_shutdown = vts_sysevent_crash_shutdown;
	data->sysevent_desc.dev = &pdev->dev;
	data->sysevent_dev = sysevent_register(&data->sysevent_desc);
	if (IS_ERR(data->sysevent_dev)) {
		result = PTR_ERR(data->sysevent_dev);
		vts_dev_err(dev, "fail in device_event_probe : %d\n", result);
		goto error;
	}
	vts_proc_probe();
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	//vts_clk_set_rate(dev, 0);
	vts_port_cfg(dev, VTS_PORT_PAD0, VTS_PORT_ID_VTS, DPDM, false);

	data->voicecall_enabled = false;
	data->current_active_ids = 0;
	data->syssel_rate = DMIC_IF_SYS_SEL_VTS;
	data->target_sysclk = 0;
	data->target_size = 0;
	data->vtsfw_version = 0x0;
	data->vtsdetectlib_version = 0x0;
	data->fw_logfile_enabled = 0;
	data->fw_logger_enabled = 0;
	data->silent_reset_support = 1;
	vts_set_clk_src(dev, VTS_CLK_SRC_RCO);

	result = device_create_file(dev, &dev_attr_vtsfw_version);
	if (result < 0)
		vts_dev_warn(dev, "Failed to create file: %s\n",
			"vtsfw_version");

	result = device_create_file(dev, &dev_attr_vtsdetectlib_version);
	if (result < 0)
		vts_dev_warn(dev, "Failed to create file: %s\n",
			"vtsdetectlib_version");

	result = device_create_file(dev, &dev_attr_vts_cmd);
	if (result < 0)
		vts_dev_warn(dev, "Failed to create file: %s\n",
			"vts_cmd");

#if defined(CONFIG_VTS_SFR_DUMP)
	result = device_create_file(dev, &dev_attr_vts_sfr_dump);
	if (result < 0)
		vts_dev_warn(dev, "Failed to create file: %s\n",
			"vts_sfr_dump");

	result = device_create_file(dev, &dev_attr_vts_dmic_if_dump);
	if (result < 0)
		vts_dev_warn(dev, "Failed to create file: %s\n",
			"vts_dmic_if_dump");

#if defined(CONFIG_SOC_S5E9925) || defined(CONFIG_SOC_S5E9935) || defined(CONFIG_SOC_S5E9945) || defined(CONFIG_SOC_S5E9955)
	result = device_create_file(dev, &dev_attr_vts_slif_dump);
	if (result < 0)
		vts_dev_warn(dev, "Failed to create file: %s\n",
			"vts_slif_dump");
#endif

	result = device_create_file(dev, &dev_attr_vts_gpio_dump);
	if (result < 0)
		vts_dev_warn(dev, "Failed to create file: %s\n",
			"vts_gpio_dump");
#endif

	data->sramlog_baseaddr = (char *)(data->sram_base + VTS_SRAMLOG_MSGS_OFFSET);
	data->shared_info = (struct vts_shared_info *)(data->sramlog_baseaddr +	VTS_SRAMLOG_SIZE_MAX);

	atomic_notifier_chain_register(&panic_notifier_list,
		&vts_panic_notifier);

	/* initialize log buffer offset as non */
	vts_register_log_buffer(dev, 0, 0);

	device_init_wakeup(dev, true);

	/* Allocate Memory for error logging */
	data->p_dump.sram_log =
		kzalloc(VTS_SRAMLOG_SIZE_MAX + sizeof(VTS_LOG_DUMP_MAGIC), GFP_KERNEL);

	if (!data->p_dump.sram_log) {
		vts_dev_warn(dev, "%s: faild to allocate memory for sram log\n",
			__func__);
	}

	data->p_dump.fw_dump =
		kzalloc(sizeof(struct vts_dbg_fw_dump), GFP_KERNEL);

	if (!data->p_dump.fw_dump) {
		vts_dev_warn(dev, "%s: failed to allocate memory for firmware dump\n",
			__func__);
	}

	result = vts_misc_register(dev);

	if (result < 0)
		goto error;

#ifdef DEF_VTS_PCM_DUMP
	vts_pcm_dump_init(dev);
	vts_pcm_dump_register(dev, PCM_DUMP_NODE_REC, "vts_rec", data->dmab_rec.area,
		data->dmab_rec.addr, DMA_BUF_SIZE);
	vts_pcm_dump_register(dev, PCM_DUMP_NODE_TRI, "vts_tri", data->dmab.area,
		data->dmab.addr, DMA_BUF_SIZE);
	vts_pcm_dump_register(dev, PCM_DUMP_NODE_INTERNAL, "vts_internal", data->dmab_internal.area,
		data->dmab_internal.addr, DMA_BUF_SIZE);
	vts_pcm_dump_register(dev, PCM_DUMP_NODE_NS_L, "vts_ns_l", data->dmab_ns_l.area,
		data->dmab_ns_l.addr, DMA_BUF_SIZE);
	vts_pcm_dump_register(dev, PCM_DUMP_NODE_NS_R, "vts_ns_r", data->dmab_ns_r.area,
		data->dmab_ns_r.addr, DMA_BUF_SIZE);
#endif

	vts_dev_info(dev, "register sub drivers\n");
	result = platform_register_drivers(vts_sub_drivers,
			ARRAY_SIZE(vts_sub_drivers));
	if (result)
		vts_dev_warn(dev, "Failed to register sub drivers\n");
	else {
		vts_dev_info(dev, "register sub drivers OK\n");
		of_platform_populate(np, NULL, NULL, dev);
	}

	if (pm_runtime_active(dev))
		pm_runtime_put(dev);

#if IS_ENABLED(CONFIG_EXYNOS_S2MPU)
	data->s2mpu_nb.subsystem = VTS_S2MPU_NAME;
	data->s2mpu_nb.notifier_call = vts_s2mpu_fault_handler;
	exynos_s2mpu_notifier_call_register(&data->s2mpu_nb);
	vts_dev_info(dev, "called exynos_s2mpu_notifier_call_register\n");
#endif
	vts_dev_info(dev, "Probed successfully\n");

error:

	return result;
}

static int samsung_vts_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct vts_data *data = platform_get_drvdata(pdev);

	pm_runtime_disable(dev);

	vts_soc_remove(dev);

#ifndef CONFIG_PM
	vts_runtime_suspend(dev);
#endif
	if(data->firmware) {
		release_firmware(data->firmware);
		data->firmware = NULL;
	}
	if (data->sm_data)
		vfree(data->sm_data);

	if (data->p_dump.sram_log)
		kfree(data->p_dump.sram_log);

	if (data->p_dump.fw_dump)
		kfree(data->p_dump.fw_dump);

	snd_soc_unregister_component(dev);
#ifdef EMULATOR
	iounmap(pmu_alive);
#endif
	return 0;
}

static struct platform_driver samsung_vts_driver = {
	.probe  = samsung_vts_probe,
	.remove = samsung_vts_remove,
	.driver = {
		.name = "samsung-vts",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_vts_of_match),
		.pm = &samsung_vts_pm,
	},
};

module_platform_driver(samsung_vts_driver);

/* Module information */
MODULE_AUTHOR("@samsung.com");
MODULE_DESCRIPTION("Samsung Voice Trigger System");
MODULE_ALIAS("platform:samsung-vts");
MODULE_LICENSE("GPL");
