/*
 * drivers/media/platform/exynos/mfc/mfc_mem.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/property.h>
#include <linux/dma-buf.h>
#include <linux/iosys-map.h>
#include <linux/iommu.h>
#include <linux/dma-heap.h>

#include "mfc_mem.h"

struct vb2_mem_ops *mfc_mem_ops(void)
{
	return (struct vb2_mem_ops *)&vb2_dma_sg_memops;
}

int mfc_mem_get_user_shared_handle(struct mfc_ctx *ctx,
	struct mfc_user_shared_handle *handle, char *name)
{
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(NULL);
	int ret = 0;

	handle->dma_buf = dma_buf_get(handle->fd);
	if (IS_ERR(handle->dma_buf)) {
		mfc_ctx_err("[MEMINFO][SH][%s] Failed to import fd\n", name);
		ret = PTR_ERR(handle->dma_buf);
		goto import_dma_fail;
	}

	if (handle->dma_buf->size < handle->data_size) {
		mfc_ctx_err("[MEMINFO][SH][%s] User-provided dma_buf size(%ld) is smaller than required size(%ld)\n",
				name, handle->dma_buf->size, handle->data_size);
		ret = -EINVAL;
		goto dma_buf_size_fail;
	}
	ret = dma_buf_vmap_unlocked(handle->dma_buf, &map);
	if (ret) {
		mfc_ctx_err("[MEMINFO][SH][%s] Failed to get kernel virtual address\n", name);
		ret = -EINVAL;
		goto map_kernel_fail;
	}

	handle->vaddr = map.vaddr;
	mfc_ctx_debug(2, "[MEMINFO][SH][%s] shared handle fd: %d, vaddr: 0x%p, buf size: %zu, data size: %zu\n",
			name, handle->fd, handle->vaddr, handle->dma_buf->size, handle->data_size);

	return 0;

map_kernel_fail:
	handle->vaddr = NULL;
dma_buf_size_fail:
	dma_buf_put(handle->dma_buf);
import_dma_fail:
	handle->dma_buf = NULL;
	handle->fd = -1;
	return ret;
}

void mfc_mem_cleanup_user_shared_handle(struct mfc_ctx *ctx,
		struct mfc_user_shared_handle *handle)
{
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(handle->vaddr);

	if (handle->vaddr)
		dma_buf_vunmap_unlocked(handle->dma_buf, &map);
	if (handle->dma_buf)
		dma_buf_put(handle->dma_buf);

	handle->data_size = 0;
	handle->dma_buf = NULL;
	handle->vaddr = NULL;
	handle->fd = -1;
}

static int mfc_mem_fw_alloc(struct mfc_dev *dev, struct mfc_special_buf *special_buf)
{
	struct page *fw_pages;
	unsigned long nr_pages = special_buf->size >> PAGE_SHIFT;
	int ret;

	special_buf->cma_area = dev->device->cma_area;
	fw_pages = cma_alloc(special_buf->cma_area, nr_pages, 0, false);
	if (!fw_pages) {
		mfc_dev_err("Failed to allocate with CMA\n");
		return -ENOMEM;
	}

	special_buf->sgt = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!special_buf->sgt) {
		mfc_dev_err("Failed to allocate with kmalloc\n");
		goto err_cma_alloc;
	}

	ret = sg_alloc_table(special_buf->sgt, 1, GFP_KERNEL);
	if (ret) {
		mfc_dev_err("Failed to allocate sg_table\n");
		goto err_sg_alloc;
	}

	sg_set_page(special_buf->sgt->sgl, fw_pages, special_buf->size, 0);

	special_buf->paddr = page_to_phys(fw_pages);
	special_buf->vaddr = phys_to_virt(special_buf->paddr);

	return 0;

err_sg_alloc:
	kfree(special_buf->sgt);
	special_buf->sgt = NULL;
err_cma_alloc:
	cma_release(special_buf->cma_area, fw_pages, nr_pages);
	return -ENOMEM;
}

static void mfc_mem_fw_free(struct mfc_special_buf *special_buf)
{
	if (special_buf->sgt) {
		cma_release(special_buf->cma_area, phys_to_page(special_buf->paddr),
				(special_buf->size >> PAGE_SHIFT));
		sg_free_table(special_buf->sgt);
		kfree(special_buf->sgt);
	}
	special_buf->sgt = NULL;
	special_buf->dma_buf = NULL;
	special_buf->attachment = NULL;
	special_buf->daddr = 0;
	special_buf->vaddr = NULL;
	special_buf->name[0] = 0;
}

static int mfc_mem_dma_heap_alloc(struct mfc_dev *dev,
		struct mfc_special_buf *special_buf)
{
	struct dma_heap *dma_heap;
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(NULL);
	const char *heapname;
	int ret = 0;

	switch (special_buf->buftype) {
	case MFCBUF_NORMAL:
		heapname = "system-uncached";
		break;
	case MFCBUF_DRM:
		heapname = "vframe-secure";
		break;
	default:
		return -EINVAL;
	}

	/* control by DMA heap API */
	dma_heap = dma_heap_find(heapname);
	if (!dma_heap) {
		mfc_dev_err("Failed to get DMA heap (name: %s)\n", heapname);
		goto err_dma_heap_find;
	}

	special_buf->dma_buf = dma_heap_buffer_alloc(dma_heap,
			special_buf->size, 0, 0);
	if (IS_ERR(special_buf->dma_buf)) {
		mfc_dev_err("Failed to allocate buffer (err %ld)\n",
				PTR_ERR(special_buf->dma_buf));
		goto err_dma_heap_alloc;
	}

	dma_heap_put(dma_heap);

	/* control by DMA buf API */
	special_buf->attachment = dma_buf_attach(special_buf->dma_buf,
					dev->device);
	if (IS_ERR(special_buf->attachment)) {
		mfc_dev_err("Failed to get dma_buf_attach (err %ld)\n",
				PTR_ERR(special_buf->attachment));
		goto err_attach;
	}

	special_buf->sgt = dma_buf_map_attachment_unlocked(special_buf->attachment,
			DMA_BIDIRECTIONAL);
	if (IS_ERR(special_buf->sgt)) {
		mfc_dev_err("Failed to get sgt (err %ld)\n",
				PTR_ERR(special_buf->sgt));
		goto err_map;
	}

	special_buf->daddr = sg_dma_address(special_buf->sgt->sgl);
	if (IS_ERR_VALUE(special_buf->daddr)) {
		mfc_dev_err("Failed to get iova (err 0x%p)\n",
				&special_buf->daddr);
		goto err_daddr;
	}

	special_buf->map_size = mfc_mem_get_sg_length(dev, special_buf->sgt);
	if (!special_buf->map_size || (special_buf->map_size < special_buf->size)) {
		mfc_dev_err("Failed to get iova map size (sg length: %zu / buf size: %zu)\n",
				special_buf->map_size, special_buf->size);
		goto err_vaddr;
	}

	if (special_buf->buftype == MFCBUF_NORMAL) {
		ret = dma_buf_vmap_unlocked(special_buf->dma_buf, &map);
		if (ret) {
			mfc_dev_err("Failed to get vaddr\n");
			goto err_vaddr;
		}
		special_buf->vaddr = map.vaddr;
	}

	special_buf->paddr = page_to_phys(sg_page(special_buf->sgt->sgl));

	return 0;
err_vaddr:
	special_buf->vaddr = NULL;
	special_buf->map_size = 0;
err_daddr:
	special_buf->daddr = 0;
	dma_buf_unmap_attachment_unlocked(special_buf->attachment, special_buf->sgt,
				 DMA_BIDIRECTIONAL);
err_map:
	special_buf->sgt = NULL;
	dma_buf_detach(special_buf->dma_buf, special_buf->attachment);
err_attach:
	special_buf->attachment = NULL;
	dma_buf_put(special_buf->dma_buf);
err_dma_heap_alloc:
	dma_heap_put(dma_heap);
	special_buf->dma_buf = NULL;
err_dma_heap_find:
	return -ENOMEM;
}

void mfc_mem_dma_heap_free(struct mfc_special_buf *special_buf)
{
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(special_buf->vaddr);

	if (special_buf->vaddr)
		dma_buf_vunmap_unlocked(special_buf->dma_buf, &map);
	if (special_buf->sgt)
		dma_buf_unmap_attachment_unlocked(special_buf->attachment,
					 special_buf->sgt, DMA_BIDIRECTIONAL);
	if (special_buf->attachment)
		dma_buf_detach(special_buf->dma_buf, special_buf->attachment);
	if (special_buf->dma_buf)
		dma_buf_put(special_buf->dma_buf);

	special_buf->dma_buf = NULL;
	special_buf->attachment = NULL;
	special_buf->sgt = NULL;
	special_buf->daddr = 0;
	special_buf->vaddr = NULL;
	special_buf->map_size = 0;
	special_buf->name[0] = 0;
}

int mfc_mem_special_buf_alloc(struct mfc_dev *dev, struct mfc_special_buf *special_buf)
{
	int ret;
	int is_drm = 0;

	if ((special_buf->buftype == MFCBUF_DRM_FW) || (special_buf->buftype == MFCBUF_DRM))
		is_drm = 1;

	mfc_dev_debug(2, "[MEMINFO] REQUEST: %s %s buf\n",
			is_drm ? "secure" : "normal", special_buf->name);

	switch (special_buf->buftype) {
	case MFCBUF_NORMAL_FW:
	case MFCBUF_DRM_FW:
		ret = mfc_mem_fw_alloc(dev, special_buf);
		break;
	case MFCBUF_DRM:
	case MFCBUF_NORMAL:
		ret = mfc_mem_dma_heap_alloc(dev, special_buf);
		break;
	default:
		mfc_dev_err("not supported mfc mem type: %d\n", special_buf->buftype);
		return -EINVAL;
	}

	mfc_dev_debug(2, "[MEMINFO] ALLOC: %s %s buf size: %zu, map size: %zu, daddr: 0x%08llx\n",
			is_drm ? "secure" : "normal", special_buf->name,
			special_buf->size, special_buf->map_size, special_buf->daddr);

	if (ret)
		mfc_dev_err("[MEMINFO] Allocating %s %s failed\n",
				is_drm ? "secure" : "normal", special_buf->name);

	return ret;
}

void mfc_mem_special_buf_free(struct mfc_dev *dev, struct mfc_special_buf *special_buf)
{
	int is_drm = 0;

	if ((special_buf->buftype == MFCBUF_DRM_FW) || (special_buf->buftype == MFCBUF_DRM))
		is_drm = 1;

	mfc_dev_debug(2, "[MEMINFO] RELEASE: %s %s buf\n",
			is_drm ? "secure" : "normal", special_buf->name);

	switch (special_buf->buftype) {
	case MFCBUF_NORMAL_FW:
	case MFCBUF_DRM_FW:
		mfc_mem_fw_free(special_buf);
		break;
	case MFCBUF_DRM:
	case MFCBUF_NORMAL:
		mfc_mem_dma_heap_free(special_buf);
		break;
	default:
		break;
	}

	return;
}

void mfc_bufcon_put_daddr(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf, int plane)
{
	int i;

	for (i = 0; i < mfc_buf->num_valid_bufs; i++) {
		if (mfc_buf->addr[i][plane]) {
			mfc_ctx_debug(4, "[BUFCON] put batch buf addr[%d][%d]: 0x%08llx\n",
					i, plane, mfc_buf->addr[i][plane]);
		}
		if (mfc_buf->attachments[i][plane])
			dma_buf_detach(mfc_buf->dmabufs[i][plane], mfc_buf->attachments[i][plane]);
		if (mfc_buf->dmabufs[i][plane])
			dma_buf_put(mfc_buf->dmabufs[i][plane]);

		mfc_buf->addr[i][plane] = 0;
		mfc_buf->attachments[i][plane] = NULL;
		mfc_buf->dmabufs[i][plane] = NULL;
	}
}

#if IS_ENABLED(CONFIG_MFC_USE_DMABUF_CONTAINER)
int mfc_bufcon_get_daddr(struct mfc_ctx *ctx, struct mfc_buf *mfc_buf,
					struct dma_buf *bufcon_dmabuf, int plane)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_raw_info *raw = &ctx->raw_buf;
	int i, j = 0;
	u32 mask;

	if (dmabuf_container_get_mask(bufcon_dmabuf, &mask)) {
		mfc_ctx_err("[BUFCON] it is not buffer container\n");
		return -1;
	}

	if (mask == 0) {
		mfc_ctx_err("[BUFCON] number of valid buffers is zero\n");
		return -1;
	}

	mfc_ctx_debug(3, "[BUFCON] bufcon mask info %#x\n", mask);

	for (i = 0; i < mfc_buf->num_bufs_in_batch; i++) {
		if ((mask & (1 << i)) == 0) {
			mfc_ctx_debug(4, "[BUFCON] unmasked buf[%d]\n", i);
			continue;
		}

		mfc_buf->dmabufs[j][plane] = dmabuf_container_get_buffer(bufcon_dmabuf, i);
		if (IS_ERR(mfc_buf->dmabufs[i][plane])) {
			mfc_ctx_err("[BUFCON] Failed to get dma_buf (err %ld)",
					PTR_ERR(mfc_buf->dmabufs[i][plane]));
			call_dop(dev, dump_and_stop_debug_mode, dev);
			goto err_get_daddr;
		}

		mfc_buf->attachments[j][plane] = dma_buf_attach(mfc_buf->dmabufs[i][plane], dev->device);
		if (IS_ERR(mfc_buf->attachments[i][plane])) {
			mfc_ctx_err("[BUFCON] Failed to get dma_buf_attach (err %ld)",
					PTR_ERR(mfc_buf->attachments[i][plane]));
			call_dop(dev, dump_and_stop_debug_mode, dev);
			goto err_get_daddr;
		}

		mfc_buf->addr[j][plane] = ion_iovmm_map(mfc_buf->attachments[i][plane], 0,
				raw->plane_size[plane], DMA_BIDIRECTIONAL, 0);
		if (IS_ERR_VALUE(mfc_buf->addr[i][plane])) {
			mfc_ctx_err("[BUFCON] Failed to allocate iova (err %pa)",
					&mfc_buf->addr[i][plane]);
			call_dop(dev, dump_and_stop_debug_mode, dev);
			goto err_get_daddr;
		}

		mfc_ctx_debug(4, "[BUFCON] get batch buf addr[%d][%d]: 0x%08llx, size: %d\n",
				j, plane, mfc_buf->addr[j][plane], raw->plane_size[plane]);
		j++;
	}

	mfc_buf->num_valid_bufs = j;
	mfc_ctx_debug(3, "[BUFCON] batch buffer has %d buffers\n", mfc_buf->num_valid_bufs);

	return 0;

err_get_daddr:
	mfc_bufcon_put_daddr(ctx, mfc_buf, plane);
	return -1;
}
#endif

void mfc_put_iovmm(struct mfc_ctx *ctx, struct dpb_table *dpb, int num_planes, int index)
{
	struct mfc_dev *dev = ctx->dev;
	int i;

	MFC_TRACE_CTX("DPB[%d] fd: %d addr: %#llx put(%d)\n",
			index, dpb[index].fd[0], dpb[index].addr[0], dpb[index].mapcnt);

	for (i = 0; i < num_planes; i++) {
#if IS_ENABLED(CONFIG_MFC_USE_DMA_SKIP_LAZY_UNMAP)
		if (dpb[index].attach[i] && (dev->skip_lazy_unmap || ctx->skip_lazy_unmap)) {
			dpb[index].attach[i]->dma_map_attrs |= DMA_ATTR_SKIP_LAZY_UNMAP;
			mfc_ctx_debug(4, "[LAZY_UNMAP] skip for dst plane[%d]\n", i);
		}
#endif

		if (dpb[index].addr[i])
			mfc_ctx_debug(2, "[IOVMM] index %d buf[%d] fd: %d addr: %#llx\n",
					index, i, dpb[index].fd[i], dpb[index].addr[i]);
		if (dpb[index].sgt[i])
			dma_buf_unmap_attachment_unlocked(dpb[index].attach[i], dpb[index].sgt[i],
					DMA_BIDIRECTIONAL);
		if (dpb[index].attach[i])
			dma_buf_detach(dpb[index].dmabufs[i], dpb[index].attach[i]);
		if (dpb[index].dmabufs[i])
			dma_buf_put(dpb[index].dmabufs[i]);

		dpb[index].fd[i] = -1;
		dpb[index].addr[i] = 0;
		dpb[index].attach[i] = NULL;
		dpb[index].dmabufs[i] = NULL;
		dpb[index].sgt[i] = NULL;
	}

	dpb[index].new_fd = -1;
	dpb[index].mapcnt--;
	mfc_ctx_debug(2, "[IOVMM] index %d mapcnt %d\n", index, dpb[index].mapcnt);

	if (dpb[index].mapcnt != 0) {
		mfc_ctx_err("[IOVMM] DPB[%d] %#llx invalid mapcnt %d\n",
				index, dpb[index].addr[0], dpb[index].mapcnt);
		call_dop(dev, dump_and_stop_debug_mode, dev);
		dpb[index].mapcnt = 0;
	}
}

void mfc_get_iovmm(struct mfc_ctx *ctx, struct vb2_buffer *vb, struct dpb_table *dpb)
{
	struct mfc_dev *dev = ctx->dev;
	int i, mem_get_count = 0;
	struct mfc_buf *mfc_buf = vb_to_mfc_buf(vb);
	int index = mfc_buf->dpb_index;
	int sub_view_index, offset;

	if (dpb[index].mapcnt != 0) {
		mfc_ctx_err("[IOVMM] DPB[%d] %#llx invalid mapcnt %d\n",
				index, dpb[index].addr[0], dpb[index].mapcnt);
		call_dop(dev, dump_and_stop_debug_mode, dev);
	}

	for (i = 0; i < ctx->dst_fmt->mem_planes; i++) {
		mem_get_count++;

		dpb[index].fd[i] = vb->planes[i].m.fd;

		dpb[index].dmabufs[i] = dma_buf_get(vb->planes[i].m.fd);
		if (IS_ERR(dpb[index].dmabufs[i])) {
			mfc_ctx_err("[IOVMM] Failed to dma_buf_get (err %ld)\n",
					PTR_ERR(dpb[index].dmabufs[i]));
			dpb[index].dmabufs[i] = NULL;
			goto err_iovmm;
		}

		dpb[index].attach[i] = dma_buf_attach(dpb[index].dmabufs[i], dev->device);
		if (IS_ERR(dpb[index].attach[i])) {
			mfc_ctx_err("[IOVMM] Failed to get dma_buf_attach (err %ld)\n",
					PTR_ERR(dpb[index].attach[i]));
			dpb[index].attach[i] = NULL;
			goto err_iovmm;
		}

		dpb[index].sgt[i] = dma_buf_map_attachment_unlocked(dpb[index].attach[i],
				DMA_BIDIRECTIONAL);
		if (IS_ERR(dpb[index].sgt[i])) {
			mfc_ctx_err("[IOVMM] Failed to get sgt (err %ld)\n",
					PTR_ERR(dpb[index].sgt[i]));
			dpb[index].sgt[i] = NULL;
			goto err_iovmm;
		}

		dpb[index].addr[i] = sg_dma_address(dpb[index].sgt[i]->sgl);
		if (IS_ERR_VALUE(dpb[index].addr[i])) {
			mfc_ctx_err("[IOVMM] Failed to get iova (err 0x%p)\n",
					&dpb[index].addr[i]);
			dpb[index].addr[i] = 0;
			goto err_iovmm;
		}

		mfc_ctx_debug(2, "[IOVMM] index %d buf[%d] fd: %d addr: %#llx\n",
				index, i, dpb[index].fd[i], dpb[index].addr[i]);
	}

	dpb[index].paddr = page_to_phys(sg_page(dpb[index].sgt[0]->sgl));
	mfc_ctx_debug(2, "[DPB] dpb index [%d][%d] paddr %#llx daddr %#llx\n",
			mfc_buf->vb.vb2_buf.index,
			index, dpb[index].paddr, dpb[index].addr[0]);

	dpb[index].mapcnt++;
	mfc_ctx_debug(2, "[IOVMM] index %d mapcnt %d\n", index, dpb[index].mapcnt);
	MFC_TRACE_CTX("DPB[%d] fd: %d addr: %#llx get(%d)\n",
			index, dpb[index].fd[0], dpb[index].addr[0], dpb[index].mapcnt);

	/* In multi_view_enable, sub_view should be mapped together.
	 * Lower 32 bits are used for main_view. Upper 32 bits are used for sub_view. */
	if (ctx->multi_view_enable) {
		sub_view_index = index + MFC_MAX_DPBS / 2;
		if (dpb[sub_view_index].mapcnt != 0) {
			mfc_ctx_err("[IOVMM] DPB[%d] %#llx invalid mapcnt %d\n",
					sub_view_index, dpb[sub_view_index].addr[0], dpb[sub_view_index].mapcnt);
			call_dop(dev, dump_and_stop_debug_mode, dev);
		}

		offset = ctx->view_buf_info[MFC_MV_BUF_IDX_VIEW1].offset;
		for (i = 0; i < ctx->dst_fmt->mem_planes; i++) {
			mem_get_count++;

			dpb[sub_view_index].fd[i] = vb->planes[offset + i].m.fd;

			dpb[sub_view_index].dmabufs[i] = dma_buf_get(vb->planes[offset + i].m.fd);
			if (IS_ERR(dpb[sub_view_index].dmabufs[i])) {
				mfc_ctx_err("[IOVMM] Failed to dma_buf_get (err %ld)\n",
						PTR_ERR(dpb[sub_view_index].dmabufs[i]));
				dpb[sub_view_index].dmabufs[i] = NULL;
				goto err_iovmm_sub;
			}

			dpb[sub_view_index].attach[i] = dma_buf_attach(dpb[sub_view_index].dmabufs[i], dev->device);
			if (IS_ERR(dpb[sub_view_index].attach[i])) {
				mfc_ctx_err("[IOVMM] Failed to get dma_buf_attach (err %ld)\n",
						PTR_ERR(dpb[sub_view_index].attach[i]));
				dpb[sub_view_index].attach[i] = NULL;
				goto err_iovmm_sub;
			}

			dpb[sub_view_index].sgt[i] = dma_buf_map_attachment_unlocked(dpb[sub_view_index].attach[i],
					DMA_BIDIRECTIONAL);
			if (IS_ERR(dpb[sub_view_index].sgt[i])) {
				mfc_ctx_err("[IOVMM] Failed to get sgt (err %ld)\n",
						PTR_ERR(dpb[sub_view_index].sgt[i]));
				dpb[sub_view_index].sgt[i] = NULL;
				goto err_iovmm_sub;
			}

			dpb[sub_view_index].addr[i] = sg_dma_address(dpb[sub_view_index].sgt[i]->sgl);
			if (IS_ERR_VALUE(dpb[sub_view_index].addr[i])) {
				mfc_ctx_err("[IOVMM] Failed to get iova (err 0x%p)\n",
						&dpb[sub_view_index].addr[i]);
				dpb[sub_view_index].addr[i] = 0;
				goto err_iovmm_sub;
			}

			mfc_ctx_debug(2, "[IOVMM] sub_view_index %d buf[%d] fd: %d addr: %#llx\n",
					sub_view_index, i, dpb[sub_view_index].fd[i], dpb[sub_view_index].addr[i]);
		}

		dpb[sub_view_index].paddr = page_to_phys(sg_page(dpb[sub_view_index].sgt[0]->sgl));
		mfc_ctx_debug(2, "[DPB] dpb index [%d][%d] paddr %#llx daddr %#llx\n",
				mfc_buf->vb.vb2_buf.index,
				sub_view_index, dpb[sub_view_index].paddr, dpb[sub_view_index].addr[0]);

		dpb[sub_view_index].mapcnt++;
		mfc_ctx_debug(2, "[IOVMM] index %d mapcnt %d\n", index, dpb[sub_view_index].mapcnt);
		MFC_TRACE_CTX("DPB[%d] fd: %d addr: %#llx get(%d)\n",
				sub_view_index, dpb[sub_view_index].fd[0], dpb[sub_view_index].addr[0], dpb[sub_view_index].mapcnt);
	}

	return;

err_iovmm:
	dpb[index].mapcnt++;
	mfc_put_iovmm(ctx, dpb, mem_get_count, index);
	return;
err_iovmm_sub:
	dpb[sub_view_index].mapcnt++;
	mfc_put_iovmm(ctx, dpb, mem_get_count, index);
	mfc_put_iovmm(ctx, dpb, mem_get_count, sub_view_index);
	return;
}

void mfc_init_dpb_table(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	int index, plane;

	mutex_lock(&dec->dpb_mutex);
	for (index = 0; index < MFC_MAX_DPBS; index++) {
		for (plane = 0; plane < MFC_MAX_PLANES; plane++) {
			dec->dpb[index].fd[plane] = -1;
			dec->dpb[index].addr[plane] = 0;
			dec->dpb[index].attach[plane] = NULL;
			dec->dpb[index].dmabufs[plane] = NULL;
		}
		dec->dpb[index].new_fd = -1;
		dec->dpb[index].mapcnt = 0;
		dec->dpb[index].queued = 0;
	}
	mutex_unlock(&dec->dpb_mutex);
}

void mfc_cleanup_iovmm(struct mfc_ctx *ctx)
{
	struct mfc_dev *dev = ctx->dev;
	struct mfc_dec *dec = ctx->dec_priv;
	int i;

	mutex_lock(&dec->dpb_mutex);

	for (i = 0; i < MFC_MAX_DPBS; i++) {
		dec->dpb[i].paddr = 0;
		dec->dpb[i].ref = 0;
		if (dec->dpb[i].mapcnt == 0) {
			continue;
		} else if (dec->dpb[i].mapcnt == 1) {
			mfc_put_iovmm(ctx, dec->dpb, ctx->dst_fmt->mem_planes, i);
		} else {
			mfc_ctx_err("[IOVMM] DPB[%d] %#llx invalid mapcnt %d\n",
					i, dec->dpb[i].addr[0], dec->dpb[i].mapcnt);
			MFC_TRACE_CTX("DPB[%d] %#llx invalid mapcnt %d\n",
					i, dec->dpb[i].addr[0], dec->dpb[i].mapcnt);
			call_dop(dev, dump_and_stop_debug_mode, dev);
		}
	}

	mutex_unlock(&dec->dpb_mutex);
}

void mfc_cleanup_iovmm_except_used(struct mfc_ctx *ctx)
{
	struct mfc_dec *dec = ctx->dec_priv;
	int i;

	mutex_lock(&dec->dpb_mutex);

	for (i = 0; i < MFC_MAX_DPBS; i++) {
		if (dec->dynamic_used & (1UL << i)) {
			continue;
		} else {
			dec->dpb[i].paddr = 0;
			dec->dpb[i].ref = 0;
			if (dec->dpb[i].mapcnt == 0) {
				continue;
			} else if (dec->dpb[i].mapcnt == 1) {
				dec->dpb_table_used &= ~(1UL << i);
				mfc_put_iovmm(ctx, dec->dpb, ctx->dst_fmt->mem_planes, i);
			} else {
				mfc_ctx_err("[IOVMM] DPB[%d] %#llx invalid mapcnt %d\n",
						i, dec->dpb[i].addr[0], dec->dpb[i].mapcnt);
				MFC_TRACE_CTX("DPB[%d] %#llx invalid mapcnt %d\n",
						i, dec->dpb[i].addr[0], dec->dpb[i].mapcnt);
			}
		}
	}

	mutex_unlock(&dec->dpb_mutex);
}

void mfc_iova_pool_free(struct mfc_dev *dev, struct mfc_special_buf *buf)
{
	/* Project that do not support iova reservation */
	if (!dev->pdata->reserved_start)
		return;

	if (!buf->iova) {
		mfc_dev_debug(2, "[POOL] There is no reserved iova for %s\n", buf->name);
		return;
	}

	if (buf->map_size)
		iommu_unmap(dev->domain, buf->daddr, buf->map_size);

	gen_pool_free(dev->iova_pool, buf->daddr, buf->map_size);

	mfc_dev_debug(2, "[POOL] FREE: %s iova: %#llx ++ %#zx, %zu/%zu (avail/total KB)\n",
			buf->name, buf->daddr, buf->map_size,
			gen_pool_avail(dev->iova_pool) / 1024,
			gen_pool_size(dev->iova_pool) / 1024);

	buf->daddr = buf->iova;
	buf->iova = 0;
}

/*
 * This iova is allocated for special FW only memory.
 * These memories deliver to F/W without 4bit shift even if 36bit DMA is supported,
 * because it is already less than 1GB(0x4000_0000) address.
 * Therefore, it should be noted that the memory below should use
 * MFC_CORE_READL(WRITEL) rather than MFC_CORE_DMA_READL(WRITEL).
 * - MFC_REG_CONTEXT_MEM_ADDR, MFC0_REG_COMMON_CONTEXT_MEM_ADDR, MFC1_REG_COMMON_CONTEXT_MEM_ADDR
 * - MFC_REG_DBG_BUFFER_ADDR
 * - MFC_REG_NAL_QUEUE_INPUT_ADDR, MFC_REG_NAL_QUEUE_OUTPUT_ADDR
 * - MFC_REG_D_METADATA_BUFFER_ADDR, MFC_REG_E_METADATA_BUFFER_ADDR
 * - MFC_REG_SHARED_MEM_ADDR (not used)
 */
int mfc_iova_pool_alloc(struct mfc_dev *dev, struct mfc_special_buf *buf)
{
	size_t size;

	/* Project that do not support iova reservation */
	if (!dev->pdata->reserved_start)
		return 0;

	if (!dev->iova_pool) {
		mfc_dev_err("there is no iova pool\n");
		return -ENOMEM;
	}

	buf->iova = buf->daddr;
	buf->daddr = gen_pool_alloc(dev->iova_pool, buf->map_size);
	if (buf->daddr == 0) {
		buf->daddr = buf->iova;
		buf->iova = 0;
		mfc_dev_err("[POOL] failed alloc %s iova. %zu/%zu (avail/total KB)\n",
				buf->name,
				gen_pool_avail(dev->iova_pool) / 1024,
				gen_pool_size(dev->iova_pool) / 1024);
		return -ENOMEM;
	}

	mfc_dev_debug(2, "[POOL] ALLOC: %s iova: %#llx ++ %#zx, %zu/%zu (avail/total KB)\n",
			buf->name, buf->daddr, buf->map_size,
			gen_pool_avail(dev->iova_pool) / 1024,
			gen_pool_size(dev->iova_pool) / 1024);

	size = iommu_map_sg(dev->domain, buf->daddr, buf->sgt->sgl,
			buf->sgt->orig_nents, IOMMU_READ|IOMMU_WRITE, GFP_KERNEL);
	if (!size || (size != buf->map_size)) {
		mfc_dev_err("[POOL] Failed to map %s iova %#llx (map size: %zu)\n",
				buf->name, buf->daddr, size);
		mfc_iova_pool_free(dev, buf);
		return -ENOMEM;
	}

	mfc_dev_debug(2, "[POOL] MAP: %s iova size: %zu(%#zx)\n",
			buf->name, buf->map_size, buf->map_size);

	return 0;
}

int mfc_iova_pool_init(struct mfc_dev *dev)
{
	struct device_node *node = dev->device->of_node;
	dma_addr_t reserved_base;
	const __be32 *prop;
	size_t reserved_size;
	int n_addr_cells = of_n_addr_cells(node);
	int n_size_cells = of_n_size_cells(node);
	int n_all_cells = n_addr_cells + n_size_cells;
	int i, cnt, ret, found = 0;
	char name[10];

	snprintf(name, sizeof(name), "MFC");
	dev->iova_pool = devm_gen_pool_create(dev->device, PAGE_SHIFT, -1, name);
	if (IS_ERR(dev->iova_pool)) {
		mfc_dev_err("failed to create MFC IOVA gen pool\n");
		goto err_init;
	}

	prop = of_get_property(node, "samsung,iommu-reserved-map", &cnt);
	if (!prop) {
		mfc_dev_err("No reserved F/W dma area\n");
		goto err_init;
	}

	cnt /= sizeof(unsigned int);
	if (cnt % n_all_cells != 0) {
		mfc_dev_err("Invalid number(%d) of values\n", cnt);
		goto err_init;
	}

	for (i = 0; i < cnt; i += n_all_cells) {
		reserved_base = of_read_number(prop + i, n_addr_cells);
		reserved_size = of_read_number(prop + i + n_addr_cells, n_size_cells);
		if (reserved_base == dev->pdata->reserved_start) {
			found = 1;
			break;
		}
	}

	if (!found) {
		mfc_dev_err("failed to search for reserved address %#x\n",
				dev->pdata->reserved_start);
		goto err_init;
	}

	ret = gen_pool_add(dev->iova_pool, reserved_base, reserved_size, -1);
	if (ret) {
		mfc_dev_err("failed to set address range of MFC IOVA pool (ret: %d)\n", ret);
		goto err_init;
	}
	mfc_dev_info("[POOL] Add MFC iova ranges %#llx ++ %#zx\n",
					reserved_base, reserved_size);

	return 0;

err_init:
	dev->iova_pool = NULL;
	return -ENOMEM;
}

int mfc_iommu_map_firmware(struct mfc_core *core, struct mfc_special_buf *fw_buf)
{
	struct mfc_dev *dev = core->dev;
	struct device_node *node = core->device->of_node;
	dma_addr_t reserved_base;
	const __be32 *prop;

	prop = of_get_property(node, "samsung,iommu-reserved-map", NULL);
	if (!prop) {
		mfc_dev_err("No reserved F/W dma area\n");
		return -ENOENT;
	}

	if (fw_buf->buftype == MFCBUF_NORMAL_FW) {
		reserved_base = of_read_number(prop, of_n_addr_cells(node));
		mfc_core_info("[F/W] %s: reserved_base is %#llx\n", fw_buf->name, reserved_base);
	} else if (fw_buf->buftype == MFCBUF_DRM_FW) {
		reserved_base = of_read_number(prop + of_n_addr_cells(node) +
				of_n_size_cells(node), of_n_addr_cells(node));
		mfc_core_info("[F/W] DRM %s: reserved_base is %#llx\n", fw_buf->name, reserved_base);
	} else {
		mfc_core_err("Wrong firmware buftype %d\n", fw_buf->buftype);
		return -EINVAL;
	}

	if (!reserved_base) {
		mfc_core_err("There is no %s firmware reserved_base\n",
				(fw_buf->buftype == MFCBUF_NORMAL_FW) ? "normal" : "drm");
		return -ENOMEM;
	}

	fw_buf->map_size = iommu_map_sg(core->domain, reserved_base,
			fw_buf->sgt->sgl, fw_buf->sgt->orig_nents,
			IOMMU_READ|IOMMU_WRITE, GFP_KERNEL);
	if (!fw_buf->map_size) {
		mfc_core_err("Failed to map iova (err VA: %#llx, PA: %#llx)\n",
				reserved_base, fw_buf->paddr);
		return -ENOMEM;
	}

	fw_buf->daddr = reserved_base;
	mfc_core_debug(2, "[MEMINFO] FW mapped iova: %#llx (pa: %#llx)\n",
			fw_buf->daddr, fw_buf->paddr);

	return 0;
}

int mfc_iommu_map_sfr(struct mfc_core *core)
{
	struct device_node *node = core->device->of_node;
	dma_addr_t reserved_base;
	const __be32 *prop;
	size_t reserved_size;
	int n_addr_cells = of_n_addr_cells(node);
	int n_size_cells = of_n_size_cells(node);
	int n_all_cells = n_addr_cells + n_size_cells;
	int i, cnt;

	prop = of_get_property(node, "samsung,iommu-identity-map", &cnt);
	if (!prop) {
		mfc_core_err("No reserved votf SFR area\n");
		return -ENOENT;
	}

	cnt /= sizeof(unsigned int);
	if (cnt % n_all_cells != 0) {
		mfc_core_err("Invalid number(%d) of values\n", cnt);
		return -EINVAL;
	}

	for (i = 0; i < cnt; i += n_all_cells) {
		reserved_base = of_read_number(prop + i, n_addr_cells);
		reserved_size = of_read_number(prop + i + n_addr_cells, n_size_cells);
		if (reserved_base == core->core_pdata->gdc_votf_base) {
			core->has_gdc_votf = 1;
			mfc_core_info("iommu mapped at GDC vOTF SFR %#llx ++ %#zx\n",
					reserved_base, reserved_size);
		} else if (reserved_base == core->core_pdata->dpu_votf_base) {
			core->has_dpu_votf = 1;
			mfc_core_info("iommu mapped at DPU vOTF SFR %#llx ++ %#zx\n",
					reserved_base, reserved_size);
		} else {
			mfc_core_err("iommu mapped at unknown SFR %#llx ++ %#zx\n",
					reserved_base, reserved_size);
			return -EINVAL;
		}
	}

	return 0;
}

void mfc_check_iova(struct mfc_dev *dev)
{
	struct mfc_platdata *pdata = dev->pdata;
	struct mfc_ctx *ctx;
	unsigned long total_iova = 0;

	if (!pdata->iova_threshold)
		return;

	/*
	 * The number of extra dpb is 8
	 * OMX: extra buffer 5, platform buffer 3
	 * Codec2: platform buffer 8
	 */
	list_for_each_entry(ctx, &dev->ctx_list, list)
		total_iova += (ctx->raw_buf.total_plane_size *
				(ctx->dpb_count + MFC_EXTRA_DPB + 3)) / 1024;

	if (total_iova > (pdata->iova_threshold * 1024))
		dev->skip_lazy_unmap = 1;
	else
		dev->skip_lazy_unmap = 0;

	mfc_dev_debug(2, "[LAZY_UNMAP] Now the IOVA for DPB is %lu/%uMB, LAZY_UNMAP %s\n",
			total_iova / 1024, pdata->iova_threshold,
			dev->skip_lazy_unmap ? "disable" : "enable");
}

int mfc_get_iovmm_from_fd(struct mfc_ctx *ctx, struct mfc_special_buf *special_buf, int fd)
{
	struct mfc_dev *dev = ctx->dev;

	special_buf->dma_buf = dma_buf_get(fd);
	if (IS_ERR(special_buf->dma_buf)) {
		mfc_ctx_err("[IOVMM] Failed to dma_buf_get (err %ld)\n",
				PTR_ERR(special_buf->dma_buf));
		special_buf->dma_buf = NULL;
		goto err_iovmm;
	}

	special_buf->attachment = dma_buf_attach(special_buf->dma_buf, dev->device);
	if (IS_ERR(special_buf->attachment)) {
		mfc_ctx_err("[IOVMM] Failed to get dma_buf_attach (err %ld)\n",
				PTR_ERR(special_buf->attachment));
		special_buf->attachment = NULL;
		goto err_iovmm;
	}

	special_buf->sgt = dma_buf_map_attachment_unlocked(special_buf->attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR(special_buf->sgt)) {
		mfc_ctx_err("[IOVMM] Failed to get sgt (err %ld)\n", PTR_ERR(special_buf->sgt));
		special_buf->sgt = NULL;
		goto err_iovmm;
	}

	special_buf->daddr = sg_dma_address(special_buf->sgt->sgl);
	if (IS_ERR_VALUE(special_buf->daddr)) {
		mfc_ctx_err("[IOVMM] Failed to get iova (err 0x%p)\n", &special_buf->daddr);
		special_buf->daddr = 0;
		goto err_iovmm;
	}

	mfc_ctx_debug(2, "[IOVMM] fd: %d addr: %#llx\n", fd, special_buf->daddr);

	return 0;

err_iovmm:
	mfc_ctx_debug(2, "[IOVMM] Failed to get iovmm from fd: %d (addr: %#llx)\n",
			fd, special_buf->daddr);

	mfc_put_iovmm_from_fd(ctx, special_buf, fd);

	return -EINVAL;
}

void mfc_put_iovmm_from_fd(struct mfc_ctx *ctx, struct mfc_special_buf *special_buf, int fd)
{
	if (special_buf->daddr)
		mfc_ctx_debug(2, "[IOVMM] fd: %d addr: %#llx\n", fd, special_buf->daddr);

	mfc_mem_dma_heap_free(special_buf);
}

MODULE_IMPORT_NS(DMA_BUF);
