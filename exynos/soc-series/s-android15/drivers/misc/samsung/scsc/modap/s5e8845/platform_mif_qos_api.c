#include "platform_mif.h"

#if IS_ENABLED(CONFIG_SCSC_QOS)
#include <soc/samsung/exynos_pm_qos.h>
#include <soc/samsung/freq-qos-tracer.h>
#include <soc/samsung/cal-if.h>
#include <dt-bindings/clock/s5e8845.h>

#include "scsc_mif_abs.h"

#ifdef CONFIG_WLBT_KUNIT
#include "../../kunit/kunit_platform_mif_qos_api.c"
#elif defined CONFIG_SCSC_WLAN_KUNIT_TEST
#include "../../kunit/kunit_net_mock.h"
#endif

/* 533Mhz / 1152Mhz / 1824Mhz for cpucl0 */
static uint qos_cpucl0_lv[] = {0, 0, 6, 13};
module_param_array(qos_cpucl0_lv, uint, NULL, 0644);
MODULE_PARM_DESC(qos_cpucl0_lv, "S5E8535 DVFS Lv of CPUCL0 to apply Min/Med/Max PM QoS");

/* 533Mhz / 1536Mhz / 2304Mhz for cpucl1 */
static uint qos_cpucl1_lv[] = {0, 0, 10, 18};
module_param_array(qos_cpucl1_lv, uint, NULL, 0644);
MODULE_PARM_DESC(qos_cpucl1_lv, "S5E8535 DVFS Lv of CPUCL1 to apply Min/Med/Max PM QoS");

/* 421Mhz / 1539Mhz / 2535Mhz for each cpucl1 level */
static uint qos_mif_lv[] = {0, 12, 6, 2};
module_param_array(qos_mif_lv, uint, NULL, 0644);
MODULE_PARM_DESC(qos_mif_lv, "S5E8845 DVFS Lv of MIF to apply Min/Med/Max PM QoS");


static int platform_mif_set_affinity_cpu(struct scsc_mif_abs *interface, u8 cpu)
{
	struct platform_mif *platform = platform_mif_from_mif_abs(interface);

	SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev, "Change CPU affinity to %d\n", cpu);
	return irq_set_affinity_hint(platform->wlbt_irq[PLATFORM_MIF_MBOX].irq_num, cpumask_of(cpu));
}

int qos_get_param(char *buffer, const struct kernel_param *kp)
{
	/* To get cpu_policy of cl0 and cl1*/
	struct cpufreq_policy *cpucl0_policy = cpufreq_cpu_get(0);
	struct cpufreq_policy *cpucl1_policy = cpufreq_cpu_get(4);
	struct dvfs_rate_volt *mif_domain_fv;
	u32 mif_lv;
	int count = 0;

	count += sprintf(buffer + count, "CPUCL0 QoS: %d, %d, %d\n",
			cpucl0_policy->freq_table[qos_cpucl0_lv[1]].frequency,
			cpucl0_policy->freq_table[qos_cpucl0_lv[2]].frequency,
			cpucl0_policy->freq_table[qos_cpucl0_lv[3]].frequency);
	count += sprintf(buffer + count, "CPUCL1 QoS: %d, %d, %d\n",
			cpucl1_policy->freq_table[qos_cpucl1_lv[1]].frequency,
			cpucl1_policy->freq_table[qos_cpucl1_lv[2]].frequency,
			cpucl1_policy->freq_table[qos_cpucl1_lv[3]].frequency);

	mif_lv = cal_dfs_get_lv_num(ACPM_DVFS_MIF);
	mif_domain_fv =
		kcalloc(mif_lv, sizeof(struct dvfs_rate_volt), GFP_KERNEL);
	cal_dfs_get_rate_asv_table(ACPM_DVFS_MIF, mif_domain_fv);
	count += sprintf(buffer + count, "MIF QoS: %lu, %lu, %lu\n",
			mif_domain_fv[qos_mif_lv[1]].rate, mif_domain_fv[qos_mif_lv[2]].rate,
			mif_domain_fv[qos_mif_lv[3]].rate);

	cpufreq_cpu_put(cpucl0_policy);
	cpufreq_cpu_put(cpucl1_policy);
	kfree(mif_domain_fv);

	return count;
}

const struct kernel_param_ops domain_id_ops = {
	.set = NULL,
	.get = &qos_get_param,
};
module_param_cb(qos_info, &domain_id_ops, NULL, 0444);

static void platform_mif_verify_qos_table(struct platform_mif *platform)
{
	u32 index;
	u32 cl0_max_idx, cl1_max_idx, mif_max_idx;

	cl0_max_idx = cpufreq_frequency_table_get_index(platform->qos.cpucl0_policy,
							platform->qos.cpucl0_policy->max);
	cl1_max_idx = cpufreq_frequency_table_get_index(platform->qos.cpucl1_policy,
							platform->qos.cpucl1_policy->max);

	if (cal_dfs_get_lv_num(ACPM_DVFS_MIF))
		mif_max_idx = (cal_dfs_get_lv_num(ACPM_DVFS_MIF) - 1);
	else
		mif_max_idx = 0;


	for (index = SCSC_QOS_MIN; index <= SCSC_QOS_MAX; index++) {
		qos_cpucl0_lv[index] = min(qos_cpucl0_lv[index], cl0_max_idx);
		qos_cpucl1_lv[index] = min(qos_cpucl1_lv[index], cl1_max_idx);
		qos_mif_lv[index] = min(qos_mif_lv[index], mif_max_idx);
	}
}


static int platform_mif_pm_qos_add_request(struct scsc_mif_abs *interface, struct scsc_mifqos_request *qos_req, enum scsc_qos_config config)
{
	struct platform_mif *platform = platform_mif_from_mif_abs(interface);

	if (!platform)
		return -ENODEV;

	if (!platform->qos_enabled) {
		SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev,
				  "PM QoS not configured\n");
		return -EOPNOTSUPP;
	}

	qos_req->cpu_cluster0_policy = cpufreq_cpu_get(0);
	qos_req->cpu_cluster1_policy = cpufreq_cpu_get(4);

	if ((!qos_req->cpu_cluster0_policy) || (!qos_req->cpu_cluster1_policy)) {
		SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev,
				"PM QoS add request error. CPU policy not loaded\n");
		return -ENOENT;
	}

	if (config == SCSC_QOS_DISABLED) {
		freq_qos_tracer_add_request(
			&qos_req->cpu_cluster0_policy->constraints,
			&qos_req->pm_qos_req_cl0,
			FREQ_QOS_MIN,
			0);
		freq_qos_tracer_add_request(
			&qos_req->cpu_cluster1_policy->constraints,
			&qos_req->pm_qos_req_cl1,
			FREQ_QOS_MIN,
			0);
		exynos_pm_qos_add_request(
			&qos_req->pm_qos_req_mif,
			PM_QOS_BUS_THROUGHPUT,
			0);
	} else {
		freq_qos_tracer_add_request(
			&qos_req->cpu_cluster0_policy->constraints,
			&qos_req->pm_qos_req_cl0,
			FREQ_QOS_MIN,
			platform->qos.cpucl0_policy->freq_table[qos_cpucl0_lv[config]].frequency);
		freq_qos_tracer_add_request(
			&qos_req->cpu_cluster1_policy->constraints,
			&qos_req->pm_qos_req_cl1,
			FREQ_QOS_MIN,
			platform->qos.cpucl1_policy->freq_table[qos_cpucl1_lv[config]].frequency);
		exynos_pm_qos_add_request(
			&qos_req->pm_qos_req_mif,
			PM_QOS_BUS_THROUGHPUT,
			platform->qos.mif_domain_fv[qos_mif_lv[config]].rate);
	}

	SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev,
		"PM QoS add request: %u. CL0 %u CL1 %u MIF %u\n", config,
		platform->qos.cpucl0_policy->freq_table[qos_cpucl0_lv[config]].frequency,
		platform->qos.cpucl1_policy->freq_table[qos_cpucl1_lv[config]].frequency,
		platform->qos.mif_domain_fv[qos_mif_lv[config]].rate);

	return 0;
}

static int platform_mif_pm_qos_update_request(struct scsc_mif_abs *interface, struct scsc_mifqos_request *qos_req, enum scsc_qos_config config)
{
	struct platform_mif *platform = platform_mif_from_mif_abs(interface);

	if (!platform)
		return -ENODEV;

	if (!platform->qos_enabled) {
		SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev, "PM QoS not configured\n");
		return -EOPNOTSUPP;
	}

	if (config == SCSC_QOS_DISABLED) {
		freq_qos_update_request(&qos_req->pm_qos_req_cl0, 0);
		freq_qos_update_request(&qos_req->pm_qos_req_cl1, 0);
		exynos_pm_qos_update_request(&qos_req->pm_qos_req_mif, 0);
	} else {
		freq_qos_update_request(&qos_req->pm_qos_req_cl0,
			platform->qos.cpucl0_policy->freq_table[qos_cpucl0_lv[config]].frequency);
		freq_qos_update_request(&qos_req->pm_qos_req_cl1,
			platform->qos.cpucl1_policy->freq_table[qos_cpucl1_lv[config]].frequency);
		exynos_pm_qos_update_request(&qos_req->pm_qos_req_mif,
			platform->qos.mif_domain_fv[qos_mif_lv[config]].rate);
	}
	SCSC_TAG_DEBUG_DEV(PLAT_MIF, platform->dev,
		"PM QoS add request: %u. CL0 %u CL1 %u MIF %u\n", config,
		platform->qos.cpucl0_policy->freq_table[qos_cpucl0_lv[config]].frequency,
		platform->qos.cpucl1_policy->freq_table[qos_cpucl1_lv[config]].frequency,
		platform->qos.mif_domain_fv[qos_mif_lv[config]].rate);

	return 0;
}

#if IS_ENABLED(CONFIG_SCSC_WLAN_SEPARATED_CLUSTER_FREQUENCY)
static int platform_mif_pm_qos_cluster_update_request(struct scsc_mif_abs *interface,
						      struct scsc_mifqos_request *qos_req,
						      enum scsc_qos_config config)
{
	struct platform_mif *platform = platform_mif_from_mif_abs(interface);
	struct freq_qos_request *big_cluster, *little_cluster;
	unsigned int big_freq, little_freq;

	if (!platform)
		return -ENODEV;

	if (!platform->qos_enabled) {
		SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev, "PM QoS not configured\n");
		return -EOPNOTSUPP;
	}

	big_cluster = &qos_req->pm_qos_req_cl1;
	little_cluster = &qos_req->pm_qos_req_cl0;
	big_freq = platform->qos.cpucl1_policy->freq_table[qos_cpucl1_lv[config]].frequency;
	little_freq = platform->qos.cpucl0_policy->freq_table[qos_cpucl0_lv[config]].frequency;

	switch (config) {
	case SCSC_QOS_MAX:
		SCSC_TAG_INFO(PLAT_MIF, "PM QoS MAX. add request CL1: %u.\n", big_freq);
		freq_qos_update_request(big_cluster, big_freq);
		break;
	case SCSC_QOS_MED:
		SCSC_TAG_INFO(PLAT_MIF, "PM QoS MED. add request CL0: %u.\n", little_freq);
		freq_qos_update_request(little_cluster, little_freq);
		break;
	case SCSC_QOS_MIN:
		SCSC_TAG_INFO(PLAT_MIF, "PM QoS MIN. add request CL0: %u CL1: %u\n", little_freq, big_freq);
		freq_qos_update_request(big_cluster, big_freq);
		freq_qos_update_request(little_cluster, little_freq);
		break;
	default:
		SCSC_TAG_INFO(PLAT_MIF, "PM QoS DISABLED. add request CL0: %u CL1: %u\n", 0, 0);
		freq_qos_update_request(big_cluster, 0);
		freq_qos_update_request(little_cluster, 0);
		break;
	}

	return 0;
}
#endif

static int platform_mif_pm_qos_remove_request(struct scsc_mif_abs *interface, struct scsc_mifqos_request *qos_req)
{
	struct platform_mif *platform = platform_mif_from_mif_abs(interface);

	if (!platform)
		return -ENODEV;

	if (!platform->qos_enabled) {
		SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev,
			"PM QoS not configured\n");
		return -EOPNOTSUPP;
	}

	SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev,
		"PM QoS remove request\n");

	freq_qos_tracer_remove_request(&qos_req->pm_qos_req_cl0);
	freq_qos_tracer_remove_request(&qos_req->pm_qos_req_cl1);
	exynos_pm_qos_remove_request(&qos_req->pm_qos_req_mif);

	return 0;
}

int platform_mif_qos_init(struct platform_mif *platform)
{
	struct scsc_mif_abs *interface = &platform->interface;
	u32 mif_lv;

	platform->qos.cpucl0_policy = cpufreq_cpu_get(0);
	platform->qos.cpucl1_policy = cpufreq_cpu_get(4);

	platform->qos_enabled = false;

	interface->mif_pm_qos_add_request = platform_mif_pm_qos_add_request;
#if IS_ENABLED(CONFIG_SCSC_WLAN_SEPARATED_CLUSTER_FREQUENCY)
	interface->mif_pm_qos_cluster_update_request = platform_mif_pm_qos_cluster_update_request;
#endif
	interface->mif_pm_qos_update_request = platform_mif_pm_qos_update_request;
	interface->mif_pm_qos_remove_request = platform_mif_pm_qos_remove_request;
	interface->mif_set_affinity_cpu = platform_mif_set_affinity_cpu;

	if ((!platform->qos.cpucl0_policy) || (!platform->qos.cpucl1_policy)) {
		SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev,
				"PM QoS init error. CPU policy is not loaded\n");
		return -ENOENT;
	}

	mif_lv = cal_dfs_get_lv_num(ACPM_DVFS_MIF);

	platform->qos.mif_domain_fv =
		devm_kzalloc(platform->dev, sizeof(struct dvfs_rate_volt) * mif_lv, GFP_KERNEL);
	cal_dfs_get_rate_asv_table(ACPM_DVFS_MIF, platform->qos.mif_domain_fv);

	if (!platform->qos.mif_domain_fv) {
		SCSC_TAG_INFO_DEV(PLAT_MIF, platform->dev,
				"PM QoS init error. DEV(MIF) policy is not loaded\n");
		return -ENOENT;
	}

	/* verify pre-configured freq-levels of cpucl0/1 */
	platform_mif_verify_qos_table(platform);

	platform->qos_enabled = true;

	return 0;
}
#else
void platform_mif_qos_init(struct platform_mif *platform)
{
	(void)platform;
}
#endif
