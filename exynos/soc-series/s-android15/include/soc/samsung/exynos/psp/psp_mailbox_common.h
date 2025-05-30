/*
 * psp_mailbox_common.h - Samsung Psp Mailbox driver for the Exynos
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __PSP_MAILBOX_COMMON_H__
#define __PSP_MAILBOX_COMMON_H__

#include "psp_common.h"
#include "psp_error.h"

/*****************************************************************************/
/* Definition								     */
/*****************************************************************************/
#define STMB_WAIT_MAX_CNT			(0x100000)
#define STMB_CMD_BIT_OFFSET			(16)
#define STMB_LEN_BIT_OFFSET			(24)

/*****************************************************************************/
/* Function prototype							     */
/*****************************************************************************/
void psp_sync_mutex(uint32_t lock);
uint32_t psp_get_power_count(void);

/**
 @brief Function for PSP to send a message througth mailbox
 @param message : Address for message buffer to send
 @param message_len : Length of the message in byte
 @param passed_message_len : Length of the passed message in byte
 @return 0: success, otherwise: error code
 */
uint32_t mailbox_send(uint8_t *message, uint32_t message_len, uint32_t *passed_message_len);

/**
 @brief Function PSP to get a (portion of long) message from mailbox
 @param message : Address for message buffer to receive
 @param message_len : Address to get the received message length of the message buffer (in byte)
 @param total_len : Address to get total message length to be received ultimately (in byte)
 @param remain_len : Address to get remaining message length be be recevied subsequently (in byte)
 @return 0: success, otherwise: error code
 @note : If sender (ex. REE) sends a long "total_len" message, this function can be called multiple times in the interrupt handler.
	 when this function gets:
	 - the first portion of the long message, the total_len is message_len plus remain_len.
	 - the final portion of the long message, the remain_len is 0.
	 - a short message, the total_len can be equal to message_len and the remain_len can be 0.
 */
uint32_t mailbox_receive(uint8_t *message, uint32_t *message_len, uint32_t *total_len, uint32_t *remain_len);
uint32_t move_mailbox_to_buf(uint8_t *data, uint64_t mbox_addr, uint32_t data_len);
void psp_print_debug_sfr(void);

#endif /* __PSP_MAILBOX_COMMON_H__ */
