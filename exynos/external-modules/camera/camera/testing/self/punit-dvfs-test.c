// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2023 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "punit-dvfs-test.h"
#include "punit-group-test.h"
#include "is-core.h"

static int punit_dvfs_checker(
	struct punit_test_criteria *criteria, struct is_group *head, struct is_frame *frame)
{
	int res;
	struct is_dvfs_ctrl *dvfs_ctrl = &is_get_is_core()->resourcemgr.dvfs_ctrl;

	switch (criteria->check_variable) {
	case PUNIT_DVFS_VAR_STATIC:
		criteria->actual_value = dvfs_ctrl->static_ctrl->cur_scenario_id;
		break;
	case PUNIT_DVFS_VAR_DYNAMIC:
		criteria->actual_value = dvfs_ctrl->dynamic_ctrl->cur_scenario_id;
		break;
	case PUNIT_DVFS_VAR_THROTT:
		criteria->actual_value = dvfs_ctrl->thrott_ctrl;
		break;
	case PUNIT_DVFS_VAR_CAM_LV:
		criteria->actual_value = dvfs_ctrl->cur_lv[IS_DVFS_CAM];
		break;
	case PUNIT_DVFS_VAR_ISP_LV:
		criteria->actual_value = dvfs_ctrl->cur_lv[IS_DVFS_ISP];
		break;
	case PUNIT_DVFS_VAR_INTCAM_LV:
		criteria->actual_value = dvfs_ctrl->cur_lv[IS_DVFS_INT_CAM];
		break;
	case PUNIT_DVFS_VAR_MIF_LV:
		criteria->actual_value = dvfs_ctrl->cur_lv[IS_DVFS_MIF];
		break;
	case PUNIT_DVFS_VAR_INT_LV:
		criteria->actual_value = dvfs_ctrl->cur_lv[IS_DVFS_INT];
		break;
	case PUNIT_DVFS_VAR_ICPU_LV:
		criteria->actual_value = dvfs_ctrl->cur_lv[IS_DVFS_ICPU];
		break;
	default:
		criteria->actual_value = PGT_UNCHECKED;
		break;
	}
	res = PUNIT_EXPECT(
		criteria->check_operator, criteria->actual_value, criteria->expect_value);
	pr_info("%s : actual_value (%d) / expect_value (%d)\n", __func__, criteria->actual_value,
		criteria->expect_value);

	return res;
}

static int punit_dvfs_result_formatter(struct punit_test_criteria *criteria, char *buffer)
{
	int ret;
	char type_str[256];

	switch (criteria->check_variable) {
	case PUNIT_DVFS_VAR_CAM_LV:
	case PUNIT_DVFS_VAR_ISP_LV:
	case PUNIT_DVFS_VAR_INTCAM_LV:
	case PUNIT_DVFS_VAR_MIF_LV:
	case PUNIT_DVFS_VAR_ICPU_LV:
		sprintf(type_str, "LV");
		break;
	default:
		sprintf(type_str, "SCENRAIO");
		break;
	}

	ret = sprintf(buffer, "%s [%s][%d][F#%d] DVFS %s acture (%d) : expect (%d) %s",
		CHECK_RES(criteria->check_result), FNAME(PUNIT_DVFS_TEST), criteria->instance,
		criteria->fcount, type_str, criteria->actual_value, criteria->expect_value,
		criteria->desc);
	return ret;
}

static const struct punit_checker_ops punit_dvfs_checker_ops = {
	.dque_checker = punit_dvfs_checker,
	.result_formatter = punit_dvfs_result_formatter,
};

const struct punit_checker_ops *punit_get_dvfs_checker_ops(void)
{
	return &punit_dvfs_checker_ops;
}
