/******************************************************************************
 *                                                                            *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd. All rights reserved       *
 *                                                                            *
 * Bluetooth HCI Packet                                                       *
 *                                                                            *
 ******************************************************************************/
#ifndef __HCI_PKT_H__
#define __HCI_PKT_H__
#include <linux/skbuff.h>

/* HCI data types */
#ifndef HCI_COMMAND_PKT
#define HCI_COMMAND_PKT		0x01
#define HCI_ACLDATA_PKT		0x02
#define HCI_SCODATA_PKT		0x03
#define HCI_EVENT_PKT		0x04
#define HCI_ISODATA_PKT		0x05
#endif
#define HCI_UNKNOWN_PKT         0x00
#define HCI_PROPERTY_PKT        0x0E

/* HCI Packet property */
enum {
	HCI_PROPERTY_HW_ERROR = 0xF,
};

/* using HCI Events */
enum {
	HCI_EVENT_HARDWARE_ERROR_EVENT  = 0x10,

	HCI_EVENT_COMMAND_COMPLETE      = 0x0E,

	HCI_EVENT_VENDOR_SPECIFIC_EVENT = 0xFF,
};

/* Sub code for vendor specific event of HCI */
enum prop_hci_vse_sub_code {
	HCI_VSE_SYSTEM_ERROR_INFO_SUB_CODE = 0x65,
};

/* Vendor specific commands of HCI */
enum {
	HCI_VSC_FWLOG_BTSNOOP = 0xFDF6,
};

/* HCI packet status */
enum {
	HCI_PKT_STATUS_COMPLETE,
	HCI_PKT_STATUS_NO_DATA,
	HCI_PKT_STATUS_UNKNOWN_TYPE,
	HCI_PKT_STATUS_NOT_ENOUGH_HEADER,
	HCI_PKT_STATUS_NOT_ENOUGH_LENGTH,
	HCI_PKT_STATUS_OVERRUN_LENGTH,
};

/* HCI Context Block for socket buffer */
struct slsi_hci_cb {
	char type;
	char trans_type;
	char property;
};

#define GET_HCI_CB(skb)		        ((struct slsi_hci_cb *)(skb->cb))

#define GET_HCI_PKT_TYPE(skb)           (GET_HCI_CB(skb)->type)
#define SET_HCI_PKT_TYPE(skb, _TYPE)    (GET_HCI_CB(skb)->type = _TYPE)

#define GET_HCI_PKT_TR_TYPE(skb)        (GET_HCI_CB(skb)->trans_type)
#define SET_HCI_PKT_TR_TYPE(skb, _TYPE) (GET_HCI_CB(skb)->trans_type = _TYPE)

#define GET_HCI_PKT_PROPERTY(skb)       (GET_HCI_CB(skb)->property)
#define SET_HCI_PKT_PROPERTY(skb, PROP) (GET_HCI_CB(skb)->property = PROP)

#define SET_HCI_PKT_HW_ERROR(skb)        do {\
	SET_HCI_PKT_TYPE(skb, HCI_PROPERTY_PKT);\
	SET_HCI_PKT_PROPERTY(skb, HCI_PROPERTY_HW_ERROR); } while(0)

#define TEST_HCI_PKT_HW_ERROR(skb)      \
	(GET_HCI_PKT_TYPE(skb) == HCI_PROPERTY_PKT && \
	 GET_HCI_PKT_PROPERTY(skb) == HCI_PROPERTY_HW_ERROR)

#define HCI_PKT_HEAD_ROOM_SIZE          (1+4)	/* slip start + bcsp header */
#define HCI_PKT_TAIL_ROOM_SIZE          (1+2)	/* slip end + bcsp tail */

#define HCI_PKT_MIN_BUF_SIZE		(2048)

static inline struct sk_buff *__alloc_hci_pkt_skb(unsigned int size,
						  unsigned int hdr_size)
{
	struct sk_buff *skb;

	size += HCI_PKT_HEAD_ROOM_SIZE + HCI_PKT_TAIL_ROOM_SIZE;

	/* It prevents frequent expansion */
	size = size > HCI_PKT_MIN_BUF_SIZE ? size : HCI_PKT_MIN_BUF_SIZE;

	skb = alloc_skb(size, GFP_ATOMIC);
	if (skb && hdr_size < HCI_PKT_HEAD_ROOM_SIZE) {
		skb->data += HCI_PKT_HEAD_ROOM_SIZE - hdr_size;
		skb->tail += HCI_PKT_HEAD_ROOM_SIZE - hdr_size;
	}
	return skb;
}

static inline struct sk_buff *alloc_hci_pkt_skb(unsigned int size)
{
	return __alloc_hci_pkt_skb(size, 0);
}

int hci_pkt_status(char type, char *data, size_t len);
size_t hci_pkt_get_size(char type, char *data, size_t len);
unsigned short hci_pkt_get_command(char* data, size_t len);

#endif /* __HCI_PKT_H__ */
