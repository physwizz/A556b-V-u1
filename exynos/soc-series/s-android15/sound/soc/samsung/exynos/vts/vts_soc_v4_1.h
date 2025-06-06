/* SPDX-License-Identifier: GPL-2.0-or-later
 * sound/soc/samsung/vts/vts_soc.h
 *
 * ALSA SoC - Samsung VTS driver
 *
 * Copyright (c) 2023 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_VTS_DEP_SOC_V4_1_H
#define __SND_SOC_VTS_DEP_SOC_V4_1_H

/* SYSREG_VTS */
#define VTS_MIF_REQ_OUT			(0x0200)
#define VTS_MIF_REQ_ACK_IN		(0x0204)
#define VTS_DMIC_CLK_CTRL		(0x0408)
#define DMIC_CLK_CON			(0x0434)
#define VTS_ENABLE_DMIC_IF		(0x1000)

/* VTS_DMIC_CLK_CTRL */
#define VTS_CG_STATUS_OFFSET    (5)
#define VTS_CG_STATUS_SIZE      (1)
#define VTS_CLK_ENABLE_OFFSET   (4)
#define VTS_CLK_ENABLE_SIZE     (1)

/* DMIC_CLK_CON */
#define VTS_ENABLE_CLK_GEN_OFFSET       (0)
#define VTS_ENABLE_CLK_GEN_SIZE         (1)
#define VTS_SEL_EXT_DMIC_CLK_OFFSET     (1)
#define VTS_SEL_EXT_DMIC_CLK_SIZE       (1)
#define VTS_ENABLE_CLK_CLK_GEN_OFFSET   (14)
#define VTS_ENABLE_CLK_CLK_GEN_SIZE     (1)

#define VTS_DMIC_ENABLE_DMIC_IF		(0x0000)
#define VTS_DMIC_CONTROL_DMIC_IF	(0x0004)

/* VTS_DMIC_ENABLE_DMIC_IF */
#define VTS_DMIC_PERIOD_DATA2REQ_OFFSET	(16)
#define VTS_DMIC_PERIOD_DATA2REQ_SIZE	(2)

/* VTS_DMIC_CONTROL_DMIC_IF */
#define VTS_DMIC_HPF_EN_OFFSET		(31)
#define VTS_DMIC_HPF_EN_SIZE		(1)
#define VTS_DMIC_LPFEXT_EN_OFFSET	(30)
#define VTS_DMIC_LPFEXT_EN_SIZE 	(1)
#define VTS_DMIC_HPF_SEL_OFFSET		(28)
#define VTS_DMIC_HPF_SEL_SIZE		(1)
#define VTS_DMIC_CPS_SEL_OFFSET		(27)
#define VTS_DMIC_CPS_SEL_SIZE		(1)
#define VTS_DMIC_GAIN_OFFSET		(24)
#define VTS_DMIC_1DB_GAIN_OFFSET	(21)
#define VTS_DMIC_GAIN_SIZE		(3)
#define VTS_DMIC_CPSEXT_EN_OFFSET	(20)
#define VTS_DMIC_CPSEXT_EN_SIZE 	(1)
#define VTS_DMIC_DMIC_SEL_OFFSET	(18)
#define VTS_DMIC_DMIC_SEL_SIZE		(1)
#define VTS_DMIC_RCH_EN_OFFSET		(17)
#define VTS_DMIC_RCH_EN_SIZE		(1)
#define VTS_DMIC_LCH_EN_OFFSET		(16)
#define VTS_DMIC_LCH_EN_SIZE		(1)
#define VTS_DMIC_SYS_SEL_OFFSET		(12)
#define VTS_DMIC_SYS_SEL_SIZE		(2)
#define VTS_DMIC_POLARITY_CLK_OFFSET	(10)
#define VTS_DMIC_POLARITY_CLK_SIZE	(1)
#define VTS_DMIC_POLARITY_OUTPUT_OFFSET	(9)
#define VTS_DMIC_POLARITY_OUTPUT_SIZE	(1)
#define VTS_DMIC_POLARITY_INPUT_OFFSET	(8)
#define VTS_DMIC_POLARITY_INPUT_SIZE	(1)
#define VTS_DMIC_OVFW_CTRL_OFFSET	(4)
#define VTS_DMIC_OVFW_CTRL_SIZE		(1)
#define VTS_DMIC_CIC_SEL_OFFSET		(0)
#define VTS_DMIC_CIC_SEL_SIZE		(1)

/* CM4 */
#define VTS_CM_R(x)			(0x0010 + (x * 0x4))
#define VTS_CM_PC			(0x0004)
#define GPR_DUMP_CNT			(23) /* guided from manual */

/* Interrupt number VTS -> AP */
#define VTS_IRQ_VTS_ERROR                (0)
#define VTS_IRQ_VTS_BOOT_COMPLETED       (1)
#define VTS_IRQ_VTS_IPC_RECEIVED         (2)
#define VTS_IRQ_VTS_VOICE_TRIGGERED      (3)
#define VTS_IRQ_VTS_PERIOD_ELAPSED       (4)
#define VTS_IRQ_VTS_REC_PERIOD_ELAPSED   (5)
#define VTS_IRQ_VTS_DBGLOG_BUFZERO       (6)
#define VTS_IRQ_VTS_INTERNAL_REC_ELAPSED (7)
#define VTS_IRQ_VTS_AUDIO_DUMP           (8)
#define VTS_IRQ_VTS_LOG_DUMP             (9)
#define VTS_IRQ_VTS_STATUS               (10)
#define VTS_IRQ_VTS_SLIF_DUMP            (11)
#define VTS_IRQ_VTS_CP_WAKEUP            (15)

/* Interrupt number AP -> VTS */
#define VTS_IRQ_AP_IPC_RECEIVED          (16)
#define VTS_IRQ_AP_SET_DRAM_BUFFER       (17)
#define VTS_IRQ_AP_START_RECOGNITION     (18)
#define VTS_IRQ_AP_STOP_RECOGNITION      (19)
#define VTS_IRQ_AP_START_COPY            (20)
#define VTS_IRQ_AP_STOP_COPY             (21)
#define VTS_IRQ_AP_SET_MODE              (22)
#define VTS_IRQ_AP_POWER_DOWN            (23)
#define VTS_IRQ_AP_TARGET_SIZE           (24)
#define VTS_IRQ_AP_SET_REC_BUFFER        (25)
#define VTS_IRQ_AP_START_REC             (26)
#define VTS_IRQ_AP_STOP_REC              (27)
#define VTS_IRQ_AP_GET_VERSION           (28)
#define VTS_IRQ_AP_RESTART_RECOGNITION   (29)
#define VTS_IRQ_AP_SET_BARGEIN_MODE      (30)
#define VTS_IRQ_AP_COMMAND               (31)


#define VTS_DMIC_IF_ENABLE_DMIC_IF		(0x1000)
#define VTS_DMIC_IF_ENABLE_DMIC_AUD0		(0x6)
#define VTS_DMIC_IF_ENABLE_DMIC_AUD1		(0x7)
#define VTS_DMIC_IF_ENABLE_DMIC_AUD2		(0x8)

#define VTS_IRQ_COUNT	(11)

#define VTS_BAAW_BASE			(0x60000000)
/* VTS_BAAW_BASE / VTS_BAAW_DRAM_DIV = Config value in baaw guide doc */
#define VTS_BAAW_DRAM_DIV	   (0x10)

#define SICD_SOC_DOWN_OFFSET	(0x18C)
#define SICD_MIF_DOWN_OFFSET	(0x19C)

#define DMA_BUF_SIZE (0x50000)
#define DMA_BUF_TRI_OFFSET      (DMA_BUF_SIZE * VTS_TRIGGRE)
#define DMA_BUF_REC_OFFSET      (DMA_BUF_SIZE * VTS_RECORD)
#define DMA_BUF_INTERNAL_OFFSET (DMA_BUF_SIZE * VTS_INTERNAL)
#define DMA_BUF_NS_L_OFFSET     (DMA_BUF_SIZE * VTS_NS_L)
#define DMA_BUF_NS_R_OFFSET     (DMA_BUF_SIZE * VTS_NS_R)
#define DMA_BUF_BYTES_MAX       (DMA_BUF_SIZE * VTS_DMA_CNT)
#define PERIOD_BYTES_MIN (SZ_4)
#define PERIOD_BYTES_MAX (DMA_BUF_SIZE)

#define VTS_BUF_PERIOD_SIZE 0x140
#define VTS_BUF_PERIOD_COUNT 0x800

#define SOUND_MODEL_SIZE_MAX (SZ_32K)
#define SOUND_MODEL_COUNT (3)

/* DRAM for copying VTS firmware logs */
#define LOG_BUFFER_BYTES_MAX	   (0x2000)
#define VTS_SRAMLOG_MSGS_OFFSET	   (0x00057800)
#define VTS_SRAMLOG_SIZE_MAX	   (SZ_4K)  /* SZ_4K : 0x1000 */
#define VTS_SRAM_TIMELOG_SIZE_MAX  (SZ_1K)  /* SZ_1K : 0x400 */
#define VTS_SRAM_EVENTLOG_SIZE_MAX (VTS_SRAMLOG_SIZE_MAX - VTS_SRAM_TIMELOG_SIZE_MAX)  /* SZ_3K : 0xC00 */

/* VTS firmware version information offset */
#define VTSFW_VERSION_OFFSET	(0xac)
#define DETLIB_VERSION_OFFSET	(0xa8)

#define PAD_RETENTION_VTS_OPTION (0x1A60)  // VTS_OUT : PAD__RTO
#define VTS_CPU_CONFIGURATION	(0x3540)
#define VTS_CPU_STATUS		(0x3544)
#define VTS_CPU_LOCAL_PWR_CFG	 (0x00000001)

#define LIMIT_IN_JIFFIES (msecs_to_jiffies(1000))
#define DMIC_CLK_RATE (768000)
#define VTS_TRIGGERED_TIMEOUT_MS (5000)

#define VTS_SYS_CLOCK_MAX	(393216000)
#define DEF_VTS_PCM_DUMP

#define VTS_ERR_HARD_FAULT	(0x1)
#define VTS_ERR_BUS_FAULT	(0x3)

#define MIC_IN_CH_NORMAL	(2)
#define MIC_IN_CH_WITH_ABOX_REC (2)

#define VTS_ITMON_NAME "CHUBVTS(VTS)" /* refer to bootloader/platform/s5e8835/debug/itmon.c */

#define VTS_SRAM_SZ 0xC0000

#endif /* __SND_SOC_VTS_DEP_SOC_V4_1_H */

