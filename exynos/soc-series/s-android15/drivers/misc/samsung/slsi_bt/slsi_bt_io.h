/******************************************************************************
 *                                                                            *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd. All rights reserved       *
 *                                                                            *
 * Bluetooth Uart Driver                                                      *
 *                                                                            *
 ******************************************************************************/
#ifndef __SLSI_BT_H_
#define __SLSI_BT_H_

/* Driver creation information */
#ifndef CONFIG_SCSC_BT
#define SLSI_BT_DEV_NAME                "scsc_h4_0"
#else
#define SLSI_BT_DEV_NAME                "scsc_h4_1"
#endif

#define SLSI_BT_MINORS			(8)
#define SLSI_BT_MINOR_CTRL              (0)

/* IOCTL */
#define SBTIOCT_CHANGE_TRS              _IOW('S', 1, unsigned int)

#define SLSI_BT_TR_EN_H4                (1 << 1)
#define SLSI_BT_TR_EN_PROP              (1 << 2)
#define SLSI_BT_TR_EN_BCSP              (1 << 3)
#define SLSI_BT_TR_EN_HCI_UART          (1 << 5)
#define SLSI_BT_TR_EN_TTY               (1 << 6)
#define SLSI_BT_TR_EN_TLBP              (1 << 7)

struct hci_trans;
int slsi_bt_open(unsigned int trs, struct hci_trans **top_htr);
int slsi_bt_release(void);

#endif /* __SLSI_BT_H_ */
