/*
 *
 *  Copyright (C) 2012	Samsung Electronics Co., Ltd(http://www.samsung.com)
 *  Copyright (C) 2012  Amit Daniel <amit.kachhap@linaro.org>
 *
 *  Copyright (C) 2014  Viresh Kumar <viresh.kumar@linaro.org>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
//TEST
//TEST
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/idr.h>
#include <linux/pm_opp.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <soc/samsung/cpu_cooling.h>
#include <soc/samsung/exynos/debug-snapshot.h>

#include <trace/events/thermal_exynos.h>

#include <soc/samsung/tmu.h>
#include <soc/samsung/ect_parser.h>
#include <soc/samsung/freq-qos-tracer.h>
#include "exynos_tmu.h"
#include "../../../../../common/drivers/thermal/thermal_core.h"

#include "exynos_acpm_tmu.h"

extern int exynos_build_static_power_table(struct device_node *np, int **var_table,
		unsigned int *var_volt_size, unsigned int *var_temp_size, char *tz_name);
/*
 * Cooling state <-> CPUFreq frequency
 *
 * Cooling states are translated to frequencies throughout this driver and this
 * is the relation between them.
 *
 * Highest cooling state corresponds to lowest possible frequency.
 *
 * i.e.
 *	level 0 --> 1st Max Freq
 *	level 1 --> 2nd Max Freq
 *	...
 */

/**
 * struct freq_table - frequency table along with power entries
 * @frequency:	frequency in KHz
 * @power:	power in mW
 *
 * This structure is built when the cooling device registers and helps
 * in translating frequency to power and vice versa.
 */
struct freq_table {
	u32 frequency;
	u32 power;
#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
	u32 volt_mv;
#endif
};

/**
 * struct time_in_idle - Idle time stats
 * @time: previous reading of the absolute time that this cpu was idle
 * @timestamp: wall time of the last invocation of get_cpu_idle_time_us()
 */
struct time_in_idle {
	u64 time;
	u64 timestamp;
};

static DEFINE_IDA(cpufreq_ida);
static DEFINE_MUTEX(cooling_list_lock);
static LIST_HEAD(cpufreq_cdev_list);

static BLOCKING_NOTIFIER_HEAD(cpu_notifier);

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)

static void
exynos_cpufreq_cdev_register(struct exynos_cpufreq_cooling_device *cpufreq_cdev)
{
	int id, i, j, size, var;
	struct freq_table *freq_table = cpufreq_cdev->freq_table;
	int num_cpus;
	struct thermal_instance *instance;

	id = cpufreq_cdev->id;

	exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_ID, 0,
			cpufreq_cdev->cdev->id);
	exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_TZ_ID, 0,
			cpufreq_cdev->tz->id);

	exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_MAX, 0,
			cpufreq_cdev->max_level);

	exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_POWER_ACTOR, 0,
			cpufreq_cdev->is_power_actor);

	if (cpufreq_cdev->is_power_actor) {
		for (i = 0; i <= cpufreq_cdev->max_level; i++) {
			exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_FREQ, i,
					freq_table[i].frequency);
			exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_POWER, i,
					freq_table[i].power);
			exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_VOLT, i,
					freq_table[i].volt_mv);
		}

		num_cpus = cpumask_weight(cpufreq_cdev->policy->related_cpus);
		exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_NUM_CPU, 0, num_cpus);

		exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_VAR_VOLT_SIZE, 0,
				cpufreq_cdev->var_volt_size);
		exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_VAR_TEMP_SIZE, 0,
				cpufreq_cdev->var_temp_size);

		size = cpufreq_cdev->var_temp_size;
		for (i = 0; i < cpufreq_cdev->var_volt_size; i++) {
			for (j = 0; j < cpufreq_cdev->var_temp_size; j++) {
				var = cpufreq_cdev->var_table[i * size + j];
				exynos_acpm_tmu_set_cpufreq_cdev(id, CPU_CDEV_VAR_TABLE,
						i * size + j, var);
			}
		}
	}

	size = cpufreq_cdev->tz->num_trips;
	for (i = 0; i < size; i++) {
		instance = get_thermal_instance(cpufreq_cdev->tz,
				cpufreq_cdev->cdev, i);
		if (instance)
			exynos_thermal_instance_update(instance);
	}
}
#endif

/* Below code defines functions to be used for cpufreq as cooling device */

/**
 ** get_level: Find the level for a particular frequency
 ** @cpufreq_cdev: cpufreq_cdev for which the property is required
 ** @freq: Frequency
 **
 ** Return: level corresponding to the frequency.
 **/
static unsigned long get_level(struct exynos_cpufreq_cooling_device *cpufreq_cdev,
		unsigned int freq)
{
	struct freq_table *freq_table = cpufreq_cdev->freq_table;
	unsigned long level;

	for (level = 1; level <= cpufreq_cdev->max_level; level++)
		if (freq > freq_table[level].frequency)
			break;

	return level - 1;
}

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
#else
/**
 ** cpufreq_cooling_get_level - for a given cpu, return the cooling level.
 ** @cpu: cpu for which the level is required
 ** @freq: the frequency of interest
 **
 ** This function will match the cooling level corresponding to the
 ** requested @freq and return it.
 **
 ** Return: The matched cooling level on success or THERMAL_CSTATE_INVALID
 ** otherwise.
 **/
unsigned long cpufreq_cooling_get_level(unsigned int cpu, unsigned int freq)
{
	struct exynos_cpufreq_cooling_device *cpufreq_cdev;

	mutex_lock(&cooling_list_lock);
	list_for_each_entry(cpufreq_cdev, &cpufreq_cdev_list, node) {
		if (cpumask_test_cpu(cpu, cpufreq_cdev->policy->related_cpus)) {
			unsigned long level = get_level(cpufreq_cdev, freq);

			mutex_unlock(&cooling_list_lock);
			return level;
		}
	}
	mutex_unlock(&cooling_list_lock);

	pr_err("%s: cpu:%d not part of any cooling device\n", __func__, cpu);
	return THERMAL_CSTATE_INVALID;
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_get_level);
#endif
/**
 * update_freq_table() - Update the freq table with power numbers
 * @cpufreq_cdev:	the cpufreq cooling device in which to update the table
 * @capacitance: dynamic power coefficient for these cpus
 *
 * Update the freq table with power numbers.  This table will be used in
 * cpu_power_to_freq() and cpu_freq_to_power() to convert between power and
 * frequency efficiently.  Power is stored in mW, frequency in KHz.  The
 * resulting table is in descending order.
 *
 * Return: 0 on success, -EINVAL if there are no OPPs for any CPUs,
 * or -ENOMEM if we run out of memory.
 */
static int update_freq_table(struct exynos_cpufreq_cooling_device *cpufreq_cdev,
			     u32 capacitance)
{
	struct freq_table *freq_table = cpufreq_cdev->freq_table;
	struct dev_pm_opp *opp;
	struct device *dev = NULL;
	int num_opps = 0, cpu = cpufreq_cdev->policy->cpu, i;

	dev = get_cpu_device(cpu);
	if (unlikely(!dev)) {
		dev_warn(&cpufreq_cdev->cdev->device,
			 "No cpu device for cpu %d\n", cpu);
		return -ENODEV;
	}

	num_opps = dev_pm_opp_get_opp_count(dev);
	if (num_opps < 0)
		return num_opps;

	/*
	 * The cpufreq table is also built from the OPP table and so the count
	 * should match.
	 */
	if (num_opps != cpufreq_cdev->max_level + 1) {
		dev_warn(dev, "Number of OPPs not matching with max_levels\n");
		return -EINVAL;
	}

	for (i = 0; i <= cpufreq_cdev->max_level; i++) {
		unsigned long freq = freq_table[i].frequency * 1000;
		u32 freq_mhz = freq_table[i].frequency / 1000;
		u64 power;
		u32 voltage_mv;

		/*
		 * Find ceil frequency as 'freq' may be slightly lower than OPP
		 * freq due to truncation while converting to kHz.
		 */
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp)) {
			dev_err(dev, "failed to get opp for %lu frequency\n",
				freq);
			return -EINVAL;
		}

		voltage_mv = dev_pm_opp_get_voltage(opp) / 1000;
		dev_pm_opp_put(opp);

		/*
		 * Do the multiplication with MHz and millivolt so as
		 * to not overflow.
		 */
		power = (u64)capacitance * freq_mhz * voltage_mv * voltage_mv;
		do_div(power, 1000000000);

		/* power is stored in mW */
		freq_table[i].power = power;
#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
		freq_table[i].volt_mv = voltage_mv;
#endif
	}

	return 0;
}

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
#else
static int lookup_static_power(struct exynos_cpufreq_cooling_device *cpufreq_cdev,
		unsigned long voltage, int temperature, u32 *power)
{
	int volt_index = 0, temp_index = 0;
	int index = 0;
	int num_cpus;
	int max_cpus;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	cpumask_t tempmask;

	cpumask_and(&tempmask, policy->related_cpus, cpu_online_mask);
	max_cpus = cpumask_weight(policy->related_cpus);
	num_cpus = cpumask_weight(&tempmask);
	voltage = voltage / 1000;
	temperature  = temperature / 1000;

	for (volt_index = 0; volt_index <= cpufreq_cdev->var_volt_size; volt_index++) {
		if (voltage < cpufreq_cdev->var_table[volt_index * ((int)cpufreq_cdev->var_temp_size + 1)]) {
			volt_index = volt_index - 1;
			break;
		}
	}

	if (volt_index == 0)
		volt_index = 1;

	if (volt_index > cpufreq_cdev->var_volt_size)
		volt_index = cpufreq_cdev->var_volt_size;

	for (temp_index = 0; temp_index <= cpufreq_cdev->var_temp_size; temp_index++) {
		if (temperature < cpufreq_cdev->var_table[temp_index]) {
			temp_index = temp_index - 1;
			break;
		}
	}

	if (temp_index == 0)
		temp_index = 1;

	if (temp_index > cpufreq_cdev->var_temp_size)
		temp_index = cpufreq_cdev->var_temp_size;

	index = (int)(volt_index * (cpufreq_cdev->var_temp_size + 1) + temp_index);
	*power = (unsigned int)cpufreq_cdev->var_table[index];

	return 0;
}

static u32 cpu_freq_to_power(struct exynos_cpufreq_cooling_device *cpufreq_cdev,
			     u32 freq)
{
	int i;
	struct freq_table *freq_table = cpufreq_cdev->freq_table;

	for (i = 1; i <= cpufreq_cdev->max_level; i++)
		if (freq > freq_table[i].frequency)
			break;

	return freq_table[i - 1].power;
}

static u32 cpu_power_to_freq(struct exynos_cpufreq_cooling_device *cpufreq_cdev,
			     u32 power)
{
	int i;
	struct freq_table *freq_table = cpufreq_cdev->freq_table;

	for (i = 0; i <= cpufreq_cdev->max_level; i++)
		if (power >= freq_table[i].power)
			break;

	i = i <= cpufreq_cdev->max_level ? i: cpufreq_cdev->max_level;
	return freq_table[i].frequency;
}

/**
 * get_static_power() - calculate the static power consumed by the cpus
 * @cpufreq_cdev:	struct &exynos_cpufreq_cooling_device for this cpu cdev
 * @tz:		thermal zone device in which we're operating
 * @freq:	frequency in KHz
 * @power:	pointer in which to store the calculated static power
 *
 * Calculate the static power consumed by the cpus described by
 * @cpu_actor running at frequency @freq.  This function relies on a
 * platform specific function that should have been provided when the
 * actor was registered.  If it wasn't, the static power is assumed to
 * be negligible.  The calculated static power is stored in @power.
 *
 * Return: 0 on success, -E* on failure.
 */
static int get_static_power(struct exynos_cpufreq_cooling_device *cpufreq_cdev,
			    struct thermal_zone_device *tz, unsigned long freq,
			    u32 *power)
{
	struct dev_pm_opp *opp;
	unsigned long voltage;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	unsigned long freq_hz = freq * 1000;
	struct device *dev;

	*power = 0;

	dev = get_cpu_device(policy->cpu);

	if (!dev)
		return 0;

	opp = dev_pm_opp_find_freq_exact(dev, freq_hz, true);
	if (IS_ERR(opp)) {
		return -EINVAL;
	}

	voltage = dev_pm_opp_get_voltage(opp);
	dev_pm_opp_put(opp);

	if (voltage == 0) {
		dev_err_ratelimited(dev, "Failed to get voltage for frequency %lu\n",
				    freq_hz);
		return -EINVAL;
	}

	lookup_static_power(cpufreq_cdev, voltage, tz->temperature, power);

	return 0;
}
#endif
/* cpufreq cooling device callback functions are defined below */

/**
 * cpufreq_get_max_state - callback function to get the max cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the max cooling state.
 *
 * Callback for the thermal cooling device to return the cpufreq
 * max cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct exynos_cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;

	*state = cpufreq_cdev->max_level;
	return 0;
}

/**
 * cpufreq_get_cur_state - callback function to get the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the current cooling state.
 *
 * Callback for the thermal cooling device to return the cpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
#if IS_ENABLED(CONFIG_SOC_S5E9945)
	exynos_acpm_tmu_cdev_get_cur_state(cdev->id, state);
#else
	struct exynos_cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	*state = cpufreq_cdev->cpufreq_state;
#endif
	return 0;
}

/**
 * cpufreq_set_cur_state - callback function to set the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: set this variable to the current cooling state.
 *
 * Callback for the thermal cooling device to change the cpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct exynos_cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	int ret = 0;

	/* Request state should be less than max_level */
	if (state > cpufreq_cdev->max_level)
		return -EINVAL;

	/* Check if the old cooling action is same as new cooling action */
	if (cpufreq_cdev->cpufreq_state == state)
		return 0;

	cpufreq_cdev->cpufreq_state = state;

	dbg_snapshot_thermal(NULL, state, cpufreq_cdev->cdev->type,
			cpufreq_cdev->freq_table[state].frequency);
	ret = freq_qos_update_request(&cpufreq_cdev->qos_req,
			cpufreq_cdev->freq_table[state].frequency);

	if (ret == 1)
		ret = 0;
	return ret;
}

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
#else
/**
 * cpufreq_state2power() - convert a cpu cdev state to power consumed
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @state:	cooling device state to be converted
 * @power:	pointer in which to store the resulting power
 *
 * Convert cooling device state @state into power consumption in
 * milliwatts assuming 100% load.  Store the calculated power in
 * @power.
 *
 * Return: 0 on success, -EINVAL if the cooling device state could not
 * be converted into a frequency or other -E* if there was an error
 * when calculating the static power.
 */
static int cpufreq_state2power(struct thermal_cooling_device *cdev,
			       unsigned long state, u32 *power)
{
	unsigned int freq, num_cpus;
	u32 static_power, dynamic_power;
	int ret;
	struct exynos_cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	struct thermal_zone_device *tz = cpufreq_cdev->tz;

	/* Request state should be less than max_level */
	if (state > cpufreq_cdev->max_level)
		return -EINVAL;

	num_cpus = cpumask_weight(cpufreq_cdev->policy->related_cpus);

	freq = cpufreq_cdev->freq_table[state].frequency;
	dynamic_power = cpu_freq_to_power(cpufreq_cdev, freq) * num_cpus;
	ret = get_static_power(cpufreq_cdev, tz, freq, &static_power);
	if (ret)
		return ret;

	*power = static_power + dynamic_power;
	return ret;
}

/**
 * cpufreq_power2state() - convert power to a cooling device state
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @power:	power in milliwatts to be converted
 * @state:	pointer in which to store the resulting state
 *
 * Calculate a cooling device state for the cpus described by @cdev
 * that would allow them to consume at most @power mW and store it in
 * @state.  Note that this calculation depends on external factors
 * such as the cpu load or the current static power.  Calling this
 * function with the same power as input can yield different cooling
 * device states depending on those external factors.
 *
 * Return: 0 on success, -ENODEV if no cpus are online or -EINVAL if
 * the calculated frequency could not be converted to a valid state.
 * The latter should not happen unless the frequencies available to
 * cpufreq have changed since the initialization of the cpu cooling
 * device.
 */
static int cpufreq_power2state(struct thermal_cooling_device *cdev,
		u32 power, unsigned long *state)
{
	unsigned int cpu, cur_freq, target_freq;
	int ret;
	s32 dyn_power;
	u32 normalised_power, static_power;
	struct exynos_cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	int num_cpus;
	struct thermal_zone_device *tz = cpufreq_cdev->tz;

	num_cpus = cpumask_weight(policy->related_cpus);
	cpu = cpumask_first(policy->related_cpus);

	/* None of our cpus are online */
	if (cpu >= nr_cpu_ids || num_cpus == 0)
		return -ENODEV;

	cur_freq = cpufreq_quick_get(policy->cpu);
	ret = get_static_power(cpufreq_cdev, tz, cur_freq, &static_power);
	if (ret)
		return ret;

	dyn_power = power - static_power;
	dyn_power = dyn_power > 0 ? dyn_power : 0;
	normalised_power = dyn_power / num_cpus;
	target_freq = cpu_power_to_freq(cpufreq_cdev, normalised_power);

	*state = cpufreq_cooling_get_level(cpu, target_freq);
	if (*state == THERMAL_CSTATE_INVALID) {
		dev_warn_ratelimited(&cdev->device,
				     "Failed to convert %dKHz for cpu %d into a cdev state\n",
				     target_freq, cpu);
		return -EINVAL;
	}

	*state = get_level(cpufreq_cdev, target_freq);
	trace_thermal_exynos_power_cpu_limit(tz->id, policy->cpu, target_freq, *state,
				      power);
	return 0;
}
#endif
/* Bind cpufreq callbacks to thermal cooling device ops */

static struct thermal_cooling_device_ops cpufreq_cooling_ops = {
	.get_max_state = cpufreq_get_max_state,
	.get_cur_state = cpufreq_get_cur_state,
	.set_cur_state = cpufreq_set_cur_state,
};

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
#else
static struct thermal_cooling_device_ops cpufreq_power_cooling_ops = {
	.get_max_state		= cpufreq_get_max_state,
	.get_cur_state		= cpufreq_get_cur_state,
	.set_cur_state		= cpufreq_set_cur_state,
	.state2power		= cpufreq_state2power,
	.power2state		= cpufreq_power2state,
};
#endif
static unsigned int find_next_max(struct cpufreq_frequency_table *table,
				  unsigned int prev_max)
{
	struct cpufreq_frequency_table *pos;
	unsigned int max = 0;

	cpufreq_for_each_valid_entry(pos, table) {
		if (pos->frequency > max && pos->frequency < prev_max)
			max = pos->frequency;
	}

	return max;
}

static struct thermal_zone_device* parse_ect_cooling_level(struct thermal_cooling_device *cdev,
				   char *tz_name)
{
	struct thermal_instance *instance;
	struct thermal_zone_device *tz = NULL;
	bool foundtz = false;
	void *thermal_block;
	struct ect_ap_thermal_function *function;
	int i, temperature;
	unsigned int freq;

	mutex_lock(&cdev->lock);
	list_for_each_entry(instance, &cdev->thermal_instances, cdev_node) {
		tz = instance->tz;
		if (!strncasecmp(tz_name, tz->type, THERMAL_NAME_LENGTH)) {
			foundtz = true;
			break;
		}
	}
	mutex_unlock(&cdev->lock);

	if (!foundtz)
		goto skip_ect;

	thermal_block = ect_get_block(BLOCK_AP_THERMAL);
	if (!thermal_block)
		goto skip_ect;

	function = ect_ap_thermal_get_function(thermal_block, tz_name);
	if (!function)
		goto skip_ect;

	for (i = 0; i < function->num_of_range; ++i) {
		struct exynos_cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
		unsigned long max_level = 0;
		int level;

		temperature = function->range_list[i].lower_bound_temperature;
		freq = function->range_list[i].max_frequency;

		instance = get_thermal_instance(tz, cdev, i);
		if (!instance) {
			pr_err("%s: (%s, %d)instance isn't valid\n", __func__, tz_name, i);
			goto skip_ect;
		}

		cdev->ops->get_max_state(cdev, &max_level);
		level = get_level(cpufreq_cdev, freq);

		if (level == THERMAL_CSTATE_INVALID)
			level = max_level;

		instance->upper = level;

		pr_info("Parsed From ECT : %s: [%d] Temperature : %d, frequency : %u, level: %d\n",
			tz_name, i, temperature, freq, level);
	}
skip_ect:
	return tz;
}

/**
 * __cpufreq_cooling_register - helper function to create cpufreq cooling device
 * @np: a valid struct device_node to the cooling device device tree node
 * @policy: cpufreq policy
 * Normally this should be same as cpufreq policy->related_cpus.
 * @capacitance: dynamic power coefficient for these cpus
 * @plat_static_func: function to calculate the static power consumed by these
 *                    cpus (optional)
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices. It also gives the opportunity to link the cooling device
 * with a device tree node, in order to bind it via the thermal DT code.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
static struct thermal_cooling_device *
__cpufreq_cooling_register(struct device_node *np,
			struct cpufreq_policy *policy, u32 capacitance,
			get_static_t plat_static_func, char *tz_name)
{
	struct thermal_cooling_device *cdev;
	struct exynos_cpufreq_cooling_device *cpufreq_cdev;
	char dev_name[THERMAL_NAME_LENGTH];
	unsigned int freq, i, num_cpus;
	int ret;
	struct thermal_cooling_device_ops *cooling_ops;

	if (IS_ERR_OR_NULL(policy)) {
		pr_err("%s: cpufreq policy isn't valid: %p\n", __func__, policy);
		return ERR_PTR(-EINVAL);
	}

	i = cpufreq_table_count_valid_entries(policy);
	if (!i) {
		pr_debug("%s: CPUFreq table not found or has no valid entries\n",
			 __func__);
		return ERR_PTR(-ENODEV);
	}

	cpufreq_cdev = kzalloc(sizeof(*cpufreq_cdev), GFP_KERNEL);
	if (!cpufreq_cdev)
		return ERR_PTR(-ENOMEM);

	cpufreq_cdev->policy = policy;
	num_cpus = cpumask_weight(policy->related_cpus);
	cpufreq_cdev->idle_time = kcalloc(num_cpus,
					 sizeof(*cpufreq_cdev->idle_time),
					 GFP_KERNEL);
	if (!cpufreq_cdev->idle_time) {
		cdev = ERR_PTR(-ENOMEM);
		goto free_cdev;
	}

	/* max_level is an index, not a counter */
	cpufreq_cdev->max_level = i - 1;

	cpufreq_cdev->freq_table = kmalloc_array(i,
					sizeof(*cpufreq_cdev->freq_table),
					GFP_KERNEL);
	if (!cpufreq_cdev->freq_table) {
		cdev = ERR_PTR(-ENOMEM);
		goto free_idle_time;
	}

	ret = ida_simple_get(&cpufreq_ida, 0, 0, GFP_KERNEL);
	if (ret < 0) {
		cdev = ERR_PTR(ret);
		goto free_table;
	}
	cpufreq_cdev->id = ret;

	snprintf(dev_name, sizeof(dev_name), "thermal-cpufreq-%d",
		 cpufreq_cdev->id);

	/* Fill freq-table in descending order of frequencies */
	for (i = 0, freq = -1; i <= cpufreq_cdev->max_level; i++) {
		freq = find_next_max(policy->freq_table, freq);
		cpufreq_cdev->freq_table[i].frequency = freq;

		/* Warn for duplicate entries */
		if (!freq)
			pr_warn("%s: table has duplicate entries\n", __func__);
		else
			pr_debug("%s: freq:%u KHz\n", __func__, freq);
	}

	if (capacitance) {
		ret = update_freq_table(cpufreq_cdev, capacitance);
		if (ret) {
			cdev = ERR_PTR(ret);
			goto remove_ida;
		}

		ret = exynos_build_static_power_table(np, &cpufreq_cdev->var_table, &cpufreq_cdev->var_volt_size,
				&cpufreq_cdev->var_temp_size, tz_name);
		if (ret) {
			cdev = ERR_PTR(ret);
			goto remove_ida;
		}

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
		cooling_ops = &cpufreq_cooling_ops;
		cpufreq_cdev->is_power_actor = true;
#else
		cooling_ops = &cpufreq_power_cooling_ops;
		cpufreq_cdev->is_power_actor = true;
#endif
	} else {
#if IS_ENABLED(CONFIG_EXYNOS_EA_DTM)
		struct device *dev = NULL;
		dev = get_cpu_device(cpufreq_cdev->policy->cpu);
		for (i = 0; i <= cpufreq_cdev->max_level; i++) {
			struct freq_table *freq_table =
				&cpufreq_cdev->freq_table[i];
			unsigned long freq = freq_table->frequency * 1000;
			u32 voltage_mv;
			struct dev_pm_opp *opp;
			opp = dev_pm_opp_find_freq_ceil(dev, &freq);
			if (IS_ERR(opp))
				continue;
			voltage_mv = dev_pm_opp_get_voltage(opp) / 1000;
			dev_pm_opp_put(opp);
			freq_table->volt_mv = voltage_mv;
		}
		cooling_ops = &cpufreq_cooling_ops;
		cpufreq_cdev->is_power_actor = true;
#else
		cooling_ops = &cpufreq_cooling_ops;
		cpufreq_cdev->is_power_actor = false;
#endif
	}

	ret = freq_qos_tracer_add_request(&policy->constraints,
				   &cpufreq_cdev->qos_req, FREQ_QOS_MAX,
				   cpufreq_cdev->freq_table[0].frequency);
	if (ret < 0) {
		pr_err("%s: Failed to add freq constraint (%d)\n", __func__,
		       ret);
		cdev = ERR_PTR(ret);
		goto remove_ida;
	}

	cpufreq_cdev->tz = thermal_zone_get_zone_by_name(tz_name);

	cdev = thermal_of_cooling_device_register(np, dev_name, cpufreq_cdev,
						  cooling_ops);
	if (IS_ERR(cdev))
		goto remove_qos_req;

	cpufreq_cdev->tz = parse_ect_cooling_level(cdev, tz_name);

	cpufreq_cdev->clipped_freq = cpufreq_cdev->freq_table[0].frequency;
	cpufreq_cdev->cdev = cdev;

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
	exynos_cpufreq_cdev_register(cpufreq_cdev);
#endif

	mutex_lock(&cooling_list_lock);
	list_add(&cpufreq_cdev->node, &cpufreq_cdev_list);
	mutex_unlock(&cooling_list_lock);

	return cdev;

remove_qos_req:
	freq_qos_tracer_remove_request(&cpufreq_cdev->qos_req);
remove_ida:
	ida_simple_remove(&cpufreq_ida, cpufreq_cdev->id);
free_table:
	kfree(cpufreq_cdev->freq_table);
free_idle_time:
	kfree(cpufreq_cdev->idle_time);
free_cdev:
	kfree(cpufreq_cdev);
	return cdev;
}

#if IS_ENABLED(CONFIG_SOC_S5E9955)
extern u32 get_invalid_sample(void);
#endif

struct thermal_cooling_device *
exynos_cpufreq_cooling_register(struct device_node *np, struct cpufreq_policy *policy)
{
	u32 index = 0;
	void *gen_block;
	struct ect_gen_param_table *pwr_coeff;
	u32 capacitance = 0;
	const char *name;
	char cooling_name[THERMAL_NAME_LENGTH];
	struct device_node *cpu_np = of_get_cpu_node(policy->cpu, NULL);

#if IS_ENABLED(CONFIG_SOC_S5E9955)
	if (get_invalid_sample()) {
		pr_err("Cannot init cpu_cooling (thermal invalid sample)");
		return ERR_PTR(-ENODEV);
	}
#endif

	if (!np)
		return ERR_PTR(-EINVAL);

	if (!cpu_np) {
		pr_err("cpu_cooling: OF node not available for cpu%d\n",
		       policy->cpu);
		of_node_put(cpu_np);
		return ERR_PTR(-EINVAL);
	}
	of_property_read_u32(cpu_np, "dynamic-power-coefficient", &capacitance);
	of_node_put(cpu_np);

	if (of_property_read_string(np, "tz-cooling-name", &name)) {
		pr_err("%s: could not find tz-cooling-name\n", __func__);
		return ERR_PTR(-EINVAL);
	}
	strncpy(cooling_name, name, sizeof(cooling_name));

	if (of_property_read_bool(np, "use-em-coeff"))
		goto regist;

	if (of_property_read_u32(np, "ect-coeff-index", &index)) {
		pr_err("%s: could not find ect-coeff-index\n", __func__);
		goto regist;
	}

	gen_block = ect_get_block("GEN");
	if (gen_block == NULL) {
		pr_err("%s: Failed to get gen block from ECT\n", __func__);
		goto regist;
	}
	pwr_coeff = ect_gen_param_get_table(gen_block, "DTM_PWR_Coeff");
	if (pwr_coeff == NULL) {
		pr_err("%s: Failed to get power coeff from ECT\n", __func__);
		goto regist;
	}
	capacitance = pwr_coeff->parameter[index];

regist:
	return __cpufreq_cooling_register(np, policy, capacitance,
				NULL, cooling_name);
}
EXPORT_SYMBOL_GPL(exynos_cpufreq_cooling_register);
