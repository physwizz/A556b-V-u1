/****************************************************************************
 *
 * Copyright (c) 2014 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/firmware.h>
#ifdef CONFIG_SCSC_COMMON_ANDROID
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <scsc/scsc_wakelock.h>
#else
#include <linux/wakelock.h>
#endif
#endif
#include <scsc/scsc_mx.h>
#include <scsc/scsc_logring.h>
#include <scsc/scsc_log_collector.h>

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#include "mxman_if.h"
#else
#include "mxman.h"
#endif
#include "scsc_mx_impl.h"
#include "mifintrbit.h"
#include "miframman.h"
#include "mifmboxman.h"
#ifdef CONFIG_SCSC_SMAPPER
#include "mifsmapper.h"
#endif
#ifdef CONFIG_SCSC_QOS
#include "mifqos.h"
#endif
#include "mxlogger.h"
#include "srvman.h"
#include "servman_messages.h"
#include "mxmgmt_transport.h"
#include "mxlog_transport.h"

static ulong sm_completion_timeout_ms = 3000;
module_param(sm_completion_timeout_ms, ulong, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sm_completion_timeout_ms, "Timeout Service Manager start/stop (ms) - default 1000. 0 = infinite");

#define	SCSC_MIFRAM_INVALID_REF		-1
#define SCSC_MX_SERVICE_RECOVERY_TIMEOUT 20000 /* 20 seconds */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#define reinit_completion(completion) INIT_COMPLETION(*(completion))
#endif

#if defined(CONFIG_SCSC_PCIE_CHIP)
extern bool scsc_pcie_is_parked(void);
extern int scsc_pcie_claim(void);
extern void scsc_pcie_release(void);
extern bool scsc_pcie_in_deferred(void);
extern int scsc_pcie_complete(void);
extern bool scsc_pcie_link_state_error(void);
extern int exynos_pcie_rc_chk_link_status(int ch_num);
extern struct device *scsc_pcie_get_dev(void);

static u32 claim_bitmap;
module_param(claim_bitmap, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(claim_bitmap, "claim_bitmap");
static u32 err_bitmap;
module_param(err_bitmap, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(err_bitmap, "err_bitmap");
static int claim_cnt[32];
module_param_array(claim_cnt, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(claim_cnt, "claim_cnt each bit is count of claim_bit");
static const char *users[29] = {"DEFAULT", "RX_CTRL", "RB", "RX_DATA",
"TX_DATA", "TX_CTRL","MLME_SEND_FRAME", "MLME_REQ_CFM_IND", "MLME_REQ",
"UDI", "GDB", "RAMRP", "LOGGER_GERERATE", "LOGGER_COLLECT", "LOGGER_UNREGISTER",
"FAILURE_WORK_WLAN", "FAILURE_WORK_WPAN", "FAILURE_WORK", "FREEZE", "FORCE_PANIC",
"RECOVER", "SET_RECOVERY",
"LERNA", "SERVICE_START", "SERVICE_STOP", "SERVICE_CLOSE", "SERVICE_OPEN", "RUNTIME_PM", "LAST"};
#endif

struct scsc_service {
	struct list_head           list;
	struct scsc_mx             *mx;
	enum scsc_service_id       id;
	struct scsc_service_client *client;
	struct completion          sm_msg_start_completion;
	struct completion          sm_msg_stop_completion;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	enum scsc_subsystem        subsystem_type;
#endif
};

#ifdef CONFIG_WLBT_KUNIT
#include "./kunit/kunit_scsc_service.c"
#endif

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
enum scsc_subsystem scsc_service_id_subsystem_mapping(enum scsc_service_id id)
{
	switch (id) {
	case SCSC_SERVICE_ID_NULL: 	return SCSC_SUBSYSTEM_WLAN;
	case SCSC_SERVICE_ID_WLAN: 	return SCSC_SUBSYSTEM_WLAN;
	case SCSC_SERVICE_ID_BT:	return SCSC_SUBSYSTEM_WPAN;
	case SCSC_SERVICE_ID_ANT:	return SCSC_SUBSYSTEM_WPAN;
	case SCSC_SERVICE_ID_WLANDBG:	return SCSC_SUBSYSTEM_WLAN;
	case SCSC_SERVICE_ID_FLASH:	return SCSC_SUBSYSTEM_WLAN;
	case SCSC_SERVICE_ID_FM:        return SCSC_SUBSYSTEM_WPAN;
	default: return SCSC_SUBSYSTEM_INVALID;
	}
}
#endif

/* true if a service is part of a sub-system that is reported by system error */
#define SERVICE_IN_SUBSYSTEM(service, subsys) \
	(((subsys == SYSERR_SUBSYS_WLAN) && (service == SCSC_SERVICE_ID_WLAN)) || \
	((subsys == SYSERR_SUBSYS_BT) && ((service == SCSC_SERVICE_ID_BT) || (service == SCSC_SERVICE_ID_ANT))))

#if defined(CONFIG_SCSC_PCIE_CHIP)
struct device *scsc_service_get_pcie_dev(void)
{
	return scsc_pcie_get_dev();
}
EXPORT_SYMBOL(scsc_service_get_pcie_dev);
#endif

static void (*check_bt_status_cb)(bool bt_on);
struct mutex check_bt_status_mutex;
#define BT_SERVICE_ON true
#define BT_SERVICE_OFF false

int scsc_service_register_check_bt_status_cb(void(*status_cb)(bool bt_on))
{
	SCSC_TAG_INFO(MXMAN, "register check_bt_status_cb\n");

	mutex_lock(&check_bt_status_mutex);

	if(check_bt_status_cb) {
		SCSC_TAG_INFO(MXMAN, "already registered check_bt_status_cb\n");
		mutex_unlock(&check_bt_status_mutex);
		return -EINVAL;
	}

	check_bt_status_cb = status_cb;

	mutex_unlock(&check_bt_status_mutex);

	return 0;
}
EXPORT_SYMBOL(scsc_service_register_check_bt_status_cb);

int scsc_service_unregister_check_bt_status_cb(void)
{
	SCSC_TAG_INFO(MXMAN, "unregister check_bt_status_cb\n");

	mutex_lock(&check_bt_status_mutex);

	if(!check_bt_status_cb) {
		SCSC_TAG_INFO(MXMAN, "already unregistered check_bt_status_cb\n");
		mutex_unlock(&check_bt_status_mutex);
		return -EINVAL;
	}

	check_bt_status_cb = NULL;

	mutex_unlock(&check_bt_status_mutex);

	return 0;
}
EXPORT_SYMBOL(scsc_service_unregister_check_bt_status_cb);

void srvman_init(struct srvman *srvman, struct scsc_mx *mx)
{
	SCSC_TAG_INFO(MXMAN, "\n");
	srvman->mx = mx;
	INIT_LIST_HEAD(&srvman->service_list);
	mutex_init(&srvman->service_list_mutex);
	mutex_init(&srvman->api_access_mutex);
	mutex_init(&srvman->error_state_mutex);
#ifdef CONFIG_SCSC_COMMON_ANDROID
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
	wake_lock_init(&srvman->sm_wake_lock, WAKE_LOCK_SUSPEND, "srvman_wakelock");
#else
	wake_lock_init(NULL, &srvman->sm_wake_lock.ws, "srvman_wakelock");
#endif
#endif
	mutex_init(&check_bt_status_mutex);
	check_bt_status_cb = NULL;
	srvman_set_error(srvman, ALLOWED_START_STOP);
}

void srvman_deinit(struct srvman *srvman)
{
	struct scsc_service *service, *next;

	SCSC_TAG_INFO(MXMAN, "\n");
	srvman_set_error(srvman, NOT_ALLOWED_START_STOP);
	list_for_each_entry_safe(service, next, &srvman->service_list, list) {
		list_del(&service->list);
		kfree(service);
	}
	mutex_destroy(&srvman->api_access_mutex);
	mutex_destroy(&srvman->service_list_mutex);
	mutex_destroy(&srvman->error_state_mutex);
	mutex_destroy(&check_bt_status_mutex);
#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_lock_destroy(&srvman->sm_wake_lock);
#endif
}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
void srvman_set_error_subsystem_complete(struct srvman *srvman, enum scsc_subsystem sub, enum error_status s)
{
	struct scsc_service *service;

	SCSC_TAG_INFO(MXMAN, "Subsytem %d\n", sub);
	/* For the time being set srvman->error to also block
	 * another subsystem start/stop while on recovery.
	 * This can be easily addressed by having error variables/states */
	srvman_set_error(srvman, s);
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if (service->subsystem_type == sub) {
			complete(&service->sm_msg_start_completion);
			complete(&service->sm_msg_stop_completion);
		}
	}
	mutex_unlock(&srvman->service_list_mutex);
}
#endif

bool srvman_start_stop_not_allowed(struct srvman *srvman, struct mxman *mxman)
{
	bool is_not_allowed;
	bool panic_in_progress;

	mutex_lock(&srvman->error_state_mutex);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	panic_in_progress = mxman_if_get_panic_in_progress(mxman);
#else
	panic_in_progress = false;
#endif
	if (srvman->error == NOT_ALLOWED_START_STOP || panic_in_progress == true)
		is_not_allowed = true;
	else
		is_not_allowed = false;
	mutex_unlock(&srvman->error_state_mutex);

	return is_not_allowed;
}

bool srvman_start_not_allowed(struct srvman *srvman)
{
	bool is_not_allowed;

	mutex_lock(&srvman->error_state_mutex);
	if (srvman->error == NOT_ALLOWED_START_STOP || srvman->error == NOT_ALLOWED_START)
		is_not_allowed = true;
	else
		is_not_allowed = false;
	mutex_unlock(&srvman->error_state_mutex);

	return is_not_allowed;
}

bool srvman_in_error_safe(struct srvman *srvman)
{
	if (srvman->error == ALLOWED_START_STOP)
		return false;
	return true;
}

bool srvman_in_error(struct srvman *srvman)
{
	bool is_error;

	mutex_lock(&srvman->error_state_mutex);
	is_error = srvman_in_error_safe(srvman);
	mutex_unlock(&srvman->error_state_mutex);
	return is_error;
}

bool srvman_allow_close(struct srvman *srvman)
{
	bool is_error;

	mutex_lock(&srvman->error_state_mutex);
	if (srvman->error == NOT_ALLOWED_START_STOP)
		is_error = false;
	else
		is_error = true;
	mutex_unlock(&srvman->error_state_mutex);
	return is_error;
}

void srvman_set_error(struct srvman *srvman, enum error_status s)
{
	mutex_lock(&srvman->error_state_mutex);
	srvman->error = s;
	mutex_unlock(&srvman->error_state_mutex);
	SCSC_TAG_INFO(MXMAN, "error:%d\n", s);
}

void srvman_set_error_complete(struct srvman *srvman, enum error_status s)
{
	struct scsc_service *service;

	SCSC_TAG_INFO(MXMAN, "\n");

	srvman_set_error(srvman, s);
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		complete(&service->sm_msg_start_completion);
		complete(&service->sm_msg_stop_completion);
	}
	mutex_unlock(&srvman->service_list_mutex);
}

void srvman_forward_bt_fw_log(struct srvman *srvman, size_t length, u32 level, const void *message)
{
	/* BT FW log to btsnoop (fwsnoop) */
	struct scsc_service *service;

	/* Forward binary FW log to BT driver to insert it to btsnoop */
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if (service->client->fw_log)
			service->client->fw_log(service->client, length, level, message);
	}
	mutex_unlock(&srvman->service_list_mutex);
}

void srvman_wake_up_mxlog_thread_for_fwsnoop(void *data)
{
	struct scsc_service *service = (struct scsc_service *)data;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mxlog_thread_wake_up_for_fwsnoop(service->mx);
#else
	SCSC_TAG_INFO(MXMAN, "CONFIG_SCSC_INDEPENDENT_SUBSYSTEM is NOT Applied\n");
#endif
}
EXPORT_SYMBOL(srvman_wake_up_mxlog_thread_for_fwsnoop);

static int wait_for_sm_msg_start_cfm(struct scsc_service *service)
{
	int r;

	if (0 == sm_completion_timeout_ms) {
		/* Zero implies infinite wait, for development use only.
		 * r = -ERESTARTSYS if interrupted (e.g. Ctrl-C), 0 if completed
		 */
		r = wait_for_completion_interruptible(&service->sm_msg_start_completion);
		if (r == -ERESTARTSYS) {
			/* Paranoid sink of any pending event skipped by the interrupted wait */
			r = wait_for_completion_timeout(&service->sm_msg_start_completion, HZ / 2);
			if (r == 0) {
				SCSC_TAG_ERR(MXMAN, "timed out\n");
				return -ETIMEDOUT;
			}
		}
		return r;
	}
	r = wait_for_completion_timeout(&service->sm_msg_start_completion, msecs_to_jiffies(sm_completion_timeout_ms));
	if (r == 0) {
		SCSC_TAG_ERR(MXMAN, "timeout\n");
		return -ETIMEDOUT;
	}
	return 0;
}

static int wait_for_sm_msg_stop_cfm(struct scsc_service *service)
{
	int r;

	if (0 == sm_completion_timeout_ms) {
		/* Zero implies infinite wait, for development use only.
		 * r = -ERESTARTSYS if interrupted (e.g. Ctrl-C), 0 if completed
		 */
		r = wait_for_completion_interruptible(&service->sm_msg_stop_completion);
		if (r == -ERESTARTSYS) {
			/* Paranoid sink of any pending event skipped by the interrupted wait */
			r = wait_for_completion_timeout(&service->sm_msg_stop_completion, HZ / 2);
			if (r == 0) {
				SCSC_TAG_ERR(MXMAN, "timed out\n");
				return -ETIMEDOUT;
			}
		}
		return r;
	}
	r = wait_for_completion_timeout(&service->sm_msg_stop_completion, msecs_to_jiffies(sm_completion_timeout_ms));
	if (r == 0) {
		SCSC_TAG_ERR(MXMAN, "timeout\n");
		return -ETIMEDOUT;
	}
	return 0;
}

static int send_sm_msg_start_blocking(struct scsc_service *service, scsc_mifram_ref ref)
{
	struct scsc_mx          *mx = service->mx;
	struct mxmgmt_transport *mxmgmt_transport;
	int                     r;
	struct sm_msg_packet    message = { .service_id = service->id,
					    .msg = SM_MSG_START_REQ,
					    .optional_data = ref };

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
		mxmgmt_transport = scsc_mx_get_mxmgmt_transport(mx);
	else
		mxmgmt_transport = scsc_mx_get_mxmgmt_transport_wpan(mx);
#else
	mxmgmt_transport = scsc_mx_get_mxmgmt_transport(mx);
#endif
	reinit_completion(&service->sm_msg_start_completion);

	/* Send to FW in MM stream */
	mxmgmt_transport_send(mxmgmt_transport, MMTRANS_CHAN_ID_SERVICE_MANAGEMENT, &message, sizeof(message));
	r = wait_for_sm_msg_start_cfm(service);
	if (r) {
		SCSC_TAG_ERR(MXMAN, "wait_for_sm_msg_start_cfm() failed: r=%d\n", r);
		mxmgmt_print_sent_data_dump();

		/* Report the error in order to get a moredump. Avoid auto-recovering this type of failure */
		if (mxman_recovery_disabled())
			scsc_mx_service_service_failed(service, "SM_MSG_START_CFM timeout");
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
		else
			mxman_if_notify_result_to_start_cmd(scsc_mx_get_mxman(mx), false);
#endif
	}
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
	else
		mxman_if_notify_result_to_start_cmd(scsc_mx_get_mxman(mx), true);
#endif
	return r;
}

static int send_sm_msg_stop_blocking(struct scsc_service *service)
{
	struct scsc_mx          *mx = service->mx;
	struct mxman            *mxman = scsc_mx_get_mxman(mx);
	struct mxmgmt_transport *mxmgmt_transport;
	int                     r;
	struct sm_msg_packet	message = { .service_id = service->id,
					    .msg = SM_MSG_STOP_REQ,
					    .optional_data = 0 };
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
		mxmgmt_transport = scsc_mx_get_mxmgmt_transport(mx);
	else
		mxmgmt_transport = scsc_mx_get_mxmgmt_transport_wpan(mx);
#else
	mxmgmt_transport = scsc_mx_get_mxmgmt_transport(mx);
#endif

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (mxman_if_subsys_in_failed_state(mxman, service->subsystem_type))
		return 0;
#else
	if (mxman->mxman_state == MXMAN_STATE_FAILED)
		return 0;
#endif

	reinit_completion(&service->sm_msg_stop_completion);

	/* Send to FW in MM stream */
	mxmgmt_transport_send(mxmgmt_transport, MMTRANS_CHAN_ID_SERVICE_MANAGEMENT, &message, sizeof(message));
	r = wait_for_sm_msg_stop_cfm(service);
	if (r) {
		SCSC_TAG_ERR(MXMAN, "wait_for_sm_msg_stop_cfm() for service=%p service->id=%d failed: r=%d\n", service, service->id, r);
		mxmgmt_print_sent_data_dump();
	}
	return r;
}

/*
 * Receive handler for messages from the FW along the maxwell management transport
 */
static void srv_message_handler(const void *message, void *data)
{
	struct srvman       *srvman = (struct srvman *)data;
	struct scsc_service *service;
	const struct sm_msg_packet *msg = message;
	bool                found = false;

	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if (service->id == msg->service_id) {
			found = true;
			break;
		}
	}
	if (!found) {
		SCSC_TAG_ERR(MXMAN, "No service for msg->service_id=%d\n", msg->service_id);
		mutex_unlock(&srvman->service_list_mutex);
		return;
	}
	/* Forward the message to the applicable service to deal with */
	switch (msg->msg) {
	case SM_MSG_START_CFM:
		SCSC_TAG_INFO(MXMAN, "Received SM_MSG_START_CFM message service=%p with service_id=%d from the firmware\n",
			      service, msg->service_id);
		complete(&service->sm_msg_start_completion);
		break;
	case SM_MSG_STOP_CFM:
		SCSC_TAG_INFO(MXMAN, "Received SM_MSG_STOP_CFM message for service=%p with service_id=%d from the firmware\n",
			      service, msg->service_id);
		complete(&service->sm_msg_stop_completion);
		break;
	default:
		/* HERE: Unknown message, raise fault */
		SCSC_TAG_WARNING(MXMAN, "Received unknown message for service=%p with service_id=%d from the firmware: msg->msg=%d\n",
				 service, msg->msg, msg->service_id);
		break;
	}
	mutex_unlock(&srvman->service_list_mutex);
}

int scsc_mx_service_start(struct scsc_service *service, scsc_mifram_ref ref)
{
	struct scsc_mx *mx = service->mx;
	struct srvman *srvman = scsc_mx_get_srvman(mx);
	struct mxman *mxman = scsc_mx_get_mxman(service->mx);
	int            r;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
	struct  __kernel_old_timeval tval = {};
#else
	struct timeval tval = {};
#endif

	SCSC_TAG_INFO(MXMAN, "service id: %d\n", service->id);
#ifdef CONFIG_SCSC_CHV_SUPPORT
	if (chv_run)
		return 0;
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
	if (scsc_mx_service_claim(SERVICE_START)) {
		SCSC_TAG_INFO(MXMAN, "Error claiming link\n");
		return -EFAULT;
	}
#endif
	mutex_lock(&srvman->api_access_mutex);
#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_lock(&srvman->sm_wake_lock);
#endif
	if (srvman_start_not_allowed(srvman)) {
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
		tval = ns_to_kernel_old_timeval(mxman_if_get_last_panic_time(mxman));
#else
		tval = ns_to_timeval(mxman_if_get_last_panic_time(mxman));
#endif
		SCSC_TAG_ERR(MXMAN, "error: refused due to previous f/w failure scsc_panic_code=0x%x happened at [%6lu.%06ld]\n",
				mxman_if_get_panic_code(mxman), tval.tv_sec, tval.tv_usec);

		/* Print the last panic record to help track ancient failures */
		mxman_if_show_last_panic(mxman);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
		tval = ns_to_kernel_old_timeval(mxman->last_panic_time);
#else
		tval = ns_to_timeval(mxman->last_panic_time);
#endif
		SCSC_TAG_ERR(MXMAN, "error: refused due to previous f/w failure scsc_panic_code=0x%x happened at [%6lu.%06ld]\n",
				mxman->scsc_panic_code, tval.tv_sec, tval.tv_usec);

		/* Print the last panic record to help track ancient failures */
		mxman_show_last_panic(mxman);
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
		scsc_mx_service_release(SERVICE_START);
#endif
#ifdef CONFIG_SCSC_COMMON_ANDROID
		wake_unlock(&srvman->sm_wake_lock);
#endif
		mutex_unlock(&srvman->api_access_mutex);
		return -EILSEQ;
	}

	r = send_sm_msg_start_blocking(service, ref);
	if (r) {
		SCSC_TAG_ERR(MXMAN, "send_sm_msg_start_blocking() failed: r=%d\n", r);
#if defined(CONFIG_SCSC_PCIE_CHIP)
		scsc_mx_service_release(SERVICE_START);
#endif
#ifdef CONFIG_SCSC_COMMON_ANDROID
		wake_unlock(&srvman->sm_wake_lock);
#endif
		mutex_unlock(&srvman->api_access_mutex);
		return r;
	}

	mutex_lock(&check_bt_status_mutex);
	if (service->id == SCSC_SERVICE_ID_BT && check_bt_status_cb)
		check_bt_status_cb(BT_SERVICE_ON);
	mutex_unlock(&check_bt_status_mutex);

#if defined(CONFIG_SCSC_PCIE_CHIP)
	scsc_mx_service_release(SERVICE_START);
#endif
#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_unlock(&srvman->sm_wake_lock);
#endif
	mutex_unlock(&srvman->api_access_mutex);
	return 0;
}
EXPORT_SYMBOL(scsc_mx_service_start);

void scsc_mx_service_control_suspend_gpio(struct scsc_mx *mx, u8 value)
{
	struct mxman *mxman = scsc_mx_get_mxman(mx);
	mxman_if_control_suspend_gpio(mxman, value);
}
EXPORT_SYMBOL(scsc_mx_service_control_suspend_gpio);

#if defined(CONFIG_SCSC_PCIE_CHIP)
static bool in_wait;
static DEFINE_SPINLOCK(pcie_users_lock);
static DEFINE_MUTEX(claim);
#define ADD_BITMAP(x, y)( x = x | (1 << (y)))
#define DEL_BITMAP(x, y)( x = x & ~(1 << (y)))

static void pcie_users_claim(enum CLAIM_TYPE claim_type)
{
	ADD_BITMAP(claim_bitmap,claim_type);
	claim_cnt[claim_type] += 1;
}

static void pcie_users_release(enum CLAIM_TYPE claim_type)
{
	if(claim_cnt[claim_type]){
		claim_cnt[claim_type] -= 1;
		if(!claim_cnt[claim_type]){
			DEL_BITMAP(claim_bitmap, claim_type);
		}
	}else{
		SCSC_TAG_ERR(MXMAN, "Wrong release PCIE %s\n", users[claim_type]);
		ADD_BITMAP(err_bitmap, claim_type);
	}
}

void pcie_users_print(void)
{
	int i =0;
	enum CLAIM_TYPE claim_type = LAST_CLAIM_TYPE;
	SCSC_TAG_ERR(MXMAN, "err_bitmap 0x%x\n", err_bitmap);
	for(i=0;i < claim_type;i++){
		SCSC_TAG_ERR(MXMAN, "claim cnt %s %d\n",users[i], claim_cnt[i]);
	}
}
EXPORT_SYMBOL(pcie_users_print);

/*
 * Request host interface on
 *
 * If claim_complete is NULL, request returns 0 when link is ready
 * If claim_complete is provided, it is called when link is ready, and function
 * returns -EAGAIN.
 */
/* Can be called from IRQ context */
int scsc_mx_service_claim_deferred(struct scsc_service *service, int (*claim_complete)(void *service, void *data), void *dev, enum CLAIM_TYPE claim_type)
{
	int ret = 0;
	unsigned long flags;
	struct scsc_mif_abs *mif_abs = scsc_mx_get_mif_abs(service->mx);

	spin_lock_irqsave(&pcie_users_lock, flags);
	if (!claim_bitmap || scsc_pcie_in_deferred()| in_wait) {
		/* PCIE is off */
		SCSC_TAG_DEBUG(MXMAN, "register deferred call\n");
		/* Kick the wq to turn on the PCIE link */
		if (mif_abs->hostif_wakeup)
			ret = mif_abs->hostif_wakeup(mif_abs, claim_complete, service, dev);
		SCSC_TAG_DEBUG(MXMAN, "register deferred call return %d\n", ret);
	}
	pcie_users_claim(claim_type);
	spin_unlock_irqrestore(&pcie_users_lock, flags);

	return ret;
}
EXPORT_SYMBOL(scsc_mx_service_claim_deferred);

/* Can't be called from IRQ/BH context!!*/
int scsc_mx_service_claim(enum CLAIM_TYPE claim_type)
{
	int ret = 0;
	unsigned long flags;

	if(scsc_pcie_is_parked()){
		SCSC_TAG_WARNING(MXMAN, "kthread is parked.\n");
		return -EAGAIN;
	}

	if (scsc_pcie_link_state_error()) {
		SCSC_TAG_WARNING(MXMAN, "PCIE link is in ERROR state.\n");
		return (claim_type == MXMAN_FAILURE_WORK || claim_type == SERVICE_CLOSE) ? 0 : -ENOLINK;
	}

	/* Mutex to serialize consecutive claims when the first claim is wating for complete */
	mutex_lock(&claim);
	spin_lock_irqsave(&pcie_users_lock, flags);
	if (!claim_bitmap || scsc_pcie_in_deferred()) {
		pcie_users_claim(claim_type);
		/* This will directly enable PCIe. Blocking call */
		SCSC_TAG_DEBUG(MXMAN, "claim PCIE\n");
		in_wait = true;
		scsc_pcie_claim(); /* blocking call to turn on PCIE */
		spin_unlock_irqrestore(&pcie_users_lock, flags);
		ret = scsc_pcie_complete();
		in_wait = false;
		if (ret) {
			spin_lock_irqsave(&pcie_users_lock, flags);
			pcie_users_release(claim_type);
			spin_unlock_irqrestore(&pcie_users_lock, flags);
		}
		mutex_unlock(&claim);
		return ret;
	}else{
		pcie_users_claim(claim_type);
		spin_unlock_irqrestore(&pcie_users_lock, flags);
	}
	mutex_unlock(&claim);

	return ret;
}
EXPORT_SYMBOL(scsc_mx_service_claim);

int scsc_mx_service_release(enum CLAIM_TYPE claim_type)
{
	unsigned long flags;

	if (scsc_pcie_link_state_error()) {
		SCSC_TAG_WARNING(MXMAN, "PCIE link is in ERROR state.\n");
		return -ENOLINK;
	}

	spin_lock_irqsave(&pcie_users_lock, flags);
	if (!claim_bitmap) {
		SCSC_TAG_ERR(MXMAN, "Wrong release PCIE %s\n", users[claim_type]);
		pcie_users_print();
		ADD_BITMAP(err_bitmap, claim_type);
		spin_unlock_irqrestore(&pcie_users_lock, flags);
		return -EIO;
	}
	pcie_users_release(claim_type);
	if (!claim_bitmap) {
		/* This will directly enable PCIe. Blocking call */
		SCSC_TAG_DEBUG(MXMAN, "release PCIE\n");
		scsc_pcie_release();
		spin_unlock_irqrestore(&pcie_users_lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&pcie_users_lock, flags);

       return 0;
}
EXPORT_SYMBOL(scsc_mx_service_release);

bool scsc_mx_service_check_pcie_link_error(void)
{
	return scsc_pcie_link_state_error();
}
EXPORT_SYMBOL(scsc_mx_service_check_pcie_link_error);
#endif

int scsc_mx_list_services(struct scsc_mx *mx, char *buf, const size_t bufsz)
{
	int    pos = 0;
	struct scsc_service *service, *next;
	struct   srvman  *srvman_p = scsc_mx_get_srvman(mx);

	list_for_each_entry_safe(service, next, &srvman_p->service_list, list) {
		switch (service->id) {
		case SCSC_SERVICE_ID_NULL:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "null");
			break;
		case SCSC_SERVICE_ID_WLAN:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "wlan");
			break;
		case SCSC_SERVICE_ID_BT:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "bt");
			break;
		case SCSC_SERVICE_ID_ANT:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "ant");
			break;
		case SCSC_SERVICE_ID_WLANDBG:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "wlandbg");
			break;
		case SCSC_SERVICE_ID_ECHO:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "echo");
			break;
		case SCSC_SERVICE_ID_DBG_SAMPLER:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "dbg sampler");
			break;
		case SCSC_SERVICE_ID_CLK20MHZ:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "clk20mhz");
			break;
		case SCSC_SERVICE_ID_FM:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "fm");
			break;
		case SCSC_SERVICE_ID_FLASH:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "flash");
			break;
		case SCSC_SERVICE_ID_INVALID:
		default:
			pos += scnprintf(buf + pos, bufsz - pos, "%s\n", "invalid");
			break;
		}
	}
	return pos;
}
EXPORT_SYMBOL(scsc_mx_list_services);

int scsc_mx_service_stop(struct scsc_service *service)
{
	struct scsc_mx *mx = service->mx;
	struct srvman *srvman = scsc_mx_get_srvman(mx);
	struct mxman *mxman = scsc_mx_get_mxman(service->mx);
	int r;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
	struct  __kernel_old_timeval tval = {};
#else
	struct timeval tval = {};
#endif

	SCSC_TAG_INFO(MXMAN, "service id: %d\n", service->id);
#ifdef CONFIG_SCSC_CHV_SUPPORT
	if (chv_run)
		return 0;
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
	if (scsc_mx_service_claim(SERVICE_STOP)) {
		SCSC_TAG_INFO(MXMAN, "Error claiming link\n");
		return -EFAULT;
	}
#endif
	mutex_lock(&srvman->api_access_mutex);
#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_lock(&srvman->sm_wake_lock);
#endif
	if (srvman_start_stop_not_allowed(srvman, mxman)) {
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
		tval = ns_to_kernel_old_timeval(mxman_if_get_last_panic_time(mxman));
#else
		tval = ns_to_timeval(mxman_if_get_last_panic_time(mxman));
#endif
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
		if (mxman_if_check_recovery_state_none())
#endif
			SCSC_TAG_ERR(MXMAN, "error: refused due to previous f/w failure scsc_panic_code=0x%x happened at [%6lu.%06ld]\n",
				mxman_if_get_panic_code(mxman), tval.tv_sec, tval.tv_usec);

		/* Print the last panic record to help track ancient failures */
		mxman_if_show_last_panic(mxman);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
		tval = ns_to_kernel_old_timeval(mxman->last_panic_time);
#else
		tval = ns_to_timeval(mxman->last_panic_time);
#endif
		SCSC_TAG_ERR(MXMAN, "error: refused due to previous f/w failure scsc_panic_code=0x%x happened at [%6lu.%06ld]\n",
				mxman->scsc_panic_code, tval.tv_sec, tval.tv_usec);

		/* Print the last panic record to help track ancient failures */
		mxman_show_last_panic(mxman);
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
		scsc_mx_service_release(SERVICE_STOP);
#endif

#ifdef CONFIG_SCSC_COMMON_ANDROID
		wake_unlock(&srvman->sm_wake_lock);
#endif
		mutex_unlock(&srvman->api_access_mutex);

		/* Return a special status to allow caller recovery logic to know
		 * that there will never be a recovery
		 */
		if (mxman_recovery_disabled()) {
			SCSC_TAG_ERR(MXMAN, "recovery disabled, return -EPERM (%d)\n", -EPERM);
			return -EPERM; /* failed due to prior failure, recovery disabled */
		} else {
			return -EILSEQ; /* operation rejected due to prior failure */
		}
	}
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
	if (mxman_if_check_recovery_state_none()){
#endif
		r = send_sm_msg_stop_blocking(service);
		if (r) {
			SCSC_TAG_ERR(MXMAN, "send_sm_msg_stop_blocking() failed: r=%d\n", r);
#if defined(CONFIG_SCSC_PCIE_CHIP)
			scsc_mx_service_release(SERVICE_STOP);
#endif
#ifdef CONFIG_SCSC_COMMON_ANDROID
			wake_unlock(&srvman->sm_wake_lock);
#endif
			mutex_unlock(&srvman->api_access_mutex);
			return -EIO; /* operation failed */
		}
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
	}
#endif
	mutex_lock(&check_bt_status_mutex);
	if (service->id == SCSC_SERVICE_ID_BT && check_bt_status_cb)
		check_bt_status_cb(BT_SERVICE_OFF);
	mutex_unlock(&check_bt_status_mutex);
#if defined(CONFIG_SCSC_PCIE_CHIP)
	scsc_mx_service_release(SERVICE_STOP);
#endif

#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_unlock(&srvman->sm_wake_lock);
#endif
	mutex_unlock(&srvman->api_access_mutex);
	return 0;
}
EXPORT_SYMBOL(scsc_mx_service_stop);


/* Returns 0 if Suspend succeeded, otherwise return error */
int srvman_suspend_services(struct srvman *srvman)
{
	int ret = 0;
	struct scsc_service *service;

	SCSC_TAG_INFO(MXMAN, "\n");
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if (service->client->suspend) {
			ret = service->client->suspend(service->client);
			/* If any service returns error message and call resume callbacks */
			if (ret) {
				list_for_each_entry(service, &srvman->service_list, list) {
					if (service->client->resume)
						service->client->resume(service->client);
				}
				SCSC_TAG_INFO(MXMAN, "Service client suspend failure ret: %d\n", ret);
				mutex_unlock(&srvman->service_list_mutex);
				return ret;
			}
		}
	}

	mutex_unlock(&srvman->service_list_mutex);
	SCSC_TAG_INFO(MXMAN, "OK\n");
	return 0;
}

/* Returns always 0. Extend API and return value if required */
int srvman_resume_services(struct srvman *srvman)
{
	struct scsc_service *service;

	SCSC_TAG_INFO(MXMAN, "\n");
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if (service->client->resume)
			service->client->resume(service->client);
	}

	mutex_unlock(&srvman->service_list_mutex);
	SCSC_TAG_INFO(MXMAN, "OK\n");

	return 0;
}

void srvman_freeze_services(struct srvman *srvman, struct mx_syserr_decode *syserr)
{
	struct scsc_service *service;
	struct mxman        *mxman = scsc_mx_get_mxman(srvman->mx);

	SCSC_TAG_INFO(MXMAN, "\n");
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	srvman->notify = false;
#else
	mxman->notify = false;
#endif
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
	if (service->client->stop_on_failure) {
		service->client->stop_on_failure(service->client);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
		srvman->notify = true;
#else
		mxman->notify = true;
#endif
	} else if ((service->client->stop_on_failure_v2) &&
		  (service->client->stop_on_failure_v2(service->client, syserr)))
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
		srvman->notify = true;
#else
		mxman->notify = true;
#endif
	}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mxman_if_reinit_completion(mxman);
#else
	reinit_completion(&mxman->recovery_completion);
#endif
	mutex_unlock(&srvman->service_list_mutex);
	SCSC_TAG_INFO(MXMAN, "OK\n");
}

void srvman_freeze_sub_system(struct srvman *srvman, struct mx_syserr_decode *syserr)
{
	struct scsc_service *service;
	struct mxman        *mxman = scsc_mx_get_mxman(srvman->mx);

	SCSC_TAG_INFO(MXMAN, "\n");
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	srvman->notify = false;
#else
	mxman->notify = false;
#endif
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if ((SERVICE_IN_SUBSYSTEM(service->id, syserr->subsys) && (service->client->stop_on_failure_v2)))
			if (service->client->stop_on_failure_v2(service->client, syserr))
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
				srvman->notify = true;
#else
				mxman->notify = true;
#endif
	}
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mxman_if_reinit_completion(mxman);
#else
	reinit_completion(&mxman->recovery_completion);
#endif
	mutex_unlock(&srvman->service_list_mutex);
	SCSC_TAG_INFO(MXMAN, "OK\n");
}

void srvman_unfreeze_services(struct srvman *srvman, struct mx_syserr_decode *syserr)
{
	struct scsc_service *service;
#if !defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	struct mxman        *mxman = scsc_mx_get_mxman(srvman->mx);
#endif

	SCSC_TAG_INFO(MXMAN, "\n");
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if (service->client->failure_reset)
			service->client->failure_reset(service->client, syserr->subcode);
		else if (service->client->failure_reset_v2)
			service->client->failure_reset_v2(service->client, syserr->level,
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
							  srvman->notify ? syserr->subcode : MX_NULL_SYSERR);
#else
							  mxman->notify ? syserr->subcode : MX_NULL_SYSERR);
#endif
	}
	mutex_unlock(&srvman->service_list_mutex);
	SCSC_TAG_INFO(MXMAN, "OK\n");
}

void srvman_unfreeze_sub_system(struct srvman *srvman, struct mx_syserr_decode *syserr)
{
	struct scsc_service *service;
#if !defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	struct mxman        *mxman = scsc_mx_get_mxman(srvman->mx);
#endif

	SCSC_TAG_INFO(MXMAN, "\n");
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if ((SERVICE_IN_SUBSYSTEM(service->id, syserr->subsys) && (service->client->failure_reset_v2)))
			service->client->failure_reset_v2(service->client, syserr->level,
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
							  srvman->notify ? syserr->subcode : MX_NULL_SYSERR);
#else
							  mxman->notify ? syserr->subcode : MX_NULL_SYSERR);
#endif
	}
	mutex_unlock(&srvman->service_list_mutex);
	SCSC_TAG_INFO(MXMAN, "OK\n");
}

u8 srvman_notify_services(struct srvman *srvman, struct mx_syserr_decode *syserr)
{
	struct scsc_service *service;
	u8 final_level = syserr->level;

	SCSC_TAG_INFO(MXMAN, "\n");
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		u8 level = service->client->failure_notification(service->client, syserr);

		if (level > final_level)
			final_level = level;
	}
	mutex_unlock(&srvman->service_list_mutex);

	if (final_level != syserr->level)
		SCSC_TAG_INFO(MXMAN, "System error level %d raised to level %d\n", syserr->level, final_level);

	SCSC_TAG_INFO(MXMAN, "OK\n");

	return final_level;
}

u8 srvman_notify_sub_system(struct srvman *srvman, struct mx_syserr_decode *syserr)
{
	struct scsc_service *service;
	u8 initial_level = syserr->level;
	u8 final_level = syserr->level;
	bool wlan_active = false;
	bool bt_active = false;
	bool affected_service_found = false;

	SCSC_TAG_INFO(MXMAN, "\n");
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if (SERVICE_IN_SUBSYSTEM(service->id, SYSERR_SUBSYS_WLAN))
			wlan_active = true;
		else if (SERVICE_IN_SUBSYSTEM(service->id, SYSERR_SUBSYS_BT))
			bt_active = true;
		if ((SERVICE_IN_SUBSYSTEM(service->id, syserr->subsys) && (service->client->failure_notification))) {
			u8 level = service->client->failure_notification(service->client, syserr);

			affected_service_found = true;
			if (level > final_level)
				final_level = level;
		}
	}
	mutex_unlock(&srvman->service_list_mutex);

	if (final_level >= MX_SYSERR_LEVEL_7)
		SCSC_TAG_INFO(MXMAN, "System error level %d raised to full reset level %d\n", initial_level, final_level);
	else if ((!(wlan_active && bt_active)) && (final_level >= MX_SYSERR_LEVEL_5)) {
		final_level = MX_SYSERR_LEVEL_6; /* Still a sub-system reset even though we will do a full restart */
		SCSC_TAG_INFO(MXMAN, "System error %d now level %d with 1 service active\n", initial_level, final_level);
	}

	SCSC_TAG_INFO(MXMAN, "OK\n");

	/* Handle race condition with affected service being closed by demoting severity to stop any recovery
	 * should not be possible, but best be careful anyway
	 */
	if ((!affected_service_found) && (final_level >= MX_SYSERR_LEVEL_5)) {
		SCSC_TAG_INFO(MXMAN, "System error %d demoted to 4 as no services affected\n", final_level);
		final_level = MX_SYSERR_LEVEL_4;
	}

	return final_level;
}

bool srvman_check_silent_recovery(struct srvman *srvman, struct mx_syserr_decode *syserr)
{
	struct scsc_service *service;
	u8 adjusted_level = syserr->level;

	SCSC_TAG_INFO(MXMAN, "Error level %d, subsys %d\n", syserr->level, syserr->subsys);
	mutex_lock(&srvman->service_list_mutex);
	list_for_each_entry(service, &srvman->service_list, list) {
		if (SERVICE_IN_SUBSYSTEM(service->id, syserr->subsys) && (service->client->check_reset_level)) {
			adjusted_level = service->client->check_reset_level(service->client, syserr->level);
			if (adjusted_level != syserr->level) {
				SCSC_TAG_INFO(MXMAN, "Error level %d -> %d\n", syserr->level, adjusted_level);
				mutex_unlock(&srvman->service_list_mutex);
				return true;
			}
		}
	}
	mutex_unlock(&srvman->service_list_mutex);
	return false;
}

/** Signal a failure detected by the Client. This will trigger the systemwide
 * failure handling procedure: _All_ Clients will be called back via
 * their stop_on_failure() handler as a side-effect.
 */
void scsc_mx_service_service_failed(struct scsc_service *service, const char *reason)
{
	struct scsc_mx *mx = service->mx;
	struct srvman  *srvman = scsc_mx_get_srvman(mx);
	u16 host_panic_code;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM) /* TODO */
	host_panic_code = (SCSC_PANIC_CODE_HOST << 15) | (service->id << 4);
#else
	host_panic_code = (SCSC_PANIC_CODE_HOST << 15) | (service->id << SCSC_SYSERR_HOST_SERVICE_SHIFT);
#endif

	srvman_set_error(srvman, NOT_ALLOWED_START_STOP);
	switch (service->id) {
	case SCSC_SERVICE_ID_WLAN:
		SCSC_TAG_INFO(MXMAN, "WLAN: %s\n", ((reason != NULL) ? reason : ""));
		break;
	case SCSC_SERVICE_ID_BT:
		SCSC_TAG_INFO(MXMAN, "BT: %s\n", ((reason != NULL) ? reason : ""));
		break;
	default:
		SCSC_TAG_INFO(MXMAN, "service id %d failed\n", service->id);
		break;

	}

	SCSC_TAG_INFO(MXMAN, "Reporting host hang code 0x%02x\n", host_panic_code);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mxman_if_fail(scsc_mx_get_mxman(mx), host_panic_code, reason, service->subsystem_type);
#else
	mxman_fail(scsc_mx_get_mxman(mx), host_panic_code, reason);
#endif
}
EXPORT_SYMBOL(scsc_mx_service_service_failed);


int scsc_mx_service_close(struct scsc_service *service)
{
	struct mxman   *mxman = scsc_mx_get_mxman(service->mx);
	struct scsc_mx *mx = service->mx;
	struct srvman  *srvman = scsc_mx_get_srvman(mx);
	bool           empty;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
	struct  __kernel_old_timeval tval = {};
#else
	struct timeval tval = {};
#endif
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	enum scsc_subsystem sub = service->subsystem_type;
#endif

	SCSC_TAG_INFO(MXMAN, "service id: %d\n", service->id);
#if defined(CONFIG_SCSC_PCIE_CHIP)
	if (scsc_mx_service_claim(SERVICE_CLOSE)) {
		SCSC_TAG_INFO(MXMAN, "Error claiming link\n");
		return -EFAULT;
	}
#endif

	mutex_lock(&srvman->api_access_mutex);
#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_lock(&srvman->sm_wake_lock);
#endif
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
	if (mxman_if_check_start_failure_during_indep_recovery(mxman) == sub) {
		SCSC_TAG_WARNING(MXMAN, "Common driver has to temporariliy keep a subsystem which failed to re-open.\n");
	} else if (srvman_start_stop_not_allowed(srvman, mxman)) {
#else
	if (srvman_start_stop_not_allowed(srvman, mxman)) {
#endif
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
		tval = ns_to_kernel_old_timeval(mxman_if_get_last_panic_time(mxman));
#else
		tval = ns_to_timeval(mxman_if_get_last_panic_time(mxman));
#endif
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
		if (mxman_if_check_recovery_state_none())
#endif
			SCSC_TAG_ERR(MXMAN, "error: refused due to previous f/w failure scsc_panic_code=0x%x happened at [%6lu.%06ld]\n",
				mxman_if_get_panic_code(mxman), tval.tv_sec, tval.tv_usec);

		/* Print the last panic record to help track ancient failures */
		mxman_if_show_last_panic(mxman);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
		tval = ns_to_kernel_old_timeval(mxman->last_panic_time);
#else
		tval = ns_to_timeval(mxman->last_panic_time);
#endif
		SCSC_TAG_ERR(MXMAN, "error: refused due to previous f/w failure scsc_panic_code=0x%x happened at [%6lu.%06ld]\n",
				mxman->scsc_panic_code, tval.tv_sec, tval.tv_usec);

		/* Print the last panic record to help track ancient failures */
		mxman_show_last_panic(mxman);
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
		scsc_mx_service_release(SERVICE_CLOSE);
#endif
		mutex_unlock(&srvman->api_access_mutex);
#ifdef CONFIG_SCSC_COMMON_ANDROID
		wake_unlock(&srvman->sm_wake_lock);
#endif

		/* Return a special status when recovery is disabled, to allow
		 * calling recovery logic to be aware that recovery is disabled,
		 * hence not wait for recovery events.
		 */
		if (mxman_recovery_disabled()) {
			SCSC_TAG_ERR(MXMAN, "recovery disabled, return -EPERM (%d)\n", -EPERM);
			return -EPERM; /* rejected due to prior failure, recovery disabled */
		} else {
			return -EIO;
		}
	}

	/* remove the service from the list and deallocate the service memory */
	mutex_lock(&srvman->service_list_mutex);
	list_del(&service->list);
	empty = list_empty(&srvman->service_list);
	mutex_unlock(&srvman->service_list_mutex);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (empty) {
		/* Unregister channgel handlers */
		if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
			mxmgmt_transport_register_channel_handler(scsc_mx_get_mxmgmt_transport(mx), MMTRANS_CHAN_ID_SERVICE_MANAGEMENT,
								  NULL, NULL);

		else if (service->subsystem_type == SCSC_SUBSYSTEM_WPAN)
			mxmgmt_transport_register_channel_handler(scsc_mx_get_mxmgmt_transport_wpan(mx), MMTRANS_CHAN_ID_SERVICE_MANAGEMENT,
								  NULL, NULL);

		/* Clear any system error information */
		mxman_if_set_syserr_recovery_in_progress(mxman, false);
		mxman_if_set_last_syserr_recovery_time(mxman, sub, 0);
	} else if (mxman_if_get_syserr_recovery_in_progress(mxman)) {
		/* If we have syserr_recovery_in_progress and all the services we have asked to close are now closed,
		 * we can clear it now - don't wait for open as it may not come - do it now!
		 */
		struct scsc_service *serv;
		bool all_cleared = true;

		mutex_lock(&srvman->service_list_mutex);
		list_for_each_entry(serv, &srvman->service_list, list) {
			if (SERVICE_IN_SUBSYSTEM(serv->id, mxman_if_get_last_syserr_subsys(mxman)))
				all_cleared = false;
		}
		mutex_unlock(&srvman->service_list_mutex);

		if (all_cleared)
			mxman_if_set_syserr_recovery_in_progress(mxman, false);
	}
#else
	if (empty) {
		/* unregister channel handler */
		mxmgmt_transport_register_channel_handler(scsc_mx_get_mxmgmt_transport(mx), MMTRANS_CHAN_ID_SERVICE_MANAGEMENT,
							  NULL, NULL);
		/* Clear any system error information */
		mxman->syserr_recovery_in_progress = false;
		mxman->last_syserr_recovery_time = 0;
	} else if (mxman->syserr_recovery_in_progress) {
		/* If we have syserr_recovery_in_progress and all the services we have asked to close are now closed,
		 * we can clear it now - don't wait for open as it may not come - do it now!
		 */
		struct scsc_service *serv;
		bool all_cleared = true;

		mutex_lock(&srvman->service_list_mutex);
		list_for_each_entry(serv, &srvman->service_list, list) {
			if (SERVICE_IN_SUBSYSTEM(serv->id, mxman->last_syserr.subsys))
				all_cleared = false;
		}
		mutex_unlock(&srvman->service_list_mutex);

		if (all_cleared)
			mxman->syserr_recovery_in_progress = false;
	}
#endif

	kfree(service);
#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
	if (mxman_if_check_start_failure_during_indep_recovery(mxman) != SCSC_SUBSYSTEM_INVALID) {
		SCSC_TAG_WARNING(MXMAN, "Skip to close a failed subsystem\n");
#if defined(CONFIG_SCSC_PCIE_CHIP)
		scsc_mx_service_release(SERVICE_CLOSE);
#endif
#ifdef CONFIG_SCSC_COMMON_ANDROID
		wake_unlock(&srvman->sm_wake_lock);
#endif
		mutex_unlock(&srvman->api_access_mutex);
		mxman_if_indep_recovery_err_completion(mxman);
		return 0;
	}
#endif
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mxman_if_close(mxman, sub);
#else
	mxman_close(mxman);
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
	scsc_mx_service_release(SERVICE_CLOSE);
#endif

#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_unlock(&srvman->sm_wake_lock);
#endif
	mutex_unlock(&srvman->api_access_mutex);
	return 0;
}
EXPORT_SYMBOL(scsc_mx_service_close);

#if IS_ENABLED(CONFIG_SCSC_FLASH_SERVICE_ON_BOOT)
static bool has_flash_service_open;
#endif
static struct scsc_service *__scsc_mx_service_open(struct scsc_mx *mx, enum scsc_service_id id, struct scsc_service_client *client, int *status, void *data, size_t data_sz)
{
	int                 ret;
	struct scsc_service *service = NULL;
	struct srvman       *srvman = scsc_mx_get_srvman(mx);
	struct mxman        *mxman = scsc_mx_get_mxman(mx);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	enum scsc_subsystem sub;
#endif
	bool                empty;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
	struct  __kernel_old_timeval tval = {};
#else
	struct timeval tval = {};
#endif

#ifdef CONFIG_HDM_WLBT_SUPPORT
	if(id == SCSC_SERVICE_ID_WLAN){
		if (mxman_if_get_hdm_wlan_support()) {
				SCSC_TAG_ERR(MXMAN, "hdm wlan support enabled. Reject wlan service\n");
				return NULL;
		} else {
			SCSC_TAG_DEBUG(MXMAN, "hdm wlan support disabled.\n");
		}
	}

	if(id == SCSC_SERVICE_ID_BT){
		if (mxman_if_get_hdm_bt_support()) {
				SCSC_TAG_ERR(MXMAN, "hdm bt support enabled. Reject bt service\n");
				return NULL;
		} else {
			SCSC_TAG_DEBUG(MXMAN, "hdm bt support disabled.\n");
		}
	}
#endif

#if IS_ENABLED(CONFIG_SCSC_FLASH_SERVICE_ON_BOOT)
	if (has_flash_service_open == false && id != SCSC_SERVICE_ID_FLASH) {
		SCSC_TAG_ERR(MXMAN, "FLASH_SERVICE_ON_BOOT is enabled but service %d is starting before\n", id);
		return NULL;
	}

	if (has_flash_service_open == false && id == SCSC_SERVICE_ID_FLASH) {
		SCSC_TAG_ERR(MXMAN, "FLASH_SERVICE_ON_BOOT is enabled. Service FLASH is starting for first time\n");
		has_flash_service_open = true;
	}
#endif

	SCSC_TAG_INFO(MXMAN, "service id: %d\n", id);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	sub = scsc_service_id_subsystem_mapping(id);
	if (sub == SCSC_SUBSYSTEM_INVALID) {
		SCSC_TAG_ERR(MXMAN, "Incorrect subsystem for service_id %u\n", id);
		return NULL;
	}
#endif
	mutex_lock(&srvman->api_access_mutex);
#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_lock(&srvman->sm_wake_lock);
#endif
	if (srvman_start_not_allowed(srvman)) {
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
		tval = ns_to_kernel_old_timeval(mxman_if_get_last_panic_time(mxman));
#else
		tval = ns_to_timeval(mxman_if_get_last_panic_time(mxman));
#endif
		SCSC_TAG_ERR(MXMAN, "error: refused due to previous f/w failure scsc_panic_code=0x%x happened at [%6lu.%06ld]\n",
				mxman_if_get_panic_code(mxman), tval.tv_sec, tval.tv_usec);

		/* Print the last panic record to help track ancient failures */
		mxman_if_show_last_panic(mxman);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
		tval = ns_to_kernel_old_timeval(mxman->last_panic_time);
#else
		tval = ns_to_timeval(mxman->last_panic_time);
#endif
		SCSC_TAG_ERR(MXMAN, "error: refused due to previous f/w failure scsc_panic_code=0x%x happened at [%6lu.%06ld]\n",
				mxman->scsc_panic_code, tval.tv_sec, tval.tv_usec);
		/* Print the last panic record to help track ancient failures */
		mxman_show_last_panic(mxman);
#endif
#ifdef CONFIG_SCSC_COMMON_ANDROID
		wake_unlock(&srvman->sm_wake_lock);
#endif
		mutex_unlock(&srvman->api_access_mutex);
		*status = -EILSEQ;
		return NULL;
	}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (mxman_if_subsys_in_failed_state(mxman, sub)) {
#else
	if (mxman->mxman_state == MXMAN_STATE_FAILED) {
#endif
		int r;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
		SCSC_TAG_INFO(MXMAN, "state = %d\n", mxman_if_get_state(mxman));
		mutex_unlock(&srvman->api_access_mutex);
		r = mxman_if_wait_for_completion_timeout(mxman, SCSC_MX_SERVICE_RECOVERY_TIMEOUT);
#else
		SCSC_TAG_INFO(MXMAN, "state = %d\n", mxman->mxman_state);

		mutex_unlock(&srvman->api_access_mutex);
		r = wait_for_completion_timeout(&mxman->recovery_completion,
						msecs_to_jiffies(SCSC_MX_SERVICE_RECOVERY_TIMEOUT));
#endif
		if (r == 0) {
			SCSC_TAG_ERR(MXMAN, "Recovery timeout\n");
#ifdef CONFIG_SCSC_COMMON_ANDROID
			wake_unlock(&srvman->sm_wake_lock);
#endif
			*status = -EIO;
			return NULL;
		}
		mutex_lock(&srvman->api_access_mutex);
	}

	service = kmalloc(sizeof(struct scsc_service), GFP_KERNEL);
	if (service) {
#if defined(CONFIG_SCSC_PCIE_CHIP)
		if (scsc_mx_service_claim(SERVICE_OPEN)) {
			kfree(service);
#ifdef CONFIG_SCSC_COMMON_ANDROID
			wake_unlock(&srvman->sm_wake_lock);
#endif
			mutex_unlock(&srvman->api_access_mutex);
			*status = -EFAULT;
			SCSC_TAG_INFO(MXMAN, "Error claiming link\n");
			return NULL;
		}
#endif
		/* MaxwellManager Should allocate Mem and download FW */
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
		ret = mxman_if_open(mxman, sub, data, data_sz);
#else
		ret = mxman_open(mxman);
#endif
		if (ret) {
			kfree(service);
#ifdef CONFIG_SCSC_COMMON_ANDROID
			wake_unlock(&srvman->sm_wake_lock);
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
			scsc_mx_service_release(SERVICE_OPEN);
#endif
			mutex_unlock(&srvman->api_access_mutex);
			*status = ret;
			return NULL;
		}
		/* Initialise service struct here */
		service->mx = mx;
		service->id = id;
		service->client = client;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
		service->subsystem_type = sub;
#endif
		init_completion(&service->sm_msg_start_completion);
		init_completion(&service->sm_msg_stop_completion);
		mutex_lock(&srvman->service_list_mutex);
		empty = list_empty(&srvman->service_list);
		mutex_unlock(&srvman->service_list_mutex);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
		/* Create service management transports */
		if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
			mxmgmt_transport_register_channel_handler(scsc_mx_get_mxmgmt_transport(mx), MMTRANS_CHAN_ID_SERVICE_MANAGEMENT,
								  &srv_message_handler, srvman);

		else if (service->subsystem_type == SCSC_SUBSYSTEM_WPAN)
			mxmgmt_transport_register_channel_handler(scsc_mx_get_mxmgmt_transport_wpan(mx), MMTRANS_CHAN_ID_SERVICE_MANAGEMENT,
								  &srv_message_handler, srvman);
#else
		if (empty)
			mxmgmt_transport_register_channel_handler(scsc_mx_get_mxmgmt_transport(mx), MMTRANS_CHAN_ID_SERVICE_MANAGEMENT,
								  &srv_message_handler, srvman);
#endif
		mutex_lock(&srvman->service_list_mutex);
		list_add_tail(&service->list, &srvman->service_list);
		mutex_unlock(&srvman->service_list_mutex);
	} else {
		*status = -ENOMEM;
	}

#ifdef CONFIG_SCSC_COMMON_ANDROID
	wake_unlock(&srvman->sm_wake_lock);
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
	scsc_mx_service_release(SERVICE_OPEN);
#endif
	mutex_unlock(&srvman->api_access_mutex);

	return service;
}

#if IS_ENABLED(CONFIG_SCSC_FLASH_SERVICE) || IS_ENABLED(CONFIG_SCSC_BOOT_SERVICE)
static bool service_lock = false;
static enum scsc_service_id service_locked;

void scsc_mx_service_lock_open(struct scsc_mx *mx, enum scsc_service_id id)
{
	if (service_lock == false) {
		service_lock = true;
		service_locked = id;
		SCSC_TAG_INFO(MXMAN, "Service id %d has locked scsc_mx_open\n", service_locked);
	} else {
		SCSC_TAG_INFO(MXMAN, "Service id %d failed to lock scsc_mx_open\n", service_locked);
	}
}
EXPORT_SYMBOL(scsc_mx_service_lock_open);

void scsc_mx_service_unlock_open(struct scsc_mx *mx, enum scsc_service_id id)
{
	if (service_lock == true && id == service_locked) {
		SCSC_TAG_INFO(MXMAN, "Service id %d has unlocked scsc_mx_open\n", service_locked);
		service_lock = false;
		service_locked = SCSC_SERVICE_ID_INVALID;
	} else {
		SCSC_TAG_INFO(MXMAN, "Service id %d failed to unlock scsc_mx_open\n", service_locked);
	}
}
EXPORT_SYMBOL(scsc_mx_service_unlock_open);

bool scsc_mx_service_users_active(struct scsc_mx *mx)
{
	struct mxman *mxman = scsc_mx_get_mxman(mx);
	return mxman_if_users_active(mxman);
}
EXPORT_SYMBOL(scsc_mx_service_users_active);
#endif

struct scsc_service *scsc_mx_service_open(struct scsc_mx *mx, enum scsc_service_id id, struct scsc_service_client *client, int *status)
{
#if IS_ENABLED(CONFIG_SCSC_FLASH_SERVICE) || IS_ENABLED(CONFIG_SCSC_BOOT_SERVICE)
	if (service_lock == true && id != service_locked) {
		SCSC_TAG_INFO(MXMAN, "scsc_mx_open blocked by service id %d\n", service_locked);
		return NULL;
	}
#endif
	return __scsc_mx_service_open(mx, id, client, status, NULL, 0);
}
EXPORT_SYMBOL(scsc_mx_service_open);

struct scsc_service *scsc_mx_service_open_boot_data(struct scsc_mx *mx, enum scsc_service_id id, struct scsc_service_client *client, int *status, void *data, size_t data_sz)
{
#if IS_ENABLED(CONFIG_SCSC_FLASH_SERVICE) || IS_ENABLED(CONFIG_SCSC_BOOT_SERVICE)
	if (service_lock == true && id != service_locked) {
		SCSC_TAG_INFO(MXMAN, "scsc_mx_open_boot_data blocked by service id %d\n", service_locked);
		return NULL;
	}
#endif
	return __scsc_mx_service_open(mx, id, client, status, data, data_sz);
}
EXPORT_SYMBOL(scsc_mx_service_open_boot_data);

struct scsc_bt_audio_abox *scsc_mx_service_get_bt_audio_abox(struct scsc_service *service)
{
	struct scsc_mx      *mx = service->mx;
	struct mifabox      *ptr;

	ptr = scsc_mx_get_aboxram(mx);

	return ptr->aboxram;
}
EXPORT_SYMBOL(scsc_mx_service_get_bt_audio_abox);

struct mifabox *scsc_mx_service_get_aboxram(struct scsc_service *service)
{
	struct scsc_mx      *mx = service->mx;
	struct mifabox      *ptr;

	ptr = scsc_mx_get_aboxram(mx);

	return ptr;
}

/**
 * Allocate a contiguous block of SDRAM accessible to Client Driver
 *
 * When allocation fails, beside returning -ENOMEM, the IN-param 'ref'
 * is cleared to an INVALID value that can be safely fed to the companion
 * function scsc_mx_service_mifram_free().
 */
int scsc_mx_service_mifram_alloc_extended(struct scsc_service *service, size_t nbytes, scsc_mifram_ref *ref, u32 align, uint32_t flags)
{
	struct scsc_mx      *mx = service->mx;
	void                *mem;
	int                 ret;
	struct miframman    *ramman;

	if (flags & MIFRAMMAN_MEM_POOL_GENERIC) {
		ramman = scsc_mx_get_ramman(mx);
	} else if (flags & MIFRAMMAN_MEM_POOL_LOGGING) {
		ramman = scsc_mx_get_ramman2(mx);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	} else if (flags & MIFRAMMAN_MEM_POOL_WPAN) {
		ramman = scsc_mx_get_ramman_wpan(mx);
#endif
	} else {
		SCSC_TAG_ERR(MXMAN, "Unsupported flags value: %d\n", flags);
		*ref = SCSC_MIFRAM_INVALID_REF;
		return -ENOMEM;
	}

	mem = miframman_alloc(ramman, nbytes, align, service->id);
	if (!mem) {
		SCSC_TAG_ERR(MXMAN, "miframman_alloc() failed\n");
		*ref = SCSC_MIFRAM_INVALID_REF;
		return -ENOMEM;
	}

	SCSC_TAG_DEBUG(MXMAN, "Allocated mem %p\n", mem);

	/* Transform native pointer and get mifram_ref type */
	ret = scsc_mx_service_mif_ptr_to_addr(service, mem, ref);
	if (ret) {
		SCSC_TAG_ERR(MXMAN, "scsc_mx_service_mif_ptr_to_addr() failed: ret=%d\n", ret);
		miframman_free(ramman, mem);
		*ref = SCSC_MIFRAM_INVALID_REF;
	} else {
		SCSC_TAG_DEBUG(MXMAN, "mem %p ref %d\n", mem, *ref);
	}
	return ret;
}
EXPORT_SYMBOL(scsc_mx_service_mifram_alloc_extended);

int scsc_mx_service_mifram_alloc(struct scsc_service *service, size_t nbytes, scsc_mifram_ref *ref, u32 align)
{
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WPAN)
		return scsc_mx_service_mifram_alloc_extended(service, nbytes, ref, align, MIFRAMMAN_MEM_POOL_WPAN);
#endif
	return scsc_mx_service_mifram_alloc_extended(service, nbytes, ref, align, MIFRAMMAN_MEM_POOL_GENERIC);
}
EXPORT_SYMBOL(scsc_mx_service_mifram_alloc);

/** Free a contiguous block of SDRAM */
void scsc_mx_service_mifram_free_extended(struct scsc_service *service, scsc_mifram_ref ref, uint32_t flags)
{
	struct scsc_mx *mx = service->mx;
	void           *mem;
	struct miframman    *ramman;

	if (flags & MIFRAMMAN_MEM_POOL_GENERIC) {
		ramman = scsc_mx_get_ramman(mx);
	} else if (flags & MIFRAMMAN_MEM_POOL_LOGGING) {
		ramman = scsc_mx_get_ramman2(mx);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	} else if (flags & MIFRAMMAN_MEM_POOL_WPAN) {
		ramman = scsc_mx_get_ramman_wpan(mx);
#endif
	} else {
		SCSC_TAG_ERR(MXMAN, "Unsupported flags value: %d\n", flags);
		return;
	}

	mem = scsc_mx_service_mif_addr_to_ptr(service, ref);

	SCSC_TAG_DEBUG(MXMAN, "**** Freeing %p\n", mem);

	miframman_free(ramman, mem);
}
EXPORT_SYMBOL(scsc_mx_service_mifram_free_extended);

void scsc_mx_service_mifram_free(struct scsc_service *service, scsc_mifram_ref ref)
{
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WPAN)
		scsc_mx_service_mifram_free_extended(service, ref, MIFRAMMAN_MEM_POOL_WPAN);
	else
		scsc_mx_service_mifram_free_extended(service, ref, MIFRAMMAN_MEM_POOL_GENERIC);
#else
	scsc_mx_service_mifram_free_extended(service, ref, MIFRAMMAN_MEM_POOL_GENERIC);
#endif
}
EXPORT_SYMBOL(scsc_mx_service_mifram_free);

/* MIF ALLOCATIONS */
bool scsc_mx_service_alloc_mboxes(struct scsc_service *service, int n, int *first_mbox_index)
{
	struct scsc_mx *mx = service->mx;

	return mifmboxman_alloc_mboxes(scsc_mx_get_mboxman(mx), n, first_mbox_index);
}
EXPORT_SYMBOL(scsc_mx_service_alloc_mboxes);

void scsc_service_free_mboxes(struct scsc_service *service, int n, int first_mbox_index)
{
	struct scsc_mx *mx = service->mx;

	return mifmboxman_free_mboxes(scsc_mx_get_mboxman(mx), first_mbox_index, n);
}
EXPORT_SYMBOL(scsc_service_free_mboxes);

u32 *scsc_mx_service_get_mbox_ptr(struct scsc_service *service, int mbox_index)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

#if !defined(CONFIG_SCSC_PCIE_CHIP)
	return mifmboxman_get_mbox_ptr(scsc_mx_get_mboxman(mx), mif_abs, mbox_index);
#else
	return NULL;
#endif
}
EXPORT_SYMBOL(scsc_mx_service_get_mbox_ptr);

int scsc_service_mifintrbit_bit_mask_status_get(struct scsc_service *service)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
		return mif_abs->irq_bit_mask_status_get(mif_abs, SCSC_MIF_ABS_TARGET_WLAN);
	else
		return mif_abs->irq_bit_mask_status_get(mif_abs, SCSC_MIF_ABS_TARGET_WPAN);
#else
	return mif_abs->irq_bit_mask_status_get(mif_abs);
#endif
}
EXPORT_SYMBOL(scsc_service_mifintrbit_bit_mask_status_get);

int scsc_service_mifintrbit_get(struct scsc_service *service)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
		return mif_abs->irq_get(mif_abs, SCSC_MIF_ABS_TARGET_WLAN);
	else
		return mif_abs->irq_get(mif_abs, SCSC_MIF_ABS_TARGET_WPAN);
#else
	return mif_abs->irq_get(mif_abs);
#endif
}
EXPORT_SYMBOL(scsc_service_mifintrbit_get);

void scsc_service_mifintrbit_bit_set(struct scsc_service *service, int which_bit, enum scsc_mifintr_target dir)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
		return mif_abs->irq_bit_set(mif_abs, which_bit, SCSC_MIF_ABS_TARGET_WLAN);
	else
		return mif_abs->irq_bit_set(mif_abs, which_bit, SCSC_MIF_ABS_TARGET_WPAN);
#else
	/* on single subsystems all the IRQ will go to WLAN Abstract core*/
	return mif_abs->irq_bit_set(mif_abs, which_bit, (enum scsc_mif_abs_target)SCSC_MIF_ABS_TARGET_WLAN);
#endif
}
EXPORT_SYMBOL(scsc_service_mifintrbit_bit_set);

void scsc_service_mifintrbit_bit_clear(struct scsc_service *service, int which_bit)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
		return mif_abs->irq_bit_clear(mif_abs, which_bit, SCSC_MIF_ABS_TARGET_WLAN);
	else
		return mif_abs->irq_bit_clear(mif_abs, which_bit, SCSC_MIF_ABS_TARGET_WPAN);
#else
	return mif_abs->irq_bit_clear(mif_abs, which_bit);
#endif
}
EXPORT_SYMBOL(scsc_service_mifintrbit_bit_clear);

void scsc_service_mifintrbit_bit_mask(struct scsc_service *service, int which_bit)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
		return mif_abs->irq_bit_mask(mif_abs, which_bit, SCSC_MIF_ABS_TARGET_WLAN);
	else
		return mif_abs->irq_bit_mask(mif_abs, which_bit, SCSC_MIF_ABS_TARGET_WPAN);
#else
	return mif_abs->irq_bit_mask(mif_abs, which_bit);
#endif
}
EXPORT_SYMBOL(scsc_service_mifintrbit_bit_mask);

void scsc_service_mifintrbit_bit_unmask(struct scsc_service *service, int which_bit)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (service->subsystem_type == SCSC_SUBSYSTEM_WLAN)
		return mif_abs->irq_bit_unmask(mif_abs, which_bit, SCSC_MIF_ABS_TARGET_WLAN);
	else
		return mif_abs->irq_bit_unmask(mif_abs, which_bit, SCSC_MIF_ABS_TARGET_WPAN);
#else
	return mif_abs->irq_bit_unmask(mif_abs, which_bit);
#endif
}
EXPORT_SYMBOL(scsc_service_mifintrbit_bit_unmask);

int scsc_service_mifintrbit_alloc_fromhost(struct scsc_service *service, enum scsc_mifintr_target dir)
{
	struct scsc_mx *mx = service->mx;
	struct mifintrbit  *mifirq;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	/* Get mifintbit instance - wLAN or BT */
	if (dir == SCSC_MIFINTR_TARGET_WLAN)
		mifirq = scsc_mx_get_intrbit(mx);
	else
		mifirq = scsc_mx_get_intrbit_wpan(mx);
#else
	/* Get only the WLAN instance */
	mifirq = scsc_mx_get_intrbit(mx);
#endif

	if (!mifirq) {
		SCSC_TAG_ERR(MXMAN, "MIFINTR instance does not exist for %u\n", dir);
		return -EIO;
	}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	return mifintrbit_alloc_fromhost(mifirq);
#else
	/* on single subsystems all the IRQ will go to WLAN Abstract core*/
	return mifintrbit_alloc_fromhost(mifirq, (enum scsc_mif_abs_target)SCSC_MIF_ABS_TARGET_WLAN);
#endif
}
EXPORT_SYMBOL(scsc_service_mifintrbit_alloc_fromhost);

int scsc_service_mifintrbit_free_fromhost(struct scsc_service *service, int which_bit, enum scsc_mifintr_target dir)
{
	struct scsc_mx *mx = service->mx;
	struct mifintrbit  *mifirq;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	/* Get mifintbit instance - wLAN or BT */
	if (dir == SCSC_MIFINTR_TARGET_WLAN)
		mifirq = scsc_mx_get_intrbit(mx);
	else
		mifirq = scsc_mx_get_intrbit_wpan(mx);
#else
	/* Get only the WLAN instance */
	mifirq = scsc_mx_get_intrbit(mx);
#endif

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	return mifintrbit_free_fromhost(mifirq, which_bit);
#else
	/* on single subsystems all the IRQ will go to WLAN Abstract core*/
	return mifintrbit_free_fromhost(mifirq, which_bit, (enum scsc_mif_abs_target)SCSC_MIFINTR_TARGET_WLAN);
#endif
}
EXPORT_SYMBOL(scsc_service_mifintrbit_free_fromhost);

int scsc_service_mifintrbit_register_tohost(struct scsc_service *service, void (*handler)(int irq, void *data), void *data, enum scsc_mifintr_target dir, enum IRQ_TYPE irq_type)
{
	struct scsc_mx *mx = service->mx;
	struct mifintrbit  *mifirq;

	SCSC_TAG_DEBUG(MXMAN, "Registering %pS\n", handler);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	/* Get mifintbit instance - wLAN or BT */
	if (dir == SCSC_MIFINTR_TARGET_WLAN)
		mifirq = scsc_mx_get_intrbit(mx);
	else
		mifirq = scsc_mx_get_intrbit_wpan(mx);
#else
	/* Get only the WLAN instance */
	mifirq = scsc_mx_get_intrbit(mx);
#endif

	return mifintrbit_alloc_tohost(mifirq, handler, data, irq_type);
}
EXPORT_SYMBOL(scsc_service_mifintrbit_register_tohost);

int scsc_service_mifintrbit_unregister_tohost(struct scsc_service *service, int which_bit, enum scsc_mifintr_target dir)
{
	struct scsc_mx *mx = service->mx;
	struct mifintrbit  *mifirq;

	SCSC_TAG_DEBUG(MXMAN, "Deregistering int for bit %d\n", which_bit);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	/* Get mifintbit instance - wLAN or BT */
	if (dir == SCSC_MIFINTR_TARGET_WLAN)
		mifirq = scsc_mx_get_intrbit(mx);
	else
		mifirq = scsc_mx_get_intrbit_wpan(mx);
#else
	/* Get only the WLAN instance */
	mifirq = scsc_mx_get_intrbit(mx);
#endif

	return mifintrbit_free_tohost(mifirq, which_bit);
}
EXPORT_SYMBOL(scsc_service_mifintrbit_unregister_tohost);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#if defined(CONFIG_SCSC_PCIE_CHIP)
__iomem void *scsc_mx_service_get_ramrp_ptr(struct scsc_service *service)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	return mif_abs->get_ramrp_ptr(mif_abs);
}
EXPORT_SYMBOL(scsc_mx_service_get_ramrp_ptr);
#endif
#endif

void *scsc_mx_service_mif_addr_to_ptr(struct scsc_service *service, scsc_mifram_ref ref)
{
	struct scsc_mx      *mx = service->mx;

	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	return mif_abs->get_mifram_ptr(mif_abs, ref);
}
EXPORT_SYMBOL(scsc_mx_service_mif_addr_to_ptr);

void *scsc_mx_service_mif_addr_to_phys(struct scsc_service *service, scsc_mifram_ref ref)
{
	struct scsc_mx      *mx = service->mx;

	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	if (mif_abs->get_mifram_phy_ptr)
		return mif_abs->get_mifram_phy_ptr(mif_abs, ref);
	else
		return NULL;
}
EXPORT_SYMBOL(scsc_mx_service_mif_addr_to_phys);

int scsc_mx_service_mif_ptr_to_addr(struct scsc_service *service, void *mem_ptr, scsc_mifram_ref *ref)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	/* Transform native pointer and get mifram_ref type */
	if (mif_abs->get_mifram_ref(mif_abs, mem_ptr, ref)) {
		SCSC_TAG_ERR(MXMAN, "ooops something went wrong\n");
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL(scsc_mx_service_mif_ptr_to_addr);

int scsc_mx_service_mif_dump_registers(struct scsc_service *service)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	/* Dump registers */
	mif_abs->mif_dump_registers(mif_abs);

	return 0;
}
EXPORT_SYMBOL(scsc_mx_service_mif_dump_registers);

struct device *scsc_service_get_device(struct scsc_service *service)
{
	return scsc_mx_get_device(service->mx);
}
EXPORT_SYMBOL(scsc_service_get_device);

struct device *scsc_service_get_device_by_mx(struct scsc_mx *mx)
{
	return scsc_mx_get_device(mx);
}
EXPORT_SYMBOL(scsc_service_get_device_by_mx);

/* Force a FW panic for test purposes only */
int scsc_service_force_panic(struct scsc_service *service)
{
	struct mxman   *mxman = scsc_mx_get_mxman(service->mx);

	SCSC_TAG_INFO(MXMAN, "%d\n", service->id);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	return mxman_if_force_panic(mxman, service->subsystem_type);
#else
	return mxman_force_panic(mxman);
#endif
}
EXPORT_SYMBOL(scsc_service_force_panic);

#ifdef CONFIG_SCSC_SMAPPER
u16 scsc_service_get_alignment(struct scsc_service *service)
{
	struct scsc_mx *mx = service->mx;

	return mifsmapper_get_alignment(scsc_mx_get_smapper(mx));
}
EXPORT_SYMBOL(scsc_service_get_alignment);

int scsc_service_mifsmapper_alloc_bank(struct scsc_service *service, bool large_bank, u32 entry_size, u16 *entries)
{
	struct scsc_mx *mx = service->mx;

	return mifsmapper_alloc_bank(scsc_mx_get_smapper(mx), large_bank, entry_size, entries);
}
EXPORT_SYMBOL(scsc_service_mifsmapper_alloc_bank);

void scsc_service_mifsmapper_configure(struct scsc_service *service, u32 granularity)
{
	struct scsc_mx *mx = service->mx;

	mifsmapper_configure(scsc_mx_get_smapper(mx), granularity);
}
EXPORT_SYMBOL(scsc_service_mifsmapper_configure);

int scsc_service_mifsmapper_write_sram(struct scsc_service *service, u8 bank, u8 num_entries, u8 first_entry, dma_addr_t *addr)
{
	struct scsc_mx *mx = service->mx;

	return mifsmapper_write_sram(scsc_mx_get_smapper(mx), bank, num_entries, first_entry, addr);
}
EXPORT_SYMBOL(scsc_service_mifsmapper_write_sram);

int scsc_service_mifsmapper_get_entries(struct scsc_service *service, u8 bank, u8 num_entries, u8 *entries)
{
	struct scsc_mx *mx = service->mx;

	return mifsmapper_get_entries(scsc_mx_get_smapper(mx), bank, num_entries, entries);
}
EXPORT_SYMBOL(scsc_service_mifsmapper_get_entries);

int scsc_service_mifsmapper_free_entries(struct scsc_service *service, u8 bank, u8 num_entries, u8 *entries)
{
	struct scsc_mx *mx = service->mx;

	return mifsmapper_free_entries(scsc_mx_get_smapper(mx), bank, num_entries, entries);
}
EXPORT_SYMBOL(scsc_service_mifsmapper_free_entries);

int scsc_service_mifsmapper_free_bank(struct scsc_service *service, u8 bank)
{
	struct scsc_mx *mx = service->mx;

	return mifsmapper_free_bank(scsc_mx_get_smapper(mx), bank);
}
EXPORT_SYMBOL(scsc_service_mifsmapper_free_bank);

u32 scsc_service_mifsmapper_get_bank_base_address(struct scsc_service *service, u8 bank)
{
	struct scsc_mx *mx = service->mx;

	return mifsmapper_get_bank_base_address(scsc_mx_get_smapper(mx), bank);
}
EXPORT_SYMBOL(scsc_service_mifsmapper_get_bank_base_address);
#endif

#ifdef CONFIG_SCSC_QOS
#if defined(CONFIG_SCSC_PCIE_CHIP)
int scsc_service_set_affinity_cpu(struct scsc_service *service, u8 msi, u8 cpu)
#else
int scsc_service_set_affinity_cpu(struct scsc_service *service, u8 cpu)
#endif
{
	struct scsc_mx      *mx = service->mx;
	int ret = 0;

#if defined(CONFIG_SCSC_PCIE_CHIP)
	ret = mifqos_set_affinity_cpu(scsc_mx_get_qos(mx), msi, cpu);
#else
	ret = mifqos_set_affinity_cpu(scsc_mx_get_qos(mx), cpu);
#endif

	return ret;
}
EXPORT_SYMBOL(scsc_service_set_affinity_cpu);

int scsc_service_pm_qos_add_request(struct scsc_service *service, enum scsc_qos_config config)
{
	struct scsc_mx      *mx = service->mx;

	mifqos_add_request(scsc_mx_get_qos(mx), service->id, config);

	return 0;
}
EXPORT_SYMBOL(scsc_service_pm_qos_add_request);

#if IS_ENABLED(CONFIG_SCSC_WLAN_SEPARATED_CLUSTER_FREQUENCY)
int scsc_service_pm_qos_update_cluster_request(struct scsc_service *service, enum scsc_qos_config config)
{
	struct scsc_mx      *mx = service->mx;

	mifqos_cluster_update_request(scsc_mx_get_qos(mx), service->id, config);

	return 0;
}
EXPORT_SYMBOL(scsc_service_pm_qos_update_cluster_request);
#endif

int scsc_service_pm_qos_update_request(struct scsc_service *service, enum scsc_qos_config config)
{
	struct scsc_mx      *mx = service->mx;

	mifqos_update_request(scsc_mx_get_qos(mx), service->id, config);

	return 0;
}
EXPORT_SYMBOL(scsc_service_pm_qos_update_request);

int scsc_service_pm_qos_remove_request(struct scsc_service *service)
{
	struct scsc_mx      *mx = service->mx;

	if (!mx)
		return -EIO;

	mifqos_remove_request(scsc_mx_get_qos(mx), service->id);

	return 0;
}
EXPORT_SYMBOL(scsc_service_pm_qos_remove_request);
#endif
#if IS_ENABLED(CONFIG_SCSC_MXLOGGER)
/* If there is no service/mxman associated, register the observer as global (will affect all the mx instanes)*/
/* Users of these functions should ensure that the registers/unregister functions are balanced (i.e. if observer is registed as global,
 * it _has_ to unregister as global) */
int scsc_service_register_observer(struct scsc_service *service, char *name)
{
	struct scsc_mx      *mx;

	if (!service)
		return mxlogger_register_global_observer(name);

	mx = service->mx;

	if (!mx)
		return -EIO;

	return mxlogger_register_observer(scsc_mx_get_mxlogger(mx), name);
}
EXPORT_SYMBOL(scsc_service_register_observer);

/* If there is no service/mxman associated, unregister the observer as global (will affect all the mx instanes)*/
int scsc_service_unregister_observer(struct scsc_service *service, char *name)
{
	struct scsc_mx      *mx;

	if (!service)
		return mxlogger_unregister_global_observer(name);

	mx = service->mx;

	if (!mx)
		return -EIO;

	return mxlogger_unregister_observer(scsc_mx_get_mxlogger(mx), name);
}
EXPORT_SYMBOL(scsc_service_unregister_observer);
#endif

bool scsc_mx_service_property_read_bool(struct scsc_service *service, const char *propname)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	if (!mif_abs->wlbt_property_read_bool)
		return false;

	return mif_abs->wlbt_property_read_bool(mif_abs, propname);
}
EXPORT_SYMBOL(scsc_mx_service_property_read_bool);

int scsc_mx_service_property_read_u8(struct scsc_service *service, const char *propname, u8 *out_value, size_t size)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	if (!mif_abs->wlbt_property_read_u8)
		return -ENOSYS;

	return mif_abs->wlbt_property_read_u8(mif_abs, propname, out_value, size);
}
EXPORT_SYMBOL(scsc_mx_service_property_read_u8);

int scsc_mx_service_property_read_u16(struct scsc_service *service, const char *propname, u16 *out_value, size_t size)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	if (!mif_abs->wlbt_property_read_u16)
		return -ENOSYS;

	return mif_abs->wlbt_property_read_u16(mif_abs, propname, out_value, size);
}
EXPORT_SYMBOL(scsc_mx_service_property_read_u16);

int scsc_mx_service_property_read_u32(struct scsc_service *service, const char *propname, u32 *out_value, size_t size)
{
	struct scsc_mx      *mx = service->mx;

	return scsc_mx_property_read_u32(mx, propname, out_value, size);
}
EXPORT_SYMBOL(scsc_mx_service_property_read_u32);

int scsc_mx_property_read_u32(struct scsc_mx *mx, const char *propname, u32 *out_value, size_t size)
{
	struct scsc_mif_abs *mif_abs = scsc_mx_get_mif_abs(mx);

	if (!mif_abs->wlbt_property_read_u32)
		return -ENOSYS;

	return mif_abs->wlbt_property_read_u32(mif_abs, propname, out_value, size);
}
EXPORT_SYMBOL(scsc_mx_property_read_u32);

int scsc_mx_service_property_read_string(struct scsc_service *service,
					 const char *propname, char **out_value, size_t size)
{
	struct scsc_mx      *mx = service->mx;
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);

	if (!mif_abs->wlbt_property_read_string)
		return -EINVAL;

	return mif_abs->wlbt_property_read_string(mif_abs, propname, out_value, size);
}
EXPORT_SYMBOL(scsc_mx_service_property_read_string);

static int __service_phandle_property_read_u32(struct scsc_mx *mx, const char *phandle_name,
					const char *propname, u32 *out_value, size_t size)
{
	struct scsc_mif_abs *mif_abs;

	mif_abs = scsc_mx_get_mif_abs(mx);
	if (!mif_abs->wlbt_phandle_property_read_u32)
		return -EINVAL;
	return mif_abs->wlbt_phandle_property_read_u32(mif_abs, phandle_name, propname, out_value, size);
}

int scsc_mx_service_phandle_property_read_u32(struct scsc_service *service, const char *phandle_name,
					 const char *propname, u32 *out_value, size_t size)
{
	struct scsc_mx      *mx = service->mx;

	return __service_phandle_property_read_u32(mx, phandle_name, propname, out_value, size);
}
EXPORT_SYMBOL(scsc_mx_service_phandle_property_read_u32);

int scsc_mx_phandle_property_read_u32(struct scsc_mx *mx, const char *phandle_name, const char *propname,
					u32 *out_value, size_t size)
{
	return __service_phandle_property_read_u32(mx, phandle_name, propname, out_value, size);
}
EXPORT_SYMBOL(scsc_mx_phandle_property_read_u32);

int scsc_service_get_panic_record(struct scsc_service *service, u8 *dst, u16 max_size)
{
	struct mxman   *mxman;

	if (!service) {
		SCSC_TAG_DEBUG(MXMAN, "Service is NULL\n");
		return 0;
	}

	mxman = scsc_mx_get_mxman(service->mx);

	if (!mxman) {
		SCSC_TAG_DEBUG(MXMAN, "Mxman is NULL\n");
		return 0;
	}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	/* last_panic_rec_sz is "integer" size, so requires multiplication by 4 to convert into bytes */
	if ((4 * mxman_if_get_last_panic_rec_sz(mxman)) > max_size) {
		SCSC_TAG_DEBUG(MXMAN, "Record size %d larger than max size %d\n", mxman_if_get_last_panic_rec_sz(mxman) * 4, max_size);
		return 0;
	}

	memcpy(dst, (u8 *)mxman_if_get_last_panic_rec(mxman), mxman_if_get_last_panic_rec_sz(mxman));

	return mxman_if_get_last_panic_rec_sz(mxman);
#else
	/* last_panic_rec_sz is "integer" size, so requires multiplication by 4 to convert into bytes */
	if ((4 * mxman->last_panic_rec_sz) > max_size) {
		SCSC_TAG_DEBUG(MXMAN, "Record size %d larger than max size %d\n", mxman->last_panic_rec_sz * 4, max_size);
		return 0;
	}

	memcpy(dst, (u8 *)mxman->last_panic_rec_r, mxman->last_panic_rec_sz * 4);

	return mxman->last_panic_rec_sz;
#endif
}
EXPORT_SYMBOL(scsc_service_get_panic_record);

size_t scsc_service_mxlogger_buff_size(struct scsc_service *service, enum scsc_log_chunk_type fw_buffer,
				       enum scsc_mifintr_target dir)
{
#if IS_ENABLED(CONFIG_SCSC_MXLOGGER) && IS_ENABLED(CONFIG_SCSC_LOG_COLLECTION)
	struct scsc_mx *mx;
	enum scsc_mif_abs_target target;

	if (!service) {
		SCSC_TAG_DEBUG(MXMAN, "Service is NULL\n");
		return 0;
	}

	mx = service->mx;
	if (!mx) {
		SCSC_TAG_DEBUG(MXMAN, "mx is NULL\n");
		return 0;
	}

	if (dir == SCSC_MIFINTR_TARGET_WLAN)
		target = SCSC_MIF_ABS_TARGET_WLAN;
	else
		target = SCSC_MIF_ABS_TARGET_WPAN;

	return mxlogger_get_fw_buf_size(scsc_mx_get_mxlogger(mx), fw_buffer, target);
#else
	SCSC_TAG_INFO(MXMAN, "MX LOGGING and LOG collection is disabled\n");
	return 0;
#endif
}
EXPORT_SYMBOL(scsc_service_mxlogger_buff_size);

size_t scsc_service_collect_buffer(struct scsc_service *service, enum scsc_log_chunk_type fw_buffer,
				   void *buffer, size_t size, enum scsc_mifintr_target dir)
{
#if IS_ENABLED(CONFIG_SCSC_MXLOGGER) && IS_ENABLED(CONFIG_SCSC_LOG_COLLECTION)
	struct scsc_mx *mx;
	size_t bytes = 0;
	enum scsc_mif_abs_target target;

	if (!service) {
		SCSC_TAG_DEBUG(MXMAN, "Service is NULL\n");
		goto exit;
	}

	mx = service->mx;

	if (!mx) {
		SCSC_TAG_DEBUG(MXMAN, "mx is NULL\n");
		goto exit;
	}

	if (dir == SCSC_MIFINTR_TARGET_WLAN)
		target = SCSC_MIF_ABS_TARGET_WLAN;
	else
		target = SCSC_MIF_ABS_TARGET_WPAN;


	bytes = mxlogger_dump_fw_buf(scsc_mx_get_mxlogger(mx), fw_buffer, buffer, size, target);
	if (bytes) {
		SCSC_TAG_DEBUG(MXMAN, "Data of size %d bytes stored in buffer\n", bytes);
		return bytes;
	}
	SCSC_TAG_DEBUG(MXMAN, "Unable to dump buffer\n");
exit:
	return 0;
#else
	SCSC_TAG_INFO(MXMAN, "MX LOGGING and LOG collection is disabled\n");
	return 0;
#endif
}
EXPORT_SYMBOL(scsc_service_collect_buffer);

#if defined(CONFIG_SLSI_WLAN_LPC)
void* scsc_service_mxlogger_buff(struct scsc_service *service)
{
#if IS_ENABLED(CONFIG_SCSC_MXLOGGER) && IS_ENABLED(CONFIG_SCSC_LOG_COLLECTION)
	struct scsc_mx *mx;

	if (!service) {
		SCSC_TAG_DEBUG(MXMAN, "Service is NULL\n");
		return NULL;
	}

	mx = service->mx;
	if (!mx) {
		SCSC_TAG_DEBUG(MXMAN, "mx is NULL\n");
		return NULL;
	}

	return mxlogger_get_fw_buf_for_wlan_lpc(scsc_mx_get_mxlogger(mx));
#else
	SCSC_TAG_INFO(MXMAN, "MX LOGGING and LOG collection is disabled\n");
	return NULL;
#endif
}
EXPORT_SYMBOL(scsc_service_mxlogger_buff);
#endif

#if defined(CONFIG_SCSC_RUNTIMEPM)
int scsc_service_set_fw_runtime_pm(struct scsc_service *service, int runtime_pm)
{
	int ret = 0;
	struct scsc_mif_abs *mif_abs;

	if (!service) {
		SCSC_TAG_ERR(MXMAN, "Service is NULL\n");
		return -EINVAL;
	}

	if (scsc_mx_service_claim(RUNTIME_PM)) {
		SCSC_TAG_ERR(MXMAN, "Error claiming link\n");
		return -ENODEV;
	}

	mif_abs = scsc_mx_get_mif_abs(service->mx);
	ret = mif_abs->set_fw_runtime_pm(mif_abs, runtime_pm);
	scsc_mx_service_release(RUNTIME_PM);

	return ret;
}
EXPORT_SYMBOL(scsc_service_set_fw_runtime_pm);
#endif
