// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Specific feature
 *
 * Copyright (C) 2024 Samsung Electronics Co., Ltd.
 *
 * Authors:
 *      Storage Driver <storage.sec@samsung.com>
 */

#include <linux/sysfs.h>
#include <linux/of_gpio.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>

#include "dw_mmc.h"
#include "dw_mmc-exynos.h"
#include "mmc-sec-feature.h"
#include "mmc-sec-sysfs.h"

static const char *const uhs_speeds[] = {
	[UHS_SDR12_BUS_SPEED] = "SDR12",
	[UHS_SDR25_BUS_SPEED] = "SDR25",
	[UHS_SDR50_BUS_SPEED] = "SDR50",
	[UHS_SDR104_BUS_SPEED] = "SDR104",
	[UHS_DDR50_BUS_SPEED] = "DDR50",
};

/* Card states */
#define MMC_STATE_READONLY	(1<<1)		/* card is read-only */

struct device *sec_sdcard_cmd_dev;
EXPORT_SYMBOL_GPL(sec_sdcard_cmd_dev);

struct device *sec_sdinfo_cmd_dev;
EXPORT_SYMBOL_GPL(sec_sdinfo_cmd_dev);

struct device *sec_sddata_cmd_dev;
EXPORT_SYMBOL_GPL(sec_sddata_cmd_dev);

static inline void sd_sec_get_size(struct mmc_card *card, char *buf, int len)
{
	static const char *const unit[] = {"KB", "MB", "GB", "TB"};
	int capacity;
	int digit = 1;

	if (card->csd.read_blkbits == 9) /* 1 Sector = 512 Bytes */
		capacity = (card->csd.capacity) >> 1;
	else if (card->csd.read_blkbits == 11) /* 1 Sector = 2048 Bytes */
		capacity = (card->csd.capacity) << 1;
	else /* 1 Sector = 1024 Bytes */
		capacity = card->csd.capacity;

	if (capacity >= 380000000 && capacity <= 410000000)
		snprintf(buf, len, "400GB");
	else if (capacity >= 190000000 && capacity <= 210000000)
		snprintf(buf, len, "200GB");
	else {
		while ((capacity >> 1) > 0) {
			capacity >>= 1;
			digit++;
		}
		snprintf(buf, len, "%d%s", 1 << (digit % 10), unit[digit / 10]);
	}
}

static inline void sd_sec_get_speedmode(struct mmc_card *card,
		const char **speedmode)
{
	if (mmc_card_uhs(card))
		*speedmode = uhs_speeds[card->sd_bus_speed];
	else if (mmc_card_hs(card))
		*speedmode = "HS";
	else
		*speedmode = "DS";
}

static ssize_t sd_sec_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (gpio_get_value(priv->cd_gpio) ^ (host->pdata->use_gpio_invert)) {
		if (sdi.sd_slot_type == SEC_HYBRID_SD_SLOT) {
			dev_info(host->dev, "SD slot tray removed.\n");
			return sprintf(buf, "Notray\n");
		}
	}

	if (host->slot && host->slot->mmc && host->slot->mmc->card) {
		dev_info(host->dev, "SD card inserted.\n");
		return sprintf(buf, "Insert\n");
	}

	dev_info(host->dev, "SD card removed.\n");
	return sprintf(buf, "Remove\n");
}

static ssize_t sd_sec_maxmode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct device_node *np = host->dev->of_node;
	const char *bus_speed_mode = "";

	if (of_find_property(np, "sd-uhs-sdr104", NULL))
		bus_speed_mode = uhs_speeds[UHS_SDR104_BUS_SPEED];
	else if (of_find_property(np, "sd-uhs-ddr50", NULL))
		bus_speed_mode = uhs_speeds[UHS_DDR50_BUS_SPEED];
	else if (of_find_property(np, "sd-uhs-sdr50", NULL))
		bus_speed_mode = uhs_speeds[UHS_SDR50_BUS_SPEED];
	else if (of_find_property(np, "sd-uhs-sdr25", NULL))
		bus_speed_mode = uhs_speeds[UHS_SDR25_BUS_SPEED];
	else if (of_find_property(np, "sd-uhs-sdr12", NULL))
		bus_speed_mode = uhs_speeds[UHS_SDR12_BUS_SPEED];
	else if (of_find_property(np, "cap-sd-highspeed", NULL))
		bus_speed_mode = "HS";
	else
		bus_speed_mode = "DS";

	dev_info(host->dev, "%s : Max supported host speed mode = %s\n",
				__func__, bus_speed_mode);
	return sprintf(buf, "%s\n", bus_speed_mode);
}

static ssize_t sd_sec_curmode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_card *card;
	const char *bus_speed_mode = "";

	if (host->slot && host->slot->mmc && host->slot->mmc->card) {
		card = host->slot->mmc->card;
		sd_sec_get_speedmode(card, &bus_speed_mode);
	} else
		bus_speed_mode = "No Card";

	dev_info(host->dev, "%s : Current SD card speed = %s\n",
				__func__, bus_speed_mode);
	return sprintf(buf, "%s\n", bus_speed_mode);
}

static ssize_t sd_sec_detect_cnt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	dev_info(dev, "%s : CD count is = %u\n",
					__func__, sdi.card_detect_cnt);
	return sprintf(buf, "%u\n", sdi.card_detect_cnt);
}

static inline void sd_sec_calc_error_count(struct sd_sec_err_info *err_log,
		unsigned long long *crc_cnt, unsigned long long *tmo_cnt)
{
	int i = 0;

	/* Only sbc(0,1)/cmd(2,3)/data(4,5) is checked. */
	for (i = 0; i < 6; i++) {
		if (err_log[i].err_type == -EILSEQ && *crc_cnt < U64_MAX)
			*crc_cnt += err_log[i].count;
		if (err_log[i].err_type == -ETIMEDOUT && *tmo_cnt < U64_MAX)
			*tmo_cnt += err_log[i].count;
	}
}

static ssize_t sd_sec_summary_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_card *card;
	struct sd_sec_status_err_info *status_err = &sdi.status_err;
	unsigned int serial;
	char size[6];
	const char *bus_speed_mode = "";

	if (host->slot && host->slot->mmc && host->slot->mmc->card) {
		card = host->slot->mmc->card;

		/* MANID */
		/* SERIAL */
		serial = card->cid.serial & (0x0000FFFF);

		/*SIZE*/
		sd_sec_get_size(card, size, sizeof(size));

		/* SPEEDMODE */
		sd_sec_get_speedmode(card, &bus_speed_mode);

		/* SUMMARY */
		dev_info(host->dev, "MANID : 0x%02X, SERIAL : %04X, SIZE : %s, SPEEDMODE : %s\n",
				card->cid.manfid, serial, size, bus_speed_mode);
		return sprintf(buf, "\"MANID\":\"0x%02X\",\"SERIAL\":\"%04X\""\
				",\"SIZE\":\"%s\",\"SPEEDMODE\":\"%s\",\"NOTI\":\"%d\"\n",
				card->cid.manfid, serial, size, bus_speed_mode,
				status_err->noti_cnt);
	} else {
		/* SUMMARY : No SD Card Case */
		dev_info(host->dev, "%s : No SD Card\n", __func__);
		return sprintf(buf, "\"MANID\":\"NoCard\",\"SERIAL\":\"NoCard\""\
			",\"SIZE\":\"NoCard\",\"SPEEDMODE\":\"NoCard\",\"NOTI\":\"NoCard\"\n");
	}
}

static ssize_t sd_sec_error_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->slot->mmc;
	struct mmc_card *card = mmc->card;
	struct sd_sec_err_info *err_log = &sdi.err_info[0];
	struct sd_sec_status_err_info *status_err = &sdi.status_err;
	u64 crc_cnt = 0;
	u64 tmo_cnt = 0;
	int len = 0;
	int i;

	if (!card) {
		len = snprintf(buf, PAGE_SIZE, "No card\n");
		goto out;
	}

	len += snprintf(buf, PAGE_SIZE,
				"type : err    status: first_issue_time:  last_issue_time:      count\n");

	for (i = 0; i < MAX_LOG_INDEX; i++) {
		len += snprintf(buf + len, PAGE_SIZE - len,
				"%5s:%4d 0x%08x %16llu, %16llu, %10d\n",
				err_log[i].type, err_log[i].err_type,
				err_log[i].status,
				err_log[i].first_issue_time,
				err_log[i].last_issue_time,
				err_log[i].count);
	}

	sd_sec_calc_error_count(err_log, &crc_cnt, &tmo_cnt);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"GE:%d,CC:%d,ECC:%d,WP:%d,OOR:%d,CRC:%lld,TMO:%lld\n",
			status_err->ge_cnt, status_err->cc_cnt,
			status_err->ecc_cnt, status_err->wp_cnt,
			status_err->oor_cnt, crc_cnt, tmo_cnt);

out:
	return len;
}

static ssize_t sd_sec_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->slot->mmc;
	struct mmc_card *card = mmc->card;
	struct sd_sec_err_info *err_log = &sdi.err_info[0];
	u64 total_cnt = 0;
	int len = 0;
	int i;

	if (!card) {
		len = snprintf(buf, PAGE_SIZE, "no card\n");
		goto out;
	}

	for (i = 0; i < 6; i++) {
		if (total_cnt < U64_MAX)
			total_cnt += err_log[i].count;
	}
	len = snprintf(buf, PAGE_SIZE, "%lld\n", total_cnt);

out:
	return len;
}

static ssize_t sd_sec_cid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->slot->mmc;
	struct mmc_card *card = mmc->card;

	if (!card)
		return snprintf(buf, PAGE_SIZE, "no card\n");

	return snprintf(buf, PAGE_SIZE,
			"%08x%08x%08x%08x\n",
			card->raw_cid[0], card->raw_cid[1],
			card->raw_cid[2], card->raw_cid[3]);
}

#define SD_SEC_CALC_STATUS_ERR(member) ({	\
		cur_status_err->member = status_err->member - saved_status_err->member; })

static inline void sd_sec_get_curr_err_info(unsigned long long *crc_cnt,
		unsigned long long *tmo_cnt, struct sd_sec_status_err_info *cur_status_err)
{
	struct sd_sec_err_info *err_log = &sdi.err_info[0];
	struct sd_sec_err_info *saved_err_log = &sdi.saved_err_info[0];
	struct sd_sec_status_err_info *status_err = &sdi.status_err;
	struct sd_sec_status_err_info *saved_status_err = &sdi.saved_status_err;
	int i;

	/* Only sbc(0,1)/cmd(2,3)/data(4,5) is checked. */
	for (i = 0; i < 6; i++) {
		if (err_log[i].err_type == -EILSEQ && *crc_cnt < U64_MAX)
			*crc_cnt += (err_log[i].count - saved_err_log[i].count);
		if (err_log[i].err_type == -ETIMEDOUT && *tmo_cnt < U64_MAX)
			*tmo_cnt += (err_log[i].count - saved_err_log[i].count);
	}

	SD_SEC_CALC_STATUS_ERR(ge_cnt);
	SD_SEC_CALC_STATUS_ERR(cc_cnt);
	SD_SEC_CALC_STATUS_ERR(ecc_cnt);
	SD_SEC_CALC_STATUS_ERR(wp_cnt);
	SD_SEC_CALC_STATUS_ERR(oor_cnt);
	SD_SEC_CALC_STATUS_ERR(noti_cnt);
}

#define SD_SEC_SAVE_STATUS_ERR(member) ({		\
		saved_status_err->member = status_err->member;	})

static inline void sd_sec_save_err_info(void)
{
	struct sd_sec_err_info *err_log = &sdi.err_info[0];
	struct sd_sec_err_info *saved_err_log = &sdi.saved_err_info[0];
	struct sd_sec_status_err_info *status_err = &sdi.status_err;
	struct sd_sec_status_err_info *saved_status_err = &sdi.saved_status_err;
	int i;

	/* Save current error count */
	for (i = 0; i < MAX_LOG_INDEX; i++)
		saved_err_log[i].count = err_log[i].count;

	SD_SEC_SAVE_STATUS_ERR(ge_cnt);
	SD_SEC_SAVE_STATUS_ERR(cc_cnt);
	SD_SEC_SAVE_STATUS_ERR(ecc_cnt);
	SD_SEC_SAVE_STATUS_ERR(wp_cnt);
	SD_SEC_SAVE_STATUS_ERR(oor_cnt);
	SD_SEC_SAVE_STATUS_ERR(noti_cnt);
}

static ssize_t sd_sec_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->slot->mmc;
	struct mmc_card *card = mmc->card;
	struct sd_sec_status_err_info status_err;
	u64 crc_cnt = 0;
	u64 tmo_cnt = 0;
	int len = 0;

	if (!card) {
		len = snprintf(buf, PAGE_SIZE,
			"\"GE\":\"0\",\"CC\":\"0\",\"ECC\":\"0\",\"WP\":\"0\"," \
			"\"OOR\":\"0\",\"CRC\":\"0\",\"TMO\":\"0\"\n");
		goto out;
	}

	memset(&status_err, 0, sizeof(struct sd_sec_status_err_info));

	sd_sec_get_curr_err_info(&crc_cnt, &tmo_cnt, &status_err);

	len = snprintf(buf, PAGE_SIZE,
		"\"GE\":\"%d\",\"CC\":\"%d\",\"ECC\":\"%d\",\"WP\":\"%d\"," \
		"\"OOR\":\"%d\",\"CRC\":\"%lld\",\"TMO\":\"%lld\"\n",
		status_err.ge_cnt,
		status_err.cc_cnt,
		status_err.ecc_cnt,
		status_err.wp_cnt,
		status_err.oor_cnt,
		crc_cnt, tmo_cnt);
out:
	return len;
}

static ssize_t sd_sec_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->slot->mmc;
	struct mmc_card *card = mmc->card;

	if (!card)
		return -ENODEV;

	if ((buf[0] != 'C' && buf[0] != 'c') || (count != 1))
		return -EINVAL;

	sd_sec_save_err_info();

	return count;
}

static ssize_t sd_sec_health_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->slot->mmc;
	struct mmc_card *card = mmc->card;
	struct sd_sec_err_info *err_log = &sdi.err_info[0];
	struct sd_sec_status_err_info *status_err = &sdi.status_err;
	u64 crc_cnt = 0;
	u64 tmo_cnt = 0;
	int len = 0;

	if (!card) {
		/* There should be no spaces in 'No Card'(Vold Team). */
		len = snprintf(buf, PAGE_SIZE, "NOCARD\n");
		goto out;
	}

	sd_sec_calc_error_count(err_log, &crc_cnt, &tmo_cnt);

	if (status_err->ge_cnt > 100 || status_err->ecc_cnt > 0 ||
			status_err->wp_cnt > 0 || status_err->oor_cnt > 10 ||
			tmo_cnt > 100 || crc_cnt > 100)
		len = snprintf(buf, PAGE_SIZE, "BAD\n");
	else
		len = snprintf(buf, PAGE_SIZE, "GOOD\n");

out:
	return len;
}

static ssize_t sd_sec_reason_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_host *mmc = host->slot->mmc;
	struct mmc_card *card = mmc->card;
	int len = 0;

	if (!card) {
		len = snprintf(buf, PAGE_SIZE, "%s\n",
				mmc->unused ? "INITFAIL" : "NOCARD");
		goto out;
	}

	len = snprintf(buf, PAGE_SIZE, "%s\n",
			card->state & MMC_STATE_READONLY ? "PERMWP" : "NORMAL");
out:
	return len;
}

static DEVICE_ATTR(status, 0444, sd_sec_status_show, NULL);
static DEVICE_ATTR(max_mode, 0444, sd_sec_maxmode_show, NULL);
static DEVICE_ATTR(current_mode, 0444, sd_sec_curmode_show, NULL);
static DEVICE_ATTR(cd_cnt, 0444, sd_sec_detect_cnt_show, NULL);
static DEVICE_ATTR(sdcard_summary, 0444, sd_sec_summary_show, NULL);
static DEVICE_ATTR(err_count, 0444, sd_sec_error_count_show, NULL);

static DEVICE_ATTR(sd_count, 0444, sd_sec_count_show, NULL);
static DEVICE_ATTR(data, 0444, sd_sec_cid_show, NULL);
static DEVICE_ATTR(fc, 0444, sd_sec_health_show, NULL);
static DEVICE_ATTR(reason, 0444, sd_sec_reason_show, NULL);

static DEVICE_ATTR(sd_data, 0664, sd_sec_data_show, sd_sec_data_store);

static struct attribute *sdcard_attributes[] = {
	&dev_attr_status.attr,
	&dev_attr_max_mode.attr,
	&dev_attr_current_mode.attr,
	&dev_attr_cd_cnt.attr,
	&dev_attr_sdcard_summary.attr,
	&dev_attr_err_count.attr,
	NULL,
};

static struct attribute_group sdcard_attr_group = {
	.attrs = sdcard_attributes,
};

static struct attribute *sdinfo_attributes[] = {
	&dev_attr_sd_count.attr,
	&dev_attr_data.attr,
	&dev_attr_fc.attr,
	&dev_attr_reason.attr,
	NULL,
};

static struct attribute_group sdinfo_attr_group = {
	.attrs = sdinfo_attributes,
};

static struct attribute *sddata_attributes[] = {
	&dev_attr_sd_data.attr,
	NULL,
};

static struct attribute_group sddata_attr_group = {
	.attrs = sddata_attributes,
};

static void sd_sec_create_sysfs_group(struct device *dev,
		const struct attribute_group *dev_attr_group,
		const char *group_name)
{
	if (sysfs_create_group(&dev->kobj, dev_attr_group))
		pr_err("%s: Failed to create %s sysfs group\n",
				__func__, group_name);
}

void sd_sec_init_sysfs(void)
{
	if (!sec_sd_vops_create_sysfs_dev(&sdi, "sdcard"))
		sd_sec_create_sysfs_group(sec_sdcard_cmd_dev,
			&sdcard_attr_group, "sdcard");

	if (!sec_sd_vops_create_sysfs_dev(&sdi, "sdinfo"))
		sd_sec_create_sysfs_group(sec_sdinfo_cmd_dev,
			&sdinfo_attr_group, "sdinfo");

	if (!sec_sd_vops_create_sysfs_dev(&sdi, "sddata"))
		sd_sec_create_sysfs_group(sec_sddata_cmd_dev,
			&sddata_attr_group, "sddata");
}

