// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-core.h"
#include "is-votfmgr.h"
#include "is-votf-id-table.h"
#include "votf/pablo-votf.h"
#include "pablo-debug.h"

static unsigned int is_votf_ip[GROUP_ID_MAX] = {
	[GROUP_ID_3AA0 ... GROUP_ID_MAX-1] = 0xFFFFFFFF,

	[GROUP_ID_SS0]	= VOTF_CSIS_W0,
	[GROUP_ID_SS1]	= VOTF_CSIS_W0,
	[GROUP_ID_SS2]	= VOTF_CSIS_W0,
	[GROUP_ID_SS3]	= VOTF_CSIS_W0,

	[GROUP_ID_3AA0] = VOTF_CSTAT_R,
	[GROUP_ID_3AA1] = VOTF_CSTAT_R,
	[GROUP_ID_3AA2] = VOTF_CSTAT_R,

	[GROUP_ID_BYRP] = VOTF_BYRP_W,

	[GROUP_ID_YUVP] = VOTF_YUVP_R,

	[GROUP_ID_RGBP] = VOTF_RGBP_W,
	[GROUP_ID_MCS0] = VOTF_MCSC_R,
};

static unsigned int is_votf_id[GROUP_ID_MAX][ENTRY_END] = {
	[GROUP_ID_3AA0 ... GROUP_ID_MAX-1][ENTRY_SENSOR ... ENTRY_END-1] = 0xFFFFFFFF,

	[GROUP_ID_SS0][ENTRY_SSVC0]	= 0,
	[GROUP_ID_SS0][ENTRY_SSVC1]	= 1,
	[GROUP_ID_SS0][ENTRY_SSVC2]	= 2,
	[GROUP_ID_SS0][ENTRY_SSVC3]	= 3,

	[GROUP_ID_SS1][ENTRY_SSVC0]	= 4,
	[GROUP_ID_SS1][ENTRY_SSVC1]	= 5,
	[GROUP_ID_SS1][ENTRY_SSVC2]	= 6,
	[GROUP_ID_SS1][ENTRY_SSVC3]	= 7,

	[GROUP_ID_SS2][ENTRY_SSVC0]	= 8,
	[GROUP_ID_SS2][ENTRY_SSVC1]	= 9,
	[GROUP_ID_SS2][ENTRY_SSVC2]	= 10,
	[GROUP_ID_SS2][ENTRY_SSVC3]	= 11,

	[GROUP_ID_SS3][ENTRY_SSVC0]	= 12,
	[GROUP_ID_SS3][ENTRY_SSVC1]	= 13,
	[GROUP_ID_SS3][ENTRY_SSVC2]	= 14,
	[GROUP_ID_SS3][ENTRY_SSVC3]	= 15,

	[GROUP_ID_3AA0][ENTRY_3AA]	= 0,
	[GROUP_ID_3AA1][ENTRY_3AA]	= 1,
	[GROUP_ID_3AA2][ENTRY_3AA]	= 2,

	[GROUP_ID_BYRP][ENTRY_BYRP]	= 0,

	[GROUP_ID_YUVP][ENTRY_YUVP] = 0,

	[GROUP_ID_RGBP][ENTRY_RGBP_HF]	= 0,
	[GROUP_ID_MCS0][ENTRY_MCSC_P5] = 0,

};

static int is_votf_set_service_cfg(struct votf_info *src_info,
		struct votf_info *dst_info,
		u32 width, u32 height)
{
	int ret = 0;
	struct votf_service_cfg cfg;

	memset(&cfg, 0, sizeof(struct votf_service_cfg));

	/* TRS: Slave */
	dst_info->service = TRS;

	cfg.enable = 0x1;
	/*
	 * 0xFF is max value.
	 * Buffer size is (limit x token_size).
	 * But VOTF can hold only 1 frame.
	 */
	cfg.limit = 0xFF;
	cfg.width = width;
	cfg.height = height;
	cfg.token_size = is_votf_get_token_size(dst_info);
	cfg.connected_ip = src_info->ip;
	cfg.connected_id = src_info->id;
	cfg.option = VOTF_OPTION_MSK_CHANGE;

	ret = votfitf_set_service_cfg(dst_info, &cfg);
	if (ret <= 0) {
		ret = -EINVAL;
		err("TRS votf set_service_cfg fail. TWS 0x%04X-%d TRS 0x%04X-%d",
				src_info->ip, src_info->id,
				dst_info->ip, dst_info->id);
		return ret;
	}

	return 0;
}

int is_get_votf_ip(int group_id)
{
	return is_votf_ip[group_id];
}

int is_get_votf_id(int group_id, int entry)
{
	return is_votf_id[group_id][entry];
}

u32 is_votf_get_token_size(struct votf_info *vinfo)
{
	u32 lines_in_token;

	switch (vinfo->ip) {
	case VOTF_CSIS_W0:
	case VOTF_PDP:
		if (vinfo->mode == VOTF_FRS) {
			lines_in_token = 40;
			break;
		} else if (vinfo->mode == VOTF_TRS_HEIGHT_X2) {
			if (vinfo->service == TRS) {
				lines_in_token = 2;
				break;
			}
		}

		fallthrough;
	case VOTF_RGBP_W:
	case VOTF_MCSC_R:
		lines_in_token = 1;
		break;
	default:
		lines_in_token = 1;
		break;
	};

	return lines_in_token;
}

/* This function should be called in TRS */
void is_votf_get_master_vinfo(struct is_group *group, struct is_group **src_gr, int *src_id, int *src_entry)
{
	int group_id = group->id;
	struct is_device_ischain *device;

	device = group->device;

	switch (group_id) {
	case GROUP_ID_3AA0:
		*src_gr = group->prev;
		*src_id = group->prev->id;
		*src_entry = group->prev->junction->id;
		break;
	case GROUP_ID_MCS0:
		*src_gr = device->group[GROUP_SLOT_RGBP];
		*src_id = GROUP_ID_RGBP;
		*src_entry = ENTRY_RGBP_HF;
		break;
	default:
		mgerr("Invalid group id(%d)\n", group, group, group_id);
		break;
	}
}

/* This function should be called in TRS */
void is_votf_get_slave_vinfo(struct is_group *group, struct is_group **dst_gr, int *dst_id, int *dst_entry)
{
	int group_id = group->id;

	switch (group_id) {
	case GROUP_ID_3AA0:
		*dst_gr = group;
		*dst_id = group->id;
		*dst_entry = group->leader.id;
		break;
	case GROUP_ID_MCS0:
		/* TRS */
		*dst_gr = group;
		*dst_id = GROUP_ID_MCS0;
		*dst_entry = ENTRY_M5P;
		break;
	default:
		mgerr("Invalid group id(%d)\n", group, group, group_id);
		break;
	}
}

int is_hw_pdp_set_votf_config(struct is_group *group, struct is_sensor_cfg *s_cfg)
{
	/* PDP v4.0/v4.1 doesn't support VOTF input */
	return -EINVAL;
}

int is_hw_mcsc_set_votf_size_config(u32 width, u32 height)
{
	struct votf_info src_info, dst_info;
	int ret = 0;

	/*********** L0 Y ***********/
	src_info.service = TWS;
	src_info.ip = VOTF_RGBP_W;
	src_info.id = 0;

	dst_info.service = TRS;
	dst_info.ip = VOTF_MCSC_R;
	dst_info.id = 0;

	ret = is_votf_set_service_cfg(&src_info, &dst_info, width, height);
	if (ret < 0) {
		err("HF votf set_service_cfg fail. TWS 0x%04X-%d TRS 0x%04X-%d",
				src_info.ip, src_info.id,
				dst_info.ip, dst_info.id);

		return ret;
	}

	return 0;
}

void is_votf_subdev_flush(struct is_group *group)
{
	struct is_group *src_gr = NULL;

	struct votf_info src_info, dst_info;
	int ret;
	unsigned int src_id, src_entry;

	is_votf_get_master_vinfo(group, &src_gr, &src_id, &src_entry);

	if (src_gr && src_gr->id == GROUP_ID_RGBP && group->id == GROUP_ID_MCS0) {
		src_info.ip = VOTF_RGBP_W;
		src_info.id = RGBP_HF;

		dst_info.ip = VOTF_MCSC_R;
		dst_info.id = MCSC_HF;

		ret = __is_votf_force_flush(&src_info, &dst_info);
		if (ret)
			mgerr("L0_UV votf flush fail. ret %d",
				group, group, ret);
	}
}

int is_votf_subdev_create_link(struct is_group *group)
{
	struct is_group *src_gr = NULL;
	struct votf_info src_info, dst_info;
	u32 width, height;
	u32 change_option = VOTF_OPTION_MSK_COUNT;
	int ret;
	unsigned int src_id, src_entry;

	is_votf_get_master_vinfo(group, &src_gr, &src_id, &src_entry);
	if (!src_gr)
		return -EINVAL;

	/* size will set in shot routine */
	width = height = 0;

	if (src_gr->id == GROUP_ID_RGBP && group->id == GROUP_ID_MCS0) {
		src_info.ip = VOTF_RGBP_W;
		src_info.id = RGBP_HF;
		src_info.mode = VOTF_NORMAL;

		dst_info.ip = VOTF_MCSC_R;
		dst_info.id = MCSC_HF;
		dst_info.mode = VOTF_NORMAL;

#if defined(USE_VOTF_AXI_APB)
		votfitf_create_link(src_info.ip, dst_info.ip);
#endif
		ret = is_votf_link_set_service_cfg(&src_info, &dst_info,
				width, height, change_option);
		if (ret)
			return ret;
	}

	return 0;
}

#if defined(USE_VOTF_AXI_APB)
void is_votf_subdev_destroy_link(struct is_group *group)
{
	struct is_group *src_gr = NULL;
	unsigned int src_id, src_entry;

	is_votf_get_master_vinfo(group, &src_gr, &src_id, &src_entry);

	if (src_gr && src_gr->id == GROUP_ID_RGBP && group->id == GROUP_ID_MCS0)
		votfitf_destroy_link(VOTF_RGBP_W, VOTF_MCSC_R);
}
#endif
