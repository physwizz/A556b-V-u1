
#ifndef DEBUG_SNAPSHOT_LOG_H
#define DEBUG_SNAPSHOT_LOG_H

#include <linux/clk-provider.h>
#include <soc/samsung/exynos/debug-snapshot.h>

/*  Length domain */
#define DSS_LOG_STRING_LEN		SZ_128

#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ_DOMAIN_NUM
#define DSS_DOMAIN_NUM			CONFIG_DEBUG_SNAPSHOT_FREQ_DOMAIN_NUM
#else
#define DSS_DOMAIN_NUM			32
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_LOG_ITEM_SIZE
#define DSS_LOG_MAX_NUM			CONFIG_DEBUG_SNAPSHOT_LOG_ITEM_SIZE
#else
#define DSS_LOG_MAX_NUM			1024
#endif
#define DSS_CALLSTACK_MAX_NUM		3
#define TASK_COMM_LEN 			16

#ifndef CONFIG_DEBUG_SNAPSHOT_NR_CPUS
#define DSS_NR_CPUS			8
#else
#define DSS_NR_CPUS			CONFIG_DEBUG_SNAPSHOT_NR_CPUS
#endif

struct task_log {
	unsigned long long time;
	struct task_struct *task;
	char task_comm[TASK_COMM_LEN];
	int pid;
};

struct work_log {
	unsigned long long time;
	struct worker *worker;
	work_func_t fn;
	char task_comm[TASK_COMM_LEN];
	int en;
};

struct cpuidle_log {
	unsigned long long time;
	char *modes;
	unsigned int state;
	u32 num_online_cpus;
	int delta;
	int en;
};

struct suspend_log {
	unsigned long long time;
	const char *log;
	const char *dev;
	int en;
	int event;
	int core;
};

struct irq_log {
	unsigned long long time;
	int irq;
	void *fn;
	struct irq_desc *desc;
	unsigned long long latency;
	int en;
};

struct clk_log {
	unsigned long long time;
	struct clk_hw *clk;
	const char *f_name;
	int mode;
	unsigned int arg;
};

struct pmu_log {
	unsigned long long time;
	unsigned int id;
	const char *f_name;
	int mode;
};

struct freq_log {
	unsigned long long time;
	int cpu;
	unsigned int old_freq;
	unsigned int target_freq;
	int en;
};

struct dm_log {
	unsigned long long time;
	int cpu;
	int req_type;
	int dm_num;
	unsigned int min_freq;
	unsigned int max_freq;
};

struct hrtimer_log {
	unsigned long long time;
	unsigned long long now;
	struct hrtimer *timer;
	void *fn;
	int en;
};

struct regulator_log {
	unsigned long long time;
	unsigned long long acpm_time;
	int cpu;
	char name[SZ_16];
	unsigned int reg;
	unsigned int voltage;
	unsigned int raw_volt;
	int en;
};

struct thermal_log {
	unsigned long long time;
	int cpu;
	unsigned int temp;
	char *cooling_device;
	unsigned long long cooling_state;
};

struct acpm_log {
	unsigned long long time;
	unsigned long long acpm_time;
	char log[9];
	unsigned int data;
};

struct print_log {
	unsigned long long time;
	int cpu;
	char log[DSS_LOG_STRING_LEN];
};

struct reg_log {
	unsigned long long time;
	unsigned char io_type;
	unsigned int data_type;
	unsigned long long val;
	void *addr;
	void *caller;
};


struct pmic_log {
	unsigned long long time;
	int cpu;
	int irq;
	int data1;
	int data2;
	char str[16];
};

struct dbg_snapshot_log {
	struct task_log task[DSS_NR_CPUS][DSS_LOG_MAX_NUM];
	struct work_log work[DSS_NR_CPUS][DSS_LOG_MAX_NUM];
	struct cpuidle_log cpuidle[DSS_NR_CPUS][DSS_LOG_MAX_NUM];
	struct suspend_log suspend[DSS_LOG_MAX_NUM * 2];
	struct irq_log irq[DSS_NR_CPUS][DSS_LOG_MAX_NUM * 4];
	struct clk_log clk[DSS_LOG_MAX_NUM];
	struct pmu_log pmu[DSS_LOG_MAX_NUM];
	struct freq_log freq[DSS_DOMAIN_NUM][DSS_LOG_MAX_NUM / 2];
	struct dm_log dm[DSS_LOG_MAX_NUM];
	struct hrtimer_log hrtimer[DSS_NR_CPUS][DSS_LOG_MAX_NUM];
	struct reg_log reg[DSS_NR_CPUS][DSS_LOG_MAX_NUM * 2];
	struct regulator_log regulator[DSS_LOG_MAX_NUM];
	struct thermal_log thermal[DSS_LOG_MAX_NUM];
	struct acpm_log acpm[DSS_LOG_MAX_NUM];
	struct pmic_log pmic[DSS_LOG_MAX_NUM];
	struct print_log print[DSS_LOG_MAX_NUM];
};

struct dbg_snapshot_log_minimized {
	struct task_log task[DSS_NR_CPUS][256];
	struct work_log work[DSS_NR_CPUS][256];
	struct cpuidle_log cpuidle[DSS_NR_CPUS][16];
	struct irq_log irq[DSS_NR_CPUS][1024];
	struct pmic_log pmic[32];
	struct freq_log freq[DSS_DOMAIN_NUM][128];
};

enum dbg_snapshot_log_type {
	eDSS_LOG_TYPE_NONE,
	eDSS_LOG_TYPE_DEFAULT,
	eDSS_LOG_TYPE_MINIMIZED,
};

struct dbg_snapshot_kevents {
	enum dbg_snapshot_log_type type;
	union {
		struct dbg_snapshot_log *default_log;
		struct dbg_snapshot_log_minimized *minimized_log;
	};
};

struct dbg_snapshot_log_misc {
	atomic_t task_log_idx[DSS_NR_CPUS];
	atomic_t work_log_idx[DSS_NR_CPUS];
	atomic_t cpuidle_log_idx[DSS_NR_CPUS];
	atomic_t suspend_log_idx;
	atomic_t irq_log_idx[DSS_NR_CPUS];
	atomic_t hrtimer_log_idx[DSS_NR_CPUS];
	atomic_t reg_log_idx[DSS_NR_CPUS];
	atomic_t clk_log_idx;
	atomic_t pmu_log_idx;
	atomic_t freq_log_idx[DSS_DOMAIN_NUM];
	atomic_t dm_log_idx;
	atomic_t regulator_log_idx;
	atomic_t thermal_log_idx;
	atomic_t print_log_idx;
	atomic_t acpm_log_idx;
	atomic_t pmic_log_idx;
};
#endif
