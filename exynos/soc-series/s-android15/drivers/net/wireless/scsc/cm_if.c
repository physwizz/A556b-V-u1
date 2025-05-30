/****************************************************************************
 *
 * Copyright (c) 2012 - 2022 Samsung Electronics Co., Ltd. All rights reserved
 *
 *       Chip Manager interface
 *
 ****************************************************************************/

#include "mgt.h"
#include "dev.h"
#include "debug.h"
#include "scsc_wifi_cm_if.h"
#include "utils.h"
#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
#include "hip4_sampler.h"
#endif
#include <linux/fs.h>

#include <scsc/scsc_mx.h>
#if IS_ENABLED(CONFIG_SCSC_LOG_COLLECTION)
#include <scsc/scsc_log_collector.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif

static bool EnableTestMode;
module_param(EnableTestMode, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(EnableTestMode, "Enable WlanLite test mode driver.");

#if defined(CONFIG_SCSC_RUNTIMEPM)
static int runtime_pm = -1;
static struct kobject *wifi_kobj_ref;
static ssize_t sysfs_show_runtime_pm(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t sysfs_store_runtime_pm(struct kobject *kobj, struct kobj_attribute *attr,
			      const char *buf, size_t count);
static struct kobj_attribute runtime_pm_attr = __ATTR(runtime_pm, 0660, sysfs_show_runtime_pm, sysfs_store_runtime_pm);


/* Register sysfs pm */
void slsi_create_sysfs_runtime_pm(void)
{
	int r;

	wifi_kobj_ref = mxman_wifi_kobject_ref_get();
	SLSI_INFO_NODEV("wifi_kobj_ref: 0x%p\n", wifi_kobj_ref);

	if (wifi_kobj_ref) {
		/* Create sysfs file /sys/wifi/runtime_pm */
		r = sysfs_create_file(wifi_kobj_ref, &runtime_pm_attr.attr);
		if (r) {
			/* Failed, so clean up dir */
			SLSI_ERR_NODEV("Can't create /sys/wifi/runtime_pm\n");
			return;
		}
	} else
		SLSI_ERR_NODEV("failed to create /sys/wifi/runtime_pm\n");
}

/* Unregister sysfs runtime_pm */
void slsi_destroy_sysfs_runtime_pm(void)
{
	if (!wifi_kobj_ref)
		return;

	/* Destroy /sys/wifi/runtime_pm file */
	sysfs_remove_file(wifi_kobj_ref, &runtime_pm_attr.attr);

	/* Destroy /sys/wifi virtual dir */
	mxman_wifi_kobject_ref_put();
	wifi_kobj_ref = NULL;
}
#endif

static BLOCKING_NOTIFIER_HEAD(slsi_wlan_notifier);

static struct mutex slsi_start_mutex;
static int recovery_in_progress;
static u16 latest_scsc_panic_code;

#define SLSI_RETRY_STOP_COUNT_ERROR 1

#if defined(CONFIG_SCSC_WLAN_HIP5)
#ifdef CONFIG_SCSC_BB_REDWOOD
#define HIP_MIFRAM_ALLOC_SIZE (3.75 * 1024 * 1024)
#else
#define HIP_MIFRAM_ALLOC_SIZE (4.65 * 1024 * 1024)
#endif
#else
#define HIP_MIFRAM_ALLOC_SIZE (2.75 * 1024 * 1024)
#endif

/* TODO: Would be good to get this removed - use module_client? */
struct slsi_cm_ctx {
	struct slsi_dev *sdev;
};

/* Only one wlan service instance is assumed for now. */
static struct slsi_cm_ctx cm_ctx;

static void slsi_hip_block_bh(struct slsi_dev *sdev);

#if defined(CONFIG_SCSC_RUNTIMEPM)
/* Retrieve runtime_pm in sysfs global */
static ssize_t sysfs_show_runtime_pm(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "%d\n", runtime_pm);
}

/* Update runtime_pm in sysfs global */
static ssize_t sysfs_store_runtime_pm(struct kobject *kobj,
			      struct kobj_attribute *attr,
			      const char *buf,
			      size_t count)
{
	int r;

	r = kstrtoint(buf, 10, &runtime_pm);
	if (r) {
		pr_err("failed to read Runtime PM value. error (%d)\n", r);
		return r;
	}

	switch (runtime_pm) {
	case -1:
		SLSI_INFO_NODEV("Runtime PM mode ON (vendor implementation)\n");
		break;
	case 0:
		SLSI_INFO_NODEV("Runtime PM mode OFF, Keep PCIe link On\n");
		break;
	case 100 ... 10000:
		SLSI_INFO_NODEV("Runtime PM mode set timer %dms\n", runtime_pm);
		break;
	default:
		SLSI_ERR_NODEV("unsupported value (supported values -1, 0, 100..10000) : %d\n", runtime_pm);
		return -ERANGE;
	}

	r = scsc_service_set_fw_runtime_pm(cm_ctx.sdev->service, runtime_pm);

	if (r)
		SLSI_ERR_NODEV("could not set fw_runtime_pm\n");

	return count;
}
#endif

#ifdef CONFIG_SEC_FACTORY
/* Only used for Factory Test */
static char factory_wifi_disable;
static int slsi_close_wifi_set(const char *val, const struct kernel_param *kp)
{
	struct net_device *dev = cm_ctx.sdev->netdev[SLSI_NET_INDEX_WLAN];
	struct netdev_vif *ndev_vif;

	if (!dev) {
		SLSI_ERR_NODEV("factory test wlan set failed\n");
		return 0;
	}

	factory_wifi_disable = val[0];
	if (!(factory_wifi_disable == '1' || factory_wifi_disable == 'Y' || factory_wifi_disable == 'y'))
		return 0;

	ndev_vif = netdev_priv(dev);
	if (!ndev_vif->is_available) {
		SLSI_INFO_NODEV("factory test wlan is disabled already\n");
		return 0;
	}

	rtnl_lock();
	dev_close(cm_ctx.sdev->netdev[SLSI_NET_INDEX_WLAN]);
	rtnl_unlock();

	return 0;
}

const struct kernel_param_ops slsi_factory_test_ops = {
	.set = &slsi_close_wifi_set,
};
module_param_cb(factory_wifi_disable, &slsi_factory_test_ops, &factory_wifi_disable, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(factory_wifi_disable, "factory test wifi enable/disable ops");
#endif

int slsi_wlan_service_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&slsi_wlan_notifier, nb);
}

int slsi_wlan_service_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&slsi_wlan_notifier, nb);
}

static int wlan_suspend(struct scsc_service_client *client)
{
	struct slsi_dev *sdev = container_of(client, struct slsi_dev, mx_wlan_client);

	SLSI_INFO_NODEV("Nofity registered functions\n");
	blocking_notifier_call_chain(&slsi_wlan_notifier, SCSC_WIFI_SUSPEND, sdev);

	return 0;
}

static int wlan_resume(struct scsc_service_client *client)
{
	struct slsi_dev *sdev = container_of(client, struct slsi_dev, mx_wlan_client);

	SLSI_INFO_NODEV("Nofity registered functions\n");
	blocking_notifier_call_chain(&slsi_wlan_notifier, SCSC_WIFI_RESUME, sdev);

	return 0;
}

static void slsi_append_log_to_system_buffer(struct slsi_dev *sdev)
{
	int    pos = 0;
	int    buf_size = 128;
	int    i = 0;
	char   log_to_sys_error_buffer[128] = { 0 };
	struct netdev_vif   *ndev_vif;

	pos = scnprintf(log_to_sys_error_buffer, buf_size, "netdev_up_count=%d ", sdev->netdev_up_count);

	for (i = 1; i <= CONFIG_SCSC_WLAN_MAX_INTERFACES; i++) {
		if (sdev->netdev[i]) {
			ndev_vif = netdev_priv(sdev->netdev[i]);
			if (ndev_vif->is_available)
				pos += scnprintf(log_to_sys_error_buffer + pos, buf_size - pos, "[%d: vif_type = %d] ", i, ndev_vif->vif_type);
		}
	}
	pos += scnprintf(log_to_sys_error_buffer + pos, buf_size - pos, "\n");
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
}

#ifdef CONFIG_SCSC_WLAN_FORCE_SILENT_RECOVERY
static u8 wlan_check_reset_level(struct scsc_service_client *client, u8 level)
{
	struct slsi_dev *sdev = container_of(client, struct slsi_dev, mx_wlan_client);

	if (sdev->forced_se_7) {
		level = atomic_read(&sdev->cm_if.reset_level);
		SLSI_INFO(sdev, "forced_se_7 is true: Host reset level = %d\n", level);
	}

	return level;
}

static bool slsi_reset_level_force_to_7_needed(struct slsi_dev *sdev)
{
	int                  i, level = 8;
	struct net_device    *dev;
	struct netdev_vif    *ndev_vif;

	if (!slsi_dev_silent_recovery_supported()) {
		SLSI_INFO(sdev, "silent_recovery_disabled from module param\n");
		return false;
	}

	/* RQMT-2171: FW panic should not be escalated to full reset.
	 * - If NAN is available, take action as level 8
	 * - If AP is available but not yet activated, take actions as Level 8,
	 * - Otherwise, (STA/P2P/AP-activated is ON), take actions same as Level 7 (silent recovery).
	 */
	complete_all(&sdev->sig_wait.completion);
	for (i = 1; i <= CONFIG_SCSC_WLAN_MAX_INTERFACES; i++) {
		if (sdev->netdev[i]) {
			dev = slsi_get_netdev(sdev, i);
			ndev_vif = netdev_priv(dev);
			complete_all(&ndev_vif->sig_wait.completion);

			SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
			SLSI_INFO(sdev, "silent recovery netdev[%d] : iftype %d available %d activated %d\n",
				  i, ndev_vif->iftype, ndev_vif->is_available, ndev_vif->activated);
			if (ndev_vif->is_available) {
				if (SLSI_IS_VIF_INDEX_WLAN(ndev_vif)) {
					level = 7;
				} else if (SLSI_IS_VIF_INDEX_P2P(ndev_vif)) {
					if (ndev_vif->activated)
						level = 7;
				} else if (SLSI_IS_VIF_INDEX_MHS(sdev, ndev_vif)) {
					if (!ndev_vif->activated) {
						SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
						return false;
					}
					level = 7;
				} else if (SLSI_IS_VIF_INDEX_NAN(ndev_vif)) {
					SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
					return false;
				}
			}
			SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		}
	}

	return (level == 7);
}
#else
static u8 wlan_check_reset_level(struct scsc_service_client *client, u8 level)
{
	return level;
}
#endif

static void slsi_send_full_reset_event_to_frwk(struct slsi_dev   *sdev, u16 vif_type)
{
	char    log_to_sys_error_buffer[128] = { 0 };

#if defined(SCSC_SEP_VERSION)
	SLSI_INFO_NODEV("Calling slsi_send_hanged_vendor_event (type %d), latest_scsc_panic_code=0x%x\n",
			vif_type, latest_scsc_panic_code);
	sprintf(log_to_sys_error_buffer, "Calling slsi_send_hanged_vendor_event (type %d), panic_code=0x%x\n",
		vif_type, latest_scsc_panic_code);
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	if (slsi_send_hanged_vendor_event(sdev, latest_scsc_panic_code) < 0) {
		SLSI_ERR(sdev, "Failed to send hang event\n");
		sprintf(log_to_sys_error_buffer, "Failed to send vendor hang event\n");
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
	}
#else
	char		  *error = "Subsystem Restart";

	SLSI_INFO_NODEV("Sending SLSI_NL80211_SUBSYSTEM_RESTART_EVENT (type = %d), panic_code=0x%x\n",
			vif_type, latest_scsc_panic_code);
	sprintf(log_to_sys_error_buffer, "Sending SLSI_NL80211_SUBSYSTEM_RESTART_EVENT (type = %d), panic_code=0x%x\n",
		vif_type, latest_scsc_panic_code);
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	if (slsi_vendor_event(sdev, SLSI_NL80211_SUBSYSTEM_RESTART_EVENT, error, strlen(error)) < 0) {
		SLSI_ERR(sdev, "Failed to send subsystem restart event\n");
		sprintf(log_to_sys_error_buffer, "Failed to send subsystem restart event\n");
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
	}
#endif
}

static u8 wlan_failure_notification(struct scsc_service_client *client, struct mx_syserr_decode *err)
{
	struct slsi_dev *sdev = container_of(client, struct slsi_dev, mx_wlan_client);
	char    log_to_sys_error_buffer[128] = { 0 };

	atomic_set(&sdev->cm_if.reset_level, err->level);
	SLSI_INFO_NODEV("SubSystem:%d,Level:%d,Type:%d,Sub_code:%d\n", err->subsys, err->level, err->type, err->subcode);

	sprintf(log_to_sys_error_buffer, "%s: SubSystem:%d,Level:%d,Type:%d,Sub_code:%d\n",
		__func__, err->subsys, err->level, err->type, err->subcode);
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	return err->level;
}

static void wlan_failure_reset_v2(struct scsc_service_client *client, u8 level, u16 scsc_syserr_code)
{
	struct slsi_dev *sdev = container_of(client, struct slsi_dev, mx_wlan_client);
	char    log_to_sys_error_buffer[128] = { 0 };

	SLSI_INFO_NODEV("Level:%d scsc_syserr_code:%d\n", level, scsc_syserr_code);

	sprintf(log_to_sys_error_buffer, "%s: Level:%d scsc_syserr_code:%d\n", __func__, level, scsc_syserr_code);
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	mutex_lock(&slsi_start_mutex);
	atomic_set(&sdev->cm_if.reset_level, level);
	mutex_unlock(&slsi_start_mutex);
	if (level < 5) {
		SLSI_WARN_NODEV("Low level error level:%d\n", level);
	} else if (level == 5 || level == 6) {
		blocking_notifier_call_chain(&slsi_wlan_notifier, SCSC_WIFI_SUBSYSTEM_RESET, sdev);
	} else if (level == SLSI_WIFI_CM_IF_SYSTEM_ERROR_PANIC) {
#if !defined(SCSC_SEP_VERSION) || defined(CONFIG_SCSC_WLAN_FORCE_SILENT_RECOVERY)
		if (sdev->forced_se_7) {
			mutex_lock(&slsi_start_mutex);
			atomic_set(&sdev->cm_if.reset_level, 7);
			mutex_unlock(&slsi_start_mutex);
		}
		sdev->forced_se_7 = false;
#endif
		latest_scsc_panic_code = scsc_syserr_code;
	}
}

static  bool wlan_stop_on_failure_v2(struct scsc_service_client *client, struct mx_syserr_decode *err)
{
	int               state;
	u8                system_error_level;
	struct slsi_dev   *sdev = container_of(client, struct slsi_dev, mx_wlan_client);
#ifndef SCSC_SEP_VERSION
	struct netdev_vif *wlan_dev_vif;
#endif
#ifdef CONFIG_SCSC_WLAN_AP_AUTO_RECOVERY
	struct netdev_vif *ndev_vif;
	int               i;
#endif

	SLSI_INFO_NODEV("state:%d, err_level:%d\n", sdev->cm_if.cm_if_state, err->level);
	mutex_lock(&slsi_start_mutex);
	recovery_in_progress = 1;
	sdev->recovery_status = 1;
#if !defined(SCSC_SEP_VERSION) || defined(CONFIG_SCSC_WLAN_FORCE_SILENT_RECOVERY)
	sdev->forced_se_7 = false;
#endif

	slsi_append_log_to_system_buffer(sdev);

	reinit_completion(&sdev->recovery_completed);

	state = atomic_read(&sdev->cm_if.cm_if_state);
	atomic_set(&sdev->cm_if.reset_level, err->level);
	/* system error level is set in failure_notification. if this is not yet set, consider
	 * a full panic. set it to SLSI_WIFI_CM_IF_SYSTEM_ERROR_PANIC
	 */
	system_error_level = atomic_read(&sdev->cm_if.reset_level);
	if (!system_error_level || system_error_level == SLSI_WIFI_CM_IF_SYSTEM_ERROR_PANIC) {
		atomic_set(&sdev->cm_if.reset_level, SLSI_WIFI_CM_IF_SYSTEM_ERROR_PANIC);
#ifndef SCSC_SEP_VERSION
		/* When wifi and softap is off and still wlan0 is up, recovery is not
		 * handled in upper layers.
		 * So force it to level7 to recover internally.
		 */
		if (sdev->netdev_up_count == 1 && sdev->netdev[SLSI_NET_INDEX_WLAN]) {
			wlan_dev_vif = netdev_priv(sdev->netdev[SLSI_NET_INDEX_WLAN]);
			if (wlan_dev_vif->vif_type != FAPI_VIFTYPE_AP && wlan_dev_vif->is_available) {
				atomic_set(&sdev->cm_if.reset_level, 7);
				sdev->forced_se_7 = true;
				SLSI_INFO_NODEV("Reducing reset level: L8->L7\n");
			}
		}
#endif

#ifdef CONFIG_SCSC_WLAN_FORCE_SILENT_RECOVERY
		mutex_unlock(&slsi_start_mutex);
		if (slsi_reset_level_force_to_7_needed(sdev)) {
			SLSI_INFO_NODEV("Reducing reset level (new policy): L8->L7\n");
			atomic_set(&sdev->cm_if.reset_level, 7);
			sdev->forced_se_7 = true;
		}
		mutex_lock(&slsi_start_mutex);
#endif
	}
	if (state != SCSC_WIFI_CM_IF_STATE_STOPPED) {
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_BLOCKED);
		sdev->fail_reported = true;

		/* If next state is stopped, then don't signal recovery since
		 * the Android framework won't/shouldn't restart (supplicant
		 * stop and start).
		 */
		if (sdev->recovery_next_state != SCSC_WIFI_CM_IF_STATE_STOPPING) {
			/* Stop wlan operations. Send event to registered parties */
			mutex_unlock(&slsi_start_mutex);
			SLSI_INFO_NODEV("Nofity registered functions\n");
			blocking_notifier_call_chain(&slsi_wlan_notifier, SCSC_WIFI_STOP, sdev);
			mutex_lock(&slsi_start_mutex);
#ifdef CONFIG_SCSC_WLAN_AP_AUTO_RECOVERY
			system_error_level = atomic_read(&sdev->cm_if.reset_level);
			if (system_error_level == SLSI_WIFI_CM_IF_SYSTEM_ERROR_PANIC)
				for (i = 1; i <= CONFIG_SCSC_WLAN_MAX_INTERFACES; i++)
					if (sdev->netdev[i]) {
						ndev_vif = netdev_priv(sdev->netdev[i]);
						if (ndev_vif->iftype == NL80211_IFTYPE_AP && ndev_vif->is_available) {
							slsi_send_full_reset_event_to_frwk(sdev, FAPI_VIFTYPE_AP);
							break;
							}
						}
#endif
		}
	} else {
		SLSI_INFO_NODEV("Wi-Fi service driver not started\n");
	}

	mutex_unlock(&slsi_start_mutex);
	SLSI_INFO_NODEV("Done!\n");
	return true;
}

/* WLAN service driver registration
 * ================================
 */
void slsi_wlan_service_probe(struct scsc_mx_module_client *module_client, struct scsc_mx *mx, enum scsc_module_client_reason reason)
{
	struct slsi_dev            *sdev;
	struct device              *dev;
	struct scsc_service_client mx_wlan_client;
#if IS_ENABLED(CONFIG_SCSC_LOG_COLLECTION)
	char buf[SCSC_LOG_FAPI_VERSION_SIZE];
#endif
	char    log_to_sys_error_buffer[128] = { 0 };
	int level;

	SLSI_UNUSED_PARAMETER(module_client);

	SLSI_INFO_NODEV("WLAN service probe\n");

	memset((void *)&mx_wlan_client, 0, (size_t)sizeof(mx_wlan_client));
	mutex_lock(&slsi_start_mutex);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (reason == SCSC_MODULE_CLIENT_REASON_RECOVERY_WPAN) {
		SLSI_INFO_NODEV("WLAN service probe - recovery. Ignore WPAN recovery.\n");
		goto done;
	}

	if (reason == SCSC_MODULE_CLIENT_REASON_RECOVERY_WLAN)
		reason = SCSC_MODULE_CLIENT_REASON_RECOVERY;
#endif

	if ((reason == SCSC_MODULE_CLIENT_REASON_RECOVERY || reason == SCSC_MODULE_CLIENT_REASON_RECOVERY_WLAN) 
		&& !recovery_in_progress)
		goto done;

	SLSI_INFO_NODEV("reason: %d, recovery_in_progress : %d\n", reason, recovery_in_progress);
	if (reason == SCSC_MODULE_CLIENT_REASON_RECOVERY || reason == SCSC_MODULE_CLIENT_REASON_RECOVERY_WLAN) {
		SLSI_INFO_NODEV("Probe recovery\n");
		sdev = cm_ctx.sdev;
		recovery_in_progress = 0;
		sdev->fail_reported = false;
		sdev->recovery_status = 0;
		mutex_unlock(&slsi_start_mutex);
		blocking_notifier_call_chain(&slsi_wlan_notifier, SCSC_WIFI_CHIP_READY, sdev);
		mutex_lock(&slsi_start_mutex);
		complete_all(&sdev->recovery_completed);
		sprintf(log_to_sys_error_buffer, "%s: recovery_completed\n", __func__);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
	} else {
		/* Register callbacks */
		mx_wlan_client.failure_notification = wlan_failure_notification;
		mx_wlan_client.stop_on_failure_v2   = wlan_stop_on_failure_v2;
		mx_wlan_client.failure_reset_v2     = wlan_failure_reset_v2;
		mx_wlan_client.suspend           = wlan_suspend;
		mx_wlan_client.resume            = wlan_resume;
		mx_wlan_client.check_reset_level = wlan_check_reset_level;

		dev = scsc_service_get_device_by_mx(mx);

		/* The mutex must be released at this point since the attach
		 * process may call various functions including
		 * slsi_sm_wlan_service_start and slsi_sm_wlan_service_open, which will
		 * claim the same mutex.
		 */
		mutex_unlock(&slsi_start_mutex);
		sdev = slsi_dev_attach(dev, mx, &mx_wlan_client);
		mutex_lock(&slsi_start_mutex);
		if (!sdev) {
			SLSI_ERR_NODEV("WLAN attach failed - slsi_dev_attach\n");
			goto done;
		}

		cm_ctx.sdev = sdev; /* TODO: For now. */

		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_PROBING);
		get_device(dev);

#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
		hip4_sampler_create(sdev, mx);
#endif
#if IS_ENABLED(CONFIG_SCSC_LOG_COLLECTION)
		memset(buf, 0, SCSC_LOG_FAPI_VERSION_SIZE);
		/* Write FAPI VERSION to collector header */
		/* IMPORTANT - Do not change the formatting as User space tooling is parsing the string
		 * to read SAP fapi versions.
		 */
		snprintf(buf, SCSC_LOG_FAPI_VERSION_SIZE, "ma:%u.%u, mlme:%u.%u, debug:%u.%u, test:%u.%u",
			 FAPI_MAJOR_VERSION(FAPI_DATA_SAP_VERSION), FAPI_MINOR_VERSION(FAPI_DATA_SAP_VERSION),
			 FAPI_MAJOR_VERSION(FAPI_CONTROL_SAP_VERSION), FAPI_MINOR_VERSION(FAPI_CONTROL_SAP_VERSION),
			 FAPI_MAJOR_VERSION(FAPI_DEBUG_SAP_VERSION), FAPI_MINOR_VERSION(FAPI_DEBUG_SAP_VERSION),
			 FAPI_MAJOR_VERSION(FAPI_TEST_SAP_VERSION), FAPI_MINOR_VERSION(FAPI_TEST_SAP_VERSION));

		scsc_log_collector_write_fapi(buf, SCSC_LOG_FAPI_VERSION_SIZE);
#endif
	}

	if (reason != SCSC_MODULE_CLIENT_REASON_RECOVERY && reason != SCSC_MODULE_CLIENT_REASON_RECOVERY_WLAN)
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_PROBED);
	level = atomic_read(&sdev->cm_if.reset_level);
	if (level != SLSI_WIFI_CM_IF_SILENT_RECOVERY)
		atomic_set(&sdev->cm_if.reset_level, 0);

done:
	mutex_unlock(&slsi_start_mutex);
}

/* service_clean_up_locked expects the slsi_start_mutex mutex to be claimed when
 * service_clean_up_locked is called.
 */
static void service_clean_up_locked(struct slsi_dev *sdev)
{
	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_REMOVING);
	put_device(sdev->dev);

	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_REMOVED);

	sdev->maxwell_core = NULL;

	/* The mutex must be released at this point since the tear down
	 * process will call various functions including
	 * slsi_sm_wlan_service_stop and slsi_sm_wlan_service_close, which will
	 * claim the same mutex.
	 */
	mutex_unlock(&slsi_start_mutex);
	slsi_dev_detach(sdev);
	mutex_lock(&slsi_start_mutex);
}

static void slsi_wlan_service_remove(struct scsc_mx_module_client *module_client, struct scsc_mx *mx, enum scsc_module_client_reason reason)
{
	struct slsi_dev *sdev;
	int             state;
	int level = 0;
	char    log_to_sys_error_buffer[128] = { 0 };

	SLSI_UNUSED_PARAMETER(mx);
	SLSI_UNUSED_PARAMETER(module_client);

	sdev = cm_ctx.sdev;
	if (!sdev) {
		SLSI_INFO_NODEV("no sdev\n");
		return;
	}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (reason == SCSC_MODULE_CLIENT_REASON_RECOVERY_WPAN) {
		SLSI_INFO_NODEV("WLAN service remove - recovery. Ignore WPAN recovery.\n");
		return;
	}

	if (reason == SCSC_MODULE_CLIENT_REASON_RECOVERY_WLAN)
		reason = SCSC_MODULE_CLIENT_REASON_RECOVERY;
#endif
	SLSI_INFO_NODEV("WLAN service remove - reason %d, recovery_in_progress %d\n", reason, recovery_in_progress);
	if ((reason == SCSC_MODULE_CLIENT_REASON_RECOVERY || reason == SCSC_MODULE_CLIENT_REASON_RECOVERY_WLAN)
		&& !recovery_in_progress) {
		SLSI_INFO_NODEV("WLAN service remove - recovery. Service not active.\n");
	} else if ((reason == SCSC_MODULE_CLIENT_REASON_RECOVERY || reason == SCSC_MODULE_CLIENT_REASON_RECOVERY_WLAN)
				&& recovery_in_progress) {
		int r;

		level = atomic_read(&sdev->cm_if.reset_level);

		SLSI_INFO_NODEV("WLAN service remove - recovery, Error Level:%d\n", level);

		sprintf(log_to_sys_error_buffer, "%s: recovery, Error Level:%d\n", __func__, level);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

		/* Only indicate if the next state is not stopping. The recovery
		 * handling won't have any affect if the framework is closing
		 * anyway.
		 */
		if (sdev->recovery_next_state != SCSC_WIFI_CM_IF_STATE_STOPPING) {
			SLSI_INFO_NODEV("Nofity registered functions\n");
			blocking_notifier_call_chain(&slsi_wlan_notifier, SCSC_WIFI_FAILURE_RESET, sdev);
		}
		if (level == SLSI_WIFI_CM_IF_SYSTEM_ERROR_PANIC) {
			mutex_lock(&slsi_start_mutex);
		/**
		 * If there was a request to stop during the recovery, then do
		 * not sent a hang - just stop here. The Wi-Fi service driver is
		 * ready to be turned on again. Let the service_stop complete.
		 */
			complete_all(&sdev->recovery_remove_completion);
			if (sdev->recovery_next_state == SCSC_WIFI_CM_IF_STATE_STOPPING) {
				SLSI_INFO_NODEV("Recovery - next state stopping\n");
			} else {
				slsi_send_full_reset_event_to_frwk(sdev, FAPI_VIFTYPE_STATION);

				/* Complete any pending ctrl signals, which will prevent
				 * the hang event from being processed.
				 */
				complete_all(&sdev->sig_wait.completion);
			}

			mutex_unlock(&slsi_start_mutex);

			r = wait_for_completion_timeout(&sdev->recovery_stop_completion,
							msecs_to_jiffies(sdev->recovery_timeout));
			if (r == 0) {
				SLSI_INFO(sdev, "recovery_stop_completion timedout, netdev_up_count = %d\n", sdev->netdev_up_count);
				sprintf(log_to_sys_error_buffer, "recovery_stop_completion timedout[%d]\n",
					sdev->recovery_timeout);
				slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
				if (sdev->netdev_up_count) {
					sdev->recovery_fail_safe = true;
					SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
					slsi_stop_chip(sdev);
					SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
					queue_work(sdev->device_wq, &sdev->system_error_user_fail_work);
				}
			}
		}
		mutex_lock(&slsi_start_mutex);
		reinit_completion(&sdev->recovery_stop_completion);
		mutex_unlock(&slsi_start_mutex);

	} else {
		SLSI_INFO_NODEV("WLAN service remove\n");

#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
		hip4_sampler_destroy(sdev, mx);
#endif

		mutex_lock(&slsi_start_mutex);
		state = atomic_read(&sdev->cm_if.cm_if_state);
		if (state != SCSC_WIFI_CM_IF_STATE_STARTED &&
		    state != SCSC_WIFI_CM_IF_STATE_PROBED &&
		    state != SCSC_WIFI_CM_IF_STATE_STOPPED &&
		    state != SCSC_WIFI_CM_IF_STATE_BLOCKED) {
			mutex_unlock(&slsi_start_mutex);
			SLSI_INFO_NODEV("state-event error %d\n", state);
			return;
		}

		service_clean_up_locked(sdev);
		mutex_unlock(&slsi_start_mutex);
	}
	SLSI_INFO_NODEV("Done!\n");
}

/* Block future HIP runs through the hip_switch */
static void slsi_hip_block_bh(struct slsi_dev *sdev)
{
	SLSI_WARN(sdev, "HIP state set to #SLSI_HIP_STATE_BLOCKED#\n");
	atomic_set(&sdev->hip.hip_state, SLSI_HIP_STATE_BLOCKED);
}

static struct scsc_mx_module_client wlan_driver = {
	.name = "WLAN driver",
	.probe = slsi_wlan_service_probe,
	.remove = slsi_wlan_service_remove,
};

int slsi_sm_service_driver_register(void)
{
	struct slsi_cm_ctx *ctx = &cm_ctx;

	memset(ctx, 0, sizeof(*ctx));
	mutex_init(&slsi_start_mutex);
	scsc_mx_module_register_client_module(&wlan_driver);

	return 0;
}

void slsi_sm_service_driver_unregister(void)
{
	scsc_mx_module_unregister_client_module(&wlan_driver);
}

/* start/stop wlan service
 * =======================
 */
void slsi_sm_service_failed(struct slsi_dev *sdev, const char *reason, bool is_work)
{
	int state;

	mutex_lock(&slsi_start_mutex);

	if (is_work)
		complete_all(&sdev->service_fail_started_indication);

	state = atomic_read(&sdev->cm_if.cm_if_state);
	if (state != SCSC_WIFI_CM_IF_STATE_STARTED &&
	    state != SCSC_WIFI_CM_IF_STATE_STOPPING) {
		mutex_unlock(&slsi_start_mutex);
		SLSI_INFO(sdev, "State %d - ignoring event\n", state);
		return;
	}

	/* Limit the volume of error reports to the core */
	if (!sdev->fail_reported) {
		/* This log may be scraped by test systems */
		SLSI_ERR(sdev, "scsc_wifibt: FATAL ERROR: %s\n", reason);

		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_BLOCKED);
		slsi_hip_block_bh(sdev);
		scsc_mx_service_service_failed(sdev->service, reason);
		scsc_mx_service_mif_dump_registers(sdev->service);
		sdev->fail_reported = true;
	}

	if (is_work)
		reinit_completion(&sdev->service_fail_started_indication);

	mutex_unlock(&slsi_start_mutex);
}

/* Is production test mode enabled? */
bool slsi_is_test_mode_enabled(void)
{
	return EnableTestMode;
}

#define SLSI_SM_WLAN_SERVICE_RECOVERY_COMPLETED_TIMEOUT 20000
#define SLSI_SM_WLAN_SERVICE_RECOVERY_DISABLED_TIMEOUT 2000

int slsi_sm_recovery_service_stop(struct slsi_dev *sdev)
{
	int err = 0;
	char             log_to_sys_error_buffer[128] = { 0 };

	mutex_lock(&slsi_start_mutex);
	SLSI_INFO_NODEV("Stopping WLAN service\n");
	sprintf(log_to_sys_error_buffer, "Stopping WLAN service\n");
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	sdev->wlan_service_on = 0;
	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPING);

	if (!sdev->service) {
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		SLSI_WARN(sdev, "Service is NULL\n");
		sprintf(log_to_sys_error_buffer, "Service is NULL err=%d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		err = -EINVAL;
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	err = scsc_mx_service_stop(sdev->service);
	if (err == -EILSEQ || err == -EIO)
		SLSI_INFO(sdev, "scsc_mx_service_stop failed err: %d\n", err);
	else
		err = 0;
	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
	mutex_unlock(&slsi_start_mutex);
	return err;
}

int slsi_sm_recovery_service_close(struct slsi_dev *sdev)
{
	int err = 0;
	char             log_to_sys_error_buffer[128] = { 0 };

	mutex_lock(&slsi_start_mutex);
	if (!sdev->service) {
		SLSI_WARN(sdev, "sdev->service is NULL\n");
		mutex_unlock(&slsi_start_mutex);
		return -EINVAL;
	}

	SLSI_INFO_NODEV("Closing WLAN service\n");
	sprintf(log_to_sys_error_buffer, "Closing WLAN service\n");
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	scsc_mx_service_mifram_free(sdev->service, sdev->hip.hip_ref);
	err = scsc_mx_service_close(sdev->service);
	if (err == -EILSEQ || err == -EIO)
		SLSI_INFO(sdev, "scsc_mx_service_close failed err: %d\n", err);
	else
		err = 0;
	if (!err)
		sdev->service = NULL;
	mutex_unlock(&slsi_start_mutex);
	return err;
}

int slsi_sm_recovery_service_open(struct slsi_dev *sdev)
{
	int err = 0;
	char             log_to_sys_error_buffer[128] = { 0 };

	mutex_lock(&slsi_start_mutex);
	SLSI_INFO_NODEV("Open WLAN service\n");
	sprintf(log_to_sys_error_buffer, "Open WLAN service\n");
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	sdev->service = scsc_mx_service_open(sdev->maxwell_core, SCSC_SERVICE_ID_WLAN, &sdev->mx_wlan_client, &err);
	if (err == -EILSEQ || err == -EIO)
		SLSI_INFO(sdev, "scsc_mx_service_open failed err: %d\n", err);
	else
		err = 0;
	mutex_unlock(&slsi_start_mutex);
	return err;
}

int slsi_sm_recovery_service_start(struct slsi_dev *sdev)
{
	int err = 0, err2 = 0;
	struct slsi_hip *hip = &sdev->hip;
	scsc_mifram_ref  ref;
	int              state;
	char             log_to_sys_error_buffer[128] = { 0 };

	mutex_lock(&slsi_start_mutex);
	state = atomic_read(&sdev->cm_if.cm_if_state);
	SLSI_INFO(sdev,
		  "Recovery -- Status:%d  In_Progress:%d  -- cm_if_state:%d\n",
		  sdev->recovery_status, recovery_in_progress, state);
	if (state != SCSC_WIFI_CM_IF_STATE_PROBED &&
	    state != SCSC_WIFI_CM_IF_STATE_STOPPED) {
		SLSI_INFO(sdev, "State-event error %d\n", state);
		mutex_unlock(&slsi_start_mutex);
		return -EINVAL;
	}

	if (!sdev->service) {
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		SLSI_WARN(sdev, "Service is NULL\n");
		sprintf(log_to_sys_error_buffer, "Service is NULL err=%d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		err = -EINVAL;
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STARTING);

	err = scsc_mx_service_mifram_alloc(sdev->service, HIP_MIFRAM_ALLOC_SIZE, &sdev->hip.hip_ref, 4096);
	if (err) {
		SLSI_WARN(sdev, "scsc_mx_service_mifram_alloc failed err: %d\n", err);
		sprintf(log_to_sys_error_buffer, "scsc_mx_service_mifram_alloc failed err: %d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	err = slsi_hip_start(sdev);
	if (err) {
		SLSI_WARN(sdev, "slsi_hip_start failed err: %d\n", err);
		sprintf(log_to_sys_error_buffer, "slsi_hip_start failed err: %d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	err = scsc_mx_service_mif_ptr_to_addr(sdev->service, hip->hip_control, &ref);
	if (err) {
		SLSI_WARN(sdev, "scsc_mx_service_mif_ptr_to_addr failed err: %d\n", err);
		sprintf(log_to_sys_error_buffer, "scsc_mx_service_mif_ptr_to_addr failed err: %d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	err = scsc_mx_service_start(sdev->service, ref);
	if (err) {
		SLSI_WARN(sdev, "scsc_mx_service_start failed err: %d\n", err);
		sprintf(log_to_sys_error_buffer, "scsc_mx_service_start failed err: %d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		slsi_hip_stop(sdev);
		if (err == -EILSEQ) {
			sdev->cm_if.recovery_state = SLSI_RECOVERY_SERVICE_STOPPED;
			sdev->require_service_close = true;
		}
		mutex_unlock(&slsi_start_mutex);
		return err;
	}
	err = slsi_hip_setup_ext(sdev);
	if (err) {
		SLSI_WARN(sdev, "slsi_hip_setup failed err: %d\n", err);
		sprintf(log_to_sys_error_buffer, "slsi_hip_setup failed err: %d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		err2 = scsc_mx_service_stop(sdev->service);
		if (err2)
			SLSI_INFO(sdev, "scsc_mx_service_stop failed err2: %d\n", err2);
		if (err2 == -EILSEQ) {
			sdev->cm_if.recovery_state = SLSI_RECOVERY_SERVICE_STOPPED;
			sdev->require_service_close = true;
		}
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}
	/* Service has started, inform SAP versions to the registered SAPs */
	err = slsi_hip_sap_setup(sdev);
	if (err) {
		SLSI_WARN(sdev, "slsi_hip_sap_setup failed err: %d\n", err);
		sprintf(log_to_sys_error_buffer, "slsi_hip_sap_setup failed err: %d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		err2 = scsc_mx_service_stop(sdev->service);
		if (err2)
			SLSI_INFO(sdev, "scsc_mx_service_stop failed err2: %d\n", err2);
		if (err2 == -EILSEQ) {
			sdev->cm_if.recovery_state = SLSI_RECOVERY_SERVICE_STOPPED;
			sdev->require_service_close = true;
		}
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}
	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STARTED);
	sdev->wlan_service_on = 1;
	sprintf(log_to_sys_error_buffer, "%s: wlan_service_on[1]\n", __func__);
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
	sdev->require_service_close = false;
	mutex_unlock(&slsi_start_mutex);
	return err;
}

int slsi_sm_wlan_service_open(struct slsi_dev *sdev)
{
	int err = 0;
	int state;
	char             log_to_sys_error_buffer[128] = { 0 };

	mutex_lock(&slsi_start_mutex);
	state = atomic_read(&sdev->cm_if.cm_if_state);
	if (state != SCSC_WIFI_CM_IF_STATE_PROBED &&
	    state != SCSC_WIFI_CM_IF_STATE_STOPPED) {
		SLSI_INFO(sdev, "State-event error %d\n", state);
		err = -EINVAL;
		goto exit;
	}

	sdev->recovery_timeout = mxman_recovery_disabled() ? SLSI_SM_WLAN_SERVICE_RECOVERY_DISABLED_TIMEOUT : SLSI_SM_WLAN_SERVICE_RECOVERY_COMPLETED_TIMEOUT;

	/* Open service - will download FW - will set MBOX0 with Starting address */
	SLSI_INFO(sdev, "Open WLAN service\n");
	sprintf(log_to_sys_error_buffer, "Open WLAN service, recovery timeout = %d\n", sdev->recovery_timeout);
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	sdev->service = scsc_mx_service_open(sdev->maxwell_core, SCSC_SERVICE_ID_WLAN, &sdev->mx_wlan_client, &err);
	if (!sdev->service) {
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_PROBED);
		SLSI_WARN(sdev, "Service open failed\n");
		sprintf(log_to_sys_error_buffer, "Service open failed, err=%d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		err = -EINVAL;
		goto exit;
	}

exit:
	mutex_unlock(&slsi_start_mutex);
	return err;
}

#if defined(CONFIG_SLUB_DEBUG_ON) || defined(CONFIG_DEBUG_PREEMPT) || defined(CONFIG_DEBUG_RT_MUTEXES) || \
	defined(CONFIG_DEBUG_SPINLOCK) && defined(CONFIG_DEBUG_MUTEXES) && defined(CONFIG_DEBUG_LOCK_ALLOC) || \
	defined(CONFIG_DEBUG_LOCK_ALLOC) && defined(CONFIG_DEBUG_ATOMIC_SLEEP) && defined(CONFIG_DEBUG_LIST)
#define KERNEL_DEBUG_OPTIONS_ENABLED
#endif

int slsi_sm_wlan_service_start(struct slsi_dev *sdev)
{
	struct slsi_hip *hip = &sdev->hip;
	scsc_mifram_ref  ref;
	int              err = 0;
	int              err2 = 0;
	int              state;
	char             log_to_sys_error_buffer[128] = { 0 };

	mutex_lock(&slsi_start_mutex);
	state = atomic_read(&sdev->cm_if.cm_if_state);
	SLSI_INFO(sdev,
		  "Recovery -- Status:%d  In_Progress:%d  -- cm_if_state:%d\n",
		  sdev->recovery_status, recovery_in_progress, state);

	sprintf(log_to_sys_error_buffer, "%s: recovery_status = %d, recovery_in_progress = %d\n",
		__func__, sdev->recovery_status, recovery_in_progress);
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	if (state != SCSC_WIFI_CM_IF_STATE_PROBED &&
	    state != SCSC_WIFI_CM_IF_STATE_STOPPED) {
		SLSI_INFO(sdev, "State-event error %d\n", state);
		mutex_unlock(&slsi_start_mutex);
		return -EINVAL;
	}

	if (!sdev->service) {
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		SLSI_WARN(sdev, "Service is NULL\n");
		sprintf(log_to_sys_error_buffer, "Service is NULL err=%d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		err = -EINVAL;
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STARTING);

#ifdef KERNEL_DEBUG_OPTIONS_ENABLED
	SLSI_WARN(sdev, "Kernel config debug options are enabled. This might impact the throughput performance.\n");
#endif

	/* Get RAM from the MIF */
	SLSI_INFO(sdev, "Allocate mifram size %lu Bytes\n", HIP_MIFRAM_ALLOC_SIZE);

	err = scsc_mx_service_mifram_alloc(sdev->service, HIP_MIFRAM_ALLOC_SIZE, &sdev->hip.hip_ref, 4096);
	if (err) {
		SLSI_WARN(sdev, "scsc_mx_service_mifram_alloc failed err: %d\n", err);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	SLSI_INFO(sdev, "Start HIP\n");
	err = slsi_hip_start(sdev);
	if (err) {
		SLSI_WARN(sdev, "slsi_hip_start failed err: %d\n", err);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	err = scsc_mx_service_mif_ptr_to_addr(sdev->service, hip->hip_control, &ref);
	if (err) {
		SLSI_WARN(sdev, "scsc_mx_service_mif_ptr_to_addr failed err: %d\n", err);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	SLSI_INFO(sdev, "Starting WLAN service\n");
	err = scsc_mx_service_start(sdev->service, ref);
	if (err) {
		SLSI_WARN(sdev, "scsc_mx_service_start failed err: %d\n", err);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		if (err == -EILSEQ) {
			sdev->cm_if.recovery_state = SLSI_RECOVERY_SERVICE_STOPPED;
			sdev->require_service_close = true;
		}
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err;
	}
	err = slsi_hip_setup_ext(sdev);
	if (err) {
		SLSI_WARN(sdev, "slsi_hip_setup failed err: %d\n", err);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		SLSI_INFO_NODEV("Stopping WLAN service\n");
		err2 = scsc_mx_service_stop(sdev->service);
		if (err2)
			SLSI_INFO(sdev, "scsc_mx_service_stop failed err2: %d\n", err2);
		err2 = scsc_mx_service_stop(sdev->service);
		if (err2)
			SLSI_INFO(sdev, "scsc_mx_service_stop failed err2: %d\n", err2);
		if (err2 == -EILSEQ) {
			sdev->cm_if.recovery_state = SLSI_RECOVERY_SERVICE_STOPPED;
			sdev->require_service_close = true;
		}
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err2;
	}
	/* Service has started, inform SAP versions to the registered SAPs */
	err = slsi_hip_sap_setup(sdev);
	if (err) {
		SLSI_WARN(sdev, "slsi_hip_sap_setup failed err: %d\n", err);
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		SLSI_INFO_NODEV("Stopping WLAN service\n");
		err2 = scsc_mx_service_stop(sdev->service);
		if (err2)
			SLSI_INFO(sdev, "scsc_mx_service_stop failed err2: %d\n", err2);
		if (err2 == -EILSEQ) {
			sdev->cm_if.recovery_state = SLSI_RECOVERY_SERVICE_STOPPED;
			sdev->require_service_close = true;
		}
		slsi_hip_stop(sdev);
		mutex_unlock(&slsi_start_mutex);
		return err2;
	}

#if defined(CONFIG_SCSC_RUNTIMEPM)
	slsi_create_sysfs_runtime_pm();
	err = scsc_service_set_fw_runtime_pm(sdev->service, runtime_pm);
	if (err)
		SLSI_ERR_NODEV("Could not set runtime_pm (%d). err (%d)\n", runtime_pm, err);
#endif

	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STARTED);
	sdev->wlan_service_on = 1;
	sdev->require_service_close = false;
	mutex_unlock(&slsi_start_mutex);
	return 0;
}

static void __slsi_sm_wlan_service_stop_wait_locked(struct slsi_dev *sdev)
{
	int r;
	char    log_to_sys_error_buffer[128] = { 0 };

	mutex_unlock(&slsi_start_mutex);
	r = wait_for_completion_timeout(&sdev->recovery_remove_completion,
					msecs_to_jiffies(sdev->recovery_timeout));
	if (r == 0) {
		SLSI_INFO(sdev, "recovery_remove_completion timeout\n");

		sprintf(log_to_sys_error_buffer, "%s: recovery_remove_completion timeout\n", __func__);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
	}

	mutex_lock(&slsi_start_mutex);
	sdev->recovery_next_state = SCSC_WIFI_CM_IF_STATE_STOPPED;

	reinit_completion(&sdev->recovery_remove_completion);
}

int slsi_sm_wlan_service_stop(struct slsi_dev *sdev)
{
	int cm_if_state;
	int err = 0;
	/* Variable to track number of stop request made to maxwell manager */
	u8 retry_count = SLSI_RETRY_STOP_COUNT_ERROR;
	char             log_to_sys_error_buffer[128] = { 0 };

	mutex_lock(&slsi_start_mutex);
	cm_if_state = atomic_read(&sdev->cm_if.cm_if_state);
	SLSI_INFO(sdev,
		  "Recovery -- Status:%d  In_Progress:%d  -- cm_if_state:%d\n",
		  sdev->recovery_status, recovery_in_progress, cm_if_state);

	if (!sdev->service) {
		atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
		SLSI_WARN(sdev, "Service is NULL\n");
		sprintf(log_to_sys_error_buffer, "Service is NULL err=%d\n", err);
		slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);
		err = -EINVAL;
		mutex_unlock(&slsi_start_mutex);
		return err;
	}

	if (cm_if_state == SCSC_WIFI_CM_IF_STATE_BLOCKED) {
		__slsi_sm_wlan_service_stop_wait_locked(sdev);
		/* service stop is called from another context*/
		if (atomic_read(&sdev->cm_if.cm_if_state) == SCSC_WIFI_CM_IF_STATE_STOPPED) {
			SLSI_INFO(sdev, "skip mx_service_close cm_if_state=%d\n", SCSC_WIFI_CM_IF_STATE_STOPPED);
			mutex_unlock(&slsi_start_mutex);
			return -EUSERS;
		}

		/* If the wait hasn't timed out, the recovery remove completion
		 * will have completed properly and the cm_if_state will be
		 * set to stopped here. If the probe hasn't fired for some reason
		 * try and do a service_stop regardless, since that's all we can
		 * do in this situation; hence skip the state check.
		 */
		goto skip_state_check;
	}

	if (cm_if_state != SCSC_WIFI_CM_IF_STATE_STARTED &&
	    cm_if_state != SCSC_WIFI_CM_IF_STATE_REMOVED &&
	    cm_if_state != SCSC_WIFI_CM_IF_STATE_PROBED) {
		SLSI_INFO(sdev, "Service not started or incorrect state %d\n",
			  cm_if_state);
		goto exit;
	}

	/**
	 * Note that the SCSC_WIFI_CM_IF_STATE_STOPPING state will inhibit
	 * auto-recovery mechanism, so be careful not to abuse it: as an
	 * example if panic happens on start or stop we don't want to
	 * un-necessarily pass by STOPPING in order to have a successful
	 * recovery in such a situation.
	 */
	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPING);
skip_state_check:
	SLSI_INFO_NODEV("Stopping WLAN service\n");
	sprintf(log_to_sys_error_buffer, "%s: Stopping WLAN service\n", __func__);

	err = scsc_mx_service_stop(sdev->service);
	if (err == -EILSEQ) {
		/* scsc_mx_service_stop failed since there's a recovery in
		 * progress, so just wait for it to complete.
		 */
		SLSI_INFO(sdev, "scsc_mx_service_stop failed err: %d attempts left: %d\n", err, retry_count);
		__slsi_sm_wlan_service_stop_wait_locked(sdev);
		/* Wait for 20 second should be enough for recovery procedure to complete */
		/* Need to close the service manually when receive remove callback*/
		if (!retry_count)
			sdev->require_service_close = true;
		if (retry_count--)
			goto skip_state_check;
	} else if (err == -EIO) {
		char reason[80];

		SLSI_INFO(sdev, "scsc_mx_service_stop failed err: %d\n", err);

		/* scsc_mx_service_stop since there was no respons from firmware
		 * to the stop request. Generate a host initiated panic to reset
		 * the chip and wait for it to complete.
		 */
		sdev->recovery_next_state = SCSC_WIFI_CM_IF_STATE_STOPPING;
		snprintf(reason, sizeof(reason), "WLAN scsc_mx_service_stop failed");

		mutex_unlock(&slsi_start_mutex);
		slsi_sm_service_failed(sdev, reason, false);
		mutex_lock(&slsi_start_mutex);

		__slsi_sm_wlan_service_stop_wait_locked(sdev);
	} else if (err == -EPERM) {
		/* Special case when recovery is disabled, otherwise the driver
		 * will wait forever for recovery that never comes
		 */
		SLSI_INFO(sdev, "refused due to previous failure, recovery is disabled: %d\n", err);
	} else if (err != 0) {
		SLSI_INFO(sdev, "scsc_mx_service_stop failed, unknown err: %d\n", err);
	}

	atomic_set(&sdev->cm_if.cm_if_state, SCSC_WIFI_CM_IF_STATE_STOPPED);
exit:
#if defined(CONFIG_SCSC_RUNTIMEPM)
	/* reset to vendor implmentation as expected */
	runtime_pm = -1;
	err = scsc_service_set_fw_runtime_pm(sdev->service, runtime_pm);
	if (err)
		SLSI_ERR_NODEV("Could not set runtime_pm (%d). err (%d)\n", runtime_pm, err);
	slsi_destroy_sysfs_runtime_pm();
#endif
	mutex_unlock(&slsi_start_mutex);
	return 0;
}

#define SLSI_SM_WLAN_SERVICE_CLOSE_RETRY 60
void slsi_sm_wlan_service_close(struct slsi_dev *sdev)
{
	int cm_if_state, r = 0;
	char             log_to_sys_error_buffer[128] = { 0 };

	mutex_lock(&slsi_start_mutex);
	sdev->wlan_service_on = 0;
	cm_if_state = atomic_read(&sdev->cm_if.cm_if_state);
	if (cm_if_state != SCSC_WIFI_CM_IF_STATE_STOPPED) {
		SLSI_INFO(sdev, "Service not stopped. cm_if_state = %d service is = %p\n", cm_if_state, sdev->service);

		/**
		 * Close the service if failure has occurred after service has successfully opened
		 * but before service has attempted to start
		 */
		if (cm_if_state == SCSC_WIFI_CM_IF_STATE_PROBED) {
			SLSI_INFO_NODEV("Closing WLAN service on error\n");
			if (sdev->service)
				r = scsc_mx_service_close(sdev->service);
		}
		goto exit;
	}

	SLSI_INFO_NODEV("Closing WLAN service. Service is %d\n", sdev->service);

	sprintf(log_to_sys_error_buffer, "%s: Closing WLAN service. Service is %p\n", __func__, sdev->service);
	slsi_add_log_to_system_error_buffer(sdev, log_to_sys_error_buffer);

	if (sdev->service) {
		scsc_mx_service_mifram_free(sdev->service, sdev->hip.hip_ref);
		r = scsc_mx_service_close(sdev->service);
		if (r == -EIO) {
			/**
			 * Error handling in progress
			 */
			sdev->require_service_close = true;
			sdev->cm_if.recovery_state = SLSI_RECOVERY_SERVICE_STOPPED;
			SLSI_INFO(sdev, "scsc_mx_service_close failed with error:%d\n", r);
		} else if (r == -EPERM) {
			SLSI_ERR(sdev, "scsc_mx_service_close - recovery is disabled (%d)\n", r);
		}
	}

	if (recovery_in_progress)
		complete_all(&sdev->recovery_stop_completion);
exit:
	if (!r)
		sdev->service = NULL;
	mutex_unlock(&slsi_start_mutex);
}

struct slsi_dev *slsi_get_sdev(void)
{
	return cm_ctx.sdev;
}
