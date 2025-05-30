/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for S5E9955
 */

#ifndef _DT_BINDINGS_S5E9955_DM_H
#define _DT_BINDINGS_S5E9955_DM_H

/* NUMBER FOR DVFS MANAGER */
#define DM_MIF				(0x00)
#define DM_INT				(0x01)
#define DM_CPUCL0			(0x02)
#define DM_CPUCL1L			(0x03)
#define DM_CPUCL1H			(0x04)
#define DM_CPUCL2			(0x05)
#define DM_NPU				(0x06)
#define DM_PSP				(0x07)
#define DM_DSU				(0x08)
#define DM_AUD				(0x09)
#define DM_G3D				(0x0A)
#define DM_INTCAM			(0x0B)
#define DM_CAM				(0x0C)
#define DM_DISP				(0x0D)
#define DM_CSIS				(0x0E)
#define DM_ISP				(0x0F)
#define DM_MFC				(0x10)
#define DM_MFD				(0x11)
#define DM_ICPU				(0x12)
#define DM_DSP				(0x13)
#define DM_DNC				(0x14)
#define DM_GNSS				(0x15)
#define DM_ALIVE			(0x16)
#define DM_CHUB				(0x17)
#define DM_VTS				(0x18)
#define DM_HSI0				(0x19)
#define DM_UFD				(0x1A)
#define DM_UNPU				(0x1B)

/* CONSTRAINT TYPE */
#define CONSTRAINT_MIN	0
#define CONSTRAINT_MAX	1

#endif	/* _DT_BINDINGS_S5E9955_DM_H */
