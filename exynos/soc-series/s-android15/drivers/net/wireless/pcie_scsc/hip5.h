/****************************************************************************
 *
 * Copyright (c) 2014 - 2024 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __HIP5_H__
#define __HIP5_H__

/**
 * This header file is the public HIP5 interface, which will be accessible by
 * Wi-Fi service driver components.
 *
 * All struct and internal HIP functions shall be moved to a private header
 * file.
 */

#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/skbuff.h>
#include <pcie_scsc/scsc_mifram.h>
#include <pcie_scsc/scsc_mx.h>
#ifdef CONFIG_SCSC_WLAN_ANDROID
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <pcie_scsc/scsc_wakelock.h>
#else
#include <linux/wakelock.h>
#endif
#endif
#include "mbulk.h"

/* Shared memory Layout
 *
 * |-------------------------| CONFIG
 * |    CONFIG  + Queues     |
 * |       ---------         |
 * |          MIB            |
 * |-------------------------| TX Pool
 * |         TX DAT          |
 * |       ---------         |
 * |         TX CTL          |
 * |-------------------------| RX Pool
 * |          RX             |
 * |-------------------------|
 */

/**** OFFSET SHOULD BE 4096 BYTES ALIGNED ***/
/*** CONFIG POOL ***/
#define HIP5_WLAN_CONFIG_OFFSET	0x00000
#define HIP5_WLAN_CONFIG_SIZE	0xBF000 /* 764 KB */
/*** MIB POOL ***/
#define HIP5_WLAN_MIB_OFFSET	(HIP5_WLAN_CONFIG_OFFSET +  HIP5_WLAN_CONFIG_SIZE)
#define HIP5_WLAN_MIB_SIZE	0x08000 /* 32 KB */

/*** TX POOL ***/
#define HIP5_WLAN_TX_OFFSET	(HIP5_WLAN_MIB_OFFSET + HIP5_WLAN_MIB_SIZE)
/*** TX POOL - CTRL POOL ***/
#define HIP5_WLAN_TX_CTRL_OFFSET	HIP5_WLAN_TX_OFFSET
#define HIP5_WLAN_TX_CTRL_SIZE	    0x10000 /*  64 KB */
/*** TX POOL - DATA POOL ***/
#define HIP5_WLAN_TX_DATA_OFFSET	(HIP5_WLAN_TX_OFFSET + HIP5_WLAN_TX_CTRL_SIZE)

#ifdef CONFIG_SCSC_WLAN_HOST_DPD
#define HIP5_WLAN_TX_DATA_SIZE	    		0x200000 /* 2 MB */
#if defined(CONFIG_WLBT_RX_CACHEABLE)
#define HIP5_WLAN_ZERO_COPY_TX_DATA_SIZE	0xE4000 /* 912 KB */
#else
#define HIP5_WLAN_ZERO_COPY_TX_DATA_SIZE	0x40000 /* 256 KB */
#endif
#define HIP5_WLAN_TX_SIZE	        		(HIP5_WLAN_TX_DATA_SIZE + HIP5_WLAN_TX_CTRL_SIZE)
#define HIP5_WLAN_ZERO_COPY_TX_SIZE			(HIP5_WLAN_ZERO_COPY_TX_DATA_SIZE + HIP5_WLAN_TX_CTRL_SIZE)

/*** RX POOL ***/
#define HIP5_WLAN_RX_OFFSET					(HIP5_WLAN_TX_DATA_OFFSET +  HIP5_WLAN_TX_DATA_SIZE)
#define HIP5_WLAN_RX_SIZE					0x20000 /* 128 KB */
#define HIP5_WLAN_ZERO_COPY_RX_OFFSET		(HIP5_WLAN_TX_DATA_OFFSET +  HIP5_WLAN_ZERO_COPY_TX_DATA_SIZE)
#if defined(CONFIG_WLBT_RX_CACHEABLE)
#define HIP5_WLAN_ZERO_COPY_RX_SIZE			0 /* RX buffers allocated in a separate cacheable area */
#else
#define HIP5_WLAN_ZERO_COPY_RX_SIZE			0x80000 /* 512 KB */
#endif
#define HIP5_WLAN_DPD_BUF_OFFSET			(HIP5_WLAN_RX_OFFSET +  HIP5_WLAN_RX_SIZE)
#define HIP5_WLAN_ZERO_COPY_DPD_BUF_OFFSET	(HIP5_WLAN_ZERO_COPY_RX_OFFSET +  HIP5_WLAN_ZERO_COPY_RX_SIZE)
#define HIP5_WLAN_DPD_BUF_SIZE				0x80000 /* 512 KB */
#define HIP5_WLAN_ZERO_COPY_DPD_BUF_SIZE	0x200000 /* 2 MB */

/*** TOTAL : CONFIG POOL + TX POOL + RX POOL + DPD buffer ***/
/* 764 KB + 32 KB + 2048 KB + 64 KB + 128 KB + 512 KB = 3548KB out of 3840KB */
#define HIP5_WLAN_TOTAL_MEM	(HIP5_WLAN_CONFIG_SIZE + HIP5_WLAN_MIB_SIZE + \
	HIP5_WLAN_TX_SIZE + HIP5_WLAN_RX_SIZE + HIP5_WLAN_DPD_BUF_SIZE)

/* #if defined(CONFIG_WLBT_RX_CACHEABLE)
 * 764 KB + 32 KB + 2048 KB + 64 KB + 912 KB + 0 KB = 3820KB out of 3840KB
 * #else
 * 764 KB + 32 KB + 256 KB + 64 KB + 512KB + 2048 KB =  3676KB out of 3840KB
 * #endif
 */
#define HIP5_WLAN_ZERO_COPY_TOTAL_MEM	(HIP5_WLAN_CONFIG_SIZE + HIP5_WLAN_MIB_SIZE + \
				 HIP5_WLAN_ZERO_COPY_TX_SIZE + HIP5_WLAN_ZERO_COPY_RX_SIZE + HIP5_WLAN_ZERO_COPY_DPD_BUF_SIZE)
#else
#define HIP5_WLAN_TX_DATA_SIZE	    		0x340000 /* 3.25 MB */
#define HIP5_WLAN_ZERO_COPY_TX_DATA_SIZE	0x80000  /* 512 KB */
#define HIP5_WLAN_TX_SIZE	        		(HIP5_WLAN_TX_DATA_SIZE + HIP5_WLAN_TX_CTRL_SIZE)
#define HIP5_WLAN_ZERO_COPY_TX_SIZE		(HIP5_WLAN_ZERO_COPY_TX_DATA_SIZE + HIP5_WLAN_TX_CTRL_SIZE)

/*** RX POOL ***/
#define HIP5_WLAN_RX_OFFSET					(HIP5_WLAN_TX_DATA_OFFSET +  HIP5_WLAN_TX_DATA_SIZE)
#define HIP5_WLAN_RX_SIZE					0x80000 /* 512 KB */
#define HIP5_WLAN_ZERO_COPY_RX_OFFSET	(HIP5_WLAN_TX_DATA_OFFSET +  HIP5_WLAN_ZERO_COPY_TX_DATA_SIZE)
#define HIP5_WLAN_ZERO_COPY_RX_SIZE		0x200000 /* 2 MB */

/*** TOTAL : CONFIG POOL + TX POOL + RX POOL ***/
#define HIP5_WLAN_TOTAL_MEM	(HIP5_WLAN_CONFIG_SIZE + HIP5_WLAN_MIB_SIZE + \
	HIP5_WLAN_TX_SIZE + HIP5_WLAN_RX_SIZE)

#define HIP5_WLAN_ZERO_COPY_TOTAL_MEM	(HIP5_WLAN_CONFIG_SIZE + HIP5_WLAN_MIB_SIZE + \
				 HIP5_WLAN_ZERO_COPY_TX_SIZE + HIP5_WLAN_ZERO_COPY_RX_SIZE)
#endif

#define HIP5_DAT_MBULK_SIZE	(2 * 1024)
#define HIP5_DAT_SLOTS		(HIP5_WLAN_TX_DATA_SIZE / HIP5_DAT_MBULK_SIZE)
#define HIP5_CTL_MBULK_SIZE	(2 * 1024)
#define HIP5_CTL_SLOTS		(HIP5_WLAN_TX_CTRL_SIZE / HIP5_CTL_MBULK_SIZE)

#define SLSI_HIP_HIP5_OPT_INVALID_OFFSET 254
#define SLSI_HIP_HIP5_OPT_DMA_LEN_ALIGN  4

/* external */
#define SLSI_HIP_TX_DATA_SLOTS_NUM HIP5_DAT_SLOTS
#define SLSI_HIP_TX_ZERO_COPY_NUM_DATA_SLOTS 4096

#define HIP5_CONFIG_INIT_MAGIC_NUM 0xcaaa0400

#define MIF_HIP_CFG_Q_NUM              24

#define MIF_HIP_COMPAT_FLAG_RX_ZERO_COPY     (2 << 1)

#ifdef CONFIG_SCSC_WLAN_HOST_DPD
#define HIP5_CONFIG_RESERVED_BYTES_NUM 6
#else
#define HIP5_CONFIG_RESERVED_BYTES_NUM 16
#endif

#define HIP5_OPT_NUM_BD_PER_SIGNAL_MAX 64

/* Check if the buffer address is a SKB */
#define HIP5_OPT_BD_BUF_ADDR_IS_SKB(addr)   ((addr) & 0x1)

enum hip5_hip_q_conf {
	/* UCPU0 FH */
	HIP5_MIF_Q_FH_CTRL = 0,
    HIP5_MIF_Q_FH_PRI0,
    HIP5_MIF_Q_FH_SKB0,
    HIP5_MIF_Q_FH_RFBC,
    HIP5_MIF_Q_FH_RFBD0,

    /* UCPU0 TH */
    HIP5_MIF_Q_TH_CTRL,
    HIP5_MIF_Q_TH_DAT0,
    HIP5_MIF_Q_TH_RFBC,
    HIP5_MIF_Q_TH_RFBD0,

    /* UCPU1 FH */
    HIP5_MIF_Q_FH_PRI1,
    HIP5_MIF_Q_FH_SKB1,
    HIP5_MIF_Q_FH_RFBD1,
    HIP5_MIF_Q_TH_DAT1,
    HIP5_MIF_Q_TH_RFBD1,

    /* FH DATA */
    HIP5_MIF_Q_FH_DAT0,
    HIP5_MIF_Q_FH_DAT1,
    HIP5_MIF_Q_FH_DAT2,
    HIP5_MIF_Q_FH_DAT3,
    HIP5_MIF_Q_FH_DAT4,
    HIP5_MIF_Q_FH_DAT5,
    HIP5_MIF_Q_FH_DAT6,
    HIP5_MIF_Q_FH_DAT7,
    HIP5_MIF_Q_FH_DAT8,
    HIP5_MIF_Q_FH_DAT9
};

struct hip5_mif_q {
	u8  q_type;
	u16 q_len;
	u16 q_idx_sz;
	u16 q_entry_sz;
	u8  int_n;
	u32 q_loc;
	u8  ucpu;
	u8  vif;
} __packed;

struct hip5_hip_config_version_1 {
	/* Host owned */
	u32 magic_number;       /* 0xcaba0401 */
	u16 hip_config_ver;     /* Version of this configuration structure = 2*/
	u16 config_len;         /* Size of this configuration structure */

	/* FW owned */
	u32 compat_flag;         /* flag of the expected driver's behaviours */

	u16 sap_mlme_ver;        /* Fapi SAP_MLME version*/
	u16 sap_ma_ver;          /* Fapi SAP_MA version */
	u16 sap_debug_ver;       /* Fapi SAP_DEBUG version */
	u16 sap_test_ver;        /* Fapi SAP_TEST version */

	u32 fw_build_id;         /* Firmware Build Id */
	u32 fw_patch_id;         /* Firmware Patch Id */

	u8  unidat_req_headroom; /* Headroom the host shall reserve in mbulk for MA-UNITDATA.REQ signal */
	u8  unidat_req_tailroom; /* Tailroom the host shall reserve in mbulk for MA-UNITDATA.REQ signal */
	u8  bulk_buffer_align;   /* 4 */

	/* Host owned */
	u8  host_cache_line;    /* 64 */

	u32 host_buf_loc;       /* location of the host buffer in MIF_ADDR */
	u32 host_buf_sz;        /* in byte, size of the host buffer */
	u32 fw_buf_loc;         /* location of the firmware buffer in MIF_ADDR */
	u32 fw_buf_sz;          /* in byte, size of the firmware buffer */
	u32 mib_loc;            /* MIB location in MIF_ADDR */
	u32 mib_sz;             /* MIB size */
	u32 log_config_loc;     /* Logging Configuration Location in MIF_ADDR */
	u32 log_config_sz;      /* Logging Configuration Size in MIF_ADDR */
	u32 scbrd_loc;          /* Scoreboard locatin in MIF_ADDR */

	u16 q_num;              /* 24 */
	struct hip5_mif_q q_cfg[MIF_HIP_CFG_Q_NUM];
} __packed;

struct hip5_hip_config_version_2 {
	/* Host owned */
	u32 magic_number;       /* 0xcaba0401 */
	u16 hip_config_ver;     /* Version of this configuration structure = 2*/
	u16 config_len;         /* Size of this configuration structure */

	/* FW owned */
	u32 compat_flag;         /* flag of the expected driver's behaviours */

	u16 sap_mlme_ver;        /* Fapi SAP_MLME version*/
	u16 sap_mlme_sub_ver;    /* Fapi SAP_MLME sub version*/
	u16 sap_ma_ver;          /* Fapi SAP_MA version */
	u16 sap_ma_sub_ver;      /* Fapi SAP_MA sub version */
	u16 sap_debug_ver;       /* Fapi SAP_DEBUG version */
	u16 sap_debug_sub_ver;   /* Fapi SAP_DEBUG sub version */
	u16 sap_test_ver;        /* Fapi SAP_TEST version */
	u16 sap_test_sub_ver;    /* Fapi SAP_TEST sub version */

	u32 fw_build_id;         /* Firmware Build Id */
	u32 fw_patch_id;         /* Firmware Patch Id */

	u8  unidat_req_headroom; /* Headroom the host shall reserve in mbulk for MA-UNITDATA.REQ signal */
	u8  unidat_req_tailroom; /* Tailroom the host shall reserve in mbulk for MA-UNITDATA.REQ signal */
	u8  bulk_buffer_align;   /* 4 */

	/* Host owned */
	u8  host_cache_line;    /* 64 */

	u32 host_buf_loc;       /* location of the host buffer in MIF_ADDR */
	u32 host_buf_sz;        /* in byte, size of the host buffer */
	u32 fw_buf_loc;         /* location of the firmware buffer in MIF_ADDR */
	u32 fw_buf_sz;          /* in byte, size of the firmware buffer */
	u32 mib_loc;            /* MIB location in MIF_ADDR */
	u32 mib_sz;             /* MIB size */
	u32 log_config_loc;     /* Logging Configuration Location in MIF_ADDR */
	u32 log_config_sz;      /* Logging Configuration Size in MIF_ADDR */
	u32 scbrd_loc;          /* Scoreboard locatin in MIF_ADDR */

	u16 q_num;              /* 24 */
#ifdef CONFIG_SCSC_WLAN_HOST_DPD
	u32 dpd_buf_loc;	 /* Host allocated DPD buffer Location in MIF_ADDR */
	u32 dpd_buf_sz; 	 /* Host allocated DPD buffer Size in MIF_ADDR */
	u8 intr_from_host_dpd;
	u8 intr_to_host_dpd;
#endif
	u8 reserved[HIP5_CONFIG_RESERVED_BYTES_NUM];
	struct hip5_mif_q q_cfg[MIF_HIP_CFG_Q_NUM];
} __packed;

struct hip5_hip_init {
	/* Host owned */
	u32 magic_number;       /* 0xcaaa0400 */
	/* FW owned */
	u32 conf_hip5_ver;
	/* Host owned */
	u32 version_a_ref;      /* Location of Config structure A (old) */
	u32 version_b_ref;      /* Location of Config structure B (new) */
} __packed;

#define MAX_NUM 2048

/* definitions HIP5 config version 5 */
#define HIP5_OPT_BD_CHAIN_START                 (1 << 0)

struct hip5_opt_bulk_desc {
	u32 buf_addr;      	/* Buffer address
						 * LSB 0: from Shared DRAM without shifting,
						 * LSB 1: (address & ~0x1) << 8
						 */
	u16 data_len:12;   	/* length of data */
	u16 buf_sz:4;      	/* Max buffer size scaling factor: buffer size = 512 * (1 << buf_sz) */
	u8  offset;        	/* start offset of data */
	u8  flag;          	/* Chained, types of memory */
} __packed; 			/* 8 bytes */

#define HIP5_OPT_HIP_HEADER_LEN 4

struct hip5_opt_hip_signal {
	u8 sig_format;	/* reserved for future use */
	u8 sig_len;	 	/* 0 or signal length defined in FAPI,
					 * 0: signal omitted
					 */
	u8 num_bd;
	u8 wake_up:1;	/* indicates that this signal caused the firmware to wake-up the host */
	u8 reserved:7;	/* reserved for future use */
} __packed __aligned(64);


struct hip5_hip_q_tlv {
	struct hip5_opt_hip_signal array[MAX_NUM];
	u16  idx_read;      /* To keep track */
	u16  idx_write;     /* To keep track */
	u16  total;
} __aligned(64);

struct hip5_hip_q {
	u32 array[MAX_NUM];
	u16  idx_read;
	u16  idx_write;
	u16  total;
} __aligned(64);

struct hip5_hip_control {
	struct hip5_hip_init             init;
	struct hip5_hip_config_version_2 config_v5 __aligned(64);
	struct hip5_hip_config_version_1 config_v4 __aligned(64);
	u32                              scoreboard[256] __aligned(64);
	struct hip5_hip_q                q[14] __aligned(64);
	struct hip5_hip_q_tlv            q_tlv[5] __aligned(64);
} __aligned(4096);

struct slsi_hip;
#ifdef CONFIG_SCSC_WLAN_LOAD_BALANCE_MANAGER
struct bh_struct;
#endif

#define SLSI_HIP_HIP5_OPT_TX_Q_MAX 32
struct hip5_opt_tx_q {
	u16                 vif_index;
	u16                 flow_id;
	u16                 configuration_option;
	u8                  addr3[ETH_ALEN];
	struct sk_buff_head tx_q;
	ktime_t             last_sent;
};

struct hip5_tx_skb_entry {
	atomic_t in_use;
	u32 colour;
	u32 skb_dma_addr;
	u16 dma_map_len;
	struct sk_buff *skb;
};

struct hip5_tx_skb_meta {
	u16 idx;             /* index in the look-up table */
} __packed;

struct hip5_rx_skb_entry {
	atomic_t in_use;
	u32 skb_dma_addr;
	struct sk_buff *skb;
};

/* This struct is private to the HIP implementation */
struct hip_priv {
	spinlock_t                   napi_cpu_lock;

	struct bh_struct            *bh_dat;
	struct bh_struct            *bh_ctl;
	struct bh_struct            *bh_rfb;
#ifdef CONFIG_SLSI_WLAN_LPC
	struct bh_struct            *bh_lpc;
#endif

	/* Interrupts cache */
	u32                          intr_from_host_ctrl;
	u32                          intr_to_host_ctrl;
	u32                          intr_to_host_ctrl_fb;
	u32                          intr_from_host_data;
	u32                          intr_to_host_data1;
	u32                          intr_to_host_data2;
#ifdef CONFIG_SCSC_WLAN_HOST_DPD
	u32                          intr_from_host_dpd;
	u32                          intr_to_host_dpd;
#endif

	/* For workqueue */
	struct slsi_hip             *hip;

	/* Pool for data frames*/
	u8                           host_pool_id_dat;
	/* Pool for ctl frames*/
	u8                           host_pool_id_ctl;

	/* rx cycle lock */
	spinlock_t                   rx_lock;
	/* tx cycle lock */
	spinlock_t                   tx_lock;

	/* Scoreboard update spinlock */
	rwlock_t                     rw_scoreboard;

#ifdef CONFIG_SCSC_WLAN_ANDROID
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct scsc_wake_lock             wake_lock_tx;
	struct scsc_wake_lock             wake_lock_ctrl;
	struct scsc_wake_lock             wake_lock_data;
#else

	struct wake_lock             wake_lock_tx;
	struct wake_lock             wake_lock_ctrl;
	struct wake_lock             wake_lock_data;
#endif
#endif

	/* Control the hip instance deinit */
	atomic_t                     closing;
	atomic_t                     in_tx;
	atomic_t                     in_rx;
	atomic_t                     in_suspend;
	atomic_t                     in_logring_disable;
	u32                          storm_count;
	u32                          th_w_idx_err_count;

	/*minor*/
	u32                          minor;
	u8                           unidat_req_headroom; /* Headroom the host shall reserve in mbulk for MA-UNITDATA.REQ signal */
	u8                           unidat_req_tailroom; /* Tailroom the host shall reserve in mbulk for MA-UNITDATA.REQ signal */
	u32                          version; /* Version of the running FW */
	u32                          compat_flag; /* Feature compatibility flag */

	void                         *scbrd_base; /* Scbrd_base pointer */
	__iomem void                 *scbrd_ramrp_base; /* Scbrd_base pointer in RAMRP */

#ifndef CONFIG_SCSC_WLAN_TX_API
	/* Global domain Q control*/
	atomic_t                     gactive;
	atomic_t                     gmod;
	atomic_t                     gcod;
	int                          saturated;
	int                          guard;
	/* Global domain Q spinlock */
	spinlock_t                   gbot_lock;
#endif
#ifdef CONFIG_SCSC_QOS
	/* PM QoS control */
	struct work_struct           pm_qos_work;
	/* PM QoS control spinlock */
	spinlock_t                   pm_qos_lock;
	u8                           pm_qos_state;
#endif
	/* Collection artificats */
	void                         *mib_collect;
	u16                          mib_sz;
	/* Mutex to protect hcf file collection if a tear down is triggered */
	struct mutex                 in_collection;

	struct workqueue_struct      *hip_workq;
	struct hip5_opt_tx_q         hip5_opt_tx_q[SLSI_HIP_HIP5_OPT_TX_Q_MAX];
	bool mx_pci_claim_data;
	bool mx_pci_claim_fb;

	/* TX zero copy */
	u16                          tx_skb_free_cnt; /* for HIP sampling */
	struct hip5_tx_skb_entry     tx_skb_table[SLSI_HIP_TX_ZERO_COPY_NUM_DATA_SLOTS];

	/* RX zero copy */
	struct hip5_rx_skb_entry rx_skb_table[MAX_NUM];
};

struct scsc_service;

/* Macros for accessing information stored in the hip_config struct */
#define scsc_wifi_get_hip_config_version_4_u8(buff_ptr, member) le16_to_cpu((((struct hip5_hip_config_version_1 *)(buff_ptr))->member))
#define scsc_wifi_get_hip_config_version_4_u16(buff_ptr, member) le16_to_cpu((((struct hip5_hip_config_version_1 *)(buff_ptr))->member))
#define scsc_wifi_get_hip_config_version_4_u32(buff_ptr, member) le32_to_cpu((((struct hip5_hip_config_version_1 *)(buff_ptr))->member))
#define scsc_wifi_get_hip_config_version_5_u8(buff_ptr, member) le16_to_cpu((((struct hip5_hip_config_version_2 *)(buff_ptr))->member))
#define scsc_wifi_get_hip_config_version_5_u16(buff_ptr, member) le16_to_cpu((((struct hip5_hip_config_version_2 *)(buff_ptr))->member))
#define scsc_wifi_get_hip_config_version_5_u32(buff_ptr, member) le32_to_cpu((((struct hip5_hip_config_version_2 *)(buff_ptr))->member))
#define scsc_wifi_get_hip_config_u8(buff_ptr, member, ver) le16_to_cpu((((struct hip5_hip_config_version_1 *)(buff_ptr->config_v##ver))->member))
#define scsc_wifi_get_hip_config_u16(buff_ptr, member, ver) le16_to_cpu((((struct hip5_hip_config_version_##ver *)(buff_ptr->config_v##ver))->member))
#define scsc_wifi_get_hip_config_u32(buff_ptr, member, ver) le32_to_cpu((((struct hip5_hip_config_version_2 *)(buff_ptr->config_v##ver))->member))
#define scsc_wifi_get_hip_config_version(buff_ptr) le32_to_cpu((((struct hip5_hip_init *)(buff_ptr))->conf_hip5_ver))

#endif /* __HIP5_H__ */
