/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License Version 2 as publised
 * by the Free Software Foundation.
 *
 * Header for BTS Bus Traffic Shaper
 *
 * Includes Data structure for BTS device driver and pre-defined offsets
 * For offset values used in register setting, please refer regs-bts.h
 *
 */

#ifndef __BTS_H__
#define __BTS_H__

#include <linux/types.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <dt-bindings/soc/samsung/exynos-bts.h>
#include <soc/samsung/exynos-pd.h>
#include <soc/samsung/bts.h>

#define DEFAULT_QOS	0x4
#define MAX_QOS		0xF
#define SCIMAX_QOS	0xF
#define MAX_MO		0xFFFF
#define MAX_MO_SCI	0xFF
#define MAX_QUTH	0xFF
#define MAX_QMAX_TH	0xFFFF

#define VC_TIMER_TH_NR	8
#define PF_TIMER_NR	8

/* BTS ACPM IPC CMD */
#define BTS_CMD_SET(data, mask, shift)		((data & mask) << shift)
#define BTS_CMD_IDX_MASK			(0x3F)
#define BTS_CMD_IDX_SHIFT			(0)
#define BTS_ONE_BIT_MASK			(0x1)
#define BTS_IPC_DIR_SHIFT			(12)
#define BTS_DATA_MASK				(0x3F)
#define BTS_DATA_SHIFT				(6)

#if IS_ENABLED(CONFIG_EXYNOS_ESCAV1) || IS_ENABLED(CONFIG_EXYNOS_ESCA) || \
	defined(CONFIG_EXYNOS_USE_SCI_LITE)
enum exynos_bts_cmd_index {
	BTS_VCH_SET = 1,
	BTS_CMD_MAX,
};

enum exynos_bts_ipc_dir {
	BTS_IPC_GET = 0,
	BTS_IPC_SET,
};
struct exynos_bts_cmd_info {
	enum exynos_bts_cmd_index	cmd_index;
	enum exynos_bts_ipc_dir		direction;
	unsigned int			data;
};
#endif

struct bts_scen;
struct bts_stat;
struct bts_info;
struct bts_bw_params;

/**
 * struct bts_bw_parmas - BTS parameters for bandwidth calculation
 * @num_channel:	The number of memory interface channel
 * @mif_bus_width:	Bus width of memory interface
 * @bus_width:		Bus width of internal bus
 * @mif_util:		Target utilization of memory interface
 * @int_util:		Target utilization of internal bus
 *
 */
struct bts_bw_params {
	int	num_channel;
	int	mif_bus_width;
	int	bus_width;
	int	mif_util;
	int	int_util;
};

/**
 * struct bts - Device BTS structure
 * @lock:	a spin-lock to protect accessing bts
 * @mutex_lock: mutex-lock to protect accessing setting DVFS
 *
 * @num_bts:	number of bts hardware
 * @num_scen:	number of scenario be used
 * @top_scen:	current top scenario
 *
 * @bts_list:	struct bts_info * - contains start address pointer for BTS devices
 * @scen_list: struct bts_scen * - contains start address pointer for BTS scenarios
 * @scen_node:	list node - contains structure about scenario
 *
 * @bts_bw:	struct bts_bw * - struct for saving bandwidth information
 * @peak_bw:	currently max bandwidth
 * @total_bw:	current total bandwidth
 *
 * @ipc_ch_num: acpm ipc channel number
 * @ipc_ch_size: acpm ipc channle size
 * @vch_size: the number of vch id
 * @*vch_pd_calid
 *
 * This structure stores basic BTS information for QoS control
 *
 * Note that it contains only basic information of BTS device driver
 *
 */
struct bts_device {
	struct device		*dev;

	spinlock_t		lock;
	struct mutex		mutex_lock;

	unsigned int		num_bts;
	unsigned int		num_scen;
	unsigned int		top_scen;

	struct bts_info		*bts_list;
	struct bts_scen		*scen_list;
	struct list_head	scen_node;

	struct bts_bw		*bts_bw;
	struct bts_bw_params	bw_params;
	unsigned int		peak_bw;
	unsigned int		total_bw;

#if IS_ENABLED(CONFIG_EXYNOS_ESCAV1) || IS_ENABLED(CONFIG_EXYNOS_ESCA) || \
	defined(CONFIG_EXYNOS_USE_SCI_LITE)
	unsigned int		ipc_ch_num;
	unsigned int		ipc_ch_size;
	unsigned int		vch_size;
	unsigned int		*vch_pd_calid;
#endif
};

/**
 * struct bts_ops - operation functions for BTS setting
 * @init_bts:	function for initialize.
 *		It will be run only on initial time and after system resume
 * @set_bts:	operation function to set bts.
 * @get_bts:	operation function to get bts.
 * @set_qos:	operation function to set axqos of bts.
 * @get_qos:	operation function to get axqos of bts.
 * @set_mo:	operation function to set xmo of bts.
 * @get_mo:	operation function to get xmo of bts.
 * @set_urgent:	operation function to set urgent timeout of bts.
 * @get_urgent:	operation function to get urgent timeout of bts.
 * @set_blocking: operation function to set blocking threshold of bts.
 * @get_blocking: operation function to get blocking threshold of bts.
 *
 */
struct bts_ops {
	void		(*init_bts)(void __iomem *va);
	int		(*set_bts)(void __iomem *va, struct bts_stat *stat);
	int		(*get_bts)(void __iomem *va, struct bts_stat *stat);
	int		(*set_qos)(void __iomem *va, struct bts_stat *stat);
	int		(*get_qos)(void __iomem *va, struct bts_stat *stat);
	int		(*set_mo)(void __iomem *va, struct bts_stat *stat);
	int		(*get_mo)(void __iomem *va, struct bts_stat *stat);
	int		(*set_urgent)(void __iomem *va, struct bts_stat *stat);
	int		(*get_urgent)(void __iomem *va, struct bts_stat *stat);
	int		(*set_blocking)(void __iomem *va, struct bts_stat *stat);
	int		(*get_blocking)(void __iomem *va, struct bts_stat *stat);
	int 	(*get_trexbts)(void __iomem *va, unsigned int *reg);
	int 	(*get_smcbts)(void __iomem *va, unsigned int *reg);
	int 	(*get_buscbts)(void __iomem *va, unsigned int *reg);
};

/**
 * struct bts_scen - BTS scenario
 * @node
 * @index:	scenario index
 * @status:	scenario status (0 means scenario is off / 1 means on)
 * @name:	scenario name
 *
 * BTS scenario is defined in Device Tree and parsed in probe function
 * index will be used as priority order of scenarios
 */
struct bts_scen {
	struct list_head	node;

	unsigned int		index;
	bool			status;
	int			usage_count;
	const char		*name;
};

/**
 * struct bts_stat - BTS status
 * @stat_on:		shows current set of config can be used or not
 * @bypass:		when BTS must bypass master request, set as true
 *
 * @arqos:		ArQoS value for master IP (0x0~0xF)
 * @awqos:		AwQoS value for master IP (0x0~0xF)
 * @rmo:		Multiple Outstanding value for Read
 * @wmo:		Multiple Outstanding value for Write
 * @qurgent:		Signal from Master which accelerates delayed requests
 * @qurgent_th_r/w:	Threshold for Qurgent. Read/Write is seperated
 * @blocking:		Signal from Slave which limits master requests
 * @qmax_limit_r/w:	Limitation criteria when QMAX is enabled
 * @qfull_limit_r/w:	Limitation criteria when QFULL is enabled
 * @qbusy_limit_r/w:	Limitation criteria when QBUSY is enabled
 *
 * This structure stores data to control BTS
 *
 */
struct bts_stat {
	bool			stat_on;
	bool			bypass;
	unsigned int		arqos;
	unsigned int		awqos;
	unsigned int		rmo;
	unsigned int		wmo;
	unsigned int		qurgent_on;
	unsigned int		qurgent_ex;
	unsigned int		qurgent_th_r;
	unsigned int		qurgent_th_w;
	bool				odd_qurgent_offset;
	bool			blocking_on;
	unsigned int		qfull_limit_r;
	unsigned int		qfull_limit_w;
	unsigned int		qbusy_limit_r;
	unsigned int		qbusy_limit_w;
	unsigned int		qmax0_limit_r;
	unsigned int		qmax0_limit_w;
	unsigned int		qmax1_limit_r;
	unsigned int		qmax1_limit_w;
	unsigned int 		hurrylevel3mo_0;
	unsigned int		hurrylevel3mo_1;
	unsigned int		vc_cfg;
	void __iomem		*qos_va_base;
	bool			drex_on;
	struct drex_stat	*drex;
	bool			drex_pf_on;
	struct drex_pf_stat	*drex_pf;
};

struct drex_stat {
	unsigned int            write_flush_config_0;
	unsigned int            write_flush_config_1;
	unsigned int            drex_timeout[MAX_QOS + 1];
	unsigned int            vc_timer_th[VC_TIMER_TH_NR];
	unsigned int            cutoff_con;
	unsigned int            brb_cutoff_con;
	unsigned int            wdbuf_cutoff_con;
};

struct drex_pf_stat {
	unsigned int		pf_token_control;
	unsigned int		pf_token_threshold0;
	unsigned int            pf_rreq_thrt_con;
	unsigned int            pf_rreq_thrt_mo_p2;
	unsigned int            pf_qos_timer[PF_TIMER_NR];
};

/**
 * struct bts_info - BTS information
 * @name:	name of BTS
 * @pa_base:	Physical Address data of BTS
 * @va_base:	Virtual Address data of BTS
 * @status:	boolean value whether BTS is on/off
 * @type:	type of bts
 * @pd_name:	name of power domain
 * @pd_on:	whether related power domain is on/off
 * @stat:	list of array that contains BTS status data for QoS control
 * @ops:	operation function classified according to bts type
 *
 */
struct bts_info {
	const char		*name;

	unsigned int		pa_base;
	void __iomem		*va_base;

	bool			status;

	unsigned int		type;

	unsigned int		pd_id;
	bool			pd_on;

	struct bts_stat		*stat;
	struct bts_ops		*ops;
};

extern int register_btsops(struct bts_info *info);
extern int exynos_bts_debugfs_init(void);
#endif
