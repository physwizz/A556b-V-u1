
/****************************************************************************
 *
 * Copyright (c) 2014 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __SAP_MLME_H__
#define __SAP_MLME_H__

int sap_mlme_init(void);
int sap_mlme_deinit(void);

/* MLME signal handlers in rx.c */
void slsi_rx_scan_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
#if defined CONFIG_SLSI_WLAN_STA_FWD_BEACON && (defined(SCSC_SEP_VERSION) && SCSC_SEP_VERSION >= 10)
void slsi_rx_beacon_reporting_event_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
#endif
void slsi_rx_scan_done_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_channel_switched_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_synchronised_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_blacklisted_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_connect_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_connected_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_received_frame_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_disconnect_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_disconnected_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_procedure_started_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_frame_transmission_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_roamed_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_roam_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_mic_failure_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_reassoc_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_tdls_peer_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_blockack_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_listen_end_ind(struct net_device *dev, struct sk_buff *skb);
void slsi_rx_ma_to_mlme_delba_req(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_start_detect_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_rcl_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_get_fapi_version_string(char *res);
void slsi_rx_twt_setup_info_event(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_twt_teardown_indication(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_twt_notification_indication(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_scheduled_pm_teardown_indication(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_scheduled_pm_leaky_ap_detect_indication(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_delayed_wakeup_indication(struct slsi_dev *sdev, struct net_device *dev,
				       struct sk_buff *skb);
void slsi_rx_sr_params_changed_indication(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
#ifdef CONFIG_SCSC_WLAN_EHT
void slsi_rx_mlo_link_info_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_mlo_link_measurement_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_rx_mlo_set_ttlm_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
#endif
u16 slsi_mlme_get_vif(struct slsi_dev *sdev, struct sk_buff *skb);
#endif
