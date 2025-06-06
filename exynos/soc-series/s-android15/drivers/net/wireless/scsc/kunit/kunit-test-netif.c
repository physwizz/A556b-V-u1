// SPDX-License-Identifier: GPL-2.0+
#include <kunit/test.h>

#include "kunit-common.h"
#include "kunit-mock-mgt.h"
#include "kunit-mock-dev.h"
#include "kunit-mock-ioctl.h"
#include "kunit-mock-kernel.h"
#include "kunit-mock-tx.h"
#include "kunit-mock-txbp.h"
#include "kunit-mock-traffic_monitor.h"
#include "kunit-mock-sap_mlme.h"
#include "kunit-mock-misc.h"
#include "kunit-mock-local_packet_capture.h"
#include "../netif.c"

static void test_slsi_net_randomize_nmi_ndi(struct kunit *test)
{
	struct slsi_dev *sdev = TEST_TO_SDEV(test);

	slsi_net_randomize_nmi_ndi(sdev);
}

static void test_slsi_mac_address_init(struct kunit *test)
{
	struct slsi_dev *sdev = TEST_TO_SDEV(test);

	slsi_mac_address_init(sdev);
}

static void test_slsi_netif_is_udp_pkt(struct kunit *test)
{
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	struct iphdr *head = kunit_kzalloc(test, sizeof(struct iphdr), GFP_KERNEL);

	skb->head = head;
	skb->network_header = 0;
	head->version = 0;
	KUNIT_EXPECT_FALSE(test, slsi_netif_is_udp_pkt(skb));

	head->version = 4;
	head->protocol = IPPROTO_UDP;
	KUNIT_EXPECT_TRUE(test, slsi_netif_is_udp_pkt(skb));

	head->version = 6;
	ipv6_hdr(skb)->nexthdr = NEXTHDR_UDP;
	KUNIT_EXPECT_TRUE(test, slsi_netif_is_udp_pkt(skb));
}

static void test_slsi_netif_set_tid_change_tid(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	struct iphdr *head = kunit_kzalloc(test, sizeof(struct iphdr), GFP_KERNEL);

	skb->head = head;
	skb->network_header = 0;

	ndev_vif->set_tid_attr.mode = SLSI_NETIF_SET_TID_ALL_UDP;
	skb->priority = FAPI_PRIORITY_QOS_UP0;
	head->version = 4;
	head->protocol = IPPROTO_UDP;

	slsi_netif_set_tid_change_tid(dev, skb);
}

static void test_slsi_netif_set_tid_config(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;
	u8 mode = SLSI_NETIF_SET_TID_OFF;
	u32 uid = 0;
	u8 tid = 0;

	KUNIT_EXPECT_EQ(test, -ENODEV, slsi_netif_set_tid_config(sdev, dev, mode, uid, tid));

	ndev_vif->activated = 1;
	mode = SLSI_NETIF_SET_TID_ALL_UDP + 1;
	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_netif_set_tid_config(sdev, dev, mode, uid, tid));

	mode = SLSI_NETIF_SET_TID_ALL_UDP - 1;
	tid = FAPI_PRIORITY_QOS_UP7 + 1;
	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_netif_set_tid_config(sdev, dev, mode, uid, tid));

	tid = FAPI_PRIORITY_QOS_UP7;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_set_tid_config(sdev, dev, mode, uid, tid));
}

static void test_slsi_net_open(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;
	struct net_device *dev3 = kunit_kzalloc(test,
						sizeof(struct net_device) + sizeof(struct netdev_vif), GFP_KERNEL);
	struct netdev_vif *ndev_vif3 = netdev_priv(dev3);
	unsigned char *addr = "\x00\x00\x0F\x11\x22\x33";

	dev->dev_addr = addr;

	ndev_vif->is_available = 0;
	sdev->mlme_blocked = 1;
	KUNIT_EXPECT_EQ(test, -EIO, slsi_net_open(dev));

	sdev->mlme_blocked = 0;
	sdev->recovery_fail_safe = 1;
	sdev->netdev_up_count = 0;
	ndev_vif->is_available = 0;
	KUNIT_EXPECT_EQ(test, 0, slsi_net_open(dev));

	sdev->netdev_up_count = 1;
	ndev_vif->is_available = 0;
	sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN] = dev3;
	ndev_vif3->is_available = 1;
	ndev_vif->ifnum = SLSI_NET_INDEX_P2PX_SWLAN;
	ndev_vif->iftype = NL80211_IFTYPE_MONITOR;
	ndev_vif->ifnum = 0;
	KUNIT_EXPECT_EQ(test, 0, slsi_net_open(dev));
}

static void test_slsi_net_stop(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;

	ndev_vif->wlan_wl_sae.ws = kunit_kzalloc(test, sizeof(struct wakeup_source), GFP_KERNEL);
	ndev_vif->wlan_wl_sae.ws->active = false;

	sdev->recovery_fail_safe = 1;
	KUNIT_EXPECT_EQ(test, 0, slsi_net_stop(dev));

	ndev_vif->is_available = 1;
	KUNIT_EXPECT_EQ(test, 0, slsi_net_stop(dev));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static void test_slsi_net_prv_ioctl(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct ifreq *rq = kunit_kzalloc(test, sizeof(struct ifreq), GFP_KERNEL);
	int cmd = 0;

	KUNIT_EXPECT_EQ(test, -EOPNOTSUPP, slsi_net_prv_ioctl(dev, rq, NULL, cmd));
}
#endif

static void test_slsi_net_ioctl(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct ifreq *rq = kunit_kzalloc(test, sizeof(struct ifreq), GFP_KERNEL);
	int cmd = SIOCDEVPRIVATE + 2;

	KUNIT_EXPECT_EQ(test, 0, slsi_net_ioctl(dev, rq, cmd));
}

static void test_slsi_net_get_stats(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	KUNIT_EXPECT_PTR_EQ(test, &ndev_vif->stats, (struct net_device_stats *)slsi_net_get_stats(dev));
}

#ifndef CONFIG_WLBT_WARN_ON
static void test_slsi_netif_show_stats(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);

	slsi_netif_show_stats(dev);
}

static void test_slsi_net_tx_timeout(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))
	slsi_net_tx_timeout(dev, 0);
#else
	slsi_net_tx_timeout(dev);
#endif
}
#endif

#ifdef CONFIG_SCSC_USE_WMM_TOS
static void test_slsi_get_priority_from_tos(struct kunit *test)
{
	u8 frame[2];
	u16 proto = 0;

	KUNIT_EXPECT_EQ(test, 0, slsi_get_priority_from_tos_dscp(NULL, proto));

	KUNIT_EXPECT_EQ(test, 0, slsi_get_priority_from_tos_dscp(frame, proto));

	proto = ETH_P_IP;
	KUNIT_EXPECT_EQ(test, 0, slsi_get_priority_from_tos_dscp(frame, proto));

	proto = ETH_P_IPV6;
	KUNIT_EXPECT_EQ(test, 0, slsi_get_priority_from_tos_dscp(frame, proto));
}
#else
static void test_slsi_get_priority_from_tos_dscp(struct kunit *test)
{
	u8 frame[2];
	u16 proto = 0;

	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP0, slsi_get_priority_from_tos_dscp(NULL, proto));

	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP0, slsi_get_priority_from_tos_dscp(frame, proto));

	proto = ETH_P_IPV6;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP0, slsi_get_priority_from_tos_dscp(frame, proto));

	proto = ETH_P_IP;
	frame[IP4_OFFSET_TO_TOS_FIELD] = CS7 << FIELD_TO_DSCP;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP7, slsi_get_priority_from_tos_dscp(frame, proto));

	frame[IP4_OFFSET_TO_TOS_FIELD] = DSCP_EF << FIELD_TO_DSCP;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP6, slsi_get_priority_from_tos_dscp(frame, proto));

	frame[IP4_OFFSET_TO_TOS_FIELD] = CS5 << FIELD_TO_DSCP;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP5, slsi_get_priority_from_tos_dscp(frame, proto));

	frame[IP4_OFFSET_TO_TOS_FIELD] = DSCP_AF41 << FIELD_TO_DSCP;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP4, slsi_get_priority_from_tos_dscp(frame, proto));

	frame[IP4_OFFSET_TO_TOS_FIELD] = DSCP_AF21 << FIELD_TO_DSCP;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP3, slsi_get_priority_from_tos_dscp(frame, proto));

	frame[IP4_OFFSET_TO_TOS_FIELD] = CS2 << FIELD_TO_DSCP;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP0, slsi_get_priority_from_tos_dscp(frame, proto));

	frame[IP4_OFFSET_TO_TOS_FIELD] = CS1 << FIELD_TO_DSCP;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP1, slsi_get_priority_from_tos_dscp(frame, proto));

	frame[IP4_OFFSET_TO_TOS_FIELD] = 0 << FIELD_TO_DSCP;
	KUNIT_EXPECT_EQ(test, FAPI_PRIORITY_QOS_UP0, slsi_get_priority_from_tos_dscp(frame, proto));
}
#endif

static void test_slsi_net_downgrade_ac(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);

	skb->priority = 6;
	KUNIT_EXPECT_EQ(test, true, slsi_net_downgrade_ac(dev, skb));

	skb->priority = 4;
	KUNIT_EXPECT_EQ(test, true, slsi_net_downgrade_ac(dev, skb));

	skb->priority = 0;
	KUNIT_EXPECT_EQ(test, true, slsi_net_downgrade_ac(dev, skb));

	skb->priority = 1;
	KUNIT_EXPECT_EQ(test, false, slsi_net_downgrade_ac(dev, skb));
}

static void test_slsi_net_up_to_ac_mapping(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, BIT(FAPI_PRIORITY_QOS_UP6) | BIT(FAPI_PRIORITY_QOS_UP7),
			(u8)slsi_net_up_to_ac_mapping(FAPI_PRIORITY_QOS_UP6));
	KUNIT_EXPECT_EQ(test, BIT(FAPI_PRIORITY_QOS_UP4) | BIT(FAPI_PRIORITY_QOS_UP5),
			(u8)slsi_net_up_to_ac_mapping(FAPI_PRIORITY_QOS_UP4));
	KUNIT_EXPECT_EQ(test, BIT(FAPI_PRIORITY_QOS_UP0) | BIT(FAPI_PRIORITY_QOS_UP3),
			(u8)slsi_net_up_to_ac_mapping(FAPI_PRIORITY_QOS_UP0));
	KUNIT_EXPECT_EQ(test, BIT(FAPI_PRIORITY_QOS_UP1) | BIT(FAPI_PRIORITY_QOS_UP2),
			(u8)slsi_net_up_to_ac_mapping(FAPI_PRIORITY_QOS_UP1));
}

static void test_slsi_frame_priority_to_ac_queue(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, SLSI_TRAFFIC_Q_BE, slsi_frame_priority_to_ac_queue(FAPI_PRIORITY_QOS_UP0));
	KUNIT_EXPECT_EQ(test, SLSI_TRAFFIC_Q_BK, slsi_frame_priority_to_ac_queue(FAPI_PRIORITY_QOS_UP1));
	KUNIT_EXPECT_EQ(test, SLSI_TRAFFIC_Q_VI, slsi_frame_priority_to_ac_queue(FAPI_PRIORITY_QOS_UP4));
	KUNIT_EXPECT_EQ(test, SLSI_TRAFFIC_Q_VO, slsi_frame_priority_to_ac_queue(FAPI_PRIORITY_QOS_UP6));
	KUNIT_EXPECT_EQ(test, SLSI_TRAFFIC_Q_BE, slsi_frame_priority_to_ac_queue(FAPI_PRIORITY_CONTENTION));
}

static void test_slsi_ac_to_tids(struct kunit *test)
{
	enum slsi_traffic_q ac;
	int tids[2];

	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_ac_to_tids(4, tids));

	ac = SLSI_TRAFFIC_Q_BE;
	KUNIT_EXPECT_EQ(test, 0, slsi_ac_to_tids(ac, tids));

	ac = SLSI_TRAFFIC_Q_BK;
	KUNIT_EXPECT_EQ(test, 0, slsi_ac_to_tids(ac, tids));

	ac = SLSI_TRAFFIC_Q_VI;
	KUNIT_EXPECT_EQ(test, 0, slsi_ac_to_tids(ac, tids));

	ac = SLSI_TRAFFIC_Q_VO;
	KUNIT_EXPECT_EQ(test, 0, slsi_ac_to_tids(ac, tids));
}

static void test_slsi_net_downgrade_pri(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct slsi_peer *peer = kunit_kzalloc(test, sizeof(struct slsi_peer), GFP_KERNEL);
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);

	skb->priority = 1;
	peer->wmm_acm = 7;
	slsi_net_downgrade_pri(dev, peer, skb);
}

#ifndef CONFIG_SCSC_WLAN_TX_API
static u16 call_slsi_net_select_queue(struct net_device *dev, struct sk_buff *skb, struct net_device *sb_dev,
				      select_queue_fallback_t fallback)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0))
	return slsi_net_select_queue(dev, skb, sb_dev);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	return slsi_net_select_queue(dev, skb, sb_dev, fallback);
#else
	return slsi_net_select_queue(dev, skb, (void *)sb_dev, fallback);
#endif
}

static void test_slsi_net_select_queue(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	struct ethhdr *ehdr = kunit_kzalloc(test, sizeof(struct ethhdr), GFP_KERNEL);

	skb->data = ehdr;
	skb->head = ehdr;
	skb->mac_header = 0;

	ehdr->h_proto = cpu_to_be16(ETH_P_PAE);
	KUNIT_EXPECT_EQ(test, SLSI_NETIF_Q_DISCARD, call_slsi_net_select_queue(dev, skb, NULL, NULL));

	ehdr->h_proto = cpu_to_be16(ETH_P_ARP);
	KUNIT_EXPECT_EQ(test, SLSI_NETIF_Q_DISCARD, call_slsi_net_select_queue(dev, skb, NULL, NULL));

	ehdr->h_proto = cpu_to_be16(ETH_P_IP);
	KUNIT_EXPECT_EQ(test, SLSI_NETIF_Q_DISCARD, call_slsi_net_select_queue(dev, skb, NULL, NULL));
}

static void test_slsi_tdls_move_packets(struct kunit *test)
{
}
#endif

static struct sk_buff *make_null_skb(struct kunit *test, size_t size)
{
	struct sk_buff *skb;
	unsigned char *data;

	skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	data = kunit_kzalloc(test, sizeof(unsigned char) * size, GFP_KERNEL);
	skb->head = data;
	skb->data = data;
	skb->len = size;

	return skb;
}

static struct sk_buff *make_ether_skb(struct kunit *test, size_t size)
{
	struct sk_buff *skb;
	struct ethhdr *ehdr;

	skb = make_null_skb(test, size);
	skb_reset_mac_header(skb);
	skb_pull(skb, (fapi_sig_size(ma_unitdata_req) + 160));
	ehdr = (struct ethhdr *)skb->data;
	eth_random_addr(ehdr->h_source);
	eth_random_addr(ehdr->h_dest);

	return skb;
}

static void test_slsi_net_hw_xmit(struct kunit *test)
{
	struct slsi_dev *sdev = TEST_TO_SDEV(test);
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sk_buff *skb;
	struct slsi_peer *peer;
	struct ethhdr *ehdr;
	size_t size = sizeof(struct ethhdr) + SLSI_NETIF_SKB_HEADROOM;
#ifdef CONFIG_SCSC_WLAN_TX_API
	struct tx_netdev_data *tx_priv = kunit_kzalloc(test, sizeof(struct tx_netdev_data), GFP_KERNEL);

	ndev_vif->tx_netdev_data = tx_priv;
#endif
//data address odd check
	skb = make_null_skb(test, size);
	skb->data = (unsigned char *)((int)(skb->data) + 1);
	KUNIT_EXPECT_EQ(test, -EFAULT, slsi_net_hw_xmit(skb, dev));
#ifdef CONFIG_SCSC_WLAN_TX_API
//zero address check
	skb = make_ether_skb(test, size);
	ehdr = skb->data;
	eth_zero_addr(ehdr->h_source);
	KUNIT_EXPECT_EQ(test, -EFAULT, slsi_net_hw_xmit(skb, dev));
//non peer mac non multicast check
	skb = make_ether_skb(test, size);
	KUNIT_EXPECT_EQ(test, -EFAULT, slsi_net_hw_xmit(skb, dev));
#endif
//ndev_vif available check
	ndev_vif->activated = true;
	ndev_vif->is_available = false;
	skb = make_ether_skb(test, size);
	skb_unset_mac_header(skb);
	ehdr = skb->data;
	sdev->device_config.qos_info = 981;
	peer = slsi_peer_add(sdev, dev, ehdr->h_dest, SLSI_PEER_INDEX_MIN);
	KUNIT_EXPECT_EQ(test, -EFAULT, slsi_net_hw_xmit(skb, dev));
	slsi_peer_remove(sdev, dev, peer);

	ndev_vif->is_available = true;
	ndev_vif->vif_type = FAPI_VIFTYPE_STATION;
#ifndef CONFIG_SCSC_WLAN_TX_API
//queue_mapping discard check
	skb = make_ether_skb(test, size);
	ehdr = skb->data;
	peer = slsi_peer_add(sdev, dev, ehdr->h_dest, SLSI_PEER_INDEX_MIN);
	skb->queue_mapping = SLSI_NETIF_Q_DISCARD;
	KUNIT_EXPECT_EQ(test, -EIO, slsi_net_hw_xmit(skb, dev));
	slsi_peer_remove(sdev, dev, peer);
#endif
	skb = make_ether_skb(test, size);
	ehdr = skb->data;
	peer = slsi_peer_add(sdev, dev, ehdr->h_dest, SLSI_PEER_INDEX_MIN);
#ifdef CONFIG_SCSC_WLAN_TX_API
//headroom size check realloc failed
	skb->queue_mapping = SLSI_TX_DATA_QUEUE_NUM - 1;
#else
	skb->queue_mapping = SLSI_NETIF_Q_PEER_START;
#endif
	KUNIT_EXPECT_EQ(test, -EFAULT, slsi_net_hw_xmit(skb, dev));
	slsi_peer_remove(sdev, dev, peer);
//headroom size realloc success
	skb = make_ether_skb(test, size);
	skb->next = skb;
	ehdr = skb->data;
	peer = slsi_peer_add(sdev, dev, ehdr->h_dest, SLSI_PEER_INDEX_MIN);
#ifdef CONFIG_SCSC_WLAN_TX_API
	skb->queue_mapping = SLSI_TX_DATA_QUEUE_NUM - 1;
#else
	skb->queue_mapping = SLSI_NETIF_Q_PEER_START;
#endif
	KUNIT_EXPECT_EQ(test, NETDEV_TX_OK, slsi_net_hw_xmit(skb, dev));
	slsi_peer_remove(sdev, dev, peer);
//NETDEV_TX_BUSY check
	dev->flags = 1;
	skb = make_ether_skb(test, size);
	ehdr = (struct ethhdr *)skb->data;
	peer = slsi_peer_add(sdev, dev, ehdr->h_dest, SLSI_PEER_INDEX_MIN);
	skb->queue_mapping = SLSI_TX_DATA_QUEUE_NUM;
	KUNIT_EXPECT_EQ(test, NETDEV_TX_BUSY, slsi_net_hw_xmit(skb, dev));
	slsi_peer_remove(sdev, dev, peer);
}

static void test_slsi_net_fix_features(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	netdev_features_t feature = 0;
	netdev_features_t ret = 0;

#ifdef CONFIG_SCSC_WLAN_SG
	ret |= (NETIF_F_SG | NETIF_F_GSO);
#endif

#ifdef CONFIG_SCSC_WLAN_RX_NAPI_GRO
	ret |= NETIF_F_GRO;
#else
	feature |= NETIF_F_GRO;
	ret &= ~NETIF_F_GRO;
#endif

	KUNIT_EXPECT_EQ(test, ret, slsi_net_fix_features(dev, feature));
}

static void test_slsi_set_multicast_list(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct netdev_hw_addr mc_next;

	ndev_vif->vif_type = FAPI_VIFTYPE_STATION;
	slsi_set_multicast_list(dev);

	ndev_vif->is_available = 1;
	slsi_set_multicast_list(dev);

	dev->mc.count = 1;
	dev->mc.list.next = &mc_next.list;
	dev->mc.list.prev = &dev->mc.list;
	mc_next.list.next = &dev->mc.list;
	mc_next.list.prev = &dev->mc.list;
	slsi_set_multicast_list(dev);
}

static void test_slsi_set_mac_address(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct sockaddr *sa = kunit_kzalloc(test, sizeof(struct sockaddr), GFP_KERNEL);
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	struct netdev_hw_addr *ha = kunit_kzalloc(test, sizeof(struct netdev_hw_addr), GFP_KERNEL);

	dev->dev_addr = ha->addr;
	dev->addr_len = ETH_ALEN;
#else

	dev->dev_addr = kunit_kzalloc(test, sizeof(unsigned char) * ETH_ALEN, GFP_KERNEL);
#endif
	eth_random_addr(sa->sa_data);
	slsi_set_mac_address(dev, sa);
	KUNIT_EXPECT_STREQ(test, sa->sa_data, dev->dev_addr);
}

static void test_slsi_if_setup(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);

	slsi_if_setup(dev);
}

static void test_slsi_set_multicast_filter_work(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;

	slsi_set_multicast_filter_work(&ndev_vif->set_multicast_filter_work);

	slsi_set_multicast_filter_work(&ndev_vif->set_multicast_filter_work);

	ndev_vif->vif_type = FAPI_VIFTYPE_STATION;
	slsi_set_multicast_filter_work(&ndev_vif->set_multicast_filter_work);

	ndev_vif->sta.vif_status = SLSI_VIF_STATUS_CONNECTED;
	ndev_vif->ifnum = CONFIG_SCSC_WLAN_MAX_INTERFACES + 1;
	slsi_set_multicast_filter_work(&ndev_vif->set_multicast_filter_work);

	ndev_vif->ifnum = 0;
	slsi_set_multicast_filter_work(&ndev_vif->set_multicast_filter_work);

	ndev_vif->sdev->device_config.user_suspend_mode = 1;
	slsi_set_multicast_filter_work(&ndev_vif->set_multicast_filter_work);

	sdev->netdev[0] = dev;
	slsi_set_multicast_filter_work(&ndev_vif->set_multicast_filter_work);
}

static void test_slsi_update_pkt_filter_work(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;

	slsi_update_pkt_filter_work(&ndev_vif->update_pkt_filter_work);

	slsi_update_pkt_filter_work(&ndev_vif->update_pkt_filter_work);

	ndev_vif->is_available = 1;
	ndev_vif->activated = 1;
	ndev_vif->vif_type = FAPI_VIFTYPE_STATION;
	slsi_update_pkt_filter_work(&ndev_vif->update_pkt_filter_work);

	ndev_vif->sta.vif_status = SLSI_VIF_STATUS_CONNECTED;
	ndev_vif->ifnum = CONFIG_SCSC_WLAN_MAX_INTERFACES + 1;
	slsi_update_pkt_filter_work(&ndev_vif->update_pkt_filter_work);

	ndev_vif->ifnum = 0;
	slsi_update_pkt_filter_work(&ndev_vif->update_pkt_filter_work);

	sdev->netdev[0] = dev;
	sdev->device_config.user_suspend_mode = 1;
	ndev_vif->is_opt_out_packet = 1;
	slsi_update_pkt_filter_work(&ndev_vif->update_pkt_filter_work);

	sdev->device_config.user_suspend_mode = 0;
	slsi_update_pkt_filter_work(&ndev_vif->update_pkt_filter_work);
}

static void test_slsi_netif_add_locked(struct kunit *test)
{
	struct slsi_dev *sdev = kunit_kzalloc(test, sizeof(struct slsi_dev), GFP_KERNEL);
	struct net_device *dev = kunit_kzalloc(test, sizeof(struct net_device) + sizeof(struct netdev_vif), GFP_KERNEL);
	struct netdev_vif *ndev_vif;
	const char *name = "abc";
	int ifnum = 0;

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	ifnum = SLSI_NET_INDEX_P2P;
	sdev->netdev[ifnum] = NULL;
	sdev->netdev_up_count = 99;
	KUNIT_EXPECT_EQ(test, 1, slsi_netif_add_locked(sdev, name, ifnum));

	ifnum = SLSI_NET_INDEX_WLAN;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_add_locked(sdev, name, ifnum));
	slsi_netif_remove_locked(sdev, sdev->netdev[ifnum], false);

	ifnum = SLSI_NET_INDEX_AP;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_add_locked(sdev, name, ifnum));
	ndev_vif = netdev_priv(sdev->netdev[ifnum]);
	slsi_netif_remove_locked(sdev, sdev->netdev[ifnum], false);
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
}

static void test_slsi_netif_dynamic_iface_add(struct kunit *test)
{
	struct slsi_dev *sdev = TEST_TO_SDEV(test);
	const char *name = "abc";
	sdev->netdev[SLSI_NET_INDEX_P2PX_SWLAN] = kunit_kzalloc(test, sizeof(struct net_device), GFP_KERNEL);

	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_netif_dynamic_iface_add(sdev, name, NL80211_IFTYPE_AP_VLAN));
}

static void test_slsi_netif_init(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct slsi_dev *sdev = TEST_TO_SDEV(test);
	struct netdev_vif *ndev_vif;

	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_netif_init(sdev));

	sdev->netdev[SLSI_NET_INDEX_WLAN] = NULL;
	sdev->netdev[SLSI_NET_INDEX_AP] = dev;
	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_netif_init(sdev));

	sdev->netdev[SLSI_NET_INDEX_AP] = NULL;
	sdev->netdev[SLSI_NET_INDEX_AP2] = dev;
	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_netif_init(sdev));

	sdev->netdev[SLSI_NET_INDEX_AP2] = NULL;
	sdev->netdev[SLSI_NET_INDEX_P2P] = dev;
	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_netif_init(sdev));

	sdev->netdev[SLSI_NET_INDEX_P2P] = NULL;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_init(sdev));

	slsi_netif_remove_locked(sdev, sdev->netdev[SLSI_NET_INDEX_WLAN], false);
	slsi_netif_remove_locked(sdev, sdev->netdev[SLSI_NET_INDEX_AP], false);
	slsi_netif_remove_locked(sdev, sdev->netdev[SLSI_NET_INDEX_AP2], false);
	slsi_netif_remove_locked(sdev, sdev->netdev[SLSI_NET_INDEX_P2P], false);
}

static void test_slsi_netif_register_locked(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;
	unsigned char *addr = "\x00\x00\x0F\x11\x22\x33";

	ndev_vif->sdev = sdev;
	dev->dev_addr = addr;

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_register_locked(sdev, dev, 1));
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_register_locked(sdev, dev, 1));
	atomic_set(&ndev_vif->is_registered, 0);
	KUNIT_EXPECT_EQ(test, -EINVAL, slsi_netif_register_locked(sdev, dev, 0));
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
}

static void test_slsi_netif_register_rtlnl_locked(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct slsi_dev *sdev = TEST_TO_SDEV(test);
	unsigned char *addr = "\x00\x00\x0F\x11\x22\x33";

	dev->dev_addr = addr;

	KUNIT_EXPECT_EQ(test, 0, slsi_netif_register_rtlnl_locked(sdev, dev, 1));
}

static void test_slsi_netif_register(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct slsi_dev *sdev = TEST_TO_SDEV(test);
	unsigned char *addr = "\x00\x00\x0F\x11\x22\x33";

	dev->dev_addr = addr;

	KUNIT_EXPECT_EQ(test, 0, slsi_netif_register(sdev, dev, 1));
}

static void test_slsi_netif_remove_locked(struct kunit *test)
{
	struct slsi_dev *sdev = kunit_kzalloc(test, sizeof(struct slsi_dev), GFP_KERNEL);
	struct net_device *dev = kunit_kzalloc(test, sizeof(struct net_device) + sizeof(struct netdev_vif), GFP_KERNEL);
	struct netdev_vif *ndev_vif;
	const char *name = "abc";
	int ifnum = 0;

	SLSI_MUTEX_LOCK(sdev->netdev_add_remove_mutex);
	ifnum = SLSI_NET_INDEX_P2P;
	sdev->netdev[ifnum] = NULL;
	sdev->netdev_up_count = 99;
	KUNIT_EXPECT_EQ(test, 1, slsi_netif_add_locked(sdev, name, ifnum));

	ifnum = SLSI_NET_INDEX_WLAN;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_add_locked(sdev, name, ifnum));
	slsi_netif_remove_locked(sdev, sdev->netdev[ifnum], false);

	ifnum = SLSI_NET_INDEX_AP;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_add_locked(sdev, name, ifnum));
	ndev_vif = netdev_priv(sdev->netdev[ifnum]);
	slsi_netif_remove_locked(sdev, sdev->netdev[ifnum], false);
	SLSI_MUTEX_UNLOCK(sdev->netdev_add_remove_mutex);
}

static void test_slsi_netif_remove_rtlnl_locked(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;

	ndev_vif->ifnum = SLSI_NET_INDEX_P2P;
	dev->reg_state = NETREG_UNREGISTERING;
	slsi_netif_remove_rtlnl_locked(sdev, dev, 0);
}

static void test_slsi_netif_remove(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;

	ndev_vif->ifnum = SLSI_NET_INDEX_P2P;
	dev->reg_state = NETREG_UNREGISTERING;
	slsi_netif_remove(sdev, dev, 0);
}

static void test_slsi_netif_remove_all(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;

	ndev_vif->ifnum = SLSI_NET_INDEX_P2P;
	dev->reg_state = NETREG_UNREGISTERING;
	slsi_netif_remove_all(sdev, 0);
}

static void test_slsi_netif_deinit(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;

	ndev_vif->ifnum = SLSI_NET_INDEX_P2P;
	dev->reg_state = NETREG_UNREGISTERING;
	slsi_netif_deinit(sdev, 0);
}

#ifndef CONFIG_ARM
static void call_slsi_netif_tcp_ack_suppression_timeout(struct slsi_tcp_ack_s *tcp_ack)
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	slsi_netif_tcp_ack_suppression_timeout(&tcp_ack->timer);
#else
	slsi_netif_tcp_ack_suppression_timeout(tcp_ack);
#endif
}

static void test_slsi_netif_tcp_ack_suppression_timeout(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;
	struct net_device *dev_skb = kunit_kzalloc(test,
						   sizeof(struct net_device) + sizeof(struct netdev_vif), GFP_KERNEL);
	struct slsi_tcp_ack_s *tcp_ack = kunit_kzalloc(test, sizeof(struct slsi_tcp_ack_s), GFP_KERNEL);
	struct sk_buff *skb;

	tcp_ack->ndev_vif = ndev_vif;
	call_slsi_netif_tcp_ack_suppression_timeout(tcp_ack);

	skb = kzalloc(sizeof(struct sk_buff), GFP_KERNEL);
	tcp_ack->list.next = skb;
	tcp_ack->list.prev = &tcp_ack->list;
	skb->next = &tcp_ack->list;
	skb->prev = &tcp_ack->list;
	tcp_ack->state = 1;
	call_slsi_netif_tcp_ack_suppression_timeout(tcp_ack);

	skb = kzalloc(sizeof(struct sk_buff), GFP_KERNEL);
	tcp_ack->list.next = skb;
	tcp_ack->list.prev = &tcp_ack->list;
	skb->next = &tcp_ack->list;
	skb->prev = &tcp_ack->list;
	skb->dev = dev_skb;
	dev_skb->flags = 0;
	call_slsi_netif_tcp_ack_suppression_timeout(tcp_ack);

	tcp_ack->list.next = skb;
	tcp_ack->list.prev = &tcp_ack->list;
	skb->next = &tcp_ack->list;
	skb->prev = &tcp_ack->list;
	dev_skb->flags = 2;
	call_slsi_netif_tcp_ack_suppression_timeout(tcp_ack);

	tcp_ack->list.next = skb;
	tcp_ack->list.prev = &tcp_ack->list;
	skb->next = &tcp_ack->list;
	skb->prev = &tcp_ack->list;
	dev_skb->flags = 1;
	call_slsi_netif_tcp_ack_suppression_timeout(tcp_ack);
}

static void test_slsi_netif_tcp_ack_suppression_option(struct kunit *test)
{
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	u8 *head = kunit_kzalloc(test, sizeof(u8)*100, GFP_KERNEL);
	struct tcphdr *thdr = head;
	unsigned char *options;
	u32 option = TCP_ACK_SUPPRESSION_OPTION_EOL;

	skb->head = head;
	skb->transport_header = 0;
	thdr->doff = 6;
	options = ((u8 *)tcp_hdr(skb)) + TCP_ACK_SUPPRESSION_OPTIONS_OFFSET;

	options[0] = TCP_ACK_SUPPRESSION_OPTION_EOL;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_tcp_ack_suppression_option(skb, option));

	options[0] = TCP_ACK_SUPPRESSION_OPTION_NOP;
	options[1] = TCP_ACK_SUPPRESSION_OPTION_MSS;
	option = TCP_ACK_SUPPRESSION_OPTION_MSS;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_tcp_ack_suppression_option(skb, option));

	options[0] = TCP_ACK_SUPPRESSION_OPTION_MSS;
	options[1] = TCP_ACK_SUPPRESSION_OPTION_MSS;
	options[2] = TCP_ACK_SUPPRESSION_OPTION_WINDOW;
	options[3] = TCP_ACK_SUPPRESSION_OPTION_WINDOW;
	options[4] = 0;
	option = TCP_ACK_SUPPRESSION_OPTION_WINDOW;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_tcp_ack_suppression_option(skb, option));

	options[0] = TCP_ACK_SUPPRESSION_OPTION_WINDOW;
	options[1] = TCP_ACK_SUPPRESSION_OPTION_SACK;
	option = TCP_ACK_SUPPRESSION_OPTION_SACK;
	KUNIT_EXPECT_EQ(test, 1, slsi_netif_tcp_ack_suppression_option(skb, option));

	options[0] = TCP_ACK_SUPPRESSION_OPTION_SACK;
	options[1] = TCP_ACK_SUPPRESSION_OPTION_SACK;
	option = 0;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_tcp_ack_suppression_option(skb, option));

	options[0] = 4;
	options[1] = TCP_ACK_SUPPRESSION_OPTION_SACK;
	option = 0;
	KUNIT_EXPECT_EQ(test, 0, slsi_netif_tcp_ack_suppression_option(skb, option));
}

static void test_slsi_netif_tcp_ack_suppression_syn(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	u8 *head = kunit_kzalloc(test, sizeof(u8)*200, GFP_KERNEL);

	skb->head = head;
	skb->network_header = 0;
	skb->transport_header = 100;

	slsi_netif_tcp_ack_suppression_syn(dev, skb);

	ndev_vif->ack_suppression[0].state = 1;
	ndev_vif->ack_suppression[0].daddr = 1;

	ndev_vif->ack_suppression[1].state = 1;
	ndev_vif->ack_suppression[1].daddr = 0;

	ndev_vif->ack_suppression[2].state = 0;
	slsi_netif_tcp_ack_suppression_syn(dev, skb);

	tcp_ack_suppression_monitor = false;
	slsi_netif_tcp_ack_suppression_syn(dev, skb);
	tcp_ack_suppression_monitor = true;
}

static void test_slsi_netif_tcp_ack_suppression_fin(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	u8 *head = kunit_kzalloc(test, sizeof(u8)*200, GFP_KERNEL);

	skb->head = head;
	skb->network_header = 0;
	skb->transport_header = 100;

	slsi_netif_tcp_ack_suppression_fin(dev, skb);
}

static void test_slsi_netif_tcp_ack_suppression_is_possible(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	u8 *head = kunit_kzalloc(test, sizeof(u8)*200, GFP_KERNEL);
	u8 dev_addr[ETH_ALEN];

	skb->head = head;
	skb->network_header = 0;
	skb->transport_header = 100;
	dev->dev_addr = &dev_addr;

	eth_hdr(skb)->h_proto = cpu_to_be16(ETH_P_IP);
	ip_hdr(skb)->protocol = IPPROTO_TCP;
	tcp_hdr(skb)->syn = 1;
	KUNIT_EXPECT_TRUE(test, 0 == slsi_netif_tcp_ack_suppression_is_possible(dev, skb));

	tcp_hdr(skb)->syn = 0;
	tcp_hdr(skb)->fin = 1;
	KUNIT_EXPECT_TRUE(test, 0 == slsi_netif_tcp_ack_suppression_is_possible(dev, skb));

	tcp_hdr(skb)->fin = 0;
	tcp_hdr(skb)->ack = 1;
	KUNIT_EXPECT_TRUE(test, 1 == slsi_netif_tcp_ack_suppression_is_possible(dev, skb));
}

static void test_slsi_netif_tcp_ack_suppression_rate_cal(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct slsi_tcp_ack_s *tcp_ack = kunit_kzalloc(test, sizeof(struct slsi_tcp_ack_s), GFP_KERNEL);
	struct sk_buff *skb = kunit_kzalloc(test, sizeof(struct sk_buff), GFP_KERNEL);
	u8 *head = kunit_kzalloc(test, sizeof(u8)*200, GFP_KERNEL);

	skb->head = head;
	skb->transport_header = 0;
	tcp_ack->hysteresis = 0;
	tcp_ack->mss = 1;

	tcp_ack_suppression_monitor_interval = 1;
	tcp_ack->num_bytes = 125 * 100;
	tcp_ack->last_sample_time = 0;
	tcp_ack->last_ack_seq = 1;
	slsi_netif_tcp_ack_suppression_rate_cal(dev, tcp_ack, skb);

	tcp_ack->last_ack_seq = 0;
	tcp_ack->num_bytes = 125 * 20;
	tcp_ack->last_sample_time = 0;
	slsi_netif_tcp_ack_suppression_rate_cal(dev, tcp_ack, skb);

	tcp_ack->last_ack_seq = 0;
	tcp_ack->num_bytes = 125 * 1;
	tcp_ack->last_sample_time = 0;
	tcp_ack->window_multiplier = 1;
	slsi_netif_tcp_ack_suppression_rate_cal(dev, tcp_ack, skb);

	tcp_ack->last_ack_seq = 0;
	tcp_ack->num_bytes = 1;
	tcp_ack->last_tcp_rate = 1;
	tcp_ack->last_sample_time = 0;
	slsi_netif_tcp_ack_suppression_rate_cal(dev, tcp_ack, skb);
	tcp_ack_suppression_monitor_interval = 500;
}

static void test_slsi_netif_tcp_ack_suppression_pkt(struct kunit *test)
{
	struct net_device *dev = TEST_TO_DEV(test);
	struct netdev_vif *ndev_vif = netdev_priv(dev);
	struct slsi_dev *sdev = ndev_vif->sdev;
	struct slsi_tcp_ack_s *tcp_ack = ndev_vif->ack_suppression;
	struct sk_buff *skb = alloc_skb(100, GFP_KERNEL);
	struct sk_buff *skb2 = alloc_skb(100, GFP_KERNEL);
	u8 *head = kunit_kzalloc(test, sizeof(u8)*300, GFP_KERNEL);
	u8 dev_addr[ETH_ALEN];

	skb->head = head;
	skb->network_header = 0;
	skb->transport_header = 100;
	skb->mac_header = 200;
	dev->dev_addr = &dev_addr;

	eth_hdr(skb)->h_proto = cpu_to_be16(ETH_P_IP);
	ip_hdr(skb)->protocol = IPPROTO_TCP;
	tcp_hdr(skb)->syn = 0;
	tcp_hdr(skb)->fin = 0;
	tcp_hdr(skb)->ack = 1;
	tcp_hdr(skb)->rst = 0;
	tcp_hdr(skb)->urg = 0;
	KUNIT_EXPECT_PTR_EQ(test, skb, (struct sk_buff *)slsi_netif_tcp_ack_suppression_pkt(dev, skb));

	ndev_vif->last_tcp_ack = tcp_ack;
	tcp_ack->state = 1;
	tcp_hdr(skb)->ack_seq = cpu_to_be32(0);
	tcp_ack->ack_seq = 1;
	KUNIT_EXPECT_PTR_EQ(test, skb, (struct sk_buff *)slsi_netif_tcp_ack_suppression_pkt(dev, skb));

	skb_queue_head_init(&tcp_ack->list);
	tcp_hdr(skb)->ack_seq = cpu_to_be32(2);
	tcp_ack_suppression_monitor = false;
	tcp_ack_suppression_delay_acks_suppress = true;
	tcp_hdr(skb)->window = cpu_to_be16(128);
	tcp_ack->window_multiplier = 10;
	tcp_ack->age = 255;
	KUNIT_EXPECT_PTR_EQ(test, (struct sk_buff *)0, (struct sk_buff *)slsi_netif_tcp_ack_suppression_pkt(dev, skb));

	skb2->head = head;
	skb2->network_header = 0;
	skb2->transport_header = 100;
	skb2->mac_header = 200;
	eth_hdr(skb2)->h_proto = cpu_to_be16(ETH_P_IP);
	ip_hdr(skb2)->protocol = IPPROTO_TCP;
	tcp_hdr(skb2)->syn = 0;
	tcp_hdr(skb2)->fin = 0;
	tcp_hdr(skb2)->ack = 1;
	tcp_hdr(skb2)->rst = 0;
	tcp_hdr(skb2)->urg = 0;
	tcp_hdr(skb2)->ack_seq = cpu_to_be32(3);
	tcp_hdr(skb2)->window = cpu_to_be16(128);
	KUNIT_EXPECT_PTR_EQ(test, skb2, (struct sk_buff *)slsi_netif_tcp_ack_suppression_pkt(dev, skb2));
	kfree_skb(skb2);
}
#endif

static int netif_test_init(struct kunit *test)
{
	test_dev_init(test);

	kunit_log(KERN_INFO, test, "%s: initialized.", __func__);
	return 0;
}

static void netif_test_exit(struct kunit *test)
{
	kunit_log(KERN_INFO, test, "%s: completed.", __func__);
}

static struct kunit_case netif_test_cases[] = {
#ifdef CONFIG_SCSC_WIFI_NAN_ENABLE
	KUNIT_CASE(test_slsi_net_randomize_nmi_ndi),
#endif
	KUNIT_CASE(test_slsi_mac_address_init),
	KUNIT_CASE(test_slsi_netif_is_udp_pkt),
	KUNIT_CASE(test_slsi_netif_set_tid_change_tid),
	KUNIT_CASE(test_slsi_netif_set_tid_config),
	KUNIT_CASE(test_slsi_net_open),
	KUNIT_CASE(test_slsi_net_stop),
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	KUNIT_CASE(test_slsi_net_prv_ioctl),
#endif
	KUNIT_CASE(test_slsi_net_ioctl),
	KUNIT_CASE(test_slsi_net_get_stats),
#ifndef CONFIG_WLBT_WARN_ON
	KUNIT_CASE(test_slsi_netif_show_stats),
	KUNIT_CASE(test_slsi_net_tx_timeout),
#endif
#ifdef CONFIG_SCSC_USE_WMM_TOS
	KUNIT_CASE(test_slsi_get_priority_from_tos),
#else
	KUNIT_CASE(test_slsi_get_priority_from_tos_dscp),
#endif
	KUNIT_CASE(test_slsi_net_downgrade_ac),
	KUNIT_CASE(test_slsi_net_up_to_ac_mapping),
	KUNIT_CASE(test_slsi_frame_priority_to_ac_queue),
	KUNIT_CASE(test_slsi_ac_to_tids),
	KUNIT_CASE(test_slsi_net_downgrade_pri),
#ifndef CONFIG_SCSC_WLAN_TX_API
	KUNIT_CASE(test_slsi_net_select_queue),
	KUNIT_CASE(test_slsi_tdls_move_packets),
#endif
	KUNIT_CASE(test_slsi_net_hw_xmit),
	KUNIT_CASE(test_slsi_net_fix_features),
	KUNIT_CASE(test_slsi_set_multicast_list),
	KUNIT_CASE(test_slsi_set_mac_address),
	KUNIT_CASE(test_slsi_if_setup),
	KUNIT_CASE(test_slsi_set_multicast_filter_work),
	KUNIT_CASE(test_slsi_update_pkt_filter_work),
	KUNIT_CASE(test_slsi_netif_add_locked),
	KUNIT_CASE(test_slsi_netif_dynamic_iface_add),
	KUNIT_CASE(test_slsi_netif_init),
	KUNIT_CASE(test_slsi_netif_register_locked),
	KUNIT_CASE(test_slsi_netif_register_rtlnl_locked),
	KUNIT_CASE(test_slsi_netif_register),
	KUNIT_CASE(test_slsi_netif_remove_locked),
	KUNIT_CASE(test_slsi_netif_remove_rtlnl_locked),
	KUNIT_CASE(test_slsi_netif_remove),
	KUNIT_CASE(test_slsi_netif_remove_all),
	KUNIT_CASE(test_slsi_netif_deinit),
#ifndef CONFIG_ARM
	KUNIT_CASE(test_slsi_netif_tcp_ack_suppression_timeout),
	KUNIT_CASE(test_slsi_netif_tcp_ack_suppression_option),
	KUNIT_CASE(test_slsi_netif_tcp_ack_suppression_syn),
	KUNIT_CASE(test_slsi_netif_tcp_ack_suppression_fin),
	KUNIT_CASE(test_slsi_netif_tcp_ack_suppression_is_possible),
	KUNIT_CASE(test_slsi_netif_tcp_ack_suppression_rate_cal),
	KUNIT_CASE(test_slsi_netif_tcp_ack_suppression_pkt),
#endif
	{}
};

static struct kunit_suite netif_test_suite[] = {
	{
		.name = "kunit-netif-test",
		.test_cases = netif_test_cases,
		.init = netif_test_init,
		.exit = netif_test_exit,
	}
};

kunit_test_suites(netif_test_suite);
