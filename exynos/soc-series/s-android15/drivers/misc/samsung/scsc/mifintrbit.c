/****************************************************************************
 *
 * Copyright (c) 2014 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

/* Uses */
#include <linux/bitmap.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/bitmap.h>
#include <scsc/scsc_logring.h>
#include "scsc_mif_abs.h"

/* Implements */
#include "mifintrbit.h"

#ifdef CONFIG_WLBT_KUNIT
#include "./kunit/kunit_mifintrbit.c"
#endif

const char *wlbt_irq_types[48] = {
	"DEFAULT_IRQ_TYPE",
	"GDB_TRANSPORT_FXM_1_INPUT_TYPE",
	"GDB_TRANSPORT_FXM_1_OUTPUT_TYPE",
	"GDB_TRANSPORT_FXM_2_INPUT_TYPE",
	"GDB_TRANSPORT_FXM_2_OUTPUT_TYPE",
	"GDB_TRANSPORT_FXM_3_INPUT_TYPE",
	"GDB_TRANSPORT_FXM_3_OUTPUT_TYPE",
	"GDB_TRANSPORT_WPAN_INPUT_TYPE",
	"GDB_TRANSPORT_WPAN_OUTPUT_TYPE",
	"GDB_TRANSPORT_PMU_INPUT_TYPE",
	"GDB_TRANSPORT_PMU_OUTPUT_TYPE",
	"GDB_TRANSPORT_WLAN_INPUT_TYPE",
	"GDB_TRANSPORT_WLAN_OUTPUT_TYPE",
	"GDB_TRANSPORT_WLAN_2_INPUT_TYPE",
	"GDB_TRANSPORT_WLAN_2_OUTPUT_TYPE",
	"GDB_TRANSPORT_WLAN_3_INPUT_TYPE",
	"GDB_TRANSPORT_WLAN_3_OUTPUT_TYPE",
	"GDB_TRANSPORT_WLAN_4_INPUT_TYPE",
	"GDB_TRANSPORT_WLAN_4_OUTPUT_TYPE",
	"GDB_TRANSPORT_WLAN_5_INPUT_TYPE",
	"GDB_TRANSPORT_WLAN_5_OUTPUT_TYPE",
	"GDB_TRANSPORT_WLAN_6_INPUT_TYPE",
	"GDB_TRANSPORT_WLAN_6_OUTPUT_TYPE",
	"GDB_TRANSPORT_WLAN_7_INPUT_TYPE",
	"GDB_TRANSPORT_WLAN_7_OUTPUT_TYPE",
	"GDB_TRANSPORT_WLAN_8_INPUT_TYPE",
	"GDB_TRANSPORT_WLAN_8_OUTPUT_TYPE",
	"MXLOG_WLAN_TYPE",
	"MXLOG_WPAN_TYPE",
	"MXMGMT_WLAN_INPUT_TYPE",
	"MXMGMT_WLAN_OUTPUT_TYPE",
	"MXMGMT_WPAN_INPUT_TYPE",
	"MXMGMT_WPAN_OUTPUT_TYPE",
	"MX_DBG_SAMPLER_TYPE",
	"SCSC_ANT_SHM_IRQ_TYPE",
	"SCSC_BT_SHM_IRQ_TYPE",
	"HIP4_SMAPPER_REFILL_TYPE",
	"HIP4_IRQ_HANDLER_FB_TYPE",
	"HIP4_IRQ_HANDLER_CTRL_TYPE",
	"HIP4_IRQ_HANDLER_DATA_TYPE",
	"HIP4_IRQ_HANDLER_TYPE",
	"HIP4_IRQ_HANDLER_DPD_TYPE",
	"HIP5_IRQ_HANDLER_CTRL_TYPE",
	"HIP5_IRQ_HANDLER_FB_TYPE",
	"HIP5_IRQ_HANDLER_DAT_TYPE",
	"HIP5_IRQ_HANDLER_DPD_TYPE",
	"HIP5_IRQ_HANDLER_STUB_TYPE",
	"LAST_IRQ_TYPE"
};

/* default handler just logs a warning and clears the bit */
static void mifintrbit_default_handler(int irq, void *data)
{
	struct mifintrbit *intr = (struct mifintrbit *)data;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	intr->mif->irq_bit_clear(intr->mif, irq, intr->target);
#else
	intr->mif->irq_bit_clear(intr->mif, irq);
#endif
}

static void print_bitmaps(struct mifintrbit *intr)
{

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	unsigned long dst1, dst2;

	bitmap_copy_le(&dst1, intr->bitmap_tohost, MIFINTRBIT_NUM_INT);
	bitmap_copy_le(&dst2, intr->bitmap_fromhost, MIFINTRBIT_NUM_INT);
#else
	unsigned long dst1, dst2, dst3;

	bitmap_copy_le(&dst1, intr->bitmap_tohost, MIFINTRBIT_NUM_INT);
	bitmap_copy_le(&dst2, intr->bitmap_fromhost_wlan, MIFINTRBIT_NUM_INT);
	bitmap_copy_le(&dst3, intr->bitmap_fromhost_fxm_1, MIFINTRBIT_NUM_INT);
#endif
}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
static void mifiintrman_isr_wpan(int irq, void *data)
{
	struct mifintrbit *intr = (struct mifintrbit *)data;
	unsigned long     flags;
	unsigned long int irq_reg = 0;
	int               bit;

	/* Avoid unused parameter error */
	(void)irq;

	spin_lock_irqsave(&intr->spinlock, flags);
	irq_reg = intr->mif->irq_get(intr->mif, SCSC_MIF_ABS_TARGET_WPAN);

	print_bitmaps(intr);
	for_each_set_bit(bit, &irq_reg, MIFINTRBIT_NUM_INT) {
		if (intr->mifintrbit_irq_handler[bit] != mifintrbit_default_handler)
			intr->mifintrbit_irq_handler[bit](bit, intr->irq_data[bit]);
		else
			intr->mif->irq_bit_clear(intr->mif, bit, SCSC_MIF_ABS_TARGET_WPAN);
	}

	spin_unlock_irqrestore(&intr->spinlock, flags);
}
#endif

static void mifiintrman_isr(int irq, void *data)
{
	struct mifintrbit *intr = (struct mifintrbit *)data;
	unsigned long     flags;
	unsigned long int irq_reg = 0;
	int               bit;

	/* Avoid unused parameter error */
	(void)irq;

	spin_lock_irqsave(&intr->spinlock, flags);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	irq_reg = intr->mif->irq_get(intr->mif, SCSC_MIF_ABS_TARGET_WLAN);
#else
	irq_reg = intr->mif->irq_get(intr->mif);
#endif

	print_bitmaps(intr);
	for_each_set_bit(bit, &irq_reg, MIFINTRBIT_NUM_INT) {
		if (intr->mifintrbit_irq_handler[bit] != mifintrbit_default_handler)
			intr->mifintrbit_irq_handler[bit](bit, intr->irq_data[bit]);
		else
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
			intr->mif->irq_bit_clear(intr->mif, bit, SCSC_MIF_ABS_TARGET_WLAN);
#else
			intr->mif->irq_bit_clear(intr->mif, bit);
#endif
	}

	spin_unlock_irqrestore(&intr->spinlock, flags);
}

#if defined(CONFIG_SCSC_BB_REDWOOD)
static inline int assign_prealloc_bit(struct mifintrbit *intr,  enum IRQ_TYPE irq_type)
{
	int prealloc_bit = 0;
	/* preallocate TH irq for REDWOOD where we are running out of IRQs */
	switch (irq_type) {
	case GDB_TRANSPORT_FXM_1_INPUT_TYPE:
	case GDB_TRANSPORT_FXM_2_INPUT_TYPE:
	case GDB_TRANSPORT_FXM_3_INPUT_TYPE:
	case GDB_TRANSPORT_PMU_INPUT_TYPE:
	case GDB_TRANSPORT_WLAN_INPUT_TYPE:
	case GDB_TRANSPORT_WLAN_2_INPUT_TYPE:
	case GDB_TRANSPORT_WLAN_3_INPUT_TYPE:
	case GDB_TRANSPORT_WLAN_4_INPUT_TYPE:
	case GDB_TRANSPORT_WLAN_5_INPUT_TYPE:
	case GDB_TRANSPORT_WLAN_6_INPUT_TYPE:
	case GDB_TRANSPORT_WLAN_7_INPUT_TYPE:
	case GDB_TRANSPORT_WLAN_8_INPUT_TYPE:
		prealloc_bit = MIFINTRBIT_RESERVED_GDB_IN_WLAN;
		break;

	default:
		prealloc_bit = find_first_zero_bit(intr->bitmap_tohost, MIFINTRBIT_NUM_INT);
		break;
	}
	return prealloc_bit;
}
#endif

/* Public functions */
int mifintrbit_alloc_tohost(struct mifintrbit *intr, mifintrbit_handler handler, void *data, enum IRQ_TYPE irq_type)
{
	struct scsc_mif_abs *mif;
	unsigned long flags;
	int           which_bit = 0;

	spin_lock_irqsave(&intr->spinlock, flags);
	/* Search for free slots */
#if defined(CONFIG_SCSC_BB_REDWOOD)
	which_bit = assign_prealloc_bit(intr, irq_type);
#else
	which_bit = find_first_zero_bit(intr->bitmap_tohost, MIFINTRBIT_NUM_INT);
#endif

	if (which_bit >= MIFINTRBIT_NUM_INT)
		goto error;

#if !defined(CONFIG_SCSC_BB_REDWOOD)
	if (intr->mifintrbit_irq_handler[which_bit] != mifintrbit_default_handler)
		goto error;
#endif
	/* Get abs implementation */
	mif = intr->mif;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	/* Mask to prevent spurious incoming interrupts */
	mif->irq_bit_mask(mif, which_bit, intr->target);
	/* Clear the interrupt */
	mif->irq_bit_clear(mif, which_bit, intr->target);

	/* Register the handler */
	intr->mifintrbit_irq_handler[which_bit] = handler;
	intr->irq_data[which_bit] = data;
	intr->irq_type[which_bit] = irq_type;

	/* Once registration is set, and IRQ has been cleared, unmask the interrupt */
	mif->irq_bit_unmask(mif, which_bit, intr->target);
#else
	/* Mask to prevent spurious incoming interrupts */
	mif->irq_bit_mask(mif, which_bit);
	/* Clear the interrupt */
	mif->irq_bit_clear(mif, which_bit);

	/* Register the handler */
	intr->mifintrbit_irq_handler[which_bit] = handler;
	intr->irq_data[which_bit] = data;

	/* Once registration is set, and IRQ has been cleared, unmask the interrupt */
	mif->irq_bit_unmask(mif, which_bit);
#endif
	/* Update bit mask */
	set_bit(which_bit, intr->bitmap_tohost);
	SCSC_TAG_INFO(MIF, "allocated irq bit %d for %s\n", which_bit, wlbt_irq_types[irq_type]);

	spin_unlock_irqrestore(&intr->spinlock, flags);

	return which_bit;

error:
	spin_unlock_irqrestore(&intr->spinlock, flags);
	SCSC_TAG_ERR(MIF, "Error registering irq %d for %s\n", which_bit, wlbt_irq_types[which_bit]);
	return -EIO;
}

int mifintrbit_free_tohost(struct mifintrbit *intr, int which_bit)
{
	struct scsc_mif_abs *mif;
	unsigned long flags;

	if (which_bit >= MIFINTRBIT_NUM_INT)
		goto error;

	spin_lock_irqsave(&intr->spinlock, flags);
	/* Get abs implementation */
	mif = intr->mif;
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	/* Mask to prevent spurious incoming interrupts */
	mif->irq_bit_mask(mif, which_bit, intr->target);
	/* Set the handler with default */
	intr->mifintrbit_irq_handler[which_bit] = mifintrbit_default_handler;
	intr->irq_data[which_bit] = NULL;
	/* Clear the interrupt for hygiene */
	mif->irq_bit_clear(mif, which_bit, intr->target);
#else
	/* Mask to prevent spurious incoming interrupts */
	mif->irq_bit_mask(mif, which_bit);
	/* Set the handler with default */
	intr->mifintrbit_irq_handler[which_bit] = mifintrbit_default_handler;
	intr->irq_data[which_bit] = NULL;
	/* Clear the interrupt for hygiene */
	mif->irq_bit_clear(mif, which_bit);
#endif
	/* Update bit mask */
	clear_bit(which_bit, intr->bitmap_tohost);
	spin_unlock_irqrestore(&intr->spinlock, flags);

	return 0;

error:
	SCSC_TAG_ERR(MIF, "Error unregistering irq\n");
	return -EIO;
}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
int mifintrbit_alloc_fromhost(struct mifintrbit *intr)
#else
int mifintrbit_alloc_fromhost(struct mifintrbit *intr, enum scsc_mif_abs_target target)
#endif
{
	unsigned long flags;
	int           which_bit = 0;
	unsigned long *p;


	spin_lock_irqsave(&intr->spinlock, flags);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	p = intr->bitmap_fromhost;
#else
	if (target == SCSC_MIF_ABS_TARGET_WLAN)
		p = intr->bitmap_fromhost_wlan;
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	else if (target == SCSC_MIF_ABS_TARGET_FXM_1)
		p = intr->bitmap_fromhost_wlan;
	else if (target == SCSC_MIF_ABS_TARGET_FXM_2)
		p = intr->bitmap_fromhost_wlan;
#else
	else if (target == SCSC_MIF_ABS_TARGET_FXM_1)
		p = intr->bitmap_fromhost_fxm_1;
#endif
	else
		goto error;
#endif /* CONFIG_SCSC_INDEPENDENT_SUBSYSTEM */

	/* Search for free slots */
	which_bit = find_first_zero_bit(p, MIFINTRBIT_NUM_INT);

	if (which_bit == MIFINTRBIT_NUM_INT)
		goto error;

	/* Update bit mask */
	set_bit(which_bit, p);

	spin_unlock_irqrestore(&intr->spinlock, flags);

	return which_bit;
error:
	spin_unlock_irqrestore(&intr->spinlock, flags);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	SCSC_TAG_ERR(MIF, "Error allocating bit %d on %s\n",
		     which_bit, (intr->target == SCSC_MIF_ABS_TARGET_WPAN) ? "WPAN" : "WLAN");
#else
	SCSC_TAG_ERR(MIF, "Error allocating bit %d on %s\n",
		     which_bit, (target == SCSC_MIF_ABS_TARGET_WPAN) ? "WPAN" : "WLAN");
#endif /* CONFIG_SCSC_INDEPENDENT_SUBSYSTEM */
	return -EIO;
}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
int mifintrbit_free_fromhost(struct mifintrbit *intr, int which_bit)
#else
int mifintrbit_free_fromhost(struct mifintrbit *intr, int which_bit, enum scsc_mif_abs_target target)
#endif
{
	unsigned long flags;
	unsigned long *p;

	spin_lock_irqsave(&intr->spinlock, flags);


	if (which_bit >= MIFINTRBIT_NUM_INT)
		goto error;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	p = intr->bitmap_fromhost;
#else
	if (target == SCSC_MIF_ABS_TARGET_WLAN)
		p = intr->bitmap_fromhost_wlan;
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	else if (target == SCSC_MIF_ABS_TARGET_FXM_1)
		p = intr->bitmap_fromhost_wlan;
	else if (target == SCSC_MIF_ABS_TARGET_FXM_2)
		p = intr->bitmap_fromhost_wlan;
#else
	else if (target == SCSC_MIF_ABS_TARGET_FXM_1)
		p = intr->bitmap_fromhost_fxm_1;
#endif
	else
		goto error;
#endif /*CONFIG_SCSC_INDEPENDENT_SUBSYSTEM*/

	/* Clear bit mask */
	clear_bit(which_bit, p);
	spin_unlock_irqrestore(&intr->spinlock, flags);

	return 0;
error:
	spin_unlock_irqrestore(&intr->spinlock, flags);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	SCSC_TAG_ERR(MIF, "Error freeing bit %d on %s\n",
		     which_bit, (intr->target == SCSC_MIF_ABS_TARGET_WPAN) ? "WPAN" : "WLAN");
#else
	SCSC_TAG_ERR(MIF, "Error freeing bit %d on %s\n",
		     which_bit, (target == SCSC_MIF_ABS_TARGET_WPAN) ? "WPAN" : "WLAN");
#endif /*CONFIG_SCSC_INDEPENDENT_SUBSYSTEM*/
	return -EIO;
}

/* core API */
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
void mifintrbit_deinit(struct mifintrbit *intr, enum scsc_mif_abs_target target)
#else
void mifintrbit_deinit(struct mifintrbit *intr)
#endif
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&intr->spinlock, flags);
	/* Set all handlers to default before unregistering the handler */
	for (i = 0; i < MIFINTRBIT_NUM_INT; i++){
		intr->mifintrbit_irq_handler[i] = mifintrbit_default_handler;
		intr->irq_type[i] = DEFAULT_IRQ_TYPE;
	}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	SCSC_TAG_INFO(MIF, "MIF IRQ deinit on %s\n", (target == SCSC_MIF_ABS_TARGET_WPAN) ? "WPAN" : "WLAN");

	if (target == SCSC_MIF_ABS_TARGET_WLAN) {
		SCSC_TAG_INFO(MIF, "MIF IRQ Unregistering IRQ handler WLAN\n");
		intr->mif->irq_unreg_handler(intr->mif);
	} else if (target == SCSC_MIF_ABS_TARGET_WPAN) {
		SCSC_TAG_INFO(MIF, "MIF IRQ Unregistering IRQ handler WPAN\n");
		intr->mif->irq_unreg_handler_wpan(intr->mif);
	}
#else
	intr->mif->irq_unreg_handler(intr->mif);
#endif
	spin_unlock_irqrestore(&intr->spinlock, flags);
}

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
void mifintrbit_init(struct mifintrbit *intr, struct scsc_mif_abs *mif, enum scsc_mif_abs_target target)
#else
void mifintrbit_init(struct mifintrbit *intr, struct scsc_mif_abs *mif)
#endif
{
	int i;
#if defined(CONFIG_SCSC_PCIE_CHIP)
	u8 start;
	u8 end;
#endif
	spin_lock_init(&intr->spinlock);
	/* Set all handlers to default before hooking the hardware interrupt */
	for (i = 0; i < MIFINTRBIT_NUM_INT; i++)
		intr->mifintrbit_irq_handler[i] = mifintrbit_default_handler;

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	/* reset bitmaps */
	/* store the target "type", we will use it to get the obj pointer */
	intr->target = target;
	bitmap_zero(intr->bitmap_fromhost, MIFINTRBIT_NUM_INT);
#if defined(CONFIG_SCSC_PCIE_CHIP)
	mif->get_msi_range(mif, &start, &end, target);
	/* fill bitmap */
	bitmap_fill(intr->bitmap_tohost, MIFINTRBIT_NUM_INT);
	/* Clear specific range to map to actual phy MSI*/
	bitmap_clear(intr->bitmap_tohost, start, (end - start) + 1);
#else
	/* reset bitmaps */
	bitmap_zero(intr->bitmap_tohost, MIFINTRBIT_NUM_INT);
#endif
#else
	/* reset bitmaps */
	bitmap_zero(intr->bitmap_tohost, MIFINTRBIT_NUM_INT);
	bitmap_zero(intr->bitmap_fromhost_wlan, MIFINTRBIT_NUM_INT);
	bitmap_zero(intr->bitmap_fromhost_fxm_1, MIFINTRBIT_NUM_INT);
#endif

	/**
	 * Pre-allocate/reserve MIF interrupt bits 0 in both
	 * .._fromhost interrupt bits.
	 *
	 * These bits are used for purpose of forcing Panics from
	 * either MX manager or GDB monitor channels.
	 */
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	if (target == SCSC_MIF_ABS_TARGET_WLAN) {
		set_bit(MIFINTRBIT_RESERVED_PANIC_WLAN, intr->bitmap_fromhost);
		SCSC_TAG_INFO(MIF, "MIF IRQ Registering IRQ handler WLAN\n");
		/* register isr with mif abstraction */
		mif->irq_reg_handler(mif, mifiintrman_isr, (void *)intr);
	} else if (target == SCSC_MIF_ABS_TARGET_WPAN) {
		set_bit(MIFINTRBIT_RESERVED_PANIC_WPAN, intr->bitmap_fromhost);
		SCSC_TAG_INFO(MIF, "MIF IRQ Registering IRQ handler WPAN\n");
		/* register isr with mif abstraction */
		mif->irq_reg_handler_wpan(mif, mifiintrman_isr_wpan, (void *)intr);
	}
#else
	set_bit(MIFINTRBIT_RESERVED_PANIC_WLAN, intr->bitmap_fromhost_wlan);
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	set_bit(MIFINTRBIT_RESERVED_PANIC_FXM_1, intr->bitmap_fromhost_fxm_1);
	set_bit(MIFINTRBIT_RESERVED_PANIC_FXM_2, intr->bitmap_fromhost_fxm_2);
#else
	set_bit(MIFINTRBIT_RESERVED_PANIC_FXM_1, intr->bitmap_fromhost_fxm_1);
#endif

	/* register isr with mif abstraction */
	mif->irq_reg_handler(mif, mifiintrman_isr, (void *)intr);
#endif /* CONFIG_SCSC_INDEPENDENT_SUBSYSTEM */

	/* cache mif */
	intr->mif = mif;
}

void mifintrbit_dump(struct mifintrbit *intr)
{
	int i = 0;
	for(i = 0; i < MIFINTRBIT_NUM_INT; i++)	{
		if(intr->irq_type[i])
			SCSC_TAG_INFO(MXMAN, "bit:%d, handler %s\n", i, wlbt_irq_types[intr->irq_type[i]]);
	}
}
