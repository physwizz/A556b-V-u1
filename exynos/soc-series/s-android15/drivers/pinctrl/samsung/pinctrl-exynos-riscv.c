// SPDX-License-Identifier: GPL-2.0+
//
// Exynos ARMv8 specific support for Samsung pinctrl/gpiolib driver
// with eint support.
//
// Copyright (c) 2012 Samsung Electronics Co., Ltd.
//		http://www.samsung.com
// Copyright (c) 2012 Linaro Ltd
//		http://www.linaro.org
// Copyright (c) 2017 Krzysztof Kozlowski <krzk@kernel.org>
//
// This file contains the Samsung Exynos specific information required by the
// the Samsung pinctrl/gpiolib driver. It also includes the implementation of
// external gpio and wakeup interrupt support.

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/soc/samsung/exynos-regs-pmu.h>

#include "pinctrl-samsung.h"
#include "pinctrl-exynos.h"

static const struct samsung_pin_bank_type bank_type_off = {
	.fld_width = { 4, 1, 2, 2, 2, 2, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, },
};

static const struct samsung_pin_bank_type bank_type_alive = {
	.fld_width = { 4, 1, 2, 2, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, },
};

/* Exynos5433 has the 4bit widths for PINCFG_TYPE_DRV bitfields. */
static const struct samsung_pin_bank_type exynos5433_bank_type_off = {
	.fld_width = { 4, 1, 2, 4, 2, 2, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, },
};

static const struct samsung_pin_bank_type exynos5433_bank_type_alive = {
	.fld_width = { 4, 1, 2, 4, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, },
};

/*
 * Bank type for non-alive type. Bit fields:
 * CON: 4, DAT: 1, PUD: 4, DRV: 4, CONPDN: 2, PUDPDN: 4
 */
static const struct samsung_pin_bank_type exynos850_bank_type_off  = {
	.fld_width = { 4, 1, 4, 4, 2, 4, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, },
};

/*
 * Bank type for alive type. Bit fields:
 * CON: 4, DAT: 1, PUD: 4, DRV: 4
 */
static const struct samsung_pin_bank_type exynos850_bank_type_alive = {
	.fld_width = { 4, 1, 4, 4, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, },
};
static struct samsung_pin_bank_type bank_type_6  = {
	.fld_width = { 4, 1, 4, 4, 2, 4, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, },
};

/* bank type for alive type
 * (CON bit field: 4, DAT bit field: 1, PUD bit field: 4, DRV bit field: 4)
 */
static struct samsung_pin_bank_type bank_type_7 = {
	.fld_width = { 4, 1, 4, 4, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, },
};

/* Pad retention control code for accessing PMU regmap */
static atomic_t exynos_shared_retention_refcnt;

/* pin banks of exynos5433 pin-controller - ALIVE */
static const struct samsung_pin_bank_data exynos5433_pin_banks0[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTW(8, 0x000, "gpa0", 0x00),
	EXYNOS5433_PIN_BANK_EINTW(8, 0x020, "gpa1", 0x04),
	EXYNOS5433_PIN_BANK_EINTW(8, 0x040, "gpa2", 0x08),
	EXYNOS5433_PIN_BANK_EINTW(8, 0x060, "gpa3", 0x0c),
	EXYNOS5433_PIN_BANK_EINTW_EXT(8, 0x020, "gpf1", 0x1004, 1),
	EXYNOS5433_PIN_BANK_EINTW_EXT(4, 0x040, "gpf2", 0x1008, 1),
	EXYNOS5433_PIN_BANK_EINTW_EXT(4, 0x060, "gpf3", 0x100c, 1),
	EXYNOS5433_PIN_BANK_EINTW_EXT(8, 0x080, "gpf4", 0x1010, 1),
	EXYNOS5433_PIN_BANK_EINTW_EXT(8, 0x0a0, "gpf5", 0x1014, 1),
};

/* pin banks of exynos5433 pin-controller - AUD */
static const struct samsung_pin_bank_data exynos5433_pin_banks1[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(7, 0x000, "gpz0", 0x00),
	EXYNOS5433_PIN_BANK_EINTG(4, 0x020, "gpz1", 0x04),
};

/* pin banks of exynos5433 pin-controller - CPIF */
static const struct samsung_pin_bank_data exynos5433_pin_banks2[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(2, 0x000, "gpv6", 0x00),
};

/* pin banks of exynos5433 pin-controller - eSE */
static const struct samsung_pin_bank_data exynos5433_pin_banks3[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(3, 0x000, "gpj2", 0x00),
};

/* pin banks of exynos5433 pin-controller - FINGER */
static const struct samsung_pin_bank_data exynos5433_pin_banks4[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(4, 0x000, "gpd5", 0x00),
};

/* pin banks of exynos5433 pin-controller - FSYS */
static const struct samsung_pin_bank_data exynos5433_pin_banks5[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(6, 0x000, "gph1", 0x00),
	EXYNOS5433_PIN_BANK_EINTG(7, 0x020, "gpr4", 0x04),
	EXYNOS5433_PIN_BANK_EINTG(5, 0x040, "gpr0", 0x08),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x060, "gpr1", 0x0c),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x080, "gpr2", 0x10),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x0a0, "gpr3", 0x14),
};

/* pin banks of exynos5433 pin-controller - IMEM */
static const struct samsung_pin_bank_data exynos5433_pin_banks6[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(8, 0x000, "gpf0", 0x00),
};

/* pin banks of exynos5433 pin-controller - NFC */
static const struct samsung_pin_bank_data exynos5433_pin_banks7[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(3, 0x000, "gpj0", 0x00),
};

/* pin banks of exynos5433 pin-controller - PERIC */
static const struct samsung_pin_bank_data exynos5433_pin_banks8[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(6, 0x000, "gpv7", 0x00),
	EXYNOS5433_PIN_BANK_EINTG(5, 0x020, "gpb0", 0x04),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x040, "gpc0", 0x08),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x060, "gpc1", 0x0c),
	EXYNOS5433_PIN_BANK_EINTG(6, 0x080, "gpc2", 0x10),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x0a0, "gpc3", 0x14),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x0c0, "gpg0", 0x18),
	EXYNOS5433_PIN_BANK_EINTG(4, 0x0e0, "gpd0", 0x1c),
	EXYNOS5433_PIN_BANK_EINTG(6, 0x100, "gpd1", 0x20),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x120, "gpd2", 0x24),
	EXYNOS5433_PIN_BANK_EINTG(5, 0x140, "gpd4", 0x28),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x160, "gpd8", 0x2c),
	EXYNOS5433_PIN_BANK_EINTG(7, 0x180, "gpd6", 0x30),
	EXYNOS5433_PIN_BANK_EINTG(3, 0x1a0, "gpd7", 0x34),
	EXYNOS5433_PIN_BANK_EINTG(5, 0x1c0, "gpg1", 0x38),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x1e0, "gpg2", 0x3c),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x200, "gpg3", 0x40),
};

/* pin banks of exynos5433 pin-controller - TOUCH */
static const struct samsung_pin_bank_data exynos5433_pin_banks9[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS5433_PIN_BANK_EINTG(3, 0x000, "gpj1", 0x00),
};

/* PMU pin retention groups registers for Exynos5433 (without audio & fsys) */
static const u32 exynos5433_retention_regs[] = {
	EXYNOS5433_PAD_RETENTION_TOP_OPTION,
	EXYNOS5433_PAD_RETENTION_UART_OPTION,
	EXYNOS5433_PAD_RETENTION_EBIA_OPTION,
	EXYNOS5433_PAD_RETENTION_EBIB_OPTION,
	EXYNOS5433_PAD_RETENTION_SPI_OPTION,
	EXYNOS5433_PAD_RETENTION_MIF_OPTION,
	EXYNOS5433_PAD_RETENTION_USBXTI_OPTION,
	EXYNOS5433_PAD_RETENTION_BOOTLDO_OPTION,
	EXYNOS5433_PAD_RETENTION_UFS_OPTION,
	EXYNOS5433_PAD_RETENTION_FSYSGENIO_OPTION,
};

static const struct samsung_retention_data exynos5433_retention_data __initconst = {
	.regs	 = exynos5433_retention_regs,
	.nr_regs = ARRAY_SIZE(exynos5433_retention_regs),
	.value	 = EXYNOS_WAKEUP_FROM_LOWPWR,
	.refcnt	 = &exynos_shared_retention_refcnt,
	.init	 = exynos_retention_init,
};

/* PMU retention control for audio pins can be tied to audio pin bank */
static const u32 exynos5433_audio_retention_regs[] = {
	EXYNOS5433_PAD_RETENTION_AUD_OPTION,
};

static const struct samsung_retention_data exynos5433_audio_retention_data __initconst = {
	.regs	 = exynos5433_audio_retention_regs,
	.nr_regs = ARRAY_SIZE(exynos5433_audio_retention_regs),
	.value	 = EXYNOS_WAKEUP_FROM_LOWPWR,
	.init	 = exynos_retention_init,
};

/* PMU retention control for mmc pins can be tied to fsys pin bank */
static const u32 exynos5433_fsys_retention_regs[] = {
	EXYNOS5433_PAD_RETENTION_MMC0_OPTION,
	EXYNOS5433_PAD_RETENTION_MMC1_OPTION,
	EXYNOS5433_PAD_RETENTION_MMC2_OPTION,
};

static const struct samsung_retention_data exynos5433_fsys_retention_data __initconst = {
	.regs	 = exynos5433_fsys_retention_regs,
	.nr_regs = ARRAY_SIZE(exynos5433_fsys_retention_regs),
	.value	 = EXYNOS_WAKEUP_FROM_LOWPWR,
	.init	 = exynos_retention_init,
};

/*
 * Samsung pinctrl driver data for Exynos5433 SoC. Exynos5433 SoC includes
 * ten gpio/pin-mux/pinconfig controllers.
 */
static const struct samsung_pin_ctrl exynos5433_pin_ctrl[] __initconst = {
	{
		/* pin-controller instance 0 data */
		.pin_banks	= exynos5433_pin_banks0,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks0),
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.nr_ext_resources = 1,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 1 data */
		.pin_banks	= exynos5433_pin_banks1,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks1),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_audio_retention_data,
	}, {
		/* pin-controller instance 2 data */
		.pin_banks	= exynos5433_pin_banks2,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks2),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 3 data */
		.pin_banks	= exynos5433_pin_banks3,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 4 data */
		.pin_banks	= exynos5433_pin_banks4,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks4),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 5 data */
		.pin_banks	= exynos5433_pin_banks5,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks5),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_fsys_retention_data,
	}, {
		/* pin-controller instance 6 data */
		.pin_banks	= exynos5433_pin_banks6,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks6),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 7 data */
		.pin_banks	= exynos5433_pin_banks7,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks7),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 8 data */
		.pin_banks	= exynos5433_pin_banks8,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks8),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 9 data */
		.pin_banks	= exynos5433_pin_banks9,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks9),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	},
};

const struct samsung_pinctrl_of_match_data exynos5433_of_data __initconst = {
	.ctrl		= exynos5433_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(exynos5433_pin_ctrl),
};

/* pin banks of exynos7 pin-controller - ALIVE */
static const struct samsung_pin_bank_data exynos7_pin_banks0[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTW(8, 0x000, "gpa0", 0x00),
	EXYNOS_PIN_BANK_EINTW(8, 0x020, "gpa1", 0x04),
	EXYNOS_PIN_BANK_EINTW(8, 0x040, "gpa2", 0x08),
	EXYNOS_PIN_BANK_EINTW(8, 0x060, "gpa3", 0x0c),
};

/* pin banks of exynos7 pin-controller - BUS0 */
static const struct samsung_pin_bank_data exynos7_pin_banks1[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(5, 0x000, "gpb0", 0x00),
	EXYNOS_PIN_BANK_EINTG(8, 0x020, "gpc0", 0x04),
	EXYNOS_PIN_BANK_EINTG(2, 0x040, "gpc1", 0x08),
	EXYNOS_PIN_BANK_EINTG(6, 0x060, "gpc2", 0x0c),
	EXYNOS_PIN_BANK_EINTG(8, 0x080, "gpc3", 0x10),
	EXYNOS_PIN_BANK_EINTG(4, 0x0a0, "gpd0", 0x14),
	EXYNOS_PIN_BANK_EINTG(6, 0x0c0, "gpd1", 0x18),
	EXYNOS_PIN_BANK_EINTG(8, 0x0e0, "gpd2", 0x1c),
	EXYNOS_PIN_BANK_EINTG(5, 0x100, "gpd4", 0x20),
	EXYNOS_PIN_BANK_EINTG(4, 0x120, "gpd5", 0x24),
	EXYNOS_PIN_BANK_EINTG(6, 0x140, "gpd6", 0x28),
	EXYNOS_PIN_BANK_EINTG(3, 0x160, "gpd7", 0x2c),
	EXYNOS_PIN_BANK_EINTG(2, 0x180, "gpd8", 0x30),
	EXYNOS_PIN_BANK_EINTG(2, 0x1a0, "gpg0", 0x34),
	EXYNOS_PIN_BANK_EINTG(4, 0x1c0, "gpg3", 0x38),
};

/* pin banks of exynos7 pin-controller - NFC */
static const struct samsung_pin_bank_data exynos7_pin_banks2[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(3, 0x000, "gpj0", 0x00),
};

/* pin banks of exynos7 pin-controller - TOUCH */
static const struct samsung_pin_bank_data exynos7_pin_banks3[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(3, 0x000, "gpj1", 0x00),
};

/* pin banks of exynos7 pin-controller - FF */
static const struct samsung_pin_bank_data exynos7_pin_banks4[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(4, 0x000, "gpg4", 0x00),
};

/* pin banks of exynos7 pin-controller - ESE */
static const struct samsung_pin_bank_data exynos7_pin_banks5[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(5, 0x000, "gpv7", 0x00),
};

/* pin banks of exynos7 pin-controller - FSYS0 */
static const struct samsung_pin_bank_data exynos7_pin_banks6[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(7, 0x000, "gpr4", 0x00),
};

/* pin banks of exynos7 pin-controller - FSYS1 */
static const struct samsung_pin_bank_data exynos7_pin_banks7[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(4, 0x000, "gpr0", 0x00),
	EXYNOS_PIN_BANK_EINTG(8, 0x020, "gpr1", 0x04),
	EXYNOS_PIN_BANK_EINTG(5, 0x040, "gpr2", 0x08),
	EXYNOS_PIN_BANK_EINTG(8, 0x060, "gpr3", 0x0c),
};

/* pin banks of exynos7 pin-controller - BUS1 */
static const struct samsung_pin_bank_data exynos7_pin_banks8[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(8, 0x020, "gpf0", 0x00),
	EXYNOS_PIN_BANK_EINTG(8, 0x040, "gpf1", 0x04),
	EXYNOS_PIN_BANK_EINTG(4, 0x060, "gpf2", 0x08),
	EXYNOS_PIN_BANK_EINTG(5, 0x080, "gpf3", 0x0c),
	EXYNOS_PIN_BANK_EINTG(8, 0x0a0, "gpf4", 0x10),
	EXYNOS_PIN_BANK_EINTG(8, 0x0c0, "gpf5", 0x14),
	EXYNOS_PIN_BANK_EINTG(5, 0x0e0, "gpg1", 0x18),
	EXYNOS_PIN_BANK_EINTG(5, 0x100, "gpg2", 0x1c),
	EXYNOS_PIN_BANK_EINTG(6, 0x120, "gph1", 0x20),
	EXYNOS_PIN_BANK_EINTG(3, 0x140, "gpv6", 0x24),
};

static const struct samsung_pin_bank_data exynos7_pin_banks9[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS_PIN_BANK_EINTG(7, 0x000, "gpz0", 0x00),
	EXYNOS_PIN_BANK_EINTG(4, 0x020, "gpz1", 0x04),
};

static const struct samsung_pin_ctrl exynos7_pin_ctrl[] __initconst = {
	{
		/* pin-controller instance 0 Alive data */
		.pin_banks	= exynos7_pin_banks0,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks0),
		.eint_wkup_init = exynos_eint_wkup_init,
	}, {
		/* pin-controller instance 1 BUS0 data */
		.pin_banks	= exynos7_pin_banks1,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks1),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 2 NFC data */
		.pin_banks	= exynos7_pin_banks2,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks2),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 3 TOUCH data */
		.pin_banks	= exynos7_pin_banks3,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 4 FF data */
		.pin_banks	= exynos7_pin_banks4,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks4),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 5 ESE data */
		.pin_banks	= exynos7_pin_banks5,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks5),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 6 FSYS0 data */
		.pin_banks	= exynos7_pin_banks6,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks6),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 7 FSYS1 data */
		.pin_banks	= exynos7_pin_banks7,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks7),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 8 BUS1 data */
		.pin_banks	= exynos7_pin_banks8,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks8),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 9 AUD data */
		.pin_banks	= exynos7_pin_banks9,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks9),
		.eint_gpio_init = exynos_eint_gpio_init,
	},
};

const struct samsung_pinctrl_of_match_data exynos7_of_data __initconst = {
	.ctrl		= exynos7_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(exynos7_pin_ctrl),
};

/* pin banks of exynos7885 pin-controller 0 (ALIVE) */
static const struct samsung_pin_bank_data exynos7885_pin_banks0[] __initconst = {
	EXYNOS_PIN_BANK_EINTN(3, 0x000, "etc0"),
	EXYNOS_PIN_BANK_EINTN(3, 0x020, "etc1"),
	EXYNOS850_PIN_BANK_EINTW(8, 0x040, "gpa0", 0x00),
	EXYNOS850_PIN_BANK_EINTW(8, 0x060, "gpa1", 0x04),
	EXYNOS850_PIN_BANK_EINTW(8, 0x080, "gpa2", 0x08),
	EXYNOS850_PIN_BANK_EINTW(5, 0x0a0, "gpq0", 0x0c),
};

/* pin banks of exynos7885 pin-controller 1 (DISPAUD) */
static const struct samsung_pin_bank_data exynos7885_pin_banks1[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(5, 0x000, "gpb0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(4, 0x020, "gpb1", 0x04),
	EXYNOS850_PIN_BANK_EINTG(5, 0x040, "gpb2", 0x08),
};

/* pin banks of exynos7885 pin-controller 2 (FSYS) */
static const struct samsung_pin_bank_data exynos7885_pin_banks2[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(4, 0x000, "gpf0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(8, 0x020, "gpf2", 0x04),
	EXYNOS850_PIN_BANK_EINTG(6, 0x040, "gpf3", 0x08),
	EXYNOS850_PIN_BANK_EINTG(6, 0x060, "gpf4", 0x0c),
};

/* pin banks of exynos7885 pin-controller 3 (TOP) */
static const struct samsung_pin_bank_data exynos7885_pin_banks3[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(4, 0x000, "gpp0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(3, 0x020, "gpg0", 0x04),
	EXYNOS850_PIN_BANK_EINTG(4, 0x040, "gpp1", 0x08),
	EXYNOS850_PIN_BANK_EINTG(4, 0x060, "gpp2", 0x0c),
	EXYNOS850_PIN_BANK_EINTG(3, 0x080, "gpp3", 0x10),
	EXYNOS850_PIN_BANK_EINTG(6, 0x0a0, "gpp4", 0x14),
	EXYNOS850_PIN_BANK_EINTG(4, 0x0c0, "gpp5", 0x18),
	EXYNOS850_PIN_BANK_EINTG(5, 0x0e0, "gpp6", 0x1c),
	EXYNOS850_PIN_BANK_EINTG(2, 0x100, "gpp7", 0x20),
	EXYNOS850_PIN_BANK_EINTG(2, 0x120, "gpp8", 0x24),
	EXYNOS850_PIN_BANK_EINTG(8, 0x140, "gpg1", 0x28),
	EXYNOS850_PIN_BANK_EINTG(8, 0x160, "gpg2", 0x2c),
	EXYNOS850_PIN_BANK_EINTG(8, 0x180, "gpg3", 0x30),
	EXYNOS850_PIN_BANK_EINTG(2, 0x1a0, "gpg4", 0x34),
	EXYNOS850_PIN_BANK_EINTG(4, 0x1c0, "gpc0", 0x38),
	EXYNOS850_PIN_BANK_EINTG(8, 0x1e0, "gpc1", 0x3c),
	EXYNOS850_PIN_BANK_EINTG(8, 0x200, "gpc2", 0x40),
};

static const struct samsung_pin_ctrl exynos7885_pin_ctrl[] __initconst = {
	{
		/* pin-controller instance 0 Alive data */
		.pin_banks	= exynos7885_pin_banks0,
		.nr_banks	= ARRAY_SIZE(exynos7885_pin_banks0),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 1 DISPAUD data */
		.pin_banks	= exynos7885_pin_banks1,
		.nr_banks	= ARRAY_SIZE(exynos7885_pin_banks1),
	}, {
		/* pin-controller instance 2 FSYS data */
		.pin_banks	= exynos7885_pin_banks2,
		.nr_banks	= ARRAY_SIZE(exynos7885_pin_banks2),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 3 TOP data */
		.pin_banks	= exynos7885_pin_banks3,
		.nr_banks	= ARRAY_SIZE(exynos7885_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	},
};

const struct samsung_pinctrl_of_match_data exynos7885_of_data __initconst = {
	.ctrl		= exynos7885_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(exynos7885_pin_ctrl),
};

/* pin banks of exynos850 pin-controller 0 (ALIVE) */
static const struct samsung_pin_bank_data exynos850_pin_banks0[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS850_PIN_BANK_EINTW(8, 0x000, "gpa0", 0x00),
	EXYNOS850_PIN_BANK_EINTW(8, 0x020, "gpa1", 0x04),
	EXYNOS850_PIN_BANK_EINTW(8, 0x040, "gpa2", 0x08),
	EXYNOS850_PIN_BANK_EINTW(8, 0x060, "gpa3", 0x0c),
	EXYNOS850_PIN_BANK_EINTW(4, 0x080, "gpa4", 0x10),
	EXYNOS850_PIN_BANK_EINTN(3, 0x0a0, "gpq0"),
};

/* pin banks of exynos850 pin-controller 1 (CMGP) */
static const struct samsung_pin_bank_data exynos850_pin_banks1[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS850_PIN_BANK_EINTW(1, 0x000, "gpm0", 0x00),
	EXYNOS850_PIN_BANK_EINTW(1, 0x020, "gpm1", 0x04),
	EXYNOS850_PIN_BANK_EINTW(1, 0x040, "gpm2", 0x08),
	EXYNOS850_PIN_BANK_EINTW(1, 0x060, "gpm3", 0x0c),
	EXYNOS850_PIN_BANK_EINTW(1, 0x080, "gpm4", 0x10),
	EXYNOS850_PIN_BANK_EINTW(1, 0x0a0, "gpm5", 0x14),
	EXYNOS850_PIN_BANK_EINTW(1, 0x0c0, "gpm6", 0x18),
	EXYNOS850_PIN_BANK_EINTW(1, 0x0e0, "gpm7", 0x1c),
};

/* pin banks of exynos850 pin-controller 2 (AUD) */
static const struct samsung_pin_bank_data exynos850_pin_banks2[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS850_PIN_BANK_EINTG(5, 0x000, "gpb0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(5, 0x020, "gpb1", 0x04),
};

/* pin banks of exynos850 pin-controller 3 (HSI) */
static const struct samsung_pin_bank_data exynos850_pin_banks3[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS850_PIN_BANK_EINTG(6, 0x000, "gpf2", 0x00),
};

/* pin banks of exynos850 pin-controller 4 (CORE) */
static const struct samsung_pin_bank_data exynos850_pin_banks4[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS850_PIN_BANK_EINTG(4, 0x000, "gpf0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(8, 0x020, "gpf1", 0x04),
};

/* pin banks of exynos850 pin-controller 5 (PERI) */
static const struct samsung_pin_bank_data exynos850_pin_banks5[] __initconst = {
	/* Must start with EINTG banks, ordered by EINT group number. */
	EXYNOS850_PIN_BANK_EINTG(2, 0x000, "gpg0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(6, 0x020, "gpp0", 0x04),
	EXYNOS850_PIN_BANK_EINTG(4, 0x040, "gpp1", 0x08),
	EXYNOS850_PIN_BANK_EINTG(4, 0x060, "gpp2", 0x0c),
	EXYNOS850_PIN_BANK_EINTG(8, 0x080, "gpg1", 0x10),
	EXYNOS850_PIN_BANK_EINTG(8, 0x0a0, "gpg2", 0x14),
	EXYNOS850_PIN_BANK_EINTG(1, 0x0c0, "gpg3", 0x18),
	EXYNOS850_PIN_BANK_EINTG(3, 0x0e0, "gpc0", 0x1c),
	EXYNOS850_PIN_BANK_EINTG(6, 0x100, "gpc1", 0x20),
};

static const struct samsung_pin_ctrl exynos850_pin_ctrl[] __initconst = {
	{
		/* pin-controller instance 0 ALIVE data */
		.pin_banks	= exynos850_pin_banks0,
		.nr_banks	= ARRAY_SIZE(exynos850_pin_banks0),
		.eint_wkup_init = exynos_eint_wkup_init,
	}, {
		/* pin-controller instance 1 CMGP data */
		.pin_banks	= exynos850_pin_banks1,
		.nr_banks	= ARRAY_SIZE(exynos850_pin_banks1),
		.eint_wkup_init = exynos_eint_wkup_init,
	}, {
		/* pin-controller instance 2 AUD data */
		.pin_banks	= exynos850_pin_banks2,
		.nr_banks	= ARRAY_SIZE(exynos850_pin_banks2),
	}, {
		/* pin-controller instance 3 HSI data */
		.pin_banks	= exynos850_pin_banks3,
		.nr_banks	= ARRAY_SIZE(exynos850_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 4 CORE data */
		.pin_banks	= exynos850_pin_banks4,
		.nr_banks	= ARRAY_SIZE(exynos850_pin_banks4),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 5 PERI data */
		.pin_banks	= exynos850_pin_banks5,
		.nr_banks	= ARRAY_SIZE(exynos850_pin_banks5),
		.eint_gpio_init = exynos_eint_gpio_init,
	},
};

const struct samsung_pinctrl_of_match_data exynos850_of_data __initconst = {
	.ctrl		= exynos850_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(exynos850_pin_ctrl),
};

/* pin banks of exynosautov9 pin-controller 0 (ALIVE) */
static const struct samsung_pin_bank_data exynosautov9_pin_banks0[] __initconst = {
	EXYNOS850_PIN_BANK_EINTW(8, 0x000, "gpa0", 0x00),
	EXYNOS850_PIN_BANK_EINTW(2, 0x020, "gpa1", 0x04),
	EXYNOS850_PIN_BANK_EINTN(2, 0x040, "gpq0"),
};

/* pin banks of exynosautov9 pin-controller 1 (AUD) */
static const struct samsung_pin_bank_data exynosautov9_pin_banks1[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(5, 0x000, "gpb0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(8, 0x020, "gpb1", 0x04),
	EXYNOS850_PIN_BANK_EINTG(8, 0x040, "gpb2", 0x08),
	EXYNOS850_PIN_BANK_EINTG(8, 0x060, "gpb3", 0x0C),
};

/* pin banks of exynosautov9 pin-controller 2 (FSYS0) */
static const struct samsung_pin_bank_data exynosautov9_pin_banks2[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(6, 0x000, "gpf0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(6, 0x020, "gpf1", 0x04),
};

/* pin banks of exynosautov9 pin-controller 3 (FSYS1) */
static const struct samsung_pin_bank_data exynosautov9_pin_banks3[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(6, 0x000, "gpf8", 0x00),
};

/* pin banks of exynosautov9 pin-controller 4 (FSYS2) */
static const struct samsung_pin_bank_data exynosautov9_pin_banks4[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(4, 0x000, "gpf2", 0x00),
	EXYNOS850_PIN_BANK_EINTG(8, 0x020, "gpf3", 0x04),
	EXYNOS850_PIN_BANK_EINTG(7, 0x040, "gpf4", 0x08),
	EXYNOS850_PIN_BANK_EINTG(8, 0x060, "gpf5", 0x0C),
	EXYNOS850_PIN_BANK_EINTG(7, 0x080, "gpf6", 0x10),
};

/* pin banks of exynosautov9 pin-controller 5 (PERIC0) */
static const struct samsung_pin_bank_data exynosautov9_pin_banks5[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(8, 0x000, "gpp0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(8, 0x020, "gpp1", 0x04),
	EXYNOS850_PIN_BANK_EINTG(8, 0x040, "gpp2", 0x08),
	EXYNOS850_PIN_BANK_EINTG(5, 0x060, "gpg0", 0x0C),
};

/* pin banks of exynosautov9 pin-controller 6 (PERIC1) */
static const struct samsung_pin_bank_data exynosautov9_pin_banks6[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(8, 0x000, "gpp3", 0x00),
	EXYNOS850_PIN_BANK_EINTG(8, 0x020, "gpp4", 0x04),
	EXYNOS850_PIN_BANK_EINTG(8, 0x040, "gpp5", 0x08),
	EXYNOS850_PIN_BANK_EINTG(8, 0x060, "gpg1", 0x0C),
	EXYNOS850_PIN_BANK_EINTG(8, 0x080, "gpg2", 0x10),
	EXYNOS850_PIN_BANK_EINTG(4, 0x0A0, "gpg3", 0x14),
};

static const struct samsung_pin_ctrl exynosautov9_pin_ctrl[] __initconst = {
	{
		/* pin-controller instance 0 ALIVE data */
		.pin_banks      = exynosautov9_pin_banks0,
		.nr_banks       = ARRAY_SIZE(exynosautov9_pin_banks0),
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend        = exynos_pinctrl_suspend,
		.resume         = exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 1 AUD data */
		.pin_banks      = exynosautov9_pin_banks1,
		.nr_banks       = ARRAY_SIZE(exynosautov9_pin_banks1),
	}, {
		/* pin-controller instance 2 FSYS0 data */
		.pin_banks      = exynosautov9_pin_banks2,
		.nr_banks       = ARRAY_SIZE(exynosautov9_pin_banks2),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend        = exynos_pinctrl_suspend,
		.resume         = exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 3 FSYS1 data */
		.pin_banks      = exynosautov9_pin_banks3,
		.nr_banks       = ARRAY_SIZE(exynosautov9_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend        = exynos_pinctrl_suspend,
		.resume         = exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 4 FSYS2 data */
		.pin_banks      = exynosautov9_pin_banks4,
		.nr_banks       = ARRAY_SIZE(exynosautov9_pin_banks4),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend        = exynos_pinctrl_suspend,
		.resume         = exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 5 PERIC0 data */
		.pin_banks      = exynosautov9_pin_banks5,
		.nr_banks       = ARRAY_SIZE(exynosautov9_pin_banks5),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend        = exynos_pinctrl_suspend,
		.resume         = exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 6 PERIC1 data */
		.pin_banks      = exynosautov9_pin_banks6,
		.nr_banks       = ARRAY_SIZE(exynosautov9_pin_banks6),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend        = exynos_pinctrl_suspend,
		.resume         = exynos_pinctrl_resume,
	},
};

const struct samsung_pinctrl_of_match_data exynosautov9_of_data __initconst = {
	.ctrl		= exynosautov9_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(exynosautov9_pin_ctrl),
};

/*
 * Pinctrl driver data for Tesla FSD SoC. FSD SoC includes three
 * gpio/pin-mux/pinconfig controllers.
 */

/* pin banks of FSD pin-controller 0 (FSYS) */
static const struct samsung_pin_bank_data fsd_pin_banks0[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(7, 0x00, "gpf0", 0x00),
	EXYNOS850_PIN_BANK_EINTG(8, 0x20, "gpf1", 0x04),
	EXYNOS850_PIN_BANK_EINTG(3, 0x40, "gpf6", 0x08),
	EXYNOS850_PIN_BANK_EINTG(2, 0x60, "gpf4", 0x0c),
	EXYNOS850_PIN_BANK_EINTG(6, 0x80, "gpf5", 0x10),
};

/* pin banks of FSD pin-controller 1 (PERIC) */
static const struct samsung_pin_bank_data fsd_pin_banks1[] __initconst = {
	EXYNOS850_PIN_BANK_EINTG(4, 0x000, "gpc8", 0x00),
	EXYNOS850_PIN_BANK_EINTG(7, 0x020, "gpf2", 0x04),
	EXYNOS850_PIN_BANK_EINTG(8, 0x040, "gpf3", 0x08),
	EXYNOS850_PIN_BANK_EINTG(8, 0x060, "gpd0", 0x0c),
	EXYNOS850_PIN_BANK_EINTG(8, 0x080, "gpb0", 0x10),
	EXYNOS850_PIN_BANK_EINTG(8, 0x0a0, "gpb1", 0x14),
	EXYNOS850_PIN_BANK_EINTG(8, 0x0c0, "gpb4", 0x18),
	EXYNOS850_PIN_BANK_EINTG(4, 0x0e0, "gpb5", 0x1c),
	EXYNOS850_PIN_BANK_EINTG(8, 0x100, "gpb6", 0x20),
	EXYNOS850_PIN_BANK_EINTG(8, 0x120, "gpb7", 0x24),
	EXYNOS850_PIN_BANK_EINTG(5, 0x140, "gpd1", 0x28),
	EXYNOS850_PIN_BANK_EINTG(5, 0x160, "gpd2", 0x2c),
	EXYNOS850_PIN_BANK_EINTG(7, 0x180, "gpd3", 0x30),
	EXYNOS850_PIN_BANK_EINTG(8, 0x1a0, "gpg0", 0x34),
	EXYNOS850_PIN_BANK_EINTG(8, 0x1c0, "gpg1", 0x38),
	EXYNOS850_PIN_BANK_EINTG(8, 0x1e0, "gpg2", 0x3c),
	EXYNOS850_PIN_BANK_EINTG(8, 0x200, "gpg3", 0x40),
	EXYNOS850_PIN_BANK_EINTG(8, 0x220, "gpg4", 0x44),
	EXYNOS850_PIN_BANK_EINTG(8, 0x240, "gpg5", 0x48),
	EXYNOS850_PIN_BANK_EINTG(8, 0x260, "gpg6", 0x4c),
	EXYNOS850_PIN_BANK_EINTG(8, 0x280, "gpg7", 0x50),
};

/* pin banks of FSD pin-controller 2 (PMU) */
static const struct samsung_pin_bank_data fsd_pin_banks2[] __initconst = {
	EXYNOS850_PIN_BANK_EINTN(3, 0x00, "gpq0"),
};

static const struct samsung_pin_ctrl fsd_pin_ctrl[] __initconst = {
	{
		/* pin-controller instance 0 FSYS0 data */
		.pin_banks	= fsd_pin_banks0,
		.nr_banks	= ARRAY_SIZE(fsd_pin_banks0),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 1 PERIC data */
		.pin_banks	= fsd_pin_banks1,
		.nr_banks	= ARRAY_SIZE(fsd_pin_banks1),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 2 PMU data */
		.pin_banks	= fsd_pin_banks2,
		.nr_banks	= ARRAY_SIZE(fsd_pin_banks2),
	},
};

const struct samsung_pinctrl_of_match_data fsd_of_data __initconst = {
	.ctrl		= fsd_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(fsd_pin_ctrl),
};

/*
 * pinctrl define for S5E9935
 */
/* pin banks of s5e9935 pin-controller (GPIO_ALIVE=0x15850000) */
static struct samsung_pin_bank_data s5e9935_pin_alive[] = {
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x0,   "gpa0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x20,  "gpa1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x40,  "gpa2", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x60,  "gpa3", 0x0c, 0x18),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x80,  "gpa4", 0x10, 0x20),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 2, 0xa0,  "gpq1"),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 1, 0xc0,  "gpq2"),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0xe0,  "gpm14", 0x14, 0x24),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x100, "gpm20", 0x18, 0x28),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x120, "gpm21", 0x1c, 0x2c),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x140, "gpm22", 0x20, 0x30),
};

/* pin banks of s5e9935 pin-controller (GPIO_CMGP=0x14E30000) */
static struct samsung_pin_bank_data s5e9935_pin_cmgp[] = {
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x0,   "gpm0",  0x00, 0x00, 0x288, 0),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x20,  "gpm1",  0x04, 0x04, 0x288, 2),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x40,  "gpm2",  0x08, 0x08, 0x288, 4),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x60,  "gpm3",  0x0c, 0x0c, 0x288, 6),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x80,  "gpm4",  0x10, 0x10, 0x288, 8),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0xa0,  "gpm5",  0x14, 0x14, 0x288, 10),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0xc0,  "gpm6",  0x18, 0x18, 0x288, 12),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0xe0,  "gpm7",  0x1c, 0x1c, 0x288, 14),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x100, "gpm8",  0x20, 0x20, 0x288, 16),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x120, "gpm9",  0x24, 0x24, 0x288, 18),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x140, "gpm10", 0x28, 0x28, 0x288, 20),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x160, "gpm11", 0x2c, 0x2c, 0x288, 22),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x180, "gpm12", 0x30, 0x30, 0x288, 24),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x1a0, "gpm13", 0x34, 0x34, 0x288, 26),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1c0, "gpm15", 0x38, 0x38, 0x288, 28),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1e0, "gpm16", 0x3c, 0x3c, 0x288, 29),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x200, "gpm17", 0x40, 0x40, 0x288, 30),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 4, 0x220, "gpq0"),
};

/* pin banks of s5e9935 pin-controller (GPIO_HSI1=0x11230000) */
static struct samsung_pin_bank_data s5e9935_pin_hsi1[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x0, "gpf0", 0x00, 0x00),
};

/* pin banks of s5e9935 pin-controller (GPIO_UFS=0x11040000) */
static struct samsung_pin_bank_data s5e9935_pin_ufs[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 7, 0x0, "gpf1", 0x00, 0x00),
};

/* pin banks of s5e9935 pin-controller (GPIO_HSI1UFS=0x11060000) */
static struct samsung_pin_bank_data s5e9935_pin_hsi1ufs[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x0, "gpf2", 0x00, 0x00),
};

/* pin banks of s5e9935 pin-controller (GPIO_PERIC0=0x10430000) */
static struct samsung_pin_bank_data s5e9935_pin_peric0[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x0,   "gpp4",  0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x20,  "gpp10",  0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x40,  "gpc0",  0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x60,  "gpc1",  0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x80,  "gpc2",  0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0xa0,  "gpc4",  0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0xc0,  "gpc5",  0x18, 0x18),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 3, 0xe0,  "gpg0",  0x1c, 0x1c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 8, 0x100, "gpg1",  0x20, 0x20),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x120, "gpg2",  0x24, 0x28),
};

/* pin banks of s5e9935 pin-controller (GPIO_PERIC1=0x10730000) */
static struct samsung_pin_bank_data s5e9935_pin_peric1[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x0,  "gpp7",  0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x20, "gpp8",  0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x40, "gpp9",  0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x60, "gpc10", 0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x80, "gpb0",  0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xa0, "gpb1",  0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xc0, "gpb2",  0x18, 0x18),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 1, 0xe0, "gpb3",  0x1c, 0x1c),
};

/* pin banks of s5e9935 pin-controller (GPIO_PERIC2=0x11C30000) */
static struct samsung_pin_bank_data s5e9935_pin_peric2[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x0,   "gpp0",  0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x20,  "gpp1",  0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x40,  "gpp2",  0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x60,  "gpp3",  0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x80,  "gpp5",  0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xa0,  "gpp6",  0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xc0,  "gpp11", 0x18, 0x18),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0xe0,  "gpc3",  0x1c, 0x1c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x100, "gpc6",  0x20, 0x20),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x120, "gpc7",  0x24, 0x24),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x140, "gpc8",  0x28, 0x28),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x160, "gpc9",  0x2c, 0x2c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 1, 0x180, "gpg3",  0x30, 0x30),
};

/* pin banks of s5e9935 pin-controller (GPIO_VTS=0x15320000) */
static struct samsung_pin_bank_data s5e9935_pin_vts[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 7, 0x0, "gpv0", 0x00, 0x00),
};

/* pin banks of s5e9935 pin-controller (GPIO_CHUBVTS=0x149D0000) */
static struct samsung_pin_bank_data s5e9935_pin_chubvts[] = {
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x0,  "gph0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x20, "gph1", 0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x40, "gph2", 0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x60, "gph3", 0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x80, "gph4", 0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 7, 0xa0, "gph5", 0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0xc0, "gpb5", 0x18, 0x1c),
};

static struct samsung_pin_ctrl s5e9935_pin_ctrl[] = {
	{
		/* pin-controller instance 0 ALIVE data */
		.pin_banks	= s5e9935_pin_alive,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_alive),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 1 CMGP data */
		.pin_banks	= s5e9935_pin_cmgp,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_cmgp),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 2 HSI1 data */
		.pin_banks	= s5e9935_pin_hsi1,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_hsi1),
	//	.eint_gpio_init = exynos_eint_gpio_init,
	//	.suspend	= exynos_pinctrl_suspend,
	//	.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 3 UFS data */
		.pin_banks	= s5e9935_pin_ufs,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_ufs),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 4 HSI1UFS data */
		.pin_banks	= s5e9935_pin_hsi1ufs,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_hsi1ufs),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 5 PERIC0 data */
		.pin_banks	= s5e9935_pin_peric0,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_peric0),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 6 PERIC1 data */
		.pin_banks	= s5e9935_pin_peric1,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_peric1),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 7 PERIC2 data */
		.pin_banks	= s5e9935_pin_peric2,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_peric2),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* For this block, pinctrl will not care for interrupt and S2R */
		/* pin-controller instance 8 VTS data */
		.pin_banks	= s5e9935_pin_vts,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_vts),
	}, {
		/* pin-controller instance 9 CHUBVTS data */
		.pin_banks	= s5e9935_pin_chubvts,
		.nr_banks	= ARRAY_SIZE(s5e9935_pin_chubvts),
	},
};

const struct samsung_pinctrl_of_match_data s5e9935_of_data __initconst = {
	.ctrl		= s5e9935_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(s5e9935_pin_ctrl),
};

/*
 * pinctrl define for S5E9945
 */
/* pin banks of s5e9945 pin-controller (GPIO_ALIVE=0x12850000) */
static struct samsung_pin_bank_data s5e9945_pin_alive[] = {
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x0,   "gpa0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x20,  "gpa1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x40,  "gpa2", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x60,  "gpa3", 0x0c, 0x18),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x80,  "gpa4", 0x10, 0x20),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 2, 0xa0,  "gpq1"),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 1, 0xc0,  "gpq2"),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 2, 0xe0,  "gpq3"),
};

/* pin banks of s5e9945 pin-controller (GPIO_CMGP=0x14030000) */
static struct samsung_pin_bank_data s5e9945_pin_cmgp[] = {
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x0,   "gpm0",  0x00, 0x00, 0x288, 0),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x20,  "gpm1",  0x04, 0x04, 0x288, 2),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x40,  "gpm2",  0x08, 0x08, 0x288, 4),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x60,  "gpm3",  0x0c, 0x0c, 0x288, 6),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x80,  "gpm4",  0x10, 0x10, 0x288, 8),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0xa0,  "gpm5",  0x14, 0x14, 0x288, 10),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0xc0,  "gpm6",  0x18, 0x18, 0x288, 12),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0xe0,  "gpm7",  0x1c, 0x1c, 0x288, 14),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x100, "gpm8",  0x20, 0x20, 0x288, 16),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x120, "gpm9",  0x24, 0x24, 0x288, 18),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x140, "gpm10", 0x28, 0x28, 0x288, 20),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x160, "gpm11", 0x2c, 0x2c, 0x288, 22),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x180, "gpm12", 0x30, 0x30, 0x288, 24),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x1a0, "gpm13", 0x34, 0x34, 0x288, 26),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1c0, "gpm15", 0x38, 0x38, 0x288, 29),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1e0, "gpm16", 0x3c, 0x3c, 0x288, 30),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x200, "gpm17", 0x40, 0x40, 0x288, 31),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 2, 0x220, "gpm18", 0x44, 0x44, 0x2b8, 0),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 4, 0x240, "gpq0"),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x260, "gpm14", 0x48, 0x48, 0x288, 28),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x280, "gpm20", 0x4c, 0x4c, 0x2b8, 2),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x2a0, "gpm21", 0x50, 0x50, 0x2b8, 3),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x2c0, "gpm22", 0x54, 0x54, 0x2b8, 4),
};

/* pin banks of s5e9945 pin-controller (GPIO_HSI1=0x18030000) */
static struct samsung_pin_bank_data s5e9945_pin_hsi1[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x0, "gpf0", 0x00, 0x00),
};

/* pin banks of s5e9945 pin-controller (GPIO_UFS=0x17040000) */
static struct samsung_pin_bank_data s5e9945_pin_ufs[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 7, 0x0, "gpf1", 0x00, 0x00),
};

/* pin banks of s5e9945 pin-controller (GPIO_HSI1UFS=0x17060000) */
static struct samsung_pin_bank_data s5e9945_pin_hsi1ufs[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x0, "gpf2", 0x00, 0x00),
};

/* pin banks of s5e9945 pin-controller (GPIO_PERIC0=0x10830000) */
static struct samsung_pin_bank_data s5e9945_pin_peric0[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x0,   "gpp4",  0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x20,  "gpc0",  0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x40,  "gpc1",  0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 3, 0x60,  "gpg0",  0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 7, 0x80, "gpg1",  0x10, 0x10),
};

/* pin banks of s5e9945 pin-controller (GPIO_PERIC1=0x11030000) */
static struct samsung_pin_bank_data s5e9945_pin_peric1[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x0,  "gpp10",  0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x20,  "gpc2",  0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x40,  "gpc4",  0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x60,  "gpc5",  0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x80,  "gpg2",  0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xa0,  "gpp7",  0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xc0, "gpp8",  0x18, 0x18),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xe0, "gpp9",  0x1c, 0x1c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x100, "gpc10", 0x20, 0x20),
};

/* pin banks of s5e9945 pin-controller (GPIO_PERIC2=0x11830000) */
static struct samsung_pin_bank_data s5e9945_pin_peric2[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x0,   "gpb0",  0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x20,   "gpb1",  0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x40,   "gpb2",  0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x60,   "gpb3",  0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x80,   "gpp0",  0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xa0,  "gpp1",  0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xc0,  "gpp2",  0x18, 0x18),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xe0,  "gpp3",  0x1c, 0x1c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x100,  "gpp5",  0x20, 0x20),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x120,  "gpp6",  0x24, 0x24),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x140,  "gpp11", 0x28, 0x28),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x160,  "gpc3",  0x2c, 0x2c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x180, "gpc6",  0x30, 0x30),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x1a0, "gpc7",  0x34, 0x34),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x1c0, "gpc8",  0x38, 0x38),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x1e0, "gpc9",  0x3c, 0x3c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 1, 0x200, "gpg3",  0x40, 0x40),
};

/* pin banks of s5e9945 pin-controller (GPIO_VTS=0x13020000) */
static struct samsung_pin_bank_data s5e9945_pin_vts[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 7, 0x0, "gpv0", 0x00, 0x00),
};

/* pin banks of s5e9945 pin-controller (GPIO_CHUBVTS=0x13EA0000) */
static struct samsung_pin_bank_data s5e9945_pin_chubvts[] = {
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x0,  "gph0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x20, "gph1", 0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x40, "gph2", 0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x60, "gph3", 0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0x80, "gph6", 0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 7, 0xa0, "gph7", 0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 4, 0xc0, "gpb5", 0x18, 0x1c),
};

static struct samsung_pin_ctrl s5e9945_pin_ctrl[] = {
	{
		/* pin-controller instance 0 ALIVE data */
		.pin_banks	= s5e9945_pin_alive,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_alive),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 1 CMGP data */
		.pin_banks	= s5e9945_pin_cmgp,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_cmgp),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 2 HSI1 data */
		.pin_banks	= s5e9945_pin_hsi1,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_hsi1),
	}, {
		/* pin-controller instance 3 UFS data */
		.pin_banks	= s5e9945_pin_ufs,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_ufs),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 4 HSI1UFS data */
		.pin_banks	= s5e9945_pin_hsi1ufs,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_hsi1ufs),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 5 PERIC0 data */
		.pin_banks	= s5e9945_pin_peric0,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_peric0),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 6 PERIC1 data */
		.pin_banks	= s5e9945_pin_peric1,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_peric1),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 7 PERIC2 data */
		.pin_banks	= s5e9945_pin_peric2,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_peric2),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* For this block, pinctrl will not care for interrupt and S2R */
		/* pin-controller instance 8 VTS data */
		.pin_banks	= s5e9945_pin_vts,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_vts),
	}, {
		/* pin-controller instance 9 CHUBVTS data */
		.pin_banks	= s5e9945_pin_chubvts,
		.nr_banks	= ARRAY_SIZE(s5e9945_pin_chubvts),
	},
};

const struct samsung_pinctrl_of_match_data s5e9945_of_data __initconst = {
	.ctrl		= s5e9945_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(s5e9945_pin_ctrl),
};

/*
 * pinctrl define for S5E8845
 */
/* pin banks of s5e8845 pin-controller (GPIO_ALIVE=0x11850000) */
static struct samsung_pin_bank_data s5e8845_pin_alive[] = {
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 8, 0x00, "gpa0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 5, 0x20, "gpa1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 3, 0x40, "gpq0"),
	EXYNOS9_PIN_BANK_EINTN(exynos850_bank_type_alive, 2, 0x60, "gpq1"),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x80, "gpc0", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0xa0, "gpc1", 0x0c, 0x14),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0xc0, "gpc2", 0x10, 0x18),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0xe0, "gpc3", 0x14, 0x1c),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x100, "gpc4", 0x18, 0x20),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x120, "gpc5", 0x1c, 0x24),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x140, "gpc6", 0x20, 0x28),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x160, "gpc7", 0x24, 0x2c),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x180, "gpc8", 0x28, 0x30),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1a0, "gpc9", 0x2c, 0x34),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1c0, "gpc10", 0x30, 0x38),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1e0, "gpc11", 0x34, 0x3c),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x200, "gpc12", 0x38, 0x40),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x220, "gpc13", 0x3c, 0x44),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x240, "gpc14", 0x40, 0x48),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x260, "gpj0", 0x44, 0x4c),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x280, "gpj1", 0x48, 0x50),
	EXYNOS9_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x2a0, "gpj2", 0x4c, 0x54),
};

/* pin banks of s5e8845 pin-controller (GPIO_CMGP=0x11430000) */
static struct samsung_pin_bank_data s5e8845_pin_cmgp[] = {
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x00,  "gpm0",  0x00, 0x00, 0x288, 0),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x20,  "gpm1",  0x04, 0x04, 0x288, 1),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x40,  "gpm2",  0x08, 0x08, 0x288, 2),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x60,  "gpm3",  0x0c, 0x0c, 0x288, 3),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x80,  "gpm4",  0x10, 0x10, 0x288, 4),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0xa0,  "gpm5",  0x14, 0x14, 0x288, 5),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0xc0,  "gpm6",  0x18, 0x18, 0x288, 6),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0xe0,  "gpm7",  0x1c, 0x1c, 0x288, 7),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x100, "gpm8",  0x20, 0x20, 0x288, 8),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x120, "gpm9",  0x24, 0x24, 0x288, 9),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x140, "gpm10", 0x28, 0x28, 0x288, 10),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x160, "gpm11", 0x2c, 0x2c, 0x288, 11),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x180, "gpm12", 0x30, 0x30, 0x288, 12),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1a0, "gpm13", 0x34, 0x34, 0x288, 13),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1c0, "gpm14", 0x38, 0x38, 0x288, 14),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x1e0, "gpm15", 0x3c, 0x3c, 0x288, 15),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x200, "gpm16", 0x40, 0x40, 0x288, 16),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x220, "gpm17", 0x44, 0x44, 0x288, 17),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x240, "gpm18", 0x48, 0x48, 0x288, 18),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x260, "gpm19", 0x4c, 0x4c, 0x288, 19),
	EXYNOS_CMGP_PIN_BANK_EINTW(exynos850_bank_type_alive, 1, 0x280, "gpm20", 0x50, 0x50, 0x288, 20),
};

/* pin banks of s5e8845 pin-controller (GPIO_HSIUFS=0x13440000) */
static struct samsung_pin_bank_data s5e8845_pin_hsiufs[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x0, "gpf3", 0x00, 0x00),
};

/* pin banks of s5e8845 pin-controller (GPIO_PERIC=0x12b40000) */
static struct samsung_pin_bank_data s5e8845_pin_peric[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 8, 0x0,   "gpp0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 8, 0x20,  "gpp1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 6, 0x40,  "gpp2", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x60,  "gpg0", 0x0c, 0x18),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 3, 0x80,  "gpg1", 0x10, 0x1c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 6, 0xa0,  "gpb0", 0x14, 0x20),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0xc0,  "gpb1", 0x18, 0x28),
};

/* pin banks of s5e8845 pin-controller (GPIO_PERIMMC=0x12bf0000) */
static struct samsung_pin_bank_data s5e8845_pin_pericmmc[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 7, 0x0, "gpf2", 0x00, 0x00),
};

/* pin banks of s5e8845 pin-controller (GPIO_USI=0x13630000) */
static struct samsung_pin_bank_data s5e8845_pin_usi[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 8, 0x0, "gpp3", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x20, "gpp4", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 2, 0x40, "gpg2", 0x08, 0x0c),
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 1, 0x60, "gpg3", 0x0c, 0x10),
};

/* pin banks of s5e8845 pin-controller (GPIO_VTS=0x11780000) */
static struct samsung_pin_bank_data s5e8845_pin_vts[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos850_bank_type_off, 4, 0x0, "gpv0", 0x00, 0x00),
};

static struct samsung_pin_ctrl s5e8845_pin_ctrl[] = {
	{
	/* pin-controller instance 0 ALIVE data */
		.pin_banks	= s5e8845_pin_alive,
		.nr_banks	= ARRAY_SIZE(s5e8845_pin_alive),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
	/* pin-controller instance 1 CMGP data */
		.pin_banks      = s5e8845_pin_cmgp,
		.nr_banks       = ARRAY_SIZE(s5e8845_pin_cmgp),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend        = exynos_pinctrl_suspend,
		.resume         = exynos_pinctrl_resume,
        }, {
	/* pin-controller instance 2 HSIUFS data */
		.pin_banks	= s5e8845_pin_hsiufs,
		.nr_banks	= ARRAY_SIZE(s5e8845_pin_hsiufs),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
	/* pin-controller instance 3 PERIC data */
		.pin_banks	= s5e8845_pin_peric,
		.nr_banks	= ARRAY_SIZE(s5e8845_pin_peric),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
	/* pin-controller instance 4 PERIC MMC data */
		.pin_banks	= s5e8845_pin_pericmmc,
		.nr_banks	= ARRAY_SIZE(s5e8845_pin_pericmmc),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
	/* pin-controller instance 5 USI data */
		.pin_banks	= s5e8845_pin_usi,
		.nr_banks	= ARRAY_SIZE(s5e8845_pin_usi),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
	/* pin-controller instance 6 VTS data */
		.pin_banks	= s5e8845_pin_vts,
		.nr_banks	= ARRAY_SIZE(s5e8845_pin_vts),
	},
};

const struct samsung_pinctrl_of_match_data s5e8845_of_data __initconst = {
	.ctrl		= s5e8845_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(s5e8845_pin_ctrl),
};

/* pin banks of s5e5535 pin-controller 0 (GPIO_ALIVE 0x12840000) */
static struct samsung_pin_bank_data s5e5535_pin_alive[] = {
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 8, 0x000, "gpa0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 8, 0x020, "gpa1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 2, 0x040, "gpa2", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 1, 0x060, "gpa3", 0x0c, 0x14),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 1, 0x080, "gpa4", 0x10, 0x18),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 1, 0x0a0, "gpa5", 0x14, 0x1c),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 3, 0x0c0, "gpq0", 0x18, 0x20),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 2, 0x0e0, "gpq1", 0x1c, 0x24),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 1, 0x100, "gpj0", 0x20, 0x28),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 1, 0x120, "gpj1", 0x24, 0x2c),
	EXYNOS9_PIN_BANK_EINTW(bank_type_7, 1, 0x140, "gpj2", 0x28, 0x30),
};

/* pin banks of s5e5535 pin-controller 1 (GPIO_AOD 0x13270000) */
static struct samsung_pin_bank_data s5e5535_pin_aod[] = {
	EXYNOS9_PIN_BANK_EINTN(bank_type_7, 6, 0x000, "gpd0"),
};

/* pin banks of s5e5535 pin-controller 2 (GPIO_DISPAUD 0x14e60000) */
static struct samsung_pin_bank_data s5e5535_pin_dispaud[] = {
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 5, 0x000, "gpb0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 4, 0x020, "gpb1", 0x04, 0x08),
};

/* pin banks of s5e5535 pin-controller 3 (GPIO_HSI 0x10a50000) */
static struct samsung_pin_bank_data s5e5535_pin_hsi[] = {
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 4, 0x000, "gpf0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 8, 0x020, "gpf1", 0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 7, 0x040, "gpf2", 0x08, 0x0c),
};

/* pin banks of s5e5535 pin-controller 4 (GPIO_PERIC 0x10290000) */
static struct samsung_pin_bank_data s5e5535_pin_peric[] = {
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x000, "gpp0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x020, "gpp1", 0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x040, "gpp2", 0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x060, "gpp3", 0x0c, 0x0c),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 4, 0x080, "gpp4", 0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 4, 0x0a0, "gpc0", 0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x0c0, "gpg0", 0x18, 0x18),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x0e0, "gpg1", 0x1c, 0x1c),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x100, "gpg2", 0x20, 0x20),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x120, "gpg3", 0x24, 0x24),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x140, "gpg4", 0x28, 0x28),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x160, "gpg5", 0x2c, 0x2c),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x180, "gpg6", 0x30, 0x30),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 2, 0x1a0, "gpg7", 0x34, 0x34),
	EXYNOS9_PIN_BANK_EINTG(bank_type_6, 1, 0x1c0, "gpg8", 0x38, 0x38),
};

/* pin banks of s5e5535 pin-controller 5 (GPIO_VTS 0x11020000) */
static struct samsung_pin_bank_data s5e5535_pin_vts[] = {
	EXYNOS9_PIN_BANK_EINTN(bank_type_6, 2, 0x000, "gpv0"),
	EXYNOS9_PIN_BANK_EINTN(bank_type_6, 2, 0x020, "gpv1"),
};

const struct samsung_pin_ctrl s5e5535_pin_ctrl[] = {
	{
		/* pin-controller instance 0 ALIVE data */
		.pin_banks	= s5e5535_pin_alive,
		.nr_banks	= ARRAY_SIZE(s5e5535_pin_alive),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 1 AOD data */
		.pin_banks	= s5e5535_pin_aod,
		.nr_banks	= ARRAY_SIZE(s5e5535_pin_aod),
		//.eint_gpio_init = exynos_eint_gpio_init,
		//.eint_wkup_init = exynos_eint_wkup_init,
		//.suspend	= exynos_pinctrl_suspend,
		//.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 2 DISP data */
		.pin_banks	= s5e5535_pin_dispaud,
		.nr_banks	= ARRAY_SIZE(s5e5535_pin_dispaud),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume	= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 3 HSI data */
		.pin_banks	= s5e5535_pin_hsi,
		.nr_banks	= ARRAY_SIZE(s5e5535_pin_hsi),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 4 PERIC data */
		.pin_banks	= s5e5535_pin_peric,
		.nr_banks	= ARRAY_SIZE(s5e5535_pin_peric),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 5 VTS data */
		.pin_banks	= s5e5535_pin_vts,
		.nr_banks	= ARRAY_SIZE(s5e5535_pin_vts),
		//.eint_gpio_init = exynos_eint_gpio_init,
		//.suspend	= exynos_pinctrl_suspend,
		//.resume	= exynos_pinctrl_resume,
	},
};

const struct samsung_pinctrl_of_match_data s5e5535_of_data __initconst = {
	.ctrl		= s5e5535_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(s5e5535_pin_ctrl),
};
MODULE_LICENSE("GPL");
