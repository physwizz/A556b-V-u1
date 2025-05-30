/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ALSA SoC - Samsung ABOX driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ABOX_H
#define __ABOX_H

#include <linux/device.h>
#include <linux/irqreturn.h>
#include <linux/scatterlist.h>
#include <sound/soc.h>
#include <sound/samsung/abox_ipc.h>

#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
#include <sound/samsung/sec_audio_exynos.h>
#endif

/**
 * abox ipc handler type definition
 * @param[in]	ipc_id	id of ipc
 * @param[in]	dev_id	dev_id from abox_register_ipc_handler
 * @param[in]	msg	message data
 * @return	reference irqreturn_t
 */
typedef irqreturn_t (*abox_ipc_handler_t)(int ipc_id, void *dev_id,
		ABOX_IPC_MSG *msg);

enum abox_call_event {
	ABOX_CALL_EVENT_OFF		= 0,
	ABOX_CALL_EVENT_ON		= 1,
	ABOX_CALL_EVENT_UNDEF		= 2,
	ABOX_CALL_EVENT_VSS_STOPPED	= 3,
	ABOX_CALL_EVENT_VSS_STARTED	= 4,
	ABOX_CALL_EVENT_VSS_UNDEF	= 5,
};

enum abox_modem_event {
	ABOX_MODEM_EVENT_RESET,
	ABOX_MODEM_EVENT_EXIT,
	ABOX_MODEM_EVENT_ONLINE,
	ABOX_MODEM_EVENT_OFFLINE,
	ABOX_MODEM_EVENT_WATCHDOG,
};

#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_ABOX)
/**
 * Check ABOX is on
 * @return	true if A-Box is on, false on otherwise
 */
extern bool abox_is_on(void);

/**
 * Get INT frequency required by ABOX
 * @return	INT frequency in kHz
 */
extern unsigned int abox_get_requiring_int_freq_in_khz(void);

/**
 * Get MIF frequency required by ABOX
 * @return	MIF frequency in kHz
 */
extern unsigned int abox_get_requiring_mif_freq_in_khz(void);

/**
 * Get AUD frequency required by ABOX
 * @return	AUD frequency in kHz
 */
extern unsigned int abox_get_requiring_aud_freq_in_khz(void);

/**
 * Get routing path
 * @return		routing path. See below.
 *	0: Receiver
 *	1: Speaker
 *	2: Headset (3.5pi)
 *	3: Bluetooth
 *	4: USB
 *	5: Call forwarding
 */
extern unsigned int abox_get_routing_path(void);

/**
 * Start abox IPC
 * @param[in]	dev		pointer to abox device
 * @param[in]	hw_irq		hardware IRQ number
 * @param[in]	msg		pointer to message
 * @param[in]	size		size of message
 * @param[in]	atomic		1, if caller context is atomic. 0, if not.
 * @param[in]	sync		1 to wait for ack. 0 if not.
 * @return	0 or error code
 */
extern int abox_request_ipc(struct device *dev, int hw_irq, const ABOX_IPC_MSG *msg,
		size_t size, int atomic, int sync);

/**
 * Register ipc handler to abox
 * @param[in]	dev		pointer to abox device
 * @param[in]	ipc_id		id of ipc
 * @param[in]	ipc_handler	abox ipc handler to register
 * @param[in]	dev_id		cookie which would be summitted with irq_handler
 * @return	0 or error code
 */
extern int abox_register_ipc_handler(struct device *dev, int ipc_id,
		abox_ipc_handler_t ipc_handler, void *dev_id);

/**
 * UAIF/DSIF hw params fixup helper
 * @param[in]	rtd		snd_soc_pcm_runtime
 * @param[out]	params		snd_pcm_hw_params
 * @return	0 or error code
 */
extern int abox_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params);

/**
 * Request or release dram during cpuidle (count based API)
 * @param[in]	dev		pointer to abox device
 * @param[in]	id		key which is used as unique handle
 * @param[in]	on		true for requesting, false on otherwise
 */
extern void abox_request_dram_on(struct device *dev, unsigned int id, bool on);

/**
 * iommu map for abox
 * @param[in]	dev		pointer to abox device
 * @param[in]	iova		device virtual address
 * @param[in]	addr		kernel physical address
 * @param[in]	bytes		size of the mapping area
 * @param[in]	area		kernel virtual address
 * @return	0 or error code
 */
extern int abox_iommu_map(struct device *dev, unsigned long iova,
		phys_addr_t addr, size_t bytes, void *area);

/**
 * iommu map for abox with cache coherency
 * @param[in]	dev		pointer to abox device
 * @param[in]	iova		device virtual address
 * @param[in]	addr		kernel physical address
 * @param[in]	bytes		size of the mapping area
 * @param[in]	area		kernel virtual address
 * @return	0 or error code
 */
extern int abox_iommu_map_cache(struct device *dev, unsigned long iova,
		phys_addr_t addr, size_t bytes, void *area);

/**
 * iommu map for abox
 * @param[in]	dev		pointer to abox device
 * @param[in]	iova		device virtual address
 * @param[in]	sg		scatter list
 * @param[in]	nents		nents of scatter list
 * @param[in]	prot		protection parameter
 * @param[in]	bytes		size of the mapping area
 * @param[in]	area		kernel virtual address
 * @return	0 or error code
 */
extern int abox_iommu_map_sg(struct device *dev, unsigned long iova,
		struct scatterlist *sg, unsigned int nents,
		int prot, size_t bytes, void *area);

/**
 * iommu unmap for abox
 * @param[in]	dev		pointer to abox device
 * @param[in]	iova		device virtual address
 * @return	0 or error code
 */
extern int abox_iommu_unmap(struct device *dev, unsigned long iova);

/**
 * query physical address from device virtual address
 * @param[in]	dev		pointer to abox device
 * @param[in]	iova		device virtual address
 * @return	physical address. 0 if not mapped
 */
extern phys_addr_t abox_iova_to_phys(struct device *dev, unsigned long iova);

/**
 * query virtual address from device virtual address
 * @param[in]	dev		pointer to abox device
 * @param[in]	iova		device virtual address
 * @return	virtual address. undefined if not mapped
 */
extern void *abox_iova_to_virt(struct device *dev, unsigned long iova);

/**
 * enable or disable RTOS profiling
 * @param[in]	en		enable or disable
 * @return	0 or error code
 */
extern int abox_set_profiling(int en);

/**
 * print only important gpr values from gpr dump sfr to buffer
 * @param[in]	buf		buffer to print gpr
 * @param[in]	len		length of the buffer
 * @return	number of characters written into @buf
 */
extern int abox_show_gpr_min(char *buf, int len);

/**
 * write value to sysreg
 * @param[in]	value		value to write
 * @param[in]	offset		offset of target address
 * @return	0 or error code
 */
extern int abox_write_sysreg(unsigned int value, unsigned int offset);

/**
 * read value from sysreg
 * @param[out]	value		value to read
 * @param[in]	offset		offset of target address
 * @return	0 or error code
 */
extern int abox_read_sysreg(unsigned int *value, unsigned int offset);

/**
 * read gpr value
 * @param[in]	core_id		id of core
 * @param[in]	gpr_id		id of gpr
 * @return	value of gpr or 0
 */
extern u32 abox_read_gpr(int core_id, int gpr_id);

/**
 * query call state
 * @return	call state
 */
extern bool abox_get_call_state(void);

/**
 * set physical address of doorbell
 * @param[in]	addr		address
 * @return	0 or error code
 */
extern bool abox_pci_doorbell_paddr_set(phys_addr_t addr);

/**
 * register abox call event notifier
 * @param[in]	nb		notifer block
 * @return	0 or error code
 */
extern int register_abox_call_event_notifier(struct notifier_block *nb);

/**
 * notify modem event
 * @param[in]	event		modem event
 * @return	0 or error code
 */
extern int abox_notify_modem_event(enum abox_modem_event event);


/**
 * request abox power
 * @param[in]	dev		pointer to abox device
 * @param[in]	id		key which is used as unique handle
 * @param[in]	name		cookie for logging
 * @return	0 or error code
 */
extern int abox_request_power(struct device *dev, unsigned int id, const char *name);

/**
 * release abox power
 * @param[in]	dev		pointer to abox device
 * @param[in]	id		key which is used as unique handle
 * @param[in]	name		cookie for logging
 * @return	0 or error code
 */
extern int abox_release_power(struct device *dev, unsigned int id, const char *name);

/**
 * synchronize pending power requests
 */
extern void abox_synchronize_power(void);

/**
 * wait until abox is booted
 * @param[in]	dev		pointer to abox device
 * @param[in]	jiffies		timeout in jiffies
 * @return	if the time is elapsed, 0 or 1. if not, remaining jiffies.
 * 		refer to the wait_event_timeout().
 */
extern long abox_wait_boot_timeout(struct device *dev, unsigned long jiffies);

/**
 * abox forced enable
**/
extern void abox_force_enable(void);
#else /* !CONFIG_SND_SOC_SAMSUNG_ABOX */

static inline bool abox_is_on(void)
{
	return false;
}

static inline unsigned int abox_get_requiring_int_freq_in_khz(void)
{
	return 0;
}

static inline unsigned int abox_get_requiring_mif_freq_in_khz(void)
{
	return 0;
}

static inline unsigned int abox_get_requiring_aud_freq_in_khz(void)
{
	return 0;
}

static inline unsigned int abox_get_routing_path(void)
{
	return 0;
}

static inline int abox_request_ipc(struct device *dev, int hw_irq, const ABOX_IPC_MSG *msg,
		size_t size, int atomic, int sync)
{
	return -ENODEV;
}

static inline int abox_register_ipc_handler(struct device *dev, int ipc_id,
		abox_ipc_handler_t ipc_handler, void *dev_id)
{
	return -ENODEV;
}

static inline int abox_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params)
{
	return -ENODEV;
}

static inline void abox_request_dram_on(struct device *dev, unsigned int id, bool on)
{
}

static inline int abox_iommu_map(struct device *dev, unsigned long iova,
		phys_addr_t addr, size_t bytes, void *area)
{
	return -ENODEV;
}

static inline int abox_iommu_map_cache(struct device *dev, unsigned long iova,
		phys_addr_t addr, size_t bytes, void *area)
{
	return -ENODEV;
}


static inline int abox_iommu_map_sg(struct device *dev, unsigned long iova,
		struct scatterlist *sg, unsigned int nents,
		int prot, size_t bytes, void *area)
{
	return -ENODEV;
}

static inline int abox_iommu_unmap(struct device *dev, unsigned long iova)
{
	return -ENODEV;
}

static inline phys_addr_t abox_iova_to_phys(struct device *dev, unsigned long iova)
{
	return 0;
}

static inline void *abox_iova_to_virt(struct device *dev, unsigned long iova)
{
	return ERR_PTR(-ENODEV);
}

static inline int abox_set_profiling(int en)
{
	return -ENODEV;
}

static inline int abox_show_gpr_min(char *buf, int len)
{
	return -ENODEV;
}

static inline int abox_write_sysreg(unsigned int value, unsigned int offset)
{
	return -ENODEV;
}

static inline int abox_read_sysreg(unsigned int *value, unsigned int offset)
{
	return -ENODEV;
}

static inline u32 abox_read_gpr(int core_id, int gpr_id)
{
	return -ENODEV;
}

static inline bool abox_get_call_state(void)
{
	return false;
}

static inline bool abox_pci_doorbell_paddr_set(phys_addr_t addr)
{
	return false;
}

static inline int register_abox_call_event_notifier(struct notifier_block *nb)
{
	return -ENODEV;
}

static inline int abox_notify_modem_event(enum abox_modem_event event)
{
	return -ENODEV;
}

static inline int abox_request_power(struct device *dev, unsigned int id, const char *name)
{
	return -ENODEV;
}

static inline int abox_release_power(struct device *dev, unsigned int id, const char *name)
{
	return -ENODEV;
}

static inline void abox_synchronize_power(void)
{
}

static inline long abox_wait_boot_timeout(struct device *dev, unsigned long jiffies)
{
	return -ENODEV;
}
#endif /* !CONFIG_SND_SOC_SAMSUNG_ABOX */

/**
 * Start abox IPC
 * @param[in]	dev		pointer to abox device
 * @param[in]	hw_irq		hardware IRQ number
 * @param[in]	supplement	pointer to data
 * @param[in]	size		size of data which are pointed by supplement
 * @param[in]	atomic		1, if caller context is atomic. 0, if not.
 * @param[in]	sync		1 to wait for ack. 0 if not.
 * @return	0 or error code
 * @deprecated	use abox_request_ipc() instead
 */
static inline int abox_start_ipc_transaction(struct device *dev,
		int hw_irq, const void *supplement,
		size_t size, int atomic, int sync)
{
	return abox_request_ipc(dev, hw_irq, supplement, size, atomic, sync);
}

/**
 * request abox power synchronously
 * @param[in]	dev		pointer to abox device
 * @param[in]	id		key which is used as unique handle
 * @param[in]	name		cookie for logging
 * @return	0 or error code
 */
static inline int abox_request_power_sync(struct device *dev, unsigned int id, const char *name)
{
	int ret;

	ret = abox_request_power(dev, id, name);
	abox_synchronize_power();
	return ret;
}

/**
 * release abox power synchronously
 * @param[in]	dev		pointer to abox device
 * @param[in]	id		key which is used as unique handle
 * @param[in]	name		cookie for logging
 * @return	0 or error code
 */
static inline int abox_release_power_sync(struct device *dev, unsigned int id, const char *name)
{
	int ret;

	ret = abox_release_power(dev, id, name);
	abox_synchronize_power();
	return ret;
}

#endif /* __ABOX_H */

