/*
 * exynos_tmu.h - Samsung EXYNOS TMU (Thermal Management Unit)
 *
 *  Copyright (C) 2011 Samsung Electronics
 *  Donggeun Kim <dg77.kim@samsung.com>
 *  Amit Daniel Kachhap <amit.daniel@samsung.com>
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
 */

#ifndef _EXYNOS_TMU_H
#define _EXYNOS_TMU_H
#include <soc/samsung/cpu_cooling.h>
#include <soc/samsung/gpu_cooling.h>
#include <soc/samsung/isp_cooling.h>
#include <soc/samsung/exynos_pm_qos.h>
#if IS_ENABLED(CONFIG_EXYNOS_CPUHP)
#include <soc/samsung/exynos-cpuhp.h>
#endif
#include <linux/kthread.h>

#define MCELSIUS        1000
#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
#define BOOST_MODE_FROM_DTM		0x2

#define NOTI_STATUS_DATA_NUM		3

#define NOTI_STATUS_DATA_POS2		2
#define NOTI_STATUS_BIG_HOTPLUG_MASK	0x00000001
#define NOTI_STATUS_MID_HOTPLUG_MASK	0x00000002
#define NOTI_STATUS_MIDH_HOTPLUG_MASK	0x00000004
#define NOTI_STATUS_MIDL_HOTPLUG_MASK	0x00000008
#define NOTI_STATUS_ISP_THROTTLE_MASK	0x000000F0
#define NOTI_STATUS_CDEV_UPDATE_MASK	0x0000FF00
#define NOTI_STATUS_POLLING_MASK	0x00010000

#define NOTI_STATUS_ISP_THROTTLE_SHIFT	4
#define NOTI_STATUS_CDEV_UPDATE_SHIFT	8
#define NOTI_STATUS_POLLING_SHIFT	16

#define NOTI_STATUS_CDEV_UPDATE_NUM	8

#define NOTI_STATUS_DATA_POS1		1

enum tmu_data_list {
	TMU_DATA_START = 0,
	TMU_DATA_TEMPERATURE,
	TMU_DATA_TRIP_COUNT,
	TMU_DATA_TRIP_TYPE,
	TMU_DATA_TRIP_TEMP,
	TMU_DATA_TRIP_HYST,
	TMU_DATA_HOTPLUG_ENABLE,
	TMU_DATA_HOTPLUG_OUT_THRESHOLD,
	TMU_DATA_HOTPLUG_IN_THRESHOLD,
	TMU_DATA_USE_PI_THERMAL,
	TMU_DATA_USE_SYNC_PI_THERMAL,
	TMU_DATA_TRIP_SWITCH_ON,
	TMU_DATA_TRIP_CONTROL_TEMP,
	TMU_DATA_SUSTAINABLE_POWER,
	TMU_DATA_K_PO,
	TMU_DATA_K_PU,
	TMU_DATA_K_I,
	TMU_DATA_K_D,
	TMU_DATA_I_MAX,
	TMU_DATA_INTEGRAL_CUTOFF,
	TMU_DATA_POLLING_DELAY_ON,
	TMU_DATA_POLLING_DELAY_OFF,
	TMU_DATA_INIT_DONE,
	TMU_DATA_SWITCHED_ON,
	TMU_DATA_CUR_POLLING_DELAY,
	TMU_DATA_PI_IS_RUNNING,
	TMU_DATA_ERR_INTEGRAL,
	TMU_DATA_PREV_ERR,
	TMU_DATA_TRACE_ERR,
	TMU_DATA_TRACE_ERR_INTEGRAL,
	TMU_DATA_TRACE_P,
	TMU_DATA_TRACE_I,
	TMU_DATA_TRACE_D,
	TMU_DATA_TRACE_POWER_RANGE,
	TMU_DATA_TRACE_MAX_POWER,
	TMU_DATA_TRACE_CONTROL_TEMP,
#if IS_ENABLED(CONFIG_EXYNOS_EA_DTM)
	TMU_DATA_PPREV_DATA,
	TMU_DATA_PREV_DATA,
	TMU_DATA_CUR_DATA,
	TMU_DATA_CUR_COEF,
#endif
	TMU_DATA_NUM,
};

enum cpufreq_cdev_member {
	CPU_CDEV_START = 0,
	CPU_CDEV_ID,
	CPU_CDEV_TZ_ID,
	CPU_CDEV_MAX,
	CPU_CDEV_POWER_ACTOR,
	CPU_CDEV_FREQ,
	CPU_CDEV_POWER,
	CPU_CDEV_VOLT,
	CPU_CDEV_NUM_CPU,
	CPU_CDEV_VAR_TABLE,
	CPU_CDEV_VAR_VOLT_SIZE,
	CPU_CDEV_VAR_TEMP_SIZE,
	CPU_CDEV_NUM,
};

enum gpufreq_cdev_member {
	GPU_CDEV_START = 0,
	GPU_CDEV_ID,
	GPU_CDEV_TZ_ID,
	GPU_CDEV_NUM_LEVEL,
	GPU_CDEV_FREQ,
	GPU_CDEV_POWER_ACTOR,
	GPU_CDEV_DYN_PWRTABLE_ENTRIES,
	GPU_CDEV_DYN_FREQ,
	GPU_CDEV_DYN_VOLT,
	GPU_CDEV_DYN_POWER,
	GPU_CDEV_VAR_TABLE,
	GPU_CDEV_VAR_VOLT_SIZE,
	GPU_CDEV_VAR_TEMP_SIZE,
	GPU_CDEV_NUM,
};

enum device_cdev_member {
	DEV_CDEV_START = 0,
	DEV_CDEV_ID,
	DEV_CDEV_TZ_ID,
	DEV_CDEV_MAX,
	DEV_CDEV_FREQ,
	DEV_CDEV_NAME,
	DEV_CDEV_NUM,
};

enum isp_cdev_member {
	ISP_CDEV_START = 0,
	ISP_CDEV_ID,
	ISP_CDEV_TZ_ID,
	ISP_CDEV_FPS,
	ISP_CDEV_NUM,
};

enum thermal_instance_member {
	INSTANCE_START = 0,
	INSTANCE_CDEV_ID,
	INSTANCE_TZ_ID,
	INSTANCE_UPPER,
	INSTANCE_LOWER,
	INSTANCE_NUM,
};

#endif

struct exynos_pi_param {
	s64 err_integral;
	int trip_switch_on;
	int trip_control_temp;

	u32 sustainable_power;
	s32 k_po;
	s32 k_pu;
	s32 k_i;
	s32 k_d;
	s32 i_max;
	s32 integral_cutoff;
	s32 prev_err;

	bool switched_on;
};

/**
 * struct exynos_tmu_data : A structure to hold the private data of the TMU
	driver
 * @id: identifier of the one instance of the TMU controller.
 * @base: base address of the single instance of the TMU controller.
 * @irq: irq number of the TMU controller.
 * @soc: id of the SOC type.
 * @irq_work: pointer to the irq work structure.
 * @lock: lock to implement synchronization.
 * @regulator: pointer to the TMU regulator structure.
 * @reg_conf: pointer to structure to register with core thermal.
 * @ntrip: number of supported trip points.
 * @tmu_initialize: SoC specific TMU initialization method
 * @tmu_control: SoC specific TMU control method
 * @tmu_read: SoC specific TMU temperature read method
 * @tmu_set_emulation: SoC specific TMU emulation setting method
 * @tmu_clear_irqs: SoC specific TMU interrupts clearing method
 */
struct exynos_tmu_data {
	int id;
	/* Throttle hotplug related variables */
	bool hotplug_enable;
	int hotplug_in_threshold;
	int hotplug_out_threshold;
	bool hotplug_completely_off;
	struct cpufreq_policy *policy;
	struct freq_qos_request thermal_limit_request;
	void __iomem *base;
	int irq;
	struct kthread_worker thermal_worker;
	struct kthread_work irq_work;
	struct kthread_work hotplug_work;
	struct mutex lock;
	struct thermal_zone_device *tzd;
	unsigned int ntrip;
	bool enabled;
	struct thermal_cooling_device *cool_dev;
	struct list_head node;
	char tmu_name[THERMAL_NAME_LENGTH + 1];
	struct device_node *np;
	struct cpumask cpu_domain;
	bool is_cpu_hotplugged_out;
	char cpuhp_name[THERMAL_NAME_LENGTH + 1];
	int temperature;
	bool use_pi_thermal;
	bool use_sync_pi_thermal;
	struct kthread_delayed_work pi_work;
	struct exynos_pi_param *pi_param;
	int polling_delay_on;
	int polling_delay_off;

	struct notifier_block nb;
	atomic_t in_suspend;
	u32 external_polling;
	u32 dfs_triggered;
	ktime_t last_thermal_status_updated;
	ktime_t thermal_status[3];
};

extern int exynos_build_static_power_table(struct device_node *np, int **var_table,
		unsigned int *var_volt_size, unsigned int *var_temp_size, char *tz_name);

extern struct exynos_tmu_data *exynos_tmu_get_data_from_tz(struct thermal_zone_device *tz);

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
extern void exynos_thermal_instance_update(struct thermal_instance *instance);
#endif

#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_ABOX)
extern bool abox_is_on(void);
#else
static inline bool abox_is_on(void)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_ISP_THERMAL)
int exynos_isp_cooling_init(void);
#else
static inline int exynos_isp_cooling_init(void)
{
	return 0;
}
#endif


#if IS_ENABLED(CONFIG_EXYNOS_ADV_TRACER)
int adv_tracer_s2d_get_enable(void);
int adv_tracer_s2d_set_enable(int en);
#else
#define adv_tracer_s2d_get_enable()	do { } while (0)
#define adv_tracer_s2d_set_enable(a)	do { } while (0)
#endif

static inline bool __cdev_is_power_actor(struct thermal_cooling_device *cdev)
{
	return cdev->ops->state2power && cdev->ops->power2state;
}

#endif /* _EXYNOS_TMU_H */
