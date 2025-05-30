/****************************************************************************
 *
 * Copyright (c) 2014 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/
#include <linux/delay.h>
#include <linux/module.h>
#include <scsc/kic/slsi_kic_lib.h>
#include <scsc/scsc_release.h>
#include <scsc/scsc_mx.h>
#include "mxman_if.h"
#include "mxman.h"

void mxman_if_control_suspend_gpio(struct mxman *mxman, u8 value)
{
	mxman_control_suspend_gpio(mxman, value);
}
EXPORT_SYMBOL(mxman_if_control_suspend_gpio);

int mxman_if_get_state(struct mxman *mxman)
{
	return mxman_get_state(mxman);
}
EXPORT_SYMBOL(mxman_if_get_state);

bool mxman_if_subsys_in_failed_state(struct mxman *mxman, enum scsc_subsystem sub)
{
	return mxman_subsys_in_failed_state(mxman, sub);
}
EXPORT_SYMBOL(mxman_if_subsys_in_failed_state);

bool mxman_if_get_panic_in_progress(struct mxman *mxman)
{
	return mxman_get_panic_in_progress(mxman);
}
EXPORT_SYMBOL(mxman_if_get_panic_in_progress);

u64 mxman_if_get_last_panic_time(struct mxman *mxman)
{
	return mxman_get_last_panic_time(mxman);
}
EXPORT_SYMBOL(mxman_if_get_last_panic_time);

u32 mxman_if_get_panic_code(struct mxman *mxman)
{
	return mxman_get_panic_code(mxman);
}
EXPORT_SYMBOL(mxman_if_get_panic_code);

void mxman_if_show_last_panic(struct mxman *mxman)
{
	mxman_show_last_panic(mxman);
}
EXPORT_SYMBOL(mxman_if_show_last_panic);

u32 *mxman_if_get_last_panic_rec(struct mxman *mxman)
{
	return mxman_get_last_panic_rec(mxman);
}
EXPORT_SYMBOL(mxman_if_get_last_panic_rec);

u16 mxman_if_get_last_panic_rec_sz(struct mxman *mxman)
{
	return mxman_get_last_panic_rec_sz(mxman);
}
EXPORT_SYMBOL(mxman_if_get_last_panic_rec_sz);

void mxman_if_reinit_completion(struct mxman *mxman)
{
	mxman_reinit_completion(mxman);
}
EXPORT_SYMBOL(mxman_if_reinit_completion);

int mxman_if_wait_for_completion_timeout(struct mxman *mxman, u32 ms)
{
	return mxman_wait_for_completion_timeout(mxman, ms);
}
EXPORT_SYMBOL(mxman_if_wait_for_completion_timeout);

void mxman_if_fail(struct mxman *mxman, u16 scsc_panic_code, const char *reason, enum scsc_subsystem id)
{
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
	mxman_recover_subsystem(mxman, scsc_panic_code, reason, id);
#else
	mxman_fail(mxman, scsc_panic_code, reason, id);
#endif
}
EXPORT_SYMBOL(mxman_if_fail);


void mxman_if_set_syserr_recovery_in_progress(struct mxman *mxman, bool value)
{
	mxman_set_syserr_recovery_in_progress(mxman, value);
}
EXPORT_SYMBOL(mxman_if_set_syserr_recovery_in_progress);

void mxman_if_set_last_syserr_recovery_time(struct mxman *mxman, enum scsc_subsystem sub, unsigned long value)
{
	mxman_set_last_syserr_recovery_time(mxman, sub, value);
}
EXPORT_SYMBOL(mxman_if_set_last_syserr_recovery_time);

bool mxman_if_get_syserr_recovery_in_progress(struct mxman *mxman)
{
	return mxman_get_syserr_recovery_in_progress(mxman);
}
EXPORT_SYMBOL(mxman_if_get_syserr_recovery_in_progress);

u16 mxman_if_get_last_syserr_subsys(struct mxman *mxman)
{
	return mxman_get_last_syserr_subsys(mxman);
}
EXPORT_SYMBOL(mxman_if_get_last_syserr_subsys);

int mxman_if_force_panic(struct mxman *mxman, enum scsc_subsystem sub)
{
	return mxman_force_panic(mxman, sub);
}
EXPORT_SYMBOL(mxman_if_force_panic);

int mxman_if_lerna_send(struct mxman *mxman, void *data, u32 size)
{
	return mxman_lerna_send(mxman, data, size);
}
EXPORT_SYMBOL(mxman_if_lerna_send);

int mxman_if_open(struct mxman *mxman, enum scsc_subsystem sub, void *data, size_t data_sz)
{
	return mxman_open(mxman, sub, data, data_sz);
}
EXPORT_SYMBOL(mxman_if_open);

void mxman_if_close(struct mxman *mxman, enum scsc_subsystem sub)
{
	mxman_close(mxman, sub);
}
EXPORT_SYMBOL(mxman_if_close);

bool mxman_if_subsys_active(struct mxman *mxman, enum scsc_subsystem sub)
{
	return mxman_subsys_active(mxman, sub);
}
EXPORT_SYMBOL(mxman_if_subsys_active);

bool mxman_if_users_active(struct mxman *mxman)
{
	return mxman_users_active(mxman);
}
EXPORT_SYMBOL(mxman_if_users_active);

#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
void mxman_if_indep_recovery_err_completion(struct mxman *mxman)
{
	mxman_indep_recovery_err_completion(mxman);
}
EXPORT_SYMBOL(mxman_if_indep_recovery_err_completion);

bool mxman_if_warm_reset_in_progress(void)
{
	return mxman_warm_reset_in_progress();
}
EXPORT_SYMBOL(mxman_if_warm_reset_in_progress);

bool mxman_if_check_recovery_state_none(void)
{
	return mxman_check_recovery_state_none();
}
EXPORT_SYMBOL(mxman_if_check_recovery_state_none);

void mxman_if_notify_result_to_start_cmd(struct mxman *mxman, bool success)
{
	if (mxman_warm_reset_in_progress())
		mxman_send_rcvry_evt_to_fsm(mxman, success ? RCVRY_EVT_START_SUCCESS : RCVRY_EVT_START_FAILURE);
	else if (mxman_check_recovery_state_none() && success)
		mxman_set_recovery_mode(mxman, true);
}
EXPORT_SYMBOL(mxman_if_notify_result_to_start_cmd);

int mxman_if_check_start_failure_during_indep_recovery(struct mxman *mxman)
{
	return mxman_check_start_failure_during_indep_recovery(mxman);
}
EXPORT_SYMBOL(mxman_if_check_start_failure_during_indep_recovery);
#endif

#ifdef CONFIG_HDM_WLBT_SUPPORT
int mxman_if_get_hdm_wlan_support(void)
{
	return mxman_get_hdm_wlan_support();
}
EXPORT_SYMBOL(mxman_if_get_hdm_wlan_support);

int mxman_if_get_hdm_bt_support(void)
{
	return mxman_get_hdm_bt_support();
}
EXPORT_SYMBOL(mxman_if_get_hdm_bt_support);
#endif

