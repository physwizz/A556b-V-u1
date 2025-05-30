#include <soc/samsung/pmucal/pmucal_common.h>
#include <soc/samsung/pmucal/pmucal_cpu.h>
#include <soc/samsung/pmucal/pmucal_local.h>
#include <soc/samsung/pmucal/pmucal_rae.h>
#include <soc/samsung/pmucal/pmucal_system.h>
#include <soc/samsung/pmucal/pmucal_powermode.h>

#include "pmucal/flexpmu_cal_cpu_s5e9945.h"
#include "pmucal/flexpmu_cal_local_s5e9945.h"
#include "pmucal/flexpmu_cal_p2vmap_s5e9945.h"
#include "pmucal/flexpmu_cal_system_s5e9945.h"
#include "pmucal/flexpmu_cal_define_s5e9945.h"

#if IS_ENABLED(CONFIG_CP_PMUCAL)
#include <soc/samsung/pmucal/pmucal_cp.h>
#include "pmucal/pmucal_cp_s5e9945.h"
#endif

#if IS_ENABLED(CONFIG_GNSS_PMUCAL)
#include <soc/samsung/pmucal/pmucal_gnss.h>
#include "pmucal/pmucal_gnss_s5e9945.h"
#endif

#if IS_ENABLED(CONFIG_CHUB_PMUCAL)
#include <soc/samsung/pmucal/pmucal_chub.h>
#include "pmucal/pmucal_chub_s5e9945.h"
#endif

#include "cmucal/cmucal-node.c"
#include "cmucal/cmucal-qch.c"
#include "cmucal/cmucal-sfr.c"
#include "cmucal/cmucal-vclk.c"
#include "cmucal/cmucal-vclklut.c"

#include "cmucal/clkout_s5e9945.c"
#include "acpm_dvfs_s5e9945.h"
#include "asv_s5e9945.h"
#include "../ra.h"
#include <linux/smc.h>

#include <soc/samsung/exynos-cpupm.h>

/* defines for PLL_MMC SSC settings */
#define S5E9945_CMU_TOP_BASE		(0x22b20000)
#define PLL_CON0_PLL_MMC	(0x2c0)
#define PLL_CON1_PLL_MMC	(0x2c4)
#define PLL_CON2_PLL_MMC	(0x2c8)
#define PLL_CON3_PLL_MMC	(0x2cc)
#define PLL_CON4_PLL_MMC	(0x2d0)
#define PLL_CON5_PLL_MMC	(0x2d4)

#define PLL_ENABLE_SHIFT	(31)
#define MANUAL_MODE		(0x2)
#define PLL_MMC_MUX_BUSY_SHIFT	(16)
#define MFR_MASK		(0xff)
#define MRR_MASK		(0x3f)
#define MFR_SHIFT		(16)
#define MRR_SHIFT		(24)
#define SSCG_EN			(16)

void __iomem *cmu_top;

#define NUM_SKIP_CMU_SFR	(4)
u32 skip_cmu_sfr[NUM_SKIP_CMU_SFR] = {0, };

unsigned int frac_rpll_list[14];
unsigned int frac_rpll_size;

unsigned int s5e9945_frac_rpll_list[] = {
	AUD_PLL_AUD,
	TOP_PLL_MMC,
	TOP_PLL_SHARED0,
	TOP_PLL_SHARED1,
	TOP_PLL_SHARED2,
	TOP_PLL_SHARED3,
	TOP_PLL_SHARED4,
	TOP_PLL_SHARED_MIF,
	CPUCL0_PLL_CPUCL0,
	CPUCL1L_PLL_CPUCL1L,
	CPUCL1H_PLL_CPUCL1H,
	DSU_PLL_DSU,
	G3DCORE_PLL_G3D,
	G3DCORE_PLL_G3D1,
};

static int cmu_stable_done(void __iomem *cmu,
			unsigned char shift,
			unsigned int done,
			int usec)
{
	unsigned int result;

	do {
		result = get_bit(cmu, shift);

		if (result == done)
			return 0;
		udelay(1);
	} while (--usec > 0);

	return -EVCLKTIMEOUT;
}

int pll_mmc_enable(int enable)
{
	unsigned int reg;
	unsigned int cmu_mode;
	int ret;

	if (!cmu_top) {
		pr_err("%s: cmu_top cmuioremap failed\n", __func__);
		return -1;
	}

	/* set PLL to manual mode */
	cmu_mode = readl(cmu_top + PLL_CON1_PLL_MMC);
	writel(MANUAL_MODE, cmu_top + PLL_CON1_PLL_MMC);

	if (!enable) {
		/* select oscclk */
		reg = readl(cmu_top + PLL_CON0_PLL_MMC);
		reg &= ~(PLL_MUX_SEL);
		writel(reg, cmu_top + PLL_CON0_PLL_MMC);

		ret = cmu_stable_done(cmu_top + PLL_CON0_PLL_MMC, PLL_MMC_MUX_BUSY_SHIFT, 0, 100);
		if (ret)
			pr_err("pll mux change time out, \'PLL_MMC\'\n");
	}

	/* setting ENABLE of PLL */
	reg = readl(cmu_top + PLL_CON3_PLL_MMC);
	if (enable)
		reg |= 1 << PLL_ENABLE_SHIFT;
	else
		reg &= ~(1 << PLL_ENABLE_SHIFT);
	writel(reg, cmu_top + PLL_CON3_PLL_MMC);

	if (enable) {
		/* wait for PLL stable */
		ret = cmu_stable_done(cmu_top + PLL_CON3_PLL_MMC, PLL_STABLE_SHIFT, 1, 100);
		if (ret)
			pr_err("pll time out, \'PLL_MMC\' %d\n", enable);

		/* select FOUT_PLL_MMC */
		reg = readl(cmu_top + PLL_CON0_PLL_MMC);
		reg |= PLL_MUX_SEL;
		writel(reg, cmu_top + PLL_CON0_PLL_MMC);

		ret = cmu_stable_done(cmu_top + PLL_CON0_PLL_MMC, PLL_MMC_MUX_BUSY_SHIFT, 0, 100);
		if (ret)
			pr_err("pll mux change time out, \'PLL_MMC\'\n");
	}

	/* restore PLL mode */
	writel(cmu_mode, cmu_top + PLL_CON1_PLL_MMC);

	return ret;
}

int cal_pll_mmc_check(void)
{
       unsigned int reg;
       bool ret = false;

       reg = readl(cmu_top + PLL_CON4_PLL_MMC);

       if (reg & (1 << SSCG_EN))
               ret = true;

       return ret;
}EXPORT_SYMBOL(cal_pll_mmc_check);

int cal_pll_mmc_set_ssc(unsigned int mfr, unsigned int mrr, unsigned int ssc_on)
{
	unsigned int reg;
	int ret = 0;

	/* disable PLL_MMC */
	ret = pll_mmc_enable(0);
	if (ret) {
		pr_err("%s: pll_mmc_disable failed\n", __func__);
		return ret;
	}

	/* setting MFR, MRR */
	reg = readl(cmu_top + PLL_CON5_PLL_MMC);
	reg &= ~((MFR_MASK << MFR_SHIFT) | (MRR_MASK << MRR_SHIFT));

	if (ssc_on)
		reg |= ((mfr & MFR_MASK) << MFR_SHIFT) | ((mrr & MRR_MASK) << MRR_SHIFT);
	writel(reg, cmu_top + PLL_CON5_PLL_MMC);

	/* setting SSCG_EN */
	reg = readl(cmu_top + PLL_CON4_PLL_MMC);
	if (ssc_on)
		reg |= 1 << SSCG_EN;
	else
		reg &= ~(1 << SSCG_EN);
	writel(reg, cmu_top + PLL_CON4_PLL_MMC);

	/* enable PLL_MMC */
	ret = pll_mmc_enable(1);
	if (ret)
		pr_err("%s: pll_mmc_enable failed\n", __func__);

	return ret;
}EXPORT_SYMBOL(cal_pll_mmc_set_ssc);

void s5e9945_cal_data_init(void)
{
	int i;

	pr_info("%s: cal data init\n", __func__);

	/* cpu inform sfr initialize */
	pmucal_sys_powermode[SYS_SICD] = CPU_INFORM_SICD;
	pmucal_sys_powermode[SYS_SLEEP] = CPU_INFORM_SLEEP;
	pmucal_sys_powermode[SYS_SLEEP_HSI1ON] = CPU_INFORM_SLEEP_HSI1ON;

	cpu_inform_c2 = CPU_INFORM_C2;
	cpu_inform_cpd = CPU_INFORM_CPD;

	cmu_top = ioremap(S5E9945_CMU_TOP_BASE, SZ_4K);
	if (!cmu_top)
		pr_err("%s: cmu_top ioremap failed\n", __func__);

	frac_rpll_size = ARRAY_SIZE(s5e9945_frac_rpll_list);
	for (i = 0; i < frac_rpll_size; i++)
		frac_rpll_list[i] = s5e9945_frac_rpll_list[i];

	mux_dbg_offset = 0x3000;
	top_gate_dbg_offset = 0x2004;
	gate_dbg_offset = 0x2000;
}

void (*cal_data_init)(void) = s5e9945_cal_data_init;

void get_high_freq_pll_idx(int *high_freq_pll_list)
{
	int i = 0, idx;

	for (idx = MIF0_PLL_MIF_MAIN; idx <= MIF3_PLL_MIF_SUB; idx++)
		high_freq_pll_list[i++] = idx;
}

bool is_ignore_cmu_dbg(u32 addr)
{
	int i;

	for (i = 0; i < NUM_SKIP_CMU_SFR; i++) {

		if (addr == skip_cmu_sfr[i])
			return true;
	}

	return false;
}
