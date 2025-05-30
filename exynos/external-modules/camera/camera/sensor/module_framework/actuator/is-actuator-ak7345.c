/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>

#include "is-actuator-ak7345.h"
#include "is-device-sensor.h"
#include "is-device-sensor-peri.h"
#include "is-core.h"

#include "is-helper-ixc.h"

#include "interface/is-interface-library.h"

#define ACTUATOR_NAME		"AK7345"

#define DEF_AK7345_FIRST_POSITION		120
#define DEF_AK7345_FIRST_DELAY			30

static int sensor_ak7345_write_position(struct i2c_client *client, u32 val)
{
	int ret = 0;
	u8 val_high = 0, val_low = 0;

	FIMC_BUG(!client);

	if (!client->adapter) {
		err("Could not find adapter!\n");
		ret = -ENODEV;
		goto p_err;
	}

	if (val > AK7345_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					val, AK7345_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	val_high = (val & 0x01FF) >> 1;
	val_low = (val & 0x0001) << 7;

	ret = actuator->ixc_ops->addr8_write8(client, AK7345_REG_POS_HIGH, val_high);
	if (ret < 0)
		goto p_err;
	ret = actuator->ixc_ops->addr8_write8(client, AK7345_REG_POS_LOW, val_low);

p_err:
	return ret;
}

int sensor_ak7345_actuator_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	u8 product_id = 0;
	struct is_actuator *actuator;
	struct i2c_client *client = NULL;
	struct is_minfo *minfo = is_get_is_minfo();
#ifdef DEBUG_ACTUATOR_TIME
	ktime_t st = ktime_get();
#endif

	long cal_addr;
	u32 cal_data;

	int first_position = DEF_AK7345_FIRST_POSITION;

	FIMC_BUG(!subdev);

	dbg_actuator("%s\n", __func__);

	actuator = (struct is_actuator *)v4l2_get_subdevdata(subdev);
	if (!actuator) {
		err("actuator is not detect!\n");
		goto p_err;
	}

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(actuator->ixc_lock);
	ret = actuator->ixc_ops->addr8_read8(client, 0x03, &product_id);
	if (ret < 0)
		goto p_err_mutex;

	/* EEPROM AF calData address */
	/* get pan_focus */
	cal_addr = minfo->kvaddr_cal[SENSOR_POSITION_REAR] + EEPROM_OEM_BASE;
	memcpy((void *)&cal_data, (void *)cal_addr, sizeof(cal_data));

	if (cal_data > 0)
		first_position = cal_data;

	ret = sensor_ak7345_write_position(client, first_position);
	if (ret <0)
		goto p_err_mutex;
	actuator->position = first_position;

	/* Go active mode */
	ret = actuator->ixc_ops->addr8_write8(client, 0x02, 0);
	if (ret <0)
		goto p_err_mutex;

	dbg_actuator("initial position: %d\n", first_position);

	mdelay(DEF_AK7345_FIRST_DELAY);

#ifdef DEBUG_ACTUATOR_TIME
	pr_info("[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));
#endif

p_err_mutex:
	IXC_MUTEX_UNLOCK(actuator->ixc_lock);

p_err:
	return ret;
}

int sensor_ak7345_actuator_get_status(struct v4l2_subdev *subdev, u32 *info)
{
	int ret = 0;
	struct is_actuator *actuator = NULL;
	struct i2c_client *client = NULL;
	enum is_actuator_status status = ACTUATOR_STATUS_NO_BUSY;
#ifdef DEBUG_ACTUATOR_TIME
	ktime_t st = ktime_get();
#endif

	dbg_actuator("%s\n", __func__);

	FIMC_BUG(!subdev);
	FIMC_BUG(!info);

	actuator = (struct is_actuator *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * The info is busy flag.
	 * But, this module can't get busy flag.
	 */
	status = ACTUATOR_STATUS_NO_BUSY;
	*info = status;

#ifdef DEBUG_ACTUATOR_TIME
	pr_info("[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));
#endif

p_err:
	return ret;
}

int sensor_ak7345_actuator_set_position(struct v4l2_subdev *subdev, u32 *info)
{
	int ret = 0;
	struct is_actuator *actuator;
	struct i2c_client *client;
	u32 position = 0;
	struct is_sysfs_actuator *sysfs_actuator = is_get_sysfs_actuator();
#ifdef DEBUG_ACTUATOR_TIME
	ktime_t st = ktime_get();
#endif

	FIMC_BUG(!subdev);
	FIMC_BUG(!info);

	actuator = (struct is_actuator *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!actuator);

	client = actuator->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(actuator->ixc_lock);
	position = *info;
	if (position > AK7345_POS_MAX_SIZE) {
		err("Invalid af position(position : %d, Max : %d).\n",
					position, AK7345_POS_MAX_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	/* debug option : fixed position testing */
	if (sysfs_actuator.enable_fixed)
		position = sysfs_actuator.fixed_position;

	/* position Set */
	ret = sensor_ak7345_write_position(client, position);
	if (ret <0)
		goto p_err;
	actuator->position = position;

	dbg_actuator("%s: position(%d)\n", __func__, position);

#ifdef DEBUG_ACTUATOR_TIME
	pr_info("[%s] time %ldus", __func__, PABLO_KTIME_US_DELTA_NOW(st));
#endif
p_err:
	IXC_MUTEX_UNLOCK(actuator->ixc_lock);
	return ret;
}

static int sensor_ak7345_actuator_g_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;
	u32 val = 0;

	switch(ctrl->id) {
	case V4L2_CID_ACTUATOR_GET_STATUS:
		ret = sensor_ak7345_actuator_get_status(subdev, &val);
		if (ret < 0) {
			err("err!!! ret(%d), actuator status(%d)", ret, val);
			ret = -EINVAL;
			goto p_err;
		}
		break;
	default:
		err("err!!! Unknown CID(%#x)", ctrl->id);
		ret = -EINVAL;
		goto p_err;
	}

	ctrl->value = val;

p_err:
	return ret;
}

static int sensor_ak7345_actuator_s_ctrl(struct v4l2_subdev *subdev, struct v4l2_control *ctrl)
{
	int ret = 0;

	switch(ctrl->id) {
	case V4L2_CID_ACTUATOR_SET_POSITION:
		ret = sensor_ak7345_actuator_set_position(subdev, &ctrl->value);
		if (ret) {
			err("failed to actuator set position: %d, (%d)\n", ctrl->value, ret);
			ret = -EINVAL;
			goto p_err;
		}
		break;
	default:
		err("err!!! Unknown CID(%#x)", ctrl->id);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

long sensor_ak7345_actuator_ioctl(struct v4l2_subdev *subdev, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct v4l2_control *ctrl;

	ctrl = (struct v4l2_control *)arg;
	switch (cmd) {
	case SENSOR_IOCTL_ACT_S_CTRL:
		ret = sensor_ak7345_actuator_s_ctrl(subdev, ctrl);
		if (ret) {
			err("err!!! actuator_s_ctrl failed(%d)", ret);
			goto p_err;
		}
		break;
	case SENSOR_IOCTL_ACT_G_CTRL:
		ret = sensor_ak7345_actuator_g_ctrl(subdev, ctrl);
		if (ret) {
			err("err!!! actuator_g_ctrl failed(%d)", ret);
			goto p_err;
		}
		break;
	default:
		err("err!!! Unknown command(%#x)", cmd);
		ret = -EINVAL;
		goto p_err;
	}
p_err:
	return (long)ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_ak7345_actuator_init,
	.ioctl = sensor_ak7345_actuator_ioctl,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

static int sensor_ak7345_actuator_probe_i2c(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct is_core *core= NULL;
	struct v4l2_subdev *subdev_actuator = NULL;
	struct is_actuator *actuator = NULL;
	struct is_device_sensor *device = NULL;
	u32 sensor_id = 0;
	u32 place = 0;
	struct device *dev;
	struct device_node *dnode;

	FIMC_BUG(!is_dev);
	FIMC_BUG(!client);

	core = (struct is_core *)dev_get_drvdata(is_dev);
	if (!core) {
		err("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	ret = of_property_read_u32(dnode, "id", &sensor_id);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto p_err;
	}

	ret = of_property_read_u32(dnode, "place", &place);
	if (ret) {
		pr_info("place read is fail(%d)", ret);
		place = 0;
	}
	probe_info("%s sensor_id(%d) actuator_place(%d)\n", __func__, sensor_id, place);

	device = &core->sensor[sensor_id];
	if (!test_bit(IS_SENSOR_PROBE, &device->state)) {
		err("sensor device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	actuator = kzalloc(sizeof(struct is_actuator), GFP_KERNEL);
	if (!actuator) {
		err("actuator is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	subdev_actuator = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_actuator) {
		err("subdev_actuator is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	/* This name must is match to sensor_open_extended actuator name */
	actuator->id = ACTUATOR_NAME_AK7345;
	actuator->subdev = subdev_actuator;
	actuator->device = sensor_id;
	actuator->client = client;
	actuator->position = 0;
	actuator->max_position = AK7345_POS_MAX_SIZE;
	actuator->pos_size_bit = AK7345_POS_SIZE_BIT;
	actuator->pos_direction = AK7345_POS_DIRECTION;
	actuator->ixc_lock = NULL;
	actuator->need_softlanding = 0;
	actuator->actuator_ops = NULL;
	actuator->ixc_ops = pablo_get_i2c();

	device->subdev_actuator[place] = subdev_actuator;
	device->actuator[place] = actuator;

	v4l2_i2c_subdev_init(subdev_actuator, client, &subdev_ops);
	v4l2_set_subdevdata(subdev_actuator, actuator);
	v4l2_set_subdev_hostdata(subdev_actuator, device);

	snprintf(subdev_actuator->name, V4L2_SUBDEV_NAME_SIZE, "actuator-subdev.%d", actuator->id);

	probe_info("%s done\n", __func__);
	return ret;

p_err:
	if (actuator)
		kzfree(actuator);

	if (subdev_actuator)
		kzfree(subdev_actuator);

	return ret;
}

static const struct of_device_id sensor_actuator_ak7345_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-actuator-ak7345",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_actuator_ak7345_match);

static const struct i2c_device_id sensor_actuator_ak7345_idt[] = {
	{ ACTUATOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_actuator_ak7345_driver = {
	.probe  = sensor_ak7345_actuator_probe_i2c,
	.driver = {
		.name	= ACTUATOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_actuator_ak7345_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_actuator_ak7345_idt,
};

static int __init sensor_actuator_ak7345_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_actuator_ak7345_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_actuator_ak7345_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_actuator_ak7345_init);

MODULE_LICENSE("GPL");
