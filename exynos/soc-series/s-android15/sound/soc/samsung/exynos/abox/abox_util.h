/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ALSA SoC - Samsung Abox utility
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_UTIL_H
#define __SND_SOC_ABOX_UTIL_H

#include <sound/pcm.h>
#include <linux/firmware.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>

/**
 * get resource
 * @param[in]	pdev		pointer to platform device structure
 * @param[in]	name		name of resource
 * @param[out]	phys_addr	physical address of the resource
 * @param[out]	size		size of the resource
 * @return	0 or error code
 */
extern int get_resource_mem(struct platform_device *pdev,
		const char *name, phys_addr_t *phys_addr, size_t *size);

/**
 * ioremap to virtual address but not request
 * @param[in]	pdev		pointer to platform device structure
 * @param[in]	name		name of resource
 * @param[out]	phys_addr	physical address of the resource
 * @param[out]	size		size of the resource
 * @return	virtual address
 */
extern void __iomem *devm_get_ioremap(struct platform_device *pdev,
		const char *name, phys_addr_t *phys_addr, size_t *size);

/**
 * Request memory resource and map to virtual address
 * @param[in]	pdev		pointer to platform device structure
 * @param[in]	name		name of resource
 * @param[out]	phys_addr	physical address of the resource
 * @param[out]	size		size of the resource
 * @return	virtual address
 */
extern void __iomem *devm_get_request_ioremap(struct platform_device *pdev,
		const char *name, phys_addr_t *phys_addr, size_t *size);

/**
 * Request clock and prepare
 * @param[in]	pdev		pointer to platform device structure
 * @param[in]	name		name of clock
 * @return	pointer to clock
 */
extern struct clk *devm_clk_get_and_prepare(struct platform_device *pdev,
		const char *name);

/**
 * Read single long physical address (sleeping function)
 * @param[in]	addr		physical address
 * @return	value of the physical address
 */
extern u32 readl_phys(phys_addr_t addr);

/**
 * Write single long physical address (sleeping function)
 * @param[in]	val		value
 * @param[in]	addr		physical address
 */
extern void writel_phys(unsigned int val, phys_addr_t addr);

/**
 * Update single long physical address (sleeping function)
 * @param[in]	val		value
 * @param[in]	addr		physical address
 */
extern void updatel_phys(unsigned int val, phys_addr_t addr);

/**
 * Atomically increments @v, if @v was @r, set to 0.
 * @param[in]	v		pointer of type atomic_t
 * @param[in]	r		maximum range of @v.
 * @return	Returns old value
 */
static inline int atomic_inc_unless_in_range(atomic_t *v, int r)
{
	int ret;

	while ((ret = atomic_add_unless(v, 1, r)) == r) {
		ret = atomic_cmpxchg(v, r, 0);
		if (ret == r)
			break;
	}

	return ret;
}

/**
 * Atomically decrements @v, if @v was 0, set to @r.
 * @param[in]	v		pointer of type atomic_t
 * @param[in]	r		maximum range of @v.
 * @return	Returns old value
 */
static inline int atomic_dec_unless_in_range(atomic_t *v, int r)
{
	int ret;

	while ((ret = atomic_add_unless(v, -1, 0)) == 0) {
		ret = atomic_cmpxchg(v, 0, r);
		if (ret == 0)
			break;
	}

	return ret;
}

/**
 * Check whether the GIC is secure (sleeping function)
 * @return	true if the GIC is secure, false on otherwise
 */
extern bool is_secure_gic(void);

/**
 * Get character from substream direction
 * @param[in]	substream	substream
 * @return	'p' if direction is playback. 'c' if not.
 */
extern char substream_to_char(struct snd_pcm_substream *substream);

/**
 * Find property with samsung, prefix
 * @param[in]	dev		pointer to device invoking this API
 * @param[in]	np		device node
 * @param[in]	propname	name of the property
 * @param[out]	lenp		length of the property
 * @return	property or NULL
 */
extern struct property *of_samsung_find_property(struct device *dev,
		const struct device_node *np,
		const char *propname, int *lenp);

/**
 * Get whether the property is exist or not with samsung, prefix
 * @param[in]	dev		pointer to device invoking this API
 * @param[in]	np		device node
 * @param[in]	propname	name of the property
 * @return	true or false
 */
extern bool of_samsung_property_read_bool(struct device *dev,
		const struct device_node *np, const char *propname);

/**
 * Get property value with samsung, prefix
 * @param[in]	dev		pointer to device invoking this API
 * @param[in]	np		device node
 * @param[in]	propname	name of the property
 * @param[out]	out_value	pointer to return value
 * @return	error code
 */
extern int of_samsung_property_read_u32(struct device *dev,
		const struct device_node *np,
		const char *propname, u32 *out_value);

/**
 * Get property value with samsung, prefix
 * @param[in]	dev		pointer to device invoking this API
 * @param[in]	np		device node
 * @param[in]	propname	name of the property
 * @param[out]	out_values	pointer to return value
 * @param[in]	sz		number of array elements to read
 * @return	error code
 */
extern int of_samsung_property_read_u32_array(struct device *dev,
		const struct device_node *np,
		const char *propname, u32 *out_values, size_t sz);

/**
 * Get property value with samsung, prefix
 * @param[in]	dev		pointer to device invoking this API
 * @param[in]	np		device node
 * @param[in]	propname	name of the property
 * @param[out]	out_values	pointer to return value
 * @param[in]	sz_min		minimum number of array elements to read
 * @param[in]	sz_max		maximum number of array elements to read
 * @return	error code
 */
extern int of_samsung_property_read_variable_u32_array(struct device *dev,
		const struct device_node *np, const char *propname,
		u32 *out_values, size_t sz_min, size_t sz_max);

/**
 * Get property value with samsung, prefix
 * @param[in]	dev		pointer to device invoking this API
 * @param[in]	np		device node
 * @param[in]	propname	name of the property
 * @param[out]	out_string	pointer to return value
 * @return	error code
 */
extern int of_samsung_property_read_string(struct device *dev,
		const struct device_node *np,
		const char *propname, const char **out_string);
/**
 * simple callback for request_firmware_nowait
 * @param[in]	fw		requested firmware
 * @param[in]	context		it should be const struct firmware **p_firmware
 */
extern void cache_firmware_simple(const struct firmware *fw, void *context);

/**
 * map a reserved memory into the kernel space
 * @param[in]	rmem		physical address
 * @param[in]	size		size of the area
 */
extern void *rmem_vmap(const struct reserved_mem *rmem);

#endif /* __SND_SOC_ABOX_UTIL_H */
