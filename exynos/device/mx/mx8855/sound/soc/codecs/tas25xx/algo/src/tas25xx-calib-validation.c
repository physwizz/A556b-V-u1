#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/power_supply.h>
#include "../inc/tas_smart_amp_v2.h"
#include "tas25xx-calib-validation.h"
#include "tas25xx-algo-intf.h"


#define MAX_STRING		200
#define CALIB_RETRY_COUNT	5
#define RDC_L			0
#define TEMP_L			1
#define RDC_R			2
#define TEMP_R			3
#define DEFAULT_AMBIENT_TEMP	25
#define CALIB_TIME		2
#define VALIDATION_TIME		3
#define STATUS_NONE		0x00
#define STATUS_SUCCESS		0x01
#define STATUS_FAIL		0xcc

#define VALIDATION_SUCCESS	0xC00DC00D

#define TRANSF_USER_TO_IMPED(X, Y) \
		((X << 19) + ((Y << 19) / 100))
#define QFORMAT19		19
#define QFORMAT31		31

#define TAS25XX_SYSFS_CLASS_NAME	"tas25xx"
#define TAS25XX_CALIB_DIR_NAME		"calib"
#define TAS25XX_VALID_DIR_NAME		"valid"
#define TAS25XX_BD_DIR_NAME		"bigdata"


static uint8_t calibration_result[MAX_CHANNELS] = {STATUS_NONE};
static uint8_t validation_result[MAX_CHANNELS] = {STATUS_NONE};
static bool calibration_status;
static bool validation_running_status;
static uint32_t calib_re_hold[MAX_CHANNELS] = {0};
static uint32_t amb_temp_hold[MAX_CHANNELS] = {0};
static int32_t re_low[MAX_CHANNELS] = {0};
static int32_t re_high[MAX_CHANNELS] = {0};
static int32_t s_algo_running;

static struct tas25xx_algo *p_tas25xx_algo;

static uint8_t trans_val_to_user_m(uint32_t val, uint8_t qformat);
static uint8_t trans_val_to_user_i(uint32_t val, uint8_t qformat);

struct tas25xx_algo *smartamp_get_sysfs_ptr(void)
{
	if (!p_tas25xx_algo)
		p_tas25xx_algo = kzalloc(sizeof(struct tas25xx_algo),
					GFP_KERNEL);

	if (!p_tas25xx_algo)
		return ERR_PTR(-ENOMEM);

	return p_tas25xx_algo;
}

int32_t tisa_get_q31_to_user(int32_t q31num)
{
	int32_t whole_num, dec_num;

	whole_num = (int32_t)trans_val_to_user_i(q31num, QFORMAT31);
	dec_num = (int32_t)trans_val_to_user_m(q31num, QFORMAT31);

	return (whole_num * 100) + dec_num;
}

int32_t tisa_get_q19_to_user(int32_t q19num)
{
	int32_t whole_num, dec_num;

	whole_num = (int32_t)trans_val_to_user_i(q19num, QFORMAT19);
	dec_num = (int32_t)trans_val_to_user_m(q19num, QFORMAT19);

	return (whole_num * 100) + dec_num;
}


/*Max value supported is 2^8*/
static uint8_t trans_val_to_user_m(uint32_t val, uint8_t qformat)
{
	uint32_t ret = (uint32_t)(((long long)val * 1000) >> qformat) % 1000;

	return (uint8_t)(ret / 10);
}

/*Max value supported is 2^8*/
static uint8_t trans_val_to_user_i(uint32_t val, uint8_t qformat)
{
	return ((val * 100) >> qformat) / 100;
}

static uint8_t tas25xx_get_amb_temp(void)
{
	struct power_supply *psy;
	union power_supply_propval value = {0};

	psy = power_supply_get_by_name("battery");
	if (!psy || !psy->desc || !psy->desc->get_property) {
		pr_err("[TI-SmartPA:%s] getting ambient temp failed, using default value %d\n",
			__func__, DEFAULT_AMBIENT_TEMP);
		return DEFAULT_AMBIENT_TEMP;
	}
	psy->desc->get_property(psy, POWER_SUPPLY_PROP_TEMP, &value);

	return DIV_ROUND_CLOSEST(value.intval, 10);
}

void tas25xx_update_big_data(void)
{
	uint8_t iter = 0;
	static uint32_t data[8];
	uint32_t param_id = 0;
	int32_t ret = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] BigData update skipping memory not allocated yet!!\n",
			__func__);
		return;
	}

	for (iter = 0; iter < algo->spk_count; iter++) {
		/*Reset data*/
		memset(data, 0, 8*sizeof(uint32_t));
		param_id = TAS_CALC_PARAM_IDX(TAS_SA_EXC_TEMP_STAT,
				ARRAY_SIZE(data), (0x0F & (iter+1)));
		ret = tas25xx_smartamp_algo_ctrl((u8 *)(&(data[0])), param_id
			, TAS_GET_PARAM, sizeof(data), TISA_MOD_RX);
		if (ret < 0) {
			pr_err("[TI-SmartPA:%s] Failed to get Excursion and Temperature Stats\n",
				__func__);
		} else {
			pr_info("[TI-SmartPA:%s] Emax[%d] %d(%02d.%02d), Tmax[%d] %d, EOcount[%d] %d, TOcount[%d] %d\n",
				__func__, iter, data[0],
				(int32_t)trans_val_to_user_i(data[0],
					QFORMAT31),
				(int32_t)trans_val_to_user_m(data[0],
					QFORMAT31),
				iter, data[1], iter, data[2], iter, data[3]);

			/*Update Excursion Data*/
			algo->b_data[iter].exc_max =
				(data[0] > algo->b_data[iter].exc_max) ? data[0]:algo->b_data[iter].exc_max;
			algo->b_data[iter].exc_max_persist =
				(data[0] > algo->b_data[iter].exc_max_persist) ?
					data[0]:algo->b_data[iter].exc_max_persist;
			algo->b_data[iter].exc_over_count += data[2];

			/*Update Temperature Data*/
			algo->b_data[iter].temp_max =
				(data[1] > algo->b_data[iter].temp_max) ? data[1]:algo->b_data[iter].temp_max;
			algo->b_data[iter].temp_max_persist =
				(data[1] > algo->b_data[iter].temp_max_persist) ?
					data[1]:algo->b_data[iter].temp_max_persist;
			algo->b_data[iter].temp_over_count += data[3];
		}

		memset(data, 0, 8*sizeof(uint32_t));
		param_id = TAS_CALC_PARAM_IDX(TAS_SA_LE_FLAG_STATS, 2, (iter+1));
		ret = tas25xx_smartamp_algo_ctrl((u8 *)(&(data[0])), param_id,
			TAS_GET_PARAM, 2 * sizeof(uint32_t), TISA_MOD_RX);
		if (ret < 0) {
			pr_err("[TI-SmartPA:%s] Failed to get LE flag info\n",
				__func__);
		} else {
			u64 temp = 0;

			algo->b_le_flag[iter].le_flag_detected = data[0];
			algo->b_le_flag[iter].le_flag_count = data[1];
			temp = data[1] + algo->b_le_flag[iter].le_flag_count_persist;
			if ((temp - data[1]) == algo->b_le_flag[iter].le_flag_count_persist)
				algo->b_le_flag[iter].le_flag_count_persist = temp;
			else
				algo->b_le_flag[iter].le_flag_count_persist = U64_MAX;

			pr_info("[TI-SmartPA:%s] LE Flag Detection Ch=%d detected=%u, count=%u, Total Count=%llu\n",
			__func__, iter, algo->b_le_flag[iter].le_flag_detected,
				algo->b_le_flag[iter].le_flag_count, algo->b_le_flag[iter].le_flag_count_persist);
		}
	}

	return;
}

int tas25xx_update_calibration_limits(void)
{
	uint8_t iter = 0;
	uint32_t param_id = 0;
	int32_t ret = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo)) {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		ret = PTR_ERR(algo);
		return ret;
	}

	/*Reset Calibration Result*/
	memset(calibration_result, STATUS_NONE, sizeof(uint8_t)*MAX_CHANNELS);

	for (iter = 0; iter < algo->spk_count; iter++) {
		int32_t re_range[2] = {0};

		param_id = TAS_CALC_PARAM_IDX(TAS_SA_GET_RE_RANGE, 1, (iter+1));
		ret = tas25xx_smartamp_algo_ctrl((u8 *)re_range, param_id,
			TAS_GET_PARAM, sizeof(re_range), TISA_MOD_RX);
		if (ret < 0) {
			pr_err("[TI-SmartPA:%s]get re low fail channel no %d Exiting ..\n",
				__func__, iter);
			return ret;
		}

		re_low[iter] = re_range[0] >> 8; /* Qformat 27 -> 19*/
		re_high[iter] = re_range[1] >> 8; /* Qformat 27 -> 19*/

		pr_info("[TI-SmartPA:%s] Channel No:%d, Rdc Limits(%02d.%02d ~ %02d.%02d)\n",
			__func__, iter,
			(int32_t)trans_val_to_user_i(re_low[iter], QFORMAT19),
			(int32_t)trans_val_to_user_m(re_low[iter], QFORMAT19),
			(int32_t)trans_val_to_user_i(re_high[iter], QFORMAT19),
			(int32_t)trans_val_to_user_m(re_high[iter], QFORMAT19));
	}
	return 0;
}

int tas25xx_check_limits(uint8_t channel, int32_t rdc)
{
	if ((rdc >= re_low[channel]) && (rdc <= re_high[channel])) {
		calibration_result[channel] = STATUS_SUCCESS;
		return 1;
	}
	calibration_result[channel] = STATUS_FAIL;
	return 0;
}

void tas25xx_send_algo_calibration(void)
{
	uint32_t data = 0;
	uint32_t param_id = 0;
	uint8_t iter;
	int32_t ret = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		return;
	}

	for (iter = 0; iter < algo->spk_count; iter++) {
		if (algo->calib_update[iter]) {
			/*Set ambient temperature*/
			data = algo->amb_temp[iter];
			param_id = TAS_CALC_PARAM_IDX(TAS_SA_SET_TCAL, 1,
				(iter+1));
			ret = tas25xx_smartamp_algo_ctrl((u8 *)&data, param_id,
				TAS_SET_PARAM, sizeof(uint32_t), TISA_MOD_RX);

			/*Set Re*/
			data = algo->calib_re[iter];
			param_id = TAS_CALC_PARAM_IDX(TAS_SA_SET_RE, 1,
				(iter+1));
			ret = tas25xx_smartamp_algo_ctrl((u8 *)&data, param_id,
				TAS_SET_PARAM, sizeof(uint32_t), TISA_MOD_RX);
		}
	}

	return;
}

static int tas25xx_save_calib_data(uint32_t *calib_rdc)
{
	uint8_t iter = 0;
	int ret = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		ret = PTR_ERR(algo);
		return ret;
	}

	if (!calib_rdc) {
		pr_err("[TI-SmartPA:%s] argument is Null\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	for (iter = 0; iter < algo->spk_count; iter++) {
		calib_re_hold[iter] = calib_rdc[iter];
		amb_temp_hold[iter] = tas25xx_get_amb_temp();
	}
	return 0;
}

/**********Calibration Related Codes Start*************************************/
/* Forward declerations */
static ssize_t calibration_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size);

static ssize_t calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t calibration_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t calibration_rdc_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t calibration_rdc_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size);

static ssize_t calibration_amb_temp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t calibration_amb_temp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size);

static void calib_work_routine(struct work_struct *work)
{
	uint8_t iter = 0, iter2 = 0;
	uint32_t data = 0;
	uint32_t param_id = 0;
	uint32_t calib_re[MAX_CHANNELS] = {0};
	int32_t ret = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		return;
	}

	if (tas25xx_update_calibration_limits())
		return;

	/*Get Re*/
	for (iter2 = 0; iter2 < CALIB_RETRY_COUNT; iter2++) {
		for (iter = 0; iter < algo->spk_count; iter++) {
			if (calibration_result[iter] == STATUS_SUCCESS)
				continue;

			/*Calinration Init*/
			data = 1;/*Value is ignored*/
			param_id = TAS_CALC_PARAM_IDX(TAS_SA_CALIB_INIT, 1,
				(iter+1));
			ret = tas25xx_smartamp_algo_ctrl((u8 *)&data, param_id,
			TAS_SET_PARAM, sizeof(uint32_t), TISA_MOD_RX);
			if (ret < 0) {
				pr_err("[TI-SmartPA:%s] Init error. Exiting ..\n",
					__func__);
				return;
			}

			msleep(CALIB_TIME*1000);

			data = 0;//Reset data to 0
			param_id = TAS_CALC_PARAM_IDX(TAS_SA_GET_RE, 1,
				(iter+1));
			ret = tas25xx_smartamp_algo_ctrl((u8 *)&data, param_id,
				TAS_GET_PARAM, sizeof(uint32_t), TISA_MOD_RX);
			if (ret < 0) {
				calibration_result[iter] = STATUS_FAIL;
				pr_info("[TI-SmartPA:%s]get re fail. Exiting ..\n",
					__func__);
				return;
			} else {
				calib_re[iter] = data;
				pr_info("[TI-SmartPA:%s]calib_re is %02d.%02d (%d)\n",
					__func__,
					(int32_t)trans_val_to_user_i(calib_re[iter],
						QFORMAT19),
					(int32_t)trans_val_to_user_m(calib_re[iter],
						QFORMAT19),
					(int32_t)calib_re[iter]);
				if (tas25xx_check_limits(iter, calib_re[iter]))
					pr_info("[TI-SmartPA:%s] Calibration Pass Channel No:%d\n",
						__func__, iter);
			}

			/*Calibration De-Init*/
			data = 1;//Value is ignored
			param_id = TAS_CALC_PARAM_IDX(TAS_SA_CALIB_DEINIT, 1,
				(iter+1));
			ret = tas25xx_smartamp_algo_ctrl((u8 *)&data, param_id,
							TAS_SET_PARAM, sizeof(uint32_t), TISA_MOD_RX);
			if (ret) {
				pr_err("[TI-SmartPA:%s] De-Init error. Exiting..ret=%d\n",
					__func__, ret);
				return;
			}
			/*Wait some time*/
			msleep(200);
		}
	}
	tas25xx_save_calib_data(calib_re);

	calibration_status = 0;

	tas25xx_prep_dev_for_calib(0);
}

static DEVICE_ATTR(calibration, 0664, calibration_show,
				calibration_store);

static DEVICE_ATTR(cstatus, 0664, calibration_status_show,
				NULL);

static DEVICE_ATTR(rdc, 0664, calibration_rdc_show,
				calibration_rdc_store);

static DEVICE_ATTR(temp, 0664, calibration_amb_temp_show,
				calibration_amb_temp_store);

static DEVICE_ATTR(cstatus_r, 0664, calibration_status_show,
				NULL);

static DEVICE_ATTR(rdc_r, 0664, calibration_rdc_show,
				calibration_rdc_store);

static DEVICE_ATTR(temp_r, 0664, calibration_amb_temp_show,
				calibration_amb_temp_store);

static ssize_t calibration_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int32_t ret = 0;
	int32_t start;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		ret = PTR_ERR(algo);
		goto end;
	}

	ret = kstrtos32(buf, 10, &start);
	if (ret) {
		pr_err("[TI-SmartPA:%s] Invalid input\n", __func__);
		goto end;
	}

	if (start) {
		calibration_status = 1;
		tas25xx_prep_dev_for_calib(1);

		/*Give time for algorithm to converge rdc*/
		schedule_delayed_work(&algo->calib_work,
				msecs_to_jiffies(200));
	}
end:
	return size;
}

static ssize_t calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/*Enough to check for only one channel*/
	return snprintf(buf, MAX_STRING, "%s\n",
			(calibration_status) ? "Enabled" : "Disabled");
}

static ssize_t calibration_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret = 0;

	if (attr == &dev_attr_cstatus_r)
		ret = snprintf(buf, MAX_STRING, "%d", calibration_result[1]);
	else
		ret = snprintf(buf, MAX_STRING, "%d", calibration_result[0]);

	return ret;
}

static ssize_t calibration_rdc_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
	uint32_t calib_re = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		ret = PTR_ERR(algo);
		return ret;
	}

	if (attr == &dev_attr_rdc_r) {
		if (calibration_result[1] == STATUS_NONE) {
			if (algo->calib_update[1])
				calib_re = algo->calib_re[1];
		} else
			calib_re = calib_re_hold[1];

		ret = snprintf(buf, MAX_STRING, "%02d.%02d",
				(int32_t)trans_val_to_user_i(calib_re,
					QFORMAT19),
				(int32_t)trans_val_to_user_m(calib_re,
					QFORMAT19));
	} else {
		if (calibration_result[0] == STATUS_NONE) {
			if (algo->calib_update[0])
				calib_re = algo->calib_re[0];
		} else
			calib_re = calib_re_hold[0];

		ret = snprintf(buf, MAX_STRING, "%02d.%02d",
				(int32_t)trans_val_to_user_i(calib_re,
					QFORMAT19),
				(int32_t)trans_val_to_user_m(calib_re,
					QFORMAT19));
	}

	return ret;
}

static ssize_t calibration_rdc_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	uint32_t data_flt[2];
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();
	int ret = 0;

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		ret = PTR_ERR(algo);
		goto end;
	}

	ret = sscanf(buf, "%d.%d", &(data_flt[0]),
					&(data_flt[1]));
	if (ret != 2) {
		pr_err("[TI-SmartPA:%s] read error\n", __func__);
		goto end;
	}
	if (attr == &dev_attr_rdc_r)
		algo->calib_re[1] = TRANSF_USER_TO_IMPED(data_flt[0], data_flt[1]);
	else
		algo->calib_re[0] = TRANSF_USER_TO_IMPED(data_flt[0], data_flt[1]);
end:
	return size;
}

static ssize_t calibration_amb_temp_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
	uint32_t amb_temp = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		ret = PTR_ERR(algo);
		return ret;
	}

	if (attr == &dev_attr_temp_r) {
		if (calibration_result[1] == STATUS_NONE) {
			if (algo->calib_update[1])
				amb_temp = algo->amb_temp[1];
		} else
			amb_temp = amb_temp_hold[1];

		ret = snprintf(buf, MAX_STRING, "%d", amb_temp);
	} else {
		if (calibration_result[0] == STATUS_NONE) {
			if (algo->calib_update[0])
				amb_temp = algo->amb_temp[0];
		} else
			amb_temp = amb_temp_hold[0];

		ret = snprintf(buf, MAX_STRING, "%d", amb_temp);
	}

	return ret;
}

static ssize_t calibration_amb_temp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	uint32_t data;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();
	int ret = 0;

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		ret = PTR_ERR(algo);
		goto end;
	}

	ret = sscanf(buf, "%d", &data);
	if (ret != 1) {
		pr_err("[TI-SmartPA:%s] read error\n", __func__);
		goto end;
	}
	if (attr == &dev_attr_temp_r) {
		algo->amb_temp[1] = data;
		algo->calib_update[1] = true;
	} else {
		algo->amb_temp[0] = data;
		algo->calib_update[0] = true;
	}
end:
	return size;
}

static struct attribute *tas25xx_calib_attr[] = {
	&dev_attr_calibration.attr,
	&dev_attr_cstatus.attr,
	&dev_attr_rdc.attr,
	&dev_attr_temp.attr,
};

static struct attribute *tas25xx_calib_attr_r[] = {
	&dev_attr_cstatus_r.attr,
	&dev_attr_rdc_r.attr,
	&dev_attr_temp_r.attr,
};

static struct attribute *tas25xx_calib_attr_m[
	ARRAY_SIZE(tas25xx_calib_attr) +
	ARRAY_SIZE(tas25xx_calib_attr_r) + 1] = {NULL};

static struct attribute_group tas25xx_calib_attr_grp = {
		.attrs = tas25xx_calib_attr_m,
};

/********************Calibration Related Codes End**************************/
/********************Validation Related Codes Start**************************/
/* Forward Declarations */
static ssize_t validation_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size);

static ssize_t validation_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t validation_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static void valid_work_routine(struct work_struct *work)
{
	uint8_t iter = 0;
	uint32_t data = 0;
	uint32_t param_id = 0;
	int32_t ret = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! memory not allocated yet!!\n",
			__func__);
		return;
	}

	//Get Validation Status
	for (iter = 0; iter < algo->spk_count; iter++) {
		data = 0;//Reset data to 0
		param_id = TAS_CALC_PARAM_IDX(TAS_SA_GET_VALID_STATUS, 1,
			(iter+1));
		ret = tas25xx_smartamp_algo_ctrl((u8 *)&data, param_id,
			TAS_GET_PARAM, sizeof(uint32_t), TISA_MOD_RX);
		if (ret < 0) {
			validation_result[iter] = STATUS_FAIL;
			pr_info("[TI-SmartPA:%s]status read failed\n",
				__func__);
		} else {
			if (data == VALIDATION_SUCCESS)
				validation_result[iter] = STATUS_SUCCESS;
			else
				validation_result[iter] = STATUS_FAIL;
		}
		pr_info("[TI-SmartPA:%s] Channel-%d\n", __func__, iter);
		pr_info("[TI-SmartPA:%s] validation_result %s(0x%x)\n",
			__func__,
			validation_result[iter] ==
				STATUS_SUCCESS ? "Success":"Fail\n",
					(int32_t)data);

		/*De-Init the validation process*/
		data = 0;//Value is ignored
		param_id = TAS_CALC_PARAM_IDX(TAS_SA_VALID_DEINIT, 1, (iter+1));
		ret = tas25xx_smartamp_algo_ctrl((u8 *)&data, param_id,
			TAS_SET_PARAM, sizeof(uint32_t), TISA_MOD_RX);
		if (ret < 0) {
			pr_err("[TI-SmartPA:%s] validation de-init failed,ret=%d\n",
				__func__, ret);
		}
	}
	validation_running_status = 0;
}

static DEVICE_ATTR(validation, 0664, validation_show,
				validation_store);

static DEVICE_ATTR(status, 0664, validation_status_show,
				NULL);

static DEVICE_ATTR(status_r, 0664, validation_status_show,
				NULL);

static ssize_t validation_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	uint8_t iter = 0;
	uint32_t data = 0;
	uint32_t param_id = 0;
	int32_t start = 0;
	int32_t ret = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! Error memory allocation\n",
			__func__);
		ret = PTR_ERR(algo);
		goto end;
	}

	ret = kstrtos32(buf, 10, &start);
	if (ret) {
		pr_err("[TI-SmartPA:%s] Invalid input\n", __func__);
		goto end;
	}

	if (start) {
		//Init
		for (iter = 0; iter < algo->spk_count; iter++) {
			data = 1;/*Value is ignored*/
			param_id = TAS_CALC_PARAM_IDX(TAS_SA_VALID_INIT, 1,
				(iter+1));

			ret = tas25xx_smartamp_algo_ctrl((u8 *)&data, param_id
				, TAS_SET_PARAM, sizeof(uint32_t), TISA_MOD_RX);
			if (ret < 0) {
				pr_err("[TI-SmartPA:%s] Init error. Exiting ..",
					__func__);
				goto end;
			}
		}

		//Give time for algorithm to converge V-sns level
		validation_running_status = 1;
		schedule_delayed_work(&algo->valid_work,
				msecs_to_jiffies(VALIDATION_TIME * 1000));
	}
end:
	return size;
}

static ssize_t validation_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return snprintf(buf, MAX_STRING, "%s\n",
			(validation_running_status) ? "Enabled" : "Disabled");
}

static ssize_t validation_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;

	if (attr == &dev_attr_status_r)
		ret = snprintf(buf, MAX_STRING, "%d", validation_result[1]);
	else
		ret = snprintf(buf, MAX_STRING, "%d", validation_result[0]);

	return ret;
}

static struct attribute *tas25xx_valid_attr[] = {
	&dev_attr_validation.attr,
	&dev_attr_status.attr,
};

static struct attribute *tas25xx_valid_attr_r[] = {
	&dev_attr_status_r.attr,
};

static struct attribute *tas25xx_valid_attr_m[
	ARRAY_SIZE(tas25xx_valid_attr) +
	ARRAY_SIZE(tas25xx_valid_attr_r) + 1] = {NULL};

static struct attribute_group tas25xx_valid_attr_grp = {
		.attrs = tas25xx_valid_attr_m,
};
/*******************Validation Related Codes End**************************/
/*******************BigData Related Codes Start**************************/
/* Forward Declarations */
static ssize_t tas25xx_bd_exc_max_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t tas25xx_bd_exc_max_persist_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t tas25xx_bd_exc_over_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t tas25xx_bd_temp_max_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t tas25xx_bd_temp_max_persist_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t tas25xx_bd_temp_over_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t tas25xx_le_flag_deteced_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t tas25xx_le_flag_count_persist_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t tas25xx_le_flag_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static DEVICE_ATTR(exc_max, 0664, tas25xx_bd_exc_max_show,
				NULL);

static DEVICE_ATTR(exc_max_persist, 0664, tas25xx_bd_exc_max_persist_show,
				NULL);

static DEVICE_ATTR(exc_over_count, 0664, tas25xx_bd_exc_over_count_show,
				NULL);

static DEVICE_ATTR(temp_max, 0664, tas25xx_bd_temp_max_show,
				NULL);

static DEVICE_ATTR(temp_max_persist, 0664, tas25xx_bd_temp_max_persist_show,
				NULL);

static DEVICE_ATTR(temp_over_count, 0664, tas25xx_bd_temp_over_count_show,
				NULL);

static DEVICE_ATTR(exc_max_r, 0664, tas25xx_bd_exc_max_show,
				NULL);

static DEVICE_ATTR(exc_max_persist_r, 0664, tas25xx_bd_exc_max_persist_show,
				NULL);

static DEVICE_ATTR(exc_over_count_r, 0664, tas25xx_bd_exc_over_count_show,
				NULL);

static DEVICE_ATTR(temp_max_r, 0664, tas25xx_bd_temp_max_show,
				NULL);

static DEVICE_ATTR(temp_max_persist_r, 0664, tas25xx_bd_temp_max_persist_show,
				NULL);

static DEVICE_ATTR(temp_over_count_r, 0664, tas25xx_bd_temp_over_count_show,
				NULL);

static DEVICE_ATTR(le_flag_detected, 0664, tas25xx_le_flag_deteced_show,
				NULL);

static DEVICE_ATTR(le_flag_count_persist, 0664, tas25xx_le_flag_count_persist_show,
				NULL);

static DEVICE_ATTR(le_flag_count, 0664, tas25xx_le_flag_count_show,
				NULL);

static DEVICE_ATTR(le_flag_detected_r, 0664, tas25xx_le_flag_deteced_show,
				NULL);

static DEVICE_ATTR(le_flag_count_persist_r, 0664, tas25xx_le_flag_count_persist_show,
				NULL);

static DEVICE_ATTR(le_flag_count_r, 0664, tas25xx_le_flag_count_show,
				NULL);

static ssize_t tas25xx_le_flag_deteced_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! Error memory allocation\n",
			__func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_le_flag_detected_r) {
		ret = snprintf(buf, MAX_STRING, "%d",
				algo->b_le_flag[1].le_flag_detected);
	} else {
		ret = snprintf(buf, MAX_STRING, "%d",
				algo->b_le_flag[0].le_flag_detected);
	}

	return ret;
}

static ssize_t tas25xx_le_flag_count_persist_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_le_flag_count_persist_r) {
		ret = snprintf(buf, MAX_STRING, "%llu",
			algo->b_le_flag[1].le_flag_count_persist);
	} else {
		ret = snprintf(buf, MAX_STRING, "%llu",
			algo->b_le_flag[0].le_flag_count_persist);
	}

	return ret;
}

static ssize_t tas25xx_le_flag_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();
	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_le_flag_count_r) {
		ret = snprintf(buf, MAX_STRING, "%lu",
				(unsigned long)algo->b_le_flag[1].le_flag_count);
	} else {
		ret = snprintf(buf, MAX_STRING, "%lu",
				(unsigned long)algo->b_le_flag[0].le_flag_count);
	}

	return ret;
}


static ssize_t tas25xx_bd_exc_max_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] failed !! Error memory allocation\n",
			__func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_exc_max_r) {
		ret = snprintf(buf, MAX_STRING, "%02d.%02d",
			(int32_t)trans_val_to_user_i(algo->b_data[1].exc_max,
				QFORMAT31),
			(int32_t)trans_val_to_user_m(algo->b_data[1].exc_max,
				QFORMAT31));
		algo->b_data[1].exc_max = 0;
	} else {
		ret = snprintf(buf, MAX_STRING, "%02d.%02d",
			(int32_t)trans_val_to_user_i(algo->b_data[0].exc_max,
				QFORMAT31),
			(int32_t)trans_val_to_user_m(algo->b_data[0].exc_max,
				QFORMAT31));
		algo->b_data[0].exc_max = 0;
	}

	return ret;
}

static ssize_t tas25xx_bd_exc_max_persist_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_exc_max_persist_r) {
		ret = snprintf(buf, MAX_STRING, "%02d.%02d",
			(int32_t)trans_val_to_user_i(
				algo->b_data[1].exc_max_persist, QFORMAT31),
			(int32_t)trans_val_to_user_m(
				algo->b_data[1].exc_max_persist, QFORMAT31));
	} else {
		ret = snprintf(buf, MAX_STRING, "%02d.%02d",
			(int32_t)trans_val_to_user_i(
				algo->b_data[0].exc_max_persist, QFORMAT31),
			(int32_t)trans_val_to_user_m(
				algo->b_data[0].exc_max_persist, QFORMAT31));
	}

	return ret;
}

static ssize_t tas25xx_bd_exc_over_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
		struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();
	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_exc_over_count_r) {
		ret = snprintf(buf, MAX_STRING, "%d",
			algo->b_data[1].exc_over_count);
	} else {
		ret = snprintf(buf, MAX_STRING, "%d",
			algo->b_data[0].exc_over_count);
	}

	return ret;
}

static ssize_t tas25xx_bd_temp_max_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
		struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();
	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_temp_max_r) {
		ret = snprintf(buf, MAX_STRING, "%d", algo->b_data[1].temp_max);
		algo->b_data[1].temp_max = 0;
	} else {
		ret = snprintf(buf, MAX_STRING, "%d", algo->b_data[0].temp_max);
		algo->b_data[0].temp_max = 0;
	}

	return ret;
}

static ssize_t tas25xx_bd_temp_max_persist_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
		struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();
	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_temp_max_persist_r) {
		ret = snprintf(
			buf, MAX_STRING,
			"%d", algo->b_data[1].temp_max_persist);
	} else {
		ret = snprintf(buf, MAX_STRING,
			"%d", algo->b_data[0].temp_max_persist);
	}

	return ret;
}

static ssize_t tas25xx_bd_temp_over_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t ret;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		return PTR_ERR(algo);
	}

	if (attr == &dev_attr_temp_over_count_r) {
		ret = snprintf(buf, MAX_STRING,
			"%d", algo->b_data[1].temp_over_count);
	} else {
		ret = snprintf(buf, MAX_STRING,
			"%d", algo->b_data[0].temp_over_count);
	}

	return ret;
}

static struct attribute *tas25xx_bd_attr[] = {
	&dev_attr_exc_max.attr,
	&dev_attr_exc_max_persist.attr,
	&dev_attr_exc_over_count.attr,
	&dev_attr_temp_max.attr,
	&dev_attr_temp_max_persist.attr,
	&dev_attr_temp_over_count.attr,
	&dev_attr_le_flag_detected.attr,
	&dev_attr_le_flag_count_persist.attr,
	&dev_attr_le_flag_count.attr,
};

static struct attribute *tas25xx_bd_attr_r[] = {
	&dev_attr_exc_max_r.attr,
	&dev_attr_exc_max_persist_r.attr,
	&dev_attr_exc_over_count_r.attr,
	&dev_attr_temp_max_r.attr,
	&dev_attr_temp_max_persist_r.attr,
	&dev_attr_temp_over_count_r.attr,
	&dev_attr_le_flag_detected_r.attr,
	&dev_attr_le_flag_count_persist_r.attr,
	&dev_attr_le_flag_count_r.attr,
};

static struct attribute *tas25xx_bd_attr_m[
	ARRAY_SIZE(tas25xx_bd_attr) +
	ARRAY_SIZE(tas25xx_bd_attr_r) + 1] = {NULL};

static struct attribute_group tas25xx_bd_attr_grp = {
	.attrs = tas25xx_bd_attr_m,
};

/********************BigData Related Codes End**************************/

static void clean_up_tas_sysfs(void)
{
	if (p_tas25xx_algo) {
		struct tas25xx_algo *algo = p_tas25xx_algo;

		if (algo->calib_dev) {
			sysfs_remove_group(&algo->calib_dev->kobj,
				&tas25xx_calib_attr_grp);
			device_destroy(algo->algo_class, 1);
		}

		if (algo->valid_dev) {
			sysfs_remove_group(&algo->valid_dev->kobj,
				&tas25xx_valid_attr_grp);
			device_destroy(algo->algo_class, 1);
		}

		if (algo->bd_dev) {
			sysfs_remove_group(&algo->bd_dev->kobj,
				&tas25xx_bd_attr_grp);
			device_destroy(algo->algo_class, 1);
		}

		if (algo->algo_class)
			class_destroy(algo->algo_class);

		kfree(p_tas25xx_algo);
		p_tas25xx_algo = NULL;
	}
}

void tas25xx_algo_add_calib_valid_bigdata(uint8_t channels)
{
	int32_t ret = 0;
	struct tas25xx_algo *algo = smartamp_get_sysfs_ptr();

	if (IS_ERR(algo))  {
		pr_err("[TI-SmartPA:%s] Error memory allocation\n", __func__);
		ret = PTR_ERR(algo);
		goto err_dev;
	}

	pr_info("[TI-SmartPA:%s] Adding Smartamp algo functions, spk_count=%d\n",
			__func__, channels);
	algo->spk_count = channels;

	memcpy(tas25xx_calib_attr_m, tas25xx_calib_attr,
		sizeof(tas25xx_calib_attr));
	memcpy(tas25xx_valid_attr_m, tas25xx_valid_attr,
		sizeof(tas25xx_valid_attr));
	memcpy(tas25xx_bd_attr_m, tas25xx_bd_attr, sizeof(tas25xx_bd_attr));
	if (channels == 2) {
		memcpy(tas25xx_calib_attr_m + ARRAY_SIZE(tas25xx_calib_attr),
			tas25xx_calib_attr_r, sizeof(tas25xx_calib_attr_r));
		memcpy(tas25xx_valid_attr_m + ARRAY_SIZE(tas25xx_valid_attr),
			tas25xx_valid_attr_r, sizeof(tas25xx_valid_attr_r));
		memcpy(tas25xx_bd_attr_m + ARRAY_SIZE(tas25xx_bd_attr),
			tas25xx_bd_attr_r, sizeof(tas25xx_bd_attr_r));
	}

	algo->algo_class = class_create(TAS25XX_SYSFS_CLASS_NAME);
	if (IS_ERR(algo->algo_class)) {
		ret = PTR_ERR(algo->algo_class);
		pr_err("[TI-SmartPA:%s] err class create\n", __func__);
		algo->algo_class = NULL;
		goto err_dev;
	}

	algo->calib_dev = device_create(algo->algo_class,
				NULL, 1, NULL, TAS25XX_CALIB_DIR_NAME);
	if (IS_ERR(algo->calib_dev)) {
		pr_err("[TI-SmartPA:%s]Failed to create calib_dev\n", __func__);
		ret = PTR_ERR(algo->calib_dev);
		algo->calib_dev = NULL;
		goto err_dev;
	}

	ret = sysfs_create_group(&algo->calib_dev->kobj,
		&tas25xx_calib_attr_grp);
	if (ret) {
		pr_err("[TI-SmartPA:%s]Failed to create sysfs group\n",
			__func__);
		goto err_dev;
	}

	INIT_DELAYED_WORK(&algo->calib_work, calib_work_routine);

	algo->valid_dev = device_create(algo->algo_class, NULL, 1, NULL,
			TAS25XX_VALID_DIR_NAME);
	if (IS_ERR(algo->valid_dev)) {
		pr_err("[TI-SmartPA:%s]Failed to create valid_dev\n", __func__);
		ret = PTR_ERR(algo->valid_dev);
		algo->valid_dev = NULL;
		goto err_dev;
	}

	ret = sysfs_create_group(&algo->valid_dev->kobj,
			&tas25xx_valid_attr_grp);
	if (ret) {
		pr_err("[TI-SmartPA:%s]Failed to create sysfs group\n",
			__func__);
		algo->valid_dev = NULL;
		goto err_dev;
	}

	INIT_DELAYED_WORK(&algo->valid_work, valid_work_routine);

	algo->bd_dev = device_create(algo->algo_class, NULL, 1, NULL,
			TAS25XX_BD_DIR_NAME);
	if (IS_ERR(algo->bd_dev)) {
		pr_err("[TI-SmartPA:%s]Failed to create bd_dev\n", __func__);
		ret = PTR_ERR(algo->bd_dev);
		algo->bd_dev = NULL;
		goto err_dev;
	}

	ret = sysfs_create_group(&algo->bd_dev->kobj, &tas25xx_bd_attr_grp);
	if (ret) {
		pr_err("[TI-SmartPA:%s]Failed to create sysfs group\n",
			__func__);
		algo->bd_dev = NULL;
		goto err_dev;
	}

	pr_info("[TI-SmartPA:%s] ret=%d\n", __func__, ret);
	return;

err_dev:
	clean_up_tas_sysfs();
	pr_err("[TI-SmartPA:%s] Error %d\n", __func__, ret);
}

void tas25xx_algo_remove_calib_valid_bigdata(void)
{
	pr_info("[TI-SmartPA:%s] Removing Smartamp Algorithm functions\n",
		__func__);
	clean_up_tas_sysfs();
}

void tas_set_algo_run_status(int enable)
{
	s_algo_running = enable;
}

int tas_get_coil_temp_nocheck(int id)
{
	int32_t value = 0;
	int32_t ret = 0;
	uint32_t param_id = 0;

	if (calibration_status)
		ret = -EAGAIN;

	if (!s_algo_running)
		ret = -ENOTCONN;

	if (ret)
		goto get_temp_done;

	param_id = TAS_CALC_PARAM_IDX(TAS_SA_GET_TV, 1, (id+1));
	ret = tas25xx_smartamp_algo_ctrl((u8 *)&value, param_id, TAS_GET_PARAM,
			sizeof(uint32_t), TISA_MOD_RX);
	if (ret < 0) {
		pr_err("TI-SmartPA:%s Error %d reading coil temp for ch=%d",
			__func__, ret, id);
	} else {
		ret = value >> 19;
		pr_info("TI-SmartPA:%s ch=%d, coil temp=%d(%d)", __func__,
			id, ret, value);
	}

get_temp_done:
	return ret;
}

int tas_set_surface_temp_nocheck(int id, int temperature)
{
	int32_t ret = 0;
	uint32_t param_id = 0;

	if (calibration_status)
		ret = -EAGAIN;

	if (!s_algo_running)
		ret = -ENOTCONN;

	if (ret)
		goto set_temp_done;

	pr_info("TI-SmartPA: %s ch=%d, temp=%d",
		__func__, id, temperature);

	param_id = TAS_CALC_PARAM_IDX(TAS_SA_SET_SKIN_TEMP, 1, (id+1));
	ret = tas25xx_smartamp_algo_ctrl((u8 *)&temperature, param_id,
		 TAS_SET_PARAM, sizeof(uint32_t), TISA_MOD_RX);
	if (ret < 0)
		pr_err("TI-SmartPA: %s Error setting surface temp %d", __func__, ret);

set_temp_done:
	return ret;
}

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS25XX Algorithm");
MODULE_LICENSE("GPL v2");
