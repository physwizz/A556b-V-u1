// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Exynos Specific Extensions for Synopsys DW Multimedia Card Interface driver
 *
 * Copyright (C) 2012, Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/smc.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/bitmap.h>
#include <soc/samsung/exynos/memlogger.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"
#include "dw_mmc-exynos.h"
#if IS_ENABLED(CONFIG_SEC_MMC_FEATURE)
#include "mmc-sec-feature.h"
#endif

/* Since MRR is a 6-bit value, it must have a value of 0~63. */
#define SSC_MRR_LIMIT	63
/* Since MFR is a 8-bit value, it must have a value of 0~255. */
#define SSC_MFR_LIMIT	255

extern int cal_pll_mmc_set_ssc(unsigned int mfr, unsigned int mrr, unsigned int ssc_on);
extern int cal_pll_mmc_check(void);

struct exynos_mmc_memlog {
	struct memlog *desc;
	struct memlog_obj *log_obj_file;
	struct memlog_obj *log_obj;
	unsigned int log_enable;
};

#define pr_memlog(dev, fmt, ...)					\
	do {								\
		if (dev.log_enable)					\
			memlog_write_printf(dev.log_obj,		\
					MEMLOG_LEVEL_EMERG,		\
					fmt, ##__VA_ARGS__);		\
		else							\
			pr_err(fmt, ##__VA_ARGS__);			\
	} while (0)

struct exynos_mmc_memlog mmc_memlog;

static void dw_mci_exynos_register_dump(struct dw_mci *host)
{
	pr_memlog(mmc_memlog, ": EMMCP_BASE:	0x%08x\n",
		host->sfr_dump->fmp_emmcp_base = mci_readl(host, EMMCP_BASE));
	pr_memlog(mmc_memlog, ": MPSECURITY:	0x%08x\n",
		host->sfr_dump->mpsecurity = mci_readl(host, MPSECURITY));
	pr_memlog(mmc_memlog, ": MPSTAT:		0x%08x\n",
		host->sfr_dump->mpstat = mci_readl(host, MPSTAT));
	pr_memlog(mmc_memlog, ": MPSBEGIN:		0x%08x\n",
		host->sfr_dump->mpsbegin = mci_readl(host, MPSBEGIN0));
	pr_memlog(mmc_memlog, ": MPSEND:		0x%08x\n",
		host->sfr_dump->mpsend = mci_readl(host, MPSEND0));
	pr_memlog(mmc_memlog, ": MPSCTRL:		0x%08x\n",
		host->sfr_dump->mpsctrl = mci_readl(host, MPSCTRL0));
	pr_memlog(mmc_memlog, ": HS400_RCLK_EN:  	0x%08x\n",
		host->sfr_dump->hs400_rdqs_en = mci_readl(host, HS400_RCLK_EN));
	pr_memlog(mmc_memlog, ": HS400_ASYNC_FIFO_CTRL:   0x%08x\n",
		host->sfr_dump->hs400_acync_fifo_ctrl = mci_readl(host, HS400_ASYNC_FIFO_CTRL));
	pr_memlog(mmc_memlog, ": HS400_DLINE_CTRL:        0x%08x\n",
		host->sfr_dump->hs400_dline_ctrl = mci_readl(host, HS400_DLINE_CTRL));
#if IS_ENABLED(MMC_DW_EXYNOS_DUMP_TO_CONSOLE)
	dev_err(host->dev, ": EMMCP_BASE:	0x%08x\n",
		host->sfr_dump->fmp_emmcp_base = mci_readl(host, EMMCP_BASE));
	dev_err(host->dev, ": MPSECURITY:	0x%08x\n",
		host->sfr_dump->mpsecurity = mci_readl(host, MPSECURITY));
	dev_err(host->dev, ": MPSTAT:		0x%08x\n",
		host->sfr_dump->mpstat = mci_readl(host, MPSTAT));
	dev_err(host->dev, ": MPSBEGIN:		0x%08x\n",
		host->sfr_dump->mpsbegin = mci_readl(host, MPSBEGIN0));
	dev_err(host->dev, ": MPSEND:		0x%08x\n",
		host->sfr_dump->mpsend = mci_readl(host, MPSEND0));
	dev_err(host->dev, ": MPSCTRL:		0x%08x\n",
		host->sfr_dump->mpsctrl = mci_readl(host, MPSCTRL0));
	dev_err(host->dev, ": HS400_RCLK_EN:  	0x%08x\n",
		host->sfr_dump->hs400_rdqs_en = mci_readl(host, HS400_RCLK_EN));
	dev_err(host->dev, ": HS400_ASYNC_FIFO_CTRL:   0x%08x\n",
		host->sfr_dump->hs400_acync_fifo_ctrl = mci_readl(host, HS400_ASYNC_FIFO_CTRL));
	dev_err(host->dev, ": HS400_DLINE_CTRL:        0x%08x\n",
		host->sfr_dump->hs400_dline_ctrl = mci_readl(host, HS400_DLINE_CTRL));
#endif
}

static void dw_mci_reg_dump(struct dw_mci *host)
{
	u32 reg;

	pr_memlog(mmc_memlog, ": ============== REGISTER DUMP ==============\n");
	pr_memlog(mmc_memlog, ": CTRL:	 0x%08x\n", host->sfr_dump->contrl = mci_readl(host, CTRL));
	pr_memlog(mmc_memlog, ": PWREN:	 0x%08x\n", host->sfr_dump->pwren = mci_readl(host, PWREN));
	pr_memlog(mmc_memlog, ": CLKDIV:	 0x%08x\n",
		host->sfr_dump->clkdiv = mci_readl(host, CLKDIV));
	pr_memlog(mmc_memlog, ": CLKSRC:	 0x%08x\n",
		host->sfr_dump->clksrc = mci_readl(host, CLKSRC));
	pr_memlog(mmc_memlog, ": CLKENA:	 0x%08x\n",
		host->sfr_dump->clkena = mci_readl(host, CLKENA));
	pr_memlog(mmc_memlog, ": TMOUT:	 0x%08x\n", host->sfr_dump->tmout = mci_readl(host, TMOUT));
	pr_memlog(mmc_memlog, ": CTYPE:	 0x%08x\n", host->sfr_dump->ctype = mci_readl(host, CTYPE));
	pr_memlog(mmc_memlog, ": BLKSIZ:	 0x%08x\n",
		host->sfr_dump->blksiz = mci_readl(host, BLKSIZ));
	pr_memlog(mmc_memlog, ": BYTCNT:	 0x%08x\n",
		host->sfr_dump->bytcnt = mci_readl(host, BYTCNT));
	pr_memlog(mmc_memlog, ": INTMSK:	 0x%08x\n",
		host->sfr_dump->intmask = mci_readl(host, INTMASK));
	pr_memlog(mmc_memlog, ": CMDARG:	 0x%08x\n",
		host->sfr_dump->cmdarg = mci_readl(host, CMDARG));
	pr_memlog(mmc_memlog, ": CMD:	 0x%08x\n", host->sfr_dump->cmd = mci_readl(host, CMD));
	pr_memlog(mmc_memlog, ": RESP0:	 0x%08x\n", mci_readl(host, RESP0));
	pr_memlog(mmc_memlog, ": RESP1:	 0x%08x\n", mci_readl(host, RESP1));
	pr_memlog(mmc_memlog, ": RESP2:	 0x%08x\n", mci_readl(host, RESP2));
	pr_memlog(mmc_memlog, ": RESP3:	 0x%08x\n", mci_readl(host, RESP3));
	pr_memlog(mmc_memlog, ": MINTSTS:	 0x%08x\n",
		host->sfr_dump->mintsts = mci_readl(host, MINTSTS));
	pr_memlog(mmc_memlog, ": RINTSTS:	 0x%08x\n",
		host->sfr_dump->rintsts = mci_readl(host, RINTSTS));
	pr_memlog(mmc_memlog, ": STATUS:	 0x%08x\n",
		host->sfr_dump->status = mci_readl(host, STATUS));
	pr_memlog(mmc_memlog, ": FIFOTH:	 0x%08x\n",
		host->sfr_dump->fifoth = mci_readl(host, FIFOTH));
	pr_memlog(mmc_memlog, ": CDETECT:	 0x%08x\n", mci_readl(host, CDETECT));
	pr_memlog(mmc_memlog, ": WRTPRT:	 0x%08x\n", mci_readl(host, WRTPRT));
	pr_memlog(mmc_memlog, ": GPIO:	 0x%08x\n", mci_readl(host, GPIO));
	pr_memlog(mmc_memlog, ": TCBCNT:	 0x%08x\n",
		host->sfr_dump->tcbcnt = mci_readl(host, TCBCNT));
	pr_memlog(mmc_memlog, ": TBBCNT:	 0x%08x\n",
		host->sfr_dump->tbbcnt = mci_readl(host, TBBCNT));
	pr_memlog(mmc_memlog, ": DEBNCE:	 0x%08x\n", mci_readl(host, DEBNCE));
	pr_memlog(mmc_memlog, ": USRID:	 0x%08x\n", mci_readl(host, USRID));
	pr_memlog(mmc_memlog, ": VERID:	 0x%08x\n", mci_readl(host, VERID));
	pr_memlog(mmc_memlog, ": HCON:	 0x%08x\n", mci_readl(host, HCON));
	pr_memlog(mmc_memlog, ": UHS_REG:	 0x%08x\n",
		host->sfr_dump->uhs_reg = mci_readl(host, UHS_REG));
	pr_memlog(mmc_memlog, ": BMOD:	 0x%08x\n", host->sfr_dump->bmod = mci_readl(host, BMOD));
	pr_memlog(mmc_memlog, ": PLDMND:	 0x%08x\n", mci_readl(host, PLDMND));
	pr_memlog(mmc_memlog, ": DBADDRL:	 0x%08x\n",
		host->sfr_dump->dbaddrl = mci_readl(host, DBADDRL));
	pr_memlog(mmc_memlog, ": DBADDRU:	 0x%08x\n",
		host->sfr_dump->dbaddru = mci_readl(host, DBADDRU));
	pr_memlog(mmc_memlog, ": DSCADDRL:	 0x%08x\n",
		host->sfr_dump->dscaddrl = mci_readl(host, DSCADDRL));
	pr_memlog(mmc_memlog, ": DSCADDRU:	 0x%08x\n",
		host->sfr_dump->dscaddru = mci_readl(host, DSCADDRU));
	pr_memlog(mmc_memlog, ": BUFADDR:	 0x%08x\n",
		host->sfr_dump->bufaddr = mci_readl(host, BUFADDR));
	pr_memlog(mmc_memlog, ": BUFADDRU:	 0x%08x\n",
		host->sfr_dump->bufaddru = mci_readl(host, BUFADDRU));
	pr_memlog(mmc_memlog, ": DBADDR:	 0x%08x\n",
		host->sfr_dump->dbaddr = mci_readl(host, DBADDR));
	pr_memlog(mmc_memlog, ": DSCADDR:	 0x%08x\n",
		host->sfr_dump->dscaddr = mci_readl(host, DSCADDR));
	pr_memlog(mmc_memlog, ": BUFADDR:	 0x%08x\n",
		host->sfr_dump->bufaddr = mci_readl(host, BUFADDR));
	pr_memlog(mmc_memlog, ": CLKSEL:    0x%08x\n",
		host->sfr_dump->clksel = mci_readl(host, CLKSEL));
	pr_memlog(mmc_memlog, ": IDSTS:	 0x%08x\n", mci_readl(host, IDSTS));
	pr_memlog(mmc_memlog, ": IDSTS64:	 0x%08x\n",
		host->sfr_dump->idsts64 = mci_readl(host, IDSTS64));
	pr_memlog(mmc_memlog, ": IDINTEN:	 0x%08x\n", mci_readl(host, IDINTEN));
	pr_memlog(mmc_memlog, ": IDINTEN64: 0x%08x\n",
		host->sfr_dump->idinten64 = mci_readl(host, IDINTEN64));
	pr_memlog(mmc_memlog, ": RESP_TAT: 0x%08x\n", mci_readl(host, RESP_TAT));
	pr_memlog(mmc_memlog, ": FORCE_CLK_STOP: 0x%08x\n",
		host->sfr_dump->force_clk_stop = mci_readl(host, FORCE_CLK_STOP));
	pr_memlog(mmc_memlog, ": CDTHRCTL: 0x%08x\n", mci_readl(host, CDTHRCTL));
	if (host->ch_id == 0)
		dw_mci_exynos_register_dump(host);
	pr_memlog(mmc_memlog, ": ============== STATUS DUMP ================\n");
	pr_memlog(mmc_memlog, ": cmd_status:      0x%08x\n",
		host->sfr_dump->cmd_status = host->cmd_status);
	pr_memlog(mmc_memlog, ": data_status:     0x%08x\n",
		host->sfr_dump->force_clk_stop = host->data_status);
	pr_memlog(mmc_memlog, ": pending_events:  0x%08lx\n",
		host->sfr_dump->pending_events = host->pending_events);
	pr_memlog(mmc_memlog, ": completed_events:0x%08lx\n",
		host->sfr_dump->completed_events = host->completed_events);
	pr_memlog(mmc_memlog, ": state:           %d\n", host->sfr_dump->host_state = host->state);
	pr_memlog(mmc_memlog, ": gate-clk:            %s\n",
		atomic_read(&host->ciu_clk_cnt) ? "enable" : "disable");
	pr_memlog(mmc_memlog, ": ciu_en_win:           %d\n", atomic_read(&host->ciu_en_win));
	reg = mci_readl(host, CMD);
	pr_memlog(mmc_memlog, ": ================= CMD REG =================\n");
	if ((reg >> 9) & 0x1) {
		pr_memlog(mmc_memlog, ": read/write        : %s\n",
			(reg & (0x1 << 10)) ? "write" : "read");
		pr_memlog(mmc_memlog, ": data expected     : %d\n", (reg >> 9) & 0x1);
	}
	pr_memlog(mmc_memlog, ": cmd index         : %d\n",
		host->sfr_dump->cmd_index = ((reg >> 0) & 0x3f));
	reg = mci_readl(host, STATUS);
	pr_memlog(mmc_memlog, ": ================ STATUS REG ===============\n");
	pr_memlog(mmc_memlog, ": fifocount         : %d\n",
		host->sfr_dump->fifo_count = ((reg >> 17) & 0x1fff));
	pr_memlog(mmc_memlog, ": response index    : %d\n", (reg >> 11) & 0x3f);
	pr_memlog(mmc_memlog, ": data state mc busy: %d\n", (reg >> 10) & 0x1);
	pr_memlog(mmc_memlog, ": data busy         : %d\n",
		host->sfr_dump->data_busy = ((reg >> 9) & 0x1));
	pr_memlog(mmc_memlog, ": data 3 state      : %d\n",
		host->sfr_dump->data_3_state = ((reg >> 8) & 0x1));
	pr_memlog(mmc_memlog, ": command fsm state : %d\n", (reg >> 4) & 0xf);
	pr_memlog(mmc_memlog, ": fifo full         : %d\n", (reg >> 3) & 0x1);
	pr_memlog(mmc_memlog, ": fifo empty        : %d\n", (reg >> 2) & 0x1);
	pr_memlog(mmc_memlog, ": fifo tx watermark : %d\n",
		host->sfr_dump->fifo_tx_watermark = ((reg >> 1) & 0x1));
	pr_memlog(mmc_memlog, ": fifo rx watermark : %d\n",
		host->sfr_dump->fifo_rx_watermark = ((reg >> 0) & 0x1));
	pr_memlog(mmc_memlog, ": ===========================================\n");

#if IS_ENABLED(MMC_DW_EXYNOS_DUMP_TO_CONSOLE)
	dev_err(host->dev, ": ============== REGISTER DUMP ==============\n");
	dev_err(host->dev, ": CTRL:	 0x%08x\n", host->sfr_dump->contrl = mci_readl(host, CTRL));
	dev_err(host->dev, ": PWREN:	 0x%08x\n", host->sfr_dump->pwren = mci_readl(host, PWREN));
	dev_err(host->dev, ": CLKDIV:	 0x%08x\n",
		host->sfr_dump->clkdiv = mci_readl(host, CLKDIV));
	dev_err(host->dev, ": CLKSRC:	 0x%08x\n",
		host->sfr_dump->clksrc = mci_readl(host, CLKSRC));
	dev_err(host->dev, ": CLKENA:	 0x%08x\n",
		host->sfr_dump->clkena = mci_readl(host, CLKENA));
	dev_err(host->dev, ": TMOUT:	 0x%08x\n", host->sfr_dump->tmout = mci_readl(host, TMOUT));
	dev_err(host->dev, ": CTYPE:	 0x%08x\n", host->sfr_dump->ctype = mci_readl(host, CTYPE));
	dev_err(host->dev, ": BLKSIZ:	 0x%08x\n",
		host->sfr_dump->blksiz = mci_readl(host, BLKSIZ));
	dev_err(host->dev, ": BYTCNT:	 0x%08x\n",
		host->sfr_dump->bytcnt = mci_readl(host, BYTCNT));
	dev_err(host->dev, ": INTMSK:	 0x%08x\n",
		host->sfr_dump->intmask = mci_readl(host, INTMASK));
	dev_err(host->dev, ": CMDARG:	 0x%08x\n",
		host->sfr_dump->cmdarg = mci_readl(host, CMDARG));
	dev_err(host->dev, ": CMD:	 0x%08x\n", host->sfr_dump->cmd = mci_readl(host, CMD));
	dev_err(host->dev, ": RESP0:	 0x%08x\n", mci_readl(host, RESP0));
	dev_err(host->dev, ": RESP1:	 0x%08x\n", mci_readl(host, RESP1));
	dev_err(host->dev, ": RESP2:	 0x%08x\n", mci_readl(host, RESP2));
	dev_err(host->dev, ": RESP3:	 0x%08x\n", mci_readl(host, RESP3));
	dev_err(host->dev, ": MINTSTS:	 0x%08x\n",
		host->sfr_dump->mintsts = mci_readl(host, MINTSTS));
	dev_err(host->dev, ": RINTSTS:	 0x%08x\n",
		host->sfr_dump->rintsts = mci_readl(host, RINTSTS));
	dev_err(host->dev, ": STATUS:	 0x%08x\n",
		host->sfr_dump->status = mci_readl(host, STATUS));
	dev_err(host->dev, ": FIFOTH:	 0x%08x\n",
		host->sfr_dump->fifoth = mci_readl(host, FIFOTH));
	dev_err(host->dev, ": CDETECT:	 0x%08x\n", mci_readl(host, CDETECT));
	dev_err(host->dev, ": WRTPRT:	 0x%08x\n", mci_readl(host, WRTPRT));
	dev_err(host->dev, ": GPIO:	 0x%08x\n", mci_readl(host, GPIO));
	dev_err(host->dev, ": TCBCNT:	 0x%08x\n",
		host->sfr_dump->tcbcnt = mci_readl(host, TCBCNT));
	dev_err(host->dev, ": TBBCNT:	 0x%08x\n",
		host->sfr_dump->tbbcnt = mci_readl(host, TBBCNT));
	dev_err(host->dev, ": DEBNCE:	 0x%08x\n", mci_readl(host, DEBNCE));
	dev_err(host->dev, ": USRID:	 0x%08x\n", mci_readl(host, USRID));
	dev_err(host->dev, ": VERID:	 0x%08x\n", mci_readl(host, VERID));
	dev_err(host->dev, ": HCON:	 0x%08x\n", mci_readl(host, HCON));
	dev_err(host->dev, ": UHS_REG:	 0x%08x\n",
		host->sfr_dump->uhs_reg = mci_readl(host, UHS_REG));
	dev_err(host->dev, ": BMOD:	 0x%08x\n", host->sfr_dump->bmod = mci_readl(host, BMOD));
	dev_err(host->dev, ": PLDMND:	 0x%08x\n", mci_readl(host, PLDMND));
	dev_err(host->dev, ": DBADDRL:	 0x%08x\n",
		host->sfr_dump->dbaddrl = mci_readl(host, DBADDRL));
	dev_err(host->dev, ": DBADDRU:	 0x%08x\n",
		host->sfr_dump->dbaddru = mci_readl(host, DBADDRU));
	dev_err(host->dev, ": DSCADDRL:	 0x%08x\n",
		host->sfr_dump->dscaddrl = mci_readl(host, DSCADDRL));
	dev_err(host->dev, ": DSCADDRU:	 0x%08x\n",
		host->sfr_dump->dscaddru = mci_readl(host, DSCADDRU));
	dev_err(host->dev, ": BUFADDR:	 0x%08x\n",
		host->sfr_dump->bufaddr = mci_readl(host, BUFADDR));
	dev_err(host->dev, ": BUFADDRU:	 0x%08x\n",
		host->sfr_dump->bufaddru = mci_readl(host, BUFADDRU));
	dev_err(host->dev, ": DBADDR:	 0x%08x\n",
		host->sfr_dump->dbaddr = mci_readl(host, DBADDR));
	dev_err(host->dev, ": DSCADDR:	 0x%08x\n",
		host->sfr_dump->dscaddr = mci_readl(host, DSCADDR));
	dev_err(host->dev, ": BUFADDR:	 0x%08x\n",
		host->sfr_dump->bufaddr = mci_readl(host, BUFADDR));
	dev_err(host->dev, ": CLKSEL:    0x%08x\n",
		host->sfr_dump->clksel = mci_readl(host, CLKSEL));
	dev_err(host->dev, ": IDSTS:	 0x%08x\n", mci_readl(host, IDSTS));
	dev_err(host->dev, ": IDSTS64:	 0x%08x\n",
		host->sfr_dump->idsts64 = mci_readl(host, IDSTS64));
	dev_err(host->dev, ": IDINTEN:	 0x%08x\n", mci_readl(host, IDINTEN));
	dev_err(host->dev, ": IDINTEN64: 0x%08x\n",
		host->sfr_dump->idinten64 = mci_readl(host, IDINTEN64));
	dev_err(host->dev, ": RESP_TAT: 0x%08x\n", mci_readl(host, RESP_TAT));
	dev_err(host->dev, ": FORCE_CLK_STOP: 0x%08x\n",
		host->sfr_dump->force_clk_stop = mci_readl(host, FORCE_CLK_STOP));
	dev_err(host->dev, ": CDTHRCTL: 0x%08x\n", mci_readl(host, CDTHRCTL));
	if (host->ch_id == 0)
		dw_mci_exynos_register_dump(host);
	dev_err(host->dev, ": ============== STATUS DUMP ================\n");
	dev_err(host->dev, ": cmd_status:      0x%08x\n",
		host->sfr_dump->cmd_status = host->cmd_status);
	dev_err(host->dev, ": data_status:     0x%08x\n",
		host->sfr_dump->force_clk_stop = host->data_status);
	dev_err(host->dev, ": pending_events:  0x%08lx\n",
		host->sfr_dump->pending_events = host->pending_events);
	dev_err(host->dev, ": completed_events:0x%08lx\n",
		host->sfr_dump->completed_events = host->completed_events);
	dev_err(host->dev, ": state:           %d\n", host->sfr_dump->host_state = host->state);
	dev_err(host->dev, ": gate-clk:            %s\n",
		atomic_read(&host->ciu_clk_cnt) ? "enable" : "disable");
	dev_err(host->dev, ": ciu_en_win:           %d\n", atomic_read(&host->ciu_en_win));
	reg = mci_readl(host, CMD);
	dev_err(host->dev, ": ================= CMD REG =================\n");
	if ((reg >> 9) & 0x1) {
		dev_err(host->dev, ": read/write        : %s\n",
			(reg & (0x1 << 10)) ? "write" : "read");
		dev_err(host->dev, ": data expected     : %d\n", (reg >> 9) & 0x1);
	}
	dev_err(host->dev, ": cmd index         : %d\n",
		host->sfr_dump->cmd_index = ((reg >> 0) & 0x3f));
	reg = mci_readl(host, STATUS);
	dev_err(host->dev, ": ================ STATUS REG ===============\n");
	dev_err(host->dev, ": fifocount         : %d\n",
		host->sfr_dump->fifo_count = ((reg >> 17) & 0x1fff));
	dev_err(host->dev, ": response index    : %d\n", (reg >> 11) & 0x3f);
	dev_err(host->dev, ": data state mc busy: %d\n", (reg >> 10) & 0x1);
	dev_err(host->dev, ": data busy         : %d\n",
		host->sfr_dump->data_busy = ((reg >> 9) & 0x1));
	dev_err(host->dev, ": data 3 state      : %d\n",
		host->sfr_dump->data_3_state = ((reg >> 8) & 0x1));
	dev_err(host->dev, ": command fsm state : %d\n", (reg >> 4) & 0xf);
	dev_err(host->dev, ": fifo full         : %d\n", (reg >> 3) & 0x1);
	dev_err(host->dev, ": fifo empty        : %d\n", (reg >> 2) & 0x1);
	dev_err(host->dev, ": fifo tx watermark : %d\n",
		host->sfr_dump->fifo_tx_watermark = ((reg >> 1) & 0x1));
	dev_err(host->dev, ": fifo rx watermark : %d\n",
		host->sfr_dump->fifo_rx_watermark = ((reg >> 0) & 0x1));
	dev_err(host->dev, ": ===========================================\n");
#endif
}

/* Variations in Exynos specific dw-mshc controller */
enum dw_mci_exynos_type {
	DW_MCI_TYPE_EXYNOS,
};

static struct dw_mci_exynos_compatible {
	char *compatible;
	enum dw_mci_exynos_type ctrl_type;
} exynos_compat[] = {
	{
.compatible = "samsung,exynos-dw-mshc",.ctrl_type = DW_MCI_TYPE_EXYNOS,},};

static inline u8 dw_mci_exynos_get_ciu_div(struct dw_mci *host)
{
	return SDMMC_CLKSEL_GET_DIV(mci_readl(host, CLKSEL)) + 1;
}

static int dw_mci_exynos_iocc_parse_dt(struct dw_mci *host)
{
	struct device_node *np;
	struct exynos_access_cxt *cxt = &host->cxt_coherency;
	int ret = -EINVAL;
	const char *const name = {
		"mmc-io-coherency",
	};

	np = of_get_child_by_name(host->dev->of_node, name);
	if (!np) {
		dev_err(host->dev, "failed to get node(%s)\n", name);
		goto out;
	}
	ret = of_property_read_u32(np, "offset", &cxt->offset);
	if (ret) {
		dev_err(host->dev, "failed to set cxt(%s) offset\n", name);
		goto out;
	}
	ret = of_property_read_u32(np, "mask", &cxt->mask);
	if (ret) {
		dev_err(host->dev, "failed to set cxt(%s) mask\n", name);
		goto out;
	}
	ret = of_property_read_u32(np, "val", &cxt->val);
	if (ret) {
		dev_err(host->dev, "failed to set cxt(%s) val\n", name);
		goto out;
	}

out:
	return ret;

}

static int dw_mci_exynos_iocc_enable(struct dw_mci *host)
{
	struct device *dev = host->dev;
	int ret = 0;
	u32 is_io_coherency;

	is_io_coherency = !IS_ERR(host->sysreg);
	if (is_io_coherency)
		ret = regmap_update_bits(host->sysreg, host->cxt_coherency.offset,
				host->cxt_coherency.mask, host->cxt_coherency.val);
	else
		dev_err(dev, "Not configured to use IO coherency\n");

	return ret;
}

static int dw_mci_exynos_priv_init(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 temp;
	int ret = 0;

	priv->saved_strobe_ctrl = mci_readl(host, HS400_DLINE_CTRL);
	priv->saved_dqs_en = mci_readl(host, HS400_RCLK_EN);
	priv->saved_dqs_en |= AXI_NON_BLOCKING_WR;
	mci_writel(host, HS400_RCLK_EN, priv->saved_dqs_en);
	if (!priv->dqs_delay)
		priv->dqs_delay = DQS_CTRL_GET_RD_DELAY(priv->saved_strobe_ctrl);
	temp = mci_readl(host, BLOCK_DMA_FOR_CI);
	temp &= ~SDMMC_DRTO_TIMER_EXTENSION_BIT;

	if (!host->extended_tmout)
		temp |= SDMMC_DRTO_TIMER_EXTENSION_BIT;
	mci_writel(host, BLOCK_DMA_FOR_CI, temp);

	if (dw_mci_exynos_iocc_parse_dt(host)) {
		dev_err(host->dev, "mmc iocc not enable\n");
	} else
		ret = dw_mci_exynos_iocc_enable(host);

#if IS_ENABLED(CONFIG_SEC_MMC_FEATURE)
	mmc_sd_sec_register_vendor_hooks();
#endif

	return ret;
}

static void dw_mci_exynos_ssclk_control(struct dw_mci *host, int enable)
{
	int err;

	if (!(host->pdata->quirks & DW_MCI_QUIRK_USE_SSC))
		return;

	if (enable) {
		if (cal_pll_mmc_check() == true)
			goto out;

		if (host->pdata->ssc_rate_mrr > SSC_MRR_LIMIT ||
			host->pdata->ssc_rate_mfr > SSC_MFR_LIMIT) {
			dev_info(host->dev, "unvalid SSC rate value\n");
			goto out;
		}

		err = cal_pll_mmc_set_ssc(host->pdata->ssc_rate_mfr, host->pdata->ssc_rate_mrr, 1);
		if (err)
			dev_info(host->dev, "SSC enable fail.\n");
		else
			dev_info(host->dev, "SSC set enable.\n");
	} else {
		if (cal_pll_mmc_check() == false)
			goto out;

		err = cal_pll_mmc_set_ssc(0, 0, 0);
		if (err)
			dev_info(host->dev, "SSC disable fail.\n");
		else
			dev_info(host->dev, "SSC set disable.\n");
	}
out:
	return;
}

static void dw_mci_exynos_set_clksel_timing(struct dw_mci *host, u32 timing)
{
	u32 clksel;

	clksel = mci_readl(host, CLKSEL);
	clksel = (clksel & ~SDMMC_CLKSEL_TIMING_MASK) | timing;

	if (host->pdata->io_mode != MMC_TIMING_MMC_HS400)
		clksel &= ~SDMMC_CLKSEL_ULP_ENABLE;

	mci_writel(host, CLKSEL, clksel);
}

#ifdef CONFIG_PM
static int dw_mci_exynos_runtime_resume(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	if (dw_mci_exynos_iocc_enable(host))
		dev_err(host->dev,"mmc io coherency enable fail!\n");

	return dw_mci_runtime_resume(dev);
}
#endif /* CONFIG_PM */

#ifdef CONFIG_PM_SLEEP
#if 0
/**
 * dw_mci_exynos_suspend_noirq - Exynos-specific suspend code
 *
 * This ensures that device will be in runtime active state in
 * dw_mci_exynos_resume_noirq after calling pm_runtime_force_resume()
 */
static int dw_mci_exynos_suspend_noirq(struct device *dev)
{
	pm_runtime_get_noresume(dev);
	return pm_runtime_force_suspend(dev);
}

/**
 * dw_mci_exynos_resume_noirq - Exynos-specific resume code
 *
 * On exynos5420 there is a silicon errata that will sometimes leave the
 * WAKEUP_INT bit in the CLKSEL register asserted.  This bit is 1 to indicate
 * that it fired and we can clear it by writing a 1 back.  Clear it to prevent
 * interrupts from going off constantly.
 *
 * We run this code on all exynos variants because it doesn't hurt.
 */

static int dw_mci_exynos_resume_noirq(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	u32 clksel;

	clksel = mci_readl(host, CLKSEL);

	if (clksel & SDMMC_CLKSEL_WAKEUP_INT)
		mci_writel(host, CLKSEL, clksel);

	return 0;
}
#endif
#endif /* CONFIG_PM_SLEEP */

static void dw_mci_card_int_hwacg_ctrl(struct dw_mci *host, u32 flag)
{
	u32 reg;

	reg = mci_readl(host, FORCE_CLK_STOP);
	if (flag == HWACG_Q_ACTIVE_EN) {
		reg |= MMC_HWACG_CONTROL;
		host->qactive_check = HWACG_Q_ACTIVE_EN;
	} else {
		reg &= ~(MMC_HWACG_CONTROL);
		host->qactive_check = HWACG_Q_ACTIVE_DIS;
	}
	mci_writel(host, FORCE_CLK_STOP, reg);
}

static void dw_mci_exynos_config_hs400(struct dw_mci *host, u32 timing)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 dqs, strobe;

	/*
	 * Not supported to configure register
	 * related to HS400
	 */

	dqs = priv->saved_dqs_en;
	strobe = priv->saved_strobe_ctrl;

	if (timing == MMC_TIMING_MMC_HS400) {
		dqs &= ~(DWMCI_TXDT_CRC_TIMER_SET(0xFF, 0xFF));
		dqs |= (DWMCI_TXDT_CRC_TIMER_SET(priv->hs400_tx_t_fastlimit,
						 priv->hs400_tx_t_initval) | DWMCI_RDDQS_EN |
			DWMCI_AXI_NON_BLOCKING_WRITE);
		if (host->pdata->quirks & DW_MCI_QUIRK_ENABLE_ULP) {
			if (priv->delay_line || priv->tx_delay_line)
				strobe = DWMCI_WD_DQS_DELAY_CTRL(priv->tx_delay_line) |
				    DWMCI_FIFO_CLK_DELAY_CTRL(0x2) |
				    DWMCI_RD_DQS_DELAY_CTRL(priv->delay_line);
			else
				strobe = DWMCI_FIFO_CLK_DELAY_CTRL(0x2) |
				    DWMCI_RD_DQS_DELAY_CTRL(90);
		} else {
			if (priv->delay_line)
				strobe = DWMCI_FIFO_CLK_DELAY_CTRL(0x2) |
				    DWMCI_RD_DQS_DELAY_CTRL(priv->delay_line);
			else
				strobe = DWMCI_FIFO_CLK_DELAY_CTRL(0x2) |
				    DWMCI_RD_DQS_DELAY_CTRL(90);
		}
		dqs |= (DATA_STROBE_EN | DWMCI_AXI_NON_BLOCKING_WRITE);
	} else {
		dqs &= ~DATA_STROBE_EN;
	}

	mci_writel(host, HS400_RCLK_EN, dqs);
	mci_writel(host, HS400_DLINE_CTRL, strobe);
}

static void dw_mci_exynos_adjust_clock(struct dw_mci *host, u32 wanted)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	unsigned long actual;
	u8 div;
	int ret;
	u32 clock;

	/*
	 * Don't care if wanted clock is zero or
	 * ciu clock is unavailable
	 */
	if (!wanted || IS_ERR(host->ciu_clk))
		return;

	/* Guaranteed minimum frequency for cclkin */
	if (wanted < EXYNOS_CCLKIN_MIN)
		wanted = EXYNOS_CCLKIN_MIN;

	div = dw_mci_exynos_get_ciu_div(host);

	if (wanted == priv->cur_speed) {
		clock = clk_get_rate(host->ciu_clk);
		if (clock == priv->cur_speed * div)
			return;
	}

	ret = clk_set_rate(host->ciu_clk, wanted * div);
	if (ret)
		dev_warn(host->dev, "failed to set clk-rate %u error: %d\n", wanted * div, ret);
	actual = clk_get_rate(host->ciu_clk);
	host->bus_hz = actual / div;
	priv->cur_speed = wanted;
	host->current_speed = 0;
}

#ifndef MHZ
#define MHZ (1000 * 1000)
#endif
#ifndef KHZ
#define KHZ (1000)
#endif

static void dw_mci_exynos_set_ios(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 wanted = ios->clock;
	u32 *clk_tbl = priv->ref_clk;
	u32 timing = ios->timing, clksel;
	u32 cclkin;
	u32 reg;

	cclkin = clk_tbl[timing];
	host->pdata->io_mode = timing;
	if (host->bus_hz != cclkin)
		wanted = cclkin;

	switch (timing) {
	case MMC_TIMING_MMC_HS400:
		if (host->pdata->quirks & DW_MCI_QUIRK_ENABLE_ULP) {
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->hs400_ulp_timing, priv->tuned_sample);
			clksel |= SDMMC_CLKSEL_ULP_ENABLE;	/* ultra low powermode on */
		} else {
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->hs400_timing, priv->tuned_sample);
			clksel &= ~SDMMC_CLKSEL_ULP_ENABLE;	/* ultra low powermode on */
			wanted <<= 1;
		}
		if (host->pdata->is_fine_tuned)
			clksel |= BIT(6);
		break;
	case MMC_TIMING_MMC_DDR52:
	case MMC_TIMING_UHS_DDR50:
		clksel = priv->ddr_timing;
		/* Should be double rate for DDR mode */
		if (ios->bus_width == MMC_BUS_WIDTH_8)
			wanted <<= 1;
		break;
	case MMC_TIMING_MMC_HS200:
		clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->hs200_timing, priv->tuned_sample);
		break;
	case MMC_TIMING_UHS_SDR104:
		if (priv->sdr104_timing)
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->sdr104_timing, priv->tuned_sample);
		else {
			dev_info(host->dev, "No SDR104 CLKSEL value in dt. replace with default value\n");
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->sdr_timing, priv->tuned_sample);
		}

		if (priv->use_dline) {
			reg = mci_readl(host, HS400_DLINE_CTRL);
			reg &= ~SDMMC_RCLK_DELAY_CTRL_MASK;
			reg |= (priv->sdr104_dline << 2);
			mci_writel(host, HS400_DLINE_CTRL, reg);
		}

		break;
	case MMC_TIMING_UHS_SDR50:
		if (priv->sdr50_timing)
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->sdr50_timing, priv->tuned_sample);
		else {
			dev_info(host->dev, "No SDR50 CLKSEL value in dt. replace with default value\n");
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->sdr_timing, priv->tuned_sample);
		}

		if (priv->use_dline) {
			reg = mci_readl(host, HS400_DLINE_CTRL);
			reg &= ~SDMMC_RCLK_DELAY_CTRL_MASK;
			reg |= (priv->sdr50_dline << 2);
			mci_writel(host, HS400_DLINE_CTRL, reg);
		}

		break;
	case MMC_TIMING_SD_HS:
		if (priv->hs_timing)
			clksel = priv->hs_timing;
		else {
			dev_info(host->dev, "No HS CLKSEL value in dt. replace with default value\n");
			clksel = priv->sdr_timing;
		}
		break;
	default:
		if ((ios->clock > 0) && (ios->clock <= 400 * KHZ)) clksel = priv->ls_timing;
		else if (ios->clock <= 25 * MHZ) clksel = priv->ds_timing;
		else clksel = priv->sdr_timing;
	}

	if (host->pdata->quirks & DW_MCI_QUIRK_USE_SSC) {
		if ((ios->clock > 0) && (ios->clock < 100 * MHZ))
			dw_mci_exynos_ssclk_control(host, 0);
		else if (ios->clock)
			dw_mci_exynos_ssclk_control(host, 1);
	}

	host->cclk_in = wanted;

	/* Set clock timing for the requested speed mode */
	dw_mci_exynos_set_clksel_timing(host, clksel);

	/* Configure setting for HS400 */
	dw_mci_exynos_config_hs400(host, timing);

	/* Configure clock rate */
	dw_mci_exynos_adjust_clock(host, wanted);
}

static int dw_mci_exynos_parse_dt(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv;
	struct device_node *np = host->dev->of_node;
	u32 timing[4];
	u32 div = 0;
	int idx;
	u32 ref_clk_size;
	u32 *ref_clk;
	u32 *ciu_clkin_values = NULL;
	int idx_ref;
	int ret = 0;
	int id = 0, i;

	priv = devm_kzalloc(host->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(host->dev, "mem alloc failed for private data\n");
		return -ENOMEM;
	}

	for (idx = 0; idx < ARRAY_SIZE(exynos_compat); idx++) {
		if (of_device_is_compatible(np, exynos_compat[idx].compatible))
			priv->ctrl_type = exynos_compat[idx].ctrl_type;
	}

	if (of_property_read_u32(np, "num-ref-clks", &ref_clk_size)) {
		dev_err(host->dev, "Getting a number of referece clock failed\n");
		ret = -ENODEV;
		goto err_ref_clk;
	}

	ref_clk = devm_kzalloc(host->dev, ref_clk_size * sizeof(*ref_clk), GFP_KERNEL);
	if (!ref_clk) {
		dev_err(host->dev, "Mem alloc failed for reference clock table\n");
		ret = -ENOMEM;
		goto err_ref_clk;
	}

	ciu_clkin_values = devm_kzalloc(host->dev,
					ref_clk_size * sizeof(*ciu_clkin_values), GFP_KERNEL);

	if (!ciu_clkin_values) {
		dev_err(host->dev, "Mem alloc failed for temporary clock values\n");
		ret = -ENOMEM;
		goto err_ref_clk;
	}
	if (of_property_read_u32_array(np, "ciu_clkin", ciu_clkin_values, ref_clk_size)) {
		dev_err(host->dev, "Getting ciu_clkin values faild\n");
		ret = -ENOMEM;
		goto err_ref_clk;
	}

	for (idx_ref = 0; idx_ref < ref_clk_size; idx_ref++, ref_clk++, ciu_clkin_values++) {
		if (*ciu_clkin_values > MHZ)
			*(ref_clk) = (*ciu_clkin_values);
		else
			*(ref_clk) = (*ciu_clkin_values) * MHZ;
	}

	ref_clk -= ref_clk_size;
	ciu_clkin_values -= ref_clk_size;
	priv->ref_clk = ref_clk;

	if (of_get_property(np, "card-detect", NULL))
		priv->cd_gpio = of_get_named_gpio(np, "card-detect", 0);
	else
		priv->cd_gpio = -1;

	if (of_find_property(np, "use-dline", NULL)) {
		priv->use_dline = true;
		if (!of_property_read_u32(np, "sdr50-dline", &priv->sdr50_dline)) priv->sdr50_dline = 0x60;
		if (!of_property_read_u32(np, "sdr104-dline", &priv->sdr50_dline)) priv->sdr104_dline = 0x2A;
	} else {
		priv->use_dline = false;
	}

	priv->pinctrl = devm_pinctrl_get(host->dev);

	if (IS_ERR(priv->pinctrl)) {
		priv->pinctrl = NULL;
	} else {
		priv->clk_drive_base = pinctrl_lookup_state(priv->pinctrl, "default");

		priv->pins_config[0] = pinctrl_lookup_state(priv->pinctrl, "pins-as-pdn");
		priv->pins_config[1] = pinctrl_lookup_state(priv->pinctrl, "pins-as-func");

		for (i = 0; i < 2; i++) {
			if (IS_ERR(priv->pins_config[i]))
				priv->pins_config[i] = NULL;
		}
	}

	of_property_read_u32(np, "samsung,dw-mshc-ciu-div", &div);
	priv->ciu_div = div;

	ret = of_property_read_u32_array(np, "samsung,dw-mshc-sdr-timing", timing, 4);
	if (ret)
		return ret;

	priv->sdr_timing = SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

	ret = of_property_read_u32_array(np, "samsung,dw-mshc-ddr-timing", timing, 4);
	if (ret)
		return ret;

	priv->ddr_timing = SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

	ret = of_property_read_u32_array(np, "samsung,dw-mshc-ls-timing", timing, 4);
	if (ret)
		return ret;

	priv->ls_timing = SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

	ret = of_property_read_u32_array(np, "samsung,dw-mshc-ds-timing", timing, 4);
	if (ret)
		return ret;

	priv->ds_timing = SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

	if (of_property_read_u32(np, "ignore-phase", &priv->ignore_phase))
		priv->ignore_phase = 0;

	if (of_find_property(np, "extended_tmout", NULL))
		host->extended_tmout = true;
	else
		host->extended_tmout = false;

	host->sysreg = syscon_regmap_lookup_by_phandle(np, "samsung,sysreg-phandle");

	if (IS_ERR(host->sysreg)) {
		/*
		 * Currently, ufs driver gets sysreg for io coherency.
		 * Some architecture might not support this feature.
		 * So the device node might not exist.
		 */
		dev_err(host->dev, "sysreg regmap lookup failed.\n");
	}

	id = of_alias_get_id(host->dev->of_node, "mshc");
	switch (id) {
	case 0:
		/* dwmmc0 : eMMC    */
		ret = of_property_read_u32_array(np, "samsung,dw-mshc-hs200-timing", timing, 4);
		if (ret)
			goto err_ref_clk;
		priv->hs200_timing =
		    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

		ret = of_property_read_u32_array(np, "samsung,dw-mshc-hs400-timing", timing, 4);
		if (ret)
			goto err_ref_clk;

		priv->hs400_timing =
		    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

		ret = of_property_read_u32_array(np, "samsung,dw-mshc-hs400-ulp-timing", timing, 4);
		if (!ret)
			priv->hs400_ulp_timing =
			    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);
		else
			ret = 0;

		/* Rx Delay Line */
		of_property_read_u32(np, "samsung,dw-mshc-hs400-delay-line", &priv->delay_line);

		/* Tx Delay Line */
		of_property_read_u32(np,
				     "samsung,dw-mshc-hs400-tx-delay-line", &priv->tx_delay_line);

		/* The fast RXCRC packet arrival time */
		of_property_read_u32(np,
				     "samsung,dw-mshc-txdt-crc-timer-fastlimit",
				     &priv->hs400_tx_t_fastlimit);

		/* Initial value of the timeout down counter for RXCRC packet */
		of_property_read_u32(np,
				     "samsung,dw-mshc-txdt-crc-timer-initval",
				     &priv->hs400_tx_t_initval);
		break;
	case 1:
		/* dwmmc1 : SDIO    */
	case 2:
		/* dwmmc2 : SD Card */
		ret = of_property_read_u32_array(np, "samsung,dw-mshc-hs-timing", timing, 4);	/* HS 50Mhz */
		if (!ret)
			priv->hs_timing =
			    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);
		else {
			priv->hs_timing = priv->sdr_timing;
			ret = 0;
		}

		ret = of_property_read_u32_array(np, "samsung,dw-mshc-sdr50-timing", timing, 4);	/* SDR50 100Mhz */
		if (!ret)
			priv->sdr50_timing =
			    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);
		else {
			priv->sdr50_timing = priv->sdr_timing;
			ret = 0;
		}

		ret = of_property_read_u32_array(np, "samsung,dw-mshc-sdr104-timing", timing, 4);	/* SDR104 200mhz */
		if (!ret)
			priv->sdr104_timing =
			    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);
		else {
			priv->sdr104_timing = priv->sdr_timing;
			ret = 0;
		}
		break;
	default:
		ret = -ENODEV;
	}
	host->priv = priv;
 err_ref_clk:
	return ret;
}

static inline u8 dw_mci_exynos_get_clksmpl(struct dw_mci *host)
{
	return SDMMC_CLKSEL_CCLK_SAMPLE(mci_readl(host, CLKSEL));
}

static inline void dw_mci_exynos_set_clksmpl(struct dw_mci *host, u8 sample)
{
	u32 clksel;

	clksel = mci_readl(host, CLKSEL);
	clksel = SDMMC_CLKSEL_UP_SAMPLE(clksel, sample);
	mci_writel(host, CLKSEL, clksel);
}

static inline u8 dw_mci_exynos_move_next_clksmpl(struct dw_mci *host)
{
	u32 clksel;
	u8 sample;

	clksel = mci_readl(host, CLKSEL);
	sample = (clksel + 1) & 0x7;
	clksel = (clksel & ~0x7) | sample;
	mci_writel(host, CLKSEL, clksel);
	return sample;
}

/* initialize the clock sample to given value */
static void dw_mci_exynos_set_sample(struct dw_mci *host, u32 sample, u32 fine_bit)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 clksel;

	clksel = mci_readl(host, CLKSEL);
	clksel = (clksel & ~0x7) | SDMMC_CLKSEL_CCLK_SAMPLE(sample);

	/*
	 * fine_bit control
	 * CLKSEL[6] = 1 : use fine bit
	 * CLKSEL[6] = 0 : not use fine bit
	 * dline control
	 * CLKSEL[21] = 1 : bypass
	 * CLKSEL[21] = 0 : use DLINE
	 */

	if (priv->use_dline) {
		clksel &= ~SDMMC_CLKSEL_DLINE_BYPASS;
		if (!fine_bit) clksel |= SDMMC_CLKSEL_DLINE_BYPASS;
	} else {
		clksel &= ~SDMMC_CLKSEL_FINE_BIT;
		if (fine_bit) clksel |= SDMMC_CLKSEL_FINE_BIT;
	}

	mci_writel(host, CLKSEL, clksel);

	if (sample == 6 || sample == 7)
		sample_path_sel_en(host, AXI_BURST_LEN);
	else
		sample_path_sel_dis(host, AXI_BURST_LEN);
}

static void dw_mci_set_pins_state(struct dw_mci *host, int config)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (priv->pinctrl && priv->pins_config[config])
		pinctrl_select_state(priv->pinctrl, priv->pins_config[config]);
}

static int dw_mci_exynos_send_test_data(struct dw_mci_slot *slot, u32 tune_type, u32 opcode, u32 *pass_length)
{
	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_command stop;
	struct mmc_data data;
	struct scatterlist sg;
	struct mmc_host *mmc = slot->mmc;
	struct dw_mci *host = slot->host;
	struct dw_mci_tuning_data* tuning_data = host->tuning_data;
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 clksel;
	u8 *tuning_blk;		/* data read from device */
	int ret = 0;
	u32 i;
	u32 tuning_map = 0;
	u32 num = 0;
	u32 pass_num = 0;

	tuning_blk = kmalloc(2 * tuning_data->blksz, GFP_KERNEL);
	if (!tuning_blk)
		return -ENOMEM;

	switch (tune_type) {
	case TUNE_TYPE_PHASE8 :
		num = NUM_PHASE8;
		break;
	case TUNE_TYPE_PHASE16 :
		num = NUM_PHASE16;
		break;
	case TUNE_TYPE_PHASEDETECT :
		num = NUM_PHASEDETECT;
		break;
	default :
		dev_err(host->dev, "tune type invalid");
		break;
	}

	/*
	 * case num 1 is Phase Detect
	 * In order to use phase detect, the following sequence should be performed.
	 * Tuning All pass => CLKSEL[28] set => send CMD => Read CLKSEL [5:3] for Phase value =>
	 * Set the value read from CLKSEL[5:3] to CLKSEL[2:0] => CLKSEL[28] clear
	 */
	if (tune_type == TUNE_TYPE_PHASEDETECT) {
		clksel = mci_readl(host, CLKSEL);
		clksel |= SDMMC_CLKSEL_HW_PHASE_EN;
		mci_writel(host, CLKSEL, clksel);
	}

	for (i = 0; i < num; i++) {
		if (tune_type == TUNE_TYPE_PHASE16) {
			if (!(priv->ignore_phase & (1 << i)))
				dw_mci_exynos_set_sample(host, i / 2, i % 2);
		} else
			dw_mci_exynos_set_sample(host, i, 0);

		memset(&cmd, 0, sizeof(cmd));
		cmd.opcode = opcode;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
		cmd.error = 0;
		cmd.busy_timeout = 10;	/* 2x * (150ms/40 + setup overhead) */

		memset(&stop, 0, sizeof(stop));
		stop.opcode = MMC_STOP_TRANSMISSION;
		stop.arg = 0;
		stop.flags = MMC_RSP_R1B | MMC_CMD_AC;
		stop.error = 0;

		memset(&data, 0, sizeof(data));
		data.blksz = tuning_data->blksz;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.sg = &sg;
		data.sg_len = 1;
		data.error = 0;

		memset(tuning_blk, ~0U, tuning_data->blksz);
		sg_init_one(&sg, tuning_blk, tuning_data->blksz);

		memset(&mrq, 0, sizeof(mrq));
		mrq.cmd = &cmd;
		mrq.stop = &stop;
		mrq.data = &data;
		host->mrq = &mrq;

		mmc_wait_for_req(mmc, &mrq);
		if (!cmd.error && !data.error) {
			tuning_map |= (1 << i);
			pass_num++;
			if (tune_type == TUNE_TYPE_PHASEDETECT)
				break;
		}
	}

	if (tune_type == TUNE_TYPE_PHASEDETECT) {
		clksel = mci_readl(host, CLKSEL);
		if (pass_num == 0) {
			dev_err(host->dev, "send phase detect cmd error all times\n");
			ret = -EINVAL;
			clksel &= ~SDMMC_CLKSEL_HW_PHASE_EN;
			mci_writel(host, CLKSEL, clksel);
			goto out;
		} else {
			pass_num = 1;
			tuning_map = (1 << SDMMC_CLKSEL_GET_PHASE_DETECT_SAMPLE(clksel));
			clksel &= ~SDMMC_CLKSEL_HW_PHASE_EN;
			mci_writel(host, CLKSEL, clksel);
		}
	}

	ret = tuning_map;
	*pass_length = pass_num;
	dev_info(host->dev, "mmc tuning, tuning_map : %08x, pass_length : %d\n", tuning_map, pass_num);

out:
	kfree(tuning_blk);

	return ret;
}

static int dw_mci_exynos_check_continuity(u32 tuning_map, u32 num, u32 pass_length)
{
	u32 start;
	u32 first_continuity_length;

	/*
	 * For continuity check, check the difference between the total number
	 * of passes and the number of first consecutive set bits.
	 * All pass and zero pass cases are guaranteed continuity.
	 */
	if (pass_length == 0 || pass_length == num)
		return 0;

	if (tuning_map & 0x1) {
		tuning_map |= tuning_map << num;
		tuning_map = tuning_map >> ffz(tuning_map);
	}

	start = ffs(tuning_map) - 1;
	first_continuity_length = ffz(tuning_map >> start);

	if (first_continuity_length != pass_length)
		return -EINVAL;

	return 0;
}

static int dw_mci_exynos_find_median(u32 tuning_map, u32 num, u32 pass_length)
{
	u32 start = 0;
	u32 median;

	/*
	 * For median calculation, the following logic is applied.
	 * Since the tuning map is a circular bitmap, bit 0 set means that is circular.
	 * Therefore, after pasting, the preceding set bits are ignored.
	 */
	if (tuning_map & 0x1) {
		tuning_map |= tuning_map << num;
		start = ffz(tuning_map);
		tuning_map = tuning_map >> start;
	}

	median = ffs(tuning_map) - 1 + (pass_length / 2) + start;
	median = median % num;

	return median;
}

/*
 * Test all 8 possible "Clock in" Sample timings.
 * Create a bitmap of which CLock sample values work and find the "median"
 * value. Apply it and remember that we found the best value.
 */
static int dw_mci_exynos_execute_tuning(struct dw_mci_slot *slot, u32 opcode,
					struct dw_mci_tuning_data *tuning_data)
{
	struct dw_mci *host = slot->host;
	int ret = 0;
	u32 tuning_map;
	u32 pass_length;
	u32 sample;

	if (!tuning_data || !opcode) {
		ret = -EINVAL;
		goto out;
	}
	host->cd_rd_thr = 512;
	mci_writel(host, CDTHRCTL, host->cd_rd_thr << 16 | 1);

	host->tuning_data = tuning_data;
	if (host->pdata->io_mode == MMC_TIMING_MMC_HS400)
		host->quirks |= DW_MCI_QUIRK_NO_DETECT_EBIT;

	tuning_map = dw_mci_exynos_send_test_data(slot, TUNE_TYPE_PHASE8, opcode, &pass_length);
	ret = dw_mci_exynos_check_continuity(tuning_map, NUM_PHASE8, pass_length);
	if (ret) {
		dev_err(host->dev, "during mmc normal tune, tuning map isn't continuous\n");
		goto out;
	}

	if (pass_length == NUM_PHASE8) {
		/* In all pass case, use phase detect */
		tuning_map = dw_mci_exynos_send_test_data(slot, TUNE_TYPE_PHASEDETECT, opcode, &pass_length);
		if (tuning_map < 0) {
			dev_err(host->dev, "mmc phase detect failed\n");
			ret = -EINVAL;
			goto out;
		}
		sample = dw_mci_exynos_find_median(tuning_map, NUM_PHASE8, pass_length);
		dev_info(host->dev, "mmc phase_detect, median : %d\n", sample);
	} else if (pass_length < NUM_MIN_PASS_PHASE) {
		/* In narrow pass case, use fine tune */
		tuning_map = dw_mci_exynos_send_test_data(slot, TUNE_TYPE_PHASE16, opcode, &pass_length);
		ret = dw_mci_exynos_check_continuity(tuning_map, NUM_PHASE16, pass_length);
		if (ret) {
			dev_err(host->dev, "during mmc fine tune, tuning map isn't continuous\n");
			goto out;
		}
		sample = dw_mci_exynos_find_median(tuning_map, NUM_PHASE16, pass_length);
		dev_info(host->dev, "mmc fine tune, median : %d\n", sample);
		if (pass_length < NUM_MIN_PASS_PHASE) {
			dev_err(host->dev, "mmc fine tune failed\n");
			ret = -EINVAL;
			goto out;
		}
		/* In the 16-phase case, the sample needs to be divided by 2 for ignore fine tune bit */
		sample /= 2;
	} else {
		sample = dw_mci_exynos_find_median(tuning_map, NUM_PHASE8, pass_length);
		dev_info(host->dev, "mmc normal tune, median : %d\n", sample);
	}

	dw_mci_exynos_set_sample(host, sample, 0);

	if (host->pdata->io_mode == MMC_TIMING_MMC_HS400)
		host->quirks &= ~DW_MCI_QUIRK_NO_DETECT_EBIT;

	dev_info(host->dev, "CLKSEL = 0x%08x, AXI_BURST_LEN = 0x%08x\n",
		 mci_readl(host, CLKSEL), mci_readl(host, AXI_BURST_LEN));

out:
	return ret;
}

static int dw_mci_exynos_request_ext_irq(struct dw_mci *host, irq_handler_t func)
{
#if !defined(CONFIG_MMC_DW_SD_BROKEN_DETECT)
	struct dw_mci_exynos_priv_data *priv = host->priv;
	int ext_cd_irq = 0;

	if (gpio_is_valid(priv->cd_gpio) && !gpio_request(priv->cd_gpio, "DWMCI_EXT_CD")) {
		ext_cd_irq = gpio_to_irq(priv->cd_gpio);
		if (ext_cd_irq &&
		    devm_request_irq(host->dev, ext_cd_irq, func,
				     IRQF_TRIGGER_RISING |
				     IRQF_TRIGGER_FALLING |
				     IRQF_ONESHOT, "tflash_det", host) == 0) {
			dev_info(host->dev, "success to request irq for card detect.\n");
			enable_irq_wake(ext_cd_irq);
		} else
			dev_info(host->dev, "cannot request irq for card detect.\n");
	}
#endif
	return 0;
}

static int dw_mci_exynos_check_cd(struct dw_mci *host)
{
	int ret = -1;
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (gpio_is_valid(priv->cd_gpio)) {
		if (host->pdata->use_gpio_invert)
			ret = gpio_get_value(priv->cd_gpio) ? 1 : 0;
		else
			ret = gpio_get_value(priv->cd_gpio) ? 0 : 1;
	}
	return ret;
}

/* Common capabilities of Exynos4/Exynos5 SoC */
static unsigned long exynos_dwmmc_caps[4] = {
	MMC_CAP_1_8V_DDR | MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23,
	MMC_CAP_CMD23,
	MMC_CAP_CMD23,
	MMC_CAP_CMD23,
};

static int dw_mci_exynos_misc_control(struct dw_mci *host,
				      enum dw_mci_misc_control control, void *priv)
{
	int ret = 0;

	switch (control) {
	case CTRL_RESTORE_CLKSEL:
		dw_mci_exynos_set_sample(host, host->pdata->clk_smpl, 0);
		break;
	case CTRL_REQUEST_EXT_IRQ:
		ret = dw_mci_exynos_request_ext_irq(host, (irq_handler_t) priv);
		break;
	case CTRL_CHECK_CD:
		ret = dw_mci_exynos_check_cd(host);
		break;
	default:
		dev_err(host->dev, "dw_mmc exynos: wrong case\n");
		ret = -ENODEV;
	}
	return ret;
}

#if IS_ENABLED(CONFIG_SEC_MMC_FEATURE)
static void dw_mci_exynos_detect_interrupt(struct dw_mci *host)
{
	sd_sec_detect_interrupt(host);
}

static void dw_mci_exynos_check_req_err(struct dw_mci *host,
		struct mmc_request *mrq)
{
	sd_sec_check_req_err(host, mrq);
}
#endif

#ifdef CONFIG_MMC_DW_EXYNOS_FMP
static struct bio *get_bio(struct dw_mci *host,
				struct mmc_data *data, bool cmdq_enabled)
{
	struct bio *bio = NULL;
	struct dw_mci_exynos_priv_data *priv;

	if (!host || !data) {
		pr_err("%s: Invalid MMC:%p data:%p\n", __func__, host, data);
		return NULL;
	}

	priv = host->priv;
	if (priv->fmp == SMU_ID_MAX)
		return NULL;

	if (cmdq_enabled) {
		pr_err("%s: no support cmdq yet:%p\n", __func__, host, data);
		bio = NULL;
	} else {
		struct mmc_queue_req *mq_rq;
		struct request *req;
		struct mmc_blk_request *brq;

		brq = container_of(data, struct mmc_blk_request, data);
		if (!brq)
			return NULL;

		mq_rq = container_of(brq, struct mmc_queue_req, brq);
		if (virt_addr_valid(mq_rq))
			req = mmc_queue_req_to_req(mq_rq);
			if (virt_addr_valid(req))
				bio = req->bio;
	}

	return bio;
}

static int dw_mci_exynos_crypto_engine_cfg(struct dw_mci *host,
					void *desc, struct mmc_data *data,
					struct page *page, int page_index,
					int sector_offset, bool cmdq_enabled)
{
	struct bio *bio = get_bio(host, host->data, cmdq_enabled);

	if (!bio)
		return 0;

	return exynos_fmp_crypt_cfg(bio, desc, page_index, sector_offset);
}

static int dw_mci_exynos_crypto_engine_clear(struct dw_mci *host,
					void *desc, bool cmdq_enabled)
{
	struct bio *bio = get_bio(host, host->data, cmdq_enabled);

	if (!bio)
		return 0;

	return exynos_fmp_crypt_clear(bio, desc);
}

static int dw_mci_exynos_crypto_sec_cfg(struct dw_mci *host, bool init)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (!priv)
		return -EINVAL;

	return exynos_fmp_sec_cfg(priv->fmp, priv->smu, init);
}

static int dw_mci_exynos_access_control_abort(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (!priv)
		return -EINVAL;

	return exynos_fmp_smu_abort(priv->smu);
}
#endif

static const struct dw_mci_drv_data exynos_drv_data = {
	.caps = exynos_dwmmc_caps,
	.init = dw_mci_exynos_priv_init,
	.set_ios = dw_mci_exynos_set_ios,
	.parse_dt = dw_mci_exynos_parse_dt,
	.execute_tuning = dw_mci_exynos_execute_tuning,
	.hwacg_control = dw_mci_card_int_hwacg_ctrl,
	.misc_control = dw_mci_exynos_misc_control,
	.pins_control = dw_mci_set_pins_state,
#ifdef CONFIG_MMC_DW_EXYNOS_FMP
	.crypto_engine_cfg = dw_mci_exynos_crypto_engine_cfg,
	.crypto_engine_clear = dw_mci_exynos_crypto_engine_clear,
	.crypto_sec_cfg = dw_mci_exynos_crypto_sec_cfg,
	.access_control_abort = dw_mci_exynos_access_control_abort,
#endif
	.ssclk_control = dw_mci_exynos_ssclk_control,
	.dump_reg = dw_mci_reg_dump,
#if IS_ENABLED(CONFIG_SEC_MMC_FEATURE)
	.detect_interrupt = dw_mci_exynos_detect_interrupt,
	.check_request_error = dw_mci_exynos_check_req_err,
#endif
};

static const struct of_device_id dw_mci_exynos_match[] = {
	{.compatible = "samsung,exynos-dw-mshc",
	 .data = &exynos_drv_data,},
	{},
};

MODULE_DEVICE_TABLE(of, dw_mci_exynos_match);

static int mmc_memlog_file_completed(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int mmc_memlog_status_notify(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int mmc_memlog_level_notify(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int mmc_memlog_enable_notify(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static const struct memlog_ops mmc_memlog_ops = {
	.file_ops_completed = mmc_memlog_file_completed,
	.log_status_notify = mmc_memlog_status_notify,
	.log_level_notify = mmc_memlog_level_notify,
	.log_enable_notify = mmc_memlog_enable_notify,
};

int exynos_mmc_init_mem_log(struct platform_device *pdev)
{
	struct exynos_mmc_memlog *memlog = &mmc_memlog;
	struct memlog *desc;
	struct memlog_obj *log_obj;
	int ret;

	pr_info("%s: +++\n", __func__);

	ret = memlog_register("MMC", &pdev->dev, &desc);
	if (ret) {
		pr_err("failed to register memlog\n");
		return -1;
	}

	memlog->desc = desc;
	desc->ops = mmc_memlog_ops;

	log_obj = memlog_alloc_printf(desc,
					SZ_64K,
					NULL,
					"log-mem",
					0);

	if (log_obj) {
		memlog->log_obj = log_obj;
		memlog->log_enable = 1;
	} else {
		pr_err("%s: failed to alloc memlog memory for log\n",
			__func__);
		return -1;
	}
	pr_info("%s: ---\n", __func__);

	return 0;
}

static int dw_mci_exynos_probe(struct platform_device *pdev)
{
	const struct dw_mci_drv_data *drv_data;
	const struct of_device_id *match;
	int ret;

	exynos_mmc_init_mem_log(pdev);
	match = of_match_node(dw_mci_exynos_match, pdev->dev.of_node);
	drv_data = match->data;

	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	ret = dw_mci_pltfm_register(pdev, drv_data);
	if (ret) {
		pm_runtime_disable(&pdev->dev);
		pm_runtime_set_suspended(&pdev->dev);
		pm_runtime_put_noidle(&pdev->dev);

		return ret;
	}
#if IS_ENABLED(CONFIG_SEC_MMC_FEATURE)
	sd_sec_set_features(pdev);
#endif

	return 0;
}

static int dw_mci_exynos_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);

	return dw_mci_pltfm_remove(pdev);
}

static const struct dev_pm_ops dw_mci_exynos_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
			pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(dw_mci_runtime_suspend,
			dw_mci_exynos_runtime_resume, NULL)
};

static struct platform_driver dw_mci_exynos_pltfm_driver = {
	.probe		= dw_mci_exynos_probe,
	.remove		= dw_mci_exynos_remove,
	.driver		= {
		.name		= "dwmmc_exynos",
		.of_match_table	= dw_mci_exynos_match,
		.pm		= &dw_mci_exynos_pmops,
	},
};

module_platform_driver(dw_mci_exynos_pltfm_driver);

MODULE_DESCRIPTION("Samsung Specific DW-MSHC Driver Extension");
MODULE_AUTHOR("Thomas Abraham <thomas.ab@samsung.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:dwmmc_exynos");
