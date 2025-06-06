/*****************************************************************************
 *
 * Copyright (c) 2014 - 2023 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/
#include <linux/version.h>
#include <net/cfg80211.h>
#include <net/ip.h>
#include <linux/etherdevice.h>
#if defined(CONFIG_SCSC_WLAN_TAS)
#include <net/genetlink.h>
#endif
#include "dev.h"
#include "cfg80211_ops.h"
#include "debug.h"
#include "mgt.h"
#include "mlme.h"
#include "netif.h"
#include "unifiio.h"
#include "mib.h"
#include "lls.h"
#include "nl80211_vendor.h"
#include "log2us.h"
#include <linux/uaccess.h>
#include "scsc_wifilogger_ring_connectivity_api.h"
#ifdef CONFIG_SCSC_WLAN_ENHANCED_LOGGING
#include "scsc_wifilogger.h"
#include "scsc_wifilogger_rings.h"
#include "scsc_wifilogger_types.h"
#include "scsc_wifilogger_ring_connectivity_api.h"
#endif

#define SLSI_WIFI_TAG_VENDOR_SPECIFIC    0
#define SLSI_WIFI_TAG_BSSID              1
#define SLSI_WIFI_TAG_ADDR               2
#define SLSI_WIFI_TAG_SSID               3
#define SLSI_WIFI_TAG_STATUS             4
#define SLSI_WIFI_TAG_IE                          12
#define SLSI_WIFI_TAG_REASON_CODE        14
#define SLSI_WIFI_TAG_RSSI               21
#define SLSI_WIFI_TAG_CHANNEL                22
#define SLSI_WIFI_TAG_LINK_ID                23
#define SLSI_WIFI_TAG_EAPOL_MESSAGE_TYPE 29

#define SLSI_GSCAN_INVALID_RSSI        0x7FFF
#define SLSI_EPNO_AUTH_FIELD_WEP_OPEN  1
#define SLSI_EPNO_AUTH_FIELD_WPA_PSK   2
#define SLSI_EPNO_AUTH_FIELD_WPA_EAP   4

#define SLSI_WIFI_TAG_VD_FRAME                0xf00a
#define SLSI_WIFI_TAG_VD_RETRY_COUNT          0xf00f
#define SLSI_WIFI_TAG_VD_EAPOL_KEY_TYPE       0xF008
#define SLSI_WIFI_TAG_VD_LOCAL_DURATION       0xf00b
#define SLSI_WIFI_TAG_VD_SCAN_TYPE            0xf012
#define SLSI_WIFI_TAG_VD_SEQUENCE_NUMBER      0xf014
#define SLSI_WIFI_TAG_VD_ROAMING_REASON       0xf019
#define SLSI_WIFI_TAG_VD_CHANNEL_UTILISATION  0xf01a
#define SLSI_WIFI_TAG_VD_BTM_REQUEST_MODE     0xf01b
#define SLSI_WIFI_TAG_VD_BTM_RESPONSE_STATUS  0xf01c
#define SLSI_WIFI_TAG_VD_SCORE                0xf01d
#define SLSI_WIFI_TAG_VD_RSSI_THRESHOLD       0xf01e
#define SLSI_WIFI_TAG_VD_DIASSOCIATION_TIMER  0xf01f
#define SLSI_WIFI_TAG_VD_VALIDITY_INTERVAL    0xf020
#define SLSI_WIFI_TAG_VD_CANDIDATE_LIST_COUNT 0xf021
#define SLSI_WIFI_TAG_VD_OPERATING_CLASS      0xf022
#define SLSI_WIFI_TAG_VD_MEASUREMENT_MODE     0xf023
#define SLSI_WIFI_TAG_VD_MEASUREMENT_DURATION 0xf024
#define SLSI_WIFI_TAG_VD_MIN_AP_COUNT         0xf025
#define SLSI_WIFI_TAG_VD_CLUSTER_ID           0xf026
#define SLSI_WIFI_TAG_VD_NAN_ROLE             0xf027
#define SLSI_WIFI_TAG_VD_NAN_AMR              0xf028
#define SLSI_WIFI_TAG_VD_NAN_HOP_COUNT        0xf029
#define SLSI_WIFI_TAG_VD_NAN_NMI              0xf02a
#define SLSI_WIFI_TAG_VD_MESSAGE_TYPE         0xf02b
#define SLSI_WIFI_TAG_VD_ESTIMATED_TP         0xf02c
#define SLSI_WIFI_TAG_VD_EXPIRED_TIMER_VALUE  0xf02d
#define SLSI_WIFI_TAG_VD_NAN_NDP_ID           0xf02e
#define SLSI_WIFI_TAG_VD_MASTER_TSF           0xf030
#define SLSI_WIFI_TAG_VD_SCHEDULE_TYPE        0xf031
#define SLSI_WIFI_TAG_VD_START_OFFSET         0xf032
#define SLSI_WIFI_TAG_VD_SLOT_DURATION        0xf033
#define SLSI_WIFI_TAG_VD_SLOT_PERIOD          0xf034
#define SLSI_WIFI_TAG_VD_BITMAP               0xf035
#define SLSI_WIFI_TAG_VD_CHANNEL_INFO         0xf036
#define SLSI_WIFI_TAG_VD_ULW_REASON           0xf037
#define SLSI_WIFI_TAG_VD_ULW_INDEX            0xf038
#define SLSI_WIFI_TAG_VD_ULW_START_TIME       0xf039
#define SLSI_WIFI_TAG_VD_ULW_PERIOD           0xf03a
#define SLSI_WIFI_TAG_VD_ULW_DURATION         0xf03b
#define SLSI_WIFI_TAG_VD_ULW_COUNT            0xf03c
#define SLSI_WIFI_TAG_VD_NAN_RX_TOTAL         0xf03d
#define SLSI_WIFI_TAG_VD_NAN_TX_TOTAL         0xf03e
#define SLSI_WIFI_TAG_VD_NAN_RX_AVERAGE       0xf03f
#define SLSI_WIFI_TAG_VD_NAN_TX_AVERAGE       0xf040
#define SLSI_WIFI_TAG_VD_FULL_SCAN_COUNT      0xf041
#define SLSI_WIFI_TAG_VD_CU_RSSI_THRESHOLD    0xf042
#define SLSI_WIFI_TAG_VD_CU_THRESHOLD         0xf043
#define SLSI_WIFI_TAG_VD_ROAMING_TYPE         0xf044
#define SLSI_WIFI_TAG_VD_AUTH_ALGO            0xf046
#define SLSI_WIFI_TAG_VD_SAE_MESSAGE_TYPE     0xf047
#define SLSI_WIFI_TAG_VD_DIALOG_TOKEN         0xf048
#define SLSI_WIFI_TAG_VD_BTM_CANDIDATE_PREFERENCE 0xf049
#define SLSI_WIFI_TAG_VD_BTM_CANDIDATE_COUNT  0xf04a
#define SLSI_WIFI_TAG_VD_ELIGIBLE             0xf04b
#define SLSI_WIFI_TAG_VD_PARAMETER_SET        0xff00
#define WIFI_TAG_VD_MGMT_FRAME_SUBTYPE        0xf04c
#define SLSI_WIFI_TAG_VD_IS_ROAMING           0xf04d
#define SLSI_WIFI_TAG_VD_AID                  0xf04e
#define SLSI_WIFI_TAG_VD_MEASUREMENT_REQUEST_MODE  0xf04f
#define SLSI_WIFI_TAG_VD_BANDS 0xF050
#define SLSI_WIFI_TAG_VD_MLD 0xF051
#define SLSI_WIFI_TAG_VD_T2LM_INFO 0xF052
#define SLSI_WIFI_TAG_VD_T2LM_FRAME_TYPE 0xF053
#define SLSI_WIFI_TAG_VD_TX_STATUS 0xF054
#define SLSI_WIFI_TAG_VD_BT_DEVICE_STATUS 0xF055
#define SLSI_WIFI_TAG_VD_REASON 0xF00D

/*ToDO: remove when fapi update is done*/
#define FAPI_EVENT_WIFI_EVENT_BT_COEX_BT_DEVICE_STATUS                0x005F

#define MAX_SSID_LEN 100
#define SLSI_MAX_NUM_RING 10

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifdef CONFIG_SCSC_WLAN_ENHANCED_LOGGING
static int mem_dump_buffer_size;
static char *mem_dump_buffer;
#endif
#ifdef CONFIG_SCSC_WLAN_DEBUG
char *slsi_print_event_name(int event_id)
{
	switch (event_id) {
	case SLSI_NL80211_SCAN_RESULTS_AVAILABLE_EVENT:
		return "SCAN_RESULTS_AVAILABLE_EVENT";
	case SLSI_NL80211_FULL_SCAN_RESULT_EVENT:
		return "FULL_SCAN_RESULT_EVENT";
	case SLSI_NL80211_SCAN_EVENT:
		return "BUCKET_SCAN_DONE_EVENT";
#ifdef CONFIG_SCSC_WLAN_KEY_MGMT_OFFLOAD
	case SLSI_NL80211_VENDOR_SUBCMD_KEY_MGMT_ROAM_AUTH:
		return "KEY_MGMT_ROAM_AUTH";
#endif
	case SLSI_NL80211_VENDOR_HANGED_EVENT:
		return "SLSI_NL80211_VENDOR_HANGED_EVENT";
	case SLSI_NL80211_EPNO_EVENT:
		return "SLSI_NL80211_EPNO_EVENT";
	case SLSI_NL80211_HOTSPOT_MATCH:
		return "SLSI_NL80211_HOTSPOT_MATCH";
	case SLSI_NL80211_RSSI_REPORT_EVENT:
		return "SLSI_NL80211_RSSI_REPORT_EVENT";
#ifdef CONFIG_SCSC_WLAN_ENHANCED_LOGGING
	case SLSI_NL80211_LOGGER_RING_EVENT:
		return "SLSI_NL80211_LOGGER_RING_EVENT";
	case SLSI_NL80211_LOGGER_FW_DUMP_EVENT:
		return "SLSI_NL80211_LOGGER_FW_DUMP_EVENT";
#endif
	case SLSI_NL80211_NAN_RESPONSE_EVENT:
		return "SLSI_NL80211_NAN_RESPONSE_EVENT";
	case SLSI_NL80211_NAN_PUBLISH_TERMINATED_EVENT:
		return "SLSI_NL80211_NAN_PUBLISH_TERMINATED_EVENT";
	case SLSI_NL80211_NAN_MATCH_EVENT:
		return "SLSI_NL80211_NAN_MATCH_EVENT";
	case SLSI_NL80211_NAN_MATCH_EXPIRED_EVENT:
		return "SLSI_NL80211_NAN_MATCH_EXPIRED_EVENT";
	case SLSI_NL80211_NAN_SUBSCRIBE_TERMINATED_EVENT:
		return "SLSI_NL80211_NAN_SUBSCRIBE_TERMINATED_EVENT";
	case SLSI_NL80211_NAN_FOLLOWUP_EVENT:
		return "SLSI_NL80211_NAN_FOLLOWUP_EVENT";
#ifdef CONFIG_SCSC_WIFI_NAN_PAIRING
	case SLSI_NL80211_NAN_BOOTSTRAPPING_REQ:
		return "SLSI_NL80211_NAN_BOOTSTRAPPING_REQ";
	case SLSI_NL80211_NAN_BOOTSTRAPPING_CFM:
		return "SLSI_NL80211_NAN_BOOTSTRAPPING_CFM";
	case SLSI_NL80211_NAN_PAIRING_REQ:
		return "SLSI_NL80211_NAN_PAIRING_REQ";
	case SLSI_NL80211_NAN_PAIRING_CFM:
		return "SLSI_NL80211_NAN_PAIRING_CFM";
#endif
	case SLSI_NL80211_NAN_DISCOVERY_ENGINE_EVENT:
		return "SLSI_NL80211_NAN_DISCOVERY_ENGINE_EVENT";
	case SLSI_NL80211_RTT_RESULT_EVENT:
		return "SLSI_NL80211_RTT_RESULT_EVENT";
	case SLSI_NL80211_RTT_COMPLETE_EVENT:
		return "SLSI_NL80211_RTT_COMPLETE_EVENT";
	case SLSI_NL80211_VENDOR_ACS_EVENT:
		return "SLSI_NL80211_VENDOR_ACS_EVENT";
	case SLSI_NL80211_NAN_TRANSMIT_FOLLOWUP_STATUS:
		return "SLSI_NL80211_NAN_TRANSMIT_FOLLOWUP_STATUS";
	case SLSI_NAN_EVENT_NDP_REQ:
		return "SLSI_NAN_EVENT_NDP_REQ";
	case SLSI_NAN_EVENT_NDP_CFM:
		return "SLSI_NAN_EVENT_NDP_CFM";
	case SLSI_NAN_EVENT_NDP_END:
		return "SLSI_NAN_EVENT_NDP_END";
	case SLSI_NL80211_SUBSYSTEM_RESTART_EVENT:
		return "SLSI_NL80211_SUBSYSTEM_RESTART_EVENT";
	case SLSI_NL80211_NAN_INTERFACE_CREATED_EVENT:
		return "SLSI_NL80211_NAN_INTERFACE_CREATED_EVENT";
	case SLSI_NL80211_NAN_INTERFACE_DELETED_EVENT:
		return "SLSI_NL80211_NAN_INTERFACE_DELETED_EVENT";
	default:
		return "UNKNOWN_EVENT";
	}
}
#endif

int slsi_vendor_event(struct slsi_dev *sdev, int event_id, const void *data, int len)
{
	struct sk_buff *skb;
	int            ret = 0;

#ifdef CONFIG_SCSC_WLAN_DEBUG
	SLSI_DBG1_NODEV(SLSI_MLME, "Event: %s(%d), data = %p, len = %d\n",
			slsi_print_event_name(event_id), event_id, data, len);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, NULL, len, event_id, GFP_KERNEL);
#else
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, len, event_id, GFP_KERNEL);
#endif
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for vendor event: %d\n", event_id);
		return -ENOMEM;
	}

	ret = nla_put_nohdr(skb, len, data);
	if (ret) {
		SLSI_ERR(sdev, "Error in nla_put*:0x%x\n", ret);
		kfree_skb(skb);
		return -EINVAL;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	return 0;
}

static int slsi_vendor_cmd_lls_reply(struct wiphy *wiphy, const void *data, int len, u32 version, bool is_mlo)
{
	struct sk_buff     *skb;
	int                ret = 0;

	if (len <= 0)
		return -EINVAL;

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, len);
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb\n");
		return -ENOMEM;
	}

	if (version == 1) {
#ifdef CONFIG_SCSC_WLAN_EHT
		ret = nla_put_u32(skb, LLS_ATTRIBUTE_GET_STATS_TYPE, (is_mlo ? 2 : 1));
		if (!ret)
			ret |= nla_put(skb, LLS_ATTRIBUTE_GET_STATS_STRUCT, len, data);
#else
		ret = nla_put_nohdr(skb, len, data);
#endif
	} else {
		ret = nla_put_nohdr(skb, len, data);
	}

	if (ret) {
		SLSI_ERR_NODEV("Error in nla_put_*:0x%x\n", ret);
		kfree_skb(skb);
		return -EINVAL;
	}

	return cfg80211_vendor_cmd_reply(skb);
}

static int slsi_vendor_cmd_reply(struct wiphy *wiphy, const void *data, int len)
{
	struct sk_buff     *skb;
	int                ret = 0;

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, len);
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb\n");
		return -ENOMEM;
	}

	ret = nla_put_nohdr(skb, len, data);
	if (ret) {
		SLSI_ERR_NODEV("Error in nla_put_*:0x%x\n", ret);
		kfree_skb(skb);
		return -EINVAL;
	}
	return cfg80211_vendor_cmd_reply(skb);
}

static struct net_device *slsi_gscan_get_netdev(struct slsi_dev *sdev)
{
	return slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
}

static struct netdev_vif *slsi_gscan_get_vif(struct slsi_dev *sdev)
{
	struct net_device *dev;

	dev = slsi_gscan_get_netdev(sdev);
	if (!dev) {
		SLSI_WARN_NODEV("Dev is NULL\n");
		return NULL;
	}

	return netdev_priv(dev);
}

int slsi_number_digits(int num)
{
	int dig = 0;

	while (num) {
		dig++;
		num = num / 10;
	}

	return dig;
}

char *slsi_print_channel_list(int channel_list[], int channel_count)
{
	int i, slen = 0;
	int max_size;
	char *string;

	if (!channel_count)
		max_size = 2; /* '0' + terminating null character(1) = 2 */
	else
		max_size = (channel_count * 4) + 1; /* channel max characters length(3)+space(1) = 4 */

	string = kmalloc(max_size, GFP_KERNEL);

	if (!string) {
		SLSI_ERR_NODEV("Failed to allocate channel string\n");
		return string;
	}

	if (!channel_count) {
		snprintf(string, max_size, "%d", 0);
		return string;
	}

	for (i = 0; i < channel_count && slen < max_size; i++)
		slen += snprintf(&string[slen], max_size - slen, "%d ", channel_list[i]);

	return string;
}

#ifdef CONFIG_SCSC_WLAN_DEBUG
static void slsi_gscan_add_dump_params(struct slsi_nl_gscan_param *nl_gscan_param)
{
	int i;
	int j;

	SLSI_DBG2_NODEV(SLSI_GSCAN, "Parameters for SLSI_NL80211_VENDOR_SUBCMD_ADD_GSCAN sub-command:\n");
	SLSI_DBG2_NODEV(SLSI_GSCAN, "base_period: %d max_ap_per_scan: %d report_threshold_percent: %d report_threshold_num_scans = %d num_buckets: %d\n",
			nl_gscan_param->base_period, nl_gscan_param->max_ap_per_scan,
			nl_gscan_param->report_threshold_percent, nl_gscan_param->report_threshold_num_scans,
			nl_gscan_param->num_buckets);

	for (i = 0; i < nl_gscan_param->num_buckets; i++) {
		SLSI_DBG2_NODEV(SLSI_GSCAN, "Bucket: %d\n", i);
		SLSI_DBG2_NODEV(SLSI_GSCAN, "\tbucket_index: %d band: %d period: %d report_events: %d num_channels: %d\n",
				nl_gscan_param->nl_bucket[i].bucket_index, nl_gscan_param->nl_bucket[i].band,
				nl_gscan_param->nl_bucket[i].period, nl_gscan_param->nl_bucket[i].report_events,
				nl_gscan_param->nl_bucket[i].num_channels);

		for (j = 0; j < nl_gscan_param->nl_bucket[i].num_channels; j++)
			SLSI_DBG2_NODEV(SLSI_GSCAN, "\tchannel_list[%d]: %d\n",
					j, nl_gscan_param->nl_bucket[i].channels[j].channel);
	}
}

void slsi_gscan_scan_res_dump(struct slsi_gscan_result *scan_data)
{
	struct slsi_nl_scan_result_param *nl_scan_res = &scan_data->nl_scan_res;

	SLSI_DBG3_NODEV(SLSI_GSCAN, "TS:%llu SSID:%s BSSID:" MACSTR " Chan:%d RSSI:%d Bcn_Int:%d Capab:%#x IE_Len:%d\n",
			nl_scan_res->ts, nl_scan_res->ssid, MAC2STR(nl_scan_res->bssid), nl_scan_res->channel,
			nl_scan_res->rssi, nl_scan_res->beacon_period, nl_scan_res->capability, nl_scan_res->ie_length);

	SLSI_DBG_HEX_NODEV(SLSI_GSCAN, &nl_scan_res->ie_data[0], nl_scan_res->ie_length, "IE_Data:\n");
	if (scan_data->anqp_length) {
		SLSI_DBG3_NODEV(SLSI_GSCAN, "ANQP_LENGTH:%d\n", scan_data->anqp_length);
		SLSI_DBG_HEX_NODEV(SLSI_GSCAN, nl_scan_res->ie_data + nl_scan_res->ie_length, scan_data->anqp_length, "ANQP_info:\n");
	}
}
#endif

static int slsi_gscan_get_capabilities(struct wiphy *wiphy,
				       struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_nl_gscan_capabilities nl_cap;
	int                               ret = 0;
	struct slsi_dev                   *sdev = SDEV_FROM_WIPHY(wiphy);

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_GET_GSCAN_CAPABILITIES\n");
	if (!slsi_dev_gscan_supported())
		return -ENOTSUPP;

	memset(&nl_cap, 0, sizeof(struct slsi_nl_gscan_capabilities));

	ret = slsi_mib_get_gscan_cap(sdev, &nl_cap);
	if (ret != 0) {
		SLSI_ERR(sdev, "Failed to read mib\n");
		return ret;
	}

	nl_cap.max_scan_cache_size = SLSI_GSCAN_MAX_SCAN_CACHE_SIZE;
	nl_cap.max_ap_cache_per_scan = SLSI_GSCAN_MAX_AP_CACHE_PER_SCAN;
	nl_cap.max_scan_reporting_threshold = SLSI_GSCAN_MAX_SCAN_REPORTING_THRESHOLD;

	ret = slsi_vendor_cmd_reply(wiphy, &nl_cap, sizeof(struct slsi_nl_gscan_capabilities));
	if (ret)
		SLSI_ERR_NODEV("gscan_get_capabilities vendor cmd reply failed (err = %d)\n", ret);

	return ret;
}

static u32 slsi_gscan_put_channels(struct ieee80211_supported_band *chan_data, bool no_dfs, bool only_dfs, u32 *buf)
{
	u32 chan_count = 0;
	u32 chan_flags;
	int i;

	if (chan_data == NULL) {
		SLSI_DBG3_NODEV(SLSI_GSCAN, "Band not supported\n");
		return 0;
	}
	chan_flags = (IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_NO_OFDM | IEEE80211_CHAN_RADAR);

	for (i = 0; i < chan_data->n_channels; i++) {
		if (chan_data->channels[i].flags & IEEE80211_CHAN_DISABLED)
			continue;
		if (only_dfs) {
			if (chan_data->channels[i].flags & chan_flags)
				buf[chan_count++] = chan_data->channels[i].center_freq;
			continue;
		}
		if (no_dfs && (chan_data->channels[i].flags & chan_flags))
			continue;
		buf[chan_count++] = chan_data->channels[i].center_freq;
	}
	return chan_count;
}

static int slsi_gscan_get_valid_channel(struct wiphy *wiphy,
					struct wireless_dev *wdev, const void *data, int len)
{
	int             ret = 0, type, band;
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	u32             *chan_list;
	u32             chan_count = 0, mem_len = 0;
	struct sk_buff  *reply;

	if (len < SLSI_NL_VENDOR_DATA_OVERHEAD || !data)
		return -EINVAL;

	type = nla_type(data);

	if (type == GSCAN_ATTRIBUTE_BAND) {
		if (slsi_util_nla_get_u32((struct nlattr *)data, (u32 *)(&band)))
			return -EINVAL;
	} else
		return -EINVAL;

	if (band == 0) {
		SLSI_WARN(sdev, "NO Bands. return 0 channel\n");
		return ret;
	}

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	SLSI_DBG3(sdev, SLSI_GSCAN, "band %d\n", band);
	if (wiphy->bands[NL80211_BAND_2GHZ])
		mem_len += wiphy->bands[NL80211_BAND_2GHZ]->n_channels * sizeof(u32);

	if (wiphy->bands[NL80211_BAND_5GHZ])
		mem_len += wiphy->bands[NL80211_BAND_5GHZ]->n_channels * sizeof(u32);

	if (mem_len == 0) {
		ret = -ENOTSUPP;
		goto exit;
	}

	chan_list = kmalloc(mem_len, GFP_KERNEL);
	if (chan_list == NULL) {
		ret = -ENOMEM;
		goto exit;
	}
	mem_len += SLSI_NL_VENDOR_REPLY_OVERHEAD + (SLSI_NL_ATTRIBUTE_U32_LEN * 2);
	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_len);
	if (reply == NULL) {
		ret = -ENOMEM;
		goto exit_with_chan_list;
	}
	switch (band) {
	case WIFI_BAND_BG:
		chan_count = slsi_gscan_put_channels(wiphy->bands[NL80211_BAND_2GHZ], false, false, chan_list);
		break;
	case WIFI_BAND_A:
		chan_count = slsi_gscan_put_channels(wiphy->bands[NL80211_BAND_5GHZ], true, false, chan_list);
		break;
	case WIFI_BAND_A_DFS:
		chan_count = slsi_gscan_put_channels(wiphy->bands[NL80211_BAND_5GHZ], false, true, chan_list);
		break;
	case WIFI_BAND_A_WITH_DFS:
		chan_count = slsi_gscan_put_channels(wiphy->bands[NL80211_BAND_5GHZ], false, false, chan_list);
		break;
	case WIFI_BAND_ABG:
		chan_count = slsi_gscan_put_channels(wiphy->bands[NL80211_BAND_2GHZ], true, false, chan_list);
		chan_count += slsi_gscan_put_channels(wiphy->bands[NL80211_BAND_5GHZ], true, false, chan_list + chan_count);
		break;
	case WIFI_BAND_ABG_WITH_DFS:
		chan_count = slsi_gscan_put_channels(wiphy->bands[NL80211_BAND_2GHZ], false, false, chan_list);
		chan_count += slsi_gscan_put_channels(wiphy->bands[NL80211_BAND_5GHZ], false, false, chan_list + chan_count);
		break;
	default:
		chan_count = 0;
		SLSI_WARN(sdev, "Invalid Band %d\n", band);
	}
	ret |= nla_put_u32(reply, GSCAN_ATTRIBUTE_NUM_CHANNELS, chan_count);
	ret |= nla_put(reply, GSCAN_ATTRIBUTE_CHANNEL_LIST, chan_count * sizeof(u32), chan_list);
	if (ret) {
		SLSI_ERR(sdev, "Error in nla_put*:0x%x\n", ret);
		kfree_skb(reply);
		goto exit_with_chan_list;
	}
	ret =  cfg80211_vendor_cmd_reply(reply);

	if (ret)
		SLSI_ERR(sdev, "FAILED to reply GET_VALID_CHANNELS\n");

exit_with_chan_list:
	kfree(chan_list);
exit:
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
	return ret;
}

struct slsi_gscan_result *slsi_prepare_scan_result(struct sk_buff *skb, u16 anqp_length, int hs2_id)
{
	struct ieee80211_mgmt    *mgmt = fapi_get_mgmt(skb);
	struct slsi_gscan_result *scan_res;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
	struct timespec64		 ts;
#else
	struct timespec		     ts;
#endif
	const u8                 *ssid_ie;
	int                      mem_reqd;
	int                      ie_len = 0;
	u8                       *ie;

	ie = &mgmt->u.beacon.variable[0];
	ie_len = fapi_get_datalen(skb) - (ie - (u8 *)mgmt) - anqp_length;

	if (ie_len <= 0) {
		SLSI_ERR_NODEV("invalid ie_len : %d\n", ie_len);
		return NULL;
	}

	/* Exclude 1 byte for ie_data[1]. sizeof(u16) to include anqp_length, sizeof(int) for hs_id */
	mem_reqd = (sizeof(struct slsi_gscan_result) - 1) + ie_len + anqp_length + sizeof(int) + sizeof(u16);

	/* Allocate memory for scan result */
	scan_res = kmalloc(mem_reqd, GFP_KERNEL);
	if (scan_res == NULL) {
		SLSI_ERR_NODEV("Failed to allocate memory for scan result\n");
		return NULL;
	}

	/* Exclude 1 byte for ie_data[1] */
	scan_res->scan_res_len = (sizeof(struct slsi_nl_scan_result_param) - 1) + ie_len;
	scan_res->anqp_length = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
	ts = ktime_to_timespec64(ktime_get_boottime());
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	ts = ktime_to_timespec(ktime_get_boottime());
#else
	get_monotonic_boottime(&ts);
#endif
	scan_res->nl_scan_res.ts = (u64)TIMESPEC_TO_US(ts);

	ssid_ie = cfg80211_find_ie(WLAN_EID_SSID, &mgmt->u.beacon.variable[0], ie_len);
	if ((ssid_ie != NULL) && (ssid_ie[1] > 0) && (ssid_ie[1] < IEEE80211_MAX_SSID_LEN)) {
		memcpy(scan_res->nl_scan_res.ssid, &ssid_ie[2], ssid_ie[1]);
		scan_res->nl_scan_res.ssid[ssid_ie[1]] = '\0';
	} else {
		scan_res->nl_scan_res.ssid[0] = '\0';
	}

	SLSI_ETHER_COPY(scan_res->nl_scan_res.bssid, mgmt->bssid);
	scan_res->nl_scan_res.channel = fapi_get_u16(skb, u.mlme_scan_ind.channel_frequency) / 2;
	scan_res->nl_scan_res.rssi = fapi_get_s16(skb, u.mlme_scan_ind.rssi);
	scan_res->nl_scan_res.rtt = SLSI_GSCAN_RTT_UNSPECIFIED;
	scan_res->nl_scan_res.rtt_sd = SLSI_GSCAN_RTT_UNSPECIFIED;
	scan_res->nl_scan_res.beacon_period = mgmt->u.beacon.beacon_int;
	scan_res->nl_scan_res.capability = mgmt->u.beacon.capab_info;
	scan_res->nl_scan_res.ie_length = ie_len;
	memcpy(scan_res->nl_scan_res.ie_data, ie, ie_len);
	memcpy(scan_res->nl_scan_res.ie_data + ie_len, &hs2_id, sizeof(int));
	memcpy(scan_res->nl_scan_res.ie_data + ie_len + sizeof(int), &anqp_length, sizeof(u16));
	if (anqp_length) {
		memcpy(scan_res->nl_scan_res.ie_data + ie_len + sizeof(u16) + sizeof(int), ie + ie_len, anqp_length);
		scan_res->anqp_length = anqp_length + sizeof(u16) + sizeof(int);
	}

#ifdef CONFIG_SCSC_WLAN_DEBUG
	slsi_gscan_scan_res_dump(scan_res);
#endif

	return scan_res;
}

static void slsi_gscan_hash_add(struct slsi_dev *sdev, struct slsi_gscan_result *scan_res)
{
	u8                key = SLSI_GSCAN_GET_HASH_KEY(scan_res->nl_scan_res.bssid[5]);
	struct netdev_vif *ndev_vif;

	ndev_vif = slsi_gscan_get_vif(sdev);
	if (!SLSI_MUTEX_IS_LOCKED(ndev_vif->scan_mutex))
		SLSI_WARN_NODEV("ndev_vif->scan_mutex is not locked\n");

	scan_res->hnext = sdev->gscan_hash_table[key];
	sdev->gscan_hash_table[key] = scan_res;

	/* Update the total buffer consumed and number of scan results */
	sdev->buffer_consumed += scan_res->scan_res_len;
	sdev->num_gscan_results++;
}

static struct slsi_gscan_result *slsi_gscan_hash_get(struct slsi_dev *sdev, u8 *mac)
{
	struct slsi_gscan_result *temp;
	struct netdev_vif        *ndev_vif;
	u8                       key = SLSI_GSCAN_GET_HASH_KEY(mac[5]);

	ndev_vif = slsi_gscan_get_vif(sdev);

	if (!SLSI_MUTEX_IS_LOCKED(ndev_vif->scan_mutex))
		SLSI_WARN_NODEV("ndev_vif->scan_mutex is not locked\n");

	temp = sdev->gscan_hash_table[key];
	while (temp != NULL) {
		if (memcmp(temp->nl_scan_res.bssid, mac, ETH_ALEN) == 0)
			return temp;
		temp = temp->hnext;
	}

	return NULL;
}

void slsi_gscan_hash_remove(struct slsi_dev *sdev, u8 *mac)
{
	u8                       key = SLSI_GSCAN_GET_HASH_KEY(mac[5]);
	struct slsi_gscan_result *curr;
	struct slsi_gscan_result *prev;
	struct netdev_vif        *ndev_vif;
	struct slsi_gscan_result *scan_res = NULL;

	ndev_vif = slsi_gscan_get_vif(sdev);
	if (!SLSI_MUTEX_IS_LOCKED(ndev_vif->scan_mutex))
		SLSI_WARN_NODEV("ndev_vif->scan_mutex is not locked\n");

	if (sdev->gscan_hash_table[key] == NULL)
		return;

	if (memcmp(sdev->gscan_hash_table[key]->nl_scan_res.bssid, mac, ETH_ALEN) == 0) {
		scan_res = sdev->gscan_hash_table[key];
		sdev->gscan_hash_table[key] = sdev->gscan_hash_table[key]->hnext;
	} else {
		prev = sdev->gscan_hash_table[key];
		curr = prev->hnext;

		while (curr != NULL) {
			if (memcmp(curr->nl_scan_res.bssid, mac, ETH_ALEN) == 0) {
				scan_res = curr;
				prev->hnext = curr->hnext;
				break;
			}
			prev = curr;
			curr = curr->hnext;
		}
	}

	if (scan_res) {
		/* Update the total buffer consumed and number of scan results */
		sdev->buffer_consumed -= scan_res->scan_res_len;
		sdev->num_gscan_results--;
		kfree(scan_res);
	}

	if (sdev->num_gscan_results < 0)
		SLSI_ERR(sdev, "Wrong num_gscan_results: %d\n", sdev->num_gscan_results);
}

int slsi_check_scan_result(struct slsi_dev *sdev, struct slsi_bucket *bucket, struct slsi_gscan_result *new_scan_res)
{
	struct slsi_gscan_result *scan_res;

	/* Check if the scan result for the same BSS already exists in driver buffer */
	scan_res = slsi_gscan_hash_get(sdev, new_scan_res->nl_scan_res.bssid);
	if (scan_res == NULL) { /* New scan result */
		if ((sdev->buffer_consumed + new_scan_res->scan_res_len) >= SLSI_GSCAN_MAX_SCAN_CACHE_SIZE) {
			SLSI_DBG2(sdev, SLSI_GSCAN,
				  "Scan buffer full, discarding scan result, buffer_consumed = %d, buffer_threshold = %d\n",
				  sdev->buffer_consumed, sdev->buffer_threshold);

			/* Scan buffer is full can't store anymore new results */
			return SLSI_DISCARD_SCAN_RESULT;
		}

		return SLSI_KEEP_SCAN_RESULT;
	}

	/* Even if scan buffer is full existing results can be replaced with the latest one */
	if (scan_res->scan_cycle == bucket->scan_cycle)
		/* For the same scan cycle the result will be replaced only if the RSSI is better */
		if (new_scan_res->nl_scan_res.rssi < scan_res->nl_scan_res.rssi)
			return SLSI_DISCARD_SCAN_RESULT;

	/* Remove the existing scan result */
	slsi_gscan_hash_remove(sdev, scan_res->nl_scan_res.bssid);

	return SLSI_KEEP_SCAN_RESULT;
}

void slsi_gscan_handle_scan_result(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb, u16 scan_id, bool scan_done)
{
	struct slsi_gscan_result *scan_res = NULL;
	struct netdev_vif        *ndev_vif = netdev_priv(dev);
	struct slsi_bucket       *bucket;
	u16                      bucket_index;
	int                      event_type = WIFI_SCAN_FAILED;
	u16                      anqp_length = 0;
	int                      hs2_network_id;

	if (!SLSI_MUTEX_IS_LOCKED(ndev_vif->scan_mutex))
		SLSI_WARN_NODEV("ndev_vif->scan_mutex is not locked\n");

	SLSI_NET_DBG_HEX(dev, SLSI_GSCAN, skb->data, skb->len, "mlme_scan_ind skb->len: %d\n", skb->len);

	bucket_index = scan_id - SLSI_GSCAN_SCAN_ID_START;
	if (bucket_index >= SLSI_GSCAN_MAX_BUCKETS) {
		SLSI_NET_ERR(dev, "Invalid bucket index: %d (scan_id = %#x)\n", bucket_index, scan_id);
		goto out;
	}

	bucket = &sdev->bucket[bucket_index];
	if (!bucket->used) {
		SLSI_NET_DBG1(dev, SLSI_GSCAN, "Bucket is not active, index: %d (scan_id = %#x)\n", bucket_index, scan_id);
		goto out;
	}

	/* For scan_done indication - no need to store the results */
	if (scan_done) {
		bucket->scan_cycle++;
		bucket->gscan->num_scans++;

		SLSI_NET_DBG3(dev, SLSI_GSCAN, "scan done, scan_cycle = %d, num_scans = %d\n",
			      bucket->scan_cycle, bucket->gscan->num_scans);

		if (bucket->report_events & SLSI_REPORT_EVENTS_EACH_SCAN)
			event_type = WIFI_SCAN_RESULTS_AVAILABLE;
		if (bucket->gscan->num_scans % bucket->gscan->report_threshold_num_scans == 0)
			event_type = WIFI_SCAN_THRESHOLD_NUM_SCANS;
		if (sdev->buffer_consumed >= sdev->buffer_threshold)
			event_type = WIFI_SCAN_THRESHOLD_PERCENT;

		if (event_type != WIFI_SCAN_FAILED)
			slsi_vendor_event(sdev, SLSI_NL80211_SCAN_EVENT, &event_type, sizeof(event_type));

		goto out;
	}

	//anqp_length = fapi_get_u16(skb, u.mlme_scan_ind.anqp_elements_length);
	/* TODO new FAPI 3.c has mlme_scan_ind.network_block_id, use that when fapi is updated. */
	hs2_network_id = 1;

	scan_res = slsi_prepare_scan_result(skb, anqp_length, hs2_network_id);
	if (scan_res == NULL) {
		SLSI_NET_ERR(dev, "Failed to prepare scan result\n");
		goto out;
	}
#if 0
	/* Check for ePNO networks */
	if (fapi_get_u16(skb, u.mlme_scan_ind.preferrednetwork_ap)) {
		if (anqp_length == 0)
			slsi_vendor_event(sdev, SLSI_NL80211_EPNO_EVENT,
					  &scan_res->nl_scan_res, scan_res->scan_res_len);
		else
			slsi_vendor_event(sdev, SLSI_NL80211_HOTSPOT_MATCH,
					  &scan_res->nl_scan_res, scan_res->scan_res_len + scan_res->anqp_length);
	}
#endif
	if (bucket->report_events & SLSI_REPORT_EVENTS_FULL_RESULTS) {
		struct sk_buff *nlevent;

		SLSI_NET_DBG3(dev, SLSI_GSCAN, "report_events: SLSI_REPORT_EVENTS_FULL_RESULTS\n");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	nlevent = cfg80211_vendor_event_alloc(sdev->wiphy, NULL, scan_res->scan_res_len + 4, SLSI_NL80211_FULL_SCAN_RESULT_EVENT, GFP_KERNEL);
#else
	nlevent = cfg80211_vendor_event_alloc(sdev->wiphy, scan_res->scan_res_len + 4, SLSI_NL80211_FULL_SCAN_RESULT_EVENT, GFP_KERNEL);
#endif
		if (!nlevent) {
			SLSI_ERR(sdev, "failed to allocate sbk of size:%d\n", scan_res->scan_res_len + 4);
			kfree(scan_res);
			goto out;
		}
		if (nla_put_u32(nlevent, GSCAN_ATTRIBUTE_SCAN_BUCKET_BIT, (1 << bucket_index)) ||
		    nla_put(nlevent, GSCAN_ATTRIBUTE_SCAN_RESULTS, scan_res->scan_res_len, &scan_res->nl_scan_res)) {
			SLSI_ERR(sdev, "failed to put data\n");
			kfree_skb(nlevent);
			kfree(scan_res);
			goto out;
		}
		cfg80211_vendor_event(nlevent, GFP_KERNEL);
	}

	if (slsi_check_scan_result(sdev, bucket, scan_res) == SLSI_DISCARD_SCAN_RESULT) {
		kfree(scan_res);
		goto out;
	 }
	slsi_gscan_hash_add(sdev, scan_res);

out:
	kfree_skb(skb);
}

u8 slsi_gscan_get_scan_policy(enum wifi_band band)
{
	u8 scan_policy;

	switch (band) {
	case WIFI_BAND_UNSPECIFIED:
		scan_policy = FAPI_SCANPOLICY_ANY_RA;
		break;
	case WIFI_BAND_BG:
		scan_policy = FAPI_SCANPOLICY_2_4GHZ;
		break;
	case WIFI_BAND_A:
		scan_policy = (FAPI_SCANPOLICY_5GHZ |
			       FAPI_SCANPOLICY_NON_DFS);
		break;
	case WIFI_BAND_A_DFS:
		scan_policy = (FAPI_SCANPOLICY_5GHZ |
			       FAPI_SCANPOLICY_DFS);
		break;
	case WIFI_BAND_A_WITH_DFS:
		scan_policy = (FAPI_SCANPOLICY_5GHZ |
			       FAPI_SCANPOLICY_NON_DFS |
			       FAPI_SCANPOLICY_DFS);
		break;
	case WIFI_BAND_ABG:
		scan_policy = (FAPI_SCANPOLICY_5GHZ |
			       FAPI_SCANPOLICY_NON_DFS |
			       FAPI_SCANPOLICY_2_4GHZ);
		break;
	case WIFI_BAND_ABG_WITH_DFS:
		scan_policy = (FAPI_SCANPOLICY_5GHZ |
			       FAPI_SCANPOLICY_NON_DFS |
			       FAPI_SCANPOLICY_DFS |
			       FAPI_SCANPOLICY_2_4GHZ);
		break;
	default:
		scan_policy = FAPI_SCANPOLICY_ANY_RA;
		break;
	}

	SLSI_DBG2_NODEV(SLSI_GSCAN, "Scan Policy: %#x\n", scan_policy);

	return scan_policy;
}

static int slsi_gscan_add_read_params(struct slsi_nl_gscan_param *nl_gscan_param, const void *data, int len)
{
	int                         j = 0;
	int                         type, tmp, tmp1, tmp2, k = 0;
	const struct nlattr         *iter, *iter1, *iter2;
	struct slsi_nl_bucket_param *nl_bucket;
	u32 val = 0;

	nla_for_each_attr(iter, data, len, tmp) {
		if (!iter)
			return -EINVAL;

		type = nla_type(iter);

		if (j >= SLSI_GSCAN_MAX_BUCKETS)
			break;

		switch (type) {
		case GSCAN_ATTRIBUTE_BASE_PERIOD:
			if (slsi_util_nla_get_u32(iter, &nl_gscan_param->base_period))
				return -EINVAL;
			break;
		case GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN:
			if (slsi_util_nla_get_u32(iter, &nl_gscan_param->max_ap_per_scan))
				return -EINVAL;
			break;
		case GSCAN_ATTRIBUTE_REPORT_THRESHOLD:
			if (slsi_util_nla_get_u32(iter, &nl_gscan_param->report_threshold_percent))
				return -EINVAL;
			break;
		case GSCAN_ATTRIBUTE_REPORT_THRESHOLD_NUM_SCANS:
			if (slsi_util_nla_get_u32(iter, &nl_gscan_param->report_threshold_num_scans))
				return -EINVAL;
			break;
		case GSCAN_ATTRIBUTE_NUM_BUCKETS:
			if (slsi_util_nla_get_u32(iter, &nl_gscan_param->num_buckets))
				return -EINVAL;
			break;
		case GSCAN_ATTRIBUTE_CH_BUCKET_1:
		case GSCAN_ATTRIBUTE_CH_BUCKET_2:
		case GSCAN_ATTRIBUTE_CH_BUCKET_3:
		case GSCAN_ATTRIBUTE_CH_BUCKET_4:
		case GSCAN_ATTRIBUTE_CH_BUCKET_5:
		case GSCAN_ATTRIBUTE_CH_BUCKET_6:
		case GSCAN_ATTRIBUTE_CH_BUCKET_7:
		case GSCAN_ATTRIBUTE_CH_BUCKET_8:
			nla_for_each_nested(iter1, iter, tmp1) {
				if (!iter1)
					return -EINVAL;

				type = nla_type(iter1);

				nl_bucket = nl_gscan_param->nl_bucket;

				switch (type) {
				case GSCAN_ATTRIBUTE_BUCKET_ID:
					if (slsi_util_nla_get_u32(iter1, &(nl_bucket[j].bucket_index)))
						return -EINVAL;
					break;
				case GSCAN_ATTRIBUTE_BUCKET_PERIOD:
					if (slsi_util_nla_get_u32(iter1, &(nl_bucket[j].period)))
						return -EINVAL;
					break;
				case GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS:
					if (slsi_util_nla_get_u32(iter1, &(nl_bucket[j].num_channels)))
						return -EINVAL;
					break;
				case GSCAN_ATTRIBUTE_BUCKET_CHANNELS:
					nla_for_each_nested(iter2, iter1, tmp2) {
						if (k >= SLSI_GSCAN_MAX_CHANNELS)
							break;
						if (slsi_util_nla_get_u32(iter2, &(nl_bucket[j].channels[k].channel)))
							return -EINVAL;
						k++;
					}
					k = 0;
					break;
				case GSCAN_ATTRIBUTE_BUCKETS_BAND:
					if (slsi_util_nla_get_u32(iter1, &(nl_bucket[j].band)))
						return -EINVAL;
					break;
				case GSCAN_ATTRIBUTE_REPORT_EVENTS:
					if (slsi_util_nla_get_u32(iter1, &val))
						return -EINVAL;
					nl_bucket[j].report_events = (u8)val;
					break;
				case GSCAN_ATTRIBUTE_BUCKET_EXPONENT:
					if (slsi_util_nla_get_u32(iter1, &(nl_bucket[j].exponent)))
						return -EINVAL;
					break;
				case GSCAN_ATTRIBUTE_BUCKET_STEP_COUNT:
					if (slsi_util_nla_get_u32(iter1, &(nl_bucket[j].step_count)))
						return -EINVAL;
					break;
				case GSCAN_ATTRIBUTE_BUCKET_MAX_PERIOD:
					if (slsi_util_nla_get_u32(iter1, &(nl_bucket[j].max_period)))
						return -EINVAL;
					break;
				default:
					SLSI_ERR_NODEV("No ATTR_BUKTS_type - 0x%x\n", type);
					break;
				}
			}
			j++;
			break;
		default:
			SLSI_ERR_NODEV("No GSCAN_ATTR_CH_BUKT_type - 0x%x\n", type);
			break;
		}
	}

	return 0;
}

int slsi_gscan_add_verify_params(struct slsi_nl_gscan_param *nl_gscan_param)
{
	int i;

	if ((nl_gscan_param->max_ap_per_scan < 0) || (nl_gscan_param->max_ap_per_scan > SLSI_GSCAN_MAX_AP_CACHE_PER_SCAN)) {
		SLSI_ERR_NODEV("Invalid max_ap_per_scan: %d\n", nl_gscan_param->max_ap_per_scan);
		return -EINVAL;
	}

	if ((nl_gscan_param->report_threshold_percent < 0) || (nl_gscan_param->report_threshold_percent > SLSI_GSCAN_MAX_SCAN_REPORTING_THRESHOLD)) {
		SLSI_ERR_NODEV("Invalid report_threshold_percent: %d\n", nl_gscan_param->report_threshold_percent);
		return -EINVAL;
	}

	if ((nl_gscan_param->num_buckets <= 0) || (nl_gscan_param->num_buckets > SLSI_GSCAN_MAX_BUCKETS)) {
		SLSI_ERR_NODEV("Invalid num_buckets: %d\n", nl_gscan_param->num_buckets);
		return -EINVAL;
	}

	for (i = 0; i < nl_gscan_param->num_buckets; i++) {
		if ((nl_gscan_param->nl_bucket[i].band == WIFI_BAND_UNSPECIFIED) && (nl_gscan_param->nl_bucket[i].num_channels == 0)) {
			SLSI_ERR_NODEV("No band/channels provided for gscan: band = %d, num_channel = %d\n",
				       nl_gscan_param->nl_bucket[i].band, nl_gscan_param->nl_bucket[i].num_channels);
			return -EINVAL;
		}

		if (nl_gscan_param->nl_bucket[i].report_events > 4) {
			SLSI_ERR_NODEV("Unsupported report event: report_event = %d\n", nl_gscan_param->nl_bucket[i].report_events);
			return -EINVAL;
		}
	}

	return 0;
}

void slsi_gscan_add_to_list(struct slsi_gscan **sdev_gscan, struct slsi_gscan *gscan)
{
	gscan->next = *sdev_gscan;
	*sdev_gscan = gscan;
}

int slsi_gscan_alloc_buckets(struct slsi_dev *sdev, struct slsi_gscan *gscan, int num_buckets)
{
	int i;
	int bucket_index = 0;
	int free_buckets = 0;

	for (i = 0; i < SLSI_GSCAN_MAX_BUCKETS; i++)
		if (!sdev->bucket[i].used)
			free_buckets++;

	if (num_buckets > free_buckets) {
		SLSI_ERR_NODEV("Not enough free buckets, num_buckets = %d, free_buckets = %d\n",
			       num_buckets, free_buckets);
		return -EINVAL;
	}

	/* Allocate free buckets for the current gscan */
	for (i = 0; i < SLSI_GSCAN_MAX_BUCKETS; i++)
		if (!sdev->bucket[i].used) {
			sdev->bucket[i].used = true;
			sdev->bucket[i].gscan = gscan;
			gscan->bucket[bucket_index] = &sdev->bucket[i];
			bucket_index++;
			if (bucket_index == num_buckets)
				break;
		}

	return 0;
}

static void slsi_gscan_free_buckets(struct slsi_gscan *gscan)
{
	struct slsi_bucket *bucket;
	int                i;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "gscan = %p, num_buckets = %d\n", gscan, gscan->num_buckets);

	for (i = 0; i < gscan->num_buckets; i++) {
		bucket = gscan->bucket[i];

		SLSI_DBG2_NODEV(SLSI_GSCAN, "bucket = %p, used = %d, report_events = %d, scan_id = %#x, gscan = %p\n",
				bucket, bucket->used, bucket->report_events, bucket->scan_id, bucket->gscan);
		if (bucket->used) {
			bucket->used = false;
			bucket->report_events = 0;
			bucket->gscan = NULL;
		}
	}
}

void slsi_gscan_flush_scan_results(struct slsi_dev *sdev)
{
	struct netdev_vif        *ndev_vif;
	struct slsi_gscan_result *temp;
	int                      i;

	ndev_vif = slsi_gscan_get_vif(sdev);
	if (!ndev_vif) {
		SLSI_WARN_NODEV("ndev_vif is NULL");
		return;
	}

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);
	for (i = 0; i < SLSI_GSCAN_HASH_TABLE_SIZE; i++)
		while (sdev->gscan_hash_table[i]) {
			temp = sdev->gscan_hash_table[i];
			sdev->gscan_hash_table[i] = sdev->gscan_hash_table[i]->hnext;
			sdev->num_gscan_results--;
			sdev->buffer_consumed -= temp->scan_res_len;
			kfree(temp);
		}

	SLSI_DBG2(sdev, SLSI_GSCAN, "num_gscan_results: %d, buffer_consumed = %d\n",
		  sdev->num_gscan_results, sdev->buffer_consumed);

	if (sdev->num_gscan_results != 0)
		SLSI_WARN_NODEV("sdev->num_gscan_results is not zero\n");

	if (sdev->buffer_consumed != 0)
		SLSI_WARN_NODEV("sdev->buffer_consumedis not zero\n");

	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
}

static int slsi_gscan_add_mlme(struct slsi_dev *sdev, struct slsi_nl_gscan_param *nl_gscan_param, struct slsi_gscan *gscan)
{
	struct slsi_gscan_param      gscan_param;
	struct net_device            *dev;
	int                          ret = 0;
	int                          i;
#ifdef CONFIG_SCSC_WLAN_ENABLE_MAC_RANDOMISATION
	int j;
	u8 mac_addr_mask[ETH_ALEN];
#endif

	dev = slsi_gscan_get_netdev(sdev);

	if (!dev) {
		SLSI_WARN_NODEV("dev is NULL\n");
		return -EINVAL;
	}

	for (i = 0; i < nl_gscan_param->num_buckets; i++) {
		u16 report_mode = 0;

		gscan_param.nl_bucket = &nl_gscan_param->nl_bucket[i]; /* current bucket */
		gscan_param.bucket = gscan->bucket[i];

		if (gscan_param.bucket->report_events) {
			if (gscan_param.bucket->report_events & SLSI_REPORT_EVENTS_EACH_SCAN)
				report_mode |= FAPI_REPORTMODE_END_OF_SCAN_CYCLE;
			if (gscan_param.bucket->report_events & SLSI_REPORT_EVENTS_FULL_RESULTS)
				report_mode |= FAPI_REPORTMODE_REAL_TIME;
			if (gscan_param.bucket->report_events & SLSI_REPORT_EVENTS_NO_BATCH)
				report_mode |= FAPI_REPORTMODE_NO_BATCH;
		} else {
			report_mode = FAPI_REPORTMODE_RESERVED;
		}

		if (report_mode == 0) {
			SLSI_NET_ERR(dev, "Invalid report event value: %d\n", gscan_param.bucket->report_events);
			return -EINVAL;
		}

		/* In case of epno no_batch mode should be set. */
		if (sdev->epno_active)
			report_mode |= FAPI_REPORTMODE_NO_BATCH;

#ifdef CONFIG_SCSC_WLAN_ENABLE_MAC_RANDOMISATION
		memset(mac_addr_mask, 0xFF, ETH_ALEN);
		if (sdev->scan_addr_set == 1) {
			for (j = 3; j < ETH_ALEN; j++)
				mac_addr_mask[j] = 0x00;
			ret = slsi_set_mac_randomisation_mask(sdev, mac_addr_mask);
			if (ret)
				sdev->scan_addr_set = 0;
		} else
			slsi_set_mac_randomisation_mask(sdev, mac_addr_mask);
#endif
		ret = slsi_mlme_add_scan(sdev,
					 dev,
					 FAPI_SCANTYPE_GSCAN,
					 report_mode,
					 0,     /* n_ssids */
					 NULL,  /* ssids */
					 nl_gscan_param->nl_bucket[i].num_channels,
					 NULL,  /* ieee80211_channel */
					 &gscan_param,
					 NULL,  /* ies */
					 0,     /* ies_len */
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
					 0,
					 NULL,
					 0,
#endif
					 false /* wait_for_ind */);

		if (ret != 0) {
			SLSI_NET_ERR(dev, "Failed to add bucket: %d\n", i);

			/* Delete the scan those are already added */
			for (i = (i - 1); i >= 0; i--)
				slsi_mlme_del_scan(sdev, dev, gscan->bucket[i]->scan_id, false);
			break;
		}
	}

	return ret;
}

static int slsi_gscan_add(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int                        ret = 0;
	struct slsi_dev            *sdev = SDEV_FROM_WIPHY(wiphy);
	struct slsi_nl_gscan_param *nl_gscan_param = NULL;
	struct slsi_gscan          *gscan;
	struct netdev_vif          *ndev_vif;
	int                        buffer_threshold;
	int                        i;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_ADD_GSCAN\n");

	if (!sdev) {
		SLSI_WARN_NODEV("sdev is NULL\n");
		return -EINVAL;
	}

	if (!slsi_dev_gscan_supported())
		return -ENOTSUPP;

	ndev_vif = slsi_gscan_get_vif(sdev);

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);
	/* Allocate memory for the received scan params */
	nl_gscan_param = kzalloc(sizeof(*nl_gscan_param), GFP_KERNEL);
	if (nl_gscan_param == NULL) {
		SLSI_ERR_NODEV("Failed for allocate memory for gscan params\n");
		ret = -ENOMEM;
		goto exit;
	}

	slsi_gscan_add_read_params(nl_gscan_param, data, len);

#ifdef CONFIG_SCSC_WLAN_DEBUG
	slsi_gscan_add_dump_params(nl_gscan_param);
#endif

	ret = slsi_gscan_add_verify_params(nl_gscan_param);
	if (ret) {
		/* After adding a hotlist a new gscan is added with 0 buckets - return success */
		if (nl_gscan_param->num_buckets == 0) {
			kfree(nl_gscan_param);
			SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
			return 0;
		}

		goto exit;
	}

	/* Allocate Memory for the new gscan */
	gscan = kzalloc(sizeof(*gscan), GFP_KERNEL);
	if (gscan == NULL) {
		SLSI_ERR_NODEV("Failed to allocate memory for gscan\n");
		ret = -ENOMEM;
		goto exit;
	}

	gscan->num_buckets = nl_gscan_param->num_buckets;
	gscan->report_threshold_percent = nl_gscan_param->report_threshold_percent;
	gscan->report_threshold_num_scans = nl_gscan_param->report_threshold_num_scans;
	gscan->nl_bucket = nl_gscan_param->nl_bucket[0];

	/* If multiple gscan is added; consider the lowest report_threshold_percent */
	buffer_threshold = (SLSI_GSCAN_MAX_SCAN_CACHE_SIZE * nl_gscan_param->report_threshold_percent) / 100;
	if ((sdev->buffer_threshold == 0) || (buffer_threshold < sdev->buffer_threshold))
		sdev->buffer_threshold = buffer_threshold;

	ret = slsi_gscan_alloc_buckets(sdev, gscan, nl_gscan_param->num_buckets);
	if (ret)
		goto exit_with_gscan_free;

	for (i = 0; i < nl_gscan_param->num_buckets; i++)
		gscan->bucket[i]->report_events = nl_gscan_param->nl_bucket[i].report_events;

	ret = slsi_gscan_add_mlme(sdev, nl_gscan_param, gscan);
	if (ret) {
		/* Free the buckets */
		slsi_gscan_free_buckets(gscan);

		goto exit_with_gscan_free;
	}

	slsi_gscan_add_to_list(&sdev->gscan, gscan);

	goto exit;

exit_with_gscan_free:
	kfree(gscan);
exit:
	kfree(nl_gscan_param);

	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
	return ret;
}

static int slsi_gscan_del(struct wiphy *wiphy,
			  struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev;
	struct netdev_vif *ndev_vif;
	struct slsi_gscan *gscan;
	int               ret = 0;
	int               i;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_DEL_GSCAN\n");

	dev = slsi_gscan_get_netdev(sdev);
	if (!dev) {
		SLSI_WARN_NODEV("dev is NULL\n");
		return -EINVAL;
	}

	ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);
	while (sdev->gscan != NULL) {
		gscan = sdev->gscan;

		SLSI_DBG3(sdev, SLSI_GSCAN, "gscan = %p, num_buckets = %d\n", gscan, gscan->num_buckets);

		for (i = 0; i < gscan->num_buckets; i++)
			if (gscan->bucket[i]->used)
				slsi_mlme_del_scan(sdev, dev, gscan->bucket[i]->scan_id, false);
		slsi_gscan_free_buckets(gscan);
		sdev->gscan = gscan->next;
		kfree(gscan);
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);

	slsi_gscan_flush_scan_results(sdev);

	sdev->buffer_threshold = 0;

	return ret;
}

static int slsi_gscan_get_scan_results(struct wiphy *wiphy,
				       struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev          *sdev = SDEV_FROM_WIPHY(wiphy);
	struct sk_buff           *skb;
	struct slsi_gscan_result *scan_res;
	struct nlattr            *scan_hdr;
	struct netdev_vif        *ndev_vif;
	int                      num_results = 0;
	int                      mem_needed;
	const struct nlattr      *attr;
	int                      nl_num_results = 0;
	int                      ret = 0;
	int                      temp;
	int                      type;
	int                      i;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_GET_SCAN_RESULTS\n");

	/* Read the number of scan results need to be given */
	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);

		switch (type) {
		case GSCAN_ATTRIBUTE_NUM_OF_RESULTS:
			if (slsi_util_nla_get_u32(attr, &nl_num_results))
				return -EINVAL;
			break;
		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			break;
		}
	}

	ndev_vif = slsi_gscan_get_vif(sdev);
	if (!ndev_vif) {
		SLSI_WARN_NODEV("ndev_vif is NULL\n");
		return -EINVAL;
	}

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);

	num_results = sdev->num_gscan_results;

	SLSI_DBG3(sdev, SLSI_GSCAN, "nl_num_results: %d, num_results = %d\n", nl_num_results, sdev->num_gscan_results);

	if (num_results == 0) {
		SLSI_DBG1(sdev, SLSI_GSCAN, "No scan results available\n");
		/* Return value should be 0 for this scenario */
		goto exit;
	}

	/* Find the number of results to return */
	if (num_results > nl_num_results)
		num_results = nl_num_results;

	/* 12 bytes additional for scan_id, flags and num_resuls */
	mem_needed = num_results * sizeof(struct slsi_nl_scan_result_param) + 12;

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (skb == NULL) {
		SLSI_ERR_NODEV("skb alloc failed");
		ret = -ENOMEM;
		goto exit;
	}

	scan_hdr = nla_nest_start(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS);
	if (scan_hdr == NULL) {
		kfree_skb(skb);
		SLSI_ERR_NODEV("scan_hdr is NULL.\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret |= nla_put_u32(skb, GSCAN_ATTRIBUTE_SCAN_ID, 0);
	ret |= nla_put_u8(skb, GSCAN_ATTRIBUTE_SCAN_FLAGS, 0);
	ret |= nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_OF_RESULTS, num_results);

	for (i = 0; i < SLSI_GSCAN_HASH_TABLE_SIZE; i++)
		while (sdev->gscan_hash_table[i]) {
			scan_res = sdev->gscan_hash_table[i];
			sdev->gscan_hash_table[i] = sdev->gscan_hash_table[i]->hnext;
			sdev->num_gscan_results--;
			sdev->buffer_consumed -= scan_res->scan_res_len;
			/* TODO: If IE is included then HAL is not able to parse the results */
			ret |= nla_put(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS, sizeof(struct slsi_nl_scan_result_param), &scan_res->nl_scan_res);
			kfree(scan_res);
			num_results--;
			if (num_results == 0)
				goto out;
		}
out:
	nla_nest_end(skb, scan_hdr);
	if (ret) {
		SLSI_ERR(sdev, "Error in nla_put*:0x%x\n", ret);
		kfree_skb(skb);
		goto exit;
	}
	ret = cfg80211_vendor_cmd_reply(skb);
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
	return ret;
}

void slsi_rx_rssi_report_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	struct slsi_rssi_monitor_evt event_data;
	u8 *bssid;

	bssid = fapi_get_buff(skb, u.mlme_rssi_report_ind.bssid);
	if (!bssid) {
		SLSI_ERR(sdev, "bssid is NULL.\n");
		kfree_skb(skb);
		return;
	}
	SLSI_ETHER_COPY(event_data.bssid, bssid);
	event_data.rssi = fapi_get_s16(skb, u.mlme_rssi_report_ind.rssi);
	SLSI_DBG3(sdev, SLSI_GSCAN, "RSSI threshold breached, Current RSSI for " MACSTR "= %d\n",
		  MAC2STR(event_data.bssid), event_data.rssi);
	slsi_vendor_event(sdev, SLSI_NL80211_RSSI_REPORT_EVENT, &event_data, sizeof(event_data));
	kfree_skb(skb);
}

static int slsi_set_vendor_ie(struct wiphy *wiphy,
			      struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device   *net_dev;
	struct netdev_vif   *ndev_vif;
	const struct nlattr *attr;
	int                 r = 0;
	int                 temp;
	int                 type;
	u8                  *ie_list = NULL;
	int                 ie_list_len = 0;

	net_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	ndev_vif = netdev_priv(net_dev);

	SLSI_INFO(sdev, "Vendor CMD SCAN_DEFAULT_IES\n");
	nla_for_each_attr(attr, data, len, temp) {
		if (!attr)
			return -EINVAL;
		type = nla_type(attr);
		switch (type) {
		case SLSI_SCAN_DEFAULT_IES:
		{
			if (!nla_len(attr))
				break;
			kfree(ie_list);
			ie_list =  kmalloc(nla_len(attr), GFP_KERNEL);
			if (!ie_list) {
				SLSI_ERR(sdev, "No memory for ie_list!");
				return -ENOMEM;
			}
			memcpy(ie_list, nla_data(attr), nla_len(attr));
			ie_list_len = nla_len(attr);
			SLSI_INFO(sdev, "SCAN_DEFAULT_IES Len:%d\n", ie_list_len);
			break;
		}
		default:
			if (type > SLSI_SCAN_DEFAULT_MAX)
				SLSI_ERR(sdev, "Invalid type : %d\n", type);
			break;
		}
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	kfree(sdev->default_scan_ies);
	sdev->default_scan_ies_len = ie_list_len;
	sdev->default_scan_ies = (u8 *)ie_list;
	if (ndev_vif->activated)
		r = slsi_add_probe_ies_request(sdev, net_dev);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

#ifdef CONFIG_SCSC_WLAN_KEY_MGMT_OFFLOAD
static int slsi_key_mgmt_set_pmk(struct wiphy *wiphy,
				 struct wireless_dev *wdev, const void *pmk, int pmklen)
{
	struct slsi_dev    *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device  *net_dev;
	struct netdev_vif  *ndev_vif;
	int r = 0;
	struct key_params params;
	u8                mac_addr[ETH_ALEN] = {0};

	if (wdev->iftype == NL80211_IFTYPE_P2P_CLIENT) {
		SLSI_DBG3(sdev, SLSI_GSCAN, "Not required to set PMK for P2P client\n");
		return r;
	}
	SLSI_DBG3(sdev, SLSI_GSCAN, "SUBCMD_SET_PMK Received\n");

	net_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	ndev_vif = netdev_priv(net_dev);
	memset(&params, 0, sizeof(params));
	memset(mac_addr, 0x00, ETH_ALEN);
	params.key = pmk;
	params.key_len = pmklen;
	params.cipher = WLAN_CIPHER_SUITE_PMK;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	r = slsi_mlme_set_key(sdev, net_dev, 0, FAPI_KEYTYPE_PMK, mac_addr, &params);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}
#endif

static int slsi_set_bssid_blacklist(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev    *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device  *net_dev;
	struct netdev_vif *ndev_vif;
	int                      temp1;
	int                      type;
	const struct nlattr      *attr;
	u32 num_bssids = 0;
	u8 i = 0;
	u8 from_supplicant = 0;
	int ret = 0;
	u8 *bssid = NULL;
	struct cfg80211_acl_data *acl_data = NULL;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_SET_BSSID_BLACK_LIST\n");

	net_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	if (!net_dev) {
		SLSI_WARN_NODEV("net_dev is NULL\n");
		return -EINVAL;
	}

	ndev_vif = netdev_priv(net_dev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	nla_for_each_attr(attr, data, len, temp1) {
		if (!attr) {
			SLSI_ERR_NODEV("Attribute is null : len = %d\n", len);
			ret = -EINVAL;
			break;
		}

		type = nla_type(attr);

		switch (type) {
		case GSCAN_ATTRIBUTE_NUM_BSSID:
			if (acl_data)
				break;

			if (slsi_util_nla_get_u32(attr, &num_bssids)) {
				ret = -EINVAL;
				goto exit;
			}
			if (num_bssids > (u32)((ULONG_MAX - sizeof(*acl_data)) / (sizeof(struct mac_address)))) {
				ret = -EINVAL;
				goto exit;
			}
			acl_data = kmalloc(sizeof(*acl_data) + (sizeof(struct mac_address) * num_bssids), GFP_KERNEL);
			if (!acl_data) {
				ret = -ENOMEM;
				goto exit;
			}
			acl_data->n_acl_entries = num_bssids;
			break;

		case GSCAN_ATTRIBUTE_BLACKLIST_BSSID:
			if (!acl_data) {
				ret = -EINVAL;
				goto exit;
			}

			if (nla_len(attr) != 6) { /*Attribute length should be equal to length of mac address which is 6 bytes.*/
				ret = -EINVAL;
				goto exit;
			}

			if (i >= num_bssids) {
				ret = -EINVAL;
				goto exit;
			}

			if (nla_len(attr) < ETH_ALEN) {
				ret = -EINVAL;
				goto exit;
			}
			bssid = (u8 *)nla_data(attr);

			SLSI_ETHER_COPY(acl_data->mac_addrs[i].addr, bssid);
			SLSI_DBG3_NODEV(SLSI_GSCAN, "mac_addrs[%d]:" MACSTR ")\n", i,
					MAC2STR(acl_data->mac_addrs[i].addr));
			i++;
			break;
		case GSCAN_ATTRIBUTE_BLACKLIST_FROM_SUPPLICANT:
			from_supplicant = 1;
			break;
		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	if (acl_data) {
		if (from_supplicant) {
			kfree(ndev_vif->acl_data_supplicant);
			ndev_vif->acl_data_supplicant = acl_data;
		} else {
			kfree(ndev_vif->acl_data_hal);
			ndev_vif->acl_data_hal = acl_data;
		}
		ret = slsi_set_acl(sdev, net_dev);
		acl_data = NULL;
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	kfree(acl_data);
	return ret;
}

static int slsi_start_keepalive_offload(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
#ifndef CONFIG_SCSC_WLAN_NAT_KEEPALIVE_DISABLE
	struct slsi_dev    *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device  *net_dev;
	struct netdev_vif *ndev_vif;

	int                      temp;
	int                      type;
	const struct nlattr      *attr;
	u16 ip_pkt_len = 0;
	u16 ip_pkt_size = 0;
	u8 *ip_pkt = NULL, *src_mac_addr = NULL, *dst_mac_addr = NULL;
	u32 period = 0;
	struct slsi_peer *peer;
	struct sk_buff *skb;
	struct ethhdr *ehdr;
	int r = 0;
	u16 host_tag = 0;
	u8 index = 0;

	SLSI_DBG3(sdev, SLSI_MLME, "SUBCMD_START_KEEPALIVE_OFFLOAD received\n");
	net_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	ndev_vif = netdev_priv(net_dev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (!ndev_vif->activated) {
		SLSI_WARN_NODEV("ndev_vif is not activated\n");
		r = -EINVAL;
		goto exit;
	}
	if (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) {
		SLSI_WARN_NODEV("ndev_vif->vif_type is not FAPI_VIFTYPE_STATION\n");
		r = -EINVAL;
		goto exit;
	}
	if (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED) {
		SLSI_WARN_NODEV("ndev_vif->sta.vif_status is not SLSI_VIF_STATUS_CONNECTED\n");
		r = -EINVAL;
		goto exit;
	}

	peer = slsi_get_peer_from_qs(sdev, net_dev, SLSI_STA_PEER_QUEUESET);
	if (!peer) {
		SLSI_WARN_NODEV("peer is NULL\n");
		r = -EINVAL;
		goto exit;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);

		switch (type) {
		case MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN:
			if (slsi_util_nla_get_u16(attr, &ip_pkt_len)) {
				r = -EINVAL;
				goto exit;
			}
			break;

		case MKEEP_ALIVE_ATTRIBUTE_IP_PKT:
			if (nla_len(attr) < ip_pkt_len) {
				 r = -EINVAL;
				 goto exit;
			}
			ip_pkt_size = nla_len(attr);
			ip_pkt = (u8 *)nla_data(attr);
			break;

		case MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC:
			if (slsi_util_nla_get_u32(attr, &period)) {
				r = -EINVAL;
				goto exit;
			}
			break;

		case MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR:
			if (nla_len(attr) < ETH_ALEN) {
				r = -EINVAL;
				goto exit;
			}
			dst_mac_addr = (u8 *)nla_data(attr);
			break;

		case MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR:
			if (nla_len(attr) < ETH_ALEN) {
				r = -EINVAL;
				goto exit;
			}
			src_mac_addr = (u8 *)nla_data(attr);
			break;

		case MKEEP_ALIVE_ATTRIBUTE_ID:
			if (slsi_util_nla_get_u8(attr, &index)) {
				r = -EINVAL;
				goto exit;
			}
			if (index > SLSI_MAX_KEEPALIVE_ID) {
				r = -EINVAL;
				goto exit;
			}
			break;

		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			r = -EINVAL;
			goto exit;
		}
	}
	if (!index) {
		SLSI_WARN_NODEV("No MKEEP_ALIVE_ATTRIBUTE_ID\n");
		r = -EINVAL;
		goto exit;
	}

	/* Stop any existing request. This may fail if no request exists
	  * so ignore the return value
	  */
	if (!slsi_mlme_send_frame_mgmt(sdev, net_dev, NULL, 0,
				       FAPI_DATAUNITDESCRIPTOR_IEEE802_3_FRAME,
				       FAPI_MESSAGETYPE_ANY_OTHER,
				       ndev_vif->sta.keepalive_host_tag[index - 1], 0, 0, 0))
		SLSI_DBG3(sdev, SLSI_MLME, "slsi_mlme_send_frame_mgmt returned failure\n");
	skb = alloc_skb(SLSI_NETIF_SKB_HEADROOM + SLSI_NETIF_SKB_TAILROOM + sizeof(struct ethhdr) + ip_pkt_len, GFP_KERNEL);
	if (!skb) {
		SLSI_WARN_NODEV("memory allocation failed for skb (size: %d)\n", SLSI_NETIF_SKB_HEADROOM + SLSI_NETIF_SKB_TAILROOM + sizeof(struct ethhdr) + ip_pkt_len);
		r = -ENOMEM;
		goto exit;
	}

	skb_reserve(skb, SLSI_NETIF_SKB_HEADROOM - SLSI_SKB_GET_ALIGNMENT_OFFSET(skb));
	skb_reset_mac_header(skb);
	skb_set_network_header(skb, sizeof(struct ethhdr));

	/* Ethernet Header */
	ehdr = (struct ethhdr *)skb_put(skb, sizeof(struct ethhdr));

	if (dst_mac_addr)
		SLSI_ETHER_COPY(ehdr->h_dest, dst_mac_addr);
	if (src_mac_addr)
		SLSI_ETHER_COPY(ehdr->h_source, src_mac_addr);
	ehdr->h_proto = cpu_to_be16(ETH_P_IP);
	if (ip_pkt && (ip_pkt_size >= ip_pkt_len)) {
		memcpy(skb_put(skb, ip_pkt_len), ip_pkt, ip_pkt_len);
	} else {
		kfree_skb(skb);
		r = -EINVAL;
		goto exit;
	}


	skb->dev = net_dev;
	skb->protocol = ETH_P_IP;
	skb->ip_summed = CHECKSUM_UNNECESSARY;

	/* Queueset 0 AC 0 */
	skb->queue_mapping = slsi_netif_get_peer_queue(0, 0);

	/* Enabling the "Don't Fragment" Flag in the IP Header */
	ip_hdr(skb)->frag_off |= htons(IP_DF);

	/* Calculation of IP header checksum */
	ip_hdr(skb)->check = 0;
	ip_send_check(ip_hdr(skb));

	host_tag = slsi_tx_mgmt_host_tag(sdev);

	r = slsi_mlme_send_frame_data(sdev, net_dev, skb, FAPI_MESSAGETYPE_ANY_OTHER, host_tag,
				      0, (period * 1000));
	if (r == 0)
		ndev_vif->sta.keepalive_host_tag[index - 1] = host_tag;
	else
		kfree_skb(skb);

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
#else
	SLSI_DBG3_NODEV(SLSI_MLME, "SUBCMD_START_KEEPALIVE_OFFLOAD received\n");
	SLSI_DBG3_NODEV(SLSI_MLME, "NAT Keep Alive Feature is disabled\n");
	return -EOPNOTSUPP;

#endif
}

static int slsi_stop_keepalive_offload(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
#ifndef CONFIG_SCSC_WLAN_NAT_KEEPALIVE_DISABLE
	struct slsi_dev    *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device  *net_dev;
	struct netdev_vif *ndev_vif;
	int r = 0;
	int                      temp;
	int                      type;
	const struct nlattr      *attr;
	u8 index = 0;

	SLSI_DBG3(sdev, SLSI_MLME, "SUBCMD_STOP_KEEPALIVE_OFFLOAD received\n");
	net_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	ndev_vif = netdev_priv(net_dev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (!ndev_vif->activated) {
		SLSI_WARN(sdev, "VIF is not activated\n");
		r = -EINVAL;
		goto exit;
	}
	if (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) {
		SLSI_WARN(sdev, "Not a STA VIF\n");
		r = -EINVAL;
		goto exit;
	}
	if (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED) {
		SLSI_WARN(sdev, "VIF is not connected\n");
		r = -EINVAL;
		goto exit;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);

		switch (type) {
		case MKEEP_ALIVE_ATTRIBUTE_ID:
			if (slsi_util_nla_get_u8(attr, &index)) {
				r = -EINVAL;
				goto exit;
			}
			if (index > SLSI_MAX_KEEPALIVE_ID) {
				r = -EINVAL;
				goto exit;
			}
			break;

		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			r = -EINVAL;
			goto exit;
		}
	}
	if (!index) {
		SLSI_WARN_NODEV("No MKEEP_ALIVE_ATTRIBUTE_ID\n");
		r = -EINVAL;
		goto exit;
	}

	r = slsi_mlme_send_frame_mgmt(sdev, net_dev, NULL, 0, FAPI_DATAUNITDESCRIPTOR_IEEE802_3_FRAME,
				      FAPI_MESSAGETYPE_ANY_OTHER, ndev_vif->sta.keepalive_host_tag[index - 1], 0, 0, 0);
	ndev_vif->sta.keepalive_host_tag[index - 1] = 0;

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
#else
	SLSI_DBG3_NODEV(SLSI_MLME, "SUBCMD_STOP_KEEPALIVE_OFFLOAD received\n");
	SLSI_DBG3_NODEV(SLSI_MLME, "NAT Keep Alive Feature is disabled\n");
	return -EOPNOTSUPP;

#endif
}

static inline int slsi_epno_ssid_list_get(struct slsi_dev *sdev,
					  struct slsi_epno_ssid_param *epno_ssid_params, const struct nlattr *outer)
{
	SLSI_DBG3_NODEV(SLSI_GSCAN, "SUBCMD_GET_EPNO_LIST Received\n");
	return -EOPNOTSUPP;
}

static int slsi_set_epno_ssid(struct wiphy *wiphy,
			      struct wireless_dev *wdev, const void *data, int len)
{
	SLSI_DBG3_NODEV(SLSI_GSCAN, "SUBCMD_SET_EPNO_LIST Received\n");
	return -EOPNOTSUPP;
}

static int slsi_set_hs_params(struct wiphy *wiphy,
			      struct wireless_dev *wdev, const void *data, int len)
{
	SLSI_DBG3_NODEV(SLSI_GSCAN, "SUBCMD_SET_HS_LIST Received\n");
	return -EOPNOTSUPP;
}

static int slsi_reset_hs_params(struct wiphy *wiphy,
				struct wireless_dev *wdev, const void *data, int len)
{
	SLSI_DBG3_NODEV(SLSI_GSCAN, "SUBCMD_RESET_HS_LIST Received\n");
	return -EOPNOTSUPP;
}

static int slsi_set_rssi_monitor(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev            *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device          *net_dev;
	struct netdev_vif          *ndev_vif;
	int                        r = 0;
	int                      temp;
	int                      type;
	const struct nlattr      *attr;
	s8 min_rssi = 0, max_rssi = 0;
	u16 enable = 0;
	u8 val = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "Recd RSSI monitor command\n");

	net_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	if (net_dev == NULL) {
		SLSI_ERR(sdev, "netdev is NULL!!\n");
		return -ENODEV;
	}

	ndev_vif = netdev_priv(net_dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!ndev_vif->activated) {
		SLSI_ERR(sdev, "Vif not activated\n");
		r = -EINVAL;
		goto exit;
	}
	if (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) {
		SLSI_ERR(sdev, "Not a STA vif\n");
		r = -EINVAL;
		goto exit;
	}
	if (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED) {
		SLSI_ERR(sdev, "STA vif not connected\n");
		r = -EINVAL;
		goto exit;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_RSSI_MONITOR_ATTRIBUTE_START:
			if (slsi_util_nla_get_u8(attr, &val)) {
				r = -EINVAL;
				goto exit;
			}
			enable = (u16)val;
			break;
		case SLSI_RSSI_MONITOR_ATTRIBUTE_MIN_RSSI:
			if (slsi_util_nla_get_s8(attr, &min_rssi)) {
				r = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_RSSI_MONITOR_ATTRIBUTE_MAX_RSSI:
			if (slsi_util_nla_get_s8(attr, &max_rssi)) {
				r = -EINVAL;
				goto exit;
			}
			break;
		default:
			r = -EINVAL;
			goto exit;
		}
	}
	if (min_rssi > max_rssi) {
		SLSI_ERR(sdev, "Invalid params, min_rssi= %d ,max_rssi = %d\n", min_rssi, max_rssi);
		r = -EINVAL;
		goto exit;
	}
	r = slsi_mlme_set_rssi_monitor(sdev, net_dev, enable, min_rssi, max_rssi);
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

static int slsi_lls_set_stats(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                 temp;
	int                 type;
	const struct nlattr *attr;
	u32                 mpdu_size_threshold = 0;
	u32                 aggr_stat_gathering = 0;

	if (!slsi_dev_lls_supported())
		return -EOPNOTSUPP;

	if (slsi_is_test_mode_enabled()) {
		SLSI_WARN(sdev, "not supported in WlanLite mode\n");
		return -EOPNOTSUPP;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);

		switch (type) {
		case LLS_ATTRIBUTE_SET_MPDU_SIZE_THRESHOLD:
			if (slsi_util_nla_get_u32(attr, &mpdu_size_threshold))
				return -EINVAL;
			break;

		case LLS_ATTRIBUTE_SET_AGGR_STATISTICS_GATHERING:
			if (slsi_util_nla_get_u32(attr, &aggr_stat_gathering))
				return -EINVAL;
			break;

		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			return -EINVAL;
		}
	}

	slsi_lls_start_stats(sdev, mpdu_size_threshold, aggr_stat_gathering);
	return 0;
}

static int slsi_lls_clear_stats(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev          *sdev = SDEV_FROM_WIPHY(wiphy);
	int                      temp;
	int                      type;
	const struct nlattr      *attr;
	u32                      stats_clear_req_mask = 0;
	u32                      stop_req             = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);

		switch (type) {
		case LLS_ATTRIBUTE_CLEAR_STOP_REQUEST_MASK:
			if (slsi_util_nla_get_u32(attr, &stats_clear_req_mask))
				return -EINVAL;
			SLSI_DBG3(sdev, SLSI_GSCAN, "stats_clear_req_mask:%u\n", stats_clear_req_mask);
			break;

		case LLS_ATTRIBUTE_CLEAR_STOP_REQUEST:
			if (slsi_util_nla_get_u32(attr, &stop_req))
				return -EINVAL;
			SLSI_DBG3(sdev, SLSI_GSCAN, "stop_req:%u\n", stop_req);
			break;

		default:
			SLSI_ERR(sdev, "Unknown attribute:%d\n", type);
			return -EINVAL;
		}
	}

	/* stop_req = 0 : clear the stats which are flaged 0
	 * stop_req = 1 : clear the stats which are flaged 1
	 */
	if (!stop_req)
		stats_clear_req_mask = ~stats_clear_req_mask;

	slsi_lls_stop_stats(sdev, stats_clear_req_mask);
	return 0;
}

static int slsi_lls_get_stats(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev        *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device      *dev = wdev->netdev;
	struct netdev_vif      *ndev_vif;
	int                    ret;
	u8                     *buf = NULL;
	int                    buf_len;
	const struct nlattr    *attr;
	int                    temp;
	int                    type;
	u32                    lls_version = 0;
	bool                   is_mlo = false;

	if (!slsi_dev_lls_supported())
		return -EOPNOTSUPP;

	if (slsi_is_test_mode_enabled()) {
		SLSI_WARN(sdev, "not supported in WlanLite mode\n");
		return -EOPNOTSUPP;
	}

	if (!sdev) {
		SLSI_ERR(sdev, "sdev is Null\n");
		return -EINVAL;
	}

	/* In case of lower layer failure do not read LLS MIBs */
	if (sdev->mlme_blocked)
		return -EIO;

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);

		switch (type) {
		case LLS_ATTRIBUTE_STATS_VERSION:
			if (slsi_util_nla_get_u32(attr, &lls_version))
				return -EINVAL;
			break;

		default:
			SLSI_WARN_NODEV("Unknown attribute: %d\n", type);
		}
	}

	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}

	ndev_vif = netdev_priv(dev);
#ifdef CONFIG_SCSC_WLAN_EHT
	if (lls_version == 1 && ndev_vif->sta.valid_links)
		is_mlo = true;
#endif

	buf_len = slsi_lls_fill_stats(sdev, &buf, is_mlo);
	ret = slsi_vendor_cmd_lls_reply(wiphy, buf, buf_len, lls_version, is_mlo);
	if (ret)
		SLSI_ERR_NODEV("vendor cmd reply failed (err:%d)\n", ret);

	kfree(buf);
	return ret;
}

static int slsi_gscan_set_oui(struct wiphy *wiphy,
			      struct wireless_dev *wdev, const void *data, int len)
{
	int ret = 0;

#ifdef CONFIG_SCSC_WLAN_ENABLE_MAC_RANDOMISATION

	struct slsi_dev          *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif;
	int                      temp;
	int                      type;
	const struct nlattr      *attr;
	u8 scan_oui[6];

	memset(&scan_oui, 0, 6);

	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}

	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);
	sdev->scan_addr_set = 0;

	nla_for_each_attr(attr, data, len, temp) {
		if (!attr) {
			ret = -EINVAL;
			break;
		}

		type = nla_type(attr);
		switch (type) {
		case SLSI_NL_ATTRIBUTE_PNO_RANDOM_MAC_OUI:
		{
			if (slsi_util_nla_get_data(attr, 3, &scan_oui)) {
				ret = -EINVAL;
				break;
			}
			memcpy(sdev->scan_mac_addr, scan_oui, 6);
			sdev->scan_addr_set = 1;
			break;
		}
		default:
			ret = -EINVAL;
			SLSI_ERR(sdev, "Invalid type : %d\n", type);
			break;
		}
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
#endif
	return ret;
}

static int slsi_get_feature_set(struct wiphy *wiphy,
				struct wireless_dev *wdev, const void *data, int len)
{
	u32 feature_set = 0;
	int ret = 0;
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);

	SLSI_DBG3_NODEV(SLSI_GSCAN, "\n");

	feature_set |= SLSI_WIFI_HAL_FEATURE_INFRA;
	if (sdev->band_5g_supported)
		feature_set |= SLSI_WIFI_HAL_FEATURE_INFRA_5G;
#ifndef CONFIG_SCSC_WLAN_STA_ONLY
	feature_set |= SLSI_WIFI_HAL_FEATURE_P2P;
	feature_set |= SLSI_WIFI_HAL_FEATURE_SOFT_AP;
#endif
	feature_set |= SLSI_WIFI_HAL_FEATURE_RSSI_MONITOR;
	feature_set |= SLSI_WIFI_HAL_FEATURE_CONTROL_ROAMING;
	feature_set |= SLSI_WIFI_HAL_FEATURE_TDLS;
	if (sdev->tdls_offchannel)
		feature_set |= SLSI_WIFI_HAL_FEATURE_TDLS_OFFCHANNEL;
	feature_set |= SLSI_WIFI_HAL_FEATURE_HOTSPOT;
#ifndef CONFIG_SCSC_WLAN_NAT_KEEPALIVE_DISABLE
	feature_set |= SLSI_WIFI_HAL_FEATURE_MKEEP_ALIVE;
#endif
#ifdef CONFIG_SCSC_WLAN_ENHANCED_LOGGING
		feature_set |= SLSI_WIFI_HAL_FEATURE_LOGGER;
#endif
	if (slsi_dev_gscan_supported())
		feature_set |= SLSI_WIFI_HAL_FEATURE_GSCAN;
	if (slsi_dev_lls_supported())
		feature_set |= SLSI_WIFI_HAL_FEATURE_LINK_LAYER_STATS;
	if (slsi_dev_epno_supported())
		feature_set |= SLSI_WIFI_HAL_FEATURE_HAL_EPNO;
	if (slsi_dev_nan_supported(SDEV_FROM_WIPHY(wiphy)))
		feature_set |= SLSI_WIFI_HAL_FEATURE_NAN;
	if (slsi_dev_rtt_supported()) {
		feature_set |= SLSI_WIFI_HAL_FEATURE_D2D_RTT;
		feature_set |= SLSI_WIFI_HAL_FEATURE_D2AP_RTT;
	}

	feature_set |= SLSI_WIFI_HAL_FEATURE_BATCH_SCAN;
#if !defined(SCSC_SEP_VERSION) || (defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION >= 11)
	feature_set |= SLSI_WIFI_HAL_FEATURE_PNO;
#endif
#ifdef CONFIG_SCSC_WLAN_WIFI_SHARING
	feature_set |= SLSI_WIFI_HAL_FEATURE_AP_STA;
#endif
	feature_set |= SLSI_WIFI_HAL_FEATURE_CONFIG_NDO;
#ifdef CONFIG_SCSC_WLAN_ENABLE_MAC_RANDOMISATION
	feature_set |= SLSI_WIFI_HAL_FEATURE_SCAN_RAND;
#endif
        feature_set |= SLSI_WIFI_HAL_FEATURE_DYNAMIC_SET_MAC;
	feature_set |= SLSI_WIFI_HAL_FEATURE_LOW_LATENCY;
	feature_set |= SLSI_WIFI_HAL_FEATURE_P2P_RAND_MAC;

	ret = slsi_vendor_cmd_reply(wiphy, &feature_set, sizeof(feature_set));

	return ret;
}

static int slsi_set_country_code(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev          *sdev = SDEV_FROM_WIPHY(wiphy);
	int                      ret = 0;
	int                      temp;
	int                      type;
	const struct nlattr      *attr;
	char country_code[SLSI_COUNTRY_CODE_LEN];

	SLSI_DBG3(sdev, SLSI_GSCAN, "Received country code command\n");

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_NL_ATTRIBUTE_COUNTRY_CODE:
		{
			if (slsi_util_nla_get_data(attr, (SLSI_COUNTRY_CODE_LEN - 1), country_code)) {
				ret = -EINVAL;
				SLSI_ERR(sdev, "Insufficient Country Code Length : %d\n", nla_len(attr));
				return ret;
			}
			break;
		}
		default:
			ret = -EINVAL;
			SLSI_ERR(sdev, "Invalid type : %d\n", type);
			return ret;
		}
	}
	ret = slsi_set_country_update_regd(sdev, country_code, SLSI_COUNTRY_CODE_LEN);
	if (ret < 0)
		SLSI_ERR(sdev, "Set country failed ret:%d\n", ret);
	return ret;
}

static int slsi_apf_read_filter(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int ret = 0;
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u8 *host_dst = NULL;
	int datalen;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_APF_READ_FILTER\n");
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	if (!sdev->device_config.fw_apf_supported) {
		SLSI_WARN(sdev, "APF not supported by the firmware.\n");
		SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -ENOTSUPP;
	}

	ret = slsi_mlme_read_apf_request(sdev, dev, &host_dst, &datalen);
	if (!ret)
		ret = slsi_vendor_cmd_reply(wiphy, host_dst, datalen);

	kfree(host_dst);
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

static int slsi_apf_get_capabilities(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int                               ret = 0;
	struct slsi_dev                   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev = wdev->netdev;
	struct sk_buff *nl_skb;
	struct nlattr *nlattr_start;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_APF_GET_CAPABILITIES\n");
	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	if (!sdev->device_config.fw_apf_supported) {
		SLSI_WARN(sdev, "APF not supported by the firmware.\n");
		ret = -ENOTSUPP;
		goto exit;
	}
	memset(&sdev->device_config.apf_cap, 0, sizeof(struct slsi_apf_capabilities));

	ret = slsi_mib_get_apf_cap(sdev, dev);
	if (ret != 0) {
		SLSI_ERR(sdev, "Failed to read mib\n");
		goto exit;
	}
	SLSI_DBG3(sdev, SLSI_GSCAN, "APF version: %d Max_Length:%d\n", sdev->device_config.apf_cap.version,
		  sdev->device_config.apf_cap.max_length);
	nl_skb = cfg80211_vendor_cmd_alloc_reply_skb(sdev->wiphy, NLMSG_DEFAULT_SIZE);
	if (!nl_skb) {
		SLSI_ERR(sdev, "NO MEM for nl_skb!!!\n");
		ret = -ENOMEM;
		goto exit;
	}

	nlattr_start = nla_nest_start(nl_skb, NL80211_ATTR_VENDOR_DATA);
	if (!nlattr_start) {
		SLSI_ERR(sdev, "failed to put NL80211_ATTR_VENDOR_DATA\n");
		/* Dont use slsi skb wrapper for this free */
		kfree_skb(nl_skb);
		ret = -EINVAL;
		goto exit;
	}

	ret = nla_put_u16(nl_skb, SLSI_APF_ATTR_VERSION, sdev->device_config.apf_cap.version);
	ret |= nla_put_u16(nl_skb, SLSI_APF_ATTR_MAX_LEN, sdev->device_config.apf_cap.max_length);
	if (ret) {
		SLSI_ERR(sdev, "Error in nla_put*:0x%x\n", ret);
		/* Dont use slsi skb wrapper for this free */
		kfree_skb(nl_skb);
		goto exit;
	}

	ret = cfg80211_vendor_cmd_reply(nl_skb);
	if (ret)
		SLSI_ERR(sdev, "apf_get_capabilities cfg80211_vendor_cmd_reply failed :%d\n", ret);
exit:
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	return ret;
}

static int slsi_apf_set_filter(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev          *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif;
	int                      ret = 0;
	int                      temp;
	int                      type;
	const struct nlattr      *attr;
	u32 program_len = 0;
	u8 *program = NULL;

	SLSI_DBG3(sdev, SLSI_GSCAN, "Received apf_set_filter command\n");
	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		ret = -EINVAL;
		return ret;
	}

	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	if (!sdev->device_config.fw_apf_supported) {
		SLSI_WARN(sdev, "APF not supported by the firmware.\n");
		ret = -ENOTSUPP;
		goto exit;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_APF_ATTR_PROGRAM_LEN:
		{
			if (slsi_util_nla_get_u32(attr, &program_len)) {
				ret = -EINVAL;
				goto exit;
			}
			kfree(program);
			program = kmalloc(program_len, GFP_KERNEL);
			if (!program) {
				ret = -ENOMEM;
				goto exit;
			}
			break;
		}
		case SLSI_APF_ATTR_PROGRAM:
		{
			if (!program) {
				SLSI_ERR(sdev, "Program len is not set!\n");
				ret = -EINVAL;
				goto exit;
			}
			if (slsi_util_nla_get_data(attr, program_len, program)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		}
		default:
			SLSI_ERR(sdev, "Invalid type : %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	ret = slsi_mlme_install_apf_request(sdev, dev, program, program_len);
	if (ret < 0) {
		SLSI_ERR(sdev, "apf_set_filter failed ret:%d\n", ret);
		ret = -EINVAL;
		goto exit;
	}
exit:
	kfree(program);
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

static int slsi_rtt_get_capabilities(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_rtt_capabilities rtt_cap;
	int                               ret = 0;
	struct slsi_dev                   *sdev = SDEV_FROM_WIPHY(wiphy);

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_GET_RTT_CAPABILITIES\n");
	if (!slsi_dev_rtt_supported()) {
		SLSI_WARN(sdev, "RTT not supported.\n");
		return -ENOTSUPP;
	}
	memset(&rtt_cap, 0, sizeof(struct slsi_rtt_capabilities));

	ret = slsi_mib_get_rtt_cap(sdev, NULL, &rtt_cap);
	if (ret != 0) {
		SLSI_ERR(sdev, "Failed to read mib\n");
		return ret;
	}
	ret = slsi_vendor_cmd_reply(wiphy, &rtt_cap, sizeof(struct slsi_rtt_capabilities));
	if (ret)
		SLSI_ERR_NODEV("rtt_get_capabilities vendor cmd reply failed (err = %d)\n", ret);
	return ret;
}

static int slsi_rtt_process_target_info(const struct nlattr *iter, struct slsi_rtt_config *nl_rtt_params,
					u8 *rtt_peer, int num_devices)
{
	int j = 0, tmp1, tmp2;
	u16 channel_freq = 0;
	const struct nlattr *outer, *inner;

	nla_for_each_nested(outer, iter, tmp1) {
		nla_for_each_nested(inner, outer, tmp2) {
			switch (nla_type(inner)) {
			case SLSI_RTT_ATTRIBUTE_TARGET_MAC:
				if (slsi_util_nla_get_data(inner, ETH_ALEN, nl_rtt_params[j].peer_addr))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_TYPE:
				if (slsi_util_nla_get_u8(inner, &nl_rtt_params[j].rtt_type))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_PEER:
				if (slsi_util_nla_get_u8(inner, rtt_peer))
					return -EINVAL;
				nl_rtt_params[j].rtt_peer = *rtt_peer;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_CHAN_FREQ:
				if (slsi_util_nla_get_u16(inner, &channel_freq))
					return -EINVAL;
				nl_rtt_params[j].channel_freq = channel_freq * 2;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_PERIOD:
				if (slsi_util_nla_get_u8(inner, &nl_rtt_params[j].burst_period))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_NUM_BURST:
				if (slsi_util_nla_get_u8(inner, &nl_rtt_params[j].num_burst))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST:
				if (slsi_util_nla_get_u8(inner, &nl_rtt_params[j].num_frames_per_burst))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTMR:
				if (slsi_util_nla_get_u8(inner, &nl_rtt_params[j].num_retries_per_ftmr))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_BURST_DURATION:
				if (slsi_util_nla_get_u8(inner, &nl_rtt_params[j].burst_duration))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_PREAMBLE:
				if (slsi_util_nla_get_u16(inner, &nl_rtt_params[j].preamble))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_BW:
				if (slsi_util_nla_get_u16(inner, &nl_rtt_params[j].bw))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_LCI:
				if (slsi_util_nla_get_u16(inner, &nl_rtt_params[j].LCI_request))
					return -EINVAL;
				break;
			case SLSI_RTT_ATTRIBUTE_TARGET_LCR:
				if (slsi_util_nla_get_u16(inner, &nl_rtt_params[j].LCR_request))
					return -EINVAL;
				break;
			default:
				break;
			}
		}
		j++;
		if (j > num_devices)
			return 0;
	}
	return 0;
}

static int slsi_rtt_set_config(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int r = -EINVAL, type, rtt_id, j = 0;
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif;
	struct net_device *dev = wdev->netdev;
	struct slsi_rtt_config *nl_rtt_params;
	struct slsi_rtt_id_params *rtt_id_params = NULL;
	const struct nlattr *iter;
	u8 source_addr[ETH_ALEN];
	int tmp;
	u16 request_id = 0;
	u8 num_devices = 0;
	u8 rtt_peer = SLSI_RTT_PEER_AP;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_RTT_RANGE_START\n");
	if (!slsi_dev_rtt_supported()) {
		SLSI_ERR(sdev, "RTT not supported.\n");
		return WIFI_HAL_ERROR_NOT_SUPPORTED;
	}
	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return r;
	}

	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
		case SLSI_RTT_ATTRIBUTE_TARGET_CNT:
			if (slsi_util_nla_get_u8(iter, &num_devices))
				goto exit_with_mutex;
			SLSI_DBG1_NODEV(SLSI_GSCAN, "Target cnt %d\n", num_devices);
			break;
		case SLSI_RTT_ATTRIBUTE_TARGET_ID:
			if (slsi_util_nla_get_u16(iter, &request_id))
				goto exit_with_mutex;
			SLSI_DBG1_NODEV(SLSI_GSCAN, "Request ID: %d\n", request_id);
			break;
		default:
			break;
		}
	}

	if (!num_devices) {
		SLSI_ERR_NODEV("No device found for rtt configuration!\n");
		goto exit_with_mutex;
	}
	/* Allocate memory for the received config params */
	nl_rtt_params = kcalloc(num_devices, sizeof(*nl_rtt_params), GFP_KERNEL);
	if (!nl_rtt_params) {
		SLSI_ERR_NODEV("Failed to allocate memory for config rtt_param\n");
		r = -ENOMEM;
		goto exit_with_mutex;
	}
	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
		case SLSI_RTT_ATTRIBUTE_TARGET_INFO:
			r = slsi_rtt_process_target_info(iter, nl_rtt_params, &rtt_peer, num_devices);
			if (r)
				goto exit_with_nl_rtt_params;
			break;
		default:
			break;
		}
	}
	SLSI_ETHER_COPY(source_addr, dev->dev_addr);
	/* Check for the first available rtt_id and allocate memory for rtt_id_aprams. */
	for (rtt_id = SLSI_MIN_RTT_ID; rtt_id <= SLSI_MAX_RTT_ID; rtt_id++) {
		if (!sdev->rtt_id_params[rtt_id - 1]) {
			rtt_id_params = kzalloc(sizeof(struct slsi_rtt_id_params) + ETH_ALEN * num_devices, GFP_KERNEL);
			if (!rtt_id_params) {
				SLSI_INFO(sdev, "Failed to allocate memory for rtt_id_params.\n");
				r = -ENOMEM;
				goto exit_with_nl_rtt_params;
			}
			sdev->rtt_id_params[rtt_id - 1] = rtt_id_params;
			break;
		}
	}
	if (rtt_id > SLSI_MAX_RTT_ID) {
		SLSI_ERR_NODEV("RTT_ID(1-7) is in use currently!!\n");
		goto exit_with_nl_rtt_params;
	}
	rtt_id_params->fapi_req_id = rtt_id;
	rtt_id_params->hal_request_id = request_id;
	rtt_id_params->peer_count = num_devices;
	rtt_id_params->peer_type = rtt_peer;
	/*Store mac addr list corresponding to each rtt_id. */
	for (j = 0; j < num_devices; j++)
		SLSI_ETHER_COPY(&rtt_id_params->peers[j * ETH_ALEN], nl_rtt_params[j].peer_addr);
	if (rtt_peer == SLSI_RTT_PEER_NAN) {
#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		r = slsi_send_nan_range_config(sdev, num_devices, nl_rtt_params, rtt_id);
		SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
#else
		SLSI_ERR_NODEV("NAN not enabled\n");
		r = -ENOTSUPP;
#endif
	} else {
		r = slsi_mlme_add_range_req(sdev, dev, num_devices, nl_rtt_params, rtt_id, source_addr);
		if (r) {
			kfree(rtt_id_params);
			sdev->rtt_id_params[rtt_id - 1] = NULL;
			SLSI_ERR_NODEV("Failed to set rtt config\n");
		}
	}
exit_with_nl_rtt_params:
	kfree(nl_rtt_params);

exit_with_mutex:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

int slsi_tx_rate_calc(struct sk_buff *nl_skb, u16 fw_rate, int res, bool tx_rate)
{
	u8 preamble;
	const u32 fw_rate_idx_to_80211_rate[] = { 0, 10, 20, 55, 60, 90, 110, 120, 180, 240, 360, 480, 540 };
	u32 data_rate = 0;
	u32 mcs = 0, nss = 0;
	u32 chan_bw_idx = 0;
	int gi_idx;

	preamble = (fw_rate & SLSI_FW_API_RATE_MODE_SELECTOR_FIELD) >> 14;
	if ((fw_rate & SLSI_FW_API_RATE_MODE_SELECTOR_FIELD) == SLSI_FW_API_RATE_NON_HT_SELECTED) {
		u16 fw_rate_idx = fw_rate & SLSI_FW_API_RATE_INDEX_FIELD;

		if (fw_rate > 0 && fw_rate_idx < ARRAY_SIZE(fw_rate_idx_to_80211_rate))
			data_rate = fw_rate_idx_to_80211_rate[fw_rate_idx];
	} else if ((fw_rate & SLSI_FW_API_RATE_MODE_SELECTOR_FIELD) == SLSI_FW_API_RATE_HT_SELECTED) {
		nss = (SLSI_FW_API_RATE_HT_NSS_FIELD & fw_rate) >> 6;
		chan_bw_idx = (fw_rate & SLSI_FW_API_RATE_BW_FIELD) >> 9;
		gi_idx = ((fw_rate & SLSI_FW_API_RATE_SGI) == SLSI_FW_API_RATE_SGI) ? 1 : 0;
		mcs = SLSI_FW_API_RATE_HT_MCS_FIELD & fw_rate;
		if (chan_bw_idx < 2 && mcs <= 7) {
			data_rate = (nss + 1) * slsi_rates_table[chan_bw_idx][gi_idx][mcs];
		} else if (mcs == 32 && chan_bw_idx == 1) {
			if (gi_idx == 1)
				data_rate = (nss + 1) * 67;
			else
				data_rate = (nss + 1) * 60;
		} else {
			SLSI_WARN_NODEV("FW DATA RATE decode error fw_rate:0x%x, bw:0x%x, mcs_idx:0x%x, nss : %d\n",
					fw_rate, chan_bw_idx, mcs, nss);
		}
	} else if ((fw_rate & SLSI_FW_API_RATE_MODE_SELECTOR_FIELD) == SLSI_FW_API_RATE_VHT_SELECTED) {
		/* report vht rate in legacy units and not as mcs index. reason: upper layers may still be not
		 * updated with vht msc table.
		 */
		chan_bw_idx = (fw_rate & SLSI_FW_API_RATE_BW_FIELD) >> 9;
		gi_idx = ((fw_rate & SLSI_FW_API_RATE_SGI) == SLSI_FW_API_RATE_SGI) ? 1 : 0;
		/* Calculate  NSS --> bits 6 to 4*/
		nss = (SLSI_FW_API_RATE_VHT_NSS_FIELD & fw_rate) >> 4;
		mcs = SLSI_FW_API_RATE_VHT_MCS_FIELD & fw_rate;
		/* Bandwidth (BW): 0x0= 20 MHz, 0x1= 40 MHz, 0x2= 80 MHz, 0x3= 160/ 80+80 MHz. 0x3 is not supported */
		if (chan_bw_idx <= 2 && mcs <= 11)
			data_rate = (nss + 1) * slsi_rates_table[chan_bw_idx][gi_idx][mcs];
		else
			SLSI_WARN_NODEV("FW DATA RATE decode error fw_rate:0x%x, bw:0x%x, mcs_idx:0x%x,nss : %d\n",
					fw_rate, chan_bw_idx, mcs, nss);
		if (nss > 1)
			nss += 1;
	}

	if (tx_rate) {
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_TX_PREAMBLE, preamble);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_TX_NSS, nss);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_TX_BW, chan_bw_idx);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_TX_MCS, mcs);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_TX_RATE, data_rate);
	} else {
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_RX_PREAMBLE, preamble);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_RX_NSS, nss);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_RX_BW, chan_bw_idx);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_RX_MCS, mcs);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_RX_RATE, data_rate);
	}
	return res;
}

void slsi_rx_range_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u32 i, tm;
	u16 rtt_entry_count = fapi_get_u16(skb, u.mlme_range_ind.entries);
	u16 rtt_id = fapi_get_u16(skb, u.mlme_range_ind.rtt_id);
	u16 request_id;
	u32 tmac = fapi_get_u32(skb, u.mlme_range_ind.timestamp);
	int data_len = fapi_get_datalen(skb);
	u8                *ip_ptr, *start_ptr;
	u16 tx_data = 0, rx_data = 0;
	struct sk_buff *nl_skb;
	int res = 0;
	struct nlattr *nlattr_nested;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
	struct timespec64 ts;
#else
	struct timespec ts;
#endif
	u64 tkernel;
	u8 rep_cnt = 0;
	__le16 *le16_ptr = NULL;
	__le32 *le32_ptr = NULL;
	u16 value;
	u32 temp_value;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (rtt_entry_count > SLSI_WIFI_RTT_RESULT_MAX_ENTRY) {
		SLSI_WARN(sdev, "Invalid rtt result entry count : %d\n", rtt_entry_count);
		goto exit;
	}
	if (data_len < SLSI_WIFI_RTT_RESULT_LENGTH + 2) {
		SLSI_WARN(sdev, "Invalid rtt result length : %d\n", data_len);
		goto exit;
	}
	if (rtt_id < SLSI_MIN_RTT_ID || rtt_id > SLSI_MAX_RTT_ID) {
		SLSI_WARN(sdev, "Invalid rtt_id : %d\n", rtt_id);
		goto exit;
	}
	if (sdev->rtt_id_params[rtt_id - 1]) {
		request_id = sdev->rtt_id_params[rtt_id - 1]->hal_request_id;
	} else {
		SLSI_WARN(sdev, "Invalid rtt_id - rtt_id_params is uninitialized : %d\n", rtt_id);
		goto exit;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	nl_skb = cfg80211_vendor_event_alloc(sdev->wiphy, NULL, NLMSG_DEFAULT_SIZE,
					     SLSI_NL80211_RTT_RESULT_EVENT, GFP_KERNEL);
#else
	nl_skb = cfg80211_vendor_event_alloc(sdev->wiphy, NLMSG_DEFAULT_SIZE, SLSI_NL80211_RTT_RESULT_EVENT,
					     GFP_KERNEL);
#endif
#ifdef CONFIG_SCSC_WLAN_DEBUG
	SLSI_DBG1_NODEV(SLSI_GSCAN, "Event: %s(%d)\n",
			slsi_print_event_name(SLSI_NL80211_RTT_RESULT_EVENT), SLSI_NL80211_RTT_RESULT_EVENT);
#endif

	if (!nl_skb) {
		SLSI_ERR(sdev, "NO MEM for nl_skb!!!\n");
		goto exit;
		}

	ip_ptr = fapi_get_data(skb);
	start_ptr = fapi_get_data(skb);
	res |= nla_put_u16(nl_skb, SLSI_RTT_ATTRIBUTE_RESULT_CNT, rtt_entry_count);
	res |= nla_put_u16(nl_skb, SLSI_RTT_ATTRIBUTE_TARGET_ID, request_id);
	res |= nla_put_u8(nl_skb, SLSI_RTT_ATTRIBUTE_RESULTS_PER_TARGET, 1);
	for (i = 0; i < rtt_entry_count; i++) {
		if (ip_ptr[0] != SLSI_WIFI_RTT_RESULT_ID) {
			SLSI_WARN(sdev, "rtt_entry : %d Invalid id : %d\n",
				  i, ip_ptr[0]);
			break;
		}
		if (ip_ptr[1] != SLSI_WIFI_RTT_RESULT_LENGTH) {
			SLSI_WARN(sdev, "rtt_entry : %d Invalid len:%d\n",
				  i, ip_ptr[1]);
			break;
		}

		nlattr_nested = nla_nest_start(nl_skb, SLSI_RTT_ATTRIBUTE_RESULT);
		if (!nlattr_nested) {
			SLSI_ERR(sdev, "Error in nla_nest_start\n");
			/* Dont use slsi skb wrapper for this free */
			kfree_skb(nl_skb);
			goto exit;
		}
		ip_ptr += 7;             /*skip first 7 bytes for fapi_ie_generic */
		res |= nla_put(nl_skb, SLSI_RTT_EVENT_ATTR_ADDR, ETH_ALEN, ip_ptr);
		ip_ptr += 6;

		le16_ptr = (__le16 *)ip_ptr;
		value = le16_to_cpu(*le16_ptr);
		res |= nla_put_u16(nl_skb, SLSI_RTT_EVENT_ATTR_BURST_NUM, value);
		ip_ptr += 2;

		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_MEASUREMENT_NUM, *ip_ptr++);
		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_SUCCESS_NUM, *ip_ptr++);
		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_NUM_PER_BURST_PEER, *ip_ptr++);

		le16_ptr = (__le16 *)ip_ptr;
		value = le16_to_cpu(*le16_ptr);
		res |= nla_put_u16(nl_skb, SLSI_RTT_EVENT_ATTR_STATUS, value);
		ip_ptr += 2;
		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_RETRY_AFTER_DURATION, *ip_ptr++);

		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_TYPE, *ip_ptr++);

		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_RSSI, *ip_ptr++);

		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_RSSI_SPREAD, *ip_ptr++);

		memcpy(&tx_data, ip_ptr, 2);
		res = slsi_tx_rate_calc(nl_skb, tx_data, res, 1);
		ip_ptr += 2;

		memcpy(&rx_data, ip_ptr, 2);
		res = slsi_tx_rate_calc(nl_skb, rx_data, res, 0);
		ip_ptr += 2;

		le32_ptr = (__le32 *)ip_ptr;
		temp_value = le32_to_cpu(*le32_ptr);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_RTT, temp_value);
		ip_ptr += 4;

		le16_ptr = (__le16 *)ip_ptr;
		value = le16_to_cpu(*le16_ptr);
		res |= nla_put_u16(nl_skb, SLSI_RTT_EVENT_ATTR_RTT_SD, value);
		ip_ptr += 2;

		le16_ptr = (__le16 *)ip_ptr;
		value = le16_to_cpu(*le16_ptr);
		res |= nla_put_u16(nl_skb, SLSI_RTT_EVENT_ATTR_RTT_SPREAD, value);
		ip_ptr += 2;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
		ts = ktime_to_timespec64(ktime_get_boottime());
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
		ts = ktime_to_timespec(ktime_get_boottime());
#else
		get_monotonic_boottime(&ts);
#endif

		tkernel = (u64)TIMESPEC_TO_US(ts);
		le32_ptr = (__le32 *)ip_ptr;
		temp_value = le32_to_cpu(*le32_ptr);
		tm = temp_value;
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_TIMESTAMP_US, tkernel - (tmac - tm));
		ip_ptr += 4;

		le32_ptr = (__le32 *)ip_ptr;
		temp_value = le32_to_cpu(*le32_ptr);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_DISTANCE_MM, temp_value);
		ip_ptr += 4;

		le32_ptr = (__le32 *)ip_ptr;
		temp_value = le32_to_cpu(*le32_ptr);
		res |= nla_put_u32(nl_skb, SLSI_RTT_EVENT_ATTR_DISTANCE_SD_MM, temp_value);
		ip_ptr += 4;

		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_BURST_DURATION_MSN, *ip_ptr++);
		res |= nla_put_u8(nl_skb, SLSI_RTT_EVENT_ATTR_NEGOTIATED_BURST_NUM, *ip_ptr++);
		for (rep_cnt = 0; rep_cnt < 2; rep_cnt++) {
			if (ip_ptr - start_ptr < data_len && ip_ptr[0] == WLAN_EID_MEASURE_REPORT) {
				if (ip_ptr[4] == 8)  /*LCI Element*/
					res |= nla_put(nl_skb, SLSI_RTT_EVENT_ATTR_LCI,
						       ip_ptr[1] + 2, ip_ptr);
				else if (ip_ptr[4] == 11)   /*LCR element */
					res |= nla_put(nl_skb, SLSI_RTT_EVENT_ATTR_LCR,
						       ip_ptr[1] + 2, ip_ptr);
				ip_ptr += ip_ptr[1] + 2;
			}
		}
		nla_nest_end(nl_skb, nlattr_nested);
	}
	SLSI_DBG_HEX(sdev, SLSI_GSCAN, fapi_get_data(skb), fapi_get_datalen(skb), "range indication skb buffer:\n");
	if (res) {
		SLSI_ERR(sdev, "Error in nla_put*:0x%x\n", res);
		kfree_skb(nl_skb);
		goto exit;
	}
	cfg80211_vendor_event(nl_skb, GFP_KERNEL);
exit:
	kfree_skb(skb);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
}

void slsi_rx_range_done_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u16 rtt_id = fapi_get_u16(skb, u.mlme_range_ind.rtt_id);
	u16 request_id;

	if (rtt_id < SLSI_MIN_RTT_ID || rtt_id > SLSI_MAX_RTT_ID) {
		SLSI_WARN(sdev, "Invalid rtt_id : %d\n", rtt_id);
		kfree_skb(skb);
		return;
	}
	if (sdev->rtt_id_params[rtt_id - 1]) {
		request_id = sdev->rtt_id_params[rtt_id - 1]->hal_request_id;
	} else {
		SLSI_WARN(sdev, "Invalid rtt_id - rtt_id_params is uninitialized : %d\n", rtt_id);
		kfree_skb(skb);
		return;
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
#ifdef CONFIG_SCSC_WLAN_DEBUG
	SLSI_DBG1_NODEV(SLSI_GSCAN, "Event: %s(%d)\n",
			slsi_print_event_name(SLSI_NL80211_RTT_COMPLETE_EVENT), SLSI_NL80211_RTT_COMPLETE_EVENT);
#endif
	slsi_vendor_event(sdev, SLSI_NL80211_RTT_COMPLETE_EVENT, &request_id, sizeof(request_id));

	kfree(sdev->rtt_id_params[rtt_id - 1]);
	sdev->rtt_id_params[rtt_id - 1] = NULL;
	kfree_skb(skb);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
}

/* Function to remove peer from structure rtt_id_params corresponding to rtt_id_idx. */
void slsi_rtt_remove_peer(struct slsi_dev *sdev, u8 *addr, u8 rtt_id_idx, u8 count_addr)
{
	int i = 0, j;
	u8 zero_addr[ETH_ALEN];
	u8 remove_id = 1;

	memset(zero_addr, 0, ETH_ALEN);
	/* Check for each peer if it's exists in the count_addr list if so then set it to zero */
	for (i = 0; i < sdev->rtt_id_params[rtt_id_idx]->peer_count; i++) {
		for (j = 0; j < count_addr; j++)
			if (SLSI_ETHER_EQUAL(&sdev->rtt_id_params[rtt_id_idx]->peers[i * ETH_ALEN],
					     &addr[j * ETH_ALEN]))
				SLSI_ETHER_COPY(&sdev->rtt_id_params[rtt_id_idx]->peers[i * ETH_ALEN], zero_addr);
		/* If peer doesn't exist in addr list then no need to remove this rtt id from rtt_id_params. */
		if (!SLSI_ETHER_EQUAL(&sdev->rtt_id_params[rtt_id_idx]->peers[i * ETH_ALEN], zero_addr))
			remove_id = 0;
	}
	/* If all the peer addresses are set to zero then remove rtt id and make it available for use */
	if (remove_id) {
		SLSI_INFO(sdev, "Remove rtt id:%d\n", rtt_id_idx + 1);
		kfree(sdev->rtt_id_params[rtt_id_idx]);
		sdev->rtt_id_params[rtt_id_idx] = NULL;
	}
}

static int slsi_rtt_cancel_config(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int temp, r = 1, j = 0, type, count = 0, i = 0, k = 0;
	struct slsi_dev            *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif;
	u8 *addr;
	/* List to store requested addresses corresponding to each rtt id which needs to be cancelled. */
	u8 *cancel_addr_list;
	const struct nlattr *iter;
	u16  num_devices = 0;
	u8 count_addr = 0, peer_count = 0;

	SLSI_DBG1_NODEV(SLSI_GSCAN, "RTT_SUBCMD_CANCEL_CONFIG\n");
	if (!slsi_dev_rtt_supported()) {
		SLSI_WARN(sdev, "RTT not supported.\n");
		return -ENOTSUPP;
	}
	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}

	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	nla_for_each_attr(iter, data, len, temp) {
		type = nla_type(iter);
		switch (type) {
		case SLSI_RTT_ATTRIBUTE_TARGET_CNT:
			if (slsi_util_nla_get_u16(iter, &num_devices)) {
				SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
				return -EINVAL;
			}
			SLSI_DBG1_NODEV(SLSI_GSCAN, "Target cnt %d\n", num_devices);
			break;
		default:
			SLSI_ERR_NODEV("No ATTRIBUTE_Target cnt - %d\n", type);
			break;
		}
	}
	if (!num_devices) {
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return r;
	}
	/* Allocate memory for the received mac addresses */
	addr = kzalloc(ETH_ALEN * num_devices, GFP_KERNEL);
	if (!addr) {
		SLSI_ERR_NODEV("Failed to allocate memory for mac addresses\n");
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -ENOMEM;
	}
	nla_for_each_attr(iter, data, len, temp) {
		type = nla_type(iter);
		if (type == SLSI_RTT_ATTRIBUTE_TARGET_MAC) {
			if (count >= num_devices)
				break;
			if (slsi_util_nla_get_data(iter, ETH_ALEN, &addr[j]))
				continue;
			j = j + ETH_ALEN;
			count++;
		} else {
			SLSI_ERR_NODEV("No ATTRIBUTE_MAC - %d\n", type);
		}
	}
	cancel_addr_list = kzalloc(ETH_ALEN * num_devices, GFP_KERNEL);
	if (!cancel_addr_list) {
		SLSI_INFO(sdev, "Failed to allocate memory for cancel_addr_list.\n");
		kfree(addr);
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -ENOMEM;
	}
	/* Iterate over each rtt_id and check if the requested address present in the current peer list. */
	for (i = 0; i < SLSI_MAX_RTT_ID; i++) {
		count_addr = 0;
		if (!sdev->rtt_id_params[i])
			continue;
		for (j = 0; j < num_devices; j++) {
			peer_count = sdev->rtt_id_params[i]->peer_count;
			for (k = 0; k < peer_count; k++)
				if (SLSI_ETHER_EQUAL(&sdev->rtt_id_params[i]->peers[k * ETH_ALEN],
						     &addr[j * ETH_ALEN])) {
					SLSI_ETHER_COPY(&cancel_addr_list[count_addr * ETH_ALEN], &addr[j * ETH_ALEN]);
					count_addr++;
					break;
				}
		}
		if (!count_addr)
			continue;
		if (sdev->rtt_id_params[i]->peer_type == SLSI_RTT_PEER_NAN) {
			sdev->rtt_id_params[i]->peer_type = 0;
#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
			SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
			r = slsi_send_nan_range_cancel(sdev);
			SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
#else
			SLSI_ERR_NODEV("NAN not enabled\n");
			r = -ENOTSUPP;
#endif
		} else {
			r = slsi_mlme_del_range_req(sdev, dev, count_addr, cancel_addr_list, i + 1);
		}
		slsi_rtt_remove_peer(sdev, cancel_addr_list, i, count_addr);
		if (r)
			SLSI_ERR_NODEV("Failed to cancel rtt config for id:%d\n", i + 1);
		memset(cancel_addr_list, 0, ETH_ALEN * num_devices);
	}
	kfree(addr);
	kfree(cancel_addr_list);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

#if IS_ENABLED(CONFIG_IPV6)
static int slsi_configure_nd_offload(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev          *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif;
	int                      ret = 0;
	int                      temp;
	int                      type;
	const struct nlattr      *attr;
	u8 nd_offload_enabled = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "Received nd_offload command\n");

	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}

	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!ndev_vif->activated || ndev_vif->vif_type != FAPI_VIFTYPE_STATION ||
	    ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED) {
		SLSI_DBG3(sdev, SLSI_GSCAN, "vif error\n");
		ret = -EPERM;
		goto exit;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_NL_ATTRIBUTE_ND_OFFLOAD_VALUE:
		{
			if (slsi_util_nla_get_u8(attr, &nd_offload_enabled)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		}
		default:
			SLSI_ERR(sdev, "Invalid type : %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	ndev_vif->sta.nd_offload_enabled = nd_offload_enabled;
	ret = slsi_mlme_set_ipv6_address(sdev, dev);
	if (ret < 0) {
		SLSI_ERR(sdev, "Configure nd_offload failed ret:%d nd_offload_enabled: %d\n", ret, nd_offload_enabled);
		ret = -EINVAL;
		goto exit;
	}
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}
#endif

static int slsi_get_roaming_capabilities(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev          *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif;
	int                      ret = 0;
	struct slsi_mib_value *values = NULL;
	struct slsi_mib_data mibrsp = { 0, NULL };
	struct slsi_mib_get_entry get_values[] = {{ SLSI_PSID_UNIFI_ROAM_BLACKLIST_SIZE, { 0, 0 } } };
	u32    max_blacklist_size = 0;
	u32    max_whitelist_size = 0;
	struct sk_buff *nl_skb;
	struct nlattr *nlattr_start;

	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}

	ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	mibrsp.dataLength = 10;
	mibrsp.data = kmalloc(mibrsp.dataLength, GFP_KERNEL);
	if (!mibrsp.data) {
		SLSI_ERR(sdev, "Cannot kmalloc %d bytes\n", mibrsp.dataLength);
		ret = -ENOMEM;
		goto exit;
	}
	values = slsi_read_mibs(sdev, NULL, get_values, ARRAY_SIZE(get_values), &mibrsp);
	if (values && (values[0].type == SLSI_MIB_TYPE_UINT ||  values[0].type == SLSI_MIB_TYPE_INT))
		max_blacklist_size = values[0].u.uintValue;
	nl_skb = cfg80211_vendor_cmd_alloc_reply_skb(sdev->wiphy, NLMSG_DEFAULT_SIZE);
	if (!nl_skb) {
		SLSI_ERR(sdev, "NO MEM for nl_skb!!!\n");
		ret = -ENOMEM;
		goto exit_with_mib_resp;
	}

	nlattr_start = nla_nest_start(nl_skb, NL80211_ATTR_VENDOR_DATA);
	if (!nlattr_start) {
		SLSI_ERR(sdev, "failed to put NL80211_ATTR_VENDOR_DATA\n");
		/* Dont use slsi skb wrapper for this free */
		kfree_skb(nl_skb);
		ret = -EINVAL;
		goto exit_with_mib_resp;
	}

	ret = nla_put_u32(nl_skb, SLSI_NL_ATTR_MAX_BLACKLIST_SIZE, max_blacklist_size);
	ret |= nla_put_u32(nl_skb, SLSI_NL_ATTR_MAX_WHITELIST_SIZE, max_whitelist_size);
	if (ret) {
		SLSI_ERR(sdev, "Error in nla_put*:0x%x\n", ret);
		/* Dont use slsi skb wrapper for this free */
		kfree_skb(nl_skb);
		goto exit_with_mib_resp;
	}

	ret = cfg80211_vendor_cmd_reply(nl_skb);
	if (ret)
		SLSI_ERR(sdev, "cfg80211_vendor_cmd_reply failed :%d\n", ret);
exit_with_mib_resp:
	kfree(mibrsp.data);
	kfree(values);
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

static int slsi_set_roaming_state(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device   *dev = wdev->netdev;
	int                 temp = 0;
	int                 type = 0;
	const struct nlattr *attr;
	int                 ret = 0;
	int                 roam_state = 0;
	u8 val = 0;

	if (!dev) {
		SLSI_WARN_NODEV("net_dev is NULL\n");
		return -EINVAL;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_NL_ATTR_ROAM_STATE:
			if (slsi_util_nla_get_u8(attr, &val)) {
				ret = -EINVAL;
				goto exit;
			}
			roam_state = (int)val;
			break;
		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	if (!slsi_is_rf_test_mode_enabled()) {
		SLSI_DBG1_NODEV(SLSI_GSCAN, "SUBCMD_SET_ROAMING_STATE roam_state = %d\n", roam_state);
		ret = slsi_set_mib_roam(sdev, NULL, SLSI_PSID_UNIFI_ROAMING_ACTIVATED, roam_state);
	} else {
		SLSI_DBG1_NODEV(SLSI_GSCAN, "Ignoring SUBCMD_SET_ROAMING_STATE for roam_state = %d in RF Test Mode.\n", roam_state);
	}
	if (ret < 0)
		SLSI_ERR_NODEV("Failed to set roaming state\n");

exit:
	return ret;
}

char *slsi_get_roam_reason_str(int roam_reason, u32 reason_code)
{
	switch (roam_reason) {
	case 0:
		return "WIFI_ROAMING_SEARCH_REASON_FORCED_ROAMING";
	case 1:
		return "WIFI_ROAMING_SEARCH_REASON_LOW_RSSI";
	case 3:
		if (reason_code == FAPI_REASONCODE_CHANNEL_SWITCH_FAILURE)
			return "WIFI_ROAMING_SEARCH_REASON_CHANNEL_SWITCH_FAILURE";
		return "WIFI_ROAMING_SEARCH_REASON_LINK_LOSS";
	case 5:
		return "WIFI_ROAMING_SEARCH_REASON_BTM_REQ";
	case 2:
		return "WIFI_ROAMING_SEARCH_REASON_CU_TRIGGER";
	case 4:
		return "WIFI_ROAMING_SEARCH_REASON_EMERGENCY";
	case 6:
		return "WIFI_ROAMING_SEARCH_REASON_IDLE";
	case 7:
		return "WIFI_ROAMING_SEARCH_REASON_WTC";
	case 8:
		return "WIFI_ROAMING_SEARCH_REASON_INACTIVITY_TIMER";
	case 9:
		return "WIFI_ROAMING_SEARCH_REASON_SCAN_TIMER";
	case 10:
		return "WIFI_ROAMING_SEARCH_REASON_BT_COEX";
	default:
		return "UNKNOWN_REASON";
	}
}

char *slsi_get_nan_role_str(int nan_role)
{
	switch (nan_role) {
	case 0:
		return "Not Set";
	case 1:
		return "Anchor Master";
	case 2:
		return "Master";
	case 3:
		return "Sync";
	case 4:
		return "Non Sync";
	default:
		return "Undefined";
	}
}

char *slsi_frame_transmit_failure_message_type(int message_type)
{
	switch (message_type) {
	case 0x0001:
		return "eap_message";
	case 0x0002:
		return "eapol_key_m123";
	case 0x0003:
		return "eapol_key_m4";
	case 0x0004:
		return "arp";
	case 0x0005:
		return "dhcp";
	case 0x0006:
		return "neighbor_discovery";
	case 0x0007:
		return "wai_message";
	case 0x0008:
		return "any_other";
	default:
		return "Undefined";
	}
}

char *slsi_get_scan_type(int scan_type)
{
	switch (scan_type) {
	case FAPI_SCANTYPE_SOFT_CACHED_ROAMING_SCAN:
		return "Soft Cached scan";
	case FAPI_SCANTYPE_SOFT_FULL_ROAMING_SCAN:
		return "Soft Full scan";
	case FAPI_SCANTYPE_HARD_CACHED_ROAMING_SCAN:
		return "Hard Cached scan";
	case FAPI_SCANTYPE_HARD_FULL_ROAMING_SCAN:
		return "Hard Full scan";
	default:
		return "Undefined";
	}
}

char *slsi_get_measure_mode(int measure_mode)
{
	switch (measure_mode) {
	case 0:
		return "passive";
	case 1:
		return "active";
	case 2:
		return "beacon_table";
	default:
		return "Undefined";
	}
}

char *slsi_get_nan_schedule_type_str(int schedule_type)
{
	switch (schedule_type) {
	case 1:
		return "NAN_FAW";
	case 2:
		return "NAN_NDC";
	case 3:
		return "NAN_DW";
	default:
		return "Undefined";
	}
}

char *slsi_get_nan_ulw_reason_str(int ulw_reason)
{
	switch (ulw_reason) {
	case 1:
		return "Peer Requested";
	case 2:
		return "Concurrent Operation";
	case 3:
		return "Scan";
	case 4:
		return "BT_COEX";
	case 5:
		return "Power Saving";
	case 6:
		return "Deleted";
	default:
		return "Undefined";
	}
}

#define SLSI_32BYTES_ARRAY_PATTERN "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d " \
				   "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
#define SLSI_32BYTES_ARRAY_LIST(array)		\
	array[0], array[1], array[2], array[3],	\
	array[4], array[5], array[6], array[7],	\
	array[8], array[9], array[10], array[11],	\
	array[12], array[13], array[14], array[15],	\
	array[16], array[17], array[18], array[19],	\
	array[20], array[21], array[22], array[23],	\
	array[24], array[25], array[26], array[27],	\
	array[28], array[29], array[30], array[31]

struct parameter_set {
	u32 master_tsf;
	u32 channel;
	u8 width;
	u8 position;
	u32 schedule_type;
	u32 start_offset;
	u32 slot_duration;
	u32 slot_bitmap;
	u16 vifnum;
};

void slsi_print_availability_log_ind(struct slsi_dev *sdev,  struct parameter_set *pm_set, const char *event, u64 timestamp)
{
	u16 width = pm_set->width;
	if (pm_set->width == 176)
		width = 320;
	if (pm_set->slot_bitmap) {
		SLSI_INFO(sdev, "[0x%x] %s, vif: %d, Master_TSF: 0x%x, Freq: %d, Width: %d, Primary channel Position: %d,"
				" Schedule Type: %s, Start Offset: %d, Slot Duration: %d, Slot Bitmap: "
				SLSI_BYTE_TO_BINARY_PATTERN" "SLSI_BYTE_TO_BINARY_PATTERN" "
				SLSI_BYTE_TO_BINARY_PATTERN" "SLSI_BYTE_TO_BINARY_PATTERN"\n",
			  timestamp, event, pm_set->vifnum, pm_set->master_tsf, pm_set->channel, width,
			  pm_set->position, slsi_get_nan_schedule_type_str(pm_set->schedule_type), pm_set->start_offset,
			  pm_set->slot_duration, SLSI_BYTE_TO_BINARY(pm_set->slot_bitmap),
			  SLSI_BYTE_TO_BINARY(pm_set->slot_bitmap >> 8), SLSI_BYTE_TO_BINARY(pm_set->slot_bitmap >> 16),
			  SLSI_BYTE_TO_BINARY(pm_set->slot_bitmap >> 24));
	} else {
		SLSI_INFO(sdev, "[0x%x] %s, vif: %d, Master_TSF: 0x%x, Freq: %d, Width: %d, Primary channel Position: %d,"
				" Schedule Type: %s\n",
			  timestamp, event, pm_set->vifnum, pm_set->master_tsf, pm_set->channel, width, pm_set->position,
			  slsi_get_nan_schedule_type_str(pm_set->schedule_type));
	}
}

#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
void slsi_handle_nan_rx_event_log_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	u16 event_id = 0;
	u64 timestamp = 0;
	u8 *tlv_data;
	int tlv_buffer__len = fapi_get_datalen(skb), i = 0, status_code = -1, tx_send_status = -1, status_stored = 0;
	u16 vendor_len, tag_id, tag_len, vtag_id, ndp_id = 0;
	u32 tag_value, vtag_value;
	bool multi_param = false, param_set_available = false;
	u32 nan_role = 0, hop_count = 0;
	u32 ulw_reason = 0, ulw_index = 0, ulw_start = 0, ulw_period = 0, ulw_duration = 0, ulw_count = 0;
	u32 tx_mpdu_total = 0, rx_mpdu_total = 0;
	u32 slot_avg_rx[32] = { 0}, slot_avg_tx[32] = {0};
	u8 nan_cluster_id[ETH_ALEN] = {0}, nan_nmi[ETH_ALEN] = {0}, peer_mac_addr[ETH_ALEN] = {0};
	u32 nan_amr_higher = 0, nan_amr_lower = 0;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct parameter_set pm_set = {0};

	pm_set.vifnum = slsi_mlme_get_vif(sdev, skb);
	event_id = fapi_get_s16(skb, u.mlme_event_log_ind.event);
	timestamp = fapi_get_u64(skb, u.mlme_event_log_ind.fw_tsf);
	tlv_data = fapi_get_data(skb);
	while (i + 4 < tlv_buffer__len) {
		tag_id = le16_to_cpu(*((__le16 *)&tlv_data[i]));
		tag_len = le16_to_cpu(*((__le16 *)&tlv_data[i + 2]));
		i += 4;
		if (i + tag_len > tlv_buffer__len) {
			SLSI_INFO(sdev,
				  "Incorrect fapi bulk data[event:0x%x fw_tsf:0x%x tag{0x%x,%d} i:%d mbulk_len:%d\n",
				  event_id, timestamp, tag_id, tag_len, i, tlv_buffer__len);
			SLSI_INFO_HEX(sdev, tlv_data, tlv_buffer__len, "mbulk_data\n");
			return;
		}
		tag_value = slsi_convert_tlv_data_to_value(&tlv_data[i], tag_len);
		multi_param = false;
		switch (tag_id) {
		case SLSI_WIFI_TAG_CHANNEL:
			pm_set.channel = tag_value / 2;
			break;
		case SLSI_WIFI_TAG_STATUS:
			if (!status_stored) {
				status_code = tag_value;
				status_stored = 1;
			} else {
				tx_send_status = tag_value;
			}
			break;
		case SLSI_WIFI_TAG_ADDR:
			SLSI_ETHER_COPY(peer_mac_addr, &tlv_data[i]);
			break;
		case SLSI_WIFI_TAG_VENDOR_SPECIFIC:
			vendor_len = tag_len - 2;
			vtag_id = le16_to_cpu(*((__le16 *)&tlv_data[i]));
			vtag_value = slsi_convert_tlv_data_to_value(&tlv_data[i + 2], vendor_len);
			switch (vtag_id) {
			case SLSI_WIFI_TAG_VD_CLUSTER_ID:
				if (vendor_len != ETH_ALEN) {
					memset(nan_cluster_id, 0, ETH_ALEN);
					SLSI_ERR(sdev, "Cluser ID should be of 6 bytes,bytes received:%d\n", vendor_len);
					break;
				}
				memcpy(nan_cluster_id, &tlv_data[i + 2], vendor_len);
				break;
			case SLSI_WIFI_TAG_VD_NAN_ROLE:
				nan_role = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_NAN_AMR:
				if (vendor_len != 8) {
					SLSI_ERR(sdev, "NAN AMR should be of 8 bytes,bytes received:%d\n", vendor_len);
					break;
				}
				slsi_convert_tlv_to_64bit_value(&tlv_data[i + 2], vendor_len, &nan_amr_lower, &nan_amr_higher);
				break;
			case SLSI_WIFI_TAG_VD_NAN_NMI:
				if (vendor_len != ETH_ALEN) {
					memset(nan_nmi, 0, ETH_ALEN);
					SLSI_ERR(sdev, "NAN NMI should be of 6 bytes,bytes received:%d\n", vendor_len);
					break;
				}
				memcpy(nan_nmi, &tlv_data[i + 2], vendor_len);
				break;
			case SLSI_WIFI_TAG_VD_NAN_HOP_COUNT:
				hop_count = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_MASTER_TSF:
				pm_set.master_tsf = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_CHANNEL_INFO:
				if (vendor_len < 2) {
					SLSI_ERR(sdev, "Channel_info should be at least 2 bytes!\n");
					break;
				}
				pm_set.width = tlv_data[i + 2];
				pm_set.position = tlv_data[i + 3];
				break;
			case SLSI_WIFI_TAG_VD_SCHEDULE_TYPE:
				pm_set.schedule_type = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_START_OFFSET:
				pm_set.start_offset = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_SLOT_DURATION:
				pm_set.slot_duration = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_BITMAP:
				pm_set.slot_bitmap = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_NAN_NDP_ID:
				ndp_id = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ULW_REASON:
				ulw_reason = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ULW_INDEX:
				ulw_index = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ULW_START_TIME:
				ulw_start = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ULW_PERIOD:
				ulw_period = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ULW_DURATION:
				ulw_duration = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ULW_COUNT:
				ulw_count = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_NAN_RX_TOTAL:
				rx_mpdu_total = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_NAN_TX_TOTAL:
				tx_mpdu_total = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_NAN_RX_AVERAGE:
				if (vendor_len > sizeof(slot_avg_rx))
					vendor_len = sizeof(slot_avg_rx);
				memcpy(slot_avg_rx, &tlv_data[i + 2], vendor_len);
				break;
			case SLSI_WIFI_TAG_VD_NAN_TX_AVERAGE:
				if (vendor_len > sizeof(slot_avg_tx))
					vendor_len = sizeof(slot_avg_tx);
				memcpy(slot_avg_tx, &tlv_data[i + 2], vendor_len);
				break;
			case SLSI_WIFI_TAG_VD_PARAMETER_SET:
				multi_param = true;
				if (!param_set_available) {
					param_set_available = true;
					break;
				}
				switch (event_id) {
				case FAPI_EVENT_WIFI_EVENT_NAN_AVAILABILITY_UPDATE:
					if (SLSI_ETHER_EQUAL(nan_nmi, ndev_vif->nan.local_nmi)) {
						slsi_print_availability_log_ind(sdev, &pm_set,
										"NAN_OWN_AVAILABILITY", timestamp);
					} else {
						slsi_print_availability_log_ind(sdev, &pm_set,
										"NAN_PEER_AVAILABILITY", timestamp);
					}
					pm_set.slot_bitmap = 0;
					break;
				case FAPI_EVENT_WIFI_EVENT_NAN_NDP_CONFIRM_RX:
					slsi_print_availability_log_ind(sdev, &pm_set, "NDP_CFM_RX", timestamp);
					break;
				case FAPI_EVENT_WIFI_EVENT_NAN_NDP_CONFIRM_TX_STATUS:
					slsi_print_availability_log_ind(sdev, &pm_set, "NDP_CFM_TX", timestamp);
					break;
				}
				break;
			}
			break;
		}
		if (multi_param)
			i += 2; /* To skip VD_PARAMETER_SET */
		else
			i += tag_len;
	}
	switch (event_id) {
	case FAPI_EVENT_WIFI_EVENT_FW_NAN_ROLE_TYPE:
		SLSI_INFO(sdev, "[0x%lx] WIFI_EVENT_FW_NAN_ROLE_TYPE, Cluster Id:" MACSTR ", NAN Role:%s, "
				"AMR:0x%08x%08x, NMI:" MACSTR ", Hop Count:%d\n", timestamp,
			  MAC2STR(nan_cluster_id), slsi_get_nan_role_str(nan_role), nan_amr_higher, nan_amr_lower,
			  MAC2STR(nan_nmi), hop_count);
		ndev_vif->nan.amr_lower = nan_amr_lower;
		ndev_vif->nan.amr_higher = nan_amr_higher;
		ndev_vif->nan.hopcount = hop_count;
		ndev_vif->nan.role = nan_role;
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_AVAILABILITY_UPDATE:
		if (SLSI_ETHER_EQUAL(nan_nmi, ndev_vif->nan.local_nmi)) {
			slsi_print_availability_log_ind(sdev, &pm_set, "NAN_OWN_AVAILABILITY", timestamp);
		} else {
			slsi_print_availability_log_ind(sdev, &pm_set, "NAN_PEER_AVAILABILITY", timestamp);
		}
		pm_set.slot_bitmap = 0;
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_ULW_UPDATE:
		SLSI_INFO(sdev, "[0x%lx] NAN_ULW_UPDATE, Master_TSF: 0x%x, ULW_Reason:%s, ULW_Index: %d,"
			  " ULW_start_time:%dms, ULW_Period: %dms, ULW_Duration: %dms, ULW_Count: %d, Freq: %d\n",
			  timestamp, pm_set.master_tsf, slsi_get_nan_ulw_reason_str(ulw_reason), ulw_index,
			  ulw_start, ulw_period, ulw_duration, ulw_count, pm_set.channel);
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_TRAFFIC_UPDATE:
		SLSI_INFO(sdev, "[0x%lx] NAN_TRAFFIC_UPDATE, Rx MPDUs total: %d, Tx MPDUs Total: %d, "
			  "Slot Average Rx: "SLSI_32BYTES_ARRAY_PATTERN" Slot Average Tx: "
			  SLSI_32BYTES_ARRAY_PATTERN"\n", timestamp, rx_mpdu_total, tx_mpdu_total,
			  SLSI_32BYTES_ARRAY_LIST(slot_avg_rx), SLSI_32BYTES_ARRAY_LIST(slot_avg_tx));
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_REQUEST_RX:
		SLSI_INFO(sdev, "[0x%lx] NDP_RX type:NDP_REQ NDP_ID:%d peer_mac_addr=" MACSTR ", status:%d\n", timestamp,
			  ndp_id, MAC2STR(peer_mac_addr), status_code);
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_RESPONSE_RX:
		SLSI_INFO(sdev, "[0x%lx] NDP_RX type:NDP_RSP NDP_ID:%d peer_mac_addr=" MACSTR " status:%d\n", timestamp,
			  ndp_id, MAC2STR(peer_mac_addr), status_code);
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_CONFIRM_RX:
		if (pm_set.slot_bitmap)
			slsi_print_availability_log_ind(sdev, &pm_set, "NDP_CFM_RX", timestamp);
		SLSI_INFO(sdev, "[0x%lx] NDP_RX type:NDP_CONFIRM NDP_ID:%d peer_mac_addr=" MACSTR " status:%d\n",
			timestamp, ndp_id, MAC2STR(peer_mac_addr), status_code);
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_INSTALL_RX:
		SLSI_INFO(sdev, "[0x%lx] NDP_RX type:NDP_INSTALL NDP_ID:%d peer_mac_addr=" MACSTR " status:%d\n",
			  timestamp, ndp_id, MAC2STR(peer_mac_addr), status_code);
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_REQUEST_TX_STATUS:
		SLSI_INFO(sdev, "[0x%lx] NDP_TX type:NDP_REQ NDP_ID:%d peer_mac_addr=" MACSTR " status:%d tx_status:%d\n",
			  timestamp, ndp_id, MAC2STR(peer_mac_addr), status_code, tx_send_status);
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_RESPONSE_TX_STATUS:
		SLSI_INFO(sdev, "[0x%lx]NDP_TX type:NDP_RSP NDP_ID:%d peer_mac_addr=" MACSTR " status:%d tx_status:%d\n",
			  timestamp, ndp_id, MAC2STR(peer_mac_addr), status_code, tx_send_status);
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_CONFIRM_TX_STATUS:
		if (pm_set.slot_bitmap)
			slsi_print_availability_log_ind(sdev, &pm_set, "NDP_CFM_TX", timestamp);
		SLSI_INFO(sdev, "[0x%lx] NDP_TX type:NDP_CONFIRM NDP_ID:%d peer_mac_addr=" MACSTR " status:%d tx_status:%d\n",
			  timestamp, ndp_id, MAC2STR(peer_mac_addr), status_code, tx_send_status);
		break;
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_INSTALL_TX_STATUS:
		SLSI_INFO(sdev, "[0x%lx] NDP_TX type:NDP_INSTALL NDP_ID:%d peer_mac_addr=" MACSTR " status:%d tx_status:%d\n",
			  timestamp, ndp_id, MAC2STR(peer_mac_addr), status_code, tx_send_status);
		break;
	}
}
#endif

static void dump_roam_scan_result(struct slsi_dev *sdev, struct net_device *dev,
				  bool *candidate, char *bssid, int freq,
				  int rssi, short cu,
				  int score, int tp_score, bool eligible_value, bool mld_ap)
{
	slsi_conn_log2us_roam_scan_result(sdev, dev, *candidate, bssid,
					  freq, rssi,
					  cu, score,
					  tp_score, eligible_value, mld_ap);
	if (*candidate) {
		SLSI_INFO(sdev, "WIFI_EVENT_ROAM_SCAN_RESULT, Candidate AP, BSSID:" MACSTR
			  ", RSSI:%d, CU:%d, Score:%d.%02d, TP Score:%d, Eligible:%s\n",
			  MAC2STR(bssid), rssi, cu, score / 100, score % 100, tp_score,
			  eligible_value ? "true" : "false");
	} else {
		SLSI_INFO(sdev, "WIFI_EVENT_ROAM_SCAN_RESULT, Current AP, BSSID:" MACSTR
			  ", RSSI:%d, CU:%d, Score:%d.%02d, TP Score:%d MLD_AP:%d\n",
			  MAC2STR(bssid), rssi, cu, score / 100, score % 100, tp_score, mld_ap);
		*candidate = true;
	}
}

struct slsi_rx_evt_vd_info {
	short chan_utilisation;
	u32 roam_reason;
	u32 btm_request_mode;
	u32 btm_response;
	u32 eapol_retry_count;
	u16 eapol_key_type;
	u32 scan_type;
	short score_val;
	short rssi_thresh;
	u32 operating_class;
	u32 measure_mode;
	u32 measure_duration;
	u32 ap_count;
	u32 candidate_count;
	u32 message_type;
	u32 expired_timer_value;
	u32 tp_score_val;
	int mgmt_frame_subtype;
	int is_roaming;
	bool eligible_value;
	u8 full_scan_count;
	short cu_rssi_thresh;
	u16 cu_thresh;
	u32 roaming_type;
	u32 auth_algo_type;
	u32 sae_type;
	u32 sn;
	int dialog_token;
	int disassoc_timer;
	int validity_time;
	int btm_cand_preference;
	int btm_cand_count;
	int btm_validity_interval;
	int btm_disassoc_timer;
	int aid;
	u8 request_mode;
	char *vs_ie;
	bool mld_ap;
	u8 mlo_band;
	u8 inactive_band;
	u8 reason;
	int tid_dl;
	int tid_ul;
	int tx_status;
	bool bt_device_status;
	int t2lm_ftype;
};

struct slsi_rx_evt_info {
	short roam_rssi_val;
	u32 reason_code;
	u32 eapol_msg_type;
	u32 tx_status;
	u32 status_code;
	bool current_info;
	bool candidate_info;
	u8 mac_addr[ETH_ALEN];
	u8 mld_addr[ETH_ALEN];
	u32 chan_frequency;
	int freq_list[MAX_FREQUENCY_COUNT];
	int channel_list[MAX_CHANNEL_COUNT];
	int channel_count;
	char ssid[MAX_SSID_LEN];
	int link_id;
	struct slsi_rx_evt_vd_info vd;
};

static void slsi_rx_event_log_parse_vsie(struct slsi_dev *sdev, u8 *str_buff,
					 struct slsi_rx_evt_info *evt_info, u16 str_len)
{
	u8 i;
	int pos = 0;
	char *vendor_ie = kmalloc(2 * str_len + 1, GFP_KERNEL);

	if (evt_info->vd.vs_ie)
		kfree(evt_info->vd.vs_ie);

	for (i=0; i < str_len; i++)
		pos += scnprintf(vendor_ie + pos, str_len * 2 + 1 - pos, "%02x", str_buff[i]);

	SLSI_INFO(sdev, "Received VSIE length=%d, data=%s\n", str_len, vendor_ie);

	evt_info->vd.vs_ie = vendor_ie;
}

static int slsi_rx_event_log_parse(struct slsi_dev *sdev, struct net_device *dev, u8 *tlv_data, int tlv_buffer__len,
				   u16 event_id, struct slsi_rx_evt_info *evt_info)
{
	int i = 0, iter = 0, lim = 0, channel_val = 0;
	u16 tag_id = 0, tag_len = 0, vtag_id = 0, vendor_len = 0;
	u32 tag_value = 0, vtag_value = 0;
	bool multi_param = false, valid_data = false;
	evt_info->vd.tid_dl = -1;
	evt_info->vd.tid_ul = -1;

	while (i + 4 < tlv_buffer__len) {
		tag_id = le16_to_cpu(*((__le16 *)&tlv_data[i]));
		tag_len = le16_to_cpu(*((__le16 *)&tlv_data[i + 2]));
		i += 4;
		if (i + tag_len > tlv_buffer__len) {
			SLSI_INFO(sdev, "Incorrect fapi bulk data\n");
			return -EINVAL;
		}
		tag_value = slsi_convert_tlv_data_to_value(&tlv_data[i], tag_len);
		multi_param = false;
		switch (tag_id) {
		case SLSI_WIFI_TAG_RSSI:
			evt_info->roam_rssi_val = (short)tag_value;
			break;
		case SLSI_WIFI_TAG_REASON_CODE:
			evt_info->reason_code = tag_value;
			break;
		case SLSI_WIFI_TAG_VENDOR_SPECIFIC:
			vendor_len = tag_len - 2;
			vtag_id = le16_to_cpu(*((__le16 *)&tlv_data[i]));
			vtag_value = slsi_convert_tlv_data_to_value(&tlv_data[i + 2], vendor_len);
			evt_info->vd.tx_status = -1;
			switch (vtag_id) {
			case SLSI_WIFI_TAG_VD_CHANNEL_UTILISATION:
				evt_info->vd.chan_utilisation = (short)vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ROAMING_REASON:
				evt_info->vd.roam_reason = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_BTM_REQUEST_MODE:
				evt_info->vd.btm_request_mode = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_BTM_RESPONSE_STATUS:
				evt_info->vd.btm_response = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_RETRY_COUNT:
				evt_info->vd.eapol_retry_count = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_EAPOL_KEY_TYPE:
				evt_info->vd.eapol_key_type = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_SCAN_TYPE:
				evt_info->vd.scan_type = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_SCORE:
				evt_info->vd.score_val = (short)vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_RSSI_THRESHOLD:
				evt_info->vd.rssi_thresh = (short)vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_OPERATING_CLASS:
				evt_info->vd.operating_class = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_MEASUREMENT_MODE:
				evt_info->vd.measure_mode = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_MEASUREMENT_DURATION:
				evt_info->vd.measure_duration = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_MIN_AP_COUNT:
				evt_info->vd.ap_count = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_CANDIDATE_LIST_COUNT:
				evt_info->vd.candidate_count = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_MESSAGE_TYPE:
				evt_info->vd.message_type = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_EXPIRED_TIMER_VALUE:
				evt_info->vd.expired_timer_value = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ESTIMATED_TP:
				evt_info->vd.tp_score_val = vtag_value;
				break;
			case WIFI_TAG_VD_MGMT_FRAME_SUBTYPE:
				evt_info->vd.mgmt_frame_subtype = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_IS_ROAMING:
				evt_info->vd.is_roaming = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ELIGIBLE:
				evt_info->vd.eligible_value = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_PARAMETER_SET:
				multi_param = true;
				if (!valid_data) {
					valid_data = true;
					break;
				}
				if (event_id == FAPI_EVENT_WIFI_EVENT_ROAM_SCAN_RESULT) {
					dump_roam_scan_result(sdev, dev, &evt_info->current_info, evt_info->mac_addr,
							      evt_info->chan_frequency, evt_info->roam_rssi_val,
							      evt_info->vd.chan_utilisation, evt_info->vd.score_val,
							      evt_info->vd.tp_score_val, evt_info->vd.eligible_value,
							      evt_info->vd.mld_ap);
					memset(evt_info->mac_addr, 0, ETH_ALEN);
					evt_info->chan_frequency = 0;
					evt_info->roam_rssi_val = 0;
					evt_info->vd.chan_utilisation = 0;
					evt_info->vd.score_val = 0;
					evt_info->vd.tp_score_val = 0;
					evt_info->vd.eligible_value = false;
				}
				break;
			case SLSI_WIFI_TAG_VD_FULL_SCAN_COUNT:
				evt_info->vd.full_scan_count = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_CU_RSSI_THRESHOLD:
				evt_info->vd.cu_rssi_thresh = (short)vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_CU_THRESHOLD:
				evt_info->vd.cu_thresh = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_ROAMING_TYPE:
				evt_info->vd.roaming_type = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_AUTH_ALGO:
				evt_info->vd.auth_algo_type = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_SAE_MESSAGE_TYPE:
				evt_info->vd.sae_type = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_SEQUENCE_NUMBER:
				evt_info->vd.sn = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_DIALOG_TOKEN:
				evt_info->vd.dialog_token = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_DIASSOCIATION_TIMER:
				evt_info->vd.disassoc_timer = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_VALIDITY_INTERVAL:
				evt_info->vd.validity_time = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_BTM_CANDIDATE_PREFERENCE:
				evt_info->vd.btm_cand_preference = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_BTM_CANDIDATE_COUNT:
				evt_info->vd.btm_cand_count = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_LOCAL_DURATION:
				if (evt_info->vd.btm_disassoc_timer)
					evt_info->vd.btm_validity_interval = vtag_value;
				else
					evt_info->vd.btm_disassoc_timer = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_AID:
				evt_info->vd.aid = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_MEASUREMENT_REQUEST_MODE:
				evt_info->vd.request_mode = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_MLD:
				evt_info->vd.mld_ap = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_BANDS:
				if (!evt_info->vd.mlo_band)
					evt_info->vd.mlo_band = vtag_value;
				else
					evt_info->vd.inactive_band = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_T2LM_INFO:
				if (evt_info->vd.tid_dl == -1)
					evt_info->vd.tid_dl = vtag_value;
				else
					evt_info->vd.tid_ul = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_T2LM_FRAME_TYPE:
				evt_info->vd.t2lm_ftype = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_TX_STATUS:
				evt_info->vd.tx_status = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_BT_DEVICE_STATUS:
				evt_info->vd.bt_device_status = vtag_value;
				break;
			case SLSI_WIFI_TAG_VD_REASON:
				evt_info->vd.reason = vtag_value;
				break;
			}
			break;
		case SLSI_WIFI_TAG_EAPOL_MESSAGE_TYPE:
			evt_info->eapol_msg_type = tag_value;
			break;
		case SLSI_WIFI_TAG_STATUS:
			if (evt_info->status_code)
				evt_info->tx_status = tag_value;
			else
				evt_info->status_code = tag_value;
			break;
		case SLSI_WIFI_TAG_BSSID:
			SLSI_ETHER_COPY(evt_info->mac_addr, &tlv_data[i]);
			break;
#ifdef CONFIG_SCSC_WLAN_EHT
		case SLSI_WIFI_TAG_ADDR:
			memset(evt_info->mld_addr, 0, ETH_ALEN);
			if (tag_len > ETH_ALEN) {
				SLSI_ERR(sdev, "Invalid tag len %d\n", tag_len);
				break;
			}
			SLSI_ETHER_COPY(evt_info->mld_addr, &tlv_data[i]);
			break;
#endif
		case SLSI_WIFI_TAG_CHANNEL:
			evt_info->chan_frequency = tag_value;
			break;
		case SLSI_WIFI_TAG_IE:
			if (event_id == FAPI_EVENT_WIFI_EVENT_FW_DEAUTHENTICATION_RECEIVED ||
			    event_id == FAPI_EVENT_WIFI_EVENT_FW_DEAUTHENTICATION_SENT ||
			    event_id == FAPI_EVENT_WIFI_EVENT_FW_DISASSOCIATION_RECEIVED ||
			    event_id == FAPI_EVENT_WIFI_EVENT_DISASSOCIATION_REQUESTED||
			    event_id == FAPI_EVENT_WIFI_EVENT_ASSOCIATING_DEAUTH_RECEIVED) {
				slsi_rx_event_log_parse_vsie(sdev, &tlv_data[i], evt_info,
							     tag_len);
				break;
			}
			iter = i;
			lim = iter + tlv_data[iter + 1] + 2;
			iter += 7; /* 1byte (id) + 1byte(length) + 3byte (oui) + 2byte */
			while (iter < lim && lim <= i + tag_len) {
				if (evt_info->channel_count >= MAX_CHANNEL_COUNT) {
					SLSI_ERR(sdev, "ERR: Channel list received >= %d\n", MAX_CHANNEL_COUNT);
					break;
				}
				channel_val = le16_to_cpu(*((__le16 *)&tlv_data[iter]));
				evt_info->freq_list[evt_info->channel_count] = channel_val / 2;
				evt_info->channel_list[evt_info->channel_count] =
							ieee80211_frequency_to_channel(channel_val / 2);
				if (evt_info->channel_list[evt_info->channel_count] < 1 ||
				    evt_info->channel_list[evt_info->channel_count] > SLSI_6GHZ_LAST_CHAN) {
					SLSI_ERR(sdev, "ERR: Invalid channel received %d\n",
						 evt_info->channel_list[evt_info->channel_count]);
					/* Invalid channel is received. Prints out TLV data for SLSI_WIFI_TAG_IE */
					SCSC_BIN_TAG_INFO(BINARY, &tlv_data[i], tlv_data[i + 1] + 2);
					break;
				}
				iter += SLSI_SCAN_CHANNEL_DESCRIPTOR_SIZE;
				evt_info->channel_count += 1;
			}
			break;
		case SLSI_WIFI_TAG_SSID:
			memset(evt_info->ssid, '\0', sizeof(evt_info->ssid));
			if (tag_len > MAX_SSID_LEN)
				memcpy(evt_info->ssid, &tlv_data[i], MAX_SSID_LEN);
			else
				memcpy(evt_info->ssid, &tlv_data[i], tag_len);
			break;
		case SLSI_WIFI_TAG_LINK_ID:
			evt_info->link_id = tag_value;
			break;
		}
		if (multi_param)
			i += 2; /* To skip VD_PARAMETER_SET */
		else
			i += tag_len;
	}
	return 0;
}

static int slsi_get_roam_reason_for_fw_reason(int roam_reason)
{
	switch (roam_reason) {
	case SLSI_WIFI_ROAMING_SEARCH_REASON_FORCED_ROAMING:
		return 0;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_LOW_RSSI:
		return 1;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_LINK_LOSS:
		return 3;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_BTM_REQ:
		return 5;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_CU_TRIGGER:
		return 2;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_EMERGENCY:
		return 4;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_IDLE:
		return 6;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_WTC:
		return 7;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_BT_USAGE:
		return 10;
	case SLSI_WIFI_ROAMING_SEARCH_REASON_SCAN_TIMER:
		return 9;
	default:
		return 0;
	}
}

/* if the Expired_Timer_Value is not set to 0,1,2,3, the Roam_Reason is set to UNKNOWN. */
static int slsi_get_roam_reason_from_expired_tv(int roam_reason, int expired_timer_value)
{
	if (expired_timer_value == SLSI_SOFT_ROAMING_TRIGGER_EVENT_DEFAULT)
		return slsi_get_roam_reason_for_fw_reason(roam_reason);
	else if (expired_timer_value == SLSI_SOFT_ROAMING_TRIGGER_EVENT_INACTIVITY_TIMER)
		return 8;
	else if (expired_timer_value == SLSI_SOFT_ROAMING_TRIGGER_EVENT_RESCAN_TIMER ||
		 expired_timer_value == SLSI_SOFT_ROAMING_TRIGGER_EVENT_BACKGROUND_RESCAN_TIMER)
		return 9;
	else
		return 0;
}

#ifdef CONFIG_SCSC_WLAN_EHT
static void slsi_handle_mlo_events(struct slsi_dev *sdev, struct net_device *dev, u16 event_id,
				   struct slsi_rx_evt_info *evt_info)
{
	switch (event_id) {
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_SETUP:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_MLD_SETUP, Band:%d, BSSID:" MACSTR ", Status:%d, link_id:%d\n",
			  evt_info->vd.mlo_band, MAC2STR(evt_info->mac_addr), evt_info->status_code,
			  evt_info->link_id);
		slsi_conn_log2us_mld_setup(sdev, dev, evt_info->vd.mlo_band, evt_info->mac_addr, evt_info->status_code,
					   evt_info->link_id);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_RECONFIG:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_MLD_RECONFIG, Band:%d, link_id:%d\n", evt_info->vd.mlo_band,
			  evt_info->link_id);
		slsi_conn_log2us_mld_reconfig(sdev, dev, evt_info->vd.mlo_band, evt_info->link_id);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_T2LM_STATUS:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_MLD_T2LM_STATUS, Band:%d, tid_dl:%d tid_ul:%d\n", evt_info->vd.mlo_band,
			  evt_info->vd.tid_dl, evt_info->vd.tid_ul);
		slsi_conn_log2us_mld_t2lm_status(sdev, dev, evt_info->vd.mlo_band, evt_info->vd.tid_dl,
						 evt_info->vd.tid_ul);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_T2LM_REQ_RSP:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_MLD_T2LM_REQ_RSP, FType:%d, Band:%d, token:%d, status:%d tx_status:%d\n",
			  evt_info->vd.t2lm_ftype, evt_info->vd.mlo_band, evt_info->vd.dialog_token,
			  evt_info->status_code, evt_info->vd.tx_status);
		slsi_conn_log2us_mld_t2lm_req_rsp(sdev, dev, evt_info->vd.t2lm_ftype, evt_info->vd.mlo_band,
						  evt_info->vd.dialog_token, evt_info->status_code,
						  evt_info->vd.tx_status);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_T2LM_TEARDOWN:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_MLD_T2LM_TEARDOWN, Band:%d, tx_status:%d\n", evt_info->vd.mlo_band,
			  evt_info->vd.tx_status);
		slsi_conn_log2us_mld_t2lm_teardown(sdev, dev, evt_info->vd.mlo_band, evt_info->vd.tx_status);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_LINK:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_MLD_LINK, Active:%d, Inactive:%d Reason:%d\n", evt_info->vd.mlo_band,
			  evt_info->vd.inactive_band, evt_info->vd.reason);
		slsi_conn_log2us_mld_link(sdev, dev, evt_info->vd.mlo_band, evt_info->vd.inactive_band,
					  evt_info->vd.reason);
		break;
	}
}
#endif

static void slsi_rx_event_log_print(struct slsi_dev *sdev, struct net_device *dev, u16 event_id, u64 timestamp,
				    struct slsi_rx_evt_info *evt_info)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	char *string = NULL;
	int roam_reason = 0;

	switch (event_id) {
	case FAPI_EVENT_WIFI_EVENT_FW_EAPOL_FRAME_TRANSMIT_START:
		sdev->conn_log2us_ctx.mlo_band = evt_info->vd.mlo_band;
		if (evt_info->vd.eapol_key_type == SLSI_WIFI_EAPOL_KEY_TYPE_GTK) {
			if (ndev_vif->iftype == NL80211_IFTYPE_STATION)
				sdev->conn_log2us_ctx.is_eapol_gtk = true;
			SLSI_INFO(sdev, "WIFI_EVENT_FW_EAPOL_FRAME_TRANSMIT_START, Send GTK, G%d\n",
				  evt_info->eapol_msg_type);
		} else if (evt_info->vd.eapol_key_type == SLSI_WIFI_EAPOL_KEY_TYPE_PTK) {
			if (ndev_vif->iftype == NL80211_IFTYPE_STATION) {
				sdev->conn_log2us_ctx.is_eapol_ptk = true;
				sdev->conn_log2us_ctx.eapol_ptk_msg_type = evt_info->eapol_msg_type;
			}
			SLSI_INFO(sdev, "WIFI_EVENT_FW_EAPOL_FRAME_TRANSMIT_START, Send 4way-H/S, M%d\n",
				  evt_info->eapol_msg_type);
		}
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_EAPOL_FRAME_TRANSMIT_STOP:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_EAPOL_FRAME_TRANSMIT_STOP,Result Code:%d, Retry Count:%d\n",
			  evt_info->status_code, evt_info->vd.eapol_retry_count);
		slsi_conn_log2us_eapol_tx(sdev, dev, evt_info->status_code, sdev->conn_log2us_ctx.mlo_band);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_EAPOL_FRAME_RECEIVED:
		if (evt_info->vd.eapol_key_type == SLSI_WIFI_EAPOL_KEY_TYPE_GTK) {
			slsi_conn_log2us_eapol_gtk(sdev, dev, evt_info->eapol_msg_type, evt_info->vd.mlo_band);
			SLSI_INFO(sdev, "WIFI_EVENT_FW_EAPOL_FRAME_RECEIVED, Received GTK, G%d\n",
				  evt_info->eapol_msg_type);
		} else if (evt_info->vd.eapol_key_type == SLSI_WIFI_EAPOL_KEY_TYPE_PTK) {
			slsi_conn_log2us_eapol_ptk(sdev, dev, evt_info->eapol_msg_type, evt_info->vd.mlo_band);
			SLSI_INFO(sdev, "WIFI_EVENT_FW_EAPOL_FRAME_RECEIVED, Received 4way-H/S, M%d\n",
				  evt_info->eapol_msg_type);
		}
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_BTM_FRAME_REQUEST:
		SLSI_INFO(sdev,
			  "WIFI_EVENT_FW_BTM_FRAME_REQUEST,Request Mode:%d, duration=%d, validity interval=%d, Candidate List Count:%d\n",
			  evt_info->vd.btm_request_mode, evt_info->vd.btm_disassoc_timer,
			  evt_info->vd.btm_validity_interval, evt_info->vd.btm_cand_count);
		slsi_conn_log2us_btm_req(sdev, dev,
					 evt_info->vd.dialog_token, evt_info->vd.btm_request_mode,
					 evt_info->vd.btm_disassoc_timer, evt_info->vd.btm_validity_interval,
					 evt_info->vd.btm_cand_count, evt_info->vd.mlo_band);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_BTM_FRAME_RESPONSE:
		if (evt_info->vd.btm_response == 0)
			SLSI_INFO(sdev, "WIFI_EVENT_FW_BTM_FRAME_RESPONSE,Status code:%d, BSSID:"MACSTR"\n",
				  evt_info->vd.btm_response, MAC2STR(evt_info->mac_addr));
		else
			SLSI_INFO(sdev, "WIFI_EVENT_FW_BTM_FRAME_RESPONSE,Status code:%d\n", evt_info->vd.btm_response);
		slsi_conn_log2us_btm_resp(sdev, dev, evt_info->vd.dialog_token, evt_info->vd.btm_response,
					  evt_info->vd.btm_disassoc_timer, evt_info->mac_addr,
					  evt_info->vd.mlo_band);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_BTM_FRAME_QUERY:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_BTM_FRAME_QUERY, dialog_token:%d, reason:%d\n",
			  evt_info->vd.dialog_token, evt_info->reason_code);
		slsi_conn_log2us_btm_query(sdev, dev, evt_info->vd.dialog_token,
					   evt_info->reason_code, evt_info->vd.mlo_band);
		break;
	case FAPI_EVENT_WIFI_EVENT_ROAM_SEARCH_STARTED:
		roam_reason = slsi_get_roam_reason_from_expired_tv(evt_info->vd.roam_reason,
								   evt_info->vd.expired_timer_value);
		slsi_conn_log2us_roam_scan_start(sdev, dev, roam_reason, evt_info->roam_rssi_val,
						 evt_info->vd.chan_utilisation, evt_info->vd.rssi_thresh, timestamp);
		SLSI_INFO(sdev, "WIFI_EVENT_ROAM_SEARCH_STARTED, Roaming Type : %s, RSSI:%d, Deauth Reason:0x%04x, "
			  "RSSI Threshold:%d,Channel Utilisation:%d, Roam Reason: %s, Expired Timer Value: %d\n",
			  (evt_info->vd.roaming_type == 0 ? "Legacy" : "NCHO"), evt_info->roam_rssi_val,
			  evt_info->reason_code, evt_info->vd.rssi_thresh, evt_info->vd.chan_utilisation,
			  slsi_get_roam_reason_str(roam_reason, evt_info->reason_code),
			  evt_info->vd.expired_timer_value);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_AUTH_SENT:
		if (evt_info->vd.is_roaming)
			SLSI_INFO(sdev, "WIFI_EVENT_FW_AUTH_STARTED in Roaming, BSSID:" MACSTR "\n",
				  MAC2STR(evt_info->mac_addr));
		else
			SLSI_INFO(sdev, "WIFI_EVENT_FW_AUTH_STARTED, BSSID:" MACSTR "\n", MAC2STR(evt_info->mac_addr));
		slsi_conn_log2us_auth_req(sdev, dev,
					  evt_info->mac_addr, evt_info->vd.auth_algo_type, evt_info->vd.sae_type,
					  evt_info->vd.sn, evt_info->status_code, evt_info->tx_status,
					  evt_info->vd.is_roaming);
		break;
	case FAPI_EVENT_WIFI_EVENT_AUTH_RECEIVED:
		if (evt_info->vd.is_roaming)
			SLSI_INFO(sdev, "WIFI_EVENT_AUTH_COMPLETE in Roaming,Status code:%d\n", evt_info->status_code);
		else
			SLSI_INFO(sdev, "WIFI_EVENT_AUTH_COMPLETE,Status code:%d\n", evt_info->status_code);
		slsi_conn_log2us_auth_resp(sdev, dev,
					   evt_info->mac_addr, evt_info->vd.auth_algo_type, evt_info->vd.sae_type,
					   evt_info->vd.sn, evt_info->status_code, evt_info->vd.is_roaming);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_ASSOC_STARTED:
		if (evt_info->vd.mgmt_frame_subtype == SLSI_MGMT_FRAME_SUBTYPE_ASSOC_REQ)
			SLSI_INFO(sdev, "WIFI_EVENT_FW_ASSOC_STARTED, status code: %d\n", evt_info->status_code);
		else if (evt_info->vd.mgmt_frame_subtype == SLSI_MGMT_FRAME_SUBTYPE_REASSOC_REQ)
			SLSI_INFO(sdev, "WIFI_EVENT_FW_REASSOC_STARTED, status code: %d\n", evt_info->status_code);
		slsi_conn_log2us_assoc_req(sdev, dev, evt_info->mac_addr, evt_info->vd.sn, evt_info->status_code,
					   evt_info->vd.mgmt_frame_subtype, evt_info->vd.mlo_band);
		break;
	case FAPI_EVENT_WIFI_EVENT_ASSOC_COMPLETE:
		if (evt_info->vd.mgmt_frame_subtype == SLSI_MGMT_FRAME_SUBTYPE_ASSOC_RESP)
			SLSI_INFO(sdev, "WIFI_EVENT_ASSOC_COMPLETE, reason code: %d\n", evt_info->reason_code);
		else if (evt_info->vd.mgmt_frame_subtype == SLSI_MGMT_FRAME_SUBTYPE_REASSOC_RESP)
			SLSI_INFO(sdev, "WIFI_EVENT_REASSOC_COMPLETE, reason code: %d\n", evt_info->reason_code);

		slsi_conn_log2us_assoc_resp(sdev, dev, evt_info->mac_addr, evt_info->vd.sn, evt_info->reason_code,
					    evt_info->vd.mgmt_frame_subtype, evt_info->vd.aid,
					    evt_info->mld_addr);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_DEAUTHENTICATION_RECEIVED:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_DEAUTHENTICATION_RECEIVED, reason code: %d\n", evt_info->reason_code);
		slsi_conn_log2us_deauth(sdev, dev, "RX", ndev_vif->sta.bssid, evt_info->vd.sn,
					evt_info->reason_code, evt_info->vd.vs_ie);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_DEAUTHENTICATION_SENT:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_DEAUTHENTICATION_SENT, reason code: %d\n", evt_info->reason_code);
		slsi_conn_log2us_deauth(sdev, dev, "TX", ndev_vif->sta.bssid, evt_info->vd.sn,
					evt_info->reason_code, evt_info->vd.vs_ie);
		break;
	case FAPI_EVENT_WIFI_EVENT_DISASSOCIATION_REQUESTED:
		SLSI_INFO(sdev, "WIFI_EVENT_DISASSOCIATION_REQUESTED, reason code: %d\n", evt_info->reason_code);
		slsi_conn_log2us_disassoc(sdev, dev, "TX", ndev_vif->sta.bssid, evt_info->vd.sn,
					  evt_info->reason_code, evt_info->vd.vs_ie);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_DISASSOCIATION_RECEIVED:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_DISASSOCIATION_RECEIVED, reason code: %d\n", evt_info->reason_code);
		slsi_conn_log2us_disassoc(sdev, dev, "RX", ndev_vif->sta.bssid, evt_info->vd.sn,
					  evt_info->reason_code, evt_info->vd.vs_ie);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_NR_FRAME_REQUEST:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_NR_FRAME_REQUEST Send Radio Measurement Frame"
			  " (Neighbor Report Req) Sent from Mobile\n");
		slsi_conn_log2us_nr_frame_req(sdev, dev, evt_info->vd.dialog_token, evt_info->ssid,
					      evt_info->vd.mlo_band);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_RM_FRAME_RESPONSE:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_RM_FRAME_RESPONSE Received Radio Measurement "
			  "Frame (Radio Measurement Rep)\n");
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_NR_FRAME_RESPONSE:
		string = slsi_print_channel_list(evt_info->channel_list, evt_info->channel_count);
		SLSI_INFO(sdev, "WIFI_EVENT_FW_NR_FRAME_RESPONSE, Channel List:%s\n", (!string ? "-1" : string));
		slsi_conn_log2us_nr_frame_resp(sdev, dev, evt_info->vd.dialog_token, evt_info->channel_count,
					       evt_info->freq_list, evt_info->vd.candidate_count,
					       evt_info->vd.mlo_band);
		kfree(string);
		break;
	case FAPI_EVENT_WIFI_EVENT_AUTH_TIMEOUT:
		SLSI_INFO(sdev, "WIFI_EVENT_AUTH_TIMEOUT, BSSID:" MACSTR ", Result:%d\n",
			  MAC2STR(evt_info->mac_addr), evt_info->status_code);
		break;
	case FAPI_EVENT_WIFI_EVENT_ROAM_AUTH_TIMEOUT:
		SLSI_INFO(sdev, "WIFI_EVENT_ROAM_AUTH_TIMEOUT, BSSID:" MACSTR ", Result:%d\n",
			  MAC2STR(evt_info->mac_addr), evt_info->status_code);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_CONNECTION_ATTEMPT_ABORTED:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_CONNECTION_ATTEMPT_ABORTED, BSSID:" MACSTR ", Result:%d\n",
			  MAC2STR(evt_info->mac_addr), evt_info->reason_code);
		break;
	case FAPI_EVENT_WIFI_EVENT_ROAM_SCAN_STARTED:
		string = slsi_print_channel_list(evt_info->channel_list, evt_info->channel_count);
		SLSI_INFO(sdev, "WIFI_EVENT_ROAM_SCAN_STARTED, SSID:%s, Scan Type:%s, Channel List:%s\n",
			evt_info->ssid, slsi_get_scan_type(evt_info->vd.scan_type), (!string ? "-1" : string));
		kfree(string);
		slsi_conn_log2us_roam_scan_save(sdev, dev, evt_info->vd.scan_type,
						evt_info->channel_count, evt_info->freq_list);
		break;
	case FAPI_EVENT_WIFI_EVENT_ROAM_RSSI_THRESHOLD:
		SLSI_INFO(sdev, "WIFI_EVENT_ROAM_RSSI_THRESHOLD, Full scan count:%d, RSSI Threshold:%d, "
			  "CU RSSI Threshold:%d, CU Threshold:%d\n",
			  evt_info->vd.full_scan_count, evt_info->vd.rssi_thresh,
			  evt_info->vd.cu_rssi_thresh, evt_info->vd.cu_thresh);
		break;
	case FAPI_EVENT_WIFI_EVENT_ROAM_SCAN_COMPLETE:
		SLSI_INFO(sdev, "WIFI_EVENT_ROAM_SCAN_COMPLETE, Scan Type:%s\n",
			  slsi_get_scan_type(evt_info->vd.scan_type));
		slsi_conn_log2us_roam_scan_done(sdev, dev, timestamp);
		if (is_zero_ether_addr(evt_info->mac_addr)) {
			SLSI_INFO(sdev,
				  "WIFI_EVENT_ROAM_SCAN_COMPLETE, [ROAM] RESULT NO_ROAM Status:1\n");
			slsi_conn_log2us_roam_result(sdev, dev, NULL, timestamp, false);

		} else {
			SLSI_INFO(sdev, "WIFI_EVENT_ROAM_SCAN_COMPLETE, [ROAM] RESULT ROAM  BSSID:" MACSTR ", \
				  Status:0\n", MAC2STR(evt_info->mac_addr));
			slsi_conn_log2us_roam_result(sdev, dev, evt_info->mac_addr, timestamp, true);
		}
		break;
	case FAPI_EVENT_WIFI_EVENT_ROAM_SCAN_RESULT:
		dump_roam_scan_result(sdev, dev, &evt_info->current_info, evt_info->mac_addr,
				      evt_info->chan_frequency, evt_info->roam_rssi_val, evt_info->vd.chan_utilisation,
				      evt_info->vd.score_val, evt_info->vd.tp_score_val, evt_info->vd.eligible_value,
				      evt_info->vd.mld_ap);
		break;
	case FAPI_EVENT_WIFI_EVENT_ROAM_SEARCH_STOPPED:
		SLSI_INFO(sdev, "FAPI_EVENT_WIFI_EVENT_ROAM_SEARCH_STOPPED, Reason:%d\n", evt_info->reason_code);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_BEACON_REPORT_REQUEST:
		string = slsi_print_channel_list(evt_info->channel_list, evt_info->channel_count);
		SLSI_INFO(sdev,
			  "WIFI_EVENT_FW_BEACON_REPORT_REQUEST, Token:%d,Operating Class:%d,channel list:%s,"
			  "Measurement Duration:%d,Measurement Mode:%s,BSSID:" MACSTR ",SSID:%s\n",
			  evt_info->vd.dialog_token, evt_info->vd.operating_class, (!string ? "-1" : string),
			  evt_info->vd.measure_duration, slsi_get_measure_mode(evt_info->vd.measure_mode),
			  MAC2STR(evt_info->mac_addr), evt_info->ssid);
		slsi_conn_log2us_beacon_report_request(sdev, dev,
						       evt_info->vd.dialog_token, evt_info->vd.operating_class,
						       (!string ? "-1" : string), evt_info->vd.measure_duration,
						       slsi_get_measure_mode(evt_info->vd.measure_mode),
						       evt_info->vd.request_mode, evt_info->vd.mlo_band);
		kfree(string);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_BEACON_REPORT_RESPONSE:
		if (evt_info->reason_code) {
			SLSI_INFO(sdev,
				  "WIFI_EVENT_FW_BEACON_REPORT_RESPONSE, Token:%d, Scanned AP Number:%d, Reason:%d\n",
				  evt_info->vd.dialog_token, evt_info->vd.ap_count, evt_info->reason_code);
		} else {
			SLSI_INFO(sdev, "WIFI_EVENT_FW_BEACON_REPORT_RESPONSE, Token:%d, Scanned AP Number:%d\n",
				  evt_info->vd.dialog_token, evt_info->vd.ap_count);
		}
		slsi_conn_log2us_beacon_report_response(sdev, dev,
							evt_info->vd.dialog_token, evt_info->vd.ap_count,
							evt_info->vd.mlo_band);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_FTM_RANGE_REQUEST:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_FTM_RANGE_REQUEST, Min Ap Count:%d, Candidate List Count:%d\n",
			  evt_info->vd.ap_count, evt_info->vd.candidate_count);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_FRAME_TRANSMIT_FAILURE:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_FRAME_TRANSMIT_FAILURE, Message Type:%s, Result:%d, Retry Count:%d\n",
			  slsi_frame_transmit_failure_message_type(evt_info->vd.message_type),
			  evt_info->status_code,
			  evt_info->vd.eapol_retry_count);
		break;
	case FAPI_EVENT_WIFI_EVENT_ASSOCIATING_DEAUTH_RECEIVED:
		SLSI_INFO(sdev, "WIFI_EVENT_ASSOCIATING_DEAUTH_RECEIVED, BSSID:" MACSTR ", Reason:%d\n",
			  MAC2STR(evt_info->mac_addr), evt_info->reason_code);
		slsi_conn_log2us_deauth(sdev, dev, "RX", ndev_vif->sta.bssid, evt_info->vd.sn,
					evt_info->reason_code, evt_info->vd.vs_ie);
		break;
	case FAPI_EVENT_WIFI_EVENT_BT_COEX_BT_SCO_START:
		sdev->conn_log2us_ctx.btcoex_sco = 1;
		break;
	case FAPI_EVENT_WIFI_EVENT_BT_COEX_BT_SCO_STOP:
		sdev->conn_log2us_ctx.btcoex_sco = 0;
		break;
	case FAPI_EVENT_WIFI_EVENT_BT_COEX_BT_SCAN_START:
		sdev->conn_log2us_ctx.btcoex_scan = 1;
		break;
	case FAPI_EVENT_WIFI_EVENT_BT_COEX_BT_SCAN_STOP:
		sdev->conn_log2us_ctx.btcoex_scan = 0;
		break;
	case FAPI_EVENT_WIFI_EVENT_BT_COEX_BT_HID_START:
		sdev->conn_log2us_ctx.btcoex_hid = 1;
		break;
	case FAPI_EVENT_WIFI_EVENT_BT_COEX_BT_HID_STOP:
		sdev->conn_log2us_ctx.btcoex_hid = 0;
		break;
	case FAPI_EVENT_WIFI_EVENT_BT_COEX_BT_DEVICE_STATUS:
		SLSI_INFO(sdev, "WIFI_EVENT_BT_COEX_BT_DEVICE_STATUS, bt_device_status:%s\n",
			  (evt_info->vd.bt_device_status == 1 ? "Connected" : "Disconnected"));
		sdev->conn_log2us_ctx.bt_device_status = evt_info->vd.bt_device_status;
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_BTM_CANDIDATE:
		SLSI_INFO(sdev, "WIFI_EVENT_FW_BTM_CANDIDATE, BSSID:" MACSTR ", Preference:%d\n",
			  MAC2STR(evt_info->mac_addr), evt_info->vd.btm_cand_preference);
		slsi_conn_log2us_btm_cand(sdev, dev, evt_info->mac_addr, evt_info->vd.btm_cand_preference);
		break;
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_SETUP:
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_RECONFIG:
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_T2LM_STATUS:
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_T2LM_REQ_RSP:
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_T2LM_TEARDOWN:
	case FAPI_EVENT_WIFI_EVENT_FW_MLD_LINK:
#ifdef CONFIG_SCSC_WLAN_EHT
		slsi_handle_mlo_events(sdev, dev, event_id, evt_info);
		break;
#else
		SLSI_ERR(sdev, "EHT is not enabled but EHT/MLO event:0x%x received\n", event_id);
#endif
	}
}

void slsi_rx_event_log_indication(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	u16 event_id = 0;
	u64 timestamp = 0;
	u8 *tlv_data = NULL;
	int tlv_buffer__len = fapi_get_datalen(skb);
	struct slsi_rx_evt_info evt_info = {0};
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	event_id = fapi_get_s16(skb, u.mlme_event_log_ind.event);
	timestamp = fapi_get_u64(skb, u.mlme_event_log_ind.fw_tsf);
	tlv_data = fapi_get_data(skb);

	SLSI_DBG3(sdev, SLSI_GSCAN, "event id = %d, len = %d\n", event_id, tlv_buffer__len);

#if IS_ENABLED(CONFIG_SCSC_WIFILOGGER)
	SCSC_WLOG_FW_EVENT(WLOG_NORMAL, event_id, timestamp, fapi_get_data(skb), fapi_get_datalen(skb));
#endif
	switch (event_id) {
	case FAPI_EVENT_WIFI_EVENT_FW_NAN_ROLE_TYPE:
	case FAPI_EVENT_WIFI_EVENT_NAN_AVAILABILITY_UPDATE:
	case FAPI_EVENT_WIFI_EVENT_NAN_ULW_UPDATE:
	case FAPI_EVENT_WIFI_EVENT_NAN_TRAFFIC_UPDATE:
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_REQUEST_RX:
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_RESPONSE_RX:
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_CONFIRM_RX:
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_INSTALL_RX:
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_REQUEST_TX_STATUS:
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_RESPONSE_TX_STATUS:
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_CONFIRM_TX_STATUS:
	case FAPI_EVENT_WIFI_EVENT_NAN_NDP_INSTALL_TX_STATUS:
#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
		slsi_handle_nan_rx_event_log_ind(sdev, dev, skb);
#else
		SLSI_ERR(sdev, "NAN is not enabled but NAN event:0x%x received\n", event_id);
#endif
		goto exit;
	}

	if (!slsi_rx_event_log_parse(sdev, dev, tlv_data, tlv_buffer__len, event_id, &evt_info))
		slsi_rx_event_log_print(sdev, dev, event_id, timestamp, &evt_info);
exit:
	if (evt_info.vd.vs_ie) {
		kfree(evt_info.vd.vs_ie);
		evt_info.vd.vs_ie = NULL;
	}
	kfree_skb(skb);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
}

#ifdef CONFIG_SCSC_WLAN_ENHANCED_LOGGING
static void slsi_on_ring_buffer_data(char *ring_name, char *buffer, int buffer_size,
				     struct scsc_wifi_ring_buffer_status *buffer_status, void *ctx)
{
#ifndef SCSC_SEP_VERSION
	struct sk_buff *skb;
	int event_id = SLSI_NL80211_LOGGER_RING_EVENT;
	struct slsi_dev *sdev = ctx;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, NULL, buffer_size, event_id, GFP_KERNEL);
#else
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, buffer_size, event_id, GFP_KERNEL);
#endif
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for vendor event: %d\n", event_id);
		return;
	}

	if (nla_put(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_STATUS, sizeof(*buffer_status), buffer_status) ||
	    nla_put(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_DATA, buffer_size, buffer)) {
		SLSI_ERR_NODEV("Failed nla_put\n");
		kfree_skb(skb);
		return;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
#endif
}

static void slsi_on_alert(char *buffer, int buffer_size, int err_code, void *ctx)
{
	struct sk_buff *skb;
	int event_id = SLSI_NL80211_LOGGER_FW_DUMP_EVENT;
	struct slsi_dev *sdev = ctx;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, NULL, buffer_size, event_id, GFP_KERNEL);
#else
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, buffer_size, event_id, GFP_KERNEL);
#endif
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for vendor event: %d\n", event_id);
		goto exit;
	}

	if (nla_put_u32(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_DUMP_LEN, buffer_size) ||
	    nla_put(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_DATA, buffer_size, buffer)) {
		SLSI_ERR_NODEV("Failed nla_put\n");
		kfree_skb(skb);
		goto exit;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
}

static void slsi_on_firmware_memory_dump(char *buffer, int buffer_size, void *ctx)
{
	SLSI_ERR_NODEV("slsi_on_firmware_memory_dump\n");
	kfree(mem_dump_buffer);
	mem_dump_buffer = NULL;
	mem_dump_buffer = kmalloc(buffer_size, GFP_KERNEL);
	if (!mem_dump_buffer) {
		SLSI_ERR_NODEV("Failed to allocate memory for mem_dump_buffer\n");
		return;
	}
	mem_dump_buffer_size = buffer_size;
	memcpy(mem_dump_buffer, buffer, mem_dump_buffer_size);
}

static void slsi_on_driver_memory_dump(char *buffer, int buffer_size, void *ctx)
{
	SLSI_ERR_NODEV("slsi_on_driver_memory_dump\n");
	kfree(mem_dump_buffer);
	mem_dump_buffer = NULL;
	mem_dump_buffer_size = buffer_size;
	mem_dump_buffer = kmalloc(mem_dump_buffer_size, GFP_KERNEL);
	if (!mem_dump_buffer) {
		SLSI_ERR_NODEV("Failed to allocate memory for mem_dump_buffer\n");
		return;
	}
	memcpy(mem_dump_buffer, buffer, mem_dump_buffer_size);
}

static int slsi_enable_logging(struct slsi_dev *sdev, bool enable)
{
	int                  status = 0;
#ifdef ENABLE_WIFI_LOGGER_MIB_WRITE
	struct slsi_mib_data mib_data = { 0, NULL };

	SLSI_DBG3(sdev, SLSI_GSCAN, "Value of enable is : %d\n", enable);
	status = slsi_mib_encode_bool(&mib_data, SLSI_PSID_UNIFI_LOGGER_ENABLED, enable, 0);
	if (status != SLSI_MIB_STATUS_SUCCESS) {
		SLSI_ERR(sdev, "slsi_enable_logging failed: no mem for MIB\n");
		status = -ENOMEM;
		goto exit;
	}
	status = slsi_mlme_set(sdev, NULL, mib_data.data, mib_data.dataLength);
	kfree(mib_data.data);
	if (status)
		SLSI_ERR(sdev, "Err setting unifiLoggerEnabled MIB. error = %d\n", status);

exit:
	return status;
#else
	SLSI_DBG3(sdev, SLSI_GSCAN, "UnifiLoggerEnabled MIB write disabled\n");
	return status;
#endif
}

static int slsi_start_logging(struct wiphy *wiphy, struct wireless_dev *wdev, const void  *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                 ret = 0;
	int                 temp = 0;
	int                 type = 0;
	char                ring_name[32] = {0};
	int                 verbose_level = 0;
	int                 ring_flags = 0;
	int                 max_interval_sec = 0;
	int                 min_data_size = 0;
	const struct nlattr *attr;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	nla_for_each_attr(attr, data, len, temp) {
		if (!attr) {
			ret = -EINVAL;
			goto exit;
		}

		type = nla_type(attr);

		switch (type) {
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_NAME:
			strncpy(ring_name, nla_data(attr), MIN(sizeof(ring_name) - 1, nla_len(attr)));
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_VERBOSE_LEVEL:
			if (slsi_util_nla_get_u32(attr, &verbose_level)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_FLAGS:
			if (slsi_util_nla_get_u32(attr, &ring_flags)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_LOG_MAX_INTERVAL:
			if (slsi_util_nla_get_u32(attr, &max_interval_sec)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_LOG_MIN_DATA_SIZE:
			if (slsi_util_nla_get_u32(attr, &min_data_size)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		default:
			SLSI_ERR(sdev, "Unknown type: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}
	ret = scsc_wifi_set_log_handler(slsi_on_ring_buffer_data, sdev);
	if (ret < 0) {
		SLSI_ERR(sdev, "scsc_wifi_set_log_handler failed ret: %d\n", ret);
		goto exit;
	}
	ret = scsc_wifi_set_alert_handler(slsi_on_alert, sdev);
	if (ret < 0)
		SLSI_ERR(sdev, "Warning : scsc_wifi_set_alert_handler failed ret: %d\n", ret);
	ret = slsi_enable_logging(sdev, 1);
	if (ret < 0) {
		SLSI_ERR(sdev, "slsi_enable_logging for enable = 1, failed ret: %d\n", ret);
		goto exit_with_reset_alert_handler;
	}
	ret = scsc_wifi_start_logging(verbose_level, ring_flags, max_interval_sec, min_data_size, ring_name);
	if (ret < 0) {
		SLSI_ERR(sdev, "scsc_wifi_start_logging failed ret: %d\n", ret);
		goto exit_with_disable_logging;
	} else {
		goto exit;
	}
exit_with_disable_logging:
	ret = slsi_enable_logging(sdev, 0);
	if (ret < 0)
		SLSI_ERR(sdev, "slsi_enable_logging for enable = 0, failed ret: %d\n", ret);
exit_with_reset_alert_handler:
	ret = scsc_wifi_reset_alert_handler();
	if (ret < 0)
		SLSI_ERR(sdev, "Warning : scsc_wifi_reset_alert_handler failed ret: %d\n", ret);
	ret = scsc_wifi_reset_log_handler();
	if (ret < 0)
		SLSI_ERR(sdev, "scsc_wifi_reset_log_handler failed ret: %d\n", ret);
exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_reset_logging(struct wiphy *wiphy, struct wireless_dev *wdev, const void  *data, int len)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	int             ret = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	ret = slsi_enable_logging(sdev, 0);
	if (ret < 0)
		SLSI_ERR(sdev, "slsi_enable_logging for enable = 0, failed ret: %d\n", ret);
	ret = scsc_wifi_reset_log_handler();
	if (ret < 0)
		SLSI_ERR(sdev, "scsc_wifi_reset_log_handler failed ret: %d\n", ret);
	ret = scsc_wifi_reset_alert_handler();
	if (ret < 0)
		SLSI_ERR(sdev, "Warning : scsc_wifi_reset_alert_handler failed ret: %d\n", ret);
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_trigger_fw_mem_dump(struct wiphy *wiphy, struct wireless_dev *wdev, const void  *data, int len)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	int             ret = 0;
	struct sk_buff  *skb = NULL;
	int             length = 100;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);

	ret = scsc_wifi_get_firmware_memory_dump(slsi_on_firmware_memory_dump, sdev);
	if (ret) {
		SLSI_ERR(sdev, "scsc_wifi_get_firmware_memory_dump failed : %d\n", ret);
		goto exit;
	}

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, length);
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for Vendor event\n");
		ret = -ENOMEM;
		goto exit;
	}

	if (nla_put_u32(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_DUMP_LEN, mem_dump_buffer_size)) {
		SLSI_ERR_NODEV("Failed nla_put\n");
		kfree_skb(skb);
		ret = -EINVAL;
		goto exit;
	}

	ret = cfg80211_vendor_cmd_reply(skb);

	if (ret)
		SLSI_ERR(sdev, "Vendor Command reply failed ret:%d\n", ret);

exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_get_fw_mem_dump(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                 ret = 0;
	int                 temp = 0;
	int                 type = 0;
	int                 buf_len = 0;
	void __user         *user_buf = NULL;
	const struct nlattr *attr;
	struct sk_buff      *skb;
	u64 val = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	if (!data)
		return -EINVAL;

	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_DUMP_LEN:
			if (slsi_util_nla_get_u32(attr, &buf_len)) {
				SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
				return -EINVAL;
			}
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_DUMP_DATA:
			if (slsi_util_nla_get_u64(attr, &val)) {
				SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
				return -EINVAL;
			}
			user_buf = (void __user *)(unsigned long)(val);
			break;
		default:
			SLSI_ERR(sdev, "Unknown type: %d\n", type);
			SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
			return -EINVAL;
		}
	}
	if (buf_len > 0 && user_buf && mem_dump_buffer) {
		if (buf_len > mem_dump_buffer_size)
			buf_len = mem_dump_buffer_size;
		ret = copy_to_user(user_buf, mem_dump_buffer, buf_len);
		if (ret) {
			SLSI_ERR(sdev, "failed to copy memdump into user buffer : %d\n", ret);
			goto exit;
		}

		/* Alloc the SKB for vendor_event */
		skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, 100);
		if (!skb) {
			SLSI_ERR_NODEV("Failed to allocate skb for Vendor event\n");
			ret = -ENOMEM;
			goto exit;
		}

		/* Indicate the memdump is successfully copied */
		if (nla_put(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_DUMP_DATA, sizeof(ret), &ret)) {
			SLSI_ERR_NODEV("Failed nla_put\n");
			kfree_skb(skb);
			ret = -EINVAL;
			goto exit;
		}

		ret = cfg80211_vendor_cmd_reply(skb);

		if (ret)
			SLSI_ERR(sdev, "Vendor Command reply failed ret:%d\n", ret);
	}

exit:
	kfree(mem_dump_buffer);
	mem_dump_buffer = NULL;
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_trigger_driver_mem_dump(struct wiphy *wiphy, struct wireless_dev *wdev, const void  *data, int len)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	int             ret = 0;
	struct sk_buff  *skb = NULL;
	int             length = 100;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);

	ret = scsc_wifi_get_driver_memory_dump(slsi_on_driver_memory_dump, sdev);
	if (ret) {
		SLSI_ERR(sdev, "scsc_wifi_get_driver_memory_dump failed : %d\n", ret);
		goto exit;
	}

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, length);
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for Vendor event\n");
		ret = -ENOMEM;
		goto exit;
	}

	if (nla_put_u32(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_DRIVER_DUMP_LEN, mem_dump_buffer_size)) {
		SLSI_ERR_NODEV("Failed nla_put\n");
		kfree_skb(skb);
		ret = -EINVAL;
		goto exit;
	}

	ret = cfg80211_vendor_cmd_reply(skb);

	if (ret)
		SLSI_ERR(sdev, "Vendor Command reply failed ret:%d\n", ret);

exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_get_driver_mem_dump(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                 ret = 0;
	int                 temp = 0;
	int                 type = 0;
	int                 buf_len = 0;
	void __user         *user_buf = NULL;
	const struct nlattr *attr;
	struct sk_buff      *skb;
	u64 val = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_DRIVER_DUMP_LEN:
			if (slsi_util_nla_get_u32(attr, &buf_len)) {
				SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
				return -EINVAL;
			}
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_DRIVER_DUMP_DATA:
			if (slsi_util_nla_get_u64(attr, &val)) {
				SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
				return -EINVAL;
			}
			user_buf = (void __user *)(unsigned long)(val);
			break;
		default:
			SLSI_ERR(sdev, "Unknown type: %d\n", type);
			SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
			return -EINVAL;
		}
	}
	if (buf_len > 0 && user_buf && mem_dump_buffer_size) {
		if (buf_len > mem_dump_buffer_size)
			buf_len = mem_dump_buffer_size;
		ret = copy_to_user(user_buf, mem_dump_buffer, buf_len);
		if (ret) {
			SLSI_ERR(sdev, "failed to copy memdump into user buffer : %d\n", ret);
			goto exit;
		}

		/* Alloc the SKB for vendor_event */
		skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, 100);
		if (!skb) {
			SLSI_ERR_NODEV("Failed to allocate skb for Vendor event\n");
			ret = -ENOMEM;
			goto exit;
		}

		/* Indicate the memdump is successfully copied */
		if (nla_put(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_DRIVER_DUMP_DATA, sizeof(ret), &ret)) {
			SLSI_ERR_NODEV("Failed nla_put\n");
			kfree_skb(skb);
			ret = -EINVAL;
			goto exit;
		}

		ret = cfg80211_vendor_cmd_reply(skb);

		if (ret)
			SLSI_ERR(sdev, "Vendor Command reply failed ret:%d\n", ret);
	}

exit:
	kfree(mem_dump_buffer);
	mem_dump_buffer = NULL;
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_get_version(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                 ret = 0;
	int                 temp = 0;
	int                 type = 0;
	int                 buffer_size = 1024;
	bool                log_version = false;
	char                *buffer;
	const struct nlattr *attr;

	buffer = kzalloc(buffer_size, GFP_KERNEL);
	if (!buffer) {
		SLSI_ERR(sdev, "No mem. Size:%d\n", buffer_size);
		return -ENOMEM;
	}
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_DRIVER_VERSION:
			log_version = true;
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_VERSION:
			log_version = false;
			break;
		default:
			SLSI_ERR(sdev, "Unknown type: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	if (log_version)
		ret = scsc_wifi_get_driver_version(buffer, buffer_size);
	else
		ret = scsc_wifi_get_firmware_version(buffer, buffer_size);

	if (ret < 0) {
		SLSI_ERR(sdev, "failed to get the version %d\n", ret);
		goto exit;
	}

	ret = slsi_vendor_cmd_reply(wiphy, buffer, strlen(buffer));
exit:
	kfree(buffer);
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_get_ring_buffers_status(struct wiphy *wiphy, struct wireless_dev *wdev, const void  *data, int len)
{
	struct slsi_dev                     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                                 ret = 0;
	int                                 num_rings = SLSI_MAX_NUM_RING;
	struct sk_buff                      *skb;
	struct scsc_wifi_ring_buffer_status status[SLSI_MAX_NUM_RING];

	SLSI_DBG1(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	memset(status, 0, sizeof(struct scsc_wifi_ring_buffer_status) * num_rings);
	ret = scsc_wifi_get_ring_buffers_status(&num_rings, status);
	if (ret < 0) {
		SLSI_ERR(sdev, "scsc_wifi_get_ring_buffers_status failed ret:%d\n", ret);
		goto exit;
	}

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, 700);
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for Vendor event\n");
		ret = -ENOMEM;
		goto exit;
	}

	/* Indicate that the ring count and ring buffers status is successfully copied */
	if (nla_put_u8(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_NUM, num_rings) ||
	    nla_put(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_STATUS, sizeof(status[0]) * num_rings, status)) {
		SLSI_ERR_NODEV("Failed nla_put\n");
		kfree_skb(skb);
		ret = -EINVAL;
		goto exit;
	}

	ret = cfg80211_vendor_cmd_reply(skb);

	if (ret)
		SLSI_ERR(sdev, "Vendor Command reply failed ret:%d\n", ret);
exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_get_ring_data(struct wiphy *wiphy, struct wireless_dev *wdev, const void  *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                 ret = 0;
	int                 temp = 0;
	int                 type = 0;
	char                ring_name[32] = {0};
	const struct nlattr *attr;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	nla_for_each_attr(attr, data, len, temp) {
		if (!attr) {
			ret = -EINVAL;
			goto exit;
		}
		type = nla_type(attr);
		switch (type) {
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_NAME:
			strncpy(ring_name, nla_data(attr), MIN(sizeof(ring_name) - 1, nla_len(attr)));
			break;
		default:
			SLSI_ERR(sdev, "Unknown type: %d\n", type);
			goto exit;
		}
	}

	ret = scsc_wifi_get_ring_data(ring_name);
	if (ret < 0)
		SLSI_ERR(sdev, "trigger_get_data failed ret:%d\n", ret);
exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_get_logger_supported_feature_set(struct wiphy *wiphy, struct wireless_dev *wdev,
						 const void  *data, int len)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	int             ret = 0;
	u32             supported_features = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	ret = scsc_wifi_get_logger_supported_feature_set(&supported_features);
	if (ret < 0) {
		SLSI_ERR(sdev, "scsc_wifi_get_logger_supported_feature_set failed ret:%d\n", ret);
		goto exit;
	}
	ret = slsi_vendor_cmd_reply(wiphy, &supported_features, sizeof(supported_features));
exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_start_pkt_fate_monitoring(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	int                  ret = 0;
#ifdef ENABLE_WIFI_LOGGER_MIB_WRITE
	struct slsi_dev      *sdev = SDEV_FROM_WIPHY(wiphy);
	struct slsi_mib_data mib_data = { 0, NULL };

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	ret = slsi_mib_encode_bool(&mib_data, SLSI_PSID_UNIFI_TX_DATA_CONFIRM, 1, 0);
	if (ret != SLSI_MIB_STATUS_SUCCESS) {
		SLSI_ERR(sdev, "Failed to set UnifiTxDataConfirm MIB : no mem for MIB\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = slsi_mlme_set(sdev, NULL, mib_data.data, mib_data.dataLength);

	if (ret) {
		SLSI_ERR(sdev, "Err setting UnifiTxDataConfirm MIB. error = %d\n", ret);
		goto exit;
	}

	ret = scsc_wifi_start_pkt_fate_monitoring();
	if (ret < 0) {
		SLSI_ERR(sdev, "scsc_wifi_start_pkt_fate_monitoring failed, ret=%d\n", ret);

		// Resetting the SLSI_PSID_UNIFI_TX_DATA_CONFIRM mib back to 0.
		mib_data.dataLength = 0;
		mib_data.data = NULL;
		ret = slsi_mib_encode_bool(&mib_data, SLSI_PSID_UNIFI_TX_DATA_CONFIRM, 1, 0);
		if (ret != SLSI_MIB_STATUS_SUCCESS) {
			SLSI_ERR(sdev, "Failed to set UnifiTxDataConfirm MIB : no mem for MIB\n");
			ret = -ENOMEM;
			goto exit;
		}

		ret = slsi_mlme_set(sdev, NULL, mib_data.data, mib_data.dataLength);

		if (ret) {
			SLSI_ERR(sdev, "Err setting UnifiTxDataConfirm MIB. error = %d\n", ret);
			goto exit;
		}
	}
exit:
	kfree(mib_data.data);
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
#else
	SLSI_ERR_NODEV("slsi_start_pkt_fate_monitoring : UnifiTxDataConfirm MIB write disabled\n");
	return ret;
#endif
}

static int slsi_get_tx_pkt_fates(struct wiphy *wiphy, struct wireless_dev *wdev, const void  *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                 ret = 0;
	int                 temp = 0;
	int                 type = 0;
	void __user         *user_buf = NULL;
	u32                 req_count = 0;
	size_t              provided_count = 0;
	struct sk_buff      *skb;
	const struct nlattr *attr;
	u64 val = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_NUM:
			if (slsi_util_nla_get_u32(attr, &req_count)) {
				ret = -EINVAL;
				goto exit;
			}
			if (req_count > MAX_FATE_LOG_LEN) {
				SLSI_ERR(sdev, "Found invalid req_count %d for SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_NUM\n", req_count);
				ret = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_DATA:
			if (slsi_util_nla_get_u64(attr, &val)) {
				ret = -EINVAL;
				goto exit;
			}
			user_buf = (void __user *)(unsigned long)(val);
			break;
		default:
			SLSI_ERR(sdev, "Unknown type: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	ret = scsc_wifi_get_tx_pkt_fates(user_buf, req_count, &provided_count, true);
	if (ret < 0) {
		SLSI_ERR(sdev, "scsc_wifi_get_tx_pkt_fates failed ret: %d\n", ret);
		goto exit;
	}

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, 200);
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for Vendor event\n");
		ret = -ENOMEM;
		goto exit;
	}

	if (nla_put(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_NUM, sizeof(provided_count), &provided_count)) {
		SLSI_ERR_NODEV("Failed nla_put\n");
		kfree_skb(skb);
		ret = -EINVAL;
		goto exit;
	}

	ret = cfg80211_vendor_cmd_reply(skb);

	if (ret)
		SLSI_ERR(sdev, "Vendor Command reply failed ret:%d\n", ret);
exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_get_rx_pkt_fates(struct wiphy *wiphy, struct wireless_dev *wdev, const void  *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	int                 ret = 0;
	int                 temp = 0;
	int                 type = 0;
	void __user         *user_buf = NULL;
	u32                 req_count = 0;
	size_t              provided_count = 0;
	struct sk_buff      *skb;
	const struct nlattr *attr;
	u64 val = 0;

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	SLSI_MUTEX_LOCK(sdev->logger_mutex);
	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_NUM:
			if (slsi_util_nla_get_u32(attr, &req_count)) {
				ret = -EINVAL;
				goto exit;
			}
			if (req_count > MAX_FATE_LOG_LEN) {
				SLSI_ERR(sdev, "Found invalid req_count %d for SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_NUM\n", req_count);
				ret = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_DATA:
			if (slsi_util_nla_get_u64(attr, &val)) {
				ret = -EINVAL;
				goto exit;
			}
			user_buf = (void __user *)(unsigned long)(val);
			break;
		default:
			SLSI_ERR(sdev, "Unknown type: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	ret = scsc_wifi_get_rx_pkt_fates(user_buf, req_count, &provided_count, true);
	if (ret < 0) {
		SLSI_ERR(sdev, "scsc_wifi_get_rx_pkt_fates failed ret: %d\n", ret);
		goto exit;
	}

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, 200);
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for Vendor event\n");
		ret = -ENOMEM;
		goto exit;
	}

	if (nla_put(skb, SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_NUM, sizeof(provided_count), &provided_count)) {
		SLSI_ERR_NODEV("Failed nla_put\n");
		kfree_skb(skb);
		ret  = -EINVAL;
		goto exit;
	}

	ret = cfg80211_vendor_cmd_reply(skb);

	if (ret)
		SLSI_ERR(sdev, "Vendor Command reply failed ret:%d\n", ret);
exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

static int slsi_wake_stats_attr_get_cnt_sz(struct slsi_wlan_driver_wake_reason_cnt *wake_reason_count,
					   const void *data, int len)
{
	const struct nlattr *attr;
	int type = 0, temp = 0;

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_WAKE_STATS_CMD_EVENT_WAKE_CNT_SZ:
			if (slsi_util_nla_get_u32(attr, &wake_reason_count->cmd_event_wake_cnt_sz))
				return -EINVAL;
			break;
		case SLSI_ENHANCED_LOGGING_ATTRIBUTE_WAKE_STATS_DRIVER_FW_LOCAL_WAKE_CNT_SZ:
			if (slsi_util_nla_get_u32(attr, &wake_reason_count->driver_fw_local_wake_cnt_sz))
				return -EINVAL;
			break;
		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			return -EINVAL;
		}
	}
	return 0;
}

static int slsi_wake_stats_attr_put_skb_buffer(struct sk_buff *skb,
					       struct slsi_nla_put *put_rule, u32 put_rule_len)
{
	int i = 0, ret = 0;

	for (i = 0; i < put_rule_len; i++) {
		if (put_rule[i].nla_type == NLA_U32) {
			ret = nla_put_u32(skb, put_rule[i].attr, *((u32 *)put_rule[i].value_ptr));
		} else if (put_rule[i].nla_type == NLA_BINARY) {
			ret = nla_put(skb, put_rule[i].attr, put_rule[i].value_len, (u32 *)put_rule[i].value_ptr);
		} else {
			SLSI_ERR_NODEV("Failed nla_put attribute (unsupported type): [%d]\n", put_rule[i].attr);
			return -EINVAL;
		}

		if (ret < 0) {
			SLSI_ERR_NODEV("Failed nla_put attribute type [%d] ret [%d]\n", put_rule[i].attr, ret);
			return -EINVAL;
		}
	}
	return 0;
}

#define SLSI_WAKE_REASON_ATTR(type, attr_name, field, len) {	\
	.nla_type = type,	\
	.attr = SLSI_ENHANCED_LOGGING_ATTRIBUTE_WAKE_STATS_ ## attr_name,	\
	.value_ptr = &wake_reason_count.field,	\
	.value_len = len	\
}

#define SLSI_WAKE_REASON_U32(attr_name, field)	SLSI_WAKE_REASON_ATTR(NLA_U32, attr_name, field, 0)
#define SLSI_WAKE_REASON_ARRAY(attr_name, field, len)	SLSI_WAKE_REASON_ATTR(NLA_BINARY, attr_name, field, len)

static int slsi_get_wake_reason_stats(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	struct slsi_wlan_driver_wake_reason_cnt wake_reason_count;
	int ret = 0;
	struct sk_buff *skb;
	struct slsi_nla_put stats_put_rule[] = {
		SLSI_WAKE_REASON_U32(TOTAL_CMD_EVENT_WAKE, total_cmd_event_wake),
		SLSI_WAKE_REASON_ARRAY(CMD_EVENT_WAKE_CNT_PTR, cmd_event_wake_cnt, 0),
		SLSI_WAKE_REASON_U32(TOTAL_DRIVER_FW_LOCAL_WAKE, total_driver_fw_local_wake),
		SLSI_WAKE_REASON_ARRAY(DRIVER_FW_LOCAL_WAKE_CNT_PTR, driver_fw_local_wake_cnt, 0),
		SLSI_WAKE_REASON_U32(TOTAL_RX_DATA_WAKE, total_rx_data_wake),
		SLSI_WAKE_REASON_U32(RX_UNICAST_CNT, rx_wake_details.rx_unicast_cnt),
		SLSI_WAKE_REASON_U32(RX_MULTICAST_CNT, rx_wake_details.rx_multicast_cnt),
		SLSI_WAKE_REASON_U32(RX_BROADCAST_CNT, rx_wake_details.rx_broadcast_cnt),
		SLSI_WAKE_REASON_U32(ICMP_PKT, rx_wake_pkt_classification_info.icmp_pkt),
		SLSI_WAKE_REASON_U32(ICMP6_PKT, rx_wake_pkt_classification_info.icmp6_pkt),
		SLSI_WAKE_REASON_U32(ICMP6_RA, rx_wake_pkt_classification_info.icmp6_ra),
		SLSI_WAKE_REASON_U32(ICMP6_NA, rx_wake_pkt_classification_info.icmp6_na),
		SLSI_WAKE_REASON_U32(ICMP6_NS, rx_wake_pkt_classification_info.icmp6_ns),
		SLSI_WAKE_REASON_U32(ICMP4_RX_MULTICAST_CNT, rx_multicast_wake_pkt_info.ipv4_rx_multicast_addr_cnt),
		SLSI_WAKE_REASON_U32(ICMP6_RX_MULTICAST_CNT, rx_multicast_wake_pkt_info.ipv6_rx_multicast_addr_cnt),
		SLSI_WAKE_REASON_U32(OTHER_RX_MULTICAST_CNT, rx_multicast_wake_pkt_info.other_rx_multicast_addr_cnt),
	};

	SLSI_DBG3(sdev, SLSI_GSCAN, "\n");
	/* Initialising the wake_reason_count structure values to 0. */
	memset(&wake_reason_count, 0, sizeof(struct slsi_wlan_driver_wake_reason_cnt));

	slsi_spinlock_lock(&sdev->wake_stats_lock);
	wake_reason_count = sdev->wake_reason_stats;
	slsi_spinlock_unlock(&sdev->wake_stats_lock);
	SLSI_MUTEX_LOCK(sdev->logger_mutex);

	ret = slsi_wake_stats_attr_get_cnt_sz(&wake_reason_count, data, len);
	if (ret < 0) {
		SLSI_ERR(sdev, "Failed to get wake reason stats attribute :  %d\n", ret);
		goto exit;
	}

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, NLMSG_DEFAULT_SIZE);
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for Vendor event\n");
		ret = -ENOMEM;
		goto exit;
	}

	ret = slsi_wake_stats_attr_put_skb_buffer(skb, stats_put_rule, ARRAY_SIZE(stats_put_rule));
	if (ret < 0) {
		kfree_skb(skb);
		goto exit;
	}

	ret = cfg80211_vendor_cmd_reply(skb);
	if (ret)
		SLSI_ERR(sdev, "Vendor Command reply failed ret:%d\n", ret);
exit:
	SLSI_MUTEX_UNLOCK(sdev->logger_mutex);
	return ret;
}

#endif /* CONFIG_SCSC_WLAN_ENHANCED_LOGGING */

static int slsi_acs_validate_width_hw_mode(struct slsi_acs_request *request)
{
	if (request->hw_mode != SLSI_ACS_MODE_IEEE80211A && request->hw_mode != SLSI_ACS_MODE_IEEE80211B &&
	    request->hw_mode != SLSI_ACS_MODE_IEEE80211G && request->hw_mode != SLSI_ACS_MODE_IEEE80211ANY)
		return -EINVAL;
	if (request->ch_width != 20 && request->ch_width != 40 && request->ch_width != 80 && request->ch_width != 160)
		return -EINVAL;
	return 0;
}

static int slsi_acs_get_param(struct slsi_dev *sdev, struct slsi_acs_request *request, const void *data,
			      int len, u32 **freq_list, int *freq_list_len)
{
	int temp, type;
	const struct nlattr *attr;

	nla_for_each_attr(attr, data, len, temp) {
		if (!attr)
			return -EINVAL;

		type = nla_type(attr);

		switch (type) {
		case SLSI_ACS_ATTR_HW_MODE:
		{
			if (slsi_util_nla_get_u8(attr, &request->hw_mode))
				return -EINVAL;
			SLSI_INFO(sdev, "ACS hw mode: %d\n", request->hw_mode);
			break;
		}
		case SLSI_ACS_ATTR_CHWIDTH:
		{
			if (slsi_util_nla_get_u16(attr, &request->ch_width))
				return -EINVAL;
			SLSI_INFO(sdev, "ACS ch_width: %d\n", request->ch_width);
			break;
		}
		case SLSI_ACS_ATTR_FREQ_LIST:
		{
			if (*freq_list) /* This check is to avoid Prevent Issue */
				break;

			*freq_list =  kmalloc(nla_len(attr), GFP_KERNEL);
			if (!(*freq_list)) {
				SLSI_ERR(sdev, "No memory for frequency list!");
				return -ENOMEM;
			}
			memcpy(*freq_list, nla_data(attr), nla_len(attr));
			*freq_list_len = nla_len(attr) / sizeof(u32);
			SLSI_INFO(sdev, "ACS freq_list_len: %d\n", *freq_list_len);
			if (*freq_list_len > SLSI_MAX_CHAN_VALUE_ACS)
				*freq_list_len = SLSI_MAX_CHAN_VALUE_ACS;
			break;
		}
		default:
			if (type > SLSI_ACS_ATTR_MAX)
				SLSI_ERR(sdev, "Invalid type : %d\n", type);
			break;
		}
	}

	if (!(*freq_list_len) || slsi_acs_validate_width_hw_mode(request)) {
		SLSI_ERR(sdev, "Invalid freq_list len:%d or ch_width:%d or hw_mode:%d\n", *freq_list_len,
			 request->ch_width, request->hw_mode);
		return -EINVAL;
	}
	return 0;
}

static int slsi_acs_init(struct wiphy *wiphy,
			 struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif;
	struct slsi_acs_request *request;
	u32 *freq_list = NULL;
	int freq_list_len = 0;
	struct ieee80211_channel *channels[SLSI_MAX_CHAN_VALUE_ACS];
	struct slsi_acs_chan_info ch_info[SLSI_MAX_CHAN_VALUE_ACS];
	struct slsi_acs_selected_channels acs_selected_channels;
	int i = 0, num_channels = 0, idx = 0, r = 0;
	u32 chan_flags = (IEEE80211_CHAN_INDOOR_ONLY | IEEE80211_CHAN_RADAR |
			  IEEE80211_CHAN_DISABLED | IEEE80211_CHAN_NO_IR);

	SLSI_INFO(sdev, "SUBCMD_ACS_INIT Received\n");
	if (slsi_is_test_mode_enabled()) {
		SLSI_ERR(sdev, "Not supported in WlanLite mode\n");
		return -EOPNOTSUPP;
	}
	if (wdev->iftype != NL80211_IFTYPE_AP) {
		SLSI_ERR(sdev, "Invalid iftype: %d\n", wdev->iftype);
		return -EINVAL;
	}
	if (!dev) {
		SLSI_ERR(sdev, "Dev not found!\n");
		return -ENODEV;
	}
	request = kcalloc(1, sizeof(*request), GFP_KERNEL);
	if (!request) {
		SLSI_ERR(sdev, "No memory for request!");
		return -ENOMEM;
	}
	ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);
	r = slsi_acs_get_param(sdev, request, data, len, &freq_list, &freq_list_len);
	if (r) {
		kfree(request);
		goto exit;
	}

	memset(channels, 0, sizeof(channels));
	memset(&ch_info, 0, sizeof(ch_info));
	for (i = 0; i < freq_list_len; i++) {
		channels[num_channels] = ieee80211_get_channel(wiphy, freq_list[i]);

		if (!channels[num_channels]) {
			SLSI_INFO(sdev, "Ignore invalid freq:%d MHz in freq list\n", freq_list[i]);
		} else if (channels[num_channels]->flags & chan_flags) {
			SLSI_INFO(sdev, "Skip invalid channel:%d for ACS\n", channels[num_channels]->hw_value);
		} else {
			idx = slsi_find_chan_idx(channels[num_channels]->hw_value, request->hw_mode,
						 channels[num_channels]->band);
			if (idx >= 0 && idx < SLSI_MAX_CHAN_VALUE_ACS) {
				ch_info[idx].chan = channels[num_channels]->hw_value;
				num_channels++;
			}
		}
	}

	if (num_channels == 1) {
		memset(&acs_selected_channels, 0, sizeof(acs_selected_channels));
		acs_selected_channels.ch_width = 20;
		acs_selected_channels.hw_mode = request->hw_mode;
		acs_selected_channels.band = channels[0]->band;
		acs_selected_channels.pri_channel = channels[0]->hw_value;
		r = slsi_send_acs_event(sdev, dev, acs_selected_channels);
		sdev->acs_channel_switched = true;
		kfree(request);
		goto exit;
	}

	if (request->hw_mode == SLSI_ACS_MODE_IEEE80211A) {
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
		SLSI_INFO(sdev, "IEEE80211A band : %d\n", channels[0]->band);
		request->band = channels[0]->band;
		if (request->band == NL80211_BAND_6GHZ)
			request->ch_list_len = SLSI_NUM_6GHZ_CHANNELS;
		else
#endif
			request->ch_list_len = SLSI_NUM_5GHZ_CHANNELS;
	} else if (request->hw_mode == SLSI_ACS_MODE_IEEE80211B ||
		   request->hw_mode == SLSI_ACS_MODE_IEEE80211G) {
		request->ch_list_len = SLSI_NUM_2P4GHZ_CHANNELS;
	} else {
		request->ch_list_len = SLSI_MAX_CHAN_VALUE_ACS;
	}

	memcpy(&request->acs_chan_info[0], &ch_info[0], sizeof(ch_info));
	ndev_vif->scan[SLSI_SCAN_HW_ID].acs_request = request;
	ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan = false;
	r = slsi_mlme_add_scan(sdev,
			       dev,
			       FAPI_SCANTYPE_AP_AUTO_CHANNEL_SELECTION,
			       FAPI_REPORTMODE_REAL_TIME,
			       0,    /* n_ssids */
			       NULL, /* ssids */
			       num_channels,
			       channels,
			       NULL,
			       NULL,		       /* ie */
			       0,		       /* ie_len */
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
			       0,
			       NULL,
			       0,
#endif
			       ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan);
exit:
	kfree(freq_list);
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
	return r;
}

static int slsi_configure_latency_mode(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device   *dev = wdev->netdev;
	int                 temp = 0;
	int                 type = 0;
	const struct nlattr *attr;
	int                 ret = 0;
	int                 low_latency_mode = 0;
	u8                  val = 0;

	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_NL_ATTRIBUTE_LATENCY_MODE:
			if (slsi_util_nla_get_u8(attr, &val)) {
				ret = -EINVAL;
				goto exit;
			}
			low_latency_mode = (int)val;
			break;
		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}
	SLSI_MUTEX_LOCK(sdev->device_config_mutex);
	sdev->device_config.latency_mode = low_latency_mode;
	low_latency_mode = max(sdev->device_config.crt_latency_mode, low_latency_mode);
	SLSI_MUTEX_UNLOCK(sdev->device_config_mutex);
	if (low_latency_mode == 0)
		sdev->soft_roaming_scans_allowed = true;
	else
		sdev->soft_roaming_scans_allowed = false;

	ret = slsi_set_latency_mode(dev, low_latency_mode, len);
	if (ret)
		SLSI_ERR(sdev, "Error in setting low latency mode ret:%d\n", ret);
exit:
	return ret;
}

static u32 slsi_uc_add_ap_channels(struct wiphy *wiphy, enum nl80211_band band,
				   struct slsi_usable_channel *buf, u32 cnt, u32 max_cnt)
{
	u32                             chan_flags;
	int                             i;
	struct ieee80211_channel        *channel = NULL;
	u16                             center_freq;
	struct ieee80211_supported_band *chan_data = wiphy->bands[band];

	if (!chan_data) {
		SLSI_INFO_NODEV("Band %d not supported\n", band);
		return 0;
	}
	chan_flags = (IEEE80211_CHAN_INDOOR_ONLY | IEEE80211_CHAN_RADAR |
		      IEEE80211_CHAN_DISABLED | IEEE80211_CHAN_NO_IR);

	for (i = 0; i < chan_data->n_channels; i++) {
		if (cnt >= max_cnt) {
			SLSI_INFO_NODEV("ap channel count is over MAX_NUM %d STOP finding...\n", cnt);
			break;
		}
		center_freq = chan_data->channels[i].center_freq;
		if (chan_data->channels[i].flags & chan_flags) {
			SLSI_DBG1_NODEV(SLSI_CFG80211, "ap invalid freq %d , chan_flags:0x%x\n", center_freq,
					chan_data->channels[i].flags);
			continue;
		}

		channel = ieee80211_get_channel(wiphy, center_freq);
		if (!channel) {
			SLSI_ERR_NODEV("Invalid frequency %d used to start AP. Channel not found\n",
				       center_freq);
			continue;
		}
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
		if (band == NL80211_BAND_6GHZ &&
		    !cfg80211_channel_is_psc(channel)) {
			SLSI_DBG1_NODEV(SLSI_CFG80211, "Invalid non-PSC freq %d\n", center_freq);
			continue;
		}
#endif
		buf[cnt].freq = center_freq;
		buf[cnt].width = SLSI_LLS_CHAN_WIDTH_20;
		buf[cnt++].iface_mode_mask = SLSI_UC_ITERFACE_SOFTAP;
		SLSI_DBG1_NODEV(SLSI_CFG80211, "ap valid [%d] freq %d , chan_flags:0x%x\n", cnt - 1,
				center_freq, chan_data->channels[i].flags);
	}
	return cnt;
}

static int slsi_get_usable_channels(struct wiphy *wiphy,
				    struct wireless_dev *wdev, const void *data, int len)
{
	int                        ret = 0, type = 0;
	struct slsi_uc_request     request = { 0 };
	struct slsi_dev            *sdev = SDEV_FROM_WIPHY(wiphy);
	int                        temp = 0;
	struct slsi_usable_channel *chan_list;
	u32                        chan_count = 0, mem_len = 0;
	struct sk_buff             *reply;
	const struct nlattr        *attr;

	if (len < SLSI_NL_VENDOR_DATA_OVERHEAD || !data)
		return -EINVAL;

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_UC_ATTRIBUTE_BAND:
			if (slsi_util_nla_get_u32(attr, &request.band)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_UC_ATTRIBUTE_IFACE_MODE:
			if (slsi_util_nla_get_u32(attr, &request.iface_mode)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_UC_ATTRIBUTE_FILTER:
			if (slsi_util_nla_get_u32(attr, &request.filter)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		case SLSI_UC_ATTRIBUTE_MAX_NUM:
			if (slsi_util_nla_get_u32(attr, &request.max_num)) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	if (request.iface_mode == SLSI_UC_ITERFACE_UNKNOWN ||
	    !(request.iface_mode & SLSI_UC_ITERFACE_SOFTAP)) {
		SLSI_ERR_NODEV("iface_mode: %d NOT supported\n", request.iface_mode);
		ret = -EOPNOTSUPP;
		goto exit;
	}

	SLSI_INFO(sdev, "band %u iface_mode %u filter %u max_num %u\n",
		  request.band, request.iface_mode, request.filter, request.max_num);
	if (wiphy->bands[NL80211_BAND_2GHZ])
		mem_len += wiphy->bands[NL80211_BAND_2GHZ]->n_channels * sizeof(struct slsi_usable_channel);

	if (wiphy->bands[NL80211_BAND_5GHZ])
		mem_len += wiphy->bands[NL80211_BAND_5GHZ]->n_channels * sizeof(struct slsi_usable_channel);

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
	if (wiphy->bands[NL80211_BAND_6GHZ])
		mem_len += wiphy->bands[NL80211_BAND_6GHZ]->n_channels * sizeof(struct slsi_usable_channel);
#endif

	if (mem_len == 0) {
		SLSI_ERR_NODEV("bands NOT supported\n");
		ret = -EOPNOTSUPP;
		goto exit;
	}

	chan_list = kmalloc(mem_len, GFP_KERNEL);
	if (!chan_list) {
		ret = -ENOMEM;
		goto exit;
	}
	mem_len += SLSI_NL_VENDOR_REPLY_OVERHEAD + (SLSI_NL_ATTRIBUTE_U32_LEN * 2);
	reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_len);
	if (!reply) {
		ret = -ENOMEM;
		goto exit_with_chan_list;
	}
	if (request.band & SLSI_UC_MAC_2_4_BAND && chan_count < request.max_num)
		chan_count = slsi_uc_add_ap_channels(wiphy, NL80211_BAND_2GHZ, chan_list, chan_count, request.max_num);

	if (request.band & SLSI_UC_MAC_5_BAND && chan_count < request.max_num)
		chan_count += slsi_uc_add_ap_channels(wiphy, NL80211_BAND_5GHZ, chan_list, chan_count, request.max_num);

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
	if (request.band & SLSI_UC_MAC_6_BAND && chan_count < request.max_num)
		chan_count += slsi_uc_add_ap_channels(wiphy, NL80211_BAND_6GHZ, chan_list, chan_count, request.max_num);
#endif

	ret |= nla_put_u32(reply, SLSI_UC_ATTRIBUTE_NUM_CHANNELS, chan_count);
	ret |= nla_put(reply, SLSI_UC_ATTRIBUTE_CHANNEL_LIST,
		       chan_count * sizeof(struct slsi_usable_channel), chan_list);
	if (ret) {
		SLSI_ERR(sdev, "Error in nla_put*:0x%x\n", ret);
		kfree_skb(reply);
		goto exit_with_chan_list;
	}
	ret =  cfg80211_vendor_cmd_reply(reply);

	if (ret)
		SLSI_ERR(sdev, "FAILED to reply GET_USABLE_CHANNELS\n");

exit_with_chan_list:
	kfree(chan_list);
exit:
	return ret;
}

static int slsi_set_dtim_config(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev		*sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device	*dev = wdev->netdev;
	struct netdev_vif	*ndev_vif;
	const struct nlattr	*attr;
	int			ret = 0;
	int			temp = 0;
	int			type = 0;
	int			multiplier = 0;
	u32			val = 0;

	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}
	ndev_vif = netdev_priv(dev);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) {
		SLSI_WARN(sdev, "Not a STA VIF\n");
		ret = -EINVAL;
		goto exit;
	}
	if (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED) {
		SLSI_WARN(sdev, "VIF is not connected\n");
		ret = -EINVAL;
		goto exit;
	}
	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_VENDOR_ATTR_DTIM_MULTIPLIER:
			if (slsi_util_nla_get_u32(attr, &val)) {
				ret = -EINVAL;
				goto exit;
			}
			if (val < 1 || val > 9) {
				ret = -EINVAL;
				goto exit;
			}
			multiplier = (int)val;
			break;
		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}
	SLSI_INFO(sdev, "multiplier %d\n", multiplier);
	if (slsi_set_uint_mib(sdev, NULL, SLSI_PSID_UNIFI_DTIM_MULTIPLIER, multiplier)) {
		SLSI_ERR(sdev, "Set MIB DTIM_MULTIPLIER failed\n");
		ret = -EIO;
		goto exit;
	}
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

#ifdef CONFIG_SCSC_WLAN_SAR_SUPPORTED
static int slsi_select_tx_power_scenario(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device   *dev = wdev->netdev;
	int                 temp = 0;
	int                 type = 0;
	const struct nlattr *attr;
	int                 ret = 0;
	int                 power_scenario = 0;
	u8                  val = 0;

	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}

	nla_for_each_attr(attr, data, len, temp) {
		type = nla_type(attr);
		switch (type) {
		case SLSI_NL_ATTRIBUTE_TX_POWER_SCENARIO:
			if (slsi_util_nla_get_u8(attr, &val)) {
				ret = -EINVAL;
				goto exit;
			}
			power_scenario = (int)val;
			break;
		default:
			SLSI_ERR_NODEV("Unknown attribute: %d\n", type);
			ret = -EOPNOTSUPP;
			goto exit;
		}
	}
	if (power_scenario < 0 || power_scenario > 4) {
		SLSI_ERR_NODEV("Unknown power_scenario: %d\n", power_scenario);
		ret = -EINVAL;
		goto exit;
	}
	if (power_scenario == 0) {
		SLSI_INFO_NODEV("SAR Scenario 0 equivalent to 2 (Voice call)\n");
		power_scenario = 2;
	}
	ret = slsi_configure_tx_power_sar_scenario(dev, power_scenario);
	if (ret)
		SLSI_ERR(sdev, "Error in setting SAR power scenario, ret:%d\n", ret);
exit:
	return ret;
}

static int slsi_reset_tx_power_scenario(struct wiphy *wiphy, struct wireless_dev *wdev, const void *data, int len)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device   *dev = wdev->netdev;
	int                 ret  = 0;
	int                 mode = -1;

	if (!dev) {
		SLSI_ERR(sdev, "dev is NULL!!\n");
		return -EINVAL;
	}

	ret = slsi_configure_tx_power_sar_scenario(dev, mode);
	if (ret)
		SLSI_ERR(sdev, "Error in reset SAR power scenario, ret:%d\n", ret);
	return ret;
}
#endif

void slsi_vendor_delay_wakeup_event(struct slsi_dev *sdev, struct net_device *dev,
				    struct slsi_delayed_wakeup_ind delayed_wakeup_ind)
{
	struct sk_buff *skb = NULL;
	u8 err = 0;
	struct netdev_vif *ndev_vif = netdev_priv(dev);

#if (KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE)
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, &ndev_vif->wdev, NLMSG_DEFAULT_SIZE,
					  SLSI_NL80211_VENDOR_DELAY_WAKEUP_EVENT, GFP_KERNEL);
#else
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, NLMSG_DEFAULT_SIZE,
					  SLSI_NL80211_VENDOR_DELAY_WAKEUP_EVENT, GFP_KERNEL);
#endif
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for delayed wakeup ind\n");
		return;
	}

	err |= nla_put_u8(skb, SLSI_VENDOR_ATTR_DELAYED_WAKEUP_WAKEUP_ID,
			  delayed_wakeup_ind.wakeup_reason);
	err |= nla_put_u8(skb, SLSI_VENDOR_ATTR_DELAYED_WAKEUP_COUNT,
			  delayed_wakeup_ind.delayed_pkt_count);
	err |= nla_put_u8(skb, SLSI_VENDOR_ATTR_DELAYED_WAKEUP_LENGTH,
			  sdev->last_delayd_pkt.pkt_size);
	err |= nla_put(skb, SLSI_VENDOR_ATTR_DELAYED_WAKEUP_PACKET,
		       sdev->last_delayd_pkt.pkt_size, sdev->last_delayd_pkt.pkt);
	if (err) {
		SLSI_ERR_NODEV("Failed nla_put err=%d\n", err);
		kfree_skb(skb);
		return;
	}
	SLSI_DBG1(sdev, SLSI_CFG80211, "Event: SLSI_NL80211_VENDOR_DELAY_WAKEUP_EVENT\n");
	cfg80211_vendor_event(skb, GFP_KERNEL);
}

void slsi_vendor_change_sr_parameter_event(struct slsi_dev *sdev, struct net_device *dev,
					   struct slsi_spatial_reuse_params spatial_reuse_ind)
{
	struct sk_buff *skb = NULL;
	u8 err = 0;
	int event_id = SLSI_NL80211_VENDOR_SPATIAL_REUSE_PARAM_CHANGE_EVENT;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u8 hesiga_spatial_reuse_value15_disallowed = 1;
	u8 non_srg_obss_pd_sr_disallowed = 1;

#if (KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE)
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, &ndev_vif->wdev, NLMSG_DEFAULT_SIZE,
					  event_id, GFP_KERNEL);
#else
	skb = cfg80211_vendor_event_alloc(sdev->wiphy, NLMSG_DEFAULT_SIZE,
					  event_id, GFP_KERNEL);
#endif
	if (!skb) {
		SLSI_ERR_NODEV("Failed to allocate skb for spatial reuse param change ind\n");
		return;
	}

	/* Inverse below values due to host requirements */
	hesiga_spatial_reuse_value15_disallowed = !spatial_reuse_ind.hesiga_sr_value15allowed;
	non_srg_obss_pd_sr_disallowed = !spatial_reuse_ind.non_srg_obss_pd_sr_allowed;

	err |= nla_put_u8(skb, SLSI_VENDOR_ATTR_SRG_OBSS_PD_MIN_OFFSET,
			  spatial_reuse_ind.srg_obss_pd_min_offset);
	err |= nla_put_u8(skb, SLSI_VENDOR_ATTR_SRG_OBSS_PD_MAX_OFFSET,
			  spatial_reuse_ind.srg_obss_pd_max_offset);
	err |= nla_put_u8(skb, SLSI_VENDOR_ATTR_NON_SRG_OBSS_PD_MAX_OFFSET,
			  spatial_reuse_ind.non_srg_obss_pd_max_offset);
	err |= nla_put_u8(skb, SLSI_VENDOR_ATTR_HESIGA_SPATIAL_REUSE_VAL15_DISALLOWED,
			  hesiga_spatial_reuse_value15_disallowed);
	err |= nla_put_u8(skb, SLSI_VENDOR_ATTR_NON_SRG_OBSS_PD_SR_DISALLOWED,
			  non_srg_obss_pd_sr_disallowed);

	if (err) {
		SLSI_ERR_NODEV("Failed nla_put err=%d\n", err);
		kfree_skb(skb);
		return;
	}

	SLSI_DBG1(sdev, SLSI_CFG80211, "Event: SLSI_NL80211_VENDOR_SPATIAL_REUSE_PARAM_CHANGE_EVENT %d %d %d %d %d\n",
		  spatial_reuse_ind.srg_obss_pd_min_offset,
		  spatial_reuse_ind.srg_obss_pd_max_offset,
		  spatial_reuse_ind.non_srg_obss_pd_max_offset,
		  hesiga_spatial_reuse_value15_disallowed,
		  non_srg_obss_pd_sr_disallowed
	);
	cfg80211_vendor_event(skb, GFP_KERNEL);
}

static const struct  nl80211_vendor_cmd_info slsi_vendor_events[] = {
	/**********Deprecated now due to fapi updates.Do not remove*/
	{ OUI_GOOGLE, SLSI_NL80211_SIGNIFICANT_CHANGE_EVENT },
	{ OUI_GOOGLE, SLSI_NL80211_HOTLIST_AP_FOUND_EVENT },
	/******************************************/
	{ OUI_GOOGLE, SLSI_NL80211_SCAN_RESULTS_AVAILABLE_EVENT },
	{ OUI_GOOGLE, SLSI_NL80211_FULL_SCAN_RESULT_EVENT },
	{ OUI_GOOGLE, SLSI_NL80211_SCAN_EVENT },
	/**********Deprecated now due to fapi updates.Do not remove*/
	{ OUI_GOOGLE, SLSI_NL80211_HOTLIST_AP_LOST_EVENT },
	/******************************************/
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_SUBCMD_KEY_MGMT_ROAM_AUTH },
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_HANGED_EVENT },
	{ OUI_GOOGLE,  SLSI_NL80211_EPNO_EVENT },
	{ OUI_GOOGLE,  SLSI_NL80211_HOTSPOT_MATCH },
	{ OUI_GOOGLE,  SLSI_NL80211_RSSI_REPORT_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_LOGGER_RING_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_LOGGER_FW_DUMP_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_RESPONSE_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_PUBLISH_TERMINATED_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_MATCH_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_MATCH_EXPIRED_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_SUBSCRIBE_TERMINATED_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_FOLLOWUP_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_DISCOVERY_ENGINE_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_DISABLED_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_RTT_RESULT_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_RTT_COMPLETE_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_ACS_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_FORWARD_BEACON},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_FORWARD_BEACON_ABORT},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_TRANSMIT_FOLLOWUP_STATUS},
	{ OUI_GOOGLE,  SLSI_NAN_EVENT_NDP_REQ},
	{ OUI_GOOGLE,  SLSI_NAN_EVENT_NDP_CFM},
	{ OUI_GOOGLE,  SLSI_NAN_EVENT_NDP_END},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_RCL_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_POWER_MEASUREMENT_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_CONNECTIVITY_LOG_EVENT},
	{ OUI_GOOGLE,  SLSI_NL80211_SUBSYSTEM_RESTART_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_TWT_SETUP_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_TWT_TEARDOWN_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_TWT_NOTIFICATION_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_SCHED_PM_TEARDOWN_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_SCHED_PM_LEAKY_AP_DETECT_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_NAN_INTERFACE_CREATED_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_NAN_INTERFACE_DELETED_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_DELAY_WAKEUP_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_SPATIAL_REUSE_PARAM_CHANGE_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_MLO_CHANNEL_CONDITION_MEASURE_EVENT},
	{ OUI_SAMSUNG, SLSI_NANSTDP_EVENT_INDICATION},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_MLO_TID_TO_LINK_MAPPING_RESPONSE_EVENT},
	{ OUI_SAMSUNG, SLSI_NL80211_VENDOR_BW_CHANGED_EVENT},
#ifdef CONFIG_SCSC_WIFI_NAN_PAIRING
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_BOOTSTRAPPING_REQ},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_BOOTSTRAPPING_CFM},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_PAIRING_REQ},
	{ OUI_GOOGLE,  SLSI_NL80211_NAN_PAIRING_CFM},
	{ OUI_SAMSUNG, SLSI_NAN_PAIRING_PASN_RECV_MLME_INDICATION}
#endif
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0))
static const struct nla_policy slsi_no_policy[2] = {0};

static const struct nla_policy
slsi_wlan_vendor_acs_policy[SLSI_ACS_ATTR_MAX + 1] = {
	[SLSI_ACS_ATTR_HW_MODE] = {.type = NLA_U8},
	[SLSI_ACS_ATTR_HT_ENABLED] = {.type = NLA_FLAG},
	[SLSI_ACS_ATTR_HT40_ENABLED] = {.type = NLA_FLAG},
	[SLSI_ACS_ATTR_VHT_ENABLED] = {.type = NLA_FLAG },
	[SLSI_ACS_ATTR_CHWIDTH] = {.type = NLA_U16},
	[SLSI_ACS_ATTR_FREQ_LIST] = {.type = NLA_BINARY,
				     .len = (SLSI_MAX_CHAN_VALUE_ACS * sizeof(u32)) },
};

static const struct nla_policy
slsi_wlan_vendor_default_scan_policy[SLSI_SCAN_DEFAULT_MAX + 1] = {
	[SLSI_SCAN_DEFAULT_IE_LEN] = {.type = NLA_U32},
	[SLSI_SCAN_DEFAULT_IES] = {.type = NLA_BINARY},
};

static const struct nla_policy
slsi_wlan_vendor_lls_policy[LLS_ATTRIBUTE_MAX + 1] = {
	[LLS_ATTRIBUTE_SET_MPDU_SIZE_THRESHOLD] = {.type = NLA_U32},
	[LLS_ATTRIBUTE_SET_AGGR_STATISTICS_GATHERING] = {.type = NLA_U32},
	[LLS_ATTRIBUTE_CLEAR_STOP_REQUEST_MASK] = {.type = NLA_U32},
	[LLS_ATTRIBUTE_CLEAR_STOP_REQUEST] = {.type = NLA_U32},
	[LLS_ATTRIBUTE_STATS_VERSION] = {.type = NLA_U32},
	[LLS_ATTRIBUTE_GET_STATS_TYPE] = {.type = NLA_U32},
	[LLS_ATTRIBUTE_GET_STATS_STRUCT] = {.type = NLA_U32},
};

static const struct nla_policy
slsi_wlan_vendor_start_keepalive_offload_policy[MKEEP_ALIVE_ATTRIBUTE_MAX + 1] = {
	[MKEEP_ALIVE_ATTRIBUTE_ID] = {.type = NLA_U8},
	[MKEEP_ALIVE_ATTRIBUTE_IP_PKT] = {.type = NLA_BINARY},
	[MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN] = {.type = NLA_U16},
	[MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC] = {.type = NLA_U32},
	[MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR] = {.type = NLA_BINARY,
						.len = ETH_ALEN},
	[MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR] = {.type = NLA_BINARY,
						.len = ETH_ALEN},
};

static const struct nla_policy
slsi_wlan_vendor_low_latency_policy[SLSI_NL_ATTRIBUTE_LATENCY_MAX + 1] = {
	[SLSI_NL_ATTRIBUTE_LATENCY_MODE] = {.type = NLA_U8},
};

static const struct nla_policy
slsi_wlan_vendor_usable_channels_policy[SLSI_UC_ATTRIBUTE_MAX + 1] = {
	[SLSI_UC_ATTRIBUTE_BAND] = {.type = NLA_U32},
	[SLSI_UC_ATTRIBUTE_IFACE_MODE] = {.type = NLA_U32},
	[SLSI_UC_ATTRIBUTE_FILTER] = {.type = NLA_U32},
	[SLSI_UC_ATTRIBUTE_MAX_NUM] = {.type = NLA_U32},
};

static const struct nla_policy
slsi_wlan_vendor_dtim_policy[SLSI_VENDOR_ATTR_DTIM_MAX + 1] = {
	[SLSI_VENDOR_ATTR_DTIM_MULTIPLIER] = {.type = NLA_U32},
};

#ifdef CONFIG_SCSC_WLAN_SAR_SUPPORTED
static const struct nla_policy
slsi_wlan_vendor_tx_power_scenario_policy[SLSI_NL_ATTRIBUTE_TX_POWER_SCENARIO_MAX + 1] = {
	[SLSI_NL_ATTRIBUTE_TX_POWER_SCENARIO] = {.type = NLA_U8},
};
#endif

static const struct nla_policy
slsi_wlan_vendor_country_code_policy[SLSI_NL_ATTRIBUTE_COUNTRY_CODE_MAX + 1] = {
	[SLSI_NL_ATTRIBUTE_COUNTRY_CODE] = {.type = NLA_BINARY},
};

static const struct nla_policy
slsi_wlan_vendor_roam_state_policy[SLSI_NL_ATTR_ROAM_MAX + 1] = {
	[SLSI_NL_ATTR_ROAM_STATE] = {.type = NLA_U8},
};

static const struct nla_policy
slsi_wlan_vendor_rssi_monitor[SLSI_RSSI_MONITOR_ATTRIBUTE_MAX + 1] = {
	[SLSI_RSSI_MONITOR_ATTRIBUTE_START] = {.type = NLA_U8},
	[SLSI_RSSI_MONITOR_ATTRIBUTE_MIN_RSSI] = {.type = NLA_S8},
	[SLSI_RSSI_MONITOR_ATTRIBUTE_MAX_RSSI] = {.type = NLA_S8},
};

static const struct nla_policy
slsi_wlan_vendor_rtt_policy[SLSI_RTT_ATTRIBUTE_MAX + 1] = {
	[SLSI_RTT_ATTRIBUTE_TARGET_CNT] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_ID] = {.type = NLA_U16},
	[SLSI_RTT_ATTRIBUTE_TARGET_INFO] = {.type = NLA_NESTED_ARRAY,
					    .len = SLSI_RTT_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					    .validation_data = slsi_wlan_vendor_rtt_policy},
#else
						},
#endif
	[SLSI_RTT_ATTRIBUTE_TARGET_MAC] = {.type = NLA_BINARY},
	[SLSI_RTT_ATTRIBUTE_TARGET_TYPE] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_PEER] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_CHAN_FREQ] = {.type = NLA_U16},
	[SLSI_RTT_ATTRIBUTE_TARGET_PERIOD] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_NUM_BURST] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTM] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_NUM_RETRY_FTMR] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_BURST_DURATION] = {.type = NLA_U8},
	[SLSI_RTT_ATTRIBUTE_TARGET_PREAMBLE] = {.type = NLA_U16},
	[SLSI_RTT_ATTRIBUTE_TARGET_BW] = {.type = NLA_U16},
	[SLSI_RTT_ATTRIBUTE_TARGET_LCI] = {.type = NLA_U16},
	[SLSI_RTT_ATTRIBUTE_TARGET_LCR] = {.type = NLA_U16},
};

static const struct nla_policy
slsi_wlan_vendor_apf_filter_policy[SLSI_APF_ATTR_MAX + 1] = {
	[SLSI_APF_ATTR_PROGRAM_LEN] = {.type = NLA_U32},
	[SLSI_APF_ATTR_PROGRAM] = {.type = NLA_BINARY},
};

static const struct nla_policy
slsi_wlan_vendor_gscan_policy[GSCAN_ATTRIBUTE_MAX] = {
	[GSCAN_ATTRIBUTE_NUM_BUCKETS] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BASE_PERIOD] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKETS_BAND] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKET_ID] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKET_PERIOD] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKET_CHANNELS] = {.type = NLA_NESTED},
	[GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_REPORT_THRESHOLD] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_REPORT_THRESHOLD_NUM_SCANS] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKETS_BAND] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_REPORT_EVENTS] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKET_EXPONENT] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKET_STEP_COUNT] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BUCKET_MAX_PERIOD] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_CH_BUCKET_1] = {.type = NLA_NESTED,
					 .len = GSCAN_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					 .validation_data = slsi_wlan_vendor_gscan_policy},
#else
					 },
#endif
	[GSCAN_ATTRIBUTE_CH_BUCKET_2] = {.type = NLA_NESTED,
					 .len = GSCAN_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					 .validation_data = slsi_wlan_vendor_gscan_policy},
#else
					 },
#endif
	[GSCAN_ATTRIBUTE_CH_BUCKET_3] = {.type = NLA_NESTED,
					 .len = GSCAN_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					 .validation_data = slsi_wlan_vendor_gscan_policy},
#else
					 },
#endif
	[GSCAN_ATTRIBUTE_CH_BUCKET_4] = {.type = NLA_NESTED,
					 .len = GSCAN_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					 .validation_data = slsi_wlan_vendor_gscan_policy},
#else
					 },
#endif
	[GSCAN_ATTRIBUTE_CH_BUCKET_5] = {.type = NLA_NESTED,
					 .len = GSCAN_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					 .validation_data = slsi_wlan_vendor_gscan_policy},
#else
					 },
#endif
	[GSCAN_ATTRIBUTE_CH_BUCKET_6] = {.type = NLA_NESTED,
					 .len = GSCAN_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					 .validation_data = slsi_wlan_vendor_gscan_policy},
#else
					 },
#endif
	[GSCAN_ATTRIBUTE_CH_BUCKET_7] = {.type = NLA_NESTED,
					 .len = GSCAN_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					 .validation_data = slsi_wlan_vendor_gscan_policy},
#else
					 },
#endif
	[GSCAN_ATTRIBUTE_CH_BUCKET_8] = {.type = NLA_NESTED,
					 .len = GSCAN_ATTRIBUTE_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
					 .validation_data = slsi_wlan_vendor_gscan_policy},
#else
					 },
#endif
	[GSCAN_ATTRIBUTE_NUM_OF_RESULTS] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_NUM_BSSID] = {.type = NLA_U32},
	[GSCAN_ATTRIBUTE_BLACKLIST_BSSID] = {.type = NLA_BINARY},
	[GSCAN_ATTRIBUTE_BLACKLIST_FROM_SUPPLICANT] = {.type = NLA_U8},
};

static const struct nla_policy
slsi_wlan_vendor_gscan_oui_policy[SLSI_NL_ATTRIBUTE_MAC_OUI_MAX] = {
	[SLSI_NL_ATTRIBUTE_ND_OFFLOAD_VALUE] = {.type = NLA_U8},
	[SLSI_NL_ATTRIBUTE_PNO_RANDOM_MAC_OUI] = {.type = NLA_BINARY},
};

static const struct nla_policy
slsi_wlan_vendor_epno_policy[SLSI_ATTRIBUTE_EPNO_MAX] = {
	[SLSI_ATTRIBUTE_EPNO_MINIMUM_5G_RSSI] = {.type = NLA_U16},
	[SLSI_ATTRIBUTE_EPNO_MINIMUM_2G_RSSI] = {.type = NLA_U16},
	[SLSI_ATTRIBUTE_EPNO_INITIAL_SCORE_MAX] = {.type = NLA_U16},
	[SLSI_ATTRIBUTE_EPNO_CUR_CONN_BONUS] = {.type = NLA_U8},
	[SLSI_ATTRIBUTE_EPNO_SAME_NETWORK_BONUS] = {.type = NLA_U8},
	[SLSI_ATTRIBUTE_EPNO_SECURE_BONUS] = {.type = NLA_U8},
	[SLSI_ATTRIBUTE_EPNO_5G_BONUS] = {.type = NLA_U8},
	[SLSI_ATTRIBUTE_EPNO_SSID_LIST] = {.type = NLA_NESTED},
	[SLSI_ATTRIBUTE_EPNO_SSID_NUM] = {.type = NLA_U8},
};

static const struct nla_policy
slsi_wlan_vendor_epno_hs_policy[SLSI_ATTRIBUTE_EPNO_HS_MAX] = {
	[SLSI_ATTRIBUTE_EPNO_HS_PARAM_LIST] = {.type = NLA_NESTED_ARRAY,
					       .len = SLSI_ATTRIBUTE_EPNO_HS_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
						   .validation_data = slsi_wlan_vendor_epno_hs_policy},
#else
						   },
#endif
	[SLSI_ATTRIBUTE_EPNO_HS_NUM] = {.type = NLA_U8},
	[SLSI_ATTRIBUTE_EPNO_HS_ID] = {.type = NLA_U32},
	[SLSI_ATTRIBUTE_EPNO_HS_REALM] = {.type = NLA_BINARY},
	[SLSI_ATTRIBUTE_EPNO_HS_CONSORTIUM_IDS] = {.type = NLA_BINARY},
	[SLSI_ATTRIBUTE_EPNO_HS_PLMN] = {.type = NLA_BINARY},
};

#ifdef CONFIG_SCSC_WLAN_ENHANCED_LOGGING
static const struct nla_policy
slsi_wlan_vendor_enhanced_logging_policy[SLSI_ENHANCED_LOGGING_ATTRIBUTE_MAX] = {
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_NAME] = {.type = NLA_BINARY},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_VERBOSE_LEVEL] = {.type = NLA_U32},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_RING_FLAGS] = {.type = NLA_U32},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_LOG_MAX_INTERVAL] = {.type = NLA_U32},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_LOG_MIN_DATA_SIZE] = {.type = NLA_U32},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_DUMP_LEN] = {.type = NLA_U32},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_DUMP_DATA] = {.type = NLA_U64},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_DRIVER_DUMP_LEN] = {.type = NLA_U32},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_DRIVER_DUMP_DATA] = {.type = NLA_U64},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_DRIVER_VERSION] = {.type = NLA_BINARY},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_FW_VERSION] = {.type = NLA_BINARY},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_NUM] = {.type = NLA_U32},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_PKT_FATE_DATA] = {.type = NLA_U64},
};

static const struct nla_policy
slsi_wlan_vendor_wake_reason_stats_policy[SLSI_ENHANCED_LOGGING_ATTRIBUTE_WAKE_STATS_MAX] = {
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_WAKE_STATS_CMD_EVENT_WAKE_CNT_SZ] = {.type = NLA_U32},
	[SLSI_ENHANCED_LOGGING_ATTRIBUTE_WAKE_STATS_DRIVER_FW_LOCAL_WAKE_CNT_SZ] = {.type = NLA_U32},
};
#endif

#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
static const struct nla_policy
slsi_wlan_vendor_nan_policy[NAN_REQ_ATTR_MAX + 1] = {
	[NAN_REQ_ATTR_MASTER_PREF] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CLUSTER_LOW] = {.type = NLA_U16},
	[NAN_REQ_ATTR_CLUSTER_HIGH] = {.type = NLA_U16},
	[NAN_REQ_ATTR_HOP_COUNT_LIMIT_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SID_BEACON_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUPPORT_2G4_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUPPORT_5G_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_RSSI_CLOSE_2G4_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_RSSI_MIDDLE_2G4_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_RSSI_PROXIMITY_2G4_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_BEACONS_2G4_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SDF_2G4_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CHANNEL_2G4_MHZ_VAL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_RSSI_PROXIMITY_VAL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_RSSI_CLOSE_5G_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_RSSI_CLOSE_PROXIMITY_5G_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_RSSI_MIDDLE_5G_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_RSSI_PROXIMITY_5G_VAL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_BEACON_5G_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SDF_5G_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CHANNEL_5G_MHZ_VAL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_RSSI_WINDOW_SIZE_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_OUI_VAL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_MAC_ADDR_VAL] = {.type = NLA_BINARY,
				       .len = ETH_ALEN},
	[NAN_REQ_ATTR_CLUSTER_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SOCIAL_CH_SCAN_DWELL_TIME] = {.type = NLA_BINARY,
						    .len = SLSI_HAL_NAN_MAX_SOCIAL_CHANNELS},
	[NAN_REQ_ATTR_SOCIAL_CH_SCAN_PERIOD] = {.type = NLA_BINARY,
						.len = SLSI_HAL_NAN_MAX_SOCIAL_CHANNELS * sizeof(u16)},
	[NAN_REQ_ATTR_RANDOM_FACTOR_FORCE_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_HOP_COUNT_FORCE_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CONN_CAPABILITY_PAYLOAD_TX] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CONN_CAPABILITY_IBSS] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CONN_CAPABILITY_WFD] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CONN_CAPABILITY_WFDS] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CONN_CAPABILITY_TDLS] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CONN_CAPABILITY_MESH] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CONN_CAPABILITY_WLAN_INFRA] = {.type = NLA_U8},
	[NAN_REQ_ATTR_DISCOVERY_ATTR_NUM_ENTRIES] = {.type = NLA_U8},
	[NAN_REQ_ATTR_DISCOVERY_ATTR_VAL] = {.type = NLA_NESTED,
					     .len = NAN_REQ_ATTR_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
						 .validation_data = slsi_wlan_vendor_nan_policy},
#else
						 },
#endif
	[NAN_REQ_ATTR_CONN_TYPE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_NAN_ROLE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_TRANSMIT_FREQ] = {.type = NLA_U8},
	[NAN_REQ_ATTR_AVAILABILITY_DURATION] = {.type = NLA_U8},
	[NAN_REQ_ATTR_AVAILABILITY_INTERVAL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_MESH_ID_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_MESH_ID] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_INFRASTRUCTURE_SSID_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_INFRASTRUCTURE_SSID] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_FURTHER_AVAIL_NUM_ENTRIES] = {.type = NLA_U8},
	[NAN_REQ_ATTR_FURTHER_AVAIL_VAL] = {.type = NLA_NESTED,
					    .len = NAN_REQ_ATTR_MAX,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 8, 0))
						.validation_data = slsi_wlan_vendor_nan_policy},
#else
						},
#endif
	[NAN_REQ_ATTR_FURTHER_AVAIL_ENTRY_CTRL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_FURTHER_AVAIL_CHAN_CLASS] = {.type = NLA_U8},
	[NAN_REQ_ATTR_FURTHER_AVAIL_CHAN] = {.type = NLA_U8},
	[NAN_REQ_ATTR_FURTHER_AVAIL_CHAN_MAPID] = {.type = NLA_U8},
	[NAN_REQ_ATTR_FURTHER_AVAIL_INTERVAL_BITMAP] = {.type = NLA_U32},
	[NAN_REQ_ATTR_PUBLISH_ID] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PUBLISH_TTL] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PUBLISH_PERIOD] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PUBLISH_TYPE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PUBLISH_TX_TYPE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PUBLISH_COUNT] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PUBLISH_SERVICE_NAME_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PUBLISH_SERVICE_NAME] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_PUBLISH_MATCH_ALGO] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PUBLISH_SERVICE_INFO_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PUBLISH_SERVICE_INFO] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_PUBLISH_RX_MATCH_FILTER_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PUBLISH_RX_MATCH_FILTER] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_PUBLISH_TX_MATCH_FILTER_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PUBLISH_TX_MATCH_FILTER] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_PUBLISH_RSSI_THRESHOLD_FLAG] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PUBLISH_CONN_MAP] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PUBLISH_RECV_IND_CFG] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_ID] = {.type = NLA_U16},
	[NAN_REQ_ATTR_SUBSCRIBE_TTL] = {.type = NLA_U16},
	[NAN_REQ_ATTR_SUBSCRIBE_PERIOD] = {.type = NLA_U16},
	[NAN_REQ_ATTR_SUBSCRIBE_TYPE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_RESP_FILTER_TYPE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_RESP_INCLUDE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_USE_RESP_FILTER] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_SSI_REQUIRED] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_MATCH_INDICATOR] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_COUNT] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_SERVICE_NAME_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_SUBSCRIBE_SERVICE_NAME] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_SUBSCRIBE_SERVICE_INFO_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_SUBSCRIBE_SERVICE_INFO] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_SUBSCRIBE_RX_MATCH_FILTER_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_SUBSCRIBE_RX_MATCH_FILTER] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_SUBSCRIBE_TX_MATCH_FILTER_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_SUBSCRIBE_TX_MATCH_FILTER] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_SUBSCRIBE_RSSI_THRESHOLD_FLAG] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_CONN_MAP] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_NUM_INTF_ADDR_PRESENT] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_INTF_ADDR] = {.type = NLA_BINARY,
					      .len = SLSI_HAL_NAN_MAX_SUBSCRIBE_MAX_ADDRESS * ETH_ALEN},
	[NAN_REQ_ATTR_SUBSCRIBE_RECV_IND_CFG] = {.type = NLA_U8},
	[NAN_REQ_ATTR_FOLLOWUP_ID] = {.type = NLA_U16},
	[NAN_REQ_ATTR_FOLLOWUP_REQUESTOR_ID] = {.type = NLA_U32},
	[NAN_REQ_ATTR_FOLLOWUP_ADDR] = {.type = NLA_BINARY,
					.len = ETH_ALEN},
	[NAN_REQ_ATTR_FOLLOWUP_PRIORITY] = {.type = NLA_U8},
	[NAN_REQ_ATTR_FOLLOWUP_SERVICE_NAME_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_FOLLOWUP_SERVICE_NAME] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_FOLLOWUP_TX_WINDOW] = {.type = NLA_U8},
	[NAN_REQ_ATTR_FOLLOWUP_RECV_IND_CFG] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SUBSCRIBE_SID_BEACON_VAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_DW_2G4_INTERVAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_DW_5G_INTERVAL] = {.type = NLA_U8},
	[NAN_REQ_ATTR_DISC_MAC_ADDR_RANDOM_INTERVAL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_PUBLISH_SDEA_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PUBLISH_SDEA] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_RANGING_AUTO_RESPONSE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SDEA_PARAM_NDP_TYPE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SDEA_PARAM_SECURITY_CFG] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SDEA_PARAM_RANGING_STATE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SDEA_PARAM_RANGE_REPORT] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SDEA_PARAM_QOS_CFG] = {.type = NLA_U8},
	[NAN_REQ_ATTR_RANGING_CFG_INTERVAL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_RANGING_CFG_INDICATION] = {.type = NLA_U32},
	[NAN_REQ_ATTR_RANGING_CFG_INGRESS_MM] = {.type = NLA_U32},
	[NAN_REQ_ATTR_RANGING_CFG_EGRESS_MM] = {.type = NLA_U32},
	[NAN_REQ_ATTR_CIPHER_TYPE] = {.type = NLA_U32},
	[NAN_REQ_ATTR_SCID_LEN] = {.type = NLA_U32},
	[NAN_REQ_ATTR_SCID] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_SECURITY_KEY_TYPE] = {.type = NLA_U32},
	[NAN_REQ_ATTR_SECURITY_PMK_LEN] = {.type = NLA_U32},
	[NAN_REQ_ATTR_SECURITY_PMK] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_SECURITY_PASSPHRASE_LEN] = {.type = NLA_U32},
	[NAN_REQ_ATTR_SECURITY_PASSPHRASE] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_RANGE_RESPONSE_CFG_PUBLISH_ID] = {.type = NLA_U16},
	[NAN_REQ_ATTR_RANGE_RESPONSE_CFG_REQUESTOR_ID] = {.type = NLA_U32},
	[NAN_REQ_ATTR_RANGE_RESPONSE_CFG_PEER_ADDR] = {.type = NLA_BINARY,
						       .len = ETH_ALEN},
	[NAN_REQ_ATTR_RANGE_RESPONSE_CFG_RANGING_RESPONSE] = {.type = NLA_U16},
	[NAN_REQ_ATTR_REQ_INSTANCE_ID] = {.type = NLA_U32},
	[NAN_REQ_ATTR_NDP_INSTANCE_ID] = {.type = NLA_U32},
	[NAN_REQ_ATTR_CHAN_REQ_TYPE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_CHAN] = {.type = NLA_U32},
	[NAN_REQ_ATTR_DATA_INTERFACE_NAME_LEN] = {.type = NLA_U8},
	[NAN_REQ_ATTR_DATA_INTERFACE_NAME] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_APP_INFO_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_APP_INFO] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_SERVICE_NAME_LEN] = {.type = NLA_U32},
	[NAN_REQ_ATTR_SERVICE_NAME] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_NDP_RESPONSE_CODE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_USE_NDPE_ATTR] = {.type = NLA_U32},
	[NAN_REQ_ATTR_HAL_TRANSACTION_ID] = {.type = NLA_U16},
	[NAN_REQ_ATTR_CONFIG_DISC_MAC_ADDR_RANDOM] = {.type = NLA_U8},
	[NAN_REQ_ATTR_DISCOVERY_BEACON_INT] = {.type = NLA_U32},
	[NAN_REQ_ATTR_NSS] = {.type = NLA_U32},
	[NAN_REQ_ATTR_ENABLE_RANGING] = {.type = NLA_U32},
	[NAN_REQ_ATTR_DW_EARLY_TERMINATION] = {.type = NLA_U32},
	[NAN_REQ_ATTR_ENABLE_INSTANT_MODE] = {.type = NLA_U32},
	[NAN_REQ_ATTR_INSTANT_MODE_CHANNEL] = {.type = NLA_U32},
	[NAN_REQ_ATTR_SET_COMMAND_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_SET_COMMAND] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_NIK] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_PAIRING_CFG_ENABLE_PAIRING_SETUP] = {.type = NLA_U32},
	[NAN_REQ_ATTR_PAIRING_CFG_ENABLE_PAIRING_CACHE] = {.type = NLA_U32},
	[NAN_REQ_ATTR_PAIRING_CFG_ENABLE_PAIRING_VERIFICATION] = {.type = NLA_U32},
	[NAN_REQ_ATTR_PAIRING_CFG_SUPPORTED_BOOTSTRAPPING_METHODS] = {.type = NLA_U16},
	[NAN_REQ_ATTR_BOOTSTRAPPING_REQUESTOR_INSTANCE_ID] = {.type = NLA_U32},
	[NAN_REQ_ATTR_BOOTSTRAPPING_PEER_DISC_MAC_ADDR] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_BOOTSTRAPPING_REQUEST_BOOTSTRAPPING_METHOD] = {.type = NLA_U16},
	[NAN_REQ_ATTR_BOOTSTRAPPING_SERVICE_SPECIFIC_INFO_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_BOOTSTRAPPING_SERVICE_SPECIFIC_INFO] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_BOOTSTRAPPING_SDEA_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_BOOTSTRAPPING_SDEA] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_BOOTSTRAPPING_SERVICE_INSTANCE_ID] = {.type = NLA_U32},
	[NAN_REQ_ATTR_BOOTSTRAPPING_COME_BACK_DELAY] = {.type = NLA_U32},
	[NAN_REQ_ATTR_BOOTSTRAPPING_COOKIE_LEN] = {.type = NLA_U32},
	[NAN_REQ_ATTR_BOOTSTRAPPING_COOKIE] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_BOOTSTRAPPING_RESPONSE_CODE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PAIRING_REQUESTOR_INSTANCE_ID] = {.type = NLA_U32},
	[NAN_REQ_ATTR_PAIRING_PEER_DISC_MAC_ADDR] = {.type = NLA_BINARY},
	[NAN_REQ_ATTR_PAIRING_REQUEST_TYPE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PAIRING_IS_OPPORTUNISTIC] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PAIRING_ENABLE_PAIRING_CACHE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PAIRING_INSTANCE_ID] = {.type = NLA_U32},
	[NAN_REQ_ATTR_PAIRING_RESPONSE_CODE] = {.type = NLA_U8},
	[NAN_REQ_ATTR_SECURITY_AKM] = {.type = NLA_U8},
	[NAN_REQ_ATTR_PASN_SEND_MLME_LEN] = {.type = NLA_U16},
	[NAN_REQ_ATTR_PASN_SEND_MLME] = {.type = NLA_BINARY},
	[NAN_PASN_SET_KEY_OWN_ADDR] = {.type = NLA_BINARY},
	[NAN_PASN_SET_KEY_PEER_ADDR] = {.type = NLA_BINARY},
	[NAN_PASN_SET_KEY_NM_TK_LEN] = {.type = NLA_U8},
	[NAN_PASN_SET_KEY_NM_TK] = {.type = NLA_BINARY},
	[NAN_PASN_SET_KEY_NM_KEK_LEN] = {.type = NLA_U8},
	[NAN_PASN_SET_KEY_NM_KEK] = {.type = NLA_BINARY},
	[NAN_PASN_SET_KEY_CIPHER] = {.type = NLA_U32},
	[NAN_REQ_ATTR_FOLLOWUP_SHARED_KEY_DESC_FLAG] = {.type = NLA_U8},
};
#endif
#endif

static struct wiphy_vendor_command slsi_vendor_cmd[] = {
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_gscan_get_capabilities
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_VALID_CHANNELS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_gscan_get_valid_channel
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_ADD_GSCAN
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_gscan_add
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_DEL_GSCAN
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_gscan_del
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_SCAN_RESULTS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_gscan_get_scan_results
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_GSCAN_OUI
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_gscan_set_oui
	},
#ifdef CONFIG_SCSC_WLAN_KEY_MGMT_OFFLOAD
	{
		{
			.vendor_id = OUI_SAMSUNG,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_KEY_MGMT_SET_KEY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_key_mgmt_set_pmk
	},
#endif
	{
		{
			.vendor_id = OUI_SAMSUNG,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_DEFAULT_SCAN_IES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_vendor_ie
	},
	{
		{
			.vendor_id = OUI_SAMSUNG,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_BSSID_BLACKLIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_bssid_blacklist
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_BSSID_BLACKLIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_bssid_blacklist
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_START_KEEP_ALIVE_OFFLOAD
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_start_keepalive_offload
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_STOP_KEEP_ALIVE_OFFLOAD
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_stop_keepalive_offload
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_EPNO_LIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_epno_ssid
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_HS_LIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_hs_params
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_RESET_HS_LIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_reset_hs_params
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_RSSI_MONITOR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_rssi_monitor
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_LSTATS_SUBCMD_SET_STATS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_lls_set_stats
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_LSTATS_SUBCMD_GET_STATS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_lls_get_stats
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_LSTATS_SUBCMD_CLEAR_STATS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_lls_clear_stats
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_FEATURE_SET
		},
		.flags = 0,
		.doit = slsi_get_feature_set
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_COUNTRY_CODE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_country_code
	},
#if IS_ENABLED(CONFIG_IPV6)
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_CONFIGURE_ND_OFFLOAD
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_configure_nd_offload
	},
#endif
#ifdef CONFIG_SCSC_WLAN_ENHANCED_LOGGING
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_START_LOGGING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_start_logging
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_RESET_LOGGING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_reset_logging
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_TRIGGER_FW_MEM_DUMP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_trigger_fw_mem_dump
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_FW_MEM_DUMP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_fw_mem_dump
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_TRIGGER_DRIVER_MEM_DUMP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_trigger_driver_mem_dump
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_DRIVER_MEM_DUMP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_driver_mem_dump
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_VERSION
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_version
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_RING_STATUS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_ring_buffers_status
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_RING_DATA
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_ring_data
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_FEATURE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_logger_supported_feature_set
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_START_PKT_FATE_MONITORING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_start_pkt_fate_monitoring
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_TX_PKT_FATES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_tx_pkt_fates
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_RX_PKT_FATES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_rx_pkt_fates
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_WAKE_REASON_STATS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_wake_reason_stats
	},
#endif /* CONFIG_SCSC_WLAN_ENHANCED_LOGGING */
#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_ENABLE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_enable
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_DISABLE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_disable
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_PUBLISH
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_publish
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_PUBLISHCANCEL
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_publish_cancel
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_SUBSCRIBE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_subscribe
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_SUBSCRIBECANCEL
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_subscribe_cancel
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_TXFOLLOWUP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_transmit_followup
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_set_config
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_get_capabilities
	},

	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_INTERFACE_CREATE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_data_iface_create
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_INTERFACE_DELETE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_data_iface_delete
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_REQUEST_INITIATOR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_ndp_initiate
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_INDICATION_RESPONSE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_ndp_respond
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_END
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_ndp_end
	},
	{
		{
			.vendor_id = OUI_SAMSUNG,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NANSTDP_SET_COMMAND
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_vendor_nan_set_command
	},
#ifdef CONFIG_SCSC_WIFI_NAN_PAIRING
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_BOOTSTRAPPING_REQUEST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_bootstrapping_request
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_BOOTSTRAPPING_INDICATION_RESPONSE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_bootstrapping_response
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_PAIRING_REQUEST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_pairing_request
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_PAIRING_INDICATION_RESPONSE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_pairing_response
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_PAIRING_END
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_pairing_end
	},
	{
		{
			.vendor_id = OUI_SAMSUNG,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_PASN_SEND_MLME
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_pasn_send_mlme
	},
	{
		{
			.vendor_id = OUI_SAMSUNG,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_NAN_SET_KEY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_nan_pasn_set_key
	},
#endif
#endif
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_ROAMING_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_roaming_capabilities
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_ROAMING_STATE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_roaming_state
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_RTT_GET_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_rtt_get_capabilities
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_RTT_RANGE_START
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_rtt_set_config
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_RTT_RANGE_CANCEL
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_rtt_cancel_config
	},
	{
		{
			.vendor_id = OUI_SAMSUNG,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_ACS_INIT
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_acs_init
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_APF_GET_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_apf_get_capabilities
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_APF_SET_FILTER
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_apf_set_filter
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_APF_READ_FILTER
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_apf_read_filter
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_LATENCY_MODE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_configure_latency_mode
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_GET_USABLE_CHANNELS
		},
		.flags =  WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_get_usable_channels
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SET_DTIM_CONFIG
		},
		.flags =  WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_set_dtim_config
	},

#ifdef CONFIG_SCSC_WLAN_SAR_SUPPORTED
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_SELECT_TX_POWER_SCENARIO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_select_tx_power_scenario
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = SLSI_NL80211_VENDOR_SUBCMD_RESET_TX_POWER_SCENARIO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = slsi_reset_tx_power_scenario
	},
#endif
};

void slsi_nl80211_vendor_deinit(struct slsi_dev *sdev)
{
	SLSI_DBG2(sdev, SLSI_GSCAN, "De-initialise vendor command and events\n");
	sdev->wiphy->vendor_commands = NULL;
	sdev->wiphy->n_vendor_commands = 0;
	sdev->wiphy->vendor_events = NULL;
	sdev->wiphy->n_vendor_events = 0;

	SLSI_DBG2(sdev, SLSI_GSCAN, "Gscan cleanup\n");
	slsi_gscan_flush_scan_results(sdev);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0))
static void slsi_nll80211_vendor_init_policy(struct wiphy_vendor_command *slsi_vendor_cmd, int n_vendor_commands)
{
	int i;
	struct wiphy_vendor_command *vcmd;

	for (i = 0; i < n_vendor_commands; i++) {
		vcmd = &slsi_vendor_cmd[i];
		switch (vcmd->info.subcmd) {
		case SLSI_NL80211_VENDOR_SUBCMD_GET_CAPABILITIES:
		case SLSI_NL80211_VENDOR_SUBCMD_DEL_GSCAN:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_GET_VALID_CHANNELS:
		case SLSI_NL80211_VENDOR_SUBCMD_ADD_GSCAN:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_SCAN_RESULTS:
			vcmd->policy = slsi_wlan_vendor_gscan_policy;
			vcmd->maxattr = GSCAN_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_SET_GSCAN_OUI:
			vcmd->policy = slsi_wlan_vendor_gscan_oui_policy;
			vcmd->maxattr = SLSI_NL_ATTRIBUTE_MAC_OUI_MAX;
			break;
#ifdef CONFIG_SCSC_WLAN_KEY_MGMT_OFFLOAD
		case SLSI_NL80211_VENDOR_SUBCMD_KEY_MGMT_SET_KEY:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
#endif
		case SLSI_NL80211_VENDOR_SUBCMD_SET_BSSID_BLACKLIST:
			vcmd->policy = slsi_wlan_vendor_gscan_policy;
			vcmd->maxattr = GSCAN_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_START_KEEP_ALIVE_OFFLOAD:
		case SLSI_NL80211_VENDOR_SUBCMD_STOP_KEEP_ALIVE_OFFLOAD:
			vcmd->policy = slsi_wlan_vendor_start_keepalive_offload_policy;
			vcmd->maxattr = MKEEP_ALIVE_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_SET_EPNO_LIST:
			vcmd->policy = slsi_wlan_vendor_epno_policy;
			vcmd->maxattr = SLSI_ATTRIBUTE_EPNO_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_SET_HS_LIST:
			vcmd->policy = slsi_wlan_vendor_epno_hs_policy;
			vcmd->maxattr = SLSI_ATTRIBUTE_EPNO_HS_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_RESET_HS_LIST:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_SET_RSSI_MONITOR:
			vcmd->policy = slsi_wlan_vendor_rssi_monitor;
			vcmd->maxattr = SLSI_RSSI_MONITOR_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_LSTATS_SUBCMD_SET_STATS:
			vcmd->policy = slsi_wlan_vendor_lls_policy;
			vcmd->maxattr = LLS_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_LSTATS_SUBCMD_GET_STATS:
			vcmd->policy = slsi_wlan_vendor_lls_policy;
			vcmd->maxattr = LLS_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_LSTATS_SUBCMD_CLEAR_STATS:
			vcmd->policy = slsi_wlan_vendor_lls_policy;
			vcmd->maxattr = LLS_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_GET_FEATURE_SET:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_SET_COUNTRY_CODE:
			vcmd->policy = slsi_wlan_vendor_country_code_policy;
			vcmd->maxattr = SLSI_NL_ATTRIBUTE_COUNTRY_CODE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_CONFIGURE_ND_OFFLOAD:
			vcmd->policy = slsi_wlan_vendor_gscan_oui_policy;
			vcmd->maxattr = SLSI_NL_ATTRIBUTE_MAC_OUI_MAX;
			break;
#ifdef CONFIG_SCSC_WLAN_ENHANCED_LOGGING
		case SLSI_NL80211_VENDOR_SUBCMD_RESET_LOGGING:
		case SLSI_NL80211_VENDOR_SUBCMD_TRIGGER_FW_MEM_DUMP:
		case SLSI_NL80211_VENDOR_SUBCMD_TRIGGER_DRIVER_MEM_DUMP:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_RING_STATUS:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_FEATURE:
		case SLSI_NL80211_VENDOR_SUBCMD_START_PKT_FATE_MONITORING:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_START_LOGGING:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_FW_MEM_DUMP:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_DRIVER_MEM_DUMP:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_VERSION:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_RING_DATA:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_TX_PKT_FATES:
		case SLSI_NL80211_VENDOR_SUBCMD_GET_RX_PKT_FATES:
			vcmd->policy = slsi_wlan_vendor_enhanced_logging_policy;
			vcmd->maxattr = SLSI_ENHANCED_LOGGING_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_GET_WAKE_REASON_STATS:
			vcmd->policy = slsi_wlan_vendor_wake_reason_stats_policy;
			vcmd->maxattr = SLSI_ENHANCED_LOGGING_ATTRIBUTE_WAKE_STATS_MAX;
			break;
#endif /* CONFIG_SCSC_WLAN_ENHANCED_LOGGING */
#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_ENABLE:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_DISABLE:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_PUBLISH:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_PUBLISHCANCEL:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_SUBSCRIBE:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_SUBSCRIBECANCEL:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_TXFOLLOWUP:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_CONFIG:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_CAPABILITIES:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_INTERFACE_CREATE:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_INTERFACE_DELETE:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_REQUEST_INITIATOR:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_INDICATION_RESPONSE:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_DATA_END:
		case SLSI_NL80211_VENDOR_SUBCMD_NANSTDP_SET_COMMAND:
#ifdef CONFIG_SCSC_WIFI_NAN_PAIRING
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_BOOTSTRAPPING_REQUEST:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_BOOTSTRAPPING_INDICATION_RESPONSE:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_PAIRING_REQUEST:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_PAIRING_INDICATION_RESPONSE:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_PAIRING_END:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_PASN_SEND_MLME:
		case SLSI_NL80211_VENDOR_SUBCMD_NAN_SET_KEY:
#endif
			vcmd->policy = slsi_wlan_vendor_nan_policy;
			vcmd->maxattr = NAN_REQ_ATTR_MAX;
			break;
#endif
		case SLSI_NL80211_VENDOR_SUBCMD_GET_ROAMING_CAPABILITIES:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_SET_ROAMING_STATE:
			vcmd->policy = slsi_wlan_vendor_roam_state_policy;
			vcmd->maxattr = SLSI_NL_ATTR_ROAM_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_RTT_GET_CAPABILITIES:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_RTT_RANGE_START:
		case SLSI_NL80211_VENDOR_SUBCMD_RTT_RANGE_CANCEL:
			vcmd->policy = slsi_wlan_vendor_rtt_policy;
			vcmd->maxattr = SLSI_RTT_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_ACS_INIT:
			vcmd->policy = slsi_wlan_vendor_acs_policy;
			vcmd->maxattr = SLSI_ACS_ATTR_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_DEFAULT_SCAN_IES:
			vcmd->policy = slsi_wlan_vendor_default_scan_policy;
			vcmd->maxattr = SLSI_SCAN_DEFAULT_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_APF_GET_CAPABILITIES:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_APF_SET_FILTER:
			vcmd->policy = slsi_wlan_vendor_apf_filter_policy;
			vcmd->maxattr = SLSI_APF_ATTR_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_APF_READ_FILTER:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_SET_LATENCY_MODE:
			vcmd->policy = slsi_wlan_vendor_low_latency_policy;
			vcmd->maxattr = SLSI_NL_ATTRIBUTE_LATENCY_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_GET_USABLE_CHANNELS:
			vcmd->policy = slsi_wlan_vendor_usable_channels_policy;
			vcmd->maxattr = SLSI_UC_ATTRIBUTE_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_SET_DTIM_CONFIG:
			vcmd->policy = slsi_wlan_vendor_dtim_policy;
			vcmd->maxattr = SLSI_VENDOR_ATTR_DTIM_MAX;
			break;
#ifdef CONFIG_SCSC_WLAN_SAR_SUPPORTED
		case SLSI_NL80211_VENDOR_SUBCMD_SELECT_TX_POWER_SCENARIO:
			vcmd->policy = slsi_wlan_vendor_tx_power_scenario_policy;
			vcmd->maxattr = SLSI_NL_ATTRIBUTE_TX_POWER_SCENARIO_MAX;
			break;
		case SLSI_NL80211_VENDOR_SUBCMD_RESET_TX_POWER_SCENARIO:
			vcmd->policy = VENDOR_CMD_RAW_DATA;
			vcmd->maxattr = 0;
			break;
#endif
		}
	}
}
#endif

void slsi_nl80211_vendor_init(struct slsi_dev *sdev)
{
	int i;

	SLSI_DBG2(sdev, SLSI_GSCAN, "Init vendor command and events\n");

	sdev->wiphy->vendor_commands = (const struct wiphy_vendor_command *)slsi_vendor_cmd;
	sdev->wiphy->n_vendor_commands = ARRAY_SIZE(slsi_vendor_cmd);
	sdev->wiphy->vendor_events = slsi_vendor_events;
	sdev->wiphy->n_vendor_events = ARRAY_SIZE(slsi_vendor_events);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0))
	slsi_nll80211_vendor_init_policy(slsi_vendor_cmd, sdev->wiphy->n_vendor_commands);
#endif

	for (i = 0; i < SLSI_GSCAN_MAX_BUCKETS; i++)
		sdev->bucket[i].scan_id = (SLSI_GSCAN_SCAN_ID_START + i);

	for (i = 0; i < SLSI_GSCAN_HASH_TABLE_SIZE; i++)
		sdev->gscan_hash_table[i] = NULL;

	INIT_LIST_HEAD(&sdev->hotlist_results);
}

#if defined(CONFIG_SCSC_WLAN_TAS)
#define SLSI_TAS_FAMILY_NAME "slsi_nl_tas_fam"
#define SLSI_TAS_GROUP_NAME  "slsi_nl_tas_grp"

enum slsi_tas_cmd {
	SLSI_TAS_CMD_SAR_IND = 1,
	SLSI_TAS_CMD_SAR_REQ,
	SLSI_TAS_CMD_IF_STATUS,
	SLSI_TAS_CMD_SHORT_WIN_NUM_REQ,
	SLSI_TAS_CMD_GET_CONFIG,
	SLSI_TAS_CMD_UPDATE_SAR_LIMIT_UPPER,
	SLSI_TAS_CMD_REQUEST_NOTIFICATION,
	SLSI_TAS_CMD_UPDATE_SAR_TARGET,
	SLSI_TAS_CMD_MAX,
};

enum slsi_tas_attr {
	SLSI_TAS_ATTR_SHORT_WIN_NUM = 1,
	SLSI_TAS_ATTR_AVG_SAR,
	SLSI_TAS_ATTR_TIMESTAMP,
	SLSI_TAS_ATTR_TX_SAR_LIMIT,
	SLSI_TAS_ATTR_CTRL_BACKOFF,
	SLSI_TAS_ATTR_IF_TYPE,
	SLSI_TAS_ATTR_IF_ENABLED,
	SLSI_TAS_ATTR_SAR_LIMIT_UPPER,
	SLSI_TAS_ATTR_IF_STATUS_ENTRIES,
	SLSI_TAS_ATTR_SAR_COMPLIANCE,
	SLSI_TAS_ATTR_RF_TEST_MODE,
	SLSI_TAS_ATTR_MAX,
};

static struct genl_family slsi_tas_fam;
static bool is_req_noti;

static int slsi_tas_tx_sar_limit_req(struct sk_buff *skb, struct genl_info *info)
{
	struct slsi_dev *sdev = slsi_get_sdev();
	struct slsi_tas_info *tas_info = NULL;
	struct tas_sar_param sar_param = {0};
	int err = 0;

	if (!sdev)
		return -ENODEV;
	tas_info = &sdev->tas_info;

	if (!info || !info->attrs[SLSI_TAS_ATTR_TX_SAR_LIMIT] ||
	    !info->attrs[SLSI_TAS_ATTR_SHORT_WIN_NUM] ||
	    !info->attrs[SLSI_TAS_ATTR_CTRL_BACKOFF]) {
		err = -EINVAL;
		goto release_lock;
	}

	sar_param.win_num = nla_get_u16(info->attrs[SLSI_TAS_ATTR_SHORT_WIN_NUM]);
	if (nla_get_u8(info->attrs[SLSI_TAS_ATTR_CTRL_BACKOFF]))
		sar_param.flags |= SLSI_TAS_SET_CTRL_BACKOFF;
	sar_param.sar_limit = nla_get_u16(info->attrs[SLSI_TAS_ATTR_TX_SAR_LIMIT]);

	slsi_mlme_tas_tx_sar_limit(sdev, &sar_param);

release_lock:
	if (slsi_wake_lock_active(&tas_info->wlan_wl_tas))
		slsi_wake_unlock(&tas_info->wlan_wl_tas);

	return err;
}

static int slsi_tas_short_win_num_req(struct sk_buff *skb, struct genl_info *info)
{
	struct slsi_dev *sdev = slsi_get_sdev();
	struct tas_sar_param sar_param = {0};

	if (!sdev)
		return -ENODEV;

	if (!info || !info->attrs[SLSI_TAS_ATTR_TX_SAR_LIMIT] || !info->attrs[SLSI_TAS_ATTR_SHORT_WIN_NUM])
		return -EINVAL;

	sar_param.win_num = nla_get_u16(info->attrs[SLSI_TAS_ATTR_SHORT_WIN_NUM]);
	sar_param.flags = 0;
	sar_param.sar_limit = nla_get_u16(info->attrs[SLSI_TAS_ATTR_TX_SAR_LIMIT]);

	slsi_mlme_tas_set_short_win_num(sdev, &sar_param);
	return 0;
}

static int slsi_tas_if_status_entry(struct sk_buff *msg, enum slsi_tas_if_type type, bool enabled)
{
	struct nlattr *attr = NULL;

	attr = nla_nest_start(msg, type);
	if (!attr)
		return -EMSGSIZE;

	if (nla_put_u8(msg, SLSI_TAS_ATTR_IF_TYPE, type) ||
	    nla_put_u8(msg, SLSI_TAS_ATTR_IF_ENABLED, enabled)) {
		nla_nest_cancel(msg, attr);
		return -EMSGSIZE;
	}

	nla_nest_end(msg, attr);
	return 0;
}

static int slsi_tas_fill_config(struct sk_buff *msg, struct genl_info *info)
{
	struct slsi_dev *sdev = slsi_get_sdev();
	struct slsi_tas_info *tas_info = NULL;
	void *hdr = NULL;
	struct nlattr *attr = NULL;
	int type = 0;

	if (!sdev)
		return -ENODEV;
	tas_info = &sdev->tas_info;

	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &slsi_tas_fam, 0, SLSI_TAS_CMD_GET_CONFIG);
	if (!hdr) {
		SLSI_ERR_NODEV("genlmsg_put failed\n");
		return -EMSGSIZE;
	}

	if (nla_put_u16(msg, SLSI_TAS_ATTR_SAR_LIMIT_UPPER, tas_info->sar_limit_upper) ||
		nla_put_u16(msg, SLSI_TAS_ATTR_SAR_COMPLIANCE, tas_info->sar_compliance) ||
		nla_put_u8(msg, SLSI_TAS_ATTR_RF_TEST_MODE, slsi_is_rf_test_mode_enabled()))
		goto nla_put_failure;

	attr = nla_nest_start(msg, SLSI_TAS_ATTR_IF_STATUS_ENTRIES);
	if (!attr)
		goto nla_put_failure;

	for (type = SLSI_TAS_IF_TYPE_NONE + 1; type < SLSI_TAS_IF_TYPE_MAX; type++) {
		if (slsi_tas_if_status_entry(msg, type, tas_info->if_enabled[type]))
			goto nest_put_failure;
	}

	nla_nest_end(msg, attr);
	genlmsg_end(msg, hdr);
	return 0;

nest_put_failure:
	nla_nest_cancel(msg, attr);
nla_put_failure:
	genlmsg_cancel(msg, hdr);
	return -EMSGSIZE;
}

static int slsi_tas_config_req(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *msg = NULL;
	int ret = 0;

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg) {
		SLSI_ERR_NODEV("No memory\n");
		return -ENOMEM;
	}

	ret = slsi_tas_fill_config(msg, info);
	if (ret) {
		SLSI_ERR_NODEV("Msg error\n");
		nlmsg_free(msg);
		return ret;
	}
	return genlmsg_reply(msg, info);
}

static int slsi_tas_notification_req(struct sk_buff *skb, struct genl_info *info)
{
	is_req_noti = true;
	return 0;
}

static const struct genl_small_ops slsi_tas_ops[] = {
	{
		.cmd = SLSI_TAS_CMD_SAR_REQ,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit = slsi_tas_tx_sar_limit_req,
	},
	{
		.cmd = SLSI_TAS_CMD_SHORT_WIN_NUM_REQ,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit = slsi_tas_short_win_num_req,
	},
	{
		.cmd = SLSI_TAS_CMD_GET_CONFIG,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit = slsi_tas_config_req,
	},
	{
		.cmd = SLSI_TAS_CMD_REQUEST_NOTIFICATION,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit = slsi_tas_notification_req,
	},
};

static struct nla_policy slsi_tas_policy[SLSI_TAS_ATTR_MAX + 1] = {
	[SLSI_TAS_ATTR_SHORT_WIN_NUM] = { .type = NLA_U16 },
	[SLSI_TAS_ATTR_AVG_SAR] = { .type = NLA_U16 },
	[SLSI_TAS_ATTR_TIMESTAMP] = { .type = NLA_U32 },
	[SLSI_TAS_ATTR_TX_SAR_LIMIT] = { .type = NLA_U16 },
	[SLSI_TAS_ATTR_CTRL_BACKOFF] = { .type = NLA_U8 },
	[SLSI_TAS_ATTR_IF_TYPE] = { .type = NLA_U8 },
	[SLSI_TAS_ATTR_IF_ENABLED] = { .type = NLA_U8 },
	[SLSI_TAS_ATTR_SAR_LIMIT_UPPER] = { .type = NLA_U16 },
	[SLSI_TAS_ATTR_IF_STATUS_ENTRIES] = { .type = NLA_NESTED },
	[SLSI_TAS_ATTR_SAR_COMPLIANCE] = { .type = NLA_U16 },
};

static const struct genl_multicast_group slsi_tas_mcgrp[] = {
	{ .name = SLSI_TAS_GROUP_NAME, },
};

static struct genl_family slsi_tas_fam __ro_after_init = {
	.name = SLSI_TAS_FAMILY_NAME,
	.hdrsize = 0,
	.version = 1,
	.maxattr = SLSI_TAS_ATTR_MAX,
	.policy = slsi_tas_policy,
	.netnsok = true,
	.module = THIS_MODULE,
	.small_ops = slsi_tas_ops,
	.n_small_ops = ARRAY_SIZE(slsi_tas_ops),
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	.resv_start_op = SLSI_TAS_CMD_MAX,
#endif
	.mcgrps = slsi_tas_mcgrp,
	.n_mcgrps = ARRAY_SIZE(slsi_tas_mcgrp),
};

static int slsi_tas_fill_sar_ind(struct sk_buff *msg, u16 win_num, u16 sar)
{
	void *hdr;

	hdr = genlmsg_put(msg, 0, 0, &slsi_tas_fam, 0, SLSI_TAS_CMD_SAR_IND);
	if (!hdr) {
		SLSI_ERR_NODEV("genlmsg_put failed\n");
		return -EMSGSIZE;
	}

	if (nla_put_u16(msg, SLSI_TAS_ATTR_SHORT_WIN_NUM, win_num) ||
	    nla_put_u16(msg, SLSI_TAS_ATTR_AVG_SAR, sar)) {
		SLSI_ERR_NODEV("attr put failed\n");
		genlmsg_cancel(msg, hdr);
		return -EMSGSIZE;
	}

	genlmsg_end(msg, hdr);
	return 0;
}

static bool is_support_notification(void)
{
	/* Need to block notification when tasd is disabled */
	return is_req_noti;
}

#define SLSI_TAS_WAKELOCK_TIME_OUT_IN_MS   (100)
int slsi_tas_notify_sar_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	struct slsi_tas_info *tas_info = &sdev->tas_info;
	struct sk_buff *msg = NULL;
	u16 win_num = 0, sar = 0;
	int ret = 0;

	if (!is_support_notification()) {
		kfree_skb(skb);
		return ret;
	}

	win_num = fapi_get_u16(skb, u.mlme_sar_ind.short_window_number);
	sar = fapi_get_u16(skb, u.mlme_sar_ind.sar);

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg) {
		SLSI_ERR(sdev, "No memory\n");
		kfree_skb(skb);
		return -ENOMEM;
	}

	ret = slsi_tas_fill_sar_ind(msg, win_num, sar);
	if (ret) {
		nlmsg_free(msg);
		kfree_skb(skb);
		return ret;
	}

	ret = genlmsg_multicast_allns(&slsi_tas_fam, msg, 0, 0, GFP_KERNEL);
	if (ret)
		SLSI_ERR(sdev, "genlmsg_multicast_allns failed : [%d]\n", ret);

	slsi_wake_lock_timeout(&tas_info->wlan_wl_tas, msecs_to_jiffies(SLSI_TAS_WAKELOCK_TIME_OUT_IN_MS));

	kfree_skb(skb);
	return ret;
}

static int slsi_tas_fill_if_status(struct sk_buff *msg, enum slsi_tas_if_type type, bool enabled)
{
	struct slsi_dev *sdev = slsi_get_sdev();
	struct slsi_tas_info *tas_info = NULL;
	void *hdr;

	if (!sdev)
		return -ENODEV;
	tas_info = &sdev->tas_info;

	hdr = genlmsg_put(msg, 0, 0, &slsi_tas_fam, 0, SLSI_TAS_CMD_IF_STATUS);
	if (!hdr) {
		SLSI_ERR_NODEV("genlmsg_put failed\n");
		return -EMSGSIZE;
	}

	if (nla_put_u8(msg, SLSI_TAS_ATTR_IF_TYPE, type) ||
	    nla_put_u8(msg, SLSI_TAS_ATTR_IF_ENABLED, enabled)) {
		genlmsg_cancel(msg, hdr);
		return -EMSGSIZE;
	}

	if (type == SLSI_TAS_IF_TYPE_WIFI && enabled) {
		if (nla_put_u16(msg, SLSI_TAS_ATTR_SAR_LIMIT_UPPER, tas_info->sar_limit_upper) ||
		    nla_put_u16(msg, SLSI_TAS_ATTR_SAR_COMPLIANCE, tas_info->sar_compliance) ||
		    nla_put_u8(msg, SLSI_TAS_ATTR_RF_TEST_MODE, slsi_is_rf_test_mode_enabled())) {
			genlmsg_cancel(msg, hdr);
			return -EMSGSIZE;
		}
	}

	genlmsg_end(msg, hdr);
	return 0;
}

static void slsi_tas_notify_if_status(enum slsi_tas_if_type type, bool enabled)
{
	struct slsi_dev *sdev = slsi_get_sdev();
	struct slsi_tas_info *tas_info = NULL;
	struct sk_buff *msg = NULL;
	int ret = 0;

	if (!sdev)
		return;
	tas_info = &sdev->tas_info;

	if (!is_support_notification())
		return;

	if (!(type > SLSI_TAS_IF_TYPE_NONE && type < SLSI_TAS_IF_TYPE_MAX)) {
		SLSI_ERR_NODEV("Invalid type : [%d]\n", type);
		return;
	}

	tas_info->if_enabled[type] = enabled;

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg) {
		SLSI_ERR_NODEV("No memory\n");
		return;
	}

	ret = slsi_tas_fill_if_status(msg, type, enabled);
	if (ret) {
		nlmsg_free(msg);
		return;
	}

	ret = genlmsg_multicast_allns(&slsi_tas_fam, msg, 0, 0, GFP_KERNEL);
	if (ret)
		SLSI_ERR_NODEV("genlmsg_multicast_allns failed : [%d]\n", ret);
}

void slsi_tas_notify_wifi_status(bool enabled)
{
	slsi_tas_notify_if_status(SLSI_TAS_IF_TYPE_WIFI, enabled);
}

static int slsi_tas_fill_sar_limit_upper(struct sk_buff *msg, u16 sar_limit_upper)
{
	void *hdr;

	hdr = genlmsg_put(msg, 0, 0, &slsi_tas_fam, 0, SLSI_TAS_CMD_UPDATE_SAR_LIMIT_UPPER);
	if (!hdr) {
		SLSI_ERR_NODEV("genlmsg_put failed\n");
		return -EMSGSIZE;
	}

	if (nla_put_u16(msg, SLSI_TAS_ATTR_SAR_LIMIT_UPPER, sar_limit_upper)) {
		SLSI_ERR_NODEV("attr put failed\n");
		genlmsg_cancel(msg, hdr);
		return -EMSGSIZE;
	}

	genlmsg_end(msg, hdr);
	return 0;
}

int slsi_tas_notify_sar_limit_upper(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	struct slsi_tas_info *tas_info = &sdev->tas_info;
	struct sk_buff *msg = NULL;
	int ret = 0;

	if (!is_support_notification()) {
		kfree_skb(skb);
		return ret;
	}

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg) {
		SLSI_ERR_NODEV("No memory\n");
		kfree_skb(skb);
		return -ENOMEM;
	}

	tas_info->sar_limit_upper = fapi_get_u16(skb, u.mlme_sar_limit_upper_ind.sar_limit_upper);

	ret = slsi_tas_fill_sar_limit_upper(msg, tas_info->sar_limit_upper);
	if (ret) {
		nlmsg_free(msg);
		kfree_skb(skb);
		return ret;
	}

	ret = genlmsg_multicast_allns(&slsi_tas_fam, msg, 0, 0, GFP_KERNEL);
	if (ret)
		SLSI_ERR_NODEV("genlmsg_multicast_allns failed : [%d]\n", ret);

	kfree_skb(skb);
	return ret;
}

static int slsi_tas_fill_sar_target(struct sk_buff *msg, u16 sar_target)
{
	void *hdr;

	hdr = genlmsg_put(msg, 0, 0, &slsi_tas_fam, 0, SLSI_TAS_CMD_UPDATE_SAR_TARGET);
	if (!hdr) {
		SLSI_ERR_NODEV("genlmsg_put failed\n");
		return -EMSGSIZE;
	}

	if (nla_put_u16(msg, SLSI_TAS_ATTR_SAR_COMPLIANCE, sar_target)) {
		SLSI_ERR_NODEV("attr put failed\n");
		genlmsg_cancel(msg, hdr);
		return -EMSGSIZE;
	}

	genlmsg_end(msg, hdr);
	return 0;
}

int slsi_tas_notify_sar_target(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	struct slsi_tas_info *tas_info = &sdev->tas_info;
	struct sk_buff *msg = NULL;
	int ret = 0;

	if (!is_support_notification()) {
		kfree_skb(skb);
		return ret;
	}

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg) {
		SLSI_ERR_NODEV("No memory\n");
		kfree_skb(skb);
		return -ENOMEM;
	}

	tas_info->sar_compliance = fapi_get_u16(skb, u.mlme_sar_target_ind.sar_target);

	ret = slsi_tas_fill_sar_target(msg, tas_info->sar_compliance);
	if (ret) {
		nlmsg_free(msg);
		kfree_skb(skb);
		return ret;
	}

	ret = genlmsg_multicast_allns(&slsi_tas_fam, msg, 0, 0, GFP_KERNEL);
	if (ret)
		SLSI_ERR_NODEV("genlmsg_multicast_allns failed : [%d]\n", ret);

	kfree_skb(skb);
	return ret;
}

void slsi_tas_nl_deinit(void)
{
	SLSI_INFO_NODEV("TAS nl deinit\n");
	genl_unregister_family(&slsi_tas_fam);
}

void slsi_tas_nl_init(void)
{
	int err;

	SLSI_INFO_NODEV("TAS nl init\n");
	is_req_noti = false;
	err = genl_register_family(&slsi_tas_fam);
	if (err)
		SLSI_ERR_NODEV("genl_register_family failed : [%d]\n", err);
}
#endif

