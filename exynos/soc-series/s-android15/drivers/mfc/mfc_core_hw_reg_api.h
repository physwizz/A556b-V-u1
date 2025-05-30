/*
 * drivers/media/platform/exynos/mfc/mfc_core_hw_reg_api.h
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_CORE_HW_REG_API_H
#define __MFC_CORE_HW_REG_API_H __FILE__

#include "mfc_core_reg_api.h"

#include "base/mfc_common.h"
#include "base/mfc_utils.h"

#define mfc_core_get_int_reason()	(MFC_CORE_READL(MFC_REG_RISC2HOST_CMD)		\
						& MFC_REG_RISC2HOST_CMD_MASK)
#define mfc_core_clear_int()				\
		do {							\
			MFC_CORE_WRITEL(0, MFC_REG_RISC2HOST_CMD);	\
			MFC_CORE_WRITEL(0, MFC_REG_RISC2HOST_INT);	\
		} while (0)

#define mfc_core_clear_int_only()				\
		do {							\
			MFC_CORE_WRITEL(0, MFC_REG_RISC2HOST_INT);	\
		} while (0)

#define mfc_core_set_hwapg_ctrl()				\
		do {							\
			MFC_CORE_WRITEL(0x2, MFC_REG_HWAPG_HOST_CTRL);	\
		} while (0)

#define mfc_core_clear_hwapg_ctrl()				\
		do {							\
			MFC_CORE_WRITEL(0, MFC_REG_HWAPG_HOST_CTRL);	\
		} while (0)

static inline int mfc_core_wait_fw_status(struct mfc_core *core)
{
	struct mfc_dev *dev = core->dev;
	unsigned int status;
	unsigned long timeout;

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->wait_fw_status)) {
		status = MFC_CORE_READL(MFC_REG_FIRMWARE_STATUS_INFO);
		if (status & 0x1)
			return 0;

		timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
		do {
			if (time_after(jiffies, timeout)) {
				mfc_core_err("Timeout while waiting MFC F/W done\n");
				return -EIO;
			}
			status = MFC_CORE_READL(MFC_REG_FIRMWARE_STATUS_INFO);
		} while ((status & 0x1) == 0);
	}

	return 0;
}

static inline int mfc_core_wait_nal_q_status(struct mfc_core *core)
{
	struct mfc_dev *dev = core->dev;
	unsigned int status;
	unsigned long timeout;

	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->wait_nalq_status)) {
		status = MFC_CORE_READL(MFC_REG_FIRMWARE_STATUS_INFO);
		if (status & 0x2)
			return 0;

		timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
		do {
			if (time_after(jiffies, timeout)) {
				mfc_core_err("Timeout while waiting NALQ status\n");
				return -EIO;
			}
			status = MFC_CORE_READL(MFC_REG_FIRMWARE_STATUS_INFO);
		} while ((status & 0x2) == 0);
	}

	return 0;
}

static inline void mfc_core_wait_bus(struct mfc_core *core)
{
	unsigned int status;
	unsigned long timeout;
	int count = 10;

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	do {
		status = MFC_CORE_READL(MFC_REG_MFC_BUS_STATUS);
		if (status) {
			mfc_core_debug(2, "there is bus pending yet %#x\n", status);
			if (time_after(jiffies, timeout)) {
				mfc_core_err("Timeout while pendng clear\n");
				mfc_core_err("MFC access pending R: %#x, BUS: %#x\n",
						MFC_CORE_READL(MFC_REG_MFC_RPEND),
						MFC_CORE_READL(MFC_REG_MFC_BUS_STATUS));
				call_dop(core->dev, dump_and_stop_debug_mode, core->dev);
				return;
			}
		} else {
			mfc_core_debug(2, "bus pending wait count %d\n", count);
			count--;
		}
	} while (count != 0);

	return;
}

static inline int mfc_core_wait_pending(struct mfc_core *core)
{
	unsigned int status;
	unsigned long timeout;

	/* Check F/W wait status */
	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	do {
		if (time_after(jiffies, timeout)) {
			mfc_core_err("Timeout while waiting MFC F/W done\n");
			return -EIO;
		}
		status = MFC_CORE_READL(MFC_REG_FIRMWARE_STATUS_INFO);
	} while ((status & 0x1) == 0);

	/* Check H/W pending status */
	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	do {
		if (time_after(jiffies, timeout)) {
			mfc_core_err("Timeout while pendng clear\n");
			mfc_core_err("MFC access pending R: %#x, BUS: %#x\n",
					MFC_CORE_READL(MFC_REG_MFC_RPEND),
					MFC_CORE_READL(MFC_REG_MFC_BUS_STATUS));
			return -EIO;
		}
		status = MFC_CORE_READL(MFC_REG_MFC_RPEND);
	} while (status != 0);

	MFC_TRACE_CORE("** pending wait done\n");

	return 0;
}

static inline int mfc_core_stop_bus(struct mfc_core *core)
{
	unsigned int status;
	unsigned long timeout;

	/* Reset */
	MFC_CORE_WRITEL(0x1, MFC_REG_MFC_BUS_RESET_CTRL);

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	/* Check bus status */
	do {
		if (time_after(jiffies, timeout)) {
			mfc_core_err("Timeout while resetting MFC.\n");
			return -EIO;
		}
		status = MFC_CORE_READL(MFC_REG_MFC_BUS_RESET_CTRL);
	} while ((status & 0x2) == 0);

	return 0;
}

static inline void mfc_core_start_bus(struct mfc_core *core)
{
	int val;

	val = MFC_CORE_READL(MFC_REG_MFC_BUS_RESET_CTRL);
	val &= ~(0x1);
	MFC_CORE_WRITEL(val, MFC_REG_MFC_BUS_RESET_CTRL);
}

static inline void mfc_core_mfc_off(struct mfc_core *core)
{
	if (core->dev->pdata->support_hwacg == MFC_HWACG_DRV_CTRL) {
		mfc_core_debug(2, "MFC_OFF 1(off)\n");
		MFC_TRACE_CORE(">> MFC OFF 1(off)\n");
		MFC_CORE_WRITEL(0x1, MFC_REG_MFC_OFF);
	}
	return;
}

static inline void mfc_core_mfc_always_off(struct mfc_core *core)
{
	mfc_core_debug(2, "MFC_OFF 1(off)\n");
	MFC_TRACE_CORE(">> MFC OFF 1(off)\n");
	MFC_CORE_WRITEL(0x1, MFC_REG_MFC_OFF);
}

static inline void mfc_core_mfc_on(struct mfc_core *core)
{
	MFC_CORE_WRITEL(0x0, MFC_REG_MFC_OFF);
	mfc_core_debug(2, "MFC_OFF 0(on)\n");
	MFC_TRACE_CORE(">> MFC OFF 0(on)\n");
}

static inline void mfc_core_set_risc_status(struct mfc_core *core)
{
	MFC_CORE_WRITEL(0x1, MFC_REG_RISC_STATUS);
	mfc_core_debug(3, "RISC STATUS 1\n");
	MFC_TRACE_CORE(">> RISC STATUS 1\n");
}

static inline void mfc_core_risc_on(struct mfc_core *core)
{
	mfc_core_clean_dev_int_flags(core);
	mfc_core_mfc_on(core);

	MFC_CORE_WRITEL(0x1, MFC_REG_RISC_ON);
	mfc_core_debug(1, "RISC_ON\n");
	MFC_TRACE_CORE(">> RISC ON\n");
}

static inline void mfc_core_risc_off(struct mfc_core *core)
{
	unsigned int status;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	/* Check pending status */
	do {
		if (time_after(jiffies, timeout)) {
			mfc_core_err("Timeout while pendng clear\n");
			mfc_core_err("MFC access pending state: %#x\n", status);
			mfc_core_err("MFC access pending R: %#x, W: %#x\n",
					MFC_CORE_READL(MFC_REG_MFC_RPEND),
					MFC_CORE_READL(MFC_REG_MFC_WPEND));
			break;
		}
		status = MFC_CORE_READL(MFC_REG_MFC_BUS_STATUS);
	} while (status != 0);

	MFC_CORE_WRITEL(0x0, MFC_REG_RISC_ON);
}

static inline void mfc_core_enable_all_clocks(struct mfc_core *core)
{
	/* Enable all FW clock gating */
	MFC_CORE_WRITEL(0xFFFFFFFF, MFC_REG_MFC_FW_CLOCK);
}

static inline void mfc_core_enable_hwapg(struct mfc_core *core)
{
	unsigned int reg;

	if (!core->has_pmu)
		return;

	mfc_core_debug(2, "[HWAPG] enable PMU HWAPG\n");

	reg = PMU_READL(0x14);
	reg |= (0x1 << 1);
	PMU_WRITEL(reg, 0x14);
}

static inline void mfc_core_disable_hwapg(struct mfc_core *core)
{
	unsigned int reg;

	if (!core->has_pmu)
		return;

	mfc_core_debug(2, "[HWAPG] disable PMU HWAPG\n");

	reg = PMU_READL(0x14);
	reg &= ~(0x1 << 1);
	PMU_WRITEL(reg, 0x14);
}

static inline void mfc_core_print_hwapg_status(struct mfc_core *core)
{
	if (!core->has_pmu)
		return;

	mfc_core_debug(2, "[HWAPG] PMU_MFC 0x8: 0x%x, 0xc: 0x%x, 0x14: 0x%x\n",
		PMU_READL(0x8), PMU_READL(0xc), PMU_READL(0x14));
	mfc_core_debug(3, "[HWAPG] PMU_MFC 0x8: 0x1 on/0x0 off, 0xc: 0x0 on/0x80 off\n");
	mfc_core_debug(3, "[HWAPG] PMU_MFC 0x14: [1] ENABLE_HW_APG\n");
}

void mfc_core_reg_clear(struct mfc_core *core);
void mfc_core_reset_mfc(struct mfc_core *core, enum mfc_buf_usage_type buf_type);
void mfc_core_set_risc_base_addr(struct mfc_core *core,
				enum mfc_buf_usage_type buf_type);
void mfc_core_cmd_host2risc(struct mfc_core *core, int cmd);
int mfc_core_check_risc2host(struct mfc_core *core);
void mfc_core_set_gdc_votf(struct mfc_core *core, struct mfc_ctx *ctx);
void mfc_core_set_dpu_votf(struct mfc_core *core, struct mfc_ctx *ctx);
void mfc_core_clear_votf(struct mfc_core *core);
#endif /* __MFC_CORE_HW_REG_API_H */
