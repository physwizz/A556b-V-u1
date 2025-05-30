/* SPDX-License-Identifier: GPL-2.0-only
 *
 * cal_st/regs-dsim.h
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

enum dsim_regs_id {
	REGS_DSIM0_ID = 0,
	REGS_DSIM_ID_MAX
};
#define MAX_DSI_CNT REGS_DSIM_ID_MAX

enum dsim_regs_type {
	REGS_DSIM_DSI = 0,
	REGS_DSIM_PHY,
	REGS_DSIM_PHY_BIAS,
	REGS_DSIM_SYS,
	REGS_DSIM_TYPE_MAX
};

/* SYSREG_DPU -> DPU_MIPI_PHY_CON : 0x1008 */
#define DPU_MIPI_PHY_CON			(0x0008)
#define SEL_RESET_DPHY_MASK(_v)			(0x1 << (4 + (_v)))

#define M_RESETN_M1_MASK			(0x1 << 1) // Invalid for Santa
#define M_RESETN_M0_MASK			(0x1 << 0)
#endif /* _REGS_SYSREG_DISP_H */

#define DSIM_RX_FIFO_MAX_DEPTH                  64 // size = 4 bytes * 64
#define DSIM_PL_FIFO_THRESHOLD			2048	/*this value depends on H/W */
#define DSIM_FCMD_ALIGN_CONSTRAINT		8

#ifndef _REGS_DSIM_H
#define _REGS_DSIM_H

#define DSIM_VERSION					(0x0000)
#define VERSION_INFO					0x02090300
#define DSIM_VERSION_GET_MAJOR(_v)			(((_v) >> 24) & 0x000000ff)
#define DSIM_VERSION_GET_MINOR(_v)			(((_v) >> 16) & 0x000000ff)

#define DSIM_SWRST					(0x0004)
#define DSIM_DPHY_RST					(1 << 16)
#define DSIM_SWRST_FUNCRST				(1 << 8)
#define DSIM_SWRST_RESET				(1 << 0)

#define DSIM_LINK_STATUS0				(0x0008)
#define DSIM_LINK_STATUS0_VIDEO_MODE_STATUS_GET(x)	((x >> 24) & 0x1)
#define DSIM_LINK_STATUS0_VT_HSTATE_GET(x)		((x >> 18) & 0x3f)
#define VT_HSTATE_HIDLE					(0x1)
#define VT_HSTATE_HSA					(0x2)
#define VT_HSTATE_HBP					(0x4)
#define VT_HSTATE_HACT					(0x8)
#define VT_HSTATE_HFP					(0x10)
#define VT_HSTATE_HUNDRUN				(0x20)
#define DSIM_LINK_STATUS0_VT_VSTATE_GET(x)		((x >> 13) & 0x1f)
#define VT_VSTATE_VIDLE					(0x1)
#define VT_VSTATE_VSA					(0x2)
#define VT_VSTATE_VBP					(0x4)
#define VT_VSTATE_VACT					(0x8)
#define VT_VSTATE_VFP					(0x10)
#define DSIM_LINK_STATUS0_VM_LINE_CNT_GET(x)		((x >> 0) & 0x1fff)

#define DSIM_LINK_STATUS1				(0x000c)
#define DSIM_LINK_STATUS1_CMD_MODE_STATUS_GET(x)	((x >> 26) & 0x1)
#define DSIM_LINK_STATUS1_CMD_LOCK_STATUS_GET(x)	((x >> 25) & 0x1)
#define DSIM_LINK_STATUS1_CMD_TRANSF_CNT_GET(x)		((x >> 0) & 0x1ffffff)
#define DSIM_STATUS_IDLE				(0)
#define DSIM_STATUS_ACTIVE				(1)
#define DSIM_STATUS_CMDLOCK				(1)

#define DSIM_LINK_STATUS2				(0x10)
/*TBD: Many other bit fields needs to be added */
/* 0: STOPDATA, 1:HSDT, 2:LPDT, 3:TRIGGER,
 * 4: ULPSDATA, 5:SKEWCALDATA * 6: BTA
 */
#define DSIM_LINK_STATUS2_DATALANE_STATUS_GET(x)	((x >> 0) & 0x7)
#define DSIM_LINK_STATUS2_HSDT_STATUS			(0x7 << 21)
#define DSIM_LINE_STATUS2_HSDT_STATUS_SHIFT		21
#define DSIM_LINK_STATUS2_HSCLK_STATUS			(0x7 << 8)
#define DSIM_LINK_STATUS2_HSCLK_STATUS_SHIFT		8

#define DSIM_LINK_STATUS3				(0x0014)
/* TBD: Many bit field might need to be added as per usage
 * [5:4]: MUX_STATUS, [7:6]: ESC_CLK_MUX_STATUS, [14:12]: PLL_SLEEP_STATUS
 * [18:16]: PHY_CFG_STATUS
 */
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_GET(x)       (((x) >> 12) & 0x7)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_IDLE         (0)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_ALVCLK	(1)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_PLL_SLEEP    (2)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_PLL_WAKEUP   (3)
#define DSIM_LINK_STATUS3_PLL_SLEEP_STATUS_WCLK         (4)
#define DSIM_LINK_STATUS3_PLL_STABLE                    (1 << 0)

#define DSIM_MIPI_STATUS				(0x0018)
#define DSIM_MIPI_STATUS_FRM_PROCESSING			(1 << 29)
#define DSIM_MIPI_STATUS_FRM_DONE			(1 << 28)
#define DSIM_MIPI_STATUS_SHADOW_REG_UP_EN		(1 << 25)
#define DSIM_MIPI_STATUS_SHADOW_REG_UP_DONE		(1 << 24)
#define DSIM_MIPI_STATUS_INSTANT_OFF_REQ		(1 << 21)
#define DSIM_MIPI_STATUS_INSTANT_OFF_ACK		(1 << 20)
#define DSIM_MIPI_STATUS_TE				(1 << 0)

#define DSIM_DPHY_STATUS				(0x001c)
/*TBD: Many other bit fields needs to be added
 * [21]: TX_REQUEST_HSCLK, [20]: ULPS_RX_DATA0, [19]: DIRECTION,
 * [18:15]: TX_READY_HS_DATA, [14:11]: TX_REQUEST_HS_DATA,
 */
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
#define DSIM_CLK_CTRL_NONCONT_CLOCK_LANE		(1 << 25) // Command mode only
#define DSIM_CLK_CTRL_CLKLANE_ONOFF			(1 << 24) // Video mode only
#define DSIM_CLK_CTRL_TX_REQUEST_HSCLK			(1 << 20)
/*
 * This register is used to receive the osc clock from
 * the idle state when using the DSIM.
 *  0 = when power is down.
 *  1 = when using DSIM.
 */
#define DSIM_CLK_CTRL_ALV_GATE_CONDITION		(1 << 18)
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
#define DSIM_NUM_OF_TRANSFER_PER_FRAME(_x)		((_x) << 0) // Only in command mode
#define DSIM_NUM_OF_TRANSFER_PER_FRAME_MASK		(0xfffff << 0)
#define DSIM_NUM_OF_TRANSFER_PER_FRAME_GET(x)		(((x) >> 0) & 0xfffff)

#define DSIM_UNDERRUN_CTRL				(0x0034)
#define DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF(_x)	((_x) << 0)	/* SHD */
#define DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF_MASK	(0xffff << 0)

#define DSIM_THRESHOLD					(0x0038)
#define DSIM_THRESHOLD_NULL_WC(_x)			((_x) << 16)	/* SHD */
#define DSIM_THRESHOLD_NULL_WC_MASK			(0xffff << 16)
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
#define DSIM_INTSRC_ERR_ESC0				(1 << 10)
#define DSIM_INTSRC_INT_FX_DATA_PL_BUF_FULL		(1 << 9) // FX_* is may be for automotive project
#define DSIM_INTSRC_INT_FX_DATA_PL_SDW_BUF_FULL		(1 << 8) // FX_* is may be for automotive project
#define DSIM_INTSRC_ERR_SYNC0				(1 << 6)
#define DSIM_INTSRC_INT_FX_DATA_PH_BUF_FULL		(1 << 5) // FX_* is may be for automotive project
#define DSIM_INTSRC_INT_FX_DATA_PH_SDW_BUF_FULL		(1 << 4) // FX_* is may be for automotive project
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
#define DSIM_INTMSK_ERR_ESC0				(1 << 10)
#define DSIM_INTMSK_FX_DATA_PL_BUF_FULL			(1 << 9) // FX_* is may be for automotive project
#define DSIM_INTMSK_FX_DATA_PL_SDW_BUF_FULL		(1 << 8) // FX_* is may be for automotive project
#define DSIM_INTMSK_ERR_SYNC0				(1 << 6)
#define DSIM_INTMSK_FX_DATA_PH_BUF_FULL			(1 << 5) // FX_* is may be for automotive project
#define DSIM_INTMSK_FX_DATA_PH_SDW_BUF_FULL		(1 << 4) // FX_* is may be for automotive project
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
#define DSIM_SFR_CTRL_SHADOW_REG_READ_EN		(1 << 1)
#define DSIM_SFR_CTRL_SHADOW_EN				(1 << 0)

/* FIFO status and control register */
#define DSIM_FIFOCTRL					(0x0068)
#define DSIM_FIFOCTRL_NUMBER_OF_PH_SFR(_x)		(((_x) & 0x3f) << 16)
#define DSIM_FIFOCTRL_NUMBER_OF_PH_SFR_GET(x)		(((x) >> 16) & 0x3f)
#define DSIM_FIFOCTRL_PL_IMG_FIFO_FULL			(1 << 15)
#define DSIM_FIFOCTRL_PL_IMG_FIFO_EMPTY			(1 << 14)
#define DSIM_FIFOCTRL_FIFO_FULL_RX			(1 << 13)
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
#define DSIM_CMD_CONFIG_PKT_GO_EN			(1 << 16)
#define DSIM_CMD_CONFIG_MULTI_CMD_PKT_EN		(1 << 8)
#define DSIM_CMD_CONFIG_MULTI_PKT_CNT(_x)		((_x) << 0)
#define DSIM_CMD_CONFIG_MULTI_PKT_CNT_MASK		(0x3f << 0)

/* TE based command register*/
#define DSIM_CMD_TE_CTRL0				(0x0084)
#define DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP(_x)		((_x) << 0)
#define DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP_MASK		(0xffff << 0)

/* TE based command register*/
#define DSIM_CMD_TE_CTRL1				(0x0088)
#define DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON(_x)	((_x) << 16)
#define DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON_MASK	(0xffff << 16)
#define DSIM_CMD_TE_CTRL1_TIME_TE_TOUT(_x)		((_x) << 0)
#define DSIM_CMD_TE_CTRL1_TIME_TE_TOUT_MASK		(0xffff << 0)

/*Command Mode Status register*/
#define DSIM_CMD_STATUS					(0x008c)
/* Command mode: 0=IDLE, 1=TE_ON, 2=FRM_START, 3=FRM_DONE,
 * 	4=CMD_ALW, 5=FRM_MSK, 6=PRT_ON
 * Video Mode: 0=IDLE, 1=IMG, 2=STABLE_VFP, 3=CMD_LOCK,
 * 	4=CMD_ALW, 5=CMD_MSK
 */
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
#define	DSIM_BIST_CTRL0_BIST_OPT_COLOR_BAR		(1 << 7)
#define	DSIM_BIST_CTRL0_BIST_COLOR_ORDER_SWAP		(1 << 6)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MOVE_EN		(1 << 4)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MODE(_x)		((_x) << 1)
#define	DSIM_BIST_CTRL0_BIST_PTRN_MODE_MASK		(0x7 << 1)
#define	DSIM_BIST_CTRL0_BIST_EN				(1 << 0)

/*BIST generation register*/
#define	DSIM_BIST_CTRL1					(0x0098)
#define	DSIM_BIST_CTRL1_BIST_PTRN_PRBS7_SEED(_x)	((_x) << 24)
#define	DSIM_BIST_CTRL1_BIST_PTRN_PRBS7_SEED_MASK	(0x7f << 24)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_R(_x)		((_x) << 12)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_R_MASK		(0X3FF << 12)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_B(_x)		((_x) << 0)
#define	DSIM_BIST_CTRL1_BIST_PTRN_USER_B_MASK		(0x3FF << 0)

/*DSIM to CSI loopback register*/
#define	DSIM_CSIS_LB					(0x009C)
#define DSIM_CSIS_LB_1BYTEPPI_MODE			(1 << 9)
#define	DSIM_CSIS_LB_CSIS_LB_EN				(1 << 8)
#define DSIM_CSIS_LB_CSIS_PH(_x)			((_x) << 0)
#define DSIM_CSIS_LB_CSIS_PH_MASK			(0xff << 0)

/*TBD:
#define DSIM_PLL_CTRL					(0x00A0)
#define DSIM_PLL_CTRL1					(0x00A4)
#define DSIM_PLL_CTRL2					(0x00A8)
#define DSIM_PLL_TMR					(0x00AC)
#define DSIM_PLL_CTRL_B1				(0x00B0)
#define DSIM_PLL_CTRL_B2				(0x00B4)
#define DSIM_PLL_CTRL_B3				(0x00B8)
#define DSIM_PLL_CTRL_B4				(0x00BC)
#define DSIM_PLL_CTRL_M1				(0x00C0)
#define DSIM_PLL_CTRL_M2				(0x00C4)
#define DSIM_PLL_CTRL_M3				(0x00C8)
#define DSIM_PLL_CTRL_M4				(0x00CC)

#define DSIM_PHY_TIMING					(0x00D0)
#define DSIM_PHY_TIMING1				(0x00D4)
#define DSIM_PHY_TIMING2				(0x00D8)
*/

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

/*TBD: Bit field*/
#define DSIM_PLL_GUARD_TIMER				(0x0100)

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

/* Bit [25] [24] [23] are Feature out(does not require implementation)
 * or feature for automotive projects
 */
#define DSIM_OPTION_SUITE_OPT_FX_DATA_TRANSF		(1 << 25)
#define DSIM_OPTION_SUITE_OPT_WD_SLOW_THAN_ESC		(1 << 24)
#define DSIM_OPTION_SUITE_OPT_CLKLANE_INITN		(1 << 23)
#define DSIM_OPTION_SUITE_OPT_SHADOW_SYNC_CMD_ALLOW	(1 << 22) // Refer to the change spec of v2.92, chapter 3.7
#define DSIM_OPTION_SUITE_OPT_PLL_SLEEP_SELF_CTRL_MASK	(0x1 << 21) // Refer to the change spec of v2.92, chapter 3.3
#define	DSIM_OPTION_SUITE_OPT_PLL_SLEEP_SELF_CTRL_GET(x) (((x) >> 21) & 0x1)
#define	DSIM_OPTION_SUITE_CPHY_LB_TEST_EN_MASK		(0x1 << 20)
#define	DSIM_OPTION_SUITE_LP_LENGTH__MASK		(0xff << 12)
#define	DSIM_OPTION_SUITE_FORCE_LP_IN_LAST_LINE_MASK	(0x1 << 11)
#define	DSIM_OPTION_SUITE_OPT_TE_ON_CMD_ALLOW_MASK	(0x1 << 10)
#define	DSIM_OPTION_SUITE_OPT_EXT_VT_SYNC_MASK		(0x1 << 9)
#define	DSIM_OPTION_SUITE_SYNC_MODE_EN_MASK		(0x1 << 8)
#define	DSIM_OPTION_SUITE_OPT_VT_COND_MASK		(0x1 << 7)
#define	DSIM_OPTION_SUITE_OPT_FD_COND_MASK		(0x1 << 6)
#define	DSIM_OPTION_SUITE_OPT_KEEP_VFP_MASK		(0x1 << 5)
#define	DSIM_OPTION_SUITE_OPT_ALIVE_MODE_MASK		(0x1 << 4)
#define	DSIM_OPTION_SUITE_OPT_CM_EXT_CMD_ALLOW_MASK	(0x1 << 3)
#define	DSIM_OPTION_SUITE_OPT_USER_EXPIRE_VFP_MASK	(0x1 << 2)
#define	DSIM_OPTION_SUITE_EMIRROR_EN_MASK		(0x1 << 1)
#define	DSIM_OPTION_SUITE_CFG_UPDT_EN_MASK		(0x1 << 0)

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
#define DSIM_FIFO_CMD_PH_FIFO_REMAIN(_x)		((_x) << 16)
#define DSIM_FIFO_CMD_PH_FIFO_REMAIN_MASK		(0x3F << 16)
#define DSIM_FIFO_CMD_PL_FIFO_REMAIN(_x)		((_x) << 0)
#define DSIM_FIFO_CMD_PL_FIFO_REMAIN_MASK		(0xFFFF << 0)

#define DSIM_FREQ_HOPP					(0x0120)	/* SHD */
#define DSIM_FREQ_HOPP_FH_LINE_CNT(_x)			((_x) << 16)
#define DSIM_FREQ_HOPP_FH_LINE_CNT_MASK			(0xffff << 16)
#define DSIM_FREQ_HOPP_FH_PRESCALER(_x)			((_x) << 4)
#define DSIM_FREQ_HOPP_FH_PRESCALER_MASK		(0xff << 4)
#define DSIM_FREQ_HOPP_OSC_SW_EN_MASK			(0x1 << 3)
#define DSIM_FREQ_HOPP_ALV_SW_EN_MASK			(0x1 << 2)
#define DSIM_FREQ_HOPP_FH_EN_MASK			(0x1 << 0)

#define DSIM_SHADOW_CONFIG				(0x0124)
#define DSIM_SHADOW_CONFIG_HACT_COMPENSATE(_x)		((_x) << 16)	/* SHD */
#define DSIM_SHADOW_CONFIG_AFTER_HOPP_COMPENSATE_EN_MASK	(0x1 << 2)	/* SHD */
#define DSIM_SHADOW_CONFIG_SHADOW_VSS_UPDT_MASK		(0x1 << 0)

#define DSIM_FH_COMPENSATE				(0x0128)
#define DSIM_FH_COMPENSATE_FH_DELAY_CNT(_x)		((_x) << 16)
#define DSIM_FH_COMPENSATE_FH_DELAY_CNT_MASK		(0xffff << 16)
#define DSIM_FH_COMPENSATE_LAST_LINE_COMPENSATE(_x)	((_x) << 8)
#define DSIM_FH_COMPENSATE_LAST_LINE_COMPENSATE_MASK	(0xff<< 8)
#define DSIM_FH_COMPENSATE_LAST_LINE_COMPENSATE_EN_MASK	(0x1 << 4)
#define DSIM_FH_COMPENSATE_FH_START_POINT(_x)		((_x) << 0)
#define DSIM_FH_COMPENSATE_FH_START_POINT_MASK		(0xff << 0)

#define DSIM_CPHY_LB_TEST0				(0x12C)
#define DSIM_CPHY_LB_TEST0_PLD_LENGHT(_x)		((_x) << 16)
#define DSIM_CPHY_LB_TEST0_PLD_LENGHT_MASK		(0xffff << 16)
#define DSIM_CPHY_LB_TEST0_FRAME_INTERVAL(_x)		((_x) << 8)
#define DSIM_CPHY_LB_TEST0_FRAME_INTERVAL_MASK		(0xff << 16)
#define DSIM_CPHY_LB_TEST0_NUM_OF_FRAME_SENT(_x)	((_x) << 4)
#define DSIM_CPHY_LB_TEST0_NUM_OF_FRAME_SENT_MASK	(0xf << 4)
#define DSIM_CPHY_LB_TEST0_NUM_OF_ACTIVE_LANE(_x)	((_x) << 2)
#define DSIM_CPHY_LB_TEST0_NUM_OF_ACTIVE_LANE_MASK	(0x3 << 2)
#define DSIM_CPHY_LB_TEST0_LB_PAT_GEN_RST		(1 << 1)
#define DSIM_CPHY_LB_TEST0_LB_PAT_GEN_ON		(1 << 0)

#define DSIM_CPHY_LB_TEST1				(0x130)
#define DSIM_CPHY_LB_TEST1_PAYLOAD_CRC(_x)		((_x) << 16)
#define DSIM_CPHY_LB_TEST1_PAYLOAD_CRC_MASK		(0xffff << 16)
#define DSIM_CPHY_LB_TEST1_HEADER_CRC(_x)		((_x) << 0)
#define DSIM_CPHY_LB_TEST1_HEADER_CRC_MASK		(0xffff << 0)

#define DSIM_BIST_CTRL2					(0x134)
#define DSIM_BIST_CTRL2_BIST_PTRN_USER_G1(_x)		((x_) << 12)
#define DSIM_BIST_CTRL2_BIST_PTRN_USER_G1_MASK		(0x3ff << 12)
#define DSIM_BIST_CTRL2_BIST_PTRN_USER_G0(_x)		((x_) << 0)
#define DSIM_BIST_CTRL2_BIST_PTRN_USER_G0_MASK		(0x3ff << 0)

#define DSIM_VMC_MISMATCH0				(0x138)
#define DSIM_VMC_MISMATCH0_DBG_MISMATCH_VIDEO_TIMING(_x)	((_x) << 0)
#define DSIM_VMC_MISMATCH0_DBG_MISMATCH_VIDEO_TIMING_MASK	(0xf << 0)

#define DSIM_VMC_MISMATCH1				(0x13C)
#define DSIM_VMC_MISMATCH0_DBG_MISMATCH_FIRST_LINE(_x)	((_x) << 16)
#define DSIM_VMC_MISMATCH0_DBG_MISMATCH_FIRST_LINE_MASK	(0xffff << 16)
#define DSIM_VMC_MISMATCH0_DBG_MISMATCH_SECOND_LINE(_x)		((_x) << 0)
#define DSIM_VMC_MISMATCH0_DBG_MISMATCH_SECOND_LINE_MASK	(0xffff << 0)

/* These SFRs are out of spec OR used for ASB OR used for automatic products
 * 0x0140 to 0x017C
 */
#define DSIM_NULL_GEN_CTRL				(0x0140)
#define DSIM_CPHY_LB_TEST2  				(0x0144)
#define DSIM_PHY_LANE_CONFIG  				(0x0148)
#define DSIM_PPI_TEST_CONFIG0  				(0x014c)
#define DSIM_PPI_TEST_CONFIG1  				(0x0150)
#define DSIM_PPI_TEST_CONFIG2  				(0x0154)
#define DSIM_PPI_TEST_CONFIG3  				(0x0158)
#define DSIM_PPI_TEST_CONFIG4  				(0x015c)
#define DSIM_PPI_TEST_CONFIG5  				(0x0160)
#define DSIM_PPI_TEST_CONFIG6  				(0x0164)
#define DSIM_PPI_TEST_CONFIG7  				(0x0168)
#define DSIM_DBG_VIDEO_TIMING0  			(0x016c)
#define DSIM_DBG_VIDEO_TIMING1  			(0x0170)
#define DSIM_FX_DATA_BUF_CTRL 				(0x0174)
#define DSIM_FX_DATA_PH  				(0x0178)
#define DSIM_FX_DATA_PL  				(0x017c)

#define DSIM_TRANS_MODE					(0x0400)
#define DSIM_CMD_CPU_ACCESS_MODE			(0)
#define DSIM_CMD_DMA_ACCESS_MODE			(1)
#define DSIM_CMD_ACCESS_MODE(_v)			((_v) << 0)
#define DSIM_CMD_ACCESS_MODE_MASK			(0x1 << 0)
#define DSIM_CMD_ACCESS_MODE_GET(_v)			(((_v) >> 0) & 0x1)

/*
 * DPHY  registers
 */
#if 0 // dcphy 6310
#define DCPHY_M0_M4S0				0x0100
#define DCPHY_M1_M4S0				0x0900
#endif

/* DPHY BIAS setting
 * _id : [0, 4]
 * DCPHY_M4S0_COMMON: 0x168E_0000
 */
#if 0 // dcphy 6310
#define DSIM_PHY_BIAS_CON(_id)			(0x0000 + (4 * (_id)))
#define DSIM_PHY_BIAS_CON0			(0x0000)
#define DSIM_PHY_BIAS_CON1			(0x0004)
#define DSIM_PHY_BIAS_CON2			(0x0008)
#define DSIM_PHY_REG400M(_x)			((_x) << 4)
#define DSIM_PHY_REG400M_MASK			(0x7 << 4)
#define DSIM_PHY_BIAS_CON3			(0x000C)
#define DSIM_PHY_BIAS_CON4			(0x0010)
#define DSIM_PHY_BIAS_CON5			(0x0014)
#endif

/* DPHY PLL setting
 * SS_DCPHY_M4_TOP 0x168E_0000
 */
#if 0 // dcphy 6310
#define DSIM_PHY_PLL_CON(_id)			(0x0000 + (4 * (_id)))
#define DSIM_PHY_PLL_CON0			(0x0000)
#define DSIM_PHY_PLL_CON1			(0x0004)
#define DSIM_PHY_PLL_CON2			(0x0008)
#define DSIM_PHY_PLL_CON3			(0x000C)
#define DSIM_PHY_PLL_CON4			(0x0010)
#define DSIM_PHY_PLL_CON5			(0x0014)
#define DSIM_PHY_PLL_CON6			(0x0018)
#define DSIM_PHY_PLL_CON7			(0x001C)
#define DSIM_PHY_PLL_CON8			(0x0020)
#define DSIM_PHY_PLL_CON9			(0x0024)
#else // dcphy 6410
#define DSIM_PHY_M0_PLL_CON0			(0x0000)
#define DSIM_PHY_M0_PLL_CON1			(0x0020)
#define DSIM_PHY_M0_PLL_CON2			(0x0024)
#endif

/* PLL_CON0 */
#if 0 // dcphy 6310
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
#else // dcphy 6410
#define DSIM_PHY_CD_SEL_DC_MODE(_x)		(((_x) & 0x1) << 20)
#define DSIM_PHY_CD_SEL_DC_MODE_MASK		(0x1 << 20)

#define DSIM_PHY_CD_SEL_TEST_FEED_OUT(_x)	(((_x) & 0x1) << 18)
#define DSIM_PHY_CD_SEL_TEST_FEED_OUT_MASK	(0x1 << 18)

#define DSIM_PHY_CD_SEL_TEST_DIV_RATIO(_x)	(((_x) & 0x1) << 17)
#define DSIM_PHY_CD_SEL_TEST_DIV_RATIO_MASK	(0x1 << 17)

#define DSIM_PHY_CD_EN_PLL_DIV(_x)		(((_x) & 0x1) << 16)
#define DSIM_PHY_CD_EN_PLL_DIV_MASK		(0x1 << 16)

#define DSIM_PHY_PLL_RSEL(_x)			(((_x) & 0xf) << 12)
#define DSIM_PHY_PLL_RSEL_MASK			(0xf << 12)

#define DSIM_PHY_PLL_BYPASS(_x)			(((_x) & 0x1) << 11)
#define DSIM_PHY_PLL_BYPASS_MASK		(0x1 << 11)

#define DSIM_PHY_PLL_FOUT_MASK(_x)		(((_x) & 0x1) << 10)
#define DSIM_PHY_PLL_FOUT_MASK_MASK		(0x1 << 10)

#define DSIM_PHY_PLL_EN_FEED(_x)		(((_x) & 0x1) << 9)
#define DSIM_PHY_PLL_EN_FEED_MASK		(0x1 << 9)

#define DSIM_PHY_PLL_FSEL(_x)			(((_x) & 0x1) << 8)
#define DSIM_PHY_PLL_FSEL_MASK			(0x1 << 8)

#define DSIM_PHY_PLL_ICP(_x)			(((_x) & 0x3) << 6)
#define DSIM_PHY_PLL_ICP_MASK			(0x3 << 6)

#define DSIM_PHY_PLL_ENB_AFC(_x)		(((_x) & 0x1) << 5)
#define DSIM_PHY_PLL_ENB_AFC_MASK		(0x1 << 5)

#define DSIM_PHY_PLL_EXTAFC(_x)			(((_x) & 0x1f) << 0)
#define DSIM_PHY_PLL_EXTAFC_MASK		(0x1f << 0)

#endif
/* PLL_CON1 */
#if 0 // dcphy 6310
#define DSIM_PHY_PMS_K(_x)			(((_x) & 0xffff) << 0)
#define DSIM_PHY_PMS_K_MASK			(0xffff << 0)
#define DSIM_PHY_PMS_K_GET(_x)			(((_x) >> 0) & 0xffff)
#else // dcphy 6410

#define DSIM_PHY_PMS_K(_x)			(((_x) & 0xffff) << 16)
#define DSIM_PHY_PMS_K_MASK			(0xffff << 16)
#define DSIM_PHY_PMS_K_GET(_x)			(((_x) >> 16) & 0xffff)

#define DSIM_PHY_PMS_M(_x)			(((_x) & 0xff) << 8)
#define DSIM_PHY_PMS_M_MASK			(0xff << 8)
#define DSIM_PHY_PMS_M_GET(_x)			(((_x) >> 8) & 0xff)

#define DSIM_PHY_PMS_P(_x)			(((_x) & 0x3f) << 2)
#define DSIM_PHY_PMS_P_MASK			(0x3f << 2)
#define DSIM_PHY_PMS_P_GET(_x)			(((_x) >> 2) & 0x3f)

#define DSIM_PHY_PMS_M_MSB(_x)			(((_x) & 0x3) << 0)
#define DSIM_PHY_PMS_M_MSB_MASK			(0x3 << 0)
#define DSIM_PHY_PMS_M_MSB_GET(_x)		(((_x) >> 0) & 0x3)

#endif

/* PLL_CON2 */
#if 0 // dcphy 6310
#define DSIM_PHY_USE_SDW_MASK			(0x1 << 15)
#define DSIM_PHY_M_ESCREF_EN(_x)		(((_x) & 0x1) << 14)
#define DSIM_PHY_M_ESCREF_EN_MASK		(0x1 << 14)
#define DSIM_PHY_PMS_M(_x)			(((_x) & 0x3ff) << 0)
#define DSIM_PHY_PMS_M_MASK			(0x3ff << 0)
#else // dcphy 6410

#define DSIM_PHY_PLL_MFR(_x)			(((_x) & 0xff) << 16)
#define DSIM_PHY_PLL_MFR_MASK			(0xff << 16)

#define DSIM_PHY_PLL_SEL_PF(_x)			(((_x) & 0x3) << 14)
#define DSIM_PHY_PLL_SEL_PF_MASK		(0x3 << 14)

#define DSIM_PHY_PLL_MRR(_x)			(((_x) & 0x3f) << 8)
#define DSIM_PHY_PLL_MRR_MASK			(0x3f << 8)

#define DSIM_PHY_PMS_S(_x)			(((_x) & 0x7) << 4)
#define DSIM_PHY_PMS_S_MASK			(0x7 << 4)
#define DSIM_PHY_PMS_S_GET(_x)			(((_x) >> 4) & 0x7)

#define DSIM_PHY_PLL_SEL_VCO(_x)		(((_x) & 0x1) << 1)
#define DSIM_PHY_PMS_SEL_VCO_MASK		(0x1 << 1)

#define DSIM_PHY_PLL_SSCG_EN(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_PMS_SSCG_EN_MASK		(0x1 << 0)

#endif

#if 0 // dcphy 6310
/* PLL_CON2 - DITHER */
#define DSIM_PHY_DITHER_FOUT_MASK		(0x1 << 13)
#define DSIM_PHY_DITHER_FEED_EN			(0x1 << 12)
#define DSIM_PHY_PMS_M(_x)			(((_x) & 0x3ff) << 0)
#define DSIM_PHY_PMS_M_MASK			(0x3ff << 0)
#define DSIM_PHY_PMS_M_GET(_x)			(((_x) >> 0) & 0x3ff)

/* PLL_CON3 */
#define DSIM_PHY_DITHER_MRR(_x)			(((_x) & 0x3f) << 8)
#define DSIM_PHY_DITHER_MRR_MASK		(0x3f << 8)
#define DSIM_PHY_DITHER_MFR(_x)			(((_x) & 0xff) << 0)
#define DSIM_PHY_DITHER_MFR_MASK		(0xff << 0)

/* PLL_CON4 */
#define DSIM_PHY_DITHER_RSEL(_x)		(((_x) & 0xf) << 12)
#define DSIM_PHY_DITHER_RSEL_MASK		(0xf << 12)
#define DSIM_PHY_DITHER_EN			(0x1 << 11)
#define DSIM_PHY_SSCG_EN			DSIM_PHY_DITHER_EN
#define DSIM_PHY_DITHER_FSEL(_x)		(((_x) & 0x1) << 10)
#define DSIM_PHY_DITHER_FSEL_MASK		(0x1 << 10)
#define DSIM_PHY_DITHER_BYPASS			(0x1 << 9)
#define DSIM_PHY_DITHER_AFC_ENB(_x)		(((_x) & 0x1) << 8)
#define DSIM_PHY_DITHER_AFC_ENB_MASK		(0x1 << 8)
#define DSIM_PHY_DITHER_EXTAFC(_x)		(((_x) & 0x1f) << 0)
#define DSIM_PHY_DITHER_EXTAFC_MASK		(0x1f << 0)

/* PLL_CON5 */
/*TBD: Some bit fields are needs to be added */
#define DSIM_PHY_DITHER_ICP(_x)			(((_x) & 0x3) << 4)
#define DSIM_PHY_DITHER_ICP_MASK		(0x3 << 4)
#define DSIM_PHY_DITHER_SEL_VCO(_x)		((_x) << 2)
#define DSIM_PHY_DITHER_SEL_VCO_MASK		(0x1 << 2)
#define DSIM_PHY_DITHER_SEL_PF(_x)		(((_x) & 0x3) << 0)
#define DSIM_PHY_DITHER_SEL_PF_MASK		(0x3 << 0)

/* PLL_CON6 */

/*TBD: Some bit fields are needs to be added */
/*
 * WCLK_BUF_SFT_CNT = Roundup((Word Clock Period) / 38.46 + 2)
 */
#define DSIM_PHY_WCLK_BUF_SFT_CNT(_x)		(((_x) & 0xf) << 8)
#define DSIM_PHY_WCLK_BUF_SFT_CNT_MASK		(0xf << 8)

/* PLL_CON7 */
#define DSIM_PHY_PLL_LOCK_CNT(_x)		(((_x) & 0xffff) << 0)
#define DSIM_PHY_PLL_LOCK_CNT_MASK		(0xffff << 0)

/* PLL_CON8 */
#define DSIM_PHY_PLL_STB_CNT(x)			((x) << 0)
#define DSIM_PHY_PLL_STB_CNT_MASK		(0xffff << 0)

/* PLL_CON9 */
#define DSIM_PHY_BIAS_CNT_EN(_x)		((_x) << 12)
#define DSIM_PHY_BIAS_CNT_EN_MASK		(0x1 << 12)
#define DSIM_PHY_BIAS_CNT(_x)			((_x) << 0)
#define DSIM_PHY_BIAS_CNT_MASK			(0xfff << 0)

/* PLL_STAT0 */
#define DSIM_PHY_PLL_STAT0			(0x0040)
#define DSIM_PHY_PLL_LOCK_GET(x)		(((x) >> 0) & 0x1)
#endif

#if 0 // dcphy 6310
/* master clock lane General Control Register : GNR */
#define DSIM_PHY_MC_GNR_CON(_id)		(0x0200 + (4 * (_id)))
#define DSIM_PHY_MC_GNR_CON0			(0x0200)
#define DSIM_PHY_MC_GNR_CON1			(0x0204)

/* GNR0 0x0300 */
#define DSIM_PHY_PPI_DATA_WIDTH			(0x1 << 5)
#define DSIM_PHY_FORCE_SFR_PPI_SEL		(0x1 << 4)
#define DSIM_PHY_PHY_READY			(0x1 << 1)
#define DSIM_PHY_PHY_READY_GET(x)		(((x) >> 1) & 0x1)
#define DSIM_PHY_PHY_ENABLE			(0x1 << 0)

/* GNR1 0x0304 */
#define DSIM_PHY_T_PHY_READY(_x)		(((_x) & 0xffff) << 0)
#define DSIM_PHY_T_PHY_READY_MASK		(0xffff << 0)
#else // dcphy 6410

/* Master common register list,
 * OFFSET 0x80
 * Base Addr 0x168E_0080
 */
#define DSIM_PHY_MASTER_CMN_VER			(0x0080)
#define DSIM_PHY_MASTER_CMN_CTRL0		(0x0084)
#define DSIM_PHY_MASTER_CMN_PG0			(0x0088)
#define DSIM_PHY_MASTER_CMN_PG1			(0x008C)
#define DSIM_PHY_MASTER_CMN_PG2			(0x0090)
#define DSIM_PHY_MASTER_CMN_PG3			(0x0094)
#define DSIM_PHY_MASTER_CMN_PG4			(0x0098)
#define DSIM_PHY_MASTER_CMN_PG5			(0x009C)
#define DSIM_PHY_MASTER_CMN_PLL_CTRL(i)		(0x00A0 + ((i) * 4))
#define DSIM_PHY_MASTER_CMN_PLL_CTRL0		(0x00A0)
#define DSIM_PHY_MASTER_CMN_PLL_CTRL1		(0x00A4)
#define DSIM_PHY_MASTER_CMN_PLL_CTRL2		(0x00A8)
#define DSIM_PHY_MASTER_CMN_PLL_CTRL3		(0x00AC)
#define DSIM_PHY_MASTER_CMN_PLL_STAT0		(0x00C0)
#define DSIM_PHY_MASTER_CMN_PLL_DBG0		(0x00E0)
#define DSIM_PHY_MASTER_CMN_PLL_DBG1		(0x00E4)
#define DSIM_PHY_MASTER_CMN_PLL_DBG2		(0x00E8)

/* DSIM_PHY_MASTER_CMN_VER */
#define DSIM_PHY_MASTER_CMN_VER_GET(_x)		(((_x) & 0xffffffff) << 0)

/* DSIM_PHY_MASTER_CMN_CTRL0 */
#define DSIM_PHY_ANA_HSTX_EN_SYNC_SEL(_x)	(((_x) & 0x1) << 14)
#define DSIM_PHY_ANA_HSTX_EN_SYNC_SEL_MASK	(0x1 << 14)

#define DSIM_PHY_PG_STBL_REG_SEL(_x)		(((_x) & 0x1) << 13)
#define DSIM_PHY_PG_STBL_REG_SEL_MASK		(0x1 << 13)

#define DSIM_PHY_USE_SDW_REG(_x)		(((_x) & 0x1) << 12)
#define DSIM_PHY_USE_SDW_REG_MASK		(0x1 << 12)

#define DSIM_PHY_DAT_LANE_ENABLE(_x)		(((_x) & 0xf) << 8)
#define DSIM_PHY_DAT_LANE_ENABLE_MASK(_x)	(0x1 << (8 + (_x)))

#define DSIM_PHY_PLL_ENABLE(_x)			(((_x) & 0x3) << 6)
#define DSIM_PHY_PLL_ENABLE_MASK		(0x3 << 6)

#define DSIM_PHY_ONE_PLL_ENABLE			(0x1 << 6)
#define DSIM_PHY_ONE_PLL_ENABLE_MASK		(0x1 << 6)
#define DSIM_PHY_TWO_PLL_ENABLE			(0x3 << 6)
#define DSIM_PHY_TWO_PLL_ENABLE_MASK		(0x3 << 6)

#define DSIM_PHY_CLK_LANE_ENABLE(_x)		(((_x) & 0x3) << 4)
#define DSIM_PHY_CLK_LANE_ENABLE_MASK		(0x3 << 4)
#define DSIM_PHY_CLK0_LANE_ENABLE_MASK		(0x1 << 4)

#define DSIM_PHY_PPI_DATA_WIDTH(_x)		(((_x) & 0x3) << 2)
#define DSIM_PHY_PPI_DATA_WIDTH_MASK		(0x3 << 2)

#define DSIM_PHY_CLK_LANE_CFG_SEL(_x)		(((_x) & 0x1) << 1)
#define DSIM_PHY_CLK_LANE_CFG_SEL_MASK		(0x1 << 1)

#define DSIM_PHY_PHY_SEL(_x)			(((_x) & 0x1) << 0)
#define DSIM_PHY_PHY_SEL_MASK			(0x1 << 0)

/* DSIM_PHY_MASTER_CMN_PG0 */
/* DSIM_PHY_MASTER_CMN_PG1 */
/* DSIM_PHY_MASTER_CMN_PG2 */
/* DSIM_PHY_MASTER_CMN_PG3 */
/* DSIM_PHY_MASTER_CMN_PG4 */
/* DSIM_PHY_MASTER_CMN_PG5 */


/* DSIM_PHY_MASTER_CMN_PLL_CTRL0 */
#define DSIM_PHY_DDR_GATING_DISABLE(_x)		(((_x) & 0x1) << 3)
#define DSIM_PHY_DDR_GATING_DISABLE_MASK	(0x1 << 3)

#define DSIM_PHY_CLK_GATING_DISABLE(_x)		(((_x) & 0x1) << 2)
#define DSIM_PHY_CLK_GATING_DISABLE_MASK	(0x1 << 2)

#define DSIM_PHY_PLL_LOCK_SEL(_x)		(((_x) & 0x1) << 1)
#define DSIM_PHY_PLL_LOCK_SEL_MASK		(0x1 << 1)

#define DSIM_PHY_PLL_WIDE_HOP(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_PLL_WIDE_HOP_MASK		(0x1 << 0)

/* DSIM_PHY_MASTER_CMN_PLL_CTRL1 */
#define DSIM_PHY_PLL_EN_CNT(_x)			(((_x) & 0xffff) << 16)
#define DSIM_PHY_PLL_EN_CNT_MASK		(0xffff << 16)

#define DSIM_PHY_PLL_LOCK_CNT(_x)		(((_x) & 0xffff) << 0)
#define DSIM_PHY_PLL_LOCK_CNT_MASK		(0xffff << 0)

/* DSIM_PHY_MASTER_CMN_PLL_CTRL2 */
#define DSIM_PHY_WCLK_BUF_SFT_CNT(_x)		(((_x) & 0xf) << 16)
#define DSIM_PHY_WCLK_BUF_SFT_CNT_MASK		(0xf << 16)

#define DSIM_PHY_PLL_STB_CNT(_x)		(((_x) & 0xffff) << 0)
#define DSIM_PHY_PLL_STB_CNT_MASK		(0xffff << 0)

/* DSIM_PHY_MASTER_CMN_PLL_CTRL3 */
#define DSIM_PHY_BIAS_WAKEUP_CNT(_x)		(((_x) & 0xffff) << 16)
#define DSIM_PHY_BIAS_WAKEUP_CNT_MASK		(0xffff << 16)

#define DSIM_PHY_BIAS_STB_CNT(_x)		(((_x) & 0xfff) << 0)
#define DSIM_PHY_BIAS_STB_CNT_MASK		(0xfff << 0)

/* DSIM_PHY_MASTER_CMN_PLL_STAT0 */
#define DSIM_PHY_STATE_PLL_LOCK_GET(x)		(((x) >> 2) & 0x1)

/* DSIM_PHY_MASTER_CMN_PLL_DBG0 */
/* DSIM_PHY_MASTER_CMN_PLL_DBG1 */
/* DSIM_PHY_MASTER_CMN_PLL_DBG2 */
#endif

#if 0 // dcphy 6310
/* master clock lane Analog Block Control Register : ANA */
#define DSIM_PHY_MC_ANA_CON(_id)		(0x0208 + (4 * (_id)))
#define DSIM_PHY_MC_ANA_CON0			(0x0208)
#define DSIM_PHY_MC_ANA_CON1			(0x020C)
#define DSIM_PHY_MC_ANA_CON2			(0x0210)
#define DSIM_PHY_MC_ANA_CON3			(0x0214)

/* ANA_CON0 0x0308 */
/*TBD: Some bit fields are needs to be added */
#define DSIM_PHY_EDGE_CON(_x)			(((_x) & 0x7) << 12)
#define DSIM_PHY_EDGE_CON_MASK			(0x7 << 12)
#define DSIM_PHY_EDGE_CON_DIR			(0x1 << 9)
#define DSIM_PHY_EDGE_CON_EN			(0x1 << 8)
#define DSIM_PHY_RES_UP(_x)			(((_x) & 0xf) << 4)
#define DSIM_PHY_RES_UP_MASK			(0xf << 4)
#define DSIM_PHY_RES_DN(_x)			(((_x) & 0xf) << 0)
#define DSIM_PHY_RES_DN_MASK			(0xf << 0)

/* ANA_CON1 0x030C */
/*TBD: Some bit fields are needs to be added */
#define DSIM_PHY_DPDN_SWAP(_x)			(((_x) & 0x1) << 12)
#define DSIM_PHY_DPDN_SWAP_MASK			(0x1 << 12)
#define DSIM_PHY_BIAS_SLEEP_EN(_x)		(((_x) & 0x1) << 2)
#define DSIM_PHY_BIAS_SLEEP_EN_MASK		(0x1 << 2)

/* ANA_CON2 0x0310 */
#define DSIM_PHY_UPI_MODE_ENB(_x)		(((_x) & 0x1) << 4)
#define DSIM_PHY_UPI_MODE_ENB_MASK		(0x1 << 4)
#define DSIM_PHY_BLEED_EN_EN(_x)		(((_x) & 0x1) << 2)
#define DSIM_PHY_BLEED_EN_MASK			(0x1 << 2)
#define DSIM_PHY_HS_VREG_AMP_ICON(_x)		(((_x) & 0x3) << 0)
#define DSIM_PHY_HS_VREG_AMP_ICON_MASK		(0x3 << 0)

/* ANA_CON3 : 0x0314 */
#define DSIM_PHY_EN_HIZ(_x)			(((_x) & 0x1) << 15)
#define DSIM_PHY_EN_HIZ_MASK			(0x1 << 15)

/* master clock lane setting */
#define DSIM_PHY_MC_TIME_CON0			(0x0230)
#define DSIM_PHY_MC_TIME_CON1			(0x0234)
#define DSIM_PHY_MC_TIME_CON2			(0x0238)
#define DSIM_PHY_MC_TIME_CON3			(0x023C)
#define DSIM_PHY_MC_TIME_CON4			(0x0240)
#define DSIM_PHY_MC_DATA_CON0			(0x0244)
#define DSIM_PHY_MC_DESKEW_CON0			(0x0250)

/* TIME_CON0 */

/* TIME_CON1 */

/* TIME_CON2 */

/* TIME_CON3 */

/* TIME_CON4 */

/* DAT_CON0 */

/* DESKEW_CON0 */
/* MC_DESKEW_CON0 */
#define DSIM_PHY_SKEWCAL_RUN_TIME(_x)		(((_x) & 0xf) << 12)
#define DSIM_PHY_SKEWCAL_RUN_TIME_MASK		(0xf << 12)
#define DSIM_PHY_SKEWCAL_INIT_RUN_TIME(_x)	(((_x) & 0xf) << 8)
#define DSIM_PHY_SKEWCAL_INIT_RUN_TIME_MASK	(0xf << 8)
#define DSIM_PHY_SKEWCAL_INIT_WAIT_TIME(_x)	(((_x) & 0xf) << 4)
#define DSIM_PHY_SKEWCAL_INIT_WAIT_TIME_MASK	(0xf << 4)
#define DSIM_PHY_SKEWCAL_EN			(0x1 << 0)

#define DSIM_PHY_MC_TEST_CON0			(0x0270)
#define DSIM_PHY_MC_TEST_CON1			(0x0274)
#define DSIM_PHY_MC_TEST_CON2			(0x027C)
#define DSIM_PHY_MC_TEST_CON3			(0x0284)
#define DSIM_PHY_MC_TEST_CON4			(0x0288)

/*Bit Filed details for the above */
#define DSIM_PHY_MC_BIST_CON0			(0x0290)
#define DSIM_PHY_MC_BIST_CON1			(0x0294)

/*Bit Filed details for the above */
#define DSIM_PHY_MC_DBG_STAT0			(0x02E0)
#define DSIM_PHY_MC_DBG_STAT1			(0x02E4)

/*Bit Filed details for the above */
#define DSIM_PHY_MC_PPI_STAT0			(0x02E8)
#define DSIM_PHY_MC_ADI_STAT1			(0x02EC)

/*Bit Filed details for the above */
#else // dcphy 6410

/* Master Clock configurations,
 * OFFSET 0x200
 * Base Addr 0x168E_0200
 */
#define DSIM_PHY_MC_ANA_CON(_id)		(0x0000 + (4 * (_id)))
#define DSIM_PHY_MC_ANA_CON0			(0x0000)
#define DSIM_PHY_MC_ANA_CON1			(0x0004)
#define DSIM_PHY_MC_ANA_SDW_CON0		(0x0020)
#define DSIM_PHY_MC_SERDES_CTRL_DPHY0		(0x0050)
#define DSIM_PHY_MC_DEC_CTRL_DCPHY0		(0x0058)
#define DSIM_PHY_MC_BIST_CTRL0			(0x0060)
#define DSIM_PHY_MC_BIST_CTRL1			(0x0064)
#define DSIM_PHY_MC_BIST_CTRL2			(0x0070)
#define DSIM_PHY_MC_BIST_CTRL3			(0x0074)
#define DSIM_PHY_MC_BIST_STATUS			(0x0078)
#define DSIM_PHY_MC_CLK_CTRL0			(0x0080)
#define DSIM_PHY_MC_CLK_CTRL1			(0x0084)
#define DSIM_PHY_MC_CLK_CTRL2			(0x0088)
#define DSIM_PHY_MC_CLK_CTRL3			(0x008C)
#define DSIM_PHY_MC_CLK_CTRL4			(0x0090)
#define DSIM_PHY_MC_CLK_CTRL5			(0x0094)
#define DSIM_PHY_MC_CLK_SKEW0			(0x00A0)
#define DSIM_PHY_MC_CLK_SKEW1			(0x00A4)
#define DSIM_PHY_MC_CLK_STAT0			(0x00C0)
#define DSIM_PHY_MC_CLK_STAT1			(0x00C4)
#define DSIM_PHY_MC_CLK_STAT2			(0x00C8)
#define DSIM_PHY_MC_CLK_STAT3			(0x00CC)
#define DSIM_PHY_MC_CLK_DBG0			(0x00E0)
#define DSIM_PHY_MC_CLK_DBG1			(0x00E4)
#define DSIM_PHY_MC_CLK_DBG2			(0x00E8)
#define DSIM_PHY_MC_CLK_TEST0			(0x00F0)
#define DSIM_PHY_MC_CLK_TEST1			(0x00F4)
#define DSIM_PHY_MC_CLK_TEST2			(0x00F8)

/* DSIM_PHY_MC_ANA_CON0 */
#define DSIM_PHY_MC_LPTX_EN_HIZ(_x)		(((_x) & 0x1) << 28)
#define DSIM_PHY_MC_LPTX_EN_HIZ_MASK		(0x1 << 28)

#define DSIM_PHY_MC_LPTX_CON_IMP(_x)		(((_x) & 0x3) << 24)
#define DSIM_PHY_MC_LPTX_CON_IMP_MASK		(0x3 << 24)

#define DSIM_PHY_MC_HSTX_EN_EDGE_CTRL(_x)	(((_x) & 0x1) << 20)
#define DSIM_PHY_MC_HSTX_EN_EDGE_CTRL_MASK	(0x1 << 20)

#define DSIM_PHY_MC_HSTX_CON_EDGE_DIR(_x)	(((_x) & 0x1) << 19)
#define DSIM_PHY_MC_HSTX_CON_EDGE_DIR_MASK	(0x1 << 19)

#define DSIM_PHY_MC_HSTX_CON_EDGE_RATE(_x)	(((_x) & 0x7) << 16)
#define DSIM_PHY_MC_HSTX_CON_EDGE_RATE_MASK	(0x7 << 16)

#define DSIM_PHY_MC_HSTX_CON_INPUT_SWAP(_x)	(((_x) & 0x1) << 9)
#define DSIM_PHY_MC_HSTX_CON_INPUT_SWAP_MASK	(0x1 << 9)

#define DSIM_PHY_MC_HSTX_CON_INPUT_SWAP_DPHY(_x)	(((_x) & 0x1) << 8)
#define DSIM_PHY_MC_HSTX_CON_INPUT_SWAP_DPHY_MASK	(0x1 << 8)

#define DSIM_PHY_MC_HSTX_CON_RES_UP(_x)		(((_x) & 0xf) << 4)
#define DSIM_PHY_HSTX_CON_RES_UP_MASK		(0xf << 4)

#define DSIM_PHY_MC_HSTX_CON_RES_DN(_x)		(((_x) & 0xf) << 0)
#define DSIM_PHY_MC_HSTX_CON_RES_DN_MASK	(0xf << 0)

/* DSIM_PHY_MC_ANA_CON1 */
#define DSIM_PHY_MC_VREG_EN_TUNE_CPHY(_x)	(((_x) & 0x1) << 18)
#define DSIM_PHY_MC_VREG_EN_TUNE_CPHY_MASK	(0x1 << 18)

#define DSIM_PHY_MC_VREG_CTRL_VREF_CPHY(_x)	(((_x) & 0x3) << 16)
#define DSIM_PHY_MC_VREG_CTRL_VREF_CPHY_MASK	(0x3 << 16)

#define DSIM_PHY_MC_VREG_EN_AMP_ICON(_x)	(((_x) & 0x1) << 14)
#define DSIM_PHY_MC_VREG_EN_AMP_ICON_MASK	(0x1 << 14)

#define DSIM_PHY_MC_VREG_AMP_ICON(_x)		(((_x) & 0x3) << 12)
#define DSIM_PHY_MC_VREG_AMP_ICON_MASK		(0x3 << 12)

#define DSIM_PHY_MC_VREG_EN_BLEED_SEL(_x)	(((_x) & 0x1) << 10)
#define DSIM_PHY_MC_VREG_EN_BLEED_SEL_MASK	(0x1 << 10)

#define DSIM_PHY_MC_VREG_EN_BLEED(_x)		(((_x) & 0x1) << 9)
#define DSIM_PHY_MC_VREG_EN_BLEED_MASK		(0x1 << 9)

#define DSIM_PHY_MC_VREG_CTRL_BLEED(_x)		(((_x) & 0x1) << 8)
#define DSIM_PHY_MC_VREG_CTRL_BLEED_MASK	(0x1 << 8)

#define DSIM_PHY_MC_VREG_SEL_EN(_x)		(((_x) & 0x1) << 1)
#define DSIM_PHY_MC_VREG_SEL_EN_MASK		(0x1 << 1)

#define DSIM_PHY_MC_VREG_EN(_x)			(((_x) & 0x1) << 0)
#define DSIM_PHY_MC_VREG_L_EN_MASK			(0x1 << 0)

/* DSIM_PHY_MC_ANA_SDW_CON0 */
#define DSIM_PHY_MC_HSTX_CON_EMP(_x)		(((_x) & 0x3) << 0)
#define DSIM_PHY_MC_HSTX_CON_EMP_MASK		(0x3 << 0)

/* DSIM_PHY_MC_SERDES_CTRL_DPHY0 */
#define DSIM_PHY_MC_DES_DATA_WIDTH_DPHY(_x)	(((_x) & 0x3) << 17)
#define DSIM_PHY_MC_DES_DATA_WIDTH_DPHY_MASK	(0x3 << 17)

#define DSIM_PHY_MC_DES_EN_DPHY(_x)		(((_x) & 0x1) << 16)
#define DSIM_PHY_MC_DES_EN_DPHY_MASK		(0x1 << 16)

#define DSIM_PHY_MC_DPHY_CLOCK_MODE_EN(_x)	(((_x) & 0x1) << 3)
#define DSIM_PHY_MC_HSTX_CLOCK_MODE_EN_MASK	(0x1 << 3)

#define DSIM_PHY_MC_SER_DATA_WIDTH_DPHY(_x)	(((_x) & 0x3) << 1)
#define DSIM_PHY_MC_SER_DATA_WIDTH_DPHY_MASK	(0x3 << 1)

#define DSIM_PHY_MC_SER_EN_DPHY(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MC_SER_EN_DPHY_MASK		(0x1 << 0)

/* DSIM_PHY_MC_DEC_CTRL_DCPHY0 */
#define DSIM_PHY_MC_MON_DEC_ERR_CNT(_x)		(((_x) & 0xff) << 8)
#define DSIM_PHY_MC_MON_DEC_ERR_CNT_MASK	(0xff << 8)

#define DSIM_PHY_MC_MON_DEC_ERR(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MC_MON_DEC_ERR_MASK		(0x1 << 0)

/* DSIM_PHY_MC_BIST_CTRL0 */
/* DSIM_PHY_MC_BIST_CTRL1 */
/* DSIM_PHY_MC_BIST_CTRL2 */
/* DSIM_PHY_MC_BIST_CTRL3 */
/* DSIM_PHY_MC_BIST_STATUS */

/* DSIM_PHY_MC_CLK_CTRL0 */
#define DSIM_PHY_MC_T_PHY_READY(_x)		(((_x) & 0xffff) << 16)
#define DSIM_PHY_MC_T_PHY_READY_MASK		(0xffff << 16)

#define DSIM_PHY_MC_PHY_READY(_x)		(((_x) & 0x1) << 15) //read only
#define DSIM_PHY_MC_PHY_READY_MASK		(0x1 << 15)
#define DSIM_PHY_MC_PHY_READY_GET(x)		(((x) >> 15) & 0x1)

#define DSIM_PHY_MC_ZERO_SKIP_SC_INIT_WAIT(_x)	(((_x) & 0x1) << 9)
#define DSIM_PHY_MC_ZERO_SKIP_SC_INIT_WAIT_MASK	(0x1 << 9)

#define DSIM_PHY_MC_TX_REQUEST_HS_SEL(_x)	(((_x) & 0x1) << 8)
#define DSIM_PHY_MC_TX_REQUEST_HS_SEL_MASK	(0x1 << 8)

#define DSIM_PHY_MC_CLK_INV(_x)			(((_x) & 0x1) << 4)
#define DSIM_PHY_MC_CLK_INV_MASK		(0x1 << 4)

#define DSIM_PHY_MC_LPTX_SWAP(_x)		(((_x) & 0x1) << 2)
#define DSIM_PHY_MC_LPTX_SWAP_MASK		(0x1 << 2)

/* DSIM_PHY_MC_CLK_CTRL1 */
#define DSIM_PHY_MC_T_ULPS_EXIT(_x)		(((_x) & 0x3ff) << 0)
#define DSIM_PHY_MC_T_ULPS_EXIT_MASK		(0x3ff << 0)

/* DSIM_PHY_MC_CLK_CTRL2 */
#define DSIM_PHY_MC_T_HS_EXIT(_x)		(((_x) & 0xff) << 24)
#define DSIM_PHY_MC_T_HS_EXIT_MASK		(0xff << 24)

#define DSIM_PHY_MC_T_CLK_ZERO(_x)		(((_x) & 0xff) << 16)
#define DSIM_PHY_MC_T_CLK_ZERO_MASK		(0xff << 16)

#define DSIM_PHY_MC_T_CLK_PREPARE(_x)		(((_x) & 0xff) << 8)
#define DSIM_PHY_MC_T_CLK_PREPARE_MASK		(0xff << 8)

#define DSIM_PHY_MC_T_LPX(_x)			(((_x) & 0xff) << 0)
#define DSIM_PHY_MC_T_LPX_MASK			(0xff << 0)

/* DSIM_PHY_MC_CLK_CTRL3 */
#define DSIM_PHY_MC_T_CLK_TRAIL(_x)		(((_x) & 0xff) << 8)
#define DSIM_PHY_MC_T_CLK_TRAIL_MASK		(0xff << 8)

#define DSIM_PHY_MC_T_CLK_POST(_x)		(((_x) & 0xff) << 0)
#define DSIM_PHY_MC_T_CLK_POST_MASK		(0xff << 0)

/* DSIM_PHY_MC_CLK_CTRL4 */
#define DSIM_PHY_MC_T_HS_EXIT_SDW(_x)		(((_x) & 0xff) << 24)
#define DSIM_PHY_MC_T_HS_EXIT_SDW_MASK		(0xff << 24)

#define DSIM_PHY_MC_T_CLK_ZERO_SDW(_x)		(((_x) & 0xff) << 16)
#define DSIM_PHY_MC_T_CLK_ZERO_SDW_MASK		(0xff << 16)

#define DSIM_PHY_MC_T_CLK_PREPARE_SDW(_x)	(((_x) & 0xff) << 8)
#define DSIM_PHY_MC_T_CLK_PREPARE_SDW_MASK	(0xff << 8)

#define DSIM_PHY_MC_T_LPX_SDW(_x)		(((_x) & 0xff) << 0)
#define DSIM_PHY_MC_T_LPX_SDW_MASK		(0xff << 0)

/* DSIM_PHY_MC_CLK_CTRL5 */
#define DSIM_PHY_MC_T_CLK_TRAIL_SDW(_x)		(((_x) & 0xff) << 8)
#define DSIM_PHY_MC_T_CLK_TRAIL_SDW_MASK	(0xff << 8)

#define DSIM_PHY_MC_T_CLK_POST_SDW(_x)		(((_x) & 0xff) << 0)
#define DSIM_PHY_MC_T_CLK_POST_SDW_MASK		(0xff << 0)

/* DSIM_PHY_MC_CLK_SKEW0 */
#define DSIM_PHY_MC_SKEW_CAL_RUN_TIME(_x)	(((_x) & 0xf) << 20)
#define DSIM_PHY_MC_SKEW_CAL_RUN_TIME_MASK	(0xf << 20)

#define DSIM_PHY_MC_SKEW_CAL_INIT_WAIT_TIME(_x)		(((_x) & 0xf) << 12)
#define DSIM_PHY_MC_SKEW_CAL_INIT_WAIT_TIME_MASK	(0xf << 12)

#define DSIM_PHY_MC_SKEW_CAL_INIT_RUN_TIME(_x)	(((_x) & 0xf) << 4)
#define DSIM_PHY_MC_SKEW_CAL_INIT_RUN_TIME_MASK	(0xf << 4)

#define DSIM_PHY_MC_INIT_SKEW_CAL_CHECK_EN(_x)	(((_x) & 0x1) << 2)
#define DSIM_PHY_MC_INIT_SKEW_CAL_CHECK_EN_MASK	(0x1 << 2)

#define DSIM_PHY_MC_SKEW_CAL_EN(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MC_SKEW_CAL_EN_MASK		(0x1 << 0)

/* DSIM_PHY_MC_CLK_SKEW1 */
#define DSIM_PHY_MC_SKEW_CAL_RUN_TIME_SDW(_x)	(((_x) & 0xf) << 20)
#define DSIM_PHY_MC_SKEW_CAL_RUN_TIME_SDW_MASK	(0xf << 20)

#define DSIM_PHY_MC_SKEW_CAL_INIT_WAIT_TIME_SDW(_x)	(((_x) & 0xf) << 12)
#define DSIM_PHY_MC_SKEW_CAL_INIT_WAIT_TIME_SDW_MASK	(0xf << 12)

#define DSIM_PHY_MC_SKEW_CAL_INIT_RUN_TIME_SDW(_x)	(((_x) & 0xf) << 4)
#define DSIM_PHY_MC_SKEW_CAL_INIT_RUN_TIME_SDW_MASK	(0xf << 4)

#define DSIM_PHY_MC_INIT_SKEW_CAL_CHECK_EN_SDW(_x)	(((_x) & 0x1) << 2)
#define DSIM_PHY_MC_INIT_SKEW_CAL_CHECK_EN_SDW_MASK	(0x1 << 2)

#define DSIM_PHY_MC_SKEW_CAL_EN_SDW(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MC_SKEW_CAL_EN_SDW_MASK	(0x1 << 0)

/* DSIM_PHY_MC_CLK_STAT0 */
/* DSIM_PHY_MC_CLK_STAT1 */
/* DSIM_PHY_MC_CLK_STAT2 */
/* DSIM_PHY_MC_CLK_STAT3 */
/* DSIM_PHY_MC_CLK_DBG0 */
/* DSIM_PHY_MC_CLK_DBG1 */
/* DSIM_PHY_MC_CLK_DBG2 */
/* DSIM_PHY_MC_CLK_TEST0 */
/* DSIM_PHY_MC_CLK_TEST1 */
/* DSIM_PHY_MC_CLK_TEST2 */

#endif



/*
 * master data lane setting : D0 ~ D3
 * D0~D2 : COMBO
 * D3    : DPHY
 */
#if 0 // dcphy 6310
#define DSIM_PHY_MD_GNR_CON0(_x)		(0x0300 + (0x100 * (_x)))
#define DSIM_PHY_MD_GNR_CON1(_x)		(0x0304 + (0x100 * (_x)))
#define DSIM_PHY_MD_ANA_CON0(_x)		(0x0308 + (0x100 * (_x)))
#define DSIM_PHY_MD_ANA_CON1(_x)		(0x030C + (0x100 * (_x)))
#define DSIM_PHY_MD_ANA_CON2(_x)		(0x0310 + (0x100 * (_x)))
#define DSIM_PHY_MD_ANA_CON3(_x)		(0x0314 + (0x100 * (_x)))

#define DSIM_PHY_MD_TIME_CON0(_x)		(0x0330 + (0x100 * (_x)))
#define DSIM_PHY_MD_TIME_CON1(_x)		(0x0334 + (0x100 * (_x)))
#define DSIM_PHY_MD_TIME_CON2(_x)		(0x0338 + (0x100 * (_x)))
#define DSIM_PHY_MD_TIME_CON3(_x)		(0x033C + (0x100 * (_x)))
#define DSIM_PHY_MD_TIME_CON4(_x)		(0x0340 + (0x100 * (_x)))
#define DSIM_PHY_MD_DATA_CON0(_x)		(0x0344 + (0x100 * (_x)))

/* master data lane(COMBO) setting : D0 */
#define DSIM_PHY_MD0_TIME_CON0			(0x0330)
#define DSIM_PHY_MD0_TIME_CON1			(0x0334)
#define DSIM_PHY_MD0_TIME_CON2			(0x0338)
#define DSIM_PHY_MD0_TIME_CON3			(0x033C)
#define DSIM_PHY_MD0_TIME_CON4			(0x0340)
#define DSIM_PHY_MD0_DATA_CON0			(0x0344)

/* TBD: Add 0x0360 to 0x03f4*/

/* master data lane(COMBO) setting : D1 */
#define DSIM_PHY_MD1_TIME_CON0			(0x0430)
#define DSIM_PHY_MD1_TIME_CON1			(0x0434)
#define DSIM_PHY_MD1_TIME_CON2			(0x0438)
#define DSIM_PHY_MD1_TIME_CON3			(0x043C)
#define DSIM_PHY_MD1_TIME_CON4			(0x0440)
#define DSIM_PHY_MD1_DATA_CON0			(0x0444)

/* TBD: Add 0x0460 to 0x04f4*/

/* master data lane(COMBO) setting : D2 */
#define DSIM_PHY_MD2_TIME_CON0			(0x0530)
#define DSIM_PHY_MD2_TIME_CON1			(0x0534)
#define DSIM_PHY_MD2_TIME_CON2			(0x0538)
#define DSIM_PHY_MD2_TIME_CON3			(0x053C)
#define DSIM_PHY_MD2_TIME_CON4			(0x0540)
#define DSIM_PHY_MD2_DATA_CON0			(0x0544)

/* TBD: Add 0x0560 to 0x05f4*/

/* master data lane setting : D3 */
#define DSIM_PHY_MD3_TIME_CON0			(0x0630)
#define DSIM_PHY_MD3_TIME_CON1			(0x0634)
#define DSIM_PHY_MD3_TIME_CON2			(0x0638)
#define DSIM_PHY_MD3_TIME_CON3			(0x063C)
#define DSIM_PHY_MD3_TIME_CON4			(0x0640)
#define DSIM_PHY_MD3_DATA_CON0			(0x0644)

/* TBD: Add 0x0660 to 0x06f4*/
#else // dcphy 6410

#define DSIM_PHY_MD_ANA_CON0(_x)		(0x0200 + (0x100 * (_x)))
#define DSIM_PHY_MD_ANA_CON1(_x)		(0x0204 + (0x100 * (_x))) //D3 is NA
#define DSIM_PHY_MD_ANA_SDW_CON0(_x)		(0x0220 + (0x100 * (_x)))

#define DSIM_PHY_MD_SERDES_CTRL_DPHY0(_x)	(0x0250 + (0x100 * (_x)))
#define DSIM_PHY_MD_SERDES_CTRL_CPHY0(_x)	(0x0254 + (0x100 * (_x))) //D3 is NA
#define DSIM_PHY_MD_DEC_CTRL_DCPHY0(_x)		(0x0258 + (0x100 * (_x)))

#define DSIM_PHY_MD_BIST_CTRL0(_x)		(0x0260 + (0x100 * (_x)))
#define DSIM_PHY_MD_BIST_CTRL1(_x)		(0x0264 + (0x100 * (_x)))
#define DSIM_PHY_MD_BIST_CTRL2(_x)		(0x0270 + (0x100 * (_x)))
#define DSIM_PHY_MD_BIST_CTRL3(_x)		(0x0274 + (0x100 * (_x)))
#define DSIM_PHY_MD_BIST_STATUS(_x)		(0x0278 + (0x100 * (_x)))

#define DSIM_PHY_MD_DAT_CTRL0(_x)		(0x0280 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_CTRL1(_x)		(0x0284 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_CTRL2(_x)		(0x0288 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_CTRL3(_x)		(0x028C + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_CTRL4(_x)		(0x0290 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_CTRL5(_x)		(0x0294 + (0x100 * (_x)))

#define DSIM_PHY_MD_DAT_SKEW0(_x)		(0x02A0 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_SKEW1(_x)		(0x02A4 + (0x100 * (_x)))

#define DSIM_PHY_MD_DAT_PROG0(_x)		(0x02B0 + (0x100 * (_x))) //D3 is NA
#define DSIM_PHY_MD_DAT_PROG1(_x)		(0x02B4 + (0x100 * (_x))) //D3 is NA

#define DSIM_PHY_MD_DAT_STAT0(_x)		(0x02C0 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_STAT1(_x)		(0x02C4 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_STAT2(_x)		(0x02C8 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_STAT3(_x)		(0x02CC + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_STAT4(_x)		(0x02D0 + (0x100 * (_x)))//D3 is NA
#define DSIM_PHY_MD_DAT_STAT5(_x)		(0x02D4 + (0x100 * (_x)))//D3 is NA
#define DSIM_PHY_MD_DAT_STAT6(_x)		(0x02D8 + (0x100 * (_x)))//D3 is NA
#define DSIM_PHY_MD_DAT_STAT7(_x)		(0x02DC + (0x100 * (_x)))//D3 is NA

#define DSIM_PHY_MD_DAT_DBG0(_x)		(0x02E0 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_DBG1(_x)		(0x02E4 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_DBG2(_x)		(0x02E8 + (0x100 * (_x)))//D3 is NA
#define DSIM_PHY_MD_DAT_DBG3(_x)		(0x02EC + (0x100 * (_x)))

#define DSIM_PHY_MD_DAT_TEST0(_x)		(0x02F0 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_TEST1(_x)		(0x02F4 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_TEST2(_x)		(0x02F8 + (0x100 * (_x)))
#define DSIM_PHY_MD_DAT_TEST3(_x)		(0x02FC + (0x100 * (_x)))

/* DSIM_PHY_MD_ANA_CON0
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_LPTX_EN_HIZ(_x)		(((_x) & 0x1) << 28)
#define DSIM_PHY_MD_LPTX_EN_HIZ_MASK		(0x1 << 28)

#define DSIM_PHY_MD_LPTX_CON_IMP(_x)		(((_x) & 0x3) << 24)
#define DSIM_PHY_MD_LPTX_CON_IMP_MASK		(0x3 << 24)

#define DSIM_PHY_MD_HSTX_EN_EDGE_CTRL(_x)	(((_x) & 0x1) << 20)
#define DSIM_PHY_MD_HSTX_EN_EDGE_CTRL_MASK	(0x1 << 20)

#define DSIM_PHY_MD_HSTX_CON_EDGE_DIR(_x)	(((_x) & 0x1) << 19)
#define DSIM_PHY_MD_HSTX_CON_EDGE_DIR_MASK	(0x1 << 19)

#define DSIM_PHY_MD_HSTX_CON_EDGE_RATE(_x)	(((_x) & 0x7) << 16)
#define DSIM_PHY_MD_HSTX_CON_EDGE_RATE_MASK	(0x7 << 16)

#define DSIM_PHY_MD_HSTX_CON_INPUT_SWAP(_x)	(((_x) & 0x1) << 9)
#define DSIM_PHY_MD_HSTX_CON_INPUT_SWAP_MASK	(0x1 << 9)

#define DSIM_PHY_MD_HSTX_CON_INPUT_SWAP_DPHY(_x)	(((_x) & 0x1) << 8)
#define DSIM_PHY_MD_HSTX_CON_INPUT_SWAP_DPHY_MASK	(0x1 << 8)

#define DSIM_PHY_MD_HSTX_CON_RES_UP(_x)		(((_x) & 0xf) << 4)
#define DSIM_PHY_MD_HSTX_CON_RES_UP_MASK	(0xf << 4)

#define DSIM_PHY_MD_HSTX_CON_RES_DN(_x)		(((_x) & 0xf) << 0)
#define DSIM_PHY_MD_HSTX_CON_RES_DN_MASK	(0xf << 0)

/* DSIM_PHY_MD_ANA_CON1 */
/* Valid for MD0/MD1/MD2, but MD3 NA */
#define DSIM_PHY_MD_MRX_CON_DTBMUX(_x)		(((_x) & 0x7) << 20)
#define DSIM_PHY_MD_HRX_CON_DTBMUX_MASK		(0x7 << 20)

#define DSIM_PHY_MD_MRX_CON_IN_SWAP(_x)		(((_x) & 0x1) << 17)
#define DSIM_PHY_MD_MRX_CON_IN_SWAP_MASK	(0x1 << 17)

#define DSIM_PHY_MD_MRX_CON_RSTN_SRC(_x)	(((_x) & 0x1) << 16)
#define DSIM_PHY_MD_MRX_CON_RSTN_SRC_MASK	(0x1 << 16)

#define DSIM_PHY_MD_LPRX_CON_BIAS(_x)		(((_x) & 0x3) << 14)
#define DSIM_PHY_MD_LPRX_CON_BIAS_MASK		(0x3 << 14)

#define DSIM_PHY_MD_LPRX_CON_ESCCLK_POL(_x)	(((_x) & 0x1) << 13)
#define DSIM_PHY_MD_LPRX_CON_ESCCLK_POL_MASK	(0x1 << 13)

#define DSIM_PHY_MD_LPRX_CON_HYS(_x)		(((_x) & 0x3) << 11)
#define DSIM_PHY_MD_LPRX_CON_HYS_MASK		(0x3 << 11)

#define DSIM_PHY_MD_LPRX_CON_PR_PATH(_x)	(((_x) & 0x1) << 10)
#define DSIM_PHY_MD_LPRX_CON_PR_PATH_MASK	(0x1 << 10)

#define DSIM_PHY_MD_LPRX_CON_PR_VER(_x)		(((_x) & 0x1) << 9)
#define DSIM_PHY_MD_LPRX_CON_PR_VER_MASK	(0x1 << 9)

#define DSIM_PHY_MD_LPRX_CON_PR_W_ORG(_x)	(((_x) & 0x1) << 8)
#define DSIM_PHY_MD_LPRX_CON_PR_W_ORG_MASK	(0x1 << 8)

#define DSIM_PHY_MD_LPCD_CON_BIAS(_x)		(((_x) & 0x3) << 2)
#define DSIM_PHY_MD_LPCD_CON_BIAS_MASK		(0x3 << 2)

#define DSIM_PHY_MD_LPCD_CON_HYS(_x)		(((_x) & 0x1) << 1)
#define DSIM_PHY_MD_LPCD_CON_HYS_MASK		(0x1 << 1)

#define DSIM_PHY_MD_LPCD_CON_PR_PATH(_x)	(((_x) & 0x1) << 0)
#define DSIM_PHY_MD_LPCD_CON_PR_PATH_MASK	(0x1 << 0)

/* DSIM_PHY_MD_ANA_SDW_CON0 */
/* Valid for MD0/MD1/MD2/MD3 */
#define DSIM_PHY_MD_HSTX_CON_EMP(_x)		(((_x) & 0x3) << 0)
#define DSIM_PHY_MD_HSTX_CON_EMP_MASK		(0x3 << 0)

/* DSIM_PHY_MD_SERDES_CTRL_DPHY0 */
/* Valid for MD0/MD1/MD2/MD3 */
#define DSIM_PHY_MD_DES_DATA_WIDTH_DPHY0(_x)	(((_x) & 0x3) << 17)
#define DSIM_PHY_MD_DES_DATA_WIDTH_DPHY0_MASK	(0x3 << 17)

#define DSIM_PHY_MD_DES_EN_DPHY(_x)		(((_x) & 0x1) << 16)
#define DSIM_PHY_MD_DES_EN_DPHY_MASK		(0x1 << 16)

#define DSIM_PHY_MD_DPHY_CLOCK_MODE_EN(_x)	(((_x) & 0x1) << 3)
#define DSIM_PHY_MD_DPHY_CLOCK_MODE_EN_MASK	(0x1 << 3)

#define DSIM_PHY_MD_SER_DATA_WIDTH_DPHY(_x)	(((_x) & 0x3) << 1)
#define DSIM_PHY_MD_SER_DATA_WIDTH_DPHY_MASK	(0x3 << 1)

#define DSIM_PHY_MD_SER_EN_DPHY(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MD_SER_EN_DPHY_MASK		(0x1 << 0)

/* DSIM_PHY_MD_SERDES_CTRL_CPHY0 */
/* Valid for MD0/MD1/MD2, but MD3 NA */

/* DSIM_PHY_MD_DEC_CTRL_DCPHY0 */
/* Valid for MD0/MD1/MD2/MD3 */
#define DSIM_PHY_MD_MON_DEC_ERR_CNT(_x)		(((_x) & 0xff) << 8)
#define DSIM_PHY_MD_MON_DEC_ERR_CNT_MASK	(0xff << 8)

#define DSIM_PHY_MD_MON_DEC_ERR(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MD_MON_DEC_ERR_MASK		(0x1 << 0)

/* DSIM_PHY_MD_BIST_CTRL0
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_BIST_CTRL1
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_BIST_CTRL2
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_BIST_CTRL3
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_BIST_STATUS
 * Valid for MD0/MD1/MD2/MD3
 */

/* DSIM_PHY_MD_DAT_CTRL0
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_T_PHY_READY(_x)		(((_x) & 0xffff) << 16)
#define DSIM_PHY_MD_T_PHY_READY_MASK		(0xffff << 16)

#define DSIM_PHY_MD_PHY_READY(_x)		(((_x) & 0x1) << 15) //read only
#define DSIM_PHY_MD_PHY_READY_MASK		(0x1 << 15)
#define DSIM_PHY_MD_PHY_READY_GET(x)		(((x) >> 15) & 0x1)

#define DSIM_PHY_MD_ERR_CONTENTION_DISABLE(_x)	(((_x) & 0x1) << 12)
#define DSIM_PHY_MD_ERR_CONTENTION_DISABLE_MASK	(0x1 << 12)

#define DSIM_PHY_MD_TRIG_EXIT_CLK_EN(_x)	(((_x) & 0x1) << 10)
#define DSIM_PHY_MD_TRIG_EXIT_CLK_EN_MASK	(0x1 << 10)

#define DSIM_PHY_MD_PROG_SEQ_EN(_x)		(((_x) & 0x1) << 9)
#define DSIM_PHY_MD_PROG_SEQ_EN_MASK		(0x1 << 9)

#define DSIM_PHY_MD_LPTX_GATING_EN(_x)		(((_x) & 0x1) << 8)
#define DSIM_PHY_MD_LPTX_GATING_EN_MASK		(0x1 << 8)

#define DSIM_PHY_MD_SYMB_SWAP(_x)		(((_x) & 0x1) << 7)
#define DSIM_PHY_MD_SYMB_SWAP_MASK		(0x1 << 7)

#define DSIM_PHY_MD_DATA_SWAP(_x)		(((_x) & 0x1) << 6)
#define DSIM_PHY_MD_DATA_SWAP_MASK		(0x1 << 6)

#define DSIM_PHY_MD_DATA_INV(_x)		(((_x) & 0x1) << 4)
#define DSIM_PHY_MD_DATA_INV_MASK		(0x1 << 4)

#define DSIM_PHY_MD_LPTX_SWAP(_x)		(((_x) & 0x1) << 2)
#define DSIM_PHY_MD_LPTX_SWAP_MASK		(0x1 << 2)

#define DSIM_PHY_MD_CRC_SKEW_CAL_EN(_x)		(((_x) & 0x1) << 1)
#define DSIM_PHY_MD_CRC_SKEW_CAL_EN_MASK	(0x1 << 1)

#define DSIM_PHY_MD_PREAMBLE_SEL(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MD_PREAMBLE_SEL_MASK		(0x1 << 0)

/* DSIM_PHY_MD_DAT_CTRL1
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_T_CAL_PREAMBLE(_x)		(((_x) & 0x3ff) << 22)
#define DSIM_PHY_MD_T_CAL_PREAMBLE_MASK		(0x3ff << 22)

#define DSIM_PHY_MD_T_LP_EXIT_SKEW(_x)		(((_x) & 0x3) << 20)
#define DSIM_PHY_MD_T_LP_EXIT_SKEW_MASK		(0x3 << 20)

#define DSIM_PHY_MD_T_LP_ENTRY_SKEW(_x)		(((_x) & 0x3) << 18)
#define DSIM_PHY_MD_T_LP_ENTRY_SKEW_MASK	(0x3 << 18)

#define DSIM_PHY_MD_T_TA_GET(_x)		(((_x) & 0xf) << 14)
#define DSIM_PHY_MD_T_TA_GET_MASK		(0xf << 14)

#define DSIM_PHY_MD_T_TA_GO(_x)			(((_x) & 0xf) << 10)
#define DSIM_PHY_MD_T_TA_GO_MASK		(0xf << 10)

#define DSIM_PHY_MD_T_ULPS_EXIT(_x)		(((_x) & 0x3ff) << 0)
#define DSIM_PHY_MD_T_ULPS_EXIT_MASK		(0x3ff << 0)

/* DSIM_PHY_MD_DAT_CTRL2
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_T_HS_TRAIL(_x)		(((_x) & 0xff) << 24)
#define DSIM_PHY_MD_T_HS_TRAIL_MASK		(0xff << 24)

#define DSIM_PHY_MD_T_HS_ZERO(_x)		(((_x) & 0xff) << 16)
#define DSIM_PHY_MD_T_HS_ZERO_MASK		(0xff << 16)

#define DSIM_PHY_MD_T_HS_PREPARE(_x)		(((_x) & 0xff) << 8)
#define DSIM_PHY_MD_T_HS_PREPARE_MASK		(0xff << 8)

#define DSIM_PHY_MD_T_LPX(_x)			(((_x) & 0xff) << 0)
#define DSIM_PHY_MD_T_LPX_MASK			(0xff << 0)

/* DSIM_PHY_MD_DAT_CTRL3
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_T_HS_EXIT(_x)		(((_x) & 0xff) << 0)
#define DSIM_PHY_MD_T_HS_EXIT_MASK		(0xff << 0)

/* DSIM_PHY_MD_DAT_CTRL4
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_T_HS_TRAIL_SDW(_x)		(((_x) & 0xff) << 24)
#define DSIM_PHY_MD_T_HS_TRAIL_SDW_MASK		(0xff << 24)

#define DSIM_PHY_MD_T_HS_ZERO_SDW(_x)		(((_x) & 0xff) << 16)
#define DSIM_PHY_MD_T_HS_ZERO_SDW_MASK		(0xff << 16)

#define DSIM_PHY_MD_T_HS_PREPARE_SDW(_x)	(((_x) & 0xff) << 8)
#define DSIM_PHY_MD_T_HS_PREPARE_SDW_MASK	(0xff << 8)

#define DSIM_PHY_MD_T_LPX_SDW(_x)		(((_x) & 0xff) << 0)
#define DSIM_PHY_MD_T_LPX_SDW_MASK		(0xff << 0)

/* DSIM_PHY_MD_DAT_CTRL5
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_T_HS_EXIT_SDW(_x)		(((_x) & 0xff) << 0)
#define DSIM_PHY_MD_T_HS_EXIT_SDW_MASK		(0xff << 0)

/* DSIM_PHY_MD_DAT_SKEW0
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_SKEW_CAL_RUN_TIME(_x)	(((_x) & 0xf) << 20)
#define DSIM_PHY_MD_SKEW_CAL_RUN_TIME_MASK	(0xf << 20)

#define DSIM_PHY_MD_SKEW_CAL_INIT_RUN_TIME(_x)	(((_x) & 0xf) << 4)
#define DSIM_PHY_MD_SKEW_CAL_INIT_RUN_TIME_MASK	(0xf << 4)

#define DSIM_PHY_MD_INIT_SKEW_CAL_CHECK_EN(_x)	(((_x) & 0x1) << 2)
#define DSIM_PHY_MD_INIT_SKEW_CAL_CHECK_EN_MASK	(0x1 << 2)

#define DSIM_PHY_MD_SKEW_CAL_EN(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MD_SKEW_CAL_EN_MASK		(0x1 << 0)

/* DSIM_PHY_MD_DAT_SKEW1
 * Valid for MD0/MD1/MD2/MD3
 */
#define DSIM_PHY_MD_SKEW_CAL_RUN_TIME_SDW(_x)	(((_x) & 0xf) << 20)
#define DSIM_PHY_MD_SKEW_CAL_RUN_TIME_SDW_MASK	(0xf << 20)

#define DSIM_PHY_MD_SKEW_CAL_INIT_RUN_TIME_SDW(_x)	(((_x) & 0xf) << 4)
#define DSIM_PHY_MD_SKEW_CAL_INIT_RUN_TIME_SDW_MASK	(0xf << 4)

#define DSIM_PHY_MD_INIT_SKEW_CAL_CHECK_EN_SDW(_x)	(((_x) & 0x1) << 2)
#define DSIM_PHY_MD_INIT_SKEW_CAL_CHECK_EN_SDW_MASK	(0x1 << 2)

#define DSIM_PHY_MD_SKEW_CAL_EN_SDW(_x)		(((_x) & 0x1) << 0)
#define DSIM_PHY_MD_SKEW_CAL_EN_SDW_MASK	(0x1 << 0)

/* DSIM_PHY_MD_DAT_PROG0
 * Valid for MD0/MD1/MD2, but MD3 NA
 */
#define DSIM_PHY_MD_PROG_SEQ0(_x)		(((_x) & 0xffffffff) << 0)
#define DSIM_PHY_MD_PROG_SEQ0_MASK		(0xffffffff << 0)

/* DSIM_PHY_MD_DAT_PROG1
 * Valid for MD0/MD1/MD2, but MD3 NA
 */
#define DSIM_PHY_MD_PROG_SEQ1(_x)		(((_x) & 0x3ff) << 0)
#define DSIM_PHY_MD_PROG_SEQ1_MASK		(0x3ff << 0)

/* DSIM_PHY_MD_DAT_STAT0
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_STAT1
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_STAT2
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_STAT3
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_STAT4
 * Valid for MD0/MD1/MD2, but MD3 NA
 */
/* DSIM_PHY_MD_DAT_STAT5
 * Valid for MD0/MD1/MD2, but MD3 NA
 */
/* DSIM_PHY_MD_DAT_STAT6
 * Valid for MD0/MD1/MD2, but MD3 NA
 */
/* DSIM_PHY_MD_DAT_STAT7
 * Valid for MD0/MD1/MD2, but MD3 NA
 */

/* DSIM_PHY_MD_DAT_DBG0
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_DBG
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_DBG2
 * Valid for MD0/MD1/MD2, but MD3 NA
 */
/* DSIM_PHY_MD_DAT_DBG3
 * Valid for MD0/MD1/MD2/MD3
 */

/* DSIM_PHY_MD_DAT_TEST0
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_TEST1
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_TEST2
 * Valid for MD0/MD1/MD2/MD3
 */
/* DSIM_PHY_MD_DAT_TEST3
 * Valid for MD0/MD1/MD2/MD3
 */

#endif

#if 0 // dcphy 6310
/* macros for DPHY timing controls */
/* MC/MD_TIME_CON0 */
#define DSIM_PHY_HSTX_CLK_SEL(_x)		(((_x) & 0x1) << 12)
#define DSIM_PHY_TLPX(_x)			(((_x) & 0xff) << 4)
#define DSIM_PHY_TLPX_MASK			(0xff << 4)
/* MD only */
#define DSIM_PHY_TLP_EXIT_SKEW(_x)		(((_x) & 0x3) << 2)
#define DSIM_PHY_TLP_EXIT_SKEW_MASK		(0x3 << 2)
#define DSIM_PHY_TLP_ENTRY_SKEW(_x)		(((_x) & 0x3) << 0)
#define DSIM_PHY_TLP_ENTRY_SKEW_MASK		(0x3 << 0)

/* MC_TIME_CON1 */
#define DSIM_PHY_TCLK_ZERO(_x)			(((_x) & 0xff) << 8)
#define DSIM_PHY_TCLK_ZERO_MASK			(0xff << 8)
#define DSIM_PHY_TCLK_PREPARE(_x)		(((_x) & 0xff) << 0)
#define DSIM_PHY_TCLK_PREPARE_MASK		(0xff << 0)
/* MD case */
#define DSIM_PHY_THS_ZERO(_x)			(((_x) & 0xff) << 8)
#define DSIM_PHY_THS_ZERO_MASK			(0xff << 8)
#define DSIM_PHY_THS_PREPARE(_x)		(((_x) & 0xff) << 0)
#define DSIM_PHY_THS_PREPARE_MASK		(0xff << 0)

/* MC/MD_TIME_CON2 */
#define DSIM_PHY_THS_EXIT(_x)			(((_x) & 0xff) << 8)
#define DSIM_PHY_THS_EXIT_MASK			(0xff << 8)
/* MC case */
#define DSIM_PHY_TCLK_TRAIL(_x)			(((_x) & 0xff) << 0)
#define DSIM_PHY_TCLK_TRAIL_MASK		(0xff << 0)
/* MD case */
#define DSIM_PHY_THS_TRAIL(_x)			(((_x) & 0xff) << 0)
#define DSIM_PHY_THS_TRAIL_MASK			(0xff << 0)

/* MC_TIME_CON3 */
#define DSIM_PHY_TCLK_POST(_x)			(((_x) & 0xff) << 0)
#define DSIM_PHY_TCLK_POST_MASK			(0xff << 0)
/* MD_TIME_CON3 */
#define DSIM_PHY_TTA_GET(_x)			(((_x) & 0xf) << 4)
#define DSIM_PHY_TTA_GET_MASK			(0xf << 4)
#define DSIM_PHY_TTA_GO(_x)			(((_x) & 0xf) << 0)
#define DSIM_PHY_TTA_GO_MASK			(0xf << 0)

/* MC/MD_TIME_CON4 */
#define DSIM_PHY_ULPS_EXIT(_x)			(((_x) & 0x3ff) << 0)
#define DSIM_PHY_ULPS_EXIT_MASK			(0x3ff << 0)

/* MC_DATA_CON0 */
#define DSIM_PHY_CLK_INV			(0x1 << 1)
#define DSIM_PHY_USE_SDW_REG			(0x1 << 3) // new bitfield for freq hopping
/* MD_DATA_CON0 */
#define DSIM_PHY_DATA_INV			(0x1 << 1)
#endif

#endif /* _REGS_DSIM_H */
