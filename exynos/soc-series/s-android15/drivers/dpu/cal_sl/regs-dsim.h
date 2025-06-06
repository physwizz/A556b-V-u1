/* SPDX-License-Identifier: GPL-2.0-only
 *
 * regs-dsim.h
 *
 * Copyright (c) 2020 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Jaehoe Yang <jaehoe.yang@samsung.com>
 * Jiun Yu <jiun.yu@samsung.com>
 *
 * Register definition file for Samsung MIPI-DSIM driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REGS_SYSREG_DISP_H
#define _REGS_SYSREG_DISP_H

#define MAX_DSI_CNT REGS_DSIM_ID_MAX

enum dsim_regs_id {
	REGS_DSIM0_ID = 0,
	REGS_DSIM1_ID,
	REGS_DSIM2_ID,
	REGS_DSIM_ID_MAX
};

enum dsim_regs_type {
	REGS_DSIM_DSI = 0,
	REGS_DSIM_PHY,
	REGS_DSIM_PHY_BIAS,
	REGS_DSIM_SYS,
	REGS_DSIM_TYPE_MAX
};

/* DPU_MIPI_PHY_CON : 0x1008 */
#define DPU_MIPI_PHY_CON			(0x0008)
#define SEL_M1_MODE(_v)				((_v) << 8)
#define M1_MODE_4LANE				SEL_M1_MODE(0)
#define M1_MODE_2P2LANE				SEL_M1_MODE(1)
/* _v : [0,1,2] */
#define SEL_RESET_DPHY_MASK(_v)			(0x1 << (4 + (_v)))
#define M_RESETN_M2_MASK			(0x1 << 2)
#define M_RESETN_M1_MASK			(0x1 << 1)
#define M_RESETN_M0_MASK			(0x1 << 0)

/* UPI_CON : 0x2000 */
#define UPI_CON					(0x1000)
#define UPI_EN(_v)				((_v) << 0)
#define UPI_ENABLE				UPI_EN(1)
#define UPI_DISABLE				UPI_EN(0)

#endif /* _REGS_SYSREG_DISP_H */

/* below 3 values depend on H/W */
#define DSIM_RX_FIFO_MAX_DEPTH			64
#define DSIM_PL_FIFO_THRESHOLD			2048
#define DSIM_FCMD_ALIGN_CONSTRAINT		8

#ifndef _REGS_DSIM_H
#define _REGS_DSIM_H

#define DSIM_VERSION					(0x0000)
#define DSIM_VERSION_GET_MAJOR(_v)			(((_v) >> 24) & 0x000000ff)
#define DSIM_VERSION_GET_MINOR(_v)			(((_v) >> 16) & 0x000000ff)

#define DSIM_SWRST					(0x0004)
#define DSIM_DPHY_RST					(1 << 16)
#define DSIM_SWRST_FUNCRST				(1 << 8)
#define DSIM_SWRST_RESET				(1 << 0)

#define DSIM_LINK_STATUS0				(0x0008)
#define DSIM_LINK_STATUS0_VIDEO_MODE_STATUS_GET(x)	((x >> 24) & 0x1)
#define DSIM_LINK_STATUS0_VT_HSTATE_GET(x)		((x >> 18) & 0x3f)
#define DSIM_LINK_STATUS0_VT_HSTATE_HIDLE		(0x1)
#define DSIM_LINK_STATUS0_VT_HSTATE_HSA			(0x2)
#define DSIM_LINK_STATUS0_VT_HSTATE_HBP			(0x4)
#define DSIM_LINK_STATUS0_VT_HSTATE_HACT		(0x8)
#define DSIM_LINK_STATUS0_VT_HSTATE_HFP			(0x10)
#define DSIM_LINK_STATUS0_VT_HSTATE_HUNDRUN		(0x20)
#define DSIM_LINK_STATUS0_VT_VSTATE_GET(x)		((x >> 13) & 0x1f)
#define DSIM_LINK_STATUS0_VT_VSTATE_VIDLE		(0x1)
#define DSIM_LINK_STATUS0_VT_VSTATE_VSA			(0x2)
#define DSIM_LINK_STATUS0_VT_VSTATE_VBP			(0x4)
#define DSIM_LINK_STATUS0_VT_VSTATE_VACT		(0x8)
#define DSIM_LINK_STATUS0_VT_VSTATE_VFP			(0x10)
#define DSIM_LINK_STATUS0_VM_LINE_CNT_GET(x)		((x >> 0) & 0x1fff)

#define DSIM_LINK_STATUS1				(0x000c)
#define DSIM_LINK_STATUS1_CMD_MODE_STATUS_GET(x)	((x >> 26) & 0x1)
#define DSIM_LINK_STATUS1_CMD_LOCK_STATUS_GET(x)	((x >> 25) & 0x1)
#define DSIM_LINK_STATUS1_CMD_TRANSF_CNT_GET(x)		((x >> 0) & 0x1ffffff)
#define DSIM_STATUS_IDLE				(0)
#define DSIM_STATUS_ACTIVE				(1)
#define DSIM_STATUS_CMDLOCK				(1)

#define DSIM_LINK_STATUS2				(0x10)
#define DSIM_LINK_STATUS2_DATALANE_STATUS_GET(x)	((x >> 0) & 0x7)
#define DSIM_LINK_STATUS2_HSDT_STATUS			(0x7 << 21)
#define DSIM_LINE_STATUS2_HSDT_STATUS_SHIFT		21
#define DSIM_LINK_STATUS2_HSCLK_STATUS			(0x7 << 8)
#define DSIM_LINK_STATUS2_HSCLK_STATUS_SHIFT		8

#define DSIM_LINK_STATUS3				(0x0014)
#define DSIM_LINK_STATUS3_PHY_CFG_STATUS_GET(x)		(((x) >> 16) & 0x7)
#define DSIM_LINK_STATUS3_PHY_CFG_STATUS_IDLE		(0)
#define DSIM_LINK_STATUS3_PHY_CFG_STATUS_ALVCLK		(1)
#define DSIM_LINK_STATUS3_PHY_CFG_STATUS_UPDT		(2)
#define DSIM_LINK_STATUS3_PHY_CFG_STATUS_WAIT_READY	(3)
#define DSIM_LINK_STATUS3_PHY_CFG_STATUS_WCLK		(4)
#define DSIM_LINK_STATUS3_PHY_CFG_STATUS_UPDT_DONE	(5)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_GET(x)	(((x) >> 12) & 0x7)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_IDLE		(0)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_OSCLK		(1)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_PLL_SLEEP	(2)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_PLL_WAKEUP	(3)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_WCLK		(4)
#define DSIM_LINK_STATUS3_MUX_STATUS_GET(x)		(((x) >> 4) & 0x3)
#define DSIM_LINK_STATUS3_MUX_STATUS_ALIVE_CLOCK	(1)
#define DSIM_LINK_STATUS3_MUX_STATUS_WORD_CLOCK		(2)
#define DSIM_LINK_STATUS3_PLL_STABLE			(1 << 0)

#define DSIM_MIPI_STATUS				(0x0018)
#define DSIM_MIPI_STATUS_FRM_PROCESSING			(1 << 29)
#define DSIM_MIPI_STATUS_FRM_PROCESSING_SHIFT		29
#define DSIM_MIPI_STATUS_FRM_DONE			(1 << 28)
#define DSIM_MIPI_STATUS_SHADOW_REG_UP_EN		(1 << 25)
#define DSIM_MIPI_STATUS_SHADOW_REG_UP_DONE		(1 << 24)
#define DSIM_MIPI_STATUS_INSTANT_OFF_REQ		(1 << 21)
#define DSIM_MIPI_STATUS_INSTANT_OFF_ACK		(1 << 20)
#define DSIM_MIPI_STATUS_TE				(1 << 0)

#define DSIM_DPHY_STATUS				(0x001c)
#define DSIM_DPHY_STATUS_TX_READY_HS_DATA		(1 << 15)
#define DSIM_DPHY_STATUS_TX_READY_HS_DATA_SHIFT		15
#define DSIM_DPHY_STATUS_TX_REQUEST_HS_DATA		(1 << 11)
#define DSIM_DPHY_STATUS_TX_REQUEST_HS_DATA_SHIFT	11
#define DSIM_DPHY_STATUS_TX_READY_HSCLK			(1 << 10)
#define DSIM_DPHY_STATUS_TX_READY_HSCLK_SHIFT		10
#define DSIM_DPHY_STATUS_ULPS_CLK			(1 << 9)
#define DSIM_DPHY_STATUS_STOPSTATE_CLK			(1 << 8)
#define DSIM_DPHY_STATUS_STOPSTATE_CLK_SHIFT		8
#define DSIM_DPHY_STATUS_ULPS_DATA_LANE_GET(x)		(((x) >> 4) & 0xf)
#define DSIM_DPHY_STATUS_ULPS_DAT(_x)			(((_x) & 0xf) << 4)
#define DSIM_DPHY_STATUS_STOPSTATE_DAT(_x)		(((_x) & 0xf) << 0)
#define DSIM_DPHY_STATUS_STOPSTATE_DATA			(1 << 0)
#define DSIM_DPHY_STATUS_STOPSTATE_DATA_SHIFT		0

#define DSIM_CLK_CTRL					(0x0020)
#define DSIM_CLK_CTRL_CLOCK_SEL				(1 << 26)
#define DSIM_CLK_CTRL_NONCONT_CLOCK_LANE		(1 << 25)
#define DSIM_CLK_CTRL_CLKLANE_ONOFF			(1 << 24)
#define DSIM_CLK_CTRL_TX_REQUEST_HSCLK			(1 << 20)
#define DSIM_CLK_CTRL_TX_REQUEST_HSCLK_GET(_x)		(((_x) >> 20) & 0x1)
/*
 * This register is used to receive the osc clock from
 * the idle state when using the DSIM.
 *  0 = when power is down.
 *  1 = when using DSIM.
 */
#define DSIM_CLK_CTRL_OSC_GATE_CONDITION		(1 << 18)
#define DSIM_CLK_CTRL_WORDCLK_EN			(1 << 17)
#define DSIM_CLK_CTRL_ESCCLK_EN				(1 << 16)
#define DSIM_CLK_CTRL_LANE_ESCCLK_EN(_x)		((_x) << 8)
#define DSIM_CLK_CTRL_LANE_ESCCLK_EN_MASK		(0x1f << 8)
#define DSIM_CLK_CTRL_ESC_PRESCALER(_x)			((_x) << 0)
#define DSIM_CLK_CTRL_ESC_PRESCALER_MASK		(0xff << 0)

#define DSIM_DESKEW_CTRL				(0x0024)
#define DSIM_DESKEW_CTRL_HW_EN				(1 << 15)
#define DSIM_DESKEW_CTRL_HW_POSITION(_x)		((_x) << 14)
#define DSIM_DESKEW_CTRL_HW_POSITION_MASK		(1 << 14)
#define DSIM_DESKEW_CTRL_HW_INTERVAL(_x)		((_x) << 2)
#define DSIM_DESKEW_CTRL_HW_INTERVAL_MASK		(0xfff << 2)
#define DSIM_DESKEW_CTRL_HW_INIT			(1 << 1)
#define DSIM_DESKEW_CTRL_SW_SEND			(1 << 0)

/* Time out register */
#define DSIM_TIMEOUT					(0x0028)
#define DSIM_TIMEOUT_BTA_TOUT(_x)			((_x) << 16)
#define DSIM_TIMEOUT_BTA_TOUT_MASK			(0xffff << 16)
#define DSIM_TIMEOUT_LPRX_TOUT(_x)			((_x) << 0)
#define DSIM_TIMEOUT_LPRX_TOUT_MASK			(0xffff << 0)

/* Escape mode register */
#define DSIM_ESCMODE					(0x002c)
#define DSIM_ESCMODE_STOP_STATE_CNT(_x)			((_x) << 21)
#define DSIM_ESCMODE_STOP_STATE_CNT_MASK		(0x7ff << 21)
#define DSIM_ESCMODE_FORCE_STOP_STATE			(1 << 20)
#define DSIM_ESCMODE_FORCE_BTA				(1 << 16)
#define DSIM_ESCMODE_CMD_LPDT				(1 << 7)
#define DSIM_ESCMODE_TRIGGER_RST			(1 << 4)
#define DSIM_ESCMODE_TX_ULPS_DATA			(1 << 3)
#define DSIM_ESCMODE_TX_ULPS_DATA_EXIT			(1 << 2)
#define DSIM_ESCMODE_TX_ULPS_CLK			(1 << 1)
#define DSIM_ESCMODE_TX_ULPS_CLK_EXIT			(1 << 0)

#define DSIM_NUM_OF_TRANSFER				(0x0030)
#define DSIM_NUM_OF_TRANSFER_PER_FRAME(_x)		((_x) << 0)
#define DSIM_NUM_OF_TRANSFER_PER_FRAME_MASK		(0xfffff << 0)
#define DSIM_NUM_OF_TRANSFER_PER_FRAME_GET(x)		(((x) >> 0) & 0xfffff)

#define DSIM_UNDERRUN_CTRL				(0x0034)
#define DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF(_x)	((_x) << 0)	/* SHD */
#define DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF_MASK	(0xffff << 0)

#define DSIM_THRESHOLD					(0x0038)
#define DSIM_THRESHOLD_LEVEL(_x)			((_x) << 0)	/* SHD */
#define DSIM_THRESHOLD_LEVEL_MASK			(0xffff << 0)

/* Display image resolution register */
#define DSIM_RESOL					(0x003c)
#define DSIM_RESOL_VRESOL(x)				(((x) & 0x1fff) << 16)	/* SHD */
#define DSIM_RESOL_VRESOL_MASK				(0x1fff << 16)
#define DSIM_RESOL_HRESOL(x)				(((x) & 0x1fff) << 0)	/* SHD */
#define DSIM_RESOL_HRESOL_MASK				(0x1fff << 0)
#define DSIM_RESOL_LINEVAL_GET(_v)			(((_v) >> 16) & 0x1fff)
#define DSIM_RESOL_HOZVAL_GET(_v)			(((_v) >> 0) & 0x1fff)

/* Main display Hporch register */
#define DSIM_HPORCH					(0x0044)
#define DSIM_HPORCH_HFP(_x)				((_x) << 16)	/* SHD */
#define DSIM_HPORCH_HFP_MASK				(0xffff << 16)
#define DSIM_HPORCH_HBP(_x)				((_x) << 0)	/* SHD */
#define DSIM_HPORCH_HBP_MASK				(0xffff << 0)

/* Main display sync area register */
#define DSIM_SYNC					(0x0048)
#define DSIM_SYNC_VSA(_x)				((_x) << 16)	/* SHD */
#define DSIM_SYNC_VSA_MASK				(0xff << 16)
#define DSIM_SYNC_HSA(_x)				((_x) << 0)	/* SHD */
#define DSIM_SYNC_HSA_MASK				(0xffff << 0)

/* Configuration register */
#define DSIM_CONFIG					(0x004c)
#define DSIM_CONFIG_PLL_CLOCK_GATING			(1 << 30)
#define DSIM_CONFIG_PLL_SLEEP				(1 << 29)
#define DSIM_CONFIG_PHY_SELECTION			(1 << 28)
#define DSIM_CONFIG_SYNC_INFORM				(1 << 27)
#define DSIM_CONFIG_BURST_MODE				(1 << 26)
#define DSIM_CONFIG_LP_FORCE_EN				(1 << 24)
#define DSIM_CONFIG_HSE_DISABLE				(1 << 23)
#define DSIM_CONFIG_HFP_DISABLE				(1 << 22)
#define DSIM_CONFIG_HBP_DISABLE				(1 << 21)
#define DSIM_CONFIG_HSA_DISABLE				(1 << 20)
#define DSIM_CONFIG_CPRS_EN				(1 << 19)	/* SHD */
#define DSIM_CONFIG_VIDEO_MODE				(1 << 18)	/* SHD */
#define DSIM_CONFIG_DISPLAY_MODE_GET(_v)		(((_v) >> 18) & 0x1)
#define DSIM_CONFIG_MULTI_PARTIAL			(1 << 17)
#define DSIM_CONFIG_VC_ID(_x)				((_x) << 15)
#define DSIM_CONFIG_VC_ID_MASK				(0x3 << 15)
#define DSIM_CONFIG_PIXEL_FORMAT(_x)			((_x) << 9)
#define DSIM_CONFIG_PIXEL_FORMAT_MASK			(0x3f << 9)
#define DSIM_CONFIG_PER_FRAME_READ_EN(_x)		((_x) << 8)
#define DSIM_CONFIG_PER_FRAME_READ_EN_MASK		(1 << 8)
#define DSIM_CONFIG_EOTP_EN(_x)				((_x) << 7)
#define DSIM_CONFIG_EOTP_EN_MASK			(1 << 7)
#define DSIM_CONFIG_NUM_OF_DATA_LANE(_x)		((_x) << 5)
#define DSIM_CONFIG_NUM_OF_DATA_LANE_MASK		(0x3 << 5)
#define DSIM_CONFIG_LANES_EN(_x)			(((_x) & 0x1f) << 0)
#define DSIM_CONFIG_CLK_LANES_EN			(1 << 0)

/* Interrupt source register */
#define DSIM_INTSRC					(0x0050)
#define DSIM_INTSRC_PLL_STABLE				(1 << 31)
#define DSIM_INTSRC_SW_RST_RELEASE			(1 << 30)
#define DSIM_INTSRC_SFR_PL_FIFO_EMPTY			(1 << 29)
#define DSIM_INTSRC_SFR_PH_FIFO_EMPTY			(1 << 28)
#define DSIM_INTSRC_SFR_PH_FIFO_OVERFLOW		(1 << 27)
#define DSIM_INTSRC_SW_DESKEW_DONE			(1 << 26)
#define DSIM_INTSRC_BUS_TURN_OVER			(1 << 25)
#define DSIM_INTSRC_FRAME_DONE				(1 << 24)
#define DSIM_INTSRC_INVALID_SFR_VALUE			(1 << 23)
#define DSIM_INTSRC_ABNORMAL_CMD_ST			(1 << 22)
#define DSIM_INTSRC_LPRX_TOUT				(1 << 21)
#define DSIM_INTSRC_BTA_TOUT				(1 << 20)
#define DSIM_INTSRC_UNDER_RUN				(1 << 19)
#define DSIM_INTSRC_RX_DATA_DONE			(1 << 18)
#define DSIM_INTSRC_RX_TE				(1 << 17)
#define DSIM_INTSRC_RX_ACK				(1 << 16)
#define DSIM_INTSRC_ERR_RX_ECC				(1 << 15)
#define DSIM_INTSRC_RX_CRC				(1 << 14)
#define DSIM_INTSRC_VT_STATUS				(1 << 13)
/*
 * The SYNC_CMD_PL_FIFO_EMPTY interrupt occurs
 * regardless of actual transmission,
 * as long as the SRAM is empty.
 * payload date: PAYLOAD_SYNC -> SRAM -> FIFO
 */
#define DSIM_INTSRC_SYNC_CMD_PL_FIFO_EMPTY		(1 << 12)
/*
 * The SYNC_CMD_PH_FIFO_EMPTY interrupt occurs
 * only when the actual transmission is completed.
 */
#define DSIM_INTSRC_SYNC_CMD_PH_FIFO_EMPTY		(1 << 11)
#define DSIM_INTSRC_ERR_ESC0				(1 << 10)
#define DSIM_INTSRC_ERR_SYNC0				(1 << 6)
#define DSIM_INTSRC_ERR_CONTROL0			(1 << 2)
#define DSIM_INTSRC_ERR_CONTENT_LP0			(1 << 1)
#define DSIM_INTSRC_ERR_CONTENT_LP1			(1 << 0)


/* Interrupt mask register */
#define DSIM_INTMSK					(0x0054)
#define DSIM_INTMSK_PLL_STABLE				(1 << 31)
#define DSIM_INTMSK_SW_RST_RELEASE			(1 << 30)
#define DSIM_INTMSK_SFR_PL_FIFO_EMPTY			(1 << 29)
#define DSIM_INTMSK_SFR_PH_FIFO_EMPTY			(1 << 28)
#define DSIM_INTMSK_SFR_PH_FIFO_OVERFLOW		(1 << 27)
#define DSIM_INTMSK_SW_DESKEW_DONE			(1 << 26)
#define DSIM_INTMSK_BUS_TURN_OVER			(1 << 25)
#define DSIM_INTMSK_FRAME_DONE				(1 << 24)
#define DSIM_INTMSK_INVALID_SFR_VALUE			(1 << 23)
#define DSIM_INTMSK_ABNRMAL_CMD_ST			(1 << 22)
#define DSIM_INTMSK_LPRX_TOUT				(1 << 21)
#define DSIM_INTMSK_BTA_TOUT				(1 << 20)
#define DSIM_INTMSK_UNDER_RUN				(1 << 19)
#define DSIM_INTMSK_RX_DATA_DONE			(1 << 18)
#define DSIM_INTMSK_RX_TE				(1 << 17)
#define DSIM_INTMSK_RX_ACK				(1 << 16)
#define DSIM_INTMSK_ERR_RX_ECC				(1 << 15)
#define DSIM_INTMSK_RX_CRC				(1 << 14)
#define DSIM_INTMSK_VT_STATUS				(1 << 13)
#define DSIM_INTMSK_SYNC_CMD_PL_FIFO_EMPTY		(1 << 12)
#define DSIM_INTMSK_SYNC_CMD_PH_FIFO_EMPTY		(1 << 11)
#define DSIM_INTMSK_ERR_ESC0				(1 << 10)
#define DSIM_INTMSK_ERR_SYNC0				(1 << 6)
#define DSIM_INTMSK_ERR_CONTROL0			(1 << 2)
#define DSIM_INTMSK_ERR_CONTENT_LP0			(1 << 1)
#define DSIM_INTMSK_ERR_CONTENT_LP1			(1 << 0)

/* Packet Header FIFO register */
#define DSIM_PKTHDR					(0x0058)
#define DSIM_PKTHDR_BTA_TYPE(_x)			((_x) << 24)
#define DSIM_PKTHDR_DATA1(_x)				((_x) << 16)
#define DSIM_PKTHDR_DATA0(_x)				((_x) << 8)
#define DSIM_PKTHDR_ID(_x)				((_x) << 0)
#define DSIM_PKTHDR_DATA				(0x1ffffff << 0)

/* Payload FIFO register */
#define DSIM_PAYLOAD					(0x005c)

/* Read FIFO register */
#define DSIM_RXFIFO					(0x0060)

/* SFR control Register for Stanby & Shadow*/
#define DSIM_SFR_CTRL					(0x0064)
#define DSIM_SFR_CTRL_INSTANT_SHADOW_UPDATE_EN		(1 << 2)
#define DSIM_SFR_CTRL_SHADOW_REG_READ_EN		(1 << 1)
#define DSIM_SFR_CTRL_SHADOW_EN				(1 << 0)

/* FIFO status and control register */
#define DSIM_FIFOCTRL					(0x0068)
#define DSIM_FIFOCTRL_NUMBER_OF_SYNC_PH_SFR(_x)		(((_x) & 0x3f) << 26)
#define DSIM_FIFOCTRL_NUMBER_OF_SYNC_PH_SFR_GET(x)	(((x) >> 26) & 0x3f)
#define DSIM_FIFOCTRL_FULL_SYNC_PH_SFR			(1 << 25)
#define DSIM_FIFOCTRL_EMPTY_SYNC_PH_SFR			(1 << 24)
#define DSIM_FIFOCTRL_FULL_SYNC_PL_SFR			(1 << 23)
#define DSIM_FIFOCTRL_EMPTY_SYNC_PL_SFR			(1 << 22)
#define DSIM_FIFOCTRL_NUMBER_OF_PH_SFR(_x)		(((_x) & 0x3f) << 16)
#define DSIM_FIFOCTRL_NUMBER_OF_PH_SFR_GET(x)		(((x) >> 16) & 0x3f)
#define DSIM_FIFOCTRL_EMPTY_RX				(1 << 12)
#define DSIM_FIFOCTRL_FULL_PH_SFR			(1 << 11)
#define DSIM_FIFOCTRL_EMPTY_PH_SFR			(1 << 10)
#define DSIM_FIFOCTRL_FULL_PL_SFR			(1 << 9)
#define DSIM_FIFOCTRL_EMPTY_PL_SFR			(1 << 8)

#define DSIM_LP_SCATTER					(0x006c)
#define DSIM_LP_SCATTER_PATTERN(_x)			((_x) << 16)
#define DSIM_LP_SCATTER_PATTERN_MASK			(0xffff << 16)
#define DSIM_LP_SCATTER_EN				(1 << 0)

#define DSIM_S3D_CTRL					(0x0070)
#define DSIM_S3D_CTRL_3D_PRESENT			(1 << 11)
#define DSIM_S3D_CTRL_3D_ORDER				(1 << 5)
#define DSIM_S3D_CTRL_3D_VSYNC				(1 << 4)
#define DSIM_S3D_CTRL_3D_FORMAT(_x)			(((_x) & 0x3) << 2)
#define DSIM_S3D_CTRL_3D_FORMAT_GET(x)			(((x) >> 2) & 0x3)
#define DSIM_S3D_CTRL_3D_MODE(_x)			(((_x) & 0x3) << 0)
#define DSIM_S3D_CTRL_3D_MODE_GET(x)			(((x) >> 0) & 0x3)

/* Multi slice setting register*/
#define DSIM_CPRS_CTRL					(0x0074)
#define DSIM_CPRS_CTRL_MULI_SLICE_PACKET(_x)		((_x) << 3)	/* SHD */
#define DSIM_CPRS_CTRL_MULI_SLICE_PACKET_MASK		(1 << 3)
#define DSIM_CPRS_CTRL_NUM_OF_SLICE(_x)			((_x) << 0)	/* SHD */
#define DSIM_CPRS_CTRL_NUM_OF_SLICE_MASK		(0x7 << 0)
#define DSIM_CPRS_CTRL_NUM_OF_SLICE_GET(x)		(((x) >> 0) & 0x7)

/*Slice01 size register*/
#define DSIM_SLICE01					(0x0078)
#define DSIM_SLICE01_SIZE_OF_SLICE1(_x)			((_x) << 16)	/* SHD */
#define DSIM_SLICE01_SIZE_OF_SLICE1_MASK		(0x1fff << 16)
#define DSIM_SLICE01_SIZE_OF_SLICE1_GET(x)		(((x) >> 16) & 0x1fff)
#define DSIM_SLICE01_SIZE_OF_SLICE0(_x)			((_x) << 0)	/* SHD */
#define DSIM_SLICE01_SIZE_OF_SLICE0_MASK		(0x1fff << 0)
#define DSIM_SLICE01_SIZE_OF_SLICE0_GET(x)		(((x) >> 0) & 0x1fff)

/*Slice23 size register*/
#define DSIM_SLICE23					(0x007c)
#define DSIM_SLICE23_SIZE_OF_SLICE3(_x)			((_x) << 16)	/* SHD */
#define DSIM_SLICE23_SIZE_OF_SLICE3_MASK		(0x1fff << 16)
#define DSIM_SLICE23_SIZE_OF_SLICE3_GET(x)		(((x) >> 16) & 0x1fff)
#define DSIM_SLICE23_SIZE_OF_SLICE2(_x)			((_x) << 0)	/* SHD */
#define DSIM_SLICE23_SIZE_OF_SLICE2_MASK		(0x1fff << 0)
#define DSIM_SLICE23_SIZE_OF_SLICE2_GET(x)		(((x) >> 0) & 0x1fff)

/* Command configuration register */
#define DSIM_CMD_CONFIG					(0x0080)
#define DSIM_CMD_CONFIG_PKT_GO_RDY			(1 << 17)
#define DSIM_CMD_CONFIG_PKT_GO_RDY_GET(_x)		(((_x) >> 17) & 0x1)
#define DSIM_CMD_CONFIG_PKT_GO_EN			(1 << 16)
#define DSIM_CMD_CONFIG_MULTI_CMD_PKT_EN		(1 << 8)
#define DSIM_CMD_CONFIG_MULTI_PKT_CNT(_x)		((_x) << 0)
#define DSIM_CMD_CONFIG_MULTI_PKT_CNT_MASK		(0x3f << 0)

/* TE based command register*/
#define DSIM_CMD_TE_CTRL0				(0x0084)
#define DSIM_CMD_TE_CTRL0_TIME_TE_FRM_MASK(_x)		((_x) << 16)
#define DSIM_CMD_TE_CTRL0_TIME_TE_FRM_MASK_MASK		(0xffff << 16)
#define DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP(_x)		((_x) << 0)
#define DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP_MASK		(0xffff << 0)

/* TE based command register*/
#define DSIM_CMD_TE_CTRL1				(0x0088)
#define DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON(_x)	((_x) << 16)
#define DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON_MASK	(0xffff << 16)
#define DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON_GET(_x)	(((_x) >> 16) & 0xffff)
#define DSIM_CMD_TE_CTRL1_TIME_TE_TOUT(_x)		((_x) << 0)
#define DSIM_CMD_TE_CTRL1_TIME_TE_TOUT_MASK		(0xffff << 0)

/*Command Mode Status register*/
#define DSIM_CMD_STATUS					(0x008c)
#define	DSIM_CMD_STATUS_ABNORMAL_CAUSE_ST_GET(x)	(((x) >> 0) & 0xff)

#define DSIM_VIDEO_TIMER				(0x0090)
#define DSIM_VIDEO_TIMER_COMPENSATE(_x)			((_x) << 8)	/* SHD */
#define DSIM_VIDEO_TIMER_COMPENSATE_MASK		(0xffffff << 8)
#define DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL(_x)		((_x) << 1)
#define DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL_MASK		(0x3 << 1)
#define DSIM_VIDEO_TIMER_SYNC_MODE			(1 << 0)

/*BIST generation register*/
#define	DSIM_BIST_CTRL0					(0x0094)
#define	DSIM_BIST_CTRL0_BIST_TE_INTERVAL(_x)		((_x) << 8)
#define	DSIM_BIST_CTRL0_BIST_TE_INTERVAL_MASK		(0xffffff << 8)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MOVE_EN		(1 << 4)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MODE(_x)		((_x) << 1)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MODE_MASK		(0x7 << 1)
#define	DSIM_BIST_CTRL0_BIST_EN				(1 << 0)

/*BIST generation register*/
#define	DSIM_BIST_CTRL1					(0x0098)
#define	DSIM_BIST_CTRL1_BIST_PTRN_PRBS7_SEED(_x)	((_x) << 24)
#define	DSIM_BIST_CTRL1_BIST_PTRN_PRBS7_SEED_MASK	(0x7f << 24)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_R(_x)		((_x) << 16)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_R_MASK		(0XFF << 16)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_G(_x)		((_x) << 8)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_G_MASK		(0xFF << 8)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_B(_x)		((_x) << 0)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_B_MASK		(0xFF << 0)

/*DSIM to CSI loopback register*/
#define	DSIM_CSIS_LB					(0x009C)
#define DSIM_CSIS_LB_DBG_SW_P_DATA			(1 << 23)
#define DSIM_CSIS_LB_DBG_SW_P_DATA_MASK			(1 << 23)
#define DSIM_CSIS_LB_DBG_SW_P_ADDR			(0x5 << 16)
#define DSIM_CSIS_LB_DBG_SW_P_ADDR_MASK			(0x7f << 16)
#define DSIM_CSIS_LB_1BYTEPPI_MODE			(1 << 9)
#define	DSIM_CSIS_LB_CSIS_LB_EN				(1 << 8)
#define DSIM_CSIS_LB_CSIS_PH(_x)			((_x) << 0)
#define DSIM_CSIS_LB_CSIS_PH_MASK			(0xff << 0)

/* IF CRC registers */
#define DSIM_IF_CRC_CTRL0				(0x00dc)
#define DSIM_IF_CRC_FAIL				(1 << 16)
#define DSIM_IF_CRC_PASS				(1 << 12)
#define DSIM_IF_CRC_VALID				(1 << 8)
#define DSIM_IF_CRC_CMP_MODE				(1 << 4)
#define DSIM_IF_CRC_CLEAR				(1 << 1)
#define DSIM_IF_CRC_EN					(1 << 0)

#define DSIM_IF_CRC_CTRL1				(0x00e0)
#define DSIM_IF_CRC_REF_R(_x)				((_x) << 16)
#define	DSIM_IF_CRC_RESULT_R_MASK			(0xffff << 0)
#define DSIM_IF_CRC_RESULT_R_GET(x)			(((x) >> 0) & 0xffff)

#define DSIM_IF_CRC_CTRL2				(0x00e4)
#define DSIM_IF_CRC_REF_G(_x)				((_x) << 16)
#define	DSIM_IF_CRC_RESULT_G_MASK			(0xffff << 0)
#define DSIM_IF_CRC_RESULT_G_GET(x)			(((x) >> 0) & 0xffff)

#define DSIM_IF_CRC_CTRL3				(0x00e8)
#define DSIM_IF_CRC_REF_B(_x)				((_x) << 16)
#define	DSIM_IF_CRC_RESULT_B_MASK			(0xffff << 0)
#define DSIM_IF_CRC_RESULT_B_GET(x)			(((x) >> 0) & 0xffff)

/* SA CRC registers */
#define DSIM_SA_CRC_CTRL0				(0x00ec)
#define DSIM_SA_CRC_FAIL				(1 << 16)
#define DSIM_SA_CRC_PASS				(1 << 12)
#define DSIM_SA_CRC_VALID				(1 << 8)
#define DSIM_SA_CRC_CMP_MODE				(1 << 4)
#define DSIM_SA_CRC_CLEAR				(1 << 1)
#define DSIM_SA_CRC_EN					(1 << 0)

#define DSIM_SA_CRC_CTRL1				(0x00f0)
#define DSIM_SA_CRC_REF_LN0(_x)				((_x) << 16)
#define	DSIM_SA_CRC_RESULT_LN0_MASK			(0xffff << 0)
#define DSIM_SA_CRC_RESULT_LN0_GET(x)			(((x) >> 0) & 0xffff)

#define DSIM_SA_CRC_CTRL2				(0x00f4)
#define DSIM_SA_CRC_REF_LN1(_x)				((_x) << 16)
#define	DSIM_SA_CRC_RESULT_LN1_MASK			(0xffff << 0)
#define DSIM_SA_CRC_RESULT_LN1_GET(x)			(((x) >> 0) & 0xffff)

#define DSIM_SA_CRC_CTRL3				(0x00f8)
#define DSIM_SA_CRC_REF_LN2(_x)				((_x) << 16)
#define	DSIM_SA_CRC_RESULT_LN2_MASK			(0xffff << 0)
#define DSIM_SA_CRC_RESULT_LN2_GET(x)			(((x) >> 0) & 0xffff)

#define DSIM_SA_CRC_CTRL4				(0x00fc)
#define DSIM_SA_CRC_REF_LN3(_x)				((_x) << 16)
#define	DSIM_SA_CRC_RESULT_LN3_MASK			(0xffff << 0)
#define DSIM_SA_CRC_RESULT_LN3_GET(x)			(((x) >> 0) & 0xffff)

#define DSIM_CRC_EN(_x)					((_x) << 0)
#define DSIM_CRC_EN_MASK				(1 << 0)


/* Main display Vporch register */
#define DSIM_VPORCH					(0x0104)
#define DSIM_VPORCH_VFP_TOTAL(_x)			((_x) << 16)	/* SHD */
#define DSIM_VPORCH_VFP_TOTAL_MASK			(0xffff << 16)
#define DSIM_VPORCH_VBP(_x)				((_x) << 0)	/* SHD */
#define DSIM_VPORCH_VBP_MASK				(0xffff << 0)

#define DSIM_VFP_DETAIL					(0x108)
#define DSIM_VPORCH_VFP_CMD_ALLOW(_x)			((_x) << 16)	/* SHD */
#define DSIM_VPORCH_VFP_CMD_ALLOW_MASK			(0xffff << 16)
#define DSIM_VPORCH_STABLE_VFP(_x)			((_x) << 0)	/* SHD */
#define DSIM_VPORCH_STABLE_VFP_MASK			(0xffff << 0)

#define DSIM_OPTION_SUITE				(0x010C)
#define DSIM_OPTION_SUITE_OPT_ADAPTIVE_SYNC_MASK	(0x1 << 27)
#define DSIM_OPTION_SUITE_OPT_VM_SCALER_MASK		(0x1 << 26)
#define DSIM_OPTION_SUITE_OPT_FX_DATA_TRANSF_MASK	(0x1 << 25)
#define DSIM_OPTION_SUITE_OPT_WD_SLOW_THAN_ESC_MASK	(0x1 << 24)
#define DSIM_OPTION_SUITE_OPT_CLKLANE_INITN_MASK	(0x1 << 23)
#define DSIM_OPTION_SUITE_OPT_SHADOW_SYNC_CMD_ALLOW_MASK	(0x1 << 22)
#define DSIM_OPTION_SUITE_OPT_PLL_SLEEP_SELF_CTRL_MASK	(0x1 << 21)
#define DSIM_OPTION_SUITE_OPT_PLL_SLEEP_SELF_CTRL_GET(x)	(((x) >> 21) & 0x1)
#define DSIM_OPTION_SUITE_CPHY_LB_TEST_EN		(0x1 << 20)
#define DSIM_OPTION_SUITE_LP_LENGTH(_x)			((_x) << 12)
#define DSIM_OPTION_SUITE_LP_LENGTH_MASK		(0xff << 12)
#define DSIM_OPTION_SUITE_FORCE_LP_IN_LAST_LINE		(0x1 << 11)
#define DSIM_OPTION_SUITE_OPT_TE_ON_CMD_ALLOW_MASK	(0x1 << 10)
#define DSIM_OPTION_SUITE_OPT_EXT_VT_SYNC_MASK		(0x1 << 9)
#define DSIM_OPTION_SUITE_SYNC_MODE_EN_MASK		(0x1 << 8)
#define DSIM_OPTION_SUITE_OPT_VT_COND_MASK		(0x1 << 7)
#define DSIM_OPTION_SUITE_OPT_FD_COND_MASK		(0x1 << 6)
#define DSIM_OPTION_SUITE_OPT_KEEP_VFP_MASK		(0x1 << 5)
#define DSIM_OPTION_SUITE_OPT_ALIVE_MODE_MASK		(0x1 << 4)
#define DSIM_OPTION_SUITE_OPT_CM_EXT_CMD_ALLOW_MASK	(0x1 << 3)
#define DSIM_OPTION_SUITE_OPT_USER_EXPIRE_VFP_MASK	(0x1 << 2)
#define DSIM_OPTION_SUITE_EMIRROR_EN_MASK		(0x1 << 1)
#define DSIM_OPTION_SUITE_CFG_UPDT_EN_MASK		(0x1 << 0)

#define DSIM_VT_HTIMING0				(0x0110)	/* SHD */
#define DSIM_VT_HTIMING0_HSA_PERIOD(_x)			((_x) << 16)
#define DSIM_VT_HTIMING0_HACT_PERIOD(_x)		((_x) << 0)

#define DSIM_VT_HTIMING1				(0X0114)	/* SHD */
#define DSIM_VT_HTIMING1_HFP_PERIOD(_x)			((_x) << 16)
#define DSIM_VT_HTIMING1_HBP_PERIOD(_x)			((_x) << 0)

#define DSIM_SYNCPKTHDR					(0X0118)
#define DSIM_SYNCPKTHDR_SYNC_PKT_HEADER(_x)		((_x) << 0)
#define DSIM_SYNCPKTHDR_SYNC_PKT_HEADER_MASK		(0xffffff << 0)

#define DSIM_FIFO_STATUS				(0x011C)
/* initial # of empty data slots remained in cmd packet header FIFO: 0x20 */
#define DSIM_FIFO_CMD_PH_FIFO_REMAIN(_x)		((_x) << 16)
#define DSIM_FIFO_CMD_PH_FIFO_REMAIN_MASK		(0x3F << 16)
/*
 * initial # of empty data slots remained in cmd payload FIFO
 * [unit: MEM_WIDTH-bit, 8bytes]
 * - until v2.93: 0x100
 * - from v2.94: 0x80 (instead, + 0x80 in sync cmd payload FIFO)
 */
#define DSIM_FIFO_CMD_PL_FIFO_REMAIN(_x)		((_x) << 0)
#define DSIM_FIFO_CMD_PL_FIFO_REMAIN_MASK		(0xFFFF << 0)


#define DSIM_FREQ_HOPP					(0x0120)	/* SHD */
#define DSIM_FREQ_HOPP_FH_LINE_CNT(_x)			((_x) << 16)
#define DSIM_FREQ_HOPP_FH_LINE_CNT_MASK			(0xffff << 16)
#define DSIM_FREQ_HOPP_FH_PRESCALER(_x)			((_x) << 4)
#define DSIM_FREQ_HOPP_FH_PRESCALER_MASK		(0xff << 4)
#define DSIM_FREQ_HOPP_ALV_SW_EN_MASK			(0x1 << 2)
#define DSIM_FREQ_HOPP_FH_EN_MASK			(0x1 << 0)

#define DSIM_SHADOW_CONFIG				(0x0124)
#define DSIM_SHADOW_CONFIG_HACT_COMPENSATE(_x)		((_x) << 16)	/* SHD */
#define DSIM_SHADOW_CONFIG_AFTER_HOPP_COMPENSATE_EN_MASK	(0x1 << 2)	/* SHD */
#define DSIM_SHADOW_CONFIG_SHADOW_VSS_UPDT_MASK		(0x1 << 0)

#define DSIM_FH_COMPENSATE				(0x0128)
#define DSIM_FH_COMPENSATE_FH_DELAY_CNT(_x)		((_x) << 16)	/* SHD */
#define DSIM_FH_COMPENSATE_FH_DELAY_CNT_MASK		(0xffff << 16)
#define DSIM_FH_COMPENSATE_LAST_LINE_COMPENSATE(_x)	((_x) << 8)
#define DSIM_FH_COMPENSATE_LAST_LINE_COMPENSATE_MASK	(0xff << 8)
#define DSIM_FH_COMPENSATE_LAST_LINE_COMPENSATE_EN_MASK	(0x1 << 4)
#define DSIM_FH_COMPENSATE_FH_START_POINT(_x)		((_x) << 0)
#define DSIM_FH_COMPENSATE_FH_START_POINT_MASK		(0xf << 0)

#define DSIM_VMC_MISMATCH0				(0x0138)

#define DSIM_VMC_MISMATCH1				(0x013C)

#define DSIM_ESYNC_CTRL					(0x0188)

/* following 3 registers added from dsim link version 2.94 */
#define DSIM_SYNC_FIFO_STATUS				(0x0190)
/*
 * initial # of empty data slots remained in sync cmd packet header FIFO
 * - until v2.93: not exist
 * - from v2.94: 0x20
 */
#define DSIM_SYNC_FIFO_CMD_PH_FIFO_REMAIN(_x)		((_x) << 16)
#define DSIM_SYNC_FIFO_CMD_PH_FIFO_REMAIN_MASK		(0x3f << 16)
#define DSIM_SYNC_FIFO_CMD_PH_FIFO_REMAIN_GET(_x)	(((_x) >> 16) & 0x3f)
/*
 * initial # of empty data slots remained in sync cmd payload FIFO
 * [unit: MEM_WIDTH-bit, 8bytes]
 * - until v2.93: not exist
 * - from v2.94: 0x80
 */
#define DSIM_SYNC_FIFO_CMD_PL_FIFO_REMAIN(_x)		((_x) << 0)
#define DSIM_SYNC_FIFO_CMD_PL_FIFO_REMAIN_MASK		(0xffff << 0)
#define DSIM_SYNC_FIFO_CMD_PL_FIFO_REMAIN_GET(_x)	(((_x) >> 0) & 0xffff)

#define DSIM_PKTHDR_SYNC				(0x0258)
#define DSIM_PKTHDR_SYNC_BTA_TYPE(_x)			((_x) << 24)
#define DSIM_PKTHDR_SYNC_DATA1(_x)			((_x) << 16)
#define DSIM_PKTHDR_SYNC_DATA0(_x)			((_x) << 8)
#define DSIM_PKTHDR_SYNC_ID(_x)				((_x) << 0)
#define DSIM_PKTHDR_SYNC_DATA				(0x1ffffff << 0)

#define DSIM_PAYLOAD_SYNC				(0x025C)

#define DSIM_TRANS_MODE					(0x0400)
#define DSIM_CMD_CPU_ACCESS_MODE			(0)
#define DSIM_CMD_DMA_ACCESS_MODE			(1)
#define DSIM_CMD_ACCESS_MODE(_v)			((_v) << 0)
#define DSIM_CMD_ACCESS_MODE_MASK			(0x1 << 0)
#define DSIM_CMD_ACCESS_MODE_GET(_v)			(((_v) >> 0) & 0x1)

/* -------------------------------------------------------------------
 * DPHY registers
 * -------------------------------------------------------------------
 */
#define DCPHY_M0_M4S0				0x0100
#define DCPHY_M1_M4S0				0x0800

/* -------------------------------------------------------------------
 * BIAS
 * -------------------------------------------------------------------
 */
#define BIAS_OFFSET				(0x0000)
#define DSIM_PHY_BIAS_CON(_id)			(BIAS_OFFSET + (4 * (_id)))
#define DSIM_PHY_BIAS_CON0			(BIAS_OFFSET + 0x0)
#define DSIM_PHY_BIAS_CON1			(BIAS_OFFSET + 0x4)
#define DSIM_PHY_REG400M(_x)			((_x) << 4)
#define DSIM_PHY_REG400M_MASK			(0x7 << 4)
#define DSIM_PHY_BIAS_CON2			(BIAS_OFFSET + 0x8)

/* -------------------------------------------------------------------
 * PLL
 * -------------------------------------------------------------------
 */
#define PLL_OFFSET				(0x0100)
#define DSIM_PHY_PLL_CON(_id)			(PLL_OFFSET + (4 * (_id)))

#define DSIM_PHY_PLL_CON0			(PLL_OFFSET + 0x0)
#define DSIM_PHY_D2AWCLK_DIV_SEL(_x)		(((_x) & 0x1) << 13)
#define DSIM_PHY_D2AWCLK_DIV_SEL_MASK		(0x1 << 13)
#define DSIM_PHY_PPI_16BIT			(0)
#define DSIM_PHY_PPI_32BIT			(1)
#define DSIM_PHY_PLL_EN(_x)			(((_x) & 0x1) << 12)
#define DSIM_PHY_PLL_EN_MASK			(0x1 << 12)
#define DSIM_PHY_PMS_S(_x)			(((_x) & 0x7) << 8)
#define DSIM_PHY_PMS_S_MASK			(0x7 << 8)
#define DSIM_PHY_PMS_S_GET(_x)			(((_x) >> 8) & 0x7)
#define DSIM_PHY_PMS_P(_x)			(((_x) & 0x3f) << 0)
#define DSIM_PHY_PMS_P_MASK			(0x3f << 0)
#define DSIM_PHY_PMS_P_GET(_x)			(((_x) >> 0) & 0x3f)

#define DSIM_PHY_PLL_CON1			(PLL_OFFSET + 0x4)
#define DSIM_PHY_DITHER_MRR(_x)			(((_x) & 0x3f) << 24)
#define DSIM_PHY_DITHER_MRR_MASK		(0x3f << 24)
#define DSIM_PHY_DITHER_MFR(_x)			(((_x) & 0xff) << 16)
#define DSIM_PHY_DITHER_MFR_MASK		(0xff << 16)
#define DSIM_PHY_USE_SDW_MASK			(0x1 << 15)
#define DSIM_PHY_M_ESCREF_EN(_x)		(((_x) & 0x1) << 14)
#define DSIM_PHY_M_ESCREF_EN_MASK		(0x1 << 14)
#define DSIM_PHY_BAND_MASK			(0x1 << 10)
#define DSIM_PHY_PMS_M(_x)			(((_x) & 0x3ff) << 0)
#define DSIM_PHY_PMS_M_MASK			(0x3ff << 0)
#define DSIM_PHY_PMS_M_GET(_x)			(((_x) >> 0) & 0x3ff)

#define DSIM_PHY_PLL_CON2			(PLL_OFFSET + 0x8)
#define DSIM_PHY_BUFF_EN_MASK			(0x1 << 31)
#define DSIM_PHY_FOUT_EN_MASK			(0x1 << 30)
#define DSIM_PHY_EXTCLK_SEL_MASK		(0x1 << 27)
#define DSIM_PHY_PLL_ENABLE_SEL_MASK		(0x1 << 24)
#define DSIM_PHY_EXTSIG_SEL_MASK		(0x1 << 23)
#define DSIM_PHY_DITHER_ICPP(_x)		(((_x) & 0x7) << 19)
#define DSIM_PHY_DITHER_ICPP_MASK		(0x7 << 19)
#define DSIM_PHY_DITHER_SEL_PF(_x)		(((_x) & 0x3) << 16)
#define DSIM_PHY_DITHER_SEL_PF_MASK		(0x3 << 16)
#define DSIM_PHY_DITHER_ICPC(_x)		(((_x) & 0x7) << 12)
#define DSIM_PHY_DITHER_ICPC_MASK		(0x7 << 12)
#define DSIM_PHY_DITHER_EN			(0x1 << 11)
#define DSIM_PHY_SSCG_EN			DSIM_PHY_DITHER_EN
#define DSIM_PHY_DITHER_BYPASS			(0x1 << 9)
#define DSIM_PHY_DITHER_AFC_ENB(_x)		(((_x) & 0x1) << 8)
#define DSIM_PHY_DITHER_AFC_ENB_MASK		(0x1 << 8)
#define DSIM_PHY_DITHER_EXTAFC(_x)		(((_x) & 0x1f) << 0)
#define DSIM_PHY_DITHER_EXTAFC_MASK		(0x1f << 0)

#define DSIM_PHY_PLL_CON3			(PLL_OFFSET + 0xC)
/* WCLK_BUF_SFT_CNT = Roundup((Word Clock Period) / 38.46 + 2) */
#define DSIM_PHY_PLL_LOCK_CNT(_x)		(((_x) & 0xffff) << 16)
#define DSIM_PHY_PLL_LOCK_CNT_MASK		(0xffff << 16)
#define DSIM_PHY_PLL_LOCK_SEL_MASK		(1 << 15)
#define DSIM_PHY_CLK_GATE_DISABLE_MASK		(1 << 12)
#define DSIM_PHY_WCLK_BUF_SFT_CNT(_x)		(((_x) & 0xf) << 8)
#define DSIM_PHY_WCLK_BUF_SFT_CNT_MASK		(0xf << 8)

#define DSIM_PHY_PLL_CON4			(PLL_OFFSET + 0x10)
#define DSIM_PHY_PLL_STB_CNT(x)			((x) << 0)
#define DSIM_PHY_PLL_STB_CNT_MASK		(0xffff << 0)
#if 0
#define DSIM_PHY_DITHER_RSEL(_x)		(((_x) & 0xf) << 12)
#define DSIM_PHY_DITHER_RSEL_MASK		(0xf << 12)
#define DSIM_PHY_DITHER_FSEL(_x)		(((_x) & 0x1) << 10)
#define DSIM_PHY_DITHER_FSEL_MASK		(0x1 << 10)
#endif

#define DSIM_PHY_PLL_CON5			(PLL_OFFSET + 0x14)
#if 0
#define DSIM_PHY_DITHER_SEL_VCO(_x)		((_x) << 2)
#define DSIM_PHY_DITHER_SEL_VCO_MASK		(0x1 << 2)
#endif

#define DSIM_PHY_PLL_CON6			(PLL_OFFSET + 0x18)
#define DSIM_PHY_PMS_F(_x)			(((_x) & 0xffffffff) << 0)
#define DSIM_PHY_PMS_F_MASK			(0xffffffff << 0)
#define DSIM_PHY_PMS_F_GET(_x)			(((_x) >> 0) & 0xffffffff)

#define DSIM_PHY_PLL_CON7			(PLL_OFFSET + 0x1C)
#define DSIM_PHY_PLL_CON8			(PLL_OFFSET + 0x20)

#define DSIM_PHY_PLL_STAT0			(PLL_OFFSET + 0x40)
#define DSIM_PHY_PLL_LOCK_GET(x)		(((x) >> 0) & 0x1)

/* -------------------------------------------------------------------
 * MC: master clock lane
 * -------------------------------------------------------------------
 */
#define MC_OFFSET				(0x0300)

/* GNR: General Control Register */
#define DSIM_PHY_MC_GNR_CON(_id)		(MC_OFFSET + (4 * (_id)))
#define DSIM_PHY_MC_GNR_CON0			(MC_OFFSET + 0x0)
#define DSIM_PHY_T_PHY_READY(_x)		(((_x) & 0xffff) << 16)
#define DSIM_PHY_T_PHY_READY_MASK		(0xffff << 16)
#define DSIM_PHY_PPI_DATA_WIDTH			(0x1 << 5)
#define DSIM_PHY_FORCE_SFR_PPI_SEL		(0x1 << 4)
#define DSIM_PHY_PHY_READY			(0x1 << 1)
#define DSIM_PHY_PHY_READY_GET(x)		(((x) >> 1) & 0x1)
#define DSIM_PHY_PHY_ENABLE			(0x1 << 0)

/* ANA: Analog Block Control Register */
#define DSIM_PHY_MC_ANA_CON(_id)		(MC_OFFSET + 0x8 + (4 * (_id)))
#define DSIM_PHY_MC_ANA_CON0			(MC_OFFSET + 0x8)
#define DSIM_PHY_DPDN_SWAP(_x)			(((_x) & 0x1) << 23)
#define DSIM_PHY_DPDN_SWAP_MASK			(0x1 << 23)
#define DSIM_PHY_EDGE_CON_EN			(0x1 << 12)
#define DSIM_PHY_EDGE_CON_DIR			(0x1 << 11)
#define DSIM_PHY_EDGE_CON(_x)			(((_x) & 0x7) << 8)
#define DSIM_PHY_EDGE_CON_MASK			(0x7 << 8)
#define DSIM_PHY_RES_UP(_x)			(((_x) & 0xf) << 4)
#define DSIM_PHY_RES_UP_MASK			(0xf << 4)
#define DSIM_PHY_RES_DN(_x)			(((_x) & 0xf) << 0)
#define DSIM_PHY_RES_DN_MASK			(0xf << 0)

#define DSIM_PHY_MC_ANA_CON1			(MC_OFFSET + 0xC)
#define DSIM_PHY_BIAS_SLEEP_EN(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_BIAS_SLEEP_EN_MASK		(0x1 << 0)

/* master clock lane setting */
#define DSIM_PHY_MC_TIME_CON0			(MC_OFFSET + 0x30)
#define DSIM_PHY_MC_TIME_CON1			(MC_OFFSET + 0x34)
#define DSIM_PHY_MC_TIME_CON2			(MC_OFFSET + 0x38)
#define DSIM_PHY_MC_DATA_CON0			(MC_OFFSET + 0x40)
#define DSIM_PHY_MC_DESKEW_CON0			(MC_OFFSET + 0x50)
#define DSIM_PHY_MC_DBG_STAT0			(MC_OFFSET + 0xE0)

/* -------------------------------------------------------------------
 * MD: master data lane
 * -------------------------------------------------------------------
 * master data lane setting : D0 ~ D3
 * D0~D2 : COMBO
 * D3    : DPHY
 */
#define MD_OFFSET(_x)				(0x0400 + (0x100 * (_x)))

#define DSIM_PHY_MD_GNR_CON0(_x)		(MD_OFFSET(_x) + 0x0)
#define DSIM_PHY_MD_ANA_CON0(_x)		(MD_OFFSET(_x) + 0x8)
#define DSIM_PHY_MD_ANA_CON1(_x)		(MD_OFFSET(_x) + 0xC)
#define DSIM_PHY_MD_TIME_CON0(_x)		(MD_OFFSET(_x) + 0x30)
#define DSIM_PHY_MD_TIME_CON1(_x)		(MD_OFFSET(_x) + 0x34)
#define DSIM_PHY_MD_TIME_CON2(_x)		(MD_OFFSET(_x) + 0x38)
#define DSIM_PHY_MD_DATA_CON0(_x)		(MD_OFFSET(_x) + 0x3C)
#define DSIM_PHY_MD_DBG_STAT0(_x)		(MD_OFFSET(_x) + 0xC0)

/* macros for DPHY timing controls */
/* MC/MD_TIME_CON0 */
#define DSIM_PHY_HSTX_CLK_SEL(_x)		(((_x) & 0x1) << 12)
#define DSIM_PHY_HSTX_CLK_SEL_MASK		(0x1 << 12)
#define DSIM_PHY_TLPX(_x)			(((_x) & 0xff) << 4)
#define DSIM_PHY_TLPX_MASK			(0xff << 4)
/* MC only */
#define DSIM_PHY_TCLK_ZERO(_x)			(((_x) & 0xff) << 24)
#define DSIM_PHY_TCLK_ZERO_MASK			(0xff << 24)
#define DSIM_PHY_TCLK_PREPARE(_x)		(((_x) & 0xff) << 16)
#define DSIM_PHY_TCLK_PREPARE_MASK		(0xff << 16)
/* MD only */
#define DSIM_PHY_THS_ZERO(_x)			(((_x) & 0xff) << 24)
#define DSIM_PHY_THS_ZERO_MASK			(0xff << 24)
#define DSIM_PHY_THS_PREPARE(_x)		(((_x) & 0xff) << 16)
#define DSIM_PHY_THS_PREPARE_MASK		(0xff << 16)
#define DSIM_PHY_TLP_EXIT_SKEW(_x)		(((_x) & 0x3) << 2)
#define DSIM_PHY_TLP_EXIT_SKEW_MASK		(0x3 << 2)
#define DSIM_PHY_TLP_ENTRY_SKEW(_x)		(((_x) & 0x3) << 0)
#define DSIM_PHY_TLP_ENTRY_SKEW_MASK		(0x3 << 0)

/* MC/MD_TIME_CON1 */
#define DSIM_PHY_THS_EXIT(_x)			(((_x) & 0xff) << 8)
#define DSIM_PHY_THS_EXIT_MASK			(0xff << 8)
/* MC only */
#define DSIM_PHY_TCLK_POST(_x)			(((_x) & 0xff) << 16)
#define DSIM_PHY_TCLK_POST_MASK			(0xff << 16)
#define DSIM_PHY_TCLK_TRAIL(_x)			(((_x) & 0xff) << 0)
#define DSIM_PHY_TCLK_TRAIL_MASK		(0xff << 0)
/* MD only */
#define DSIM_PHY_TTA_GET(_x)			(((_x) & 0xf) << 20)
#define DSIM_PHY_TTA_GET_MASK			(0xf << 20)
#define DSIM_PHY_TTA_GO(_x)			(((_x) & 0xf) << 16)
#define DSIM_PHY_TTA_GO_MASK			(0xf << 16)
#define DSIM_PHY_THS_TRAIL(_x)			(((_x) & 0xff) << 0)
#define DSIM_PHY_THS_TRAIL_MASK			(0xff << 0)

/* MC/MD_TIME_CON2 */
#define DSIM_PHY_WCLK_GATING_DISABLE(_x)	(((_x) & 0x1) << 11)
#define DSIM_PHY_ULPS_EXIT(_x)			(((_x) & 0x3ff) << 0)
#define DSIM_PHY_ULPS_EXIT_MASK			(0x3ff << 0)

/* MC_DATA_CON0 */
#define DSIM_PHY_CLK_INV			(0x1 << 1)
/* MD_DATA_CON0 */
#define DSIM_PHY_DATA_INV			(0x1 << 1)

/* MC_DESKEW_CON0 */
#define DSIM_PHY_SKEWCAL_RUN_TIME(_x)		(((_x) & 0xf) << 12)
#define DSIM_PHY_SKEWCAL_RUN_TIME_MASK		(0xf << 12)
#define DSIM_PHY_SKEWCAL_INIT_RUN_TIME(_x)	(((_x) & 0xf) << 8)
#define DSIM_PHY_SKEWCAL_INIT_RUN_TIME_MASK	(0xf << 8)
#define DSIM_PHY_SKEWCAL_INIT_WAIT_TIME(_x)	(((_x) & 0xf) << 4)
#define DSIM_PHY_SKEWCAL_INIT_WAIT_TIME_MASK	(0xf << 4)
#define DSIM_PHY_SKEWCAL_EN			(0x1 << 0)

#endif /* _REGS_DSIM_H */
