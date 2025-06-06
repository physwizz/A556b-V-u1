/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NPU_IF_PROTODRV_MBOX2_H_
#define _NPU_IF_PROTODRV_MBOX2_H_

#include <linux/types.h>
#include <linux/mutex.h>

/* Type definition of signal function of protodrv */
typedef void (*protodrv_notifier)(void *);

#include "include/npu-config.h"
#include "include/npu-common.h"
#include "npu-util-msgidgen.h"
#include "npu-protodrv.h"
#include "interface/hardware/mailbox_ipc.h"
#include "interface/hardware/mailbox_msg.h"

struct npu_if_protodrv_mbox_ops {
	int (*frame_result_available)(enum channel_flag c);
	int (*frame_post_request)(int msgid, struct npu_frame *frame);
	int (*frame_get_result)(int *ret_msgid, struct npu_frame *frame, enum channel_flag c);
	int (*nw_result_available)(void);
	int (*nw_post_request)(int msgid, struct npu_nw *nw);
	int (*nw_get_result)(int *ret_msgid, struct npu_nw *nw);
#if IS_ENABLED(CONFIG_NPU_USE_IMB_ALLOCATOR_V2)
	int (*fwmsg_available)(void);
	int (*fw_msg_get)(struct fw_message *fw_msg);
	int (*fw_res_put)(struct fw_message *fw_msg);
#endif
	int (*register_notifier)(protodrv_notifier);
	int (*register_msgid_type_getter)(int (*)(int));
};

struct npu_if_protodrv_mbox {
	struct device *dev;
	const struct npu_if_protodrv_mbox_ops *npu_if_protodrv_mbox_ops;
};

/* Exported functions */
int npu_mbox_op_register_notifier(protodrv_notifier sig_func);
int npu_mbox_op_register_msgid_type_getter(int (*msgid_get_type_func)(int));
int npu_nw_mbox_op_is_available(void);
int npu_nw_mbox_ops_get(struct msgid_pool *pool, struct proto_req_nw **target);
int npu_nw_mbox_ops_put(struct msgid_pool *pool, struct proto_req_nw *src);
#if IS_ENABLED(CONFIG_NPU_USE_IMB_ALLOCATOR_V2)
int npu_fw_res_put(struct fw_message *fw_msg);
int npu_fw_message_get(struct fw_message *fw_msg);
int npu_fwmsg_mbox_op_is_available(void);
#endif
int npu_frame_mbox_op_is_available(void);
int npu_frame_mbox_ops_get(struct msgid_pool *pool, struct proto_req_frame **target);
int npu_frame_mbox_ops_put(struct msgid_pool *pool, struct proto_req_frame *src);
int npu_kpi_frame_mbox_put(struct msgid_pool *pool, struct npu_frame *frame);


extern const struct npu_if_protodrv_mbox_ops protodrv_mbox_ops;

#endif	/* _NPU_IF_PROTODRV_MBOX_H_ */
