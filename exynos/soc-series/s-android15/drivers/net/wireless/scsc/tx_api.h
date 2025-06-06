/*****************************************************************************
 *
 * Copyright (c) 2021 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __TX_API_H__
#define __TX_API_H__

#include <linux/netdevice.h>
#include <linux/types.h>
#include "dev.h"
#include "mgt.h"
#include "mlme.h"

enum slsi_tx_packet_t {
	SLSI_CTRL_PKT,
	SLSI_DATA_PKT
};

/**
 * Lock: No lock is acquired.
 * Context: Process
 * Description: It is called at the end of slsi_dev_attach.
 * We can allocate sdev domain resources in this function and assign it to sdev->tx_priv for reference in future.
 */
void slsi_dev_attach_post(struct slsi_dev *sdev);

/**
 * Lock: No lock is acquired.
 * Context: Process
 * Description: It is called at the end of slsi_dev_detach.
 * We should free the resources allocated in on_dev_attach.
 */
void slsi_dev_detach_post(struct slsi_dev *sdev);

/**
 * Lock: No lock is acquired.
 * Context: Process
 * Description: It is called at the end of slsi_hip_setup.
 * It sets up the maximum TX units in the HIP that the TXBP will use as MOD.
 */
void slsi_hip_setup_post(struct slsi_dev *sdev, u16 tx_slots);

/**
 * Lock: netdev_add_remove_mutex
 * Context: Process.
 * Description: return # of Tx queue. Return value should be > 1.
 */
int slsi_tx_get_number_of_queues(void);
/**
 * Lock: netdev_add_remove_mutex
 * Context: Process
 * Description: In this function, we can setup dev parameter.
 * e.g., dev->watchdog_timeo = XXXX
 */
void slsi_tx_setup_net_device(struct net_device *dev);

/**
 * Lock:
 * - Mutex: vif_mutex, start_stop_mutex
 * - Spinlock: tcp_ack_lock, peer_lock
 * Context: Atomic (due the cases where spinlock is acquired).
 * Description: It is called after all peer data is initialized before enabling valid flag.
 * We can allocate resources required for peer.
 * We can access the priv data when peer->valid is true.
 */
bool slsi_peer_add_post(struct slsi_dev *sdev, struct net_device *dev, struct netdev_vif *ndev_vif, struct slsi_peer *peer);

/**
 * Lock: vif_mutex
 * Context: Process
 * Description: It is called at the end of slsi_peer_remove.
 * We should free the resources allocated in on_peer_add.
 * At this stage, peer->valid is false.
 */
void slsi_peer_remove_post(struct slsi_dev *sdev, struct net_device *dev, struct netdev_vif *ndev_vif, struct slsi_peer *peer);

/**
 * Lock: vif_mutex, start_stop_mutex
 * Context: Process
 * Description: It is called at the end of slsi_vif_activated before enabling activated flag.
 * We can allocate per vif tx_priv data in this function.
 * We can access the priv data when ndev_vif->activated is true.
 */
bool slsi_vif_activated_post(struct slsi_dev *sdev, struct net_device *dev, struct netdev_vif *ndev_vif);

/**
 * Lock: vif_mutex, start_stop_mutex
 * Context: Process
 * Description: It is called at the end of slsi_vif_deactivated.
 * We should free the resources allocated in on_vif_activated.
 * At this stage, ndev_vif->activated is false.
 */
bool slsi_vif_deactivated_post(struct slsi_dev *sdev, struct net_device *dev, struct netdev_vif *ndev_vif);

#if (defined(CONFIG_SCSC_QOS) && !defined(CONFIG_SCSC_WLAN_HIP5))
/**
 * Lock:
 * Context: Process
 * Description: It is called at the end of slsi_vif_deactivated_post.
 * This function should be called for every nan_data vif.
 */
void slsi_ndp_vif_deactivated_post(struct slsi_dev *sdev);

/**
 * Lock:
 * Context: Process
 * Description: It is called at the begining of slsi_vif_activated_post.
 * This function should be called for every nan_data vif.
 */
void slsi_ndp_vif_activated_post(struct slsi_dev *sdev, struct net_device *dev, struct netdev_vif *ndev_vif);
#endif

/**
 * Lock:
 * - Mutex: vif_mutex, start_stop_mutex, rtnl_lock
 * - Spinlock: peer_lock
 * Context: Atomic (due the cases where spinlock is acquired).
 * Description: It is called whenerver there is change in port state. Tx logic should handle this event properly when necessary.
 */
int slsi_tx_ps_port_control(struct slsi_dev *sdev, struct net_device *dev, struct slsi_peer *peer, enum slsi_sta_conn_state s);

/**
 * Lock:
 * - Mutex: vif_mutex
 * - Spinlock: tcp_ack_lock, peer_lock
 * Context: Atomic (due the cases where spinlock is acquired).
 * Description: It is called when tdls peer is connected and disconnected.
 * We should take care of tdls peer's packet in this function if necessary.
 */
bool slsi_tx_tdls_update(struct slsi_dev *sdev, struct net_device *dev, struct slsi_peer *sta_peer, struct slsi_peer *tdls_peer, bool connection);

/**
 * Lock: N/A
 * Context: Process or interrupt
 * Description: This function should return tx q index for skb.
 */
#if (KERNEL_VERSION(5, 2, 0) <= LINUX_VERSION_CODE)
u16 slsi_tx_select_queue(struct net_device *dev, struct sk_buff *skb, struct net_device *sb_dev);
#elif (KERNEL_VERSION(4, 19, 0) <= LINUX_VERSION_CODE)
u16 slsi_tx_select_queue(struct net_device *dev, struct sk_buff *skb, struct net_device *sb_dev,
			 select_queue_fallback_t fallback);
#else
u16 slsi_tx_select_queue(struct net_device *dev, struct sk_buff *skb, void *accel_priv,
			 select_queue_fallback_t fallback);
#endif

/**
 * Lock: __netif_tx_lock spinlock
 * Context: Process with BHs disabled or BH
 * Description: Check if packet should be on MLME or MA.
 * SLSI_CTRL_PKT would let skb to take MLME path and
 * SLSI_DATA_PKT would let skb to take DATA path.
 */
enum slsi_tx_packet_t slsi_tx_get_packet_type(struct sk_buff *skb);

/**
 * Lock: __netif_tx_lock spinlock
 * Context: Process with BHs disabled or BH
 * Description: Transmit packet over MLME path.
 */
int slsi_tx_transmit_ctrl(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);

/**
 * Lock: vif_mutex
 * Context: Process
 * Description: It is called on slsi_rx_frame_transmission_ind from FW.
 * We can use this function for flow control of CTRL path.
 */
int slsi_tx_mlme_ind(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);

/**
 * Lock: N/A
 * Context: Process
 * Description: It is called when host receives MLME_SEND_CFM from FW.
 * With on_mlme_ind, we can use this function for flow control of CTRL path.
 * Do NOT free skb in cfm.
 */
int slsi_tx_mlme_cfm(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);

/**
 * Lock: __netif_tx_lock spinlock
 * Context: Process with BHs disabled or BH
 * Description: Transmit packet over DATA path.
 */
int slsi_tx_transmit_data(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);

/**
 * Lock: rx_lock
 * Context: Process with BH disabeld or BH
 * Description: It is called when Tx request is completed by FW.
 * We can use this function for flow control of DATA path.
 */
int slsi_tx_done(struct slsi_dev *sdev, u32 colour, bool more);

/**
 * Lock: N/A
 * Context: Process
 * Description: It is called when there is write from udi cdev.
 * Need to check if it is still used.
 */
int slsi_tx_transmit_lower(struct slsi_dev *sdev, struct sk_buff *skb);

/**
 * Lock: net_device tx_global_lock
 * Context: softirq
 * Description: Print debug message if tx timeout happens
 */
void slsi_tx_timeout(struct net_device *dev);

/**
 * Lock: start_stop_mutex
 * Context: Process
 * Description: It is called when host goes to suspend.
 * It disables TX napis.
 */
void slsi_txbp_suspend(struct slsi_dev *sdev);
/**
 * Lock: start_stop_mutex
 * Context: Process
 * Description: It is called when host wakes up.
 * It enables TX napis.
 */
void slsi_txbp_resume(struct slsi_dev *sdev);

/**
 * Lock: N/A
 * Context: N/A
 * Description: Get gcod value
 */
int slsi_tx_get_gcod(void);
#endif
