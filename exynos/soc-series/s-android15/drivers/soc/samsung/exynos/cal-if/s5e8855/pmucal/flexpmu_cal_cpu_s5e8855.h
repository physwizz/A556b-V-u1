struct cpu_inform pmucal_cpuinform_list[] = {
	PMUCAL_CPU_INFORM(0, 0x11860000, 0xF4),
	PMUCAL_CPU_INFORM(1, 0x11860000, 0xF8),
	PMUCAL_CPU_INFORM(2, 0x11860000, 0xFC),
	PMUCAL_CPU_INFORM(3, 0x11860000, 0x100),
	PMUCAL_CPU_INFORM(4, 0x11860000, 0x104),
	PMUCAL_CPU_INFORM(5, 0x11860000, 0x108),
	PMUCAL_CPU_INFORM(6, 0x11860000, 0x10C),
	PMUCAL_CPU_INFORM(7, 0x11860000, 0x110),
};
unsigned int cpu_inform_list_size = ARRAY_SIZE(pmucal_cpuinform_list);
#ifndef ACPM_FRAMEWORK
/* individual sequence descriptor for each core - on, off, status */
struct pmucal_seq core00_on[] = {
};

struct pmucal_seq core00_off[] = {
};

struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_0", 0x18800000, 0x0478, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core01_on[] = {
};

struct pmucal_seq core01_off[] = {
};

struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_0", 0x18800000, 0x0478, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core02_on[] = {
};

struct pmucal_seq core02_off[] = {
};

struct pmucal_seq core02_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_1", 0x18800000, 0x047c, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core03_on[] = {
};

struct pmucal_seq core03_off[] = {
};

struct pmucal_seq core03_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_1", 0x18800000, 0x047c, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core10_on[] = {
};

struct pmucal_seq core10_off[] = {
};

struct pmucal_seq core10_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_2", 0x18800000, 0x0480, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core11_on[] = {
};

struct pmucal_seq core11_off[] = {
};

struct pmucal_seq core11_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_2", 0x18800000, 0x0480, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core12_on[] = {
};

struct pmucal_seq core12_off[] = {
};

struct pmucal_seq core12_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_3", 0x18800000, 0x0484, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core20_on[] = {
};

struct pmucal_seq core20_off[] = {
};

struct pmucal_seq core20_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CORE_PPUHWSTAT_3", 0x18800000, 0x0484, (0x1 << 16), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster0_on[] = {
};

struct pmucal_seq cluster0_off[] = {
};

struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x11860000, 0x1204, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster1_on[] = {
};

struct pmucal_seq cluster1_off[] = {
};

struct pmucal_seq cluster1_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_NONCPU_STATUS", 0x11860000, 0x1504, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster2_on[] = {
};

struct pmucal_seq cluster2_off[] = {
};

struct pmucal_seq cluster2_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_NONCPU_STATUS", 0x11860000, 0x1704, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	CPU_CORE02,
	CPU_CORE03,
	CPU_CORE10,
	CPU_CORE11,
	CPU_CORE12,
	CPU_CORE20,
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
	[CPU_CORE02] = {
		.id = CPU_CORE02,
		.release = 0,
		.on = core02_on,
		.off = core02_off,
		.status = core02_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core02_on),
		.num_off = ARRAY_SIZE(core02_off),
		.num_status = ARRAY_SIZE(core02_status),
	},
	[CPU_CORE03] = {
		.id = CPU_CORE03,
		.release = 0,
		.on = core03_on,
		.off = core03_off,
		.status = core03_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core03_on),
		.num_off = ARRAY_SIZE(core03_off),
		.num_status = ARRAY_SIZE(core03_status),
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
};

unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	CPU_CLUSTER1,
	CPU_CLUSTER2,
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
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU0_STATUS", 0x11860000, 0x1004, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
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
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU1_STATUS", 0x11860000, 0x1084, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core02_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x2, 0, 0, 0, 0),
};

struct pmucal_seq core02_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x2, 0, 0, 0, 0),
};

struct pmucal_seq core02_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x2, 0, 0, 0, 0),
};

struct pmucal_seq core02_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU2_STATUS", 0x11860000, 0x1104, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core03_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core03_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core03_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core03_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU3_STATUS", 0x11860000, 0x1184, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core10_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x20, 0x4, 0, 0, 0, 0),
};

struct pmucal_seq core10_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x21, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core10_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x22, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq core10_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU0_STATUS", 0x11860000, 0x1304, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core11_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x20, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core11_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x21, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core11_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x22, 0x1, 0, 0, 0, 0),
};

struct pmucal_seq core11_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU1_STATUS", 0x11860000, 0x1384, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
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
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU2_STATUS", 0x11860000, 0x1404, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq core13_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_reset_release", 0, 0, 0x30, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core13_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_up", 0, 0, 0x31, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core13_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_down", 0, 0, 0x32, 0x3, 0, 0, 0, 0),
};

struct pmucal_seq core13_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_CPU0_STATUS", 0x11860000, 0x1604, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster0_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_noncpu_up", 0, 0, 0x28, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster0_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_noncpu_down", 0, 0, 0x29, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x11860000, 0x1204, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster1_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_noncpu_up", 0, 0, 0x38, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster1_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_noncpu_down", 0, 0, 0x39, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster1_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_NONCPU_STATUS", 0x11860000, 0x1504, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

struct pmucal_seq cluster1_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_noncpu_up", 0, 0, 0x38, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster2_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_noncpu_down", 0, 0, 0x39, 0x0, 0, 0, 0, 0),
};

struct pmucal_seq cluster2_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_NONCPU_STATUS", 0x11860000, 0x1704, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};

enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	CPU_CORE02,
	CPU_CORE03,
	CPU_CORE10,
	CPU_CORE11,
	CPU_CORE12,
	CPU_CORE20,
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
	[CPU_CORE02] = {
		.id = CPU_CORE02,
		.release = core02_release,
		.on = core02_on,
		.off = core02_off,
		.status = core02_status,
		.num_release = ARRAY_SIZE(core02_release),
		.num_on = ARRAY_SIZE(core02_on),
		.num_off = ARRAY_SIZE(core02_off),
		.num_status = ARRAY_SIZE(core02_status),
	},
	[CPU_CORE03] = {
		.id = CPU_CORE03,
		.release = core03_release,
		.on = core03_on,
		.off = core03_off,
		.status = core03_status,
		.num_release = ARRAY_SIZE(core03_release),
		.num_on = ARRAY_SIZE(core03_on),
		.num_off = ARRAY_SIZE(core03_off),
		.num_status = ARRAY_SIZE(core03_status),
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
};

unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	CPU_CLUSTER1,
	CPU_CLUSTER2,
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
		.max = 3,
		.total = 4,
	},
	[1] = {
		.min = 4,
		.max = 6,
		.total = 3,
	},
	[2] = {
		.min = 7,
		.max = 7,
		.total = 1,
	},
};
#endif