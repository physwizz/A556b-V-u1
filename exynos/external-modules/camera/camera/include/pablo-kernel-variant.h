/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef PABLO_KERNEL_VARIANT_H
#define PABLO_KERNEL_VARIANT_H

#include <linux/delay.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/dma-buf.h>

#define PKV_VER_GT(a, b, c)	(KERNEL_VERSION((a), (b), (c)) < LINUX_VERSION_CODE)
#define PKV_VER_GE(a, b, c)	(KERNEL_VERSION((a), (b), (c)) <= LINUX_VERSION_CODE)
#define PKV_VER_EQ(a, b, c)	(KERNEL_VERSION((a), (b), (c)) == LINUX_VERSION_CODE)
#define PKV_VER_LE(a, b, c)	(KERNEL_VERSION((a), (b), (c)) >= LINUX_VERSION_CODE)
#define PKV_VER_LT(a, b, c)	(KERNEL_VERSION((a), (b), (c)) > LINUX_VERSION_CODE)

#if PKV_VER_GE(6, 6, 0)
#define pkv_class_create(m, n) class_create(n)
#define pkv_iommu_map_sg(d, b, l, n, f, g) iommu_map_sg(d, b, l, n, f, g)
#define pkv_dma_buf_map_attachment dma_buf_map_attachment_unlocked
#define pkv_dma_buf_unmap_attachment dma_buf_unmap_attachment_unlocked
#define DEFINE_I2C_DRIVER_PROBE(_n) static int _n(struct i2c_client *client)
#else
#define pkv_class_create(m, n) class_create(m, n)
#define pkv_iommu_map_sg(d, b, l, n, f, g) iommu_map_sg(d, b, l, n, f)
#define pkv_dma_buf_map_attachment dma_buf_map_attachment
#define pkv_dma_buf_unmap_attachment dma_buf_unmap_attachment
#define DEFINE_I2C_DRIVER_PROBE(_n) static int _n(struct i2c_client *client, \
		const struct i2c_device_id *id)
#endif

#if PKV_VER_GE(5, 15, 0)
#include <linux/panic_notifier.h>
#endif

#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
#include <soc/samsung/exynos_pm_qos.h>
#define START_DVFS_LEVEL_SN		IS_SN_MAX
#if PKV_VER_LT(5, 15, 0)
#define PM_QOS_M2M_THROUGHPUT		0
#endif
#if PKV_VER_LT(6, 1, 0)
#define PM_QOS_ICPU_THROUGHPUT		0
#endif
#else
#include <linux/pm_qos.h>
#define START_DVFS_LEVEL_SN		IS_DVFS_SN_DEFAULT
#define PM_QOS_DEVICE_THROUGHPUT	0
#define PM_QOS_INTCAM_THROUGHPUT	0
#define PM_QOS_TNR_THROUGHPUT		0
#define PM_QOS_CSIS_THROUGHPUT		0
#define PM_QOS_ISP_THROUGHPUT		0
#define PM_QOS_BUS_THROUGHPUT		0
#define PM_QOS_CAM_THROUGHPUT		0
#define PM_QOS_M2M_THROUGHPUT		0
#define PM_QOS_ICPU_THROUGHPUT		0
#endif

#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
#if PKV_VER_GE(5, 15, 0)
#include <soc/samsung/exynos/memlogger.h>
#else
#include <soc/samsung/memlogger.h>
#endif
#else
struct memlog;
#endif

#if IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
#if PKV_VER_GE(5, 15, 0)
#include <soc/samsung/exynos/debug-snapshot.h>
#else
#include <soc/samsung/debug-snapshot.h>
#endif
#endif

#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG)
#include <soc/samsung/exynos-bcm_dbg.h>
#else
#define PANIC_HANDLE				0
#define CAMERA_DRIVER				0
#endif

#if IS_ENABLED(CONFIG_EXYNOS_BTS)
#include <soc/samsung/bts.h>
#endif

#if IS_ENABLED(CONFIG_EXYNOS_ITMON)
#if PKV_VER_GE(5, 15, 0)
#include <soc/samsung/exynos/exynos-itmon.h>
#else
#include <soc/samsung/exynos-itmon.h>
#endif
#endif

#if PKV_VER_GE(5, 15, 0)
#include <soc/samsung/exynos/exynos-soc.h>
#else
#include <linux/soc/samsung/exynos-soc.h>
#endif

#if PKV_VER_GE(5, 12, 0)
#define PKV_DEFINE_I3C_DRIVER_REMOVE(n) static void n(struct i3c_device *client)
#define PKV_DEFINE_I3C_DRIVER_REMOVE_RETURN
#else
#define PKV_DEFINE_I3C_DRIVER_REMOVE(n) static int n(struct i3c_device *client)
#define PKV_DEFINE_I3C_DRIVER_REMOVE_RETURN { return 0; }
#endif

/* Configuration of Timer function type by kernel version */
#if PKV_VER_LT(4, 15, 0)
#define IS_TIMER_PARAM_TYPE unsigned long
#else
#define IS_TIMER_PARAM_TYPE struct timer_list*
#endif
#define IS_TIMER_FUNC(__FUNC_NAME)	\
	static void __FUNC_NAME(IS_TIMER_PARAM_TYPE data)

/* Configuration of memblock_alloc function type by kernel version */
#if PKV_VER_LT(4, 20, 0)
#define is_memblock_alloc		memblock_alloc
#else
#define is_memblock_alloc		memblock_phys_alloc
#endif

/* tasklet stuffs */
#if !IS_ENABLED(CONFIG_CFI_CLANG) && !defined(MODULE)
#define pkv_tasklet_setup(t, c) tasklet_setup(t, c)
void __attribute__((weak)) tasklet_setup(struct tasklet_struct *t,
			void (*callback)(struct tasklet_struct *));
#else
#if PKV_VER_GE(5, 10, 0)
#define PKV_DECLARE_TASKLET_CALLBACK(n, a) \
		static void tasklet_##n(struct tasklet_struct *a)
#define PKV_ASSIGN_TASKLET_CALLBACK(n) .tasklet_##n = tasklet_##n
#define PKV_CALL_TASKLET_CALLBACK(fp, n, a) fp->tasklet_##n(a)
void pkv_tasklet_setup(struct tasklet_struct *t,
			void (*callback)(struct tasklet_struct *));
#else
#define PKV_DECLARE_TASKLET_CALLBACK(n, a) \
		static void tasklet_##n(unsigned long a)
#define PKV_ASSIGN_TASKLET_CALLBACK(n) .old_tasklet_##n = tasklet_##n
#define PKV_CALL_TASKLET_CALLBACK(fp, n, a) \
		fp->old_tasklet_##n((unsigned long)a)
#define from_tasklet(v, t, f) \
		container_of((struct tasklet_struct *)t, typeof(*v), f)
void pkv_tasklet_setup(struct tasklet_struct *t, void (*func)(unsigned long));
#endif
#endif

/* flexible sleep */
#if PKV_VER_LT(5, 10, 0)
/* copy from include/linux/delay.h */
static inline void fsleep(unsigned long usecs)
{
	if (usecs <= 10)
		udelay(usecs);
	else if (usecs <= 20000)
		usleep_range(usecs, 2 * usecs);
	else
		msleep(DIV_ROUND_UP(usecs, 1000));
}

static inline void bitmap_next_clear_region(unsigned long *bitmap,
					    unsigned int *rs, unsigned int *re,
					    unsigned int end)
{
	*rs = find_next_zero_bit(bitmap, end, *rs);
	*re = find_next_bit(bitmap, end, *rs + 1);
}

static inline void bitmap_next_set_region(unsigned long *bitmap,
					  unsigned int *rs, unsigned int *re,
					  unsigned int end)
{
	*rs = find_next_bit(bitmap, end, *rs);
	*re = find_next_zero_bit(bitmap, end, *rs + 1);
}

/*
 * Bitmap region iterators.  Iterates over the bitmap between [@start, @end).
 * @rs and @re should be integer variables and will be set to start and end
 * index of the current clear or set region.
 */
#define bitmap_for_each_clear_region(bitmap, rs, re, start, end)	     \
	for ((rs) = (start),						     \
	     bitmap_next_clear_region((bitmap), &(rs), &(re), (end));	     \
	     (rs) < (re);						     \
	     (rs) = (re) + 1,						     \
	     bitmap_next_clear_region((bitmap), &(rs), &(re), (end)))

#define bitmap_for_each_set_region(bitmap, rs, re, start, end)		     \
	for ((rs) = (start),						     \
	     bitmap_next_set_region((bitmap), &(rs), &(re), (end));	     \
	     (rs) < (re);						     \
	     (rs) = (re) + 1,						     \
	     bitmap_next_set_region((bitmap), &(rs), &(re), (end)))

#define __iomb()
#endif

#if PKV_VER_GE(6, 1, 0)
#define bitmap_for_each_clear_region(bitmap, rs, re, start, end)	     \
	for_each_clear_bitrange(rs, re, bitmap, end - start)
#define bitmap_for_each_set_region(bitmap, rs, re, start, end)		     \
	for_each_set_bitrange(rs, re, bitmap, end - start)
#define PKV_DEFINE_I2C_DRIVER_REMOVE(n) static void n(struct i2c_client *client)
#define PKV_DEFINE_I2C_DRIVER_REMOVE_RETURN
#else
#define PKV_DEFINE_I2C_DRIVER_REMOVE(n) static int n(struct i2c_client *client)
#define PKV_DEFINE_I2C_DRIVER_REMOVE_RETURN { return 0; }
#endif

/* vb2 functions */
#if PKV_VER_GE(5, 4, 0)
#define pablo_vb2_qbuf(vbq, buf)		vb2_qbuf(vbq, NULL, buf)
#define pablo_vb2_prepare_buf(vbq, buf)		vb2_prepare_buf(vbq, NULL, buf)
#define pablo_fill_vb2_buffer(vb, buf, planes)	fill_vb2_buffer(vb, planes)
#else
#define pablo_vb2_qbuf(vbq, buf)		vb2_qbuf(vbq, buf)
#define pablo_vb2_prepare_buf(vbq, buf)		vb2_prepare_buf(vbq, buf)
#define pablo_fill_vb2_buffer(vb, buf, planes)	fill_vb2_buffer(vb, buf, planes)
#endif

/* type of V4L2 device node */
#if PKV_VER_GE(5, 10, 0)
#define VFL_TYPE_PABLO	VFL_TYPE_VIDEO
#else
#define VFL_TYPE_PABLO	VFL_TYPE_GRABBER
#endif

/* vmalloc offset depened on KASAN config */
#if PKV_VER_GE(5, 10, 0)
#if defined(BPF_JIT_REGION_SIZE)
#define BPF_OFS		0
#else
#define BPF_OFS		SZ_128M
#endif
#if defined(CONFIG_KASAN_GENERIC)
#define VMALLOC_OFS	BPF_OFS				/* KASAN: 64GB */
#elif defined(CONFIG_KASAN_SW_TAGS)
#define VMALLOC_OFS	(0x0800000000ULL + BPF_OFS)	/* KASAN: 32GB */
#else
#define VMALLOC_OFS	(0x1000000000ULL + BPF_OFS)
#endif /* defined(CONFIG_KASAN_GENERIC) || defined(CONFIG_KASAN_SW_TAGS) */
#else
#ifdef CONFIG_KASAN
#define VMALLOC_OFS	0				/* KASAN: 64GB */
#else
#define VMALLOC_OFS	0x1000000000ULL
#endif /* CONFIG_KASAN */
#endif /* PKV_VER_GE(5, 10, 0) */

#if PKV_VER_GE(5, 15, 0)
typedef struct v4l2_subdev_state v4l2_subdev_config_t;

enum dma_sync_target {
	SYNC_FOR_CPU = 0,
	SYNC_FOR_DEVICE = 1,
};
#define pkv_in_hardirq() in_hardirq()
#else
typedef struct v4l2_subdev_pad_config v4l2_subdev_config_t;
#define pkv_in_hardirq() in_irq()
#endif

void *pkv_dma_buf_vmap(struct dma_buf *dbuf);
void pkv_dma_buf_vunmap(struct dma_buf *dbuf, void *kva);

#if (PKV_VER_GE(5, 4, 0) && PKV_VER_LT(5, 15, 0) && !IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON))
#include <linux/iommu.h>
#include <linux/dma-iommu.h>

int pkv_iommu_dma_reserve_iova_map(struct device *dev, dma_addr_t base, u64 size);
void pkv_iommu_dma_reserve_iova_unmap(struct device *dev, dma_addr_t base, u64 size);
#else
#if IS_ENABLED(CONFIG_EXYNOS_IOVMM)
#include <linux/exynos_iovmm.h>
#else
#include <linux/iommu.h>
#endif

#define pkv_iommu_dma_reserve_iova_map(a, b, c) (0)
#define pkv_iommu_dma_reserve_iova_unmap(a, b, c) do { } while (0)
#endif

void pkv_emstune_add_request(void);
void pkv_emstune_remove_request(void);
void pkv_emstune_boost(int enable);

#if PKV_VER_GE(6, 6, 0)
#define PABLO_TRACE_INCLUDE_PATH ../../../exynos/external-modules/camera/camera/lib/
#else
#define PABLO_TRACE_INCLUDE_PATH ../../drivers/media/platform/exynos/camera/lib/
#define PKV_KVMEMDUP_UNSUPPORTED
#endif

void *pkv_kvrealloc(const void *p, size_t oldsize, size_t newsize, gfp_t flags);

/* cleanup helpers */
#if PKV_VER_GE(6, 1, 79)
#include <linux/cleanup.h>
#else
#include <linux/compiler.h>

#undef __cleanup
#define __cleanup(func)	__maybe_unused __attribute__((__cleanup__(func)))

#define DEFINE_FREE(_name, _type, _free) \
	static inline void __free_##_name(void *p) { _type _T = *(_type *)p; _free; }

#define __free(_name)	__cleanup(__free_##_name)

#define no_free_ptr(p) \
	({ __auto_type __ptr = (p); (p) = NULL; __ptr; })

#define return_ptr(p)	return no_free_ptr(p)

#define DEFINE_CLASS(_name, _type, _exit, _init, _init_args...)		\
typedef _type class_##_name##_t;					\
static inline void class_##_name##_destructor(_type *p)			\
{ _type _T = *p; _exit; }						\
static inline _type class_##_name##_constructor(_init_args)		\
{ _type t = _init; return t; }

#define EXTEND_CLASS(_name, ext, _init, _init_args...)			\
typedef class_##_name##_t class_##_name##ext##_t;			\
static inline void class_##_name##ext##_destructor(class_##_name##_t *p)\
{ class_##_name##_destructor(p); }					\
static inline class_##_name##_t class_##_name##ext##_constructor(_init_args) \
{ class_##_name##_t t = _init; return t; }

#define CLASS(_name, var)						\
	class_##_name##_t var __cleanup(class_##_name##_destructor) =	\
		class_##_name##_constructor

#define DEFINE_GUARD(_name, _type, _lock, _unlock) \
	DEFINE_CLASS(_name, _type, _unlock, ({ _lock; _T; }), _type _T)

#define guard(_name) \
	CLASS(_name, __UNIQUE_ID(guard))

#define scoped_guard(_name, args...)					\
	for (CLASS(_name, scope)(args),					\
	     *done = NULL; !done; done = (void *)1)

DEFINE_FREE(kfree, void *, if (_T) kfree(_T))

DEFINE_GUARD(mutex, struct mutex *, mutex_lock(_T), mutex_unlock(_T))
#endif /* upstream cleanup helpers */

/* pkv cleanup helpers */
#define scoped_guard_begin(_name, args...)				\
	do {								\
		CLASS(_name, scope)(args), *done;			\
		for (done = NULL; !done; done = (void *)1)
#define scoped_guard_end						\
	} while(0)

DEFINE_FREE(kvfree, void *, if (_T) kvfree(_T))
DEFINE_FREE(__putname, void *, if (_T) __putname(_T))
#if PKV_VER_LT(6, 6, 49)
DEFINE_FREE(device_node, struct device_node *, if (_T) of_node_put(_T))
#endif

#if !IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON)
extern struct exynos_chipid_info exynos_soc_info;
#endif

#endif /* PABLO_KERNEL_VARIANT_H */
