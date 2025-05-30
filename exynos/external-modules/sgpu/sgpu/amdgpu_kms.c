/*
 * Copyright 2008-2021 Advanced Micro Devices, Inc.
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
 */

#include "amdgpu.h"
#include <drm/drm_debugfs.h>
#include <drm/sgpu_drm.h>
#include "amdgpu_sched.h"
#include "sgpu_debugfs.h"

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/pm_runtime.h>
#include <linux/file.h>
#include "amdgpu_gem.h"
#include "amdgpu_cwsr.h"
#include "amdgpu_trace.h"
#include "sgpu_swap.h"
#ifdef CONFIG_DRM_SGPU_BPMD
#include "sgpu_bpmd.h"
#endif

#ifdef CONFIG_DRM_SGPU_EXYNOS
/* save global variable to access this as symbol on ramdump */
struct amdgpu_device *p_adev;
#endif /* CONFIG_DRM_SGPU_EXYNOS */

void amdgpu_unregister_gpu_instance(struct amdgpu_device *adev)
{
	struct amdgpu_gpu_instance *gpu_instance;
	int i;

	mutex_lock(&mgpu_info.mutex);

	for (i = 0; i < mgpu_info.num_gpu; i++) {
		gpu_instance = &(mgpu_info.gpu_ins[i]);
		if (gpu_instance->adev == adev) {
			mgpu_info.gpu_ins[i] =
				mgpu_info.gpu_ins[mgpu_info.num_gpu - 1];
			mgpu_info.num_gpu--;
			break;
		}
	}

	mutex_unlock(&mgpu_info.mutex);
}

/**
 * amdgpu_driver_unload_kms - Main unload function for KMS.
 *
 * @dev: drm dev pointer
 *
 * This is the main unload function for KMS (all asics).
 * Returns 0 on success.
 */
void amdgpu_driver_unload_kms(struct drm_device *dev)
{
	struct amdgpu_device *adev = drm_to_adev(dev);

	if (adev == NULL)
		return;

	amdgpu_unregister_gpu_instance(adev);

	if (adev->rmmio == NULL)
		return;

	if (adev->runpm) {
		pm_runtime_get_sync(dev->dev);
		pm_runtime_forbid(dev->dev);
	}

	sgpu_ifpo_lock(adev);

	amdgpu_device_fini(adev);
}

void amdgpu_register_gpu_instance(struct amdgpu_device *adev)
{
	struct amdgpu_gpu_instance *gpu_instance;

	mutex_lock(&mgpu_info.mutex);

	if (mgpu_info.num_gpu >= MAX_GPU_INSTANCE) {
		DRM_ERROR("Cannot register more gpu instance\n");
		mutex_unlock(&mgpu_info.mutex);
		return;
	}

	gpu_instance = &(mgpu_info.gpu_ins[mgpu_info.num_gpu]);
	gpu_instance->adev = adev;
	gpu_instance->mgpu_fan_enabled = 0;

	mgpu_info.num_gpu++;

	mutex_unlock(&mgpu_info.mutex);
}

/**
 * amdgpu_driver_load_kms - Main load function for KMS.
 *
 * @adev: pointer to struct amdgpu_device
 * @flags: device flags
 *
 * This is the main load function for KMS (all asics).
 * Returns 0 on success, error on failure.
 */
int amdgpu_driver_load_kms(struct amdgpu_device *adev)
{
	struct drm_device *dev;
	int r;

#ifdef CONFIG_DRM_SGPU_EXYNOS
	p_adev = adev;
#endif /* CONFIG_DRM_SGPU_EXYNOS */

	dev = adev_to_drm(adev);

	if (amdgpu_runtime_pm != 0)
		adev->runpm = true;

	if (adev->runpm) {
		pm_runtime_use_autosuspend(dev->dev);
		pm_runtime_set_autosuspend_delay(dev->dev, 50);
		pm_runtime_mark_last_busy(dev->dev);
		pm_runtime_enable(dev->dev);
		adev->in_runpm = false;
	}

	sgpu_ifpo_init(adev);

	if (adev->runpm) {
		/* amdgpu_device_init func need to power for fw_init */
		pm_runtime_get_sync(adev->dev);
	}

	sgpu_pm_monitor_init(adev);

	/* amdgpu_device_init should report only fatal error
	 * like memory allocation failure or iomapping failure,
	 * or memory manager initialization failure, it must
	 * properly initialize the GPU MC controller and permit
	 * VRAM allocation
	 */
	r = amdgpu_device_init(adev);
	if (r) {
		dev_err(&adev->pldev->dev, "Fatal error during GPU init\n");
		goto out;
	}

out:
	if (r) {
		/* balance pm_runtime_get_sync in amdgpu_driver_unload_kms */
		if (adev->rmmio && adev->runpm)
			pm_runtime_put_noidle(dev->dev);
		amdgpu_driver_unload_kms(dev);
	}

	if (adev->runpm) {
		/* balance pm_runtime_get_sync for amdgpu_device_init */
		pm_runtime_put_autosuspend(dev->dev);
	}

	return r;
}

static int amdgpu_firmware_info(struct drm_amdgpu_info_firmware *fw_info,
				struct drm_amdgpu_query_fw *query_fw,
				struct amdgpu_device *adev)
{
	switch (query_fw->fw_type) {
	case AMDGPU_INFO_FW_GMC:
		fw_info->ver = adev->gmc.fw_version;
		fw_info->feature = 0;
		break;
	case AMDGPU_INFO_FW_GFX_ME:
		fw_info->ver = adev->gfx.me_fw_version;
		fw_info->feature = adev->gfx.me_feature_version;
		break;
	case AMDGPU_INFO_FW_GFX_PFP:
		fw_info->ver = adev->gfx.pfp_fw_version;
		fw_info->feature = adev->gfx.pfp_feature_version;
		break;
	case AMDGPU_INFO_FW_GFX_CE:
		fw_info->ver = adev->gfx.ce_fw_version;
		fw_info->feature = adev->gfx.ce_feature_version;
		break;
	case AMDGPU_INFO_FW_GFX_RLC:
		fw_info->ver = adev->gfx.rlc_fw_version;
		fw_info->feature = adev->gfx.rlc_feature_version;
		break;
	case AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_CNTL:
		fw_info->ver = adev->gfx.rlc_srlc_fw_version;
		fw_info->feature = adev->gfx.rlc_srlc_feature_version;
		break;
	case AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_GPM_MEM:
		fw_info->ver = adev->gfx.rlc_srlg_fw_version;
		fw_info->feature = adev->gfx.rlc_srlg_feature_version;
		break;
	case AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_SRM_MEM:
		fw_info->ver = adev->gfx.rlc_srls_fw_version;
		fw_info->feature = adev->gfx.rlc_srls_feature_version;
		break;
	case AMDGPU_INFO_FW_GFX_MEC:
		if (query_fw->index == 0) {
			fw_info->ver = adev->gfx.mec_fw_version;
			fw_info->feature = adev->gfx.mec_feature_version;
		} else if (query_fw->index == 1) {
			fw_info->ver = adev->gfx.mec2_fw_version;
			fw_info->feature = adev->gfx.mec2_feature_version;
		} else
			return -EINVAL;
		break;
	case AMDGPU_INFO_FW_SMC:
		fw_info->ver = adev->pm.fw_version;
		fw_info->feature = 0;
		break;
	case AMDGPU_INFO_FW_SDMA:
		if (query_fw->index >= adev->sdma.num_instances)
			return -EINVAL;
		fw_info->ver = adev->sdma.instance[query_fw->index].fw_version;
		fw_info->feature = adev->sdma.instance[query_fw->index].feature_version;
		break;
	case AMDGPU_INFO_FW_UNIFIED:
		fw_info->ver = (0x00 & 0xFF) << 24 |
			       (adev->gfx.sgpu_fw_major_version & 0xFF) << 16 |
			       (adev->gfx.sgpu_fw_minor_version & 0xFF) << 8 |
			       (adev->gfx.sgpu_fw_option_version & 0xFF);
		fw_info->feature = adev->gfx.sgpu_rtl_cl_number;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int amdgpu_hw_ip_info(struct amdgpu_device *adev,
			     struct drm_amdgpu_info *info,
			     struct drm_amdgpu_info_hw_ip *result)
{
	uint32_t ib_start_alignment = 0;
	uint32_t ib_size_alignment = 0;
	enum amd_ip_block_type type;
	unsigned int num_rings = 0;
	unsigned int i;

	if (info->query_hw_ip.ip_instance >= AMDGPU_HW_IP_INSTANCE_MAX_COUNT)
		return -EINVAL;

	switch (info->query_hw_ip.type) {
	case AMDGPU_HW_IP_GFX:
		type = AMD_IP_BLOCK_TYPE_GFX;
		for (i = 0; i < adev->gfx.num_gfx_rings; i++)
			if (adev->gfx.gfx_ring[i].sched.ready)
				++num_rings;
		ib_start_alignment = 32;
		ib_size_alignment = 32;
		break;
	case AMDGPU_HW_IP_COMPUTE:
		type = AMD_IP_BLOCK_TYPE_GFX;
		for (i = 0; i < adev->gfx.num_compute_rings; i++)
			if (adev->gfx.compute_ring[i].sched.ready)
				++num_rings;
		ib_start_alignment = 32;
		ib_size_alignment = 32;
		break;
	case AMDGPU_HW_IP_DMA:
		type = AMD_IP_BLOCK_TYPE_SDMA;
		for (i = 0; i < adev->sdma.num_instances; i++)
			if (adev->sdma.instance[i].ring.sched.ready)
				++num_rings;
		ib_start_alignment = 256;
		ib_size_alignment = 4;
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < adev->num_ip_blocks; i++)
		if (adev->ip_blocks[i].version->type == type &&
		    adev->ip_blocks[i].status.valid)
			break;

	if (i == adev->num_ip_blocks)
		return 0;

	num_rings = min(amdgpu_ctx_num_entities[info->query_hw_ip.type],
			num_rings);

	result->hw_ip_version_major = adev->ip_blocks[i].version->major;
	result->hw_ip_version_minor = adev->ip_blocks[i].version->minor;
	result->capabilities_flags = 0;
	result->available_rings = (1 << num_rings) - 1;
	result->ib_start_alignment = ib_start_alignment;
	result->ib_size_alignment = ib_size_alignment;
	return 0;
}

/*
 * Userspace get information ioctl
 */
/**
 * amdgpu_info_ioctl - answer a device specific request.
 *
 * @adev: amdgpu device pointer
 * @data: request object
 * @filp: drm filp
 *
 * This function is used to pass device specific parameters to the userspace
 * drivers.  Examples include: pci device id, pipeline parms, tiling params,
 * etc. (all asics).
 * Returns 0 on success, -EINVAL on failure.
 */
static int amdgpu_info_ioctl(struct drm_device *dev, void *data, struct drm_file *filp)
{
	struct amdgpu_device *adev = drm_to_adev(dev);
	struct amdgpu_fpriv *fpriv = (struct amdgpu_fpriv *)filp->driver_priv;
	struct drm_amdgpu_info *info = data;
	struct amdgpu_mode_info *minfo = &adev->mode_info;
	void __user *out = (void __user *)(uintptr_t)info->return_pointer;
	uint32_t size = info->return_size;
	struct drm_crtc *crtc;
	uint32_t ui32 = 0;
	uint64_t ui64 = 0;
	int i, found;

	if (!info->return_size || !info->return_pointer)
		return -EINVAL;

	switch (info->query) {
	case AMDGPU_INFO_ACCEL_WORKING:
		ui32 = adev->accel_working;
		return copy_to_user(out, &ui32, min(size, 4u)) ? -EFAULT : 0;
	case AMDGPU_INFO_CRTC_FROM_ID:
		for (i = 0, found = 0; i < adev->mode_info.num_crtc; i++) {
			crtc = (struct drm_crtc *)minfo->crtcs[i];
			if (crtc && crtc->base.id == info->mode_crtc.id) {
				struct amdgpu_crtc *amdgpu_crtc = to_amdgpu_crtc(crtc);
				ui32 = amdgpu_crtc->crtc_id;
				found = 1;
				break;
			}
		}
		if (!found) {
			DRM_DEBUG_KMS("unknown crtc id %d\n", info->mode_crtc.id);
			return -EINVAL;
		}
		return copy_to_user(out, &ui32, min(size, 4u)) ? -EFAULT : 0;
	case AMDGPU_INFO_HW_IP_INFO: {
		struct drm_amdgpu_info_hw_ip ip = {};
		int ret;

		ret = amdgpu_hw_ip_info(adev, info, &ip);
		if (ret)
			return ret;

		ret = copy_to_user(out, &ip, min((size_t)size, sizeof(ip)));
		return ret ? -EFAULT : 0;
	}
	case AMDGPU_INFO_HW_IP_COUNT: {
		enum amd_ip_block_type type;
		uint32_t count = 0;

		switch (info->query_hw_ip.type) {
		case AMDGPU_HW_IP_GFX:
			type = AMD_IP_BLOCK_TYPE_GFX;
			break;
		case AMDGPU_HW_IP_COMPUTE:
			type = AMD_IP_BLOCK_TYPE_GFX;
			break;
		case AMDGPU_HW_IP_DMA:
			type = AMD_IP_BLOCK_TYPE_SDMA;
			break;
		default:
			return -EINVAL;
		}

		for (i = 0; i < adev->num_ip_blocks; i++)
			if (adev->ip_blocks[i].version->type == type &&
			    adev->ip_blocks[i].status.valid &&
			    count < AMDGPU_HW_IP_INSTANCE_MAX_COUNT)
				count++;

		return copy_to_user(out, &count, min(size, 4u)) ? -EFAULT : 0;
	}
	case AMDGPU_INFO_TIMESTAMP:
		sgpu_ifpo_lock(adev);
		ui64 = amdgpu_gfx_get_gpu_clock_counter(adev);
		sgpu_ifpo_unlock(adev);

		return copy_to_user(out, &ui64, min(size, 8u)) ? -EFAULT : 0;
	case AMDGPU_INFO_FW_VERSION: {
		struct drm_amdgpu_info_firmware fw_info;
		int ret;

		/* We only support one instance of each IP block right now. */
		if (info->query_fw.ip_instance != 0)
			return -EINVAL;

		ret = amdgpu_firmware_info(&fw_info, &info->query_fw, adev);
		if (ret)
			return ret;

		return copy_to_user(out, &fw_info,
				    min((size_t)size, sizeof(fw_info))) ? -EFAULT : 0;
	}
	case AMDGPU_INFO_NUM_BYTES_MOVED:
		ui64 = atomic64_read(&adev->num_bytes_moved);
		return copy_to_user(out, &ui64, min(size, 8u)) ? -EFAULT : 0;
	case AMDGPU_INFO_NUM_EVICTIONS:
		ui64 = atomic64_read(&adev->num_evictions);
		return copy_to_user(out, &ui64, min(size, 8u)) ? -EFAULT : 0;
	case AMDGPU_INFO_NUM_VRAM_CPU_PAGE_FAULTS:
		ui64 = atomic64_read(&adev->num_vram_cpu_page_faults);
		return copy_to_user(out, &ui64, min(size, 8u)) ? -EFAULT : 0;
	case AMDGPU_INFO_VRAM_USAGE:
		ui64 = 0;
		return copy_to_user(out, &ui64, min(size, 8u)) ? -EFAULT : 0;
	case AMDGPU_INFO_VIS_VRAM_USAGE:
		ui64 = 0;
		return copy_to_user(out, &ui64, min(size, 8u)) ? -EFAULT : 0;
	case AMDGPU_INFO_GTT_USAGE:
		ui64 = ttm_resource_manager_usage(&adev->mman.gtt_mgr.manager);
		return copy_to_user(out, &ui64, min(size, 8u)) ? -EFAULT : 0;
	case AMDGPU_INFO_GDS_CONFIG: {
		struct drm_amdgpu_info_gds gds_info;

		memset(&gds_info, 0, sizeof(gds_info));
		gds_info.compute_partition_size = adev->gds.gds_size;
		gds_info.gds_total_size = adev->gds.gds_size;
		gds_info.gws_per_compute_partition = adev->gds.gws_size;
		gds_info.oa_per_compute_partition = adev->gds.oa_size;
		return copy_to_user(out, &gds_info,
				    min((size_t)size, sizeof(gds_info))) ? -EFAULT : 0;
	}
	case AMDGPU_INFO_VRAM_GTT: {
		struct drm_amdgpu_info_vram_gtt vram_gtt;
		memset(&vram_gtt, 0, sizeof(vram_gtt));
		return copy_to_user(out, &vram_gtt,
				    min((size_t)size, sizeof(vram_gtt))) ? -EFAULT : 0;
	}
	case AMDGPU_INFO_MEMORY: {
		struct drm_amdgpu_memory_info mem;
		struct ttm_resource_manager *gtt_man =
			ttm_manager_type(&adev->mman.bdev, TTM_PL_TT);
		memset(&mem, 0, sizeof(mem));

			mem.gtt.total_heap_size = gtt_man->size;
			mem.gtt.usable_heap_size = mem.gtt.total_heap_size -
				atomic64_read(&adev->gart_pin_size);
			mem.gtt.heap_usage = ttm_resource_manager_usage(gtt_man);
			mem.gtt.max_allocation = mem.gtt.usable_heap_size * 3 / 4;

		return copy_to_user(out, &mem,
				    min((size_t)size, sizeof(mem)))
				    ? -EFAULT : 0;
	}
	case AMDGPU_INFO_READ_MMR_REG: {
		unsigned n, alloc_size;
		uint32_t *regs;
		unsigned se_num = (info->read_mmr_reg.instance >>
				   AMDGPU_INFO_MMR_SE_INDEX_SHIFT) &
				  AMDGPU_INFO_MMR_SE_INDEX_MASK;
		unsigned sh_num = (info->read_mmr_reg.instance >>
				   AMDGPU_INFO_MMR_SH_INDEX_SHIFT) &
				  AMDGPU_INFO_MMR_SH_INDEX_MASK;
		int ret = 0;

		sgpu_ifpo_lock(adev);

		/* set full masks if the userspace set all bits
		 * in the bitfields */
		if (se_num == AMDGPU_INFO_MMR_SE_INDEX_MASK)
			se_num = 0xffffffff;
		else if (se_num >= AMDGPU_GFX_MAX_SE) {
			ret = -EINVAL;
			goto out_read_mmr;
		}

		if (sh_num == AMDGPU_INFO_MMR_SH_INDEX_MASK)
			sh_num = 0xffffffff;
		else if (sh_num >= AMDGPU_GFX_MAX_SH_PER_SE) {
			ret = -EINVAL;
			goto out_read_mmr;
		}

		if (info->read_mmr_reg.count > 128) {
			ret = -EINVAL;
			goto out_read_mmr;
		}

		regs = kmalloc_array(info->read_mmr_reg.count, sizeof(*regs), GFP_KERNEL);
		if (!regs) {
			ret = -ENOMEM;
			goto out_read_mmr;
		}
		alloc_size = info->read_mmr_reg.count * sizeof(*regs);

		amdgpu_gfx_off_ctrl(adev, false);
		for (i = 0; i < info->read_mmr_reg.count; i++) {
			if (amdgpu_asic_read_register(adev, se_num, sh_num,
						      info->read_mmr_reg.dword_offset + i,
						      &regs[i])) {
				DRM_DEBUG_KMS("unallowed offset %#x\n",
					      info->read_mmr_reg.dword_offset + i);
				kfree(regs);
				amdgpu_gfx_off_ctrl(adev, true);
				ret = -EFAULT;
				goto out_read_mmr;
			}
		}
		amdgpu_gfx_off_ctrl(adev, true);
		n = copy_to_user(out, regs, min(size, alloc_size));
		if (n)
			ret = -EFAULT;
		kfree(regs);
out_read_mmr:
		sgpu_ifpo_unlock(adev);

		return ret;
	}
	case AMDGPU_INFO_DEV_INFO: {
		struct drm_amdgpu_info_device dev_info;
		uint64_t vm_size;

		memset(&dev_info, 0, sizeof(dev_info));
		dev_info.device_id = adev->device_id;
		dev_info.chip_rev = adev->rev_id;
		dev_info.external_rev = adev->external_rev_id;
		dev_info.family = adev->family;
		dev_info.num_shader_engines = adev->gfx.config.max_shader_engines;
		dev_info.num_shader_arrays_per_engine = adev->gfx.config.max_sh_per_se;
		/* return all clocks in KHz */
		dev_info.gpu_counter_freq = amdgpu_asic_get_xclk(adev) * 10;
		dev_info.max_engine_clock = adev->clock.default_sclk * 10;
		dev_info.max_memory_clock = adev->clock.default_mclk * 10;
		dev_info.enabled_rb_pipes_mask = adev->gfx.config.backend_enable_mask;
		dev_info.num_rb_pipes = adev->gfx.config.max_backends_per_se *
			adev->gfx.config.max_shader_engines;
		dev_info.num_hw_gfx_contexts = adev->gfx.config.max_hw_contexts;
		dev_info._pad = 0;
		dev_info.ids_flags = 0;
		if (amdgpu_mcbp || amdgpu_sriov_vf(adev))
			dev_info.ids_flags |= AMDGPU_IDS_FLAGS_PREEMPTION;
		if (amdgpu_is_tmz(adev))
			dev_info.ids_flags |= AMDGPU_IDS_FLAGS_TMZ;
		if (!sgpu_is_dma_coherent(adev))
			dev_info.ids_flags |= AMDGPU_IDS_FLAGS_NON_IOCOHERENT;

		vm_size = adev->vm_manager.max_pfn * AMDGPU_GPU_PAGE_SIZE;
		vm_size -= AMDGPU_VA_RESERVED_SIZE;

		dev_info.virtual_address_offset = AMDGPU_VA_RESERVED_SIZE;
		dev_info.virtual_address_max =
			min(vm_size, AMDGPU_GMC_HOLE_START);

		if (vm_size > AMDGPU_GMC_HOLE_START) {
			dev_info.high_va_offset = AMDGPU_GMC_HOLE_END;
			dev_info.high_va_max = AMDGPU_GMC_HOLE_END | vm_size;
		}
		dev_info.virtual_address_alignment = max((int)PAGE_SIZE, AMDGPU_GPU_PAGE_SIZE);
		dev_info.pte_fragment_size = (1 << adev->vm_manager.fragment_size) * AMDGPU_GPU_PAGE_SIZE;
		dev_info.gart_page_size = AMDGPU_GPU_PAGE_SIZE;
		dev_info.cu_active_number = adev->gfx.cu_info.number;
		dev_info.cu_ao_mask = adev->gfx.cu_info.ao_cu_mask;
		dev_info.ce_ram_size = adev->gfx.ce_ram_size;
		memcpy(&dev_info.cu_ao_bitmap[0], &adev->gfx.cu_info.ao_cu_bitmap[0],
		       sizeof(adev->gfx.cu_info.ao_cu_bitmap));
		memcpy(&dev_info.cu_bitmap[0], &adev->gfx.cu_info.bitmap[0],
		       sizeof(adev->gfx.cu_info.bitmap));
		dev_info.vram_type = adev->gmc.vram_type;
		dev_info.vram_bit_width = adev->gmc.vram_width;
		dev_info.gc_double_offchip_lds_buf =
			adev->gfx.config.double_offchip_lds_buf;
		dev_info.wave_front_size = adev->gfx.cu_info.wave_front_size;
		dev_info.num_shader_visible_vgprs = adev->gfx.config.max_gprs;
		dev_info.num_cu_per_sh = adev->gfx.config.max_cu_per_sh;
		dev_info.num_tcc_blocks = adev->gfx.config.max_texture_channel_caches;
		dev_info.gs_vgt_table_depth = adev->gfx.config.gs_vgt_table_depth;
		dev_info.gs_prim_buffer_depth = adev->gfx.config.gs_prim_buffer_depth;
		dev_info.max_gs_waves_per_vgt = adev->gfx.config.max_gs_threads;

		if (adev->family >= AMDGPU_FAMILY_NV)
			dev_info.pa_sc_tile_steering_override =
				adev->gfx.config.pa_sc_tile_steering_override;

		dev_info.tcc_disabled_mask = adev->gfx.config.tcc_disabled_mask;

		return copy_to_user(out, &dev_info,
				    min((size_t)size, sizeof(dev_info))) ? -EFAULT : 0;
	}
	case AMDGPU_INFO_VBIOS: {
		uint32_t bios_size = adev->bios_size;

		switch (info->vbios_info.type) {
		case AMDGPU_INFO_VBIOS_SIZE:
			return copy_to_user(out, &bios_size,
					min((size_t)size, sizeof(bios_size)))
					? -EFAULT : 0;
		case AMDGPU_INFO_VBIOS_IMAGE: {
			uint8_t *bios;
			uint32_t bios_offset = info->vbios_info.offset;

			if (bios_offset >= bios_size)
				return -EINVAL;

			bios = adev->bios + bios_offset;
			return copy_to_user(out, bios,
					    min((size_t)size, (size_t)(bios_size - bios_offset)))
					? -EFAULT : 0;
		}
		default:
			DRM_DEBUG_KMS("Invalid request %d\n",
					info->vbios_info.type);
			return -EINVAL;
		}
	}
	case AMDGPU_INFO_VRAM_LOST_COUNTER:
		ui32 = atomic_read(&adev->vram_lost_counter);
		return copy_to_user(out, &ui32, min(size, 4u)) ? -EFAULT : 0;
	case SGPU_INFO_GPU_PAGE_FAULTS: { /* GPU Page Fault Info Query */
		struct drm_sgpu_query_gpu_page_faults gpf;
		struct sgpu_fault_record r;
		gpf.num_faults = 0;
		while(sgpu_fault_read_record(&fpriv->page_fault_record, &r)
			&& (gpf.num_faults < SGPU_INFO_GPU_PAGE_FAULTS_MAX)) {
				gpf.fault_addr[gpf.num_faults++] = r.addr;
		}
		return copy_to_user(out, &gpf, min((size_t)size, sizeof(gpf))) ? -EFAULT : 0;
	}
	case SGPU_INFO_BPMD_TO_FD: { /* dump BPMD to file descriptor */
		if (IS_ENABLED(CONFIG_DRM_SGPU_BPMD_FILE_DUMP) &&
			(info->dump_bpmd_to_fd.flags & SGPU_INFO_BPMD_FLAG_DUMP)) {
			struct file *f = fget(info->dump_bpmd_to_fd.fd);
			uint32_t section_filter = SGPU_BPMD_SECTION_ALL;

			if (f == NULL) {
				dev_err(adev->dev,"SGPU_INFO_BPMD_TO_FD:fget(%u) failed\n",
						info->dump_bpmd_to_fd.fd);
				return -EBADF;
			}
			sgpu_bpmd_dump_to_file(adev, f, section_filter);
			fput(f);
			return 0;
		} else {
			dev_err(adev->dev,"SGPU_INFO_BPMD_TO_FD but CONFIG_DRM_SGPU_BPMD not defined\n");
			return -EINVAL;
		}
	}
	case SGPU_INFO_BPMD: {
		if (IS_ENABLED(CONFIG_DRM_SGPU_BPMD)) {
			struct drm_sgpu_query_bpmd bpmd_result = {
				.bpmd_size = 0
			};
			uint32_t section_filter = SGPU_BPMD_SECTION_ALL;

			if (info->dump_bpmd.buffer) {
				void __user *buffer = (void __user *)(uintptr_t)info->dump_bpmd.buffer;

				bpmd_result.bpmd_size = sgpu_bpmd_dump_to_userptr(adev, buffer, info->dump_bpmd.buffer_size, section_filter);
			} else {
				bpmd_result.bpmd_size = sgpu_bpmd_dump_size(adev, section_filter);
			}
			return copy_to_user(out, &bpmd_result, min((size_t)size, sizeof(bpmd_result))) ? -EFAULT : 0;
		} else {
			dev_err(adev->dev,"SGPU_INFO_BPMD but CONFIG_DRM_SGPU_BPMD not defined\n");
			return -ENODEV;
		}
	}
	default:
		DRM_DEBUG_KMS("Invalid request %d\n", info->query);
		return -EINVAL;
	}
	return 0;
}

static int amdgpu_wgp_gating_ioctl(struct drm_device *dev, void *data,
				       struct drm_file *filp)
{
	int r = 0;
	struct amdgpu_device *adev = drm_to_adev(dev);
	union drm_amdgpu_wgp_gating *wgp = data;

	switch (wgp->in.op) {
	case AMDGPU_WGP_GATING_WGP_CLOCK_ON:
		r = amdgpu_gfx_set_num_clock_on_wgp(adev, wgp->in.value);
		wgp->out.wgp_clock_on.number = adev->gfx.num_clock_on_wgp;
		break;
	case AMDGPU_WGP_GATING_WGP_AON:
		r = amdgpu_gfx_set_num_aon_wgp(adev, wgp->in.value);
		wgp->out.wgp_aon.number = adev->gfx.num_aon_wgp;
		memcpy(wgp->out.wgp_aon.bitmap, adev->gfx.wgp_aon_bitmap,
		       sizeof(adev->gfx.wgp_aon_bitmap));
		break;
	case AMDGPU_WGP_GATING_WGP_STATUS:
		amdgpu_gfx_read_status_static_wgp(adev);
		memcpy(wgp->out.wgp_status.bitmap, adev->gfx.wgp_status_bitmap,
		       sizeof(adev->gfx.wgp_status_bitmap));
		break;
	default:
		DRM_DEBUG_KMS("Invalid request %d\n", wgp->in.op);
		return -EINVAL;
	}

	return r;
}

/**
 * sgpu_instance_data_create - creates an sgpu_instance_data
 *
 * @fpriv: DRM file private
 * @instance_handle: Pointer to the returned platform id
 *
 * The sgpu_instance_data is also added to fpriv->instance_data_handles
 * Returns 0 on success, -EINVAL or-ENOMEM on failure.
 */
static int sgpu_instance_data_create(struct amdgpu_fpriv *fpriv, uint32_t *handle)
{
	struct sgpu_instance_data *instance_data;
	int r;
	uint32_t id;

	instance_data = kzalloc(sizeof(struct sgpu_instance_data), GFP_KERNEL);
	if (!instance_data)
		return -ENOMEM;

	mutex_lock(&fpriv->instance_data_handles_lock);
	// Start from 1: treat id 0 as an invalid id
	r = idr_alloc(&fpriv->instance_data_handles, instance_data, 1, 0, GFP_KERNEL);

	if (r < 0)
		goto error_free;

	id = (uint32_t)r;

	// initialize the sgpu_instance_data
	kref_init(&instance_data->ref);
	instance_data->fpriv = fpriv;
	r = sgpu_instance_data_debugfs_add(instance_data, id);
	if (r)
		goto error_idr_remove;

	*handle = id;
	mutex_unlock(&fpriv->instance_data_handles_lock);
	return 0;

error_idr_remove:
	idr_remove(&fpriv->instance_data_handles, id);

error_free:
	mutex_unlock(&fpriv->instance_data_handles_lock);
	kfree(instance_data);
	return r;
}

/**
 * sgpu_instance_data_free - destroys an sgpu_instance_data
 *
 * @ref: Reference counter
 */
static void sgpu_instance_data_free(struct kref *ref)
{
	struct sgpu_instance_data *instance_data =
			container_of(ref, struct sgpu_instance_data, ref);

	sgpu_instance_data_debugfs_remove(instance_data);
	kfree(instance_data);
}

/**
 * sgpu_instance_data_destroy - destroys an sgpu_instance_data
 *
 * @fpriv: DRM file private
 * @handle: Handle of the sgpu_instance_data to destroy
 */
static int sgpu_instance_data_destroy(struct amdgpu_fpriv *fpriv, uint32_t handle)
{
	struct sgpu_instance_data *instance_data;

	mutex_lock(&fpriv->instance_data_handles_lock);
	instance_data = idr_remove(&fpriv->instance_data_handles, handle);

	if (instance_data)
		kref_put(&instance_data->ref, sgpu_instance_data_free);

	mutex_unlock(&fpriv->instance_data_handles_lock);
	return 0;
}

static int sgpu_instance_data_ioctl(struct drm_device *dev, void *data,
				    struct drm_file *filp)
{
	int r;
	union drm_sgpu_instance_data *args = data;
	struct amdgpu_fpriv *fpriv = filp->driver_priv;

	switch (args->in.op) {
	case SGPU_INSTANCE_DATA_OP_CREATE:
		r = sgpu_instance_data_create(fpriv, &args->out.handle);
		break;
	case SGPU_INSTANCE_DATA_OP_DESTROY:
		r = sgpu_instance_data_destroy(fpriv, args->in.handle);
		break;
	default:
		r = -EINVAL;
	}

	return r;
}

/*
 * Outdated mess for old drm with Xorg being in charge (void function now).
 */
/**
 * amdgpu_driver_lastclose_kms - drm callback for last close
 *
 * @dev: drm dev pointer
 *
 */
void amdgpu_driver_lastclose_kms(struct drm_device *dev)
{
	drm_fb_helper_lastclose(dev);
}

/**
 * amdgpu_driver_open_kms - drm callback for open
 *
 * @dev: drm dev pointer
 * @file_priv: drm file
 *
 * On device open, init vm on cayman+ (all asics).
 * Returns 0 on success, error on failure.
 */
int amdgpu_driver_open_kms(struct drm_device *dev, struct drm_file *file_priv)
{
	struct amdgpu_device *adev = drm_to_adev(dev);
	struct amdgpu_fpriv *fpriv;
	int r, pasid;

	/* Ensure IB tests are run on ring */
	flush_delayed_work(&adev->delayed_init_work);

	file_priv->driver_priv = NULL;

	if (adev->runpm) {
		r = pm_runtime_get_sync(dev->dev);
		if (r < 0)
			goto pm_put;
	}

	sgpu_ifpo_lock(adev);

	fpriv = kzalloc(sizeof(*fpriv), GFP_KERNEL);
	if (unlikely(!fpriv)) {
		r = -ENOMEM;
		goto out_suspend;
	}

	pasid = amdgpu_pasid_alloc(16);
	if (pasid < 0) {
		dev_warn(adev->dev, "No more PASIDs available!");
		pasid = 0;
	}
	r = amdgpu_vm_init(adev, &fpriv->vm, AMDGPU_VM_CONTEXT_GFX, pasid);
	if (r)
		goto error_pasid;

	fpriv->tgid = current->tgid;

	r = sgpu_proc_add_context(fpriv);
	if (r)
		goto error_proc;

	r = sgpu_worktime_init(fpriv, 0);
	if (r)
		goto error_vm;

	fpriv->prt_va = amdgpu_vm_bo_add(adev, &fpriv->vm, NULL);
	if (!fpriv->prt_va) {
		r = -ENOMEM;
		goto error_vm;
	}

	if (amdgpu_mcbp || amdgpu_sriov_vf(adev)) {
		uint64_t csa_addr = amdgpu_csa_vaddr(adev) & AMDGPU_GMC_HOLE_MASK;

		r = amdgpu_map_static_csa(adev, &fpriv->vm, adev->csa_obj,
						&fpriv->csa_va, csa_addr,
						AMDGPU_CSA_SIZE * adev->gfx.num_gfx_rings);
		if (r)
			goto error_vm;
	}

	/* Init fault recording ring (for SGPU_INFO_GPU_PAGE_FAULTS) */
	sgpu_fault_record_ring_init(&fpriv->page_fault_record);

	mutex_init(&fpriv->bo_list_lock);
	idr_init(&fpriv->bo_list_handles);

	amdgpu_ctx_mgr_init(&fpriv->ctx_mgr);
	ida_init(&fpriv->res_slots);
	mutex_init(&fpriv->lock);

	mutex_init(&fpriv->memory_lock);
	fpriv->total_pages = 0;

	mutex_init(&fpriv->instance_data_handles_lock);
	idr_init(&fpriv->instance_data_handles);

	file_priv->driver_priv = fpriv;
	fpriv->drm_file = file_priv;

	fpriv->power_hold = false;

	sgpu_vm_log_init(adev, &fpriv->vm);

	trace_amdgpu_vm_pt_base(pasid,
				amdgpu_gmc_pd_addr(fpriv->vm.root.base.bo));
	goto out_suspend;

error_vm:
	sgpu_proc_remove_context(fpriv);

error_proc:
	amdgpu_vm_fini(adev, &fpriv->vm);

error_pasid:
	if (pasid)
		amdgpu_pasid_free(pasid);

	kfree(fpriv);

out_suspend:
	sgpu_ifpo_unlock(adev);

	if (adev->runpm)
		pm_runtime_mark_last_busy(dev->dev);
pm_put:
	if (adev->runpm)
		pm_runtime_put_autosuspend(dev->dev);

	if (r)
		DRM_ERROR("%s: failed to open kms (%d)\n", __func__, r);

	return r;
}

/**
 * amdgpu_driver_postclose_kms - drm callback for post close
 *
 * @dev: drm dev pointer
 * @file_priv: drm file
 *
 * On device post close, tear down vm on cayman+ (all asics).
 */
void amdgpu_driver_postclose_kms(struct drm_device *dev,
				 struct drm_file *file_priv)
{
	struct amdgpu_device *adev = drm_to_adev(dev);
	struct amdgpu_fpriv *fpriv = file_priv->driver_priv;
	struct amdgpu_bo_list *list;
	struct amdgpu_bo *pd;
	u32 pasid;
	int handle;
	struct sgpu_instance_data *instance_data;

	if (!fpriv)
		return;

	if (adev->runpm)
		pm_runtime_get_sync(dev->dev);

	sgpu_ifpo_lock(adev);

	fpriv->vm.process_flags = current->flags;

	if (fpriv->csa_va) {
		uint64_t csa_addr = amdgpu_csa_vaddr(adev) & AMDGPU_GMC_HOLE_MASK;
		int ret;

		ret = amdgpu_unmap_static_csa(adev, &fpriv->vm, adev->csa_obj,
							fpriv->csa_va, csa_addr);
		WARN(ret < 0 && ret != -ERESTARTSYS, "error %d", ret);

		fpriv->csa_va = NULL;
	}

	pasid = fpriv->vm.pasid;
	pd = amdgpu_bo_ref(fpriv->vm.root.base.bo);
	if (!WARN_ON(amdgpu_bo_reserve(pd, true))) {
		amdgpu_vm_bo_del(adev, fpriv->prt_va);
		amdgpu_bo_unreserve(pd);
	}

	amdgpu_ctx_mgr_fini(&fpriv->ctx_mgr);
	amdgpu_vm_fini(adev, &fpriv->vm);
	ida_destroy(&fpriv->res_slots);

	if (pasid)
		amdgpu_pasid_free_delayed(pd->tbo.base.resv, pasid);
	amdgpu_bo_unref(&pd);

	idr_for_each_entry(&fpriv->bo_list_handles, list, handle)
		amdgpu_bo_list_put(list);

	sgpu_proc_remove_context(fpriv);

	idr_destroy(&fpriv->bo_list_handles);
	mutex_destroy(&fpriv->bo_list_lock);
	mutex_destroy(&fpriv->memory_lock);

	idr_for_each_entry(&fpriv->instance_data_handles, instance_data, handle)
		kref_put(&instance_data->ref, sgpu_instance_data_free);

	idr_destroy(&fpriv->instance_data_handles);
	mutex_destroy(&fpriv->instance_data_handles_lock);

	if (fpriv->power_hold) {
		sgpu_ifpo_user_disable(adev, false);

		if (adev->runpm)
			pm_runtime_put_autosuspend(dev->dev);
	}

	sgpu_worktime_deinit(fpriv);

	sgpu_vm_log_deinit(&fpriv->vm);

	kfree(fpriv);
	file_priv->driver_priv = NULL;

	sgpu_ifpo_unlock(adev);

	if (adev->runpm) {
		pm_runtime_mark_last_busy(dev->dev);
		pm_runtime_put_autosuspend(dev->dev);
	}
}

static int sgpu_mem_profile_add(struct drm_device *dev, void *data,
				struct drm_file *filp)
{
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct drm_sgpu_mem_profile_add *args = (struct drm_sgpu_mem_profile_add *)data;
	struct amdgpu_fpriv *fpriv = filp->driver_priv;

	char *buf = NULL;
	int err = 0;
	struct sgpu_instance_data *instance_data;

	if (args->len == 0 || args->len > PAGE_SIZE) {
		DRM_DEBUG_VBL("mem_profile_add: buffer too big");
		return -EINVAL;
	}

	buf = kmalloc(args->len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	err = copy_from_user(buf, u64_to_user_ptr(args->buffer), args->len);
	if (err) {
		kfree(buf);
		return -EFAULT;
	}

	mutex_lock(&fpriv->instance_data_handles_lock);
	instance_data = idr_find(&fpriv->instance_data_handles, args->instance_data_handle);
	if (instance_data && !kref_get_unless_zero(&instance_data->ref))
		instance_data = NULL;
	mutex_unlock(&fpriv->instance_data_handles_lock);

	if (!instance_data) {
		kfree(buf);
		return -EINVAL;
	}

	err = sgpu_mem_profile_debugfs_update(instance_data, buf, args->len);
	kref_put(&instance_data->ref, sgpu_instance_data_free);

	return err;
#else
	return 0;
#endif
}

static int sgpu_driver_control_power(struct drm_device *dev, void *data,
				struct drm_file *filp)
{
	struct amdgpu_device *adev = drm_to_adev(dev);
	struct sgpu_ifpo *ifpo = &adev->ifpo;
	struct amdgpu_fpriv *fpriv = filp->driver_priv;
	struct drm_sgpu_driver_control *driv_ctl = data;
	bool power_hold = driv_ctl->in_data == SGPU_POWER_HOLD_ACTIVATE;

	if (ifpo->func == NULL)
		return -ENODEV;

	if (fpriv->power_hold == power_hold)
		return -EBUSY;

	if (adev->runpm && power_hold)
		pm_runtime_get_sync(dev->dev);

	fpriv->power_hold = power_hold;
	sgpu_ifpo_user_disable(adev, power_hold);

	if (adev->runpm && !power_hold)
		pm_runtime_put_autosuspend(dev->dev);

	return 0;
}

static int sgpu_driver_control_ioctl(struct drm_device *dev, void *data,
				struct drm_file *filp)
{
	struct drm_sgpu_driver_control *driv_ctl = data;
	int r = 0;

	switch (driv_ctl->op) {
	case SGPU_DRIV_CTL_OP_POWER_HOLD:
		r = sgpu_driver_control_power(dev, data, filp);
		break;
	default:
		r = -EINVAL;
	}

	return r;
}

const struct drm_ioctl_desc amdgpu_ioctls_kms[] = {
	DRM_IOCTL_DEF_DRV(AMDGPU_GEM_CREATE, amdgpu_gem_create_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SGPU_GEM_CREATE, sgpu_gem_create_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_CTX, amdgpu_ctx_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_VM, amdgpu_vm_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_SCHED, amdgpu_sched_ioctl, DRM_MASTER),
	DRM_IOCTL_DEF_DRV(AMDGPU_BO_LIST, amdgpu_bo_list_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_FENCE_TO_HANDLE, amdgpu_cs_fence_to_handle_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	/* KMS */
	DRM_IOCTL_DEF_DRV(AMDGPU_GEM_MMAP, amdgpu_gem_mmap_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_GEM_WAIT_IDLE, amdgpu_gem_wait_idle_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_CS, amdgpu_cs_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_INFO, amdgpu_info_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_WAIT_CS, amdgpu_cs_wait_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_WAIT_FENCES, amdgpu_cs_wait_fences_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_GEM_METADATA, amdgpu_gem_metadata_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_GEM_VA, amdgpu_gem_va_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_GEM_OP, amdgpu_gem_op_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_GEM_USERPTR, amdgpu_gem_userptr_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	/* Mariner */
	DRM_IOCTL_DEF_DRV(SGPU_INSTANCE_DATA, sgpu_instance_data_ioctl, DRM_AUTH|DRM_UNLOCKED|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(AMDGPU_WGP_GATING, amdgpu_wgp_gating_ioctl, DRM_AUTH|DRM_UNLOCKED|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SGPU_MEM_PROFILE_ADD, sgpu_mem_profile_add, DRM_AUTH|DRM_UNLOCKED|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(SGPU_DRIVER_CONTROL, sgpu_driver_control_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
};
const int amdgpu_max_kms_ioctl = ARRAY_SIZE(amdgpu_ioctls_kms);

/*
 * Debugfs info
 */
#if defined(CONFIG_DEBUG_FS)

static int amdgpu_debugfs_firmware_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct amdgpu_device *adev = drm_to_adev(dev);
	struct drm_amdgpu_info_firmware fw_info;
	struct drm_amdgpu_query_fw query_fw;
	int ret, i;

	seq_printf(m, "Unified firmware version: %u.%u.%u\n",
		   adev->gfx.sgpu_fw_major_version,
		   adev->gfx.sgpu_fw_minor_version,
		   adev->gfx.sgpu_fw_option_version);

	/* GMC */
	query_fw.fw_type = AMDGPU_INFO_FW_GMC;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "MC feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* ME */
	query_fw.fw_type = AMDGPU_INFO_FW_GFX_ME;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "ME feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* PFP */
	query_fw.fw_type = AMDGPU_INFO_FW_GFX_PFP;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "PFP feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* CE */
	query_fw.fw_type = AMDGPU_INFO_FW_GFX_CE;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "CE feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* RLC */
	query_fw.fw_type = AMDGPU_INFO_FW_GFX_RLC;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "RLC feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* RLC SAVE RESTORE LIST CNTL */
	query_fw.fw_type = AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_CNTL;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "RLC SRLC feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* RLC SAVE RESTORE LIST GPM MEM */
	query_fw.fw_type = AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_GPM_MEM;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "RLC SRLG feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* RLC SAVE RESTORE LIST SRM MEM */
	query_fw.fw_type = AMDGPU_INFO_FW_GFX_RLC_RESTORE_LIST_SRM_MEM;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "RLC SRLS feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* MEC */
	query_fw.fw_type = AMDGPU_INFO_FW_GFX_MEC;
	query_fw.index = 0;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "MEC feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* MEC2 */
	if (adev->gfx.mec2_fw) {
		query_fw.index = 1;
		ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
		if (ret)
			return ret;
		seq_printf(m, "MEC2 feature version: %u, firmware version: 0x%08x\n",
			   fw_info.feature, fw_info.ver);
	}

	/* SMC */
	query_fw.fw_type = AMDGPU_INFO_FW_SMC;
	ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
	if (ret)
		return ret;
	seq_printf(m, "SMC feature version: %u, firmware version: 0x%08x\n",
		   fw_info.feature, fw_info.ver);

	/* SDMA */
	query_fw.fw_type = AMDGPU_INFO_FW_SDMA;
	for (i = 0; i < adev->sdma.num_instances; i++) {
		query_fw.index = i;
		ret = amdgpu_firmware_info(&fw_info, &query_fw, adev);
		if (ret)
			return ret;
		seq_printf(m, "SDMA%d feature version: %u, firmware version: 0x%08x\n",
			   i, fw_info.feature, fw_info.ver);
	}

	return 0;
}

static const struct drm_info_list amdgpu_firmware_info_list[] = {
	{"amdgpu_firmware_info", amdgpu_debugfs_firmware_info, 0, NULL},
};
#endif

int amdgpu_debugfs_firmware_init(struct amdgpu_device *adev)
{
#if defined(CONFIG_DEBUG_FS)
	return amdgpu_debugfs_add_files(adev, amdgpu_firmware_info_list,
					ARRAY_SIZE(amdgpu_firmware_info_list));
#else
	return 0;
#endif
}
