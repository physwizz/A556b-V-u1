/****************************************************************************
 *
 * Copyright (c) 2014 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

/**
 * Maxwell gdb transport (Interface)
 *
 * Provides bi-directional communication between the firmware and the
 * host.
 *
 * This interface also provides a utility method for sending messages across
 * the stream.
 */

#ifndef GDB_TRANSPORT_H__
#define GDB_TRANSPORT_H__

/** Uses */
#include <linux/kthread.h>
#include "mifstream.h"

/*
 * Initialising a buffer of 1 byte is never legitimate, do not allow it.
 * The memory buffer length must be a multiple of the packet size.
 */
#define GDB_TRANSPORT_BUF_LENGTH (2 * 1024)

struct gdb_transport;
enum gdb_transport_enum {
	GDB_TRANSPORT_WLAN = 0,
	GDB_TRANSPORT_FXM_1 = 1,
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	GDB_TRANSPORT_FXM_2 = 2,
#endif
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	GDB_TRANSPORT_WPAN = 3,
#endif
#if defined(CONFIG_SCSC_PCIE_CHIP)
	GDB_TRANSPORT_PMU = 4,
	GDB_TRANSPORT_FXM_3 = 5,
	GDB_TRANSPORT_WLAN_2 = 6,
	GDB_TRANSPORT_WLAN_3 = 7,
	GDB_TRANSPORT_WLAN_4 = 8,
#endif
#if IS_ENABLED(CONFIG_SCSC_BB_REDWOOD)
	GDB_TRANSPORT_WLAN_5 = 9,
	GDB_TRANSPORT_WLAN_6 = 10,
	GDB_TRANSPORT_WLAN_7 = 11,
	GDB_TRANSPORT_WLAN_8 = 12,
#endif // CONFIG_SCSC_BB_REDWOOD
};
/**
 * Transport channel callback handler. This will be invoked each time a message on a channel is
 * received. Handlers may perform work within
 * their callback implementation, but should not block.
 *
 * Note that the message pointer passed is only valid for the duration of the function call.
 *
 * The function returns 0 on success, or -EINVAL to stop the source on error
 */
typedef int (*gdb_channel_handler)(const void *message, size_t length, void *data);

/**
 * Sends a message to the AP across the given channel.
 *
 * This function is safe to call from any RTOS thread.
 */
void gdb_transport_send(struct gdb_transport *gdb_transport,
			void *message, uint32_t message_length);

/**
 * Initialises the maxwell management transport and configures the necessary
 * interrupt handlers. Called once during boot.
 */
int gdb_transport_init(struct gdb_transport *gdb_transport, struct scsc_mx *mx, enum gdb_transport_enum type);
void gdb_transport_release(struct gdb_transport *gdb_transport);

/*
 *  Initialises the configuration area incl. Maxwell Infrastructure Configuration,
 * MIF Management Transport Configuration and  MIF Management Stream Configuration.
 */
void gdb_transport_config_serialise(struct gdb_transport *gdb_transport, struct mxtransconf *trans_conf);
void gdb_transport_set_error(struct gdb_transport *gdb_transport);

struct gdb_transport {
	struct scsc_mx          *mx;
	struct mif_stream       mif_istream;
	struct mif_stream       mif_ostream;
	/** Registered channel handlers for messages coming from the AP for each channel */
	gdb_channel_handler     channel_handler_fn;
	void                    *channel_handler_data;
	struct mutex            channel_handler_mutex;
	struct mutex            channel_open_mutex;
	/* Transport processor type  */
	enum gdb_transport_enum type;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	enum scsc_mif_abs_target target;
#endif
};

struct gdb_transport_client {
	char *name;
	void (*probe)(struct gdb_transport_client *gdb_client, struct gdb_transport *gdb_transport, char *dev_uid);
	void (*remove)(struct gdb_transport_client *gdb_client, struct gdb_transport *gdb_transport);
};

int gdb_transport_register_client(struct gdb_transport_client *gdb_client);
void gdb_transport_unregister_client(struct gdb_transport_client *gdb_client);
void gdb_transport_register_channel_handler(struct gdb_transport *gdb_transport, gdb_channel_handler handler, void *data);
void gdb_transport_register_char_device(struct scsc_mx *mx, struct gdb_transport **gdb_transport_handler);
#endif /* GDB_TRANSPORT_H__ */
