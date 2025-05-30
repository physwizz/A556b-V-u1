/******************************************************************************
 *
 * Copyright (c) 2012 - 2022 Samsung Electronics Co., Ltd. All rights reserved
 *
 *****************************************************************************/

#ifndef __SLSI_MGT_H__
#define __SLSI_MGT_H__

#include <linux/version.h>
#include <linux/mutex.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
#include <uapi/linux/if_arp.h>
#endif

#include "dev.h"
#include "reg_info.h"
#include "debug.h"

/* For 3.4.11 kernel support */
#ifndef WLAN_OUI_MICROSOFT
#define WLAN_OUI_MICROSOFT              0x0050f2
#define WLAN_OUI_TYPE_MICROSOFT_WPA     1
#define WLAN_OUI_TYPE_MICROSOFT_WMM     2
#define WLAN_OUI_TYPE_MICROSOFT_WPS     4
#endif

#define WLAN_OUI_SAMSUNG                        0x0000f0 /* Samsung OUI */
#define WLAN_OUI_TYPE_SAMSUNG_KEO               0x22

#define SLSI_COUNTRY_CODE_LEN 3

#define SLSI_EAPOL_TYPE_RSN_KEY          (2)
#define SLSI_EAPOL_TYPE_WPA_KEY          (254)

#define SLSI_IEEE8021X_TYPE_EAPOL_KEY    3
#define SLSI_IEEE8021X_TYPE_EAP_PACKET   0
#define SLSI_IEEE8021X_TYPE_EAP_START    1

#define VHT_CAP_INFO_CHAN_WIDTH_SET_MASK                0x0000000C
#define VHT_CAP_INFO_SUPPORTED_BW_160_80P80             BIT(2)

#define SLSI_EAPOL_KEY_INFO_KEY_TYPE_BIT_IN_LOWER_BYTE      BIT(3) /* Group = 0, Pairwise = 1 */
#define SLSI_EAPOL_KEY_INFO_ACK_BIT_IN_LOWER_BYTE           BIT(7)
#define SLSI_EAPOL_KEY_INFO_MIC_BIT_IN_HIGHER_BYTE          BIT(0)
#define SLSI_EAPOL_KEY_INFO_SECURE_BIT_IN_HIGHER_BYTE       BIT(1)
#define SLSI_EAPOL_KEY_INFO_REQUEST_BIT_IN_HIGHER_BYTE      BIT(3)
#define SLSI_EAPOL_KEY_INFO_ENCR_DATA_BIT_IN_HIGHER_BYTE    BIT(4)

/* pkt_data would start from 802.1X Authentication field (pkt_data[0] = Version).
 * For M4 packet, it will be something as below... member(size, position)
 * Version (1, 0) + Type (1, 1) + Length (2, 2:3) + Descriptor Type (1, 4) + Key Information (2, 5:6) +
 *  key_length(2, 7:8) + replay_counter(8, 9:16) + key_nonce(32, 17:48) + key_iv(16, 49:64) +
 *  key_rsc (8, 65:72) + key_id(16, 73:80) + key_mic (16, 81:96) + key_data_length(2, 97:98) +
 *  keydata(key_data_length, 99:99+key_data_length)
 */

#define SLSI_EAPOL_IEEE8021X_TYPE_POS                       (1)
#define SLSI_EAPOL_TYPE_POS                                 (4)
#define SLSI_EAPOL_KEY_INFO_HIGHER_BYTE_POS                 (5)
#define SLSI_EAPOL_KEY_INFO_LOWER_BYTE_POS                  (6)
#define SLSI_EAPOL_KEY_DATA_LENGTH_HIGHER_BYTE_POS          (97)
#define SLSI_EAPOL_KEY_DATA_LENGTH_LOWER_BYTE_POS           (98)
#define SLSI_EAPOL_KEY_RSC_LENGTH			    (8)

#define SLSI_EAP_CODE_POS                 (4)
#define SLSI_EAP_PACKET_REQUEST     (1)
#define SLSI_EAP_PACKET_RESPONSE   (2)
#define SLSI_EAP_PACKET_SUCCESS     (3)
#define SLSI_EAP_PACKET_FAILURE      (4)
#define SLSI_EAP_TYPE_POS                  (8)
#define SLSI_EAP_TYPE_EXPANDED       (254)
#define SLSI_EAP_OPCODE_POS                      (16)
#define SLSI_EAP_OPCODE_WSC_MSG          (4)
#define SLSI_EAP_OPCODE_WSC_START          (1)
#define SLSI_EAP_MSGTYPE_POS                    (27)
#define SLSI_EAP_MSGTYPE_M8                    (12)
#define SLSI_EAP_WPS_DWELL_TIME           (100000)       /*100 ms */
#define SLSI_EAP_TYPE_IDENTITY           (1)
#define SLSI_EAP_TYPE_NOTIFICATION  (2)
#define SLSI_EAP_TYPE_NAK (3)
#define SLSI_EAP_TYPE_MD5  (4)
#define SLSI_EAP_TYPE_OTP  (5)
#define SLSI_EAP_TYPE_GTC  (6)
#define SLSI_EAP_TYPE_TLS  (13)
#define SLSI_EAP_TYPE_LEAP  (17)
#define SLSI_EAP_TYPE_SIM  (18)
#define SLSI_EAP_TYPE_TTLS  (21)
#define SLSI_EAP_TYPE_AKA  (23)
#define SLSI_EAP_TYPE_PEAP  (25)
#define SLSI_EAP_TYPE_MSCHAPV2  (26)
#define SLSI_EAP_TYPE_TLV  (33)
#define SLSI_EAP_TYPE_TNC  (38)
#define SLSI_EAP_TYPE_FAST  (43)
#define SLSI_EAP_TYPE_PAX  (46)
#define SLSI_EAP_TYPE_PSK  (47)
#define SLSI_EAP_TYPE_SAKE  (48)
#define SLSI_EAP_TYPE_IKEV2  (49)
#define SLSI_EAP_TYPE_AKA_PRIME  (50)
#define SLSI_EAP_TYPE_GPSK  (51)
#define SLSI_EAP_TYPE_PWD  (52)
#define SLSI_EAP_TYPE_EKE  (53)

#define SLSI_80211_AC_VO 0
#define SLSI_80211_AC_VI 1
#define SLSI_80211_AC_BE 2
#define SLSI_80211_AC_BK 3

#define SLSI_AES_BLOCK_SIZE 16
#define SLSI_WPA_REPLAY_COUNTER_LEN 8
#define SLSI_WPA_NONCE_LEN 32
#define SLSI_WPA_KEY_RSC_LEN 8

struct slsi_wpa_eapol_key {
	u8 version;
	u8 type;
	u16 length;
	u8 key_desc_type;
	u8 key_info[2];
	u8 key_length[2];
	u8 replay_counter[SLSI_WPA_REPLAY_COUNTER_LEN];
	u8 key_nonce[SLSI_WPA_NONCE_LEN];
	u8 key_iv[16];
	u8 key_rsc[SLSI_WPA_KEY_RSC_LEN];
	u8 key_id[8];
} __packed;

/* IF Number (Index) based checks */
#ifdef CONFIG_SCSC_WLAN_DUAL_STATION
#define SLSI_IS_VIF_INDEX_WLAN(ndev_vif) (((ndev_vif)->ifnum == SLSI_NET_INDEX_WLAN) ||\
					  (((ndev_vif)->ifnum == SLSI_NET_INDEX_P2PX_SWLAN) && ((ndev_vif)->iftype == NL80211_IFTYPE_STATION)))
#else
#define SLSI_IS_VIF_INDEX_WLAN(ndev_vif) (ndev_vif->ifnum == SLSI_NET_INDEX_WLAN)
#endif
#define SLSI_IS_VIF_INDEX_P2P(ndev_vif) (ndev_vif->ifnum == SLSI_NET_INDEX_P2P)
#if defined(CONFIG_SCSC_WLAN_WIFI_SHARING) || defined(CONFIG_SCSC_WLAN_DUAL_STATION)
#define SLSI_IS_VIF_INDEX_P2P_GROUP(sdev, ndev_vif) ((ndev_vif->ifnum == SLSI_NET_INDEX_P2PX_SWLAN) &&\
						     (sdev->netdev_ap != sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN]))
#define SLSI_IS_VIF_INDEX_MHS_DUALSTA(sdev, ndev_vif) ((ndev_vif->ifnum == SLSI_NET_INDEX_P2PX_SWLAN) &&\
							(sdev->netdev_ap == sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN]))
#else
#define SLSI_IS_VIF_INDEX_P2P_GROUP(sdev, ndev_vif) (SLSI_UNUSED_PARAMETER(sdev), (ndev_vif)->ifnum == SLSI_NET_INDEX_P2PX_SWLAN)
#endif
#if defined(CONFIG_SCSC_WLAN_WIFI_SHARING)
#define SLSI_IS_VIF_INDEX_MHS(sdev, ndev_vif) ((ndev_vif->ifnum == SLSI_NET_INDEX_AP) &&\
					       (ndev_vif->iftype == NL80211_IFTYPE_AP) &&\
					       (sdev->netdev_ap == sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN]))
#endif
#define SLSI_IS_VIF_INDEX_NAN(ndev_vif) ((ndev_vif)->ifnum == SLSI_NET_INDEX_NAN)

/* Check for P2P unsync vif type */
#define SLSI_IS_P2P_UNSYNC_VIF(ndev_vif) ((ndev_vif->ifnum == SLSI_NET_INDEX_P2P) && (ndev_vif->vif_type == FAPI_VIFTYPE_DISCOVERY))

/* Check for HS unsync vif type */
#define SLSI_IS_HS2_UNSYNC_VIF(ndev_vif) ((ndev_vif->ifnum == SLSI_NET_INDEX_WLAN) && (ndev_vif->vif_type == FAPI_VIFTYPE_PRECONNECT))

/* Check for P2P Group role */
#define SLSI_IS_P2P_GROUP_STATE(sdev)  ((sdev->p2p_state == P2P_GROUP_FORMED_GO) || (sdev->p2p_state == P2P_GROUP_FORMED_CLI))

/* Extra delay to wait after MLME-Roam.Response before obtaining roam reports */
#define SLSI_STA_ROAM_REPORT_EXTRA_DELAY_MSEC 50

/* Extra duration in addition to ROC duration - For any workqueue scheduling delay */
#define SLSI_P2P_ROC_EXTRA_MSEC 10

/* Extra duration to retain unsync vif even after ROC/mgmt_tx completes */
#define SLSI_P2P_UNSYNC_VIF_EXTRA_MSEC  2000
/* Extra duration to retain HS2 unsync vif even after mgmt_tx completes */
#define SLSI_HS2_UNSYNC_VIF_EXTRA_MSEC  1000

/* Increased wait duration to retain unsync vif for GO-Negotiated to complete
 * due to delayed response or, to allow peer to retry GO-Negotiation
 */
#define SLSI_P2P_NEG_PROC_UNSYNC_VIF_RETAIN_DURATION 3000

/* Increased wait duration to send unset channel to Fw.
 * This would increase the listen time.
 */
#define SLSI_P2P_UNSET_CHANNEL_EXTRA_MSEC 600
#define SLSI_P2P_DELAY_UNSET_CHANNEL_AFTER_P2P_PROCEDURE 30
/* Extra duration in addition to mgmt tx wait */
#define SLSI_P2P_MGMT_TX_EXTRA_MSEC  100

#define SLSI_FORCE_SCHD_ACT_FRAME_MSEC 100
#define SLSI_P2PGO_KEEP_ALIVE_PERIOD_SEC 10
#define SLSI_P2PGC_CONN_TIMEOUT_MSEC 10000

#if !defined(CONFIG_SCSC_WLAN_TX_API) && defined(CONFIG_SCSC_WLAN_ARP_FLOW_CONTROL)
#define SLSI_UNPAUSE_ARP_Q_TIMEOUT_MSEC 10000
#endif

/* Maximum Offchannel Dwell Time in Firmware in STA Connected Mode. */
#define SLSI_FW_MAX_OFFCHANNEL_DWELL_TIME 100

/* P2P Public Action Frames */
#define SLSI_P2P_PA_GO_NEG_REQ  0
#define SLSI_P2P_PA_GO_NEG_RSP          1
#define SLSI_P2P_PA_GO_NEG_CFM  2
#define SLSI_P2P_PA_INV_REQ 3
#define SLSI_P2P_PA_INV_RSP 4
#define SLSI_P2P_PA_DEV_DISC_REQ        5
#define SLSI_P2P_PA_DEV_DISC_RSP        6
#define SLSI_P2P_PA_PROV_DISC_REQ       7
#define SLSI_P2P_PA_PROV_DISC_RSP       8

#define SLSI_PA_INVALID 0xFF

/* Service discovery public action frame types */
#define SLSI_PA_GAS_INITIAL_REQ  (10)
#define SLSI_PA_GAS_INITIAL_RSP  (11)
#define SLSI_PA_GAS_COMEBACK_REQ (12)
#define SLSI_PA_GAS_COMEBACK_RSP (13)

/* DPP action frames types */
#define SLSI_PA_DPP_AUTHENTICATION_REQ (0)
#define SLSI_PA_DPP_AUTHENTICATION_RESP (1)
#define SLSI_PA_DPP_AUTHENTICATION_CONF (2)

/*Radio Measurement action frames types */
#define SLSI_RM_RADIO_MEASUREMENT_REQ (0)
#define SLSI_RM_RADIO_MEASUREMENT_REP (1)
#define SLSI_RM_LINK_MEASUREMENT_REQ  (2)
#define SLSI_RM_LINK_MEASUREMENT_REP  (3)
#define SLSI_RM_NEIGH_REP_REQ         (4)
#define SLSI_RM_NEIGH_REP_RSP         (5)

#define SLSI_WNM_ACTION_FIELD_MIN (0)
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
#define SLSI_WNM_BSS_TM_REQ_PREF_CAND_LIST_INCLUDED BIT(0)
#define SLSI_WNM_BSS_TM_REQ_BSS_TERMINATION_INCLUDED BIT(3)
#define SLSI_WNM_BSS_TM_REQ_ESS_DISASSOC_IMMINENT BIT(4)
#endif
#define SLSI_WNM_ACTION_FIELD_MAX (27)

/* For service discovery action frames dummy subtype is used by setting the 7th bit */
#define SLSI_PA_GAS_DUMMY_SUBTYPE_MASK   0x80
#define SLSI_PA_GAS_INITIAL_REQ_SUBTYPE  (SLSI_PA_GAS_INITIAL_REQ | SLSI_PA_GAS_DUMMY_SUBTYPE_MASK)
#define SLSI_PA_GAS_INITIAL_RSP_SUBTYPE  (SLSI_PA_GAS_INITIAL_RSP | SLSI_PA_GAS_DUMMY_SUBTYPE_MASK)
#define SLSI_PA_GAS_COMEBACK_REQ_SUBTYPE (SLSI_PA_GAS_COMEBACK_REQ | SLSI_PA_GAS_DUMMY_SUBTYPE_MASK)
#define SLSI_PA_GAS_COMEBACK_RSP_SUBTYPE (SLSI_PA_GAS_COMEBACK_RSP | SLSI_PA_GAS_DUMMY_SUBTYPE_MASK)

/* For DPP frame dummy subtype is used by setting the 7th and 6th bit */
#define SLSI_PA_DPP_DUMMY_SUBTYPE_MASK   0xC0
#define SLSI_PA_DPP_AUTHENTICATION_REQ_SUBTYPE (SLSI_PA_DPP_AUTHENTICATION_REQ | SLSI_PA_DPP_DUMMY_SUBTYPE_MASK)
#define SLSI_PA_DPP_AUTHENTICATION_RESP_SUBTYPE (SLSI_PA_DPP_AUTHENTICATION_RESP | SLSI_PA_DPP_DUMMY_SUBTYPE_MASK)
#define SLSI_PA_DPP_AUTHENTICATION_CONF_SUBTYPE (SLSI_PA_DPP_AUTHENTICATION_CONF | SLSI_PA_DPP_DUMMY_SUBTYPE_MASK)

#define SLSI_P2P_STATUS_ATTR_ID 0
#define SLSI_P2P_STATUS_CODE_SUCCESS 0

#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
#define SLSI_MBO_ASSOC_DISALLOWED_ATTR_ID 0x04
#define SLSI_MBO_ASSOC_RETRY_DELAY_ATTR_ID 0x08
#endif

#define SLSI_ROAMING_CHANNEL_CACHE_TIMEOUT (5 * 60)

#define SET_ETHERTYPE_PATTERN_DESC(pd, ethertype) \
	pd.offset  = 0x0C; \
	pd.mask_length = 2; \
	pd.mask[0] = 0xff; \
	pd.mask[1] = 0xff; \
	pd.pattern[0] = ethertype >> 8; \
	pd.pattern[1] = ethertype & 0xFF

/* For checking DHCP frame */
#define SLSI_IP_TYPE_UDP 0x11
#define SLSI_IP_TYPE_OFFSET 23
#define SLSI_IP_SOURCE_PORT_OFFSET 34
#define SLSI_IP_DEST_PORT_OFFSET 36
#define SLSI_DHCP_SERVER_PORT 67
#define SLSI_DHCP_CLIENT_PORT 68
#define SLSI_DNS_DEST_PORT 53
#define SLSI_MDNS_DEST_PORT 5353

#define SLSI_DHCP_MSG_MAGIC_OFFSET 278
#define SLSI_DHCP_OPTION 53
#define SLSI_DHCP_MESSAGE_TYPE_DISCOVER   0x01
#define SLSI_DHCP_MESSAGE_TYPE_OFFER      0x02
#define SLSI_DHCP_MESSAGE_TYPE_REQUEST    0x03
#define SLSI_DHCP_MESSAGE_TYPE_DECLINE    0x04
#define SLSI_DHCP_MESSAGE_TYPE_ACK        0x05
#define SLSI_DHCP_MESSAGE_TYPE_NAK        0x06
#define SLSI_DHCP_MESSAGE_TYPE_RELEASE    0x07
#define SLSI_DHCP_MESSAGE_TYPE_INFORM     0x08
#define SLSI_DHCP_MESSAGE_TYPE_FORCERENEW 0x09
#define SLSI_DHCP_MESSAGE_TYPE_INVALID    0x0A

#define SLSI_ARP_SRC_IP_ADDR_OFFSET   14
#define SLSI_ARP_DEST_IP_ADDR_OFFSET  24
#define SLSI_IS_GRATUITOUS_ARP(frame) (!memcmp(&frame[SLSI_ARP_SRC_IP_ADDR_OFFSET],\
					       &frame[SLSI_ARP_DEST_IP_ADDR_OFFSET], 4))
#define SLSI_ARP_REPLY_OPCODE  2
#define SLSI_ARP_REQUEST_OPCODE  1
#define SLSI_ARP_OPCODE_OFFSET  6

#define SLSI_ICMP6_PACKET           58
#define SLSI_ICMP6_PACKET_TYPE_RA   134
#define SLSI_ICMP6_PACKET_TYPE_NS   135
#define SLSI_ICMP6_PACKET_TYPE_NA   136

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(4, 4, 0))
	#define WLAN_CATEGORY_WNM 10
#endif

#ifdef SCSC_SEP_VERSION
#define SLSI_CONNECT_NO_NETWORK_FOUND	0x0401
#define SLSI_CONNECT_AUTH_NO_ACK	0x0402
#define SLSI_CONNECT_AUTH_NO_RESP	0x0403
#define SLSI_CONNECT_AUTH_TX_FAIL	0x0404
#define SLSI_CONNECT_AUTH_SAE_NO_ACK	0x0405
#define SLSI_CONNECT_AUTH_SAE_NO_RESP	0x0406
#define SLSI_CONNECT_AUTH_SAE_TX_FAIL	0x0407
#define SLSI_CONNECT_ASSOC_NO_ACK	0x0408
#define SLSI_CONNECT_ASSOC_NO_RESP	0x0409
#define SLSI_CONNECT_ASSOC_TX_FAIL	0x040a
#endif

#define SLSI_RECOVERY_SERVICE_STARTED   0
#define SLSI_RECOVERY_SERVICE_STOPPED    1
#define SLSI_RECOVERY_SERVICE_CLOSED   2
#define SLSI_RECOVERY_SERVICE_OPENED    3
#define SLSI_DEFAULT_HW_MAC_ADDR    "\x00\x00\x0F\x11\x22\x33"

#define SLSI_MGMT_FRAME_SUBTYPE_ASSOC_REQ    0
#define SLSI_MGMT_FRAME_SUBTYPE_ASSOC_RESP   1
#define SLSI_MGMT_FRAME_SUBTYPE_REASSOC_REQ  2
#define SLSI_MGMT_FRAME_SUBTYPE_REASSOC_RESP 3

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
#define SLSI_WIFISHARING_PERMITTED_CHANNELS_SIZE 13
#else
#define SLSI_WIFISHARING_PERMITTED_CHANNELS_SIZE 8
#endif

#define TWT_SETUP_EVENT_SUCCESS               0
#define TWT_SETUP_EVENT_REJECTED              1
#define TWT_SETUP_EVENT_TIMEOUT               2
#define TWT_SETUP_EVENT_INVALID_IE            3
#define TWT_SETUP_EVENT_PARAMS_VALUE_REJECTED 4
#define TWT_SETUP_EVENT_AP_NO_TWT_INFO        5

#define TWT_TEARDOWN_HOST_INITIATED                      0
#define TWT_TEARDOWN_PEER_INITIATED                      1
#define TWT_TEARDOWN_CONCURRENT_OPERATION_SAME_BAND      2
#define TWT_TEARDOWN_CONCURRENT_OPERATION_DIFFERENT_BAND 3
#define TWT_TEARDOWN_ROAMING_OR_ECSA                     4
#define TWT_TEARDOWN_BT_COEX                             5
#define TWT_TEARDOWN_TIMEOUT                             6
#define TWT_TEARDOWN_PS_DISABLE                          7

#define TWT_RESULTCODE_UNKNOWN 255

#ifdef CONFIG_SCSC_WLAN_EHT
#define SLSI_CONFIG_MLO_MODE_NONE 0
#define SLSI_CONFIG_MLO_MODE_MLSR 1
#define SLSI_CONFIG_MLO_MODE_EMLSR 2
#define SLSI_CONFIG_MLO_MODE_NSTR 3
#define SLSI_CONFIG_MLO_MODE_STR 4
#define SLSI_CONFIG_MLO_MODE_ANY 5
#endif

#define SLSI_MODE_EHT         BIT(4)
#define SLSI_MODE_HE          BIT(2)
#define SLSI_MODE_VHT         BIT(1)
#define SLSI_MODE_LEGACY_HT   BIT(0)
#define SLSI_2_4_BAND_SUPPORT BIT(0)
#define SLSI_5_BAND_SUPPORT   BIT(1)
#define SLSI_6_BAND_SUPPORT   BIT(2)

#define SLSI_GETCAP_TWT_REQUESTER_SUPPORT BIT(0)
#define SLSI_GETCAP_TWT_RESPONDER_SUPPORT BIT(1)
#define SLSI_GETCAP_BROADCAST_TWT_SUPPORT BIT(2)
#define SLSI_GETCAP_FLEXIBLE_TWT_SUPPORT  BIT(3)
#define SLSI_GETCAP_TWT_REQUIRED          BIT(4)

#define SLSI_TWT_FW_BROADCAST_SUPPORT BIT(0)
#define SLSI_TWT_FW_RESPONDER_SUPPORT BIT(1)
#define SLSI_TWT_FW_REQUESTER_SUPPORT BIT(2)
#define SLSI_TWT_FW_FLEXIBLE_SUPPORT  BIT(3)

#define SLSI_TWT_FW_RESPONDER_SUPPORT_BIT_POS 1

#ifdef CONFIG_SCSC_WLAN_EHT
#define SLSI_MIN_BASIC_ML_IE_COMMON_INFO_LEN \
         (2 + /* Multi-Link Control field */ \
          1 + /* Common Info Length field (Basic) */ \
          ETH_ALEN) /* MLD MAC Address field (Basic) */
#endif

enum slsi_dhcp_tx {
	SLSI_TX_IS_NOT_DHCP,
	SLSI_TX_IS_DHCP_SERVER,
	SLSI_TX_IS_DHCP_CLIENT
};

enum slsi_fw_regulatory_rule_flags {
	SLSI_REGULATORY_NO_IR = BIT(0),
	SLSI_REGULATORY_DFS = BIT(1),
	SLSI_REGULATORY_NO_OFDM = BIT(2),
	SLSI_REGULATORY_NO_INDOOR = BIT(3),
	SLSI_REGULATORY_NO_OUTDOOR = BIT(4),
	SLSI_REGULATORY_AUTO_BW = BIT(5),
	SLSI_REGULATORY_6GHZ_VLP = BIT(29),
	SLSI_REGULATORY_6GHZ_LPI = BIT(30),
	SLSI_REGULATORY_6GHZ_SP = BIT(31)
};

enum slsi_sta_conn_state {
	SLSI_STA_CONN_STATE_DISCONNECTED = 0,
	SLSI_STA_CONN_STATE_CONNECTING = 1,
	SLSI_STA_CONN_STATE_DOING_KEY_CONFIG = 2,
	SLSI_STA_CONN_STATE_CONNECTED = 3
};

enum slsi_wlan_vendor_attr_rcl_channel_list {
	SLSI_WLAN_VENDOR_ATTR_SSID = 1,
	SLSI_WLAN_VENDOR_ATTR_RCL_CHANNEL_COUNT,
	SLSI_WLAN_VENDOR_ATTR_RCL_CHANNEL_LIST,
	SLSI_WLAN_VENDOR_ATTR_RCL_CHANNEL_LIST_EVENT_MAX
};

enum slsi_vendor_attr_twt_setup {
	SLSI_VENDOR_ATTR_SETUP_ID = 1,
	SLSI_VENDOR_ATTR_RESULT_CODE,
	SLSI_VENDOR_ATTR_NEGOTIATION_TYPE,
	SLSI_VENDOR_ATTR_FLOW_TYPE,
	SLSI_VENDOR_ATTR_TRIGGERED_TYPE,
	SLSI_VENDOR_ATTR_WAKE_TIME,
	SLSI_VENDOR_ATTR_WAKE_DURATION,
	SLSI_VENDOR_ATTR_WAKE_INTERVAL
};

enum slsi_vendor_attr_twt_teardown {
	SLSI_VENDOR_ATTR_TEARDOWN_SETUP_ID = 1,
	SLSI_VENDOR_ATTR_TEARDOWN_RESULT_CODE
};

enum slsi_vendor_attr_twt_notify {
	SLSI_VENDOR_ATTR_NOTIFY_NOTIFICATION = 1
};

enum slsi_vendor_attr_sched_pm_teardown {
	SLSI_VENDOR_ATTR_SCHED_PM_TEARDOWN_RESULT_CODE
};

#ifdef CONFIG_SCSC_WLAN_EHT
enum slsi_vendor_attr_mlo_tid_to_link_mapping_event {
	SLSI_VENDOR_ATTR_MLO_TTLM_DEFAULT_MAPPING = 1,
	SLSI_VENDOR_ATTR_MLO_TTLM_NUM_LINKS,
	SLSI_VENDOR_ATTR_MLO_TTLM_LINKS,
	SLSI_VENDOR_ATTR_MLO_TTLM_LINK_ID,
	SLSI_VENDOR_ATTR_MLO_TTLM_DOWNLINK_TID,
	SLSI_VENDOR_ATTR_MLO_TTLM_UPLINK_TID
};
#endif

struct slsi_twt_setup_event {
	u16 setup_id;
	u8  reason_code;
	u16 negotiation_type;
	u16 flow_type;
	u16 triggered_type;
	u64 wake_time;
	u32 wake_duration;
	u32 wake_interval;
};

#ifdef CONFIG_SCSC_WLAN_EHT
enum slsi_vendor_attr_mlo_channel_measure_event {
	SLSI_VENDOR_ATTR_MLO_CHANNEL_MEASURE_LINK_COUNT = 1,
	SLSI_VENDOR_ATTR_MLO_CHANNEL_MEASURE_LINKS,
	SLSI_VENDOR_ATTR_MLO_CHANNEL_MEASURE_LINK_ID,
	SLSI_VENDOR_ATTR_MLO_CHANNEL_MEASURE_LINK_RSSI,
	SLSI_VENDOR_ATTR_MLO_CHANNEL_MEASURE_LINK_SUBCHANNEL_COUNT,
	SLSI_VENDOR_ATTR_MLO_CHANNEL_MEASURE_LINK_CCA_BUSY_TIME_RATIO,
	SLSI_VENDOR_ATTR_MLO_CHANNEL_MEASURE_LINK_MAX
};

struct mlo_link_measurement {
	u16 link_id;
	int rssi;
	u8  subchannel_count;
	u8  cca_busy_time_ratio[MAX_NUM_MLO_SUBCHANNEL];
};
#endif

#define SLSI_NAN_MGMT_VIF_NUM(sdev) (FAPI_VIFRANGE_VIF_INDEX_MAX - sdev->nan_max_ndp_instances)
#define SLSI_NAN_DATA_VIF_NUM_START(sdev) (FAPI_VIFRANGE_VIF_INDEX_MAX - sdev->nan_max_ndp_instances + 1)

static inline unsigned compare_ether_addr(const u8 *addr1, const u8 *addr2)
{
	return !ether_addr_equal(addr1, addr2);
}

/**
 * Peer record handling:
 * Records are created/destroyed by the control path eg cfg80211 connect or
 * when handling a MLME-CONNECT-IND when the VIA is an AP.
 *
 * However peer records are also currently accessed from the data path in both
 * Tx and Rx directions:
 * Tx - to determine the queueset
 * Rx - for routing received packets back out to peers
 *
 * So the interactions required for the data path:
 * 1. can NOT block
 * 2. needs to be as quick as possible
 */
static inline struct slsi_peer *slsi_get_peer_from_mac(struct slsi_dev *sdev, struct net_device *dev, const u8 *mac)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct net_device *ap_dev = NULL;

	(void)sdev; /* unused */

	if (ndev_vif->iftype == NL80211_IFTYPE_AP_VLAN) {
		rcu_read_lock();
		ap_dev = ndev_vif->netdev_ap;
		rcu_read_unlock();
		ndev_vif = netdev_priv(ap_dev);
	}
	/* Accesses the peer records but doesn't block as called from the data path.
	 * MUST check the valid flag on the record before accessing any other data in the record.
	 * Records are static, so having obtained a pointer the pointer will remain valid
	 * it just maybe the data that it points to gets set to ZERO.
	 */

	if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION) {
		if (ndev_vif->sta.tdls_enabled) {
			int i;

			for (i = 1; i < SLSI_TDLS_PEER_INDEX_MAX; i++)
				if (ndev_vif->peer_sta_record[i] && ndev_vif->peer_sta_record[i]->valid &&
				    compare_ether_addr(ndev_vif->peer_sta_record[i]->address, mac) == 0)
					return ndev_vif->peer_sta_record[i];
		}
		if (ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET] && ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET]->valid)
			return ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET];
	} else if (ndev_vif->vif_type == FAPI_VIFTYPE_AP) {
		int i = 0;

		for (i = 0; i < SLSI_PEER_INDEX_MAX; i++)
			if (ndev_vif->peer_sta_record[i] && ndev_vif->peer_sta_record[i]->valid &&
			    compare_ether_addr(ndev_vif->peer_sta_record[i]->address, mac) == 0)
				return ndev_vif->peer_sta_record[i];
#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
	} else if (ndev_vif->ifnum >= SLSI_NAN_DATA_IFINDEX_START) {
		int i = 0;

		for (i = 0; i < SLSI_PEER_INDEX_MAX; i++) {
			if (ndev_vif->peer_sta_record[i] && ndev_vif->peer_sta_record[i]->valid &&
			    compare_ether_addr(ndev_vif->peer_sta_record[i]->address, mac) == 0)
				return ndev_vif->peer_sta_record[i];
		}

		if (is_multicast_ether_addr(mac)) {
			for (i = 0; i < SLSI_PEER_INDEX_MAX; i++)
			if (ndev_vif->peer_sta_record[i] && ndev_vif->peer_sta_record[i]->valid) {
				SLSI_NET_DBG1(dev, SLSI_TX,
					      "multicast/broadcast packet on NAN netif (dest:" MACSTR ", change to peer: " MACSTR ")\n",
					      MAC2STR(mac), MAC2STR(ndev_vif->peer_sta_record[i]->address));
				return ndev_vif->peer_sta_record[i];
			}
		}
	}
#else
	}
#endif
	return NULL;
}

static inline struct slsi_peer *slsi_get_peer_from_qs(struct slsi_dev *sdev, struct net_device *dev, u16 queueset)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	(void)sdev; /* unused */

	if (!ndev_vif->peer_sta_record[queueset] || !ndev_vif->peer_sta_record[queueset]->valid)
		return NULL;

	return ndev_vif->peer_sta_record[queueset];
}

static inline bool slsi_is_tdls_peer(struct net_device *dev, struct slsi_peer *peer)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	return (ndev_vif->vif_type == FAPI_VIFTYPE_STATION) && (peer->aid >= SLSI_TDLS_PEER_INDEX_MIN);
}

static inline bool slsi_is_proxy_arp_supported_on_ap(struct sk_buff *assoc_resp_ie)
{
	const u8 *ie = cfg80211_find_ie(WLAN_EID_EXT_CAPABILITY, assoc_resp_ie->data, assoc_resp_ie->len);

	if ((ie) && (ie[1] > 1))
		return ie[3] & 0x10;     /*0: eid, 1: len; 3: proxy arp is 12th bit*/

	return 0;
}

static inline int slsi_cache_ies(const u8 *src_ie, size_t src_ie_len, u8 **dest_ie, size_t *dest_ie_len)
{
	*dest_ie = kmalloc(src_ie_len, GFP_KERNEL);
	if (*dest_ie == NULL)
		return -ENOMEM;

	memcpy(*dest_ie, src_ie, src_ie_len);
	*dest_ie_len = src_ie_len;

	return 0;
}

static inline void slsi_clear_cached_ies(u8 **ie, size_t *ie_len)
{
	if (*ie_len != 0)
		kfree(*ie);
	*ie = NULL;
	*ie_len = 0;
}

/* Public Action frame subtype in text format for debug purposes */
static inline char *slsi_pa_subtype_text(int subtype)
{
	switch (subtype) {
	case SLSI_P2P_PA_GO_NEG_REQ:
		return "GO_NEG_REQ";
	case SLSI_P2P_PA_GO_NEG_RSP:
		return "GO_NEG_RSP";
	case SLSI_P2P_PA_GO_NEG_CFM:
		return "GO_NEG_CFM";
	case SLSI_P2P_PA_INV_REQ:
		return "INV_REQ";
	case SLSI_P2P_PA_INV_RSP:
		return "INV_RSP";
	case SLSI_P2P_PA_DEV_DISC_REQ:
		return "DEV_DISC_REQ";
	case SLSI_P2P_PA_DEV_DISC_RSP:
		return "DEV_DISC_RSP";
	case SLSI_P2P_PA_PROV_DISC_REQ:
		return "PROV_DISC_REQ";
	case SLSI_P2P_PA_PROV_DISC_RSP:
		return "PROV_DISC_RSP";
	case SLSI_PA_GAS_INITIAL_REQ_SUBTYPE:
		return "GAS_INITIAL_REQUEST";
	case SLSI_PA_GAS_INITIAL_RSP_SUBTYPE:
		return "GAS_INITIAL_RESPONSE";
	case SLSI_PA_GAS_COMEBACK_REQ_SUBTYPE:
		return "GAS_COMEBACK_REQUEST";
	case SLSI_PA_GAS_COMEBACK_RSP_SUBTYPE:
		return "GAS_COMEBACK_RESPONSE";
	case SLSI_PA_INVALID:
		return "PA_INVALID";
	default:
		return "UNKNOWN";
	}
}

/* Cookie generation and assignment for user space ROC and mgmt_tx request from supplicant */
static inline void slsi_assign_cookie_id(u64 *cookie, u64 *counter)
{
	(*cookie) = ++(*counter);
	if ((*cookie) == 0)
		(*cookie) = ++(*counter);
}

/* Update P2P Probe Response IEs in driver */
static inline void slsi_unsync_vif_set_probe_rsp_ie(struct netdev_vif *ndev_vif, u8 *ies, size_t ies_len)
{
	if (ndev_vif->unsync.probe_rsp_ies_len)
		kfree(ndev_vif->unsync.probe_rsp_ies);
	ndev_vif->unsync.probe_rsp_ies = ies;
	ndev_vif->unsync.probe_rsp_ies_len = ies_len;
}

/* Set management frame tx data of vif */
static inline int slsi_set_mgmt_tx_data(struct netdev_vif *ndev_vif, u64 cookie, u16 host_tag, const u8 *buf, size_t buf_len)
{
	u8 *tx_frame = NULL;

	if (buf_len != 0) {
		tx_frame = kmalloc(buf_len, GFP_KERNEL);
		if (!tx_frame) {
			SLSI_NET_ERR(ndev_vif->wdev.netdev, "FAILED to allocate memory for Tx frame\n");
			return -ENOMEM;
		}
		SLSI_NET_DBG3(ndev_vif->wdev.netdev, SLSI_CFG80211, "Copy buffer for tx_status\n");
		memcpy(tx_frame, buf, buf_len);
	} else if (ndev_vif->mgmt_tx_data.buf) {
		SLSI_NET_DBG3(ndev_vif->wdev.netdev, SLSI_CFG80211, "Free buffer of tx_status\n");
		kfree(ndev_vif->mgmt_tx_data.buf);
	}

	ndev_vif->mgmt_tx_data.cookie = cookie;
	ndev_vif->mgmt_tx_data.host_tag = host_tag;
	ndev_vif->mgmt_tx_data.buf = tx_frame;
	ndev_vif->mgmt_tx_data.buf_len = buf_len;

	return 0;
}

/**
 * Handler to queue P2P unsync vif deletion work.
 */
static inline void slsi_p2p_queue_unsync_vif_del_work(struct netdev_vif *ndev_vif, unsigned int delay)
{
	cancel_delayed_work(&ndev_vif->unsync.del_vif_work);
	queue_delayed_work(ndev_vif->sdev->device_wq, &ndev_vif->unsync.del_vif_work, msecs_to_jiffies(delay));
}

/* Update the new state for P2P. Also log the state change for debug purpose */
#define SLSI_P2P_STATE_CHANGE(sdev, next_state) \
	do { \
		SLSI_DBG1(sdev, SLSI_CFG80211, "P2P state change: %s -> %s\n", slsi_p2p_state_text(sdev->p2p_state), slsi_p2p_state_text(next_state)); \
		sdev->p2p_state = next_state; \
	} while (0)

void slsi_purge_scan_results(struct netdev_vif *ndev_vif, u16 scan_id);
void slsi_purge_scan_results_locked(struct netdev_vif *ndev_vif, u16 scan_id);
struct sk_buff *slsi_dequeue_cached_scan_result(struct slsi_scan *scan, int *count);
void slsi_get_hw_mac_address(struct slsi_dev *sdev, u8 *addr);
int slsi_start(struct slsi_dev *sdev, struct net_device *dev);
int slsi_start_monitor_mode(struct slsi_dev *sdev, struct net_device *dev);
void slsi_stop_net_dev(struct slsi_dev *sdev, struct net_device *dev);
void slsi_stop(struct slsi_dev *sdev);
void slsi_stop_locked(struct slsi_dev *sdev);
#ifdef CONFIG_SCSC_WLAN_EHT
struct slsi_peer *slsi_get_link_peer_from_mac(struct net_device *dev, const u8 *mac);
struct slsi_peer *slsi_sta_add_peer_link(struct slsi_dev *sdev, struct net_device *dev,
					 u8 *peer_address, u16 link_id);
#endif
struct slsi_peer *slsi_peer_add(struct slsi_dev *sdev, struct net_device *dev, u8 *peer_address, u16 aid);
void slsi_peer_update_assoc_req(struct slsi_dev *sdev, struct net_device *dev, struct slsi_peer *peer, struct sk_buff *skb);
void slsi_peer_update_assoc_rsp(struct slsi_dev *sdev, struct net_device *dev, struct slsi_peer *peer, struct sk_buff *skb);
void slsi_peer_reset_stats(struct slsi_dev *sdev, struct net_device *dev, struct slsi_peer *peer);
int slsi_peer_remove(struct slsi_dev *sdev, struct net_device *dev, struct slsi_peer *peer);
int slsi_ps_port_control(struct slsi_dev *sdev, struct net_device *dev, struct slsi_peer *peer, enum slsi_sta_conn_state s);
int slsi_del_station(struct wiphy *wiphy, struct net_device *dev,
		     struct station_del_parameters *del_params);
int slsi_sta_ieee80211_mode(struct net_device *dev, u16 current_bss_channel_frequency);
int slsi_vif_activated(struct slsi_dev *sdev, struct net_device *dev);
void slsi_vif_deactivated(struct slsi_dev *sdev, struct net_device *dev);
int slsi_handle_disconnect(struct slsi_dev *sdev, struct net_device *dev, u8 *peer_address, u16 reason,
			   u8 *disassoc_rsp_ie, u32 disassoc_rsp_ie_len
#ifdef CONFIG_SCSC_WLAN_EHT
			   , u16 mlo_vif
#endif
			   );
int slsi_band_update(struct slsi_dev *sdev, int band);
void slsi_band_cfg_update(struct slsi_dev *sdev, int band);
int slsi_ip_address_changed(struct slsi_dev *sdev, struct net_device *dev, __be32 ipaddress);
int slsi_send_gratuitous_arp(struct slsi_dev *sdev, struct net_device *dev);
struct ieee80211_channel *slsi_find_scan_channel(struct slsi_dev *sdev, struct ieee80211_mgmt *mgmt, size_t mgmt_len, u16 freq);
int slsi_auto_chan_select_scan(struct slsi_dev *sdev, int chan_count, struct ieee80211_channel *channels[]);
int slsi_set_uint_mib(struct slsi_dev *dev, struct net_device *ndev, u16 psid, int value);
int slsi_set_boolean_mib(struct slsi_dev *sdev, struct net_device *dev, u16 psid, bool val, int idx);
int slsi_update_regd_rules(struct slsi_dev *sdev, bool country_check);
int slsi_set_boost(struct slsi_dev *sdev, struct net_device *dev);
int slsi_p2p_init(struct slsi_dev *sdev, struct netdev_vif *ndev_vif);
void slsi_p2p_deinit(struct slsi_dev *sdev, struct netdev_vif *ndev_vif);
int slsi_p2p_vif_activate(struct slsi_dev *sdev, struct net_device *dev, struct ieee80211_channel *chan, u16 duration, bool set_probe_rsp_ies);
void slsi_p2p_vif_deactivate(struct slsi_dev *sdev, struct net_device *dev, bool hw_available);
void slsi_p2p_group_start_remove_unsync_vif(struct slsi_dev *sdev);
int slsi_p2p_dev_probe_rsp_ie(struct slsi_dev *sdev, struct net_device *dev, u8 *probe_rsp_ie, size_t probe_rsp_ie_len);
int slsi_p2p_dev_null_ies(struct slsi_dev *sdev, struct net_device *dev);
int slsi_get_public_action_subtype(const struct ieee80211_mgmt *mgmt);
int slsi_p2p_get_action_frame_status(struct net_device *dev, const struct ieee80211_mgmt *mgmt);
u8 slsi_get_exp_peer_frame_subtype(u8 subtype);
int slsi_send_txq_params(struct slsi_dev *sdev, struct net_device *ndev);
void slsi_abort_sta_scan(struct slsi_dev *sdev);
int slsi_is_dhcp_packet(u8 *data);
int  slsi_set_multicast_packet_filters(struct slsi_dev *sdev, struct net_device *dev);
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
void slsi_set_reset_connect_attempted_flag(struct slsi_dev *sdev, struct net_device *dev, const u8 *bssid);
bool slsi_select_ap_for_connection(struct slsi_dev *sdev, struct net_device *dev, const u8 **bssid,
				   struct ieee80211_channel **channel, bool retry);
#endif
#ifdef CONFIG_SCSC_WLAN_PRIORITISE_IMP_FRAMES
int slsi_is_dns_packet(u8 *data);
int slsi_is_mdns_packet(u8 *data);
int slsi_is_tcp_sync_packet(struct net_device *dev, struct sk_buff *skb);
#endif

#ifdef CONFIG_SCSC_WLAN_ENHANCED_PKT_FILTER
int slsi_set_enhanced_pkt_filter(struct net_device *dev, char *command, int buf_len);
#endif
void slsi_set_packet_filters(struct slsi_dev *sdev, struct net_device *dev);
int  slsi_update_packet_filters(struct slsi_dev *sdev, struct net_device *dev);
int  slsi_clear_packet_filters(struct slsi_dev *sdev, struct net_device *dev);
int slsi_ap_prepare_add_info_ies(struct netdev_vif *ndev_vif, const u8 *ies, size_t ies_len);
int slsi_set_mib_roam(struct slsi_dev *dev, struct net_device *ndev, u16 psid, int value);
int slsi_twt_update_ctrl_flags(struct net_device *dev, int enable);

#ifdef CONFIG_SCSC_WLAN_SET_PREFERRED_ANTENNA
int slsi_set_mib_preferred_antenna(struct slsi_dev *dev, u16 value);
#endif
void slsi_reset_throughput_stats(struct net_device *dev);
int slsi_set_mib_rssi_boost(struct slsi_dev *sdev, struct net_device *dev, u16 psid, int index, int boost);
#ifdef CONFIG_SCSC_WLAN_LOW_LATENCY_MODE
int slsi_set_mib_soft_roaming_enabled(struct slsi_dev *sdev, struct net_device *dev, bool enable);
#endif
#ifdef CONFIG_SCSC_WLAN_STA_ENHANCED_ARP_DETECT
int slsi_read_enhanced_arp_rx_count_by_lower_mac(struct slsi_dev *sdev, struct net_device *dev, u16 psid);
void slsi_fill_enhanced_arp_out_of_order_drop_counter(struct net_device *dev, struct sk_buff *skb);
#endif
void slsi_modify_ies_on_channel_switch(struct net_device *dev, struct cfg80211_ap_settings *settings,
				       struct ieee80211_mgmt *mgmt, u16 beacon_ie_head_len);
#ifdef CONFIG_SCSC_WLAN_WIFI_SHARING
void slsi_extract_valid_wifi_sharing_channels(struct slsi_dev *sdev);
int slsi_set_wifisharing_permitted_channels(struct net_device *dev, char *buffer, int buf_len);
#endif
#ifdef CONFIG_SCSC_WLAN_WIFI_SHARING_CSA_LEGACY
bool slsi_if_valid_wifi_sharing_channel(struct slsi_dev *sdev, int freq);
int slsi_check_if_non_indoor_non_dfs_channel(struct slsi_dev *sdev, int freq);
int slsi_get_mhs_ws_chan_vsdb(struct wiphy *wiphy, struct net_device *dev,
			      struct cfg80211_ap_settings *settings,
			      struct slsi_dev *sdev, int *wifi_sharing_channel_switched);
int slsi_get_mhs_ws_chan_rsdb(struct wiphy *wiphy, struct net_device *dev,
			      struct cfg80211_ap_settings *settings,
			      struct slsi_dev *sdev, int *wifi_sharing_channel_switched);
int slsi_check_if_channel_restricted_already(struct slsi_dev *sdev, int channel);
#endif
struct net_device *slsi_dynamic_interface_create(struct wiphy        *wiphy,
					     const char          *name,
					     enum nl80211_iftype type,
					     struct vif_params   *params,
					     bool is_cfg80211);
void slsi_stop_chip(struct slsi_dev *sdev);
int slsi_get_beacon_cu(struct slsi_dev *sdev, struct net_device *dev, int *mib_value);
int slsi_get_ps_disabled_duration(struct slsi_dev *sdev, struct net_device *dev, int *mib_value);
int slsi_get_ps_entry_counter(struct slsi_dev *sdev, struct net_device *dev, int *mib_value);
int slsi_get_mib_roam(struct slsi_dev *sdev, u16 psid, int *mib_value);
void slsi_roam_channel_cache_add_entry(struct slsi_dev *sdev, struct net_device *dev, const u8 *ssid, u8 ssid_len,
				       const u8 *bssid, u8 channel, enum nl80211_band band);
void slsi_roam_channel_cache_add(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_roam_channel_cache_prune(struct net_device *dev, int seconds, char *ssid);
int slsi_roaming_scan_configure_channels(struct slsi_dev *sdev, struct net_device *dev, const u8 *ssid, u16 *channels);
int slsi_send_max_transmit_msdu_lifetime(struct slsi_dev *dev, struct net_device *ndev, u32 msdu_lifetime);
int slsi_read_max_transmit_msdu_lifetime(struct slsi_dev *dev, struct net_device *ndev, u32 *msdu_lifetime);
int slsi_read_disconnect_ind_timeout(struct slsi_dev *sdev, u16 psid);
int slsi_read_regulatory_rules(struct slsi_dev *sdev, struct slsi_802_11d_reg_domain *domain_info, const char *alpha2);
int slsi_send_acs_event(struct slsi_dev *sdev, struct net_device *dev,
			struct slsi_acs_selected_channels acs_selected_channels);
struct slsi_roaming_network_map_entry *slsi_roam_channel_cache_get(struct net_device *dev, const u8 *ssid);
int slsi_roam_channel_cache_get_channels_int(struct net_device *dev,
					     struct slsi_roaming_network_map_entry *network_map,
					     u16 *channels);
int slsi_send_rcl_event(struct slsi_dev *sdev, u32 channel_count, u16 *channel_list, u8 *ssid, u8 ssid_len);
#ifdef CONFIG_SCSC_WLAN_ENABLE_MAC_RANDOMISATION
int slsi_set_mac_randomisation_mask(struct slsi_dev *sdev, u8 *mac_address_mask);
#endif
int slsi_set_country_update_regd(struct slsi_dev *sdev, const char *alpha2_code, int size);
void slsi_clear_offchannel_data(struct slsi_dev *sdev, bool acquire_lock);
int slsi_wlan_unsync_vif_activate(struct slsi_dev *sdev, struct net_device *dev,
				  struct ieee80211_channel *chan, u16 duration);
void slsi_wlan_unsync_vif_deactivate(struct slsi_dev *sdev, struct net_device *devbool, bool hw_available);
void slsi_hs2_unsync_vif_delete_work(struct work_struct *work);
bool slsi_is_bssid_in_blacklist(struct slsi_dev *sdev, struct net_device *dev, u8 *bssid);

int slsi_is_wes_action_frame(const struct ieee80211_mgmt *mgmt);
void slsi_scan_ind_timeout_handle(struct work_struct *work);
void slsi_blacklist_del_work_handle(struct work_struct *work);
void slsi_vif_cleanup(struct slsi_dev *sdev, struct net_device *dev, bool hw_available, bool is_recovery);
void slsi_scan_cleanup(struct slsi_dev *sdev, struct net_device *dev);
void slsi_dump_stats(struct net_device *dev);
int slsi_send_hanged_vendor_event(struct slsi_dev *sdev, u16 scsc_panic_code);
int slsi_set_ext_cap(struct slsi_dev *sdev, struct net_device *dev, const u8 *ies, int ie_len, const u8 *ext_cap_mask);
void slsi_update_supported_channels_regd_flags(struct slsi_dev *sdev);
#ifdef CONFIG_SCSC_WLAN_HANG_TEST
int slsi_test_send_hanged_vendor_event(struct net_device *dev);
#endif
int slsi_send_power_measurement_vendor_event(struct slsi_dev *sdev, s16 power_in_db);
#if defined(CONFIG_SLSI_WLAN_STA_FWD_BEACON) && (defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION >= 10)
int slsi_send_forward_beacon_vendor_event(struct slsi_dev *sdev, struct net_device *dev,
					  const u8 *ssid, const int ssid_len, const u8 *bssid, u8 channel,
					  const u16 beacon_int, const u64 timestamp, const u64 sys_time);
int slsi_send_forward_beacon_abort_vendor_event(struct slsi_dev *sdev, struct net_device *dev, u16 reason_code);
#endif
void slsi_wlan_dump_public_action_subtype(struct slsi_dev *sdev, struct ieee80211_mgmt *mgmt, bool tx);
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11) || defined(CONFIG_SCSC_WLAN_SUPPORT_6G)
u8 slsi_bss_connect_type_get(struct slsi_dev *sdev, const u8 *ie, size_t ie_len, u8 *sec_ie);
#endif
void slsi_reset_channel_flags(struct slsi_dev *sdev);
int slsi_merge_lists(u16 ar1[], int len1, u16 ar2[], int len2, u16 result[]);
int slsi_remove_duplicates(u16 arr[], int n);
void slsi_sort_array(u16 arr[], int n);
#ifdef CONFIG_SCSC_WLAN_SAR_SUPPORTED
int slsi_configure_tx_power_sar_scenario(struct net_device *dev, int mode);
#endif
/* Sysfs based mac address override */
void slsi_create_sysfs_macaddr(void);
void slsi_destroy_sysfs_macaddr(void);
/* Sysfs based version information */
void slsi_create_sysfs_version_info(void);
void slsi_destroy_sysfs_version_info(void);
#if defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION >= 12
void slsi_create_sysfs_debug_dump(void);
void slsi_destroy_sysfs_debug_dump(void);
void slsi_collect_chipset_logs(struct work_struct *work);
#endif
void slsi_create_sysfs_pm(void);
void slsi_destroy_sysfs_pm(void);
void slsi_create_sysfs_ant(void);
void slsi_destroy_sysfs_ant(void);
void slsi_create_sysfs_max_log_size(void);
void slsi_destroy_sysfs_max_log_size(void);
int slsi_find_chan_idx(u16 chan, u8 hw_mode, int band);
int slsi_set_latency_mode(struct net_device *dev, int latency_mode, int cmd_len);
void slsi_trigger_service_failure(struct work_struct *work);
void slsi_failure_reset(struct work_struct *work);
int slsi_start_ap(struct wiphy *wiphy, struct net_device *dev,
		  struct cfg80211_ap_settings *settings);
void slsi_subsystem_reset(struct work_struct *work);
void slsi_wakeup_time_work(struct work_struct *work);
void slsi_chip_recovery(struct work_struct *work);
void slsi_system_error_recovery(struct work_struct *work);
int slsi_set_acl(struct slsi_dev *sdev, struct net_device *dev);
void slsi_purge_blacklist(struct netdev_vif *ndev_vif);
void slsi_rx_update_mlme_stats(struct slsi_dev *sdev, struct sk_buff *skb);
void slsi_rx_update_wake_stats(struct slsi_dev *sdev, struct ethhdr *ehdr, size_t buff_len, struct sk_buff *skb);
int slsi_set_latency_crt_data(struct net_device *dev, int latency_mode);
bool slsi_is_bssid_in_hal_blacklist(struct net_device *dev, u8 *bssid);
bool slsi_is_bssid_in_ioctl_blacklist(struct net_device *dev, u8 *bssid);
int slsi_remove_bssid_blacklist(struct slsi_dev *sdev, struct net_device *dev, u8 *addr);
int slsi_add_ioctl_blacklist(struct slsi_dev *sdev, struct net_device *dev, u8 *addr);
u8 *slsi_get_scan_extra_ies(struct slsi_dev *sdev, const u8 *ies,
			    int total_len, int *extra_len);
int slsi_set_boolean_mib(struct slsi_dev *sdev, struct net_device *dev, u16 psid, bool val, int idx);
#ifdef CONFIG_SCSC_WLAN_EHT
int slsi_get_ap_mld_addr(struct slsi_dev *sdev, const struct element *ml_ie, u8 *mld_addr);
#endif
#ifdef CONFIG_SCSC_WLAN_DYNAMIC_ITO
int slsi_set_ito(struct net_device *dev, char *command, int buf_len);
int slsi_enable_ito(struct net_device *dev, char *command, int buf_len);
#endif
#if !defined(CONFIG_SCSC_WLAN_TX_API) && defined(CONFIG_SCSC_WLAN_ARP_FLOW_CONTROL)
void slsi_arp_q_stuck_work_handle(struct work_struct *work);
#endif
int slsi_fill_ap_sta_info(struct slsi_dev *sdev, struct net_device *dev,
			  const u8 *peer_mac, struct slsi_ap_sta_info *peer_info, const u16 reason_code);
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
int slsi_retry_connection(struct slsi_dev *sdev, struct net_device *dev);
void slsi_free_connection_params(struct slsi_dev *sdev, struct net_device *dev);
#endif
int slsi_add_probe_ies_request(struct slsi_dev *sdev, struct net_device *dev);
int slsi_dump_eth_packet(struct slsi_dev *sdev, struct sk_buff *skb);
int slsi_send_twt_setup_event(struct slsi_dev *sdev, struct net_device *dev, struct slsi_twt_setup_event setup_event);
int slsi_send_twt_teardown(struct slsi_dev *sdev, struct net_device *dev, u16 setup_id, u8 result_code);
int slsi_send_twt_notification(struct slsi_dev *sdev, struct net_device *dev);

int slsi_set_mib_obss_pd_enable(struct slsi_dev *sdev, struct net_device *dev, bool enable, int rssi);
int slsi_set_mib_obss_pd_enable_per_obss(struct slsi_dev *sdev, struct net_device *dev,
					 int config_cnt, int *rssi, u8 mac_addr[][ETH_ALEN]);
int slsi_set_mib_srp_non_srg_obss_pd_prohibited(struct slsi_dev *sdev, bool enable);

bool slsi_release_dp_resources(struct slsi_dev *sdev, struct net_device *dev, struct netdev_vif *ndev_vif);
u16 slsi_get_vifnum_by_ifnum(struct slsi_dev *sdev, int ifnum);
int slsi_get_ifnum_by_vifid(struct slsi_dev *sdev, u16 vif_id);
void slsi_mlme_clear_vif(struct slsi_dev *sdev, struct net_device *dev, u16 vif_id);
void slsi_mlme_assign_vif(struct slsi_dev *sdev, struct net_device *dev, u16 vif_id);
bool slsi_is_valid_vifnum(struct slsi_dev *sdev, struct net_device *dev);
void slsi_monitor_set_if_with_vif(struct slsi_dev *sdev, struct netdev_vif *ndev_vif, int ifnum);

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
int slsi_set_mib_6g_safe_mode(struct slsi_dev *sdev, bool enable);
#endif
#endif /*__SLSI_MGT_H__*/
