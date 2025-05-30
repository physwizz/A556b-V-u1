/*
 * s2dos07.c - Regulator driver for the Samsung s2dos07
 *
 * Copyright (C) 2023 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/pmic/s2dos07.h>
#include <linux/pmic/pmic_class.h>
#include <linux/regulator/of_regulator.h>
#include <linux/notifier.h>

struct s2dos07_data {
	struct s2dos07_dev *iodev;
	int num_regulators;
	struct regulator_dev *rdev[S2DOS07_REGULATOR_MAX];
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	u8 read_addr;
	u8 read_val;
	struct device *dev;
#endif
};

int s2dos07_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct s2dos07_data *info = i2c_get_clientdata(i2c);
	struct s2dos07_dev *s2dos07 = info->iodev;
	int ret;

	mutex_lock(&s2dos07->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&s2dos07->i2c_lock);
	if (ret < 0) {
		pr_info("%s:%s reg(0x%02hhx), ret(%d)\n",
			 MFD_DEV_NAME, __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(s2dos07_read_reg);

int s2dos07_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct s2dos07_data *info = i2c_get_clientdata(i2c);
	struct s2dos07_dev *s2dos07 = info->iodev;
	int ret;

	mutex_lock(&s2dos07->i2c_lock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&s2dos07->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(s2dos07_bulk_read);

int s2dos07_read_word(struct i2c_client *i2c, u8 reg)
{
	struct s2dos07_data *info = i2c_get_clientdata(i2c);
	struct s2dos07_dev *s2dos07 = info->iodev;
	int ret;

	mutex_lock(&s2dos07->i2c_lock);
	ret = i2c_smbus_read_word_data(i2c, reg);
	mutex_unlock(&s2dos07->i2c_lock);
	if (ret < 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL_GPL(s2dos07_read_word);

int s2dos07_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct s2dos07_data *info = i2c_get_clientdata(i2c);
	struct s2dos07_dev *s2dos07 = info->iodev;
	int ret;

	mutex_lock(&s2dos07->i2c_lock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&s2dos07->i2c_lock);
	if (ret < 0)
		pr_info("%s:%s reg(0x%02hhx), ret(%d)\n",
				MFD_DEV_NAME, __func__, reg, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(s2dos07_write_reg);

int s2dos07_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct s2dos07_data *info = i2c_get_clientdata(i2c);
	struct s2dos07_dev *s2dos07 = info->iodev;
	int ret;

	mutex_lock(&s2dos07->i2c_lock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&s2dos07->i2c_lock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(s2dos07_bulk_write);

int s2dos07_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct s2dos07_data *info = i2c_get_clientdata(i2c);
	struct s2dos07_dev *s2dos07 = info->iodev;
	int ret;
	u8 old_val, new_val;

	mutex_lock(&s2dos07->i2c_lock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		old_val = ret & 0xff;
		new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&s2dos07->i2c_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(s2dos07_update_reg);

static int s2m_enable(struct regulator_dev *rdev)
{
	struct s2dos07_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;

	return s2dos07_update_reg(i2c, rdev->desc->enable_reg,
					rdev->desc->enable_mask,
					rdev->desc->enable_mask);
}

static int s2m_disable_regmap(struct regulator_dev *rdev)
{
	struct s2dos07_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	u8 val;

	if (rdev->desc->enable_is_inverted)
		val = rdev->desc->enable_mask;
	else
		val = 0;

	return s2dos07_update_reg(i2c, rdev->desc->enable_reg,
				   val, rdev->desc->enable_mask);
}

static int s2m_is_enabled_regmap(struct regulator_dev *rdev)
{
	struct s2dos07_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	ret = s2dos07_read_reg(i2c, rdev->desc->enable_reg, &val);
	if (ret < 0)
		return ret;

	if (rdev->desc->enable_is_inverted)
		return (val & rdev->desc->enable_mask) == 0;
	else
		return (val & rdev->desc->enable_mask) != 0;
}

static int s2m_get_voltage_sel_regmap(struct regulator_dev *rdev)
{
	struct s2dos07_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;
	u8 val;

	ret = s2dos07_read_reg(i2c, rdev->desc->vsel_reg, &val);
	if (ret < 0)
		return ret;

	val &= rdev->desc->vsel_mask;

	return val;
}

static int s2m_set_voltage_sel_regmap_buck(struct regulator_dev *rdev,
					unsigned sel)
{
	struct s2dos07_data *info = rdev_get_drvdata(rdev);
	struct i2c_client *i2c = info->iodev->i2c;
	int ret;

	ret = s2dos07_update_reg(i2c, rdev->desc->vsel_reg,
			sel, rdev->desc->vsel_mask);
	if (ret < 0)
		goto out;

	if (rdev->desc->apply_bit)
		ret = s2dos07_update_reg(i2c, rdev->desc->apply_reg,
					 rdev->desc->apply_bit,
					 rdev->desc->apply_bit);
	return ret;
out:
	pr_warn("%s: failed to set voltage_sel_regmap\n", rdev->desc->name);
	return ret;
}

static int s2m_set_voltage_time_sel(struct regulator_dev *rdev,
				   unsigned int old_selector,
				   unsigned int new_selector)
{
	int old_volt, new_volt;

	/* sanity check */
	if (!rdev->desc->ops->list_voltage)
		return -EINVAL;

	old_volt = rdev->desc->ops->list_voltage(rdev, old_selector);
	new_volt = rdev->desc->ops->list_voltage(rdev, new_selector);

	if (old_selector < new_selector)
		return DIV_ROUND_UP(new_volt - old_volt, S2DOS07_RAMP_DELAY);

	return 0;
}

static struct regulator_ops s2dos07_buck_ops = {
	.list_voltage		= regulator_list_voltage_linear,
	.map_voltage		= regulator_map_voltage_linear,
	.is_enabled		= s2m_is_enabled_regmap,
	.enable			= s2m_enable,
	.disable		= s2m_disable_regmap,
	.get_voltage_sel	= s2m_get_voltage_sel_regmap,
	.set_voltage_sel	= s2m_set_voltage_sel_regmap_buck,
	.set_voltage_time_sel	= s2m_set_voltage_time_sel,
};

#define _BUCK(macro)	S2DOS07_BUCK##macro
#define _buck_ops(num)	s2dos07_buck_ops##num
#define _REG(ctrl)	S2DOS07_REG##ctrl
#define _MASK(macro)	S2DOS07_ENABLE_MASK##macro
#define _TIME(macro)	S2DOS07_ENABLE_TIME##macro

#define BUCK_DESC(_name, _id, _ops, m, s, v, e, em, t)	{	\
	.name		= _name,				\
	.id		= _id,					\
	.ops		= _ops,					\
	.type		= REGULATOR_VOLTAGE,			\
	.owner		= THIS_MODULE,				\
	.min_uV		= m,					\
	.uV_step	= s,					\
	.n_voltages	= S2DOS07_BUCK_N_VOLTAGES,		\
	.vsel_reg	= v,					\
	.vsel_mask	= S2DOS07_BUCK_VSEL_MASK,		\
	.enable_reg	= e,					\
	.enable_mask	= em,					\
	.enable_time	= t					\
}

static struct regulator_desc regulators[S2DOS07_REGULATOR_MAX] = {
	/* name, id, ops, min_uv, uV_step, vsel_reg, enable_reg */
	BUCK_DESC("s2dos07-buck1", _BUCK(1), &_buck_ops(), _BUCK(_MIN1),
		_BUCK(_STEP1), _REG(_BUCK_VOUT),
		_REG(_BUCK_EN), _MASK(_B1), _TIME(_BUCK)),
};

static BLOCKING_NOTIFIER_HEAD(s2dos07_irq_notifier);
int s2dos07_register_irq_notifier(struct notifier_block *nb)
{
	int ret = 0;

	ret = blocking_notifier_chain_register(&s2dos07_irq_notifier, nb);
	if (ret < 0)
		pr_err("%s: fail blocking notifier chain register(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(s2dos07_register_irq_notifier);

static int s2dos07_call_irq_notifier(struct s2dos07_irq_data *irq_data)
{
	int ret = 0;

	ret = blocking_notifier_call_chain(&s2dos07_irq_notifier, 0, irq_data);
	if (ret < 0)
		pr_err("%s: fail to call s2dos07 irq notifier(%d)\n", __func__, ret);

	return ret;
}

static void s2dos07_notifier_work(struct work_struct *work)
{
	struct s2dos07_dev *iodev = container_of(work, struct s2dos07_dev,
		notifier_work.work);

	s2dos07_call_irq_notifier(&iodev->irq_data);
	dev_info(iodev->dev, "%s : name = %s, val = 0x%x, irq = %d\n", __func__,
		iodev->irq_data.name, iodev->irq_data.val, iodev->irq_data.irq_num);
}

int s2dos07_create_irq_notifier_wq(struct device *dev, struct s2dos07_dev *iodev)
{
	iodev->notifier_wqueue = create_singlethread_workqueue("s2dos07-irq-notifier-wq");
	if (!iodev->notifier_wqueue) {
		pr_err("%s: failed to create notifier work_queue\n", __func__);
		return -ESRCH;
	}

	INIT_DELAYED_WORK(&iodev->notifier_work, s2dos07_notifier_work);

	return 0;
}
EXPORT_SYMBOL_GPL(s2dos07_create_irq_notifier_wq);

static irqreturn_t s2dos07_irq_thread(int irq, void *irq_data)
{
	struct s2dos07_data *s2dos07 = irq_data;
	u8 val = 0;

	s2dos07_read_reg(s2dos07->iodev->i2c, S2DOS07_REG_IRQ, &val);
	dev_info(s2dos07->iodev->dev, "%s: irq(%d) S2DOS07_REG_IRQ : 0x%02hhx\n", __func__, irq, val);

	s2dos07->iodev->irq_data.name = "S2DOS07_REG_IRQ";
	s2dos07->iodev->irq_data.irq_num = irq;
	s2dos07->iodev->irq_data.val = val;
	queue_delayed_work(s2dos07->iodev->notifier_wqueue, &s2dos07->iodev->notifier_work, 0);

	return IRQ_HANDLED;
}

#if IS_ENABLED(CONFIG_OF)
static int s2dos07_pmic_dt_parse_pdata(struct device *dev,
					struct s2dos07_platform_data *pdata)
{
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct s2dos07_regulator_data *rdata;
	size_t i;

	pmic_np = dev->of_node;
	if (!pmic_np) {
		dev_err(dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}

	pdata->dp_pmic_irq = of_get_named_gpio(pmic_np, "s2dos07,s2dos07_int", 0);
	if (pdata->dp_pmic_irq < 0)
		pr_err("%s error reading s2dos07_irq = %d\n",
			__func__, pdata->dp_pmic_irq);

	pdata->wakeup = of_property_read_bool(pmic_np, "s2dos07,wakeup");

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	/* count the number of regulators to be supported in pmic */
	pdata->num_regulators = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		pdata->num_regulators++;
	}

	rdata = devm_kzalloc(dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		dev_err(dev,
			"could not allocate memory for regulator data\n");
		return -ENOMEM;
	}

	pdata->regulators = rdata;
	pdata->num_rdata = 0;
	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < ARRAY_SIZE(regulators); i++)
			if (!of_node_cmp(reg_np->name,
					regulators[i].name))
				break;

		if (i == ARRAY_SIZE(regulators)) {
			dev_warn(dev,
			"don't know how to configure regulator %s\n",
			reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(
						dev, reg_np,
						&regulators[i]);
		rdata->reg_node = reg_np;
		rdata++;
		pdata->num_rdata++;
	}
	of_node_put(regulators_np);

	return 0;
}
#else
static int s2dos07_pmic_dt_parse_pdata(struct s2dos07_dev *iodev,
					struct s2dos07_platform_data *pdata)
{
	return 0;
}
#endif /* CONFIG_OF */
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
static ssize_t s2dos07_read_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct s2dos07_data *s2dos07 = dev_get_drvdata(dev);
	int ret;
	u8 val, reg_addr;

	if (buf == NULL) {
		pr_info("%s: empty buffer\n", __func__);
		return -1;
	}

	ret = kstrtou8(buf, 0, &reg_addr);
	if (ret < 0)
		pr_info("%s: fail to transform i2c address\n", __func__);

	ret = s2dos07_read_reg(s2dos07->iodev->i2c, reg_addr, &val);
	if (ret < 0)
		pr_info("%s: fail to read i2c address\n", __func__);

	pr_info("%s: reg(0x%02hhx) data(0x%02hhx)\n", __func__, reg_addr, val);
	s2dos07->read_addr = reg_addr;
	s2dos07->read_val = val;

	return size;
}

static ssize_t s2dos07_read_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct s2dos07_data *s2dos07 = dev_get_drvdata(dev);
	return sprintf(buf, "0x%02hhx: 0x%02hhx\n", s2dos07->read_addr,
		       s2dos07->read_val);
}

static ssize_t s2dos07_write_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	struct s2dos07_data *s2dos07 = dev_get_drvdata(dev);
	int ret;
	u8 reg = 0, data = 0;

	if (buf == NULL) {
		pr_info("%s: empty buffer\n", __func__);
		return size;
	}

	ret = sscanf(buf, "0x%02hhx 0x%02hhx", &reg, &data);
	if (ret != 2) {
		pr_info("%s: input error\n", __func__);
		return size;
	}

	pr_info("%s: reg(0x%02hhx) data(0x%02hhx)\n", __func__, reg, data);

	ret = s2dos07_write_reg(s2dos07->iodev->i2c, reg, data);
	if (ret < 0)
		pr_info("%s: fail to write i2c addr/data\n", __func__);

	return size;
}

static ssize_t s2dos07_write_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	return sprintf(buf, "echo (register addr.) (data) > s2dos07_write\n");
}

#define ATTR_REGULATOR	(2)
static struct pmic_device_attribute regulator_attr[] = {
	PMIC_ATTR(write, S_IRUGO | S_IWUSR, s2dos07_write_show, s2dos07_write_store),
	PMIC_ATTR(read, S_IRUGO | S_IWUSR, s2dos07_read_show, s2dos07_read_store),
};

static int s2dos07_create_sysfs(struct s2dos07_data *s2dos07)
{
	struct device *s2dos07_pmic = s2dos07->dev;
	struct device *dev = s2dos07->iodev->dev;
	char device_name[32] = {0, };
	int err = -ENODEV, i = 0;

	pr_info("%s()\n", __func__);
	s2dos07->read_addr = 0;
	s2dos07->read_val = 0;

	/* Dynamic allocation for device name */
	snprintf(device_name, sizeof(device_name) - 1, "%s@%s",
		 dev_driver_string(dev), dev_name(dev));

	s2dos07_pmic = pmic_device_create(s2dos07, device_name);
	s2dos07->dev = s2dos07_pmic;

	/* Create sysfs entries */
	for (i = 0; i < ATTR_REGULATOR; i++) {
		err = device_create_file(s2dos07_pmic, &regulator_attr[i].dev_attr);
		if (err)
			goto remove_pmic_device;
	}

	return 0;

remove_pmic_device:
	for (i--; i >= 0; i--)
		device_remove_file(s2dos07_pmic, &regulator_attr[i].dev_attr);
	pmic_device_destroy(s2dos07_pmic->devt);

	return -1;
}
#endif
static int s2dos07_pmic_probe(struct i2c_client *i2c)
{
	struct s2dos07_dev *iodev;
	struct s2dos07_platform_data *pdata = i2c->dev.platform_data;
	struct regulator_config config = { };
	struct s2dos07_data *s2dos07;
	size_t i;
	int ret = 0;
	u8 val = 0, mask = 0;

	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);

	iodev = devm_kzalloc(&i2c->dev, sizeof(struct s2dos07_dev), GFP_KERNEL);
	if (!iodev) {
		dev_err(&i2c->dev, "%s: Failed to alloc mem for s2dos07\n",
							__func__);
		return -ENOMEM;
	}

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,
			sizeof(struct s2dos07_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_pdata;
		}

		ret = s2dos07_pmic_dt_parse_pdata(&i2c->dev, pdata);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get device of_node\n");
			goto err_pdata;
		}

		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	iodev->dev = &i2c->dev;
	iodev->i2c = i2c;

	ret = s2dos07_create_irq_notifier_wq(iodev->dev, iodev);
	if (ret < 0)
		return ret;

	if (pdata) {
		iodev->pdata = pdata;
		iodev->wakeup = pdata->wakeup;
	} else {
		ret = -EINVAL;
		goto err_pdata;
	}
	mutex_init(&iodev->i2c_lock);

	s2dos07 = devm_kzalloc(&i2c->dev, sizeof(struct s2dos07_data),
				GFP_KERNEL);
	if (!s2dos07) {
		pr_info("[%s:%d] if (!s2dos07)\n", __FILE__, __LINE__);
		ret = -ENOMEM;
		goto err_s2dos07_data;
	}

	i2c_set_clientdata(i2c, s2dos07);
	s2dos07->iodev = iodev;
	s2dos07->num_regulators = pdata->num_rdata;

	for (i = 0; i < pdata->num_rdata; i++) {
		int id = pdata->regulators[i].id;
		config.dev = &i2c->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = s2dos07;
		config.of_node = pdata->regulators[i].reg_node;
		s2dos07->rdev[i] = devm_regulator_register(&i2c->dev,
							   &regulators[id], &config);
		if (IS_ERR(s2dos07->rdev[i])) {
			ret = PTR_ERR(s2dos07->rdev[i]);
			dev_err(&i2c->dev, "regulator init failed for %d\n",
				id);
			s2dos07->rdev[i] = NULL;
			goto err_s2dos07_data;
		}
	}

	val = (S2DOS07_IRQ_UVP_MASK | S2DOS07_IRQ_OVP_MASK | S2DOS07_IRQ_PRETSD_MASK
		| S2DOS07_IRQ_TSD_MASK | S2DOS07_IRQ_SSD_MASK | S2DOS07_IRQ_UVLO_MASK);
	mask = (S2DOS07_IRQ_UVP_MASK | S2DOS07_IRQ_OVP_MASK | S2DOS07_IRQ_PRETSD_MASK
		| S2DOS07_IRQ_TSD_MASK | S2DOS07_IRQ_SSD_MASK | S2DOS07_IRQ_UVLO_MASK);
	ret = s2dos07_update_reg(iodev->i2c, S2DOS07_REG_IRQ_MASK, val, mask);
	if (ret < 0) {
		dev_err(&i2c->dev, "Failed to mask IRQ MASK address\n");
		return ret;
	}

	if (pdata->dp_pmic_irq > 0) {
		iodev->dp_pmic_irq = gpio_to_irq(pdata->dp_pmic_irq);
		pr_info("%s : dp_pmic_irq = %d\n", __func__, iodev->dp_pmic_irq);
		if (iodev->dp_pmic_irq > 0) {
			ret = request_threaded_irq(iodev->dp_pmic_irq,
					NULL, s2dos07_irq_thread,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					"dp-pmic-irq", s2dos07);
			if (ret) {
				dev_err(&i2c->dev,
						"%s: Failed to Request IRQ\n", __func__);
				goto err_s2dos07_data;
			}
			if (pdata->wakeup) {
				ret = enable_irq_wake(iodev->dp_pmic_irq);
				if (ret < 0)
					dev_err(&i2c->dev,
							"%s: Failed to Enable Wakeup Source(%d)\n",
							__func__, ret);
				ret = device_init_wakeup(iodev->dev, pdata->wakeup);
				if (ret < 0)
					dev_err(&i2c->dev, "%s: Fail to device init wakeup fail(%d)\n",
					__func__, ret);
			}
		} else {
			dev_err(&i2c->dev, "%s: Failed gpio_to_irq(%d)\n",
					__func__, iodev->dp_pmic_irq);
			goto err_s2dos07_data;
		}
	}
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	ret = s2dos07_create_sysfs(s2dos07);
	if (ret < 0) {
		pr_err("%s: s2dos07_create_sysfs fail\n", __func__);
		goto err_s2dos07_data;
	}
#endif
	return ret;

err_s2dos07_data:
	mutex_destroy(&iodev->i2c_lock);
	destroy_workqueue(iodev->notifier_wqueue);
err_pdata:
	return ret;
}

#if IS_ENABLED(CONFIG_OF)
static struct of_device_id s2dos07_i2c_dt_ids[] = {
	{ .compatible = "samsung,s2dos07pmic" },
	{ },
};
#endif /* CONFIG_OF */

static void s2dos07_pmic_remove(struct i2c_client *i2c)
{
	struct s2dos07_data *info = i2c_get_clientdata(i2c);
#if IS_ENABLED(CONFIG_DRV_SAMSUNG_PMIC)
	struct device *s2dos07_pmic = info->dev;
	int i = 0;

	dev_info(&i2c->dev, "%s\n", __func__);

	if (info->iodev->notifier_wqueue)
		destroy_workqueue(info->iodev->notifier_wqueue);

	/* Remove sysfs entries */
	for (i = 0; i < ATTR_REGULATOR; i++)
		device_remove_file(s2dos07_pmic, &regulator_attr[i].dev_attr);
	pmic_device_destroy(s2dos07_pmic->devt);
#else
	dev_info(&i2c->dev, "%s\n", __func__);
#endif

}

#define s2dos07_pmic_suspend	NULL
#define s2dos07_pmic_resume	NULL

static const struct dev_pm_ops s2dos07_pmic_pm = {
	.suspend = s2dos07_pmic_suspend,
	.resume = s2dos07_pmic_resume,
};

#if IS_ENABLED(CONFIG_OF)
static const struct i2c_device_id s2dos07_pmic_id[] = {
	{"s2dos07-regulator", 0},
	{},
};
#endif

static struct i2c_driver s2dos07_i2c_driver = {
	.driver = {
		.name = "s2dos07-regulator",
		.owner = THIS_MODULE,
		.pm = &s2dos07_pmic_pm,
#if IS_ENABLED(CONFIG_OF)
		.of_match_table	= s2dos07_i2c_dt_ids,
#endif /* CONFIG_OF */
		.suppress_bind_attrs = true,
	},
	.probe = s2dos07_pmic_probe,
	.remove = s2dos07_pmic_remove,
	.id_table = s2dos07_pmic_id,
};

static int __init s2dos07_i2c_init(void)
{
	pr_info("%s:%s\n", MFD_DEV_NAME, __func__);
	return i2c_add_driver(&s2dos07_i2c_driver);
}
subsys_initcall(s2dos07_i2c_init);

static void __exit s2dos07_i2c_exit(void)
{
	i2c_del_driver(&s2dos07_i2c_driver);
}
module_exit(s2dos07_i2c_exit);

MODULE_DESCRIPTION("SAMSUNG s2dos07 Regulator Driver");
MODULE_LICENSE("GPL");
