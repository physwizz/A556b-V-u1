// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2014-2019, Samsung Electronics.
 *
 */

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_fdt.h>
#include <soc/samsung/shm_ipc.h>
#if IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
#include <soc/samsung/exynos/debug-snapshot.h>
#endif

#include "modem_utils.h"
#include "shm_ipc_priv.h"

/*
 * Reserved memory
 */
static int _rmem_count;
static struct cp_reserved_mem _cp_rmem[MAX_CP_RMEM];

#if defined(MODULE)
static int cp_rmem_setup_latecall(struct platform_device *pdev)
{
	struct device_node *np;
	struct reserved_mem *rmem;
	u32 rmem_index = 0;
	int i;

	for (i = 0; i < MAX_CP_RMEM; i++) {
		np = of_parse_phandle(pdev->dev.of_node, "memory-region", i);
		if (!np)
			break;

		mif_dt_read_u32(np, "rmem_index", rmem_index);

		rmem = of_reserved_mem_lookup(np);
		if (!rmem) {
			mif_err("of_reserved_mem_lookup() failed\n");
			break;
		}

		_cp_rmem[i].index = rmem_index;
		_cp_rmem[i].name = (char *)rmem->name;
		_cp_rmem[i].p_base = rmem->base;
		_cp_rmem[i].size = rmem->size;

		mif_info("rmem %d %s 0x%08lx 0x%08x\n",
				_cp_rmem[i].index, _cp_rmem[i].name,
				_cp_rmem[i].p_base, _cp_rmem[i].size);
	}

	return 0;
}
#else
static int __init cp_rmem_setup(struct reserved_mem *rmem)
{
	const __be32 *prop;
	int len;

	if (_rmem_count >= MAX_CP_RMEM) {
		mif_err("_cp_rmem is full for %s\n", rmem->name);
		return -ENOMEM;
	}

	prop = of_get_flat_dt_prop(rmem->fdt_node, "rmem_index", &len);
	if (!prop) {
		mif_err("rmem_name is not defined for %s\n", rmem->name);
		return -ENOENT;
	}
	_cp_rmem[be32_to_cpu(prop[0])].index = be32_to_cpu(prop[0]);
	_cp_rmem[be32_to_cpu(prop[0])].name = (char *)rmem->name;
	_cp_rmem[be32_to_cpu(prop[0])].p_base = rmem->base;
	_cp_rmem[be32_to_cpu(prop[0])].size = rmem->size;
	_rmem_count++;

	mif_info("rmem %d %s 0x%08lx 0x%08x\n",
			_cp_rmem[be32_to_cpu(prop[0])].index, _cp_rmem[be32_to_cpu(prop[0])].name,
			_cp_rmem[be32_to_cpu(prop[0])].p_base, _cp_rmem[be32_to_cpu(prop[0])].size);

	return 0;
}
RESERVEDMEM_OF_DECLARE(modem_if, "exynos,modem_if", cp_rmem_setup);

#endif

/*
 * Shared memory
 */
static struct cp_shared_mem _cp_shmem[MAX_CP_SHMEM];

static int cp_shmem_setup(struct device *dev)
{
	struct device_node *regions = NULL;
	struct device_node *child = NULL;
	u32 shmem_index, rmem_index;
	u32 offset;
	u32 count = 0;

	regions = of_get_child_by_name(dev->of_node, "regions");
	if (!regions) {
		mif_err("of_get_child_by_name() error:regions\n");
		return -EINVAL;
	}

	for_each_child_of_node(regions, child) {
		if (count >= MAX_CP_SHMEM) {
			mif_err("_cp_shmem is full for %d\n", count);
			return -ENOMEM;
		}
		mif_dt_read_u32(child, "region,index", shmem_index);
		_cp_shmem[shmem_index].index = shmem_index;
		mif_dt_read_string(child, "region,name", _cp_shmem[shmem_index].name);

		mif_dt_read_u32(child, "region,rmem", _cp_shmem[shmem_index].rmem);
		rmem_index = _cp_shmem[shmem_index].rmem;
		if (!_cp_rmem[rmem_index].p_base) {
			mif_err("_cp_rmem[%d].p_base is null\n", rmem_index);
			return -ENOMEM;
		}
		mif_dt_read_u32(child, "region,offset", offset);

		_cp_shmem[shmem_index].p_base = _cp_rmem[rmem_index].p_base + offset;
		mif_dt_read_u32(child, "region,size", _cp_shmem[shmem_index].size);
		if ((_cp_shmem[shmem_index].p_base + _cp_shmem[shmem_index].size) >
			(_cp_rmem[rmem_index].p_base + _cp_rmem[rmem_index].size)) {
			mif_err("%d %d size error 0x%08lx 0x%08x 0x%08lx 0x%08x\n",
				rmem_index, shmem_index,
				_cp_shmem[shmem_index].p_base,
				_cp_shmem[shmem_index].size,
				_cp_rmem[rmem_index].p_base, _cp_rmem[rmem_index].size);
			return -ENOMEM;
		}

		mif_dt_read_u32_noerr(child, "region,cached",
				_cp_shmem[shmem_index].cached);
		count++;
	}

	return 0;
}

/*
 * Memory map on CP binary
 */
static int _mem_map_on_cp;

static int cp_shmem_check_mem_map_on_cp(struct device *dev)
{
	void __iomem *base = NULL;
	struct cp_toc *toc = NULL;
	struct cp_mem_map *map = NULL;
	int i;
	u32 rmem_index = 0;
	u32 shmem_index = 0;
	long long base_diff = 0;

	base = phys_to_virt(cp_shmem_get_base(SHMEM_CP));
	if (!base) {
		mif_err("base is null\n");
		return -ENOMEM;
	}

	toc = (struct cp_toc *)(base + sizeof(struct cp_toc));
	if (!toc) {
		mif_err("toc is null\n");
		return -ENOMEM;
	}
	mif_info("offset:0x%08x\n", toc->img_offset);

	if (toc->img_offset > (cp_shmem_get_size(SHMEM_CP) - MAP_ON_CP_OFFSET)) {
		mif_info("Invalid img_offset:0x%08x. Use dt information\n", toc->img_offset);
		return 0;
	}

	map = (struct cp_mem_map *)(base + toc->img_offset + MAP_ON_CP_OFFSET);
	if (map->version != CP_MEM_MAP_VER) {
		mif_err("version error:0x%08x. Try enc offset\n", map->version);

		map = (struct cp_mem_map *)(base + toc->img_offset + MAP_ON_CP_OFFSET_ENC);
		if (map->version != CP_MEM_MAP_VER) {
			mif_err("enc version error:0x%08x. Use dt information\n",
				map->version);
			return 0;
		}
	}

	_mem_map_on_cp = 1;

	mif_info("map_count:%d\n", map->map_count);
	if (!map->map_count) {
		mif_err("map_count error\n");
		return -EINVAL;
	}
	for (i = 0; i < map->map_count; i++) {
		switch (map->map_info[i].name) {
			case CP_MEM_MAP_CP0:
				shmem_index = SHMEM_CP;
				break;
			case CP_MEM_MAP_VSS:
				shmem_index = SHMEM_VSS;
				break;
			case CP_MEM_MAP_IPC:
				shmem_index = SHMEM_IPC;
				break;
			case CP_MEM_MAP_LOG:
				shmem_index = SHMEM_BTL;
				break;
			case CP_MEM_MAP_PKP:
				shmem_index = SHMEM_PKTPROC;
				break;
			case CP_MEM_MAP_L2B:
				shmem_index = SHMEM_L2B;
				break;
			case CP_MEM_MAP_DDM:
				shmem_index = SHMEM_DDM;
				break;
			case CP_MEM_MAP_CP1:
				shmem_index = SHMEM_CP1;
				break;
			case CP_MEM_MAP_STS:
				shmem_index = SHMEM_CP_STATE;
				break;
			default:
				continue;
		}

		rmem_index = _cp_shmem[shmem_index].rmem;
		if (!_cp_rmem[rmem_index].p_base) {
			mif_err("_cp_rmem[%d].p_base is null\n", rmem_index);
			return -ENOMEM;
		}

		base_diff = _cp_rmem[rmem_index].p_base - _cp_rmem[0].p_base;
		_cp_shmem[shmem_index].p_base =
			_cp_rmem[rmem_index].p_base + map->map_info[i].offset - base_diff;
		_cp_shmem[shmem_index].size = map->map_info[i].size;
#if IS_ENABLED(CONFIG_CP_MEM_MAP_V1)
		_cp_shmem[shmem_index].option = map->map_info[i].option;
#endif

		if ((_cp_shmem[shmem_index].p_base + _cp_shmem[shmem_index].size) >
			(_cp_rmem[rmem_index].p_base + _cp_rmem[rmem_index].size)) {
			mif_err("rmem:%d shmem_index:%d size error 0x%08lx 0x%08x 0x%08lx 0x%08x\n",
				rmem_index, shmem_index,
				_cp_shmem[shmem_index].p_base, _cp_shmem[shmem_index].size,
				_cp_rmem[rmem_index].p_base, _cp_rmem[rmem_index].size);
			return -ENOMEM;
		}

		mif_info("rmem:%d shmem_index:%d base:0x%08lx size:0x%08x opt:0x%08x\n",
				rmem_index, shmem_index, _cp_shmem[shmem_index].p_base,
				_cp_shmem[shmem_index].size, _cp_shmem[shmem_index].option);
	}

	return 0;
}

/*
 * Export functions - legacy
 */
unsigned long shm_get_msi_base(void)
{
	return cp_shmem_get_base(SHMEM_MSI);
}
EXPORT_SYMBOL(shm_get_msi_base);

void __iomem *shm_get_vss_region(void)
{
	return cp_shmem_get_region(SHMEM_VSS);
}
EXPORT_SYMBOL(shm_get_vss_region);

unsigned long shm_get_vss_base(void)
{
	return cp_shmem_get_base(SHMEM_VSS);
}
EXPORT_SYMBOL(shm_get_vss_base);

u32 shm_get_vss_size(void)
{
	return cp_shmem_get_size(SHMEM_VSS);
}
EXPORT_SYMBOL(shm_get_vss_size);

void __iomem *shm_get_vparam_region(void)
{
	return cp_shmem_get_region(SHMEM_VPA);
}
EXPORT_SYMBOL(shm_get_vparam_region);

unsigned long shm_get_vparam_base(void)
{
	return cp_shmem_get_base(SHMEM_VPA);
}
EXPORT_SYMBOL(shm_get_vparam_base);

u32 shm_get_vparam_size(void)
{
	return cp_shmem_get_size(SHMEM_VPA);
}
EXPORT_SYMBOL(shm_get_vparam_size);

/*
 * Export functions
 */
int cp_shmem_get_mem_map_on_cp_flag(void)
{
	mif_info("flag:%d\n", _mem_map_on_cp);

	return _mem_map_on_cp;
}
EXPORT_SYMBOL(cp_shmem_get_mem_map_on_cp_flag);

void __iomem *cp_shmem_get_nc_region(unsigned long base, u32 size)
{
	unsigned int num_pages = (unsigned int)DIV_ROUND_UP(size, PAGE_SIZE);
	pgprot_t prot = pgprot_writecombine(PAGE_KERNEL);
	struct page **pages;
	void *v_addr;
	unsigned int i;
	u32 v_offset = base % PAGE_SIZE;

	if (!base)
		return NULL;

	pages = kvcalloc(num_pages, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return NULL;

	for (i = 0; i < num_pages; i++) {
		pages[i] = phys_to_page(base);
		base += PAGE_SIZE;
	}

	v_addr = vmap(pages, num_pages, VM_MAP, prot);
	if (!v_addr)
		mif_err("Failed to vmap pages\n");

	kvfree(pages);

	return (void __iomem *)v_addr + v_offset;
}
EXPORT_SYMBOL(cp_shmem_get_nc_region);

void __iomem *cp_shmem_get_region_with_size(u32 idx, u32 size)
{
	if (_cp_shmem[idx].v_base)
		return _cp_shmem[idx].v_base;

	if (_cp_shmem[idx].cached)
		_cp_shmem[idx].v_base = phys_to_virt(_cp_shmem[idx].p_base);
	else
		_cp_shmem[idx].v_base = cp_shmem_get_nc_region(_cp_shmem[idx].p_base,
				size ?: _cp_shmem[idx].size);

	return _cp_shmem[idx].v_base;
}
EXPORT_SYMBOL(cp_shmem_get_region_with_size);

void __iomem *cp_shmem_get_region(u32 idx)
{
	return cp_shmem_get_region_with_size(idx, 0);
}
EXPORT_SYMBOL(cp_shmem_get_region);

void cp_shmem_release_region(u32 idx)
{
	if (_cp_shmem[idx].v_base)
		vunmap(_cp_shmem[idx].v_base);
}
EXPORT_SYMBOL(cp_shmem_release_region);

void cp_shmem_release_rmem(u32 idx, u32 headroom)
{
	int i;
	unsigned long base, offset = 0;
	u32 size;
	struct page *page;

	base = cp_shmem_get_base(idx);
	size = cp_shmem_get_size(idx);
	mif_info("Release rmem base:0x%08lx size:0x%08x headroom:0x%08x\n",
		base, size, headroom);

	for (i = 0; i < (size >> PAGE_SHIFT); i++) {
		if (offset >= headroom) {
			page = phys_to_page(base + offset);
			free_reserved_page(page);
		}
		offset += PAGE_SIZE;
	}
}
EXPORT_SYMBOL(cp_shmem_release_rmem);

unsigned long cp_shmem_get_base(u32 idx)
{
	return _cp_shmem[idx].p_base;
}
EXPORT_SYMBOL(cp_shmem_get_base);

u32 cp_shmem_get_size(u32 idx)
{
	return _cp_shmem[idx].size;
}
EXPORT_SYMBOL(cp_shmem_get_size);

void cp_shmem_set_size(u32 idx, u32 size)
{
	_cp_shmem[idx].size = size;
}
EXPORT_SYMBOL(cp_shmem_set_size);

/*
 * Platform driver
 */
static int cp_shmem_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;
	u32 use_map_on_cp = 0;
	int i;
#if IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
	bool log_cpmem = true;
#endif

	mif_info("+++\n");

#if defined(MODULE)
	if (!_rmem_count) {
		ret = cp_rmem_setup_latecall(pdev);
		if (ret) {
			mif_err("cp_rmem_setup_latecall() error:%d\n", ret);
			goto fail;
		}
	}
#endif

	ret = cp_shmem_setup(dev);
	if (ret) {
		mif_err("cp_shmem_setup() error:%d\n", ret);
		goto fail;
	}

	mif_dt_read_u32(dev->of_node, "use_mem_map_on_cp", use_map_on_cp);

	if (use_map_on_cp) {
		ret = cp_shmem_check_mem_map_on_cp(dev);
		if (ret) {
			mif_err("cp_shmem_check_mem_map_on_cp() error:%d\n", ret);
			goto fail;
		}
	} else {
		mif_info("use_mem_map_on_cp is disabled. Use dt information\n");
	}

	for (i = 0; i < MAX_CP_SHMEM; i++) {
		if (!_cp_shmem[i].name)
			continue;

		mif_info("rmem:%d index:%d %s 0x%08lx 0x%08x c:%d opt:0x%08x\n",
			_cp_shmem[i].rmem, _cp_shmem[i].index, _cp_shmem[i].name,
			_cp_shmem[i].p_base, _cp_shmem[i].size, _cp_shmem[i].cached,
			_cp_shmem[i].option);
	}

#if IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
	/* Set ramdump for rmem index 0 */
#if IS_ENABLED(CONFIG_CPIF_CHECK_SJTAG_STATUS)
	if (dbg_snapshot_get_sjtag_status() && !dbg_snapshot_get_dpm_status())
		log_cpmem = false;
#endif
	mif_info("cpmem dump on fastboot is %s\n", log_cpmem ? "enabled" : "disabled");
	if (log_cpmem) {
		for (i = 0; i < MAX_CP_RMEM; i++) {
			char log_name[40];

			if (!_cp_rmem[i].p_base)
				continue;
			snprintf(log_name, sizeof(log_name), "log_cpmem_%d", i);
			mif_info("%s will be generated after ramdump\n", log_name);
			dbg_snapshot_add_bl_item_info(log_name,
					_cp_rmem[i].p_base, _cp_rmem[i].size);
		}
	}
#endif

	mif_info("---\n");

	return 0;

fail:
	mif_err("CP shmem probe failed\n");
	return ret;
}

static int cp_shmem_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id cp_shmem_dt_match[] = {
	{ .compatible = "samsung,exynos-cp-shmem", },
	{},
};
MODULE_DEVICE_TABLE(of, cp_shmem_dt_match);

static struct platform_driver cp_shmem_driver = {
	.probe = cp_shmem_probe,
	.remove = cp_shmem_remove,
	.driver = {
		.name = "cp_shmem",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(cp_shmem_dt_match),
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(cp_shmem_driver);

MODULE_DESCRIPTION("Exynos CP shared memory driver");
MODULE_LICENSE("GPL");
