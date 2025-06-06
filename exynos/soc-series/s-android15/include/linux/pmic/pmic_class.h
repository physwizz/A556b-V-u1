#ifndef PMIC_CLASS_H
#define PMIC_CLASS_H

#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)

#define PMIC_ATTR(_name, _mode, _show, _store)	\
	{ .dev_attr = __ATTR(_name, _mode, _show, _store) }

struct pmic_sysfs_dev {
	uint16_t base_addr;
	uint8_t read_addr;
	uint8_t read_val;
	struct device *dev;
};

extern struct device *pmic_device_create(void *drvdata, const char *fmt);
extern void pmic_device_destroy(dev_t devt);

struct pmic_device_attribute {
	struct device_attribute dev_attr;
};

#else
#define pmic_device_create(a, b)		(-1)
#define pmic_device_destroy(a)		do { } while (0)
#endif

#endif /* PMIC_CLASS_H */
