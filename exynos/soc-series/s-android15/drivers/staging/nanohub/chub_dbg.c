// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * CHUB IF Driver Debug
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 * Authors:
 *      Boojin Kim <boojin.kim@samsung.com>
 *      Sukwon Ryu <sw.ryoo@samsung.com>
 *
 */

#include <soc/samsung/cal-if.h>
#include <linux/smc.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/iommu.h>
#include <linux/of_reserved_mem.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <soc/samsung/exynos/debug-snapshot.h>
#include "chub_log.h"
#include "chub_dbg.h"
#include "ipc_chub.h"
#include "chub.h"
#include "chub_exynos.h"
#ifdef CONFIG_SENSOR_DRV
#include "main.h"
#endif
#if IS_ENABLED(CONFIG_SHUB)
#include "chub_shub.h"
#include <soc/samsung/exynos/nanohub.h>
#endif

#define GPR_PC_INDEX (NUM_OF_GPR - 1)
#define AREA_NAME_MAX (8)
/* it's align ramdump side to prevent override */
#define SRAM_ALIGN (1024 * 8)
#define S_IRWUG (0660)

struct map_info {
	char name[AREA_NAME_MAX];
	u32 offset;
	u32 size;
};

struct dbg_dump {
	union {
		struct {
			struct map_info info[DBG_AREA_MAX];
			uint64_t time;
			int reason;
			struct ipc_area ipc_addr[IPC_REG_MAX];
			u32 gpr[NUM_OF_GPR];
			u32 cmu[CMU_REG_MAX];
			u32 sys[SYS_REG_MAX];
			u32 wdt[WDT_REG_MAX];
			u32 timer[TIMER_REG_MAX];
			u32 pwm[PWM_REG_MAX];
			u32 rtc[RTC_REG_MAX];
			u32 usi[USI_REG_MAX * MAX_USI_CNT];
		};
		char dummy[SRAM_ALIGN];
	};
	char sram[];
};

struct RamPersistedDataAndDropbox {
	u32 magic;
	u32 r[16];
	u32 sr_hfsr_cfsr_lo;
	u32 bits;
	u32 tid;
};

typedef struct
{
	uint32_t CPUID;    /*!< Offset: 0x000 (R/ )  CPUID Base Register */
	uint32_t ICSR;     /*!< Offset: 0x004 (R/W)  Interrupt Control and State Register*/
	uint32_t VTOR;     /*!< Offset: 0x008 (R/W)  Vector Table Offset Register        */
	uint32_t AIRCR;    /*!< Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register */
	uint32_t SCR;      /*!< Offset: 0x010 (R/W)  System Control Register             */
	uint32_t CCR;      /*!< Offset: 0x014 (R/W)  Configuration Control Register      */
	uint8_t  SHP[12];  /*!< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
	uint32_t SHCSR;    /*!< Offset: 0x024 (R/W)  System Handler Control and State Register */
	uint32_t CFSR;     /*!< Offset: 0x028 (R/W)  Configurable Fault Status Register  */
	uint32_t HFSR;     /*!< Offset: 0x02C (R/W)  HardFault Status Register           */
	uint32_t DFSR;     /*!< Offset: 0x030 (R/W)  Debug Fault Status Register         */
	uint32_t MMFAR;    /*!< Offset: 0x034 (R/W)  MemManage Fault Address Register    */
	uint32_t BFAR;     /*!< Offset: 0x038 (R/W)  BusFault Address Register           */
	uint32_t AFSR;     /*!< Offset: 0x03C (R/W)  Auxiliary Fault Status Register     */
	uint32_t PFR[2];   /*!< Offset: 0x040 (R/ )  Processor Feature Register          */
	uint32_t DFR;      /*!< Offset: 0x048 (R/ )  Debug Feature Register              */
	uint32_t ADR;      /*!< Offset: 0x04C (R/ )  Auxiliary Feature Register          */
	uint32_t MMFR[4];  /*!< Offset: 0x050 (R/ )  Memory Model Feature Register       */
	uint32_t ISAR[5];  /*!< Offset: 0x060 (R/ )  Instruction Set Attributes Register */
	uint32_t RESERVED0[5];
	uint32_t CPACR;    /*!< Offset: 0x088 (R/W)  Coprocessor Access Control Register */
} SCB_Type;

struct hardFaultDebugInfo {
	uint32_t msp;
	uint32_t psp;
	uint32_t reserved0[2];
	uint32_t gpr[16];
	uint32_t NVIC_ISER;
	uint32_t NVIC_ICER;
	uint32_t NVIC_ISPR;
	uint32_t NVIC_ICPR;
	uint32_t NVIC_IABR;
	uint32_t NVIC_IP;
	uint32_t NVIC_STIR;
	uint32_t reserved1;
	SCB_Type scb;
	uint32_t reserved2;
};

static struct dbg_dump *p_dbg_dump;
static struct reserved_mem *chub_rmem;
struct hardFaultDebugInfo hardFaultInfo;

static void *get_contexthub_info_from_dev(struct device *dev) {
#ifdef CONFIG_SENSOR_DRV
	struct nanohub_data *data = dev_get_nanohub_data(dev);

	return data->pdata->mailbox_client;
#else
	return dev_get_drvdata(dev);
#endif
}

static void chub_dbg_dump_gpr(struct contexthub_ipc_info *chub)
{
	if (p_dbg_dump) {
		int i;
		struct dbg_dump *p_dump = p_dbg_dump;

		IPC_HW_WRITE_DUMPGPR_CTRL(chub->iomem.chub_dumpgpr, 0x1);
		/* dump GPR */
		for (i = 0; i <= GPR_PC_INDEX - 1; i++)
			p_dump->gpr[i] =
			    readl(chub->iomem.chub_dumpgpr + REG_CHUB_DUMPGPR_GP0R +
				  i * 4);
		p_dump->gpr[GPR_PC_INDEX] =
		    readl(chub->iomem.chub_dumpgpr + REG_CHUB_DUMPGPR_PCR);

		nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x, r3:0x%x, r4:0x%x, r5:0x%x, r6:0x%x, r7:0x%x\n",
				 __func__, p_dump->gpr[0], p_dump->gpr[1], p_dump->gpr[2],
				 p_dump->gpr[3], p_dump->gpr[4], p_dump->gpr[5], p_dump->gpr[6],
				 p_dump->gpr[7]);

		nanohub_dev_info(chub->dev, "%s: r8:0x%x, r9:0x%x, r10:0x%x, r11:0x%x, r12:0x%x, sp:0x%x, lr:0x%x, pc:0x%x\n",
				 __func__, p_dump->gpr[8], p_dump->gpr[9], p_dump->gpr[10],
				 p_dump->gpr[11], p_dump->gpr[12], p_dump->gpr[13], p_dump->gpr[14],
				 p_dump->gpr[GPR_PC_INDEX]);

#if defined(NANOHUB_CPU_CM55)
		nanohub_dev_info(chub->dev, "%s: msp_sec:0x%x, psp_sec:0x%x, msp_nosec:0x%x, psp_nosec:0x%x, msplim_sec:0x%x,"
				"psplim_sec:0x%x, msplim_nosec:0x%x, psplim_nosec:0x%x, splim_ex:0x%x\n",
				__func__, p_dump->gpr[15], p_dump->gpr[16], p_dump->gpr[17], p_dump->gpr[18],
				p_dump->gpr[19], p_dump->gpr[20], p_dump->gpr[21], p_dump->gpr[22], p_dump->gpr[23]);
#endif



	}
}

static void chub_dbg_dump_cmu(struct contexthub_ipc_info *chub)
{
	if (p_dbg_dump && !IS_ERR_OR_NULL(chub->iomem.chub_dump_cmu)) {
		int i;
		struct dbg_dump *p_dump = p_dbg_dump;

		/* dump CMU */
		for (i = 0; i <= CMU_REG_MAX - 1; i++)
			p_dump->cmu[i] =
			    readl(chub->iomem.chub_dump_cmu +
			    dump_chub_cmu_registers[i]);

		nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x, r3:0x%x, r4:0x%x, r5:0x%x, r6:0x%x, r7:0x%x\n",
				 __func__, p_dump->cmu[0], p_dump->cmu[1], p_dump->cmu[2],
				 p_dump->cmu[3], p_dump->cmu[4], p_dump->cmu[5],
				 p_dump->cmu[6], p_dump->cmu[7]);

		nanohub_dev_info(chub->dev, "%s: r8:0x%x, r9:0x%x, r10:0x%x, r11:0x%x, r12:0x%x, r13:0x%x, r14:0x%x, r15:0x%x\n",
				 __func__, p_dump->cmu[8], p_dump->cmu[9], p_dump->cmu[10],
				 p_dump->cmu[11], p_dump->cmu[12], p_dump->cmu[13],
				 p_dump->cmu[14], p_dump->cmu[15]);

		nanohub_dev_info(chub->dev, "%s: r16:0x%x\n", __func__, p_dump->cmu[16]);
	}
}

static void chub_dbg_dump_sys(struct contexthub_ipc_info *chub)
{
	if (p_dbg_dump && !IS_ERR_OR_NULL(chub->iomem.chub_dump_sys)) {
		int i;
		struct dbg_dump *p_dump = p_dbg_dump;

		/* dump SYS */
		for (i = 0; i <= SYS_REG_MAX - 1; i++)
			p_dump->sys[i] =
			    readl(chub->iomem.chub_dump_sys +
			    dump_chub_sys_registers[i]);

		nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x, r3:0x%x, r4:0x%x, r5:0x%x, r6:0x%x, r7:0x%x\n",
				 __func__, p_dump->sys[0], p_dump->sys[1], p_dump->sys[2],
				 p_dump->sys[3], p_dump->sys[4], p_dump->sys[5],
				 p_dump->sys[6], p_dump->sys[7]);

		nanohub_dev_info(chub->dev, "%s: r8:0x%x, r9:0x%x, r10:0x%x, r11:0x%x, r12:0x%x, r13:0x%x, r14:0x%x, r15:0x%x\n",
				 __func__, p_dump->sys[8], p_dump->sys[9],
				 p_dump->sys[10], p_dump->sys[11], p_dump->sys[12],
				 p_dump->sys[13], p_dump->sys[14], p_dump->sys[15]);

		nanohub_dev_info(chub->dev, "%s: r16:0x%x, r17:0x%x, r18:0x%x, r19:0x%x, r20:0x%x\n",
				 __func__, p_dump->sys[16], p_dump->sys[17], p_dump->sys[18],
				 p_dump->sys[19], p_dump->sys[20]);

	}
}

static void chub_dbg_dump_wdt(struct contexthub_ipc_info *chub)
{
	if (p_dbg_dump && !IS_ERR_OR_NULL(chub->iomem.chub_dump_wdt)) {
		int i;
		struct dbg_dump *p_dump = p_dbg_dump;

		/* dump wdt */
		for (i = 0; i <= WDT_REG_MAX - 1; i++)
			p_dump->wdt[i] =
			    readl(chub->iomem.chub_dump_wdt + dump_chub_wdt_registers[i]);

		nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x\n",
				 __func__, p_dump->wdt[0], p_dump->wdt[1]);
	}
}

static void chub_dbg_dump_timer(struct contexthub_ipc_info *chub)
{
	if (p_dbg_dump && !IS_ERR_OR_NULL(chub->iomem.chub_dump_wdt)) {
		int i;
		struct dbg_dump *p_dump = p_dbg_dump;

		/* dump timer */
		for (i = 0; i <= TIMER_REG_MAX - 1; i++)
			p_dump->timer[i] =
			    readl(chub->iomem.chub_dump_timer +
			    dump_chub_timer_registers[i]);

		nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x\n",
				 __func__, p_dump->timer[0], p_dump->timer[1], p_dump->timer[2]);
	}
}

static void chub_dbg_dump_pwm(struct contexthub_ipc_info *chub)
{
	if (p_dbg_dump && !IS_ERR_OR_NULL(chub->iomem.chub_dump_pwm)) {
		int i;
		struct dbg_dump *p_dump = p_dbg_dump;

		/* dump pwm */
		for (i = 0; i <= PWM_REG_MAX - 1; i++)
			p_dump->pwm[i] =
				readl(chub->iomem.chub_dump_pwm + dump_chub_pwm_registers[i]);

		nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x, r3:0x%x, r4:0x%x, r5:0x%x, r6:0x%x, r7:0x%x\n",
				 __func__, p_dump->pwm[0],
				 p_dump->pwm[1], p_dump->pwm[2],
				 p_dump->pwm[3], p_dump->pwm[4],
				 p_dump->pwm[5], p_dump->pwm[6], p_dump->pwm[7]);

		nanohub_dev_info(chub->dev, "%s: r8:0x%x, r9:0x%x, r10:0x%x, r11:0x%x, r12:0x%x, r13:0x%x, r14:0x%x, r15:0x%x\n",
				 __func__, p_dump->pwm[8],
				 p_dump->pwm[9], p_dump->pwm[10],
				 p_dump->pwm[11], p_dump->pwm[12],
				 p_dump->pwm[13], p_dump->pwm[14], p_dump->pwm[15]);

	}
}

static void chub_dbg_dump_rtc(struct contexthub_ipc_info *chub)
{
	if (p_dbg_dump && !IS_ERR_OR_NULL(chub->iomem.chub_dump_rtc)) {
		int i;
		struct dbg_dump *p_dump = p_dbg_dump;

		/* dump rtc */
		for (i = 0; i <= RTC_REG_MAX - 1; i++)
			p_dump->rtc[i] =
				readl(chub->iomem.chub_dump_rtc +
				dump_chub_rtc_registers[i]);

		nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x, r3:0x%x, r4:0x%x, r5:0x%x, r6:0x%x, r7:0x%x\n",
				 __func__, p_dump->rtc[0], p_dump->rtc[1], p_dump->rtc[2],
				 p_dump->rtc[3], p_dump->rtc[4], p_dump->rtc[5], p_dump->rtc[6],
				 p_dump->rtc[7]);

		nanohub_dev_info(chub->dev, "%s: r8:0x%x, r9:0x%x, r10:0x%x, r11:0x%x, r12:0x%x, r13:0x%x, r14:0x%x, r15:0x%x\n",
				 __func__, p_dump->rtc[8],
				 p_dump->rtc[9], p_dump->rtc[10],
				 p_dump->rtc[11], p_dump->rtc[12],
				 p_dump->rtc[13], p_dump->rtc[14],
				 p_dump->rtc[15]);

		nanohub_dev_info(chub->dev, "%s: r16:0x%x, r17:0x%x, r18:0x%x, r19:0x%x, r20:0x%x, r21:0x%x, r22:0x%x\n",
				 __func__, p_dump->rtc[16],
				 p_dump->rtc[17], p_dump->rtc[18],
				 p_dump->rtc[19], p_dump->rtc[20],
				 p_dump->rtc[21], p_dump->rtc[22]);
	}
}

static void chub_dbg_dump_usi(struct contexthub_ipc_info *chub)
{
	if (p_dbg_dump && chub->iomem.usi_cnt) {
		int i;
		int j;
		int index = 0;
		int index_tmp = 0;
		u32 usi_protocol;
		struct dbg_dump *p_dump = p_dbg_dump;

		for (j = 0; j < chub->iomem.usi_cnt; j++) {
			/* dump usi */
			usi_protocol =
			    READ_CHUB_USI_CONF(chub->iomem.usi_array[j]);
			switch (usi_protocol) {
			case USI_PROTOCOL_UART:
				nanohub_dev_info(chub->dev, "%s: chub_usi %d config as UART\n",
						 __func__, j);
				index_tmp = index;
				for (i = 0; i <= UART_REG_MAX - 1; i++) {
					p_dump->usi[index++] =
					   readl(chub->iomem.usi_array[j]
						 + dump_chub_uart_registers[i]);
				}

				nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x, r3:0x%x, r4:0x%x, r5:0x%x, r6:0x%x, r7:0x%x\n",
						 __func__,
						 p_dump->usi[index_tmp],
						 p_dump->usi[index_tmp + 1],
						 p_dump->usi[index_tmp + 2],
						 p_dump->usi[index_tmp + 3],
						 p_dump->usi[index_tmp + 4],
						 p_dump->usi[index_tmp + 5],
						 p_dump->usi[index_tmp + 6],
						 p_dump->usi[index_tmp + 7]);

				nanohub_dev_info(chub->dev, "%s: r8:0x%x, r9:0x%x, r10:0x%x, r11:0x%x, r12:0x%x, r13:0x%x, r14:0x%x, r15:0x%x\n",
						 __func__,
						 p_dump->usi[index_tmp + 8],
						 p_dump->usi[index_tmp + 9],
						 p_dump->usi[index_tmp + 10],
						 p_dump->usi[index_tmp + 11],
						 p_dump->usi[index_tmp + 12],
						 p_dump->usi[index_tmp + 13],
						 p_dump->usi[index_tmp + 14],
						 p_dump->usi[index_tmp + 15]);
				break;
			case USI_PROTOCOL_SPI:
				nanohub_dev_info(chub->dev, "%s: chub_usi %d config as SPI\n",
						 __func__, j);
				index_tmp = index;
				for (i = 0; i <= SPI_REG_MAX - 1; i++) {
					p_dump->usi[index++] =
					    readl(chub->iomem.usi_array[j]
						  + dump_chub_spi_registers[i]);
				}

				nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x, r3:0x%x, r4:0x%x, r5:0x%x, r6:0x%x, r7:0x%x\n",
						 __func__,
						 p_dump->usi[index_tmp],
						 p_dump->usi[index_tmp + 1],
						 p_dump->usi[index_tmp + 2],
						 p_dump->usi[index_tmp + 3],
						 p_dump->usi[index_tmp + 4],
						 p_dump->usi[index_tmp + 5],
						 p_dump->usi[index_tmp + 6],
						 p_dump->usi[index_tmp + 7]);

				nanohub_dev_info(chub->dev, "%s: r8:0x%x, r9:0x%x, r10:0x%x\n",
						 __func__,
						 p_dump->usi[index_tmp + 8],
						 p_dump->usi[index_tmp + 9],
						 p_dump->usi[index_tmp + 10]);
				break;
			case USI_PROTOCOL_I2C:
				nanohub_dev_info(chub->dev,
						 "%s: chub_usi %d config as I2C\n", __func__, j);
				index_tmp = index;
				for (i = 0; i <= I2C_REG_MAX - 1; i++)
					p_dump->usi[index++] =
					   readl(chub->iomem.usi_array[j] + dump_chub_i2c_registers[i]);

				nanohub_dev_info(chub->dev, "%s: r0:0x%x, r1:0x%x, r2:0x%x, r3:0x%x, r4:0x%x, r5:0x%x, r6:0x%x, r7:0x%x\n",
						 __func__,
						 p_dump->usi[index_tmp],
						 p_dump->usi[index_tmp + 1],
						 p_dump->usi[index_tmp + 2],
						 p_dump->usi[index_tmp + 3],
						 p_dump->usi[index_tmp + 4],
						 p_dump->usi[index_tmp + 5],
						 p_dump->usi[index_tmp + 6],
						 p_dump->usi[index_tmp + 7]);

				nanohub_dev_info(chub->dev, "%s: r8:0x%x, r9:0x%x, r10:0x%x, r11:0x%x, r12:0x%x, r13:0x%x, r14:0x%x, r15:0x%x\n",
						 __func__,
						 p_dump->usi[index_tmp + 8],
						 p_dump->usi[index_tmp + 9],
						 p_dump->usi[index_tmp + 10],
						 p_dump->usi[index_tmp + 11],
						 p_dump->usi[index_tmp + 12],
						 p_dump->usi[index_tmp + 13],
						 p_dump->usi[index_tmp + 14],
						 p_dump->usi[index_tmp + 15]);

				nanohub_dev_info(chub->dev, "%s: r16:0x%x, r17:0x%x, r18:0x%x, r19:0x%x\n",
						 __func__,
						 p_dump->usi[index_tmp + 16],
						 p_dump->usi[index_tmp + 17],
						 p_dump->usi[index_tmp + 18],
						 p_dump->usi[index_tmp + 19]);
				break;
			default:
				break;
			}
		}
	}
}

static void chub_dbg_dump_barac(struct contexthub_ipc_info *chub)
{
	int i, ret;
	unsigned int log_st;

	/* tzpc setting, if defined in dt */
	if (of_get_property(chub->dev->of_node, "smc-required", NULL)) {
		ret = exynos_smc(SMC_CMD_CONN_IF,
				 (EXYNOS_CHUB << 32) | EXYNOS_SET_CONN_TZPC,
				 0, 0);
		if (ret) {
			nanohub_err("%s: TZPC setting fail by ret:%d\n", __func__, ret);
			return;
		}
	}

	for (i = 0 ; i < BARAC_MAX; i++) {
		if (IS_ERR_OR_NULL(chub->iomem.chub_baracs[i].addr))
			break;

		log_st = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_LOG_ST_OFFSET);
		if (log_st & BA_LOG_ST_AW_CONFLIT) {
			nanohub_dev_err(chub->dev, "%s: %s: AW window conflict \n", __func__,
					chub->iomem.chub_baracs[i].name);
		}
		if (log_st & BA_LOG_ST_AW_CART) {
			unsigned int awlog0, awlog1, awlog2, awlog3;
			unsigned long int awaddr;

			awlog0 = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_AWLOG0_OFFSET);
			awlog1 = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_AWLOG1_OFFSET);
			awlog2 = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_AWLOG2_OFFSET);
			awlog3 = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_AWLOG3_OFFSET);

			awaddr = ((unsigned long int)(awlog1 & BA_ARLOG1_ARLOG_ARADDR39_32) << 32) | (unsigned long int) awlog0;

			nanohub_dev_err(chub->dev, "%s: %s: AW window err\n", __func__,
					chub->iomem.chub_baracs[i].name);
			nanohub_dev_err(chub->dev, "AWADDR : 0x%010X\n", awaddr);
			nanohub_dev_err(chub->dev, "AWQOS  : 0x%X\n", (awlog1 & BA_AWLOG1_AWLOG_AWQOS) >> BA_AWLOG1_AWLOG_AWQOS_BIT);
			nanohub_dev_err(chub->dev, "AWSIZE : 0x%X\n", (awlog1 & BA_AWLOG1_AWLOG_AWSIZE) >> BA_AWLOG1_AWLOG_AWSIZE_BIT);
			nanohub_dev_err(chub->dev, "AWPROT : 0x%X\n", (awlog1 & BA_AWLOG1_AWLOG_AWPROT) >> BA_AWLOG1_AWLOG_AWPROT_BIT);
			nanohub_dev_err(chub->dev, "AWLOCK : 0x%X\n", (awlog1 & BA_AWLOG1_AWLOG_AWLOCK) >> BA_AWLOG1_AWLOG_AWLOCK_BIT);
			nanohub_dev_err(chub->dev, "AWCACHE: 0x%X\n", (awlog1 & BA_AWLOG1_AWLOG_AWCACHE) >> BA_AWLOG1_AWLOG_AWCACHE_BIT);
			nanohub_dev_err(chub->dev, "AWBURST: 0x%X\n", (awlog1 & BA_AWLOG1_AWLOG_AWBURST) >> BA_AWLOG1_AWLOG_AWBURST_BIT);
			nanohub_dev_err(chub->dev, "AWLEN  : 0x%X\n", (awlog1 & BA_AWLOG1_AWLOG_AWLEN) >> BA_AWLOG1_AWLOG_AWLEN_BIT);

			__raw_writel(BA_LOG_CLR_AW_CAPT_CLEAR, chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_LOG_CLR_OFFSET);
		}
		if (log_st & BA_LOG_ST_AR_CONFLIT) {
			nanohub_dev_err(chub->dev, "%s: %s: AR window conflict\n", __func__,
					chub->iomem.chub_baracs[i].name);
		}
		if (log_st & BA_LOG_ST_AR_CART) {
			unsigned int arlog0, arlog1, arlog2, arlog3;
			unsigned long int araddr;

			arlog0 = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_ARLOG0_OFFSET);
			arlog1 = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_ARLOG1_OFFSET);
			arlog2 = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_ARLOG2_OFFSET);
			arlog3 = __raw_readl(chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_ARLOG3_OFFSET);

			araddr = ((unsigned long int)(arlog1 & BA_ARLOG1_ARLOG_ARADDR39_32) << 32) | (unsigned long int) arlog0;

			nanohub_dev_err(chub->dev, "%s: %s: AR window err\n", __func__,
					chub->iomem.chub_baracs[i].name);
			nanohub_dev_err(chub->dev, "ARADDR : 0x%010X\n", araddr);
			nanohub_dev_err(chub->dev, "ARQOS  : 0x%X\n", (arlog1 & BA_ARLOG1_ARLOG_ARQOS) >> BA_ARLOG1_ARLOG_ARQOS_BIT);
			nanohub_dev_err(chub->dev, "ARSIZE : 0x%X\n", (arlog1 & BA_ARLOG1_ARLOG_ARSIZE) >> BA_ARLOG1_ARLOG_ARSIZE_BIT);
			nanohub_dev_err(chub->dev, "ARPROT : 0x%X\n", (arlog1 & BA_ARLOG1_ARLOG_ARPROT) >> BA_ARLOG1_ARLOG_ARPROT_BIT);
			nanohub_dev_err(chub->dev, "ARLOCK : 0x%X\n", (arlog1 & BA_ARLOG1_ARLOG_ARLOCK) >> BA_ARLOG1_ARLOG_ARLOCK_BIT);
			nanohub_dev_err(chub->dev, "ARCACHE: 0x%X\n", (arlog1 & BA_ARLOG1_ARLOG_ARCACHE) >> BA_ARLOG1_ARLOG_ARCACHE_BIT);
			nanohub_dev_err(chub->dev, "ARBURST: 0x%X\n", (arlog1 & BA_ARLOG1_ARLOG_ARBURST) >> BA_ARLOG1_ARLOG_ARBURST_BIT);
			nanohub_dev_err(chub->dev, "ARLEN  : 0x%X\n", (arlog1 & BA_ARLOG1_ARLOG_ARLEN) >> BA_ARLOG1_ARLOG_ARLEN_BIT);

			__raw_writel(BA_LOG_CLR_AR_CAPT_CLEAR, chub->iomem.chub_baracs[i].addr + REG_BARAC_BA_LOG_CLR_OFFSET);
		}
	}
}

static u32 get_dbg_dump_size(void)
{
	return sizeof(struct dbg_dump) + ipc_get_chub_mem_size();
};

static u32 get_chub_dumped_registers(int cnt)
{
	return sizeof(u32) * (NUM_OF_GPR + CMU_REG_MAX + SYS_REG_MAX + WDT_REG_MAX +
		 TIMER_REG_MAX + PWM_REG_MAX + RTC_REG_MAX + USI_REG_MAX * cnt);
};

// return 0 if copying hardFaultInfo success.
// return -1 if failed.(p_dbg_dump is null)
int chub_dbg_get_hardFaultInfo(struct contexthub_ipc_info *chub) {
	struct chub_bootargs *map = ipc_get_base(IPC_REG_BL_MAP);
	u32 debug_info_offset = map->debug_info_offset;

	if (p_dbg_dump) {
		memset(&hardFaultInfo, 0, sizeof(struct hardFaultDebugInfo));
		memcpy(&hardFaultInfo, p_dbg_dump->sram + debug_info_offset, sizeof(struct hardFaultDebugInfo));
		memset_io(chub->iomem.sram + debug_info_offset, 0, sizeof(struct hardFaultDebugInfo));
		return 0;
	}

	return -1;
}

/* dump hw into dram (chub reserved mem) */
void chub_dbg_dump_ram(struct contexthub_ipc_info *chub, enum chub_err_type reason)
{
	if (p_dbg_dump) {
		p_dbg_dump->time = sched_clock();
		p_dbg_dump->reason = reason;

		/* dump SRAM to reserved DRAM */
		memcpy_fromio(p_dbg_dump->sram,
			      ipc_get_base(IPC_REG_DUMP), ipc_get_chub_mem_size());
		if (reason == CHUB_ERR_KERNEL_PANIC)
			chub_dbg_dump_gpr(chub);

		if (chub->misc.hardfault_debug)
			chub_dbg_get_hardFaultInfo(chub);
	}
}

static void chub_dbg_dump_status(struct contexthub_ipc_info *chub)
{
	int i;

#ifdef CONFIG_SENSOR_DRV
	struct nanohub_data *data = chub->data;

	nanohub_dev_info(chub->dev,
			 "%s: nanohub driver status\nwu:%d wu_l:%d acq:%d irq_apInt:%d fired:%d\n",
			 __func__, atomic_read(&data->wakeup_cnt),
			 atomic_read(&data->wakeup_lock_cnt),
			 atomic_read(&data->wakeup_acquired),
			 atomic_read(&chub->atomic.irq_apInt), nanohub_irq_fired(data));
	print_chub_user(data);
#endif

	nanohub_dev_info(chub->dev, "%s: status:%d, reset_cnt:%d, wakeup_by_chub_cnt:%d\n",
			 __func__, atomic_read(&chub->atomic.chub_status),
			 chub->misc.err_cnt[CHUB_ERR_RESET_CNT], chub->misc.wakeup_by_chub_cnt);
	/* print error status */
	for (i = 0; i < CHUB_ERR_MAX; i++) {
		if (chub->misc.err_cnt[i])
			nanohub_dev_info(chub->dev, "%s: err(%d) : err_cnt:%d times\n",
					 __func__, i, chub->misc.err_cnt[i]);
	}
#ifdef USE_FW_DUMP
	contexthub_ipc_write_event(chub, MAILBOX_EVT_DUMP_STATUS);
#endif
	ipc_dump();
}

void chub_dbg_print_funcname(struct contexthub_ipc_info *chub, u32 lr)
{
	u32 index;
	u32 offset;
	u32 i;
	char *funcname_table;
	char funcname[50];

/*
	nanohub_dev_info(ipc->dev, "%s - symbol_table:%llx\n", __func__, ipc->symbol_table);
	nanohub_dev_info(ipc->dev, "%s - symbol_table->size:%x\n", __func__, ipc->symbol_table->size);
	nanohub_dev_info(ipc->dev, "%s - symbol_table->count:%x\n", __func__, ipc->symbol_table->count);
	nanohub_dev_info(ipc->dev, "%s - symbol_table->name_offset:%x\n", __func__, ipc->symbol_table->name_offset);
	nanohub_dev_info(ipc->dev, "%s - symbol_table->symbol:%llx\n", __func__, ipc->symbol_table->symbol);

	print_hex_dump(KERN_CONT, "nanohub :",
                     DUMP_PREFIX_OFFSET, 16, 1, ipc->symbol_table, 32, false);
*/

	funcname_table = (char *)(chub->symbol_table) + chub->symbol_table->size;

	for (index = 0 ; index < chub->symbol_table->count ; index++) {
		if ((chub->symbol_table->symbol[index].base <= lr) &&
		    ((lr - chub->symbol_table->symbol[index].base) <
		    chub->symbol_table->symbol[index].size))
			break;
	}

	if (index >= chub->symbol_table->count)
		return;

	offset = lr - chub->symbol_table->symbol[index].base;

	for (i = 0 ; i < chub->symbol_table->symbol[index].length ; i++)
		funcname[i] = funcname_table[chub->symbol_table->symbol[index].offset + i];
	funcname[i] = '\0';

	nanohub_dev_info(chub->dev, "[ %08x ] %s + 0x%x\n", lr, funcname, offset);
}

void chub_dbg_call_trace(struct contexthub_ipc_info *chub)
{
	struct RamPersistedDataAndDropbox *dbx;
	void __iomem *sp, *stack_top;
	u32 pc, lr;
	u32 code_start, code_end;
	u32 count = 0;
	u16 opcode1, opcode2;

	nanohub_dev_info(chub->dev, "%s - Dump CHUB call stack\n", __func__);
	if (!chub->symbol_table) {
		nanohub_dev_info(chub->dev, "%s - there is no symbol_table\n", __func__);
		return;
	}

	dbx = (struct RamPersistedDataAndDropbox *)ipc_get_base(IPC_REG_PERSISTBUF);

	if (dbx->magic == 0x31416200) {
		sp = ipc_get_base(IPC_REG_BL) + dbx->r[13];
		pc = dbx->r[15];
		lr = dbx->r[14];
		nanohub_dev_info(chub->dev,
				 "%s : Get PC/LR from Dropbox : %llx %llx\n", __func__, pc, lr);
	} else {
		sp = ipc_get_base(IPC_REG_BL) + p_dbg_dump->gpr[13];
		pc = p_dbg_dump->gpr[GPR_PC_INDEX];
		lr = p_dbg_dump->gpr[14];
		nanohub_dev_info(chub->dev,
				 "%s : Get PC/LR from GPR : %llx %llx\n", __func__, pc, lr);
	}

	stack_top = ipc_get_base(IPC_REG_BL) + (u32)__raw_readl(ipc_get_base(IPC_REG_OS));

	if (sp >= stack_top)
		return;

	chub_dbg_print_funcname(chub, pc);

	code_start = (uintptr_t)ipc_get_base(IPC_REG_OS) - (uintptr_t)ipc_get_base(IPC_REG_BL);
	code_end = code_start + (u32)ipc_get_size(IPC_REG_OS);

	while (sp < stack_top && count < 4) {
		lr = (u32)__raw_readl(sp);

		sp += 4;

		if ((lr & 0x1) == 0) {
			continue;
		}

		lr = lr - 1;
		if (lr <= code_start + 0x100 || lr > code_end) {
			continue;
		}

		opcode1 = (u16)__raw_readw((ipc_get_base(IPC_REG_BL) + lr - 2));
		opcode2 = (u16)__raw_readw((ipc_get_base(IPC_REG_BL) + lr - 4));

		if ((opcode1 & 0xF800) == 0x4000) {
			lr = lr - 2;
		} else if ((opcode2 & 0xF000) == 0xF000) {
			lr = lr - 4;
		} else {
			continue;
		}

		chub_dbg_print_funcname(chub, lr);
		count++;
	}
	nanohub_dev_info(chub->dev, "%s : SP : %llx\n", __func__, sp);
}

void chub_dbg_dump_hw(struct contexthub_ipc_info *chub, enum chub_err_type reason)
{
	u64 ktime_now = ktime_get_boottime_ns();
	static u64 last_dump_time = 0;
#if IS_ENABLED(CONFIG_SHUB)
	struct contexthub_dump dump;
#endif

	nanohub_dev_info(chub->dev, "%s: reason:%d\n", __func__, reason);

	chub_dbg_dump_gpr(chub);

	if (!last_dump_time || ktime_now - last_dump_time > 60ULL*1000*1000*1000)
		chub_dbg_dump_ram(chub, reason);
	else
		nanohub_dev_info(chub->dev, "%s: repeated dump in 1 min, skip sram\n", __func__);
	last_dump_time = ktime_now;

	if (chub->misc.hardfault_debug) {
		nanohub_dev_info(chub->dev, "%s : Hardfault info\n", __func__);
		nanohub_dev_info(chub->dev, "  msp : 0x%X\n", hardFaultInfo.msp);
		nanohub_dev_info(chub->dev, "  psp : 0x%X\n", hardFaultInfo.psp);
		nanohub_dev_info(chub->dev, "  NVIC_ISER : 0x%X\n", hardFaultInfo.NVIC_ISER);
		nanohub_dev_info(chub->dev, "  NVIC_ICER : 0x%X\n", hardFaultInfo.NVIC_ICER);
		nanohub_dev_info(chub->dev, "  NVIC_ISPR : 0x%X\n", hardFaultInfo.NVIC_ISPR);
		nanohub_dev_info(chub->dev, "  NVIC_ICPR : 0x%X\n", hardFaultInfo.NVIC_ICPR);
		nanohub_dev_info(chub->dev, "  NVIC_IABR : 0x%X\n", hardFaultInfo.NVIC_IABR);
		nanohub_dev_info(chub->dev, "  NVIC_IP   : 0x%X\n", hardFaultInfo.NVIC_IP);
		nanohub_dev_info(chub->dev, "  NVIC_STIR : 0x%X\n", hardFaultInfo.NVIC_STIR);
		nanohub_dev_info(chub->dev, "  SHCSR : 0x%X\n", hardFaultInfo.scb.SHCSR);
		nanohub_dev_info(chub->dev, "  CFSR  : 0x%X\n", hardFaultInfo.scb.CFSR);
		nanohub_dev_info(chub->dev, "  HFSR  : 0x%X\n", hardFaultInfo.scb.HFSR);
		nanohub_dev_info(chub->dev, "  DFSR  : 0x%X\n", hardFaultInfo.scb.DFSR);
		nanohub_dev_info(chub->dev, "  MMFAR : 0x%X\n", hardFaultInfo.scb.MMFAR);
		nanohub_dev_info(chub->dev, "  BFAR  : 0x%X\n", hardFaultInfo.scb.BFAR);
		if (p_dbg_dump && ipc_hw_read_shared_reg(chub->iomem.mailbox, SR_3) == 0xFFFFFFFF) {
			nanohub_dev_info(chub->dev, "=== restored hardfault gpr ===\n");
			nanohub_dev_info(chub->dev, "r0:  0x%08x\n", hardFaultInfo.gpr[0]);
			nanohub_dev_info(chub->dev, "r1:  0x%08x\n", hardFaultInfo.gpr[1]);
			nanohub_dev_info(chub->dev, "r2:  0x%08x\n", hardFaultInfo.gpr[2]);
			nanohub_dev_info(chub->dev, "r3:  0x%08x\n", hardFaultInfo.gpr[3]);
			nanohub_dev_info(chub->dev, "r4:  0x%08x\n", hardFaultInfo.gpr[4]);
			nanohub_dev_info(chub->dev, "r5:  0x%08x\n", hardFaultInfo.gpr[5]);
			nanohub_dev_info(chub->dev, "r6:  0x%08x\n", hardFaultInfo.gpr[6]);
			nanohub_dev_info(chub->dev, "r7:  0x%08x\n", hardFaultInfo.gpr[7]);
			nanohub_dev_info(chub->dev, "r8:  0x%08x\n", hardFaultInfo.gpr[8]);
			nanohub_dev_info(chub->dev, "r9:  0x%08x\n", hardFaultInfo.gpr[9]);
			nanohub_dev_info(chub->dev, "r10: 0x%08x\n", hardFaultInfo.gpr[10]);
			nanohub_dev_info(chub->dev, "r11: 0x%08x\n", hardFaultInfo.gpr[11]);
			nanohub_dev_info(chub->dev, "r12: 0x%08x\n", hardFaultInfo.gpr[12]);
			nanohub_dev_info(chub->dev, "sp:  0x%08x\n", hardFaultInfo.gpr[13]);
			nanohub_dev_info(chub->dev, "lr:  0x%08x\n", hardFaultInfo.gpr[14]);
			nanohub_dev_info(chub->dev, "pc:  0x%08x\n", hardFaultInfo.gpr[15]);
		}
	}

	chub_dbg_call_trace(chub);
	chub_dbg_dump_cmu(chub);
	chub_dbg_dump_sys(chub);
	chub_dbg_dump_wdt(chub);
	chub_dbg_dump_timer(chub);
	chub_dbg_dump_pwm(chub);
	chub_dbg_dump_rtc(chub);
	chub_dbg_dump_usi(chub);
	if (!chub->iomem.is_baaw)
		chub_dbg_dump_barac(chub);

#ifdef CONFIG_SENSOR_DRV
	nanohub_dev_info(chub->dev, "%s: notice to dump chub registers\n", __func__);
	nanohub_add_dump_request(chub->data);
#endif
	if (atomic_read(&chub->atomic.force_wdt) == 2) {
		nanohub_dev_err(chub->dev, "%s: policy: CHUB reset --> AP watchdog reset\n", __func__);
		dbg_snapshot_expire_watchdog();
	} else 	if (atomic_read(&chub->atomic.force_wdt) == 1 &&
			ipc_hw_read_shared_reg(chub->iomem.mailbox, SR_3) == 0xFFFFFFFF &&
			(!atomic_read(&chub->atomic.force_wdt_cfsr) ||
			(hardFaultInfo.scb.CFSR & atomic_read(&chub->atomic.force_wdt_cfsr))))
	{
		nanohub_dev_err(chub->dev, "%s: policy: CHUB Hardfault exception --> AP watchdog reset\n", __func__);
		dbg_snapshot_expire_watchdog();
	}
#if IS_ENABLED(CONFIG_SHUB)
	dump.reason = reason;
	dump.dump = p_dbg_dump->sram;
	dump.size = ipc_get_chub_mem_size();

	if (chub->misc.hardfault_debug && p_dbg_dump) {
		struct timespec64 time;
		struct rtc_time tm;
		uint32_t debug_info_offset = 0;
		uint32_t debug_info[7] = {0, };
		char *p_debug_info;

		ktime_get_real_ts64(&time);
		rtc_time64_to_tm(time.tv_sec, &tm);

		if (0xD == (0xD & p_dbg_dump->gpr[14])) {
			debug_info_offset = hardFaultInfo.psp;
		} else {
			debug_info_offset = hardFaultInfo.msp;
		}

		p_debug_info = dump.dump + debug_info_offset;

		if (p_debug_info + sizeof(debug_info) <= dump.dump + dump.size)
			memcpy(&debug_info[0], p_debug_info, sizeof(debug_info));

		snprintf(dump.mini_dump, SHUB_MINI_DUMP_LENGTH,
				"%04d%02d%02d %02d:%02d:%02d reason:%d "
				"r0~r12:%x %x %x %x %x %x %x %x %x %x %x %x %x sp:%x lr:%x r15:%x pc:%x "
				"msp:%X psp:%X ISER:%X ICER:%X ISPR:%X ICPR:%X IABR:%X IP:%X STIR:%X "
				"SHCSR:%X CFSR:%X HFSR:%X DFSR:%X MMFAR:%X BFAR:%X "
				"gpr0~15:%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x "
				"dump0~6:%X %X %X %X %X %X %X",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, reason,
				p_dbg_dump->gpr[0], p_dbg_dump->gpr[1], p_dbg_dump->gpr[2], p_dbg_dump->gpr[3],
				p_dbg_dump->gpr[4], p_dbg_dump->gpr[5], p_dbg_dump->gpr[6], p_dbg_dump->gpr[7],
				p_dbg_dump->gpr[8], p_dbg_dump->gpr[9], p_dbg_dump->gpr[10], p_dbg_dump->gpr[11],
				p_dbg_dump->gpr[12], p_dbg_dump->gpr[13], p_dbg_dump->gpr[14], p_dbg_dump->gpr[15],
				p_dbg_dump->gpr[16],
				hardFaultInfo.msp, hardFaultInfo.psp, hardFaultInfo.NVIC_ISER,
				hardFaultInfo.NVIC_ICER, hardFaultInfo.NVIC_ISPR, hardFaultInfo.NVIC_ICPR,
				hardFaultInfo.NVIC_IABR, hardFaultInfo.NVIC_IP, hardFaultInfo.NVIC_STIR,
				hardFaultInfo.scb.SHCSR, hardFaultInfo.scb.CFSR, hardFaultInfo.scb.HFSR,
				hardFaultInfo.scb.DFSR, hardFaultInfo.scb.MMFAR, hardFaultInfo.scb.BFAR,
				hardFaultInfo.gpr[0], hardFaultInfo.gpr[1], hardFaultInfo.gpr[2], hardFaultInfo.gpr[3],
				hardFaultInfo.gpr[4], hardFaultInfo.gpr[5], hardFaultInfo.gpr[6], hardFaultInfo.gpr[7],
				hardFaultInfo.gpr[8], hardFaultInfo.gpr[9], hardFaultInfo.gpr[10], hardFaultInfo.gpr[11],
				hardFaultInfo.gpr[12], hardFaultInfo.gpr[13], hardFaultInfo.gpr[14], hardFaultInfo.gpr[15],
				debug_info[0], debug_info[1], debug_info[2], debug_info[3],
				debug_info[4], debug_info[5], debug_info[6]);
	}

	contexthub_dump_notifier_call(CHUB_ERR_DUMP, &dump);
#endif
}

static ssize_t chub_get_chub_register_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	char *pbuf = buf;
	int i;
	u32 usi_protocol;
	int j;
	int index = 0;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	if (p_dbg_dump) {
		chub_dbg_dump_cmu(chub);
		pbuf += sprintf(pbuf, "===================\n");
		pbuf += sprintf(pbuf, "CHUB CMU register\n");

		for (i = 0; i <= CMU_REG_MAX - 1; i++)
			pbuf +=
			    sprintf(pbuf, "R%02d : %08x\n", i,
				    p_dbg_dump->cmu[i]);

		chub_dbg_dump_sys(chub);
		pbuf += sprintf(pbuf, "===================\n");
		pbuf += sprintf(pbuf, "CHUB SYS register\n");

		for (i = 0; i <= SYS_REG_MAX - 1; i++)
			pbuf +=
			    sprintf(pbuf, "R%02d : %08x\n", i,
				    p_dbg_dump->sys[i]);

		chub_dbg_dump_wdt(chub);
		pbuf += sprintf(pbuf, "===================\n");
		pbuf += sprintf(pbuf, "CHUB WDT register\n");

		for (i = 0; i <= WDT_REG_MAX - 1; i++)
			pbuf +=
			    sprintf(pbuf, "R%02d : %08x\n", i,
				    p_dbg_dump->wdt[i]);

		chub_dbg_dump_timer(chub);
		pbuf += sprintf(pbuf, "===================\n");
		pbuf += sprintf(pbuf, "CHUB TIMER register\n");

		for (i = 0; i <= TIMER_REG_MAX - 1; i++)
			pbuf +=
			    sprintf(pbuf, "R%02d : %08x\n", i,
				    p_dbg_dump->timer[i]);

		chub_dbg_dump_pwm(chub);
		pbuf += sprintf(pbuf, "====================\n");
		pbuf += sprintf(pbuf, "CHUB PWM register\n");

		for (i = 0; i <= PWM_REG_MAX - 1; i++)
			pbuf +=
			    sprintf(pbuf, "R%02d : %08x\n", i,
				    p_dbg_dump->pwm[i]);

		chub_dbg_dump_rtc(chub);
		pbuf +=	sprintf(pbuf, "===================\n");
		pbuf += sprintf(pbuf, "CHUB RTC register\n");

		for (i = 0; i <= RTC_REG_MAX - 1; i++)
			pbuf +=
			    sprintf(pbuf, "R%02d : %08x\n", i,
				    p_dbg_dump->rtc[i]);

		chub_dbg_dump_usi(chub);

		for (j = 0; j < chub->iomem.usi_cnt; j++) {
			usi_protocol = READ_CHUB_USI_CONF(chub->iomem.usi_array[j]);
			switch (usi_protocol) {
			case USI_PROTOCOL_UART:
				pbuf +=
				    sprintf(pbuf, "===================\n");
				pbuf +=
				    sprintf(pbuf,
					    "CHUB USI%d UART register\n", j);

				for (i = 0; i <= UART_REG_MAX - 1; i++)
					pbuf +=
					    sprintf(pbuf,
						    "R%02d : %08x\n", i,
						    p_dbg_dump->usi[index++]);
				break;
			case USI_PROTOCOL_SPI:
				pbuf +=
				    sprintf(pbuf, "===================\n");
				pbuf +=
				    sprintf(pbuf,
					    "CHUB USI%d SPI register\n", j);

				for (i = 0; i <= SPI_REG_MAX - 1; i++)
					pbuf +=
					    sprintf(pbuf,
						    "R%02d : %08x\n", i,
						    p_dbg_dump->usi[index++]);
				break;
			case USI_PROTOCOL_I2C:
				pbuf +=
				    sprintf(pbuf, "===================\n");
				pbuf +=
				    sprintf(pbuf,
					    "CHUB USI%d I2C register\n", j);

				for (i = 0; i <= I2C_REG_MAX - 1; i++)
					pbuf +=
					    sprintf(pbuf,
						    "R%02d : %08x\n", i,
						    p_dbg_dump->usi[index++]);
				break;
			default:
				break;
			}
		}
	}

	if ((u32)(pbuf - buf) > 4096)
		nanohub_dev_err(dev, "show size (%u) bigger than 4096\n",
			(u32)(pbuf - buf));

	return pbuf - buf;
}

static ssize_t chub_get_gpr_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	char *pbuf = buf;
	int i;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	if (p_dbg_dump) {
		chub_dbg_dump_gpr(chub);

		pbuf +=
		    sprintf(pbuf, "========================================\n");
		pbuf += sprintf(pbuf, "CHUB CPU register dump\n");

		for (i = 0; i <= 15; i++)
			pbuf +=
			    sprintf(pbuf, "R%02d        : %08x\n", i,
				    p_dbg_dump->gpr[i]);

		pbuf +=
		    sprintf(pbuf, "PC         : %08x\n",
			    p_dbg_dump->gpr[GPR_PC_INDEX]);
		pbuf +=
		    sprintf(pbuf, "========================================\n");
	}

	return pbuf - buf;
}

static ssize_t chub_bin_dump_registers_read(struct file *file,
					    struct kobject *kobj,
					    struct bin_attribute *battr,
					    char *buf, loff_t off, size_t size)
{
	struct contexthub_ipc_info *chub = NULL;
	if (!battr || !battr->private)
		return -ENODEV;

	chub = (struct contexthub_ipc_info *)battr->private;
	if (contexthub_get_token(chub))
		return -ENODEV;

	memcpy(buf, battr->private + off, size);
	contexthub_put_token(chub);
	return size;
}

static ssize_t chub_bin_sram_read(struct file *file, struct kobject *kobj,
				  struct bin_attribute *battr, char *buf,
				  loff_t off, size_t size)
{
	struct contexthub_ipc_info *chub = NULL;

	if (!battr || !battr->private)
		return -ENODEV;

	chub = (struct contexthub_ipc_info *)battr->private;
	if (contexthub_get_token(chub))
		return -ENODEV;

	if (off == 0) {
		nanohub_info("%s: read start. request chub to flush cache\n", __func__);
		contexthub_request_chub_flush_cache(chub);
	}
	memcpy_fromio(buf, ipc_get_base(IPC_REG_DUMP) + off, size);
	contexthub_put_token(chub);
	return size;
}

static ssize_t chub_bin_dram_read(struct file *file, struct kobject *kobj,
				  struct bin_attribute *battr, char *buf,
				  loff_t off, size_t size)
{
	if (!battr || !battr->private)
		return -ENODEV;

	memcpy(buf, battr->private + off, size);
	return size;
}

static ssize_t chub_bin_dumped_sram_read(struct file *file, struct kobject *kobj,
				  struct bin_attribute *battr, char *buf,
				  loff_t off, size_t size)
{
	struct contexthub_ipc_info *chub = NULL;

	if (!battr || !battr->private)
		return -ENODEV;

	chub = (struct contexthub_ipc_info *)battr->private;
	if (contexthub_get_token(chub))
		return -ENODEV;

	if (off == 0) {
		nanohub_info("%s: read start. request chub to flush cache\n", __func__);
		contexthub_request_chub_flush_cache(chub);
	}
	memcpy(buf, chub->chub_ipc->sram_base + off, size);
	contexthub_put_token(chub);
	return size;
}

static ssize_t chub_bin_logbuf_dram_read(struct file *file, struct kobject *kobj,
				  struct bin_attribute *battr, char *buf,
				  loff_t off, size_t size)
{
	if (!battr || !battr->private)
		return -ENODEV;

	memcpy(buf, battr->private + off, size);
	return size;
}

static ssize_t chub_bin_dfs_read(struct file *file, struct kobject *kobj,
				  struct bin_attribute *battr, char *buf,
				  loff_t off, size_t size)
{
	if (!battr || !battr->private)
		return -ENODEV;

	memcpy_fromio(buf, battr->private + off, size);
	return size;
}

static BIN_ATTR_RO(chub_bin_dram, 0);
static BIN_ATTR_RO(chub_bin_dumped_sram, 0);
static BIN_ATTR_RO(chub_bin_logbuf_dram, 0);
static BIN_ATTR_RO(chub_bin_dfs, 0);
static BIN_ATTR_RO(chub_bin_sram, 0);
static BIN_ATTR_RO(chub_bin_dump_registers, 0);

static struct bin_attribute *chub_bin_attrs[] = {
	&bin_attr_chub_bin_sram,
	&bin_attr_chub_bin_dram,
	&bin_attr_chub_bin_dumped_sram,
	&bin_attr_chub_bin_logbuf_dram,
	&bin_attr_chub_bin_dfs,
	&bin_attr_chub_bin_dump_registers,
};

#define SIZE_UTC_NAME (32)

#define IPC_DBG_UTC_CIPC_TEST (IPC_DEBUG_UTC_REBOOT + 1)
char chub_utc_name[][SIZE_UTC_NAME] = {
	[IPC_DEBUG_UTC_STOP] = "stop",
	[IPC_DEBUG_UTC_AGING] = "aging",
	[IPC_DEBUG_UTC_WDT] = "wdt",
	[IPC_DEBUG_UTC_IDLE] = "idle",
	[IPC_DEBUG_UTC_TIMER] = "timer",
	[IPC_DEBUG_UTC_MEM] = "mem",
	[IPC_DEBUG_UTC_GPIO] = "gpio",
	[IPC_DEBUG_UTC_SPI] = "spi",
	[IPC_DEBUG_UTC_CMU] = "cmu",
	[IPC_DEBUG_UTC_GPIO] = "gpio",
	[IPC_DEBUG_UTC_TIME_SYNC] = "time_sync",
	[IPC_DEBUG_UTC_ASSERT] = "assert",
	[IPC_DEBUG_UTC_FAULT] = "fault",
	[IPC_DEBUG_UTC_CHECK_STATUS] = "stack",
	[IPC_DEBUG_UTC_CHECK_CPU_UTIL] = "utilization",
	[IPC_DEBUG_UTC_HEAP_DEBUG] = "heap",
	[IPC_DEBUG_UTC_HANG] = "hang",
	[IPC_DEBUG_UTC_HANG_ITMON] = "itmon",
	[IPC_DEBUG_UTC_DFS] = "dfs",
	[IPC_DEBUG_UTC_HANG_IPC_C2A_FULL] = "ipc_c2a_evt_full",
	[IPC_DEBUG_UTC_HANG_IPC_C2A_CRASH] = "ipc_c2a_evt_crash",
	[IPC_DEBUG_UTC_HANG_IPC_C2A_DATA_FULL] = "ipc_c2a_data_full",
	[IPC_DEBUG_UTC_HANG_IPC_C2A_DATA_CRASH] = "ipc_c2a_data_crash",
	[IPC_DEBUG_UTC_HANG_IPC_A2C_FULL] = "ipc_a2c_evt_full",
	[IPC_DEBUG_UTC_HANG_IPC_A2C_CRASH] = "ipc_a2c_evt_crash",
	[IPC_DEBUG_UTC_HANG_IPC_A2C_DATA_FULL] = "ipc_a2c_data_full",
	[IPC_DEBUG_UTC_HANG_IPC_A2C_DATA_CRASH] = "ipc_a2c_data_crash",
	[IPC_DEBUG_UTC_HANG_IPC_LOGBUF_EQ_CRASH] = "ipc_logbuf_eq_crash",
	[IPC_DEBUG_UTC_HANG_IPC_LOGBUF_DQ_CRASH] = "ipc_logbuf_dq_crash",
	[IPC_DEBUG_UTC_HANG_INVAL_INT] = "ipc_inval_int",
	[IPC_DEBUG_UTC_REBOOT] = "reboot(CSP_REBOOT)",
	[IPC_DBG_UTC_CIPC_TEST] = "cipc debug", /* ap can handle it */
};

#define SIZE_DFS_NAME   (32)
char chub_dfs_name[][SIZE_DFS_NAME] = {
	[DFS_GOVERNOR_OFF] = "dfs_off",
	[DFS_GOVERNOR_SIMPLE] = "dfs_simple_governor",
	[DFS_GOVERNOR_POWER] = "dfs_power_governor",
};

static ssize_t chub_alive_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	int index = 0;
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	int ret;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER) {
		index += sprintf(buf, "chub isn't alive\n");
		return index;
	}

	ret = contexthub_ipc_write_event(chub, MAILBOX_EVT_CHUB_ALIVE);
	if (!ret)
		index += sprintf(buf, "chub alive\n");
	else
		index += sprintf(buf, "chub isn't alive\n");

	return index;
}

static ssize_t chub_utc_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	int i;
	int index = 0;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	for (i = 0; i < sizeof(chub_utc_name) / SIZE_UTC_NAME; i++)
		if (chub_utc_name[i][0])
			index +=
			    sprintf(buf + index, "%d %s\n", i,
				    chub_utc_name[i]);

	return index;
}

#define MAX_UTC_ARGC  5
#define UTC_ARG_ENDCHAR  0xffffffff

static ssize_t chub_utc_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	int argc = 0;
	char *p = (char *)buf;
	char *args;
	uint32_t params[MAX_UTC_ARGC];
	int err = 0;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	for(int i = 0; i < MAX_UTC_ARGC; i++)
		params[i] = UTC_ARG_ENDCHAR;

	nanohub_dev_info(chub->dev, "%s: %s\n", __func__, buf);
	while((args = strsep(&p, " ")) != NULL && argc < MAX_UTC_ARGC) {
		if (strlen(args) == 0) continue;
		err = kstrtol(args, 10, (long *)&params[argc]);

		nanohub_dev_info(chub->dev, "%s: param %d:%lu/%s/%d\n", __func__, argc, params[argc], args, strlen(args));

		if (err) {
			if (argc) {
				params[argc] = UTC_ARG_ENDCHAR;
				break;
			}
			return 0;
		}
		argc++;
	}

	nanohub_dev_info(chub->dev, "%s: event:%d\n", __func__, params[0]);

	if (argc) {
		for (int i = 1; i < MAX_UTC_ARGC; ++i) {
			nanohub_dev_info(chub->dev, "%s: set param[%d]:%lu\n", __func__, i - 1, params[i]);
			ipc_write_shared_reg(i - 1, params[i]);
		}

		contexthub_ipc_write_event(chub, params[0]);

		if (params[0] >= IPC_DEBUG_UTC_HANG_IPC_C2A_FULL)
			ipc_dump();

		return count;
	} else {
		return 0;
	}
}

static ssize_t chub_dfs_gov_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	int i;
	int index = 0;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	for (i = 0; i < sizeof(chub_dfs_name) / SIZE_DFS_NAME; i++)
		if (chub_dfs_name[i][0])
			index +=
			    sprintf(buf + index, "%d %s\n", i,
				    chub_dfs_name[i]);

	return index;
}

static ssize_t chub_dfs_gov_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	long event;
	int ret;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	ret = kstrtol(&buf[0], 10, &event);
	if (ret)
		return ret;

	ipc_write_value(IPC_VAL_A2C_DFS, event);
	nanohub_dev_info(chub->dev, "%s: event: %d, %d\n",
			 __func__, event, ipc_read_value(IPC_VAL_A2C_DFS));
	contexthub_ipc_write_event(chub, MAILBOX_EVT_DFS_GOVERNOR);

	return count;
}

struct chub_ipc_utc {
	char name[IPC_NAME_MAX];
	enum cipc_region reg;
};

static struct chub_ipc_utc ipc_utc[] = {
	{"AP2CHUB", CIPC_REG_DATA_CHUB2AP},
	{"AP2CHUB_BATCH", CIPC_REG_DATA_CHUB2AP_BATCH},
	{"ABOX2CHUB", CIPC_REG_DATA_CHUB2ABOX},
	{"ABOX2CHUB_BAAW", CIPC_REG_DATA_CHUB2ABOX | (1 << CIPC_TEST_BAAW_REQ_BIT)},
	{"CIPC_RESET", 0},
};

static ssize_t chub_ipc_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	int i;
	int index = 0;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	for (i = 0; i < sizeof(ipc_utc) / sizeof(struct chub_ipc_utc); i++)
		index +=
		    sprintf(buf + index, "%d %s\n", i, ipc_utc[i].name);

	return index;
}

#define CIPC_TEST_SIZE (64)
static ssize_t chub_ipc_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	char input[CIPC_TEST_SIZE];
	char output[CIPC_TEST_SIZE];
	int ret;
	long event;
	int err;
	int i;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	err = kstrtol(&buf[0], 10, &event);
	nanohub_dev_info(chub->dev, "%s: event:%d\n", __func__, event);

	if (event >= ARRAY_SIZE(ipc_utc) || event < 0) {
		nanohub_dev_err(chub->dev, "%s: invalid value(%ld), 0~%d\n",
				__func__, event, ARRAY_SIZE(ipc_utc));
		return count;
	}

	if (ipc_utc[event].reg == CIPC_REG_SRAM_BASE) {
		nanohub_dev_info(chub->dev, "%s: cipc reset\n", __func__);
		cipc_reset_map();
		return count;
	}

	memset(input, 0, CIPC_TEST_SIZE);
	memset(output, 0, CIPC_TEST_SIZE);

	if (count <= CIPC_TEST_SIZE) {
		memset(output, 0, CIPC_TEST_SIZE);
		for (i = 0; i < CIPC_TEST_SIZE; i++)
			output[i] = i;
	} else {
		nanohub_dev_err(chub->dev, "%s: ipc size(%d) is bigger than max(%d)\n",
				__func__, (int)count, (int)CIPC_TEST_SIZE);
		return -EINVAL;
	}

	nanohub_dev_err(chub->dev, "%s: event:%d, reg:%d\n", __func__, event, ipc_utc[event].reg);

	ipc_write_value(IPC_VAL_A2C_DEBUG2, ipc_utc[event].reg);

	ret = contexthub_ipc_write_event(chub, (u32)IPC_DEBUG_UTC_IPC_TEST_START);
	if (ret) {
		nanohub_dev_err(chub->dev,
				"%s: fails to set start test event. ret:%d\n", __func__, ret);
		count = ret;
		goto out;
	}

	if (event == IPC_DEBUG_UTC_IPC_AP) {
		ret = contexthub_ipc_write(chub, input, count, IPC_MAX_TIMEOUT);
		if (ret != count) {
			nanohub_dev_info(chub->dev, "%s: fail to write\n", __func__);
			goto out;
		}

		ret = contexthub_ipc_read(chub, output, 0, IPC_MAX_TIMEOUT);
		if (count != ret) {
			nanohub_dev_info(chub->dev, "%s: fail to read ret:%d\n", __func__, ret);
		}

		if (strncmp(input, output, count)) {
			nanohub_dev_info(chub->dev, "%s: fail to compare input/output\n", __func__);
			print_hex_dump(KERN_CONT, "chub input:",
				       DUMP_PREFIX_OFFSET, 16, 1, input,
				       count, false);
			print_hex_dump(KERN_CONT, "chub output:",
				       DUMP_PREFIX_OFFSET, 16, 1, output,
				       count, false);
		} else
			nanohub_dev_info(chub->dev,
					 "[%s pass] len:%d, str: %s\n", __func__,
					 (int)count, output);
	} else {
		nanohub_dev_err(chub->dev, "%s: %d: %s. reg:%d\n",
				__func__, event, ipc_utc[event].name, ipc_utc[event].reg);
		msleep(1000); /* wait util chub recived it */
	}

	out:
		ret = contexthub_ipc_write_event(chub, (u32)IPC_DEBUG_UTC_IPC_TEST_END);
		if (ret) {
			nanohub_dev_err(chub->dev, "%s: fails to set end test event. ret:%d\n",
					__func__, ret);
			count = ret;
		}
	return count;
}

static ssize_t chub_get_dump_status_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	chub_dbg_dump_status(chub);
	return count;
}

static ssize_t chub_set_dump_hw_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	chub_dbg_dump_hw(chub, 0);
	return count;
}

static ssize_t chub_loglevel_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	enum ipc_fw_loglevel loglevel = chub->log.chub_rt_log.loglevel;
	int index = 0;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	nanohub_dev_info(dev, "%s: %d\n", __func__, loglevel);
	index += sprintf(buf, "%d:%s, %d:%s, %d:%s\n",
		CHUB_RT_LOG_OFF, "off", CHUB_RT_LOG_DUMP, "dump-only",
		CHUB_RT_LOG_DUMP_PRT, "dump-prt");
	index += sprintf(buf + index, "cur-loglevel: %d: %s\n",
		loglevel, !loglevel ? "off" : ((loglevel == CHUB_RT_LOG_DUMP) ? "dump-only" : "dump-prt"));

	return index;
}

static ssize_t chub_loglevel_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	long event;
	int ret;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	ret = kstrtol(&buf[0], 10, &event);
	if (ret)
		return ret;

	chub->log.chub_rt_log.loglevel = (enum ipc_fw_loglevel)event;
	nanohub_dev_info(dev, "%s: %d->%d\n", __func__, event, chub->log.chub_rt_log.loglevel);
	contexthub_ipc_write_event(chub, MAILBOX_EVT_RT_LOGLEVEL);

	return count;
}

static ssize_t chub_clk_div_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	int index = 0;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	index += sprintf(buf, "clk_div: [%lu], CLK_BASE:[%u]\n", chub->misc.clkrate, CHUB_CLK_BASE);
	index += sprintf(buf + index, "usage: echo [0-7] > clk_div\n");

	nanohub_dev_info(chub->dev, "%s: clk_div: [%u], CLK_BASE:[%u]\n",
			 __func__, chub->misc.clkrate, CHUB_CLK_BASE);

	return index;
}

static ssize_t chub_clk_div_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	u32 old;
	long div;
	int ret;

	if (atomic_read(&chub->atomic.chub_status) == CHUB_ST_NO_POWER)
		return -ENODEV;

	old = chub->misc.clkrate;
	ret = kstrtol(&buf[0], 10, &div);
	if (ret)
		return ret;

	if (div < 0 || div > 7) {
		nanohub_dev_info(dev, "%s: invalid div value %ld\n", __func__, div);
		return -EINVAL;
	}
	chub->misc.clkrate = CHUB_CLK_BASE/(div + 1);
	nanohub_dev_info(dev, "%s: %u->%u\n", __func__, old, chub->misc.clkrate);
	ipc_set_chub_clk(chub->misc.clkrate);
	contexthub_ipc_write_event(chub, MAILBOX_EVT_CLK_DIV);

	return count;
}

static ssize_t chub_policy_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	int force_wdt = atomic_read(&chub->atomic.force_wdt);

	if (force_wdt == 2)
		return sprintf(buf, "2: AP watchdog reset instead of chub reset.\n");
	else if (force_wdt == 1)
		return sprintf(buf, "1: AP watchdog reset when CHUB hardfault execption occurs. (CFSR:0x%08x)\n",
				atomic_read(&chub->atomic.force_wdt_cfsr));
	else
		return sprintf(buf, "0: Silent Reset when CHUB hardfault execption occurs.\n");

}

#define MAX_POLICY_ARGC 2
static ssize_t chub_policy_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct contexthub_ipc_info *chub = get_contexthub_info_from_dev(dev);
	int argc = 0;
	char *p = (char *)buf;
	char *args;
	long params[MAX_POLICY_ARGC];
	int err = 0;

	for(int i = 0; i < MAX_POLICY_ARGC; i++)
		params[i] = 0;

	while((args = strsep(&p, " ")) != NULL && argc < MAX_POLICY_ARGC) {
		if (strlen(args) == 0) continue;
		err = kstrtol(args, 16, &params[argc]);

		if (err)
			break;
		argc++;
	}

	if (!argc)
		return -EINVAL;

	nanohub_dev_info(dev, "%s: force_wdt:%d (CFSR:0x%08x)\n", __func__, params[0], params[1]);

	if (params[0] == 2)
	    atomic_set(&chub->atomic.force_wdt, 2);
	else if (params[0] == 1)
	    atomic_set(&chub->atomic.force_wdt, 1);
	else
	    atomic_set(&chub->atomic.force_wdt, 0);
	atomic_set(&chub->atomic.force_wdt_cfsr, (uint32_t) params[1]);

	return count;
}

static struct device_attribute attributes[] = {
	__ATTR(get_gpr, 0440, chub_get_gpr_show, NULL),
	__ATTR(get_chub_register, 0440, chub_get_chub_register_show, NULL),
	__ATTR(dump_status, 0220, NULL, chub_get_dump_status_store),
	__ATTR(dump_hw, 0220, NULL, chub_set_dump_hw_store),
	__ATTR(utc, 0664, chub_utc_show, chub_utc_store),
	__ATTR(dfs_gov, 0664, chub_dfs_gov_show, chub_dfs_gov_store),
	__ATTR(ipc_test, 0664, chub_ipc_show, chub_ipc_store),
	__ATTR(alive, 0440, chub_alive_show, NULL),
	__ATTR(loglevel, 0664, chub_loglevel_show, chub_loglevel_store),
	__ATTR(clk_div, 0664, chub_clk_div_show, chub_clk_div_store),
	__ATTR(policy, 0600, chub_policy_show, chub_policy_store),
};

void chub_dbg_get_memory(struct device_node *node)
{
	struct device_node *np;

	pr_info("%s: chub_rmem\n", __func__);

	np = of_parse_phandle(node, "memory-region", 0);
	if (!np)
		pr_err("%s memory region not parsed!!", __func__);
	else
		chub_rmem = of_reserved_mem_lookup(np);

	if (!chub_rmem) {
		pr_err("%s: rmem not available, kmalloc instead", __func__);
		p_dbg_dump = kmalloc(SZ_2M, GFP_KERNEL);
	} else
		p_dbg_dump = phys_to_virt(chub_rmem->base);
}

void chub_dbg_register_dump_to_dss(uint32_t sram_phys, uint32_t sram_size)
{
	if (!chub_rmem) {
		pr_info("chub_rmem not available. skip register dram dump\n");
	} else {
		pr_info("register chub_dram to dss\n");
		pr_info("chub_dram info : base : %pK, size : 0x%llX\n", (void *) chub_rmem->base, chub_rmem->size);
		dbg_snapshot_add_bl_item_info("chub_dram", chub_rmem->base, chub_rmem->size);
	}
}


int chub_dbg_create_attribute(struct contexthub_ipc_info *chub)
{
	int i, ret = 0;
	struct device *dev;
	struct device *sensor_dev = NULL;

	if (!chub)
		return -EINVAL;

	bin_attr_chub_bin_dumped_sram.size = chub->iomem.sram_size;
	bin_attr_chub_bin_dram.size = sizeof(struct dbg_dump);
	bin_attr_chub_bin_sram.size = chub->iomem.sram_size;
	bin_attr_chub_bin_logbuf_dram.size = SIZE_OF_BUFFER;
	bin_attr_chub_bin_dfs.size = sizeof(struct chub_dfs);
	bin_attr_chub_bin_dump_registers.size = get_chub_dumped_registers(chub->iomem.usi_cnt);

	sensor_dev = dev = chub->dev;
#ifdef CONFIG_SENSOR_DRV
	if (chub->data)
		sensor_dev = chub->data->io.dev;
#endif
	for (i = 0; i < ARRAY_SIZE(chub_bin_attrs); i++) {
		struct bin_attribute *battr = chub_bin_attrs[i];
		battr->attr.mode = 0440;

		ret = device_create_bin_file(sensor_dev, battr);
		if (ret < 0) {
			nanohub_dev_warn(sensor_dev, "Failed to create file: %s\n",
				 battr->attr.name);
			return ret;
		}
	}

	for (i = 0, ret = 0; i < ARRAY_SIZE(attributes); i++) {
		ret = device_create_file(sensor_dev, &attributes[i]);
		if (ret) {
			nanohub_dev_warn(dev, "Failed to create file: %s\n",
				 attributes[i].attr.name);
			return ret;
		}
	}

	return ret;
}


int chub_dbg_init(struct contexthub_ipc_info *chub, void *kernel_logbuf, int kernel_logbuf_size)
{
	int ret = 0;
	enum dbg_dump_area area;

	struct device *dev;
	struct device *sensor_dev = NULL;

	if (!chub)
		return -EINVAL;

	sensor_dev = dev = chub->dev;
#ifdef CONFIG_SENSOR_DRV
	if (chub->data)
		sensor_dev = chub->data->io.dev;
#endif

	nanohub_info("%s: %s: %s\n", __func__, dev_name(dev), dev_name(sensor_dev));

	bin_attr_chub_bin_dumped_sram.private = chub;

	bin_attr_chub_bin_dram.private= p_dbg_dump;
	bin_attr_chub_bin_sram.private = chub;
	bin_attr_chub_bin_logbuf_dram.private = kernel_logbuf;
	bin_attr_chub_bin_dfs.private = ipc_get_base(IPC_REG_IPC) + CHUB_PERSISTBUF_SIZE;
	bin_attr_chub_bin_dump_registers.private = p_dbg_dump->gpr;

	if (chub_rmem && chub_rmem->size < get_dbg_dump_size())
		nanohub_dev_err(dev,
			"rmem size (%u) should be bigger than dump size(%u)\n",
			(u32)chub_rmem->size, get_dbg_dump_size());

	area = DBG_IPC_AREA;
	strncpy(p_dbg_dump->info[area].name, "ipc_map", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
	    (void *)p_dbg_dump->ipc_addr - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(struct ipc_area) * IPC_REG_MAX;

	area = DBG_NANOHUB_DD_AREA;
	strncpy(p_dbg_dump->info[area].name, "nano_dd", AREA_NAME_MAX);
	/*
	 * FIXME: the object of contexthub_ipc_info is now removed from dbg_dump
	 * So, consider if the content of contexthub_ipc_info is necessary to be in
	 * dbg_info again. If so, try another way to include the snapshot of
	 * contexthub_ipc_info in dbg_dump.
	 */
	p_dbg_dump->info[area].offset = 0;
	p_dbg_dump->info[area].size = (u32)sizeof(struct contexthub_ipc_info);

	area = DBG_GPR_AREA;
	strncpy(p_dbg_dump->info[area].name, "gpr", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
	    (void *)p_dbg_dump->gpr - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(u32) * NUM_OF_GPR;

	area = DBG_CMU_AREA;
	strncpy(p_dbg_dump->info[area].name, "cmu", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
		(void *)p_dbg_dump->cmu - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(u32) * CMU_REG_MAX;

	area = DBG_SYS_AREA;
	strncpy(p_dbg_dump->info[area].name, "sys", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
	    (void *)p_dbg_dump->sys - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(u32) * SYS_REG_MAX;

	area = DBG_WDT_AREA;
	strncpy(p_dbg_dump->info[area].name, "wdt", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
	    (void *)p_dbg_dump->wdt - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(u32) * WDT_REG_MAX;

	area = DBG_TIMER_AREA;
	strncpy(p_dbg_dump->info[area].name, "timer", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
	    (void *)p_dbg_dump->timer - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(u32) * TIMER_REG_MAX;

	area = DBG_PWM_AREA;
	strncpy(p_dbg_dump->info[area].name, "pwm", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
	    (void *)p_dbg_dump->pwm - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(u32) * PWM_REG_MAX;

	area = DBG_RTC_AREA;
	strncpy(p_dbg_dump->info[area].name, "rtc", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
	    (void *)p_dbg_dump->rtc - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(u32) * RTC_REG_MAX;

	area = DBG_USI_AREA;
	strncpy(p_dbg_dump->info[area].name, "usi", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset =
	    (void *)p_dbg_dump->usi - (void *)p_dbg_dump;
	p_dbg_dump->info[area].size = sizeof(u32) *
						USI_REG_MAX * MAX_USI_CNT;

	area = DBG_SRAM_AREA;
	strncpy(p_dbg_dump->info[area].name, "sram", AREA_NAME_MAX);
	p_dbg_dump->info[area].offset = offsetof(struct dbg_dump, sram);
	p_dbg_dump->info[area].size = bin_attr_chub_bin_sram.size;

	nanohub_dev_info(dev,
		"%s is mapped (startoffset:%d) with dump size %u\n",
		"dump buffer", offsetof(struct dbg_dump, sram), get_dbg_dump_size());

	return ret;
}
