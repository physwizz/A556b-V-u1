/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/version.h>
#include <linux/highmem.h>
#include <soc/samsung/exynos-sci.h>

#include "npu-scheduler.h"
#include "npu-log.h"
#include "npu-device.h"

#if IS_ENABLED(CONFIG_EXYNOS_SCI) || IS_ENABLED(CONFIG_EXYNOS_SCI_MODULE)
static unsigned int npu_llc_region_alloc(unsigned int region_index,
				unsigned int set, unsigned int ways)
{
	llc_region_alloc(region_index, set, ways);
	return llc_get_region_info(region_index);
}

static unsigned int npu_llc_get_region_info(unsigned int region_index)
{
	return llc_get_region_info(region_index);
}
#else
static unsigned int npu_llc_region_alloc(unsigned int region_index,
				unsigned int set, unsigned int ways)
{
	llc_region_alloc(region_index, set, ways);
	llc_get_region_info(region_index);
	return 0;
}

static unsigned int npu_llc_get_region_info(unsigned int region_index)
{
	llc_get_region_info(region_index);
	return 0;
}
#endif
static void npu_llc_set_mode(struct npu_scheduler_info *info,
					u32 n0, u32 n1, u32 n2)
{
	struct npu_system *system;
	volatile struct mailbox_hdr *mbox_hdr;

	system = &(info->device->system);
	mbox_hdr = system->mbox_hdr;

	if (!mbox_hdr)
		return;

	info->llc_mode = (n0 & 0xff) << 24 | (n1 & 0xff) << 16 |
		(n2 & 0xff) << 8 | (info->mode & 0xff);
	writel(info->llc_mode, &mbox_hdr->llc_mode);
	flush_kernel_vmap_range((void *)system->mbox_hdr, (int)sizeof(*system->mbox_hdr));
	npu_info("(mode:%u, status:%u, llc_mode:0x%08x)\n",
			info->mode, info->llc_status, info->llc_mode);
	return;
}

static void __npu_set_llc_preset(struct npu_scheduler_info *info)
{
	u32 used_ways, req_ways, npu_ways;

	if (info->llc_status) {
		npu_info("npu llc is in use\n");
		return;
	}

	if (info->llc_ways == 0) {
		if (info->llc_status)
			info->llc_status = npu_llc_region_alloc(LLC_REGION_NPU0, 0, 0);
		info->llc_mode = info->mode & 0xff;

		return;
	}

	npu_ways = npu_llc_get_region_info(LLC_REGION_NPU0);

	if (npu_ways != info->llc_ways) {
		int llc_region_idx = LLC_REGION_DSP1 + 1;

		// Priority high modules
		used_ways = npu_llc_get_region_info(llc_region_idx++);

		while (LLC_REGION_MAX > llc_region_idx)
			used_ways += npu_llc_get_region_info(llc_region_idx++);

		req_ways = LLC_MAX_WAYS - used_ways;
		req_ways = (req_ways < info->llc_ways) ? req_ways : info->llc_ways;

		if (req_ways) {
			if (info->llc_status)
				info->llc_status = npu_llc_region_alloc(LLC_REGION_NPU0, 0, 0);

			npu_ways = npu_llc_region_alloc(LLC_REGION_NPU0, 1, req_ways);
			info->llc_status = 1;
			npu_info("npu set llc used(%u) req(%u), set(%u)\n", used_ways, req_ways, npu_ways);
			info->llc_ways = npu_ways;
		}

		npu_llc_set_mode(info, npu_ways, 0, 0);
	}
}

u32 npu_kpi_llc_size(struct npu_scheduler_info *info)
{
	u32 ret = 0;

	if (info->mode == NPU_PERF_MODE_NPU_BOOST ||
			info->mode == NPU_PERF_MODE_NPU_BOOST_BLOCKING ||
			info->mode == NPU_PERF_MODE_NPU_BOOST_PRUNE)
		ret = (npu_get_configs(NPU_LLC_CHUNK_SIZE)/K_SIZE) * LLC_MAX_WAYS;

	return ret;
}

void npu_set_llc(struct npu_scheduler_info *info)
{
	__npu_set_llc_preset(info);
	return;
}

void npu_llc_close(struct npu_scheduler_info *info)
{
	if (info->llc_status)
		info->llc_status = npu_llc_region_alloc(LLC_REGION_NPU0, 0, 0);
	info->llc_ways = 0;

	npu_dbg("called\n");
}
