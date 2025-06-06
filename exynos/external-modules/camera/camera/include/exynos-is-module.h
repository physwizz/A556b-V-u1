/*
 * /include/media/exynos_is_sensor.h
 *
 * Copyright (C) 2012 Samsung Electronics, Co. Ltd
 *
 * Exynos series exynos_is_sensor device support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MEDIA_EXYNOS_MODULE_H
#define MEDIA_EXYNOS_MODULE_H

#include <linux/platform_device.h>
#include <dt-bindings/camera/exynos_is_dt.h>

#include "exynos-is-sensor.h"

#define RESOURSE_NAME_LEN (30)

enum ixc_type {
	I2C_TYPE,
	I3C_TYPE,
};

struct exynos_sensor_pin {
	ulong pin;
	u32 delay;
	u32 value;
	char *name;
	u32 act;
	u32 voltage;

	u32 shared_rsc_type;
	struct mutex *shared_rsc_lock;
	atomic_t *shared_rsc_count;
	int shared_rsc_active;

	u32 actuator_i2c_delay;
};

struct exynos_sensor_module_match {
	u32 slave_addr;
	u32 reg;
	u32 reg_type;
	u32 expected_data;
	u32 data_type;
};

#define SET_PIN_INIT(d, s, c) d->pinctrl_index[s][c] = 0;

#define SET_PIN(d, s, c, p, n, a, v, t)                                                            \
	do {                                                                                       \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].pin = p;                              \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].name = n;                             \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].act = a;                              \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].value = v;                            \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].delay = t;                            \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].voltage = 0;                          \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_type = 0;                  \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_lock = NULL;               \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_count = NULL;              \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_active = 0;                \
		(d)->pinctrl_index[s][c]++;                                                        \
	} while (0)

#define SET_PIN_VOLTAGE(d, s, c, p, n, a, v, t, e)                                                 \
	do {                                                                                       \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].pin = p;                              \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].name = n;                             \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].act = a;                              \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].value = v;                            \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].delay = t;                            \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].voltage = e;                          \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_type = 0;                  \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_lock = NULL;               \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_count = NULL;              \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_active = 0;                \
		(d)->pinctrl_index[s][c]++;                                                        \
	} while (0)

#define SET_PIN_SHARED(d, s, c, type, lock, count, active)                                         \
	do {                                                                                       \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_type = type;           \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_lock = lock;           \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_count = count;         \
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_active = active;       \
	} while (0)

struct is_module_regulator {
	struct regulator *regulator;
	char *name;
	struct list_head list;
	bool retention_pin;
	struct mutex regulator_lock;
};

struct is_resource_share {
	char name[RESOURSE_NAME_LEN];
	struct list_head list;
	u32 init_sensor_id;
	struct mutex *shared_rsc_lock;
	atomic_t *shared_rsc_count;
	bool is_shared;
};

struct exynos_platform_is_module {
	int (*gpio_cfg)(struct is_module_enum *module, u32 scenario, u32 gpio_scenario);
	int (*gpio_dbg)(struct is_module_enum *module, u32 scenario, u32 gpio_scenario);
	struct exynos_sensor_pin pin_ctrls[SENSOR_SCENARIO_MAX][GPIO_SCENARIO_MAX][GPIO_CTRL_MAX];
	u32 pinctrl_index[SENSOR_SCENARIO_MAX][GPIO_SCENARIO_MAX];
	struct pinctrl *pinctrl;
	struct is_module_regulator *module_regulator;

	/* common */
	u32 sensor_id; /* TODO: deprecated */
	u32 active_width;
	u32 active_height;
	u32 margin_left;
	u32 margin_right;
	u32 margin_top;
	u32 margin_bottom;
	u32 pixel_width;
	u32 pixel_height;
	u32 max_framerate;
	u32 bitwidth;
	u32 use_retention_mode;
	char *sensor_maker;
	char *sensor_name;
	char *setfile_name;
	u32 sensor_module_type;
	u32 cfgs;
	struct is_sensor_cfg *cfg;
	struct is_sensor_vc_extra_info vc_extra_info[VC_BUF_DATA_TYPE_MAX];
	u32 vpd_sensor_mode;

	/* board */
	u32 position;
	u32 mclk_ch;
	u32 mclk_freq_khz;
	u32 id;
	u32 sensor_i2c_ch;
	u32 sensor_i2c_addr;
	enum ixc_type cis_ixc_type;
	bool cis_i3c_probed;

	/* peri */
	u32 flash_product_name;
	u32 flash_first_gpio;
	u32 flash_second_gpio;
	u32 af_product_name;
	u32 af_i2c_addr;
	u32 af_i2c_ch;
	u32 aperture_product_name;
	u32 aperture_i2c_addr;
	u32 aperture_i2c_ch;
	u32 ois_product_name;
	u32 ois_i2c_addr;
	u32 ois_i2c_ch;
	u32 mcu_product_name;
	u32 mcu_i2c_addr;
	u32 mcu_i2c_ch;

	/* vendor */
	u32 rom_id;
	u32 rom_cal_index;
	u32 rom_dualcal_id;
	u32 rom_dualcal_index;
	bool use_dualcal_from_file;
	char *dual_cal_file_name;
	u32 eeprom_product_name;
	u32 eeprom_i2c_ch;
	u32 eeprom_i2c_addr;
	u32 laser_af_product_name;
	bool power_seq_dt;

	struct exynos_sensor_module_match match_entry[MATCH_GROUP_MAX][MATCH_ENTRY_MAX];
	int num_of_match_entry[MATCH_GROUP_MAX];
	int num_of_match_groups;
	struct device_node *cis_np;
	struct device_node *eeprom_np;
	struct device_node *af_np;
	struct device_node *ois_np;
	/* internal use only, if you need to refer them you have to check validation of them */
	struct i2c_client *cis_client;
	struct i2c_client *eeprom_client;
	struct i2c_client *af_client;
	struct i2c_client *ois_client;
};

void is_get_gpio_ops(struct exynos_platform_is_module *pdata);

#endif /* MEDIA_EXYNOS_MODULE_H */
