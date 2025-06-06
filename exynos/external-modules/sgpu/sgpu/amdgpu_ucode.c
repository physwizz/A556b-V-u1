/*
 * Copyright 2014 Advanced Micro Devices, Inc.
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

#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "amdgpu.h"
#include "amdgpu_ucode.h"

static void amdgpu_ucode_print_common_hdr(const struct common_firmware_header *hdr)
{
	DRM_DEBUG("size_bytes: %u\n", le32_to_cpu(hdr->size_bytes));
	DRM_DEBUG("header_size_bytes: %u\n", le32_to_cpu(hdr->header_size_bytes));
	DRM_DEBUG("header_version_major: %u\n", le16_to_cpu(hdr->header_version_major));
	DRM_DEBUG("header_version_minor: %u\n", le16_to_cpu(hdr->header_version_minor));
	DRM_DEBUG("ip_version_major: %u\n", le16_to_cpu(hdr->ip_version_major));
	DRM_DEBUG("ip_version_minor: %u\n", le16_to_cpu(hdr->ip_version_minor));
	DRM_DEBUG("ucode_version: 0x%08x\n", le32_to_cpu(hdr->ucode_version));
	DRM_DEBUG("ucode_size_bytes: %u\n", le32_to_cpu(hdr->ucode_size_bytes));
	DRM_DEBUG("ucode_array_offset_bytes: %u\n",
		  le32_to_cpu(hdr->ucode_array_offset_bytes));
	DRM_DEBUG("crc32: 0x%08x\n", le32_to_cpu(hdr->crc32));
}

void amdgpu_ucode_print_mc_hdr(const struct common_firmware_header *hdr)
{
	uint16_t version_major = le16_to_cpu(hdr->header_version_major);
	uint16_t version_minor = le16_to_cpu(hdr->header_version_minor);

	DRM_DEBUG("MC\n");
	amdgpu_ucode_print_common_hdr(hdr);

	if (version_major == 1) {
		const struct mc_firmware_header_v1_0 *mc_hdr =
			container_of(hdr, struct mc_firmware_header_v1_0, header);

		DRM_DEBUG("io_debug_size_bytes: %u\n",
			  le32_to_cpu(mc_hdr->io_debug_size_bytes));
		DRM_DEBUG("io_debug_array_offset_bytes: %u\n",
			  le32_to_cpu(mc_hdr->io_debug_array_offset_bytes));
	} else {
		DRM_ERROR("Unknown MC ucode version: %u.%u\n", version_major, version_minor);
	}
}

void amdgpu_ucode_print_smc_hdr(const struct common_firmware_header *hdr)
{
	uint16_t version_major = le16_to_cpu(hdr->header_version_major);
	uint16_t version_minor = le16_to_cpu(hdr->header_version_minor);

	DRM_DEBUG("SMC\n");
	amdgpu_ucode_print_common_hdr(hdr);

	if (version_major == 1) {
		const struct smc_firmware_header_v1_0 *smc_hdr =
			container_of(hdr, struct smc_firmware_header_v1_0, header);

		DRM_DEBUG("ucode_start_addr: %u\n", le32_to_cpu(smc_hdr->ucode_start_addr));
	} else if (version_major == 2) {
		const struct smc_firmware_header_v1_0 *v1_hdr =
			container_of(hdr, struct smc_firmware_header_v1_0, header);
		const struct smc_firmware_header_v2_0 *v2_hdr =
			container_of(v1_hdr, struct smc_firmware_header_v2_0, v1_0);

		DRM_DEBUG("ppt_offset_bytes: %u\n", le32_to_cpu(v2_hdr->ppt_offset_bytes));
		DRM_DEBUG("ppt_size_bytes: %u\n", le32_to_cpu(v2_hdr->ppt_size_bytes));
	} else {
		DRM_ERROR("Unknown SMC ucode version: %u.%u\n", version_major, version_minor);
	}
}

void amdgpu_ucode_print_gfx_hdr(const struct common_firmware_header *hdr)
{
	uint16_t version_major = le16_to_cpu(hdr->header_version_major);
	uint16_t version_minor = le16_to_cpu(hdr->header_version_minor);

	DRM_DEBUG("GFX\n");
	amdgpu_ucode_print_common_hdr(hdr);

	if (version_major == 1) {
		const struct gfx_firmware_header_v1_0 *gfx_hdr =
			container_of(hdr, struct gfx_firmware_header_v1_0, header);

		DRM_DEBUG("ucode_feature_version: %u\n",
			  le32_to_cpu(gfx_hdr->ucode_feature_version));
		DRM_DEBUG("jt_offset: %u\n", le32_to_cpu(gfx_hdr->jt_offset));
		DRM_DEBUG("jt_size: %u\n", le32_to_cpu(gfx_hdr->jt_size));
	} else {
		DRM_ERROR("Unknown GFX ucode version: %u.%u\n", version_major, version_minor);
	}
}

void amdgpu_ucode_print_rlc_hdr(const struct common_firmware_header *hdr)
{
	uint16_t version_major = le16_to_cpu(hdr->header_version_major);
	uint16_t version_minor = le16_to_cpu(hdr->header_version_minor);

	DRM_DEBUG("RLC\n");
	amdgpu_ucode_print_common_hdr(hdr);

	if (version_major == 1) {
		const struct rlc_firmware_header_v1_0 *rlc_hdr =
			container_of(hdr, struct rlc_firmware_header_v1_0, header);

		DRM_DEBUG("ucode_feature_version: %u\n",
			  le32_to_cpu(rlc_hdr->ucode_feature_version));
		DRM_DEBUG("save_and_restore_offset: %u\n",
			  le32_to_cpu(rlc_hdr->save_and_restore_offset));
		DRM_DEBUG("clear_state_descriptor_offset: %u\n",
			  le32_to_cpu(rlc_hdr->clear_state_descriptor_offset));
		DRM_DEBUG("avail_scratch_ram_locations: %u\n",
			  le32_to_cpu(rlc_hdr->avail_scratch_ram_locations));
		DRM_DEBUG("master_pkt_description_offset: %u\n",
			  le32_to_cpu(rlc_hdr->master_pkt_description_offset));
	} else if (version_major == 2) {
		const struct rlc_firmware_header_v2_0 *rlc_hdr =
			container_of(hdr, struct rlc_firmware_header_v2_0, header);

		DRM_DEBUG("ucode_feature_version: %u\n",
			  le32_to_cpu(rlc_hdr->ucode_feature_version));
		DRM_DEBUG("jt_offset: %u\n", le32_to_cpu(rlc_hdr->jt_offset));
		DRM_DEBUG("jt_size: %u\n", le32_to_cpu(rlc_hdr->jt_size));
		DRM_DEBUG("save_and_restore_offset: %u\n",
			  le32_to_cpu(rlc_hdr->save_and_restore_offset));
		DRM_DEBUG("clear_state_descriptor_offset: %u\n",
			  le32_to_cpu(rlc_hdr->clear_state_descriptor_offset));
		DRM_DEBUG("avail_scratch_ram_locations: %u\n",
			  le32_to_cpu(rlc_hdr->avail_scratch_ram_locations));
		DRM_DEBUG("reg_restore_list_size: %u\n",
			  le32_to_cpu(rlc_hdr->reg_restore_list_size));
		DRM_DEBUG("reg_list_format_start: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_format_start));
		DRM_DEBUG("reg_list_format_separate_start: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_format_separate_start));
		DRM_DEBUG("starting_offsets_start: %u\n",
			  le32_to_cpu(rlc_hdr->starting_offsets_start));
		DRM_DEBUG("reg_list_format_size_bytes: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_format_size_bytes));
		DRM_DEBUG("reg_list_format_array_offset_bytes: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_format_array_offset_bytes));
		DRM_DEBUG("reg_list_size_bytes: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_size_bytes));
		DRM_DEBUG("reg_list_array_offset_bytes: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_array_offset_bytes));
		DRM_DEBUG("reg_list_format_separate_size_bytes: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_format_separate_size_bytes));
		DRM_DEBUG("reg_list_format_separate_array_offset_bytes: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_format_separate_array_offset_bytes));
		DRM_DEBUG("reg_list_separate_size_bytes: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_separate_size_bytes));
		DRM_DEBUG("reg_list_separate_array_offset_bytes: %u\n",
			  le32_to_cpu(rlc_hdr->reg_list_separate_array_offset_bytes));
		if (version_minor == 1) {
			const struct rlc_firmware_header_v2_1 *v2_1 =
				container_of(rlc_hdr, struct rlc_firmware_header_v2_1, v2_0);
			DRM_DEBUG("reg_list_format_direct_reg_list_length: %u\n",
				  le32_to_cpu(v2_1->reg_list_format_direct_reg_list_length));
			DRM_DEBUG("save_restore_list_cntl_ucode_ver: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_cntl_ucode_ver));
			DRM_DEBUG("save_restore_list_cntl_feature_ver: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_cntl_feature_ver));
			DRM_DEBUG("save_restore_list_cntl_size_bytes %u\n",
				  le32_to_cpu(v2_1->save_restore_list_cntl_size_bytes));
			DRM_DEBUG("save_restore_list_cntl_offset_bytes: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_cntl_offset_bytes));
			DRM_DEBUG("save_restore_list_gpm_ucode_ver: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_gpm_ucode_ver));
			DRM_DEBUG("save_restore_list_gpm_feature_ver: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_gpm_feature_ver));
			DRM_DEBUG("save_restore_list_gpm_size_bytes %u\n",
				  le32_to_cpu(v2_1->save_restore_list_gpm_size_bytes));
			DRM_DEBUG("save_restore_list_gpm_offset_bytes: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_gpm_offset_bytes));
			DRM_DEBUG("save_restore_list_srm_ucode_ver: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_srm_ucode_ver));
			DRM_DEBUG("save_restore_list_srm_feature_ver: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_srm_feature_ver));
			DRM_DEBUG("save_restore_list_srm_size_bytes %u\n",
				  le32_to_cpu(v2_1->save_restore_list_srm_size_bytes));
			DRM_DEBUG("save_restore_list_srm_offset_bytes: %u\n",
				  le32_to_cpu(v2_1->save_restore_list_srm_offset_bytes));
		}
	} else {
		DRM_ERROR("Unknown RLC ucode version: %u.%u\n", version_major, version_minor);
	}
}

void amdgpu_ucode_print_sdma_hdr(const struct common_firmware_header *hdr)
{
	uint16_t version_major = le16_to_cpu(hdr->header_version_major);
	uint16_t version_minor = le16_to_cpu(hdr->header_version_minor);

	DRM_DEBUG("SDMA\n");
	amdgpu_ucode_print_common_hdr(hdr);

	if (version_major == 1) {
		const struct sdma_firmware_header_v1_0 *sdma_hdr =
			container_of(hdr, struct sdma_firmware_header_v1_0, header);

		DRM_DEBUG("ucode_feature_version: %u\n",
			  le32_to_cpu(sdma_hdr->ucode_feature_version));
		DRM_DEBUG("ucode_change_version: %u\n",
			  le32_to_cpu(sdma_hdr->ucode_change_version));
		DRM_DEBUG("jt_offset: %u\n", le32_to_cpu(sdma_hdr->jt_offset));
		DRM_DEBUG("jt_size: %u\n", le32_to_cpu(sdma_hdr->jt_size));
		if (version_minor >= 1) {
			const struct sdma_firmware_header_v1_1 *sdma_v1_1_hdr =
				container_of(sdma_hdr, struct sdma_firmware_header_v1_1, v1_0);
			DRM_DEBUG("digest_size: %u\n", le32_to_cpu(sdma_v1_1_hdr->digest_size));
		}
	} else {
		DRM_ERROR("Unknown SDMA ucode version: %u.%u\n",
			  version_major, version_minor);
	}
}

void amdgpu_ucode_print_gpu_info_hdr(const struct common_firmware_header *hdr)
{
	uint16_t version_major = le16_to_cpu(hdr->header_version_major);
	uint16_t version_minor = le16_to_cpu(hdr->header_version_minor);

	DRM_DEBUG("GPU_INFO\n");
	amdgpu_ucode_print_common_hdr(hdr);

	if (version_major == 1) {
		const struct gpu_info_firmware_header_v1_0 *gpu_info_hdr =
			container_of(hdr, struct gpu_info_firmware_header_v1_0, header);

		DRM_DEBUG("version_major: %u\n",
			  le16_to_cpu(gpu_info_hdr->version_major));
		DRM_DEBUG("version_minor: %u\n",
			  le16_to_cpu(gpu_info_hdr->version_minor));
	} else {
		DRM_ERROR("Unknown gpu_info ucode version: %u.%u\n", version_major, version_minor);
	}
}

int amdgpu_ucode_validate(const struct firmware *fw)
{
	const struct common_firmware_header *hdr =
		(const struct common_firmware_header *)fw->data;

	if (fw->size == le32_to_cpu(hdr->size_bytes))
		return 0;

	return -EINVAL;
}

bool amdgpu_ucode_hdr_version(union amdgpu_firmware_header *hdr,
				uint16_t hdr_major, uint16_t hdr_minor)
{
	if ((hdr->common.header_version_major == hdr_major) &&
		(hdr->common.header_version_minor == hdr_minor))
		return false;
	return true;
}

enum amdgpu_firmware_load_type
amdgpu_ucode_get_load_type(struct amdgpu_device *adev, int load_type)
{
	return AMDGPU_FW_LOAD_RLC_BACKDOOR_AUTO;
}

#define FW_VERSION_ATTR(name, mode, field)				\
static ssize_t show_##name(struct device *dev,				\
			  struct device_attribute *attr,		\
			  char *buf)					\
{									\
	struct drm_device *ddev = dev_get_drvdata(dev);			\
	struct amdgpu_device *adev = drm_to_adev(ddev);			\
									\
	return snprintf(buf, PAGE_SIZE, "0x%08x\n", adev->field);	\
}									\
static DEVICE_ATTR(name, mode, show_##name, NULL)

FW_VERSION_ATTR(mc_fw_version, 0444, gmc.fw_version);
FW_VERSION_ATTR(me_fw_version, 0444, gfx.me_fw_version);
FW_VERSION_ATTR(pfp_fw_version, 0444, gfx.pfp_fw_version);
FW_VERSION_ATTR(ce_fw_version, 0444, gfx.ce_fw_version);
FW_VERSION_ATTR(rlc_fw_version, 0444, gfx.rlc_fw_version);
FW_VERSION_ATTR(rlc_srlc_fw_version, 0444, gfx.rlc_srlc_fw_version);
FW_VERSION_ATTR(rlc_srlg_fw_version, 0444, gfx.rlc_srlg_fw_version);
FW_VERSION_ATTR(rlc_srls_fw_version, 0444, gfx.rlc_srls_fw_version);
FW_VERSION_ATTR(mec_fw_version, 0444, gfx.mec_fw_version);
FW_VERSION_ATTR(mec2_fw_version, 0444, gfx.mec2_fw_version);
FW_VERSION_ATTR(smc_fw_version, 0444, pm.fw_version);
FW_VERSION_ATTR(sdma_fw_version, 0444, sdma.instance[0].fw_version);
FW_VERSION_ATTR(sdma2_fw_version, 0444, sdma.instance[1].fw_version);
FW_VERSION_ATTR(sgpu_rtl_cl_number, 0444, gfx.sgpu_rtl_cl_number);

static ssize_t show_sgpu_fw_version(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	struct amdgpu_device *adev = drm_to_adev(ddev);

	return snprintf(buf, PAGE_SIZE, "%u.%u.%u\n",
		adev->gfx.sgpu_fw_major_version,
		adev->gfx.sgpu_fw_minor_version,
		adev->gfx.sgpu_fw_option_version);
}
static DEVICE_ATTR(sgpu_fw_version, 0444, show_sgpu_fw_version, NULL);


static struct attribute *fw_attrs[] = {
	&dev_attr_mc_fw_version.attr, &dev_attr_me_fw_version.attr,
	&dev_attr_pfp_fw_version.attr, &dev_attr_ce_fw_version.attr,
	&dev_attr_rlc_fw_version.attr, &dev_attr_rlc_srlc_fw_version.attr,
	&dev_attr_rlc_srlg_fw_version.attr, &dev_attr_rlc_srls_fw_version.attr,
	&dev_attr_mec_fw_version.attr, &dev_attr_mec2_fw_version.attr,
	&dev_attr_smc_fw_version.attr, &dev_attr_sdma_fw_version.attr,
	&dev_attr_sdma2_fw_version.attr, &dev_attr_sgpu_rtl_cl_number.attr,
	&dev_attr_sgpu_fw_version.attr,
	NULL
};

static const struct attribute_group fw_attr_group = {
	.name = "fw_version",
	.attrs = fw_attrs
};

int amdgpu_ucode_sysfs_init(struct amdgpu_device *adev)
{
	return sysfs_create_group(&adev->dev->kobj, &fw_attr_group);
}

void amdgpu_ucode_sysfs_fini(struct amdgpu_device *adev)
{
	sysfs_remove_group(&adev->dev->kobj, &fw_attr_group);
}

static int amdgpu_ucode_init_single_fw(struct amdgpu_device *adev,
				       struct amdgpu_firmware_info *ucode,
				       uint64_t mc_addr, void *kptr)
{
	const struct common_firmware_header *header = NULL;
	const struct gfx_firmware_header_v1_0 *cp_hdr = NULL;

	if (NULL == ucode->fw)
		return 0;

	ucode->mc_addr = mc_addr;
	ucode->kaddr = kptr;

	if (ucode->ucode_id == AMDGPU_UCODE_ID_STORAGE)
		return 0;

	header = (const struct common_firmware_header *)ucode->fw->data;
	cp_hdr = (const struct gfx_firmware_header_v1_0 *)ucode->fw->data;

	if (adev->firmware.load_type != AMDGPU_FW_LOAD_PSP ||
	    (ucode->ucode_id != AMDGPU_UCODE_ID_CP_MEC1 &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_CP_MEC2 &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_CP_MEC1_JT &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_CP_MEC2_JT &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_CP_MES &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_CP_MES_DATA &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_RLC_RESTORE_LIST_CNTL &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_RLC_RESTORE_LIST_GPM_MEM &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_RLC_RESTORE_LIST_SRM_MEM &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_RLC_IRAM &&
	     ucode->ucode_id != AMDGPU_UCODE_ID_RLC_DRAM &&
		 ucode->ucode_id != AMDGPU_UCODE_ID_DMCU_ERAM &&
		 ucode->ucode_id != AMDGPU_UCODE_ID_DMCU_INTV &&
		 ucode->ucode_id != AMDGPU_UCODE_ID_DMCUB)) {
		ucode->ucode_size = le32_to_cpu(header->ucode_size_bytes);

		memcpy(ucode->kaddr, (void *)((uint8_t *)ucode->fw->data +
					      le32_to_cpu(header->ucode_array_offset_bytes)),
		       ucode->ucode_size);
	} else if (ucode->ucode_id == AMDGPU_UCODE_ID_CP_MEC1 ||
		   ucode->ucode_id == AMDGPU_UCODE_ID_CP_MEC2) {
		ucode->ucode_size = le32_to_cpu(header->ucode_size_bytes) -
			le32_to_cpu(cp_hdr->jt_size) * 4;

		memcpy(ucode->kaddr, (void *)((uint8_t *)ucode->fw->data +
					      le32_to_cpu(header->ucode_array_offset_bytes)),
		       ucode->ucode_size);
	} else if (ucode->ucode_id == AMDGPU_UCODE_ID_CP_MEC1_JT ||
		   ucode->ucode_id == AMDGPU_UCODE_ID_CP_MEC2_JT) {
		ucode->ucode_size = le32_to_cpu(cp_hdr->jt_size) * 4;

		memcpy(ucode->kaddr, (void *)((uint8_t *)ucode->fw->data +
					      le32_to_cpu(header->ucode_array_offset_bytes) +
					      le32_to_cpu(cp_hdr->jt_offset) * 4),
		       ucode->ucode_size);
	} else if (ucode->ucode_id == AMDGPU_UCODE_ID_RLC_RESTORE_LIST_CNTL) {
		ucode->ucode_size = adev->gfx.rlc.save_restore_list_cntl_size_bytes;
		memcpy(ucode->kaddr, adev->gfx.rlc.save_restore_list_cntl,
		       ucode->ucode_size);
	} else if (ucode->ucode_id == AMDGPU_UCODE_ID_RLC_RESTORE_LIST_GPM_MEM) {
		ucode->ucode_size = adev->gfx.rlc.save_restore_list_gpm_size_bytes;
		memcpy(ucode->kaddr, adev->gfx.rlc.save_restore_list_gpm,
		       ucode->ucode_size);
	} else if (ucode->ucode_id == AMDGPU_UCODE_ID_RLC_RESTORE_LIST_SRM_MEM) {
		ucode->ucode_size = adev->gfx.rlc.save_restore_list_srm_size_bytes;
		memcpy(ucode->kaddr, adev->gfx.rlc.save_restore_list_srm,
		       ucode->ucode_size);
	} else if (ucode->ucode_id == AMDGPU_UCODE_ID_RLC_IRAM) {
		ucode->ucode_size = adev->gfx.rlc.rlc_iram_ucode_size_bytes;
		memcpy(ucode->kaddr, adev->gfx.rlc.rlc_iram_ucode,
		       ucode->ucode_size);
	} else if (ucode->ucode_id == AMDGPU_UCODE_ID_RLC_DRAM) {
		ucode->ucode_size = adev->gfx.rlc.rlc_dram_ucode_size_bytes;
		memcpy(ucode->kaddr, adev->gfx.rlc.rlc_dram_ucode,
		       ucode->ucode_size);
	}

	return 0;
}

static int amdgpu_ucode_patch_jt(struct amdgpu_firmware_info *ucode,
				uint64_t mc_addr, void *kptr)
{
	const struct gfx_firmware_header_v1_0 *header = NULL;
	const struct common_firmware_header *comm_hdr = NULL;
	uint8_t* src_addr = NULL;
	uint8_t* dst_addr = NULL;

	if (NULL == ucode->fw)
		return 0;

	comm_hdr = (const struct common_firmware_header *)ucode->fw->data;
	header = (const struct gfx_firmware_header_v1_0 *)ucode->fw->data;
	dst_addr = ucode->kaddr +
			   ALIGN(le32_to_cpu(comm_hdr->ucode_size_bytes),
			   PAGE_SIZE);
	src_addr = (uint8_t *)ucode->fw->data +
			   le32_to_cpu(comm_hdr->ucode_array_offset_bytes) +
			   (le32_to_cpu(header->jt_offset) * 4);
	memcpy(dst_addr, src_addr, le32_to_cpu(header->jt_size) * 4);

	return 0;
}

int amdgpu_ucode_create_bo(struct amdgpu_device *adev)
{
	/* Nothing to do for Vangogh device and for load types
	 *   AMDGPU_FW_LOAD_RLC_BACKDOOR_AUTO
	 *   AMDGPU_FW_LOAD_DIRECT
	 *
	 *   the above are the only ones supported.
	 */
	return 0;
}

void amdgpu_ucode_free_bo(struct amdgpu_device *adev)
{
	/* Nothing to do for Vangogh device and for load types
	 *   AMDGPU_FW_LOAD_RLC_BACKDOOR_AUTO
	 *   AMDGPU_FW_LOAD_DIRECT
	 *
	 *   the above are the only ones supported.
	 */
	return;
}

int amdgpu_ucode_init_bo(struct amdgpu_device *adev)
{
	uint64_t fw_offset = 0;
	int i;
	struct amdgpu_firmware_info *ucode = NULL;

 /* for baremetal, the ucode is allocated in gtt, so don't need to fill the bo when reset/suspend */
	if (!amdgpu_sriov_vf(adev) && (amdgpu_in_reset(adev) || adev->in_suspend))
		return 0;
	/*
	 * if SMU loaded firmware, it needn't add SMC, UVD, and VCE
	 * ucode info here
	 */
	if (adev->firmware.load_type != AMDGPU_FW_LOAD_PSP) {
		if (amdgpu_sriov_vf(adev))
			adev->firmware.max_ucodes = AMDGPU_UCODE_ID_MAXIMUM - 3;
		else
			adev->firmware.max_ucodes = AMDGPU_UCODE_ID_MAXIMUM - 4;
	} else {
		adev->firmware.max_ucodes = AMDGPU_UCODE_ID_MAXIMUM;
	}

	for (i = 0; i < adev->firmware.max_ucodes; i++) {
		ucode = &adev->firmware.ucode[i];
		if (ucode->fw) {
			amdgpu_ucode_init_single_fw(adev, ucode, adev->firmware.fw_buf_mc + fw_offset,
						    adev->firmware.fw_buf_ptr + fw_offset);
			if (i == AMDGPU_UCODE_ID_CP_MEC1 &&
			    adev->firmware.load_type != AMDGPU_FW_LOAD_PSP) {
				const struct gfx_firmware_header_v1_0 *cp_hdr;
				cp_hdr = (const struct gfx_firmware_header_v1_0 *)ucode->fw->data;
				amdgpu_ucode_patch_jt(ucode,  adev->firmware.fw_buf_mc + fw_offset,
						    adev->firmware.fw_buf_ptr + fw_offset);
				fw_offset += ALIGN(le32_to_cpu(cp_hdr->jt_size) << 2, PAGE_SIZE);
			}
			fw_offset += ALIGN(ucode->ucode_size, PAGE_SIZE);
		}
	}
	return 0;
}
