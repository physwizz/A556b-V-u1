/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/version.h>
#include <linux/atomic.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/sched/clock.h>
#include <soc/samsung/exynos/exynos-soc.h>

#include "npu-device.h"
#include "npu-debug.h"
#include "npu-log.h"
#include "interface/hardware/npu-interface.h"
#include "npu-hw-device.h"
#include "dsp-dhcp.h"

/* Non-printable character to mark the last dump postion is overwritten or not */
const char NPU_LOG_DUMP_MARK = (char)0x01;

/* Log level to filter message written to kernel log */
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
static struct npu_log npu_log = {
#else
struct npu_log npu_log = {
#endif
	.pr_level = NPU_LOG_INFO,	/* To kmsg */
	.st_level = NPU_LOG_DBG,	/* To memory buffer */
	.kpi_level = 1,			/* Default set for no prints in boost mode */
	.st_buf = NULL,
	.st_size = 0,
	.wr_pos = 0,
	.rp_pos = 0,
	.line_cnt = 0,
	.last_dump_line_cnt = 0,
	.last_dump_mark_pos = 0,
	.fs_ref = ATOMIC_INIT(0),
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
	.memlog_desc_log = NULL,
	.memlog_desc_array = NULL,
	.npu_memlog_obj = NULL,
	.npu_memfile_obj = NULL,
	.npu_ioctl_array_obj = NULL,
	.npu_array_file_obj = NULL,
#endif
};

/*
 *	In fw_report, some var(line_cnt) will be used with another purpose.
 *	this var will be used finish line when write pointer circled to start line.
 *	So, buffer in file object does not know about ...
 */
static struct npu_log fw_report = {
	.st_buf = NULL,
	.st_size = 0,
	.wr_pos = 0,
	.rp_pos = 0,
	.line_cnt = 0,
	.last_dump_line_cnt = 0,
};

/* Spinlock for memory logger */
static DEFINE_SPINLOCK(npu_log_lock);
static DEFINE_SPINLOCK(fw_report_lock);

/* Temporal buffer */
#define NPU_LOG_MSG_MAX		1024

const char *LOG_LEVEL_NAME[NPU_LOG_INVALID] = {
	"Trace", "Debug", "Info", "Warning", "Error", "None"
};
const char LOG_LEVEL_MARK[NPU_LOG_INVALID] = {
	'T', 'D', 'I', 'W', 'E', '_'
};

#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
size_t npu_dvfs_array_to_string(void *src, size_t src_size,
			void *buf, size_t count, loff_t *pos)
{
	struct npu_log_dvfs *ptr;
	size_t n = 0;
	unsigned long nsec = 0;

	if (src_size < sizeof(struct npu_log_dvfs))
		return 0;

	ptr = src;
	nsec = do_div(ptr->timestamp, 1000000000);

	n += scnprintf(buf + n, count - n, "[%5lu.%-6lu] [NPU][dvfs] dvfs_id : %d, dvfs_from : %d, dvfs_to : %d\n",
					(unsigned long)ptr->timestamp, nsec / 1000, ptr->id, ptr->from, ptr->to);

	*pos += n;

	return sizeof(*ptr);
}

size_t npu_scheduler_array_to_string(void *src, size_t src_size,
			void *buf, size_t count, loff_t *pos)
{
	struct npu_log_scheduler *ptr;
	size_t n = 0;
	unsigned long nsec = 0;

	if (src_size < sizeof(struct npu_log_scheduler))
		return 0;

	ptr = src;
	nsec = do_div(ptr->timestamp, 1000000000);

	n += scnprintf(buf + n, count - n,
			"[%5lu.%-6lu] [NPU][scheduler] NPUt : %d, CL0t : %d, CL1t : %d, CL2t : %d, idle_load : %d, fps_load : %d\n",
			(unsigned long)ptr->timestamp, nsec / 1000, (ptr->temp >> 24) & 0xff, (ptr->temp >> 16) & 0xff,
			(ptr->temp >> 8) & 0xff, ptr->temp & 0xff, (ptr->load >> 16) & 0xffff, ptr->load & 0xffff);

	*pos += n;

	return sizeof(*ptr);
}

size_t npu_protodrv_array_to_string(void *src, size_t src_size,
			void *buf, size_t count, loff_t *pos)
{
	struct npu_log_protodrv *ptr;
	size_t n = 0;
	unsigned long nsec = 0;

	if (src_size < sizeof(struct npu_log_protodrv))
		return 0;

	ptr = src;
	nsec = do_div(ptr->timestamp, 1000000000);

	n += scnprintf(buf + n, count - n,
			"[%5lu.%-6lu] [NPU][protodrv] protodrv_uid : %d, protodrv_cmd : %d, protodrv_param0 : %d, protodrv_param1 : %d\n",
			(unsigned long)ptr->timestamp, nsec / 1000, (ptr->uid_cmd & 0xFFFF0000) >> 16,
			(ptr->uid_cmd & 0xFFFF), ptr->param0, ptr->param1);

	*pos += n;

	return sizeof(*ptr);
}

size_t npu_hwdev_array_to_string(void *src, size_t src_size,
			void *buf, size_t count, loff_t *pos)
{
	struct npu_log_hwdev *ptr;
	size_t n = 0;
	unsigned long nsec = 0;

	if (src_size < sizeof(struct npu_log_hwdev))
		return 0;

	ptr = src;
	nsec = do_div(ptr->timestamp, 1000000000);

	n += scnprintf(buf + n, count - n,
			"[%5lu.%-6lu] [NPU][hwdev] hwdev_id : %d, hwdev_pwr : %d, hwdev_clk : %d, ioctl_cmd : 0x%x\n",
			(unsigned long)ptr->timestamp, nsec / 1000, ptr->hid,
			(ptr->status >> 16) & 0xffff, ptr->status & 0xffff, ptr->cmd);

	*pos += n;

	return sizeof(*ptr);
}

size_t npu_ioctl_array_to_string(void *src, size_t src_size,
			void *buf, size_t count, loff_t *pos)
{
	struct npu_log_ioctl *ptr;
	size_t n = 0;
	unsigned long nsec = 0;

	if (src_size < sizeof(struct npu_log_ioctl))
		return 0;

	ptr = src;
	nsec = do_div(ptr->timestamp, 1000000000);

	n += scnprintf(buf + n, count - n,
			"[%5lu.%-6lu] [NPU][ioctl] ioctl_cmd : 0x%x, ioctl_dir : %d\n",
			(unsigned long)ptr->timestamp, nsec / 1000, ptr->cmd, ptr->dir);

	*pos += n;

	return sizeof(*ptr);
}

size_t npu_ipc_array_to_string(void *src, size_t src_size,
			void *buf, size_t count, loff_t *pos)
{
	struct npu_log_ipc *ptr;
	size_t n = 0;
	unsigned long nsec = 0;

	if (src_size < sizeof(struct npu_log_ipc))
		return 0;

	ptr = src;
	nsec = do_div(ptr->timestamp, 1000000000);

	n += scnprintf(buf + n, count - n,
			"[%5lu.%-6lu] [NPU][ipc] h2f_ctrl : %d, rptr : 0x%x, wptr : 0x%x\n",
			(unsigned long)ptr->timestamp, nsec / 1000, ptr->h2fctrl, ptr->rptr, ptr->wptr);

	*pos += n;

	return sizeof(*ptr);
}

static size_t npu_log_memlog_data_to_string(void *src, size_t src_size,
			void *buf, size_t count, loff_t *pos)
{
	size_t	cp_sz = src_size >= count - 1 ? count - 1 : src_size;
	char	*dst_str = buf;

	dst_str[cp_sz] = 0;
	memcpy(buf, src, cp_sz);

	*pos += cp_sz;

	return cp_sz;
}

static void npu_log_rmemlog(struct npu_device *npu_dev)
{
	int ret = 0;

	/* Register NPU Device Driver for Unified Logging */
	ret = memlog_register("NPU_DRV1", npu_dev->dev, &npu_log.memlog_desc_log);
	if (ret)
		probe_err("memlog_register NPU_DRV1() for log failed: ret = %d\n", ret);

	ret = memlog_register("NPU_DRV2", npu_dev->dev, &npu_log.memlog_desc_dump);
	if (ret)
		probe_err("memlog_register NPU_DRV2() for log failed: ret = %d\n", ret);

	ret = memlog_register("NPU_DRV3", npu_dev->dev, &npu_log.memlog_desc_array);
	if (ret)
		probe_err("memlog_register NPU_DRV3() for array failed: ret = %d\n", ret);

	ret = memlog_register("NPU_DRV4", npu_dev->dev, &npu_log.memlog_desc_health);
	if (ret)
		probe_err("memlog_register NPU_DRV4() for array failed: ret = %d\n", ret);

	/* Receive allocation of memory for saving data to vendor storage */
	npu_log.npu_memfile_obj = memlog_alloc_file(npu_log.memlog_desc_log, "npu-fil",
						SZ_2M*2, SZ_2M*2, 500, 1);
	if (npu_log.npu_memfile_obj) {
		memlog_register_data_to_string(npu_log.npu_memfile_obj, npu_log_memlog_data_to_string);
		npu_log.npu_memlog_obj = memlog_alloc_printf(npu_log.memlog_desc_log, SZ_1M,
						npu_log.npu_memfile_obj, "npu-mem", 0);
		if (!npu_log.npu_memlog_obj)
			probe_err("memlog_alloc_printf() failed\n");
	} else {
		probe_err("memlog_alloc_file() failed\n");
	}

	npu_log.npu_dumplog_obj = memlog_alloc_printf(npu_log.memlog_desc_dump, SZ_1M,
						NULL, "npu-dum", 0);
	if (!npu_log.npu_dumplog_obj)
		probe_err("memlog_alloc_printf() failed\n");

	npu_log.npu_memhealth_obj = memlog_alloc_printf(npu_log.memlog_desc_health, SZ_128K,
						NULL, "npu-dum", 0);
	if (!npu_log.npu_memhealth_obj)
		probe_err("memlog_alloc_printf() failed\n");

	npu_log.npu_array_file_obj = memlog_alloc_file(npu_log.memlog_desc_array, "hw-fil",
						sizeof(union npu_log_tag)*LOG_UNIT_NUM,
						sizeof(union npu_log_tag)*LOG_UNIT_NUM,
						500, 1);

	if (npu_log.npu_array_file_obj) {
		npu_log.npu_ioctl_array_obj = memlog_alloc_array(npu_log.memlog_desc_array, LOG_UNIT_NUM,
			sizeof(struct npu_log_ioctl), npu_log.npu_array_file_obj, "io-arr",
			"npu_log_ioctl", 0);
		if (npu_log.npu_ioctl_array_obj) {
			memlog_register_data_to_string(npu_log.npu_ioctl_array_obj, npu_ioctl_array_to_string);
		} else {
			probe_err("memlog_alloc_array() failed\n");
		}
	} else {
		probe_err("memlog_alloc_file() for array failed\n");
	}
}

static void npu_log_store_log_sync(void) {};

void npu_log_set_loglevel(int slient, int loglevel)
{
	if (slient)
		npu_log.npu_memlog_obj->log_level = loglevel;
	else
		npu_log.npu_memlog_obj->log_level = npu_log.npu_dumplog_obj->log_level;
}

inline void npu_log_ioctl_set_date(int cmd, int dir)
{
	struct npu_log_ioctl npu_log_ioctl;

	npu_log_ioctl.cmd = cmd;
	npu_log_ioctl.dir = dir;

	if (npu_log.npu_ioctl_array_obj)
		memlog_write_array(npu_log.npu_ioctl_array_obj, MEMLOG_LEVEL_CAUTION, &npu_log_ioctl);
}

void npu_memlog_store(npu_log_level_e loglevel, const char *fmt, ...)
{
	char npu_string[1024];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(npu_string, fmt, ap);
	va_end(ap);

	if (npu_log.npu_memlog_obj->log_level < loglevel)
		return;

	if (npu_log.npu_memlog_obj)
		memlog_write_printf(npu_log.npu_memlog_obj, loglevel, npu_string);
	if (npu_log.npu_err_in_dmesg == NPU_ERR_IN_DMESG_ENABLE)
		pr_err("%s\n", npu_string);
}

void npu_dumplog_store(npu_log_level_e loglevel, const char *fmt, ...)
{
	char npu_string[1024];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(npu_string, fmt, ap);
	va_end(ap);

	if (npu_log.npu_dumplog_obj)
		memlog_write_printf(npu_log.npu_dumplog_obj, loglevel, npu_string);
}

void npu_healthlog_store(npu_log_level_e loglevel, const char *fmt, ...)
{
	char npu_string[1024];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(npu_string, fmt, ap);
	va_end(ap);

	if (npu_log.npu_memhealth_obj)
		memlog_write_printf(npu_log.npu_memhealth_obj, loglevel, npu_string);
}
#else
/* for debug and performance */
inline void npu_log_ioctl_set_date(int cmd, int dir) {}
inline void npu_log_ipc_set_date(int h2fctrl, int rptr, int wptr) {}
inline void npu_log_hwdev_set_data(int id) {}
static void npu_log_rmemlog(__attribute__((unused))struct npu_device *npu_dev) {};
static void npu_log_store_log_sync(void)
{
	pr_info(NPU_STORE_LOG_SYNC_MARK_MSG, npu_log.line_cnt);
}
#endif

/*
 * 0 : log success
 * != 0 : Failure
 */
int npu_store_log(npu_log_level_e loglevel, const char *fmt, ...)
{
	int		ret = 0;
	int		pr_size;
	size_t		wr_len = 0;
	size_t		remain;
	unsigned long	intr_flags;
	va_list		arg_ptr;
	char		*buf;

	spin_lock_irqsave(&npu_log_lock, intr_flags);

	remain = npu_log.st_size - npu_log.wr_pos;
	buf = npu_log.st_buf + npu_log.wr_pos;

	if (unlikely(!npu_log.st_buf)) {
		ret = -ENOENT;
		goto unlock_exit;
	}

	if ((npu_log.line_cnt & NPU_STORE_LOG_SYNC_MARK_INTERVAL_MASK) == 0)
		npu_log_store_log_sync();

	/* Execute from start */
	goto start;

retry:
	/* Executed when hit the buffer end during the writing messages. */
	if (unlikely(buf == npu_log.st_buf)) {
		ret = -ENOMEM;
		goto err_exit;		/* Buffer is too short */
	}

	/* Fill the remain buffer as separator line and move to start pos to retry */
	memset(buf, '#', npu_log.st_size - npu_log.wr_pos);
	npu_log.st_buf[npu_log.st_size - 1] = '\n';
	buf = npu_log.st_buf;
	remain = npu_log.st_size;
	npu_log.wr_pos = 0;
	wr_len = 0;

start:
	if ((npu_log.line_cnt & NPU_STORE_LOG_SYNC_MARK_INTERVAL_MASK) == 0) {
		pr_size = scnprintf(buf + wr_len, remain, NPU_STORE_LOG_SYNC_MARK_MSG, npu_log.line_cnt);
		if (unlikely(pr_size < 0)) {
			ret = -EFAULT;
			goto err_exit;
		}
		remain -= pr_size;
		wr_len += pr_size;
		if ((remain <= 1) || (pr_size == 0)) {		/* Underflow on 'remain -= pr_size' */
			goto retry;
		}
	}

	pr_size = scnprintf(buf + wr_len, remain, "%016llu;%c;"
		, sched_clock(), LOG_LEVEL_MARK[loglevel]);
	if (unlikely(pr_size < 0)) {
		ret = -EFAULT;
		goto err_exit;
	}
	remain -= pr_size;
	wr_len += pr_size;
	if ((remain <= 1) || (pr_size == 0)) {		/* Underflow on 'remain -= pr_size' */
		goto retry;
	}

	va_start(arg_ptr, fmt);
	pr_size = vscnprintf(buf + wr_len, remain, fmt, arg_ptr);
	va_end(arg_ptr);
	if (unlikely(pr_size < 0)) {
		ret = -EFAULT;
		goto err_exit;
	}
	remain -= pr_size;
	wr_len += pr_size;
	if ((remain <= 1) || (pr_size == 0)) {		/* Underflow on 'remain -= pr_size' */
		goto retry;
	}

	/* Update write position */
	npu_log.wr_pos = (npu_log.wr_pos + wr_len) % npu_log.st_size;
	npu_log.line_cnt++;

	ret = 0;
	goto unlock_exit;

err_exit:
	npu_err("Log store error : remain: %zu wr_len: %zu pr_size : %d ret :%d\n",
		remain, wr_len, pr_size, ret);

unlock_exit:
	spin_unlock_irqrestore(&npu_log_lock, intr_flags);

	return ret;
}

void npu_store_log_init(char *buf_addr, const size_t size)
{
	unsigned long	intr_flags;

	BUG_ON(!buf_addr);
	BUG_ON(size < PAGE_SIZE);

	spin_lock_irqsave(&npu_log_lock, intr_flags);
	npu_log.st_buf = buf_addr;
	buf_addr[0] = '\0';
	buf_addr[size - 1] = '\0';
	npu_log.st_size = size;
	npu_log.wr_pos = 0;
	npu_log.rp_pos = 0;
	spin_unlock_irqrestore(&npu_log_lock, intr_flags);

	npu_dbg("Store log memory initialized : %pK[Len = %zu]\n", buf_addr, size);
}

void npu_store_log_deinit(void)
{
	unsigned long	intr_flags;

	/* Wake-up readers and preserve some time to flush */
	wake_up_all(&npu_log.wq);

	/* Wait a few ms until all the readers dump their log */
	if (atomic_read(&npu_log.fs_ref) > 0) {
		npu_info("Waiting for all the logs are dumped via debufgs.\n");
		msleep(NPU_STORE_LOG_FLUSH_INTERVAL_MS);
	}

	npu_info("Store log memory deinitializing : %pK -> NULL\n", npu_log.st_buf);
	spin_lock_irqsave(&npu_log_lock, intr_flags);
	npu_log.st_buf = NULL;
	npu_log.st_size = 0;
	npu_log.wr_pos = 0;
	/*
	 * Reset ref count as Zero. There may be other client left,
	 * but they will not reduce counter if current value is zero.
	 */
	atomic_set(&npu_log.fs_ref, 0);
	spin_unlock_irqrestore(&npu_log_lock, intr_flags);
}

void npu_fw_report_init(char *buf_addr, const size_t size)
{
	unsigned long	intr_flags;

	BUG_ON(!buf_addr);
	BUG_ON(size < PAGE_SIZE);

	spin_lock_irqsave(&fw_report_lock, intr_flags);
	fw_report.st_buf = buf_addr;
	buf_addr[0] = '\0';
	buf_addr[size - 1] = '\0';
	fw_report.st_size = size;
	fw_report.wr_pos = 0;
	fw_report.rp_pos = 0;
	fw_report.line_cnt = 0;

	spin_unlock_irqrestore(&fw_report_lock, intr_flags);
}

void npu_fw_report_deinit(void)
{
	unsigned long	intr_flags;

	/* Wake-up readers and preserve some time to flush */
	wake_up_all(&fw_report.wq);
	msleep(NPU_STORE_LOG_FLUSH_INTERVAL_MS);

	npu_info("fw_report memory deinitializing : %pK -> NULL\n", fw_report.st_buf);
	spin_lock_irqsave(&fw_report_lock, intr_flags);
	fw_report.st_buf = NULL;
	fw_report.st_size = 0;
	fw_report.wr_pos = 0;
	fw_report.line_cnt = 0;

	spin_unlock_irqrestore(&fw_report_lock, intr_flags);
}

const u32 READ_OBJ_MAGIC = 0x1090D358;
struct npu_store_log_read_obj {
	u32			magic;
	size_t			read_pos;
};

static ssize_t npuerr_in_dmesg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;

	count = sprintf(buf, "\nvalue = %d\n\
			Valid values:\n\
			0 -> npu errors go into memlogger only\n\
			1 -> npu errors go into dmesg AND memloger)\n",
				npu_log.npu_err_in_dmesg);
	return count;
}

static ssize_t npuerr_in_dmesg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int idx;
	int ret;

	ret = sscanf(buf, "%1d", &idx);
	if (ret != 1) {
		npu_err("parsing error %d\n", ret);
		return -EINVAL;
	}

	if ((idx < NPU_ERR_IN_DMESG_DISABLE) || (idx > NPU_ERR_IN_DMESG_ENABLE)) {
		npu_err("invalid value: %d\n", idx);
		return -EINVAL;
	}
	npu_log.npu_err_in_dmesg = idx;
	return count;
}
static DEVICE_ATTR(npu_err_in_dmesg, 0644, npuerr_in_dmesg_show, npuerr_in_dmesg_store);

static int npu_fw_report_fops_open(struct inode *inode, struct file *file)
{
	int				ret = 0;
	struct npu_store_log_read_obj	*robj;

	robj = kzalloc(sizeof(*robj), GFP_ATOMIC);
	if (!robj) {
		ret = -ENOMEM;
		goto err_exit;
	}
	robj->read_pos = 0;
	robj->magic = READ_OBJ_MAGIC;
	/* TODO: It would be more useful if read_pos can start from circular queue tail */
	file->private_data = robj;

	npu_info("fd open robj @ %pK\n", robj);
	return 0;

err_exit:
	return ret;
}

static int npu_fw_report_fops_close(struct inode *inode, struct file *file)
{
	struct npu_store_log_read_obj	*robj;

	BUG_ON(!file);
	robj = file->private_data;
	BUG_ON(!robj);
	BUG_ON(robj->magic != READ_OBJ_MAGIC);

	npu_info("fd close robj @ %pK\n", robj);
	kfree(robj);

	return 0;

}

int npu_fw_report_store(char *strRep, size_t nSize)
{
	size_t	wr_len = 0;
	size_t	remain = fw_report.st_size - fw_report.wr_pos;
	char	*buf = NULL;
	unsigned long intr_flags;

	spin_lock_irqsave(&fw_report_lock, intr_flags);
	if (fw_report.st_buf == NULL)
		return -ENOMEM;

	buf = fw_report.st_buf + fw_report.wr_pos;

	//check overflow
	if (nSize >= remain) {
		fw_report.st_buf[fw_report.wr_pos] = '\0';
		fw_report.line_cnt = fw_report.wr_pos;
		fw_report.wr_pos = 0;
		remain = fw_report.st_size;
		buf = fw_report.st_buf;
		fw_report.last_dump_line_cnt++;
	}

	memcpy(buf, strRep, nSize);
	remain -= nSize;
	wr_len += nSize;
	npu_dbg("fw_report nSize : %zu,\t remain = %zu\n", nSize, remain);

	/* Update write position */
	fw_report.wr_pos = (fw_report.wr_pos + wr_len) % fw_report.st_size;
	fw_report.st_buf[fw_report.wr_pos] = '\0';

	spin_unlock_irqrestore(&fw_report_lock, intr_flags);

	return 0;
}

static ssize_t __npu_fw_report_fops_read(struct npu_store_log_read_obj *robj, char *outbuf, const size_t outlen)
{
	size_t	copy_len, buf_end;

	/* Copy data to temporary buffer*/
	if (fw_report.st_buf) {
		buf_end = (robj->read_pos > fw_report.wr_pos) ? fw_report.line_cnt : fw_report.wr_pos;
		copy_len = min(outlen, buf_end - robj->read_pos);
		memcpy(outbuf, fw_report.st_buf + robj->read_pos, copy_len);
	} else
		copy_len = 0;

	if (copy_len > 0) {
		robj->read_pos = (robj->read_pos + copy_len) % fw_report.st_size;
		if (fw_report.line_cnt == robj->read_pos) {
			fw_report.line_cnt = 0;
			robj->read_pos = 0;
			memset(&fw_report.st_buf[fw_report.wr_pos], '\0', (fw_report.st_size - fw_report.wr_pos));
		}
	}

	return copy_len;
}

static ssize_t npu_fw_report_fops_read(struct file *file, char __user *outbuf, size_t outlen, loff_t *loff)
{
	struct npu_store_log_read_obj	*robj;
	ssize_t				ret, copy_len;
	size_t				tmp_buf_len;
	char				*tmp_buf = NULL;
	unsigned long			intr_flags;

	BUG_ON(!file);
	robj = file->private_data;
	BUG_ON(!robj);
	BUG_ON(robj->magic != READ_OBJ_MAGIC);

	/* Temporal kernel buffer, to read inside of spinlock */
	tmp_buf_len = min(outlen, PAGE_SIZE);
	tmp_buf = kzalloc(tmp_buf_len, GFP_KERNEL);
	if (!tmp_buf) {
		ret = -ENOMEM;
		goto err_exit;
	}

	/* Check data available */
	ret = 0;
	while (ret == 0) {	/* ret = 0 on timeout */
		/* TODO: Accessing npu_log.wr_pos outside of spinlock is potentially dangerous */
		ret = wait_event_interruptible_timeout(fw_report.wq, robj->read_pos != fw_report.wr_pos, 1 * HZ);
		if (ret == -ERESTARTSYS) {
			ret = 0;
			goto err_exit;
		}
	}

	spin_lock_irqsave(&fw_report_lock, intr_flags);
	copy_len = __npu_fw_report_fops_read(robj, tmp_buf, tmp_buf_len);
	spin_unlock_irqrestore(&fw_report_lock, intr_flags);

	if (copy_len > 0) {
		ret = copy_to_user(outbuf, tmp_buf, copy_len);
		if (ret) {
			npu_err("copy_to_user failed : %zd\n", ret);
			ret = -EFAULT;
			goto err_exit;
		}
	}

	ret = copy_len;
err_exit:
	if (tmp_buf)
		kfree(tmp_buf);
	return ret;
}

const struct file_operations npu_fw_report_fops = {
	.owner = THIS_MODULE,
	.open = npu_fw_report_fops_open,
	.release = npu_fw_report_fops_close,
	.read = npu_fw_report_fops_read,
};

s32 atoi(const char *psz_buf)
{
	const char *pch = psz_buf;
	s32 base = 0;

	while (isspace(*pch))
		pch++;

	if (*pch == '-' || *pch == '+') {
		base = 10;
		pch++;
	} else if (*pch && tolower(pch[strlen(pch) - 1]) == 'h') {
		base = 16;
	}

	return simple_strtoul(pch, NULL, base);
}

int bitmap_scnprintf(char *buf, unsigned int buflen,
	const unsigned long *maskp, int nmaskbits)
{
	int i, word, bit, len = 0;
	unsigned long val;
	const char *sep = "";
	int chunksz;
	u32 chunkmask;

	chunksz = nmaskbits & (CHUNKSZ - 1);
	if (chunksz == 0)
		chunksz = CHUNKSZ;

	i = ALIGN(nmaskbits, CHUNKSZ) - CHUNKSZ;
	for (; i >= 0; i -= CHUNKSZ) {
		chunkmask = ((1ULL << chunksz) - 1);
		word = i / BITS_PER_LONG;
		bit = i % BITS_PER_LONG;
		val = (maskp[word] >> bit) & chunkmask;
		len += scnprintf(buf+len, buflen-len, "%s%0*lx", sep,
			(chunksz+3)/4, val);
		chunksz = CHUNKSZ;
		sep = ",";
	}
	return len;
}

int npu_debug_memdump8(u8 *start, u8 *end)
{
	int ret = 0;
	u8 *cur;
	u32 items;
	size_t offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump8(%pK ~ %pK)", start, end);

	while (cur < end) {
		if ((items % 16) == 0) {
#ifdef DEBUG_LOG_MEMORY
			pr_debug("%s\n", sentence);
#else
			pr_info("%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %pK:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "%02X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		pr_debug("%s\n", sentence);
#else
		pr_info("%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}


int npu_debug_memdump16(u16 *start, u16 *end)
{
	int ret = 0;
	u16 *cur;
	u32 items;
	size_t offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump16(%pK ~ %pK)", start, end);

	while (cur < end) {
		if ((items % 16) == 0) {
#ifdef DEBUG_LOG_MEMORY
			pr_debug("%s\n", sentence);
#else
			pr_info("%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %pK:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "0x%04X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		pr_debug("%s\n", sentence);
#else
		pr_info("%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}

int npu_debug_memdump32(u32 *start, u32 *end)
{
	int ret = 0;
	u32 *cur;
	u32 items;
	size_t offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump32(%pK ~ %pK)", start, end);

	while (cur < end) {
		if ((items % 8) == 0) {
#ifdef DEBUG_LOG_MEMORY
			pr_debug("%s\n", sentence);
#else
			pr_info("%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %pK:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "0x%08X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		pr_debug("%s\n", sentence);
#else
		pr_info("%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}

/*
 *This function could be used when direct access to SRAM is not approved.
 */
int npu_debug_memdump32_by_memcpy(u32 *start, u32 *end)
{
	int j, k, l;
	int ret = 0;
	u32 items;
	u32 *cur;
	char term[4], strHexa[128], strString[128], sentence[256];

	cur = start;
	items = 0;

	memset(sentence, 0, sizeof(sentence));
	memset(strString, 0, sizeof(strString));
	memset(strHexa, 0, sizeof(strHexa));
	j = sprintf(sentence, "[V] Memory Dump32(%pK ~ %pK)", start, end);
	while (cur < end) {
		if ((items % 4) == 0) {
			j += sprintf(sentence + j, "%s   %s", strHexa, strString);
#ifdef DEBUG_LOG_MEMORY
			pr_debug("%s\n", sentence);
#else
			npu_dump("%s\n", sentence);
#endif
			j = 0; items = 0; k = 0; l = 0;
			j = sprintf(sentence, "[V] %pK:      ", cur);
			items = 0;
		}
		memcpy_fromio(term, cur, sizeof(term));
		k += sprintf(strHexa + k, "%02X%02X%02X%02X ",
			term[0], term[1], term[2], term[3]);
		l += sprintf(strString + l, "%c%c%c%c", ISPRINTABLE(term[0]),
			ISPRINTABLE(term[1]), ISPRINTABLE(term[2]), ISPRINTABLE(term[3]));
		cur++;
		items++;
	}
	if (items) {
		j += sprintf(sentence + j, "%s   %s", strHexa, strString);
#ifdef DEBUG_LOG_MEMORY
		pr_debug("%s\n", sentence);
#else
		npu_dump("%s\n", sentence);
#endif
	}
	ret = cur - end;
	return ret;
}

bool npu_log_is_kpi_silent(void)
{
	if (npu_log.kpi_level == 1)
		return true;
	else
		return false;
}

static ssize_t npu_chg_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/*
	 * sysfs read buffer size is PAGE_SIZE
	 * Ref: http://www.kernel.org/pub/linux/kernel/people/mochel/doc/papers/ols-2005/mochel.pdf
	 */
	ssize_t remain = PAGE_SIZE - 1;
	ssize_t len = 0;
	ssize_t ret = 0;
	int	lv;

	npu_info("start.");

	npu_dbg("current log level : pr = %d, st = %d, kpi_level = %d", npu_log.pr_level, npu_log.st_level, npu_log.kpi_level);
	ret = scnprintf(buf, remain,
		" Usage  : echo <printk>.<Memory>.<kpi_level> > log_level\n"
		" Example: # echo 3.4.1 > log_level\n\n"
		"   <printk>      <Memory>    <kpi_level>\n");
	if (ret > 0)
		len += ret, remain -= ret, buf += ret;

	for (lv = 0; lv < NPU_LOG_INVALID; ++lv) {
		if (npu_log.pr_level == lv)
			ret = scnprintf(buf, remain,
				"[%d] %-10s", lv, LOG_LEVEL_NAME[lv]);
		else
			ret = scnprintf(buf, remain,
					" %d  %-10s", lv, LOG_LEVEL_NAME[lv]);
		if (ret > 0)
			len += ret, remain -= ret, buf += ret;

		if (npu_log.st_level == lv)
			ret = scnprintf(buf, remain,
					"[%d] %-10s\n", lv, LOG_LEVEL_NAME[lv]);
		else
			ret = scnprintf(buf, remain,
					" %d  %-10s\n", lv, LOG_LEVEL_NAME[lv]);
		if (ret > 0)
			len += ret, remain -= ret, buf += ret;
	}

	return len;
}

static ssize_t npu_chg_log_level_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int		pr, st, kpi_level;
	int		ret;

	/* Parsing */
	pr = st = -1;
	ret = sscanf(buf, "%1d.%1d.%1d", &pr, &st, &kpi_level);
	if (ret != 3) {
		npu_err("Command line parsing error %d.\n", ret);
		return -EINVAL;
	}
	if (pr < NPU_LOG_TRACE || NPU_LOG_INVALID < pr) {
		npu_err("Invalid pr_level [%d] specified.\n", pr);
		return -EINVAL;
	}
	if (st < NPU_LOG_TRACE || NPU_LOG_INVALID < st) {
		npu_err("Invalid st_level [%d] specified.\n", st);
		return -EINVAL;
	}
	if ((kpi_level != 0) && (kpi_level != 1)) {
		npu_err("Invalid kpi_level [%d] specified.\n", kpi_level);
		return -EINVAL;
	}

	/* Change log level */
	npu_info("log level for pr_level [%d -> %d] / st_level [%d -> %d] / kpi_level [%d -> %d]\n",
		npu_log.pr_level, pr, npu_log.st_level, st, npu_log.kpi_level, kpi_level);
	npu_log.pr_level = pr;
	npu_log.st_level = st;
	npu_log.kpi_level = kpi_level;
	barrier();

	return count;
}

static DEVICE_ATTR(log_level, 0644, npu_chg_log_level_show, npu_chg_log_level_store);

int fw_will_note_to_kernel(size_t len)
{
	unsigned long intr_flags;
	size_t i, pos;
	bool bReqLegacy = FALSE;

	//Gather one more time before make will note.
	npu_log.log_ops->fw_rprt_gather();
	spin_lock_irqsave(&fw_report_lock, intr_flags);

	if (fw_report.st_buf == NULL)
		return -ENOMEM;

	//Consideration for early phase
	if (fw_report.wr_pos < len) {
		len = fw_report.wr_pos;
		bReqLegacy = TRUE;
	}

	pos = 0;
	probe_err("----------- Start will_note for npu_fw (/sys/kernel/debug/npu/fw-report )-------------\n");
	if ((fw_report.last_dump_line_cnt != 0) && (bReqLegacy == TRUE)) {
		pos = fw_report.st_size - (len - fw_report.wr_pos);
		for (i = pos; i < fw_report.st_size; i++) {
			if (fw_report.st_buf[i] == '\n') {
				fw_report.st_buf[i] = '\0';
				probe_err("%s\n", &fw_report.st_buf[pos]);
				fw_report.st_buf[i] = '\n';
				pos = i+1;
			}
		}
		//Change length to current position.
		len = fw_report.wr_pos;
	}
	pos = fw_report.wr_pos - len;
	for (i = pos ; i < fw_report.wr_pos; i++) {
		if (fw_report.st_buf[i] == '\n') {
			fw_report.st_buf[i] = '\0';
			probe_err("%s\n", &fw_report.st_buf[pos]);
			fw_report.st_buf[i] = '\n';
			pos = i+1;
		}
	}

	fw_report.rp_pos = fw_report.wr_pos;

	probe_err("----------- End of will_note for fw -------------\n");
	spin_unlock_irqrestore(&fw_report_lock, intr_flags);

	return 0;
}

int fw_will_note(size_t len)
{
	unsigned long intr_flags;
	size_t i, pos;
	bool bReqLegacy = FALSE;

	//Gather one more time before make will note.
	npu_log.log_ops->fw_rprt_gather();
	spin_lock_irqsave(&fw_report_lock, intr_flags);

	if (fw_report.st_buf == NULL)
		return -ENOMEM;

	//Consideration for early phase
	if (fw_report.wr_pos < len) {
		len = fw_report.wr_pos;
		bReqLegacy = TRUE;
	}

	pos = 0;
	npu_dump("----------- Start will_note for npu_fw (/sys/kernel/debug/npu/fw-report )-------------\n");
	if ((fw_report.last_dump_line_cnt != 0) && (bReqLegacy == TRUE)) {
		pos = fw_report.st_size - (len - fw_report.wr_pos);
		for (i = pos; i < fw_report.st_size; i++) {
			if (fw_report.st_buf[i] == '\n') {
				fw_report.st_buf[i] = '\0';
				npu_dump("%s\n", &fw_report.st_buf[pos]);
				fw_report.st_buf[i] = '\n';
				pos = i+1;
			}
		}
		//Change length to current position.
		len = fw_report.wr_pos;
	}
	pos = fw_report.wr_pos - len;
	for (i = pos ; i < fw_report.wr_pos; i++) {
		if (fw_report.st_buf[i] == '\n') {
			fw_report.st_buf[i] = '\0';
			npu_dump("%s\n", &fw_report.st_buf[pos]);
			fw_report.st_buf[i] = '\n';
			pos = i+1;
		}
	}

	fw_report.rp_pos = fw_report.wr_pos;

	npu_dump("----------- End of will_note for npu_fw -------------\n");
	spin_unlock_irqrestore(&fw_report_lock, intr_flags);
	npu_dump("----------- Check unposted_mbox ---------------------\n");
	npu_log.log_ops->npu_check_unposted_mbox(ECTRL_LOW);
	npu_log.log_ops->npu_check_unposted_mbox(ECTRL_MEDIUM);
	npu_log.log_ops->npu_check_unposted_mbox(ECTRL_HIGH);
#if IS_ENABLED(CONFIG_NPU_USE_IMB_ALLOCATOR_V2)
	npu_log.log_ops->npu_check_unposted_mbox(ECTRL_FWRESPONSE);
#endif
	npu_log.log_ops->npu_check_unposted_mbox(ECTRL_ACK);
	npu_log.log_ops->npu_check_unposted_mbox(ECTRL_NACK);
#if IS_ENABLED(CONFIG_NPU_USE_IMB_ALLOCATOR_V2)
	npu_log.log_ops->npu_check_unposted_mbox(ECTRL_FWMSG);
#endif
	npu_log.log_ops->npu_check_unposted_mbox(ECTRL_REPORT);
	npu_dump("----------- Done unposted_mbox ----------------------\n");

	return 0;
}

#define DEACTIVE_INTERVAL	(10000000) // 10 second
static void npu_log_deactive_session_memory_info(struct npu_device *device)
{
	s64 now = 0, deactive_time = 0;
	struct npu_session *cur = NULL;
	struct npu_sessionmgr *sess_mgr;
	struct npu_memory *memory;
	struct npu_vertex_ctx *vctx = NULL;

	memory = &(device->system.memory);
	sess_mgr = &device->sessionmgr;

	npu_memory_health(memory);

	mutex_lock(&sess_mgr->active_list_lock);
	list_for_each_entry(cur, &sess_mgr->active_session_list, active_session) {
		now = npu_get_time_us();
		deactive_time = now - cur->last_qbuf_arrival;
		if (deactive_time < DEACTIVE_INTERVAL)
			continue;

#if IS_ENABLED(CONFIG_DSP_USE_VS4L)
		if (cur->hids & NPU_HWDEV_ID_DSP)
			continue;
#endif
		vctx = &cur->vctx;
		if (!(vctx->state & BIT(NPU_VERTEX_FORMAT)))
			continue;

		/* This session is not triggered by frame request over 10 second */
		npu_health("--------- session(%u) NCP/IOFM/IMB memory ----------\n", cur->uid);
		npu_health("model name : %s\n", cur->model_name);
		npu_memory_buffer_health_dump(cur->ncp_hdr_buf);
		npu_memory_buffer_health_dump(cur->IOFM_mem_buf);
#if IS_ENABLED(CONFIG_NPU_USE_IMB_ALLOCATOR)
		npu_memory_buffer_health_dump(cur->IMB_mem_buf);
#endif
	}
	mutex_unlock(&sess_mgr->active_list_lock);

	queue_delayed_work(device->npu_log_wq,
			&device->npu_log_work,
			msecs_to_jiffies(10000));
}

static void npu_log_health_monitor(struct work_struct * work)
{
	struct npu_device *npu_device;
	struct dsp_dhcp *dhcp;

	if (atomic_read(&npu_log.npu_log_active) != NPU_LOG_ACTIVE)
		return;

	npu_device = container_of(work, struct npu_device, npu_log_work.work);

	if ((exynos_soc_info.main_rev == 1) && (exynos_soc_info.sub_rev > 1)) {
		npu_log_deactive_session_memory_info(npu_device);
		return;
	}

	dhcp = npu_device->system.dhcp;

	queue_delayed_work(npu_device->npu_log_wq,
			&npu_device->npu_log_work,
			msecs_to_jiffies(60000));

	pr_info("npu rcnt = %u\n", dsp_dhcp_read_reg_idx(dhcp, DSP_DHCP_RESET_COUNT));
}

/* Exported functions */
int npu_log_probe(struct npu_device *npu_device)
{
	int ret = 0;

	/* Basic initialization of log store */
	npu_log.st_buf = NULL;
	npu_log.st_size = 0;
	npu_log.wr_pos = 0;
	npu_log.dev = npu_device->dev;
	npu_log.log_ops = &npu_log_ops;
	init_waitqueue_head(&npu_log.wq);
	init_waitqueue_head(&fw_report.wq);

	npu_log_rmemlog(npu_device);

	atomic_set(&npu_log.npu_log_active, NPU_LOG_DEACTIVE);

	/* Log level change function on sysfs */
	ret = device_create_file(npu_device->dev, &dev_attr_log_level);
	if (ret) {
		probe_err("device_create_file() failed: ret = %d\n", ret);
		return ret;
	}

	/* Register FW log keeper */
	ret = npu_debug_register("fw-report", &npu_fw_report_fops);
	if (ret) {
		npu_err("npu_debug_register error : ret = %d\n", ret);
		return ret;
	}

	npu_log.npu_err_in_dmesg = NPU_ERR_IN_DMESG_DISABLE;
	ret = device_create_file(npu_device->dev, &dev_attr_npu_err_in_dmesg);
	if (ret) {
		npu_err("npu_debug_register error : ret = %d\n", ret);
		return ret;
	}

	INIT_DELAYED_WORK(&npu_device->npu_log_work, npu_log_health_monitor);

	npu_device->npu_log_wq = create_singlethread_workqueue(dev_name(npu_device->dev));
	if (!npu_device->npu_log_wq) {
		probe_err("fail to create workqueue -> npu_device->npu_log_wq\n");
		ret = -EFAULT;
	}

	return ret;
}

int npu_log_release(struct npu_device *npu_device)
{
	device_remove_file(npu_device->dev, &dev_attr_log_level);
	return 0;
}

int npu_log_open(struct npu_device *npu_device)
{
	struct dsp_dhcp *dhcp = npu_device->system.dhcp;

	if (exynos_soc_info.main_rev != 1)
		goto done;

	atomic_set(&npu_log.npu_log_active, NPU_LOG_ACTIVE);

	queue_delayed_work(npu_device->npu_log_wq,
			&npu_device->npu_log_work,
			msecs_to_jiffies(1000));

	dsp_dhcp_write_reg_idx(dhcp, DSP_DHCP_RESET_COUNT, 0x0);
done:
	return 0;
}

int npu_log_close(struct npu_device *npu_device)
{
	struct dsp_dhcp *dhcp = npu_device->system.dhcp;

	atomic_set(&npu_log.npu_log_active, NPU_LOG_DEACTIVE);
	cancel_delayed_work_sync(&npu_device->npu_log_work);

	if ((exynos_soc_info.main_rev == 1) && (exynos_soc_info.sub_rev > 1))
		goto done;

	pr_info("npu rcnt = %u\n", dsp_dhcp_read_reg_idx(dhcp, DSP_DHCP_RESET_COUNT));
done:
	return 0;
}

/* Unit test */
#if IS_ENABLED(CONFIG_NPU_UNITTEST)
#define IDIOT_TESTCASE_IMPL "npu-log.idiot"
#include "idiot-def.h"
#endif
