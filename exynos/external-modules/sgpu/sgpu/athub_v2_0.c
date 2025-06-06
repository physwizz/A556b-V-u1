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
#include "athub_v2_0.h"

#include "athub/athub_2_0_0_offset.h"
#include "athub/athub_2_0_0_sh_mask.h"
#include "athub/athub_2_0_0_default.h"
#include "navi10_enum.h"

#include "soc15_common.h"

static void
athub_v2_0_update_medium_grain_clock_gating(struct amdgpu_device *adev,
					    bool enable)
{
	uint32_t def, data;

	if (!(adev->cg_flags & AMD_CG_SUPPORT_MC_MGCG))
		return;

	def = data = RREG32_SOC15(ATHUB, 0, mmATHUB_MISC_CNTL);

	if (enable)
		data |= ATHUB_MISC_CNTL__CG_ENABLE_MASK;
	else
		data &= ~ATHUB_MISC_CNTL__CG_ENABLE_MASK;

	if (def != data)
		WREG32_SOC15(ATHUB, 0, mmATHUB_MISC_CNTL, data);
}

static void
athub_v2_0_update_medium_grain_light_sleep(struct amdgpu_device *adev,
					   bool enable)
{
	uint32_t def, data;

	if (!((adev->cg_flags & AMD_CG_SUPPORT_MC_LS) &&
	       (adev->cg_flags & AMD_CG_SUPPORT_HDP_LS)))
		return;

	def = data = RREG32_SOC15(ATHUB, 0, mmATHUB_MISC_CNTL);

	if (enable)
		data |= ATHUB_MISC_CNTL__CG_MEM_LS_ENABLE_MASK;
	else
		data &= ~ATHUB_MISC_CNTL__CG_MEM_LS_ENABLE_MASK;

	if (def != data)
		WREG32_SOC15(ATHUB, 0, mmATHUB_MISC_CNTL, data);
}

int athub_v2_0_set_clockgating(struct amdgpu_device *adev,
			       enum amd_clockgating_state state)
{
	if (amdgpu_sriov_vf(adev))
		return 0;

	switch (adev->ip_versions[ATHUB_HWIP][0]) {
	case IP_VERSION(1, 3, 1):
	case IP_VERSION(2, 0, 0):
	case IP_VERSION(2, 0, 2):
		athub_v2_0_update_medium_grain_clock_gating(adev,
				state == AMD_CG_STATE_GATE);
		athub_v2_0_update_medium_grain_light_sleep(adev,
				state == AMD_CG_STATE_GATE);
		break;
	default:
		break;
	}

	return 0;
}

void athub_v2_0_get_clockgating(struct amdgpu_device *adev, u64 *flags)
{
	int data;

	/* AMD_CG_SUPPORT_ATHUB_MGCG */
	data = RREG32_SOC15(ATHUB, 0, mmATHUB_MISC_CNTL);
	if (data & ATHUB_MISC_CNTL__CG_ENABLE_MASK)
		*flags |= AMD_CG_SUPPORT_ATHUB_MGCG;

	/* AMD_CG_SUPPORT_ATHUB_LS */
	if (data & ATHUB_MISC_CNTL__CG_MEM_LS_ENABLE_MASK)
		*flags |= AMD_CG_SUPPORT_ATHUB_LS;
}
