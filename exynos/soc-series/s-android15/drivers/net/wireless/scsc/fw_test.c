/*****************************************************************************
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All rights reserved
 *
 *****************************************************************************/
#include <net/cfg80211.h>
#include <linux/etherdevice.h>
#include "dev.h"
#include "fapi.h"
#include "fw_test.h"
#include "debug.h"
#include "mgt.h"
#include "mlme.h"
#include "netif.h"
#include "ba.h"
#include "sap_mlme.h"
#ifdef CONFIG_SCSC_WLAN_TX_API
#include "tx_api.h"
#endif
#include <scsc/scsc_warn.h>

static void slsi_fw_test_save_frame(struct slsi_dev *sdev, struct slsi_fw_test *fwtest, struct sk_buff *saved_skbs[CONFIG_SCSC_WLAN_MAX_INTERFACES + 1], struct sk_buff *skb, bool udi_header)
{
	u16 vif;

	skb = skb_copy(skb, GFP_KERNEL);

	if (udi_header)
		skb_pull(skb, sizeof(struct udi_msg_t));

	vif = fapi_get_vif(skb);

	SLSI_DBG3(sdev, SLSI_FW_TEST, "sig:0x%.4X, vif:%d\n", fapi_get_sigid(skb), vif);
	slsi_debug_frame(sdev, NULL, skb, "SAVE");

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	kfree_skb(saved_skbs[vif]);
	saved_skbs[vif] = skb;
	slsi_spinlock_unlock(&fwtest->fw_test_lock);
}

static void slsi_fw_test_update_vif(struct slsi_dev *sdev, struct net_device *dev, u16 vif, u16 id)
{
	SLSI_DBG1(sdev, SLSI_FW_TEST, "Id: %d ,(vif:%d)\n", vif, id);
	if (id == MLME_ADD_VIF_REQ) {
		slsi_mlme_assign_vif(sdev, dev, vif);
	} else if (id == MLME_DEL_VIF_REQ) {
		slsi_mlme_clear_vif(sdev, dev, vif);
	}
}

static void slsi_fw_test_process_frame(struct slsi_dev *sdev, struct slsi_fw_test *fwtest, struct sk_buff *skb, bool udi_header)
{
	u16 vif;

	skb = skb_copy(skb, GFP_KERNEL);

	if (udi_header)
		skb_pull(skb, sizeof(struct udi_msg_t));

	vif = fapi_get_vif(skb);

	SLSI_DBG3(sdev, SLSI_FW_TEST, "sig:0x%.4X, vif:%d\n", fapi_get_sigid(skb), vif);
	slsi_debug_frame(sdev, NULL, skb, "PROCESS");

	slsi_skb_work_enqueue(&fwtest->fw_test_work, skb);
}

int slsi_fw_test_signal(struct slsi_dev *sdev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	u16 vif = fapi_get_vif(skb);
	struct net_device   *dev;

	/* Atleast one write to via the UDI interface */
	fwtest->fw_test_enabled = true;
	SLSI_DBG3(sdev, SLSI_FW_TEST, "0x%p: sig:0x%.4X, vif:%d\n", skb, fapi_get_sigid(skb), vif);

	if (WLBT_WARN(vif > FAPI_VIFRANGE_VIF_INDEX_MAX, "vif(%d) > FAPI_VIFRANGE_VIF_INDEX_MAX", vif))
		return -EINVAL;
	dev = slsi_get_netdev(sdev, vif);

	switch (fapi_get_sigid(skb)) {
	case MLME_ADD_VIF_REQ:
		SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Save MLME_ADD_VIF_REQ(0x%.4X, vif:%d)\n", skb, fapi_get_sigid(skb), vif);
		slsi_fw_test_update_vif(sdev, dev, vif, MLME_ADD_VIF_REQ);
		slsi_fw_test_save_frame(sdev, fwtest, fwtest->mlme_add_vif_req, skb, false);
		slsi_fw_test_process_frame(sdev, fwtest, skb, false);
		break;
	case MLME_CONNECT_REQ:
		SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Save MLME_CONNECT_REQ(0x%.4X, vif:%d)\n", skb, fapi_get_sigid(skb), vif);
		slsi_fw_test_save_frame(sdev, fwtest, fwtest->mlme_connect_req, skb, false);
		break;
	case MLME_DEL_VIF_REQ:
		SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Save MLME_DEL_VIF_REQ(0x%.4X, vif:%d)\n", skb, fapi_get_sigid(skb), vif);
		slsi_fw_test_process_frame(sdev, fwtest, skb, false);
		slsi_fw_test_update_vif(sdev, dev, vif, MLME_DEL_VIF_REQ);
		break;
	default:
		return 0;
	}

	return 0;
}

int slsi_fw_test_signal_with_udi_header(struct slsi_dev *sdev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct udi_msg_t          *udi_msg = (struct udi_msg_t *)skb->data;
	struct fapi_vif_signal_header *fapi_header = (struct fapi_vif_signal_header *)(skb->data + sizeof(struct udi_msg_t));

	if (!fwtest->fw_test_enabled)
		return 0;

	SLSI_DBG3(sdev, SLSI_FW_TEST, "0x%p: sig:0x%.4X, vif:%d\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));

	if (udi_msg->direction == SLSI_LOG_DIRECTION_TO_HOST) {
		switch (le16_to_cpu(fapi_header->id)) {
		case MLME_DISCONNECTED_IND:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Process MLME_DISCONNECTED_IND(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_process_frame(sdev, fwtest, skb, true);
			break;
		case MLME_CONNECT_IND:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Process MLME_CONNECT_IND(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_process_frame(sdev, fwtest, skb, true);
			break;
		case MLME_CONNECTED_IND:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Process MLME_CONNECTED_IND(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_process_frame(sdev, fwtest, skb, true);
			break;
		case MLME_ROAMED_IND:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Process MLME_ROAMED_IND(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_process_frame(sdev, fwtest, skb, true);
			break;
		case MLME_TDLS_PEER_IND:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Process MLME_TDLS_PEER_IND(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_process_frame(sdev, fwtest, skb, true);
			break;
		case MLME_CONNECT_CFM:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Save MLME_CONNECT_CFM(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_save_frame(sdev, fwtest, fwtest->mlme_connect_cfm, skb, true);
			break;
		case MLME_PROCEDURE_STARTED_IND:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Save MLME_PROCEDURE_STARTED_IND(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_save_frame(sdev, fwtest, fwtest->mlme_procedure_started_ind, skb, true);
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Process MLME_PROCEDURE_STARTED_IND(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_process_frame(sdev, fwtest, skb, true);
			break;
		case MLME_START_CFM:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Process MLME_START_CFM(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_process_frame(sdev, fwtest, skb, true);
			sdev->device_config.ap_disconnect_ind_timeout =  SLSI_DEFAULT_AP_DISCONNECT_IND_TIMEOUT;
			break;
		case MA_BLOCKACKREQ_IND:
			SLSI_DBG2(sdev, SLSI_FW_TEST, "0x%p: Process MA_BLOCKACKREQ_IND(0x%.4X, vif:%d)\n", skb, le16_to_cpu(fapi_header->id), le16_to_cpu(fapi_header->vif));
			slsi_fw_test_process_frame(sdev, fwtest, skb, true);
			break;
		default:
			break;
		}
	}

	return 0;
}

static void slsi_fw_test_connect_station_roam(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif     *ndev_vif = netdev_priv(dev);
	struct slsi_peer      *peer = slsi_get_peer_from_qs(sdev, dev, SLSI_STA_PEER_QUEUESET);
	struct ieee80211_mgmt *mgmt = fapi_get_mgmt(skb);
	struct sk_buff        *mlme_procedure_started_ind;
	u16                    flow_id;

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Station Connect(vif:%d) Roam\n", ndev_vif->ifnum);

	if (WLBT_WARN(!ndev_vif->is_fw_test, "!is_fw_test"))
		return;

	if (WLBT_WARN(!ndev_vif->activated, "Not Activated"))
		return;

	if (WLBT_WARN(ndev_vif->vif_type != FAPI_VIFTYPE_STATION, "Not Station Vif"))
		return;

	if (WLBT_WARN(!peer, "peer not found"))
		return;

	flow_id = fapi_get_u16(skb, u.mlme_roamed_ind.flow_id);

	peer->flow_id = flow_id;
	slsi_spinlock_lock(&fwtest->fw_test_lock);
	mlme_procedure_started_ind = fwtest->mlme_procedure_started_ind[ndev_vif->ifnum];
	fwtest->mlme_procedure_started_ind[ndev_vif->ifnum] = NULL;
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	if (WLBT_WARN(!mlme_procedure_started_ind, "mlme_procedure_started_ind not found"))
		return;

	slsi_rx_ba_stop_all(dev, peer);

	SLSI_ETHER_COPY(peer->address, mgmt->bssid);
	slsi_peer_update_assoc_req(sdev, dev, peer, mlme_procedure_started_ind);
	slsi_peer_update_assoc_rsp(sdev, dev, peer, skb_copy(skb, GFP_KERNEL));
}

static void slsi_fw_test_connect_start_station(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *req;
	struct sk_buff    *cfm;
	struct sk_buff    *ind;
	struct slsi_peer  *peer;
	u8                bssid[ETH_ALEN];

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Station Connect Start(vif:%d)\n", ndev_vif->ifnum);

	if (WLBT_WARN(!ndev_vif->is_fw_test, "!is_fw_test"))
		return;
	if (WLBT_WARN(ndev_vif->activated, "Already Activated"))
		return;

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	req = fwtest->mlme_connect_req[ndev_vif->ifnum];
	cfm = fwtest->mlme_connect_cfm[ndev_vif->ifnum];
	ind = fwtest->mlme_procedure_started_ind[ndev_vif->ifnum];
	if (req)
		SLSI_ETHER_COPY(bssid, fapi_get_buff(req, u.mlme_connect_req.bssid));
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	if (WLBT_WARN(!req, "mlme_connect_req Not found"))
		return;
	if (WLBT_WARN(!cfm, "mlme_connect_cfm Not found"))
		return;

	ndev_vif->iftype = NL80211_IFTYPE_STATION;
	dev->ieee80211_ptr->iftype = NL80211_IFTYPE_STATION;
	ndev_vif->vif_type = FAPI_VIFTYPE_STATION;

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "vif:%d slsi_vif_activated\n", ndev_vif->ifnum);
	if (WLBT_WARN(slsi_vif_activated(sdev, dev) != 0, "slsi_vif_activated() Failed"))
		return;

	slsi_spinlock_lock(&ndev_vif->peer_lock);
	peer = slsi_peer_add(sdev, dev, bssid, SLSI_STA_PEER_QUEUESET + 1);
	if (WLBT_WARN(!peer, "slsi_peer_add(" MACSTR ") Failed", MAC2STR(bssid))) {
		slsi_spinlock_unlock(&ndev_vif->peer_lock);
		slsi_vif_deactivated(sdev, dev);
		return;
	}

	slsi_peer_update_assoc_req(sdev, dev, peer, skb_copy(skb, GFP_KERNEL));
	slsi_spinlock_unlock(&ndev_vif->peer_lock);
}

static void slsi_fw_test_connect_station(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *req;
	struct sk_buff    *cfm;
	struct sk_buff    *ind;
	struct slsi_peer  *peer;
	u16               result;
	u16               flow_id;

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Station Connect(vif:%d)\n", ndev_vif->ifnum);

	if (WLBT_WARN(!ndev_vif->is_fw_test, "!is_fw_test"))
		return;

	result = fapi_get_u16(skb, u.mlme_connect_ind.result_code);
	flow_id = fapi_get_u16(skb, u.mlme_connect_ind.flow_id);

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	req = fwtest->mlme_connect_req[ndev_vif->ifnum];
	cfm = fwtest->mlme_connect_cfm[ndev_vif->ifnum];
	ind = fwtest->mlme_procedure_started_ind[ndev_vif->ifnum];
	fwtest->mlme_connect_req[ndev_vif->ifnum] = NULL;
	fwtest->mlme_connect_cfm[ndev_vif->ifnum] = NULL;
	fwtest->mlme_procedure_started_ind[ndev_vif->ifnum] = NULL;
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	if (WLBT_WARN(!req, "mlme_connect_req Not found"))
		goto exit;
	if (WLBT_WARN(!cfm, "mlme_connect_cfm Not found"))
		goto exit;
	if (FAPI_RESULTCODE_SUCCESS == result &&
	    WLBT_WARN(!ind, "mlme_procedure_started_ind Not found"))
		goto exit;
	if (FAPI_RESULTCODE_SUCCESS != result)
		goto exit;

	if (WLBT_WARN(!ndev_vif->activated, "Not Activated"))
		return;

	peer = slsi_get_peer_from_mac(sdev, dev, fapi_get_buff(req, u.mlme_connect_req.bssid));
	if (WLBT_WARN(!peer, "slsi_get_peer_from_mac(" MACSTR ") Failed",
		      MAC2STR(fapi_get_buff(req, u.mlme_connect_req.bssid))))
		goto exit;

	peer->flow_id = flow_id;
	slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_CONNECTED);
	netif_carrier_on(dev);

exit:
	kfree_skb(req);
	kfree_skb(cfm);
	kfree_skb(ind);
}

static void slsi_fw_test_started_network(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u16               result = fapi_get_u16(skb, u.mlme_start_cfm.result_code);

	SLSI_UNUSED_PARAMETER(fwtest);

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Start Network(vif:%d)\n", ndev_vif->ifnum);

	if (WLBT_WARN(!ndev_vif->is_fw_test, "!is_fw_test"))
		return;
	if (WLBT_WARN(ndev_vif->activated, "Already Activated"))
		return;

	ndev_vif->iftype = NL80211_IFTYPE_AP;
	dev->ieee80211_ptr->iftype = NL80211_IFTYPE_AP;
	ndev_vif->vif_type = FAPI_VIFTYPE_AP;

	if (WLBT_WARN(slsi_vif_activated(sdev, dev) != 0, "slsi_vif_activated() Failed"))
		return;

	if (FAPI_RESULTCODE_SUCCESS == result)
		netif_carrier_on(dev);
}

static void slsi_fw_test_stop_network(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	SLSI_UNUSED_PARAMETER(fwtest);
	SLSI_UNUSED_PARAMETER(skb);

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	if (!ndev_vif->is_fw_test)
		return;

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Stopping Network(vif:%d)\n", ndev_vif->ifnum);

	if (WLBT_WARN(!ndev_vif->activated, "Not Activated"))
		return;

	netif_carrier_off(dev);
	slsi_vif_deactivated(sdev, dev);
}

static void slsi_fw_test_connect_start_ap(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif     *ndev_vif = netdev_priv(dev);
	struct slsi_peer      *peer = NULL;
	struct ieee80211_mgmt *mgmt = fapi_get_mgmt(skb);
	u16                   peer_index;
	u16                   flow_id;

	SLSI_UNUSED_PARAMETER(fwtest);

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Network Peer Connect Start(vif:%d)\n", ndev_vif->ifnum);
	WLBT_WARN(!ndev_vif->is_fw_test, "!is_fw_test");

	if (WLBT_WARN(!ndev_vif->activated, "Not Activated"))
		return;

	if (WLBT_WARN_ON(!ieee80211_is_assoc_req(mgmt->frame_control) &&
		    !ieee80211_is_reassoc_req(mgmt->frame_control)))
		return;
	flow_id = fapi_get_u16(skb, u.mlme_procedure_started_ind.flow_id);
	peer_index = (flow_id >> 8);

	slsi_spinlock_lock(&ndev_vif->peer_lock);
	peer = slsi_peer_add(sdev, dev, mgmt->sa, peer_index);
	if (WLBT_WARN_ON(!peer)) {
		slsi_spinlock_unlock(&ndev_vif->peer_lock);
		return;
	}

	slsi_peer_update_assoc_req(sdev, dev, peer, skb_copy(skb, GFP_KERNEL));
	peer->connected_state = SLSI_STA_CONN_STATE_CONNECTING;
	slsi_spinlock_unlock(&ndev_vif->peer_lock);
}

static void slsi_fw_test_connected_network(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_peer  *peer = NULL;
	u16               flow_id = fapi_get_u16(skb, u.mlme_connected_ind.flow_id);
	u16               aid = (flow_id >> 8);

	SLSI_UNUSED_PARAMETER(fwtest);

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Network Peer Connect(vif:%d, aid:%d)\n", ndev_vif->ifnum, aid);
	WLBT_WARN(!ndev_vif->is_fw_test, "!is_fw_test");

	if (WLBT_WARN(!ndev_vif->activated, "Not Activated"))
		return;

	if (WLBT_WARN_ON(aid < SLSI_PEER_INDEX_MIN))
		return;

	if (WLBT_WARN_ON(aid > SLSI_PEER_INDEX_MAX))
		return;

	peer = slsi_get_peer_from_qs(sdev, dev, aid - 1);
	if (WLBT_WARN(!peer, "Peer(aid:%d) Not Found", aid))
		return;

	peer->flow_id = flow_id;
	slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_CONNECTED);
	peer->connected_state = SLSI_STA_CONN_STATE_CONNECTED;

	slsi_rx_buffered_frames(sdev, dev, peer, 0xFF);
}

/* Setup the NetDev / Peers based on the saved frames */
static void slsi_fw_test_procedure_started_ind(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *add_vif_req;
	u16               viftype = FAPI_VIFTYPE_STATION;

	if (!ndev_vif->is_fw_test) {
		kfree_skb(skb);
		return;
	}

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "ProcedureStarted(vif:%d)\n", ndev_vif->ifnum);

	if (fapi_get_u16(skb, u.mlme_procedure_started_ind.procedure_type) != FAPI_PROCEDURETYPE_CONNECTION_STARTED) {
		kfree_skb(skb);
		return;
	}

	/* Set up the VIF and Data plane ready to go BUT do not open the control port */
	slsi_spinlock_lock(&fwtest->fw_test_lock);
	add_vif_req = fwtest->mlme_add_vif_req[ndev_vif->ifnum];
	if (add_vif_req)
		viftype = fapi_get_u16(add_vif_req, u.mlme_add_vif_req.virtual_interface_type);
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Start UDI test NetDevice(vif:%d)\n", ndev_vif->ifnum);
	if (WLBT_WARN(!add_vif_req, "fwtest->mlme_add_vif_req[ndev_vif->ifnum] == NULL"))
		goto out;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	switch (viftype) {
	case FAPI_VIFTYPE_STATION:
		slsi_fw_test_connect_start_station(sdev, dev, fwtest, skb);
		break;
	case FAPI_VIFTYPE_AP:
		slsi_fw_test_connect_start_ap(sdev, dev, fwtest, skb);
		break;
	default:
		SLSI_NET_DBG1(dev, SLSI_FW_TEST, "vif:%d virtual_interface_type:%d NOT SUPPORTED\n", ndev_vif->ifnum, viftype);
		break;
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

out:
	kfree_skb(skb);
}

/* Setup the NetDev / Peers based on the saved frames */
static void slsi_fw_test_connect_ind(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *add_vif_req;
	u16               viftype = FAPI_VIFTYPE_STATION;

	if (!ndev_vif->is_fw_test) {
		kfree_skb(skb);
		return;
	}

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Network Peer Connect(vif:%d)\n", ndev_vif->ifnum);

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	add_vif_req = fwtest->mlme_add_vif_req[ndev_vif->ifnum];
	if (add_vif_req)
		viftype = fapi_get_u16(add_vif_req, u.mlme_add_vif_req.virtual_interface_type);
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Start UDI test NetDevice(vif:%d)\n", ndev_vif->ifnum);
	if (WLBT_WARN(!add_vif_req, "fwtest->mlme_add_vif_req[ndev_vif->ifnum] == NULL"))
		goto out;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	switch (viftype) {
	case FAPI_VIFTYPE_STATION:
		slsi_fw_test_connect_station(sdev, dev, fwtest, skb);
		break;
	default:
		SLSI_NET_DBG1(dev, SLSI_FW_TEST, "vif:%d virtual_interface_type:%d NOT SUPPORTED\n", ndev_vif->ifnum, viftype);
		break;
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

out:
	kfree_skb(skb);
}

static void slsi_fw_test_connected_ind(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *add_vif_req;
	u16               viftype = FAPI_VIFTYPE_STATION;

	if (!ndev_vif->is_fw_test) {
		kfree_skb(skb);
		return;
	}

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Connected(vif:%d)\n", ndev_vif->ifnum);

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	add_vif_req = fwtest->mlme_add_vif_req[ndev_vif->ifnum];
	if (add_vif_req)
		viftype = fapi_get_u16(add_vif_req, u.mlme_add_vif_req.virtual_interface_type);
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	if (WLBT_WARN(!add_vif_req, "fwtest->mlme_add_vif_req[ndev_vif->ifnum] == NULL"))
		goto out;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	switch (viftype) {
	case FAPI_VIFTYPE_AP:
		slsi_fw_test_connected_network(sdev, dev, fwtest, skb);
		break;
	default:
		SLSI_NET_DBG1(dev, SLSI_FW_TEST, "vif:%d virtual_interface_type:%d NOT SUPPORTED\n", ndev_vif->ifnum, viftype);
		break;
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

out:
	kfree_skb(skb);
}

static void slsi_fw_test_roamed_ind(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *add_vif_req;
	u16               viftype = FAPI_VIFTYPE_STATION;

	if (!ndev_vif->is_fw_test) {
		kfree_skb(skb);
		return;
	}

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Roamed(vif:%d)\n", ndev_vif->ifnum);

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	add_vif_req = fwtest->mlme_add_vif_req[ndev_vif->ifnum];
	if (add_vif_req)
		viftype = fapi_get_u16(add_vif_req, u.mlme_add_vif_req.virtual_interface_type);
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	if (WLBT_WARN(!add_vif_req, "fwtest->mlme_add_vif_req[ndev_vif->ifnum] == NULL"))
		goto out;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	switch (viftype) {
	case FAPI_VIFTYPE_STATION:
		slsi_fw_test_connect_station_roam(sdev, dev, fwtest, skb);
		break;
	default:
		SLSI_NET_DBG1(dev, SLSI_FW_TEST, "vif:%d virtual_interface_type:%d NOT SUPPORTED\n", ndev_vif->ifnum, viftype);
		break;
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

out:
	kfree_skb(skb);
}

static void slsi_fw_test_disconnect_station(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_peer  *peer = slsi_get_peer_from_qs(sdev, dev, SLSI_STA_PEER_QUEUESET);

	SLSI_UNUSED_PARAMETER(fwtest);
	SLSI_UNUSED_PARAMETER(skb);

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	if (!ndev_vif->is_fw_test)
		return;

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Station Disconnect(vif:%d)\n", ndev_vif->ifnum);

	if (WLBT_WARN(!ndev_vif->activated, "Not Activated"))
		return;

	netif_carrier_off(dev);
	if (peer) {
		slsi_spinlock_lock(&ndev_vif->peer_lock);
		slsi_peer_remove(sdev, dev, peer);
		slsi_spinlock_unlock(&ndev_vif->peer_lock);
	}
	slsi_vif_deactivated(sdev, dev);
}

static void slsi_fw_test_disconnect_network(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	/* Find the peer based on MAC address, mlme-disconnect-ind and mlme-disconnected-ind
	 * both have the MAC address in the same position.
	 */
	struct slsi_peer *peer = slsi_get_peer_from_mac(sdev, dev, fapi_get_buff(skb, u.mlme_disconnected_ind.peer_sta_address));

	SLSI_UNUSED_PARAMETER(fwtest);

	WLBT_WARN_ON(!SLSI_MUTEX_IS_LOCKED(ndev_vif->vif_mutex));

	if (!ndev_vif->is_fw_test)
		return;

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Network Peer Disconnect(vif:%d)\n", ndev_vif->ifnum);

	if (peer) {
		slsi_spinlock_lock(&ndev_vif->peer_lock);
		slsi_peer_remove(sdev, dev, peer);
		slsi_spinlock_unlock(&ndev_vif->peer_lock);
	}
}

static void slsi_fw_test_disconnected_ind(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *add_vif_req;
	u16               viftype = FAPI_VIFTYPE_STATION;

	if (!ndev_vif->is_fw_test) {
		kfree_skb(skb);
		return;
	}

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	add_vif_req = fwtest->mlme_add_vif_req[ndev_vif->ifnum];
	if (add_vif_req)
		viftype = fapi_get_u16(add_vif_req, u.mlme_add_vif_req.virtual_interface_type);
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	if (WLBT_WARN(!add_vif_req, "fwtest->mlme_add_vif_req[ndev_vif->ifnum] == NULL"))
		goto out;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	switch (viftype) {
	case FAPI_VIFTYPE_STATION:
		slsi_fw_test_disconnect_station(sdev, dev, fwtest, skb);
		break;
	case FAPI_VIFTYPE_AP:
		slsi_fw_test_disconnect_network(sdev, dev, fwtest, skb);
		break;
	default:
		SLSI_NET_DBG1(dev, SLSI_FW_TEST, "vif:%d virtual_interface_type:%d NOT SUPPORTED\n", ndev_vif->ifnum, viftype);
		break;
	}
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

out:
	kfree_skb(skb);
}

static void slsi_fw_test_tdls_event_connected(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	struct slsi_peer  *peer = NULL;
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	u16				  flow_id = fapi_get_u16(skb, u.mlme_tdls_peer_ind.flow_id);
	u16				  peer_index = (flow_id >> 8);


	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	ndev_vif->sta.tdls_enabled = true;
	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "TDLS connect (vif:%d, peer_index:%d, mac:" MACSTR ")\n",
		      fapi_get_vif(skb), peer_index,
		      MAC2STR(fapi_get_buff(skb, u.mlme_tdls_peer_ind.peer_sta_address)));

	if (ndev_vif->sta.tdls_peer_sta_records + 1 > ndev_vif->sta.tdls_max_peer) {
		SLSI_NET_ERR(dev, "max TDLS limit reached (peer_index:%d)\n", peer_index);
		goto out;
	}

	if (peer_index < SLSI_TDLS_PEER_INDEX_MIN || peer_index > SLSI_TDLS_PEER_INDEX_MAX) {
		SLSI_NET_ERR(dev, "incorrect index (peer_index:%d)\n", peer_index);
		goto out;
	}

	slsi_spinlock_lock(&ndev_vif->peer_lock);
	peer = slsi_peer_add(sdev, dev, fapi_get_buff(skb, u.mlme_tdls_peer_ind.peer_sta_address), peer_index);
	if (!peer) {
		slsi_spinlock_unlock(&ndev_vif->peer_lock);
		SLSI_NET_ERR(dev, "peer add failed\n");
		goto out;
	}

	/* QoS is mandatory for TDLS - enable QoS for TDLS peer by default */
	peer->qos_enabled = true;
	peer->flow_id = flow_id;

	slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_CONNECTED);

#ifdef CONFIG_SCSC_WLAN_TX_API
	slsi_tx_tdls_update(sdev, dev, ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET], peer, true);
#else
	/* move TDLS packets from STA Q to TDLS Q */
	slsi_tdls_move_packets(sdev, dev, ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET], peer, true);
#endif
	slsi_spinlock_unlock(&ndev_vif->peer_lock);
out:
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
}

static void slsi_fw_test_tdls_event_disconnected(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb)
{
	struct slsi_peer  *peer = NULL;
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);
	SLSI_NET_DBG1(dev, SLSI_MLME, "TDLS dis-connect (vif:%d, mac:" MACSTR ")\n",
		      ndev_vif->ifnum,
		      MAC2STR(fapi_get_buff(skb, u.mlme_tdls_peer_ind.peer_sta_address)));

	slsi_spinlock_lock(&ndev_vif->tcp_ack_lock);
	slsi_spinlock_lock(&ndev_vif->peer_lock);
	peer = slsi_get_peer_from_mac(sdev, dev, fapi_get_buff(skb, u.mlme_tdls_peer_ind.peer_sta_address));
	if (!peer || (peer->aid == 0)) {
		WLBT_WARN_ON(!peer || (peer->aid == 0));
		SLSI_NET_DBG1(dev, SLSI_MLME, "can't find peer by MAC address\n");
		goto out;
	}

	slsi_ps_port_control(sdev, dev, peer, SLSI_STA_CONN_STATE_DISCONNECTED);

#ifdef CONFIG_SCSC_WLAN_TX_API
	slsi_tx_tdls_update(sdev, dev, ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET], peer, false);
#else
	/* move TDLS packets from TDLS Q to STA Q */
	slsi_tdls_move_packets(sdev, dev, ndev_vif->peer_sta_record[SLSI_STA_PEER_QUEUESET], peer, false);
#endif
	slsi_peer_remove(sdev, dev, peer);
out:
	slsi_spinlock_unlock(&ndev_vif->peer_lock);
	slsi_spinlock_unlock(&ndev_vif->tcp_ack_lock);
	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);
}

static void slsi_fw_test_tdls_peer_ind(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *add_vif_req;
	u16               vif_type = 0;
	u16 tdls_event;

	if (!ndev_vif->is_fw_test) {
		kfree_skb(skb);
		return;
	}
	if (WLBT_WARN(!ndev_vif->activated, "Not Activated")) {
		kfree_skb(skb);
		return;
	}
	slsi_spinlock_lock(&fwtest->fw_test_lock);
	add_vif_req = fwtest->mlme_add_vif_req[ndev_vif->ifnum];
	if (add_vif_req)
		vif_type = fapi_get_u16(add_vif_req, u.mlme_add_vif_req.virtual_interface_type);
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	if (WLBT_WARN(vif_type != FAPI_VIFTYPE_STATION, "Not STA VIF")) {
		kfree_skb(skb);
		return;
	}

	tdls_event =  fapi_get_u16(skb, u.mlme_tdls_peer_ind.tdls_event);
	SLSI_NET_DBG1(dev, SLSI_MLME, "TDLS peer(vif:%d tdls_event:%d)\n", ndev_vif->ifnum, tdls_event);
	switch (tdls_event) {
	case FAPI_TDLSEVENT_CONNECTED:
		slsi_fw_test_tdls_event_connected(sdev, dev, skb);
		break;
	case FAPI_TDLSEVENT_DISCONNECTED:
		slsi_fw_test_tdls_event_disconnected(sdev, dev, skb);
		break;
	case FAPI_TDLSEVENT_DISCOVERED:
		/* nothing to do */
		break;
	default:
		SLSI_NET_DBG1(dev, SLSI_FW_TEST, "vif:%d tdls_event:%d not supported\n", ndev_vif->ifnum, tdls_event);
		break;
	}
	kfree_skb(skb);
}

/* Setup the NetDev */
static void slsi_fw_test_start_cfm(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *add_vif_req;
	u16               viftype = FAPI_VIFTYPE_DISCOVERY;

	if (!ndev_vif->is_fw_test) {
		kfree_skb(skb);
		return;
	}

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Network Start(vif:%d)\n", ndev_vif->ifnum);

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	add_vif_req = fwtest->mlme_add_vif_req[ndev_vif->ifnum];
	if (add_vif_req)
		viftype = fapi_get_u16(add_vif_req, u.mlme_add_vif_req.virtual_interface_type);
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "Start UDI test NetDevice(vif:%d)\n", ndev_vif->ifnum);
	if (WLBT_WARN(!add_vif_req, "fwtest->mlme_add_vif_req[ndev_vif->ifnum] == NULL"))
		goto out;

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	switch (viftype) {
	case FAPI_VIFTYPE_AP:
		slsi_fw_test_started_network(sdev, dev, fwtest, skb);
		break;
	default:
		SLSI_NET_DBG1(dev, SLSI_FW_TEST, "vif:%d virtual_interface_type:%d NOT SUPPORTED\n", ndev_vif->ifnum, viftype);
		break;
	}

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

out:
	kfree_skb(skb);
}

static void slsi_fw_test_add_vif_req(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	SLSI_UNUSED_PARAMETER(sdev);
	SLSI_UNUSED_PARAMETER(fwtest);

	SLSI_DBG1(sdev, SLSI_FW_TEST, "Mark UDI test NetDevice(vif:%d)\n", fapi_get_vif(skb));
	ndev_vif->is_fw_test = true;
	kfree_skb(skb);
}

static void slsi_fw_test_del_vif_req(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff    *add_vif_req;
	u16               viftype = FAPI_VIFTYPE_DISCOVERY;
	SLSI_DBG1(sdev, SLSI_FW_TEST, "Unmark UDI test NetDevice(vif:%d)\n", fapi_get_vif(skb));

	slsi_spinlock_lock(&fwtest->fw_test_lock);
	add_vif_req = fwtest->mlme_add_vif_req[ndev_vif->ifnum];
	if (add_vif_req)
		viftype = fapi_get_u16(add_vif_req, u.mlme_add_vif_req.virtual_interface_type);
	kfree_skb(fwtest->mlme_add_vif_req[ndev_vif->ifnum]);
	kfree_skb(fwtest->mlme_connect_req[ndev_vif->ifnum]);
	kfree_skb(fwtest->mlme_connect_cfm[ndev_vif->ifnum]);
	kfree_skb(fwtest->mlme_procedure_started_ind[ndev_vif->ifnum]);

	fwtest->mlme_add_vif_req[ndev_vif->ifnum] = NULL;
	fwtest->mlme_connect_req[ndev_vif->ifnum] = NULL;
	fwtest->mlme_connect_cfm[ndev_vif->ifnum] = NULL;
	fwtest->mlme_procedure_started_ind[ndev_vif->ifnum] = NULL;
	slsi_spinlock_unlock(&fwtest->fw_test_lock);

	SLSI_MUTEX_LOCK(ndev_vif->vif_mutex);

	switch (viftype) {
	/* As there is no specific MLME primitive for shutting down the network
	 * perform an actions on the MLME-DEL-VIF.
	 */
	case FAPI_VIFTYPE_AP:
		slsi_fw_test_stop_network(sdev, dev, fwtest, skb);
		break;
	default:
		if (ndev_vif->is_fw_test && ndev_vif->activated) {
			netif_carrier_off(dev);
			slsi_vif_deactivated(sdev, dev);
		}
		break;
	}
	ndev_vif->is_fw_test = false;

	SLSI_MUTEX_UNLOCK(ndev_vif->vif_mutex);

	kfree_skb(skb);
}

static void slsi_fw_test_ma_blockackreq_ind(struct slsi_dev *sdev, struct net_device *dev, struct slsi_fw_test *fwtest, struct sk_buff *skb)
{
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	if (!ndev_vif->is_fw_test) {
		kfree_skb(skb);
		return;
	}

	SLSI_NET_DBG1(dev, SLSI_FW_TEST, "MA Block Ack Req Indication(vif:%d)\n", ndev_vif->ifnum);
	slsi_rx_ma_blockack_ind(sdev, dev, skb);
}

void slsi_fw_test_work(struct work_struct *work)
{
	struct slsi_fw_test *fw_test = container_of(work, struct slsi_fw_test, fw_test_work.work);
	struct slsi_dev     *sdev = fw_test->sdev;
	struct sk_buff      *skb = slsi_skb_work_dequeue(&fw_test->fw_test_work);
	struct net_device   *dev;

	while (skb) {
		u16 vif = fapi_get_vif(skb);

		SLSI_DBG3(sdev, SLSI_FW_TEST, "0x%p: Signal:0x%.4X, vif:%d\n", skb, fapi_get_sigid(skb), vif);

		if (WLBT_WARN(!vif, "!vif")) {
			kfree_skb(skb);
			skb = slsi_skb_work_dequeue(&fw_test->fw_test_work);
			continue;
		}

		SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
		dev = slsi_get_netdev_locked(sdev, vif);
		if (!dev) {
			/* Just ignore the signal. This is valid in some error testing scenarios*/
			SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
			kfree_skb(skb);
			skb = slsi_skb_work_dequeue(&fw_test->fw_test_work);
			continue;
		}

		switch (fapi_get_sigid(skb)) {
		case MLME_PROCEDURE_STARTED_IND:
			slsi_fw_test_procedure_started_ind(sdev, dev, fw_test, skb);
			break;
		case MLME_CONNECT_IND:
			slsi_fw_test_connect_ind(sdev, dev, fw_test, skb);
			break;
		case MLME_ROAMED_IND:
			slsi_fw_test_roamed_ind(sdev, dev, fw_test, skb);
			break;
		case MLME_CONNECTED_IND:
			slsi_fw_test_connected_ind(sdev, dev, fw_test, skb);
			break;
		case MLME_DISCONNECTED_IND:
			slsi_fw_test_disconnected_ind(sdev, dev, fw_test, skb);
			break;
		case MLME_TDLS_PEER_IND:
			slsi_fw_test_tdls_peer_ind(sdev, dev, fw_test, skb);
			break;
		case MLME_START_CFM:
			slsi_fw_test_start_cfm(sdev, dev, fw_test, skb);
			break;
		case MLME_ADD_VIF_REQ:
			slsi_fw_test_add_vif_req(sdev, dev, fw_test, skb);
			break;
		case MLME_DEL_VIF_REQ:
			slsi_fw_test_del_vif_req(sdev, dev, fw_test, skb);
			break;
		case MA_BLOCKACKREQ_IND:
			slsi_fw_test_ma_blockackreq_ind(sdev, dev, fw_test, skb);
			break;
#ifdef CONFIG_SCSC_WLAN_TX_API
		case MLME_FRAME_TRANSMISSION_IND:
			slsi_tx_mlme_ind(sdev, dev, skb);
			kfree_skb(skb);
			break;
		case MLME_SEND_FRAME_CFM:
			slsi_tx_mlme_cfm(sdev, dev, skb);
			kfree_skb(skb);
			break;
#endif
		default:
			WLBT_WARN(1, "Unhandled Signal");
			kfree_skb(skb);
			break;
		}
		SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);

		skb = slsi_skb_work_dequeue(&fw_test->fw_test_work);
	}
}

void slsi_fw_test_init(struct slsi_dev *sdev, struct slsi_fw_test *fwtest)
{
	SLSI_DBG1(sdev, SLSI_FW_TEST, "\n");
	memset(fwtest, 0x00, sizeof(struct slsi_fw_test));
	fwtest->sdev = sdev;
	slsi_spinlock_create(&fwtest->fw_test_lock);
	slsi_skb_work_init(sdev, NULL, &fwtest->fw_test_work, "slsi_wlan_fw_test", slsi_fw_test_work);
}

void slsi_fw_test_deinit(struct slsi_dev *sdev, struct slsi_fw_test *fwtest)
{
	int i;

	SLSI_UNUSED_PARAMETER(sdev);

	SLSI_DBG1(sdev, SLSI_FW_TEST, "\n");
	fwtest->fw_test_enabled = false;
	slsi_skb_work_deinit(&fwtest->fw_test_work);
	slsi_spinlock_lock(&fwtest->fw_test_lock);
	for (i = 1; i <= CONFIG_SCSC_WLAN_MAX_INTERFACES; i++) {
		kfree_skb(fwtest->mlme_add_vif_req[i]);
		kfree_skb(fwtest->mlme_connect_req[i]);
		kfree_skb(fwtest->mlme_connect_cfm[i]);
		kfree_skb(fwtest->mlme_procedure_started_ind[i]);

		fwtest->mlme_add_vif_req[i] = NULL;
		fwtest->mlme_connect_req[i] = NULL;
		fwtest->mlme_connect_cfm[i] = NULL;
		fwtest->mlme_procedure_started_ind[i] = NULL;
	}
	slsi_spinlock_unlock(&fwtest->fw_test_lock);
	memset(fwtest, 0x00, sizeof(struct slsi_fw_test));
}
