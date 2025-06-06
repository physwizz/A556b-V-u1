/****************************************************************************
 *
 * Copyright (c) 2014 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/
/**
 * Maxwell mxlogger (Interface)
 *
 * Provides bi-directional communication between the firmware and the
 * host.
 *
 */
#ifndef __MX_LOGGER_H__
#define __MX_LOGGER_H__
#include <linux/types.h>
#include <scsc/scsc_mifram.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/completion.h>
#include <linux/jiffies.h>
#include "scsc_mif_abs.h"
#include "mxmgmt_transport_format.h"
#include <scsc/scsc_log_collector.h>
/**
 * ___________________________________________________________________
 * |    Cmd       |    Arg       |   ...  payload (opt)  ...         |
 * -------------------------------------------------------------------
 * <-- uint8_t --><-- uint8_t --><-----  uint8_t[] buffer ----------->
 *
 */
#define	MXLOGGER_RINGS_TMO_US		200000
/* CMD/EVENTS */
#define MM_MXLOGGER_LOGGER_CMD			(0)
#define MM_MXLOGGER_DIRECTION_CMD		(1)
#define MM_MXLOGGER_CONFIG_CMD			(2)
#define MM_MXLOGGER_INITIALIZED_EVT		(3)
#define MM_MXLOGGER_SYNC_RECORD			(4)
#define	MM_MXLOGGER_STARTED_EVT			(5)
#define	MM_MXLOGGER_STOPPED_EVT			(6)
#define	MM_MXLOGGER_COLLECTION_FW_REQ_EVT	(7)
#if defined(CONFIG_CHIPLOGGER_V_2_0)
#define MM_MXLOGGER_DUMP_BUFFER_RT_CMD		(8)
#define MM_MXLOGGER_DUMP_BUFFER_RT_FLUSHED_EVT  (9)
#endif
/* ARG - LOGGER */
#define MM_MXLOGGER_LOGGER_ENABLE		(0)
#define MM_MXLOGGER_LOGGER_DISABLE		(1)
#define MM_MXLOGGER_DISABLE_REASON_STOP		(0)
#define MM_MXLOGGER_DISABLE_REASON_COLLECTION	(1)
/* ARG - DIRECTION */
#define MM_MXLOGGER_DIRECTION_DRAM	(0)
#define MM_MXLOGGER_DIRECTION_HOST	(1)
/* ARG - CONFIG TABLE */
#define MM_MXLOGGER_CONFIG_BASE_ADDR	(0)
/* ARG - CONFIG TABLE */
#define MM_MXLOGGER_SYNC_INDEX		(0)
#define MM_MXLOGGER_PAYLOAD_SZ          (MXMGR_MESSAGE_PAYLOAD_SIZE - 2)

#if defined(CONFIG_SOC_EXYNOS3830) || defined(CONFIG_SOC_EXYNOS7885)
#define MXL_INTERNAL_RSV		(24 * 1024)
#define MXL_POOL_SZ			((4 * 1024 * 1024) - MXL_INTERNAL_RSV)
#define MXLOGGER_RSV_COMMON_SZ		(2 * 1024)
#define MXLOGGER_RSV_WLAN_SZ		(4 * 1024)
#define MXLOGGER_RSV_RADIO_SZ		(2 * 1024)
#define MXLOGGER_RSV_BT_SZ		(4 * 1024)

#elif defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#define MXL_INTERNAL_RSV		(24 * 1024)
#ifdef CONFIG_SCSC_64KB_ALLIGNED_MEMLOG
#define MX_DRAM_SIZE_SECTION_LOG	(8 * 1024 * 1024 + 64 * 1024)
#else
#define MX_DRAM_SIZE_SECTION_LOG	(8 * 1024 * 1024)
#endif
#define MX_DRAM_SIZE_SECTION_WLAN	(6 * 1024 * 1024)
#define MX_DRAM_SIZE_SECTION_WPAN	(2 * 1024 * 1024)
#define MXL_POOL_SZ			(MX_DRAM_SIZE_SECTION_LOG - MXL_INTERNAL_RSV)
#define MXL_POOL_SZ_WLAN		(MX_DRAM_SIZE_SECTION_WLAN - MXL_INTERNAL_RSV)
#define MXL_POOL_SZ_WPAN		(MX_DRAM_SIZE_SECTION_WPAN  - MXL_INTERNAL_RSV)
#define MXLOGGER_RSV_COMMON_SZ		(0 * 1024)
#define MXLOGGER_RSV_WLAN_SZ		(4 * 1024 * 1024)
#define MXLOGGER_RSV_RADIO_SZ		(0 * 1024)
#define MXLOGGER_RSV_BT_SZ		(0 * 1024)
#define MXLOGGER_RSV_COMMON_SZ_WPAN	(0 * 1024)
#define MXLOGGER_RSV_WLAN_SZ_WPAN	(0 * 1024)
#define MXLOGGER_RSV_RADIO_SZ_WPAN	(0 * 1024)
#define MXLOGGER_RSV_BT_SZ_WPAN		(4 * 1024)
#else
#define MXL_POOL_SZ			(6 * 1024 * 1024)
#define MXLOGGER_RSV_COMMON_SZ		(4 * 1024)
#define MXLOGGER_RSV_WLAN_SZ		(2 * 1024 * 1024)
#define MXLOGGER_RSV_RADIO_SZ		(4 * 1024)
#define MXLOGGER_RSV_BT_SZ		(4 * 1024)
#endif

#define MXLOGGER_SYNC_SIZE		(10 * 1024)
#define MXLOGGER_IMP_SIZE		(102 * 1024)
#if defined(CONFIG_CHIPLOGGER_V_2_0)
#define MXLOGGER_IMPD12_SIZE		(768 * 1024)
#define MXLOGGER_LINK_SIZE              (256 * 1024)
#else
#define MXLOGGER_IMPD12_SIZE		(0)
#define MXLOGGER_LINK_SIZE              (0)
#endif
#define MXLOGGER_TOTAL_FIX_BUF		(MXLOGGER_SYNC_SIZE + MXLOGGER_IMP_SIZE + MXLOGGER_IMPD12_SIZE + \
					MXLOGGER_LINK_SIZE + MXLOGGER_RSV_COMMON_SZ + MXLOGGER_RSV_BT_SZ + \
					MXLOGGER_RSV_WLAN_SZ + MXLOGGER_RSV_RADIO_SZ)
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#define MXLOGGER_TOTAL_FIX_BUF_WPAN	(MXLOGGER_SYNC_SIZE + MXLOGGER_IMP_SIZE + \
					MXLOGGER_RSV_COMMON_SZ_WPAN + MXLOGGER_RSV_BT_SZ_WPAN + \
					MXLOGGER_RSV_WLAN_SZ_WPAN + MXLOGGER_RSV_RADIO_SZ_WPAN)
#endif
#define MXLOGGER_NON_FIX_BUF_ALIGN	32
#define MXLOGGER_MAGIG_NUMBER		0xcaba0401
#define MXLOGGER_MAJOR			0
#define MXLOGGER_MINOR			0
#define NUM_SYNC_RECORDS		256
#define SYNC_MASK			(NUM_SYNC_RECORDS - 1)
/* Shared memory Layout
 *
 * |-------------------------| CONFIG
 * |    CONFIG AREA          |
 * |    ...                  |
 * |    *bufs     	     |------|
 * |    ....                 |      |
 * |    ....                 |      |
 * |  --------------------   |<-----|
 * |                         |
 * |  loc | sz | state |info |---------------------|
 * |  loc | sz | state |info |---------------------|
 * |  loc | sz | state |info |---------------------|
 * |    ...                  |                     |
 * |-------------------------| Fixed size buffers  |
 * |     SYNC  BUFFER        |<--------------------|
 * |-------------------------| 			   |
 * |     IMPORTANT EVENTS    |<--------------------|
 * |-------------------------|                     |
 * |    Reserved COMMON      |<--------------------|
 * |-------------------------|
 * |    Reserved BT          |
 * |-------------------------|
 * |    Reserved WL          |
 * |-------------------------|
 * |    Reserved RADIO       |
 * |-------------------------|
 * |         MXLOG           |<--------------------|
 * |-------------------------| Variable size buffers
 * |         UDI             |
 * |-------------------------| Fixed size buffers  |
 * |   IMPORTANT D12 LOGS    |<--------------------|
 * |-------------------------|			   |
 * |       LINK STATS        |<--------------------|
 * |-------------------------|
 * |  Future buffers (TBD)   |
 * |-------------------------|
 * |  Future buffers (TBD)   |
 * |-------------------------|
 */
enum mxlogger_buffers {
	MXLOGGER_FIRST_FIXED_SZ,
	MXLOGGER_SYNC = MXLOGGER_FIRST_FIXED_SZ,
	MXLOGGER_IMP,
	MXLOGGER_RESERVED_COMMON,
	MXLOGGER_RESERVED_BT,
	MXLOGGER_RESERVED_WLAN,
	MXLOGGER_RESERVED_RADIO,
	MXLOGGER_MXLOG,
	MXLOGGER_UDI,
#if defined(CONFIG_CHIPLOGGER_V_2_0)
	MXLOGGER_IMPD12,
	MXLOGGER_LINK,
#endif
	MXLOGGER_NUM_BUFFERS
};

enum mxlogger_sync_event {
	MXLOGGER_SYN_SUSPEND,
	MXLOGGER_SYN_RESUME,
	MXLOGGER_SYN_TOHOST,
	MXLOGGER_SYN_TORAM,
	MXLOGGER_SYN_LOGCOLLECTION,
};

struct mxlogger_sync_record {
	u64 tv_sec; /* struct timeval.tv_sec */
	u64 tv_usec; /* struct timeval.tv_usec */
	u64 kernel_time; /* ktime_t */
	u32 sync_event; /* type of sync event*/
	u32 fw_time;
	u32 fw_wrap;
	u8  reserved[4];
} __packed;

struct buffer_desc {
	u32 location;			/* Buffer location */
	u32 size;			/* Buffer sz (in bytes) */
	u32 status;			/* buffer status */
	u32 info;			/* buffer info */
} __packed;

struct mxlogger_config {
	u32			magic_number;   /* 0xcaba0401 */
	u32			config_major;	/* Version Major */
	u32			config_minor;	/* Version Minor */
	u32			num_buffers;	/* configured buffers */
	scsc_mifram_ref		bfds_ref;
} __packed;

struct mxlogger_config_area {
	struct mxlogger_config	config;
	struct buffer_desc	bfds[MXLOGGER_NUM_BUFFERS];
	uint8_t	*buffers_start;
} __packed;

struct log_msg_packet {
	uint8_t		msg;		/* cmd or event id */
	uint8_t		arg;
	uint8_t		payload[MM_MXLOGGER_PAYLOAD_SZ];
} __packed;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#define MXLOGGER_CHANNELS		2
#define MXLOGGER_CHANNEL_WLAN		0
#define MXLOGGER_CHANNEL_WPAN		1

#if IS_ENABLED(CONFIG_BT_FWSNOOP_LOGGING)
#define MXLOGGER_CLASS_REALTIME		(0)
#define MXLOGGER_CLASS_RELAXED		(1)
#define MXLOGGER_CLASS_REGISTERED	(2)

#define MXLOGGER_BITPOS_REALTIME	(4) /* For the number of observer w/ REALTIME class */
#define MXLOGGER_BITPOS_RELAXED		(0)	/* For the number of observer w/ RELAXED class. */
#endif

struct mxlogger_channel {
	void				*mem;
	void				*mem_sync_buf;
	uint32_t			msz;
	scsc_mifram_ref			mifram_ref;
	struct mxlogger_config_area	*cfg;
	u8				sync_buffer_index;
	enum scsc_mif_abs_target	target;
	struct completion		rings_serialized_ops;
	bool				enabled;
	bool				configured;
	struct mxlogger			*mxlogger;
	bool				re_enable;
	bool				crashed;
};

struct mxlogger {
	bool				initialized;
	bool				configured;
	bool				enabled;
	struct scsc_mx			*mx;
	struct mxlogger_channel		chan[MXLOGGER_CHANNELS];
	struct mutex			lock;
	struct mutex			chan_lock;
	u8				observers;
#if IS_ENABLED(CONFIG_BT_FWSNOOP_LOGGING)
	/* Number of observers with different classes in each bit region. */
	uint8_t			registered_class;
#endif
};
#else
struct mxlogger {
	bool				initialized;
	bool				configured;
	bool				enabled;
	struct scsc_mx			*mx;
	void				*mem;
	void				*mem_sync_buf;
	uint32_t			msz;
	scsc_mifram_ref			mifram_ref;
	struct mutex			lock;
	struct mxlogger_config_area	*cfg;
	u8				observers;
	u8				sync_buffer_index;
	/* collection variables */
	bool				re_enable;
	struct completion		rings_serialized_ops;
};

#endif
int mxlogger_generate_sync_record(struct mxlogger *mxlogger, enum mxlogger_sync_event event);
int mxlogger_dump_shared_memory_to_file(struct mxlogger *mxlogger);
int mxlogger_init(struct scsc_mx *mx, struct mxlogger *mxlogger, uint32_t mem_sz);
void mxlogger_deinit(struct scsc_mx *mx, struct mxlogger *mxlogger);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
int mxlogger_init_channel(struct mxlogger *mxlogger, enum scsc_mif_abs_target target);
int mxlogger_init_transport_channel(struct mxlogger *mxlogger, enum scsc_mif_abs_target target);
unsigned int mxlogger_get_channel_ref(struct mxlogger *mxlogger, enum scsc_mif_abs_target target);
unsigned int mxlogger_get_channel_len(struct mxlogger *mxlogger, enum scsc_mif_abs_target target);
int mxlogger_start_channel(struct mxlogger *mxlogger, enum scsc_mif_abs_target target);
int mxlogger_stop_channel(struct mxlogger *mxlogger, enum scsc_mif_abs_target target);
void mxlogger_deinit_channel(struct mxlogger *mxlogger, enum scsc_mif_abs_target target);
void mxlogger_mark_channel_as_crashed(struct mxlogger *mxlogger, enum scsc_mif_abs_target target);
#else
int mxlogger_start(struct mxlogger *mxlogger);
#endif
int mxlogger_register_observer(struct mxlogger *mxlogger, char *name);
int mxlogger_unregister_observer(struct mxlogger *mxlogger, char *name);
int mxlogger_register_global_observer(char *name);
int mxlogger_unregister_global_observer(char *name);
#if IS_ENABLED(CONFIG_BT_FWSNOOP_LOGGING)
int mxlogger_register_observer_class(struct mxlogger *mxlogger, char *name, uint8_t class);
int mxlogger_unregister_observer_class(struct mxlogger *mxlogger, char *name, uint8_t class);
int mxlogger_register_global_observer_class(char *name, uint8_t class);
int mxlogger_unregister_global_observer_class(char *name, uint8_t class);
#endif
bool mxlogger_set_enabled_status(bool enable);
size_t mxlogger_dump_fw_buf(struct mxlogger *mxlogger, enum scsc_log_chunk_type fw_buffer, void *buf, size_t size,
			      enum scsc_mif_abs_target target);
size_t mxlogger_get_fw_buf_size(struct mxlogger *mxlogger, enum scsc_log_chunk_type fw_buffer,
				enum scsc_mif_abs_target target);
#if defined(CONFIG_SLSI_WLAN_LPC)
void* mxlogger_get_fw_buf_for_wlan_lpc(struct mxlogger *mxlogger);
#endif
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#define MEM_LAYOUT_CHECK()	\
({				\
	BUILD_BUG_ON((sizeof(struct mxlogger_sync_record) * NUM_SYNC_RECORDS) >  MXLOGGER_SYNC_SIZE); \
	BUILD_BUG_ON((MXLOGGER_TOTAL_FIX_BUF + sizeof(struct mxlogger_config_area))  > MXL_POOL_SZ_WLAN); \
	BUILD_BUG_ON((MXLOGGER_TOTAL_FIX_BUF_WPAN + sizeof(struct mxlogger_config_area))  > MXL_POOL_SZ_WPAN ); \
	BUILD_BUG_ON((MXL_POOL_SZ_WPAN + MXL_POOL_SZ_WLAN) > MXL_POOL_SZ); \
})
#else
#define MEM_LAYOUT_CHECK()	\
({				\
	BUILD_BUG_ON((sizeof(struct mxlogger_sync_record) * NUM_SYNC_RECORDS) >  MXLOGGER_SYNC_SIZE); \
	BUILD_BUG_ON((MXLOGGER_TOTAL_FIX_BUF + sizeof(struct mxlogger_config_area))  > MXL_POOL_SZ); \
})
#endif

#endif /* __MX_LOGGER_H__ */
