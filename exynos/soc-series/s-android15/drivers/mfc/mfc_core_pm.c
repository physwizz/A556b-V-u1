/*
 * drivers/media/platform/exynos/mfc/mfc_core_pm.c
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/pm_runtime.h>
#include <soc/samsung/exynos/exynos-hvc.h>

#include "mfc_core_pm.h"
#include "mfc_core_sync.h"

#include "mfc_core_hw_reg_api.h"

#include "base/mfc_qos.h"

void mfc_core_pm_init(struct mfc_core *core)
{
	spin_lock_init(&core->pm.clklock);
	atomic_set(&core->pm.pwr_ref, 0);
	atomic_set(&core->pm.protect_ref, 0);
	atomic_set(&core->clk_ref, 0);

	core->pm.device = core->device;
	core->pm.clock_on_steps = 0;
	core->pm.clock_off_steps = 0;
	pm_runtime_enable(core->pm.device);
}

void mfc_core_pm_final(struct mfc_core *core)
{
	pm_runtime_disable(core->pm.device);
}

void mfc_core_protection_on(struct mfc_core *core, int pending_check)
{
#if IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
	unsigned long flags;
	int ret = 0;

	if (!core->dev->pdata->security_ctrl && pending_check) {
		ret = mfc_core_wait_pending(core);
		if (ret != 0) {
			mfc_core_err("pending wait failed (%d)\n", ret);
			call_dop(core, dump_and_stop_debug_mode, core);
			mfc_core_change_state(core, MFCCORE_ERROR);
		}
	}

	spin_lock_irqsave(&core->pm.clklock, flags);
	mfc_core_debug(3, "Begin: enable protection\n");

	if (atomic_read(&core->pm.protect_ref)) {
		mfc_core_err("IP protection is already enabled\n");
		MFC_TRACE_CORE("already protected\n");
		spin_unlock_irqrestore(&core->pm.clklock, flags);
		return;
	}

	ret = exynos_hvc(HVC_PROTECTION_SET, 0,
			core->id * PROT_MFC1, HVC_PROTECTION_ENABLE, 0);
	if (ret != HVC_OK) {
		mfc_core_err("Protection Enable failed! ret(%u)\n", ret);
		call_dop(core, dump_and_stop_debug_mode, core);
		spin_unlock_irqrestore(&core->pm.clklock, flags);
		return;
	}

	atomic_inc(&core->pm.protect_ref);
	MFC_TRACE_CORE("protection\n");
	mfc_core_debug(3, "End: enable protection\n");
	spin_unlock_irqrestore(&core->pm.clklock, flags);

	/* RISC_ON is required because the MFC was reset */
	if (core->dev->pdata->security_ctrl) {
		mfc_core_risc_on(core);
		mfc_core_debug(2, "Will now wait for completion of firmware transfer\n");
		if (mfc_wait_for_done_core(core, MFC_REG_R2H_CMD_FW_STATUS_RET)) {
			mfc_core_err("Failed to RISC_ON\n");
			mfc_core_clean_dev_int_flags(core);
			call_dop(core, dump_and_stop_always, core);
		}
	} else {
		mfc_core_set_risc_base_addr(core, MFCBUF_DRM);
	}
#endif
}

void mfc_core_protection_off(struct mfc_core *core, int pending_check)
{
#if IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
	unsigned long flags;
	int ret = 0;

	if (!core->dev->pdata->security_ctrl && pending_check) {
		ret = mfc_core_wait_pending(core);
		if (ret != 0) {
			mfc_core_err("pending wait failed (%d)\n", ret);
			call_dop(core, dump_and_stop_debug_mode, core);
			mfc_core_change_state(core, MFCCORE_ERROR);
		}
	}

	/*
	 * After clock off the protection disable should be
	 * because the MFC core can continuously run during clock on
	 */
	spin_lock_irqsave(&core->pm.clklock, flags);
	mfc_core_debug(3, "Begin: disable protection\n");

	if (!atomic_read(&core->pm.protect_ref)) {
		mfc_core_err("IP protection is already disabled\n");
		MFC_TRACE_CORE("already un-protected\n");
		spin_unlock_irqrestore(&core->pm.clklock, flags);
		return;
	}

	ret = exynos_hvc(HVC_PROTECTION_SET, 0,
			core->id * PROT_MFC1, HVC_PROTECTION_DISABLE, 0);
	if (ret != HVC_OK) {
		mfc_core_err("Protection Disable failed! ret(%u)\n", ret);
		call_dop(core, dump_and_stop_debug_mode, core);
		spin_unlock_irqrestore(&core->pm.clklock, flags);
		return;
	}

	atomic_dec(&core->pm.protect_ref);
	mfc_core_debug(3, "End: disable protection\n");
	MFC_TRACE_CORE("un-protection\n");
	spin_unlock_irqrestore(&core->pm.clklock, flags);

	/* Normal base address setting is required because the MFC was reset */
	mfc_core_set_risc_base_addr(core, MFCBUF_NORMAL);
#endif
}

int mfc_core_pm_clock_on(struct mfc_core *core, bool qos_update)
{
	int ret = 0;
	int state;

	if (core->dev->pdata->support_hwacg == MFC_HWACG_HWFW_CTRL)
		return ret;

	core->pm.clock_on_steps = 1;
	state = mfc_core_get_clk_ref_cnt(core);

	/*
	 * When the clock is enabled, the MFC core can run immediately.
	 * So the base addr and protection should be applied before clock enable.
	 * The MFC and TZPC SFR are in APB clock domain and it is accessible
	 * through Q-CH even if clock off.
	 * The sequence for switching normal to drm is following
	 * cache flush (cmd 12) -> clock off -> set DRM base addr
	 * -> IP Protection enable -> clock on
	 */
	core->pm.clock_on_steps |= 0x1 << 1;
	if (core->pm.base_type != MFCBUF_INVALID) {
		/*
		 * There is no place to set core->pm.base_type,
		 * so it is always MFCBUF_INVALID now.
		 * When necessary later, you can set the bse_type.
		 */
		core->pm.clock_on_steps |= 0x1 << 2;
		ret = mfc_core_wait_pending(core);
		if (ret != 0) {
			mfc_core_err("pending wait failed (%d)\n", ret);
			call_dop(core, dump_and_stop_debug_mode, core);
			return ret;
		}
		core->pm.clock_on_steps |= 0x1 << 3;
		mfc_core_set_risc_base_addr(core, core->pm.base_type);
	}

	core->pm.clock_on_steps |= 0x1 << 4;
	if (!IS_ERR(core->pm.clock)) {
		ret = clk_enable(core->pm.clock);
		if (ret < 0)
			mfc_core_err("clk_enable failed (%d)\n", ret);
	}

	core->pm.clock_on_steps |= 0x1 << 5;
	state = atomic_inc_return(&core->clk_ref);

	if (!core->dev->multi_core_inst_bits && !core->dev->debugfs.hwacg_disable)
		mfc_core_mfc_on(core);

	mfc_core_debug(2, "clk_ref = %d\n", state);
	MFC_TRACE_LOG_CORE("clk_ref = %d", state);

	if (qos_update && state == 1)
		mfc_qos_update(core, 1);

	return 0;
}

void mfc_core_pm_clock_off(struct mfc_core *core, bool qos_update)
{
	int state;

	if (core->dev->pdata->support_hwacg == MFC_HWACG_HWFW_CTRL)
		return;

	core->pm.clock_off_steps = 1;
	state = atomic_dec_return(&core->clk_ref);
	if (state < 0) {
		mfc_core_info("MFC clock is already disabled (%d)\n", state);
		atomic_set(&core->clk_ref, 0);
		core->pm.clock_off_steps |= 0x1 << 2;
		MFC_TRACE_CORE("** clock_off already: ref state(%d)\n", mfc_core_get_clk_ref_cnt(core));
	} else {
		core->pm.clock_off_steps |= 0x1 << 3;
		if (!IS_ERR(core->pm.clock)) {
			clk_disable(core->pm.clock);
			core->pm.clock_off_steps |= 0x1 << 4;
		}
	}

	state = mfc_core_get_clk_ref_cnt(core);

	if (!core->dev->multi_core_inst_bits && !core->dev->debugfs.hwacg_disable
			&& !state)
		mfc_core_mfc_off(core);

	core->pm.clock_off_steps |= 0x1 << 5;

	mfc_core_debug(2, "clk_ref = %d\n", state);
	MFC_TRACE_LOG_CORE("clk_ref = %d", state);

	if (qos_update && state == 0)
		mfc_qos_update(core, 0);
}

int mfc_core_pm_power_on(struct mfc_core *core)
{
	int ret;

	MFC_TRACE_CORE("++ Power on\n");
	ret = pm_runtime_get_sync(core->pm.device);
	if (ret < 0) {
		mfc_core_err("Failed to get power: ret(%d)\n", ret);
		call_dop(core, dump_and_stop_debug_mode, core);
		goto err_power_on;
	}

#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
	if (core->dev->pdata->idle_clk_ctrl) {
		mfc_core_debug(2, "request mfc idle clk OFF\n");
		exynos_pm_qos_add_request(&core->qos_req_mfc_noidle, core->core_pdata->pm_qos_id,
				core->dev->pdata->mfc_freqs[0]);
	}
#endif

	core->pm.clock = clk_get(core->device, "aclk_mfc");
	if (IS_ERR(core->pm.clock)) {
		mfc_core_err("failed to get parent clock: ret(%d)\n", ret);
	} else {
		ret = clk_prepare(core->pm.clock);
		if (ret) {
			mfc_core_err("clk_prepare() failed: ret(%d)\n", ret);
			clk_put(core->pm.clock);
		}
	}

	atomic_inc(&core->pm.pwr_ref);

	MFC_TRACE_CORE("-- Power on: ret(%d)\n", ret);
	MFC_TRACE_LOG_CORE("p+%d", mfc_core_get_pwr_ref_cnt(core));

	return 0;

err_power_on:
	return ret;
}

int mfc_core_pm_power_off(struct mfc_core *core)
{
	int state;
	int ret;

	MFC_TRACE_CORE("++ Power off\n");

	state = mfc_core_get_clk_ref_cnt(core);
	if ((state > 0) && core->dev->pdata->support_hwacg != MFC_HWACG_HWFW_CTRL) {
		mfc_core_info("MFC clock is still enabled (%d)\n", state);
		mfc_core_pm_clock_off(core, 0);
	}

	if (!IS_ERR(core->pm.clock)) {
		clk_unprepare(core->pm.clock);
		clk_put(core->pm.clock);
	}

#ifdef CONFIG_MFC_USE_BUS_DEVFREQ
	if (core->dev->pdata->idle_clk_ctrl) {
		exynos_pm_qos_remove_request(&core->qos_req_mfc_noidle);
		mfc_core_debug(2, "request mfc idle clk ON\n");
	}
#endif

	mfc_core_mfc_always_off(core);

	ret = pm_runtime_put_sync(core->pm.device);
	if (ret < 0) {
		mfc_core_err("Failed to put power: ret(%d)\n", ret);
		call_dop(core, dump_and_stop_debug_mode, core);
		return ret;
	}

	atomic_dec(&core->pm.pwr_ref);

	MFC_TRACE_CORE("-- Power off: ret(%d)\n", ret);
	MFC_TRACE_LOG_CORE("p-%d", mfc_core_get_pwr_ref_cnt(core));

	return ret;
}
