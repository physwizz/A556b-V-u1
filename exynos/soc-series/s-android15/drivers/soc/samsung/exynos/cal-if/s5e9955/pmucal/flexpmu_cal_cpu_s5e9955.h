struct cpu_inform pmucal_cpuinform_list[] = {
	PMUCAL_CPU_INFORM(0, 0x13860000, 0xF4),
	PMUCAL_CPU_INFORM(1, 0x13860000, 0xF8),
	PMUCAL_CPU_INFORM(2, 0x13860000, 0xFC),
	PMUCAL_CPU_INFORM(3, 0x13860000, 0x100),
	PMUCAL_CPU_INFORM(4, 0x13860000, 0x104),
	PMUCAL_CPU_INFORM(5, 0x13860000, 0x108),
	PMUCAL_CPU_INFORM(6, 0x13860000, 0x10C),
	PMUCAL_CPU_INFORM(7, 0x13860000, 0x110),
	PMUCAL_CPU_INFORM(8, 0x13860000, 0x114),
	PMUCAL_CPU_INFORM(9, 0x13860000, 0x118),
};
unsigned int cpu_inform_list_size = ARRAY_SIZE(pmucal_cpuinform_list);
#ifndef ACPM_FRAMEWORK
/* individual sequence descriptor for each core - on, off, status */
struct pmucal_seq core00_on[] = {
};

struct pmucal_seq core00_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 0), (0x1 << 0), 0x13860000, 0x3018, (0x1 << 0), (0x1 << 0) | (0x1 << 0)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 10), (0x1 << 10), 0x13860000, 0x3018, (0x1 << 10), (0x1 << 10) | (0x1 << 10)),
};

struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_0", 0x2fc70000, 0x1538, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core01_on[] = {
};

struct pmucal_seq core01_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 1), (0x1 << 1), 0x13860000, 0x3018, (0x1 << 1), (0x1 << 1) | (0x1 << 1)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 11), (0x1 << 11), 0x13860000, 0x3018, (0x1 << 11), (0x1 << 11) | (0x1 << 11)),
};

struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_0", 0x2fc70000, 0x1538, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core10_on[] = {
};

struct pmucal_seq core10_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 2), (0x1 << 2), 0x13860000, 0x3018, (0x1 << 2), (0x1 << 2) | (0x1 << 2)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 12), (0x1 << 12), 0x13860000, 0x3018, (0x1 << 12), (0x1 << 12) | (0x1 << 12)),
};

struct pmucal_seq core10_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_1", 0x2fc70000, 0x1540, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core11_on[] = {
};

struct pmucal_seq core11_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 3), (0x1 << 3), 0x13860000, 0x3018, (0x1 << 3), (0x1 << 3) | (0x1 << 3)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 13), (0x1 << 13), 0x13860000, 0x3018, (0x1 << 13), (0x1 << 13) | (0x1 << 13)),
};

struct pmucal_seq core11_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_1", 0x2fc70000, 0x1540, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core12_on[] = {
};

struct pmucal_seq core12_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 4), (0x1 << 4), 0x13860000, 0x3018, (0x1 << 4), (0x1 << 4) | (0x1 << 4)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 14), (0x1 << 14), 0x13860000, 0x3018, (0x1 << 14), (0x1 << 14) | (0x1 << 14)),
};

struct pmucal_seq core12_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_2", 0x2fc70000, 0x1544, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core13_on[] = {
};

struct pmucal_seq core13_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 5), (0x1 << 5), 0x13860000, 0x3018, (0x1 << 5), (0x1 << 5) | (0x1 << 5)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 15), (0x1 << 15), 0x13860000, 0x3018, (0x1 << 15), (0x1 << 15) | (0x1 << 15)),
};

struct pmucal_seq core13_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_2", 0x2fc70000, 0x1544, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core14_on[] = {
};

struct pmucal_seq core14_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 6), (0x1 << 6), 0x13860000, 0x3018, (0x1 << 6), (0x1 << 6) | (0x1 << 6)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 16), (0x1 << 16), 0x13860000, 0x3018, (0x1 << 16), (0x1 << 16) | (0x1 << 16)),
};

struct pmucal_seq core14_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_3", 0x2fc70000, 0x1548, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core20_on[] = {
};

struct pmucal_seq core20_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 7), (0x1 << 7), 0x13860000, 0x3018, (0x1 << 7), (0x1 << 7) | (0x1 << 7)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 17), (0x1 << 17), 0x13860000, 0x3018, (0x1 << 17), (0x1 << 17) | (0x1 << 17)),
};

struct pmucal_seq core20_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_3", 0x2fc70000, 0x1548, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core21_on[] = {
};

struct pmucal_seq core21_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 8), (0x1 << 8), 0x13860000, 0x3018, (0x1 << 8), (0x1 << 8) | (0x1 << 8)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 18), (0x1 << 18), 0x13860000, 0x3018, (0x1 << 18), (0x1 << 18) | (0x1 << 18)),
};

struct pmucal_seq core21_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_4", 0x2fc70000, 0x154c, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core30_on[] = {
};

struct pmucal_seq core30_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 9), (0x1 << 9), 0x13860000, 0x3018, (0x1 << 9), (0x1 << 9) | (0x1 << 9)),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x13860000, 0x301c, (0x1 << 19), (0x1 << 19), 0x13860000, 0x3018, (0x1 << 19), (0x1 << 19) | (0x1 << 19)),
};

struct pmucal_seq core30_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_4", 0x2fc70000, 0x154c, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster0_on[] = {
};

struct pmucal_seq cluster0_off[] = {
};

struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x13860000, 0x1104, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster1_on[] = {
};

struct pmucal_seq cluster1_off[] = {
};

struct pmucal_seq cluster1_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_NONCPU_STATUS", 0x13860000, 0x1484, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster2_on[] = {
};

struct pmucal_seq cluster2_off[] = {
};

struct pmucal_seq cluster2_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_NONCPU_STATUS", 0x13860000, 0x1604, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster3_on[] = {
};

struct pmucal_seq cluster3_off[] = {
};

struct pmucal_seq cluster3_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER3_NONCPU_STATUS", 0x13860000, 0x1784, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	CPU_CORE10,
	CPU_CORE11,
	CPU_CORE12,
	CPU_CORE13,
	CPU_CORE14,
	CPU_CORE20,
	CPU_CORE21,
	CPU_CORE30,
	PMUCAL_NUM_CORES,
};

struct pmucal_cpu pmucal_cpu_list[PMUCAL_NUM_CORES] = {
	[CPU_CORE00] = {
		.id = CPU_CORE00,
		.release = 0,
		.on = core00_on,
		.off = core00_off,
		.status = core00_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core00_on),
		.num_off = ARRAY_SIZE(core00_off),
		.num_status = ARRAY_SIZE(core00_status),
	},
	[CPU_CORE01] = {
		.id = CPU_CORE01,
		.release = 0,
		.on = core01_on,
		.off = core01_off,
		.status = core01_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core01_on),
		.num_off = ARRAY_SIZE(core01_off),
		.num_status = ARRAY_SIZE(core01_status),
	},
	[CPU_CORE10] = {
		.id = CPU_CORE10,
		.release = 0,
		.on = core10_on,
		.off = core10_off,
		.status = core10_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core10_on),
		.num_off = ARRAY_SIZE(core10_off),
		.num_status = ARRAY_SIZE(core10_status),
	},
	[CPU_CORE11] = {
		.id = CPU_CORE11,
		.release = 0,
		.on = core11_on,
		.off = core11_off,
		.status = core11_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core11_on),
		.num_off = ARRAY_SIZE(core11_off),
		.num_status = ARRAY_SIZE(core11_status),
	},
	[CPU_CORE12] = {
		.id = CPU_CORE12,
		.release = 0,
		.on = core12_on,
		.off = core12_off,
		.status = core12_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core12_on),
		.num_off = ARRAY_SIZE(core12_off),
		.num_status = ARRAY_SIZE(core12_status),
	},
	[CPU_CORE13] = {
		.id = CPU_CORE13,
		.release = 0,
		.on = core13_on,
		.off = core13_off,
		.status = core13_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core13_on),
		.num_off = ARRAY_SIZE(core13_off),
		.num_status = ARRAY_SIZE(core13_status),
	},
	[CPU_CORE14] = {
		.id = CPU_CORE14,
		.release = 0,
		.on = core14_on,
		.off = core14_off,
		.status = core14_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core14_on),
		.num_off = ARRAY_SIZE(core14_off),
		.num_status = ARRAY_SIZE(core14_status),
	},
	[CPU_CORE20] = {
		.id = CPU_CORE20,
		.release = 0,
		.on = core20_on,
		.off = core20_off,
		.status = core20_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core20_on),
		.num_off = ARRAY_SIZE(core20_off),
		.num_status = ARRAY_SIZE(core20_status),
	},
	[CPU_CORE21] = {
		.id = CPU_CORE21,
		.release = 0,
		.on = core21_on,
		.off = core21_off,
		.status = core21_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core21_on),
		.num_off = ARRAY_SIZE(core21_off),
		.num_status = ARRAY_SIZE(core21_status),
	},
	[CPU_CORE30] = {
		.id = CPU_CORE30,
		.release = 0,
		.on = core30_on,
		.off = core30_off,
		.status = core30_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core30_on),
		.num_off = ARRAY_SIZE(core30_off),
		.num_status = ARRAY_SIZE(core30_status),
	},
};

unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	CPU_CLUSTER1,
	CPU_CLUSTER2,
	CPU_CLUSTER3,
	PMUCAL_NUM_CLUSTERS,
};

struct pmucal_cpu pmucal_cluster_list[PMUCAL_NUM_CLUSTERS] = {
	[CPU_CLUSTER0] = {
		.id = CPU_CLUSTER0,
		.on = cluster0_on,
		.off = cluster0_off,
		.status = cluster0_status,
		.num_on = ARRAY_SIZE(cluster0_on),
		.num_off = ARRAY_SIZE(cluster0_off),
		.num_status = ARRAY_SIZE(cluster0_status),
	},
	[CPU_CLUSTER1] = {
		.id = CPU_CLUSTER1,
		.on = cluster1_on,
		.off = cluster1_off,
		.status = cluster1_status,
		.num_on = ARRAY_SIZE(cluster1_on),
		.num_off = ARRAY_SIZE(cluster1_off),
		.num_status = ARRAY_SIZE(cluster1_status),
	},
	[CPU_CLUSTER2] = {
		.id = CPU_CLUSTER2,
		.on = cluster2_on,
		.off = cluster2_off,
		.status = cluster2_status,
		.num_on = ARRAY_SIZE(cluster2_on),
		.num_off = ARRAY_SIZE(cluster2_off),
		.num_status = ARRAY_SIZE(cluster2_status),
	},
	[CPU_CLUSTER3] = {
		.id = CPU_CLUSTER3,
		.on = cluster3_on,
		.off = cluster3_off,
		.status = cluster3_status,
		.num_on = ARRAY_SIZE(cluster3_on),
		.num_off = ARRAY_SIZE(cluster3_off),
		.num_status = ARRAY_SIZE(cluster3_status),
	},
};

unsigned int pmucal_cluster_list_size = ARRAY_SIZE(pmucal_cluster_list);

enum pmucal_opsnum {
	NUM_PMUCAL_OPTIONS,
};

struct pmucal_cpu pmucal_pmu_ops_list[] = {};

unsigned int pmucal_option_list_size = 0;

#else

struct pmucal_seq core00_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core00_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core00_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU0_STATUS", 0x13860000, 0x1004, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core01_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core01_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core01_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU1_STATUS", 0x13860000, 0x1084, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core10_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x30, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core10_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x31, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core10_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x32, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core10_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU0_STATUS", 0x13860000, 0x1184, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core11_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x30, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core11_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x31, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core11_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x32, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core11_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU1_STATUS", 0x13860000, 0x1204, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core12_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x30, 0x2, 0, 0, 0, 0),
};

struct pmucal_seq core12_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x31, 0x2, 0, 0, 0, 0),
};

struct pmucal_seq core12_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x32, 0x2, 0, 0, 0, 0),
};

struct pmucal_seq core12_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU2_STATUS", 0x13860000, 0x1284, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core13_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x30, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core13_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x31, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core13_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x32, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core13_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU3_STATUS", 0x13860000, 0x1304, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core14_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x30, 0x4, 0, 0, 0, 0),
};

struct pmucal_seq core14_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x31, 0x4, 0, 0, 0, 0),
};

struct pmucal_seq core14_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x32, 0x4, 0, 0, 0, 0),
};

struct pmucal_seq core14_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU4_STATUS", 0x13860000, 0x1384, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core20_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_reset_release", 0, 0, 0x30, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core20_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_up", 0, 0, 0x31, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core20_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_down", 0, 0, 0x32, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core20_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_CPU0_STATUS", 0x13860000, 0x1504, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core21_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_reset_release", 0, 0, 0x40, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core21_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_up", 0, 0, 0x41, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core21_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_down", 0, 0, 0x42, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core21_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_CPU1_STATUS", 0x13860000, 0x1584, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core30_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster3_cpu_reset_release", 0, 0, 0x50, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core30_on[] = {
};

struct pmucal_seq core30_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster3_cpu_down", 0, 0, 0x52, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core30_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER3_CPU0_STATUS", 0x13860000, 0x1704, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster0_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_noncpu_up", 0, 0, 0x28, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster0_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_noncpu_down", 0, 0, 0x29, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x13860000, 0x1104, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster1_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_noncpu_up", 0, 0, 0x38, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster1_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_noncpu_down", 0, 0, 0x39, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster1_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_NONCPU_STATUS", 0x13860000, 0x1484, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster2_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_noncpu_up", 0, 0, 0x48, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster2_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_noncpu_down", 0, 0, 0x49, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster2_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_NONCPU_STATUS", 0x13860000, 0x1604, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster3_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster3_noncpu_up", 0, 0, 0x58, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster3_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster3_noncpu_down", 0, 0, 0x59, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster3_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER3_NONCPU_STATUS", 0x13860000, 0x1784, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	CPU_CORE10,
	CPU_CORE11,
	CPU_CORE12,
	CPU_CORE13,
	CPU_CORE14,
	CPU_CORE20,
	CPU_CORE21,
	CPU_CORE30,
	PMUCAL_NUM_CORES,
};

struct pmucal_cpu pmucal_cpu_list[PMUCAL_NUM_CORES] = {
	[CPU_CORE00] = {
		.id = CPU_CORE00,
		.release = core00_release,
		.on = core00_on,
		.off = core00_off,
		.status = core00_status,
		.num_release = ARRAY_SIZE(core00_release),
		.num_on = ARRAY_SIZE(core00_on),
		.num_off = ARRAY_SIZE(core00_off),
		.num_status = ARRAY_SIZE(core00_status),
	},
	[CPU_CORE01] = {
		.id = CPU_CORE01,
		.release = core01_release,
		.on = core01_on,
		.off = core01_off,
		.status = core01_status,
		.num_release = ARRAY_SIZE(core01_release),
		.num_on = ARRAY_SIZE(core01_on),
		.num_off = ARRAY_SIZE(core01_off),
		.num_status = ARRAY_SIZE(core01_status),
	},
	[CPU_CORE10] = {
		.id = CPU_CORE10,
		.release = core10_release,
		.on = core10_on,
		.off = core10_off,
		.status = core10_status,
		.num_release = ARRAY_SIZE(core10_release),
		.num_on = ARRAY_SIZE(core10_on),
		.num_off = ARRAY_SIZE(core10_off),
		.num_status = ARRAY_SIZE(core10_status),
	},
	[CPU_CORE11] = {
		.id = CPU_CORE11,
		.release = core11_release,
		.on = core11_on,
		.off = core11_off,
		.status = core11_status,
		.num_release = ARRAY_SIZE(core11_release),
		.num_on = ARRAY_SIZE(core11_on),
		.num_off = ARRAY_SIZE(core11_off),
		.num_status = ARRAY_SIZE(core11_status),
	},
	[CPU_CORE12] = {
		.id = CPU_CORE12,
		.release = core12_release,
		.on = core12_on,
		.off = core12_off,
		.status = core12_status,
		.num_release = ARRAY_SIZE(core12_release),
		.num_on = ARRAY_SIZE(core12_on),
		.num_off = ARRAY_SIZE(core12_off),
		.num_status = ARRAY_SIZE(core12_status),
	},
	[CPU_CORE13] = {
		.id = CPU_CORE13,
		.release = core13_release,
		.on = core13_on,
		.off = core13_off,
		.status = core13_status,
		.num_release = ARRAY_SIZE(core13_release),
		.num_on = ARRAY_SIZE(core13_on),
		.num_off = ARRAY_SIZE(core13_off),
		.num_status = ARRAY_SIZE(core13_status),
	},
	[CPU_CORE14] = {
		.id = CPU_CORE14,
		.release = core14_release,
		.on = core14_on,
		.off = core14_off,
		.status = core14_status,
		.num_release = ARRAY_SIZE(core14_release),
		.num_on = ARRAY_SIZE(core14_on),
		.num_off = ARRAY_SIZE(core14_off),
		.num_status = ARRAY_SIZE(core14_status),
	},
	[CPU_CORE20] = {
		.id = CPU_CORE20,
		.release = core20_release,
		.on = core20_on,
		.off = core20_off,
		.status = core20_status,
		.num_release = ARRAY_SIZE(core20_release),
		.num_on = ARRAY_SIZE(core20_on),
		.num_off = ARRAY_SIZE(core20_off),
		.num_status = ARRAY_SIZE(core20_status),
	},
	[CPU_CORE21] = {
		.id = CPU_CORE21,
		.release = core21_release,
		.on = core21_on,
		.off = core21_off,
		.status = core21_status,
		.num_release = ARRAY_SIZE(core21_release),
		.num_on = ARRAY_SIZE(core21_on),
		.num_off = ARRAY_SIZE(core21_off),
		.num_status = ARRAY_SIZE(core21_status),
	},
	[CPU_CORE30] = {
		.id = CPU_CORE30,
		.release = core30_release,
		.on = core30_on,
		.off = core30_off,
		.status = core30_status,
		.num_release = ARRAY_SIZE(core30_release),
		.num_on = ARRAY_SIZE(core30_on),
		.num_off = ARRAY_SIZE(core30_off),
		.num_status = ARRAY_SIZE(core30_status),
	},
};

unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	CPU_CLUSTER1,
	CPU_CLUSTER2,
	CPU_CLUSTER3,
	PMUCAL_NUM_CLUSTERS,
};

struct pmucal_cpu pmucal_cluster_list[PMUCAL_NUM_CLUSTERS] = {
	[CPU_CLUSTER0] = {
		.id = CPU_CLUSTER0,
		.on = cluster0_on,
		.off = cluster0_off,
		.status = cluster0_status,
		.num_on = ARRAY_SIZE(cluster0_on),
		.num_off = ARRAY_SIZE(cluster0_off),
		.num_status = ARRAY_SIZE(cluster0_status),
	},
	[CPU_CLUSTER1] = {
		.id = CPU_CLUSTER1,
		.on = cluster1_on,
		.off = cluster1_off,
		.status = cluster1_status,
		.num_on = ARRAY_SIZE(cluster1_on),
		.num_off = ARRAY_SIZE(cluster1_off),
		.num_status = ARRAY_SIZE(cluster1_status),
	},
	[CPU_CLUSTER2] = {
		.id = CPU_CLUSTER2,
		.on = cluster2_on,
		.off = cluster2_off,
		.status = cluster2_status,
		.num_on = ARRAY_SIZE(cluster2_on),
		.num_off = ARRAY_SIZE(cluster2_off),
		.num_status = ARRAY_SIZE(cluster2_status),
	},
	[CPU_CLUSTER3] = {
		.id = CPU_CLUSTER3,
		.on = cluster3_on,
		.off = cluster3_off,
		.status = cluster3_status,
		.num_on = ARRAY_SIZE(cluster3_on),
		.num_off = ARRAY_SIZE(cluster3_off),
		.num_status = ARRAY_SIZE(cluster3_status),
	},
};

unsigned int pmucal_cluster_list_size = ARRAY_SIZE(pmucal_cluster_list);

enum pmucal_opsnum {
	NUM_PMUCAL_OPTIONS,
};

struct pmucal_cpu pmucal_pmu_ops_list[NUM_PMUCAL_OPTIONS] = {
};
unsigned int pmucal_option_list_size = ARRAY_SIZE(pmucal_pmu_ops_list);

struct cpu_info cpuinfo[] = {
	[0] = {
		.min = 0,
		.max = 1,
		.total = 2,
	},
	[1] = {
		.min = 2,
		.max = 6,
		.total = 5,
	},
	[2] = {
		.min = 7,
		.max = 8,
		.total = 2,
	},
	[3] = {
		.min = 9,
		.max = 9,
		.total = 1,
	},
};
#endif