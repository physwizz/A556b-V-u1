// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024, Linux Foundation. All rights reserved.
 */
#ifndef _UFS_CAL_
#define _UFS_CAL_

#include "ufs-vs-mmio.h"
#include "ufs-cal-if.h"

#undef NULL
#define NULL    0

#undef BIT
#define BIT(a)	(1U << (a))

struct ufs_cal_param;

struct uic_pwr_mode {
	u8 lane;
	u8 gear;
	u8 mode;
	u8 hs_series;
};

enum {
	HOST_EMBD = 0,
	HOST_CARD = 1,
};

enum {
	EOM_RTY_G1 = 8,
	EOM_RTY_G2 = 4,
	EOM_RTY_G3 = 2,
	EOM_RTY_G4 = 1,
	EOM_RTY_MAX = 8,
};

enum {
	GEAR_1 = 1,
	GEAR_2,
	GEAR_3,
	GEAR_4,
	GEAR_5,
	GEAR_MAX = GEAR_5,
};

#define MAX_LANE		4

#define EOM_PH_SEL_MAX		128
#define EOM_DEF_VREF_MAX	256
#define EOM_MAX_SIZE		(EOM_RTY_MAX * EOM_PH_SEL_MAX * \
					EOM_DEF_VREF_MAX)
#define PMA_OTP_BASE		(0xE3C0)
#define PMA_OTP_SIZE		(64)
#define PMA_OC_CODE_SAVECNT		(20)

static const u32 ufs_s_eom_repeat[GEAR_MAX + 1] = {
		0, EOM_RTY_G1, EOM_RTY_G2, EOM_RTY_G3, EOM_RTY_G4
};

struct ufs_eom_result_s {
	u32 v_phase;
	u32 v_vref;
	u32 v_err;
};

typedef enum {
	UFS_CAL_NO_ERROR = 0,
	UFS_CAL_TIMEOUT,
	UFS_CAL_ERROR,
	UFS_CAL_INV_ARG,
	UFS_CAL_INV_CONF,
} ufs_cal_errno;

struct ufs_exynos_variant_ops {
	const char *name;

	void (*gate_clk)(struct ufs_cal_param *);
	void (*ctrl_gpio)(struct ufs_cal_param *);
	void (*pad_retention)(struct ufs_cal_param *);
	void (*pmu_input_iso)(struct ufs_cal_param *);
};

/* interface */
struct ufs_cal_param {
	/* input */
	struct ufs_vs_handle *handle;
	u32 available_lane;
	u32 connected_tx_lane;
	u32 connected_rx_lane;
	u32 active_tx_lane;
	u32 active_rx_lane;
	u32 mclk_rate;
	u32 tbl;
	u8 board;
	u32 evt_ver;
	u32 max_gear;
	struct uic_pwr_mode *pmd;

	/* output */
	u32 eom_sz;
	struct ufs_eom_result_s *eom[MAX_LANE];	/* per lane */

	/* private data */
	u32 mclk_period;
	u32 mclk_period_unipro_18;
	u32 config_pcs_clk;

	/* AH8 */
	u32 support_ah8_cal;
	u32 ah8_enabled;
	u32 ah8_thinern8_time;
	u32 ah8_brefclkgatingwaittime;

	/* save & restore */
	u32 save_and_restore_mode;
	unsigned long m_phy_bias;
	int snr_phy_power_on;

	const struct ufs_exynos_variant_ops *vops;
	void *backup;

	/* OTP */
	u8 pma_otp[PMA_OTP_SIZE];	//64byte

	/* save OC code */
	u8 backup_oc_code[GEAR_MAX + GEAR_1][2][PMA_OC_CODE_SAVECNT];

	/* update cal */
	u32 addr;
	u32 value;

	u8 overwrite;

	/* variant for device */
	u8 need_ovrd_vnd_cal;
	u16 vendor;
	char model[64];
};

enum {
	NO_MODE = 0,
	SAVE_MODE,
	RESTORE_MODE,
};

enum {
	__BRD_SMDK,
	__BRD_ERD,
	__BRD_ASB,
	__BRD_HSIE,
	__BRD_ZEBU,
	__BRD_UNIV,
	__BRD_MAX,
};

#define BRD_SMDK	BIT(__BRD_SMDK)
#define BRD_ERD		BIT(__BRD_ERD)
#define BRD_ASB		BIT(__BRD_ASB)
#define BRD_HSIE	BIT(__BRD_HSIE)
#define BRD_ZEBU	BIT(__BRD_ZEBU)
#define BRD_UNIV	BIT(__BRD_UNIV)
#define BRD_MAX		BIT(__BRD_MAX)
#define BRD_ALL		(BIT(__BRD_MAX) - 1)

/* UFS CAL interface */
typedef ufs_cal_errno (*cal_if_func) (struct ufs_cal_param *);
ufs_cal_errno ufs_cal_post_h8_enter(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_pre_h8_exit(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_post_pmc(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_pre_pmc(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_post_link(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_pre_link(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_init(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_eom(struct ufs_cal_param *p);

ufs_cal_errno ufs_cal_loopback_init(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_loopback_set_1(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_loopback_set_2(struct ufs_cal_param *p);

ufs_cal_errno ufs_cal_enable_eom(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_disable_eom(struct ufs_cal_param *p);

ufs_cal_errno ufs_cal_store(struct ufs_cal_param *p);
ufs_cal_errno ufs_cal_show(struct ufs_cal_param *p);

static inline void ufs_vops_gate_clk(struct ufs_cal_param *p)
{
	if (p->vops && p->vops->gate_clk)
		return p->vops->gate_clk(p);
}

static inline void ufs_vops_ctrl_gpio(struct ufs_cal_param *p)
{
	if (p->vops && p->vops->ctrl_gpio)
		return p->vops->ctrl_gpio(p);
}

static inline void ufs_vops_pmu_pad_retention(struct ufs_cal_param *p)
{
	if (p->vops && p->vops->pad_retention)
		return p->vops->pad_retention(p);
}

#endif /*_UFS_CAL_ */
