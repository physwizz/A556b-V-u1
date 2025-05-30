/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for S5E8855 clock controller.
*/

#ifndef _DT_BINDINGS_CLOCK_S5E8855_H
#define	_DT_BINDINGS_CLOCK_S5E8855_H

//*********************************OSC*************************************
#define	NONE	(0 + 0)
#define	OSCCLK1	(0 + 1)

//*********************************ALIVE*************************************
#define	CLK_ALIVE_BASE							(10)
#define	GATE_MCT_ALIVE_QCH						(CLK_ALIVE_BASE + 0)
#define	DOUT_CLKALIVE_CHUBVTS_NOC				(CLK_ALIVE_BASE + 1)
#define	DOUT_DIV_CLKALIVE_SCA_USI0				(CLK_ALIVE_BASE + 2)
#define	DOUT_DIV_CLK_ALIVE_NOC					(CLK_ALIVE_BASE + 3)
#define	DOUT_DIV_CLK_ALIVE_I2C					(CLK_ALIVE_BASE + 4)
#define DOUT_DIV_CLK_ALIVE_DBGCORE_UART			(CLK_ALIVE_BASE + 5)
#define DOUT_DIV_CLK_ALIVE_SPMI					(CLK_ALIVE_BASE + 6)
#define	GATE_USI_ALIVE0_QCH						(CLK_ALIVE_BASE + 7)
#define	GATE_I2C_ALIVE0_QCH						(CLK_ALIVE_BASE + 8)

#define	DOUT_DIV_CLKCMU_ALIVE_BOOST				(CLK_ALIVE_BASE + 9)
#define	DOUT_DIV_CLK_ALIVE_CMGP_NOC				(CLK_ALIVE_BASE + 10)
#define	DOUT_DIV_CLK_ALIVE_CMGP_PERI_ALIVE		(CLK_ALIVE_BASE + 11)
#define	DOUT_DIV_CLK_ALIVE_DBGCORE_NOC			(CLK_ALIVE_BASE + 12)

#define	MOUT_MUX_CLK_RCO_ALIVE_USER				(CLK_ALIVE_BASE + 13)
#define	MOUT_MUX_CLK_ALIVE_USI0					(CLK_ALIVE_BASE + 14)
#define	MOUT_MUX_CLK_RCO_ALIVE_SPMI_USER		(CLK_ALIVE_BASE + 15)
#define	MOUT_MUX_CLK_ALIVE_DBGCORE_UART			(CLK_ALIVE_BASE + 16)
#define MOUT_MUX_CLK_ALIVE_I2C					(CLK_ALIVE_BASE + 17)
#define	MOUT_MUX_CLK_ALIVE_NOC					(CLK_ALIVE_BASE + 18)
#define	MOUT_MUX_CLK_ALIVE_SPMI					(CLK_ALIVE_BASE + 19)
#define	MOUT_MUX_CLK_ALIVE_TIMER_USER			(CLK_ALIVE_BASE + 20)
#define	MOUT_MUX_CLK_ALIVE_TIMER_ASM_USER		(CLK_ALIVE_BASE + 21)




/* top */
#define	CLK_TOP_BASE						(100)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK0		(CLK_TOP_BASE + 0)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK1		(CLK_TOP_BASE + 1)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK2		(CLK_TOP_BASE + 2)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK3		(CLK_TOP_BASE + 3)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK4		(CLK_TOP_BASE + 4)

#define	DOUT_DIV_CLKCMU_AUD_NOC				(CLK_TOP_BASE + 5)
#define	DOUT_DIV_CLKCMU_AUD_CPU				(CLK_TOP_BASE + 6)
#define	DOUT_DIV_CLKCMU_CIS_CLK0			(CLK_TOP_BASE + 7)
#define	DOUT_DIV_CLKCMU_CIS_CLK1			(CLK_TOP_BASE + 8)
#define	DOUT_DIV_CLKCMU_CIS_CLK2			(CLK_TOP_BASE + 9)
#define	DOUT_DIV_CLKCMU_CIS_CLK3			(CLK_TOP_BASE + 10)
#define	DOUT_DIV_CLKCMU_CIS_CLK4			(CLK_TOP_BASE + 11)
#define	DOUT_CLKCMU_USB_NOC					(CLK_TOP_BASE + 12)
#define	DOUT_CLKCMU_USB_USB20DRD			(CLK_TOP_BASE + 13)
#define	DOUT_CLKCMU_M2M_NOC					(CLK_TOP_BASE + 14)
#define	DOUT_CLKCMU_M2M_JPEG				(CLK_TOP_BASE + 15)
#define	DOUT_CLKCMU_M2M_GDC					(CLK_TOP_BASE + 16)
#define	DOUT_CLKCMU_PERIC_MMC_CARD			(CLK_TOP_BASE + 17)

#define	DOUT_CLKCMU_DPU_NOC					(CLK_TOP_BASE + 18)
#define	DOUT_CLKCMU_GNPU0_NOC				(CLK_TOP_BASE + 19)
#define	DOUT_CLKCMU_GNPU0_XMAA				(CLK_TOP_BASE + 20)
#define	DOUT_CLKCMU_UFS_UFS_EMBD			(CLK_TOP_BASE + 21)
#define	DOUT_CLKCMU_HSI0_NOC				(CLK_TOP_BASE + 22)
#define	DOUT_DIV_CLKCMU_CSIS_OIS			(CLK_TOP_BASE + 23)
#define	DOUT_DIV_CLKCMU_CSIS_DCPHY			(CLK_TOP_BASE + 24)
#define DOUT_DIV_CLKCMU_G3D_HTU				(CLK_TOP_BASE + 25)
#define	DOUT_DIV_CLKCMU_G3D_NOCP			(CLK_TOP_BASE + 26)
#define	DOUT_DIV_CLKCMU_GNPU0_NOC			(CLK_TOP_BASE + 27)

/* aud */
#define	CLK_AUD_BASE					(200)

#define	GATE_ABOX_QCH_BCLK_DSIF			(CLK_AUD_BASE + 0)
#define	GATE_ABOX_QCH_BCLK0				(CLK_AUD_BASE + 1)
#define	GATE_ABOX_QCH_BCLK1				(CLK_AUD_BASE + 2)
#define	GATE_ABOX_QCH_BCLK2				(CLK_AUD_BASE + 3)
#define	GATE_ABOX_QCH_BCLK3				(CLK_AUD_BASE + 4)
#define	GATE_ABOX_QCH_BCLK4				(CLK_AUD_BASE + 5)
#define	GATE_ABOX_QCH_BCLK5				(CLK_AUD_BASE + 6)
#define	GATE_ABOX_QCH_BCLK6				(CLK_AUD_BASE + 7)

#define	MOUT_MUX_CLK_AUD_PCMC			(CLK_AUD_BASE + 8)
#define	MOUT_CLK_AUD_UAIF0				(CLK_AUD_BASE + 9)
#define	MOUT_CLK_AUD_UAIF1				(CLK_AUD_BASE + 10)
#define	MOUT_CLK_AUD_UAIF2				(CLK_AUD_BASE + 11)
#define	MOUT_CLK_AUD_UAIF3				(CLK_AUD_BASE + 12)
#define	MOUT_CLK_AUD_UAIF4				(CLK_AUD_BASE + 13)
#define	MOUT_CLK_AUD_UAIF5				(CLK_AUD_BASE + 14)
#define	MOUT_CLK_AUD_UAIF6				(CLK_AUD_BASE + 15)
#define	UMUX_CP_PCMC_CLK				(CLK_AUD_BASE + 16)
#define	UMUX_CP_PCMC_CLK_USER			(CLK_AUD_BASE + 17)
#define	UMUX_CLK_AUD_CPU_PLL			(CLK_AUD_BASE + 18)
#define	MOUT_CLKVTS_AUD_DMIC			(CLK_AUD_BASE + 19)

#define	DOUT_DIV_CLK_AUD_DSIF			(CLK_AUD_BASE + 20)
#define	DOUT_DIV_CLK_AUD_UAIF0			(CLK_AUD_BASE + 21)
#define	DOUT_DIV_CLK_AUD_UAIF1			(CLK_AUD_BASE + 22)
#define	DOUT_DIV_CLK_AUD_UAIF2			(CLK_AUD_BASE + 23)
#define	DOUT_DIV_CLK_AUD_UAIF3			(CLK_AUD_BASE + 24)
#define	DOUT_DIV_CLK_AUD_UAIF4			(CLK_AUD_BASE + 25)
#define	DOUT_DIV_CLK_AUD_UAIF5			(CLK_AUD_BASE + 26)
#define	DOUT_DIV_CLK_AUD_UAIF6			(CLK_AUD_BASE + 27)

#define	MOUT_CLK_AUD_CPU_PLL			(CLK_AUD_BASE + 28)
#define	DOUT_DIV_CLK_AUD_NOCD			(CLK_AUD_BASE + 29)
#define	DOUT_DIV_CLK_AUD_CNT			(CLK_AUD_BASE + 30)
#define	DOUT_DIV_CLK_AUD_AUDIF			(CLK_AUD_BASE + 31)
#define	DOUT_DIV_CLK_AUD_FM				(CLK_AUD_BASE + 32)
#define	DOUT_CLK_AUD_PCMC				(CLK_AUD_BASE + 33)
#define	DOUT_CLK_AUD_SERIAL_LIF			(CLK_AUD_BASE + 34)
#define	DOUT_CLK_AUD_SERIAL_LIF_CORE	(CLK_AUD_BASE + 35)


#define	GATE_LH_AXI_SI_D_AUD_QCH		(CLK_AUD_BASE + 36)
#define	GATE_CMU_AUD_QCH				(CLK_AUD_BASE + 37)
#define	GATE_PPMU_D_AUD_QCH				(CLK_AUD_BASE + 38)
#define	GATE_WDT_AUD_QCH				(CLK_AUD_BASE + 39)
#define	GATE_D_TZPC_AUD_QCH				(CLK_AUD_BASE + 40)
#define	GATE_ECU_AUD_QCH				(CLK_AUD_BASE + 41)
#define	GATE_SLH_AXI_MI_P_AUD_QCH		(CLK_AUD_BASE + 42)
#define	GATE_SERIAL_LIF_AUD_QCH_BCLK	(CLK_AUD_BASE + 43)
#define	GATE_SYSREG_AUD_QCH				(CLK_AUD_BASE + 44)
#define	GATE_VGEN_LITE_D_AUD_QCH		(CLK_AUD_BASE + 45)

#define DOUT_DIV_CLK_AUD_MCLK			(CLK_AUD_BASE + 46)

/* hsi */
#define	CLK_HSI_BASE						(300)
#define	UMUX_CLKCMU_HSI_NOC_USER			(CLK_HSI_BASE + 0)
#define	UMUX_CLKCMU_HSI_UFS_EMBD_USER		(CLK_HSI_BASE + 1)
#define	GATE_UFS_EMBD_QCH_FMP				(CLK_HSI_BASE + 2)
#define	GATE_D_TZPC_HSI_QCH					(CLK_HSI_BASE + 3)
#define	GATE_GPIO_HSI_UFS_QCH				(CLK_HSI_BASE + 4)
#define	GATE_HSI_CMU_HSI_QCH				(CLK_HSI_BASE + 5)
#define	GATE_PPMU_D_HSI_QCH					(CLK_HSI_BASE + 6)
#define	GATE_S2MPU_S0_HSI_QCH_S0			(CLK_HSI_BASE + 7)
#define	GATE_SLH_AXI_MI_P_HSI_QCH			(CLK_HSI_BASE + 8)
#define	GATE_SLH_AXI_SI_D_HSI_QCH			(CLK_HSI_BASE + 9)
#define	GATE_SYSREG_HSI_QCH					(CLK_HSI_BASE + 10)
#define	GATE_VGEN_LITE_HSI_QCH				(CLK_HSI_BASE + 11)
#define	GATE_ECU_HSI_QCH					(CLK_HSI_BASE + 12)
#define	GATE_S2MPU_S0_PMMU0_HSI_QCH_S0		(CLK_HSI_BASE + 13)

/* peric */
#define	CLK_PERIC_BASE						(400)

#define	GATE_PERI_CMU_PERI_QCH				(CLK_PERIC_BASE + 0)
#define	UMUX_CLKCMU_PERIC_MMC_CARD_USER		(CLK_PERIC_BASE + 1)
#define	MOUT_CLK_PERIC_UART_DBG				(CLK_PERIC_BASE + 2)
#define	DOUT_DIV_CLK_PERIC_UART_DBG			(CLK_PERIC_BASE + 3)
#define	DOUT_DIV_CLK_PERIC_USI_I2C			(CLK_PERIC_BASE + 4)
#define	DOUT_DIV_CLK_PERIC_USI10_USI		(CLK_PERIC_BASE + 5)
#define	DOUT_DIV_CLK_PERIC_USI09_USI		(CLK_PERIC_BASE + 6)
#define	DOUT_DIV_CLK_PERIC_USI04_USI		(CLK_PERIC_BASE + 7)
#define	DOUT_DIV_CLK_PERIC_USI03_USI		(CLK_PERIC_BASE + 8)
#define	DOUT_DIV_CLK_PERIC_USI02_USI		(CLK_PERIC_BASE + 9)
#define	DOUT_DIV_CLK_PERIC_USI01_USI		(CLK_PERIC_BASE + 10)
#define	DOUT_DIV_CLK_PERIC_USI00_USI		(CLK_PERIC_BASE + 11)
#define	GATE_PPMU_D_PERIC_QCH				(CLK_PERIC_BASE + 12)
#define	GATE_PWM_QCH						(CLK_PERIC_BASE + 13)
#define	GATE_S2MPU_S0_PERIC_QCH_S0			(CLK_PERIC_BASE + 14)
#define	GATE_SLH_AXI_MI_P_PERI_QCH			(CLK_PERIC_BASE + 15)
#define	GATE_SLH_AXI_SI_D_PERI_QCH			(CLK_PERIC_BASE + 16)
#define	GATE_SYSREG_PERI_QCH				(CLK_PERIC_BASE + 17)
#define	GATE_TMU_QCH						(CLK_PERIC_BASE + 18)
#define	GATE_VGEN_LITE_D_PERIC_QCH			(CLK_PERIC_BASE + 19)
#define	GATE_USI05_I2C_QCH					(CLK_PERIC_BASE + 20)
#define	GATE_USI00_I2C_QCH					(CLK_PERIC_BASE + 21)
#define	GATE_USI01_I2C_QCH					(CLK_PERIC_BASE + 22)
#define	GATE_USI02_I2C_QCH					(CLK_PERIC_BASE + 23)
#define	GATE_USI03_I2C_QCH					(CLK_PERIC_BASE + 24)
#define	GATE_USI04_I2C_QCH					(CLK_PERIC_BASE + 25)
#define	GATE_USI00_USI_QCH					(CLK_PERIC_BASE + 26)
#define	GATE_USI01_USI_QCH					(CLK_PERIC_BASE + 27)
#define	GATE_USI02_USI_QCH					(CLK_PERIC_BASE + 28)
#define	GATE_USI03_USI_QCH					(CLK_PERIC_BASE + 29)
#define	GATE_USI04_USI_QCH					(CLK_PERIC_BASE + 30)
#define	GATE_USI09_USI_QCH					(CLK_PERIC_BASE + 31)
#define	GATE_USI10_USI_QCH					(CLK_PERIC_BASE + 32)
#define	GATE_UART_DBG_QCH					(CLK_PERIC_BASE + 33)
#define	GATE_D_TZPC_PERI_QCH				(CLK_PERIC_BASE + 34)
#define	GATE_GPIO_PERI_QCH					(CLK_PERIC_BASE + 35)
#define	GATE_GPIO_PERIMMC_QCH_GPIO			(CLK_PERIC_BASE + 36)
#define	GATE_MMC_CARD_QCH					(CLK_PERIC_BASE + 37)
#define	GATE_OTP_CON_TOP_QCH				(CLK_PERIC_BASE + 38)
#define	GATE_PDMA_PERIC_QCH					(CLK_PERIC_BASE + 39)


/* rgbp */
#define	CLK_RGBP_BASE					(500)
#define	DOUT_DIV_CLK_RGBP_NOCP			(CLK_RGBP_BASE + 0)
#define	MOUT_MUX_CLKCMU_RGBP_NOC_USER	(CLK_RGBP_BASE + 1)

#define	GATE_RGBP_QCH					(CLK_RGBP_BASE + 2)
#define	GATE_CMU_RGBP_QCH				(CLK_RGBP_BASE + 3)
#define	GATE_D_TZPC_RGBP_QCH			(CLK_RGBP_BASE + 4)
#define	GATE_SYSMMU_S0_RGBP_QCH_S0		(CLK_RGBP_BASE + 5)
#define	GATE_SYSREG_RGBP_QCH			(CLK_RGBP_BASE + 6)
#define	GATE_SLH_AXI_MI_P_RGBP_QCH		(CLK_RGBP_BASE + 7)
#define	GATE_SSLH_AXI_SI_P_RGBP_QCH		(CLK_RGBP_BASE + 8)



/* usb */
#define	CLK_USB_BASE						(600)
#define	MOUT_MUX_CLKCMU_USB_NOC_USER		(CLK_USB_BASE + 0)
#define	MOUT_MUX_CLKCMU_USB_USB20DRD_USER	(CLK_USB_BASE + 1)
#define	MOUT_MUX_CLK_USB_USB20DRD			(CLK_USB_BASE + 2)
#define	DOUT_DIV_CLK_USB_NOC_DIV3			(CLK_USB_BASE + 3)

#define	GATE_D_TZPC_USB_QCH					(CLK_USB_BASE + 4)
#define	GATE_VGEN_LITE_USB_QCH				(CLK_USB_BASE + 5)
#define	GATE_S2MPU_S0_USB_QCH_S0			(CLK_USB_BASE + 6)
#define	GATE_SLH_AXI_MI_P_USB_QCH			(CLK_USB_BASE + 7)
#define	GATE_SLH_AXI_SI_D_USB_QCH			(CLK_USB_BASE + 8)
#define	GATE_LH_AXI_MI_D_USB_QCH			(CLK_USB_BASE + 9)
#define	GATE_SYSREG_USB_QCH					(CLK_USB_BASE + 10)
#define	GATE_USB20DRD_TOP_QCH_S_SUBCTRL		(CLK_USB_BASE + 11)
#define	GATE_USB20DRD_TOP_QCH_S_LINK		(CLK_USB_BASE + 12)
#define	GATE_USB_CMU_USB_QCH				(CLK_USB_BASE + 13)


/* yuvp */
#define	CLK_YUVP_BASE						(700)
#define	GATE_YUVP_QCH						(CLK_YUVP_BASE + 0)
#define	MOUT_MUX_CLKCMU_YUVP_NOC_USER		(CLK_YUVP_BASE + 1)
#define	DOUT_DIV_CLK_YUVP_NOCP				(CLK_YUVP_BASE + 2)

#define	GATE_CMU_YUVP_QCH					(CLK_YUVP_BASE + 3)
#define	GATE_D_TZPC_YUVP_QCH				(CLK_YUVP_BASE + 4)
#define	GATE_PPMU_D_YUVP_QCH				(CLK_YUVP_BASE + 5)
#define	GATE_SLH_AXI_MI_P_YUVP_QCH			(CLK_YUVP_BASE + 6)
#define	GATE_SYSREG_YUVP_QCH				(CLK_YUVP_BASE + 7)
#define	GATE_SLH_AXI_SI_P_YUVP_QCH			(CLK_YUVP_BASE + 8)

/* csis */
#define	CLK_CSIS_BASE							(800)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS0		(CLK_CSIS_BASE + 0)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS1		(CLK_CSIS_BASE + 1)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS2		(CLK_CSIS_BASE + 2)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS3		(CLK_CSIS_BASE + 3)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS4		(CLK_CSIS_BASE + 4)
#define	GATE_LH_AST_MI_OTF0_CSIS_CSTAT_QCH		(CLK_CSIS_BASE + 5)
#define	GATE_LH_AST_MI_OTF1_CSIS_CSTAT_QCH		(CLK_CSIS_BASE + 6)

#define	GATE_SYSREG_CSIS_QCH					(CLK_CSIS_BASE + 7)
#define	GATE_SLH_AXI_MI_P_CSIS_QCH				(CLK_CSIS_BASE + 8)
#define	GATE_CMU_CSIS_QCH						(CLK_CSIS_BASE + 9)
#define	GATE_D_TZPC_CSIS_QCH					(CLK_CSIS_BASE + 10)
#define	GATE_SLH_AXI_SI_P_CSIS_QCH				(CLK_CSIS_BASE + 11)
#define	GATE_LH_AXI_MI_IP_POIS_CSIS_QCH			(CLK_CSIS_BASE + 12)
#define	GATE_LH_AXI_SI_IP_POIS_CSIS_QCH			(CLK_CSIS_BASE + 13)

#define	DOUT_DIV_CLK_CSIS_DCPHY					(CLK_CSIS_BASE + 14)
#define	DOUT_DIV_CLK_CSIS_NOCP					(CLK_CSIS_BASE + 15)
#define	MOUT_MUX_CLKCMU_CSIS_NOC_USER			(CLK_CSIS_BASE + 16)
#define	MOUT_MUX_CLKCMU_CSIS_OIS_USER			(CLK_CSIS_BASE + 17)
#define	MOUT_MUX_CLKCMU_CSIS_DCPHY_USER			(CLK_CSIS_BASE + 18)
#define	MOUT_MUX_CLK_CSIS_DCPHY					(CLK_CSIS_BASE + 19)

#define	GATE_CSIS_PDP_QCH_PDP_TOP				(CLK_CSIS_BASE + 20)

/* m2m */
#define	CLK_M2M_BASE						(900)
#define	GATE_M2M_QCH_S1						(CLK_M2M_BASE + 0)
#define	GATE_M2M_QCH_S2						(CLK_M2M_BASE + 1)
#define	GATE_D_TZPC_M2M_QCH					(CLK_M2M_BASE + 3)
#define	GATE_JPEG0_QCH						(CLK_M2M_BASE + 4)
#define	GATE_LH_AXI_SI_D0_M2M_QCH			(CLK_M2M_BASE + 5)
#define	GATE_LH_AXI_SI_D1_M2M_QCH			(CLK_M2M_BASE + 6)
#define	GATE_M2M_CMU_M2M_QCH				(CLK_M2M_BASE + 7)
#define	GATE_PPMU_D0_M2M_QCH				(CLK_M2M_BASE + 8)
#define	GATE_PPMU_D1_M2M_QCH				(CLK_M2M_BASE + 9)
#define	GATE_SLH_AXI_MI_P_M2M_QCH			(CLK_M2M_BASE + 10)
#define	GATE_SYSREG_M2M_QCH					(CLK_M2M_BASE + 11)
#define	GATE_VGEN_LITE_D0_M2M_QCH			(CLK_M2M_BASE + 12)
#define	GATE_VGEN_LITE_D1_M2M_QCH			(CLK_M2M_BASE + 13)

#define	MOUT_MUX_CLKCMU_M2M_GDC_USER		(CLK_M2M_BASE + 14)
#define	MOUT_MUX_CLKCMU_M2M_JPEG_USER		(CLK_M2M_BASE + 15)
#define	MOUT_MUX_CLKCMU_M2M_NOC_USER		(CLK_M2M_BASE + 16)
#define	DOUT_DIV_CLK_M2M_NOCP				(CLK_M2M_BASE + 17)

/* cmgp */
#define	CLK_CMGP_BASE					(1000)
#define	GATE_CMGP_CMU_CMGP_QCH			(CLK_CMGP_BASE + 0)
#define	GATE_D_TZPC_CMGP_QCH			(CLK_CMGP_BASE + 1)
#define	GATE_GPIO_CMGP_QCH				(CLK_CMGP_BASE + 2)
#define	GATE_I2C_CMGP0_QCH				(CLK_CMGP_BASE + 3)
#define	GATE_I2C_CMGP1_QCH				(CLK_CMGP_BASE + 4)
#define	GATE_I2C_CMGP2_QCH				(CLK_CMGP_BASE + 5)
#define	GATE_I2C_CMGP3_QCH				(CLK_CMGP_BASE + 6)
#define	GATE_I2C_CMGP4_QCH				(CLK_CMGP_BASE + 7)
#define	GATE_I3C_CMGP0_QCH_P			(CLK_CMGP_BASE + 8)
#define	GATE_I3C_CMGP0_QCH_S			(CLK_CMGP_BASE + 9)
#define	GATE_I3C_CMGP1_QCH_P			(CLK_CMGP_BASE + 10)
#define	GATE_I3C_CMGP1_QCH_S			(CLK_CMGP_BASE + 11)
#define	GATE_SLH_AXI_MI_C_CMGP_QCH		(CLK_CMGP_BASE + 12)
#define	GATE_SYSREG_CMGP_QCH			(CLK_CMGP_BASE + 13)
#define	GATE_SYSREG_CMGP2APM_QCH		(CLK_CMGP_BASE + 14)
#define	GATE_SYSREG_CMGP2CHUB_QCH		(CLK_CMGP_BASE + 15)
#define	GATE_SYSREG_CMGP2CP_QCH			(CLK_CMGP_BASE + 16)
#define	GATE_SYSREG_CMGP2GNSS_QCH		(CLK_CMGP_BASE + 17)
#define	GATE_SYSREG_CMGP2PMU_AP_QCH		(CLK_CMGP_BASE + 18)
#define	GATE_SYSREG_CMGP2WLBT_QCH		(CLK_CMGP_BASE + 19)
#define	GATE_USI_CMGP0_QCH				(CLK_CMGP_BASE + 20)
#define	GATE_USI_CMGP1_QCH				(CLK_CMGP_BASE + 21)
#define	GATE_USI_CMGP2_QCH				(CLK_CMGP_BASE + 22)
#define	GATE_USI_CMGP3_QCH				(CLK_CMGP_BASE + 23)
#define	GATE_USI_CMGP4_QCH				(CLK_CMGP_BASE + 24)

#define	DOUT_DIV_CLK_CMGP_USI00_USI		(CLK_CMGP_BASE + 25)
#define	DOUT_DIV_CLK_CMGP_USI01_USI		(CLK_CMGP_BASE + 26)
#define	DOUT_DIV_CLK_CMGP_USI02_USI		(CLK_CMGP_BASE + 27)
#define	DOUT_DIV_CLK_CMGP_USI03_USI		(CLK_CMGP_BASE + 28)
#define	DOUT_DIV_CLK_CMGP_USI04_USI		(CLK_CMGP_BASE + 29)
#define	DOUT_DIV_CLK_CMGP_USI_I2C		(CLK_CMGP_BASE + 30)
#define	DOUT_DIV_CLK_CMGP_USI_I3C		(CLK_CMGP_BASE + 31)

/* cpucl0_glb */
#define	CLK_CPUCL0_GLB_BASE					(1100)
#define	MOUT_CLKCMU_CPUCL0_DBG_NOC_USER		(CLK_CPUCL0_GLB_BASE + 0)

/* mif */
#define	CLK_MIF_BASE				(1200)
#define	MUX_MIF_DDRPHY2X			(CLK_MIF_BASE + 0)
#define	MUX_MIF_DDRPHY2X_MIF1		(CLK_MIF_BASE + 1)

//*********************************NOCL1A*************************************
#define	CLK_NOCL1A_BASE					(1300)
#define	DOUT_DIV_CLK_NOCL1A_NOCP		(CLK_NOCL1A_BASE + 0)
#define	GATE_CMU_NOCL1A_QCH				(CLK_NOCL1A_BASE + 1)
#define	GATE_D_TZPC_NOCL1A_QCH			(CLK_NOCL1A_BASE + 2)
#define	GATE_ECU_NOCL1A_QCH				(CLK_NOCL1A_BASE + 3)
#define	GATE_SYSREG_NOCL1A_QCH			(CLK_NOCL1A_BASE + 4)
#define	GATE_TREX_D_NOCL1A_QCH			(CLK_NOCL1A_BASE + 5)
#define	GATE_TREX_PPMU_D0_NOCL1A_QCH	(CLK_NOCL1A_BASE + 6)
#define	GATE_TREX_PPMU_D1_NOCL1A_QCH	(CLK_NOCL1A_BASE + 7)

//*********************************MFC****************************************
#define	CLK_MFC_BASE							(1400)
#define	UMUX_CLKCMU_MFC_MFC						(CLK_MFC_BASE + 0)
#define	GATE_D_TZPC_MFC_QCH						(CLK_MFC_BASE + 1)
#define	GATE_SYSREG_MFC_QCH						(CLK_MFC_BASE + 2)
#define	GATE_MFC_QCH							(CLK_MFC_BASE + 3)
#define	GATE_MFC_CMU_MFC_QCH					(CLK_MFC_BASE + 4)
#define	GATE_PPMU_D0_MFC_QCH					(CLK_MFC_BASE + 5)
#define	GATE_VGEN_LITE_D_MFC_QCH				(CLK_MFC_BASE + 6)
#define	GATE_RSTNSYNC_CLK_MFC_BUSD_SW_RESET_QCH	(CLK_MFC_BASE + 7)
#define	GATE_SLH_AXI_MI_P_MFC_QCH				(CLK_MFC_BASE + 8)
#define	GATE_SYSMMU_S0_MFC_QCH_S0				(CLK_MFC_BASE + 9)
#define	DOUT_DIV_CLK_MFC_NOCP					(CLK_MFC_BASE + 10)
#define	DOUT_CLKCMU_MFC_MFC						(CLK_MFC_BASE + 11)
#define	UMUX_MUX_CLKCMU_MFC_NOC_USER			(CLK_MFC_BASE + 12)


//*********************************GNPU0***************************************
#define	CLK_GNPU0_BASE									(1500)
#define	GATE_D_TZPC_GNPU0_QCH							(CLK_GNPU0_BASE + 0)
#define	GATE_GNPU0_CMU_GNPU0_QCH						(CLK_GNPU0_BASE + 1)
#define	GATE_HTU_GNPU0_QCH_CLK							(CLK_GNPU0_BASE + 2)
#define	GATE_HTU_GNPU0_QCH_PCLK							(CLK_GNPU0_BASE + 3)
#define	GATE_SLH_AXI_MI_LP_DNC_GNPU0_QCH				(CLK_GNPU0_BASE + 4)
#define	GATE_IP_NPUCORE_QCH_CORE						(CLK_GNPU0_BASE + 5)
#define	GATE_LH_AST_SI_LD_SRAMCSTFIFO_SDMA_GNPU0_QCH	(CLK_GNPU0_BASE + 6)
#define	GATE_LH_AST_SI_LD_SRAMRDRSP0_SDMA_GNPU0_QCH		(CLK_GNPU0_BASE + 7)
#define	GATE_SLH_AXI_SI_LP_DNC_GNPU0_QCH				(CLK_GNPU0_BASE + 8)
#define	GATE_LH_AXI_SI_LD_CTRL_DNC_GNPU0_QCH			(CLK_GNPU0_BASE + 9)

#define	MOUT_MUX_CLKCMU_GNPU0_NOC_USER					(CLK_GNPU0_BASE + 10)
#define	MOUT_MUX_CLKCMU_GNPU0_XMAA_USER					(CLK_GNPU0_BASE + 11)
#define	DOUT_DIV_CLK_GNPU0_NOCP							(CLK_GNPU0_BASE + 12)
#define	GATE_LH_AXI_MI_LD_CTRL_GNPU0_DNC_QCH			(CLK_GNPU0_BASE + 13)
#define	GATE_LH_AXI_MI_LD_DRAM_GNPU0_DNC_QCH			(CLK_GNPU0_BASE + 14)


//*********************************NPUS***************************************
#define	CLK_NPUS_BASE				(1600)


//*********************************ICPU***************************************
#define	CLK_ICPU_BASE				(1700)
#define	GATE_SLH_AXI_SI_P_ICPU_QCH	(CLK_ICPU_BASE + 0)
#define	GATE_LH_AXI_MI_IP_ICPU_QCH	(CLK_ICPU_BASE + 1)
#define	GATE_CMU_ICPU_QCH			(CLK_ICPU_BASE + 2)
#define	GATE_D_TZPC_ICPU_QCH		(CLK_ICPU_BASE + 3)
#define	GATE_LH_AXI_SI_IP_ICPU_QCH	(CLK_ICPU_BASE + 4)
#define	GATE_PPMU_D_ICPU_QCH		(CLK_ICPU_BASE + 5)
#define	GATE_SLH_AXI_MI_P_ICPU_QCH	(CLK_ICPU_BASE + 6)
#define	GATE_SYSREG_ICPU_QCH		(CLK_ICPU_BASE + 7)
#define	GATE_ICPU_QCH_CPU0			(CLK_ICPU_BASE + 8)
#define	GATE_ICPU_QCH_PERI			(CLK_ICPU_BASE + 9)

#define	DOUT_DIV_CLK_ICPU_NOCP		(CLK_ICPU_BASE + 10)
#define	DOUT_DIV_CLK_ICPU_PCLKDBG	(CLK_ICPU_BASE + 11)

//*********************************S2D****************************************
#define	CLK_S2D_BASE				(1800)

//*********************************VTS****************************************
#define	CLK_VTS_BASE						(1900)
#define	GATE_CMU_VTS_QCH					(CLK_VTS_BASE + 0)
#define	GATE_GPIO_VTS_QCH					(CLK_VTS_BASE + 1)
#define	GATE_SYSREG_VTS_QCH					(CLK_VTS_BASE + 2)
#define	GATE_TIMER_VTS_QCH					(CLK_VTS_BASE + 3)
#define	GATE_WDT_VTS_QCH					(CLK_VTS_BASE + 4)

#define	DOUT_DIV_CLKVTS_AUD_DMIC			(CLK_VTS_BASE + 5)
#define	DOUT_DIV_CLK_VTS_NOC				(CLK_VTS_BASE + 6)
#define	DOUT_DIV_CLK_VTS_DMIC_IF			(CLK_VTS_BASE + 7)
#define	DOUT_DIV_CLK_VTS_DMIC_IF_DIV2		(CLK_VTS_BASE + 8)
#define	DOUT_DIV_CLK_VTS_SERIAL_LIF			(CLK_VTS_BASE + 9)
#define	DOUT_DIV_CLK_VTS_SERIAL_LIF_CORE	(CLK_VTS_BASE + 10)

#define	MOUT_MUX_VTS_DMIC_AUD				(CLK_VTS_BASE + 11)
#define	MOUT_MUX_CLK_VTS_NOC				(CLK_VTS_BASE + 12)
#define	MOUT_MUX_CLKCMU_VTS_NOC_USER		(CLK_VTS_BASE + 13)
#define	MOUT_MUX_CLKCMU_VTS_RCO_USER		(CLK_VTS_BASE + 14)
#define	MOUT_MUX_CLKCMU_AUD_DMIC_BUS_USER	(CLK_VTS_BASE + 15)
#define	DOUT_DIV_VTS_DMIC_AUD				(CLK_VTS_BASE + 16)


//*********************************DSU****************************************
#define	CLK_DSU_BASE						(2000)
#define	GATE_DSU_CMU_DSU_QCH				(CLK_DSU_BASE + 0)
#define	GATE_HTU_DSU_QCH_PCLK				(CLK_DSU_BASE + 1)
#define	GATE_HTU_DSU_QCH_CLK				(CLK_DSU_BASE + 2)
#define	GATE_LH_AXI_SI_D0_MIF_CPU_QCH		(CLK_DSU_BASE + 3)
#define	GATE_PPC_INSTRRET_CLUSTER0_0_QCH	(CLK_DSU_BASE + 4)
#define	GATE_PPC_INSTRRET_CLUSTER0_1_QCH	(CLK_DSU_BASE + 5)
#define	GATE_PPC_INSTRRUN_CLUSTER0_0_QCH	(CLK_DSU_BASE + 6)
#define	GATE_PPC_INSTRRUN_CLUSTER0_1_QCH	(CLK_DSU_BASE + 7)
#define	GATE_PPMU_CPUCL0_QCH				(CLK_DSU_BASE + 8)
#define	GATE_PPMU_CPUCL1_QCH				(CLK_DSU_BASE + 9)
#define	GATE_SLH_AXI_SI_P_CLUSTER0_QCH		(CLK_DSU_BASE + 10)

#define	DOUT_DIV_CLK_DSU_CLUSTER			(CLK_DSU_BASE + 11)
#define	DOUT_DIV_CLK_CLUSTER0_ACLK			(CLK_DSU_BASE + 12)
#define	DOUT_DIV_CLK_CLUSTER0_ATCLK			(CLK_DSU_BASE + 13)
#define	DOUT_DIV_CLK_CLUSTER0_PCLK			(CLK_DSU_BASE + 14)
#define	DOUT_DIV_CLK_CLUSTER0_PERIPHCLK		(CLK_DSU_BASE + 15)

//*********************************DPU****************************************
#define	CLK_DPU_BASE					(2100)
#define	GATE_CLUSTER0_QCH_PDBGCLK		(CLK_DPU_BASE + 0)
#define	GATE_DPU_QCH_DPU_DMA			(CLK_DPU_BASE + 1)
#define	GATE_DPU_QCH_DPU_DPP			(CLK_DPU_BASE + 2)
#define	GATE_DPU_QCH_DPU_C2SERV			(CLK_DPU_BASE + 3)
#define	GATE_DPU_QCH					(CLK_DPU_BASE + 4)
#define	GATE_DPU_CMU_DPU_QCH			(CLK_DPU_BASE + 5)
#define	GATE_D_TZPC_DPU_QCH				(CLK_DPU_BASE + 6)
#define	GATE_LH_AXI_SI_D0_DPU_QCH		(CLK_DPU_BASE + 7)
#define	GATE_LH_AXI_SI_D1_DPU_QCH		(CLK_DPU_BASE + 8)
#define	GATE_PPMU_D0_DPU_QCH			(CLK_DPU_BASE + 9)
#define	GATE_PPMU_D1_DPU_QCH			(CLK_DPU_BASE + 10)
#define	GATE_SLH_AXI_MI_P_DPU_QCH		(CLK_DPU_BASE + 11)

#define	GATE_SYSREG_DPU_QCH				(CLK_DPU_BASE + 12)
#define	GATE_CLUSTER0_QCH_SCLK			(CLK_DPU_BASE + 13)
#define	GATE_CLUSTER0_QCH_ATCLK			(CLK_DPU_BASE + 14)
#define	GATE_CLUSTER0_QCH_GIC			(CLK_DPU_BASE + 15)
#define	GATE_CLUSTER0_QCH_DBG_PD		(CLK_DPU_BASE + 16)
#define	GATE_CLUSTER0_QCH_PCLK			(CLK_DPU_BASE + 17)
#define	GATE_CLUSTER0_QCH_PERIPHCLK		(CLK_DPU_BASE + 18)

#define	UMUX_CLKCMU_DPU_NOC_USER		(CLK_DPU_BASE + 19)
#define	DOUT_DIV_CLK_DPU_NOCP			(CLK_DPU_BASE + 20)
#define	DOUT_DIV_CLK_DPU_DSIM			(CLK_DPU_BASE + 21)


//*********************************G3D****************************************
#define	CLK_G3D_BASE						(2200)
#define	GATE_D_TZPC_G3D_QCH					(CLK_G3D_BASE + 0)
#define	GATE_G3D_CMU_G3D_QCH				(CLK_G3D_BASE + 1)
#define	GATE_HTU_G3D_QCH_CLK				(CLK_G3D_BASE + 3)
#define	GATE_HTU_G3D_QCH_PCLK				(CLK_G3D_BASE + 4)
#define	GATE_LH_AXI_SI_IP_G3D_QCH			(CLK_G3D_BASE + 5)
#define	GATE_LH_AXI_MI_D_G3D_QCH 			(CLK_G3D_BASE + 6)
#define	GATE_PPMU_D_AXI_G3D_QCH				(CLK_G3D_BASE + 7)
#define	GATE_SYSREG_G3D_QCH					(CLK_G3D_BASE + 8)
#define	GATE_CMU_G3DCORE_QCH				(CLK_G3D_BASE + 9)

#define	MOUT_MUX_CLKCMU_G3D_NOCP			(CLK_G3D_BASE + 10)
#define	MOUT_MUX_CLKCMU_G3DCORE_SWITCH_USER	(CLK_G3D_BASE + 11)
#define	MOUT_MUX_CLK_G3DCORE_PLL			(CLK_G3D_BASE + 12)
#define	DOUT_DIV_CLK_G3D_NOCP				(CLK_G3D_BASE + 13)

//*********************************NOCL0*************************************
#define	CLK_NOCL0_BASE				(2300)
#define	GATE_CMU_NOCL0_QCH			(CLK_NOCL0_BASE + 0)
#define	GATE_D_TZPC_NOCL0_QCH		(CLK_NOCL0_BASE + 1)
#define	GATE_ECU_NOCL0_QCH			(CLK_NOCL0_BASE + 2)
#define	GATE_SYSREG_NOCL0_QCH		(CLK_NOCL0_BASE + 3)
#define	GATE_TREX_D_NOCL0_QCH		(CLK_NOCL0_BASE + 4)
#define	GATE_TREX_P_NOCL0_QCH		(CLK_NOCL0_BASE + 5)
#define	GATE_VGEN_D_NOCL0_QCH		(CLK_NOCL0_BASE + 6)
#define	GATE_VGEN_LITE_D_NOCL0_QCH	(CLK_NOCL0_BASE + 7)

#define	DOUT_DIV_CLK_NOCL0_NOCP		(CLK_NOCL0_BASE + 8)


//*********************************PSP*************************************
#define	CLK_PSP_BASE					(2400)
#define	GATE_CMU_PSP_QCH				(CLK_PSP_BASE + 0)
#define	GATE_D_TZPC_PSP_QCH				(CLK_PSP_BASE + 1)
#define	GATE_LH_AXI_MI_D_PSP_QCH		(CLK_PSP_BASE + 2)
#define	GATE_SLH_AXI_MI_P_PSP_QCH		(CLK_PSP_BASE + 3)
#define	GATE_SYSREG_PSP_BLK_QCH			(CLK_PSP_BASE + 4)
#define	GATE_LH_AXI_SI_D_PSP_QCH		(CLK_PSP_BASE + 5)
#define	GATE_LH_AXI_SI_IP_PSP_QCH		(CLK_PSP_BASE + 6)
#define	GATE_SLH_AXI_SI_P_PSP_QCH		(CLK_PSP_BASE + 7)

#define	DOUT_DIV_CLK_PSP_NOCP			(CLK_PSP_BASE + 8)
#define	MOUT_MUX_CLKCMU_PSP_NOC_USER	(CLK_PSP_BASE + 9)

//*********************************DNC*************************************
#define	CLK_DNC_BASE					(2500)
#define	GATE_SYSMMU_S1_DNC_QCH_S0		(CLK_DNC_BASE + 0)
#define	GATE_IP_DNC_QCH					(CLK_DNC_BASE + 1)
#define	GATE_CMU_DNC_QCH				(CLK_DNC_BASE + 2)
#define	GATE_D_TZPC_DNC_QCH				(CLK_DNC_BASE + 3)
#define	GATE_SYSREG_DNC_QCH				(CLK_DNC_BASE + 4)
#define	GATE_VGEN_LITE_D_DNC_QCH		(CLK_DNC_BASE + 5)
#define	GATE_VGEN_D_DNC_QCH				(CLK_DNC_BASE + 6)
#define	GATE_SLH_AXI_SI_P_DNC_QCH		(CLK_DNC_BASE + 7)
#define	GATE_LH_AXI_MI_D0_DNC_QCH		(CLK_DNC_BASE + 8)
#define	GATE_LH_AXI_MI_D1_DNC_QCH		(CLK_DNC_BASE + 9)
#define	GATE_SYSMMU_S0_DNC_QCH_S0		(CLK_DNC_BASE + 10)

#define	UMUX_CLKCMU_DNC_NOC				(CLK_DNC_BASE + 11)
#define	DOUT_DIV_CLK_DNC_NOCP			(CLK_DNC_BASE + 12)

//*********************************SDMA*************************************
#define	CLK_SDMA_BASE					(2600)
#define	GATE_IP_SDMA_WRAP_QCH			(CLK_SDMA_BASE + 0)
#define	GATE_CMU_SDMA_QCH				(CLK_SDMA_BASE + 1)
#define	GATE_D_TZPC_SDMA_QCH			(CLK_SDMA_BASE + 2)
#define	GATE_SYSREG_SDMA_QCH			(CLK_SDMA_BASE + 3)

//*********************************PERIS*************************************
#define	CLK_PERIS_BASE							(2700)
#define	UMUX_CLKCMU_PERIS_NOC					(CLK_PERIS_BASE + 0)
#define	GATE_WDT0_QCH							(CLK_PERIC_BASE + 1)
#define	GATE_WDT1_QCH							(CLK_PERIC_BASE + 2)
#define	GATE_SLH_AXI_SI_P_PERIS_QCH				(CLK_PERIS_BASE + 3)
#define	GATE_SYSREG_PERIS_QCH					(CLK_PERIS_BASE + 4)
#define	GATE_CMU_PERIS_QCH						(CLK_PERIS_BASE + 5)
#define	GATE_D_TZPC_PERIS_QCH					(CLK_PERIS_BASE + 6)
#define	GATE_SPC_PERIS_QCH						(CLK_PERIS_BASE + 7)
#define	GATE_SLH_AXI_MI_P_PERIS_QCH				(CLK_PERIS_BASE + 8)

#define	MOUT_MUX_CLK_PERIS_GIC_USER				(CLK_PERIS_BASE + 9)
#define	DOUT_DIV_CLK_PERIS_OTP					(CLK_PERIS_BASE + 10)
#define	DOUT_DIV_CLK_PERIS_NOCP					(CLK_PERIS_BASE + 11)
#define	OSC_PERIS_OSCCLK_SUB					(CLK_PERIS_BASE + 12)

//*********************************CSTAT*************************************
#define	CLK_CSTAT_BASE							(2800)
#define	GATE_OTF0_CSIS_CSTAT_QCH				(CLK_CSTAT_BASE + 0)
#define	GATE_OTF1_CSIS_CSTAT_QCH				(CLK_CSTAT_BASE + 1)
#define	GATE_OTF2_CSIS_CSTAT_QCH				(CLK_CSTAT_BASE + 2)

//*********************************USI*************************************
#define	CLK_USI_BASE					(2900)
#define	GATE_CMU_USI_QCH				(CLK_USI_BASE + 0)
#define	GATE_D_TZPC_USI_QCH				(CLK_USI_BASE + 1)
#define	GATE_GPIO_USI_QCH_GPIO			(CLK_USI_BASE + 2)
#define	GATE_SLH_AXI_MI_P_USI_QCH		(CLK_USI_BASE + 3)
#define	GATE_SYSREG_USI_QCH				(CLK_USI_BASE + 4)
#define	GATE_USI06_I2C_QCH				(CLK_USI_BASE + 5)
#define	GATE_USI06_USI_QCH				(CLK_USI_BASE + 6)
#define	GATE_USI07_I2C_QCH				(CLK_USI_BASE + 7)
#define	GATE_USI07_USI_QCH				(CLK_USI_BASE + 8)
#define	GATE_USI08_I2C_QCH				(CLK_USI_BASE + 9)

#define	DOUT_DIV_CLK_USI_USI06_USI		(CLK_USI_BASE + 10)
#define	DOUT_DIV_CLK_USI_USI07_USI		(CLK_USI_BASE + 11)
#define	DOUT_DIV_CLK_USI_USI_I2C		(CLK_USI_BASE + 12)



//*********************************CHUB*************************************
#define	CLK_CHUB_BASE							(3000)
#define	GATE_MAILBOX_APM_CHUB_QCH				(CLK_CHUB_BASE + 0)
#define	GATE_MAILBOX_AP_CHUB_QCH				(CLK_CHUB_BASE + 1)
#define	GATE_MAILBOX_CP_CHUB_QCH				(CLK_CHUB_BASE + 2)
#define	GATE_MAILBOX_GNSS_CHUB_QCH				(CLK_CHUB_BASE + 3)
#define	GATE_MAILBOX_WLBT_CHUB_QCH				(CLK_CHUB_BASE + 4)
#define	GATE_CMU_CHUB_QCH						(CLK_CHUB_BASE + 5)
#define	GATE_PWM_CHUB_QCH						(CLK_CHUB_BASE + 6)
#define	GATE_SYSREG_CHUB_QCH					(CLK_CHUB_BASE + 7)
#define	GATE_WDT_CHUB_QCH						(CLK_CHUB_BASE + 8)
#define	GATE_TIMER_CHUB_QCH						(CLK_CHUB_BASE + 9)

#define	DOUT_DIV_CLK_CHUB_I2C					(CLK_CHUB_BASE + 10)
#define	DOUT_DIV_CLK_CHUB_NOC					(CLK_CHUB_BASE + 11)
#define	DOUT_DIV_CLK_CHUB_USI0					(CLK_CHUB_BASE + 12)
#define	DOUT_DIV_CLK_CHUB_USI1					(CLK_CHUB_BASE + 13)
#define	DOUT_DIV_CLK_CHUB_USI2					(CLK_CHUB_BASE + 14)

//*********************************MFC****************************************
#define	CLK_MFD_BASE						(3100)

	/* aoccsis */
#define	CLK_AOCCSIS_BASE					(3200)
#define	GATE_CSIS_PDP_QCH_DMA				(CLK_AOCCSIS_BASE + 0)
#define	GATE_CSIS_PDP_QCH_PDP				(CLK_AOCCSIS_BASE + 1)
#define	GATE_LH_AST_SI_OTF0_CSISCSTAT_QCH	(CLK_AOCCSIS_BASE + 2)
#define	GATE_LH_AST_SI_OTF1_CSISCSTAT_QCH	(CLK_AOCCSIS_BASE + 3)
#define	GATE_LH_AST_SI_OTF2_CSISCSTAT_QCH	(CLK_AOCCSIS_BASE + 4)
#define	GATE_OIS_MCU_TOP_QCH				(CLK_AOCCSIS_BASE + 5)

//*********************************DBGCORE****************************************
#define	CLK_DBGCORE_BASE					(3300)
#define	DOUT_DIV_CLK_DBGCORE_NOC_DIV		(CLK_DBGCORE_BASE + 0)

//*********************************CHUBVTS****************************************
#define	CLK_CHUBVTS_BASE					(3400)
#define	DOUT_DIV_CHUBVTS_NOC				(CLK_CHUBVTS_BASE + 0)
#define	MOUT_MUX_CLK_CHUBVTS_NOC			(CLK_CHUBVTS_BASE + 1)
#define	MOUT_MUX_CLKCMU_CHUBVTS_NOC_USER	(CLK_CHUBVTS_BASE + 2)
#define	MOUT_MUX_CLKCMU_CHUBVTS_RCO_USER	(CLK_CHUBVTS_BASE + 3)

//*********************************MCFP****************************************
#define	CLK_MCFP_BASE	(3500)
#define	GATE_MCFP_QCH	(CLK_MCFP_BASE + 0)

//*********************************MCSC****************************************
#define	CLK_MCSC_BASE	(3600)
#define	GATE_MCSC_QCH	(CLK_MCSC_BASE + 0)

//*********************************UFS****************************************
#define	CLK_UFS_BASE					(3700)
#define	GATE_UFS_EMBD_QCH				(CLK_UFS_BASE + 0)

//*********************************NOCL1B****************************************
#define	CLK_NOCL1B_BASE					(3800)
#define	DOUT_DIV_CLK_NOCL1B_NOCP		(CLK_NOCL1B_BASE + 0)
#define	GATE_CMU_NOCL1B_QCH				(CLK_NOCL1B_BASE + 1)
#define	GATE_D_TZPC_NOCL1B_QCH			(CLK_NOCL1B_BASE + 2)
#define	GATE_ECU_NOCL1B_QCH				(CLK_NOCL1B_BASE + 3)
#define	GATE_SYSREG_NOCL1B_QCH			(CLK_NOCL1B_BASE + 4)
#define	GATE_TREX_D_NOCL1B_QCH			(CLK_NOCL1B_BASE + 5)
#define	GATE_TREX_PPMU_D0_NOCL1B_QCH	(CLK_NOCL1B_BASE + 6)
#define	GATE_TREX_PPMU_D1_NOCL1B_QCH	(CLK_NOCL1B_BASE + 7)

//*********************************SCA****************************************
#define	CLK_SCA_BASE					(3900)
#define	DOUT_DIV_CLK_SCA_NOC_DIV2		(CLK_SCA_BASE + 0)
#define	GATE_PMU_SCA_QCH				(CLK_SCA_BASE + 1)
#define	GATE_CMU_SCA_QCH				(CLK_SCA_BASE + 2)

//*********************************CLKOUT****************************************
#define	CLK_CLKOUT_BASE				(4000)
#define	OSC_AUD						(CLK_CLKOUT_BASE + 0)

/* must be greater than maximal clock id */
#define	CLK_NR_CLKS					(4200 + 1)



#define	ACPM_DVFS_MIF				(0x0B040000)
#define	ACPM_DVFS_INT				(0x0B040001)
#define	ACPM_DVFS_CPUCL0			(0x0B040002)
#define	ACPM_DVFS_CPUCL1			(0x0B040003)
#define	ACPM_DVFS_CPUCL2			(0x0B040004)
#define	ACPM_DVFS_DSU				(0x0B040005)
#define	ACPM_DVFS_NPU				(0x0B040006)
#define	ACPM_DVFS_DNC				(0x0B040007)
#define	ACPM_DVFS_AUD				(0x0B040008)
#define	ACPM_DVFS_CP_CPU			(0x0B040009)
#define	ACPM_DVFS_CP				(0x0B04000A)
#define	ACPM_DVFS_GNSS				(0x0B04000B)
#define	ACPM_DVFS_CP_MCW			(0x0B04000C)
#define ACPM_DVFS_G3D               (0x0B04000D)
#define	ACPM_DVFS_DISP				(0x0B04000E)
#define ACPM_DVFS_MFC               (0x0B04000F)
#define	ACPM_DVFS_INTCAM			(0x0B040010)
#define ACPM_DVFS_ICPU              (0x0B040011)
#define	ACPM_DVFS_CAM				(0x0B040012)
#define	ACPM_DVFS_ISP				(0x0B040013)
#define	ACPM_DVFS_CSIS				(0x0B040014)
#define ACPM_DVFS_WLBT              (0x0B040015)
#define	ACPM_DVFS_INTSCI			(0x0B040016)

#define	EWF_CMU_ALIVE		(0)
#define	EWF_CMU_AUD			(1)
#define	EWF_CMU_NOCL1A		(2)
#define	EWF_CMU_NOCL1B		(3)
#define	EWF_CMU_NOCL1C		(4)
#define	EWF_CMU_BRP			(5)
#define	EWF_CMU_CHUB		(6)
#define	EWF_CMU_CMGP		(7)
#define	EWF_CMU_NOCL0		(8)
#define	EWF_CMU_CPUCL0_GLB	(9)
#define	EWF_CMU_CPUCL0		(10)
#define	EWF_CMU_CPUCL1		(11)
#define	EWF_CMU_CPUCL2		(12)
#define	EWF_CMU_CSIS		(13)
#define	EWF_CMU_CSTAT		(14)
#define	EWF_CMU_DPUB		(16)
#define	EWF_CMU_DPUF		(17)
#define	EWF_CMU_DSU			(19)
#define	EWF_CMU_G3D			(20)
#define	EWF_CMU_G3DCORE		(21)
#define	EWF_CMU_GNPU0		(22)
#define	EWF_CMU_GNPU1		(23)
#define	EWF_CMU_HSI0		(24)
#define	EWF_CMU_HSI1		(25)
#define	EWF_CMU_LME			(26)
#define	EWF_CMU_M2M			(27)
#define	EWF_CMU_MCSC		(29)
#define	EWF_CMU_MFC0		(30)
#define	EWF_CMU_MFC1		(31)
#define	EWF_CMU_MIF0		(31 + 0)
#define	EWF_CMU_MIF1		(31 + 1)
#define	EWF_CMU_MIF2		(31 + 2)
#define	EWF_CMU_MIF3		(31 + 3)
#define	EWF_CMU_PERIC0		(31 + 4)
#define	EWF_CMU_PERIC1		(31 + 5)
#define	EWF_CMU_PERIC2		(31 + 6)
#define	EWF_CMU_PERIS		(31 + 7)
#define	EWF_CMU_SDMA		(31 + 9)
#define	EWF_CMU_SSP			(31 + 10)
#define	EWF_CMU_STRONG		(31 + 11)
#define	EWF_CMU_DNC			(31 + 12)
#define	EWF_CMU_DSP0		(31 + 13)
#define	EWF_CMU_VTS			(31 + 16)
#define	EWF_CMU_YUVP		(31 + 17)
#define	EWF_CMU_TOP			(31 + 18)
#define	EWF_CMU_DBGCORE		(31 + 19)
#define	EWF_CMU_UFD			(31 + 20)
#define	EWF_CMU_UFS			(31 + 23)
#define	EWF_CMU_CHUBVTS		(31 + 24)
#define	EWF_CMU_ALLCSIS		(31 + 25)
#define	EWF_GRP_CAM			(31 + 26)

#endif	/* _DT_BINDINGS_CLOCK_S5E8855_H */
