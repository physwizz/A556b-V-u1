#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/version.h>

/* CAUTION : Do not be declared as external sec_class  */
static struct class *sec_class;
static atomic_t sec_dev;

static int sec_class_match_device_by_name(struct device *dev, const void *data)
{
	const char *name = data;

	return sysfs_streq(name, dev_name(dev));
}
struct device *sec_dev_get_by_name(const char *name)
{
	return class_find_device(sec_class, NULL,
		(void *)name, sec_class_match_device_by_name);
}
EXPORT_SYMBOL(sec_dev_get_by_name);

struct device *sec_device_create(void *drvdata, const char *fmt)
{
	struct device *dev;

	if (!sec_class) {
		pr_err("Not yet created class(sec)!\n");
		BUG();
	}

	if (IS_ERR(sec_class)) {
		pr_err("Failed to create class(sec) %ld\n", PTR_ERR(sec_class));
		BUG();
	}

	dev = device_create(sec_class, NULL, atomic_inc_return(&sec_dev),
			drvdata, fmt);
	if (IS_ERR(dev))
		pr_err("Failed to create device %s %ld\n", fmt, PTR_ERR(dev));
	else
		pr_debug("%s : %s : %d\n", __func__, fmt, dev->devt);

	return dev;
}
EXPORT_SYMBOL(sec_device_create);

void sec_device_destroy(dev_t devt)
{
	pr_info("%s : %d\n", __func__, devt);
	device_destroy(sec_class, devt);
}
EXPORT_SYMBOL(sec_device_destroy);

static int __init sec_class_init(void)
{
#if (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
	sec_class = class_create("sec");
#else
	sec_class = class_create(THIS_MODULE, "sec");
#endif

	if (IS_ERR(sec_class)) {
		pr_err("Failed to create class(sec) %ld\n", PTR_ERR(sec_class));
		return PTR_ERR(sec_class);
	}

	return 0;
}

static void __exit sec_class_exit(void)
{
	class_destroy(sec_class);
}

arch_initcall(sec_class_init);
module_exit(sec_class_exit);

MODULE_DESCRIPTION("sec-class");
MODULE_LICENSE("GPL v2");
