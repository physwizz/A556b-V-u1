// SPDX-License-Identifier: GPL-2.0+
/*
 * u_ether.c -- Ethernet-over-USB link layer utilities for Gadget stack
 *
 * Copyright (C) 2003-2005,2008 David Brownell
 * Copyright (C) 2003-2004 Robert Schwebel, Benedikt Spranger
 * Copyright (C) 2008 Nokia Corporation
 */

/* #define VERBOSE_DEBUG */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <linux/hrtimer.h>
#include <linux/string_helpers.h>

#include "u_ether_mp.h"


/*
 * This component encapsulates the Ethernet link glue needed to provide
 * one (!) network link through the USB gadget stack, normally "usb0".
 *
 * The control and data models are handled by the function driver which
 * connects to this code; such as CDC Ethernet (ECM or EEM),
 * "CDC Subset", or RNDIS.  That includes all descriptor and endpoint
 * management.
 *
 * Link level addressing is handled by this component using module
 * parameters; if no such parameters are provided, random link level
 * addresses are used.  Each end of the link uses one address.  The
 * host end address is exported in various ways, and is often recorded
 * in configuration databases.
 *
 * The driver which assembles each configuration using such a link is
 * responsible for ensuring that each configuration includes at most one
 * instance of is network link.  (The network layer provides ways for
 * this single "physical" link to be used by multiple virtual links.)
 */

#define UETH__VERSION	"29-May-2008"

/* Experiments show that both Linux and Windows hosts allow up to 16k
 * frame sizes. Set the max MTU size to 15k+52 to prevent allocating 32k
 * blocks and still have efficient handling. */
#define GETHER_MAX_MTU_SIZE 15412
#define GETHER_MAX_ETH_FRAME_LEN (GETHER_MAX_MTU_SIZE + ETH_HLEN)

struct workqueue_struct	*uether_wq;

struct rndis_multipacket g_rndis_mp;
struct eth_dev *g_eth_dev;

/*-------------------------------------------------------------------------*/

#define RX_EXTRA	20	/* bytes guarding against rx overflows */

#define DEFAULT_QLEN	2	/* double buffering by default */

/* for dual-speed hardware, use deeper queues at high/super speed */
static inline int qlen(struct usb_gadget *gadget, unsigned qmult)
{
	if (gadget_is_dualspeed(gadget) && (gadget->speed == USB_SPEED_HIGH ||
					    gadget->speed >= USB_SPEED_SUPER))
		return qmult * DEFAULT_QLEN;
	else
		return DEFAULT_QLEN;
}

/*-------------------------------------------------------------------------*/

/* REVISIT there must be a better way than having two sets
 * of debug calls ...
 */

#undef DBG
#undef VDBG
#undef ERROR
#undef INFO

#define xprintk(d, level, fmt, args...) \
	printk(level "%s: " fmt , (d)->net->name , ## args)

#ifdef DEBUG
#undef DEBUG
#define DBG(dev, fmt, args...) \
	xprintk(dev , KERN_DEBUG , fmt , ## args)
#else
#define DBG(dev, fmt, args...) \
	do { } while (0)
#endif /* DEBUG */

#ifdef VERBOSE_DEBUG
#define VDBG	DBG
#else
#define VDBG(dev, fmt, args...) \
	do { } while (0)
#endif /* DEBUG */

#define ERROR(dev, fmt, args...) \
	xprintk(dev , KERN_ERR , fmt , ## args)
#define INFO(dev, fmt, args...) \
	xprintk(dev , KERN_INFO , fmt , ## args)

/*-------------------------------------------------------------------------*/

/* NETWORK DRIVER HOOKUP (to the layer above this driver) */

static void mp_eth_get_drvinfo(struct net_device *net, struct ethtool_drvinfo *p)
{
	struct eth_dev *dev = netdev_priv(net);

	strscpy(p->driver, "g_ether", sizeof(p->driver));
	strscpy(p->version, UETH__VERSION, sizeof(p->version));
	strscpy(p->fw_version, dev->gadget->name, sizeof(p->fw_version));
	strscpy(p->bus_info, dev_name(&dev->gadget->dev), sizeof(p->bus_info));
}

/* REVISIT can also support:
 *   - WOL (by tracking suspends and issuing remote wakeup)
 *   - msglevel (implies updated messaging)
 *   - ... probably more ethtool ops
 */

static const struct ethtool_ops ops = {
	.get_drvinfo = mp_eth_get_drvinfo,
	.get_link = ethtool_op_get_link,
};

static void mp_defer_kevent(struct eth_dev *dev, int flag)
{
	if (test_and_set_bit(flag, &dev->todo))
		return;
	if (!schedule_work(&dev->work))
		ERROR(dev, "kevent %d may have been dropped\n", flag);
	else
		DBG(dev, "kevent %d scheduled\n", flag);
}

static void mp_rx_complete(struct usb_ep *ep, struct usb_request *req);

static int
mp_rx_submit(struct eth_dev *dev, struct usb_request *req, gfp_t gfp_flags)
{
	struct usb_gadget *g = dev->gadget;
	struct sk_buff	*skb;
	int		retval = -ENOMEM;
	size_t		size = 0;
	struct usb_ep	*out;
	unsigned long	flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (dev->port_usb)
		out = dev->port_usb->out_ep;
	else
		out = NULL;

	if (!out)
	{
		spin_unlock_irqrestore(&dev->lock, flags);
		return -ENOTCONN;
	}

	/* Padding up to RX_EXTRA handles minor disagreements with host.
	 * Normally we use the USB "terminate on short read" convention;
	 * so allow up to (N*maxpacket), since that memory is normally
	 * already allocated.  Some hardware doesn't deal well with short
	 * reads (e.g. DMA must be N*maxpacket), so for now don't trim a
	 * byte off the end (to force hardware errors on overflow).
	 *
	 * RNDIS uses internal framing, and explicitly allows senders to
	 * pad to end-of-packet.  That's potentially nice for speed, but
	 * means receivers can't recover lost synch on their own (because
	 * new packets don't only start after a short RX).
	 */
	size += sizeof(struct ethhdr) + dev->net->mtu + RX_EXTRA;
	size += dev->port_usb->header_len;

	if (g->quirk_ep_out_aligned_size) {
		size += out->maxpacket - 1;
		size -= size % out->maxpacket;
	}

	if (g_rndis_mp.ul_max_pkts_per_xfer)
		size *= g_rndis_mp.ul_max_pkts_per_xfer;

	if (dev->port_usb->is_fixed)
		size = max_t(size_t, size, dev->port_usb->fixed_out_len);
	spin_unlock_irqrestore(&dev->lock, flags);

	DBG(dev, "%s: size: %zd\n", __func__, size);
	skb = __netdev_alloc_skb(dev->net, size + NET_IP_ALIGN, gfp_flags);
	if (skb == NULL) {
		DBG(dev, "no rx skb\n");
		goto enomem;
	}

	/* Some platforms perform better when IP packets are aligned,
	 * but on at least one, checksumming fails otherwise.  Note:
	 * RNDIS headers involve variable numbers of LE32 values.
	 */
	skb_reserve(skb, NET_IP_ALIGN);

	req->buf = skb->data;
	req->length = size;
	req->complete = mp_rx_complete;
	req->context = skb;

	retval = usb_ep_queue(out, req, gfp_flags);
	if (retval == -ENOMEM)
enomem:
		mp_defer_kevent(dev, WORK_RX_MEMORY);
	if (retval) {
		DBG(dev, "rx submit --> %d\n", retval);
		if (skb)
			dev_kfree_skb_any(skb);
	}
	return retval;
}

static void mp_rx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct sk_buff	*skb = req->context;
	struct eth_dev	*dev = ep->driver_data;
	int		status = req->status;
	bool		queue = 0;

	switch (status) {

	/* normal completion */
	case 0:
		skb_put(skb, req->actual);

		if (dev->unwrap) {
			unsigned long	flags;

			spin_lock_irqsave(&dev->lock, flags);
			if (dev->port_usb) {
				status = dev->unwrap(dev->port_usb,
							skb,
							&dev->rx_frames);
				if (status == -EINVAL)
					dev->net->stats.rx_errors++;
				else if (status == -EOVERFLOW)
					dev->net->stats.rx_over_errors++;
			} else {
				dev_kfree_skb_any(skb);
				status = -ENOTCONN;
			}
			spin_unlock_irqrestore(&dev->lock, flags);
		} else {
			skb_queue_tail(&dev->rx_frames, skb);
		}
		if (!status)
			queue = 1;
		break;

	/* software-driven interface shutdown */
	case -ECONNRESET:		/* unlink */
	case -ESHUTDOWN:		/* disconnect etc */
		VDBG(dev, "rx shutdown, code %d\n", status);
		goto quiesce;

	/* for hardware automagic (such as pxa) */
	case -ECONNABORTED:		/* endpoint reset */
		DBG(dev, "rx %s reset\n", ep->name);
		mp_defer_kevent(dev, WORK_RX_MEMORY);
quiesce:
		dev_kfree_skb_any(skb);
		goto clean;

	/* data overrun */
	case -EOVERFLOW:
		dev->net->stats.rx_over_errors++;
		fallthrough;
		/* FALLTHROUGH */

	default:
		queue = 1;
		dev_kfree_skb_any(skb);
		dev->net->stats.rx_errors++;
		DBG(dev, "rx status %d\n", status);
		break;
	}

clean:
		spin_lock(&dev->req_lock);
		list_add(&req->list, &dev->rx_reqs);
		spin_unlock(&dev->req_lock);

	if (queue)
		queue_work_on((nr_cpu_ids - 1), uether_wq, &g_rndis_mp.rx_work);
}

static int mp_prealloc(struct list_head *list, struct usb_ep *ep, unsigned n)
{
	unsigned		i;
	struct usb_request	*req;

	if (!n)
		return -ENOMEM;

	/* queue/recycle up to N requests */
	i = n;
	list_for_each_entry(req, list, list) {
		if (i-- == 0)
			goto extra;
	}
	while (i--) {
		req = usb_ep_alloc_request(ep, GFP_ATOMIC);
		if (!req)
			return list_empty(list) ? -ENOMEM : 0;
		list_add(&req->list, list);
	}
	return 0;

extra:
	/* free extras */
	for (;;) {
		struct list_head	*next;

		next = req->list.next;
		list_del(&req->list);
		usb_ep_free_request(ep, req);

		if (next == list)
			break;

		req = container_of(next, struct usb_request, list);
	}
	return 0;
}

static int mp_alloc_requests(struct eth_dev *dev, struct gether *link, unsigned n)
{
	int	status;

	spin_lock(&dev->req_lock);
	status = mp_prealloc(&dev->tx_reqs, link->in_ep, n);
	if (status < 0)
		goto fail;
	status = mp_prealloc(&dev->rx_reqs, link->out_ep, n);
	if (status < 0)
		goto fail;
	goto done;
fail:
	DBG(dev, "can't alloc requests\n");
done:
	spin_unlock(&dev->req_lock);
	return status;
}

static void mp_rx_fill(struct eth_dev *dev, gfp_t gfp_flags)
{
	struct usb_request	*req;
	unsigned long		flags;

	/* fill unused rxq slots with some skb */
	spin_lock_irqsave(&dev->req_lock, flags);
	while (!list_empty(&dev->rx_reqs)) {
		req = list_first_entry(&dev->rx_reqs, struct usb_request, list);
		list_del_init(&req->list);
		spin_unlock_irqrestore(&dev->req_lock, flags);

		if (mp_rx_submit(dev, req, gfp_flags) < 0) {
			spin_lock_irqsave(&dev->req_lock, flags);
			list_add(&req->list, &dev->rx_reqs);
			spin_unlock_irqrestore(&dev->req_lock, flags);
			mp_defer_kevent(dev, WORK_RX_MEMORY);
			return;
		}

		spin_lock_irqsave(&dev->req_lock, flags);
	}
	spin_unlock_irqrestore(&dev->req_lock, flags);
}

static void mp_process_rx_w(struct work_struct *work)
{
	struct eth_dev  *dev = g_eth_dev;
	struct sk_buff  *skb;
	int             status = 0;

	if (!dev->port_usb)
		return;

	while ((skb = skb_dequeue(&dev->rx_frames))) {
		if (status < 0
				|| ETH_HLEN > skb->len
				|| skb->len > ETH_FRAME_LEN) {
			dev->net->stats.rx_errors++;
			dev->net->stats.rx_length_errors++;
			DBG(dev, "rx length %d\n", skb->len);
			dev_kfree_skb_any(skb);
			continue;
		}
		skb->protocol = eth_type_trans(skb, dev->net);
		dev->net->stats.rx_packets++;
		dev->net->stats.rx_bytes += skb->len;

		status = netif_rx(skb);
	}

	if (netif_running(dev->net))
		mp_rx_fill(dev, GFP_KERNEL);
}

static void mp_eth_work(struct work_struct *work)
{
	struct eth_dev	*dev = container_of(work, struct eth_dev, work);

	if (test_and_clear_bit(WORK_RX_MEMORY, &dev->todo)) {
		if (netif_running(dev->net))
			mp_rx_fill(dev, GFP_KERNEL);
	}

	if (dev->todo)
		DBG(dev, "work done, flags = 0x%lx\n", dev->todo);
}

static void mp_tx_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct sk_buff	*skb = req->context;
	struct eth_dev	*dev = ep->driver_data;

	switch (req->status) {
	default:
		dev->net->stats.tx_errors++;
		VDBG(dev, "tx err %d\n", req->status);
		fallthrough;
		/* FALLTHROUGH */
	case -ECONNRESET:		/* unlink */
	case -ESHUTDOWN:		/* disconnect etc */
		dev_kfree_skb_any(skb);
		break;
	case 0:
		if (!req->zero && !dev->zlp)
			dev->net->stats.tx_bytes += req->length-1;
		else
			dev->net->stats.tx_bytes += req->length;

		dev_consume_skb_any(skb);
	}
	dev->net->stats.tx_packets++;

	spin_lock(&dev->req_lock);
	list_add_tail(&req->list, &dev->tx_reqs);
	if (g_rndis_mp.multi_pkt_xfer)
		req->length = 0;
	spin_unlock(&dev->req_lock);
	if (netif_carrier_ok(dev->net))
		netif_wake_queue(dev->net);
}

static inline int mp_is_promisc(u16 cdc_filter)
{
	return cdc_filter & USB_CDC_PACKET_TYPE_PROMISCUOUS;
}

static void mp_alloc_tx_buffer(struct eth_dev *dev)
{
	struct list_head	*act;
	struct usb_request	*req;

	g_rndis_mp.tx_req_bufsize = (g_rndis_mp.dl_max_pkts_per_xfer *
				(dev->net->mtu
				+ sizeof(struct ethhdr)
				/* size of rndis_packet_msg_type */
				+ 44
				+ 22));

	list_for_each(act, &dev->tx_reqs) {
		req = container_of(act, struct usb_request, list);
		if (!req->buf)
			req->buf = kmalloc(g_rndis_mp.tx_req_bufsize,
						GFP_ATOMIC);
	}
}

static int mp_tx_task(struct eth_dev *dev, struct usb_request *req)
{
	struct usb_ep *in = dev->port_usb->in_ep;
	int length = req->length;
	int retval;

	req->complete = mp_tx_complete;

	/* NCM requires no zlp if transfer is dwNtbInMaxSize */
	if (dev->port_usb->is_fixed && length == dev->port_usb->fixed_in_len &&
		(length % in->maxpacket) == 0)
		req->zero = 0;
	else
		req->zero = 1;

	/* use zlp framing on tx for strict CDC-Ether conformance,
	 * though any robust network rx path ignores extra padding.
	 * and some hardware doesn't like to write zlps.
	 */
	if (req->zero && !dev->zlp && (length % in->maxpacket) == 0) {
		req->zero = 0;
		length++;
	}
	req->length = length;

	/* throttle highspeed IRQ rate back slightly */
	if (gadget_is_dualspeed(dev->gadget) &&
		(dev->gadget->speed == USB_SPEED_HIGH)) {
		atomic_inc(&dev->tx_qlen);
		if (atomic_read(&dev->tx_qlen) == (dev->qmult/2)) {
			req->no_interrupt = 0;
			atomic_set(&dev->tx_qlen, 0);
		} else {
			req->no_interrupt = 1;
		}
	} else {
		req->no_interrupt = 0;
	}
	retval = usb_ep_queue(in, req, GFP_ATOMIC);

	return retval;
}

#if IS_ENABLED(CONFIG_USB_F_RNDIS_MP)
static enum hrtimer_restart mp_tx_timeout(struct hrtimer *data)
{
	struct eth_dev *dev = g_eth_dev;
	struct usb_request *req = NULL;

	int retval;
	unsigned long flags;

	spin_lock_irqsave(&dev->req_lock, flags);

	/*
	 * this freelist can be empty if an interrupt triggered disconnect()
	 * and reconfigured the gadget (shutting down this queue) after the
	 * network stack decided to xmit but before we got the spinlock.
	 */

	if (list_empty(&dev->tx_reqs)) {
		spin_unlock_irqrestore(&dev->req_lock, flags);
		pr_info("\n\n%s: TX REQS list empty!\n\n", __func__);
		return HRTIMER_NORESTART;
	}

	req = container_of(dev->tx_reqs.next, struct usb_request, list);

	list_del(&req->list);

	/* temporarily stop TX queue when the freelist empties */
	if (list_empty(&dev->tx_reqs))
		netif_stop_queue(dev->net);

	spin_unlock_irqrestore(&dev->req_lock, flags);

	g_rndis_mp.occurred_timeout = 1;
	retval = mp_tx_task(dev, req);
	switch (retval) {
	default:
		DBG(dev, "tx queue err %d\n", retval);
		break;
	}

	if (retval) {
		req->length = 0;
		dev->net->stats.tx_dropped++;
		spin_lock_irqsave(&dev->req_lock, flags);
		if (list_empty(&dev->tx_reqs))
			netif_start_queue(dev->net);
		list_add(&req->list, &dev->tx_reqs);
		spin_unlock_irqrestore(&dev->req_lock, flags);
	}

	return HRTIMER_NORESTART;
}
#endif

static netdev_tx_t mp_eth_start_xmit(struct sk_buff *skb,
		struct net_device *net)
{
	struct eth_dev		*dev = netdev_priv(net);
	int			length = 0;
	int			retval;
	struct usb_request	*req = NULL;
	unsigned long		flags;
	struct usb_ep		*in;
	u16			cdc_filter;
	unsigned long	tx_timeout;

	if (g_rndis_mp.en_timer) {
		hrtimer_cancel(&g_rndis_mp.tx_timer);
		g_rndis_mp.en_timer = 0;
	}

	spin_lock_irqsave(&dev->lock, flags);
	if (dev->port_usb) {
		in = dev->port_usb->in_ep;
		cdc_filter = dev->port_usb->cdc_filter;
	} else {
		in = NULL;
		cdc_filter = 0;
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	if (!in) {
		if (skb)
			dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	/* Allocate memory for tx_reqs to support multi packet transfer */
	if (g_rndis_mp.multi_pkt_xfer && !g_rndis_mp.tx_req_bufsize)
		mp_alloc_tx_buffer(dev);

	/* apply outgoing CDC or RNDIS filters */
	if (skb && !mp_is_promisc(cdc_filter)) {
		u8		*dest = skb->data;

		if (is_multicast_ether_addr(dest)) {
			u16	type;

			/* ignores USB_CDC_PACKET_TYPE_MULTICAST and host
			 * SET_ETHERNET_MULTICAST_FILTERS requests
			 */
			if (is_broadcast_ether_addr(dest))
				type = USB_CDC_PACKET_TYPE_BROADCAST;
			else
				type = USB_CDC_PACKET_TYPE_ALL_MULTICAST;
			if (!(cdc_filter & type)) {
				dev_kfree_skb_any(skb);
				return NETDEV_TX_OK;
			}
		}
		/* ignores USB_CDC_PACKET_TYPE_DIRECTED */
	}

	spin_lock_irqsave(&dev->req_lock, flags);
	/*
	 * this freelist can be empty if an interrupt triggered disconnect()
	 * and reconfigured the gadget (shutting down this queue) after the
	 * network stack decided to xmit but before we got the spinlock.
	 */
	if (list_empty(&dev->tx_reqs)) {
		spin_unlock_irqrestore(&dev->req_lock, flags);
		return NETDEV_TX_BUSY;
	}

	req = list_first_entry(&dev->tx_reqs, struct usb_request, list);
	list_del(&req->list);

	/* temporarily stop TX queue when the freelist empties */
	if (list_empty(&dev->tx_reqs))
		netif_stop_queue(net);
	spin_unlock_irqrestore(&dev->req_lock, flags);

	/* no buffer copies needed, unless the network stack did it
	 * or the hardware can't use skb buffers.
	 * or there's not enough space for extra headers we need
	 */
	if (dev->wrap) {
		unsigned long	flags;

		spin_lock_irqsave(&dev->lock, flags);
		if (dev->port_usb)
			skb = dev->wrap(dev->port_usb, skb);
		spin_unlock_irqrestore(&dev->lock, flags);
		if (!skb) {
			/* Multi frame CDC protocols may store the frame for
			 * later which is not a dropped frame.
			 */
			if (dev->port_usb->supports_multi_frame)
				goto multiframe;
			goto drop;
		}
	}

	spin_lock_irqsave(&dev->req_lock, flags);
	g_rndis_mp.tx_skb_hold_count++;
	spin_unlock_irqrestore(&dev->req_lock, flags);

	if (skb) {
		if (g_rndis_mp.multi_pkt_xfer) {
			memcpy(req->buf + req->length, skb->data, skb->len);
			req->length = req->length + skb->len;
			length = req->length;
			dev_kfree_skb_any(skb);

			spin_lock_irqsave(&dev->req_lock, flags);
			if (g_rndis_mp.tx_skb_hold_count < g_rndis_mp.dl_max_pkts_per_xfer) {
				list_add(&req->list, &dev->tx_reqs);
				spin_unlock_irqrestore(&dev->req_lock, flags);

				tx_timeout = g_rndis_mp.occurred_timeout ?
					MIN_TX_TIMEOUT_NSECS : MAX_TX_TIMEOUT_NSECS;
				g_rndis_mp.occurred_timeout = 0;
				hrtimer_start(&g_rndis_mp.tx_timer, ktime_set(0, tx_timeout),
						HRTIMER_MODE_REL);
				g_rndis_mp.en_timer = 1;
				goto success;
			}

			spin_unlock_irqrestore(&dev->req_lock, flags);

			spin_lock_irqsave(&dev->lock, flags);
			g_rndis_mp.tx_skb_hold_count = 0;
			spin_unlock_irqrestore(&dev->lock, flags);
		} else {
			req->length = skb->len;
			req->buf = skb->data;
			req->context = skb;
		}
	}

	retval = mp_tx_task(dev, req);
	switch (retval) {
		default:
			DBG(dev, "tx queue err %d\n", retval);
			break;
	}

	if (retval) {
		if (!g_rndis_mp.multi_pkt_xfer)
			dev_kfree_skb_any(skb);
drop:
		dev->net->stats.tx_dropped++;
multiframe:
		spin_lock_irqsave(&dev->req_lock, flags);
		if (list_empty(&dev->tx_reqs))
			netif_start_queue(net);
		list_add(&req->list, &dev->tx_reqs);
		spin_unlock_irqrestore(&dev->req_lock, flags);
	}
success:
	return NETDEV_TX_OK;
}

/*-------------------------------------------------------------------------*/

static void mp_eth_start(struct eth_dev *dev, gfp_t gfp_flags)
{
	pr_info("<<< %s\n", __func__);

	DBG(dev, "%s\n", __func__);

	/* fill the rx queue */
	mp_rx_fill(dev, gfp_flags);

	/* and open the tx floodgates */
	g_rndis_mp.occurred_timeout = 1;
	atomic_set(&dev->tx_qlen, 0);
	netif_wake_queue(dev->net);
}

static int mp_eth_open(struct net_device *net)
{
	struct eth_dev	*dev = netdev_priv(net);
	struct gether	*link;

	pr_info("<<< %s\n", __func__);

	DBG(dev, "%s\n", __func__);
	if (netif_carrier_ok(dev->net))
		mp_eth_start(dev, GFP_KERNEL);

	spin_lock_irq(&dev->lock);
	link = dev->port_usb;
	if (link && link->open)
		link->open(link);
	spin_unlock_irq(&dev->lock);

	return 0;
}

static int mp_eth_stop(struct net_device *net)
{
	struct eth_dev	*dev = netdev_priv(net);
	unsigned long	flags;

	pr_info("<<< %s\n", __func__);

	VDBG(dev, "%s\n", __func__);
	netif_stop_queue(net);

	DBG(dev, "stop stats: rx/tx %ld/%ld, errs %ld/%ld\n",
		dev->net->stats.rx_packets, dev->net->stats.tx_packets,
		dev->net->stats.rx_errors, dev->net->stats.tx_errors
		);

	/* ensure there are no more active requests */
	spin_lock_irqsave(&dev->lock, flags);
	if (dev->port_usb) {
		struct gether	*link = dev->port_usb;
		const struct usb_endpoint_descriptor *in;
		const struct usb_endpoint_descriptor *out;

		if (link->close)
			link->close(link);

		/* NOTE:  we have no abort-queue primitive we could use
		 * to cancel all pending I/O.  Instead, we disable then
		 * reenable the endpoints ... this idiom may leave toggle
		 * wrong, but that's a self-correcting error.
		 *
		 * REVISIT:  we *COULD* just let the transfers complete at
		 * their own pace; the network stack can handle old packets.
		 * For the moment we leave this here, since it works.
		 */
		in = link->in_ep->desc;
		out = link->out_ep->desc;
		usb_ep_disable(link->in_ep);
		usb_ep_disable(link->out_ep);
		if (netif_carrier_ok(net)) {
			DBG(dev, "host still using in/out endpoints\n");
			link->in_ep->desc = in;
			link->out_ep->desc = out;
			usb_ep_enable(link->in_ep);
			usb_ep_enable(link->out_ep);
		}
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

/*-------------------------------------------------------------------------*/

static int mp_get_ether_addr(const char *str, u8 *dev_addr)
{
	if (str) {
		unsigned	i;

		for (i = 0; i < 6; i++) {
			unsigned char num;

			if ((*str == '.') || (*str == ':'))
				str++;
			num = hex_to_bin(*str++) << 4;
			num |= hex_to_bin(*str++);
			dev_addr [i] = num;
		}
		if (is_valid_ether_addr(dev_addr))
			return 0;
	}
	eth_random_addr(dev_addr);
	return 1;
}

static int mp_get_ether_addr_str(u8 dev_addr[ETH_ALEN], char *str, int len)
{
	if (len < 18)
		return -EINVAL;

	snprintf(str, len, "%pM", dev_addr);
	return 18;
}

static const struct net_device_ops mp_eth_netdev_ops = {
	.ndo_open		= mp_eth_open,
	.ndo_stop		= mp_eth_stop,
	.ndo_start_xmit		= mp_eth_start_xmit,
	.ndo_set_mac_address 	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

static struct device_type mp_gadget_type = {
	.name	= "gadget",
};

/*
 * mp_gether_setup_name - initialize one ethernet-over-usb link
 * @g: gadget to associated with these links
 * @ethaddr: NULL, or a buffer in which the ethernet address of the
 *	host side of the link is recorded
 * @netname: name for network device (for example, "usb")
 * Context: may sleep
 *
 * This sets up the single network link that may be exported by a
 * gadget driver using this framework.  The link layer addresses are
 * set up using module parameters.
 *
 * Returns an eth_dev pointer on success, or an ERR_PTR on failure.
 */
struct eth_dev *mp_gether_setup_name(struct usb_gadget *g,
		const char *dev_addr, const char *host_addr,
		u8 ethaddr[ETH_ALEN], unsigned qmult, const char *netname)
{
	struct eth_dev		*dev;
	struct net_device	*net;
	int			status;
	u8			addr[ETH_ALEN];

	pr_info("<<< %s\n", __func__);

	net = alloc_etherdev(sizeof *dev);
	if (!net)
		return ERR_PTR(-ENOMEM);

	dev = netdev_priv(net);
	spin_lock_init(&dev->lock);
	spin_lock_init(&dev->req_lock);
	INIT_WORK(&dev->work, mp_eth_work);
	INIT_WORK(&g_rndis_mp.rx_work, mp_process_rx_w);
	INIT_LIST_HEAD(&dev->tx_reqs);
	INIT_LIST_HEAD(&dev->rx_reqs);

	skb_queue_head_init(&dev->rx_frames);

	/* network device setup */
	dev->net = net;
	dev->qmult = qmult;
	snprintf(net->name, sizeof(net->name), "%s%%d", netname);

	if (mp_get_ether_addr(dev_addr, addr)) {
		net->addr_assign_type = NET_ADDR_RANDOM;
		dev_warn(&g->dev,
			"using random %s ethernet address\n", "self");
	} else {
		net->addr_assign_type = NET_ADDR_SET;
	}
	eth_hw_addr_set(net, addr);
	if (mp_get_ether_addr(host_addr, dev->host_mac))
		dev_warn(&g->dev,
			"using random %s ethernet address\n", "host");

	if (ethaddr)
		memcpy(ethaddr, dev->host_mac, ETH_ALEN);

	net->netdev_ops = &mp_eth_netdev_ops;

	net->ethtool_ops = &ops;

	/* MTU range: 14 - 15412 */
	net->min_mtu = ETH_HLEN;
	net->max_mtu = GETHER_MAX_MTU_SIZE;

	dev->gadget = g;
	SET_NETDEV_DEV(net, &g->dev);
	SET_NETDEV_DEVTYPE(net, &mp_gadget_type);

	status = register_netdev(net);
	if (status < 0) {
		dev_dbg(&g->dev, "register_netdev failed, %d\n", status);
		free_netdev(net);
		dev = ERR_PTR(status);
	} else {
		INFO(dev, "MAC %pM\n", net->dev_addr);
		INFO(dev, "HOST MAC %pM\n", dev->host_mac);

		/*
		 * two kinds of host-initiated state changes:
		 *  - iff DATA transfer is active, carrier is "on"
		 *  - tx queueing enabled if open *and* carrier is "on"
		 */
		netif_carrier_off(net);
	}

	return dev;
}

struct net_device *mp_gether_setup_name_default(const char *netname)
{
	struct net_device	*net;
	struct eth_dev		*dev;
	pr_info("<<< %s: rndis multi-packet support!\n", __func__);
	net = alloc_etherdev(sizeof(*dev));
	if (!net)
		return ERR_PTR(-ENOMEM);

	dev = netdev_priv(net);
	spin_lock_init(&dev->lock);
	spin_lock_init(&dev->req_lock);
	INIT_WORK(&dev->work, mp_eth_work);
#if IS_ENABLED(CONFIG_USB_F_RNDIS_MP)
	g_eth_dev = dev;
	INIT_WORK(&g_rndis_mp.rx_work, mp_process_rx_w);
#endif
	INIT_LIST_HEAD(&dev->tx_reqs);
	INIT_LIST_HEAD(&dev->rx_reqs);

#if IS_ENABLED(CONFIG_USB_F_RNDIS_MP)
	hrtimer_init(&g_rndis_mp.tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_rndis_mp.tx_timer.function = mp_tx_timeout;
#endif

	/* by default we always have a random MAC address */
	net->addr_assign_type = NET_ADDR_RANDOM;

	skb_queue_head_init(&dev->rx_frames);

	/* network device setup */
	dev->net = net;
	dev->qmult = QMULT_DEFAULT;
	snprintf(net->name, sizeof(net->name), "%s%%d", netname);

	eth_random_addr(dev->dev_mac);
	pr_warn("using random %s ethernet address\n", "self");

	/* by default we always have a random MAC address */
	net->addr_assign_type = NET_ADDR_RANDOM;

	eth_random_addr(dev->host_mac);
	pr_warn("using random %s ethernet address\n", "host");

	net->netdev_ops = &mp_eth_netdev_ops;

	net->ethtool_ops = &ops;
	SET_NETDEV_DEVTYPE(net, &mp_gadget_type);

	/* MTU range: 14 - 15412 */
	net->min_mtu = ETH_HLEN;
	net->max_mtu = GETHER_MAX_MTU_SIZE;

	return net;
}
EXPORT_SYMBOL_GPL(mp_gether_setup_name_default);

int mp_gether_register_netdev(struct net_device *net)
{
	struct eth_dev *dev;
	struct usb_gadget *g;
	int status;

	if (!net->dev.parent)
		return -EINVAL;
	dev = netdev_priv(net);
	g = dev->gadget;

	eth_hw_addr_set(net, dev->dev_mac);

	status = register_netdev(net);
	if (status < 0) {
		dev_dbg(&g->dev, "register_netdev failed, %d\n", status);
		return status;
	} else {
		INFO(dev, "HOST MAC %pM\n", dev->host_mac);
		INFO(dev, "MAC %pM\n", dev->dev_mac);

		/* two kinds of host-initiated state changes:
		 *  - iff DATA transfer is active, carrier is "on"
		 *  - tx queueing enabled if open *and* carrier is "on"
		 */
		netif_carrier_off(net);
	}

	return status;
}

void mp_gether_set_gadget(struct net_device *net, struct usb_gadget *g)
{
	struct eth_dev *dev;

	dev = netdev_priv(net);
	dev->gadget = g;
	SET_NETDEV_DEV(net, &g->dev);
}

int mp_gether_set_dev_addr(struct net_device *net, const char *dev_addr)
{
	struct eth_dev *dev;
	u8 new_addr[ETH_ALEN];

	dev = netdev_priv(net);
	if (mp_get_ether_addr(dev_addr, new_addr))
		return -EINVAL;
	memcpy(dev->dev_mac, new_addr, ETH_ALEN);
	net->addr_assign_type = NET_ADDR_SET;
	return 0;
}

int mp_gether_get_dev_addr(struct net_device *net, char *dev_addr, int len)
{
	struct eth_dev *dev;
	int ret;

	dev = netdev_priv(net);
	ret = mp_get_ether_addr_str(dev->dev_mac, dev_addr, len);
	if (ret + 1 < len) {
		dev_addr[ret++] = '\n';
		dev_addr[ret] = '\0';
	}

	return ret;
}

int mp_gether_set_host_addr(struct net_device *net, const char *host_addr)
{
	struct eth_dev *dev;
	u8 new_addr[ETH_ALEN];

	dev = netdev_priv(net);
	if (mp_get_ether_addr(host_addr, new_addr))
		return -EINVAL;
	memcpy(dev->host_mac, new_addr, ETH_ALEN);
	return 0;
}

int mp_gether_get_host_addr(struct net_device *net, char *host_addr, int len)
{
	struct eth_dev *dev;
	int ret;

	dev = netdev_priv(net);
	ret = mp_get_ether_addr_str(dev->host_mac, host_addr, len);
	if (ret + 1 < len) {
		host_addr[ret++] = '\n';
		host_addr[ret] = '\0';
	}

	return ret;
}

int mp_gether_get_host_addr_cdc(struct net_device *net, char *host_addr, int len)
{
	struct eth_dev *dev;

	if (len < 13)
		return -EINVAL;

	dev = netdev_priv(net);
	snprintf(host_addr, len, "%pm", dev->host_mac);

	string_upper(host_addr, host_addr);

	return strlen(host_addr);
}

void mp_gether_get_host_addr_u8(struct net_device *net, u8 host_mac[ETH_ALEN])
{
	struct eth_dev *dev;

	dev = netdev_priv(net);
	memcpy(host_mac, dev->host_mac, ETH_ALEN);
}

void mp_gether_set_qmult(struct net_device *net, unsigned qmult)
{
	struct eth_dev *dev;

	dev = netdev_priv(net);
	dev->qmult = qmult;
}

unsigned mp_gether_get_qmult(struct net_device *net)
{
	struct eth_dev *dev;

	dev = netdev_priv(net);
	return dev->qmult;
}

int mp_gether_get_ifname(struct net_device *net, char *name, int len)
{
	struct eth_dev *dev = netdev_priv(net);
	int ret;

	rtnl_lock();
	ret = scnprintf(name, len, "%s\n",
			dev->ifname_set ? net->name : netdev_name(net));
	rtnl_unlock();
	return ret;
}

int mp_gether_set_ifname(struct net_device *net, const char *name, int len)
{
	struct eth_dev *dev = netdev_priv(net);
	char tmp[IFNAMSIZ];
	const char *p;

	if (name[len - 1] == '\n')
		len--;

	if (len >= sizeof(tmp))
		return -E2BIG;

	strscpy(tmp, name, len + 1);
	if (!dev_valid_name(tmp))
		return -EINVAL;

	/* Require exactly one %d, so binding will not fail with EEXIST. */
	p = strchr(name, '%');
	if (!p || p[1] != 'd' || strchr(p + 2, '%'))
		return -EINVAL;

	strncpy(net->name, tmp, sizeof(net->name));
	dev->ifname_set = true;

	return 0;
}

/*
 * mp_gether_cleanup - remove Ethernet-over-USB device
 * Context: may sleep
 *
 * This is called to free all resources allocated by @gether_setup().
 */
void mp_gether_cleanup(struct eth_dev *dev)
{
	pr_info("<<< %s\n", __func__);

	if (!dev)
		return;

	unregister_netdev(dev->net);
	flush_work(&dev->work);
	free_netdev(dev->net);
}

/**
 * mp_gether_connect - notify network layer that USB link is active
 * @link: the USB link, set up with endpoints, descriptors matching
 *	current device speed, and any framing wrapper(s) set up.
 * Context: irqs blocked
 *
 * This is called to activate endpoints and let the network layer know
 * the connection is active ("carrier detect").  It may cause the I/O
 * queues to open and start letting network packets flow, but will in
 * any case activate the endpoints so that they respond properly to the
 * USB host.
 *
 * Verify net_device pointer returned using IS_ERR().  If it doesn't
 * indicate some error code (negative errno), ep->driver_data values
 * have been overwritten.
 */
struct net_device *mp_gether_connect(struct gether *link)
{
	struct eth_dev		*dev = link->ioport;
	int			result = 0;

	pr_info("<<< %s\n", __func__);

	if (!dev)
		return ERR_PTR(-EINVAL);

	link->in_ep->driver_data = dev;
	result = usb_ep_enable(link->in_ep);
	if (result != 0) {
		DBG(dev, "enable %s --> %d\n",
			link->in_ep->name, result);
		goto fail0;
	}

	link->out_ep->driver_data = dev;
	result = usb_ep_enable(link->out_ep);
	if (result != 0) {
		DBG(dev, "enable %s --> %d\n",
			link->out_ep->name, result);
		goto fail1;
	}

	if (result == 0)
		result = mp_alloc_requests(dev, link, qlen(dev->gadget,
					dev->qmult));

	if (result == 0) {
		dev->zlp = link->is_zlp_ok;
		dev->no_skb_reserve = gadget_avoids_skb_reserve(dev->gadget);
		DBG(dev, "qlen %d\n", qlen(dev->gadget, dev->qmult));

		dev->header_len = link->header_len;
		dev->unwrap = link->unwrap;
		dev->wrap = link->wrap;
		g_rndis_mp.ul_max_pkts_per_xfer = g_rndis_mp.link_ul_max_pkts_per_xfer;
		g_rndis_mp.dl_max_pkts_per_xfer = g_rndis_mp.link_dl_max_pkts_per_xfer;

		spin_lock(&dev->lock);
		g_rndis_mp.tx_skb_hold_count = 0;
		g_rndis_mp.no_tx_req_used = 0;
		g_rndis_mp.tx_req_bufsize = 0;
		dev->port_usb = link;
		if (netif_running(dev->net)) {
			if (link->open)
				link->open(link);
		} else {
			if (link->close)
				link->close(link);
		}
		spin_unlock(&dev->lock);

		netif_carrier_on(dev->net);
		if (netif_running(dev->net))
			mp_eth_start(dev, GFP_ATOMIC);

	/* on error, disable any endpoints  */
	} else {
		(void) usb_ep_disable(link->out_ep);
fail1:
		(void) usb_ep_disable(link->in_ep);
	}
fail0:
	/* caller is responsible for cleanup on error */
	if (result < 0)
		return ERR_PTR(result);
	return dev->net;
}

/**
 * mp_gether_disconnect - notify network layer that USB link is inactive
 * @link: the USB link, on which mp_gether_connect() was called
 * Context: irqs blocked
 *
 * This is called to deactivate endpoints and let the network layer know
 * the connection went inactive ("no carrier").
 *
 * On return, the state is as if mp_gether_connect() had never been called.
 * The endpoints are inactive, and accordingly without active USB I/O.
 * Pointers to endpoint descriptors and endpoint private data are nulled.
 */
void mp_gether_disconnect(struct gether *link)
{
	struct eth_dev		*dev = link->ioport;
	struct usb_request	*req;
	struct sk_buff		*skb;

	pr_info("<<< %s\n", __func__);

	WARN_ON(!dev);
	if (!dev)
		return;

	DBG(dev, "%s\n", __func__);

	netif_stop_queue(dev->net);
	netif_carrier_off(dev->net);

	/* disable endpoints, forcing (synchronous) completion
	 * of all pending i/o.  then free the request objects
	 * and forget about the endpoints.
	 */
	usb_ep_disable(link->in_ep);
	spin_lock(&dev->req_lock);
	while (!list_empty(&dev->tx_reqs)) {
		req = list_first_entry(&dev->tx_reqs, struct usb_request, list);
		list_del(&req->list);

		spin_unlock(&dev->req_lock);
		if (g_rndis_mp.multi_pkt_xfer)
			kfree(req->buf);
		usb_ep_free_request(link->in_ep, req);
		spin_lock(&dev->req_lock);
	}
	spin_unlock(&dev->req_lock);
	link->in_ep->desc = NULL;

	usb_ep_disable(link->out_ep);
	spin_lock(&dev->req_lock);
	while (!list_empty(&dev->rx_reqs)) {
		req = list_first_entry(&dev->rx_reqs, struct usb_request, list);
		list_del(&req->list);

		spin_unlock(&dev->req_lock);
		usb_ep_free_request(link->out_ep, req);
		spin_lock(&dev->req_lock);
	}
	spin_unlock(&dev->req_lock);
	spin_lock(&dev->rx_frames.lock);
	while ((skb = __skb_dequeue(&dev->rx_frames)))
		dev_kfree_skb_any(skb);
	spin_unlock(&dev->rx_frames.lock);

	link->out_ep->desc = NULL;

	/* finish forgetting about this USB link episode */
	dev->header_len = 0;
	dev->unwrap = NULL;
	dev->wrap = NULL;

	spin_lock(&dev->lock);
	dev->port_usb = NULL;
	spin_unlock(&dev->lock);

	if (g_rndis_mp.en_timer) {
		hrtimer_cancel(&g_rndis_mp.tx_timer);
		g_rndis_mp.en_timer = 0;
	}
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Brownell");
