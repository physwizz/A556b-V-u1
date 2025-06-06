/*
* Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is vendor functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_TYPE_H
#define IS_TYPE_H

#include <linux/media-bus-format.h>
#include <media/v4l2-device.h>
#include "is-common-enum.h"

enum is_device_type {
	IS_DEVICE_SENSOR,
	IS_DEVICE_ISCHAIN,
	IS_DEVICE_MAX
};

struct is_window {
	u32 o_width;
	u32 o_height;
	u32 width;
	u32 height;
	u32 offs_h;
	u32 offs_v;
	u32 otf_width;
	u32 otf_height;
};

struct is_frame_cfg {
	struct is_fmt		*format;
	enum v4l2_colorspace	colorspace;
	enum v4l2_quantization	quantization;
	ulong			flip;
	u32			width;
	u32			height;
	u32			hw_pixeltype;
	u32			size[IS_MAX_PLANES];
	u32			bytesperline[IS_MAX_PLANES];
};

struct is_fmt {
	char	*name;
	u32	mbus_code;
	u32	pixelformat;
	u32	field;
	u32	num_planes;
	u32	hw_format;
	u32	hw_order;
	u32	hw_bitwidth;
	u32	hw_plane;
	u32	sbwc_type;
	u32	sbwc_align;
	u32	sbwc_lossy_fr;
	u8	bitsperpixel[VIDEO_MAX_PLANES];
	void	(*setup_plane_sz)(struct is_frame_cfg *fc, unsigned int sizes[]);
};

struct is_image {
	u32			framerate;
	struct is_window	window;
	struct is_fmt	format;
};

struct is_crop {
	u32			x;
	u32			y;
	u32			w;
	u32			h;
};

struct is_rectangle {
	u32 w;
	u32 h;
};

struct is_multi_layer {
	u32 l0;
	u32 l1;
	u32 l2;
	u32 l3;
	u32 l4;
};

enum cr_state {
	CR_SET_EMPTY,
	CR_SET_CONFIG,
	CR_SET_EMPTY_EXT1,
	CR_SET_CONFIG_EXT1,
	CR_SET_END
};

struct cr_set {
	u32 reg_addr;
	u32 reg_data;
};

#define MAX_CR_SET 7000
struct size_cr_set {
	u32 size;
	struct cr_set cr[MAX_CR_SET];
};

#define INIT_CROP(c) ((c)->x = 0, (c)->y = 0, (c)->w = 0, (c)->h = 0)
#define IS_NULL_CROP(c) (!((c)->x + (c)->y + (c)->w + (c)->h) \
		|| ((int)(c)->x < 0 || (int)(c)->y < 0 || (int)(c)->w < 0 || (int)(c)->h < 0))
#define COMPARE_CROP(c1, c2) (((c1)->x == (c2)->x) && ((c1)->y == (c2)->y) && ((c1->w) == (c2)->w) && ((c1)->h == (c2)->h))
#define TRANS_CROP(d, s) ((d)[0] = (s)[0], (d)[1] = (s)[1], (d)[2] = (s)[2], (d)[3] = (s)[3])
#define COMPARE_FORMAT(f1, f2) ((f1) == (f2))
#define CHECK_STRIPE_CFG(stripe_info)	((stripe_info)->region_num > 0)


#define TO_WORD_OFFSET(byte_offset) ((byte_offset) >> 2)

#define CONVRES(src, src_max, tar_max) \
	((src <= 0) ? (0) : ((src * tar_max + (src_max >> 1)) / src_max))

#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))

#define ZERO_IF_NEG(val) ((val) > 0 ? (val) : 0)

#define BOOL(x) (!!(x))

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __get_glpg_size(_l0_size, _lev)			\
	({						\
		u32 i = 0;				\
		u32 _size = _l0_size;				\
		for (i = 0; i < _lev; ++i)		\
			_size = ALIGN(_size / 2, 2);		\
		_size;					\
	})
#endif
