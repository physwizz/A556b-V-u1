/*
 * drivers/media/platform/exynos/mfc/mfc_core_run.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "mfc_core_run.h"

#include "mfc_core_pm.h"
#include "mfc_core_otf.h"
#include "mfc_core_sync.h"

#include "mfc_core_cmd.h"
#include "mfc_core_hw_reg_api.h"
#include "mfc_core_enc_param.h"

#include "base/mfc_queue.h"
#include "base/mfc_utils.h"
#include "base/mfc_mem.h"
#include "base/mfc_qos.h"

void mfc_core_run_cache_flush(struct mfc_core *core, int is_drm,
		enum mfc_do_cache_flush do_cache_flush, int drm_switch, int reg_clear)
{
	enum mfc_fw_status fw_status;
	int pending_check = 1;

	if (core->state == MFCCORE_ERROR) {
		mfc_core_info("[MSR] Couldn't lock HW. It's Error state\n");
		return;
	}

	/*
	 * Even if it is determined that the attribute of the previous instance
	 * and the current instance have been changed, (= drm_switch)
	 * there is no need to cache flush if the F/W of the previous instance is unloaded.
	 */
	if (drm_switch) {
		if (is_drm)
			fw_status = core->fw.status;
		else
			fw_status = core->fw.drm_status;

		if (!(fw_status & MFC_FW_LOADED)) {
			mfc_core_debug(2, "F/W has already un-loaded\n");
			do_cache_flush = MFC_NO_CACHEFLUSH;
			pending_check = 0;
		}
	}

	if (do_cache_flush == MFC_CACHEFLUSH) {
		mfc_core_cmd_cache_flush(core);
		if (mfc_wait_for_done_core(core, MFC_REG_R2H_CMD_CACHE_FLUSH_RET)) {
			mfc_core_err("Failed to CACHE_FLUSH\n");
			core->logging_data->cause |= (1 << MFC_CAUSE_FAIL_CACHE_FLUSH);
			call_dop(core, dump_and_stop_always, core);
		}
	} else if (do_cache_flush == MFC_NO_CACHEFLUSH) {
		mfc_core_debug(2, "F/W has already done cache flush with prediction\n");
	}

	/* When init_hw(), reg_clear is required between cache flush and (un)protection */
	if (reg_clear) {
		mfc_core_reg_clear(core);
		pending_check = 0;
		mfc_core_debug(2, "Done register clear\n");
	}

	mfc_core_change_attribute(core, is_drm);

	/* drm_switch may not occur when cache flush is required during migration. */
	if (!drm_switch)
		return;

	if (is_drm) {
		MFC_TRACE_CORE("Normal -> DRM\n");
		mfc_core_debug(2, "Normal -> DRM need protection\n");
		mfc_core_protection_on(core, pending_check);
	} else {
		MFC_TRACE_CORE("DRM -> Normal\n");
		mfc_core_debug(2, "DRM -> Normal need un-protection\n");
		mfc_core_protection_off(core, pending_check);
	}
}

/* Initialize hardware */
int mfc_core_run_init_hw(struct mfc_core *core, int is_drm)
{
	enum mfc_buf_usage_type buf_type;
	enum mfc_do_cache_flush do_cache_flush;
	int fw_ver, drm_switch;
	int ret = 0;

	if (is_drm)
		buf_type = MFCBUF_DRM;
	else
		buf_type = MFCBUF_NORMAL;

	mfc_core_debug(2, "%s F/W initialize start\n", is_drm ? "secure" : "normal");

	/* 0. MFC reset */
	ret = mfc_core_pm_clock_on(core, 0);
	if (ret) {
		mfc_core_err("Failed to enable clock before reset(%d)\n", ret);
		return ret;
	}

	mfc_core_pm_clock_get(core);

	/* cache flush for previous FW */
	if (core->curr_core_ctx_is_drm != is_drm) {
		do_cache_flush = MFC_CACHEFLUSH;
		drm_switch = 1;
	} else {
		do_cache_flush = MFC_NO_CACHEFLUSH;
		drm_switch = 0;
	}
	mfc_core_run_cache_flush(core, is_drm, do_cache_flush, drm_switch, 1);

	mfc_core_reset_mfc(core, buf_type);
	mfc_core_debug(2, "Done MFC reset\n");

	/* 1. Set DRAM base Addr */
	mfc_core_set_risc_base_addr(core, buf_type);

	/* 2. Release reset signal to the RISC */
	if (!(core->dev->pdata->security_ctrl && is_drm)) {
		mfc_core_risc_on(core);

		mfc_core_debug(2, "Will now wait for completion of firmware transfer\n");
		if (mfc_wait_for_done_core(core, MFC_REG_R2H_CMD_FW_STATUS_RET)) {
			mfc_core_err("Failed to RISC_ON\n");
			mfc_core_clean_dev_int_flags(core);
			ret = -EIO;
			goto err_init_hw;
		}
	}

	/* 3. Initialize firmware */
	mfc_core_cmd_sys_init(core, buf_type);

	mfc_core_debug(2, "Ok, now will write a command to init the system\n");
	if (mfc_wait_for_done_core(core, MFC_REG_R2H_CMD_SYS_INIT_RET)) {
		mfc_core_err("Failed to SYS_INIT\n");
		mfc_core_clean_dev_int_flags(core);
		ret = -EIO;
		goto err_init_hw;
	}

	core->int_condition = 0;
	if (core->int_err != 0 || core->int_reason != MFC_REG_R2H_CMD_SYS_INIT_RET) {
		/* Failure. */
		mfc_core_err("Failed to init firmware - error: %d, int: %d\n",
				core->int_err, core->int_reason);
		ret = -EIO;
		goto err_init_hw;
	}

	core->fw.fimv_info = mfc_core_get_fimv_info();
	if (core->fw.fimv_info != 'D' && core->fw.fimv_info != 'E')
		core->fw.fimv_info = 'N';

	mfc_core_info("[F/W] MFC %s v%x, %02xyy %02xmm %02xdd (%c)\n",
			is_drm ? "secure" : "normal",
			core->core_pdata->ip_ver,
			mfc_core_get_fw_ver_year(),
			mfc_core_get_fw_ver_month(),
			mfc_core_get_fw_ver_date(),
			core->fw.fimv_info);

	core->fw.date = mfc_core_get_fw_ver_all();
	/* Check MFC version and F/W version */
	fw_ver = mfc_core_get_mfc_version();
	if ((fw_ver & MFC_REG_MFC_VER_MAJOR_MASK) != core->core_pdata->ip_ver) {
		mfc_core_err("Invalid F/W version(0x%x) for MFC H/W(0x%x)\n",
				fw_ver, core->core_pdata->ip_ver);
		ret = -EIO;
		goto err_init_hw;
	}
	core->dev->fw_changed_info = (fw_ver & MFC_REG_MFC_VER_MINOR_MASK);

	if (is_drm)
		mfc_core_change_fw_state(core, 1, MFC_FW_INITIALIZED, 1);
	else
		mfc_core_change_fw_state(core, 0, MFC_FW_INITIALIZED, 1);

err_init_hw:
	mfc_core_pm_clock_off(core, 0);
	mfc_core_debug_leave();

	return ret;
}

/* Deinitialize hardware */
void mfc_core_run_deinit_hw(struct mfc_core *core)
{
	int ret;

	mfc_core_debug(2, "mfc deinit start\n");

	ret = mfc_core_pm_clock_on(core, 0);
	if (ret) {
		mfc_core_err("Failed to enable clock before reset(%d)\n", ret);
		return;
	}

	mfc_core_mfc_off(core);

	mfc_core_pm_clock_off(core, 0);

	mfc_core_debug(2, "mfc deinit completed\n");
}

void mfc_core_run_hwapg_reset(struct mfc_core *core)
{
	unsigned long flags;
	u64 hwapg_time = 0;
	ktime_t hwapg_start = 0;
	ktime_t hwapg_end = 0;

	if (core->hwapg_status == MFC_HWAPG_CLEAR)
		return;

	mfc_core_debug(2, "[HWAPG] Start MFC reset\n");

	if (core->dev->debugfs.batch_qos_enable)
		mfc_qos_on(core, core->dev->ctx[core->batch_index]);

	hwapg_start = ktime_get();

	spin_lock_irqsave(&core->batch_lock, flags);
	mfc_core_print_hwapg_status(core);
	mfc_core_reset_mfc(core, MFCBUF_NORMAL);
	mfc_core_disable_hwapg(core);
	mfc_core_set_risc_base_addr(core, MFCBUF_NORMAL);
	mfc_core_set_risc_status(core);
	mfc_core_risc_on(core);
	spin_unlock_irqrestore(&core->batch_lock, flags);

	mfc_core_debug(2, "[HWAPG] Will now wait for completion of firmware transfer\n");
	if (mfc_wait_for_done_core(core, MFC_REG_R2H_CMD_FW_STATUS_RET)) {
		mfc_core_err("Failed to RISC_ON\n");
		mfc_core_clean_dev_int_flags(core);
		call_dop(core, dump_and_stop_always, core);
	}

	hwapg_end = ktime_get();
	hwapg_time = ktime_to_us(hwapg_end) - ktime_to_us(hwapg_start);
	mfc_core_debug(2, "[HWAPG] reset time : %llu(us)\n", hwapg_time);
	mfc_core_print_hwapg_status(core);

	core->hwapg_status = MFC_HWAPG_CLEAR;

	return;
}

int mfc_core_run_sleep(struct mfc_core *core)
{
	struct mfc_core_ctx *core_ctx;
	int i;
	int drm_switch = 0;

	mfc_core_debug_enter();

	core_ctx = core->core_ctx[core->curr_core_ctx];
	if (!core_ctx) {
		for (i = 0; i < MFC_NUM_CONTEXTS; i++) {
			if (core->core_ctx[i]) {
				core_ctx = core->core_ctx[i];
				break;
			}
		}

		if (!core_ctx) {
			mfc_core_err("no mfc context to run\n");
			return -EINVAL;
		}
		mfc_info("ctx is changed %d -> %d\n",
				core->curr_core_ctx, core_ctx->num);

		core->curr_core_ctx = core_ctx->num;
		if (core->curr_core_ctx_is_drm != core_ctx->is_drm) {
			drm_switch = 1;
			mfc_info("DRM attribute is changed %d->%d\n",
					core->curr_core_ctx_is_drm, core_ctx->is_drm);
		}
	}
	mfc_info("curr_core_ctx_is_drm:%d\n", core->curr_core_ctx_is_drm);

	mfc_core_pm_clock_on(core, 0);

	if (drm_switch)
		mfc_core_run_cache_flush(core, core_ctx->is_drm, MFC_CACHEFLUSH, drm_switch, 0);

	mfc_core_cmd_sleep(core);

	if (mfc_wait_for_done_core(core, MFC_REG_R2H_CMD_SLEEP_RET)) {
		mfc_err("Failed to SLEEP\n");
		core->logging_data->cause |= (1 << MFC_CAUSE_FAIL_SLEEP);
		call_dop(core, dump_and_stop_always, core);
		return -EBUSY;
	}

	core->int_condition = 0;
	if (core->int_err != 0 || core->int_reason != MFC_REG_R2H_CMD_SLEEP_RET) {
		/* Failure. */
		mfc_err("Failed to sleep - error: %d, int: %d\n",
				core->int_err, core->int_reason);
		call_dop(core, dump_and_stop_debug_mode, core);
		return -EBUSY;
	}

	core->sleep = 1;

	mfc_core_mfc_always_off(core);
	mfc_core_pm_clock_off(core, 0);

	if (core->curr_core_ctx_is_drm)
		mfc_core_protection_off(core, 0);

	mfc_core_debug_leave();

	return 0;
}

int mfc_core_run_wakeup(struct mfc_core *core)
{
	enum mfc_buf_usage_type buf_type;
	int ret = 0;

	mfc_core_debug_enter();

	mfc_core_info("curr_core_ctx_is_drm:%d\n", core->curr_core_ctx_is_drm);
	if (core->curr_core_ctx_is_drm)
		buf_type = MFCBUF_DRM;
	else
		buf_type = MFCBUF_NORMAL;

	/* 0. MFC reset */
	ret = mfc_core_pm_clock_on(core, 0);
	if (ret) {
		mfc_core_err("Failed to enable clock before reset(%d)\n", ret);
		return ret;
	}
	mfc_core_reg_clear(core);
	mfc_core_debug(2, "Done register clear\n");

	/*
	 * When protection_on by init or wakeup sequence,
	 * do not need to pending check because the register already cleared.
	 */
	if (core->curr_core_ctx_is_drm)
		mfc_core_protection_on(core, 0);

	mfc_core_reset_mfc(core, buf_type);
	mfc_core_debug(2, "Done MFC reset\n");

	/* 1. Set DRAM base Addr */
	mfc_core_set_risc_base_addr(core, buf_type);

	/* 2. Release reset signal to the RISC */
	if (!(core->dev->pdata->security_ctrl && (buf_type == MFCBUF_DRM))) {
		mfc_core_risc_on(core);

		mfc_core_debug(2, "Will now wait for completion of firmware transfer\n");
		if (mfc_wait_for_done_core(core, MFC_REG_R2H_CMD_FW_STATUS_RET)) {
			mfc_core_err("Failed to RISC_ON\n");
			core->logging_data->cause |= (1 << MFC_CAUSE_FAIL_RISC_ON);
			call_dop(core, dump_and_stop_always, core);
			return -EBUSY;
		}
	}

	mfc_core_debug(2, "Ok, now will write a command to wakeup the system\n");
	mfc_core_cmd_wakeup(core);

	mfc_core_debug(2, "Will now wait for completion of firmware wake up\n");
	if (mfc_wait_for_done_core(core, MFC_REG_R2H_CMD_WAKEUP_RET)) {
		mfc_core_err("Failed to WAKEUP\n");
		core->logging_data->cause |= (1 << MFC_CAUSE_FAIL_WAKEUP);
		call_dop(core, dump_and_stop_always, core);
		return -EBUSY;
	}

	core->int_condition = 0;
	if (core->int_err != 0 || core->int_reason != MFC_REG_R2H_CMD_WAKEUP_RET) {
		/* Failure. */
		mfc_core_err("Failed to wakeup - error: %d, int: %d\n",
				core->int_err, core->int_reason);
		call_dop(core, dump_and_stop_debug_mode, core);
	}

	core->sleep = 0;

	mfc_core_pm_clock_off(core, 0);

	mfc_core_debug_leave();

	return ret;
}

int mfc_core_run_dec_init(struct mfc_core *core, struct mfc_ctx *ctx)
{
	struct mfc_core_ctx *core_ctx = core->core_ctx[ctx->num];
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *src_mb;
	unsigned int strm_size;

	/* Initializing decoding - parsing header */

	/* Get the next source buffer */
	src_mb = mfc_get_buf(ctx, &core_ctx->src_buf_queue, MFC_BUF_SET_USED);
	if (!src_mb) {
		mfc_err("no src buffers\n");
		return -EAGAIN;
	}

	strm_size = mfc_dec_get_strm_size(ctx, src_mb);
	mfc_debug(2, "Preparing to init decoding\n");
	mfc_debug(2, "[STREAM] Header size: %d, (offset: %u, consumed: %u)\n",
			strm_size,
			src_mb->vb.vb2_buf.planes[0].data_offset,
			dec->consumed);

	mfc_core_set_dec_stream_buffer(core, ctx, src_mb,
			mfc_dec_get_strm_offset(ctx, src_mb), strm_size);

	mfc_debug(2, "[BUFINFO] Header addr: 0x%08llx\n", src_mb->addr[0][0]);
	mfc_clean_core_ctx_int_flags(core->core_ctx[ctx->num]);
	mfc_core_cmd_dec_seq_header(core, ctx);

	return 0;
}

static int __mfc_check_last_frame(struct mfc_core_ctx *core_ctx,
			struct mfc_buf *mfc_buf)
{
	if (mfc_check_mb_flag(mfc_buf, MFC_FLAG_LAST_FRAME)) {
		mfc_debug(2, "Setting core_ctx->state to FINISHING\n");
		mfc_change_state(core_ctx, MFCINST_FINISHING);
		return 1;
	}

	return 0;
}

int mfc_core_run_dec_frame(struct mfc_core *core, struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_core_ctx *core_ctx = core->core_ctx[ctx->num];
	struct mfc_buf *src_mb, *dst_mb;
	int last_frame = 0;
	unsigned int index, src_index;
	int ret;

	/* Get the next source buffer */
	if (IS_TWO_MODE2(ctx)) {
		src_mb = mfc_get_buf_no_used(ctx, &core_ctx->src_buf_queue,
				MFC_BUF_SET_USED);
		if (!src_mb) {
			mfc_debug(2, "no src buffers\n");
			return -EAGAIN;
		}
	} else {
		src_mb = mfc_get_buf(ctx, &core_ctx->src_buf_queue,
				MFC_BUF_SET_USED);
		if (!src_mb) {
			mfc_debug(2, "no src buffers\n");
			return -EAGAIN;
		}
	}

	/* Try to use the non-referenced DPB on dst-queue */
	if (dec->is_dynamic_dpb) {
		dst_mb = mfc_search_for_dpb(core_ctx);
		if (!dst_mb) {
			src_mb->used = MFC_BUF_RESET_USED;
			mfc_debug(2, "[DPB] couldn't find dst buffers\n");
			return -EAGAIN;
		}
	}

	index = src_mb->vb.vb2_buf.index;
	src_index = src_mb->src_index;

	if (mfc_check_mb_flag(src_mb, MFC_FLAG_EMPTY_DATA))
		src_mb->vb.vb2_buf.planes[0].bytesused = 0;

	mfc_core_set_dec_stream_buffer(core, ctx, src_mb,
			mfc_dec_get_strm_offset(ctx, src_mb),
			mfc_dec_get_strm_size(ctx, src_mb));

	if (call_bop(ctx, core_set_buf_ctrls, core, ctx, &ctx->src_ctrls[index]) < 0)
		mfc_err("failed in core_set_buf_ctrls\n");
	mfc_core_update_tag(core, ctx, ctx->stored_tag);

	if (dec->is_dynamic_dpb)
		mfc_core_set_dynamic_dpb(core, ctx, dst_mb);

	mfc_clean_core_ctx_int_flags(core_ctx);

	last_frame = __mfc_check_last_frame(core_ctx, src_mb);
	ret = mfc_core_cmd_dec_one_frame(core, ctx, last_frame, src_index);

	if (dec->consumed && IS_MULTI_MODE(ctx) && !CODEC_MULTIFRAME(ctx)) {
		mfc_debug(2, "[STREAM][2CORE] clear consumed for next core\n");
		dec->consumed = 0;
	}
	return ret;
}

int mfc_core_run_dec_last_frames(struct mfc_core *core, struct mfc_ctx *ctx)
{
	struct mfc_core_ctx *core_ctx = core->core_ctx[ctx->num];
	struct mfc_dec *dec = ctx->dec_priv;
	struct mfc_buf *src_mb, *dst_mb;
	unsigned int src_index;

	if (mfc_is_queue_count_same(&ctx->buf_queue_lock, &ctx->dst_buf_queue, 0)) {
		mfc_debug(2, "no dst buffer\n");
		return -EAGAIN;
	}

	/* Try to use the non-referenced DPB on dst-queue */
	if (dec->is_dynamic_dpb) {
		dst_mb = mfc_search_for_dpb(core_ctx);
		if (!dst_mb) {
			mfc_debug(2, "[DPB] couldn't find dst buffers\n");
			return -EAGAIN;
		}
	}

	/* Get the next source buffer */
	src_mb = mfc_get_buf(ctx, &core_ctx->src_buf_queue, MFC_BUF_SET_USED);

	/* Frames are being decoded */
	mfc_core_set_dec_stream_buffer(core, ctx, 0, 0, 0);
	src_index = ctx->curr_src_index + 1;

	if (dec->is_dynamic_dpb)
		mfc_core_set_dynamic_dpb(core, ctx, dst_mb);

	mfc_clean_core_ctx_int_flags(core_ctx);
	mfc_core_cmd_dec_one_frame(core, ctx, 1, src_index);

	return 0;
}

int mfc_core_run_enc_init(struct mfc_core *core, struct mfc_ctx *ctx)
{
	struct mfc_buf *dst_mb;
	int ret;

	if (!(ctx->stream_op_mode == MFC_OP_TWO_MODE1 && core->id == MFC_CORE_SUB)) {
		dst_mb = mfc_get_buf(ctx, &ctx->dst_buf_queue, MFC_BUF_SET_USED);
		if (!dst_mb) {
			mfc_ctx_debug(2, "no dst buffers\n");
			return -EAGAIN;
		}

		mfc_core_set_enc_stream_buffer(core, ctx, dst_mb);
		mfc_ctx_debug(2, "[BUFINFO] Header addr: 0x%08llx\n", dst_mb->addr[0][0]);
	}

	mfc_core_set_enc_stride(core, ctx);

	mfc_clean_core_ctx_int_flags(core->core_ctx[ctx->num]);

	ret = mfc_core_cmd_enc_seq_header(core, ctx);
	return ret;
}

int mfc_core_run_enc_frame(struct mfc_core *core, struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_core_ctx *core_ctx = core->core_ctx[ctx->num];
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_buf *dst_mb;
	struct mfc_buf *src_mb;
	struct mfc_raw_info *raw;
	struct hdr10_plus_meta dst_sei_meta, *src_sei_meta;
	exynos_video_sub_view_meta *src_sub_meta;
	unsigned int index;
	int last_frame = 0;
	int is_uncomp = 0;
	unsigned char *vaddr;

	raw = &ctx->raw_buf;

	/* Get the next source buffer */
	src_mb = mfc_get_buf(ctx, &core_ctx->src_buf_queue, MFC_BUF_SET_USED);
	if (!src_mb) {
		mfc_debug(2, "no src buffers\n");
		return -EAGAIN;
	}

	if (src_mb->num_valid_bufs > 0) {
		/* last image in a buffer container */
		if (src_mb->next_index == (src_mb->num_valid_bufs - 1)) {
			mfc_debug(4, "[BUFCON] last image in a container\n");
			last_frame = __mfc_check_last_frame(core_ctx, src_mb);
		}
	} else {
		last_frame = __mfc_check_last_frame(core_ctx, src_mb);
	}

	/* Support per-frame SBWC change for encoder source */
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->sbwc_enc_src_ctrl)
			&& ctx->is_sbwc) {
		if (mfc_check_mb_flag(src_mb, MFC_FLAG_ENC_SRC_UNCOMP)) {
			is_uncomp = 1;
			mfc_debug(2, "[SBWC] src is uncomp\n");
		} else {
			is_uncomp = 0;
		}

		mfc_core_set_enc_src_sbwc(core,
			(is_uncomp ? MFC_ENC_SRC_SBWC_OFF : MFC_ENC_SRC_SBWC_ON));
		mfc_set_linear_stride_size(ctx, &ctx->raw_buf,
			(is_uncomp ? enc->uncomp_fmt : ctx->src_fmt));
		mfc_core_set_enc_stride(core, ctx);
	}

	/* Support per-frame vOTF change for encoder source */
	if (ctx->gdc_votf) {
		if (mfc_check_mb_flag(src_mb, MFC_FLAG_ENC_SRC_VOTF)) {
			mfc_debug(2, "[vOTF] Source(GDC) is vOTF\n");
			if (mfc_core_votf_run(ctx, &core_ctx->src_buf_queue,
						src_mb->i_ino))
				return -EAGAIN;
			mfc_core_set_enc_src_votf(core, MFC_ENC_SRC_VOTF_ON);
		} else {
			mfc_debug(3, "[vOTF] Source(GDC) is not vOTF. OFF\n");
			mfc_core_set_enc_src_votf(core, MFC_ENC_SRC_VOTF_OFF);
		}
	}

	if (mfc_check_mb_flag(src_mb, MFC_FLAG_ENC_SRC_FAKE)) {
		enc->fake_src = 1;
		mfc_debug(2, "src is fake\n");
	}

	index = src_mb->vb.vb2_buf.index;
	if (mfc_check_mb_flag(src_mb, MFC_FLAG_EMPTY_DATA)) {
		enc->empty_data = 1;
		mfc_debug(2, "[FRAME] src is empty data\n");
		mfc_core_set_enc_frame_buffer(core, ctx, 0, raw->num_planes);
	} else {
		mfc_core_set_enc_frame_buffer(core, ctx, src_mb, raw->num_planes);
	}

	/* HDR10+ sei meta */
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->hdr10_plus)) {
		if (enc->sh_handle_hdr.fd == -1) {
			mfc_debug(3, "[HDR+] there is no handle for SEI meta\n");
		} else {
			src_sei_meta = (struct hdr10_plus_meta *)enc->sh_handle_hdr.vaddr + index;
			if (src_sei_meta->valid) {
				if (ctx->multi_view_enable && ctx->select_view == MFC_VIEW_ID_SUB) {
					mfc_debug(3, "[HDR+/SUB] there is valid SUB meta data in buf[%d]\n",
							index);
					vaddr = vb2_plane_vaddr(&src_mb->vb.vb2_buf,
								ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW1_META].offset);
					src_sub_meta = (exynos_video_sub_view_meta *)vaddr;
					if (src_sub_meta->eType ==
						VIDEO_SUBVIEW_INFO_TYPE_HDR_DYNAMIC) {
						memcpy_for_sub_view_meta(&dst_sei_meta, src_sub_meta);
						mfc_core_set_hdr_plus_info(core, ctx, &dst_sei_meta);
					} else {
						mfc_debug(3, "[HDR+/SUB] hdr meta of main-view was valid, however sub-view is not.\n");
					}
				}
				else {
					mfc_debug(3, "[HDR+] there is valid SEI meta data in buf[%d]\n",
							index);
					memcpy(&dst_sei_meta, src_sei_meta,
						sizeof(struct hdr10_plus_meta));
					mfc_core_set_hdr_plus_info(core, ctx, &dst_sei_meta);
				}
			}
		}
	}

	dst_mb = mfc_get_buf(ctx, &ctx->dst_buf_queue, MFC_BUF_SET_USED);
	if (!dst_mb) {
		mfc_debug(2, "no dst buffers\n");
		return -EAGAIN;
	}

	mfc_debug(2, "nal start : src index from src_buf_queue:%d\n",
		src_mb->vb.vb2_buf.index);
	mfc_debug(2, "nal start : dst index from dst_buf_queue:%d\n",
		dst_mb->vb.vb2_buf.index);

	mfc_core_set_enc_stream_buffer(core, ctx, dst_mb);

	if (call_bop(ctx, core_set_buf_ctrls, core, ctx, &ctx->src_ctrls[index]) < 0)
		mfc_err("failed in core_set_buf_ctrls\n");

	mfc_clean_core_ctx_int_flags(core_ctx);

	if (IS_H264_ENC(ctx))
		mfc_core_set_aso_slice_order_h264(core, ctx);
	if (!dev->debugfs.reg_test)
		mfc_core_set_slice_mode(core, ctx);
	mfc_core_set_enc_config_qp(core, ctx);
	mfc_core_set_enc_ts_delta(core, ctx);

	/* HDR10+ statistic info */
	index = dst_mb->vb.vb2_buf.index;
	if (MFC_FEATURE_SUPPORT(dev, dev->pdata->hdr10_plus_stat_info)) {
		if (enc->sh_handle_hdr10_plus_stat.fd == -1)
			mfc_debug(3, "[HDR+] there is no handle for stat info\n");
		else
			mfc_core_set_hdr10_plus_stat_info(core, ctx, index);
	}

	mfc_core_cmd_enc_one_frame(core, ctx, last_frame);

	return 0;
}

int mfc_core_run_enc_last_frames(struct mfc_core *core, struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_buf *dst_mb = NULL;
	struct mfc_raw_info *raw;
	unsigned int index;

	raw = &ctx->raw_buf;

	if (IS_SWITCH_SINGLE_MODE(ctx) && core->id == ctx->op_core_num[MFC_CORE_SUB]) {
		mfc_ctx_info("last-frame of subcore doesn't have dst buffer\n");
	} else {
		dst_mb = mfc_get_buf(ctx, &ctx->dst_buf_queue, MFC_BUF_SET_USED);
		if (!dst_mb) {
			mfc_ctx_debug(2, "no dst buffers set to zero\n");

			if (mfc_is_enc_bframe(ctx))
				mfc_ctx_info("B frame encoding doesn't have dst buffer\n");
		}
	}

	mfc_ctx_debug(2, "Set address zero for all planes\n");
	mfc_core_set_enc_frame_buffer(core, ctx, 0, raw->num_planes);
	mfc_core_set_enc_stream_buffer(core, ctx, dst_mb);

	if (mfc_is_enc_bframe(ctx)) {
		/* HDR10+ statistic info */
		if (MFC_FEATURE_SUPPORT(dev, dev->pdata->hdr10_plus_stat_info)) {
			if (enc->sh_handle_hdr10_plus_stat.fd == -1) {
				mfc_ctx_debug(3, "[HDR+] there is no handle for stat info\n");
			} else {
				if (dst_mb) {
					index = dst_mb->vb.vb2_buf.index;
					mfc_core_set_hdr10_plus_stat_info(core, ctx, index);
				} else {
					mfc_ctx_info("[HDR+] there is no dst buffer in last + B frame\n");
				}
			}
		}
	}

	mfc_clean_core_ctx_int_flags(core->core_ctx[ctx->num]);
	mfc_core_cmd_enc_one_frame(core, ctx, 1);

	return 0;
}
