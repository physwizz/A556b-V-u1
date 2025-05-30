/* linux/include/soc/samsung/exynos-dm.h
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS5 - Header file for exynos DVFS Manager support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_DM_H
#define __EXYNOS_DM_H

#include <linux/irq_work.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/types.h>
#if IS_ENABLED(CONFIG_EXYNOS_ESCA_DVFS_MANAGER)
#include <soc/samsung/exynos-dm-acpm-ipc.h>
#endif

#define EXYNOS_DM_MODULE_NAME		"exynos-dm"
#define EXYNOS_DM_TYPE_NAME_LEN		16
#define EXYNOS_DM_ATTR_NAME_LEN		(EXYNOS_DM_TYPE_NAME_LEN + 16)

#define EXYNOS_DM_RELATION_L		0
#define EXYNOS_DM_RELATION_H		1

enum exynos_constraint_type {
	CONSTRAINT_MIN = 0,
	CONSTRAINT_MAX,
	CONSTRAINT_END
};

enum dss_dm_req_type {
	POLICY_IN = 0,
	POLICY_OUT,
	FREQ_SYNC_IN,
	FREQ_SYNC_OUT,
	FREQ_ASYNC,
};

struct exynos_dm_freq {
	u32				master_freq;
	u32				slave_freq;
};

struct exynos_dm_attrs {
	struct device_attribute attr;
	char name[EXYNOS_DM_ATTR_NAME_LEN];
};

struct exynos_dm_constraint {
	int				dm_master;
	int				dm_slave;
	enum exynos_constraint_type	constraint_type;
	bool				guidance;	/* check constraint table by hw guide */

	u32				table_length;

	bool				support_dynamic_disable;
	bool				support_variable_freq_table;
	struct exynos_dm_freq		*freq_table;
	struct exynos_dm_freq		**variable_freq_table;
	u32				num_table_index;
	u32				current_table_idx;

	u32				const_freq;
	u32				gov_freq;
#if IS_ENABLED(CONFIG_EXYNOS_DVFS_MANAGER)
	struct list_head		master_domain;
	struct list_head		slave_domain;

	struct exynos_dm_constraint	*sub_constraint;
#elif IS_ENABLED(CONFIG_EXYNOS_ESCA_DVFS_MANAGER)
	u32				constraint_id;
#endif
};

struct exynos_dm_data {
	bool				available;		/* use for DVFS domain available */
	int				dm_type;
	char				dm_type_name[EXYNOS_DM_TYPE_NAME_LEN];

	/* For Fast Switch */
	bool				fast_switch;		// Use Fast Switch

	int				(*freq_scaler)(int dm_type, void *devdata, u32 target_freq, unsigned int relation);

	void				*devdata;

	struct exynos_dm_attrs		dm_policy_attr;
	struct exynos_dm_attrs		constraint_table_attr;

	u32				cal_id;
	u32			num_constraints;
#if IS_ENABLED(CONFIG_EXYNOS_DVFS_MANAGER)
	int			my_order;		// Scaling order in domain_order
	int			indegree;		// Number of min masters

	u32			cur_freq;		// Current frequency
	u32			next_target_freq;	// Next target frequency determined by current status
	u32			governor_freq;	// Frequency determined by DVFS governor
	u32			gov_min;		// Constraint by current frequency of min master domains
	u32			policy_min;		// Min frequency limition in this domin
	u32			policy_max;		// Min frequency limition in this domin
	u32			const_min;		// Constraint by min frequency of min master domains
	u32			const_max;		// Constraint1 by max frequency of max master domains
	struct list_head		min_slaves;
	struct list_head		max_slaves;
	struct list_head		min_masters;
	struct list_head		max_masters;

	bool				fast_switch_post_in_progress;
	struct irq_work			fast_switch_post_irq_work;
	struct task_struct		*fast_switch_post_worker;
#if IS_ENABLED(CONFIG_EXYNOS_ACPM) || IS_ENABLED(CONFIG_EXYNOS_ESCA)
	bool				policy_use;
#endif
#endif
};

struct exynos_dm_device {
	struct device			*dev;
	struct mutex			lock;
	int				domain_count;
	int				*domain_order;
	struct exynos_dm_data		*dm_data;
	int				dynamic_disable;
#if IS_ENABLED(CONFIG_EXYNOS_DVFS_MANAGER)
	int				constraint_domain_count;
	unsigned int			fast_switch_ch;
#elif IS_ENABLED(CONFIG_EXYNOS_ESCA_DVFS_MANAGER)
	unsigned int			dm_ch;
	unsigned int			dm_req_ch;
	unsigned int			dm_sync_ch;
	unsigned int			fast_switch_ch;
	unsigned int			num_running_request;
#endif
};

#if IS_ENABLED(CONFIG_EXYNOS_DVFS_MANAGER)
struct exynos_dm_fast_switch_notify_data {
	u32 domain;
	u32 freq;
	ktime_t time;
};

#elif IS_ENABLED(CONFIG_EXYNOS_ESCA_DVFS_MANAGER)
struct exynos_dm_domain_constraint_policy {
	u32 master_dm_type;
	u32 type;
	u32 policy_minmax;
	u32 const_minmax;
	u32 const_freq;
	u32 gov_min;
	u32 governor_freq;
	u32 gov_freq;
	u32 guidance;
};

struct exynos_dm_domain_policy {
	u32 dm_type;
	u32 policy_min;
	u32 policy_max;
	u32 const_min;
	u32 const_max;
	u32 gov_min;
	u32 governor_freq;
	u32 cur_freq;
	u32 num_const;
	struct exynos_dm_domain_constraint_policy *const_policy;
};

struct exynos_dm_domain_constraint {
	u32 num_constraints;

	struct exynos_dm_constraint *constraint;
};
#endif

/* External Function call */
#if IS_ENABLED(CONFIG_EXYNOS_DVFS_MANAGER) || IS_ENABLED(CONFIG_EXYNOS_ESCA_DVFS_MANAGER)
extern int exynos_dm_data_init(int dm_type, void *data,
			u32 min_freq, u32 max_freq, u32 cur_freq);
extern int register_exynos_dm_constraint_table(int dm_type,
				struct exynos_dm_constraint *constraint);
extern int register_exynos_dm_freq_scaler(int dm_type,
			int (*scaler_func)(int dm_type, void *devdata, u32 target_freq, unsigned int relation));
extern int unregister_exynos_dm_freq_scaler(int dm_type);
extern int policy_update_call_to_DM(int dm_type, u32 min_freq, u32 max_freq);
extern int DM_CALL(int dm_type, unsigned long *target_freq);
extern void exynos_dm_dynamic_disable(int flag);
extern int exynos_dm_fast_switch_notifier_register(struct notifier_block *n);
extern int exynos_dm_change_freq_table(struct exynos_dm_constraint *constraint, int idx);
#else
static inline
int exynos_dm_data_init(int dm_type, void *data,
			u32 min_freq, u32 max_freq, u32 cur_freq)
{
	return -ENODEV;
}
static inline
int register_exynos_dm_constraint_table(int dm_type,
				struct exynos_dm_constraint *constraint)
{
	return -ENODEV;
}
static inline
int register_exynos_dm_freq_scaler(int dm_type,
			int (*scaler_func)(int dm_type, void *devdata, u32 target_freq, unsigned int relation))
{
	return -ENODEV;
}
static inline
int unregister_exynos_dm_freq_scaler(int dm_type)
{
	return -ENODEV;
}
static inline
int policy_update_call_to_DM(int dm_type, u32 min_freq, u32 max_freq)
{
	return -ENODEV;
}
static inline
int DM_CALL(int dm_type, unsigned long *target_freq)
{
	return -ENODEV;
}
static inline
void exynos_dm_dynamic_disable(int flag)
{
	return;
}
static inline int exynos_dm_fast_switch_notifier_register(struct notifier_block *n)
{
	return 0;
}
static inline int exynos_dm_change_freq_table(struct exynos_dm_constraint *constraint, int idx)
{
	return -ENODEV;
}
#endif

#endif /* __EXYNOS_DM_H */
