/*
 * exynos_acpm_tmu.c - ACPM TMU plugin interface
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/debugfs.h>
#include <linux/sched/clock.h>
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
#include <soc/samsung/esca.h>
#else
#include <soc/samsung/acpm_ipc_ctrl.h>
#endif
#include "exynos_acpm_tmu.h"

#include <kunit/visibility.h>
#include <kunit/static_stub.h>

static unsigned int acpm_tmu_ch_num, acpm_tmu_size;

static bool acpm_tmu_test_mode;
static bool acpm_tmu_log;

VISIBLE_IF_KUNIT bool exynos_acpm_tmu_is_test_mode(void)
{
#if IS_ENABLED(CONFIG_DTM_EXYNOS_KUNIT_TEST)
	KUNIT_STATIC_STUB_REDIRECT(exynos_acpm_tmu_is_test_mode);
#endif

	return acpm_tmu_test_mode;
}
EXPORT_SYMBOL_IF_KUNIT(exynos_acpm_tmu_is_test_mode);

void exynos_acpm_tmu_set_test_mode(bool mode)
{
	acpm_tmu_test_mode = mode;
}

void exynos_acpm_tmu_log(bool mode)
{
	acpm_tmu_log = mode;
}

#define acpm_ipc_latency_check() \
	do { \
		if (acpm_tmu_log) { \
			pr_info("[acpm_tmu] type 0x%02x latency %llu ns ret %d\n", \
					message->req.type, latency, ret); \
		} \
	} while (0)

#define acpm_ipc_err_check() \
	do { \
		if (ret < 0) { \
			pr_warn("[acpm_tmu] IPC error! type 0x%02x latency %llu ns ret %d\n", \
					message->req.type, latency, ret); \
		} \
	} while (0)

static void exynos_acpm_tmu_ipc_send_data(union tmu_ipc_message *message)
{

	struct ipc_config config;
	int ret;
	unsigned long long before, after, latency;

	config.cmd = message->data;
	config.response = true;
	config.indirection = false;

	before = sched_clock();
	ret = esca_ipc_send_data(acpm_tmu_ch_num, &config);
	after = sched_clock();
	latency = after - before;

	acpm_ipc_err_check();
	acpm_ipc_latency_check();
}

/*
 * TMU_IPC_INIT
 */
int exynos_acpm_tmu_set_init(struct acpm_tmu_cap *cap)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));

	message.req.type = TMU_IPC_INIT;

	exynos_acpm_tmu_ipc_send_data(&message);

	if (message.resp.ret & CAP_APM_IRQ)
		cap->acpm_irq = true;

	if (message.resp.ret & CAP_APM_DIVIDER)
		cap->acpm_divider = true;

	return 0;
}

/*
 * TMU_IPC_GET_DSS_DUMP_ADDR
 *
 * - tz: thermal zone index registered in device tree
 */
int exynos_acpm_tmu_get_addr(u32 type, u32 *addr, u32 *size)
{
	union tmu_ipc_message message;

	if (acpm_tmu_test_mode)
		return -1;

	memset(&message, 0, sizeof(message));

	message.req.type = type;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	*addr = message.resp.rsvd0;
	*size = message.resp.rsvd1;

	return 0;
}

/*
 * TMU_IPC_READ_TEMP
 *
 * - tz: thermal zone index registered in device tree
 */
int exynos_acpm_tmu_set_read_temp(int tz, int *temp, int *stat, u32 *data)
{
	union tmu_ipc_message message;

	if (acpm_tmu_test_mode)
		return -1;

	memset(&message, 0, sizeof(message));

	message.req.type = TMU_IPC_READ_TEMP;
	message.req.tzid = tz;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	*temp = message.resp.temp;
	*stat = message.resp.stat;
	if (data) {
#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
		data[0] = message.resp.rsvd0;
		data[1] = message.resp.rsvd1;
		data[2] = message.resp.rsvd2;
#else
		data[0] = message.data[2];
		data[1] = message.data[3];
		data[0] &= ~(0xff);
		data[0] |= *temp;
#endif
	}

	return 0;
}

int exynos_acpm_tmu_get_dfs_time(ktime_t *ktime, u64 *tick)
{
	union tmu_ipc_message message = { 0, };

	*ktime = ktime_get();

	message.req.type = TMU_IPC_GET_DFS_TIME;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	*tick = ((u64)message.data[2] << 32) | (u64)message.data[1];

	return 0;
}

/*
 * TMU_IPC_AP_SUSPEND
 */
int exynos_acpm_tmu_set_suspend(int flag)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));

	message.req.type = TMU_IPC_AP_SUSPEND;
	message.req.rsvd = flag;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	return 0;
}

/*
 * TMU_IPC_CP_CALL
 */
int exynos_acpm_tmu_set_cp_call(void)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));

	message.req.type = TMU_IPC_CP_CALL;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	return 0;
}

/*
 * TMU_IPC_AP_RESUME
 */
int exynos_acpm_tmu_set_resume(void)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));

	message.req.type = TMU_IPC_AP_RESUME;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	pr_info("%s: acpm irq %d stat %d\n",
			__func__, message.resp.rsvd2, message.resp.stat);

	return 0;
}

int exynos_acpm_tmu_ipc_dump(int no, unsigned int dump[])
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));

	message.req.type = TMU_IPC_READ_TEMP;
	message.req.tzid = no;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu_dump] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	if (dump) {
		dump[0] = message.data[2];
		dump[1] = message.data[3];
	}

	return 0;
}

void exynos_acpm_tmu_set_threshold(int tz, unsigned char temp[])
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));

	message.req.type = TMU_IPC_THRESHOLD;
	message.req.tzid = tz;
	message.req.req_rsvd0 = temp[0];
	message.req.req_rsvd1 = temp[1];
	message.req.req_rsvd2 = temp[2];
	message.req.req_rsvd3 = temp[3];
	message.req.req_rsvd4 = temp[4];
	message.req.req_rsvd5 = temp[5];
	message.req.req_rsvd6 = temp[6];
	message.req.req_rsvd7 = temp[7];

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_set_interrupt_enable(int tz, unsigned char inten)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.req.type = TMU_IPC_INTEN;
	message.req.tzid = tz;
	message.req.req_rsvd0 = inten;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_tz_control(int tz, bool enable)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.req.type = TMU_IPC_TMU_CONTROL;
	message.req.tzid = tz;
	message.req.req_rsvd0 = ((enable == true ) ? 1 : 0);

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_set_emul_temp(int tz, unsigned char temp)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.req.type = TMU_IPC_EMUL_TEMP;
	message.req.tzid = tz;
	message.req.req_rsvd0 = temp;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

#if IS_ENABLED(CONFIG_EXYNOS_ESCA_THERMAL)
void exynos_acpm_tmu_set_tmu_data(int tz, unsigned char member,
						  unsigned char index, int data)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_SET_TMU_DATA;
	message.sdata.tzid = tz;
	message.sdata.rsvd = member;
	message.sdata.rsvd2 = index;
	message.sdata.data = data;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_set_cpufreq_cdev(int tz, unsigned char member,
		unsigned char index, int data)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_SET_CPUFREQ_CDEV;
	message.sdata.tzid = tz;
	message.sdata.rsvd = member;
	message.sdata.rsvd2 = index;
	message.sdata.data = data;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_instance_update(int cdev_id, int id,
		unsigned char member, int data)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_INSTANCE_UPDATE;
	message.sdata.tzid = cdev_id;
	message.sdata.rsvd = id;
	message.sdata.rsvd2 = member;
	message.sdata.data = data;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_init_pi_work(int tz)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_INIT_PI_WORK;
	message.sdata.tzid = tz;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_start_pi_polling(int tz)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_START_PI_POLLING;
	message.sdata.tzid = tz;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

#if IS_ENABLED(CONFIG_SOC_S5E9945)
int exynos_acpm_tmu_cdev_get_cur_state(int cdev_id, unsigned long *state)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_CDEV_GET_CUR_STATE;
	message.sdata.rsvd = cdev_id;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	*state = message.sdata.data;
	return 0;
}
EXPORT_SYMBOL_GPL(exynos_acpm_tmu_cdev_get_cur_state);
#endif

void exynos_acpm_tmu_set_device_cdev(int tz, unsigned char member,
		unsigned char index, int data)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_SET_DEVICE_CDEV;
	message.sdata.tzid = tz;
	message.sdata.rsvd = member;
	message.sdata.rsvd2 = index;
	message.sdata.data = data;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_get_pi_params(int tz, unsigned char member, s64 *data)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_GET_PI_PARAMS;
	message.sdata.tzid = tz;
	message.sdata.rsvd = member;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
	*data = (s64) message.sdata.data2 << 32;
	*data |= message.sdata.data;
}
#if IS_ENABLED(CONFIG_EXYNOS_EA_DTM)
EXPORT_SYMBOL_GPL(exynos_acpm_tmu_get_pi_params);
#endif

void exynos_acpm_tmu_set_gpufreq_cdev(int tz, unsigned char member,
		unsigned char index, int data)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_SET_GPUFREQ_CDEV;
	message.sdata.tzid = tz;
	message.sdata.rsvd = member;
	message.sdata.rsvd2 = index;
	message.sdata.data = data;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}
EXPORT_SYMBOL_GPL(exynos_acpm_tmu_set_gpufreq_cdev);

void exynos_acpm_tmu_set_isp_cdev(int tz, unsigned char member,
		unsigned char index, int data)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_SET_ISP_CDEV;
	message.sdata.tzid = tz;
	message.sdata.rsvd = member;
	message.sdata.rsvd2 = index;
	message.sdata.data = data;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_get_tmu_data(int tz, unsigned char member, int *data)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_GET_TMU_DATA;
	message.sdata.tzid = tz;
	message.sdata.rsvd = member;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	*data = message.sdata.data;
}
#if IS_ENABLED(CONFIG_EXYNOS_EA_DTM)
EXPORT_SYMBOL_GPL(exynos_acpm_tmu_get_tmu_data);
#endif

#if IS_ENABLED(CONFIG_SOC_S5E9945)
void exynos_acpm_tmu_clear_cdev_update(int tz)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_CLEAR_CDEV_UPDATE;
	message.sdata.tzid = tz;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}
#endif

void exynos_acpm_tmu_get_pi_trace_mode(unsigned long long *mode)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_GET_PI_TRACE_MODE;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
	*mode = (unsigned long long)message.sdata.rsvd;
}

void exynos_acpm_tmu_set_pi_trace_mode(unsigned long long mode)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_SET_PI_TRACE_MODE;
	message.sdata.rsvd = (u8)mode;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_stop_pi_polling(int tz)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_STOP_PI_POLLING;
	message.sdata.tzid = tz;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

int exynos_acpm_tmu_cdev_get_cur_freq(int cdev_id, unsigned long *freq)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_GET_CUR_FREQ;
	message.sdata.rsvd = cdev_id;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	if (!message.resp.ret)
		*freq = message.sdata.data;
	else
		*freq = 0;

	return 0;
}

void exynos_acpm_tmu_set_tmu_mode(int tz, unsigned int mode)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_SET_TMU_MODE;
	message.sdata.tzid = tz;
	message.sdata.rsvd = mode;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}

void exynos_acpm_tmu_get_tmu_mode(int tz, unsigned int *mode)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.sdata.type = TMU_IPC_GET_TMU_MODE;
	message.sdata.tzid = tz;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}

	*mode = message.sdata.rsvd;
}
#else
void exynos_acpm_tmu_change_threshold(int tz, unsigned char temp, unsigned char point)
{
	union tmu_ipc_message message;

	memset(&message, 0, sizeof(message));
	message.req.type = TMU_IPC_CHANGE_THRESHOLD;
	message.req.tzid = tz;
	message.req.req_rsvd0 = temp;
	message.req.req_rsvd1 = point;

	exynos_acpm_tmu_ipc_send_data(&message);
	if (acpm_tmu_log) {
		pr_info("[acpm_tmu] data 0:0x%08x 1:0x%08x 2:0x%08x 3:0x%08x\n",
				message.data[0],
				message.data[1],
				message.data[2],
				message.data[3]);
	}
}
#endif
int exynos_acpm_tmu_init(ipc_callback handler)
{
	struct device_node *np;

	np = of_find_node_by_name(NULL, "acpm_tmu");
	if (!np)
		return -ENODEV;

#if IS_ENABLED(CONFIG_EXYNOS_ESCAV1) || IS_ENABLED(CONFIG_EXYNOS_ESCA)
	return esca_ipc_request_channel(np, handler, &acpm_tmu_ch_num, &acpm_tmu_size);
#else
	return acpm_ipc_request_channel(np, handler, &acpm_tmu_ch_num, &acpm_tmu_size);
#endif

}
