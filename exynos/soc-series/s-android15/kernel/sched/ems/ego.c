/*
 * EGO(Energy-Aware CPUFreq Governor) on Energy and Scheduler-Event.
 * Copyright (C) 2021, Samsung Electronic Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/sched/cpufreq.h>
#include <linux/cpu_pm.h>
#include <linux/cpufreq.h>
#include <trace/hooks/cpuidle.h>

#include "ems.h"

#include <dt-bindings/soc/samsung/ems.h>
#include <trace/events/ems.h>
#include <trace/events/ems_debug.h>

#define IOWAIT_BOOST_MIN	(SCHED_CAPACITY_SCALE / 8)
#define HIST_SIZE		40
#define	RATIO_UNIT		1000

struct ego_policy {
	struct cpufreq_policy	*policy;

	raw_spinlock_t		update_lock;	/* For shared policies */
	u64			last_freq_update_time;
	s64			freq_update_delay_ns;
	unsigned int		next_freq;	/* final target freq */
	unsigned int		cached_raw_freq;/* util based raw freq */
	unsigned int		org_freq;	/* util based freq in table  */
	unsigned int		eng_freq;	/* lowest energy freq */
	unsigned int		flags;

	/* The next fields are only needed if fast switch cannot be used: */
	struct			irq_work irq_work;
	struct			kthread_work work;
	struct			mutex work_lock;
	struct			kthread_worker worker;
	struct task_struct	*thread;
	bool			work_in_progress;

	bool			limits_changed;
	bool			need_freq_update;
	bool			quick_rampup;
	s32			heavy_task_rate_limit;
	s32			uclamp_task_rate_limit;

	/* EGO specific */
	struct cpumask		cpus;
	struct cpumask		thread_allowed_cpus;
	int			heaviest_cpu;

	/* EGO tunables */
	unsigned int		ratio;
	int			dis_buck_share;	/* ignore buck-share when computing energy */
	int			pelt_boost;	/* dynamic changed boost */
	int			ptb_margin;	/* tunable boost */
	unsigned int		ptb_min_freq;
	int			pelt_margin;
	int			split_pelt_margin;
	unsigned int		split_pelt_margin_freq;
	s64			up_rate_limit_ns;
	int			split_up_rate_limit_ns;
	unsigned int		split_up_rate_limit_freq;
	s64			down_rate_limit_ns;

	bool			build_somac_wall;
	unsigned int		somac_wall;

	unsigned int		adaptive_high_freq;
	unsigned int		adaptive_high_freq_kernel;
	unsigned int		adaptive_low_freq;
	unsigned int		adaptive_low_freq_kernel;

	int			reason;

	struct kobject		kobj;
};

struct ego_cpu {
	struct update_util_data	update_util;
	struct ego_policy	*egp;
	unsigned int		cpu;

	bool			iowait_boost_pending;
	unsigned int		iowait_boost;
	u64			last_update;

	unsigned long		bw_dl;
	unsigned long		max;

	unsigned long		util;		/* current pelt util */
	unsigned long		io_util;	/* current io util */
	unsigned long		boosted_util;	/* current boosted util */

	struct task_struct	*heavy_task;

	unsigned long		min_cap;

	int			boost_reason;
};

struct kobject *ego_kobj;
static DEFINE_PER_CPU(struct ego_cpu, ego_cpu);
struct cpufreq_governor energy_aware_gov;

/*********************************************************************/
/*			EGO Specific Implementation		     */
/*********************************************************************/
/* returns wether cpufreq governor is EGO or NOT */
static inline bool ego_is_working(struct ego_policy *egp)
{
	return ((likely(egp)) && (likely(egp->policy)) && (egp->policy->governor_data == egp));
}

/* compute freq level diff between cur freq and given freq */
static unsigned int get_diff_num_levels(struct cpufreq_policy *policy, unsigned int freq)
{
	int index1, index2;

	index1 = cpufreq_frequency_table_get_index(policy, policy->cur);
	index2 = cpufreq_frequency_table_get_index(policy, freq);

	return abs(index1 - index2);
}

#define ESG_MAX_DELAY_PERIODS 5
/*
 * Return true if we can delay frequency update because the requested frequency
 * change is not large enough, and false if it is large enough. The condition
 * for determining large enough compares the number of frequency level change
 * vs., elapsed time since last frequency update. For example,
 * ESG_MAX_DELAY_PERIODS of 5 would mean immediate frequency change is allowed
 * only if the change in frequency level is greater or equal to 5;
 * It also means change in frequency level equal to 1 would need to
 * wait 5 ticks for it to take effect.
 */
static bool ego_postpone_freq_update(struct ego_policy *egp, u64 time, unsigned int target_freq)
{
	unsigned int diff_num_levels, num_periods, elapsed, margin;

	if (egp->need_freq_update || egp->quick_rampup)
		return false;

	elapsed = time - egp->last_freq_update_time;

	if (egp->policy->cur < target_freq)
		return elapsed < egp->up_rate_limit_ns;

	margin  = egp->freq_update_delay_ns >> 2;
	num_periods = (elapsed + margin) / egp->freq_update_delay_ns;
	if (num_periods > ESG_MAX_DELAY_PERIODS)
		return false;

	diff_num_levels = get_diff_num_levels(egp->policy, target_freq);
	if (diff_num_levels > ESG_MAX_DELAY_PERIODS - num_periods)
		return false;
	else
		return true;
}

/*********************************************************************/
/*		      Sysbusy state change notifier		     */
/*********************************************************************/
static int ego_sysbusy_notifier_call(struct notifier_block *nb,
		unsigned long val, void *v)
{
	enum sysbusy_state state = *(enum sysbusy_state *)v;
	int cpu;

	if (val != SYSBUSY_STATE_CHANGE)
		return NOTIFY_OK;

	for_each_possible_cpu(cpu) {
		struct ego_policy *egp;

		if (cpu != cpumask_first(cpu_clustergroup_mask(cpu)))
			continue;

		egp = per_cpu(ego_cpu, cpu).egp;
		if (!ego_is_working(egp))
			continue;

		egp->build_somac_wall = (state == SYSBUSY_SOMAC);
	}

	return NOTIFY_OK;
}

static struct notifier_block ego_sysbusy_notifier = {
	.notifier_call = ego_sysbusy_notifier_call,
};

/*********************************************************************/
/*		      EGO mode change notifier		     */
/*********************************************************************/
#define DEFAULT_PELT_MARGIN	(25)	/* 25% in default */
static int ego_mode_update_callback(struct notifier_block *nb, unsigned long val, void *v)
{
	struct emstune_set *cur_set = (struct emstune_set *)v;
	struct ego_policy *egp;
	int cpu;

	for_each_possible_cpu(cpu) {
		if (cpu != cpumask_first(cpu_clustergroup_mask(cpu)))
			continue;

		egp = per_cpu(ego_cpu, cpu).egp;
		if (!egp)
			continue;

		egp->pelt_boost = cur_set->cpufreq_gov.pelt_boost[cpu];
		egp->split_pelt_margin = cur_set->cpufreq_gov.split_pelt_margin[cpu];
		egp->split_pelt_margin_freq = cur_set->cpufreq_gov.split_pelt_margin_freq[cpu];
		egp->split_up_rate_limit_ns =
				cur_set->cpufreq_gov.split_up_rate_limit[cpu] * NSEC_PER_MSEC;
		egp->split_up_rate_limit_freq =
				cur_set->cpufreq_gov.split_up_rate_limit_freq[cpu];
		egp->ptb_margin = cur_set->cpufreq_gov.ptb_margin[cpu];
		egp->ptb_min_freq = cur_set->cpufreq_gov.ptb_min_freq[cpu];
	}

	return NOTIFY_OK;
}

static struct notifier_block ego_mode_update_notifier = {
	.notifier_call = ego_mode_update_callback,
};

/*********************************************************************/
/* 			      SLACK TIMER			     */
/*********************************************************************/
static void ego_update_min_cap(struct cpufreq_policy *policy)
{
	unsigned int cpu;
	unsigned long max_cap, min_cap;

	max_cap = capacity_cpu_orig(policy->cpu);

	/* min_cap is minimum value making higher frequency than policy->min */
	min_cap = (max_cap * policy->min) / policy->max;
	min_cap -= 1;

	for_each_cpu(cpu, policy->cpus)
		per_cpu(ego_cpu, cpu).min_cap = min_cap;
}

static int ego_need_slack_timer(void)
{
	unsigned int cpu = raw_smp_processor_id();
	struct ego_cpu *egc = &per_cpu(ego_cpu, cpu);
	struct ego_policy *egp = egc->egp;
	int need = 0;

	if (!ego_is_working(egp))
		return 0;

	if (egc->boosted_util > egc->min_cap) {
		need = 1;
		goto out;
	}

	/* want to add timer heaviest cpu only in this domain */
	if (egp->heaviest_cpu == cpu) {
		/* want to add timer when freq is high with energy freq, not min lock */
		if (egp->policy->cur > egp->policy->cpuinfo.min_freq &&
				egp->eng_freq > egp->org_freq)
			need = 1;
	}

out:
	trace_ego_need_slack_timer(cpu, egc->boosted_util, egc->min_cap,
			egp->heaviest_cpu, egp->policy->cur,
			egp->policy->cpuinfo.min_freq,
			egp->eng_freq, egp->org_freq, need);

	return need;
}

static int ego_get_next_cap(struct tp_env *env, struct cpumask *cpus, int dst_cpu)
{
	unsigned int cpu = raw_smp_processor_id();
	struct ego_cpu *egc = &per_cpu(ego_cpu, cpu);
	struct ego_policy *egp = egc->egp;
	unsigned long next_cap = 0;

	if (!ego_is_working(egp))
		return -1;

	for_each_cpu_and(cpu, cpus, cpu_active_mask) {
		unsigned long util;

		egc = &per_cpu(ego_cpu, cpu);
		egp = egc->egp;

		if (cpu == dst_cpu) /* util with task */
			util = env->cpu_stat[cpu].util_with;
		else /* util without task */
			util = env->cpu_stat[cpu].util_wo;

		/* apply pelt margin to calculate next cap */
		util = (util * (100 + egp->pelt_margin)) / 100;

		/*
		 * The cpu in the coregroup has same capacity and the
		 * capacity depends on the cpu with biggest utilization.
		 * Find biggest utilization in the coregroup and use it
		 * as max floor to know what capacity the cpu will have.
		 */
		if (util > next_cap)
			next_cap = util;
	}

	return next_cap;
}

/************************************************************************/
/* 			APIs for Adaptive Freq				*/
/************************************************************************/
static inline unsigned int get_adaptive_low_freq(struct ego_policy *egp)
{
	return max(egp->adaptive_low_freq, egp->adaptive_low_freq_kernel);
}

static inline unsigned int get_adaptive_high_freq(struct ego_policy *egp)
{
	return max(egp->adaptive_high_freq, egp->adaptive_high_freq_kernel);
}

int ego_set_adaptive_freq(unsigned int cpu, unsigned int low_freq, unsigned int high_freq)
{
	struct ego_policy *egp;

	if (!cpu_possible(cpu))
		return -EFAULT;

	egp = per_cpu(ego_cpu, cpu).egp;

	if (!egp->policy)
		return 0;

	egp->adaptive_low_freq_kernel = low_freq;
	egp->adaptive_high_freq_kernel = high_freq;

	return 0;

}
EXPORT_SYMBOL(ego_set_adaptive_freq);

int ego_get_adaptive_freq(unsigned int cpu, unsigned int *low_freq, unsigned int *high_freq)
{
	struct ego_policy *egp;

	if (!cpu_possible(cpu))
		return -EFAULT;

	egp = per_cpu(ego_cpu, cpu).egp;

	if (!virt_addr_valid(low_freq) || !virt_addr_valid(high_freq))
		return -EINVAL;

	*low_freq = get_adaptive_low_freq(egp);
	*high_freq = get_adaptive_high_freq(egp);

	return 0;
}
EXPORT_SYMBOL(ego_get_adaptive_freq);

int ego_reset_adaptive_freq(unsigned int cpu)
{
	struct ego_policy *egp;

	if (!cpu_possible(cpu))
		return -EFAULT;

	egp = per_cpu(ego_cpu, cpu).egp;

	egp->adaptive_low_freq_kernel = 0;
	egp->adaptive_high_freq_kernel = 0;

	return 0;
}
EXPORT_SYMBOL(ego_reset_adaptive_freq);

int ego_get_update_reason(struct cpufreq_policy *policy)
{
	struct ego_policy *egp;

	if (unlikely(!policy))
		return -1;

	if (unlikely(policy->governor != &energy_aware_gov))
		return -1;

	egp = policy->governor_data;
	if (unlikely(!egp))
		return -1;

	return egp->reason;
}
EXPORT_SYMBOL(ego_get_update_reason);

int ego_get_flag(struct cpufreq_policy *policy)
{
	struct ego_policy *egp;

	if (unlikely(!policy))
		return -1;

	if (unlikely(policy->governor != &energy_aware_gov))
		return -1;

	egp = policy->governor_data;
	if (unlikely(!egp))
		return -1;

	return egp->flags;
}
EXPORT_SYMBOL(ego_get_flag);

/************************ Governor internals ***********************/
static unsigned int ego_resolve_freq_wo_clamp(struct cpufreq_policy *policy,
					unsigned int freq)
{
	int index;

	index = cpufreq_table_find_index_al(policy, freq, false);
	if (index < 0) {
		pr_err("target frequency(%d) out of range\n", freq);
		return 0;
	}

	return policy->freq_table[index].frequency;
}

/*
 * DVFS request is sent once every 4ms, the request could be delayed.
 * To ensure the performance, support quick rampup when boosting task(uclamp_min task) is running.
 */
static bool ego_need_quick_rampup(struct ego_policy *egp, s64 delta_ns)
{
	s64 rate_limit_ns = LLONG_MAX;

	if (egp->flags & HEAVY_TASK)
		rate_limit_ns = min(egp->heavy_task_rate_limit, rate_limit_ns);
	if (egp->flags & UCLAMP_TASK)
		rate_limit_ns = min(egp->uclamp_task_rate_limit, rate_limit_ns);

	return delta_ns >= rate_limit_ns;
}

static bool ego_should_update_freq(struct ego_policy *egp, u64 time)
{
	bool should_update = false;
	s64 delta_ns, rate_limit_ns;

	/*
	 * Since cpufreq_update_util() is called with rq->lock held for
	 * the @target_cpu, our per-CPU data is fully serialized.
	 *
	 * However, drivers cannot in general deal with cross-CPU
	 * requests, so while get_next_freq() will work, our
	 * ego_update_commit() call may not for the fast switching platforms.
	 *
	 * Hence stop here for remote requests if they aren't supported
	 * by the hardware, as calculating the frequency is pointless if
	 * we cannot in fact act on it.
	 *
	 * This is needed on the slow switching platforms too to prevent CPUs
	 * going offline from leaving stale IRQ work items behind.
	 */
	if (!cpufreq_this_cpu_can_update(egp->policy))
		return false;

	if (unlikely(egp->limits_changed)) {
		egp->limits_changed = false;
		egp->need_freq_update = true;
		return true;
	}

	delta_ns = time - egp->last_freq_update_time;

	/*
	 * EGO doesn't know target frequency at this point, so consider
	 * the minimum value between up/down rate limit to cover all cases.
	 * The exact rate limit will be considered in ego_postpone_freq_update().
	 */
	rate_limit_ns = min(egp->up_rate_limit_ns, egp->down_rate_limit_ns);

	if (delta_ns >= rate_limit_ns)
		should_update = true;
	else if (ego_need_quick_rampup(egp, delta_ns)) {
		/*
		 * If dvfs is requested too frequently, acpm can be hang.
		 * To avoid this problem, at least 500us were required to request quick rampup.
		 */
		should_update = true;
		egp->quick_rampup = true;
	}

	return should_update;
}

static void ego_update_up_rate_limit(struct ego_policy *egp, u64 time,
					unsigned int next_freq)
{
	if (next_freq < egp->split_up_rate_limit_freq)
		egp->up_rate_limit_ns = 4 * NSEC_PER_MSEC; /* 4ms in default */
	else
		egp->up_rate_limit_ns = egp->split_up_rate_limit_ns;
}

static void ego_update_pelt_margin(struct ego_policy *egp, u64 time,
					unsigned int next_freq)
{
	if (next_freq < egp->split_pelt_margin_freq)
		egp->pelt_margin = DEFAULT_PELT_MARGIN;
	else
		egp->pelt_margin = egp->split_pelt_margin;
}

static void ego_update_freq_variant_param(struct ego_policy *egp, u64 time,
					unsigned int next_freq)
{
	ego_update_up_rate_limit(egp, time, next_freq);
	ego_update_pelt_margin(egp, time, next_freq);
}

static bool ego_request_freq_change(struct ego_policy *egp, u64 time, unsigned int next_freq)
{
	egp->quick_rampup = false;

	if (!egp->need_freq_update) {
		if (egp->policy->cur == next_freq)
			return false;
	} else {
		egp->need_freq_update = false;
	}

	return true;
}

/* update next freq and last frequency change requesting time  */
static void ego_update_next_freq(struct ego_policy *egp, u64 time, unsigned int next_freq)
{
	egp->next_freq = next_freq;
	egp->last_freq_update_time = time;
}

static void ego_fast_switch(struct ego_policy *egp, u64 time, unsigned int next_freq)
{
	struct cpufreq_policy *policy = egp->policy;

	if (!ego_request_freq_change(egp, time, next_freq))
		return;

	ego_update_next_freq(egp, time, next_freq);
	cpufreq_driver_fast_switch(policy, next_freq);
}


static void ego_deferred_update(struct ego_policy *egp, u64 time, unsigned int next_freq)
{
	if (!ego_request_freq_change(egp, time, next_freq))
		return;

	ego_update_next_freq(egp, time, next_freq);

	if (!egp->work_in_progress) {
		egp->work_in_progress = true;
		irq_work_queue(&egp->irq_work);
	}
}

static inline unsigned long ego_map_util_freq(struct ego_policy *egp, unsigned long util,
		unsigned long freq, unsigned long cap)
{
	return ((freq * (100 + egp->pelt_margin)) / 100) * util / cap;
}

/**
 * get_next_freq - Compute a new frequency for a given cpufreq policy.
 * @egp: schedutil policy object to compute the new frequency for.
 * @util: Current CPU utilization.
 * @max: CPU capacity.
 *
 * If the utilization is frequency-invariant, choose the new frequency to be
 * proportional to it, that is
 *
 * next_freq = C * max_freq * util / max
 *
 * Otherwise, approximate the would-be frequency-invariant utilization by
 * util_raw * (curr_freq / max_freq) which leads to
 *
 * next_freq = C * curr_freq * util_raw / max
 *
 * Take C = 1.25 for the frequency tipping point at (util / max) = 0.8.
 *
 * The lowest driver-supported frequency which is equal or greater than the raw
 * next_freq (as calculated above) is returned, subject to policy min/max and
 * cpufreq driver limitations.
 */

static unsigned int get_next_freq(struct ego_policy *egp, unsigned long util, unsigned long max)
{
	struct cpufreq_policy *policy = egp->policy;
	unsigned int freq, org_freq, fclamp_freq;
	unsigned int low_freq, high_freq;

	/* compute pure frequency base on util */
	org_freq = ego_map_util_freq(egp, util, policy->cpuinfo.max_freq, max);
	if ((org_freq == egp->cached_raw_freq || egp->work_in_progress)
			&& !egp->need_freq_update) {
		freq = max(egp->org_freq, egp->next_freq);
		goto skip_find_next_freq;
	}
	egp->cached_raw_freq = org_freq;

	/* find freq from table */
	org_freq = ego_resolve_freq_wo_clamp(policy, org_freq);
	if (egp->org_freq != org_freq) {
		egp->org_freq = org_freq;
		/* inform new freq to et */
		et_update_freq(policy->cpu, org_freq);
	}

	freq = egp->eng_freq = org_freq;

skip_find_next_freq:
	/* Apply adaptive_high/low_freq */
	low_freq = get_adaptive_low_freq(egp);
	high_freq = get_adaptive_high_freq(egp);
	if (high_freq) {
		if (org_freq < low_freq) {
			freq = low_freq;
			egp->reason = EGO_REASON_ADAPTIVE_LOW;
		} else if (org_freq <= high_freq) {
			freq = high_freq;
			egp->reason = EGO_REASON_ADAPTIVE_HIGH;
		} else {
			/* To keep calculated freq */
			egp->reason = EGO_REASON_ADAPTIVE_NONE;
		}
		goto out;
	}

	/* Apply fclamp */
	fclamp_freq = fclamp_apply(policy, freq);
	if (fclamp_freq != freq) {
		freq = fclamp_freq;
		egp->reason = EGO_REASON_FCLAMP;
	}

	/* Apply somac wall */
	if (egp->build_somac_wall && (freq > egp->somac_wall)) {
		freq = egp->somac_wall;
		egp->reason = EGO_REASON_SOMAC_WALL;
	}

out:
	freq = clamp_val(freq, policy->min, policy->max);

	trace_ego_req_freq(policy->cpu, egp->heaviest_cpu, freq, policy->min, policy->max,
			org_freq, util, max, ego_cpufreq_reason_name[egp->reason]);

	return freq;
}

/*
 * This function computes an effective utilization for the given CPU, to be
 * used for frequency selection given the linear relation: f = u * f_max.
 *
 * The scheduler tracks the following metrics:
 *
 *   cpu_util_{cfs,rt,dl,irq}()
 *   cpu_bw_dl()
 *
 * Where the cfs,rt and dl util numbers are tracked with the same metric and
 * synchronized windows and are thus directly comparable.
 *
 * The cfs,rt,dl utilization are the running times measured with rq->clock_task
 * which excludes things like IRQ and steal-time. These latter are then accrued
 * in the irq utilization.
 *
 * The DL bandwidth number otoh is not a measured metric but a value computed
 * based on the task model parameters and gives the minimal utilization
 * required to meet deadlines.
 */
unsigned long ego_cpu_util(int cpu, unsigned long util_cfs, unsigned long max,
		enum cpu_util_type type, struct task_struct *p)
{
	unsigned long dl_util, util, irq;
	struct rq *rq = cpu_rq(cpu);

	/*
	 * Early check to see if IRQ/steal time saturates the CPU, can be
	 * because of inaccuracies in how we track these -- see
	 * update_irq_load_avg().
	 */

	irq = cpu_util_irq(rq);
	if (unlikely(irq >= max)) {
		util = irq;
		goto out;
	}

	/*
	 * Because the time spend on RT/DL tasks is visible as 'lost' time to
	 * CFS tasks and we use the same metric to track the effective
	 * utilization (PELT windows are synchronized) we can directly add them
	 * to obtain the CPU's actual utilization.
	 *
	 * CFS and RT utilization can be boosted or capped, depending on
	 * utilization clamp constraints requested by currently RUNNABLE
	 * tasks.
	 * When there are no CFS RUNNABLE tasks, clamps are released and
	 * frequency will be gracefully reduced with the utilization decay.
	 */
	util = util_cfs + cpu_util_rt(rq);
	if (type == FREQUENCY_UTIL)
		util = uclamp_rq_util_with(rq, util, p);
	dl_util = cpu_util_dl(rq);

	/*
	 * For frequency selection we do not make cpu_util_dl() a permanent part
	 * of this sum because we want to use cpu_bw_dl() later on, but we need
	 * to check if the CFS+RT+DL sum is saturated (ie. no idle time) such
	 * that we select f_max when there is no idle time.
	 *
	 * NOTE: numerical errors or stop class might cause us to not quite hit
	 * saturation when we should -- something for later.
	 */
	if (util + dl_util >= max) {
		util = util + dl_util;
		goto out;
	}

	/*
	 * OTOH, for energy computation we need the estimated running time, so
	 * include util_dl and ignore dl_bw.
	 */
	if (type == ENERGY_UTIL)
		util += dl_util;

	/*
	 * There is still idle time; further improve the number by using the
	 * irq metric. Because IRQ/steal time is hidden from the task clock we
	 * need to scale the task numbers:
	 *
	 *              max - irq
	 *   U' = irq + --------- * U
	 *                 max
	 */
	util = scale_irq_capacity(util, irq, max);
	util += irq;

	/*
	 * Bandwidth required by DEADLINE must always be granted while, for
	 * FAIR and RT, we use blocked utilization of IDLE CPUs as a mechanism
	 * to gracefully reduce the frequency when no tasks show up for longer
	 * periods of time.
	 *
	 * Ideally we would like to set bw_dl as min/guaranteed freq and util +
	 * bw_dl as requested freq. However, cpufreq is not yet ready for such
	 * an interface. So, we only do the latter for now.
	 */
	if (type == FREQUENCY_UTIL)
		util += cpu_bw_dl(rq);

out:
	trace_ego_sched_util(cpu, util, util_cfs, cpu_util_rt(rq),
			cpu_util_dl(rq), cpu_bw_dl(rq), cpu_util_irq(rq));

	return min(max, util);
}

static unsigned long ego_get_util(struct ego_cpu *egc)
{
	struct rq *rq = cpu_rq(egc->cpu);
	unsigned long util = ml_cpu_util(egc->cpu);
	unsigned long max = arch_scale_cpu_capacity(egc->cpu);

	egc->max = max;
	egc->bw_dl = cpu_bw_dl(rq);
	egc->heavy_task = (struct task_struct *)ems_rq_htask(rq);

	return ego_cpu_util(egc->cpu, util, max, FREQUENCY_UTIL, NULL);
}

/**
 * ego_iowait_reset() - Reset the IO boost status of a CPU.
 * @egc: the ego data for the CPU to boost
 * @time: the update time from the caller
 * @set_iowait_boost: true if an IO boost has been requested
 *
 * The IO wait boost of a task is disabled after a tick since the last update
 * of a CPU. If a new IO wait boost is requested after more then a tick, then
 * we enable the boost starting from IOWAIT_BOOST_MIN, which improves energy
 * efficiency by ignoring sporadic wakeups from IO.
 */
static bool ego_iowait_reset(struct ego_cpu *egc, u64 time, bool set_iowait_boost)
{
	s64 delta_ns = time - egc->last_update;

	/* Reset boost only if a tick has elapsed since last request */
	if (delta_ns <= TICK_NSEC)
		return false;

	egc->iowait_boost = set_iowait_boost ? IOWAIT_BOOST_MIN : 0;
	egc->iowait_boost_pending = set_iowait_boost;

	return true;
}

/**
 * ego_iowait_boost() - Updates the IO boost status of a CPU.
 * @egc: the ego data for the CPU to boost
 * @time: the update time from the caller
 * @flags: SCHED_CPUFREQ_IOWAIT if the task is waking up after an IO wait
 *
 * Each time a task wakes up after an IO operation, the CPU utilization can be
 * boosted to a certain utilization which doubles at each "frequent and
 * successive" wakeup from IO, ranging from IOWAIT_BOOST_MIN to the utilization
 * of the maximum OPP.
 *
 * To keep doubling, an IO boost has to be requested at least once per tick,
 * otherwise we restart from the utilization of the minimum OPP.
 */
static void ego_iowait_boost(struct ego_cpu *egc, u64 time, unsigned int flags)
{
	bool set_iowait_boost = flags & SCHED_CPUFREQ_IOWAIT;

	/* Reset boost if the CPU appears to have been idle enough */
	if (egc->iowait_boost &&
			ego_iowait_reset(egc, time, set_iowait_boost))
		return;

	/* Boost only tasks waking up after IO */
	if (!set_iowait_boost)
		return;

	/* Ensure boost doubles only one time at each request */
	if (egc->iowait_boost_pending)
		return;
	egc->iowait_boost_pending = true;

	/* Double the boost at each request */
	if (egc->iowait_boost) {
		egc->iowait_boost =
			min_t(unsigned int, egc->iowait_boost << 1, SCHED_CAPACITY_SCALE);
		return;
	}

	/* First wakeup after IO: start with minimum boost */
	egc->iowait_boost = IOWAIT_BOOST_MIN;
}

/**
 * ego_iowait_apply() - Apply the IO boost to a CPU.
 * @egc: the ego data for the cpu to boost
 * @time: the update time from the caller
 * @util: the utilization to (eventually) boost
 * @max: the maximum value the utilization can be boosted to
 *
 * A CPU running a task which woken up after an IO operation can have its
 * utilization boosted to speed up the completion of those IO operations.
 * The IO boost value is increased each time a task wakes up from IO, in
 * ego_iowait_apply(), and it's instead decreased by this function,
 * each time an increase has not been requested (!iowait_boost_pending).
 *
 * A CPU which also appears to have been idle for at least one tick has also
 * its IO boost utilization reset.
 *
 * This mechanism is designed to boost high frequently IO waiting tasks, while
 * being more conservative on tasks which does sporadic IO operations.
 */
static unsigned long ego_iowait_apply(struct ego_cpu *egc, u64 time,
		unsigned long util, unsigned long max)
{
	unsigned long boost;

	/* No boost currently required */
	if (!egc->iowait_boost)
		return 0;

	/* Reset boost if the CPU appears to have been idle enough */
	if (ego_iowait_reset(egc, time, false))
		return 0;

	if (!egc->iowait_boost_pending) {
		/*
		 * No boost pending; reduce the boost value.
		 */
		egc->iowait_boost >>= 1;
		if (egc->iowait_boost < IOWAIT_BOOST_MIN) {
			egc->iowait_boost = 0;
			return 0;
		}
	}

	egc->iowait_boost_pending = false;

	/*
	 * @util is already in capacity scale; convert iowait_boost
	 * into the same scale so we can compare.
	 */
	boost = (egc->iowait_boost * max) >> SCHED_CAPACITY_SHIFT;
	return boost;
}

/*
 * Make ego_should_update_freq() ignore the rate limit when DL
 * has increased the utilization.
 */
static inline void ignore_dl_rate_limit(struct ego_cpu *egc, struct ego_policy *egp)
{
	if (cpu_bw_dl(cpu_rq(egc->cpu)) > egc->bw_dl)
		egp->limits_changed = true;
}

static int get_boost_pelt_util(int capacity, int util, int boost)
{
	long long margin;

	margin = util * boost / 100;
	return util + margin;
}

static void update_max_util(struct ego_policy *egp, struct ego_cpu *egc, unsigned long util,
			unsigned long *max, int reason)
{
	if (egp->pelt_boost && (reason == EGO_REASON_PELT_BOOST))
		goto update;

	if (util <= *max)
		return;

update:
	*max = util;
	egc->boost_reason = reason;
}

static unsigned long freq_to_cap(struct ego_policy *egp, unsigned int freq)
{
	unsigned long max_cap = arch_scale_cpu_capacity(egp->policy->cpu);
	unsigned long cap;

	cap = mult_frac(max_cap, freq, egp->policy->cpuinfo.max_freq);
	cap = mult_frac(cap, 80, 100);

	return cap;
}

static unsigned long __get_heavy_task_util(struct ego_policy *egp, struct task_struct *p)
{
	unsigned long load, margin;

	if (egp->ptb_margin < 0)
		return 0;

	load = mlt_latest_task_load(p);
	margin = mult_frac(load, egp->ptb_margin, 100);

	return load + margin;
}

static unsigned long get_heavy_task_util(struct ego_policy *egp, struct task_struct *p)
{
	unsigned long boosted_util, min_load, load;

	if (!p)
		return 0;

	min_load = freq_to_cap(egp, egp->ptb_min_freq);
	load = __get_heavy_task_util(egp, p);

	boosted_util = max(min_load, load);

	trace_ego_get_heavy_task_util(p, boosted_util, min_load, load);

	return boosted_util;
}

static unsigned long ego_apply_boost(struct ego_policy *egp, struct ego_cpu *egc,
				unsigned long util)
{
	unsigned long max_boosted_util = util, boosted_util;
	int cpu = egc->cpu;

	boosted_util = freqboost_cpu_boost(cpu, util);
	update_max_util(egp, egc, boosted_util, &max_boosted_util, EGO_REASON_FREQ_BOOST);

	boosted_util = get_heavy_task_util(egp, egc->heavy_task);
	update_max_util(egp, egc, boosted_util, &max_boosted_util, EGO_REASON_HEAVY_TASK);

	boosted_util = get_boost_pelt_util(capacity_cpu(cpu), max_boosted_util, egp->pelt_boost);
	update_max_util(egp, egc, boosted_util, &max_boosted_util, EGO_REASON_PELT_BOOST);

	return max_boosted_util;
}

static unsigned int ego_next_freq_shared(struct ego_policy *egp, u64 time)
{
	struct cpufreq_policy *policy = egp->policy;
	unsigned long util = 0, max = 1;
	unsigned int cpu;

	egp->reason = EGO_REASON_NONE;

	for_each_cpu(cpu, policy->cpus) {
		struct ego_cpu *egc = &per_cpu(ego_cpu, cpu);
		unsigned long cpu_util, cpu_boosted_util, cpu_io_util;
		unsigned long cpu_max = egc->max;

		cpu_util = ego_get_util(egc);
		if ((cpu_util > util) && !egp->pelt_boost) {
			util = cpu_util;
			egp->heaviest_cpu = cpu;
			egp->reason = EGO_REASON_UTIL;
		}

		cpu_boosted_util = ego_apply_boost(egp, egc, cpu_util);
		if (cpu_boosted_util > util) {
			util = cpu_boosted_util;
			egp->heaviest_cpu = cpu;
			egp->reason = egc->boost_reason;
		}

		cpu_io_util = ego_iowait_apply(egc, time, cpu_util, cpu_max);
		if (cpu_io_util > util) {
			util = cpu_io_util;
			egp->heaviest_cpu = cpu;
			egp->reason = EGO_REASON_IO_UTIL;
		}

		egc->util = cpu_util;
		egc->io_util = cpu_io_util;
		egc->boosted_util = cpu_boosted_util;

		/* find heaviest max */
		max = max(max, cpu_max);

		trace_ego_cpu_util(cpu, egp->pelt_boost, cpu_util, cpu_io_util,
				cpu_boosted_util, egc->heavy_task);
	}

	return get_next_freq(egp, util, max);
}

static void ego_update_shared(struct ego_cpu *egc, u64 time, unsigned int flags)
{
	struct ego_policy *egp = egc->egp;
	unsigned int next_f;

	ego_iowait_boost(egc, time, flags);
	egc->last_update = time;
	ignore_dl_rate_limit(egc, egp);

	if (egc->iowait_boost || egp->limits_changed)
		raw_spin_lock(&egp->update_lock);
	else
		if (!raw_spin_trylock(&egp->update_lock))
			return;

	egp->flags = flags;

	if (ego_should_update_freq(egp, time)) {
		next_f = ego_next_freq_shared(egp, time);

		ego_update_freq_variant_param(egp, time, next_f);

		if (ego_postpone_freq_update(egp, time, next_f))
			goto out;

		if (egp->policy->fast_switch_enabled)
			ego_fast_switch(egp, time, next_f);
		else
			ego_deferred_update(egp, time, next_f);
	}

out:
	raw_spin_unlock(&egp->update_lock);
}

static void ego_update_util_hook(struct update_util_data *hook, u64 time, unsigned int flags)
{
	struct ego_cpu *egc = container_of(hook, struct ego_cpu, update_util);

	ego_update_shared(egc, time, flags);
}

void ego_update_freq(struct rq *rq, u64 time, unsigned int flags)
{
	struct ego_cpu *egc = &per_cpu(ego_cpu, cpu_of(rq));

	if (!ego_is_working(egc->egp))
		return;

	ego_update_shared(egc, time, flags);
}

static void ego_work(struct kthread_work *work)
{
	struct ego_policy *egp = container_of(work, struct ego_policy, work);
	unsigned int freq;
	unsigned long flags;

	/*
	 * Hold egp->update_lock shortly to handle the case where:
	 * incase egp->next_freq is read here, and then updated by
	 * ego_deferred_update() just before work_in_progress is set to false
	 * here, we may miss queueing the new update.
	 *
	 * Note: If a work was queued after the update_lock is released,
	 * ego_work() will just be called again by kthread_work code; and the
	 * request will be proceed before the ego thread sleeps.
	 */
	raw_spin_lock_irqsave(&egp->update_lock, flags);
	freq = egp->next_freq;
	egp->work_in_progress = false;
	raw_spin_unlock_irqrestore(&egp->update_lock, flags);

	mutex_lock(&egp->work_lock);
	__cpufreq_driver_target(egp->policy, freq, CPUFREQ_RELATION_L);
	mutex_unlock(&egp->work_lock);
}

static void ego_irq_work(struct irq_work *irq_work)
{
	struct ego_policy *egp;

	egp = container_of(irq_work, struct ego_policy, irq_work);

	kthread_queue_work(&egp->worker, &egp->work);
}

/************************** sysfs interface ************************/
struct ego_attr {
	struct attribute attr;
	ssize_t (*show)(struct kobject *, char *);
	ssize_t (*store)(struct kobject *, const char *, size_t count);
};

#define ego_attr_rw(name)			\
static struct ego_attr name##_attr =		\
__ATTR(name, 0644, show_##name, store_##name)

#define ego_show(name)								\
static ssize_t show_##name(struct kobject *k, char *buf)			\
{										\
	struct ego_policy *egp = container_of(k, struct ego_policy, kobj);	\
										\
	return sprintf(buf, "%d\n", egp->name);					\
}										\

#define ego_store(name)								\
static ssize_t store_##name(struct kobject *k, const char *buf, size_t count)	\
{										\
	struct ego_policy *egp = container_of(k, struct ego_policy, kobj);	\
	int data;								\
										\
	if (!sscanf(buf, "%d", &data))						\
		return -EINVAL;							\
										\
	egp->name = data;							\
	return count;								\
}

ego_show(ratio);
ego_store(ratio);
ego_attr_rw(ratio);
ego_show(dis_buck_share);
ego_store(dis_buck_share);
ego_attr_rw(dis_buck_share);

ego_show(somac_wall);
ego_store(somac_wall);
ego_attr_rw(somac_wall);

static ssize_t show(struct kobject *kobj, struct attribute *at, char *buf)
{
	struct ego_attr *fvattr = container_of(at, struct ego_attr, attr);

	return fvattr->show(kobj, buf);
}

static ssize_t store(struct kobject *kobj, struct attribute *at,
		const char *buf, size_t count)
{
	struct ego_attr *fvattr = container_of(at, struct ego_attr, attr);

	return fvattr->store(kobj, buf, count);
}

static const struct sysfs_ops ego_sysfs_ops = {
	.show	= show,
	.store	= store,
};

static struct attribute *ego_attrs[] = {
	&ratio_attr.attr,
	&somac_wall_attr.attr,
	&dis_buck_share_attr.attr,
	NULL
};
ATTRIBUTE_GROUPS(ego);

static struct kobj_type ktype_ego = {
	.sysfs_ops	= &ego_sysfs_ops,
	.default_groups	= ego_groups,
};

#define adaptive_store(name)						\
static ssize_t store_##name(struct cpufreq_policy *policy,		\
		const char *buf, size_t count)				\
{									\
	struct ego_policy *egp = per_cpu(ego_cpu, policy->cpu).egp;	\
	unsigned int freq;						\
	if (sscanf(buf, "%u", &freq) != 1)				\
		return -EINVAL;						\
	egp->name = freq;						\
	return count;							\
}

#define adaptive_show(name)							\
static ssize_t show_##name(struct cpufreq_policy *policy,		\
		char *buf)						\
{									\
	struct ego_policy *egp = per_cpu(ego_cpu, policy->cpu).egp;	\
	return sprintf(buf, "%u\n", egp->name);				\
}

adaptive_store(adaptive_high_freq);
adaptive_show(adaptive_high_freq);
cpufreq_freq_attr_rw(adaptive_high_freq);

adaptive_show(adaptive_high_freq_kernel);
cpufreq_freq_attr_ro(adaptive_high_freq_kernel);

adaptive_store(adaptive_low_freq);
adaptive_show(adaptive_low_freq);
cpufreq_freq_attr_rw(adaptive_low_freq);

adaptive_show(adaptive_low_freq_kernel);
cpufreq_freq_attr_ro(adaptive_low_freq_kernel);

static int ego_adaptive_init(struct cpufreq_policy *policy)
{
	int ret = 0;

	ret |= sysfs_create_file(&policy->kobj, &adaptive_high_freq.attr);
	ret |= sysfs_create_file(&policy->kobj, &adaptive_high_freq_kernel.attr);
	ret |= sysfs_create_file(&policy->kobj, &adaptive_low_freq.attr);
	ret |= sysfs_create_file(&policy->kobj, &adaptive_low_freq_kernel.attr);

	return ret;
}

/********************** cpufreq governor interface *********************/
static int ego_kthread_create(struct ego_policy *egp)
{
	struct task_struct *thread;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO / 2 };
	struct cpufreq_policy *policy = egp->policy;
	int ret;

	/* kthread only required for slow path */
	if (policy->fast_switch_enabled)
		return 0;

	kthread_init_work(&egp->work, ego_work);
	kthread_init_worker(&egp->worker);
	thread = kthread_create(kthread_worker_fn, &egp->worker,
			"ego:%d", cpumask_first(policy->related_cpus));
	if (IS_ERR(thread)) {
		pr_err("failed to create ego thread: %ld\n", PTR_ERR(thread));
		return PTR_ERR(thread);
	}

	ret = sched_setscheduler_nocheck(thread, SCHED_FIFO, &param);
	if (ret) {
		kthread_stop(thread);
		pr_warn("%s: failed to set SCHED_FIFO\n", __func__);
		return ret;
	}

	set_cpus_allowed_ptr(thread, &egp->thread_allowed_cpus);
	thread->flags |= PF_NO_SETAFFINITY;
	egp->thread = thread;
	init_irq_work(&egp->irq_work, ego_irq_work);
	mutex_init(&egp->work_lock);

	pr_info("%s: cpus=%#x, allowed-cpu=%#x\n", __func__,
			*(unsigned int *)cpumask_bits(&egp->cpus),
			*(unsigned int *)cpumask_bits(&egp->thread_allowed_cpus));

	return 0;
}

static int ego_init(struct cpufreq_policy *policy)
{
	struct ego_policy *egp = NULL;
	int cpu;

	/* State should be equivalent to EXIT */
	if (policy->governor_data)
		return -EBUSY;

	cpufreq_enable_fast_switch(policy);

	egp = per_cpu(ego_cpu, policy->cpu).egp;
	if (!egp) {
		pr_info("%s: ego_policy is not ready\n", __func__);
		goto fail_ego_init;
	}

	if (egp->policy) {
		egp->policy = policy;
		pr_info("%s: Already ego_policy was initialized\n", __func__);
		goto complete_ego_init;
	}
	egp->policy = policy;

	if (ego_adaptive_init(policy)) {
		pr_info("%s: failed to create adaptive freq sysfs\n", __func__);
		goto fail_ego_init;
	}

	if (ego_kthread_create(egp)) {
		pr_info("%s: failed to create kthread\n", __func__);
		goto fail_ego_init;
	}

complete_ego_init:
	if (!policy->fast_switch_enabled)
		wake_up_process(egp->thread);

	policy->governor_data = egp;

	for_each_cpu(cpu, policy->related_cpus)
		cpufreq_register_hook(cpu, ego_get_next_cap, ego_need_slack_timer);

	pr_info("%s: ego init complete: cpus=%#x, allowed-cpu=%#x\n", __func__,
			*(unsigned int *)cpumask_bits(&egp->cpus),
			*(unsigned int *)cpumask_bits(&egp->thread_allowed_cpus));
	return 0;

fail_ego_init:
	cpufreq_disable_fast_switch(policy);
	pr_err("initialization failed\n");
	return -1;
}

static void ego_exit(struct cpufreq_policy *policy)
{
	int cpu;

	policy->governor_data = NULL;
	cpufreq_disable_fast_switch(policy);

	for_each_cpu(cpu, policy->related_cpus)
		cpufreq_unregister_hook(cpu);
}

static int ego_start(struct cpufreq_policy *policy)
{
	struct ego_policy *egp = policy->governor_data;
	unsigned int cpu;

	egp->pelt_margin		= DEFAULT_PELT_MARGIN;
	egp->freq_update_delay_ns	= 4 * NSEC_PER_MSEC;
	egp->up_rate_limit_ns		= 4 * NSEC_PER_MSEC;
	egp->down_rate_limit_ns		= 4 * NSEC_PER_MSEC;
	egp->last_freq_update_time	= 0;
	egp->next_freq			= 0;
	egp->work_in_progress		= false;
	egp->limits_changed		= false;
	egp->need_freq_update		= false;
	egp->cached_raw_freq		= 0;
	egp->quick_rampup		= false;

	for_each_cpu(cpu, policy->cpus) {
		struct ego_cpu *egc = &per_cpu(ego_cpu, cpu);

		egc->iowait_boost_pending = false;
		egc->iowait_boost = 0;
		egc->last_update = 0;
		egc->bw_dl = 0;
		egc->max = 0;
		egc->util = 0;
		egc->boosted_util = 0;
		egc->egp = egp;
		egc->cpu = cpu;
		egc->min_cap = ULONG_MAX;
	}

	for_each_cpu(cpu, policy->cpus) {
		struct ego_cpu *egc = &per_cpu(ego_cpu, cpu);

		cpufreq_add_update_util_hook(cpu, &egc->update_util, ego_update_util_hook);
	}
	return 0;
}

static void ego_stop(struct cpufreq_policy *policy)
{
	struct ego_policy *egp = policy->governor_data;
	unsigned int cpu;

	for_each_cpu(cpu, policy->cpus)
		cpufreq_remove_update_util_hook(cpu);

	synchronize_rcu();

	if (!policy->fast_switch_enabled) {
		irq_work_sync(&egp->irq_work);
		kthread_cancel_work_sync(&egp->work);
	}
}

static void __ego_limits(struct cpufreq_policy *policy, unsigned int target_freq)
{
	struct ego_policy *egp = policy->governor_data;

	ego_update_min_cap(policy);
	ego_update_next_freq(egp, egp->last_freq_update_time, target_freq);
	egp->reason = EGO_REASON_LIMITS;
}

static void ego_limits(struct cpufreq_policy *policy)
{
	struct ego_policy *egp = policy->governor_data;
	unsigned int target_freq;
	unsigned long flags;

	target_freq = max(egp->org_freq, egp->eng_freq);
	target_freq = clamp_val(target_freq, policy->min, policy->max);

	if (!policy->fast_switch_enabled) {
		mutex_lock(&egp->work_lock);

		raw_spin_lock_irqsave(&egp->update_lock, flags);
		__ego_limits(policy, target_freq);
		raw_spin_unlock_irqrestore(&egp->update_lock, flags);

		__cpufreq_driver_target(policy, target_freq, CPUFREQ_RELATION_H);
		mutex_unlock(&egp->work_lock);
	} else {
		raw_spin_lock_irqsave(&egp->update_lock, flags);

		__ego_limits(policy, target_freq);
		cpufreq_driver_fast_switch(policy, target_freq);

		raw_spin_unlock_irqrestore(&egp->update_lock, flags);
	}
}

struct cpufreq_governor energy_aware_gov = {
	.name			= "energy_aware",
	.owner			= THIS_MODULE,
	.flags			= CPUFREQ_GOV_DYNAMIC_SWITCHING,
	.init			= ego_init,
	.exit			= ego_exit,
	.start			= ego_start,
	.stop			= ego_stop,
	.limits			= ego_limits,
};

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ENERGYAWARE
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &energy_aware_gov;
}
#endif

static int ego_register(struct kobject *ems_kobj)
{
	ego_kobj = kobject_create_and_add("ego", ems_kobj);
	if (!ego_kobj)
		return -EINVAL;

	sysbusy_register_notifier(&ego_sysbusy_notifier);
	emstune_register_notifier(&ego_mode_update_notifier);

	return cpufreq_register_governor(&energy_aware_gov);
}

static struct ego_policy *ego_policy_alloc(void)
{
	return kzalloc(sizeof(struct ego_policy), GFP_KERNEL);
}

#define BOOST_RATE_LIMIT	(NSEC_PER_MSEC / 2)
static int ego_parse_dt(struct device_node *dn, struct ego_policy *egp)
{
	struct cpumask mask;
	const char *buf;

	if (of_property_read_string(dn, "cpus", &buf)) {
		pr_err("%s: cpus property is omitted\n", __func__);
		return -1;
	} else {
		cpulist_parse(buf, &egp->cpus);
	}

	if (!of_property_read_string(dn, "thread-run-on", &buf))
		cpulist_parse(buf, &mask);
	else
		cpumask_copy(&mask, cpu_possible_mask);
	cpumask_copy(&egp->thread_allowed_cpus, &mask);

	if (of_property_read_u32(dn, "ratio", &egp->ratio))
		egp->ratio = RATIO_UNIT;

	if (of_property_read_u32(dn, "dis-buck-share", &egp->dis_buck_share))
		egp->dis_buck_share = 0;

	if (of_property_read_u32(dn, "somac_wall", &egp->somac_wall))
		egp->somac_wall = UINT_MAX;

	if (of_property_read_s32(dn, "heavy-task-rate-limit", &egp->heavy_task_rate_limit))
		egp->heavy_task_rate_limit = BOOST_RATE_LIMIT;

	if (of_property_read_s32(dn, "uclamp-task-rate-limit", &egp->uclamp_task_rate_limit))
		egp->uclamp_task_rate_limit = BOOST_RATE_LIMIT;

	return 0;
}

int ego_pre_init(struct kobject *ems_kobj)
{
	struct device_node *dn, *child;
	int cpu;

	dn = of_find_node_by_path("/ems/ego");
	if (!dn)
		goto fail;

	ego_register(ems_kobj);

	for_each_child_of_node(dn, child) {
		struct ego_policy *egp;

		egp = ego_policy_alloc();
		if (!egp) {
			pr_err("%s: failed to alloc ego_policy\n", __func__);
			goto fail;
		}

		/* Parse device tree */
		if (ego_parse_dt(child, egp))
			goto fail;

		/* Init Sysfs */
		if (kobject_init_and_add(&egp->kobj, &ktype_ego, ego_kobj,
					"coregroup%d", cpumask_first(&egp->cpus)))
			goto fail;

		/* init policy spin lock */
		raw_spin_lock_init(&egp->update_lock);

		for_each_cpu(cpu, &egp->cpus) {
			struct ego_cpu *egc = &per_cpu(ego_cpu, cpu);
			egc->egp = egp;
		}
	}

	return 0;

fail:
	for_each_possible_cpu(cpu) {
		if (per_cpu(ego_cpu, cpu).egp)
			kfree(per_cpu(ego_cpu, cpu).egp);
		per_cpu(ego_cpu, cpu).egp = NULL;
	}

	return -1;
}

void ego_update_pelt_boost(int cpu, int value) {
	struct ego_policy *egp = per_cpu(ego_cpu, cpu).egp;

	if (egp)
		egp->pelt_boost = value;
}
