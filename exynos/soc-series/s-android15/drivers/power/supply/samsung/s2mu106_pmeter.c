/*
 * s2mu106_pmeter.c - S2MU106 Power Meter Driver
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <linux/mfd/samsung/s2mu106.h>
#include <linux/power/samsung/s2mu106_pmeter.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>

#define EN_IRQ		0
#define VOLTAGE_9V	8000
#define VOLTAGE_5V	6000

static enum power_supply_property s2mu106_pmeter_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static const unsigned char enable_bit_data[PM_TYPE_MAX] = {
	0x80, 0x40, 0x20, 0x01, 0x08, 0x04, 0x02, 0x80, 0x40, 0x01, 0x10, 0x80
};

static int s2mu106_pm_enable(struct s2mu106_pmeter_data *pmeter, int mode, enum pm_type type)
{
	u8 addr1 = S2MU106_PM_REQ_BOX_CO1;
	u8 addr2 = S2MU106_PM_REQ_BOX_CO2;
	u8 data1, data2;

	/* Default PM mode = continuous */
	if (mode == REQUEST_RESPONSE_MODE) {
		pr_info("%s PM mode : Request Response mode (RR)\n", __func__);
		addr1 = S2MU106_PM_REQ_BOX_RR1;
		addr2 = S2MU106_PM_REQ_BOX_RR2;
	}

	s2mu106_read_reg(pmeter->i2c, addr1, &data1);
	s2mu106_read_reg(pmeter->i2c, addr2, &data2);

	switch (type) {
	case PM_TYPE_VCHGIN ... PM_TYPE_VGPADC:
	case PM_TYPE_VCC2:
		data1 |= enable_bit_data[type];
		break;
	case PM_TYPE_ICHGIN:
	case PM_TYPE_IWCIN:
	case PM_TYPE_IOTG:
	case PM_TYPE_ITX:
		data2 |= enable_bit_data[type];
		break;
	default:
		return -EINVAL;
	}

	s2mu106_write_reg(pmeter->i2c, addr1, data1);
	s2mu106_write_reg(pmeter->i2c, addr2, data2);

	pr_info("%s: data1: 0x%2x, data2: 0x%2x\n", __func__, data1, data2);
	return 0;
}

static int s2mu106_pm_get_vchgin(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_voltage = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_VCHGIN, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_VCHGIN, &data2);

	charge_voltage = ((data1 << 4) | (data2 >> 4)) * 5;
	//pr_info("%s: data1: 0x%2x, data2: 0x%2x, voltage: %d\n", __func__, data1, data2, charge_voltage);
	return charge_voltage;
}

static int s2mu106_pm_get_vwcin(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_voltage = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_VWCIN, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_VWCIN, &data2);

	charge_voltage = ((data1 << 4) | (data2 >> 4)) * 5;
	pr_info("%s: data1: 0x%2x, data2: 0x%2x, voltage: %d\n", __func__, data1, data2, charge_voltage);
	return charge_voltage;
}

static int s2mu106_pm_get_vbyp(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_voltage = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_VBYP, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_VBYP, &data2);

	charge_voltage = ((data1 << 4) | (data2 >> 4)) * 5;
	pr_info("%s: data1: 0x%2x, data2: 0x%2x, voltage: %d\n", __func__, data1, data2, charge_voltage);
	return charge_voltage;
}

static int s2mu106_pm_get_vsysa(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_voltage = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_VSYS, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_VSYS, &data2);

	charge_voltage = ((data1 << 4) | (data2 >> 4)) * 25;
	charge_voltage = charge_voltage / 10;

	pr_info("%s: data1: 0x%2x, data2: 0x%2x, voltage: %d\n", __func__, data1, data2, charge_voltage);
	return charge_voltage;
}

static int s2mu106_pm_get_vbata(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_voltage = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_VBAT, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_VBAT, &data2);

	charge_voltage = ((data1 << 4) | (data2 >> 4)) * 25;
	charge_voltage = charge_voltage / 10;

	pr_info("%s: data1: 0x%2x, data2: 0x%2x, voltage: %d\n", __func__, data1, data2, charge_voltage);
	return charge_voltage;
}

static int s2mu106_pm_get_vgpadc(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_voltage = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_VGPADC, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_VGPADC, &data2);

	charge_voltage = ((data1 << 4) | (data2 >> 4)) * 25;
	charge_voltage = charge_voltage / 10;

	pr_info("%s: data1: 0x%2x, data2: 0x%2x, voltage: %d\n", __func__, data1, data2, charge_voltage);
	return charge_voltage;
}

static int s2mu106_pm_get_vcc1(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_voltage = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_VCC1, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_VCC1, &data2);

	charge_voltage = ((data1 << 4) | (data2 >> 4)) * 625;
	charge_voltage = charge_voltage / 1000;

	pr_info("%s: data1: 0x%2x, data2: 0x%2x, voltage: %d\n", __func__, data1, data2, charge_voltage);
	return charge_voltage;
}

static int s2mu106_pm_get_vcc2(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_voltage = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_VCC2, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_VCC2, &data2);

	charge_voltage = ((data1 << 4) | (data2 >> 4)) * 625;
	charge_voltage = charge_voltage / 1000;

	pr_info("%s: data1: 0x%2x, data2: 0x%2x, voltage: %d\n", __func__, data1, data2, charge_voltage);
	return charge_voltage;
}

static int s2mu106_pm_get_ichgin(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_current = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_ICHGIN, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_ICHGIN, &data2);

	charge_current = (int)((data1 << 4) | (data2 >> 4));

	//pr_info("%s: data1: 0x%2x, data2: 0x%2x, current: %d\n", __func__, data1, data2, charge_current);
	return charge_current;
}

static int s2mu106_pm_get_iwcin(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_current = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_IWCIN, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_IWCIN, &data2);

	charge_current = (int)((data1 << 4) | (data2 >> 4));

	pr_info("%s: data1: 0x%2x, data2: 0x%2x, current: %d\n", __func__, data1, data2, charge_current);
	return charge_current;
}

static int s2mu106_pm_get_iotg(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_current = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_IOTG, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_IOTG, &data2);

	charge_current = (int)((data1 << 4) | (data2 >> 4));

	pr_info("%s: data1: 0x%2x, data2: 0x%2x, current: %d\n", __func__, data1, data2, charge_current);
	return charge_current;
}

static int s2mu106_pm_get_itx(struct s2mu106_pmeter_data *pmeter)
{
	u8 data1, data2;
	int charge_current = 0;

	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL1_ITX, &data1);
	s2mu106_read_reg(pmeter->i2c, S2MU106_PM_VAL2_ITX, &data2);

	charge_current = (int)((data1 << 4) | (data2 >> 4));

	pr_info("%s: data1: 0x%2x, data2: 0x%2x, current: %d\n", __func__, data1, data2, charge_current);
	return charge_current;
}

static int s2mu106_pm_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s2mu106_pmeter_data *pmeter = power_supply_get_drvdata(psy);
	enum s2m_power_supply_property s2m_psp = (enum s2m_power_supply_property) psp;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("[DEBUG]%s: POWER_SUPPLY_PROP_ONLINE\n", __func__);
		return 1;
	case POWER_SUPPLY_S2M_PROP_MIN ... POWER_SUPPLY_S2M_PROP_MAX:
		switch (s2m_psp) {
		case POWER_SUPPLY_S2M_PROP_VCHGIN:
			val->intval = s2mu106_pm_get_vchgin(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_VWCIN:
			val->intval = s2mu106_pm_get_vwcin(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_VBYP:
			val->intval = s2mu106_pm_get_vbyp(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_VSYS:
			val->intval = s2mu106_pm_get_vsysa(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_VBAT:
			val->intval = s2mu106_pm_get_vbata(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_VGPADC:
			val->intval = s2mu106_pm_get_vgpadc(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_VCC1:
			val->intval = s2mu106_pm_get_vcc1(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_VCC2:
			val->intval = s2mu106_pm_get_vcc2(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_ICHGIN:
			val->intval = s2mu106_pm_get_ichgin(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_IWCIN:
			val->intval = s2mu106_pm_get_iwcin(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_IOTG:
			val->intval = s2mu106_pm_get_iotg(pmeter);
			break;
		case POWER_SUPPLY_S2M_PROP_ITX:
			val->intval = s2mu106_pm_get_itx(pmeter);
			break;
		default:
			return -EINVAL;
		}
		return 0;
	default:
		return -EINVAL;
	}
	return 0;
}

static int s2mu106_pm_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct s2mu106_pmeter_data *pmeter = power_supply_get_drvdata(psy);
	enum s2m_power_supply_property s2m_psp = (enum s2m_power_supply_property) psp;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		pr_info("[DEBUG]%s: POWER_SUPPLY_PROP_ONLINE\n", __func__);
		return 1;
	case POWER_SUPPLY_S2M_PROP_MIN ... POWER_SUPPLY_S2M_PROP_MAX:
		switch (s2m_psp) {
		case POWER_SUPPLY_S2M_PROP_CO_ENABLE:
			s2mu106_pm_enable(pmeter, CONTINUOUS_MODE, val->intval);
			break;
		case POWER_SUPPLY_S2M_PROP_RR_ENABLE:
			s2mu106_pm_enable(pmeter, REQUEST_RESPONSE_MODE, val->intval);
			break;
		default:
			return -EINVAL;
		}
		return 0;
	default:
		return -EINVAL;
	}
	return 0;
}
#if IS_ENABLED(EN_IRQ)
static irqreturn_t s2mu106_vchgin_isr(int irq, void *data)
{
	struct s2mu106_pmeter_data *pmeter = data;
	int voltage;

	voltage = s2mu106_pm_get_vchgin(pmeter);
	pr_info("%s voltage : %d", __func__, voltage);

	return IRQ_HANDLED;
}
#endif
static const struct of_device_id s2mu106_pmeter_match_table[] = {
	{ .compatible = "samsung,s2mu106-pmeter",},
	{},
};

static void s2mu106_powermeter_initial(struct s2mu106_pmeter_data *pmeter)
{
	s2mu106_pm_enable(pmeter, CONTINUOUS_MODE, PM_TYPE_VCHGIN);
	s2mu106_pm_enable(pmeter, CONTINUOUS_MODE, PM_TYPE_VCC1);
	s2mu106_pm_enable(pmeter, CONTINUOUS_MODE, PM_TYPE_VCC2);
}

static int s2mu106_pmeter_probe(struct platform_device *pdev)
{
	struct s2mu106_dev *s2mu106 = dev_get_drvdata(pdev->dev.parent);
	struct s2mu106_pmeter_data *pmeter;
	struct power_supply_config psy_cfg = {};
	int ret = 0;

	pr_info("%s:[BATT] S2MU106 Power meter driver probe\n", __func__);
	pmeter = kzalloc(sizeof(struct s2mu106_pmeter_data), GFP_KERNEL);
	if (!pmeter)
		return -ENOMEM;

	pmeter->dev = &pdev->dev;
	pmeter->i2c = s2mu106->muic; // share the i2c slave address with MUIC

	platform_set_drvdata(pdev, pmeter);

	pmeter->psy_pm_desc.name           = "s2mu106-pmeter"; //pmeter->pdata->powermeter_name;
	pmeter->psy_pm_desc.type           = POWER_SUPPLY_TYPE_UNKNOWN;
	pmeter->psy_pm_desc.get_property   = s2mu106_pm_get_property;
	pmeter->psy_pm_desc.set_property   = s2mu106_pm_set_property;
	pmeter->psy_pm_desc.properties     = s2mu106_pmeter_props;
	pmeter->psy_pm_desc.num_properties = ARRAY_SIZE(s2mu106_pmeter_props);

	psy_cfg.drv_data = pmeter;

	pmeter->psy_pm = power_supply_register(&pdev->dev, &pmeter->psy_pm_desc, &psy_cfg);
	if (IS_ERR(pmeter->psy_pm)) {
		pr_err("%s: Failed to Register psy_chg\n", __func__);
		ret = PTR_ERR(pmeter->psy_pm);
		goto err_power_supply_register;
	}

#if IS_ENABLED(EN_IRQ)
	pmeter->irq_vchgin = s2mu106->pdata->irq_base + S2MU106_PM_IRQ1_VCHGINUP;
	ret = request_threaded_irq(pmeter->irq_vchgin, NULL, s2mu106_vchgin_isr, 0, "vchgin-irq", pmeter);
	if (ret < 0)
		pr_err("%s: Fail to request SYS in IRQ: %d: %d\n", __func__, pmeter->irq_vchgin, ret);
#endif
	s2mu106_powermeter_initial(pmeter);

	pr_info("%s:[BATT] S2MU106 pmeter driver loaded OK\n", __func__);

	return ret;

err_power_supply_register:
	mutex_destroy(&pmeter->pmeter_mutex);
	kfree(pmeter);

	return ret;
}

static int s2mu106_pmeter_remove(struct platform_device *pdev)
{
	struct s2mu106_pmeter_data *pmeter =
		platform_get_drvdata(pdev);

	kfree(pmeter);
	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static int s2mu106_pmeter_suspend(struct device *dev)
{
	return 0;
}

static int s2mu106_pmeter_resume(struct device *dev)
{
	return 0;
}
#else
#define s2mu106_pmeter_suspend NULL
#define s2mu106_pmeter_resume NULL
#endif

static void s2mu106_pmeter_shutdown(struct device *dev)
{
	pr_info("%s: S2MU106 PowerMeter driver shutdown\n", __func__);
}

static SIMPLE_DEV_PM_OPS(s2mu106_pmeter_pm_ops, s2mu106_pmeter_suspend,
		s2mu106_pmeter_resume);

static struct platform_driver s2mu106_pmeter_driver = {
	.driver         = {
		.name   = "s2mu106-powermeter",
		.owner  = THIS_MODULE,
		.of_match_table = s2mu106_pmeter_match_table,
		.pm     = &s2mu106_pmeter_pm_ops,
		.shutdown   =   s2mu106_pmeter_shutdown,
	},
	.probe          = s2mu106_pmeter_probe,
	.remove		= s2mu106_pmeter_remove,
};

static int __init s2mu106_pmeter_init(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);
	ret = platform_driver_register(&s2mu106_pmeter_driver);

	return ret;
}
module_init(s2mu106_pmeter_init);

static void __exit s2mu106_pmeter_exit(void)
{
	platform_driver_unregister(&s2mu106_pmeter_driver);
}
module_exit(s2mu106_pmeter_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("PowerMeter driver for S2MU106");
MODULE_SOFTDEP("post: s2m_muic_module");
