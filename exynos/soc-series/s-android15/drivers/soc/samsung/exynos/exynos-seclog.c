/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - Logging message from Secure World
 * Author: Junho Choi <junhosj.choi@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>

#include <soc/samsung/exynos-smc.h>
#include <soc/samsung/exynos/exynos-seclog.h>

/*
 * Macro for converting physical address to
 * virtual address that is mapped by vmap
 */
#define SECLOG_PHYS_TO_VIRT(addr)		((unsigned long)ldata.virt_addr	\
						+ ((unsigned long)(addr)	\
						- ldata.phys_addr))

static struct seclog_data ldata;
static struct seclog_ctx slog_ctx;
static struct sec_log_info *sec_log[CONFIG_VENDOR_NR_CPUS];


static void exynos_ldfw_error(struct platform_device *pdev,
				int error)
{
	int err_ldfw, i;

	for (i = 0; i < LDFW_MAX_NUM; i++) {
		err_ldfw = (error >> (BITLEN_LDFW_ERROR * i))
				& MASK_LDFW_ERROR;

		switch (err_ldfw) {
		case 0:
			break;
		case ERROR_NOT_SUPPORT_LDFW_SEC_LOG:
			dev_err(&pdev->dev,
				"[ERROR] LDFW[%d] doesn't support Secure log\n",
				i);
			break;
		case ERROR_LDFW_ALREADY_INITIALIZED:
			dev_err(&pdev->dev,
				"[ERROR] LDFW[%d] already initialized Secure log\n",
				i);
			break;
		case ERROR_NOT_SUPPORT_LDFW_ERR_VALUE:
			dev_err(&pdev->dev,
				"[ERROR] LDFW[%d] returns unsupported error value\n",
				i);
			break;
		default:
			dev_err(&pdev->dev,
				"[ERROR] LDFW[%d] Unknown error value from LDFW [err = %#x]\n",
				i, err_ldfw);
		}
	}
}

static void *exynos_seclog_request_region(unsigned long addr,
					unsigned int size)
{
	int i;
	unsigned int num_pages = (size >> PAGE_SHIFT);
	pgprot_t prot = pgprot_writecombine(PAGE_KERNEL);
	struct page **pages = NULL;
	void *v_addr = NULL;


	if (!addr)
		return NULL;

	pages = kmalloc_array(num_pages, sizeof(struct page *), GFP_ATOMIC);
	if (!pages)
		return NULL;

	for (i = 0; i < num_pages; i++) {
		pages[i] = phys_to_page(addr);
		addr += PAGE_SIZE;
	}

	v_addr = vmap(pages, num_pages, VM_MAP, prot);
	kfree(pages);

	return v_addr;
}

static void exynos_seclog_worker(struct work_struct *work)
{
	struct log_header_info *v_log_arr = 0;
	unsigned long v_log_addr = 0;
	unsigned int cpu = 0;
	unsigned int tmp_read_cnt = 0;
	unsigned int tmp_write_cnt = 0;

	pr_debug("%s: Start seclog_worker\n", __func__);

	/* Print log message in a message buffer */
	for (cpu = 0; cpu < CONFIG_VENDOR_NR_CPUS; cpu++) {
		v_log_addr = SECLOG_PHYS_TO_VIRT(sec_log[cpu]->initial_log_addr);
		v_log_arr = (struct log_header_info *)v_log_addr;
		tmp_read_cnt = sec_log[cpu]->log_read_cnt;
		tmp_write_cnt = sec_log[cpu]->log_write_cnt;

		while (tmp_read_cnt != tmp_write_cnt) {

			/* For debug */
			pr_debug("[SECLOG_DEBUG C%d] read_cnt[%d]\n",
					cpu, sec_log[cpu]->log_read_cnt);
			pr_debug("[SECLOG_DEBUG C%d] write_cnt[%d]\n",
					cpu, sec_log[cpu]->log_write_cnt);
			pr_debug("[SECLOG_DEBUG C%d] v_log_addr[%#lx]\n",
					cpu, v_log_addr);
			pr_debug("[SECLOG_DEBUG C%d] p_log_addr[%#lx]\n",
					cpu,
					v_log_addr
					- (unsigned long)ldata.virt_addr
					+ ldata.phys_addr);

			/* Set log address and log's header address */
			/* For debug */
			pr_debug("[SECLOG_DEBUG C%d] v_log_addr[%#lx]\n",
					cpu, (unsigned long)v_log_addr);
			pr_debug("[SECLOG_DEBUG C%d] v_log_arr[%#lx]\n",
					cpu, (unsigned long)v_log_arr);
			/* Print logs from SWd */
			pr_info("[SECLOG C%d] [%06d.%06d] %s",
				cpu,
				v_log_arr[tmp_read_cnt].tv_sec,
				v_log_arr[tmp_read_cnt].tv_usec,
				v_log_arr[tmp_read_cnt].buf);

			/* Increase read count */
			tmp_read_cnt = (tmp_read_cnt + 1) % MAX_LOG_COUNT;
		}
		sec_log[cpu]->log_read_cnt = tmp_read_cnt;
		if(sec_log[cpu]->log_buffer_full_flag) {
			pr_info("[SECLOG Error C%d] Sec_log is full!!", cpu);
			pr_info("[SECLOG Error C%d] %d Messages blocked...",
					cpu, sec_log[cpu]->blocked_log_cnt);
			sec_log[cpu]->blocked_log_cnt = 0;
			sec_log[cpu]->log_buffer_full_flag = 0;
		}
	}
}

static irqreturn_t exynos_seclog_irq_handler(int irq, void *dev_id)
{
	unsigned int cpu = 0;

	if (slog_ctx.enabled) {
		schedule_work(&slog_ctx.work);
	} else {
		/* Skip all log messages */
		for (cpu = 0; cpu < CONFIG_VENDOR_NR_CPUS; cpu++) {
			sec_log[cpu]->log_read_cnt = sec_log[cpu]->log_write_cnt;
		}
	}

	pr_debug("ISR for Secure log is implemented!\n");

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF_RESERVED_MEM
static int __init exynos_seclog_reserved_mem_setup(struct reserved_mem *remem)
{
	ldata.phys_addr = remem->base;
	ldata.size = remem->size;

	pr_err("%s: Reserved memory for seclog: addr=%lx, size=%lx\n",
			__func__, ldata.phys_addr, ldata.size);

	return 0;
}
RESERVEDMEM_OF_DECLARE(seclog_mem, "exynos,seclog", exynos_seclog_reserved_mem_setup);
#endif	/* CONFIG_OF_RESERVED_MEM */

static int exynos_seclog_probe(struct platform_device *pdev)
{
	struct irq_data *seclog_irqd = NULL;
	struct reserved_mem *rmem;
	struct device_node *rmem_np;
	irq_hw_number_t hwirq = 0;
	int err, i;

	rmem_np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	rmem = of_reserved_mem_lookup(rmem_np);
	if (!rmem) {
		dev_err(&pdev->dev, "failed to acquire memory region\n");
		return 0;
	}

	ldata.phys_addr = rmem->base;
	ldata.size = rmem->size;

	/* Translate PA to VA of message buffer */
	ldata.virt_addr = exynos_seclog_request_region(ldata.phys_addr, ldata.size);
	if (!ldata.virt_addr) {
		dev_err(&pdev->dev, "Fail to translate message buffer\n");
		return -EFAULT;
	}

	dev_info(&pdev->dev,
		"Message buffer address[%#lx], Message buffer size[%#lx]\n",
		ldata.phys_addr, ldata.size);

	/* Setup virtual address of message buffer of each core */
	for (i = 0; i < CONFIG_VENDOR_NR_CPUS; i++) {
		sec_log[i] = (struct sec_log_info *)((unsigned long)ldata.virt_addr
							+ (SECLOG_LOG_BUF_SIZE * i));
		if (!sec_log[i]) {
			dev_err(&pdev->dev, "sec_log[C%d]: %#lx\n",
				i, (unsigned long)sec_log[i]);
			vunmap(ldata.virt_addr);
			return -EINVAL;
		}
	}

	slog_ctx.irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!slog_ctx.irq) {
		dev_err(&pdev->dev, "Fail to get irq from dt\n");
		vunmap(ldata.virt_addr);
		return -EINVAL;
	}

	/* Get irq_data for secure log */
	seclog_irqd = irq_get_irq_data(slog_ctx.irq);
	if (!seclog_irqd) {
		dev_err(&pdev->dev, "Fail to get irq_data\n");
		vunmap(ldata.virt_addr);
		return -EINVAL;
	}

	/* Get hardware interrupt number */
	hwirq = irqd_to_hwirq(seclog_irqd);

	dev_dbg(&pdev->dev,
		"hwirq for seclog (%ld)\n",
		hwirq);

	err = devm_request_irq(&pdev->dev, slog_ctx.irq,
				exynos_seclog_irq_handler,
				IRQF_TRIGGER_RISING, pdev->name, NULL);
	if (err) {
		dev_err(&pdev->dev,
			"Fail to request IRQ handler. err(%d) irq(%d)\n",
			err, slog_ctx.irq);
		vunmap(ldata.virt_addr);
		return err;
	}

	/* Set workqueue for Secure log as bottom half */
	INIT_WORK(&slog_ctx.work, exynos_seclog_worker);
	slog_ctx.enabled = true;

	/* Create debugfs for Secure log */
	slog_ctx.debug_dir = debugfs_create_dir("seclog", NULL);
	debugfs_create_bool("seclog_debug", 0600, slog_ctx.debug_dir,
			&slog_ctx.enabled);

	/* Send message buffer information to EL3 Monitor */
	dev_dbg(&pdev->dev,
		"SMC arguments(%#x, %#lx, %#lx, %ld)\n",
		SMC_CMD_SEC_LOG_INFO, ldata.phys_addr, ldata.size, hwirq);

	err = exynos_smc(SMC_CMD_SEC_LOG_INFO, ldata.phys_addr, ldata.size, hwirq);
	if (err) {
		switch (err) {
		case ERROR_INVALID_LOG_LEN:
			dev_err(&pdev->dev,
				"[ERROR] Invalid log length [Message buffer length = %#lx]\n",
				ldata.size);
			break;
		case ERROR_INVALID_LOG_ADDR:
			dev_err(&pdev->dev,
				"[ERROR] Invalid log address [Message buffer address = %#lx]\n",
				ldata.phys_addr);
			break;
		case ERROR_INVALID_INTR_NUM:
			dev_err(&pdev->dev,
				"[ERROR] Invalid interrupt number [Interrupt number = %ld]\n",
				hwirq);
			break;
		case ERROR_ALREADY_INITIALIZED:
			dev_err(&pdev->dev, "[ERROR] Already initialized\n");
			break;
		case SMC_CMD_SEC_LOG_INFO:
			dev_err(&pdev->dev, "[ERROR] EL3 Monitor doesn't support Secure log\n");
			break;
		default:
			/* Error cases by LDFW */
			if ((err & MASK_LDFW_MAGIC) == LDFW_MAGIC) {
				exynos_ldfw_error(pdev, err);
				goto detect_ldfw_err;
			} else {
				dev_err(&pdev->dev,
					"[ERROR] Unknown error value [err = %#x]\n",
					err);
				break;
			}
		}

		dev_err(&pdev->dev, "Fail to initialize Secure log\n");

		cancel_work_sync(&slog_ctx.work);
		devm_free_irq(&pdev->dev, slog_ctx.irq, NULL);
		for (i = 0; i < CONFIG_VENDOR_NR_CPUS; i++) {
			sec_log[i] = 0;
		}
		vunmap(ldata.virt_addr);

		return -EINVAL;
	}

detect_ldfw_err:
	dev_info(&pdev->dev, "Exynos Secure Log driver probe done!\n");

	return 0;
}

static const struct of_device_id exynos_seclog_of_match_table[] = {
	{ .compatible = "samsung,exynos-seclog", },
	{ },
};
MODULE_DEVICE_TABLE(of, exynos_seclog_of_match_table);

static struct platform_driver exynos_seclog_driver = {
	.probe = exynos_seclog_probe,
	.driver = {
		.name = "exynos-seclog",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_seclog_of_match_table),
	}
};

static int __init exynos_seclog_init(void)
{
	return platform_driver_register(&exynos_seclog_driver);
}

static void __exit exynos_seclog_exit(void)
{
	if (slog_ctx.enabled)
		schedule_work(&slog_ctx.work);
	flush_work(&slog_ctx.work);

	platform_driver_unregister(&exynos_seclog_driver);
}

module_init(exynos_seclog_init);
module_exit(exynos_seclog_exit);

MODULE_DESCRIPTION("Exynos Secure log printing driver");
MODULE_AUTHOR("<junhosj.choi@samsung.com>");
MODULE_LICENSE("GPL");
