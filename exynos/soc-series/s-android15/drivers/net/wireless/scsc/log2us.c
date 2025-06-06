/***************************************************************************
 *
 * Copyright (c) 2014 - 2022 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#include <linux/time.h>
#include <linux/ktime.h>

#include "log2us.h"
#include "dev.h"
#include "debug.h"
#include "mgt.h"
#include "nl80211_vendor.h"
#include "mib.h"
#include "mlme.h"

#define MACSTR_NOMASK "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR_LOG(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

#ifdef CONFIG_SCSC_WLAN_LOG_2_USER_SP
enum slsi_wlan_vendor_attr_conn_log_event {
	SLSI_WLAN_VENDOR_ATTR_CONN_LOG_EVENT = 1,
	SLSI_WLAN_VENDOR_ATTR_CONN_LOG_BUFF_COUNT,
	SLSI_WLAN_VENDOR_ATTR_CONN_LOG_EVENT_MAX
};


static void delete_node(struct slsi_dev *sdev)
{
	struct buff_list *curr = NULL;

	curr = sdev->conn_log2us_ctx.list;
	if (curr) {
		sdev->conn_log2us_ctx.list = sdev->conn_log2us_ctx.list->next;
		kfree(curr);
	}
}

static int slsi_send_conn_log_event(struct slsi_dev *sdev)
{
	struct sk_buff *skb;
	int res;
	u32 buf_count = 0;
	struct buff_list *curr;
	struct nlattr *nla = NULL;
	u32 *count_p;
	struct net_device *dev;
	struct netdev_vif *ndev_vif;

	while (!(__ratelimit(&sdev->conn_log2us_ctx.rs)))
		msleep(50);

	dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	if (!dev) {
		SLSI_WARN_NODEV("net_dev is NULL\n");
		return -EINVAL;
	}

	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION) {
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -1;
	}

	skb = cfg80211_vendor_event_alloc(sdev->wiphy, &ndev_vif->wdev, NLMSG_DEFAULT_SIZE,
					  SLSI_NL80211_VENDOR_CONNECTIVITY_LOG_EVENT,
					  GFP_KERNEL);

	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate SKB for vendor conn_log_event event\n");
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -ENOMEM;
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	nla = nla_reserve(skb, SLSI_WLAN_VENDOR_ATTR_CONN_LOG_BUFF_COUNT, sizeof(buf_count));
	if (!nla) {
		SLSI_ERR_NODEV("failed in nla_reserve\n");
		kfree_skb(skb);
		return -1;
	}

	count_p = nla_data(nla);

	spin_lock(&sdev->conn_log2us_ctx.conn_lock);
	curr = sdev->conn_log2us_ctx.list;

	while (curr && (buf_count < 100)) {
		res = nla_put(skb, SLSI_WLAN_VENDOR_ATTR_CONN_LOG_EVENT, curr->len,
			      curr->str);
		if (res) {
			SLSI_ERR_NODEV("failed in nla_put for buffer no = %d, res = %d\n",
				       buf_count + 1, res);
			break;
		}

		buf_count++;
		delete_node(sdev);
		curr = sdev->conn_log2us_ctx.list;
	}

	spin_unlock(&sdev->conn_log2us_ctx.conn_lock);
	if (buf_count == 0) {
		SLSI_ERR_NODEV("Buffer count is 0\n");
		kfree_skb(skb);
		return -1;
	}

	*count_p = buf_count;
	rtnl_lock();
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rtnl_unlock();

	return 0;
}

static void enqueue_log_buffer_for_roam_cand(struct buff_list *new_node, struct conn_log2us *ctx)
{
	struct buff_list *list;

	list = ctx->list_roam_cand;

	if (!list) {
		ctx->list_roam_cand = new_node;
		return;
	}
	while (list->next != NULL)
		list = list->next;

	list->next = new_node;
}

static void enqueue_log_buffer(struct buff_list *new_node, struct conn_log2us *ctx)
{
	struct buff_list *list;

	spin_lock(&ctx->conn_lock);
	list = ctx->list;

	if (!list) {
		ctx->list = new_node;
		spin_unlock(&ctx->conn_lock);
		return;
	}
	while (list->next != NULL)
		list = list->next;

	list->next = new_node;

	spin_unlock(&ctx->conn_lock);
}

static void conn_log_worker(struct work_struct *work)
{
	struct conn_log2us *p = container_of(work, struct conn_log2us, log2us_work);
	struct slsi_dev *sdev = container_of(p, struct slsi_dev, conn_log2us_ctx);
	int status = 0;

	while (p->list && (!(p->stop))) {
		status = slsi_send_conn_log_event(sdev);
		if (status)
			SLSI_ERR_NODEV("slsi_send_conn_log_event failed status = %d\n", status);
	}
}

void slsi_conn_log2us_init(struct slsi_dev *sdev)
{
	spin_lock_init(&sdev->conn_log2us_ctx.conn_lock);

	sdev->conn_log2us_ctx.log2us_workq = create_workqueue("conn_logger");

	INIT_WORK(&sdev->conn_log2us_ctx.log2us_work, conn_log_worker);

	sdev->conn_log2us_ctx.list = NULL;
	sdev->conn_log2us_ctx.list_roam_cand = NULL;
	ratelimit_state_init(&sdev->conn_log2us_ctx.rs, 1 * HZ, SLSI_LOG2US_BURST);
	sdev->conn_log2us_ctx.stop = false;
}

void slsi_conn_log2us_deinit(struct slsi_dev *sdev)
{
	struct buff_list *tmp;

	sdev->conn_log2us_ctx.stop = true;
	cancel_work_sync(&sdev->conn_log2us_ctx.log2us_work);
	destroy_workqueue(sdev->conn_log2us_ctx.log2us_workq);
	spin_lock(&sdev->conn_log2us_ctx.conn_lock);
	while (sdev->conn_log2us_ctx.list) {
		tmp = sdev->conn_log2us_ctx.list;
		sdev->conn_log2us_ctx.list = sdev->conn_log2us_ctx.list->next;
		kfree(tmp);
	}
	while (sdev->conn_log2us_ctx.list_roam_cand) {
		tmp = sdev->conn_log2us_ctx.list_roam_cand;
		sdev->conn_log2us_ctx.list_roam_cand = sdev->conn_log2us_ctx.list_roam_cand->next;
		kfree(tmp);
	}
	spin_unlock(&sdev->conn_log2us_ctx.conn_lock);
}

#ifdef CONFIG_SCSC_WLAN_EHT
static int slsi_log2us_fill_mlo_band_info(char *log_buffer, int buf_size, int pos, u8 mlo_band)
{
	bool is_2g = false;
	bool is_5g = false;
	bool is_6g = false;

	if (!mlo_band)
		return pos;

	is_2g = SLSI_UC_MAC_2_4_BAND & mlo_band;
	is_5g = SLSI_UC_MAC_5_BAND & mlo_band;
	is_6g = SLSI_UC_MAC_6_BAND & mlo_band;

	if (is_2g && is_5g && is_6g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "band=2G+5G+6G ");
	else if (is_2g && is_5g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "band=2G+5G ");
	else if (is_2g && is_6g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "band=2G+6G ");
	else if (is_5g && is_6g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "band=5G+6G ");
	else if (is_2g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "band=2G ");
	else if (is_5g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "band=5G ");
	else if (is_6g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "band=6G ");

	return pos;
}
#else
static int slsi_log2us_fill_mlo_band_info(char *log_buffer, int buf_size, int pos, u8 mlo_band)
{
	SLSI_UNUSED_PARAMETER(log_buffer);
	SLSI_UNUSED_PARAMETER(buf_size);
	SLSI_UNUSED_PARAMETER(mlo_band);

	return pos;
}
#endif

u8 *get_eap_type_from_val(int val, u8 *str)
{
	switch (val) {
	case SLSI_EAP_TYPE_IDENTITY:
		memcpy(str, "IDENTITY", strlen("IDENTITY") + 1);
		return str;
	case SLSI_EAP_TYPE_NOTIFICATION:
		memcpy(str, "NOTIFICATION", strlen("NOTIFICATION") + 1);
		return str;
	case SLSI_EAP_TYPE_NAK:
		memcpy(str, "NAK", strlen("NAK") + 1);
		return str;
	case SLSI_EAP_TYPE_MD5:
		memcpy(str, "MD5", strlen("MD5") + 1);
		return str;
	case SLSI_EAP_TYPE_OTP:
		memcpy(str, "OTP", strlen("OTP") + 1);
		return str;
	case SLSI_EAP_TYPE_GTC:
		memcpy(str, "GTC", strlen("GTC") + 1);
		return str;
	case SLSI_EAP_TYPE_TLS:
		memcpy(str, "TLS", strlen("TLS") + 1);
		return str;
	case SLSI_EAP_TYPE_LEAP:
		memcpy(str, "LEAP", strlen("LEAP") + 1);
		return str;
	case SLSI_EAP_TYPE_SIM:
		memcpy(str, "SIM", strlen("SIM") + 1);
		return str;
	case SLSI_EAP_TYPE_TTLS:
		memcpy(str, "TTLS", strlen("TTLS") + 1);
		return str;
	case SLSI_EAP_TYPE_AKA:
		memcpy(str, "AKA", strlen("AKA") + 1);
		return str;
	case SLSI_EAP_TYPE_PEAP:
		memcpy(str, "PEAP", strlen("PEAP") + 1);
		return str;
	case SLSI_EAP_TYPE_MSCHAPV2:
		memcpy(str, "MSCHAPV2", strlen("MSCHAPV2") + 1);
		return str;
	case SLSI_EAP_TYPE_TLV:
		memcpy(str, "TLV", strlen("TLV") + 1);
		return str;
	case SLSI_EAP_TYPE_TNC:
		memcpy(str, "TNC", strlen("TNC") + 1);
		return str;
	case SLSI_EAP_TYPE_FAST:
		memcpy(str, "FAST", strlen("FAST") + 1);
		return str;
	case SLSI_EAP_TYPE_PAX:
		memcpy(str, "PAX", strlen("PAX") + 1);
		return str;
	case SLSI_EAP_TYPE_PSK:
		memcpy(str, "PSK", strlen("PSK") + 1);
		return str;
	case SLSI_EAP_TYPE_SAKE:
		memcpy(str, "SAKE", strlen("SAKE") + 1);
		return str;
	case SLSI_EAP_TYPE_IKEV2:
		memcpy(str, "IKEV2", strlen("IKEV2") + 1);
		return str;
	case SLSI_EAP_TYPE_AKA_PRIME:
		memcpy(str, "AKA PRIME", strlen("AKA PRIME") + 1);
		return str;
	case SLSI_EAP_TYPE_GPSK:
		memcpy(str, "GPSK", strlen("GPSK") + 1);
		return str;
	case SLSI_EAP_TYPE_PWD:
		memcpy(str, "PWD", strlen("PWD") + 1);
		return str;
	case SLSI_EAP_TYPE_EKE:
		memcpy(str, "EKE", strlen("EKE") + 1);
		return str;
	case SLSI_EAP_TYPE_EXPANDED:
		memcpy(str, "EXPANDED", strlen("EXPANDED") + 1);
		return str;
	default:
		memcpy(str, "UNKNOWN", strlen("UNKNOWN") + 1);
		return str;
	}
}

static int slsi_log2us_get_rssi(struct slsi_dev *sdev, struct net_device *dev,
				int *mib_value, u16 psid)
{
	struct slsi_mib_data mibrsp = { 0, NULL };
	struct slsi_mib_value     *values = NULL;
	struct slsi_mib_get_entry get_values[] = { { psid, { 0, 0 } } };

	mibrsp.dataLength = 64;
	mibrsp.data = kmalloc(mibrsp.dataLength, GFP_KERNEL);
	if (!mibrsp.data) {
		SLSI_ERR(sdev, "Cannot kmalloc %d bytes\n", mibrsp.dataLength);
		kfree(mibrsp.data);
		return -ENOMEM;
	}

	values = slsi_read_mibs(sdev, dev, get_values, 1, &mibrsp);
	if (!values) {
		kfree(mibrsp.data);
		return -EINVAL;
	}

	if (values[0].type == SLSI_MIB_TYPE_INT)
		*mib_value = (int)(values->u.intValue);
	else if (values[0].type == SLSI_MIB_TYPE_UINT)
		*mib_value = (int)(values->u.uintValue);
	else
		SLSI_ERR(sdev, "Invalid type (%d) for SLSI_PSID_UNIFI_LAST_BSS_RSSI\n",
			 values[0].type);
	kfree(values);
	kfree(mibrsp.data);
	return 0;
}

static struct buff_list *slsi_conn_log2us_alloc_new_node(void)
{

	struct buff_list *new_node;

	new_node = kmalloc(sizeof(struct buff_list), GFP_KERNEL);
	if (!new_node) {
		SLSI_ERR_NODEV("Memory is not available\n");
		return NULL;
	}
	new_node->next = NULL;
	return new_node;
}

static struct buff_list *slsi_conn_log2us_alloc_atomic_new_node(void)
{
	struct buff_list *new_node;

	new_node = kmalloc(sizeof(*new_node), GFP_ATOMIC);
	if (!new_node) {
		SLSI_ERR_NODEV("Memory is not available\n");
		return NULL;
	}
	new_node->next = NULL;
	return new_node;
}

void slsi_conn_log2us_connect_sta_info(struct slsi_dev *sdev, struct net_device *dev)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;

	log_buffer = new_node->str;
	get_kernel_timestamp(time);

#ifndef CONFIG_SCSC_WLAN_EHT
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] STA_INFO mac=" MACSTR_NOMASK,
			 time[0], time[1], MAC2STR_LOG(dev->dev_addr));
#else
	if(slsi_check_ml_ie_from_bcn_ie(dev))
		pos += scnprintf(log_buffer + pos, buf_size - pos,
				 "[%d.%d][CONN] STA_INFO mld_mac= "MACSTR_NOMASK " 2G="
				 MACSTR_NOMASK " 5G=" MACSTR_NOMASK " 6G=" MACSTR_NOMASK,
				 time[0], time[1], MAC2STR_LOG(dev->dev_addr),
				 MAC2STR_LOG((u8 *)ndev_vif->sta.sta_link_addr[0]),
				 MAC2STR_LOG((u8 *)ndev_vif->sta.sta_link_addr[1]),
				 MAC2STR_LOG((u8 *)ndev_vif->sta.sta_link_addr[2]));
	else
		pos += scnprintf(log_buffer + pos, buf_size - pos,
				 "[%d.%d][CONN] STA_INFO mac=" MACSTR_NOMASK,
				 time[0], time[1], MAC2STR_LOG(dev->dev_addr));
#endif

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_connecting(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_connect_params *sme)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	int btcoex = 0;
	const u8 *rsn;
	int rsn_pos = 0;
	u32 group_mgmt = 0;
	int rsn_len = 0;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);
	int pairwise = 0, group = 0, freq = 0, freq_hint = 0, akm_suite = 0;

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;

	log_buffer = new_node->str;
	get_kernel_timestamp(time);
	btcoex = sdev->conn_log2us_ctx.bt_device_status;

	rsn = cfg80211_find_ie(WLAN_EID_RSN, sme->ie, sme->ie_len);

	/* element_id(1) + length(1) + version(2) + Group Data Cipher Suite(4)+ */
	/* Pairwise Cipher Suite Count(2) + Pairwise Cipher Suite List(count * 4) */
	if (rsn && (sme->ie_len - (rsn - sme->ie) >= 10)) {
		rsn_pos = 8 + 2 + (rsn[8] * 4);

		/* AKM Suite Count(2) + AKM Suite List(count * 4) */
		if ((sme->ie_len - (rsn - sme->ie)) >= rsn[1] + 2) {
			rsn_pos += 2 + (rsn[rsn_pos] * 4);
			rsn_len = rsn[1];
			/* RSN caps(2) + PMKID Count(2) */
			if ((rsn_pos + 2 + 2) <= (rsn_len + 2)) {
				rsn_pos += 4;
				rsn_pos += rsn[rsn_pos] * 16; /* PMKID List(count * 16) */
				if ((rsn_pos + 4) <= (rsn_len + 2)) {/* last 4 octets are group_mgmt field. */
					memcpy(&group_mgmt, &rsn[rsn_pos], 4);
					group_mgmt = be32_to_cpu(group_mgmt);
				}
			}
		}
	}

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] CONNECTING ssid=\"%.*s\"",
			 time[0], time[1], (int)sme->ssid_len, sme->ssid);
	if (sme->bssid)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " bssid=" MACSTR_NOMASK,
				 MAC2STR_LOG(sme->bssid));
	if (sme->bssid_hint)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " bssid_hint=" MACSTR_NOMASK,
				 MAC2STR_LOG(sme->bssid_hint));
	freq = sme->channel ? sme->channel->center_freq : 0;
	if (freq)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " freq=%d", freq);
	freq_hint = sme->channel_hint ? sme->channel_hint->center_freq : 0;
	if (freq_hint)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " freq_hint=%d", freq_hint);

	if (sme->crypto.n_ciphers_pairwise)
                pairwise = sme->crypto.ciphers_pairwise[0];
        group = sme->crypto.cipher_group;
        if (sme->crypto.n_akm_suites)
                akm_suite = sme->crypto.akm_suites[0];

	pos += scnprintf(log_buffer + pos, buf_size - pos, " pairwise=0x%x group=0x%x ",
			 pairwise, group);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "akm=0x%x auth_type=%d ",
			 akm_suite, (sme->auth_type == NL80211_AUTHTYPE_SAE) ? FAPI_AUTHENTICATIONTYPE_SAE : sme->auth_type);

	if (group_mgmt)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "group_mgmt=0x%x btcoex=%d", group_mgmt, btcoex);
	else
		pos += scnprintf(log_buffer + pos, buf_size - pos, "btcoex=%d", btcoex);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_connecting_fail(struct slsi_dev *sdev, struct net_device *dev,
				      const unsigned char *bssid,
				      int freq, int reason)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	char *str;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (1 == reason)
		str = "no probe response";
	else
		str = "unknown";

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] CONNECTING FAIL", time[0], time[1]);
	if (bssid)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " bssid=" MACSTR_NOMASK, MAC2STR_LOG(bssid));
	pos += scnprintf(log_buffer + pos, buf_size - pos, " freq=%d [reason=%d %s]", freq, reason, str);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_disconnect(struct slsi_dev *sdev, struct net_device *dev,
				 const unsigned char *bssid,
				 int reason)
{

	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	int rssi = 100;
	int status;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	sdev->conn_log2us_ctx.conn_flag = false;
	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	status = slsi_log2us_get_rssi(sdev, dev, &rssi, SLSI_PSID_UNIFI_LAST_BSS_RSSI);
	if (status)
		SLSI_ERR(sdev, "Could not get rssi status = %d\n", status);

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] DISCONN bssid="
			 MACSTR_NOMASK " rssi=%d "
			 "reason=%d", time[0], time[1], MAC2STR_LOG(bssid), rssi, reason);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);

}

void slsi_conn_log2us_eapol_gtk(struct slsi_dev *sdev, struct net_device *dev,
				int eapol_msg_type, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][EAPOL] GTK M%d ", time[0],
			 time[1], eapol_msg_type);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "rx");
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_eapol_gtk_tx(struct slsi_dev *sdev, u32 status_code, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL, *tx_status_str = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);

	if (status_code == FAPI_TRANSMISSIONSTATUS_SUCCESSFUL)
		tx_status_str = "ACK";
	else if (status_code == FAPI_TRANSMISSIONSTATUS_RETRY_LIMIT)
		tx_status_str = "NO_ACK";
	else
		tx_status_str = "TX_FAIL";

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][EAPOL] GTK M2 ",
			 time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "tx_status=%s",
			 tx_status_str);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_eapol_ptk(struct slsi_dev *sdev, struct net_device *dev,
				int eapol_msg_type, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][EAPOL] 4WAY M%d ", time[0],
			 time[1],
			 eapol_msg_type);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "rx");
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_eapol_ptk_tx(struct slsi_dev *sdev, u32 status_code, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL, *tx_status_str = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (status_code == FAPI_TRANSMISSIONSTATUS_SUCCESSFUL)
		tx_status_str = "ACK";
	else if (status_code == FAPI_TRANSMISSIONSTATUS_RETRY_LIMIT)
		tx_status_str = "NO_ACK";
	else
		tx_status_str = "TX_FAIL";

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][EAPOL] 4WAY M%d ",
			 time[0], time[1], sdev->conn_log2us_ctx.eapol_ptk_msg_type);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "tx_status=%s",
			 tx_status_str);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_eapol_tx(struct slsi_dev *sdev, struct net_device *dev, u32 status_code, u8 mlo_band)
{
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	if (sdev->conn_log2us_ctx.is_eapol_ptk) {
		slsi_conn_log2us_eapol_ptk_tx(sdev, status_code, mlo_band);
		sdev->conn_log2us_ctx.is_eapol_ptk = false;
	} else if (sdev->conn_log2us_ctx.is_eapol_gtk) {
		slsi_conn_log2us_eapol_gtk_tx(sdev, status_code, mlo_band);
		sdev->conn_log2us_ctx.is_eapol_gtk = false;
	}
}

void slsi_conn_log2us_roam_scan_start(struct slsi_dev *sdev, struct net_device *dev,
				      int reason, int roam_rssi_val,
				      short chan_utilisation,
				      int rssi_thresh, u64 timestamp)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	sdev->conn_log2us_ctx.roam_ap_count = 0;
	sdev->conn_log2us_ctx.cand_number = 0;
	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][ROAM] SCAN_START reason=%d ",
			 time[0], time[1], reason);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "rssi=%d cu=%d full_scan=%d rssi_thres=%d [fw_time=%llu]",
			 roam_rssi_val, chan_utilisation, sdev->conn_log2us_ctx.full_scan_roam, rssi_thresh, timestamp);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_roam_result(struct slsi_dev *sdev, struct net_device *dev,
				  char *bssid, u64 timestamp, bool roam_candidate)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	char *res = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (roam_candidate)
		res = "ROAM";
	else
		res = "NO_ROAM";

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][ROAM] RESULT %s",
			 time[0], time[1], res);
	if (roam_candidate && bssid)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " bssid=" MACSTR_NOMASK,
				 MAC2STR_LOG(bssid));
	pos += scnprintf(log_buffer + pos, buf_size - pos, " [fw_time=%llu status=%d]",
			 timestamp, !roam_candidate);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_eap_with_len(struct slsi_dev *sdev, struct net_device *dev,
				   u8 *eap, int eap_length, u8 mlo_band)
{
	int  pos         = 0;
	char *log_buffer = NULL;
	u32  time[2]     = { 0 };
	char *eap_type   = NULL;
	int buf_size = BUFF_SIZE;
	char str_type[15] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);

	switch (eap[SLSI_EAP_CODE_POS]) {
	case SLSI_EAP_PACKET_REQUEST:
		eap_type = "REQ";
		break;
	}

	sdev->conn_log2us_ctx.eap_str_type = eap[SLSI_EAP_TYPE_POS];
	get_eap_type_from_val(eap[SLSI_EAP_TYPE_POS], str_type);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][EAP] %s ",
			 time[0], time[1], eap_type);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "rx type=%s len=%d",
			 str_type, eap_length);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_eap_tx(struct slsi_dev *sdev, struct netdev_vif *ndev_vif,
			     int eap_length, int eap_type, char *tx_status_str, u8 mlo_band)
{
	int  pos         = 0;
	char *log_buffer = NULL;
	u32  time[2]     = { 0 };
	int buf_size = BUFF_SIZE;
	struct buff_list *new_node = NULL;
	char eap_type_str[15] = {0};

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (sdev->conn_log2us_ctx.host_tag_eap_type == SLSI_IEEE8021X_TYPE_EAP_PACKET) {
		get_eap_type_from_val(eap_type, eap_type_str);
		pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][EAP] RESP type=%s len=%d ",
				 time[0], time[1], eap_type_str, eap_length);
	} else if (sdev->conn_log2us_ctx.host_tag_eap_type == SLSI_IEEE8021X_TYPE_EAP_START) {
		pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][EAP] START ", time[0],
				 time[1]);
	}
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "tx_status=%s", tx_status_str);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_eap(struct slsi_dev *sdev, struct net_device *dev, u8 *eap, u8 mlo_band)
{
	int  pos         = 0;
	char *log_buffer = NULL;
	char *eap_type   = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);

	switch (eap[SLSI_EAP_CODE_POS]) {
	case SLSI_EAP_PACKET_SUCCESS:
		eap_type = "SUCC";
		break;
	case SLSI_EAP_PACKET_FAILURE:
		eap_type = "FAIL";
		break;
	}

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][EAP] %s ",
			 time[0], time[1], eap_type);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "rx");
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_dhcp(struct slsi_dev *sdev, struct net_device *dev, char *str, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][DHCP] %s ", time[0], time[1],
			 str);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "rx");
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_dhcp_tx(struct slsi_dev *sdev, struct net_device *dev, char *tx_status, u8 mlo_band,
			      char *dhcp_type_str)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	if (!dhcp_type_str)
		return;

	new_node = slsi_conn_log2us_alloc_atomic_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][DHCP] %s ",
			 time[0], time[1], dhcp_type_str);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "tx_status=%s",
			 tx_status);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_log2us_handle_frame_tx_status(struct slsi_dev *sdev, struct net_device *dev,
					u16 host_tag, u16 tx_status, u8 mlo_band)
{
	char *tx_status_str;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	char *dhcp_str = NULL;

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	if (tx_status == FAPI_TRANSMISSIONSTATUS_SUCCESSFUL)
		tx_status_str = "ACK";
	else if (tx_status == FAPI_TRANSMISSIONSTATUS_RETRY_LIMIT)
		tx_status_str = "NO_ACK";
	else
		tx_status_str = "TX_FAIL";

	if (!host_tag) {
		SLSI_ERR_NODEV("host_tag 0\n");
		return;
	}

	if (host_tag == sdev->conn_log2us_ctx.host_tag_eap) {
		if (sdev->conn_log2us_ctx.host_tag_eap_type == SLSI_IEEE8021X_TYPE_EAP_PACKET)
			slsi_conn_log2us_eap_tx(sdev, ndev_vif, sdev->conn_log2us_ctx.eap_resp_len,
						sdev->conn_log2us_ctx.eap_str_type, tx_status_str, mlo_band);
		else if (sdev->conn_log2us_ctx.host_tag_eap_type == SLSI_IEEE8021X_TYPE_EAP_START)
			slsi_conn_log2us_eap_tx(sdev, ndev_vif, sdev->conn_log2us_ctx.eap_start_len,
						sdev->conn_log2us_ctx.eap_str_type, tx_status_str, mlo_band);
		sdev->conn_log2us_ctx.host_tag_eap = 0;
	} else if (host_tag & SLSI_HOST_TAG_DHCP_DISC_MASK) {
		dhcp_str = "DISCOVER";
		slsi_conn_log2us_dhcp_tx(sdev, dev, tx_status_str, mlo_band, dhcp_str);
	} else if (host_tag & SLSI_HOST_TAG_DHCP_REQ_MASK) {
		dhcp_str = "REQUEST";
		slsi_conn_log2us_dhcp_tx(sdev, dev, tx_status_str, mlo_band, dhcp_str);
	} else {
		SLSI_ERR_NODEV("host_tag %d doesn't match\n", host_tag);
	}
}

void slsi_conn_log2us_auth_req(struct slsi_dev *sdev, struct net_device *dev, const unsigned char *bssid,
			       int auth_algo, int sae_type, int sn, int status, u32 tx_status, int is_roaming)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	char *tx_status_str;
	int rssi = 100;
	int res;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	if (status == FAPI_RESULTCODE_AUTH_TX_FAIL)
		tx_status_str = "TX_FAIL";
	else if (status == FAPI_RESULTCODE_AUTH_NO_ACK)
		tx_status_str = "NO_ACK";
	else
		tx_status_str = "ACK";
	get_kernel_timestamp(time);
	res = slsi_log2us_get_rssi(sdev, dev, &rssi, SLSI_PSID_UNIFI_RSSI);
	if (res)
		SLSI_ERR(sdev, "Could not get rssi status = %d\n", res);

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] AUTH REQ bssid=" MACSTR_NOMASK,
			 time[0], time[1], MAC2STR_LOG(bssid));
	pos += scnprintf(log_buffer + pos, buf_size - pos, " rssi=%d auth_algo=%d type=%d sn=%d status=%d",
			 rssi, auth_algo, sae_type, sn, status);
	if (is_roaming)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " tx_status=%s [ROAM]", tx_status_str);
	else
		pos += scnprintf(log_buffer + pos, buf_size - pos, " tx_status=%s", tx_status_str);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_auth_resp(struct slsi_dev *sdev, struct net_device *dev,
				const unsigned char *bssid, int auth_algo,
				int sae_type,
				int sn, int status, int is_roaming)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] AUTH RESP",
			 time[0], time[1]);
	if (is_roaming)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " bssid="
				 MACSTR_NOMASK " auth_algo=%d "
				 "type=%d sn=%d status=%d [ROAM]", MAC2STR_LOG(bssid),
				 auth_algo, sae_type, sn, status);
	else
		pos += scnprintf(log_buffer + pos, buf_size - pos, " bssid="
				 MACSTR_NOMASK " auth_algo=%d "
				 "type=%d sn=%d status=%d", MAC2STR_LOG(bssid),
				 auth_algo, sae_type, sn, status);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_assoc_req(struct slsi_dev *sdev, struct net_device *dev,
				const unsigned char *bssid, int sn, int tx_status,
				int mgmt_frame_subtype, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	char *tx_status_str;
	int rssi = 100;
	int status;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (tx_status == FAPI_RESULTCODE_ASSOC_TX_FAIL)
		tx_status_str = "TX_FAIL";
	else if (tx_status == FAPI_RESULTCODE_ASSOC_NO_ACK)
		tx_status_str = "NO_ACK";
	else
		tx_status_str = "ACK";
	status = slsi_log2us_get_rssi(sdev, dev, &rssi, SLSI_PSID_UNIFI_RSSI);
	if (status)
		SLSI_ERR(sdev, "Could not get rssi status = %d\n", status);
	if (mgmt_frame_subtype == SLSI_MGMT_FRAME_SUBTYPE_ASSOC_REQ)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] ASSOC REQ ",
				 time[0], time[1]);
	else if (mgmt_frame_subtype == SLSI_MGMT_FRAME_SUBTYPE_REASSOC_REQ)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] REASSOC REQ ",
				 time[0], time[1]);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "bssid="
			 MACSTR_NOMASK  " rssi=%d sn=%d ",
			 MAC2STR_LOG(bssid), rssi, sn);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "tx_status=%s", tx_status_str);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_assoc_resp(struct slsi_dev *sdev, struct net_device *dev,
				 const unsigned char *bssid, int sn, int status,
				 int mgmt_frame_subtype, int aid, const unsigned char *mld_addr)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (mgmt_frame_subtype == SLSI_MGMT_FRAME_SUBTYPE_ASSOC_RESP)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] ASSOC RESP",
				 time[0], time[1]);
	else if (mgmt_frame_subtype == SLSI_MGMT_FRAME_SUBTYPE_REASSOC_RESP)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] REASSOC RESP",
				 time[0], time[1]);
	pos += scnprintf(log_buffer + pos, buf_size - pos, " bssid="
			 MACSTR_NOMASK " sn=%d "
			 "status=%d assoc_id=%d", MAC2STR_LOG(bssid), sn, status, aid);

#ifdef CONFIG_SCSC_WLAN_EHT
	if (slsi_check_ml_ie_from_bcn_ie(dev) && !is_zero_ether_addr(mld_addr))
		pos += scnprintf(log_buffer + pos, buf_size - pos, " mld_mac=" MACSTR_NOMASK,
				 MAC2STR_LOG(mld_addr));
#endif
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_deauth(struct slsi_dev *sdev, struct net_device *dev, char *str_type,
			     const unsigned char *bssid,
			     int sn, int status, char *vs_ie)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	int rssi = 100;
	int res;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	sdev->conn_log2us_ctx.conn_flag = false;
	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	res = slsi_log2us_get_rssi(sdev, dev, &rssi, SLSI_PSID_UNIFI_LAST_BSS_RSSI);
	if (res)
		SLSI_ERR(sdev, "Could not get rssi status = %d\n", res);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] DEAUTH %s bssid="
			 MACSTR_NOMASK " rssi=%d sn=%d reason=%d", time[0], time[1],
			 str_type, MAC2STR_LOG(bssid), rssi, sn, status);
	if (vs_ie)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " VSIE=%s", vs_ie);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_disassoc(struct slsi_dev *sdev, struct net_device *dev, char *str_type,
			       const unsigned char *bssid,
			       int sn, int status, char *vs_ie)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	int rssi = 100;
	int res;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	sdev->conn_log2us_ctx.conn_flag = false;
	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	res = slsi_log2us_get_rssi(sdev, dev, &rssi, SLSI_PSID_UNIFI_LAST_BSS_RSSI);
	if (res)
		SLSI_ERR(sdev, "Could not get rssi status = %d\n", res);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][CONN] DISASSOC %s bssid="
			 MACSTR_NOMASK " rssi=%d sn=%d "
			 "reason=%d", time[0], time[1], str_type, MAC2STR_LOG(bssid),
			 rssi, sn, status);
	if (vs_ie)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " VSIE=%s", vs_ie);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_roam_scan_save(struct slsi_dev *sdev, struct net_device *dev,
				     int scan_type, int freq_count,
				     int *freq_list)
{
	int i;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	sdev->conn_log2us_ctx.roam_freq_count = freq_count;

	for (i = 0; i < freq_count; i++)
		sdev->conn_log2us_ctx.roam_freq_list[i] = *(freq_list + i);

	if (scan_type == FAPI_SCANTYPE_SOFT_CACHED_ROAMING_SCAN ||
	    scan_type == FAPI_SCANTYPE_HARD_CACHED_ROAMING_SCAN)
		sdev->conn_log2us_ctx.full_scan_roam = 0;
	else if (scan_type == FAPI_SCANTYPE_SOFT_FULL_ROAMING_SCAN ||
		 scan_type == FAPI_SCANTYPE_HARD_FULL_ROAMING_SCAN)
		sdev->conn_log2us_ctx.full_scan_roam = 1;
}

void slsi_conn_log2us_roam_scan_done(struct slsi_dev *sdev, struct net_device *dev, u64 timestamp)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	int i;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);
	int btcoex = 0;

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);

	btcoex = sdev->conn_log2us_ctx.bt_device_status;

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][ROAM] SCAN_DONE btcoex=%d ap_count=%d freq[%d]=",
			 time[0], time[1], btcoex, sdev->conn_log2us_ctx.roam_ap_count,
			 sdev->conn_log2us_ctx.roam_freq_count);

	for (i = 0; i < sdev->conn_log2us_ctx.roam_freq_count; i++)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "%d ",
				 sdev->conn_log2us_ctx.roam_freq_list[i]);

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[fw_time=%llu]", timestamp);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	enqueue_log_buffer(sdev->conn_log2us_ctx.list_roam_cand, &sdev->conn_log2us_ctx);
	sdev->conn_log2us_ctx.list_roam_cand = NULL;
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_roam_scan_result(struct slsi_dev *sdev, struct net_device *dev,
				       bool candidate, char *bssid, int freq,
				       int rssi, short cu,
				       int score, int tp_score, bool eligible, bool mld_ap)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	sdev->conn_log2us_ctx.roam_ap_count++;
	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (candidate) {
		pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][ROAM] SCORE_CANDI[%d] %s bssid="
				 MACSTR_NOMASK " freq=%d rssi=%d cu=%d score=%d.%02d tp=%dkbps [eligible=%s]",
				 time[0], time[1], sdev->conn_log2us_ctx.cand_number, mld_ap ? "MLD" : "",
				 MAC2STR_LOG(bssid), freq / 2, rssi, cu, score / 100, score % 100,
				 tp_score, eligible ? "true" : "false");
		sdev->conn_log2us_ctx.cand_number++;
	} else {
		pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][ROAM] SCORE_CUR_AP %s bssid="
				 MACSTR_NOMASK " freq=%d rssi=%d cu=%d score=%d.%02d", time[0],
				 time[1], mld_ap ? "MLD" : "", MAC2STR_LOG(bssid), freq / 2, rssi, cu, score / 100,
				 score % 100);
	}
	new_node->len = pos + 1;
	enqueue_log_buffer_for_roam_cand(new_node, &sdev->conn_log2us_ctx);
}

void slsi_conn_log2us_btm_query(struct slsi_dev *sdev, struct net_device *dev,
				int dialog, int reason, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][BTM] QUERY ",
			 time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "token=%d reason=%d",
			 dialog, reason);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_btm_req(struct slsi_dev *sdev, struct net_device *dev,
			      int dialog, int btm_mode, int disassoc_timer,
			      int validity_time, int candidate_count, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	sdev->conn_log2us_ctx.cand_count = 0;
	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][BTM] REQ ", time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "token=%d mode=%d disassoc=%d"
			 " validity=%d candidate_list_cnt=%d", dialog, btm_mode,
			 disassoc_timer, validity_time, candidate_count);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_btm_resp(struct slsi_dev *sdev, struct net_device *dev, int dialog,
			       int btm_mode, int delay, char *bssid, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][BTM] RESP token=%d ",
			 time[0], time[1], dialog);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "status=%d delay=%d",
			 btm_mode, delay);
	if (btm_mode == 0)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " target=" MACSTR_NOMASK, MAC2STR_LOG(bssid));
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_btm_cand(struct slsi_dev *sdev, struct net_device *dev,
			       char *bssid, int prefer)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][BTM] REQ_CANDI[%d] bssid="
			 MACSTR_NOMASK " preference=%d", time[0], time[1],
			 sdev->conn_log2us_ctx.cand_count, MAC2STR_LOG(bssid), prefer);
	sdev->conn_log2us_ctx.cand_count++;
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_nr_frame_req(struct slsi_dev *sdev, struct net_device *dev,
				   int dialog_token, char *ssid, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][NBR_RPT] REQ ",
				 time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	if(ssid)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " token=%d ssid=\"%s\"",
				 dialog_token, ssid);
	else
		pos += scnprintf(log_buffer + pos, buf_size - pos, " token=%d ssid=\"\"",
				 dialog_token);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_nr_frame_resp(struct slsi_dev *sdev, struct net_device *dev,
				    int dialog_token, int freq_count, int *freq_list,
				    int report_number, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);
	int i = 0;

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][NBR_RPT] RESP ",
			 time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, " token=%d freq[%d]=",
			 dialog_token, freq_count);
	for (i = 0; i < freq_count; i++)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "%d ", *(freq_list + i));
	pos += scnprintf(log_buffer + pos, buf_size - pos, "report_number=%d ", report_number);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_beacon_report_request(struct slsi_dev *sdev, struct net_device *dev,
					    int dialog_token, int operating_class, char *string,
					    int measure_duration, char *measure_mode, u8 request_mode, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][BCN_RPT] REQ ",
			 time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "token=%d mode=%s "
			 "operating_class=%d channel=%s duration=%d request_mode=0x%x",
			 dialog_token, measure_mode, operating_class, string,
			 measure_duration, request_mode);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_beacon_report_response(struct slsi_dev *sdev, struct net_device *dev,
					     int dialog_token, int report_number, u8 mlo_band)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][BCN_RPT] RESP ", time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "token=%d"
				 " report_number=%d", dialog_token, report_number);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_ncho_mode(struct slsi_dev *sdev, struct net_device *dev, int enable)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos = scnprintf(log_buffer, buf_size, "[%d.%d][NCHO] MODE enable=%d",
			 time[0], time[1], enable);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_roam_cancelled(struct slsi_dev *sdev, struct net_device *dev,
				     int reason_code, char *reason_desc)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos,
			 "[%d.%d][ROAM] CANCELED [reason=%d %s]\n",
			 time[0], time[1], reason_code, reason_desc);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

#ifdef CONFIG_SCSC_WLAN_EHT
void slsi_conn_log2us_mld_setup(struct slsi_dev *sdev, struct net_device *dev, int band,
			       char *bssid, int status_code, int link_id)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][MLD] SETUP ", time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, band);
	pos +=  scnprintf(log_buffer + pos, buf_size - pos, "status=%d bssid=" MACSTR_NOMASK, status_code,
			  MAC2STR_LOG(bssid));
	if (!status_code)
		pos +=  scnprintf(log_buffer + pos, buf_size - pos, " link_id=%d", link_id);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_mld_reconfig(struct slsi_dev *sdev, struct net_device *dev, int band, int link_id)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][MLD] RECONFIG ", time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, band);
	pos +=  scnprintf(log_buffer + pos, buf_size - pos, "link_id=%d", link_id);
	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

static int slsi_log2us_fill_tid2lm_string(char *log_buffer, int buf_size, int position, int tid2lm, int direction)
{
	int i = 0;
	int pos = position;

	if (!direction)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "tid_dl=");
	else
		pos += scnprintf(log_buffer + pos, buf_size - pos, " tid_ul=");

	if (tid2lm == 0) {
		pos += scnprintf(log_buffer + pos, buf_size - pos, "NONE");
		return pos;
	} else if (tid2lm == 0xFF) {
		pos += scnprintf(log_buffer + pos, buf_size - pos, "ALL");
		return pos;
	}
	while(tid2lm) {
		if (tid2lm & 0x01) {
			pos += scnprintf(log_buffer + pos, buf_size - pos, "%d", i);
			tid2lm = tid2lm >> 1;
			if (tid2lm)
				pos += scnprintf(log_buffer + pos, buf_size - pos, ",");
		} else {
			tid2lm = tid2lm >> 1;
		}
		i++;
	}
	return pos;
}

void slsi_conn_log2us_mld_t2lm_status(struct slsi_dev *sdev, struct net_device *dev, int band, int tid_dl, int tid_ul)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][MLD] T2LM STATUS ", time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, band) ;
	pos = slsi_log2us_fill_tid2lm_string(log_buffer, buf_size, pos, tid_dl, 0);
	pos = slsi_log2us_fill_tid2lm_string(log_buffer, buf_size, pos, tid_ul, 1);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_mld_t2lm_req_rsp(struct slsi_dev *sdev, struct net_device *dev, int t2lm_ftype, int mlo_band,
				       int dialog_token, int status_code, int tx_status)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);
	char *ftype_str;
	char *tx_status_str = NULL;

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (!t2lm_ftype)
		ftype_str = "REQ";
	else
		ftype_str = "RESP";

	if (tx_status == 2)
		tx_status_str = "tx_status=ACK";
	else if (tx_status == 1)
		tx_status_str = "tx_status=NO_ACK";
	else if (tx_status == 0)
		tx_status_str = "tx_status=TX_FAIL";
	else
		tx_status_str = "Rx";

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][MLD] T2LM %s ", time[0], time[1], ftype_str);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos +=  scnprintf(log_buffer + pos, buf_size - pos, "token=%d status=%d %s", dialog_token, status_code,
			  tx_status_str);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

void slsi_conn_log2us_mld_t2lm_teardown(struct slsi_dev *sdev, struct net_device *dev, int mlo_band, int tx_status)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);
	char *tx_status_str = NULL;

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	if (tx_status == 2)
		tx_status_str = "ACK";
	else if (tx_status == 1)
		tx_status_str = "NO_ACK";
	else
		tx_status_str = "TX_FAIL";

	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][MLD] T2LM TEARDOWN ", time[0], time[1]);
	pos = slsi_log2us_fill_mlo_band_info(log_buffer, buf_size, pos, mlo_band);
	pos +=  scnprintf(log_buffer + pos, buf_size - pos, "tx_status=%s", tx_status_str);

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}

static int slsi_log2us_fill_mlo_active_inactive_band(char *log_buffer, int buf_size, int pos, u8 mlo_band,
						     bool is_active)
{
	bool is_2g = false;
	bool is_5g = false;
	bool is_6g = false;

	if (!mlo_band)
		return pos;

	is_2g = SLSI_UC_MAC_2_4_BAND & mlo_band;
	is_5g = SLSI_UC_MAC_5_BAND & mlo_band;
	is_6g = SLSI_UC_MAC_6_BAND & mlo_band;

	if (is_active)
		pos += scnprintf(log_buffer + pos, buf_size - pos, " active=");
	else
		pos += scnprintf(log_buffer + pos, buf_size - pos, " inactive=");

	if (is_2g && is_5g && is_6g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "2G,5G,6G");
	else if (is_2g && is_5g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "2G,5G");
	else if (is_2g && is_6g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "2G,6G");
	else if (is_5g && is_6g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "5G,6G");
	else if (is_2g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "2G");
	else if (is_5g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "5G");
	else if (is_6g)
		pos += scnprintf(log_buffer + pos, buf_size - pos, "6G");

	return pos;
}

static char *slsi_get_mld_link_reason_string(u8 event)
{
	switch (event) {
	case SLSI_STA_MLO_LS_EVENT_HIGH_BMISS:
		return "High Beacon miss";
	case SLSI_STA_MLO_LS_EVENT_LESS_BMISS:
		return "Less Beacon miss";
	case SLSI_STA_MLO_LS_EVENT_HIGH_BRSSI:
		return "High Beacon RSSI";
	case SLSI_STA_MLO_LS_EVENT_LESS_BRSSI:
		return "Less Beacon RSSI";
	case SLSI_STA_MLO_LS_EVENT_HIGH_CONGESTION:
		return "High Congestion";
	case SLSI_STA_MLO_LS_EVENT_LESS_CONGESTION:
		return "Less Congestion";
	case SLSI_STA_MLO_LS_EVENT_HIGH_THRPUT:
		return "High Throughput";
	case SLSI_STA_MLO_LS_EVENT_LESS_THRPUT:
		return "Less Throughput";
	case SLSI_STA_MLO_LS_EVENT_LOW_LTNCY_DISABLE:
		return "Low Latency Disable";
	case SLSI_STA_MLO_LS_EVENT_LOW_LTNCY_ENABLE:
		return "Low Latency Enable";
	case SLSI_STA_MLO_LS_SAME_TXOP_UL_DL :
		return "Same TXOP UL DL";
	case SLSI_STA_MLO_LS_COEX_IN_USE:
		return "Coex is in Use";
	case SLSI_STA_MLO_LS_DISABLE_LINK:
		return "Disable link";
	case SLSI_STA_MLO_LS_ENABLE_LINK:
		return "Enable link";
	case SLSI_STA_MLO_LS_ACTIVE_LINK:
		return "Active link";
	case SLSI_STA_MLO_LS_INACTIVE_LINK:
		return "Inactive link";
	case SLSI_STA_MLO_LS_FORCED_ACTIVE_LINK:
		return "Forced active link";
	case SLSI_STA_MLO_LS_FORCED_INACTIVE_LINK:
		return "Forced inactive link";
	default:
		return "Unknown";
	}
}

void slsi_conn_log2us_mld_link(struct slsi_dev *sdev, struct net_device *dev, int active_band, int inactive_band,
			       u8 reason)
{
	int pos = 0;
	char *log_buffer = NULL;
	int buf_size = BUFF_SIZE;
	u32 time[2] = { 0 };
	struct buff_list *new_node = NULL;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	if (ndev_vif->iftype != NL80211_IFTYPE_STATION)
		return;

	new_node = slsi_conn_log2us_alloc_new_node();
	if (!new_node)
		return;
	log_buffer = new_node->str;

	get_kernel_timestamp(time);
	pos += scnprintf(log_buffer + pos, buf_size - pos, "[%d.%d][MLD] LINK", time[0], time[1]);
	pos = slsi_log2us_fill_mlo_active_inactive_band(log_buffer, buf_size, pos, active_band, true);
	pos = slsi_log2us_fill_mlo_active_inactive_band(log_buffer, buf_size, pos, inactive_band, false);
	pos +=  scnprintf(log_buffer + pos, buf_size - pos, " reason=%s", slsi_get_mld_link_reason_string(reason));

	new_node->len = pos + 1;
	enqueue_log_buffer(new_node, &sdev->conn_log2us_ctx);
	queue_work(sdev->conn_log2us_ctx.log2us_workq, &sdev->conn_log2us_ctx.log2us_work);
}
#endif
#else

void slsi_conn_log2us_init(struct slsi_dev *sdev)
{
}

void slsi_conn_log2us_deinit(struct slsi_dev *sdev)
{
}

void slsi_conn_log2us_connecting(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_connect_params *sme)
{
}

void slsi_conn_log2us_connect_sta_info(struct slsi_dev *sdev, struct net_device *dev)
{
}

void slsi_conn_log2us_connecting_fail(struct slsi_dev *sdev, struct net_device *dev,
				      const unsigned char *bssid,
				      int freq, int reason)
{
}

void slsi_conn_log2us_disconnect(struct slsi_dev *sdev, struct net_device *dev,
				 const unsigned char *bssid, int reason)
{
}

void slsi_conn_log2us_eapol_gtk(struct slsi_dev *sdev, struct net_device *dev,
				int eapol_msg_type, u8 mlo_band)
{
}

void slsi_conn_log2us_eapol_ptk(struct slsi_dev *sdev, struct net_device *dev,
				int eapol_msg_type, u8 mlo_band)
{
}

void slsi_conn_log2us_roam_scan_start(struct slsi_dev *sdev, struct net_device *dev,
				      int reason, int roam_rssi_val,
				      short chan_utilisation,
				      int rssi_thresh, u64 timestamp)
{
}

void slsi_conn_log2us_roam_result(struct slsi_dev *sdev, struct net_device *dev,
				  char *bssid, u64 timestamp, bool roam_candidate)
{
}

void slsi_conn_log2us_eap(struct slsi_dev *sdev, struct net_device *dev, u8 *eap_type, u8 mlo_band)
{
}

void slsi_conn_log2us_dhcp(struct slsi_dev *sdev, struct net_device *dev, char *str, u8 mlo_band)
{
}

void slsi_conn_log2us_dhcp_tx(struct slsi_dev *sdev, struct net_device *dev, char *tx_status, u8 mlo_band,
			      char *dhcp_type_str)
{
}

void slsi_conn_log2us_eap_with_len(struct slsi_dev *sdev, struct net_device *dev,
				   u8 *eap_type, int eap_length, u8 mlo_band)
{
}

void slsi_conn_log2us_auth_req(struct slsi_dev *sdev, struct net_device *dev,
			       const unsigned char *bssid,
			       int auth_algo, int sae_type, int sn, int status, u32 tx_status, int is_roaming)
{
}

void slsi_conn_log2us_auth_resp(struct slsi_dev *sdev, struct net_device *dev,
				const unsigned char *bssid, int auth_algo,
				int sae_type,
				int sn, int status, int is_roaming)
{
}

void slsi_conn_log2us_assoc_req(struct slsi_dev *sdev, struct net_device *dev,
				const unsigned char *bssid, int sn, int tx_status,
				int mgmt_frame_subtype, u8 mlo_band)
{
}

void slsi_conn_log2us_assoc_resp(struct slsi_dev *sdev, struct net_device *dev,
				 const unsigned char *bssid, int sn, int status,
				 int mgmt_frame_subtype, int aid, const unsigned char *mld_addr)
{
}

void slsi_conn_log2us_deauth(struct slsi_dev *sdev, struct net_device *dev, char *str_type,
			     const unsigned char *bssid, int sn, int status, char *vs_ie)
{
}

void slsi_conn_log2us_disassoc(struct slsi_dev *sdev, struct net_device *dev, char *str_type,
			       const unsigned char *bssid,
			       int sn, int status, char *vs_ie)
{
}

void slsi_conn_log2us_roam_scan_done(struct slsi_dev *sdev, struct net_device *dev, u64 timestamp)
{
}

void slsi_conn_log2us_roam_scan_result(struct slsi_dev *sdev, struct net_device *dev,
				       bool candidate, char *bssid, int freq,
				       int rssi, short cu,
				       int score, int tp_score, bool eligible, bool mld_ap)
{
}

void slsi_conn_log2us_btm_query(struct slsi_dev *sdev, struct net_device *dev,
				int dialog, int reason, u8 mlo_band)
{
}

void slsi_conn_log2us_btm_req(struct slsi_dev *sdev, struct net_device *dev,
			      int dialog, int btm_mode, int disassoc_timer,
			      int validity_time, int candidate_count, u8 mlo_band)
{
}

void slsi_conn_log2us_btm_resp(struct slsi_dev *sdev, struct net_device *dev,
			       int dialog, int btm_mode, int delay, char *bssid, u8 mlo_band)
{
}

u8 *get_eap_type_from_val(int val, u8 *str)
{
	return NULL;
}

void slsi_conn_log2us_eapol_tx(struct slsi_dev *sdev, struct net_device *dev, u32 status_code, u8 mlo_band)
{
}

void slsi_conn_log2us_eapol_ptk_tx(struct slsi_dev *sdev, u32 status_code, u8 mlo_band)
{
}

void slsi_conn_log2us_eapol_gtk_tx(struct slsi_dev *sdev, u32 status_code, u8 mlo_band)
{
}

void slsi_conn_log2us_eap_tx(struct slsi_dev *sdev, struct netdev_vif *ndev_vif,
			     int eap_length, int eap_type, char *tx_status_str, u8 mlo_band)
{
}

void slsi_conn_log2us_btm_cand(struct slsi_dev *sdev, struct net_device *dev,
			       char *bssid, int prefer)
{
}

void slsi_log2us_handle_frame_tx_status(struct slsi_dev *sdev, struct net_device *dev,
					u16 host_tag, u16 tx_status, u8 mlo_band)
{
}

void slsi_conn_log2us_roam_scan_save(struct slsi_dev *sdev, struct net_device *dev,
				     int scan_type, int freq_count, int *freq_list)
{
}

void slsi_conn_log2us_nr_frame_req(struct slsi_dev *sdev, struct net_device *dev, int dialog_token,
				   char *ssid, u8 mlo_band)
{
}

void slsi_conn_log2us_nr_frame_resp(struct slsi_dev *sdev, struct net_device *dev, int dialog_token,
				    int freq_count, int *freq_list, int report_number, u8 mlo_band)
{
}

void slsi_conn_log2us_beacon_report_request(struct slsi_dev *sdev, struct net_device *dev,
					    int dialog_token, int operating_class, char *string,
					    int measure_duration, char *measure_mode, u8 request_mode, u8 mlo_band)
{
}

void slsi_conn_log2us_beacon_report_response(struct slsi_dev *sdev, struct net_device *dev, int dialog_token,
					     int report_number, u8 mlo_band)
{
}

void slsi_conn_log2us_ncho_mode(struct slsi_dev *sdev, struct net_device *dev, int enable)
{
}

void slsi_conn_log2us_roam_cancelled(struct slsi_dev *sdev, struct net_device *dev, int reason_code,
				     char *reason_desc)
{
}
#endif
