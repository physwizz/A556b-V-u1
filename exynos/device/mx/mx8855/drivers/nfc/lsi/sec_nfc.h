/*
 * SAMSUNG NFC N2 Controller
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Author: Woonki Lee <woonki84.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 */

#include <linux/platform_device.h>

#include <linux/clk.h>

#define SEC_NFC_DRIVER_NAME		"sec-nfc"

#define SEC_NFC_MAX_BUFFER_SIZE	512

/* ioctl */
#define SEC_NFC_MAGIC	'S'
#define SEC_NFC_GET_MODE		_IOW(SEC_NFC_MAGIC, 0, unsigned int)
#define SEC_NFC_SET_MODE		_IOW(SEC_NFC_MAGIC, 1, unsigned int)
#define SEC_NFC_SLEEP			_IOW(SEC_NFC_MAGIC, 2, unsigned int)
#define SEC_NFC_WAKEUP			_IOW(SEC_NFC_MAGIC, 3, unsigned int)
#define SEC_NFC_SET_NPT_MODE		_IOW(SEC_NFC_MAGIC, 4, unsigned int)
#ifdef CONFIG_ESE_COLDRESET
#define SEC_NFC_COLD_RESET              _IOW(SEC_NFC_MAGIC, 5, unsigned int)
#endif

#define SEC_NFC_DEBUG			_IO(SEC_NFC_MAGIC, 99)

#ifndef CONFIG_SEC_NFC_LOGGER
#define NFC_LOG_ERR(fmt, ...)		pr_err("sec_nfc: "fmt, ##__VA_ARGS__)
#define NFC_LOG_INFO(fmt, ...)		pr_info("sec_nfc: "fmt, ##__VA_ARGS__)
#define NFC_LOG_INFO_WITH_DATE(fmt, ...) pr_info("sec_nfc: "fmt, ##__VA_ARGS__)
#define NFC_LOG_DBG(fmt, ...)		pr_debug("sec_nfc: "fmt, ##__VA_ARGS__)
#define NFC_LOG_REC(fmt, ...)		do { } while (0)

#define nfc_print_hex_dump(a, b, c)	do { } while (0)
#define nfc_logger_init()		do { } while (0)
#define nfc_logger_set_max_count(a)	do { } while (0)
#define nfc_logger_register_nfc_stauts_func(a)	do { } while (0)
#endif

/* size */
#define SEC_NFC_MSG_MAX_SIZE	(256 + 4)

/* wait for device stable */
#ifdef CONFIG_SEC_NFC_MARGINTIME
#define SEC_NFC_VEN_WAIT_TIME	(150)
#else
#define SEC_NFC_VEN_WAIT_TIME	(20)
#endif

/* gpio pin configuration */
struct sec_nfc_platform_data {
	int irq;
	int clk_irq;
	int ven;
	int firm_and_wake;
	int pvdd;
	unsigned int tvdd;
	unsigned int avdd;
	bool clk_req_wake;
	bool irq_all_trigger;
	bool eint_mode;
#ifdef CONFIG_ESE_COLDRESET
  unsigned int coldreset;
#endif

	unsigned int clk_req;
	struct   clk *clk;

	void (*cfg_gpio)(void);
	u32 ven_gpio_flags;
	u32 firm_gpio_flags;
	u32 irq_gpio_flags;
/*[START] NPT*/
	unsigned int npt;
	u32 npt_gpio_flags;
/*[END] NPT*/
	struct regulator *nfc_pvdd;
	const char *nfc_ic_type; /* used for nfc test */
	int i2c_switch; /*i2c swicth on_off gpio*/
};

enum sec_nfc_mode {
	SEC_NFC_MODE_OFF = 0,
	SEC_NFC_MODE_FIRMWARE,
	SEC_NFC_MODE_BOOTLOADER,
	SEC_NFC_MODE_COUNT,
	SEC_NFC_MODE_TURNING_ON_OFF,
};

enum sec_nfc_power {
	SEC_NFC_PW_ON = 0,
	SEC_NFC_PW_OFF,
};

enum sec_nfc_firmpin {
	SEC_NFC_FW_OFF = 0,
	SEC_NFC_FW_ON,
};

enum sec_nfc_wake {
	SEC_NFC_WAKE_SLEEP = 0,
	SEC_NFC_WAKE_UP,
};

/*[START] NPT*/
enum sec_nfc_npt_mode {
	SEC_NFC_NPT_OFF = 0,
	SEC_NFC_NPT_ON,
	SEC_NFC_NPT_CMD_ON = 0x7E,
	SEC_NFC_NPT_CMD_OFF,
};
/*[END] NPT*/

#ifdef CONFIG_ESE_COLDRESET
/*[START] COLDRESET*/
enum sec_nfc_coldreset{
	SEC_NFC_COLDRESET_OFF = 0,
	SEC_NFC_COLDRESET_ON,
};

#define FIRMWARE_GUARD_TIME (4)
#define DEVICEHOST_ID (0x00)
#define ESE_ID (0x02)
#define IDX_SLEEP_WAKEUP_NFC 0
#define IDX_SLEEP_WAKEUP_ESE 1
/*[END] COLDRESET*/
#endif

#define NFC_I2C_LDO_ON  1
#define NFC_I2C_LDO_OFF 0

enum lpm_status {
	LPM_NO_SUPPORT = -1,
	LPM_FALSE,
	LPM_TRUE
};
