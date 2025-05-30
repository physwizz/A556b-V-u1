// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 * Pablo v9.1 specific functions
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#if IS_ENABLED(CONFIG_EXYNOS_SCI)
#include <soc/samsung/exynos-sci.h>
#endif

#include "is-config.h"
#include "is-param.h"
#include "is-type.h"
#include "is-core.h"
#include "is-hw-chain.h"
#include "is-device-sensor.h"
#include "is-device-csi.h"
#include "is-device-ischain.h"

#include "../../interface/is-interface-ischain.h"
#include "is-hw.h"
#include "../../interface/is-interface-library.h"
#include "votf/pablo-votf.h"
#include "pablo-device-iommu-group.h"
#include "pablo-irq.h"
#include "is-dvfs-config.h"

/* SYSREG register description */
/* SYSREG_CSIS REG, FIELD */
static const struct is_reg sysreg_csis_regs[SYSREG_CSIS_REG_CNT] = {
	{0x0108, "MEMCLK"},
	{0x0408, "CSIS_PDP_SC_CON0"},
	{0x040C, "CSIS_PDP_SC_CON1"},
	{0x0410, "CSIS_PDP_SC_CON3"},
	{0x0414, "CSIS_PDP_SC_CON4"},
	{0x0418, "CSIS_PDP_SC_CON5"},
	{0x041C, "CSIS_PDP_SC_CON6"},
	{0x0430, "CSIS_PDP_VC_CON0"},
	{0x0434, "CSIS_PDP_VC_CON1"},
	{0x0438, "CSIS_PDP_VC_CON2"},
	{0x043C, "CSIS_PDP_VC_CON3"},
	{0x0440, "CSIS_FRAME_ID_EN"},
	{0x0444, "CSIS_PDP_SC_PDP3_IN_EN"},
	{0x0448, "CSIS_PDP_SC_PDP2_IN_EN"},
	{0x044C, "CSIS_PDP_SC_PDP1_IN_EN"},
	{0x0450, "CSIS_PDP_SC_PDP0_IN_EN"},
	{0x0470, "MIPI_PHY_CON"},
	{0x0474, "MIPI_PHY_SEL"},
	{0x048C, "CSIS_PDP_SC_CON2"},
};

static const struct is_field sysreg_csis_fields[SYSREG_CSIS_REG_FIELD_CNT] = {
	{"EN", 0, 1, RW, 0x1},
	{"GLUEMUX_PDP0_VAL", 0, 4, RW, 0x0},
	{"GLUEMUX_PDP1_VAL", 0, 4, RW, 0x0},
	{"GLUEMUX_CSIS_DMA0_OTF_SEL", 0, 5, RW, 0x0},
	{"GLUEMUX_CSIS_DMA1_OTF_SEL", 0, 4, RW, 0x0},
	{"GLUEMUX_CSIS_DMA2_OTF_SEL", 0, 4, RW, 0x0},
	{"GLUEMUX_CSIS_DMA3_OTF_SEL", 0, 4, RW, 0x0},
	{"MUX_IMG_VC_PDP0", 16, 3, RW, 0x0},
	{"MUX_AF_VC_PDP0", 0, 3, RW, 0x1},
	{"MUX_IMG_VC_PDP1", 16, 3, RW, 0x0},
	{"MUX_AF_VC_PDP1", 0, 3, RW, 0x1},
	{"MUX_IMG_VC_PDP2", 16, 3, RW, 0x0},
	{"MUX_AF_VC_PDP2", 0, 3, RW, 0x1},
	{"FRAME_ID_EN_CSIS5", 5, 1, RW, 0x0},
	{"FRAME_ID_EN_CSIS4", 4, 1, RW, 0x0},
	{"FRAME_ID_EN_CSIS3", 3, 1, RW, 0x0},
	{"FRAME_ID_EN_CSIS2", 2, 1, RW, 0x0},
	{"FRAME_ID_EN_CSIS1", 1, 1, RW, 0x0},
	{"FRAME_ID_EN_CSIS0", 0, 1, RW, 0x0},
	{"PDP2_IN_CSIS5_EN", 5, 1, RW, 0x0},
	{"PDP2_IN_CSIS4_EN", 4, 1, RW, 0x0},
	{"PDP2_IN_CSIS3_EN", 3, 1, RW, 0x0},
	{"PDP2_IN_CSIS2_EN", 2, 1, RW, 0x0},
	{"PDP2_IN_CSIS1_EN", 1, 1, RW, 0x0},
	{"PDP2_IN_CSIS0_EN", 0, 1, RW, 0x0},
	{"PDP1_IN_CSIS5_EN", 5, 1, RW, 0x0},
	{"PDP1_IN_CSIS4_EN", 4, 1, RW, 0x0},
	{"PDP1_IN_CSIS3_EN", 3, 1, RW, 0x0},
	{"PDP1_IN_CSIS2_EN", 2, 1, RW, 0x0},
	{"PDP1_IN_CSIS1_EN", 1, 1, RW, 0x0},
	{"PDP1_IN_CSIS0_EN", 0, 1, RW, 0x0},
	{"PDP0_IN_CSIS5_EN", 5, 1, RW, 0x0},
	{"PDP0_IN_CSIS4_EN", 4, 1, RW, 0x0},
	{"PDP0_IN_CSIS3_EN", 3, 1, RW, 0x0},
	{"PDP0_IN_CSIS2_EN", 2, 1, RW, 0x0},
	{"PDP0_IN_CSIS1_EN", 1, 1, RW, 0x0},
	{"PDP0_IN_CSIS0_EN", 0, 1, RW, 0x0},
	{"MIPI_RESETN_DPHY_S3", 5, 1, RW, 0x0},
	{"MIPI_RESETN_DPHY_S2", 4, 1, RW, 0x0},
	{"MIPI_RESETN_DPHY_S1", 3, 1, RW, 0x0},
	{"MIPI_RESETN_DPHY_S", 2, 1, RW, 0x0},
	{"MIPI_RESETN_DCPHY_S1", 1, 1, RW, 0x0},
	{"MIPI_RESETN_DCPHY_S", 0, 1, RW, 0x0},
	{"MIPI_SEPARATION_SEL", 0, 3, RW, 0x0},
	{"GLUEMUX_PDP2_VAL", 0, 4, RW, 0x0},
};

/* SYSREG_TAA REG, FIELD */
static const struct is_reg sysreg_taa_regs[SYSREG_TAA_REG_CNT] = {
	{0X0108, "MEMCLK"},
	{0X0404, "TAA_USER_CON1"},
};

static const struct is_field sysreg_taa_fields[SYSREG_TAA_REG_FIELD_CNT] = {
	{"EN", 0, 1, RW, 0x1},	/* 0x108 */
	{"GLUEMUX_OTFOUT_SEL", 0, 2, RW, 0x0},  /* 0x404 */
};

/* SYSREG_TNR REG, FIELD */
static const struct is_reg sysreg_tnr_regs[SYSREG_TNR_REG_CNT] = {
	{0x0108, "MEMCLK"},
	{0x0400, "TNR_USER_CON0"},
};

static const struct is_field sysreg_tnr_fields[SYSREG_TNR_REG_FIELD_CNT] = {
	{"EN", 0, 1, RW, 0x1}, /* 0x108 */
	{"SW_RESETN_LHS_AST_GLUE_OTF1_TNRISP", 7, 1, RW, 0x1}, /* 0x400 */
	{"TYPE_LHS_AST_GLUE_OTF1_TNRISP", 5, 2, RW, 0x1},
	{"EN_OTF_IN_LH_AST_SI_OTF1_TNRISP", 4, 1, RW, 0x1},
	{"SW_RESETN_LHS_AST_GLUE_OTF0_TNRISP", 3, 1, RW, 0x1},
	{"TYPE_LHS_AST_GLUE_OTF0_TNRISP", 1, 2, RW, 0x1},
	{"EN_OTF_IN_LH_AST_SI_OTF0_TNRISP", 0, 1, RW, 0x1},
};

/* SYSREG_ISP REG, FIELD */
static const struct is_reg sysreg_isp_regs[SYSREG_ISP_REG_CNT] = {
	{0x0108, "MEMCLK"},
	{0x0414, "ISP_USER_CON3"},
};

static const struct is_field sysreg_isp_fields[SYSREG_ISP_REG_FIELD_CNT] = {
	{"EN", 0, 1, RW, 0x1}, /* 0x108 */
	{"OTF_SEL", 0, 1, RW, 0x1},
};

static const struct is_reg sysreg_mcsc_regs[SYSREG_MCSC_REG_CNT] = {
	{0x0108, "MEMCLK"},
	{0x0408, "MCSC_USER_CON2"},
};

static const struct is_field sysreg_mcsc_fields[SYSREG_MCSC_REG_FIELD_CNT] = {
	{"EN", 0, 1, RW, 0x1}, /* 0x108 */
	{"EN_OTF_IN_LH_AST_MI_OTF_ISPMCSC", 0, 1, RW, 0x1}, /* 0x408 */
};

static void __iomem *hwfc_rst;

static const enum is_subdev_id subdev_id[GROUP_SLOT_MAX] = {
	[GROUP_SLOT_SENSOR] = ENTRY_SENSOR,
	[GROUP_SLOT_PAF] = ENTRY_PAF,
	[GROUP_SLOT_3AA] = ENTRY_3AA,
	[GROUP_SLOT_ORB] = ENTRY_ORB,
	[GROUP_SLOT_ISP] = ENTRY_ISP,
	[GROUP_SLOT_MCS] = ENTRY_MCS,
};

static const char *const subdev_id_name[GROUP_SLOT_MAX] = {
	[GROUP_SLOT_SENSOR] = "SSX",
	[GROUP_SLOT_PAF] = "PXS",
	[GROUP_SLOT_3AA] = "3XS",
	[GROUP_SLOT_ORB] = "ORB",
	[GROUP_SLOT_ISP] = "ISP",
	[GROUP_SLOT_MCS] = "MXS",
};

static const struct is_subdev_ops *(*subdev_ops[GROUP_SLOT_MAX])(void) = {
	[GROUP_SLOT_SENSOR] = pablo_get_is_subdev_sensor_ops,
	[GROUP_SLOT_PAF] = pablo_get_is_subdev_paf_ops,
	[GROUP_SLOT_3AA] = pablo_get_is_subdev_3aa_ops,
	[GROUP_SLOT_ORB] = pablo_get_is_subdev_orb_ops,
	[GROUP_SLOT_ISP] = pablo_get_is_subdev_isp_ops,
	[GROUP_SLOT_MCS] = pablo_get_is_subdev_mcs_ops,
};
static const int ioresource_to_hw_id[IORESOURCE_MAX] = {
	[0 ... IORESOURCE_MAX - 1] = DEV_HW_END,
	[IORESOURCE_3AA0] = DEV_HW_3AA0,
	[IORESOURCE_3AA1] = DEV_HW_3AA1,
	[IORESOURCE_3AA2] = DEV_HW_3AA2,
	[IORESOURCE_ORBMCH0] = DEV_HW_ORB0,
	[IORESOURCE_ITP] = DEV_HW_ISP0,
	[IORESOURCE_MCSC] = DEV_HW_MCSC0,
};

static const u32 s2mpu_address_table[SYSMMU_DMAX_S2] = {
	[SYSMMU_D0_CSIS_S2 ... SYSMMU_DMAX_S2 - 1] = 0,
	[SYSMMU_D0_CSIS_S2] = 0x15120000,
	[SYSMMU_D1_CSIS_S2] = 0x15150000,
	[SYSMMU_D2_CSIS_S2] = 0x15180000,
	[SYSMMU_D3_CSIS_S2] = 0x151B0000,
	[SYSMMU_D0_TAA_S2] = 0x15590000,
	[SYSMMU_D0_TNR_S2] = 0x153C0000,
	[SYSMMU_D1_TNR_S2] = 0x153F0000,
	[SYSMMU_D0_ISP_S2] = 0x154A0000,
	[SYSMMU_D0_MCSC_S2] = 0x156A0000,
	[SYSMMU_D1_MCSC_S2] = 0x156D0000,
};

const int *is_hw_get_ioresource_to_hw_id(void)
{
	return ioresource_to_hw_id;
}

static inline void __is_isr_host(void *data, int handler_id)
{
	struct is_interface_hwip *itf_hw = NULL;
	struct hwip_intr_handler *intr_hw = NULL;

	itf_hw = (struct is_interface_hwip *)data;
	intr_hw = &itf_hw->handler[handler_id];

	if (intr_hw->valid)
		intr_hw->handler(intr_hw->id, (void *)itf_hw->hw_ip);
	else
		err_itfc("[ID:%d] empty handler handler_id:%d!!", itf_hw->id, handler_id);
}

/* 3AA0 */
static irqreturn_t __is_isr1_3aa0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP1);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr2_3aa0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP2);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr3_3aa0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP3);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr4_3aa0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP4);
	return IRQ_HANDLED;
}

/* 3AA1 */
static irqreturn_t __is_isr1_3aa1(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP1);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr2_3aa1(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP2);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr3_3aa1(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP3);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr4_3aa1(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP4);
	return IRQ_HANDLED;
}

/* 3AA2 */
static irqreturn_t __is_isr1_3aa2(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP1);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr2_3aa2(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP2);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr3_3aa2(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP3);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr4_3aa2(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP4);
	return IRQ_HANDLED;
}

/* ITP0 */
static irqreturn_t __is_isr1_isp0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP1);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr2_isp0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP2);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr3_isp0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP3);
	return IRQ_HANDLED;
}

static irqreturn_t __is_isr4_isp0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP4);
	return IRQ_HANDLED;
}

/* ORBMCH */
static irqreturn_t __is_isr1_orbmch0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP1);
	return IRQ_HANDLED;
}

/* MCSC */
static irqreturn_t __is_isr1_mcs0(int irq, void *data)
{
	__is_isr_host(data, INTR_HWIP1);
	return IRQ_HANDLED;
}

/*
 * HW group related functions
 */
void is_hw_get_subdev_info(u32 slot, u32 *id, const char **name, const struct is_subdev_ops **sops)
{
	*id = subdev_id[slot];
	*name = subdev_id_name[slot];
	*sops = subdev_ops[slot]();
}

int is_hw_group_open(void *group_data)
{
	int ret = 0;
	u32 group_id;
	struct is_subdev *leader;
	struct is_group *group;
	struct is_device_ischain *device;

	FIMC_BUG(!group_data);

	group = group_data;
	leader = &group->leader;
	device = group->device;
	group_id = group->id;

	switch (group_id) {
	case GROUP_ID_SS0:
	case GROUP_ID_SS1:
	case GROUP_ID_SS2:
	case GROUP_ID_SS3:
	case GROUP_ID_SS4:
	case GROUP_ID_SS5:
		leader->constraints_width = GROUP_SENSOR_MAX_WIDTH;
		leader->constraints_height = GROUP_SENSOR_MAX_HEIGHT;
		break;
	case GROUP_ID_PAF0:
	case GROUP_ID_PAF1:
	case GROUP_ID_PAF2:
		leader->constraints_width = GROUP_PDP_MAX_WIDTH;
		leader->constraints_height = GROUP_PDP_MAX_HEIGHT;
		break;
	case GROUP_ID_3AA0:
	case GROUP_ID_3AA1:
	case GROUP_ID_3AA2:
		leader->constraints_width = GROUP_3AA_MAX_WIDTH;
		leader->constraints_height = GROUP_3AA_MAX_HEIGHT;
		break;
	case GROUP_ID_ISP0:
	case GROUP_ID_MCS0:
		leader->constraints_width = GROUP_ISP_MAX_WIDTH;
		leader->constraints_height = GROUP_ISP_MAX_HEIGHT;
		break;
	case GROUP_ID_ORB0:
		leader->constraints_width = GROUP_ORBMCH_MAX_WIDTH;
		leader->constraints_height = GROUP_ORBMCH_MAX_HEIGHT;
		break;
	default:
		merr("(%s) is invalid", group, group_id_name[group_id]);
		break;
	}

	return ret;
}

int is_get_hw_list(int group_id, int *hw_list)
{
	int i;
	int hw_index = 0;

	/* initialization */
	for (i = 0; i < GROUP_HW_MAX; i++)
		hw_list[i] = -1;

	switch (group_id) {
	case GROUP_ID_PAF0:
		hw_list[hw_index] = DEV_HW_PAF0; hw_index++;
		break;
	case GROUP_ID_PAF1:
		hw_list[hw_index] = DEV_HW_PAF1; hw_index++;
		break;
	case GROUP_ID_PAF2:
		hw_list[hw_index] = DEV_HW_PAF2; hw_index++;
		break;
	case GROUP_ID_3AA0:
		hw_list[hw_index] = DEV_HW_3AA0; hw_index++;
		break;
	case GROUP_ID_3AA1:
		hw_list[hw_index] = DEV_HW_3AA1; hw_index++;
		break;
	case GROUP_ID_3AA2:
		hw_list[hw_index] = DEV_HW_3AA2; hw_index++;
		break;
	case GROUP_ID_ORB0:
		hw_list[hw_index] = DEV_HW_ORB0; hw_index++;
		break;
	case GROUP_ID_ISP0:
		hw_list[hw_index] = DEV_HW_ISP0; hw_index++;
		break;
	case GROUP_ID_MCS0:
		hw_list[hw_index] = DEV_HW_MCSC0; hw_index++;
		break;
	case GROUP_ID_MAX:
		break;
	default:
		err("Invalid group%d(%s)", group_id, group_id_name[group_id]);
		break;
	}

	return hw_index;
}
/*
 * System registers configurations
 */
static inline int __is_hw_get_address(struct platform_device *pdev,
				struct is_interface_hwip *itf_hwip,
				int hw_id, char *hw_name,
				u32 resource_id, enum base_reg_index reg_index,
				bool alloc_memlog)
{
	struct resource *mem_res = NULL;
	struct device *dev = &pdev->dev;

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, resource_id);
	if (!mem_res) {
		dev_err(&pdev->dev, "Failed to get io memory region\n");
		return -EINVAL;
	}

	itf_hwip->hw_ip->regs_start[reg_index] = mem_res->start;
	itf_hwip->hw_ip->regs_end[reg_index] = mem_res->end;
	itf_hwip->hw_ip->regs[reg_index] =
		devm_ioremap(dev, mem_res->start, resource_size(mem_res));
	if (!itf_hwip->hw_ip->regs[reg_index]) {
		dev_err(&pdev->dev, "Failed to remap io region\n");
		return -EINVAL;
	}

	if (alloc_memlog)
		is_debug_memlog_alloc_dump(mem_res->start, 0, resource_size(mem_res), hw_name);

	info_itfc("[ID:%2d] %s VA(0x%lx)\n", hw_id, hw_name,
		(ulong)itf_hwip->hw_ip->regs[reg_index]);

	return 0;
}

int is_hw_get_address(void *itfc_data, void *pdev_data, int hw_id)
{
	struct platform_device *pdev = NULL;
	struct is_interface_hwip *itf_hwip = NULL;
	int idx;

	FIMC_BUG(!itfc_data);
	FIMC_BUG(!pdev_data);

	itf_hwip = (struct is_interface_hwip *)itfc_data;
	pdev = (struct platform_device *)pdev_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		__is_hw_get_address(pdev, itf_hwip, hw_id, "3AA0", IORESOURCE_3AA0, REG_SETA, false);
		/* TODO: need check if exist dump_region */
		idx = 0;
		itf_hwip->hw_ip->dump_region[REG_SETA][idx].start = 0x0;
		itf_hwip->hw_ip->dump_region[REG_SETA][idx++].end = 0x1FAF;
		itf_hwip->hw_ip->dump_region[REG_SETA][idx].start = 0x1FC0;
		itf_hwip->hw_ip->dump_region[REG_SETA][idx++].end = 0x9FAF;
		itf_hwip->hw_ip->dump_region[REG_SETA][idx].start = 0x9FC0;
		itf_hwip->hw_ip->dump_region[REG_SETA][idx++].end = 0xFFFF;

		__is_hw_get_address(pdev, itf_hwip, hw_id, "3AA0", IORESOURCE_3AA0, REG_EXT1, false);
		__is_hw_get_address(pdev, itf_hwip, hw_id, "ZSL0 DMA", IORESOURCE_ZSL0_DMA, REG_EXT2, false);
		__is_hw_get_address(pdev, itf_hwip, hw_id, "STRP0 DMA", IORESOURCE_STRP0_DMA, REG_EXT3, false);
		break;
	case DEV_HW_3AA1:
		__is_hw_get_address(pdev, itf_hwip, hw_id, "3AA1", IORESOURCE_3AA1, REG_SETA, false);
		__is_hw_get_address(pdev, itf_hwip, hw_id, "3AA0", IORESOURCE_3AA0, REG_EXT1, false);
		__is_hw_get_address(pdev, itf_hwip, hw_id, "ZSL1 DMA", IORESOURCE_ZSL1_DMA, REG_EXT2, false);
		__is_hw_get_address(pdev, itf_hwip, hw_id, "STRP1 DMA", IORESOURCE_STRP1_DMA, REG_EXT3, false);
		break;
	case DEV_HW_3AA2:
		__is_hw_get_address(pdev, itf_hwip, hw_id, "3AA2", IORESOURCE_3AA2, REG_SETA, false);
		__is_hw_get_address(pdev, itf_hwip, hw_id, "3AA0", IORESOURCE_3AA0, REG_EXT1, false);
		__is_hw_get_address(pdev, itf_hwip, hw_id, "ZSL2 DMA", IORESOURCE_ZSL2_DMA, REG_EXT2, false);
		__is_hw_get_address(pdev, itf_hwip, hw_id, "STRP2 DMA", IORESOURCE_STRP2_DMA, REG_EXT3, false);
		break;
	case DEV_HW_ORB0:
		__is_hw_get_address(pdev, itf_hwip, hw_id, "ORBMCH", IORESOURCE_ORBMCH0, REG_SETA, false);
		break;
	case DEV_HW_ISP0:
		__is_hw_get_address(pdev, itf_hwip, hw_id, "ITP", IORESOURCE_ITP, REG_SETA, true);

		__is_hw_get_address(pdev, itf_hwip, hw_id, "MCFP0", IORESOURCE_MCFP0, REG_EXT1, false);

		idx = 0;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x0;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x03FF;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x0500;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x05FF;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x0800;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x09FF;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x1800;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x1AFF;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x2000;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x23FF;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x2600;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x2BFF;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x3800;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x39FF;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x4000;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x4DFF;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx].start = 0x5200;
		itf_hwip->hw_ip->dump_region[REG_EXT1][idx++].end = 0x55FF;

		__is_hw_get_address(pdev, itf_hwip, hw_id, "DNS", IORESOURCE_DNS, REG_EXT2, true);

		__is_hw_get_address(pdev, itf_hwip, hw_id, "MCFP1", IORESOURCE_MCFP1, REG_EXT3, false);

		idx = 0;
		itf_hwip->hw_ip->dump_region[REG_EXT3][idx].start = 0x0;
		itf_hwip->hw_ip->dump_region[REG_EXT3][idx++].end = 0x0FFF;
		itf_hwip->hw_ip->dump_region[REG_EXT3][idx].start = 0x2900;
		itf_hwip->hw_ip->dump_region[REG_EXT3][idx++].end = 0x3CFF;
		break;
	case DEV_HW_MCSC0:
		__is_hw_get_address(pdev, itf_hwip, hw_id, "MCSC0", IORESOURCE_MCSC, REG_SETA, true);
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
	}

	return 0;
}

int is_hw_get_irq(void *itfc_data, void *pdev_data, int hw_id)
{
	struct is_interface_hwip *itf_hwip = NULL;
	struct platform_device *pdev = NULL;
	int ret = 0;

	FIMC_BUG(!itfc_data);

	itf_hwip = (struct is_interface_hwip *)itfc_data;
	pdev = (struct platform_device *)pdev_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 0);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 1);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa0-2\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP3] = platform_get_irq(pdev, 2);
		if (itf_hwip->irq[INTR_HWIP3] < 0) {
			err("Failed to get irq 3aa0 zsl dma\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP4] = platform_get_irq(pdev, 3);
		if (itf_hwip->irq[INTR_HWIP4] < 0) {
			err("Failed to get irq 3aa0 strp dma\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_3AA1:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 4);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa1-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 5);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa1-2\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP3] = platform_get_irq(pdev, 6);
		if (itf_hwip->irq[INTR_HWIP3] < 0) {
			err("Failed to get irq 3aa1 zsl dma\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP4] = platform_get_irq(pdev, 7);
		if (itf_hwip->irq[INTR_HWIP4] < 0) {
			err("Failed to get irq 3aa1 strp dma\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_3AA2:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 8);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq 3aa2-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 9);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq 3aa2-2\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP3] = platform_get_irq(pdev, 10);
		if (itf_hwip->irq[INTR_HWIP3] < 0) {
			err("Failed to get irq 3aa2 zsl dma\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP4] = platform_get_irq(pdev, 11);
		if (itf_hwip->irq[INTR_HWIP4] < 0) {
			err("Failed to get irq 3aa2 strp dma\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ORB0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 12);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq ORBMCH0-1\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_ISP0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 13);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq isp0-1\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP2] = platform_get_irq(pdev, 14);
		if (itf_hwip->irq[INTR_HWIP2] < 0) {
			err("Failed to get irq isp0-2\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP3] = platform_get_irq(pdev, 15);
		if (itf_hwip->irq[INTR_HWIP3] < 0) {
			err("Failed to get irq tnr0\n");
			return -EINVAL;
		}

		itf_hwip->irq[INTR_HWIP4] = platform_get_irq(pdev, 16);
		if (itf_hwip->irq[INTR_HWIP4] < 0) {
			err("Failed to get irq tnr1\n");
			return -EINVAL;
		}
		break;
	case DEV_HW_MCSC0:
		itf_hwip->irq[INTR_HWIP1] = platform_get_irq(pdev, 17);
		if (itf_hwip->irq[INTR_HWIP1] < 0) {
			err("Failed to get irq mcsc0\n");
			return -EINVAL;
		}
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
	}

	return ret;
}

static inline int __is_hw_request_irq(struct is_interface_hwip *itf_hwip,
	const char *name, int isr_num,
	unsigned int added_irq_flags,
	irqreturn_t (*func)(int, void *))
{
	size_t name_len = 0;
	int ret = 0;

	name_len = sizeof(itf_hwip->irq_name[isr_num]);
	snprintf(itf_hwip->irq_name[isr_num], name_len, "%s-%d", name, isr_num);

	ret = pablo_request_irq(itf_hwip->irq[isr_num], func,
		itf_hwip->irq_name[isr_num],
		added_irq_flags,
		itf_hwip);
	if (ret) {
		err_itfc("[HW:%s] request_irq [%d] fail", name, isr_num);
		return -EINVAL;
	}

	itf_hwip->handler[isr_num].id = isr_num;
	itf_hwip->handler[isr_num].valid = true;

	return ret;
}

static inline int __is_hw_free_irq(struct is_interface_hwip *itf_hwip, int isr_num)
{
	pablo_free_irq(itf_hwip->irq[isr_num], itf_hwip);

	return 0;
}

int is_hw_request_irq(void *itfc_data, int hw_id)
{
	struct is_interface_hwip *itf_hwip = NULL;
	int ret = 0;

	FIMC_BUG(!itfc_data);


	itf_hwip = (struct is_interface_hwip *)itfc_data;

	switch (hw_id) {
	case DEV_HW_3AA0:
		ret = __is_hw_request_irq(itf_hwip, "3a0-0", INTR_HWIP1, IRQF_TRIGGER_NONE, __is_isr1_3aa0);
		ret = __is_hw_request_irq(itf_hwip, "3a0-1", INTR_HWIP2, IRQF_TRIGGER_NONE, __is_isr2_3aa0);
		ret = __is_hw_request_irq(itf_hwip, "3a0-zsl", INTR_HWIP3, IRQF_TRIGGER_NONE, __is_isr3_3aa0);
		ret = __is_hw_request_irq(itf_hwip, "3a0-strp", INTR_HWIP4, IRQF_SHARED, __is_isr4_3aa0);
		break;
	case DEV_HW_3AA1:
		ret = __is_hw_request_irq(itf_hwip, "3a1-0", INTR_HWIP1, IRQF_TRIGGER_NONE, __is_isr1_3aa1);
		ret = __is_hw_request_irq(itf_hwip, "3a1-1", INTR_HWIP2, IRQF_TRIGGER_NONE, __is_isr2_3aa1);
		ret = __is_hw_request_irq(itf_hwip, "3a1-zsl", INTR_HWIP3, IRQF_TRIGGER_NONE, __is_isr3_3aa1);
		ret = __is_hw_request_irq(itf_hwip, "3a1-strp", INTR_HWIP4, IRQF_TRIGGER_NONE, __is_isr4_3aa1);
		break;
	case DEV_HW_3AA2:
		ret = __is_hw_request_irq(itf_hwip, "3a2-0", INTR_HWIP1, IRQF_TRIGGER_NONE, __is_isr1_3aa2);
		ret = __is_hw_request_irq(itf_hwip, "3a2-1", INTR_HWIP2, IRQF_TRIGGER_NONE, __is_isr2_3aa2);
		ret = __is_hw_request_irq(itf_hwip, "3a2-zsl", INTR_HWIP3, IRQF_TRIGGER_NONE, __is_isr3_3aa2);
		ret = __is_hw_request_irq(itf_hwip, "3a2-strp", INTR_HWIP4, IRQF_TRIGGER_NONE, __is_isr4_3aa2);
		break;
	case DEV_HW_ORB0:
		/* To apply ORBMCH SW W/A, irq request for ORB was moved after power on */
		if (!IS_ENABLED(USE_ORBMCH_WA))
			ret = __is_hw_request_irq(itf_hwip, "orbmch0", INTR_HWIP1, IRQF_TRIGGER_NONE, __is_isr1_orbmch0);
		break;
	case DEV_HW_ISP0:
		ret = __is_hw_request_irq(itf_hwip, "dns-0", INTR_HWIP1, IRQF_TRIGGER_NONE, __is_isr1_isp0);
		ret = __is_hw_request_irq(itf_hwip, "dns-1", INTR_HWIP2, IRQF_TRIGGER_NONE, __is_isr2_isp0);
		ret = __is_hw_request_irq(itf_hwip, "tnr-0", INTR_HWIP3, IRQF_TRIGGER_NONE, __is_isr3_isp0);
		ret = __is_hw_request_irq(itf_hwip, "tnr-1", INTR_HWIP4, IRQF_TRIGGER_NONE, __is_isr4_isp0);
		break;
	case DEV_HW_MCSC0:
		ret = __is_hw_request_irq(itf_hwip, "mcs0", INTR_HWIP1, IRQF_TRIGGER_NONE, __is_isr1_mcs0);
		break;
	default:
		probe_err("hw_id(%d) is invalid", hw_id);
		return -EINVAL;
	}

	return ret;
}

int is_hw_s_ctrl(void *itfc_data, int hw_id, enum hw_s_ctrl_id id, void *val)
{
	int ret = 0;

	switch (id) {
	case HW_S_CTRL_FULL_BYPASS:
		break;
	case HW_S_CTRL_CHAIN_IRQ:
		break;
	case HW_S_CTRL_HWFC_IDX_RESET:
		if (hw_id == IS_VIDEO_M2P_NUM) {
			struct is_video_ctx *vctx = (struct is_video_ctx *)itfc_data;
			struct is_device_ischain *device;
			unsigned long data = (unsigned long)val;

			FIMC_BUG(!vctx);
			FIMC_BUG(!GET_DEVICE(vctx));

			device = GET_DEVICE(vctx);

			/* reset if this instance is reprocessing */
			if (test_bit(IS_ISCHAIN_REPROCESSING, &device->state))
				writel(data, hwfc_rst);
		}
		break;
	case HW_S_CTRL_MCSC_SET_INPUT:
		{
			unsigned long mode = (unsigned long)val;

			info_itfc("%s: mode(%lu)\n", __func__, mode);
		}
		break;
	default:
		break;
	}

	return ret;
}

void is_hw_camif_init(void)
{
	/* TODO */
}

#if (IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON))
static void is_hw_tzpc_info(void)
{
	void __iomem *reg;
	int i, j;

	pr_info("[DBG] TZPC disable\n");

	/* CSIS WDMA */
	reg = ioremap(0x15090080, 0x20);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);

	/* PDP DMA */
	reg = ioremap(0x150C0210, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);
	reg = ioremap(0x150D0210, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);
	reg = ioremap(0x150E0210, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);

	/* 3AA DMA */
	reg = ioremap(0x15543b10, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);
	reg = ioremap(0x15553b10, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);
	reg = ioremap(0x15563b10, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);

	/* ORB DMA */
	reg = ioremap(0x15750050, 0x10);
	for (i = 0, j = 0; i < 4; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);
	reg = ioremap(0x15750700, 0x10);
	for (i = 0, j = 0; i < 4; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);

	/* TNR DMA */
	reg = ioremap(0x14B00810, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);
	reg = ioremap(0x14B40810, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);

	/* DNS/ITP DMA */
	reg = ioremap(0x15434010, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);

	/* MCSC DMA */
	reg = ioremap(0x156401A0, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);
	/* GDC DMA */
	reg = ioremap(0x15660610, 0x30);
	for (i = 0, j = 0; i < 8; j = i * 0x04, i++)
		writel(0xFFFFFFFF, reg + j);
	iounmap(reg);
}

void is_hw_s2mpu_cfg(void)
{
	void __iomem *reg;
	int idx;

	pr_info("[DBG] S2MPU disable\n");

	for (idx = 0; idx < SYSMMU_DMAX_S2; idx++) {
		if (!s2mpu_address_table[idx])
			continue;

		reg = ioremap(s2mpu_address_table[idx], 0x4);
		writel(0x0, reg);
		iounmap(reg);
	}
}
PST_EXPORT_SYMBOL(is_hw_s2mpu_cfg);
#endif

int is_hw_camif_cfg(void *sensor_data)
{
	int ret = 0;
	int i;
	void __iomem *csis_sys_regs;
	struct is_core *core;
	struct is_device_sensor *sensor;
	struct is_device_csi *csi;
	struct pablo_camif_otf_info *otf_info;
	bool csi_f_enabled = false;
	bool csi_e_enabled = false;
	u32 sensor_open_cnt = 1;

	FIMC_BUG(!sensor_data);

	sensor = (struct is_device_sensor *)sensor_data;

	core = (struct is_core *)sensor->private_data;
	if (!core) {
		merr("core is null\n", sensor);
		ret = -ENODEV;
		return ret;
	}

	csi = (struct is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		merr("csi is null\n", sensor);
		ret = -ENODEV;
		return ret;
	}

	otf_info = &csi->otf_info;

	csis_sys_regs = ioremap(SYSREG_CSIS_BASE_ADDR, 0x1000);

	/* When more than one is opened */
	for (i = 0; i < IS_SENSOR_COUNT; i++) {
		if (test_bit(IS_SENSOR_OPEN, &(core->sensor[i].state))
				&& core->sensor[i].device_id != sensor->device_id) {
			csi = (struct is_device_csi *)v4l2_get_subdevdata(core->sensor[i].subdev_csi);
			if (!csi) {
				merr("csi is null\n", sensor);
				ret = -ENODEV;
				iounmap(csis_sys_regs);
				return ret;
			}
			sensor_open_cnt++;

			if (otf_info->csi_ch == CSI_ID_E) {
				info("remain mipi phy mux val for CSI_E");
				csi_e_enabled = true;
			} else if (otf_info->csi_ch == CSI_ID_F) {
				info("remain mipi phy mux val for CSI_F");
				csi_f_enabled = true;
			}
		}
	}

	/* When only one is open */
	if (sensor_open_cnt == 1 && test_bit(IS_SENSOR_OPEN, &sensor->state)) {
		if (otf_info->csi_ch == CSI_ID_E) {
			info("set mipi phy mux val for CSI_E");
			csi_e_enabled = true;
		} else if (otf_info->csi_ch == CSI_ID_F) {
			info("set mipi phy mux val for CSI_F");
			csi_f_enabled = true;
		}
	}

	if (csi_e_enabled && csi_f_enabled)		/* 2 + 1 + 1 */
		is_hw_set_field(csis_sys_regs,
			&sysreg_csis_regs[SYSREG_R_CSIS_MIPI_PHY_SEL],
			&sysreg_csis_fields[SYSREG_F_CSIS_MIPI_SEPARATION_SEL], BIT(2));
	else if (csi_e_enabled && !csi_f_enabled)	/* 2 + 2 */
		is_hw_set_field(csis_sys_regs,
			&sysreg_csis_regs[SYSREG_R_CSIS_MIPI_PHY_SEL],
			&sysreg_csis_fields[SYSREG_F_CSIS_MIPI_SEPARATION_SEL], BIT(1));
	else
		is_hw_set_field(csis_sys_regs,
			&sysreg_csis_regs[SYSREG_R_CSIS_MIPI_PHY_SEL],
			&sysreg_csis_fields[SYSREG_F_CSIS_MIPI_SEPARATION_SEL], BIT(0));

	iounmap(csis_sys_regs);

#if (IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON))
	is_hw_s2mpu_cfg();
	is_hw_tzpc_info();
#endif

	return ret;
}

static int is_hw_orbmch_isr_clear_register(u32 hw_id, bool enable)
{
	struct is_interface_hwip *itf_hwip = NULL;
	struct is_hw_ip *hw_ip = NULL;
	struct is_core *core = is_get_is_core();
	struct is_interface_ischain *itfc;
	int hw_slot = -1;
	int ret = 0;

	if (!IS_ENABLED(USE_ORBMCH_WA))
		return ret;

	itfc = &core->interface_ischain;

	/* SW WA for ORBMCH ISR */
	if (hw_id == DEV_HW_ORB0)
		hw_slot = CALL_HW_CHAIN_INFO_OPS(&core->hardware, get_hw_slot_id, DEV_HW_ORB0);

	if (hw_slot == -1)
		return ret;

	itf_hwip = &(itfc->itf_ip[hw_slot]);
	hw_ip = itf_hwip->hw_ip;
	writel(0x0, hw_ip->regs[REG_SETA] + 0x60);	/* isr all bit are disabled */
	writel(0x3FF, hw_ip->regs[REG_SETA] + 0x64);	/* isr all bit clear */

	if (enable) {
		dbg_hw(2, "%s: SW WA for ORBMCH[hw_id = %d]\n", __func__, hw_id);

		if (hw_id == DEV_HW_ORB0)
			ret = __is_hw_request_irq(itf_hwip, "orbmch0",
					INTR_HWIP1, IRQF_TRIGGER_NONE, __is_isr1_orbmch0);
	} else {
		dbg_hw(2, "%s: SW WA disable for ORBMCH[hw_id = %d]\n", __func__, hw_id);

		ret = __is_hw_free_irq(itf_hwip, INTR_HWIP1);
	}
	return ret;
}

int is_hw_ischain_enable(struct is_core *core)
{
	int ret = 0;
	struct is_interface_hwip *itf_hwip = NULL;
	struct is_interface_ischain *itfc;
	struct is_hardware *hw;
	int hw_slot;
	u32 idx, i;

	itfc = &core->interface_ischain;
	hw = &core->hardware;

	/* irq affinity should be restored after S2R at gic600 */
	for (idx = 0; idx < IORESOURCE_MAX; idx++) {
		if (ioresource_to_hw_id[idx] >= DEV_HW_END)
			continue;

		hw_slot = CALL_HW_CHAIN_INFO_OPS(hw, get_hw_slot_id, ioresource_to_hw_id[idx]);
		itf_hwip = &(itfc->itf_ip[hw_slot]);

		for (i = 0; i < INTR_HWIP_MAX; i++) {
			if (!itf_hwip->handler[i].valid)
				continue;

			pablo_set_affinity_irq(itf_hwip->irq[i], true);
		}
	}

	votfitf_disable_service();

	is_hw_orbmch_isr_clear_register(DEV_HW_ORB0, true);

	info("%s: complete\n", __func__);

	return ret;
}

int is_hw_ischain_disable(struct is_core *core)
{
	int ret = 0;

	is_hw_orbmch_isr_clear_register(DEV_HW_ORB0, false);

	info("%s: complete\n", __func__);

	return ret;
}

/* TODO: remove this, compile check only */
#ifdef ENABLE_HWACG_CONTROL
void is_hw_csi_qchannel_enable_all(bool enable)
{
	void __iomem *csi0_regs;
	void __iomem *csi1_regs;
	void __iomem *csi2_regs;
	void __iomem *csi3_regs;
	void __iomem *csi4_regs;
	void __iomem *csi5_regs;

	u32 reg_val;

	csi0_regs = ioremap(CSIS0_QCH_EN_ADDR, SZ_4);
	csi1_regs = ioremap(CSIS1_QCH_EN_ADDR, SZ_4);
	csi2_regs = ioremap(CSIS2_QCH_EN_ADDR, SZ_4);
	csi3_regs = ioremap(CSIS3_QCH_EN_ADDR, SZ_4);
	csi4_regs = ioremap(CSIS4_QCH_EN_ADDR, SZ_4);
	csi5_regs = ioremap(CSIS5_QCH_EN_ADDR, SZ_4);

	reg_val = readl(csi0_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi0_regs);

	reg_val = readl(csi1_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi1_regs);

	reg_val = readl(csi2_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi2_regs);

	reg_val = readl(csi3_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi3_regs);

	reg_val = readl(csi4_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi4_regs);

	reg_val = readl(csi5_regs);
	reg_val &= ~(1 << 20);
	writel(enable << 20 | reg_val, csi5_regs);

	iounmap(csi0_regs);
	iounmap(csi1_regs);
	iounmap(csi2_regs);
	iounmap(csi3_regs);
	iounmap(csi4_regs);
	iounmap(csi5_regs);
}
#endif

void is_hw_configure_llc(bool on, void *ischain, ulong *llc_state)
{
	dbg("not supported");
}
KUNIT_EXPORT_SYMBOL(is_hw_configure_llc);

void is_hw_configure_bts_scen(struct is_resourcemgr *resourcemgr, int scenario_id)
{
	int bts_index = 0;

	switch (scenario_id) {
	case IS_DVFS_SN_REAR_SINGLE_VIDEO_8K24:
	case IS_DVFS_SN_REAR_SINGLE_VIDEO_8K30:
		bts_index = 1;
		break;
	default:
		bts_index = 0;
		break;
	}

	/* If default scenario & specific scenario selected,
	 * off specific scenario first.
	 */
	if (resourcemgr->cur_bts_scen_idx && bts_index == 0)
		is_bts_scen(resourcemgr, resourcemgr->cur_bts_scen_idx, false);

	if (bts_index && bts_index != resourcemgr->cur_bts_scen_idx)
		is_bts_scen(resourcemgr, bts_index, true);
	resourcemgr->cur_bts_scen_idx = bts_index;
}

int is_hw_get_capture_slot(struct is_frame *frame, dma_addr_t **taddr, u64 **taddr_k, u32 vid)
{
	int ret = 0;

	if (taddr) *taddr = NULL;
	if (taddr_k) *taddr_k = NULL;

	switch(vid) {
	/* TAA */
	case IS_VIDEO_30C_NUM:
	case IS_VIDEO_31C_NUM:
	case IS_VIDEO_32C_NUM:
		*taddr = frame->txcTargetAddress;
		break;
	case IS_VIDEO_30P_NUM:
	case IS_VIDEO_31P_NUM:
	case IS_VIDEO_32P_NUM:
		*taddr = frame->txpTargetAddress;
		break;
	case IS_VIDEO_30G_NUM:
	case IS_VIDEO_31G_NUM:
	case IS_VIDEO_32G_NUM:
		*taddr = frame->mrgTargetAddress;
		break;
	case IS_VIDEO_30F_NUM:
	case IS_VIDEO_31F_NUM:
	case IS_VIDEO_32F_NUM:
		*taddr = frame->efdTargetAddress;
		break;
	case IS_VIDEO_30D_NUM:
	case IS_VIDEO_31D_NUM:
	case IS_VIDEO_32D_NUM:
		*taddr = frame->txdgrTargetAddress;
		break;
	case IS_VIDEO_30O_NUM:
	case IS_VIDEO_31O_NUM:
	case IS_VIDEO_32O_NUM:
		*taddr = frame->txodsTargetAddress;
		break;
	case IS_VIDEO_30L_NUM:
	case IS_VIDEO_31L_NUM:
	case IS_VIDEO_32L_NUM:
		*taddr = frame->txldsTargetAddress;
		break;
	case IS_VIDEO_30H_NUM:
	case IS_VIDEO_31H_NUM:
	case IS_VIDEO_32H_NUM:
		*taddr = frame->txhfTargetAddress;
		break;
	/* ISP */
	case IS_VIDEO_I0C_NUM:
		*taddr = frame->ixcTargetAddress;
		break;
	case IS_VIDEO_I0P_NUM:
		*taddr = frame->ixpTargetAddress;
		break;
	case IS_VIDEO_I0V_NUM:
		*taddr = frame->ixvTargetAddress;
		break;
	case IS_VIDEO_I0W_NUM:
		*taddr = frame->ixwTargetAddress;
		break;
	case IS_VIDEO_I0T_NUM:
		*taddr = frame->ixtTargetAddress;
		break;
	case IS_VIDEO_I0G_NUM:
		*taddr = frame->ixgTargetAddress;
		break;
	case IS_VIDEO_IMM_NUM:
		*taddr = frame->ixmTargetAddress;
		if (taddr_k)
			*taddr_k = frame->ixmKTargetAddress;
		break;
	case IS_VIDEO_IRG_NUM:
		*taddr = frame->ixrrgbTargetAddress;
		break;
	case IS_VIDEO_ISC_NUM:
		*taddr = frame->ixscmapTargetAddress;
		break;
	case IS_VIDEO_IDR_NUM:
		*taddr = frame->ixdgrTargetAddress;
		break;
	case IS_VIDEO_INR_NUM:
		*taddr = frame->ixnoirTargetAddress;
		break;
	case IS_VIDEO_IND_NUM:
		*taddr = frame->ixnrdsTargetAddress;
		break;
	case IS_VIDEO_IDG_NUM:
		*taddr = frame->ixdgaTargetAddress;
		break;
	case IS_VIDEO_ISH_NUM:
		*taddr = frame->ixsvhistTargetAddress;
		break;
	case IS_VIDEO_IHF_NUM:
		*taddr = frame->ixhfTargetAddress;
		break;
	case IS_VIDEO_INW_NUM:
		*taddr = frame->ixnoiTargetAddress;
		break;
	case IS_VIDEO_INRW_NUM:
		*taddr = frame->ixnoirwTargetAddress;
		break;
	case IS_VIDEO_IRGW_NUM:
		*taddr = frame->ixwrgbTargetAddress;
		break;
	case IS_VIDEO_INB_NUM:
		*taddr = frame->ixbnrTargetAddress;
		break;
	/* MCSC */
	case IS_VIDEO_M0P_NUM:
		*taddr = frame->sc0TargetAddress;
		break;
	case IS_VIDEO_M1P_NUM:
		*taddr = frame->sc1TargetAddress;
		break;
	case IS_VIDEO_M2P_NUM:
		*taddr = frame->sc2TargetAddress;
		break;
	case IS_VIDEO_M3P_NUM:
		*taddr = frame->sc3TargetAddress;
		break;
	case IS_VIDEO_M4P_NUM:
		*taddr = frame->sc4TargetAddress;
		break;
	case IS_VIDEO_M5P_NUM:
		*taddr = frame->sc5TargetAddress;
		break;
	/* ORBMCH */
	case IS_VIDEO_ORB0C_NUM:
		*taddr = frame->orbxcTargetAddress;
		if (taddr_k)
			*taddr_k = frame->orbxcKTargetAddress;
		break;
	case IS_VIDEO_ORB0M_NUM:
		/* No DMA out */
		if (taddr_k)
			*taddr_k = frame->orbxmKTargetAddress;
		break;
	case IS_VIDEO_MCH0S_NUM:
		*taddr = frame->mchxsTargetAddress;
		if (taddr_k)
			*taddr_k = frame->mchxsKTargetAddress;
		/* Should not clear buffer to keep mch previous input data */
		return ret;
	default:
		err_hw("Unsupported vid(%d)", vid);
		ret = -EINVAL;
		break;
	}

	/* Clear subdev frame's target address before set */
	if (taddr && *taddr)
		memset(*taddr, 0x0, sizeof(typeof(**taddr)) * IS_MAX_PLANES);
	if (taddr_k && *taddr_k)
		memset(*taddr_k, 0x0, sizeof(typeof(**taddr_k)) * IS_MAX_PLANES);

	return ret;
}

void * is_get_dma_blk(int type)
{
	struct is_lib_support *lib = is_get_lib_support();
	struct lib_mem_block * mblk = NULL;

	switch (type) {
	case ID_DMA_3AAISP:
		mblk = &lib->mb_dma_taaisp;
		break;
	case ID_DMA_MEDRC:
		mblk = &lib->mb_dma_medrc;
		break;
	case ID_DMA_ORBMCH:
		mblk = &lib->mb_dma_orbmch;
		break;
	case ID_DMA_TNR:
		mblk = &lib->mb_dma_tnr;
		break;
	case ID_DMA_CLAHE:
		mblk = &lib->mb_dma_clahe;
		break;
	default:
		err_hw("Invalid DMA type: %d\n", type);
		return NULL;
	}

	return (void *)mblk;
}

void is_hw_fill_target_address(u32 hw_id, struct is_frame *dst, struct is_frame *src,
	bool reset)
{
	/* A previous address should not be cleared for sysmmu debugging. */
	reset = false;

	switch (hw_id) {
	case DEV_HW_PAF0:
	case DEV_HW_PAF1:
	case DEV_HW_PAF2:
		break;
	case DEV_HW_3AA0:
	case DEV_HW_3AA1:
	case DEV_HW_3AA2:
		TADDR_COPY(dst, src, txcTargetAddress, reset);
		TADDR_COPY(dst, src, txpTargetAddress, reset);
		TADDR_COPY(dst, src, mrgTargetAddress, reset);
		TADDR_COPY(dst, src, efdTargetAddress, reset);
		TADDR_COPY(dst, src, txdgrTargetAddress, reset);
		TADDR_COPY(dst, src, txodsTargetAddress, reset);
		TADDR_COPY(dst, src, txldsTargetAddress, reset);
		TADDR_COPY(dst, src, txhfTargetAddress, reset);
		break;
	case DEV_HW_ORB0:
		TADDR_COPY(dst, src, orbxmKTargetAddress, reset);
		TADDR_COPY(dst, src, orbxcTargetAddress, reset);
		TADDR_COPY(dst, src, orbxcKTargetAddress, reset);
		TADDR_COPY(dst, src, mchxsTargetAddress, reset);
		TADDR_COPY(dst, src, mchxsKTargetAddress, reset);
		break;
	case DEV_HW_ISP0:
		TADDR_COPY(dst, src, ixcTargetAddress, reset);
		TADDR_COPY(dst, src, ixpTargetAddress, reset);
		TADDR_COPY(dst, src, ixtTargetAddress, reset);
		TADDR_COPY(dst, src, ixgTargetAddress, reset);
		TADDR_COPY(dst, src, ixvTargetAddress, reset);
		TADDR_COPY(dst, src, ixwTargetAddress, reset);
		TADDR_COPY(dst, src, ixmTargetAddress, reset);
		TADDR_COPY(dst, src, ixmKTargetAddress, reset);
		TADDR_COPY(dst, src, ixdgrTargetAddress, reset);
		TADDR_COPY(dst, src, ixrrgbTargetAddress, reset);
		TADDR_COPY(dst, src, ixnoirTargetAddress, reset);
		TADDR_COPY(dst, src, ixscmapTargetAddress, reset);
		TADDR_COPY(dst, src, ixnrdsTargetAddress, reset);
		TADDR_COPY(dst, src, ixdgaTargetAddress, reset);
		TADDR_COPY(dst, src, ixhfTargetAddress, reset);
		TADDR_COPY(dst, src, ixwrgbTargetAddress, reset);
		TADDR_COPY(dst, src, ixnoirwTargetAddress, reset);
		TADDR_COPY(dst, src, ixbnrTargetAddress, reset);
		TADDR_COPY(dst, src, ixnoiTargetAddress, reset);
		break;
	case DEV_HW_MCSC0:
		TADDR_COPY(dst, src, sc0TargetAddress, reset);
		TADDR_COPY(dst, src, sc1TargetAddress, reset);
		TADDR_COPY(dst, src, sc2TargetAddress, reset);
		TADDR_COPY(dst, src, sc3TargetAddress, reset);
		TADDR_COPY(dst, src, sc4TargetAddress, reset);
		TADDR_COPY(dst, src, sc5TargetAddress, reset);
		break;
	default:
		err("[%d] Invalid hw id(%d)", src->instance, hw_id);
		break;
	}
}

struct is_mem *is_hw_get_iommu_mem(u32 vid)
{
	struct pablo_device_iommu_group *iommu_group;

	switch (vid) {
	case IS_VIDEO_SS0_NUM:
	case IS_VIDEO_SS1_NUM:
	case IS_VIDEO_SS2_NUM:
	case IS_VIDEO_SS3_NUM:
	case IS_VIDEO_SS4_NUM:
	case IS_VIDEO_SS5_NUM:
	case IS_VIDEO_SS6_NUM:
	case IS_VIDEO_SS7_NUM:
	case IS_VIDEO_SS8_NUM:
	case IS_VIDEO_SS0VC0_NUM:
	case IS_VIDEO_SS0VC1_NUM:
	case IS_VIDEO_SS0VC2_NUM:
	case IS_VIDEO_SS0VC3_NUM:
	case IS_VIDEO_SS1VC0_NUM:
	case IS_VIDEO_SS1VC1_NUM:
	case IS_VIDEO_SS1VC2_NUM:
	case IS_VIDEO_SS1VC3_NUM:
	case IS_VIDEO_SS2VC0_NUM:
	case IS_VIDEO_SS2VC1_NUM:
	case IS_VIDEO_SS2VC2_NUM:
	case IS_VIDEO_SS2VC3_NUM:
	case IS_VIDEO_SS3VC0_NUM:
	case IS_VIDEO_SS3VC1_NUM:
	case IS_VIDEO_SS3VC2_NUM:
	case IS_VIDEO_SS3VC3_NUM:
	case IS_VIDEO_SS4VC0_NUM:
	case IS_VIDEO_SS4VC1_NUM:
	case IS_VIDEO_SS4VC2_NUM:
	case IS_VIDEO_SS4VC3_NUM:
	case IS_VIDEO_SS5VC0_NUM:
	case IS_VIDEO_SS5VC1_NUM:
	case IS_VIDEO_SS5VC2_NUM:
	case IS_VIDEO_SS5VC3_NUM:
	case IS_VIDEO_SS6VC0_NUM:
	case IS_VIDEO_SS6VC1_NUM:
	case IS_VIDEO_SS6VC2_NUM:
	case IS_VIDEO_SS6VC3_NUM:
	case IS_VIDEO_PAF0S_NUM:
	case IS_VIDEO_PAF1S_NUM:
	case IS_VIDEO_PAF2S_NUM:
	case IS_VIDEO_PAF3S_NUM:
	case IS_VIDEO_30S_NUM:
	case IS_VIDEO_30C_NUM:
	case IS_VIDEO_30P_NUM:
	case IS_VIDEO_30F_NUM:
	case IS_VIDEO_30G_NUM:
	case IS_VIDEO_30O_NUM:
	case IS_VIDEO_30L_NUM:
	case IS_VIDEO_32O_NUM:
	case IS_VIDEO_32L_NUM:
	case IS_VIDEO_31S_NUM:
	case IS_VIDEO_31C_NUM:
	case IS_VIDEO_31P_NUM:
	case IS_VIDEO_31F_NUM:
	case IS_VIDEO_31G_NUM:
	case IS_VIDEO_31O_NUM:
	case IS_VIDEO_31L_NUM:
	case IS_VIDEO_33O_NUM:
	case IS_VIDEO_33L_NUM:
	case IS_VIDEO_32S_NUM:
	case IS_VIDEO_32C_NUM:
	case IS_VIDEO_32P_NUM:
	case IS_VIDEO_32F_NUM:
	case IS_VIDEO_32G_NUM:
	case IS_VIDEO_33S_NUM:
	case IS_VIDEO_33C_NUM:
	case IS_VIDEO_33P_NUM:
	case IS_VIDEO_33F_NUM:
	case IS_VIDEO_33G_NUM:
	case IS_VIDEO_30D_NUM:
	case IS_VIDEO_31D_NUM:
	case IS_VIDEO_32D_NUM:
	case IS_VIDEO_33D_NUM:
	case IS_VIDEO_30H_NUM:
	case IS_VIDEO_31H_NUM:
	case IS_VIDEO_32H_NUM:
	case IS_VIDEO_33H_NUM:
		iommu_group = pablo_iommu_group_get(0);
		return &iommu_group->mem;
	case IS_VIDEO_I0S_NUM:
	case IS_VIDEO_I0C_NUM:
	case IS_VIDEO_I0P_NUM:
	case IS_VIDEO_I0V_NUM:
	case IS_VIDEO_I0W_NUM:
	case IS_VIDEO_I0T_NUM:
	case IS_VIDEO_I0G_NUM:
	case IS_VIDEO_IMM_NUM:
	case IS_VIDEO_IRG_NUM:
	case IS_VIDEO_ISC_NUM:
	case IS_VIDEO_IDR_NUM:
	case IS_VIDEO_INR_NUM:
	case IS_VIDEO_IND_NUM:
	case IS_VIDEO_IDG_NUM:
	case IS_VIDEO_ISH_NUM:
	case IS_VIDEO_IHF_NUM:
	case IS_VIDEO_INW_NUM:
	case IS_VIDEO_INRW_NUM:
	case IS_VIDEO_IRGW_NUM:
	case IS_VIDEO_INB_NUM:
	case IS_VIDEO_ORB0_NUM:
	case IS_VIDEO_ORB1_NUM:
	case IS_VIDEO_ORB0C_NUM:
	case IS_VIDEO_ORB0M_NUM:
	case IS_VIDEO_MCH0S_NUM:
	case IS_VIDEO_ORB1C_NUM:
	case IS_VIDEO_ORB1M_NUM:
	case IS_VIDEO_MCH1S_NUM:
	case IS_VIDEO_M0P_NUM:
	case IS_VIDEO_M1P_NUM:
	case IS_VIDEO_M2P_NUM:
	case IS_VIDEO_M3P_NUM:
	case IS_VIDEO_M4P_NUM:
	case IS_VIDEO_M5P_NUM:
		iommu_group = pablo_iommu_group_get(1);
		return &iommu_group->mem;
	default:
		err("Invalid vid(%d)", vid);
		return NULL;
	}
}

void is_hw_print_target_dva(struct is_frame *leader_frame, u32 instance)
{
	u32 i;
#if defined(CSTAT_CTX_NUM)
	u32 ctx;
#endif

	for (i = 0; i < IS_MAX_PLANES; i++) {

#if IS_ENABLED(SOC_30C)
		IS_PRINT_TARGET_DVA(txcTargetAddress);
#endif
#if defined(CSTAT_CTX_NUM)
		for (ctx = 0; ctx < CSTAT_CTX_NUM; ctx++)
			IS_PRINT_TARGET_DVA(txpTargetAddress[ctx]);
#else
		IS_PRINT_TARGET_DVA(txpTargetAddress);
#endif

#if IS_ENABLED(SOC_30G)
		IS_PRINT_TARGET_DVA(mrgTargetAddress);
#endif
#if defined(CSTAT_CTX_NUM)
		for (ctx = 0; ctx < CSTAT_CTX_NUM; ctx++)
			IS_PRINT_TARGET_DVA(efdTargetAddress[ctx]);
#else
		IS_PRINT_TARGET_DVA(efdTargetAddress);
#endif
#if IS_ENABLED(LOGICAL_VIDEO_NODE)
#if defined(CSTAT_CTX_NUM)
		for (ctx = 0; ctx < CSTAT_CTX_NUM; ctx++)
			IS_PRINT_TARGET_DVA(txdgrTargetAddress[ctx]);
#else
		IS_PRINT_TARGET_DVA(txdgrTargetAddress);
#endif
#endif
#if defined(ENABLE_ORBDS)
		IS_PRINT_TARGET_DVA(txodsTargetAddress);
#endif
#if defined(ENABLE_LMEDS)
#if defined(CSTAT_CTX_NUM)
		for (ctx = 0; ctx < CSTAT_CTX_NUM; ctx++)
			IS_PRINT_TARGET_DVA(txldsTargetAddress[ctx]);
#else
		IS_PRINT_TARGET_DVA(txldsTargetAddress);
#endif
#endif
#if defined(ENABLE_LMEDS1)
#if defined(CSTAT_CTX_NUM)
		for (ctx = 0; ctx < CSTAT_CTX_NUM; ctx++)
			IS_PRINT_TARGET_DVA(dva_cstat_lmeds1[ctx]);
#else
		IS_PRINT_TARGET_DVA(dva_cstat_lmeds1);
#endif
#endif
#if defined(ENABLE_HF) && IS_ENABLED(SOC_30S)
		IS_PRINT_TARGET_DVA(txhfTargetAddress);
#endif
#if IS_ENABLED(SOC_CSTAT_SVHIST)
#if defined(CSTAT_CTX_NUM)
		for (ctx = 0; ctx < CSTAT_CTX_NUM; ctx++)
			IS_PRINT_TARGET_DVA(dva_cstat_vhist[ctx]);
#else
		IS_PRINT_TARGET_DVA(dva_cstat_vhist);
#endif
#endif
#if IS_ENABLED(SOC_LME0)
		IS_PRINT_TARGET_DVA(lmesTargetAddress);
		IS_PRINT_TARGET_DVA(lmecTargetAddress);
#endif
#if IS_ENABLED(ENABLE_BYRP_HDR)
		IS_PRINT_TARGET_DVA(dva_byrp_hdr);
#endif
		IS_PRINT_TARGET_DVA(ixcTargetAddress);
		IS_PRINT_TARGET_DVA(ixpTargetAddress);
		IS_PRINT_TARGET_DVA(ixtTargetAddress);
		IS_PRINT_TARGET_DVA(ixgTargetAddress);
		IS_PRINT_TARGET_DVA(ixvTargetAddress);
		IS_PRINT_TARGET_DVA(ixwTargetAddress);
		IS_PRINT_TARGET_DVA(mexcTargetAddress);
		IS_PRINT_TARGET_DVA(orbxcKTargetAddress);
#if IS_ENABLED(SOC_ORBMCH)
		IS_PRINT_TARGET_DVA(mchxsTargetAddress);
#endif
#if IS_ENABLED(USE_MCFP_MOTION_INTERFACE)
		IS_PRINT_TARGET_DVA(ixmTargetAddress);
#endif
#if IS_ENABLED(SOC_YPP)
		IS_PRINT_TARGET_DVA(ixdgrTargetAddress);
		IS_PRINT_TARGET_DVA(ixrrgbTargetAddress);
		IS_PRINT_TARGET_DVA(ixnoirTargetAddress);
		IS_PRINT_TARGET_DVA(ixscmapTargetAddress);
		IS_PRINT_TARGET_DVA(ixnrdsTargetAddress);
		IS_PRINT_TARGET_DVA(ixnoiTargetAddress);
		IS_PRINT_TARGET_DVA(ixdgaTargetAddress);
		IS_PRINT_TARGET_DVA(ixsvhistTargetAddress);
		IS_PRINT_TARGET_DVA(ixhfTargetAddress);
		IS_PRINT_TARGET_DVA(ixwrgbTargetAddress);
		IS_PRINT_TARGET_DVA(ixnoirwTargetAddress);
		IS_PRINT_TARGET_DVA(ixbnrTargetAddress);
		IS_PRINT_TARGET_DVA(ypnrdsTargetAddress);
		IS_PRINT_TARGET_DVA(ypnoiTargetAddress);
		IS_PRINT_TARGET_DVA(ypdgaTargetAddress);
		IS_PRINT_TARGET_DVA(ypsvhistTargetAddress);
#endif
		IS_PRINT_TARGET_DVA(sc0TargetAddress);
		IS_PRINT_TARGET_DVA(sc1TargetAddress);
		IS_PRINT_TARGET_DVA(sc2TargetAddress);
		IS_PRINT_TARGET_DVA(sc3TargetAddress);
		IS_PRINT_TARGET_DVA(sc4TargetAddress);
		IS_PRINT_TARGET_DVA(sc5TargetAddress);
		IS_PRINT_TARGET_DVA(clxsTargetAddress);
		IS_PRINT_TARGET_DVA(clxcTargetAddress);
	}
}

int is_hw_config(struct is_hw_ip *hw_ip, struct pablo_crta_buf_info *buf_info)
{
	return 0;
}

void is_hw_update_pcfi(struct is_hardware *hardware, struct is_group *group,
			struct is_frame *frame, struct pablo_crta_buf_info *pcfi_buf)
{
}

void is_hw_update_frame_info(struct is_group *group, struct is_frame *frame)
{
}

void is_hw_check_iteration_state(struct is_frame *frame,
	struct is_dvfs_ctrl *dvfs_ctrl, struct is_group *group)
{
}

size_t is_hw_get_param_dump(char *buf, size_t buf_size, struct is_param_region *param, u32 group_id)
{
	return 0;
}
