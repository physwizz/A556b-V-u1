/*
 * drivers/media/platform/exynos/mfc/mfc_enc_vb2_ops.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "base/mfc_common.h"

#include "mfc_rm.h"

#include "mfc_core_sync.h"

#include "base/mfc_rate_calculate.h"
#include "base/mfc_meminfo.h"
#include "base/mfc_queue.h"
#include "base/mfc_utils.h"
#include "base/mfc_buf.h"
#include "base/mfc_mem.h"

static int mfc_enc_queue_setup(struct vb2_queue *vq,
				unsigned int *buf_count, unsigned int *plane_count,
				unsigned int psize[], struct device *alloc_devs[])
{
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_core *core;
	struct mfc_core_ctx *core_ctx;
	struct mfc_raw_info *raw;
	int i;

	mfc_ctx_debug_enter();

	/* Encoder works only single core */
	core = mfc_get_main_core_lock(dev, ctx);
	core_ctx = core->core_ctx[ctx->num];

	if (core_ctx->state != MFCINST_GOT_INST &&
	    vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_err("invalid state: %d\n", core_ctx->state);
		return -EINVAL;
	}
	if (core_ctx->state >= MFCINST_FINISHING &&
	    vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_err("invalid state: %d\n", core_ctx->state);
		return -EINVAL;
	}

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_debug(4, "enc dst\n");
		if (ctx->dst_fmt)
			*plane_count = ctx->dst_fmt->mem_planes;
		else
			*plane_count = MFC_ENC_CAP_PLANE_COUNT;

		if (*buf_count < 1)
			*buf_count = 1;
		if (*buf_count > MFC_MAX_BUFFERS)
			*buf_count = MFC_MAX_BUFFERS;

		psize[0] = enc->dst_buf_size;
		alloc_devs[0] = dev->device;
		/* In case of VP8/VP9 encoder, part of stream buffer should be read */
		vq->dma_dir = DMA_BIDIRECTIONAL;
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		mfc_ctx_debug(4, "enc src\n");
		raw = &ctx->raw_buf;

		if (ctx->src_fmt)
			*plane_count = ctx->num_fd_frame;
		else
			*plane_count = MFC_ENC_OUT_PLANE_COUNT;

		if (*buf_count < 1)
			*buf_count = 1;
		if (*buf_count > MFC_MAX_BUFFERS)
			*buf_count = MFC_MAX_BUFFERS;

		/* need to use minimum size to prevent qbuf fail */
		if (*plane_count == 1) {
			psize[0] = 1;
			alloc_devs[0] = dev->device;
		} else {
			for (i = 0; i < *plane_count; i++) {
				psize[i] = 1;
				alloc_devs[i] = dev->device;
			}
		}
	} else {
		mfc_err("invalid queue type: %d\n", vq->type);
		return -EINVAL;
	}

	mfc_debug(2, "buf_count: %d, plane_count: %d, type: %#x\n",
			*buf_count, *plane_count, vq->type);
	for (i = 0; i < *plane_count; i++)
		mfc_debug(2, "plane[%d] size: %d\n", i, psize[i]);

	mfc_ctx_debug_leave();

	return 0;
}

static void mfc_enc_unlock(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;

	mutex_unlock(&dev->mfc_mutex);
}

static void mfc_enc_lock(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;

	mutex_lock(&dev->mfc_mutex);
}

static int mfc_enc_buf_init(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	int ret;

	mfc_ctx_debug_enter();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = mfc_check_vb_with_fmt(ctx->dst_fmt, vb);
		if (ret < 0)
			return ret;

		if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_DST,
					vb->index) < 0)
			mfc_ctx_err("failed in init_buf_ctrls\n");

	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		ret = mfc_check_vb_with_fmt(ctx->src_fmt, vb);
		if (ret < 0)
			return ret;

		if (call_cop(ctx, init_buf_ctrls, ctx, MFC_CTRL_TYPE_SRC,
					vb->index) < 0)
			mfc_ctx_err("failed in init_buf_ctrls\n");
	} else {
		mfc_ctx_err("inavlid queue type: %d\n", vq->type);
		return -EINVAL;
	}

	mfc_ctx_debug_leave();

	return 0;
}

static int mfc_enc_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_enc *enc = ctx->enc_priv;
	struct mfc_raw_info *raw;
	unsigned int index = vb->index;
	struct mfc_buf *buf = vb_to_mfc_buf(vb);
	struct dma_buf *bufcon_dmabuf[MFC_MAX_PLANES];
	int i, mem_get_count = 0;
	size_t buf_size;

	mfc_ctx_debug_enter();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		buf_size = vb2_plane_size(vb, 0);
		mfc_ctx_debug(2, "[STREAM] vb size: %lu, calc size: %u\n",
			buf_size, enc->dst_buf_size);

		if (buf_size < enc->dst_buf_size) {
			mfc_ctx_err("[STREAM] size(%lu) is smaller than (%d)\n",
					buf_size, enc->dst_buf_size);
			return -EINVAL;
		}

		buf->addr[0][0] = mfc_mem_get_daddr_vb(vb, 0);

		/* Copy dst buffer flag to buf_ctrl */
		buf->flag = call_cop(ctx, get_buf_ctrl_val, ctx,
				&ctx->dst_ctrls[index],
				V4L2_CID_MPEG_VIDEO_DST_BUF_FLAG);
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		raw = &ctx->raw_buf;
		if (ctx->src_fmt->mem_planes == 1) {
			buf_size = vb2_plane_size(vb, 0);
			mfc_ctx_debug(2, "[FRAME] single plane vb size: %lu, calc size: %d\n",
					buf_size, raw->total_plane_size);
			if (buf_size < raw->total_plane_size) {
				mfc_ctx_err("[FRAME] single plane size(%lu) is smaller than (%d)\n",
						buf_size, raw->total_plane_size);
				return -EINVAL;
			}
		} else {
			for (i = 0; i < ctx->src_fmt->mem_planes; i++) {
				buf_size = vb2_plane_size(vb, i);
				mfc_ctx_debug(2, "[FRAME] plane[%d] vb size: %lu, calc size: %d\n",
						i, buf_size, raw->plane_size[i]);
				if (buf_size < raw->plane_size[i]) {
					mfc_ctx_err("[FRAME] plane[%d] size(%lu) is smaller than (%d)\n",
							i, buf_size, raw->plane_size[i]);
					return -EINVAL;
				}
			}
		}

		for (i = 0; i < ctx->src_fmt->mem_planes; i++) {
			bufcon_dmabuf[i] = dma_buf_get(vb->planes[i].m.fd);
			if (IS_ERR(bufcon_dmabuf[i])) {
				mfc_ctx_err("failed to get bufcon dmabuf\n");
				goto err_mem_put;
			}

			mem_get_count++;
			buf->num_bufs_in_batch = mfc_bufcon_get_buf_count(bufcon_dmabuf[i]);
			mfc_ctx_debug(3, "[BUFCON] num bufs in batch: %d\n", buf->num_bufs_in_batch);
			if (buf->num_bufs_in_batch == 0) {
				mfc_ctx_err("[BUFCON] bufs count couldn't be zero\n");
				goto err_mem_put;
			}

			if (buf->num_bufs_in_batch < 0)
				buf->num_bufs_in_batch = 0;

			if (!ctx->batch_mode && buf->num_bufs_in_batch > 0) {
				ctx->batch_mode = 1;
				mfc_ctx_debug(2, "[BUFCON] buffer batch mode enable\n");
			}

			if (buf->num_bufs_in_batch > 0) {
				if (mfc_bufcon_get_daddr(ctx, buf, bufcon_dmabuf[i], i)) {
					mfc_ctx_err("[BUFCON] failed to get daddr[%d] in buffer container\n", i);
					goto err_mem_put;
				}

				ctx->framerate = buf->num_valid_bufs * ENC_DEFAULT_CAM_CAPTURE_FPS;
				mfc_ctx_debug(3, "[BUFCON] framerate: %ld\n", ctx->framerate);

				dma_buf_put(bufcon_dmabuf[i]);
			} else {
				dma_buf_put(bufcon_dmabuf[i]);
				mfc_calc_base_addr(ctx, vb, ctx->src_fmt);
			}
		}

		call_cop(ctx, to_buf_ctrls, ctx, &ctx->src_ctrls[index]);

		/* Copy src buffer flag to buf_ctrl */
		buf->flag = call_cop(ctx, get_buf_ctrl_val, ctx,
				&ctx->src_ctrls[index],
				V4L2_CID_MPEG_VIDEO_SRC_BUF_FLAG);
	} else {
		mfc_ctx_err("inavlid queue type: %d\n", vq->type);
		return -EINVAL;
	}

	mfc_mem_buf_prepare(vb, 0);

	mfc_ctx_debug_leave();
	return 0;

err_mem_put:
	for (i = 0; i < mem_get_count; i++)
		dma_buf_put(bufcon_dmabuf[i]);

	return -ENOMEM;
}

static void mfc_enc_buf_finish(struct vb2_buffer *vb)
{
	struct mfc_buf *buf = vb_to_mfc_buf(vb);
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	unsigned int index = vb->index;
#if IS_ENABLED(CONFIG_MFC_USE_DMA_SKIP_LAZY_UNMAP)
	struct mfc_dev *dev = ctx->dev;
	int i;
#endif

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		/* Copy to dst buffer flag */
		call_cop(ctx, update_buf_val, ctx, &ctx->dst_ctrls[index],
				V4L2_CID_MPEG_VIDEO_DST_BUF_FLAG, buf->flag);
		mfc_ctx_debug(4, "[FLAG] dst update buf[%d] flag = %#x\n",
				index, buf->flag);

		call_cop(ctx, to_ctx_ctrls, ctx, &ctx->dst_ctrls[index]);

		mfc_mem_buf_finish(vb, 1);
#if IS_ENABLED(CONFIG_MFC_USE_DMA_SKIP_LAZY_UNMAP)
		vb2_dma_sg_set_map_attr(vb->planes[0].mem_priv, DMA_ATTR_SKIP_LAZY_UNMAP);
		mfc_ctx_debug(4, "[LAZY_UNMAP] skip for dst\n");
#endif
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		/* Copy to src buffer flag */
		call_cop(ctx, update_buf_val, ctx, &ctx->src_ctrls[index],
				V4L2_CID_MPEG_VIDEO_SRC_BUF_FLAG, buf->flag);
		mfc_ctx_debug(4, "[FLAG] src update buf[%d] flag = %#x\n",
				index, buf->flag);

		call_cop(ctx, to_ctx_ctrls, ctx, &ctx->src_ctrls[index]);

#if IS_ENABLED(CONFIG_MFC_USE_DMA_SKIP_LAZY_UNMAP)
		if (dev->skip_lazy_unmap || ctx->skip_lazy_unmap) {
			for (i = 0; i < ctx->src_fmt->mem_planes; i++) {
				vb2_dma_sg_set_map_attr(vb->planes[i].mem_priv,
							DMA_ATTR_SKIP_LAZY_UNMAP);
				mfc_ctx_debug(4, "[LAZY_UNMAP] skip for src plane[%d]\n", i);
			}
		}
#endif
	}
}

static void mfc_enc_buf_cleanup(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_ctx *ctx = vq->drv_priv;
	unsigned int index = vb->index;

	mfc_ctx_debug_enter();

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (call_cop(ctx, cleanup_buf_ctrls, ctx,
					MFC_CTRL_TYPE_DST, index) < 0)
			mfc_ctx_err("failed in cleanup_buf_ctrls\n");
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (call_cop(ctx, cleanup_buf_ctrls, ctx,
					MFC_CTRL_TYPE_SRC, index) < 0)
			mfc_ctx_err("failed in cleanup_buf_ctrls\n");
	} else {
		mfc_ctx_err("mfc_enc_buf_cleanup: unknown queue type\n");
	}

	mfc_ctx_debug_leave();
}

static int mfc_enc_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_core *core;
	struct mfc_core_ctx *core_ctx;

	/* Encoder works only single core */
	core = mfc_get_main_core_lock(dev, ctx);
	core_ctx = core->core_ctx[ctx->num];

	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE &&
			core_ctx->state == MFCINST_FINISHED) {
		mfc_change_state(core_ctx, MFCINST_GOT_INST);
		mfc_info("enc start_streaming changes state %d\n",
				core_ctx->state);
		MFC_TRACE_CTX("** ENC streamon, state: %d\n",
				core_ctx->state);
	}

	mfc_rm_update_real_time(ctx);
	mfc_rm_request_work(dev, MFC_WORK_TRY, ctx);

	return 0;
}

static void mfc_enc_stop_streaming(struct vb2_queue *q)
{
	struct mfc_ctx *ctx = q->drv_priv;
	struct mfc_dev *dev = ctx->dev;

	mfc_ctx_info("enc stop_streaming is called, type : %d\n", q->type);
	MFC_TRACE_CTX("** ENC streamoff(type:%d)\n", q->type);

	mfc_rm_instance_enc_stop(dev, ctx, q->type);
}

static void mfc_enc_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_queue *vq = vb->vb2_queue;
	struct mfc_core *core;
	struct mfc_core_ctx *core_ctx;
	struct mfc_ctx *ctx = vq->drv_priv;
	struct mfc_dev *dev = ctx->dev;
	struct mfc_buf *buf = vb_to_mfc_buf(vb);
	struct dma_buf *dbuf;
	int i;
	int is_dst_buf_ready;

	mfc_ctx_debug_enter();

	buf->next_index = 0;
	buf->done_index = 0;

	if (vq->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		mfc_ctx_debug(2, "[BUFINFO] ctx[%d] add dst index: %d, addr: 0x%08llx\n",
				ctx->num, vb->index, buf->addr[0][0]);

		/* Mark destination as available for use by MFC */
		mfc_add_tail_buf(ctx, &ctx->dst_buf_queue, buf);
		if (dev->debugfs.meminfo_enable == 1)
			mfc_meminfo_add_outbuf(ctx,vb);
		mfc_rm_qos_control(ctx, MFC_QOS_TRIGGER);
	} else if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (ctx->gdc_votf) {
			dbuf = dma_buf_get(vb->planes[0].m.fd);
			if (IS_ERR(dbuf)) {
				mfc_ctx_err("failed to get dmabuf for vOTF\n");
			} else {
				buf->i_ino = file_inode(dbuf->file)->i_ino;
				dma_buf_put(dbuf);
			}
			mfc_ctx_debug(2, "[vOTF] enc src qbuf index %d fd %d inode %ld paddr: %#llx\n",
					vb->index, vb->planes[0].m.fd, buf->i_ino,
					mfc_mem_get_paddr_vb(vb));
		}
		buf->src_index = ctx->serial_src_index++;
		if (ctx->multi_view_enable) {
			for (i = 0; i < ctx->raw_buf.num_planes; i++)
				mfc_ctx_debug(2, "[BUFINFO-view0] ctx[%d] add src index: %d(%d), addr[0][%d]: 0x%08llx\n",
						ctx->num, vb->index, buf->src_index, i, buf->addr[0][i]);
			for (i = 0; i < ctx->raw_buf.num_planes; i++)
				mfc_ctx_debug(2, "[BUFINFO-view1] ctx[%d] add src index: %d(%d), addr[2][%d]: 0x%08llx\n",
						ctx->num, vb->index, buf->src_index, i, buf->addr[2][i]);
		}
		else {
			if (ctx->num_fd_frame > 3) {
				mfc_ctx_err("if not multi_view_enable, num_fd_frame must be <= 3\n");
			}
			else {
				for (i = 0; i < ctx->num_fd_frame; i++) {
					mfc_ctx_debug(2, "[BUFINFO] ctx[%d] add src index: %d(%d), addr[%d]: 0x%08llx\n",
							ctx->num, vb->index, buf->src_index, i, buf->addr[0][i]);
				}
			}
		}
		mfc_add_tail_buf(ctx, &ctx->src_buf_ready_queue, buf);

		if (dev->debugfs.debug_ts == 1)
			mfc_ctx_info("[TS] framerate: %ld, timestamp: %lld\n",
					ctx->framerate, buf->vb.vb2_buf.timestamp);
		if (dev->debugfs.meminfo_enable == 1)
			mfc_meminfo_add_inbuf(ctx, vb);

		mfc_rate_update_last_framerate(ctx, buf->vb.vb2_buf.timestamp);
		mfc_rm_qos_control(ctx, MFC_QOS_TRIGGER);
	} else {
		mfc_ctx_err("unsupported buffer type (%d)\n", vq->type);
	}

	if (ctx->stream_op_mode == MFC_OP_TWO_MODE1)
		is_dst_buf_ready =
			mfc_is_queue_count_greater(&ctx->buf_queue_lock,
						   &ctx->dst_buf_queue, 0);

	mfc_rm_request_work(dev, MFC_WORK_TRY, ctx);

	if (!mfc_rm_query_state(ctx, EQUAL_BIGGER, MFCINST_HEAD_PARSED) &&
	    ctx->stream_op_mode == MFC_OP_TWO_MODE1 && is_dst_buf_ready) {
		core = mfc_get_main_core(dev, ctx);
		if (core == NULL) {
			mfc_ctx_err("[RM] main core is NULL\n");
			return;
		}
		core_ctx = core->core_ctx[ctx->num];

		if (mfc_wait_for_done_core_ctx(core_ctx,
				MFC_REG_R2H_CMD_SEQ_DONE_RET)) {
			mfc_ctx_err("[RM] sub core header parsing failed\n");
			return;
		}

		mfc_ctx_info("[2CORE] start the sub core\n");
		if (ctx->op_core_num[MFC_CORE_SUB] == MFC_CORE_INVALID) {
			if (mfc_rm_instance_setup(dev, ctx))
				mfc_ctx_err("[2CORE] failed to setup sub core\n");
		} else {
			if (mfc_rm_subcore_seq_start(dev, ctx))
				mfc_ctx_err("[2CORE] failed to seq_start sub core\n");
		}
	}

	mfc_ctx_debug_leave();
}

const struct vb2_ops mfc_enc_qops = {
	.queue_setup		= mfc_enc_queue_setup,
	.wait_prepare		= mfc_enc_unlock,
	.wait_finish		= mfc_enc_lock,
	.buf_init		= mfc_enc_buf_init,
	.buf_prepare		= mfc_enc_buf_prepare,
	.buf_finish		= mfc_enc_buf_finish,
	.buf_cleanup		= mfc_enc_buf_cleanup,
	.start_streaming	= mfc_enc_start_streaming,
	.stop_streaming		= mfc_enc_stop_streaming,
	.buf_queue		= mfc_enc_buf_queue,
};
