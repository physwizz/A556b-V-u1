/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2014 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __GNSS_IF_H__
#define __GNSS_IF_H__

#include <linux/platform_device.h>
#include <linux/miscdevice.h>

#define MAX_NAME_LEN            64

/**
 * struct gnss_io_t - declaration for io_device
 * @name:	device name
 * @id:		for SIPC4, contains format & channel information
 *		(id & 11100000b)>>5 = format  (eg, 0=FMT, 1=RAW, 2=RFS)
 *		(id & 00011111b)    = channel (valid only if format is RAW)
 *		for SIPC5, contains only 8-bit channel ID
 * @format:	device format
 * @io_type:	type of this io_device
 * @links:	list of link_devices to use this io_device
 *		for example, if you want to use DPRAM and USB in an io_device.
 *		.links = LINKTYPE(LINKDEV_DPRAM) | LINKTYPE(LINKDEV_USB)
 * @tx_link:	when you use 2+ link_devices, set the link for TX.
 *		If define multiple link_devices in @links,
 *		you can receive data from them. But, cannot send data to all.
 *		TX is only one link_device.
 * @app:	the name of the application that will use this IO device
 *
 */
struct gnss_io_t {
	char *name;
	int   id;
	char *app;
};

enum gnss_bcmd_ctrl {
	CTRL0,
	CTRL1,
	CTRL2,
	CTRL3,
	BCMD_CTRL_COUNT,
};

enum gnss_reg_type {
	GNSS_REG_RX_IPC_MSG,
	GNSS_REG_TX_IPC_MSG,
	GNSS_REG_RX_HEAD,
	GNSS_REG_RX_TAIL,
	GNSS_REG_TX_HEAD,
	GNSS_REG_TX_TAIL,
	GNSS_REG_COUNT,
};

enum gnss_ipc_vector {
	GNSS_IPC_MBOX,
	GNSS_IPC_SHMEM,
	GNSS_IPC_COUNT,
};

#define		GNSS_BARAC_GENERAL		0x0
#define		GNSS_BARAC_BA			0x1000
#define		GNSS_BARAC_BA_CTRL		GNSS_BARAC_BA
#define		GNSS_BARAC_WINDOW_OFFSET	GNSS_BARAC_BA + 0x800
#define		GNSS_BARAC_WINDOW_IMPLEMENT	0x0
#define		GNSS_BARAC_WINDOW_CTRL		0x4
#define		GNSS_BARAC_WINDOW_BASE_ADDR	0x8	/* size 28bits */
#define		GNSS_BARAC_WINDOW_END_ADDR	0xC	/* size 28bits */
#define		GNSS_BARAC_WINDOW_REMAP_ADDR	0x10	/* size 28bits */

struct gnss_remap {
	u32 offset;
	u32 gnss_start;
	u32 gnss_end;
	u32 ap_start;
	u32 authority;
};

struct gnss_mbox {
	/* ap2gnss */
	int int_bcmd;
	int int_req_fault_info;
	int int_ipc_msg;
	int int_ack_wake_set;

	/* gnss2ap */
	int irq_bcmd;
	int irq_rsp_fault_info;
	int irq_ipc_msg;
	int irq_req_wake_clr;
	int irq_simple_lock;

	unsigned int reg_bcmd_ctrl[BCMD_CTRL_COUNT];

	int id;
};

struct gnss_shared_reg_value {
	int index;
	u32 __iomem *addr;
};

struct gnss_shared_reg {
	const char *name;
	struct gnss_shared_reg_value value;
	u32 device;
};

struct gnss_fault_data_area_value {
	u32 index;
	u8 __iomem *addr;
};

struct gnss_fault_data_area {
	const char *name;
	struct gnss_fault_data_area_value value;
	u32 size;
	u32 device;
};

/* Platform data */
struct gnss_pdata {
	char *name;
	char *device_node_name;

	int irq_gnss_active;
	int irq_gnss_wdt;
	int irq_gnss_wakeup;


	struct gnss_remap *wnd_set;
	u32 num_wnd;
	void __iomem *remap_reg;

	struct gnss_mbox *mbx;

	struct gnss_shared_reg *reg[GNSS_REG_COUNT];

	struct gnss_fault_data_area fault_info;

	struct gnss_io_t *iodev;
	struct shmem_link_device *shmd;

	u64 shmem_base;
	u32 shmem_size;
	u32 code_offset;
	u32 code_allowed_size;
	u32 ipc_offset;
	u32 ipc_size;
	u32 ipc_rx_offset;
	u32 ipc_rx_size;
	u32 ipc_tx_offset;
	u32 ipc_tx_size;
	u32 ipc_reg_offset;
	u32 ipc_reg_size;

	u32 pmu_gnss_ctrl_ns;
	u32 pmu_gnss_ctrl_s;
	u32 pmu_gnss_stat;
	u32 pmu_gnss_debug;

	u32 remap_hw;
};

struct gnss_irq {
	spinlock_t lock;
	unsigned int num;
	char name[MAX_NAME_LEN];
	unsigned long flags;
	bool active;
	bool registered;
};

#define gif_dt_read_enum(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
		dest = (__typeof__(dest))(val); \
	} while (0)

#define gif_dt_read_bool(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
		dest = val ? true : false; \
	} while (0)

#define gif_dt_read_string(np, prop, dest) \
	do { \
		if (of_property_read_string(np, prop, \
				(const char **)&dest)) \
		return -EINVAL; \
	} while (0)

#define gif_dt_read_u32(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) \
			return -EINVAL; \
		dest = val; \
	} while (0)
#define gif_dt_read_u32_noerr(np, prop, dest) \
	do { \
		u32 val; \
		if (!of_property_read_u32(np, prop, &val)) \
			dest = val; \
	} while (0)
#define gif_dt_read_u32_array(np, prop, dest, sz) \
	do { \
		if (of_property_read_u32_array(np, prop, dest, (sz))) \
			return -EINVAL; \
	} while (0)

#define LOG_TAG	"gif: "
#define CALLEE	(__func__)
#define CALLER	(__builtin_return_address(0))

#define gif_err_limited(fmt, ...) \
	printk_ratelimited(KERN_ERR LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define gif_err(fmt, ...) \
	pr_err(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define gif_debug(fmt, ...) \
	pr_debug(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define gif_info(fmt, ...) \
	pr_info(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define gif_trace(fmt, ...) \
	printk(KERN_DEBUG "gif: %s: %d: called(%pF): " fmt, \
		__func__, __LINE__, __builtin_return_address(0), ##__VA_ARGS__)

#endif
