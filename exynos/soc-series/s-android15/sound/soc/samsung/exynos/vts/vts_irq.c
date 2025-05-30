// SPDX-License-Identifier: GPL-2.0-or-later
/* sound/soc/samsung/vts/vts_irq.c
 *
 * ALSA SoC - Samsung VTS driver
 *
 * implementing the most common irq chip callback functions
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/syscore_ops.h>

static LIST_HEAD(gc_list);
static DEFINE_RAW_SPINLOCK(gc_lock);

static void ack_bad(struct irq_data *data) { }

/*
 * NOP functions
 */
static void noop(struct irq_data *data) { }

static unsigned int noop_ret(struct irq_data *data)
{
	return 0;
}

/*
 * Generic no controller implementation
 */
struct irq_chip vts_no_irq_chip = {
	.name		= "none",
	.irq_startup	= noop_ret,
	.irq_shutdown	= noop,
	.irq_enable	= noop,
	.irq_disable	= noop,
	.irq_ack	= ack_bad,
	.flags		= IRQCHIP_SKIP_SET_WAKE,
};

/**
 * irq_gc_mask_set_bit - Mask chip via setting bit in mask register
 * @d: irq_data
 *
 * Chip has a single mask register. Values of this register are cached
 * and protected by gc->lock
 */
void vts_irq_gc_mask_set_bit(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	u32 mask = d->mask;

	irq_gc_lock(gc);
	*ct->mask_cache |= mask;
	irq_reg_writel(gc, *ct->mask_cache, ct->regs.mask);
	irq_gc_unlock(gc);
}
EXPORT_SYMBOL_GPL(vts_irq_gc_mask_set_bit);

/**
 * vts_irq_gc_mask_clr_bit - Mask chip via clearing bit in mask register
 * @d: irq_data
 *
 * Chip has a single mask register. Values of this register are cached
 * and protected by gc->lock
 */
void vts_irq_gc_mask_clr_bit(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	u32 mask = d->mask;

	irq_gc_lock(gc);
	*ct->mask_cache &= ~mask;
	irq_reg_writel(gc, *ct->mask_cache, ct->regs.mask);
	irq_gc_unlock(gc);
}
EXPORT_SYMBOL_GPL(vts_irq_gc_mask_clr_bit);

/**
 * vts_irq_gc_ack_set_bit - Ack pending interrupt via setting bit
 * @d: irq_data
 */
void vts_irq_gc_ack_set_bit(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	u32 mask = d->mask;

	irq_gc_lock(gc);
	irq_reg_writel(gc, mask, ct->regs.ack);
	irq_gc_unlock(gc);
}
EXPORT_SYMBOL_GPL(vts_irq_gc_ack_set_bit);

static u32 vts_irq_readl_be(void __iomem *addr)
{
	return ioread32be(addr);
}

static void vts_irq_writel_be(u32 val, void __iomem *addr)
{
	iowrite32be(val, addr);
}

void vts_irq_init_generic_chip(struct irq_chip_generic *gc, const char *name,
			   int num_ct, unsigned int irq_base,
			   void __iomem *reg_base, irq_flow_handler_t handler)
{
	raw_spin_lock_init(&gc->lock);
	gc->num_ct = num_ct;
	gc->irq_base = irq_base;
	gc->reg_base = reg_base;
	gc->chip_types->chip.name = name;
	gc->chip_types->handler = handler;
}

static void
vts_irq_gc_init_mask_cache(struct irq_chip_generic *gc, enum irq_gc_flags flags)
{
	struct irq_chip_type *ct = gc->chip_types;
	u32 *mskptr = &gc->mask_cache, mskreg = ct->regs.mask;
	int i;

	for (i = 0; i < gc->num_ct; i++) {
		if (flags & IRQ_GC_MASK_CACHE_PER_TYPE) {
			mskptr = &ct[i].mask_cache_priv;
			mskreg = ct[i].regs.mask;
		}
		ct[i].mask_cache = mskptr;
		if (flags & IRQ_GC_INIT_MASK_CACHE)
			*mskptr = irq_reg_readl(gc, mskreg);
	}
}

/**
 * __irq_alloc_domain_generic_chip - Allocate generic chips for an irq domain
 * @d:			irq domain for which to allocate chips
 * @irqs_per_chip:	Number of interrupts each chip handles (max 32)
 * @num_ct:		Number of irq_chip_type instances associated with this
 * @name:		Name of the irq chip
 * @handler:		Default flow handler associated with these chips
 * @clr:		IRQ_* bits to clear in the mapping function
 * @set:		IRQ_* bits to set in the mapping function
 * @gcflags:		Generic chip specific setup flags
 */
int vts_irq_alloc_domain_generic_chips(struct irq_domain *d, int irqs_per_chip,
				     int num_ct, const char *name,
				     irq_flow_handler_t handler,
				     unsigned int clr, unsigned int set,
				     enum irq_gc_flags gcflags)
{
	struct irq_domain_chip_generic *dgc;
	struct irq_chip_generic *gc;
	int numchips, sz, i;
	unsigned long flags;
	void *tmp;

	// BUILD_BUG_ON(irqs_per_chip >= 32);
	if (d->gc)
		return -EBUSY;

	numchips = DIV_ROUND_UP(d->revmap_size, irqs_per_chip);
	if (!numchips)
		return -EINVAL;

	/* Allocate a pointer, generic chip and chiptypes for each chip */
	sz = sizeof(*dgc) + numchips * sizeof(gc);
	sz += numchips * (sizeof(*gc) + num_ct * sizeof(struct irq_chip_type));

	tmp = dgc = kzalloc(sz, GFP_KERNEL);
	if (!dgc)
		return -ENOMEM;
	dgc->irqs_per_chip = irqs_per_chip;
	dgc->num_chips = numchips;
	dgc->irq_flags_to_set = set;
	dgc->irq_flags_to_clear = clr;
	dgc->gc_flags = gcflags;
	d->gc = dgc;

	/* Calc pointer to the first generic chip */
	tmp += sizeof(*dgc) + numchips * sizeof(gc);
	for (i = 0; i < numchips; i++) {
		/* Store the pointer to the generic chip */
		dgc->gc[i] = gc = tmp;
		vts_irq_init_generic_chip(gc, name, num_ct, i * irqs_per_chip,
				      NULL, handler);

		gc->domain = d;
		if (gcflags & IRQ_GC_BE_IO) {
			gc->reg_readl = &vts_irq_readl_be;
			gc->reg_writel = &vts_irq_writel_be;
		}

		raw_spin_lock_irqsave(&gc_lock, flags);
		list_add_tail(&gc->list, &gc_list);
		raw_spin_unlock_irqrestore(&gc_lock, flags);
		/* Calc pointer to the next generic chip */
		tmp += sizeof(*gc) + num_ct * sizeof(struct irq_chip_type);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(vts_irq_alloc_domain_generic_chips);

static struct irq_chip_generic *
__vts_irq_get_domain_generic_chip(struct irq_domain *d, unsigned int hw_irq)
{
	struct irq_domain_chip_generic *dgc = d->gc;
	int idx;

	if (!dgc)
		return ERR_PTR(-ENODEV);
	idx = hw_irq / dgc->irqs_per_chip;
	if (idx >= dgc->num_chips)
		return ERR_PTR(-EINVAL);
	return dgc->gc[idx];
}

/**
 * irq_get_domain_generic_chip - Get a pointer to the generic chip of a hw_irq
 * @d:			irq domain pointer
 * @hw_irq:		Hardware interrupt number
 */
struct irq_chip_generic *
vts_irq_get_domain_generic_chip(struct irq_domain *d, unsigned int hw_irq)
{
	struct irq_chip_generic *gc =
		__vts_irq_get_domain_generic_chip(d, hw_irq);

	return !IS_ERR(gc) ? gc : NULL;
}
EXPORT_SYMBOL_GPL(vts_irq_get_domain_generic_chip);

/*
 * Separate lockdep classes for interrupt chip which can nest irq_desc
 * lock and request mutex.
 */
static struct lock_class_key irq_nested_lock_class;
static struct lock_class_key irq_nested_request_class;

/*
 * irq_map_generic_chip - Map a generic chip for an irq domain
 */
int vts_irq_map_generic_chip(struct irq_domain *d, unsigned int virq,
			 irq_hw_number_t hw_irq)
{
	struct irq_data *data = irq_domain_get_irq_data(d, virq);
	struct irq_domain_chip_generic *dgc = d->gc;
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;
	struct irq_chip *chip;
	unsigned long flags;
	int idx;

	gc = __vts_irq_get_domain_generic_chip(d, hw_irq);
	if (IS_ERR(gc))
		return PTR_ERR(gc);

	idx = hw_irq % dgc->irqs_per_chip;

	if (test_bit(idx, &gc->unused))
		return -ENOTSUPP;

	if (test_bit(idx, &gc->installed))
		return -EBUSY;

	ct = gc->chip_types;
	chip = &ct->chip;

	/* We only init the cache for the first mapping of a generic chip */
	if (!gc->installed) {
		raw_spin_lock_irqsave(&gc->lock, flags);
		vts_irq_gc_init_mask_cache(gc, dgc->gc_flags);
		raw_spin_unlock_irqrestore(&gc->lock, flags);
	}

	/* Mark the interrupt as installed */
	set_bit(idx, &gc->installed);

	if (dgc->gc_flags & IRQ_GC_INIT_NESTED_LOCK)
		irq_set_lockdep_class(virq, &irq_nested_lock_class,
				      &irq_nested_request_class);

	if (chip->irq_calc_mask)
		chip->irq_calc_mask(data);
	else
		data->mask = 1 << idx;

	irq_domain_set_info(d, virq, hw_irq, chip, gc, ct->handler, NULL, NULL);
	irq_modify_status(virq, dgc->irq_flags_to_clear, dgc->irq_flags_to_set);
	return 0;
}

void vts_irq_unmap_generic_chip(struct irq_domain *d, unsigned int virq)
{
	struct irq_data *data = irq_domain_get_irq_data(d, virq);
	struct irq_domain_chip_generic *dgc = d->gc;
	unsigned int hw_irq = data->hwirq;
	struct irq_chip_generic *gc;
	int irq_idx;

	gc = vts_irq_get_domain_generic_chip(d, hw_irq);
	if (!gc)
		return;

	irq_idx = hw_irq % dgc->irqs_per_chip;

	clear_bit(irq_idx, &gc->installed);
	irq_domain_set_info(d, virq, hw_irq, &vts_no_irq_chip, NULL, NULL, NULL,
			    NULL);

}

/**
 * irq_remove_generic_chip - Remove a chip
 * @gc:		Generic irq chip holding all data
 * @msk:	Bitmask holding the irqs to initialize relative to gc->irq_base
 * @clr:	IRQ_* bits to clear
 * @set:	IRQ_* bits to set
 *
 * Remove up to 32 interrupts starting from gc->irq_base.
 */
void vts_irq_remove_generic_chip(struct irq_chip_generic *gc, u32 msk,
			     unsigned int clr, unsigned int set)
{
	unsigned int i = gc->irq_base;

	raw_spin_lock(&gc_lock);
	list_del(&gc->list);
	raw_spin_unlock(&gc_lock);

	for (; msk; msk >>= 1, i++) {
		if (!(msk & 0x01))
			continue;

		/* Remove handler first. That will mask the irq line */
		irq_set_handler(i, NULL);
		irq_set_chip(i, &vts_no_irq_chip);
		irq_set_chip_data(i, NULL);
		irq_modify_status(i, clr, set);
	}
}
EXPORT_SYMBOL_GPL(vts_irq_remove_generic_chip);
