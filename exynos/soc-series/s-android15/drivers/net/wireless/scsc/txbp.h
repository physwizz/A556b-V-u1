/* SPDX-License-Identifier: <SPDX License Expression> */
/*****************************************************************************
 *
 * Copyright (c) 2012 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __SLSI_TXBP_H__
#define __SLSI_TXBP_H__

/**
 * AC queues: DATA.
 * Priority: EAPOL/DHCP
 * ARP: ARP
 * ++++++++++++++++++++++++++++++++++++++
 * | BE | BK | VI | VO | Priority | ARP |
 * ++++++++++++++++++++++++++++++++++++++
 */
#define AC_CATEGORIES (4)
#define SLSI_TX_DATA_QUEUE_NUM       (AC_CATEGORIES)
#define SLSI_TX_CTRL_QUEUE_NUM       (2)
#define SLSI_TX_TOTAL_QUEUES         (SLSI_TX_DATA_QUEUE_NUM + SLSI_TX_CTRL_QUEUE_NUM)
#define SLSI_TX_PRIORITY_Q_INDEX     (SLSI_TX_DATA_QUEUE_NUM)
#define SLSI_TX_ARP_Q_INDEX          (SLSI_TX_PRIORITY_Q_INDEX + 1)
#define SLSI_TX_BP_MBULK_GUARD       (10)
#define SLSI_TX_LBM_RUNNING          (0x2)
#define SLSI_TX_NAPI_ENABLED         (0x1)
#define SLSI_TX_NAPI_POLLING         (SLSI_TX_NAPI_ENABLED << 1)

struct txq_stats {
	ktime_t stop;
	ktime_t wake;
	ktime_t cumulated_stop;
	ktime_t cumulated_wake;
	ktime_t last_cumulated_stop;
	ktime_t last_cumulated_wake;
};

struct txbp_data {
	rwlock_t cod_lock;
	int cod;
	struct list_head vif_list;
	rwlock_t vif_lock;
	u32 vif_cnt;
	u32 peers;
#ifdef CONFIG_SCSC_WLAN_ADAPTIVE_TX_MOD
	 atomic_t backlog;
#endif
};

#define MX_CLAIM_STATE_DEFERRED (0x1)
#define MX_CLAIM_STATE_CLAIMED (0x2)
struct tx_struct {
#ifdef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	unsigned long lbm_bh_state;
#else
	struct napi_struct napi;
	u8 target_cpu;
	call_single_data_t csd;
#endif
	unsigned long napi_state;
	struct sk_buff_head *assigned_q[SLSI_TX_DATA_QUEUE_NUM];
	struct slsi_dev *sdev;
	struct net_device *ndev;
};

struct tx_netdev_data {
	struct list_head list;
	int netdev_mod;
	int netdev_cod;
	int ac_cod[AC_CATEGORIES];
	int ac_completed[AC_CATEGORIES];
	struct net_device napi_netdev;
	u8 ac_presence;
	u8 ac_presence_update;
	unsigned long last_presence_check_time;
	struct sk_buff_head q[SLSI_TX_DATA_QUEUE_NUM];
	struct txq_stats qstat[SLSI_TX_DATA_QUEUE_NUM];
	struct tx_struct tx;
	/* tx arp lock for vif */
	spinlock_t arp_lock;
#ifdef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
	struct bh_struct *bh_tx;
	unsigned int dedicated_napi;
#endif
#ifdef CONFIG_SCSC_WLAN_ADAPTIVE_TX_MOD
	atomic_t backlog;
#endif
};

#ifdef SCSC_WLAN_PRE_AGGREGATE_AMSDU
struct subframe_list {
	struct list_head list;
	int len;
	int count;
	struct sk_buff *first;
	struct timer_list timer;
	struct net_device *dev;
	struct work_struct timeout_work;
	/* subframe_list lock */
	spinlock_t list_lock;
};
#endif
/**
 * Lock: vif_mutex, netdev_add_remove_mutex
 * Context: Process.
 * Description: write the value of txbp_priv.cod, tx_priv->netdev_cod and tx_priv->ac_cod to given parameters.
 * return 0 if succeed, -EINVAL if paramters are not valid.
 */
int slsi_tx_get_cod(struct slsi_dev *sdev, void *tx_priv, int *gcod, int *netdev_cod, int *ac_cod);

#endif
