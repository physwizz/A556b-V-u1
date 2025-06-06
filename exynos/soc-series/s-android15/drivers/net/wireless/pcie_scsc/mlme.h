/****************************************************************************
 *
 * Copyright (c) 2012 - 2023 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __SLSI_MLME_H__
#define __SLSI_MLME_H__

#include "dev.h"
#include "mib.h"

enum slsi_ac_index_wmm_pe {
	AC_BE,
	AC_BK,
	AC_VI,
	AC_VO
};

#define SLSI_FREQ_FW_TO_HOST(f) ((f) / 2)
#define SLSI_FREQ_HOST_TO_FW(f) ((f) * 2)

#define SLSI_SINFO_MIB_ACCESS_TIMEOUT (1 * HZ) /* 1 sec timeout */

#define SLSI_WLAN_EID_VENDOR_SPECIFIC 0xdd
#define SLSI_WLAN_EID_INTERWORKING 107
#define SLSI_WLAN_EID_EXTENSION 255

/* Element ID Extension (EID 255) values */
#define SLSI_WLAN_EID_EXT_OWE_DH_PARAM 32

#define SLSI_WLAN_OUI_TYPE_WFA_HS20_IND 0x10
#define SLSI_WLAN_OUI_TYPE_WFA_OSEN 0x12
#define SLSI_WLAN_OUI_TYPE_WFA_MBO 0x16

#define SLSI_WLAN_FAIL_WORK_TIMEOUT  1500 /* 1.5s timeout */

/*Extended capabilities bytes*/
#define SLSI_WLAN_EXT_CAPA2_BSS_TRANSISITION_ENABLED  (BIT(3))
#define SLSI_WLAN_EXT_CAPA3_INTERWORKING_ENABLED        (BIT(7))
#define SLSI_WLAN_EXT_CAPA4_QOS_MAP_ENABLED                  (BIT(0))
#define SLSI_WLAN_EXT_CAPA5_WNM_NOTIF_ENABLED              (BIT(6))
#define SLSI_WLAN_EXT_CAPA2_QBSS_LOAD_ENABLED                  BIT(7)
#define SLSI_WLAN_EXT_CAPA1_PROXY_ARP_ENABLED                  BIT(4)
#define SLSI_WLAN_EXT_CAPA2_TFS_ENABLED                              BIT(0)
#define SLSI_WLAN_EXT_CAPA2_WNM_SLEEP_ENABLED                 BIT(1)
#define SLSI_WLAN_EXT_CAPA2_TIM_ENABLED                              BIT(2)
#define SLSI_WLAN_EXT_CAPA2_DMS_ENABLED                             BIT(4)

/*RM Enabled Capabilities Bytes*/
#define SLSI_WLAN_RM_CAPA0_LINK_MEASUREMENT_ENABLED       BIT(0)
#define SLSI_WLAN_RM_CAPA0_NEIGHBOR_REPORT_ENABLED         BIT(1)
#define SLSI_WLAN_RM_CAPA0_PASSIVE_MODE_ENABLED              BIT(4)
#define SLSI_WLAN_RM_CAPA0_ACTIVE_MODE_ENABLED	               BIT(5)
#define SLSI_WLAN_RM_CAPA0_TABLE_MODE_ENABLED                 BIT(6)

/* EID (1) + Len (1) + Ext Capab (12) */
#define SLSI_AP_EXT_CAPAB_IE_LEN_MAX 14

#if (KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE) || defined(CONFIG_SCSC_WLAN_BSS_COLOR)
#define SLSI_WLAN_HE_BSS_COLOR_IE_LEN 8
#define SLSI_WLAN_HE_BSS_COLOR_PARTIAL_OFFSET 6
#define SLSI_WLAN_HE_BSS_COLOR_ENABLE_DISABLE_INDEX 7
#define SLSI_WLAN_HE_BSS_COLOR_DISABLED BIT(7)
#endif

#define SLSI_SCAN_DONE_IND_WAIT_TIMEOUT 40000 /* 40 seconds */
#define SLSI_NAN_START_TIMEOUT 9000 /* 9 seconds */

/* WLAN_EID_COUNTRY available from kernel version 3.7 */
#ifndef WLAN_EID_COUNTRY
#define WLAN_EID_COUNTRY 7
#endif

/* P2P (Wi-Fi Direct) */
#define SLSI_P2P_WILDCARD_SSID "DIRECT-"
#define SLSI_P2P_WILDCARD_SSID_LENGTH 7
#define SLSI_P2P_SOCIAL_CHAN_COUNT      3
#define SLSI_P2P_SOCIAL_CHAN_1	1
#define SLSI_P2P_SOCIAL_CHAN_6	6
#define SLSI_P2P_SOCIAL_CHAN_11	11

#define SLSI_P2P_IS_SOCAIL_CHAN(ch) ({	\
	typeof(ch) ch_ = (ch);	\
	((ch_ == SLSI_P2P_SOCIAL_CHAN_1) ||	\
	 (ch_ == SLSI_P2P_SOCIAL_CHAN_6) ||	\
	 (ch_ == SLSI_P2P_SOCIAL_CHAN_11));	\
})

/* A join scan with P2P GO SSID can come and hence the SSID length
 * comparision should include >=
 */
#define SLSI_IS_P2P_SSID(ssid, ssid_len) ((ssid_len >= SLSI_P2P_WILDCARD_SSID_LENGTH) && \
					  (memcmp(ssid, SLSI_P2P_WILDCARD_SSID, SLSI_P2P_WILDCARD_SSID_LENGTH) == 0))

/* Action frame categories for registering with firmware */
#define SLSI_ACTION_FRAME_PUBLIC        (BIT(4))
#define SLSI_ACTION_FRAME_VENDOR_SPEC_PROTECTED (BIT(30))
#define SLSI_ACTION_FRAME_VENDOR_SPEC   (BIT(31))
#define SLSI_ACTION_FRAME_WMM     (BIT(17))
#define SLSI_ACTION_FRAME_WNM     (BIT(10))
#define SLSI_ACTION_FRAME_QOS     (BIT(1))
#define SLSI_ACTION_FRAME_PROTECTED_DUAL    BIT(9)
#define SLSI_ACTION_FRAME_RADIO_MEASUREMENT    BIT(5)
#define SLSI_ACTION_FRAME_ROBUST_AV    BIT(19)

/* Firmware transmit rates */
#define SLSI_TX_RATE_NON_HT_1MBPS 0x4001
#define SLSI_TX_RATE_NON_HT_6MBPS 0x4004

#define SLSI_WLAN_EID_WAPI 68
#define SLSI_WLAN_EID_RSNX 244

#define SLSI_SCAN_PRIVATE_IE_CHANNEL_LIST_HEADER_LEN	7
#define SLSI_SCAN_PRIVATE_IE_SSID_FILTER_HEADER_LEN		7
#define SLSI_CHANN_INFO_HT_SCB  0x0100
#define SLSI_SCAN_CHANNEL_DESCRIPTOR_SIZE 4
#define SLSI_6GHZ_DESCRIPTOR_SIZE 15
/**
 * If availability_duration is set to SLSI_FW_CHANNEL_DURATION_UNSPECIFIED
 * then the firmware autonomously decides how long to remain listening on
 * the configured channel.
 */
#define SLSI_FW_CHANNEL_DURATION_UNSPECIFIED             (0x0000)
extern struct ieee80211_supported_band    slsi_band_2ghz;
extern struct ieee80211_supported_band    slsi_band_5ghz;
extern struct ieee80211_supported_band    slsi_band_6ghz;
extern struct ieee80211_sta_vht_cap       slsi_vht_cap;

/* Packet Filtering */
#define SLSI_MAX_PATTERN_DESC    4                                         /* We are not using more than 4 pattern descriptors in a pkt filter*/
#define SLSI_PKT_DESC_FIXED_LEN 2                                          /* offset (1) + mask length (1)*/
#define SLSI_PKT_FILTER_ELEM_FIXED_LEN  6                                  /* oui(3) + oui type(1) + filter id (1) + pkt filter mode(1)*/
#define SLSI_PKT_FILTER_ELEM_HDR_LEN  (2 + SLSI_PKT_FILTER_ELEM_FIXED_LEN) /* element id + len + SLSI_PKT_FILTER_ELEM_FIXED_LEN*/
#define SLSI_MAX_PATTERN_LENGTH 6

#define SLSI_INVALID_VIF 0xFFFF
#define SLSI_INVALID_IFNUM 0xFFFF

/*Default values of MIBS params for GET_STA_INFO driver private command */
#define SLSI_DEFAULT_UNIFI_PEER_RX_RETRY_PACKETS     0
#define SLSI_DEFAULT_UNIFI_PEER_RX_BC_MC_PACKETS     0
#define SLSI_DEFAULT_UNIFI_PEER_BANDWIDTH           -1
#define SLSI_DEFAULT_UNIFI_PEER_NSS                  0
#define SLSI_DEFAULT_UNIFI_PEER_RSSI                 1
#define SLSI_DEFAULT_UNIFI_PEER_TX_DATA_RATE         0
#define SLSI_DEFAULT_UNIFI_PEER_AVG_ANT_RSSI        -1
#define SLSI_DEFAULT_UNIFI_PEER_RX_FILURE_PACKETS    0

#define SLSI_CHECK_TYPE(sdev, recv_type, exp_type) \
	do { \
		int var1 = recv_type; \
		int var2 = exp_type; \
		if (var1 != var2) { \
			SLSI_WARN(sdev, "Type mismatch, expected:%d received:%d\n", var2, var1); \
		} \
	} while (0)

struct slsi_mlme_pattern_desc {
	u8 offset;
	u8 mask_length;
	u8 mask[SLSI_MAX_PATTERN_LENGTH];
	u8 pattern[SLSI_MAX_PATTERN_LENGTH];
};

struct slsi_mlme_pkt_filter_elem {
	u8                            header[SLSI_PKT_FILTER_ELEM_HDR_LEN];
	u8                            num_pattern_desc;
	struct slsi_mlme_pattern_desc pattern_desc[SLSI_MAX_PATTERN_DESC];
};

struct twt_setup {
	int setup_id;
	int negotiation_type;
	int flow_type;
	int trigger_type;
	u32 d_wake_duration;
	u32 d_wake_interval;
	u32 d_wake_time;
	int min_wake_interval;
	int max_wake_interval;
	int min_wake_duration;
	int max_wake_duration;
	int avg_pkt_num;
	int avg_pkt_size;
};

enum slsi_error_code {
	SLSI_WIFI_ERROR_NOT_SUPPORTED = -3,
	SLSI_WIFI_ERROR_NOT_AVAILABLE = -4,
	SLSI_WIFI_ERROR_INVALID_ARGUMENT = -5,
	SLSI_WIFI_ERROR_BUSY = -10,
	SLSI_WIFI_ERROR_TEMPORAL = -99
};

struct slsi_scheduled_pm_setup {
	int desired_duration;
	int desired_interval;
	int additional_duration;
	int minimum_interval;
	int maximum_interval;
	int minimum_duration;
	int maximum_duration;
};

struct slsi_scheduled_pm_status {
	u16 state;
	u16 duration;
	u32 interval;
};

struct slsi_sched_pm_leaky_ap {
	int detection_type;
	u8 src_ipaddr[4];
	u8 target_ipaddr[4];
	int duration;
	int grace_period;
};

#ifdef CONFIG_SCSC_WLAN_SAP_POWER_SAVE
struct ap_rps_params {
	int ips;
	int phase;
	u32 npktime;
	int checkstastatus;
};
#endif

#define SLSI_MLME_OUI_LENGTH		3
#define SLSI_MLME_SAMSUNG_OUI		0x001632
#define SLSI_MLME_TYPE_SCAN_PARAM	0x01
#define SLSI_MLME_PARAM_HEADER_SIZE	5
#define SLSI_MLME_TYPE_FILS_ASSOC_IES	0x0b
#define SLSI_MLME_TYPE_IP 0x03
#define SLSI_MLME_TYPE_MAC_ADDRESS	0x0b
#ifdef CONFIG_SCSC_WLAN_EHT
#define SLSI_MLME_TYPE_MLO 0x0f
#endif

#define SLSI_MLME_MAX_CHAN_COUNT ((0xFF - SLSI_SCAN_PRIVATE_IE_CHANNEL_LIST_HEADER_LEN) / \
				  SLSI_SCAN_CHANNEL_DESCRIPTOR_SIZE)

#define SLSI_MLME_MAX_6GHZ_DESC_COUNT ((0xFF - SLSI_SCAN_PRIVATE_IE_CHANNEL_LIST_HEADER_LEN) / \
				       SLSI_6GHZ_DESCRIPTOR_SIZE)

#define SLSI_MLME_SUBTYPE_SCAN_TIMING     0x1
#define SLSI_MLME_SUBTYPE_CHANNEL_LIST    0x2
#define SLSI_MLME_SUBTYPE_SSID_FILTER     0x4
#define SLSI_MLME_SUBTYPE_FILS_ASSOC_IES  0x01
#define SLSI_MLME_SUBTYPE_IPV4 0x01
#define SLSI_MLME_SUBTYPE_IPV6 0x02
#define SLSI_MLME_SUBTYPE_RESERVED 0x00

#define SLSI_MLME_SAMSUNG_OUI_MLO 0x0e
#define SLSI_MLME_SUBTYPE_MLO_LEGACY_LINK_LIST 0x00
#define SLSI_MLME_SUBTYPE_MLO_LINK_LIST 0x01
#ifdef CONFIG_SCSC_WLAN_EHT
#define SLSI_MLME_SUBTYPE_MLO_TTLM_ELEMENT 0x03
#endif

#define SLSI_KEY_MGMT_PSK 0x000fac02
#define SLSI_KEY_MGMT_PSK_SHA 0x000fac06
#define SLSI_KEY_MGMT_FILS_SHA256 0x000fac0e
#define SLSI_KEY_MGMT_FILS_SHA384 0x000fac0f
#define SLSI_KEY_MGMT_FT_FILS_SHA256 0x000fac10
#define SLSI_KEY_MGMT_FT_FILS_SHA384 0x000fac11
#define SLSI_KEY_MGMT_OWE 0x000fac12
#define SLSI_KEY_MGMT_802_1X_SUITE_B_192 0x000fac0c
#define SLSI_KEY_MGMT_FT_802_1X_SHA384 0x000fac0d

struct slsi_mlme_parameters {
	u8 element_id;
	u8 length;
	u8 oui[SLSI_MLME_OUI_LENGTH];
	u8 oui_type;
	u8 oui_subtype;
} __packed;

struct slsi_mlme_scan_timing_element {
	struct slsi_mlme_parameters header;
	u32 min_period;
	u32 max_period;
	u8 exponent;
	u8 step_count;
	u16 number_of_iterations;
	u16 skip_first_period;
} __packed;

#define SLSI_MLME_TYPE_REGULATORY_PARAM	0x0d
#define SLSI_MLME_SUBTYPE_REGULATORY_SUBBAND_DESC 0x00

struct slsi_mlme_reg_bulk_data {
	u16 start_freq;
	u16 end_freq;
	u8 max_bw;
	u8 max_power;
	u8 rule_flags;
} __packed;

struct slsi_mlme_reg_subband_desc_ie {
	struct slsi_mlme_parameters header;
	u16 start_freq;
	u16 end_freq;
	u8 max_bw;
	u8 max_power;
	u8 max_power_lpi;
	u8 max_power_vlp;
	u8 rule_flags;
} __packed;

enum sta_dump_params {
	SLSI_STA_DUMP_RSSI,
	SLSI_STA_DUMP_ANT1RSSI,
	SLSI_STA_DUMP_ANT2RSSI,
	SLSI_STA_DUMP_RX_COUNTERS,
	SLSI_STA_DUMP_TX_COUNTERS,
	SLSI_STA_DUMP_RXBYTES,
	SLSI_STA_DUMP_TXBYTES,
	SLSI_STA_DUMP_RXBITRATE,
	SLSI_STA_DUMP_TXBITRATE,
	SLSI_STA_DUMP_RXMCS,
	SLSI_STA_DUMP_TXMCS,
	SLSI_STA_DUMP_RXBANDWIDTH,
	SLSI_STA_DUMP_TXBANDWIDTH,
	SLSI_STA_DUMP_RXNSS,
	SLSI_STA_DUMP_TXNSS,
	SLSI_STA_DUMP_MODE,
	SLSI_STA_DUMP_MAX
};

static inline void slsi_mlme_put_oui_type(struct slsi_mlme_parameters *ie, u8 type, u8 subtype)
{
	ie->oui_type = type;
	ie->oui_subtype = subtype;
}

static inline void slsi_mlme_put_oui(u8 *oui, u32 val)
{
	oui[0] = (u8)((val >> 16) & 0xff);
	oui[1] = (u8)((val >> 8) & 0xff);
	oui[2] = (u8)(val & 0xff);
}

#ifdef CONFIG_SCSC_WLAN_EHT

#define SLSI_GET_MAX_EHT_TX_MCS(mcs) ((mcs & 0xF0) >> 4)
#define SLSI_GET_MAX_EHT_RX_MCS(mcs) (mcs & 0x0F)

static inline u8 slsi_get_nss_from_eht_mcs_supp_bw(struct ieee80211_eht_mcs_nss_supp_bw mcs)
{
	u8 tx_max_nss, rx_max_nss;
	u8 tx_max_nss_mcs13, rx_max_nss_mcs13;
	u8 tx_max_nss_mcs11, rx_max_nss_mcs11;
	u8 tx_max_nss_mcs9, rx_max_nss_mcs9;

	rx_max_nss_mcs13 = SLSI_GET_MAX_EHT_RX_MCS(mcs.rx_tx_mcs13_max_nss);
	tx_max_nss_mcs13 = SLSI_GET_MAX_EHT_TX_MCS(mcs.rx_tx_mcs13_max_nss);
	rx_max_nss_mcs11 = SLSI_GET_MAX_EHT_RX_MCS(mcs.rx_tx_mcs11_max_nss);
	tx_max_nss_mcs11 = SLSI_GET_MAX_EHT_TX_MCS(mcs.rx_tx_mcs11_max_nss);
	rx_max_nss_mcs9 = SLSI_GET_MAX_EHT_RX_MCS(mcs.rx_tx_mcs9_max_nss);
	tx_max_nss_mcs9 = SLSI_GET_MAX_EHT_TX_MCS(mcs.rx_tx_mcs9_max_nss);

	rx_max_nss = max(rx_max_nss_mcs13, (u8)max(rx_max_nss_mcs11, rx_max_nss_mcs9));
	tx_max_nss = max(tx_max_nss_mcs13, (u8)max(tx_max_nss_mcs11, tx_max_nss_mcs9));
	return max(rx_max_nss, tx_max_nss);
}

static inline u8
slsi_get_nss_from_eht_mcs_supp_bw_20mhz_only(struct ieee80211_eht_mcs_nss_supp_20mhz_only mcs)
{
	u8 tx_max_nss, rx_max_nss;
	u8 tx_max_nss_mcs13, rx_max_nss_mcs13;
	u8 tx_max_nss_mcs11, rx_max_nss_mcs11;
	u8 tx_max_nss_mcs9, rx_max_nss_mcs9;
	u8 tx_max_nss_mcs7, rx_max_nss_mcs7;

	rx_max_nss_mcs13 = SLSI_GET_MAX_EHT_RX_MCS(mcs.rx_tx_mcs13_max_nss);
	tx_max_nss_mcs13 = SLSI_GET_MAX_EHT_TX_MCS(mcs.rx_tx_mcs13_max_nss);
	rx_max_nss_mcs11 = SLSI_GET_MAX_EHT_RX_MCS(mcs.rx_tx_mcs11_max_nss);
	tx_max_nss_mcs11 = SLSI_GET_MAX_EHT_TX_MCS(mcs.rx_tx_mcs11_max_nss);
	rx_max_nss_mcs9 = SLSI_GET_MAX_EHT_RX_MCS(mcs.rx_tx_mcs9_max_nss);
	tx_max_nss_mcs9 = SLSI_GET_MAX_EHT_TX_MCS(mcs.rx_tx_mcs9_max_nss);
	rx_max_nss_mcs7 = SLSI_GET_MAX_EHT_RX_MCS(mcs.rx_tx_mcs9_max_nss);
	tx_max_nss_mcs7 = SLSI_GET_MAX_EHT_TX_MCS(mcs.rx_tx_mcs9_max_nss);

	rx_max_nss = max(max(rx_max_nss_mcs13, rx_max_nss_mcs11),
			 max(rx_max_nss_mcs9, rx_max_nss_mcs7));
	tx_max_nss = max(max(tx_max_nss_mcs13, tx_max_nss_mcs11),
			 max(tx_max_nss_mcs9, tx_max_nss_mcs7));
	return max(rx_max_nss, tx_max_nss);
}

static inline u32
slsi_get_antenna_from_eht_caps(struct slsi_dev *sdev, u8 supports_80mhz, u8 supports_160mhz,
			       u8 supports_320mhz,
			       struct ieee80211_eht_mcs_nss_supp *eht_mcs_nss_supp)
{
	u8 max_nss_320 = 0, max_nss_160 = 0, max_nss_80 = 0, max_nss_20only = 0;

	if (!eht_mcs_nss_supp)
		return 1;

	if (supports_320mhz)
		max_nss_320 = slsi_get_nss_from_eht_mcs_supp_bw(eht_mcs_nss_supp->bw._320);

	if (supports_160mhz)
		max_nss_160 = slsi_get_nss_from_eht_mcs_supp_bw(eht_mcs_nss_supp->bw._160);

	if (supports_80mhz)
		max_nss_80 = slsi_get_nss_from_eht_mcs_supp_bw(eht_mcs_nss_supp->bw._80);

	if (max_nss_320 == 0 && max_nss_160 == 0 && max_nss_80 == 0)
		max_nss_20only =
			slsi_get_nss_from_eht_mcs_supp_bw_20mhz_only(eht_mcs_nss_supp->only_20mhz);

	if (max_nss_20only)
		return max_nss_20only;

	return max(max_nss_320, (u8)max(max_nss_160, max_nss_80));
}
#endif

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
#define SLSI_MLME_SUBTYPE_6GHZ_PARAM	0x10

enum slsi_scan_6ghz_opt {
	SLSI_UNSOLICITED_PROBE = BIT(0),
	SLSI_SHORT_SSID_VALID = BIT(1),
	SLSI_PSC_NO_LISTEN = BIT(2)
};

struct slsi_mlme_scan_6ghz_descriptor {
	u32 short_ssid;
	u32 channel_idx;
	u8 bssid[ETH_ALEN];
	u8 scan_6ghz_option;
} __packed;

#endif

struct slsi_sta_dump_data {
	s16 param_field;
	int data[SLSI_STA_DUMP_MAX];
};

#ifdef CONFIG_SCSC_WLAN_EHT
enum slsi_mlo_error_code {
	SLSI_WIFI_MLO_ERROR_TEMPORAL = -1,
	SLSI_WIFI_MLO_ERROR_INVALID_ARGUMENT = -2,
	SLSI_WIFI_MLO_ERROR_TEMPORARILY_BUSY = -3,
	SLSI_WIFI_MLO_ERROR_NOT_SUPPORTED = -4,
	SLSI_WIFI_MLO_ERROR_NOT_AVAILABLE = -5
};

struct link_active_state_descriptor {
	u16 link_id;
	u16 link_state;
	u16 operating_frequency;
};

struct mlo_link_info {
	u16 control_mode;
	int num_links;
	struct link_active_state_descriptor links[MAX_NUM_MLD_LINKS];
};
#endif

#define SLSI_MLME_FAPI_MINOR_4_REG_RULE_v2 0xc

#define SLSI_MLME_FAPI_CHAN_WIDTH_320MHZ 0xb0

#ifdef CONFIG_SCSC_WLAN_EHT
/* Common Info Length field (Basic) + MLD MAC Address field (Basic) */
#define BASIC_ML_IE_COMMON_INFO_LINK_ID_IDX (1 + ETH_ALEN)
void slsi_scan_all_ml_link(struct net_device *dev, struct slsi_dev *sdev,
			   struct cfg80211_connect_params *sme, u16 capability);

struct ttlm_element {
	u16 link_id;
	u8  downlink_tid;
	u8  uplink_tid;
};
#endif

u16 slsi_get_chann_info(struct slsi_dev *sdev, struct cfg80211_chan_def *chandef);
int slsi_check_channelization(struct slsi_dev *sdev, struct cfg80211_chan_def *chandef,
			      int wifi_sharing_channel_switched);
int slsi_mlme_set_ip_address(struct slsi_dev *sdev, struct net_device *dev);
#if IS_ENABLED(CONFIG_IPV6)
int slsi_mlme_set_ipv6_address(struct slsi_dev *sdev, struct net_device *dev);
#endif
int slsi_mlme_set_with_vifnum(struct slsi_dev *sdev, struct net_device *dev, u8 *mib, int mib_len, u16 vifnum);
int slsi_mlme_set(struct slsi_dev *sdev, struct net_device *dev, u8 *req, int req_len);
struct sk_buff *slsi_mlme_set_with_cfm(struct slsi_dev *sdev, struct net_device *dev, u8 *mib, int mib_len, u16 vifnum);
int slsi_mlme_get_with_vifidx(struct slsi_dev *sdev, struct net_device *dev, u8 *mib, int mib_len,
			      u8 *resp, int resp_buf_len, int *resp_len, u16 vifidx);
int slsi_mlme_get(struct slsi_dev *sdev, struct net_device *dev, u8 *req, int req_len,
		  u8 *resp, int resp_buf_len, int *resp_len);
#ifdef CONFIG_SCSC_WLAN_EHT
int slsi_mlme_get_by_link(struct slsi_dev *sdev, struct net_device *dev, u8 link_id, u8 *req, int req_len,
			  u8 *resp, int resp_buf_len, int *resp_len);
#endif
int slsi_mlme_add_vif(struct slsi_dev *sdev, struct net_device *dev, const unsigned char *interface_address, const u8 *device_address);
int slsi_mlme_del_vif(struct slsi_dev *sdev, struct net_device *dev);
int slsi_mlme_add_detect_vif(struct slsi_dev *sdev, struct net_device *dev, const u8 *interface_address, const u8 *device_address);
int slsi_mlme_del_detect_vif(struct slsi_dev *sdev, struct net_device *dev);
#if defined(CONFIG_SLSI_WLAN_STA_FWD_BEACON) && (defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION >= 10)
int slsi_mlme_set_forward_beacon(struct slsi_dev *sdev, struct net_device *dev, int action);
#endif
int slsi_mlme_set_channel(struct slsi_dev *sdev, struct net_device *dev, struct ieee80211_channel *chan, u16 duration, u16 interval, u16 count);
int slsi_mlme_twt_setup(struct slsi_dev *sdev, struct net_device *dev, struct twt_setup *tsetup);
int slsi_mlme_twt_teardown(struct slsi_dev *sdev, struct net_device *dev, u16 setup_id, u16 negotiation_type);
int slsi_mlme_twt_status_query(struct slsi_dev *sdev, struct net_device *dev, char *command,
			       int command_len, int *command_pos, u16 setup_id);
int slsi_mlme_set_adps_state(struct slsi_dev *sdev, struct net_device *dev, int adps_state);

int slsi_mlme_sched_pm_setup(struct slsi_dev *sdev, struct net_device *dev,
			     struct slsi_scheduled_pm_setup *sched_pm_setup);
int slsi_mlme_sched_pm_teardown(struct slsi_dev *sdev, struct net_device *dev);
int slsi_mlme_sched_pm_status(struct slsi_dev *sdev, struct net_device *dev, struct slsi_scheduled_pm_status *status);
int slsi_mlme_set_delayed_wakeup(struct slsi_dev *sdev, struct net_device *dev, int delayed_wakeup_on, int timeout);
int slsi_mlme_set_delayed_wakeup_type(struct slsi_dev  *sdev, struct net_device *dev, int type,
				      u8 macaddrlist[][ETH_ALEN], int  mac_count, u8 ipv4addrlist[][4],
				      int ipv4_count, unsigned char ipv6addrlist[][16], int ipv6_count);

void slsi_ap_obss_scan_done_ind(struct net_device *dev, struct netdev_vif *ndev_vif);

int slsi_mlme_unset_channel_req(struct slsi_dev *sdev, struct net_device *dev);

#ifdef CONFIG_SCSC_WLAN_SAP_POWER_SAVE
int slsi_mlme_ap_suspend(struct slsi_dev *sdev, struct net_device *dev, u16 suspend_mode);
int slsi_mlme_set_ap_rps_params(struct slsi_dev *sdev, struct net_device *dev,
				struct ap_rps_params *rps_params, u16 ifnum);
int slsi_mlme_set_ap_rps(struct slsi_dev *sdev, struct net_device *dev, u16 rps_enable, u16 vifnum);
int slsi_mlme_set_ap_tx_power(struct slsi_dev *sdev, struct net_device *dev, u16 tx_power, u16 vifnum);
int slsi_mlme_get_ap_tx_power(struct slsi_dev *sdev, struct net_device *dev, char *command,
			      int buf_len, u16 vifnum);
#endif
int slsi_mlme_set_max_tx_power(struct slsi_dev *sdev, struct net_device *dev, int *pwr_limit);

int slsi_mlme_scheduled_pm_leaky_ap_detect(struct slsi_dev *sdev, struct net_device *dev, struct slsi_sched_pm_leaky_ap *leaky_ap);
int slsi_mlme_set_tx_num_ant(struct slsi_dev *sdev, struct net_device *dev, int cfg);

u16 slsi_compute_chann_info(struct slsi_dev *sdev, u16 width, u16 center_freq0, u16 channel_freq);
/**
 * slsi_mlme_add_autonomous_scan() Returns:
 *  0 : Scan installed
 * >0 : Scan NOT installed. Not an Error
 * <0 : Scan NOT installed. Error
 */

int slsi_mlme_add_scan_mld_addr(struct slsi_dev				*sdev,
				struct net_device			*dev,
				u16					scan_type,
				u16					report_mode,
				u32					n_ssids,
				struct cfg80211_ssid			*ssids,
				u32					n_channels,
				struct ieee80211_channel		*channels[],
				void					*gscan,
				const u8				*ies,
				u16					ies_len,
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
				u32					n_6ghz_params,
				struct cfg80211_scan_6ghz_params	*scan_6ghz_params,
				u32					scan_flags,
#endif
				bool					wait_for_ind,
				u8 					*macaddr);

int slsi_mlme_add_scan(struct slsi_dev                    *sdev,
		       struct net_device                  *dev,
		       u16								  scan_type,
		       u16                                report_mode,
		       u32                                n_ssids,
		       struct cfg80211_ssid               *ssids,
		       u32                                n_channels,
		       struct ieee80211_channel           *channels[],
		       void                               *gscan_param,
		       const u8                           *ies,
		       u16                                ies_len,
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
		       u32                                n_6ghz_params,
		       struct cfg80211_scan_6ghz_params   *scan_6ghz_params,
		       u32                                scan_flags,
#endif
		       bool                               wait_for_ind);

int slsi_mlme_add_sched_scan(struct slsi_dev                    *sdev,
			     struct net_device                  *dev,
			     struct cfg80211_sched_scan_request *request,
			     const u8                           *ies,
			     u16                                ies_len);

int slsi_mlme_del_scan(struct slsi_dev *sdev, struct net_device *dev, u16 scan_id, bool scan_timed_out);
int slsi_mlme_start(struct slsi_dev *sdev, struct net_device *dev, const unsigned char *bssid, struct cfg80211_ap_settings *settings, const u8 *wpa_ie_pos, const u8 *wmm_ie_pos, bool append_vht_ies);
int slsi_mlme_connect(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_connect_params *sme, struct ieee80211_channel *channel, const u8 *bssid);
#ifdef CONFIG_SCSC_WLAN_EHT
int slsi_check_ml_ie_from_bcn_ie(struct net_device *dev);
#endif
int slsi_mlme_set_key(struct slsi_dev *sdev, struct net_device *dev, u16 key_id, u16 key_type,
		      const u8 *address, struct key_params *key);
int slsi_mlme_get_key(struct slsi_dev *sdev, struct net_device *dev, u16 key_id, u16 key_type, u8 *seq, int *seq_len);

/**
 * Sends MLME-DISCONNECT-REQ and waits for the MLME-DISCONNECT-CFM
 * MLME-DISCONNECT-CFM only indicates if the firmware has accepted the request
 *  (or not) the actual end of the disconnection is indicated by the firmware
 * sending MLME-DISCONNECT-IND (following a successful MLME-DISCONNECT-CFM).
 * The host has to wait for the full exchange to complete with the firmware
 * before returning to cfg80211 if it made the disconnect request. So this
 * function waits for both the MLME-DISCONNECT-CFM and the MLME-DISCONNECT-IND
 * (if the MLME-DISCONNECT-CFM was successful)
 */
int slsi_mlme_disconnect(struct slsi_dev *sdev, struct net_device *dev, u8 *bssid, u16 reason_code, bool wait_ind
#ifdef CONFIG_SCSC_WLAN_EHT
			 , u16 *mlo_vif
#endif
			);
struct sk_buff *slsi_mlme_roaming_channel_list_req(struct slsi_dev *sdev, struct net_device *dev);
int slsi_mlme_req(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
struct sk_buff *slsi_mlme_req_no_cfm(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);

struct sk_buff *slsi_mlme_req_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb, u16 ind_id);
void slsi_mlme_get_sta_dump(struct netdev_vif *ndev_vif, struct net_device *dev,
			    struct slsi_sta_dump_data *dump_data);
/* Read multiple MIBs related to station info */
int slsi_mlme_get_sinfo_mib(struct slsi_dev *sdev, struct net_device *dev,
			    struct slsi_peer *peer, u16 vifidx);

int slsi_mlme_connect_scan(struct slsi_dev *sdev, struct net_device *dev,
			   u32 n_ssids, struct cfg80211_ssid *ssids, struct ieee80211_channel *channel);
int slsi_mlme_powermgt(struct slsi_dev *sdev, struct net_device *dev, u16 ps_mode);
int slsi_mlme_powermgt_unlocked(struct slsi_dev *sdev, struct net_device *dev, u16 ps_mode);
int slsi_mlme_register_action_frame(struct slsi_dev *sdev, struct net_device *dev,  u32 af_bitmap_active, u32 af_bitmap_suspended);
int slsi_mlme_synchronised_response(struct slsi_dev *sdev, struct net_device *dev,
				    struct cfg80211_external_auth_params *params);
int slsi_mlme_get_num_antennas(struct slsi_dev *sdev, struct net_device *dev, int *num_antennas);
int slsi_mlme_channel_switch(struct slsi_dev *sdev, struct net_device *dev,  u16 center_freq, u16 chan_info);
int slsi_mlme_add_info_elements(struct slsi_dev *sdev, struct net_device *dev,  u16 purpose, const u8 *ies, const u16 ies_len);
int slsi_mlme_update_connect_params(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_connect_params *sme, u32 changed);
int slsi_mlme_send_frame_mgmt(struct slsi_dev *sdev, struct net_device *dev, const u8 *frame, int frame_len, u16 data_desc, u16 msg_type, u16 host_tag, u16 freq, u32 dwell_time, u32 period);
int slsi_mlme_send_frame_data(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb, u16 msg_type,
			      u16 host_tag, u32 dwell_time, u32 period);
int slsi_mlme_wifisharing_permitted_channels(struct slsi_dev *sdev, struct net_device *dev, u8 *permitted_channels);
int slsi_mlme_reset_dwell_time(struct slsi_dev *sdev, struct net_device *dev);
int slsi_mlme_set_packet_filter(struct slsi_dev *sdev, struct net_device *dev, int pkt_filter_len, u8 num_filters, struct slsi_mlme_pkt_filter_elem *pkt_filter_elems);
void slsi_mlme_connect_resp(struct slsi_dev *sdev, struct net_device *dev);
void slsi_mlme_connected_resp(struct slsi_dev *sdev, struct net_device *dev, u16 peer_index);
void slsi_mlme_roamed_resp(struct slsi_dev *sdev, struct net_device *dev);
int slsi_mlme_roam(struct slsi_dev *sdev, struct net_device *dev, const u8 *bssid, u16 freq);
int slsi_mlme_wtc_mode_req(struct slsi_dev *sdev, struct net_device *dev, int wtc_mode, int scan_mode,
			   int rssi, int rssi_th_2g, int rssi_th_5g, int rssi_th_6g);
int slsi_mlme_set_cached_channels(struct slsi_dev *sdev, struct net_device *dev, u32 channels_count, u16 *channels);
int slsi_mlme_tdls_action(struct slsi_dev *sdev, struct net_device *dev, const u8 *peer, int action);
int slsi_mlme_set_tdls_state(struct slsi_dev *sdev, struct net_device *dev, u16 enabled_flag);
int slsi_mlme_set_acl(struct slsi_dev *sdev, struct net_device *dev, u16 ifnum, enum nl80211_acl_policy acl_policy, int max_acl_entries, struct mac_address mac_addrs[]);
int slsi_mlme_set_traffic_parameters(struct slsi_dev *sdev, struct net_device *dev, u16 user_priority, u16 medium_time, u16 minimun_data_rate, u8 *mac);
int slsi_mlme_del_traffic_parameters(struct slsi_dev *sdev, struct net_device *dev, u16 user_priority);
#ifdef CONFIG_SCSC_WLAN_GSCAN_ENABLE
int slsi_mlme_set_pno_list(struct slsi_dev *sdev, int count,
			   struct slsi_epno_param *epno_param, struct slsi_epno_hs2_param *epno_hs2_param);
int slsi_mlme_start_link_stats_req(struct slsi_dev *sdev, u16 mpdu_size_threshold, bool aggressive_statis_enabled);
int slsi_mlme_stop_link_stats_req(struct slsi_dev *sdev, u16 stats_stop_mask);
#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
int slsi_mlme_nan_enable(struct slsi_dev *sdev, struct net_device *dev, struct slsi_hal_nan_enable_req *hal_req);
int slsi_mlme_nan_publish(struct slsi_dev *sdev, struct net_device *dev, struct slsi_hal_nan_publish_req *hal_req,
			  u16 publish_id);
int slsi_mlme_nan_subscribe(struct slsi_dev *sdev, struct net_device *dev, struct slsi_hal_nan_subscribe_req *hal_req,
			    u16 subscribe_id);
int slsi_mlme_nan_tx_followup(struct slsi_dev *sdev, struct net_device *dev,
			      struct slsi_hal_nan_transmit_followup_req *hal_req);
int slsi_mlme_nan_set_config(struct slsi_dev *sdev, struct net_device *dev, struct slsi_hal_nan_config_req *hal_req);
int slsi_mlme_ndp_request(struct slsi_dev *sdev, struct net_device *dev,
			  struct slsi_hal_nan_data_path_initiator_req *hal_req, u32 ndp_instance_id, u16 ndl_vif_id);
int slsi_mlme_ndp_response(struct slsi_dev *sdev, struct net_device *dev,
			   struct slsi_hal_nan_data_path_indication_response *hal_req, u16 local_ndp_instance_id);
int slsi_mlme_ndp_terminate(struct slsi_dev *sdev, struct net_device *dev, u16 ndp_instance_id, u16 transaction_id);
int slsi_mlme_nan_range_req(struct slsi_dev *sdev, struct net_device *dev, u8 count, struct slsi_rtt_config *nl_rtt_params);
int slsi_mlme_nan_range_cancel_req(struct slsi_dev *sdev, struct net_device *dev);
int slsi_mlme_vendor_nan_set_command(struct slsi_dev *sdev, struct net_device *dev,
			   struct slsi_hal_nan_vendor_cmd_data *hal_req, struct slsi_hal_nan_vendor_cmd_resp_data **hal_cfm);
#endif
#endif

int slsi_mlme_set_ext_capab(struct slsi_dev *sdev, struct net_device *dev, u8 *data, int datalength);
int slsi_mlme_reassociate(struct slsi_dev *sdev, struct net_device *dev);
void slsi_mlme_reassociate_resp(struct slsi_dev *sdev, struct net_device *dev);
int slsi_modify_ies(struct net_device *dev, u8 eid, u8 *ies, int ies_len, u8 ie_index, u8 ie_value);
int slsi_mlme_set_rssi_monitor(struct slsi_dev *sdev, struct net_device *dev, u8 enable, s8 low_rssi_threshold, s8 high_rssi_threshold);
struct slsi_mib_value *slsi_read_mibs(struct slsi_dev *sdev, struct net_device *dev, struct slsi_mib_get_entry *mib_entries, int mib_count, struct slsi_mib_data *mibrsp);
#ifdef CONFIG_SCSC_WLAN_EHT
struct slsi_mib_value *slsi_read_mibs_with_vifidx(struct slsi_dev *sdev, struct net_device *dev,
						 struct slsi_mib_get_entry *mib_entries, int mib_count,
						 struct slsi_mib_data *mibrsp, u16 vifidx);
struct slsi_mib_value *slsi_read_mibs_by_link(struct slsi_dev *sdev, struct net_device *dev, u8 link_id,
					      struct slsi_mib_get_entry *mib_entries, int mib_count,
					      struct slsi_mib_data *mibrsp);
#endif
int slsi_mlme_set_host_state(struct slsi_dev *sdev, struct net_device *dev, u16 host_state);
int slsi_mlme_read_apf_request(struct slsi_dev *sdev, struct net_device *dev, u8 **host_dst, int *datalen);
int slsi_mlme_install_apf_request(struct slsi_dev *sdev, struct net_device *dev,
				  u8 *program, u32 program_len);
int slsi_mlme_set_multicast_ip(struct slsi_dev *sdev, struct net_device *dev,  __be32 multicast_ip_list[], int count);

#ifdef CONFIG_SCSC_WLAN_STA_ENHANCED_ARP_DETECT
int slsi_mlme_arp_detect_request(struct slsi_dev *sdev, struct net_device *dev, u16 action, u8 *ipaddr);
#endif
int slsi_mlme_start_detect_request(struct slsi_dev *sdev, struct net_device *dev);
int slsi_mlme_set_ctwindow(struct slsi_dev *sdev, struct net_device *dev, unsigned int ct_param);
int slsi_mlme_set_p2p_noa(struct slsi_dev *sdev, struct net_device *dev, unsigned int noa_count,
			  unsigned int interval, unsigned int duration);
void slsi_decode_fw_rate(u32 fw_rate, struct rate_info *rate, unsigned long *data_rate_mbps,
			 int *rate_nss, int *rate_bw, int * rate_mcs);
int slsi_test_sap_configure_monitor_mode(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_chan_def *chandef);
int slsi_mlme_delba_req(struct slsi_dev *sdev, struct net_device *dev, u16 vif, u8 *peer_qsta_address,
			u16 priority, u16 direction, u16 sequence_number, u16 reason_code);
int slsi_mlme_configure_monitor_mode(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_chan_def *chandef);
struct sk_buff *slsi_mlme_req_cfm(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb, u16 cfm_id);
struct sk_buff *slsi_mlme_req_cfm_ind(struct slsi_dev *sdev,
				      struct net_device *dev,
				      struct sk_buff *skb,
				      u16 cfm_id,
				      u16 ind_id,
				      bool (*validate_cfm_wait_ind)(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *cfm));
int slsi_mlme_set_country(struct slsi_dev *sdev, char *alpha2);
int slsi_mlme_set_roaming_parameters(struct slsi_dev *sdev, struct net_device *dev, u16 psid, int mib_value, int mib_length);
int slsi_mlme_set_band_req(struct slsi_dev *sdev, struct net_device *dev, uint band, u16 avoid_disconnection);
int slsi_mlme_set_scan_mode_req(struct slsi_dev *sdev, struct net_device *dev, u16 scan_mode, u16 max_channel_time,
				u16 home_away_time, u16 home_time, u16 max_channel_passive_time);
int slsi_mlme_add_range_req(struct slsi_dev *sdev, struct net_device *dev, u8 count, struct slsi_rtt_config *nl_rtt_params,
			    u16 rtt_id, u8 *source_addr);
int slsi_mlme_del_range_req(struct slsi_dev *sdev, struct net_device *dev, u16 count, u8 *addr, u16 rtt_id);
#if !defined(CONFIG_SCSC_WLAN_TX_API) && defined(CONFIG_SCSC_WLAN_ARP_FLOW_CONTROL)
void slsi_rx_send_frame_cfm_async(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
#endif
int slsi_mlme_set_num_antennas(struct net_device *dev, const u16 num_of_antennas, int frame_type);
#if defined(CONFIG_SCSC_WLAN_TAS)
#define SLSI_TAS_SET_CTRL_BACKOFF BIT(0)

void slsi_mlme_tas_init(struct slsi_dev *sdev);
void slsi_mlme_tas_deinit(struct slsi_dev *sdev);
void slsi_mlme_tas_deferred_tx_sar_limit(struct slsi_dev *sdev);
void slsi_mlme_tas_tx_sar_limit(struct slsi_dev *sdev, struct tas_sar_param *sar_param);
void slsi_mlme_tas_set_tx_sar_limit(struct slsi_dev *sdev, bool deferred_req, struct tas_sar_param *sar_param);
void slsi_mlme_tas_set_short_win_num(struct slsi_dev *sdev, struct tas_sar_param *sar_param);
#endif
int slsi_mlme_set_elnabypass_threshold(struct slsi_dev *sdev, struct net_device *elna_net_device, int elna_value);
#ifdef CONFIG_SCSC_WLAN_UWB_COEX
int slsi_mlme_set_uwbcx(struct slsi_dev *sdev, struct net_device *dev);
#endif
int slsi_mlme_get_elna_bypass_status(struct slsi_dev *sdev, struct net_device *elna_net_device);
int slsi_mlme_set_scan_timing_req(struct slsi_dev *sdev, struct net_device *dev, u16 scan_mode, u16 max_channel_time,
				  u16 home_away_time, u16 home_time, u16 max_channel_passive_time);
int slsi_mlme_set_low_latency_mode_req(struct slsi_dev *sdev, struct net_device *dev, u16 latency_mode,
				       u16 soft_roaming_scans_allowed);
int slsi_mlme_set_max_bw(struct slsi_dev *sdev, struct net_device *dev, u16 protection_scope,
			 u16 announce_validate, u16 bandwidth);
#ifdef CONFIG_SCSC_WLAN_EHT
int slsi_mlme_ml_link_state_query(struct slsi_dev *sdev, struct net_device *dev, char *command, int command_len);
int slsi_mlme_get_mlo_link_state(struct slsi_dev *sdev, struct net_device *dev, struct mlo_link_info *ml_info);
int slsi_mlo_link_vif_to_link_id_mapping(struct net_device *dev, u16 mlo_vif);
int slsi_mlme_ml_link_state_control(struct slsi_dev *sdev, struct net_device *dev,
				    u8 control_mode, u8 number_of_active_links, u16 active_links_bitmap);
int slsi_mlme_get_ml_tid_mapping_status(struct slsi_dev *sdev, struct net_device *dev,  char *command, int command_len);
int slsi_mlme_get_measure_ml_channel_condition(struct slsi_dev *sdev, struct net_device *dev);
int slsi_get_mlo_link_count(struct net_device *dev);
int slsi_mlme_ml_tid_mapping_request(struct slsi_dev *sdev, struct net_device *dev, u16 default_mapping,
				     struct ttlm_element *ttlm_list, int num_links);
#endif
#endif
