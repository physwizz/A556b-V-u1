/*
 *  Copyright (C) 2012	Samsung Electronics Co., Ltd(http://www.samsung.com)
 *  Copyright (C) 2012  Amit Daniel <amit.kachhap@linaro.org>
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
#include <linux/module.h>
#include <linux/thermal.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <soc/samsung/gpu_cooling.h>
#include <soc/samsung/tmu.h>

#define CREATE_TRACE_POINTS
#include <trace/events/thermal_exynos_gpu.h>

#include <soc/samsung/exynos/debug-snapshot.h>

#include <soc/samsung/ect_parser.h>
#include "exynos_tmu.h"
#include "../../../../../common/drivers/thermal/thermal_core.h"
#include "exynos_acpm_tmu.h"

/**
 * struct power_table - frequency to power conversion
 * @frequency:	frequency in KHz
 * @power:	power in mW
 *
 * This structure is built when the cooling device registers and helps
 * in translating frequency to power and viceversa.
 */
struct power_table {
	u32 frequency;
	u32 power;
#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
	u32 volt_mv;
#endif
};

/**
 * struct gpufreq_cooling_device - data for cooling device with gpufreq
 * @id: unique integer value corresponding to each gpufreq_cooling_device
 *	registered.
 * @cool_dev: thermal_cooling_device pointer to keep track of the
 *	registered cooling device.
 * @gpufreq_state: integer value representing the current state of gpufreq
 *	cooling	devices.
 * @gpufreq_val: integer value representing the absolute value of the clipped
 *	frequency.
 *
 * This structure is required for keeping information of each
 * gpufreq_cooling_device registered. In order to prevent corruption of this a
 * mutex lock cooling_gpu_lock is used.
 */
struct gpufreq_cooling_device {
	int id;
	struct thermal_cooling_device *cool_dev;
	unsigned long gpufreq_state;
	unsigned int gpufreq_val;
	u32 last_load;
	struct power_table *dyn_power_table;
	int dyn_power_table_entries;
	int *var_table;
	unsigned int var_volt_size;
	unsigned int var_temp_size;
	struct thermal_zone_device *tz;
	bool is_power_actor;
};

static DEFINE_IDR(gpufreq_idr);
static DEFINE_MUTEX(cooling_gpu_lock);

static unsigned int gpufreq_cdev_count;

static const struct gpu_dvfs_fn *gpu_dvfs_fn;

struct cpufreq_frequency_table *gpu_freq_table;

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
static void
exynos_gpufreq_cdev_register(struct gpufreq_cooling_device *gpufreq_cdev)
{
	int id, i, j, size, var;
	int num_level;
	struct thermal_instance *instance;
	unsigned long freq;
	struct power_table *power_table = gpufreq_cdev->dyn_power_table;

	id = gpufreq_cdev->id;

	exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_ID, 0,
			gpufreq_cdev->cool_dev->id);
	exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_TZ_ID, 0,
			gpufreq_cdev->tz->id);

	num_level = gpu_dvfs_fn->get_num_lv();
	if (num_level == 0) {
		pr_err("Faile to get gpu num_lv()\n");
		return;
	}
	exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_NUM_LEVEL, 0, num_level);

	for (i = 0; i < num_level; i++) {
		freq = gpu_freq_table[i].frequency;
		exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_FREQ, i, freq);
	}

	exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_POWER_ACTOR, 0,
			gpufreq_cdev->is_power_actor);

	if (gpufreq_cdev->is_power_actor) {
		exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_DYN_PWRTABLE_ENTRIES, 0,
				gpufreq_cdev->dyn_power_table_entries);
		for (i = 0; i < gpufreq_cdev->dyn_power_table_entries; i++) {
			exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_DYN_FREQ, i,
					power_table[i].frequency);
			exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_DYN_VOLT, i,
					power_table[i].volt_mv);
			exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_DYN_POWER, i,
					power_table[i].power);
		}

		exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_VAR_VOLT_SIZE, 0,
				gpufreq_cdev->var_volt_size);
		exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_VAR_TEMP_SIZE, 0,
				gpufreq_cdev->var_temp_size);

		size = gpufreq_cdev->var_temp_size;
		for (i = 0; i < gpufreq_cdev->var_volt_size; i++) {
			for (j = 0; j < gpufreq_cdev->var_temp_size; j++) {
				var = gpufreq_cdev->var_table[i * size + j];
				exynos_acpm_tmu_set_gpufreq_cdev(id, GPU_CDEV_VAR_TABLE,
						i * size + j, var);
			}
		}
	}

	size = gpufreq_cdev->tz->num_trips;
	for (i = 0; i < size; i++) {
		instance = get_thermal_instance(gpufreq_cdev->tz,
				gpufreq_cdev->cool_dev, i);
		if (instance)
			exynos_thermal_instance_update(instance);
	}
}
#endif

/**
 * get_idr - function to get a unique id.
 * @idr: struct idr * handle used to create a id.
 * @id: int * value generated by this function.
 *
 * This function will populate @id with an unique
 * id, using the idr API.
 *
 * Return: 0 on success, an error code on failure.
 */
static int get_idr(struct idr *idr, int *id)
{
	int ret;

	mutex_lock(&cooling_gpu_lock);
	ret = idr_alloc(idr, NULL, 0, 0, GFP_KERNEL);
	mutex_unlock(&cooling_gpu_lock);
	if (unlikely(ret < 0))
		return ret;
	*id = ret;

	return 0;
}

/* Below code defines functions to be used for gpufreq as cooling device */

enum gpufreq_cooling_property {
	GET_LEVEL,
	GET_FREQ,
	GET_MAXL,
};

/**
 * get_property - fetch a property of interest for a give gpu.
 * @gpu: gpu for which the property is required
 * @input: query parameter
 * @output: query return
 * @property: type of query (frequency, level, max level)
 *
 * This is the common function to
 * 1. get maximum gpu cooling states
 * 2. translate frequency to cooling state
 * 3. translate cooling state to frequency
 * Note that the code may be not in good shape
 * but it is written in this way in order to:
 * a) reduce duplicate code as most of the code can be shared.
 * b) make sure the logic is consistent when translating between
 *    cooling states and frequencies.
 *
 * Return: 0 on success, -EINVAL when invalid parameters are passed.
 */
static int get_property(unsigned int gpu, unsigned long input,
			unsigned int *output,
			enum gpufreq_cooling_property property)
{
	int i;
	unsigned long max_level = 0, level = 0;
	unsigned int freq = CPUFREQ_ENTRY_INVALID;
	int descend = -1;
	struct cpufreq_frequency_table *pos, *table =
					gpu_freq_table;

	if (!output)
		return -EINVAL;

	cpufreq_for_each_valid_entry(pos, table) {
		/* ignore duplicate entry */
		if (freq == pos->frequency)
			continue;

		/* get the frequency order */
		if (freq != CPUFREQ_ENTRY_INVALID && descend == -1)
			descend = freq > pos->frequency;

		freq = pos->frequency;
		max_level++;
	}

	/* No valid cpu frequency entry */
	if (max_level == 0)
		return -EINVAL;

	/* max_level is an index, not a counter */
	max_level--;

	/* get max level */
	if (property == GET_MAXL) {
		*output = (unsigned int)max_level;
		return 0;
	}

	if (property == GET_FREQ)
		level = descend ? input : (max_level - input);

	i = 0;
	cpufreq_for_each_valid_entry(pos, table) {
		/* ignore duplicate entry */
		if (freq == pos->frequency)
			continue;

		/* now we have a valid frequency entry */
		freq = pos->frequency;

		if (property == GET_LEVEL && (unsigned int)input == freq) {
			/* get level by frequency */
			*output = (unsigned int)(descend ? i : (max_level - i));
			return 0;
		}
		if (property == GET_FREQ && level == i) {
			/* get frequency by level */
			*output = freq;
			return 0;
		}
		i++;
	}

	return -EINVAL;
}

/**
 * gpufreq_cooling_get_level - for a give gpu, return the cooling level.
 * @gpu: gpu for which the level is required
 * @freq: the frequency of interest
 *
 * This function will match the cooling level corresponding to the
 * requested @freq and return it.
 *
 * Return: The matched cooling level on success or THERMAL_CSTATE_INVALID
 * otherwise.
 */
unsigned long gpufreq_cooling_get_level(unsigned int gpu, unsigned int freq)
{
	unsigned int val;

	if (get_property(gpu, (unsigned long)freq, &val, GET_LEVEL))
		return THERMAL_CSTATE_INVALID;

	return (unsigned long)val;
}
EXPORT_SYMBOL_GPL(gpufreq_cooling_get_level);

/**
 * gpufreq_cooling_get_freq - for a give gpu, return the cooling frequency.
 * @gpu: gpu for which the level is required
 * @level: the level of interest
 *
 * This function will match the cooling level corresponding to the
 * requested @freq and return it.
 *
 * Return: The matched cooling level on success or THERMAL_CFREQ_INVALID
 * otherwise.
 */
int gpufreq_cooling_get_freq(unsigned int gpu, unsigned long level)
{
	unsigned int val = 0;

	if (get_property(gpu, level, &val, GET_FREQ))
		return THERMAL_CFREQ_INVALID;

	return (int)val;
}
EXPORT_SYMBOL_GPL(gpufreq_cooling_get_freq);

/**
 * build_dyn_power_table() - create a dynamic power to frequency table
 * @gpufreq_cdev:	the gpufreq cooling device in which to store the table
 * @capacitance: dynamic power coefficient for these gpus
 *
 * Build a dynamic power to frequency table for this gpu and store it
 * in @gpufreq_cdev.  This table will be used in gpu_power_to_freq() and
 * gpu_freq_to_power() to convert between power and frequency
 * efficiently.  Power is stored in mW, frequency in KHz.  The
 * resulting table is in ascending order.
 *
 * Return: 0 on success, -EINVAL if there are no OPPs for any CPUs,
 * -ENOMEM if we run out of memory or -EAGAIN if an OPP was
 * added/enabled while the function was executing.
 */
static int build_dyn_power_table(struct gpufreq_cooling_device *gpufreq_cdev,
				 u32 capacitance)
{
	struct power_table *power_table;
	int num_opps = 0, i, cnt = 0;
	unsigned long freq, max_freq;

	num_opps = gpu_dvfs_fn->get_num_lv();

	if (num_opps == 0)
		return -EINVAL;

	max_freq = gpu_dvfs_fn->get_max_freq();

	power_table = kcalloc(num_opps, sizeof(*power_table), GFP_KERNEL);
	if (!power_table)
		return -ENOMEM;

	for (freq = 0, i = 0; i < num_opps; i++) {
		u32 voltage_mv;
		u64 power;

		freq = gpu_dvfs_fn->get_freq(num_opps - i - 1);

		if (freq > max_freq || freq == 0)
			continue;

		voltage_mv = gpu_dvfs_fn->get_volt(freq) / 1000;

		/*
		 * Do the multiplication with MHz and millivolt so as
		 * to not overflow.
		 */
		power = (u64)capacitance * (freq / 1000) * voltage_mv * voltage_mv;
		do_div(power, 1000000000);

		power_table[i].frequency = (unsigned int)freq;

		/* power is stored in mW */
		power_table[i].power = power;
#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
		power_table[i].volt_mv = voltage_mv;
#endif
		cnt++;
	}

	gpufreq_cdev->dyn_power_table = power_table;
	gpufreq_cdev->dyn_power_table_entries = cnt;

	return 0;
}

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
#else
static int lookup_static_power(struct gpufreq_cooling_device *gpufreq_cdev,
		unsigned long voltage, int temperature, u32 *power)
{
	int volt_index = 0, temp_index = 0;
	int index = 0;

	voltage = voltage / 1000;
	temperature  = temperature / 1000;

	for (volt_index = 0; volt_index <= gpufreq_cdev->var_volt_size; volt_index++) {
		if (voltage < gpufreq_cdev->var_table[volt_index * ((int)gpufreq_cdev->var_temp_size + 1)]) {
			volt_index = volt_index - 1;
			break;
		}
	}

	if (volt_index == 0)
		volt_index = 1;

	if (volt_index > gpufreq_cdev->var_volt_size)
		volt_index = gpufreq_cdev->var_volt_size;

	for (temp_index = 0; temp_index <= gpufreq_cdev->var_temp_size; temp_index++) {
		if (temperature < gpufreq_cdev->var_table[temp_index]) {
			temp_index = temp_index - 1;
			break;
		}
	}

	if (temp_index == 0)
		temp_index = 1;

	if (temp_index > gpufreq_cdev->var_temp_size)
		temp_index = gpufreq_cdev->var_temp_size;

	index = (int)(volt_index * (gpufreq_cdev->var_temp_size + 1) + temp_index);
	*power = (unsigned int)gpufreq_cdev->var_table[index];

	return 0;
}

static u32 gpu_freq_to_power(struct gpufreq_cooling_device *gpufreq_cdev,
			     u32 freq)
{
	int i;
	struct power_table *pt = gpufreq_cdev->dyn_power_table;

	for (i = 1; i < gpufreq_cdev->dyn_power_table_entries; i++)
		if (freq < pt[i].frequency)
			break;

	return pt[i - 1].power;
}

static u32 gpu_power_to_freq(struct gpufreq_cooling_device *gpufreq_cdev,
			     u32 power)
{
	int i;
	struct power_table *pt = gpufreq_cdev->dyn_power_table;

	for (i = 1; i < gpufreq_cdev->dyn_power_table_entries; i++)
		if (power < pt[i].power)
			break;

	return pt[i - 1].frequency;
}

/**
 * get_static_power() - calculate the static power consumed by the gpus
 * @gpufreq_cdev:	struct &gpufreq_cooling_device for this gpu cdev
 * @tz:		thermal zone device in which we're operating
 * @freq:	frequency in KHz
 * @power:	pointer in which to store the calculated static power
 *
 * Calculate the static power consumed by the gpus described by
 * @gpu_actor running at frequency @freq.  This function relies on a
 * platform specific function that should have been provided when the
 * actor was registered.  If it wasn't, the static power is assumed to
 * be negligible.  The calculated static power is stored in @power.
 *
 * Return: 0 on success, -E* on failure.
 */
static int get_static_power(struct gpufreq_cooling_device *gpufreq_cdev,
			    struct thermal_zone_device *tz, unsigned long freq,
			    u32 *power)
{
	unsigned long voltage;

	if (!freq) {
		*power = 0;
		return 0;
	}

	voltage = gpu_dvfs_fn->get_volt(freq);

	if (voltage == 0) {
		pr_warn("Failed to get voltage for frequency %lu\n", freq);
		return -EINVAL;
	}

	return lookup_static_power(gpufreq_cdev, voltage, tz->temperature, power);
}

/**
 * get_dynamic_power() - calculate the dynamic power
 * @gpufreq_cdev:	&gpufreq_cooling_device for this cdev
 * @freq:	current frequency
 *
 * Return: the dynamic power consumed by the gpus described by
 * @gpufreq_cdev.
 */
static u32 get_dynamic_power(struct gpufreq_cooling_device *gpufreq_cdev,
			     unsigned long freq)
{
	u32 raw_gpu_power;

	raw_gpu_power = gpu_freq_to_power(gpufreq_cdev, freq);
	return (raw_gpu_power * gpufreq_cdev->last_load) / 100;
}
#endif

/* gpufreq cooling device callback functions are defined below */

/**
 * gpufreq_get_max_state - callback function to get the max cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the max cooling state.
 *
 * Callback for the thermal cooling device to return the gpufreq
 * max cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int gpufreq_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	unsigned int count = 0;
	int ret;

	ret = get_property(0, 0, &count, GET_MAXL);

	if (count > 0)
		*state = count;

	return ret;
}

/**
 * gpufreq_get_cur_state - callback function to get the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the current cooling state.
 *
 * Callback for the thermal cooling device to return the gpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int gpufreq_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
#if IS_ENABLED(CONFIG_SOC_S5E9945)
	exynos_acpm_tmu_cdev_get_cur_state(cdev->id, state);
#else
	struct gpufreq_cooling_device *gpufreq_cdev = cdev->devdata;
	*state = gpufreq_cdev->gpufreq_state;
#endif
	return 0;
}

/**
 * gpufreq_set_cur_state - callback function to set the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: set this variable to the current cooling state.
 *
 * Callback for the thermal cooling device to change the gpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int gpufreq_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct gpufreq_cooling_device *gpufreq_cdev = cdev->devdata;

	/* Check if the old cooling action is same as new cooling action */
	if (gpufreq_cdev->gpufreq_state == state)
		return 0;

	gpufreq_cdev->gpufreq_state = state;
	gpufreq_cdev->gpufreq_val = gpufreq_cooling_get_freq(0, state);
	dbg_snapshot_thermal(NULL, state, cdev->type, gpufreq_cdev->gpufreq_val);

	if (gpufreq_cdev->gpufreq_val == THERMAL_CFREQ_INVALID) {
		pr_warn("Failed to convert %lu gpu_level\n",
				     gpufreq_cdev->gpufreq_state);
		return -EINVAL;
	}

	gpu_dvfs_fn->set_maxlock(gpufreq_cdev->gpufreq_val);

	return 0;
}

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
#else
/**
 * gpufreq_get_requested_power() - get the current power
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @power:	pointer in which to store the resulting power
 *
 * Calculate the current power consumption of the gpus in milliwatts
 * and store it in @power.  This function should actually calculate
 * the requested power, but it's hard to get the frequency that
 * gpufreq would have assigned if there were no thermal limits.
 * Instead, we calculate the current power on the assumption that the
 * immediate future will look like the immediate past.
 *
 * We use the current frequency and the average load since this
 * function was last called.  In reality, there could have been
 * multiple opps since this function was last called and that affects
 * the load calculation.  While it's not perfectly accurate, this
 * simplification is good enough and works.  REVISIT this, as more
 * complex code may be needed if experiments show that it's not
 * accurate enough.
 *
 * Return: 0 on success, -E* if getting the static power failed.
 */
static int gpufreq_get_requested_power(struct thermal_cooling_device *cdev,
				       u32 *power)
{
	unsigned long freq;
	int ret = 0;
	u32 static_power, dynamic_power;
	struct gpufreq_cooling_device *gpufreq_cdev = cdev->devdata;
	u32 load_gpu = 0;
	struct thermal_zone_device *tz = gpufreq_cdev->tz;

	freq = gpu_dvfs_fn->get_freq();

	load_gpu = gpu_dvfs_fn->get_utilization();;

	gpufreq_cdev->last_load = load_gpu;

	dynamic_power = get_dynamic_power(gpufreq_cdev, freq);
	ret = get_static_power(gpufreq_cdev, tz, freq, &static_power);

	if (ret)
		return ret;

	if (trace_thermal_exynos_power_gpu_get_power_enabled()) {
		trace_thermal_exynos_power_gpu_get_power(
			freq, load_gpu, dynamic_power, static_power);
	}

	*power = static_power + dynamic_power;
	return 0;
}

/**
 * gpufreq_state2power() - convert a gpu cdev state to power consumed
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
static int gpufreq_state2power(struct thermal_cooling_device *cdev,
			       unsigned long state, u32 *power)
{
	unsigned int freq;
	u32 static_power, dynamic_power;
	int ret;
	struct gpufreq_cooling_device *gpufreq_cdev = cdev->devdata;
	struct thermal_zone_device *tz = gpufreq_cdev->tz;

	freq = gpu_freq_table[state].frequency;
	if (!freq)
		return -EINVAL;

	dynamic_power = gpu_freq_to_power(gpufreq_cdev, freq);
	ret = get_static_power(gpufreq_cdev, tz, freq, &static_power);
	if (ret)
		return ret;

	*power = static_power + dynamic_power;
	return 0;
}

/**
 * gpufreq_power2state() - convert power to a cooling device state
 * @cdev:	&thermal_cooling_device pointer
 * @tz:		a valid thermal zone device pointer
 * @power:	power in milliwatts to be converted
 * @state:	pointer in which to store the resulting state
 *
 * Calculate a cooling device state for the gpus described by @cdev
 * that would allow them to consume at most @power mW and store it in
 * @state.  Note that this calculation depends on external factors
 * such as the gpu load or the current static power.  Calling this
 * function with the same power as input can yield different cooling
 * device states depending on those external factors.
 *
 * Return: 0 on success, -ENODEV if no gpus are online or -EINVAL if
 * the calculated frequency could not be converted to a valid state.
 * The latter should not happen unless the frequencies available to
 * gpufreq have changed since the initialization of the gpu cooling
 * device.
 */
static int gpufreq_power2state(struct thermal_cooling_device *cdev,
			       u32 power, unsigned long *state)
{
	unsigned int cur_freq, target_freq;
	int ret;
	s32 dyn_power;
	u32 static_power;
	struct gpufreq_cooling_device *gpufreq_cdev = cdev->devdata;
	struct thermal_zone_device *tz = gpufreq_cdev->tz;

	cur_freq = gpu_dvfs_fn->get_cur_freq();
	ret = get_static_power(gpufreq_cdev, tz, cur_freq, &static_power);
	if (ret)
		return ret;

	dyn_power = power - static_power;
	dyn_power = dyn_power > 0 ? dyn_power : 0;
	target_freq = gpu_power_to_freq(gpufreq_cdev, dyn_power);

	*state = gpufreq_cooling_get_level(0, target_freq);
	if (*state == THERMAL_CSTATE_INVALID) {
		pr_warn("Failed to convert %dKHz for gpu into a cdev state\n",
				     target_freq);
		return -EINVAL;
	}

	trace_thermal_exynos_power_gpu_limit(target_freq, *state, power);
	return 0;
}
#endif
/* Bind gpufreq callbacks to thermal cooling device ops */
static struct thermal_cooling_device_ops gpufreq_cooling_ops = {
	.get_max_state = gpufreq_get_max_state,
	.get_cur_state = gpufreq_get_cur_state,
	.set_cur_state = gpufreq_set_cur_state,
};

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
		level = gpufreq_cooling_get_level(0, freq);

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
 * __gpufreq_cooling_register - helper function to create gpufreq cooling device
 * @np: a valid struct device_node to the cooling device device tree node
 * @clip_gpus: gpumask of gpus where the frequency constraints will happen.
 * @capacitance: dynamic power coefficient for these gpus
 *
 * This interface function registers the gpufreq cooling device with the name
 * "thermal-gpufreq-%x". This api can support multiple instances of gpufreq
 * cooling devices. It also gives the opportunity to link the cooling device
 * with a device tree node, in order to bind it via the thermal DT code.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
static struct thermal_cooling_device *
__gpufreq_cooling_register(struct device_node *np,
			   const struct cpumask *clip_gpus, u32 capacitance)
{
	struct thermal_cooling_device *cool_dev;
	struct gpufreq_cooling_device *gpufreq_cdev = NULL;
	char dev_name[THERMAL_NAME_LENGTH];
	int ret = 0;

	gpufreq_cdev = kzalloc(sizeof(struct gpufreq_cooling_device),
			      GFP_KERNEL);
	if (!gpufreq_cdev)
		return ERR_PTR(-ENOMEM);

	ret = get_idr(&gpufreq_idr, &gpufreq_cdev->id);
	if (ret) {
		ret = -EINVAL;
		goto err;
	}

	if (capacitance) {
#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
#else
		gpufreq_cooling_ops.get_requested_power =
			gpufreq_get_requested_power;
		gpufreq_cooling_ops.state2power = gpufreq_state2power;
		gpufreq_cooling_ops.power2state = gpufreq_power2state;
#endif
		gpufreq_cdev->is_power_actor = true;

		ret = build_dyn_power_table(gpufreq_cdev, capacitance);

		if (ret)
			goto err;

		ret = exynos_build_static_power_table(np,
					&gpufreq_cdev->var_table,
					&gpufreq_cdev->var_volt_size,
					&gpufreq_cdev->var_temp_size,
					"G3D");
		if (ret)
			goto err_st_pwr_tbl;
	}

	snprintf(dev_name, sizeof(dev_name), "thermal-gpufreq-%d",
		 gpufreq_cdev->id);

	gpufreq_cdev->tz = thermal_zone_get_zone_by_name("G3D");

	cool_dev = thermal_of_cooling_device_register(np, dev_name, gpufreq_cdev,
						      &gpufreq_cooling_ops);
	if (IS_ERR(cool_dev)) {
		ret = PTR_ERR(cool_dev);
		goto err_dev_reg;

	}

	gpufreq_cdev->tz = parse_ect_cooling_level(cool_dev, "G3D");

	gpufreq_cdev->cool_dev = cool_dev;
	gpufreq_cdev->gpufreq_state = 0;

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
	exynos_gpufreq_cdev_register(gpufreq_cdev);
#endif
	mutex_lock(&cooling_gpu_lock);

	gpufreq_cdev_count++;

	mutex_unlock(&cooling_gpu_lock);

	return cool_dev;

err_dev_reg:
	kfree(gpufreq_cdev->var_table);
err_st_pwr_tbl:
	kfree(gpufreq_cdev->dyn_power_table);
err:
	kfree(gpufreq_cdev);
	return ERR_PTR(ret);
}

/**
 * gpu_cooling_table_init - function to make GPU throttling table.
 *
 * Return : a valid struct gpu_freq_table pointer on success,
 * on failture, it returns a corresponding ERR_PTR().
 */
static int gpu_cooling_table_init(void)
{
	int i = 0;
	int num_level = 0, count = 0;
	unsigned long freq, max_freq;

	num_level = gpu_dvfs_fn->get_num_lv();
	max_freq = gpu_dvfs_fn->get_max_freq();

	if (num_level == 0) {
		pr_err("Faile to get gpu_dvfs_get_step()\n");
		return -EINVAL;
	}

	/* Table size can be num_of_range + 1 since last row has the value of TABLE_END */
	gpu_freq_table = kzalloc(sizeof(struct cpufreq_frequency_table)
					* (num_level + 1), GFP_KERNEL);

	if (!gpu_freq_table)
		return -ENOMEM;

	for (i = 0; i < num_level; i++) {
		freq = gpu_dvfs_fn->get_freq(i);

		if (freq > max_freq || freq == 0)
			continue;

		gpu_freq_table[count].flags = 0;
		gpu_freq_table[count].driver_data = count;
		gpu_freq_table[count].frequency = (unsigned int)freq;

		pr_info("[GPU cooling] index : %d, frequency : %d\n",
			gpu_freq_table[count].driver_data, gpu_freq_table[count].frequency);

		count++;
	}

	if (i == num_level)
		gpu_freq_table[count].frequency = GPU_TABLE_END;

	return 0;
}

#if IS_ENABLED(CONFIG_SOC_S5E9955)
extern u32 get_invalid_sample(void);
#endif

int exynos_gpu_cooling_init(const struct platform_device *pdev,
				const struct gpu_dvfs_fn* fn)
{
	struct device_node *np = pdev->dev.of_node;
	struct thermal_cooling_device *dev;
	void *gen_block;
	struct ect_gen_param_table *pwr_coeff;
	u32 capacitance = 0, index;
	int ret = 0;

#if IS_ENABLED(CONFIG_SOC_S5E9955)
	if (get_invalid_sample()) {
		pr_err("Cannot init gpu_cooling (thermal invalid sample)");
		return 0;
	}
#endif

	gpu_dvfs_fn = fn;

	ret = gpu_cooling_table_init();
	if (ret) {
		pr_err("Fail to initialize gpu_cooling_table\n");
		return ret;
	}

	if (!np) {
		pr_err("Fail to find device node\n");
		return -EINVAL;
	}

	of_property_read_u32(np, "gpu_power_coeff", &capacitance);

	if (of_property_read_bool(np, "use-em-coeff"))
		goto regist;

	if (!of_property_read_u32(np, "ect-coeff-index", &index)) {
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
	} else {
		pr_err("%s: could not find ect-coeff-index\n", __func__);
	}

regist:
	dev = __gpufreq_cooling_register(np, NULL, capacitance);

	if (IS_ERR(dev)) {
		pr_err("Fail to register gpufreq cooling\n");
		ret = -EINVAL;
		goto err_table;
	}

	return ret;

err_table:
	kfree(gpu_freq_table);
	return ret;
}
EXPORT_SYMBOL(exynos_gpu_cooling_init);
MODULE_LICENSE("GPL");
