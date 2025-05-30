/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ALSA SoC - Samsung Abox SoC layer for version 1 and 2
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_SOC_2_H
#define __SND_SOC_ABOX_SOC_2_H

/* System */
#define ABOX_IP_INDEX			0x0000
#define ABOX_VERSION			0x0004
#define ABOX_SYSPOWER_CTRL		0x0010
#define ABOX_SYSPOWER_STATUS		0x0014
#define ABOX_SYSTEM_CONFIG0		0x0020
#define ABOX_REMAP_MASK			0x0024
#define ABOX_REMAP_ADDR			0x0028
#define ABOX_DYN_CLOCK_OFF		0x0030
#define ABOX_DYN_CLOCK_OFF1		0x0034
#define ABOX_QCHANNEL_DISABLE		0x0038
#define ABOX_ROUTE_CTRL0		0x0040
#define ABOX_ROUTE_CTRL1		0x0044
#define ABOX_ROUTE_CTRL2		0x0048
#define ABOX_TICK_DIV_RATIO		0x0050
#define ABOX_MO_CTRL			0x0054
/* ABOX_VERSION */
#define ABOX_VERSION_H			31
#define ABOX_VERSION_L			8
#define ABOX_VERSION_MASK		ABOX_FLD(VERSION)
/* ABOX_SYSPOWER_CTRL */
#define ABOX_SYSPOWER_CTRL_H		0
#define ABOX_SYSPOWER_CTRL_L		0
#define ABOX_SYSPOWER_CTRL_MASK		ABOX_FLD(SYSPOWER_CTRL)
/* ABOX_SYSPOWER_STATUS */
#define ABOX_SYSPOWER_STATUS_H		0
#define ABOX_SYSPOWER_STATUS_L		0
#define ABOX_SYSPOWER_STATUS_MASK	ABOX_FLD(SYSPOWER_STATUS)
/* ABOX_DYN_CLOCK_OFF */
#define ABOX_DYN_CLOCK_OFF_H		30
#define ABOX_DYN_CLOCK_OFF_L		0
#define ABOX_DYN_CLOCK_OFF_MASK		ABOX_FLD(DYN_CLOCK_OFF)
/* ABOX_QCHANNEL_DISABLE */
#define ABOX_QCHANNEL_DISABLE_BASE	0
#define ABOX_QCHANNEL_DISABLE_ITV	1
#define ABOX_QCHANNEL_DISABLE_H(x)	ABOX_H(QCHANNEL_DISABLE, 0, x)
#define ABOX_QCHANNEL_DISABLE_L(x)	ABOX_L(QCHANNEL_DISABLE, 0, x)
#define ABOX_QCHANNEL_DISABLE_MASK(x)	ABOX_FLD_X(QCHANNEL_DISABLE, x)
/* ABOX_ROUTE_CTRL0 */
#if (ABOX_SOC_VERSION(2, 0, 0) <= CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_ROUTE_SPUSM_H		27
#define ABOX_ROUTE_SPUSM_L		24
#define ABOX_ROUTE_SPUSM_MASK		ABOX_FLD(ROUTE_SPUSM)
#endif
#define ABOX_ROUTE_DSIF_H		23
#define ABOX_ROUTE_DSIF_L		20
#define ABOX_ROUTE_DSIF_MASK		ABOX_FLD(ROUTE_DSIF)
#define ABOX_ROUTE_UAIF_SPK_BASE	0
#define ABOX_ROUTE_UAIF_SPK_ITV		4
#define ABOX_ROUTE_UAIF_SPK_H(x)	ABOX_H(ROUTE_UAIF_SPK, 3, x)
#define ABOX_ROUTE_UAIF_SPK_L(x)	ABOX_L(ROUTE_UAIF_SPK, 0, x)
#define ABOX_ROUTE_UAIF_SPK_MASK(x)	ABOX_FLD_X(ROUTE_UAIF_SPK, x)
/* ABOX_ROUTE_CTRL1 */
#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_ROUTE_SPUSM_H		19
#define ABOX_ROUTE_SPUSM_L		16
#define ABOX_ROUTE_SPUSM_MASK		ABOX_FLD(ROUTE_SPUSM)
#endif
#define ABOX_ROUTE_NSRC_BASE		0
#define ABOX_ROUTE_NSRC_ITV		4
#define ABOX_ROUTE_NSRC_H(x)		ABOX_H(ROUTE_NSRC, 3, x)
#define ABOX_ROUTE_NSRC_L(x)		ABOX_L(ROUTE_NSRC, 0, x)
#define ABOX_ROUTE_NSRC_MASK(x)		ABOX_FLD_X(ROUTE_NSRC, x)
/* ABOX_ROUTE_CTRL2 */
#define ABOX_RSRC_CONNECTION_TYPE_BASE	28
#define ABOX_RSRC_CONNECTION_TYPE_ITV	1
#define ABOX_RSRC_CONNECTION_TYPE_H(x)	ABOX_H(RSRC_CONNECTION_TYPE, 0, x)
#define ABOX_RSRC_CONNECTION_TYPE_L(x)	ABOX_L(RSRC_CONNECTION_TYPE, 0, x)
#define ABOX_RSRC_CONNECTION_TYPE_MASK(x) ABOX_FLD_X(RSRC_CONNECTION_TYPE, x)
#define ABOX_NSRC_CONNECTION_TYPE_BASE	20
#define ABOX_NSRC_CONNECTION_TYPE_ITV	1
#define ABOX_NSRC_CONNECTION_TYPE_H(x)	ABOX_H(NSRC_CONNECTION_TYPE, 0, x)
#define ABOX_NSRC_CONNECTION_TYPE_L(x)	ABOX_L(NSRC_CONNECTION_TYPE, 0, x)
#define ABOX_NSRC_CONNECTION_TYPE_MASK(x) ABOX_FLD_X(NSRC_CONNECTION_TYPE, x)
#define ABOX_ROUTE_RSRC_BASE		0
#define ABOX_ROUTE_RSRC_ITV		4
#define ABOX_ROUTE_RSRC_H(x)		ABOX_H(ROUTE_RSRC, 3, x)
#define ABOX_ROUTE_RSRC_L(x)		ABOX_L(ROUTE_RSRC, 0, x)
#define ABOX_ROUTE_RSRC_MASK(x)		ABOX_FLD_X(ROUTE_RSRC, x)

/* SPUS */
#define ABOX_SPUS_CTRL0			0x0200
#define ABOX_SPUS_CTRL1			0x0204
#define ABOX_SPUS_CTRL2			0x0208
#define ABOX_SPUS_CTRL3			0x020C
#define ABOX_SPUS_CTRL4			0x0210
#define ABOX_SPUS_CTRL5			0x0214
#define ABOX_SPUS_CTRL_FC1		0x0218
#define ABOX_SPUS_SBANK_RDMA_BASE	0x0220
#define ABOX_SPUS_SBANK_RDMA_ITV	0x0004
#define ABOX_SPUS_SBANK_RDMA(x)		ABOX_SFR(SPUS_SBANK_RDMA, 0x0, x)
#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_SPUS_SBANK_ASRC_BASE	0x0240
#define ABOX_SPUS_SBANK_MIXP_BASE	0x0260
#define ABOX_SPUS_CTRL_SIFS_CNT_BASE	0x0280
#else
#define ABOX_SPUS_SBANK_ASRC_BASE	0x0250
#define ABOX_SPUS_SBANK_MIXP_BASE	0x0280
#define ABOX_SPUS_CTRL_SIFS_CNT_BASE	0x0290
#endif
#define ABOX_SPUS_SBANK_ASRC_ITV	0x0004
#define ABOX_SPUS_CTRL_SIFS_CNT_ITV	0x0004
#define ABOX_SPUS_SBANK_ASRC(x)		ABOX_SFR(SPUS_SBANK_ASRC, 0x0, x)
#define ABOX_SPUS_SBANK_MIXP		ABOX_SPUS_SBANK_MIXP_BASE
#define ABOX_SPUS_CTRL_SIFS_CNT(x)	\
	((x > 3) ? 0x028C : ABOX_SFR(SPUS_CTRL_SIFS_CNT, 0x0, x))
/* ABOX_SPUS_CTRL0 */
#define ABOX_FUNC_CHAIN_SRC_BASE	0
#if (ABOX_SOC_VERSION(2, 0, 1) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_FUNC_CHAIN_SRC_ITV		4
#define ABOX_FUNC_CHAIN_SRC_IN_H(x)	ABOX_H(FUNC_CHAIN_SRC, 3, x)
#define ABOX_FUNC_CHAIN_SRC_IN_L(x)	ABOX_L(FUNC_CHAIN_SRC, 3, x)
#define ABOX_FUNC_CHAIN_SRC_IN_MASK(x)	ABOX_FLD_X(FUNC_CHAIN_SRC_IN, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_H(x)	ABOX_H(FUNC_CHAIN_SRC, 2, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_L(x)	ABOX_L(FUNC_CHAIN_SRC, 1, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_MASK(x)	ABOX_FLD_X(FUNC_CHAIN_SRC_OUT, x)
#else
#define ABOX_FUNC_CHAIN_SRC_ITV		5
#define ABOX_FUNC_CHAIN_SRC_IN_H(x)	ABOX_H(FUNC_CHAIN_SRC, 4, x)
#define ABOX_FUNC_CHAIN_SRC_IN_L(x)	ABOX_L(FUNC_CHAIN_SRC, 4, x)
#define ABOX_FUNC_CHAIN_SRC_IN_MASK(x)	ABOX_FLD_X(FUNC_CHAIN_SRC_IN, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_H(x)	ABOX_H(FUNC_CHAIN_SRC, 3, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_L(x)	ABOX_L(FUNC_CHAIN_SRC, 1, x)
#define ABOX_FUNC_CHAIN_SRC_OUT_MASK(x)	ABOX_FLD_X(FUNC_CHAIN_SRC_OUT, x)
#endif
#define ABOX_FUNC_CHAIN_SRC_ASRC_H(x)	ABOX_H(FUNC_CHAIN_SRC, 0, x)
#define ABOX_FUNC_CHAIN_SRC_ASRC_L(x)	ABOX_L(FUNC_CHAIN_SRC, 0, x)
#define ABOX_FUNC_CHAIN_SRC_ASRC_MASK(x) ABOX_FLD_X(FUNC_CHAIN_SRC_ASRC, x)
/* ABOX_SPUS_CTRL1 */
#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_SIFSM_IN_SEL_H		24
#define ABOX_SIFSM_IN_SEL_L		22
#define ABOX_SIFSM_IN_SEL_MASK		ABOX_FLD(SIFSM_IN_SEL)
#define ABOX_SIFS_OUT_SEL_BASE		13
#define ABOX_SIFS_OUT_SEL_ITV		3
#elif (ABOX_SOC_VERSION(2, 0, 1) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_SIFSM_IN_SEL_H		27
#define ABOX_SIFSM_IN_SEL_L		24
#define ABOX_SIFSM_IN_SEL_MASK		ABOX_FLD(SIFSM_IN_SEL)
#define ABOX_SIFS_OUT_SEL_BASE		8
#define ABOX_SIFS_OUT_SEL_ITV		4
#else
#define ABOX_SIFSM_IN_SEL_H		11
#define ABOX_SIFSM_IN_SEL_L		8
#define ABOX_SIFSM_IN_SEL_MASK		ABOX_FLD(SIFSM_IN_SEL)
#define ABOX_SIFS_OUT_SEL_BASE		8
#define ABOX_SIFS_OUT_SEL_ITV		4
#endif
#define ABOX_SIFS_OUT_SEL_H(x)		ABOX_H(SIFS_OUT_SEL, 2, x)
#define ABOX_SIFS_OUT_SEL_L(x)		ABOX_L(SIFS_OUT_SEL, 0, x)
#define ABOX_SIFS_OUT_SEL_MASK(x)	ABOX_FLD_X(SIFS_OUT_SEL, x)
#define ABOX_MIXP_FORMAT_H		4
#define ABOX_MIXP_FORMAT_L		0
#define ABOX_MIXP_FORMAT_MASK		ABOX_FLD(MIXP_FORMAT)
/* ABOX_SPUS_CTRL2 */
#define ABOX_MIXP_LD_FLUSH_H		1
#define ABOX_MIXP_LD_FLUSH_L		1
#define ABOX_MIXP_LD_FLUSH_MASK		ABOX_FLD(MIXP_LD_FLUSH)
#define ABOX_MIXP_FLUSH_H		0
#define ABOX_MIXP_FLUSH_L		0
#define ABOX_MIXP_FLUSH_MASK		ABOX_FLD(MIXP_FLUSH)
/* ABOX_SPUS_CTRL3 */
#define ABOX_SIFSM_FLUSH_H		2
#define ABOX_SIFSM_FLUSH_L		2
#define ABOX_SIFSM_FLUSH_MASK		ABOX_FLD(SIFSM_FLUSH)
#define ABOX_SIFS_FLUSH_BASE		-1
#define ABOX_SIFS_FLUSH_ITV		1
#define ABOX_SIFS_FLUSH_H(x)		ABOX_H(SIFS_FLUSH, 0, x)
#define ABOX_SIFS_FLUSH_L(x)		ABOX_L(SIFS_FLUSH, 0, x)
#define ABOX_SIFS_FLUSH_MASK(x)		ABOX_FLD_X(SIFS_FLUSH, x)
/* ABOX_SPUS_CTRL4 */
/* ABOX_SPUS_CTRL5 */
#define ABOX_SRC_ASRC_ID_BASE		16
#define ABOX_SRC_ASRC_ID_ITV		4
#define ABOX_SRC_ASRC_ID_H(x)		ABOX_H(SRC_ASRC_ID, 3, x)
#define ABOX_SRC_ASRC_ID_L(x)		ABOX_L(SRC_ASRC_ID, 0, x)
#define ABOX_SRC_ASRC_ID_MASK(x)	ABOX_FLD_X(SRC_ASRC_ID, x)
/* ABOX_SPUS_CTRL_SIFS_CNTx */
#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_SIFS_CNT_VAL_BASE		0
#define ABOX_SIFS_CNT_VAL_ITV		16
#define ABOX_SIFS_CNT_VAL_H(x)		ABOX_H(SIFS_CNT_VAL, 15, x)
#define ABOX_SIFS_CNT_VAL_L(x)		ABOX_L(SIFS_CNT_VAL, 0, x)
#else
#define ABOX_SIFS_CNT_VAL_BASE		0
#define ABOX_SIFS_CNT_VAL_ITV		32
#define ABOX_SIFS_CNT_VAL_H(x)		ABOX_H(SIFS_CNT_VAL, 23, x)
#define ABOX_SIFS_CNT_VAL_L(x)		ABOX_L(SIFS_CNT_VAL, 0, x)
#endif
#define ABOX_SIFS_CNT_VAL_MASK(x)	ABOX_FLD_X(SIFS_CNT_VAL, x)
/* SPUM */
#define ABOX_SPUM_CTRL0			0x0300
#define ABOX_SPUM_CTRL1			0x0304
#define ABOX_SPUM_CTRL2			0x0308
#define ABOX_SPUM_CTRL3			0x030C
#define ABOX_SPUM_CTRL4			0x0310
#define ABOX_SPUM_SBANK_RSRC_BASE	0x0320
#define ABOX_SPUM_SBANK_RSRC_ITV	0x0004
#define ABOX_SPUM_SBANK_RSRC(x)		ABOX_SFR(SPUM_SBANK_RSRC, 0x0, x)
#define ABOX_SPUM_SBANK_NSRC_BASE	0x0328
#define ABOX_SPUM_SBANK_NSRC_ITV	0x0004
#define ABOX_SPUM_SBANK_NSRC(x)		ABOX_SFR(SPUM_SBANK_NSRC, 0x0, x)
#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_SPUM_SBANK_RECP_BASE	0x0338
#define ABOX_SPUM_SBANK_ASRC_BASE	0x033C
#else
#define ABOX_SPUM_SBANK_RECP_BASE	0x0348
#define ABOX_SPUM_SBANK_ASRC_BASE	0x034C
#endif
#define ABOX_SPUM_SBANK_ASRC_ITV	0x0004
#define ABOX_SPUM_SBANK_RECP		ABOX_SPUM_SBANK_RECP_BASE
#define ABOX_SPUM_SBANK_ASRC(x)		ABOX_SFR(SPUM_SBANK_ASRC, 0x0, x)
/* ABOX_SPUM_CTRL0 */
#define ABOX_FUNC_CHAIN_NSRC_BASE	4
#define ABOX_FUNC_CHAIN_NSRC_ITV	4
#define ABOX_FUNC_CHAIN_NSRC_OUT_H(x)	ABOX_H(FUNC_CHAIN_NSRC, 3, x)
#define ABOX_FUNC_CHAIN_NSRC_OUT_L(x)	ABOX_L(FUNC_CHAIN_NSRC, 3, x)
#define ABOX_FUNC_CHAIN_NSRC_OUT_MASK(x) ABOX_FLD_X(FUNC_CHAIN_NSRC_OUT, x)
#define ABOX_FUNC_CHAIN_NSRC_ASRC_H(x)	ABOX_H(FUNC_CHAIN_NSRC, 0, x)
#define ABOX_FUNC_CHAIN_NSRC_ASRC_L(x)	ABOX_L(FUNC_CHAIN_NSRC, 0, x)
#define ABOX_FUNC_CHAIN_NSRC_ASRC_MASK(x) ABOX_FLD_X(FUNC_CHAIN_NSRC_ASRC, x)
#define ABOX_FUNC_CHAIN_RSRC_RECP_H	1
#define ABOX_FUNC_CHAIN_RSRC_RECP_L	1
#define ABOX_FUNC_CHAIN_RSRC_RECP_MASK	ABOX_FLD(FUNC_CHAIN_RSRC_RECP)
#define ABOX_FUNC_CHAIN_RSRC_ASRC_H	0
#define ABOX_FUNC_CHAIN_RSRC_ASRC_L	0
#define ABOX_FUNC_CHAIN_RSRC_ASRC_MASK	ABOX_FLD(FUNC_CHAIN_RSRC_ASRC)
/* ABOX_SPUM_CTRL1 */
#define ABOX_SIFMS_OUT_SEL_H		18
#define ABOX_SIFMS_OUT_SEL_L		16
#define ABOX_SIFMS_OUT_SEL_MASK		ABOX_FLD(SIFMS_OUT_SEL)
#define ABOX_RECP_SRC_FORMAT_BASE	3
#define ABOX_RECP_SRC_FORMAT_ITV	5
#define ABOX_RECP_SRC_FORMAT_H(x)	ABOX_H(RECP_SRC_FORMAT, 4, x)
#define ABOX_RECP_SRC_FORMAT_L(x)	ABOX_L(RECP_SRC_FORMAT, 0, x)
#define ABOX_RECP_SRC_FORMAT_MASK(x)	ABOX_FLD_X(RECP_SRC_FORMAT, x)
#define ABOX_RECP_SRC_VALID_H		1
#define ABOX_RECP_SRC_VALID_L		0
#define ABOX_RECP_SRC_VALID_MASK	ABOX_FLD(RECP_SRC_VALID)
/* ABOX_SPUM_CTRL2 */
#define ABOX_RECP_LD_FLUSH_H		1
#define ABOX_RECP_LD_FLUSH_L		1
#define ABOX_RECP_LD_FLUSH_MASK		ABOX_FLD(RECP_LD_FLUSH)
#define ABOX_RECP_FLUSH_H		0
#define ABOX_RECP_FLUSH_L		0
#define ABOX_RECP_FLUSH_MASK		ABOX_FLD(RECP_FLUSH)
/* ABOX_SPUM_CTRL3 */
#define ABOX_SIFMS_FLUSH_H		7
#define ABOX_SIFMS_FLUSH_L		7
#define ABOX_SIFMS_FLUSH_MASK		ABOX_FLD(SIFMS_FLUSH)
#define ABOX_SIFM_FLUSH_BASE		0
#define ABOX_SIFM_FLUSH_ITV		1
#define ABOX_SIFM_FLUSH_H(x)		ABOX_H(SIFM_FLUSH, 0, x)
#define ABOX_SIFM_FLUSH_L(x)		ABOX_L(SIFM_FLUSH, 0, x)
#define ABOX_SIFM_FLUSH_MASK(x)		ABOX_FLD_X(SIFM_FLUSH, x)
/* ABOX_SPUM_CTRL4 */
#define ABOX_NSRC_ASRC_ID_BASE		4
#define ABOX_NSRC_ASRC_ID_ITV		4
#define ABOX_NSRC_ASRC_ID_H(x)		ABOX_H(NSRC_ASRC_ID, 3, x)
#define ABOX_NSRC_ASRC_ID_L(x)		ABOX_L(NSRC_ASRC_ID, 0, x)
#define ABOX_NSRC_ASRC_ID_MASK(x)	ABOX_FLD_X(NSRC_ASRC_ID, x)
#define ABOX_RSRC_ASRC_ID_H		2
#define ABOX_RSRC_ASRC_ID_L		0
#define ABOX_RSRC_ASRC_ID_MASK		ABOX_FLD_X(RSRC_ASRC_ID, x)

/* ABOX_SPUS_SBANK_RDMAx */
/* ABOX_SPUS_SBANK_ASRCx */
/* ABOX_SPUM_SBANK_RSRCx */
/* ABOX_SPUM_SBANK_NSRCx */
/* ABOX_SPUM_SBANK_RECP */
/* ABOX_SPUM_SBANK_ASRCx */
#define ABOX_SBANK_SIZE_H		29
#define ABOX_SBANK_SIZE_L		20
#define ABOX_SBANK_SIZE_MASK		ABOX_FLD(SBANK_SIZE)
#define ABOX_SBANK_STR_H		13
#define ABOX_SBANK_STR_L		4
#define ABOX_SBANK_STR_MASK		ABOX_FLD(SBANK_STR)

/* UAIF */
#define ABOX_UAIF_BASE			0x0500
#define ABOX_UAIF_ITV			0x0010
#define ABOX_UAIF_CTRL0(x)		ABOX_SFR(UAIF, 0x0, x)
#define ABOX_UAIF_CTRL1(x)		ABOX_SFR(UAIF, 0x4, x)
#define ABOX_UAIF_STATUS(x)		ABOX_SFR(UAIF, 0xC, x)
/* ABOX_UAIF_CTRL0 */
#define ABOX_START_FIFO_DIFF_MIC_H	31
#define ABOX_START_FIFO_DIFF_MIC_L	28
#define ABOX_START_FIFO_DIFF_MIC_MASK	ABOX_FLD(START_FIFO_DIFF_MIC)
#define ABOX_START_FIFO_DIFF_SPK_H	27
#define ABOX_START_FIFO_DIFF_SPK_L	24
#define ABOX_START_FIFO_DIFF_SPK_MASK	ABOX_FLD(START_FIFO_DIFF_SPK)
#define ABOX_DATA_MODE_H		4
#define ABOX_DATA_MODE_L		4
#define ABOX_DATA_MODE_MASK		ABOX_FLD(DATA_MODE)
#define ABOX_IRQ_MODE_H			3
#define ABOX_IRQ_MODE_L			3
#define ABOX_IRQ_MODE_MASK		ABOX_FLD(IRQ_MODE)
#define ABOX_MODE_H			2
#define ABOX_MODE_L			2
#define ABOX_MODE_MASK			ABOX_FLD(MODE)
#define ABOX_MIC_ENABLE_H		1
#define ABOX_MIC_ENABLE_L		1
#define ABOX_MIC_ENABLE_MASK		ABOX_FLD(MIC_ENABLE)
#define ABOX_SPK_ENABLE_H		0
#define ABOX_SPK_ENABLE_L		0
#define ABOX_SPK_ENABLE_MASK		ABOX_FLD(SPK_ENABLE)
/* ABOX_UAIF_CTRL1 */
#define ABOX_FORMAT_H			28
#define ABOX_FORMAT_L			24
#define ABOX_FORMAT_MASK		ABOX_FLD(FORMAT)
#define ABOX_BCLK_POLARITY_H		23
#define ABOX_BCLK_POLARITY_L		23
#define ABOX_BCLK_POLARITY_MASK		ABOX_FLD(BCLK_POLARITY)
#define ABOX_WS_MODE_H			22
#define ABOX_WS_MODE_L			22
#define ABOX_WS_MODE_MASK		ABOX_FLD(WS_MODE)
#define ABOX_WS_POLAR_H			21
#define ABOX_WS_POLAR_L			21
#define ABOX_WS_POLAR_MASK		ABOX_FLD(WS_POLAR)
#define ABOX_SLOT_MAX_H			20
#define ABOX_SLOT_MAX_L			18
#define ABOX_SLOT_MAX_MASK		ABOX_FLD(SLOT_MAX)
#define ABOX_SBIT_MAX_H			17
#define ABOX_SBIT_MAX_L			12
#define ABOX_SBIT_MAX_MASK		ABOX_FLD(SBIT_MAX)
#define ABOX_VALID_STR_H		11
#define ABOX_VALID_STR_L		6
#define ABOX_VALID_STR_MASK		ABOX_FLD(VALID_STR)
#define ABOX_VALID_END_H		5
#define ABOX_VALID_END_L		0
#define ABOX_VALID_END_MASK		ABOX_FLD(VALID_END)
/* ABOX_UAIF_STATUS */
#define ABOX_ERROR_OF_MIC_H		1
#define ABOX_ERROR_OF_MIC_L		1
#define ABOX_ERROR_OF_MIC_MASK		ABOX_FLD(ERROR_OF_MIC)
#define ABOX_ERROR_OF_SPK_H		0
#define ABOX_ERROR_OF_SPK_L		0
#define ABOX_ERROR_OF_SPK_MASK		ABOX_FLD(ERROR_OF_SPK)

/* DSIF */
#define ABOX_DSIF_CTRL			0x0550
#define ABOX_DSIF_STATUS		0x0554
/* ABOX_DSIF_CTRL */
#define ABOX_DSIF_MODE_H		3
#define ABOX_DSIF_MODE_L		3
#define ABOX_DSIF_MODE_MASK		ABOX_FLD(DSIF_MODE)
#define ABOX_DSIF_BCLK_POLARITY_H	2
#define ABOX_DSIF_BCLK_POLARITY_L	2
#define ABOX_DSIF_BCLK_POLARITY_MASK	ABOX_FLD(DSIF_BCLK_POLARITY)
#define ABOX_ORDER_H			1
#define ABOX_ORDER_L			1
#define ABOX_ORDER_MASK			ABOX_FLD(ORDER)
#define ABOX_ENABLE_H			0
#define ABOX_ENABLE_L			0
#define ABOX_ENABLE_MASK		ABOX_FLD(ENABLE)
/* ABOX_DSIF_STATUS */
#define ABOX_ERROR_H			0
#define ABOX_ERROR_L			0
#define ABOX_ERROR_MASK			ABOX_FLD(ERROR)

/* TIMER */
#define ABOX_TIMER_BASE			0x0600
#define ABOX_TIMER_ITV			0x0020
#define ABOX_TIMER_CTRL0(x)		ABOX_SFR(TIMER, 0x0, x)
#define ABOX_TIMER_CTRL1(x)		ABOX_SFR(TIMER, 0x4, x)
#define ABOX_TIMER_PRESET_LSB(x)	ABOX_SFR(TIMER, 0x8, x)
#define ABOX_TIMER_PRESET_MSB(x)	ABOX_SFR(TIMER, 0xC, x)
#define ABOX_TIMER_CURVALUD_LSB(x)	ABOX_SFR(TIMER, 0x10, x)
#define ABOX_TIMER_CURVALUD_MSB(x)	ABOX_SFR(TIMER, 0x14, x)
/* ABOX_TIMER_CTRL0 */
#define ABOX_TIMER_FLUSH_H		1
#define ABOX_TIMER_FLUSH_L		1
#define ABOX_TIMER_FLUSH_MASK		ABOX_FLD(TIMER_FLUSH)
#define ABOX_TIMER_START_H		0
#define ABOX_TIMER_START_L		0
#define ABOX_TIMER_START_MASK		ABOX_FLD(TIMER_START)
/* ABOX_TIMER_CTRL1 */
#define ABOX_TIMER_MODE_H		0
#define ABOX_TIMER_MODE_L		0
#define ABOX_TIMER_MODE_MASK		ABOX_FLD(TIMER_MODE)

/* RDMA */
#define ABOX_RDMA_BASE			0x1000
#define ABOX_RDMA_ITV			0x0100
#define ABOX_RDMA_CTRL0(x)		ABOX_SFR(RDMA, 0x00, x)
#define ABOX_RDMA_CTRL1(x)		ABOX_SFR(RDMA, 0x04, x)
#define ABOX_RDMA_BUF_STR(x)		ABOX_SFR(RDMA, 0x08, x)
#define ABOX_RDMA_BUF_END(x)		ABOX_SFR(RDMA, 0x0C, x)
#define ABOX_RDMA_BUF_OFFSET(x)		ABOX_SFR(RDMA, 0x10, x)
#define ABOX_RDMA_STR_POINT(x)		ABOX_SFR(RDMA, 0x14, x)
#define ABOX_RDMA_VOL_FACTOR(x)		ABOX_SFR(RDMA, 0x18, x)
#define ABOX_RDMA_VOL_CHANGE(x)		ABOX_SFR(RDMA, 0x1C, x)
#if (ABOX_SOC_VERSION(1, 0, 0) < CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_RDMA_SBANK_LIMIT(x)	ABOX_SFR(RDMA, 0x20, x)
#define ABOX_RDMA_STATUS(x)		ABOX_SFR(RDMA, 0x30, x)
#else
#define ABOX_RDMA_STATUS(x)		ABOX_SFR(RDMA, 0x20, x)
#endif
/* ABOX_RDMA_CTRL0 */
#define ABOX_RDMA_BURST_LEN_H		22
#define ABOX_RDMA_BURST_LEN_L		19
#define ABOX_RDMA_BURST_LEN_MASK	ABOX_FLD(RDMA_BURST_LEN)
#define ABOX_RDMA_ENABLE_H		0
#define ABOX_RDMA_ENABLE_L		0
#define ABOX_RDMA_ENABLE_MASK		ABOX_FLD(RDMA_ENABLE)
/* ABOX_RDMA_STATUS */
#define ABOX_RDMA_PROGRESS_H		31
#define ABOX_RDMA_PROGRESS_L		31
#define ABOX_RDMA_PROGRESS_MASK		ABOX_FLD(RDMA_PROGRESS)
#if (ABOX_SOC_VERSION(2, 0, 1) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_RDMA_RBUF_OFFSET_H		27
#define ABOX_RDMA_RBUF_OFFSET_L		16
#define ABOX_RDMA_RBUF_OFFSET_MASK	ABOX_FLD(RDMA_RBUF_OFFSET)
#else
#define ABOX_RDMA_RBUF_OFFSET_H		27
#define ABOX_RDMA_RBUF_OFFSET_L		12
#define ABOX_RDMA_RBUF_OFFSET_MASK	ABOX_FLD(RDMA_RBUF_OFFSET)
#endif
#define ABOX_RDMA_RBUF_CNT_H		11
#define ABOX_RDMA_RBUF_CNT_L		0
#define ABOX_RDMA_RBUF_CNT_MASK		ABOX_FLD(RDMA_RBUF_CNT)
/* ABOX_RDMA_VOL_FACTOR */
#define ABOX_RDMA_VOL_FACTOR_H		(23)
#define ABOX_RDMA_VOL_FACTOR_L		(0)
#define ABOX_RDMA_VOL_FACTOR_MASK	ABOX_FLD(RDMA_VOL_FACTOR)

/* SPUS ASRC */
#define ABOX_SPUS_ASRC_BASE		0x2000
#define ABOX_SPUS_ASRC_ITV		0x0100
#define ABOX_SPUS_ASRC_CTRL(x)		ABOX_SFR(SPUS_ASRC, 0x0, x)
#define ABOX_SPUS_ASRC_IS_PARA0(x)	ABOX_SFR(SPUS_ASRC, 0x10, x)
#define ABOX_SPUS_ASRC_IS_PARA1(x)	ABOX_SFR(SPUS_ASRC, 0x14, x)
#define ABOX_SPUS_ASRC_OS_PARA0(x)	ABOX_SFR(SPUS_ASRC, 0x18, x)
#define ABOX_SPUS_ASRC_OS_PARA1(x)	ABOX_SFR(SPUS_ASRC, 0x1C, x)
#define ABOX_SPUS_ASRC_DITHER_CTRL(x)	ABOX_SFR(SPUS_ASRC, 0x20, x)
#define ABOX_SPUS_ASRC_SEED_IN(x)	ABOX_SFR(SPUS_ASRC, 0x24, x)
#define ABOX_SPUS_ASRC_SEED_OUT(x)	ABOX_SFR(SPUS_ASRC, 0x28, x)
#define ABOX_SPUS_ASRC_FILTER_CTRL(x)	ABOX_SFR(SPUS_ASRC, 0x2C, x)

/* WDMA */
#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_WDMA_BASE			0x2000
#else
#define ABOX_WDMA_BASE			0x3000
#endif
#define ABOX_WDMA_ITV			0x0100
#define ABOX_WDMA_CTRL(x)		ABOX_SFR(WDMA, 0x00, x)
#define ABOX_WDMA_BUF_STR(x)		ABOX_SFR(WDMA, 0x08, x)
#define ABOX_WDMA_BUF_END(x)		ABOX_SFR(WDMA, 0x0C, x)
#define ABOX_WDMA_BUF_OFFSET(x)		ABOX_SFR(WDMA, 0x10, x)
#define ABOX_WDMA_STR_POINT(x)		ABOX_SFR(WDMA, 0x14, x)
#define ABOX_WDMA_VOL_FACTOR(x)		ABOX_SFR(WDMA, 0x18, x)
#define ABOX_WDMA_VOL_CHANGE(x)		ABOX_SFR(WDMA, 0x1C, x)
#if (ABOX_SOC_VERSION(1, 0, 0) < CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_WDMA_SBANK_LIMIT(x)	ABOX_SFR(WDMA, 0x20, x)
#define ABOX_WDMA_STATUS(x)		ABOX_SFR(WDMA, 0x30, x)
#else
#define ABOX_WDMA_STATUS(x)		ABOX_SFR(WDMA, 0x20, x)
#endif
/* ABOX_WDMA_CTRL */
#define ABOX_WDMA_ENABLE_H		0
#define ABOX_WDMA_ENABLE_L		0
#define ABOX_WDMA_ENABLE_MASK		ABOX_FLD(WDMA_ENABLE)
/* ABOX_WDMA_STATUS */
#define ABOX_WDMA_PROGRESS_H		31
#define ABOX_WDMA_PROGRESS_L		31
#define ABOX_WDMA_PROGRESS_MASK		ABOX_FLD(WDMA_PROGRESS)
#if (ABOX_SOC_VERSION(2, 0, 1) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_WDMA_RBUF_OFFSET_H		27
#define ABOX_WDMA_RBUF_OFFSET_L		16
#define ABOX_WDMA_RBUF_OFFSET_MASK	ABOX_FLD(WDMA_RBUF_OFFSET)
#else
#define ABOX_WDMA_RBUF_OFFSET_H		27
#define ABOX_WDMA_RBUF_OFFSET_L		12
#define ABOX_WDMA_RBUF_OFFSET_MASK	ABOX_FLD(WDMA_RBUF_OFFSET)
#endif
#define ABOX_WDMA_RBUF_CNT_H		11
#define ABOX_WDMA_RBUF_CNT_L		0
#define ABOX_WDMA_RBUF_CNT_MASK		ABOX_FLD(WDMA_RBUF_CNT)


/* SPUM ASRC */
#define ABOX_SPUM_ASRC_BASE		0x4000
#define ABOX_SPUM_ASRC_ITV		0x0100
#define ABOX_SPUM_ASRC_CTRL(x)		ABOX_SFR(SPUM_ASRC, 0x0, x)
#define ABOX_SPUM_ASRC_IS_PARA0(x)	ABOX_SFR(SPUM_ASRC, 0x10, x)
#define ABOX_SPUM_ASRC_IS_PARA1(x)	ABOX_SFR(SPUM_ASRC, 0x14, x)
#define ABOX_SPUM_ASRC_OS_PARA0(x)	ABOX_SFR(SPUM_ASRC, 0x18, x)
#define ABOX_SPUM_ASRC_OS_PARA1(x)	ABOX_SFR(SPUM_ASRC, 0x1C, x)
#define ABOX_SPUM_ASRC_DITHER_CTRL(x)	ABOX_SFR(SPUM_ASRC, 0x20, x)
#define ABOX_SPUM_ASRC_SEED_IN(x)	ABOX_SFR(SPUM_ASRC, 0x24, x)
#define ABOX_SPUM_ASRC_SEED_OUT(x)	ABOX_SFR(SPUM_ASRC, 0x28, x)
#define ABOX_SPUM_ASRC_FILTER_CTRL(x)	ABOX_SFR(SPUM_ASRC, 0x2C, x)

/* ABOX_SPUS_ASRCx_CTRL */
/* ABOX_SPUM_ASRCx_CTRL */
#define ABOX_ASRC_OS_SOURCE_SEL_H	25
#define ABOX_ASRC_OS_SOURCE_SEL_L	23
#define ABOX_ASRC_OS_SOURCE_SEL_MASK	ABOX_FLD(ASRC_OS_SOURCE_SEL)
#define ABOX_ASRC_IS_SOURCE_SEL_H	22
#define ABOX_ASRC_IS_SOURCE_SEL_L	20
#define ABOX_ASRC_IS_SOURCE_SEL_MASK	ABOX_FLD(ASRC_IS_SOURCE_SEL)
#define ABOX_ASRC_TICKDIV_H		19
#define ABOX_ASRC_TICKDIV_L		16
#define ABOX_ASRC_TICKDIV_MASK		ABOX_FLD(ASRC_TICKDIV)
#define ABOX_ASRC_TICKNUM_H		15
#define ABOX_ASRC_TICKNUM_L		8
#define ABOX_ASRC_TICKNUM_MASK		ABOX_FLD(ASRC_TICKNUM)
#define ABOX_ASRC_BIT_WIDTH_H	7
#define ABOX_ASRC_BIT_WIDTH_L	6
#define ABOX_ASRC_BIT_WIDTH_MASK	ABOX_FLD(ASRC_BIT_WIDTH)
#define ABOX_ASRC_DCMF_RATIO_H		5
#define ABOX_ASRC_DCMF_RATIO_L		4
#define ABOX_ASRC_DCMF_RATIO_MASK	ABOX_FLD(ASRC_DCMF_RATIO)
#define ABOX_ASRC_OVSF_RATIO_H		3
#define ABOX_ASRC_OVSF_RATIO_L		2
#define ABOX_ASRC_OVSF_RATIO_MASK	ABOX_FLD(ASRC_OVSF_RATIO)
#define ABOX_ASRC_OS_SYNC_MODE_H	1
#define ABOX_ASRC_OS_SYNC_MODE_L	1
#define ABOX_ASRC_OS_SYNC_MODE_MASK	ABOX_FLD(ASRC_OS_SYNC_MODE)
#define ABOX_ASRC_IS_SYNC_MODE_H	0
#define ABOX_ASRC_IS_SYNC_MODE_L	0
#define ABOX_ASRC_IS_SYNC_MODE_MASK	ABOX_FLD(ASRC_IS_SYNC_MODE)
/* ABOX_SPUS_ASRCx_IS_PARA0 */
/* ABOX_SPUM_ASRCx_IS_PARA0 */
#define ABOX_ASRC_IS_DEFAULT_H		16
#define ABOX_ASRC_IS_DEFAULT_L		0
#define ABOX_ASRC_IS_DEFAULT_MASK	ABOX_FLD(ASRC_IS_DEFAULT)
/* ABOX_SPUS_ASRCx_IS_PARA1 */
/* ABOX_SPUM_ASRCx_IS_PARA1 */
#define ABOX_ASRC_IS_TPERIOD_LIMIT_H	16
#define ABOX_ASRC_IS_TPERIOD_LIMIT_L	0
#define ABOX_ASRC_IS_TPERIOD_LIMIT_MASK	ABOX_FLD(ASRC_IS_TPERIOD_LIMIT)
/* ABOX_SPUS_ASRCx_OS_PARA0 */
/* ABOX_SPUM_ASRCx_OS_PARA0 */
#define ABOX_ASRC_OS_DEFAULT_H		16
#define ABOX_ASRC_OS_DEFAULT_L		0
#define ABOX_ASRC_OS_DEFAULT_MASK	ABOX_FLD(ASRC_OS_DEFAULT)
/* ABOX_SPUS_ASRCx_OS_PARA1 */
/* ABOX_SPUM_ASRCx_OS_PARA1 */
#define ABOX_ASRC_OS_TPERIOD_LIMIT_H	16
#define ABOX_ASRC_OS_TPERIOD_LIMIT_L	0
#define ABOX_ASRC_OS_TPERIOD_LIMIT_MASK	ABOX_FLD(ASRC_OS_TPERIOD_LIMIT)
/* ABOX_SPUS_ASRCx_DITHER_CTRL */
/* ABOX_SPUM_ASRCx_DITHER_CTRL */
#define ABOX_ASRC_DITHER_IN_ON_H	25
#define ABOX_ASRC_DITHER_IN_ON_L	25
#define ABOX_ASRC_DITHER_IN_ON_MASK	ABOX_FLD(ASRC_DITHER_IN_ON)
#define ABOX_ASRC_DITHER_IN_STRENGTH_H	24
#define ABOX_ASRC_DITHER_IN_STRENGTH_L	18
#define ABOX_ASRC_DITHER_IN_STRENGTH_MASK ABOX_FLD(ASRC_DITHER_IN_STRENGTH)
#define ABOX_ASRC_DITHER_IN_WIDTH_H	17
#define ABOX_ASRC_DITHER_IN_WIDTH_L	16
#define ABOX_ASRC_DITHER_IN_WIDTH_MASK	ABOX_FLD(ASRC_DITHER_IN_WIDTH)
#define ABOX_ASRC_DITHER_OUT_ON_H	9
#define ABOX_ASRC_DITHER_OUT_ON_L	9
#define ABOX_ASRC_DITHER_OUT_ON_MASK	ABOX_FLD(ASRC_DITHER_OUT_ON)
#define ABOX_ASRC_DITHER_OUT_STRENGTH_H	8
#define ABOX_ASRC_DITHER_OUT_STRENGTH_L	2
#define ABOX_ASRC_DITHER_OUT_STRENGTH_MASK ABOX_FLD(ASRC_DITHER_OUT_STRENGTH)
#define ABOX_ASRC_DITHER_OUT_WIDTH_H	1
#define ABOX_ASRC_DITHER_OUT_WIDTH_L	0
#define ABOX_ASRC_DITHER_OUT_WIDTH_MASK	ABOX_FLD(ASRC_DITHER_OUT_WIDTH)
/* ABOX_SPUS_ASRCx_SEED_IN */
/* ABOX_SPUM_ASRCx_SEED_IN */
#define ABOX_ASRC_DIHER_IN_SEED_H	31
#define ABOX_ASRC_DIHER_IN_SEED_L	0
#define ABOX_ASRC_DIHER_IN_SEED_MASK	ABOX_FLD(ASRC_DITHER_IN_SEED)
/* ABOX_SPUS_ASRCx_SEED_OUT */
/* ABOX_SPUM_ASRCx_SEED_OUT */
#define ABOX_ASRC_DIHER_OUT_SEED_H	31
#define ABOX_ASRC_DIHER_OUT_SEED_L	0
#define ABOX_ASRC_DIHER_OUT_SEED_MASK	ABOX_FLD(ASRC_DITHER_OUT_SEED)
/* ABOX_SPUS_ASRCx_FILTER_CTRL */
/* ABOX_SPUM_ASRCx_FILTER_CTRL */
#define ABOX_ASRC_APF_COEF_SEL_H	6
#define ABOX_ASRC_APF_COEF_SEL_L	6
#define ABOX_ASRC_APF_COEF_SEL_MASK	ABOX_FLD(ASRC_APF_COEF_SEL)
#define ABOX_ASRC_APF_FILTER_SEL_H	5
#define ABOX_ASRC_APF_FILTER_SEL_L	5
#define ABOX_ASRC_APF_FILTER_SEL_MASK	ABOX_FLD(ASRC_APF_FILTER_SEL)
#define ABOX_ASRC_TRF_ON_H		4
#define ABOX_ASRC_TRF_ON_L		4
#define ABOX_ASRC_TRF_ON_MASK		ABOX_FLD(ASRC_TRF_ON)
#define ABOX_ASRC_TRF_GAIN_H		3
#define ABOX_ASRC_TRF_GAIN_L		0
#define ABOX_ASRC_TRF_GAIN_MASK		ABOX_FLD(ASRC_TRF_GAIN)

/* CA7 */
#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_CA7_R_BASE		0x2C00
#else
#define ABOX_CA7_R_BASE		0x5000
#endif
#define ABOX_CA7_R_ITV		0x0004
#define ABOX_CA7_R(x)		ABOX_SFR(CA7_R, 0x0, x)
#define ABOX_CA7_PC		ABOX_CA7_R(15)

/* CA32 */
#define ABOX_CA32_CORE0_BASE	0x5000
#define ABOX_CA32_CORE0_ITV	0x0004
#define ABOX_CA32_CORE0_R(x)	ABOX_SFR(CA32_CORE0, 0x0, x)
#define ABOX_CA32_CORE0_PC	ABOX_CA32_CORE0_R(31)
#define ABOX_CA32_CORE1_BASE	0x5080
#define ABOX_CA32_CORE1_ITV	0x0004
#define ABOX_CA32_CORE1_R(x)	ABOX_SFR(CA32_CORE1, 0x0, x)
#define ABOX_CA32_CORE1_PC	ABOX_CA32_CORE1_R(31)
#define ABOX_CA32_STATUS	0x5100

/* APF_COEF */
#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_APF_BASE		0x3000
#else
#define ABOX_APF_BASE		0x7000
#endif
#define ABOX_APF_ITV		0x0100
#define ABOX_COEF_2EVEN0(x)	ABOX_SFR(APF, 0x00, x)
#define ABOX_COEF_2EVEN1(x)	ABOX_SFR(APF, 0x04, x)
#define ABOX_COEF_2EVEN2(x)	ABOX_SFR(APF, 0x08, x)
#define ABOX_COEF_2EVEN3(x)	ABOX_SFR(APF, 0x0C, x)
#define ABOX_COEF_2EVEN4(x)	ABOX_SFR(APF, 0x10, x)
#define ABOX_COEF_2EVEN5(x)	ABOX_SFR(APF, 0x14, x)
#define ABOX_COEF_2EVEN6(x)	ABOX_SFR(APF, 0x18, x)
#define ABOX_COEF_2EVEN7(x)	ABOX_SFR(APF, 0x1C, x)
#define ABOX_COEF_2EVEN8(x)	ABOX_SFR(APF, 0x20, x)
#define ABOX_COEF_2EVEN9(x)	ABOX_SFR(APF, 0x24, x)
#define ABOX_COEF_2ODD0(x)	ABOX_SFR(APF, 0x28, x)
#define ABOX_COEF_2ODD1(x)	ABOX_SFR(APF, 0x2C, x)
#define ABOX_COEF_2ODD2(x)	ABOX_SFR(APF, 0x30, x)
#define ABOX_COEF_2ODD3(x)	ABOX_SFR(APF, 0x34, x)
#define ABOX_COEF_2ODD4(x)	ABOX_SFR(APF, 0x38, x)
#define ABOX_COEF_2ODD5(x)	ABOX_SFR(APF, 0x3C, x)
#define ABOX_COEF_2ODD6(x)	ABOX_SFR(APF, 0x40, x)
#define ABOX_COEF_2ODD7(x)	ABOX_SFR(APF, 0x44, x)
#define ABOX_COEF_2ODD8(x)	ABOX_SFR(APF, 0x48, x)
#define ABOX_COEF_4EVEN0(x)	ABOX_SFR(APF, 0x4C, x)
#define ABOX_COEF_4EVEN1(x)	ABOX_SFR(APF, 0x50, x)
#define ABOX_COEF_4EVEN2(x)	ABOX_SFR(APF, 0x54, x)
#define ABOX_COEF_4ODD0(x)	ABOX_SFR(APF, 0x58, x)
#define ABOX_COEF_4ODD1(x)	ABOX_SFR(APF, 0x5C, x)
#define ABOX_COEF_8EVEN0(x)	ABOX_SFR(APF, 0x60, x)
#define ABOX_COEF_8EVEN1(x)	ABOX_SFR(APF, 0x64, x)
#define ABOX_COEF_8EVEN2(x)	ABOX_SFR(APF, 0x68, x)
#define ABOX_COEF_8ODD0(x)	ABOX_SFR(APF, 0x6C, x)
#define ABOX_COEF_8ODD1(x)	ABOX_SFR(APF, 0x70, x)

#if (ABOX_SOC_VERSION(2, 0, 0) > CONFIG_SND_SOC_SAMSUNG_ABOX_VERSION)
#define ABOX_MAX_REGISTERS		(0x3170)
#else
#define ABOX_MAX_REGISTERS		(0x7170)
#endif

/* SYSREG */
#if IS_ENABLED(CONFIG_SOC_EXYNOS9820)
#define ABOX_SYSREG_L2_CACHE_CON	(0x0000)
#define ABOX_SYSREG_MISC_CON		(0x0504)
#else
#define ABOX_SYSREG_L2_CACHE_CON	(0x0328)
#define ABOX_SYSREG_MISC_CON		(0x032C)
#endif

#define AUD_PLL_RATE_HZ_FOR_48000	(1179648000)
#define AUD_PLL_RATE_HZ_FOR_44100	(1083801600)

#define TIMER_RATE 26000000ULL
#define TIMER_MOD 2000000
/* Predefined rate of MUX_CLK_AUD_UAIF for slave mode */
#define UAIF_RATE_MUX_SLAVE 100000000

#define COUNT_SIFS 5
#define COUNT_SPUS 12
#define COUNT_SIFM 8
#define COUNT_SPUM 8
#define COUNT_UDMA_SIFS 0
#define COUNT_UDMA_SIFM 0

enum abox_gic_target {
	ABOX_GIC_CORE0,
	ABOX_GIC_CP,
	ABOX_GIC_AP,
	ABOX_GIC_CORE1,
};

enum qchannel {
	ABOX_CCLK_CORE,
	ABOX_ACLK,
	ABOX_BCLK_UAIF0,
	ABOX_BCLK_UAIF1,
	ABOX_BCLK_UAIF2,
	ABOX_BCLK_UAIF3,
	ABOX_BCLK_UAIF4,
	ABOX_BCLK_UAIF5,
	ABOX_BCLK_UAIF6,
	ABOX_BCLK_RESERVED,
	ABOX_BCLK_DSIF,
	ABOX_CCLK_ASB = 16,
	ABOX_PCMC_CLK,
	ABOX_XCLK0,
	ABOX_XCLK1,
	ABOX_XCLK2,
	ABOX_CCLK_ACP,
};

#if IS_ENABLED(CONFIG_SOC_EXYNOS9820)
enum abox_irq {
	SGI_IPC_RECEIVED	= 0x0,
	SGI_IPC_SYSTEM		= 0x1,
	SGI_IPC_PCMPLAYBACK	= 0x2,
	SGI_IPC_PCMCAPTURE	= 0x3,
	SGI_IPC_OFFLOAD		= 0x4,
	SGI_IPC_ERAP		= 0x5,
	SGI_FLUSH		= 0x7,
	SGI_WDMA0_BUF_FULL	= 0x8,
	SGI_WDMA1_BUF_FULL	= 0x9,
	SGI_IPC_ABOX_CONFIG	= 0xA,
	SGI_RDMA0_BUF_EMPTY	= 0xB,
	SGI_RDMA1_BUF_EMPTY	= 0xC,
	SGI_RDMA2_BUF_EMPTY	= 0xD,
	SGI_RDMA3_BUF_EMPTY	= 0xE,
	SGI_ABOX_MSG		= 0xF,
	IRQ_CA32_0		= 0x20,
	IRQ_CA32_1,
	IRQ_TIMER0_DONE,
	IRQ_TIMER1_DONE,
	IRQ_TIMER2_DONE,
	IRQ_TIMER3_DONE,
	IRQ_RDMA0_BUF_EMPTY,
	IRQ_RDMA1_BUF_EMPTY,
	IRQ_RDMA2_BUF_EMPTY,
	IRQ_RDMA3_BUF_EMPTY,
	IRQ_RDMA4_BUF_EMPTY,
	IRQ_RDMA5_BUF_EMPTY,
	IRQ_RDMA6_BUF_EMPTY,
	IRQ_RDMA7_BUF_EMPTY,
	IRQ_RDMA8_BUF_EMPTY,
	IRQ_RDMA9_BUF_EMPTY,
	IRQ_RDMA10_BUF_EMPTY,
	IRQ_RDMA11_BUF_EMPTY,
	IRQ_RDMA0_FADE_DONE,
	IRQ_RDMA1_FADE_DONE,
	IRQ_RDMA2_FADE_DONE,
	IRQ_RDMA3_FADE_DONE,
	IRQ_RDMA4_FADE_DONE,
	IRQ_RDMA5_FADE_DONE,
	IRQ_RDMA6_FADE_DONE,
	IRQ_RDMA7_FADE_DONE,
	IRQ_RDMA8_FADE_DONE,
	IRQ_RDMA9_FADE_DONE,
	IRQ_RDMA10_FADE_DONE,
	IRQ_RDMA11_FADE_DONE,
	IRQ_WDMA0_BUF_FULL,
	IRQ_WDMA1_BUF_FULL,
	IRQ_WDMA2_BUF_FULL,
	IRQ_WDMA3_BUF_FULL,
	IRQ_WDMA4_BUF_FULL,
	IRQ_WDMA5_BUF_FULL,
	IRQ_WDMA6_BUF_FULL,
	IRQ_WDMA7_BUF_FULL,
	IRQ_WDMA0_FADE_DONE,
	IRQ_WDMA1_FADE_DONE,
	IRQ_WDMA2_FADE_DONE,
	IRQ_WDMA3_FADE_DONE,
	IRQ_WDMA4_FADE_DONE,
	IRQ_WDMA5_FADE_DONE,
	IRQ_WDMA6_FADE_DONE,
	IRQ_WDMA7_FADE_DONE,
	IRQ_UAIF0_SPEAKER,
	IRQ_UAIF0_MIC,
	IRQ_UAIF1_SPEAKER,
	IRQ_UAIF1_MIC,
	IRQ_UAIF2_SPEAKER,
	IRQ_UAIF2_MIC,
	IRQ_UAIF3_SPEAKER,
	IRQ_UAIF3_MIC,
	IRQ_DSIF_OVERFLOW,
	IRQ_PCM_COUNTER0,
	IRQ_PCM_COUNTER1,
	IRQ_PCM_COUNTER2,
	IRQ_PCM_COUNTER3,
	IRQ_PCM_COUNTER4,
	IRQ_PCM_COUNTER5,
	IRQ_IRQC,
	IRQ_WDT,
	IRQ_RDMA0_ERR,
	IRQ_RDMA1_ERR,
	IRQ_RDMA2_ERR,
	IRQ_RDMA3_ERR,
	IRQ_RDMA4_ERR,
	IRQ_RDMA5_ERR,
	IRQ_RDMA6_ERR,
	IRQ_RDMA7_ERR,
	IRQ_RDMA8_ERR,
	IRQ_RDMA9_ERR,
	IRQ_RDMA10_ERR,
	IRQ_RDMA11_ERR,
	IRQ_WDMA0_ERR,
	IRQ_WDMA1_ERR,
	IRQ_WDMA2_ERR,
	IRQ_WDMA3_ERR,
	IRQ_WDMA4_ERR,
	IRQ_WDMA5_ERR,
	IRQ_WDMA6_ERR,
	IRQ_WDMA7_ERR,
	IRQ_CA32_AXIERR,
	IRQ_CA32_PMUIRQ_0,
	IRQ_CA32_PMUIRQ_1,
	IRQ_USB1_INTR_IN,
	IRQ_USB2_INTR_IN,
	IRQ_USB3_INTR_IN,
	IRQ_USB4_INTR_IN,
	IRQ_GPIO_INTR_IN,
	IRQ_MBOX_VTSS_INTR_IN,
	IRQ_INTR_UAIF0_HOLD,
	IRQ_INTR_UAIF0_RESUME,
	IRQ_INTR_UAIF1_HOLD,
	IRQ_INTR_UAIF1_RESUME,
	IRQ_INTR_UAIF2_HOLD,
	IRQ_INTR_UAIF2_RESUME,
	IRQ_INTR_UAIF3_HOLD,
	IRQ_INTR_UAIF3_RESUME,
	IRQ_INTR_SIFM_FADE_DONE,
};
#else
#error "irq table isn't defined"
#endif

#endif /* __SND_SOC_ABOX_SOC_2_H */
