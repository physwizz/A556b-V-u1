/****************************************************************************
 *
 * Copyright (c) 2012 - 2018 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __TRAFFIC_MONITOR_H__
#define __TRAFFIC_MONITOR_H__

#define SLSI_TRAFFIC_MON_TIMER_PERIOD		100		/* in ms */
#define SLSI_TRAFFIC_MON_HYSTERESIS_HIGH	2		/* in number of timer period */
#define SLSI_TRAFFIC_MON_HYSTERESIS_LOW		10		/* in number of timer period */

#define SLSI_TRAFFIC_MON_BYTES_TO_MBPS(bytes)				((bytes * 8) / (1000 * 1000))
#define SLSI_TRAFFIC_MON_MBPS_TO_BYTES_PER_SEC(mbps)		((mbps * 1000 * 1000) / 8)

enum {
	TRAFFIC_MON_CLIENT_MODE_NONE,
	TRAFFIC_MON_CLIENT_MODE_PERIODIC,
	TRAFFIC_MON_CLIENT_MODE_EVENTS
};

enum {
	TRAFFIC_MON_CLIENT_STATE_LOW,
	TRAFFIC_MON_CLIENT_STATE_MID,
	TRAFFIC_MON_CLIENT_STATE_HIGH,
	TRAFFIC_MON_CLIENT_STATE_OVERRIDE,
	TRAFFIC_MON_CLIENT_MAX_NUM_OF_STATE
};

enum {
	TRAFFIC_MON_DIR_DEFAULT,
	TRAFFIC_MON_DIR_RX,
	TRAFFIC_MON_DIR_TX
};

struct slsi_traffic_mon_clients {
	/* client list lock */
	spinlock_t       lock;

	struct timer_list timer;
	struct list_head client_list;
};

void slsi_traffic_mon_event_rx(struct slsi_dev *sdev, struct net_device *dev, struct sk_buff *skb);
void slsi_traffic_mon_event_tx(struct slsi_dev *sdev, struct net_device *dev, u32 len);

/* Client request to override traffic monitor
 *
 * A client use the API to request to override it's own state
 * detected by traffic monitor.
 * All clients are notified about the event, and each client
 * can take it's own decision for "override" state.
 *
 * Returns:		None
 */
void slsi_traffic_mon_override(struct slsi_dev *sdev);

/* Is traffic monitor running?
 *
 * A caller can seek whether the traffic monitor is running or not.
 * If the traffic monitor is running, then the throughput for Rx and
 * Tx is stored in netdev_vif structure and caller can access them.
 *
 * Returns:		1 - traffic monitor is running
 *				0 - traffic monitor is NOT running
 */
u8 slsi_traffic_mon_is_running(struct slsi_dev *sdev);

/* register a client to traffic monitor
 *
 * A client can register to traffic monitor to either get notification
 * per timer period or to get notification when the throughput changes
 * state from one state of LOW/MID/HIGH to another.
 *
 * client_ctx:	client context that is passed back in callback
 *				Also, is an unique ID for the client.
 * mode:		can be periodic or event based
 * mid_tput:	Mid throughput level
 * high_tput:	High throughput level
 * traffic_mon_client_cb: function to callback each period or on events
 *                        This function is called back in Timer interrupt
 *                        context.
 */
void *slsi_traffic_mon_client_register(
	struct slsi_dev *sdev,
	void *client_ctx,
	u32 mode,
	u32 mid_tput,
	u32 high_tput,
	u32 dir,
	/* WARNING: THIS IS CALLED BACK IN TIMER INTERRUPT CONTEXT! */
	void (*traffic_mon_client_cb)(void *client_ctx, u32 state, u32 tput_tx, u32 tput_rx));
void slsi_traffic_mon_client_unregister(struct slsi_dev *sdev, void *traffic_mon_client);

void slsi_traffic_mon_clients_init(struct slsi_dev *sdev);
void slsi_traffic_mon_clients_deinit(struct slsi_dev *sdev);
void slsi_traffic_mon_init(struct slsi_dev *sdev);
void slsi_traffic_mon_deinit(struct slsi_dev *sdev);

#endif
