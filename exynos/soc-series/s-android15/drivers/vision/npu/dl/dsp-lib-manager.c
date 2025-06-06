// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series dsp driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 */

#include "dsp-common.h"
#include "dsp-dl-engine.h"
#include "dsp-lib-manager.h"
#include "dsp-pm-manager.h"
#include "dsp-gpt-manager.h"
#include "dsp-dl-out-manager.h"
#include "dsp-xml-parser.h"

static const char *dl_lib_path;
static struct dsp_hash_tab *dsp_lib_hash;

int dsp_lib_init(struct dsp_lib *lib, struct dsp_dl_lib_info *info)
{
	lib->name = (char *)dsp_dl_malloc(strlen(info->name) + 1,
			"lib name");
	if (!lib->name) {
		dsp_err("Alloc is failed (lib name)\n");
		return -ENOMEM;
	}

	strcpy(lib->name, info->name);
	dsp_dbg("lib name : %s\n", lib->name);

	lib->elf = NULL;
	lib->pm = NULL;
	lib->gpt = NULL;
	lib->dl_out = NULL;
	lib->dl_out_mem = NULL;
	lib->link_info = NULL;
	lib->ref_cnt = 0;
	lib->loaded = 0;

	return 0;
}

void dsp_lib_unload(struct dsp_lib *lib)
{
	dsp_dbg("DL lib unload\n");

	if (lib->elf) {
		dsp_elf32_free(lib->elf);
		dsp_dl_free(lib->elf);
		lib->elf = NULL;
	}

	if (lib->link_info) {
		dsp_link_info_free(lib->link_info);
		dsp_dl_free(lib->link_info);
		lib->link_info = NULL;
	}
}

void dsp_lib_free(struct dsp_lib *lib)
{
	dsp_dbg("DL lib free\n");

	dsp_dl_free(lib->name);

	dsp_lib_unload(lib);

	dsp_pm_free(lib);
	dsp_gpt_free(lib);
	dsp_dl_out_free(lib);
}

#ifndef CONFIG_NPU_KUNIT_TEST
void dsp_lib_print(struct dsp_lib *lib)
{
	dsp_dump(DL_BORDER);
	dsp_dump("Library name(%s) ref_cnt(%u) %s\n",
		lib->name, lib->ref_cnt,
		(lib->loaded) ? "loaded" : "unloaded");

	if (lib->elf) {
		dsp_dbg("\n");
		dsp_dbg("ELF information\n");
		dsp_elf32_print(lib->elf);
	}

	if (lib->pm) {
		dsp_dump("\n");
		dsp_dump("Program memory\n");
		dsp_tlsf_mem_print(lib->pm);
		dsp_dbg("\n");
		dsp_pm_print(lib);
	}

	if (lib->link_info) {
		dsp_dump("\n");
		dsp_dump("Linking information\n");
		dsp_link_info_print(lib->link_info);
	}

	if (lib->gpt) {
		dsp_dump("\n");
		dsp_dump("Global pointer\n");
		dsp_gpt_print(lib->gpt);
	}

	if (lib->dl_out_mem) {
		dsp_dump("\n");
		dsp_dump("Loader output\n");
		dsp_tlsf_mem_print(lib->dl_out_mem);
		dsp_dl_out_print(lib->dl_out);
	}
}
#endif

int dsp_lib_manager_init(const char *lib_path)
{
	dsp_dbg("DL lib init\n");
	dsp_lib_hash = (struct dsp_hash_tab *)dsp_dl_malloc(
			sizeof(struct dsp_hash_tab),
			"Dsp lib hash");
	if (!dsp_lib_hash) {
		dsp_err("dsp_lib_manager_init: alloc is failed\n");
		return -1;
	}

	dl_lib_path = lib_path;
	dsp_dbg("%s\n", dl_lib_path);
	dsp_hash_tab_init(dsp_lib_hash);

	return 0;
}

void dsp_lib_manager_free(void)
{
	unsigned int idx;

	dsp_dbg("DL lib_manager free\n");

	for (idx = 0; idx < DSP_HASH_MAX; idx++) {
		struct dsp_list_node *cur, *next;

		cur = (&dsp_lib_hash->list[idx])->next;

		while (cur != NULL) {
			struct dsp_hash_node *hash_node =
				container_of(cur, struct dsp_hash_node, node);
			struct dsp_lib *lib =
				(struct dsp_lib *)hash_node->value;

			next = cur->next;
			dsp_lib_free(lib);
			cur = next;
		}
	}

	dsp_hash_free(dsp_lib_hash, 1);
	dsp_dl_free(dsp_lib_hash);
}

#ifndef CONFIG_NPU_KUNIT_TEST
void dsp_lib_manager_print(void)
{
	unsigned int idx;

	dsp_dump(DL_BORDER);
	dsp_dump("Library manager\n");

	for (idx = 0; idx < DSP_HASH_MAX; idx++) {
		struct dsp_list_node *cur, *next;

		cur = (&dsp_lib_hash->list[idx])->next;

		while (cur != NULL) {
			struct dsp_hash_node *hash_node =
				container_of(cur, struct dsp_hash_node, node);
			struct dsp_lib *lib =
				(struct dsp_lib *)hash_node->value;

			next = cur->next;
			dsp_lib_print(lib);
			cur = next;
		}
	}
}
#endif

struct dsp_lib **dsp_lib_manager_get_libs(struct dsp_dl_lib_info *lib_infos,
	size_t lib_infos_size)
{
	int ret;
	unsigned int idx;
	struct dsp_lib **libs = (struct dsp_lib **)dsp_dl_malloc(
			sizeof(*libs) * lib_infos_size,
			"struct dsp_lib list");
	if (!libs) {
		dsp_err("Alloc is failed (sturct dsp_lib list)\n");
		return NULL;
	}

	dsp_dbg("DL lib_manager get libs\n");

	for (idx = 0; idx < lib_infos_size; idx++) {
		struct dsp_lib *lib;
		const char *name = lib_infos[idx].name;

		ret = dsp_hash_get(dsp_lib_hash, name, (void **)&lib);
		if (ret == -1) {
			dsp_dbg("Create library %s\n", name);
			lib = (struct dsp_lib *)dsp_dl_malloc(
					sizeof(*lib), "Lib created");
			if (!lib) {
				dsp_err("Alloc is failed (Lib created)\n");
				return NULL;
			}

			ret = dsp_lib_init(lib, &lib_infos[idx]);
			if (ret) {
				dsp_err("dsp_lib_init is failed\n");
				return NULL;
			}

			if (dsp_hash_push(dsp_lib_hash, name, lib)) {
				dsp_err("dsp_hash_push is failed\n");
				dsp_lib_manager_delete_unloaded_libs(libs, idx);
				dsp_dl_free(libs);
				return NULL;
			}
			libs[idx] = lib;
		} else {
			dsp_dbg("Check lib duplicate\n");
			if (!lib->loaded) {
				dsp_err("Library(%s) is duplicated\n",
					lib->name);
				dsp_lib_manager_delete_unloaded_libs(libs, idx);
				dsp_dl_free(libs);
				return NULL;
			}
		}

		libs[idx] = lib;
	}

	return libs;
}

void dsp_lib_manager_inc_ref_cnt(struct dsp_lib **libs, size_t libs_size)
{
	unsigned int idx;

	dsp_dbg("DL lib_manager inc_ref_cnt\n");

	for (idx = 0; idx < libs_size; idx++)
		libs[idx]->ref_cnt++;
}

void dsp_lib_manager_dec_ref_cnt(struct dsp_lib **libs, size_t libs_size)
{
	unsigned int idx;

	dsp_dbg("DL lib_manager dec_ref_cnt\n");
	for (idx = 0; idx < libs_size; idx++)
		libs[idx]->ref_cnt--;
}

void dsp_lib_manager_delete_unloaded_libs(struct dsp_lib **libs,
	size_t libs_size)
{
	unsigned int idx;

	dsp_dbg("DL lib_manager delete unloaded libs\n");
	for (idx = 0; idx < libs_size; idx++) {
		if (!libs[idx]->loaded)
			dsp_lib_manager_delete_lib(libs[idx]);
	}
}

void __dsp_lib_manager_delete_xml(char *name)
{
	int ret;
	struct dsp_xml_lib *xml_lib;

	dsp_dbg("__dsp_lib_manager_delete_xml start, lib->name[%s]\n", name);
	ret = dsp_hash_get(&xml_libs->lib_hash, name,
			(void **)&xml_lib);
	if (ret == -1) {
		dsp_dbg("No Library %s\n", name);
		return;
	}
	dsp_xml_lib_free(xml_lib);
	dsp_hash_pop(&xml_libs->lib_hash, name, 1);
}

void dsp_lib_manager_delete_remain_xml(struct string_manager *manager)
{
	for (int i = 0; i < manager->count; i++) {
		__dsp_lib_manager_delete_xml(get_string(manager, i));
	}
	free_strings(manager);
}

void dsp_lib_manager_delete_lib(struct dsp_lib *lib)
{
	dsp_hash_pop(dsp_lib_hash, lib->name, 0);
	__dsp_lib_manager_delete_xml(lib->name);
	dsp_lib_free(lib);
	dsp_dl_free(lib);
}

int dsp_lib_manager_delete_no_ref(void)
{
	unsigned int idx;
	int ret = 0;

	dsp_dbg("Delete no ref libs\n");
	for (idx = 0; idx < DSP_HASH_MAX; idx++) {
		struct dsp_list_node *cur, *next;

		cur = (&dsp_lib_hash->list[idx])->next;
		while (cur != NULL) {
			struct dsp_hash_node *hash_node =
				container_of(cur, struct dsp_hash_node, node);
			struct dsp_lib *lib =
				(struct dsp_lib *)hash_node->value;

			if (lib->loaded && lib->ref_cnt == 0) {
				dsp_dbg("Delete lib(%s)\n", lib->name);
				ret = 1;
				dsp_lib_manager_delete_lib(lib);
			}

			next = cur->next;
			cur = next;
		}
	}
	return ret;
}

static int __dsp_lib_manager_load_kernel_table(struct dsp_lib *lib,
	struct dsp_dl_out_section sec)
{
	unsigned int ret;
	struct dsp_xml_lib *xml_lib;
	struct dsp_dl_kernel_table *kernel_table;
	unsigned int idx;
	char *data;

	ret = dsp_hash_get(&xml_libs->lib_hash, lib->name,
			(void **)&xml_lib);
	if (ret == -1U) {
		dsp_err("No Library %s\n", lib->name);
		return -1;
	}

	data = (char *)lib->dl_out->data;
	kernel_table = (struct dsp_dl_kernel_table *)(data +
			sec.offset);

	for (idx = 0; idx < xml_lib->kernel_cnt; idx++) {
		struct dsp_xml_kernel_table *kernel = &xml_lib->kernels[idx];

		ret = dsp_link_info_get_kernel_addr(lib->link_info,
				kernel->pre);
		if (ret == (unsigned int) -1) {
			dsp_err("pre CHK_ERR\n");
			return -1;
		}

		kernel_table[idx].pre = ret;

		ret = dsp_link_info_get_kernel_addr(lib->link_info,
				kernel->exe);
		if (ret == (unsigned int) -1) {
			dsp_err("exe CHK_ERR\n");
			return -1;
		}

		kernel_table[idx].exe = ret;

		ret = dsp_link_info_get_kernel_addr(lib->link_info,
				kernel->post);
		if (ret == (unsigned int) -1) {
			dsp_err("post CHK_ERR\n");
			return -1;
		}

		kernel_table[idx].post = ret;
	}

	return 0;
}

static void __dsp_lib_manager_load_gpt(struct dsp_lib *lib)
{
	dsp_dbg("load gpt\n");
	if (likely(lib && lib->dl_out)) {
		lib->dl_out->gpt_addr = (unsigned int)lib->gpt->addr;
	}
}

static void __dsp_lib_manager_load_pm(struct dsp_lib *lib)
{
	struct dsp_elf32 *elf = lib->elf;
	struct dsp_list_node *node;

	dsp_dbg("load pm\n");
	dsp_list_for_each(node, &elf->text.text) {
		struct dsp_elf32_idx_node *idx_node =
			container_of(node, struct dsp_elf32_idx_node, node);
		unsigned int ndx = idx_node->idx;
		struct dsp_elf32_shdr *text_hdr = elf->shdr + ndx;
		unsigned char *text = (unsigned char *)(elf->data
				+ text_hdr->sh_offset);
		unsigned char *text_end = text + text_hdr->sh_size;
		unsigned char *dest = (unsigned char *)(dsp_pm_start_addr +
				lib->link_info->sec[ndx]);

		dsp_dbg("Dest : %p, Src : %p, Src end : %p, size : %u\n",
			dest, text, text_end, text_hdr->sh_size);

		for (; text < text_end; text += 4, dest += 4) {
			int idx;

			for (idx = 0; idx < 4; idx++)
				dest[idx] = text[3 - idx];
		}
	}
}

static void __dsp_lib_manager_load_bss_sec(struct dsp_lib *lib,
	struct dsp_list_head *head, struct dsp_dl_out_section sec)
{
	struct dsp_list_node *node;
	struct dsp_elf32 *elf = lib->elf;

	dsp_dbg("load bss sec\n");
	dsp_list_for_each(node, head) {
		struct dsp_elf32_idx_node *idx_node =
			container_of(node, struct dsp_elf32_idx_node, node);
		unsigned int ndx = idx_node->idx;
		struct dsp_elf32_shdr *mem_hdr = elf->shdr + ndx;
		char *data = lib->dl_out->data;
		char *dest = data + sec.offset +
			lib->link_info->sec[ndx];
		unsigned long end = (unsigned long)(dest + mem_hdr->sh_size);
		unsigned int *addr;

		dsp_dbg("load sec %u\n", ndx);
		dsp_dbg("Dest : %p, size : %u\n", dest, mem_hdr->sh_size);

		for (addr = (unsigned int *)dest; (unsigned long)addr < end;
			addr++)
			*addr = 0;
	}
}

static void __dsp_lib_manager_load_sec(struct dsp_lib *lib,
	struct dsp_list_head *head, struct dsp_dl_out_section sec,
	int rev_endian)
{
	struct dsp_list_node *node;
	struct dsp_elf32 *elf = lib->elf;

	dsp_dbg("load sec\n");
	dsp_list_for_each(node, head) {
		struct dsp_elf32_idx_node *idx_node =
			container_of(node, struct dsp_elf32_idx_node, node);
		unsigned int ndx = idx_node->idx;
		struct dsp_elf32_shdr *mem_hdr = elf->shdr + ndx;
		unsigned char *data = (unsigned char *)(elf->data +
				mem_hdr->sh_offset);
		unsigned char *data_end = data + mem_hdr->sh_size;
		char *dl_data = lib->dl_out->data;
		unsigned char *dest = dl_data + sec.offset +
			lib->link_info->sec[ndx];

		dsp_dbg("load sec %u\n", ndx);
		dsp_dbg("Dest : %p, Src : %p, size : %u\n",
			dest, data, mem_hdr->sh_size);

		if (rev_endian) {
			for (; data < data_end; data += 4, dest += 4) {
				int idx;

				for (idx = 0; idx < 4; idx++)
					dest[idx] = data[3 - idx];
			}
		} else
			memcpy(dest, data, mem_hdr->sh_size);
	}
}

static void __dsp_lib_manager_load_mem(struct dsp_lib *lib,
	struct dsp_elf32_mem *mem, struct dsp_dl_out_section sec,
	int rev_endian)
{
	dsp_dbg("load mem\n");
	dsp_dbg("Load robss\n");
	__dsp_lib_manager_load_bss_sec(lib, &mem->robss, sec);

	dsp_dbg("Load rodata\n");
	__dsp_lib_manager_load_sec(lib, &mem->rodata, sec, rev_endian);

	dsp_dbg("Load bss\n");
	__dsp_lib_manager_load_bss_sec(lib, &mem->bss, sec);

	dsp_dbg("Load data\n");
	__dsp_lib_manager_load_sec(lib, &mem->data, sec, rev_endian);
}


int dsp_lib_manager_load_libs(struct dsp_lib **libs, size_t libs_size)
{
	int ret;
	unsigned int idx;

	dsp_dbg(DL_BORDER);
	dsp_dbg("DL lib_manager load libs\n");
	for (idx = 0; idx < libs_size; idx++) {
		if (!libs[idx]->loaded) {
			struct dsp_dl_out *dl_out = libs[idx]->dl_out;

			if (dl_out) {
				char *data = dl_out->data;
				dsp_dbg("DL out data addr : %p\n",
					data);

				dsp_dbg("Load Kernel table\n");
				ret = __dsp_lib_manager_load_kernel_table(
						libs[idx],
						dl_out->kernel_table);
				if (ret == -1) {
					dsp_err("CHK_ERR\n");
					return -1;
				}
			}

			if (libs[idx]->pm) {
				dsp_dbg("Load PM\n");
				__dsp_lib_manager_load_pm(libs[idx]);
			}

			if (libs[idx]->gpt) {
				dsp_dbg("Load GPT\n");
				__dsp_lib_manager_load_gpt(libs[idx]);
			}

			if (dl_out && libs[idx]->dl_out_mem) {
				dsp_dbg("Load DM\n");
				__dsp_lib_manager_load_mem(libs[idx],
					&libs[idx]->elf->DMb,
					dl_out->DM_sh, 0);
				dsp_dbg("Load DM_local\n");
				__dsp_lib_manager_load_mem(libs[idx],
					&libs[idx]->elf->DMb_local,
					dl_out->DM_local, 0);
				dsp_dbg("Load TCM\n");
				__dsp_lib_manager_load_mem(libs[idx],
					&libs[idx]->elf->TCMb,
					dl_out->TCM_sh, 0);
				dsp_dbg("Load TCM_local\n");
				__dsp_lib_manager_load_mem(libs[idx],
					&libs[idx]->elf->TCMb_local,
					dl_out->TCM_local, 0);
				dsp_dbg("Load Shared mem\n");
				__dsp_lib_manager_load_mem(libs[idx],
					&libs[idx]->elf->SFRw,
					dl_out->sh_mem, 1);
			}
			libs[idx]->loaded = 1;
		}
	}

	return 0;
}

void dsp_lib_manager_unload_libs(struct dsp_lib **libs, size_t libs_size)
{
	unsigned int idx;

	dsp_dbg("DL lib_manager unload libs\n");

	dsp_lib_manager_dec_ref_cnt(libs, libs_size);
	for (idx = 0; idx < libs_size; idx++) {
		if (libs[idx]->ref_cnt == 0)
			dsp_lib_unload(libs[idx]);
	}

}
