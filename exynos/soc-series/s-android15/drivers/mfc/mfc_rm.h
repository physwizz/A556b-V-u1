/*
 * drivers/media/platform/exynos/mfc/mfc_rm.h
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __MFC_RM_H
#define __MFC_RM_H __FILE__

#include "mfc_core_sync.h"

#include "base/mfc_common.h"

#define MFC_RM_LOAD_DELETE		0
#define MFC_RM_LOAD_ADD			1
#define MFC_RM_LOAD_DELETE_UPDATE	2

static inline struct mfc_core *mfc_get_main_core_lock(struct mfc_dev *dev,
			struct mfc_ctx *ctx)
{
	struct mfc_core *core;

	mfc_get_corelock_ctx(ctx);

	if (ctx->op_core_num[MFC_CORE_MAIN] == MFC_CORE_INVALID)
		core = NULL;
	else
		core = dev->core[ctx->op_core_num[MFC_CORE_MAIN]];

	mfc_release_corelock_ctx(ctx);

	return core;
}

static inline struct mfc_core *mfc_get_sub_core_lock(struct mfc_dev *dev,
			struct mfc_ctx *ctx)
{
	struct mfc_core *core;

	mfc_get_corelock_ctx(ctx);

	if (ctx->op_core_num[MFC_CORE_SUB] == MFC_CORE_INVALID)
		core = NULL;
	else
		core = dev->core[ctx->op_core_num[MFC_CORE_SUB]];

	mfc_release_corelock_ctx(ctx);

	return core;
}

static inline struct mfc_core *mfc_get_main_core(struct mfc_dev *dev,
			struct mfc_ctx *ctx)
{
	if (ctx->op_core_num[MFC_CORE_MAIN] == MFC_CORE_INVALID)
		return NULL;

	return dev->core[ctx->op_core_num[MFC_CORE_MAIN]];
}

static inline struct mfc_core *mfc_get_sub_core(struct mfc_dev *dev,
			struct mfc_ctx *ctx)
{
	if (ctx->op_core_num[MFC_CORE_SUB] == MFC_CORE_INVALID)
		return NULL;

	return dev->core[ctx->op_core_num[MFC_CORE_SUB]];
}

static inline void mfc_rm_set_core_num(struct mfc_ctx *ctx, int main_core_num)
{
	ctx->op_core_num[MFC_CORE_MAIN] = main_core_num;

	if (main_core_num == MFC_DEC_DEFAULT_CORE)
		ctx->op_core_num[MFC_CORE_SUB] = MFC_SURPLUS_CORE;
	else
		ctx->op_core_num[MFC_CORE_SUB] = MFC_DEC_DEFAULT_CORE;

	mfc_ctx_debug(2, "[RM] main core %d, sub core %d\n",
			ctx->op_core_num[MFC_CORE_MAIN],
			ctx->op_core_num[MFC_CORE_SUB]);
}

/* load balancing */
void mfc_rm_migration_worker(struct work_struct *work);
void mfc_rm_load_balancing(struct mfc_ctx *ctx, int load_add);

/* core ops */
int mfc_rm_instance_init(struct mfc_dev *dev, struct mfc_ctx *ctx);
int mfc_rm_instance_deinit(struct mfc_dev *dev, struct mfc_ctx *ctx);
int mfc_rm_instance_open(struct mfc_dev *dev, struct mfc_ctx *ctx);
void mfc_rm_instance_dec_stop(struct mfc_dev *dev, struct mfc_ctx *ctx,
			unsigned int type);
void mfc_rm_instance_enc_stop(struct mfc_dev *dev, struct mfc_ctx *ctx,
			unsigned int type);
int mfc_rm_subcore_seq_start(struct mfc_dev *dev, struct mfc_ctx *ctx);
int mfc_rm_instance_setup(struct mfc_dev *dev, struct mfc_ctx *ctx);
void mfc_rm_request_work(struct mfc_dev *dev, enum mfc_request_work work,
		struct mfc_ctx *ctx);

/* utils */
void mfc_rm_qos_control(struct mfc_ctx *ctx, enum mfc_qos_control qos_control);
int mfc_rm_query_state(struct mfc_ctx *ctx, enum mfc_inst_state_query query,
			enum mfc_inst_state state);

void mfc_rm_update_real_time(struct mfc_ctx *ctx);
#endif /* __MFC_RM_H */
