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

#ifndef _NPU_UTIL_REGS_H_
#define _NPU_UTIL_REGS_H_

#include "include/npu-common.h"

#define NPU_REG_CMD_LEN		5
enum npu_cmd_sfr {
	NPU_CMD_SFR_READ = 0,
	NPU_CMD_SFR_WRITE,
	NPU_CMD_SFR_SMC_READ,
	NPU_CMD_SFR_SMC_WRITE
};

/* Structure used to specify SFR entry */
struct reg_set_map {
	u32             offset;
	u32             val;
	u32             mask;
};

struct reg_set_map_2 {
	struct npu_iomem_area   *iomem_area;
	u32                     offset;
	u32                     val;
	u32                     mask;
};

struct reg_cmd_contents {
	u32			cmd;
	u32			offset;
	u32			val;
	u32			mask;
	u32			mdelay;
};

struct reg_cmd_map {
	char			*name;
	struct reg_cmd_contents	contents;
};

void npu_write_hw_reg(const struct npu_iomem_area *base, u32 offset, u32 val, u32 mask, int silent);
u32 npu_read_hw_reg(const struct npu_iomem_area *base, u32 offset, u32 mask, int silent);
u32 npu_smc_read_hw_reg(const struct npu_iomem_area *base,
					u32 offset, u32 mask, int silent);
void npu_smc_write_hw_reg(const struct npu_iomem_area *base,
					u32 offset, u32 val, u32 mask, int silent);
int npu_set_hw_reg(const struct npu_iomem_area *base, const struct reg_set_map *set_map,
	size_t map_len, unsigned long regset_mdelay);
int npu_set_hw_reg_2(const struct reg_set_map_2 *set_map, size_t map_len, unsigned long regset_mdelay);
int npu_set_sfr(const u32 sfr_addr, const u32 value, const u32 mask);
int npu_get_sfr(const u32 sfr_addr);
int npu_cmd_map(struct npu_system *system, const char *cmd_name);
int npu_cmd_map_dump(struct npu_system *system, const char *cmd_name);
int npu_get_reg_cmd_map(struct npu_system *system, struct reg_cmd_list *cmd_list);
void npu_reg_dump(struct npu_device *device);

/* global structure for iomem area */
extern struct npu_iomem_init_data init_data[];

#endif	/* _NPU_UTIL_REGS_H_ */
