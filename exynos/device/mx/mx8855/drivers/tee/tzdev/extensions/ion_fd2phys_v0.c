/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "IONFD2PHYS: " fmt
/* #define DEBUG */

#include <linux/compat.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#if defined(CONFIG_ION_FD2PHYS_USE_ION_FROM_STAGING)
#include "../../../staging/android/ion/ion.h"
#include <linux/dma-buf.h>
#else
#include <linux/ion.h>
#endif

#include "tzdev_internal.h"
#include "core/cdev.h"
#include "core/subsystem.h"

#define TZDEV_MAX_PFNS_COUNT	(SIZE_MAX / sizeof(sk_pfn_t))

struct ionfd2phys {
	int fd;
	size_t nr_pfns;
	sk_pfn_t *pfns;
};

#ifdef CONFIG_COMPAT
struct ionfd2phys32 {
	int fd;
	unsigned int nr_pfns;
	u32 pfns;
};
#endif

#if defined(CONFIG_ARCH_EXYNOS)
extern struct ion_device *ion_exynos;
#elif defined(CONFIG_MACH_MT6853) || defined(CONFIG_MACH_MT6768)
extern struct ion_device *g_ion_device;
#endif

static struct ion_client *client;

static long __ionfd2phys_ioctl(int fd, size_t nr_pfns, sk_pfn_t *pfns)
{
	int ret;
	struct ion_handle *handle;
	void *addr;
	int pfn;
	size_t size = 0;

#if defined(CONFIG_MACH_MT6853) || defined(CONFIG_MACH_MT6768)
	handle = ion_import_dma_buf_fd(client, fd);
#else
	handle = ion_import_dma_buf(client, fd);
#endif

	if (IS_ERR_OR_NULL(handle)) {
		pr_err("Failed to import an ION FD\n");
		ret = handle ? PTR_ERR(handle) : -EINVAL;
		goto out;
	}

	ret = ion_phys(client, handle, (ion_phys_addr_t *)&addr, &size);
	if (ret) {
		pr_err("Failed ION FD: ion_phys()\n");
		addr = ion_map_kernel(client, handle);
		if (IS_ERR_OR_NULL(addr)) {
			pr_err("Failed to map an ION FD\n");
			ret = addr ? PTR_ERR(addr) : -EINVAL;
			goto handle_free;
		}

		/* There is no kernel public API for getting imported buffer
		*  size. So just use what user has supplied */
		for (pfn = 0; pfn < nr_pfns; ++pfn) {
			pfns[pfn] = vmalloc_to_pfn(addr);
			addr += PAGE_SIZE;
		}
		ion_unmap_kernel(client, handle);
	} else {
		for (pfn = 0; pfn < nr_pfns; ++pfn) {
			pfns[pfn] = (sk_pfn_t)((unsigned long)addr >> PAGE_SHIFT);
			addr += PAGE_SIZE;
		}
	}

	ret = 0;

handle_free:
	ion_free(client, handle);
out:
	return ret;
}

static long ionfd2phys_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret;
	struct ionfd2phys data;
	sk_pfn_t *pfns;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
		pr_err("Failed ION FD: copy_from_user()\n");
		ret = -EFAULT;
		goto out;
	}

	if (data.nr_pfns > TZDEV_MAX_PFNS_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	pfns = kzalloc(data.nr_pfns * sizeof(sk_pfn_t), GFP_KERNEL);
	if (!pfns) {
		pr_err("Failed ION FD: kzalloc(%zu)\n", data.nr_pfns);
		ret = -ENOMEM;
		goto out;
	}

	ret = __ionfd2phys_ioctl(data.fd, data.nr_pfns, pfns);
	if (ret) {
		pr_err("Failed ION FD: __ionfd2phys_ioctl()\n");
		goto pfns_free;
	}

	if (copy_to_user((void __user *)data.pfns, pfns,
				data.nr_pfns * sizeof(sk_pfn_t))) {
		pr_err("Failed ION FD: copy_to_user()\n");
		ret = -EFAULT;
		goto pfns_free;
	}

pfns_free:
	kfree(pfns);
out:
	return ret;
}

#ifdef CONFIG_COMPAT
static long compat_ionfd2phys_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int ret;
	struct ionfd2phys32 data;
	sk_pfn_t *pfns;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
		pr_err("Failed ION FD: copy_from_user()\n");
		ret = -EFAULT;
		goto out;
	}

	if (data.nr_pfns > TZDEV_MAX_PFNS_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	pfns = kzalloc(data.nr_pfns * sizeof(sk_pfn_t), GFP_KERNEL);
	if (!pfns) {
		pr_err("Failed ION FD: kzalloc(%d)\n", data.nr_pfns);
		ret = -ENOMEM;
		goto out;
	}

	ret = __ionfd2phys_ioctl(data.fd, (size_t)data.nr_pfns, pfns);
	if (ret) {
		pr_err("Failed ION FD: __ionfd2phys_ioctl()\n");
		goto pfns_free;
	}

	if (copy_to_user(compat_ptr(data.pfns), pfns,
				data.nr_pfns * sizeof(sk_pfn_t))) {
		pr_err("Failed ION FD: copy_to_user()\n");
		ret = -EFAULT;
		goto pfns_free;
	}

pfns_free:
	kfree(pfns);
out:
	return ret;
}
#endif

static const struct file_operations ionfd2phys_fops = {
	.unlocked_ioctl = ionfd2phys_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_ionfd2phys_ioctl,
#endif
};

static struct tz_cdev ionfd2phys_cdev = {
	.name = "ionfd2phys",
	.fops = &ionfd2phys_fops,
	.owner = THIS_MODULE,
};

static ssize_t system_heap_id_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
#if defined(CONFIG_ARCH_EXYNOS) || defined(CONFIG_MACH_MT6853) || defined(CONFIG_MACH_MT6768)
	return sprintf(buf, "%d\n", ION_HEAP_TYPE_SYSTEM);
#endif
	return -ENOSYS;
}

static DEVICE_ATTR(system_heap_id, S_IRUGO, system_heap_id_show, NULL);

static ssize_t ionfd2phys_id_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", 0);
}

static DEVICE_ATTR(ionfd2phys_id, S_IRUGO, ionfd2phys_id_show, NULL);

int ionfd2phys_init(void)
{
	int ret;

#if defined(CONFIG_ARCH_EXYNOS)
	struct ion_device *ion_dev = ion_exynos;
#elif defined(CONFIG_MACH_MT6853) || defined(CONFIG_MACH_MT6768)
	struct ion_device *ion_dev = g_ion_device;
#endif
	pr_devel("module init\n");

#if defined(CONFIG_ARCH_EXYNOS) || defined(CONFIG_MACH_MT6853) || defined(CONFIG_MACH_MT6768)
	if (!ion_dev) {
		pr_err("Failed to get ion device\n");
		return 0;
	}

	client = ion_client_create(ion_dev, "IONFD2PHYS");
#endif
	if (IS_ERR_OR_NULL(client)) {
		pr_err("Failed to create an ION client\n");
		ret = client ? PTR_ERR(client) : -ENOSYS;
		goto out;
	}

	ret = tz_cdev_register(&ionfd2phys_cdev);
	if (ret)
		goto ion_destroy;

	ret = device_create_file(ionfd2phys_cdev.device, &dev_attr_system_heap_id);
	if (ret) {
		pr_err("system_heap_id sysfs file creation failed\n");
		goto out_unregister;
	}

	ret = device_create_file(ionfd2phys_cdev.device, &dev_attr_ionfd2phys_id);
	if (ret) {
		pr_err("ionfd2phys_id sysfs file creation failed\n");
		goto out_remove_file;
	}
	return 0;

out_remove_file:
	device_remove_file(ionfd2phys_cdev.device, &dev_attr_system_heap_id);
out_unregister:
	tz_cdev_unregister(&ionfd2phys_cdev);
ion_destroy:
	ion_client_destroy(client);
out:
	return ret;
}

void ionfd2phys_exit(void)
{
	pr_devel("module exit\n");
	tz_cdev_unregister(&ionfd2phys_cdev);
	ion_client_destroy(client);
}

tzdev_initcall(ionfd2phys_init);
tzdev_exitcall(ionfd2phys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergey Fedorov <s.fedorov@samsung.com>");
