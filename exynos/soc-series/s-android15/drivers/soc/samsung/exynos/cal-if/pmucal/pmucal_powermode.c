#include <soc/samsung/cal-if.h>
#include <soc/samsung/pwrcal-env.h>
#include <soc/samsung/pmucal/pmucal_system.h>
#include <soc/samsung/pmucal/pmucal_powermode.h>
#include <soc/samsung/pmucal/pmucal_rae.h>
#include <soc/samsung/pmucal/pmucal_cpu.h>

void pmucal_powermode_hint(unsigned int mode)
{
	unsigned int cpu = smp_processor_id();

	__raw_writel(mode, pmucal_cpuinform_list[cpu].base_va
			+ pmucal_cpuinform_list[cpu].offset);
}

u32 pmucal_get_powermode_hint(unsigned int cpu)
{
	return __raw_readl(pmucal_cpuinform_list[cpu].base_va
			+ pmucal_cpuinform_list[cpu].offset);
}

u32 pmucal_is_lastcore_detecting(unsigned int cpu)
{
	u32 power_mode;

	if (cpu >= cpu_inform_list_size)
		return 0;

	power_mode = pmucal_get_powermode_hint(cpu);

	if (!cal_cpu_status(cpu))
		return 0;

	if (power_mode == cpu_inform_cpd || power_mode == pmucal_sys_powermode[SYS_SICD])
		return 1;

	return 0;
}

void pmucal_powermode_hint_clear(void)
{
	unsigned int cpu = smp_processor_id();

	__raw_writel(0, pmucal_cpuinform_list[cpu].base_va
			+ pmucal_cpuinform_list[cpu].offset);
}

int pmucal_cpuinform_init(void)
{
	int i, j;

	for (i = 0; i < cpu_inform_list_size; i++) {
		for (j = 0; j < pmucal_p2v_list_size; j++)
			if (pmucal_p2v_list[j].pa == (phys_addr_t)pmucal_cpuinform_list[i].base_pa)
				break;

		if (j != pmucal_p2v_list_size) {
			pmucal_cpuinform_list[i].base_va = pmucal_p2v_list[j].va;
		} else {
			pr_err("%s %s: there is no such PA in p2v_list (idx:%d)\n",
					PMUCAL_PREFIX, __func__, i);
			return -ENOENT;
		}

	}

	return 0;
}
