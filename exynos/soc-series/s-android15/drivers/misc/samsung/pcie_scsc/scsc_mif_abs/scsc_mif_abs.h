struct device;

/* To WLAN/WPAN */
enum scsc_mif_abs_target {
	SCSC_MIF_ABS_TARGET_WLAN = 0,
	/* To keep backwards compatibility */
	SCSC_MIF_ABS_TARGET_R4 = 0,
	SCSC_MIF_ABS_TARGET_FXM_1 = 1,
	/* To keep backwards compatibility */
	SCSC_MIF_ABS_TARGET_M4 = 1,
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	SCSC_MIF_ABS_TARGET_M4_1 = 2,
	SCSC_MIF_ABS_TARGET_FXM_2 = 2,
#endif
	SCSC_MIF_ABS_TARGET_WPAN = 3,
#if defined(CONFIG_SCSC_PCIE_CHIP)
	SCSC_MIF_ABS_TARGET_PMU = 4,
	SCSC_MIF_ABS_TARGET_FXM_3 = 5,
	SCSC_MIF_ABS_TARGET_WLAN_2 = 6,
	SCSC_MIF_ABS_TARGET_WLAN_3 = 7,
	SCSC_MIF_ABS_TARGET_WLAN_4 = 8,
#endif
#if defined(CONFIG_SCSC_BB_REDWOOD)
	SCSC_MIF_ABS_TARGET_WLAN_5 = 9,
	SCSC_MIF_ABS_TARGET_WLAN_6 = 10,
	SCSC_MIF_ABS_TARGET_WLAN_7 = 11,
	SCSC_MIF_ABS_TARGET_WLAN_8 = 12,
#endif
};


#ifdef CONFIG_SCSC_QOS
struct scsc_mifqos_request {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct exynos_pm_qos_request pm_qos_req_mif;
	struct exynos_pm_qos_request pm_qos_req_int;
	struct freq_qos_request pm_qos_req_cl0;
	struct freq_qos_request pm_qos_req_cl1;
	struct cpufreq_policy* cpu_cluster0_policy;
	struct cpufreq_policy* cpu_cluster1_policy;
#if defined(CONFIG_SOC_S5E9815) || defined(CONFIG_SCSC_PCIE_CHIP)
	struct freq_qos_request pm_qos_req_cl2;
	struct cpufreq_policy* cpu_cluster2_policy;
#if defined(CONFIG_SOC_S5E9945)
	struct freq_qos_request pm_qos_req_cl3;
	struct cpufreq_policy *cpu_cluster3_policy;
#endif
#endif
#else
	struct pm_qos_request pm_qos_req_mif;
	struct pm_qos_request pm_qos_req_int;
	struct pm_qos_request pm_qos_req_cl0;
	struct pm_qos_request pm_qos_req_cl1;
#ifdef CONFIG_SOC_S5E9815
	struct pm_qos_request pm_qos_req_cl2;
#ifdef CONFIG_SOC_S5E9945
	struct pm_qos_request pm_qos_req_cl3;
#endif
#endif
#endif
};
#endif

#define SCSC_REG_READ_WLBT_STAT		0

/**
 * Abstraction of the Maxwell "Memory Interface" aka  MIF.
 *
 * There will be at least two implementations of this
 * interface - The native AXI one and a PCIe based emulation.
 *
 * A reference to an interface will be passed to the
 * scsc_mx driver when the system startsup.
 */
struct scsc_mif_abs {
/**
 * Destroy this interface.
 *
 * This should be called when the underlying device is
 * removed.
 */
	void (*destroy)(struct scsc_mif_abs *interface);
	/* Return an unique id for this host, and prefreabrly identifies specific device (example pcie0, pcie1) */
	char *(*get_uid)(struct scsc_mif_abs *interface);
/**
 * Controls the hardware "reset" state of the Maxwell
 * subsystem.
 *
 * Setting reset=TRUE places the subsystem in its low
 * power "reset" state. This function is called
 * by the Maxwell Manager near the end of the subsystem
 * shutdown process, before "unmapping" the interface.
 *
 * Setting reset=FALSE release the subsystem reset state.
 * The subystem will then start its cold boot sequence. This
 * function is called
 * by the Subsystem Manager near the end of the subsystem
 * startup process after installing the maxwell firmware and
 * other resources in MIF RAM.
 */
	int (*reset)(struct scsc_mif_abs *interface, bool reset);
/**
 * This function maps the Maxwell interface hardware (MIF
 * DRAM) into kernel memory space.
 *
 * Amount of memory allocated must be defined and returned
 * on (*allocated) by the abstraction layer implemenation.
 *
 * This returns kernel-space pointer to the start of the
 * shared MIF DRAM. The Maxwell Manager will load firmware
 * to this location and configure the MIF Heap Manager to
 * manage any unused memory at the end of the DRAM region.
 *
 * The scsc_mx driver should call this when the Maxwell
 * subsystem is required by any service client.
 *
 * The mailbox, irq and dram functions are only usable once
 * this call has returned. HERE: Should we rename this to
 * "open" and return a handle to these conditional methods?
 */
	void *(*map)(struct scsc_mif_abs *interface, size_t *allocated);
/**
 * The inverse of "map". Should be called once the maxwell
 * subsystem is no longer required and has been placed into
 * "reset" state (see reset method).
 */
	void (*unmap)(struct scsc_mif_abs *interface, void *mem);

#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	u32  *(*get_mbox_ptr)(struct scsc_mif_abs *interface, u32 mbox_index, enum scsc_mif_abs_target target);
	/** Get the incoming interrupt source mask */
	u32 (*irq_bit_mask_status_get)(struct scsc_mif_abs *interface, enum scsc_mif_abs_target target);

	/** Get the incoming interrupt pending (waiting  *AND* not masked) mask */
	u32 (*irq_get)(struct scsc_mif_abs *interface, enum scsc_mif_abs_target target);

	void (*irq_bit_clear)(struct scsc_mif_abs *interface, int bit_num, enum scsc_mif_abs_target target);
	void (*irq_bit_mask)(struct scsc_mif_abs *interface, int bit_num, enum scsc_mif_abs_target target);
	void (*irq_bit_unmask)(struct scsc_mif_abs *interface, int bit_num, enum scsc_mif_abs_target target);

	void (*mailbox_set)(struct scsc_mif_abs *interface, u32 mbox_index, u32 value, enum scsc_mif_abs_target target);
	u32  (*mailbox_get)(struct scsc_mif_abs *interface, u32 mbox_index, enum scsc_mif_abs_target target);
/**
 * Outgoing MIF Interrupt Hardware Controls
 */
	void (*irq_bit_set)(struct scsc_mif_abs *interface, int bit_num, enum scsc_mif_abs_target target);
#if defined(CONFIG_SCSC_PCIE_CHIP)
	void (*get_msi_range)(struct scsc_mif_abs *interface, u8 *start, u8 *end, enum scsc_mif_abs_target target);
	int (*hostif_wakeup)(struct scsc_mif_abs *interface, int (*cb)(void *first, void *secound), void *service, void *dev);
	int (*get_pcie_link_state)(struct scsc_mif_abs *interface);
#endif
#if defined(CONFIG_SCSC_RUNTIMEPM)
	int (*set_fw_runtime_pm)(struct scsc_mif_abs *interface, int runtime_pm);
#endif
/*The Overlay Remapper is responsible for adding an address offset to all (primary AXI port) transactions when
the main processor starts booting to avoid booting from (fixed) address 0x0 (From the view of WLBT
programmer). Cortex boot code can be remapped to any DRAM address. At the end of the boot sequence,
this remapping operation becomes irrelevant, as the processor can freely jump and execute from any DRAM
address (provided it is within a permissible BAAW range).*/
	void (*remap_set)(struct scsc_mif_abs *interface, uintptr_t remap_addr, enum scsc_mif_abs_target target);
#else
	u32  *(*get_mbox_ptr)(struct scsc_mif_abs *interface, u32 mbox_index);
/**
 * Incoming MIF Interrupt Hardware Controls
 */
	/** Get the incoming interrupt source mask */
	u32 (*irq_bit_mask_status_get)(struct scsc_mif_abs *interface);

	/** Get the incoming interrupt pending (waiting  *AND* not masked) mask */
	u32 (*irq_get)(struct scsc_mif_abs *interface);

	void (*irq_bit_clear)(struct scsc_mif_abs *interface, int bit_num);
	void (*irq_bit_mask)(struct scsc_mif_abs *interface, int bit_num);
	void (*irq_bit_unmask)(struct scsc_mif_abs *interface, int bit_num);

/**
 * Outgoing MIF Interrupt Hardware Controls
 */
	void (*irq_bit_set)(struct scsc_mif_abs *interface, int bit_num, enum scsc_mif_abs_target target);
#endif
/**
 * Register handler for the interrupt from the
 * MIF Interrupt Hardware.
 *
 * This is used by the MIF Interrupt Manager to
 * register a handler that demultiplexes the
 * individual interrupt sources (MIF Interrupt Bits)
 * to source-specific handlers.
 */
	void (*irq_reg_handler)(struct scsc_mif_abs *interface, void (*handler)(int irq, void *data), void *dev);
	void (*irq_unreg_handler)(struct scsc_mif_abs *interface);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
	void (*irq_reg_handler_wpan)(struct scsc_mif_abs *interface, void (*handler)(int irq, void *data), void *dev);
	void (*irq_unreg_handler_wpan)(struct scsc_mif_abs *interface);
#endif

	/* Clear HW interrupt line */
	void (*irq_clear)(void);
	void (*irq_reg_reset_request_handler)(struct scsc_mif_abs *interface, void (*handler)(int irq, void *data), void *dev);
	void (*irq_unreg_reset_request_handler)(struct scsc_mif_abs *interface);

/**
 * Install suspend/resume handlers for the MIF abstraction driver
 */
	void (*suspend_reg_handler)(struct scsc_mif_abs *abs,
				    int (*suspend)(struct scsc_mif_abs *abs, void *data),
				    void (*resume)(struct scsc_mif_abs *abs, void *data),
				    void *data);
	void (*suspend_unreg_handler)(struct scsc_mif_abs *abs);

/**
 * Return kernel-space pointer to MIF ram.
 * The pointer is guaranteed to remain valid between map and unmap calls.
 */
	void *(*get_mifram_ptr)(struct scsc_mif_abs *interface, scsc_mifram_ref ref);
/* Maps kernel-space pointer to MIF RAM to portable reference */
	int (*get_mifram_ref)(struct scsc_mif_abs *interface, void *ptr, scsc_mifram_ref *ref);
#if IS_ENABLED(CONFIG_SCSC_MEMLOG)
	void *(*get_mifram_ptr_region2)(struct scsc_mif_abs *interface, scsc_mifram_ref ref);
	int (*get_mifram_ref_region2)(struct scsc_mif_abs *interface, void *ptr, scsc_mifram_ref *ref);
	int (*set_mem_region2)(struct scsc_mif_abs *interface, void __iomem *_mem_region2, size_t _mem_size_region2);
	void (*set_memlog_paddr)(struct scsc_mif_abs *interface, dma_addr_t paddr);
#endif

/* Return physical page frame number corresponding to the physical addres to which
 * the virtual address is mapped . Needed in mmap file operations*/
	uintptr_t (*get_mifram_pfn)(struct scsc_mif_abs *interface);

/**
 * Return physical address from MIF ram address.
 */
	void      *(*get_mifram_phy_ptr)(struct scsc_mif_abs *interface, scsc_mifram_ref ref);
/** Return a kernel device associated 1:1 with the Maxwell instance.
 * This is published only for the purpose of associating service drivers with a Maxwell instance
 * for logging purposes. Clients should not make any assumptions about the device type.
 * In some configurations this may be the associated host-interface device (AXI/PCIe),
 * but this may change in future.
 */
	struct device *(*get_mif_device)(struct scsc_mif_abs *interface);


	void (*mif_dump_registers)(struct scsc_mif_abs *interface);
	void (*mif_cleanup)(struct scsc_mif_abs *interface);
	void (*mif_restart)(struct scsc_mif_abs *interface);

#ifdef CONFIG_SCSC_SMAPPER
/* SMAPPER */
	int  (*mif_smapper_get_mapping)(struct scsc_mif_abs *interface, u8 *phy_map, u16 *align);
	int  (*mif_smapper_get_bank_info)(struct scsc_mif_abs *interface, u8 bank, struct scsc_mif_smapper_info *bank_info);
	int  (*mif_smapper_write_sram)(struct scsc_mif_abs *interface, u8 bank, u8 num_entries, u8 first_entry, dma_addr_t *addr);
	void (*mif_smapper_configure)(struct scsc_mif_abs *interface, u32 granularity);
	u32  (*mif_smapper_get_bank_base_address)(struct scsc_mif_abs *interface, u8 bank);
#endif
#ifdef CONFIG_SCSC_QOS
	int  (*mif_pm_qos_add_request)(struct scsc_mif_abs *interface, struct scsc_mifqos_request *qos_req, enum scsc_qos_config config);
	int  (*mif_pm_qos_update_request)(struct scsc_mif_abs *interface, struct scsc_mifqos_request *qos_req, enum scsc_qos_config config);
	int  (*mif_pm_qos_remove_request)(struct scsc_mif_abs *interface, struct scsc_mifqos_request *qos_req);
#if defined(CONFIG_SCSC_PCIE_CHIP)
	int  (*mif_set_affinity_cpu)(struct scsc_mif_abs *interface, u8 msi, u8 cpu);
#else
	int  (*mif_set_affinity_cpu)(struct scsc_mif_abs *interface, u8 cpu);
#endif
#endif
	bool (*mif_reset_failure)(struct scsc_mif_abs *interface);
	int (*mif_read_register)(struct scsc_mif_abs *interface, u64 id, u32 *val);
#ifdef CONFIG_SOC_EXYNOS7885
/**
* Return scsc_btabox_data structure with physical address & size of the DTB region
* exposed by the platform driver. The platform driver uses it to configure BAAW1.
* The BT driver needs to know and pass it down to BT firmware to configure ABOX
* shared data structure
*/
	void (*get_abox_shared_mem)(struct scsc_mif_abs *interface, void **data);
#endif
/* To un/register callbacks to mxman functionality */
	void (*recovery_disabled_reg)(struct scsc_mif_abs *interface, bool (*handler)(void));
	void (*recovery_disabled_unreg)(struct scsc_mif_abs *interface);
#if defined(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
#if defined(CONFIG_SCSC_PCIE_CHIP)
	void __iomem *(*get_ramrp_ptr)(struct scsc_mif_abs *interface);
	int (*get_ramrp_buff)(struct scsc_mif_abs *interface, void **buff, int count, u64 offset);
#endif
#if IS_ENABLED(CONFIG_WLBT_PMU2AP_MBOX)
	void (*irq_pmu_bit_mask)(struct scsc_mif_abs *interface);
	void (*irq_pmu_bit_unmask)(struct scsc_mif_abs *interface);
#endif
	int (*get_mbox_pmu)(struct scsc_mif_abs *interface);
	int (*set_mbox_pmu)(struct scsc_mif_abs *interface, u32 val);
	int (*load_pmu_fw)(struct scsc_mif_abs *interface, u32 *ka_patch, size_t ka_patch_len);
#if IS_ENABLED(CONFIG_SOC_S5E5515) || IS_ENABLED(CONFIG_SCSC_PCIE_CHIP)
	int (*load_pmu_fw_flags)(struct scsc_mif_abs *interface, u32 *ka_patch, size_t ka_patch_len, u32 flags);
#endif
	void (*irq_reg_pmu_handler)(struct scsc_mif_abs *interface, void (*handler)(int irq, void *data), void *dev);

#if defined(CONFIG_SCSC_PCIE_CHIP)
	void (*irq_reg_pmu_error_handler)(struct scsc_mif_abs *interface, void (*handler)(int irq, void *data), void *dev);
	void (*irq_reg_pmu_pcie_handler)(struct scsc_mif_abs *interface, void (*handler)(int irq, void *data), void *dev);
	int (*get_mbox_pmu_pcie)(struct scsc_mif_abs *interface);
	int (*set_mbox_pmu_pcie)(struct scsc_mif_abs *interface, u32 val);
	int (*get_mbox_pmu_error)(struct scsc_mif_abs *interface);
#endif
#if defined(CONFIG_WLBT_DCXO_TUNE)
	void (*send_dcxo_cmd)(struct scsc_mif_abs *interface, u8 opcode, u32 val);
	int (*check_dcxo_ack)(struct scsc_mif_abs *interface, u8 opcode, u32* val);
	int (*irq_register_mbox_apm)(struct scsc_mif_abs *interface);
	void (*irq_unregister_mbox_apm)(struct scsc_mif_abs *interface);
#endif
#endif

	bool (*wlbt_property_read_bool)(struct scsc_mif_abs *interface, const char *propname);
	int (*wlbt_property_read_u8)(struct scsc_mif_abs *interface,
				     const char *propname, u8 *out_value, size_t size);
	int (*wlbt_property_read_u16)(struct scsc_mif_abs *interface,
				      const char *propname, u16 *out_value, size_t size);
	int (*wlbt_property_read_u32)(struct scsc_mif_abs *interface,
				      const char *propname, u32 *out_value, size_t size);
	int (*wlbt_property_read_string)(struct scsc_mif_abs *interface,
				      const char *propname, char **out_value, size_t size);
	int (*wlbt_phandle_property_read_u32) (struct scsc_mif_abs *interface, const char *phandle_name,
				      const char *propname, u32 *out_value, size_t size);

#if IS_ENABLED(CONFIG_SOC_S5E5515)
#define S615	0
#define S620	1
	void (*set_ldo_radio)(struct scsc_mif_abs *interface, u8 radio);
#endif
	void (*wlbt_regdump)(struct scsc_mif_abs *interface);
	void (*wlbt_karamdump)(struct scsc_mif_abs *interface);
#if defined(CONFIG_SCSC_PCIE_CHIP) || defined(CONFIG_WLBT_REFACTORY)
	void (*wlbt_irqdump)(struct scsc_mif_abs *interface);
#endif
#if defined(CONFIG_SCSC_BB_PAEAN)
	int (*acpm_write_reg)(struct scsc_mif_abs *interface, u8 reg, u8 value);
#elif defined(CONFIG_SCSC_BB_REDWOOD)
	void (*control_suspend_gpio)(struct scsc_mif_abs *interface, u8 value);
#endif

	bool (*get_scan2mem_mode)(struct scsc_mif_abs *interface);
	void (*set_scan2mem_mode)(struct scsc_mif_abs *interface, bool enable);
	u32 (*get_s2m_size_octets)(struct scsc_mif_abs *interface);
	void (*set_s2m_dram_offset)(struct scsc_mif_abs *intergace, u32 offset);

	unsigned long (*get_mem_start)(struct scsc_mif_abs *interface);
#if defined(CONFIG_SCSC_PCIE_CHIP)
	void (*reg_pcie_recovery_notifier)(struct scsc_mif_abs *interface, void (*notifier)(void));
	void (*unreg_pcie_recovery_notifier)(struct scsc_mif_abs *interface);
#endif
#if defined(CONFIG_SCSC_BB_REDWOOD)
	void (*mxman_force_panic_active)(void);
#endif
};

