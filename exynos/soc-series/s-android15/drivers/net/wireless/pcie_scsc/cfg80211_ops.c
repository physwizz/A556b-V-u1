/***************************************************************************
 *
 * Copyright (c) 2014 - 2024 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#include <linux/version.h>
#include <net/cfg80211.h>
#include <linux/etherdevice.h>
#include "dev.h"
#include "cfg80211_ops.h"
#include "debug.h"
#include "mgt.h"
#include "mlme.h"
#include "netif.h"
#include "unifiio.h"
#include "mib.h"
#include "scsc_wifilogger_ring_pktfate_api.h"
#include "scsc_wifilogger_ring_connectivity_api.h"
#include "log2us.h"
#include "tdls_manager.h"
#include "ba.h"
#include <pcie_scsc/scsc_warn.h>
#include "utils_80211.h"

#ifdef CONFIG_SCSC_WLAN_ANDROID
#include "scsc_wifilogger_rings.h"
#endif
#include "nl80211_vendor.h"

/* Ext capab is decided by firmware. But there are certain bits
 * which are set by supplicant. So we set the capab and mask in
 * such way so that supplicant sets only the bits our solution supports
 */

static const u8                    slsi_extended_cap[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8                    slsi_extended_cap_mask[] = {
	0xFF, 0xFF, 0xF7, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
#define SLSI_DEFAULT_CH_MAX	(64 + SLSI_NUM_6GHZ_CHANNELS)
#else
#define SLSI_DEFAULT_CH_MAX	64
#endif

struct slsi_scan_params {
	struct ieee80211_channel *channels[SLSI_DEFAULT_CH_MAX];
	int chan_count;
	int scan_type;
	u8 *scan_ie;
	size_t scan_ie_len;
	int strip_wsc;
	int strip_p2p;
};

static uint keep_alive_period = SLSI_P2PGO_KEEP_ALIVE_PERIOD_SEC;
module_param(keep_alive_period, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(keep_alive_period, "default is 10 seconds");
static uint monitor_vif_set = 1;
module_param(monitor_vif_set, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(monitor_vif_set, "monitor vif(default:1)");


static bool slsi_is_mhs_active(struct slsi_dev *sdev, u8 *ifname)
{
	struct net_device *mhs_dev = NULL;
	struct netdev_vif *ndev_vif;
	bool ret;

	mhs_dev = slsi_get_netdev_by_ifname(sdev, ifname);
	if (mhs_dev) {
		ndev_vif = netdev_priv(mhs_dev);
		SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
		ret = ndev_vif->activated;
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return ret;
	}

	return 0;
}

static bool slsi_is_160mhz_supported(struct slsi_dev *sdev)
{
	int support_for_160;

	support_for_160 = sdev->fw_vht_cap[3] << 24 | sdev->fw_vht_cap[2] << 16 |
			  sdev->fw_vht_cap[1] << 8 | sdev->fw_vht_cap[0];
	if ((support_for_160 & VHT_CAP_INFO_CHAN_WIDTH_SET_MASK) == VHT_CAP_INFO_SUPPORTED_BW_160_80P80)
		return 1;

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
struct wireless_dev *slsi_add_virtual_intf(struct wiphy        *wiphy,
					   const char          *name,
					   unsigned char       name_assign_type,
					   enum nl80211_iftype type,
					   struct vif_params   *params)
{
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
struct wireless_dev *slsi_add_virtual_intf(struct wiphy        *wiphy,
					   const char          *name,
					   unsigned char       name_assign_type,
					   enum nl80211_iftype type,
					   u32                 *flags,
					   struct vif_params   *params)
{
#else
struct wireless_dev *slsi_add_virtual_intf(struct wiphy        *wiphy,
					   const char          *name,
					   enum nl80211_iftype type,
					   u32                 *flags,
					   struct vif_params   *params)
{
#endif

	struct net_device *dev = NULL;
	struct netdev_vif *ndev_vif = NULL;
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	bool is_cfg80211 = true;
	char *ap_if_name, *tmp_name, apvlan_name[IFNAMSIZ];

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(4, 11, 0))
	SLSI_UNUSED_PARAMETER(flags);
#endif
	SLSI_NET_DBG1(dev, SLSI_CFG80211, "Intf name:%s, type:%d, macaddr:" MACSTR "\n",
		      name, type, MAC2STR(params->macaddr));

	if (type == NL80211_IFTYPE_AP_VLAN) {
		if (sdev->num_ap_vlan == SLSI_MAX_AP_VLAN) {
			SLSI_ERR(sdev, "Maximum number of VLAN limits reached.\n");
			return ERR_PTR(-EINVAL);
		}

		strcpy(apvlan_name, name);
		tmp_name = apvlan_name;
		ap_if_name = strsep(&tmp_name, ".");
		if (!slsi_is_mhs_active(sdev, ap_if_name)) {
			SLSI_ERR(sdev, "Cannot add AP_VLAN interface when MHS is not active\n");
			return ERR_PTR(-EINVAL);
		}

		dev = slsi_dynamic_interface_create(wiphy, name, type, params, is_cfg80211);
		if (!dev) {
			SLSI_ERR(sdev, "AP_VLAN interface creation failed\n");
			return ERR_PTR(-ENODEV);
		}

		ndev_vif = netdev_priv(dev);
		sdev->num_ap_vlan++;
		rcu_assign_pointer(ndev_vif->netdev_ap,
				   slsi_get_netdev_by_ifname(sdev, ap_if_name));
	} else {
		rcu_read_lock();
		if (slsi_is_mhs_active(sdev, sdev->netdev_ap->name)) {
			rcu_read_unlock();
			SLSI_ERR(sdev, "MHS is active. cannot add new interface\n");
			return ERR_PTR(-EOPNOTSUPP);
		}
		rcu_read_unlock();
		dev = slsi_dynamic_interface_create(wiphy, name, type, params, is_cfg80211);
		if (!dev)
			return ERR_PTR(-ENODEV);
		ndev_vif = netdev_priv(dev);
	}

	return &ndev_vif->wdev;
}

int slsi_del_virtual_intf(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	struct net_device *dev = wdev->netdev;
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	bool is_cfg80211 = true;

	if (WLBT_WARN_ON(!dev))
		return -EINVAL;

	SLSI_NET_DBG1(dev, SLSI_CFG80211, "Dev name:%s\n", dev->name);

	/* for p2p-wlan0-0 or p2p-p2p0-x */
	if (strncmp(dev->name, "p2p-", 4) == 0) {
		struct netdev_vif *ndev_vif_p2p = NULL;

		ndev_vif_p2p = netdev_priv(sdev->netdev[SLSI_NET_INDEX_P2P]);
		if (sdev->p2p_state != P2P_IDLE_NO_VIF && ndev_vif_p2p && ndev_vif_p2p->drv_in_p2p_procedure) {
			ndev_vif_p2p->drv_in_p2p_procedure = false;
			SLSI_NET_DBG1(dev, SLSI_CFG80211, "drv_in_p2p_procedure = %d, sdev->p2p_state=%s\n",
				      ndev_vif_p2p->drv_in_p2p_procedure, slsi_p2p_state_text(sdev->p2p_state));
		}
	}

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	slsi_stop_net_dev(sdev, dev);
	slsi_netif_remove_locked(sdev, dev, is_cfg80211);

	if (ndev_vif->iftype == NL80211_IFTYPE_AP_VLAN) {
		sdev->num_ap_vlan--;
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
		return 0;
	}

	if (dev == sdev->netdev_ap)
		rcu_assign_pointer(sdev->netdev_ap, NULL);
	if (!sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN])
		rcu_assign_pointer(sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN], sdev->netdev_ap);
	if (dev == sdev->netdev_p2p)
		rcu_assign_pointer(sdev->netdev_p2p, NULL);
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
int slsi_change_virtual_intf(struct wiphy *wiphy,
			     struct net_device *dev,
			     enum nl80211_iftype type,
			     struct vif_params *params)
{
#else
int slsi_change_virtual_intf(struct wiphy *wiphy,
			     struct net_device *dev,
			     enum nl80211_iftype type,
			     u32 *flags,
			     struct vif_params *params)
{
#endif

	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int               r = 0;

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(4, 11, 0))
	SLSI_UNUSED_PARAMETER(flags);
#endif

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_DBG1(dev, SLSI_CFG80211, "type:%u, iftype:%d\n", type, ndev_vif->iftype);

	if (WLBT_WARN_ON(ndev_vif->activated)) {
		r = -EINVAL;
		goto exit;
	}

	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
	case NL80211_IFTYPE_ADHOC:
	case NL80211_IFTYPE_STATION:
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_P2P_GO:
	case NL80211_IFTYPE_MONITOR:
		ndev_vif->iftype = type;
		dev->ieee80211_ptr->iftype = type;
		if (params)
			dev->ieee80211_ptr->use_4addr = params->use_4addr;
		break;
	default:
		r = -EINVAL;
		break;
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
int slsi_add_key(struct wiphy *wiphy, struct net_device *dev,
		 int link_id, u8 key_index, bool pairwise, const u8 *mac_addr,
		 struct key_params *params)
#else
int slsi_add_key(struct wiphy *wiphy, struct net_device *dev,
		 u8 key_index, bool pairwise, const u8 *mac_addr,
		 struct key_params *params)
#endif
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_peer  *peer = NULL;
	int               r = 0;
	u16               key_type = FAPI_KEYTYPE_GROUP;
	int               group_key_index = 0;
	struct net_device *ap_dev = NULL;

	if (WLBT_WARN_ON(pairwise && !mac_addr))
		return -EINVAL;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!mac_addr)
		SLSI_NET_DBG2(dev, SLSI_CFG80211,
			      "(key_index:%d, pairwise:%d, address:null, cipher:0x%.8X, key_len:%d, vif_type:%d)\n",
			      key_index, pairwise, params->cipher, params->key_len, ndev_vif->vif_type);
	else
		SLSI_NET_DBG2(dev, SLSI_CFG80211,
			      "(key_index:%d, pairwise:%d, address:" MACSTR ", cipher:0x%.8X, key_len:%d, vif_type:%d)\n",
			      key_index, pairwise, MAC2STR(mac_addr), params->cipher, params->key_len,
			      ndev_vif->vif_type);

	if (!ndev_vif->activated) {
		SLSI_NET_DBG1(dev, SLSI_CFG80211, "vif not active\n");
		goto exit;
	}

	if (params->cipher == WLAN_CIPHER_SUITE_PMK) {
		r = slsi_mlme_set_key(sdev, dev, key_index, key_type, mac_addr, params);
		goto exit;
	}

	/* FW doesn't know about VLAN vif. Use AP vif. */
	if (ndev_vif->iftype == NL80211_IFTYPE_AP_VLAN && !pairwise) {
		group_key_index = slsi_get_group_key_idx(dev->name);
		if (group_key_index < 0) {
			SLSI_ERR(sdev, "Failed to get group key index for VLAN interface\n");
			r = group_key_index;
			goto exit;
		}
		ndev_vif->group_key_index = group_key_index;
		rcu_read_lock();
		ap_dev = ndev_vif->netdev_ap;
		mac_addr = ap_dev->dev_addr;
		rcu_read_unlock();
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		ndev_vif = netdev_priv(ndev_vif->netdev_ap);
		SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	}

	if (mac_addr && pairwise) {
		/* All Pairwise Keys will have a peer record. */
		peer = slsi_get_peer_from_mac(sdev, dev, mac_addr);
		if (peer)
			mac_addr = peer->address;
#ifdef CONFIG_SCSC_WLAN_EHT
		if (link_id >= 0 && link_id < MAX_NUM_MLD_LINKS)
			mac_addr = ndev_vif->sta.links[link_id].bssid;
#endif
	} else if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION) {
		/* Sta Group Key will use the peer address */
		peer = slsi_get_peer_from_qs(sdev, dev, SLSI_STA_PEER_QUEUESET);
		if (peer)
			mac_addr = peer->address;
#ifdef CONFIG_SCSC_WLAN_EHT
		if (link_id >= 0 && link_id < MAX_NUM_MLD_LINKS)
			mac_addr = ndev_vif->sta.links[link_id].bssid;
#endif
	} else if (ndev_vif->vif_type == FAPI_VIFTYPE_AP && !pairwise) {
		/* AP Group Key will use the Interface address */
		if (!mac_addr)
			mac_addr = dev->dev_addr;
	} else {
		r = -EINVAL;
		goto exit;
	}

	/*Treat WEP key as pairwise key*/
	if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION &&
	    (params->cipher == WLAN_CIPHER_SUITE_WEP40 ||
	     params->cipher == WLAN_CIPHER_SUITE_WEP104) && peer) {
		u8 bc_mac_addr[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

		SLSI_NET_DBG3(dev, SLSI_CFG80211, "WEP Key: store key\n");
		r = slsi_mlme_set_key(sdev, dev, key_index, FAPI_KEYTYPE_WEP, bc_mac_addr, params);
		if (r == FAPI_RESULTCODE_SUCCESS) {
			ndev_vif->sta.wep_key_set = true;
			/* if static ip is set before connection, after setting keys enable powersave. */
			if (ndev_vif->ipaddress)
				slsi_mlme_powermgt(sdev, dev, ndev_vif->set_power_mode);
		} else {
			SLSI_NET_ERR(dev, "Error adding WEP key\n");
		}
		goto exit;
	}

	if (pairwise) {
		key_type = FAPI_KEYTYPE_PAIRWISE;
		if (WLBT_WARN_ON(!peer)) {
			r = -EINVAL;
			goto exit;
		}
	} else if (params->cipher == WLAN_CIPHER_SUITE_AES_CMAC || params->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_128 ||
				params->cipher == WLAN_CIPHER_SUITE_BIP_GMAC_256) {
		key_type = FAPI_KEYTYPE_IGTK;
	}

	if (key_index == 6 || key_index == 7)
		key_type = FAPI_KEYTYPE_BIGTK;

	if (WLBT_WARN(!mac_addr, "mac_addr not defined\n")) {
		r = -EINVAL;
		goto exit;
	}
	if (!(ndev_vif->vif_type == FAPI_VIFTYPE_AP && key_index == 4)) {
		r = slsi_mlme_set_key(sdev, (!ap_dev) ? dev : ap_dev,
				      group_key_index ? group_key_index : key_index,
				      key_type, mac_addr, params);
		if (r) {
			SLSI_NET_ERR(dev, "error in adding key (key_type: %d)\n", key_type);
			goto exit;
		}
	}

	if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION) {
		ndev_vif->sta.eap_hosttag = 0xFFFF;
		/* if static IP is set before connection, after setting keys enable powersave. */
		if (ndev_vif->ipaddress)
			slsi_mlme_powermgt(sdev, dev, ndev_vif->set_power_mode);
	}

	if (key_type == FAPI_KEYTYPE_GROUP) {
		ndev_vif->sta.group_key_set = true;
		ndev_vif->ap.cipher = params->cipher;
	} else if (key_type == FAPI_KEYTYPE_PAIRWISE) {
		if (peer) {
			slsi_ba_replay_reset_pn(dev, peer);
			peer->pairwise_key_set = true;
		}
	}

	if (peer) {
		if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION) {
			if (pairwise && params->cipher == WLAN_CIPHER_SUITE_SMS4) {
				slsi_mlme_connect_resp(sdev, dev);
				if (ndev_vif->ipaddress)
					slsi_ip_address_changed(sdev, dev, ndev_vif->ipaddress);

				slsi_set_acl(sdev, dev);
				slsi_set_packet_filters(sdev, dev);
				slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_CONNECTED);
			}

			if (ndev_vif->sta.fils_connection) {
				if (ndev_vif->sta.resp_id == MLME_ROAMED_RES) {
					slsi_mlme_roamed_resp(sdev, dev);
					ndev_vif->sta.resp_id = 0;
				} else {
					slsi_mlme_connect_resp(sdev, dev);
				}
				if (ndev_vif->ipaddress)
					slsi_ip_address_changed(sdev, dev, ndev_vif->ipaddress);
				slsi_set_acl(sdev, dev);
				slsi_set_packet_filters(sdev, dev);
				slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_CONNECTED);
				ndev_vif->sta.fils_connection = 0;
			}

			if (ndev_vif->sta.gratuitous_arp_needed) {
				ndev_vif->sta.gratuitous_arp_needed = false;
				slsi_send_gratuitous_arp(sdev, dev);
			}

		} else if (ndev_vif->vif_type == FAPI_VIFTYPE_AP && pairwise) {
			slsi_mlme_connected_resp(sdev, dev, peer->flow_id);
			slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_CONNECTED);
			peer->connected_state = SLSI_STA_CONN_STATE_CONNECTED;
			if (ndev_vif->iftype == NL80211_IFTYPE_P2P_GO)
				ndev_vif->ap.p2p_gc_keys_set = true;
		}
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
int slsi_del_key(struct wiphy *wiphy, struct net_device *dev,
		 int link_id, u8 key_index, bool pairwise, const u8 *mac_addr)
#else
int slsi_del_key(struct wiphy *wiphy, struct net_device *dev,
		 u8 key_index, bool pairwise, const u8 *mac_addr)
#endif
{
	SLSI_UNUSED_PARAMETER(wiphy);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	SLSI_UNUSED_PARAMETER(link_id);
#endif
	SLSI_UNUSED_PARAMETER(key_index);
	SLSI_UNUSED_PARAMETER(pairwise);
	SLSI_UNUSED_PARAMETER(mac_addr);

	if (slsi_is_test_mode_enabled()) {
		SLSI_NET_INFO(dev, "Skip sending signal, WlanLite FW does not support MLME_DELETEKEYS.request\n");
		return -EOPNOTSUPP;
	}

	return 0;
}

int slsi_change_station(struct wiphy *wiphy, struct net_device *dev,
			const u8 *mac, struct station_parameters *params)
{
	struct netdev_vif          *ndev_vif = netdev_priv(dev);
	struct slsi_dev            *sdev = SDEV_FROM_WIPHY(wiphy);
	enum cfg80211_station_type statype;
	struct net_device          *vlan = NULL;
	struct slsi_peer           *peer = NULL;
	int                        err = 0;
	char                       apvlan_name[10];
	int                        group_key_index = 0;

	switch (ndev_vif->iftype) {
	case NL80211_IFTYPE_ADHOC:
		statype = CFG80211_STA_IBSS;
		break;
	case NL80211_IFTYPE_STATION:
		if (!(params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))) {
			statype = CFG80211_STA_AP_STA;
			break;
		}

		if (params->sta_flags_set & BIT(NL80211_STA_FLAG_AUTHORIZED))
			statype = CFG80211_STA_TDLS_PEER_ACTIVE;
		else
			statype = CFG80211_STA_TDLS_PEER_SETUP;
		break;
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_AP_VLAN:
		if (params->sta_flags_set & BIT(NL80211_STA_FLAG_ASSOCIATED))
			statype = CFG80211_STA_AP_CLIENT;
		else
			statype = CFG80211_STA_AP_CLIENT_UNASSOC;
		break;
	default:
		return -EOPNOTSUPP;
	}

	err = cfg80211_check_station_change(wiphy, params, statype);
	if (err)
		return err;

	if (!params->vlan)
		return 0;

	/* Map group key index with corresponding peer structure */
	vlan = params->vlan;
	strcpy(apvlan_name, vlan->name);
	group_key_index = slsi_get_group_key_idx(apvlan_name);
	if (group_key_index < 0) {
		SLSI_ERR(sdev, "Failed to get group key index for VLAN interface\n");
		return group_key_index;
	}

	peer = slsi_get_peer_from_mac(sdev, dev, mac);
	if (!peer) {
		SLSI_ERR(sdev, "No peer record found.\n");
		return -ENOENT;
	}
	peer->group_key_index = group_key_index;
	rcu_assign_pointer(peer->netdev_vlan, vlan);

	return 0;
}

int slsi_channel_switch(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_csa_settings *params)
{
	struct netdev_vif        *ndev_vif = netdev_priv(dev);
	struct netdev_vif        *ap_dev_vif;
	struct slsi_dev          *sdev = ndev_vif->sdev;
	struct net_device        *wlan_dev;
	struct netdev_vif        *ndev_sta_vif;
	int                      result = 0;
	u16                      center_freq = 0;
	u16                      chan_info = 0;
	u16                      current_chan_info = 0;
	struct cfg80211_chan_def chandef = params->chandef;
	struct cfg80211_chan_def current_chandef = ndev_vif->chandef_saved;
	struct net_device        *ap_dev = NULL;
	struct ieee80211_channel *chan = chandef.chan;
	int                      width  = chandef.width;

	wlan_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (ndev_vif->iftype != NL80211_IFTYPE_AP) {
		SLSI_NET_ERR(dev, "AP Mode is not active\n");
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -EPERM;
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	ndev_sta_vif = netdev_priv(wlan_dev);
#ifdef CONFIG_SCSC_WLAN_WIFI_SHARING
	if (SLSI_IS_VIF_INDEX_MHS(sdev, ndev_vif)) {
		if (ndev_sta_vif) {
			SLSI_MUTEX_LOCK(ndev_sta_vif->vif_mutex);
			if (ndev_sta_vif->activated && ndev_sta_vif->vif_type == FAPI_VIFTYPE_STATION &&
			    (ndev_sta_vif->sta.vif_status == SLSI_VIF_STATUS_CONNECTING ||
			     ndev_sta_vif->sta.vif_status == SLSI_VIF_STATUS_CONNECTED)) {
				SLSI_NET_ERR(dev, "Sta is in connected state (Chan Switch not allowed in WiFi Sharing mode)\n");
				SLSI_MUTEX_UNLOCK(ndev_sta_vif->vif_mutex);
				return -EPERM;
			}
			SLSI_MUTEX_UNLOCK(ndev_sta_vif->vif_mutex);
		}
	}
#endif

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (current_chandef.chan->center_freq == chan->center_freq) {
		current_chan_info = slsi_get_chann_info(sdev, &current_chandef);
		if (current_chan_info == slsi_get_chann_info(sdev, &chandef)) {
			SLSI_NET_ERR(dev, "Channel Switch requested on current channel freq-> %u and Chan info %d\n", current_chandef.chan->center_freq, current_chan_info);
			SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
			return -EPERM;
			}
		}
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	ap_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_P2PX_SWLAN);
	chan_info = slsi_get_chann_info(sdev, &chandef);
	center_freq = chan->center_freq;
	if (width != NL80211_CHAN_WIDTH_20_NOHT && width != NL80211_CHAN_WIDTH_20)
		center_freq = slsi_get_center_freq1(sdev, chan_info, center_freq);
	ap_dev_vif = netdev_priv(ap_dev);
	SLSI_MUTEX_LOCK(ap_dev_vif->vif_mutex);
	result = slsi_mlme_channel_switch(sdev, ap_dev, center_freq, chan_info);
	SLSI_MUTEX_UNLOCK(ap_dev_vif->vif_mutex);
	return result;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
int slsi_get_key(struct wiphy *wiphy, struct net_device *dev,
		 int link_id, u8 key_index, bool pairwise, const u8 *mac_addr,
		 void *cookie,
		 void (*callback)(void *cookie, struct key_params *))
#else
int slsi_get_key(struct wiphy *wiphy, struct net_device *dev,
		 u8 key_index, bool pairwise, const u8 *mac_addr,
		 void *cookie,
		 void (*callback)(void *cookie, struct key_params *))
#endif
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct key_params params;

#define SLSI_MAX_KEY_SIZE 8 /*used only for AP case, so WAPI not considered*/
	u8                key_seq[SLSI_MAX_KEY_SIZE] = { 0 };
	int               r = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	SLSI_UNUSED_PARAMETER(link_id);
#endif
	SLSI_UNUSED_PARAMETER(mac_addr);

	if (slsi_is_test_mode_enabled()) {
		SLSI_NET_INFO(dev, "Skip sending signal, WlanLite FW does not support MLME_GET_KEY_SEQUENCE.request\n");
		return -EOPNOTSUPP;
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!mac_addr)
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "(key_index:%d, pairwise:%d, mac_addr:null, vif_type:%d)\n",
			      key_index, pairwise, ndev_vif->vif_type);
	else
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "(key_index:%d, pairwise:%d, mac_addr:" MACSTR ", vif_type:%d)\n",
			      key_index, pairwise, MAC2STR(mac_addr), ndev_vif->vif_type);

	if (!ndev_vif->activated) {
		SLSI_NET_ERR(dev, "vif not active\n");
		r = -EINVAL;
		goto exit;
	}

	/* The get_key call is expected only for AP vif with Group Key type */
	if (FAPI_VIFTYPE_AP != ndev_vif->vif_type) {
		SLSI_NET_ERR(dev, "Invalid vif type: %d\n", ndev_vif->vif_type);
		r = -EINVAL;
		goto exit;
	}
	if (pairwise) {
		SLSI_NET_ERR(dev, "Invalid key type\n");
		r = -EINVAL;
		goto exit;
	}

	memset(&params, 0, sizeof(params));
	/* Update params with sequence number, key field would be updated NULL */
	params.key = NULL;
	params.key_len = 0;
	params.cipher = ndev_vif->ap.cipher;
	if (!(ndev_vif->vif_type == FAPI_VIFTYPE_AP && key_index == 4)) {
		r = slsi_mlme_get_key(sdev, dev, key_index, FAPI_KEYTYPE_GROUP, key_seq, &params.seq_len);

		if (!r) {
			params.seq = key_seq;
			callback(cookie, &params);
		}
	}
#undef SLSI_MAX_KEY_SIZE
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

static bool slsi_is_p2p_scan_req(struct cfg80211_scan_request *request)
{
	if (request->ie &&
	    cfg80211_find_vendor_ie(WLAN_OUI_WFA, WLAN_OUI_TYPE_WFA_P2P, request->ie, request->ie_len) &&
	    request->ssids && SLSI_IS_P2P_SSID(request->ssids[0].ssid, request->ssids[0].ssid_len))
		return true;
	return false;
}

static void slsi_p2p_cancel_unset_channel(struct slsi_dev *sdev, struct cfg80211_scan_request *request)
{
	struct net_device *dev = request->wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (!slsi_is_p2p_scan_req(request))
		return;

	if (ndev_vif->drv_in_p2p_procedure)
		return;

	if (delayed_work_pending(&ndev_vif->unsync.unset_channel_expiry_work)) {
		cancel_delayed_work(&ndev_vif->unsync.unset_channel_expiry_work);
		slsi_mlme_unset_channel_req(sdev, dev);
		ndev_vif->driver_channel = 0;
	}
}

static void slsi_set_wlan_scan_type_param(struct slsi_dev *sdev, struct net_device *dev,
					  struct slsi_scan_params *params)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	params->scan_type = FAPI_SCANTYPE_FULL_SCAN;
	ndev_vif->unsync.slsi_p2p_continuous_fullscan = false;
	if (params->chan_count == 1) {
		params->scan_type = FAPI_SCANTYPE_SINGLE_CHANNEL_SCAN;
		return;
	}

	if (sdev->initial_scan) {
		sdev->initial_scan = false;
		if (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTING &&
		    ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED)
			params->scan_type = FAPI_SCANTYPE_INITIAL_SCAN;
	}
}

static void slsi_set_p2p_social_ch_param(struct cfg80211_scan_request *request,
					 struct slsi_scan_params *params)
{
	int count = 0, chann = 0, i = 0;

	for (i = 0; i < request->n_channels; i++) {
		chann = params->channels[i]->hw_value & 0xFF;
		if (SLSI_P2P_IS_SOCAIL_CHAN(chann)) {
			params->channels[count] = request->channels[i];
			count++;
		}
	}
	params->chan_count = count;
}

static void slsi_set_p2p_scan_type_param(struct slsi_dev *sdev, struct net_device *dev,
					 struct cfg80211_scan_request *request,
					 struct slsi_scan_params *params)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (sdev->p2p_state == P2P_GROUP_FORMED_GO)
		ndev_vif->unsync.slsi_p2p_continuous_fullscan = false;

	/* In supplicant during joining procedure the P2P GO scan
	 * with GO's operating channel comes on P2P device. Hence added the
	 * check for n_channels as 1
	 */
	if (request->n_channels == SLSI_P2P_SOCIAL_CHAN_COUNT || request->n_channels == 1) {
		params->scan_type = FAPI_SCANTYPE_P2P_SCAN_SOCIAL;
		ndev_vif->unsync.slsi_p2p_continuous_fullscan = false;
	} else if (request->n_channels > SLSI_P2P_SOCIAL_CHAN_COUNT) {
		if (!ndev_vif->unsync.slsi_p2p_continuous_fullscan) {
			params->scan_type = FAPI_SCANTYPE_P2P_SCAN_FULL;
			ndev_vif->unsync.slsi_p2p_continuous_fullscan = true;
		} else {
			params->scan_type = FAPI_SCANTYPE_P2P_SCAN_SOCIAL;
			ndev_vif->unsync.slsi_p2p_continuous_fullscan = false;
			slsi_set_p2p_social_ch_param(request, params);
		}
	}
}

static void slsi_set_scan_type_param(struct slsi_dev *sdev, struct cfg80211_scan_request *request,
				     struct slsi_scan_params *params)
{
	struct net_device *dev = request->wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (SLSI_IS_VIF_INDEX_WLAN(ndev_vif))
		slsi_set_wlan_scan_type_param(sdev, dev, params);

	if (slsi_is_p2p_scan_req(request))
		slsi_set_p2p_scan_type_param(sdev, dev, request, params);
}

static bool slsi_is_p2p_wsc_ie_strip_condition(struct net_device *dev, struct cfg80211_scan_request *request,
					       struct slsi_scan_params *params)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

#ifdef CONFIG_SCSC_WLAN_DUAL_STATION
	if ((SLSI_IS_VIF_INDEX_WLAN(ndev_vif) || ndev_vif->ifnum == SLSI_NET_INDEX_P2PX_SWLAN) && request->ie &&
	    !(params->scan_type == FAPI_SCANTYPE_P2P_SCAN_SOCIAL || params->scan_type == FAPI_SCANTYPE_P2P_SCAN_FULL))
		return true;
#else
	if (SLSI_IS_VIF_INDEX_WLAN(ndev_vif) && request->ie)
		return true;
#endif
	return false;
}

static void slsi_scan_set_p2p_wps_strip_param(const u8 *request_ie, size_t request_ie_len,
					      struct slsi_scan_params *params)
{
	const u8 *ie;

	/* Supplicant adds wsc and p2p in Station scan at the end of scan request ie.
	 * for non-wps case remove both wps and p2p IEs
	 * for wps case remove only p2p IE
	 */
	ie = cfg80211_find_vendor_ie(WLAN_OUI_MICROSOFT, WLAN_OUI_TYPE_MICROSOFT_WPS, request_ie, request_ie_len);
	if (ie && ie[1] > SLSI_WPS_REQUEST_TYPE_POS) {
		/* Check whether scan is wps_scan or not, if not a wps_scan set strip_wsc to true
		 * to strip WPS IE
		 */
		if (ie[SLSI_WPS_REQUEST_TYPE_POS] == SLSI_WPS_REQUEST_TYPE_ENROLEE_INFO_ONLY)
			params->strip_wsc = true;
	}

	ie = cfg80211_find_vendor_ie(WLAN_OUI_WFA, WLAN_OUI_TYPE_WFA_P2P, request_ie, request_ie_len);
	if (ie)
		params->strip_p2p = true;
}

static size_t slsi_strip_wsc_p2p_ie(const u8 *src_ie, size_t src_ie_len, u8 *dest_ie, bool strip_wsc, bool strip_p2p)
{
	const u8 *ie;
	const u8 *next_ie;
	size_t   dest_ie_len = 0;

	if (!dest_ie || !(strip_p2p || strip_wsc))
		return dest_ie_len;

	for (ie = src_ie; (ie - src_ie) < src_ie_len; ie = next_ie) {
		next_ie = ie + ie[1] + 2;

		if (ie[0] == WLAN_EID_VENDOR_SPECIFIC && ie[1] > 4) {
			int          i;
			unsigned int oui = 0;

			for (i = 0; i < 4; i++)
				oui = (oui << 8) | ie[5 - i];

			if (strip_wsc && oui == SLSI_WPS_OUI_PATTERN)
				continue;
			if (strip_p2p && oui == SLSI_P2P_OUI_PATTERN)
				continue;
		}

		if (next_ie - src_ie <= src_ie_len) {
			memcpy(dest_ie + dest_ie_len, ie, ie[1] + 2);
			dest_ie_len += ie[1] + 2;
		}
	}

	return dest_ie_len;
}

static int slsi_alloc_scan_ie_param(struct net_device *dev, const u8 *req_ie, size_t req_ie_len,
				    struct slsi_scan_params *params)
{
	if ((params->strip_wsc || params->strip_p2p) && req_ie) {
		params->scan_ie = kmalloc(req_ie_len, GFP_KERNEL);
		if (!params->scan_ie) {
			SLSI_NET_INFO(dev, "Out of memory for scan IEs\n");
			return -ENOMEM;
		}
		params->scan_ie_len = slsi_strip_wsc_p2p_ie(req_ie, req_ie_len,
							    params->scan_ie, params->strip_wsc, params->strip_p2p);
	} else {
		params->scan_ie = (u8 *)req_ie;
		params->scan_ie_len = req_ie_len;
	}
	return 0;
}

static void slsi_free_scan_ie_param(struct slsi_scan_params *params)
{
	if (params->strip_p2p || params->strip_wsc)
		kfree(params->scan_ie);
}

#ifdef CONFIG_SCSC_WLAN_ENABLE_MAC_RANDOMISATION
static void slsi_set_mac_randomization(struct slsi_dev *sdev, struct cfg80211_scan_request *request,
				       struct slsi_scan_params *params)
{
	struct net_device *dev = request->wdev->netdev;
	u8 mac_addr_mask[ETH_ALEN] = {0xFF};
	bool wps_sta = false;
	int r = 0;
	const u8 *ie;

	ie = cfg80211_find_vendor_ie(WLAN_OUI_MICROSOFT, WLAN_OUI_TYPE_MICROSOFT_WPS, request->ie, request->ie_len);
	if (ie && ie[1] > SLSI_WPS_REQUEST_TYPE_POS)
		if (ie[SLSI_WPS_REQUEST_TYPE_POS] != SLSI_WPS_REQUEST_TYPE_ENROLEE_INFO_ONLY)
			wps_sta = true;

	/* If Supplicant triggers WPS scan on station interface,
	 * mac radomization for scan should be disabled to avoid WPS overlap.
	 * Firmware also disables Mac Randomization for WPS Scan.
	 */
	if (request->flags & NL80211_SCAN_FLAG_RANDOM_ADDR && !wps_sta) {
		if (sdev->fw_mac_randomization_enabled) {
			memcpy(sdev->scan_mac_addr, request->mac_addr, ETH_ALEN);
			r = slsi_set_mac_randomisation_mask(sdev, request->mac_addr_mask);
			if (!r)
				sdev->scan_addr_set = 1;
		} else {
			SLSI_NET_INFO(dev, "Mac Randomization is not enabled in Firmware\n");
			sdev->scan_addr_set = 0;
		}
	} else if (sdev->scan_addr_set) {
		memset(mac_addr_mask, 0xFF, ETH_ALEN);
		r = slsi_set_mac_randomisation_mask(sdev, mac_addr_mask);
		sdev->scan_addr_set = 0;
	}
}
#endif

#if defined(CONFIG_SLSI_WLAN_STA_FWD_BEACON) && (defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION >= 10)
static void slsi_set_sta_forward_beacon(struct slsi_dev *sdev, struct net_device *dev)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int ret = 0;

	if (!ndev_vif->is_wips_running)
		return;

	if (ndev_vif->vif_type != FAPI_VIFTYPE_STATION)
		return;

	if (ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED)
		return;

	SLSI_NET_DBG3(dev, SLSI_CFG80211, "Scan invokes DRIVER_BCN_ABORT\n");
	ret = slsi_mlme_set_forward_beacon(sdev, dev, FAPI_ACTION_STOP);
	if (ret < 0)
		ret = slsi_send_forward_beacon_abort_vendor_event(sdev, dev,
								  SLSI_FORWARD_BEACON_ABORT_REASON_SCANNING);
}
#endif

static void slsi_update_state_scan_in_device_role(struct slsi_dev *sdev, struct net_device *dev,
						  struct slsi_scan_params *params)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (!SLSI_IS_VIF_INDEX_P2P(ndev_vif) || SLSI_IS_P2P_GROUP_STATE(sdev))
		return;

	if (params->scan_type == FAPI_SCANTYPE_P2P_SCAN_SOCIAL)
		SLSI_P2P_STATE_CHANGE(sdev, P2P_SCANNING);
}

static void slsi_scan_save_probe_req_ies(struct cfg80211_scan_request *request,
					 struct slsi_scan_params *params)
{
	struct net_device *dev = request->wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (SLSI_IS_VIF_INDEX_P2P(ndev_vif) || !params->scan_ie_len)
		return;

	kfree(ndev_vif->probe_req_ies);
	ndev_vif->probe_req_ies = kmalloc(request->ie_len, GFP_KERNEL);
	if (!ndev_vif->probe_req_ies) { /* Don't fail, continue as it would still work */
		ndev_vif->probe_req_ie_len = 0;
	} else {
		ndev_vif->probe_req_ie_len = params->scan_ie_len;
		memcpy(ndev_vif->probe_req_ies, params->scan_ie, params->scan_ie_len);
	}
}

#if defined(CONFIG_SCSC_WLAN_HE) && (defined(SCSC_SEP_VERSION))
static void slsi_p2p_go_set_he_mode(struct slsi_dev *sdev, struct net_device *ndev, struct cfg80211_ap_settings *request)
{
	struct netdev_vif	*ndev_vif = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
	bool	 	        p2p_ax_mode = false;
#endif

	ndev_vif = netdev_priv(ndev);

	if (ndev_vif->ifnum != SLSI_NET_INDEX_P2PX_SWLAN || ndev_vif->iftype != NL80211_IFTYPE_P2P_GO)
		return;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
	if (request->he_cap) {
		p2p_ax_mode = true;
		SLSI_NET_DBG3(ndev, SLSI_CFG80211, "P2P GO 11ax mode\n");

		if (slsi_set_uint_mib(sdev, NULL, SLSI_PSID_UNIFI_HE_ACTIVATED_P2P_GO, p2p_ax_mode)) {
			SLSI_ERR(sdev, "Set HE_ACTIVATED_P2P_GO has failed\n");
			return;
		}
		ndev_vif->p2p_ax_mode = p2p_ax_mode;
	}
#endif
}
#endif

static void slsi_abort_hw_scan(struct slsi_dev *sdev, struct net_device *dev)
{
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);

	SLSI_NET_INFO(dev, "Abort on-going scan, ifnum:%d,"
		      "ndev_vif->scan[SLSI_SCAN_HW_ID].scan_req:%p\n", ndev_vif->ifnum,
		      ndev_vif->scan[SLSI_SCAN_HW_ID].scan_req);

	if (ndev_vif->scan[SLSI_SCAN_HW_ID].scan_req) {
		(void)slsi_mlme_del_scan(sdev, dev, ndev_vif->ifnum << 8 | SLSI_SCAN_HW_ID, false);
		slsi_scan_complete(sdev, dev, SLSI_SCAN_HW_ID, false, false);
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
}

void slsi_abort_scan(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);

	SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	slsi_abort_hw_scan(sdev, dev);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
}

int slsi_scan(struct wiphy                 *wiphy,
	      struct cfg80211_scan_request *request)
{
	struct net_device         *dev = NULL;
	struct netdev_vif         *ndev_vif = NULL;
	struct netdev_vif         *ndev_vif_p2p = NULL;
	struct slsi_dev           *sdev = NULL;
	int                       r = 0, i = 0;
	struct slsi_scan_params params = {
		.scan_type = FAPI_SCANTYPE_FULL_SCAN, .chan_count = 0,
		.strip_wsc = false, .strip_p2p = false
	};

	if (!request->wdev || !request->wdev->netdev) {
		SLSI_ERR_NODEV("wdev or net_dev NULL\n");
		return -EINVAL;
	}

	dev = request->wdev->netdev;
	ndev_vif = netdev_priv(dev);
	sdev = SDEV_FROM_WIPHY(wiphy);

	if (slsi_is_test_mode_enabled()) {
		SLSI_NET_WARN(dev, "not supported in WlanLite mode\n");
		return -EOPNOTSUPP;
	}

	SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
	/* Reject scan request if Group Formation is in progress
	 * The following condition means that it received GON Resonse (w/ status=1).
	 *   : sdev->p2p_state != P2P_IDLE_NO_VIF && ndev_vif_p2p && ndev_vif_p2p->drv_in_p2p_procedure == true
	 * So, probably, GON Req will be coming soon. Just allow to keep (iterate) ROC to receive it.
	 */
	ndev_vif_p2p = netdev_priv(sdev->netdev[SLSI_NET_INDEX_P2P]);
	if (sdev->p2p_state == P2P_ACTION_FRAME_TX_RX ||
	    (sdev->p2p_state != P2P_IDLE_NO_VIF && ndev_vif_p2p && ndev_vif_p2p->drv_in_p2p_procedure)) {
		SLSI_NET_INFO(dev, "Scan received in P2P Action Frame Tx/Rx state or drv_in_p2p_procedure=%d, Reject\n",
			      (int)ndev_vif_p2p->drv_in_p2p_procedure);
		SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
		return -EBUSY;
	}

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);

	if (ndev_vif->scan[SLSI_SCAN_HW_ID].scan_req) {
		SLSI_NET_INFO(dev, "Rejecting scan request as last scan is still running\n");
		r = -EBUSY;
		goto exit;
	}

#if defined(CONFIG_SLSI_WLAN_STA_FWD_BEACON) && (defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION >= 10)
	slsi_set_sta_forward_beacon(sdev, dev);
#endif
	SLSI_NET_DBG3(dev, SLSI_CFG80211, "channels:%d, ssids:%d, ie_len:%d, ifnum:%d, vif_type:%d\n",
		      request->n_channels, request->n_ssids, (int)request->ie_len, ndev_vif->ifnum, ndev_vif->iftype);

	slsi_p2p_cancel_unset_channel(sdev, request);

	for (i = 0; i < request->n_channels; i++)
		params.channels[i] = request->channels[i];
	params.chan_count = request->n_channels;
	slsi_set_scan_type_param(sdev, request, &params);

	if (slsi_is_p2p_wsc_ie_strip_condition(dev, request, &params))
		slsi_scan_set_p2p_wps_strip_param(request->ie, request->ie_len, &params);
	r = slsi_alloc_scan_ie_param(dev, request->ie, request->ie_len, &params);
	if (r < 0)
		goto exit;

	/* Flush out any outstanding single scan timeout work */
	cancel_delayed_work(&ndev_vif->scan_timeout_work);

	ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan = false;
	slsi_purge_scan_results(ndev_vif, SLSI_SCAN_HW_ID);
#ifdef CONFIG_SCSC_WLAN_ENABLE_MAC_RANDOMISATION
	slsi_set_mac_randomization(sdev, request, &params);
#endif

	r = slsi_mlme_add_scan(sdev,
			       dev,
			       params.scan_type,
			       FAPI_REPORTMODE_REAL_TIME,
			       request->n_ssids,
			       request->ssids,
			       params.chan_count,
			       params.channels,
			       NULL,
			       params.scan_ie,
			       params.scan_ie_len,
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
			       request->n_6ghz_params,
			       request->scan_6ghz_params,
			       request->flags,
#endif
			       ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan);
	if (r == 0) {
		ndev_vif->scan[SLSI_SCAN_HW_ID].scan_req = request;

		/* if delayed work is already scheduled, queue delayed work fails. So set
		 * requeue_timeout_work flag to enqueue delayed work in the timeout handler
		 */
		if (queue_delayed_work(sdev->device_wq, &ndev_vif->scan_timeout_work,
				       msecs_to_jiffies(SLSI_FW_SCAN_DONE_TIMEOUT_MSEC)))
			ndev_vif->scan[SLSI_SCAN_HW_ID].requeue_timeout_work = false;
		else
			ndev_vif->scan[SLSI_SCAN_HW_ID].requeue_timeout_work = true;
		slsi_update_state_scan_in_device_role(sdev, dev, &params);
		slsi_scan_save_probe_req_ies(request, &params);
	} else {
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "add_scan error: %d\n", r);
		r = -EIO;
	}
	slsi_free_scan_ie_param(&params);
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
	SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
	return r;
}

int slsi_sched_scan_start(struct wiphy                       *wiphy,
			  struct net_device                  *dev,
			  struct cfg80211_sched_scan_request *request)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct netdev_vif *ndev_vif_p2p = NULL;
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	int               r;
	struct slsi_scan_params params;

	if (slsi_is_test_mode_enabled()) {
		SLSI_NET_INFO(dev, "Skip sending signal, WlanLite FW does not support MLME_ADD_SCAN.request\n");
		return -EOPNOTSUPP;
	}

	/* Allow sched_scan only on wlan0. For P2PCLI interface, sched_scan might get requested following a
	 * wlan0 scan and its results being shared to sibling interfaces. Reject sched_scan for other interfaces.
	 */
	if (!SLSI_IS_VIF_INDEX_WLAN(ndev_vif)) {
		SLSI_NET_INFO(dev, "Scheduled scan req received on ifnum %d - Reject\n", ndev_vif->ifnum);
		return -EINVAL;
	}

	SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
	/* Unlikely to get a schedule scan while Group formation is in progress.
	 * In case it is requested, it will be rejected.
	 */
	ndev_vif_p2p = netdev_priv(sdev->netdev[SLSI_NET_INDEX_P2P]);
	if (sdev->p2p_state == P2P_ACTION_FRAME_TX_RX ||
	    (sdev->p2p_state != P2P_IDLE_NO_VIF && ndev_vif_p2p && ndev_vif_p2p->drv_in_p2p_procedure)) {
		SLSI_NET_INFO(dev,
			      "Scheduled scan req received in P2P Action Frame Tx/Rx state or drv_in_p2p_procedure=%d, Reject\n",
			      (int)ndev_vif_p2p->drv_in_p2p_procedure);
		SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
		return -EBUSY;
	}

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);

	SLSI_NET_INFO(dev, "channels:%d, ssids:%d, ie_len:%d, ifnum:%d, scan_plans:%d\n", request->n_channels,
		      request->n_ssids, (int)request->ie_len, ndev_vif->ifnum, request->n_scan_plans);

	if (ndev_vif->scan[SLSI_SCAN_HW_ID].sched_req) {
		r = -EBUSY;
		goto exit;
	}

	memset(&params, 0, sizeof(struct slsi_scan_params));
	if (request->ie)
		slsi_scan_set_p2p_wps_strip_param(request->ie, request->ie_len, &params);
	r = slsi_alloc_scan_ie_param(dev, request->ie, request->ie_len, &params);
	if (r != 0)
		goto exit;

	slsi_purge_scan_results(ndev_vif, SLSI_SCAN_SCHED_ID);
	r = slsi_mlme_add_sched_scan(sdev, dev, request, params.scan_ie, params.scan_ie_len);
	ndev_vif->scan[SLSI_SCAN_SCHED_ID].sched_req = request;
	slsi_free_scan_ie_param(&params);

	if (r != 0) {
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "add_scan error: %d\n", r);
		ndev_vif->scan[SLSI_SCAN_SCHED_ID].sched_req = NULL;
		r = -EIO;
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
	SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
	return r;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
int slsi_sched_scan_stop(struct wiphy *wiphy, struct net_device *dev, u64 reqid)
{
#else
int slsi_sched_scan_stop(struct wiphy *wiphy, struct net_device *dev)
{
#endif
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	int               r = 0;

	SLSI_UNUSED_PARAMETER(reqid);
	SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);
	SLSI_NET_DBG1(dev, SLSI_CFG80211, "ifnum:%d\n", ndev_vif->ifnum);
	if (!ndev_vif->scan[SLSI_SCAN_SCHED_ID].sched_req) {
		SLSI_NET_DBG1(dev, SLSI_CFG80211, "No sched scan req\n");
		goto exit;
	}

	r = slsi_mlme_del_scan(sdev, dev, (ndev_vif->ifnum << 8 | SLSI_SCAN_SCHED_ID), false);

	ndev_vif->scan[SLSI_SCAN_SCHED_ID].sched_req = NULL;

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
	SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
	return r;
}

int slsi_update_connect_params(struct wiphy *wiphy,
			       struct net_device *dev,
			       struct cfg80211_connect_params *sme, u32 changed)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int r = 0;

	SLSI_NET_INFO(dev, "\n");

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	r = slsi_mlme_update_connect_params(sdev, dev, sme, changed);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return r;
}

#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
void slsi_save_connection_params(struct slsi_dev *sdev, struct net_device *dev,
				 struct cfg80211_connect_params *sme,
				 struct ieee80211_channel *channel, const u8 *bssid,
				 u32 action_frame_bmap, u32 action_frame_suspend_bmap)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	memset(&ndev_vif->sta.sme, 0, sizeof(struct cfg80211_connect_params));
	ndev_vif->sta.sme = *sme;

	if (sme->ie && sme->ie_len)
		ndev_vif->sta.sme.ie = slsi_mem_dup((u8 *)sme->ie, sme->ie_len);
	else
		ndev_vif->sta.sme.ie = NULL;

	if (sme->ssid && sme->ssid_len)
		ndev_vif->sta.sme.ssid = slsi_mem_dup((u8 *)sme->ssid, sme->ssid_len);
	else
		ndev_vif->sta.sme.ssid = NULL;

	if (sme->key && sme->key_len)
		ndev_vif->sta.sme.key = slsi_mem_dup((u8 *)sme->key, sme->key_len);
	else
		ndev_vif->sta.sme.key = NULL;

	ndev_vif->sta.action_frame_bmap = action_frame_bmap;
	ndev_vif->sta.action_frame_suspend_bmap = action_frame_suspend_bmap;
	ndev_vif->sta.connected_bssid = slsi_mem_dup((u8 *)bssid, ETH_ALEN);
	ndev_vif->sta.connected_ssid_len = (int)sme->ssid_len;
	ndev_vif->sta.connected_ssid = slsi_mem_dup((u8 *)sme->ssid, ndev_vif->sta.connected_ssid_len);
}
#endif

#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
void slsi_set_params_on_bss(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_connect_params *sme,
			    struct ieee80211_channel **channel, const u8 **bssid)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	ndev_vif->sta.akm_type = slsi_bss_connect_type_get(sdev, sme->ie, sme->ie_len, NULL);
	ndev_vif->sta.ssid_len = sme->ssid_len;
	memcpy(ndev_vif->sta.ssid, sme->ssid, sme->ssid_len);
	/* If bssid is not present, check if bssid hint is present, if even hint not present,
	 * select bssid in driver set connect_attempted to true.
	 */
	if (!sme->bssid) {
		ndev_vif->sta.drv_bss_selection = true;
		if (sme->bssid_hint) {
			*bssid = sme->bssid_hint;
			*channel = sme->channel_hint;
		}
	} else {
		ndev_vif->sta.drv_bss_selection = false;
	}
}
#endif

int slsi_check_wificonnect_to_connectedGo(struct slsi_dev *sdev, struct net_device *dev, const u8 *bssid)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct net_device *p2p_dev;
	struct netdev_vif *ndev_p2p_vif;

	if (bssid) {
		if (SLSI_IS_VIF_INDEX_WLAN(ndev_vif) && sdev->p2p_state == P2P_GROUP_FORMED_CLI) {
			p2p_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_P2PX_SWLAN);
			if (p2p_dev) {
				ndev_p2p_vif = netdev_priv(p2p_dev);
				if (ndev_p2p_vif->sta.sta_bss) {
					if (SLSI_ETHER_EQUAL(ndev_p2p_vif->sta.sta_bss->bssid, bssid))
						return -EINVAL;
				}
			}
		}
	}
	return 0;
}

int slsi_set_roam_reassoc(struct net_device *dev, struct slsi_dev *sdev, struct cfg80211_connect_params *sme)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_peer  *peer;
	const u8          *connected_ssid = NULL;
	int		  r = 0;

	/* reassociation */
	peer = slsi_get_peer_from_qs(sdev, dev, SLSI_STA_PEER_QUEUESET);
	if (WLBT_WARN_ON(!peer))
		return -EINVAL;

	if (!sme->bssid && !sme->bssid_hint) {
		SLSI_NET_ERR(dev, "Require bssid in reassoc but received null\n");
		return -EINVAL;
	}
	if ((sme->bssid && !memcmp(peer->address, sme->bssid, ETH_ALEN)) ||
	     (sme->bssid_hint && !memcmp(peer->address, sme->bssid_hint, ETH_ALEN))) { /* same bssid or bssid hint*/
		r = slsi_mlme_reassociate(sdev, dev);
		if (r) {
			SLSI_NET_ERR(dev, "Failed to reassociate : %d\n", r);
		} else {
			ndev_vif->sta.vif_status = SLSI_VIF_STATUS_CONNECTING;
			slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_DISCONNECTED);
		}
	} else { /* different bssid */
		if (!ndev_vif->sta.sta_bss) {
			SLSI_NET_ERR(dev, "Bss is not stored in ndev_vif sta\n");
			return -EINVAL;
		}
		connected_ssid = cfg80211_find_ie(WLAN_EID_SSID, ndev_vif->sta.sta_bss->ies->data, ndev_vif->sta.sta_bss->ies->len);

		if (!connected_ssid) {
			SLSI_NET_ERR(dev, "Require ssid in roam but received null\n");
			return -EINVAL;
		}

		if (!memcmp(&connected_ssid[2], sme->ssid, connected_ssid[1])) { /* same ssid */
			if (!sme->channel) {
				SLSI_NET_ERR(dev, "Roaming has been rejected, as sme->channel is null\n");
				return -EINVAL;
			}
			if (sme->bssid) {
				r = slsi_mlme_roam(sdev, dev, sme->bssid, sme->channel->center_freq);
			} else if (sme->bssid_hint) {
				r = slsi_mlme_roam(sdev, dev, sme->bssid_hint, sme->channel->center_freq);
			} else  {
				SLSI_NET_ERR(dev, "Roaming has been rejected, as bssid and bssid_hint are null\n");
				return -EINVAL;
			}
			if (r) {
				SLSI_NET_ERR(dev, "Failed to roam : %d\n", r);
				return -EINVAL;
			}
		} else {
			SLSI_NET_ERR(dev, "Connected but received connect to new ESS, without disconnect");
			return -EINVAL;
		}
	}
	return r;
}

int slsi_check_valid_netdev_vif_state(struct slsi_dev *sdev, struct net_device *dev)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	/* Sta started case */
#ifdef CONFIG_SCSC_WLAN_WIFI_SHARING
	if (SLSI_IS_VIF_INDEX_MHS_DUALSTA(sdev, ndev_vif) && ndev_vif->iftype == NL80211_IFTYPE_P2P_CLIENT) {
		SLSI_NET_ERR(dev, "Iftype: %d\n", ndev_vif->iftype);
		return -EINVAL;
	}
#endif /* wifi sharing */
	/* Check netdev_vif is activated or not */
	if (WLBT_WARN_ON(ndev_vif->activated)) {
		SLSI_NET_ERR(dev, "Vif is already activated: %d\n", ndev_vif->activated);
		return -EINVAL;
	}
	if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION &&
	    ndev_vif->sta.vif_status != SLSI_VIF_STATUS_UNSPECIFIED) {
		SLSI_NET_ERR(dev, "VIF status: %d\n", ndev_vif->sta.vif_status);
		return -EINVAL;
	}
	return 0;
}

int slsi_set_bmap(struct slsi_dev *sdev, struct net_device *dev, u32 *action_frame_bmap, u32 *action_frame_suspend_bmap, u8 *device_address)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct net_device *p2p_dev;

	switch (ndev_vif->iftype) {
	case NL80211_IFTYPE_UNSPECIFIED:
	case NL80211_IFTYPE_STATION:
		ndev_vif->iftype = NL80211_IFTYPE_STATION;
		dev->ieee80211_ptr->iftype = NL80211_IFTYPE_STATION;
		*action_frame_bmap = SLSI_STA_ACTION_FRAME_BITMAP;
		*action_frame_suspend_bmap = SLSI_STA_ACTION_FRAME_SUSPEND_BITMAP;
#ifdef CONFIG_SCSC_WLAN_WES_NCHO
		if (sdev->device_config.wes_mode) {
			*action_frame_bmap |= SLSI_ACTION_FRAME_VENDOR_SPEC;
			*action_frame_suspend_bmap |= SLSI_ACTION_FRAME_VENDOR_SPEC;
		}
#endif
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
		slsi_p2p_group_start_remove_unsync_vif(sdev);
		p2p_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_P2P);
		if (p2p_dev)
			SLSI_ETHER_COPY(device_address, p2p_dev->dev_addr);
		*action_frame_bmap = SLSI_ACTION_FRAME_PUBLIC;
		*action_frame_suspend_bmap = SLSI_ACTION_FRAME_PUBLIC;
		break;
	default:
		SLSI_NET_ERR(dev, "Invalid Device Type: %d\n", ndev_vif->iftype);
		return -EINVAL;
	}
	return 0;
}

#ifdef CONFIG_SCSC_WLAN_EHT

static int slsi_calc_short_ssid(const struct cfg80211_bss_ies *ies,
				const struct element **elem, u32 *s_ssid)
{
	*elem = cfg80211_find_elem(WLAN_EID_SSID, ies->data, ies->len);
	if (!*elem || (*elem)->datalen > IEEE80211_MAX_SSID_LEN)
		return -EINVAL;

	*s_ssid = ~crc32_le(~0, (*elem)->data, (*elem)->datalen);
	return 0;
}

void slsi_scan_all_ml_link(struct net_device *dev, struct slsi_dev *sdev,
			   struct cfg80211_connect_params *sme, u16 capability)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	const struct cfg80211_bss_ies *beacon_ies;
	const u8 *rnr_ie = NULL, *ml_ie = NULL, *pos;
	u8 rnr_ie_len;
	const struct element *ssid_elem;
	size_t len;
	u32 s_ssid_tmp;
	int i, n_channels = 0, freq[MAX_NUM_MLD_LINKS];
	struct ieee80211_channel *scan_channel = NULL;
	struct cfg80211_ssid ssid;
	enum nl80211_band band;
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
	int index_id_6g = 0;
	struct cfg80211_scan_6ghz_params *scan_6ghz_params = NULL;
	u32 scan_flags;
#endif
	u8 mld_addr[ETH_ALEN] = {0};
	struct slsi_neighbor_ap_info {
		u8 tbtt_info_hdr;
		u8 tbtt_info_len;
		u8 op_class;
		u8 channel;
		u8 data[0];
	} __packed;

	if (!ndev_vif->sta.sta_bss)
		return;

	beacon_ies = ndev_vif->sta.sta_bss->ies;
	if (!beacon_ies)
		return;

	ml_ie = (u8 *)cfg80211_find_ext_elem(WLAN_EID_EXT_EHT_MULTI_LINK, beacon_ies->data, beacon_ies->len);
	if (!ml_ie)
		return;

	rnr_ie = (u8 *)cfg80211_find_ie(WLAN_EID_REDUCED_NEIGHBOR_REPORT, beacon_ies->data, beacon_ies->len);
	if (!rnr_ie)
		return;

	if (slsi_get_ap_mld_addr(sdev, (const struct element *)ml_ie, mld_addr) < 0) {
		SLSI_WARN(sdev, "MLD: Invalid MLD Address");
		return;
	}
	SLSI_ETHER_COPY(ndev_vif->sta.ap_mld_addr, mld_addr);
	if ((beacon_ies->len < rnr_ie - beacon_ies->data + 2) ||
	    (beacon_ies->len - (rnr_ie - beacon_ies->data + 2) < rnr_ie[1])) {
		SLSI_WARN(sdev, "MLD: Invalid RNR IE length");
		return;
	}

	/* Include scan on Associating channel */
	if (ndev_vif->sta.sta_bss->transmitted_bss) {
		freq[n_channels] = ndev_vif->sta.sta_bss->transmitted_bss->channel->center_freq;
		band = ndev_vif->sta.sta_bss->transmitted_bss->channel->band;
	} else {
		freq[n_channels] = ndev_vif->sta.sta_bss->channel->center_freq;
		band = ndev_vif->sta.sta_bss->channel->band;
	}
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
		if (!index_id_6g) {
			if (band == NL80211_BAND_6GHZ) {
				scan_channel = ieee80211_get_channel(sdev->wiphy, freq[n_channels]);
				scan_6ghz_params =
				kmalloc(sizeof(struct cfg80211_scan_6ghz_params), GFP_KERNEL);
				scan_6ghz_params->short_ssid = 0;
				scan_6ghz_params->channel_idx = 1;
				scan_6ghz_params->unsolicited_probe = true;
				scan_6ghz_params->short_ssid_valid = false;
				scan_6ghz_params->psc_no_listen = cfg80211_channel_is_psc(scan_channel);
				index_id_6g = 1;
				scan_flags = NL80211_SCAN_FLAG_COLOCATED_6GHZ;
			}
		}
#endif
	rnr_ie_len = rnr_ie[1];
	pos = rnr_ie + 2;
	n_channels++;
	slsi_calc_short_ssid(beacon_ies, &ssid_elem, &s_ssid_tmp);

	while (rnr_ie_len > sizeof(struct slsi_neighbor_ap_info)) {
		const struct slsi_neighbor_ap_info *ap_info = (const struct slsi_neighbor_ap_info *)pos;
		const u8 *data = ap_info->data;
		u8 link_bssid[ETH_ALEN];
		int i, count;
		u32 colocated_ap_short_ssid;

		count = u8_get_bits(ap_info->tbtt_info_hdr, IEEE80211_AP_INFO_TBTT_HDR_COUNT) + 1;
		len = ap_info->tbtt_info_len;
		if (!len) {
			SLSI_INFO(sdev, " Zero Length Err, rnr_ie len:%d rnrie:%p pos:%p\n", rnr_ie[1], rnr_ie, pos);
			SLSI_INFO_HEX(sdev, rnr_ie, rnr_ie[1], "rnr_ie Dump:\n");
		}
		if (len > rnr_ie_len || len == 0)
			break;

/* TBTT offset (1) + BSSID (6) + Short SSID (4)
 * + BSS parameter (2) + MLD Parameter (3) */

		if (ap_info->tbtt_info_len < 16) {
			rnr_ie_len -= (len * count);
			pos += (len * count);
			continue;
		}
		if (rnr_ie_len < count * len)
			break;
		pos = ap_info->data;

		for (i = 0; i < count; i++) {
			/* skip the TBTT offset */
			data = pos;
			data++;
			SLSI_ETHER_COPY(link_bssid, data);
			data += ETH_ALEN;
			colocated_ap_short_ssid = le32_to_cpu(*(u32 *)data);

			if (colocated_ap_short_ssid != s_ssid_tmp)
				goto continue_next_tbtt;

			data += 6;
			if (*data == 0xff)
				goto continue_next_tbtt;

			if (!ieee80211_operating_class_to_band(ap_info->op_class, &band)) {
				/* Kernel Version yet to include OP class 137 */
				if (ap_info->op_class == 137)
					band = NL80211_BAND_6GHZ;
				else
					goto continue_next_tbtt;
			}
			freq[n_channels] = ieee80211_channel_to_frequency(ap_info->channel, band);
			scan_channel = ieee80211_get_channel(sdev->wiphy, freq[n_channels]);

			if (!scan_channel) {
				SLSI_WARN(sdev, "MLD: Invalid channel %d in RNR");
				goto continue_next_tbtt;
			}
			if (scan_channel->flags & IEEE80211_CHAN_DISABLED)
				goto continue_next_tbtt;

			n_channels++;
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
			if (!index_id_6g) {
				if (band == NL80211_BAND_6GHZ) {
					scan_6ghz_params =
					kmalloc(sizeof(struct cfg80211_scan_6ghz_params), GFP_KERNEL);
					scan_6ghz_params->short_ssid = 0;
					scan_6ghz_params->channel_idx = 1;
					SLSI_ETHER_COPY(scan_6ghz_params->bssid, link_bssid);
					scan_6ghz_params->unsolicited_probe = true;
					scan_6ghz_params->short_ssid_valid = false;
					scan_6ghz_params->psc_no_listen = cfg80211_channel_is_psc(scan_channel);
					index_id_6g = 1;
					scan_flags = NL80211_SCAN_FLAG_COLOCATED_6GHZ;
				}

			}
#endif
continue_next_tbtt:
			rnr_ie_len -= len;
			pos += len;
		} /* for all neighnor ie */
	} /* while more neigh info */
	if (n_channels) {
		struct ieee80211_channel *add_scan_channel[MAX_NUM_MLD_LINKS];
		struct cfg80211_scan_info info = {.aborted = true};
		struct sk_buff *scan;

		ssid.ssid_len = sme->ssid_len;
		memcpy(ssid.ssid, sme->ssid, ssid.ssid_len);
		SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);

		if (ndev_vif->scan[SLSI_SCAN_HW_ID].scan_req) {
			SLSI_NET_DBG3(dev, SLSI_MLME, "stop on-going Scan\n");
			(void)slsi_mlme_del_scan(sdev, dev, ndev_vif->ifnum << 8 | SLSI_SCAN_HW_ID, false);
			cfg80211_scan_done(ndev_vif->scan[SLSI_SCAN_HW_ID].scan_req, &info);
			ndev_vif->scan[SLSI_SCAN_HW_ID].scan_req = NULL;
		}
		for (i = 0; i < n_channels; i++) {
			SLSI_INFO(sdev, "MLD: Multi link Scan on freq[%d] = %d\n", i, freq[i]);
			add_scan_channel[i] = ieee80211_get_channel(sdev->wiphy, freq[i]);
		}
		ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan = true;
		slsi_mlme_add_scan_mld_addr(sdev,
					    dev,
					    FAPI_SCANTYPE_FULL_SCAN,
					    FAPI_REPORTMODE_REAL_TIME,
					    1,
					    &ssid,
					    n_channels,
					    add_scan_channel,
					    NULL,
					    NULL,                   /* ie */
					    0,                      /* ie_len */
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
					    index_id_6g,
					    scan_6ghz_params,
					    scan_flags,
#endif
					    ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan,
					    mld_addr);
		SLSI_MUTEX_LOCK(ndev_vif->scan_result_mutex);
		scan = slsi_dequeue_cached_scan_result(&ndev_vif->scan[SLSI_SCAN_HW_ID], NULL);
		while (scan) {
			slsi_rx_scan_pass_to_cfg80211(sdev, dev, scan, true);
			scan = slsi_dequeue_cached_scan_result(&ndev_vif->scan[SLSI_SCAN_HW_ID], NULL);
		}
		SLSI_MUTEX_UNLOCK(ndev_vif->scan_result_mutex);

		ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan = false;
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
		kfree(scan_6ghz_params);
#endif
		SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
	}
}
#endif

int slsi_set_sta_bss_info(struct wiphy *wiphy, struct net_device *dev, struct slsi_dev *sdev,
			  struct cfg80211_connect_params *sme, struct ieee80211_channel **channel,
			  const u8 **bssid, u16 prev_vif_type)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u16		  capability = WLAN_CAPABILITY_ESS;
	int		  r = 0;

	capability = sme->privacy ? IEEE80211_PRIVACY_ON : IEEE80211_PRIVACY_OFF;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 17))
	ndev_vif->sta.sta_bss = __cfg80211_get_bss(wiphy,
						   *channel,
						   *bssid,
						   sme->ssid,
						   sme->ssid_len,
						   IEEE80211_BSS_TYPE_ANY,
						   capability,
						   NL80211_BSS_USE_FOR_NORMAL);
#else
	ndev_vif->sta.sta_bss = cfg80211_get_bss(wiphy,
						 *channel,
						 *bssid,
						 sme->ssid,
						 sme->ssid_len,
						 IEEE80211_BSS_TYPE_ANY,
						 capability);
#endif
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
	if (!sme->bssid && !sme->bssid_hint && !ndev_vif->sta.sta_bss) {
		struct slsi_ssid_info *ssid_info;

		list_for_each_entry(ssid_info, &ndev_vif->sta.ssid_info, list) {
			struct slsi_bssid_info *bssid_info;

			if (ssid_info->ssid.ssid_len != ndev_vif->sta.ssid_len ||
			    memcmp(ssid_info->ssid.ssid, &ndev_vif->sta.ssid, ndev_vif->sta.ssid_len) != 0 ||
			    !(ssid_info->akm_type & ndev_vif->sta.akm_type))
				continue;
			list_for_each_entry(bssid_info, &ssid_info->bssid_list, list) {
				if (*bssid && !memcmp(bssid_info->bssid, *bssid, ETH_ALEN))
					continue;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 17))
				ndev_vif->sta.sta_bss = __cfg80211_get_bss(wiphy,
									   ieee80211_get_channel(sdev->wiphy,
											       (bssid_info->freq / 2)),
									   bssid_info->bssid,
									   sme->ssid,
									   sme->ssid_len,
									   IEEE80211_BSS_TYPE_ANY,
									   capability,
									   NL80211_BSS_USE_FOR_NORMAL);
#else
				ndev_vif->sta.sta_bss = cfg80211_get_bss(wiphy,
									 ieee80211_get_channel(sdev->wiphy,
											       (bssid_info->freq / 2)),
									 bssid_info->bssid,
									 sme->ssid,
									 sme->ssid_len,
									 IEEE80211_BSS_TYPE_ANY,
									 capability);
#endif
				if (ndev_vif->sta.sta_bss) {
					*bssid = bssid_info->bssid;
					*channel = ieee80211_get_channel(sdev->wiphy, (bssid_info->freq / 2));
					break;
				}
			}
		}
	}
#endif
	if (!ndev_vif->sta.sta_bss) {
		struct cfg80211_ssid ssid;

		SLSI_NET_DBG3(dev, SLSI_CFG80211, "BSS info is not available - Perform scan\n");
		ssid.ssid_len = sme->ssid_len;
		memcpy(ssid.ssid, sme->ssid, ssid.ssid_len);
		if (!(ssid.ssid_len > 0 && *channel)) {
			r = slsi_mlme_connect_scan(sdev, dev, 1, &ssid, *channel);
			if (r) {
				SLSI_NET_ERR(dev, "slsi_mlme_connect_scan failed\n");
				return r;
			}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 17))
			ndev_vif->sta.sta_bss = __cfg80211_get_bss(wiphy,
								   *channel,
								   *bssid,
								   sme->ssid,
								   sme->ssid_len,
								   IEEE80211_BSS_TYPE_ANY,
								   capability,
								   NL80211_BSS_USE_FOR_NORMAL);
#else
			ndev_vif->sta.sta_bss = cfg80211_get_bss(wiphy,
								 *channel,
								 *bssid,
								 sme->ssid,
								 sme->ssid_len,
								 IEEE80211_BSS_TYPE_ANY,
								 capability);
#endif
			if (!ndev_vif->sta.sta_bss) {
				if (*bssid)
					SLSI_NET_ERR(dev, "cfg80211_get_bss(%.*s, " MACSTR ") Not found\n",
						     (int)sme->ssid_len, sme->ssid, MAC2STR(*bssid));
				else
					SLSI_NET_ERR(dev, "cfg80211_get_bss(%.*s) Not found\n",
						     (int)sme->ssid_len, sme->ssid);
				/* Set previous status in case of failure */
				ndev_vif->vif_type = prev_vif_type;
				return -ENOENT;
			}
			*channel = ndev_vif->sta.sta_bss->channel;
			*bssid = ndev_vif->sta.sta_bss->bssid;
		}
	} else {
#ifdef CONFIG_SEC_FACTORY
		if (sdev->device_config.supported_band != SLSI_FREQ_BAND_AUTO) {
			int supported_band = sdev->device_config.supported_band;
			int bss_band = ndev_vif->sta.sta_bss->channel->band;

			SLSI_NET_DBG3(dev, SLSI_CFG80211, "sup_band %d, bss_band %d\n", supported_band, bss_band);
			if (supported_band == SLSI_FREQ_BAND_2GHZ && bss_band != NL80211_BAND_2GHZ)
				return -EPERM;
			if (supported_band == SLSI_FREQ_BAND_5GHZ && bss_band != NL80211_BAND_5GHZ)
				return -EPERM;
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
			if (supported_band == SLSI_FREQ_BAND_6GHZ && bss_band != NL80211_BAND_6GHZ)
				return -EPERM;
#endif
		}
#endif

		*channel = ndev_vif->sta.sta_bss->channel;
		*bssid = ndev_vif->sta.sta_bss->bssid;
	}
#ifdef CONFIG_SCSC_WLAN_EHT
	if (sdev->fw_sta_eht_supported)
		slsi_scan_all_ml_link(dev, sdev, sme, capability);
#endif
	return 0;
}

void slsi_config_rsn_ie(struct net_device *dev, struct cfg80211_connect_params *sme)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (SLSI_IS_VIF_INDEX_WLAN(ndev_vif) || ndev_vif->iftype == NL80211_IFTYPE_P2P_CLIENT) {
		const u8 *rsn;

		SLSI_NET_DBG1(dev, SLSI_CFG80211, "N AKM Suites: : %1d\n", sme->crypto.n_akm_suites);
		rsn = cfg80211_find_ie(WLAN_EID_RSN, sme->ie, sme->ie_len);

		if (rsn) {
			int pos;

			/* Calculate the position of AKM suite in RSNIE
			 * RSNIE TAG(1 byte) + length(1 byte) + version(2 byte) + Group cipher suite(4 bytes)
			 * pairwise suite count(2 byte) + pairwise suite count * 4 + AKM suite count(2 byte)
			 * pos is the array index not length
			 */
			pos = 7 + 2 + (rsn[8] * 4) + 2;
			ndev_vif->sta.crypto.akm_suites[0] = ((rsn[pos + 1] << 24) | (rsn[pos + 2] << 16) | (rsn[pos + 3] << 8) | (rsn[pos + 4]));
			if (slsi_is_wpa3_support(rsn, sme->ie_len - (rsn - sme->ie))) {
				ndev_vif->sta.crypto.wpa_versions = 3;
				ndev_vif->sta.use_set_pmksa = 1;
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
				ndev_vif->sta.wpa3_auth_state = SLSI_WPA3_PREAUTH;
#endif
			} else if (slsi_is_owe_support(rsn, sme->ie_len - (rsn - sme->ie))) {
				ndev_vif->sta.use_set_pmksa = 1;
			} else {
				ndev_vif->sta.crypto.wpa_versions = 0;
				ndev_vif->sta.use_set_pmksa = 0;
			}
			SLSI_NET_DBG1(dev, SLSI_CFG80211, "RSN IE: : %1d\n", ndev_vif->sta.crypto.akm_suites[0]);
			if ((rsn[pos + 1] == 0x00 && rsn[pos + 2] == 0x0f && rsn[pos + 3] == 0xac) &&
			    (rsn[pos + 4] == 0x0e || rsn[pos + 4] == 0x0f || rsn[pos + 4] == 0x10 ||
			    rsn[pos + 4] == 0x11) && sme->auth_type == NL80211_AUTHTYPE_FILS_SK) {
				SLSI_NET_DBG1(dev, SLSI_CFG80211, "Realm Len: : %d\n", sme->fils_erp_realm_len);
				ndev_vif->sta.use_set_pmksa = 1;
				if (ndev_vif->sta.fils_realm) {
					kfree(ndev_vif->sta.fils_realm);
					ndev_vif->sta.fils_realm_len = 0;
				}
				if (sme->fils_erp_realm_len) {
					ndev_vif->sta.fils_realm = kmalloc(sme->fils_erp_realm_len, GFP_KERNEL);
					if (ndev_vif->sta.fils_realm) {
						ndev_vif->sta.fils_realm_len = sme->fils_erp_realm_len;
						memcpy(ndev_vif->sta.fils_realm, sme->fils_erp_realm,
						       sme->fils_erp_realm_len);
					} else {
						SLSI_NET_ERR(dev, "Failed to Allocate Memory for realm\n");
					}
				}
			} else {
				ndev_vif->sta.fils_connection = false;
			}
		}
		if (rsn) {
			ndev_vif->sta.rsn_ie_len = rsn[1];
			kfree(ndev_vif->sta.rsn_ie);
			ndev_vif->sta.rsn_ie = NULL;
			/* Len+2 because RSN IE TAG and Length */
			ndev_vif->sta.rsn_ie = kmalloc(ndev_vif->sta.rsn_ie_len + 2, GFP_KERNEL);

			/* len+2 because RSNIE TAG and Length */
			if (ndev_vif->sta.rsn_ie)
				memcpy(ndev_vif->sta.rsn_ie, rsn, ndev_vif->sta.rsn_ie_len + 2);
		}
	}
}

#ifdef CONFIG_SCSC_WLAN_EHT
static int slsi_get_mib_max_link(struct slsi_dev *sdev, struct net_device *dev)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_mib_data mibrsp = { 0, NULL };
	struct slsi_mib_value *values = NULL;
	struct slsi_mib_get_entry get_values[] = {{SLSI_PSID_UNIFI_MULTILINK_NUMBER_OF_LINKS, { 0, 0 } } };
	int ret = 0;

	mibrsp.dataLength = 10;
	mibrsp.data = kmalloc(mibrsp.dataLength, GFP_KERNEL);
	if (!mibrsp.data) {
		SLSI_ERR(sdev, "Cannot kmalloc %d bytes\n", mibrsp.dataLength);
		return -ENOMEM;
	}
	values = slsi_read_mibs(sdev, NULL, get_values, 1, &mibrsp);
	if (!values) {
		SLSI_ERR(sdev, "Error in slsi_read_mibs for SLSI_PSID_UNIFI_MULTILINK_NUMBER_OF_LINKS\n");
		ret = -EAGAIN;
		goto exit;
	}
	if (values[0].type != SLSI_MIB_TYPE_UINT) {
		SLSI_ERR(sdev, "Invalid type (%d) for SLSI_PSID_UNIFI_MULTILINK_NUMBER_OF_LINKS\n", values[0].type);
		ret = -EINVAL;
		goto exit_with_values;
	}
	ndev_vif->sta.max_ml_link = values[0].u.uintValue;
exit_with_values:
	kfree(values);
exit:
	kfree(mibrsp.data);
	return ret;
}
#endif

int slsi_connect(struct wiphy *wiphy, struct net_device *dev,
		 struct cfg80211_connect_params *sme)
{
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif   *ndev_vif = netdev_priv(dev);
	u8                  device_address[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	int                 r = 0;
	struct slsi_peer    *peer;
	u16                 prev_vif_type;
	u32                 action_frame_bmap;
	u32                 action_frame_suspend_bmap;
	const u8            *bssid;
	struct ieee80211_channel *channel;
	u8                  peer_address[ETH_ALEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	u16                 center_freq = 0;
	const u8            *rsn;

	if (slsi_is_test_mode_enabled()) {
		SLSI_NET_INFO(dev, "Skip sending signal, WlanLite FW does not support MLME_CONNECT.request\n");
		return -EOPNOTSUPP;
	}

	SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
	if (sdev->device_state != SLSI_DEVICE_STATE_STARTED) {
		SLSI_WARN(sdev, "device not started yet (device_state:%d)\n", sdev->device_state);
		SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
		return -EINVAL;
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	channel = sme->channel;
	bssid = sme->bssid;
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
	slsi_set_params_on_bss(sdev, dev, sme, &channel, &bssid);
#endif
	/* check if ap is found in the blacklist.
	 * if present in the blacklist return failure
	 */
	r = slsi_is_bssid_in_blacklist(sdev, dev, (u8 *)bssid);
	if (r) {
		SLSI_NET_ERR(dev, "Blacklist bssid not allowed\n");
		goto exit_with_error;
	}

	if (ndev_vif->sta.sta_bss)
		SLSI_ETHER_COPY(peer_address, ndev_vif->sta.sta_bss->bssid);

	center_freq = channel ? channel->center_freq : 0;

	if (WLBT_WARN_ON(!sme->ssid) || WLBT_WARN_ON(sme->ssid_len > IEEE80211_MAX_SSID_LEN))
		goto exit_with_error;

	if (bssid)
		SLSI_NET_INFO(dev, "%.*s Freq=%d vifStatus=%d CurrBssid:" MACSTR " NewBssid:" MACSTR " auth_type: %d Qinfo:%d ieLen:%d\n",
			      (int)sme->ssid_len, sme->ssid, center_freq, ndev_vif->sta.vif_status,
			      MAC2STR(peer_address), MAC2STR(bssid), (int)sme->auth_type, sdev->device_config.qos_info, (int)sme->ie_len);
	else
		SLSI_NET_INFO(dev, "%.*s Freq=%d vifStatus=%d CurrBssid:" MACSTR " auth_type: %d Qinfo:%d ieLen:%d\n",
			      (int)sme->ssid_len, sme->ssid, center_freq, ndev_vif->sta.vif_status,
			      MAC2STR(peer_address), (int)sme->auth_type, sdev->device_config.qos_info, (int)sme->ie_len);

#if IS_ENABLED(CONFIG_SCSC_WIFILOGGER)
	SCSC_WLOG_PKTFATE_NEW_ASSOC();
	if (bssid) {
		SCSC_WLOG_DRIVER_EVENT(WLOG_NORMAL, WIFI_EVENT_ASSOCIATION_REQUESTED, 3,
				       WIFI_TAG_BSSID, ETH_ALEN, bssid,
				       WIFI_TAG_SSID, sme->ssid_len, sme->ssid,
				       WIFI_TAG_CHANNEL, sizeof(u16), &center_freq);
				       // ?? WIFI_TAG_VENDOR_SPECIFIC, sizeof(RSSE), RSSE);
	}
#endif
	if (SLSI_IS_HS2_UNSYNC_VIF(ndev_vif)) {
		slsi_wlan_unsync_vif_deactivate(sdev, dev, true);
	} else if (SLSI_IS_VIF_INDEX_P2P(ndev_vif)) {
		SLSI_NET_ERR(dev, "Connect requested on incorrect vif\n");
		goto exit_with_error;
	}

	r = slsi_check_wificonnect_to_connectedGo(sdev, dev, bssid);
	if (r != 0) {
		SLSI_NET_ERR(dev, "Connect Request Rejected\n");
		goto exit_with_error;
	}
	rsn = cfg80211_find_ie(WLAN_EID_RSN, sme->ie, sme->ie_len);

	if (rsn) {
		if (sme->auth_type == NL80211_AUTHTYPE_FILS_SK ||
		    sme->auth_type == NL80211_AUTHTYPE_FILS_SK_PFS ||
		    sme->auth_type == NL80211_AUTHTYPE_FILS_SK_PFS ||
		    slsi_is_fils_akm(rsn, sme->ie_len - (rsn - sme->ie))) {
				SLSI_NET_ERR(dev, "FILS Auth not supported. Connect Request Rejected\n");
				goto exit_with_error;
		}
	}

	/* Determine Connection Type Reassoc or Roaming by BSSID */
	if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION && ndev_vif->sta.vif_status == SLSI_VIF_STATUS_CONNECTED) {
		r = slsi_set_roam_reassoc(dev, sdev, sme);
		if (r == 0)
			goto exit;
		else
			goto exit_with_error;
	}

	r = slsi_check_valid_netdev_vif_state(sdev, dev);
	if (r != 0) {
		SLSI_NET_ERR(dev, "Invalid netdev vif state\n");
		goto exit_with_error;
	}
	/* Back up vif_type because setting each vif_type */
	prev_vif_type = ndev_vif->vif_type;

	r = slsi_set_bmap(sdev, dev, &action_frame_bmap, &action_frame_suspend_bmap, device_address);
	if (r != 0)
		goto exit_with_error;

	/* Initial Roaming checks done - assign vif type */
	ndev_vif->vif_type = FAPI_VIFTYPE_STATION;

#if (defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
	channel = sme->channel;
	bssid = sme->bssid;
#endif
#ifdef CONFIG_SCSC_WLAN_EHT
	r = slsi_get_mib_max_link(sdev, dev);
	if (r != 0)
		ndev_vif->sta.max_ml_link = 1;
	ndev_vif->sta.connecting_links = ndev_vif->sta.max_ml_link;
#endif
	r = slsi_set_sta_bss_info(wiphy, dev, sdev, sme, &channel, &bssid, prev_vif_type);
	if (r != 0)
		goto exit;

	ndev_vif->chan = channel;
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
	ndev_vif->sta.ssid_len = sme->ssid_len;
	memcpy(ndev_vif->sta.ssid, sme->ssid, sme->ssid_len);
#endif
	/* Always check the BSSID is not null during connection
	 * It will cause kernel panic if we access null BSSID.
	 */
	if (bssid)
		SLSI_ETHER_COPY(ndev_vif->sta.bssid, bssid);

	if (slsi_mlme_add_vif(sdev, dev, dev->dev_addr, device_address) != 0) {
		SLSI_NET_ERR(dev, "slsi_mlme_add_vif failed\n");
		goto exit_with_bss;
	}
	if (slsi_vif_activated(sdev, dev) != 0) {
		SLSI_NET_ERR(dev, "slsi_vif_activated failed\n");
		goto exit_with_vif;
	}
	if (slsi_mlme_register_action_frame(sdev, dev, action_frame_bmap, action_frame_suspend_bmap) != 0) {
		SLSI_NET_ERR(dev, "Action frame registration failed for bitmap value 0x%x 0x%x\n", action_frame_bmap, action_frame_suspend_bmap);
		goto exit_with_vif;
	}

	r = slsi_set_boost(sdev, dev);
	if (r != 0)
		SLSI_NET_ERR(dev, "Rssi Boost set failed: %d\n", r);

	/* add_info_elements with Probe Req IEs. Proceed even if confirm fails for add_info as it would
	 * still work if the fw pre-join scan does not include the vendor IEs
	 */
	if (ndev_vif->probe_req_ies) {
		if (ndev_vif->iftype == NL80211_IFTYPE_P2P_CLIENT) {
			if (sme->crypto.wpa_versions == 2)
				ndev_vif->delete_probe_req_ies = true; /* Stored Probe Req can be deleted at vif
									* deletion after WPA2 association
									*/
			else
				/* Retain stored Probe Req at vif deletion until WPA2 connection to allow Probe req */
				ndev_vif->delete_probe_req_ies = false;
		} else {
			ndev_vif->delete_probe_req_ies = true; /* Delete stored Probe Req at vif deletion for STA */
		}
	}
	r = slsi_add_probe_ies_request(sdev, dev);

	/* Sometimes netif stack takes more time to initialize and any packet
	 * sent to stack would be dropped. This behavior is random in nature,
	 * so start the netif stack before sending out the connect req, it shall
	 * give enough time to netstack to initialize.
	 */
	netif_dormant_off(dev);
	ndev_vif->sta.vif_status = SLSI_VIF_STATUS_CONNECTING;
	ndev_vif->sta.roam_on_disconnect = false;
	r = slsi_set_ext_cap(sdev, dev, sme->ie, sme->ie_len, slsi_extended_cap_mask);
	if (r != 0)
		SLSI_NET_ERR(dev, "Failed to set extended capability MIB: %d\n", r);

	slsi_config_rsn_ie(dev, sme);

	slsi_conn_log2us_connect_sta_info(sdev, dev);
	slsi_conn_log2us_connecting(sdev, dev, sme);

	r = slsi_mlme_connect(sdev, dev, sme, channel, bssid);
	if (r != 0) {
		ndev_vif->sta.is_wps = false;
		SLSI_NET_ERR(dev, "connect failed: %d\n", r);
		netif_dormant_on(dev);
		goto exit_with_vif;
	}
	if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION && ndev_vif->iftype == NL80211_IFTYPE_STATION &&
	    ndev_vif->sta.set_elna_after_connect)
		r = slsi_mlme_set_elnabypass_threshold(sdev, dev, ndev_vif->sta.store_elna_value);

	slsi_spinlock_lock(&ndev_vif->peer_lock);
	peer = slsi_peer_add(sdev, dev, (u8 *)bssid, SLSI_STA_PEER_QUEUESET + 1);
	ndev_vif->sta.resp_id = 0;
	if (!peer) {
		slsi_spinlock_unlock(&ndev_vif->peer_lock);
		goto exit_with_error;
	}
	slsi_spinlock_unlock(&ndev_vif->peer_lock);

#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
	if (ndev_vif->sta.drv_bss_selection) {
		slsi_save_connection_params(sdev, dev, sme, channel, bssid,
					    action_frame_bmap, action_frame_suspend_bmap);
		slsi_set_reset_connect_attempted_flag(sdev, dev, bssid);
		ndev_vif->sta.wpa3_sae_reconnection = false;
	}
#endif
	goto exit;

exit_with_vif:
	if (slsi_mlme_del_vif(sdev, dev) != 0)
		SLSI_NET_ERR(dev, "slsi_mlme_del_vif failed\n");
	slsi_vif_deactivated(sdev, dev);
exit_with_bss:
	if (ndev_vif->sta.sta_bss) {
		slsi_cfg80211_put_bss(wiphy, ndev_vif->sta.sta_bss);
		ndev_vif->sta.sta_bss = NULL;
	}
exit_with_error:
	slsi_conn_log2us_connecting_fail(sdev, dev, bssid, center_freq, 3);
	r = -EINVAL;
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
	return r;
}

int slsi_disconnect(struct wiphy *wiphy, struct net_device *dev,
		    u16 reason_code)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_peer  *peer;
	int               r = 0;

	cancel_work_sync(&ndev_vif->set_multicast_filter_work);
	cancel_work_sync(&ndev_vif->update_pkt_filter_work);
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_DBG2(dev, SLSI_CFG80211, "reason: %d, ifnum = %d, vif_type = %d, vifnum =%d\n", reason_code,
		      ndev_vif->ifnum, ndev_vif->vif_type, ndev_vif->vifnum);

	/* Assuming that the time it takes the firmware to disconnect is not significant
	 * as this function holds the locks until the MLME-DISCONNECT-IND comes back.
	 * Unless the MLME-DISCONNECT-CFM fails.
	 */
	if (!ndev_vif->activated) {
		r = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0))
		cfg80211_disconnected(dev, reason_code, NULL, 0, false, GFP_KERNEL);
#else
		cfg80211_disconnected(dev, reason_code, NULL, 0, GFP_KERNEL);
#endif
		SLSI_NET_INFO(dev, "Vif is already Deactivated\n");
		goto exit;
	}

	peer = ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET];

#if IS_ENABLED(CONFIG_SCSC_WIFILOGGER)
	SCSC_WLOG_DRIVER_EVENT(WLOG_NORMAL, WIFI_EVENT_DISASSOCIATION_REQUESTED, 2,
			       WIFI_TAG_BSSID, ETH_ALEN, peer->address,
			       WIFI_TAG_REASON_CODE, sizeof(u16), &reason_code);
#endif

	switch (ndev_vif->vif_type) {
	case FAPI_VIFTYPE_STATION:
	{
#ifdef CONFIG_SCSC_WLAN_EHT
		u16 mlo_vif = 0;
#endif

		slsi_reset_throughput_stats(dev);
		/* Disconnecting spans several host firmware interactions so track the status
		 * so that the Host can ignore connect related signaling eg. MLME-CONNECT-IND
		 * now that it has triggered a disconnect.
		 */
		ndev_vif->sta.vif_status = SLSI_VIF_STATUS_DISCONNECTING;

		netif_dormant_on(dev);
		if (peer->valid)
			slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_DISCONNECTED);

		/* MLME-DISCONNECT_CFM only means that the firmware has accepted the request it has not yet
		 * disconnected. Completion of the disconnect is indicated by MLME-DISCONNECT-IND, so have
		 * to wait for that before deleting the VIF. Also any new activities eg. connect can not yet
		 * be started on the VIF until the disconnection is completed. So the MLME function also handles
		 * waiting for the MLME-DISCONNECT-IND (if the CFM is successful)
		 */

#ifdef CONFIG_SCSC_WLAN_EHT
		r = slsi_mlme_disconnect(sdev, dev, peer->address,  reason_code, true, &mlo_vif);
		if (r != 0)
			SLSI_NET_ERR(dev, "Disconnection returned with failure\n");
		/* Even if we fail to disconnect cleanly, tidy up. */
		r = slsi_handle_disconnect(sdev, dev, peer->address, 0, NULL, 0, mlo_vif);
#else
		r = slsi_mlme_disconnect(sdev, dev, peer->address,  reason_code, true);
		if (r != 0)
			SLSI_NET_ERR(dev, "Disconnection returned with failure\n");
		/* Even if we fail to disconnect cleanly, tidy up. */
		r = slsi_handle_disconnect(sdev, dev, peer->address, 0, NULL, 0);
#endif
		break;
	}
	default:
		SLSI_NET_WARN(dev, "Invalid - vif type:%d, device type:%d)\n", ndev_vif->vif_type, ndev_vif->iftype);
		r = -EINVAL;
		break;
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
int slsi_set_default_key(struct wiphy *wiphy, struct net_device *dev,
			 int link_id, u8 key_index, bool unicast, bool multicast)
#else
int slsi_set_default_key(struct wiphy *wiphy, struct net_device *dev,
			 u8 key_index, bool unicast, bool multicast)
#endif
{
	SLSI_UNUSED_PARAMETER(wiphy);
	SLSI_UNUSED_PARAMETER(dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	SLSI_UNUSED_PARAMETER(link_id);
#endif
	SLSI_UNUSED_PARAMETER(key_index);
	SLSI_UNUSED_PARAMETER(unicast);
	SLSI_UNUSED_PARAMETER(multicast);
	/* Key is set in add_key. Nothing to do here */
	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
int slsi_config_default_mgmt_key(struct wiphy      *wiphy,
				 struct net_device *dev, int link_id,
				 u8                key_index)
#else
int slsi_config_default_mgmt_key(struct wiphy      *wiphy,
				 struct net_device *dev,
				 u8                key_index)
#endif
{
	SLSI_UNUSED_PARAMETER(wiphy);
	SLSI_UNUSED_PARAMETER(dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	SLSI_UNUSED_PARAMETER(link_id);
#endif
	SLSI_UNUSED_PARAMETER(key_index);

	return 0;
}

int slsi_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	int             r = 0;

	SLSI_DBG1(sdev, SLSI_CFG80211, "slsi_set_wiphy_parms Frag Threshold = %d, RTS Threshold = %d",
		  wiphy->frag_threshold, wiphy->rts_threshold);

	if ((changed & WIPHY_PARAM_FRAG_THRESHOLD) && (wiphy->frag_threshold != -1)) {
		r = slsi_set_uint_mib(sdev, NULL, SLSI_PSID_DOT11_FRAGMENTATION_THRESHOLD, wiphy->frag_threshold);
		if (r != 0) {
			SLSI_ERR(sdev, "Setting FRAG_THRESHOLD failed\n");
			return r;
		}
	}

	if ((changed & WIPHY_PARAM_RTS_THRESHOLD) && (wiphy->rts_threshold != -1)) {
		r = slsi_set_uint_mib(sdev, NULL, SLSI_PSID_DOT11_RTS_THRESHOLD, wiphy->rts_threshold);
		if (r != 0) {
			SLSI_ERR(sdev, "Setting RTS_THRESHOLD failed\n");
			return r;
		}
	}

	return r;
}

int slsi_set_tx_power(struct wiphy *wiphy, struct wireless_dev *wdev,
		      enum nl80211_tx_power_setting type, int mbm)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	int             r = 0;

	SLSI_UNUSED_PARAMETER(wdev);
	SLSI_UNUSED_PARAMETER(type);
	SLSI_UNUSED_PARAMETER(mbm);
	SLSI_UNUSED_PARAMETER(sdev);

	r = -EOPNOTSUPP;

	return r;
}

static enum nl80211_chan_width slsi_bw_to_nl80211_bw(int width)
{
	switch (width) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
	case 320:
		return NL80211_CHAN_WIDTH_320;
#endif
	case 160:
		return NL80211_CHAN_WIDTH_160;
	case 80:
		return NL80211_CHAN_WIDTH_80;
	case 40:
		return NL80211_CHAN_WIDTH_40;
	default:
		return NL80211_CHAN_WIDTH_20;
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 41))
int slsi_get_channel(struct wiphy *wiphy, struct wireless_dev *wdev,
			unsigned int link_id, struct cfg80211_chan_def *chandef)
{
#else
int slsi_get_channel(struct wiphy *wiphy, struct wireless_dev *wdev,
			struct cfg80211_chan_def *chandef)
{
#endif
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int bss_freq = 0;
	int primary_freq = 0;
	int width = 0;
	int r = 0;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if ((ndev_vif->sta.vif_status != SLSI_VIF_STATUS_CONNECTED &&
	     ndev_vif->vif_type != FAPI_VIFTYPE_MONITOR) || !ndev_vif->chan) {
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -ENODATA;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 41) && defined(CONFIG_SCSC_WLAN_EHT))
	if ((link_id < MAX_NUM_MLD_LINKS) && (ndev_vif->sta.valid_links & BIT(link_id))) {
		bss_freq = ndev_vif->sta.links[link_id].cfreq1;
		width = ndev_vif->sta.links[link_id].width;
		primary_freq = ndev_vif->sta.links[link_id].freq;
		chandef->chan = ieee80211_get_channel(wiphy, primary_freq);
	} else {
#endif
	bss_freq = ndev_vif->sta.bss_cf;
	width = ndev_vif->sta.ch_width;
	primary_freq = ndev_vif->chan->center_freq;
	chandef->chan = ndev_vif->chan;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 41) && defined(CONFIG_SCSC_WLAN_EHT))
	}
#endif
	switch (width) {
	case 20:
		if (bss_freq != primary_freq) {
			r = -EINVAL;
			goto exit;
		}
		break;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
	case 320:
		if (bss_freq == primary_freq + 150 ||
		    bss_freq == primary_freq + 130 ||
		    bss_freq == primary_freq + 110 ||
		    bss_freq == primary_freq + 90 ||
		    bss_freq == primary_freq - 90 ||
		    bss_freq == primary_freq - 110 ||
		    bss_freq == primary_freq - 130 ||
		    bss_freq == primary_freq - 150)
			break;
		fallthrough;
#endif
	case 160:
		if (bss_freq == primary_freq + 70 ||
		    bss_freq == primary_freq + 50 ||
		    bss_freq == primary_freq - 50 ||
		    bss_freq == primary_freq - 70)
			break;
		fallthrough;
	case 80:
		if (bss_freq == primary_freq + 30 ||
		    bss_freq == primary_freq - 30)
			break;
		fallthrough;
	case 40:
		if (bss_freq == primary_freq + 10 ||
		    bss_freq == primary_freq - 10)
			break;
		fallthrough;
	default:
		r = -EINVAL;
		goto exit;
	}

	chandef->center_freq1 = bss_freq;
	chandef->width = slsi_bw_to_nl80211_bw(width);
	chandef->center_freq2 = 0;
exit:
	SLSI_NET_INFO(dev, "width:%dMHz, center_freq1:%dMHz, primary:%dMHz, r: %d\n",
		      width, bss_freq, primary_freq, r);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return r;
}

int slsi_get_tx_power(struct wiphy *wiphy, struct wireless_dev *wdev, int *dbm)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	int             r = 0;

	SLSI_UNUSED_PARAMETER(wdev);
	SLSI_UNUSED_PARAMETER(dbm);
	SLSI_UNUSED_PARAMETER(sdev);

	r = -EOPNOTSUPP;

	return r;
}

int slsi_del_station(struct wiphy *wiphy, struct net_device *dev,
		     struct station_del_parameters *del_params)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_peer  *peer;
	int               r = 0;
	u16               reason_code = WLAN_REASON_DEAUTH_LEAVING;
	const u8          *mac = del_params->mac;
	reason_code = del_params->reason_code;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!mac)
		SLSI_NET_DBG1(dev, SLSI_CFG80211,
			      "mac:null, vifType:%d, IfIndex:%d vifActivated:%d ap.p2p_gc_keys_set = %d\n",
			      ndev_vif->vif_type, ndev_vif->ifnum, ndev_vif->activated,
			      ndev_vif->ap.p2p_gc_keys_set);
	else
		SLSI_NET_DBG1(dev, SLSI_CFG80211,
			      "mac:" MACSTR ", vifType:%d, IfIndex:%d vifActivated:%d ap.p2p_gc_keys_set = %d\n",
			      MAC2STR(mac), ndev_vif->vif_type, ndev_vif->ifnum,
			      ndev_vif->activated, ndev_vif->ap.p2p_gc_keys_set);

	/* Function is called by cfg80211 before the VIF is added */
	if (!ndev_vif->activated)
		goto exit;

	if (FAPI_VIFTYPE_AP != ndev_vif->vif_type) {
		r = -EINVAL;
		goto exit;
	}
	/* MAC with NULL value will come in case of flushing VLANS . Ignore this.*/
	if (!mac)
		goto exit;
	else if (is_broadcast_ether_addr(mac) && reason_code == WLAN_REASON_DEAUTH_LEAVING) {
		int  i = 0;

		while (i < SLSI_PEER_INDEX_MAX) {
			peer = ndev_vif->peer_sta_record[i];
			if (peer && peer->valid) {
				slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_DISCONNECTED);
			}
			++i;
		}

		/* Note AP :: mlme_disconnect_request with broadcast mac address is
		 * not required. Other third party devices don't support this. Conclusively,
		 * BIP support is not present with AP
		 */

		/* Free WPA and WMM IEs if present */
		slsi_clear_cached_ies(&ndev_vif->ap.cache_wpa_ie, &ndev_vif->ap.wpa_ie_len);
		slsi_clear_cached_ies(&ndev_vif->ap.cache_wmm_ie, &ndev_vif->ap.wmm_ie_len);

		netif_dormant_on(dev);

		/* All STA related packets and info should already have been flushed */
		if (slsi_mlme_del_vif(sdev, dev) != 0)
			SLSI_NET_ERR(dev, "slsi_mlme_del_vif failed\n");
		slsi_vif_deactivated(sdev, dev);
		ndev_vif->ipaddress = cpu_to_be32(0);

		if (ndev_vif->ap.p2p_gc_keys_set) {
			slsi_wake_unlock(&sdev->wlan_wl);
			ndev_vif->ap.p2p_gc_keys_set = false;
		}
	} else {
#ifdef CONFIG_SCSC_WLAN_EHT
		u16 mlo_vif = 0;
#endif

		peer = slsi_get_peer_from_mac(sdev, dev, mac);
		if (peer) {  /* To handle race condition when disconnect_req is sent before procedure_strted_ind and before mlme-connected_ind*/
			if (peer->connected_state == SLSI_STA_CONN_STATE_CONNECTING) {
				SLSI_NET_DBG1(dev, SLSI_CFG80211, "SLSI_STA_CONN_STATE_CONNECTING : mlme-disconnect-req dropped at driver\n");
				goto exit;
			}
			if (peer->is_wps) {
				/* To inter-op with Intel STA in P2P cert need to discard the deauth after successful WPS handshake as a P2P GO */
				SLSI_NET_INFO(dev, "DISCONNECT after WPS : mlme-disconnect-req dropped at driver\n");
				goto exit;
			}
			slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_DISCONNECTED);
#ifdef CONFIG_SCSC_WLAN_EHT
			r = slsi_mlme_disconnect(sdev, dev, peer->address, reason_code, true, &mlo_vif);
			if (r != 0)
				SLSI_NET_ERR(dev, "Disconnection returned with failure\n");
			/* Even if we fail to disconnect cleanly, tidy up. */
			r = slsi_handle_disconnect(sdev, dev, peer->address, reason_code, NULL, 0, mlo_vif);
#else
			r = slsi_mlme_disconnect(sdev, dev, peer->address, reason_code, true);
			if (r != 0)
				SLSI_NET_ERR(dev, "Disconnection returned with failure\n");
			/* Even if we fail to disconnect cleanly, tidy up. */
			r = slsi_handle_disconnect(sdev, dev, peer->address, reason_code, NULL, 0);
#endif
		}
	}

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

#if defined(CONFIG_SCSC_WLAN_EHT)
static u16 slsi_get_vifidx(struct net_device *dev, const u8 *mac)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u16 vif_idx = ndev_vif->vifnum;
	u8 i = 0;

	for (i = 0; i < MAX_NUM_MLD_LINKS; i++) {
		if ((ndev_vif->sta.valid_links & BIT(i)) &&
		    (SLSI_ETHER_EQUAL(mac, ndev_vif->sta.links[i].bssid)) &&
		    (ndev_vif->sta.links[i].mlo_vif_idx)) {
			vif_idx = ndev_vif->sta.links[i].mlo_vif_idx;
			break;
		}
	}
	SLSI_DBG3(ndev_vif->sdev, SLSI_CFG80211, "vif_idx %d set\n", vif_idx);

	return vif_idx;
}
#else
static u16 slsi_get_vifidx(struct net_device *dev, const u8 *mac)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	SLSI_UNUSED_PARAMETER(mac);

	return ndev_vif->vifnum;
}
#endif

int slsi_get_station(struct wiphy *wiphy, struct net_device *dev,
		     const u8 *mac, struct station_info *sinfo)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_peer  *peer;
	int               r = 0;
	u16 vif_idx = 0;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!ndev_vif->activated) {
		r = -EINVAL;
		goto exit;
	}

#ifdef CONFIG_SCSC_WLAN_EHT
	peer = slsi_get_link_peer_from_mac(dev, mac);
	if (!peer) {
		SLSI_NET_DBG1(dev, SLSI_CFG80211, MACSTR " : Getting peer from MAC\n",
			      MAC2STR(mac));
		peer = slsi_get_peer_from_mac(sdev, dev, mac);
	}
#else
	peer = slsi_get_peer_from_mac(sdev, dev, mac);
#endif

	if (!peer) {
		SLSI_NET_DBG1(dev, SLSI_CFG80211, MACSTR " : Not Found\n", MAC2STR(mac));
		r = -EINVAL;
		goto exit;
	}

	vif_idx = slsi_get_vifidx(dev, mac);

	if (((ndev_vif->iftype == NL80211_IFTYPE_STATION && !(ndev_vif->sta.roam_in_progress)) ||
	     ndev_vif->iftype == NL80211_IFTYPE_P2P_CLIENT)) {
		/*Read MIB and fill into the peer.sinfo*/
		r = slsi_mlme_get_sinfo_mib(sdev, dev, peer, vif_idx);
		if (r) {
			SLSI_NET_DBG1(dev, SLSI_CFG80211, "failed to read Station Info Error:%d\n", r);
			goto exit;
		}
	}

	*sinfo = peer->sinfo;
	sinfo->generation = ndev_vif->cfg80211_sinfo_generation;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0))
	SLSI_NET_DBG1(dev, SLSI_CFG80211,
		      MACSTR ", tx:%d, txbytes:%llu, rx:%d, rxbytes:%llu tx_fail:%d tx_retry:%d tx_retry_times:%d\n",
#else
	SLSI_NET_DBG1(dev, SLSI_CFG80211,
		      MACSTR ", tx:%d, txbytes:%llu, rx:%d, rxbytes:%llu tx_fail:%d tx_retry:%d\n",
#endif
		      MAC2STR(mac),
		      peer->sinfo.tx_packets,
		      peer->sinfo.tx_bytes,
		      peer->sinfo.rx_packets,
		      peer->sinfo.rx_bytes,
		      peer->sinfo.tx_failed,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0))
		      peer->sinfo.tx_retries,
		      peer->sinfo.airtime_link_metric);
#else
		      peer->sinfo.tx_retries);
#endif

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

int slsi_set_power_mgmt(struct wiphy *wiphy, struct net_device *dev,
			bool enabled, int timeout)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int               r = -EINVAL;
	u16               pwr_mode = enabled ? FAPI_POWERMANAGEMENTMODE_POWER_SAVE : FAPI_POWERMANAGEMENTMODE_ACTIVE_MODE;

	SLSI_UNUSED_PARAMETER(timeout);
	if (slsi_is_test_mode_enabled()) {
		SLSI_NET_INFO(dev, "Skip sending signal, WlanLite FW does not support MLME_POWERMGT.request\n");
		return -EOPNOTSUPP;
	}

	if (slsi_is_rf_test_mode_enabled()) {
		SLSI_NET_INFO(dev, "Skip sending signal, RF test does not support.\n");
		return -EOPNOTSUPP;
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_INFO(dev, "PS MODE enabled:%d, vif_type:%d, ifnum:%d vifnum=%d\n", enabled, ndev_vif->vif_type,
		      ndev_vif->ifnum, ndev_vif->vifnum);

	if (ndev_vif->activated && ndev_vif->vif_type == FAPI_VIFTYPE_STATION) {
		ndev_vif->set_power_mode = pwr_mode;
		r = slsi_mlme_powermgt(sdev, dev, pwr_mode);
	} else {
		r = 0;
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

int slsi_set_qos_map(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_qos_map *qos_map)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_peer  *peer;
	int               r = 0;

	/* Cleaning up is inherently taken care by driver */
	if (!qos_map)
		return r;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!ndev_vif->activated) {
		r = -EINVAL;
		goto exit;
	}

	if (ndev_vif->vif_type != FAPI_VIFTYPE_STATION) {
		r = -EINVAL;
		goto exit;
	}

	SLSI_NET_DBG3(dev, SLSI_CFG80211, "Set QoS Map\n");
	peer = ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET];
	if (!peer || !peer->valid) {
		r = -EINVAL;
		goto exit;
	}

	memcpy(&peer->qos_map, qos_map, sizeof(struct cfg80211_qos_map));
	peer->qos_map_set = true;

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

int slsi_set_monitor_channel(struct wiphy *wiphy, struct cfg80211_chan_def *chandef)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	struct net_device *dev;
	struct netdev_vif *ndev_vif;

	SLSI_DBG1(sdev, SLSI_CFG80211, "channel (freq:%u MHz)\n", chandef->chan->center_freq);

	rcu_read_lock();
	dev = slsi_get_netdev_rcu(sdev, SLSI_NET_INDEX_WLAN);
	if (!dev) {
		SLSI_ERR(sdev, "netdev No longer exists\n");
		rcu_read_unlock();
		return -EINVAL;
	}
	ndev_vif = netdev_priv(dev);
	rcu_read_unlock();

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (monitor_vif_set == 2)
		slsi_monitor_set_if_with_vif(sdev, ndev_vif, SLSI_NET_INDEX_MONITOR2);
	else
		slsi_monitor_set_if_with_vif(sdev, ndev_vif, SLSI_NET_INDEX_MONITOR);
	if (slsi_mlme_configure_monitor_mode(sdev, dev, chandef) != 0) {
		SLSI_ERR(sdev, "set Monitor channel failed\n");
		slsi_monitor_set_if_with_vif(sdev, ndev_vif, SLSI_NET_INDEX_MONITOR);
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -EINVAL;
	}
	slsi_monitor_set_if_with_vif(sdev, ndev_vif, SLSI_NET_INDEX_MONITOR);
	if (monitor_vif_set == 1) {
		ndev_vif->chan = chandef->chan;
		ndev_vif->sta.bss_cf = chandef->center_freq1;
		ndev_vif->sta.ch_width = 0x00FF & slsi_get_chann_info(sdev, chandef);
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return 0;
}

int slsi_suspend(struct wiphy *wiphy, struct cfg80211_wowlan *wow)
{
	SLSI_UNUSED_PARAMETER(wow);

	return 0;
}

int slsi_resume(struct wiphy *wiphy)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);

	/* Scheduling the IO thread */
/*	(void)slsi_hip_run_bh(sdev); */
	SLSI_UNUSED_PARAMETER(sdev);

	return 0;
}

int slsi_set_pmksa(struct wiphy *wiphy, struct net_device *dev,
		   struct cfg80211_pmksa *pmksa)
{
	int i = 0;
	struct slsi_dev     *sdev = SDEV_FROM_WIPHY(wiphy);
	u8 *rsnie; /* final RSN IE with the PMKID*/
	u8 *buf; /* Complete Buffer: RSNIE and Assoc add info elements used in connection */
	u16 rsnie_len;
	int left = 0;
	u16 count = 0;
	int ret = 0;
	int pos = 0;
	int buf_len = 0;
	struct netdev_vif   *ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (pmksa->cache_id && pmksa->ssid_len) {
		memcpy(ndev_vif->sta.fils_cache_id, pmksa->cache_id, FILS_CACHE_ID_LEN);
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "Cache_id: 0x%x%x\n",
			      ndev_vif->sta.fils_cache_id[0], ndev_vif->sta.fils_cache_id[1]);
	}

	if (ndev_vif->sta.use_set_pmksa) {
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "\n");

		if (ndev_vif->sta.rsn_ie) {
			rsnie_len = ndev_vif->sta.rsn_ie_len;
			/* Tag and Len + Length of RSNIE + PMKID Count + PMKID */
			rsnie = kmalloc(2 + rsnie_len + 2 + PMKID_LEN, GFP_KERNEL);
			if (rsnie) {
				memset(rsnie, 0, rsnie_len + 2 + PMKID_LEN);
				/* parse the RSN IE and copy PMKID to Required position in RSN IE*/
				left = rsnie_len + 2; //FOR RSN IE TAG and Length
				pos = 4; /* RSN IE ID, LEN, and Version*/
				left -= 4;
				if (left < 4) {
					kfree(rsnie);
					kfree(ndev_vif->sta.rsn_ie);
					ndev_vif->sta.rsn_ie = NULL;
					ret = -EINVAL;
					goto exit;
				}
				pos += RSN_SELECTOR_LEN; /* Group cipher suite */
				left -= RSN_SELECTOR_LEN;

				/* Pairwise and AKM suite count and suite list */
				i = 0;
				while (i < 2) {
					pos += 2;
					left -= 2;
					count = le16_to_cpu(*(u16 *)(&ndev_vif->sta.rsn_ie[pos - 2]));
					pos += count * RSN_SELECTOR_LEN;
					left -= count * RSN_SELECTOR_LEN;
					i++;
				}
				pos += 2; /* RSN Capabilities */
				left -= 2;
				memcpy(rsnie, ndev_vif->sta.rsn_ie, pos);
				count = 0;
				if (left > 0 && left >= 2) {
					count = le16_to_cpu(*(u16 *)(&ndev_vif->sta.rsn_ie[pos]));  /* PMKID count */
					left -= 2;
				}
				pos += 2; /* PMKID count */
				rsnie[pos - 2] = 1;
				rsnie[pos - 1] = 0;

				memcpy(&rsnie[pos], pmksa->pmkid, PMKID_LEN); /* copy PMKID */
				pos += PMKID_LEN;
				if (count && left > 0) {
					left -= (count * PMKID_LEN);
					memcpy(&rsnie[pos], &ndev_vif->sta.rsn_ie[pos + ((count - 1) * PMKID_LEN)], left);
				} else if (left > 0) {
					memcpy(&rsnie[pos], &ndev_vif->sta.rsn_ie[pos - PMKID_LEN], left);
				}
				pos += left;
				rsnie_len = pos;
				rsnie[1] = rsnie_len - 2;
				buf_len = rsnie_len + ndev_vif->sta.assoc_req_add_info_elem_len;
				buf = kmalloc(buf_len, GFP_KERNEL);
				if (!buf) {
					SLSI_NET_ERR(dev, "Out of memory for buffer\n");
					ret = -ENOMEM;
					kfree(rsnie);
					goto exit;
				}
				memcpy(buf, rsnie, rsnie_len);
				if (ndev_vif->sta.assoc_req_add_info_elem_len)
					memcpy(buf + rsnie_len, ndev_vif->sta.assoc_req_add_info_elem, ndev_vif->sta.assoc_req_add_info_elem_len);
				ret = slsi_mlme_add_info_elements(sdev, dev, FAPI_PURPOSE_ASSOCIATION_REQUEST, buf, buf_len);
				if (ret != 0) {
					SLSI_NET_ERR(dev, "RSN IE with PMKID setting failed\n");
					kfree(rsnie);
					kfree(buf);
					goto exit;
				}
				kfree(buf);
				kfree(rsnie);
			} else {
				SLSI_NET_ERR(dev, "Out of memory for RSN IE\n");
				ret = -ENOMEM;
				goto exit;
			}
		} else {
			SLSI_NET_ERR(dev, "RSN IE is not present in Station VIF\n");
			ret = -EINVAL;
			goto exit;
		}
	}
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return ret;
}

int slsi_del_pmksa(struct wiphy *wiphy, struct net_device *dev,
		   struct cfg80211_pmksa *pmksa)
{
	struct netdev_vif   *ndev_vif = netdev_priv(dev);
	if (ndev_vif->sta.use_set_pmksa) {
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "\n");
		SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
		if (!ndev_vif->activated)
			SLSI_NET_DBG1(dev, SLSI_CFG80211, "VIF not activated\n");

		/* Just consume the DEL-PMKSA, because after DEL-PMKSA,
		 * ADD-PMKSA will override the PMKID
		 */
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	}
	return 0;
}

int slsi_flush_pmksa(struct wiphy *wiphy, struct net_device *dev)
{
	SLSI_UNUSED_PARAMETER(wiphy);
	SLSI_UNUSED_PARAMETER(dev);
	return 0;
}

int slsi_remain_on_channel(struct wiphy             *wiphy,
			   struct wireless_dev      *wdev,
			   struct ieee80211_channel *chan,
			   unsigned int             duration,
			   u64                      *cookie)
{
	struct net_device *dev = wdev->netdev;
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int               r = 0;

	SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
	if (sdev->device_state != SLSI_DEVICE_STATE_STARTED) {
		SLSI_WARN(sdev, "device not started yet (device_state:%d)\n", sdev->device_state);
		goto exit_with_error;
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_DBG2(dev, SLSI_CFG80211, "channel freq = %d, duration = %d, vif_type = %d, ifnum = %d,"
		      "sdev->p2p_state = %s\n", chan->center_freq, duration, ndev_vif->vif_type, ndev_vif->ifnum,
		      slsi_p2p_state_text(sdev->p2p_state));
	if (!SLSI_IS_VIF_INDEX_P2P(ndev_vif)) {
		SLSI_NET_ERR(dev, "Invalid vif type\n");
		goto exit_with_error;
	}

	if (SLSI_IS_P2P_GROUP_STATE(sdev)) {
		slsi_assign_cookie_id(cookie, &ndev_vif->unsync.roc_cookie);

		cfg80211_ready_on_channel(wdev, *cookie, chan, duration, GFP_KERNEL);
		cfg80211_remain_on_channel_expired(wdev, *cookie, chan, GFP_KERNEL);
		goto exit;
	}

	/* Unsync vif will be required, cancel any pending work of its deletion */
	cancel_delayed_work(&ndev_vif->unsync.del_vif_work);
	if (sdev->p2p_state == P2P_ACTION_FRAME_TX_RX)
		queue_delayed_work(sdev->device_wq, &ndev_vif->unsync.del_vif_work,
				   msecs_to_jiffies(duration + SLSI_P2P_UNSYNC_VIF_EXTRA_MSEC));

	/* Ideally, there should not be any ROC work pending. However, supplicant can send back to back ROC in a race scenario as below.
	 * If action frame is received while P2P social scan, the response frame tx is delayed till scan completes. After scan completion,
	 * frame tx is done and ROC is started. Upon frame tx status, supplicant sends another ROC without cancelling the previous one.
	 */
	cancel_delayed_work(&ndev_vif->unsync.roc_expiry_work);

	if (delayed_work_pending(&ndev_vif->unsync.unset_channel_expiry_work))
		cancel_delayed_work(&ndev_vif->unsync.unset_channel_expiry_work);

	/* If action frame tx is in progress and ROC comes, then it would mean action frame tx was done in ROC and
	 * frame tx ind is awaited, don't change state. Also allow back to back ROC in case it comes.
	 */
	if (sdev->p2p_state == P2P_ACTION_FRAME_TX_RX || sdev->p2p_state == P2P_LISTENING) {
		goto exit_with_roc;
	}

	/* Unsync vif activation: Possible P2P state at this point is P2P_IDLE_NO_VIF or P2P_IDLE_VIF_ACTIVE */
	if (sdev->p2p_state == P2P_IDLE_NO_VIF) {
		if (slsi_p2p_vif_activate(sdev, dev, chan, duration, true) != 0)
			goto exit_with_error;
	} else if (sdev->p2p_state == P2P_IDLE_VIF_ACTIVE) {
		/* Configure Probe Response IEs in firmware if they have changed */
		if (ndev_vif->unsync.ies_changed) {
			u16 purpose = FAPI_PURPOSE_PROBE_RESPONSE;

			if (slsi_mlme_add_info_elements(sdev, dev, purpose, ndev_vif->unsync.probe_rsp_ies, ndev_vif->unsync.probe_rsp_ies_len) != 0) {
				SLSI_NET_ERR(dev, "Probe Rsp IEs setting failed\n");
				goto exit_with_vif;
			}
			ndev_vif->unsync.ies_changed = false;
		}
		/* Channel Setting - Don't set if already on same channel */
		if (ndev_vif->driver_channel != chan->hw_value) {
			if (slsi_mlme_set_channel(sdev, dev, chan, SLSI_FW_CHANNEL_DURATION_UNSPECIFIED, 0, 0) != 0) {
				SLSI_NET_ERR(dev, "Channel setting failed\n");
				goto exit_with_vif;
			} else {
				ndev_vif->chan = chan;
				ndev_vif->driver_channel = chan->hw_value;
			}
		}
	} else {
		SLSI_NET_ERR(dev, "Driver in incorrect P2P state (%s)\n", slsi_p2p_state_text(sdev->p2p_state));
		goto exit_with_error;
	}

	SLSI_P2P_STATE_CHANGE(sdev, P2P_LISTENING);

exit_with_roc:
	/* Cancel remain on channel is sent to the supplicant 10ms before the duration
	 *This is to avoid the race condition of supplicant sending cancel remain on channel and
	 *drv sending cancel_remain on channel because of roc expiry.
	 *This race condition causes delay to the next p2p search
	 */
	queue_delayed_work(sdev->device_wq, &ndev_vif->unsync.roc_expiry_work,
			   msecs_to_jiffies(duration - SLSI_P2P_ROC_EXTRA_MSEC));

	slsi_assign_cookie_id(cookie, &ndev_vif->unsync.roc_cookie);
	SLSI_NET_DBG2(dev, SLSI_CFG80211, "Cookie = 0x%llx\n", *cookie);

	cfg80211_ready_on_channel(wdev, *cookie, chan, duration, GFP_KERNEL);
	goto exit;

exit_with_vif:
	slsi_p2p_vif_deactivate(sdev, dev, true);
exit_with_error:
	r = -EINVAL;
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
	return r;
}

int slsi_cancel_remain_on_channel(struct wiphy        *wiphy,
				  struct wireless_dev *wdev,
				  u64                 cookie)
{
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	int               r = 0;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_DBG2(dev, SLSI_CFG80211, "Cookie = 0x%llx, vif_type = %d, if_index = %d, sdev->p2p_state = %s,"
		      "ndev_vif->ap.p2p_gc_keys_set = %d, ndev_vif->unsync.roc_cookie = 0x%llx\n", cookie,
		      ndev_vif->vif_type, ndev_vif->ifnum, slsi_p2p_state_text(sdev->p2p_state),
		      ndev_vif->ap.p2p_gc_keys_set, ndev_vif->unsync.roc_cookie);

	if (!SLSI_IS_VIF_INDEX_P2P(ndev_vif)) {
		SLSI_NET_ERR(dev, "Invalid vif type\n");
		r = -EINVAL;
		goto exit;
	}

	if (!((sdev->p2p_state == P2P_LISTENING) || (sdev->p2p_state == P2P_ACTION_FRAME_TX_RX))) {
		goto exit;
	}

	if (sdev->p2p_state == P2P_ACTION_FRAME_TX_RX && ndev_vif->mgmt_tx_data.exp_frame != SLSI_PA_INVALID) {
		/* Reset the expected action frame as procedure got completed */
		SLSI_INFO(sdev, "Action frame (%s) was not received\n", slsi_pa_subtype_text(ndev_vif->mgmt_tx_data.exp_frame));
		ndev_vif->mgmt_tx_data.exp_frame = SLSI_PA_INVALID;
	}

	cancel_delayed_work(&ndev_vif->unsync.roc_expiry_work);
	cfg80211_remain_on_channel_expired(&ndev_vif->wdev, ndev_vif->unsync.roc_cookie, ndev_vif->chan, GFP_KERNEL);

	if (!ndev_vif->drv_in_p2p_procedure) {
		if (delayed_work_pending(&ndev_vif->unsync.unset_channel_expiry_work))
			cancel_delayed_work(&ndev_vif->unsync.unset_channel_expiry_work);
		queue_delayed_work(sdev->device_wq, &ndev_vif->unsync.unset_channel_expiry_work,
				   msecs_to_jiffies(SLSI_P2P_UNSET_CHANNEL_EXTRA_MSEC));
	}
	/* Queue work to delete unsync vif */
	slsi_p2p_queue_unsync_vif_del_work(ndev_vif, SLSI_P2P_UNSYNC_VIF_EXTRA_MSEC);
	SLSI_P2P_STATE_CHANGE(sdev, P2P_IDLE_VIF_ACTIVE);
	ndev_vif->drv_in_p2p_procedure = false;

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

int slsi_change_bss(struct wiphy *wiphy, struct net_device *dev,
		    struct bss_parameters *params)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int               r = 0;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_INFO(dev, "ifnum=%d, iftype=%d, current ap_isolate=%d, params.ap_isolate=%d\n",
		      ndev_vif->ifnum, ndev_vif->iftype, ndev_vif->ap.ap_isolate, params->ap_isolate);

	if (ndev_vif->iftype != NL80211_IFTYPE_AP) {
		SLSI_NET_ERR(dev, "Invalid vif type\n");
		r = -EINVAL;
		goto exit;
	}

	if (params->ap_isolate < -1 || params->ap_isolate > 1) {
		SLSI_NET_ERR(dev, "Invalid param (ap_isolate should be one of -1, 0  or 1)\n");
		r = -EINVAL;
		goto exit;
	}

	/* -1 means 'do not change' */
	if (params->ap_isolate != -1)
		ndev_vif->ap.ap_isolate = params->ap_isolate;

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

static void slsi_ap_start_obss_scan(struct slsi_dev *sdev, struct net_device *dev, struct netdev_vif *ndev_vif)
{
	struct cfg80211_ssid     ssids;
	struct ieee80211_channel *channel;
	int                      n_ssids = 1, n_channels = 1, i;

	if (!ndev_vif->chan) {
		SLSI_NET_ERR(dev, "ndev_vif->chan is null\n");
		return;
	}

	SLSI_NET_DBG1(dev, SLSI_CFG80211, "channel %u\n", ndev_vif->chan->hw_value);

	SLSI_MUTEX_LOCK(ndev_vif->scan_mutex);

	ssids.ssid_len = 0;
	for (i = 0; i < IEEE80211_MAX_SSID_LEN; i++)
		ssids.ssid[i] = 0x00;   /* Broadcast SSID */

	channel = ieee80211_get_channel(sdev->wiphy, ndev_vif->chan->center_freq);

	ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan = true;
	(void)slsi_mlme_add_scan(sdev,
				 dev,
				 FAPI_SCANTYPE_OBSS_SCAN,
				 FAPI_REPORTMODE_REAL_TIME,
				 n_ssids,
				 &ssids,
				 n_channels,
				 &channel,
				 NULL,
				 NULL, /* No IEs */
				 0,
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
				 0,
				 NULL,
				 0,
#endif
				 ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan /* Wait for scan_done_ind */);

	slsi_ap_obss_scan_done_ind(dev, ndev_vif);
	ndev_vif->scan[SLSI_SCAN_HW_ID].is_blocking_scan = false;
	SLSI_MUTEX_UNLOCK(ndev_vif->scan_mutex);
}

static int slsi_ap_start_validate(struct net_device *dev, struct slsi_dev *sdev, struct cfg80211_ap_settings *settings)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (SLSI_IS_VIF_INDEX_P2P(ndev_vif)) {
		SLSI_NET_ERR(dev, "AP start requested on incorrect vif\n");
		goto exit_with_error;
	}

	if (!settings->ssid_len || !settings->ssid) {
		SLSI_NET_ERR(dev, "SSID not provided\n");
		goto exit_with_error;
	}

	if (!settings->beacon.head_len || !settings->beacon.head) {
		SLSI_NET_ERR(dev, "Beacon not provided\n");
		goto exit_with_error;
	}

	if (!settings->beacon_interval) {
		SLSI_NET_ERR(dev, "Beacon Interval not provided\n");
		goto exit_with_error;
	}

	ndev_vif->chandef = &settings->chandef;
	ndev_vif->chan = ndev_vif->chandef->chan;
	ndev_vif->chandef_saved = settings->chandef;

	if (WLBT_WARN_ON(!ndev_vif->chan))
		goto exit_with_error;

	if (WLBT_WARN_ON(ndev_vif->activated))
		goto exit_with_error;

	if (WLBT_WARN_ON((ndev_vif->iftype != NL80211_IFTYPE_AP) && (ndev_vif->iftype != NL80211_IFTYPE_P2P_GO)))
		goto exit_with_error;

	if (ndev_vif->chan->band == NL80211_BAND_2GHZ && !sdev->fw_SoftAp_2g_40mhz_enabled &&
	    ndev_vif->chandef->width == NL80211_CHAN_WIDTH_40) {
		SLSI_NET_ERR(dev, "Configuration error: 40 MHz on 2.4 GHz is not supported. Channel_no: %d Channel_width: %d\n", ndev_vif->chan->hw_value, slsi_get_chann_info(sdev, ndev_vif->chandef));
		goto exit_with_error;
	}

	return 0;

exit_with_error:
	return -EINVAL;
}

static int slsi_get_max_bw_mhz(struct slsi_dev *sdev, u16 prim_chan_cf)
{
	int i;
	struct ieee80211_regdomain *regd = sdev->device_config.domain_info.regdomain;

	if (!regd) {
		SLSI_WARN(sdev, "NO regdomain info\n");
		return 0;
	}

	for (i = 0; i < regd->n_reg_rules; i++) {
		if ((regd->reg_rules[i].freq_range.start_freq_khz / 1000 <= prim_chan_cf - 10) &&
		    (regd->reg_rules[i].freq_range.end_freq_khz / 1000 >= prim_chan_cf + 10)) {
			u32 start_freq = regd->reg_rules[i].freq_range.start_freq_khz;
			u32 end_freq = regd->reg_rules[i].freq_range.end_freq_khz;

			if (i + 1 < regd->n_reg_rules &&
			    end_freq  == regd->reg_rules[i + 1].freq_range.start_freq_khz &&
			    regd->reg_rules[i].flags & NL80211_RRF_AUTO_BW &&
			    regd->reg_rules[i + 1].flags & NL80211_RRF_AUTO_BW &&
			    !(regd->reg_rules[i].flags & NL80211_RRF_DFS) &&
			    !(regd->reg_rules[i + 1].flags & NL80211_RRF_DFS))
				return (regd->reg_rules[i + 1].freq_range.end_freq_khz - start_freq) / 1000;
			else if (i - 1 >= 0 &&
				 start_freq ==  regd->reg_rules[i - 1].freq_range.end_freq_khz &&
				 regd->reg_rules[i].flags & NL80211_RRF_AUTO_BW &&
				 regd->reg_rules[i - 1].flags & NL80211_RRF_AUTO_BW &&
				 !(regd->reg_rules[i].flags & NL80211_RRF_DFS) &&
				 !(regd->reg_rules[i - 1].flags & NL80211_RRF_DFS))
				return (end_freq - regd->reg_rules[i - 1].freq_range.start_freq_khz) / 1000;
			else
				return regd->reg_rules[i].freq_range.max_bandwidth_khz / 1000;
		}
	}

	SLSI_WARN(sdev, "Freq(%d) not found in regdomain\n", prim_chan_cf);
	return 0;
}

int slsi_configure_country_code(struct slsi_dev *sdev, struct cfg80211_ap_settings *settings)
{
	const u8 *country_ie = NULL;
	char alpha2[SLSI_COUNTRY_CODE_LEN];

	/* Reg domain changes */
	country_ie = cfg80211_find_ie(WLAN_EID_COUNTRY, settings->beacon.tail, settings->beacon.tail_len);
	if (country_ie) {
		country_ie += 2;
		memcpy(alpha2, country_ie, SLSI_COUNTRY_CODE_LEN);
		if (memcmp(sdev->device_config.domain_info.regdomain->alpha2, alpha2, SLSI_COUNTRY_CODE_LEN - 1) != 0) {
			if (slsi_set_country_update_regd(sdev, alpha2, SLSI_COUNTRY_CODE_LEN) < 0)
				return -EINVAL;
		}
	}
	return 0;
}

int slsi_ap_start_get_indoor_chan(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_ap_settings *settings, int skip_indoor_check_for_wifi_sharing, int *indoor_channel)
{
	struct wiphy *wiphy = sdev->wiphy;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct ieee80211_channel *channel = NULL;
	u32 chan_flags;
	u16 center_freq;

	if (!skip_indoor_check_for_wifi_sharing) {
		channel = ieee80211_get_channel(sdev->wiphy, settings->chandef.chan->center_freq);
		if (!channel) {
			SLSI_ERR(sdev, "Invalid frequency %d used to start AP. Channel not found\n",
				 settings->chandef.chan->center_freq);
			return -EINVAL;
		}

		if (ndev_vif->iftype != NL80211_IFTYPE_P2P_GO && (channel->flags & IEEE80211_CHAN_INDOOR_ONLY)) {
			int idx = settings->chandef.chan->band;
			int i;

			chan_flags = (IEEE80211_CHAN_INDOOR_ONLY | IEEE80211_CHAN_RADAR |
					  IEEE80211_CHAN_DISABLED | IEEE80211_CHAN_NO_IR);
			for (i = 0; i < wiphy->bands[idx]->n_channels; i++) {
				if (!(wiphy->bands[idx]->channels[i].flags & chan_flags)) {
					center_freq = wiphy->bands[idx]->channels[i].center_freq;
					settings->chandef.chan = ieee80211_get_channel(wiphy, center_freq);

					if (!settings->chandef.chan) {
						SLSI_NET_DBG2(dev, SLSI_MLME, "Invalid chan for frequency %d\n", center_freq);
						continue;
					}
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
					if (idx == NL80211_BAND_6GHZ &&
					    !cfg80211_channel_is_psc(settings->chandef.chan)) {
						SLSI_NET_DBG2(dev, SLSI_MLME, "Invalid non-PSC freq %d\n", center_freq);
						continue;
					}
#endif
					settings->chandef.center_freq1 = center_freq;
					settings->chandef.width = NL80211_CHAN_WIDTH_20;
					SLSI_DBG1(sdev, SLSI_CFG80211, "ap valid frequency:%d MHz,chan_flags:0x%x,width:%d\n",
						  center_freq, wiphy->bands[idx]->channels[i].flags,
						  settings->chandef.width);
					*indoor_channel = 1;
					break;
				}
			}
			if (*indoor_channel == 0) {
				SLSI_ERR(sdev, "No valid channel found to start the AP");
				return -EINVAL;
			}
		}
	}
	return 0;
}

int slsi_modify_ht_capa_ies(struct net_device *dev, struct slsi_dev *sdev, struct cfg80211_ap_settings *settings)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (ndev_vif->chandef->width <= NL80211_CHAN_WIDTH_20) {
		/* Enable LDPC, SGI20 and SGI40 for both SoftAP & P2PGO if firmware supports */
		if (cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, settings->beacon.tail, settings->beacon.tail_len)) {
			u8 enforce_ht_cap1 = sdev->fw_ht_cap[0] & (IEEE80211_HT_CAP_LDPC_CODING |
								  IEEE80211_HT_CAP_SGI_20);
			u8 enforce_ht_cap2 = sdev->fw_ht_cap[1] & (IEEE80211_HT_CAP_RX_STBC >> 8);

			slsi_modify_ies(dev, WLAN_EID_HT_CAPABILITY, (u8 *)settings->beacon.tail,
					settings->beacon.tail_len, 2, enforce_ht_cap1);
			slsi_modify_ies(dev, WLAN_EID_HT_CAPABILITY, (u8 *)settings->beacon.tail,
					settings->beacon.tail_len, 3, enforce_ht_cap2);
		}
	} else if (cfg80211_chandef_valid(ndev_vif->chandef)) {
		u8 *ht_operation_ie;
		u8 sec_chan_offset = 0;
		u8 ch;
		u8 bw_40_minus_channels[] = { 40, 48, 153, 161, 5, 6, 7, 8, 9, 10, 11 };

		ht_operation_ie = (u8 *)cfg80211_find_ie(WLAN_EID_HT_OPERATION, settings->beacon.tail,
							 settings->beacon.tail_len);
		if (!ht_operation_ie) {
			SLSI_NET_ERR(dev, "HT Operation IE is not passed by wpa_supplicant\n");
			return -EINVAL;
		}

		sec_chan_offset = IEEE80211_HT_PARAM_CHA_SEC_ABOVE;
		for (ch = 0; ch < ARRAY_SIZE(bw_40_minus_channels); ch++)
			if (bw_40_minus_channels[ch] == ndev_vif->chandef->chan->hw_value) {
				sec_chan_offset = IEEE80211_HT_PARAM_CHA_SEC_BELOW;
				break;
			}

		/* Change HT Information IE subset 1 */
		ht_operation_ie += 3;
		*(ht_operation_ie) |= sec_chan_offset;
		*(ht_operation_ie) |= IEEE80211_HT_PARAM_CHAN_WIDTH_ANY;

		/* For 80MHz, Enable HT Capabilities : Support 40MHz Channel Width, SGI20 and SGI40
		 * for AP (both softAp as well as P2P GO), if firmware supports.
		 */
		if (cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, settings->beacon.tail,
				     settings->beacon.tail_len)) {
			u8 enforce_ht_cap1 = sdev->fw_ht_cap[0] & (IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
								  IEEE80211_HT_CAP_SGI_20 |
								  IEEE80211_HT_CAP_SGI_40 |
								  IEEE80211_HT_CAP_LDPC_CODING);
			u8 enforce_ht_cap2 = sdev->fw_ht_cap[1] & (IEEE80211_HT_CAP_RX_STBC >> 8);

			slsi_modify_ies(dev, WLAN_EID_HT_CAPABILITY, (u8 *)settings->beacon.tail,
					settings->beacon.tail_len, 2, enforce_ht_cap1);
			slsi_modify_ies(dev, WLAN_EID_HT_CAPABILITY, (u8 *)settings->beacon.tail,
					settings->beacon.tail_len, 3, enforce_ht_cap2);
		}
	}
	return 0;
}

void slsi_set_ap_mode(struct netdev_vif *ndev_vif, struct cfg80211_ap_settings *settings, bool append_vht_ies)
{
	if (append_vht_ies) {
		ndev_vif->ap.mode = SLSI_80211_MODE_11AC;
	} else if (cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, settings->beacon.tail, settings->beacon.tail_len) &&
		   cfg80211_find_ie(WLAN_EID_HT_OPERATION, settings->beacon.tail, settings->beacon.tail_len)) {
		ndev_vif->ap.mode = SLSI_80211_MODE_11N;
	} else {
		const u8 *ie;

		ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, settings->beacon.tail, settings->beacon.tail_len);
		if (ie)
			ndev_vif->ap.mode = slsi_get_supported_mode(ie);
	}
}

void slsi_store_settings_for_recovery(struct cfg80211_ap_settings *settings, struct netdev_vif *ndev_vif)
{
	if (&ndev_vif->backup_settings == settings)
		return;
	kfree(ndev_vif->backup_settings.chandef.chan);
	kfree(ndev_vif->backup_settings.beacon.head);
	kfree(ndev_vif->backup_settings.beacon.tail);
	kfree(ndev_vif->backup_settings.beacon.beacon_ies);
	kfree(ndev_vif->backup_settings.beacon.proberesp_ies);
	kfree(ndev_vif->backup_settings.beacon.assocresp_ies);
	kfree(ndev_vif->backup_settings.beacon.probe_resp);
	kfree(ndev_vif->backup_settings.ssid);
	kfree(ndev_vif->backup_settings.ht_cap);
	kfree(ndev_vif->backup_settings.vht_cap);
	ndev_vif->backup_settings = *settings;
	ndev_vif->backup_settings.chandef.chan =
	(struct ieee80211_channel *)slsi_mem_dup((u8 *)settings->chandef.chan, sizeof(struct ieee80211_channel));
	ndev_vif->backup_settings.beacon.head = slsi_mem_dup((u8 *)settings->beacon.head, settings->beacon.head_len);
	ndev_vif->backup_settings.beacon.tail = slsi_mem_dup((u8 *)settings->beacon.tail, settings->beacon.tail_len);
	ndev_vif->backup_settings.beacon.beacon_ies = slsi_mem_dup((u8 *)settings->beacon.beacon_ies,
								   settings->beacon.beacon_ies_len);
	ndev_vif->backup_settings.beacon.proberesp_ies = slsi_mem_dup((u8 *)settings->beacon.proberesp_ies,
								      settings->beacon.proberesp_ies_len);
	ndev_vif->backup_settings.beacon.assocresp_ies = slsi_mem_dup((u8 *)settings->beacon.assocresp_ies,
								      settings->beacon.assocresp_ies_len);
	ndev_vif->backup_settings.beacon.probe_resp = slsi_mem_dup((u8 *)settings->beacon.probe_resp,
								   settings->beacon.probe_resp_len);
	ndev_vif->backup_settings.ssid = slsi_mem_dup((u8 *)settings->ssid, settings->ssid_len);
	if (settings->ht_cap) {
		ndev_vif->backup_settings.ht_cap =
		(struct ieee80211_ht_cap *)slsi_mem_dup((u8 *)settings->ht_cap, sizeof(struct ieee80211_ht_cap));
	}
	if (settings->vht_cap) {
		ndev_vif->backup_settings.vht_cap =
		(struct ieee80211_vht_cap *)slsi_mem_dup((u8 *)settings->vht_cap, sizeof(struct ieee80211_vht_cap));
	}
}

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
static bool slsi_is_320_mhz_supprtd(struct slsi_dev *sdev)
{
#ifdef CONFIG_SCSC_WLAN_EHT
	u8 * eht_cap = sdev->fw_mhs_eht_cap;

	return (eht_cap[2] & IEEE80211_EHT_PHY_CAP0_320MHZ_IN_6GHZ);
#else
	return false;
#endif
}
#endif
void slsi_update_6g_chandef(struct net_device *dev, struct slsi_dev *sdev)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u16 oper_chan = ndev_vif->chandef->chan->hw_value;
	int i;

	if (ndev_vif->chandef->chan->band != NL80211_BAND_6GHZ)
		return;

	if (!slsi_dev_6ghz_skip_acs())
		return;

	SLSI_NET_DBG1(dev, SLSI_MLME, "Channel: %d, freq : %d, bandwidth : %d\n",
		      oper_chan, ndev_vif->chandef->chan->center_freq, ndev_vif->chandef->width);

	if (oper_chan == 229) {
		if (sdev->allow_switch_40_mhz) {
			ndev_vif->chandef->width = NL80211_CHAN_WIDTH_40;
			ndev_vif->chandef->center_freq1 =
				ieee80211_channel_to_frequency(227, NL80211_BAND_6GHZ);
		} else {
			ndev_vif->chandef->width = NL80211_CHAN_WIDTH_20;
			ndev_vif->chandef->center_freq1 =
				ieee80211_channel_to_frequency(229, NL80211_BAND_6GHZ);
		}
		return;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
	if (slsi_is_320_mhz_supprtd(sdev)) {
		for (i = 0; i < SLSI_6GHZ_320MHz_MAX_CH_NUM; i++) {
			if (oper_chan == (SLSI_6GHZ_320MHz_IDX_2_CHAN(i) - 26) ||
			    oper_chan == (SLSI_6GHZ_320MHz_IDX_2_CHAN(i) + 6)) {
				ndev_vif->chandef->center_freq1 =
					ieee80211_channel_to_frequency(SLSI_6GHZ_320MHz_IDX_2_CHAN(i),
								       NL80211_BAND_6GHZ);
				ndev_vif->chandef->width = NL80211_CHAN_WIDTH_320;

				SLSI_NET_DBG1(dev, SLSI_MLME, "idx : %d, center_freq1 : %d, bandwidth: %d\n",
					      SLSI_6GHZ_320MHz_IDX_2_CHAN(i),
					      ndev_vif->chandef->center_freq1,
					      ndev_vif->chandef->width);
				break;
			}
		}
	} else if (sdev->supported_6g_160mhz && sdev->allow_switch_160_mhz) {
#else
	if (sdev->supported_6g_160mhz && sdev->allow_switch_160_mhz) {
#endif
		for (i = 0; i < SLSI_6GHZ_160MHz_MAX_CH_NUM; i++) {
			if (oper_chan == (SLSI_6GHZ_160MHz_IDX_2_CHAN(i) - 10) ||
			    oper_chan == (SLSI_6GHZ_160MHz_IDX_2_CHAN(i) + 6)) {
				ndev_vif->chandef->center_freq1 =
					ieee80211_channel_to_frequency(SLSI_6GHZ_160MHz_IDX_2_CHAN(i),
								       NL80211_BAND_6GHZ);
				ndev_vif->chandef->width = NL80211_CHAN_WIDTH_160;

				SLSI_NET_DBG1(dev, SLSI_MLME, "idx : %d, center_freq1 : %d, bandwidth: %d\n",
					      SLSI_6GHZ_160MHz_IDX_2_CHAN(i), ndev_vif->chandef->center_freq1,
					      ndev_vif->chandef->width);
				break;
			}
		}
	} else if (sdev->allow_switch_80_mhz) {
		for (i = 0; i < SLSI_6GHZ_40MHz_80MHz_MAX_CH_NUM; i++) {
			if (oper_chan == (SLSI_6GHZ_80MHz_IDX_2_CHAN(i) - 2)) {
				ndev_vif->chandef->center_freq1 =
					ieee80211_channel_to_frequency(SLSI_6GHZ_80MHz_IDX_2_CHAN(i),
								       NL80211_BAND_6GHZ);
				ndev_vif->chandef->width = NL80211_CHAN_WIDTH_80;

				SLSI_NET_DBG1(dev, SLSI_MLME, "idx : %d, center_freq1 : %d, bandwidth: %d\n",
					      SLSI_6GHZ_80MHz_IDX_2_CHAN(i), ndev_vif->chandef->center_freq1,
					      ndev_vif->chandef->width);
				break;
			}
		}
	} else if (sdev->allow_switch_40_mhz) {
		for (i = 0; i < SLSI_6GHZ_40MHz_80MHz_MAX_CH_NUM; i++) {
			if (oper_chan == (SLSI_6GHZ_40MHz_IDX_2_CHAN(i) + 2)) {
				ndev_vif->chandef->center_freq1 =
					ieee80211_channel_to_frequency(SLSI_6GHZ_40MHz_IDX_2_CHAN(i),
								       NL80211_BAND_6GHZ);
				ndev_vif->chandef->width = NL80211_CHAN_WIDTH_40;

				SLSI_NET_DBG1(dev, SLSI_MLME, "idx : %d, center_freq1 : %d, bandwidth: %d\n",
					      SLSI_6GHZ_40MHz_IDX_2_CHAN(i), ndev_vif->chandef->center_freq1,
					      ndev_vif->chandef->width);
				break;
			}
		}
	} else {
		ndev_vif->chandef->width = NL80211_CHAN_WIDTH_20;
		ndev_vif->chandef->center_freq1 =
					ieee80211_channel_to_frequency(oper_chan,
								       NL80211_BAND_6GHZ);
	}
}
#endif

static bool slsi_ap_chandef_vht_ht(struct net_device *dev, struct slsi_dev *sdev, int wifi_sharing_channel_switched)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	bool append_vht_ies = false;

	SLSI_NET_DBG1(dev, SLSI_MLME, "Channel: %d, Maximum bandwidth: %d, freq : %d\n",
		      ndev_vif->chandef->chan->hw_value, slsi_get_max_bw_mhz(sdev, ndev_vif->chandef->chan->center_freq),
		      ndev_vif->chandef->chan->center_freq);

	/* 11ac configuration (5GHz and VHT) */
	if (ndev_vif->chandef->chan->hw_value >= 36 && ndev_vif->chandef->chan->hw_value <= 128 &&
	    sdev->fw_vht_enabled && sdev->allow_switch_160_mhz && slsi_is_160mhz_supported(sdev) &&
	    (slsi_get_max_bw_mhz(sdev, ndev_vif->chandef->chan->center_freq) >= 160)) {
		u16 oper_chan = ndev_vif->chandef->chan->hw_value;

		append_vht_ies = true;
		ndev_vif->chandef->width = NL80211_CHAN_WIDTH_160;
		if (oper_chan >= 36 && oper_chan <= 64)
			ndev_vif->chandef->center_freq1 = ieee80211_channel_to_frequency(50, NL80211_BAND_5GHZ);
		if (oper_chan >= 100 && oper_chan <= 128)
			ndev_vif->chandef->center_freq1 = ieee80211_channel_to_frequency(114,
											 NL80211_BAND_5GHZ);
	} else if (ndev_vif->chandef->chan->hw_value >= 36 && ndev_vif->chandef->chan->hw_value < 165 &&
		   sdev->fw_vht_enabled && sdev->allow_switch_80_mhz &&
		   (slsi_get_max_bw_mhz(sdev, ndev_vif->chandef->chan->center_freq) >= 80)) {
		u16 oper_chan = ndev_vif->chandef->chan->hw_value;

		append_vht_ies = true;
		ndev_vif->chandef->width = NL80211_CHAN_WIDTH_80;

		SLSI_NET_DBG1(dev, SLSI_MLME, "5 GHz- Include VHT\n");
		if (oper_chan >= 36 && oper_chan <= 48)
			ndev_vif->chandef->center_freq1 = ieee80211_channel_to_frequency(42, NL80211_BAND_5GHZ);
		else if (oper_chan >= 149 && oper_chan <= 161)
			ndev_vif->chandef->center_freq1 = ieee80211_channel_to_frequency(155, NL80211_BAND_5GHZ);

		/* In wifi sharing case, AP can start on STA channel even though it is DFS channel*/
		if (wifi_sharing_channel_switched == 1) {
			if (oper_chan >= 52 && oper_chan <= 64)
				ndev_vif->chandef->center_freq1 = ieee80211_channel_to_frequency(58,
												 NL80211_BAND_5GHZ);
			else if (oper_chan >= 100 && oper_chan <= 112)
				ndev_vif->chandef->center_freq1 = ieee80211_channel_to_frequency(106,
												 NL80211_BAND_5GHZ);
			else if (oper_chan >= 116 && oper_chan <= 128)
				ndev_vif->chandef->center_freq1 = ieee80211_channel_to_frequency(122,
												 NL80211_BAND_5GHZ);
			else if (oper_chan >= 132 && oper_chan <= 144)
				ndev_vif->chandef->center_freq1 = ieee80211_channel_to_frequency(138,
												 NL80211_BAND_5GHZ);
		}
	} else if (sdev->fw_ht_enabled && sdev->allow_switch_40_mhz &&
			   slsi_get_max_bw_mhz(sdev, ndev_vif->chandef->chan->center_freq) >= 40 &&
			   ((ndev_vif->chandef->chan->hw_value < 165 && ndev_vif->chandef->chan->hw_value >= 36)) &&
			   (sdev->fw_ht_cap[0] & IEEE80211_HT_CAP_SUP_WIDTH_20_40)) {
		/* HT40 configuration (5GHz/2GHz and HT) */
		u16  oper_chan = ndev_vif->chandef->chan->hw_value;
		u8   bw_40_minus_channels[] = { 40, 48, 153, 161, 5, 6, 7, 8, 9, 10, 11 };
		u8 bw_40_minus_dfs_channels[] = { 144, 136, 128, 120, 112, 104, 64, 56 };
		u8   ch;

		ndev_vif->chandef->width = NL80211_CHAN_WIDTH_40;
		ndev_vif->chandef->center_freq1 =  ndev_vif->chandef->chan->center_freq + 10;
		for (ch = 0; ch < ARRAY_SIZE(bw_40_minus_channels); ch++)
			if (oper_chan == bw_40_minus_channels[ch]) {
				ndev_vif->chandef->center_freq1 =  ndev_vif->chandef->chan->center_freq - 10;
				break;
			}

		if (wifi_sharing_channel_switched == 1) {
			for (ch = 0; ch < ARRAY_SIZE(bw_40_minus_dfs_channels); ch++)
				if (oper_chan == bw_40_minus_dfs_channels[ch]) {
					ndev_vif->chandef->center_freq1 =  ndev_vif->chandef->chan->center_freq - 10;
					break;
				}
		}
	}
	return append_vht_ies;
}

void slsi_stop_p2p_group_iface_on_ap_start(struct slsi_dev *sdev)
{
	struct net_device *p2p_dev = sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN];
	struct netdev_vif *ndev_p2p_vif;

	SLSI_NET_INFO(p2p_dev, "P2P group interface is not removed before starting AP\n");
	if (!p2p_dev)
		return;
	ndev_p2p_vif = netdev_priv(p2p_dev);
	SLSI_MUTEX_LOCK(ndev_p2p_vif->vif_mutex);
	slsi_vif_cleanup(sdev, p2p_dev, true, 0);
	SLSI_MUTEX_UNLOCK(ndev_p2p_vif->vif_mutex);
	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	rcu_assign_pointer(sdev->netdev_p2p, p2p_dev);
	if (sdev->netdev_ap)
		rcu_assign_pointer(sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN], sdev->netdev_ap);
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
}

static int slsi_modify_vendor_ies(struct slsi_dev *sdev, struct net_device *dev, struct cfg80211_ap_settings *settings,
				  const u8 *wpa_ie_pos, const u8 *wmm_ie_pos)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int wpa_ie_len = 0;
	int wmm_ie_len = 0;
	int r = 0;

	/* Extract the WMM and WPA IEs from settings->beacon.tail - This is sent in add_info_elements and
	 * shouldn't be included in start_req Cache IEs to be used in later add_info_elements_req.
	 * The IEs would be freed during AP stop
	 */
	if (wpa_ie_pos) {
		wpa_ie_len = *(wpa_ie_pos + 1) + 2;     /* For 0xdd (1) and Tag Length (1) */
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "WPA IE found: Length = %zu\n", wpa_ie_len);
		r = slsi_cache_ies(wpa_ie_pos, wpa_ie_len, &ndev_vif->ap.cache_wpa_ie, &ndev_vif->ap.wpa_ie_len);

		if (r != 0)
			return r;
	}

	if (wmm_ie_pos) {
		wmm_ie_len = *(wmm_ie_pos + 1) + 2;
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "WMM IE found: Length = %zu\n", wmm_ie_len);
		r = slsi_cache_ies(wmm_ie_pos, wmm_ie_len, &ndev_vif->ap.cache_wmm_ie, &ndev_vif->ap.wmm_ie_len);

		if (r != 0)
			return r;
	}
	slsi_clear_cached_ies(&ndev_vif->ap.add_info_ies, &ndev_vif->ap.add_info_ies_len);

	/* Set Vendor specific IEs (WPA, WMM, WPS, P2P) for Beacon, Probe Response and Association Response
	 * The Beacon and Assoc Rsp IEs can include Extended Capability (WLAN_EID_EXT_CAPAB) IE when supported.
	 * Some other IEs (like internetworking, etc) can also come if supported.
	 * The add_info should include only vendor specific IEs and other IEs should be removed if supported in future.
	 */
	if (wmm_ie_pos || wpa_ie_pos || (settings->beacon.beacon_ies_len > 0 && settings->beacon.beacon_ies)) {
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "Add info elements for beacon\n");
		r = slsi_ap_prepare_add_info_ies(ndev_vif, settings->beacon.beacon_ies,
						 settings->beacon.beacon_ies_len);
		if (r != 0)
			return r;

		r = slsi_mlme_add_info_elements(sdev, dev, FAPI_PURPOSE_BEACON, ndev_vif->ap.add_info_ies,
						ndev_vif->ap.add_info_ies_len);
		if (r != 0)
			return r;
		slsi_clear_cached_ies(&ndev_vif->ap.add_info_ies, &ndev_vif->ap.add_info_ies_len);
	}

	if (wmm_ie_pos || wpa_ie_pos || (settings->beacon.proberesp_ies_len > 0 && settings->beacon.proberesp_ies)) {
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "Add info elements for probe response\n");
		r = slsi_ap_prepare_add_info_ies(ndev_vif, settings->beacon.proberesp_ies,
						 settings->beacon.proberesp_ies_len);
		if (r != 0)
			return r;

		r = slsi_mlme_add_info_elements(sdev, dev, FAPI_PURPOSE_PROBE_RESPONSE, ndev_vif->ap.add_info_ies,
						ndev_vif->ap.add_info_ies_len);
		if (r != 0)
			return r;

		slsi_clear_cached_ies(&ndev_vif->ap.add_info_ies, &ndev_vif->ap.add_info_ies_len);
	}

	if (wmm_ie_pos || wpa_ie_pos || (settings->beacon.assocresp_ies_len > 0 && settings->beacon.assocresp_ies)) {
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "Add info elements for assoc response\n");
		r = slsi_ap_prepare_add_info_ies(ndev_vif, settings->beacon.assocresp_ies,
						 settings->beacon.assocresp_ies_len);
		if (r != 0)
			return r;

		r = slsi_mlme_add_info_elements(sdev, dev, FAPI_PURPOSE_ASSOCIATION_RESPONSE, ndev_vif->ap.add_info_ies,
						ndev_vif->ap.add_info_ies_len);
		if (r != 0)
			return r;

		slsi_clear_cached_ies(&ndev_vif->ap.add_info_ies, &ndev_vif->ap.add_info_ies_len);
	}
	return r;
}

#ifdef CONFIG_SCSC_WLAN_WIFI_SHARING_CSA_LEGACY
static int slsi_wifi_sharing_switch_chan(struct wiphy *wiphy, struct net_device *dev,
					 struct cfg80211_ap_settings *settings,
					 int *wifi_sharing_channel_switched, int *skip_indoor_check_for_wifi_sharing)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct netdev_vif *ndev_sta_vif = NULL;
	struct net_device *wlan_dev;
	int r = 0;

	wlan_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	if (wlan_dev)
		ndev_sta_vif = netdev_priv(wlan_dev);

	if (SLSI_IS_VIF_INDEX_MHS(sdev, ndev_vif) && ndev_sta_vif) {
		SLSI_MUTEX_LOCK(ndev_sta_vif->vif_mutex);
		if (ndev_sta_vif->activated && ndev_sta_vif->vif_type == FAPI_VIFTYPE_STATION &&
		    (ndev_sta_vif->sta.vif_status == SLSI_VIF_STATUS_CONNECTING ||
		    ndev_sta_vif->sta.vif_status == SLSI_VIF_STATUS_CONNECTED)) {
			if (!sdev->dualband_concurrency)
				r = slsi_get_mhs_ws_chan_vsdb(wiphy, dev, settings, sdev,
							      wifi_sharing_channel_switched);
			else
				r = slsi_get_mhs_ws_chan_rsdb(wiphy, dev, settings, sdev,
							      wifi_sharing_channel_switched);

			if (r < 0)
				SLSI_NET_ERR(dev, "Rejecting AP start req at host (invalid channel)\n");
			else
				SLSI_DBG1(sdev, SLSI_CFG80211, "Station frequency: %d MHz, SoftAP frequency: %d MHz\n",
					  ndev_sta_vif->chan->center_freq, settings->chandef.chan->center_freq);

			*skip_indoor_check_for_wifi_sharing = 1;
		}
		SLSI_MUTEX_UNLOCK(ndev_sta_vif->vif_mutex);
	}
	return r;
}
#endif

#ifdef CONFIG_SCSC_WLAN_EHT
static void slsi_set_link_id_bssid(struct net_device *dev, char *mhs_bssid, u8 link_id)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u8 *bssid = ndev_vif->ap.ap_link.links[link_id].addr;

	if (ndev_vif->ap.ap_link.valid_links & BIT(link_id))
		SLSI_ETHER_COPY(mhs_bssid, bssid);
	else
		SLSI_ETHER_COPY(mhs_bssid, dev->dev_addr);

	SLSI_NET_INFO(dev, "bssid configured" MACSTR "link id: %d valid_links %d\n",
		      MAC2STR(mhs_bssid), link_id, ndev_vif->ap.ap_link.valid_links);
}
#else
static void slsi_set_link_id_bssid(struct net_device *dev, char *mhs_bssid, u8 link_id)
{
	SLSI_ETHER_COPY(mhs_bssid, dev->dev_addr);

	SLSI_NET_INFO(dev, "bssid configured" MACSTR "\n", MAC2STR(mhs_bssid));
}
#endif

int slsi_start_ap(struct wiphy *wiphy, struct net_device *dev,
		  struct cfg80211_ap_settings *settings)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct net_device *wlan_dev;
	u8                device_address[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	int               r = 0;
	const u8          *wpa_ie_pos = NULL;
	const u8          *wmm_ie_pos = NULL;
	bool              append_vht_ies = false;
	int wifi_sharing_channel_switched = 0;
	int skip_indoor_check_for_wifi_sharing = 0;
	struct ieee80211_mgmt  *mgmt;
	u16                    beacon_ie_head_len;
	int indoor_channel = 0;
	u8 mhs_bssid[ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#ifdef CONFIG_SCSC_WLAN_EHT
	struct slsi_ap_links ap_link;
#endif
	u8 link_id = 0;

#ifdef CONFIG_SCSC_WLAN_EHT
	link_id = settings->beacon.link_id;
#endif

	SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
	if (sdev->device_state != SLSI_DEVICE_STATE_STARTED) {
		SLSI_WARN(sdev, "device not started yet (device_state:%d)\n", sdev->device_state);
		r = -EINVAL;
		goto exit_with_start_stop_mutex;
	}

	if (ndev_vif->iftype == NL80211_IFTYPE_AP && sdev->netdev_ap != sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN])
		slsi_stop_p2p_group_iface_on_ap_start(sdev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	/* Abort any ongoing wlan scan. */
	wlan_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_WLAN);
	if (wlan_dev)
		slsi_abort_hw_scan(sdev, wlan_dev);

	SLSI_NET_DBG1(dev, SLSI_CFG80211, "AP frequency received: %d, width : %d\n", settings->chandef.chan->center_freq, settings->chandef.width);
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
	if (settings->chandef.chan->band == NL80211_BAND_6GHZ && !cfg80211_channel_is_psc(settings->chandef.chan)) {
		SLSI_NET_ERR(dev, "The channel is not PSC channel, not support MHS\n");
		r = -EINVAL;
		goto exit_with_vif_mutex;
	}
#endif

	mgmt = (struct ieee80211_mgmt *)settings->beacon.head;
	beacon_ie_head_len = settings->beacon.head_len - ((u8 *)mgmt->u.beacon.variable - (u8 *)mgmt);

#ifdef CONFIG_SCSC_WLAN_WIFI_SHARING_CSA_LEGACY
	r = slsi_wifi_sharing_switch_chan(wiphy, dev, settings, &wifi_sharing_channel_switched,
					  &skip_indoor_check_for_wifi_sharing);
	if (r != 0)
		goto exit_with_vif_mutex;
#endif

#ifdef CONFIG_SCSC_WLAN_EHT
	memcpy(ap_link.links, ndev_vif->ap.ap_link.links, sizeof(ap_link.links));
	ap_link.valid_links= ndev_vif->ap.ap_link.valid_links;
#endif

	memset(&ndev_vif->ap, 0, sizeof(ndev_vif->ap));
	/* Initialise all allocated peer structures to remove old data. */
	/*slsi_netif_init_all_peers(sdev, dev);*/

#ifdef CONFIG_SCSC_WLAN_EHT
	memcpy(ndev_vif->ap.ap_link.links, ap_link.links, sizeof(ap_link.links));
	ndev_vif->ap.ap_link.valid_links = ap_link.valid_links;
#endif

	r = slsi_configure_country_code(sdev, settings);
	if (r != 0)
		goto exit_with_vif_mutex;
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
	if (settings->chandef.chan->band == NL80211_BAND_5GHZ || settings->chandef.chan->band == NL80211_BAND_6GHZ) {
#else
	if (settings->chandef.chan->band == NL80211_BAND_5GHZ) {
#endif
		r = slsi_ap_start_get_indoor_chan(sdev, dev, settings, skip_indoor_check_for_wifi_sharing,
						  &indoor_channel);
		if (r != 0)
			goto exit_with_vif_mutex;
	}

	r = slsi_ap_start_validate(dev, sdev, settings);
	if (r != 0)
		goto exit_with_vif_mutex;

	if (ndev_vif->iftype == NL80211_IFTYPE_P2P_GO) {
		struct net_device   *p2p_dev;

		slsi_p2p_group_start_remove_unsync_vif(sdev);
		p2p_dev = slsi_get_netdev(sdev, SLSI_NET_INDEX_P2P);
		SLSI_ETHER_COPY(device_address, p2p_dev->dev_addr);
#if defined(CONFIG_SCSC_WLAN_HE) && (defined(SCSC_SEP_VERSION))
		slsi_p2p_go_set_he_mode(sdev, dev, settings);
#endif
		if (keep_alive_period != SLSI_P2PGO_KEEP_ALIVE_PERIOD_SEC)
			if (slsi_set_uint_mib(sdev, NULL, SLSI_PSID_UNIFI_MLMEGO_KEEP_ALIVE_TIMEOUT,
					      keep_alive_period) != 0) {
				SLSI_NET_ERR(dev, "P2PGO Keep Alive MIB set failed");
				r = -EINVAL;
				goto exit_with_vif_mutex;
			}
	}
	slsi_twt_update_ctrl_flags(dev, sdev->twt_enable_responder);

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
	slsi_update_6g_chandef(dev, sdev);
#endif

	if (settings->chandef.chan->band == NL80211_BAND_2GHZ || settings->chandef.chan->band == NL80211_BAND_5GHZ) {
		append_vht_ies = slsi_ap_chandef_vht_ht(dev, sdev, wifi_sharing_channel_switched);
		if (slsi_check_channelization(sdev, ndev_vif->chandef, wifi_sharing_channel_switched) != 0) {
			r = -EINVAL;
			goto exit_with_vif_mutex;
		}
	}

	/* Legacy AP */
	if (ndev_vif->iftype == NL80211_IFTYPE_AP && ndev_vif->chandef->width == NL80211_CHAN_WIDTH_20)
		slsi_ap_start_obss_scan(sdev, dev, ndev_vif);

	if (settings->chandef.chan->band == NL80211_BAND_2GHZ || settings->chandef.chan->band == NL80211_BAND_5GHZ)
		r = slsi_modify_ht_capa_ies(dev, sdev, settings);

	if (indoor_channel == 1	|| wifi_sharing_channel_switched == 1)
		slsi_modify_ies_on_channel_switch(dev, settings, mgmt, beacon_ie_head_len);
	ndev_vif->vif_type = FAPI_VIFTYPE_AP;

	if (slsi_mlme_add_vif(sdev, dev, dev->dev_addr, device_address) != 0) {
		SLSI_NET_ERR(dev, "slsi_mlme_add_vif failed\n");
		r = -EINVAL;
		goto exit_with_vif_mutex;
	}

	if (slsi_vif_activated(sdev, dev) != 0) {
		SLSI_NET_ERR(dev, "slsi_vif_activated failed\n");
		goto exit_with_vif;
	}

	wpa_ie_pos = cfg80211_find_vendor_ie(WLAN_OUI_MICROSOFT, WLAN_OUI_TYPE_MICROSOFT_WPA, settings->beacon.tail, settings->beacon.tail_len);
	wmm_ie_pos = cfg80211_find_vendor_ie(WLAN_OUI_MICROSOFT, WLAN_OUI_TYPE_MICROSOFT_WMM, settings->beacon.tail, settings->beacon.tail_len);
	if (slsi_modify_vendor_ies(sdev, dev, settings, wpa_ie_pos, wmm_ie_pos) != 0) {
		SLSI_NET_ERR(dev, "slsi_modify_vendor_ies failed\n");
		goto exit_with_vif;
	}

	if (ndev_vif->iftype == NL80211_IFTYPE_P2P_GO) {
		u32 af_bmap_active = SLSI_ACTION_FRAME_PUBLIC;
		u32 af_bmap_suspended = SLSI_ACTION_FRAME_PUBLIC;

		r = slsi_mlme_register_action_frame(sdev, dev, af_bmap_active, af_bmap_suspended);
		if (r != 0) {
			SLSI_NET_ERR(dev, "slsi_mlme_register_action_frame failed: resultcode = %d\n", r);
			goto exit_with_vif;
		}
	}

	slsi_set_ap_mode(ndev_vif, settings, append_vht_ies);

	slsi_set_link_id_bssid(dev, (char *) mhs_bssid, link_id);
	r = slsi_mlme_start(sdev, dev, (const unsigned char *) mhs_bssid, settings, wpa_ie_pos,
			    wmm_ie_pos, append_vht_ies);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 94))
        cfg80211_ch_switch_notify(dev, &settings->chandef, 0, 0);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 41))
	cfg80211_ch_switch_notify(dev, &settings->chandef, 0);
#else
	cfg80211_ch_switch_notify(dev, &settings->chandef);
#endif

	if (r != 0) {
		SLSI_NET_ERR(dev, "Start ap failed: resultcode = %d frequency = %d\n", r,
			     settings->chandef.chan->center_freq);
		goto exit_with_vif;
	}
	SLSI_NET_DBG1(dev, SLSI_CFG80211, "Soft Ap started on frequency: %d MHz\n",
		      settings->chandef.chan->center_freq);
	if (ndev_vif->iftype == NL80211_IFTYPE_P2P_GO)
		SLSI_P2P_STATE_CHANGE(sdev, P2P_GROUP_FORMED_GO);
#ifdef CONFIG_SCSC_WLAN_WIFI_SHARING
	else if (SLSI_IS_VIF_INDEX_MHS(sdev, ndev_vif))
		ndev_vif->chan = settings->chandef.chan;
#endif

	ndev_vif->ap.beacon_interval = settings->beacon_interval;
	ndev_vif->ap.ssid_len = settings->ssid_len;
	memcpy(ndev_vif->ap.ssid, settings->ssid, settings->ssid_len);

	netif_dormant_off(dev);

	if (ndev_vif->ipaddress)
		/* Static IP is assigned already */
		slsi_ip_address_changed(sdev, dev, ndev_vif->ipaddress);

	r = slsi_read_disconnect_ind_timeout(sdev, SLSI_PSID_UNIFI_DISCONNECT_TIMEOUT);
	if (r != 0)
		sdev->device_config.ap_disconnect_ind_timeout = *sdev->sig_wait_cfm_timeout;

	SLSI_NET_DBG2(dev, SLSI_CFG80211, "slsi_read_disconnect_ind_timeout: timeout = %d\n",
		      sdev->device_config.ap_disconnect_ind_timeout);
	goto exit_with_vif_mutex;
exit_with_vif:
	slsi_clear_cached_ies(&ndev_vif->ap.add_info_ies, &ndev_vif->ap.add_info_ies_len);
	if (slsi_mlme_del_vif(sdev, dev) != 0)
		SLSI_NET_ERR(dev, "slsi_mlme_del_vif failed\n");
	slsi_vif_deactivated(sdev, dev);
	r = -EINVAL;
exit_with_vif_mutex:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
exit_with_start_stop_mutex:
	SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
	slsi_store_settings_for_recovery(settings, ndev_vif);
	return r;
}

int slsi_change_beacon(struct wiphy *wiphy, struct net_device *dev,
		       struct cfg80211_beacon_data *info)
{
	SLSI_UNUSED_PARAMETER(info);

	return -EOPNOTSUPP;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 41))
int slsi_stop_ap(struct wiphy *wiphy, struct net_device *dev, unsigned int link_id)
{
#else
int slsi_stop_ap(struct wiphy *wiphy, struct net_device *dev)
{
#endif
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	slsi_reset_throughput_stats(dev);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return 0;
}

#if (KERNEL_VERSION(5, 2, 0) < LINUX_VERSION_CODE)
int slsi_update_owe_info(struct wiphy *wiphy, struct net_device *dev,
			 struct cfg80211_update_owe_info *owe_info)

{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int               r = 0;

	if (owe_info->status) {
		SLSI_NET_INFO(dev, "OWE status failed:%d\n", owe_info->status);
		return -EINVAL;
	}

	if (!cfg80211_find_ext_ie(SLSI_WLAN_EID_EXT_OWE_DH_PARAM, owe_info->ie, owe_info->ie_len))
		SLSI_NET_INFO(dev, "OWE DH Param (for AssocResp) is NOT present. using PMKSA cache\n");
	else
		SLSI_NET_INFO(dev, "OWE DH Param (for AssocResp) is present.\n");

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!ndev_vif->activated || ndev_vif->iftype != NL80211_IFTYPE_AP) {
		SLSI_NET_INFO(dev, "ndev_vif (type:%d, activated:%d) not valid for OWE\n",
			      ndev_vif->iftype, ndev_vif->activated);
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		return -EINVAL;
	}

	r = slsi_mlme_add_info_elements(sdev, dev, FAPI_PURPOSE_ASSOCIATION_RESPONSE,
					owe_info->ie, owe_info->ie_len);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	return r;
}
#endif

static int slsi_p2p_group_mgmt_tx(const struct ieee80211_mgmt *mgmt, struct wiphy *wiphy,
				  struct net_device *dev, struct ieee80211_channel *chan,
				  unsigned int wait, const u8 *buf, size_t len,
				  bool dont_wait_for_ack, u64 *cookie)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif;
	struct net_device *netdev;
	int               subtype = slsi_get_public_action_subtype(mgmt);
	int               r = 0;
	u32               host_tag = slsi_tx_mgmt_host_tag(sdev);
	u16               freq = 0;
	u32               dwell_time = SLSI_FORCE_SCHD_ACT_FRAME_MSEC;
	u16               data_unit_desc = FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME;

	if (sdev->p2p_group_exp_frame != SLSI_PA_INVALID) {
		SLSI_NET_ERR(dev, "sdev->p2p_group_exp_frame : %d\n", sdev->p2p_group_exp_frame);
		return -EINVAL;
	}

	netdev = slsi_get_netdev(sdev, SLSI_NET_INDEX_P2PX_SWLAN);
	ndev_vif = netdev_priv(netdev);

	if (!ndev_vif->chan) {
		SLSI_NET_ERR(dev, "ndev_vif->chan is null\n");
		return -EINVAL;
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	SLSI_NET_DBG2(dev, SLSI_CFG80211, "Sending Action frame (%s) on p2p group vif (%d), if_index = %d,"
		      "vif_type = %d, chan->hw_value = %d, ndev_vif->chan->hw_value = %d, wait = %d,"
		      "sdev->p2p_group_exp_frame = %d\n", slsi_pa_subtype_text(subtype), ndev_vif->activated,
		      ndev_vif->ifnum, ndev_vif->vif_type, chan->hw_value, ndev_vif->chan->hw_value, wait,
		      sdev->p2p_group_exp_frame);

	if (!((ndev_vif->iftype == NL80211_IFTYPE_P2P_GO) || (ndev_vif->iftype == NL80211_IFTYPE_P2P_CLIENT)))
		goto exit_with_error;

	if (chan->hw_value != ndev_vif->chan->hw_value) {
		freq = SLSI_FREQ_HOST_TO_FW(chan->center_freq);
		dwell_time = wait;
	}

	/* Incase of GO dont wait for resp/cfm packets for go-negotiation.*/
	if (subtype != SLSI_P2P_PA_GO_NEG_RSP)
		sdev->p2p_group_exp_frame = slsi_get_exp_peer_frame_subtype(subtype);

	r = slsi_mlme_send_frame_mgmt(sdev, netdev, buf, len, data_unit_desc, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, freq, dwell_time * 1000, 0);
	if (r)
		goto exit_with_lock;
	slsi_assign_cookie_id(cookie, &ndev_vif->mgmt_tx_cookie);
	r = slsi_set_mgmt_tx_data(ndev_vif, *cookie, host_tag, buf, len);         /* If error then it is returned in exit */
	goto exit_with_lock;

exit_with_error:
	r = -EINVAL;
exit_with_lock:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

/* Handle mgmt_tx callback for P2P modes */
static int slsi_p2p_mgmt_tx(const struct ieee80211_mgmt *mgmt, struct wiphy *wiphy,
			    struct net_device *dev, struct netdev_vif *ndev_vif,
			    struct ieee80211_channel *chan, unsigned int wait,
			    const u8 *buf, size_t len, bool dont_wait_for_ack, u64 *cookie)
{
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
	int             ret = 0, status = 0;

	if (ieee80211_is_action(mgmt->frame_control)) {
		u16 host_tag = slsi_tx_mgmt_host_tag(sdev);
		int subtype = slsi_get_public_action_subtype(mgmt);
		u8  exp_peer_frame;
		u32 dwell_time = 0;

		SLSI_NET_DBG2(dev, SLSI_CFG80211, "Action frame (%s), unsync_vif_active (%d)\n", slsi_pa_subtype_text(subtype), ndev_vif->activated);

		if (subtype == SLSI_PA_INVALID) {
			SLSI_NET_ERR(dev, "Invalid Action frame subtype\n");
			goto exit_with_error;
		}

		/* Check if unsync vif is available */
		if (sdev->p2p_state == P2P_IDLE_NO_VIF)
			if (slsi_p2p_vif_activate(sdev, dev, chan, wait, false) != 0)
				goto exit_with_error;

			/* Clear Probe Response IEs if vif was already present with a different channel */
		if (ndev_vif->driver_channel != chan->hw_value) {
			if (slsi_mlme_add_info_elements(sdev, dev, FAPI_PURPOSE_PROBE_RESPONSE, NULL, 0) != 0)
				SLSI_NET_ERR(dev, "Clearing Probe Response IEs failed for unsync vif\n");
			slsi_unsync_vif_set_probe_rsp_ie(ndev_vif, NULL, 0);

			if (slsi_mlme_set_channel(sdev, dev, chan, SLSI_FW_CHANNEL_DURATION_UNSPECIFIED, 0, 0) != 0)
				goto exit_with_vif;
			else {
				ndev_vif->chan = chan;
				ndev_vif->driver_channel = chan->hw_value;
			}
		}

		/* Check if peer frame response is expected */
		exp_peer_frame = slsi_get_exp_peer_frame_subtype(subtype);
		status = slsi_p2p_get_action_frame_status(dev, mgmt);
		if (exp_peer_frame != SLSI_PA_INVALID) {
			if ((subtype == SLSI_P2P_PA_GO_NEG_RSP || subtype == SLSI_P2P_PA_GO_NEG_REQ ||
			    subtype == SLSI_P2P_PA_PROV_DISC_REQ) && status!= SLSI_P2P_STATUS_CODE_SUCCESS) {
				SLSI_NET_DBG1(dev, SLSI_CFG80211, "Action frame (%s) , Status:%d peer response not expected\n",
					      slsi_pa_subtype_text(subtype), status);
				exp_peer_frame = SLSI_PA_INVALID;
			} else {
				SLSI_NET_DBG1(dev, SLSI_CFG80211, "Peer response expected with action frame (%s)\n",
					      slsi_pa_subtype_text(exp_peer_frame));

				if (ndev_vif->mgmt_tx_data.exp_frame != SLSI_PA_INVALID)
					(void)slsi_set_mgmt_tx_data(ndev_vif, 0, 0, NULL, 0);

				if (subtype == SLSI_P2P_PA_GO_NEG_RSP)
					ndev_vif->drv_in_p2p_procedure = true;

				/* Change Force Schedule Duration as peer response is expected */
				if (wait)
					dwell_time = wait;
				else
					dwell_time = SLSI_FORCE_SCHD_ACT_FRAME_MSEC;
			}
		}

		slsi_assign_cookie_id(cookie, &ndev_vif->mgmt_tx_cookie);

		/* Send the action frame, transmission status indication would be received later */
		if (slsi_mlme_send_frame_mgmt(sdev, dev, buf, len, FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, 0, dwell_time * 1000, 0) != 0)
			goto exit_with_vif;
		if (subtype == SLSI_P2P_PA_GO_NEG_CFM)
			ndev_vif->drv_in_p2p_procedure = false;
		else if ((subtype == SLSI_P2P_PA_GO_NEG_REQ || subtype == SLSI_P2P_PA_PROV_DISC_REQ) && exp_peer_frame != SLSI_PA_INVALID)
			ndev_vif->drv_in_p2p_procedure = true;
		/* If multiple frames are requested for tx, only the info of first frame would be stored */
		if (ndev_vif->mgmt_tx_data.host_tag == 0) {
			unsigned int n_wait = 0;

			SLSI_NET_DBG1(dev, SLSI_CFG80211, "Store mgmt frame tx data for cookie = 0x%llx\n", *cookie);

			ret = slsi_set_mgmt_tx_data(ndev_vif, *cookie, host_tag, buf, len);
			if (ret != 0)
				goto exit_with_vif;
			ndev_vif->mgmt_tx_data.exp_frame = exp_peer_frame;

			SLSI_P2P_STATE_CHANGE(sdev, P2P_ACTION_FRAME_TX_RX);
			if ((exp_peer_frame == SLSI_P2P_PA_GO_NEG_RSP) || (exp_peer_frame == SLSI_P2P_PA_GO_NEG_CFM))
				/* Retain vif for larger duration that wpa_supplicant asks to wait,
				 * during GO-Negotiation to allow peer to retry GO neg in bad radio condition.
				 * Some of phones retry GO-Negotiation after 2 seconds
				 */
				n_wait = SLSI_P2P_NEG_PROC_UNSYNC_VIF_RETAIN_DURATION;
			else if (exp_peer_frame != SLSI_PA_INVALID)
				/* If a peer response is expected queue work to retain vif till wait time else the work will be handled in mgmt_tx_cancel_wait */
				n_wait = wait + SLSI_P2P_MGMT_TX_EXTRA_MSEC;
			if (n_wait) {
				SLSI_NET_DBG2(dev, SLSI_CFG80211, "retain unsync vif for duration (%d) msec\n", n_wait);
				slsi_p2p_queue_unsync_vif_del_work(ndev_vif, n_wait);
			}
		} else {
			/* Already a frame Tx is in progress, send immediate tx_status as success. Sending immediate tx status should be ok
			 * as supplicant is in another procedure and so these frames would be mostly only response frames.
			 */
			WLBT_WARN_ON(sdev->p2p_state != P2P_ACTION_FRAME_TX_RX);

			if (!dont_wait_for_ack) {
				SLSI_NET_DBG1(dev, SLSI_CFG80211, "Send immediate tx_status (cookie = 0x%llx)\n", *cookie);
				cfg80211_mgmt_tx_status(&ndev_vif->wdev, *cookie, buf, len, true, GFP_KERNEL);
			}
		}
		goto exit;
	}

	/* Else send failure for unexpected management frame */
	SLSI_NET_ERR(dev, "Drop Tx frame: Unexpected Management frame\n");
	goto exit_with_error;

exit_with_vif:
	if (sdev->p2p_state != P2P_LISTENING)
		slsi_p2p_vif_deactivate(sdev, dev, true);
exit_with_error:
	ret = -EINVAL;
exit:
	return ret;
}

int     slsi_mgmt_tx_cancel_wait(struct wiphy        *wiphy,
				 struct wireless_dev *wdev,
				 u64                 cookie)
{
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	SLSI_NET_INFO(dev, "iface_num = %d, cookie = 0x%llx, vif_index = %d, vif_type = %d,"
		      "sdev->p2p_state = %d, ndev_vif->mgmt_tx_data.cookie = 0x%llx, sdev->p2p_group_exp_frame = %d,"
		      "sdev->wlan_unsync_vif_state = %d\n", (int)ndev_vif->ifnum, cookie, (int)ndev_vif->ifnum,
		      (int)ndev_vif->vif_type, sdev->p2p_state, ndev_vif->mgmt_tx_data.cookie,
		      (int)sdev->p2p_group_exp_frame, sdev->wlan_unsync_vif_state);

	/* If device was in frame tx_rx state, clear mgmt tx data and change state */
	if (SLSI_IS_VIF_INDEX_P2P(ndev_vif) && (sdev->p2p_state == P2P_ACTION_FRAME_TX_RX) && (ndev_vif->mgmt_tx_data.cookie == cookie)) {
		if (ndev_vif->mgmt_tx_data.exp_frame != SLSI_PA_INVALID)
			(void)slsi_mlme_reset_dwell_time(sdev, dev);

		(void)slsi_set_mgmt_tx_data(ndev_vif, 0, 0, NULL, 0);
		ndev_vif->mgmt_tx_data.exp_frame = SLSI_PA_INVALID;

		if (delayed_work_pending(&ndev_vif->unsync.roc_expiry_work)) {
			SLSI_P2P_STATE_CHANGE(sdev, P2P_LISTENING);
		} else {
			slsi_p2p_queue_unsync_vif_del_work(ndev_vif, SLSI_P2P_UNSYNC_VIF_EXTRA_MSEC);
			SLSI_P2P_STATE_CHANGE(ndev_vif->sdev, P2P_IDLE_VIF_ACTIVE);
		}
	} else if ((SLSI_IS_P2P_GROUP_STATE(sdev)) && (sdev->p2p_group_exp_frame != SLSI_PA_INVALID)) {
		/* acquire mutex lock if it is not group net dev */
		slsi_clear_offchannel_data(sdev, (!SLSI_IS_VIF_INDEX_P2P_GROUP(sdev, ndev_vif)) ? true : false);
	} else if (SLSI_IS_VIF_INDEX_WLAN(ndev_vif) && (sdev->wlan_unsync_vif_state == WLAN_UNSYNC_VIF_TX) && (ndev_vif->mgmt_tx_data.cookie == cookie)) {
		sdev->wlan_unsync_vif_state = WLAN_UNSYNC_VIF_ACTIVE;
		cancel_delayed_work(&ndev_vif->unsync.hs2_del_vif_work);
		queue_delayed_work(sdev->device_wq, &ndev_vif->unsync.hs2_del_vif_work, msecs_to_jiffies(SLSI_HS2_UNSYNC_VIF_EXTRA_MSEC));
		if (ndev_vif->mgmt_tx_data.exp_frame != SLSI_PA_INVALID) {
			ndev_vif->mgmt_tx_data.exp_frame = SLSI_PA_INVALID;
			(void)slsi_mlme_reset_dwell_time(sdev, dev);
		}
	} else if (ndev_vif->activated && ndev_vif->vif_type == FAPI_VIFTYPE_STATION
		   && ndev_vif->sta.vif_status == SLSI_VIF_STATUS_CONNECTED) {
		if (ndev_vif->mgmt_tx_data.exp_frame != SLSI_PA_INVALID) {
			ndev_vif->mgmt_tx_data.exp_frame = SLSI_PA_INVALID;
			(void)slsi_mlme_reset_dwell_time(sdev, dev);
		}
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return 0;
}

void slsi_mgmt_frame_register(struct wiphy *wiphy,
			      struct wireless_dev *wdev,
			      u16 frame_type, bool reg)
{
	struct net_device *dev = wdev->netdev;
	struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);

	SLSI_UNUSED_PARAMETER(frame_type);
	SLSI_UNUSED_PARAMETER(reg);

	if (WLBT_WARN_ON(!dev))
		return;

	SLSI_UNUSED_PARAMETER(sdev);
}

int slsi_wlan_mgmt_tx(struct slsi_dev *sdev, struct net_device *dev,
		      struct ieee80211_channel *chan, unsigned int wait,
		      const u8 *buf, size_t len, bool dont_wait_for_ack, u64 *cookie)
{
	u32                   host_tag = slsi_tx_mgmt_host_tag(sdev);
	struct netdev_vif     *ndev_vif = netdev_priv(dev);
	int                   r = 0;
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)buf;
	u8                    exp_peer_frame = SLSI_PA_INVALID;
	int                   subtype = SLSI_PA_INVALID;

	slsi_wlan_dump_public_action_subtype(sdev, mgmt, true);

	if (ieee80211_is_action(mgmt->frame_control)) {
		subtype = slsi_get_public_action_subtype(mgmt);

		if (subtype != SLSI_PA_INVALID)
			exp_peer_frame = slsi_get_exp_peer_frame_subtype(subtype);
	}

	if (!ndev_vif->activated) {
		if (subtype >= SLSI_PA_GAS_INITIAL_REQ_SUBTYPE && subtype <= SLSI_PA_GAS_COMEBACK_RSP_SUBTYPE) {
			ndev_vif->mgmt_tx_gas_frame = true;
			SLSI_ETHER_COPY(ndev_vif->gas_frame_mac_addr, mgmt->sa);
		} else {
			ndev_vif->mgmt_tx_gas_frame = false;
		}
		r = slsi_wlan_unsync_vif_activate(sdev, dev, chan, wait);
		if (r)
			return r;

		r = slsi_mlme_send_frame_mgmt(sdev, dev, buf, len, FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, 0, wait * 1000, 0);
		if (r)
			goto exit_with_vif;
		sdev->wlan_unsync_vif_state = WLAN_UNSYNC_VIF_TX;
		queue_delayed_work(sdev->device_wq, &ndev_vif->unsync.hs2_del_vif_work, msecs_to_jiffies(wait));
	} else {
		if (ndev_vif->vif_type == FAPI_VIFTYPE_PRECONNECT) {
			if (subtype >= SLSI_PA_GAS_INITIAL_REQ_SUBTYPE && subtype <= SLSI_PA_GAS_COMEBACK_RSP_SUBTYPE) {
				slsi_wlan_unsync_vif_deactivate(sdev, dev, true);
				ndev_vif->mgmt_tx_gas_frame = true;
				SLSI_ETHER_COPY(ndev_vif->gas_frame_mac_addr, mgmt->sa);
				r = slsi_wlan_unsync_vif_activate(sdev, dev, chan, wait);
				if (r)
					return r;
			} else {
				if (ndev_vif->mgmt_tx_gas_frame) {
					slsi_wlan_unsync_vif_deactivate(sdev, dev, true);
					ndev_vif->mgmt_tx_gas_frame = false;
					r = slsi_wlan_unsync_vif_activate(sdev, dev, chan, wait);
					if (r)
						return r;
				}
			}

			cancel_delayed_work(&ndev_vif->unsync.hs2_del_vif_work);
			/*even if we fail to cancel the delayed work, we shall go ahead and send action frames*/
			if (ndev_vif->driver_channel != chan->hw_value) {
				r = slsi_mlme_set_channel(sdev, dev, chan, SLSI_FW_CHANNEL_DURATION_UNSPECIFIED, 0, 0);
				if (r)
					goto exit_with_vif;
				else
					ndev_vif->driver_channel = chan->hw_value;
			}
			SLSI_NET_DBG1(dev, SLSI_CFG80211, "wlan unsync vif is active, send frame on channel freq = %d\n", chan->center_freq);
			r = slsi_mlme_send_frame_mgmt(sdev, dev, buf, len, FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, 0, wait * 1000, 0);
			if (r)
				goto exit_with_vif;
			sdev->wlan_unsync_vif_state = WLAN_UNSYNC_VIF_TX;
			queue_delayed_work(sdev->device_wq, &ndev_vif->unsync.hs2_del_vif_work, msecs_to_jiffies(wait));
		} else if (ndev_vif->chan && ndev_vif->chan->hw_value == chan->hw_value) {
			/* Dwell time not provided when sending frames on connected channel. */
			SLSI_NET_DBG1(dev, SLSI_CFG80211, "STA VIF is active on same channel, send frame on channel freq %d\n", chan->center_freq);
			r = slsi_mlme_send_frame_mgmt(sdev, dev, buf, len, FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, 0, 0, 0);
			if (r)
				return r;
		} else {
			SLSI_NET_DBG1(dev, SLSI_CFG80211, "STA VIF is active on a different channel, send frame on channel freq %d\n", chan->center_freq);
			/* Dwell time for GAS (ANQP) request packet set to 100ms if dwell time(wait) is more than 100ms */
			if ((subtype == SLSI_PA_GAS_INITIAL_REQ_SUBTYPE || subtype == SLSI_PA_GAS_COMEBACK_REQ_SUBTYPE) && wait > SLSI_FW_MAX_OFFCHANNEL_DWELL_TIME)
				wait = SLSI_FW_MAX_OFFCHANNEL_DWELL_TIME;
			r = slsi_mlme_send_frame_mgmt(sdev, dev, buf, len, FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, SLSI_FREQ_HOST_TO_FW(chan->center_freq), wait * 1000, 0);
			if (r)
				return r;
		}
	}

	ndev_vif->mgmt_tx_data.exp_frame = exp_peer_frame;
	slsi_assign_cookie_id(cookie, &ndev_vif->mgmt_tx_cookie);
	slsi_set_mgmt_tx_data(ndev_vif, *cookie, host_tag, buf, len);
	return r;

exit_with_vif:
	slsi_wlan_unsync_vif_deactivate(sdev, dev, true);
	return r;
}

static int slsi_wlan_mgmt_auth_tx(struct slsi_dev *sdev, struct net_device *dev, struct ieee80211_channel *chan,
				   unsigned int wait, const u8 *buf, size_t len, u64 *cookie)
{
	u32                   host_tag = slsi_tx_mgmt_host_tag(sdev);
	struct netdev_vif     *ndev_vif = netdev_priv(dev);
	int                   r = 0;
	struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)buf;

	SLSI_NET_DBG1(dev, SLSI_CFG80211, "Transmit on the current frequency\n");

	if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION) {
		ndev_vif->sta.sae_auth_type = mgmt->u.auth.auth_transaction;
		wait = sdev->sae_dwell_time;
	}

	r = slsi_mlme_send_frame_mgmt(sdev, dev, buf, len, FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME,
				      FAPI_MESSAGETYPE_IEEE80211_MGMT, host_tag, 0, wait * 1000, 0);
	if (r)
		return r;

	ndev_vif->mgmt_tx_data.exp_frame = SLSI_PA_INVALID;
	slsi_assign_cookie_id(cookie, &ndev_vif->mgmt_tx_cookie);
	return r;
}

int slsi_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
		 struct cfg80211_mgmt_tx_params *params,
		 u64 *cookie)
{
	/* Note to explore for AP ::All public action frames which come to host should be handled properly
	 * Additionally, if PMF is negotiated over the link, the host shall not issue "mlme-send-frame.request"
	 * primitive  for action frames before the pairwise keys have been installed in F/W. Presently, for
	 * SoftAP with PMF support, there is no scenario in which slsi_mlme_send_frame will be called for
	 * action frames for VIF TYPE = AP.
	 */

	struct net_device           *dev = wdev->netdev;
	struct ieee80211_channel    *chan = NULL;
	bool                        offchan = params->offchan;
	unsigned int                wait = params->wait;
	const u8                    *buf = params->buf;
	size_t                      len = params->len;
	bool                        no_cck = params->no_cck;
	bool                        dont_wait_for_ack = params->dont_wait_for_ack;
	struct slsi_dev             *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif           *ndev_vif = netdev_priv(dev);
	const struct ieee80211_mgmt *mgmt = (const struct ieee80211_mgmt *)buf;
	int                         r = 0;

	SLSI_UNUSED_PARAMETER(offchan);
	SLSI_UNUSED_PARAMETER(no_cck);
	SLSI_MUTEX_LOCK(sdev->start_stop_mutex);
	if (sdev->device_state != SLSI_DEVICE_STATE_STARTED) {
		SLSI_WARN(sdev, "device not started yet (device_state:%d)\n", sdev->device_state);
		r = -EINVAL;
		goto exit;
	}

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (!(ieee80211_is_mgmt(mgmt->frame_control))) {
		SLSI_NET_ERR(dev, "Drop Tx frame: Not a Management frame\n");
		r = -EINVAL;
		goto exit;
	}

	/* If params->chan is NULL, it should use the current chan as per the nl80211 */
	chan = params->chan ? params->chan : ndev_vif->chan;
	if (!chan) {
		SLSI_NET_ERR(dev, "Drop mgmt_tx request. chan is NULL\n");
		r = -EINVAL;
		goto exit;
	}

	SLSI_NET_INFO(dev, "fc:%d, len:%d\n", mgmt->frame_control, len);
	if (!(ieee80211_is_auth(mgmt->frame_control))) {
		SLSI_NET_DBG2(dev, SLSI_CFG80211, "Mgmt Frame Tx: iface_num = %d, channel = %d, wait = %d, noAck = %d,"
			      "offchannel = %d, mgmt->frame_control = %d, vif_type = %d\n", ndev_vif->ifnum, chan->hw_value,
			      wait, dont_wait_for_ack, offchan, mgmt->frame_control, ndev_vif->vif_type);

		if (SLSI_IS_VIF_INDEX_WLAN(ndev_vif)) {
			r = slsi_wlan_mgmt_tx(sdev, dev, chan, wait, buf, len, dont_wait_for_ack, cookie);
			goto exit;
		}
	} else {
		if (!ndev_vif->activated) {
			SLSI_NET_ERR(dev, "Drop Auth Frame: VIF not activated\n");
			r = -EINVAL;
			goto exit;
		}

		SLSI_NET_DBG2(dev, SLSI_CFG80211, "Send Auth Frame : iface_num = %d, iftype = %d\n",
			      ndev_vif->ifnum, ndev_vif->iftype);

		if (SLSI_IS_VIF_INDEX_WLAN(ndev_vif) || ndev_vif->iftype == NL80211_IFTYPE_AP ||
		    SLSI_IS_VIF_INDEX_P2P_GROUP(sdev, ndev_vif)) {
			r = slsi_wlan_mgmt_auth_tx(sdev, dev, chan, wait, buf, len, cookie);
			goto exit;
		}
	}

	/*P2P*/

	/* Drop Probe Responses which can come in P2P Device and P2P Group role */
	if (ieee80211_is_probe_resp(mgmt->frame_control)) {
		/* Ideally supplicant doesn't expect Tx status for Probe Rsp. Send tx status just in case it requests ack */
		if (!dont_wait_for_ack) {
			slsi_assign_cookie_id(cookie, &ndev_vif->mgmt_tx_cookie);
			cfg80211_mgmt_tx_status(wdev, *cookie, buf, len, true, GFP_KERNEL);
		}
		goto exit;
	}

	if (SLSI_IS_VIF_INDEX_P2P(ndev_vif)) {
		struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
		/* Check whether STA scan is running or not. If yes, then abort the STA scan */
		slsi_abort_sta_scan(sdev);
		if (SLSI_IS_P2P_GROUP_STATE(sdev))
			r = slsi_p2p_group_mgmt_tx(mgmt, wiphy, dev, chan, wait, buf, len, dont_wait_for_ack, cookie);
		else
			r = slsi_p2p_mgmt_tx(mgmt, wiphy, dev, ndev_vif, chan, wait, buf, len, dont_wait_for_ack, cookie);
	} else if (SLSI_IS_VIF_INDEX_P2P_GROUP(sdev, ndev_vif))
		if (ndev_vif->chan && chan->hw_value == ndev_vif->chan->hw_value) {
			struct slsi_dev *sdev = SDEV_FROM_WIPHY(wiphy);
			u16             host_tag = slsi_tx_mgmt_host_tag(sdev);

			r = slsi_mlme_send_frame_mgmt(sdev, dev, buf, len, FAPI_DATAUNITDESCRIPTOR_IEEE802_11_FRAME, FAPI_MESSAGETYPE_IEEE80211_ACTION, host_tag, 0, 0, 0);
			if (r) {
				SLSI_NET_ERR(dev, "Failed to send action frame, r = %d\n", r);
				goto exit;
			}
			slsi_assign_cookie_id(cookie, &ndev_vif->mgmt_tx_cookie);
			r = slsi_set_mgmt_tx_data(ndev_vif, *cookie, host_tag, buf, len);
		}
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	SLSI_MUTEX_UNLOCK(sdev->start_stop_mutex);
	return r;
}

/* cw = (2^n -1). But WMM IE needs value n. */
u8 slsi_get_ecw(int cw)
{
	int ecw = 0;

	cw = cw + 1;
	do {
		cw = cw >> 1;
		ecw++;
	} while (cw);
	return ecw - 1;
}

int slsi_set_txq_params(struct wiphy *wiphy, struct net_device *ndev,
			struct ieee80211_txq_params *params)
{
	struct slsi_dev                   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif                 *ndev_vif = netdev_priv(ndev);
	struct slsi_wmm_parameter_element *wmm_ie = &ndev_vif->ap.wmm_ie;
	int                               r = 0;
	int                               ac = params->ac;

	/* Index remapping for AC from nl80211_ac enum to slsi_ac_index_wmm enum (index to be used in the IE).
	 * Kernel version less than 3.5.0 doesn't support nl80211_ac enum hence not using the nl80211_ac enum.
	 * Eg. NL80211_AC_VO (index value 0) would be remapped to AC_VO (index value 3).
	 * Don't change the order of array elements.
	 */
	u8  ac_index_map[4] = { AC_VO, AC_VI, AC_BE, AC_BK };
	int ac_remapped = ac_index_map[ac];

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	SLSI_NET_DBG2(ndev, SLSI_CFG80211, " ac= 0x%x, ac_remapped = %d aifs = %d, cmin=0x%x cmax = 0x%x, txop = 0x%x,"
		      "if_index = %d vif_type = %d\n", ac, ac_remapped, params->aifs, params->cwmin, params->cwmax,
		      params->txop, ndev_vif->ifnum, ndev_vif->vif_type);

	if (ndev_vif->activated) {
		wmm_ie->ac[ac_remapped].aci_aifsn = (ac_remapped << 5) | (params->aifs & 0x0f);
		wmm_ie->ac[ac_remapped].ecw = ((slsi_get_ecw(params->cwmax)) << 4) | ((slsi_get_ecw(params->cwmin)) & 0x0f);
		wmm_ie->ac[ac_remapped].txop_limit = cpu_to_le16(params->txop);
		if (ac == 3) {
			wmm_ie->eid = SLSI_WLAN_EID_VENDOR_SPECIFIC;
			wmm_ie->len = 24;
			wmm_ie->oui[0] = 0x00;
			wmm_ie->oui[1] = 0x50;
			wmm_ie->oui[2] = 0xf2;
			wmm_ie->oui_type = WLAN_OUI_TYPE_MICROSOFT_WMM;
			wmm_ie->oui_subtype = 1;
			wmm_ie->version = 1;
			wmm_ie->qos_info = 0;
			wmm_ie->reserved = 0;
			r = slsi_mlme_add_info_elements(sdev, ndev, FAPI_PURPOSE_LOCAL, (const u8 *)wmm_ie, sizeof(struct slsi_wmm_parameter_element));
			if (r)
				SLSI_NET_ERR(ndev, "Error sending TX Queue Parameters for AP error = %d\n", r);
		}
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

int slsi_synchronised_response(struct wiphy *wiphy, struct net_device *dev,
			       struct cfg80211_external_auth_params *params)
{
	struct slsi_dev                   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif                 *ndev_vif = netdev_priv(dev);
	int r;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
	if (ndev_vif->sta.wpa3_sae_reconnection && SLSI_ETHER_EQUAL(params->bssid, ndev_vif->sta.bssid)) {
		SLSI_NET_ERR(dev, "Droping synchronised_resp for bssid:" MACSTR "\n",
			     MAC2STR(params->bssid));
		SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
		ndev_vif->sta.wpa3_sae_reconnection = false;
		return 0;
	}
#endif
	r = slsi_mlme_synchronised_response(sdev, dev, params);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	slsi_wake_unlock(&ndev_vif->wlan_wl_sae);
	return r;
}

#ifdef CONFIG_SCSC_WLAN_EHT
int slsi_set_intf_link(struct wiphy *wiphy, struct wireless_dev *wdev,
		       uint link_id)
{
	struct net_device *dev = wdev->netdev;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int r = 0;
	u16 valid_links = wdev->valid_links;

	SLSI_NET_INFO(dev, "\n");
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	ndev_vif->ap.ap_link.valid_links = valid_links;

	if(!(valid_links & BIT(link_id)))
		goto exit;

	SLSI_NET_INFO(dev, "ndev_vif->ap.valid_links %d link_id %d bssid:" MACSTR "\n",
		      valid_links, link_id, MAC2STR(wdev->links[link_id].addr));

	SLSI_ETHER_COPY(ndev_vif->ap.ap_link.links[link_id].addr, wdev->links[link_id].addr);

exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}
#endif


static int slsi_update_ft_ies(struct wiphy *wiphy, struct net_device *dev, struct cfg80211_update_ft_ies_params *ftie)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int               r = 0;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	if (ndev_vif->vif_type == FAPI_VIFTYPE_STATION) {
		const u8 *keo_ie_pos = NULL;
		u8 *ie_buf = NULL;
		int ie_len = 0;
		int ie_buf_len = 0;

		keo_ie_pos = cfg80211_find_vendor_ie(WLAN_OUI_SAMSUNG, WLAN_OUI_TYPE_SAMSUNG_KEO,
						     ndev_vif->sta.assoc_req_add_info_elem,
						     ndev_vif->sta.assoc_req_add_info_elem_len);
		if (keo_ie_pos) {
			ie_buf_len = ftie->ie_len +
				     ndev_vif->sta.assoc_req_add_info_elem_len -
				     (keo_ie_pos - ndev_vif->sta.assoc_req_add_info_elem);
			ie_buf = kmalloc(ie_buf_len, GFP_KERNEL);
			if (!ie_buf) {
				SLSI_NET_ERR(dev, "kmalloc failed\n");
				SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
				return -ENOMEM;
			}
			ie_len = ftie->ie_len;
			if (ie_buf_len < ie_len) {
				SLSI_NET_ERR(dev, "ft_ie buffer overflow!!\n");
				kfree(ie_buf);
				ie_buf = NULL;
				SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
				return -EINVAL;
			}
			memcpy(ie_buf, ftie->ie, ie_len);
			if ((ie_buf_len - ie_len) >= ((int)keo_ie_pos[1] + 2)) {
				memcpy(&ie_buf[ie_len], keo_ie_pos, ((int)keo_ie_pos[1] + 2));
				ie_len += (keo_ie_pos[1] + 2);
				keo_ie_pos += (keo_ie_pos[1] + 2);
			} else {
				SLSI_NET_ERR(dev, "ie_buf buffer overflow!!\n");
				kfree(ie_buf);
				ie_buf = NULL;
				SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
				return -EINVAL;
			}
			while ((ndev_vif->sta.assoc_req_add_info_elem_len -
			       (keo_ie_pos - ndev_vif->sta.assoc_req_add_info_elem)) > 2) {
				keo_ie_pos = cfg80211_find_vendor_ie(WLAN_OUI_SAMSUNG, WLAN_OUI_TYPE_SAMSUNG_KEO,
								     keo_ie_pos,
								     ndev_vif->sta.assoc_req_add_info_elem_len -
								     (keo_ie_pos - ndev_vif->sta.assoc_req_add_info_elem));
				if (!keo_ie_pos)
					break;
				if ((ie_buf_len - ie_len) >= ((int)keo_ie_pos[1] + 2)) {
					memcpy(&ie_buf[ie_len], keo_ie_pos, (keo_ie_pos[1] + 2));
					ie_len += (keo_ie_pos[1] + 2);
					keo_ie_pos += (keo_ie_pos[1] + 2);
				} else {
					SLSI_NET_ERR(dev, "ie_buf buffer overflow!\n");
					kfree(ie_buf);
					ie_buf = NULL;
					SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
					return -EINVAL;
				}
			}
			r = slsi_mlme_add_info_elements(sdev, dev, FAPI_PURPOSE_ASSOCIATION_REQUEST, ie_buf, ie_len);
		} else {
			r = slsi_mlme_add_info_elements(sdev, dev, FAPI_PURPOSE_ASSOCIATION_REQUEST, ftie->ie, ftie->ie_len);
		}
		kfree(ie_buf);
		ie_buf = NULL;
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

#ifdef CONFIG_SCSC_WLAN_MAC_ACL_PER_MAC
int slsi_set_mac_acl_per_mac(struct wiphy *wiphy, struct net_device *dev,
			     const struct cfg80211_acl_data *params)
{
	struct slsi_dev          *sdev           = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif        *ndev_vif       = netdev_priv(dev);
	int                      r               = 0;
	int                      i               = 0;
	int                      malloc_len      = 0;
	struct cfg80211_acl_data *saved_acl_data = NULL;
	int                      last_index      = 0;
	struct mac_address       zero_addr       = {0};
	bool                     found_flag      = false;

	if (slsi_is_test_mode_enabled()) {
		SLSI_NET_INFO(dev, "Skip sending signal, WlanLite FW does not support MLME_SET_ACL.request\n");
		return -EOPNOTSUPP;
	}
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (ndev_vif->vif_type != FAPI_VIFTYPE_AP) {
		SLSI_NET_ERR(dev, "Invalid vif type: %d\n", ndev_vif->vif_type);
		r = -EINVAL;
		goto exit;
	}
	SLSI_NET_DBG2(dev, SLSI_CFG80211, "ACL:: Policy: %d  Number of stations: %d\n", params->acl_policy, params->n_acl_entries);

	if (params->n_acl_entries != 1) {
		SLSI_NET_ERR(dev, "n_acl_entries != 1, No action taken\n");
		goto exit;
	}

	saved_acl_data = ndev_vif->ap.acl_data_blacklist;
	if (!saved_acl_data) {
		if (params->acl_policy == NL80211_ACL_POLICY_DENY_UNLESS_LISTED) {
			SLSI_NET_ERR(dev, "Deletion requested on empty list\n");
			r = -EINVAL;
			goto exit;
		}
		malloc_len = sizeof(struct cfg80211_acl_data) + sizeof(struct mac_address) * SLSI_ACL_MAX_BSSID_COUNT;
		saved_acl_data = kmalloc(malloc_len, GFP_KERNEL);
		if (!saved_acl_data) {
			SLSI_ERR(sdev, "Memory Allocation failure for ACL List");
			r = -ENOMEM;
			goto exit;
		}
		memset(saved_acl_data, 0, malloc_len);
		ndev_vif->ap.acl_data_blacklist = saved_acl_data;
	}

	last_index = saved_acl_data->n_acl_entries;
	if (params->acl_policy == NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED) {	/* Add mac address on the blacklist */
		if (SLSI_ETHER_EQUAL(params->mac_addrs[0].addr, zero_addr.addr)) {
			SLSI_NET_ERR(dev, "Addition of addr 00:00:00:00:00:00 in blacklist\n");
			r = -EINVAL;
			goto exit;
		}
		for (i = 0 ; i < last_index; i++) { /*Check for duplicate entries*/
			if (SLSI_ETHER_EQUAL(saved_acl_data->mac_addrs[i].addr, params->mac_addrs[0].addr)) {
				SLSI_NET_INFO(dev, "Mac addr already present in blacklist\n");
				r = 0;
				goto exit;
			}
		}
		for (i = 0 ; i < last_index + 1 && i < SLSI_ACL_MAX_BSSID_COUNT; i++) {
			if (SLSI_ETHER_EQUAL(saved_acl_data->mac_addrs[i].addr, zero_addr.addr)) {
				SLSI_ETHER_COPY(saved_acl_data->mac_addrs[i].addr, params->mac_addrs[0].addr);
				if (i == last_index)
					last_index = i + 1;
				break;
			}
		}
		if (i == SLSI_ACL_MAX_BSSID_COUNT) {
			SLSI_NET_ERR(dev, "Blacklist is full\n");
			r = -EINVAL;
			goto exit;
		}
	} else if (params->acl_policy == NL80211_ACL_POLICY_DENY_UNLESS_LISTED) {	/* Delete mac address from the blacklist */
		found_flag = false;
		for (i = 0 ; i < last_index; i++) {
			if (SLSI_ETHER_EQUAL(saved_acl_data->mac_addrs[i].addr, params->mac_addrs[0].addr)) {
				SLSI_ETHER_COPY(saved_acl_data->mac_addrs[i].addr, zero_addr.addr);
				found_flag = true;
				if (i == last_index - 1) {
					while (i >= 0 && SLSI_ETHER_EQUAL(saved_acl_data->mac_addrs[i].addr, zero_addr.addr))
						i--;
					last_index = i + 1;
				}
				break;
			}
		}
		if (!found_flag) {
			r = -EINVAL;
			SLSI_NET_ERR(dev, "Deletion requested for addr not present in blacklist");
			goto exit;
		}
	}
	saved_acl_data->n_acl_entries = last_index;
	r = slsi_mlme_set_acl(sdev, dev, ndev_vif->vifnum, saved_acl_data->acl_policy,
			      saved_acl_data->n_acl_entries, saved_acl_data->mac_addrs);
exit:
	if (!last_index) {
		ndev_vif->ap.acl_data_blacklist = NULL;
		kfree(saved_acl_data);
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}
#endif

int slsi_set_mac_acl(struct wiphy *wiphy, struct net_device *dev,
		     const struct cfg80211_acl_data *params)
{
	struct slsi_dev   *sdev = SDEV_FROM_WIPHY(wiphy);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	int               r = 0;

	if (slsi_is_test_mode_enabled()) {
		SLSI_NET_INFO(dev, "Skip sending signal, WlanLite FW does not support MLME_SET_ACL.request\n");
		return -EOPNOTSUPP;
	}
	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	if (FAPI_VIFTYPE_AP != ndev_vif->vif_type) {
		SLSI_NET_ERR(dev, "Invalid vif type: %d\n", ndev_vif->vif_type);
		r = -EINVAL;
		goto exit;
	}
	SLSI_NET_DBG2(dev, SLSI_CFG80211, "ACL:: Policy: %d  Number of stations: %d\n", params->acl_policy, params->n_acl_entries);
	r = slsi_mlme_set_acl(sdev, dev, ndev_vif->vifnum, params->acl_policy, params->n_acl_entries, (struct mac_address *)params->mac_addrs);
	if (r != 0)
		SLSI_NET_ERR(dev, "mlme_set_acl_req returned with CFM failure\n");
exit:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
	return r;
}

static struct cfg80211_ops slsi_ops = {
	.add_virtual_intf = slsi_add_virtual_intf,
	.del_virtual_intf = slsi_del_virtual_intf,
	.change_virtual_intf = slsi_change_virtual_intf,

	.scan = slsi_scan,
	.abort_scan = slsi_abort_scan,
	.connect = slsi_connect,
	.disconnect = slsi_disconnect,

	.add_key = slsi_add_key,
	.del_key = slsi_del_key,
	.change_station = slsi_change_station,
	.get_key = slsi_get_key,
	.set_default_key = slsi_set_default_key,
	.set_default_mgmt_key = slsi_config_default_mgmt_key,

	.set_wiphy_params = slsi_set_wiphy_params,

	.del_station = slsi_del_station,
	.get_station = slsi_get_station,
	.set_tx_power = slsi_set_tx_power,
	.get_tx_power = slsi_get_tx_power,
	.set_power_mgmt = slsi_set_power_mgmt,
	.get_channel = slsi_get_channel,

	.suspend = slsi_suspend,
	.resume = slsi_resume,

	.set_pmksa = slsi_set_pmksa,
	.del_pmksa = slsi_del_pmksa,
	.flush_pmksa = slsi_flush_pmksa,

	.remain_on_channel = slsi_remain_on_channel,
	.cancel_remain_on_channel = slsi_cancel_remain_on_channel,

	.change_bss = slsi_change_bss,

	.start_ap = slsi_start_ap,
	.change_beacon = slsi_change_beacon,
	.stop_ap = slsi_stop_ap,
#if (KERNEL_VERSION(5, 2, 0) < LINUX_VERSION_CODE)
	.update_owe_info = slsi_update_owe_info,
#endif

	.sched_scan_start = slsi_sched_scan_start,
	.sched_scan_stop = slsi_sched_scan_stop,
	.update_connect_params = slsi_update_connect_params,
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 7, 0))
	.mgmt_frame_register = slsi_mgmt_frame_register,
#endif
	.mgmt_tx = slsi_mgmt_tx,
	.mgmt_tx_cancel_wait = slsi_mgmt_tx_cancel_wait,
	.set_txq_params = slsi_set_txq_params,
	.external_auth = slsi_synchronised_response,
#ifdef CONFIG_SCSC_WLAN_MAC_ACL_PER_MAC
	.set_mac_acl = slsi_set_mac_acl_per_mac,
#else
	.set_mac_acl = slsi_set_mac_acl,
#endif
	.update_ft_ies = slsi_update_ft_ies,
	.tdls_oper = slsi_tdls_manager_oper,
	.set_monitor_channel = slsi_set_monitor_channel,
	.set_qos_map = slsi_set_qos_map,
	.channel_switch = slsi_channel_switch,
#ifdef CONFIG_SCSC_WLAN_EHT
	.add_intf_link = slsi_set_intf_link,
#endif
};

#define RATE_LEGACY(_rate, _hw_value, _flags) { \
		.bitrate = (_rate), \
		.hw_value = (_hw_value), \
		.flags = (_flags), \
}

#define CHAN2G(_freq, _idx)  { \
		.band = NL80211_BAND_2GHZ, \
		.center_freq = (_freq), \
		.hw_value = (_idx), \
		.max_power = 17, \
}

#define CHAN5G(_freq, _idx)  { \
		.band = NL80211_BAND_5GHZ, \
		.center_freq = (_freq), \
		.hw_value = (_idx), \
		.max_power = 17, \
}

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
#define CHAN6G(_freq, _idx)  { \
		.band = NL80211_BAND_6GHZ, \
		.center_freq = (_freq), \
		.hw_value = (_idx), \
		.max_power = 17, \
}
#endif

static struct ieee80211_channel slsi_2ghz_channels[] = {
	CHAN2G(2412, 1),
	CHAN2G(2417, 2),
	CHAN2G(2422, 3),
	CHAN2G(2427, 4),
	CHAN2G(2432, 5),
	CHAN2G(2437, 6),
	CHAN2G(2442, 7),
	CHAN2G(2447, 8),
	CHAN2G(2452, 9),
	CHAN2G(2457, 10),
	CHAN2G(2462, 11),
	CHAN2G(2467, 12),
	CHAN2G(2472, 13),
	CHAN2G(2484, 14),
};

static struct ieee80211_rate    slsi_11g_rates[] = {
	RATE_LEGACY(10,  1,  0),
	RATE_LEGACY(20,  2,  IEEE80211_RATE_SHORT_PREAMBLE),
	RATE_LEGACY(55,  3,  IEEE80211_RATE_SHORT_PREAMBLE),
	RATE_LEGACY(110, 6,  IEEE80211_RATE_SHORT_PREAMBLE),
	RATE_LEGACY(60,  4,  0),
	RATE_LEGACY(90,  5,  0),
	RATE_LEGACY(120, 7,  0),
	RATE_LEGACY(180, 8,  0),
	RATE_LEGACY(240, 9,  0),
	RATE_LEGACY(360, 10, 0),
	RATE_LEGACY(480, 11, 0),
	RATE_LEGACY(540, 12, 0),
};

static struct ieee80211_channel slsi_5ghz_channels[] = {
	/* UNII 1 */
	CHAN5G(5180, 36),
	CHAN5G(5200, 40),
	CHAN5G(5220, 44),
	CHAN5G(5240, 48),
	/* UNII 2a */
	CHAN5G(5260, 52),
	CHAN5G(5280, 56),
	CHAN5G(5300, 60),
	CHAN5G(5320, 64),
	/* UNII 2c */
	CHAN5G(5500, 100),
	CHAN5G(5520, 104),
	CHAN5G(5540, 108),
	CHAN5G(5560, 112),
	CHAN5G(5580, 116),
	CHAN5G(5600, 120),
	CHAN5G(5620, 124),
	CHAN5G(5640, 128),
	CHAN5G(5660, 132),
	CHAN5G(5680, 136),
	CHAN5G(5700, 140),
	CHAN5G(5720, 144),
	/* UNII 3 */
	CHAN5G(5745, 149),
	CHAN5G(5765, 153),
	CHAN5G(5785, 157),
	CHAN5G(5805, 161),
	CHAN5G(5825, 165),
#ifdef CONFIG_SCSC_UNII4
	/* UNII 4 */
	CHAN5G(5845, 169),
	CHAN5G(5865, 173),
	CHAN5G(5885, 177),
#endif
};

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
static struct ieee80211_channel slsi_6ghz_channels[] = {
	/* U-NII-5 */
	CHAN6G(5935, 2),
	CHAN6G(5955, 1),
	CHAN6G(5975, 5),
	CHAN6G(5995, 9),
	CHAN6G(6015, 13),
	CHAN6G(6035, 17),
	CHAN6G(6055, 21),
	CHAN6G(6075, 25),
	CHAN6G(6095, 29),
	CHAN6G(6115, 33),
	CHAN6G(6135, 37),
	CHAN6G(6155, 41),
	CHAN6G(6175, 45),
	CHAN6G(6195, 49),
	CHAN6G(6215, 53),
	CHAN6G(6235, 57),
	CHAN6G(6255, 61),
	CHAN6G(6275, 65),
	CHAN6G(6295, 69),
	CHAN6G(6315, 73),
	CHAN6G(6335, 77),
	CHAN6G(6355, 81),
	CHAN6G(6375, 85),
	CHAN6G(6395, 89),
	CHAN6G(6415, 93),
	/* U-NII-6 */
	CHAN6G(6435, 97),
	CHAN6G(6455, 101),
	CHAN6G(6475, 105),
	CHAN6G(6495, 109),
	CHAN6G(6515, 113),
	/* U-NII-7 */
	CHAN6G(6535, 117),
	CHAN6G(6555, 121),
	CHAN6G(6575, 125),
	CHAN6G(6595, 129),
	CHAN6G(6615, 133),
	CHAN6G(6635, 137),
	CHAN6G(6655, 141),
	CHAN6G(6675, 145),
	CHAN6G(6695, 149),
	CHAN6G(6715, 153),
	CHAN6G(6735, 157),
	CHAN6G(6755, 161),
	CHAN6G(6775, 165),
	CHAN6G(6795, 169),
	CHAN6G(6815, 173),
	CHAN6G(6835, 177),
	CHAN6G(6855, 181),
	/* U-NII-8 */
	CHAN6G(6875, 185),
	CHAN6G(6895, 189),
	CHAN6G(6915, 193),
	CHAN6G(6935, 197),
	CHAN6G(6955, 201),
	CHAN6G(6975, 205),
	CHAN6G(6995, 209),
	CHAN6G(7015, 213),
	CHAN6G(7035, 217),
	CHAN6G(7055, 221),
	CHAN6G(7075, 225),
	CHAN6G(7095, 229),
	CHAN6G(7115, 233),
};
#endif

/* note fw_rate_idx_to_host_11a_idx[] below must change if this table changes */

static struct ieee80211_rate       wifi_11a_rates[] = {
	RATE_LEGACY(60,  4,  0),
	RATE_LEGACY(90,  5,  0),
	RATE_LEGACY(120, 7,  0),
	RATE_LEGACY(180, 8,  0),
	RATE_LEGACY(240, 9,  0),
	RATE_LEGACY(360, 10, 0),
	RATE_LEGACY(480, 11, 0),
	RATE_LEGACY(540, 12, 0),
};

static struct ieee80211_sta_ht_cap slsi_ht_cap = {
	.ht_supported       = true,
	.cap                = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
			      IEEE80211_HT_CAP_LDPC_CODING |
			      IEEE80211_HT_CAP_RX_STBC |
			      IEEE80211_HT_CAP_GRN_FLD |
			      IEEE80211_HT_CAP_SGI_20 |
			      IEEE80211_HT_CAP_SGI_40,
	.ampdu_factor       = IEEE80211_HT_MAX_AMPDU_64K,
	.ampdu_density      = IEEE80211_HT_MPDU_DENSITY_4,
	.mcs                = {
		.rx_mask    = { 0xff, 0, },
		.rx_highest = cpu_to_le16(0),
		.tx_params  = 0,
	},
};

struct ieee80211_sta_vht_cap       slsi_vht_cap = {
	.vht_supported = true,
	.cap = IEEE80211_VHT_CAP_MAX_MPDU_LENGTH_7991 |
	       IEEE80211_VHT_CAP_SHORT_GI_80 |
	       IEEE80211_VHT_CAP_RXSTBC_1 |
	       IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE |
	       (5 << IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT),
	.vht_mcs = {
		.rx_mcs_map = cpu_to_le16(0xfffe),
		.rx_highest = cpu_to_le16(0),
		.tx_mcs_map = cpu_to_le16(0xfffe),
		.tx_highest = cpu_to_le16(0),
	},
};

#if defined(CONFIG_SCSC_WLAN_HE) || defined(CONFIG_SCSC_WLAN_SUPPORT_6G)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static struct ieee80211_sband_iftype_data slsi_he_cap[] = {
	{
		.types_mask = BIT(NL80211_IFTYPE_STATION) |
			BIT(NL80211_IFTYPE_P2P_CLIENT) |
			BIT(NL80211_IFTYPE_P2P_DEVICE),
		.he_cap = {
			.has_he = true,
			.he_cap_elem = {
				.mac_cap_info[0] =
						IEEE80211_HE_MAC_CAP0_HTC_HE,
				.mac_cap_info[1] =
						IEEE80211_HE_MAC_CAP1_TF_MAC_PAD_DUR_16US,
				.mac_cap_info[3] =
						IEEE80211_HE_MAC_CAP3_OMI_CONTROL,
				.phy_cap_info[0] =
						IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G |
						IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G |
						IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_160MHZ_IN_5G |
						IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_RU_MAPPING_IN_2G,
				.phy_cap_info[1] =
						IEEE80211_HE_PHY_CAP1_DEVICE_CLASS_A |
						IEEE80211_HE_PHY_CAP1_LDPC_CODING_IN_PAYLOAD |
						IEEE80211_HE_PHY_CAP1_HE_LTF_AND_GI_FOR_HE_PPDUS_0_8US,
				.phy_cap_info[2] =
						IEEE80211_HE_PHY_CAP2_NDP_4x_LTF_AND_3_2US,
				.phy_cap_info[6] =
						IEEE80211_HE_PHY_CAP6_PPE_THRESHOLD_PRESENT,
				.phy_cap_info[7] =
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
						IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_SUPP |
#else
						IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_AR |
#endif
						IEEE80211_HE_PHY_CAP7_MAX_NC_1,
				.phy_cap_info[9] =
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))
						IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_16US |
#endif
						IEEE80211_HE_PHY_CAP9_LONGER_THAN_16_SIGB_OFDM_SYM |
						IEEE80211_HE_PHY_CAP9_TX_1024_QAM_LESS_THAN_242_TONE_RU |
						IEEE80211_HE_PHY_CAP9_RX_1024_QAM_LESS_THAN_242_TONE_RU,
			},
			.he_mcs_nss_supp = {
				.rx_mcs_80 = cpu_to_le16(0xfffa),
				.tx_mcs_80 = cpu_to_le16(0xfffa),
				.rx_mcs_160 = cpu_to_le16(0xfffa),
				.tx_mcs_160 = cpu_to_le16(0xfffa),
			},
			.ppe_thres = { 0x79, 0x1C, 0xC7, 0x71, 0x1C, 0xC7, 0x71 },
		},
		.he_6ghz_capa = {
			.capa = 0,
		},
	},
	{
		.types_mask = BIT(NL80211_IFTYPE_AP) |
			      BIT(NL80211_IFTYPE_P2P_GO),
		.he_cap = {
			.has_he = true,
			.he_cap_elem = {
				.mac_cap_info[0] =
						IEEE80211_HE_MAC_CAP0_HTC_HE,
				.mac_cap_info[1] =
						IEEE80211_HE_MAC_CAP1_TF_MAC_PAD_DUR_16US,
				.mac_cap_info[3] =
						IEEE80211_HE_MAC_CAP3_OMI_CONTROL,
				.phy_cap_info[0] =
						IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_IN_2G |
						IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_40MHZ_80MHZ_IN_5G |
						IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_160MHZ_IN_5G |
						IEEE80211_HE_PHY_CAP0_CHANNEL_WIDTH_SET_RU_MAPPING_IN_2G,
				.phy_cap_info[1] =
						IEEE80211_HE_PHY_CAP1_DEVICE_CLASS_A |
						IEEE80211_HE_PHY_CAP1_LDPC_CODING_IN_PAYLOAD |
						IEEE80211_HE_PHY_CAP1_HE_LTF_AND_GI_FOR_HE_PPDUS_0_8US,
				.phy_cap_info[2] =
						IEEE80211_HE_PHY_CAP2_NDP_4x_LTF_AND_3_2US,
				.phy_cap_info[6] =
						IEEE80211_HE_PHY_CAP6_PPE_THRESHOLD_PRESENT,
				.phy_cap_info[7] =
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 13, 0))
						IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_SUPP |
#else
						IEEE80211_HE_PHY_CAP7_POWER_BOOST_FACTOR_AR |
#endif
						IEEE80211_HE_PHY_CAP7_MAX_NC_1,
				.phy_cap_info[9] =
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))
						IEEE80211_HE_PHY_CAP9_NOMIMAL_PKT_PADDING_16US |
#endif
						IEEE80211_HE_PHY_CAP9_LONGER_THAN_16_SIGB_OFDM_SYM |
						IEEE80211_HE_PHY_CAP9_TX_1024_QAM_LESS_THAN_242_TONE_RU |
						IEEE80211_HE_PHY_CAP9_RX_1024_QAM_LESS_THAN_242_TONE_RU,
			},
			.he_mcs_nss_supp = {
				.rx_mcs_80 = cpu_to_le16(0xfffa),
				.tx_mcs_80 = cpu_to_le16(0xfffa),
				.rx_mcs_160 = cpu_to_le16(0xfffa),
				.tx_mcs_160 = cpu_to_le16(0xfffa),
			},
			.ppe_thres = { 0x79, 0x1C, 0xC7, 0x71, 0x1C, 0xC7, 0x71 },
		},
		.he_6ghz_capa = {
			.capa = 0,
		},
	},
};
#endif
#endif

#if defined(CONFIG_SCSC_WLAN_EHT) && (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
static const struct wiphy_iftype_ext_capab slsi_ext_capab[] = {
	{
		.iftype = BIT(NL80211_IFTYPE_STATION) |
			BIT(NL80211_IFTYPE_P2P_CLIENT) |
			BIT(NL80211_IFTYPE_P2P_DEVICE),
		.eml_capabilities = 0x0000,
		.mld_capa_and_ops = 0x0000,
	},
	{
		.iftype = BIT(NL80211_IFTYPE_AP) |
			  BIT(NL80211_IFTYPE_P2P_GO),
		.eml_capabilities = 0x0000,
		.mld_capa_and_ops = 0x0000,
	},
};
#endif

struct ieee80211_supported_band    slsi_band_2ghz = {
	.channels   = slsi_2ghz_channels,
	.band       = NL80211_BAND_2GHZ,
	.n_channels = ARRAY_SIZE(slsi_2ghz_channels),
	.bitrates   = slsi_11g_rates,
	.n_bitrates = ARRAY_SIZE(slsi_11g_rates),
};

struct ieee80211_supported_band    slsi_band_5ghz = {
	.channels   = slsi_5ghz_channels,
	.band       = NL80211_BAND_5GHZ,
	.n_channels = ARRAY_SIZE(slsi_5ghz_channels),
	.bitrates   = wifi_11a_rates,
	.n_bitrates = ARRAY_SIZE(wifi_11a_rates),
};

#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
struct ieee80211_supported_band    slsi_band_6ghz = {
	.channels   = slsi_6ghz_channels,
	.band       = NL80211_BAND_6GHZ,
	.n_channels = ARRAY_SIZE(slsi_6ghz_channels),
	.bitrates   = wifi_11a_rates,
	.n_bitrates = ARRAY_SIZE(wifi_11a_rates),
	.ht_cap = {
		.ht_supported = false,
	},
	.vht_cap = {
		.vht_supported = false,
	},
};
#endif

static const u32                   slsi_cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
	WLAN_CIPHER_SUITE_AES_CMAC,
	WLAN_CIPHER_SUITE_SMS4,
	WLAN_CIPHER_SUITE_PMK,
	WLAN_CIPHER_SUITE_GCMP,
	WLAN_CIPHER_SUITE_GCMP_256,
	WLAN_CIPHER_SUITE_CCMP_256,
	WLAN_CIPHER_SUITE_BIP_GMAC_128,
	WLAN_CIPHER_SUITE_BIP_GMAC_256
};

static const struct ieee80211_txrx_stypes
				   ieee80211_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_AP] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		      BIT(IEEE80211_STYPE_AUTH >> 4)
	},
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		      BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		      BIT(IEEE80211_STYPE_AUTH >> 4)
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		      BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		      BIT(IEEE80211_STYPE_AUTH >> 4)
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		      BIT(IEEE80211_STYPE_AUTH >> 4)
	},
};

/* Interface combinations supported by driver */
static struct ieee80211_iface_limit       iface_limits[] = {
#ifdef CONFIG_SCSC_WLAN_STA_ONLY
	/* Basic STA-only */
	{
		.max = CONFIG_SCSC_WLAN_MAX_INTERFACES,
		.types = BIT(NL80211_IFTYPE_STATION),
	},
#else
	/* AP mode: # AP <= 1 on channel = 1 */
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_AP),
	},
	/* STA and P2P mode: #STA <= 1, #{P2P-client,P2P-GO} <= 1 on two channels */
	/* For P2P, the device mode and group mode is first started as STATION and then changed.
	 * Similarly it is changed to STATION on group removal. Hence set maximum interfaces for STATION.
	 */
	{
		.max = CONFIG_SCSC_WLAN_MAX_INTERFACES,
		.types = BIT(NL80211_IFTYPE_STATION),
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_CLIENT) | BIT(NL80211_IFTYPE_P2P_GO),
	},
	/* ADHOC mode: #ADHOC <= 1 on channel = 1 */
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_ADHOC),
	},
#endif
};

static struct ieee80211_regdomain         slsi_regdomain = {
	.reg_rules = {
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
		REG_RULE(0, 0, 0, 0, 0, 0),
	}
};

static struct ieee80211_iface_combination iface_comb[] = {
	{
		.limits = iface_limits,
		.n_limits = ARRAY_SIZE(iface_limits),
		.num_different_channels = 2,
		.max_interfaces = CONFIG_SCSC_WLAN_MAX_INTERFACES,
	},
};

#ifdef CONFIG_PM
static struct cfg80211_wowlan slsi_wowlan_config = {
	.any = true,
};
#endif

#ifdef CONFIG_SCSC_WLAN_EHT
static void slsi_fill_wiphy_eml_mld_cap(struct slsi_dev *sdev)
{
	u16 ml_ctrl_field = 0;
	u8 cmn_info_len = 0;
	u8 ml_cap_pos = 9;
	u16 *eml_cap = NULL;
	u16 *mld_capa = NULL;
	u8 *fw_ml_cap = NULL;
	u8 i = 0;

	for (i = 0; i < ARRAY_SIZE(slsi_ext_capab); i++) {
		eml_cap = (u16 *)&slsi_ext_capab[i].eml_capabilities;
		mld_capa = (u16 *)&slsi_ext_capab[i].mld_capa_and_ops;

		if (slsi_ext_capab[i].iftype & BIT(NL80211_IFTYPE_STATION)) {
			ml_ctrl_field = le16_to_cpu(*(u16 *)sdev->fw_sta_basic_ml_cap);
			cmn_info_len = sdev->fw_sta_basic_ml_cap[2];
			fw_ml_cap = sdev->fw_sta_basic_ml_cap;
		} else if (slsi_ext_capab[i].iftype & BIT(NL80211_IFTYPE_AP)) {
			ml_ctrl_field = le16_to_cpu(*(u16 *)sdev->fw_mhs_basic_ml_cap);
			cmn_info_len = sdev->fw_mhs_basic_ml_cap[2];
			fw_ml_cap = sdev->fw_mhs_basic_ml_cap;
		}

		if (cmn_info_len < 7) {
			SLSI_ERR(sdev, "Invalid Common info element length (%d)",
				 cmn_info_len);
		}

		if (ml_ctrl_field & BIT(SLSI_ML_CTRL_FLD_LINK_ID))
			ml_cap_pos++;

		if (ml_ctrl_field & BIT(SLSI_ML_CTRL_FLD_BSS_PARAM_CHANGE_CNT))
			ml_cap_pos++;

		if (ml_ctrl_field & BIT(SLSI_ML_CTRL_FLD_SYNC_DELAY))
			ml_cap_pos += 2;

		if (fw_ml_cap && (ml_ctrl_field & BIT(SLSI_ML_CTRL_FLD_EML_CAP))) {
			*eml_cap = le16_to_cpu(*(u16 *)&fw_ml_cap[ml_cap_pos]);
			ml_cap_pos += 2;
		}

		if (fw_ml_cap && (ml_ctrl_field & BIT(SLSI_ML_CTRL_FLD_MLD_CAP_OPT)))
			*mld_capa = le16_to_cpu(*(u16 *)&fw_ml_cap[ml_cap_pos]);
	}
}

static void slsi_update_wiphy_eht(struct slsi_dev *sdev)
{
	u8 i = 0;

	for (i = 0; i < ARRAY_SIZE(slsi_he_cap); i++) {
		if (!sdev->fw_sta_eht_supported) {
			/* if EHT is not supported then MLO won't be supported*/
			SLSI_ERR(sdev, "fw doesn't support sta eht: %d\n",
				 sdev->fw_sta_eht_supported);
			break;
		}
		slsi_he_cap[i].eht_cap.has_eht = true;
		memcpy(&(slsi_he_cap[i].eht_cap.eht_cap_elem), sdev->fw_sta_eht_cap,
		       sizeof(struct ieee80211_eht_cap_elem_fixed));
		SLSI_INFO(sdev, "EHT -> cap:0x%08x\n", slsi_he_cap[i].eht_cap.eht_cap_elem);

		memcpy(&(slsi_he_cap[i].eht_cap.eht_mcs_nss_supp), &sdev->fw_sta_eht_cap[11],
		       sizeof(struct ieee80211_eht_mcs_nss_supp));
	}

	for (i = 0; i < ARRAY_SIZE(slsi_he_cap); i++) {
		if (!sdev->fw_mhs_eht_supported) {
			/* if EHT is not supported then MLO won't be supported*/
			SLSI_ERR(sdev, "fw doesn't support mhs eht: %d\n",
				 sdev->fw_mhs_eht_supported);
			break;
		}

		if (!(slsi_he_cap[i].types_mask & BIT(NL80211_IFTYPE_AP)))
			continue;

		slsi_he_cap[i].eht_cap.has_eht = true;
		memcpy(&(slsi_he_cap[i].eht_cap.eht_cap_elem), sdev->fw_mhs_eht_cap,
		       sizeof(struct ieee80211_eht_cap_elem_fixed));
		SLSI_INFO(sdev, "SoftAP EHT -> cap:0x%08x\n", slsi_he_cap[i].eht_cap.eht_cap_elem);

		memcpy(&(slsi_he_cap[i].eht_cap.eht_mcs_nss_supp), &sdev->fw_mhs_eht_cap[11],
		       sizeof(struct ieee80211_eht_mcs_nss_supp));
	}
}

static bool slsi_update_wiphy_eht_mlo(struct wiphy *wiphy, struct slsi_dev *sdev)
{
	slsi_update_wiphy_eht(sdev);
	if (!sdev->fw_mlo_supported) {
		SLSI_ERR(sdev, "fw doesn't support softap_mlo: %d\n", sdev->fw_mlo_supported);
		return false;
	}
	slsi_fill_wiphy_eml_mld_cap(sdev);
	return true;
}
#endif

struct slsi_dev                           *slsi_cfg80211_new(struct device *dev)
{
	struct wiphy    *wiphy;
	struct slsi_dev *sdev = NULL;

	SLSI_DBG1_NODEV(SLSI_CFG80211, "wiphy_new()\n");
	wiphy = wiphy_new(&slsi_ops, sizeof(struct slsi_dev));
	if (!wiphy) {
		SLSI_ERR_NODEV("wiphy_new() failed");
		return NULL;
	}

	sdev = (struct slsi_dev *)wiphy->priv;

	sdev->wiphy = wiphy;

	set_wiphy_dev(wiphy, dev);

	/* Allow changing of the netns, if NOT set then no changes are allowed */
	wiphy->flags |= WIPHY_FLAG_NETNS_OK;
	wiphy->flags |= WIPHY_FLAG_PS_ON_BY_DEFAULT;
	wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;
#if !(defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION < 11)
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM;
#endif
	wiphy->flags |= WIPHY_FLAG_HAS_CHANNEL_SWITCH;
	wiphy->max_num_csa_counters = 2;

	wiphy->flags |= WIPHY_FLAG_HAVE_AP_SME |
			WIPHY_FLAG_AP_PROBE_RESP_OFFLOAD |
			WIPHY_FLAG_AP_UAPSD;

	wiphy->max_acl_mac_addrs = SLSI_AP_PEER_CONNECTIONS_MAX;

	wiphy->privid = sdev;

	wiphy->interface_modes =
#ifdef CONFIG_SCSC_WLAN_STA_ONLY
		BIT(NL80211_IFTYPE_STATION);
#else
		BIT(NL80211_IFTYPE_P2P_GO) |
		BIT(NL80211_IFTYPE_P2P_CLIENT) |
		BIT(NL80211_IFTYPE_STATION) |
		BIT(NL80211_IFTYPE_AP) |
		BIT(NL80211_IFTYPE_MONITOR) |
		BIT(NL80211_IFTYPE_ADHOC) |
		BIT(NL80211_IFTYPE_AP_VLAN);
#endif
	slsi_band_2ghz.ht_cap = slsi_ht_cap;
	slsi_band_5ghz.ht_cap = slsi_ht_cap;
	slsi_band_5ghz.vht_cap = slsi_vht_cap;

#ifdef CONFIG_SCSC_WLAN_HE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	slsi_band_2ghz.iftype_data = slsi_he_cap;
	slsi_band_2ghz.n_iftype_data = ARRAY_SIZE(slsi_he_cap);
	slsi_band_5ghz.iftype_data = slsi_he_cap;
	slsi_band_5ghz.n_iftype_data = ARRAY_SIZE(slsi_he_cap);
#endif
#endif

	wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

	wiphy->bands[NL80211_BAND_2GHZ] = &slsi_band_2ghz;
	wiphy->bands[NL80211_BAND_5GHZ] = &slsi_band_5ghz;

	memset(&sdev->device_config, 0, sizeof(struct slsi_dev_config));
	sdev->device_config.band_5G = &slsi_band_5ghz;
	sdev->device_config.band_2G = &slsi_band_2ghz;
	sdev->device_config.domain_info.regdomain = &slsi_regdomain;

	wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	wiphy->max_remain_on_channel_duration = 5000;         /* 5000 msec */

	wiphy->cipher_suites = slsi_cipher_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(slsi_cipher_suites);

	wiphy->extended_capabilities = slsi_extended_cap;
	wiphy->extended_capabilities_mask = slsi_extended_cap_mask;
	wiphy->extended_capabilities_len = ARRAY_SIZE(slsi_extended_cap);

	wiphy->mgmt_stypes = ieee80211_default_mgmt_stypes;

	/* Driver interface combinations */
	wiphy->n_iface_combinations = ARRAY_SIZE(iface_comb);
	wiphy->iface_combinations = iface_comb;

	/* Basic scan parameters */
	wiphy->max_scan_ssids = 10;
	wiphy->max_scan_ie_len = 2048;

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(4, 11, 0))
	/* Scheduled scanning support */
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
	/* Parameters for Scheduled Scanning Support */
	wiphy->max_sched_scan_reqs = 1;
	/* Setting the default scheduled scan plan to 1 */
	wiphy->max_sched_scan_plans = 1;
#ifdef CONFIG_SCSC_WLAN_EXPONENTIAL_SCHED_SCAN
	wiphy->max_sched_scan_plans = 2;
	wiphy->max_sched_scan_plan_interval = 60;
	wiphy->max_sched_scan_plan_iterations = 3;
#endif
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_SCHED_SCAN_RELATIVE_RSSI);

	/* Randomize TA of Public Action frames. */
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_MGMT_TX_RANDOM_TA);
#endif

#if !defined(__x86_64__)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	/* Beacon Protection */
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_BEACON_PROTECTION);
#if defined(CONFIG_SCSC_WLAN_OCV_SUPPORT) || (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	/* Operating Channel Validation (OCV) */
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_OPERATING_CHANNEL_VALIDATION);
#endif
#endif
#endif

	/* Match the maximum number of SSIDs that could be requested from wpa_supplicant */
	wiphy->max_sched_scan_ssids = 16;

	/* To get a list of SSIDs rather than just the wildcard SSID need to support match sets */
	wiphy->max_match_sets = 16;

	wiphy->max_sched_scan_ie_len = 2048;

#ifdef CONFIG_PM
	wiphy->wowlan = NULL;
	wiphy->wowlan_config = &slsi_wowlan_config;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	wiphy->regulatory_flags |=  REGULATORY_WIPHY_SELF_MANAGED;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	wiphy->regulatory_flags |= (REGULATORY_STRICT_REG |
					REGULATORY_CUSTOM_REG |
					REGULATORY_DISABLE_BEACON_HINTS |
					REGULATORY_COUNTRY_IE_IGNORE);
#else
	wiphy->regulatory_flags |= (REGULATORY_STRICT_REG |
				    REGULATORY_CUSTOM_REG |
				    REGULATORY_DISABLE_BEACON_HINTS);
#endif
#ifndef CONFIG_SCSC_WLAN_STA_ONLY
	/* P2P flags */
	wiphy->flags |= WIPHY_FLAG_OFFCHAN_TX;

	/* Enable Probe response offloading w.r.t WPS and P2P */
	wiphy->probe_resp_offload |=
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS |
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_WPS2 |
		NL80211_PROBE_RESP_OFFLOAD_SUPPORT_P2P;

	/* TDLS support */
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS;
#endif
	/* Mac Randomization */
#ifdef CONFIG_SCSC_WLAN_ENABLE_MAC_RANDOMISATION
	wiphy->features |= NL80211_FEATURE_SCAN_RANDOM_MAC_ADDR;
	wiphy->features |= NL80211_FEATURE_SCHED_SCAN_RANDOM_MAC_ADDR;
#endif
	wiphy->features |= NL80211_FEATURE_SAE;
#ifdef CONFIG_SCSC_WLAN_HE
	/* 11ax related parameters */
	wiphy->support_mbssid = 1;
	wiphy->support_only_he_mbssid = 1;
#endif
#ifdef CONFIG_SCSC_WLAN_OCE
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_FILS_MAX_CHANNEL_TIME);
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_ACCEPT_BCAST_PROBE_RESP);
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_OCE_PROBE_REQ_HIGH_TX_RATE);
	wiphy_ext_feature_set(wiphy, NL80211_EXT_FEATURE_OCE_PROBE_REQ_DEFERRAL_SUPPRESSION);
	wiphy->flags |= NL80211_SCAN_FLAG_FILS_MAX_CHANNEL_TIME;
	wiphy->flags |= NL80211_SCAN_FLAG_ACCEPT_BCAST_PROBE_RESP;
	wiphy->flags |= NL80211_SCAN_FLAG_OCE_PROBE_REQ_HIGH_TX_RATE;
	wiphy->flags |= NL80211_SCAN_FLAG_OCE_PROBE_REQ_DEFERRAL_SUPPRESSION;
#endif

#ifdef CONFIG_SCSC_WLAN_EHT
	wiphy->flags |= WIPHY_FLAG_SUPPORTS_MLO;
	wiphy->iftype_ext_capab = &slsi_ext_capab[0];
	wiphy->num_iftype_ext_capab = 1;
#endif

	wiphy->software_iftypes |= BIT(NL80211_IFTYPE_AP_VLAN);

	return sdev;
}

int slsi_cfg80211_register(struct slsi_dev *sdev)
{
	SLSI_DBG1(sdev, SLSI_CFG80211, "wiphy_register()\n");
	return wiphy_register(sdev->wiphy);
}

void slsi_cfg80211_unregister(struct slsi_dev *sdev)
{
#ifdef CONFIG_PM
	sdev->wiphy->wowlan = NULL;
	sdev->wiphy->wowlan_config = NULL;
#endif
	SLSI_DBG1(sdev, SLSI_CFG80211, "wiphy_unregister()\n");
	wiphy_unregister(sdev->wiphy);
}

void slsi_cfg80211_free(struct slsi_dev *sdev)
{
	SLSI_DBG1(sdev, SLSI_CFG80211, "wiphy_free()\n");
	wiphy_free(sdev->wiphy);
}

static bool slsi_update_wiphy_6ghz(struct slsi_dev *sdev)
{
#ifdef CONFIG_SCSC_WLAN_SUPPORT_6G
	bool split_scan_enabled = false;
	u16 he_6ghz_capa = 0;
	u32 i;

	if (!sdev->band_6g_supported) {
		sdev->wiphy->bands[NL80211_BAND_6GHZ] = NULL;
		sdev->device_config.band_6G = NULL;
		sdev->wiphy->flags &= ~WIPHY_FLAG_SPLIT_SCAN_6GHZ;
		return false;
	}

	he_6ghz_capa = le16_to_cpu(*(u16 *)sdev->fw_he_cap);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	for (i = 0; i < ARRAY_SIZE(slsi_he_cap); i++)
		slsi_he_cap[i].he_6ghz_capa.capa = cpu_to_le16(he_6ghz_capa);

	slsi_band_6ghz.iftype_data = slsi_he_cap;
	slsi_band_6ghz.n_iftype_data = ARRAY_SIZE(slsi_he_cap);
#endif

	sdev->wiphy->bands[NL80211_BAND_6GHZ] = &slsi_band_6ghz;
	sdev->device_config.band_6G = &slsi_band_6ghz;
	sdev->device_config.supported_band = SLSI_FREQ_BAND_AUTO;
	sdev->device_config.supported_roam_band |= FAPI_BAND_6GHZ;

	split_scan_enabled = slsi_dev_6ghz_split_scan_enabled();
	if (split_scan_enabled)
		sdev->wiphy->flags |= WIPHY_FLAG_SPLIT_SCAN_6GHZ;
	else
		sdev->wiphy->flags &= ~WIPHY_FLAG_SPLIT_SCAN_6GHZ;

	SLSI_INFO(sdev, "6GHz split scan enable : %d (he_6ghz_capa : 0x%x)\n", split_scan_enabled, he_6ghz_capa);
	return true;
#else
	return false;
#endif
}

void slsi_cfg80211_update_wiphy(struct slsi_dev *sdev)
{
	bool support_6ghz = false;

	/* Band 2G probably be disabled by slsi_band_cfg_update() while factory test or NCHO.
	 * So, we need to make sure that Band 2.4G enabled when initialized. */
	sdev->wiphy->bands[NL80211_BAND_2GHZ] = &slsi_band_2ghz;
	sdev->device_config.band_2G = &slsi_band_2ghz;
	sdev->device_config.supported_roam_band = FAPI_BAND_2_4GHZ;

	/* update supported Bands */
	if (sdev->band_5g_supported) {
		sdev->wiphy->bands[NL80211_BAND_5GHZ] = &slsi_band_5ghz;
		sdev->device_config.band_5G = &slsi_band_5ghz;
		sdev->device_config.supported_band = SLSI_FREQ_BAND_AUTO;
		sdev->device_config.supported_roam_band |= FAPI_BAND_5GHZ;
	} else {
		sdev->wiphy->bands[NL80211_BAND_5GHZ] = NULL;
		sdev->device_config.band_5G = NULL;
		sdev->device_config.supported_band = SLSI_FREQ_BAND_2GHZ;
	}

	/* update HT features */
	if (sdev->fw_ht_enabled) {
		slsi_ht_cap.ht_supported = true;
		slsi_ht_cap.cap = le16_to_cpu(*(u16 *)sdev->fw_ht_cap);
		slsi_ht_cap.ampdu_density = (sdev->fw_ht_cap[2] & IEEE80211_HT_AMPDU_PARM_DENSITY) >> IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT;
		slsi_ht_cap.ampdu_factor = sdev->fw_ht_cap[2] & IEEE80211_HT_AMPDU_PARM_FACTOR;
	} else {
		slsi_ht_cap.ht_supported = false;
	}
	slsi_band_2ghz.ht_cap = slsi_ht_cap;
	slsi_band_5ghz.ht_cap = slsi_ht_cap;

	/* update VHT features */
	if (sdev->fw_vht_enabled) {
		slsi_vht_cap.vht_supported = true;
		slsi_vht_cap.cap = le32_to_cpu(*(u32 *)sdev->fw_vht_cap);
	} else {
		slsi_vht_cap.vht_supported = false;
	}
	slsi_band_5ghz.vht_cap = slsi_vht_cap;

	support_6ghz = slsi_update_wiphy_6ghz(sdev);

	SLSI_INFO(sdev, "BANDS SUPPORTED -> 2.4:'%c' 5:'%c' 6:'%c'\n",
		  sdev->wiphy->bands[NL80211_BAND_2GHZ] ? 'Y' : 'N',
		  sdev->wiphy->bands[NL80211_BAND_5GHZ] ? 'Y' : 'N',
		  support_6ghz ? 'Y' : 'N');

	SLSI_INFO(sdev, "HT/VHT SUPPORTED -> HT:'%c' VHT:'%c'\n", sdev->fw_ht_enabled ? 'Y' : 'N',
		  sdev->fw_vht_enabled ? 'Y' : 'N');
	SLSI_INFO(sdev, "HT  -> cap:0x%04x ampdu_density:%d ampdu_factor:%d\n", slsi_ht_cap.cap, slsi_ht_cap.ampdu_density, slsi_ht_cap.ampdu_factor);
	SLSI_INFO(sdev, "VHT -> cap:0x%08x\n", slsi_vht_cap.cap);
#ifdef CONFIG_SCSC_WLAN_EHT
	SLSI_INFO(sdev, "MLO supported: %d\n", slsi_update_wiphy_eht_mlo(sdev->wiphy, sdev));
#endif
}
