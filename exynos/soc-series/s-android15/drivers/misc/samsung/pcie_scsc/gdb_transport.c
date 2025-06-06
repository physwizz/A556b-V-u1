/****************************************************************************
 *
 * Copyright (c) 2014 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

/** Implements */
#include "gdb_transport.h"

/** Uses */
#include <linux/module.h>
#include <linux/slab.h>
#include <pcie_scsc/scsc_logring.h>
#include "mifintrbit.h"
#ifdef CONFIG_WLBT_KUNIT
#include "./kunit/kunit_gdb_transport.c"
#endif

struct clients_node {
	struct list_head            list;
	struct gdb_transport_client *gdb_client;
};

struct gdb_transport_node {
	struct list_head     list;
	struct gdb_transport *gdb_transport;
};

static struct gdb_transport_module {
	struct list_head clients_list;
	struct list_head gdb_transport_list;
} gdb_transport_module = {
	.clients_list = LIST_HEAD_INIT(gdb_transport_module.clients_list),
	.gdb_transport_list = LIST_HEAD_INIT(gdb_transport_module.gdb_transport_list)
};

static void gdb_transport_process_input_stream(struct gdb_transport *gdb_transport, int irq)
{
	u32 num_bytes = 0;
	u32 alloc_bytes = 0;
	char *buf = NULL;
	int r = 0;

	/* 1st length */
	while (mif_stream_read(&gdb_transport->mif_istream, &num_bytes, sizeof(uint32_t))) {
		SCSC_TAG_DEBUG(GDB_TRANS, "Transferring %d byte payload to handler.\n", num_bytes);
		if (num_bytes > 0 && num_bytes
			< (GDB_TRANSPORT_BUF_LENGTH - sizeof(uint32_t))) {
			alloc_bytes = sizeof(char) * num_bytes;
			/* This is called in atomic context so must use kmalloc with GFP_ATOMIC flag */
			buf = kmalloc(alloc_bytes, GFP_ATOMIC);
			/* 2nd payload (msg) */
			mif_stream_read(&gdb_transport->mif_istream, buf, num_bytes);
			r = gdb_transport->channel_handler_fn(buf, num_bytes, gdb_transport->channel_handler_data);
			if (r == -EINVAL) {
				/* The handler function rejects further input on this channel,
				 * e.g. because it is getting spurious requests.
				 */
				SCSC_TAG_ERR(GDB_TRANS, "channel_handler_fn rejects source (irq %d)", irq);

				/* This case occurs when gdb irq occurs before gdb channel is opened.
				 * It is confirmed that unidentified data is continuously updated
				 * for a while in cpacketbuffer of gdb transport when gdb irq occurs.
				 * In other words, host driver must remain in this loop
				 * until the data update is completed.
				 * Therefore, It is necessary to escape from the loop of the handler
				 * function in order to avoid kernel panic due to core occupancy.
				 */
				kfree(buf);
				break;
			}
			kfree(buf);
		} else {
			SCSC_TAG_ERR(GDB_TRANS, "Incorrect num_bytes: 0x%08x\n", num_bytes);
			mif_stream_log(&gdb_transport->mif_istream, SCSC_ERR);
		}
	}
}

#if defined(CONFIG_SCSC_BB_REDWOOD)
/* Handle incoming packets and pass to handler - poll all gdb channels to find which one was responding and process it */
static void gdb_input_irq_handler_poll(int irq, void *data)
{
	struct scsc_mif_abs  *mif_abs;
	struct gdb_transport *gdb_transport = (struct gdb_transport *)data;
	const void *peek_message;
	struct gdb_transport_node *gdb_transport_node;

	/* Clear the interrupt first to ensure we can't possibly miss one */
	mif_abs = scsc_mx_get_mif_abs(gdb_transport->mx);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mif_abs->irq_bit_clear(mif_abs, irq, gdb_transport->target);
#else
	mif_abs->irq_bit_clear(mif_abs, irq);
#endif

	/* peek for a message from all/any wlan/fxm gdb channels */
	list_for_each_entry(gdb_transport_node, &gdb_transport_module.gdb_transport_list, list) {
		gdb_transport = gdb_transport_node->gdb_transport;
		if ((gdb_transport->type != GDB_TRANSPORT_WPAN) && (gdb_transport->type != GDB_TRANSPORT_PMU)) {
			if ((peek_message = mif_stream_peek(&gdb_transport->mif_istream, NULL)) != NULL) {
				gdb_transport_process_input_stream(gdb_transport, irq);
				/* continue to loop so we'll find any other channel containing data and process it */
			}
		}
	}
}
#endif

/** Handle incoming packets and pass to handler */
static void gdb_input_irq_handler(int irq, void *data)
{
	struct scsc_mif_abs  *mif_abs;
	struct gdb_transport *gdb_transport = (struct gdb_transport *)data;

	SCSC_TAG_DEBUG(GDB_TRANS, "Handling write signal from gdb_transport_type %d irq %d\n", gdb_transport->type, irq);

	/* Clear the interrupt first to ensure we can't possibly miss one */
	mif_abs = scsc_mx_get_mif_abs(gdb_transport->mx);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mif_abs->irq_bit_clear(mif_abs, irq, gdb_transport->target);
#else
	mif_abs->irq_bit_clear(mif_abs, irq);
#endif

	gdb_transport_process_input_stream(gdb_transport, irq);
}

/** MIF Interrupt handler for acknowledging reads made by the AP */
static void gdb_output_irq_handler(int irq, void *data)
{
	struct scsc_mif_abs  *mif_abs;
	struct gdb_transport *gdb_transport = (struct gdb_transport *)data;

	SCSC_TAG_DEBUG(GDB_TRANS, "Ignoring read signal.\n");

	/* Clear the interrupt first to ensure we can't possibly miss one */
	/* The FW read some data from the output stream.
	 * Currently we do not care, so just clear the interrupt. */
	mif_abs = scsc_mx_get_mif_abs(gdb_transport->mx);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mif_abs->irq_bit_clear(mif_abs, irq, gdb_transport->target);
#else
	mif_abs->irq_bit_clear(mif_abs, irq);
#endif
}

static void gdb_transport_probe_registered_clients(struct gdb_transport *gdb_transport)
{
	bool                client_registered = false;
	struct clients_node *gdb_client_node, *gdb_client_next;
	struct scsc_mif_abs *mif_abs;
	char                *dev_uid;

	/* Traverse Linked List for each mif_driver node */
	list_for_each_entry_safe(gdb_client_node, gdb_client_next, &gdb_transport_module.clients_list, list) {
		/* Get UID */
		mif_abs = scsc_mx_get_mif_abs(gdb_transport->mx);
		dev_uid = mif_abs->get_uid(mif_abs);
		gdb_client_node->gdb_client->probe(gdb_client_node->gdb_client, gdb_transport, dev_uid);
		client_registered = true;
	}
	if (client_registered == false)
		SCSC_TAG_INFO(GDB_TRANS, "No clients registered\n");
}

void gdb_transport_release(struct gdb_transport *gdb_transport)
{
	struct clients_node       *gdb_client_node, *gdb_client_next;
	struct gdb_transport_node *gdb_transport_node, *gdb_transport_node_next;
	bool                      match = false;

	list_for_each_entry_safe(gdb_transport_node, gdb_transport_node_next, &gdb_transport_module.gdb_transport_list, list) {
		if (gdb_transport_node->gdb_transport == gdb_transport) {
			match = true;
			SCSC_TAG_INFO(GDB_TRANS, "release client\n");
			/* Wait for client to close */
			mutex_lock(&gdb_transport->channel_open_mutex);
			/* Need to notify clients using the transport has been released */
			list_for_each_entry_safe(gdb_client_node, gdb_client_next, &gdb_transport_module.clients_list, list) {
				gdb_client_node->gdb_client->remove(gdb_client_node->gdb_client, gdb_transport);
			}
			list_del(&gdb_transport_node->list);
			kfree(gdb_transport_node);
			mutex_unlock(&gdb_transport->channel_open_mutex);
		}
	}
	if (match == false)
		SCSC_TAG_INFO(GDB_TRANS, "No match for given scsc_mif_abs\n");

	mif_stream_release(&gdb_transport->mif_istream);
	mif_stream_release(&gdb_transport->mif_ostream);
}

void gdb_transport_config_serialise(struct gdb_transport *gdb_transport,
				    struct mxtransconf   *trans_conf)
{
	mif_stream_config_serialise(&gdb_transport->mif_istream, &trans_conf->to_ap_stream_conf);
	mif_stream_config_serialise(&gdb_transport->mif_ostream, &trans_conf->from_ap_stream_conf);
}


/** Public functions */
int gdb_transport_init(struct gdb_transport *gdb_transport, struct scsc_mx *mx, enum gdb_transport_enum type)
{
	int                       r = 0;
	uint32_t                  mem_length = GDB_TRANSPORT_BUF_LENGTH;
	uint32_t                  packet_size = 4;
	uint32_t                  num_packets;
	struct gdb_transport_node *gdb_transport_node;

	gdb_transport_node = kzalloc(sizeof(*gdb_transport_node), GFP_KERNEL);
	if (!gdb_transport_node)
		return -EIO;

	memset(gdb_transport, 0, sizeof(struct gdb_transport));
	num_packets = mem_length / packet_size;
	mutex_init(&gdb_transport->channel_handler_mutex);
	mutex_init(&gdb_transport->channel_open_mutex);
	gdb_transport->mx = mx;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (type == GDB_TRANSPORT_WPAN)
		gdb_transport->target = SCSC_MIF_ABS_TARGET_WPAN;
	else
		gdb_transport->target = SCSC_MIF_ABS_TARGET_WLAN;
#endif

#if defined(CONFIG_SCSC_BB_REDWOOD)
	if (type == GDB_TRANSPORT_FXM_1)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_FXM_1, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_FXM_1_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_FXM_2)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_FXM_2, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_FXM_2_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WPAN)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WPAN, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_WPAN_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_PMU)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_PMU, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_PMU_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_FXM_3)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_FXM_3, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_FXM_3_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_2)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_2, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_WLAN_2_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_3)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_3, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_WLAN_3_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_4)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_4, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_WLAN_4_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_5)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_5, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_WLAN_5_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_6)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_6, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_WLAN_6_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_7)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_7, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_WLAN_7_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_8)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_8, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_WLAN_8_INPUT_TYPE);
	else
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_PREALLOC, gdb_input_irq_handler_poll, gdb_transport, GDB_TRANSPORT_WLAN_INPUT_TYPE);
#else /* CONFIG_SCSC_BB_REDWOOD */
	if (type == GDB_TRANSPORT_FXM_1)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_FXM_1, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_FXM_1_INPUT_TYPE);
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	else if (type == GDB_TRANSPORT_FXM_2)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_FXM_2, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_FXM_2_INPUT_TYPE);
#endif
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	else if (type == GDB_TRANSPORT_WPAN)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WPAN, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_WPAN_INPUT_TYPE);
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
	else if (type == GDB_TRANSPORT_PMU)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_PMU, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_PMU_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_FXM_3)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_FXM_3, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_FXM_3_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_2)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_2, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_2_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_3)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_3, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_3_INPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_4)
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN_4, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_4_INPUT_TYPE);
#endif
	else
		r = mif_stream_init(&gdb_transport->mif_istream, SCSC_MIF_ABS_TARGET_WLAN, MIF_STREAM_DIRECTION_IN, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_ALLOC, gdb_input_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_INPUT_TYPE);
#endif /* CONFIG_SCSC_BB_REDWOOD */
	if (r) {
		kfree(gdb_transport_node);
		return r;
	}

	if (type == GDB_TRANSPORT_FXM_1)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_FXM_1, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_FXM_1_OUTPUT_TYPE);
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	else if (type == GDB_TRANSPORT_FXM_2)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_FXM_2, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_FXM_2_OUTPUT_TYPE);
#endif
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	else if (type == GDB_TRANSPORT_WPAN)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WPAN, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WPAN_OUTPUT_TYPE);
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
	else if (type == GDB_TRANSPORT_PMU)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_PMU, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_PMU_OUTPUT_TYPE);
	else if (type == GDB_TRANSPORT_FXM_3)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_FXM_3, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_FXM_3_OUTPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_2)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WLAN_2, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_2_OUTPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_3)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WLAN_3, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_3_OUTPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_4)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WLAN_4, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_4_OUTPUT_TYPE);
#endif
#if defined(CONFIG_SCSC_BB_REDWOOD)
	else if (type == GDB_TRANSPORT_WLAN_5)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WLAN_5, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_5_OUTPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_6)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WLAN_6, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_6_OUTPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_7)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WLAN_7, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_7_OUTPUT_TYPE);
	else if (type == GDB_TRANSPORT_WLAN_8)
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WLAN_8, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_8_OUTPUT_TYPE);
#endif
	else
		r = mif_stream_init(&gdb_transport->mif_ostream, SCSC_MIF_ABS_TARGET_WLAN, MIF_STREAM_DIRECTION_OUT, num_packets, packet_size, mx, MIF_STREAM_INTRBIT_TYPE_RESERVED, gdb_output_irq_handler, gdb_transport, GDB_TRANSPORT_WLAN_OUTPUT_TYPE);
	if (r) {
		mif_stream_release(&gdb_transport->mif_istream);
		kfree(gdb_transport_node);
		return r;
	}

	gdb_transport->channel_handler_fn = NULL;
	gdb_transport->channel_handler_data = NULL;

	gdb_transport_node->gdb_transport = gdb_transport;
	/* Add gdb_transport node */
	list_add_tail(&gdb_transport_node->list, &gdb_transport_module.gdb_transport_list);
	gdb_transport->type = type;
	gdb_transport_probe_registered_clients(gdb_transport);
	return 0;
}

void gdb_transport_send(struct gdb_transport *gdb_transport, void *message, uint32_t message_length)
{
	char msg[300];

	if (message_length > sizeof(msg))
		return;

	memcpy(msg, message, message_length);

	mutex_lock(&gdb_transport->channel_handler_mutex);
	/* 1st length */
	mif_stream_write(&gdb_transport->mif_ostream, &message_length, sizeof(uint32_t));
	/* 2nd payload (msg) */
	mif_stream_write(&gdb_transport->mif_ostream, message, message_length);
	mutex_unlock(&gdb_transport->channel_handler_mutex);
}
EXPORT_SYMBOL(gdb_transport_send);

void gdb_transport_register_channel_handler(struct gdb_transport *gdb_transport,
					    gdb_channel_handler handler, void *data)
{
	mutex_lock(&gdb_transport->channel_handler_mutex);
	gdb_transport->channel_handler_fn = handler;
	gdb_transport->channel_handler_data = (void *)data;
	mutex_unlock(&gdb_transport->channel_handler_mutex);
}
EXPORT_SYMBOL(gdb_transport_register_channel_handler);

int gdb_transport_register_client(struct gdb_transport_client *gdb_client)
{
	struct clients_node       *gdb_client_node;
	struct gdb_transport_node *gdb_transport_node;
	struct scsc_mif_abs       *mif_abs;
	char                      *dev_uid;

	/* Add node in modules linked list */
	gdb_client_node = kzalloc(sizeof(*gdb_client_node), GFP_KERNEL);
	if (!gdb_client_node)
		return -ENOMEM;

	gdb_client_node->gdb_client = gdb_client;
	list_add_tail(&gdb_client_node->list, &gdb_transport_module.clients_list);


	/* Traverse Linked List for transport registered */
	list_for_each_entry(gdb_transport_node, &gdb_transport_module.gdb_transport_list, list) {
		/* Get UID */
		mif_abs = scsc_mx_get_mif_abs(gdb_transport_node->gdb_transport->mx);
		dev_uid = mif_abs->get_uid(mif_abs);
		gdb_client->probe(gdb_client, gdb_transport_node->gdb_transport, dev_uid);
	}
	return 0;
}
EXPORT_SYMBOL(gdb_transport_register_client);

void gdb_transport_unregister_client(struct gdb_transport_client *gdb_client)
{
	struct clients_node *gdb_client_node, *gdb_client_next;

	/* Traverse Linked List for each client_list  */
	list_for_each_entry_safe(gdb_client_node, gdb_client_next, &gdb_transport_module.clients_list, list) {
		if (gdb_client_node->gdb_client == gdb_client) {
			list_del(&gdb_client_node->list);
			kfree(gdb_client_node);
		}
	}
}
EXPORT_SYMBOL(gdb_transport_unregister_client);
