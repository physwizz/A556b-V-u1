// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "amdgpu.h"
#include "sgpu_governor.h"
#include "exynos_gpu_interface.h"
#include <linux/pm_runtime.h>
#if defined(CONFIG_GPU_VERSION_M3)
#include <linux/notifier.h>
#include <linux/pmic/s2p.h>

static void sgpu_afm_set_warn_level(struct sgpu_afm_domain *afm_dom)
{
	int warn_level;
	u8 val;

	afm_dom->pmic_read_reg(afm_dom->num, afm_dom->s2mps_afm_level_offset, &val);
	val &= S2MPS_AFM_WARN_LVL_MASK;
	warn_level = (int)val - OCP_OI_WARN_LVL_DOWN;

	if (warn_level > S2MPS_AFM_WARN_LVL_MASK || warn_level < 0) {
		DRM_ERROR("Invalid warn_level %d\n", warn_level);
		return;
	}

	afm_dom->pmic_update_reg(afm_dom->num, afm_dom->s2mps_afm_level_offset,
				 (uint32_t)warn_level, S2MPS_AFM_WARN_LVL_MASK);

	/*
	 * ocp_oi_throttle can be applied only once after booting.
	 * ocp_oi_throttle is true means that throttle has already been applied.
	 */
	afm_dom->ocp_oi_throttle = true;

	DRM_INFO("change warn_level %u -> %d\n", val, warn_level);

	return;
}

static int sgpu_afm_ocp_oi_notifier_handler(struct notifier_block *nb,
					    unsigned long action, void *data)
{
	struct sgpu_afm_domain *afm_dom  = container_of(nb,
				       struct sgpu_afm_domain, ocp_oi_notifier);
	struct s2p_ocp_oi_data *ocp_oi_data = data;

	if (strcmp(ocp_oi_data->name, PMIC_OCP_IRQ) && strcmp(ocp_oi_data->name,
	    PMIC_OI_IRQ))
		return IRQ_NONE;

	if (!afm_dom->ocp_oi_throttle)
		sgpu_afm_set_warn_level(afm_dom);

	return IRQ_HANDLED;
}

static int sgpu_afm_ocp_oi_set_notifier(struct sgpu_afm_domain *afm_dom)
{
	int ret = 0;

	afm_dom->ocp_oi_notifier.notifier_call = sgpu_afm_ocp_oi_notifier_handler;

	ret = s2p_register_ocp_oi_notifier(&afm_dom->ocp_oi_notifier);
	if (ret < 0)
		DRM_ERROR("fail to register ocp oi notifier\n");

	return ret;
}
#else
static int sgpu_afm_ocp_oi_set_notifier(struct sgpu_afm_domain *afm_dom)
{
	return 0;
}
#endif /* CONFIG_GPU_VERSION_M3 */

static void sgpu_afm_profile_start_ipi(void *arg)
{
	struct sgpu_afm_domain *afm_dom = arg;
	struct amdgpu_device *adev = afm_dom->adev;
	int ret;

	if (atomic_inc_return(&afm_dom->profile_cnt) > 1) {
		afm_dom->throttle_cnt = readl(afm_dom->base + OCPTHROTTCNTM);
		afm_dom->throttle_cnt &= ~(1 << 31);
		return;
	}

	if (adev->runpm) {
		ret = pm_runtime_get_sync(adev->dev);
		if (ret < 0) {
			atomic_dec(&afm_dom->profile_cnt);
			pm_runtime_put(adev->dev);
			return;
		}
	}

	/* Enable OCP Throttle Accumulation Counter (TAC) */
	writel((1 << 31), afm_dom->base + OCPTHROTTCNTM);
	afm_dom->throttle_cnt = 0;
}

static void sgpu_afm_profile_end_ipi(void *arg)
{
	struct sgpu_afm_domain *afm_dom = arg;
	struct amdgpu_device *adev = afm_dom->adev;

	if (atomic_read(&afm_dom->profile_cnt) == 0)
		return;

	afm_dom->throttle_cnt = readl(afm_dom->base + OCPTHROTTCNTM);
	afm_dom->throttle_cnt &= ~(1 << 31);

	if (!atomic_dec_return(&afm_dom->profile_cnt)) {
		/* Disable OCP Throttle Accumulation Counter (TAC) */
		writel(0x0, afm_dom->base + OCPTHROTTCNTM);

		if (adev->runpm)
			pm_runtime_put(adev->dev);
	}
}

static void sgpu_afm_control_interrupt(struct sgpu_afm_domain *afm_dom, bool enable)
{
	int val = 0;

	val = readl(afm_dom->base + OCPTHROTTCNTA);
	/* Enable/Disable OCP S/W interrupt */
	if (enable)
		val |= OCPTHROTTERRAEN_VALUE | TDCEN_VALUE;
	else
		val &= ~(OCPTHROTTERRAEN_VALUE | TDCEN_VALUE);

	writel(val, afm_dom->base + OCPTHROTTCNTA);
}

static void sgpu_afm_init_htu(struct sgpu_afm_domain *afm_dom)

{
	int irp, pwrthresh, reactor, val;

	/* Integration Resolution Periods (IRP) */
	irp = (afm_dom->max_freq * 2) / afm_dom->min_freq - 2;
	irp = min(irp, IRP_MASK);

	/* update OCPTOPPWRTHRESH IRP field */
	pwrthresh = readl(afm_dom->base + OCPTOPPWRTHRESH);
	pwrthresh &= ~(IRP_MASK << IRP_SHIFT);
	pwrthresh |= (irp << IRP_SHIFT);

	/* disable OCP Controller before change IRP */
	reactor = readl(afm_dom->base + OCPMCTL);
	reactor &= ~OCPMCTL_EN_VALUE;
	writel(reactor, afm_dom->base + OCPMCTL);

	/* IRQ value update */
	writel(pwrthresh, afm_dom->base + OCPTOPPWRTHRESH);

	/* enable OCP controller after change IRP */
	reactor |= OCPMCTL_EN_VALUE;
	writel(reactor, afm_dom->base + OCPMCTL);

	/* update TDT == 1 (Threshold for TDC) */
	val = readl(afm_dom->base + OCPTHROTTCNTA);
	val |= (0x1<<20);
	writel(val, afm_dom->base + OCPTHROTTCNTA);

	/* Enable GLOBALTHROTTEN.HIU1ST */
	val = readl(afm_dom->base + GLOBALTHROTTEN);
	val |= (0x1<<5);
	writel(val, afm_dom->base + GLOBALTHROTTEN);

	sgpu_afm_control_interrupt(afm_dom, true);
}

static void sgpu_afm_clear_interrupt(struct sgpu_afm_domain *afm_dom)
{
	int val;

	/* Write 1 to clear the OCP S/W interrupt  */
	val = readl(afm_dom->base + OCPTHROTTCTL);
	val |= OCPTHROTTERRA_VALUE;
	writel(val, afm_dom->base + OCPTHROTTCTL);
	val &= ~OCPTHROTTERRA_VALUE;
	writel(val, afm_dom->base + OCPTHROTTCTL);
}

static void sgpu_afm_clear_throttling_duration_counter(struct sgpu_afm_domain *afm_dom)
{
	int val;

	val = readl(afm_dom->base + OCPTHROTTCNTA);
	val &= ~TDC_MASK;
	writel(val, afm_dom->base + OCPTHROTTCNTA);
}

static void sgpu_afm_update_status(struct sgpu_afm_domain *afm_dom)
{
	struct amdgpu_device *adev = afm_dom->adev;
	struct devfreq *df = adev->devfreq;
	unsigned int i, level = afm_dom->last_level;
	u64 cur_time = get_jiffies_64();

	afm_dom->time_in_state[level] += cur_time - afm_dom->last_time;
	afm_dom->last_time = cur_time;

	for (i = 0; i < df->profile->max_state; i++)
		if (afm_dom->clipped_freq == df->profile->freq_table[i])
			break;

	if (i != df->profile->max_state) {
		if (afm_dom->last_level != i)
			afm_dom->total_trans++;
		afm_dom->last_level = i;
	}
}

static void set_afm_max_limit(struct sgpu_afm_domain *afm_dom, bool set_max_limit)
{
	if (set_max_limit)
		afm_dom->clipped_freq = gpu_afm_decrease_maxlock(afm_dom->down_step);
	else
		afm_dom->clipped_freq = gpu_afm_release_maxlock();

	sgpu_afm_update_status(afm_dom);
}

static void sgpu_afm_work(struct work_struct *work)
{
	struct sgpu_afm_domain *afm_dom = container_of(work, struct sgpu_afm_domain,
						       afm_work);
	struct amdgpu_device *adev = afm_dom->adev;

	set_afm_max_limit(afm_dom, SET_MAX_LIMIT);

	/* Call delayed work */
	mod_delayed_work(system_wq, &afm_dom->release_dwork,
			 msecs_to_jiffies(afm_dom->release_duration));

	sgpu_afm_control_interrupt(afm_dom, true);
	afm_dom->flag = false;

	pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);
}

static void sgpu_afm_work_release(struct work_struct *work)
{
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct sgpu_afm_domain *delay_afm_dom = container_of(dw, struct sgpu_afm_domain,
							     release_dwork);

	set_afm_max_limit(delay_afm_dom, RELEASE_MAX_LIMIT);
}

static irqreturn_t sgpu_afm_irq_handler(int irq, void *arg)
{
	struct sgpu_afm_domain *afm_dom = (struct sgpu_afm_domain *)arg;
	struct amdgpu_device *adev = afm_dom->adev;
	int need_pm_put = 0;

	if (adev->in_runpm)
		return IRQ_HANDLED;

	if (pm_runtime_get_if_active(adev_to_drm(adev)->dev, true)) {
		/* Schedule set max lock work */
		if (!afm_dom->flag) {
			afm_dom->flag = true;
			sgpu_afm_control_interrupt(afm_dom, false);
			schedule_work(&afm_dom->afm_work);
		} else
			need_pm_put = 1;

		/* Clear IRQ */
		sgpu_afm_clear_throttling_duration_counter(afm_dom);
		sgpu_afm_clear_interrupt(afm_dom);

		if (need_pm_put)
			pm_runtime_put_autosuspend(adev_to_drm(adev)->dev);
	} else
		dev_info(adev->dev, "Skip gpu AFM interrupt clear\n");

	return IRQ_HANDLED;
}

static void sgpu_afm_register(struct work_struct *work)
{
	int ret = 0;
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct sgpu_afm_domain *afm_dom = container_of(dw, struct sgpu_afm_domain,
						       register_dwork);
	struct amdgpu_device *adev = afm_dom->adev;

	/* Check if the pmic_i2c is initialized */
	if (!afm_dom->pmic_check_info(afm_dom->num)) {
		DRM_INFO("pmic info get failed\n");
		schedule_delayed_work(&afm_dom->register_dwork,
				      msecs_to_jiffies(afm_dom->register_duration));
	} else {
		if (adev->runpm) {
			ret = pm_runtime_get_sync(adev->dev);
			if (ret < 0)
				goto pm_put;
		}

		/* TODO : Read PMIC chip-revision indicating AFM_WARN trim valid */
		ret = devm_request_irq(adev->dev, afm_dom->irq,
				       sgpu_afm_irq_handler,
				       IRQF_TRIGGER_HIGH | IRQF_SHARED,
				       dev_name(adev->dev),
				       afm_dom);
		if (ret) {
			DRM_ERROR("Failed to request IRQ handler for GPU AFM: %d\n",
				  afm_dom->irq);
			goto pm_put;
		}

		sgpu_afm_init_htu(afm_dom);
		sgpu_afm_ocp_oi_set_notifier(afm_dom);

		adev->enable_afm = true;
		DRM_INFO("exynos-afm: AFM data(gpu) structure update complete");

pm_put:
		if (adev->runpm) {
			pm_runtime_put(adev->dev);
		}

	}
}


/**
 * enabled_show() - Read AFM_WARN enable field
 *
 * Read the register field that indicates AFM_WARN enable.
 */
static ssize_t enabled_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	u8 val;

	afm_dom->pmic_read_reg(afm_dom->num, afm_dom->s2mps_afm_enable_offset, &val);
	val &= 1 << S2MPS_AFM_WARN_EN_SHIFT;
	return scnprintf(buf, PAGE_SIZE, "%u\n", !!val);
}

/**
 * enabled_store() - Write to AFM_WARN enable field
 *
 * Write input value to the register field that indicates AFM_WARN enable.
 */
static ssize_t enabled_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	uint32_t val;

	if (kstrtou32(buf, 0, &val))
		return -EINVAL;

	if (val > 0x1)
		return -EINVAL;

	afm_dom->pmic_update_reg(afm_dom->num, afm_dom->s2mps_afm_enable_offset,
				 ((val) << S2MPS_AFM_WARN_EN_SHIFT),
				 (1 << S2MPS_AFM_WARN_EN_SHIFT));

	return count;
}
static DEVICE_ATTR_RW(enabled);

/**
 * warn_level_show() - Write to AFM_WARN enable field
 *
 * Read the register field that indicates AFM_WARN_LVL.
 */
static ssize_t warn_level_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	u8 val;

	afm_dom->pmic_read_reg(afm_dom->num, afm_dom->s2mps_afm_level_offset, &val);
	val &= S2MPS_AFM_WARN_LVL_MASK;
	return scnprintf(buf, PAGE_SIZE, "%#x\n", val);
}

/**
 * warn_level_store() - Read AFM_WARN_LVL field
 *
 * Write input value to the register field that indicates AFM_WARN_LVL.
 */
static ssize_t warn_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	uint32_t val;

	if (kstrtou32(buf, 0, &val))
		return -EINVAL;

	if (val > S2MPS_AFM_WARN_LVL_MASK)
		return -EINVAL;

	afm_dom->pmic_update_reg(afm_dom->num, afm_dom->s2mps_afm_level_offset,
				 val, S2MPS_AFM_WARN_LVL_MASK);

	return count;
}
static DEVICE_ATTR_RW(warn_level);

/**
 * clipped_freq_show() - Show the clipped_freq.
 *
 * clipped_freq indicates GPU AFM's max frequency lock value. This shows
 * current clipped_freq.
 */
static ssize_t clipped_freq_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;

	return scnprintf(buf, PAGE_SIZE, "%lu\n", afm_dom->clipped_freq);
}
static DEVICE_ATTR_RO(clipped_freq);

/**
 * release_duration_show() - Show the release_duration.
 *
 * release_duration is used at delayed_work that release GPU AFM's max
 * frequency lock if AFM_WARN interrupt doesn't occur during
 * release_duration(ms).  This show the value of release_duration.
 */
static ssize_t release_duration_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;

	return scnprintf(buf, PAGE_SIZE, "%u\n", afm_dom->release_duration);
}

/**
 * release_duration_store() - Update release_duration.
 *
 * Write input value to release_duration.
 */
static ssize_t release_duration_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	uint32_t val;

	if (kstrtou32(buf, 0, &val))
		return -EINVAL;

	afm_dom->release_duration = val;

	return count;
}
static DEVICE_ATTR_RW(release_duration);

/**
 * down_step_show() - Show the down_step.
 *
 * When AFM_WARN interrupted, GPU AFM feature locks the GPU's maxfrequency.
 * down_step indicates how many steps GPU frequency will be lowered.
 */
static ssize_t down_step_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;

	return scnprintf(buf, PAGE_SIZE, "%u\n", afm_dom->down_step);
}

/**
 * down_step_store() - Update down_step.
 *
 * Write input value to down_step.
 */
static ssize_t down_step_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	struct devfreq *df = adev->devfreq;
	uint32_t val;

	if (kstrtou32(buf, 0, &val))
		return -EINVAL;

	if (val < 1 || val >= df->profile->max_state)
		return -EINVAL;

	afm_dom->down_step = val;

	return count;
}
static DEVICE_ATTR_RW(down_step);

/**
 * total_trans_show() - Show the total_trans.
 *
 * total_trans counts how many clipped_freq was transitted. This is a node
 * that can check whether AFM_WARN occurs after boot.
 */
static ssize_t total_trans_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;

	return scnprintf(buf, PAGE_SIZE, "%u\n", afm_dom->total_trans);
}
static DEVICE_ATTR_RO(total_trans);

/**
 * time_in_state_show() - Update statistics of clipped_freq behavior
 *
 * This node show the table of clipped_freq hehavior. Each line has a
 * frequnecy and the time it stayed at that frequency as a pair.
 */
static ssize_t time_in_state_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	struct devfreq *df = adev->devfreq;
	unsigned int i;
	ssize_t len = 0;

	sgpu_afm_update_status(afm_dom);

	len += scnprintf(buf + len, PAGE_SIZE - len, "    FREQ    TIME(ms)\n");
	for (i = 0; i < df->profile->max_state; i++) {
		len += scnprintf(buf + len, PAGE_SIZE - len, "%8lu  %10llu\n",
				 df->profile->freq_table[i],
				 jiffies64_to_msecs(afm_dom->time_in_state[i]));
	}

	return len;
}
static DEVICE_ATTR_RO(time_in_state);

/**
 * profile_show() - Show the profile.
 *
 * Read current OCP Throttle Accumulation Counter (TAC).
 * TAC counts the total throttling durations in terms of the number of IRPs.
 */
static ssize_t profile_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	int ret, cur_cnt;

	if (!atomic_read(&afm_dom->profile_cnt))
		ret = scnprintf(buf, PAGE_SIZE, "%d\n", afm_dom->throttle_cnt);
	else {
		cur_cnt = readl(afm_dom->base + OCPTHROTTCNTM);
		cur_cnt &= ~(1 << 31);
		ret = scnprintf(buf, PAGE_SIZE, "%d\n", cur_cnt);
	}
	return ret;
}

/**
 * profile_store() - store the profile
 *
 * profile start : write 1 to OCPTHROTTCNTM EN bit
 * profile end : write 0 to OCPTHROTTCNTM EN bit
 */
static ssize_t profile_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct drm_device *ddev = dev_get_drvdata(dev->parent);
	struct amdgpu_device *adev = drm_to_adev(ddev);
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	uint32_t val;

	if (kstrtou32(buf, 0, &val))
		return -EINVAL;

	if (val)
		sgpu_afm_profile_start_ipi(afm_dom);
	else
		sgpu_afm_profile_end_ipi(afm_dom);

	return count;
}
static DEVICE_ATTR_RW(profile);

static struct attribute *sgpu_afm_sysfs_entries[] = {
	&dev_attr_enabled.attr,
	&dev_attr_warn_level.attr,
	&dev_attr_clipped_freq.attr,
	&dev_attr_release_duration.attr,
	&dev_attr_down_step.attr,
	&dev_attr_total_trans.attr,
	&dev_attr_time_in_state.attr,
	&dev_attr_profile.attr,
	NULL,
};

static struct attribute_group sgpu_afm_attr_group = {
	.name = "afm",
	.attrs = sgpu_afm_sysfs_entries,
};

static int sgpu_afm_create_sysfs_file(struct amdgpu_device *adev)
{
	struct devfreq *df = adev->devfreq;

	return sysfs_create_group(&df->dev.kobj, &sgpu_afm_attr_group);
}

static int sgpu_afm_dt_parsing(struct device_node *dn, struct sgpu_afm_domain *afm_dom)
{
	struct amdgpu_device *adev = afm_dom->adev;
	int ret = 0;

	afm_dom->base = devm_platform_ioremap_resource_byname(adev->pldev, "htu");
	if (IS_ERR(afm_dom->base))
		return PTR_ERR(afm_dom->base);

	ret |= of_property_read_u32(dn, "interrupt-src",
					&afm_dom->interrupt_src);
	ret |= of_property_read_u32(dn, "pmic-num", &afm_dom->num);
	ret |= of_property_read_u32(dn, "s2mps-afm-enable-offset",
					&afm_dom->s2mps_afm_enable_offset);
	ret |= of_property_read_u32(dn, "s2mps-afm-level-offset",
					&afm_dom->s2mps_afm_level_offset);
	ret |= of_property_read_u32(dn, "down-step", &afm_dom->down_step);
	ret |= of_property_read_u32(dn, "release-duration",
					&afm_dom->release_duration);
	ret |= of_property_read_u32(dn, "register-duration",
					&afm_dom->register_duration);

	return ret ? -EINVAL : 0;
}

static int sgpu_afm_param_init(struct amdgpu_device *adev,
				struct sgpu_afm_domain *afm_dom)
{
	struct devfreq *df = adev->devfreq;
	uint32_t max_level = 0, min_level = df->profile->max_state - 1;

	afm_dom->max_freq = df->profile->freq_table[max_level];
	afm_dom->min_freq = df->profile->freq_table[min_level];
	afm_dom->clipped_freq = afm_dom->max_freq;
	afm_dom->irq = adev->afm_irq;
	afm_dom->adev = adev;
	afm_dom->total_trans = 0;
	afm_dom->last_level = max_level;
	afm_dom->last_time = get_jiffies_64();
	afm_dom->throttle_cnt = 0;
	atomic_set(&afm_dom->profile_cnt, 0);


	afm_dom->time_in_state = kcalloc(df->profile->max_state,
					 sizeof(*afm_dom->time_in_state),
					 GFP_KERNEL);
	if (!afm_dom->time_in_state)
		return -ENOMEM;

	return 0;
}

static int sgpu_afm_alloc_pmic_function(struct sgpu_afm_domain *afm_dom)
{
	switch (afm_dom->interrupt_src) {
	case MAIN_PMIC:
		afm_dom->pmic_update_reg = main_pmic_afm_update_reg;
		afm_dom->pmic_check_info = chk_main_pmic_info;
		afm_dom->pmic_read_reg = main_pmic_afm_read_reg;
		break;
	case SUB_PMIC:
		afm_dom->pmic_update_reg = sub_pmic_afm_update_reg;
		afm_dom->pmic_check_info = chk_sub_pmic_info;
		afm_dom->pmic_read_reg = sub_pmic_afm_read_reg;
		break;
	default:
		DRM_ERROR("%s: There is no init pmic function (src=%u)"
			  , __func__, afm_dom->interrupt_src);
		return -ENODEV;
	}

	return 0;
}

int sgpu_afm_init(struct amdgpu_device *adev)
{
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;
	int ret = 0;

	/* GPU AFM initialization */
	ret = sgpu_afm_param_init(adev, afm_dom);
	if (ret) {
		DRM_ERROR("Failed to init AFM param\n");
		return ret;
	}

	ret = sgpu_afm_dt_parsing(adev->pldev->dev.of_node, afm_dom);
	if (ret) {
		DRM_ERROR("Failed to parse AFM data\n");
		return ret;
	}

	ret = sgpu_afm_alloc_pmic_function(afm_dom);
	if (ret)
		return ret;

	ret = sgpu_afm_create_sysfs_file(adev);
	if (ret) {
		DRM_ERROR("Failed to crate sysfs for afm\n");
		return -ENODEV;
	}

	INIT_WORK(&afm_dom->afm_work, sgpu_afm_work);
	INIT_DELAYED_WORK(&afm_dom->release_dwork, sgpu_afm_work_release);
	INIT_DELAYED_WORK(&afm_dom->register_dwork, sgpu_afm_register);

	schedule_delayed_work(&afm_dom->register_dwork,
			      msecs_to_jiffies(afm_dom->register_duration));
	return 0;
}

int sgpu_afm_get_irq(struct amdgpu_device *adev)
{
	adev->afm_irq = platform_get_irq_byname(adev->pldev, "GPU-AFM");

	if (adev->afm_irq < 0)
		return adev->afm_irq;

	return 0;
}

void sgpu_afm_suspend(struct amdgpu_device *adev)
{
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;

	if (!adev->enable_afm)
		return;

	sgpu_afm_control_interrupt(afm_dom, false);
}

void sgpu_afm_resume(struct amdgpu_device *adev)
{
	struct sgpu_afm_domain *afm_dom = &adev->afm_dom;

	if (!adev->enable_afm)
		return;

	sgpu_afm_init_htu(afm_dom);
}

