// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "is-hw.h"
#include "pablo-hw-api-dlfe.h"
#include "pablo-hw-dlfe.h"
#include "pablo-hw-common-ctrl.h"
#include "pablo-json.h"
#include "is-hw-param-debug.h"

#define GET_HW(hw_ip)				((struct pablo_hw_dlfe *)hw_ip->priv_info)
#define CALL_HW_DLFE_OPS(hw, op, args...)	((hw && hw->ops) ? hw->ops->op(args) : 0)
#define GET_EN_STR(en)				(en ? "ON" : "OFF")

/* DEBUG module params */
enum pablo_hw_dlfe_dbg_type {
	DLFE_DBG_NONE = 0,
	DLFE_DBG_S_TRIGGER,
	DLFE_DBG_CR_DUMP,
	DLFE_DBG_STATE_DUMP,
	DLFE_DBG_EN_STRGEN,
	DLFE_DBG_S2D,
	DLFE_DBG_APB_DIRECT,
	DLFE_DBG_TYPE_NUM,
};

struct pablo_hw_dlfe_dbg_info {
	bool en[DLFE_DBG_TYPE_NUM];
	u32 instance;
	u32 fcount;
	u32 int_msk[2];
};

static struct pablo_hw_dlfe_dbg_info dbg_info = {
	.en = {false, },
	.instance = 0,
	.fcount = 0,
	.int_msk = {0, },
};

static int _parse_dbg_s_trigger(char **argv, int argc)
{
	int ret;
	u32 instance, fcount, int_msk0, int_msk1;

	if (argc != 4) {
		err("[DBG_DLFE] Invalid arguments! %d", argc);
		return -EINVAL;
	}

	ret = kstrtouint(argv[0], 0, &instance);
	if (ret) {
		err("[DBG_DLFE] Invalid instance %d ret %d", instance, ret);
		return -EINVAL;
	}

	ret = kstrtouint(argv[1], 0, &fcount);
	if (ret) {
		err("[DBG_DLFE] Invalid fcount %d ret %d", fcount, ret);
		return -EINVAL;
	}

	ret = kstrtouint(argv[2], 0, &int_msk0);
	if (ret) {
		err("[DBG_DLFE] Invalid int_msk0 %d ret %d", int_msk0, ret);
		return -EINVAL;
	}

	ret = kstrtouint(argv[3], 0, &int_msk1);
	if (ret) {
		err("[DBG_DLFE] Invalid int_msk1 %d ret %d", int_msk1, ret);
		return -EINVAL;
	}

	dbg_info.instance = instance;
	dbg_info.fcount = fcount;
	dbg_info.int_msk[0] = int_msk0;
	dbg_info.int_msk[1] = int_msk1;

	info("[DBG_DLFE] S_TRIGGER:[I%d][F%d] INT0 0x%08x INT1 0x%08x\n",
			dbg_info.instance,
			dbg_info.fcount,
			dbg_info.int_msk[0], dbg_info.int_msk[1]);

	return 0;
}

typedef int (*dbg_parser)(char **argv, int argc);
struct dbg_parser_info {
	char name[NAME_MAX];
	char man[NAME_MAX];
	dbg_parser parser;
};

static struct dbg_parser_info dbg_parsers[DLFE_DBG_TYPE_NUM] = {
	[DLFE_DBG_S_TRIGGER] = {
		"S_TRIGGER",
		"<instance> <fcount> <int_msk0> <int_msk1> ",
		_parse_dbg_s_trigger,
	},
	[DLFE_DBG_CR_DUMP] = {
		"CR_DUMP",
		"",
		NULL,
	},
	[DLFE_DBG_STATE_DUMP] = {
		"STATE_DUMP",
		"",
		NULL,
	},
	[DLFE_DBG_EN_STRGEN] = {
		"EN_STRGEN",
		"",
		NULL,
	},
	[DLFE_DBG_S2D] = {
		"S2D",
		"",
		NULL,
	},
	[DLFE_DBG_APB_DIRECT] = {
		"APB_DIRECT",
		"",
		NULL,
	},
};

static int pablo_hw_dlfe_dbg_set(const char *val)
{
	int ret = 0, argc = 0;
	char **argv;
	u32 dbg_type, en, arg_i = 0;

	argv = argv_split(GFP_KERNEL, val, &argc);
	if (!argv) {
		err("[DBG_DLFE] No argument!");
		return -EINVAL;
	} else if (argc < 2) {
		err("[DBG_DLFE] Too short argument!");
		goto func_exit;
	}

	ret = kstrtouint(argv[arg_i++], 0, &dbg_type);
	if (ret || !dbg_type || dbg_type >= DLFE_DBG_TYPE_NUM) {
		err("[DBG_DLFE] Invalid dbg_type %u ret %d", dbg_type, ret);
		goto func_exit;
	}

	ret = kstrtouint(argv[arg_i++], 0, &en);
	if (ret) {
		err("[DBG_DLFE] Invalid en %u ret %d", en, ret);
		goto func_exit;
	}

	dbg_info.en[dbg_type] = en;
	info("[DBG_DLFE] %s[%s]\n", dbg_parsers[dbg_type].name, GET_EN_STR(dbg_info.en[dbg_type]));

	argc = (argc > arg_i) ? (argc - arg_i) : 0;

	if (argc && dbg_parsers[dbg_type].parser && dbg_parsers[dbg_type].parser(&argv[arg_i], argc)) {
		err("[DBG_DLFE] Failed to %s", dbg_parsers[dbg_type].name);
		goto func_exit;
	}

func_exit:
	argv_free(argv);
	return ret;
}

static int pablo_hw_dlfe_dbg_get(char *buffer, const size_t buf_size)
{
	int ret;
	u32 dbg_type;

	const char *print_value_msg = "= DLFE DEBUG Configuration =====================\n"
				    "  Current Trigger Point:\n"
				    "    - instance %d\n"
				    "    - fcount %d\n"
				    "    - int0_msk 0x%08x int1_msk 0x%08x\n";

	ret = scnprintf(buffer, buf_size, print_value_msg, dbg_info.instance, dbg_info.fcount,
		dbg_info.int_msk[0], dbg_info.int_msk[1]);

	for (dbg_type = 1; dbg_type < DLFE_DBG_TYPE_NUM; dbg_type++)
		ret += scnprintf(buffer + ret, buf_size - ret, "  %10s[%3s]\n",
			dbg_parsers[dbg_type].name, GET_EN_STR(dbg_info.en[dbg_type]));

	ret += scnprintf(
		buffer + ret, buf_size - ret, "================================================\n");

	return ret;
}

static int pablo_hw_dlfe_dbg_usage(char *buffer, const size_t buf_size)
{
	int ret = 0;
	u32 dbg_type;

	for (dbg_type = 1; dbg_type < DLFE_DBG_TYPE_NUM; dbg_type++)
		ret += scnprintf(buffer + ret, buf_size - ret,
			"  - %10s: echo %d <en> %s> debug_dlfe\n", dbg_parsers[dbg_type].name,
			dbg_type, dbg_parsers[dbg_type].man);

	return ret;
}

static struct pablo_debug_param debug_dlfe = {
	.type = IS_DEBUG_PARAM_TYPE_STR,
	.ops.set = pablo_hw_dlfe_dbg_set,
	.ops.get = pablo_hw_dlfe_dbg_get,
	.ops.usage = pablo_hw_dlfe_dbg_usage,
};

module_param_cb(debug_dlfe, &pablo_debug_param_ops, &debug_dlfe, 0644);

static void _dbg_handler(struct is_hw_ip *hw_ip, u32 irq_id, u32 status)
{
	struct pablo_hw_dlfe *hw;
	struct pablo_common_ctrl *pcc;
	u32 instance, fcount;

	hw = GET_HW(hw_ip);
	pcc = hw->pcc;
	instance = atomic_read(&hw_ip->instance);
	fcount = atomic_read(&hw_ip->fcount);

	if (dbg_info.instance && dbg_info.instance != instance)
		return;
	else if (dbg_info.fcount && dbg_info.fcount != fcount)
		return;
	else if (dbg_info.int_msk[irq_id] && !(dbg_info.int_msk[irq_id] & status))
		return;

	info("[DBG_DLFE] %s:[I%d][F%d] INT%d 0x%08x\n", __func__,
		instance, fcount, irq_id, status);

	if (dbg_info.en[DLFE_DBG_CR_DUMP])
		CALL_HW_DLFE_OPS(hw, dump, hw_ip->pmio, HW_DUMP_CR);

	if (dbg_info.en[DLFE_DBG_STATE_DUMP]) {
		CALL_PCC_OPS(pcc, dump, pcc, PCC_DUMP_FULL);
		CALL_HW_DLFE_OPS(hw, dump, hw_ip->pmio, HW_DUMP_DBG_STATE);
	}

	if (dbg_info.en[DLFE_DBG_S2D])
		is_debug_s2d(true, "DLFE_DBG_S2D");
}

/* DLFE HW OPS */
static struct pablo_mmio *_pablo_hw_dlfe_pmio_init(struct is_hw_ip *hw_ip)
{
	int ret;
	struct pablo_mmio *pmio;
	struct pmio_config *pcfg;

	pcfg = &hw_ip->pmio_config;

	pcfg->name = "DLFE";
	pcfg->mmio_base = hw_ip->mmio_base;
	pcfg->cache_type = PMIO_CACHE_NONE;
	pcfg->phys_base = hw_ip->regs_start[REG_SETA];

	dlfe_hw_g_pmio_cfg(pcfg);

	pmio = pmio_init(NULL, NULL, pcfg);
	if (IS_ERR(pmio))
		goto err_init;

	ret = pmio_field_bulk_alloc(pmio, &hw_ip->pmio_fields,
				pcfg->fields, pcfg->num_fields);
	if (ret) {
		serr_hw("Failed to alloc DLFE PMIO field_bulk. ret %d", hw_ip, ret);
		goto err_field_bulk_alloc;
	}

	return pmio;

err_field_bulk_alloc:
	pmio_exit(pmio);
	pmio = ERR_PTR(ret);
err_init:
	return pmio;
}

static void _pablo_hw_dlfe_pmio_deinit(struct is_hw_ip *hw_ip)
{
	struct pablo_mmio *pmio = hw_ip->pmio;

	pmio_field_bulk_free(pmio, hw_ip->pmio_fields);
	pmio_exit(pmio);
}

static int _pablo_hw_dlfe_cloader_init(struct is_hw_ip *hw_ip, u32 instance)
{
	struct pablo_hw_dlfe *hw;
	struct is_mem *mem;
	struct pablo_internal_subdev *pis;
	int ret;
	enum base_reg_index reg_id;
	resource_size_t reg_size;
	u32 reg_num, hdr_size;

	/* DLFE DMA uses the MMU of MCFP */
	hw = GET_HW(hw_ip);
	mem = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_iommu_mem, GROUP_ID_MCFP);
	pis = &hw->subdev_cloader;
	ret = pablo_internal_subdev_probe(pis, instance, mem, "CLOADER");
	if (ret) {
		mserr_hw("failed to probe internal sub-device for %s: %d", instance, hw_ip,
			 pis->name, ret);
		return ret;
	}

	reg_id = REG_SETA;
	reg_size = (hw_ip->regs_end[reg_id] - hw_ip->regs_start[reg_id] + 1) >> 1; /* Payload: Remove corex shadow region. */
	reg_num = DIV_ROUND_UP(reg_size, 4); /* 4 Bytes per 1 CR. */
	hdr_size = ALIGN(reg_num, 16); /* Header: 16 Bytes per 16 CRs. */
	pis->width = hdr_size + reg_size;
	pis->height = 1;
	pis->num_planes = 1;
	pis->num_batch = 1;
	pis->num_buffers = 1;
	pis->bits_per_pixel = BITS_PER_BYTE;
	pis->memory_bitwidth = BITS_PER_BYTE;
	pis->size[0] = ALIGN(DIV_ROUND_UP(pis->width * pis->memory_bitwidth, BITS_PER_BYTE), 32) *
		       pis->height;
	ret = CALL_I_SUBDEV_OPS(pis, alloc, pis);
	if (ret) {
		mserr_hw("failed to alloc internal sub-device for %s: %d", instance, hw_ip,
			 pis->name, ret);
		return ret;
	}

	return ret;
}

static inline int _pablo_hw_dlfe_pcc_init(struct is_hw_ip *hw_ip)
{
	struct pablo_hw_dlfe *hw = GET_HW(hw_ip);

	hw->pcc = pablo_common_ctrl_hw_get_pcc(&hw_ip->pmio_config);

	return CALL_PCC_HW_OPS(hw->pcc, init, hw->pcc, hw_ip->pmio, hw_ip->name, PCC_M2M, NULL);
}

static int pablo_hw_dlfe_open(struct is_hw_ip *hw_ip, u32 instance)
{
	struct pablo_hw_dlfe *hw;
	int ret;

	if (test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	frame_manager_probe(hw_ip->framemgr, "HWDLFE");

	hw_ip->priv_info = vzalloc(sizeof(struct pablo_hw_dlfe));
	if (!hw_ip->priv_info) {
		mserr_hw("Failed to alloc pablo_hw_dlfe", instance, hw_ip);
		return -ENOMEM;
	}

	hw = GET_HW(hw_ip);
	hw->ops = dlfe_hw_g_ops();

	/* Setup C-loader */
	ret = _pablo_hw_dlfe_cloader_init(hw_ip, instance);
	if (ret)
		return ret;

	/* Setup Common-CTRL */
	ret = _pablo_hw_dlfe_pcc_init(hw_ip);
	if (ret)
		return ret;

	set_bit(HW_OPEN, &hw_ip->state);

	msdbg_hw(2, "open\n", instance, hw_ip);

	return 0;
}

static int pablo_hw_dlfe_init(struct is_hw_ip *hw_ip, u32 instance, bool flag, u32 f_type)
{
	struct pablo_hw_dlfe *hw;

	hw = GET_HW(hw_ip);
	hw->config[instance].dlfe_en = 0;

	set_bit(HW_INIT, &hw_ip->state);

	msdbg_hw(2, "init\n", instance, hw_ip);

	return 0;
}

static int pablo_hw_dlfe_close(struct is_hw_ip *hw_ip, u32 instance)
{
	struct pablo_hw_dlfe *hw;
	struct pablo_internal_subdev *pis;
	int ret;

	if (!test_bit(HW_OPEN, &hw_ip->state))
		return 0;

	hw = GET_HW(hw_ip);
	ret = CALL_HW_DLFE_OPS(hw, wait_idle, hw_ip->pmio);
	if (ret)
		mserr_hw("failed to wait_idle. ret %d", instance, hw_ip, ret);

	CALL_PCC_HW_OPS(hw->pcc, deinit, hw->pcc);

	pis = &hw->subdev_cloader;
	CALL_I_SUBDEV_OPS(pis, free, pis);

	vfree(hw_ip->priv_info);
	hw_ip->priv_info = NULL;

	clear_bit(HW_OPEN, &hw_ip->state);

	msdbg_hw(2, "close\n", instance, hw_ip);

	return 0;
}

static int pablo_hw_dlfe_enable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct pablo_mmio *pmio;
	struct pablo_hw_dlfe *hw;
	struct pablo_common_ctrl *pcc;
	struct pablo_common_ctrl_cfg cfg;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	} else if (test_bit(HW_RUN, &hw_ip->state)) {
		return 0;
	}

	msdbg_hw(2, "enable: start\n", instance, hw_ip);

	hw = GET_HW(hw_ip);
	pcc = hw->pcc;
	pmio = hw_ip->pmio;

	if (unlikely(dbg_info.en[DLFE_DBG_EN_STRGEN])) {
		CALL_HW_DLFE_OPS(hw, s_strgen, pmio);
		cfg.fs_mode = PCC_ASAP;
	} else {
		cfg.fs_mode = PCC_VVALID_RISE;
	}

	CALL_HW_DLFE_OPS(hw, g_int_en, cfg.int_en);
	CALL_PCC_OPS(pcc, enable, pcc, &cfg);

	CALL_HW_DLFE_OPS(hw, init, pmio);

	hw_ip->pmio_config.cache_type = PMIO_CACHE_FLAT;
	if (pmio_reinit_cache(pmio, &hw_ip->pmio_config)) {
		mserr_hw("failed to reinit PMIO cache", instance, hw_ip);
		return -EINVAL;
	}

	if (unlikely(dbg_info.en[DLFE_DBG_APB_DIRECT])) {
		pmio_cache_set_only(pmio, false);
		pmio_cache_set_bypass(pmio, true);
	} else {
		pmio_cache_set_only(pmio, true);
	}

	set_bit(HW_RUN, &hw_ip->state);
	msdbg_hw(2, "enable: done\n", instance, hw_ip);

	return 0;
}

static int pablo_hw_dlfe_disable(struct is_hw_ip *hw_ip, u32 instance, ulong hw_map)
{
	struct pablo_hw_dlfe *hw;
	struct pablo_mmio *pmio;
	struct pablo_common_ctrl *pcc;
	int ret = 0;
	long timetowait;

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	msinfo_hw("disable: %s\n", instance, hw_ip,
			atomic_read(&hw_ip->status.Vvalid) == V_VALID ? "V_VALID" : "V_BLANK");

	hw = GET_HW(hw_ip);
	pcc = hw->pcc;
	pmio = hw_ip->pmio;

	timetowait = wait_event_timeout(hw_ip->status.wait_queue,
			!atomic_read(&hw_ip->status.Vvalid),
			IS_HW_STOP_TIMEOUT);

	if (!timetowait) {
		mserr_hw("wait FRAME_END timeout. timetowait %uus", instance, hw_ip,
				jiffies_to_usecs(IS_HW_STOP_TIMEOUT));
		CALL_PCC_OPS(pcc, dump, pcc, PCC_DUMP_FULL);
		CALL_HW_DLFE_OPS(hw, dump, pmio, HW_DUMP_DBG_STATE);
		ret = -ETIME;
	}

	if (hw_ip->run_rsc_state)
		return ret;

	/* Disable PMIO Cache */
	pmio_cache_set_only(pmio, false);
	pmio_cache_set_bypass(pmio, true);

	CALL_HW_DLFE_OPS(hw, reset, pmio);
	CALL_PCC_OPS(pcc, reset, pcc);

	clear_bit(HW_RUN, &hw_ip->state);
	clear_bit(HW_CONFIG, &hw_ip->state);

	return ret;
}

static void _pablo_hw_dlfe_update_param(struct is_hw_ip *hw_ip, struct is_frame *frame,
		struct is_mcfp_config *cfg, struct dlfe_param_set *p_set)
{
	struct is_param_region *p_region;
	struct mcfp_param *param;
	u32 width = 0, height = 0, dbg_lv = 2;

	p_region = frame->parameter;
	param = &p_region->mcfp;

	p_set->instance = frame->instance;
	p_set->fcount = frame->fcount;

	if (test_bit(PARAM_MCFP_OTF_INPUT, frame->pmap)) {
		width = param->otf_input.width;
		height = param->otf_input.height;
	}

	if (cfg->dlfe_en && (!width || !height)) {
		mserr_hw("[F%d] Invalid OTF input size. Force to disable.", p_set->instance, hw_ip, p_set->fcount);
		cfg->dlfe_en = 0;
	}

	/**
	 * S/W Workaround
	 *
	 * HW Align Constraint:
	 * - MCFP: 2px X 2px
	 * - DLFE: 4px X 2px
	 *
	 * For smoothing zoom scenario, 4px alignment cannot be accepted.
	 * DLFE, however, can finish frame processing with horizontaly aligned-up size with 4px.
	 */
	if (cfg->dlfe_en && ((width % 2) || (height % 2)))
		mserr_hw("[F%d] 2px align constraint violation. %dx%d",
			  p_set->instance, hw_ip, p_set->fcount,
			  width, height);

	if (cfg->dlfe_en && (width % 4)) {
		width = ALIGN(width, 4);
		msdbg_hw(2, "[F%d] horizontal align-up %d\n",
			 p_set->instance, hw_ip, p_set->fcount, width);
	}

	if (p_set->wgt_out.cmd && !cfg->dlfe_en) {
		p_set->curr_in.cmd = OTF_INPUT_COMMAND_DISABLE;
		p_set->prev_in.cmd = OTF_INPUT_COMMAND_DISABLE;
		p_set->wgt_out.cmd = OTF_OUTPUT_COMMAND_DISABLE;
		dbg_lv = 0;
	} else if (!p_set->wgt_out.cmd && cfg->dlfe_en) {
		p_set->wgt_out.cmd = OTF_OUTPUT_COMMAND_ENABLE;
		p_set->prev_in.cmd = OTF_INPUT_COMMAND_ENABLE;
		p_set->curr_in.cmd = OTF_INPUT_COMMAND_ENABLE;
		dbg_lv = 0;
	}

	if (p_set->curr_in.width != width || p_set->curr_in.height != height) {
		p_set->wgt_out.width = width;
		p_set->wgt_out.height = height;

		p_set->prev_in.width = width;
		p_set->prev_in.height = height;

		p_set->curr_in.width = width;
		p_set->curr_in.height = height;

		dbg_lv = 0;
	}

	msdbg_hw(dbg_lv, "[F%d] %s\n", p_set->instance, hw_ip, p_set->fcount, GET_EN_STR(cfg->dlfe_en));
}

static void _pablo_hw_dlfe_s_cmd(struct pablo_hw_dlfe *hw, struct c_loader_buffer *clb, u32 fcount,
				 struct pablo_common_ctrl_cmd *cmd)
{
	cmd->set_mode = PCC_APB_DIRECT;
	cmd->fcount = fcount;
	cmd->int_grp_en = CALL_HW_DLFE_OPS(hw, g_int_grp_en);

	if (!clb || unlikely(dbg_info.en[DLFE_DBG_APB_DIRECT]))
		return;

	cmd->base_addr = clb->header_dva;
	cmd->header_num = clb->num_of_headers;
	cmd->set_mode = PCC_DMA_DIRECT;
}

static int pablo_hw_dlfe_shot(struct is_hw_ip *hw_ip, struct is_frame *frame, ulong hw_map)
{
	struct pablo_hw_dlfe *hw;
	struct pablo_mmio *pmio;
	struct is_mcfp_config *cfg;
	struct dlfe_param_set *p_set;
	struct is_framemgr *framemgr;
	struct is_frame *cl_frame;
	struct is_priv_buf *pb;
	struct c_loader_buffer clb, *p_clb = NULL;
	struct pablo_common_ctrl *pcc;
	struct pablo_common_ctrl_frame_cfg frame_cfg;
	u32 instance, fcount, cur_idx;
	u32 prev_cmd;

	instance = frame->instance;
	fcount = frame->fcount;
	cur_idx = frame->cur_buf_index;

	msdbgs_hw(2, "[F%d]shot(%d)\n", instance, hw_ip, fcount, cur_idx);

	if (!test_bit_variables(hw_ip->id, &hw_map))
		return 0;

	if (!test_bit(HW_INIT, &hw_ip->state)) {
		mserr_hw("not initialized!!", instance, hw_ip);
		return -EINVAL;
	}

	hw = GET_HW(hw_ip);
	pmio = hw_ip->pmio;
	pcc = hw->pcc;

	/* HW parameter */
	cfg = &hw->config[instance];
	p_set = &hw->param_set[instance];
	prev_cmd = p_set->curr_in.cmd;

	/* Prepare C-Loader buffer */
	framemgr = GET_SUBDEV_I_FRAMEMGR(&hw->subdev_cloader);
	cl_frame = peek_frame(framemgr, FS_FREE);
	if (likely(!dbg_info.en[DLFE_DBG_APB_DIRECT] && cl_frame)) {
		memset(&clb, 0x00, sizeof(clb));
		clb.header_dva = cl_frame->dvaddr_buffer[0];
		clb.payload_dva = cl_frame->dvaddr_buffer[0] + 0x2000;
		clb.clh = (struct c_loader_header *)cl_frame->kvaddr_buffer[0];
		clb.clp = (struct c_loader_payload *)(cl_frame->kvaddr_buffer[0] + 0x2000);

		p_clb = &clb;
	}

	if (CALL_HW_HELPER_OPS(hw_ip, set_rta_regs, instance, COREX_DIRECT, false, frame,
			       (void *)p_clb))
		cfg->dlfe_en = 0;

	_pablo_hw_dlfe_update_param(hw_ip, frame, cfg, p_set);

	if (!p_set->curr_in.cmd) {
		clear_bit(hw_ip->id, &frame->core_flag);

		/* Mark dummy event to prevent false alarm of DLFE timeout. */
		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, fcount, DEBUG_POINT_FRAME_START);
		CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, fcount, DEBUG_POINT_FRAME_END);

		if (prev_cmd == p_set->curr_in.cmd)
			goto skip_hw_set;
	}

	memset(&frame_cfg, 0, sizeof(struct pablo_common_ctrl_frame_cfg));

	CALL_HW_DLFE_OPS(hw, s_core, pmio, p_set);
	CALL_HW_DLFE_OPS(hw, s_path, pmio, p_set, &frame_cfg);

	if (likely(p_clb)) {
		/* Sync PMIO cache */
		pmio_cache_fsync(pmio, (void *)&clb, PMIO_FORMATTER_PAIR);

		/* Flush Host CPU cache */
		pb = cl_frame->pb_output;
		CALL_BUFOP(pb, sync_for_device, pb, 0, pb->size, DMA_TO_DEVICE);

		if (clb.num_of_pairs > 0)
			clb.num_of_headers++;
	}

	/* Prepare CMD for CMDQ */
	_pablo_hw_dlfe_s_cmd(hw, p_clb, fcount, &frame_cfg.cmd);

	CALL_PCC_OPS(pcc, shot, pcc, &frame_cfg);

skip_hw_set:
	return 0;
}

static int pablo_hw_dlfe_frame_ndone(struct is_hw_ip *hw_ip, struct is_frame *frame,
		enum ShotErrorType done_type)
{
	if (test_bit(hw_ip->id, &frame->core_flag))
		return CALL_HW_OPS(hw_ip, frame_done, hw_ip, frame, -1,
				IS_HW_CORE_END, done_type, false);

	return 0;
}

static int pablo_hw_dlfe_notify_timeout(struct is_hw_ip *hw_ip, u32 instance)
{
	struct pablo_hw_dlfe *hw = GET_HW(hw_ip);
	struct pablo_common_ctrl *pcc = hw->pcc;

	CALL_PCC_OPS(pcc, dump, pcc, PCC_DUMP_FULL);
	CALL_HW_DLFE_OPS(hw, dump, hw_ip->pmio, HW_DUMP_DBG_STATE);

	return 0;
}

static int pablo_hw_dlfe_reset(struct is_hw_ip *hw_ip, u32 instance)
{
	struct pablo_hw_dlfe *hw = GET_HW(hw_ip);
	struct pablo_common_ctrl *pcc = hw->pcc;

	msinfo_hw("reset\n", instance, hw_ip);

	CALL_HW_DLFE_OPS(hw, reset, hw_ip->pmio);
	return CALL_PCC_OPS(pcc, reset, pcc);
}

static int pablo_hw_dlfe_restore(struct is_hw_ip *hw_ip, u32 instance)
{
	int ret;

	if (!test_bit(HW_OPEN, &hw_ip->state)) {
		mserr_hw("restore: Not opened", instance, hw_ip);
		return -EINVAL;
	}

	ret = CALL_HWIP_OPS(hw_ip, reset, instance);
	if (ret)
		return ret;

	return 0;
}

static int pablo_hw_dlfe_dump_regs(struct is_hw_ip *hw_ip, u32 instance, u32 fcount,
		struct cr_set *regs, u32 regs_size, enum is_reg_dump_type dump_type)
{
	struct pablo_hw_dlfe *hw = GET_HW(hw_ip);

	switch (dump_type) {
	case IS_REG_DUMP_TO_LOG:
		CALL_HW_DLFE_OPS(hw, dump, hw_ip->pmio, HW_DUMP_CR);
		break;
	default:
		/* Do nothing */
		break;
	}

	return 0;
}

static int pablo_hw_dlfe_set_config(struct is_hw_ip *hw_ip, u32 chain_id, u32 instance, u32 fcount,
		void *conf)
{
	struct pablo_hw_dlfe *hw = GET_HW(hw_ip);
	struct is_mcfp_config *org_cfg = &hw->config[instance];
	struct is_mcfp_config *new_cfg = (struct is_mcfp_config *)conf;

	FIMC_BUG(!new_cfg);

	if (org_cfg->dlfe_en != new_cfg->dlfe_en)
		msinfo_hw("[F%d]set_config:%s\n", instance, hw_ip, fcount,
			  GET_EN_STR(new_cfg->dlfe_en));

	memcpy(org_cfg, new_cfg, sizeof(struct is_mcfp_config));

	return 0;
}

static size_t pablo_hw_dlfe_dump_params(
	struct is_hw_ip *hw_ip, u32 instance, char *buf, size_t size)
{
	struct pablo_hw_dlfe *hw = GET_HW(hw_ip);
	struct dlfe_param_set *param;
	size_t rem = size;
	char *p = buf;

	param = &hw->param_set[instance];

	p = pablo_json_nstr(p, "hw name", hw_ip->name, strlen(hw_ip->name), &rem);
	p = pablo_json_uint(p, "hw id", hw_ip->id, &rem);

	p = dump_param_otf_input(p, "curr_in", &param->curr_in, &rem);
	p = dump_param_otf_input(p, "prev_in", &param->prev_in, &rem);
	p = dump_param_otf_output(p, "wgt_out", &param->wgt_out, &rem);

	p = pablo_json_uint(p, "instance", param->instance, &rem);
	p = pablo_json_uint(p, "fcount", param->fcount, &rem);
	p = pablo_json_bool(p, "crc_seed", param->crc_seed, &rem);

	return WRITTEN(size, rem);
}

const struct is_hw_ip_ops pablo_hw_dlfe_ops = {
	.open			= pablo_hw_dlfe_open,
	.init			= pablo_hw_dlfe_init,
	.close			= pablo_hw_dlfe_close,
	.enable			= pablo_hw_dlfe_enable,
	.disable		= pablo_hw_dlfe_disable,
	.shot			= pablo_hw_dlfe_shot,
	.frame_ndone		= pablo_hw_dlfe_frame_ndone,
	.notify_timeout		= pablo_hw_dlfe_notify_timeout,
	.reset			= pablo_hw_dlfe_reset,
	.restore		= pablo_hw_dlfe_restore,
	.dump_regs		= pablo_hw_dlfe_dump_regs,
	.set_config		= pablo_hw_dlfe_set_config,
	.dump_params		= pablo_hw_dlfe_dump_params,
};

static void fs_fe_handler(struct is_hw_ip *hw_ip, u32 instance, u32 fcount, u32 status)
{
	mswarn_hw("[F%d][INT0]FS/FE overlapped!! status 0x%08x", instance, hw_ip, fcount, status);
}

static void fs_handler(struct is_hw_ip *hw_ip, u32 instance, u32 fcount, u32 status)
{
	struct is_hardware *hardware = hw_ip->hardware;
	u32 dbg_hw_lv = atomic_read(&hardware->streaming[hardware->sensor_position[instance]]) ? 2 : 0;

	msdbg_hw(dbg_hw_lv, "[F%d]FS\n", instance, hw_ip, fcount);

	atomic_set(&hw_ip->status.Vvalid, V_VALID);
	CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, fcount, DEBUG_POINT_FRAME_START);
	CALL_HW_OPS(hw_ip, frame_start, hw_ip, instance);
}

static void fe_handler(struct is_hw_ip *hw_ip, u32 instance, u32 fcount, u32 status)
{
	struct is_hardware *hardware = hw_ip->hardware;
	u32 dbg_hw_lv = atomic_read(&hardware->streaming[hardware->sensor_position[instance]]) ? 2 : 0;

	msdbg_hw(dbg_hw_lv, "[F%d]FE\n", instance, hw_ip, fcount);

	CALL_HW_OPS(hw_ip, dbg_trace, hw_ip, fcount, DEBUG_POINT_FRAME_END);

	CALL_HW_OPS(hw_ip, frame_done, hw_ip, NULL, -1, IS_HW_CORE_END, IS_SHOT_SUCCESS, true);

	if (atomic_read(&hw_ip->count.fs) < atomic_read(&hw_ip->count.fe)) {
		mserr_hw("fs(%d), fe(%d), dma(%d), status(0x%x)",
				instance, hw_ip,
				atomic_read(&hw_ip->count.fs),
				atomic_read(&hw_ip->count.fe),
				atomic_read(&hw_ip->count.dma),
				status);
	}

	wake_up(&hw_ip->status.wait_queue);
}

static void int0_err_handler(struct is_hw_ip *hw_ip, u32 instance, u32 fcount, u32 status)
{
	struct pablo_hw_dlfe *hw;
	struct pablo_common_ctrl *pcc;

	mserr_hw("[F%d][INT0] HW Error!! status 0x%08x", instance, hw_ip, fcount, status);

	hw = GET_HW(hw_ip);
	pcc = hw->pcc;

	CALL_PCC_OPS(pcc, dump, pcc, PCC_DUMP_FULL);
	CALL_HW_DLFE_OPS(hw, dump, hw_ip->pmio, HW_DUMP_DBG_STATE);
	CALL_HW_DLFE_OPS(hw, dump, hw_ip->pmio, HW_DUMP_CR);
}

static void int1_err_handler(struct is_hw_ip *hw_ip, u32 instance, u32 fcount, u32 status)
{
	struct pablo_hw_dlfe *hw;
	struct pablo_common_ctrl *pcc;

	mserr_hw("[F%d][INT1] HW Error!! status 0x%08x", instance, hw_ip, fcount, status);

	hw = GET_HW(hw_ip);
	pcc = hw->pcc;

	CALL_PCC_OPS(pcc, dump, pcc, PCC_DUMP_FULL);
	CALL_HW_DLFE_OPS(hw, dump, hw_ip->pmio, HW_DUMP_DBG_STATE);
	CALL_HW_DLFE_OPS(hw, dump, hw_ip->pmio, HW_DUMP_CR);
}

typedef void (*int_handler)(struct is_hw_ip *hw_ip, u32 instance, u32 fcount, u32 status);
struct dlfe_irq_handler {
	bool		valid;
	ulong		bitmask;
	int_handler	func;
};

/**
 * Array of IRQ handler for each interrupt bitmask.
 * It will call each IRQ handler as the order of declaration.
 */

#define NUM_DLFE_IRQ_HANDLER	32
static struct dlfe_irq_handler irq_handler[][NUM_DLFE_IRQ_HANDLER] = {
	/* INT0 */
	{
		{
			.valid = true,
			.bitmask = (BIT_MASK(INT_FRAME_START) | BIT_MASK(INT_FRAME_END)),
			.func = fs_fe_handler,
		},
		{
			.valid = true,
			.bitmask = BIT_MASK(INT_FRAME_START),
			.func = fs_handler,
		},
		{
			.valid = true,
			.bitmask = BIT_MASK(INT_FRAME_END),
			.func = fe_handler,
		},
		{
			.valid = true,
			.bitmask = BIT_MASK(INT_ERR0),
			.func = int0_err_handler,
		},
	},
	/* INT1 */
	{
		{
			.valid = true,
			.bitmask = BIT_MASK(INT_ERR1),
			.func = int1_err_handler,
		}
	},
};

static int pablo_hw_dlfe_handler_int(u32 id, void *ctx)
{
	struct is_hw_ip *hw_ip;
	struct pablo_hw_dlfe *hw;
	struct pablo_common_ctrl *pcc;
	struct dlfe_irq_handler* handler;
	u32 instance, hw_fcount, status, i;
	bool occur;

	hw_ip = (struct is_hw_ip *)ctx;
	instance = atomic_read(&hw_ip->instance);
	if (!test_bit(HW_OPEN, &hw_ip->state)) {
		mserr_hw("Invalid INT%d. hw_state 0x%lx", instance, hw_ip, id, hw_ip->state);
		return 0;
	}

	hw = GET_HW(hw_ip);
	pcc = hw->pcc;
	hw_fcount = atomic_read(&hw_ip->fcount);
	handler = irq_handler[id];
	status = CALL_PCC_OPS(pcc, get_int_status, pcc, id, true);

	if (!test_bit(HW_RUN, &hw_ip->state)) {
		mserr_hw("Invalid INT%d. int_status 0x%08x hw_state 0x%lx", instance, hw_ip, id,
				status, hw_ip->state);
		return 0;
	}

	msdbg_hw(2, "[F%d][INT%d] status 0x%08x\n", instance, hw_ip, hw_fcount, id, status);

	if (unlikely(dbg_info.en[DLFE_DBG_S_TRIGGER]))
		_dbg_handler(hw_ip, id, status);

	for (i = 0; i < NUM_DLFE_IRQ_HANDLER; i++) {
		occur = CALL_HW_DLFE_OPS(hw, is_occurred, status, handler[i].bitmask);
		if (handler[i].valid && occur)
			handler[i].func(hw_ip, instance, hw_fcount, status);
	}

	return 0;
}

int pablo_hw_dlfe_probe(struct is_hw_ip *hw_ip, struct is_interface *itf,
		struct is_interface_ischain *itfc, int id, const char *name)
{
	struct pablo_mmio *pmio;
	int hw_slot;

	hw_ip->ops = &pablo_hw_dlfe_ops;

	hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, id);
	if (!valid_hw_slot_id(hw_slot)) {
		serr_hw("Invalid hw_slot %d", hw_ip, hw_slot);
		return -EINVAL;
	}

	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].handler = &pablo_hw_dlfe_handler_int;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].id = INTR_HWIP1;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].valid = true;

	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].handler = &pablo_hw_dlfe_handler_int;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].id = INTR_HWIP2;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].valid = true;

	hw_ip->mmio_base = hw_ip->regs[REG_SETA];

	pmio = _pablo_hw_dlfe_pmio_init(hw_ip);
	if (IS_ERR(pmio)) {
		serr_hw("Failed to DLFE pmio_init.", hw_ip);
		return -EINVAL;
	}

	hw_ip->pmio = pmio;

	return 0;
}
EXPORT_SYMBOL_GPL(pablo_hw_dlfe_probe);

void pablo_hw_dlfe_remove(struct is_hw_ip *hw_ip)
{
	struct is_interface_ischain *itfc = hw_ip->itfc;
	int id = hw_ip->id;
	int hw_slot = CALL_HW_CHAIN_INFO_OPS(hw_ip->hardware, get_hw_slot_id, id);

	itfc->itf_ip[hw_slot].handler[INTR_HWIP1].valid = false;
	itfc->itf_ip[hw_slot].handler[INTR_HWIP2].valid = false;

	_pablo_hw_dlfe_pmio_deinit(hw_ip);
}
EXPORT_SYMBOL_GPL(pablo_hw_dlfe_remove);
