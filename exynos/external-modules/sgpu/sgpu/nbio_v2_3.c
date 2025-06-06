/*
 * Copyright 2019 Advanced Micro Devices, Inc.
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
#include "nbio_v2_3.h"

#include "nbio/nbio_2_3_offset.h"
#include "nbio/nbio_2_3_sh_mask.h"
#include "gc/gc_10_4_0_offset.h"
#include <uapi/linux/kfd_ioctl.h>

#define mmBIF_SDMA2_DOORBELL_RANGE		0x01d6
#define mmBIF_SDMA2_DOORBELL_RANGE_BASE_IDX	2
#define mmBIF_SDMA3_DOORBELL_RANGE		0x01d7
#define mmBIF_SDMA3_DOORBELL_RANGE_BASE_IDX	2

#define mmBIF_MMSCH1_DOORBELL_RANGE		0x01d8
#define mmBIF_MMSCH1_DOORBELL_RANGE_BASE_IDX	2

static void nbio_v2_3_remap_hdp_registers(struct amdgpu_device *adev)
{
	WREG32_SOC15(NBIO, 0, mmREMAP_HDP_MEM_FLUSH_CNTL,
		adev->rmmio_remap.reg_offset + KFD_MMIO_REMAP_HDP_MEM_FLUSH_CNTL);
	WREG32_SOC15(NBIO, 0, mmREMAP_HDP_REG_FLUSH_CNTL,
		adev->rmmio_remap.reg_offset + KFD_MMIO_REMAP_HDP_REG_FLUSH_CNTL);
}

static u32 nbio_v2_3_get_rev_id(struct amdgpu_device *adev)
{
	u32 tmp = RREG32_SOC15(NBIO, 0, mmRCC_DEV0_EPF0_STRAP0);

	tmp = REG_GET_FIELD(tmp, RCC_DEV0_EPF0_STRAP0, STRAP_ATI_REV_ID_DEV0_F0);

	return tmp;
}

static u32 nbio_v2_3_get_chip_revision(struct amdgpu_device *adev)
{
	return RREG32_SOC15(GC, 0, mmGRBM_CHIP_REVISION);
}

static void nbio_v2_3_mc_access_enable(struct amdgpu_device *adev, bool enable)
{
	if (enable)
		WREG32_SOC15(NBIO, 0, mmBIF_FB_EN,
			     BIF_FB_EN__FB_READ_EN_MASK |
			     BIF_FB_EN__FB_WRITE_EN_MASK);
	else
		WREG32_SOC15(NBIO, 0, mmBIF_FB_EN, 0);
}

static void nbio_v2_3_hdp_flush(struct amdgpu_device *adev,
				struct amdgpu_ring *ring)
{
	/* Vangogh Lite does not have HDP */
	/* Not removing function, it is called in several places
	 * without nullptr check */
	if (adev->gmc.aper_size == 0)
		return;

	if (!ring || !ring->funcs->emit_wreg)
		WREG32_NO_KIQ((adev->rmmio_remap.reg_offset + KFD_MMIO_REMAP_HDP_MEM_FLUSH_CNTL) >> 2, 0);
	else
		amdgpu_ring_emit_wreg(ring, (adev->rmmio_remap.reg_offset + KFD_MMIO_REMAP_HDP_MEM_FLUSH_CNTL) >> 2, 0);
}

static u32 nbio_v2_3_get_memsize(struct amdgpu_device *adev)
{
	return 0;
}

static void nbio_v2_3_sdma_doorbell_range(struct amdgpu_device *adev, int instance,
					  bool use_doorbell, int doorbell_index,
					  int doorbell_size)
{
	u32 reg = instance == 0 ? SOC15_REG_OFFSET(NBIO, 0, mmBIF_SDMA0_DOORBELL_RANGE) :
			instance == 1 ? SOC15_REG_OFFSET(NBIO, 0, mmBIF_SDMA1_DOORBELL_RANGE) :
			instance == 2 ? SOC15_REG_OFFSET(NBIO, 0, mmBIF_SDMA2_DOORBELL_RANGE) :
			SOC15_REG_OFFSET(NBIO, 0, mmBIF_SDMA3_DOORBELL_RANGE);

	u32 doorbell_range = RREG32(reg);

	if (use_doorbell) {
		doorbell_range = REG_SET_FIELD(doorbell_range,
					       BIF_SDMA0_DOORBELL_RANGE, OFFSET,
					       doorbell_index);
		doorbell_range = REG_SET_FIELD(doorbell_range,
					       BIF_SDMA0_DOORBELL_RANGE, SIZE,
					       doorbell_size);
	} else
		doorbell_range = REG_SET_FIELD(doorbell_range,
					       BIF_SDMA0_DOORBELL_RANGE, SIZE,
					       0);

	WREG32(reg, doorbell_range);
}

static void nbio_v2_3_vcn_doorbell_range(struct amdgpu_device *adev, bool use_doorbell,
					 int doorbell_index, int instance)
{
	u32 reg = instance ? SOC15_REG_OFFSET(NBIO, 0, mmBIF_MMSCH1_DOORBELL_RANGE) :
		SOC15_REG_OFFSET(NBIO, 0, mmBIF_MMSCH0_DOORBELL_RANGE);

	u32 doorbell_range = RREG32(reg);

	if (use_doorbell) {
		doorbell_range = REG_SET_FIELD(doorbell_range,
					       BIF_MMSCH0_DOORBELL_RANGE, OFFSET,
					       doorbell_index);
		doorbell_range = REG_SET_FIELD(doorbell_range,
					       BIF_MMSCH0_DOORBELL_RANGE, SIZE, 8);
	} else
		doorbell_range = REG_SET_FIELD(doorbell_range,
					       BIF_MMSCH0_DOORBELL_RANGE, SIZE, 0);

	WREG32(reg, doorbell_range);
}

static void nbio_v2_3_enable_doorbell_aperture(struct amdgpu_device *adev,
					       bool enable)
{
	WREG32_FIELD15(NBIO, 0, RCC_DEV0_EPF0_RCC_DOORBELL_APER_EN, BIF_DOORBELL_APER_EN,
		       enable ? 1 : 0);
}

static void nbio_v2_3_enable_doorbell_selfring_aperture(struct amdgpu_device *adev,
							bool enable)
{
	u32 tmp = 0;

	if (enable) {
		tmp = REG_SET_FIELD(tmp, BIF_BX_PF_DOORBELL_SELFRING_GPA_APER_CNTL,
				    DOORBELL_SELFRING_GPA_APER_EN, 1) |
		      REG_SET_FIELD(tmp, BIF_BX_PF_DOORBELL_SELFRING_GPA_APER_CNTL,
				    DOORBELL_SELFRING_GPA_APER_MODE, 1) |
		      REG_SET_FIELD(tmp, BIF_BX_PF_DOORBELL_SELFRING_GPA_APER_CNTL,
				    DOORBELL_SELFRING_GPA_APER_SIZE, 0);

		WREG32_SOC15(NBIO, 0, mmBIF_BX_PF_DOORBELL_SELFRING_GPA_APER_BASE_LOW,
			     lower_32_bits(adev->doorbell.base));
		WREG32_SOC15(NBIO, 0, mmBIF_BX_PF_DOORBELL_SELFRING_GPA_APER_BASE_HIGH,
			     upper_32_bits(adev->doorbell.base));
	}

	WREG32_SOC15(NBIO, 0, mmBIF_BX_PF_DOORBELL_SELFRING_GPA_APER_CNTL,
		     tmp);
}


static void nbio_v2_3_ih_doorbell_range(struct amdgpu_device *adev,
					bool use_doorbell, int doorbell_index)
{
	u32 ih_doorbell_range = RREG32_SOC15(NBIO, 0, mmBIF_IH_DOORBELL_RANGE);

	if (use_doorbell) {
		ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range,
						  BIF_IH_DOORBELL_RANGE, OFFSET,
						  doorbell_index);
		ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range,
						  BIF_IH_DOORBELL_RANGE, SIZE,
						  2);
	} else
		ih_doorbell_range = REG_SET_FIELD(ih_doorbell_range,
						  BIF_IH_DOORBELL_RANGE, SIZE,
						  0);

	WREG32_SOC15(NBIO, 0, mmBIF_IH_DOORBELL_RANGE, ih_doorbell_range);
}

static void nbio_v2_3_ih_control(struct amdgpu_device *adev)
{
	u32 interrupt_cntl;

	/* setup interrupt control */
	WREG32_SOC15(NBIO, 0, mmINTERRUPT_CNTL2, adev->dummy_page_addr >> 8);

	interrupt_cntl = RREG32_SOC15(NBIO, 0, mmINTERRUPT_CNTL);
	/*
	 * INTERRUPT_CNTL__IH_DUMMY_RD_OVERRIDE_MASK=0 - dummy read disabled with msi, enabled without msi
	 * INTERRUPT_CNTL__IH_DUMMY_RD_OVERRIDE_MASK=1 - dummy read controlled by IH_DUMMY_RD_EN
	 */
	interrupt_cntl = REG_SET_FIELD(interrupt_cntl, INTERRUPT_CNTL,
				       IH_DUMMY_RD_OVERRIDE, 0);

	/* INTERRUPT_CNTL__IH_REQ_NONSNOOP_EN_MASK=1 if ring is in non-cacheable memory, e.g., vram */
	interrupt_cntl = REG_SET_FIELD(interrupt_cntl, INTERRUPT_CNTL,
				       IH_REQ_NONSNOOP_EN, 0);

	WREG32_SOC15(NBIO, 0, mmINTERRUPT_CNTL, interrupt_cntl);
}

static void nbio_v2_3_update_medium_grain_clock_gating(struct amdgpu_device *adev,
						       bool enable)
{
	/* no implementation for SGPU */
	return;
}

static void nbio_v2_3_update_medium_grain_light_sleep(struct amdgpu_device *adev,
						      bool enable)
{
	/* no implementation for SGPU */
	return;
}

static void nbio_v2_3_get_clockgating_state(struct amdgpu_device *adev,
					    u64 *flags)
{
	/* no implementation for SGPU */
	return;
}

static u32 nbio_v2_3_get_hdp_flush_req_offset(struct amdgpu_device *adev)
{
	return SOC15_REG_OFFSET(NBIO, 0, mmBIF_BX_PF_GPU_HDP_FLUSH_REQ);
}

static u32 nbio_v2_3_get_hdp_flush_done_offset(struct amdgpu_device *adev)
{
	return SOC15_REG_OFFSET(NBIO, 0, mmBIF_BX_PF_GPU_HDP_FLUSH_DONE);
}

static u32 nbio_v2_3_get_pcie_index_offset(struct amdgpu_device *adev)
{
	/* no implementation for SGPU */
	return 0;
}

static u32 nbio_v2_3_get_pcie_data_offset(struct amdgpu_device *adev)
{
	/* no implementation for SGPU */
	return 0;
}

const struct nbio_hdp_flush_reg nbio_v2_3_hdp_flush_reg = {
	.ref_and_mask_cp0 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP0_MASK,
	.ref_and_mask_cp1 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP1_MASK,
	.ref_and_mask_cp2 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP2_MASK,
	.ref_and_mask_cp3 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP3_MASK,
	.ref_and_mask_cp4 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP4_MASK,
	.ref_and_mask_cp5 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP5_MASK,
	.ref_and_mask_cp6 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP6_MASK,
	.ref_and_mask_cp7 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP7_MASK,
	.ref_and_mask_cp8 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP8_MASK,
	.ref_and_mask_cp9 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__CP9_MASK,
	.ref_and_mask_sdma0 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__SDMA0_MASK,
	.ref_and_mask_sdma1 = BIF_BX_PF_GPU_HDP_FLUSH_DONE__SDMA1_MASK,
};

static void nbio_v2_3_init_registers(struct amdgpu_device *adev)
{
	/* nothing to do for SGPU */
	return;
}

const struct amdgpu_nbio_funcs nbio_v2_3_funcs = {
	.get_hdp_flush_req_offset = nbio_v2_3_get_hdp_flush_req_offset,
	.get_hdp_flush_done_offset = nbio_v2_3_get_hdp_flush_done_offset,
	.get_pcie_index_offset = nbio_v2_3_get_pcie_index_offset,
	.get_pcie_data_offset = nbio_v2_3_get_pcie_data_offset,
	.get_rev_id = nbio_v2_3_get_rev_id,
	.get_chip_revision = nbio_v2_3_get_chip_revision,
	.mc_access_enable = nbio_v2_3_mc_access_enable,
	.hdp_flush = nbio_v2_3_hdp_flush,
	.get_memsize = nbio_v2_3_get_memsize,
	.sdma_doorbell_range = nbio_v2_3_sdma_doorbell_range,
	.vcn_doorbell_range = nbio_v2_3_vcn_doorbell_range,
	.enable_doorbell_aperture = nbio_v2_3_enable_doorbell_aperture,
	.enable_doorbell_selfring_aperture = nbio_v2_3_enable_doorbell_selfring_aperture,
	.ih_doorbell_range = nbio_v2_3_ih_doorbell_range,
	.update_medium_grain_clock_gating = nbio_v2_3_update_medium_grain_clock_gating,
	.update_medium_grain_light_sleep = nbio_v2_3_update_medium_grain_light_sleep,
	.get_clockgating_state = nbio_v2_3_get_clockgating_state,
	.ih_control = nbio_v2_3_ih_control,
	.init_registers = nbio_v2_3_init_registers,
	.remap_hdp_registers = nbio_v2_3_remap_hdp_registers,
};
