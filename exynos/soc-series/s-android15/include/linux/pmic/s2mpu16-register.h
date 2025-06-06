/*
 * s2mpu16-register.h - PMIC register for the S2MPU16
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

#ifndef __LINUX_S2MPU16_REGISTER_H
#define __LINUX_S2MPU16_REGISTER_H

#define MASK(width, shift)		(((0x1 << (width)) - 1) << shift)
#define SetBit(no)			(0x1 << (no))

/* PMIC base addr */
#define S2MPU16_VGPIO_ADDR		0x00
#define S2MPU16_COMMON_ADDR		0x03
#define S2MPU16_PMIC1_ADDR		0x05
#define S2MPU16_PMIC2_ADDR		0x06
#define S2MPU16_CLOSE1_ADDR		0x0E
#define S2MPU16_CLOSE2_ADDR		0x0F
#define S2MPU16_GPIO_ADDR		0x0B

/* PMIC ADDRESS: VGPIO_bitmap */
#define S2MPU16_VGPIO_REG0		0x00
#define S2MPU16_VGPIO_PSI		0x01
#define S2MPU16_VGPIO_VGI0		0x02
#define S2MPU16_VGPIO_VGI1		0x03
#define S2MPU16_VGPIO_VGI2		0x04
#define S2MPU16_VGPIO_VGI3		0x05
#define S2MPU16_VGPIO_VGI4		0x06
#define S2MPU16_VGPIO_VGI5		0x07
#define S2MPU16_VGPIO_VGI6		0x08
#define S2MPU16_VGPIO_VGI7		0x09
#define S2MPU16_VGPIO_VGI8		0x0A
#define S2MPU16_VGPIO_VGI9		0x0B
#define S2MPU16_VGPIO_VGI10		0x0C
#define S2MPU16_VGPIO_VGI11		0x0D
#define S2MPU16_VGPIO_VGI12		0x0E
#define S2MPU16_VGPIO_VGI13		0x0F
#define S2MPU16_VGPIO_VGI14		0x10
#define S2MPU16_VGPIO_VGI15		0x11
#define S2MPU16_VGPIO_VGI16		0x12
#define S2MPU16_VGPIO_VGI17		0x13
#define S2MPU16_VGPIO_VGI18		0x14

/* PMIC ADDRESS: COMMON_bitmap */
#define S2MPU16_COMMON_REG0_VGPIO0	0x00
#define S2MPU16_COMMON_REG0_VGPIO1	0x01
#define S2MPU16_COMMON_CHIP_ID	  	0x0E
#define S2MPU16_COMMON_TX_MASK	  	0x16
#define S2MPU16_COMMON_IRQ	  	0x17

/* CHIP ID MASK */
#define S2MPU16_CHIP_ID_MASK		(0xFF)
#define S2MPU16_CHIP_ID_HW_MASK		(0x0F)
#define S2MPU16_CHIP_ID_SW_MASK		(0xF0)
#define S2MPU16_CHIP_ID_HW(id)		(id & 0x0F)
#define S2MPU16_CHIP_ID_SW(id)		((id >> 4) & 0x0F)

/* TX_MASK MASK */
#define S2MPU16_GPIO_IRQM_MASK		(1 << 2)
#define S2MPU16_ADC_IRQM_MASK		(1 << 1)
#define S2MPU16_PM_IRQM_MASK		(1 << 0)

/* PMIC ADDRESS: OOTP1_bitmap */
#define S2MPU16_PM1_INT1			0x00
#define S2MPU16_PM1_INT2			0x01
#define S2MPU16_PM1_INT3			0x02
#define S2MPU16_PM1_INT4			0x03
#define S2MPU16_PM1_INT5			0x04
#define S2MPU16_PM1_INT6			0x05
#define S2MPU16_PM1_INT7			0x06
#define S2MPU16_PM1_INT1M			0x07
#define S2MPU16_PM1_INT2M			0x08
#define S2MPU16_PM1_INT3M			0x09
#define S2MPU16_PM1_INT4M			0x0A
#define S2MPU16_PM1_INT5M			0x0B
#define S2MPU16_PM1_INT6M			0x0C
#define S2MPU16_PM1_INT7M			0x0D
#define S2MPU16_PM1_STATUS1			0x0E
#define S2MPU16_PM1_OFFSRC1_CUR			0x11
#define S2MPU16_PM1_OFFSRC2_CUR			0x12
#define S2MPU16_PM1_OFFSRC1_OLD1		0x13
#define S2MPU16_PM1_OFFSRC2_OLD1		0x14
#define S2MPU16_PM1_OFFSRC1_OLD2		0x15
#define S2MPU16_PM1_OFFSRC2_OLD2		0x16
#define S2MPU16_PM1_CTRL1			0x17
#define S2MPU16_PM1_CTRL3			0x19
#define S2MPU16_PM1_CFG_PM			0x1D
#define S2MPU16_PM1_TIME_CTRL			0x1E
#define S2MPU16_PM1_BUCK1S_CTRL			0x1F
#define S2MPU16_PM1_BUCK1S_OUT1			0x20
#define S2MPU16_PM1_BUCK1S_OUT2			0x21
#define S2MPU16_PM1_BUCK1S_OUT3			0x22
#define S2MPU16_PM1_BUCK1S_OUT4			0x23
#define S2MPU16_PM1_BUCK1S_DVS			0x24
#define S2MPU16_PM1_BUCK1S_AFM			0x25
#define S2MPU16_PM1_BUCK1S_AFMX			0x26
#define S2MPU16_PM1_BUCK1S_AFMY			0x27
#define S2MPU16_PM1_BUCK1S_AFMZ			0x28
#define S2MPU16_PM1_BUCK1S_OCP			0x29
#define S2MPU16_PM1_BUCK1S_AVP			0x2A
#define S2MPU16_PM1_BUCK2S_CTRL			0x2B
#define S2MPU16_PM1_BUCK2S_OUT1			0x2C
#define S2MPU16_PM1_BUCK2S_OUT2			0x2D
#define S2MPU16_PM1_BUCK2S_OUT3			0x2E
#define S2MPU16_PM1_BUCK2S_OUT4			0x2F
#define S2MPU16_PM1_BUCK2S_DVS			0x30
#define S2MPU16_PM1_BUCK2S_AFM			0x31
#define S2MPU16_PM1_BUCK2S_AFMX			0x32
#define S2MPU16_PM1_BUCK2S_AFMY			0x33
#define S2MPU16_PM1_BUCK2S_AFMZ			0x34
#define S2MPU16_PM1_BUCK2S_OCP			0x35
#define S2MPU16_PM1_BUCK2S_AVP			0x36
#define S2MPU16_PM1_BUCK3S_CTRL			0x37
#define S2MPU16_PM1_BUCK3S_OUT1			0x38
#define S2MPU16_PM1_BUCK3S_OUT2			0x39
#define S2MPU16_PM1_BUCK3S_OUT3			0x3A
#define S2MPU16_PM1_BUCK3S_OUT4			0x3B
#define S2MPU16_PM1_BUCK3S_DVS			0x3C
#define S2MPU16_PM1_BUCK3S_AFM			0x3D
#define S2MPU16_PM1_BUCK3S_AFMX			0x3E
#define S2MPU16_PM1_BUCK3S_AFMY			0x3F
#define S2MPU16_PM1_BUCK3S_AFMZ			0x40
#define S2MPU16_PM1_BUCK3S_OCP			0x41
#define S2MPU16_PM1_BUCK3S_AVP			0x42
#define S2MPU16_PM1_BUCK4S_CTRL			0x43
#define S2MPU16_PM1_BUCK4S_OUT1			0x44
#define S2MPU16_PM1_BUCK4S_OUT2			0x45
#define S2MPU16_PM1_BUCK4S_OUT3			0x46
#define S2MPU16_PM1_BUCK4S_OUT4			0x47
#define S2MPU16_PM1_BUCK4S_DVS			0x48
#define S2MPU16_PM1_BUCK4S_AFM			0x49
#define S2MPU16_PM1_BUCK4S_AFMX			0x4A
#define S2MPU16_PM1_BUCK4S_AFMY			0x4B
#define S2MPU16_PM1_BUCK4S_AFMZ			0x4C
#define S2MPU16_PM1_BUCK4S_OCP			0x4D
#define S2MPU16_PM1_BUCK4S_AVP			0x4E
#define S2MPU16_PM1_BUCK5S_CTRL			0x4F
#define S2MPU16_PM1_BUCK5S_OUT1			0x50
#define S2MPU16_PM1_BUCK5S_OUT2			0x51
#define S2MPU16_PM1_BUCK5S_OUT3			0x52
#define S2MPU16_PM1_BUCK5S_OUT4			0x53
#define S2MPU16_PM1_BUCK5S_DVS			0x54
#define S2MPU16_PM1_BUCK5S_AFM			0x55
#define S2MPU16_PM1_BUCK5S_AFMX			0x56
#define S2MPU16_PM1_BUCK5S_AFMY			0x57
#define S2MPU16_PM1_BUCK5S_AFMZ			0x58
#define S2MPU16_PM1_BUCK5S_OCP			0x59
#define S2MPU16_PM1_BUCK5S_AVP			0x5A
#define S2MPU16_PM1_BUCK6S_CTRL			0x5B
#define S2MPU16_PM1_BUCK6S_OUT1			0x5C
#define S2MPU16_PM1_BUCK6S_OUT2			0x5D
#define S2MPU16_PM1_BUCK6S_OUT3			0x5E
#define S2MPU16_PM1_BUCK6S_OUT4			0x5F
#define S2MPU16_PM1_BUCK6S_DVS			0x60
#define S2MPU16_PM1_BUCK6S_AFM			0x61
#define S2MPU16_PM1_BUCK6S_AFMX			0x62
#define S2MPU16_PM1_BUCK6S_AFMY			0x63
#define S2MPU16_PM1_BUCK6S_AFMZ			0x64
#define S2MPU16_PM1_BUCK6S_OCP			0x65
#define S2MPU16_PM1_BUCK6S_AVP			0x66
#define S2MPU16_PM1_BUCK7S_CTRL			0x67
#define S2MPU16_PM1_BUCK7S_OUT1			0x68
#define S2MPU16_PM1_BUCK7S_OUT2			0x69
#define S2MPU16_PM1_BUCK7S_OUT3			0x6A
#define S2MPU16_PM1_BUCK7S_OUT4			0x6B
#define S2MPU16_PM1_BUCK7S_DVS			0x6C
#define S2MPU16_PM1_BUCK7S_AFM			0x6D
#define S2MPU16_PM1_BUCK7S_AFMX			0x6E
#define S2MPU16_PM1_BUCK7S_AFMY			0x6F
#define S2MPU16_PM1_BUCK7S_AFMZ			0x70
#define S2MPU16_PM1_BUCK7S_OCP			0x71
#define S2MPU16_PM1_BUCK7S_AVP			0x72
#define S2MPU16_PM1_BUCK8S_CTRL			0x73
#define S2MPU16_PM1_BUCK8S_OUT1			0x74
#define S2MPU16_PM1_BUCK8S_OUT2			0x75
#define S2MPU16_PM1_BUCK8S_OUT3			0x76
#define S2MPU16_PM1_BUCK8S_OUT4			0x77
#define S2MPU16_PM1_BUCK8S_DVS			0x78
#define S2MPU16_PM1_BUCK8S_AFM			0x79
#define S2MPU16_PM1_BUCK8S_AFMX			0x7A
#define S2MPU16_PM1_BUCK8S_AFMY			0x7B
#define S2MPU16_PM1_BUCK8S_AFMZ			0x7C
#define S2MPU16_PM1_BUCK8S_OCP			0x7D
#define S2MPU16_PM1_BUCK8S_AVP			0x7E
#define S2MPU16_PM1_BUCK9S_CTRL			0x7F
#define S2MPU16_PM1_BUCK9S_OUT1			0x80
#define S2MPU16_PM1_BUCK9S_OUT2			0x81
#define S2MPU16_PM1_BUCK9S_OUT3			0x82
#define S2MPU16_PM1_BUCK9S_OUT4			0x83
#define S2MPU16_PM1_BUCK9S_DVS			0x84
#define S2MPU16_PM1_BUCK9S_AFM			0x85
#define S2MPU16_PM1_BUCK9S_AFMX			0x86
#define S2MPU16_PM1_BUCK9S_AFMY			0x87
#define S2MPU16_PM1_BUCK9S_AFMZ			0x88
#define S2MPU16_PM1_BUCK9S_OCP			0x89
#define S2MPU16_PM1_BUCK9S_AVP			0x8A
#define S2MPU16_PM1_BUCK10S_CTRL		0x8B
#define S2MPU16_PM1_BUCK10S_OUT1		0x8C
#define S2MPU16_PM1_BUCK10S_OUT2		0x8D
#define S2MPU16_PM1_BUCK10S_OUT3		0x8E
#define S2MPU16_PM1_BUCK10S_OUT4		0x8F
#define S2MPU16_PM1_BUCK10S_DVS			0x90
#define S2MPU16_PM1_BUCK10S_AFM			0x91
#define S2MPU16_PM1_BUCK10S_AFMX		0x92
#define S2MPU16_PM1_BUCK10S_AFMY		0x93
#define S2MPU16_PM1_BUCK10S_AFMZ		0x94
#define S2MPU16_PM1_BUCK10S_OCP			0x95
#define S2MPU16_PM1_BUCK10S_AVP			0x96
#define S2MPU16_PM1_SR1S_CTRL			0x97
#define S2MPU16_PM1_SR1S_OUT1			0x98
#define S2MPU16_PM1_SR1S_OUT2			0x99
#define S2MPU16_PM1_SR1S_DVS1			0x9A
#define S2MPU16_PM1_SR1S_DVS2			0x9B
#define S2MPU16_PM1_SR1S_OCP			0x9C
#define S2MPU16_PM1_SR2S_CTRL			0x9D
#define S2MPU16_PM1_SR2S_OUT1			0x9E
#define S2MPU16_PM1_SR2S_OUT2			0x9F
#define S2MPU16_PM1_SR2S_OCP			0xA0
#define S2MPU16_PM1_DVS_LDO_OFFSET1		0xA1
#define S2MPU16_PM1_DVS_LDO_OFFSET2		0xA2
#define S2MPU16_PM1_DVS_LDO_OFFSET3		0xA3
#define S2MPU16_PM1_DVS_LDO_RAMP1		0xA4
#define S2MPU16_PM1_DVS_LDO_RAMP2		0xA5
#define S2MPU16_PM1_DVS_LDO_RAMP3		0xA6
#define S2MPU16_PM1_LDO1S_CTRL			0xA7
#define S2MPU16_PM1_LDO1S_OUT1			0xA8
#define S2MPU16_PM1_LDO1S_OUT2			0xA9
#define S2MPU16_PM1_LDO1S_OUT3			0xAA
#define S2MPU16_PM1_LDO1S_OUT4			0xAB
#define S2MPU16_PM1_LDO2S_CTRL			0xAC
#define S2MPU16_PM1_LDO2S_OUT			0xAD
#define S2MPU16_PM1_LDO3S_CTRL			0xAE
#define S2MPU16_PM1_LDO4S_CTRL			0xAF
#define S2MPU16_PM1_LDO4S_OUT1			0xB0
#define S2MPU16_PM1_LDO4S_OUT2			0xB1
#define S2MPU16_PM1_LDO4S_OUT3			0xB2
#define S2MPU16_PM1_LDO4S_OUT4			0xB3
#define S2MPU16_PM1_LDO5S_CTRL			0xB4
#define S2MPU16_PM1_LDO5S_OUT1			0xB5
#define S2MPU16_PM1_LDO5S_OUT2			0xB6
#define S2MPU16_PM1_LDO5S_OUT3			0xB7
#define S2MPU16_PM1_LDO5S_OUT4			0xB8
#define S2MPU16_PM1_LDO6S_CTRL			0xB9
#define S2MPU16_PM1_LDO6S_OUT			0xBA
#define S2MPU16_PM1_LDO7S_CTRL			0xBB
#define S2MPU16_PM1_LDO7S_OUT			0xBC
#define S2MPU16_PM1_LDO8S_CTRL			0xBD
#define S2MPU16_PM1_LDO9S_CTRL			0xBE
#define S2MPU16_PM1_LDO10S_CTRL			0xBF
#define S2MPU16_PM1_LDO11S_CTRL			0xC0
#define S2MPU16_PM1_LDO12S_CTRL			0xC1
#define S2MPU16_PM1_LDO13S_CTRL			0xC2
#define S2MPU16_PM1_LDO14S_CTRL			0xC3
#define S2MPU16_PM1_LDO14S_OUT			0xC4
#define S2MPU16_PM1_LDO15S_CTRL			0xC5
#define S2MPU16_PM1_LDO15S_OUT1			0xC6
#define S2MPU16_PM1_LDO15S_OUT2			0xC7
#define S2MPU16_PM1_LDO15S_OUT3			0xC8
#define S2MPU16_PM1_LDO15S_OUT4			0xC9
#define S2MPU16_PM1_LDO_DSCH1			0xCA
#define S2MPU16_PM1_LDO_DSCH2			0xCB
#define S2MPU16_PM1_PLATFORM_ID			0xCC

/* PMIC ADDRESS: OOTP2_bitmap */
#define S2MPU16_PM2_ONSEQ_CTRL1			0x00
#define S2MPU16_PM2_ONSEQ_CTRL2			0x01
#define S2MPU16_PM2_ONSEQ_CTRL3			0x02
#define S2MPU16_PM2_ONSEQ_CTRL4			0x03
#define S2MPU16_PM2_ONSEQ_CTRL5			0x04
#define S2MPU16_PM2_ONSEQ_CTRL6			0x05
#define S2MPU16_PM2_ONSEQ_CTRL7			0x06
#define S2MPU16_PM2_ONSEQ_CTRL8			0x07
#define S2MPU16_PM2_ONSEQ_CTRL9			0x08
#define S2MPU16_PM2_ONSEQ_CTRL10		0x09
#define S2MPU16_PM2_ONSEQ_CTRL11		0x0A
#define S2MPU16_PM2_ONSEQ_CTRL12		0x0B
#define S2MPU16_PM2_ONSEQ_CTRL13		0x0C
#define S2MPU16_PM2_ONSEQ_CTRL14		0x0D
#define S2MPU16_PM2_ONSEQ_CTRL15		0x0E
#define S2MPU16_PM2_ONSEQ_CTRL16		0x0F
#define S2MPU16_PM2_ONSEQ_CTRL17		0x10
#define S2MPU16_PM2_ONSEQ_CTRL18		0x11
#define S2MPU16_PM2_ONSEQ_CTRL19		0x12
#define S2MPU16_PM2_ONSEQ_CTRL20		0x13
#define S2MPU16_PM2_ONSEQ_CTRL21		0x14
#define S2MPU16_PM2_ONSEQ_CTRL22		0x15
#define S2MPU16_PM2_ONSEQ_CTRL23		0x16
#define S2MPU16_PM2_ONSEQ_CTRL24		0x17
#define S2MPU16_PM2_ONSEQ_CTRL25		0x18
#define S2MPU16_PM2_ONSEQ_CTRL26		0x19
#define S2MPU16_PM2_ONSEQ_CTRL27		0x1A
#define S2MPU16_PM2_OFF_SEQ_CTRL1		0x1B
#define S2MPU16_PM2_OFF_SEQ_CTRL2		0x1C
#define S2MPU16_PM2_OFF_SEQ_CTRL3		0x1D
#define S2MPU16_PM2_OFF_SEQ_CTRL4		0x1E
#define S2MPU16_PM2_OFF_SEQ_CTRL5		0x1F
#define S2MPU16_PM2_OFF_SEQ_CTRL6		0x20
#define S2MPU16_PM2_OFF_SEQ_CTRL7		0x21
#define S2MPU16_PM2_OFF_SEQ_CTRL8		0x22
#define S2MPU16_PM2_OFF_SEQ_CTRL9		0x23
#define S2MPU16_PM2_OFF_SEQ_CTRL10		0x24
#define S2MPU16_PM2_OFF_SEQ_CTRL11		0x25
#define S2MPU16_PM2_OFF_SEQ_CTRL12		0x26
#define S2MPU16_PM2_OFF_SEQ_CTRL13		0x27
#define S2MPU16_PM2_OFF_SEQ_CTRL14		0x28
#define S2MPU16_PM2_S_SEL_VGPIO1		0x29
#define S2MPU16_PM2_S_SEL_VGPIO2		0x2A
#define S2MPU16_PM2_S_SEL_VGPIO3		0x2B
#define S2MPU16_PM2_S_SEL_VGPIO4		0x2C
#define S2MPU16_PM2_S_SEL_VGPIO5		0x2D
#define S2MPU16_PM2_S_SEL_VGPIO6		0x2E
#define S2MPU16_PM2_S_SEL_VGPIO7		0x2F
#define S2MPU16_PM2_S_SEL_VGPIO8		0x30
#define S2MPU16_PM2_S_SEL_VGPIO9		0x31
#define S2MPU16_PM2_S_SEL_VGPIO10		0x32
#define S2MPU16_PM2_S_SEL_VGPIO11		0x33
#define S2MPU16_PM2_S_SEL_VGPIO12		0x34
#define S2MPU16_PM2_S_SEL_VGPIO13		0x35
#define S2MPU16_PM2_S_SEL_VGPIO14		0x36
#define S2MPU16_PM2_S_SEL_VGPIO15		0x37
#define S2MPU16_PM2_S_SEL_VGPIO16		0x38
#define S2MPU16_PM2_S_SEL_VGPIO17		0x39
#define S2MPU16_PM2_S_SEL_VGPIO18		0x3A
#define S2MPU16_PM2_S_SEL_VGPIO19		0x3B
#define S2MPU16_PM2_S_SEL_VGPIO20		0x3C
#define S2MPU16_PM2_S_SEL_VGPIO21		0x3D
#define S2MPU16_PM2_S_SEL_VGPIO22		0x3E
#define S2MPU16_PM2_S_SEL_VGPIO23		0x3F
#define S2MPU16_PM2_S_SEL_VGPIO24		0x40
#define S2MPU16_PM2_S_SEL_VGPIO25		0x41
#define S2MPU16_PM2_S_SEL_VGPIO26		0x42
#define S2MPU16_PM2_S_SEL_VGPIO27		0x43
#define S2MPU16_PM2_OFF_CTRL6			0x44
#define S2MPU16_PM2_OFF_CTRL7			0x45
#define S2MPU16_PM2_DUMP_CTRL3			0x46
#define S2MPU16_PM2_DUMP_CTRL4			0x47
#define S2MPU16_PM2_DUMP_CTRL5			0x48
#define S2MPU16_PM2_OOTP2_RSVD1			0x49
#define S2MPU16_PM2_S_SEL_DVS_EN1		0x4A
#define S2MPU16_PM2_S_SEL_DVS_EN2		0x4B
#define S2MPU16_PM2_S_SEL_DVS_EN3		0x4C
#define S2MPU16_PM2_S_SEL_DVS_EN4		0x4D
#define S2MPU16_PM2_S_SEL_DVS_EN5		0x4E
#define S2MPU16_PM2_S_SEL_DVS_EN6		0x4F
#define S2MPU16_PM2_S_SEL_DVS_EN7		0x50
#define S2MPU16_PM2_SR_SEL_DVS_EN1		0x51
#define S2MPU16_PM2_OFF_CTRL1			0x53
#define S2MPU16_PM2_OFF_CTRL2			0x54
#define S2MPU16_PM2_OFF_CTRL3			0x55
#define S2MPU16_PM2_OFF_CTRL4			0x56
#define S2MPU16_PM2_OFF_CTRL5			0x57
#define S2MPU16_PM2_SEL_HW_GPIO			0x5F
#define S2MPU16_PM2_LDO_OI_CTRL1		0x60

/* PMIC ADDRESS: GPIO_bitmap */
#define S2MPU16_GPIO_GPIO_INT1			0x00
#define S2MPU16_GPIO_GPIO_INT2			0x01
#define S2MPU16_GPIO_GPIO_INT3			0x02
#define S2MPU16_GPIO_GPIO_INTM1			0x03
#define S2MPU16_GPIO_GPIO_INTM2			0x04
#define S2MPU16_GPIO_GPIO_INTM3			0x05
#define S2MPU16_GPIO_GPIO_STATUS1		0x06
#define S2MPU16_GPIO_GPIO_STATUS2		0x07
#define S2MPU16_GPIO_GPIO_SET0			0x12
#define S2MPU16_GPIO_GPIO_SET1			0x13
#define S2MPU16_GPIO_GPIO_SET2			0x14
#define S2MPU16_GPIO_GPIO_SET3			0x15
#define S2MPU16_GPIO_GPIO_SET4			0x16
#define S2MPU16_GPIO_GPIO_SET5			0x17
#define S2MPU16_GPIO_GPIO_SET6			0x18
#define S2MPU16_GPIO_GPIO_SET7			0x19
#define S2MPU16_GPIO_GPIO_SET8			0x1A
#define S2MPU16_GPIO_GPIO_SET9			0x1B
#define S2MPU16_GPIO_WRSTBI_SET			0x1C
#define S2MPU16_GPIO_GPIO_SET_HV		0x1D
#define S2MPU16_GPIO_GPIO_TX_MASK1		0x1E
#define S2MPU16_GPIO_GPIO_TX_MASK2		0x1F

/* GPIO BIT */
/* Samsung specific pin configurations */
#define S2MPU16_GPIO_OEN_SHIFT			(6)
#define S2MPU16_GPIO_OEN_MASK			(0x01 << S2MPU16_GPIO_OEN_SHIFT)

#define S2MPU16_GPIO_OUT_SHIFT			(5)
#define S2MPU16_GPIO_OUT_MASK			(0x01 << S2MPU16_GPIO_OUT_SHIFT)

#define S2MPU16_GPIO_PULL_SHIFT			(3)
#define S2MPU16_GPIO_PULL_MASK			(0x03 << S2MPU16_GPIO_PULL_SHIFT)

#define S2MPU16_GPIO_DRV_STR_SHIFT		(0)
#define S2MPU16_GPIO_DRV_STR_MASK		(0x07 << S2MPU16_GPIO_DRV_STR_SHIFT)

/* GPIO Info */
#define S2MPU16_GPIO_HV_IDX			(10)
#define S2MPU16_GPIO_RANGE			(8)
#endif /* __LINUX_S2MPU16_REGISTER_H */
