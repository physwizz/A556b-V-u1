/* SPDX-License-Identifier: GPL-2.0 */
/******************************************************************************
 *
 * Copyright (c) 2014 - 2020 Samsung Electronics Co., Ltd. All rights reserved
 *
 *****************************************************************************/

#ifndef __LOAD_BALANCE_H__
#define __LOAD_BALANCE_H__

#include "dev.h"

#ifdef CONFIG_SCSC_WLAN_CPUHP_MONITOR
#include <linux/cpufreq.h>
#endif

#define SLSI_HIP_NAPI_STATE_ENABLED (0x1)
#ifdef CONFIG_SCSC_WLAN_CPUHP_MONITOR
#define SLSI_MAX_CLUSTERS SLSI_NR_CPUS
#endif
#define RPS_MAP_LEN 4
#define RPS_MAP_LOW "000"
#if defined(CONFIG_SOC_S5E8855)
#define RPS_MAP_HIGH "060"
#else
#define RPS_MAP_HIGH "070"
#endif

enum bh_type_t {
	NP_T,
	TL_T,
	WQ_T
};

enum napi_idx {
	NP_RX_0,
	NP_RX_1,
	NP_TX_0,
	NAPI_NUM_MAX
};

struct bh_struct;

enum ctrl_event_type_t {
	RPS_T,
	CPU_AFFINITY_T
};

struct ctrl_event {
	struct list_head list;
	u8 type;
	union {
		struct {
			struct net_device *dev;
			u32 state;
		} rps;
		struct {
			struct bh_struct *bh;
			u32 state;
		} cpu_affinity;
	} event;
};

#ifdef CONFIG_SCSC_WLAN_CPUHP_MONITOR
struct cpu_cluster_info {
	uint first_cpu;
	uint last_cpu;
	int cpu_num;
};
#endif

struct load_manager {
	struct slsi_dev *sdev;
	/**
	 * list of bh handlers
	 */
	struct list_head bh_list;
	rwlock_t bh_list_lock;

	/**
	 * CPU hotplug handler and its status.
	 */
#ifdef CONFIG_SCSC_WLAN_CPUHP_MONITOR
	struct hlist_node *cpuhp_node;
#endif
	bool cpu_avail[SLSI_NR_CPUS];
	struct net_device napi_netdev;

	struct work_struct lbm_ctrl_work;
	struct list_head ctrl_event_list;
	/* spinlock for control rps and affinity event */
	spinlock_t ctrl_event_list_lock;
};

#ifndef CONFIG_SCSC_WLAN_THREADED_NAPI
struct napi_cpu_info {
		struct napi_struct napi_instance;
		call_single_data_t csd;
		struct napi_priv *priv;
};
#endif

struct napi_priv {
#ifdef CONFIG_SCSC_WLAN_THREADED_NAPI
	struct napi_struct napi_instance;
#else
	struct napi_cpu_info cpu_info[SLSI_NR_CPUS];
#endif
	/* Global napi variables */
	struct bh_struct *bh;
	unsigned long napi_state;
	int (*poll)(struct napi_struct *napi, int weight);
};

struct work_priv {
	struct work_struct	bh;
	struct bh_struct	*bh_s;
};

struct tasklet_priv {
	struct tasklet_struct	bh;
	struct bh_struct	*bh_s;
};

struct cpu_affinity_ctrl_info {
	int cpu[TRAFFIC_MON_CLIENT_MAX_NUM_OF_STATE];
	int curr_cpu;
	int mid;
	int high;
	u32 state;
	int idx;
};

struct io_saturatioin_ctrl_info {
	bool saturated;
	int saturation_cnt;
	int saturation_trigger_threshold;
};

struct bh_struct {
	struct list_head list;
	struct slsi_dev *sdev;
	struct hip_priv *hip_priv;

	enum bh_type_t type;
	int irq;

	union {
		struct napi_priv napi;
		struct tasklet_priv tasklet;
		struct work_priv work;
	} bh_priv;

	/**
	 * These are optional fields set when cpu affinity control and io saturation detection is required
	 * for the registered bh. register_cpu_affinity_control and register_io_saturation_control are used
	 * to register control info. unregister_cpu_affinity_control and unregister_io_saturation_control
	 * are used to unregister control info.
	 */
	struct cpu_affinity_ctrl_info *cpu_affinity;
	struct io_saturatioin_ctrl_info *io_saturation;

#ifdef CONFIG_SCSC_WLAN_TX_API
	struct tx_struct *tx;
#endif
	void *irq_affinity_client;
};

struct rps_ctrl_info {
	char rps[TRAFFIC_MON_CLIENT_MAX_NUM_OF_STATE][RPS_MAP_LEN];
	int mid;
	int high;
	u32 state;
};

static __always_inline struct hip_priv *slsi_lbm_get_hip_priv_from_napi(struct napi_struct *napi)
{
#ifdef CONFIG_SCSC_WLAN_THREADED_NAPI
	struct napi_priv *priv = container_of(napi, struct napi_priv, napi_instance);

	return priv->bh->hip_priv;
#else
	struct napi_cpu_info *cpu_info = container_of(napi, struct napi_cpu_info, napi_instance);

	return cpu_info->priv->bh->hip_priv;
#endif
}

static __always_inline struct hip_priv *slsi_lbm_get_hip_priv_from_work(struct work_struct *data)
{
	struct work_priv        *work_priv = container_of(data, struct work_priv, bh);

	return work_priv->bh_s->hip_priv;
}

#ifdef CONFIG_SCSC_WLAN_CPUHP_MONITOR
int slsi_find_proper_cpu(struct bh_struct *bh, int state, enum napi_idx idx, int cl_idx, cpumask_t *except_cpu);
#endif
void slsi_lbm_init(struct slsi_dev *sdev);
void slsi_lbm_deinit(struct slsi_dev *sdev);

int slsi_lbm_set_property(struct slsi_dev *sdev);

int slsi_lbm_netdev_activate(struct slsi_dev *sdev, struct net_device *dev);
int slsi_lbm_netdev_deactivate(struct slsi_dev *sdev, struct net_device *dev, struct netdev_vif *ndev_vif);

struct bh_struct *slsi_lbm_register_napi(struct slsi_dev *sdev, int (*napi_poll)(struct napi_struct *, int),
					 int irq, int idx);
struct bh_struct *slsi_lbm_register_tasklet(struct slsi_dev *sdev, void (*tasklet_func)(unsigned long data), int irq);
struct bh_struct *slsi_lbm_register_workqueue(struct slsi_dev *sdev, void (*workqueue_func)(struct work_struct *data),
					      int irq);
struct cpu_affinity_ctrl_info *slsi_lbm_register_cpu_affinity_control(struct bh_struct *bh, int idx);
struct io_saturatioin_ctrl_info *slsi_lbm_register_io_saturation_control(struct bh_struct *bh);
struct rps_ctrl_info *slsi_lbm_register_rps_control(struct net_device *dev, const int mid, const int high);

int slsi_lbm_unregister_bh(struct bh_struct *bh);
void slsi_lbm_unregister_cpu_affinity_control(struct slsi_dev *sdev, struct bh_struct *bh);
void slsi_lbm_unregister_io_saturation_control(struct bh_struct *bh);
void slsi_lbm_unregister_rps_control(struct net_device *dev);
void slsi_lbm_state_change_for_affinity(struct bh_struct *bh, u32 state, int force_cpu);

int slsi_lbm_run_bh(struct bh_struct *bh);

#if (defined(CONFIG_SCSC_QOS) && !defined(CONFIG_SCSC_WLAN_HIP5))
void slsi_set_saturation_trigger_threshold(uint val);
uint slsi_get_saturation_trigger_threshold(void);
#endif

#ifdef CONFIG_SCSC_WLAN_CPUHP_MONITOR
int slsi_lbm_cpuhp_online_cb(int cpu, void *data);
int slsi_lbm_cpuhp_offline_cb(int cpu, void *data);
#endif

void slsi_lbm_freeze(void);
void slsi_lbm_setup(struct bh_struct *bh);

#ifdef CONFIG_SCSC_WLAN_TX_API
void slsi_lbm_disable_tx_napi(void);
void slsi_lbm_enable_tx_napi(void);
#endif

unsigned int slsi_lbm_get_napi_cpu(int idx, u32 state);
#endif	/* __LOAD_BALANCE_H__ */
