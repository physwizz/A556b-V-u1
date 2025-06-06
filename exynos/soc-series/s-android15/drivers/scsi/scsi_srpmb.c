/*
 * Secure RPMB Driver for Exynos scsi rpmb
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/smc.h>
#include <linux/pm_wakeup.h>
#include <linux/bsg-lib.h>
#include <soc/samsung/exynos-smc.h>
#ifdef CONFIG_UFS_SRPMB_MEMLOG
#include <soc/samsung/exynos/memlogger.h>
#endif

#include <scsi/scsi.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_proto.h>

#include <ufs/ufshcd.h>
#include "ufs-cal-if.h"
#include "../ufs/host/ufs-exynos.h"

#include "scsi_srpmb.h"

#define SRPMB_DEVICE_PROPNAME	"samsung,ufs-srpmb"

struct platform_device *sr_pdev;

#ifdef CONFIG_UFS_SRPMB_MEMLOG
struct exynos_srpmb_memlog {
	struct memlog *desc;
	struct memlog_obj *log_obj_file;
	struct memlog_obj *log_obj;
	unsigned int log_enable;
};

static struct exynos_srpmb_memlog srpmb_memlog;

/*
 * Memlog is a debugging tool which stores logs in a separate space apart
 * from the Kernel Log. By using a separate area, memlog performs better
 * and ease the management.
 *
 * srpmb_log_info: If the memlog is enabled, it prints the log to memlog,
 * and if memlog is disabled, it prints the log to kernel log.
 *
 * srpmb_log_err: It prints log to both kernel log and memlog. If memlog
 * is disabled, it prints the log only to kernel log.
 */

#define srpmb_log_info(dev, fmt, ...)					\
	do {								\
		if ((&srpmb_memlog)->log_enable)			\
			memlog_write_printf((&srpmb_memlog)->log_obj,	\
					MEMLOG_LEVEL_EMERG,		\
					fmt, ##__VA_ARGS__);		\
		else							\
			dev_info(dev, fmt, ##__VA_ARGS__);		\
	} while (0)

#define srpmb_log_err(dev, fmt, ...)					\
	do {								\
		if((&srpmb_memlog)->log_enable)				\
			memlog_write_printf((&srpmb_memlog)->log_obj,	\
					MEMLOG_LEVEL_EMERG,		\
					fmt, ##__VA_ARGS__);		\
		dev_err(dev, fmt, ##__VA_ARGS__);			\
	} while (0)
#else
#define srpmb_log_info(dev, fmt, ...) dev_info(dev, fmt, ##__VA_ARGS__);
#define srpmb_log_err(dev, fmt, ...) dev_err(dev, fmt, ##__VA_ARGS__);
#endif

#if defined(DEBUG_SRPMB)
static void dump_packet(u8 *data, int len)
{
	u8 s[17];
	int i, j;

	s[16] = '\0';
	for (i = 0; i < len; i += 16) {
		printk("%06x :", i);
		for (j = 0; j < 16; j++) {
			printk(" %02x", data[i+j]);
			s[j] = (data[i+j] < ' ' ? '.' : (data[i+j] > '}' ? '.' : data[i+j]));
		}
		printk(" |%s|\n", s);
	}
	printk("\n");
}
#endif

static void swap_packet(u8 *p, u8 *d)
{
	int i;

	for (i = 0; i < RPMB_PACKET_SIZE; i++)
		d[i] = p[RPMB_PACKET_SIZE - 1 - i];
}

static inline void swap_ehs_meta(u8 *d, u8* s)
{
	int i;

	for (i = 0; i < ARPMB_EHS_META_SIZE; i++)
		d[i] = s[ARPMB_EHS_META_SIZE - 1 - i];
}

static inline u16 ufshcd_upiu_wlun_to_scsi_wlun(u8 upiu_wlun_id)
{
	return (upiu_wlun_id & ~UFS_UPIU_WLUN_ID) | SCSI_W_LUN_BASE;
}

static inline void update_arpmb_status_flag(struct rpmb_irq_ctx *ctx,
				ARpmb_Req *areq, int status)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->lock, flags);
	areq->status_flag = status;
	spin_unlock_irqrestore(&ctx->lock, flags);
}

static void update_rpmb_status_flag(struct rpmb_irq_ctx *ctx,
				Rpmb_Req *req, int status)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->lock, flags);
	req->status_flag = status;
	spin_unlock_irqrestore(&ctx->lock, flags);
}

static int advanced_srpmb_execute_cmd(ARpmb_Req *areq, char *cmd)
{
	struct request *rq = NULL;
	struct bsg_job *bjob;
	struct ufs_rpmb_request rpmb_request;
	struct ufs_rpmb_reply rpmb_reply;
	struct ufs_hba *hba = NULL;
	int dma_direction;
	int result;
	int ret = 0;
	struct ufshcd_lrb *lrbp;
	u8 *buf = NULL;
	unsigned int bufflen;
	u8 *cdb;

#ifdef CONFIG_SCSI_UFS_EXYNOS_SRPMB
	hba = exynos_ufs_get_ufs_hba();
#endif
	if (IS_ERR_OR_NULL(hba)) {
		dev_err(&sr_pdev->dev, "%s: FAIL to get ufs_hba\n", __func__);
		return -EINVAL;
	}

	if (areq->cmd == SCSI_IOCTL_SECURITY_PROTOCOL_IN)
		dma_direction = REQ_OP_DRV_IN;
	else if (areq->cmd == SCSI_IOCTL_SECURITY_PROTOCOL_OUT)
		dma_direction = REQ_OP_DRV_OUT;
	else
		return -EFAULT;

	bufflen = areq->data_len;
	if (bufflen > MAX_BUFFLEN) {
		dev_err(&sr_pdev->dev, "%s: Invalid bufflen : %x\n", __func__, bufflen);
		return -EFAULT;
	}

	if (bufflen) {
		buf = kzalloc(bufflen, GFP_KERNEL);
		if (!virt_addr_valid(buf) || IS_ERR_OR_NULL(buf))
			return -ENOMEM;

		if (areq->cmd == SCSI_IOCTL_SECURITY_PROTOCOL_OUT)
			memcpy(buf, areq->rpmb_data, bufflen);
	}

	rq = blk_mq_alloc_request(hba->bsg_queue, dma_direction, 0);
	if (IS_ERR(rq)) {
		ret = PTR_ERR(rq);
		goto free_buf;
	}

	rq->timeout = START_STOP_TIMEOUT;
	rq->rq_flags |= RQF_QUIET;

	bjob = blk_mq_rq_to_pdu(rq);

	bjob->dev = hba->dev;
	bjob->request = &rpmb_request;
	bjob->request_len = (unsigned int)(sizeof(struct ufs_rpmb_request));
	bjob->reply = &rpmb_reply;
	bjob->reply_len = (unsigned int)(sizeof(struct ufs_rpmb_reply));

	memset(&rpmb_request, 0, sizeof(struct ufs_rpmb_request));
	memset(&rpmb_reply, 0, sizeof(struct ufs_rpmb_reply));

	rpmb_request.bsg_request.msgcode = UPIU_TRANSACTION_ARPMB_CMD;
	/* Set Total EHS Length in CMD UPIU  */
	rpmb_request.bsg_request.upiu_req.header.dword_2 |= cpu_to_be32(0x2 << 24);

	memcpy(rpmb_request.bsg_request.upiu_req.sc.cdb, cmd, MAX_COMMAND_SIZE);

	/* Set Expected Data Transfer Length in upiu header */
	rpmb_request.bsg_request.upiu_req.sc.exp_data_transfer_len =
		cpu_to_be32(areq->data_len);

	rpmb_request.ehs_req.length = 0x2;
	rpmb_request.ehs_req.ehs_type = 0x1;

	switch (areq->type) {
	case GET_WRITE_COUNTER:
		/* Set Transaction Type, Flags & LUN in upiu header */
		rpmb_request.bsg_request.upiu_req.header.dword_0 |=
		       cpu_to_be32((0x1 << 24) | (UPIU_CMD_FLAGS_READ << 16) |
				       (UFS_UPIU_RPMB_WLUN << 8));

		/* Swap EHS meta space */
		swap_ehs_meta((u8 *)&rpmb_request.ehs_req.meta, (u8 *)&areq->meta);

		blk_execute_rq(rq, true);
		break;
	case WRITE_DATA:
		/* Set Transaction Type, Flags & LUN in upiu header */
		rpmb_request.bsg_request.upiu_req.header.dword_0 |=
			cpu_to_be32((0x1 << 24) | (UPIU_CMD_FLAGS_WRITE << 16) |
					(UFS_UPIU_RPMB_WLUN << 8));

		/* Swap EHS meta space */
		swap_ehs_meta((u8 *)&rpmb_request.ehs_req.meta, (u8 *)&areq->meta);

		memcpy(rpmb_request.ehs_req.mac_key, areq->mac_key, 32);

		ret = blk_rq_map_kern(rq->q, rq, buf, bufflen, GFP_NOIO);
		if (ret)
			goto out;

		blk_execute_rq(rq, true);
		break;
	case READ_DATA:
		/* Set Transaction Type, Flags & LUN in upiu header */
		rpmb_request.bsg_request.upiu_req.header.dword_0 |=
			cpu_to_be32((0x1 << 24) | (UPIU_CMD_FLAGS_READ << 16) |
					(UFS_UPIU_RPMB_WLUN << 8));

		/* Swap EHS meta space */
		swap_ehs_meta((u8 *)&rpmb_request.ehs_req.meta, (u8 *)&areq->meta);

		ret = blk_rq_map_kern(rq->q, rq, buf, bufflen, GFP_NOIO);
		if (ret)
			goto out;

		blk_execute_rq(rq, true);
		memcpy(areq->rpmb_data, buf, bufflen);
		break;
	}

	result = be32_to_cpu(rpmb_reply.bsg_reply.upiu_rsp.header.dword_1) & MASK_RSP_UPIU_RESULT;
	if (result) {
		dev_warn(&sr_pdev->dev, "%s: result return error (result = 0x%x)\n",
				__func__, result);
		if ((result & 0xFF) == 0x2) { //Check Condition
			lrbp = &hba->lrb[hba->reserved_slot];
			if (IS_ERR_OR_NULL(lrbp)) {
				ret = result;
				goto out;
			}
			cdb = (u8 *)lrbp->ucd_rsp_ptr + 32; //SENSE DATA OFFSET: 32
			dev_warn(&sr_pdev->dev,
					"%s: Check Condition Return, Sense Data Length = 0x%x\n",
					__func__, cdb[0] << 8 | cdb[1]);
			if (cdb[0] << 8 | cdb[1]) {
				dev_warn(&sr_pdev->dev,
						"%s: RESPONSE CODE = 0x%x, SENSE KEY = 0x%x\n",
						__func__, cdb[2], cdb[4]);
				ret = cdb[4]; // return SENSE KEY
				goto out;
			}
		}

		ret = result;
		goto out;
	}

	swap_ehs_meta((u8 *)&areq->meta, (u8 *)&rpmb_reply.ehs_rsp.meta);
	memcpy(areq->mac_key, rpmb_reply.ehs_rsp.mac_key, 32);

out:
	/* Fill sg_list pointer to NULL for avoiding reuse (double-free problem occurs) */
	bjob->request_payload.sg_list = NULL;

	blk_mq_free_request(rq);
free_buf:
	if (bufflen)
		kfree(buf);
	return ret;
}

static int srpmb_ioctl_secu_prot_command(struct scsi_device *sdev, char *cmd,
					Rpmb_Req *req, struct scsi_sense_hdr *sshdr,
					int timeout, int retries)
{
	int result, dma_direction;
	unsigned char *buf = NULL;
	unsigned int bufflen;
	int prot_in_out = req->cmd;
	struct scsi_exec_args args;

	if (prot_in_out == SCSI_IOCTL_SECURITY_PROTOCOL_IN) {
		dma_direction = REQ_OP_DRV_IN;
		bufflen = req->inlen;
		if (bufflen <= 0 || bufflen > MAX_BUFFLEN) {
			sdev_printk(KERN_INFO, sdev,
					"Invalid bufflen : %x\n", bufflen);
			result = -EFAULT;
			goto err_pre_buf_alloc;
		}
		buf = kzalloc(bufflen, GFP_KERNEL);
		if (!virt_addr_valid(buf)) {
			result = -ENOMEM;
			goto err_kzalloc;
		}
	} else if (prot_in_out == SCSI_IOCTL_SECURITY_PROTOCOL_OUT) {
		dma_direction = REQ_OP_DRV_OUT;
		bufflen = req->outlen;
		if (bufflen <= 0 || bufflen > MAX_BUFFLEN) {
			sdev_printk(KERN_INFO, sdev,
					"Invalid bufflen : %x\n", bufflen);
			result = -EFAULT;
			goto err_pre_buf_alloc;
		}
		buf = kzalloc(bufflen, GFP_KERNEL);
		if (!virt_addr_valid(buf)) {
			result = -ENOMEM;
			goto err_kzalloc;
		}

		if (!(req->rpmb_data[RPMB_REQRES] == 0x0 &&
					(req->rpmb_data[RPMB_REQRES + 1] > 0x0) &&
					(req->rpmb_data[RPMB_REQRES + 1] < 0x6)))
			sdev_printk(KERN_INFO, sdev, "Invalid REQ Message Type : %02x%02x\n",
					req->rpmb_data[RPMB_REQRES],
					req->rpmb_data[RPMB_REQRES + 1]);

		memcpy(buf, req->rpmb_data, bufflen);
	} else {
		sdev_printk(KERN_INFO, sdev,
				"prot_in_out not set!! %d\n", prot_in_out);
		result = -EFAULT;
		goto err_pre_buf_alloc;
	}

	memset(&args, 0, sizeof(struct scsi_exec_args));
	args.sshdr = sshdr;
	args.resid = NULL;

	result = scsi_execute_cmd(sdev, cmd, dma_direction, buf, bufflen,
				  timeout, retries,
				  &args);

	if (prot_in_out == SCSI_IOCTL_SECURITY_PROTOCOL_IN) {
		memcpy(req->rpmb_data, buf, bufflen);
	}

	if (scsi_sense_valid(sshdr)) {
		sdev_printk(KERN_INFO, sdev,
			    "ioctl_secu_prot_command return code = %x\n",
			    result);
		scsi_print_sense_hdr(sdev, NULL, sshdr);
	}

	kfree(buf);
err_pre_buf_alloc:
	return result;
err_kzalloc:
	if (buf)
		kfree(buf);
	printk(KERN_INFO "%s kzalloc faild\n", __func__);
	return result;
}

int advanced_srpmb_ioctl(struct scsi_device *sdev, ARpmb_Req *areq)
{
	char scsi_cmd[MAX_COMMAND_SIZE];
	unsigned short prot_spec;
	unsigned long t_len;
	int ret, count;

	if (!sdev) {
		printk(KERN_ERR "sdev empty\n");
		return -ENXIO;
	}

	/*
	 * If we are in the middle of error recovery, don't let anyone
	 * else try and use this device. Also, if error recovery fails, it
	 * may try and take the device offline, in which case all further
	 * access to the device is prohibited.
	 */
	if (!scsi_block_when_processing_errors(sdev))
		return -ENODEV;

	memset(scsi_cmd, 0x0, MAX_COMMAND_SIZE);

	prot_spec = SECU_PROT_SPEC_CERT_DATA;
	t_len = areq->data_len;

	scsi_cmd[0] = (areq->cmd == SCSI_IOCTL_SECURITY_PROTOCOL_IN) ?
		SECURITY_PROTOCOL_IN :
		SECURITY_PROTOCOL_OUT;
	scsi_cmd[1] = SECU_PROT_UFS;
	scsi_cmd[2] = ((unsigned char)(prot_spec >> 8)) & 0xff;
	scsi_cmd[3] = ((unsigned char)(prot_spec)) & 0xff;
	//scsi_cmd[4] = 0;
	//scsi_cmd[5] = 0;
	scsi_cmd[6] = ((unsigned char)(t_len >> 24)) & 0xff;
	scsi_cmd[7] = ((unsigned char)(t_len >> 16)) & 0xff;
	scsi_cmd[8] = ((unsigned char)(t_len >> 8)) & 0xff;
	scsi_cmd[9] = (unsigned char)t_len & 0xff;
	//scsi_cmd[10] = 0;
	//scsi_cmd[11] = 0;

	for (count = 0; count < MAX_RETRY; count++) {
		ret = advanced_srpmb_execute_cmd(areq, scsi_cmd);
		if (ret == UNIT_ATTENTION)
			dev_warn(&sr_pdev->dev, "RPMB UAC detected: Retry! (count = %d)\n",
				       count + 1);
		else
			break;
	}

	return ret;
}

int srpmb_scsi_ioctl(struct scsi_device *sdev, Rpmb_Req *req)
{
	char scsi_cmd[MAX_COMMAND_SIZE];
	unsigned short prot_spec;
	unsigned long t_len;
	struct scsi_sense_hdr sshdr;
	int ret, count;

	if (!sdev) {
		printk(KERN_ERR "sdev empty\n");
		return -ENXIO;
	}

	/*
	 * If we are in the middle of error recovery, don't let anyone
	 * else try and use this device. Also, if error recovery fails, it
	 * may try and take the device offline, in which case all further
	 * access to the device is prohibited.
	 */
	if (!scsi_block_when_processing_errors(sdev))
		return -ENODEV;

	memset(scsi_cmd, 0x0, MAX_COMMAND_SIZE);

	prot_spec = SECU_PROT_SPEC_CERT_DATA;
	if (req->cmd == SCSI_IOCTL_SECURITY_PROTOCOL_IN)
		t_len = req->inlen;
	else
		t_len = req->outlen;

	scsi_cmd[0] = (req->cmd == SCSI_IOCTL_SECURITY_PROTOCOL_IN) ?
		SECURITY_PROTOCOL_IN :
		SECURITY_PROTOCOL_OUT;
	scsi_cmd[1] = SECU_PROT_UFS;
	scsi_cmd[2] = ((unsigned char)(prot_spec >> 8)) & 0xff;
	scsi_cmd[3] = ((unsigned char)(prot_spec)) & 0xff;
	//scsi_cmd[4] = 0;
	//scsi_cmd[5] = 0;
	scsi_cmd[6] = ((unsigned char)(t_len >> 24)) & 0xff;
	scsi_cmd[7] = ((unsigned char)(t_len >> 16)) & 0xff;
	scsi_cmd[8] = ((unsigned char)(t_len >> 8)) & 0xff;
	scsi_cmd[9] = (unsigned char)t_len & 0xff;
	//scsi_cmd[10] = 0;
	//scsi_cmd[11] = 0;

	/* Retry when UAC occurs */
	for (count = 0; count < MAX_RETRY; count++) {
		ret = srpmb_ioctl_secu_prot_command(sdev, scsi_cmd,
			req, &sshdr,
			RPMB_REQ_TIMEOUT, NORMAL_RETRIES);

		if (sshdr.sense_key == UNIT_ATTENTION)
			dev_warn(&sr_pdev->dev, "RPMB UAC detected: Retry! (count = %d)\n", count + 1);
		else
			break;
	}

	return ret;
}

static void advanced_srpmb_worker(struct work_struct *data)
{
	int ret;
	struct rpmb_irq_ctx *rpmb_ctx;
	ARpmb_Req *areq;
#ifdef CONFIG_SCSI_UFS_EXYNOS_SRPMB
	struct scsi_device *wlun_sdp;
	struct scsi_target *starget;
#endif
	static struct scsi_device *sdp = NULL;

	if (!data) {
		dev_err(&sr_pdev->dev, "rpmb work_struct data invalid\n");
		return;
	}

	rpmb_ctx = container_of(data, struct rpmb_irq_ctx, work);
	if (!rpmb_ctx->dev) {
		dev_err(&sr_pdev->dev, "rpmb_ctx->dev invalid\n");
		return;
	}

	if (!rpmb_ctx->vir_addr) {
		dev_err(&sr_pdev->dev, "rpmb_ctx->vir_addr invalid\n");
		return;
	}

	areq = (ARpmb_Req *)rpmb_ctx->vir_addr;

	if (sdp == NULL) {
#ifdef CONFIG_SCSI_UFS_EXYNOS_SRPMB
		wlun_sdp = exynos_ufs_get_wlun_sdev();
		starget = scsi_target(wlun_sdp);
		sdp = __scsi_device_lookup_by_target(starget,
				ufshcd_upiu_wlun_to_scsi_wlun(UFS_UPIU_RPMB_WLUN));
#endif
		if (IS_ERR_OR_NULL(sdp)) {
			dev_err(&sr_pdev->dev, "FAIL to get scsi_device from ufs_hba\n");
			sdp = NULL;
			return;
		}
	}

	__pm_stay_awake(&rpmb_ctx->wakesrc);

	srpmb_log_info(&sr_pdev->dev, "[START] ReqType: %d, StatusFlag: 0x%X\n",
                        areq->type, areq->status_flag);

	switch (areq->type) {
	case GET_WRITE_COUNTER:
		if (areq->data_len) {
			update_arpmb_status_flag(rpmb_ctx, areq, WRITE_COUTNER_DATA_LEN_ERROR);
			srpmb_log_err(&sr_pdev->dev, "data len is invalid for GET_WRITE_COUNTER\n");
			break;
		}

		areq->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;

		ret = advanced_srpmb_ioctl(sdp, areq);
		if (ret < 0) {
			update_arpmb_status_flag(rpmb_ctx, areq, WRITE_COUTNER_SECURITY_IN_ERROR);
			srpmb_log_err(&sr_pdev->dev, "ioctl get write counter error : %x\n", ret);
			break;
		}

		if (areq->meta.result)
			srpmb_log_err(&sr_pdev->dev, "GET_WRITE_COUNTER: REQ/RES = 0x%x, RESULT = 0x%x\n",
					areq->meta.req_resp_type, areq->meta.result);

		update_arpmb_status_flag(rpmb_ctx, areq, RPMB_PASSED);

		break;
	case WRITE_DATA:
		if (areq->data_len < ARPMB_DATA_SIZE || areq->data_len > ARPMB_DATA_SIZE * 32) {
			/* intiallly set as 32 need to be change */
			srpmb_log_err(&sr_pdev->dev, "data len is invalid for WRITE_DATA\n");
			break;
		}

		areq->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;

		ret = advanced_srpmb_ioctl(sdp, areq);
		if (ret < 0) {
			update_arpmb_status_flag(rpmb_ctx, areq, WRITE_DATA_SECURITY_OUT_ERROR);
			srpmb_log_err(&sr_pdev->dev, "ioctl write data error: %x\n", ret);
			break;
		}

		if (areq->meta.result)
			srpmb_log_err(&sr_pdev->dev, "WRITE_DATA: REQ/RES = 0x%x, RESULT = 0x%x\n",
					areq->meta.req_resp_type, areq->meta.result);

		update_arpmb_status_flag(rpmb_ctx, areq, RPMB_PASSED);

		break;
	case READ_DATA:
		if (areq->data_len < ARPMB_DATA_SIZE || areq->data_len > ARPMB_DATA_SIZE * 32) {
			/* intiallly set as 32 need to be change */
			srpmb_log_err(&sr_pdev->dev, "data len is invalid for READ_DATA\n");
			break;
		}

		areq->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;

		ret = advanced_srpmb_ioctl(sdp, areq);
		if (ret < 0) {
			update_arpmb_status_flag(rpmb_ctx, areq, READ_DATA_SECURITY_IN_ERROR);
			srpmb_log_err(&sr_pdev->dev, "ioctl result read data error : %x\n", ret);
			break;
		}

		if (areq->meta.result)
			srpmb_log_info(&sr_pdev->dev, "READ_DATA: REQ/RES = 0x%x, RESULT = 0x%x\n",
					areq->meta.req_resp_type, areq->meta.result);

		update_arpmb_status_flag(rpmb_ctx, areq, RPMB_PASSED);

		break;
	default :
		srpmb_log_err(&sr_pdev->dev, "invalid requset type : %x\n", areq->type);
	}

	__pm_relax(&rpmb_ctx->wakesrc);
	srpmb_log_info(&sr_pdev->dev, "[FINISH] ReqType: %d, CMD: 0x%x, sector: %d, StatusFlag: 0x%X, \
			REQ/RES: 0x%04x, RESULT = %04x\n",
			areq->type, areq->cmd, areq->data_len, areq->status_flag,
			areq->meta.req_resp_type, areq->meta.result);
}

static void srpmb_worker(struct work_struct *data)
{
	int ret;
	struct rpmb_packet packet;
	struct rpmb_irq_ctx *rpmb_ctx;
	Rpmb_Req *req;
#ifdef CONFIG_SCSI_UFS_EXYNOS_SRPMB
	struct scsi_device *wlun_sdp;
	struct scsi_target *starget;
#endif
	static struct scsi_device *sdp = NULL;

	if (!data) {
		dev_err(&sr_pdev->dev, "rpmb work_struct data invalid\n");
		return;
	}
	rpmb_ctx = container_of(data, struct rpmb_irq_ctx, work);
	if (!rpmb_ctx->dev) {
		dev_err(&sr_pdev->dev, "rpmb_ctx->dev invalid\n");
		return;
	}

	if (!rpmb_ctx->vir_addr) {
		dev_err(&sr_pdev->dev, "rpmb_ctx->vir_addr invalid\n");
		return;
	}
	req = (Rpmb_Req *)rpmb_ctx->vir_addr;

	if (sdp == NULL) {
#ifdef CONFIG_SCSI_UFS_EXYNOS_SRPMB
		wlun_sdp = exynos_ufs_get_wlun_sdev();
		starget = scsi_target(wlun_sdp);
		sdp = __scsi_device_lookup_by_target(starget,
				ufshcd_upiu_wlun_to_scsi_wlun(UFS_UPIU_RPMB_WLUN));
#endif
		if (IS_ERR_OR_NULL(sdp)) {
			dev_err(&sr_pdev->dev, "FAIL to get scsi_device from ufs_hba\n");
			sdp = NULL;
			return;
		}
	}

	__pm_stay_awake(&rpmb_ctx->wakesrc);

	srpmb_log_info(&sr_pdev->dev, "[START] ReqType: %d, StatusFlag: 0x%X\n",
			req->type, req->status_flag);

	switch (req->type) {
	case GET_WRITE_COUNTER:
		if (req->data_len != RPMB_PACKET_SIZE) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_COUTNER_DATA_LEN_ERROR);
			srpmb_log_err(&sr_pdev->dev, "data len is invalid\n");
			break;
		}

		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
		req->outlen = RPMB_PACKET_SIZE;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_COUTNER_SECURITY_OUT_ERROR);
			srpmb_log_err(&sr_pdev->dev, "ioctl read_counter error: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0x0, req->data_len);
		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
		req->inlen = req->data_len;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_COUTNER_SECURITY_IN_ERROR);
			srpmb_log_err(&sr_pdev->dev, "ioctl error : %x\n", ret);
			break;
		}
		if (req->rpmb_data[RPMB_RESULT] || req->rpmb_data[RPMB_RESULT+1]) {
			srpmb_log_err(&sr_pdev->dev, "GET_WRITE_COUNTER: REQ/RES = %02x%02x, \
					RESULT = %02x%02x\n",
					req->rpmb_data[RPMB_REQRES], req->rpmb_data[RPMB_REQRES+1],
					req->rpmb_data[RPMB_RESULT], req->rpmb_data[RPMB_RESULT+1]);
		}

		update_rpmb_status_flag(rpmb_ctx, req, RPMB_PASSED);

		break;
	case WRITE_DATA:
		if (req->data_len < RPMB_PACKET_SIZE ||
			req->data_len > RPMB_PACKET_SIZE * 64) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_DATA_LEN_ERROR);
			srpmb_log_err(&sr_pdev->dev, "data len is invalid\n");
			break;
		}

		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
		req->outlen = req->data_len;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_DATA_SECURITY_OUT_ERROR);
			srpmb_log_err(&sr_pdev->dev, "ioctl write data error: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0x0, req->data_len);
		memset(&packet, 0x0, RPMB_PACKET_SIZE);
		packet.request = RESULT_READ_REQ;
		swap_packet((uint8_t *)&packet, req->rpmb_data);
		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
		req->outlen = RPMB_PACKET_SIZE;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_DATA_RESULT_SECURITY_OUT_ERROR);
			srpmb_log_err(&sr_pdev->dev,
					"ioctl write_data result error: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0x0, req->data_len);
		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
		req->inlen = RPMB_PACKET_SIZE;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_DATA_SECURITY_IN_ERROR);
			srpmb_log_err(&sr_pdev->dev,
					"ioctl write_data result error: %x\n", ret);
			break;
		}
		if (req->rpmb_data[RPMB_RESULT] || req->rpmb_data[RPMB_RESULT+1]) {
			srpmb_log_err(&sr_pdev->dev, "WRITE_DATA: REQ/RES = %02x%02x, RESULT = %02x%02x\n",
				req->rpmb_data[RPMB_REQRES], req->rpmb_data[RPMB_REQRES+1],
				req->rpmb_data[RPMB_RESULT], req->rpmb_data[RPMB_RESULT+1]);
		}

		update_rpmb_status_flag(rpmb_ctx, req, RPMB_PASSED);

		break;
	case READ_DATA:
		if (req->data_len < RPMB_PACKET_SIZE ||
			req->data_len > RPMB_PACKET_SIZE * 64) {
			update_rpmb_status_flag(rpmb_ctx, req, READ_LEN_ERROR);
			srpmb_log_err(&sr_pdev->dev, "data len is invalid\n");
			break;
		}

		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
		req->outlen = RPMB_PACKET_SIZE;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					READ_DATA_SECURITY_OUT_ERROR);
			srpmb_log_err(&sr_pdev->dev, "ioctl read data error: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0x0, req->data_len);
		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
		req->inlen = req->data_len;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					READ_DATA_SECURITY_IN_ERROR);
			srpmb_log_err(&sr_pdev->dev,
					"ioctl result read data error : %x\n", ret);
			break;
		}
		if (req->rpmb_data[RPMB_RESULT] || req->rpmb_data[RPMB_RESULT+1]) {
			srpmb_log_err(&sr_pdev->dev, "READ_DATA: REQ/RES = %02x%02x, RESULT = %02x%02x\n",
				req->rpmb_data[RPMB_REQRES], req->rpmb_data[RPMB_REQRES+1],
				req->rpmb_data[RPMB_RESULT], req->rpmb_data[RPMB_RESULT+1]);
		}

		update_rpmb_status_flag(rpmb_ctx, req, RPMB_PASSED);

		break;
	default:
		srpmb_log_err(&sr_pdev->dev, "invalid requset type : %x\n", req->type);
	}

	__pm_relax(&rpmb_ctx->wakesrc);

	srpmb_log_info(&sr_pdev->dev, "[FINISH] ReqType: %d, CMD: 0x%x, sector: %d, StatusFlag: 0x%X, \
			REQ/RES: 0x%02x%02x, RESULT = %02x%02x\n",
			req->type, req->cmd, req->data_len, req->status_flag,
			req->rpmb_data[RPMB_REQRES], req->rpmb_data[RPMB_REQRES+1],
			req->rpmb_data[RPMB_RESULT], req->rpmb_data[RPMB_RESULT+1]);
}

static int srpmb_suspend_notifier(struct notifier_block *nb, unsigned long event,
				void *dummy)
{
	struct rpmb_irq_ctx *rpmb_ctx;
	struct device *dev;
	Rpmb_Req *req;

	if (!nb) {
		dev_err(&sr_pdev->dev, "noti_blk work_struct data invalid\n");
		return -1;
	}
	rpmb_ctx = container_of(nb, struct rpmb_irq_ctx, pm_notifier);
	dev = rpmb_ctx->dev;
	req = (Rpmb_Req *)rpmb_ctx->vir_addr;
	if (!req) {
		dev_err(dev, "Invalid wsm address for rpmb\n");
		return -EINVAL;
	}

	srpmb_log_info(dev, "%s: event = 0x%lx\n", __func__, event);

	switch (event) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
	case PM_RESTORE_PREPARE:
		flush_workqueue(rpmb_ctx->srpmb_queue);
		if (req->status_flag != RPMB_PASSED) {
			srpmb_log_info(dev, "%s: prev status_flag = 0x%x\n",
					__func__, req->status_flag);
			update_rpmb_status_flag(rpmb_ctx, req, RPMB_FAIL_SUSPEND_STATUS);
		}
		break;
	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
		if (req->status_flag != RPMB_PASSED) {
			srpmb_log_info(dev, "%s: prev status_flag = 0x%x\n",
					__func__, req->status_flag);
			update_rpmb_status_flag(rpmb_ctx, req, 0);
		}
		break;
	default:
		break;
	}

	return 0;
}

static int advanced_srpmb_suspend_notifier(struct notifier_block *nb, unsigned long event,
				void *dummy)
{
	struct rpmb_irq_ctx *rpmb_ctx;
	struct device *dev;
	ARpmb_Req *areq;

	if (!nb) {
		dev_err(&sr_pdev->dev, "noti_blk work_struct data invalid\n");
		return -1;
	}
	rpmb_ctx = container_of(nb, struct rpmb_irq_ctx, pm_notifier);
	dev = rpmb_ctx->dev;
	areq = (ARpmb_Req *)rpmb_ctx->vir_addr;
	if (!areq) {
		dev_err(dev, "Invalid wsm address for rpmb\n");
		return -EINVAL;
	}

	switch (event) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
	case PM_RESTORE_PREPARE:
		flush_workqueue(rpmb_ctx->srpmb_queue);
		if (areq->status_flag != RPMB_PASSED)
			update_arpmb_status_flag(rpmb_ctx, areq, RPMB_FAIL_SUSPEND_STATUS);
		break;
	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
		if (areq->status_flag != RPMB_PASSED)
			update_arpmb_status_flag(rpmb_ctx, areq, 0);
		break;
	default:
		break;
	}

	return 0;
}

static irqreturn_t rpmb_irq_handler(int intr, void *arg)
{
	struct rpmb_irq_ctx *rpmb_ctx = (struct rpmb_irq_ctx *)arg;
	struct device *dev;
	Rpmb_Req *req;

	dev = rpmb_ctx->dev;
	req = (Rpmb_Req *)rpmb_ctx->vir_addr;
	if (!req) {
		dev_err(dev, "Invalid wsm address for rpmb\n");
		return IRQ_HANDLED;
	}

	update_rpmb_status_flag(rpmb_ctx, req, RPMB_IN_PROGRESS);
	queue_work(rpmb_ctx->srpmb_queue, &rpmb_ctx->work);

	return IRQ_HANDLED;
}

static irqreturn_t arpmb_irq_handler(int intr, void *arg)
{
	struct rpmb_irq_ctx *rpmb_ctx = (struct rpmb_irq_ctx *)arg;
	struct device *dev;
	ARpmb_Req *areq;

	dev = rpmb_ctx->dev;
	areq = (ARpmb_Req *)rpmb_ctx->vir_addr;
	if (!areq) {
		dev_err(dev, "Invalid wsm address for rpmb\n");
		return IRQ_HANDLED;
	}

	update_arpmb_status_flag(rpmb_ctx, areq, RPMB_IN_PROGRESS);
	queue_work(rpmb_ctx->srpmb_queue, &rpmb_ctx->work);

	return IRQ_HANDLED;
}

int init_wsm(struct device *dev, unsigned long AdvancedRPMBSupport,
		unsigned long RPMB_ReadWriteSize)
{
	int ret;
	unsigned long smc_ret;
	struct rpmb_irq_ctx *rpmb_ctx;
	struct irq_data *rpmb_irqd = NULL;
	irq_hw_number_t hwirq = 0;

	rpmb_ctx = kzalloc(sizeof(struct rpmb_irq_ctx), GFP_KERNEL);
	if (!rpmb_ctx) {
		dev_err(&sr_pdev->dev, "kzalloc failed\n");
		goto out_srpmb_ctx_alloc_fail;
	}

	/* buffer init */
	if (AdvancedRPMBSupport)
		rpmb_ctx->vir_addr = dma_alloc_coherent(&sr_pdev->dev,
				sizeof(ARpmb_Req), &rpmb_ctx->phy_addr, GFP_KERNEL);
	else
		rpmb_ctx->vir_addr = dma_alloc_coherent(&sr_pdev->dev,
				sizeof(Rpmb_Req), &rpmb_ctx->phy_addr, GFP_KERNEL);

	if (rpmb_ctx->vir_addr && rpmb_ctx->phy_addr) {
		dev_info(dev, "%s: srpmb: wsm initialized successfully\n", __func__);

		rpmb_ctx->irq = irq_of_parse_and_map(sr_pdev->dev.of_node, 0);
		if (rpmb_ctx->irq <= 0) {
			dev_err(&sr_pdev->dev, "No IRQ number, aborting\n");
			goto out_srpmb_init_fail;
		}

		/* Get irq_data for secure log */
		rpmb_irqd = irq_get_irq_data(rpmb_ctx->irq);
		if (!rpmb_irqd) {
			dev_err(&sr_pdev->dev, "Fail to get irq_data\n");
			goto out_srpmb_init_fail;
		}

		/* Get hardware interrupt number */
		hwirq = irqd_to_hwirq(rpmb_irqd);
		dev_dbg(&sr_pdev->dev, "hwirq for srpmb (%ld)\n", hwirq);

		rpmb_ctx->dev = dev;
		rpmb_ctx->srpmb_queue = alloc_workqueue("srpmb_wq",
				WQ_MEM_RECLAIM | WQ_UNBOUND | WQ_HIGHPRI, 1);
		if (!rpmb_ctx->srpmb_queue) {
			dev_err(&sr_pdev->dev,
				"Fail to alloc workqueue for ufs sprmb\n");
			goto out_srpmb_init_fail;
		}

		if (AdvancedRPMBSupport)
			ret = request_irq(rpmb_ctx->irq, arpmb_irq_handler,
					IRQF_TRIGGER_RISING, sr_pdev->name, rpmb_ctx);
		else
			ret = request_irq(rpmb_ctx->irq, rpmb_irq_handler,
					IRQF_TRIGGER_RISING, sr_pdev->name, rpmb_ctx);

		if (ret) {
			dev_err(&sr_pdev->dev, "request irq failed: %x\n", ret);
			goto out_srpmb_init_fail;
		}

		if (AdvancedRPMBSupport)
			rpmb_ctx->pm_notifier.notifier_call = advanced_srpmb_suspend_notifier;
		else
			rpmb_ctx->pm_notifier.notifier_call = srpmb_suspend_notifier;

		ret = register_pm_notifier(&rpmb_ctx->pm_notifier);
		if (ret) {
			dev_err(&sr_pdev->dev, "Failed to setup pm notifier\n");
			goto out_srpmb_free_irq_req;
		}

		memset(&rpmb_ctx->wakesrc, 0, sizeof(rpmb_ctx->wakesrc));
		(&rpmb_ctx->wakesrc)->name = "srpmb";
		wakeup_source_add(&rpmb_ctx->wakesrc);
		spin_lock_init(&rpmb_ctx->lock);

		if (AdvancedRPMBSupport)
			INIT_WORK(&rpmb_ctx->work, advanced_srpmb_worker);
		else
			INIT_WORK(&rpmb_ctx->work, srpmb_worker);

		smc_ret = exynos_smc(SMC_SRPMB_WSM, rpmb_ctx->phy_addr, hwirq, 0);
		if (smc_ret) {
			dev_err(&sr_pdev->dev, "wsm smc init failed: %lx\n", smc_ret);
			goto out_srpmb_unregister_pm;
		}

	} else {
		dev_err(&sr_pdev->dev, "wsm dma alloc failed\n");
		goto out_srpmb_dma_alloc_fail;
	}

	return 0;

out_srpmb_unregister_pm:
	wakeup_source_remove(&rpmb_ctx->wakesrc);
	unregister_pm_notifier(&rpmb_ctx->pm_notifier);
out_srpmb_free_irq_req:
	free_irq(rpmb_ctx->irq, rpmb_ctx);
out_srpmb_init_fail:
	if (rpmb_ctx->srpmb_queue)
		destroy_workqueue(rpmb_ctx->srpmb_queue);

	if (AdvancedRPMBSupport)
		dma_free_coherent(&sr_pdev->dev, sizeof(ARpmb_Req),
				rpmb_ctx->vir_addr, rpmb_ctx->phy_addr);
	else
		dma_free_coherent(&sr_pdev->dev, sizeof(Rpmb_Req),
				rpmb_ctx->vir_addr, rpmb_ctx->phy_addr);

out_srpmb_dma_alloc_fail:
	kfree(rpmb_ctx);

out_srpmb_ctx_alloc_fail:
	return -ENOMEM;
}

static inline void srpmb_get_param(unsigned long *AdvancedRPMBSupport,
		unsigned long *RPMB_ReadWriteSize)
{
#ifdef CONFIG_UFS_ADVANCED_SRPMB
	struct arm_smccc_res res;
	unsigned long bAdvancedRPMBSupport;
	unsigned long wSpecVersion;
	unsigned long bRPMB_ReadWriteSize;

	arm_smccc_smc(SMC_SRPMB_GET_PARAM, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0 == 0) {
		wSpecVersion = (unsigned long)res.a1;
		bAdvancedRPMBSupport = (unsigned long)res.a2;
		bRPMB_ReadWriteSize = (unsigned long)res.a3;
		dev_info(&sr_pdev->dev,
			"srpmb_get_param: AdvancedRPMBSupport = %ld, wSpecVersion = 0x%x\n",
			bAdvancedRPMBSupport, (int)wSpecVersion);
		dev_info(&sr_pdev->dev,
			"srpmb_get_param: RPMB_ReadWriteSize = %ld\n", bRPMB_ReadWriteSize);
	} else {
		*AdvancedRPMBSupport = 0;
		*RPMB_ReadWriteSize = 0;
		dev_info(&sr_pdev->dev,
			"srpmb_get_param smc not working, use Normal RPMB (ret = 0x%x)\n",
			(int)res.a0);
		return;
	}

	if (bAdvancedRPMBSupport && (wSpecVersion >= 0x400)) {
		*AdvancedRPMBSupport = bAdvancedRPMBSupport;
		*RPMB_ReadWriteSize = bRPMB_ReadWriteSize;
	} else {
		*AdvancedRPMBSupport = 0;
		*RPMB_ReadWriteSize = 0;
	}
#else
	*AdvancedRPMBSupport = 0;
	*RPMB_ReadWriteSize = 0;
#endif
}

#ifdef CONFIG_UFS_SRPMB_MEMLOG
static int srpmb_memlog_file_completed(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int srpmb_memlog_status_notify(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int srpmb_memlog_level_notify(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static int srpmb_memlog_enable_notify(struct memlog_obj *obj, u32 flags)
{
	/* NOP */
	return 0;
}

static const struct memlog_ops srpmb_memlog_ops = {
	.file_ops_completed = srpmb_memlog_file_completed,
	.log_status_notify = srpmb_memlog_status_notify,
	.log_level_notify = srpmb_memlog_level_notify,
	.log_enable_notify = srpmb_memlog_enable_notify,
};

static int exynos_srpmb_init_mem_log(struct platform_device *pdev)
{
	struct exynos_srpmb_memlog *memlog = &srpmb_memlog;
	struct memlog *desc;
	struct memlog_obj *log_obj;
	struct device *dev = &pdev->dev;
	int ret;

	ret =  memlog_register("ufs-rpmb-", &pdev->dev, &desc);
	if (ret) {
		dev_err(dev, "%s: failed to register memlog\n", __func__);
		return -1;
	}

	memlog->desc = desc;
	desc->ops = srpmb_memlog_ops;

	log_obj = memlog_alloc_printf(desc, SZ_64K, NULL, "log-mem", 0);

	if (log_obj) {
		memlog->log_obj = log_obj;
		memlog->log_enable = 1;
	} else {
		dev_err(dev, "%s: failed to alloc memlog memory\n", __func__);
		return -1;
	}

	dev_info(dev, "%s: complete to init srpmb memlog\n", __func__);

	return 0;
}
#else
static int exynos_srpmb_init_mem_log(struct platform_device *pdev)
{
	return 0;
}
#endif

static int srpmb_probe(struct platform_device *pdev)
{
	int ret;
	unsigned long AdvancedRPMBSupport;
	unsigned long RPMB_ReadWriteSize;
#ifdef CONFIG_SCSI_UFS_EXYNOS_SRPMB
	static struct scsi_device *sdp;
	static int retries = 1;

	sdp = exynos_ufs_get_wlun_sdev();
	if (IS_ERR_OR_NULL(sdp)) {
		sdp = NULL;
		if (retries > 200) {
			dev_err(&pdev->dev, "srpmb_probe retry execution expired\n");
			return -ENODATA;
		}
		retries++;
		return -EPROBE_DEFER;
	}
#endif
	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(36));
	dev_info(&pdev->dev, "srpmb_probe has been inited\n");

	/* Init srpmb memlog */
	ret = exynos_srpmb_init_mem_log(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Fail to initailize srpmb memlog\n");
		return ret;
	}

	sr_pdev = pdev;
	srpmb_get_param(&AdvancedRPMBSupport, &RPMB_ReadWriteSize);
	ret = init_wsm(&pdev->dev, AdvancedRPMBSupport, RPMB_ReadWriteSize);
	if (ret) {
		dev_err(&pdev->dev, "srpmb init_wsm failed: %x\n", ret);
		return ret;
	}

	return 0;
}

static const struct of_device_id of_match_table[] = {
	{ .compatible = SRPMB_DEVICE_PROPNAME },
	{ }
};

static struct platform_driver srpmb_plat_driver = {
	.probe = srpmb_probe,
	.driver = {
		.name = "exynos-ufs-srpmb",
		.owner = THIS_MODULE,
		.of_match_table = of_match_table,
	}
};

static int __init srpmb_init(void)
{
	return platform_driver_register(&srpmb_plat_driver);
}

static void __exit srpmb_exit(void)
{
	platform_driver_unregister(&srpmb_plat_driver);
}

module_init(srpmb_init);
module_exit(srpmb_exit);

MODULE_AUTHOR("Yongtaek Kwon <ycool.kwon@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("UFS SRPMB driver");
