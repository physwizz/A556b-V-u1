/*
 * Copyright 2019 - 2021 Advanced Micro Devices, Inc.
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
 */
#include "amdgpu.h"
#include "gfxhub_v2_1.h"

#include "gc/gc_10_4_0_offset.h"
#include "gc/gc_10_4_0_sh_mask.h"
#include "gc/gc_10_4_0_default.h"
#include "navi10_enum.h"

#include "soc15_common.h"

static const char *gfxhub_client_ids[] = {
	"CB/DB",
	"Reserved",
	"GE1",
	"GE2",
	"CPF",
	"CPC",
	"CPG",
	"RLC",
	"TCP",
	"SQC (inst)",
	"SQC (data)",
	"SQG",
	"Reserved",
	"SDMA0",
	"SDMA1",
	"GCR",
	"SDMA2",
	"SDMA3",
};

static uint32_t gfxhub_v2_1_get_invalidate_req(unsigned int vmid,
					       uint32_t flush_type)
{
	u32 req = 0;

	/* invalidate using legacy mode on vmid*/
	req = REG_SET_FIELD(req, GCVM_INVALIDATE_ENG0_REQ,
			    PER_VMID_INVALIDATE_REQ, 1 << vmid);
	req = REG_SET_FIELD(req, GCVM_INVALIDATE_ENG0_REQ, FLUSH_TYPE, flush_type);
	req = REG_SET_FIELD(req, GCVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PTES, 1);
	req = REG_SET_FIELD(req, GCVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE0, 1);
	req = REG_SET_FIELD(req, GCVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE1, 1);
	req = REG_SET_FIELD(req, GCVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE2, 1);
	req = REG_SET_FIELD(req, GCVM_INVALIDATE_ENG0_REQ, INVALIDATE_L1_PTES, 1);
	req = REG_SET_FIELD(req, GCVM_INVALIDATE_ENG0_REQ,
			    CLEAR_PROTECTION_FAULT_STATUS_ADDR,	0);

	return req;
}

static void
gfxhub_v2_1_print_l2_protection_fault_status(struct amdgpu_device *adev,
					     uint32_t status)
{
	u32 cid = REG_GET_FIELD(status,
				GCVM_L2_PROTECTION_FAULT_STATUS, CID);

	dev_err(adev->dev,
		"GCVM_L2_PROTECTION_FAULT_STATUS:0x%08X\n",
		status);
	dev_err(adev->dev, "\t Faulty UTCL2 client ID: %s (0x%x)\n",
		cid >= ARRAY_SIZE(gfxhub_client_ids) ? "unknown" : gfxhub_client_ids[cid],
		cid);
	dev_err(adev->dev, "\t MORE_FAULTS: 0x%lx\n",
		REG_GET_FIELD(status,
		GCVM_L2_PROTECTION_FAULT_STATUS, MORE_FAULTS));
	dev_err(adev->dev, "\t WALKER_ERROR: 0x%lx\n",
		REG_GET_FIELD(status,
		GCVM_L2_PROTECTION_FAULT_STATUS, WALKER_ERROR));
	dev_err(adev->dev, "\t PERMISSION_FAULTS: 0x%lx\n",
		REG_GET_FIELD(status,
		GCVM_L2_PROTECTION_FAULT_STATUS, PERMISSION_FAULTS));
	dev_err(adev->dev, "\t MAPPING_ERROR: 0x%lx\n",
		REG_GET_FIELD(status,
		GCVM_L2_PROTECTION_FAULT_STATUS, MAPPING_ERROR));
	dev_err(adev->dev, "\t RW: 0x%lx\n",
		REG_GET_FIELD(status,
		GCVM_L2_PROTECTION_FAULT_STATUS, RW));
}

static u64 gfxhub_v2_1_get_fb_location(struct amdgpu_device *adev)
{
	u64 base = RREG32_SOC15(GC, 0, mmGCMC_VM_FB_LOCATION_BASE);

	base &= GCMC_VM_FB_LOCATION_BASE__FB_BASE_MASK;
	base <<= 24;

	return base;
}

static u64 gfxhub_v2_1_get_mc_fb_offset(struct amdgpu_device *adev)
{
	return (u64)RREG32_SOC15(GC, 0, mmGCMC_VM_FB_OFFSET) << 24;
}

static void gfxhub_v2_1_setup_vm_pt_regs(struct amdgpu_device *adev, uint32_t vmid,
				uint64_t page_table_base)
{
	struct amdgpu_vmhub *hub = &adev->vmhub[AMDGPU_GFXHUB_0];

	WREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32,
			    hub->ctx_addr_distance * vmid,
			    lower_32_bits(page_table_base));

	WREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32,
			    hub->ctx_addr_distance * vmid,
			    upper_32_bits(page_table_base));
}

static void gfxhub_v2_1_init_gart_aperture_regs(struct amdgpu_device *adev)
{
	uint64_t pt_base = 0;

	/* 1) if set AMDGPU_PTE_SYSTEM bit and
	 *    GCVM_L2_CNTL4:*_REQUEST_PHYSICAL
	 *    utcl2 accesses datawith IO=1.
	 * 2) if set AMDGPU_PTE_SYSTEM bit and
	 *    GCVM_L2_CNTL4:*_REQUEST_PHYSICAL
	 *    utcl2 accesses data with IO=0.
	 * 3) if GCVM_L2_CNTL4:*_REQUEST_PHYSICAL is 0,
	 *    utcl2 accesses data with IO=0.
	 */
	pt_base &= ~AMDGPU_PTE_SYSTEM;
	pt_base |= AMDGPU_PTE_VALID;
	pt_base |= AMDGPU_PTE_SNOOPED;
	pt_base |= adev->csm_gart_paddr;

	gfxhub_v2_1_setup_vm_pt_regs(adev, 0, pt_base);

	WREG32_SOC15(GC, 0, mmGCVM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32,
		     (u32)(adev->gmc.gart_start >> 12));
	WREG32_SOC15(GC, 0, mmGCVM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32,
		     (u32)(adev->gmc.gart_start >> 44));

	WREG32_SOC15(GC, 0, mmGCVM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32,
		     (u32)(adev->gmc.gart_end >> 12));
	WREG32_SOC15(GC, 0, mmGCVM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32,
		     (u32)(adev->gmc.gart_end >> 44));
}

static void gfxhub_v2_1_init_system_aperture_regs(struct amdgpu_device *adev)
{
	uint64_t value;
	uint64_t tmr_base = adev->gfx.rlc.rlc_autoload_gpu_addr;

	/* Disable AGP. */
	WREG32_SOC15(GC, 0, mmGCMC_VM_AGP_BASE, 0);
	WREG32_SOC15(GC, 0, mmGCMC_VM_AGP_TOP, 0);
	WREG32_SOC15(GC, 0, mmGCMC_VM_AGP_BOT, 0x00FFFFFF);

	/* Disable LFB */
	WREG32_SOC15(GC, 0, mmGCMC_VM_SYSTEM_APERTURE_LOW_ADDR,
						(tmr_base >> 24) << 6);
	WREG32_SOC15(GC, 0, mmGCMC_VM_SYSTEM_APERTURE_HIGH_ADDR,
						((tmr_base >> 24) + 1) << 6);
	WREG32_SOC15(GC, 0, mmGCMC_VM_FB_LOCATION_BASE, (tmr_base >> 24));
	WREG32_SOC15(GC, 0, mmGCMC_VM_FB_LOCATION_TOP, (tmr_base >> 24) + 1);
	WREG32_SOC15(GC, 0, mmGCMC_VM_FB_OFFSET, (tmr_base >> 24));

	value = adev->sysram_scratch_p;

	WREG32_SOC15(GC, 0, mmGCMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_LSB,
		     (u32)(value >> 12));
	WREG32_SOC15(GC, 0, mmGCMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_MSB,
		     (u32)(value >> 44));

	/* Program "protection fault". */
	WREG32_SOC15(GC, 0, mmGCVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_LO32,
		     (u32)(adev->dummy_page_addr >> 12));
	WREG32_SOC15(GC, 0, mmGCVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_HI32,
		     (u32)((u64)adev->dummy_page_addr >> 44));

	WREG32_FIELD15(GC, 0, GCVM_L2_PROTECTION_FAULT_CNTL2,
		       ACTIVE_PAGE_MIGRATION_PTE_READ_RETRY, 1);
}


static void gfxhub_v2_1_init_tlb_regs(struct amdgpu_device *adev)
{
	uint32_t tmp;

	/* Setup TLB control */
	tmp = RREG32_SOC15(GC, 0, mmGCMC_VM_MX_L1_TLB_CNTL);

	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL,
			    ENABLE_ADVANCED_DRIVER_MODEL, 1);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL,
			    SYSTEM_APERTURE_UNMAPPED_ACCESS, 0);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, ECO_BITS, 0);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL,
			    MTYPE, MTYPE_UC); /* UC, uncached */

	WREG32_SOC15(GC, 0, mmGCMC_VM_MX_L1_TLB_CNTL, tmp);
}

static void gfxhub_v2_1_init_cache_regs(struct amdgpu_device *adev)
{
	uint32_t tmp;

	/* These registers are not accessible to VF-SRIOV.
	 * The PF will program them instead.
	 */
	if (amdgpu_sriov_vf(adev))
		return;

	/* Setup L2 cache */
	tmp = RREG32_SOC15(GC, 0, mmGCVM_L2_CNTL);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, ENABLE_L2_CACHE, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, ENABLE_L2_FRAGMENT_PROCESSING, 0);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL,
			    ENABLE_DEFAULT_PAGE_OUT_TO_SYSTEM_MEMORY, 1);
	/* XXX for emulation, Refer to closed source code.*/
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL,
			    L2_PDE0_CACHE_TAG_GENERATION_MODE, 0);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, PDE_FAULT_CLASSIFICATION, 0);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, CONTEXT1_IDENTITY_ACCESS_MODE, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL, IDENTITY_MODE_FRAGMENT_SIZE, 0);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CNTL, tmp);

	tmp = RREG32_SOC15(GC, 0, mmGCVM_L2_CNTL2);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL2, INVALIDATE_ALL_L1_TLBS, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL2, INVALIDATE_L2_CACHE, 1);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CNTL2, tmp);

	tmp = mmGCVM_L2_CNTL3_DEFAULT;
	/* For mariner, bigK is configured as 4K, BANK_SELECT and
		* BIGK_FRAGMENT_SIZE need to be configured to use bigK 4K cache
		*/
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL3, BANK_SELECT, 3);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL3,
			L2_CACHE_BIGK_FRAGMENT_SIZE, 0);
	/* L2_CACHE_4K_ASSOCIATIVITY is obsolete, clear the default
		*/
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL3,
			L2_CACHE_4K_ASSOCIATIVITY, 0);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CNTL3, tmp);

	tmp = mmGCVM_L2_CNTL4_DEFAULT;
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL4, VMC_TAP_PDE_REQUEST_PHYSICAL, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL4, VMC_TAP_PTE_REQUEST_PHYSICAL, 1);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CNTL4, tmp);

	tmp = mmGCVM_L2_CNTL5_DEFAULT;
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CNTL5, L2_CACHE_SMALLK_FRAGMENT_SIZE, 0);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CNTL5, tmp);

	tmp = mmGCVM_L2_CACHE_PARITY_CNTL_DEFAULT;
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CACHE_PARITY_CNTL,
			    ENABLE_PARITY_CHECKS_IN_4K_PTE_CACHES, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CACHE_PARITY_CNTL,
			    ENABLE_PARITY_CHECKS_IN_BIGK_PTE_CACHES, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_CACHE_PARITY_CNTL,
			    ENABLE_PARITY_CHECKS_IN_PDE_CACHES, 1);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CACHE_PARITY_CNTL, tmp);
}

static void gfxhub_v2_1_init_direct_system_access(struct amdgpu_device *adev)
{
	u32 tmp;

	/*
	 * set top of DRAM1 to be 4G
	 */
	tmp = RREG32_SOC15(GC, 0, mmGCMC_VM_NB_TOP_OF_DRAM_SLOT1);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_NB_TOP_OF_DRAM_SLOT1,
			TOP_OF_DRAM, 0x1ff);
	WREG32_SOC15(GC, 0, mmGCMC_VM_NB_TOP_OF_DRAM_SLOT1, tmp);

	/*
	 * set top of DRAM2 to be 16G
	 */
	tmp = 0;
	tmp = REG_SET_FIELD(tmp, GCMC_VM_NB_UPPER_TOP_OF_DRAM2,
			    UPPER_TOM2, 0x4);
	WREG32_SOC15(GC, 0, mmGCMC_VM_NB_UPPER_TOP_OF_DRAM2, tmp);
	tmp = 0;
	tmp = REG_SET_FIELD(tmp, GCMC_VM_NB_LOWER_TOP_OF_DRAM2,
			    ENABLE, 0x1);
	WREG32_SOC15(GC, 0, mmGCMC_VM_NB_LOWER_TOP_OF_DRAM2, tmp);

	/* enable direct system access */
	tmp = RREG32_SOC15(GC, 0, mmGCMC_VM_APT_CNTL);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_APT_CNTL,
			DIRECT_SYSTEM_EN, 1);
	WREG32_SOC15(GC, 0, mmGCMC_VM_APT_CNTL, tmp);
}

static void gfxhub_v2_1_enable_system_domain(struct amdgpu_device *adev)
{
	uint32_t tmp;

	tmp = RREG32_SOC15(GC, 0, mmGCVM_CONTEXT0_CNTL);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, ENABLE_CONTEXT, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL, PAGE_TABLE_DEPTH, 0);
	tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT0_CNTL,
			    RETRY_PERMISSION_OR_INVALID_PAGE_FAULT, 0);
	WREG32_SOC15(GC, 0, mmGCVM_CONTEXT0_CNTL, tmp);
}

static void gfxhub_v2_1_disable_identity_aperture(struct amdgpu_device *adev)
{
	/* These registers are not accessible to VF-SRIOV.
	 * The PF will program them instead.
	 */
	if (amdgpu_sriov_vf(adev))
		return;

	WREG32_SOC15(GC, 0, mmGCVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_LO32,
		     0xFFFFFFFF);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_HI32,
		     0x0000000F);

	WREG32_SOC15(GC, 0, mmGCVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_LO32,
		     0);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_HI32,
		     0);

	WREG32_SOC15(GC, 0, mmGCVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_LO32, 0);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_HI32, 0);

}

static void gfxhub_v2_1_setup_vmid_config(struct amdgpu_device *adev)
{
	struct amdgpu_vmhub *hub = &adev->vmhub[AMDGPU_GFXHUB_0];
	int i;
	uint32_t tmp;

	for (i = 0; i <= 14; i++) {
		tmp = RREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT1_CNTL, i);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL, ENABLE_CONTEXT, 1);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL, PAGE_TABLE_DEPTH,
				    adev->vm_manager.num_level);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				DUMMY_PAGE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				PDE0_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				VALID_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				READ_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				WRITE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				EXECUTE_PROTECTION_FAULT_ENABLE_DEFAULT, 1);
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				PAGE_TABLE_BLOCK_SIZE,
				adev->vm_manager.block_size - 9);
		/* Send no-retry XNACK on fault to suppress VM fault storm. */
		tmp = REG_SET_FIELD(tmp, GCVM_CONTEXT1_CNTL,
				    RETRY_PERMISSION_OR_INVALID_PAGE_FAULT,
				    !adev->gmc.noretry);
		WREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT1_CNTL,
				    i * hub->ctx_distance, tmp);
		WREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT1_PAGE_TABLE_START_ADDR_LO32,
				    i * hub->ctx_addr_distance, 0);
		WREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT1_PAGE_TABLE_START_ADDR_HI32,
				    i * hub->ctx_addr_distance, 0);
		WREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT1_PAGE_TABLE_END_ADDR_LO32,
				    i * hub->ctx_addr_distance,
				    lower_32_bits(adev->vm_manager.max_pfn - 1));
		WREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT1_PAGE_TABLE_END_ADDR_HI32,
				    i * hub->ctx_addr_distance,
				    upper_32_bits(adev->vm_manager.max_pfn - 1));
	}
}

static void gfxhub_v2_1_program_invalidation(struct amdgpu_device *adev)
{
	struct amdgpu_vmhub *hub = &adev->vmhub[AMDGPU_GFXHUB_0];
	unsigned i;

	for (i = 0 ; i < 18; ++i) {
		WREG32_SOC15_OFFSET(GC, 0, mmGCVM_INVALIDATE_ENG0_ADDR_RANGE_LO32,
				    i * hub->eng_addr_distance, 0xffffffff);
		WREG32_SOC15_OFFSET(GC, 0, mmGCVM_INVALIDATE_ENG0_ADDR_RANGE_HI32,
				    i * hub->eng_addr_distance, 0x1f);
	}
}

static int gfxhub_v2_1_gart_enable(struct amdgpu_device *adev)
{
	if (amdgpu_sriov_vf(adev)) {
		/*
		 * GCMC_VM_FB_LOCATION_BASE/TOP is NULL for VF, becuase they are
		 * VF copy registers so vbios post doesn't program them, for
		 * SRIOV driver need to program them
		 */
		WREG32_SOC15(GC, 0, mmGCMC_VM_FB_LOCATION_BASE,
			     adev->gmc.vram_start >> 24);
		WREG32_SOC15(GC, 0, mmGCMC_VM_FB_LOCATION_TOP,
			     adev->gmc.vram_end >> 24);
	}

	/* GART Enable. */
	gfxhub_v2_1_init_gart_aperture_regs(adev);
	gfxhub_v2_1_init_system_aperture_regs(adev);
	gfxhub_v2_1_init_tlb_regs(adev);
	gfxhub_v2_1_init_cache_regs(adev);
	gfxhub_v2_1_init_direct_system_access(adev);

	gfxhub_v2_1_enable_system_domain(adev);
	gfxhub_v2_1_disable_identity_aperture(adev);
	gfxhub_v2_1_setup_vmid_config(adev);
	gfxhub_v2_1_program_invalidation(adev);

	return 0;
}

static void gfxhub_v2_1_gart_disable(struct amdgpu_device *adev)
{
	struct amdgpu_vmhub *hub = &adev->vmhub[AMDGPU_GFXHUB_0];
	u32 tmp;
	u32 i;

	/* Disable all tables */
	for (i = 0; i < 16; i++)
		WREG32_SOC15_OFFSET(GC, 0, mmGCVM_CONTEXT0_CNTL,
				    i * hub->ctx_distance, 0);

	/* Setup TLB control */
	tmp = RREG32_SOC15(GC, 0, mmGCMC_VM_MX_L1_TLB_CNTL);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 0);
	tmp = REG_SET_FIELD(tmp, GCMC_VM_MX_L1_TLB_CNTL,
			    ENABLE_ADVANCED_DRIVER_MODEL, 0);
	WREG32_SOC15(GC, 0, mmGCMC_VM_MX_L1_TLB_CNTL, tmp);

	/* Setup L2 cache */
	WREG32_FIELD15(GC, 0, GCVM_L2_CNTL, ENABLE_L2_CACHE, 0);
	WREG32_SOC15(GC, 0, mmGCVM_L2_CNTL3, 0);
}

/**
 * gfxhub_v2_1_set_fault_enable_default - update GART/VM fault handling
 *
 * @adev: amdgpu_device pointer
 * @value: true redirects VM faults to the default page
 */
static void gfxhub_v2_1_set_fault_enable_default(struct amdgpu_device *adev,
					  bool value)
{
	u32 tmp;

	/* These registers are not accessible to VF-SRIOV.
	 * The PF will program them instead.
	 */
	if (amdgpu_sriov_vf(adev))
		return;

	tmp = RREG32_SOC15(GC, 0, mmGCVM_L2_PROTECTION_FAULT_CNTL);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    PDE0_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    PDE1_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    PDE2_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    TRANSLATE_FURTHER_PROTECTION_FAULT_ENABLE_DEFAULT,
			    value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    NACK_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    DUMMY_PAGE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    VALID_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    READ_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    WRITE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    EXECUTE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
				CRASH_ON_NO_RETRY_FAULT, 1);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
				CRASH_ON_RETRY_FAULT, 1);
	/* Suggested by HW team, these fiels are enabled by default, just keep
	 * the default value, otherwise, the VMC page fault will not be
	 * triggered */
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    CLIENT_ID_NO_RETRY_FAULT_INTERRUPT,
			    0xFFFF);
	tmp = REG_SET_FIELD(tmp, GCVM_L2_PROTECTION_FAULT_CNTL,
			    OTHER_CLIENT_ID_NO_RETRY_FAULT_INTERRUPT,
			    1);

	WREG32_SOC15(GC, 0, mmGCVM_L2_PROTECTION_FAULT_CNTL, tmp);
}

static void gfxhub_v2_1_parity_errors_show(struct amdgpu_device *adev)
{
	u32 sts = RREG32_SOC15(GC, 0, mmGCVM_L2_STATUS);

	dev_err(adev->dev, "4K PTE cache parity errors %sfound\n",
		REG_GET_FIELD(sts, GCVM_L2_STATUS,
			      FOUND_4K_PTE_CACHE_PARITY_ERRORS) ? "": "not ");
	dev_err(adev->dev, "BIGK PTE cache parity errors %sfound\n",
		REG_GET_FIELD(sts, GCVM_L2_STATUS,
			      FOUND_BIGK_PTE_CACHE_PARITY_ERRORS) ? "": "not ");
	dev_err(adev->dev, "PDE0 cache parity errors %sfound\n",
		REG_GET_FIELD(sts, GCVM_L2_STATUS,
			      FOUND_PDE0_CACHE_PARITY_ERRORS) ? "": "not ");
	dev_err(adev->dev, "PDE1 cache parity errors %sfound\n",
		REG_GET_FIELD(sts, GCVM_L2_STATUS,
			      FOUND_PDE1_CACHE_PARITY_ERRORS) ? "": "not ");
	dev_err(adev->dev, "PDE2 cache parity errors %sfound\n",
		REG_GET_FIELD(sts, GCVM_L2_STATUS,
			      FOUND_PDE2_CACHE_PARITY_ERRORS) ? "": "not ");
}

static const struct amdgpu_vmhub_funcs gfxhub_v2_1_vmhub_funcs = {
	.print_l2_protection_fault_status = gfxhub_v2_1_print_l2_protection_fault_status,
	.get_invalidate_req = gfxhub_v2_1_get_invalidate_req,
	.parity_errors_show = gfxhub_v2_1_parity_errors_show,
};

static void gfxhub_v2_1_init(struct amdgpu_device *adev)
{
	struct amdgpu_vmhub *hub = &adev->vmhub[AMDGPU_GFXHUB_0];

	hub->ctx0_ptb_addr_lo32 =
		SOC15_REG_OFFSET(GC, 0,
				 mmGCVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32);
	hub->ctx0_ptb_addr_hi32 =
		SOC15_REG_OFFSET(GC, 0,
				 mmGCVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32);
	hub->vm_inv_eng0_sem =
		SOC15_REG_OFFSET(GC, 0, mmGCVM_INVALIDATE_ENG0_SEM);
	hub->vm_inv_eng0_req =
		SOC15_REG_OFFSET(GC, 0, mmGCVM_INVALIDATE_ENG0_REQ);
	hub->vm_inv_eng0_ack =
		SOC15_REG_OFFSET(GC, 0, mmGCVM_INVALIDATE_ENG0_ACK);
	hub->vm_context0_cntl =
		SOC15_REG_OFFSET(GC, 0, mmGCVM_CONTEXT0_CNTL);
	hub->vm_l2_pro_fault_status =
		SOC15_REG_OFFSET(GC, 0, mmGCVM_L2_PROTECTION_FAULT_STATUS);
	hub->vm_l2_pro_fault_cntl =
		SOC15_REG_OFFSET(GC, 0, mmGCVM_L2_PROTECTION_FAULT_CNTL);
	hub->ctx_distance = mmGCVM_CONTEXT1_CNTL - mmGCVM_CONTEXT0_CNTL;
	hub->ctx_addr_distance = mmGCVM_CONTEXT1_PAGE_TABLE_BASE_ADDR_LO32 -
		mmGCVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32;
	hub->eng_distance = mmGCVM_INVALIDATE_ENG1_REQ -
		mmGCVM_INVALIDATE_ENG0_REQ;
	hub->eng_addr_distance = mmGCVM_INVALIDATE_ENG1_ADDR_RANGE_LO32 -
		mmGCVM_INVALIDATE_ENG0_ADDR_RANGE_LO32;

	hub->vm_cntx_cntl_vm_fault = GCVM_CONTEXT1_CNTL__RANGE_PROTECTION_FAULT_ENABLE_INTERRUPT_MASK |
		GCVM_CONTEXT1_CNTL__DUMMY_PAGE_PROTECTION_FAULT_ENABLE_INTERRUPT_MASK |
		GCVM_CONTEXT1_CNTL__PDE0_PROTECTION_FAULT_ENABLE_INTERRUPT_MASK |
		GCVM_CONTEXT1_CNTL__VALID_PROTECTION_FAULT_ENABLE_INTERRUPT_MASK |
		GCVM_CONTEXT1_CNTL__READ_PROTECTION_FAULT_ENABLE_INTERRUPT_MASK |
		GCVM_CONTEXT1_CNTL__WRITE_PROTECTION_FAULT_ENABLE_INTERRUPT_MASK |
		GCVM_CONTEXT1_CNTL__EXECUTE_PROTECTION_FAULT_ENABLE_INTERRUPT_MASK;

	hub->vmhub_funcs = &gfxhub_v2_1_vmhub_funcs;

	hub->vm_l2_pro_fault_addr_lo32 =
		SOC15_REG_OFFSET(GC, 0, mmGCVM_L2_PROTECTION_FAULT_ADDR_LO32);
	hub->vm_l2_pro_fault_addr_hi32 =
		SOC15_REG_OFFSET(GC, 0, mmGCVM_L2_PROTECTION_FAULT_ADDR_HI32);
}

const struct amdgpu_gfxhub_funcs gfxhub_v2_1_funcs = {
	.get_fb_location = gfxhub_v2_1_get_fb_location,
	.get_mc_fb_offset = gfxhub_v2_1_get_mc_fb_offset,
	.setup_vm_pt_regs = gfxhub_v2_1_setup_vm_pt_regs,
	.gart_enable = gfxhub_v2_1_gart_enable,
	.gart_disable = gfxhub_v2_1_gart_disable,
	.set_fault_enable_default = gfxhub_v2_1_set_fault_enable_default,
	.init = gfxhub_v2_1_init,
};
