/****************************************************************************
 *
 *   Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved.
 *
 ****************************************************************************/

#include <pcie_scsc/scsc_logring.h>

#include "panicmon.h"
#include "scsc_mif_abs.h"
#include "mxman.h"

#ifdef CONFIG_WLBT_KUNIT
#include "./kunit/kunit_panicmon.c"
#endif

static void panicmon_isr(int irq, void *data)
{
	struct panicmon *panicmon = (struct panicmon *)data;

	SCSC_TAG_DEBUG(PANIC_MON, "panicmon=%p panicmon->mx=%p mxman=%p\n", panicmon, panicmon->mx, scsc_mx_get_mxman(panicmon->mx));
	/* Avoid unused parameter error */
	(void)irq;
#if IS_ENABLED(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	mxman_fail(scsc_mx_get_mxman(panicmon->mx), SCSC_PANIC_CODE_FW << 15, __func__, SCSC_SUBSYSTEM_PMU);
#else
	mxman_fail(scsc_mx_get_mxman(panicmon->mx), SCSC_PANIC_CODE_FW << 15, __func__);
#endif
}


void panicmon_init(struct panicmon *panicmon, struct scsc_mx *mx)
{
	struct scsc_mif_abs *mif;

	panicmon->mx = mx;
	mif = scsc_mx_get_mif_abs(mx);
	/* register isr with mif abstraction */
	mif->irq_reg_reset_request_handler(mif, panicmon_isr, (void *)panicmon);
}

void panicmon_deinit(struct panicmon *panicmon)
{
	struct scsc_mif_abs *mif;

	mif = scsc_mx_get_mif_abs(panicmon->mx);
	mif->irq_unreg_reset_request_handler(mif);
}
