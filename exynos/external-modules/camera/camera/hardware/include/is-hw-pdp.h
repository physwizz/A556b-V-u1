/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_PDP_H
#define IS_PDP_H

#include "is-interface-sensor.h"
#include "pablo-internal-subdev-ctrl.h"
#include "pablo-work.h"

#define PDP_IRQ_NAME_LENGTH	32

#define PDP_STUCK_CNT		(20)
#define STAT_REG_MAX		(400)
#define MAX_NUM_INPUT_OTF_VC 	(4) /* vc number of each otf ch */

enum is_pdp_state {
	IS_PDP_SET_PARAM,
	IS_PDP_SET_PARAM_COPY,
	IS_PDP_ONESHOT_PENDING,
	IS_PDP_DISABLE_REQ_IN_FS,
	IS_PDP_WAITING_IDLE,
};

enum pdp_dbg_mode {
	PDP_DBG_LOG_LEV_1,
	PDP_DBG_LOG_LEV_2,
	PDP_DBG_REG_DUMP,
	PDP_DBG_DEFAULT_CONFIG,
	PDP_DBG_STAT_HPD_DUMP,
	PDP_DBG_STAT_VPD_DUMP,
	PDP_DBG_MAX,
};

struct paf_action {
	enum itf_vc_stat_type	type;
	vc_dma_notifier_t		notifier;
	void			*data;
	unsigned int		flags;
	const char		*name;
	struct list_head	list;
};

struct pdp_lic_lut {
	u32				mode;
	u32				param0;
	u32				param1;
	u32				param2;
	u32				param3;
};

enum pdp_irq_src {
	PDP_INT1,
	PDP_INT2,
	PDP_INT2_RDMA,
	PDP_INT_MAX,
};

struct pdp_stat_reg {
	u32 reg_addr;
	u32 reg_data;
};

enum pdp_subdev_id {
	PDP_SUBDEV_STAT,
	PDP_SUBDEV_NUM,
};

struct is_pdp {
	struct device			*dev;
	struct is_hw_ip			*hw_ip;
	u32				id;
	u32				max_num;
	u32				prev_instance;
	void __iomem			*base;
	void __iomem			*cmn_base;
	resource_size_t			regs_start;
	resource_size_t			regs_end;
	int				irq[PDP_INT_MAX];
	char				irq_name[PDP_INT_MAX][PDP_IRQ_NAME_LENGTH];
	u32				irq_state[PDP_INT_MAX];
	struct mutex			control_lock;

	bool				hw_fro_en;

	void __iomem			*mux_base; /* select CSIS ch(e.g. CSIS0~CSIS5) */
	u32				*mux_val;
	u32				mux_elems;

	u32				ctx_num;

	void __iomem			*vc_mux_base; /* select VC ch in single CSIS(e.g. VC0~VC3) */
	u32	 			*vc_mux_val;
	u32				vc_mux_elems;

	void __iomem			*en_base; /* enable CSIS ch gate(e.g. CSIS0~CSIS5)*/
	u32				*en_val;
	u32				en_elems;

	unsigned long			state;

	struct is_pdp_ops		*pdp_ops;
	struct v4l2_subdev		*subdev; /* connected module subdevice */

	spinlock_t			slock_paf_action;
	struct list_head		list_of_paf_action;
	spinlock_t			slock_paf_s_param;
	spinlock_t			slock_oneshot;
	spinlock_t			slock_shot;

	struct workqueue_struct		*wq_stat;
	struct work_struct		work_stat[WORK_PDP_MAX];
	struct is_work_list		work_list[WORK_PDP_MAX];
	struct is_framemgr              *stat_framemgr;

	struct pdp_stat_reg 		regs[STAT_REG_MAX]; /* RTA CR setting buffer */
	u32				regs_size;

	const char			*int1_str[BITS_PER_LONG];
	const char			*int2_str[BITS_PER_LONG];

	bool				stat_enable;

	struct is_sensor_cfg		*sensor_cfg;

	u32				csi_ch;
	u32 otf_id;

	/* debug */
	unsigned long long		time_rta_cfg;
	unsigned long long		time_err;

	u32				cur_frm_time;

	/* IMG/AF input VC */
	u32				mux_offset;

	/* icpu */
	struct pablo_icpu_adt		*icpu_adt;

	u32 pd_mode;

	struct pablo_internal_subdev i_subdev[IS_STREAM_COUNT][PDP_SUBDEV_NUM];
};

struct platform_driver *pablo_paf_pdp_get_platform_driver(void);
#endif
