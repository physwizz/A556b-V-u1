// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ALSA SoC - Samsung Abox WDMA driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/iommu.h>
#include <linux/delay.h>
#include <linux/memblock.h>
#include <linux/sched/clock.h>
#include <sound/hwdep.h>
#include <linux/miscdevice.h>
#include <linux/dma-buf.h>
#include <linux/compat.h>

#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <soc/samsung/exynos/debug-snapshot.h>

#include <sound/samsung/abox.h>
#include "abox_util.h"
#include "abox_gic.h"
#include "abox_dbg.h"
#include "abox_vss.h"
#include "abox_atune.h"
#include "abox_cmpnt.h"
#include "abox.h"
#include "abox_dma.h"
#include "abox_memlog.h"

static int abox_wdma_request_ipc(struct abox_dma_data *data,
		ABOX_IPC_MSG *msg, int atomic, int sync)
{
	return abox_request_ipc(data->dev_abox, msg->ipcid, msg, sizeof(*msg),
			atomic, sync);
}

static const struct snd_pcm_hardware abox_wdma_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED
				| SNDRV_PCM_INFO_BLOCK_TRANSFER
				| SNDRV_PCM_INFO_MMAP
				| SNDRV_PCM_INFO_MMAP_VALID,
	.formats		= ABOX_SAMPLE_FORMATS,
	.channels_min		= 1,
	.channels_max		= 8,
	.buffer_bytes_max	= BUFFER_BYTES_MAX,
	.period_bytes_min	= PERIOD_BYTES_MIN,
	.period_bytes_max	= PERIOD_BYTES_MAX,
	.periods_min		= BUFFER_BYTES_MAX / PERIOD_BYTES_MAX,
	.periods_max		= BUFFER_BYTES_MAX / PERIOD_BYTES_MIN,
};

static irqreturn_t abox_wdma_fade_done(int irq, void *dev_id)
{
	struct device *dev = dev_id;
	struct abox_dma_data *data = dev_get_drvdata(dev);

	abox_info(dev, "%s(%d)\n", __func__, irq);

	complete(&data->func_changed);

	return IRQ_HANDLED;
}

static irqreturn_t abox_wdma_ipc_handler(int ipc, void *dev_id,
		ABOX_IPC_MSG *msg)
{
	struct abox_data *abox_data = dev_id;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	int id = pcmtask_msg->channel_id;
	struct abox_dma_data *data;
	struct device *dev;

	if (id >= ARRAY_SIZE(abox_data->dev_wdma) || !abox_data->dev_wdma[id])
		return IRQ_NONE;

	dev = abox_data->dev_wdma[id];
	data = dev_get_drvdata(dev);

	abox_dbg(dev, "%s(%d)\n", __func__, pcmtask_msg->msgtype);

	switch (pcmtask_msg->msgtype) {
	case PCM_PLTDAI_POINTER:
		if (data->backend) {
			dev_warn_ratelimited(dev, "pointer ipc to backend\n");
			break;
		}

		data->pointer = pcmtask_msg->param.pointer;
		snd_pcm_period_elapsed(data->substream);
		break;
	case PCM_PLTDAI_ACK:
		data->ack_enabled = !!pcmtask_msg->param.trigger;
		break;
	case PCM_PLTDAI_CLOSED:
		complete(&data->closed);
		break;
	case PCM_TX_BARGE_IN_DETECT:
		schedule_work(&abox_data->notify_bargein_detect_work);
		break;
	default:
		abox_warn(dev, "unknown message: %d\n", pcmtask_msg->msgtype);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static int abox_wdma_enabled(struct abox_dma_data *data)
{
	unsigned int val = 0;

	regmap_read(data->abox_data->regmap, ABOX_WDMA_CTRL(data->id), &val);

	return !!(val & ABOX_WDMA_ENABLE_MASK);
}

static int abox_wdma_progress(struct abox_dma_data *data)
{
	unsigned int val = 0;

	regmap_read(data->abox_data->regmap, ABOX_WDMA_STATUS(data->id), &val);

	return !!(val & ABOX_WDMA_PROGRESS_MASK);
}

static void abox_wdma_disable_barrier(struct device *dev,
		struct abox_dma_data *data)
{
	struct abox_data *abox_data = data->abox_data;
	u64 timeout = local_clock() + abox_get_waiting_ns(true);

	while (abox_wdma_progress(data) || abox_wdma_enabled(data)) {
		if (local_clock() <= timeout) {
			cond_resched();
			continue;
		}
		dev_warn_ratelimited(dev, "WDMA disable timeout\n");
		abox_dbg_dump_mem(dev, abox_data, ABOX_DBG_DUMP_FIRMWARE, "WDMA disable timeout");

		/* Disable DMA by force */
		regmap_update_bits_base(abox_data->regmap,
				ABOX_WDMA_CTRL(data->id),
				ABOX_WDMA_ENABLE_MASK, 0, NULL, false, true);
		break;
	}
}

static int abox_wdma_backend(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);

	return (asoc_rtd_to_cpu(rtd, 0)->id >= ABOX_WDMA0_BE);
}

static int abox_wdma_hw_params(struct snd_soc_component *component,
		struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;
	struct abox_data *abox_data = data->abox_data;
	struct device *dev_abox = abox_data->dev;
	struct snd_dma_buffer *dmab;
	int id = data->id;
	size_t buffer_bytes = params_buffer_bytes(params);
	unsigned int iova;
	int burst_len, ret;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	if (abox_wdma_backend(substream) && !abox_dma_can_params(rtd, substream->stream)) {
		abox_info(dev, "%s skip\n", __func__);
		return 0;
	}
	abox_dbg(dev, "%s\n", __func__);

	data->hw_params = *params;

	switch (data->buf_type) {
	case BUFFER_TYPE_RAM:
		if (data->ramb.bytes >= buffer_bytes) {
			iova = data->ramb.addr;
			dmab = &data->ramb;
			break;
		}
		/* fallback to DMA mode */
		abox_info(dev, "fallback to dma buffer\n");
		fallthrough;
	case BUFFER_TYPE_DMA:
		if (data->dmab.bytes < buffer_bytes) {
			abox_iommu_unmap(dev_abox, IOVA_WDMA_BUFFER(id));
			snd_dma_free_pages(&data->dmab);
			ret = snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV,
					dev, PAGE_ALIGN(buffer_bytes),
					&data->dmab);
			if (ret < 0)
				return ret;
			ret = abox_iommu_map(dev_abox, IOVA_WDMA_BUFFER(id),
					data->dmab.addr, data->dmab.bytes,
					data->dmab.area);
			if (ret < 0)
				return ret;
			abox_info(dev, "dma buffer changed\n");
		}
		iova = IOVA_WDMA_BUFFER(id);
		dmab = &data->dmab;
		break;
	case BUFFER_TYPE_ION:
		if (data->dmab.bytes < buffer_bytes)
			return -ENOMEM;

		abox_info(dev, "ion_buffer %s bytes(%zu) size(%zu)\n",
				__func__, buffer_bytes, data->ion_buf->size);
		iova = IOVA_WDMA_BUFFER(id);
		dmab = &data->dmab;
		break;
	default:
		abox_err(dev, "buf_type is not defined\n");
		break;
	}

	if (!abox_wdma_backend(substream)) {
		snd_pcm_set_runtime_buffer(substream, dmab);
		runtime->dma_bytes = buffer_bytes;
		abox_dma_acquire_irq(data, DMA_IRQ_FADE_DONE);
	} else {
		abox_dbg(dev, "backend dai mode\n");
	}
	data->backend = abox_wdma_backend(substream);

	if (abox_dma_is_sync_mode(data))
		burst_len = 0x1;
	else
		burst_len = 0x4;
	ret = snd_soc_component_update_bits(data->cmpnt,
			DMA_REG_CTRL0, ABOX_DMA_BURST_LEN_MASK,
			burst_len << ABOX_DMA_BURST_LEN_L);
	if (ret < 0)
		abox_err(dev, "burst length write error: %d\n", ret);

	pcmtask_msg->channel_id = id;
	msg.ipcid = IPC_PCMCAPTURE;
	msg.task_id = pcmtask_msg->channel_id = id;

	pcmtask_msg->msgtype = PCM_SET_BUFFER;
	pcmtask_msg->param.setbuff.addr = iova;
	pcmtask_msg->param.setbuff.size = params_period_bytes(params);
	pcmtask_msg->param.setbuff.count = params_periods(params);
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);
	if (ret < 0)
		return ret;

	pcmtask_msg->msgtype = PCM_PLTDAI_HW_PARAMS;
	pcmtask_msg->param.hw_params.sample_rate = params_rate(params);
	pcmtask_msg->param.hw_params.bit_depth = params_width(params);
	pcmtask_msg->param.hw_params.channels = params_channels(params);
	if (params_format(params) == SNDRV_PCM_FORMAT_S24_3LE)
		pcmtask_msg->param.hw_params.packed = 1;
	else
		pcmtask_msg->param.hw_params.packed = 0;
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);
	if (ret < 0)
		return ret;

	if (params_rate(params) > 48000)
		abox_request_cpu_gear_dai(dev, abox_data, cpu_dai,
				abox_data->cpu_gear_min - 1);

	abox_info(dev, "%s:Total=%u PrdSz=%u(%u) #Prds=%u rate=%u, width=%d, channels=%u\n",
			snd_pcm_stream_str(substream),
			params_buffer_bytes(params), params_period_size(params),
			params_period_bytes(params), params_periods(params),
			params_rate(params), params_width(params),
			params_channels(params));

	return 0;
}

static int abox_wdma_hw_free(struct snd_soc_component *component,
		struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;
	int id = data->id;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	if (abox_wdma_backend(substream) && !abox_dma_can_free(rtd, substream->stream)) {
		abox_dbg(dev, "%s skip\n", __func__);
		return 0;
	}
	abox_dbg(dev, "%s\n", __func__);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_HW_FREE;
	msg.task_id = pcmtask_msg->channel_id = id;
	abox_wdma_request_ipc(data, &msg, 0, 0);

	if (!abox_wdma_backend(substream)) {
		abox_dma_release_irq(data, DMA_IRQ_FADE_DONE);
		snd_pcm_set_runtime_buffer(substream, NULL);
	}

	return 0;
}

static int abox_wdma_prepare(struct snd_soc_component *component,
		struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;
	int id = data->id;
	int ret;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	if (abox_wdma_backend(substream) && !abox_dma_can_prepare(rtd, substream->stream)) {
		abox_dbg(dev, "%s skip\n", __func__);
		return 0;
	}
	abox_dbg(dev, "%s\n", __func__);

	data->pointer = IOVA_WDMA_BUFFER(id);

	/* set auto fade in before dma enable */
	snd_soc_component_update_bits(data->cmpnt, DMA_REG_CTRL,
			ABOX_DMA_AUTO_FADE_IN_MASK,
			data->auto_fade_in ? ABOX_DMA_AUTO_FADE_IN_MASK : 0);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_PREPARE;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);

	return ret;
}

static int abox_wdma_trigger_ipc(struct abox_dma_data *data, bool atomic,
		bool start)
{
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	if (data->enabled == start)
		return 0;

	data->enabled = start;

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_TRIGGER;
	msg.task_id = pcmtask_msg->channel_id = data->id;
	pcmtask_msg->param.trigger = start;
	return abox_wdma_request_ipc(data, &msg, atomic, 0);
}

static int abox_wdma_trigger(struct snd_soc_component *component,
		struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;
	int ret;

	/* use mute_stream callback instead, if the stream is backend */
	if (abox_wdma_backend(substream))
		return 0;

	abox_info(dev, "%s(%d)\n", __func__, cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = abox_wdma_trigger_ipc(data, true, true);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		ret = abox_wdma_trigger_ipc(data, true, false);
		if (!completion_done(&data->func_changed))
			complete(&data->func_changed);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static snd_pcm_uframes_t abox_wdma_pointer(struct snd_soc_component *component,
		struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;
	int id = data->id;
	ssize_t pointer;

	if (data->pointer >= IOVA_WDMA_BUFFER(id)) {
		pointer = data->pointer - IOVA_WDMA_BUFFER(id);
	} else {
		switch (data->type) {
		case PLATFORM_NORMAL:
		case PLATFORM_SYNC:
			pointer = abox_wdma_progress(data) ? abox_dma_read_pointer(data) : 0;
			break;
		default:
			pointer = 0;
			break;
		}
	}

	abox_dbg(dev, "%s: pointer=%#zx\n", __func__, pointer);

	return bytes_to_frames(runtime, pointer);
}

static int abox_wdma_open(struct snd_soc_component *component,
		struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;
	struct abox_data *abox_data = data->abox_data;
	int id = data->id;
	int ret;
	long time;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	if (abox_wdma_backend(substream) && !abox_dma_can_open(rtd, substream->stream)) {
		abox_info(dev, "%s skip\n", __func__);
		return 0;
	}
	abox_info(dev, "%s\n", __func__);

	abox_wait_restored(abox_data);

	if (data->closing) {
		data->closing = false;
		/* complete close before new open */
		time = wait_for_completion_timeout(&data->closed,
				abox_get_waiting_jiffies(true));
		if (time == 0)
			abox_warn(dev, "close timeout\n");
	}

	if (data->type == PLATFORM_CALL) {
		if (abox_cpu_gear_idle(dev, ABOX_CPU_GEAR_CALL_VSS))
			abox_request_cpu_gear_sync(dev, abox_data,
					ABOX_CPU_GEAR_CALL_KERNEL,
					ABOX_CPU_GEAR_MAX, cpu_dai->name);
		ret = abox_vss_notify_call(dev, abox_data, 1);
		if (ret < 0)
			abox_warn(dev, "call notify failed: %d\n", ret);
	}
	abox_request_cpu_gear_dai(dev, abox_data, cpu_dai,
			abox_data->cpu_gear_min);

	if (substream->runtime)
		snd_soc_set_runtime_hwparams(substream, &abox_wdma_hardware);

	data->substream = substream;

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_OPEN;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);

	return ret;
}

static int abox_wdma_close(struct snd_soc_component *component,
		struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;
	struct abox_data *abox_data = data->abox_data;
	int id = data->id;
	int ret;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;

	if (abox_wdma_backend(substream) && !abox_dma_can_close(rtd, substream->stream)) {
		abox_info(dev, "%s skip\n", __func__);
		return 0;
	}
	abox_info(dev, "%s\n", __func__);

	abox_wdma_disable_barrier(dev, data);

	data->substream = NULL;
	if (!abox_data->failsafe)
		data->closing = true;

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_CLOSE;
	msg.task_id = pcmtask_msg->channel_id = id;
	ret = abox_wdma_request_ipc(data, &msg, 0, 0);

	abox_request_cpu_gear_dai(dev, abox_data, cpu_dai, 0);
	if (data->type == PLATFORM_CALL) {
		abox_request_cpu_gear(dev, abox_data, ABOX_CPU_GEAR_CALL_KERNEL,
				0, cpu_dai->name);
		ret = abox_vss_notify_call(dev, abox_data, 0);
		if (ret < 0)
			abox_warn(dev, "call notify failed: %d\n", ret);
	}

	/* Release ASRC to reuse it in other DMA */
	abox_cmpnt_asrc_release(abox_data, SNDRV_PCM_STREAM_CAPTURE, id);

	return ret;
}

static int abox_wdma_mmap(struct snd_soc_component *component,
		struct snd_pcm_substream *substream,
		struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;

	abox_info(dev, "%s\n", __func__);

	if (data->buf_type == BUFFER_TYPE_ION)
		return dma_buf_mmap(data->ion_buf->dma_buf, vma, 0);
	else
		return dma_mmap_wc(dev, vma,
				runtime->dma_area,
				runtime->dma_addr,
				runtime->dma_bytes);
}

static int abox_wdma_copy_user(struct snd_soc_component *component,
		struct snd_pcm_substream *substream, int channel,
		unsigned long pos, struct iov_iter *iter, unsigned long bytes)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(cpu_dai);
	struct device *dev = data->dev;
	int id = data->id;
	ABOX_IPC_MSG msg;
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg.msg.pcmtask;
	unsigned long appl_bytes = (pos + bytes) % runtime->dma_bytes;
	unsigned long start;
	unsigned long copy_bytes;
	void *ptr;
	int ret = 0;

	start = pos + channel * (runtime->dma_bytes / runtime->channels);

	ptr = runtime->dma_area + start;
	copy_bytes = copy_to_iter(ptr, bytes, iter);
	if (copy_bytes < 0) {
		abox_err(dev, "%s: %ld -> %ld\n", __func__, bytes, copy_bytes);
		ret = -EFAULT;
	}

	if (!data->ack_enabled)
		return ret;

	abox_dbg(dev, "%s: %ld\n", __func__, appl_bytes);

	msg.ipcid = IPC_PCMCAPTURE;
	pcmtask_msg->msgtype = PCM_PLTDAI_ACK;
	pcmtask_msg->param.pointer = (unsigned int)appl_bytes;
	msg.task_id = pcmtask_msg->channel_id = id;

	return abox_wdma_request_ipc(data, &msg, 1, 0);
}

static int abox_wdma_pcm_new(struct snd_soc_component *component,
		struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(dai);
	struct device *dev = data->dev;
	struct device *dev_abox = data->abox_data->dev;
	int id = data->id;
	size_t buffer_bytes = data->dmab.bytes;
	int ret;

	switch (data->buf_type) {
	case BUFFER_TYPE_ION:
		buffer_bytes = BUFFER_ION_BYTES_MAX;
		data->ion_buf = abox_ion_alloc(dev, data->abox_data,
				IOVA_WDMA_BUFFER(id), buffer_bytes, false);
		if (!IS_ERR(data->ion_buf)) {
			/* update buffer infomation using ion allocated buffer  */
			data->dmab.area = data->ion_buf->kva;
			data->dmab.addr = data->ion_buf->iova;
			data->dmab.bytes  = data->ion_buf->size;

			ret = abox_ion_new_hwdep(rtd, data->ion_buf, &data->hwdep);
			if (ret < 0) {
				abox_err(dev, "failed to add hwdep: %d\n", ret);
				return ret;
			}
			break;
		}
		abox_warn(dev, "fallback to dma alloc\n");
		data->buf_type = BUFFER_TYPE_DMA;
		fallthrough;
	case BUFFER_TYPE_DMA:
		if (buffer_bytes < BUFFER_BYTES_MIN)
			buffer_bytes = BUFFER_BYTES_MIN;

		ret = snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV, dev,
				buffer_bytes, &data->dmab);
		if (ret < 0)
			return ret;

		ret = abox_iommu_map(dev_abox, IOVA_WDMA_BUFFER(id),
				data->dmab.addr, data->dmab.bytes,
				data->dmab.area);
		break;
	case BUFFER_TYPE_RAM:
		ret = abox_of_get_addr(data->abox_data, dev->of_node,
				"samsung,buffer-address",
				(void **)&data->ramb.area,
				&data->ramb.addr, &data->ramb.bytes);
		break;
	}

	return ret;
}

static void abox_wdma_pcm_free(struct snd_soc_component *component,
		struct snd_pcm *pcm)
{
	struct snd_soc_pcm_runtime *rtd = snd_pcm_chip(pcm);
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(dai);
	struct device *dev = data->dev;
	struct device *dev_abox = data->abox_data->dev;
	int id = data->id;
	int ret = 0;

	switch (data->buf_type) {
	case BUFFER_TYPE_ION:
		if (data->ion_buf) {
			ret = abox_ion_free(dev, data->abox_data, data->ion_buf);
			if (ret < 0)
				abox_err(dev, "abox_ion_free() failed %d\n", ret);
			data->ion_buf = NULL;
		}

		if (data->hwdep) {
			snd_device_free(rtd->card->snd_card, data->hwdep);
			data->hwdep = NULL;
		}
		break;
	case BUFFER_TYPE_DMA:
		abox_iommu_unmap(dev_abox, IOVA_WDMA_BUFFER(id));
		if (data->dmab.area) {
			snd_dma_free_pages(&data->dmab);
			data->dmab.area = NULL;
		}
		break;
	default:
		/* nothing to do */
		break;
	}
}

static int abox_wdma_probe(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_dma_data *data = snd_soc_component_get_drvdata(cmpnt);
	char *wname;
	u32 id;
	int ret;

	abox_dbg(dev, "%s\n", __func__);

	data->cmpnt = cmpnt;

	wname = kasprintf(GFP_KERNEL, "%s %s", data->dai_drv[DMA_DAI_PCM].name, "AIF");
	if (!wname)
		return -ENOMEM;
	abox_cmpnt_register_wdma(data->abox_data, dev, data->id, wname);
	kfree(wname);

	ret = of_samsung_property_read_u32(dev, dev->of_node, "asrc-id", &id);
	if (ret >= 0) {
		ret = abox_cmpnt_asrc_lock(data->abox_data,
				SNDRV_PCM_STREAM_CAPTURE, data->id, id);
		if (ret < 0)
			abox_err(dev, "asrc id lock failed\n");
		else
			abox_info(dev, "asrc id locked: %u\n", id);
	}

	ret = abox_dma_add_hw_params_controls(cmpnt);
	if (ret < 0)
		return ret;

	ret = abox_dma_add_func_controls(cmpnt);
	if (ret < 0)
		return ret;

	return 0;
}

static void abox_wdma_remove(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;

	abox_info(dev, "%s\n", __func__);
}

static unsigned int abox_wdma_read(struct snd_soc_component *cmpnt,
		unsigned int reg)
{
	struct abox_dma_data *data = snd_soc_component_get_drvdata(cmpnt);
	struct abox_data *abox_data = data->abox_data;
	unsigned int id = data->id;
	unsigned int base = ABOX_WDMA_CTRL(id);
	unsigned int val = 0;

	if (reg > data->dma_reg_max) {
		abox_warn(cmpnt->dev, "invalid dma register:%#x\n", reg);
		dump_stack();
	}

	/* CTRL register is shared with firmware */
	if (reg == DMA_REG_CTRL) {
		if (pm_runtime_get_if_in_use(cmpnt->dev) > 0) {
			regmap_read(abox_data->regmap, base + reg, &val);
			pm_runtime_put(cmpnt->dev);
		} else {
			val = data->c_reg_ctrl;
		}
	} else {
		regmap_read(abox_data->regmap, base + reg, &val);
	}

	return val;
}

static int abox_wdma_write(struct snd_soc_component *cmpnt,
		unsigned int reg, unsigned int val)
{
	struct abox_dma_data *data = snd_soc_component_get_drvdata(cmpnt);
	struct abox_data *abox_data = data->abox_data;
	unsigned int id = data->id;
	unsigned int base = ABOX_WDMA_CTRL(id);
	int ret = 0;

	if (reg > data->dma_reg_max) {
		abox_warn(cmpnt->dev, "invalid dma register:%#x\n", reg);
		dump_stack();
	}

	/* CTRL register is shared with firmware */
	if (reg == DMA_REG_CTRL) {
		data->c_reg_ctrl &= ~REG_CTRL_KERNEL_MASK;
		data->c_reg_ctrl |= val & REG_CTRL_KERNEL_MASK;
		if (pm_runtime_get_if_in_use(cmpnt->dev) > 0) {
			ret = regmap_update_bits(abox_data->regmap, base + reg,
					REG_CTRL_KERNEL_MASK, val);
			pm_runtime_put(cmpnt->dev);
		}
	} else {
		ret = regmap_write(abox_data->regmap, base + reg, val);
	}

	return ret;
}

static const char * const dither_type_texts[] = {
	"OFF", "RPDF", "TPDF",
};
static SOC_ENUM_SINGLE_DECL(dither_type_enum, DMA_REG_BIT_CTRL,
		ABOX_DMA_DITHER_TYPE_L, dither_type_texts);

static const struct snd_kcontrol_new abox_wdma_controls[] = {
	SOC_ENUM("Dither Type", dither_type_enum),
	ABOX_DMA_SINGLE_S("Dither Seed", DMA_REG_DITHER_SEED,
			ABOX_DMA_DITHER_SEED_L, INT_MAX, 31, 0),
};

static int abox_wdma_aif_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *k, int e)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_dma_data *data = dev_get_drvdata(dev);
	int width, ret = 0;

	if (SND_SOC_DAPM_EVENT_ON(e)) {
		width = abox_cmpnt_get_wdma_dst_bit_width(data->abox_data, dev, data->id);
		abox_info(dev, "dst bit width: %d\n", width);
		ret = abox_dma_set_dst_bit_width(dev, width);
	}

	return ret;
}

static const struct snd_soc_dapm_widget abox_wdma_widgets[] = {
	SND_SOC_DAPM_AIF_OUT_E("AIF", NULL, 0, SND_SOC_NOPM, 0, 0,
			abox_wdma_aif_event, SND_SOC_DAPM_PRE_PMU),
};

static const struct snd_soc_dapm_route abox_wdma_routes[] = {
	{"Capture", NULL, "AIF"},
};

static const struct snd_soc_component_driver abox_wdma = {
	.controls	= abox_wdma_controls,
	.num_controls	= ARRAY_SIZE(abox_wdma_controls),
	.dapm_widgets	= abox_wdma_widgets,
	.num_dapm_widgets = ARRAY_SIZE(abox_wdma_widgets),
	.dapm_routes	= abox_wdma_routes,
	.num_dapm_routes = ARRAY_SIZE(abox_wdma_routes),
	.probe		= abox_wdma_probe,
	.remove		= abox_wdma_remove,
	.read		= abox_wdma_read,
	.write		= abox_wdma_write,
	.pcm_construct	= abox_wdma_pcm_new,
	.pcm_destruct	= abox_wdma_pcm_free,
	.open		= abox_wdma_open,
	.close		= abox_wdma_close,
	.hw_params	= abox_wdma_hw_params,
	.hw_free	= abox_wdma_hw_free,
	.prepare	= abox_wdma_prepare,
	.trigger	= abox_wdma_trigger,
	.pointer	= abox_wdma_pointer,
	.copy		= abox_wdma_copy_user,
	.mmap		= abox_wdma_mmap,
};

static int abox_wdma_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(data->substream);
	struct device *dev = dai->dev;

	if (mute) {
		if (!abox_dma_can_stop(rtd, stream))
			return 0;
	} else {
		if (!abox_dma_can_start(rtd, stream))
			return 0;
	}

	abox_info(dev, "%s(%d)\n", __func__, mute);

	return abox_wdma_trigger_ipc(data, false, !mute);
}

static const struct snd_soc_dai_ops abox_wdma_be_dai_ops = {
	.mute_stream	= abox_wdma_mute_stream,
};

static const struct snd_soc_dai_driver abox_wdma_dai_drv[] = {
	{
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.ops = &abox_wdma_be_dai_ops,
		.playback = {
			.stream_name = "BE Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "BE Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.symmetric_rate = 1,
		.symmetric_channels = 1,
		.symmetric_sample_bits = 1,
	},
};

static enum abox_irq abox_wdma_get_irq(struct abox_dma_data *data,
		enum abox_dma_irq irq)
{
	unsigned int id = data->id;
	enum abox_irq ret;

	if (id >= COUNT_SPUM)
		return -EINVAL;

	switch (irq) {
	case DMA_IRQ_BUF_FULL:
		ret = IRQ_WDMA0_BUF_FULL + id;
		break;
	case DMA_IRQ_FADE_DONE:
		ret = IRQ_WDMA0_FADE_DONE + id;
		break;
	case DMA_IRQ_ERR:
		ret = IRQ_WDMA0_ERR + id;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum abox_dai abox_wdma_get_dai_id(enum abox_dma_dai dai, int id)
{
	enum abox_dai ret;

	if (id >= COUNT_SPUM)
		return -EINVAL;

	switch (dai) {
	case DMA_DAI_PCM:
		ret = ABOX_WDMA0 + id;
		break;
	case DMA_DAI_BE:
		ret = ABOX_WDMA0_BE + id;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static char *abox_wdma_get_dai_name(struct device *dev, enum abox_dma_dai dai,
		int id)
{
	char *ret;

	if (id >= COUNT_SPUM)
		return ERR_PTR(-EINVAL);

	switch (dai) {
	case DMA_DAI_PCM:
		ret = devm_kasprintf(dev, GFP_KERNEL, "WDMA%d", id);
		break;
	case DMA_DAI_BE:
		ret = devm_kasprintf(dev, GFP_KERNEL, "WDMA%d BE", id);
		break;
	default:
		ret = ERR_PTR(-EINVAL);
		break;
	}

	return ret;
}

static const struct of_device_id samsung_abox_wdma_match[] = {
	{
		.compatible = "samsung,abox-wdma",
		.data = (void *)&(struct abox_dma_of_data){
			.get_irq = abox_wdma_get_irq,
			.get_dai_id = abox_wdma_get_dai_id,
			.get_dai_name = abox_wdma_get_dai_name,
			.dai_drv = abox_wdma_dai_drv,
			.num_dai = ARRAY_SIZE(abox_wdma_dai_drv),
			.cmpnt_drv = &abox_wdma
		},
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_wdma_match);

static int abox_wdma_runtime_suspend(struct device *dev)
{
	abox_dbg(dev, "%s\n", __func__);

	return 0;
}

static int abox_wdma_runtime_resume(struct device *dev)
{
	struct abox_dma_data *data = dev_get_drvdata(dev);

	abox_dbg(dev, "%s\n", __func__);

	regmap_update_bits(data->abox_data->regmap, ABOX_WDMA_CTRL0(data->id),
			REG_CTRL_KERNEL_MASK, data->c_reg_ctrl);

	return 0;
}

static int samsung_abox_wdma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct abox_dma_data *data;
	const struct abox_dma_of_data *of_data;
	int i, ret;
	u32 value;
	const char *type;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);
	data->dev = dev;
	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(36));

	data->sfr_base = devm_get_ioremap(pdev, "sfr", NULL, NULL);
	if (IS_ERR(data->sfr_base))
		return PTR_ERR(data->sfr_base);

	data->dev_abox = pdev->dev.parent;
	if (!data->dev_abox) {
		abox_err(dev, "Failed to get abox device\n");
		return -EPROBE_DEFER;
	}
	data->abox_data = dev_get_drvdata(data->dev_abox);

	init_completion(&data->closed);
	init_completion(&data->func_changed);

	abox_register_ipc_handler(data->dev_abox, IPC_PCMCAPTURE,
			abox_wdma_ipc_handler, data->abox_data);

	ret = of_samsung_property_read_u32(dev, np, "id", &data->id);
	if (ret < 0)
		return ret;

	ret = of_samsung_property_read_string(dev, np, "type", &type);
	if (ret < 0)
		type = "";
	if (!strncmp(type, "call", sizeof("call")))
		data->type = PLATFORM_CALL;
	else if (!strncmp(type, "compress", sizeof("compress")))
		data->type = PLATFORM_COMPRESS;
	else if (!strncmp(type, "realtime", sizeof("realtime")))
		data->type = PLATFORM_REALTIME;
	else if (!strncmp(type, "vi-sensing", sizeof("vi-sensing")))
		data->type = PLATFORM_VI_SENSING;
	else if (!strncmp(type, "sync", sizeof("sync")))
		data->type = PLATFORM_SYNC;
	else
		data->type = PLATFORM_NORMAL;

	ret = of_samsung_property_read_u32(dev, np, "buffer_bytes", &value);
	if (ret < 0)
		value = 0;
	data->dmab.bytes = value;

	ret = of_samsung_property_read_string(dev, np, "buffer_type", &type);
	if (ret < 0)
		type = "";
	if (!strncmp(type, "ion", sizeof("ion")))
		data->buf_type = BUFFER_TYPE_ION;
	else if (!strncmp(type, "dma", sizeof("dma")))
		data->buf_type = BUFFER_TYPE_DMA;
	else if (!strncmp(type, "ram", sizeof("ram")))
		data->buf_type = BUFFER_TYPE_RAM;
	else
		data->buf_type = BUFFER_TYPE_DMA;

	of_data = data->of_data = of_device_get_match_data(dev);
	data->num_dai = of_data->num_dai;
	data->dai_drv = devm_kmemdup(dev, of_data->dai_drv,
			sizeof(*of_data->dai_drv) * data->num_dai,
			GFP_KERNEL);
	if (!data->dai_drv)
		return -ENOMEM;

	for (i = 0; i < data->num_dai; i++) {
		data->dai_drv[i].id = of_data->get_dai_id(i, data->id);
		data->dai_drv[i].name = of_data->get_dai_name(dev, i, data->id);
	}

	ret = devm_snd_soc_register_component(dev, data->of_data->cmpnt_drv,
			data->dai_drv, data->num_dai);
	if (ret < 0)
		return ret;

	pm_runtime_enable(dev);

	abox_dma_register_irq(data, DMA_IRQ_FADE_DONE,
			abox_wdma_fade_done, dev);

	data->hwdep = NULL;

	return 0;
}

static int samsung_abox_wdma_remove(struct platform_device *pdev)
{
	return 0;
}

static void samsung_abox_wdma_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	abox_dbg(dev, "%s\n", __func__);
	pm_runtime_disable(dev);
}

static const struct dev_pm_ops samsung_abox_wdma_pm = {
	SET_RUNTIME_PM_OPS(abox_wdma_runtime_suspend,
			abox_wdma_runtime_resume, NULL)
};

struct platform_driver samsung_abox_wdma_driver = {
	.probe  = samsung_abox_wdma_probe,
	.remove = samsung_abox_wdma_remove,
	.shutdown = samsung_abox_wdma_shutdown,
	.driver = {
		.name = "abox-wdma",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_wdma_match),
		.pm = &samsung_abox_wdma_pm,
	},
};
