/*
 * Copyright 2021 Advanced Micro Devices, Inc.
 * Copyright 2008 Jerome Glisse.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Jerome Glisse <glisse@freedesktop.org>
 */

#include <linux/file.h>
#include <linux/pagemap.h>
#include <linux/sync_file.h>
#include <linux/dma-buf.h>

#include <drm/sgpu_drm.h>
#include <drm/drm_syncobj.h>
#include <drm/ttm/ttm_tt.h>
#include "amdgpu.h"
#include "amdgpu_trace.h"
#include "amdgpu_gmc.h"
#include "amdgpu_gem.h"
#include "amdgpu_sync.h"
#include "sgpu_swap.h"

#ifdef CONFIG_DRM_SGPU_EXYNOS
#include "exynos_gpu_interface.h"
#endif /* CONFIG_DRM_SGPU_EXYNOS */

#if IS_ENABLED(CONFIG_EXYNOS_PROFILER_GPU)
#include <soc/samsung/profiler/exynos-profiler-extif.h>
#endif /* CONFIG_EXYNOS_PROFILER_GPU */

static int amdgpu_cs_parser_init(struct amdgpu_cs_parser *p,
				 struct amdgpu_device *adev,
				 struct drm_file *filp,
				 union drm_amdgpu_cs *cs)
{
	struct amdgpu_fpriv *fpriv = filp->driver_priv;

	if (cs->in.num_chunks == 0)
		return -EINVAL;

	memset(p, 0, sizeof(*p));
	p->adev = adev;
	p->filp = filp;

	p->ctx = amdgpu_ctx_get(fpriv, cs->in.ctx_id);
	if (!p->ctx)
		return -EINVAL;

	if (atomic_read(&p->ctx->guilty)) {
		amdgpu_ctx_put(p->ctx);
		return -ECANCELED;
	}
	return 0;
}

static int amdgpu_cs_job_idx(struct amdgpu_cs_parser *p,
			     struct drm_amdgpu_cs_chunk_ib *chunk_ib)
{
	struct drm_sched_entity *entity;
	unsigned int i;
	int r;

	r = amdgpu_ctx_get_entity(p->ctx, chunk_ib->ip_type,
				  chunk_ib->ip_instance,
				  chunk_ib->ring, &entity);
	if (r)
		return r;

	/*
	 * Abort if there is no run queue associated with this entity.
	 * Possibly because of disabled HW IP.
	 */
	if (entity->rq == NULL)
		return -EINVAL;

	/* Check if we can add this IB to some existing job */
	for (i = 0; i < p->gang_size; ++i)
		if (p->entities[i] == entity)
			return i;

	/* If not increase the gang size if possible */
	if (i == AMDGPU_CS_GANG_SIZE)
		return -EINVAL;

	p->entities[i] = entity;
	p->gang_size = i + 1;
	return i;
}

static int amdgpu_cs_p1_ib(struct amdgpu_cs_parser *p,
			   struct drm_amdgpu_cs_chunk_ib *chunk_ib,
			   unsigned int *num_ibs)
{
	int r;

	r = amdgpu_cs_job_idx(p, chunk_ib);
	if (r < 0)
		return r;

	++(num_ibs[r]);
	return 0;
}

static int amdgpu_cs_p1_user_fence(struct amdgpu_cs_parser *p,
				   struct drm_amdgpu_cs_chunk_fence *data,
				   uint32_t *offset)
{
	struct drm_gem_object *gobj;
	struct amdgpu_bo *bo;
	unsigned long size;
	int r;

	gobj = drm_gem_object_lookup(p->filp, data->handle);
	if (gobj == NULL)
		return -EINVAL;

	bo = amdgpu_bo_ref(gem_to_amdgpu_bo(gobj));
	p->uf_entry.priority = 0;
	p->uf_entry.tv.bo = &bo->tbo;
	/* One for TTM and one for the CS job */
	p->uf_entry.tv.num_shared = 2;

	drm_gem_object_put(gobj);

	size = amdgpu_bo_size(bo);
	if (size != PAGE_SIZE || (data->offset + 8) > size) {
		r = -EINVAL;
		goto error_unref;
	}

	if (amdgpu_ttm_tt_get_usermm(bo->tbo.ttm)) {
		r = -EINVAL;
		goto error_unref;
	}

	*offset = data->offset;

	return 0;

error_unref:
	amdgpu_bo_unref(&bo);
	p->uf_entry.tv.bo = NULL;
	return r;
}

static int amdgpu_cs_p1_bo_handles(struct amdgpu_cs_parser *p,
				   struct drm_amdgpu_bo_list_in *data)
{
	struct drm_amdgpu_bo_list_entry *info;
	int r;

	r = amdgpu_bo_create_list_entry_array(data, &info);
	if (r)
		return r;

	r = amdgpu_bo_list_create(p->adev, p->filp, info, data->bo_number,
				  &p->bo_list);
	if (r)
		goto error_free;

	kvfree(info);
	return 0;

error_free:
	if (info)
		kvfree(info);

	return r;
}

static void amdgpu_cs_update_ctx_mem_total(struct amdgpu_cs_parser *parser,
					   struct amdgpu_ctx *ctx, int idx)
{
	struct amdgpu_cs_chunk *chunk;
	struct drm_amdgpu_cs_chunk_memtrack_htile_wa *chunk_data;

	if (idx < 0 || idx >= parser->nchunks)
		return;

	chunk = &parser->chunks[idx];

	chunk_data = (struct drm_amdgpu_cs_chunk_memtrack_htile_wa *) chunk->kdata;
	if (!chunk_data)
		return;

	ctx->mem_size = chunk_data->mem_size;
}

/* Copy the data from userspace and go over it the first time */
static int amdgpu_cs_pass1(struct amdgpu_cs_parser *p,
			   union drm_amdgpu_cs *cs)
{
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	unsigned int num_ibs[AMDGPU_CS_GANG_SIZE] = { };
	struct amdgpu_vm *vm = &fpriv->vm;
	uint64_t *chunk_array_user;
	uint64_t *chunk_array;
	uint32_t uf_offset = 0;
	unsigned int size;
	int ret;
	int i;

	chunk_array = kvmalloc_array(cs->in.num_chunks, sizeof(uint64_t), GFP_KERNEL);
	if (!chunk_array)
		return -ENOMEM;

	/* get chunks */
	chunk_array_user = u64_to_user_ptr(cs->in.chunks);
	if (copy_from_user(chunk_array, chunk_array_user,
			   sizeof(uint64_t)*cs->in.num_chunks)) {
		ret = -EFAULT;
		goto free_chunk;
	}

	p->nchunks = cs->in.num_chunks;
	p->chunks = kvmalloc_array(p->nchunks, sizeof(struct amdgpu_cs_chunk),
			    GFP_KERNEL);

	if (!p->chunks) {
		ret = -ENOMEM;
		goto free_chunk;
	}

	for (i = 0; i < p->nchunks; i++) {
		struct drm_amdgpu_cs_chunk __user **chunk_ptr = NULL;
		struct drm_amdgpu_cs_chunk user_chunk;
		uint32_t __user *cdata;

		chunk_ptr = u64_to_user_ptr(chunk_array[i]);
		if (copy_from_user(&user_chunk, chunk_ptr,
				       sizeof(struct drm_amdgpu_cs_chunk))) {
			ret = -EFAULT;
			i--;
			goto free_partial_kdata;
		}
		p->chunks[i].chunk_id = user_chunk.chunk_id;
		p->chunks[i].length_dw = user_chunk.length_dw;

		size = p->chunks[i].length_dw;
		cdata = u64_to_user_ptr(user_chunk.chunk_data);

		p->chunks[i].kdata = kvmalloc_array(size, sizeof(uint32_t),
						    GFP_KERNEL);
		if (p->chunks[i].kdata == NULL) {
			ret = -ENOMEM;
			i--;
			goto free_partial_kdata;
		}
		size *= sizeof(uint32_t);
		if (copy_from_user(p->chunks[i].kdata, cdata, size)) {
			ret = -EFAULT;
			goto free_partial_kdata;
		}

		/* Assume the worst on the following checks */
		ret = -EINVAL;
		switch (p->chunks[i].chunk_id) {
		case AMDGPU_CHUNK_ID_IB:
			if (size < sizeof(struct drm_amdgpu_cs_chunk_ib))
				goto free_partial_kdata;

			ret = amdgpu_cs_p1_ib(p, p->chunks[i].kdata, num_ibs);
			if (ret)
				goto free_partial_kdata;
			break;

		case AMDGPU_CHUNK_ID_FENCE:
			if (size < sizeof(struct drm_amdgpu_cs_chunk_fence))
				goto free_partial_kdata;

			ret = amdgpu_cs_p1_user_fence(p, p->chunks[i].kdata,
						      &uf_offset);
			if (ret)
				goto free_partial_kdata;
			break;

		case AMDGPU_CHUNK_ID_BO_HANDLES:
			if (size < sizeof(struct drm_amdgpu_bo_list_in))
				goto free_partial_kdata;

			ret = amdgpu_cs_p1_bo_handles(p, p->chunks[i].kdata);
			if (ret)
				goto free_partial_kdata;
			break;

		case AMDGPU_CHUNK_ID_DEPENDENCIES:
		case AMDGPU_CHUNK_ID_SYNCOBJ_IN:
		case AMDGPU_CHUNK_ID_SYNCOBJ_OUT:
		case AMDGPU_CHUNK_ID_SCHEDULED_DEPENDENCIES:
		case AMDGPU_CHUNK_ID_SYNCOBJ_TIMELINE_WAIT:
		case AMDGPU_CHUNK_ID_SYNCOBJ_TIMELINE_SIGNAL:
		case AMDGPU_CHUNK_ID_GFX_PROFILE:
			break;

		case AMDGPU_CHUNK_ID_MEMTRACK_HTILE_WA:
			amdgpu_cs_update_ctx_mem_total(p, p->ctx, i);
			break;

		default:
			goto free_partial_kdata;
		}
	}

	if (!p->gang_size)
		return -EINVAL;

	for (i = 0; i < p->gang_size; ++i) {
		ret = amdgpu_job_alloc(p->adev, num_ibs[i], &p->jobs[i], vm);
		if (ret)
			goto free_all_kdata;

		ret = drm_sched_job_init(&p->jobs[i]->base, p->entities[i],
					 &fpriv->vm);
		if (ret)
			goto free_all_kdata;

		p->jobs[i]->ctx = p->ctx;
	}
	p->gang_leader = p->jobs[p->gang_size - 1];

	if (p->ctx->vram_lost_counter != p->gang_leader->vram_lost_counter) {
		ret = -ECANCELED;
		goto free_all_kdata;
	}

	if (p->uf_entry.tv.bo)
		p->gang_leader->uf_addr = uf_offset;

	kvfree(chunk_array);

	/* Use this opportunity to fill in task info for the vm */
	amdgpu_vm_set_task_info(vm);

	return 0;

free_all_kdata:
	i = p->nchunks - 1;
free_partial_kdata:
	for (; i >= 0; i--)
		kvfree(p->chunks[i].kdata);
	kvfree(p->chunks);
	p->chunks = NULL;
	p->nchunks = 0;
free_chunk:
	kvfree(chunk_array);

	return ret;
}

#ifdef CONFIG_DRM_SGPU_FORCE_WRITECOMBINE
#define cache_flush_for_ib(adev, vm, ib) do { } while (0)
#else
static int __vm_compare(const void *key, const struct rb_node *node)
{
	struct amdgpu_bo_va_mapping *mapping = container_of(node, struct amdgpu_bo_va_mapping, rb);
	uint64_t addr = *(uint64_t *)key;

	if (addr < mapping->start)
		return -1;
	if (addr > mapping->last)
		return 1;
	return 0;
}

static void cache_flush_for_ib(struct amdgpu_device *adev,
			       struct amdgpu_vm *vm, struct amdgpu_ib *ib)
{
	uint64_t addr = ib->gpu_addr >> PAGE_SHIFT;
	struct amdgpu_bo_va_mapping *mapping;
	struct rb_node *node;
	uint64_t base, end;

	if (sgpu_is_dma_coherent(adev))
		return;

	node = rb_find_first(&addr, &vm->va.rb_root, __vm_compare);
	if (!node) {
		DRM_ERROR("%s: IB @ %#llx / %#x bytes Not Found!\n",
			  __func__, ib->gpu_addr, ib->length_dw * 4);
		return;
	}

	mapping = container_of(node, struct amdgpu_bo_va_mapping, rb);

	base = mapping->start << PAGE_SHIFT;
	end = (mapping->last + 1) << PAGE_SHIFT;

	if ((ib->gpu_addr + ib->length_dw * 4) >= end) {
		DRM_ERROR("%s: IB @ %#llx / %#x bytes out of range [%#llx ~ %#llx]\n",
			  __func__, ib->gpu_addr, ib->length_dw * 4, base, end);
		return;
	}

	amdgpu_ttm_cache_sync(adev, mapping->bo_va->base.bo, DMA_TO_DEVICE,
			      true, ib->gpu_addr - base, ib->length_dw * 4);

}
#endif /* CONFIG_DRM_SGPU_FORCE_WRITECOMBINE */

static int amdgpu_cs_p2_ib(struct amdgpu_cs_parser *p,
			   struct amdgpu_cs_chunk *chunk,
			   unsigned int *ce_preempt,
			   unsigned int *de_preempt)
{
	struct drm_amdgpu_cs_chunk_ib *chunk_ib = chunk->kdata;
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	struct amdgpu_vm *vm = &fpriv->vm;
	struct amdgpu_ring *ring;
	struct amdgpu_job *job;
	struct amdgpu_ib *ib;
	int r;

	r = amdgpu_cs_job_idx(p, chunk_ib);
	if (r < 0)
		return r;

	job = p->jobs[r];
	ring = amdgpu_job_ring(job);
	ib = &job->ibs[job->num_ibs++];

	/* MM engine doesn't support user fences */
	if (p->uf_entry.tv.bo && ring->funcs->no_user_fence)
		return -EINVAL;

	if (chunk_ib->ip_type == AMDGPU_HW_IP_GFX &&
	    chunk_ib->flags & AMDGPU_IB_FLAG_PREEMPT) {
		if (chunk_ib->flags & AMDGPU_IB_FLAG_CE)
			(*ce_preempt)++;
		else
			(*de_preempt)++;

		/* Each GFX command submit allows only 1 IB max
		 * preemptible for CE & DE */
		if (*ce_preempt > 1 || *de_preempt > 1)
			return -EINVAL;
	}

	if (chunk_ib->flags & AMDGPU_IB_FLAG_PREAMBLE)
		job->preamble_status |= AMDGPU_PREAMBLE_IB_PRESENT;

	r =  amdgpu_ib_get(p->adev, vm, ring->funcs->parse_cs ?
			   chunk_ib->ib_bytes : 0,
			   AMDGPU_IB_POOL_DELAYED, ib);
	if (r) {
		DRM_ERROR("Failed to get ib !\n");
		return r;
	}

	ib->gpu_addr = chunk_ib->va_start;
	ib->length_dw = chunk_ib->ib_bytes / 4;
	ib->flags = chunk_ib->flags;

	cache_flush_for_ib(p->adev, vm, ib);

	return 0;
}

static int amdgpu_cs_p2_dependencies(struct amdgpu_cs_parser *p,
				     struct amdgpu_cs_chunk *chunk)
{
	struct drm_amdgpu_cs_chunk_dep *deps = chunk->kdata;
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	unsigned num_deps;
	int i, r;

	num_deps = chunk->length_dw * 4 /
		sizeof(struct drm_amdgpu_cs_chunk_dep);

	for (i = 0; i < num_deps; ++i) {
		struct amdgpu_ctx *ctx;
		struct drm_sched_entity *entity;
		struct dma_fence *fence;

		ctx = amdgpu_ctx_get(fpriv, deps[i].ctx_id);
		if (ctx == NULL)
			return -EINVAL;

		r = amdgpu_ctx_get_entity(ctx, deps[i].ip_type,
					  deps[i].ip_instance,
					  deps[i].ring, &entity);
		if (r) {
			amdgpu_ctx_put(ctx);
			return r;
		}

		fence = amdgpu_ctx_get_fence(ctx, entity, deps[i].handle);
		amdgpu_ctx_put(ctx);

		if (IS_ERR(fence))
			return PTR_ERR(fence);
		else if (!fence)
			continue;

		if (chunk->chunk_id == AMDGPU_CHUNK_ID_SCHEDULED_DEPENDENCIES) {
			struct drm_sched_fence *s_fence;
			struct dma_fence *old = fence;

			s_fence = to_drm_sched_fence(fence);
			fence = dma_fence_get(&s_fence->scheduled);
			dma_fence_put(old);
		}

		r = amdgpu_sync_fence(&p->gang_leader->sync, fence);
		dma_fence_put(fence);
		if (r)
			return r;
	}
	return 0;
}

static int amdgpu_syncobj_lookup_and_add(struct amdgpu_cs_parser *p,
					 uint32_t handle, u64 point,
					 u64 flags)
{
	struct dma_fence *fence;
	int r;

	r = drm_syncobj_find_fence(p->filp, handle, point, flags, &fence);
	if (r) {
		DRM_ERROR("syncobj %u failed to find fence @ %llu (%d)!\n",
			  handle, point, r);
		return r;
	}

	r = amdgpu_sync_fence(&p->gang_leader->sync, fence);
	dma_fence_put(fence);

	return r;
}

static int amdgpu_cs_p2_syncobj_in(struct amdgpu_cs_parser *p,
				   struct amdgpu_cs_chunk *chunk)
{
	struct drm_amdgpu_cs_chunk_sem *deps = chunk->kdata;
	unsigned num_deps;
	int i, r;

	num_deps = chunk->length_dw * 4 /
		sizeof(struct drm_amdgpu_cs_chunk_sem);
	for (i = 0; i < num_deps; ++i) {
		r = amdgpu_syncobj_lookup_and_add(p, deps[i].handle, 0, 0);
		if (r)
			return r;
	}

	return 0;
}

static int amdgpu_cs_p2_syncobj_timeline_wait(struct amdgpu_cs_parser *p,
					      struct amdgpu_cs_chunk *chunk)
{
	struct drm_amdgpu_cs_chunk_syncobj *syncobj_deps = chunk->kdata;
	unsigned num_deps;
	int i, r;

	num_deps = chunk->length_dw * 4 /
		sizeof(struct drm_amdgpu_cs_chunk_syncobj);
	for (i = 0; i < num_deps; ++i) {
		r = amdgpu_syncobj_lookup_and_add(p, syncobj_deps[i].handle,
						  syncobj_deps[i].point,
						  syncobj_deps[i].flags);
		if (r)
			return r;
	}

	return 0;
}

static int amdgpu_cs_p2_syncobj_out(struct amdgpu_cs_parser *p,
				    struct amdgpu_cs_chunk *chunk)
{
	struct drm_amdgpu_cs_chunk_sem *deps = chunk->kdata;
	unsigned num_deps;
	int i;

	num_deps = chunk->length_dw * 4 /
		sizeof(struct drm_amdgpu_cs_chunk_sem);

	if (p->post_deps)
		return -EINVAL;

	p->post_deps = kmalloc_array(num_deps, sizeof(*p->post_deps),
				     GFP_KERNEL);
	p->num_post_deps = 0;

	if (!p->post_deps)
		return -ENOMEM;

	for (i = 0; i < num_deps; ++i) {
		p->post_deps[i].syncobj =
			drm_syncobj_find(p->filp, deps[i].handle);
		if (!p->post_deps[i].syncobj)
			return -EINVAL;
		p->post_deps[i].chain = NULL;
		p->post_deps[i].point = 0;
		p->num_post_deps++;
	}

	return 0;
}

static int amdgpu_cs_p2_syncobj_timeline_signal(struct amdgpu_cs_parser *p,
						struct amdgpu_cs_chunk *chunk)
{
	struct drm_amdgpu_cs_chunk_syncobj *syncobj_deps = chunk->kdata;
	unsigned num_deps;
	int i;

	num_deps = chunk->length_dw * 4 /
		sizeof(struct drm_amdgpu_cs_chunk_syncobj);

	if (p->post_deps)
		return -EINVAL;

	p->post_deps = kmalloc_array(num_deps, sizeof(*p->post_deps),
				     GFP_KERNEL);
	p->num_post_deps = 0;

	if (!p->post_deps)
		return -ENOMEM;

	for (i = 0; i < num_deps; ++i) {
		struct amdgpu_cs_post_dep *dep = &p->post_deps[i];

		dep->chain = NULL;
		if (syncobj_deps[i].point) {
			dep->chain = dma_fence_chain_alloc();
			if (!dep->chain)
				return -ENOMEM;
		}

		dep->syncobj = drm_syncobj_find(p->filp,
						syncobj_deps[i].handle);
		if (!dep->syncobj) {
			dma_fence_chain_free(dep->chain);
			return -EINVAL;
		}
		dep->point = syncobj_deps[i].point;
		p->num_post_deps++;
	}

	return 0;
}

static int amdgpu_cs_pass2(struct amdgpu_cs_parser *p)
{
	unsigned int ce_preempt = 0, de_preempt = 0;
	int i, r;

	for (i = 0; i < p->nchunks; ++i) {
		struct amdgpu_cs_chunk *chunk;

		chunk = &p->chunks[i];

		switch (chunk->chunk_id) {
		case AMDGPU_CHUNK_ID_IB:
			r = amdgpu_cs_p2_ib(p, chunk, &ce_preempt, &de_preempt);
			if (r)
				return r;
			break;
		case AMDGPU_CHUNK_ID_DEPENDENCIES:
		case AMDGPU_CHUNK_ID_SCHEDULED_DEPENDENCIES:
			r = amdgpu_cs_p2_dependencies(p, chunk);
			if (r)
				return r;
			break;
		case AMDGPU_CHUNK_ID_SYNCOBJ_IN:
			r = amdgpu_cs_p2_syncobj_in(p, chunk);
			if (r)
				return r;
			break;
		case AMDGPU_CHUNK_ID_SYNCOBJ_OUT:
			r = amdgpu_cs_p2_syncobj_out(p, chunk);
			if (r)
				return r;
			break;
		case AMDGPU_CHUNK_ID_SYNCOBJ_TIMELINE_WAIT:
			r = amdgpu_cs_p2_syncobj_timeline_wait(p, chunk);
			if (r)
				return r;
			break;
		case AMDGPU_CHUNK_ID_SYNCOBJ_TIMELINE_SIGNAL:
			r = amdgpu_cs_p2_syncobj_timeline_signal(p, chunk);
			if (r)
				return r;
			break;
		}
	}

	return 0;
}

/* Convert microseconds to bytes. */
static u64 us_to_bytes(struct amdgpu_device *adev, s64 us)
{
	if (us <= 0 || !adev->mm_stats.log2_max_MBps)
		return 0;

	/* Since accum_us is incremented by a million per second, just
	 * multiply it by the number of MB/s to get the number of bytes.
	 */
	return us << adev->mm_stats.log2_max_MBps;
}

static s64 bytes_to_us(struct amdgpu_device *adev, u64 bytes)
{
	if (!adev->mm_stats.log2_max_MBps)
		return 0;

	return bytes >> adev->mm_stats.log2_max_MBps;
}

/* Returns how many bytes TTM can move right now. If no bytes can be moved,
 * it returns 0. If it returns non-zero, it's OK to move at least one buffer,
 * which means it can go over the threshold once. If that happens, the driver
 * will be in debt and no other buffer migrations can be done until that debt
 * is repaid.
 *
 * This approach allows moving a buffer of any size (it's important to allow
 * that).
 *
 * The currency is simply time in microseconds and it increases as the clock
 * ticks. The accumulated microseconds (us) are converted to bytes and
 * returned.
 */
static void amdgpu_cs_get_threshold_for_moves(struct amdgpu_device *adev,
					      u64 *max_bytes,
					      u64 *max_vis_bytes)
{
	s64 time_us, increment_us;
	/* Allow a maximum of 200 accumulated ms. This is basically per-IB
	 * throttling.
	 *
	 * It means that in order to get full max MBps, at least 5 IBs per
	 * second must be submitted and not more than 200ms apart from each
	 * other.
	 */
	const s64 us_upper_bound = 200000;

	if (!adev->mm_stats.log2_max_MBps) {
		*max_bytes = 0;
		*max_vis_bytes = 0;
		return;
	}


	spin_lock(&adev->mm_stats.lock);

	/* Increase the amount of accumulated us. */
	time_us = ktime_to_us(ktime_get());
	increment_us = time_us - adev->mm_stats.last_update_us;
	adev->mm_stats.last_update_us = time_us;
	adev->mm_stats.accum_us = min(adev->mm_stats.accum_us + increment_us,
                                      us_upper_bound);

	/* This is set to 0 if the driver is in debt to disallow (optional)
	 * buffer moves.
	 */
	*max_bytes = us_to_bytes(adev, adev->mm_stats.accum_us);

	*max_vis_bytes = 0;

	spin_unlock(&adev->mm_stats.lock);
}

/* Report how many bytes have really been moved for the last command
 * submission. This can result in a debt that can stop buffer migrations
 * temporarily.
 */
void amdgpu_cs_report_moved_bytes(struct amdgpu_device *adev, u64 num_bytes,
				  u64 num_vis_bytes)
{
	spin_lock(&adev->mm_stats.lock);
	adev->mm_stats.accum_us -= bytes_to_us(adev, num_bytes);
	adev->mm_stats.accum_us_vis -= bytes_to_us(adev, num_vis_bytes);
	spin_unlock(&adev->mm_stats.lock);
}

static int amdgpu_cs_bo_validate(struct amdgpu_cs_parser *p,
				 struct amdgpu_bo *bo)
{
	struct amdgpu_device *adev = amdgpu_ttm_adev(bo->tbo.bdev);
	struct ttm_operation_ctx ctx = {
		.interruptible = true,
		.no_wait_gpu = false,
		.resv = bo->tbo.base.resv
	};
	uint32_t domain;
	int r;

	if (bo->tbo.pin_count)
		return 0;

	/* Don't move this buffer if we have depleted our allowance
	 * to move it. Don't move anything if the threshold is zero.
	 */
	if (p->bytes_moved < p->bytes_moved_threshold &&
	    (!bo->tbo.base.dma_buf ||
	    list_empty(&bo->tbo.base.dma_buf->attachments))) {
		if (!amdgpu_gmc_vram_full_visible(&adev->gmc) &&
		    (bo->flags & AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED)) {
			/* And don't move a CPU_ACCESS_REQUIRED BO to limited
			 * visible VRAM if we've depleted our allowance to do
			 * that.
			 */
			if (p->bytes_moved_vis < p->bytes_moved_vis_threshold)
				domain = bo->preferred_domains;
			else
				domain = bo->allowed_domains;
		} else {
			domain = bo->preferred_domains;
		}
	} else {
		domain = bo->allowed_domains;
	}

retry:
	amdgpu_bo_placement_from_domain(bo, domain);
	r = ttm_bo_validate(&bo->tbo, &bo->placement, &ctx);

	p->bytes_moved += ctx.bytes_moved;

	if (unlikely(r == -ENOMEM) && domain != bo->allowed_domains) {
		domain = bo->allowed_domains;
		goto retry;
	}

	return r;
}

static int amdgpu_cs_validate(void *param, struct amdgpu_bo *bo)
{
	struct amdgpu_cs_parser *p = param;
	int r;

	r = amdgpu_cs_bo_validate(p, bo);
	if (r)
		return r;

	if (bo->shadow)
		r = amdgpu_cs_bo_validate(p, bo->shadow);

	return r;
}

static int amdgpu_cs_list_validate(struct amdgpu_cs_parser *p,
			    struct list_head *validated)
{
	struct ttm_operation_ctx ctx = { true, false };
	struct amdgpu_bo_list_entry *lobj;
	int r;

	list_for_each_entry(lobj, validated, tv.head) {
		struct amdgpu_bo *bo = ttm_to_amdgpu_bo(lobj->tv.bo);
		struct mm_struct *usermm;

		usermm = amdgpu_ttm_tt_get_usermm(bo->tbo.ttm);
		if (usermm && usermm != current->mm)
			return -EPERM;

		if (amdgpu_ttm_tt_is_userptr(bo->tbo.ttm) &&
		    lobj->user_invalidated && lobj->user_pages) {
			amdgpu_bo_placement_from_domain(bo,
							AMDGPU_GEM_DOMAIN_CPU);
			r = ttm_bo_validate(&bo->tbo, &bo->placement, &ctx);
			if (r)
				return r;

			amdgpu_ttm_tt_set_user_pages(bo->tbo.ttm,
						     lobj->user_pages);
		}

		r = amdgpu_cs_validate(p, bo);
		if (r)
			return r;

		kvfree(lobj->user_pages);
		lobj->user_pages = NULL;
	}
	return 0;
}

static int amdgpu_cs_parser_bos(struct amdgpu_cs_parser *p,
				union drm_amdgpu_cs *cs)
{
	struct amdgpu_device *adev = p->adev;
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	struct amdgpu_vm *vm = &fpriv->vm;
	struct amdgpu_bo_list_entry *e;
	struct list_head duplicates;
	unsigned int i;
	int r;

	INIT_LIST_HEAD(&p->validated);

	/* p->bo_list could already be assigned if AMDGPU_CHUNK_ID_BO_HANDLES is present */
	if (cs->in.bo_list_handle) {
		if (p->bo_list)
			return -EINVAL;

		r = amdgpu_bo_list_get(fpriv, cs->in.bo_list_handle,
				       &p->bo_list);
		if (r)
			return r;
	} else if (!p->bo_list) {
		/* Create a empty bo_list when no handle is provided */
		r = amdgpu_bo_list_create(p->adev, p->filp, NULL, 0,
					  &p->bo_list);
		if (r)
			return r;
	}
	SGPU_LOG(adev, DMSG_INFO, DMSG_ETC, "bo_num=%d", p->bo_list->num_entries);

	/* One for TTM and one for the CS job */
	amdgpu_bo_list_for_each_entry(e, p->bo_list)
		e->tv.num_shared = 2;

	amdgpu_bo_list_get_list(p->bo_list, &p->validated);

	INIT_LIST_HEAD(&duplicates);
	amdgpu_vm_get_pd_bo(&fpriv->vm, &p->validated, &p->vm_pd);

	if (amdgpu_mcbp) {
		p->csa_entry.tv.bo = &adev->csa_obj->tbo;
		p->csa_entry.tv.num_shared = 1;
		list_add(&p->csa_entry.tv.head, &p->validated);
	}

	if (p->uf_entry.tv.bo && !ttm_to_amdgpu_bo(p->uf_entry.tv.bo)->parent)
		list_add(&p->uf_entry.tv.head, &p->validated);

	/* Get userptr backing pages. If pages are updated after registered
	 * in amdgpu_gem_userptr_ioctl(), amdgpu_cs_list_validate() will do
	 * amdgpu_ttm_backend_bind() to flush and invalidate new pages
	 */
	r = 0;
	amdgpu_bo_list_for_each_userptr_entry(e, p->bo_list) {
		struct amdgpu_bo *bo = ttm_to_amdgpu_bo(e->tv.bo);
		bool userpage_invalidated = false;
		int i;
		long pinned;
		struct amdgpu_bo_va *bo_va;
		struct amdgpu_bo_va_mapping *mapping;
		unsigned long num_of_pages;

		DRM_DEBUG("%s mem type %x domain allow %x prefer %x\n",
			  __func__, bo->tbo.resource->mem_type,
			  bo->allowed_domains, bo->preferred_domains);
		bo_va = amdgpu_vm_bo_find(vm, bo);
		if (bo_va) {
			list_for_each_entry(mapping, &bo_va->valids, list)
				DRM_DEBUG("%s start %llx end %llx\n", __func__,
					 mapping->start, mapping->last);
		}

		e->user_pages = kvmalloc_array(bo->tbo.ttm->num_pages,
					       sizeof(struct page *),
					       GFP_KERNEL | __GFP_ZERO);
		if (!e->user_pages) {
			DRM_ERROR("calloc failure\n");
			return -ENOMEM;
		}

		pinned = pin_user_pages_fast(amdgpu_ttm_tt_get_start_addr(bo->tbo.ttm),
					     bo->tbo.ttm->num_pages,
					     FOLL_WRITE | FOLL_LONGTERM,
					     e->user_pages);
		DRM_DEBUG("pin_user_pages_fast..cs bo:0x%p ttm:0x%p add:%lx :%ld %d\n",
			  bo, bo->tbo.ttm, amdgpu_ttm_tt_get_start_addr(bo->tbo.ttm),
			  pinned, bo->tbo.ttm->num_pages);
		if (pinned != bo->tbo.ttm->num_pages) {
			for (i = 0; i < pinned; i++)
				unpin_user_page(e->user_pages[i]);
			DRM_DEBUG("pin_user_pages_fast failed :%ld %d\n",
				  pinned, bo->tbo.ttm->num_pages);
			kvfree(e->user_pages);
			e->user_pages = NULL;
			return -ENOMEM;
		}

		for (i = 0; i < bo->tbo.ttm->num_pages; i++) {
			if (bo->tbo.ttm->pages[i] != e->user_pages[i]) {
				num_of_pages = bo->tbo.ttm->num_pages;
				userpage_invalidated = true;

				while(num_of_pages--)
					unpin_user_page(bo->tbo.ttm->pages[num_of_pages]);
				break;
			}
		}
		e->user_invalidated = userpage_invalidated;
		if (!userpage_invalidated) {
			num_of_pages = bo->tbo.ttm->num_pages;

			DRM_DEBUG("unpin_user_pages bo:0x%p, ttm:0x%p userpage_invalidated:%x.\n",
				  bo, bo->tbo.ttm, userpage_invalidated);

			while(num_of_pages--)
				unpin_user_page(e->user_pages[num_of_pages]);
		}
	}
	r = ttm_eu_reserve_buffers(&p->ticket, &p->validated, true,
				   &duplicates);
	if (unlikely(r != 0)) {
		if (r != -ERESTARTSYS)
			DRM_ERROR("ttm_eu_reserve_buffers failed.\n");
		goto out;
	}

	amdgpu_bo_list_for_each_entry(e, p->bo_list) {
		struct amdgpu_bo *bo = ttm_to_amdgpu_bo(e->tv.bo);

		e->bo_va = amdgpu_vm_bo_find(vm, bo);
	}

	amdgpu_cs_get_threshold_for_moves(p->adev, &p->bytes_moved_threshold,
					  &p->bytes_moved_vis_threshold);
	p->bytes_moved = 0;
	p->bytes_moved_vis = 0;

	r = amdgpu_vm_validate_pt_bos(p->adev, &fpriv->vm,
				      amdgpu_cs_validate, p);
	if (r) {
		DRM_ERROR("amdgpu_vm_validate_pt_bos() failed.\n");
		goto error_validate;
	}

	r = amdgpu_cs_list_validate(p, &duplicates);
	if (r)
		goto error_validate;

	r = amdgpu_cs_list_validate(p, &p->validated);
	if (r)
		goto error_validate;

	if (p->uf_entry.tv.bo) {
		struct amdgpu_bo *uf = ttm_to_amdgpu_bo(p->uf_entry.tv.bo);

		r = amdgpu_ttm_alloc_gart(&uf->tbo);
		if (r)
			goto error_validate;

		p->gang_leader->uf_addr += amdgpu_bo_gpu_offset(uf);
	}

	amdgpu_cs_report_moved_bytes(p->adev, p->bytes_moved,
				     p->bytes_moved_vis);

	for (i = 0; i < p->gang_size; ++i)
		amdgpu_job_set_resources(p->jobs[i], p->bo_list->gds_obj,
					 p->bo_list->gws_obj,
					 p->bo_list->oa_obj);
	return 0;

error_validate:
	ttm_eu_backoff_reservation(&p->ticket, &p->validated);

out:
	return r;
}

static void trace_amdgpu_cs_ibs(struct amdgpu_cs_parser *p)
{
	int i, j;

	if (!trace_amdgpu_cs_enabled())
		return;

	for (i = 0; i < p->gang_size; ++i) {
		struct amdgpu_job *job = p->jobs[i];

		for (j = 0; j < job->num_ibs; ++j)
			trace_amdgpu_cs(p, job, &job->ibs[j]);
	}
}

#if IS_ENABLED(CONFIG_EXYNOS_PROFILER_GPU)
static int amdgpu_cs_user_time(struct amdgpu_cs_parser *p)
{
	int r = 0;
	int i, j;

	for (i = 0; i < p->nchunks; i++) {
		struct amdgpu_cs_chunk *chunk;

		chunk = &p->chunks[i];

		if (chunk->chunk_id == AMDGPU_CHUNK_ID_GFX_PROFILE) {
			for (j = 0; j < p->gang_size; ++j)
				p->jobs[j]->end_of_frame = true;

			sgpu_profiler_update_interframe_sw(chunk->length_dw, chunk->kdata);

			/* first data is valid */
			break;
		}
	}

	return r;
}
#else
static inline int amdgpu_cs_user_time(struct amdgpu_cs_parser *p)
{
	return 0;
}
#endif

static int amdgpu_cs_patch_ibs(struct amdgpu_cs_parser *p,
			       struct amdgpu_job *job)
{
	struct amdgpu_ring *ring = amdgpu_job_ring(job);
	unsigned int i;
	int r;

	/* Only for UVD/VCE VM emulation */
	if (!ring->funcs->parse_cs && !ring->funcs->patch_cs_in_place)
		return 0;

	for (i = 0; i < job->num_ibs; ++i) {
		struct amdgpu_ib *ib = &job->ibs[i];
		struct amdgpu_bo_va_mapping *m;
		struct amdgpu_bo *aobj;
		uint64_t va_start;
		uint8_t *kptr;

		va_start = ib->gpu_addr & AMDGPU_GMC_HOLE_MASK;
		r = amdgpu_cs_find_mapping(p, va_start, &aobj, &m);
		if (r) {
			DRM_ERROR("IB va_start is invalid\n");
			return r;
		}

		if ((va_start + ib->length_dw * 4) >
		    (m->last + 1) * AMDGPU_GPU_PAGE_SIZE) {
			DRM_ERROR("IB va_start+ib_bytes is invalid\n");
			return -EINVAL;
		}
		/* the IB should be reserved at this point */
		r = amdgpu_bo_kmap(aobj, (void **)&kptr);
		if (r) {
			return r;
		}

		kptr += va_start - (m->start * AMDGPU_GPU_PAGE_SIZE);

		if (ring->funcs->parse_cs) {
			memcpy(ib->ptr, kptr, ib->length_dw * 4);
			amdgpu_bo_kunmap(aobj);

			r = amdgpu_ring_parse_cs(ring, p, job, ib);
			if (r)
				return r;
		} else {
			ib->ptr = (uint32_t *)kptr;
			r = amdgpu_ring_patch_cs_in_place(ring, p, job, ib);
			amdgpu_bo_kunmap(aobj);
			if (r)
				return r;
		}
	}

	return 0;
}

static int amdgpu_cs_patch_jobs(struct amdgpu_cs_parser *p)
{
	unsigned int i;
	int r;

	for (i = 0; i < p->gang_size; ++i) {
		r = amdgpu_cs_patch_ibs(p, p->jobs[i]);
		if (r)
			return r;
	}
	return 0;
}

static int amdgpu_cs_vm_handling(struct amdgpu_cs_parser *p)
{
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	struct amdgpu_job *job = p->gang_leader;
	struct amdgpu_device *adev = p->adev;
	struct amdgpu_vm *vm = &fpriv->vm;
	struct amdgpu_bo_list_entry *e;
	struct amdgpu_bo_va *bo_va;
	struct amdgpu_bo *bo;
	unsigned int i;
	int r;

	r = amdgpu_vm_clear_freed(adev, vm, NULL);
	if (r)
		return r;

	r = amdgpu_vm_bo_update(adev, fpriv->prt_va, false);
	if (r)
		return r;

	r = amdgpu_sync_fence(&job->sync, fpriv->prt_va->last_pt_update);
	if (r)
		return r;

	if (fpriv->csa_va) {
		bo_va = fpriv->csa_va;
		BUG_ON(!bo_va);
		r = amdgpu_vm_bo_update(adev, bo_va, false);
		if (r)
			return r;

		r = amdgpu_sync_fence(&job->sync, bo_va->last_pt_update);
		if (r)
			return r;
	}

	amdgpu_bo_list_for_each_entry(e, p->bo_list) {
		/* ignore duplicates */
		bo = ttm_to_amdgpu_bo(e->tv.bo);
		if (!bo)
			continue;

		bo_va = e->bo_va;
		if (bo_va == NULL)
			continue;

		r = amdgpu_vm_bo_update(adev, bo_va, false);
		if (r)
			return r;

		r = amdgpu_sync_fence(&job->sync, bo_va->last_pt_update);
		if (r)
			return r;
	}

	r = amdgpu_vm_handle_moved(adev, vm);
	if (r)
		return r;

	r = amdgpu_vm_update_pdes(adev, vm, false);
	if (r)
		return r;

	r = amdgpu_sync_fence(&job->sync, vm->last_update);
	if (r)
		return r;

	for (i = 0; i < p->gang_size; ++i) {
		job = p->jobs[i];

		if (!job->vm)
			continue;

		job->vm_pd_addr = amdgpu_gmc_pd_addr(vm->root.base.bo);
	}

	if (amdgpu_vm_debug) {
		/* Invalidate all BOs to test for userspace bugs */
		amdgpu_bo_list_for_each_entry(e, p->bo_list) {
			struct amdgpu_bo *bo = ttm_to_amdgpu_bo(e->tv.bo);

			/* ignore duplicates */
			if (!bo)
				continue;
			amdgpu_vm_bo_invalidate(adev, bo, false);
		}
	}

	return 0;
}

static int amdgpu_cs_sync_rings(struct amdgpu_cs_parser *p)
{
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	struct amdgpu_job *leader = p->gang_leader;
	struct amdgpu_bo_list_entry *e;
	unsigned int i;
	int r;

	list_for_each_entry(e, &p->validated, tv.head) {
		struct amdgpu_bo *bo = ttm_to_amdgpu_bo(e->tv.bo);
		struct dma_resv *resv = bo->tbo.base.resv;
		enum amdgpu_sync_mode sync_mode;

		sync_mode = amdgpu_bo_explicit_sync(bo) ?
			AMDGPU_SYNC_EXPLICIT : AMDGPU_SYNC_NE_OWNER;
		r = amdgpu_sync_resv(p->adev, &leader->sync, resv, sync_mode,
				     &fpriv->vm);
		if (r)
			return r;
	}

	for (i = 0; i < p->gang_size - 1; ++i) {
		r = amdgpu_sync_clone(&leader->sync, &p->jobs[i]->sync);
		if (r)
			return r;
	}

	r = amdgpu_ctx_wait_prev_fence(p->ctx, p->entities[p->gang_size - 1]);
	if (r && r != -ERESTARTSYS)
		DRM_ERROR("amdgpu_ctx_wait_prev_fence failed.\n");

	return r;
}

static void amdgpu_cs_post_dependencies(struct amdgpu_cs_parser *p)
{
	int i;

	for (i = 0; i < p->num_post_deps; ++i) {
		if (p->post_deps[i].chain && p->post_deps[i].point) {
			drm_syncobj_add_point(p->post_deps[i].syncobj,
					      p->post_deps[i].chain,
					      p->fence, p->post_deps[i].point);
			p->post_deps[i].chain = NULL;
		} else {
			drm_syncobj_replace_fence(p->post_deps[i].syncobj,
						  p->fence);
		}
	}
}

static void sgpu_eu_fence_buffer_objects(struct ww_acquire_ctx *ticket, struct list_head *list,
					 struct dma_fence *fence, struct ttm_buffer_object *vm_bo)
{
	struct ttm_validate_buffer *entry;

	if (list_empty(list))
		return;

	list_for_each_entry(entry, list, head) {
		struct ttm_buffer_object *bo = entry->bo;
		/*
		 * sgpu_eu_fence_buffer_objects() does exactly the same as
                 * ttm_eu_fence_buffer_objects() except skipping fence register for root buffer
                 * object of the current GPUVM. It is not to block GPUVM update until the
                 * current job from the same GPUVM is done.
		 */
		if (bo != vm_bo)
			dma_resv_add_fence(bo->base.resv, fence, DMA_RESV_USAGE_WRITE);

		ttm_bo_move_to_lru_tail_unlocked(bo);
		dma_resv_unlock(bo->base.resv);
	}

	if (ticket)
		ww_acquire_fini(ticket);
}

static int amdgpu_cs_submit(struct amdgpu_cs_parser *p,
			    union drm_amdgpu_cs *cs)
{
	struct amdgpu_fpriv *fpriv = p->filp->driver_priv;
	struct amdgpu_job *leader = p->gang_leader;
	struct amdgpu_bo_list_entry *e;
	unsigned int i;
	uint64_t seq;
	int r;

	for (i = 0; i < p->gang_size; ++i) {
		drm_sched_job_arm(&p->jobs[i]->base);
		p->jobs[i]->ifh_mode = p->ctx->ifh_mode;
	}

	for (i = 0; i < (p->gang_size - 1); ++i) {
		struct dma_fence *fence;

		fence = &p->jobs[i]->base.s_fence->scheduled;
		r = amdgpu_sync_fence(&leader->sync, fence);
		if (r)
			goto error_cleanup;
	}

	if (p->gang_size > 1) {
		for (i = 0; i < p->gang_size; ++i)
			amdgpu_job_set_gang_leader(p->jobs[i], leader);
	}

	/* No memory allocation is allowed while holding the notifier lock.
	 * The lock is held until amdgpu_cs_submit is finished and fence is
	 * added to BOs.
	 */
	mutex_lock(&p->adev->notifier_lock);

	p->fence = dma_fence_get(&leader->base.s_fence->finished);
	list_for_each_entry(e, &p->validated, tv.head) {

		/* Everybody except for the gang leader uses READ */
		for (i = 0; i < (p->gang_size - 1); ++i) {
			dma_resv_add_fence(e->tv.bo->base.resv,
					   &p->jobs[i]->base.s_fence->finished,
					   DMA_RESV_USAGE_READ);
		}
	}

	amdgpu_ctx_add_fence(p->ctx, p->entities[p->gang_size - 1], p->fence, &seq);
	amdgpu_cs_post_dependencies(p);

	if ((leader->preamble_status & AMDGPU_PREAMBLE_IB_PRESENT) &&
	    !p->ctx->preamble_presented) {
		leader->preamble_status |= AMDGPU_PREAMBLE_IB_PRESENT_FIRST;
		p->ctx->preamble_presented = true;
	}

	cs->out.handle = seq;
	leader->uf_sequence = seq;

	amdgpu_vm_bo_trace_cs(&fpriv->vm, &p->ticket);
	for (i = 0; i < p->gang_size; ++i) {
		amdgpu_job_free_resources(p->jobs[i]);
		trace_amdgpu_cs_ioctl(p->jobs[i]);
		drm_sched_entity_push_job(&p->jobs[i]->base);
		p->jobs[i] = NULL;
	}

	amdgpu_vm_move_to_lru_tail(p->adev, &fpriv->vm);

	if (trace_sgpu_job_dependency_enabled())
		sgpu_sync_trace_fence(&leader->sync);

	sgpu_eu_fence_buffer_objects(&p->ticket, &p->validated, p->fence,
				     &fpriv->vm.root.base.bo->tbo);

	mutex_unlock(&p->adev->notifier_lock);

	return 0;

error_cleanup:
	for (i = 0; i < p->gang_size; ++i)
		drm_sched_job_cleanup(&p->jobs[i]->base);
	return r;
}

/* Cleanup the parser structure */
static void amdgpu_cs_parser_fini(struct amdgpu_cs_parser *parser)
{
	unsigned i;

	for (i = 0; i < parser->num_post_deps; i++) {
		drm_syncobj_put(parser->post_deps[i].syncobj);
		kfree(parser->post_deps[i].chain);
	}
	kfree(parser->post_deps);

	dma_fence_put(parser->fence);

	if (parser->ctx)
		amdgpu_ctx_put(parser->ctx);
	if (parser->bo_list)
		amdgpu_bo_list_put(parser->bo_list);

	for (i = 0; i < parser->nchunks; i++)
		kvfree(parser->chunks[i].kdata);
	kvfree(parser->chunks);
	for (i = 0; i < parser->gang_size; ++i) {
		if (parser->jobs[i])
			amdgpu_job_free(parser->jobs[i]);
	}
	if (parser->uf_entry.tv.bo) {
		struct amdgpu_bo *uf = ttm_to_amdgpu_bo(parser->uf_entry.tv.bo);

		amdgpu_bo_unref(&uf);
	}
}

int amdgpu_cs_ioctl(struct drm_device *dev, void *data, struct drm_file *filp)
{
	struct amdgpu_device *adev = drm_to_adev(dev);
	struct amdgpu_cs_parser parser;
	int r;

	if (current->flags & PF_EXITING)
		return -EPERM;

	if (!adev->accel_working)
		return -EBUSY;

	r = amdgpu_cs_parser_init(&parser, adev, filp, data);
	if (r) {
		if (printk_ratelimit())
			DRM_ERROR("Failed to initialize parser %d!\n", r);
		return r;
	}

	r = amdgpu_cs_pass1(&parser, data);
	if (r)
		goto error_fini;

	r = amdgpu_cs_pass2(&parser);
	if (r)
		goto error_fini;

	r = amdgpu_cs_user_time(&parser);
	if (r)
		goto error_fini;

	sgpu_swap_lock(filp->driver_priv);

	r = amdgpu_cs_parser_bos(&parser, data);
	if (r) {
		if (r == -ENOMEM)
			DRM_ERROR("Not enough memory for command submission!\n");
		else if (r != -ERESTARTSYS && r != -EAGAIN)
			DRM_ERROR("Failed to process the buffer list %d!\n", r);
		goto error_swap;
	}

	r = amdgpu_cs_patch_jobs(&parser);
	if (r)
		goto error_backoff;

	r = amdgpu_cs_vm_handling(&parser);
	if (r)
		goto error_backoff;

	r = amdgpu_cs_sync_rings(&parser);
	if (r)
		goto error_backoff;

	trace_amdgpu_cs_ibs(&parser);

	r = amdgpu_cs_submit(&parser, data);
	if (r)
		goto error_backoff;

	amdgpu_cs_parser_fini(&parser);
	return 0;

error_backoff:
	ttm_eu_backoff_reservation(&parser.ticket, &parser.validated);

error_swap:
	sgpu_swap_unlock(filp->driver_priv);

error_fini:
	amdgpu_cs_parser_fini(&parser);
	return r;
}

/**
 * amdgpu_cs_wait_ioctl - wait for a command submission to finish
 *
 * @dev: drm device
 * @data: data from userspace
 * @filp: file private
 *
 * Wait for the command submission identified by handle to finish.
 */
int amdgpu_cs_wait_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *filp)
{
	union drm_amdgpu_wait_cs *wait = data;
	unsigned long timeout = amdgpu_gem_timeout(wait->in.timeout);
	struct drm_sched_entity *entity;
	struct amdgpu_ctx *ctx;
	struct dma_fence *fence;
	long r;

	ctx = amdgpu_ctx_get(filp->driver_priv, wait->in.ctx_id);
	if (ctx == NULL)
		return -EINVAL;

	r = amdgpu_ctx_get_entity(ctx, wait->in.ip_type, wait->in.ip_instance,
				  wait->in.ring, &entity);
	if (r) {
		amdgpu_ctx_put(ctx);
		return r;
	}

	fence = amdgpu_ctx_get_fence(ctx, entity, wait->in.handle);
	if (IS_ERR(fence))
		r = PTR_ERR(fence);
	else if (fence) {
		r = dma_fence_wait_timeout(fence, true, timeout);
		if (r > 0 && fence->error)
			r = fence->error;
		dma_fence_put(fence);
	} else
		r = 1;

	amdgpu_ctx_put(ctx);
	if (r < 0)
		return r;

	memset(wait, 0, sizeof(*wait));
	wait->out.status = (r == 0);

	return 0;
}

/**
 * amdgpu_cs_get_fence - helper to get fence from drm_amdgpu_fence
 *
 * @adev: amdgpu device
 * @filp: file private
 * @user: drm_amdgpu_fence copied from user space
 */
static struct dma_fence *amdgpu_cs_get_fence(struct amdgpu_device *adev,
					     struct drm_file *filp,
					     struct drm_amdgpu_fence *user)
{
	struct drm_sched_entity *entity;
	struct amdgpu_ctx *ctx;
	struct dma_fence *fence;
	int r;

	ctx = amdgpu_ctx_get(filp->driver_priv, user->ctx_id);
	if (ctx == NULL)
		return ERR_PTR(-EINVAL);

	r = amdgpu_ctx_get_entity(ctx, user->ip_type, user->ip_instance,
				  user->ring, &entity);
	if (r) {
		amdgpu_ctx_put(ctx);
		return ERR_PTR(r);
	}

	fence = amdgpu_ctx_get_fence(ctx, entity, user->seq_no);
	amdgpu_ctx_put(ctx);

	return fence;
}

int amdgpu_cs_fence_to_handle_ioctl(struct drm_device *dev, void *data,
				    struct drm_file *filp)
{
	struct amdgpu_device *adev = drm_to_adev(dev);
	union drm_amdgpu_fence_to_handle *info = data;
	struct dma_fence *fence;
	struct drm_syncobj *syncobj;
	struct sync_file *sync_file;
	int fd, r;

	fence = amdgpu_cs_get_fence(adev, filp, &info->in.fence);
	if (IS_ERR(fence))
		return PTR_ERR(fence);

	if (!fence)
		fence = dma_fence_get_stub();

	switch (info->in.what) {
	case AMDGPU_FENCE_TO_HANDLE_GET_SYNCOBJ:
		r = drm_syncobj_create(&syncobj, 0, fence);
		dma_fence_put(fence);
		if (r)
			return r;
		r = drm_syncobj_get_handle(filp, syncobj, &info->out.handle);
		drm_syncobj_put(syncobj);
		return r;

	case AMDGPU_FENCE_TO_HANDLE_GET_SYNCOBJ_FD:
		r = drm_syncobj_create(&syncobj, 0, fence);
		dma_fence_put(fence);
		if (r)
			return r;
		r = drm_syncobj_get_fd(syncobj, (int*)&info->out.handle);
		drm_syncobj_put(syncobj);
		return r;

	case AMDGPU_FENCE_TO_HANDLE_GET_SYNC_FILE_FD:
		fd = get_unused_fd_flags(O_CLOEXEC);
		if (fd < 0) {
			dma_fence_put(fence);
			return fd;
		}

		sync_file = sync_file_create(fence);
		dma_fence_put(fence);
		if (!sync_file) {
			put_unused_fd(fd);
			return -ENOMEM;
		}

		fd_install(fd, sync_file->file);
		info->out.handle = fd;
		return 0;

	default:
		dma_fence_put(fence);
		return -EINVAL;
	}
}

/**
 * amdgpu_cs_wait_all_fences - wait on all fences to signal
 *
 * @adev: amdgpu device
 * @filp: file private
 * @wait: wait parameters
 * @fences: array of drm_amdgpu_fence
 */
static int amdgpu_cs_wait_all_fences(struct amdgpu_device *adev,
				     struct drm_file *filp,
				     union drm_amdgpu_wait_fences *wait,
				     struct drm_amdgpu_fence *fences)
{
	uint32_t fence_count = wait->in.fence_count;
	unsigned int i;
	long r = 1;

	for (i = 0; i < fence_count; i++) {
		struct dma_fence *fence;
		unsigned long timeout = amdgpu_gem_timeout(wait->in.timeout_ns);

		fence = amdgpu_cs_get_fence(adev, filp, &fences[i]);
		if (IS_ERR(fence))
			return PTR_ERR(fence);
		else if (!fence)
			continue;

		r = dma_fence_wait_timeout(fence, true, timeout);
		dma_fence_put(fence);
		if (r < 0)
			return r;

		if (r == 0)
			break;

		if (fence->error)
			return fence->error;
	}

	memset(wait, 0, sizeof(*wait));
	wait->out.status = (r > 0);

	return 0;
}

/**
 * amdgpu_cs_wait_any_fence - wait on any fence to signal
 *
 * @adev: amdgpu device
 * @filp: file private
 * @wait: wait parameters
 * @fences: array of drm_amdgpu_fence
 */
static int amdgpu_cs_wait_any_fence(struct amdgpu_device *adev,
				    struct drm_file *filp,
				    union drm_amdgpu_wait_fences *wait,
				    struct drm_amdgpu_fence *fences)
{
	unsigned long timeout = amdgpu_gem_timeout(wait->in.timeout_ns);
	uint32_t fence_count = wait->in.fence_count;
	uint32_t first = ~0;
	struct dma_fence **array;
	unsigned int i;
	long r;

	/* Prepare the fence array */
	array = kcalloc(fence_count, sizeof(struct dma_fence *), GFP_KERNEL);

	if (array == NULL)
		return -ENOMEM;

	for (i = 0; i < fence_count; i++) {
		struct dma_fence *fence;

		fence = amdgpu_cs_get_fence(adev, filp, &fences[i]);
		if (IS_ERR(fence)) {
			r = PTR_ERR(fence);
			goto err_free_fence_array;
		} else if (fence) {
			array[i] = fence;
		} else { /* NULL, the fence has been already signaled */
			r = 1;
			first = i;
			goto out;
		}
	}

	r = dma_fence_wait_any_timeout(array, fence_count, true, timeout,
				       &first);
	if (r < 0)
		goto err_free_fence_array;

out:
	memset(wait, 0, sizeof(*wait));
	wait->out.status = (r > 0);
	wait->out.first_signaled = first;

	if (first < fence_count && array[first])
		r = array[first]->error;
	else
		r = 0;

err_free_fence_array:
	for (i = 0; i < fence_count; i++)
		dma_fence_put(array[i]);
	kfree(array);

	return r;
}

/**
 * amdgpu_cs_wait_fences_ioctl - wait for multiple command submissions to finish
 *
 * @dev: drm device
 * @data: data from userspace
 * @filp: file private
 */
int amdgpu_cs_wait_fences_ioctl(struct drm_device *dev, void *data,
				struct drm_file *filp)
{
	struct amdgpu_device *adev = drm_to_adev(dev);
	union drm_amdgpu_wait_fences *wait = data;
	uint32_t fence_count = wait->in.fence_count;
	struct drm_amdgpu_fence *fences_user;
	struct drm_amdgpu_fence *fences;
	int r;

	/* Get the fences from userspace */
	fences = kmalloc_array(fence_count, sizeof(struct drm_amdgpu_fence),
			GFP_KERNEL);
	if (fences == NULL)
		return -ENOMEM;

	fences_user = u64_to_user_ptr(wait->in.fences);
	if (copy_from_user(fences, fences_user,
		sizeof(struct drm_amdgpu_fence) * fence_count)) {
		r = -EFAULT;
		goto err_free_fences;
	}

	if (wait->in.wait_all)
		r = amdgpu_cs_wait_all_fences(adev, filp, wait, fences);
	else
		r = amdgpu_cs_wait_any_fence(adev, filp, wait, fences);

err_free_fences:
	kfree(fences);

	return r;
}

/**
 * amdgpu_cs_find_mapping - find bo_va for VM address
 *
 * @parser: command submission parser context
 * @addr: VM address
 * @bo: resulting BO of the mapping found
 * @map: Placeholder to return found BO mapping
 *
 * Search the buffer objects in the command submission context for a certain
 * virtual memory address. Returns allocation structure when found, NULL
 * otherwise.
 */
int amdgpu_cs_find_mapping(struct amdgpu_cs_parser *parser,
			   uint64_t addr, struct amdgpu_bo **bo,
			   struct amdgpu_bo_va_mapping **map)
{
	struct amdgpu_fpriv *fpriv = parser->filp->driver_priv;
	struct ttm_operation_ctx ctx = { false, false };
	struct amdgpu_vm *vm = &fpriv->vm;
	struct amdgpu_bo_va_mapping *mapping;
	int r;

	addr /= AMDGPU_GPU_PAGE_SIZE;

	mapping = amdgpu_vm_bo_lookup_mapping(vm, addr);
	if (!mapping || !mapping->bo_va || !mapping->bo_va->base.bo)
		return -EINVAL;

	*bo = mapping->bo_va->base.bo;
	*map = mapping;

	/* Double check that the BO is reserved by this CS */
	if (dma_resv_locking_ctx((*bo)->tbo.base.resv) != &parser->ticket)
		return -EINVAL;

	if (!((*bo)->flags & AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS)) {
		(*bo)->flags |= AMDGPU_GEM_CREATE_VRAM_CONTIGUOUS;
		amdgpu_bo_placement_from_domain(*bo, (*bo)->allowed_domains);
		r = ttm_bo_validate(&(*bo)->tbo, &(*bo)->placement, &ctx);
		if (r)
			return r;
	}

	return amdgpu_ttm_alloc_gart(&(*bo)->tbo);
}
