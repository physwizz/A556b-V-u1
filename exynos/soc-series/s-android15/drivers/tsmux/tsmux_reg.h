/*
 * copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS TSMUX driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TSMUX_REG_H
#define TSMUX_REG_H

#include <linux/kernel.h>

#include "tsmux_dev.h"

void tsmux_set_partial_done_info(int enable_partial_done,
		int rtp_pkt_count_per_partial_data);

void tsmux_set_log_level(int log_level);

void tsmux_set_debug_level(int debug_level);

void tsmux_set_sfr_dump_addr(void *sfr_dump_addr);

void tsmux_ioremap_cmu_mfc_sfr(struct tsmux_device *tsmux_dev);

void tsmux_print_cmu_mfc_sfr(struct tsmux_device *tsmux_dev);

void tsmux_ioremap_wfd_pactive_sfr(struct tsmux_device *tsmux_dev);

void tsmux_set_wfd_pactive(struct tsmux_device *tsmux_dev, bool enable);

uint32_t tsmux_get_hw_version(struct tsmux_device *tsmux_dev);

void tsmux_print_dbg_info_all(struct tsmux_device *tsmux_dev);

void tsmux_print_tsmux_sfr(struct tsmux_device *tsmux_dev);

void tsmux_set_psi_info(struct tsmux_device *tsmux_dev,
	struct tsmux_psi_info *psi_info);

void tsmux_set_pkt_ctrl(struct tsmux_device *tsmux_dev,
	struct tsmux_pkt_ctrl *pkt_ctrl);

void tsmux_set_pes_hdr(struct tsmux_device *tsmux_dev,
	struct tsmux_pes_hdr *pes_hdr);

void tsmux_set_ts_hdr(struct tsmux_device *tsmux_dev,
	struct tsmux_ts_hdr *ts_hdr);

void tsmux_set_rtp_hdr(struct tsmux_device *tsmux_dev,
	struct tsmux_rtp_hdr *rtp_hdr);

void tsmux_set_swp_ctrl(struct tsmux_device *tsmux_dev,
	struct tsmux_swp_ctrl *swp_ctrl);

void tsmux_clear_hex_ctrl(void);

void tsmux_set_hex_ctrl(struct tsmux_context *ctx,
	struct tsmux_hex_ctrl *hex_ctrl);

void tsmux_set_src_addr(struct tsmux_device *tsmux_dev,
	struct tsmux_buffer_info *buf_info);

void tsmux_set_src_len(struct tsmux_device *tsmux_dev, int src_len);

void tsmux_set_dst_addr(struct tsmux_device *tsmux_dev,
	struct tsmux_buffer_info *buf_info);

void tsmux_job_queue_pkt_ctrl(struct tsmux_device *tsmux_dev);

bool tsmux_reset_pkt_ctrl(struct tsmux_device *tsmux_dev);

void tsmux_enable_int_job_done(struct tsmux_device *tsmux_dev, bool need_partial_done);
void tsmux_disable_int_job_done(struct tsmux_device *tsmux_dev);

void tsmux_set_cmd_ctrl(struct tsmux_device *tsmux_dev, int mode);

bool tsmux_is_job_done_id(struct tsmux_device *tsmux_dev, int job_id);

void tsmux_clear_job_done(struct tsmux_device *tsmux_dev, int job_id);

bool tsmux_is_partial_done_id(struct tsmux_device *tsmux_dev, int job_id);

void tsmux_clear_partial_done(struct tsmux_device *tsmux_dev, u32 job_id);

int tsmux_get_dst_len(struct tsmux_device *tsmux_dev, int job_id);

#ifdef ASB_TEST
void tsmux_set_normal_sfr_cmd(
	struct tsmux_device *tsmux_dev, struct tsmux_reg *cmd, int count);
void tsmux_set_secure_sfr_cmd(
	struct tsmux_device *tsmux_dev, struct tsmux_reg *cmd, int count);
#endif

#endif
