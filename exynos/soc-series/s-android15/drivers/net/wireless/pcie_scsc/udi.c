/****************************************************************************
 *
 * Copyright (c) 2012 - 2018 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#include <linux/version.h>
#include <linux/sysfs.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <pcie_scsc/scsc_warn.h>

#include "dev.h"

#include "hip.h"
#include "log_clients.h"
#include "mlme.h"
#include "fw_test.h"
#include "debug.h"
#include "udi.h"
#include "src_sink.h"
#include "unifiio.h"
#include "procfs.h"
#include "sap_mlme.h"
#include "mgt.h"

#ifdef SLSI_TEST_DEV
#include "unittest.h"
#define UDI_CHAR_DEVICE_NAME "s5n2560unittest"
#define UDI_CLASS_NAME "s5n2560test"
#else
#define UDI_CHAR_DEVICE_NAME "s5n2560udi"
#define UDI_CLASS_NAME "s5n2560"
#endif

#define UDI_LOG_MASK_FILTER_NUM_MAX 5

#define UDI_MIB_SET_LEN_MAX 65535
#define UDI_MIB_GET_LEN_MAX 2048

#ifndef ETH_P_WAPI
#define ETH_P_WAPI 0x88b4
#endif

#define SLSI_IP_TYPE_UDP 0x11
#define SLSI_DHCP_SERVER_PORT 67
#define SLSI_DHCP_CLIENT_PORT 68
#define SLSI_DHCP_MAGIC_OFFSET 272
#define SLSI_DHCP_MESSAGE_TYPE_ACK 0x05

/**
 * Control character device for debug
 * ==================================
 */
#define NUM_CHAR_CLIENTS 12                  /* Number of client programmes on one node. */

#define MAX_MINOR (SLSI_UDI_MINOR_NODES - 1) /* Maximum node number. */
static dev_t        major_number;            /* Major number of device created by system. */
static struct class *class;                  /* Device class. */

struct slsi_cdev_client;

struct slsi_cdev {
	int                     minor;
	struct cdev             cdev;
	struct slsi_cdev_client *client[NUM_CHAR_CLIENTS];

	struct slsi_dev         *sdev;
	struct device           *parent;
};

struct udi_signal_header {
        __le16 id;
        __le16 receiver_pid;
        __le16 sender_pid;
        __le32 fw_reference;
        __le16 vif;
} __packed;

struct slsi_cdev_client {
	struct slsi_cdev    *ufcdev;
	int                 log_enabled;
	int                 log_allow_driver_signals;

	u16                 tx_sender_id;
	struct slsi_fw_test fw_test;

	/* Flags set for special filtering of ma_packet data */
	u16                 ma_unitdata_filter_config;

	u16					ma_unitdata_size_limit;

	struct sk_buff_head log_list;
	struct semaphore    log_mutex;
	wait_queue_head_t   log_wq;

	/* Drop Frames and report the number dropped */
#define UDI_MAX_QUEUED_FRAMES 10000
#define UDI_RESTART_QUEUED_FRAMES 9000

#define UDI_MAX_QUEUED_DATA_FRAMES 9000
#define UDI_RESTART_QUEUED_DATA_FRAMES 8000

	/* Start dropping ALL frames at    queue_len == UDI_MAX_QUEUED_FRAMES
	 * Restart queueing ALL frames at  queue_len == UDI_RESTART_QUEUED_FRAMES
	 * Enable MA_PACKET filters at     queue_len == UDI_MAX_QUEUED_DATA_FRAMES
	 * Disable MA_PACKET filters at    queue_len == UDI_RESTART_QUEUED_DATA_FRAMES
	 */
	u32  log_dropped;
	u32  log_dropped_data;
	bool log_drop_data_packets;
};

static inline struct sk_buff *udi_signal_alloc_skb(size_t sig_size, size_t data_size, u16 id, u16 vif, const char *file, int line)
{
	struct sk_buff                *skb = alloc_skb(sig_size + data_size, GFP_ATOMIC);
	struct udi_signal_header      *header;

	WLBT_WARN_ON(sig_size < sizeof(struct udi_signal_header));
	if (WLBT_WARN_ON(!skb))
		return NULL;

	slsi_skb_cb_init(skb)->sig_length = sig_size;
	slsi_skb_cb_get(skb)->data_length = sig_size;

	header = (struct udi_signal_header *)skb_put(skb, sig_size);
	header->id = cpu_to_le16(id);
	header->receiver_pid = 0;
	header->sender_pid = 0;
	header->fw_reference = 0;
	header->vif = vif;
	return skb;
}

static inline bool slsi_cdev_unitdata_filter_allow(struct slsi_cdev_client *client, u16 filter)
{
	return (client->ma_unitdata_filter_config & filter) == filter;
}

/* One minor node per phy. In normal driver mode, this may be one.
 * In unit test mode, this may be several.
 */
static struct slsi_cdev *uf_cdevs[SLSI_UDI_MINOR_NODES];

static int udi_log_event(struct slsi_log_client *log_client, struct sk_buff *skb, int dir);
static int send_signal_to_log_filter(struct slsi_log_client *log_client, struct sk_buff *skb, int dir);
static int send_signal_to_inverse_log_filter(struct slsi_log_client *log_client, struct sk_buff *skb, int dir);

int slsi_check_cdev_refs(void)
{
	int              client_num;
	int              cdev_num;
	struct slsi_cdev *cdev = NULL;

	for (cdev_num = 0; cdev_num < SLSI_UDI_MINOR_NODES; cdev_num++) {
		cdev = uf_cdevs[cdev_num];

		if (!cdev)
			continue;

		for (client_num = 0; client_num < NUM_CHAR_CLIENTS; client_num++)
			if (cdev->client[client_num])
				return 1;
	}

	return 0;
}

int slsi_kernel_to_user_space_event(struct slsi_log_client *log_client, u16 event, u32 data_length, const u8 *data)
{
	struct slsi_cdev_client *client = log_client->log_client_ctx;
	struct sk_buff          *skb;
	int                     ret;

	if (WLBT_WARN_ON(!client))
		return -EINVAL;

	if (!client->log_allow_driver_signals)
		return 0;

	skb = udi_signal_alloc_skb(sizeof(struct udi_signal_header), data_length, event, 0, __FILE__, __LINE__);
	if (WLBT_WARN_ON(!skb))
		return -ENOMEM;

	if (data_length)
		fapi_append_data(skb, data, data_length);

	ret = udi_log_event(log_client, skb, UDI_CONFIG_IND);
	if (ret)
		SLSI_WARN_NODEV("Udi log event not registered\n");

	/* udi_log_event takes a copy, so ensure that the skb allocated in this
	 * function is freed again.
	 */
	kfree_skb(skb);
	return ret;
}

static int slsi_cdev_open(struct inode *inode, struct file *file)
{
	struct slsi_cdev        *uf_cdev;
	struct slsi_cdev_client *client;
	int                     indx;
	int                     minor;

	minor = iminor(inode);
	if (minor > MAX_MINOR) {
		SLSI_ERR_NODEV("minor %d exceeds range\n", minor);
		return -EINVAL;
	}

	uf_cdev = uf_cdevs[minor];
	if (!uf_cdev) {
		SLSI_ERR_NODEV("no cdev instance for minor %d\n", minor);
		return -EINVAL;
	}

	for (indx = 0; indx < NUM_CHAR_CLIENTS; indx++)
		if (!uf_cdev->client[indx])
			break;
	if (indx >= NUM_CHAR_CLIENTS) {
		SLSI_ERR_NODEV("already opened\n");
		return -ENOTSUPP;
	}

	client = kmalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		return -ENOMEM;
	memset(client, 0, sizeof(struct slsi_cdev_client));

	/* init other resource */
	skb_queue_head_init(&client->log_list);
	init_waitqueue_head(&client->log_wq);
	sema_init(&client->log_mutex, 1);
	client->tx_sender_id = SLSI_TX_PROCESS_ID_UDI_MIN;
	slsi_fw_test_init(uf_cdev->sdev, &client->fw_test);

	client->ufcdev = uf_cdev;
	uf_cdev->client[indx] = client;
	file->private_data = client;
	slsi_procfs_inc_node();

#if IS_ENABLED(CONFIG_SCSC_MXLOGGER)
	scsc_service_register_observer(NULL, "udi");
#endif

	SLSI_DBG1_NODEV(SLSI_UDI, "Client:%d added\n", indx);

	return 0;
}

static int slsi_cdev_release(struct inode *inode, struct file *filp)
{
	struct slsi_cdev_client *client = (void *)filp->private_data;
	struct slsi_cdev        *uf_cdev;
	int                     indx;
	int                     minor;

	minor = iminor(inode);
	if (minor > MAX_MINOR) {
		SLSI_ERR_NODEV("minor %d exceeds range\n", minor);
		return -EINVAL;
	}

	uf_cdev = uf_cdevs[minor];
	if (!uf_cdev) {
		SLSI_ERR_NODEV("no cdev instance for minor %d\n", minor);
		return -EINVAL;
	}

	if (!client)
		return -EINVAL;

	for (indx = 0; indx < NUM_CHAR_CLIENTS; indx++)
		if (uf_cdev->client[indx] == client)
			break;
	if (indx >= NUM_CHAR_CLIENTS) {
		SLSI_ERR_NODEV("client not found in list\n");
		return -EINVAL;
	}

	if (waitqueue_active(&client->log_wq))
		wake_up_interruptible(&client->log_wq);

	if (client->log_enabled)
		slsi_log_client_unregister(client->ufcdev->sdev, client);

	skb_queue_purge(&client->log_list);

	slsi_fw_test_deinit(uf_cdev->sdev, &client->fw_test);
	uf_cdev->client[indx] = NULL;

	/* free other resource */
	kfree(client);
	slsi_procfs_dec_node();

#if IS_ENABLED(CONFIG_SCSC_MXLOGGER)
	scsc_service_unregister_observer(NULL, "udi");
#endif

	SLSI_DBG1_NODEV(SLSI_UDI, "Client:%d removed\n", indx);

	return 0;
}

static ssize_t slsi_cdev_read(struct file *filp, char *p, size_t len, loff_t *poff)
{
	struct slsi_cdev_client *client = (void *)filp->private_data;
	struct slsi_dev         *sdev;
	int                     msglen;
	struct sk_buff          *skb;

	SLSI_UNUSED_PARAMETER(poff);

	if (!client)
		return -EINVAL;

	if (!skb_queue_len(&client->log_list)) {
		if (filp->f_flags & O_NONBLOCK)
			return 0;

		/* wait until getting a signal */
		if (wait_event_interruptible(client->log_wq, skb_queue_len(&client->log_list))) {
			SLSI_ERR_NODEV("slsi_cdev_read: wait_event_interruptible failed.\n");
			return -ERESTARTSYS;
		}
	}

	sdev = client->ufcdev->sdev;
	if (!sdev) {
		SLSI_ERR_NODEV("sdev not set\n");
		return -EINVAL;
	}

	skb = skb_dequeue(&client->log_list);
	if (!skb) {
		SLSI_ERR(sdev, "No Data\n");
		return -EINVAL;
	}

	slsi_fw_test_signal_with_udi_header(sdev, &client->fw_test, skb);

	msglen = skb->len;
	if (msglen > (s32)len) {
		SLSI_WARN(sdev, "truncated read to %d actual msg len is %lu\n", msglen, (unsigned long int)len);
		msglen = len;
	}

	if (copy_to_user(p, skb->data, msglen)) {
		SLSI_ERR(sdev, "Failed to copy UDI log to user\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	kfree_skb(skb);
	return msglen;
}

static ssize_t slsi_cdev_write(struct file *filp, const char *p, size_t len, loff_t *poff)
{
	struct slsi_cdev_client *client;
	struct slsi_dev         *sdev;
	struct sk_buff          *skb;
	u8                      *data;
	struct slsi_skb_cb	*cb;
	u16 sig_len;

	SLSI_UNUSED_PARAMETER(poff);

	client = (void *)filp->private_data;
	if (!client) {
		SLSI_ERR_NODEV("filep private data not set\n");
		return -EINVAL;
	}

	if (!client->ufcdev) {
		SLSI_ERR_NODEV("ufcdev not set\n");
		return -EINVAL;
	}

	sdev = client->ufcdev->sdev;
	if (!sdev) {
		SLSI_ERR_NODEV("sdev not set\n");
		return -EINVAL;
	}
	skb = alloc_skb(SLSI_NETIF_SKB_HEADROOM + SLSI_NETIF_SKB_TAILROOM + len, GFP_KERNEL);
	if (!skb) {
		SLSI_WARN_NODEV("error allocating skb (len: %d)\n", len);
		return -ENOMEM;
	}

	skb_reserve(skb, SLSI_NETIF_SKB_HEADROOM - SLSI_SKB_GET_ALIGNMENT_OFFSET(skb));
	data = skb_put(skb, len);
	if (copy_from_user(data, p, len)) {
		SLSI_ERR(sdev, "copy from user failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cb = slsi_skb_cb_init(skb);

	/* If first two octets of fw_ref are non-0, use that for signal length
	 * else use the look up table for signal length
	*/
	sig_len = (u16)fapi_get_fwref(skb);
	if (sig_len)
		cb->sig_length = sig_len;
	else
		cb->sig_length = fapi_get_expected_size(skb);

	cb->data_length = skb->len;

	/* F/w will panic if fw_reference is not zero. */
	fapi_set_u32(skb, fw_reference, 0);
	/* set mac header uses values from above initialized cb */
	skb_set_mac_header(skb, fapi_get_data(skb) - skb->data);

	SLSI_DBG3_NODEV(SLSI_UDI,
			"UDI Signal:%.4X  SigLEN:%d  DataLen:%d  SKBHeadroom:%d  bytes:%d\n",
			fapi_get_sigid(skb), fapi_get_siglen(skb),
			fapi_get_datalen(skb), skb_headroom(skb), (int)len);

	/* In WlanLite test mode req signals IDs are 0x1000, 0x1002, 0x1004 */
	if (slsi_is_test_mode_enabled() || fapi_is_req(skb) || fapi_is_res(skb)) {
#if defined(CONFIG_SCSC_PCIE_CHIP)
		if (scsc_mx_service_claim(WLAN_UDI)) {
			SLSI_ERR(sdev, "Failed to get the PCIe link\n");
			kfree_skb(skb);
			return -EFAULT;
		}
#endif
		/* Use the range of PIDs allocated to the udi clients */
		client->tx_sender_id++;
		if (client->tx_sender_id > SLSI_TX_PROCESS_ID_UDI_MAX)
			client->tx_sender_id = SLSI_TX_PROCESS_ID_UDI_MIN;

		fapi_set_u16(skb, sender_pid, client->tx_sender_id);
		if (!slsi_is_test_mode_enabled())
			slsi_fw_test_signal(sdev, &client->fw_test, skb);
		if (fapi_is_ma(skb)) {
			if (slsi_tx_data_lower(sdev, skb)) {
				kfree_skb(skb);
#if defined(CONFIG_SCSC_PCIE_CHIP)
				scsc_mx_service_release(WLAN_UDI);
#endif
				return -EINVAL;
			}
		} else if (slsi_tx_control(sdev, NULL, skb)) {
			kfree_skb(skb);
#if defined(CONFIG_SCSC_PCIE_CHIP)
			scsc_mx_service_release(WLAN_UDI);
#endif
			return -EINVAL;
		}
#if defined(CONFIG_SCSC_PCIE_CHIP)
		scsc_mx_service_release(WLAN_UDI);
#endif
	} else if (slsi_hip_rx(sdev, skb)) {
		kfree_skb(skb);
		return -EINVAL;
	}

	return len;
}

static u16 slsi_get_vifidx_frm_linkid(struct net_device *dev, u16 link_id)
{
	struct netdev_vif *ndev_vif = NULL;
	u16 vif_idx = 0;

	if (!dev)
		goto exit;

	ndev_vif = netdev_priv(dev);
	vif_idx = ndev_vif->vifnum;

#ifdef CONFIG_SCSC_WLAN_EHT
	if (link_id >= 15)
		goto exit;

	if ((ndev_vif->sta.valid_links & BIT(link_id)) &&
	    (ndev_vif->sta.links[link_id].mlo_vif_idx))
		vif_idx = ndev_vif->sta.links[link_id].mlo_vif_idx;

#endif

exit:
	SLSI_DBG3_NODEV(SLSI_CFG80211, "vif_idx %d link_id %d\n", vif_idx, link_id);

	return vif_idx;
}

static long slsi_unifi_set_mib(struct slsi_dev *sdev, unsigned long arg)
{
	struct net_device *dev = NULL;
	long              r = 0;
	u32               mib_data_length; /* Length of valid Mib data in the buffer */
	u32               mib_data_size;   /* Size of the mib buffer  */
	unsigned char     *mib_data;       /* Mib Input/Output Buffer */
	u16               mib_vif;
	u16               ifnum;
	u16               vif;
	u16               link_id = 15;
	u8                link_siz = 0;

	if (sdev->device_state != SLSI_DEVICE_STATE_STARTED) {
		SLSI_ERR(sdev, "UNIFI_SET_MIB: Device not yet available\n");
		return -EFAULT;
	}
	/* First 2 Bytes are the VIF */
	if (copy_from_user((void *)&mib_vif, (void *)arg, 2)) {
		SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed to copy in vif\n");
		return -EFAULT;
	}

	/* Next 2 Bytes are the Link Id if ioctl_mib_linkid_supported is true */
	if (slsi_ioctl_mib_linkid_supported()) {
		if (copy_from_user((void *)&link_id, (void *)(arg + 2), 2)) {
			SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed to copy link_id\n");
			return -EFAULT;
		}
		link_siz = 2;
	}

	/* First 4 Bytes are the Number of Bytes of input Data */
	if (copy_from_user((void *)&mib_data_length, (void *)(arg + 2 + link_siz), 4)) {
		SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed to copy in mib_data_length\n");
		return -EFAULT;
	}
	/* Second 4 Bytes are the size of the Buffer */
	if (copy_from_user((void *)&mib_data_size, (void *)(arg + 6 + link_siz), 4)) {
		SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed to copy in mib_data_size\n");
		return -EFAULT;
	}
	/* check if length is valid */
	if (unlikely(mib_data_length > UDI_MIB_SET_LEN_MAX || mib_data_size > UDI_MIB_SET_LEN_MAX)) {
		SLSI_ERR(sdev, "UNIFI_SET_MIB: size too long (mib_data_length:%u mib_data_size:%u)\n", mib_data_length, mib_data_size);
		return -EFAULT;
	}

	mib_data = kmalloc(mib_data_size, GFP_KERNEL);
	if (!mib_data) {
		SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed to allocate memory for mib_data\n");
		return -ENOMEM;
	}
	/* Read the rest of the Mib Data */
	if (copy_from_user((void *)mib_data, (void *)(arg + 10 + link_siz), mib_data_length)) {
		SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed to copy in mib_data\n");
		kfree(mib_data);
		return -EFAULT;
	}

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	if (mib_vif) {
		ifnum = slsi_get_ifnum_by_vifid(sdev, mib_vif);
		dev = slsi_get_netdev_locked(sdev, ifnum);
		if (!dev) {
			SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed - net_device is NULL for interface = %d ifnum %d\n",
				mib_vif, ifnum);
			kfree(mib_data);
			SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
			return -EFAULT;
		}
	}

	vif = slsi_get_vifidx_frm_linkid(dev, link_id);

	r = slsi_mlme_set_with_vifnum(sdev, dev, mib_data, mib_data_length, vif);
	if (r != 0)
		SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed - mlme_set_req : Result code = %d\n", r);

	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
	kfree(mib_data);

	return r;
}

static long slsi_unifi_get_mib(struct slsi_dev *sdev, unsigned long arg)
{
	struct net_device *dev = NULL;
	struct netdev_vif *ndev_vif = NULL;
	long              r = 0;
	u32               mib_data_length; /* Length of valid Mib data in the buffer */
	u32               mib_data_size;   /* Size of the mib buffer  */
	unsigned char     *mib_data;       /* Mib Input/Output Buffer */
	u16               mib_vif;
	u16               ifnum;
	u16               link_id = 15;
	u16               vif = 0;
	u8                link_siz = 0;

	if (sdev->device_state != SLSI_DEVICE_STATE_STARTED) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Device not yet available\n");
		return -EFAULT;
	}

	/* First 2 Bytes are the VIF */
	if (copy_from_user((void *)&mib_vif, (void *)arg, 2)) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed to copy in vif\n");
		return -EFAULT;
	}

	/* Next 2 Bytes are the Link Id if ioctl_mib_linkid_supported is true */
	if (slsi_ioctl_mib_linkid_supported()) {
		if (copy_from_user((void *)&link_id, (void *)(arg + link_siz + 2), 2)) {
			SLSI_ERR(sdev, "UNIFI_SET_MIB: Failed to copy link_id\n");
			return -EFAULT;
		}
		link_siz = 2;
	}

	/* First 4 Bytes are the Number of Bytes of input Data */
	if (copy_from_user((void *)&mib_data_length, (void *)(arg + link_siz + 2), 4)) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed to copy in mib_data_length\n");
		return -EFAULT;
	}

	/* Second 4 Bytes are the size of the Buffer */
	if (copy_from_user((void *)&mib_data_size, (void *)(arg + link_siz + 6), 4)) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed to copy in mib_data_size\n");
		return -EFAULT;
	}

	/* check if length is valid */
	if (unlikely(mib_data_length > UDI_MIB_GET_LEN_MAX || mib_data_size > UDI_MIB_GET_LEN_MAX)) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: size too long (mib_data_length:%u mib_data_size:%u)\n", mib_data_length, mib_data_size);
		return -EFAULT;
	}

	mib_data = kmalloc(mib_data_size, GFP_KERNEL);
	if (!mib_data) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed to allocate memory for mib_data\n");
		return -ENOMEM;
	}
	/* Read the rest of the Mib Data */
	if (copy_from_user((void *)mib_data, (void *)(arg + link_siz + 10), mib_data_length)) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed to copy in mib_data\n");
		kfree(mib_data);
		return -EFAULT;
	}

	if (mib_vif) {
		ifnum = slsi_get_ifnum_by_vifid(sdev, mib_vif);
		dev = slsi_get_netdev(sdev, ifnum);
		if (!dev) {
			SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed - net_device is NULL for interface = %d, vifnum:%d\n",
				ifnum, mib_vif);
			kfree(mib_data);
			return -EFAULT;
		}
	}

	vif = slsi_get_vifidx_frm_linkid(dev, link_id);

	if (dev) {
		ndev_vif = netdev_priv(dev);
		SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	}

	r = slsi_mlme_get_with_vifidx(sdev, dev, mib_data, mib_data_length, mib_data, mib_data_size, &mib_data_length,
				      vif);

	if (dev && ndev_vif)
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	if (r != 0) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed - mlme_get_req : Result code = %d\n", r);
		kfree(mib_data);
		return -EINVAL;
	}

	/* Check the buffer is big enough */
	if (mib_data_length > mib_data_size) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Mib result data is to long. (%d bytes when the max is %d bytes)\n", mib_data_length, mib_data_size);
		kfree(mib_data);
		return -EINVAL;
	}

	/* Copy back the number of Bytes in the Mib result */
	if (copy_to_user((void *)arg, (void *)&mib_data_length, 4)) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed to copy in mib_data_length back to user\n");
		kfree(mib_data);
		return -EFAULT;
	}

	/* Copy back the Mib data */
	if (copy_to_user((void *)(arg + 4), mib_data, mib_data_length)) {
		SLSI_ERR(sdev, "UNIFI_GET_MIB: Failed to copy in mib_data back to user\n");
		kfree(mib_data);
		return -EFAULT;
	}
	kfree(mib_data);
	return 0;
}

static long slsi_unifi_set_log_mask(struct slsi_cdev_client *client, struct slsi_dev *sdev, unsigned long arg)
{
	struct unifiio_filter_t filter;
	int                     i;

	/* to minimise load on data path, list is converted here to array indexed by signal number */
	if (copy_from_user(&filter, (void *)arg, sizeof(filter))) {
		SLSI_ERR(sdev, "UNIFI_SET_UDI_LOG_MASK: Failed to copy from userspace\n");
		return -EFAULT;
	}

	if (unlikely(filter.signal_ids_n > UDI_LOG_MASK_FILTER_NUM_MAX)) {
		SLSI_ERR(sdev, "UNIFI_SET_UDI_LOG_MASK: number of filters too long\n");
		return -EFAULT;
	}

	if (filter.signal_ids_n) {
		char *signal_filter_index;
		int  max;
		int  min;

		max = filter.signal_ids[0];
		min = filter.signal_ids[0];

		/* find maximum and minimum signal id in filter */
		for (i = 0; i < filter.signal_ids_n; i++) {
			if (filter.signal_ids[i] & UDI_MA_UNITDATA_FILTER_ALLOW_MASK) {
				client->ma_unitdata_filter_config |= filter.signal_ids[i];
				continue;
			}
			if (filter.signal_ids[i] > max)
				max = filter.signal_ids[i];
			else if (filter.signal_ids[i] < min)
				min = filter.signal_ids[i];
		}
		/* and create array only big enough to index the range of signal id specified */
		signal_filter_index = kmalloc(max - min + 1, GFP_KERNEL);
		if (signal_filter_index) {
			memset(signal_filter_index, 0, max - min + 1);
			for (i = 0; i < filter.signal_ids_n; i++) {
				if (filter.signal_ids[i] & UDI_MA_UNITDATA_FILTER_ALLOW_MASK)
					continue;
				signal_filter_index[filter.signal_ids[i] - min] = 1;
			}
			slsi_log_client_unregister(sdev, client);
			slsi_log_client_register(sdev, client,
						 filter.log_listed_flag ? send_signal_to_inverse_log_filter :
						 send_signal_to_log_filter, signal_filter_index, min, max);
		} else {
			return -ENOMEM;
		}
	}
	return 0;
}

static long slsi_cdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct slsi_cdev_client *client = (void *)filp->private_data;
	struct slsi_dev         *sdev;
	long                    r = 0;
	int                     int_param;

	if (!client || !client->ufcdev)
		return -EINVAL;
	sdev = client->ufcdev->sdev;

	slsi_wake_lock(&sdev->wlan_wl);

	switch (cmd) {
	case UNIFI_GET_UDI_ENABLE:
		int_param = client->log_enabled;
		put_user(int_param, (int *)arg);
		break;

	case UNIFI_SET_UDI_ENABLE:
		if (get_user(int_param, (int *)arg)) {
			r = -EFAULT;
			break;
		}

		if (int_param) {
			slsi_log_client_register(sdev, client, udi_log_event, NULL, 0, 0);
			client->log_enabled = 1;
			if (int_param > 1)
				client->log_allow_driver_signals = 1;
		} else {
			slsi_log_client_unregister(sdev, client);
			client->log_enabled = 0;
		}

		break;

	case UNIFI_SET_UDI_LOG_CONFIG:
	{
		struct unifiio_udi_config_t config;

		if (copy_from_user(&config, (void *)arg, sizeof(config))) {
			SLSI_ERR(sdev, "UNIFI_SET_UDI_LOG_CONFIG: Failed to copy from userspace\n");
			r = -EFAULT;
			break;
		}

		client->ma_unitdata_size_limit = config.ma_unitdata_size_limit;
		break;
	}
	case UNIFI_SET_UDI_LOG_MASK:
		r = slsi_unifi_set_log_mask(client, sdev, arg);
		break;

	case UNIFI_SET_MIB:
		r = slsi_unifi_set_mib(sdev, arg);
		break;

	case UNIFI_GET_MIB:
		r = slsi_unifi_get_mib(sdev, arg);
		break;

	case UNIFI_SRC_SINK_IOCTL:
		if (sdev->device_state != SLSI_DEVICE_STATE_STARTED) {
			SLSI_ERR(sdev, "UNIFI_SRC_SINK_IOCTL: Device not yet available\n");
			r = -EFAULT;
			break;
		}
		r = slsi_src_sink_cdev_ioctl_cfg(sdev, arg);
		break;

	case UNIFI_SOFTMAC_CFG:
	{
		u32 softmac_cmd;
		u8  cmd_param_size;

		SLSI_ERR(sdev, "UNIFI_SOFTMAC_CFG\n");

		if (copy_from_user((void *)&softmac_cmd, (void *)arg, 4)) {
			SLSI_ERR(sdev, "Failed to get the command\n");
			r = -EFAULT;
			break;
		}
		SLSI_DBG3_NODEV(SLSI_UDI, "softmac_cmd -> %u\n", softmac_cmd);

		arg += sizeof(softmac_cmd); /* Advance past the command bit */
		if (copy_from_user((void *)&cmd_param_size, (void *)(arg + 4), 1)) {
			SLSI_ERR(sdev, "Failed to get the command size\n");
			r = -EFAULT;
			break;
		}
		SLSI_DBG3_NODEV(SLSI_UDI, "cmd_param_size -> %u\n", cmd_param_size);

		if (cmd_param_size)
			client->ma_unitdata_filter_config = UDI_MA_UNITDATA_FILTER_ALLOW_EAPOL_ID;
		else
			client->ma_unitdata_filter_config = 0;
		break;
	}
	case UNIFI_GET_SW_VERSION:
	{
		int res = 0;
		char build_info[200];
		int len = 0;
#ifndef SLSI_TEST_DEV
		memset(build_info, 0, 200);
		mxman_get_fw_version(build_info, 200);
		len = strlen(build_info);
		sprintf(build_info + len, " ");
		if (len >= 197) {
			len = 197;
			build_info[len - 1] = '.';
			build_info[len - 2] = '.';
			build_info[len - 3] = '.';
		} else {
			mxman_get_driver_version(build_info + len + 1, 200 - len - 1);
		}
#else
		memset(build_info, 0, 200);
		strcpy(build_info, "UT Build");
#endif
		/* Copy back the number of Bytes in the version result */
		res = copy_to_user((void *)arg, build_info, 200);
		if (res) {
			SLSI_ERR(sdev, "UNIFI_GET_SW_VERSION: error back to user %d\n", res);
			r = -EINVAL;
			break;
		}
		break;
	}
	case UNIFI_GET_FAPI_VERSION:
	{
		int res = 0;
		char fapi_info[100];

		memset(fapi_info, 0, 100);
		slsi_get_fapi_version_string(fapi_info);
		/* Copy back the number of Bytes in the version result */
		res = copy_to_user((void *)arg, fapi_info, 100);
		if (res) {
			SLSI_ERR(sdev, "UNIFI_GET_FAPI_VERSION: error back to user %d\n", res);
			r = -EINVAL;
			break;
		}
		break;
	}
	default:
		SLSI_WARN(sdev, "Operation (%d) not supported\n", cmd);
		r = -EINVAL;
	}

	slsi_wake_unlock(&sdev->wlan_wl);
	return r;
}

static unsigned int slsi_cdev_poll(struct file *filp, poll_table *wait)
{
	struct slsi_cdev_client *client = (void *)filp->private_data;

	SLSI_DBG4_NODEV(SLSI_UDI, "Poll(%d)\n", skb_queue_len(&client->log_list));

	if (skb_queue_len(&client->log_list))
		return POLLIN | POLLRDNORM;   /* readable */

	poll_wait(filp, &client->log_wq, wait);

	if (skb_queue_len(&client->log_list))
		return POLLIN | POLLRDNORM;     /* readable */

	return 0;
}

/* we know for sure that there is a filter present in log_client->signal_filter if this function is called.
 * we know this because it is called only through a function pointer that is assigned
 * only when a filter is also set up in the log_client
 */
static int send_signal_to_log_filter(struct slsi_log_client *log_client, struct sk_buff *skb, int dir)
{
	int ret = 0;
	u16 signal_id =  fapi_get_sigid(skb);

	if (signal_id > log_client->max_signal_id || signal_id < log_client->min_signal_id || !log_client->signal_filter[signal_id - log_client->min_signal_id])
		ret = udi_log_event(log_client, skb, dir);

	return ret;
}

static int send_signal_to_inverse_log_filter(struct slsi_log_client *log_client, struct sk_buff *skb, int dir)
{
	int ret = 0;
	u16 signal_id =  fapi_get_sigid(skb);

	if (signal_id <= log_client->max_signal_id && signal_id >= log_client->min_signal_id && log_client->signal_filter[signal_id - log_client->min_signal_id])
		ret = udi_log_event(log_client, skb, dir);

	return ret;
}

static int udi_log_event(struct slsi_log_client *log_client, struct sk_buff *skb, int dir)
{

	struct slsi_cdev_client *client = log_client->log_client_ctx;
	struct udi_msg_t        msg;
	struct udi_msg_t        *msg_skb;
	u16                     signal_id = fapi_get_sigid(skb);

	if (WLBT_WARN_ON(!client))
		return -EINVAL;
	if (WLBT_WARN_ON(!skb))
		return -EINVAL;
	if (WLBT_WARN_ON(skb->len == 0))
		return -EINVAL;

	/* Special Filtering of MaPacket frames */
	if (slsi_cdev_unitdata_filter_allow(client, UDI_MA_UNITDATA_FILTER_ALLOW_MASK) &&
	    (signal_id == MA_UNITDATA_REQ || signal_id == MA_UNITDATA_IND)) {
		SLSI_DBG4_NODEV(SLSI_UDI, "FILTER(0x%.4X) DROP\n", signal_id);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		if (down_trylock(&client->log_mutex)) {
#else
		if (down_interruptible(&client->log_mutex)) {
#endif
			SLSI_WARN_NODEV("Failed to get udi sem\n");
			return -ERESTARTSYS;
		}
		if (client->log_drop_data_packets)
			client->log_dropped_data++;
		up(&client->log_mutex);
		return -ECANCELED;
	}

	/* Exception for driver configuration frames.
	 * these frames must be sent irrespective of number of frames
	 * in queue.
	 */
	if (dir == UDI_CONFIG_IND)
		goto allow_config_frame;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	if (down_trylock(&client->log_mutex)) {
#else
	if (down_interruptible(&client->log_mutex)) {
#endif
		SLSI_WARN_NODEV("Failed to get udi sem\n");
		return -ERESTARTSYS;
	}

	/* Handle hitting the UDI_MAX_QUEUED_FRAMES Limit */
	if (client->log_dropped) {
		if (skb_queue_len(&client->log_list) <= UDI_RESTART_QUEUED_FRAMES) {
			u32 dropped = client->log_dropped;

			SLSI_WARN_NODEV("Stop Dropping UDI Frames : %d frames Dropped\n", dropped);
			client->log_dropped = 0;
			up(&client->log_mutex);
			slsi_kernel_to_user_space_event(log_client, UDI_DRV_DROPPED_FRAMES, sizeof(u32), (u8 *)&dropped);
			return -ECANCELED;
		}
		client->log_dropped++;
		up(&client->log_mutex);
		return -ECANCELED;
	} else if (!client->log_dropped && skb_queue_len(&client->log_list) >= UDI_MAX_QUEUED_FRAMES) {
		SLSI_WARN_NODEV("Start Dropping UDI Frames\n");
		client->log_dropped++;
		up(&client->log_mutex);
		return -ECANCELED;
	}

	/* Handle hitting the UDI_MAX_QUEUED_DATA_FRAMES Limit
	 * Turn ON the MA_PACKET Filters before we get near the absolute limit of UDI_MAX_QUEUED_FRAMES
	 * This should allow key frames (mgt, dhcp and eapol etc) to still be in the logs but stop the logging general data frames.
	 * This occurs when the Transfer rate is higher than we can take the frames out of the UDI list.
	 */
	if (client->log_drop_data_packets && skb_queue_len(&client->log_list) < UDI_RESTART_QUEUED_DATA_FRAMES) {
		u32 dropped = client->log_dropped_data;

		SLSI_WARN_NODEV("Stop Dropping UDI Frames : %d Basic Data frames Dropped\n", client->log_dropped_data);
		client->log_drop_data_packets = false;
		client->ma_unitdata_filter_config = 0;
		client->log_dropped_data = 0;
		up(&client->log_mutex);
		slsi_kernel_to_user_space_event(log_client, UDI_DRV_DROPPED_DATA_FRAMES, sizeof(u32), (u8 *)&dropped);
		return -ECANCELED;
	} else if (!client->log_drop_data_packets && skb_queue_len(&client->log_list) >= UDI_MAX_QUEUED_DATA_FRAMES && !slsi_cdev_unitdata_filter_allow(client, UDI_MA_UNITDATA_FILTER_ALLOW_MASK)) {
		SLSI_WARN_NODEV("Start Dropping UDI Basic Data Frames\n");
		client->log_drop_data_packets = true;
		client->ma_unitdata_filter_config = UDI_MA_UNITDATA_FILTER_ALLOW_MGT_ID |
						    UDI_MA_UNITDATA_FILTER_ALLOW_KEY_ID |
						    UDI_MA_UNITDATA_FILTER_ALLOW_CFM_ERROR_ID |
						    UDI_MA_UNITDATA_FILTER_ALLOW_EAPOL_ID;
	}
	up(&client->log_mutex);

allow_config_frame:
	if ((signal_id == MA_UNITDATA_REQ || signal_id == MA_UNITDATA_IND) &&
		(client->ma_unitdata_size_limit) && (skb->len > client->ma_unitdata_size_limit)) {
		struct slsi_skb_cb  *cb;
		struct sk_buff *skb2 = alloc_skb(sizeof(msg) + client->ma_unitdata_size_limit, GFP_ATOMIC);

		if (WLBT_WARN_ON(!skb2))
			return -ENOMEM;

		skb_reserve(skb2, sizeof(msg));
		cb = slsi_skb_cb_init(skb2);
		cb->sig_length = fapi_get_siglen(skb);
		cb->data_length = client->ma_unitdata_size_limit;
		skb_copy_bits(skb, 0, skb_put(skb2, client->ma_unitdata_size_limit), client->ma_unitdata_size_limit);
		skb = skb2;
	} else {
		skb = skb_copy_expand(skb, sizeof(msg), 0, GFP_ATOMIC);
		if (WLBT_WARN_ON(!skb))
			return -ENOMEM;
	}

	msg.length = sizeof(msg) + skb->len;
	msg.timestamp = ktime_to_ms(ktime_get());
	msg.direction = dir;
	msg.signal_length = fapi_get_siglen(skb);

	msg_skb = (struct udi_msg_t *)skb_push(skb, sizeof(msg));
	*msg_skb = msg;

	skb_queue_tail(&client->log_list, skb);

	/* Wake any waiting user process */
	wake_up_interruptible(&client->log_wq);
	return 0;
}

#define UF_DEVICE_CREATE(_class, _parent, _devno, _priv, _fmt, _args)       \
	device_create(_class, _parent, _devno, _priv, _fmt, _args)

static const struct file_operations slsi_cdev_fops = {
	.owner          = THIS_MODULE,
	.open           = slsi_cdev_open,
	.release        = slsi_cdev_release,
	.read           = slsi_cdev_read,
	.write          = slsi_cdev_write,
	.unlocked_ioctl = slsi_cdev_ioctl,
	.compat_ioctl   = slsi_cdev_ioctl,
	.poll           = slsi_cdev_poll,
};

#ifndef SLSI_TEST_DEV
static int slsi_get_minor(void)
{
	int minor;

	for (minor = 0; minor < SLSI_UDI_MINOR_NODES; minor++)
		if (!uf_cdevs[minor])
			return minor;
	return -1;
}
#endif

static int slsi_cdev_create(struct slsi_dev *sdev, struct device *parent)
{
	dev_t            devno;
	int              ret;
	struct slsi_cdev *pdev;
	int              minor;

	SLSI_DBG3_NODEV(SLSI_UDI, "\n");
#ifdef SLSI_TEST_DEV
	{
		/* Use the same minor as the unittesthip char device so the number match */
		struct slsi_test_dev *uftestdev = (struct slsi_test_dev *)sdev->maxwell_core;

		minor = uftestdev->device_minor_number;
		if (minor >= 0 && minor < SLSI_UDI_MINOR_NODES && uf_cdevs[minor])
			return -EINVAL;
	}
#else
	minor = slsi_get_minor();
#endif
	if (minor >= SLSI_UDI_MINOR_NODES || minor < 0) {
		SLSI_ERR(sdev, "no minor numbers available,minor:%d\n", minor);
		return -ENOMEM;
	}

	pdev = kmalloc(sizeof(*pdev), GFP_KERNEL);
	if (!pdev)
		return -ENOMEM;
	memset(pdev, 0, sizeof(*pdev));

	cdev_init(&pdev->cdev, &slsi_cdev_fops);
	pdev->cdev.owner = THIS_MODULE;
	pdev->minor = minor;
	devno = MKDEV(MAJOR(major_number), minor);
	ret = cdev_add(&pdev->cdev, devno, 1);
	if (ret) {
		SLSI_ERR(sdev, "cdev_add failed with %d for minor %d\n", ret, minor);
		kfree(pdev);
		return ret;
	}

	pdev->sdev = sdev;
	pdev->parent = parent;
	if (!UF_DEVICE_CREATE(class, pdev->parent, devno, pdev, UDI_CHAR_DEVICE_NAME "%d", minor)) {
		cdev_del(&pdev->cdev);
		kfree(pdev);
		return -EINVAL;
	}
	sdev->uf_cdev = (void *)pdev;
	sdev->procfs_instance = minor;
	uf_cdevs[minor] = pdev;

	return 0;
}

static void slsi_cdev_destroy(struct slsi_dev *sdev)
{
	struct slsi_cdev *pdev = (struct slsi_cdev *)sdev->uf_cdev;
	struct kobject   *kobj;
	struct kref      *kref;

	if (!pdev)
		return;

	SLSI_DBG1(sdev, SLSI_UDI, "\n");
	while (slsi_check_cdev_refs()) {
		SLSI_ERR(sdev, "UDI Client still attached. Please Terminate!\n");
		msleep(1000);
	}

	/* There exist a possibility of race such that the
	 *
	 * - file operation release callback (slsi_cdev_release) is called
	 * - the cdev client structure is freed
	 * - the context is pre-empted and this context (slsi_cdev_destroy) is executed
	 * - slsi_cdev_destroy deletes cdev and hence the kobject embedded inside cdev
	 *   and returns
	 * - the release context again executes and operates on a non-existent kobject
	 *   leading to kernel Panic
	 *
	 * Ideally the kernel should protect against such race. But it is not!
	 * So we check here that the file operation release callback is complete by
	 * checking the refcount in the kobject embedded in cdev structure.
	 * The refcount is initialized to 1; so anything more than that means
	 * there exists attached clients.
	 */

	kobj = &pdev->cdev.kobj;
	kref = &kobj->kref;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
	while (refcount_read(&kref->refcount) > 1) {
		SLSI_WARN(sdev, "UDI client File op release not completed yet! (count=%d)\n", refcount_read(&kref->refcount));
		msleep(50);
	}
#else
	while (atomic_read(&kref->refcount) > 1) {
		SLSI_WARN(sdev, "UDI client File op release not completed yet! (count=%d)\n", atomic_read(&kref->refcount));
		msleep(50);
	}
#endif
	device_destroy(class, pdev->cdev.dev);
	cdev_del(&pdev->cdev);
	sdev->uf_cdev = NULL;
	uf_cdevs[pdev->minor] = NULL;
	kfree(pdev);
}

static int udi_initialised;

int slsi_udi_init(void)
{
	int ret;

	SLSI_DBG1_NODEV(SLSI_UDI, "\n");
	memset(uf_cdevs, 0, sizeof(uf_cdevs));

	/* Allocate two device numbers for each device. */
	ret = alloc_chrdev_region(&major_number, 0, SLSI_UDI_MINOR_NODES, UDI_CLASS_NAME);
	if (ret) {
		SLSI_ERR_NODEV("Failed to add alloc dev numbers: %d\n", ret);
		return ret;
	}

	/* Create a driver class */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	class = class_create(UDI_CLASS_NAME);
#else
	class = class_create(THIS_MODULE, UDI_CLASS_NAME);
#endif
	if (IS_ERR(class)) {
		SLSI_ERR_NODEV("Failed to create driver udi class\n");
		unregister_chrdev_region(major_number, SLSI_UDI_MINOR_NODES);
		major_number = 0;
		return -EINVAL;
	}

	udi_initialised = 1;

	return 0;
}

int slsi_udi_deinit(void)
{
	if (!udi_initialised)
		return -1;
	SLSI_DBG1_NODEV(SLSI_UDI, "\n");
	class_destroy(class);
	unregister_chrdev_region(major_number, SLSI_UDI_MINOR_NODES);
	udi_initialised = 0;
	return 0;
}

int slsi_udi_node_init(struct slsi_dev *sdev, struct device *parent)
{
	return slsi_cdev_create(sdev, parent);
}

int slsi_udi_node_deinit(struct slsi_dev *sdev)
{
	slsi_cdev_destroy(sdev);
	return 0;
}
