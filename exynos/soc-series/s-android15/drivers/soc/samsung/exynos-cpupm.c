/*
 * Copyright (c) 2018 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Exynos CPU Power Management driver implementation
 */

#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/tick.h>
#include <linux/cpu.h>
#include <linux/cpuhotplug.h>
#include <linux/cpu_pm.h>
#include <linux/cpuidle.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched/clock.h>

#include <trace/hooks/cpuidle.h>
#include <trace/events/ipi.h>

#include <soc/samsung/exynos-cpupm.h>
#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-pmu-if.h>
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
#include <soc/samsung/esca.h>
#else
#include <soc/samsung/exynos-pm.h>
#endif

/*
 * State of CPUPM objects
 * All CPUPM objects have 2 states, BUSY and IDLE.
 *
 * @BUSY
 * a state in which the power domain referred to by the object is turned on.
 *
 * @IDLE
 * a state in which the power domain referred to by the object is turned off.
 * However, the power domain is not necessarily turned off even if the object
 * is in IDLE state because the cpu may be booting or executing power off
 * sequence.
 */
enum {
	CPUPM_STATE_BUSY = 0,
	CPUPM_STATE_IDLE,
};

/* Length of power mode name */
#define NAME_LEN	32

#define IDLE_MAX_STATE	2

/* CPUPM statistics */
struct cpupm_stats {
	/* count of power mode entry */
	unsigned int entry_count;

	/* count of power mode entry cancllations */
	unsigned int cancel_count;

	/* power mode residency time */
	s64 residency_time;

	/* time entered power mode */
	ktime_t entry_time;
};

struct wakeup_checklist_item {
	int status_offset;
	int active_mask;

	struct list_head node;
};

struct wakeup_mask {
	int mask_reg_offset;
	int stat_reg_offset;
	int mask;

	struct list_head checklist;
};

/* wakeup mask configuration for system idle */
struct wakeup_mask_config {
	int num_wakeup_mask;
	struct wakeup_mask *wakeup_masks;

	int num_eint_wakeup_mask;
	int *eint_wakeup_mask_reg_offset;
};
static struct wakeup_mask_config *wm_config;

/*
 * Power modes
 * In CPUPM, power mode controls the power domain consisting of cpu and enters
 * the power mode by cpuidle. Basically, to enter power mode, all cpus in power
 * domain must be in IDLE state, and sleep length of cpus must be smaller than
 * target_residency.
 */
struct power_mode {
	/* name of power mode, it is declared in device tree */
	char		name[NAME_LEN];

	/* power mode state, BUSY or IDLE */
	int		state;

	/* sleep length criterion of cpus to enter power mode */
	int		target_residency;

	/* type according to range of power domain */
	int		type;

	/* index of cal, for POWERMODE_TYPE_CLUSTER */
	int		cal_id;

	/* cpus belonging to the power domain */
	struct cpumask	siblings;

	/*
	 * Among siblings, the cpus that can enter the power mode.
	 * Due to H/W constraint, only certain cpus need to enter power mode.
	 */
	struct cpumask	entry_allowed;

	/* disable count */
	atomic_t	disable;

	/*
	 * device attribute for sysfs,
	 * it supports for enabling or disabling this power mode
	 */
	struct device_attribute	attr;

	/* user's request for enabling/disabling power mode */
	bool		user_request;

	/* list of power mode */
	struct list_head	list;

	/* CPUPM statistics */
	struct cpupm_stats	stat;
	struct cpupm_stats	stat_snapshot;

	/* mode deferred */
	bool		deferred;
};

static LIST_HEAD(mode_list);

/*
 * Main struct of CPUPM
 * Each cpu has its own data structure and main purpose of this struct is to
 * manage the state of the cpu and the power modes containing the cpu.
 */
struct exynos_cpupm {
	/* cpu state, BUSY or IDLE */
	int			state;

	/* CPU statistics */
	struct cpupm_stats	stat[CPUIDLE_STATE_MAX];
	struct cpupm_stats	stat_snapshot[CPUIDLE_STATE_MAX];
	int			entered_state;
	ktime_t			entered_time;

	/* array to manage the power mode that contains the cpu */
	struct power_mode *	modes[POWERMODE_TYPE_END];

	/* hotplug flag */
	bool			hotplug;

	/* cpu state reg info */
	unsigned int		offset;
	unsigned int		lsb;
};

static struct exynos_cpupm __percpu *cpupm;
static void __iomem *cpu_state_base;
static unsigned int state_mask;
static unsigned int off_state;

/* APM IDLE IP register for to check idle/non-idle scenario */
unsigned int idle_ip_reg = 0;

static struct delayed_work deferred_work;

static void do_nothing(void *unused) { }

/*
 * State of each cpu is managed by a structure declared by percpu, so there
 * is no need for protection for synchronization. However, when entering
 * the power mode, it is necessary to set the critical section to check the
 * state of cpus in the power domain, cpupm_lock is used for it.
 */
static spinlock_t cpupm_lock;

/******************************************************************************
 *                                  CPUPM Debug                               *
 ******************************************************************************/
#define DEBUG_INFO_BUF_SIZE 1000
struct cpupm_debug_info {
	int cpu;
	u64 time;
	int event;
};

static struct cpupm_debug_info *cpupm_debug_info;
static int cpupm_debug_info_index;

static void cpupm_debug(int cpu, int state, int mode_type, int action)
{
	int i, event = -1;

	if (unlikely(!cpupm_debug_info))
		return;

	cpupm_debug_info_index++;
	if (cpupm_debug_info_index >= DEBUG_INFO_BUF_SIZE)
		cpupm_debug_info_index = 0;

	i = cpupm_debug_info_index;

	cpupm_debug_info[i].cpu = cpu;
	cpupm_debug_info[i].time = cpu_clock(cpu);

	if (state > 0) {
		event = action ? C2_ENTER : C2_EXIT;
		goto out;
	}

	switch (mode_type) {
	case POWERMODE_TYPE_CLUSTER:
		event = action ? CPD_ENTER : CPD_EXIT;
		break;
	case POWERMODE_TYPE_DSU:
		event = action ? DSUPD_ENTER : DSUPD_EXIT;
		break;
	case POWERMODE_TYPE_SYSTEM:
		event = action ? SICD_ENTER : SICD_EXIT;
		break;
	}

out:
	cpupm_debug_info[i].event = event;
}

/******************************************************************************
 *                                    Notifier                                *
 ******************************************************************************/
static DEFINE_RWLOCK(notifier_lock);
static RAW_NOTIFIER_HEAD(notifier_chain);

int exynos_cpupm_notifier_register(struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	write_lock_irqsave(&notifier_lock, flags);
	ret = raw_notifier_chain_register(&notifier_chain, nb);
	write_unlock_irqrestore(&notifier_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_cpupm_notifier_register);

static int exynos_cpupm_notify(int event, int v)
{
	int ret = 0;

	read_lock(&notifier_lock);
	ret = raw_notifier_call_chain(&notifier_chain, event, &v);
	read_unlock(&notifier_lock);

	return notifier_to_errno(ret);
}

static DEFINE_RWLOCK(fcd_notifier_lock);
static RAW_NOTIFIER_HEAD(fcd_notifier_chain);

int exynos_cpupm_fcd_notifier_register(struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	write_lock_irqsave(&fcd_notifier_lock, flags);
	ret = raw_notifier_chain_register(&fcd_notifier_chain, nb);
	write_unlock_irqrestore(&fcd_notifier_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(exynos_cpupm_fcd_notifier_register);

/* cpupm lock must be held */
static int exynos_cpupm_fcd_notify(int event, int v)
{
	int ret = 0;

	lockdep_assert_held(&cpupm_lock);

	ret = raw_notifier_call_chain(&fcd_notifier_chain, event, &v);

	return notifier_to_errno(ret);
}

/******************************************************************************
 *                                  IDLE_IP                                   *
 ******************************************************************************/
struct idle_ip {
	/* list node of ip */
	struct list_head	list;

	/* ip name */
	const char		*name;

	/* ip index, unique */
	unsigned int		index;

	/* IO coherency */
	unsigned int		io_cc;

	/* ip type , 0:normal ip, 1:extern ip */
	unsigned int		type;

	/* ip idle state, 0:busy, 1:io-co */
	unsigned int		idle;

	/* busy count for cpuidle-profiler */
	unsigned int		busy_count;
	unsigned int		busy_count_profile;

	/* pmu offset for extern idle-ip */
	unsigned int		pmu_offset;
};

static DEFINE_SPINLOCK(idle_ip_lock);

static LIST_HEAD(ip_list);

#define NORMAL_IP	0
#define EXTERN_IP	1
static bool __ip_busy(struct idle_ip *ip)
{
	unsigned int val;

	if (ip->type == NORMAL_IP) {
		if (ip->idle == CPUPM_STATE_BUSY)
			return true;
		else
			return false;
	}

	if (ip->type == EXTERN_IP) {
		exynos_pmu_read(ip->pmu_offset, &val);
		if (!!val == CPUPM_STATE_BUSY)
			return true;
		else
			return false;
	}

	return false;
}

static void cpupm_profile_idle_ip(void);

static bool ip_busy(struct power_mode *mode)
{
	struct idle_ip *ip;
	unsigned long flags;

	spin_lock_irqsave(&idle_ip_lock, flags);

	cpupm_profile_idle_ip();

	list_for_each_entry(ip, &ip_list, list) {
		if (__ip_busy(ip)) {
			/*
			 * IP that does not do IO coherency with DSU does
			 * not need to be checked in DSU off mode.
			 */
			if (mode->type == POWERMODE_TYPE_DSU && !ip->io_cc)
				continue;

			spin_unlock_irqrestore(&idle_ip_lock, flags);

			/* IP is busy */
			return true;
		}
	}
	spin_unlock_irqrestore(&idle_ip_lock, flags);

	/* IPs are idle */
	return false;
}

static struct idle_ip *find_ip(int index)
{
	struct idle_ip *ip = NULL;

	list_for_each_entry(ip, &ip_list, list)
		if (ip->index == index)
			break;

	return ip;
}

static u64 non_idle_ip_list = 0;
static void exynos_update_apm_ip_idle_status(int index, int idle)
{
	u64 pos;

	pos = index % 64;
	non_idle_ip_list  = non_idle_ip_list & ((u64)(~(((u64)1) << pos)));
	non_idle_ip_list = non_idle_ip_list | ((u64)(((u64)!idle) << pos));
	if (non_idle_ip_list)
		exynos_pmu_write(idle_ip_reg, 0);
	else
		exynos_pmu_write(idle_ip_reg, 1);
}

/*
 * @index: index of idle-ip
 * @idle: idle status, (idle == 0)non-idle or (idle == 1)idle
 */
void exynos_update_ip_idle_status(int index, int idle)
{
	struct idle_ip *ip;
	unsigned long flags;

	spin_lock_irqsave(&idle_ip_lock, flags);
	ip = find_ip(index);
	if (!ip) {
		pr_err("unknown idle-ip index %d\n", index);
		spin_unlock_irqrestore(&idle_ip_lock, flags);
		return;
	}

	ip->idle = idle;
	if(idle_ip_reg)
		exynos_update_apm_ip_idle_status(index, idle);

	spin_unlock_irqrestore(&idle_ip_lock, flags);
}
EXPORT_SYMBOL_GPL(exynos_update_ip_idle_status);

/*
 * register idle-ip dynamically by name, return idle-ip index.
 */
int exynos_get_idle_ip_index(const char *name, int io_cc)
{
	struct idle_ip *ip;
	unsigned long flags;
	int new_index;

	ip = kzalloc(sizeof(struct idle_ip), GFP_KERNEL);
	if (!ip) {
		pr_err("Failed to allocate idle_ip. [ip=%s].", name);
		return -ENOMEM;
	}

	spin_lock_irqsave(&idle_ip_lock, flags);

	if (list_empty(&ip_list))
		new_index = 0;
	else
		new_index = list_last_entry(&ip_list,
				struct idle_ip, list)->index + 1;

	ip->name = name;
	ip->index = new_index;
	ip->type = NORMAL_IP;
	ip->io_cc = io_cc;
	list_add_tail(&ip->list, &ip_list);

	spin_unlock_irqrestore(&idle_ip_lock, flags);

	exynos_update_ip_idle_status(ip->index, CPUPM_STATE_BUSY);

	return ip->index;
}
EXPORT_SYMBOL_GPL(exynos_get_idle_ip_index);

/******************************************************************************
 *                               CPUPM profiler                               *
 ******************************************************************************/
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
struct esca_dbg {
	unsigned int apsoc;
	unsigned int apsoc_dsupd;
	unsigned int apsoc_dsupd_ew;
	unsigned int apsoc_sicd;
	unsigned int apsoc_sicd_ew;
	unsigned int apsoc_sleep;
	unsigned int apsoc_sleep_ew;
	unsigned int mif;
	unsigned int mif_sicd;
	unsigned int mif_sleep;
};
static struct esca_dbg esca_dbg;
#else
struct acpm_dbg {
	unsigned int apsoc;
	unsigned int apsoc_dsupd;
	unsigned int apsoc_dsupd_ew;
	unsigned int apsoc_sicd;
	unsigned int apsoc_sicd_ew;
	unsigned int apsoc_sleep;
	unsigned int apsoc_sleep_ew;
	unsigned int mif;
	unsigned int mif_sicd;
	unsigned int mif_sleep;
};
static struct acpm_dbg acpm_dbg;
#endif

static ktime_t cpupm_init_time;
static void cpupm_profile_begin(struct cpupm_stats *stat, ktime_t now)
{
	stat->entry_time = now;
	stat->entry_count++;
}

static void
cpupm_profile_end(struct cpupm_stats *stat, int cancel, ktime_t now)
{
	if (!stat->entry_time)
		return;

	if (cancel) {
		stat->cancel_count++;
		return;
	}

	stat->residency_time +=
		ktime_to_us(ktime_sub(now, stat->entry_time));
	stat->entry_time = 0;
}

static u32 idle_ip_check_count;
static u32 idle_ip_check_count_profile;

static void cpupm_profile_idle_ip(void)
{
	struct idle_ip *ip;

	idle_ip_check_count++;

	list_for_each_entry(ip, &ip_list, list)
		if (__ip_busy(ip))
			ip->busy_count++;
}

#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
static void update_esca_dbg(bool start)
{
	if (start) {
		esca_dbg.apsoc = esca_get_apsoc_count();
		esca_dbg.apsoc_dsupd = esca_get_apsoc_dsupd_count();
		esca_dbg.apsoc_dsupd_ew = esca_get_apsoc_dsupd_early_wakeup_count();
		esca_dbg.apsoc_sicd = esca_get_apsicd_count();
		esca_dbg.apsoc_sicd_ew = esca_get_apsoc_sicd_early_wakeup_count();
		esca_dbg.apsoc_sleep = esca_get_apsleep_count();
		esca_dbg.apsoc_sleep_ew = esca_get_apsoc_sleep_early_wakeup_count();
		esca_dbg.mif = esca_get_mifdn_count();
		esca_dbg.mif_sicd = esca_get_mifdn_sicd_count();
		esca_dbg.mif_sleep = esca_get_mifdn_sleep_count();
	} else {
		esca_dbg.apsoc = esca_get_apsoc_count() - esca_dbg.apsoc;
		esca_dbg.apsoc_dsupd = esca_get_apsoc_dsupd_count() - esca_dbg.apsoc_dsupd;
		esca_dbg.apsoc_dsupd_ew = esca_get_apsoc_dsupd_early_wakeup_count()
					- esca_dbg.apsoc_dsupd_ew;
		esca_dbg.apsoc_sicd = esca_get_apsicd_count() - esca_dbg.apsoc_sicd;
		esca_dbg.apsoc_sicd_ew = esca_get_apsoc_sicd_early_wakeup_count()
					- esca_dbg.apsoc_sicd_ew;
		esca_dbg.apsoc_sleep = esca_get_apsleep_count() - esca_dbg.apsoc_sleep;
		esca_dbg.apsoc_sleep_ew = esca_get_apsoc_sleep_early_wakeup_count()
					- esca_dbg.apsoc_sleep_ew;
		esca_dbg.mif = esca_get_mifdn_count() - esca_dbg.mif;
		esca_dbg.mif_sicd = esca_get_mifdn_sicd_count() - esca_dbg.mif_sicd;
		esca_dbg.mif_sleep = esca_get_mifdn_sleep_count() - esca_dbg.mif_sleep;
	}
}
#else
static void update_acpm_dbg(bool start)
{
	if (start) {
		acpm_dbg.apsoc = acpm_get_apsoc_count();
		acpm_dbg.apsoc_dsupd = acpm_get_apsoc_dsupd_count();
		acpm_dbg.apsoc_dsupd_ew = acpm_get_apsoc_dsupd_early_wakeup_count();
		acpm_dbg.apsoc_sicd = acpm_get_apsicd_count();
		acpm_dbg.apsoc_sicd_ew = acpm_get_apsoc_sicd_early_wakeup_count();
		acpm_dbg.apsoc_sleep = acpm_get_apsleep_count();
		acpm_dbg.apsoc_sleep_ew = acpm_get_apsoc_sleep_early_wakeup_count();
		acpm_dbg.mif = acpm_get_mifdn_count();
		acpm_dbg.mif_sicd = acpm_get_mifdn_sicd_count();
		acpm_dbg.mif_sleep = acpm_get_mifdn_sleep_count();
	} else {
		acpm_dbg.apsoc = acpm_get_apsoc_count() - acpm_dbg.apsoc;
		acpm_dbg.apsoc_dsupd = acpm_get_apsoc_dsupd_count() - acpm_dbg.apsoc_dsupd;
		acpm_dbg.apsoc_dsupd_ew = acpm_get_apsoc_dsupd_early_wakeup_count()
					- acpm_dbg.apsoc_dsupd_ew;
		acpm_dbg.apsoc_sicd = acpm_get_apsicd_count() - acpm_dbg.apsoc_sicd;
		acpm_dbg.apsoc_sicd_ew = acpm_get_apsoc_sicd_early_wakeup_count()
					- acpm_dbg.apsoc_sicd_ew;
		acpm_dbg.apsoc_sleep = acpm_get_apsleep_count() - acpm_dbg.apsoc_sleep;
		acpm_dbg.apsoc_sleep_ew = acpm_get_apsoc_sleep_early_wakeup_count()
					- acpm_dbg.apsoc_sleep_ew;
		acpm_dbg.mif = acpm_get_mifdn_count() - acpm_dbg.mif;
		acpm_dbg.mif_sicd = acpm_get_mifdn_sicd_count() - acpm_dbg.mif_sicd;
		acpm_dbg.mif_sleep = acpm_get_mifdn_sleep_count() - acpm_dbg.mif_sleep;
	}
}
#endif


static int profiling;
static ktime_t profile_time;

static ssize_t show_stats(char *buf, bool profile)
{
	struct exynos_cpupm *pm;
	struct power_mode *mode;
	struct cpupm_stats *stat;
	struct idle_ip *ip;
	s64 total;
	int i, cpu, ret = 0;

	if (profile && profiling)
		return scnprintf(buf, PAGE_SIZE, "Profile is ongoing\n");

	ret += scnprintf(buf + ret, PAGE_SIZE - ret,
			"format : [mode] [entry_count] [cancel_count] [time] [(ratio)]\n\n");

	total = profile ? ktime_to_us(profile_time)
			: ktime_to_us(ktime_sub(ktime_get(), cpupm_init_time));

	for (i = 0; i < IDLE_MAX_STATE; i++) {
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "[state%d]\n", i);
		for_each_possible_cpu(cpu) {
			pm = per_cpu_ptr(cpupm, cpu);
			stat = profile ? pm->stat_snapshot : pm->stat;
			ret += scnprintf(buf + ret, PAGE_SIZE - ret,
					 "cpu%d %d %d %lld (%llu%%)\n",
					 cpu,
					 stat[i].entry_count,
					 stat[i].cancel_count,
					 stat[i].residency_time,
					 stat[i].residency_time * 100 / total);
		}
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}

	list_for_each_entry(mode, &mode_list, list) {
		stat = profile ? &mode->stat_snapshot : &mode->stat;
		ret += scnprintf(buf + ret, PAGE_SIZE - ret,
				 "%-7s %d %d %lld (%llu%%)\n",
				 mode->name,
				 stat->entry_count,
				 stat->cancel_count,
				 stat->residency_time,
				 stat->residency_time * 100 / total);
	}

	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
	ret += scnprintf(buf + ret, PAGE_SIZE - ret,
			"[IDLE-IP statistics]\n");
	ret += scnprintf(buf + ret, PAGE_SIZE - ret,
			"(I:IO-CC, E:Extern IP)\n");
	list_for_each_entry(ip, &ip_list, list)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret,
				 "%s%s %-20s : busy %d/%d\n",
				 ip->io_cc ? "[I]" : "[-]",
				 ip->type == EXTERN_IP ? "[E]" : "[-]",
				 ip->name,
				 profile ? ip->busy_count_profile
					 : ip->busy_count,
				 profile ? idle_ip_check_count_profile
					 : idle_ip_check_count);

	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "[ACPM DEBUG]\n");

#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC CNT",
			profile ? esca_dbg.apsoc : esca_get_apsoc_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC DSUPD",
			profile ? esca_dbg.apsoc_dsupd : esca_get_apsoc_dsupd_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC DSUPD EARLY WAKEUP",
			profile ? esca_dbg.apsoc_dsupd_ew : esca_get_apsoc_dsupd_early_wakeup_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC SICD",
			profile ? esca_dbg.apsoc_sicd : esca_get_apsicd_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC SICD EARLY WAKEUP",
			profile ? esca_dbg.apsoc_sicd_ew : esca_get_apsoc_sicd_early_wakeup_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC SLEEP",
			profile ? esca_dbg.apsoc_sleep : esca_get_apsleep_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC SLEEP EARLY WAKEUP",
			profile ? esca_dbg.apsoc_sleep_ew : esca_get_apsoc_sleep_early_wakeup_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "MIF CNT",
			profile ? esca_dbg.mif : esca_get_mifdn_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "MIF SICD",
			profile ? esca_dbg.mif_sicd : esca_get_mifdn_sicd_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "MIF SLEEP",
			profile ? esca_dbg.mif_sleep : esca_get_mifdn_sleep_count());
#else
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC CNT",
			profile ? acpm_dbg.apsoc : acpm_get_apsoc_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC DSUPD",
			profile ? acpm_dbg.apsoc_dsupd : acpm_get_apsoc_dsupd_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC DSUPD EARLY WAKEUP",
			profile ? acpm_dbg.apsoc_dsupd_ew : acpm_get_apsoc_dsupd_early_wakeup_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC SICD",
			profile ? acpm_dbg.apsoc_sicd : acpm_get_apsicd_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC SICD EARLY WAKEUP",
			profile ? acpm_dbg.apsoc_sicd_ew : acpm_get_apsoc_sicd_early_wakeup_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC SLEEP",
			profile ? acpm_dbg.apsoc_sleep : acpm_get_apsleep_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "APSOC SLEEP EARLY WAKEUP",
			profile ? acpm_dbg.apsoc_sleep_ew : acpm_get_apsoc_sleep_early_wakeup_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "MIF CNT",
			profile ? acpm_dbg.mif : acpm_get_mifdn_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "MIF SICD",
			profile ? acpm_dbg.mif_sicd : acpm_get_mifdn_sicd_count());
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%s: %d\n", "MIF SLEEP",
			profile ? acpm_dbg.mif_sleep : acpm_get_mifdn_sleep_count());
#endif

	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "\n");
	ret += scnprintf(buf + ret, PAGE_SIZE - ret, "(total %lldus)\n", total);

	return ret;

}

static ssize_t profile_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return show_stats(buf, true);
}

static ssize_t profile_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct exynos_cpupm *pm;
	struct power_mode *mode;
	struct idle_ip *ip;
	int input, cpu, i;

	if (!sscanf(buf, "%d", &input))
		return -EINVAL;

	input = !!input;
	if (profiling == input)
		return count;

	profiling = input;

	if (!input)
		goto stop_profile;

	preempt_disable();
	smp_call_function(do_nothing, NULL, 1);
	preempt_enable();

	for_each_possible_cpu(cpu) {
		pm = per_cpu_ptr(cpupm, cpu);
		for (i = 0; i < IDLE_MAX_STATE; i++)
			pm->stat_snapshot[i] = pm->stat[i];
	}

	list_for_each_entry(mode, &mode_list, list)
		mode->stat_snapshot = mode->stat;

	list_for_each_entry(ip, &ip_list, list)
		ip->busy_count_profile = ip->busy_count;

	profile_time = ktime_get();
	idle_ip_check_count_profile = idle_ip_check_count;

#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
	update_esca_dbg(true);
#else
	update_acpm_dbg(true);
#endif

	return count;

stop_profile:
#define delta(a, b)	(a = b - a)
#define field_delta(field)				\
	delta(mode->stat_snapshot.field, mode->stat.field);
#define state_delta(field)				\
	for (i = 0; i < IDLE_MAX_STATE; i++)	\
		delta(pm->stat_snapshot[i].field, pm->stat[i].field)

	preempt_disable();
	smp_call_function(do_nothing, NULL, 1);
	preempt_enable();

	for_each_possible_cpu(cpu) {
		pm = per_cpu_ptr(cpupm, cpu);
		state_delta(entry_count);
		state_delta(cancel_count);
		state_delta(residency_time);
	}

	list_for_each_entry(mode, &mode_list, list) {
		field_delta(entry_count);
		field_delta(cancel_count);
		field_delta(residency_time);
	}

	list_for_each_entry(ip, &ip_list, list)
		ip->busy_count_profile = ip->busy_count - ip->busy_count_profile;

	profile_time = ktime_sub(ktime_get(), profile_time);
	idle_ip_check_count_profile = idle_ip_check_count
				- idle_ip_check_count_profile;

#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
	update_esca_dbg(false);
#else
	update_acpm_dbg(false);
#endif

	return count;
}

static ssize_t time_in_state_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return show_stats(buf, false);
}

static DEVICE_ATTR_RO(time_in_state);
static DEVICE_ATTR_RW(profile);

/******************************************************************************
 *                            CPU idle management                             *
 ******************************************************************************/
/* Macros for CPUPM state */
#define set_state_busy(object)		((object)->state = CPUPM_STATE_BUSY)
#define set_state_idle(object)		((object)->state = CPUPM_STATE_IDLE)
#define check_state_busy(object)	((object)->state == CPUPM_STATE_BUSY)
#define check_state_idle(object)	((object)->state == CPUPM_STATE_IDLE)
#define valid_powermode(type)		((type >= 0) && (type < POWERMODE_TYPE_END))

static void awake_cpus(const struct cpumask *cpus)
{
	int cpu;

	for_each_cpu_and(cpu, cpus, cpu_online_mask)
		smp_call_function_single(cpu, do_nothing, NULL, 1);
}

/*
 * disable_power_mode/enable_power_mode
 * It provides "disable" function to enable/disable power mode as required by
 * user or device driver. To handle multiple disable requests, it use the
 * atomic disable count, and disable the mode that contains the given cpu.
 */
static void __disable_power_mode(struct power_mode *mode)
{
	/*
	 * There are no entry allowed cpus, it means that mode is
	 * disabled, skip awaking cpus.
	 */
	if (cpumask_empty(&mode->entry_allowed))
		return;

	/*
	 * The first disable request wakes the cpus to exit power mode
	 */
	if (atomic_inc_return(&mode->disable) == 1)
		awake_cpus(&mode->siblings);
}

static void __enable_power_mode(struct power_mode *mode)
{
	atomic_dec(&mode->disable);
	awake_cpus(&mode->siblings);
}

static void control_power_mode(int cpu, int type, bool enable)
{
	struct exynos_cpupm *pm;
	struct power_mode *mode;

	if (!valid_powermode(type))
		return;

	pm = per_cpu_ptr(cpupm, cpu);
	mode = pm->modes[type];
	if (mode == NULL)
		return;

	if (enable)
		__enable_power_mode(mode);
	else
		__disable_power_mode(mode);
}

void disable_power_mode(int cpu, int type)
{
	control_power_mode(cpu, type, false);
}
EXPORT_SYMBOL_GPL(disable_power_mode);

void enable_power_mode(int cpu, int type)
{
	control_power_mode(cpu, type, true);
}
EXPORT_SYMBOL_GPL(enable_power_mode);

struct cpumask pm_allowed_mask;
void update_pm_allowed_mask(const struct cpumask *mask)
{
	cpumask_copy(&pm_allowed_mask, mask);
}
EXPORT_SYMBOL_GPL(update_pm_allowed_mask);

struct cpumask idle_allowed_mask;
void update_idle_allowed_mask(const struct cpumask *mask)
{
	cpumask_copy(&idle_allowed_mask, mask);
}
EXPORT_SYMBOL_GPL(update_idle_allowed_mask);

static ktime_t __percpu *cpu_next_event;
static s64 get_sleep_length(int cpu, ktime_t now)
{
	ktime_t next_event = *per_cpu_ptr(cpu_next_event, cpu);
	return ktime_to_us(ktime_sub(next_event, now));
}

static int __percpu *cpu_ipi_pending;
static inline int ipi_pending(int cpu)
{
	return *per_cpu_ptr(cpu_ipi_pending, cpu);
}

static int cpus_busy(int target_residency, const struct cpumask *cpus)
{
	int cpu;
	ktime_t now = ktime_get();

	/*
	 * If there is even one cpu which is not in POWERDOWN state or has
	 * the smaller sleep length than target_residency, CPUPM regards
	 * it as BUSY.
	 */
	for_each_cpu_and(cpu, cpu_online_mask, cpus) {
		struct exynos_cpupm *pm = per_cpu_ptr(cpupm, cpu);

		if (check_state_busy(pm))
			return -EBUSY;

		if (get_sleep_length(cpu, now) < target_residency)
			return -EBUSY;

		if (ipi_pending(cpu))
			return -EBUSY;
	}

	return 0;
}

static bool is_cpu_off(int cpu)
{
	struct exynos_cpupm *pm = per_cpu_ptr(cpupm, cpu);
	unsigned int val;

	val = readl(cpu_state_base + pm->offset) >> pm->lsb;

	return (val & state_mask) == off_state;
}

static int cpus_last_core_detecting(int request_cpu, const struct cpumask *cpus)
{
	int cpu, i;

	/*
	 * To prevent ping-pong problems when entering power mode, power status of CPU
	 * about to enter power mode is updated to C2 after last-core detection (flexpmu).
	 * Additionally, if it fails to enter power mode, it will not enter C2 either.
	 *
	 * Therefore, when trying to enter power mode, if any of the sibling CPUs
	 * are in power on state & trying to enter power mode, current CPU cannot enter
	 * power mode because it may not be the last core.
	 */

	for_each_cpu_and(cpu, cpu_online_mask, cpus) {
		if (cpu == request_cpu)
			continue;

		if (is_cpu_off(cpu))
			continue;

		for (i = 0; i < POWERMODE_TYPE_END; i++) {
			struct power_mode *mode = per_cpu_ptr(cpupm, cpu)->modes[i];

			if (mode == NULL)
				continue;

			if (check_state_idle(mode))
				return -EBUSY;
		}
	}

	return 0;
}

#define BOOT_CPU	0
static int cluster_busy(void)
{
	int cpu;
	struct power_mode *mode = NULL;

	for_each_online_cpu(cpu) {
		if (mode && cpumask_test_cpu(cpu, &mode->siblings))
			continue;

		mode = per_cpu_ptr(cpupm, cpu)->modes[POWERMODE_TYPE_CLUSTER];
		if (mode == NULL)
			continue;

		if (cpumask_test_cpu(BOOT_CPU, &mode->siblings))
			continue;

		if (check_state_busy(mode))
			return 1;
	}

	return 0;
}

static bool system_busy(struct power_mode *mode)
{
	if (mode->type < POWERMODE_TYPE_DSU)
		return false;

	if (cluster_busy())
		return true;

	if (ip_busy(mode))
		return true;

	return false;
}

/*
 * In order to enter the power mode, the following conditions must be met:
 * 1. power mode should not be disabled
 * 2. the cpu attempting to enter must be a cpu that is allowed to enter the
 *    power mode.
 */
static bool entry_allow(int cpu, struct power_mode *mode)
{
	if (atomic_read(&mode->disable))
		return false;

	if (mode->type == POWERMODE_TYPE_CLUSTER && !cpumask_test_cpu(cpu, &pm_allowed_mask))
		return false;

	if (!cpumask_test_cpu(cpu, &mode->entry_allowed))
		return false;

	if (cpus_busy(mode->target_residency, &mode->siblings))
		return false;

	if (cpus_last_core_detecting(cpu, &mode->siblings))
		return false;

	if (system_busy(mode))
		return false;

	return true;
}

static void update_wakeup_mask(int *wakeup_mask, int idx)
{
	int on;
	struct list_head *checklist = &wm_config->wakeup_masks[idx].checklist;
	struct wakeup_checklist_item *item;

	list_for_each_entry(item, checklist, node) {
		exynos_pmu_read(item->status_offset, &on);

		if (on)
			*wakeup_mask |= item->active_mask;
	}
}

extern u32 exynos_eint_wake_mask_array[3];
static void set_wakeup_mask(void)
{
	int i;
	int wakeup_mask;

	if (!wm_config) {
		WARN_ONCE(1, "no wakeup mask information\n");
		return;
	}

	for (i = 0; i < wm_config->num_wakeup_mask; i++) {
		wakeup_mask = wm_config->wakeup_masks[i].mask;

		update_wakeup_mask(&wakeup_mask, i);

		exynos_pmu_write(wm_config->wakeup_masks[i].stat_reg_offset, 0);
		exynos_pmu_write(wm_config->wakeup_masks[i].mask_reg_offset,
				 wakeup_mask);
	}

	for (i = 0; i < wm_config->num_eint_wakeup_mask; i++)
		exynos_pmu_write(wm_config->eint_wakeup_mask_reg_offset[i],
				exynos_eint_wake_mask_array[i]);
}

static void cluster_disable(struct power_mode *mode)
{
	cal_cluster_disable(mode->cal_id);
}

static void cluster_enable(struct power_mode *mode)
{
	cal_cluster_enable(mode->cal_id);
}

static bool system_disabled;
static int system_disable(struct power_mode *mode)
{
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
	if (mode->type == POWERMODE_TYPE_DSU)
		esca_noti_dsu_cpd(true);
	else
		esca_noti_dsu_cpd(false);
#else
	if (mode->type == POWERMODE_TYPE_DSU)
		acpm_noti_dsu_cpd(true);
	else
		acpm_noti_dsu_cpd(false);
#endif

	if (system_disabled)
		return 0;

	if (mode->type == POWERMODE_TYPE_SYSTEM) {
		if (unlikely(exynos_cpupm_notify(SICD_ENTER, 0))) {
			exynos_cpupm_notify(SICD_EXIT, 0);
			return -EBUSY;
		}
	} else if (mode->type == POWERMODE_TYPE_DSU) {
		if (unlikely(exynos_cpupm_notify(DSUPD_ENTER, 0))) {
			exynos_cpupm_notify(DSUPD_EXIT, 0);
			return -EBUSY;
		}
	}

	set_wakeup_mask();
	cal_pm_enter(mode->cal_id);
	system_disabled = 1;

	return 0;
}

static void system_enable(struct power_mode *mode, int cancel)
{
	if (!system_disabled)
		return;

#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
	esca_noti_dsu_cpd(false);
#else
	acpm_noti_dsu_cpd(false);
#endif

	if (cancel)
		cal_pm_earlywakeup(mode->cal_id);
	else
		cal_pm_exit(mode->cal_id);

	if (mode->type == POWERMODE_TYPE_SYSTEM)
		exynos_cpupm_notify(SICD_EXIT, cancel);
	else if (mode->type == POWERMODE_TYPE_DSU)
		exynos_cpupm_notify(DSUPD_EXIT, cancel);

	system_disabled = 0;
}

static void enter_power_mode(int cpu, struct power_mode *mode, ktime_t now)
{
	switch (mode->type) {
	case POWERMODE_TYPE_CLUSTER:
		cluster_disable(mode);
		break;
	case POWERMODE_TYPE_DSU:
	case POWERMODE_TYPE_SYSTEM:
		if (system_disable(mode))
			return;
		break;
	}

	cpupm_debug(cpu, -1, mode->type, 1);
	dbg_snapshot_cpuidle(mode->name, 0, 0, DSS_FLAG_IN);
	set_state_idle(mode);

	cpupm_profile_begin(&mode->stat, now);
}

static void exit_power_mode(int cpu, struct power_mode *mode, int cancel, ktime_t now)
{
	cpupm_profile_end(&mode->stat, cancel, now);

	/*
	 * Configure settings to exit power mode. This is executed by the
	 * first cpu exiting from power mode.
	 */
	set_state_busy(mode);
	dbg_snapshot_cpuidle(mode->name, 0, 0, DSS_FLAG_OUT);
	cpupm_debug(cpu, -1, mode->type, 0);

	switch (mode->type) {
	case POWERMODE_TYPE_CLUSTER:
		cluster_enable(mode);
		break;
	case POWERMODE_TYPE_DSU:
	case POWERMODE_TYPE_SYSTEM:
		system_enable(mode, cancel);
		break;
	}
}

/*
 * exynos_cpu_pm_enter/exit() are called by android_vh_cpu_idle_enter/exit(), vendorhook
 * to handle platform specific configuration to control cpu power domain.
 */
static void exynos_cpupm_enter(int cpu, ktime_t now)
{
	struct exynos_cpupm *pm;
	int i;

	spin_lock(&cpupm_lock);
	cpupm_debug(cpu, 1, -1, 1);

	pm = per_cpu_ptr(cpupm, cpu);

	/* Configure PMUCAL to power down core */
	cal_cpu_disable(cpu);

	/* Set cpu state to IDLE */
	set_state_idle(pm);

	/* Try to enter power mode */
	for (i = 0; i < POWERMODE_TYPE_END; i++) {
		struct power_mode *mode = pm->modes[i];

		if (mode == NULL)
			continue;

		if (entry_allow(cpu, mode))
			enter_power_mode(cpu, mode, now);
	}

	spin_unlock(&cpupm_lock);
}

static bool first_core_detection(void)
{
	int cpu, num_of_busy_cpus = 0;
	struct exynos_cpupm *pm;

	for_each_cpu(cpu, cpu_online_mask) {
		pm = per_cpu_ptr(cpupm, cpu);
		if (check_state_busy(pm))
			num_of_busy_cpus++;
	}

	return num_of_busy_cpus == 1;
}

static void exynos_cpupm_exit(int cpu, int cancel, ktime_t now)
{
	struct exynos_cpupm *pm;
	int i;

	spin_lock(&cpupm_lock);
	pm = per_cpu_ptr(cpupm, cpu);

	/* Make settings to exit from mode */
	for (i = 0; i < POWERMODE_TYPE_END; i++) {
		struct power_mode *mode = pm->modes[i];

		if (mode == NULL)
			continue;

		if (check_state_idle(mode))
			exit_power_mode(cpu, mode, cancel, now);
	}

	/* Set cpu state to BUSY */
	set_state_busy(pm);

	/* Configure PMUCAL to power up core */
	cal_cpu_enable(cpu);

	cpupm_debug(cpu, 1, -1, 0);

	if (first_core_detection())
		exynos_cpupm_fcd_notify(0, 0);

	spin_unlock(&cpupm_lock);
}
/******************************************************************************
 *			mode deferred control				      *
 ******************************************************************************/
#define BOOT_LOCK_TIME_MS	40000
static void deferred_work_fn(struct work_struct *work)
{
	struct power_mode *mode;

	list_for_each_entry(mode, &mode_list, list) {
		if (mode->deferred) {
			__enable_power_mode(mode);
			pr_info("%s: [%s] deferred done", __func__, mode->name);
		}
	}
}

static void deferred_init(void)
{
	struct power_mode *mode;

	list_for_each_entry(mode, &mode_list, list) {
		if (mode->deferred) {
			__disable_power_mode(mode);
			pr_info("%s: [%s] deferred start", __func__, mode->name);
		}
	}

	INIT_DELAYED_WORK(&deferred_work, deferred_work_fn);
	schedule_delayed_work(&deferred_work, msecs_to_jiffies(BOOT_LOCK_TIME_MS));
}

static ssize_t debug_net_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cpu, ret = 0;

	for_each_online_cpu(cpu)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "[cpu%d] %lld\n",
				cpu, *per_cpu_ptr(cpu_next_event, cpu));

	return ret;
}
DEVICE_ATTR_RO(debug_net);

static ssize_t debug_ipip_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cpu, ret = 0;

	for_each_online_cpu(cpu)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "[cpu%d] %d\n",
				cpu, *per_cpu_ptr(cpu_ipi_pending, cpu));

	return ret;
}
DEVICE_ATTR_RO(debug_ipip);

static ssize_t idle_ip_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct idle_ip *ip;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&idle_ip_lock, flags);

	list_for_each_entry(ip, &ip_list, list)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "[%d] %s %s\n",
				 ip->index, ip->name,
				 ip->type == EXTERN_IP ? "(E)" : "");

	spin_unlock_irqrestore(&idle_ip_lock, flags);

	return ret;
}
DEVICE_ATTR_RO(idle_ip);

static ssize_t power_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct power_mode *mode = container_of(attr, struct power_mode, attr);

	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 atomic_read(&mode->disable) > 0 ? "disabled" : "enabled");
}

static ssize_t power_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct power_mode *mode = container_of(attr, struct power_mode, attr);
	unsigned int val;
	int cpu, type;

	if (!sscanf(buf, "%u", &val))
		return -EINVAL;

	cpu = cpumask_any(&mode->siblings);
	type = mode->type;

	val = !!val;
	if (mode->user_request == val)
		return count;

	mode->user_request = val;
	if (val)
		__enable_power_mode(mode);
	else
		__disable_power_mode(mode);

	return count;
}

static struct attribute *exynos_cpupm_attrs[] = {
	&dev_attr_debug_net.attr,
	&dev_attr_debug_ipip.attr,
	&dev_attr_idle_ip.attr,
	&dev_attr_time_in_state.attr,
	&dev_attr_profile.attr,
	NULL,
};

static struct attribute_group exynos_cpupm_group = {
	.name = "cpupm",
	.attrs = exynos_cpupm_attrs,
};

#define CPUPM_ATTR(_attr, _name, _mode, _show, _store)		\
	sysfs_attr_init(&_attr.attr);				\
	_attr.attr.name	= _name;				\
	_attr.attr.mode	= VERIFY_OCTAL_PERMISSIONS(_mode);	\
	_attr.show	= _show;				\
	_attr.store	= _store;

/******************************************************************************
 *                               CPU HOTPLUG                                  *
 ******************************************************************************/
static void cpuhp_cluster_enable(int cpu)
{
	struct exynos_cpupm *pm = per_cpu_ptr(cpupm, cpu);
	struct power_mode *mode = pm->modes[POWERMODE_TYPE_CLUSTER];

	if (mode) {
		struct cpumask mask;

		cpumask_and(&mask, &mode->siblings, cpu_online_mask);
		if (cpumask_weight(&mask) == 0)
			cluster_enable(mode);
	}
}

static void cpuhp_cluster_disable(int cpu)
{
	struct exynos_cpupm *pm = per_cpu_ptr(cpupm, cpu);
	struct power_mode *mode = pm->modes[POWERMODE_TYPE_CLUSTER];

	if (mode) {
		struct cpumask mask;

		cpumask_and(&mask, &mode->siblings, cpu_online_mask);
		if (cpumask_weight(&mask) == 1)
			cluster_disable(mode);
	}
}

static int cpuhp_cpupm_online(unsigned int cpu)
{
	cpuhp_cluster_enable(cpu);
	cal_cpu_enable(cpu);

	/*
	 * At this point, mark this cpu as finished the hotplug.
	 * Because the hotplug in sequence is done, this cpu could enter C2.
	 */
	per_cpu_ptr(cpupm, cpu)->hotplug = false;

	return 0;
}

static int cpuhp_cpupm_offline(unsigned int cpu)
{
	/*
	 * At this point, mark this cpu as entering the hotplug.
	 * In order not to confusing ACPM, the cpu that entering the hotplug
	 * should not enter C2.
	 */
	per_cpu_ptr(cpupm, cpu)->hotplug = true;

	cal_cpu_disable(cpu);
	cpuhp_cluster_disable(cpu);

	return 0;
}

/******************************************************************************
 *                                  vendor hook                               *
 ******************************************************************************/
static void android_vh_cpu_idle_enter(void *data, int *state,
		struct cpuidle_device *dev)
{
	struct exynos_cpupm *pm = per_cpu_ptr(cpupm, dev->cpu);
	struct cpuidle_driver *drv = cpuidle_get_cpu_driver(dev);
	struct cpuidle_state *target_state;
	ktime_t now = ktime_get();
	int cpu = smp_processor_id();

	if (*state >= IDLE_MAX_STATE)
		*state = IDLE_MAX_STATE - 1;

	if (pm->hotplug) {
		cpuhp_cluster_enable(cpu);
		cal_cpu_enable(cpu);
		*state = 0;
	}

	if (!cpumask_test_cpu(cpu, &idle_allowed_mask))
		*state = 0;

	if (*state > 0) {
		if (unlikely(exynos_cpupm_notify(C2_ENTER, 0))) {
			exynos_cpupm_notify(C2_EXIT, 0);
			*state = 0;
		}
	}

	pm->entered_state = *state;
	cpupm_profile_begin(&pm->stat[pm->entered_state], now);

	target_state = &drv->states[pm->entered_state];
	dbg_snapshot_cpuidle(target_state->desc, 0, 0, DSS_FLAG_IN);
	pm->entered_time = ns_to_ktime(local_clock());

	/* Only handle requests except C1 */
	if (pm->entered_state > 0) {
		*per_cpu_ptr(cpu_next_event, dev->cpu) = dev->next_hrtimer;
		exynos_cpupm_enter(cpu, now);
	}
}

static void android_vh_cpu_idle_exit(void *data, int state,
		struct cpuidle_device *dev)
{
	struct exynos_cpupm *pm = per_cpu_ptr(cpupm, dev->cpu);
	int cancel = (state < 0);
	int cpu = smp_processor_id();
	struct cpuidle_driver *drv = cpuidle_get_cpu_driver(dev);
	struct cpuidle_state *target_state;
	ktime_t time_start = pm->entered_time, time_end;
	int residency;
	ktime_t now = ktime_get();

	cpupm_profile_end(&pm->stat[pm->entered_state], cancel, now);

	if (pm->hotplug) {
		cal_cpu_disable(cpu);
		cpuhp_cluster_disable(cpu);
	}

	/* Only handle requests except C1 */
	if (pm->entered_state > 0) {
		exynos_cpupm_notify(C2_EXIT, cancel);
		exynos_cpupm_exit(cpu, cancel, now);
	}

	target_state = &drv->states[pm->entered_state];
	time_end = ns_to_ktime(local_clock());
	residency = (int)ktime_to_us(ktime_sub(time_end, time_start));
	dbg_snapshot_cpuidle(target_state->desc, 0, residency, cancel ? state : DSS_FLAG_OUT);
}

static void ipi_raise(void *data, const struct cpumask *target,
		const char *reason)
{
	int cpu;

	for_each_cpu(cpu, target)
		*per_cpu_ptr(cpu_ipi_pending, cpu) = 1;
}

static void ipi_entry(void *data, const char *reason)
{
}

static void ipi_exit(void *data, const char *reason)
{
	*per_cpu_ptr(cpu_ipi_pending, smp_processor_id()) = 0;
}

static void register_vendor_hooks(void)
{
	register_trace_android_vh_cpu_idle_enter(android_vh_cpu_idle_enter, NULL);
	register_trace_android_vh_cpu_idle_exit(android_vh_cpu_idle_exit, NULL);
	register_trace_ipi_raise(ipi_raise, NULL);
	register_trace_ipi_entry(ipi_entry, NULL);
	register_trace_ipi_exit(ipi_exit, NULL);
}

static void wakeup_mask_init(struct device_node *cpupm_dn)
{
	struct device_node *root_dn, *wm_dn, *dn;
	int count, i, free_wm_count;

	root_dn = of_find_node_by_name(cpupm_dn, "wakeup-mask");
	if (!root_dn) {
		pr_warn("wakeup-mask is omitted in device tree\n");
		return;
	}

	wm_config = kzalloc(sizeof(struct wakeup_mask_config), GFP_KERNEL);
	if (!wm_config) {
		pr_warn("failed to allocate wakeup mask config\n");
		return;
	}

	/* initialize wakeup-mask */
	wm_dn = of_find_node_by_name(root_dn, "wakeup-masks");
	if (!wm_dn) {
		pr_warn("wakeup-masks is omitted in device tree\n");
		goto fail;
	}

	count = of_get_child_count(wm_dn);
	wm_config->num_wakeup_mask = count;
	wm_config->wakeup_masks = kcalloc(count,
			sizeof(struct wakeup_mask), GFP_KERNEL);
	if (!wm_config->wakeup_masks) {
		pr_warn("failed to allocate wakeup masks\n");
		goto fail;
	}

	i = 0;
	for_each_child_of_node(wm_dn, dn) {
		struct device_node *checklist_dn;
		struct wakeup_mask *wm;
		struct of_phandle_iterator iter;
		int err;

		/* get pointer of currently-initialized wakeup mask */
		wm = &wm_config->wakeup_masks[i];

		of_property_read_u32(dn, "mask-reg-offset",
				     &wm->mask_reg_offset);
		of_property_read_u32(dn, "stat-reg-offset",
				     &wm->stat_reg_offset);
		of_property_read_u32(dn, "mask",
				     &wm->mask);

		INIT_LIST_HEAD(&wm->checklist);

		checklist_dn = of_get_child_by_name(dn, "checklist");
		of_for_each_phandle(&iter, err, checklist_dn, "list", NULL, 0) {
			struct wakeup_checklist_item *item;

			item = kzalloc(sizeof(struct wakeup_checklist_item),
				       GFP_KERNEL);
			if (!item) {
				pr_warn("failed to allocate "
					"wakeup mask changelist item\n");
				free_wm_count = i + 1;
				goto fail_to_alloc_checklist;
			}

			of_property_read_u32(iter.node, "status-offset",
					     &item->status_offset);
			of_property_read_u32(iter.node, "active-mask",
					     &item->active_mask);

			INIT_LIST_HEAD(&item->node);
			list_add_tail(&item->node, &wm->checklist);
		}

		i++;
	}

	/* store the number of wakeup masks */
	free_wm_count = i;

	/* initialize eint-wakeup-mask */
	wm_dn = of_find_node_by_name(root_dn, "eint-wakeup-masks");
	if (!wm_dn) {
		pr_warn("eint-wakeup-masks is omitted in device tree\n");
		goto fail;
	}

	count = of_get_child_count(wm_dn);
	wm_config->num_eint_wakeup_mask = count;
	wm_config->eint_wakeup_mask_reg_offset = kcalloc(count,
			sizeof(int), GFP_KERNEL);
	if (!wm_config->eint_wakeup_mask_reg_offset) {
		pr_warn("failed to allocate eint wakeup masks\n");
		goto fail_to_alloc_eint_wm_reg_offset;
	}

	i = 0;
	for_each_child_of_node(wm_dn, dn) {
		of_property_read_u32(dn, "mask-reg-offset",
			&wm_config->eint_wakeup_mask_reg_offset[i]);
		i++;
	}

	return;

fail_to_alloc_eint_wm_reg_offset:
	kfree(wm_config->eint_wakeup_mask_reg_offset);
fail_to_alloc_checklist:
	for (i = 0; i < free_wm_count; i++) {
		struct list_head *checklist, *cursor, *temp;

		checklist = &wm_config->wakeup_masks[i].checklist;
		list_for_each_safe(cursor, temp, checklist) {
			struct wakeup_checklist_item *item;

			list_del(cursor);
			item = list_entry(cursor,
					  struct wakeup_checklist_item,
					  node);
			kfree(item);
		}
	}
fail:
	kfree(wm_config->wakeup_masks);
	kfree(wm_config);
}

static int cpu_state_init(struct device_node *cpupm_dn)
{
	struct device_node *dn;
	struct exynos_cpupm *pm;
	unsigned int base_addr;
	unsigned int *cpu_offsets, *cpu_lsbs;
	int cpu, num, ret = 0;

	dn = of_get_child_by_name(cpupm_dn, "cpu-state");
	if (!dn)
		return -EINVAL;

	ret = of_property_read_u32(dn, "base-addr", &base_addr);
	if (ret) {
		pr_err("Failed to get base address of cpu state\n");
		return ret;
	}

	cpu_state_base = ioremap(base_addr, SZ_64K);

	ret = of_property_read_u32(dn, "state-mask", &state_mask);
	if (ret) {
		pr_err("Failed to get state-mask\n");
		return ret;
	}

	ret = of_property_read_u32(dn, "off-state", &off_state);
	if (ret) {
		pr_err("Failed to get off-state\n");
		return ret;
	}

	num = of_property_count_u32_elems(dn, "cpu-offset");
	if (num < 0) {
		pr_err("Failed to get count of cpu-offset\n");
		return -EINVAL;
	}

	if (of_property_count_u32_elems(dn, "cpu-lsb") != num) {
		pr_err("Failed to get count of cpu-lsb\n");
		return -EINVAL;
	}

	cpu_offsets = kzalloc(sizeof(unsigned int) * num, GFP_KERNEL);
	if (!cpu_offsets) {
		pr_err("Failed to alloc cpu_offsets\n");
		return -ENOMEM;
	}

	cpu_lsbs = kzalloc(sizeof(unsigned int) * num, GFP_KERNEL);
	if (!cpu_lsbs) {
		pr_err("Failed to alloc cpu_lsbs\n");
		kfree(cpu_offsets);
		return -ENOMEM;
	}

	ret = of_property_read_u32_array(dn, "cpu-offset", cpu_offsets, num);
	if (ret) {
		pr_err("Failed to get cpu-offset array\n");
		goto out;
	}

	ret = of_property_read_u32_array(dn, "cpu-lsb", cpu_lsbs, num);
	if (ret) {
		pr_err("Failed to get cpu-lsb array\n");
		goto out;
	}

	for (cpu = 0; cpu < num; cpu++) {
		pm = per_cpu_ptr(cpupm, cpu);
		pm->offset = cpu_offsets[cpu];
		pm->lsb = cpu_lsbs[cpu];
	}

out:
	kfree(cpu_offsets);
	kfree(cpu_lsbs);

	return ret;
}

static int exynos_cpupm_mode_init(struct platform_device *pdev)
{
	struct device_node *dn = pdev->dev.of_node;

	cpupm = alloc_percpu(struct exynos_cpupm);

	while ((dn = of_find_node_by_type(dn, "cpupm"))) {
		struct power_mode *mode;
		const char *buf;
		int cpu, ret;

		/*
		 * Power mode is dynamically generated according to
		 * what is defined in device tree.
		 */
		mode = kzalloc(sizeof(struct power_mode), GFP_KERNEL);
		if (!mode) {
			pr_err("Failed to allocate powermode.\n");
			return -ENOMEM;
		}

		strncpy(mode->name, dn->name, NAME_LEN - 1);

		ret = of_property_read_u32(dn, "target-residency",
				&mode->target_residency);
		if (ret)
			return ret;

		ret = of_property_read_u32(dn, "type", &mode->type);
		if (ret)
			return ret;

		if (!valid_powermode(mode->type))
			return -EINVAL;

		if (!of_property_read_string(dn, "siblings", &buf)) {
			cpulist_parse(buf, &mode->siblings);
			cpumask_and(&mode->siblings, &mode->siblings,
					cpu_possible_mask);
		}

		if (!of_property_read_string(dn, "entry-allowed", &buf)) {
			cpulist_parse(buf, &mode->entry_allowed);
			cpumask_and(&mode->entry_allowed, &mode->entry_allowed,
					cpu_possible_mask);
		}

		ret = of_property_read_u32(dn, "cal-id", &mode->cal_id);
		if (ret) {
			if (mode->type >= POWERMODE_TYPE_DSU)
				mode->cal_id = SYS_SICD;
			else
				return ret;
		}

		mode->deferred = of_property_read_bool(dn, "deferred-enabled");

		atomic_set(&mode->disable, 0);

		/*
		 * The users' request is set to enable since initialization
		 * state of power mode is enabled.
		 */
		mode->user_request = true;

		/*
		 * Initialize attribute for sysfs.
		 * The absence of entry allowed cpu is equivalent to this power
		 * mode being disabled. In this case, no attribute is created.
		 */
		if (!cpumask_empty(&mode->entry_allowed)) {
			CPUPM_ATTR(mode->attr, mode->name, 0644,
					power_mode_show, power_mode_store);

			ret = sysfs_add_file_to_group(&pdev->dev.kobj,
					&mode->attr.attr,
					exynos_cpupm_group.name);
			if (ret)
				pr_warn("Failed to add sysfs or POWERMODE\n");
		}

		/* Connect power mode to the cpus in the power domain */
		for_each_cpu(cpu, &mode->siblings) {
			struct exynos_cpupm *pm = per_cpu_ptr(cpupm, cpu);

			pm->modes[mode->type] = mode;
		}

		list_add_tail(&mode->list, &mode_list);

		/*
		 * At the point of CPUPM initialization, all CPUs are already
		 * hotplugged in without calling cluster_enable() because
		 * CPUPM cannot know cal-id at that time.
		 * Explicitly call cluster_enable() here.
		 */
		if (mode->type == POWERMODE_TYPE_CLUSTER)
			cluster_enable(mode);
	}

	wakeup_mask_init(pdev->dev.of_node);

	return 0;
}

#define PMU_IDLE_IP(x)			(0x03E0 + (0x4 * x))
#define EXTERN_IDLE_IP_MAX		4
static int extern_idle_ip_init(struct device_node *dn)
{
	struct device_node *child = of_get_child_by_name(dn, "idle-ip");
	int i, count, new_index;
	unsigned long flags;

	if (!child)
		return 0;

	count = of_property_count_strings(child, "extern-idle-ip");
	if (count <= 0 || count > EXTERN_IDLE_IP_MAX)
		return 0;

	spin_lock_irqsave(&idle_ip_lock, flags);

	for (i = 0; i < count; i++) {
		struct idle_ip *ip;
		const char *name;

		ip = kzalloc(sizeof(struct idle_ip), GFP_KERNEL);
		if (!ip) {
			pr_err("Failed to allocate idle_ip [ip=%s].", name);
			spin_unlock_irqrestore(&idle_ip_lock, flags);
			return -ENOMEM;
		}

		of_property_read_string_index(child, "extern-idle-ip", i, &name);

		new_index = list_last_entry(&ip_list,
				struct idle_ip, list)->index + 1;

		ip->name = name;
		ip->index = new_index;
		ip->type = EXTERN_IP;
		ip->pmu_offset = PMU_IDLE_IP(i);
		list_add_tail(&ip->list, &ip_list);
	}

	spin_unlock_irqrestore(&idle_ip_lock, flags);

	return 0;
}

/*
 * The power off sequence is executed after CPUPM_ENABLE.
 * Before SMC is done, interrupt is asserted to
 * all CPUs to cancel power off sequence in progress. Because there may
 * be a CPU that tries to power off without setting PMUCAL handled in
 * CPU_PM_ENTER event.
 */
static void notify_cpupm_init_to_el3(struct platform_device *pdev)
{
	/* 0 : Disable 1 : Enable for the second argument*/
	exynos_smc(SMC_CMD_CPUPM_ENABLE, 1, 0, 0);
}

static int exynos_cpupm_probe(struct platform_device *pdev)
{
	struct device *dev_root;
	struct device_node *dn = pdev->dev.of_node;
	int ret, cpu;

	ret = extern_idle_ip_init(pdev->dev.of_node);
	if (ret)
		return ret;

	ret = sysfs_create_group(&pdev->dev.kobj, &exynos_cpupm_group);
	if (ret)
		pr_warn("Failed to create sysfs for CPUPM\n");

	/* Link CPUPM sysfs to /sys/devices/system/cpu/cpupm */
	dev_root = bus_get_dev_root(&cpu_subsys);
	if (dev_root) {
		if (sysfs_create_link(&dev_root->kobj, &pdev->dev.kobj, "cpupm"))
			pr_err("Failed to link CPUPM sysfs to cpu\n");

		put_device(dev_root);
	} else {
		pr_warn("Failed to find cpu subsys device root\n");
	}

	if (!cpuidle_get_driver())
		return -EINVAL;

	ret = exynos_cpupm_mode_init(pdev);
	if (ret)
		return ret;

	of_property_read_u32(dn, "apm-idle-ip", &idle_ip_reg);

	ret = cpu_state_init(pdev->dev.of_node);
	if (ret)
		return ret;

	update_pm_allowed_mask(cpu_possible_mask);
	update_idle_allowed_mask(cpu_possible_mask);

	/* set PMU to power on */
	for_each_online_cpu(cpu)
		cal_cpu_enable(cpu);

	cpu_next_event = alloc_percpu(ktime_t);
	cpu_ipi_pending = alloc_percpu(int);

	cpuhp_setup_state(CPUHP_AP_ARM_TWD_STARTING,
			"AP_EXYNOS_CPU_POWER_UP_CONTROL",
			cpuhp_cpupm_online, NULL);

	cpuhp_setup_state(CPUHP_AP_ONLINE_DYN,
			"AP_EXYNOS_CPU_POWER_DOWN_CONTROL",
			NULL, cpuhp_cpupm_offline);

	spin_lock_init(&cpupm_lock);

	cpupm_debug_info = kzalloc(sizeof(struct cpupm_debug_info)
				* DEBUG_INFO_BUF_SIZE, GFP_KERNEL);

	register_vendor_hooks();

	notify_cpupm_init_to_el3(pdev);

	cpupm_init_time = ktime_get();

	deferred_init();

	return 0;
}

static const struct of_device_id of_exynos_cpupm_match[] = {
	{ .compatible = "samsung,exynos-cpupm", },
	{ },
};
MODULE_DEVICE_TABLE(of, of_exynos_cpupm_match);

static struct platform_driver exynos_cpupm_driver = {
	.driver = {
		.name = "exynos-cpupm",
		.owner = THIS_MODULE,
		.of_match_table = of_exynos_cpupm_match,
	},
	.probe		= exynos_cpupm_probe,
};

static int __init exynos_cpupm_driver_init(void)
{
	return platform_driver_register(&exynos_cpupm_driver);
}
arch_initcall(exynos_cpupm_driver_init);

static void __exit exynos_cpupm_driver_exit(void)
{
	platform_driver_unregister(&exynos_cpupm_driver);
}
module_exit(exynos_cpupm_driver_exit);

MODULE_DESCRIPTION("Exynos CPUPM driver");
MODULE_LICENSE("GPL");
