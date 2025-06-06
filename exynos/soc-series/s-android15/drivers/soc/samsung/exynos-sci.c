/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/panic_notifier.h>
#if IS_ENABLED(CONFIG_EXYNOS_ESCA)
#include <soc/samsung/esca.h>
#else
#include <soc/samsung/acpm_ipc_ctrl.h>
#endif
#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-sci.h>
#include <soc/samsung/exynos-llcgov.h>

static struct exynos_sci_data *sci_data = NULL;

static u32 llc_way_max = 16;

u32 get_llc_way_max(void)
{
	return llc_way_max;
}
EXPORT_SYMBOL(get_llc_way_max);

static enum exynos_sci_err_code exynos_sci_ipc_err_handle(unsigned int cmd)
{
	enum exynos_sci_err_code err_code;

	err_code = SCI_CMD_GET(cmd, SCI_ERR_MASK, SCI_ERR_SHIFT);
	if (err_code)
		SCI_ERR("%s: SCI IPC error return(%u)\n", __func__, err_code);

	return err_code;
}

static int __exynos_sci_ipc_send_data(enum exynos_sci_cmd_index cmd_index,
				struct exynos_sci_data *data,
				unsigned int *cmd)
{
#if defined(CONFIG_EXYNOS_ACPM) || defined(CONFIG_EXYNOS_ACPM_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA)
	struct ipc_config config;
	unsigned int *sci_cmd;
#endif
	int ret = 0;

	if (cmd_index >= SCI_CMD_MAX) {
		SCI_ERR("%s: Invalid CMD Index: %u\n", __func__, cmd_index);
		ret = -EINVAL;
		goto out;
	}

#if defined(CONFIG_EXYNOS_ACPM) || defined(CONFIG_EXYNOS_ACPM_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA)
	sci_cmd = cmd;
	config.cmd = sci_cmd;
	config.response = true;
	config.indirection = false;

	ret = esca_ipc_send_data(data->ipc_ch_num, &config);
	if (ret) {
		SCI_ERR("%s: Failed to send IPC(%d:%u) data\n",
			__func__, cmd_index, data->ipc_ch_num);
		goto out;
	}
#endif

out:
	return ret;
}

static int exynos_sci_ipc_send_data(enum exynos_sci_cmd_index cmd_index,
				struct exynos_sci_data *data,
				unsigned int *cmd)
{
	return __exynos_sci_ipc_send_data(cmd_index, data, cmd);
}

static void exynos_sci_base_cmd(struct exynos_sci_cmd_info *cmd_info,
					unsigned int *cmd)
{
	cmd[0] |= SCI_CMD_SET(cmd_info->cmd_index,
				SCI_CMD_IDX_MASK, SCI_CMD_IDX_SHIFT);
	cmd[0] |= SCI_CMD_SET(cmd_info->direction,
				SCI_ONE_BIT_MASK, SCI_IPC_DIR_SHIFT);
	cmd[0] |= SCI_CMD_SET(cmd_info->data,
				SCI_DATA_MASK, SCI_DATA_SHIFT);
}

/*********************************************************/
/* IPC functions for llc control			 */
/*********************************************************/
static int exynos_sci_llc_on(struct exynos_sci_data *data,
					enum exynos_sci_ipc_dir direction,
					unsigned int *on)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	cmd_info.cmd_index = SCI_LLC_ON;
	cmd_info.direction = direction;
	cmd_info.data = *on;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	if (direction == SCI_IPC_GET)
		*on = SCI_CMD_GET(cmd[1], SCI_DATA_MASK, SCI_DATA_SHIFT);


	if (direction == SCI_IPC_SET)
		SCI_INFO("%s: LLC is turned %s(%u)\n", __func__,
			(*on)? "on" : "off", sci_data->on_cnt);
out:
	return ret;
}

static int exynos_sci_llc_region_alloc(struct exynos_sci_data *data,
					enum exynos_sci_ipc_dir direction,
					unsigned int *index,
					bool on,
					unsigned int way)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;
	unsigned int shared_idx = data->llc_region[*index].shared_idx;

	if (direction == SCI_IPC_SET)
		if (on)
			cmd_info.cmd_index = SCI_LLC_REGION_ALLOC;
		else
			cmd_info.cmd_index = SCI_LLC_REGION_DEALLOC;
	else
		cmd_info.cmd_index = SCI_LLC_REGION_ALLOC;

	cmd_info.direction = direction;
	cmd_info.data = *index;
	cmd[2] = way;
	cmd[3] |= SCI_CMD_SET(data->qpd_onoff[*index], SCI_CMD_QPD_MASK, SCI_CMD_QPD_SHIFT);
	cmd[3] |= SCI_CMD_SET(shared_idx, SCI_CMD_SHARED_MASK, SCI_CMD_SHARED_SHIFT);

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed to send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	if (direction == SCI_IPC_GET)
		*index = cmd[3];

	if (direction == SCI_IPC_SET) {
		SCI_DBG("%s:%d: LLC_REGION_ALLOC SET Index(%u) Way(%u)\n",
					__func__, __LINE__, *index, way);

		data->llc_shared_data[shared_idx].policy_index[*index] = cmd[3];
		if (on)
			data->llc_shared_data[shared_idx].index_cnt++;
		else if (!on && data->llc_shared_data[shared_idx].index_cnt)
			data->llc_shared_data[shared_idx].index_cnt--;
	} else {
		SCI_DBG("%s:%d: LLC_REGION_ALLOC GET 0x%8X\n",
					__func__, __LINE__, *index);
	}

out:
	return ret;
}

static int exynos_sci_llc_get_region_info(struct exynos_sci_data *data,
					enum exynos_sci_ipc_dir direction,
					unsigned int region_index,
					unsigned int *way)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	if (direction != SCI_IPC_GET)
		return ret;

	cmd_info.cmd_index = SCI_LLC_GET_REGION_INFO;
	cmd_info.direction = direction;
	cmd_info.data = region_index;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed to send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	*way = SCI_CMD_GET(cmd[1], SCI_DATA_MASK, SCI_DATA_SHIFT);

out:
	return ret;
}

#if !(defined (CONFIG_EXYNOS_ESCAV1) || defined(CONFIG_EXYNOS_ESCAV1_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA))
static int exynos_sci_pd_sync(struct exynos_sci_data *data,
					enum exynos_sci_ipc_dir direction,
					unsigned int *cal_pdid)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int i, ret = 0;
	enum exynos_sci_err_code ipc_err;

	cmd_info.data = 0;
	for (i = 0; i < data->vch_size; i++) {
		if (*cal_pdid == data->vch_pd_calid[i]) {
			cmd_info.data = (*cal_pdid) & 0x0000FFFF;
			break;
		}
	}

	if (cmd_info.data == 0) {
		/* Nothing to do */
		return ret;
	}

	cmd_info.cmd_index = SCI_VCH_SET;
	cmd_info.direction = direction;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	if (direction == SCI_IPC_GET)
		*cal_pdid = SCI_CMD_GET(cmd[1], SCI_DATA_MASK, SCI_DATA_SHIFT);

out:
	return ret;
}
#endif

static int exynos_sci_llc_quadrant_control(struct exynos_sci_data *data,
		enum exynos_sci_ipc_dir direction,
		unsigned int *on,
		unsigned int *way)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	if (direction == SCI_IPC_SET) {
	}

	cmd_info.cmd_index = SCI_LLC_QUADRANT_EN;
	cmd_info.direction = direction;
	cmd_info.data = *on;
	cmd[2] = *way;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	if (direction == SCI_IPC_GET)
		*way = SCI_CMD_GET(cmd[2], SCI_DATA_MASK, SCI_DATA_SHIFT);

out:
	return ret;
}

static int exynos_sci_llc_shared_idx(struct exynos_sci_data *data,
					enum exynos_sci_ipc_dir direction,
					unsigned int *index)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	if (direction == SCI_IPC_GET)
		cmd_info.cmd_index = SCI_LLC_SHARED_IDX;

	cmd_info.direction = direction;
	cmd_info.data = *index;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed to send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	*index = cmd[3];

	SCI_DBG("%s:%d: LLC_REGION_ALLOC GET 0x%8X\n",
					__func__, __LINE__, *index);

out:
	return ret;
}

static int exynos_sci_llc_shared_way(struct exynos_sci_data *data,
					enum exynos_sci_ipc_dir direction,
					unsigned int *index)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	if (direction == SCI_IPC_GET)
		cmd_info.cmd_index = SCI_LLC_SHARED_WAY;

	cmd_info.direction = direction;
	cmd_info.data = *index;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed to send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	*index = cmd[3];
out:
	return ret;
}

static inline bool llc_get_disable_status(void)
{
	int i;

	for (i = 0; i < LLC_ENABLE_SOURCE_MAX; i++) {
		if (sci_data->llc_enable_status[i] == DISABLE_LLC)
			return DISABLE_LLC;
	}

	return ENABLE_LLC;
}

/*********************************************************/
/* Export symbol functions				 */
/*********************************************************/
int llc_set_enable(bool enable, enum exynos_sci_llc_enable_source_index src_index)
{
	int ret = 0;
	unsigned int i;
	unsigned int on = 0;
	unsigned long flags;

	spin_lock_irqsave(&sci_data->lock, flags);
	if (enable == sci_data->llc_enable)
		goto out;

	sci_data->llc_enable_status[src_index] = enable;

	if (enable == DISABLE_LLC) { // Disable LLC
		if (sci_data->llc_enable == ENABLE_LLC) {
			if (sci_data->on_cnt) {
				i = sci_data->llc_disable_threshold;
				do {
					if (sci_data->llc_region[i].usage_count)
						goto out;
					i++;
				} while (i < sci_data->region_size);
			}
			/* Disable LLC */
			ret = exynos_sci_llc_on(sci_data, SCI_IPC_SET, &on);
			if (ret) {
				SCI_ERR("%s: Failed to disable LLC\n", __func__);
				goto out;
			}
			sci_data->llc_enable = DISABLE_LLC;
			if (sci_data->llc_gov_en)
				sci_data->gov_data->llc_gov_en = enable;
		}
	} else if (llc_get_disable_status() == ENABLE_LLC) { // Enable LLC
		sci_data->llc_enable = ENABLE_LLC;
		if (sci_data->llc_gov_en)
			sci_data->gov_data->llc_gov_en = enable;
		if (sci_data->on_cnt) {
			on = 1;
			ret = exynos_sci_llc_on(sci_data, SCI_IPC_SET, &on);
			if (ret) {
				SCI_ERR("%s: Failed to enable LLC\n", __func__);
				goto out;
			}

			for (i = 0; i < sci_data->region_size; i++) {
				if (sci_data->llc_region[i].usage_count)
					ret = exynos_sci_llc_region_alloc(sci_data, SCI_IPC_SET,
						&i, on, sci_data->llc_region[i].way);
				if (ret) {
					SCI_ERR("%s: Failed to reset LLC region(%d) - Requested way %u\n",
							__func__, i, sci_data->llc_region[i].way);
					goto out;
				}
			}
		}
	}
out:
	spin_unlock_irqrestore(&sci_data->lock, flags);
	return ret;
}
EXPORT_SYMBOL(llc_set_enable);

int llc_region_alloc(unsigned int index, bool on, unsigned int way)
{
	int ret = 0;
	unsigned int uint_on = on;
	unsigned long flags;

	if (sci_data->llc_debug_mode) {
		SCI_INFO("%s:%d: LLC debug mode is on. Ignore request\n",
			__func__, __LINE__);
		return ret;
	}

	if (way > llc_way_max) {
		SCI_ERR("%s:%d: way request should be in range of 0-%d: Requested %u\n",
			__func__, __LINE__, llc_way_max, way);
		return -EINVAL;
	}

	spin_lock_irqsave(&sci_data->lock, flags);
	if (on == true) {
		sci_data->llc_region[index].usage_count++;
		sci_data->llc_region[index].way = way;

		if (sci_data->llc_region[index].usage_count == 1) {
			sci_data->on_cnt++;

			if (sci_data->on_cnt == 1) {
				/* Hardware control - LLC on */
				SCI_DBG("%s:%d: call exynos_sci_llc_on for on(on_cnt:%u llc_enable:%u)\n",
						__func__, __LINE__, sci_data->on_cnt, sci_data->llc_enable);
				if (sci_data->llc_enable)
					ret = exynos_sci_llc_on(sci_data, SCI_IPC_SET, &uint_on);
			}
		}

		if (sci_data->llc_enable)
			ret = exynos_sci_llc_region_alloc(sci_data, SCI_IPC_SET, &index, on, way);
	} else {
		if (sci_data->llc_region[index].usage_count <= 0) {
			SCI_ERR("%s:%d: Invalid usage count %d on index %u\n",
				__func__, __LINE__, sci_data->llc_region[index].usage_count, index);
			ret = -EINVAL;
			goto out;
		}

		sci_data->llc_region[index].usage_count--;

		if (sci_data->llc_region[index].usage_count == 0) {
			sci_data->llc_region[index].way = 0;

			if (sci_data->llc_enable)
				ret = exynos_sci_llc_region_alloc(sci_data, SCI_IPC_SET, &index, on, way);

			sci_data->on_cnt--;

			if (sci_data->on_cnt == 0 && sci_data->llc_enable) {
				/* Hardware control - LLC off */
				if (sci_data->llc_enable)
					ret = exynos_sci_llc_on(sci_data, SCI_IPC_SET, &uint_on);
				SCI_DBG("%s:%d: call exynos_sci_llc_on for off(on_cnt:%u llc_enable:%u)\n",
					__func__, __LINE__, sci_data->on_cnt, sci_data->llc_enable);
			}
		}
	}

out:
	spin_unlock_irqrestore(&sci_data->lock, flags);
	return ret;
}
EXPORT_SYMBOL(llc_region_alloc);

static int exynos_sci_llc_mpam_param_set(int index, int part_id, int pmon_gr, int ns, int on)
{
	struct exynos_sci_mpam_data data = {0, };
	int param = 0;

	if (on) {
		data.part_id = part_id;
		data.perf_mon_gr = pmon_gr;
		data.ns = ns;
		data.valen = on;
		param |= (part_id << 3) | (pmon_gr << 2) | (ns << 1) | (on << 0);
	}

	sci_data->mpam_data[index - LLC_REGION_CPU - 1] = data;

	SCI_INFO("%s: MPAM Param 0x%x:\n", __func__, param);

	return param;
}

static int exynos_sci_llc_mpam_alloc(struct exynos_sci_data *data, int param,
				int index, enum exynos_sci_ipc_dir direction)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	cmd_info.cmd_index = SCI_LLC_MPAM_ALLOC;
	cmd_info.direction = direction;

	index = index - LLC_REGION_CPU - 1;
	cmd_info.data = index;
	cmd[2] = param;
	cmd[3] = 0;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed to send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

out:
	return ret;
}

int llc_mpam_alloc(unsigned int index, int way, int pid, int pmon_gr, int ns, int on)
{
	int ret = 0;
	int mpam_param = 0;

	if (sci_data->llc_debug_mode) {
		SCI_INFO("%s:%d: LLC debug mode is on. Ignore request\n",
			__func__, __LINE__);
		return ret;
	}

	mpam_param = exynos_sci_llc_mpam_param_set(index, pid, pmon_gr, ns, on);

	if (on) {
		exynos_sci_llc_mpam_alloc(sci_data, mpam_param, index, SCI_IPC_SET);
	}

	llc_region_alloc(index, on, way);

	return ret;
}
EXPORT_SYMBOL(llc_mpam_alloc);

unsigned int llc_get_region_info(unsigned int region_index)
{
	int ret;
	unsigned int way = 0;
	unsigned long flags;

	if (sci_data->llc_debug_mode) {
		SCI_INFO("%s:%d: LLC debug mode is on. Ignore request\n",
			__func__, __LINE__);
		return 0;
	}

	if (!sci_data->llc_enable)
		return 0;

	if (region_index > sci_data->region_size)
		return 0;

	spin_lock_irqsave(&sci_data->lock, flags);
	ret = exynos_sci_llc_get_region_info(sci_data, SCI_IPC_GET, region_index, &way);
	if (ret)
		SCI_ERR("%s: Failed to get llc region info\n", __func__);

	spin_unlock_irqrestore(&sci_data->lock, flags);
	return way;
}
EXPORT_SYMBOL(llc_get_region_info);

int llc_get_region_index_by_name(const char *name)
{
	int index = 0;
	int ret = 0;
	unsigned long flags;

	if ((sci_data == NULL) || (sci_data->llc_region == NULL))
		return -EINVAL;

	spin_lock_irqsave(&sci_data->lock, flags);
	for (index = 0;
		(sci_data->llc_region[index].name != NULL) && (index < sci_data->region_size);
		index++) {
		if (!strcmp(sci_data->llc_region[index].name, name)) {
			ret = index;
			goto out;
		}
	}

	if (index == sci_data->region_size) {
		ret = -EINVAL;
		goto out;
	}

out:
	spin_unlock_irqrestore(&sci_data->lock, flags);
	return ret;
}
EXPORT_SYMBOL(llc_get_region_index_by_name);

static int exynos_sci_llc_ppc_en(struct exynos_sci_data *data,
				enum exynos_sci_ipc_dir direction, unsigned int *on)
{
	struct exynos_sci_cmd_info cmd_info = { 0, };
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	cmd_info.cmd_index = SCI_LLC_PPC_EN;
	cmd_info.direction = direction;
	cmd_info.data = *on;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed to send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	if (direction == SCI_IPC_GET)
		*on = SCI_CMD_GET(cmd[1], SCI_DATA_MASK, SCI_DATA_SHIFT);


	if (direction == SCI_IPC_SET)
		SCI_INFO("%s: LLC Clk Gating is %s\n", __func__,
			(*on)? "disabled" : "enabled");
out:
	return ret;
}

int llc_ppc_en(unsigned int on)
{
	int ret = 0;

	exynos_sci_llc_ppc_en(sci_data, SCI_IPC_SET, &on);

	return ret;
}
EXPORT_SYMBOL(llc_ppc_en);

void llc_qpd_on(unsigned int index, unsigned int on)
{
	unsigned long flags;

	spin_lock_irqsave(&sci_data->lock, flags);
	sci_data->qpd_onoff[index] = on;
	SCI_INFO("LLC INDEX %s:	QPD %s\n",
			sci_data->llc_region[index].name, on? "on" : "off");
	spin_unlock_irqrestore(&sci_data->lock, flags);

}
EXPORT_SYMBOL(llc_qpd_on);

#if !(defined (CONFIG_EXYNOS_ESCAV1) || defined(CONFIG_EXYNOS_ESCAV1_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA))
int sci_pd_sync(unsigned int cal_pdid, bool on)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&sci_data->lock, flags);
	ret = exynos_sci_pd_sync(sci_data, SCI_IPC_SET, &cal_pdid);
	spin_unlock_irqrestore(&sci_data->lock, flags);

	return ret;
}
#endif

static void print_sci_data(struct exynos_sci_data *data)
{
	int i = 0;

	SCI_INFO("IPC Channel Number: %u\n", data->ipc_ch_num);
	SCI_INFO("IPC Channel Size: %u\n", data->ipc_ch_size);
	SCI_INFO("LLC Enabled: %u\n", data->llc_enable);
	SCI_INFO("LLC Initial count: %u\n", data->on_cnt);
	for (i = 0; i < data->region_size; i++)
		SCI_INFO("LLC Region[%d]: %s(%d)\n", data->llc_region[i].priority,
				data->llc_region[i].name, data->llc_region[i].usage_count);
}

/*********************************************************/
/* SYSFS entries					 */
/*********************************************************/
static ssize_t llc_region_alloc_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	unsigned int i, index;
	int ret;

	if (!data->llc_enable || !data->on_cnt) {
		count += snprintf(buf + count, PAGE_SIZE, "LLC is off\n");
		return count;
	}

	for (i = 0; i < LLC_REGION_MAX; i++) {
		index = i;
		ret = exynos_sci_llc_region_alloc(data, SCI_IPC_GET, &index, 0, 0);
		if (ret) {
			count += snprintf(buf + count, PAGE_SIZE,
				"Failed to get allocated llc info\n");
			return count;
		}

		count += snprintf(buf + count, PAGE_SIZE,
				"LLC Region: %s\t\tStatus(%u)\tAllocated(%u)\tSharedIndex(%u)\n",
				data->llc_region[i].name, (index >> 16),
				(index & 0xFFFF),data->llc_region[i].shared_idx);
	}

	return count;
}

static ssize_t llc_region_alloc_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int index, on, way;
	int ret;

	ret = sscanf(buf, "%u %u %u", &index, &on, &way);
	if (ret != 3) {
		SCI_ERR("%s: usage: echo [index] [on] [way] > llc_region_alloc\n",
				__func__);
		return -EINVAL;
	}

	ret = llc_region_alloc(index, (bool)on, way);
	if (ret) {
		SCI_ERR("%s: failed to allocate llc region (%d)\n", __func__, ret);
		return ret;
	}

	return count;
}

static ssize_t llc_get_region_info_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	unsigned int i, index, way;
	int ret;

	if (!data->llc_enable || !data->on_cnt) {
		count += snprintf(buf + count, PAGE_SIZE, "LLC is off\n");
		return count;
	}

	for (i = 0; i < LLC_REGION_MAX; i++) {
		index = i;
		ret = exynos_sci_llc_get_region_info(data, SCI_IPC_GET, index, &way);
		if (ret) {
			count += snprintf(buf + count, PAGE_SIZE,
					"Failed to get allocated llc info\n");
			return count;
		}

		count += snprintf(buf + count, PAGE_SIZE,
				"LLC Region: %s(%u)\t\t\tAllocated way(%u)\n",
				data->llc_region[i].name, index, way);
	}

	ret = exynos_sci_llc_get_region_info(data, SCI_IPC_GET, LLC_REGION_MAX, &way);
	if (ret) {
		count += snprintf(buf + count, PAGE_SIZE,
				"Failed to get allocated llc info\n");
		return count;
	}

	count += snprintf(buf + count, PAGE_SIZE,
			"LLC Region: LLC_TOTAL\t\tAllocated way(%u)\n", way);

	return count;
}

static ssize_t llc_set_endisable_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;

	count += snprintf(buf + count, PAGE_SIZE, "LLC is %s\n",
		data->llc_enable ? "enabled" : "disabled");

	return count;
}

static ssize_t llc_set_endisable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int en;
	int ret;

	ret = sscanf(buf, "%u", &en);
	if (ret != 1) {
		SCI_ERR("%s: usage: echo [en] > llc_set_endisable\n",
				__func__);
		return -EINVAL;
	}

	ret = llc_set_enable(en, LLC_ENABLE_SYSFS);
	if (ret) {
		SCI_ERR("%s: failed to set llc en/disable (%d)\n", __func__, ret);
		return ret;
	}

	return count;
}

static ssize_t llc_mpam_alloc_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	unsigned int i, index;

	if (!data->llc_enable || !data->on_cnt) {
		count += snprintf(buf + count, PAGE_SIZE, "LLC is off\n");
		return count;
	}

	for (i = 0; i < data->mpam_nr; i++) {
		index = i;
		count += snprintf(buf + count, PAGE_SIZE,
				"LLC Mpam Data: pard_id: 0x%x, perf_mon_gr: 0x%x, ns: 0x%x, en: 0x%x\n",
				data->mpam_data[i].part_id,
				data->mpam_data[i].perf_mon_gr,
				data->mpam_data[i].ns,
				data->mpam_data[i].valen);
	}

	return count;
}

static ssize_t llc_mpam_alloc_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int index, way, pid, pmon_gr, ns, on;
	int ret;

	ret = sscanf(buf, "%u %u %u %u %u %u", &index, &way, &pid, &pmon_gr, &ns, &on);
	if (ret != 6) {
		SCI_ERR("%s: usage: echo [index] [on] [way] > llc_region_alloc\n", __func__);
		return -EINVAL;
	}

	if (index < LLC_REGION_CPU_MPAM0 || index > LLC_REGION_CPU_MPAM7) {
		SCI_ERR("%s: index should be %u <= index <= %u\n",
				__func__, LLC_REGION_CPU_MPAM0, LLC_REGION_CPU_MPAM7);
		return -EINVAL;
	}

	ret = llc_mpam_alloc(index, way, pid, pmon_gr, ns, on);
	if (ret) {
		SCI_ERR("%s: failed to allocate llc mpam (%d)\n", __func__, ret);
		return ret;
	}

	return count;
}

static ssize_t llc_quadrant_control_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	unsigned int on = 0;
	unsigned int way = 0;
	ssize_t count = 0;
	int ret;

	if (!sci_data->llc_enable) {
		count += snprintf(buf + count, PAGE_SIZE, "LLC is disabled\n");
		return count;
	}

	ret = exynos_sci_llc_quadrant_control(data, SCI_IPC_GET, &on, &way);
	if (ret) {
		count += snprintf(buf + count, PAGE_SIZE,
				"Failed to get llc way state\n");
		return count;
	}

	count += snprintf(buf + count, PAGE_SIZE,
			"LLC is %s : way status: 0x%X\n", on ? "enabled" : "disabled", way);

	return count;
}

static ssize_t llc_quadrant_control_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	unsigned int on = 0;
	unsigned int way = 0;
	int ret;

	if (!sci_data->llc_enable) {
		SCI_INFO("%s: LLC is disabled\n", __func__);
		return count;
	}

	ret = sscanf(buf, "%u %x", &on, &way);
	if (ret != 2) {
		SCI_ERR("%s: usage: echo [on] [way] > llc_quadrant_control\n",
				__func__);
		return -EINVAL;
	}

	ret = exynos_sci_llc_quadrant_control(data, SCI_IPC_SET, &on, &way);
	if (ret) {
		SCI_ERR("%s: Failed llc enable control\n", __func__);
		return ret;
	}

	return count;
}

static ssize_t llc_qpd_onoff_show(struct device *dev,
			struct device_attribute *attr, char*buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	for (i = 0; i < LLC_REGION_MAX;	i++) {
		count += snprintf(buf +	count, PAGE_SIZE, "LLC Region[%d]: %s\t\t qpd_onoff(%u)\n",
				i, data->llc_region[i].name, data->qpd_onoff[i]);
	}

	return count;
}

static ssize_t llc_qpd_onoff_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct	platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	unsigned int index = 0;
	unsigned int on = 0;
	int ret;

	ret = sscanf(buf, "%u %u", &index, &on);
	if (ret != 2) {
		SCI_ERR("%s: usage: echo [index] [on] > llc_qpd_onoff\n", __func__);
		return -EINVAL;
	}

	if (index < LLC_REGION_MAX)
		data->qpd_onoff[index] = on;
	else
		SCI_ERR("%s: invalid llc region index\n", __func__);

	return count;
}

static ssize_t llc_shared_idx_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	unsigned int i, index;
	int ret;

	for (i = 0; i < LLC_REGION_MAX; i++) {
		index = i;
		ret = exynos_sci_llc_shared_idx(data, SCI_IPC_GET, &index);
		if (ret) {
			count += snprintf(buf + count, PAGE_SIZE,
				"Failed to get allocated llc info\n");
			return count;
		}

		count += snprintf(buf + count, PAGE_SIZE,
				"LLC Region: %s\t\tshared_idx: %u \n",
				data->llc_region[i].name, index);
	}

	return count;
}

static ssize_t llc_shared_idx_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev, struct	platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	unsigned int region_index = 0;
	unsigned int shared_idx = 0;
	int ret;

	ret = sscanf(buf, "%u %u", &region_index, &shared_idx);
	if (ret != 2) {
		SCI_ERR("%s: usage: echo [region_index] [shared_idx] > llc_shared_idx\n", __func__);
		return -EINVAL;
	}

	if (region_index < LLC_REGION_MAX)
		data->llc_region[region_index].shared_idx = shared_idx;
	else
		SCI_ERR("%s: invalid llc region index\n", __func__);

	return count;
}

static ssize_t llc_shared_way_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	unsigned int i, index;
	int ret;

	for (i = 1; i < LLC_ID_MAX; i++) {
		index = i;
		ret = exynos_sci_llc_shared_way(data, SCI_IPC_GET, &index);
		if (ret) {
			count += snprintf(buf + count, PAGE_SIZE,
				"Failed to get allocated llc info\n");
			return count;
		}

		count += snprintf(buf + count, PAGE_SIZE,
				"LLC Shared Group[%u]: 0x%04X\n", i, index);
	}

	return count;
}

static ssize_t llc_get_way_max_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	ssize_t count = 0;

	count += snprintf(buf + count, PAGE_SIZE, "%d\n",
		get_llc_way_max());

	return count;
}

static DEVICE_ATTR_RW(llc_region_alloc);
static DEVICE_ATTR_RO(llc_get_region_info);
static DEVICE_ATTR_RW(llc_set_endisable);
static DEVICE_ATTR_RW(llc_mpam_alloc);
static DEVICE_ATTR_RW(llc_quadrant_control);
static DEVICE_ATTR_RW(llc_qpd_onoff);
static DEVICE_ATTR_RW(llc_shared_idx);
static DEVICE_ATTR_RO(llc_shared_way);
static DEVICE_ATTR_RO(llc_get_way_max);

static struct attribute *exynos_sci_sysfs_entries[] = {
	&dev_attr_llc_region_alloc.attr,
	&dev_attr_llc_get_region_info.attr,
	&dev_attr_llc_set_endisable.attr,
	&dev_attr_llc_mpam_alloc.attr,
	&dev_attr_llc_quadrant_control.attr,
	&dev_attr_llc_qpd_onoff.attr,
	&dev_attr_llc_shared_idx.attr,
	&dev_attr_llc_shared_way.attr,
	&dev_attr_llc_get_way_max.attr,
	NULL,
};

static struct attribute_group exynos_sci_attr_group = {
	.name	= "sci_attr",
	.attrs	= exynos_sci_sysfs_entries,
};

/* SYSFS entries for debug */
#if defined(CONFIG_EXYNOS_SCI_DBG) || defined(CONFIG_EXYNOS_SCI_DBG_MODULE)
static ssize_t dbg_mode_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;

	count += snprintf(buf + count, PAGE_SIZE, "debug mode is %s\n",
			(data->llc_debug_mode) ? "on" : "off");

	return count;
}

static ssize_t dbg_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int dbg_on;
	int ret;

	ret = kstrtou32(buf, 0, &dbg_on);
	if (ret)
		return -EINVAL;

	sci_data->llc_debug_mode = dbg_on;
	scidbg_log = dbg_on;

	return count;
}

static ssize_t dbg_llc_data_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	for (i = 0; i < data->region_size; i++)
		count += snprintf(buf + count, PAGE_SIZE,
				"[%d]%s\t\t: Priority(%2u)\tUsageCnt(%2u)\tWay(%2u)MinFreq(%2u)\n",
				i, data->llc_region[i].name, data->llc_region[i].priority,
				data->llc_region[i].usage_count,
				data->llc_region[i].way,
				data->llc_region[i].min_freq);

	return count;
}

static ssize_t dbg_log_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int dbglog;
	int ret;

	ret = kstrtou32(buf, 0, &dbglog);
	if (ret)
		return -EINVAL;

	scidbg_log = dbglog;

	return count;
}


static ssize_t dbg_llc_on_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	unsigned int on = 0;
	ssize_t count = 0;

	exynos_sci_llc_on(sci_data, SCI_IPC_GET, &on);

	count += snprintf(buf + count, PAGE_SIZE, "[IPC]LLC HW is %s\n",
			(on) ? "on" : "off");

	return count;
}

static ssize_t dbg_llc_on_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int on;
	int ret;

	ret = kstrtou32(buf, 0, &on);
	if (ret)
		return -EINVAL;

	exynos_sci_llc_on(sci_data, SCI_IPC_SET, &on);

	return count;
}

static ssize_t dbg_llc_id_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;
	u32 llc_id, llc_alloc;

	for (i = 0; i < LLC_ID_MAX; i++) {
		llc_id = __raw_readl(data->sci_base + LLCId(i));
		llc_alloc = __raw_readl(data->sci_base + LLCIdAllocLkup(i));
		count += snprintf(buf + count, PAGE_SIZE, "LLCId_%d: 0x%08X / ", i, llc_id);
		count += snprintf(buf + count, PAGE_SIZE, "LLCIdAllocLkup_%d: 0x%08X\n", i, llc_alloc);
	}

	return count;
}

static ssize_t dbg_llc_id_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	int ret;
	u32 id, llc_id, llc_alloc;

	ret = sscanf(buf, "%d %x %x", &id, &llc_id, &llc_alloc);
	if (ret != 3)
		return -EINVAL;

	if (!data->llc_debug_mode)
		return count;

	if (id >= LLC_ID_MAX)
		return -EINVAL;

	__raw_writel(llc_id, data->sci_base + LLCId(id));
	__raw_writel(llc_alloc, data->sci_base + LLCIdAllocLkup(id));

	return count;
}

static ssize_t dbg_llc_mpam_id_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;
	u32 llc_mpam_id;

	for (i = 0; i < LLC_ID_MAX; i++) {
		llc_mpam_id = __raw_readl(data->sci_base + LLCMpamId(i));
		count += snprintf(buf + count, PAGE_SIZE, "LLCMpamId_%d: 0x%x\n",
				i, llc_mpam_id);
	}

	return count;
}

static ssize_t dbg_llc_mpam_id_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	int ret;
	u32 id, llc_mpam_id;

	ret = sscanf(buf, "%d %x", &id, &llc_mpam_id);
	if (ret != 2)
		return -EINVAL;

	if (!data->llc_debug_mode)
		return count;

	if (id >= LLC_ID_MAX)
		return -EINVAL;

	__raw_writel(llc_mpam_id, data->sci_base + LLCMpamId(id));

	return count;
}

static ssize_t dbg_llc_ctrl_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
       struct platform_device *pdev = container_of(dev,
				struct platform_device, dev);
       struct exynos_sci_data *data = platform_get_drvdata(pdev);
       ssize_t count = 0;
       u32 llc_ctrl;

       llc_ctrl = __raw_readl(data->sci_base + LLCCtrl);

       count += snprintf(buf + count, PAGE_SIZE, "LLC CTRL: 0x%08X\n", llc_ctrl);

       return count;
}

static ssize_t dbg_llc_ctrl_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
       struct platform_device *pdev = container_of(dev,
				struct platform_device, dev);
       struct exynos_sci_data *data = platform_get_drvdata(pdev);
       int ret;
       u32 llc_ctrl;

       ret = sscanf(buf, "%x", &llc_ctrl);
       if (ret != 1)
		return -EINVAL;

       if (!data->llc_debug_mode)
		return count;

       __raw_writel(llc_ctrl, data->sci_base + LLCCtrl);

       return count;
}

static ssize_t dbg_llc_ppc_en_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	unsigned int on = 0;
	ssize_t count = 0;

	exynos_sci_llc_ppc_en(sci_data, SCI_IPC_GET, &on);

	count += snprintf(buf + count, PAGE_SIZE, "[IPC]LLC PPC en flag is %s\n",
			(on) ? "on" : "off");

	return count;
}

static ssize_t dbg_llc_ppc_en_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int on;
	int ret;

	ret = kstrtou32(buf, 0, &on);
	if (ret)
		return -EINVAL;

	exynos_sci_llc_ppc_en(sci_data, SCI_IPC_SET, &on);

	return count;
}
static DEVICE_ATTR_RW(dbg_mode);
static DEVICE_ATTR_WO(dbg_log);
static DEVICE_ATTR_RO(dbg_llc_data);
static DEVICE_ATTR_RW(dbg_llc_on);
static DEVICE_ATTR_RW(dbg_llc_id);
static DEVICE_ATTR_RW(dbg_llc_mpam_id);
static DEVICE_ATTR_RW(dbg_llc_ctrl);
static DEVICE_ATTR_RW(dbg_llc_ppc_en);

static struct attribute *exynos_sci_dbg_sysfs_entries[] = {
	&dev_attr_dbg_mode.attr,
	&dev_attr_dbg_log.attr,
	&dev_attr_dbg_llc_data.attr,
	&dev_attr_dbg_llc_on.attr,
	&dev_attr_dbg_llc_id.attr,
	&dev_attr_dbg_llc_mpam_id.attr,
	&dev_attr_dbg_llc_ctrl.attr,
	&dev_attr_dbg_llc_ppc_en.attr,
	NULL,
};

static struct attribute_group exynos_sci_dbg_attr_group = {
	.name	= "sci_dbg",
	.attrs	= exynos_sci_dbg_sysfs_entries,
};
#endif

static int sci_panic_handler(struct notifier_block *nb, unsigned long l, void *buf)
{
	int index;
	int i;
	if (sci_data->llc_enable) {
		llc_set_enable(DISABLE_LLC, LLC_ENABLE_PANIC);
	} else {
		SCI_INFO("%s: LLC is already Disabled\n", __func__);
	}

	SCI_INFO("------------------------------------------\n");
	SCI_INFO("Total CECC err count:	%d\n", sci_data->err_data[C_ERR].total_err_cnt);
	SCI_INFO("Total UcECC err count: %d\n", sci_data->err_data[UC_ERR].total_err_cnt);
	if (sci_data->err_data[C_ERR].total_err_cnt) {
		SCI_INFO("-----------------CErr Log-------------------------\n");
		i = sci_data->err_data[C_ERR].rptr +
			(sci_data->err_data[C_ERR].total_err_cnt >= NUM_OF_ERR_LOG ? 0 : 1);
		index = 0;
		while (1) {
			SCI_INFO("[%3d]: ktime: %llu, CErrType: 0x%X, "
					"CErrSubType: 0x%X, CErrSource:	0x%X\n", index++,
					sci_data->err_data[C_ERR].err_time[i],
					((sci_data->err_data[C_ERR].err_data[i]) >> 13) & 0xF,
					((sci_data->err_data[C_ERR].err_data[i]) >> 17) & 0x1FF,
					sci_data->err_data[C_ERR].err_data[i] & 0xFF);

			if (i == sci_data->err_data[C_ERR].wptr)
				break;

			i++;
			if (i == NUM_OF_ERR_LOG)
				i = 0;
		}
	}

	if (sci_data->err_data[UC_ERR].total_err_cnt) {
		SCI_INFO("------------------UcErr Log------------------------\n");
		i = sci_data->err_data[UC_ERR].rptr +
			(sci_data->err_data[UC_ERR].total_err_cnt >= NUM_OF_ERR_LOG ? 0 : 1);
		index = 0;
		while (1) {
			SCI_INFO("[%3d]: ktime: %llu, UcErrType: 0x%X, "
				"UcErrSubType: 0x%X, UcErrSource: 0x%X\n", index++,
				sci_data->err_data[UC_ERR].err_time[i],
				((sci_data->err_data[UC_ERR].err_data[i]) >> 13) & 0xF,
				((sci_data->err_data[UC_ERR].err_data[i]) >> 17) & 0x1FF,
				sci_data->err_data[UC_ERR].err_data[i] & 0xFF);

			if (i == sci_data->err_data[UC_ERR].wptr)
				break;

			i++;
			if (i == NUM_OF_ERR_LOG)
				i = 0;
		}

	}

	return NOTIFY_DONE;
}

static struct notifier_block nb_sci_panic = {
	.notifier_call = sci_panic_handler,
	.priority = INT_MAX,
};

static irqreturn_t exynos_sci_irq_handler(int irq, void *p)
{
	struct exynos_sci_data *data = p;
	unsigned int source, miscinfo;
	unsigned int addrlow, addrhigh;
	unsigned int errtype, errsubtype;

	if (!data->sci_base)
		return IRQ_HANDLED;

	/* Print Correted Error
	 * SCI_CErrMiscInfo		0x918
	 * SCI_CErrSource		0x914
	 * SCI_CErrAddrLow		0x91C
	 * SCI_CErrAddrHigh		0x920
	 * SCI_CErrOverrunMiscInfo	0x924
	 */
	source = __raw_readl(data->sci_base + SCI_CErrSource);
	miscinfo = __raw_readl(data->sci_base + SCI_CErrMiscInfo);
	addrlow = __raw_readl(data->sci_base + SCI_CErrAddrLow);
	addrhigh = __raw_readl(data->sci_base + SCI_CErrAddrHigh);

	errtype = (miscinfo >> 13) & 0xF;
	errsubtype = (miscinfo >> 17) & 0x1FF;

	if (source) {
		data->err_data[C_ERR].total_err_cnt++;

		data->err_data[C_ERR].wptr++;
		if (data->err_data[C_ERR].wptr == NUM_OF_ERR_LOG) {
			data->err_data[C_ERR].wptr = 0;
		}
		data->err_data[C_ERR].err_data[data->err_data[C_ERR].wptr]
			= errsubtype << 17 | errtype << 13 | source;
		data->err_data[C_ERR].err_time[data->err_data[C_ERR].wptr] = ktime_get();

		if (data->err_data[C_ERR].wptr == data->err_data[C_ERR].rptr) {
			data->err_data[C_ERR].rptr++;
			if (data->err_data[C_ERR].rptr == NUM_OF_ERR_LOG)
				data->err_data[C_ERR].rptr = 0;
		}
	}

	SCI_INFO("------------------------------------------\n");
	SCI_INFO("Total Corr Count:	%08d\n", data->err_data[C_ERR].total_err_cnt);
	SCI_INFO("CorrErrSource:	0x%08X\n", source);
	SCI_INFO("Addr	:	%s\n", ((addrhigh >> 24) & 0x1) ? ("valid") : ("invalid"));
	SCI_INFO("CorrErrAddr	:	0x%08X 0x%08X\n", addrhigh, addrlow);
	SCI_INFO("CorrErrMiscInfo:	0x%08X\n", miscinfo);
	SCI_INFO("ErrType	:	0x%01X\n", errtype);
	SCI_INFO("ErrSubType	:	0x%03X\n", errsubtype);

	if ((miscinfo >> 12) & 0x1) {
		SCI_INFO("SCI/LLC Syndrome is valid\n");
		SCI_INFO("Syndrome: 0x%03X\n", miscinfo & 0xFFF);
	}

	SCI_INFO("CorrErrOverrun: 0x%08X\n",
			__raw_readl(data->sci_base + SCI_CErrOverrunMiscInfo));

	/* Print Uncorrectable Error
	 * SCI_UcErrMiscInfo		0x944
	 * SCI_UcErrSource		0x940
	 * SCI_UcErrAddrLow		0x948
	 * SCI_UcErrAddrHigh		0x94C
	 * SCI_UcErrOverrunMiscInfo	0x950
	 */
	source = __raw_readl(data->sci_base + SCI_UcErrSource);
	miscinfo = __raw_readl(data->sci_base +	SCI_UcErrMiscInfo);
	addrlow = __raw_readl(data->sci_base + SCI_UcErrAddrLow);
	addrhigh = __raw_readl(data->sci_base + SCI_UcErrAddrHigh);

	errtype = (miscinfo >> 13) & 0xF;
	errsubtype = (miscinfo >> 17) & 0x1FF;

	if (source) {
		data->err_data[UC_ERR].total_err_cnt++;

		data->err_data[UC_ERR].wptr++;
		if (data->err_data[UC_ERR].wptr == NUM_OF_ERR_LOG) {
			data->err_data[UC_ERR].wptr = 0;
		}
		data->err_data[UC_ERR].err_data[data->err_data[UC_ERR].wptr]
			= errsubtype << 17 | errtype << 13 | source;
		data->err_data[UC_ERR].err_time[data->err_data[UC_ERR].wptr] = ktime_get();

		if (data->err_data[UC_ERR].wptr == data->err_data[UC_ERR].rptr) {
			data->err_data[UC_ERR].rptr++;
			if (data->err_data[UC_ERR].rptr == NUM_OF_ERR_LOG)
				data->err_data[UC_ERR].rptr = 0;
		}
	}

	SCI_INFO("------------------------------------------\n");
	SCI_INFO("Total UCErr Count:	%08d\n", data->err_data[UC_ERR].total_err_cnt);
	SCI_INFO("UcErrSource:	0x%08X\n", source);
	SCI_INFO("Addr	:	%s\n", ((addrhigh >> 24) & 0x1) ? ("valid") : ("invalid"));
	SCI_INFO("UcErrAddr	:	0x%08X 0x%08X\n", addrhigh, addrlow);
	SCI_INFO("UcErrMiscInfo	:	0x%08X\n", miscinfo);
	SCI_INFO("ErrType	:	0x%01X\n", errtype);
	SCI_INFO("ErrSubType	:	0x%03X\n", errsubtype);

	if ((miscinfo >> 12) & 0x1) {
		SCI_INFO("SCI/LLC Syndrome is valid\n");
		SCI_INFO("Syndrome: 0x%03X\n", miscinfo	& 0xFFF);
	}

	SCI_INFO("UcErrOverrun : 0x%08X\n",
			__raw_readl(data->sci_base + SCI_UcErrOverrunMiscInfo));
	SCI_INFO("------------------------------------------\n");

	/* panic only when LLC uncorrected error occurred */
	if (source != 0x0) {
		pr_err("SCI uncorrectable error (irqnum: %d)\n", irq);
		disable_irq_nosync(irq);
		dbg_snapshot_expire_watchdog();
	} else {
		source = __raw_readl(data->sci_base + 0x928);
		source |= ((0x1 << 10) | (0x1 << 9));
		__raw_writel(source, data->sci_base + 0x928);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static int exynos_sci_parse_base_config_dt(struct device_node *np,
				struct exynos_sci_data *data)
{
	const char *tmp;
	int ret = 0;
	int size, prop;
	int i;

	if (!np)
		return -ENODEV;

	ret = of_property_read_u32(np, "sci_base", &prop);
	if (ret) {
		dev_err(data->dev, "Failed to get SCI base address\n");
		return ret;
	}

	ret = of_property_read_u32(np, "nr_irq", &data->irqcnt);
	if (ret) {
		dev_err(data->dev, "Failed to get irqcnt value!\n");
		return ret;
	}

	data->sci_base = ioremap(prop, SZ_4K);
	if (IS_ERR(data->sci_base)) {
		SCI_ERR("%s: Failed to map SCI base address\n", __func__);
		ret = -EINVAL;
		goto err_ioremap;
	}

	tmp = of_get_property(np, "use-llcgov", NULL);
	if (tmp && !strcmp(tmp, "enabled"))
		data->llc_gov_en = true;
	else
		data->llc_gov_en = false;

	for (i = 0; i < LLC_ENABLE_SOURCE_MAX; i++)
		data->llc_enable_status[i] = ENABLE_LLC;

	tmp = of_get_property(np, "use-llc", NULL);
	if (tmp && !strcmp(tmp, "enabled")) {
		data->llc_enable = ENABLE_LLC;
		data->llc_enable_status[LLC_ENABLE_DEFAULT] = ENABLE_LLC;
	} else {
		data->llc_enable = DISABLE_LLC;
		data->llc_enable_status[LLC_ENABLE_DEFAULT] = DISABLE_LLC;
	}

	tmp = of_get_property(np, "use-llc-retention", NULL);
	if (tmp && !strcmp(tmp, "enabled"))
		data->llc_retention_mode = true;
	else
		data->llc_retention_mode = false;


	ret = of_property_read_u32(np, "disable-threshold", &prop);
	if (ret) {
		dev_err(data->dev, "Failed to get disable threshold\n");
		return ret;
	}
	data->llc_disable_threshold = prop;

	size = of_property_count_strings(np, "region_name");
	if (size < 0) {
		SCI_ERR("%s: Failed to get numbers of regions\n", __func__);
		ret = size;
		goto err_size;
	}
	data->region_size = size;

	ret = of_property_read_u32(np, "mpam-nr", &prop);
	if (ret) {
		dev_err(data->dev, "Failed to get mpam-nr\n");
		return ret;
	}
	data->mpam_nr = prop;

	if (!of_property_read_u32(np, "llc_way_max", &prop))
		llc_way_max = prop;

#if !(defined (CONFIG_EXYNOS_ESCAV1) || defined(CONFIG_EXYNOS_ESCAV1_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA))
	size = of_property_count_u32_elems(np, "vch_pd_calid");
	if (size < 0) {
		SCI_ERR("%s: Failed to get number of CAL IDs for virtual channel settings\n", __func__);
		ret = size;
		goto err_size;
	}
	data->vch_size = size;

	data->vch_pd_calid = kzalloc(sizeof(int) * data->vch_size,
					GFP_KERNEL);
	for (i = 0; i < data->vch_size; i++) {
		ret = of_property_read_u32_index(np, "vch_pd_calid",
			i, &(data->vch_pd_calid[i]));
		if (ret) {
			SCI_ERR("%s: Failed to get vch_pd_calid(index: %d)\n",
				__func__, i);
			goto err_vch;
		}
	}

	if (data->vch_size)
		exynos_cal_pd_sci_sync = sci_pd_sync;
#endif

	size = of_property_count_u32_elems(np, "qpd_onoff");
	if (size < 0) {
		SCI_INFO("%s: Failed get number of qpd_onoff\n", __func__);
	} else {
		for (i = 0; i < size; i++) {
			ret = of_property_read_u32_index(np, "qpd_onoff", i, &(data->qpd_onoff[i]));
			if (ret) {
				SCI_ERR("%s: Failed get qpd_onoff(index:%d)\n",	__func__, i);
				return ret;
			}
		}
	}

	return ret;

#if !(defined (CONFIG_EXYNOS_ESCAV1) || defined(CONFIG_EXYNOS_ESCAV1_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA))
err_vch:
	kfree(data->vch_pd_calid);
#endif
err_size:
	iounmap(data->sci_base);
err_ioremap:
	return ret;
}

static int exynos_sci_parse_details(struct device_node *np,
				struct exynos_sci_data *data)
{
	int ret = 0;
	int i;
	const char *name;

	if (!np)
		return -ENODEV;

	for (i = 0; i < data->region_size; i++) {
		ret = of_property_read_string_index(np, "region_name", i, &name);
		if (ret < 0) {
			SCI_ERR("%s: Failed to get region_name\n", __func__);
			goto err_size;
		}

		data->llc_region[i].name = name;
		data->llc_region[i].priority = i;
/*
		ret = of_property_read_u32_index(np, "quadrant_pd",
					i, &(data->llc_region[i].use_qpd));
		if (ret) {
			SCI_ERR("%s: Failed to get quadrant power control\n", __func__);
			data->llc_region[i].use_qpd = 0;
		}
*/
		data->llc_region[i].usage_count = 0;
		data->llc_region[i].way = 0;
		data->llc_region[i].min_freq = 0;
		data->llc_region[i].shared_idx = 0; // This means single allocation
	}

err_size:
	return ret;
}
#else
static inline int exynos_sci_parse_base_config_dt(struct device_node *np,
				struct exynos_sci_data *data)
{
	return -ENODEV;
}

static inline int exynos_sci_parse_details(struct device_node *np,
				struct exynos_sci_data *data)
{
	return -ENODEV;
}
#endif

static int exynos_sci_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i;
	unsigned int irqnum = 0;
	struct exynos_sci_data *data;

	data = kzalloc(sizeof(struct exynos_sci_data), GFP_KERNEL);
	if (data == NULL) {
		SCI_ERR("%s: failed to allocate SCI device\n", __func__);
		ret = -ENOMEM;
		goto err_data;
	}

	sci_data = data;
	data->dev = &pdev->dev;

	spin_lock_init(&sci_data->lock);

#if defined(CONFIG_EXYNOS_ACPM) || defined(CONFIG_EXYNOS_ACPM_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA)
	/* acpm_ipc_request_channel */
	ret = esca_ipc_request_channel(data->dev->of_node, NULL,
				&data->ipc_ch_num, &data->ipc_ch_size);

	if (ret) {
		SCI_ERR("%s: acpm request channel is failed, ipc_ch: %u, size: %u\n",
				__func__, data->ipc_ch_num, data->ipc_ch_size);
		goto err_acpm;
	}
#endif

	/* parsing base data for SCI */
	ret = exynos_sci_parse_base_config_dt(data->dev->of_node, data);
	if (ret) {
		SCI_ERR("%s: failed to parse private data\n", __func__);
		goto err_parse_dt;
	}

	data->llc_region = kmalloc_array(data->region_size, sizeof(struct exynos_sci_llc_region), GFP_ATOMIC);
	if (data->llc_region == NULL) {
		SCI_ERR("%s: failed to allocate llc region array\n", __func__);
		goto err_alloc_region;
	}

	data->mpam_data = kmalloc_array(data->mpam_nr, sizeof(struct exynos_sci_mpam_data), GFP_ATOMIC);
	if (data->mpam_data == NULL) {
		SCI_ERR("%s: failed to allocate mpam data array\n", __func__);
		goto err_alloc_region;
	}

	/* parsing details for SCI */
	ret = exynos_sci_parse_details(data->dev->of_node, data);
	if (ret) {
		SCI_ERR("%s: failed to parse private data\n", __func__);
		goto err_parse_details;
	}

	data->on_cnt = 0;
	data->llc_debug_mode = false;

	ret = sci_register_llc_governor(sci_data);
	if (ret) {
		SCI_ERR("%s: failed to register SCI LLC governor\n", __func__);
		goto err_register_gov;
	}

	atomic_notifier_chain_register(&panic_notifier_list, &nb_sci_panic);
	platform_set_drvdata(pdev, data);

	ret = sysfs_create_group(&data->dev->kobj, &exynos_sci_attr_group);
	if (ret)
		SCI_ERR("%s: failed creat sysfs for Exynos SCI\n", __func__);
#if defined(CONFIG_EXYNOS_SCI_DBG) || defined(CONFIG_EXYNOS_SCI_DBG_MODULE)
	ret = sysfs_create_group(&data->dev->kobj, &exynos_sci_dbg_attr_group);
	if (ret)
		SCI_ERR("%s: failed creat sysfs for Exynos SCI\n", __func__);
#endif
	/* Register Interrupt for LLC Uncorrected error */
	for (i = 0; i < data->irqcnt; i++) {
		irqnum = irq_of_parse_and_map(data->dev->of_node, i);
		if (!irqnum) {
			dev_err(data->dev, "Failed to get IRQ map\n");
			return -EINVAL;
		}

		ret = devm_request_irq(data->dev, irqnum,
				exynos_sci_irq_handler,
				IRQF_SHARED, dev_name(data->dev), data);
		if (ret)
			return ret;
	}

	print_sci_data(data);

	SCI_INFO("%s: EXYNOS SCI is initialized\n", __func__);

	return 0;

err_register_gov:
err_parse_details:
#if !(defined (CONFIG_EXYNOS_ESCAV1) || defined(CONFIG_EXYNOS_ESCAV1_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA))
	data->vch_pd_calid = NULL;
#endif
	data->llc_region = NULL;
	kfree(data->llc_region);
err_alloc_region:
err_parse_dt:
#if defined(CONFIG_EXYNOS_ACPM) || defined(CONFIG_EXYNOS_ACPM_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA)
	esca_ipc_release_channel(data->dev->of_node, data->ipc_ch_num);
err_acpm:
#endif
	kfree(data);
err_data:
	return ret;
}

static int exynos_sci_remove(struct platform_device *pdev)
{

	struct exynos_sci_data *data = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	iounmap(data->sci_base);
#if defined(CONFIG_EXYNOS_ACPM) || defined(CONFIG_EXYNOS_ACPM_MODULE) || IS_ENABLED(CONFIG_EXYNOS_ESCA)
	esca_ipc_release_channel(data->dev->of_node, data->ipc_ch_num);
#endif
	kfree(data);

	SCI_INFO("%s: exynos sci is removed!!\n", __func__);


	return 0;
}

static int exynos_sci_pm_suspend(struct device *dev)
{
	if (sci_data->llc_enable) {
		/* llc off before suspend */
		llc_set_enable(DISABLE_LLC, LLC_ENABLE_STR);
		SCI_INFO("%s: llc off success\n", __func__);
	}

	return 0;
}

static int exynos_sci_pm_resume(struct device *dev)
{
	llc_set_enable(ENABLE_LLC, LLC_ENABLE_STR);
	return 0;
}

static struct dev_pm_ops exynos_sci_pm_ops = {
	.suspend	= exynos_sci_pm_suspend,
	.resume		= exynos_sci_pm_resume,
};

static struct platform_device_id exynos_sci_driver_ids[] = {
	{ .name = EXYNOS_SCI_MODULE_NAME, },
	{},
};
MODULE_DEVICE_TABLE(platform, exynos_sci_driver_ids);

static const struct of_device_id exynos_sci_match[] = {
	{ .compatible = "samsung,exynos-sci", },
	{},
};

static struct platform_driver exynos_sci_driver = {
	.id_table = exynos_sci_driver_ids,
	.driver = {
		.name = EXYNOS_SCI_MODULE_NAME,
		.owner = THIS_MODULE,
		.pm = &exynos_sci_pm_ops,
		.of_match_table = exynos_sci_match,
	},
	.probe = exynos_sci_probe,
	.remove = exynos_sci_remove,
};

static int exynos_sci_init(void)
{
	int ret;

	ret = platform_driver_register(&exynos_sci_driver);
	if (ret) {
		SCI_INFO("Error registering platform driver\n");
		return ret;
	}

	return ret;
}
late_initcall(exynos_sci_init);

MODULE_DESCRIPTION("Samsung SCI Interface driver");
MODULE_LICENSE("GPL");
