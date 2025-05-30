/*
 * s2se910-mfd.h - PMIC mfd driver for the S2SE910
 *
 *  Copyright (C) 2024 Samsung Electrnoics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_MFD_S2SE910_H
#define __LINUX_MFD_S2SE910_H

#include <linux/i2c.h>
#include <linux/pmic/s2p.h>
#include <linux/pmic/s2se910-register.h>

#if IS_ENABLED(CONFIG_S2SE910_ADC)
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/iio/consumer.h>
#endif

/* VGPIO TX (PMIC -> AP) table */
#define S2SE910_VGI1_WRSTBO			(1 << 0)
#define S2SE910_VGI1_IRQ_S1			(1 << 1)
#define S2SE910_VGI1_IRQ_S2			(1 << 2)
#define S2SE910_VGI1_IRQ_EXTRA			(1 << 3)

#define S2SE910_VGI2_IRQ_RF1			(1 << 0)
#define S2SE910_VGI2_IRQ_RF2			(1 << 1)
#define S2SE910_VGI2_IRQ_S3			(1 << 2)

#define S2SE910_VGI4_IRQ_S4			(1 << 0)
#define S2SE910_VGI4_ONOB			(1 << 1)
#define S2SE910_VGI4_IRQ_M			(1 << 2)
#define S2SE910_VGI4_VOL_DN			(1 << 3)

#define S2SE910_VGI5_IRQ_S5			(1 << 0)

#define S2SE910_BUCK_MAX		3 	/* BUCK, BUCK_SR, BB */
#define S2SE910_TEMP_MAX		2	/* TEMP 140, 120 */
#define S2SE910_OVP_MAX			1

#define BB1M_IDX			1

enum s2se910_irq_source {
	S2SE910_IRQ_RTC_PM_INT1 = 0,
	S2SE910_IRQ_RTC_PM_INT2,
	S2SE910_IRQ_RTC_PM_INT3,
	S2SE910_IRQ_RTC_PM_INT4,
	S2SE910_IRQ_RTC_PM_INT5,
	S2SE910_IRQ_RTC_PM_INT6,
	S2SE910_IRQ_RTC_PM_INT7,
	S2SE910_IRQ_RTC_PM_INT8,
	//S2SE910_IRQ_ADC_INTP, /* Not use NTC IRQ*/

	S2SE910_IRQ_GROUP_NR,
};

enum s2se910_irq {
	S2SE910_PMIC_IRQ_PWRONR_PM_INT1,
	S2SE910_PMIC_IRQ_PWRONP_PM_INT1,
	S2SE910_PMIC_IRQ_JIGONBF_PM_INT1,
	S2SE910_PMIC_IRQ_JIGONBR_PM_INT1,
	S2SE910_PMIC_IRQ_ACOKBF_PM_INT1,
	S2SE910_PMIC_IRQ_ACOKBR_PM_INT1,
	S2SE910_PMIC_IRQ_PWRON1S_PM_INT1,
	S2SE910_PMIC_IRQ_MRB_PM_INT1,

	S2SE910_PMIC_IRQ_RTC60S_PM_INT2,
	S2SE910_PMIC_IRQ_RTCA1_PM_INT2,
	S2SE910_PMIC_IRQ_RTCA0_PM_INT2,
	S2SE910_PMIC_IRQ_SMPL_PM_INT2,
	S2SE910_PMIC_IRQ_RTC1S_PM_INT2,
	S2SE910_PMIC_IRQ_WTSR_PM_INT2,
	S2SE910_PMIC_IRQ_SPMI_LDO_OK_FAIL_PM_INT2,
	S2SE910_PMIC_IRQ_WRSTB_PM_INT2,

	S2SE910_PMIC_IRQ_INT120C_PM_INT3,
	S2SE910_PMIC_IRQ_INT140C_PM_INT3,
	S2SE910_PMIC_IRQ_TSD_PM_INT3,
	S2SE910_PMIC_IRQ_OVP_PM_INT3,
	S2SE910_PMIC_IRQ_TX_TRAN_FAIL_PM_INT3,
	S2SE910_PMIC_IRQ_OTP_CSUM_ERR_PM_INT3,
	S2SE910_PMIC_IRQ_VOLDNR_PM_INT3,
	S2SE910_PMIC_IRQ_VOLDNP_PM_INT3,

	S2SE910_PMIC_IRQ_OCP_SR1_PM_INT4,
	S2SE910_PMIC_IRQ_OCP_BB1_PM_INT4,
	S2SE910_PMIC_IRQ_OCP_BB2_PM_INT4,
	S2SE910_PMIC_IRQ_OI_SR1_PM_INT4,
	S2SE910_PMIC_IRQ_OI_BB1_PM_INT4,
	S2SE910_PMIC_IRQ_OI_BB2_PM_INT4,
	S2SE910_PMIC_IRQ_PARITY_ERR_PM_INT4,
	S2SE910_PMIC_IRQ_IO_1P2_LDO_OK_FAIL_PM_INT4,

	S2SE910_PMIC_IRQ_UV_BB1_PM_INT5,
	S2SE910_PMIC_IRQ_UV_BB2_PM_INT5,
	S2SE910_PMIC_IRQ_BB1_NTR_DET_PM_INT5,
	S2SE910_PMIC_IRQ_BB2_NTR_DET_PM_INT5,
	S2SE910_PMIC_IRQ_WDT_PM_INT5,
	S2SE910_PMIC_IRQ_OI_L25_PM_INT5,
	S2SE910_PMIC_IRQ_OI_L26_PM_INT5,

	S2SE910_PMIC_IRQ_OI_L1_PM_INT6,
	S2SE910_PMIC_IRQ_OI_L2_PM_INT6,
	S2SE910_PMIC_IRQ_OI_L3_PM_INT6,
	S2SE910_PMIC_IRQ_OI_L4_PM_INT6,
	S2SE910_PMIC_IRQ_OI_L5_PM_INT6,
	S2SE910_PMIC_IRQ_OI_L6_PM_INT6,
	S2SE910_PMIC_IRQ_OI_L7_PM_INT6,
	S2SE910_PMIC_IRQ_OI_L8_PM_INT6,

	S2SE910_PMIC_IRQ_OI_L9_PM_INT7,
	S2SE910_PMIC_IRQ_OI_L10_PM_INT7,
	S2SE910_PMIC_IRQ_OI_L11_PM_INT7,
	S2SE910_PMIC_IRQ_OI_L12_PM_INT7,
	S2SE910_PMIC_IRQ_OI_L13_PM_INT7,
	S2SE910_PMIC_IRQ_OI_L14_PM_INT7,
	S2SE910_PMIC_IRQ_OI_L15_PM_INT7,
	S2SE910_PMIC_IRQ_OI_L16_PM_INT7,

	S2SE910_PMIC_IRQ_OI_L17_PM_INT8,
	S2SE910_PMIC_IRQ_OI_L18_PM_INT8,
	S2SE910_PMIC_IRQ_OI_L19_PM_INT8,
	S2SE910_PMIC_IRQ_OI_L20_PM_INT8,
	S2SE910_PMIC_IRQ_OI_L21_PM_INT8,
	S2SE910_PMIC_IRQ_OI_L22_PM_INT8,
	S2SE910_PMIC_IRQ_OI_L23_PM_INT8,
	S2SE910_PMIC_IRQ_OI_L24_PM_INT8,

//	S2SE910_ADC_IRQ_NTC0_OVER_INTP,
//	S2SE910_ADC_IRQ_NTC1_OVER_INTP,

	S2SE910_IRQ_NR,
};

#define MFD_DEV_NAME "s2se910"

enum s2se910_types {
	TYPE_S2SE910,

	TYPE_S2SE910_NR,
};

struct s2se910_dev {
	struct device *dev;
	struct s2p_dev *sdev;

	uint16_t vgpio;
	uint16_t com;
	uint16_t rtc;
	uint16_t pm1;
	uint16_t pm2;
	uint16_t pm3;
	uint16_t adc;
	uint16_t ext_buck;
	uint16_t ext;

	/* IRQ */
	struct mutex irq_lock;
	int irq_masks_cur[S2SE910_IRQ_GROUP_NR];
	int irq_masks_cache[S2SE910_IRQ_GROUP_NR];
	uint8_t irq_reg[S2SE910_IRQ_GROUP_NR];
};

extern int s2se910_irq_init(struct s2se910_dev *s2se910);
#endif /* __LINUX_MFD_S2SE910_H */
