// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ALSA SoC - Samsung Abox Component driver
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/sched/clock.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#include "abox_soc.h"
#include "abox.h"
#include "abox_dump.h"
#include "abox_vdma.h"
#include "abox_udma.h"
#include "abox_dma.h"
#include "abox_if.h"
#include "abox_vss.h"
#include "abox_atune.h"
#include "abox_cmpnt.h"
#include "abox_memlog.h"
#include "abox_ipc.h"

#define ABOX_ASRC_TUNE_DEFAULT	1000
#define ABOX_ASRC_TUNE_MIN	(ABOX_ASRC_TUNE_DEFAULT / 10 * 6) /* 60% */
#define ABOX_ASRC_TUNE_MAX	(ABOX_ASRC_TUNE_DEFAULT / 10 * 14) /* 140% */

enum asrc_tick {
	TICK_UAIF0 = 0x0,
	TICK_UAIF1 = 0x1,
	TICK_UAIF2 = 0x2,
	TICK_UAIF3 = 0x3,
	TICK_UAIF4 = 0x4,
	TICK_UAIF5 = 0x5,
	TICK_UAIF6 = 0x6,
	TICK_UAIF7 = 0x7,
	TICK_USB = 0x8,
	TICK_CP_EXT = 0x9,
	TICK_BCLK_CP_EXT = 0xA,
	TICK_CP = 0xB,
	TICK_PCM_CNTR = 0xC,
	TICK_BCLK_SPDY = 0xD,
	TICK_SYNC = 0xF,
};

enum asrc_ratio {
	RATIO_1,
	RATIO_2,
	RATIO_4,
	RATIO_8,
	RATIO_16,
	RATIO_32,
	RATIO_64,
	RATIO_128,
	RATIO_256,
};

enum asrc_rate {
	RATE_8000,
	RATE_16000,
	RATE_32000,
	RATE_40000,
	RATE_44100,
	RATE_48000,
	RATE_96000,
	RATE_192000,
	RATE_384000,
	ASRC_RATE_COUNT,
};

static const unsigned int asrc_rates[] = {
	8000, 16000, 32000, 40000, 44100, 48000, 96000, 192000, 384000
};

static enum asrc_rate to_asrc_rate(unsigned int rate)
{
	enum asrc_rate arate;

	for (arate = RATE_8000; arate < ASRC_RATE_COUNT; arate++) {
		if (asrc_rates[arate] == rate)
			break;
	}

	return (arate < ASRC_RATE_COUNT) ? arate : RATE_48000;
}

struct asrc_ctrl {
	unsigned int isr;
	unsigned int osr;
	enum asrc_ratio ovsf;
	unsigned int ifactor;
	enum asrc_ratio dcmf;
};

static const struct asrc_ctrl asrc_table[ASRC_RATE_COUNT][ASRC_RATE_COUNT] = {
		/* isr, osr, ovsf, ifactor, dcmf */
	{
		{  8000,   8000, RATIO_8, 65536, RATIO_8},
		{  8000,  16000, RATIO_8, 65536, RATIO_8},
		{  8000,  32000, RATIO_8, 98304, RATIO_8},
		{  8000,  40000, RATIO_8, 76800, RATIO_8},
		{  8000,  44100, RATIO_8, 88200, RATIO_8},
		{  8000,  48000, RATIO_8, 98304, RATIO_8},
		{  8000,  96000, RATIO_8, 98304, RATIO_4},
		{  8000, 192000, RATIO_8, 98304, RATIO_2},
		{  8000, 384000, RATIO_8, 98304, RATIO_1},
	},
	{
		{ 16000,   8000, RATIO_8, 32768, RATIO_8},
		{ 16000,  16000, RATIO_8, 65536, RATIO_8},
		{ 16000,  32000, RATIO_8, 65536, RATIO_8},
		{ 16000,  40000, RATIO_8, 76800, RATIO_8},
		{ 16000,  44100, RATIO_8, 88200, RATIO_8},
		{ 16000,  48000, RATIO_8, 98304, RATIO_8},
		{ 16000,  96000, RATIO_8, 98304, RATIO_8},
		{ 16000, 192000, RATIO_8, 98304, RATIO_4},
		{ 16000, 384000, RATIO_8, 98304, RATIO_2},
	},
	{
		{ 32000,   8000, RATIO_8, 24576, RATIO_8},
		{ 32000,  16000, RATIO_8, 32768, RATIO_8},
		{ 32000,  32000, RATIO_8, 65536, RATIO_8},
		{ 32000,  40000, RATIO_8, 76800, RATIO_8},
		{ 32000,  44100, RATIO_8, 88200, RATIO_8},
		{ 32000,  48000, RATIO_8, 98304, RATIO_8},
		{ 32000,  96000, RATIO_8, 73728, RATIO_2},
		{ 32000, 192000, RATIO_8, 98304, RATIO_2},
		{ 32000, 384000, RATIO_8, 98304, RATIO_1},
	},
	{
		{ 40000,   8000, RATIO_8, 15360, RATIO_8},
		{ 40000,  16000, RATIO_8, 30720, RATIO_8},
		{ 40000,  32000, RATIO_8, 61440, RATIO_8},
		{ 40000,  40000, RATIO_8, 65536, RATIO_8},
		{ 40000,  44100, RATIO_8, 88200, RATIO_8},
		{ 40000,  48000, RATIO_8, 61440, RATIO_8},
		{ 40000,  96000, RATIO_8, 61440, RATIO_4},
		{ 40000, 192000, RATIO_8, 61440, RATIO_2},
		{ 40000, 384000, RATIO_8, 61440, RATIO_1},
	},
	{
		{ 44100,   8000, RATIO_8, 20480, RATIO_8},
		{ 44100,  16000, RATIO_8, 40960, RATIO_8},
		{ 44100,  32000, RATIO_8, 61440, RATIO_8},
		{ 44100,  40000, RATIO_8, 80000, RATIO_8},
		{ 44100,  44100, RATIO_8, 61440, RATIO_8},
		{ 44100,  48000, RATIO_8, 61440, RATIO_8},
		{ 44100,  96000, RATIO_8, 61440, RATIO_2},
		{ 44100, 192000, RATIO_8, 61440, RATIO_2},
		{ 44100, 384000, RATIO_8, 61440, RATIO_1},
	},
	{
		{ 48000,   8000, RATIO_8, 16384, RATIO_8},
		{ 48000,  16000, RATIO_8, 32768, RATIO_8},
		{ 48000,  32000, RATIO_8, 65536, RATIO_8},
		{ 48000,  40000, RATIO_8, 51200, RATIO_8},
		{ 48000,  44100, RATIO_8, 88200, RATIO_8},
		{ 48000,  48000, RATIO_8, 65536, RATIO_8},
		{ 48000,  96000, RATIO_8, 32768, RATIO_2},
		{ 48000, 192000, RATIO_8, 65536, RATIO_2},
		{ 48000, 384000, RATIO_8, 65536, RATIO_1},
	},
	{
		{ 96000,   8000, RATIO_2, 32768, RATIO_8},
		{ 96000,  16000, RATIO_2, 65536, RATIO_8},
		{ 96000,  32000, RATIO_2, 98304, RATIO_8},
		{ 96000,  40000, RATIO_4, 51200, RATIO_8},
		{ 96000,  44100, RATIO_4, 88200, RATIO_8},
		{ 96000,  48000, RATIO_4, 65536, RATIO_8},
		{ 96000,  96000, RATIO_4, 98304, RATIO_8},
		{ 96000, 192000, RATIO_4, 98304, RATIO_4},
		{ 96000, 384000, RATIO_4, 98304, RATIO_2},
	},
	{
		{192000,   8000, RATIO_2, 16384, RATIO_8},
		{192000,  16000, RATIO_2, 32768, RATIO_8},
		{192000,  32000, RATIO_2, 32768, RATIO_8},
		{192000,  40000, RATIO_2, 51200, RATIO_8},
		{192000,  44100, RATIO_4, 44100, RATIO_8},
		{192000,  48000, RATIO_2, 98304, RATIO_8},
		{192000,  96000, RATIO_1, 98304, RATIO_2},
		{192000, 192000, RATIO_1, 98304, RATIO_1},
		{192000, 384000, RATIO_2, 98304, RATIO_1},
	},
	{
		{384000,   8000, RATIO_1, 16384, RATIO_8},
		{384000,  16000, RATIO_1, 32768, RATIO_8},
		{384000,  32000, RATIO_1, 32768, RATIO_8},
		{384000,  40000, RATIO_1, 51200, RATIO_8},
		{384000,  44100, RATIO_1, 56448, RATIO_8},
		{384000,  48000, RATIO_1, 32768, RATIO_4},
		{384000,  96000, RATIO_1, 65536, RATIO_4},
		{384000, 192000, RATIO_1, 98304, RATIO_2},
		{384000, 384000, RATIO_1, 98304, RATIO_1},
	},
};

#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E9945)
static const struct asrc_ctrl ssrc_table[ASRC_RATE_COUNT][ASRC_RATE_COUNT] = {
	/* isr, osr, ovsf, ifactor, dcmf */
	{},	/*8Khz*/
	{},	/*16Khz*/
	{},	/*32Khz*/
	{},	/*40Khz*/
	{},	/*44.1Khz*/
	{
		{},
		{},
		{},
		{},
		{},
		{ 48000,  48000, RATIO_1, 0, 0},
		{ 48000,  96000, RATIO_2, 0, 0},
		{ 48000, 192000, RATIO_4, 0, 0},
		{ 48000, 384000, RATIO_8, 0, 0},
	},
};
#endif

static enum abox_dai get_sink_dai_id(struct abox_data *data, enum abox_dai id);
static int asrc_update(struct snd_soc_component *cmpnt, int idx, int stream);

static int get_mixp_channels(struct abox_data *data)
{
	int idx, dma_ch, ch = 0;

	for (idx = 0; idx <= data->rdma_count; idx++) {
		if (get_sink_dai_id(data, ABOX_RDMA0 + idx) == ABOX_SIFS0) {
			if (!abox_dma_is_opened(data->dev_rdma[idx]))
				continue;
			dma_ch = abox_dma_get_channels(data->dev_rdma[idx]);
			if (ch < dma_ch)
				ch = dma_ch;
		}
	}

	return ch;
}

static bool sifs_channel_selected(struct abox_data *data, int idx)
{
	unsigned int reg, val = 0;

	reg = ABOX_SIFS_CH_SEL(idx);
	val = snd_soc_component_read(data->cmpnt, reg);
	return !!(val & ABOX_SIFS_CH_SEL_MASK(idx));
}

static bool sifm_channel_selected(struct abox_data *data, int idx)
{
	unsigned int reg, val = 0;

	reg = ABOX_SIFM_CH_SEL(idx);
	val = snd_soc_component_read(data->cmpnt, reg);
	return !!(val & ABOX_SIFM_CH_SEL_MASK(idx));
}

static unsigned int cal_ofactor(const struct asrc_ctrl *ctrl, int tune)
{
	unsigned long isr, osr, ofactor;

	isr = ctrl->isr << ctrl->ovsf;
	osr = (ctrl->osr * tune / ABOX_ASRC_TUNE_DEFAULT) << ctrl->dcmf;
	ofactor = isr * ctrl->ifactor / osr;

	return (unsigned int)ofactor;
}

static unsigned int is_limit(unsigned int is_default)
{
	return is_default / (100 / 5); /* 5% */
}

static unsigned int os_limit(unsigned int os_default)
{
	return os_default / (100 / 5); /* 5% */
}

static int sif_idx(enum ABOX_CONFIGMSG configmsg)
{
	return configmsg - ((configmsg < SET_SIFS0_FORMAT) ?
			SET_SIFS0_RATE : SET_SIFS0_FORMAT);
}

static unsigned int get_sif_rate(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return data->sif_rate[sif_idx(configmsg)];
}

static void set_sif_rate(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, unsigned int val)
{
	data->sif_rate[sif_idx(configmsg)] = val;
}

static snd_pcm_format_t get_sif_format(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return data->sif_format[sif_idx(configmsg)];
}

static void set_sif_format(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, snd_pcm_format_t val)
{
	data->sif_format[sif_idx(configmsg)] = val;
}

static int __maybe_unused get_sif_physical_width(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	snd_pcm_format_t format = get_sif_format(data, configmsg);

	return snd_pcm_format_physical_width(format);
}

static int get_sif_width(struct abox_data *data, enum ABOX_CONFIGMSG configmsg)
{
	return snd_pcm_format_width(get_sif_format(data, configmsg));
}

static void __maybe_unused set_sif_width(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, int width)
{
	struct device *dev = data->dev;
	snd_pcm_format_t format = SNDRV_PCM_FORMAT_S16;

	switch (width) {
	case 16:
		format = SNDRV_PCM_FORMAT_S16;
		break;
	case 24:
		format = SNDRV_PCM_FORMAT_S24;
		break;
	case 32:
		format = SNDRV_PCM_FORMAT_S32;
		break;
	default:
		abox_err(dev, "%s(%d): invalid argument\n", __func__, width);
	}

	set_sif_format(data, configmsg, format);
}

static unsigned int get_sif_channels(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg)
{
	return data->sif_channels[sif_idx(configmsg)];
}

static void set_sif_channels(struct abox_data *data,
		enum ABOX_CONFIGMSG configmsg, unsigned int val)
{
	data->sif_channels[sif_idx(configmsg)] = val;
}

static int rate_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_rate(data, reg);

	abox_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int rate_put_ipc(struct device *adev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	static DEFINE_MUTEX(lock);
	struct abox_data *data = dev_get_drvdata(adev);
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;
	int ret;

	abox_dbg(adev, "%s(%u, %#x)\n", __func__, val, configmsg);

	mutex_lock(&lock);

	if (val > 0)
		set_sif_rate(data, configmsg, val);

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->param1 = get_sif_rate(data, configmsg);
	abox_config_msg->msgtype = configmsg;
	ret = abox_request_ipc(adev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		abox_err(adev, "%s(%u, %#x) failed: %d\n", __func__, val,
				configmsg, ret);

	mutex_unlock(&lock);

	return ret;
}

static int rate_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	abox_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	if (val < mc->min || val > mc->max)
		return -EINVAL;

	return rate_put_ipc(dev, val, reg);
}

static int format_put_ipc(struct device *adev, snd_pcm_format_t format,
		unsigned int channels, enum ABOX_CONFIGMSG configmsg)
{
	static DEFINE_MUTEX(lock);
	struct abox_data *data = dev_get_drvdata(adev);
	struct regmap *regmap = data->regmap;
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;
	int width = snd_pcm_format_width(format);
	int mixp_channels, ret;

	abox_dbg(adev, "%s(%d, %u, %#x)\n", __func__, width, channels,
			configmsg);

	mutex_lock(&lock);

	if (format > 0)
		set_sif_format(data, configmsg, format);
	if (channels > 0)
		set_sif_channels(data, configmsg, channels);

	width = get_sif_width(data, configmsg);
	channels = get_sif_channels(data, configmsg);

	/* update manually for regmap cache sync */
	switch (configmsg) {
	case SET_SIFS0_FORMAT:
		mixp_channels = get_mixp_channels(data);
		if (mixp_channels < channels)
			mixp_channels = channels;

		regmap_update_bits(regmap, ABOX_SPUS_CTRL_MIXP_FORMAT,
				ABOX_MIXP_FORMAT_MASK,
				abox_get_format(width, mixp_channels) <<
				ABOX_MIXP_FORMAT_L);

		/* Set STMIX format */
		regmap_update_bits(regmap, ABOX_SPUS_CTRL_MIXP_FORMAT,
				ABOX_STMIX_FORMAT_MASK,
				abox_get_format(width, mixp_channels) <<
				ABOX_STMIX_FORMAT_L);

		/* Set sidetone output bit width */
		regmap_update_bits(regmap, ABOX_SIDETONE_CTRL,
				ABOX_SDTN_OUT_BITWIDTH_MASK,
				((width / 8) - 1) <<
				ABOX_SDTN_OUT_BITWIDTH_L);
		break;
	default:
		/* Nothing to do */
		break;
	}

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->param1 = width;
	abox_config_msg->param2 = channels;
	abox_config_msg->msgtype = configmsg;
	ret = abox_request_ipc(adev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		abox_err(adev, "%d(%d bit, %u channels) failed: %d\n", configmsg,
				width, channels, ret);

	mutex_unlock(&lock);

	return ret;
}

static int width_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_width(data, reg);

	abox_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int width_put_ipc(struct device *dev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	snd_pcm_format_t format = SNDRV_PCM_FORMAT_S16;

	abox_dbg(dev, "%s(%u, %#x)\n", __func__, val, configmsg);

	switch (val) {
	case 16:
		format = SNDRV_PCM_FORMAT_S16;
		break;
	case 24:
		format = SNDRV_PCM_FORMAT_S24;
		break;
	case 32:
		format = SNDRV_PCM_FORMAT_S32;
		break;
	default:
		abox_warn(dev, "%s(%u, %#x) invalid argument\n", __func__,
				val, configmsg);
		break;
	}

	return format_put_ipc(dev, format, 0, configmsg);
}

static int width_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	abox_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	if (val < mc->min || val > mc->max)
		return -EINVAL;

	return width_put_ipc(dev, val, reg);
}

static int channels_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = get_sif_channels(data, reg);

	abox_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int channels_put_ipc(struct device *dev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	unsigned int channels = val;

	abox_dbg(dev, "%s(%u, %#x)\n", __func__, val, configmsg);

	return format_put_ipc(dev, 0, channels, configmsg);
}

static int channels_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	abox_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	if (val < mc->min || val > mc->max)
		return -EINVAL;

	return channels_put_ipc(dev, val, reg);
}

static void update_ch_num(struct device *adev, enum ABOX_CONFIGMSG configmsg)
{
	struct abox_data *data = dev_get_drvdata(adev);
	struct regmap *regmap = data->regmap;
	unsigned int channels = get_sif_channels(data, configmsg);
	int idx;

	abox_dbg(adev, "%s(%#x)\n", __func__, configmsg);

	switch (configmsg) {
	case SET_SIFS0_FORMAT ... SET_SIFS7_FORMAT:
		/* update output channels */
		idx = configmsg - SET_SIFS0_FORMAT;
		regmap_update_bits_async(regmap, ABOX_SIFS_CH_NUM(idx),
				ABOX_SIFS_CH_NUM_EN_MASK(idx),
				sifs_channel_selected(data, idx) <<
				ABOX_SIFS_CH_NUM_EN_L(idx));
		regmap_update_bits(regmap, ABOX_SIFS_CH_NUM(idx),
				ABOX_SIFS_CH_NUM_MASK(idx),
				(channels - 1) << ABOX_SIFS_CH_NUM_L(idx));
		break;
	case SET_SIFM0_FORMAT ... SET_SIFM15_FORMAT:
		/* update output channels */
		idx = configmsg - SET_SIFM0_FORMAT;
		regmap_update_bits_async(regmap, ABOX_SIFM_CH_NUM(idx),
				ABOX_SIFM_CH_NUM_EN_MASK(idx),
				sifm_channel_selected(data, idx) <<
				ABOX_SIFM_CH_NUM_EN_L(idx));
		regmap_update_bits(regmap, ABOX_SIFM_CH_NUM(idx),
				ABOX_SIFM_CH_NUM_MASK(idx),
				(channels - 1) << ABOX_SIFM_CH_NUM_L(idx));
		break;
	default:
		/* Nothing to do */
		break;
	}
}

int abox_disable_qchannel(struct device *dev, struct abox_data *data,
		enum qchannel clk, int disable)
{
	return regmap_update_bits(data->regmap, ABOX_QCHANNEL_DISABLE,
			ABOX_QCHANNEL_DISABLE_MASK(clk),
			!!disable << ABOX_QCHANNEL_DISABLE_L(clk));
}

static int audio_mode_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int item;

	abox_dbg(dev, "%s: %u\n", __func__, data->audio_mode);

	item = snd_soc_enum_val_to_item(e, data->audio_mode);
	ucontrol->value.enumerated.item[0] = item;

	return 0;
}

static int audio_mode_put_ipc(struct device *dev, enum audio_mode mode)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	abox_dbg(dev, "%s(%d)\n", __func__, mode);

	data->audio_mode_time = local_clock();

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_SET_MODE;
	system_msg->param1 = data->audio_mode = mode;
	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int audio_mode_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	struct abox_data *data = dev_get_drvdata(dev);
	enum audio_mode mode;

	if (item[0] >= e->items)
		return -EINVAL;

	mode = snd_soc_enum_item_to_val(e, item[0]);
	abox_info(dev, "%s(%u)\n", __func__, mode);

	if (IS_ENABLED(CONFIG_SOC_EXYNOS9830) ||
		IS_ENABLED(CONFIG_SOC_EXYNOS2100)) {

		switch (mode) {
		case MODE_IN_CALL:
			abox_vss_notify_call(dev, data, 1);
			break;
		case MODE_NORMAL:
			abox_vss_notify_call(dev, data, 0);
			break;
		default:
			/* nothing to do */
			break;
		}
	}

	return audio_mode_put_ipc(dev, mode);
}

static const char *const audio_mode_enum_texts[] = {
	"NORMAL",
	"RINGTONE",
	"IN_CALL",
	"IN_COMMUNICATION",
	"IN_VIDEOCALL",
	"RESERVED0",
	"RESERVED1",
	"IN_LOOPBACK",
};
static const unsigned int audio_mode_enum_values[] = {
	MODE_NORMAL,
	MODE_RINGTONE,
	MODE_IN_CALL,
	MODE_IN_COMMUNICATION,
	MODE_IN_VIDEOCALL,
	MODE_RESERVED0,
	MODE_RESERVED1,
	MODE_IN_LOOPBACK,
};
static SOC_VALUE_ENUM_SINGLE_DECL(audio_mode_enum, SND_SOC_NOPM, 0, 0,
		audio_mode_enum_texts, audio_mode_enum_values);

static int sound_type_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int item;

	abox_dbg(dev, "%s: %u\n", __func__, data->sound_type);

	item = snd_soc_enum_val_to_item(e, data->sound_type);
	ucontrol->value.enumerated.item[0] = item;

	return 0;
}

static int sound_type_put_ipc(struct device *dev, enum sound_type type)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	abox_dbg(dev, "%s(%d)\n", __func__, type);

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_SET_TYPE;
	system_msg->param1 = data->sound_type = type;

	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int sound_type_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum sound_type type;

	if (item[0] >= e->items)
		return -EINVAL;

	type = snd_soc_enum_item_to_val(e, item[0]);
	abox_info(dev, "%s(%d)\n", __func__, type);

	return sound_type_put_ipc(dev, type);
}
static const char *const sound_type_enum_texts[] = {
	"VOICE",
	"SPEAKER",
	"HEADSET",
	"BTVOICE",
	"USB",
	"CALLFWD",
};
static const unsigned int sound_type_enum_values[] = {
	SOUND_TYPE_VOICE,
	SOUND_TYPE_SPEAKER,
	SOUND_TYPE_HEADSET,
	SOUND_TYPE_BTVOICE,
	SOUND_TYPE_USB,
	SOUND_TYPE_CALLFWD,
};
static SOC_VALUE_ENUM_SINGLE_DECL(sound_type_enum, SND_SOC_NOPM, 0, 0,
		sound_type_enum_texts, sound_type_enum_values);

static int tickle_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	abox_dbg(dev, "%s\n", __func__);

	ucontrol->value.integer.value[0] = data->enabled;

	return 0;
}

static int tickle_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	long val = ucontrol->value.integer.value[0];

	abox_dbg(dev, "%s(%ld)\n", __func__, val);

	if (!!val)
		pm_request_resume(dev);

	return 0;
}

static int cold_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	abox_dbg(dev, "%s\n", __func__);

	ucontrol->value.integer.value[0] = pm_runtime_suspended(data->dev);

	return 0;
}

static int cold_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	/* ignore */
	return 0;
}

static int debug_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int item;

	abox_dbg(dev, "%s: %u\n", __func__, data->debug_mode);

	item = snd_soc_enum_val_to_item(e, data->debug_mode);
	ucontrol->value.enumerated.item[0] = item;

	return 0;
}

static int debug_put_ipc(struct device *dev, enum debug_mode mode)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	abox_dbg(dev, "%s(%d)\n", __func__, mode);

	data->debug_mode = mode;

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_RECOVERY_ACTIVE;
	system_msg->param1 = (mode == DEBUG_MODE_NONE);
	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int debug_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum debug_mode mode;

	if (item[0] >= e->items)
		return -EINVAL;

	mode = snd_soc_enum_item_to_val(e, item[0]);
	abox_info(dev, "%s(%d)\n", __func__, mode);

	return debug_put_ipc(dev, mode);
}
static const char *const debug_enum_texts[] = {
	"None",
	"Dram",
	"File",
	"Wdt",
};
static const unsigned int debug_enum_values[] = {
	DEBUG_MODE_NONE,
	DEBUG_MODE_DRAM,
	DEBUG_MODE_FILE,
	DEBUG_MODE_WDT,
};
static SOC_VALUE_ENUM_SINGLE_DECL(debug_enum, SND_SOC_NOPM, 0, 0,
		debug_enum_texts, debug_enum_values);

static unsigned int s_default = 36864;

enum spus_control_id {
	SPUS_CONTROL_ASRC,
	SPUS_CONTROL_ASRC_ID,
	SPUS_CONTROL_ASRC_APF_COEF,
	SPUS_CONTROL_ASRC_OS,
	SPUS_CONTROL_ASRC_IS,
	SPUS_CONTROL_ASRC_TUNE,
	SPUS_CONTROL_ASRC_DUMMY_START,
	SPUS_CONTROL_ASRC_START_NUM,
	SPUS_CONTROL_COUNT,
};

enum spum_control_id {
	SPUM_CONTROL_ASRC,
	SPUM_CONTROL_ASRC_ID,
	SPUM_CONTROL_ASRC_APF_COEF,
	SPUM_CONTROL_ASRC_OS,
	SPUM_CONTROL_ASRC_IS,
	SPUM_CONTROL_ASRC_TUNE,
	SPUM_CONTROL_COUNT,
};

static int asrc_factor_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = s_default;

	abox_dbg(dev, "%s(%#x): %u\n", __func__, reg, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int asrc_factor_put_ipc(struct device *adev, unsigned int val,
		enum ABOX_CONFIGMSG configmsg)
{
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;
	int ret;

	abox_dbg(adev, "%s(%u, %#x)\n", __func__, val, configmsg);

	s_default = val;

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->param1 = s_default;
	abox_config_msg->msgtype = configmsg;
	ret = abox_request_ipc(adev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		abox_err(adev, "%s(%u, %#x) failed: %d\n", __func__, val,
				configmsg, ret);

	return ret;
}

static int asrc_factor_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	abox_info(dev, "%s(%#x, %u)\n", __func__, reg, val);

	if (val < mc->min || val > mc->max)
		return -EINVAL;

	return asrc_factor_put_ipc(dev, val, reg);
}

static bool spus_asrc_force_enable[] = {
	false, false, false, false,
	false, false, false, false,
	false, false, false, false,
	false, false, false, false
};

static bool spum_asrc_force_enable[] = {
	false, false, false, false,
	false, false, false, false,
	false, false, false, false,
	false, false, false, false
};

static int spus_asrc_enable_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	long val = ucontrol->value.integer.value[0];
	int idx, ret;

	ret = sscanf(kcontrol->id.name, "%*s %*s ASRC%d", &idx);
	if (ret < 1) {
		abox_err(dev, "%s(%s): %d\n", __func__, kcontrol->id.name, ret);
		return ret;
	}
	if (idx < 0 || idx >= ARRAY_SIZE(spus_asrc_force_enable)) {
		abox_err(dev, "%s(%s): %d\n", __func__, kcontrol->id.name, idx);
		return -EINVAL;
	}

	abox_info(dev, "%s(%ld, %d)\n", __func__, val, idx);

	spus_asrc_force_enable[idx] = !!val;

	return 0;
}

static int spum_asrc_enable_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	long val = ucontrol->value.integer.value[0];
	int idx, ret;

	ret = sscanf(kcontrol->id.name, "%*s %*s ASRC%d", &idx);
	if (ret < 1) {
		abox_err(dev, "%s(%s): %d\n", __func__, kcontrol->id.name, ret);
		return ret;
	}
	if (idx < 0 || idx >= ARRAY_SIZE(spum_asrc_force_enable)) {
		abox_err(dev, "%s(%s): %d\n", __func__, kcontrol->id.name, idx);
		return -EINVAL;
	}

	abox_info(dev, "%s(%ld, %d)\n", __func__, val, idx);

	spum_asrc_force_enable[idx] = !!val;

	return 0;
}

static int spus_asrc_id_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	/* ignore asrc id change */
	return 0;
}

static int spum_asrc_id_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	/* ignore asrc id change */
	return 0;
}

static int get_apf_coef(struct abox_data *data, int stream, int idx)
{
	return data->apf_coef[stream ? 1 : 0][idx];
}

static void set_apf_coef(struct abox_data *data, int stream, int idx, int coef)
{
	data->apf_coef[stream ? 1 : 0][idx] = coef;
}

static int asrc_apf_coef_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int stream = mc->reg;
	int idx = mc->shift;

	abox_dbg(dev, "%s(%d, %d)\n", __func__, stream, idx);

	ucontrol->value.integer.value[0] = get_apf_coef(data, stream, idx);

	return 0;
}

static int asrc_apf_coef_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int stream = mc->reg;
	int idx = mc->shift;
	long val = ucontrol->value.integer.value[0];

	abox_dbg(dev, "%s(%d, %d, %ld)\n", __func__, stream, idx, val);

	set_apf_coef(data, stream, idx, val);

	return 0;
}

static int wake_lock_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int val = data->ws->active;

	abox_dbg(dev, "%s: %u\n", __func__, val);

	ucontrol->value.integer.value[0] = val;

	return 0;
}

static int wake_lock_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	abox_info(dev, "%s(%u)\n", __func__, val);

	if (val)
		__pm_stay_awake(data->ws);
	else
		__pm_relax(data->ws);

	return 0;
}

static int reset_log_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned long *addr;

	abox_dbg(dev, "%s\n", __func__);

	for (addr = data->slog_base + ABOX_SLOG_OFFSET; (void *)addr <
			data->slog_base + data->slog_size; addr++) {
		if (*addr) {
			/* There is non-zero */
			ucontrol->value.integer.value[0] = 0;
			return 0;
		}
	}

	/* Area is all zero */
	ucontrol->value.integer.value[0] = 1;
	return 0;
}

static int reset_log_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	abox_dbg(dev, "%s(%u)\n", __func__, val);

	if (val) {
		abox_info(dev, "reset silent log area\n");
		memset(data->slog_base + ABOX_SLOG_OFFSET, 0,
				data->slog_size - ABOX_SLOG_OFFSET);
	}

	return 0;
}

static bool nsrc_bridge[16];

static int nsrc_bridge_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	unsigned int id = mc->shift - ABOX_NSRC_CONNECTION_TYPE_L(0);

	abox_dbg(dev, "%s(%u)\n", __func__, id);

	if (id >= COUNT_SIFM)
		return -EINVAL;

	ucontrol->value.integer.value[0] = nsrc_bridge[id];

	return 0;
}

static int nsrc_bridge_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	unsigned int id = mc->shift - ABOX_NSRC_CONNECTION_TYPE_L(0);
	unsigned int val = (unsigned int)ucontrol->value.integer.value[0];

	abox_dbg(dev, "%s(%u, %u)\n", __func__, id, val);

	nsrc_bridge[id] = val;

	return 0;
}

static enum asrc_tick spus_asrc_os[] = {
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
};
static enum asrc_tick spus_asrc_is[] = {
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC
};

static enum asrc_tick spum_asrc_os[] = {
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
};
static enum asrc_tick spum_asrc_is[] = {
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
	TICK_SYNC, TICK_SYNC, TICK_SYNC, TICK_SYNC,
};

static int spus_asrc_os_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick = spus_asrc_os[idx];

	abox_dbg(dev, "%s(%d): %d\n", __func__, idx, tick);

	item[0] = snd_soc_enum_val_to_item(e, tick);

	return 0;
}

static int spus_asrc_os_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick;

	if (item[0] >= e->items)
		return -EINVAL;

	tick = snd_soc_enum_item_to_val(e, item[0]);
	abox_dbg(dev, "%s(%d, %d)\n", __func__, idx, tick);
	spus_asrc_os[idx] = tick;

	return 0;
}

static int spus_asrc_is_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick = spus_asrc_is[idx];

	abox_dbg(dev, "%s(%d): %d\n", __func__, idx, tick);

	item[0] = snd_soc_enum_val_to_item(e, tick);

	return 0;
}

static int spus_asrc_is_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick;

	if (item[0] >= e->items)
		return -EINVAL;

	tick = snd_soc_enum_item_to_val(e, item[0]);
	abox_dbg(dev, "%s(%d, %d)\n", __func__, idx, tick);
	spus_asrc_is[idx] = tick;

	return 0;
}

static int spum_asrc_os_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick = spum_asrc_os[idx];

	abox_dbg(dev, "%s(%d): %d\n", __func__, idx, tick);

	item[0] = snd_soc_enum_val_to_item(e, tick);

	return 0;
}

static int spum_asrc_os_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick;

	if (item[0] >= e->items)
		return -EINVAL;

	tick = snd_soc_enum_item_to_val(e, item[0]);
	abox_dbg(dev, "%s(%d, %d)\n", __func__, idx, tick);
	spum_asrc_os[idx] = tick;

	return 0;
}

static int spum_asrc_is_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick = spum_asrc_is[idx];

	abox_dbg(dev, "%s(%d): %d\n", __func__, idx, tick);

	item[0] = snd_soc_enum_val_to_item(e, tick);

	return 0;
}

static int spum_asrc_is_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int idx = e->reg;
	unsigned int *item = ucontrol->value.enumerated.item;
	enum asrc_tick tick;

	if (item[0] >= e->items)
		return -EINVAL;

	tick = snd_soc_enum_item_to_val(e, item[0]);
	abox_dbg(dev, "%s(%d, %d)\n", __func__, idx, tick);
	spum_asrc_is[idx] = tick;

	return 0;
}

static int vdma_buffer_status_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	/* ignore */
	return 0;
}

static int vdma_buffer_status_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int id = ucontrol->value.integer.value[0];
	struct abox_vdma_info *info;
	struct abox_vdma_rtd *rtd;
	struct snd_pcm_runtime *runtime;
	snd_pcm_uframes_t avail;

	abox_info(dev, "%s(%d)\n", __func__, id);

	if (id < PCMTASK_VDMA_ID_BASE || id > mc->max)
		return -EINVAL;

	info = abox_vdma_get_info(id);
	if (!info)
		return -ENOMEM;

	rtd = &info->rtd[info->stream];
	if (!rtd->substream)
		return -ENOMEM;

	runtime = rtd->substream->runtime;
	if (!runtime)
		return -ENOMEM;

	avail = runtime->status->hw_ptr - runtime->control->appl_ptr;
	if (avail < 0)
		avail += runtime->boundary;

	abox_info(dev, "%s(%lu)(%lu)(%lu)\n", __func__, avail, runtime->buffer_size, runtime->stop_threshold);

	return 0;
}


static const struct snd_kcontrol_new cmpnt_controls[] = {
	SOC_SINGLE_EXT("SIFS0 Rate", SET_SIFS0_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS1 Rate", SET_SIFS1_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS2 Rate", SET_SIFS2_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS3 Rate", SET_SIFS3_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS4 Rate", SET_SIFS4_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS5 Rate", SET_SIFS5_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS6 Rate", SET_SIFS6_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS7 Rate", SET_SIFS7_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM0 Rate", SET_SIFM0_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM1 Rate", SET_SIFM1_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM2 Rate", SET_SIFM2_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM3 Rate", SET_SIFM3_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM4 Rate", SET_SIFM4_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM5 Rate", SET_SIFM5_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM6 Rate", SET_SIFM6_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM7 Rate", SET_SIFM7_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM8 Rate", SET_SIFM8_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM9 Rate", SET_SIFM9_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM10 Rate", SET_SIFM10_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM11 Rate", SET_SIFM11_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM12 Rate", SET_SIFM12_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM13 Rate", SET_SIFM13_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM14 Rate", SET_SIFM14_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFM15 Rate", SET_SIFM15_RATE, 8000, 384000, 0,
			rate_get, rate_put),
	SOC_SINGLE_EXT("SIFS0 Width", SET_SIFS0_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS1 Width", SET_SIFS1_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS2 Width", SET_SIFS2_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS3 Width", SET_SIFS3_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS4 Width", SET_SIFS4_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS5 Width", SET_SIFS5_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS6 Width", SET_SIFS6_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS7 Width", SET_SIFS7_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM0 Width", SET_SIFM0_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM1 Width", SET_SIFM1_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM2 Width", SET_SIFM2_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM3 Width", SET_SIFM3_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM4 Width", SET_SIFM4_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM5 Width", SET_SIFM5_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM6 Width", SET_SIFM6_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM7 Width", SET_SIFM7_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM8 Width", SET_SIFM8_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM9 Width", SET_SIFM9_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM10 Width", SET_SIFM10_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM11 Width", SET_SIFM11_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM12 Width", SET_SIFM12_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM13 Width", SET_SIFM13_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM14 Width", SET_SIFM14_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFM15 Width", SET_SIFM15_FORMAT, 16, 32, 0,
			width_get, width_put),
	SOC_SINGLE_EXT("SIFS0 Channel", SET_SIFS0_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFS1 Channel", SET_SIFS1_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFS2 Channel", SET_SIFS2_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFS3 Channel", SET_SIFS3_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFS4 Channel", SET_SIFS4_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFS5 Channel", SET_SIFS5_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFS6 Channel", SET_SIFS6_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFS7 Channel", SET_SIFS7_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM0 Channel", SET_SIFM0_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM1 Channel", SET_SIFM1_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM2 Channel", SET_SIFM2_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM3 Channel", SET_SIFM3_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM4 Channel", SET_SIFM4_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM5 Channel", SET_SIFM5_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM6 Channel", SET_SIFM6_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM7 Channel", SET_SIFM7_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM8 Channel", SET_SIFM8_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM9 Channel", SET_SIFM9_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM10 Channel", SET_SIFM10_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM11 Channel", SET_SIFM11_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM12 Channel", SET_SIFM12_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM13 Channel", SET_SIFM13_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM14 Channel", SET_SIFM14_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE_EXT("SIFM15 Channel", SET_SIFM15_FORMAT, 1, 8, 0,
			channels_get, channels_put),
	SOC_SINGLE("SIFS0 CH0 Switch", ABOX_SIFS_CH_SEL(0),
			ABOX_SIFS_CH_SEL_L(0) + 0, 1, 0),
	SOC_SINGLE("SIFS0 CH1 Switch", ABOX_SIFS_CH_SEL(0),
			ABOX_SIFS_CH_SEL_L(0) + 1, 1, 0),
	SOC_SINGLE("SIFS0 CH2 Switch", ABOX_SIFS_CH_SEL(0),
			ABOX_SIFS_CH_SEL_L(0) + 2, 1, 0),
	SOC_SINGLE("SIFS0 CH3 Switch", ABOX_SIFS_CH_SEL(0),
			ABOX_SIFS_CH_SEL_L(0) + 3, 1, 0),
	SOC_SINGLE("SIFS0 CH4 Switch", ABOX_SIFS_CH_SEL(0),
			ABOX_SIFS_CH_SEL_L(0) + 4, 1, 0),
	SOC_SINGLE("SIFS0 CH5 Switch", ABOX_SIFS_CH_SEL(0),
			ABOX_SIFS_CH_SEL_L(0) + 5, 1, 0),
	SOC_SINGLE("SIFS0 CH6 Switch", ABOX_SIFS_CH_SEL(0),
			ABOX_SIFS_CH_SEL_L(0) + 6, 1, 0),
	SOC_SINGLE("SIFS0 CH7 Switch", ABOX_SIFS_CH_SEL(0),
			ABOX_SIFS_CH_SEL_L(0) + 7, 1, 0),
	SOC_SINGLE("SIFS1 CH0 Switch", ABOX_SIFS_CH_SEL(1),
			ABOX_SIFS_CH_SEL_L(1) + 0, 1, 0),
	SOC_SINGLE("SIFS1 CH1 Switch", ABOX_SIFS_CH_SEL(1),
			ABOX_SIFS_CH_SEL_L(1) + 1, 1, 0),
	SOC_SINGLE("SIFS1 CH2 Switch", ABOX_SIFS_CH_SEL(1),
			ABOX_SIFS_CH_SEL_L(1) + 2, 1, 0),
	SOC_SINGLE("SIFS1 CH3 Switch", ABOX_SIFS_CH_SEL(1),
			ABOX_SIFS_CH_SEL_L(1) + 3, 1, 0),
	SOC_SINGLE("SIFS1 CH4 Switch", ABOX_SIFS_CH_SEL(1),
			ABOX_SIFS_CH_SEL_L(1) + 4, 1, 0),
	SOC_SINGLE("SIFS1 CH5 Switch", ABOX_SIFS_CH_SEL(1),
			ABOX_SIFS_CH_SEL_L(1) + 5, 1, 0),
	SOC_SINGLE("SIFS1 CH6 Switch", ABOX_SIFS_CH_SEL(1),
			ABOX_SIFS_CH_SEL_L(1) + 6, 1, 0),
	SOC_SINGLE("SIFS1 CH7 Switch", ABOX_SIFS_CH_SEL(1),
			ABOX_SIFS_CH_SEL_L(1) + 7, 1, 0),
	SOC_SINGLE("SIFS2 CH0 Switch", ABOX_SIFS_CH_SEL(2),
			ABOX_SIFS_CH_SEL_L(2) + 0, 1, 0),
	SOC_SINGLE("SIFS2 CH1 Switch", ABOX_SIFS_CH_SEL(2),
			ABOX_SIFS_CH_SEL_L(2) + 1, 1, 0),
	SOC_SINGLE("SIFS2 CH2 Switch", ABOX_SIFS_CH_SEL(2),
			ABOX_SIFS_CH_SEL_L(2) + 2, 1, 0),
	SOC_SINGLE("SIFS2 CH3 Switch", ABOX_SIFS_CH_SEL(2),
			ABOX_SIFS_CH_SEL_L(2) + 3, 1, 0),
	SOC_SINGLE("SIFS2 CH4 Switch", ABOX_SIFS_CH_SEL(2),
			ABOX_SIFS_CH_SEL_L(2) + 4, 1, 0),
	SOC_SINGLE("SIFS2 CH5 Switch", ABOX_SIFS_CH_SEL(2),
			ABOX_SIFS_CH_SEL_L(2) + 5, 1, 0),
	SOC_SINGLE("SIFS2 CH6 Switch", ABOX_SIFS_CH_SEL(2),
			ABOX_SIFS_CH_SEL_L(2) + 6, 1, 0),
	SOC_SINGLE("SIFS2 CH7 Switch", ABOX_SIFS_CH_SEL(2),
			ABOX_SIFS_CH_SEL_L(2) + 7, 1, 0),
	SOC_SINGLE("SIFS3 CH0 Switch", ABOX_SIFS_CH_SEL(3),
			ABOX_SIFS_CH_SEL_L(3) + 0, 1, 0),
	SOC_SINGLE("SIFS3 CH1 Switch", ABOX_SIFS_CH_SEL(3),
			ABOX_SIFS_CH_SEL_L(3) + 1, 1, 0),
	SOC_SINGLE("SIFS3 CH2 Switch", ABOX_SIFS_CH_SEL(3),
			ABOX_SIFS_CH_SEL_L(3) + 2, 1, 0),
	SOC_SINGLE("SIFS3 CH3 Switch", ABOX_SIFS_CH_SEL(3),
			ABOX_SIFS_CH_SEL_L(3) + 3, 1, 0),
	SOC_SINGLE("SIFS3 CH4 Switch", ABOX_SIFS_CH_SEL(3),
			ABOX_SIFS_CH_SEL_L(3) + 4, 1, 0),
	SOC_SINGLE("SIFS3 CH5 Switch", ABOX_SIFS_CH_SEL(3),
			ABOX_SIFS_CH_SEL_L(3) + 5, 1, 0),
	SOC_SINGLE("SIFS3 CH6 Switch", ABOX_SIFS_CH_SEL(3),
			ABOX_SIFS_CH_SEL_L(3) + 6, 1, 0),
	SOC_SINGLE("SIFS3 CH7 Switch", ABOX_SIFS_CH_SEL(3),
			ABOX_SIFS_CH_SEL_L(3) + 7, 1, 0),
	SOC_SINGLE("SIFS4 CH0 Switch", ABOX_SIFS_CH_SEL(4),
			ABOX_SIFS_CH_SEL_L(4) + 0, 1, 0),
	SOC_SINGLE("SIFS4 CH1 Switch", ABOX_SIFS_CH_SEL(4),
			ABOX_SIFS_CH_SEL_L(4) + 1, 1, 0),
	SOC_SINGLE("SIFS4 CH2 Switch", ABOX_SIFS_CH_SEL(4),
			ABOX_SIFS_CH_SEL_L(4) + 2, 1, 0),
	SOC_SINGLE("SIFS4 CH3 Switch", ABOX_SIFS_CH_SEL(4),
			ABOX_SIFS_CH_SEL_L(4) + 3, 1, 0),
	SOC_SINGLE("SIFS4 CH4 Switch", ABOX_SIFS_CH_SEL(4),
			ABOX_SIFS_CH_SEL_L(4) + 4, 1, 0),
	SOC_SINGLE("SIFS4 CH5 Switch", ABOX_SIFS_CH_SEL(4),
			ABOX_SIFS_CH_SEL_L(4) + 5, 1, 0),
	SOC_SINGLE("SIFS4 CH6 Switch", ABOX_SIFS_CH_SEL(4),
			ABOX_SIFS_CH_SEL_L(4) + 6, 1, 0),
	SOC_SINGLE("SIFS4 CH7 Switch", ABOX_SIFS_CH_SEL(4),
			ABOX_SIFS_CH_SEL_L(4) + 7, 1, 0),
	SOC_SINGLE("SIFS5 CH0 Switch", ABOX_SIFS_CH_SEL(5),
			ABOX_SIFS_CH_SEL_L(5) + 0, 1, 0),
	SOC_SINGLE("SIFS5 CH1 Switch", ABOX_SIFS_CH_SEL(5),
			ABOX_SIFS_CH_SEL_L(5) + 1, 1, 0),
	SOC_SINGLE("SIFS5 CH2 Switch", ABOX_SIFS_CH_SEL(5),
			ABOX_SIFS_CH_SEL_L(5) + 2, 1, 0),
	SOC_SINGLE("SIFS5 CH3 Switch", ABOX_SIFS_CH_SEL(5),
			ABOX_SIFS_CH_SEL_L(5) + 3, 1, 0),
	SOC_SINGLE("SIFS5 CH4 Switch", ABOX_SIFS_CH_SEL(5),
			ABOX_SIFS_CH_SEL_L(5) + 4, 1, 0),
	SOC_SINGLE("SIFS5 CH5 Switch", ABOX_SIFS_CH_SEL(5),
			ABOX_SIFS_CH_SEL_L(5) + 5, 1, 0),
	SOC_SINGLE("SIFS5 CH6 Switch", ABOX_SIFS_CH_SEL(5),
			ABOX_SIFS_CH_SEL_L(5) + 6, 1, 0),
	SOC_SINGLE("SIFS5 CH7 Switch", ABOX_SIFS_CH_SEL(5),
			ABOX_SIFS_CH_SEL_L(5) + 7, 1, 0),
	SOC_SINGLE("SIFS6 CH0 Switch", ABOX_SIFS_CH_SEL(6),
			ABOX_SIFS_CH_SEL_L(6) + 0, 1, 0),
	SOC_SINGLE("SIFS6 CH1 Switch", ABOX_SIFS_CH_SEL(6),
			ABOX_SIFS_CH_SEL_L(6) + 1, 1, 0),
	SOC_SINGLE("SIFS6 CH2 Switch", ABOX_SIFS_CH_SEL(6),
			ABOX_SIFS_CH_SEL_L(6) + 2, 1, 0),
	SOC_SINGLE("SIFS6 CH3 Switch", ABOX_SIFS_CH_SEL(6),
			ABOX_SIFS_CH_SEL_L(6) + 3, 1, 0),
	SOC_SINGLE("SIFS6 CH4 Switch", ABOX_SIFS_CH_SEL(6),
			ABOX_SIFS_CH_SEL_L(6) + 4, 1, 0),
	SOC_SINGLE("SIFS6 CH5 Switch", ABOX_SIFS_CH_SEL(6),
			ABOX_SIFS_CH_SEL_L(6) + 5, 1, 0),
	SOC_SINGLE("SIFS6 CH6 Switch", ABOX_SIFS_CH_SEL(6),
			ABOX_SIFS_CH_SEL_L(6) + 6, 1, 0),
	SOC_SINGLE("SIFS6 CH7 Switch", ABOX_SIFS_CH_SEL(6),
			ABOX_SIFS_CH_SEL_L(6) + 7, 1, 0),
	SOC_SINGLE("SIFS7 CH0 Switch", ABOX_SIFS_CH_SEL(7),
			ABOX_SIFS_CH_SEL_L(7) + 0, 1, 0),
	SOC_SINGLE("SIFS7 CH1 Switch", ABOX_SIFS_CH_SEL(7),
			ABOX_SIFS_CH_SEL_L(7) + 1, 1, 0),
	SOC_SINGLE("SIFS7 CH2 Switch", ABOX_SIFS_CH_SEL(7),
			ABOX_SIFS_CH_SEL_L(7) + 2, 1, 0),
	SOC_SINGLE("SIFS7 CH3 Switch", ABOX_SIFS_CH_SEL(7),
			ABOX_SIFS_CH_SEL_L(7) + 3, 1, 0),
	SOC_SINGLE("SIFS7 CH4 Switch", ABOX_SIFS_CH_SEL(7),
			ABOX_SIFS_CH_SEL_L(7) + 4, 1, 0),
	SOC_SINGLE("SIFS7 CH5 Switch", ABOX_SIFS_CH_SEL(7),
			ABOX_SIFS_CH_SEL_L(7) + 5, 1, 0),
	SOC_SINGLE("SIFS7 CH6 Switch", ABOX_SIFS_CH_SEL(7),
			ABOX_SIFS_CH_SEL_L(7) + 6, 1, 0),
	SOC_SINGLE("SIFS7 CH7 Switch", ABOX_SIFS_CH_SEL(7),
			ABOX_SIFS_CH_SEL_L(7) + 7, 1, 0),
	SOC_SINGLE("SIFM0 CH0 Switch", ABOX_SIFM_CH_SEL(0),
			ABOX_SIFM_CH_SEL_L(0) + 0, 1, 0),
	SOC_SINGLE("SIFM0 CH1 Switch", ABOX_SIFM_CH_SEL(0),
			ABOX_SIFM_CH_SEL_L(0) + 1, 1, 0),
	SOC_SINGLE("SIFM0 CH2 Switch", ABOX_SIFM_CH_SEL(0),
			ABOX_SIFM_CH_SEL_L(0) + 2, 1, 0),
	SOC_SINGLE("SIFM0 CH3 Switch", ABOX_SIFM_CH_SEL(0),
			ABOX_SIFM_CH_SEL_L(0) + 3, 1, 0),
	SOC_SINGLE("SIFM0 CH4 Switch", ABOX_SIFM_CH_SEL(0),
			ABOX_SIFM_CH_SEL_L(0) + 4, 1, 0),
	SOC_SINGLE("SIFM0 CH5 Switch", ABOX_SIFM_CH_SEL(0),
			ABOX_SIFM_CH_SEL_L(0) + 5, 1, 0),
	SOC_SINGLE("SIFM0 CH6 Switch", ABOX_SIFM_CH_SEL(0),
			ABOX_SIFM_CH_SEL_L(0) + 6, 1, 0),
	SOC_SINGLE("SIFM0 CH7 Switch", ABOX_SIFM_CH_SEL(0),
			ABOX_SIFM_CH_SEL_L(0) + 7, 1, 0),
	SOC_SINGLE("SIFM1 CH0 Switch", ABOX_SIFM_CH_SEL(1),
			ABOX_SIFM_CH_SEL_L(1) + 0, 1, 0),
	SOC_SINGLE("SIFM1 CH1 Switch", ABOX_SIFM_CH_SEL(1),
			ABOX_SIFM_CH_SEL_L(1) + 1, 1, 0),
	SOC_SINGLE("SIFM1 CH2 Switch", ABOX_SIFM_CH_SEL(1),
			ABOX_SIFM_CH_SEL_L(1) + 2, 1, 0),
	SOC_SINGLE("SIFM1 CH3 Switch", ABOX_SIFM_CH_SEL(1),
			ABOX_SIFM_CH_SEL_L(1) + 3, 1, 0),
	SOC_SINGLE("SIFM1 CH4 Switch", ABOX_SIFM_CH_SEL(1),
			ABOX_SIFM_CH_SEL_L(1) + 4, 1, 0),
	SOC_SINGLE("SIFM1 CH5 Switch", ABOX_SIFM_CH_SEL(1),
			ABOX_SIFM_CH_SEL_L(1) + 5, 1, 0),
	SOC_SINGLE("SIFM1 CH6 Switch", ABOX_SIFM_CH_SEL(1),
			ABOX_SIFM_CH_SEL_L(1) + 6, 1, 0),
	SOC_SINGLE("SIFM1 CH7 Switch", ABOX_SIFM_CH_SEL(1),
			ABOX_SIFM_CH_SEL_L(1) + 7, 1, 0),
	SOC_SINGLE("SIFM2 CH0 Switch", ABOX_SIFM_CH_SEL(2),
			ABOX_SIFM_CH_SEL_L(2) + 0, 1, 0),
	SOC_SINGLE("SIFM2 CH1 Switch", ABOX_SIFM_CH_SEL(2),
			ABOX_SIFM_CH_SEL_L(2) + 1, 1, 0),
	SOC_SINGLE("SIFM2 CH2 Switch", ABOX_SIFM_CH_SEL(2),
			ABOX_SIFM_CH_SEL_L(2) + 2, 1, 0),
	SOC_SINGLE("SIFM2 CH3 Switch", ABOX_SIFM_CH_SEL(2),
			ABOX_SIFM_CH_SEL_L(2) + 3, 1, 0),
	SOC_SINGLE("SIFM2 CH4 Switch", ABOX_SIFM_CH_SEL(2),
			ABOX_SIFM_CH_SEL_L(2) + 4, 1, 0),
	SOC_SINGLE("SIFM2 CH5 Switch", ABOX_SIFM_CH_SEL(2),
			ABOX_SIFM_CH_SEL_L(2) + 5, 1, 0),
	SOC_SINGLE("SIFM2 CH6 Switch", ABOX_SIFM_CH_SEL(2),
			ABOX_SIFM_CH_SEL_L(2) + 6, 1, 0),
	SOC_SINGLE("SIFM2 CH7 Switch", ABOX_SIFM_CH_SEL(2),
			ABOX_SIFM_CH_SEL_L(2) + 7, 1, 0),
	SOC_SINGLE("SIFM3 CH0 Switch", ABOX_SIFM_CH_SEL(3),
			ABOX_SIFM_CH_SEL_L(3) + 0, 1, 0),
	SOC_SINGLE("SIFM3 CH1 Switch", ABOX_SIFM_CH_SEL(3),
			ABOX_SIFM_CH_SEL_L(3) + 1, 1, 0),
	SOC_SINGLE("SIFM3 CH2 Switch", ABOX_SIFM_CH_SEL(3),
			ABOX_SIFM_CH_SEL_L(3) + 2, 1, 0),
	SOC_SINGLE("SIFM3 CH3 Switch", ABOX_SIFM_CH_SEL(3),
			ABOX_SIFM_CH_SEL_L(3) + 3, 1, 0),
	SOC_SINGLE("SIFM3 CH4 Switch", ABOX_SIFM_CH_SEL(3),
			ABOX_SIFM_CH_SEL_L(3) + 4, 1, 0),
	SOC_SINGLE("SIFM3 CH5 Switch", ABOX_SIFM_CH_SEL(3),
			ABOX_SIFM_CH_SEL_L(3) + 5, 1, 0),
	SOC_SINGLE("SIFM3 CH6 Switch", ABOX_SIFM_CH_SEL(3),
			ABOX_SIFM_CH_SEL_L(3) + 6, 1, 0),
	SOC_SINGLE("SIFM3 CH7 Switch", ABOX_SIFM_CH_SEL(3),
			ABOX_SIFM_CH_SEL_L(3) + 7, 1, 0),
	SOC_SINGLE("SIFM4 CH0 Switch", ABOX_SIFM_CH_SEL(4),
			ABOX_SIFM_CH_SEL_L(4) + 0, 1, 0),
	SOC_SINGLE("SIFM4 CH1 Switch", ABOX_SIFM_CH_SEL(4),
			ABOX_SIFM_CH_SEL_L(4) + 1, 1, 0),
	SOC_SINGLE("SIFM4 CH2 Switch", ABOX_SIFM_CH_SEL(4),
			ABOX_SIFM_CH_SEL_L(4) + 2, 1, 0),
	SOC_SINGLE("SIFM4 CH3 Switch", ABOX_SIFM_CH_SEL(4),
			ABOX_SIFM_CH_SEL_L(4) + 3, 1, 0),
	SOC_SINGLE("SIFM4 CH4 Switch", ABOX_SIFM_CH_SEL(4),
			ABOX_SIFM_CH_SEL_L(4) + 4, 1, 0),
	SOC_SINGLE("SIFM4 CH5 Switch", ABOX_SIFM_CH_SEL(4),
			ABOX_SIFM_CH_SEL_L(4) + 5, 1, 0),
	SOC_SINGLE("SIFM4 CH6 Switch", ABOX_SIFM_CH_SEL(4),
			ABOX_SIFM_CH_SEL_L(4) + 6, 1, 0),
	SOC_SINGLE("SIFM4 CH7 Switch", ABOX_SIFM_CH_SEL(4),
			ABOX_SIFM_CH_SEL_L(4) + 7, 1, 0),
	SOC_SINGLE("SIFM5 CH0 Switch", ABOX_SIFM_CH_SEL(5),
			ABOX_SIFM_CH_SEL_L(5) + 0, 1, 0),
	SOC_SINGLE("SIFM5 CH1 Switch", ABOX_SIFM_CH_SEL(5),
			ABOX_SIFM_CH_SEL_L(5) + 1, 1, 0),
	SOC_SINGLE("SIFM5 CH2 Switch", ABOX_SIFM_CH_SEL(5),
			ABOX_SIFM_CH_SEL_L(5) + 2, 1, 0),
	SOC_SINGLE("SIFM5 CH3 Switch", ABOX_SIFM_CH_SEL(5),
			ABOX_SIFM_CH_SEL_L(5) + 3, 1, 0),
	SOC_SINGLE("SIFM5 CH4 Switch", ABOX_SIFM_CH_SEL(5),
			ABOX_SIFM_CH_SEL_L(5) + 4, 1, 0),
	SOC_SINGLE("SIFM5 CH5 Switch", ABOX_SIFM_CH_SEL(5),
			ABOX_SIFM_CH_SEL_L(5) + 5, 1, 0),
	SOC_SINGLE("SIFM5 CH6 Switch", ABOX_SIFM_CH_SEL(5),
			ABOX_SIFM_CH_SEL_L(5) + 6, 1, 0),
	SOC_SINGLE("SIFM5 CH7 Switch", ABOX_SIFM_CH_SEL(5),
			ABOX_SIFM_CH_SEL_L(5) + 7, 1, 0),
	SOC_SINGLE("SIFM6 CH0 Switch", ABOX_SIFM_CH_SEL(6),
			ABOX_SIFM_CH_SEL_L(6) + 0, 1, 0),
	SOC_SINGLE("SIFM6 CH1 Switch", ABOX_SIFM_CH_SEL(6),
			ABOX_SIFM_CH_SEL_L(6) + 1, 1, 0),
	SOC_SINGLE("SIFM6 CH2 Switch", ABOX_SIFM_CH_SEL(6),
			ABOX_SIFM_CH_SEL_L(6) + 2, 1, 0),
	SOC_SINGLE("SIFM6 CH3 Switch", ABOX_SIFM_CH_SEL(6),
			ABOX_SIFM_CH_SEL_L(6) + 3, 1, 0),
	SOC_SINGLE("SIFM6 CH4 Switch", ABOX_SIFM_CH_SEL(6),
			ABOX_SIFM_CH_SEL_L(6) + 4, 1, 0),
	SOC_SINGLE("SIFM6 CH5 Switch", ABOX_SIFM_CH_SEL(6),
			ABOX_SIFM_CH_SEL_L(6) + 5, 1, 0),
	SOC_SINGLE("SIFM6 CH6 Switch", ABOX_SIFM_CH_SEL(6),
			ABOX_SIFM_CH_SEL_L(6) + 6, 1, 0),
	SOC_SINGLE("SIFM6 CH7 Switch", ABOX_SIFM_CH_SEL(6),
			ABOX_SIFM_CH_SEL_L(6) + 7, 1, 0),
	SOC_SINGLE("SIFM7 CH0 Switch", ABOX_SIFM_CH_SEL(7),
			ABOX_SIFM_CH_SEL_L(7) + 0, 1, 0),
	SOC_SINGLE("SIFM7 CH1 Switch", ABOX_SIFM_CH_SEL(7),
			ABOX_SIFM_CH_SEL_L(7) + 1, 1, 0),
	SOC_SINGLE("SIFM7 CH2 Switch", ABOX_SIFM_CH_SEL(7),
			ABOX_SIFM_CH_SEL_L(7) + 2, 1, 0),
	SOC_SINGLE("SIFM7 CH3 Switch", ABOX_SIFM_CH_SEL(7),
			ABOX_SIFM_CH_SEL_L(7) + 3, 1, 0),
	SOC_SINGLE("SIFM7 CH4 Switch", ABOX_SIFM_CH_SEL(7),
			ABOX_SIFM_CH_SEL_L(7) + 4, 1, 0),
	SOC_SINGLE("SIFM7 CH5 Switch", ABOX_SIFM_CH_SEL(7),
			ABOX_SIFM_CH_SEL_L(7) + 5, 1, 0),
	SOC_SINGLE("SIFM7 CH6 Switch", ABOX_SIFM_CH_SEL(7),
			ABOX_SIFM_CH_SEL_L(7) + 6, 1, 0),
	SOC_SINGLE("SIFM7 CH7 Switch", ABOX_SIFM_CH_SEL(7),
			ABOX_SIFM_CH_SEL_L(7) + 7, 1, 0),
	SOC_SINGLE("SIFM8 CH0 Switch", ABOX_SIFM_CH_SEL(8),
			ABOX_SIFM_CH_SEL_L(8) + 0, 1, 0),
	SOC_SINGLE("SIFM8 CH1 Switch", ABOX_SIFM_CH_SEL(8),
			ABOX_SIFM_CH_SEL_L(8) + 1, 1, 0),
	SOC_SINGLE("SIFM8 CH2 Switch", ABOX_SIFM_CH_SEL(8),
			ABOX_SIFM_CH_SEL_L(8) + 2, 1, 0),
	SOC_SINGLE("SIFM8 CH3 Switch", ABOX_SIFM_CH_SEL(8),
			ABOX_SIFM_CH_SEL_L(8) + 3, 1, 0),
	SOC_SINGLE("SIFM8 CH4 Switch", ABOX_SIFM_CH_SEL(8),
			ABOX_SIFM_CH_SEL_L(8) + 4, 1, 0),
	SOC_SINGLE("SIFM8 CH5 Switch", ABOX_SIFM_CH_SEL(8),
			ABOX_SIFM_CH_SEL_L(8) + 5, 1, 0),
	SOC_SINGLE("SIFM8 CH6 Switch", ABOX_SIFM_CH_SEL(8),
			ABOX_SIFM_CH_SEL_L(8) + 6, 1, 0),
	SOC_SINGLE("SIFM8 CH7 Switch", ABOX_SIFM_CH_SEL(8),
			ABOX_SIFM_CH_SEL_L(8) + 7, 1, 0),
	SOC_SINGLE("SIFM9 CH0 Switch", ABOX_SIFM_CH_SEL(9),
			ABOX_SIFM_CH_SEL_L(9) + 0, 1, 0),
	SOC_SINGLE("SIFM9 CH1 Switch", ABOX_SIFM_CH_SEL(9),
			ABOX_SIFM_CH_SEL_L(9) + 1, 1, 0),
	SOC_SINGLE("SIFM9 CH2 Switch", ABOX_SIFM_CH_SEL(9),
			ABOX_SIFM_CH_SEL_L(9) + 2, 1, 0),
	SOC_SINGLE("SIFM9 CH3 Switch", ABOX_SIFM_CH_SEL(9),
			ABOX_SIFM_CH_SEL_L(9) + 3, 1, 0),
	SOC_SINGLE("SIFM9 CH4 Switch", ABOX_SIFM_CH_SEL(9),
			ABOX_SIFM_CH_SEL_L(9) + 4, 1, 0),
	SOC_SINGLE("SIFM9 CH5 Switch", ABOX_SIFM_CH_SEL(9),
			ABOX_SIFM_CH_SEL_L(9) + 5, 1, 0),
	SOC_SINGLE("SIFM9 CH6 Switch", ABOX_SIFM_CH_SEL(9),
			ABOX_SIFM_CH_SEL_L(9) + 6, 1, 0),
	SOC_SINGLE("SIFM9 CH7 Switch", ABOX_SIFM_CH_SEL(9),
			ABOX_SIFM_CH_SEL_L(9) + 7, 1, 0),
	SOC_SINGLE("SIFM10 CH0 Switch", ABOX_SIFM_CH_SEL(10),
			ABOX_SIFM_CH_SEL_L(10) + 0, 1, 0),
	SOC_SINGLE("SIFM10 CH1 Switch", ABOX_SIFM_CH_SEL(10),
			ABOX_SIFM_CH_SEL_L(10) + 1, 1, 0),
	SOC_SINGLE("SIFM10 CH2 Switch", ABOX_SIFM_CH_SEL(10),
			ABOX_SIFM_CH_SEL_L(10) + 2, 1, 0),
	SOC_SINGLE("SIFM10 CH3 Switch", ABOX_SIFM_CH_SEL(10),
			ABOX_SIFM_CH_SEL_L(10) + 3, 1, 0),
	SOC_SINGLE("SIFM10 CH4 Switch", ABOX_SIFM_CH_SEL(10),
			ABOX_SIFM_CH_SEL_L(10) + 4, 1, 0),
	SOC_SINGLE("SIFM10 CH5 Switch", ABOX_SIFM_CH_SEL(10),
			ABOX_SIFM_CH_SEL_L(10) + 5, 1, 0),
	SOC_SINGLE("SIFM10 CH6 Switch", ABOX_SIFM_CH_SEL(10),
			ABOX_SIFM_CH_SEL_L(10) + 6, 1, 0),
	SOC_SINGLE("SIFM10 CH7 Switch", ABOX_SIFM_CH_SEL(10),
			ABOX_SIFM_CH_SEL_L(10) + 7, 1, 0),
	SOC_SINGLE("SIFM11 CH0 Switch", ABOX_SIFM_CH_SEL(11),
			ABOX_SIFM_CH_SEL_L(11) + 0, 1, 0),
	SOC_SINGLE("SIFM11 CH1 Switch", ABOX_SIFM_CH_SEL(11),
			ABOX_SIFM_CH_SEL_L(11) + 1, 1, 0),
	SOC_SINGLE("SIFM11 CH2 Switch", ABOX_SIFM_CH_SEL(11),
			ABOX_SIFM_CH_SEL_L(11) + 2, 1, 0),
	SOC_SINGLE("SIFM11 CH3 Switch", ABOX_SIFM_CH_SEL(11),
			ABOX_SIFM_CH_SEL_L(11) + 3, 1, 0),
	SOC_SINGLE("SIFM11 CH4 Switch", ABOX_SIFM_CH_SEL(11),
			ABOX_SIFM_CH_SEL_L(11) + 4, 1, 0),
	SOC_SINGLE("SIFM11 CH5 Switch", ABOX_SIFM_CH_SEL(11),
			ABOX_SIFM_CH_SEL_L(11) + 5, 1, 0),
	SOC_SINGLE("SIFM11 CH6 Switch", ABOX_SIFM_CH_SEL(11),
			ABOX_SIFM_CH_SEL_L(11) + 6, 1, 0),
	SOC_SINGLE("SIFM11 CH7 Switch", ABOX_SIFM_CH_SEL(11),
			ABOX_SIFM_CH_SEL_L(11) + 7, 1, 0),
	SOC_SINGLE("SIFM12 CH0 Switch", ABOX_SIFM_CH_SEL(12),
			ABOX_SIFM_CH_SEL_L(12) + 0, 1, 0),
	SOC_SINGLE("SIFM12 CH1 Switch", ABOX_SIFM_CH_SEL(12),
			ABOX_SIFM_CH_SEL_L(12) + 1, 1, 0),
	SOC_SINGLE("SIFM12 CH2 Switch", ABOX_SIFM_CH_SEL(12),
			ABOX_SIFM_CH_SEL_L(12) + 2, 1, 0),
	SOC_SINGLE("SIFM12 CH3 Switch", ABOX_SIFM_CH_SEL(12),
			ABOX_SIFM_CH_SEL_L(12) + 3, 1, 0),
	SOC_SINGLE("SIFM12 CH4 Switch", ABOX_SIFM_CH_SEL(12),
			ABOX_SIFM_CH_SEL_L(12) + 4, 1, 0),
	SOC_SINGLE("SIFM12 CH5 Switch", ABOX_SIFM_CH_SEL(12),
			ABOX_SIFM_CH_SEL_L(12) + 5, 1, 0),
	SOC_SINGLE("SIFM12 CH6 Switch", ABOX_SIFM_CH_SEL(12),
			ABOX_SIFM_CH_SEL_L(12) + 6, 1, 0),
	SOC_SINGLE("SIFM12 CH7 Switch", ABOX_SIFM_CH_SEL(12),
			ABOX_SIFM_CH_SEL_L(12) + 7, 1, 0),
	SOC_SINGLE("SIFM13 CH0 Switch", ABOX_SIFM_CH_SEL(13),
			ABOX_SIFM_CH_SEL_L(13) + 0, 1, 0),
	SOC_SINGLE("SIFM13 CH1 Switch", ABOX_SIFM_CH_SEL(13),
			ABOX_SIFM_CH_SEL_L(13) + 1, 1, 0),
	SOC_SINGLE("SIFM13 CH2 Switch", ABOX_SIFM_CH_SEL(13),
			ABOX_SIFM_CH_SEL_L(13) + 2, 1, 0),
	SOC_SINGLE("SIFM13 CH3 Switch", ABOX_SIFM_CH_SEL(13),
			ABOX_SIFM_CH_SEL_L(13) + 3, 1, 0),
	SOC_SINGLE("SIFM13 CH4 Switch", ABOX_SIFM_CH_SEL(13),
			ABOX_SIFM_CH_SEL_L(13) + 4, 1, 0),
	SOC_SINGLE("SIFM13 CH5 Switch", ABOX_SIFM_CH_SEL(13),
			ABOX_SIFM_CH_SEL_L(13) + 5, 1, 0),
	SOC_SINGLE("SIFM13 CH6 Switch", ABOX_SIFM_CH_SEL(13),
			ABOX_SIFM_CH_SEL_L(13) + 6, 1, 0),
	SOC_SINGLE("SIFM13 CH7 Switch", ABOX_SIFM_CH_SEL(13),
			ABOX_SIFM_CH_SEL_L(13) + 7, 1, 0),
	SOC_SINGLE("SIFM14 CH0 Switch", ABOX_SIFM_CH_SEL(14),
			ABOX_SIFM_CH_SEL_L(14) + 0, 1, 0),
	SOC_SINGLE("SIFM14 CH1 Switch", ABOX_SIFM_CH_SEL(14),
			ABOX_SIFM_CH_SEL_L(14) + 1, 1, 0),
	SOC_SINGLE("SIFM14 CH2 Switch", ABOX_SIFM_CH_SEL(14),
			ABOX_SIFM_CH_SEL_L(14) + 2, 1, 0),
	SOC_SINGLE("SIFM14 CH3 Switch", ABOX_SIFM_CH_SEL(14),
			ABOX_SIFM_CH_SEL_L(14) + 3, 1, 0),
	SOC_SINGLE("SIFM14 CH4 Switch", ABOX_SIFM_CH_SEL(14),
			ABOX_SIFM_CH_SEL_L(14) + 4, 1, 0),
	SOC_SINGLE("SIFM14 CH5 Switch", ABOX_SIFM_CH_SEL(14),
			ABOX_SIFM_CH_SEL_L(14) + 5, 1, 0),
	SOC_SINGLE("SIFM14 CH6 Switch", ABOX_SIFM_CH_SEL(14),
			ABOX_SIFM_CH_SEL_L(14) + 6, 1, 0),
	SOC_SINGLE("SIFM14 CH7 Switch", ABOX_SIFM_CH_SEL(14),
			ABOX_SIFM_CH_SEL_L(14) + 7, 1, 0),
	SOC_SINGLE("SIFM15 CH0 Switch", ABOX_SIFM_CH_SEL(15),
			ABOX_SIFM_CH_SEL_L(15) + 0, 1, 0),
	SOC_SINGLE("SIFM15 CH1 Switch", ABOX_SIFM_CH_SEL(15),
			ABOX_SIFM_CH_SEL_L(15) + 1, 1, 0),
	SOC_SINGLE("SIFM15 CH2 Switch", ABOX_SIFM_CH_SEL(15),
			ABOX_SIFM_CH_SEL_L(15) + 2, 1, 0),
	SOC_SINGLE("SIFM15 CH3 Switch", ABOX_SIFM_CH_SEL(15),
			ABOX_SIFM_CH_SEL_L(15) + 3, 1, 0),
	SOC_SINGLE("SIFM15 CH4 Switch", ABOX_SIFM_CH_SEL(15),
			ABOX_SIFM_CH_SEL_L(15) + 4, 1, 0),
	SOC_SINGLE("SIFM15 CH5 Switch", ABOX_SIFM_CH_SEL(15),
			ABOX_SIFM_CH_SEL_L(15) + 5, 1, 0),
	SOC_SINGLE("SIFM15 CH6 Switch", ABOX_SIFM_CH_SEL(15),
			ABOX_SIFM_CH_SEL_L(15) + 6, 1, 0),
	SOC_SINGLE("SIFM15 CH7 Switch", ABOX_SIFM_CH_SEL(15),
			ABOX_SIFM_CH_SEL_L(15) + 7, 1, 0),

	SOC_VALUE_ENUM_EXT("Audio Mode", audio_mode_enum,
			audio_mode_get, audio_mode_put),
	SOC_VALUE_ENUM_EXT("Sound Type", sound_type_enum,
			sound_type_get, sound_type_put),
	SOC_SINGLE_EXT("VDMA Buffer Status", 0, 0, VDMA_COUNT_MAX +
			PCMTASK_VDMA_ID_BASE - 1, 0, vdma_buffer_status_get, vdma_buffer_status_put),
	SOC_SINGLE_EXT("Tickle", 0, 0, 1, 0, tickle_get, tickle_put),
	SOC_SINGLE_EXT("Cold", 0, 0, 1, 0, cold_get, cold_put),
	SOC_ENUM_EXT("Debug", debug_enum, debug_get, debug_put),
	SOC_SINGLE_EXT("Wakelock", 0, 0, 1, 0, wake_lock_get, wake_lock_put),
	SOC_SINGLE_EXT("Reset Log", 0, 0, 1, 0, reset_log_get, reset_log_put),
	SOC_SINGLE_EXT("NSRC0 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(0), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC1 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(1), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC2 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(2), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC3 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(3), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC4 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(4), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC5 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(5), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC6 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(6), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC7 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(7), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC8 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(8), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC9 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(9), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC10 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(10), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC11 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(11), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC12 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(12), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC13 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(13), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC14 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(14), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("NSRC15 Bridge", ABOX_ROUTE_CTRL_CONNECT,
			ABOX_NSRC_CONNECTION_TYPE_L(15), 1, 0,
			nsrc_bridge_get, nsrc_bridge_put),
	SOC_SINGLE_EXT("ASRC Factor CP", SET_ASRC_FACTOR_CP, 0, 0x1ffff, 0,
			asrc_factor_get, asrc_factor_put),
	SOC_SINGLE("MIXP Dummy Start", ABOX_SPUS_CTRL_MIXP_DUMMY_START,
			ABOX_MIXP_DUMMY_START_L, 1, 0),
	SOC_SINGLE("SPUS TUNE0 Dummy Start", ABOX_SPUS_LATENCY_CTRL0,
			ABOX_TUNE0_DUMMY_START_L, 1, 0),
	SOC_SINGLE("SPUS TUNE1 Dummy Start", ABOX_SPUS_LATENCY_CTRL0,
			ABOX_TUNE1_DUMMY_START_L, 1, 0),
};

static const struct snd_kcontrol_new spus_asrc_dummy_start_controls[] = {
	SOC_SINGLE("SPUS ASRC0 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(0), 1, 0),
	SOC_SINGLE("SPUS ASRC1 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(1), 1, 0),
	SOC_SINGLE("SPUS ASRC2 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(2), 1, 0),
	SOC_SINGLE("SPUS ASRC3 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(3), 1, 0),
	SOC_SINGLE("SPUS ASRC4 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(4), 1, 0),
	SOC_SINGLE("SPUS ASRC5 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(5), 1, 0),
	SOC_SINGLE("SPUS ASRC6 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(6), 1, 0),
	SOC_SINGLE("SPUS ASRC7 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(7), 1, 0),
	SOC_SINGLE("SPUS ASRC8 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(8), 1, 0),
	SOC_SINGLE("SPUS ASRC9 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(9), 1, 0),
	SOC_SINGLE("SPUS ASRC10 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(10), 1, 0),
	SOC_SINGLE("SPUS ASRC11 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(11), 1, 0),
	SOC_SINGLE("SPUS ASRC12 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(12), 1, 0),
	SOC_SINGLE("SPUS ASRC13 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(13), 1, 0),
	SOC_SINGLE("SPUS ASRC14 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(14), 1, 0),
	SOC_SINGLE("SPUS ASRC15 Dummy Start",
			ABOX_SPUS_CTRL_RDMA_ASRC_DUMMY_START,
			ABOX_RDMA_ASRC_DUMMY_START_L(15), 1, 0),
};

static const struct snd_kcontrol_new spus_asrc_start_num_controls[] = {
	SOC_SINGLE("SPUS ASRC0 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(0),
			ABOX_RDMA_START_ASRC_NUM_L(0), 32, 0),
	SOC_SINGLE("SPUS ASRC1 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(0),
			ABOX_RDMA_START_ASRC_NUM_L(1), 32, 0),
	SOC_SINGLE("SPUS ASRC2 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(0),
			ABOX_RDMA_START_ASRC_NUM_L(2), 32, 0),
	SOC_SINGLE("SPUS ASRC3 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(3),
			ABOX_RDMA_START_ASRC_NUM_L(3), 32, 0),
	SOC_SINGLE("SPUS ASRC4 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(4),
			ABOX_RDMA_START_ASRC_NUM_L(4), 32, 0),
	SOC_SINGLE("SPUS ASRC5 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(5),
			ABOX_RDMA_START_ASRC_NUM_L(5), 32, 0),
	SOC_SINGLE("SPUS ASRC6 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(6),
			ABOX_RDMA_START_ASRC_NUM_L(6), 32, 0),
	SOC_SINGLE("SPUS ASRC7 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(7),
			ABOX_RDMA_START_ASRC_NUM_L(7), 32, 0),
	SOC_SINGLE("SPUS ASRC8 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(8),
			ABOX_RDMA_START_ASRC_NUM_L(8), 32, 0),
	SOC_SINGLE("SPUS ASRC9 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(9),
			ABOX_RDMA_START_ASRC_NUM_L(9), 32, 0),
	SOC_SINGLE("SPUS ASRC10 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(10),
			ABOX_RDMA_START_ASRC_NUM_L(10), 32, 0),
	SOC_SINGLE("SPUS ASRC11 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(11),
			ABOX_RDMA_START_ASRC_NUM_L(11), 32, 0),
	SOC_SINGLE("SPUS ASRC12 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(12),
			ABOX_RDMA_START_ASRC_NUM_L(12), 32, 0),
	SOC_SINGLE("SPUS ASRC13 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(13),
			ABOX_RDMA_START_ASRC_NUM_L(13), 32, 0),
	SOC_SINGLE("SPUS ASRC14 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(14),
			ABOX_RDMA_START_ASRC_NUM_L(14), 32, 0),
	SOC_SINGLE("SPUS ASRC15 Start Num",
			ABOX_SPUS_CTRL_RDMA_START_ASRC_NUM(15),
			ABOX_RDMA_START_ASRC_NUM_L(15), 32, 0),
};

static const struct snd_kcontrol_new spus_asrc_controls[] = {
	SOC_SINGLE_EXT("SPUS ASRC0", ABOX_SPUS_CTRL_FC_SRC(0),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(0), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC1", ABOX_SPUS_CTRL_FC_SRC(1),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(1), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC2", ABOX_SPUS_CTRL_FC_SRC(2),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(2), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC3", ABOX_SPUS_CTRL_FC_SRC(3),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(3), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC4", ABOX_SPUS_CTRL_FC_SRC(4),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(4), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC5", ABOX_SPUS_CTRL_FC_SRC(5),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(5), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC6", ABOX_SPUS_CTRL_FC_SRC(6),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(6), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC7", ABOX_SPUS_CTRL_FC_SRC(7),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(7), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC8", ABOX_SPUS_CTRL_FC_SRC(8),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(8), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC9", ABOX_SPUS_CTRL_FC_SRC(9),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(9), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC10", ABOX_SPUS_CTRL_FC_SRC(10),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(10), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC11", ABOX_SPUS_CTRL_FC_SRC(11),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(11), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC12", ABOX_SPUS_CTRL_FC_SRC(12),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(12), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC13", ABOX_SPUS_CTRL_FC_SRC(13),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(13), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC14", ABOX_SPUS_CTRL_FC_SRC(14),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(14), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
	SOC_SINGLE_EXT("SPUS ASRC15", ABOX_SPUS_CTRL_FC_SRC(15),
			ABOX_FUNC_CHAIN_SRC_ASRC_L(15), 1, 0,
			snd_soc_get_volsw, spus_asrc_enable_put),
};

static const struct snd_kcontrol_new spum_asrc_controls[] = {
	SOC_SINGLE_EXT("SPUM ASRC0", ABOX_SPUM_CTRL_FC_NSRC(0),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(0), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC1", ABOX_SPUM_CTRL_FC_NSRC(1),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(1), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC2", ABOX_SPUM_CTRL_FC_NSRC(2),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(2), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC3", ABOX_SPUM_CTRL_FC_NSRC(3),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(3), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC4", ABOX_SPUM_CTRL_FC_NSRC(4),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(4), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC5", ABOX_SPUM_CTRL_FC_NSRC(5),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(5), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC6", ABOX_SPUM_CTRL_FC_NSRC(6),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(6), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC7", ABOX_SPUM_CTRL_FC_NSRC(7),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(7), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC8", ABOX_SPUM_CTRL_FC_NSRC(8),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(8), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC9", ABOX_SPUM_CTRL_FC_NSRC(9),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(9), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC10", ABOX_SPUM_CTRL_FC_NSRC(10),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(10), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC11", ABOX_SPUM_CTRL_FC_NSRC(11),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(11), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC12", ABOX_SPUM_CTRL_FC_NSRC(12),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(12), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC13", ABOX_SPUM_CTRL_FC_NSRC(13),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(13), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC14", ABOX_SPUM_CTRL_FC_NSRC(14),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(14), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
	SOC_SINGLE_EXT("SPUM ASRC15", ABOX_SPUM_CTRL_FC_NSRC(15),
			ABOX_FUNC_CHAIN_NSRC_ASRC_L(15), 1, 0,
			snd_soc_get_volsw, spum_asrc_enable_put),
};

static const struct snd_kcontrol_new spus_asrc_id_controls[] = {
	SOC_SINGLE_EXT("SPUS ASRC0 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(0),
			ABOX_SRC_ASRC_ID_L(0), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC1 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(1),
			ABOX_SRC_ASRC_ID_L(1), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC2 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(2),
			ABOX_SRC_ASRC_ID_L(2), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC3 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(3),
			ABOX_SRC_ASRC_ID_L(3), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC4 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(4),
			ABOX_SRC_ASRC_ID_L(4), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC5 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(5),
			ABOX_SRC_ASRC_ID_L(5), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC6 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(6),
			ABOX_SRC_ASRC_ID_L(6), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC7 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(7),
			ABOX_SRC_ASRC_ID_L(7), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC8 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(8),
			ABOX_SRC_ASRC_ID_L(8), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC9 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(9),
			ABOX_SRC_ASRC_ID_L(9), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC10 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(10),
			ABOX_SRC_ASRC_ID_L(10), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC11 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(11),
			ABOX_SRC_ASRC_ID_L(11), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC12 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(12),
			ABOX_SRC_ASRC_ID_L(12), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC13 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(13),
			ABOX_SRC_ASRC_ID_L(13), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC14 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(14),
			ABOX_SRC_ASRC_ID_L(14), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
	SOC_SINGLE_EXT("SPUS ASRC15 ID", ABOX_SPUS_CTRL_SRC_ASRC_ID(15),
			ABOX_SRC_ASRC_ID_L(15), 15, 0,
			snd_soc_get_volsw, spus_asrc_id_put),
};

static const struct snd_kcontrol_new spum_asrc_id_controls[] = {
	SOC_SINGLE_EXT("SPUM ASRC0 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(0),
			ABOX_NSRC_ASRC_ID_L(0), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC1 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(1),
			ABOX_NSRC_ASRC_ID_L(1), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC2 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(2),
			ABOX_NSRC_ASRC_ID_L(2), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC3 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(3),
			ABOX_NSRC_ASRC_ID_L(3), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC4 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(4),
			ABOX_NSRC_ASRC_ID_L(4), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC5 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(5),
			ABOX_NSRC_ASRC_ID_L(5), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC6 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(6),
			ABOX_NSRC_ASRC_ID_L(6), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC7 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(7),
			ABOX_NSRC_ASRC_ID_L(7), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC8 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(8),
			ABOX_NSRC_ASRC_ID_L(8), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC9 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(9),
			ABOX_NSRC_ASRC_ID_L(9), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC10 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(10),
			ABOX_NSRC_ASRC_ID_L(10), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC11 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(11),
			ABOX_NSRC_ASRC_ID_L(11), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC12 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(12),
			ABOX_NSRC_ASRC_ID_L(12), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC13 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(13),
			ABOX_NSRC_ASRC_ID_L(13), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC14 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(14),
			ABOX_NSRC_ASRC_ID_L(14), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
	SOC_SINGLE_EXT("SPUM ASRC15 ID", ABOX_SPUM_CTRL_NSRC_ASRC_ID(15),
			ABOX_NSRC_ASRC_ID_L(15), 7, 0,
			snd_soc_get_volsw, spum_asrc_id_put),
};

static const struct snd_kcontrol_new spus_asrc_apf_coef_controls[] = {
	SOC_SINGLE_EXT("SPUS ASRC0 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			0, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC1 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			1, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC2 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			2, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC3 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			3, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC4 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			4, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC5 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			5, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC6 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			6, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC7 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			7, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC8 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			8, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC9 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			9, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC10 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			10, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC11 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			11, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC12 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			12, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC13 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			13, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC14 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			14, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUS ASRC15 APF COEF", SNDRV_PCM_STREAM_PLAYBACK,
			15, 1, 0, asrc_apf_coef_get, asrc_apf_coef_put),
};

static const struct snd_kcontrol_new spum_asrc_apf_coef_controls[] = {
	SOC_SINGLE_EXT("SPUM ASRC0 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 0, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC1 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 1, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC2 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 2, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC3 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 3, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC4 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 4, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC5 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 5, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC6 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 6, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC7 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 7, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC8 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 8, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC9 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 9, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC10 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 10, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC11 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 11, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC12 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 12, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC13 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 13, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC14 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 14, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
	SOC_SINGLE_EXT("SPUM ASRC15 APF COEF", SNDRV_PCM_STREAM_CAPTURE, 15, 1, 0,
			asrc_apf_coef_get, asrc_apf_coef_put),
};

static const char *const asrc_source_enum_texts[] = {
	"UAIF0",
	"UAIF1",
	"UAIF2",
	"UAIF3",
	"UAIF4",
	"UAIF5",
	"UAIF6",
	"UAIF7",
	"USB",
	"Ext CP",
	"Ext BCLK_CP",
	"CP",
	"PCM_CNTR",
	"BCLK_SPDY",
	"ABOX",
};

static const unsigned int asrc_source_enum_values[] = {
	TICK_UAIF0,
	TICK_UAIF1,
	TICK_UAIF2,
	TICK_UAIF3,
	TICK_UAIF4,
	TICK_UAIF5,
	TICK_UAIF6,
	TICK_UAIF7,
	TICK_USB,
	TICK_CP_EXT,
	TICK_BCLK_CP_EXT,
	TICK_CP,
	TICK_PCM_CNTR,
	TICK_BCLK_SPDY,
	TICK_SYNC,
};

static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc0_os_enum, 0, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc1_os_enum, 1, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc2_os_enum, 2, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc3_os_enum, 3, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc4_os_enum, 4, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc5_os_enum, 5, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc6_os_enum, 6, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc7_os_enum, 7, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc8_os_enum, 8, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc9_os_enum, 9, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc10_os_enum, 10, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc11_os_enum, 11, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc12_os_enum, 12, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc13_os_enum, 13, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc14_os_enum, 14, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc15_os_enum, 15, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);

static const struct snd_kcontrol_new spus_asrc_os_controls[] = {
	SOC_VALUE_ENUM_EXT("SPUS ASRC0 OS", spus_asrc0_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC1 OS", spus_asrc1_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC2 OS", spus_asrc2_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC3 OS", spus_asrc3_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC4 OS", spus_asrc4_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC5 OS", spus_asrc5_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC6 OS", spus_asrc6_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC7 OS", spus_asrc7_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC8 OS", spus_asrc8_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC9 OS", spus_asrc9_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC10 OS", spus_asrc10_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC11 OS", spus_asrc11_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC12 OS", spus_asrc12_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC13 OS", spus_asrc13_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC14 OS", spus_asrc14_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC15 OS", spus_asrc15_os_enum,
			spus_asrc_os_get, spus_asrc_os_put),
};

static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc0_is_enum, 0, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc1_is_enum, 1, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc2_is_enum, 2, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc3_is_enum, 3, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc4_is_enum, 4, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc5_is_enum, 5, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc6_is_enum, 6, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc7_is_enum, 7, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc8_is_enum, 8, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc9_is_enum, 9, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc10_is_enum, 10, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc11_is_enum, 11, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc12_is_enum, 12, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc13_is_enum, 13, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc14_is_enum, 14, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spus_asrc15_is_enum, 15, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);

static const struct snd_kcontrol_new spus_asrc_is_controls[] = {
	SOC_VALUE_ENUM_EXT("SPUS ASRC0 IS", spus_asrc0_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC1 IS", spus_asrc1_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC2 IS", spus_asrc2_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC3 IS", spus_asrc3_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC4 IS", spus_asrc4_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC5 IS", spus_asrc5_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC6 IS", spus_asrc6_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC7 IS", spus_asrc7_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC8 IS", spus_asrc8_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC9 IS", spus_asrc9_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC10 IS", spus_asrc10_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC11 IS", spus_asrc11_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC12 IS", spus_asrc12_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC13 IS", spus_asrc13_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC14 IS", spus_asrc14_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUS ASRC15 IS", spus_asrc15_is_enum,
			spus_asrc_is_get, spus_asrc_is_put),
};

static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc0_os_enum, 0, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc1_os_enum, 1, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc2_os_enum, 2, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc3_os_enum, 3, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc4_os_enum, 4, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc5_os_enum, 5, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc6_os_enum, 6, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc7_os_enum, 7, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc8_os_enum, 8, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc9_os_enum, 9, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc10_os_enum, 10, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc11_os_enum, 11, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc12_os_enum, 12, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc13_os_enum, 13, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc14_os_enum, 14, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc15_os_enum, 15, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);

static const struct snd_kcontrol_new spum_asrc_os_controls[] = {
	SOC_VALUE_ENUM_EXT("SPUM ASRC0 OS", spum_asrc0_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC1 OS", spum_asrc1_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC2 OS", spum_asrc2_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC3 OS", spum_asrc3_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC4 OS", spum_asrc4_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC5 OS", spum_asrc5_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC6 OS", spum_asrc6_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC7 OS", spum_asrc7_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC8 OS", spum_asrc8_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC9 OS", spum_asrc9_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC10 OS", spum_asrc10_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC11 OS", spum_asrc11_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC12 OS", spum_asrc12_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC13 OS", spum_asrc13_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC14 OS", spum_asrc14_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC15 OS", spum_asrc15_os_enum,
			spum_asrc_os_get, spum_asrc_os_put),
};

static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc0_is_enum, 0, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc1_is_enum, 1, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc2_is_enum, 2, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc3_is_enum, 3, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc4_is_enum, 4, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc5_is_enum, 5, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc6_is_enum, 6, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc7_is_enum, 7, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc8_is_enum, 8, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc9_is_enum, 9, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc10_is_enum, 10, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc11_is_enum, 11, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc12_is_enum, 12, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc13_is_enum, 13, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc14_is_enum, 14, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);
static SOC_VALUE_ENUM_SINGLE_DECL(spum_asrc15_is_enum, 15, 0, 0,
		asrc_source_enum_texts, asrc_source_enum_values);

static const struct snd_kcontrol_new spum_asrc_is_controls[] = {
	SOC_VALUE_ENUM_EXT("SPUM ASRC0 IS", spum_asrc0_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC1 IS", spum_asrc1_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC2 IS", spum_asrc2_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC3 IS", spum_asrc3_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC4 IS", spum_asrc4_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC5 IS", spum_asrc5_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC6 IS", spum_asrc6_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC7 IS", spum_asrc7_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC8 IS", spum_asrc8_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC9 IS", spum_asrc9_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC10 IS", spum_asrc10_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC11 IS", spum_asrc11_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC12 IS", spum_asrc12_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC13 IS", spum_asrc13_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC14 IS", spum_asrc14_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
	SOC_VALUE_ENUM_EXT("SPUM ASRC15 IS", spum_asrc15_is_enum,
			spum_asrc_is_get, spum_asrc_is_put),
};

struct snd_soc_dapm_widget *spus_asrc_widget_cache[ARRAY_SIZE(spus_asrc_controls)];
struct snd_soc_dapm_widget *spum_asrc_widget_cache[ARRAY_SIZE(spum_asrc_controls)];

static void asrc_cache_widget(struct snd_soc_dapm_widget *w,
		int idx, int stream)
{
	struct device *dev = w->dapm->dev;

	abox_dbg(dev, "%s(%s, %d, %d)\n", __func__, w->name, idx, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		spus_asrc_widget_cache[idx] = w;
	else
		spum_asrc_widget_cache[idx] = w;
}

static struct snd_soc_dapm_widget *asrc_find_widget(
		struct snd_soc_card *card, int idx, int stream)
{
	struct device *dev = card->dev;
	struct snd_soc_dapm_widget *w;
	const char *name;

	dev_dbg(dev, "%s(%s, %d, %d)\n", __func__, card->name, idx, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		name = spus_asrc_controls[idx].name;
	else
		name = spum_asrc_controls[idx].name;

	list_for_each_entry(w, &card->widgets, list) {
		struct snd_soc_component *cmpnt = w->dapm->component;
		const char *name_prefix = cmpnt ? cmpnt->name_prefix : NULL;
		size_t prefix_len = name_prefix ? strlen(name_prefix) + 1 : 0;
		const char *w_name = w->name + prefix_len;

		if (!strcmp(name, w_name))
			return w;
	}

	return NULL;
}

static struct snd_soc_dapm_widget *asrc_get_widget(
		struct snd_soc_component *cmpnt, int idx, int stream)
{
	struct snd_soc_dapm_widget *w;
	struct device *dev;

	if (idx < 0)
		return NULL;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		w = spus_asrc_widget_cache[idx];
	else
		w = spum_asrc_widget_cache[idx];

	if (!w) {
		w = asrc_find_widget(cmpnt->card, idx, stream);
		asrc_cache_widget(w, idx, stream);
	}

	dev = w->dapm->dev;
	abox_dbg(dev, "%s(%d, %d): %s\n", __func__, idx, stream, w->name);

	return w;
}

static enum abox_dai spus_sink[COUNT_SPUS];
static int write_spus_sink(unsigned int id, enum abox_dai dai)
{
	if (id >= ARRAY_SIZE(spus_sink))
		return -EINVAL;

	spus_sink[id] = dai;
	return 0;
}
static int read_spus_sink(int id, enum abox_dai *dai)
{
	if (id >= ARRAY_SIZE(spus_sink))
		return -EINVAL;

	*dai = spus_sink[id];
	return 0;
}
static int set_spus_sink(struct snd_soc_component *cmpnt, enum abox_dai spus,
		enum abox_dai dai)
{
	struct device *dev = cmpnt->dev;
	int ret;

	abox_dbg(dev, "%s(%#x, %#x)\n", __func__, spus, dai);

	switch (spus) {
	case ABOX_RDMA0:
	case ABOX_RDMA1:
	case ABOX_RDMA2:
	case ABOX_RDMA3:
	case ABOX_RDMA4:
	case ABOX_RDMA5:
	case ABOX_RDMA6:
	case ABOX_RDMA7:
	case ABOX_RDMA8:
	case ABOX_RDMA9:
	case ABOX_RDMA10:
	case ABOX_RDMA11:
	case ABOX_RDMA12:
	case ABOX_RDMA13:
	case ABOX_RDMA14:
	case ABOX_RDMA15:
		ret = write_spus_sink(spus - ABOX_RDMA0, dai);
		break;
	case ABOX_RDMA0_BE:
	case ABOX_RDMA1_BE:
	case ABOX_RDMA2_BE:
	case ABOX_RDMA3_BE:
	case ABOX_RDMA4_BE:
	case ABOX_RDMA5_BE:
	case ABOX_RDMA6_BE:
	case ABOX_RDMA7_BE:
	case ABOX_RDMA8_BE:
	case ABOX_RDMA9_BE:
	case ABOX_RDMA10_BE:
	case ABOX_RDMA11_BE:
	case ABOX_RDMA12_BE:
	case ABOX_RDMA13_BE:
	case ABOX_RDMA14_BE:
	case ABOX_RDMA15_BE:
		ret = write_spus_sink(spus - ABOX_RDMA0_BE, dai);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret < 0)
		abox_err(dev, "%s: invalid dai %#x\n", __func__, spus);

	return ret;
}
static enum abox_dai get_spus_sink(struct snd_soc_component *cmpnt,
		enum abox_dai spus)
{
	struct device *dev = cmpnt->dev;
	enum abox_dai dai = ABOX_NONE;
	int ret;

	switch (spus) {
	case ABOX_RDMA0:
	case ABOX_RDMA1:
	case ABOX_RDMA2:
	case ABOX_RDMA3:
	case ABOX_RDMA4:
	case ABOX_RDMA5:
	case ABOX_RDMA6:
	case ABOX_RDMA7:
	case ABOX_RDMA8:
	case ABOX_RDMA9:
	case ABOX_RDMA10:
	case ABOX_RDMA11:
	case ABOX_RDMA12:
	case ABOX_RDMA13:
	case ABOX_RDMA14:
	case ABOX_RDMA15:
		ret = read_spus_sink(spus - ABOX_RDMA0, &dai);
		break;
	case ABOX_RDMA0_BE:
	case ABOX_RDMA1_BE:
	case ABOX_RDMA2_BE:
	case ABOX_RDMA3_BE:
	case ABOX_RDMA4_BE:
	case ABOX_RDMA5_BE:
	case ABOX_RDMA6_BE:
	case ABOX_RDMA7_BE:
	case ABOX_RDMA8_BE:
	case ABOX_RDMA9_BE:
	case ABOX_RDMA10_BE:
	case ABOX_RDMA11_BE:
	case ABOX_RDMA12_BE:
	case ABOX_RDMA13_BE:
	case ABOX_RDMA14_BE:
	case ABOX_RDMA15_BE:
		ret = read_spus_sink(spus - ABOX_RDMA0_BE, &dai);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	abox_dbg(dev, "%s(%#x): %#x\n", __func__, spus, dai);

	if (ret < 0)
		abox_err(dev, "%s: invalid dai %#x\n", __func__, spus);

	return dai;
}

static unsigned int nsrc_source[COUNT_SIFM];
static int write_nsrc_source(unsigned int id, enum abox_dai dai)
{
	if (id >= ARRAY_SIZE(nsrc_source))
		return -EINVAL;

	nsrc_source[id] = dai;
	return 0;
}
static int read_nsrc_source(int id, enum abox_dai *dai)
{
	if (id >= ARRAY_SIZE(nsrc_source))
		return -EINVAL;

	*dai = nsrc_source[id];
	return 0;
}
static int set_nsrc_source(struct snd_soc_component *cmpnt, enum abox_dai nsrc,
		enum abox_dai dai)
{
	struct device *dev = cmpnt->dev;
	int ret;

	abox_dbg(dev, "%s(%#x, %#x)\n", __func__, nsrc, dai);

	switch (nsrc) {
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
	case ABOX_NSRC7:
	case ABOX_NSRC8:
	case ABOX_NSRC9:
	case ABOX_NSRC10:
	case ABOX_NSRC11:
	case ABOX_NSRC12:
	case ABOX_NSRC13:
	case ABOX_NSRC14:
	case ABOX_NSRC15:
		ret = write_nsrc_source(nsrc - ABOX_NSRC0, dai);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret < 0)
		abox_err(dev, "%s: invalid dai %#x\n", __func__, nsrc);

	return ret;
}
static enum abox_dai get_nsrc_source(struct snd_soc_component *cmpnt,
		enum abox_dai nsrc)
{
	struct device *dev = cmpnt->dev;
	enum abox_dai dai = ABOX_NONE;
	int ret;

	switch (nsrc) {
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
	case ABOX_NSRC7:
	case ABOX_NSRC8:
	case ABOX_NSRC9:
	case ABOX_NSRC10:
	case ABOX_NSRC11:
	case ABOX_NSRC12:
	case ABOX_NSRC13:
	case ABOX_NSRC14:
	case ABOX_NSRC15:
		ret = read_nsrc_source(nsrc - ABOX_NSRC0, &dai);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	abox_dbg(dev, "%s(%#x): %#x\n", __func__, nsrc, dai);

	if (ret < 0)
		abox_err(dev, "%s: invalid dai %#x\n", __func__, nsrc);

	return dai;
}

static bool is_direct_connection(struct snd_soc_component *cmpnt,
		enum abox_dai dai)
{
	bool ret;

	switch (dai) {
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
	case ABOX_NSRC7:
	case ABOX_NSRC8:
	case ABOX_NSRC9:
	case ABOX_NSRC10:
	case ABOX_NSRC11:
	case ABOX_NSRC12:
	case ABOX_NSRC13:
	case ABOX_NSRC14:
	case ABOX_NSRC15:
		ret = !nsrc_bridge[dai - ABOX_NSRC0];
		break;
	default:
		ret = false;
		break;
	}

	return ret;
}

static enum abox_dai get_source_dai_id(struct abox_data *data, enum abox_dai id)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	unsigned int val = 0, idx, reg;
	enum abox_dai _id, ret = ABOX_NONE;

	switch (id) {
	case ABOX_WDMA0:
	case ABOX_WDMA0_BE:
		ret = ABOX_NSRC0;
		break;
	case ABOX_WDMA1:
	case ABOX_WDMA1_BE:
		ret = ABOX_NSRC1;
		break;
	case ABOX_WDMA2:
	case ABOX_WDMA2_BE:
		ret = ABOX_NSRC2;
		break;
	case ABOX_WDMA3:
	case ABOX_WDMA3_BE:
		ret = ABOX_NSRC3;
		break;
	case ABOX_WDMA4:
	case ABOX_WDMA4_BE:
		ret = ABOX_NSRC4;
		break;
	case ABOX_WDMA5:
	case ABOX_WDMA5_BE:
		ret = ABOX_NSRC5;
		break;
	case ABOX_WDMA6:
	case ABOX_WDMA6_BE:
		ret = ABOX_NSRC6;
		break;
	case ABOX_WDMA7:
	case ABOX_WDMA7_BE:
		ret = ABOX_NSRC7;
		break;
	case ABOX_WDMA8:
	case ABOX_WDMA8_BE:
		ret = ABOX_NSRC8;
		break;
	case ABOX_WDMA9:
	case ABOX_WDMA9_BE:
		ret = ABOX_NSRC9;
		break;
	case ABOX_WDMA10:
	case ABOX_WDMA10_BE:
		ret = ABOX_NSRC10;
		break;
	case ABOX_WDMA11:
	case ABOX_WDMA11_BE:
		ret = ABOX_NSRC11;
		break;
	case ABOX_WDMA12:
	case ABOX_WDMA12_BE:
		ret = ABOX_NSRC12;
		break;
	case ABOX_WDMA13:
	case ABOX_WDMA13_BE:
		ret = ABOX_NSRC13;
		break;
	case ABOX_WDMA14:
	case ABOX_WDMA14_BE:
		ret = ABOX_NSRC14;
		break;
	case ABOX_WDMA15:
	case ABOX_WDMA15_BE:
		ret = ABOX_NSRC15;
		break;
	case ABOX_UAIF0:
	case ABOX_UAIF1:
	case ABOX_UAIF2:
	case ABOX_UAIF3:
	case ABOX_UAIF4:
	case ABOX_UAIF5:
	case ABOX_UAIF6:
		idx = id - ABOX_UAIF0;
		reg = ABOX_ROUTE_CTRL_UAIF_SPK(idx);
		val = snd_soc_component_read(cmpnt, reg);
		val &= ABOX_ROUTE_UAIF_SPK_MASK(idx);
		val >>= ABOX_ROUTE_UAIF_SPK_L(idx);
		switch (val) {
		case 0x1:
			ret = ABOX_SIFS0;
			break;
		case 0x2:
			ret = ABOX_SIFS1;
			break;
		case 0x3:
			ret = ABOX_SIFS2;
			break;
		case 0x4:
			ret = ABOX_SIFS3;
			break;
		case 0x5:
			ret = ABOX_SIFS4;
			break;
		case 0x6:
			ret = ABOX_SIFS5;
			break;
		case 0x7:
			ret = ABOX_SIFS6;
			break;
		case 0x8:
			ret = ABOX_SIFS7;
			break;
		default:
			ret = ABOX_NONE;
			break;
		}
		break;
	case ABOX_DSIF:
		val = snd_soc_component_read(cmpnt, ABOX_ROUTE_CTRL_DSIF);
		val &= ABOX_ROUTE_DSIF_MASK;
		val >>= ABOX_ROUTE_DSIF_L;
		switch (val) {
		case 0x2:
			ret = ABOX_SIFS1;
			break;
		case 0x3:
			ret = ABOX_SIFS2;
			break;
		case 0x4:
			ret = ABOX_SIFS3;
			break;
		case 0x5:
			ret = ABOX_SIFS4;
			break;
		case 0x6:
			ret = ABOX_SIFS5;
			break;
		case 0x7:
			ret = ABOX_SIFS6;
			break;
		case 0x8:
			ret = ABOX_SIFS7;
			break;
		default:
			ret = ABOX_NONE;
			break;
		}
		break;
	case ABOX_SIFS0:
	case ABOX_SIFS1:
	case ABOX_SIFS2:
	case ABOX_SIFS3:
	case ABOX_SIFS4:
	case ABOX_SIFS5:
	case ABOX_SIFS6:
	case ABOX_SIFS7:
		for (_id = ABOX_RDMA0; _id <= ABOX_RDMA15; _id++) {
			if (get_sink_dai_id(data, _id) == id) {
				ret = _id;
				break;
			}
		}
		break;
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
	case ABOX_NSRC7:
	case ABOX_NSRC8:
	case ABOX_NSRC9:
	case ABOX_NSRC10:
	case ABOX_NSRC11:
	case ABOX_NSRC12:
	case ABOX_NSRC13:
	case ABOX_NSRC14:
	case ABOX_NSRC15:
		/* Autodisable widget set SFR after DAPM power up. */
		ret = get_nsrc_source(cmpnt, id);
		break;
	case ABOX_UDMA_WR0:
	case ABOX_UDMA_WR1:
		idx = id - ABOX_UDMA_WR0;
		reg = ABOX_ROUTE_UDMA_SIFM;
		val = snd_soc_component_read(cmpnt, reg);
		val &= ABOX_ROUTE_UDMA_SIFM_MASK(idx);
		val >>= ABOX_ROUTE_UDMA_SIFM_L(idx);
		switch (val) {
		case 0x1:
			ret = ABOX_SIFS0;
			break;
		case 0x2:
			ret = ABOX_SIFS1;
			break;
		case 0x3:
			ret = ABOX_SIFS2;
			break;
		case 0x4:
			ret = ABOX_SIFS3;
			break;
		case 0x5:
			ret = ABOX_SIFS4;
			break;
		case 0x6:
			ret = ABOX_SIFS5;
			break;
		case 0x7:
			ret = ABOX_SIFS6;
			break;
		case 0x8:
			ret = ABOX_SIFS7;
			break;
		default:
			ret = ABOX_NONE;
			break;
		}
		break;
	case ABOX_RDMA0:
	case ABOX_RDMA1:
	case ABOX_RDMA2:
	case ABOX_RDMA3:
	case ABOX_RDMA4:
	case ABOX_RDMA5:
	case ABOX_RDMA6:
	case ABOX_RDMA7:
	case ABOX_RDMA8:
	case ABOX_RDMA9:
	case ABOX_RDMA10:
	case ABOX_RDMA11:
	case ABOX_RDMA12:
	case ABOX_RDMA13:
	case ABOX_RDMA14:
	case ABOX_RDMA15:
	case ABOX_RDMA0_BE:
	case ABOX_RDMA1_BE:
	case ABOX_RDMA2_BE:
	case ABOX_RDMA3_BE:
	case ABOX_RDMA4_BE:
	case ABOX_RDMA5_BE:
	case ABOX_RDMA6_BE:
	case ABOX_RDMA7_BE:
	case ABOX_RDMA8_BE:
	case ABOX_RDMA9_BE:
	case ABOX_RDMA10_BE:
	case ABOX_RDMA11_BE:
	case ABOX_RDMA12_BE:
	case ABOX_RDMA13_BE:
	case ABOX_RDMA14_BE:
	case ABOX_RDMA15_BE:
		idx = id - ((id < ABOX_RDMA0_BE) ? ABOX_RDMA0 : ABOX_RDMA0_BE);
		reg = ABOX_SPUS_CTRL_FC_SRC(idx);
		val = snd_soc_component_read(cmpnt, reg);
		val &= ABOX_FUNC_CHAIN_SRC_IN_MASK(idx);
		val >>= ABOX_FUNC_CHAIN_SRC_IN_L(idx);
		switch (val) {
		case 0x1:
			ret = ABOX_SIFSM;
			break;
		case 0x3:
			ret = ABOX_SIFST;
			break;
		default:
			ret = ABOX_NONE;
			break;
		}
		break;
	case ABOX_SIFSM:
		val = snd_soc_component_read(cmpnt, ABOX_ROUTE_CTRL_SIFM);
		val &= ABOX_ROUTE_SPUSM_MASK;
		val >>= ABOX_ROUTE_SPUSM_L;
		switch (val) {
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
		case 0xc:
		case 0xd:
		case 0xe:
			ret = ABOX_UAIF0 + val - 0x8;
			break;
		default:
			ret = ABOX_NONE;
			break;
		}
		break;
	case ABOX_SIFST:
		val = snd_soc_component_read(cmpnt, ABOX_ROUTE_CTRL_SIFT);
		val &= ABOX_ROUTE_SPUST_MASK;
		val >>= ABOX_ROUTE_SPUST_L;
		switch (val) {
		case 0x8:
		case 0x9:
		case 0xa:
		case 0xb:
		case 0xc:
		case 0xd:
		case 0xe:
			ret = ABOX_UAIF0 + val - 0x8;
			break;
		default:
			ret = ABOX_NONE;
			break;
		}
		break;
	default:
		ret = ABOX_NONE;
		break;
	}

	return ret;
}

static enum abox_dai get_sink_dai_id(struct abox_data *data, enum abox_dai id)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	enum abox_dai _id, ret = ABOX_NONE;

	switch (id) {
	case ABOX_RDMA0:
	case ABOX_RDMA1:
	case ABOX_RDMA2:
	case ABOX_RDMA3:
	case ABOX_RDMA4:
	case ABOX_RDMA5:
	case ABOX_RDMA6:
	case ABOX_RDMA7:
	case ABOX_RDMA8:
	case ABOX_RDMA9:
	case ABOX_RDMA10:
	case ABOX_RDMA11:
	case ABOX_RDMA12:
	case ABOX_RDMA13:
	case ABOX_RDMA14:
	case ABOX_RDMA15:
	case ABOX_RDMA0_BE:
	case ABOX_RDMA1_BE:
	case ABOX_RDMA2_BE:
	case ABOX_RDMA3_BE:
	case ABOX_RDMA4_BE:
	case ABOX_RDMA5_BE:
	case ABOX_RDMA6_BE:
	case ABOX_RDMA7_BE:
	case ABOX_RDMA8_BE:
	case ABOX_RDMA9_BE:
	case ABOX_RDMA10_BE:
	case ABOX_RDMA11_BE:
	case ABOX_RDMA12_BE:
	case ABOX_RDMA13_BE:
	case ABOX_RDMA14_BE:
	case ABOX_RDMA15_BE:
		/* Autodisable widget set SFR after DAPM power up. */
		ret = get_spus_sink(cmpnt, id);
		break;
	case ABOX_UAIF0:
	case ABOX_UAIF1:
	case ABOX_UAIF2:
	case ABOX_UAIF3:
	case ABOX_UAIF4:
	case ABOX_UAIF5:
	case ABOX_UAIF6:
	case ABOX_SPDY:
		for (_id = ABOX_NSRC0; _id < ABOX_NSRC0 + COUNT_SPUM; _id++) {
			if (get_source_dai_id(data, _id) == id &&
					is_direct_connection(cmpnt, _id)) {
				ret = _id;
				break;
			}
		}
		break;
	case ABOX_UDMA_RD0:
	case ABOX_UDMA_RD1:
		for (_id = ABOX_NSRC0; _id < ABOX_NSRC0 + COUNT_SPUM; _id++) {
			if (get_source_dai_id(data, _id) == id) {
				ret = _id;
				break;
			}
		}
		break;
	case ABOX_SIFS0:
	case ABOX_SIFS1:
	case ABOX_SIFS2:
	case ABOX_SIFS3:
	case ABOX_SIFS4:
	case ABOX_SIFS5:
	case ABOX_SIFS6:
	case ABOX_SIFS7:
		for (_id = ABOX_UAIF0; _id <= ABOX_DSIF; _id++) {
			if (get_source_dai_id(data, _id) == id) {
				ret = _id;
				break;
			}
		}
		if (ret != ABOX_NONE)
			break;

		for (_id = ABOX_NSRC0; _id < ABOX_NSRC0 + COUNT_SPUM; _id++) {
			if (get_source_dai_id(data, _id) == id &&
					is_direct_connection(cmpnt, _id)) {
				ret = _id;
				break;
			}
		}
		if (ret != ABOX_NONE)
			break;

		for (_id = ABOX_UDMA_WR0; _id <= ABOX_UDMA_WR0 + COUNT_UDMA_SIFM; _id++) {
			if (get_source_dai_id(data, _id) == id) {
				ret = _id;
				break;
			}
		}
		break;
	case ABOX_NSRC0:
		ret = ABOX_WDMA0;
		break;
	case ABOX_NSRC1:
		ret = ABOX_WDMA1;
		break;
	case ABOX_NSRC2:
		ret = ABOX_WDMA2;
		break;
	case ABOX_NSRC3:
		ret = ABOX_WDMA3;
		break;
	case ABOX_NSRC4:
		ret = ABOX_WDMA4;
		break;
	case ABOX_NSRC5:
		ret = ABOX_WDMA5;
		break;
	case ABOX_NSRC6:
		ret = ABOX_WDMA6;
		break;
	case ABOX_NSRC7:
		ret = ABOX_WDMA7;
		break;
	case ABOX_NSRC8:
		ret = ABOX_WDMA8;
		break;
	case ABOX_NSRC9:
		ret = ABOX_WDMA9;
		break;
	case ABOX_NSRC10:
		ret = ABOX_WDMA10;
		break;
	case ABOX_NSRC11:
		ret = ABOX_WDMA11;
		break;
	case ABOX_NSRC12:
		ret = ABOX_WDMA12;
		break;
	case ABOX_NSRC13:
		ret = ABOX_WDMA13;
		break;
	case ABOX_NSRC14:
		ret = ABOX_WDMA14;
		break;
	case ABOX_NSRC15:
		ret = ABOX_WDMA15;
		break;
	default:
		ret = ABOX_NONE;
		break;
	}

	return ret;
}

static bool is_nsrc_connected(struct abox_data *data, enum abox_dai id)
{
	enum abox_dai _id;
	bool ret = false;

	switch (id) {
	case ABOX_SIFS0:
	case ABOX_SIFS1:
	case ABOX_SIFS2:
	case ABOX_SIFS3:
	case ABOX_SIFS4:
	case ABOX_SIFS5:
	case ABOX_SIFS6:
	case ABOX_SIFS7:
		for (_id = ABOX_NSRC0; _id < ABOX_NSRC0 + COUNT_SPUM; _id++) {
			if (get_source_dai_id(data, _id) == id) {
				ret = true;
				break;
			}
		}
		break;
	default:
		break;
	}

	return ret;
}

static struct snd_soc_dai *find_dai(struct snd_soc_card *card, enum abox_dai id)
{
	struct snd_soc_pcm_runtime *rtd;
	struct snd_soc_dai *cpu_dai;

	list_for_each_entry(rtd, &card->rtd_list, list) {
		cpu_dai = asoc_rtd_to_cpu(rtd, 0);
		if (cpu_dai->id == id)
			return cpu_dai;
	}

	return NULL;
}

static int get_configmsg(enum abox_dai id, enum ABOX_CONFIGMSG *rate,
		enum ABOX_CONFIGMSG *format)
{
	int ret = 0;

	switch (id) {
	case ABOX_SIFS0:
		*rate = SET_SIFS0_RATE;
		*format = SET_SIFS0_FORMAT;
		break;
	case ABOX_SIFS1:
		*rate = SET_SIFS1_RATE;
		*format = SET_SIFS1_FORMAT;
		break;
	case ABOX_SIFS2:
		*rate = SET_SIFS2_RATE;
		*format = SET_SIFS2_FORMAT;
		break;
	case ABOX_SIFS3:
		*rate = SET_SIFS3_RATE;
		*format = SET_SIFS3_FORMAT;
		break;
	case ABOX_SIFS4:
		*rate = SET_SIFS4_RATE;
		*format = SET_SIFS4_FORMAT;
		break;
	case ABOX_SIFS5:
		*rate = SET_SIFS5_RATE;
		*format = SET_SIFS5_FORMAT;
		break;
	case ABOX_SIFS6:
		*rate = SET_SIFS6_RATE;
		*format = SET_SIFS6_FORMAT;
		break;
	case ABOX_SIFS7:
		*rate = SET_SIFS7_RATE;
		*format = SET_SIFS7_FORMAT;
		break;
	case ABOX_NSRC0:
		*rate = SET_SIFM0_RATE;
		*format = SET_SIFM0_FORMAT;
		break;
	case ABOX_NSRC1:
		*rate = SET_SIFM1_RATE;
		*format = SET_SIFM1_FORMAT;
		break;
	case ABOX_NSRC2:
		*rate = SET_SIFM2_RATE;
		*format = SET_SIFM2_FORMAT;
		break;
	case ABOX_NSRC3:
		*rate = SET_SIFM3_RATE;
		*format = SET_SIFM3_FORMAT;
		break;
	case ABOX_NSRC4:
		*rate = SET_SIFM4_RATE;
		*format = SET_SIFM4_FORMAT;
		break;
	case ABOX_NSRC5:
		*rate = SET_SIFM5_RATE;
		*format = SET_SIFM5_FORMAT;
		break;
	case ABOX_NSRC6:
		*rate = SET_SIFM6_RATE;
		*format = SET_SIFM6_FORMAT;
		break;
	case ABOX_NSRC7:
		*rate = SET_SIFM7_RATE;
		*format = SET_SIFM7_FORMAT;
		break;
	case ABOX_NSRC8:
		*rate = SET_SIFM8_RATE;
		*format = SET_SIFM8_FORMAT;
		break;
	case ABOX_NSRC9:
		*rate = SET_SIFM9_RATE;
		*format = SET_SIFM9_FORMAT;
		break;
	case ABOX_NSRC10:
		*rate = SET_SIFM10_RATE;
		*format = SET_SIFM10_FORMAT;
		break;
	case ABOX_NSRC11:
		*rate = SET_SIFM11_RATE;
		*format = SET_SIFM11_FORMAT;
		break;
	case ABOX_NSRC12:
		*rate = SET_SIFM12_RATE;
		*format = SET_SIFM12_FORMAT;
		break;
	case ABOX_NSRC13:
		*rate = SET_SIFM13_RATE;
		*format = SET_SIFM13_FORMAT;
		break;
	case ABOX_NSRC14:
		*rate = SET_SIFM14_RATE;
		*format = SET_SIFM14_FORMAT;
		break;
	case ABOX_NSRC15:
		*rate = SET_SIFM15_RATE;
		*format = SET_SIFM15_FORMAT;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int set_sif_params(struct abox_data *data, enum abox_dai id,
		const struct snd_pcm_hw_params *params)
{
	struct device *adev = data->dev;
	enum ABOX_CONFIGMSG msg_rate, msg_format;
	unsigned int rate, channels;
	snd_pcm_format_t format;
	int mixp_channels, ret = 0;

	ret = get_configmsg(id, &msg_rate, &msg_format);
	if (ret < 0) {
		abox_err(adev, "can't set sif params: %d\n", ret);
		return ret;
	}

	rate = params_rate(params);
	format = params_format(params);
	channels = params_channels(params);

	if (get_sif_rate(data, msg_rate) != rate) {
		set_sif_rate(data, msg_rate, rate);
		rate_put_ipc(adev, rate, msg_rate);
	}

	if (get_sif_channels(data, msg_format) != channels ||
			get_sif_format(data, msg_format) != format) {
		set_sif_format(data, msg_format, format);
		set_sif_channels(data, msg_format, channels);
		format_put_ipc(adev, format, channels, msg_format);
	}

	if (msg_format == SET_SIFS0_FORMAT) {
		mixp_channels = get_mixp_channels(data);
		if ((mixp_channels > channels)){ /* input(mixp) ch > output ch */
			format_put_ipc(adev, format, mixp_channels, msg_format);
			set_sif_channels(data, msg_format, channels); /* update with output ch */
		}
	}

	update_ch_num(adev, msg_format);

	return ret;
}

static unsigned int sifsx_cnt_val(unsigned long aclk, unsigned int rate,
		unsigned int physical_width, unsigned int channels)
{
	static const int correction;
	unsigned int n, d;

	/* k = n / d */
	d = channels * rate;
	n = 2 * (32 / physical_width);

	return DIV_ROUND_CLOSEST_ULL(aclk * n, d) - 1 + correction;
}

static int set_cnt_val(struct abox_data *data, enum abox_dai id,
		struct snd_pcm_hw_params *params)
{
	struct device *dev = data->dev;
	struct regmap *regmap = data->regmap;
	int idx = id - ABOX_SIFS0;
	unsigned int rate = params_rate(params);
	unsigned int width = params_width(params);
	unsigned int pwidth = params_physical_width(params);
	unsigned int channels = params_channels(params);
	unsigned long clk;
	unsigned int cnt_val;
	int ret = 0;

	ret = abox_register_bclk_usage(dev, data, id, rate, channels, width);
	if (ret < 0)
		abox_err(dev, "Unable to register bclk usage: %d\n", ret);

	data->sifs_cnt_dirty[idx] = true;
	clk = clk_get_rate(data->clk_cnt);
	cnt_val = sifsx_cnt_val(clk, rate, pwidth, channels);

	abox_info(dev, "%s[%#x](%ubit %uchannel %uHz at %luHz): %u\n",
			__func__, id, width, channels, rate, clk, cnt_val);

	ret = regmap_update_bits(regmap, ABOX_SPUS_CTRL_SIFS_CNT_VAL(idx),
			ABOX_SIFS_CNT_VAL_MASK(idx),
			cnt_val << ABOX_SIFS_CNT_VAL_L(idx));

	return ret;
}
static int get_width_for_spus_atune(struct abox_data *data, int idx)
{
	/* SPUS POST TUNE can process 32bit only */
	return abox_atune_spus_posttune_connected(data, idx) ? 32 : 0;
}

static int get_width_for_spum_atune(struct abox_data *data, int idx)
{
	/* SPUM PRE TUNE can process 32bit only */
	return abox_atune_spum_pretune_connected(data, idx) ? 32 : 0;
}

static int hw_params_fixup(struct abox_data *data, enum abox_dai id,
		struct snd_pcm_hw_params *params)
{
	struct device *dev = data->dev;
	enum ABOX_CONFIGMSG msg_rate, msg_format;
	unsigned int rate, channels, width;
	snd_pcm_format_t format;
	int ret = 0;

	abox_dbg(dev, "%s(%#x)\n", __func__, id);

	ret = get_configmsg(id, &msg_rate, &msg_format);
	if (ret < 0)
		return ret;

	rate = get_sif_rate(data, msg_rate);
	channels = get_sif_channels(data, msg_format);
	width = get_sif_width(data, msg_format);
	format = get_sif_format(data, msg_format);

	if (rate)
		hw_param_interval(params, SNDRV_PCM_HW_PARAM_RATE)->min = rate;

	if (channels)
		hw_param_interval(params, SNDRV_PCM_HW_PARAM_CHANNELS)->min =
				channels;

	if (format) {
		struct snd_mask *mask;

		mask = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);
		snd_mask_none(mask);
		snd_mask_set(mask, format);
	}

	if (rate || channels || format)
		abox_dbg(dev, "%#x: %d bit, %u ch, %uHz\n", id,
				width, channels, rate);

	return ret;
}

static int udma_hw_params_fixup(struct snd_soc_dai *dai,
		struct snd_pcm_hw_params *params)
{
	return abox_dma_hw_params_fixup(dai->dev, params);
}

static int sifs_hw_params_fixup(struct abox_data *data, enum abox_dai id,
		struct snd_pcm_hw_params *params)
{
	struct device *dev = data->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	enum abox_dai _id;
	struct snd_soc_dai *_dai;

	abox_dbg(dev, "%s(%#x)\n", __func__, id);

	_id = get_sink_dai_id(data, id);
	switch (_id) {
	case ABOX_UAIF0:
	case ABOX_UAIF1:
	case ABOX_UAIF2:
	case ABOX_UAIF3:
	case ABOX_UAIF4:
	case ABOX_UAIF5:
	case ABOX_UAIF6:
	case ABOX_DSIF:
		_dai = find_dai(cmpnt->card, _id);
		abox_if_hw_params_fixup(_dai, params);
		if (is_nsrc_connected(data, id))
			set_cnt_val(data, id, params);
		break;
	case ABOX_UDMA_WR0:
	case ABOX_UDMA_WR1:
		_dai = find_dai(cmpnt->card, _id);
		udma_hw_params_fixup(_dai, params);
		set_cnt_val(data, id, params);
		break;
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
	case ABOX_NSRC7:
	case ABOX_NSRC8:
	case ABOX_NSRC9:
	case ABOX_NSRC10:
	case ABOX_NSRC11:
	case ABOX_NSRC12:
	case ABOX_NSRC13:
	case ABOX_NSRC14:
	case ABOX_NSRC15:
		hw_params_fixup(data, id, params);
		set_cnt_val(data, id, params);
		break;
	default:
		/* Use sifs format if sink is unknown */
		hw_params_fixup(data, id, params);
		abox_err(dev, "%#x: invalid sink dai:%#x\n", id, _id);
		return 0;
	}

	return set_sif_params(data, id, params);
}

static int sifm_hw_params_fixup(struct abox_data *data, enum abox_dai id,
		struct snd_pcm_hw_params *params)
{
	struct device *dev = data->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	enum abox_dai _id;
	struct snd_soc_dai *_dai;
	int ret;

	abox_dbg(dev, "%s(%#x)\n", __func__, id);

	_id = get_source_dai_id(data, id);
	switch (_id) {
	case ABOX_UAIF0:
	case ABOX_UAIF1:
	case ABOX_UAIF2:
	case ABOX_UAIF3:
	case ABOX_UAIF4:
	case ABOX_UAIF5:
	case ABOX_UAIF6:
	case ABOX_DSIF:
	case ABOX_SPDY:
		_dai = find_dai(cmpnt->card, _id);
		abox_if_hw_params_fixup(_dai, params);
		ret = abox_cmpnt_adjust_sbank(data, id, params, 0);
		if (ret < 0)
			return ret;
		break;
	case ABOX_UDMA_RD0:
	case ABOX_UDMA_RD1:
		_dai = find_dai(cmpnt->card, _id);
		udma_hw_params_fixup(_dai, params);
		ret = abox_cmpnt_adjust_sbank(data, id, params, 0);
		if (ret < 0)
			return ret;
		break;
	case ABOX_SIFS0:
	case ABOX_SIFS1:
	case ABOX_SIFS2:
	case ABOX_SIFS3:
	case ABOX_SIFS4:
	case ABOX_SIFS5:
	case ABOX_SIFS6:
	case ABOX_SIFS7:
		sifs_hw_params_fixup(data, _id, params);
		break;
	default:
		abox_err(dev, "%#x: invalid source dai:%#x\n", id, _id);
		return 0;
	}

	return set_sif_params(data, id, params);
}

static int rdma_hw_params_fixup(struct snd_soc_dai *dai,
		struct snd_pcm_hw_params *params)
{
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(dai);
	struct device *dev = dai->dev;
	enum abox_dai id = dai->id;
	enum abox_dai _id;
	int ret;

	abox_dbg(dev, "%s(%s)\n", __func__, dai->name);

	_id = get_sink_dai_id(data->abox_data, id);
	switch (_id) {
	case ABOX_SIFS0:
	case ABOX_SIFS1:
	case ABOX_SIFS2:
	case ABOX_SIFS3:
	case ABOX_SIFS4:
	case ABOX_SIFS5:
	case ABOX_SIFS6:
	case ABOX_SIFS7:
		ret = sifs_hw_params_fixup(data->abox_data, _id, params);
		break;
	default:
		abox_err(dev, "%s: invalid sifs dai:%#x\n", dai->name, _id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int wdma_hw_params_fixup(struct snd_soc_dai *dai,
		struct snd_pcm_hw_params *params)
{
	struct abox_dma_data *data = snd_soc_dai_get_drvdata(dai);
	struct device *dev = dai->dev;
	enum abox_dai id = dai->id;
	enum abox_dai _id;
	int ret;

	abox_dbg(dev, "%s(%s)\n", __func__, dai->name);

	_id = get_source_dai_id(data->abox_data, id);
	switch (_id) {
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
	case ABOX_NSRC7:
	case ABOX_NSRC8:
	case ABOX_NSRC9:
	case ABOX_NSRC10:
	case ABOX_NSRC11:
	case ABOX_NSRC12:
	case ABOX_NSRC13:
	case ABOX_NSRC14:
	case ABOX_NSRC15:
		ret = sifm_hw_params_fixup(data->abox_data, _id, params);
		break;
	default:
		abox_err(dev, "%s: invalid sifs dai:%#x\n", dai->name, _id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int rdma_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params)
{
	struct snd_pcm_hw_params _params = *params;
	struct snd_soc_dai *cpu_dai;

	cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	rdma_hw_params_fixup(cpu_dai, &_params);
	return abox_dma_hw_params_fixup(cpu_dai->dev, params);
}

static int wdma_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params)
{
	struct snd_pcm_hw_params _params = *params;
	struct snd_soc_dai *cpu_dai;

	cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	wdma_hw_params_fixup(cpu_dai, &_params);
	return abox_dma_hw_params_fixup(cpu_dai->dev, params);
}

int abox_cmpnt_spus_get_sifs(struct abox_data *data, int id)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	enum abox_dai spus, sink;

	spus = ABOX_RDMA0 + id;
	sink = get_spus_sink(cmpnt, spus);
	if (sink < ABOX_SIFS0 || sink >= ABOX_SIFS0 + COUNT_SIFS)
		return -EINVAL;

	return sink - ABOX_SIFS0;
}

unsigned int abox_cmpnt_sif_get_dst_format(struct abox_data *data,
		int stream, int id)
{
	static const enum ABOX_CONFIGMSG configmsg_p[] = {
		SET_SIFS0_FORMAT, SET_SIFS1_FORMAT, SET_SIFS2_FORMAT,
		SET_SIFS3_FORMAT, SET_SIFS4_FORMAT, SET_SIFS5_FORMAT,
		SET_SIFS6_FORMAT, SET_SIFS7_FORMAT,
	};
	static const enum ABOX_CONFIGMSG configmsg_c[] = {
		SET_SIFM0_FORMAT, SET_SIFM1_FORMAT, SET_SIFM2_FORMAT,
		SET_SIFM3_FORMAT, SET_SIFM4_FORMAT, SET_SIFM5_FORMAT,
		SET_SIFM6_FORMAT, SET_SIFM7_FORMAT, SET_SIFM8_FORMAT,
		SET_SIFM9_FORMAT, SET_SIFM10_FORMAT, SET_SIFM11_FORMAT,
		SET_SIFM12_FORMAT, SET_SIFM13_FORMAT, SET_SIFM14_FORMAT,
		SET_SIFM15_FORMAT,
	};
	const enum ABOX_CONFIGMSG *configmsg;
	unsigned int width, channels, format;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (id >= ARRAY_SIZE(configmsg_p)) {
			abox_err(data->dev, "%s(%d, %d): invalid argument\n",
					__func__, stream, id);
			return 0;
		}
		configmsg = configmsg_p;
	} else {
		if (id >= ARRAY_SIZE(configmsg_c)) {
			abox_err(data->dev, "%s(%d, %d): invalid argument\n",
					__func__, stream, id);
			return 0;
		}
		configmsg = configmsg_c;
	}

	width = get_sif_width(data, configmsg[id]);
	channels = get_sif_channels(data, configmsg[id]);
	format = abox_get_format(width, channels);

	return format;
}

unsigned int abox_cmpnt_sif_get_dst_rate(struct abox_data *data,
		int stream, int id)
{
	static const enum ABOX_CONFIGMSG configmsg_p[] = {
		SET_SIFS0_RATE, SET_SIFS1_RATE, SET_SIFS2_RATE,
		SET_SIFS3_RATE, SET_SIFS4_RATE, SET_SIFS5_RATE,
		SET_SIFS6_RATE, SET_SIFS7_RATE,
	};
	static const enum ABOX_CONFIGMSG configmsg_c[] = {
		SET_SIFM0_RATE, SET_SIFM1_RATE, SET_SIFM2_RATE,
		SET_SIFM3_RATE, SET_SIFM4_RATE, SET_SIFM5_RATE,
		SET_SIFM6_RATE, SET_SIFM7_RATE, SET_SIFM8_RATE,
		SET_SIFM9_RATE, SET_SIFM10_RATE, SET_SIFM11_RATE,
		SET_SIFM12_RATE, SET_SIFM13_RATE, SET_SIFM14_RATE,
		SET_SIFM15_RATE,
	};
	const enum ABOX_CONFIGMSG *configmsg;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (id >= ARRAY_SIZE(configmsg_p)) {
			abox_err(data->dev, "%s(%d, %d): invalid argument\n",
					__func__, stream, id);
			return 0;
		}
		configmsg = configmsg_p;
	} else {
		if (id >= ARRAY_SIZE(configmsg_c)) {
			abox_err(data->dev, "%s(%d, %d): invalid argument\n",
					__func__, stream, id);
			return 0;
		}
		configmsg = configmsg_c;
	}

	return get_sif_rate(data, configmsg[id]);
}

void abox_cmpnt_register_event_notifier(struct abox_data *data,
		enum abox_widget w, int (*notify)(void *priv, bool en),
		void *priv)
{
	struct abox_event_notifier *event_notifier = &data->event_notifier[w];

	WRITE_ONCE(event_notifier->priv, priv);
	WRITE_ONCE(event_notifier->notify, notify);
}

void abox_cmpnt_unregister_event_notifier(struct abox_data *data,
		enum abox_widget w)
{
	struct abox_event_notifier *event_notifier = &data->event_notifier[w];

	WRITE_ONCE(event_notifier->notify, NULL);
	WRITE_ONCE(event_notifier->priv, NULL);
}

static int notify_event(struct abox_data *data, enum abox_widget w, int e)
{
	struct abox_event_notifier event_notifier;
	bool en;

	switch (e) {
	case SND_SOC_DAPM_POST_PMU:
	case SND_SOC_DAPM_PRE_PMU:
		en = true;
		break;
	case SND_SOC_DAPM_PRE_PMD:
	case SND_SOC_DAPM_POST_PMD:
		en = false;
		break;
	default:
		return 0;
	};

	event_notifier = data->event_notifier[w];

	if (event_notifier.notify)
		return event_notifier.notify(event_notifier.priv, en);
	else
		return 0;
}

static int spus_in_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int e)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int id = w->shift;

	abox_dbg(dev, "%s(%d, %d)\n", __func__, id, e);

	return notify_event(data, ABOX_WIDGET_SPUS_IN0 + id, e);
}

static int sifs_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int e)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int id = w->shift;

	abox_dbg(dev, "%s(%d, %d)\n", __func__, id, e);

	return notify_event(data, ABOX_WIDGET_SIFS0 + id, e);
}

static int nsrc_event(struct snd_soc_dapm_widget *w, struct snd_kcontrol *k, int e)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int id = w->shift;

	abox_dbg(dev, "%s(%d, %d)\n", __func__, id, e);

	if (SND_SOC_DAPM_EVENT_ON(e)) {
		abox_info(dev, "nsrc%d connection type: %d\n", id, nsrc_bridge[id]);
		snd_soc_component_update_bits(data->cmpnt,
				ABOX_ROUTE_CTRL_CONNECT,
				ABOX_NSRC_CONNECTION_TYPE_MASK(id),
				nsrc_bridge[id] << ABOX_NSRC_CONNECTION_TYPE_L(id));
	}

	return notify_event(data, ABOX_WIDGET_NSRC0 + id, e);
}

static int asrc_get_idx(struct snd_soc_dapm_widget *w)
{
	return w->shift;
}

static const struct snd_kcontrol_new *asrc_get_kcontrol(int idx, int stream)
{
	if (idx < 0)
		return ERR_PTR(-EINVAL);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (idx < ARRAY_SIZE(spus_asrc_controls))
			return &spus_asrc_controls[idx];
		else
			return ERR_PTR(-EINVAL);
	} else {
		if (idx < ARRAY_SIZE(spum_asrc_controls))
			return &spum_asrc_controls[idx];
		else
			return ERR_PTR(-EINVAL);
	}
}

static const struct snd_kcontrol_new *asrc_get_id_kcontrol(int idx, int stream)
{
	if (idx < 0)
		return ERR_PTR(-EINVAL);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (idx < ARRAY_SIZE(spus_asrc_id_controls))
			return &spus_asrc_id_controls[idx];
		else
			return ERR_PTR(-EINVAL);
	} else {
		if (idx < ARRAY_SIZE(spum_asrc_id_controls))
			return &spum_asrc_id_controls[idx];
		else
			return ERR_PTR(-EINVAL);
	}
}

static int asrc_get_id(struct snd_soc_component *cmpnt, int idx, int stream)
{
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;

	kcontrol = asrc_get_id_kcontrol(idx, stream);
	if (IS_ERR(kcontrol))
		return PTR_ERR(kcontrol);

	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = ((1 << fls(mc->max)) - 1) << mc->shift;
	val = snd_soc_component_read(cmpnt, reg);

	return (val & mask) >> mc->shift;
}

static bool asrc_get_active(struct snd_soc_component *cmpnt, int idx,
		int stream)
{
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;

	kcontrol = asrc_get_kcontrol(idx, stream);
	if (IS_ERR(kcontrol))
		return false;

	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = 1 << mc->shift;
	val = snd_soc_component_read(cmpnt, reg);

	return !!(val & mask);
}

static int asrc_get_idx_of_id(struct snd_soc_component *cmpnt, int id,
		int stream)
{
	int idx;
	size_t len;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		len = ARRAY_SIZE(spus_asrc_controls);
	else
		len = ARRAY_SIZE(spum_asrc_controls);

	for (idx = 0; idx < len; idx++) {
		if (id == asrc_get_id(cmpnt, idx, stream))
			return idx;
	}

	return -EINVAL;
}

static bool asrc_get_id_active(struct snd_soc_component *cmpnt, int id,
		int stream)
{
	int idx;

	idx = asrc_get_idx_of_id(cmpnt, id, stream);
	if (idx < 0)
		return false;

	return asrc_get_active(cmpnt, idx, stream);
}

static int asrc_put_id(struct snd_soc_component *cmpnt, int idx, int stream,
		int id)
{
	struct device *dev = cmpnt->dev;
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;

	abox_dbg(dev, "%s(%d, %d, %d)\n", __func__, idx, stream, id);

	kcontrol = asrc_get_id_kcontrol(idx, stream);
	if (IS_ERR(kcontrol))
		return PTR_ERR(kcontrol);

	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = ((1 << fls(mc->max)) - 1) << mc->shift;
	val = id << mc->shift;
	return snd_soc_component_update_bits(cmpnt, reg, mask, val);
}

static int asrc_put_active(struct snd_soc_dapm_widget *w, int stream, int on)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;
	struct device *dev = cmpnt->dev;
	const struct snd_kcontrol_new *kcontrol;
	struct soc_mixer_control *mc;
	unsigned int reg, mask, val;

	abox_dbg(dev, "%s(%s, %d, %d)\n", __func__, w->name, stream, on);

	kcontrol = asrc_get_kcontrol(asrc_get_idx(w), stream);
	if (IS_ERR(kcontrol))
		return PTR_ERR(kcontrol);

	abox_info(dev, "%s %s\n", w->name, on ? "on" : "off");
	mc = (struct soc_mixer_control *)kcontrol->private_value;
	reg = mc->reg;
	mask = 1 << mc->shift;
	val = !!on << mc->shift;
	return snd_soc_component_update_bits(cmpnt, reg, mask, val);
}

static int asrc_exchange_id(struct snd_soc_component *cmpnt, int stream,
		int idx1, int idx2)
{
	struct device *dev = cmpnt->dev;
	int id1 = asrc_get_id(cmpnt, idx1, stream);
	int id2 = asrc_get_id(cmpnt, idx2, stream);
	int ret;

	abox_dbg(dev, "%s(%d, %d, %d)\n", __func__, stream, idx1, idx2);

	if (idx1 == idx2)
		return 0;

	ret = asrc_put_id(cmpnt, idx1, stream, id2);
	if (ret < 0)
		return ret;
	ret = asrc_put_id(cmpnt, idx2, stream, id1);
	if (ret < 0)
		asrc_put_id(cmpnt, idx1, stream, id1);

	return ret;
}

static const int spus_asrc_max_channels[] = {
	8, 4, 4, 2, 8, 4, 4, 2,
};

static const int spum_asrc_max_channels[] = {
	8, 4, 4, 2,
};

static const int spus_asrc_sorted_id[] = {
	3, 7, 2, 6, 1, 5, 0, 4,
};

static const int spum_asrc_sorted_id[] = {
	3, 2, 1, 0,
};

static int spus_asrc_channels[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
};

static int spum_asrc_channels[] = {
	0, 0, 0, 0,
};

static int spus_asrc_lock[] = {
	-1, -1, -1, -1,
	-1, -1, -1, -1,
	-1, -1, -1, -1,
	-1, -1, -1, -1,
};

static int spum_asrc_lock[] = {
	-1, -1, -1, -1,
	-1, -1, -1, -1,
	-1, -1, -1, -1,
	-1, -1, -1, -1,
};

static bool abox_cmpnt_check_asrc_id(int id)
{
#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E9945)
	return (id < SSRC_START_ID);
#else
	return true;
#endif
}

int abox_cmpnt_asrc_lock(struct abox_data *data, int stream,
		int idx, int id)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	int ret = 0;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		spus_asrc_lock[idx] = id;
	else
		spum_asrc_lock[idx] = id;

	if (abox_cmpnt_check_asrc_id(id)) {
		if (id != asrc_get_id(cmpnt, idx, stream)) {
			ret = asrc_exchange_id(cmpnt, stream, idx,
					asrc_get_idx_of_id(cmpnt, id, stream));
			if (ret < 0)
				return ret;
			ret = asrc_put_active(asrc_get_widget(cmpnt, idx, stream),
					stream, 1);
			if (ret < 0)
				return ret;
		}
	} else {
		ret = asrc_put_id(cmpnt, idx, stream, id);
		if (ret < 0)
			return ret;
		ret = asrc_put_active(asrc_get_widget(cmpnt, idx, stream),
				stream, 1);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static bool asrc_is_lock(int stream, int id)
{
	int i;

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0; i < ARRAY_SIZE(spus_asrc_lock); i++) {
			if (spus_asrc_lock[i] == id)
				return true;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(spum_asrc_lock); i++) {
			if (spum_asrc_lock[i] == id)
				return true;
		}
	}

	return false;
}

static int asrc_get_lock_id(int stream, int idx)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return spus_asrc_lock[idx];
	else
		return spum_asrc_lock[idx];
}

static int asrc_get_num(int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return ARRAY_SIZE(spus_asrc_sorted_id);
	else
		return ARRAY_SIZE(spum_asrc_sorted_id);
}

static int asrc_get_max_channels(int id, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return spus_asrc_max_channels[id];
	else
		return spum_asrc_max_channels[id];
}

static int asrc_get_sorted_id(int i, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return spus_asrc_sorted_id[i];
	else
		return spum_asrc_sorted_id[i];
}

static int asrc_get_channels(int id, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return spus_asrc_channels[id];
	else
		return spum_asrc_channels[id];
}

static void asrc_set_channels(int id, int stream, int channels)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		spus_asrc_channels[id] = channels;
	else
		spum_asrc_channels[id] = channels;
}

static int asrc_is_avail_id(struct snd_soc_dapm_widget *w, int id,
		int stream, int channels)
{
	struct snd_soc_component *cmpnt = w->dapm->component;
	struct device *dev = cmpnt->dev;
	int idx = asrc_get_idx_of_id(cmpnt, id, stream);
	struct snd_soc_dapm_widget *w_t = asrc_get_widget(cmpnt, idx, stream);
	int ret;

	if (asrc_get_max_channels(id, stream) < channels) {
		ret = false;
		goto out;
	}

	if (w_t != w && asrc_is_lock(stream, id)) {
		ret = false;
		goto out;
	}

	if (w_t != w && asrc_get_id_active(cmpnt, id, stream)) {
		ret = false;
		goto out;
	}

	if (id % 2) {
		if (asrc_get_id_active(cmpnt, id - 1, stream))
			ret = asrc_get_channels(id - 1, stream) <
					asrc_get_max_channels(id - 1, stream);
		else
			ret = true;
	} else {
		if (channels < asrc_get_max_channels(id, stream))
			ret = true;
		else
			ret = !asrc_get_id_active(cmpnt, id + 1, stream);
	}
out:
	abox_info(dev, "%s(%d, %d, %d): %d\n", __func__,
			id, stream, channels, ret);
	return ret;
}

static int asrc_assign_id(struct snd_soc_dapm_widget *w, int stream,
		unsigned int channels)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;
	int i, id, ret = -EINVAL;

	id = asrc_get_lock_id(stream, asrc_get_idx(w));
	if (id >= 0) {
		ret = asrc_exchange_id(cmpnt, stream, asrc_get_idx(w),
				asrc_get_idx_of_id(cmpnt, id, stream));
		if (ret >= 0)
			asrc_set_channels(id, stream, channels);
		return ret;
	}

	for (i = 0; i < asrc_get_num(stream); i++) {
		id = asrc_get_sorted_id(i, stream);

		if (asrc_is_avail_id(w, id, stream, channels)) {
			ret = asrc_exchange_id(cmpnt, stream, asrc_get_idx(w),
					asrc_get_idx_of_id(cmpnt, id, stream));
			if (ret >= 0)
				asrc_set_channels(id, stream, channels);
			break;
		}
	}

	return ret;
}

static void asrc_release_id(struct snd_soc_dapm_widget *w, int stream)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;
	int idx = asrc_get_idx(w);
	int id = asrc_get_id(cmpnt, idx, stream);

	if (id < 0 || id >= asrc_get_num(stream))
		return;

	if (asrc_get_lock_id(stream, idx) >= 0)
		return;

	asrc_set_channels(id, stream, 0);
	asrc_put_active(w, stream, 0);
}

unsigned int abox_cmpnt_asrc_get_dst_format(struct abox_data *data,
		int stream, int id)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	int width, channels;
	unsigned int val;

	channels = asrc_get_channels(id, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		val = snd_soc_component_read(cmpnt, ABOX_SPUS_ASRC_CTRL(id));
	else
		val = snd_soc_component_read(cmpnt, ABOX_SPUM_ASRC_CTRL(id));
	width = (val & ABOX_ASRC_BIT_WIDTH_MASK) >> ABOX_ASRC_BIT_WIDTH_L;

	return (width << 0x3) | (channels - 1);
}

void abox_cmpnt_asrc_release(struct abox_data *data, int stream, int idx)
{
	struct snd_soc_component *cmpnt = data->cmpnt;

	abox_dbg(cmpnt->dev, "%s(%d, %d)\n", __func__, stream, idx);

	asrc_release_id(asrc_get_widget(cmpnt, idx, stream), stream);
}

void abox_cmpnt_asrc_enable(struct abox_data *data, int stream, int idx)
{
	struct snd_soc_component *cmpnt = data->cmpnt;

	abox_dbg(cmpnt->dev, "%s(%d, %d)\n", __func__, stream, idx);

	asrc_put_active(asrc_get_widget(cmpnt, idx, stream), stream, 1);
}

static int spus_asrc_tune[] = {
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT
};

static int spum_asrc_tune[] = {
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT,
	ABOX_ASRC_TUNE_DEFAULT, ABOX_ASRC_TUNE_DEFAULT
};

static int get_asrc_tune(int stream, int idx)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (idx < ARRAY_SIZE(spus_asrc_tune))
			return spus_asrc_tune[idx];
	} else {
		if (idx < ARRAY_SIZE(spum_asrc_tune))
			return spum_asrc_tune[idx];
	}

	return ABOX_ASRC_TUNE_DEFAULT;
}

static void set_asrc_tune(int stream, int idx, int tune)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (idx < ARRAY_SIZE(spus_asrc_tune))
			spus_asrc_tune[idx] = tune;
	} else {
		if (idx < ARRAY_SIZE(spum_asrc_tune))
			spum_asrc_tune[idx] = tune;
	}
}

static int asrc_tune_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int stream = mc->reg;
	int idx = mc->shift;

	dev_dbg(dev, "%s(%d, %d)\n", __func__, stream, idx);

	ucontrol->value.integer.value[0] = get_asrc_tune(stream, idx);

	return 0;
}

static int asrc_tune_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int stream = mc->reg;
	int idx = mc->shift;
	int val = (int)ucontrol->value.integer.value[0];
	int ret = 0;

	dev_dbg(dev, "%s(%d, %d, %d)\n", __func__, stream, idx, val);

	if (val < ABOX_ASRC_TUNE_MIN || val > mc->max)
		return -EINVAL;

	set_asrc_tune(stream, idx, val);
	if (asrc_get_active(cmpnt, idx, stream))
		ret = asrc_update(cmpnt, idx, stream);

	return ret;
}

static const struct snd_kcontrol_new spus_asrc_tune_controls[] = {
	SOC_SINGLE_EXT("SPUS ASRC0 TUNE", 0, 0, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC1 TUNE", 0, 1, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC2 TUNE", 0, 2, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC3 TUNE", 0, 3, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC4 TUNE", 0, 4, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC5 TUNE", 0, 5, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC6 TUNE", 0, 6, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC7 TUNE", 0, 7, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC8 TUNE", 0, 8, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC9 TUNE", 0, 9, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC10 TUNE", 0, 10, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC11 TUNE", 0, 11, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC12 TUNE", 0, 12, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC13 TUNE", 0, 13, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC14 TUNE", 0, 14, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUS ASRC15 TUNE", 0, 15, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
};

static const struct snd_kcontrol_new spum_asrc_tune_controls[] = {
	SOC_SINGLE_EXT("SPUM ASRC0 TUNE", 1, 0, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC1 TUNE", 1, 1, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC2 TUNE", 1, 2, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC3 TUNE", 1, 3, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC4 TUNE", 1, 4, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC5 TUNE", 1, 5, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC6 TUNE", 1, 6, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC7 TUNE", 1, 7, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC8 TUNE", 1, 8, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC9 TUNE", 1, 9, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC10 TUNE", 1, 10, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC11 TUNE", 1, 11, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC12 TUNE", 1, 12, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC13 TUNE", 1, 13, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC14 TUNE", 1, 14, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
	SOC_SINGLE_EXT("SPUM ASRC15 TUNE", 1, 15, ABOX_ASRC_TUNE_MAX, 0,
			asrc_tune_get, asrc_tune_put),
};

static const struct snd_kcontrol_new *spus_controls[][SPUS_CONTROL_COUNT] = {
	{
		&spus_asrc_controls[0],
		&spus_asrc_id_controls[0],
		&spus_asrc_apf_coef_controls[0],
		&spus_asrc_os_controls[0],
		&spus_asrc_is_controls[0],
		&spus_asrc_tune_controls[0],
		&spus_asrc_dummy_start_controls[0],
		&spus_asrc_start_num_controls[0],
	},
	{
		&spus_asrc_controls[1],
		&spus_asrc_id_controls[1],
		&spus_asrc_apf_coef_controls[1],
		&spus_asrc_os_controls[1],
		&spus_asrc_is_controls[1],
		&spus_asrc_tune_controls[1],
		&spus_asrc_dummy_start_controls[1],
		&spus_asrc_start_num_controls[1],
	},
	{
		&spus_asrc_controls[2],
		&spus_asrc_id_controls[2],
		&spus_asrc_apf_coef_controls[2],
		&spus_asrc_os_controls[2],
		&spus_asrc_is_controls[2],
		&spus_asrc_tune_controls[2],
		&spus_asrc_dummy_start_controls[2],
		&spus_asrc_start_num_controls[2],
	},
	{
		&spus_asrc_controls[3],
		&spus_asrc_id_controls[3],
		&spus_asrc_apf_coef_controls[3],
		&spus_asrc_os_controls[3],
		&spus_asrc_is_controls[3],
		&spus_asrc_tune_controls[3],
		&spus_asrc_dummy_start_controls[3],
		&spus_asrc_start_num_controls[3],

	},
	{
		&spus_asrc_controls[4],
		&spus_asrc_id_controls[4],
		&spus_asrc_apf_coef_controls[4],
		&spus_asrc_os_controls[4],
		&spus_asrc_is_controls[4],
		&spus_asrc_tune_controls[4],
		&spus_asrc_dummy_start_controls[4],
		&spus_asrc_start_num_controls[4],
	},
	{
		&spus_asrc_controls[5],
		&spus_asrc_id_controls[5],
		&spus_asrc_apf_coef_controls[5],
		&spus_asrc_os_controls[5],
		&spus_asrc_is_controls[5],
		&spus_asrc_tune_controls[5],
		&spus_asrc_dummy_start_controls[5],
		&spus_asrc_start_num_controls[5],
	},
	{
		&spus_asrc_controls[6],
		&spus_asrc_id_controls[6],
		&spus_asrc_apf_coef_controls[6],
		&spus_asrc_os_controls[6],
		&spus_asrc_is_controls[6],
		&spus_asrc_tune_controls[6],
		&spus_asrc_dummy_start_controls[6],
		&spus_asrc_start_num_controls[6],
	},
	{
		&spus_asrc_controls[7],
		&spus_asrc_id_controls[7],
		&spus_asrc_apf_coef_controls[7],
		&spus_asrc_os_controls[7],
		&spus_asrc_is_controls[7],
		&spus_asrc_tune_controls[7],
		&spus_asrc_dummy_start_controls[7],
		&spus_asrc_start_num_controls[7],
	},
	{
		&spus_asrc_controls[8],
		&spus_asrc_id_controls[8],
		&spus_asrc_apf_coef_controls[8],
		&spus_asrc_os_controls[8],
		&spus_asrc_is_controls[8],
		&spus_asrc_tune_controls[8],
		&spus_asrc_dummy_start_controls[8],
		&spus_asrc_start_num_controls[8],
	},
	{
		&spus_asrc_controls[9],
		&spus_asrc_id_controls[9],
		&spus_asrc_apf_coef_controls[9],
		&spus_asrc_os_controls[9],
		&spus_asrc_is_controls[9],
		&spus_asrc_tune_controls[9],
		&spus_asrc_dummy_start_controls[9],
		&spus_asrc_start_num_controls[9],
	},
	{
		&spus_asrc_controls[10],
		&spus_asrc_id_controls[10],
		&spus_asrc_apf_coef_controls[10],
		&spus_asrc_os_controls[10],
		&spus_asrc_is_controls[10],
		&spus_asrc_tune_controls[10],
		&spus_asrc_dummy_start_controls[10],
		&spus_asrc_start_num_controls[10],
	},
	{
		&spus_asrc_controls[11],
		&spus_asrc_id_controls[11],
		&spus_asrc_apf_coef_controls[11],
		&spus_asrc_os_controls[11],
		&spus_asrc_is_controls[11],
		&spus_asrc_tune_controls[11],
		&spus_asrc_dummy_start_controls[11],
		&spus_asrc_start_num_controls[11],
	},
	{
		&spus_asrc_controls[12],
		&spus_asrc_id_controls[12],
		&spus_asrc_apf_coef_controls[12],
		&spus_asrc_os_controls[12],
		&spus_asrc_is_controls[12],
		&spus_asrc_tune_controls[12],
		&spus_asrc_dummy_start_controls[12],
		&spus_asrc_start_num_controls[12],
	},
	{
		&spus_asrc_controls[13],
		&spus_asrc_id_controls[13],
		&spus_asrc_apf_coef_controls[13],
		&spus_asrc_os_controls[13],
		&spus_asrc_is_controls[13],
		&spus_asrc_tune_controls[13],
		&spus_asrc_dummy_start_controls[13],
		&spus_asrc_start_num_controls[13],
	},
	{
		&spus_asrc_controls[14],
		&spus_asrc_id_controls[14],
		&spus_asrc_apf_coef_controls[14],
		&spus_asrc_os_controls[14],
		&spus_asrc_is_controls[14],
		&spus_asrc_tune_controls[14],
		&spus_asrc_dummy_start_controls[14],
		&spus_asrc_start_num_controls[14],
	},
	{
		&spus_asrc_controls[15],
		&spus_asrc_id_controls[15],
		&spus_asrc_apf_coef_controls[15],
		&spus_asrc_os_controls[15],
		&spus_asrc_is_controls[15],
		&spus_asrc_tune_controls[15],
		&spus_asrc_dummy_start_controls[15],
		&spus_asrc_start_num_controls[15],
	},
};

static const struct snd_kcontrol_new *spum_controls[][SPUM_CONTROL_COUNT] = {
	{
		&spum_asrc_controls[0],
		&spum_asrc_id_controls[0],
		&spum_asrc_apf_coef_controls[0],
		&spum_asrc_os_controls[0],
		&spum_asrc_is_controls[0],
		&spum_asrc_tune_controls[0],
	},
	{
		&spum_asrc_controls[1],
		&spum_asrc_id_controls[1],
		&spum_asrc_apf_coef_controls[1],
		&spum_asrc_os_controls[1],
		&spum_asrc_is_controls[1],
		&spum_asrc_tune_controls[1],
	},
	{
		&spum_asrc_controls[2],
		&spum_asrc_id_controls[2],
		&spum_asrc_apf_coef_controls[2],
		&spum_asrc_os_controls[2],
		&spum_asrc_is_controls[2],
		&spum_asrc_tune_controls[2],
	},
	{
		&spum_asrc_controls[3],
		&spum_asrc_id_controls[3],
		&spum_asrc_apf_coef_controls[3],
		&spum_asrc_os_controls[3],
		&spum_asrc_is_controls[3],
		&spum_asrc_tune_controls[3],

	},
	{
		&spum_asrc_controls[4],
		&spum_asrc_id_controls[4],
		&spum_asrc_apf_coef_controls[4],
		&spum_asrc_os_controls[4],
		&spum_asrc_is_controls[4],
		&spum_asrc_tune_controls[4],
	},
	{
		&spum_asrc_controls[5],
		&spum_asrc_id_controls[5],
		&spum_asrc_apf_coef_controls[5],
		&spum_asrc_os_controls[5],
		&spum_asrc_is_controls[5],
		&spum_asrc_tune_controls[5],
	},
	{
		&spum_asrc_controls[6],
		&spum_asrc_id_controls[6],
		&spum_asrc_apf_coef_controls[6],
		&spum_asrc_os_controls[6],
		&spum_asrc_is_controls[6],
		&spum_asrc_tune_controls[6],
	},
	{
		&spum_asrc_controls[7],
		&spum_asrc_id_controls[7],
		&spum_asrc_apf_coef_controls[7],
		&spum_asrc_os_controls[7],
		&spum_asrc_is_controls[7],
		&spum_asrc_tune_controls[7],
	},
	{
		&spum_asrc_controls[8],
		&spum_asrc_id_controls[8],
		&spum_asrc_apf_coef_controls[8],
		&spum_asrc_os_controls[8],
		&spum_asrc_is_controls[8],
		&spum_asrc_tune_controls[8],
	},
	{
		&spum_asrc_controls[9],
		&spum_asrc_id_controls[9],
		&spum_asrc_apf_coef_controls[9],
		&spum_asrc_os_controls[9],
		&spum_asrc_is_controls[9],
		&spum_asrc_tune_controls[9],
	},
	{
		&spum_asrc_controls[10],
		&spum_asrc_id_controls[10],
		&spum_asrc_apf_coef_controls[10],
		&spum_asrc_os_controls[10],
		&spum_asrc_is_controls[10],
		&spum_asrc_tune_controls[10],
	},
	{
		&spum_asrc_controls[11],
		&spum_asrc_id_controls[11],
		&spum_asrc_apf_coef_controls[11],
		&spum_asrc_os_controls[11],
		&spum_asrc_is_controls[11],
		&spum_asrc_tune_controls[11],
	},
	{
		&spum_asrc_controls[12],
		&spum_asrc_id_controls[12],
		&spum_asrc_apf_coef_controls[12],
		&spum_asrc_os_controls[12],
		&spum_asrc_is_controls[12],
		&spum_asrc_tune_controls[12],
	},
	{
		&spum_asrc_controls[13],
		&spum_asrc_id_controls[13],
		&spum_asrc_apf_coef_controls[13],
		&spum_asrc_os_controls[13],
		&spum_asrc_is_controls[13],
		&spum_asrc_tune_controls[13],
	},
	{
		&spum_asrc_controls[14],
		&spum_asrc_id_controls[14],
		&spum_asrc_apf_coef_controls[14],
		&spum_asrc_os_controls[14],
		&spum_asrc_is_controls[14],
		&spum_asrc_tune_controls[14],
	},
	{
		&spum_asrc_controls[15],
		&spum_asrc_id_controls[15],
		&spum_asrc_apf_coef_controls[15],
		&spum_asrc_os_controls[15],
		&spum_asrc_is_controls[15],
		&spum_asrc_tune_controls[15],
	},
};

static int asrc_find_id(struct snd_soc_dapm_widget *w, int stream)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_component *cmpnt = dapm->component;

	return asrc_get_id(cmpnt, asrc_get_idx(w), stream);
}

static int update_bits_async(struct device *dev,
		struct snd_soc_component *cmpnt, const char *name,
		unsigned int reg, unsigned int mask, unsigned int val)
{
	int ret;

	abox_dbg(dev, "%s(%s, %#x, %#x, %#x)\n", __func__, name, reg, mask, val);

	ret = snd_soc_component_update_bits_async(cmpnt, reg, mask, val);
	if (ret < 0)
		abox_err(dev, "%s(%s, %#x, %#x, %#x): %d\n", __func__, name, reg,
				mask, val, ret);

	return ret;
}

static int asrc_update_tick(struct abox_data *data, int stream, int id)
{
	static const int tick_table[][3] = {
		/* aclk, ticknum, tickdiv */
		{600000000, 1, 1},
		{400000000, 3, 2},
		{300000000, 2, 1},
		{250000000, 5, 2},
		{200000000, 3, 1},
		{150000000, 4, 1},
		{100000000, 6, 1},
	};
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct device *dev = cmpnt->dev;
	unsigned int reg, mask, val;
	enum asrc_tick itick, otick;
	int idx = asrc_get_idx_of_id(cmpnt, id, stream);
	unsigned long aclk = clk_get_rate(data->clk_bus);
	int ticknum = 1, tickdiv = 1;
	int i, res, ret = 0;

	abox_dbg(dev, "%s(%d, %d, %luHz)\n", __func__, stream, id, aclk);

	if (idx < 0) {
		abox_dbg(dev, "%s(%d, %dcore): Not allocated ASRC: %d\n", __func__,
				stream, id, idx);
		return 0;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg = ABOX_SPUS_ASRC_CTRL(id);
		itick = spus_asrc_is[idx];
		otick = spus_asrc_os[idx];
	} else {
		reg = ABOX_SPUM_ASRC_CTRL(id);
		itick = spum_asrc_is[idx];
		otick = spum_asrc_os[idx];
	}

	if ((itick == TICK_SYNC) && (otick == TICK_SYNC))
		return 0;

	for (i = 0; i < ARRAY_SIZE(tick_table); i++) {
		if (aclk > tick_table[i][0])
			break;

		ticknum = tick_table[i][1];
		tickdiv = tick_table[i][2];
	}

	mask = ABOX_ASRC_TICKNUM_MASK;
	val = ticknum << ABOX_ASRC_TICKNUM_L;
	res = update_bits_async(dev, cmpnt, "ticknum", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_TICKDIV_MASK;
	val = tickdiv << ABOX_ASRC_TICKDIV_L;
	res = update_bits_async(dev, cmpnt, "tickdiv", reg, mask, val);
	if (res < 0)
		ret = res;

	/* Todo: change it to abox_dbg() */
	abox_info(dev, "asrc tick(%d, %d) aclk=%luHz: %d, %d\n",
			stream, id, aclk, ticknum, tickdiv);

	return ret;
}

int abox_cmpnt_update_asrc_tick(struct device *adev)
{
	struct device *dev = adev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct snd_soc_component *cmpnt = data->cmpnt;
	int stream, id, res, ret = 0;

	if (!cmpnt)
		return 0;

	abox_dbg(dev, "%s\n", __func__);

	for (stream = 0; stream <= SNDRV_PCM_STREAM_LAST; stream++) {
		for (id = 0; id < asrc_get_num(stream); id++) {
			if (pm_runtime_get_if_in_use(dev) > 0) {
				res = asrc_update_tick(data, stream, id);
				if (res < 0)
					ret = res;
				pm_runtime_put(dev);
			}
		}
	}

	snd_soc_component_async_complete(cmpnt);

	return ret;
}

static int asrc_config_async(struct abox_data *data, int id, int stream,
		enum asrc_tick itick, unsigned int isr,
		enum asrc_tick otick, unsigned int osr, int tune,
		unsigned int s_default, unsigned int width, int apf_coef)
{
	struct device *dev = data->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	bool spus = (stream == SNDRV_PCM_STREAM_PLAYBACK);
	unsigned int ofactor;
	unsigned int reg, mask, val;
	struct asrc_ctrl ctrl;
	int res, ret = 0;

	abox_dbg(dev, "%s(%d, %d, %d, %uHz, %d, %uHz, %d, %u, %ubit, %d)\n",
			__func__, id, stream, itick, isr,
			otick, osr, tune, s_default, width, apf_coef);

	if ((itick == TICK_SYNC) == (otick == TICK_SYNC)) {
		abox_err(dev, "%s: itick=%d otick=%d\n", __func__, itick, otick);
		return -EINVAL;
	}

	if (s_default == 0) {
		abox_err(dev, "invalid default\n");
		return -EINVAL;
	}

	/* set async side to input side */
	ctrl.isr = (itick != TICK_SYNC) ? isr : osr;
	ctrl.osr = (itick != TICK_SYNC) ? osr : isr;
	ctrl.ovsf = RATIO_8;
	ctrl.ifactor = s_default;
	ctrl.dcmf = RATIO_8;
	ofactor = cal_ofactor(&ctrl, tune);
	while (ofactor > ABOX_ASRC_OS_DEFAULT_MASK) {
		ctrl.ovsf--;
		ofactor = cal_ofactor(&ctrl, tune);
	}

	if (itick == TICK_SYNC) {
		swap(ctrl.isr, ctrl.osr);
		swap(ctrl.ovsf, ctrl.dcmf);
		swap(ctrl.ifactor, ofactor);
	}

	abox_dbg(dev, "asrc(%d, %d): %d, %uHz, %d, %uHz, %d, %d, %u, %u, %u, %u\n",
			stream, id, itick, ctrl.isr, otick, ctrl.osr,
			1 << ctrl.ovsf, 1 << ctrl.dcmf, ctrl.ifactor, ofactor,
			is_limit(ctrl.ifactor), os_limit(ofactor));

	reg = spus ? ABOX_SPUS_ASRC_CTRL(id) : ABOX_SPUM_ASRC_CTRL(id);

	mask = ABOX_ASRC_BIT_WIDTH_MASK;
	val = ((width / 8) - 1) << ABOX_ASRC_BIT_WIDTH_L;
	res = update_bits_async(dev, cmpnt, "width", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_IS_SYNC_MODE_MASK;
	val = (itick != TICK_SYNC) << ABOX_ASRC_IS_SYNC_MODE_L;
	res = update_bits_async(dev, cmpnt, "is sync", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OS_SYNC_MODE_MASK;
	val = (otick != TICK_SYNC) << ABOX_ASRC_OS_SYNC_MODE_L;
	res = update_bits_async(dev, cmpnt, "os sync", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OVSF_RATIO_MASK;
	val = ctrl.ovsf << ABOX_ASRC_OVSF_RATIO_L;
	res = update_bits_async(dev, cmpnt, "ovsf ratio", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_DCMF_RATIO_MASK;
	val = ctrl.dcmf << ABOX_ASRC_DCMF_RATIO_L;
	res = update_bits_async(dev, cmpnt, "dcmf ratio", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_IS_SOURCE_SEL_MASK;
	val = itick << ABOX_ASRC_IS_SOURCE_SEL_L;
	res = update_bits_async(dev, cmpnt, "is source", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OS_SOURCE_SEL_MASK;
	val = otick << ABOX_ASRC_OS_SOURCE_SEL_L;
	res = update_bits_async(dev, cmpnt, "os source", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_IS_DEFAULT(id) :
			ABOX_SPUM_ASRC_IS_DEFAULT(id);
	mask = ABOX_ASRC_IS_DEFAULT_MASK;
	val = ctrl.ifactor;
	res = update_bits_async(dev, cmpnt, "is default", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_IS_TPLIMIT(id) :
			ABOX_SPUM_ASRC_IS_TPLIMIT(id);
	mask = ABOX_ASRC_IS_TPERIOD_LIMIT_MASK;
	val = is_limit(val);
	res = update_bits_async(dev, cmpnt, "is tperiod limit", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_OS_DEFAULT(id) :
			ABOX_SPUM_ASRC_OS_DEFAULT(id);
	mask = ABOX_ASRC_OS_DEFAULT_MASK;
	val = ofactor;
	res = update_bits_async(dev, cmpnt, "os default", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_OS_TPLIMIT(id) :
			ABOX_SPUM_ASRC_OS_TPLIMIT(id);
	mask = ABOX_ASRC_OS_TPERIOD_LIMIT_MASK;
	val = os_limit(val);
	res = update_bits_async(dev, cmpnt, "os tperiod limit", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_FILTER_CTRL(id) :
			ABOX_SPUM_ASRC_FILTER_CTRL(id);
	mask = ABOX_ASRC_APF_COEF_SEL_MASK;
	val = apf_coef << ABOX_ASRC_APF_COEF_SEL_L;
	res = update_bits_async(dev, cmpnt, "apf coef sel", reg, mask, val);
	if (res < 0)
		ret = res;

	res = asrc_update_tick(data, stream, id);
	if (res < 0)
		ret = res;

	snd_soc_component_async_complete(cmpnt);

	return ret;
}

static int asrc_config_sync(struct abox_data *data, int id, int stream,
		unsigned int isr, unsigned int osr, int tune,
		unsigned int width, int apf_coef)
{
	struct device *dev = data->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	bool spus = (stream == SNDRV_PCM_STREAM_PLAYBACK);
	const struct asrc_ctrl *ctrl;
	unsigned int reg, mask, val;
	int res, ret = 0;

	abox_dbg(dev, "%s(%d, %d, %uHz, %uHz, %d, %ubit, %d)\n", __func__,
			id, stream, isr, osr, tune, width, apf_coef);

	ctrl = &asrc_table[to_asrc_rate(isr)][to_asrc_rate(osr)];

	reg = spus ? ABOX_SPUS_ASRC_CTRL(id) : ABOX_SPUM_ASRC_CTRL(id);
	mask = ABOX_ASRC_BIT_WIDTH_MASK;
	val = ((width / 8) - 1) << ABOX_ASRC_BIT_WIDTH_L;
	res = update_bits_async(dev, cmpnt, "width", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_IS_SYNC_MODE_MASK;
	val = 0 << ABOX_ASRC_IS_SYNC_MODE_L;
	res = update_bits_async(dev, cmpnt, "is sync", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OS_SYNC_MODE_MASK;
	val = 0 << ABOX_ASRC_OS_SYNC_MODE_L;
	res = update_bits_async(dev, cmpnt, "os sync", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OVSF_RATIO_MASK;
	val = ctrl->ovsf << ABOX_ASRC_OVSF_RATIO_L;
	res = update_bits_async(dev, cmpnt, "ovsf ratio", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_DCMF_RATIO_MASK;
	val = ctrl->dcmf << ABOX_ASRC_DCMF_RATIO_L;
	res = update_bits_async(dev, cmpnt, "dcmf ratio", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_IS_SOURCE_SEL_MASK;
	val = TICK_SYNC << ABOX_ASRC_IS_SOURCE_SEL_L;
	res = update_bits_async(dev, cmpnt, "is source", reg, mask, val);
	if (res < 0)
		ret = res;

	mask = ABOX_ASRC_OS_SOURCE_SEL_MASK;
	val = TICK_SYNC << ABOX_ASRC_OS_SOURCE_SEL_L;
	res = update_bits_async(dev, cmpnt, "os source", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_IS_DEFAULT(id) :
			ABOX_SPUM_ASRC_IS_DEFAULT(id);
	mask = ABOX_ASRC_IS_DEFAULT_MASK;
	val = ctrl->ifactor;
	res = update_bits_async(dev, cmpnt, "is default", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_IS_TPLIMIT(id) :
			ABOX_SPUM_ASRC_IS_TPLIMIT(id);
	mask = ABOX_ASRC_IS_TPERIOD_LIMIT_MASK;
	val = is_limit(val);
	res = update_bits_async(dev, cmpnt, "is tperiod limit", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_OS_DEFAULT(id) :
			ABOX_SPUM_ASRC_OS_DEFAULT(id);
	mask = ABOX_ASRC_OS_DEFAULT_MASK;
	val = cal_ofactor(ctrl, tune);
	res = update_bits_async(dev, cmpnt, "os default", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_OS_TPLIMIT(id) :
			ABOX_SPUM_ASRC_OS_TPLIMIT(id);
	mask = ABOX_ASRC_OS_TPERIOD_LIMIT_MASK;
	val = os_limit(val);
	res = update_bits_async(dev, cmpnt, "os tperiod limit", reg, mask, val);
	if (res < 0)
		ret = res;

	reg = spus ? ABOX_SPUS_ASRC_FILTER_CTRL(id) :
			ABOX_SPUM_ASRC_FILTER_CTRL(id);
	mask = ABOX_ASRC_APF_COEF_SEL_MASK;
	val = apf_coef << ABOX_ASRC_APF_COEF_SEL_L;
	res = update_bits_async(dev, cmpnt, "apf coef sel", reg, mask, val);
	if (res < 0)
		ret = res;

	snd_soc_component_async_complete(cmpnt);

	return ret;
}

static int asrc_config(struct abox_data *data, int id, int stream,
		enum asrc_tick itick, unsigned int isr,
		enum asrc_tick otick, unsigned int osr, int tune,
		unsigned int width, int apf_coef)
{
	struct device *dev = data->dev;
	int ret;

	abox_info(dev, "%s(%d, %d, %d, %uHz, %d, %uHz, %d, %ubit, %d)\n",
			__func__, id, stream, itick, isr, otick, osr, tune,
			width, apf_coef);

	if ((itick == TICK_SYNC) && (otick == TICK_SYNC)) {
		ret = asrc_config_sync(data, id, stream, isr, osr, tune,
				width, apf_coef);
	} else {
		if (itick == TICK_CP) {
			ret = asrc_config_async(data, id, stream,
					itick, isr, otick, osr, tune,
					s_default, width, apf_coef);
		} else if (otick == TICK_CP) {
			ret = asrc_config_async(data, id, stream,
					itick, isr, otick, osr, tune,
					s_default, width, apf_coef);
		} else {
			abox_err(dev, "not supported\n");
			ret = -EINVAL;
		}
	}

	return ret;
}

#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E9945)
static int ssrc_set(struct abox_data *data, int idx, int id, int stream,
	unsigned int rate, unsigned int tgt_rate, unsigned int tgt_width)
{
	struct device *dev = data->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct snd_soc_dapm_widget *w = asrc_get_widget(cmpnt, idx, stream);
	const struct asrc_ctrl *ctrl;
	bool force_enable;
	unsigned int reg, mask, val;
	int on, ret;

	abox_info(dev, "%s(%d, %dcore, %uHz -> %uHz, %ubit)\n", __func__,
		idx, id, rate, tgt_rate, tgt_width);

	reg = ABOX_SPUS_SSRC_CTRL(id - SSRC_START_ID);

	/*Select SSRC Core ID*/
	asrc_put_id(cmpnt, idx, SNDRV_PCM_STREAM_PLAYBACK, id);

	/*Set SSRC configuration*/
	ctrl = &ssrc_table[to_asrc_rate(rate)][to_asrc_rate(tgt_rate)];
	mask = ABOX_SSRC_OVSF_RATIO_MASK;
	val = ctrl->ovsf << ABOX_SSRC_OVSF_RATIO_L;
	ret = update_bits_async(dev, cmpnt, "ovsf ratio", reg, mask, val);
	if (ret < 0)
		return ret;

	/*Enable SSRC*/
	force_enable = spus_asrc_force_enable[idx];
	on = force_enable || (rate != tgt_rate);
	ret = asrc_put_active(w, SNDRV_PCM_STREAM_PLAYBACK, on);

	return ret;
}
#endif

static int asrc_set(struct abox_data *data, int stream, int idx,
		unsigned int channels, unsigned int rate, unsigned int tgt_rate,
		unsigned int tgt_width, int tune)
{
	struct device *dev = data->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	struct snd_soc_dapm_widget *w = asrc_get_widget(cmpnt, idx, stream);
	enum asrc_tick itick, otick;
	int apf_coef = get_apf_coef(data, stream, idx);
	bool force_enable;
	int on, ret;

#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E9945)
	int id;
	id = asrc_get_lock_id(stream, asrc_get_idx(w));
	if (id > 0 && id >= SSRC_START_ID) {
		ret = ssrc_set(data, idx, id, stream, rate, tgt_rate, tgt_width);
		return ret;
	}
#endif
	abox_dbg(dev, "%s(%d, %d, %d, %uHz, %uHz, %ubit, %d)\n", __func__,
			stream, idx, channels, rate, tgt_rate, tgt_width, tune);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		force_enable = spus_asrc_force_enable[idx];
		itick = spus_asrc_is[idx];
		otick = spus_asrc_os[idx];
	} else {
		force_enable = spum_asrc_force_enable[idx];
		itick = spum_asrc_is[idx];
		otick = spum_asrc_os[idx];
	}

	on = force_enable || (rate != tgt_rate) ||
			(itick != TICK_SYNC) || (otick != TICK_SYNC) ||
			(tune != ABOX_ASRC_TUNE_DEFAULT);

	if (!on)
		goto out;

	ret = asrc_assign_id(w, stream, channels);
	if (ret < 0) {
		abox_err(dev, "%s: assign failed: %d\n", __func__, ret);
		on = false;
		goto out;
	}

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ret = asrc_config(data, asrc_find_id(w, stream), stream,
				itick, rate, otick, tgt_rate, tune,
				tgt_width, apf_coef);
	} else {
		ret = asrc_config(data, asrc_find_id(w, stream), stream,
				itick, tgt_rate, otick, rate, tune,
				tgt_width, apf_coef);
	}
	if (ret < 0) {
		abox_err(dev, "%s: config failed: %d\n", __func__, ret);
		on = false;
	}
out:
	ret = asrc_put_active(w, stream, on);

	return ret;
}

static int asrc_event(struct snd_soc_dapm_widget *w, int e, int stream)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	int idx = asrc_get_idx(w);
	struct device *dev_dma;
	struct snd_soc_dai *dai_dma;
	struct snd_pcm_hw_params params, tgt_params;
	unsigned int tgt_rate = 0, tgt_width = 0;
	unsigned int rate = 0, width = 0, channels = 0;
	int id, tune, ret = 0;

	abox_dbg(dev, "%s(%s, %d)\n", __func__, w->name, e);

	switch (e) {
	case 0x0:
		/* special case internally defined in abox driver
		 * fall-through
		 */
	case SND_SOC_DAPM_PRE_PMU:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			dev_dma = data->dev_rdma[idx];
		else
			dev_dma = data->dev_wdma[idx];

		ret = abox_dma_hw_params_fixup(dev_dma, &params);
		if (ret < 0) {
			abox_err(dev_dma, "hw params get failed: %d\n", ret);
			break;
		}

		rate = params_rate(&params);
		width = params_width(&params);
		channels = params_channels(&params);
		if (!rate || !width || !channels) {
			abox_err(dev_dma, "hw params invalid: %u %u %u\n",
					rate, width, channels);
			ret = -EINVAL;
			break;
		}

		dai_dma = abox_dma_get_dai(dev_dma, DMA_DAI_PCM);
		if (IS_ERR_OR_NULL(dai_dma)) {
			abox_err(dev_dma, "dai get failed: %ld\n",
					PTR_ERR(dai_dma));
			break;
		}

		tgt_params = params;
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			ret = rdma_hw_params_fixup(dai_dma, &tgt_params);
			tgt_width = get_width_for_spus_atune(data, idx);
		} else {
			ret = wdma_hw_params_fixup(dai_dma, &tgt_params);
			tgt_width = get_width_for_spum_atune(data, idx);
		}
		if (ret < 0)
			abox_err(dev_dma, "hw params fixup failed: %d\n", ret);

		tgt_rate = params_rate(&tgt_params);
		tgt_width = tgt_width ? : params_width(&tgt_params);

		tune = get_asrc_tune(stream, idx);

		ret = asrc_set(data, stream, idx, channels, rate, tgt_rate,
				tgt_width, tune);
		if (ret < 0)
			abox_err(dev, "%s: set failed: %d\n", __func__, ret);
		if (e) {
			ret = abox_atune_dapm_event(data, e, stream, idx);
			if (ret < 0)
				abox_err(dev, "%s: atune set failed: %d\n",
						__func__, ret);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* ASRC will be released in DMA stop. */
		break;
	}

	if (asrc_get_active(data->cmpnt, idx, stream) && e) {
		id = asrc_get_id(data->cmpnt, idx, stream);
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			notify_event(data, ABOX_WIDGET_SPUS_ASRC0 + id, e);
		else
			notify_event(data, ABOX_WIDGET_SPUM_ASRC0 + id, e);
	}

	return ret;
}

static int asrc_update(struct snd_soc_component *cmpnt, int idx, int stream)
{
	return asrc_event(asrc_get_widget(cmpnt, idx, stream), 0x0, stream);
}

static int spus_asrc_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *k, int e)
{
	return asrc_event(w, e, SNDRV_PCM_STREAM_PLAYBACK);
}

static int spum_asrc_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *k, int e)
{
	return asrc_event(w, e, SNDRV_PCM_STREAM_CAPTURE);
}

static const char *const spus_inx_texts[] = {
	"RDMA", "SIFSM", "RESERVED", "SIFST",
};
static SOC_ENUM_SINGLE_DECL(spus_in0_enum, ABOX_SPUS_CTRL_FC_SRC(0),
		ABOX_FUNC_CHAIN_SRC_IN_L(0), spus_inx_texts);
static const struct snd_kcontrol_new spus_in0_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in0_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in1_enum, ABOX_SPUS_CTRL_FC_SRC(1),
		ABOX_FUNC_CHAIN_SRC_IN_L(1), spus_inx_texts);
static const struct snd_kcontrol_new spus_in1_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in1_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in2_enum, ABOX_SPUS_CTRL_FC_SRC(2),
		ABOX_FUNC_CHAIN_SRC_IN_L(2), spus_inx_texts);
static const struct snd_kcontrol_new spus_in2_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in2_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in3_enum, ABOX_SPUS_CTRL_FC_SRC(3),
		ABOX_FUNC_CHAIN_SRC_IN_L(3), spus_inx_texts);
static const struct snd_kcontrol_new spus_in3_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in3_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in4_enum, ABOX_SPUS_CTRL_FC_SRC(4),
		ABOX_FUNC_CHAIN_SRC_IN_L(4), spus_inx_texts);
static const struct snd_kcontrol_new spus_in4_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in4_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in5_enum, ABOX_SPUS_CTRL_FC_SRC(5),
		ABOX_FUNC_CHAIN_SRC_IN_L(5), spus_inx_texts);
static const struct snd_kcontrol_new spus_in5_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in5_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in6_enum, ABOX_SPUS_CTRL_FC_SRC(6),
		ABOX_FUNC_CHAIN_SRC_IN_L(6), spus_inx_texts);
static const struct snd_kcontrol_new spus_in6_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in6_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in7_enum, ABOX_SPUS_CTRL_FC_SRC(7),
		ABOX_FUNC_CHAIN_SRC_IN_L(7), spus_inx_texts);
static const struct snd_kcontrol_new spus_in7_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in7_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in8_enum, ABOX_SPUS_CTRL_FC_SRC(8),
		ABOX_FUNC_CHAIN_SRC_IN_L(8), spus_inx_texts);
static const struct snd_kcontrol_new spus_in8_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in8_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in9_enum, ABOX_SPUS_CTRL_FC_SRC(9),
		ABOX_FUNC_CHAIN_SRC_IN_L(9), spus_inx_texts);
static const struct snd_kcontrol_new spus_in9_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in9_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in10_enum, ABOX_SPUS_CTRL_FC_SRC(10),
		ABOX_FUNC_CHAIN_SRC_IN_L(10), spus_inx_texts);
static const struct snd_kcontrol_new spus_in10_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in10_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in11_enum, ABOX_SPUS_CTRL_FC_SRC(11),
		ABOX_FUNC_CHAIN_SRC_IN_L(11), spus_inx_texts);
static const struct snd_kcontrol_new spus_in11_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in11_enum),
};
static SOC_ENUM_SINGLE_DECL(spus_in12_enum, ABOX_SPUS_CTRL_FC_SRC(12),
		ABOX_FUNC_CHAIN_SRC_IN_L(12), spus_inx_texts);
static const struct snd_kcontrol_new spus_in12_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in12_enum),
};

static SOC_ENUM_SINGLE_DECL(spus_in13_enum, ABOX_SPUS_CTRL_FC_SRC(13),
		ABOX_FUNC_CHAIN_SRC_IN_L(13), spus_inx_texts);
static const struct snd_kcontrol_new spus_in13_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in13_enum),
};

static SOC_ENUM_SINGLE_DECL(spus_in14_enum, ABOX_SPUS_CTRL_FC_SRC(14),
		ABOX_FUNC_CHAIN_SRC_IN_L(14), spus_inx_texts);
static const struct snd_kcontrol_new spus_in14_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in14_enum),
};

static SOC_ENUM_SINGLE_DECL(spus_in15_enum, ABOX_SPUS_CTRL_FC_SRC(15),
		ABOX_FUNC_CHAIN_SRC_IN_L(15), spus_inx_texts);
static const struct snd_kcontrol_new spus_in15_controls[] = {
	SOC_DAPM_ENUM("MUX", spus_in15_enum),
};

static const char *const spus_outx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2", "SIFS3",
	"SIFS4", "SIFS5", "SIFS6", "SIFS7",
};
#define SPUS_OUT(x)	((x) ? ((x) << 1) : 0x1)
static const unsigned int spus_outx_values[] = {
	0x0, SPUS_OUT(0), SPUS_OUT(1), SPUS_OUT(2), SPUS_OUT(3),
	SPUS_OUT(4), SPUS_OUT(5), SPUS_OUT(6), SPUS_OUT(7),
};
static const unsigned int spus_outx_mask = ABOX_FUNC_CHAIN_SRC_OUT_MASK(0) >>
		ABOX_FUNC_CHAIN_SRC_OUT_L(0);

static int spus_outx_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol, enum abox_dai spus)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	struct snd_soc_component *cmpnt;
	enum abox_dai dai;
	int ret;

	if (item[0] >= e->items)
		return -EINVAL;

	cmpnt = snd_soc_dapm_kcontrol_dapm(kcontrol)->component;
	dai = ABOX_SIFS0 + (item[0] - 1);
	ret = set_spus_sink(cmpnt, spus, dai);
	if (ret < 0)
		return ret;

	return snd_soc_dapm_put_enum_double(kcontrol, ucontrol);
}

static int spus_out0_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA0);
}
static int spus_out1_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA1);
}
static int spus_out2_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA2);
}
static int spus_out3_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA3);
}
static int spus_out4_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA4);
}
static int spus_out5_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA5);
}
static int spus_out6_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA6);
}
static int spus_out7_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA7);
}
static int spus_out8_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA8);
}
static int spus_out9_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA9);
}
static int spus_out10_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA10);
}
static int spus_out11_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA11);
}
static int spus_out12_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA12);
}
static int spus_out13_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA13);
}
static int spus_out14_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA14);
}
static int spus_out15_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return spus_outx_put(kcontrol, ucontrol, ABOX_RDMA15);
}

static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out0_enum,
		ABOX_SPUS_CTRL_FC_SRC(0), ABOX_FUNC_CHAIN_SRC_OUT_L(0),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out0_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out0_enum, snd_soc_dapm_get_enum_double,
			spus_out0_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out1_enum,
		ABOX_SPUS_CTRL_FC_SRC(1), ABOX_FUNC_CHAIN_SRC_OUT_L(1),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out1_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out1_enum, snd_soc_dapm_get_enum_double,
			spus_out1_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out2_enum,
		ABOX_SPUS_CTRL_FC_SRC(2), ABOX_FUNC_CHAIN_SRC_OUT_L(2),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out2_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out2_enum, snd_soc_dapm_get_enum_double,
			spus_out2_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out3_enum,
		ABOX_SPUS_CTRL_FC_SRC(3), ABOX_FUNC_CHAIN_SRC_OUT_L(3),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out3_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out3_enum, snd_soc_dapm_get_enum_double,
			spus_out3_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out4_enum,
		ABOX_SPUS_CTRL_FC_SRC(4), ABOX_FUNC_CHAIN_SRC_OUT_L(4),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out4_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out4_enum, snd_soc_dapm_get_enum_double,
			spus_out4_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out5_enum,
		ABOX_SPUS_CTRL_FC_SRC(5), ABOX_FUNC_CHAIN_SRC_OUT_L(5),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out5_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out5_enum, snd_soc_dapm_get_enum_double,
			spus_out5_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out6_enum,
		ABOX_SPUS_CTRL_FC_SRC(6), ABOX_FUNC_CHAIN_SRC_OUT_L(6),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out6_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out6_enum, snd_soc_dapm_get_enum_double,
			spus_out6_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out7_enum,
		ABOX_SPUS_CTRL_FC_SRC(7), ABOX_FUNC_CHAIN_SRC_OUT_L(7),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out7_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out7_enum, snd_soc_dapm_get_enum_double,
			spus_out7_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out8_enum,
		ABOX_SPUS_CTRL_FC_SRC(8), ABOX_FUNC_CHAIN_SRC_OUT_L(8),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out8_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out8_enum, snd_soc_dapm_get_enum_double,
			spus_out8_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out9_enum,
		ABOX_SPUS_CTRL_FC_SRC(9), ABOX_FUNC_CHAIN_SRC_OUT_L(9),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out9_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out9_enum, snd_soc_dapm_get_enum_double,
			spus_out9_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out10_enum,
		ABOX_SPUS_CTRL_FC_SRC(10), ABOX_FUNC_CHAIN_SRC_OUT_L(10),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out10_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out10_enum,
			snd_soc_dapm_get_enum_double, spus_out10_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out11_enum,
		ABOX_SPUS_CTRL_FC_SRC(11), ABOX_FUNC_CHAIN_SRC_OUT_L(11),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out11_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out11_enum,
			snd_soc_dapm_get_enum_double, spus_out11_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out12_enum,
		ABOX_SPUS_CTRL_FC_SRC(12), ABOX_FUNC_CHAIN_SRC_OUT_L(12),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out12_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out12_enum,
			snd_soc_dapm_get_enum_double, spus_out12_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out13_enum,
		ABOX_SPUS_CTRL_FC_SRC(13), ABOX_FUNC_CHAIN_SRC_OUT_L(13),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out13_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out13_enum,
			snd_soc_dapm_get_enum_double, spus_out13_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out14_enum,
		ABOX_SPUS_CTRL_FC_SRC(14), ABOX_FUNC_CHAIN_SRC_OUT_L(14),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out14_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out14_enum,
			snd_soc_dapm_get_enum_double, spus_out14_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(spus_out15_enum,
		ABOX_SPUS_CTRL_FC_SRC(15), ABOX_FUNC_CHAIN_SRC_OUT_L(15),
		spus_outx_mask, spus_outx_texts, spus_outx_values);
static const struct snd_kcontrol_new spus_out15_controls[] = {
	SOC_DAPM_ENUM_EXT("DEMUX", spus_out15_enum,
			snd_soc_dapm_get_enum_double, spus_out15_put),
};

static const char *const spusm_texts[] = {
	"RESERVED",
	"UAIF0", "UAIF1", "UAIF2", "UAIF3",
	"UAIF4", "UAIF5", "UAIF6",
	"SPDY",
};
static const unsigned int spusm_values[] = {
	0x0,
	0x8, 0x9, 0xa, 0xb,
	0xc, 0xd, 0xe,
	0x11
};

static const unsigned int spusm_mask =
		ABOX_ROUTE_SPUSM_MASK >> ABOX_ROUTE_SPUSM_L;
static SOC_VALUE_ENUM_SINGLE_DECL(spusm_enum, ABOX_ROUTE_CTRL_SIFM,
		ABOX_ROUTE_SPUSM_L, spusm_mask, spusm_texts, spusm_values);
static const struct snd_kcontrol_new spusm_controls[] = {
	SOC_DAPM_ENUM("MUX", spusm_enum),
};

static const unsigned int spust_mask =
		ABOX_ROUTE_SPUSM_MASK >> ABOX_ROUTE_SPUSM_L;
static SOC_VALUE_ENUM_SINGLE_DECL(spust_enum, ABOX_ROUTE_CTRL_SIFT,
		ABOX_ROUTE_SPUST_L, spust_mask, spusm_texts, spusm_values);
static const struct snd_kcontrol_new spust_controls[] = {
	SOC_DAPM_ENUM("MUX", spust_enum),
};

int abox_cmpnt_sifsm_prepare(struct device *dev, struct abox_data *data,
		enum abox_dai dai)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	enum abox_dai src;
	unsigned int reg, reg_val, val;
	int idx, ret = 0;

	abox_dbg(dev, "%s\n", __func__);

	switch (get_source_dai_id(data, dai)) {
	case ABOX_SIFSM:
		/* ToDo */
		break;
	case ABOX_SIFST:
		/* Needs force update. Flush is write only field. */
		ret = regmap_update_bits_base(cmpnt->regmap, ABOX_SIDETONE_CTRL,
			    ABOX_SDTN_FLUSH_MASK, ABOX_SDTN_FLUSH_MASK,
			    NULL, false, true);
		if (ret < 0)
			break;

		src = get_source_dai_id(data, ABOX_SIFST);
		switch (src) {
		case ABOX_UAIF0 ... ABOX_UAIF6:
			idx = src - ABOX_UAIF0;
			reg = ABOX_UAIF_CTRL1(idx);
			reg_val = snd_soc_component_read(cmpnt, reg);
			val = (reg_val & ABOX_FORMAT_MASK) >> ABOX_FORMAT_L;
			ret = snd_soc_component_update_bits(cmpnt,
					ABOX_SIDETONE_CTRL,
					ABOX_SDTN_FORMAT_MASK,
					val << ABOX_SDTN_FORMAT_L);
			break;
		default:
			ret = -EINVAL;
			break;
		}
		break;
	default:
		/* nothing to do */
		break;
	}

	return ret;
}

static const char *const sidetone_mode_texts[] = {
	"Normal", "Zero", "Bypass",
};
static SOC_ENUM_SINGLE_DECL(sidetone_mode, ABOX_SIDETONE_CTRL,
		ABOX_SDTN_ZERO_OUTPUT_L, sidetone_mode_texts);

static DECLARE_TLV_DB_LINEAR(sidetone_gain_tlv, 0, 3600);

static const char *const sidetone_headroom_hpf_texts[] = {
	"6dB", "12dB", "18dB", "24dB",
};
static const unsigned int sidetone_headroom_hpf_values[] = {
	2, 3, 4, 5,
};
static const unsigned int sidetone_headroom_hpf_mask =
		ABOX_SDTN_HEADROOM_HPF_MASK >> ABOX_SDTN_HEADROOM_HPF_L;
static SOC_VALUE_ENUM_SINGLE_DECL(sidetone_headroom_hpf,
		ABOX_SIDETONE_FILTER_CTRL0, ABOX_SDTN_HEADROOM_HPF_L,
		sidetone_headroom_hpf_mask, sidetone_headroom_hpf_texts,
		sidetone_headroom_hpf_values);

static const char *const sidetone_headroom_eq_texts[] = {
	"12dB", "18dB", "24dB",
};
static const unsigned int sidetone_headroom_eq_values[] = {
	3, 4, 5,
};
static const unsigned int sidetone_headroom_eq_mask =
		ABOX_SDTN_HEADROOM_PEAK0_MASK >> ABOX_SDTN_HEADROOM_PEAK0_L;
static SOC_VALUE_ENUM_SINGLE_DECL(sidetone_headroom_peak0,
		ABOX_SIDETONE_FILTER_CTRL0, ABOX_SDTN_HEADROOM_PEAK0_L,
		sidetone_headroom_eq_mask, sidetone_headroom_eq_texts,
		sidetone_headroom_eq_values);
static SOC_VALUE_ENUM_SINGLE_DECL(sidetone_headroom_peak1,
		ABOX_SIDETONE_FILTER_CTRL0, ABOX_SDTN_HEADROOM_PEAK1_L,
		sidetone_headroom_eq_mask, sidetone_headroom_eq_texts,
		sidetone_headroom_eq_values);
static SOC_VALUE_ENUM_SINGLE_DECL(sidetone_headroom_peak2,
		ABOX_SIDETONE_FILTER_CTRL0, ABOX_SDTN_HEADROOM_PEAK2_L,
		sidetone_headroom_eq_mask, sidetone_headroom_eq_texts,
		sidetone_headroom_eq_values);
static SOC_VALUE_ENUM_SINGLE_DECL(sidetone_headroom_lowsh,
		ABOX_SIDETONE_FILTER_CTRL0, ABOX_SDTN_HEADROOM_LOWSH_L,
		sidetone_headroom_eq_mask, sidetone_headroom_eq_texts,
		sidetone_headroom_eq_values);
static SOC_VALUE_ENUM_SINGLE_DECL(sidetone_headroom_highsh,
		ABOX_SIDETONE_FILTER_CTRL0, ABOX_SDTN_HEADROOM_HIGHSH_L,
		sidetone_headroom_eq_mask, sidetone_headroom_eq_texts,
		sidetone_headroom_eq_values);

static struct snd_kcontrol_new sidetone_controls[] = {
	SOC_SINGLE("SIDETONE IN CH", ABOX_SIDETONE_CTRL,
			ABOX_SDTN_CH_SEL_IN_L, 7, 0),
	SOC_SINGLE("SIDETONE OUT CH", ABOX_SIDETONE_CTRL,
			ABOX_SDTN_CH_SEL_OUT_L, 7, 0),
	SOC_SINGLE("SIDETONE OUT2 CH", ABOX_SIDETONE_CTRL,
			ABOX_SDTN_CH_SEL_OUT2_L, 7, 0),
	SOC_SINGLE("SIDETONE OUT2 EN", ABOX_SIDETONE_CTRL,
			ABOX_SDTN_OUT2_ENABLE_L, 1, 0),
	SOC_ENUM("SIDETONE MODE", sidetone_mode),
	SOC_SINGLE("SIDETONE HPF EN", ABOX_SIDETONE_CTRL,
			ABOX_SDTN_HPF_ENABLE_L, 1, 0),
	SOC_SINGLE("SIDETONE EQ EN", ABOX_SIDETONE_CTRL,
			ABOX_SDTN_EQ_ENABLE_L, 1, 0),
	SOC_SINGLE("SIDETONE GAIN IN EN", ABOX_SIDETONE_CTRL,
			ABOX_SDTN_GAIN_IN_ENABLE_L, 1, 0),
	SOC_SINGLE("SIDETONE GAIN OUT EN", ABOX_SIDETONE_CTRL,
			ABOX_SDTN_GAIN_OUT_ENABLE_L, 1, 0),
	SOC_SINGLE_TLV("SIDETONE GAIN IN", ABOX_SIDETONE_GAIN_CTRL,
			ABOX_SDTN_GAIN_IN_L, 133, 0, sidetone_gain_tlv),
	SOC_SINGLE_TLV("SIDETONE GAIN OUT", ABOX_SIDETONE_GAIN_CTRL,
			ABOX_SDTN_GAIN_OUT_L, 133, 0, sidetone_gain_tlv),
	SOC_ENUM("SIDETONE HEADROOM HPF", sidetone_headroom_hpf),
	SOC_ENUM("SIDETONE HEADROOM PEAK0", sidetone_headroom_peak0),
	SOC_ENUM("SIDETONE HEADROOM PEAK1", sidetone_headroom_peak1),
	SOC_ENUM("SIDETONE HEADROOM PEAK2", sidetone_headroom_peak2),
	SOC_ENUM("SIDETONE HEADROOM LOWSH", sidetone_headroom_lowsh),
	SOC_ENUM("SIDETONE HEADROOM HIGHSH", sidetone_headroom_highsh),
	SOC_SINGLE("SIDETONE POSTAMP HPF", ABOX_SIDETONE_FILTER_CTRL1,
			ABOX_SDTN_POSTAMP_HPF_L, 3, 0),
	SOC_SINGLE("SIDETONE POSTAMP PEAK0", ABOX_SIDETONE_FILTER_CTRL1,
			ABOX_SDTN_POSTAMP_PEAK0_L, 3, 0),
	SOC_SINGLE("SIDETONE POSTAMP PEAK1", ABOX_SIDETONE_FILTER_CTRL1,
			ABOX_SDTN_POSTAMP_PEAK1_L, 3, 0),
	SOC_SINGLE("SIDETONE POSTAMP PEAK2", ABOX_SIDETONE_FILTER_CTRL1,
			ABOX_SDTN_POSTAMP_PEAK2_L, 3, 0),
	SOC_SINGLE("SIDETONE POSTAMP LOWSH", ABOX_SIDETONE_FILTER_CTRL1,
			ABOX_SDTN_POSTAMP_LOWSH_L, 3, 0),
	SOC_SINGLE("SIDETONE POSTAMP HIGHSH", ABOX_SIDETONE_FILTER_CTRL1,
			ABOX_SDTN_POSTAMP_HIGHSH_L, 3, 0),
	SOC_SINGLE_XR_SX("SIDETONE HPF COEF0", ABOX_SIDETONE_HPF_COEF0,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HPF COEF1", ABOX_SIDETONE_HPF_COEF1,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HPF COEF2", ABOX_SIDETONE_HPF_COEF2,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HPF COEF3", ABOX_SIDETONE_HPF_COEF3,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HPF COEF4", ABOX_SIDETONE_HPF_COEF4,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK0 COEF0", ABOX_SIDETONE_PEAK0_COEF0,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK0 COEF1", ABOX_SIDETONE_PEAK0_COEF1,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK0 COEF2", ABOX_SIDETONE_PEAK0_COEF2,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK0 COEF3", ABOX_SIDETONE_PEAK0_COEF3,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK0 COEF4", ABOX_SIDETONE_PEAK0_COEF4,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK1 COEF0", ABOX_SIDETONE_PEAK1_COEF0,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK1 COEF1", ABOX_SIDETONE_PEAK1_COEF1,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK1 COEF2", ABOX_SIDETONE_PEAK1_COEF2,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK1 COEF3", ABOX_SIDETONE_PEAK1_COEF3,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK1 COEF4", ABOX_SIDETONE_PEAK1_COEF4,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK2 COEF0", ABOX_SIDETONE_PEAK2_COEF0,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK2 COEF1", ABOX_SIDETONE_PEAK2_COEF1,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK2 COEF2", ABOX_SIDETONE_PEAK2_COEF2,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK2 COEF3", ABOX_SIDETONE_PEAK2_COEF3,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE PEAK2 COEF4", ABOX_SIDETONE_PEAK2_COEF4,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE LOWSH COEF0", ABOX_SIDETONE_LOWSH_COEF0,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE LOWSH COEF1", ABOX_SIDETONE_LOWSH_COEF1,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE LOWSH COEF2", ABOX_SIDETONE_LOWSH_COEF2,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE LOWSH COEF3", ABOX_SIDETONE_LOWSH_COEF3,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE LOWSH COEF4", ABOX_SIDETONE_LOWSH_COEF4,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HIGHSH COEF0", ABOX_SIDETONE_HIGHSH_COEF0,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HIGHSH COEF1", ABOX_SIDETONE_HIGHSH_COEF1,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HIGHSH COEF2", ABOX_SIDETONE_HIGHSH_COEF2,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HIGHSH COEF3", ABOX_SIDETONE_HIGHSH_COEF3,
			1, 32, INT_MIN, INT_MAX, 0),
	SOC_SINGLE_XR_SX("SIDETONE HIGHSH COEF4", ABOX_SIDETONE_HIGHSH_COEF4,
			1, 32, INT_MIN, INT_MAX, 0),
};

static const struct snd_soc_dapm_route sidetone_routes[] = {
	{"SIDETONE", NULL, "SPUST"},
	{"SIFST", NULL, "SIDETONE"},
};

static const char *const sifsx_texts[] = {
	"SPUS OUT0", "SPUS OUT1", "SPUS OUT2", "SPUS OUT3",
	"SPUS OUT4", "SPUS OUT5", "SPUS OUT6", "SPUS OUT7",
	"SPUS OUT8", "SPUS OUT9", "SPUS OUT10", "SPUS OUT11",
	"SPUS OUT12", "SPUS OUT13", "SPUS OUT14", "SPUS OUT15",
};
static SOC_ENUM_SINGLE_DECL(sifs1_enum, ABOX_SPUS_CTRL_SIFS_OUT_SEL(1),
		ABOX_SIFS_OUT_SEL_L(1), sifsx_texts);
static const struct snd_kcontrol_new sifs1_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs1_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs2_enum, ABOX_SPUS_CTRL_SIFS_OUT_SEL(2),
		ABOX_SIFS_OUT_SEL_L(2), sifsx_texts);
static const struct snd_kcontrol_new sifs2_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs2_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs3_enum, ABOX_SPUS_CTRL_SIFS_OUT_SEL(3),
		ABOX_SIFS_OUT_SEL_L(3), sifsx_texts);
static const struct snd_kcontrol_new sifs3_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs3_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs4_enum, ABOX_SPUS_CTRL_SIFS_OUT_SEL(4),
		ABOX_SIFS_OUT_SEL_L(4), sifsx_texts);
static const struct snd_kcontrol_new sifs4_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs4_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs5_enum, ABOX_SPUS_CTRL_SIFS_OUT_SEL(5),
		ABOX_SIFS_OUT_SEL_L(5), sifsx_texts);
static const struct snd_kcontrol_new sifs5_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs5_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs6_enum, ABOX_SPUS_CTRL_SIFS_OUT_SEL(6),
		ABOX_SIFS_OUT_SEL_L(6), sifsx_texts);
static const struct snd_kcontrol_new sifs6_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs6_enum),
};
static SOC_ENUM_SINGLE_DECL(sifs7_enum, ABOX_SPUS_CTRL_SIFS_OUT_SEL(7),
		ABOX_SIFS_OUT_SEL_L(7), sifsx_texts);
static const struct snd_kcontrol_new sifs7_controls[] = {
	SOC_DAPM_ENUM("MUX", sifs7_enum),
};

static int sifs0_switch_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_dapm_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	int en = (int)ucontrol->value.integer.value[0];
	int ret = 0, udma_wr_ctrl_0;

	if (!pm_runtime_suspended(data->dev) && !en) {
		udma_wr_ctrl_0 = readl(data->sfr_base + ABOX_UDMA_WR_CTRL(0));
		udma_wr_ctrl_0 &= 0x1;
		if (udma_wr_ctrl_0) {
			data->udma_stop = false;
			wait_event_timeout(data->udma_fade_done,
						data->udma_stop ,
						msecs_to_jiffies(200));
			abox_info(dev, "%s: done\n", __func__);
		}
	}

	snd_soc_dapm_put_volsw(kcontrol, ucontrol);

	return ret;
}

static const struct snd_kcontrol_new sifs0_out_controls[] = {
	SOC_SINGLE_EXT("Switch", SND_SOC_NOPM, 0, 1, 1,
		snd_soc_dapm_get_volsw, sifs0_switch_put),
};
static const struct snd_kcontrol_new sifs1_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs2_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs3_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs4_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs5_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs6_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new sifs7_out_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};


static const struct snd_kcontrol_new nsrc0_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc1_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc2_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc3_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc4_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc5_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc6_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc7_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc8_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc9_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc10_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc11_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc12_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc13_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc14_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new nsrc15_in_controls[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 1),
};

static const char *const sifsm_texts[] = {
	"SPUS IN0", "SPUS IN1", "SPUS IN2", "SPUS IN3",
	"SPUS IN4", "SPUS IN5", "SPUS IN6", "SPUS IN7",
	"SPUS IN8", "SPUS IN9", "SPUS IN10", "SPUS IN11",
	"SPUS IN12", "SPUS IN13", "SPUS IN14", "SPUS IN15",
};
static SOC_ENUM_SINGLE_DECL(sifsm_enum, ABOX_SPUS_CTRL_SIFM_SEL,
		ABOX_SIFSM_IN_SEL_L, sifsm_texts);
static const struct snd_kcontrol_new sifsm_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifsm_enum),
};

static SOC_ENUM_SINGLE_DECL(sifst_enum, ABOX_SPUS_CTRL_SIFM_SEL,
		ABOX_SIFST_IN_SEL_L, sifsm_texts);
static const struct snd_kcontrol_new sifst_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifst_enum),
};

static const char *const uaif_spkx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"SIFS3", "SIFS4", "SIFS5", "SIFS6",
	"SIFS7", "RESERVED", "RESERVED", "RESERVED",
	"SIFMS",
};

static SOC_ENUM_SINGLE_DECL(uaif0_spk_enum, ABOX_ROUTE_CTRL_UAIF_SPK(0),
		ABOX_ROUTE_UAIF_SPK_L(0), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif0_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif0_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif1_spk_enum, ABOX_ROUTE_CTRL_UAIF_SPK(1),
		ABOX_ROUTE_UAIF_SPK_L(1), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif1_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif1_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif2_spk_enum, ABOX_ROUTE_CTRL_UAIF_SPK(2),
		ABOX_ROUTE_UAIF_SPK_L(2), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif2_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif2_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif3_spk_enum, ABOX_ROUTE_CTRL_UAIF_SPK(3),
		ABOX_ROUTE_UAIF_SPK_L(3), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif3_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif3_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif4_spk_enum, ABOX_ROUTE_CTRL_UAIF_SPK(4),
		ABOX_ROUTE_UAIF_SPK_L(4), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif4_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif4_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif5_spk_enum, ABOX_ROUTE_CTRL_UAIF_SPK(5),
		ABOX_ROUTE_UAIF_SPK_L(5), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif5_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif5_spk_enum),
};
static SOC_ENUM_SINGLE_DECL(uaif6_spk_enum, ABOX_ROUTE_CTRL_UAIF_SPK(6),
		ABOX_ROUTE_UAIF_SPK_L(6), uaif_spkx_texts);
static const struct snd_kcontrol_new uaif6_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", uaif6_spk_enum),
};

static const char *const dsif_spk_texts[] = {
	"RESERVED", "RESERVED", "SIFS1", "SIFS2",
	"SIFS3", "SIFS4", "SIFS5", "SIFS6",
	"SIFS7",
};
static SOC_ENUM_SINGLE_DECL(dsif_spk_enum, ABOX_ROUTE_CTRL_DSIF,
		ABOX_ROUTE_DSIF_L, dsif_spk_texts);
static const struct snd_kcontrol_new dsif_spk_controls[] = {
	SOC_DAPM_ENUM("MUX", dsif_spk_enum),
};

static const struct snd_kcontrol_new uaif0_controls[] = {
	SOC_DAPM_SINGLE("UAIF0 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif1_controls[] = {
	SOC_DAPM_SINGLE("UAIF1 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif2_controls[] = {
	SOC_DAPM_SINGLE("UAIF2 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif3_controls[] = {
	SOC_DAPM_SINGLE("UAIF3 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif4_controls[] = {
	SOC_DAPM_SINGLE("UAIF4 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif5_controls[] = {
	SOC_DAPM_SINGLE("UAIF5 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new uaif6_controls[] = {
	SOC_DAPM_SINGLE("UAIF6 Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new dsif_controls[] = {
	SOC_DAPM_SINGLE("DSIF Switch", SND_SOC_NOPM, 0, 1, 1),
};
static const struct snd_kcontrol_new spdy_controls[] = {
	SOC_DAPM_SINGLE("SPDY Switch", SND_SOC_NOPM, 0, 1, 1),
};

static const char *const nsrcx_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"SIFS3", "SIFS4", "SIFS5", "SIFS6",
	"SIFS7", "UAIF0", "UAIF1", "UAIF2",
	"UAIF3", "UAIF4", "UAIF5", "UAIF6",
	"UDMA_SIFS0", "SPDY",
};
static const unsigned int nsrcx_values[] = {
	0x0, 0x1, 0x2, 0x3,
	0x4, 0x5, 0x6, 0x7,
	0x8, 0x9, 0xa, 0xb,
	0xc, 0xd, 0xe, 0xf,
	0x10, 0x11,
};
static const unsigned int nsrcx_mask = ABOX_ROUTE_NSRC_MASK(0) >>
		ABOX_ROUTE_NSRC_L(0);

static const enum abox_dai nsrcx_dai[] = {
	ABOX_NONE, ABOX_SIFS0, ABOX_SIFS1, ABOX_SIFS2,
	ABOX_SIFS3, ABOX_SIFS4, ABOX_SIFS5, ABOX_SIFS6,
	ABOX_SIFS7, ABOX_UAIF0, ABOX_UAIF1, ABOX_UAIF2,
	ABOX_UAIF3, ABOX_UAIF4, ABOX_UAIF5, ABOX_UAIF6,
	ABOX_UDMA_RD0, ABOX_SPDY,
};

static int nsrcx_dai_to_val(enum abox_dai dai)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(nsrcx_values); i++)
		if (dai == nsrcx_dai[i])
			return nsrcx_values[i];

	return -EINVAL;
}

static int nsrcx_put_ipc(struct device *dev, enum abox_dai nsrc, enum abox_dai dai)
{
	ABOX_IPC_MSG msg;
	struct IPC_ABOX_CONFIG_MSG *abox_config_msg = &msg.msg.config;

	abox_dbg(dev, "%s(%#x, %#x)\n", __func__, nsrc, dai);

	if (nsrc < ABOX_NSRC0 || nsrc > ABOX_NSRC15)
		return -EINVAL;

	msg.ipcid = IPC_ABOX_CONFIG;
	abox_config_msg->msgtype = SET_ROUTE_NSRC0 + (nsrc - ABOX_NSRC0);
	abox_config_msg->param1 = nsrcx_dai_to_val(dai);
	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int nsrcx_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol, enum abox_dai nsrc)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	struct snd_soc_component *cmpnt;
	enum abox_dai dai;
	int ret;

	if (item[0] >= e->items)
		return -EINVAL;

	cmpnt = snd_soc_dapm_kcontrol_dapm(kcontrol)->component;
	dai = nsrcx_dai[item[0]];
	ret = set_nsrc_source(cmpnt, nsrc, dai);
	if (ret < 0)
		return ret;

	nsrcx_put_ipc(cmpnt->dev, nsrc, dai);

	return snd_soc_dapm_put_enum_double(kcontrol, ucontrol);
}

static int nsrc0_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC0);
}
static int nsrc1_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC1);
}
static int nsrc2_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC2);
}
static int nsrc3_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC3);
}
static int nsrc4_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC4);
}
static int nsrc5_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC5);
}
static int nsrc6_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC6);
}
static int nsrc7_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC7);
}
static int nsrc8_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC8);
}
static int nsrc9_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC9);
}
static int nsrc10_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC10);
}
static int nsrc11_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC11);
}
#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E8855)
static int nsrc12_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC12);
}
static int nsrc13_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC13);
}
static int nsrc14_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC14);
}
static int nsrc15_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return nsrcx_put(kcontrol, ucontrol, ABOX_NSRC15);
}
#endif

static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc0_enum,
		ABOX_ROUTE_CTRL_NSRC(0), ABOX_ROUTE_NSRC_L(0),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc0_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc0_enum, snd_soc_dapm_get_enum_double,
			nsrc0_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc1_enum,
		ABOX_ROUTE_CTRL_NSRC(1), ABOX_ROUTE_NSRC_L(1),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc1_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc1_enum, snd_soc_dapm_get_enum_double,
			nsrc1_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc2_enum,
		ABOX_ROUTE_CTRL_NSRC(2), ABOX_ROUTE_NSRC_L(2),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc2_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc2_enum, snd_soc_dapm_get_enum_double,
			nsrc2_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc3_enum,
		ABOX_ROUTE_CTRL_NSRC(3), ABOX_ROUTE_NSRC_L(3),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc3_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc3_enum, snd_soc_dapm_get_enum_double,
			nsrc3_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc4_enum,
		ABOX_ROUTE_CTRL_NSRC(4), ABOX_ROUTE_NSRC_L(4),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc4_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc4_enum, snd_soc_dapm_get_enum_double,
			nsrc4_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc5_enum,
		ABOX_ROUTE_CTRL_NSRC(5), ABOX_ROUTE_NSRC_L(5),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc5_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc5_enum, snd_soc_dapm_get_enum_double,
			nsrc5_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc6_enum,
		ABOX_ROUTE_CTRL_NSRC(6), ABOX_ROUTE_NSRC_L(6),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc6_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc6_enum, snd_soc_dapm_get_enum_double,
			nsrc6_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc7_enum,
		ABOX_ROUTE_CTRL_NSRC(7), ABOX_ROUTE_NSRC_L(7),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc7_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc7_enum, snd_soc_dapm_get_enum_double,
			nsrc7_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc8_enum,
		ABOX_ROUTE_CTRL_NSRC(8), ABOX_ROUTE_NSRC_L(8),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc8_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc8_enum, snd_soc_dapm_get_enum_double,
			nsrc8_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc9_enum,
		ABOX_ROUTE_CTRL_NSRC(9), ABOX_ROUTE_NSRC_L(9),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc9_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc9_enum, snd_soc_dapm_get_enum_double,
			nsrc9_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc10_enum,
		ABOX_ROUTE_CTRL_NSRC(10), ABOX_ROUTE_NSRC_L(10),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc10_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc10_enum, snd_soc_dapm_get_enum_double,
			nsrc10_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc11_enum,
		ABOX_ROUTE_CTRL_NSRC(11), ABOX_ROUTE_NSRC_L(11),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc11_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc11_enum, snd_soc_dapm_get_enum_double,
			nsrc11_put),
};
#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E8855)
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc12_enum,
		ABOX_ROUTE_CTRL_NSRC12(0), ABOX_ROUTE_NSRC12_L(0),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc12_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc12_enum, snd_soc_dapm_get_enum_double,
			nsrc12_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc13_enum,
		ABOX_ROUTE_CTRL_NSRC12(1), ABOX_ROUTE_NSRC12_L(1),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc13_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc13_enum, snd_soc_dapm_get_enum_double,
			nsrc13_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc14_enum,
		ABOX_ROUTE_CTRL_NSRC12(2), ABOX_ROUTE_NSRC12_L(2),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc14_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc14_enum, snd_soc_dapm_get_enum_double,
			nsrc14_put),
};
static SOC_VALUE_ENUM_SINGLE_AUTODISABLE_DECL(nsrc15_enum,
		ABOX_ROUTE_CTRL_NSRC12(3), ABOX_ROUTE_NSRC12_L(3),
		nsrcx_mask, nsrcx_texts, nsrcx_values);
static const struct snd_kcontrol_new nsrc15_controls[] = {
	SOC_DAPM_ENUM_EXT("MUX", nsrc15_enum, snd_soc_dapm_get_enum_double,
			nsrc15_put),
};
#endif

static const char *const sifmx_texts[] = {
	"WDMA", "SIFMS",
};
static SOC_ENUM_SINGLE_DECL(sifm0_enum, ABOX_SPUM_CTRL_FC_NSRC(0),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(0), sifmx_texts);
static const struct snd_kcontrol_new sifm0_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm0_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm1_enum, ABOX_SPUM_CTRL_FC_NSRC(1),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(1), sifmx_texts);
static const struct snd_kcontrol_new sifm1_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm1_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm2_enum, ABOX_SPUM_CTRL_FC_NSRC(2),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(2), sifmx_texts);
static const struct snd_kcontrol_new sifm2_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm2_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm3_enum, ABOX_SPUM_CTRL_FC_NSRC(3),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(3), sifmx_texts);
static const struct snd_kcontrol_new sifm3_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm3_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm4_enum, ABOX_SPUM_CTRL_FC_NSRC(4),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(4), sifmx_texts);
static const struct snd_kcontrol_new sifm4_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm4_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm5_enum, ABOX_SPUM_CTRL_FC_NSRC(5),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(5), sifmx_texts);
static const struct snd_kcontrol_new sifm5_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm5_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm6_enum, ABOX_SPUM_CTRL_FC_NSRC(6),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(6), sifmx_texts);
static const struct snd_kcontrol_new sifm6_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm6_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm7_enum, ABOX_SPUM_CTRL_FC_NSRC(7),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(7), sifmx_texts);
static const struct snd_kcontrol_new sifm7_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm7_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm8_enum, ABOX_SPUM_CTRL_FC_NSRC(8),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(8), sifmx_texts);
static const struct snd_kcontrol_new sifm8_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm8_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm9_enum, ABOX_SPUM_CTRL_FC_NSRC(9),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(9), sifmx_texts);
static const struct snd_kcontrol_new sifm9_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm9_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm10_enum, ABOX_SPUM_CTRL_FC_NSRC(10),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(10), sifmx_texts);
static const struct snd_kcontrol_new sifm10_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm10_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm11_enum, ABOX_SPUM_CTRL_FC_NSRC(11),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(11), sifmx_texts);
static const struct snd_kcontrol_new sifm11_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm11_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm12_enum, ABOX_SPUM_CTRL_FC_NSRC(12),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(12), sifmx_texts);
static const struct snd_kcontrol_new sifm12_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm12_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm13_enum, ABOX_SPUM_CTRL_FC_NSRC(13),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(13), sifmx_texts);
static const struct snd_kcontrol_new sifm13_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm13_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm14_enum, ABOX_SPUM_CTRL_FC_NSRC(14),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(14), sifmx_texts);
static const struct snd_kcontrol_new sifm14_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm14_enum),
};
static SOC_ENUM_SINGLE_DECL(sifm15_enum, ABOX_SPUM_CTRL_FC_NSRC(15),
		ABOX_FUNC_CHAIN_NSRC_OUT_L(15), sifmx_texts);
static const struct snd_kcontrol_new sifm15_controls[] = {
	SOC_DAPM_ENUM("DEMUX", sifm15_enum),
};
static int udma_sifs_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *k, int e)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int id = w->shift;

	return notify_event(data, ABOX_WIDGET_UDMA_RD0 + id, e);
}

static int udma_sifm_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *k, int e)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct device *dev = dapm->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int id = w->shift;

	return notify_event(data, ABOX_WIDGET_UDMA_WR0 + id, e);
}

static const char *const udma_sifm_texts[] = {
	"RESERVED", "SIFS0", "SIFS1", "SIFS2",
	"SIFS3", "SIFS4", "SIFS5", "SIFS6",
	"SIFS7",
};
static SOC_ENUM_SINGLE_DECL(udma_sifm0_enum, ABOX_ROUTE_UDMA_SIFM,
		ABOX_ROUTE_UDMA_SIFM_L(0), udma_sifm_texts);
static const struct snd_kcontrol_new udma_sifm0_controls[] = {
	SOC_DAPM_ENUM("MUX", udma_sifm0_enum),
};
static SOC_ENUM_SINGLE_DECL(udma_sifm1_enum, ABOX_ROUTE_UDMA_SIFM,
		ABOX_ROUTE_UDMA_SIFM_L(1), udma_sifm_texts);
static const struct snd_kcontrol_new udma_sifm1_controls[] = {
	SOC_DAPM_ENUM("MUX", udma_sifm1_enum),
};

static const char * const sifms_texts[] = {
	"SIFM0", "SIFM1", "SIFM2", "SIFM3", "SIFM4", "SIFM5",
	"SIFM6", "SIFM7", "SIFM8", "SIFM9", "SIFM10", "SIFM11",
	"SIFM12", "SIFM13", "SIFM14", "SIFM15",
};
static SOC_ENUM_SINGLE_DECL(sifms_enum, ABOX_SPUM_CTRL_SIFS_SEL,
		ABOX_SIFMS_OUT_SEL_L, sifms_texts);
static const struct snd_kcontrol_new sifms_controls[] = {
	SOC_DAPM_ENUM("MUX", sifms_enum),
};

enum spus_widget_id {
	SPUS_WIDGET_SIFMS_SPUS_IN,
	SPUS_WIDGET_SIFST_SPUS_IN,
	SPUS_WIDGET_SPUS_IN,
	SPUS_WIDGET_SPUS_PGA,
	SPUS_WIDGET_SPUS_ASRC,
	SPUS_WIDGET_SPUS_OUT,
	SPUS_WIDGET_SPUS_OUT_SIFS0,
	SPUS_WIDGET_SPUS_OUT_SIFS1,
	SPUS_WIDGET_SPUS_OUT_SIFS2,
	SPUS_WIDGET_SPUS_OUT_SIFS3,
	SPUS_WIDGET_SPUS_OUT_SIFS4,
	SPUS_WIDGET_SPUS_OUT_SIFS5,
	SPUS_WIDGET_SPUS_OUT_SIFS6,
	SPUS_WIDGET_SPUS_OUT_SIFS7,
	SPUS_WIDGET_COUNT,
};
static const struct snd_soc_dapm_widget spus_widgets[][SPUS_WIDGET_COUNT] = {
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN0", SND_SOC_NOPM, 0, 0, spus_in0_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC0", SND_SOC_NOPM, 0, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT0", SND_SOC_NOPM, 0, 0, spus_out0_controls),
		SND_SOC_DAPM_PGA("SPUS OUT0-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT0-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT0-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT0-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT0-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT0-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT0-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT0-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN1", SND_SOC_NOPM, 1, 0, spus_in1_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC1", SND_SOC_NOPM, 1, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT1", SND_SOC_NOPM, 0, 0, spus_out1_controls),
		SND_SOC_DAPM_PGA("SPUS OUT1-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT1-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT1-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT1-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT1-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT1-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT1-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT1-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN2", SND_SOC_NOPM, 2, 0, spus_in2_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC2", SND_SOC_NOPM, 2, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT2", SND_SOC_NOPM, 0, 0, spus_out2_controls),
		SND_SOC_DAPM_PGA("SPUS OUT2-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT2-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT2-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT2-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT2-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT2-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT2-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT2-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN3", SND_SOC_NOPM, 3, 0, spus_in3_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC3", SND_SOC_NOPM, 3, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT3", SND_SOC_NOPM, 0, 0, spus_out3_controls),
		SND_SOC_DAPM_PGA("SPUS OUT3-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT3-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT3-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT3-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT3-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT3-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT3-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT3-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN4", SND_SOC_NOPM, 4, 0, spus_in4_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC4", SND_SOC_NOPM, 4, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT4", SND_SOC_NOPM, 0, 0, spus_out4_controls),
		SND_SOC_DAPM_PGA("SPUS OUT4-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT4-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT4-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT4-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT4-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT4-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT4-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT4-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN5", SND_SOC_NOPM, 5, 0, spus_in5_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC5", SND_SOC_NOPM, 5, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT5", SND_SOC_NOPM, 0, 0, spus_out5_controls),
		SND_SOC_DAPM_PGA("SPUS OUT5-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT5-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT5-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT5-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT5-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT5-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT5-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT5-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN6", SND_SOC_NOPM, 6, 0, spus_in6_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC6", SND_SOC_NOPM, 6, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT6", SND_SOC_NOPM, 0, 0, spus_out6_controls),
		SND_SOC_DAPM_PGA("SPUS OUT6-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT6-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT6-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT6-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT6-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT6-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT6-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT6-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN7", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN7", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN7", SND_SOC_NOPM, 7, 0, spus_in7_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA7", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC7", SND_SOC_NOPM, 7, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT7", SND_SOC_NOPM, 0, 0, spus_out7_controls),
		SND_SOC_DAPM_PGA("SPUS OUT7-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT7-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT7-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT7-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT7-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT7-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT7-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT7-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN8", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN8", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN8", SND_SOC_NOPM, 8, 0, spus_in8_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA8", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC8", SND_SOC_NOPM, 8, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT8", SND_SOC_NOPM, 0, 0, spus_out8_controls),
		SND_SOC_DAPM_PGA("SPUS OUT8-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT8-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT8-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT8-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT8-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT8-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT8-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT8-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN9", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN9", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN9", SND_SOC_NOPM, 9, 0, spus_in9_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA9", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC9", SND_SOC_NOPM, 9, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT9", SND_SOC_NOPM, 0, 0, spus_out9_controls),
		SND_SOC_DAPM_PGA("SPUS OUT9-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT9-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT9-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT9-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT9-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT9-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT9-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT9-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN10", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN10", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN10", SND_SOC_NOPM, 10, 0, spus_in10_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA10", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC10", SND_SOC_NOPM, 10, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT10", SND_SOC_NOPM, 0, 0, spus_out10_controls),
		SND_SOC_DAPM_PGA("SPUS OUT10-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT10-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT10-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT10-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT10-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT10-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT10-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT10-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN11", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN11", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN11", SND_SOC_NOPM, 11, 0, spus_in11_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA11", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC11", SND_SOC_NOPM, 11, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT11", SND_SOC_NOPM, 0, 0, spus_out11_controls),
		SND_SOC_DAPM_PGA("SPUS OUT11-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT11-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT11-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT11-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT11-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT11-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT11-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT11-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN12", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN12", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN12", SND_SOC_NOPM, 12, 0, spus_in12_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA12", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC12", SND_SOC_NOPM, 12, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT12", SND_SOC_NOPM, 0, 0, spus_out12_controls),
		SND_SOC_DAPM_PGA("SPUS OUT12-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT12-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT12-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT12-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT12-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT12-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT12-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT12-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN13", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN13", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN13", SND_SOC_NOPM, 13, 0, spus_in13_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA13", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC13", SND_SOC_NOPM, 13, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT13", SND_SOC_NOPM, 0, 0, spus_out13_controls),
		SND_SOC_DAPM_PGA("SPUS OUT13-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT13-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT13-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT13-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT13-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT13-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT13-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT13-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN14", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN14", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN14", SND_SOC_NOPM, 14, 0, spus_in14_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA14", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC14", SND_SOC_NOPM, 14, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT14", SND_SOC_NOPM, 0, 0, spus_out14_controls),
		SND_SOC_DAPM_PGA("SPUS OUT14-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT14-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT14-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT14-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT14-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT14-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT14-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT14-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA("SIFSM-SPUS IN15", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SIFST-SPUS IN15", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_MUX_E("SPUS IN15", SND_SOC_NOPM, 15, 0, spus_in15_controls,
				spus_in_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUS PGA15", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA_E("SPUS ASRC15", SND_SOC_NOPM, 15, 0, NULL, 0,
				spus_asrc_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_DEMUX("SPUS OUT15", SND_SOC_NOPM, 0, 0, spus_out15_controls),
		SND_SOC_DAPM_PGA("SPUS OUT15-SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT15-SIFS1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT15-SIFS2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT15-SIFS3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT15-SIFS4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT15-SIFS5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT15-SIFS6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_PGA("SPUS OUT15-SIFS7", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
};

enum spum_widget_id {
	SPUM_WIDGET_SPUM_ASRC,
	SPUM_WIDGET_SPUM_PGA,
	SPUM_WIDGET_SIFM,
	SPUM_WIDGET_SIFM_SIFMS,
	SPUM_WIDGET_COUNT,
};

static const struct snd_soc_dapm_widget spum_widgets[][SPUM_WIDGET_COUNT] = {
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC0", SND_SOC_NOPM, 0, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA0", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM0", SND_SOC_NOPM, 0, 0, sifm0_controls),
		SND_SOC_DAPM_PGA("SIFM0-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC1", SND_SOC_NOPM, 1, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA1", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM1", SND_SOC_NOPM, 0, 0, sifm1_controls),
		SND_SOC_DAPM_PGA("SIFM1-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC2", SND_SOC_NOPM, 2, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA2", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM2", SND_SOC_NOPM, 0, 0, sifm2_controls),
		SND_SOC_DAPM_PGA("SIFM2-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),

	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC3", SND_SOC_NOPM, 3, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA3", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM3", SND_SOC_NOPM, 0, 0, sifm3_controls),
		SND_SOC_DAPM_PGA("SIFM3-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC4", SND_SOC_NOPM, 4, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA4", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM4", SND_SOC_NOPM, 0, 0, sifm4_controls),
		SND_SOC_DAPM_PGA("SIFM4-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC5", SND_SOC_NOPM, 5, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA5", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM5", SND_SOC_NOPM, 0, 0, sifm5_controls),
		SND_SOC_DAPM_PGA("SIFM5-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC6", SND_SOC_NOPM, 6, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA6", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM6", SND_SOC_NOPM, 0, 0, sifm6_controls),
		SND_SOC_DAPM_PGA("SIFM6-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC7", SND_SOC_NOPM, 7, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA7", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM7", SND_SOC_NOPM, 0, 0, sifm7_controls),
		SND_SOC_DAPM_PGA("SIFM7-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC8", SND_SOC_NOPM, 8, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA8", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM8", SND_SOC_NOPM, 0, 0, sifm8_controls),
		SND_SOC_DAPM_PGA("SIFM8-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC9", SND_SOC_NOPM, 9, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA9", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM9", SND_SOC_NOPM, 0, 0, sifm9_controls),
		SND_SOC_DAPM_PGA("SIFM9-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC10", SND_SOC_NOPM, 10, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA10", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM10", SND_SOC_NOPM, 0, 0, sifm10_controls),
		SND_SOC_DAPM_PGA("SIFM10-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC11", SND_SOC_NOPM, 11, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA11", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM11", SND_SOC_NOPM, 0, 0, sifm11_controls),
		SND_SOC_DAPM_PGA("SIFM11-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC12", SND_SOC_NOPM, 12, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA12", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM12", SND_SOC_NOPM, 0, 0, sifm12_controls),
		SND_SOC_DAPM_PGA("SIFM12-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC13", SND_SOC_NOPM, 13, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA13", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM13", SND_SOC_NOPM, 0, 0, sifm13_controls),
		SND_SOC_DAPM_PGA("SIFM13-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC14", SND_SOC_NOPM, 14, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA14", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM14", SND_SOC_NOPM, 0, 0, sifm14_controls),
		SND_SOC_DAPM_PGA("SIFM14-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
	{
		SND_SOC_DAPM_PGA_E("SPUM ASRC15", SND_SOC_NOPM, 15, 0, NULL, 0,
				spum_asrc_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
		SND_SOC_DAPM_PGA("SPUM PGA15", SND_SOC_NOPM, 0, 0, NULL, 0),
		SND_SOC_DAPM_DEMUX("SIFM15", SND_SOC_NOPM, 0, 0, sifm15_controls),
		SND_SOC_DAPM_PGA("SIFM15-SIFMS", SND_SOC_NOPM, 0, 0, NULL, 0),
	},
};

static const struct snd_soc_dapm_widget cmpnt_widgets[] = {
	SND_SOC_DAPM_MUX("SPUSM", SND_SOC_NOPM, 0, 0, spusm_controls),
	SND_SOC_DAPM_DEMUX("SIFSM", SND_SOC_NOPM, 0, 0, sifsm_controls),

	SND_SOC_DAPM_MUX("SPUST", SND_SOC_NOPM, 0, 0, spust_controls),
	SND_SOC_DAPM_PGA("SIDETONE", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_DEMUX("SIFST", SND_SOC_NOPM, 0, 0, sifst_controls),

	SND_SOC_DAPM_MIXER_E("SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0,
			sifs_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS1", SND_SOC_NOPM, 1, 0, sifs1_controls,
			sifs_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS2", SND_SOC_NOPM, 2, 0, sifs2_controls,
			sifs_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS3", SND_SOC_NOPM, 3, 0, sifs3_controls,
			sifs_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS4", SND_SOC_NOPM, 4, 0, sifs4_controls,
			sifs_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS5", SND_SOC_NOPM, 5, 0, sifs5_controls,
			sifs_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS6", SND_SOC_NOPM, 6, 0, sifs6_controls,
			sifs_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("SIFS7", SND_SOC_NOPM, 7, 0, sifs7_controls,
			sifs_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER("STMIX", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_PGA("SIFS0 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFS1 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFS2 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFS3 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFS4 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFS5 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFS6 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("SIFS7 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SWITCH("SIFS0 OUT", SND_SOC_NOPM, 0, 0,
			sifs0_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS1 OUT", SND_SOC_NOPM, 0, 0,
			sifs1_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS2 OUT", SND_SOC_NOPM, 0, 0,
			sifs2_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS3 OUT", SND_SOC_NOPM, 0, 0,
			sifs3_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS4 OUT", SND_SOC_NOPM, 0, 0,
			sifs4_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS5 OUT", SND_SOC_NOPM, 0, 0,
			sifs5_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS6 OUT", SND_SOC_NOPM, 0, 0,
			sifs6_out_controls),
	SND_SOC_DAPM_SWITCH("SIFS7 OUT", SND_SOC_NOPM, 0, 0,
			sifs7_out_controls),

	SND_SOC_DAPM_MUX("UAIF0 SPK", SND_SOC_NOPM, 0, 0, uaif0_spk_controls),
	SND_SOC_DAPM_MUX("UAIF1 SPK", SND_SOC_NOPM, 0, 0, uaif1_spk_controls),
	SND_SOC_DAPM_MUX("UAIF2 SPK", SND_SOC_NOPM, 0, 0, uaif2_spk_controls),
	SND_SOC_DAPM_MUX("UAIF3 SPK", SND_SOC_NOPM, 0, 0, uaif3_spk_controls),
	SND_SOC_DAPM_MUX("UAIF4 SPK", SND_SOC_NOPM, 0, 0, uaif4_spk_controls),
	SND_SOC_DAPM_MUX("UAIF5 SPK", SND_SOC_NOPM, 0, 0, uaif5_spk_controls),
	SND_SOC_DAPM_MUX("UAIF6 SPK", SND_SOC_NOPM, 0, 0, uaif6_spk_controls),
	SND_SOC_DAPM_MUX("DSIF SPK", SND_SOC_NOPM, 0, 0, dsif_spk_controls),

	SND_SOC_DAPM_SWITCH("UAIF0 PLA", SND_SOC_NOPM, 0, 0, uaif0_controls),
	SND_SOC_DAPM_SWITCH("UAIF1 PLA", SND_SOC_NOPM, 0, 0, uaif1_controls),
	SND_SOC_DAPM_SWITCH("UAIF2 PLA", SND_SOC_NOPM, 0, 0, uaif2_controls),
	SND_SOC_DAPM_SWITCH("UAIF3 PLA", SND_SOC_NOPM, 0, 0, uaif3_controls),
	SND_SOC_DAPM_SWITCH("UAIF4 PLA", SND_SOC_NOPM, 0, 0, uaif4_controls),
	SND_SOC_DAPM_SWITCH("UAIF5 PLA", SND_SOC_NOPM, 0, 0, uaif5_controls),
	SND_SOC_DAPM_SWITCH("UAIF6 PLA", SND_SOC_NOPM, 0, 0, uaif6_controls),
	SND_SOC_DAPM_SWITCH("DSIF PLA", SND_SOC_NOPM, 0, 0, dsif_controls),

	SND_SOC_DAPM_SWITCH("UAIF0 CAP", SND_SOC_NOPM, 0, 0, uaif0_controls),
	SND_SOC_DAPM_SWITCH("UAIF1 CAP", SND_SOC_NOPM, 0, 0, uaif1_controls),
	SND_SOC_DAPM_SWITCH("UAIF2 CAP", SND_SOC_NOPM, 0, 0, uaif2_controls),
	SND_SOC_DAPM_SWITCH("UAIF3 CAP", SND_SOC_NOPM, 0, 0, uaif3_controls),
	SND_SOC_DAPM_SWITCH("UAIF4 CAP", SND_SOC_NOPM, 0, 0, uaif4_controls),
	SND_SOC_DAPM_SWITCH("UAIF5 CAP", SND_SOC_NOPM, 0, 0, uaif5_controls),
	SND_SOC_DAPM_SWITCH("UAIF6 CAP", SND_SOC_NOPM, 0, 0, uaif6_controls),
	SND_SOC_DAPM_SWITCH("SPDY CAP", SND_SOC_NOPM, 0, 0, spdy_controls),

	SND_SOC_DAPM_MUX_E("NSRC0", SND_SOC_NOPM, 0, 0, nsrc0_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC1", SND_SOC_NOPM, 1, 0, nsrc1_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC2", SND_SOC_NOPM, 2, 0, nsrc2_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC3", SND_SOC_NOPM, 3, 0, nsrc3_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC4", SND_SOC_NOPM, 4, 0, nsrc4_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC5", SND_SOC_NOPM, 5, 0, nsrc5_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC6", SND_SOC_NOPM, 6, 0, nsrc6_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC7", SND_SOC_NOPM, 7, 0, nsrc7_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC8", SND_SOC_NOPM, 8, 0, nsrc8_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC9", SND_SOC_NOPM, 9, 0, nsrc9_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC10", SND_SOC_NOPM, 10, 0, nsrc10_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC11", SND_SOC_NOPM, 11, 0, nsrc11_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E8855)
	SND_SOC_DAPM_MUX_E("NSRC12", SND_SOC_NOPM, 12, 0, nsrc12_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC13", SND_SOC_NOPM, 13, 0, nsrc13_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC14", SND_SOC_NOPM, 14, 0, nsrc14_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX_E("NSRC15", SND_SOC_NOPM, 15, 0, nsrc15_controls,
			nsrc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
#endif

	SND_SOC_DAPM_SWITCH("NSRC0 In", SND_SOC_NOPM, 0, 0, nsrc0_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC1 In", SND_SOC_NOPM, 0, 0, nsrc1_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC2 In", SND_SOC_NOPM, 0, 0, nsrc2_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC3 In", SND_SOC_NOPM, 0, 0, nsrc3_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC4 In", SND_SOC_NOPM, 0, 0, nsrc4_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC5 In", SND_SOC_NOPM, 0, 0, nsrc5_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC6 In", SND_SOC_NOPM, 0, 0, nsrc6_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC7 In", SND_SOC_NOPM, 0, 0, nsrc7_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC8 In", SND_SOC_NOPM, 0, 0, nsrc8_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC9 In", SND_SOC_NOPM, 0, 0, nsrc9_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC10 In", SND_SOC_NOPM, 0, 0, nsrc10_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC11 In", SND_SOC_NOPM, 0, 0, nsrc11_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC12 In", SND_SOC_NOPM, 0, 0, nsrc12_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC13 In", SND_SOC_NOPM, 0, 0, nsrc13_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC14 In", SND_SOC_NOPM, 0, 0, nsrc14_in_controls),
	SND_SOC_DAPM_SWITCH("NSRC15 In", SND_SOC_NOPM, 0, 0, nsrc15_in_controls),

	SND_SOC_DAPM_PGA("NSRC0 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC1 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC2 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC3 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC4 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC5 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC6 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC7 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC8 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC9 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC10 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC11 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC12 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC13 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC14 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("NSRC15 PGA", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_MUX("SIFMS", SND_SOC_NOPM, 0, 0, sifms_controls),
};

static const struct snd_soc_dapm_route cmpnt_routes[] = {
	/* sink, control, source */
	{"SIFSM", NULL, "SPUSM"},

	{"SIFS0 PGA", NULL, "SIFS0"},
	{"SIFS1 PGA", NULL, "SIFS1"},
	{"SIFS2 PGA", NULL, "SIFS2"},
	{"SIFS3 PGA", NULL, "SIFS3"},
	{"SIFS4 PGA", NULL, "SIFS4"},
	{"SIFS5 PGA", NULL, "SIFS5"},
	{"SIFS6 PGA", NULL, "SIFS6"},
	{"SIFS7 PGA", NULL, "SIFS7"},

	{"STMIX", NULL, "SIFS0 PGA"},

	{"SIFS0 OUT", "Switch", "STMIX"},
	{"SIFS1 OUT", "Switch", "SIFS1 PGA"},
	{"SIFS2 OUT", "Switch", "SIFS2 PGA"},
	{"SIFS3 OUT", "Switch", "SIFS3 PGA"},
	{"SIFS4 OUT", "Switch", "SIFS4 PGA"},
	{"SIFS5 OUT", "Switch", "SIFS5 PGA"},
	{"SIFS6 OUT", "Switch", "SIFS6 PGA"},
	{"SIFS7 OUT", "Switch", "SIFS7 PGA"},

	{"UAIF0 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF0 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF0 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF0 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF0 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF0 SPK", "SIFS5", "SIFS5 OUT"},
	{"UAIF0 SPK", "SIFS6", "SIFS6 OUT"},
	{"UAIF0 SPK", "SIFS7", "SIFS7 OUT"},
	{"UAIF0 SPK", "SIFMS", "SIFMS"},
	{"UAIF1 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF1 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF1 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF1 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF1 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF1 SPK", "SIFS5", "SIFS5 OUT"},
	{"UAIF1 SPK", "SIFS6", "SIFS6 OUT"},
	{"UAIF1 SPK", "SIFS7", "SIFS7 OUT"},
	{"UAIF1 SPK", "SIFMS", "SIFMS"},
	{"UAIF2 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF2 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF2 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF2 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF2 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF2 SPK", "SIFS5", "SIFS5 OUT"},
	{"UAIF2 SPK", "SIFS6", "SIFS6 OUT"},
	{"UAIF2 SPK", "SIFS7", "SIFS7 OUT"},
	{"UAIF2 SPK", "SIFMS", "SIFMS"},
	{"UAIF3 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF3 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF3 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF3 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF3 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF3 SPK", "SIFS5", "SIFS5 OUT"},
	{"UAIF3 SPK", "SIFS6", "SIFS6 OUT"},
	{"UAIF3 SPK", "SIFS7", "SIFS7 OUT"},
	{"UAIF3 SPK", "SIFMS", "SIFMS"},
	{"UAIF4 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF4 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF4 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF4 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF4 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF4 SPK", "SIFS5", "SIFS5 OUT"},
	{"UAIF4 SPK", "SIFS6", "SIFS6 OUT"},
	{"UAIF4 SPK", "SIFS7", "SIFS7 OUT"},
	{"UAIF4 SPK", "SIFMS", "SIFMS"},
	{"UAIF5 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF5 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF5 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF5 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF5 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF5 SPK", "SIFS5", "SIFS5 OUT"},
	{"UAIF5 SPK", "SIFS6", "SIFS6 OUT"},
	{"UAIF5 SPK", "SIFS7", "SIFS7 OUT"},
	{"UAIF5 SPK", "SIFMS", "SIFMS"},
	{"UAIF6 SPK", "SIFS0", "SIFS0 OUT"},
	{"UAIF6 SPK", "SIFS1", "SIFS1 OUT"},
	{"UAIF6 SPK", "SIFS2", "SIFS2 OUT"},
	{"UAIF6 SPK", "SIFS3", "SIFS3 OUT"},
	{"UAIF6 SPK", "SIFS4", "SIFS4 OUT"},
	{"UAIF6 SPK", "SIFS5", "SIFS5 OUT"},
	{"UAIF6 SPK", "SIFS6", "SIFS6 OUT"},
	{"UAIF6 SPK", "SIFS7", "SIFS7 OUT"},
	{"UAIF6 SPK", "SIFMS", "SIFMS"},
	{"DSIF SPK", "SIFS1", "SIFS1 OUT"},
	{"DSIF SPK", "SIFS2", "SIFS2 OUT"},
	{"DSIF SPK", "SIFS3", "SIFS3 OUT"},
	{"DSIF SPK", "SIFS4", "SIFS4 OUT"},
	{"DSIF SPK", "SIFS5", "SIFS5 OUT"},
	{"DSIF SPK", "SIFS6", "SIFS6 OUT"},
	{"DSIF SPK", "SIFS7", "SIFS7 OUT"},

	{"UAIF0 PLA", "UAIF0 Switch", "UAIF0 SPK"},
	{"UAIF1 PLA", "UAIF1 Switch", "UAIF1 SPK"},
	{"UAIF2 PLA", "UAIF2 Switch", "UAIF2 SPK"},
	{"UAIF3 PLA", "UAIF3 Switch", "UAIF3 SPK"},
	{"UAIF4 PLA", "UAIF4 Switch", "UAIF4 SPK"},
	{"UAIF5 PLA", "UAIF5 Switch", "UAIF5 SPK"},
	{"UAIF6 PLA", "UAIF6 Switch", "UAIF6 SPK"},
	{"DSIF PLA", "DSIF Switch", "DSIF SPK"},

	{"NSRC0", "SIFS0", "SIFS0 OUT"},
	{"NSRC0", "SIFS1", "SIFS1 OUT"},
	{"NSRC0", "SIFS2", "SIFS2 OUT"},
	{"NSRC0", "SIFS3", "SIFS3 OUT"},
	{"NSRC0", "SIFS4", "SIFS4 OUT"},
	{"NSRC0", "SIFS5", "SIFS5 OUT"},
	{"NSRC0", "SIFS6", "SIFS6 OUT"},
	{"NSRC0", "SIFS7", "SIFS7 OUT"},
	{"NSRC0", "UAIF0", "UAIF0 CAP"},
	{"NSRC0", "UAIF1", "UAIF1 CAP"},
	{"NSRC0", "UAIF2", "UAIF2 CAP"},
	{"NSRC0", "UAIF3", "UAIF3 CAP"},
	{"NSRC0", "UAIF4", "UAIF4 CAP"},
	{"NSRC0", "UAIF5", "UAIF5 CAP"},
	{"NSRC0", "UAIF6", "UAIF6 CAP"},
	{"NSRC0", "SPDY", "SPDY CAP"},

	{"NSRC1", "SIFS0", "SIFS0 OUT"},
	{"NSRC1", "SIFS1", "SIFS1 OUT"},
	{"NSRC1", "SIFS2", "SIFS2 OUT"},
	{"NSRC1", "SIFS3", "SIFS3 OUT"},
	{"NSRC1", "SIFS4", "SIFS4 OUT"},
	{"NSRC1", "SIFS5", "SIFS5 OUT"},
	{"NSRC1", "SIFS6", "SIFS6 OUT"},
	{"NSRC1", "SIFS7", "SIFS7 OUT"},
	{"NSRC1", "UAIF0", "UAIF0 CAP"},
	{"NSRC1", "UAIF1", "UAIF1 CAP"},
	{"NSRC1", "UAIF2", "UAIF2 CAP"},
	{"NSRC1", "UAIF3", "UAIF3 CAP"},
	{"NSRC1", "UAIF4", "UAIF4 CAP"},
	{"NSRC1", "UAIF5", "UAIF5 CAP"},
	{"NSRC1", "UAIF6", "UAIF6 CAP"},
	{"NSRC1", "SPDY", "SPDY CAP"},

	{"NSRC2", "SIFS0", "SIFS0 OUT"},
	{"NSRC2", "SIFS1", "SIFS1 OUT"},
	{"NSRC2", "SIFS2", "SIFS2 OUT"},
	{"NSRC2", "SIFS3", "SIFS3 OUT"},
	{"NSRC2", "SIFS4", "SIFS4 OUT"},
	{"NSRC2", "SIFS5", "SIFS5 OUT"},
	{"NSRC2", "SIFS6", "SIFS6 OUT"},
	{"NSRC2", "SIFS7", "SIFS7 OUT"},
	{"NSRC2", "UAIF0", "UAIF0 CAP"},
	{"NSRC2", "UAIF1", "UAIF1 CAP"},
	{"NSRC2", "UAIF2", "UAIF2 CAP"},
	{"NSRC2", "UAIF3", "UAIF3 CAP"},
	{"NSRC2", "UAIF4", "UAIF4 CAP"},
	{"NSRC2", "UAIF5", "UAIF5 CAP"},
	{"NSRC2", "UAIF6", "UAIF6 CAP"},
	{"NSRC2", "SPDY", "SPDY CAP"},

	{"NSRC3", "SIFS0", "SIFS0 OUT"},
	{"NSRC3", "SIFS1", "SIFS1 OUT"},
	{"NSRC3", "SIFS2", "SIFS2 OUT"},
	{"NSRC3", "SIFS3", "SIFS3 OUT"},
	{"NSRC3", "SIFS4", "SIFS4 OUT"},
	{"NSRC3", "SIFS5", "SIFS5 OUT"},
	{"NSRC3", "SIFS6", "SIFS6 OUT"},
	{"NSRC3", "SIFS7", "SIFS7 OUT"},
	{"NSRC3", "UAIF0", "UAIF0 CAP"},
	{"NSRC3", "UAIF1", "UAIF1 CAP"},
	{"NSRC3", "UAIF2", "UAIF2 CAP"},
	{"NSRC3", "UAIF3", "UAIF3 CAP"},
	{"NSRC3", "UAIF4", "UAIF4 CAP"},
	{"NSRC3", "UAIF5", "UAIF5 CAP"},
	{"NSRC3", "UAIF6", "UAIF6 CAP"},
	{"NSRC3", "SPDY", "SPDY CAP"},

	{"NSRC4", "SIFS0", "SIFS0 OUT"},
	{"NSRC4", "SIFS1", "SIFS1 OUT"},
	{"NSRC4", "SIFS2", "SIFS2 OUT"},
	{"NSRC4", "SIFS3", "SIFS3 OUT"},
	{"NSRC4", "SIFS4", "SIFS4 OUT"},
	{"NSRC4", "SIFS5", "SIFS5 OUT"},
	{"NSRC4", "SIFS6", "SIFS6 OUT"},
	{"NSRC4", "SIFS7", "SIFS7 OUT"},
	{"NSRC4", "UAIF0", "UAIF0 CAP"},
	{"NSRC4", "UAIF1", "UAIF1 CAP"},
	{"NSRC4", "UAIF2", "UAIF2 CAP"},
	{"NSRC4", "UAIF3", "UAIF3 CAP"},
	{"NSRC4", "UAIF4", "UAIF4 CAP"},
	{"NSRC4", "UAIF5", "UAIF5 CAP"},
	{"NSRC4", "UAIF6", "UAIF6 CAP"},
	{"NSRC4", "SPDY", "SPDY CAP"},

	{"NSRC5", "SIFS0", "SIFS0 OUT"},
	{"NSRC5", "SIFS1", "SIFS1 OUT"},
	{"NSRC5", "SIFS2", "SIFS2 OUT"},
	{"NSRC5", "SIFS3", "SIFS3 OUT"},
	{"NSRC5", "SIFS4", "SIFS4 OUT"},
	{"NSRC5", "SIFS5", "SIFS5 OUT"},
	{"NSRC5", "SIFS6", "SIFS6 OUT"},
	{"NSRC5", "SIFS7", "SIFS7 OUT"},
	{"NSRC5", "UAIF0", "UAIF0 CAP"},
	{"NSRC5", "UAIF1", "UAIF1 CAP"},
	{"NSRC5", "UAIF2", "UAIF2 CAP"},
	{"NSRC5", "UAIF3", "UAIF3 CAP"},
	{"NSRC5", "UAIF4", "UAIF4 CAP"},
	{"NSRC5", "UAIF5", "UAIF5 CAP"},
	{"NSRC5", "UAIF6", "UAIF6 CAP"},
	{"NSRC5", "SPDY", "SPDY CAP"},

	{"NSRC6", "SIFS0", "SIFS0 OUT"},
	{"NSRC6", "SIFS1", "SIFS1 OUT"},
	{"NSRC6", "SIFS2", "SIFS2 OUT"},
	{"NSRC6", "SIFS3", "SIFS3 OUT"},
	{"NSRC6", "SIFS4", "SIFS4 OUT"},
	{"NSRC6", "SIFS5", "SIFS5 OUT"},
	{"NSRC6", "SIFS6", "SIFS6 OUT"},
	{"NSRC6", "SIFS7", "SIFS7 OUT"},
	{"NSRC6", "UAIF0", "UAIF0 CAP"},
	{"NSRC6", "UAIF1", "UAIF1 CAP"},
	{"NSRC6", "UAIF2", "UAIF2 CAP"},
	{"NSRC6", "UAIF3", "UAIF3 CAP"},
	{"NSRC6", "UAIF4", "UAIF4 CAP"},
	{"NSRC6", "UAIF5", "UAIF5 CAP"},
	{"NSRC6", "UAIF6", "UAIF6 CAP"},
	{"NSRC6", "SPDY", "SPDY CAP"},

	{"NSRC7", "SIFS0", "SIFS0 OUT"},
	{"NSRC7", "SIFS1", "SIFS1 OUT"},
	{"NSRC7", "SIFS2", "SIFS2 OUT"},
	{"NSRC7", "SIFS3", "SIFS3 OUT"},
	{"NSRC7", "SIFS4", "SIFS4 OUT"},
	{"NSRC7", "SIFS5", "SIFS5 OUT"},
	{"NSRC7", "SIFS6", "SIFS6 OUT"},
	{"NSRC7", "SIFS7", "SIFS7 OUT"},
	{"NSRC7", "UAIF0", "UAIF0 CAP"},
	{"NSRC7", "UAIF1", "UAIF1 CAP"},
	{"NSRC7", "UAIF2", "UAIF2 CAP"},
	{"NSRC7", "UAIF3", "UAIF3 CAP"},
	{"NSRC7", "UAIF4", "UAIF4 CAP"},
	{"NSRC7", "UAIF5", "UAIF5 CAP"},
	{"NSRC7", "UAIF6", "UAIF6 CAP"},
	{"NSRC7", "SPDY", "SPDY CAP"},

	{"NSRC8", "SIFS0", "SIFS0 OUT"},
	{"NSRC8", "SIFS1", "SIFS1 OUT"},
	{"NSRC8", "SIFS2", "SIFS2 OUT"},
	{"NSRC8", "SIFS3", "SIFS3 OUT"},
	{"NSRC8", "SIFS4", "SIFS4 OUT"},
	{"NSRC8", "SIFS5", "SIFS5 OUT"},
	{"NSRC8", "SIFS6", "SIFS6 OUT"},
	{"NSRC8", "SIFS7", "SIFS7 OUT"},
	{"NSRC8", "UAIF0", "UAIF0 CAP"},
	{"NSRC8", "UAIF1", "UAIF1 CAP"},
	{"NSRC8", "UAIF2", "UAIF2 CAP"},
	{"NSRC8", "UAIF3", "UAIF3 CAP"},
	{"NSRC8", "UAIF4", "UAIF4 CAP"},
	{"NSRC8", "UAIF5", "UAIF5 CAP"},
	{"NSRC8", "UAIF6", "UAIF6 CAP"},
	{"NSRC8", "SPDY", "SPDY CAP"},

	{"NSRC9", "SIFS0", "SIFS0 OUT"},
	{"NSRC9", "SIFS1", "SIFS1 OUT"},
	{"NSRC9", "SIFS2", "SIFS2 OUT"},
	{"NSRC9", "SIFS3", "SIFS3 OUT"},
	{"NSRC9", "SIFS4", "SIFS4 OUT"},
	{"NSRC9", "SIFS5", "SIFS5 OUT"},
	{"NSRC9", "SIFS6", "SIFS6 OUT"},
	{"NSRC9", "SIFS7", "SIFS7 OUT"},
	{"NSRC9", "UAIF0", "UAIF0 CAP"},
	{"NSRC9", "UAIF1", "UAIF1 CAP"},
	{"NSRC9", "UAIF2", "UAIF2 CAP"},
	{"NSRC9", "UAIF3", "UAIF3 CAP"},
	{"NSRC9", "UAIF4", "UAIF4 CAP"},
	{"NSRC9", "UAIF5", "UAIF5 CAP"},
	{"NSRC9", "UAIF6", "UAIF6 CAP"},
	{"NSRC9", "SPDY", "SPDY CAP"},

	{"NSRC10", "SIFS0", "SIFS0 OUT"},
	{"NSRC10", "SIFS1", "SIFS1 OUT"},
	{"NSRC10", "SIFS2", "SIFS2 OUT"},
	{"NSRC10", "SIFS3", "SIFS3 OUT"},
	{"NSRC10", "SIFS4", "SIFS4 OUT"},
	{"NSRC10", "SIFS5", "SIFS5 OUT"},
	{"NSRC10", "SIFS6", "SIFS6 OUT"},
	{"NSRC10", "SIFS7", "SIFS7 OUT"},
	{"NSRC10", "UAIF0", "UAIF0 CAP"},
	{"NSRC10", "UAIF1", "UAIF1 CAP"},
	{"NSRC10", "UAIF2", "UAIF2 CAP"},
	{"NSRC10", "UAIF3", "UAIF3 CAP"},
	{"NSRC10", "UAIF4", "UAIF4 CAP"},
	{"NSRC10", "UAIF5", "UAIF5 CAP"},
	{"NSRC10", "UAIF6", "UAIF6 CAP"},
	{"NSRC10", "SPDY", "SPDY CAP"},

	{"NSRC11", "SIFS0", "SIFS0 OUT"},
	{"NSRC11", "SIFS1", "SIFS1 OUT"},
	{"NSRC11", "SIFS2", "SIFS2 OUT"},
	{"NSRC11", "SIFS3", "SIFS3 OUT"},
	{"NSRC11", "SIFS4", "SIFS4 OUT"},
	{"NSRC11", "SIFS5", "SIFS5 OUT"},
	{"NSRC11", "SIFS6", "SIFS6 OUT"},
	{"NSRC11", "SIFS7", "SIFS7 OUT"},
	{"NSRC11", "UAIF0", "UAIF0 CAP"},
	{"NSRC11", "UAIF1", "UAIF1 CAP"},
	{"NSRC11", "UAIF2", "UAIF2 CAP"},
	{"NSRC11", "UAIF3", "UAIF3 CAP"},
	{"NSRC11", "UAIF4", "UAIF4 CAP"},
	{"NSRC11", "UAIF5", "UAIF5 CAP"},
	{"NSRC11", "UAIF6", "UAIF6 CAP"},
	{"NSRC11", "SPDY", "SPDY CAP"},

#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E8855)
	{"NSRC12", "SIFS0", "SIFS0 OUT"},
	{"NSRC12", "SIFS1", "SIFS1 OUT"},
	{"NSRC12", "SIFS2", "SIFS2 OUT"},
	{"NSRC12", "SIFS3", "SIFS3 OUT"},
	{"NSRC12", "SIFS4", "SIFS4 OUT"},
	{"NSRC12", "SIFS5", "SIFS5 OUT"},
	{"NSRC12", "SIFS6", "SIFS6 OUT"},
	{"NSRC12", "SIFS7", "SIFS7 OUT"},
	{"NSRC12", "UAIF0", "UAIF0 CAP"},
	{"NSRC12", "UAIF1", "UAIF1 CAP"},
	{"NSRC12", "UAIF2", "UAIF2 CAP"},
	{"NSRC12", "UAIF3", "UAIF3 CAP"},
	{"NSRC12", "UAIF4", "UAIF4 CAP"},
	{"NSRC12", "UAIF5", "UAIF5 CAP"},
	{"NSRC12", "UAIF6", "UAIF6 CAP"},
	{"NSRC12", "SPDY", "SPDY CAP"},

	{"NSRC13", "SIFS0", "SIFS0 OUT"},
	{"NSRC13", "SIFS1", "SIFS1 OUT"},
	{"NSRC13", "SIFS2", "SIFS2 OUT"},
	{"NSRC13", "SIFS3", "SIFS3 OUT"},
	{"NSRC13", "SIFS4", "SIFS4 OUT"},
	{"NSRC13", "SIFS5", "SIFS5 OUT"},
	{"NSRC13", "SIFS6", "SIFS6 OUT"},
	{"NSRC13", "SIFS7", "SIFS7 OUT"},
	{"NSRC13", "UAIF0", "UAIF0 CAP"},
	{"NSRC13", "UAIF1", "UAIF1 CAP"},
	{"NSRC13", "UAIF2", "UAIF2 CAP"},
	{"NSRC13", "UAIF3", "UAIF3 CAP"},
	{"NSRC13", "UAIF4", "UAIF4 CAP"},
	{"NSRC13", "UAIF5", "UAIF5 CAP"},
	{"NSRC13", "UAIF6", "UAIF6 CAP"},
	{"NSRC13", "SPDY", "SPDY CAP"},

	{"NSRC14", "SIFS0", "SIFS0 OUT"},
	{"NSRC14", "SIFS1", "SIFS1 OUT"},
	{"NSRC14", "SIFS2", "SIFS2 OUT"},
	{"NSRC14", "SIFS3", "SIFS3 OUT"},
	{"NSRC14", "SIFS4", "SIFS4 OUT"},
	{"NSRC14", "SIFS5", "SIFS5 OUT"},
	{"NSRC14", "SIFS6", "SIFS6 OUT"},
	{"NSRC14", "SIFS7", "SIFS7 OUT"},
	{"NSRC14", "UAIF0", "UAIF0 CAP"},
	{"NSRC14", "UAIF1", "UAIF1 CAP"},
	{"NSRC14", "UAIF2", "UAIF2 CAP"},
	{"NSRC14", "UAIF3", "UAIF3 CAP"},
	{"NSRC14", "UAIF4", "UAIF4 CAP"},
	{"NSRC14", "UAIF5", "UAIF5 CAP"},
	{"NSRC14", "UAIF6", "UAIF6 CAP"},
	{"NSRC14", "SPDY", "SPDY CAP"},

	{"NSRC15", "SIFS0", "SIFS0 OUT"},
	{"NSRC15", "SIFS1", "SIFS1 OUT"},
	{"NSRC15", "SIFS2", "SIFS2 OUT"},
	{"NSRC15", "SIFS3", "SIFS3 OUT"},
	{"NSRC15", "SIFS4", "SIFS4 OUT"},
	{"NSRC15", "SIFS5", "SIFS5 OUT"},
	{"NSRC15", "SIFS6", "SIFS6 OUT"},
	{"NSRC15", "SIFS7", "SIFS7 OUT"},
	{"NSRC15", "UAIF0", "UAIF0 CAP"},
	{"NSRC15", "UAIF1", "UAIF1 CAP"},
	{"NSRC15", "UAIF2", "UAIF2 CAP"},
	{"NSRC15", "UAIF3", "UAIF3 CAP"},
	{"NSRC15", "UAIF4", "UAIF4 CAP"},
	{"NSRC15", "UAIF5", "UAIF5 CAP"},
	{"NSRC15", "UAIF6", "UAIF6 CAP"},
	{"NSRC15", "SPDY", "SPDY CAP"},
#endif

	{"NSRC0 In", "Switch", "NSRC0"},
	{"NSRC1 In", "Switch", "NSRC1"},
	{"NSRC2 In", "Switch", "NSRC2"},
	{"NSRC3 In", "Switch", "NSRC3"},
	{"NSRC4 In", "Switch", "NSRC4"},
	{"NSRC5 In", "Switch", "NSRC5"},
	{"NSRC6 In", "Switch", "NSRC6"},
	{"NSRC7 In", "Switch", "NSRC7"},
	{"NSRC8 In", "Switch", "NSRC8"},
	{"NSRC9 In", "Switch", "NSRC9"},
	{"NSRC10 In", "Switch", "NSRC10"},
	{"NSRC11 In", "Switch", "NSRC11"},
#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E8855)
	{"NSRC12 In", "Switch", "NSRC12"},
	{"NSRC13 In", "Switch", "NSRC13"},
	{"NSRC14 In", "Switch", "NSRC14"},
	{"NSRC15 In", "Switch", "NSRC15"},
#endif

	{"NSRC0 PGA", NULL, "NSRC0 In"},
	{"NSRC1 PGA", NULL, "NSRC1 In"},
	{"NSRC2 PGA", NULL, "NSRC2 In"},
	{"NSRC3 PGA", NULL, "NSRC3 In"},
	{"NSRC4 PGA", NULL, "NSRC4 In"},
	{"NSRC5 PGA", NULL, "NSRC5 In"},
	{"NSRC6 PGA", NULL, "NSRC6 In"},
	{"NSRC7 PGA", NULL, "NSRC7 In"},
	{"NSRC8 PGA", NULL, "NSRC8 In"},
	{"NSRC9 PGA", NULL, "NSRC9 In"},
	{"NSRC10 PGA", NULL, "NSRC10 In"},
	{"NSRC11 PGA", NULL, "NSRC11 In"},
#if IS_ENABLED(CONFIG_SOC_S5E9955) || IS_ENABLED(CONFIG_SOC_S5E8855)
	{"NSRC12 PGA", NULL, "NSRC12 In"},
	{"NSRC13 PGA", NULL, "NSRC13 In"},
	{"NSRC14 PGA", NULL, "NSRC14 In"},
	{"NSRC15 PGA", NULL, "NSRC15 In"},
#endif
};

static const char *kasprintf_template(struct device *dev, const char *template, int id)
{
	if (strstr(template, "%d"))
		return devm_kasprintf(dev, GFP_KERNEL, template, id);
	else
		return devm_kstrdup(dev, template, GFP_KERNEL);
}

static int register_routes_template(struct snd_soc_component *cmpnt, int id,
		const struct snd_soc_dapm_route *routes_template, size_t route_len)
{
	struct device *dev = cmpnt->dev;
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(cmpnt);
	struct snd_soc_dapm_route *routes, *route;
	int ret = 0;

	routes = devm_kmemdup(dev, routes_template, sizeof(routes_template[0]) * route_len, GFP_KERNEL);
	if (!routes)
		return -ENOMEM;

	for (route = routes; route - routes < route_len; route++) {
		route->sink = kasprintf_template(dev, route->sink, id);
		if (!route->sink) {
			ret = -ENOMEM;
			break;
		}

		if (route->control) {
			route->control = kasprintf_template(dev, route->control, id);
			if (!route->control) {
				ret = -ENOMEM;
				break;
			}
		}

		route->source = kasprintf_template(dev, route->source, id);
		if (!route->source) {
			ret = -ENOMEM;
			break;
		}

		abox_dbg(dev, "route added: %s, %s, %s\n", route->sink, route->control ? : "(null)", route->source);
	}

	if (ret >= 0)
		ret = snd_soc_dapm_add_routes(dapm, routes, route_len);

	for (route = routes; route - routes < route_len; route++) {
		if (route->sink)
			devm_kfree(dev, route->sink);
		if (route->control)
			devm_kfree(dev, route->control);
		if (route->source)
			devm_kfree(dev, route->source);
	}
	devm_kfree(dev, routes);

	return ret;
}

static int register_spus_controls(struct snd_soc_component *cmpnt, int id)
{
	int i, ret = 0;

	if (id >= ARRAY_SIZE(spus_controls))
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(spus_controls[id]); i++)
		ret |= snd_soc_add_component_controls(cmpnt, spus_controls[id][i], 1);

	return ret;
}

static int register_spus_widgets(struct snd_soc_component *cmpnt, int id)
{
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(cmpnt);

	if (id >= ARRAY_SIZE(spus_widgets))
		return -EINVAL;

	return snd_soc_dapm_new_controls(dapm, spus_widgets[id], ARRAY_SIZE(spus_widgets[id]));
}

static const struct snd_soc_dapm_route spus_routes_template[] = {
	{"SIFSM-SPUS IN%d", "SPUS IN%d", "SIFSM"},
	{"SPUS IN%d", "SIFSM", "SIFSM-SPUS IN%d"},
	{"SIFST-SPUS IN%d", "SPUS IN%d", "SIFST"},
	{"SPUS IN%d", "SIFST", "SIFST-SPUS IN%d"},
	{"SPUS PGA%d", NULL, "SPUS IN%d"},
	{"SPUS ASRC%d", NULL, "SPUS PGA%d"},
	{"SPUS OUT%d", NULL, "SPUS ASRC%d"},
	{"SPUS OUT%d-SIFS0", "SIFS0", "SPUS OUT%d"},
	{"SPUS OUT%d-SIFS1", "SIFS1", "SPUS OUT%d"},
	{"SPUS OUT%d-SIFS2", "SIFS2", "SPUS OUT%d"},
	{"SPUS OUT%d-SIFS3", "SIFS3", "SPUS OUT%d"},
	{"SPUS OUT%d-SIFS4", "SIFS4", "SPUS OUT%d"},
	{"SPUS OUT%d-SIFS5", "SIFS5", "SPUS OUT%d"},
	{"SPUS OUT%d-SIFS6", "SIFS6", "SPUS OUT%d"},
	{"SPUS OUT%d-SIFS7", "SIFS7", "SPUS OUT%d"},
	{"SIFS0", NULL, "SPUS OUT%d-SIFS0"},
	{"SIFS1", "SPUS OUT%d", "SPUS OUT%d-SIFS1"},
	{"SIFS2", "SPUS OUT%d", "SPUS OUT%d-SIFS2"},
	{"SIFS3", "SPUS OUT%d", "SPUS OUT%d-SIFS3"},
	{"SIFS4", "SPUS OUT%d", "SPUS OUT%d-SIFS4"},
	{"SIFS5", "SPUS OUT%d", "SPUS OUT%d-SIFS5"},
	{"SIFS6", "SPUS OUT%d", "SPUS OUT%d-SIFS6"},
	{"SIFS7", "SPUS OUT%d", "SPUS OUT%d-SIFS7"},
};

static int register_spus(struct snd_soc_component *cmpnt, int id)
{
	int ret;

	ret = register_spus_widgets(cmpnt, id);
	if (ret < 0)
		return ret;

	ret = register_routes_template(cmpnt, id, spus_routes_template,
			ARRAY_SIZE(spus_routes_template));
	if (ret < 0)
		return ret;

	return register_spus_controls(cmpnt, id);
}

static int register_spum_controls(struct snd_soc_component *cmpnt, int id)
{
	int i, ret = 0;

	if (id >= ARRAY_SIZE(spum_controls))
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(spum_controls[id]); i++)
		ret |= snd_soc_add_component_controls(cmpnt, spum_controls[id][i], 1);

	return ret;
}

static int register_spum_widgets(struct snd_soc_component *cmpnt, int id)
{
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(cmpnt);

	if (id >= ARRAY_SIZE(spum_widgets))
		return -EINVAL;

	return snd_soc_dapm_new_controls(dapm, spum_widgets[id], ARRAY_SIZE(spum_widgets[id]));
}

static const struct snd_soc_dapm_route spum_routes_template[] = {
	{"SPUM ASRC%d", NULL, "NSRC%d PGA"},
	{"SPUM PGA%d", NULL, "SPUM ASRC%d"},
	{"SIFM%d", NULL, "SPUM PGA%d"},
	{"SIFM%d-SIFMS", "SIFMS", "SIFM%d"},
	{"SIFMS", "SIFM%d", "SIFM%d-SIFMS"},
};

static int register_spum(struct snd_soc_component *cmpnt, int id)
{
	int ret;

	ret = register_spum_widgets(cmpnt, id);
	if (ret < 0)
		return ret;

	ret = register_routes_template(cmpnt, id, spum_routes_template,
			ARRAY_SIZE(spum_routes_template));
	if (ret < 0)
		return ret;

	return register_spum_controls(cmpnt, id);
}

static int cmpnt_probe(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(cmpnt);
	int i;

	abox_dbg(dev, "%s\n", __func__);

	snd_soc_component_init_regmap(cmpnt, data->regmap);

	for (i = 0; i < COUNT_SPUS; i++)
		register_spus(cmpnt, i);

	for (i = 0; i < COUNT_SPUM; i++)
		register_spum(cmpnt, i);

	snd_soc_add_component_controls(cmpnt, sidetone_controls,
			ARRAY_SIZE(sidetone_controls));

	snd_soc_dapm_add_routes(dapm, sidetone_routes,
			ARRAY_SIZE(sidetone_routes));
	snd_soc_dapm_weak_routes(dapm, sidetone_routes,
			ARRAY_SIZE(sidetone_routes));

	data->cmpnt = cmpnt;

	abox_atune_probe(data);

	/* vdma and dump are initialized in abox component probe
	 * to set vdma to sound card 1 and dump to sound card 2.
	 */
	abox_dump_init(dev);
	abox_udma_init(data);

	data->ws = wakeup_source_register(NULL, "abox");

	return 0;
}

static void cmpnt_remove(struct snd_soc_component *cmpnt)
{
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);

	abox_dbg(dev, "%s\n", __func__);

	wakeup_source_unregister(data->ws);
}

static const struct snd_soc_component_driver abox_cmpnt_drv = {
	.probe			= cmpnt_probe,
	.remove			= cmpnt_remove,
	.controls		= cmpnt_controls,
	.num_controls		= ARRAY_SIZE(cmpnt_controls),
	.dapm_widgets		= cmpnt_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(cmpnt_widgets),
	.dapm_routes		= cmpnt_routes,
	.num_dapm_routes	= ARRAY_SIZE(cmpnt_routes),
	.probe_order		= SND_SOC_COMP_ORDER_FIRST,
};

int abox_cmpnt_adjust_sbank(struct abox_data *data,
		enum abox_dai id, struct snd_pcm_hw_params *params,
		unsigned int sbank_size)
{
	struct snd_soc_component *cmpnt = data->cmpnt;
	unsigned int time, size, reg, val;
	unsigned int mask = ABOX_SBANK_SIZE_MASK;
	int ret;

	switch (id) {
	case ABOX_RDMA0:
	case ABOX_RDMA1:
	case ABOX_RDMA2:
	case ABOX_RDMA3:
	case ABOX_RDMA4:
	case ABOX_RDMA5:
	case ABOX_RDMA6:
	case ABOX_RDMA7:
	case ABOX_RDMA8:
	case ABOX_RDMA9:
	case ABOX_RDMA10:
	case ABOX_RDMA11:
	case ABOX_RDMA12:
	case ABOX_RDMA13:
	case ABOX_RDMA14:
	case ABOX_RDMA15:
		reg = ABOX_SPUS_SBANK_RDMA(id - ABOX_RDMA0);
		break;
	case ABOX_RDMA0_BE:
	case ABOX_RDMA1_BE:
	case ABOX_RDMA2_BE:
	case ABOX_RDMA3_BE:
	case ABOX_RDMA4_BE:
	case ABOX_RDMA5_BE:
	case ABOX_RDMA6_BE:
	case ABOX_RDMA7_BE:
	case ABOX_RDMA8_BE:
	case ABOX_RDMA9_BE:
	case ABOX_RDMA10_BE:
	case ABOX_RDMA11_BE:
	case ABOX_RDMA12_BE:
	case ABOX_RDMA13_BE:
	case ABOX_RDMA14_BE:
	case ABOX_RDMA15_BE:
		reg = ABOX_SPUS_SBANK_RDMA(id - ABOX_RDMA0_BE);
		break;
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
	case ABOX_NSRC7:
	case ABOX_NSRC8:
	case ABOX_NSRC9:
	case ABOX_NSRC10:
	case ABOX_NSRC11:
	case ABOX_NSRC12:
	case ABOX_NSRC13:
	case ABOX_NSRC14:
	case ABOX_NSRC15:
		reg = ABOX_SPUM_SBANK_NSRC(id - ABOX_NSRC0);
		break;
	default:
		return -EINVAL;
	}

	time = hw_param_interval_c(params, SNDRV_PCM_HW_PARAM_PERIOD_TIME)->min;
	time /= 1000;

	if (sbank_size >= SZ_16 && sbank_size <= SZ_256)
		size = sbank_size;
	else if (time <= 10)
		size = SZ_128;
	else
		size = SZ_256;

	/* Sbank size unit is 16byte(= 4 shifts) */
	val = size << (ABOX_SBANK_SIZE_L - 4);

	ret = snd_soc_component_update_bits(cmpnt, reg, mask, val);
	if (ret < 0) {
		abox_err(cmpnt->dev, "sbank write error: %d\n", ret);
		return ret;
	} else if (ret > 0) {
		abox_info(cmpnt->dev, "%s(%#x, %ums): %u\n", __func__, id, time, size);
	}

	return size;
}

int abox_cmpnt_reset_cnt_val(struct abox_data *data, enum abox_dai id)
{
	struct device *dev = data->dev;
	struct snd_soc_component *cmpnt = data->cmpnt;
	unsigned int src_id, sifs_id;
	int ret;

	abox_dbg(dev, "%s(%#x)\n", __func__, id);

	src_id = get_source_dai_id(data, id);
	if (src_id < ABOX_SIFS0 || src_id > ABOX_SIFS7)
		return -EINVAL;

	sifs_id = src_id - ABOX_SIFS0;
	ret = snd_soc_component_write(cmpnt,
			ABOX_SPUS_CTRL_SIFS_CNT_VAL(sifs_id), 0);
	if (ret < 0)
		return ret;
	abox_info(dev, "reset sifs%d_cnt_val\n", sifs_id);

	return ret;
}

static struct snd_soc_dai_driver abox_cmpnt_dai_drv[] = {
	{
		.name = "USB",
		.id = ABOX_USB,
		.playback = {
			.stream_name = "USB Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "USB Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
	{
		.name = "FWD",
		.id = ABOX_FWD,
		.playback = {
			.stream_name = "FWD Playback",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
		.capture = {
			.stream_name = "FWD Capture",
			.channels_min = 1,
			.channels_max = 8,
			.rates = ABOX_SAMPLING_RATES,
			.rate_min = 8000,
			.rate_max = 384000,
			.formats = ABOX_SAMPLE_FORMATS,
		},
	},
};

int abox_cmpnt_update_cnt_val(struct device *adev)
{
	/* nothing to do anymore */
	return 0;
}

int abox_cmpnt_get_rdma_dst_bit_width(struct abox_data *data, struct device *dev_dma, int id)
{
	struct snd_soc_dai *dai_dma;
	struct snd_pcm_hw_params params;
	int width, ret;

	width = get_width_for_spus_atune(data, id);
	if (width > 0)
		return width;

	dai_dma = abox_dma_get_dai(dev_dma, DMA_DAI_PCM);
	if (IS_ERR(dai_dma))
		return PTR_ERR(dai_dma);

	ret = rdma_hw_params_fixup(dai_dma, &params);
	if (ret < 0)
		return ret;

	return params_width(&params);
}

int abox_cmpnt_get_wdma_dst_bit_width(struct abox_data *data, struct device *dev_dma, int id)
{
	struct snd_soc_dai *dai_dma;
	struct snd_pcm_hw_params params;
	int width, ret;

	width = get_width_for_spum_atune(data, id);
	if (width > 0)
		return width;

	dai_dma = abox_dma_get_dai(dev_dma, DMA_DAI_PCM);
	if (IS_ERR(dai_dma))
		return PTR_ERR(dai_dma);

	ret = wdma_hw_params_fixup(dai_dma, &params);
	if (ret < 0)
		return ret;

	return params_width(&params);
}

int abox_cmpnt_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct device *dev = dai->dev;
	struct abox_data *data;
	int ret;

	abox_dbg(dev, "%s(%s)\n", __func__, dai->name);

	switch (dai->id) {
	case ABOX_RDMA0:
	case ABOX_RDMA1:
	case ABOX_RDMA2:
	case ABOX_RDMA3:
	case ABOX_RDMA4:
	case ABOX_RDMA5:
	case ABOX_RDMA6:
	case ABOX_RDMA7:
	case ABOX_RDMA8:
	case ABOX_RDMA9:
	case ABOX_RDMA10:
	case ABOX_RDMA11:
	case ABOX_RDMA12:
	case ABOX_RDMA13:
	case ABOX_RDMA14:
	case ABOX_RDMA15:
		ret = rdma_hw_params_fixup(dai, params);
		break;
	case ABOX_UAIF0:
	case ABOX_UAIF1:
	case ABOX_UAIF2:
	case ABOX_UAIF3:
	case ABOX_UAIF4:
	case ABOX_UAIF5:
	case ABOX_UAIF6:
	case ABOX_SPDY:
		ret = abox_if_hw_params_fixup(dai, params);
		break;
	case ABOX_UDMA_RD0:
	case ABOX_UDMA_RD1:
	case ABOX_UDMA_WR0:
	case ABOX_UDMA_WR1:
		ret = udma_hw_params_fixup(dai, params);
		break;
	case ABOX_SIFS0:
	case ABOX_SIFS1:
	case ABOX_SIFS2:
	case ABOX_SIFS3:
	case ABOX_SIFS4:
	case ABOX_SIFS5:
	case ABOX_SIFS6:
	case ABOX_SIFS7:
		/* deprecated call-path */
		data = dev_get_drvdata(dev);
		ret = sifs_hw_params_fixup(data, dai->id, params);
		break;
	case ABOX_NSRC0:
	case ABOX_NSRC1:
	case ABOX_NSRC2:
	case ABOX_NSRC3:
	case ABOX_NSRC4:
	case ABOX_NSRC5:
	case ABOX_NSRC6:
	case ABOX_NSRC7:
	case ABOX_NSRC8:
	case ABOX_NSRC9:
	case ABOX_NSRC10:
	case ABOX_NSRC11:
	case ABOX_NSRC12:
	case ABOX_NSRC13:
	case ABOX_NSRC14:
	case ABOX_NSRC15:
		/* deprecated call-path */
		data = dev_get_drvdata(dev);
		ret = sifm_hw_params_fixup(data, dai->id, params);
		break;
	case ABOX_RDMA0_BE:
	case ABOX_RDMA1_BE:
	case ABOX_RDMA2_BE:
	case ABOX_RDMA3_BE:
	case ABOX_RDMA4_BE:
	case ABOX_RDMA5_BE:
	case ABOX_RDMA6_BE:
	case ABOX_RDMA7_BE:
	case ABOX_RDMA8_BE:
	case ABOX_RDMA9_BE:
	case ABOX_RDMA10_BE:
	case ABOX_RDMA11_BE:
	case ABOX_RDMA12_BE:
	case ABOX_RDMA13_BE:
	case ABOX_RDMA14_BE:
	case ABOX_RDMA15_BE:
		ret = rdma_be_hw_params_fixup(rtd, params);
		break;
	case ABOX_WDMA0_BE:
	case ABOX_WDMA1_BE:
	case ABOX_WDMA2_BE:
	case ABOX_WDMA3_BE:
	case ABOX_WDMA4_BE:
	case ABOX_WDMA5_BE:
	case ABOX_WDMA6_BE:
	case ABOX_WDMA7_BE:
	case ABOX_WDMA8_BE:
	case ABOX_WDMA9_BE:
	case ABOX_WDMA10_BE:
	case ABOX_WDMA11_BE:
	case ABOX_WDMA12_BE:
	case ABOX_WDMA13_BE:
	case ABOX_WDMA14_BE:
	case ABOX_WDMA15_BE:
		ret = wdma_be_hw_params_fixup(rtd, params);
		break;
	default:
		abox_err(dev, "invalid hw_params fixup request\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int register_if_routes(struct device *dev,
		const struct snd_soc_dapm_route *route_base, int num,
		struct snd_soc_dapm_context *dapm, const char *name)
{
	struct snd_soc_dapm_route *route;
	int i;

	route = devm_kmemdup(dev, route_base, sizeof(*route_base) * num,
			GFP_KERNEL);
	if (!route)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		if (route[i].sink)
			route[i].sink = devm_kasprintf(dev, GFP_KERNEL,
					route[i].sink, name);
		if (route[i].control)
			route[i].control = devm_kasprintf(dev, GFP_KERNEL,
					route[i].control, name);
		if (route[i].source)
			route[i].source = devm_kasprintf(dev, GFP_KERNEL,
					route[i].source, name);
	}

	snd_soc_dapm_add_routes(dapm, route, num);
	devm_kfree(dev, route);

	return 0;
}

static const struct snd_soc_dapm_route route_base_if_pla[] = {
	/* sink, control, source */
	{"%s Playback", NULL, "%s PLA"},
};

static const struct snd_soc_dapm_route route_base_if_cap[] = {
	/* sink, control, source */
	{"SPUSM", "%s", "%s CAP"},
	{"SPUST", "%s", "%s CAP"},
	{"%s CAP", "%s Switch", "%s Capture"},
};

int abox_cmpnt_register_if(struct device *dev_abox,
		struct device *dev, unsigned int id, const char *name,
		bool playback, bool capture)
{
	struct abox_data *data = dev_get_drvdata(dev_abox);
	struct snd_soc_dapm_context *dapm;
	int ret;

	if (id >= ARRAY_SIZE(data->dev_if)) {
		abox_err(dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	dapm = snd_soc_component_get_dapm(data->cmpnt);

	data->dev_if[id] = dev;
	if (id >= data->if_count)
		data->if_count = id + 1;

	if (playback) {
		ret = register_if_routes(dev, route_base_if_pla,
				ARRAY_SIZE(route_base_if_pla), dapm, name);
		if (ret < 0)
			return ret;
	}

	if (capture) {
		ret = register_if_routes(dev, route_base_if_cap,
				ARRAY_SIZE(route_base_if_cap), dapm, name);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int register_rdma_routes(struct abox_data *data,
		const struct snd_soc_dapm_route *routes, int num_routes,
		const char *wname, int id)
{
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(data->cmpnt);
	struct snd_soc_dapm_route *route;
	int i, ret = 0;

	route = kmemdup(routes, sizeof(*routes) * num_routes, GFP_KERNEL);
	if (!route)
		return -ENOMEM;

	for (i = 0; i < num_routes; i++) {
		if (route[i].sink) {
			route[i].sink = kasprintf(GFP_KERNEL, route[i].sink, id);
			if (!route[i].sink)
				ret = -ENOMEM;
		}
		if (route[i].control) {
			route[i].control = kasprintf(GFP_KERNEL, route[i].control);
			if (!route[i].control)
				ret = -ENOMEM;
		}
		if (route[i].source) {
			route[i].source = kasprintf(GFP_KERNEL, route[i].source, wname);
			if (!route[i].source)
				ret = -ENOMEM;
		}
	}

	if (ret >= 0)
		ret = snd_soc_dapm_add_routes(dapm, route, num_routes);

	for (i = 0; i < num_routes; i++) {
		kfree(route[i].sink);
		kfree(route[i].control);
		kfree(route[i].source);
	}
	kfree(route);

	return ret;
}

static const struct snd_soc_dapm_route route_base_rdma[] = {
	/* sink, control, source */
	{"SPUS IN%d", NULL, "%s"},
};

int abox_cmpnt_register_rdma(struct abox_data *data, struct device *dev,
		unsigned int id, const char *wname)
{
	abox_dbg(data->dev, "%s(%u, %s)\n", __func__, id, wname);

	if (id >= ARRAY_SIZE(data->dev_rdma)) {
		abox_err(dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	data->dev_rdma[id] = dev;
	if (id >= data->rdma_count)
		data->rdma_count = id + 1;

	return register_rdma_routes(data, route_base_rdma,
			ARRAY_SIZE(route_base_rdma), wname, id);
}

static int register_wdma_routes(struct abox_data *data,
		const struct snd_soc_dapm_route *routes, int num_routes,
		const char *wname, int id)
{
	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(data->cmpnt);
	struct snd_soc_dapm_route *route;
	int i, ret = 0;

	route = kmemdup(routes, sizeof(*routes) * num_routes, GFP_KERNEL);
	if (!route)
		return -ENOMEM;

	for (i = 0; i < num_routes; i++) {
		if (route[i].sink) {
			route[i].sink = kasprintf(GFP_KERNEL, route[i].sink, wname);
			if (!route[i].sink)
				ret = -ENOMEM;
		}
		if (route[i].control) {
			route[i].control = kasprintf(GFP_KERNEL, route[i].control);
			if (!route[i].control)
				ret = -ENOMEM;
		}
		if (route[i].source) {
			route[i].source = kasprintf(GFP_KERNEL, route[i].source, id);
			if (!route[i].source)
				ret = -ENOMEM;
		}
	}

	if (ret >= 0)
		ret = snd_soc_dapm_add_routes(dapm, route, num_routes);

	for (i = 0; i < num_routes; i++) {
		kfree(route[i].sink);
		kfree(route[i].control);
		kfree(route[i].source);
	}
	kfree(route);

	return ret;
}

static const struct snd_soc_dapm_route route_base_wdma[] = {
	/* sink, control, source */
	{"%s", "WDMA", "SIFM%d"},
};

int abox_cmpnt_register_wdma(struct abox_data *data, struct device *dev,
		unsigned int id, const char *wname)
{
	abox_dbg(data->dev, "%s(%u, %s)\n", __func__, id, wname);

	if (id >= ARRAY_SIZE(data->dev_wdma)) {
		abox_err(dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	data->dev_wdma[id] = dev;
	if (id >= data->wdma_count)
		data->wdma_count = id + 1;

	return register_wdma_routes(data, route_base_wdma,
			ARRAY_SIZE(route_base_wdma), wname, id);
}

enum udma_spus_widget_id {
	UDMA_SPUS_WIDGET_SIFS,
	UDMA_SPUS_WIDGET_COUNT,
};
static const struct snd_soc_dapm_widget udma_spus_widgets[][UDMA_SPUS_WIDGET_COUNT] = {
	{
		SND_SOC_DAPM_MIXER_E("UDMA SIFS0", SND_SOC_NOPM, 0, 0, NULL, 0,
				udma_sifs_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	},
	{
		SND_SOC_DAPM_MIXER_E("UDMA SIFS1", SND_SOC_NOPM, 1, 0, NULL, 0,
				udma_sifs_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	},
};

static int register_udma_spus_widgets(struct snd_soc_component *cmpnt, unsigned int id)
{
	int ret;

	if (id >= ARRAY_SIZE(udma_spus_widgets))
		return -EINVAL;

	ret = snd_soc_dapm_new_controls(snd_soc_component_get_dapm(cmpnt),
			udma_spus_widgets[id], ARRAY_SIZE(udma_spus_widgets[id]));
	if (ret < 0)
		abox_err(cmpnt->dev, "Failed to register udma spus %d widgets: %d\n", id, ret);

	return ret;
}

static const struct snd_soc_dapm_route udma_spus_routes_template[] = {
	{"NSRC0", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC1", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC2", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC3", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC4", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC5", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC6", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC7", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC8", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC9", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC10", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC11", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC12", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC13", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC14", "UDMA_SIFS%d", "UDMA SIFS%d"},
	{"NSRC15", "UDMA_SIFS%d", "UDMA SIFS%d"},
};

static int register_udma_spus(struct snd_soc_component *cmpnt, unsigned int id)
{
	int ret;

	ret = register_udma_spus_widgets(cmpnt, id);
	if (ret < 0)
		return ret;

	ret = register_routes_template(cmpnt, id, udma_spus_routes_template,
			ARRAY_SIZE(udma_spus_routes_template));
	if (ret < 0)
		return ret;

	return 0;
}

static const struct snd_soc_dapm_route route_base_udma_rd[] = {
	/* sink, control, source */
	{"UDMA SIFS%d", NULL, "%s Capture"},
};

int abox_cmpnt_register_udma_rd(struct device *dev_abox,
		struct device *dev, unsigned int id, const char *name)
{
	struct abox_data *data = dev_get_drvdata(dev_abox);
	int ret;

	if (id >= ARRAY_SIZE(data->dev_udma_rd)) {
		abox_err(dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	data->dev_udma_rd[id] = dev;
	if (id >= data->udma_rd_count)
		data->udma_rd_count = id + 1;

	ret = register_udma_spus(data->cmpnt, id);
	if (ret < 0)
		return ret;

	/* reuse register_rdma_routes() */
	return register_rdma_routes(data, route_base_udma_rd,
			ARRAY_SIZE(route_base_udma_rd), name, id);
}

enum udma_spum_widget_id {
	UDMA_SPUM_WIDGET_SIFM,
	UDMA_SPUM_WIDGET_COUNT,
};
static const struct snd_soc_dapm_widget udma_spum_widgets[][UDMA_SPUM_WIDGET_COUNT] = {
	{
		SND_SOC_DAPM_MUX_E("UDMA SIFM0", SND_SOC_NOPM, 0, 0, udma_sifm0_controls,
				udma_sifm_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	},
	{
		SND_SOC_DAPM_MUX_E("UDMA SIFM1", SND_SOC_NOPM, 1, 0, udma_sifm1_controls,
				udma_sifm_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),	},
};

static int register_udma_spum_widgets(struct snd_soc_component *cmpnt, unsigned int id)
{
	int ret;

	if (id >= ARRAY_SIZE(udma_spum_widgets))
		return -EINVAL;

	ret = snd_soc_dapm_new_controls(snd_soc_component_get_dapm(cmpnt),
			udma_spum_widgets[id], ARRAY_SIZE(udma_spum_widgets[id]));
	if (ret < 0)
		abox_err(cmpnt->dev, "Failed to register udma spum %d widgets: %d\n", id, ret);

	return ret;
}

static const struct snd_soc_dapm_route udma_spum_routes_template[] = {
	{"UDMA SIFM%d", "SIFS0", "SIFS0 OUT"},
	{"UDMA SIFM%d", "SIFS1", "SIFS1 OUT"},
	{"UDMA SIFM%d", "SIFS2", "SIFS2 OUT"},
	{"UDMA SIFM%d", "SIFS3", "SIFS3 OUT"},
	{"UDMA SIFM%d", "SIFS4", "SIFS4 OUT"},
	{"UDMA SIFM%d", "SIFS5", "SIFS5 OUT"},
	{"UDMA SIFM%d", "SIFS6", "SIFS6 OUT"},
	{"UDMA SIFM%d", "SIFS7", "SIFS7 OUT"},
};

static int register_udma_spum(struct snd_soc_component *cmpnt, unsigned int id)
{
	int ret;

	ret = register_udma_spum_widgets(cmpnt, id);
	if (ret < 0)
		return ret;

	ret = register_routes_template(cmpnt, id, udma_spum_routes_template,
			ARRAY_SIZE(udma_spum_routes_template));
	if (ret < 0)
		return ret;

	return 0;
}

static const struct snd_soc_dapm_route route_base_udma_wr[] = {
	/* sink, control, source */
	{"%s Playback", NULL, "UDMA SIFM%d"},
};

int abox_cmpnt_register_udma_wr(struct device *dev_abox,
		struct device *dev, unsigned int id, const char *name)
{
	struct abox_data *data = dev_get_drvdata(dev_abox);
	int ret;

	if (id >= ARRAY_SIZE(data->dev_udma_wr)) {
		abox_err(dev, "%s: invalid id(%u)\n", __func__, id);
		return -EINVAL;
	}

	data->dev_udma_wr[id] = dev;
	if (id >= data->udma_wr_count)
		data->udma_wr_count = id + 1;

	ret = register_udma_spum(data->cmpnt, id);
	if (ret < 0)
		return ret;

	/* reuse register_wdma_routes() */
	return register_wdma_routes(data, route_base_udma_wr,
			ARRAY_SIZE(route_base_udma_wr), name, id);
}

int abox_cmpnt_restore(struct device *adev)
{
	struct abox_data *data = dev_get_drvdata(adev);
	enum abox_dai dai;

	abox_dbg(adev, "%s\n", __func__);

	if (!data->cmpnt)
		return -EINVAL;

	for (dai = ABOX_NSRC0; dai < ABOX_NSRC0 + COUNT_SIFM; dai++)
		nsrcx_put_ipc(adev, dai, get_nsrc_source(data->cmpnt, dai));
	if (s_default)
		asrc_factor_put_ipc(adev, s_default, SET_ASRC_FACTOR_CP);
	if (data->audio_mode)
		audio_mode_put_ipc(adev, data->audio_mode);
	if (data->sound_type)
		sound_type_put_ipc(adev, data->sound_type);
	if (data->debug_mode == DEBUG_MODE_NONE)
		debug_put_ipc(adev, data->debug_mode);

	return 0;
}

int abox_cmpnt_register(struct device *adev)
{
	const struct snd_soc_component_driver *cmpnt_drv;
	struct snd_soc_dai_driver *dai_drv;
	int num_dai;

	cmpnt_drv = &abox_cmpnt_drv;
	dai_drv = abox_cmpnt_dai_drv;
	num_dai = ARRAY_SIZE(abox_cmpnt_dai_drv);

	return snd_soc_register_component(adev, cmpnt_drv, dai_drv, num_dai);
}
