// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_HW_IP_H
#define IS_HW_IP_H

#include "is-hw.h"
#include "is-interface.h"

int is_hw_3aa_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_3aa_remove(struct is_hw_ip *hw_ip);
int is_hw_isp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_isp_remove(struct is_hw_ip *hw_ip);
int is_hw_byrp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_byrp_remove(struct is_hw_ip *hw_ip);
int is_hw_rgbp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_rgbp_remove(struct is_hw_ip *hw_ip);
int is_hw_mcsc_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_mcsc_remove(struct is_hw_ip *hw_ip);
int is_hw_vra_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_vra_remove(struct is_hw_ip *hw_ip);
int is_hw_ypp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_ypp_remove(struct is_hw_ip *hw_ip);
int is_hw_yuvp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_yuvp_remove(struct is_hw_ip *hw_ip);
int is_hw_shrp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_shrp_remove(struct is_hw_ip *hw_ip);
int is_hw_mcfp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_mcfp_remove(struct is_hw_ip *hw_ip);
int is_hw_lme_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_lme_remove(struct is_hw_ip *hw_ip);
int is_hw_cstat_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_cstat_remove(struct is_hw_ip *hw_ip);
int is_hw_orbmch_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void is_hw_orbmch_remove(struct is_hw_ip *hw_ip);
int pablo_hw_rgbp_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void pablo_hw_rgbp_remove(struct is_hw_ip *hw_ip);
int pablo_hw_dlfe_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void pablo_hw_dlfe_remove(struct is_hw_ip *hw_ip);
int pablo_hw_yuvsc_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void pablo_hw_yuvsc_remove(struct is_hw_ip *hw_ip);
int pablo_hw_mtnr0_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void pablo_hw_mtnr0_remove(struct is_hw_ip *hw_ip);
int pablo_hw_mtnr1_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void pablo_hw_mtnr1_remove(struct is_hw_ip *hw_ip);
int pablo_hw_msnr_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void pablo_hw_msnr_remove(struct is_hw_ip *hw_ip);
int pablo_hw_mlsc_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
	struct is_interface_ischain *itfc, int id, const char *name);
void pablo_hw_mlsc_remove(struct is_hw_ip *hw_ip);
void is_hw_mcsc_set_ni(struct is_hardware *hardware, struct is_frame *frame,
	u32 instance);
#endif
