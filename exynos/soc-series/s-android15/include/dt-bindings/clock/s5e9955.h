/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for S5E9955 clock controller.
*/

#ifndef _DT_BINDINGS_CLOCK_S5E9955_H
#define _DT_BINDINGS_CLOCK_S5E9955_H

	/* osc */
#define	NONE	(0 + 0)
#define	OSCCLK1	(0 + 1)
#define	OSCCLK2	(0 + 2)

	/* alive */
#define	CLK_ALIVE_BASE	(10)
#define	GATE_MCT_ALIVE_QCH	(CLK_ALIVE_BASE + 0)
#define	DOUT_CLKALIVE_CHUBVTS_NOC	(CLK_ALIVE_BASE + 1)
#define	DOUT_CLKALIVE_CHUB_PERI	(CLK_ALIVE_BASE + 2)
#define GATE_GREBEINTEGRATION1_QCH_GREBE (CLK_ALIVE_BASE + 3)

	/* aoccsis */
#define	CLK_AOCCSIS_BASE	(100)
#define	GATE_CSIS_PDP_QCH_DMA	(CLK_AOCCSIS_BASE + 0)
#define	GATE_CSIS_PDP_QCH_PDP	(CLK_AOCCSIS_BASE + 1)
#define	GATE_LH_AST_SI_OTF0_CSISCSTAT_QCH	(CLK_AOCCSIS_BASE + 2)
#define	GATE_LH_AST_SI_OTF1_CSISCSTAT_QCH	(CLK_AOCCSIS_BASE + 3)
#define	GATE_LH_AST_SI_OTF2_CSISCSTAT_QCH	(CLK_AOCCSIS_BASE + 4)
#define	GATE_LH_AST_SI_OTF3_CSISCSTAT_QCH	(CLK_AOCCSIS_BASE + 5)
#define GATE_OIS_MCU_TOP_QCH			(CLK_AOCCSIS_BASE + 6)

	/* aud */
#define	CLK_AUD_BASE	(200)
#define	UMUX_CLKVTS_AUD_DMIC	(CLK_AUD_BASE + 0)
#define	GATE_ABOX_QCH_BCLK_DSIF	(CLK_AUD_BASE + 2)
#define	GATE_ABOX_QCH_BCLK0	(CLK_AUD_BASE + 3)
#define	GATE_ABOX_QCH_BCLK1	(CLK_AUD_BASE + 4)
#define	GATE_ABOX_QCH_BCLK2	(CLK_AUD_BASE + 5)
#define	GATE_ABOX_QCH_BCLK3	(CLK_AUD_BASE + 6)
#define	GATE_ABOX_QCH_BCLK4	(CLK_AUD_BASE + 7)
#define	GATE_ABOX_QCH_BCLK5	(CLK_AUD_BASE + 8)
#define	GATE_ABOX_QCH_BCLK6	(CLK_AUD_BASE + 9)
#define	MOUT_MUX_CLK_AUD_PCMC	(CLK_AUD_BASE + 10)
#define	MOUT_MUX_CLK_AUD_SCLK	(CLK_AUD_BASE + 11)
#define	MOUT_CLK_AUD_UAIF6	(CLK_AUD_BASE + 12)
#define	MOUT_CLK_AUD_UAIF0	(CLK_AUD_BASE + 13)
#define	MOUT_CLK_AUD_UAIF1	(CLK_AUD_BASE + 14)
#define	MOUT_CLK_AUD_UAIF2	(CLK_AUD_BASE + 15)
#define	MOUT_CLK_AUD_UAIF3	(CLK_AUD_BASE + 16)
#define	MOUT_CLK_AUD_UAIF4	(CLK_AUD_BASE + 17)
#define	MOUT_CLK_AUD_UAIF5	(CLK_AUD_BASE + 18)
#define	UMUX_CP_PCMC_CLK	(CLK_AUD_BASE + 19)
#define	MOUT_CLK_AUD_SERIAL_LIF	(CLK_AUD_BASE + 20)
#define	DOUT_DIV_CLK_AUD_DSIF	(CLK_AUD_BASE + 21)
#define	DOUT_DIV_CLK_AUD_UAIF0	(CLK_AUD_BASE + 22)
#define	DOUT_DIV_CLK_AUD_UAIF1	(CLK_AUD_BASE + 23)
#define	DOUT_DIV_CLK_AUD_UAIF2	(CLK_AUD_BASE + 24)
#define	DOUT_DIV_CLK_AUD_UAIF3	(CLK_AUD_BASE + 25)
#define	DOUT_DIV_CLK_AUD_NOC	(CLK_AUD_BASE + 26)
#define	DOUT_DIV_CLK_AUD_CNT	(CLK_AUD_BASE + 27)
#define	DOUT_DIV_CLK_AUD_UAIF4	(CLK_AUD_BASE + 28)
#define	DOUT_DIV_CLK_AUD_UAIF5	(CLK_AUD_BASE + 29)
#define	DOUT_DIV_CLK_AUD_UAIF6	(CLK_AUD_BASE + 30)
#define	DOUT_DIV_CLK_AUD_SERIAL_LIF	(CLK_AUD_BASE + 31)

	/* byrp */
#define	CLK_BYRP_BASE	(100)
#define	GATE_SIPU_BYRP_QCH		(CLK_BYRP_BASE + 0)

	/* cmgp */
#define	CLK_CMGP_BASE	(300)
#define	GATE_I2C_CMGP2_QCH	(CLK_CMGP_BASE + 1)
#define	GATE_I2C_CMGP3_QCH	(CLK_CMGP_BASE + 2)
#define	GATE_I2C_CMGP4_QCH	(CLK_CMGP_BASE + 3)
#define	GATE_I2C_CMGP5_QCH	(CLK_CMGP_BASE + 4)
#define	GATE_I2C_CMGP6_QCH	(CLK_CMGP_BASE + 5)
#define	GATE_SPI_I2C_CMGP0_QCH	(CLK_CMGP_BASE + 6)
#define	GATE_SPI_I2C_CMGP1_QCH	(CLK_CMGP_BASE + 7)
#define	GATE_USI_CMGP0_QCH	(CLK_CMGP_BASE + 8)
#define	GATE_USI_CMGP1_QCH	(CLK_CMGP_BASE + 9)
#define	GATE_USI_CMGP2_QCH	(CLK_CMGP_BASE + 10)
#define	GATE_USI_CMGP3_QCH	(CLK_CMGP_BASE + 11)
#define	GATE_USI_CMGP4_QCH	(CLK_CMGP_BASE + 12)
#define	GATE_USI_CMGP5_QCH	(CLK_CMGP_BASE + 13)
#define	GATE_USI_CMGP6_QCH	(CLK_CMGP_BASE + 14)
#define	DOUT_DIV_CLK_CMGP_USI4	(CLK_CMGP_BASE + 15)
#define	DOUT_DIV_CLK_CMGP_USI1	(CLK_CMGP_BASE + 16)
#define	DOUT_DIV_CLK_CMGP_USI0	(CLK_CMGP_BASE + 17)
#define	DOUT_DIV_CLK_CMGP_USI2	(CLK_CMGP_BASE + 18)
#define	DOUT_DIV_CLK_CMGP_USI3	(CLK_CMGP_BASE + 19)
#define	DOUT_DIV_CLK_CMGP_USI5	(CLK_CMGP_BASE + 20)
#define	DOUT_DIV_CLK_CMGP_USI6	(CLK_CMGP_BASE + 21)
#define	DOUT_DIV_CLK_CMGP_I2C	(CLK_CMGP_BASE + 22)
#define	DOUT_DIV_CLK_CMGP_SPI_I2C0	(CLK_CMGP_BASE + 23)
#define	DOUT_DIV_CLK_CMGP_SPI_I2C1	(CLK_CMGP_BASE + 24)

	/* csis */
#define	CLK_CSIS_BASE	(400)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS0	(CLK_CSIS_BASE + 0)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS1	(CLK_CSIS_BASE + 1)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS2	(CLK_CSIS_BASE + 2)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS3	(CLK_CSIS_BASE + 3)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS4	(CLK_CSIS_BASE + 4)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS5	(CLK_CSIS_BASE + 5)
#define	GATE_MIPI_PHY_LINK_WRAP_QCH_CSIS6	(CLK_CSIS_BASE + 6)

	/* cstat */
#define	CLK_CSTAT_BASE	(450)
#define	GATE_OTF0_CSIS_CSTAT_QCH		(CLK_CSTAT_BASE + 0)
#define	GATE_OTF1_CSIS_CSTAT_QCH		(CLK_CSTAT_BASE + 1)
#define	GATE_OTF2_CSIS_CSTAT_QCH		(CLK_CSTAT_BASE + 2)
#define	GATE_OTF3_CSIS_CSTAT_QCH		(CLK_CSTAT_BASE + 3)

	/* dlfe */
#define	CLK_DLFE_BASE	(460)
#define	GATE_IP_DLFE_QCH	(CLK_DLFE_BASE + 0)

	/* dlne */
#define	CLK_DLNE_BASE	(470)
#define	GATE_IP_DLNE_QCH	(CLK_DLNE_BASE + 0)

	/* dnc */
#define	CLK_DNC_BASE	(500)
#define	UMUX_CLKCMU_DNC_NOC	(CLK_DNC_BASE + 0)
#define	GATE_IP_DNC_QCH	(CLK_DNC_BASE + 1)

	/* dpub */
#define	CLK_DPUB_BASE	(600)
#define	UMUX_CLKCMU_DPUB_NOC	(CLK_DPUB_BASE + 0)
#define	DOUT_DIV_CLK_DPUB_OSCCLK_DSIM	(CLK_DPUB_BASE + 1)

	/* dsp */
#define	CLK_DSP_BASE	(700)
#define	GATE_IP_DSP_QCH	(CLK_DSP_BASE + 0)

	/* g3d */
#define	CLK_G3D_BASE	(800)
#define	GATE_BG3D_PWRCTL_QCH	(CLK_G3D_BASE + 0)

	/* gnpu */
#define	CLK_GNPU_BASE	(850)
#define	GATE_GNPU_QCH	(CLK_GNPU_BASE + 0)

	/* snpu */
#define	CLK_SNPU_BASE	(860)
#define	GATE_SNPU_QCH	(CLK_SNPU_BASE + 0)

	/* unpu */
#define	CLK_UNPU_BASE	(870)
#define	GATE_UNPU_QCH	(CLK_UNPU_BASE + 0)

	/* hsi0 */
#define	CLK_HSI0_BASE	(900)
#define	UMUX_CLKCMU_HSI0_DPOSC	(CLK_HSI0_BASE + 0)
#define	UMUX_CLKCMU_HSI0_USB32DRD	(CLK_HSI0_BASE + 1)
#define	MOUT_CLK_HSI0_USB32DRD	(CLK_HSI0_BASE + 2)
#define	GATE_USB32DRD_QCH_S_LINK	(CLK_HSI0_BASE + 3)
#define	UMUX_CLKCMU_HSI0_NOC	(CLK_HSI0_BASE + 4)

	/* ufs */
#define	CLK_UFS_BASE	(1000)
#define	GATE_UFS_EMBD_QCH	(CLK_UFS_BASE + 0)

	/* hsi1 */
#define	CLK_HSI1_BASE	(1100)

	/* lme */
#define	CLK_LME_BASE	(1200)
#define	GATE_LME_QCH_0	(CLK_LME_BASE + 0)

	/* m2m */
#define	CLK_M2M_BASE	(1300)
#define UMUX_CLKCMU_M2M_NOC	(CLK_M2M_BASE + 0)
#define GATE_M2M_QCH	(CLK_M2M_BASE + 1)
#define GATE_M2M_FG	(CLK_M2M_BASE + 2)

	/* mcfp */
#define	CLK_MCFP_BASE	(1350)
#define	GATE_MCFP_QCH	(CLK_MCFP_BASE + 0)

	/* mcsc */
#define	CLK_MCSC_BASE	(1400)
#define	GATE_MCSC_QCH	(CLK_MCSC_BASE + 0)

	/* mif */
#define	CLK_MIF_BASE	(1500)
#define	MUX_MIF_DDRPHY2X	(CLK_MIF_BASE + 0)

	/* pdma */
#define	CLK_PDMA_BASE	(1600)
#define	GATE_PDMA_QCH	(CLK_PDMA_BASE + 0)

	/* nocl1b */
#define	CLK_NOCL1B_BASE	(1650)
#define	UMUX_CLKCMU_NOCL1B_NOC1	(CLK_NOCL1B_BASE + 0)

	/* nocl2a */
#define	CLK_NOCL2A_BASE	(1660)
#define	UMUX_CLKCMU_NOCL2A_NOC2 (CLK_NOCL2A_BASE + 0)

	/* peric0 */
#define	CLK_PERIC0_BASE	(1700)
#define	GATE_PERIC0_CMU_PERIC0_QCH	(CLK_PERIC0_BASE + 0)
#define	DOUT_DIV_CLK_PERIC0_USI04	(CLK_PERIC0_BASE + 1)
#define	DOUT_DIV_CLK_PERIC0_I2C		(CLK_PERIC0_BASE + 2)
#define	DOUT_DIV_CLK_PERIC0_DBG_UART	(CLK_PERIC0_BASE + 3)
#define	DOUT_DIV_CLK_PERIC0_USI22_SPI_I2C	(CLK_PERIC0_BASE + 4)

	/* peric1 */
#define	CLK_PERIC1_BASE	(1800)
#define	GATE_PERIC1_CMU_PERIC1_QCH	(CLK_PERIC1_BASE + 0)
#define	DOUT_DIV_CLK_PERIC1_UART_BT	(CLK_PERIC1_BASE + 1)
#define	DOUT_DIV_CLK_PERIC1_I2C		(CLK_PERIC1_BASE + 2)
#define	DOUT_DIV_CLK_PERIC1_USI07	(CLK_PERIC1_BASE + 3)
#define	DOUT_DIV_CLK_PERIC1_USI08	(CLK_PERIC1_BASE + 4)
#define	DOUT_DIV_CLK_PERIC1_USI09	(CLK_PERIC1_BASE + 5)
#define	DOUT_DIV_CLK_PERIC1_USI10	(CLK_PERIC0_BASE + 6)
#define	DOUT_DIV_CLK_PERIC1_USI07_SPI_I2C	(CLK_PERIC1_BASE + 7)
#define	DOUT_DIV_CLK_PERIC1_USI08_SPI_I2C	(CLK_PERIC1_BASE + 8)

	/* peric2 */
#define	CLK_PERIC2_BASE	(1900)
#define	GATE_PERIC2_CMU_PERIC2_QCH	(CLK_PERIC2_BASE + 0)
#define	GATE_PWM_QCH	(CLK_PERIC2_BASE + 1)
#define	DOUT_DIV_CLK_PERIC2_I2C	(CLK_PERIC2_BASE + 2)
#define	DOUT_DIV_CLK_PERIC2_USI00	(CLK_PERIC2_BASE + 3)
#define	DOUT_DIV_CLK_PERIC2_USI01	(CLK_PERIC2_BASE + 4)
#define	DOUT_DIV_CLK_PERIC2_USI02	(CLK_PERIC2_BASE + 5)
#define	DOUT_DIV_CLK_PERIC2_USI03	(CLK_PERIC2_BASE + 6)
#define	DOUT_DIV_CLK_PERIC2_USI05	(CLK_PERIC2_BASE + 7)
#define	DOUT_DIV_CLK_PERIC2_USI06	(CLK_PERIC2_BASE + 8)
#define	DOUT_DIV_CLK_PERIC2_USI11	(CLK_PERIC2_BASE + 9)
#define	DOUT_DIV_CLK_PERIC2_USI00_SPI_I2C	(CLK_PERIC2_BASE + 11)
#define	DOUT_DIV_CLK_PERIC2_USI01_SPI_I2C	(CLK_PERIC2_BASE + 12)

	/* peris */
#define	CLK_PERIS_BASE	(2000)
#define	UMUX_CLKCMU_PERIS_NOC	(CLK_PERIS_BASE + 0)
#define	GATE_MCT_QCH	(CLK_PERIS_BASE + 1)
#define	GATE_WDT0_QCH	(CLK_PERIS_BASE + 2)
#define	GATE_WDT1_QCH	(CLK_PERIS_BASE + 3)

	/* sdma */
#define	CLK_SDMA_BASE	(2100)
#define	GATE_IP_SDMA_WRAP_QCH		(CLK_SDMA_BASE + 0)
#define	GATE_IP_SDMA_WRAP_QCH_2		(CLK_SDMA_BASE + 1)

	/* top */
#define	CLK_TOP_BASE	(2200)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK0	(CLK_TOP_BASE + 0)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK1	(CLK_TOP_BASE + 1)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK2	(CLK_TOP_BASE + 2)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK3	(CLK_TOP_BASE + 3)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK4	(CLK_TOP_BASE + 4)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK5	(CLK_TOP_BASE + 5)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK6	(CLK_TOP_BASE + 6)
#define	GATE_DFTMUX_CMU_QCH_CIS_CLK7	(CLK_TOP_BASE + 7)
#define	DOUT_DIV_CLKCMU_AUD_AUDIF0	(CLK_TOP_BASE + 8)
#define	DOUT_DIV_CLKCMU_AUD_AUDIF1	(CLK_TOP_BASE + 9)
#define	DOUT_DIV_CLKCMU_CIS_CLK0	(CLK_TOP_BASE + 10)
#define	DOUT_DIV_CLKCMU_CIS_CLK1	(CLK_TOP_BASE + 11)
#define	DOUT_DIV_CLKCMU_CIS_CLK2	(CLK_TOP_BASE + 12)
#define	DOUT_DIV_CLKCMU_CIS_CLK3	(CLK_TOP_BASE + 13)
#define	DOUT_DIV_CLKCMU_CIS_CLK4	(CLK_TOP_BASE + 14)
#define	DOUT_DIV_CLKCMU_CIS_CLK5	(CLK_TOP_BASE + 15)
#define	DOUT_DIV_CLKCMU_CIS_CLK6	(CLK_TOP_BASE + 16)
#define	DOUT_DIV_CLKCMU_CIS_CLK7	(CLK_TOP_BASE + 17)
#define	POUT_SHARED3_D1	(CLK_TOP_BASE + 18)
#define	POUT_SHARED4_D1	(CLK_TOP_BASE + 19)
#define	DOUT_CLKCMU_UFS_UFS_EMBD	(CLK_TOP_BASE + 20)
#define	DOUT_CLKCMU_HSI0_NOC	(CLK_TOP_BASE + 21)

	/* vts */
#define	CLK_VTS_BASE	(2300)
#define	UMUX_CLKCMU_VTS_DMIC_USER	(CLK_VTS_BASE + 0)
#define	MOUT_CLK_VTS_DMIC	(CLK_VTS_BASE + 1)
#define	MOUT_CLKALIVE_VTS_NOC_USER	(CLK_VTS_BASE + 2)
#define	MOUT_CLKALIVE_VTS_RCO_USER	(CLK_VTS_BASE + 3)
#define	DOUT_DIV_CLK_VTS_DMIC_IF	(CLK_VTS_BASE + 4)
#define	DOUT_DIV_CLK_VTS_DMIC_IF_DIV2	(CLK_VTS_BASE + 5)
#define	DOUT_DIV_CLK_VTS_SERIAL_LIF	(CLK_VTS_BASE + 6)
#define	DOUT_DIV_CLK_VTS_CPU	(CLK_VTS_BASE + 7)
#define	DOUT_DIV_CLKVTS_AUD_DMIC	(CLK_VTS_BASE + 8)

	/* yuvp */
#define	CLK_YUVP_BASE	(2400)
#define	GATE_YUVP_QCH	(CLK_YUVP_BASE + 0)

	/* mfc */
#define CLK_MFC_BASE	(2500)
#define UMUX_CLKCMU_MFC_MFC	(CLK_MFC_BASE + 0)

	/* mfd */
#define CLK_MFD_BASE	(2600)
#define UMUX_CLKCMU_MFD_MFD	(CLK_MFD_BASE + 0)
#define GATE_MFD		(CLK_MFD_BASE + 1)

	/* clkout */
#define	CLK_CLKOUT_BASE	(2700)
#define	OSC_AUD	(CLK_CLKOUT_BASE + 0)

       /* CPUCL0_GLB */
#define CLK_CPUCL0_GLB_BASE    (2800)
#define MOUT_CLKCMU_CPUCL0_DBG_NOC_USER                (CLK_CPUCL0_GLB_BASE + 0)

	/* icpu */
#define CLK_ICPU_BASE    (2850)
#define GATE_ICPU_QCH_CPU0		(CLK_ICPU_BASE + 0)
#define GATE_ICPU_QCH_PERI		(CLK_ICPU_BASE + 1)

	/* mlsc */
#define CLK_MLSC_BASE    (2900)
#define GATE_MLSC_QCH		(CLK_MLSC_BASE + 0)

	/* msnr */
#define CLK_MSNR_BASE    (2950)
#define GATE_MSNR_QCH		(CLK_MSNR_BASE + 0)

	/* mtnr */
#define CLK_MTNR_BASE    (3000)
#define GATE_MTNR_QCH		(CLK_MTNR_BASE + 0)

	/* rgbp */
#define CLK_RGBP_BASE    (3050)
#define GATE_RGBP_QCH		(CLK_RGBP_BASE + 0)

/* must be greater than maximal clock id */
#define	CLK_NR_CLKS	(3100 + 1)

#define ACPM_DVFS_MIF				(0x0B040000)
#define ACPM_DVFS_INT				(0x0B040001)
#define ACPM_DVFS_CPUCL0			(0x0B040002)
#define ACPM_DVFS_CPUCL1			(0x0B040003)
#define ACPM_DVFS_CPUCL2			(0x0B040004)
#define ACPM_DVFS_CPUCL3			(0x0B040005)
#define ACPM_DVFS_NPU				(0x0B040006)
#define ACPM_DVFS_PSP				(0x0B040007)
#define ACPM_DVFS_DSU				(0x0B040008)
#define ACPM_DVFS_CP_CPU			(0x0B040009)
#define ACPM_DVFS_CP				(0x0B04000A)
#define ACPM_DVFS_AUD				(0x0B04000B)
#define ACPM_DVFS_CP_MCW			(0x0B04000C)
#define ACPM_DVFS_G3D				(0x0B04000D)
#define ACPM_DVFS_INTCAM			(0x0B04000E)
#define ACPM_DVFS_CAM				(0x0B04000F)
#define ACPM_DVFS_DISP				(0x0B040010)
#define ACPM_DVFS_CSIS				(0x0B040011)
#define ACPM_DVFS_ISP				(0x0B040012)
#define ACPM_DVFS_MFC				(0x0B040013)
#define ACPM_DVFS_MFD				(0x0B040014)
#define ACPM_DVFS_INTSCI			(0x0B040015)
#define ACPM_DVFS_ICPU				(0x0B040016)
#define ACPM_DVFS_DSP				(0x0B040017)
#define ACPM_DVFS_DNC				(0x0B040018)
#define ACPM_DVFS_GNSS				(0x0B040019)
#define ACPM_DVFS_ALIVE				(0x0B04001A)
#define ACPM_DVFS_CHUB				(0x0B04001B)
#define ACPM_DVFS_VTS				(0x0B04001C)
#define ACPM_DVFS_HSI0				(0x0B04001D)
#define ACPM_DVFS_UFD				(0x0B04001E)
#define ACPM_DVFS_CHUBVTS			(0x0B04001F)
#define ACPM_DVFS_UNPU				(0x0B040020)

#define EWF_CMU_ALIVE		(0)
#define EWF_CMU_AUD		(1)
#define EWF_CMU_NOCL1A		(2)
#define EWF_CMU_NOCL1B		(3)
#define EWF_CMU_NOCL1C		(4)
#define EWF_CMU_BRP		(5)
#define EWF_CMU_CHUB		(6)
#define EWF_CMU_CMGP		(7)
#define EWF_CMU_NOCL0		(8)
#define EWF_CMU_CPUCL0_GLB		(9)
#define EWF_CMU_CPUCL0		(10)
#define EWF_CMU_CPUCL1		(11)
#define EWF_CMU_CPUCL2		(12)
#define EWF_CMU_CSIS		(13)
#define EWF_CMU_CSTAT		(14)
#define EWF_CMU_DPUB		(16)
#define EWF_CMU_DPUF		(17)
#define EWF_CMU_DSU		(19)
#define EWF_CMU_G3D		(20)
#define EWF_CMU_G3DCORE		(21)
#define EWF_CMU_GNPU0		(22)
#define EWF_CMU_GNPU1		(23)
#define EWF_CMU_HSI0		(24)
#define EWF_CMU_HSI1		(25)
#define EWF_CMU_LME		(26)
#define EWF_CMU_M2M		(27)
#define EWF_CMU_MCSC		(29)
#define EWF_CMU_MFC0		(30)
#define EWF_CMU_MFC1		(31)
#define EWF_CMU_MIF0		(31 + 0)
#define EWF_CMU_MIF1		(31 + 1)
#define EWF_CMU_MIF2		(31 + 2)
#define EWF_CMU_MIF3		(31 + 3)
#define EWF_CMU_PERIC0		(31 + 4)
#define EWF_CMU_PERIC1		(31 + 5)
#define EWF_CMU_PERIC2		(31 + 6)
#define EWF_CMU_PERIS		(31 + 7)
#define EWF_CMU_SDMA		(31 + 9)
#define EWF_CMU_SSP		(31 + 10)
#define EWF_CMU_STRONG		(31 + 11)
#define EWF_CMU_DNC		(31 + 12)
#define EWF_CMU_DSP0		(31 + 13)
#define EWF_CMU_VTS		(31 + 16)
#define EWF_CMU_YUVP		(31 + 17)
#define EWF_CMU_TOP		(31 + 18)
#define EWF_CMU_DBGCORE		(31 + 19)
#define EWF_CMU_UFD		(31 + 20)
#define EWF_CMU_UFS		(31 + 23)
#define EWF_CMU_CHUBVTS		(31 + 24)
#define EWF_CMU_ALLCSIS		(31 + 25)
#define EWF_GRP_CAM		(31 + 26)

#endif	/* _DT_BINDINGS_CLOCK_S5E9955_H */
