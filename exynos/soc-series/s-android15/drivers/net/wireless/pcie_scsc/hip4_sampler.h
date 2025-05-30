/****************************************************************************
 *
 * Copyright (c) 2014 - 2022 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __HIP4_SAMPLER_H__
#define __HIP4_SAMPLER_H__

#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/seq_file.h>

#include "dev.h"

#define HIP4_SAMPLER_SIGNAL_CTRLTX      0x20
#define HIP4_SAMPLER_SIGNAL_CTRLRX      0x21
#define HIP4_SAMPLER_THROUG             0x22
#define HIP4_SAMPLER_THROUG_K           0x23
#define HIP4_SAMPLER_THROUG_M           0x24
#define HIP4_SAMPLER_STOP_Q             0x25
#define HIP4_SAMPLER_START_Q            0x26
#define HIP4_SAMPLER_QREF               0x27
#define HIP4_SAMPLER_PEER               0x29
#define HIP4_SAMPLER_BOT_RX             0x2a
#define HIP4_SAMPLER_BOT_TX             0x2b
#define HIP4_SAMPLER_BOT_ADD            0x2c
#define HIP4_SAMPLER_BOT_REMOVE         0x2d
#define HIP4_SAMPLER_BOT_STOP_Q         0x2e
#define HIP4_SAMPLER_BOT_START_Q        0x2f
#define HIP4_SAMPLER_BOT_QMOD_RX        0x30
#define HIP4_SAMPLER_BOT_QMOD_TX        0x31
#define HIP4_SAMPLER_BOT_QMOD_STOP      0x32
#define HIP4_SAMPLER_BOT_QMOD_START     0x33
#define HIP4_SAMPLER_PKT_TX             0x40
#define HIP4_SAMPLER_PKT_TX_HIP4        0x41
#define HIP4_SAMPLER_PKT_TX_FB          0x42
#define HIP4_SAMPLER_SUSPEND            0x50
#define HIP4_SAMPLER_RESUME             0x51

#define HIP4_SAMPLER_MBULK              0xaa
#define HIP4_SAMPLER_QFULL              0xbb
#define HIP4_SAMPLER_MFULL              0xcc
#define HIP4_SAMPLER_INT                0xdd
#define HIP4_SAMPLER_INT_OUT            0xee
#define HIP4_SAMPLER_INT_BH             0xde
#define HIP4_SAMPLER_INT_OUT_BH		0xef
#define HIP4_SAMPLER_RESET              0xff

#define SCSC_HIP4_INTERFACES	1

#define SCSC_HIP4_STREAM_CH     1
#define SCSC_HIP4_OFFLINE_CH    SCSC_HIP4_STREAM_CH

#if (SCSC_HIP4_OFFLINE_CH != SCSC_HIP4_STREAM_CH)
#error "SCSC_HIP4_STREAM_CH has to be equal to SCSC_HIP4_OFFLINE_CH"
#endif

#define SCSC_HIP4_DEBUG_INTERFACES	((SCSC_HIP4_INTERFACES) * (SCSC_HIP4_STREAM_CH + SCSC_HIP4_OFFLINE_CH))

struct scsc_mx;

void hip4_sampler_create(struct slsi_dev *sdev, struct scsc_mx *mx);
void hip4_sampler_destroy(struct slsi_dev *sdev, struct scsc_mx *mx);

/* Register hip4 instance with the logger */
/* return char device minor associated with the maxwell instance*/
int hip4_sampler_register_hip(struct scsc_mx *mx);

void hip4_sampler_update_record(u32 minor, u8 param1, u8 param2, u8 param3, u8 param4, u32 param5);

#ifdef CONFIG_SCSC_WLAN_HIP4_PROFILING
#ifdef CONFIG_SCSC_WLAN_HIP5
#define SCSC_HIP4_SAMPLER_Q(minor, q, idx_rw, value, rw) \
	hip4_sampler_update_record(minor, q, idx_rw, 0, rw, value)
#else
#define SCSC_HIP4_SAMPLER_Q(minor, q, idx_rw, value, rw) \
	hip4_sampler_update_record(minor, q, idx_rw, value, rw, 0)
#endif
#define SCSC_HIP4_SAMPLER_QREF(minor, ref, q) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_QREF, (ref & 0xff0000) >> 16, (ref & 0xff00) >> 8, (ref & 0xf0) | q, 0)

#define SCSC_HIP4_SAMPLER_SIGNAL_CTRLTX(minor, bytes16_h, bytes16_l) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_SIGNAL_CTRLTX, 0, bytes16_h, bytes16_l, 0)

#define SCSC_HIP4_SAMPLER_SIGNAL_CTRLRX(minor, bytes16_h, bytes16_l) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_SIGNAL_CTRLRX, 0, bytes16_h, bytes16_l, 0)

#define SCSC_HIP4_SAMPLER_THROUG(minor, rx_tx, bytes16_h, bytes16_l) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_THROUG, rx_tx, bytes16_h, bytes16_l, 0)

#define SCSC_HIP4_SAMPLER_THROUG_K(minor, rx_tx, bytes16_h, bytes16_l) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_THROUG_K, rx_tx, bytes16_h, bytes16_l, 0)

#define SCSC_HIP4_SAMPLER_THROUG_M(minor, rx_tx, bytes16_h, bytes16_l) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_THROUG_M, rx_tx, bytes16_h, bytes16_l, 0)

#define SCSC_HIP4_SAMPLER_STOP_Q(minor, vif_id) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_STOP_Q, 0, 0, vif_id, 0)

#define SCSC_HIP4_SAMPLER_START_Q(minor, vif_id) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_START_Q, 0, 0, vif_id, 0)

#define SCSC_HIP4_SAMPLER_MBULK(minor, bytes16_h, bytes16_l, clas, tot_num) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_MBULK, clas, bytes16_h, bytes16_l, tot_num)

#define SCSC_HIP4_SAMPLER_QFULL(minor, q) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_QFULL, 0, 0, q, 0)

#define SCSC_HIP4_SAMPLER_MFULL(minor) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_MFULL, 0, 0, 0, 0)

#define SCSC_HIP4_SAMPLER_INT(minor, id) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_INT, 0, 0, id, 0)

#define SCSC_HIP4_SAMPLER_INT_OUT(minor, id) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_INT_OUT, 0, 0, id, 0)

#define SCSC_HIP4_SAMPLER_INT_BH(minor, id) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_INT_BH, 0, 0, id, 0)

#define SCSC_HIP4_SAMPLER_INT_OUT_BH(minor, id) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_INT_OUT_BH, 0, 0, id, 0)

#define SCSC_HIP4_SAMPLER_RESET(minor) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_RESET, 0, 0, 0, 0)

#define SCSC_HIP4_SAMPLER_VIF_PEER(minor, tx, vif, peer_index) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_PEER, tx, vif, peer_index, 0)

#define SCSC_HIP4_SAMPLER_BOT_RX(minor, vif, peer_index, pri, smod_and_scod) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_RX, vif, peer_index, pri, smod_and_scod)

#define SCSC_HIP4_SAMPLER_BOT_TX(minor, vif, peer_index, pri, smod_and_scod) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_TX, vif, peer_index, pri, smod_and_scod)

#define SCSC_HIP4_SAMPLER_BOT_ADD(minor, vif, peer_index, addr_0_to_3) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_ADD, vif, peer_index, 0, addr_0_to_3)

#define SCSC_HIP4_SAMPLER_BOT_REMOVE(minor, vif, peer_index) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_REMOVE, vif, peer_index, 0, 0)

#define SCSC_HIP4_SAMPLER_BOT_START_Q(minor, vif, peer_index) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_START_Q, vif, peer_index, 0, 0)

#define SCSC_HIP4_SAMPLER_BOT_STOP_Q(minor, vif, peer_index) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_STOP_Q, vif, peer_index, 0, 0)

#define SCSC_HIP4_SAMPLER_BOT_QMOD_RX(minor, vif, peer_index, pri, qmod_and_qcod) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_QMOD_RX, vif, peer_index, pri, qmod_and_qcod)

#define SCSC_HIP4_SAMPLER_BOT_QMOD_TX(minor, vif, peer_index, pri, qmod_and_qcod) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_QMOD_TX, vif, peer_index, pri, qmod_and_qcod)

#define SCSC_HIP4_SAMPLER_BOT_QMOD_START(minor, vif, peer_index, priority) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_QMOD_START, vif, peer_index, priority, 0)

#define SCSC_HIP4_SAMPLER_BOT_QMOD_STOP(minor, vif, peer_index, priority) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_BOT_QMOD_STOP, vif, peer_index, priority, 0)

#define SCSC_HIP4_SAMPLER_PKT_TX(minor, host_tag) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_PKT_TX, 0, (host_tag >> 8) & 0xff, host_tag & 0xff, 0)

#define SCSC_HIP4_SAMPLER_PKT_TX_HIP4(minor, host_tag) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_PKT_TX_HIP4, 0, (host_tag >> 8) & 0xff, host_tag & 0xff, 0)

#define SCSC_HIP4_SAMPLER_PKT_TX_FB(minor, host_tag) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_PKT_TX_FB, 0, (host_tag >> 8) & 0xff, host_tag & 0xff, 0)

#define SCSC_HIP4_SAMPLER_SUSPEND(minor) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_SUSPEND, 0, 0, 0, 0)

#define SCSC_HIP4_SAMPLER_RESUME(minor) \
	hip4_sampler_update_record(minor, HIP4_SAMPLER_RESUME, 0, 0, 0, 0)
#else
#define SCSC_HIP4_SAMPLER_Q(minor, q, idx_rw, value, rw)
#define SCSC_HIP4_SAMPLER_QREF(minor, ref, q)
#define SCSC_HIP4_SAMPLER_SIGNAL_CTRLTX(minor, bytes16_h, bytes16_l)
#define SCSC_HIP4_SAMPLER_SIGNAL_CTRLRX(minor, bytes16_h, bytes16_l)
#define SCSC_HIP4_SAMPLER_TPUT(minor, rx_tx, payload)
#define SCSC_HIP4_SAMPLER_THROUG(minor, bytes16_h, bytes16_l)
#define SCSC_HIP4_SAMPLER_THROUG_K(minor, bytes16_h, bytes16_l)
#define SCSC_HIP4_SAMPLER_THROUG_M(minor, bytes16_h, bytes16_l)
#define SCSC_HIP4_SAMPLER_MBULK(minor, bytes16_h, bytes16_l, clas, tot_num)
#define SCSC_HIP4_SAMPLER_QFULL(minor, q)
#define SCSC_HIP4_SAMPLER_MFULL(minor)
#define SCSC_HIP4_SAMPLER_INT(minor, id)
#define SCSC_HIP4_SAMPLER_INT_BH(minor, id)
#define SCSC_HIP4_SAMPLER_INT_OUT(minor, id)
#define SCSC_HIP4_SAMPLER_INT_OUT_BH(minor, id)
#define SCSC_HIP4_SAMPLER_RESET(minor)
#define SCSC_HIP4_SAMPLER_VIF_PEER(minor, tx, vif, peer_index)
#define SCSC_HIP4_SAMPLER_BOT_RX(minor, vif, peer_index, pri, smod_and_scod)
#define SCSC_HIP4_SAMPLER_BOT_TX(minor, vif, peer_index, pri, smod_and_scod)
#define SCSC_HIP4_SAMPLER_BOT_ADD(minor, vif, peer_index, addr_0_to_3)
#define SCSC_HIP4_SAMPLER_BOT_REMOVE(minor, vif, peer_index)
#define SCSC_HIP4_SAMPLER_BOT_START_Q(minor, vif, peer_index)
#define SCSC_HIP4_SAMPLER_BOT_STOP_Q(minor, vif, peer_index)
#define SCSC_HIP4_SAMPLER_BOT_QMOD_RX(minor, vif, peer_index, pri, qmod_and_qcod)
#define SCSC_HIP4_SAMPLER_BOT_QMOD_TX(minor, vif, peer_index, pri, qmod_and_qcod)
#define SCSC_HIP4_SAMPLER_BOT_QMOD_START(minor, vif, peer_index, priority)
#define SCSC_HIP4_SAMPLER_BOT_QMOD_STOP(minor, vif, peer_index, priority)
#define SCSC_HIP4_SAMPLER_PKT_TX(minor, host_tag)
#define SCSC_HIP4_SAMPLER_PKT_TX_HIP4(minor, host_tag)
#define SCSC_HIP4_SAMPLER_PKT_TX_FB(minor, host_tag)
#define SCSC_HIP4_SAMPLER_SUSPEND(minor)
#define SCSC_HIP4_SAMPLER_RESUME(minor)
#endif /* CONFIG_SCSC_WLAN_HIP4_PROFILING */

/* HIP4 sample headers */
#ifdef CONFIG_SCSC_WLAN_HIP5
#define SCSC_HIP4_SAMPLER_HEADER_VERSION_MAJOR	0x00
#define SCSC_HIP4_SAMPLER_HEADER_VERSION_MINOR	0x02
#else
#define SCSC_HIP4_SAMPLER_HEADER_VERSION_MAJOR	0x01
#define SCSC_HIP4_SAMPLER_HEADER_VERSION_MINOR	0x02
#endif

#define SCSC_HIP4_SAMPLER_RESERVED		(1)
#define SCSC_HIP4_SAMPLER_RESERVED_2		(10)

#ifdef CONFIG_SCSC_WLAN_HIP5
#define SCSC_HIP4_SAMPLER_MAGIC			"HIP5"
#else
#define SCSC_HIP4_SAMPLER_MAGIC			"HIP4"
#endif

enum scsc_hip4_sampler_type {
	SCSC_HIP4_SAMPLER_TYPE_PRE_TCP = 0,
	SCSC_HIP4_SAMPLER_TYPE_TCP,
	/* Add others */
};

enum scsc_hip4_sampler_platform {
	SCSC_HIP4_SAMPLER_EXYNOS9610 = 1,
	SCSC_HIP4_SAMPLER_EXYNOS9630,
	SCSC_HIP4_SAMPLER_EXYNOS7885,
	SCSC_HIP4_SAMPLER_EXYNOS9815,
	SCSC_HIP4_SAMPLER_EXYNOS8825,
	SCSC_HIP4_SAMPLER_EXYNOS9925,
	SCSC_HIP4_SAMPLER_EXYNOS8535,
	SCSC_HIP4_SAMPLER_EXYNOS8835,
	SCSC_HIP4_SAMPLER_EXYNOS8845,
	/* Add others */
	SCSC_HIP4_SAMPLER_UNDEF = 0xffff
};

/* HIP4 sampler HEADER v 0.0*/
struct scsc_hip4_sampler_header {
	char magic[4]; /* HIP4 */
	u16  offset_data;
	u8   version_major; /* Major version */
	u8   version_minor; /* Minor version */
	u16  platform;    /* Enumeration containing platform information */
	u8   sample_type; /* Enumerate type of HIP4 sample captured */
	u8   reserved[SCSC_HIP4_SAMPLER_RESERVED];
	u16  hip4_status; /* hip4 sampling settings */
	u64  num_samples; /* For file validation in post-processing */
	u8   reserved_2[SCSC_HIP4_SAMPLER_RESERVED_2];
} __packed;

#endif /* __HIP4_SAMPLER_H__ */
