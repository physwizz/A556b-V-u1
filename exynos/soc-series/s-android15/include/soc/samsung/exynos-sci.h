#ifndef __EXYNOS_SCI_H_
#define __EXYNOS_SCI_H_

#include <linux/module.h>
#include <soc/samsung/exynos-llcgov.h>

#define EXYNOS_SCI_MODULE_NAME		"exynos-sci"

#define SCI_INFO(x...)	pr_info("sci_info: " x)
#define SCI_DBG(x...)	if (scidbg_log) pr_info("sci_dbg: " x)
#define SCI_ERR(x...)	pr_err("sci_err: " x)

static bool scidbg_log = false;

#define LLC_ID_MAX				(9)
#define LLC_WAY_MAX				(get_llc_way_max())

#define DISABLE_LLC				(false)
#define ENABLE_LLC				(true)

/* IPC common definition */
#define SCI_ONE_BIT_MASK			(0x1)
#define SCI_ERR_MASK				(0x7)
#define SCI_ERR_SHIFT				(13)

#define SCI_CMD_IDX_MASK			(0x3F)
#define SCI_CMD_IDX_SHIFT			(0)
#define SCI_DATA_MASK				(0x3F)
#define SCI_DATA_SHIFT				(6)
#define SCI_IPC_DIR_SHIFT			(12)
#define NUM_OF_ERR_LOG				(100)

#define SCI_CMD_QPD_MASK                        (0x1)
#define SCI_CMD_QPD_SHIFT                       (0)
#define SCI_CMD_SHARED_MASK                     (0x7)
#define SCI_CMD_SHARED_SHIFT                    (1)

#define SCI_CMD_GET(cmd_data, mask, shift)	((cmd_data & (mask << shift)) >> shift)
#define SCI_CMD_CLEAR(mask, shift)		(~(mask << shift))
#define SCI_CMD_SET(data, mask, shift)		((data & mask) << shift)

#define SYS_READ(base, reg, val)	do { val = __raw_readl(base + reg); } while (0);
#define SYS_WRITE(base, reg, val)	do { __raw_writel(val, base + reg); } while (0);

enum exynos_sci_cmd_index {
	SCI_LLC_ENABLE = 0,
	SCI_LLC_ON = 4,
	SCI_LLC_REGION_ALLOC = 3,
	SCI_LLC_REGION_DEALLOC = 6,
	SCI_LLC_GET_REGION_INFO = 8,
	SCI_LLC_QUADRANT_EN = 12,
	SCI_VCH_SET = 13,
	SCI_SCRATCH_PAD_SET = 14,
	SCI_LLC_MPAM_ALLOC = 15,
	SCI_LLC_PPC_EN = 16,
	SCI_LLC_SHARED_IDX = 17,
	SCI_LLC_SHARED_WAY = 18,
	SCI_CMD_MAX,
};

enum exynos_sci_err_code {
	E_OK_S = 0,
	E_INVAL_S,
	E_BUSY_S,
};

enum exynos_sci_ecc_err_type {
	C_ERR = 0,
	UC_ERR,
	NUM_ERR_TYPE,
};

enum exynos_sci_ipc_dir {
	SCI_IPC_GET = 0,
	SCI_IPC_SET,
};

enum exynos_sci_llc_region_index {
	LLC_REGION_DISABLE = 0,
	LLC_REGION_CPU, /* default policy */
	LLC_REGION_CPU_MPAM0,
	LLC_REGION_CPU_MPAM1,
	LLC_REGION_CPU_MPAM2,
	LLC_REGION_CPU_MPAM3,
	LLC_REGION_CPU_MPAM4,
	LLC_REGION_CPU_MPAM5,
	LLC_REGION_CPU_MPAM6,
	LLC_REGION_CPU_MPAM7,
	LLC_REGION_CALL,
	LLC_REGION_OFFLOAD,
	LLC_REGION_CPD2,
	LLC_REGION_CPCPU,
	LLC_REGION_DPU,
	LLC_REGION_ICPU,
	LLC_REGION_MFC0_DPB,
	LLC_REGION_MFC1_DPB,
	LLC_REGION_MFC0_INT,
	LLC_REGION_MFC1_INT,
	LLC_REGION_GDC,
	LLC_REGION_PROFILER,
	LLC_REGION_GPU,
	LLC_REGION_NPU0,
	LLC_REGION_NPU1,
	LLC_REGION_NPU2,
	LLC_REGION_DSP0,
	LLC_REGION_DSP1,
	LLC_REGION_CAM_MCFP,
	LLC_REGION_CAM_CSIS,
	LLC_REGION_CP_MAX_TP,
	LLC_REGION_MAX,
};

enum exynos_sci_llc_enable_source_index {
	LLC_ENABLE_DEFAULT,
	LLC_ENABLE_PANIC,
	LLC_ENABLE_SYSFS,
	LLC_ENABLE_THERMAL,
	LLC_ENABLE_STR,
	LLC_ENABLE_SOURCE_MAX,
};

struct exynos_sci_cmd_info {
	enum exynos_sci_cmd_index	cmd_index;
	enum exynos_sci_ipc_dir		direction;
	unsigned int			data;
};

struct exynos_sci_llc_region {
	const char			*name;
	unsigned int			priority;
	unsigned int			use_qpd;
	int				usage_count;
	unsigned int			way;
	unsigned int			min_freq;

	unsigned int			shared_idx;// 0 means single allocation 
};

struct exynos_sci_mpam_data {
	int part_id;
	int perf_mon_gr;
	int ns; /* Non Secure Partition */
	int valen;
};

struct exynos_sci_llc_shared_data {
	unsigned int			policy_index[LLC_REGION_MAX];
	unsigned int			index_cnt;//the number of policy in this grp
	unsigned int			used_way;// all allocated way
	unsigned int			total_way;//all 	policy way
	unsigned int			max_way; //max way

	unsigned int used_way_info; // after allocation return info from ASM
	//ex. 0x0f00, 0x3000 
};

struct exynos_sci_err_hist {
	/* UcErrSource [7:0], UcErrType [16:13], UcErrSubType [25:17] */
	unsigned int err_data[NUM_OF_ERR_LOG];
	unsigned long long err_time[NUM_OF_ERR_LOG];
	unsigned int wptr;
	unsigned int rptr;
	unsigned int total_err_cnt;
};

struct exynos_sci_data {
	struct device			*dev;
	spinlock_t			lock;

	void __iomem			*sci_base;

	unsigned int			ipc_ch_num;
	unsigned int			ipc_ch_size;

	bool				llc_enable;
	bool				llc_enable_status[LLC_ENABLE_SOURCE_MAX];
	bool				llc_retention_mode;
	unsigned int			llc_disable_threshold;

	int				on_cnt;

	unsigned int			region_size;
	struct exynos_sci_llc_region	*llc_region;
/*
#if !(defined (CONFIG_EXYNOS_ESCAV1) || (CONFIG_EXYNOS_ESCAV1_MODULE))
	unsigned int			vch_size;
	unsigned int			*vch_pd_calid;
#endif
*/
	bool				llc_debug_mode;

	/* Container for governor data */
        bool                            llc_gov_en;
	struct llc_gov_data		*gov_data;
	unsigned int			irqcnt;
	unsigned int			mpam_nr;
	struct exynos_sci_mpam_data	*mpam_data;

	unsigned int			qpd_onoff[LLC_REGION_MAX];
	struct exynos_sci_err_hist	err_data[NUM_ERR_TYPE];

	struct exynos_sci_llc_shared_data llc_shared_data[LLC_REGION_MAX];
};

#if defined(CONFIG_EXYNOS_SCI_DBG) || defined(CONFIG_EXYNOS_SCI_DBG_MODULE)
#define LLCId_BASE					(0x5C0)
#define LLCId(x)					(LLCId_BASE + ((x) * 12))
#define LLCIdAllocLkup_BASE				(0x5C4)
#define LLCIdAllocLkup(x)				(LLCIdAllocLkup_BASE + ((x) * 12))
#define LLCCtrl						(0x544)
#define LLCMpamId_BASE					(0x5C8)
#define LLCMpamId(x)					(LLCMpamId_BASE + ((x) * 12))
#endif
/* S5E9935 ECC Error logging Register offset */
#define SCI_CErrMiscInfo				(0x918)
#define SCI_CErrSource					(0x914)
#define SCI_CErrAddrLow					(0x91C)
#define SCI_CErrAddrHigh				(0x920)
#define SCI_CErrOverrunMiscInfo				(0x924)
#define SCI_UcErrSource					(0x940)
#define SCI_UcErrMiscInfo				(0x944)
#define SCI_UcErrAddrLow				(0x948)
#define SCI_UcErrAddrHigh				(0x94C)
#define SCI_UcErrOverrunMiscInfo			(0x950)

#if IS_ENABLED(CONFIG_EXYNOS_SCI)
extern int llc_set_enable(bool enable, enum exynos_sci_llc_enable_source_index src_index);
extern int llc_region_alloc(unsigned int index, bool on, unsigned int way);
extern unsigned int llc_get_region_info(unsigned int index);
extern int llc_get_region_index_by_name(const char *name);
extern int llc_ppc_en(unsigned int on);
extern void llc_qpd_on(unsigned int index, unsigned int on);
extern u32 get_llc_way_max(void);
extern int llc_disable_force(bool off);

/* ToDo */
static inline void llc_invalidate(unsigned int invway) {}
static inline void llc_flush(unsigned int invway) {}
static inline int llc_enable(bool on) { return -EPERM; }
static inline int llc_get_en(void) { return 0; }
#else
static inline int llc_set_enable(bool enable, enum exynos_sci_llc_enable_source_index src_index) { return 0; }
static inline int llc_region_alloc(unsigned int index, bool on, unsigned int way)
{
	return -ENODEV;
}
static inline unsigned int llc_get_region_info(unsigned int index) { return -ENODEV; }
static inline int llc_get_region_index_by_name(const char *name) { return -ENODEV; }
static inline int llc_ppc_en(unsigned int on) { return -ENODEV; }
static inline void llc_qpd_on(unsigned int index, unsigned int on) {}

/* ToDo */
static inline void llc_invalidate(unsigned int invway) {}
static inline void llc_flush(unsigned int invway) {}
static inline int llc_enable(bool on) { return -ENODEV; }
static inline int llc_get_en(void) { return 0; }
static inline int llc_disable_force(bool off) { return 0; }
static inline u32 get_llc_way_max(void) { return 0; }
#endif

#endif	/* __EXYNOS_SCI_H_ */
