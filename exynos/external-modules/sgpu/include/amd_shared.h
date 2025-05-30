/*
 * Copyright 2015 - 2020 Advanced Micro Devices, Inc.
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
 */

#ifndef __AMD_SHARED_H__
#define __AMD_SHARED_H__

#include <linux/types.h>

#define AMD_MAX_USEC_TIMEOUT		1000000  /* 1000 ms */

/**
* DOC: IP Blocks
*
* GPUs are composed of IP (intellectual property) blocks. These
* IP blocks provide various functionalities: display, graphics,
* video decode, etc. The IP blocks that comprise a particular GPU
* are listed in the GPU's respective SoC file. amdgpu_device.c
* acquires the list of IP blocks for the GPU in use on initialization.
* It can then operate on this list to perform standard driver operations
* such as: init, fini, suspend, resume, etc.
*
*
* IP block implementations are named using the following convention:
* <functionality>_v<version> (E.g.: gfx_v6_0).
*/

/**
* enum amd_ip_block_type - Used to classify IP blocks by functionality.
*
* @AMD_IP_BLOCK_TYPE_COMMON: GPU Family
* @AMD_IP_BLOCK_TYPE_GMC: Graphics Memory Controller
* @AMD_IP_BLOCK_TYPE_IH: Interrupt Handler
* @AMD_IP_BLOCK_TYPE_SMC: System Management Controller
* @AMD_IP_BLOCK_TYPE_PSP: Platform Security Processor
* @AMD_IP_BLOCK_TYPE_DCE: Display and Compositing Engine
* @AMD_IP_BLOCK_TYPE_GFX: Graphics and Compute Engine
* @AMD_IP_BLOCK_TYPE_SDMA: System DMA Engine
* @AMD_IP_BLOCK_TYPE_UVD: Unified Video Decoder
* @AMD_IP_BLOCK_TYPE_VCE: Video Compression Engine
* @AMD_IP_BLOCK_TYPE_ACP: Audio Co-Processor
* @AMD_IP_BLOCK_TYPE_VCN: Video Core/Codec Next
* @AMD_IP_BLOCK_TYPE_MES: Micro-Engine Scheduler
* @AMD_IP_BLOCK_TYPE_JPEG: JPEG Engine
*/
enum amd_ip_block_type {
	AMD_IP_BLOCK_TYPE_COMMON,
	AMD_IP_BLOCK_TYPE_GMC,
	AMD_IP_BLOCK_TYPE_IH,
	AMD_IP_BLOCK_TYPE_SMC,
	AMD_IP_BLOCK_TYPE_PSP,
	AMD_IP_BLOCK_TYPE_DCE,
	AMD_IP_BLOCK_TYPE_GFX,
	AMD_IP_BLOCK_TYPE_SDMA,
	AMD_IP_BLOCK_TYPE_UVD,
	AMD_IP_BLOCK_TYPE_VCE,
	AMD_IP_BLOCK_TYPE_ACP,
	AMD_IP_BLOCK_TYPE_VCN,
	AMD_IP_BLOCK_TYPE_MES,
	AMD_IP_BLOCK_TYPE_JPEG
};

enum amd_clockgating_state {
	AMD_CG_STATE_GATE = 0,
	AMD_CG_STATE_UNGATE,
};


enum amd_powergating_state {
	AMD_PG_STATE_GATE = 0,
	AMD_PG_STATE_UNGATE,
};


/* CG flags */
#define AMD_CG_SUPPORT_GFX_MGCG			(1 << 0)
#define AMD_CG_SUPPORT_GFX_MGLS			(1 << 1)
#define AMD_CG_SUPPORT_GFX_CGCG			(1 << 2)
#define AMD_CG_SUPPORT_GFX_CGLS			(1 << 3)
#define AMD_CG_SUPPORT_GFX_CGTS			(1 << 4)
#define AMD_CG_SUPPORT_GFX_CGTS_LS		(1 << 5)
#define AMD_CG_SUPPORT_GFX_CP_LS		(1 << 6)
#define AMD_CG_SUPPORT_GFX_RLC_LS		(1 << 7)
#define AMD_CG_SUPPORT_MC_LS			(1 << 8)
#define AMD_CG_SUPPORT_MC_MGCG			(1 << 9)
#define AMD_CG_SUPPORT_SDMA_LS			(1 << 10)
#define AMD_CG_SUPPORT_SDMA_MGCG		(1 << 11)
#define AMD_CG_SUPPORT_BIF_LS			(1 << 12)
#define AMD_CG_SUPPORT_UVD_MGCG			(1 << 13)
#define AMD_CG_SUPPORT_VCE_MGCG			(1 << 14)
#define AMD_CG_SUPPORT_HDP_LS			(1 << 15)
#define AMD_CG_SUPPORT_HDP_MGCG			(1 << 16)
#define AMD_CG_SUPPORT_ROM_MGCG			(1 << 17)
#define AMD_CG_SUPPORT_DRM_LS			(1 << 18)
#define AMD_CG_SUPPORT_BIF_MGCG			(1 << 19)
#define AMD_CG_SUPPORT_GFX_3D_CGCG		(1 << 20)
#define AMD_CG_SUPPORT_GFX_3D_CGLS		(1 << 21)
#define AMD_CG_SUPPORT_DRM_MGCG			(1 << 22)
#define AMD_CG_SUPPORT_DF_MGCG			(1 << 23)
#define AMD_CG_SUPPORT_VCN_MGCG			(1 << 24)
#define AMD_CG_SUPPORT_HDP_DS			(1 << 25)
#define AMD_CG_SUPPORT_HDP_SD			(1 << 26)
#define AMD_CG_SUPPORT_IH_CG			(1 << 27)
#define AMD_CG_SUPPORT_ATHUB_LS			(1 << 28)
#define AMD_CG_SUPPORT_ATHUB_MGCG		(1 << 29)
#define AMD_CG_SUPPORT_GFX_STATIC_WGP		(1 << 30)
#define AMD_CG_SUPPORT_GFX_FGCG			(1 << 31)
/* PG flags */
#define AMD_PG_SUPPORT_GFX_PG			(1 << 0)
#define AMD_PG_SUPPORT_GFX_SMG			(1 << 1)
#define AMD_PG_SUPPORT_GFX_DMG			(1 << 2)
#define AMD_PG_SUPPORT_UVD			(1 << 3)
#define AMD_PG_SUPPORT_VCE			(1 << 4)
#define AMD_PG_SUPPORT_CP			(1 << 5)
#define AMD_PG_SUPPORT_GDS			(1 << 6)
#define AMD_PG_SUPPORT_RLC_SMU_HS		(1 << 7)
#define AMD_PG_SUPPORT_SDMA			(1 << 8)
#define AMD_PG_SUPPORT_ACP			(1 << 9)
#define AMD_PG_SUPPORT_SAMU			(1 << 10)
#define AMD_PG_SUPPORT_GFX_QUICK_MG		(1 << 11)
#define AMD_PG_SUPPORT_GFX_PIPELINE		(1 << 12)
#define AMD_PG_SUPPORT_MMHUB			(1 << 13)
#define AMD_PG_SUPPORT_VCN			(1 << 14)
#define AMD_PG_SUPPORT_VCN_DPG			(1 << 15)
#define AMD_PG_SUPPORT_ATHUB			(1 << 16)
#define AMD_PG_SUPPORT_JPEG			(1 << 17)

/**
 * enum PP_FEATURE_MASK - Used to mask power play features.
 *
 * @PP_SCLK_DPM_MASK: Dynamic adjustment of the system (graphics) clock.
 * @PP_MCLK_DPM_MASK: Dynamic adjustment of the memory clock.
 * @PP_PCIE_DPM_MASK: Dynamic adjustment of PCIE clocks and lanes.
 * @PP_SCLK_DEEP_SLEEP_MASK: System (graphics) clock deep sleep.
 * @PP_POWER_CONTAINMENT_MASK: Power containment.
 * @PP_UVD_HANDSHAKE_MASK: Unified video decoder handshake.
 * @PP_SMC_VOLTAGE_CONTROL_MASK: Dynamic voltage control.
 * @PP_VBI_TIME_SUPPORT_MASK: Vertical blank interval support.
 * @PP_ULV_MASK: Ultra low voltage.
 * @PP_ENABLE_GFX_CG_THRU_SMU: SMU control of GFX engine clockgating.
 * @PP_CLOCK_STRETCH_MASK: Clock stretching.
 * @PP_OD_FUZZY_FAN_CONTROL_MASK: Overdrive fuzzy fan control.
 * @PP_SOCCLK_DPM_MASK: Dynamic adjustment of the SoC clock.
 * @PP_DCEFCLK_DPM_MASK: Dynamic adjustment of the Display Controller Engine Fabric clock.
 * @PP_OVERDRIVE_MASK: Over- and under-clocking support.
 * @PP_GFXOFF_MASK: Dynamic graphics engine power control.
 * @PP_ACG_MASK: Adaptive clock generator.
 * @PP_STUTTER_MODE: Stutter mode.
 * @PP_AVFS_MASK: Adaptive voltage and frequency scaling.
 *
 * To override these settings on boot, append amdgpu.ppfeaturemask=<mask> to
 * the kernel's command line parameters. This is usually done through a system's
 * boot loader (E.g. GRUB). If manually loading the driver, pass
 * ppfeaturemask=<mask> as a modprobe parameter.
 */
enum PP_FEATURE_MASK {
	PP_SCLK_DPM_MASK = 0x1,
	PP_MCLK_DPM_MASK = 0x2,
	PP_PCIE_DPM_MASK = 0x4,
	PP_SCLK_DEEP_SLEEP_MASK = 0x8,
	PP_POWER_CONTAINMENT_MASK = 0x10,
	PP_UVD_HANDSHAKE_MASK = 0x20,
	PP_SMC_VOLTAGE_CONTROL_MASK = 0x40,
	PP_VBI_TIME_SUPPORT_MASK = 0x80,
	PP_ULV_MASK = 0x100,
	PP_ENABLE_GFX_CG_THRU_SMU = 0x200,
	PP_CLOCK_STRETCH_MASK = 0x400,
	PP_OD_FUZZY_FAN_CONTROL_MASK = 0x800,
	PP_SOCCLK_DPM_MASK = 0x1000,
	PP_DCEFCLK_DPM_MASK = 0x2000,
	PP_OVERDRIVE_MASK = 0x4000,
	PP_GFXOFF_MASK = 0x8000,
	PP_ACG_MASK = 0x10000,
	PP_STUTTER_MODE = 0x20000,
	PP_AVFS_MASK = 0x40000,
};

enum DC_FEATURE_MASK {
	DC_FBC_MASK = 0x1,
	DC_MULTI_MON_PP_MCLK_SWITCH_MASK = 0x2,
	DC_DISABLE_FRACTIONAL_PWM_MASK = 0x4,
	DC_PSR_MASK = 0x8,
};

enum DC_DEBUG_MASK {
	DC_DISABLE_PIPE_SPLIT = 0x1,
	DC_DISABLE_STUTTER = 0x2,
	DC_DISABLE_DSC = 0x4,
	DC_DISABLE_CLOCK_GATING = 0x8
};

enum amd_dpm_forced_level;

/**
 * struct amd_ip_funcs - general hooks for managing amdgpu IP Blocks
 * @name: Name of IP block
 * @early_init: sets up early driver state (pre sw_init),
 *              does not configure hw - Optional
 * @late_init: sets up late driver/hw state (post hw_init) - Optional
 * @sw_init: sets up driver state, does not configure hw
 * @sw_fini: tears down driver state, does not configure hw
 * @hw_init: sets up the hw state
 * @hw_fini: tears down the hw state
 * @late_fini: final cleanup
 * @suspend: handles IP specific hw/sw changes for suspend
 * @resume: handles IP specific hw/sw changes for resume
 * @is_idle: returns current IP block idle status
 * @wait_for_idle: poll for idle
 * @check_soft_reset: check soft reset the IP block
 * @pre_soft_reset: pre soft reset the IP block
 * @soft_reset: soft reset the IP block
 * @post_soft_reset: post soft reset the IP block
 * @set_clockgating_state: enable/disable cg for the IP block
 * @set_powergating_state: enable/disable pg for the IP block
 * @get_clockgating_state: get current clockgating status
 * @enable_umd_pstate: enable UMD powerstate
 *
 * These hooks provide an interface for controlling the operational state
 * of IP blocks. After acquiring a list of IP blocks for the GPU in use,
 * the driver can make chip-wide state changes by walking this list and
 * making calls to hooks from each IP block. This list is ordered to ensure
 * that the driver initializes the IP blocks in a safe sequence.
 */
struct amd_ip_funcs {
	char *name;
	int (*early_init)(void *handle);
	int (*late_init)(void *handle);
	int (*fw_init)(void *handle);
	int (*sw_init)(void *handle);
	int (*sw_fini)(void *handle);
	int (*hw_init)(void *handle);
	int (*hw_fini)(void *handle);
	void (*late_fini)(void *handle);
	int (*suspend)(void *handle);
	int (*resume)(void *handle);
	bool (*is_idle)(void *handle);
	int (*wait_for_idle)(void *handle);
	bool (*check_soft_reset)(void *handle);
	int (*pre_soft_reset)(void *handle);
	int (*soft_reset)(void *handle);
	int (*post_soft_reset)(void *handle);
	int (*set_clockgating_state)(void *handle,
				     enum amd_clockgating_state state);
	int (*set_powergating_state)(void *handle,
				     enum amd_powergating_state state);
	void (*get_clockgating_state)(void *handle, u64 *flags);
	int (*enable_umd_pstate)(void *handle, enum amd_dpm_forced_level *level);
	/** @is_power_on: return current IP block power status */
	bool (*is_power_on)(void *handle);
	/* @fault_detect: return current IP block fault detected status */
	bool (*fault_detect)(void *handle, unsigned long flags);
};


#endif /* __AMD_SHARED_H__ */
