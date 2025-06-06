/******************************************************************************
 *                                                                            *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd. All rights reserved       *
 *                                                                            *
 * Bluetooth HCI 3-Wired Uart Transport Layer & BCSP                          *
 *                                                                            *
 ******************************************************************************/
#ifndef __HCI_BCSP_H__
#define __HCI_BCSP_H__

#include "hci_trans.h"
#include "slsi_bt_tty.h"
#include "hci_pkt.h"

#define HCI_BCSP_TYPE_ACK                 (HCI_UNKNOWN_PKT)
#define HCI_BCSP_TYPE_CMD                 (HCI_COMMAND_PKT)
#define HCI_BCSP_TYPE_ACL                 (HCI_ACLDATA_PKT)
#define HCI_BCSP_TYPE_SCO                 (HCI_SCODATA_PKT)
#define HCI_BCSP_TYPE_EVT                 (HCI_EVENT_PKT)
#define HCI_BCSP_TYPE_ISO                 (HCI_ISODATA_PKT)
#define HCI_BCSP_TYPE_UNKNOWN             (13)
#define HCI_BCSP_TYPE_VENDOR              (HCI_PROPERTY_PKT)
#define HCI_BCSP_TYPE_LINK_CONTROL        (15)

#define HCI_BCSP_MAX_PAYLOAD        (4096)
#define HCI_BCSP_MAX_OCT            (HCI_BCSP_MAX_PAYLOAD + 8) // hdr + crc + 2*c0
#define HCI_BCSP_T_MAX              ((HCI_BCSP_MAX_OCT*8) / SLSI_BT_TTY_BAUD)

/* Host supported configuration     */
#define HCI_BCSP_WINDOWSIZE         7  // MAX
#define HCI_BCSP_SW_FLOW_CTR        0
#define HCI_BCSP_DATA_INT_TYPE      1
#define HCI_BCSP_VERSION            0  // 1.0

enum {
	HCI_BCSP_MSG_TYPE_SIG = 0,	/* default */
	HCI_BCSP_MSG_TYPE_BCSP,
};

#define HCI_BCSP_RESEND_LIMIT       20
#define HCI_BCSP_RESEND_TIMEOUT     (jiffies + HZ/4)
#define HCI_BCSP_RESEND_WAKEUP_MS   (jiffies + msecs_to_jiffies(30))

#define HCI_BCSP_TRACE_SLIP         1
#define HCI_BCSP_TRACE_BCSP         2

int hci_bcsp_init(struct hci_trans *htr);
void hci_bcsp_deinit(struct hci_trans *htr);

#endif /* __HCI_BCSP_H__ */
