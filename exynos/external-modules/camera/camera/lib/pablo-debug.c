/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/wait.h>

#include "pablo-debug.h"
#include "exynos-is.h"
#include "pablo-binary.h"
#include "pablo-lib.h"
#include "is-video.h"
#include "pablo-blob.h"
#include "pablo-debug-param.h"

static struct is_debug is_debug;
static struct is_debug_event is_debug_event;

#define DEBUG_FS_ROOT_NAME	"is"
#define DEBUG_FS_LOGFILE_NAME	"isfw-msg"
#define DEBUG_FS_EVENT_DIR_NAME	"event"
#define DEBUG_FS_EVENT_FILTER "filter"
#define DEBUG_FS_EVENT_LOGFILE "log"
#define DEBUG_FS_EVENT_LOGEN "log_enable"

static const struct file_operations dbg_log_fops;
static const struct file_operations dbg_event_fops;

#define DBG_DIGIT_CNT	5 /* Max count of total digit */
#define DBG_DIGIT_W	6
#define DBG_DIGIT_H	9

#define CALL_PML_OPS(pml, op, args...)	\
	(((pml).ops && (pml).ops->op) ? ((pml).ops->op(args)) : 0)
#define CALL_PBCM_OPS(debug, op, args...) \
	(debug.bcm_ops->op(args))
#define CALL_PDSS_OPS(debug, op, args...) \
	(debug.dss_ops->op(args))

#define pablo_module_param(name, param_num)                                                        \
	module_param_cb(name, &pablo_debug_param_ops, &debug_param[param_num], 0644)

#define IS_STR_TYPE_PARAM(param) (param->type == IS_DEBUG_PARAM_TYPE_STR)

/* sysfs global variable for debug */
static struct is_sysfs_debug sysfs_debug;
static struct is_sysfs_sensor sysfs_sensor;
/* sysfs global variable for set position to actuator */
static struct is_sysfs_actuator sysfs_actuator;
/* sysfs global variable for eeprom */
static struct is_sysfs_eeprom sysfs_eeprom;

static int pablo_debug_param_set(const char *val, const struct kernel_param *kp);
static int pablo_debug_param_get(char *buffer, const struct kernel_param *kp);

static int draw_digit_set(const char *val);
static int draw_digit_get(char *buffer, const size_t buf_size);
static int draw_digit_usage(char *buffer, const size_t buf_size);

static const char *const pablo_debug_param_type_names[] = {
	"NUM",
	"BIT",
	"STR",
};

#define PABLO_DEBUG_PARAM_TYPE(type)                                                               \
	((type) < IS_DEBUG_PARAM_TYPE_MAX ? pablo_debug_param_type_names[(type)] : "UNKNOWN")

const struct kernel_param_ops pablo_debug_param_ops = {
	.set = pablo_debug_param_set,
	.get = pablo_debug_param_get,
};
EXPORT_SYMBOL(pablo_debug_param_ops);

static struct pablo_debug_param debug_param[IS_DEBUG_PARAM_MAX] = {
	[IS_DEBUG_PARAM_CLK] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 2,
		.ops.usage = param_debug_clock_usage,
	},
	[IS_DEBUG_PARAM_STREAM] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 6,
		.ops.usage = param_debug_stream_usage,
	},
	[IS_DEBUG_PARAM_VIDEO] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_usage,
	},
	[IS_DEBUG_PARAM_HW] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 4,
		.ops.usage = param_debug_hw_usage,
	},
	[IS_DEBUG_PARAM_DEVICE] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_usage,
	},
	[IS_DEBUG_PARAM_IRQ] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 2,
		.ops.usage = param_debug_irq_usage,
	},
	[IS_DEBUG_PARAM_CSI] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 5,
		.ops.usage = param_debug_csi_usage,
	},
	[IS_DEBUG_PARAM_TIME_LAUNCH] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_usage,
	},
	[IS_DEBUG_PARAM_TIME_QUEUE] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = UINT_MAX,
		.ops.usage = param_debug_time_queue_usage,
	},
	[IS_DEBUG_PARAM_TIME_SHOT] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = UINT_MAX,
		.ops.usage = param_debug_time_shot_usage,
	},
	[IS_DEBUG_PARAM_S2D] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_usage,
	},
	[IS_DEBUG_PARAM_DVFS] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 2,
		.ops.usage = param_debug_dvfs_usage,
	},
	[IS_DEBUG_PARAM_MEM] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 2,
		.ops.usage = param_debug_mem_usage,
	},
	[IS_DEBUG_PARAM_LVN] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 4,
		.ops.usage = param_debug_lvn_usage,
	},
	[IS_DEBUG_PARAM_LLC] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = UINT_MAX,
		.ops.usage = param_debug_llc_usage,
	},
	[IS_DEBUG_PARAM_YUVP] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_hw_dump_usage,
	},
	[IS_DEBUG_PARAM_SHRP] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_hw_dump_usage,
	},
	[IS_DEBUG_PARAM_IQ] = {
		.type = IS_DEBUG_PARAM_TYPE_BIT,
		.max_value = LONG_MAX,
		.ops.usage = param_debug_iq_usage,
	},
	[IS_DEBUG_PARAM_IQ_CR_DUMP] = {
		.type = IS_DEBUG_PARAM_TYPE_BIT,
		.max_value = LONG_MAX,
		.ops.usage = param_debug_iq_usage,
	},
	[IS_DEBUG_PARAM_PHY_TUNE] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 2,
		.ops.usage = param_debug_phy_tune_usage,
	},
	[IS_DEBUG_PARAM_CRC_SEED] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = UINT_MAX,
		.ops.usage = param_debug_crc_seed_usage,
	},
	[IS_DEBUG_PARAM_DRAW_DIGIT] = {
		.type = IS_DEBUG_PARAM_TYPE_STR,
		.ops.set = draw_digit_set,
		.ops.get = draw_digit_get,
		.ops.usage = draw_digit_usage,
	},
	[IS_DEBUG_PARAM_SENSOR] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 7,
		.ops.usage = param_debug_sensor_usage,
	},
	[IS_DEBUG_PARAM_ASSERT_CRASH] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_usage,
	},
	[IS_DEBUG_PARAM_IXC] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_usage,
	},
	[IS_DEBUG_PARAM_ICPU_RELOAD] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_usage,
	},
	[IS_DEBUG_PARAM_DISABLE_CRTA] = {
		.type = IS_DEBUG_PARAM_TYPE_NUM,
		.max_value = 1,
		.ops.usage = param_debug_usage,
	},
};

static int pablo_debug_param_usage(
	char *buffer, size_t buf_size, const char *param_name, struct pablo_debug_param *param)
{
	int ret;

	ret = scnprintf(
		buffer, buf_size, "%s usage: echo [value|option] > <target param>\n", param_name);
	if (param->ops.usage)
		ret += param->ops.usage(buffer + ret, buf_size - ret);
	ret += scnprintf(buffer + ret, buf_size - ret, "[option]\n");
	ret += scnprintf(
		buffer + ret, buf_size - ret, "\t%-8s : print usage when calling get()\n", "usage");
	ret += scnprintf(
		buffer + ret, buf_size - ret, "\t%-8s : print type when calling get()\n", "type");
	if (!IS_STR_TYPE_PARAM(param))
		ret += scnprintf(buffer + ret, buf_size - ret,
			"\t%-8s : print max value when calling get()\n", "max");
	ret += scnprintf(buffer + ret, buf_size - ret,
		"\t%-8s : print type, max, value when calling get()\n", "query");

	return ret;
}

static int pablo_debug_param_value(
	char *buffer, size_t buf_size, const struct pablo_debug_param *param)
{
	if (param->ops.get)
		return param->ops.get(buffer, buf_size);
	else
		return scnprintf(buffer, buf_size, "%lu", param->value);
}

static int pablo_debug_param_set(const char *val, const struct kernel_param *kp)
{
	struct pablo_debug_param *param = (struct pablo_debug_param *)kp->arg;
	int ret = 0;

	if (!strncmp(val, "usage", 5)) {
		param->mode = IS_DEBUG_PARAM_MODE_GET_USAGE;
	} else if (!strncmp(val, "type", 4)) {
		param->mode = IS_DEBUG_PARAM_MODE_GET_TYPE;
	} else if (!strncmp(val, "max", 3)) {
		param->mode = IS_DEBUG_PARAM_MODE_GET_MAX;
	} else if (!strncmp(val, "query", 5)) {
		param->mode = IS_DEBUG_PARAM_MODE_GET_QUERY;
	} else {
		param->mode = IS_DEBUG_PARAM_MODE_GET_VAL;
		if (param->ops.set)
			ret = param->ops.set(val);
		else
			ret = kstrtoul(val, 0, &param->value);
	}

	return ret;
}

static int pablo_debug_param_get(char *buffer, const struct kernel_param *kp)
{
	struct pablo_debug_param *param = (struct pablo_debug_param *)kp->arg;
	size_t buf_size = PAGE_SIZE;
	int ret;

	if (param->mode == IS_DEBUG_PARAM_MODE_GET_USAGE) {
		ret = pablo_debug_param_usage(buffer, buf_size, kp->name, param);
	} else if (param->mode == IS_DEBUG_PARAM_MODE_GET_MAX && !IS_STR_TYPE_PARAM(param)) {
		ret = scnprintf(buffer, buf_size, "max %lu\n", param->max_value);
	} else if (param->mode == IS_DEBUG_PARAM_MODE_GET_TYPE) {
		ret = scnprintf(buffer, buf_size, "%s\n", PABLO_DEBUG_PARAM_TYPE(param->type));
	} else if (param->mode == IS_DEBUG_PARAM_MODE_GET_QUERY) {
		ret = scnprintf(
			buffer, buf_size, "type : %s\n", PABLO_DEBUG_PARAM_TYPE(param->type));
		if (!IS_STR_TYPE_PARAM(param))
			ret += scnprintf(
				buffer + ret, buf_size - ret, "max : %lu\n", param->max_value);
		ret += scnprintf(buffer + ret, buf_size - ret, "value : ");
		ret += pablo_debug_param_value(buffer + ret, buf_size - ret, param);
	} else {
		ret = pablo_debug_param_value(buffer, buf_size, param);
	}

	param->mode = IS_DEBUG_PARAM_MODE_GET_VAL;
	return ret;
}

pablo_module_param(debug_clk, IS_DEBUG_PARAM_CLK);
pablo_module_param(debug_stream, IS_DEBUG_PARAM_STREAM);
pablo_module_param(debug_video, IS_DEBUG_PARAM_VIDEO);
pablo_module_param(debug_hw, IS_DEBUG_PARAM_HW);
pablo_module_param(debug_device, IS_DEBUG_PARAM_DEVICE);
pablo_module_param(debug_irq, IS_DEBUG_PARAM_IRQ);
pablo_module_param(debug_csi, IS_DEBUG_PARAM_CSI);
pablo_module_param(debug_time_launch, IS_DEBUG_PARAM_TIME_LAUNCH);
pablo_module_param(debug_time_queue, IS_DEBUG_PARAM_TIME_QUEUE);
pablo_module_param(debug_time_shot, IS_DEBUG_PARAM_TIME_SHOT);
pablo_module_param(debug_s2d, IS_DEBUG_PARAM_S2D);
pablo_module_param(debug_dvfs, IS_DEBUG_PARAM_DVFS);
pablo_module_param(debug_mem, IS_DEBUG_PARAM_MEM);
pablo_module_param(debug_lvn, IS_DEBUG_PARAM_LVN);
pablo_module_param(debug_llc, IS_DEBUG_PARAM_LLC);
pablo_module_param(debug_yuvp, IS_DEBUG_PARAM_YUVP);
pablo_module_param(debug_shrp, IS_DEBUG_PARAM_SHRP);
pablo_module_param(debug_iq, IS_DEBUG_PARAM_IQ);
pablo_module_param(debug_iq_cr_dump, IS_DEBUG_PARAM_IQ_CR_DUMP);
pablo_module_param(debug_phy_tune, IS_DEBUG_PARAM_PHY_TUNE);
pablo_module_param(debug_crc_seed, IS_DEBUG_PARAM_CRC_SEED);
pablo_module_param(draw_digit, IS_DEBUG_PARAM_DRAW_DIGIT);
pablo_module_param(debug_sensor, IS_DEBUG_PARAM_SENSOR);
pablo_module_param(debug_assert_crash, IS_DEBUG_PARAM_ASSERT_CRASH);
pablo_module_param(debug_ixc, IS_DEBUG_PARAM_IXC);
pablo_module_param(debug_icpu_reload, IS_DEBUG_PARAM_ICPU_RELOAD);
pablo_module_param(debug_disable_crta, IS_DEBUG_PARAM_DISABLE_CRTA);

/*
 * Decimal digit dot pattern.
 * 6 bits consist a single line of 6x9 digit pattern.
 *
 * e.g) Digit 2 pattern
 *    [5][4][3][2][1][0]
 * [0] 0  0  0  0  0  0 : 0x00
 * [1] 0  1  1  1  1  0 : 0x1E
 * [2] 0  0  0  0  1  0 : 0x02
 * [3] 0  0  0  0  1  0 : 0x02
 * [4] 0  1  1  1  1  0 : 0x1E
 * [5] 0  1  0  0  0  0 : 0x10
 * [6] 0  1  0  0  0  0 : 0x10
 * [7] 0  1  1  1  1  0 : 0x1E
 * [8] 0  0  0  0  0  0 : 0x00
 */
static uint8_t is_digits[10][DBG_DIGIT_H] = {
	{0x00, 0x1E, 0x12, 0x12, 0x12, 0x12, 0x12, 0x1E, 0x00}, /* 0 */
	{0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00}, /* 1 */
	{0x00, 0x1E, 0x02, 0x02, 0x1E, 0x10, 0x10, 0x1E, 0x00}, /* 2 */
	{0x00, 0x1E, 0x02, 0x02, 0x1E, 0x02, 0x02, 0x1E, 0x00}, /* 3 */
	{0x00, 0x12, 0x12, 0x12, 0x1E, 0x02, 0x02, 0x02, 0x00}, /* 4 */
	{0x00, 0x1E, 0x10, 0x10, 0x1E, 0x02, 0x02, 0x1E, 0x00}, /* 5 */
	{0x00, 0x1E, 0x10, 0x10, 0x1E, 0x12, 0x12, 0x1E, 0x00}, /* 6 */
	{0x00, 0x1E, 0x02, 0x02, 0x04, 0x08, 0x08, 0x08, 0x00}, /* 7 */
	{0x00, 0x1E, 0x12, 0x12, 0x1E, 0x12, 0x12, 0x1E, 0x00}, /* 8 */
	{0x00, 0x1E, 0x12, 0x12, 0x1E, 0x02, 0x02, 0x1E, 0x00}, /* 9 */
};

enum is_dot_align {
	IS_DOT_LEFT_TOP,
	IS_DOT_CENTER_TOP,
	IS_DOT_RIGHT_TOP,
	IS_DOT_LEFT_CENTER,
	IS_DOT_CENTER_CENTER,
	IS_DOT_RIGHT_CENTER,
	IS_DOT_LEFT_BOTTOM,
	IS_DOT_CENTER_BOTTOM,
	IS_DOT_RIGHT_BOTTOM,
	IS_DOT_ALIGN_MAX
};

enum is_dot_channel {
	IS_DOT_B,
	IS_DOT_W,
	IS_DOT_CH_MAX
};

/*
 * struct is_dot_info
 *
 * pixelperdot: num of pixel for a single dot
 * byteperdot: byte length of a single dot
 * scale: scaling factor for scarving digit on the image.
 *	This is applied for both direction & upscaling only.
 * align: digit alignment on the image coordination.
 * offset_y: offset for digit based on the align. -: up / +: down
 * move: vertical moving internal value. It overwrites offset_y to move.
 * pattern: byte pattern for a single dot.
 *	It has two channels, black & white.
 */
struct is_dot_info {
	uint8_t pixelperdot;
	uint8_t byteperdot;
	uint8_t scale;
	uint8_t align;
	int32_t offset_y;
	uint8_t move;
	uint8_t pattern[IS_DOT_CH_MAX][16];
};

enum is_dot_format {
	IS_DOT_8BIT,
	IS_DOT_10BIT,
	IS_DOT_12BIT,
	IS_DOT_13BIT_S,
	IS_DOT_16BIT,
	IS_DOT_YUV,
	IS_DOT_YUYV,
	IS_DOT_FORMAT_MAX
};

static struct is_dot_info is_dot_infos[IS_DOT_FORMAT_MAX] = {
	/* IS_DOT_8BIT */
	{
		.pixelperdot = 1,
		.byteperdot = 1,
		.scale = 16,
		.align = IS_DOT_LEFT_TOP,
		.offset_y = 0,
		.move = 0,
		.pattern = {
			{0x00},
			{0xFF},
		},
	},
	/* IS_DOT_10BIT */
	{
		.pixelperdot = 4,
		.byteperdot = 5,
		.scale = 2,
		.align = IS_DOT_CENTER_CENTER,
		.offset_y = -2,
		.move = 0,
		.pattern = {
			{0x00, 0x00, 0x00, 0x00, 0x00},
			{0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
		},
	},
	/* IS_DOT_12BIT */
	{
		.pixelperdot = 2,
		.byteperdot = 3,
		.scale = 4,
		.align = IS_DOT_CENTER_CENTER,
		.offset_y = -2,
		.move = 0,
		.pattern = {
			{0x00, 0x00, 0x00},
			{0xFF, 0xFF, 0xFF},
		},
	},
	/* IS_DOT_13BIT_S */
	{
		.pixelperdot = 8,
		.byteperdot = 13,
		.scale = 1,
		.align = IS_DOT_CENTER_CENTER,
		.offset_y = 0,
		.move = 0,
		.pattern = {
			{0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
			{0xFF, 0xEF, 0xFF, 0xFD, 0xBF, 0xFF,
			0xF7, 0xFF, 0xFE, 0xDF, 0xFF, 0xFB, 0x7F},
		},
	},
	/* IS_DOT_16BIT */
	{
	},
	/* IS_DOT_YUV */
	{
		.pixelperdot = 1,
		.byteperdot = 1,
		.scale = 8,
		.align = IS_DOT_CENTER_CENTER,
		.offset_y = 2,
		.move = 0,
		.pattern = {
			{0x00},
			{0xFF},
		},
	},
	/* IS_DOT_YUYV */
	{
		.pixelperdot = 2,
		.byteperdot = 2,
		.scale = 4,
		.align = IS_DOT_CENTER_CENTER,
		.offset_y = 2,
		.move = 0,
		.pattern = {
			{0x00, 0x00},
			{0xFF, 0xFF},
		},
	},
};

static int draw_digit_set(const char *val)
{
	int ret = 0, argc = 0;
	char **argv;
	u32 dot_fmt;
	u8 scale, align, move;
	int32_t offset_y;

	argv = argv_split(GFP_KERNEL, val, &argc);

	if (argc == 1) {
		ret = kstrtoul(argv[0], 0, &debug_param[IS_DEBUG_PARAM_DRAW_DIGIT].value);
	} else if (argc > 4) {
		ret = kstrtouint(argv[0], 0, &dot_fmt);
		if (ret || dot_fmt >= IS_DOT_FORMAT_MAX) {
			err("Invalid dot_fmt %u ret %d\n", dot_fmt, ret);
			goto func_exit;
		}

		ret = kstrtou8(argv[1], 0, &scale);
		if (ret) {
			err("Invalid scale %u ret %d\n", scale, ret);
			goto func_exit;
		}

		ret = kstrtou8(argv[2], 0, &align);
		if (ret || align >= IS_DOT_ALIGN_MAX) {
			err("Invalid align %u ret %d\n", align, ret);
			goto func_exit;
		}

		ret = kstrtoint(argv[3], 0, &offset_y);
		if (ret) {
			err("Invalid offset_y %d ret %d\n", offset_y, ret);
			goto func_exit;
		}

		ret = kstrtou8(argv[4], 0, &move);
		if (ret) {
			err("Invalid move %d ret %d\n", move, ret);
			goto func_exit;
		}

		/* Update is_dot_infos fields */
		is_dot_infos[dot_fmt].scale = scale;
		is_dot_infos[dot_fmt].align = align;
		is_dot_infos[dot_fmt].offset_y = offset_y;
		is_dot_infos[dot_fmt].move = move;
	}

func_exit:
	argv_free(argv);
	return ret;
}

static int draw_digit_usage(char *buffer, const size_t buf_size)
{
	const char *usage_msg = "[value] string, configure draw digit for debugging\n"
				"\techo <1,0> > draw_digit : on/off draw_digit\n"
				"\techo <dot_fmt> <scale> <align> <offset_y> <move> > draw_digit\n";

	return scnprintf(buffer, buf_size, usage_msg);
}

static int draw_digit_get(char *buffer, const size_t buf_size)
{
	int ret, i;

	ret = scnprintf(buffer, buf_size, "%7s: %8s %8s %8s %8s\n", "dot_fmt", "scale", "align",
		"offset_y", "move");

	for (i = 0; i < IS_DOT_FORMAT_MAX; i++)
		ret += scnprintf(buffer + ret, buf_size - ret, "%7d: %8d %8d %8d %8d\n", i,
			is_dot_infos[i].scale, is_dot_infos[i].align, is_dot_infos[i].offset_y,
			is_dot_infos[i].move);

	return ret;
}

/**
 * draw_digit: Draw digit on the DMA buffers
 *
 * echo 0/1 > draw_digit: turn off/on draw_digit
 * echo <dot_fmt> <scale> <align> <offset_y> <move> > draw_digit: Configure the specific dot_fmt
 */
static int is_get_carve_addr(ulong *addr, struct is_dot_info *d_info,
		int num_length, int bytesperline, int height)
{
	ulong carve_addr = *addr;
	u32 num_width, num_height, img_width, img_height;
	int offset_x, offset_y;

	/* dot domain */
	num_width = DBG_DIGIT_W * d_info->scale * num_length;
	num_height = DBG_DIGIT_H * d_info->scale * d_info->pixelperdot;
	img_width = bytesperline / d_info->byteperdot;
	img_height = height;

	switch (d_info->align) {
	case IS_DOT_CENTER_TOP:
		offset_x = (img_width >> 1) - (num_width >> 1);
		offset_y = 0;
		break;
	case IS_DOT_RIGHT_TOP:
		offset_x = img_width - num_width;
		offset_y = 0;
		break;
	case IS_DOT_LEFT_CENTER:
		offset_x = 0;
		offset_y = (img_height >> 1) - (num_height >> 1);
		break;
	case IS_DOT_CENTER_CENTER:
		offset_x = (img_width >> 1) - (num_width >> 1);
		offset_y = (img_height >> 1) - (num_height >> 1);
		break;
	case IS_DOT_RIGHT_CENTER:
		offset_x = img_width - num_width;
		offset_y = (img_height >> 1) - (num_height >> 1);
		break;
	case IS_DOT_LEFT_BOTTOM:
		offset_x = 0;
		offset_y = img_height - num_height - 1;
		break;
	case IS_DOT_CENTER_BOTTOM:
		offset_x = (img_width >> 1) - (num_width >> 1);
		offset_y = img_height - num_height - 1;
		break;
	case IS_DOT_RIGHT_BOTTOM:
		offset_x = img_width - num_width;
		offset_y = img_height - num_height - 1;
		break;
	case IS_DOT_LEFT_TOP:
	default:
		offset_x = 0;
		offset_y = 0;
		break;
	}

	offset_y += (d_info->offset_y * (int) num_height);

	if (offset_x < 0)
		offset_x = 0;
	if (offset_x + num_width > img_width)
		offset_x = img_width - num_width;
	if (offset_y < 0)
		offset_y = 0;
	if (offset_y + num_height > img_height)
		offset_y = img_height - num_height;

	/* Check boundary */
	if (offset_x < 0 || offset_y < 0)
		return -ERANGE;

	/* byte domain */
	carve_addr += (bytesperline * offset_y);
	carve_addr += (ulong)(offset_x * (int)d_info->byteperdot);

	*addr = carve_addr;

	return 0;
}

static void is_carve_digit(int digit, struct is_dot_info *d_info,
			int bytesperline, ulong addr)
{
	uint8_t *digit_buf;
	int w, h, digit_w, digit_h, pos_w, pos_h, dot_ch, byteperdot;

	digit_buf = is_digits[digit];
	byteperdot = d_info->byteperdot * d_info->scale;
	digit_w = DBG_DIGIT_W * d_info->scale;
	digit_h = DBG_DIGIT_H * d_info->scale * d_info->pixelperdot;

	/* per-line of digit pattern */
	for (h = 0; h < digit_h; h++) {
		pos_h = h / (d_info->scale * d_info->pixelperdot);
		for (w = digit_w - 1; w >= 0; w--) {
			pos_w = w / d_info->scale;
			dot_ch = (digit_buf[pos_h] & (1 << pos_w)) ?
				IS_DOT_W : IS_DOT_B;
			memcpy((void *)addr, d_info->pattern[dot_ch],
				d_info->byteperdot);
			addr += d_info->byteperdot;
		}
		addr += (bytesperline - (byteperdot * DBG_DIGIT_W));
	}
}

static void is_carve_num(u32 num, u32 dotformat, u32 bytesperline,
		u32 height, ulong addr)
{
	struct is_dot_info *d_info;
	u8 num_buf[DBG_DIGIT_CNT] = { 0 };
	int num_len;
	u32 digit_offset;

	d_info = &is_dot_infos[dotformat];
	digit_offset = d_info->byteperdot * d_info->scale * DBG_DIGIT_W;

	if (d_info->move)
		d_info->offset_y = (num % d_info->move);

	for (num_len = 0; num_len < DBG_DIGIT_CNT && num; num_len++) {
		num_buf[num_len] = num % 10;
		num /= 10;
	}

	if (!is_get_carve_addr(&addr, d_info, num_len, bytesperline, height)) {
		for (num_len--; num_len >= 0; num_len--, addr += digit_offset)
			is_carve_digit(num_buf[num_len], d_info, bytesperline, addr);
	} else if (debug_param[IS_DEBUG_PARAM_DRAW_DIGIT].value > 1) {
		warn("%s:Out of boundary! digit %d\n", __func__, num);
	}
}

void is_dbg_draw_digit(struct is_debug_dma_info *dinfo, u64 digit)
{
	u8 dotformat, bpp, b_align = 16;
	u32 bytesperline, pixelsize;

	if (!dinfo || !dinfo->addr) {
		err("Invalid parameters. dinfo(%p) addr(0x%lx)\n",
				(void *)dinfo, dinfo ? dinfo->addr : 0);
		return;
	}

	bpp = dinfo->bpp;
	pixelsize = dinfo->pixeltype & PIXEL_TYPE_SIZE_MASK;

	switch (dinfo->pixelformat) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_UYVY:
		dotformat = IS_DOT_YUYV;
		break;
	case V4L2_PIX_FMT_SBGGR10P:
		dotformat = IS_DOT_10BIT;
		b_align = 32;
		break;
	case V4L2_PIX_FMT_SBGGR12P:
		if (pixelsize == CAMERA_PIXEL_SIZE_13BIT) {
			dotformat = IS_DOT_13BIT_S;
			bpp = 13;
		} else {
			dotformat = IS_DOT_12BIT;
		}
		b_align = 32;
		break;
	case V4L2_PIX_FMT_NV16:
	case V4L2_PIX_FMT_NV61:
	case V4L2_PIX_FMT_NV16M:
	case V4L2_PIX_FMT_NV61M:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
	case V4L2_PIX_FMT_YUV422P:
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_YVU420:
	case V4L2_PIX_FMT_YUV420M:
	case V4L2_PIX_FMT_YVU420M:
		dotformat = IS_DOT_YUV;
		break;
	case V4L2_PIX_FMT_GREY:
		dotformat = IS_DOT_8BIT;
		break;
	default:
		if (debug_param[IS_DEBUG_PARAM_DRAW_DIGIT].value > 1)
			warn("%s:Not supported format(%c%c%c%c)\n",
				__func__,
				(char)((dinfo->pixelformat >> 0) & 0xFF),
				(char)((dinfo->pixelformat >> 8) & 0xFF),
				(char)((dinfo->pixelformat >> 16) & 0xFF),
				(char)((dinfo->pixelformat >> 24) & 0xFF));

		return;
	}

	bytesperline = ALIGN(dinfo->width * bpp / BITS_PER_BYTE, b_align);

	is_carve_num(digit, dotformat, bytesperline, dinfo->height, dinfo->addr);
}
EXPORT_SYMBOL_GPL(is_dbg_draw_digit);

long is_get_debug_param(int param_idx)
{
	if (param_idx >= IS_DEBUG_PARAM_MAX) {
		err("param_idx is invalid");
		return -EINVAL;
	}

	return debug_param[param_idx].value;
}
EXPORT_SYMBOL_GPL(is_get_debug_param);

int is_set_debug_param(int param_idx, long val)
{
	if (param_idx >= IS_DEBUG_PARAM_MAX) {
		err("param_idx is invalid");
		return -EINVAL;
	}
	debug_param[param_idx].value = val;
	return 0;
}
EXPORT_SYMBOL_GPL(is_set_debug_param);

struct is_sysfs_debug *is_get_sysfs_debug(void)
{
	return &sysfs_debug;
}
EXPORT_SYMBOL_GPL(is_get_sysfs_debug);

struct is_sysfs_sensor *is_get_sysfs_sensor(void)
{
	return &sysfs_sensor;
}
EXPORT_SYMBOL(is_get_sysfs_sensor);

struct is_sysfs_actuator *is_get_sysfs_actuator(void)
{
	return &sysfs_actuator;
}
EXPORT_SYMBOL_GPL(is_get_sysfs_actuator);

struct is_sysfs_eeprom *is_get_sysfs_eeprom(void)
{
	return &sysfs_eeprom;
}
EXPORT_SYMBOL_GPL(is_get_sysfs_eeprom);

void is_dmsg_init(void)
{
	is_debug.dsentence_pos = 0;
	memset(is_debug.dsentence, 0x0, DEBUG_SENTENCE_MAX);
}
EXPORT_SYMBOL_GPL(is_dmsg_init);

void is_dmsg_concate(const char *fmt, ...)
{
	va_list ap;
	char term[50];
	u32 copy_len;

	va_start(ap, fmt);
	vsnprintf(term, sizeof(term), fmt, ap);
	va_end(ap);

	copy_len = (u32)min((DEBUG_SENTENCE_MAX - is_debug.dsentence_pos), strlen(term));
	strncpy(is_debug.dsentence + is_debug.dsentence_pos, term, copy_len);
	is_debug.dsentence_pos += copy_len;
}
EXPORT_SYMBOL_GPL(is_dmsg_concate);

char *is_dmsg_print(void)
{
	return is_debug.dsentence;
}
EXPORT_SYMBOL_GPL(is_dmsg_print);

void is_print_buffer(char *buffer, size_t len)
{
	u32 sentence_i;
	size_t read_cnt;
	char sentence[250];
	char letter;

	FIMC_BUG_VOID(!buffer);

	sentence_i = 0;

	for (read_cnt = 0; read_cnt < len; read_cnt++) {
		letter = buffer[read_cnt];
		if (letter) {
			sentence[sentence_i] = letter;
			if (sentence_i >= 247) {
				sentence[sentence_i + 1] = '\n';
				sentence[sentence_i + 2] = 0;
				printk(KERN_DEBUG "%s", sentence);
				sentence_i = 0;
			} else if (letter == '\n') {
				sentence[sentence_i + 1] = 0;
				printk(KERN_DEBUG "%s", sentence);
				sentence_i = 0;
			} else {
				sentence_i++;
			}
		}
	}
}
EXPORT_SYMBOL_GPL(is_print_buffer);

#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
static int pablo_debug_memlog_alloc_printf(u32 idx, size_t size, const char *name)
{
	struct pablo_mobj_printf *mobj_printf;

	if (!is_debug.memlog.desc) {
		err("memlog for CAM is not registered");
		return -EINVAL;
	}

	if (idx >= PABLO_MEMLOG_MAX) {
		err("Invalid pablo mboj_printf index %d", idx);
		return -ERANGE;
	}

	mobj_printf = &is_debug.memlog.printf[idx];

	mobj_printf->size = size;
	strcpy(mobj_printf->name, name);
	mobj_printf->uflag = 0;

	mobj_printf->obj = CALL_PML_OPS(is_debug.memlog, alloc_printf,
			is_debug.memlog.desc, mobj_printf->size, NULL,
			mobj_printf->name, mobj_printf->uflag);

	info("[MEMLOG][I:%d] memlog_alloc_printf for %s(size:0x%lx)",
			idx, mobj_printf->name, mobj_printf->size);

	return 0;
}

int is_debug_memlog_alloc_dump(phys_addr_t paddr, u64 vaddr, size_t size, const char *name)
{
	int idx;
	char dump_name[10];
	struct memlog_obj *obj = NULL;

	if (!is_debug.memlog.desc) {
		err("memlog for CAM is not registered");
		return -EINVAL;
	}

	idx = atomic_read(&is_debug.memlog.dump_nums);
	snprintf(dump_name, sizeof(dump_name), "%d_%s", idx, name);

	if (paddr)
		obj = CALL_PML_OPS(is_debug.memlog, alloc_dump, is_debug.memlog.desc, size, paddr,
			true, NULL, dump_name);
	else if (vaddr)
		obj = CALL_PML_OPS(
			is_debug.memlog, alloc_direct, is_debug.memlog.desc, size, NULL, dump_name);

	if (!obj) {
		err("failed to alloc memlog dump for %s", name);
		return -EINVAL;
	}

	is_debug.memlog.dump[idx].obj = obj;
	is_debug.memlog.dump[idx].paddr = paddr;
	is_debug.memlog.dump[idx].vaddr = vaddr;
	is_debug.memlog.dump[idx].size = size;
	strncpy(is_debug.memlog.dump[idx].name, name, (PATH_MAX - 1));
	atomic_inc(&is_debug.memlog.dump_nums);

	info("[MEMLOG][I:%d] memlog_alloc_dump for %s(addr:%pad, size:0x%zx)",
		atomic_read(&is_debug.memlog.dump_nums), name, &paddr, size);

	return 0;
}
EXPORT_SYMBOL_GPL(is_debug_memlog_alloc_dump);

void is_debug_memlog_dump_cr_all(int log_level)
{
	int i;
	int nums = atomic_read(&is_debug.memlog.dump_nums);
	struct pablo_mobj_dump *dump;

	for (i = 0; i < nums; i++) {
		dump = &is_debug.memlog.dump[i];
		if (!dump->obj)
			continue;

		if (dump->paddr) {
			CALL_PML_OPS(is_debug.memlog, do_dump, dump->obj, log_level);
		} else if (dump->vaddr) {
			void *src, *dst;

			src = (void *)*((u64 *)dump->vaddr);
			dst = dump->obj->vaddr;

			memcpy(dst, src, dump->size);
			CALL_PML_OPS(is_debug.memlog, dump_direct, dump->obj, log_level);
		} else {
			continue;
		}

		info("[MEMLOG][Lv:%d][I:%d] %s CR DUMP(V/P/S):(0x%llx/%pad/0x%zx)", log_level, i,
			dump->name, (u64)dump->obj->vaddr, &dump->paddr, dump->size);
	}
}
EXPORT_SYMBOL_GPL(is_debug_memlog_dump_cr_all);
#endif

static const struct pablo_memlog_operations pml_ops = {
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
	.alloc_printf = memlog_alloc_printf,
	.alloc_dump = memlog_alloc_dump,
	.alloc_direct = memlog_alloc_direct,
	.do_dump = memlog_do_dump,
	.dump_direct = memlog_dump_direct_to_file,
#endif
};

static const struct pablo_bcm_operations bcm_ops = {
#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG)
	.start = exynos_bcm_dbg_start,
	.stop = exynos_bcm_dbg_stop,
#endif
};

static const struct pablo_dss_operations dss_ops = {
#if IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
	.expire_watchdog = dbg_snapshot_expire_watchdog,
#endif
};

void is_debug_probe(struct device *dev)
{
#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
	struct pablo_lib_platform_data *plpd = pablo_lib_get_platform_data();
#endif
#ifdef ENABLE_DBG_FS
	umode_t mode;
#endif

	pablo_blob_debugfs_create_dir("pablo-blob-lvn", &is_debug.blob_lvn_root);
	pablo_blob_lvn_probe(is_debug.blob_lvn_root, &is_debug.blob_lvn, blob_lvn_name,
			     ARRAY_SIZE(blob_lvn_name));

	pablo_blob_debugfs_create_dir("pablo-blob-pd", &is_debug.blob_pd_root);
	pablo_blob_pd_probe(is_debug.blob_pd_root, &is_debug.blob_pd);

	is_debug.read_vptr = 0;
	is_debug.minfo = NULL;

	is_debug.dsentence_pos = 0;
	memset(is_debug.dsentence, 0x0, DEBUG_SENTENCE_MAX);


#ifdef ENABLE_DBG_FS
	is_debug.root = debugfs_create_dir(DEBUG_FS_ROOT_NAME, NULL);
	if (is_debug.root)
		probe_info("%s is created\n", DEBUG_FS_ROOT_NAME);

#ifdef ENABLE_CONTINUOUS_DDK_LOG
	mode = 0444;
#else
	mode = S_IRUSR;
#endif

	is_debug.dbg_log_fops = &dbg_log_fops;
	is_debug.logfile = debugfs_create_file(DEBUG_FS_LOGFILE_NAME, mode,
			is_debug.root, &is_debug, is_debug.dbg_log_fops);
	if (is_debug.logfile)
		probe_info("%s is created\n", DEBUG_FS_LOGFILE_NAME);

	mode = 0644;
	is_debug.dbg_event_fops = &dbg_event_fops;
	is_debug.event_dir = debugfs_create_dir(DEBUG_FS_EVENT_DIR_NAME, is_debug.root);

	debugfs_create_u32(DEBUG_FS_EVENT_FILTER, mode, is_debug.event_dir, &is_debug_event.log_filter);
	debugfs_create_u32(DEBUG_FS_EVENT_LOGEN, mode, is_debug.event_dir, &is_debug_event.log_enable);
	is_debug_event.log = debugfs_create_file(DEBUG_FS_EVENT_LOGFILE, mode,
			is_debug.event_dir, &is_debug_event, is_debug.dbg_event_fops);

	is_debug_event.log_enable = 1;
	is_debug_event.log_filter = (0x1 << IS_EVENT_ALL) - 1;

#ifdef ENABLE_CONTINUOUS_DDK_LOG
	is_debug.read_debug_fs_logfile = false;
	init_waitqueue_head(&is_debug.debug_fs_logfile_queue);
#endif

#ifdef ENABLE_DBG_EVENT_PRINT
	atomic_set(&is_debug_event.event_index, -1);
	atomic_set(&is_debug_event.critical_log_tail, -1);
	atomic_set(&is_debug_event.normal_log_tail, -1);
#endif
#endif

#if IS_ENABLED(CONFIG_EXYNOS_MEMORY_LOGGER)
	is_debug.memlog.ops = &pml_ops;
	atomic_set(&is_debug.memlog.dump_nums, 0);
	if (memlog_register("IS", dev, &is_debug.memlog.desc))
		probe_err("failed to memlog_register for CAM");

	pablo_debug_memlog_alloc_printf(PABLO_MEMLOG_DRV, plpd->memlog_size[PABLO_MEMLOG_DRV], "ker-mem");
	pablo_debug_memlog_alloc_printf(PABLO_MEMLOG_DDK, plpd->memlog_size[PABLO_MEMLOG_DDK], "bin-mem");
#endif

	is_debug.bcm_ops = &bcm_ops;
	is_debug.dss_ops = &dss_ops;

	is_debug.num_dump_func = 0;

	spin_lock_init(&is_debug.slock);

	clear_bit(IS_DEBUG_OPEN, &is_debug.state);
}
EXPORT_SYMBOL_GPL(is_debug_probe);

int is_debug_open(struct is_minfo *minfo)
{
	/*
	 * debug control should be reset on camera entrance
	 * because firmware doesn't update this area after reset
	 */
	if (minfo->kvaddr_debug_cnt)
		*((int *)(minfo->kvaddr_debug_cnt)) = 0;
	is_debug.read_vptr = 0;
	is_debug.minfo = minfo;

	set_bit(IS_DEBUG_OPEN, &is_debug.state);

	return 0;
}
EXPORT_SYMBOL(is_debug_open);

struct is_debug *is_debug_get(void)
{
	return &is_debug;
}
EXPORT_SYMBOL(is_debug_get);

int is_debug_close(void)
{
	clear_bit(IS_DEBUG_OPEN, &is_debug.state);

	return 0;
}
EXPORT_SYMBOL(is_debug_close);

#ifdef USE_KERNEL_VFS_READ_WRITE
/**
  * is_debug_dma_dump: dump buffer by is_queue.
  *                         should be enable DBG_IMAGE_KMAPPING for kernel addr
  * @queue: buffer info
  * @index: buffer index
  * @vid: video node id for filename
  * @type: enum dbg_dma_dump_type
  **/
int is_dbg_dma_dump(void *q, u32 instance, u32 index, u32 vid, u32 type)
{
	int i;
	int ret;
	u32 flags;
	char *filename;
	struct is_binary bin;
	struct is_queue *queue = (struct is_queue *)q;
	struct is_frame *frame = &queue->framemgr.frames[index];
	u32 region_id = 0;
	int total_size;
	struct vb2_buffer *buf = queue->vbq->bufs[index];
	u32 framecount = frame->fcount;

	switch (type) {
	case DBG_DMA_DUMP_IMAGE:
		/* Dump each region */
		do {
			filename = __getname();
			if (unlikely(!filename))
				return -ENOMEM;

			snprintf(filename, PATH_MAX, "%s/V%02d_F%08d_I%02d_R%d.raw",
					DBG_DMA_DUMP_PATH, vid, framecount, instance, region_id);

			/* Dump each plane */
			for (i = 0; i < (buf->num_planes - 1); i++) {
				if (frame->stripe_info.region_num) {
					bin.data = (void *)frame->stripe_info.kva[region_id][i];
					bin.size = frame->stripe_info.size[region_id][i];
				} else {
					bin.data = (void *)queue->buf_kva[index][i];
					bin.size = queue->framecfg.size[i];
				}

				if (!bin.data) {
					err("[V%d][F%d][I%d][R%d] kva is NULL\n",
							vid, frame->fcount, index, region_id);
					__putname(filename);
					return -EINVAL;
				}

				if (i == 0) {
					/* first plane for image */
					flags = O_TRUNC | O_CREAT | O_EXCL | O_WRONLY | O_APPEND;
					total_size = bin.size;
				} else {
					/* after first plane for image */
					flags = O_WRONLY | O_APPEND;
					total_size += bin.size;
				}

				ret = put_filesystem_binary(filename, &bin, flags);
				if (ret) {
					err("failed to dump %s (%d)", filename, ret);
					__putname(filename);
					return -EINVAL;
				}
			}

			info("[V%d][F%d][I%d][R%d] img dumped..(%s, %d)\n",
					vid, frame->fcount, index, region_id, filename, total_size);

			__putname(filename);
		} while (++region_id < frame->stripe_info.region_num);

		break;
	case DBG_DMA_DUMP_META:
		filename = __getname();
		if (unlikely(!filename))
			return -ENOMEM;

		snprintf(filename, PATH_MAX, "%s/V%02d_F%08d_I%02d.meta",
				DBG_DMA_DUMP_PATH, vid, frame->fcount, index);

		bin.data = (void *)queue->buf_kva[index][buf->num_planes - 1];
		bin.size = queue->framecfg.size[buf->num_planes - 1];

		/* last plane for meta */
		flags = O_TRUNC | O_CREAT | O_EXCL | O_WRONLY;
		total_size = bin.size;

		ret = put_filesystem_binary(filename, &bin, flags);
		if (ret) {
			err("failed to dump %s (%d)", filename, ret);
			__putname(filename);
			return -EINVAL;
		}

		info("[V%d][F%d] meta dumped..(%s, %d)\n", vid, frame->fcount, filename, total_size);
		__putname(filename);

		break;
	default:
		err("invalid type(%d)", type);
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(is_dbg_dma_dump);

int is_dbg_dma_dump_by_frame(struct is_frame *frame, u32 vid, u32 type)
{
	int i = 0;
	int ret = 0;
	u32 flags = 0;
	int total_size = 0;
	u32 framecount = frame->fcount;
	char *filename;
	struct is_binary bin;

	switch (type) {
	case DBG_DMA_DUMP_IMAGE:
		filename = __getname();

		if (unlikely(!filename))
			return -ENOMEM;

		snprintf(filename, PATH_MAX, "%s/V%02d_F%08d.raw",
				DBG_DMA_DUMP_PATH, vid, framecount);

		for (i = 0; i < (frame->planes / frame->num_buffers); i++) {
			bin.data = (void *)frame->kvaddr_buffer[i];
			bin.size = frame->size[i];

			if (!i) {
				/* first plane for image */
				flags = O_TRUNC | O_CREAT | O_EXCL | O_WRONLY | O_APPEND;
				total_size += bin.size;
			} else {
				/* after first plane for image */
				flags = O_WRONLY | O_APPEND;
				total_size += bin.size;
			}

			ret = put_filesystem_binary(filename, &bin, flags);
			if (ret) {
				err("failed to dump %s (%d)", filename, ret);
				__putname(filename);
				return -EINVAL;
			}
		}

		info("[V%d][F%d] img dumped..(%s, %d)\n", vid, framecount, filename, total_size);
		__putname(filename);

		break;
	default:
		err("invalid type(%d)", type);
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(is_dbg_dma_dump_by_frame);
#endif

static int islib_debug_open(struct inode *inode, struct file *file)
{
	if (inode->i_private)
		file->private_data = inode->i_private;

	return 0;
}

static ssize_t islib_debug_read(struct file *file, char __user *user_buf,
	size_t buf_len, loff_t *ppos)
{
	int ret = 0;
	void *read_ptr;
	size_t write_vptr, read_vptr, buf_vptr;
	size_t read_cnt, read_cnt1, read_cnt2;
	struct is_minfo *minfo;

#ifdef ENABLE_CONTINUOUS_DDK_LOG
debug_fs_logfile_restart:
	if (!test_bit(IS_DEBUG_OPEN, &is_debug.state))
		wait_event_interruptible(is_debug.debug_fs_logfile_queue, is_debug.state);
	if (!is_debug.read_debug_fs_logfile)
		wait_event_interruptible(is_debug.debug_fs_logfile_queue, is_debug.read_debug_fs_logfile);
#else
	if (!test_bit(IS_DEBUG_OPEN, &is_debug.state))
		return 0;
#endif

	minfo = is_debug.minfo;

	write_vptr = *((int *)(minfo->kvaddr_debug_cnt));
	read_vptr = is_debug.read_vptr;
	buf_vptr = buf_len;

	if (write_vptr >= read_vptr) {
		/**
		 *      kvaddr_debug-> ================
		 *                     ----------------
		 *                     ----------------
		 *                     ----------------
		 *         read_vptr-> ----------------
		 *                     ---------------- \
		 *                     ---------------- | read_cnt1
		 *                     ---------------- /
		 *        write_vptr-> ----------------
		 *                     ----------------
		 *                     ----------------
		 *                     ----------------
		 * DEBUG_REGION_SIZE-> ================
		 */
		read_cnt1 = write_vptr - read_vptr;
		read_cnt2 = 0;
	} else {
		/**
		 *      kvaddr_debug-> ================
		 *                     ---------------- \
		 *                     ---------------- | read_cnt2
		 *                     ---------------- /
		 *        write_vptr-> ----------------
		 *                     ----------------
		 *                     ----------------
		 *                     ----------------
		 *         read_vptr-> ----------------
		 *                     ---------------- \
		 *                     ---------------- | read_cnt1
		 *                     ---------------- /
		 * DEBUG_REGION_SIZE-> ================
		 */
		read_cnt1 = DEBUG_REGION_SIZE - read_vptr;
		read_cnt2 = write_vptr;
	}

	if (buf_vptr && read_cnt1) {
		read_ptr = (void *)(minfo->kvaddr_debug + is_debug.read_vptr);

		if (read_cnt1 > buf_vptr)
			read_cnt1 = buf_vptr;

		ret = copy_to_user(user_buf, read_ptr, read_cnt1);
		if (ret) {
			err("[DBG] failed copying %d bytes of debug log\n", ret);
			return ret;
		}
		is_debug.read_vptr += read_cnt1;
		buf_vptr -= read_cnt1;
	}

	if (is_debug.read_vptr >= DEBUG_REGION_SIZE) {
		if (is_debug.read_vptr > DEBUG_REGION_SIZE)
			err("[DBG] read_vptr(%zd) is invalid", is_debug.read_vptr);
		is_debug.read_vptr = 0;
	}

	if (buf_vptr && read_cnt2) {
		read_ptr = (void *)(minfo->kvaddr_debug + is_debug.read_vptr);

		if (read_cnt2 > buf_vptr)
			read_cnt2 = buf_vptr;

		ret = copy_to_user(user_buf, read_ptr, read_cnt2);
		if (ret) {
			err("[DBG] failed copying %d bytes of debug log\n", ret);
			return ret;
		}
		is_debug.read_vptr += read_cnt2;
		buf_vptr -= read_cnt2;
	}

	read_cnt = buf_len - buf_vptr;

	/* info("[DBG] FW_READ : read_vptr(%zd), write_vptr(%zd) - dump(%zd)\n", read_vptr, write_vptr, read_cnt); */

#ifdef ENABLE_CONTINUOUS_DDK_LOG
	if (read_cnt == 0) {
		is_debug.read_debug_fs_logfile = false;
		goto debug_fs_logfile_restart;
	}
#endif

	return read_cnt;
}

static const struct file_operations dbg_log_fops = {
	.open	= islib_debug_open,
	.read	= islib_debug_read,
	.llseek	= default_llseek
};

static struct seq_file *seq_f;

static int is_event_show(struct seq_file *s, void *unused)
{
	struct is_debug_event *event_log = s->private;

	is_debug_info_dump(s, event_log);

	return 0;
}

static int is_debug_event_open(struct inode *inode, struct file *file)
{
	return single_open(file, is_event_show, inode->i_private);
}

#ifdef ENABLE_DBG_EVENT_PRINT
void is_debug_event_print(is_event_store_type_t event_store_type,
	void (*callfunc)(void *),
	void *ptrdata,
	size_t datasize,
	const char *fmt, ...)
{
	int index;
	struct is_debug_event_log *event_log;
	unsigned int log_num;
	va_list args;

	if (is_debug_event.log_enable == 0)
		return;

	switch (event_store_type) {
	case IS_EVENT_CRITICAL:
		index = atomic_inc_return(&is_debug_event.critical_log_tail);
		if (index >= IS_EVENT_MAX_NUM - 1) {
			if (index % LOG_INTERVAL_OF_WARN == 0)
				warn("critical event log buffer full...!");

			return;
		}
		event_log = &is_debug_event.event_log_critical[index];
		break;
	case IS_EVENT_NORMAL:
		index = atomic_inc_return(&is_debug_event.normal_log_tail) &
				(IS_EVENT_MAX_NUM - 1);

		event_log = &is_debug_event.event_log_normal[index];
		break;
	default:
		warn("invalid event type(%d)", event_store_type);
		goto p_err;
	}

	log_num = atomic_inc_return(&is_debug_event.event_index);

	event_log->time = ktime_get();
	va_start(args, fmt);
	vsnprintf(event_log->dbg_msg, sizeof(event_log->dbg_msg), fmt, args);
	va_end(args);
	event_log->log_num = log_num;
	event_log->event_store_type = event_store_type;
	event_log->callfunc = callfunc;
	event_log->cpu = raw_smp_processor_id();

	/* ptrdata should be used in non-atomic context */
	if (!in_atomic()) {
		if (event_log->ptrdata) {
			vfree(event_log->ptrdata);
			event_log->ptrdata = NULL;
		}

		if (datasize) {
			event_log->ptrdata = vmalloc(datasize);
			if (event_log->ptrdata)
				memcpy(event_log->ptrdata, ptrdata, datasize);
			else
				warn("couldn't allocate ptrdata");
		}
	}

p_err:
	return;
}
EXPORT_SYMBOL_GPL(is_debug_event_print);
#endif

/**
 * Compare two values whether they are near or far.
 * It removes the bit 0 to ignore the fcount mismatching
 * between sensor domain & ischain domain.
 */
#define _IS_NEAR(a, b) ((a == b) || ((a + 1) == b))
#define IS_NEAR(a, b) ((a > b) ? _IS_NEAR(b, a) : _IS_NEAR(a, b))

static void _is_debug_handle_overflow_dma(u32 instance, u32 fcount)
{
	struct is_debug_event_cnt *link_err;

	if (debug_param[IS_DEBUG_PARAM_PHY_TUNE].value)
		return;

	/**
	 * Since there was link error for the near frame,
	 * ignore the DMA overflow.
	 */
	link_err = &is_debug_event.event_cnt[IS_EVENT_CSI_LINK_ERR];
	if (link_err->instance == instance && IS_NEAR(link_err->fcount, fcount))
		return;

	is_debug_s2d(true, "DMA Overflow");
}

static void _is_debug_dec_overflow_ibuf(void)
{
	struct is_debug_event_cnt *ovf_ibuf = &is_debug_event.event_cnt[IS_EVENT_OVERFLOW_IBUF];

	atomic_dec(&ovf_ibuf->cnt);
}

void is_debug_event_count(is_event_store_type_t event_type, u32 instance, u32 fcount)
{
	struct is_debug_event_cnt *event_cnt = &is_debug_event.event_cnt[event_type];

	switch (event_type) {
	case IS_EVENT_OVERFLOW_CSI:
		if (IS_ENABLED(CONFIG_PANIC_ON_CSIS_OVERFLOW))
			is_debug_s2d(true, "CSI Overflow");
		break;
	case IS_EVENT_OVERFLOW_DMA:
		if (IS_ENABLED(CONFIG_PANIC_ON_CSIS_OVERFLOW))
			_is_debug_handle_overflow_dma(instance, fcount);
		break;
	case IS_EVENT_OVERFLOW_IBUF:
		if (atomic_read(&event_cnt->cnt) < 0) {
			atomic_set(&event_cnt->cnt, 0);
		} else if (atomic_read(&event_cnt->cnt) > 0) {
// solomon_bringup
// #ifdef CONFIG_CAMERA_VENDOR_MCD
//		if (is_debug.secdbg_ops->secdbg_mode_enter_upload())
//			is_debug_s2d(true, "IBUF Overflow");
//		else
//			err("[DBG] IBUF Overflow");
// #else
			is_debug_s2d(true, "IBUF Overflow");
// #endif
		}
		break;
	case IS_EVENT_COTF_ERR:
		if (IS_ENABLED(CONFIG_PANIC_ON_COTF_ERR))
			is_debug_s2d(true, "COTF Error");
		fallthrough;
	case IS_EVENT_CSI_LINK_ERR:
		/**
		 * When there were link error or COTF error,
		 * it could make IBUF overflow
		 * due to corrupted link protocol or stall from COTF.
		 */
		_is_debug_dec_overflow_ibuf();
		break;
	default:
		do_nothing;
	}

	atomic_inc(&event_cnt->cnt);
	event_cnt->instance = instance;
	event_cnt->fcount = fcount;
}
EXPORT_SYMBOL_GPL(is_debug_event_count);

struct is_debug_event *pablo_debug_get_event(void)
{
	return &is_debug_event;
}
EXPORT_SYMBOL_GPL(pablo_debug_get_event);

int is_debug_info_dump(struct seq_file *s, struct is_debug_event *debug_event)
{
	seq_f = s;
	seq_printf(s, "------------------- FIMC-IS EVENT LOGGER - START --------------\n");

#ifdef ENABLE_DBG_EVENT_PRINT
	{
		int index_normal = 0;
		int latest_normal = atomic_read(&is_debug_event.normal_log_tail);
		int index_critical = 0;
		int latest_critical = atomic_read(&is_debug_event.critical_log_tail);
		bool normal_done = 0;
		bool critical_done = 0;
		struct timespec64 tv;
		struct is_debug_event_log *log_critical;
		struct is_debug_event_log *log_normal;
		struct is_debug_event_log *log_print;

		if ((latest_normal < 0) || !(is_debug_event.log_filter & IS_EVENT_NORMAL))
			normal_done = 1; /* normal log empty */
		else if (latest_normal > IS_EVENT_MAX_NUM - 1)
			index_normal = (latest_normal % IS_EVENT_MAX_NUM) + 1;
		else
			index_normal = 0;

		latest_normal = latest_normal % IS_EVENT_MAX_NUM;

		if ((latest_critical < 0) || !(is_debug_event.log_filter & IS_EVENT_CRITICAL))
			critical_done = 1; /* critical log empty */

		while (!(normal_done) || !(critical_done)) {
			index_normal = index_normal % IS_EVENT_MAX_NUM;
			index_critical = index_critical % IS_EVENT_MAX_NUM;
			log_normal = &is_debug_event.event_log_normal[index_normal];
			log_critical = &is_debug_event.event_log_critical[index_critical];

			if (!normal_done && !critical_done) {
				if (log_normal->log_num < log_critical->log_num) {
					log_print = log_normal;
					index_normal++;
				} else {
					log_print = log_critical;
					index_critical++;
				}
			} else if (!normal_done) {
				log_print = log_normal;
				index_normal++;
			} else if (!critical_done) {
				log_print = log_critical;
				index_critical++;
			}

			if (latest_normal == index_normal)
				normal_done = 1;

			if (latest_critical == index_critical)
				critical_done = 1;

			tv = ktime_to_timespec64(log_print->time);
			seq_printf(s, "[%d][%6lld.%06ld] num(%d) ", log_print->cpu,
				tv.tv_sec, tv.tv_nsec / 1000, log_print->log_num);
			seq_printf(s, "%s\n", log_print->dbg_msg);

			if (log_print->callfunc != NULL)
				log_print->callfunc(log_print->ptrdata);

		}
	}
#endif

	seq_printf(s, "overflow: csi(%d), 3aa(%d)\n",
		atomic_read(&debug_event->event_cnt[IS_EVENT_OVERFLOW_CSI].cnt),
		atomic_read(&debug_event->event_cnt[IS_EVENT_OVERFLOW_3AA].cnt));

	seq_printf(s, "------------------- FIMC-IS EVENT LOGGER - END ----------------\n");
	return 0;
}

static const struct file_operations dbg_event_fops = {
	.open = is_debug_event_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

#ifdef CONFIG_CAMERA_VENDOR_MCD
void is_debug_register_secdbg_ops(struct pablo_secdbg_ops *ops)
{
	is_debug.secdbg_ops = ops;
}
EXPORT_SYMBOL_GPL(is_debug_register_secdbg_ops);
#endif

void is_debug_register_icpu_ops(struct pablo_icpu_operations *ops)
{
	if (!ops || !ops->s2d_handler)
		return;

	is_debug.icpu_ops = ops;
}
EXPORT_SYMBOL_GPL(is_debug_register_icpu_ops);

void is_debug_icpu_s2d_handler(void)
{
	if (is_debug.icpu_ops)
		is_debug.icpu_ops->s2d_handler();
}
EXPORT_SYMBOL_GPL(is_debug_icpu_s2d_handler);

void *is_debug_get_icpu_fw_loginfo(u32 *size)
{
	void *addr = NULL;

	if (is_debug.icpu_ops)
		addr = is_debug.icpu_ops->get_fw_loginfo(size);

	return addr;
}
EXPORT_SYMBOL_GPL(is_debug_get_icpu_fw_loginfo);

void is_debug_register_dump_func(dump_func_t func)
{
	if (is_debug.num_dump_func >= PABLO_DBG_DUMP_FUNC_NUM)
		return;

	is_debug.dump_func[is_debug.num_dump_func++] = func;
}
EXPORT_SYMBOL_GPL(is_debug_register_dump_func);

void is_debug_s2d(bool en_s2d, const char *fmt, ...)
{
	static char buf[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (is_get_debug_param(IS_DEBUG_PARAM_PHY_TUNE))
		return;

	err("[DBG] DUMP!!!: %s", buf);

	is_debug_bcm(false, PANIC_HANDLE);

	if (en_s2d || is_get_debug_param(IS_DEBUG_PARAM_S2D)) {
		u32 i;

		is_debug_icpu_s2d_handler();
		for (i = 0; i < is_debug.num_dump_func; i++)
			is_debug.dump_func[i]();

		dump_stack();

#if IS_ENABLED(CONFIG_DEBUG_SNAPSHOT)
		err("[DBG] S2D!!!: %s", buf);

		if (CALL_PDSS_OPS(is_debug, expire_watchdog) < 0)
			panic("DSS doesn't support S2D");
#endif
	} else {
		panic(buf);
	}
}
EXPORT_SYMBOL(is_debug_s2d);

void is_debug_bcm(bool en, unsigned int bcm_owner)
{
	if (!IS_ENABLED(ENABLE_CAMERA_BCM))
		return;

#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG)
	if (en)
		CALL_PBCM_OPS(is_debug, start);
	else
		CALL_PBCM_OPS(is_debug, stop, bcm_owner);
#endif
}
EXPORT_SYMBOL_GPL(is_debug_bcm);

void is_debug_iommu_perf(bool en, struct device *dev)
{
#if IS_ENABLED(CONFIG_IOMMU_PERF_MEASURE)
	if (en)
		samsung_iommu_set_perf_measure(dev);
	else
		samsung_iommu_get_perf_measure(dev);
#endif
}
EXPORT_SYMBOL(is_debug_iommu_perf);

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
const struct kernel_param *pablo_debug_get_kernel_param(u32 param_id)
{
	switch (param_id) {
	case IS_DEBUG_PARAM_DRAW_DIGIT:
		return G_KERNEL_PARAM(draw_digit);
	case IS_DEBUG_PARAM_CLK:
		return G_KERNEL_PARAM(debug_clk);
	default:
		warn("Invalid param_id %u\n", param_id);
		return NULL;
	}

	return NULL;
}
KUNIT_EXPORT_SYMBOL(pablo_debug_get_kernel_param);
#endif

static struct is_fault_handler_cb fault_handler_cb;

#if defined(CONFIG_EXYNOS_IOVMM)
static int __attribute__((unused)) is_fault_handler(struct iommu_domain *domain,
	struct device *dev,
	unsigned long fault_addr,
	int fault_flag,
	void *token)
{
	pr_err("[@] <Pablo IS FAULT HANDLER> ++\n");
	pr_err("[@] Device virtual(0x%08X) is invalid access\n", (u32)fault_addr);

	if (fault_handler_cb.func)
		fault_handler_cb.func(dev);

	pr_err("[@] <Pablo IS FAULT HANDLER> --\n");

	return -EINVAL;
}
#else
static int __attribute__((unused)) is_fault_handler(struct iommu_fault *fault,
	void *data)
{
	pr_err("[@] <Pablo IS FAULT HANDLER> ++\n");

	if (fault_handler_cb.func)
		fault_handler_cb.func(fault_handler_cb.data);

	pr_err("[@] <Pablo IS FAULT HANDLER> --\n");

	return -EINVAL;
}
#endif

void is_register_iommu_fault_handler_cb(void (*func)(struct device *dev), struct device *data)
{
	fault_handler_cb.func = func;
	fault_handler_cb.data = data;
}
EXPORT_SYMBOL_GPL(is_register_iommu_fault_handler_cb);

void is_register_iommu_fault_handler(struct device *dev)
{
#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_set_fault_handler(dev, is_fault_handler, NULL);
#else
	iommu_register_device_fault_handler(dev, is_fault_handler, NULL);
#endif
}
EXPORT_SYMBOL_GPL(is_register_iommu_fault_handler);

bool is_debug_support_crta(void)
{
	if (!IS_ENABLED(CRTA_CALL))
		return false;

	if (is_get_debug_param(IS_DEBUG_PARAM_DISABLE_CRTA))
		return false;
	else
		return true;
}
EXPORT_SYMBOL_GPL(is_debug_support_crta);

void is_debug_lock(void)
{
	spin_lock(&is_debug.slock);
}
EXPORT_SYMBOL_GPL(is_debug_lock);

void is_debug_unlock(void)
{
	spin_unlock(&is_debug.slock);
}
EXPORT_SYMBOL_GPL(is_debug_unlock);
