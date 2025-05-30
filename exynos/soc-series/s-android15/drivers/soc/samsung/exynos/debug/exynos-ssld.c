// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Exynos - S2R Scenario Lockup Detector
 *
 */

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/cpuhotplug.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <linux/sched/clock.h>
#include <linux/preempt.h>
#include <linux/timer.h>
#include <uapi/linux/sched/types.h>
#include <trace/events/power.h>
#include <soc/samsung/exynos/exynos-ssld.h>

enum s2r_stage {
	ePM_RUNNING = 0,
	ePM_SUSPEND_PREPARE,
	ePM_DEV_SUSPEND_PREPARE,
	ePM_DEV_SUSPEND_NOIRQ,
	ePM_DEV_RESUME_NOIRQ,
	ePM_DEV_RESUME_COMPLETE,
	ePM_POST_SUSPEND,
	ePM_CNT,
};

struct suspend_device {
	u64 time;
	struct device *dev;
	const char *pm_ops;
	int event;
	bool start;
};

struct ssld_descriptor {
	enum s2r_stage s2r_stage;
	const char *s2r_stage_name[ePM_CNT];
	struct timer_list suspend_timer;
	struct task_struct *s2r_leading_tsk;
	u64 pm_prepare_jiffies;
	struct s2r_trace_info last_info;
	struct notifier_block pre_s2r_lockup_detector_pm_nb;
	struct notifier_block post_s2r_lockup_detector_pm_nb;

	unsigned long reboot_stage;
	struct timer_list reboot_timer;
	struct task_struct *reboot_leading_tsk;
	u64 reboot_jiffies;
	u64 reboot_start_time_nsec;
	struct notifier_block reboot_lockup_detector_nb;

	struct suspend_device *sus_dev;
	atomic_t suspend_idx;
	unsigned int num_idx;
	atomic_t num_suspend;
	struct notifier_block exynos_ssld_lock_check_nb;

	size_t panic_msg_offset;
	char panic_msg[1024];
};

static unsigned int threshold = 25;
module_param(threshold, uint, 0644);
#define SUS_DEV_DEFAULT_NUM 128
ATOMIC_NOTIFIER_HEAD(ssld_notifier_list);
ATOMIC_NOTIFIER_HEAD(ssld_reboot_notifier_list);

#if IS_ENABLED(CONFIG_EXYNOS_HARDLOCKUP_WATCHDOG)
extern struct atomic_notifier_head hardlockup_notifier_list;
#else
static struct atomic_notifier_head hardlockup_notifier_list;
#endif

void ssld_notifier_chain_register(struct notifier_block *nb)
{
	atomic_notifier_chain_register(&ssld_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(ssld_notifier_chain_register);

void ssld_reboot_notifier_chain_register(struct notifier_block *nb)
{
	atomic_notifier_chain_register(&ssld_reboot_notifier_list, nb);
}
EXPORT_SYMBOL_GPL(ssld_reboot_notifier_chain_register);

static int __maybe_unused sec_from_saved_jiffies(u64 from)
{
	if (from > jiffies)
		return 0;
	else
		return (jiffies - from) / HZ;
}

static int s2r_lockup_detector_prepare(struct device *dev)
{
	struct ssld_descriptor *desc;

	desc = dev_get_drvdata(dev);
	desc->s2r_stage = ePM_DEV_SUSPEND_PREPARE;
	return 0;
}

static int s2r_lockup_detector_suspend_noirq(struct device *dev)
{
	struct ssld_descriptor *desc;

	desc = dev_get_drvdata(dev);
	desc->s2r_stage = ePM_DEV_SUSPEND_NOIRQ;
	return 0;
}

static int s2r_lockup_detector_resume_noirq(struct device *dev)
{
	struct ssld_descriptor *desc;

	desc = dev_get_drvdata(dev);
	desc->s2r_stage = ePM_DEV_RESUME_NOIRQ;
	return 0;
}

static void s2r_lockup_detector_complete(struct device *dev)
{
	struct ssld_descriptor *desc;

	desc = dev_get_drvdata(dev);
	desc->s2r_stage = ePM_DEV_RESUME_COMPLETE;
}

static const struct dev_pm_ops __maybe_unused s2r_lockup_detector_pm_ops = {
	.prepare = s2r_lockup_detector_prepare,
	.suspend_noirq = s2r_lockup_detector_suspend_noirq,
	.resume_noirq = s2r_lockup_detector_resume_noirq,
	.complete = s2r_lockup_detector_complete,
};

static int pre_s2r_lockup_detector_pm_notifier(struct notifier_block *notifier,
						unsigned long pm_event, void *v)
{
	struct ssld_descriptor *desc = container_of(notifier, struct ssld_descriptor,
							pre_s2r_lockup_detector_pm_nb);

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		desc->s2r_stage = ePM_SUSPEND_PREPARE;
		desc->pm_prepare_jiffies = jiffies;
		desc->s2r_leading_tsk = current;
		if (timer_pending(&desc->suspend_timer)) {
			mod_timer(&desc->suspend_timer, jiffies + HZ * threshold);
		} else {
			desc->suspend_timer.expires = jiffies + HZ * threshold;
			add_timer(&desc->suspend_timer);
		}
		break;
	case PM_POST_SUSPEND:
		if (desc->s2r_stage != ePM_DEV_RESUME_COMPLETE) {
			del_timer_sync(&desc->suspend_timer);
			desc->s2r_stage = ePM_RUNNING;
		}
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int post_s2r_lockup_detector_pm_notifier(struct notifier_block *notifier,
						unsigned long pm_event, void *v)
{
	struct ssld_descriptor *desc = container_of(notifier, struct ssld_descriptor,
							post_s2r_lockup_detector_pm_nb);

	switch (pm_event) {
	case PM_POST_SUSPEND:
		if (timer_pending(&desc->suspend_timer))
			del_timer_sync(&desc->suspend_timer);
		desc->s2r_stage = ePM_RUNNING;
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int reboot_lockup_detector_notifier(struct notifier_block *notifier,
					   unsigned long stage, void *v)
{
	struct ssld_descriptor *desc = container_of(notifier, struct ssld_descriptor,
							reboot_lockup_detector_nb);

	switch (stage) {
	case SYS_RESTART:
	case SYS_POWER_OFF:
		if (timer_pending(&desc->reboot_timer))
			break;
		desc->reboot_stage = stage;
		desc->reboot_jiffies = jiffies;
		desc->reboot_start_time_nsec = local_clock();
		desc->reboot_leading_tsk = current;
		desc->reboot_timer.expires = jiffies + HZ * threshold;
		add_timer(&desc->reboot_timer);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static void print_log_to_buffer(struct ssld_descriptor *desc, const char *fmt, ...)
{
	va_list args;

	if (desc->panic_msg_offset >= (sizeof(desc->panic_msg) - 1)) {
		pr_emerg("%s: buffer is full\n", __func__);
		return;
	}

	va_start(args, fmt);
	desc->panic_msg_offset += vscnprintf(desc->panic_msg + desc->panic_msg_offset,
						sizeof(desc->panic_msg) - desc->panic_msg_offset,
						fmt, args);
	va_end(args);
}

static void print_last_suspend_resume_info(struct ssld_descriptor *desc)
{
	struct s2r_trace_info *info = &desc->last_info;

	if (info->start) {
		if (!!info->dev) {
			print_log_to_buffer(desc, "DPM dev timeout[%s],",
						dev_name(info->dev));
			print_log_to_buffer(desc, "time = %lu usec|pm_ops(%s, %d)",
						info->time / 1000, info->pm_ops, info->event);
		} else {
			print_log_to_buffer(desc, "S2R timeout,");
			print_log_to_buffer(desc, "time = %lu usec|action(%s, %d)",
						info->time / 1000, info->action, info->val);
		}
	} else {
		print_log_to_buffer(desc, "Not DPM callback,");
		if (!!info->dev) {
			print_log_to_buffer(desc, "last DPM dev[%s](%d) time = %lu usec",
						dev_name(info->dev),
						info->error, info->time / 1000);
		} else {
			print_log_to_buffer(desc, "last pm info(%s, %d) time = %lu usec",
						info->action, info->val, info->time / 1000);
		}
	}
}

static void print_async_suspend_resume_info(struct ssld_descriptor *desc)
{
	unsigned int num_idx = desc->num_idx;
	unsigned long num_suspend = atomic_read(&desc->num_suspend);
	unsigned int i;
	unsigned long idx;
	unsigned long index;

	print_log_to_buffer(desc, "SSLD:");
	print_log_to_buffer(desc, "PendDevNum[%lu]:", num_suspend);
	idx = (atomic_read(&desc->suspend_idx) & (num_idx - 1));
	for (i = 0; i < num_idx; i++) {
		if (num_suspend == 0) {
			break;
		}
		index = (idx + i + num_idx) % num_idx;
		if (desc->sus_dev[index].start) {
			print_log_to_buffer(desc, "[%s][%llu][%s,%d] ",
			dev_name(desc->sus_dev[index].dev),
			desc->sus_dev[index].time / 1000,
			desc->sus_dev[index].pm_ops,
			desc->sus_dev[index].event);
			num_suspend--;
		}
	}
}

static void s2r_lockup_print(struct ssld_descriptor *desc, bool hardlockup)
{
	if (desc->s2r_stage > ePM_RUNNING && desc->s2r_stage < ePM_POST_SUSPEND) {
		struct ssld_notifier_data nb_data;

		pr_emerg("Suspend/Resume hang between %s and %s\n",
					desc->s2r_stage_name[desc->s2r_stage],
					desc->s2r_stage_name[desc->s2r_stage + 1]);
		pr_emerg("curr jiffies(%lu) pm_prepare_jiffies (%llu) expires jiffies(%lu)\n",
					jiffies, desc->pm_prepare_jiffies, desc->suspend_timer.expires);
		pr_emerg("Suspend/Resume hang detected\n");
		print_async_suspend_resume_info(desc);
		print_last_suspend_resume_info(desc);
		print_log_to_buffer(desc, "[%s, %d]",
					desc->s2r_leading_tsk->comm, desc->s2r_leading_tsk->pid);

		nb_data.s2r_leading_tsk = desc->s2r_leading_tsk;
		nb_data.last_info = &desc->last_info;
		nb_data.pm_prepare_jiffies = desc->pm_prepare_jiffies;
		atomic_notifier_call_chain(&ssld_notifier_list, 0, &nb_data);
		if(!hardlockup)
			panic("%s", desc->panic_msg);
	} else {
		pr_emerg("Not Suspend/Resume hang");
	}
}

static void s2r_lockup_detector_handler(struct timer_list *t)
{
	struct ssld_descriptor *desc = container_of(t, struct ssld_descriptor, suspend_timer);

	s2r_lockup_print(desc, false);
}

static void ssld_suspend_resume(void *data, const char *action, int val, bool start)
{
	struct ssld_descriptor *desc = (struct ssld_descriptor *)data;

	desc->last_info.time = local_clock();
	desc->last_info.dev = NULL;
	desc->last_info.action = action;
	desc->last_info.val = val;
	desc->last_info.start = start;
}

static void ssld_dev_pm_cb_start(void *data, struct device *dev, const char *pm_ops, int event)
{
	struct ssld_descriptor *desc = (struct ssld_descriptor *)data;
	unsigned int write = 0;
	unsigned int num_idx = desc->num_idx;
	unsigned long idx;
	unsigned int i;

	atomic_inc(&desc->num_suspend);
	idx = (atomic_fetch_inc(&desc->suspend_idx) & (num_idx - 1));
	for (i = 0; i < num_idx; i++) {
		if (!desc->sus_dev[idx].start) {
			desc->sus_dev[idx].time = local_clock();
			desc->sus_dev[idx].dev = dev;
			desc->sus_dev[idx].start = true;
			desc->sus_dev[idx].pm_ops = pm_ops;
			desc->sus_dev[idx].event = event;
			write = 1;
			break;
		} else {
			idx = (atomic_fetch_inc(&desc->suspend_idx) & (num_idx - 1));
		}
	}
	if (write == 0)
		pr_err("Cannot write to ssld sus_dev array\n");


	desc->last_info.time = local_clock();
	desc->last_info.dev = dev;
	desc->last_info.pm_ops = pm_ops;
	desc->last_info.event = event;
	desc->last_info.start = true;
}

static void ssld_dev_pm_cb_end(void *data, struct device *dev, int error)
{
	struct ssld_descriptor *desc = (struct ssld_descriptor *)data;
	unsigned int num_idx = desc->num_idx;
	unsigned int i;
	unsigned long idx;
	unsigned long index;

	atomic_dec(&desc->num_suspend);
	idx = (atomic_read(&desc->suspend_idx) & (num_idx - 1));
	for (i = 0; i < num_idx; i++) {
		index = (idx - (i + 1) + num_idx) % num_idx;
		if (desc->sus_dev[index].start) {
			if (desc->sus_dev[index].dev == dev) {
				desc->sus_dev[index].start = false;
				desc->sus_dev[index].dev = NULL;
				desc->sus_dev[index].pm_ops = NULL;
				break;
			}
		}
	}

	desc->last_info.time = local_clock();
	desc->last_info.dev = dev;
	desc->last_info.pm_ops = NULL;
	desc->last_info.error = error;
	desc->last_info.start = false;
}

static int exynos_ssld_lock_check_notifier(struct notifier_block *notifier,
					unsigned long l, void *core)
{
	struct ssld_descriptor *desc = container_of(notifier, struct ssld_descriptor,
							exynos_ssld_lock_check_nb);

	s2r_lockup_print(desc, true);
	pr_emerg("%s", desc->panic_msg);
	return 0;
}

static void s2r_lockup_detector_resource_init(struct ssld_descriptor *desc)
{
	struct device_node *np;

	desc->num_idx = SUS_DEV_DEFAULT_NUM;
	np = of_find_compatible_node(NULL, NULL, "samsung,exynos-ssld");
	if (np == NULL) {
		pr_err("Fail to find exynos ssld node\n");
	} else {
		if (of_property_read_u32(np, "nr_idx", &desc->num_idx))
			pr_err("No nr_idx data in ssld\n");
	}

	desc->s2r_stage_name[ePM_RUNNING] = "RUNNING";
	desc->s2r_stage_name[ePM_SUSPEND_PREPARE] = "PM_SUSPEND_PREPARE";
	desc->s2r_stage_name[ePM_DEV_SUSPEND_PREPARE] = "DEV_SUSPEND_PREPARE";
	desc->s2r_stage_name[ePM_DEV_SUSPEND_NOIRQ] = "DEV_SUSPEND_NOIRQ";
	desc->s2r_stage_name[ePM_DEV_RESUME_NOIRQ] = "DEV_RESUME_NOIRQ";
	desc->s2r_stage_name[ePM_DEV_RESUME_COMPLETE] = "DEV_RESUME_COMPLETE";
	desc->s2r_stage_name[ePM_POST_SUSPEND] = "PM_POST_SUSPEND";

	atomic_set(&desc->suspend_idx, 0);
	atomic_set(&desc->num_suspend, 0);

	desc->pre_s2r_lockup_detector_pm_nb.notifier_call = pre_s2r_lockup_detector_pm_notifier;
	desc->pre_s2r_lockup_detector_pm_nb.priority = INT_MAX;
	register_pm_notifier(&desc->pre_s2r_lockup_detector_pm_nb);

	desc->post_s2r_lockup_detector_pm_nb.notifier_call = post_s2r_lockup_detector_pm_notifier;
	desc->post_s2r_lockup_detector_pm_nb.priority = INT_MIN;
	register_pm_notifier(&desc->post_s2r_lockup_detector_pm_nb);

	desc->s2r_stage = ePM_RUNNING;
	timer_setup(&desc->suspend_timer, s2r_lockup_detector_handler, 0);

	register_trace_suspend_resume(ssld_suspend_resume, desc);
	register_trace_device_pm_callback_start(ssld_dev_pm_cb_start, desc);
	register_trace_device_pm_callback_end(ssld_dev_pm_cb_end, desc);

	desc->exynos_ssld_lock_check_nb.notifier_call = exynos_ssld_lock_check_notifier;
	desc->exynos_ssld_lock_check_nb.priority = INT_MIN;
	atomic_notifier_chain_register(&hardlockup_notifier_list,
			&desc->exynos_ssld_lock_check_nb);
}

static void reboot_lockup_detector_handler(struct timer_list *t)
{
	struct ssld_descriptor *desc = container_of(t, struct ssld_descriptor, reboot_timer);
	struct ssld_reboot_notifier_data nb_data;
	const char *stage;

	switch (desc->reboot_stage) {
	case SYS_RESTART:
		stage = "RESTART";
		break;
	case SYS_POWER_OFF:
		stage = "POWER OFF";
		break;
	default:
		stage = "INVALID STAGE";
		break;
	}

	pr_emerg("%s hang, curr jiffies(%lu) reboot_jiffies(%llu)\n",
					stage, jiffies, desc->reboot_jiffies);
	print_log_to_buffer(desc, "%s hang detected[%s, %d] reboot started at [%lu nsec]",
						stage, desc->reboot_leading_tsk->comm,
						desc->reboot_leading_tsk->pid,
						desc->reboot_start_time_nsec);

	nb_data.reboot_leading_tsk = desc->reboot_leading_tsk;
	nb_data.reboot_stage = desc->reboot_stage;
	nb_data.reboot_jiffies = desc->reboot_jiffies;
	nb_data.reboot_start_time_nsec = desc->reboot_start_time_nsec;
	atomic_notifier_call_chain(&ssld_reboot_notifier_list, 0, &nb_data);

	panic("%s", desc->panic_msg);
}

static void reboot_lockup_detector_resource_init(struct ssld_descriptor *desc)
{
	desc->reboot_lockup_detector_nb.notifier_call = reboot_lockup_detector_notifier;
	desc->reboot_lockup_detector_nb.priority = INT_MIN;
	register_reboot_notifier(&desc->reboot_lockup_detector_nb);

	timer_setup(&desc->reboot_timer, reboot_lockup_detector_handler, 0);
}

static struct bus_type ssld_bus_type = {
	.name = "ssld",
};

static struct device_driver s2r_lockup_detector_driver = {
	.name = "ssld_drv",
	.bus = &ssld_bus_type,
	.pm = &s2r_lockup_detector_pm_ops,
};

static struct device s2r_lockup_detector_device = {
	.init_name = "ssld_dev",
	.bus = &ssld_bus_type,
};

static int __init s2r_lockup_detector_driver_init(void)
{
	struct ssld_descriptor *ssld_desc;
	int ret;

	ret = bus_register(&ssld_bus_type);
	if (ret)
		goto bus_register_err;

	ret = driver_register(&s2r_lockup_detector_driver);
	if (ret < 0)
		goto driver_register_err;

	ret = device_register(&s2r_lockup_detector_device);
	if (ret < 0)
		goto device_register_err;

	ssld_desc = devm_kzalloc(&s2r_lockup_detector_device,
				sizeof(struct ssld_descriptor), GFP_KERNEL);
	if (!ssld_desc) {
		ret = -ENOMEM;
		goto alloc_fail;
	}

	dev_set_drvdata(&s2r_lockup_detector_device, ssld_desc);
	s2r_lockup_detector_resource_init(ssld_desc);
	ssld_desc->sus_dev = devm_kzalloc(&s2r_lockup_detector_device,
				sizeof(struct suspend_device) * ssld_desc->num_idx, GFP_KERNEL);
	if (!ssld_desc->sus_dev) {
		ret = -ENOMEM;
		goto alloc_fail;
	}

	reboot_lockup_detector_resource_init(ssld_desc);
	pr_info("ssld lockup detector is enabled(threshold = %u)\n", threshold);
	return 0;

alloc_fail:
	device_unregister(&s2r_lockup_detector_device);
device_register_err:
	driver_unregister(&s2r_lockup_detector_driver);
driver_register_err:
	bus_unregister(&ssld_bus_type);
bus_register_err:
	return ret;
}

static void __exit s2r_lockup_detector_driver_exit(void)
{
	struct ssld_descriptor *desc = dev_get_drvdata(&s2r_lockup_detector_device);

	unregister_trace_suspend_resume(ssld_suspend_resume, desc);
	unregister_trace_device_pm_callback_start(ssld_dev_pm_cb_start, desc);
	unregister_trace_device_pm_callback_end(ssld_dev_pm_cb_end, desc);

	unregister_pm_notifier(&desc->pre_s2r_lockup_detector_pm_nb);
	unregister_pm_notifier(&desc->post_s2r_lockup_detector_pm_nb);
	unregister_reboot_notifier(&desc->reboot_lockup_detector_nb);

	del_timer(&desc->suspend_timer);
	del_timer(&desc->reboot_timer);
	device_unregister(&s2r_lockup_detector_device);
	driver_unregister(&s2r_lockup_detector_driver);
	bus_unregister(&ssld_bus_type);
}

module_init(s2r_lockup_detector_driver_init);
module_exit(s2r_lockup_detector_driver_exit);

MODULE_DESCRIPTION("Samsung Exynos S2R Scenario Lockup Detector");
MODULE_LICENSE("GPL v2");
