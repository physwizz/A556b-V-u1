// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "pablo-kernel-variant.h"

#if IS_ENABLED(CONFIG_SCHED_EMS_TUNE) || IS_ENABLED(CONFIG_SCHED_EMS)
#include <linux/ems.h>
#endif

#if (!IS_ENABLED(CONFIG_CFI_CLANG)) && !defined(MODULE)
void __attribute__((weak)) tasklet_setup(struct tasklet_struct *t,
				void (*callback)(struct tasklet_struct *))
{
	tasklet_init(t, (void (*)(unsigned long))callback, (unsigned long)t);
}
#else
#if PKV_VER_GE(5, 10, 0)
void pkv_tasklet_setup(struct tasklet_struct *t, void (*callback)(struct tasklet_struct *))
{
	tasklet_setup(t, callback);
}
#else
void pkv_tasklet_setup(struct tasklet_struct *t, void (*func)(unsigned long))
{
	tasklet_init(t, func, (unsigned long)t);
}
#endif
EXPORT_SYMBOL_GPL(pkv_tasklet_setup);
#endif

#if PKV_VER_GE(6, 6, 0)
void *pkv_dma_buf_vmap(struct dma_buf *dbuf)
{
	struct iosys_map map = { 0, };
	int ret;

	ret = dma_buf_vmap_unlocked(dbuf, &map);
	if (ret) {
		pr_err("failed to dma_buf_vmap(ret:%d)\n", ret);
		return NULL;
	}

	return map.vaddr;
}

void pkv_dma_buf_vunmap(struct dma_buf *dbuf, void *kva)
{
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(kva);

	dma_buf_vunmap_unlocked(dbuf, &map);
}
#elif PKV_VER_GE(6, 1, 0)
void *pkv_dma_buf_vmap(struct dma_buf *dbuf)
{
	struct iosys_map map = { 0, };
	int ret;

	ret = dma_buf_vmap(dbuf, &map);
	if (ret) {
		pr_err("failed to dma_buf_vmap(ret:%d)\n", ret);
		return NULL;
	}

	return map.vaddr;
}

void pkv_dma_buf_vunmap(struct dma_buf *dbuf, void *kva)
{
	struct iosys_map map = IOSYS_MAP_INIT_VADDR(kva);

	dma_buf_vunmap(dbuf, &map);
}
#elif PKV_VER_GE(5, 15, 0)
void *pkv_dma_buf_vmap(struct dma_buf *dbuf)
{
	struct dma_buf_map map;
	int ret;

	ret = dma_buf_vmap(dbuf, &map);
	if (ret) {
		pr_err("failed to dma_buf_vmap(ret:%d)\n", ret);
		return NULL;
	}

	return map.vaddr;
}

void pkv_dma_buf_vunmap(struct dma_buf *dbuf, void *kva)
{
	struct dma_buf_map map = DMA_BUF_MAP_INIT_VADDR(kva);

	dma_buf_vunmap(dbuf, &map);
}
#else
void *pkv_dma_buf_vmap(struct dma_buf *dbuf)
{
	return dma_buf_vmap(dbuf);
}

void pkv_dma_buf_vunmap(struct dma_buf *dbuf, void *kva)
{
	dma_buf_vunmap(dbuf, kva);
}
#endif
EXPORT_SYMBOL_GPL(pkv_dma_buf_vmap);
EXPORT_SYMBOL_GPL(pkv_dma_buf_vunmap);

/*
 * These APIs were removed after v5.15.
 * After v5.15, DT configuraion is used instead of direct function call.
 */
#if (PKV_VER_GE(5, 4, 0) && PKV_VER_LT(5, 15, 0) && !IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON))
int pkv_iommu_dma_reserve_iova_map(struct device *dev, dma_addr_t base, u64 size)
{
	int ret;
	struct iommu_domain *domain = iommu_get_domain_for_dev(dev);

	ret = iommu_map(domain, base, base, size, 0);
	if (ret) {
		dev_err(dev, "%s: iommu_map is fail(%d)", __func__, ret);
		return ret;
	}

	iommu_dma_reserve_iova(dev, base, size);

	return 0;
}

void pkv_iommu_dma_reserve_iova_unmap(struct device *dev, dma_addr_t base, u64 size)
{
	struct iommu_domain *domain = iommu_get_domain_for_dev(dev);

	iommu_unmap(domain, base, size);
}
EXPORT_SYMBOL_GPL(pkv_iommu_dma_reserve_iova_map);
EXPORT_SYMBOL_GPL(pkv_iommu_dma_reserve_iova_unmap);
#endif

#if IS_ENABLED(CONFIG_SCHED_EMS_TUNE) || IS_ENABLED(CONFIG_SCHED_EMS)
static struct emstune_mode_request emstune_req;
void pkv_emstune_add_request(void)
{
	__emstune_add_request(&emstune_req, (char *)__func__, __LINE__);
}
EXPORT_SYMBOL_GPL(pkv_emstune_add_request);
void pkv_emstune_remove_request(void)
{
	emstune_remove_request(&emstune_req);
}
EXPORT_SYMBOL_GPL(pkv_emstune_remove_request);
void pkv_emstune_boost(int enable)
{
	emstune_boost(&emstune_req, enable);
}
EXPORT_SYMBOL_GPL(pkv_emstune_boost);
#else
void pkv_emstune_add_request(void)
{
}
void pkv_emstune_remove_request(void)
{
}
void pkv_emstune_boost(int enable)
{
}
#endif

#if PKV_VER_GE(6, 6, 0)
void *pkv_kvrealloc(const void *p, size_t oldsize, size_t newsize, gfp_t flags)
{
	return kvrealloc(p, oldsize, newsize, flags);
}
#else
void *pkv_kvrealloc(const void *p, size_t oldsize, size_t newsize, gfp_t flags)
{
	void *newp;

	if (oldsize >= newsize)
		return (void *)p;
	newp = kvmalloc(newsize, flags);
	if (!newp)
		return NULL;
	memcpy(newp, p, oldsize);
	kvfree(p);
	return newp;
}
#endif
EXPORT_SYMBOL_GPL(pkv_kvrealloc);
