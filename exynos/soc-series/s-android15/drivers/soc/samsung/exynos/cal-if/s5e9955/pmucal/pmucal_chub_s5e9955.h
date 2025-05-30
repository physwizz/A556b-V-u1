/* individual sequence descriptor for chub control - chub blk on, reset(config, cpu), release(config), release(cpu) */
struct pmucal_seq chub_blk_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_SET_BIT_ATOMIC, "CHUB_OUT", 0x13860000, 0x19e0, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_SET_BIT_ATOMIC, "CHUB_OUT", 0x13860000, 0x19e0, (0xffffffff << 0), (0x1f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_SET_BIT_ATOMIC_SUB, "CTRL", 0x2d821000, 0x0014, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_SET_BIT_ATOMIC_SUB, "STATE_CONFIGURATION", 0x2d821000, 0x0000, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "STATUS", 0x2d821000, 0x0008, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_7", 0x2d821000, 0x011c, (0xffffffff << 0), (0x410c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_8", 0x2d821000, 0x0120, (0xffffffff << 0), (0x410d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_9", 0x2d821000, 0x0124, (0xffffffff << 0), (0x410e << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_10", 0x2d821000, 0x0128, (0xffffffff << 0), (0x410f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_11", 0x2d821000, 0x012c, (0xffffffff << 0), (0x9004 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_12", 0x2d821000, 0x0130, (0xffffffff << 0), (0x111 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_13", 0x2d821000, 0x0134, (0xffffffff << 0), (0x109 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_14", 0x2d821000, 0x0138, (0xffffffff << 0), (0x9002 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_15", 0x2d821000, 0x013c, (0xffffffff << 0), (0x110 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_16", 0x2d821000, 0x0140, (0xffffffff << 0), (0x8 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_17", 0x2d821000, 0x0144, (0xffffffff << 0), (0x7 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_18", 0x2d821000, 0x0148, (0xffffffff << 0), (0x9014 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_19", 0x2d821000, 0x014c, (0xffffffff << 0), (0x2000 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_20", 0x2d821000, 0x0150, (0xffffffff << 0), (0x6 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_21", 0x2d821000, 0x0154, (0xffffffff << 0), (0x4 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_22", 0x2d821000, 0x0158, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_23", 0x2d821000, 0x015c, (0xffffffff << 0), (0xd << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_24", 0x2d821000, 0x0160, (0xffffffff << 0), (0x5 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_25", 0x2d821000, 0x0164, (0xffffffff << 0), (0x9004 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_26", 0x2d821000, 0x0168, (0xffffffff << 0), (0x16 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_27", 0x2d821000, 0x016c, (0xffffffff << 0), (0x810a << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_28", 0x2d821000, 0x0170, (0xffffffff << 0), (0x810b << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_29", 0x2d821000, 0x0174, (0xffffffff << 0), (0xf11f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_12", 0x2d821000, 0x01b0, (0xffffffff << 0), (0x400c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_13", 0x2d821000, 0x01b4, (0xffffffff << 0), (0x400d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_14", 0x2d821000, 0x01b8, (0xffffffff << 0), (0x400e << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_15", 0x2d821000, 0x01bc, (0xffffffff << 0), (0x400f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_16", 0x2d821000, 0x01c0, (0xffffffff << 0), (0x9014 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_17", 0x2d821000, 0x01c4, (0xffffffff << 0), (0x10d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_18", 0x2d821000, 0x01c8, (0xffffffff << 0), (0x8115 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_19", 0x2d821000, 0x01cc, (0xffffffff << 0), (0x8015 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_20", 0x2d821000, 0x01d0, (0xffffffff << 0), (0x106 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_21", 0x2d821000, 0x01d4, (0xffffffff << 0), (0x100 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_22", 0x2d821000, 0x01d8, (0xffffffff << 0), (0x104 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_23", 0x2d821000, 0x01dc, (0xffffffff << 0), (0x9013 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_24", 0x2d821000, 0x01e0, (0xffffffff << 0), (0x9011 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_25", 0x2d821000, 0x01e4, (0xffffffff << 0), (0x114 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_26", 0x2d821000, 0x01e8, (0xffffffff << 0), (0x800c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_27", 0x2d821000, 0x01ec, (0xffffffff << 0), (0x9004 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_28", 0x2d821000, 0x01f0, (0xffffffff << 0), (0x9010 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_29", 0x2d821000, 0x01f4, (0xffffffff << 0), (0x901d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_30", 0x2d821000, 0x01f8, (0xffffffff << 0), (0x2100 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_31", 0x2d821000, 0x01fc, (0xffffffff << 0), (0xf11f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "DENY_AND_UP", 0x2d821000, 0x0028, (0xffffffff << 0), (0x1c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MASK_DOWN_SEQ", 0x2d821000, 0x0020, (0xffffffff << 0), (0x3b40012 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MASK_UP_SEQ", 0x2d821000, 0x001c, (0xffffffff << 0), (0x105e0020 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEM_OUT", 0x2d821000, 0x0050, (0xf << 26), (0xf << 26), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEM_OUT", 0x2d821000, 0x0050, (0xf << 0), (0xf << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEM_OUT", 0x2d821000, 0x0050, (0xf << 16), (0xf << 16), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CHUB_STATUS", 0x13860000, 0x19c4, (0xffffffff << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
    PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CHUB_INT_EN/WAKEUP_REQ", 0x13860000, 0x19ec, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_CODE_CHUB/R0SIZE0", 0x14820000, 0x0004, (0x3ff << 0), (0x0 << 0),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_CODE_CHUB/R0SIZE1", 0x14820000, 0x0004, (0x3ff << 16), (0x0 << 16),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_DATA0_CHUB/R0SIZE0", 0x14850000, 0x0004, (0x3ff << 0), (0x0 << 0),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_DATA0_CHUB/R0SIZE1", 0x14850000, 0x0004, (0x3ff << 16), (0x0 << 16),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_DATA1_CHUB/R0SIZE0", 0x14870000, 0x0004, (0x3ff << 0), (0x0 << 0),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_DATA1_CHUB/R0SIZE1", 0x14870000, 0x0004, (0x3ff << 16), (0x0 << 16),0,0,0xffffffff,0),
};

struct pmucal_seq chub_reset_assert[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLR_BIT_ATOMIC_SUB, "CTRL", 0x2d821000, 0x0014, (0xffffffff << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "STATES", 0x2d821000, 0x000c, (0xff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_0", 0x2d821000, 0x0100, (0xffffffff << 0), (0x9009 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_11", 0x2d821000, 0x012c, (0xffffffff << 0), (0x401f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_12", 0x2d821000, 0x0130, (0xffffffff << 0), (0x9004 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_13", 0x2d821000, 0x0134, (0xffffffff << 0), (0x111 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_14", 0x2d821000, 0x0138, (0xffffffff << 0), (0x109 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_15", 0x2d821000, 0x013c, (0xffffffff << 0), (0x9002 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_16", 0x2d821000, 0x0140, (0xffffffff << 0), (0x110 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_17", 0x2d821000, 0x0144, (0xffffffff << 0), (0x8 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_18", 0x2d821000, 0x0148, (0xffffffff << 0), (0x7 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_19", 0x2d821000, 0x014c, (0xffffffff << 0), (0x9014 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_20", 0x2d821000, 0x0150, (0xffffffff << 0), (0x2000 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_21", 0x2d821000, 0x0154, (0xffffffff << 0), (0x6 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_22", 0x2d821000, 0x0158, (0xffffffff << 0), (0x4 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_23", 0x2d821000, 0x015c, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_24", 0x2d821000, 0x0160, (0xffffffff << 0), (0xd << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_25", 0x2d821000, 0x0164, (0xffffffff << 0), (0x5 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_26", 0x2d821000, 0x0168, (0xffffffff << 0), (0x9004 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_27", 0x2d821000, 0x016c, (0xffffffff << 0), (0x16 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_28", 0x2d821000, 0x0170, (0xffffffff << 0), (0x810a << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_29", 0x2d821000, 0x0174, (0xffffffff << 0), (0x810b << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_30", 0x2d821000, 0x0178, (0xffffffff << 0), (0xf11f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_12", 0x2d821000, 0x01b0, (0xffffffff << 0), (0x411f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_13", 0x2d821000, 0x01b4, (0xffffffff << 0), (0x400c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_14", 0x2d821000, 0x01b8, (0xffffffff << 0), (0x400d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_15", 0x2d821000, 0x01bc, (0xffffffff << 0), (0x400e << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_16", 0x2d821000, 0x01c0, (0xffffffff << 0), (0x400f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_28", 0x2d821000, 0x01f0, (0xffffffff << 0), (0x9019 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MASK_DOWN_SEQ", 0x2d821000, 0x0020, (0xffffffff << 0), (0x040E1012 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MASK_UP_SEQ", 0x2d821000, 0x001c, (0xffffffff << 0), (0x280000c0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPU_OUT", 0x2d821000, 0x0040, (0xff << 4), (0x0 << 4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CHUB_CONTROLLER_OPTION0_CMU_CTRL", 0x14800000, 0x800, (0x1 << 24), (0x0 << 24), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "QCH_CON_YAMIN_MCU_CHUB_QCH_CLKIN", 0x14800000, 0x30fc, (0x7f << 0), (0x74 << 0), 0x2d821000, 0x2c, (0x0 << 0), (0x0 << 0)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEM_OUT", 0x2d821000, 0x0050, (0xf << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEM_OUT", 0x2d821000, 0x0050, (0xf << 16), (0x0 << 16), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPU_OPTION", 0x2d821000, 0x002c, (0xf << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLR_BIT_ATOMIC_SUB, "STATE_CONFIGURATION", 0x2d821000, 0x0000, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "STATUS", 0x2d821000, 0x0008, (0x1 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CHUB_STATUS", 0x13860000, 0x19c4, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_SET_BIT_ATOMIC_SUB, "STATE_CONFIGURATION", 0x2d821000, 0x0000, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "STATUS", 0x2d821000, 0x0008, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CHUB_STATUS", 0x13860000, 0x19c4, (0xffffffff << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_0", 0x2d821000, 0x0100, (0xffffffff << 0), (0x9000 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_7", 0x2d821000, 0x011c, (0xffffffff << 0), (0x410c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_8", 0x2d821000, 0x0120, (0xffffffff << 0), (0x410d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_9", 0x2d821000, 0x0124, (0xffffffff << 0), (0x410e << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_10", 0x2d821000, 0x0128, (0xffffffff << 0), (0x410f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_11", 0x2d821000, 0x012c, (0xffffffff << 0), (0x9004 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_12", 0x2d821000, 0x0130, (0xffffffff << 0), (0x111 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_13", 0x2d821000, 0x0134, (0xffffffff << 0), (0x109 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_14", 0x2d821000, 0x0138, (0xffffffff << 0), (0x9002 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_15", 0x2d821000, 0x013c, (0xffffffff << 0), (0x110 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_16", 0x2d821000, 0x0140, (0xffffffff << 0), (0x8 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_17", 0x2d821000, 0x0144, (0xffffffff << 0), (0x7 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_18", 0x2d821000, 0x0148, (0xffffffff << 0), (0x9014 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_19", 0x2d821000, 0x014c, (0xffffffff << 0), (0x2000 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_20", 0x2d821000, 0x0150, (0xffffffff << 0), (0x6 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_21", 0x2d821000, 0x0154, (0xffffffff << 0), (0x4 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_22", 0x2d821000, 0x0158, (0xffffffff << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_23", 0x2d821000, 0x015c, (0xffffffff << 0), (0xd << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_24", 0x2d821000, 0x0160, (0xffffffff << 0), (0x5 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_25", 0x2d821000, 0x0164, (0xffffffff << 0), (0x9004 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_26", 0x2d821000, 0x0168, (0xffffffff << 0), (0x16 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_27", 0x2d821000, 0x016c, (0xffffffff << 0), (0x810a << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_28", 0x2d821000, 0x0170, (0xffffffff << 0), (0x810b << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_DOWN_29", 0x2d821000, 0x0174, (0xffffffff << 0), (0xf11f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_12", 0x2d821000, 0x01b0, (0xffffffff << 0), (0x400c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_13", 0x2d821000, 0x01b4, (0xffffffff << 0), (0x400d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_14", 0x2d821000, 0x01b8, (0xffffffff << 0), (0x400e << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_15", 0x2d821000, 0x01bc, (0xffffffff << 0), (0x400f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_16", 0x2d821000, 0x01c0, (0xffffffff << 0), (0x9014 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_17", 0x2d821000, 0x01c4, (0xffffffff << 0), (0x10d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_18", 0x2d821000, 0x01c8, (0xffffffff << 0), (0x8115 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_19", 0x2d821000, 0x01cc, (0xffffffff << 0), (0x8015 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_20", 0x2d821000, 0x01d0, (0xffffffff << 0), (0x106 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_21", 0x2d821000, 0x01d4, (0xffffffff << 0), (0x100 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_22", 0x2d821000, 0x01d8, (0xffffffff << 0), (0x104 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_23", 0x2d821000, 0x01dc, (0xffffffff << 0), (0x9013 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_24", 0x2d821000, 0x01e0, (0xffffffff << 0), (0x9011 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_25", 0x2d821000, 0x01e4, (0xffffffff << 0), (0x114 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_26", 0x2d821000, 0x01e8, (0xffffffff << 0), (0x800c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_27", 0x2d821000, 0x01ec, (0xffffffff << 0), (0x9004 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_28", 0x2d821000, 0x01f0, (0xffffffff << 0), (0x9010 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_29", 0x2d821000, 0x01f4, (0xffffffff << 0), (0x901d << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_30", 0x2d821000, 0x01f8, (0xffffffff << 0), (0x2100 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "STATE_UP_31", 0x2d821000, 0x01fc, (0xffffffff << 0), (0xf11f << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "DENY_AND_UP", 0x2d821000, 0x0028, (0xffffffff << 0), (0x1c << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MASK_DOWN_SEQ", 0x2d821000, 0x0020, (0xffffffff << 0), (0x3b40012 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MASK_UP_SEQ", 0x2d821000, 0x001c, (0xffffffff << 0), (0x105e0020 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEM_OUT", 0x2d821000, 0x0050, (0xf << 26), (0xf << 26), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEM_OUT", 0x2d821000, 0x0050, (0xf << 0), (0xf << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MEM_OUT", 0x2d821000, 0x0050, (0xf << 16), (0xf << 16), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_CODE_CHUB/R0SIZE0", 0x14820000, 0x0004, (0x3ff << 0), (0x0 << 0),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_CODE_CHUB/R0SIZE1", 0x14820000, 0x0004, (0x3ff << 16), (0x0 << 16),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_DATA0_CHUB/R0SIZE0", 0x14850000, 0x0004, (0x3ff << 0), (0x0 << 0),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_DATA0_CHUB/R0SIZE1", 0x14850000, 0x0004, (0x3ff << 16), (0x0 << 16),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_DATA1_CHUB/R0SIZE0", 0x14870000, 0x0004, (0x3ff << 0), (0x0 << 0),0,0,0xffffffff,0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "INTMEM_DATA1_CHUB/R0SIZE1", 0x14870000, 0x0004, (0x3ff << 16), (0x0 << 16),0,0,0xffffffff,0),
};

struct pmucal_seq chub_reset_release_config[] = {
};

struct pmucal_seq chub_reset_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_CHUB/CPU_OPTION", 0x2D821000, 0x2c, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_CHUB/CPU_OUT", 0x2D821000, 0x40, (0x1 << 4), (0x1 << 4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_CHUB/CPU_OUT", 0x2D821000, 0x40, (0x1 << 8), (0x1 << 8), 0, 0, 0xffffffff, 0),
};

struct pmucal_chub pmucal_chub_list = {
		.on = chub_blk_on,
		.reset_assert = chub_reset_assert,
		.reset_release_config = chub_reset_release_config,
		.reset_release = chub_reset_release,
		.num_on = ARRAY_SIZE(chub_blk_on),
		.num_reset_assert = ARRAY_SIZE(chub_reset_assert),
		.num_reset_release_config = ARRAY_SIZE(chub_reset_release_config),
		.num_reset_release = ARRAY_SIZE(chub_reset_release),
};
unsigned int pmucal_chub_list_size = 1;
