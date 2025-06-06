/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Samsung Exynos SoC series dsp driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 */

#ifndef __DL_DSP_XML_PARSER_H__
#define __DL_DSP_XML_PARSER_H__

#include "dsp-sxml.h"
#include "dsp-common.h"
#include "dsp-hash.h"

enum dsp_xml_token {
	LIBS,
	COUNT,
	LIB,
	NAME,
	KERNEL_COUNT,
	KERNEL,
	ID,
	PRE,
	EXE,
	POST,
	TOKEN_NUM,
};

struct dsp_xml_kernel_table {
	char *pre;
	char *exe;
	char *post;
};

struct dsp_xml_lib {
	char *name;
	unsigned int kernel_cnt;
	struct dsp_xml_kernel_table *kernels;
};

struct dsp_xml_lib_table {
	unsigned int lib_cnt;
	struct dsp_hash_tab lib_hash;
};

void dsp_xml_parser_init(void);
void dsp_xml_parser_free(void);
int dsp_xml_parser_parse(struct dsp_dl_lib_file *file,
		struct string_manager *str_manager, unsigned int unique_id);
void dsp_xml_lib_free(struct dsp_xml_lib *lib);

extern struct dsp_xml_lib_table *xml_libs;

#endif
