/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PERI_MAILBOX_H_
#define PERI_MAILBOX_H_

#include <linux/version.h>
#include "../../include/npu-config.h"

#define MAILBOX_SIGNATURE1		0x0C0FFEE0
#define MAILBOX_SIGNATURE2		0xC0DEC0DE
#define MAILBOX_SHARED_MAX		64

#define MAILBOX_H2FCTRL_LPRIORITY	0
#define MAILBOX_H2FCTRL_MPRIORITY	1
#define MAILBOX_H2FCTRL_HPRIORITY	2
#define MAILBOX_H2FCTRL_FWRESPONSE	3
#define MAILBOX_H2FCTRL_MAX		4

#define MAILBOX_F2HCTRL_RESPONSE	0
#define MAILBOX_F2HCTRL_NRESPONSE	1
#define MAILBOX_F2HCTRL_FWMSG		2
#define MAILBOX_F2HCTRL_REPORT		3
#define MAILBOX_F2HCTRL_MAX		4

#if IS_ENABLED(CONFIG_NPU_USE_MAILBOX_GROUP)
#define MAILBOX_GROUP_LPRIORITY         0 /* for nw request */
#define MAILBOX_GROUP_MPRIORITY         1 /* for frame request */
#define MAILBOX_GROUP_HPRIORITY         2 /* for frame high request */
#define MAILBOX_GROUP_FW_RESP           3 /* FW Resp */
#define MAILBOX_GROUP_HRESPONSE         3 /* High response */
#define MAILBOX_GROUP_NRESPONSE         4 /* Normal response */
#define MAILBOX_GROUP_FW_MSG            5 /* FW_MSG */
#define MAILBOX_GROUP_REPORT            6 /* FW-report */
#define MAILBOX_GROUP_MAX               8
#else
#define MAILBOX_SWI_F2H                 0
#define MAILBOX_SWI_H2F                 1
#endif

#define MAX_MAILBOX                     9

enum npu_mbox_index_e {
	NPU_MBOX_REQUEST_LOW = 0,
	NPU_MBOX_REQUEST_MEDIUM,
	NPU_MBOX_REQUEST_HIGH,
#if IS_ENABLED(CONFIG_NPU_USE_IMB_ALLOCATOR_V2)
	NPU_MBOX_FWRESPONSE,
#endif
	NPU_MBOX_RESULT,//and RESPONSE
	NPU_MBOX_NRESULT,
#if IS_ENABLED(CONFIG_NPU_USE_IMB_ALLOCATOR_V2)
	NPU_MBOX_FWMSG,
#endif
	NPU_MBOX_LOG,
#ifdef CONFIG_EXYNOS_UNPU_DRIVER
	NPU_MBOX_REQUEST_UNPU,
#else
	NPU_MBOX_RESERVED,
#endif
	NPU_MBOX_MAX_ID
};

/*
 *  +-----------------------------+----------- <--- the mailbox base address
 *  |      struct mailbox_hdr     |    |     |
 *  +-----------------------------+    4K    |
 *  |      Padding                |    |     |
 *  +-----------------------------+-----     |
 *  |      Mailbox #1             |         16MB in maximum (= MAILBOX MEMORY)
 *  | (high priority command)     |          |
 *  +-----------------------------+          |
 *  |      Mailbox #2             |          |
 *  | (medium priority command)   |          |
 *  +-----------------------------+          |
 *  |      Mailbox #3             |          |
 *  | (low priority command)      |          |
 *  +-----------------------------+          |
 *  |      Mailbox #4             |          |
 *  | (high priority response)    |          |
 *  +-----------------------------+          |
 *  |      Mailbox #5             |          |
 *  | (normal priority response)  |          |
 *  +-----------------------------+          |
 *  |      Mailbox #6             |          |
 *  |         (log)               |          |
 *  +-----------------------------+-----------
 */

/* Mailbox HW */
/* group0 : low priority
 * group1 : reserved for score
 * group2 : high priority
 * group3 : response
 * group4 : log
 */
struct intr_group {
#ifdef CONFIG_NPU_USE_MAILBOX_GROUP
	u32 g; /* Interrupt Generation Register */
	u32 c; /* Interrupt Clear Register */
	u32 m; /* Interrupt Mask register */
	u32 s; /* Interrupt Status Register */
	u32 ms; /* Interrupt Mask Status Register */
#else
	u32 st;	/* status */
	u32 ms;	/* mask status */
	u32 e;	/* enable */
	u32 s;	/* set */
	u32 c;	/* clear */
#endif
};

struct mailbox_sfr {
#ifdef CONFIG_NPU_USE_MAILBOX_GROUP
	u32			mcuctl;
	u32			pad0;
	struct intr_group	grp[MAILBOX_GROUP_MAX];
	u32			pad1;
	u32			version;
	u32			pad2[3];
	u32			shared[MAILBOX_SHARED_MAX];
#else
	/* 0th group sends irq; 1st group receives irq
	*  each group has 15 lines to send interrupt
	*/
	struct intr_group       grp[2];
#endif
};

struct mailbox_ctrl {
	u32			sgmt_ofs; /* minus offset from SRAM END(0x80000) */
	u32			sgmt_len; /* plus length in byte from sgmt_ofs */
	u32			wptr;
	u32			rptr;
};

#ifdef CONFIG_NPU_USE_UTIL_STATS
struct fw_stats {
	/* utilization in us per second */
	u32 cpu_utilization;
	u32 dsp_utilization;
	u32 npu_utilization[CONFIG_NPU_NUM_CORES];
};
#endif

struct mailbox_hdr {
	u32			signature1; /* firmware should update this after firmware initialization is done */
	u32			signature2; /* host should update this after host configuration is done */
	u32			version;	/* half low 16 bits is mailbox ipc version, half high 16 bits is command version */
	u32			hw_info;
	u32			totsize;
	u32			boottype;
#ifdef CONFIG_NPU_USE_UTIL_STATS
	struct fw_stats		stats;
	u32			reserved[17 - CONFIG_NPU_NUM_CORES];
#else
	u32			reserved[18];
#endif
	struct mailbox_ctrl	h2fctrl[MAILBOX_H2FCTRL_MAX];
	struct mailbox_ctrl	f2hctrl[MAILBOX_F2HCTRL_MAX];

	u32			log_dram;
	u32			log_level; /* this means the time when read trigger is generated on report mailbox */
	u32			debug_time; /* start time in micro second at boot */
	u32			debug_code; /* firmware boot progress */
	u32			max_slot; /* the maximum number of ncp object slot */
	u32			llc_mode; /* |LLC_FLC_8|LLC_SDMA_8|LLC_CA32_8|NPU_mode_8| */
};

struct MAILBOX {
	volatile struct mailbox_sfr *sfr;
	struct uk_event *event[MAILBOX_H2FCTRL_MAX];
};

//void mailbox_init(u32 addr);
void mailbox_send_interrupt(u32 idx);

/* Field definition for mailbox_hdr.hw_info */
union npu_hw_info {
        struct {
                u32 dcache_en   : 1;    /* 1: D$ enable / 0: D$ disable */
                u32 reserved    : 15;
                u32 product_id  : 16;   /* Hex coded SoC name e.g.0x9630 */
        } fields;
        u32 value;
};

static u32 NPU_MAILBOX_SECTION_CONFIG[MAX_MAILBOX] = {
	SZ_4K,	/* Size of mailbox hdr*/
	SZ_128K,	/* Size of 1st mailbox */
	SZ_128K,	/* Size of 2nd mailbox */
	SZ_128K,	/* Size of 3rd mailbox */
	SZ_128K,	/* Size of 4th mailbox */
	SZ_256K,	/* Size of 5th mailbox */
	SZ_256K,	/* Size of 6th mailbox */
	SZ_256K,	/* Size of 7th mailbox */
	SZ_256K,	/* Size of 8th mailbox */
};

#define NPU_MBOX_BASE(x)		(x)
#define NPU_MAILBOX_GET_CTRL(x, y)	((x)+(y))

#endif	/* PERI_MAILBOX_H_ */
