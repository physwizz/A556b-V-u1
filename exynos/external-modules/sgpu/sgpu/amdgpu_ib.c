/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 * Copyright 2009 Jerome Glisse.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 *          Jerome Glisse
 *          Christian König
 */
#include <linux/seq_file.h>
#include <linux/slab.h>

#include <drm/sgpu_drm.h>
#include <drm/drm_debugfs.h>

#include "amdgpu.h"
#include "amdgpu_trace.h"

#ifdef CONFIG_DRM_SGPU_EMULATION_FULL_SOC
#define AMDGPU_IB_TEST_TIMEOUT	msecs_to_jiffies(10000)
#else
#define AMDGPU_IB_TEST_TIMEOUT	msecs_to_jiffies(1000)
#endif
#define AMDGPU_IB_TEST_GFX_XGMI_TIMEOUT	msecs_to_jiffies(2000)

/*
 * IB
 * IBs (Indirect Buffers) and areas of GPU accessible memory where
 * commands are stored.  You can put a pointer to the IB in the
 * command ring and the hw will fetch the commands from the IB
 * and execute them.  Generally userspace acceleration drivers
 * produce command buffers which are send to the kernel and
 * put in IBs for execution by the requested ring.
 */

/**
 * amdgpu_ib_get - request an IB (Indirect Buffer)
 *
 * @ring: ring index the IB is associated with
 * @size: requested IB size
 * @pool_type: IB pool type (delayed, immediate, direct)
 * @ib: IB object returned
 *
 * Request an IB (all asics).  IBs are allocated using the
 * suballocator.
 * Returns 0 on success, error on failure.
 */
int amdgpu_ib_get(struct amdgpu_device *adev, struct amdgpu_vm *vm,
		  unsigned size, enum amdgpu_ib_pool_type pool_type,
		  struct amdgpu_ib *ib)
{
	int r;

	if (size) {
		r = amdgpu_sa_bo_new(&adev->ib_pools[pool_type],
				      &ib->sa_bo, size, 256);
		if (r) {
			dev_err(adev->dev, "failed to get a new IB (%d)\n", r);
			return r;
		}

		ib->ptr = amdgpu_sa_bo_cpu_addr(ib->sa_bo);

		if (!vm)
			ib->gpu_addr = amdgpu_sa_bo_gpu_addr(ib->sa_bo);
	}

	return 0;
}

/**
 * amdgpu_ib_free - free an IB (Indirect Buffer)
 *
 * @adev: amdgpu_device pointer
 * @ib: IB object to free
 * @f: the fence SA bo need wait on for the ib alloation
 *
 * Free an IB (all asics).
 */
void amdgpu_ib_free(struct amdgpu_device *adev, struct amdgpu_ib *ib,
		    struct dma_fence *f)
{
	amdgpu_sa_bo_free(adev, &ib->sa_bo, f);
}

/**
 * amdgpu_ib_schedule - schedule an IB (Indirect Buffer) on the ring
 *
 * @adev: amdgpu_device pointer
 * @num_ibs: number of IBs to schedule
 * @ibs: IB objects to schedule
 * @job: job to schedule
 * @f: fence created during this submission
 *
 * Schedule an IB on the associated ring (all asics).
 * Returns 0 on success, error on failure.
 *
 * On SI, there are two parallel engines fed from the primary ring,
 * the CE (Constant Engine) and the DE (Drawing Engine).  Since
 * resource descriptors have moved to memory, the CE allows you to
 * prime the caches while the DE is updating register state so that
 * the resource descriptors will be already in cache when the draw is
 * processed.  To accomplish this, the userspace driver submits two
 * IBs, one for the CE and one for the DE.  If there is a CE IB (called
 * a CONST_IB), it will be put on the ring prior to the DE IB.  Prior
 * to SI there was just a DE IB.
 */
int amdgpu_ib_schedule(struct amdgpu_ring *ring, unsigned num_ibs,
		       struct amdgpu_ib *ibs, struct amdgpu_job *job,
		       struct dma_fence **f)
{
	struct amdgpu_device *adev = ring->adev;
	struct amdgpu_ib *ib = &ibs[0];
	struct dma_fence *tmp = NULL;
	bool skip_preamble, need_ctx_switch;
	unsigned patch_offset = ~0;
	struct amdgpu_vm *vm;
	uint64_t fence_ctx;
	uint32_t status = 0, alloc_size;
	unsigned fence_flags = 0;
	bool secure;

	unsigned i;
	int r = 0;
	bool need_pipe_sync = false;
#ifdef CONFIG_DRM_SGPU_DVFS
	uint32_t job_count = ring->fence_drv.sync_seq;
	bool cu_job = false;
#endif /* CONFIG_DRM_SGPU_DVFS */

	if (num_ibs == 0)
		return -EINVAL;

	/* ring tests don't use a job */
	if (job) {
		vm = job->vm;
		fence_ctx = job->base.s_fence ?
			job->base.s_fence->scheduled.context : 0;
	} else {
		vm = NULL;
		fence_ctx = 0;
	}

	if (!ring->sched.ready) {
		dev_err(adev->dev, "couldn't schedule ib on ring <%s>\n", ring->name);
		return -EINVAL;
	}

	if (vm && !job->vmid) {
		dev_err(adev->dev, "VM IB without ID\n");
		return -EINVAL;
	}

	if ((ib->flags & AMDGPU_IB_FLAGS_SECURE) &&
	    (ring->funcs->type == AMDGPU_RING_TYPE_COMPUTE)) {
		dev_err(adev->dev, "per IB secure submissions not supported on compute rings\n");
		dev_err(adev->dev, "Secure submission to compute ring must be done through a secure context\n");
		return -EINVAL;
	}

	alloc_size = ring->funcs->emit_frame_size + num_ibs *
		ring->funcs->emit_ib_size;

	r = amdgpu_ring_alloc(ring, alloc_size);
	if (r) {
		dev_err(adev->dev, "scheduling IB failed (%d).\n", r);
		return r;
	}

	need_ctx_switch = ring->current_ctx != fence_ctx;
	if (ring->funcs->emit_pipeline_sync && job &&
	    ((tmp = amdgpu_sync_get_fence(&job->sched_sync)) ||
	     (amdgpu_sriov_vf(adev) && need_ctx_switch) ||
	     amdgpu_vm_need_pipeline_sync(ring, job))) {
		need_pipe_sync = true;

		if (tmp)
			trace_amdgpu_ib_pipe_sync(job, tmp);

		dma_fence_put(tmp);
	}

	if ((ib->flags & AMDGPU_IB_FLAG_EMIT_MEM_SYNC) && ring->funcs->emit_mem_sync)
		ring->funcs->emit_mem_sync(ring);

	if (ring->funcs->insert_start)
		ring->funcs->insert_start(ring);

	if (job) {
		r = amdgpu_vm_flush(ring, job, need_pipe_sync);
		if (r) {
			amdgpu_ring_undo(ring);
			return r;
		}
	}

	if (job && ring->funcs->init_cond_exec)
		patch_offset = amdgpu_ring_init_cond_exec(ring);

	if (ring->funcs->emit_hdp_flush)
		amdgpu_ring_emit_hdp_flush(ring);
	else
		amdgpu_asic_flush_hdp(adev, ring);

	/* Setup initial TMZiness and send it off.
	 */
	secure = false;
	if (job && ring->funcs->emit_frame_cntl) {
		secure = ib->flags & AMDGPU_IB_FLAGS_SECURE;
		amdgpu_ring_emit_frame_cntl(ring, true, secure);
	}

	if (need_ctx_switch)
		status |= AMDGPU_HAVE_CTX_SWITCH;

	skip_preamble = ring->current_ctx == fence_ctx;
	if (job && ring->funcs->emit_cntxcntl) {
		status |= job->preamble_status;
		status |= job->preemption_status;
		amdgpu_ring_emit_cntxcntl(ring, status);
	}

	for (i = 0; i < num_ibs; ++i) {
		ib = &ibs[i];

		/* drop preamble IBs if we don't have a context switch */
		if ((ib->flags & AMDGPU_IB_FLAG_PREAMBLE) &&
		    skip_preamble &&
		    !(status & AMDGPU_PREAMBLE_IB_PRESENT_FIRST) &&
		    !amdgpu_mcbp &&
		    !amdgpu_sriov_vf(adev)) /* for SRIOV preemption, Preamble CE ib must be inserted anyway */
			continue;

		if (job && ring->funcs->emit_frame_cntl) {
			/* Compute rings are completely TMZ or not at all,
			 * cannot be set by a KMD frame
			 */
			if (ring->funcs->type != AMDGPU_RING_TYPE_COMPUTE &&
			    secure != !!(ib->flags & AMDGPU_IB_FLAGS_SECURE)) {
				amdgpu_ring_emit_frame_cntl(ring, false, secure);
				secure = !secure;
				amdgpu_ring_emit_frame_cntl(ring, true, secure);
			}
		}

		amdgpu_ring_emit_ib(ring, job, ib, status);
		status &= ~AMDGPU_HAVE_CTX_SWITCH;
	}

	if (job && ring->funcs->emit_frame_cntl)
		amdgpu_ring_emit_frame_cntl(ring, false, secure);

	amdgpu_asic_invalidate_hdp(adev, ring);

	if (ib->flags & AMDGPU_IB_FLAG_TC_WB_NOT_INVALIDATE)
		fence_flags |= AMDGPU_FENCE_FLAG_TC_WB_ONLY;

	/* wrap the last IB with fence */
	if (job && job->uf_addr) {
		amdgpu_ring_emit_fence(ring, job->uf_addr, job->uf_sequence,
				       fence_flags | AMDGPU_FENCE_FLAG_64BIT);
	}

	r = amdgpu_fence_emit(ring, f, fence_flags);
	if (r) {
		dev_err(adev->dev, "failed to emit fence (%d)\n", r);
		if (job && job->vmid)
			amdgpu_vmid_reset(adev, ring->funcs->vmhub, job->vmid);
		amdgpu_ring_undo(ring);
		return r;
	}

	if (job && job->fence)
		SGPU_LOG(adev, DMSG_INFO, DMSG_ETC,
			"fence replace %s sgpu %llu/%llu -> %llu/%llu",
			ring->name,
			job->fence->context, job->fence->seqno,
			(*f)->context, (*f)->seqno);

	SGPU_LOG(adev, DMSG_INFO, DMSG_ETC,
		"%s id %d vmid %u pasid %u secure %d "
		"drm %llu/%llu/%llu sgpu %llu/%llu",
		ring->name,
		(job) ? job->base.id : 0,
		(job) ? job->vmid : 0,
		(job) ? job->pasid : 0,
		secure,
		(job) ? job->base.s_fence->scheduled.context : 0,
		(job) ? job->base.s_fence->finished.context : 0,
		(job) ? job->base.s_fence->finished.seqno : 0,
		(*f)->context, (*f)->seqno);

	if (ring->funcs->insert_end)
		ring->funcs->insert_end(ring);

	if (patch_offset != ~0 && ring->funcs->patch_cond_exec)
		amdgpu_ring_patch_cond_exec(ring, patch_offset);

	ring->current_ctx = fence_ctx;

#ifdef CONFIG_DRM_SGPU_DVFS
	if (ring->fence_drv.sync_seq >= job_count)
		job_count = ring->fence_drv.sync_seq - job_count;
	cu_job = (ring->funcs->type == AMDGPU_RING_TYPE_COMPUTE);
	sgpu_utilization_job_start(ring->adev->devfreq, job_count, cu_job);
#endif /* CONFIG_DRM_SGPU_DVFS */

	/* dma_fence_cb will be added to measure end_time */
	sgpu_worktime_start(job, *f);

	trace_amdgpu_ib_schedule(job, *f);

	if (job && job->ifh_mode &&
	    (ring->funcs->type == AMDGPU_RING_TYPE_GFX ||
	     ring->funcs->type == AMDGPU_RING_TYPE_SDMA ||
	     ring->funcs->type == AMDGPU_RING_TYPE_COMPUTE)) {
		amdgpu_ring_undo(ring);
		goto out;
	}

	amdgpu_ring_commit(ring);

out:
	return 0;
}

/**
 * amdgpu_ib_pool_init - Init the IB (Indirect Buffer) pool
 *
 * @adev: amdgpu_device pointer
 *
 * Initialize the suballocator to manage a pool of memory
 * for use as IBs (all asics).
 * Returns 0 on success, error on failure.
 */
int amdgpu_ib_pool_init(struct amdgpu_device *adev)
{
	unsigned size;
	int r, i;

	if (adev->ib_pool_ready)
		return 0;

	for (i = 0; i < AMDGPU_IB_POOL_MAX; i++) {
		if (i == AMDGPU_IB_POOL_DIRECT)
			size = PAGE_SIZE * 2;
		else
			size = AMDGPU_IB_POOL_SIZE;

		r = amdgpu_sa_bo_manager_init(adev, &adev->ib_pools[i],
					      size, AMDGPU_GPU_PAGE_SIZE,
					      AMDGPU_GEM_DOMAIN_GTT);
		if (r)
			goto error;
	}
	adev->ib_pool_ready = true;

	return 0;

error:
	while (i--)
		amdgpu_sa_bo_manager_fini(adev, &adev->ib_pools[i]);
	return r;
}

/**
 * amdgpu_ib_pool_fini - Free the IB (Indirect Buffer) pool
 *
 * @adev: amdgpu_device pointer
 *
 * Tear down the suballocator managing the pool of memory
 * for use as IBs (all asics).
 */
void amdgpu_ib_pool_fini(struct amdgpu_device *adev)
{
	int i;

	if (!adev->ib_pool_ready)
		return;

	for (i = 0; i < AMDGPU_IB_POOL_MAX; i++)
		amdgpu_sa_bo_manager_fini(adev, &adev->ib_pools[i]);
	adev->ib_pool_ready = false;
}

/**
 * amdgpu_ib_ring_tests - test IBs on the rings
 *
 * @adev: amdgpu_device pointer
 *
 * Test an IB (Indirect Buffer) on each ring.
 * If the test fails, disable the ring.
 * Returns 0 on success, error if the primary GFX ring
 * IB test fails.
 */
int amdgpu_ib_ring_tests(struct amdgpu_device *adev)
{
	long tmo_gfx, tmo_mm;
	int r, ret = 0;
	unsigned i;

	if (sgpu_no_hw_access != 0)
		return 0;

	tmo_mm = tmo_gfx = AMDGPU_IB_TEST_TIMEOUT;

	if (adev->gmc.xgmi.hive_id) {
		tmo_gfx = AMDGPU_IB_TEST_GFX_XGMI_TIMEOUT;
	}

	if (sgpu_no_timeout != 0) {
		tmo_gfx = MAX_SCHEDULE_TIMEOUT;
	}
	for (i = 0; i < adev->num_rings; ++i) {
		struct amdgpu_ring *ring = adev->rings[i];
		long tmo;

		/* KIQ rings don't have an IB test because we never submit IBs
		 * to them and they have no interrupt support.
		 */
		if (!ring->sched.ready || !ring->funcs->test_ib)
			continue;

		/* MM engine need more time */
		if (ring->funcs->type == AMDGPU_RING_TYPE_UVD ||
			ring->funcs->type == AMDGPU_RING_TYPE_VCE ||
			ring->funcs->type == AMDGPU_RING_TYPE_UVD_ENC ||
			ring->funcs->type == AMDGPU_RING_TYPE_VCN_DEC ||
			ring->funcs->type == AMDGPU_RING_TYPE_VCN_ENC ||
			ring->funcs->type == AMDGPU_RING_TYPE_VCN_JPEG)
			tmo = tmo_mm;
		else
			tmo = tmo_gfx;

		r = amdgpu_ring_test_ib(ring, tmo);
		if (!r) {
			DRM_DEV_DEBUG(adev->dev, "ib test on %s succeeded\n",
				      ring->name);
			continue;
		}

		ring->sched.ready = false;
		DRM_DEV_ERROR(adev->dev, "HW defect detected : IB test failed on %s (%d).\n",
			  ring->name, r);
		sgpu_debug_snapshot_expire_watchdog();

		if (ring == &adev->gfx.gfx_ring[0]) {
			/* oh, oh, that's really bad */
			adev->accel_working = false;
			return r;

		} else {
			ret = r;
		}
	}
	return ret;
}

/*
 * Debugfs info
 */
#if defined(CONFIG_DEBUG_FS)

static int amdgpu_debugfs_sa_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct amdgpu_device *adev = drm_to_adev(dev);

	seq_printf(m, "--------------------- DELAYED --------------------- \n");
	amdgpu_sa_bo_dump_debug_info(&adev->ib_pools[AMDGPU_IB_POOL_DELAYED],
				     m);
	seq_printf(m, "-------------------- IMMEDIATE -------------------- \n");
	amdgpu_sa_bo_dump_debug_info(&adev->ib_pools[AMDGPU_IB_POOL_IMMEDIATE],
				     m);
	seq_printf(m, "--------------------- DIRECT ---------------------- \n");
	amdgpu_sa_bo_dump_debug_info(&adev->ib_pools[AMDGPU_IB_POOL_DIRECT], m);

	return 0;
}

static const struct drm_info_list amdgpu_debugfs_sa_list[] = {
	{"amdgpu_sa_info", &amdgpu_debugfs_sa_info, 0, NULL},
};

#endif

int amdgpu_debugfs_sa_init(struct amdgpu_device *adev)
{
#if defined(CONFIG_DEBUG_FS)
	return amdgpu_debugfs_add_files(adev, amdgpu_debugfs_sa_list,
					ARRAY_SIZE(amdgpu_debugfs_sa_list));
#else
	return 0;
#endif
}
