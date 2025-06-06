/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is mipi channel definition
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_VENDOR_MIPI_H
#define IS_VENDOR_MIPI_H

#include "is-device-sensor.h"

#define CAM_RAT_BAND(rat, band) ((rat<<16)|(band & 0xffff))
#define CAM_GET_RAT(rat_band) ((rat_band & 0xffff0000)>>16)
#define CAM_GET_BAND(rat_band) (0xffff & rat_band)

#define CAM_MIPI_NOT_INITIALIZED	-1
#define CAM_MIPI_MAX_BAND 16
#define CAM_MIPI_MAX_FREQ 4
#define CAM_MIPI_PCC_WEIGHT 10

#define DEFINE_BAND_INFO(_rat, _band, _channel, _conn_status, _bandwidth, _sinr, _rsrp, _rsrq, _cqi, _dl_mcs, _pusch_power) { \
	.rat = _rat, \
	.band = _band, \
	.channel = _channel, \
	.connection_status = _conn_status, \
	.bandwidth = _bandwidth, \
	.sinr = _sinr, \
	.rsrp = _rsrp, \
	.rsrq = _rsrq, \
	.cqi = _cqi, \
	.dl_mcs = _dl_mcs, \
	.pusch_power = _pusch_power }

#define DEFINE_TEST_BAND_INFO(_rat, _band, _channel, _conn_status, _bandwidth, _sinr) { \
	.rat = _rat, \
	.band = _band, \
	.channel = _channel, \
	.connection_status = _conn_status, \
	.bandwidth = _bandwidth, \
	.sinr = _sinr, \
	.rsrp = 0, \
	.rsrq = 0, \
	.cqi = 0, \
	.dl_mcs = 0, \
	.pusch_power = 0 }


struct cam_mipi_setting {
	const char *str_mipi_clk;
	const u32 mipi_rate; /* it's not internal mipi clock */
	const u32 *setting;
	const u32 setting_size;
};

struct cam_mipi_cell_ratings {
	u32 rat_band;
	u32 channel_min;
	u32 channel_max;
	u8 freq_ratings[CAM_MIPI_MAX_FREQ];
};

struct cam_mipi_sensor_mode {
	const u32 mode;
	const struct cam_mipi_cell_ratings *mipi_cell_ratings;
	const u32 mipi_cell_ratings_size;
	const struct cam_mipi_setting *sensor_setting;
	const u32 sensor_setting_size;
	const char *name;
};

struct cam_cp_ril_bridge_msg {
	unsigned int dev_id;
	unsigned int data_len;
	void *data;
};

struct __packed cam_cp_cell_info {
	u8 rat;
	u32 band;
	u32 channel;
	u8 connection_status;
	u32 bandwidth;
	int32_t sinr;
	//optional
	int32_t rsrp;
	int32_t rsrq;
	u8 cqi;
	u8 dl_mcs;
	int32_t pusch_power;
};

struct __packed cam_cp_noti_cell_infos {
	u32 num_cell;
	struct cam_cp_cell_info cell_list[CAM_MIPI_MAX_BAND];
};

/* RAT */
enum {
	CAM_RAT_1_GSM = 1,
	CAM_RAT_2_WCDMA = 2,
	CAM_RAT_3_LTE = 3,
	CAM_RAT_4_TDSCDMA = 4,
	CAM_RAT_5_CDMA = 5,
	CAM_RAT_6_WIFI = 6,
	CAM_RAT_7_NR5G = 7,
};

/* BAND */
enum {
	CAM_BAND_001_GSM_GSM850 = 1,
	CAM_BAND_002_GSM_EGSM900 = 2,
	CAM_BAND_003_GSM_DCS1800 = 3,
	CAM_BAND_004_GSM_PCS1900 = 4,

	CAM_BAND_011_WCDMA_WB01 = 11,
	CAM_BAND_012_WCDMA_WB02 = 12,
	CAM_BAND_013_WCDMA_WB03 = 13,
	CAM_BAND_014_WCDMA_WB04 = 14,
	CAM_BAND_015_WCDMA_WB05 = 15,
	CAM_BAND_016_WCDMA_WB06 = 16,
	CAM_BAND_017_WCDMA_WB07 = 17,
	CAM_BAND_018_WCDMA_WB08 = 18,
	CAM_BAND_019_WCDMA_WB09 = 19,
	CAM_BAND_020_WCDMA_WB10 = 20,
	CAM_BAND_021_WCDMA_WB11 = 21,
	CAM_BAND_022_WCDMA_WB12 = 22,
	CAM_BAND_023_WCDMA_WB13 = 23,
	CAM_BAND_024_WCDMA_WB14 = 24,
	CAM_BAND_025_WCDMA_WB15 = 25,
	CAM_BAND_026_WCDMA_WB16 = 26,
	CAM_BAND_027_WCDMA_WB17 = 27,
	CAM_BAND_028_WCDMA_WB18 = 28,
	CAM_BAND_029_WCDMA_WB19 = 29,
	CAM_BAND_030_WCDMA_WB20 = 30,
	CAM_BAND_031_WCDMA_WB21 = 31,
	CAM_BAND_032_WCDMA_WB22 = 32,
	CAM_BAND_033_WCDMA_WB23 = 33,
	CAM_BAND_034_WCDMA_WB24 = 34,
	CAM_BAND_035_WCDMA_WB25 = 35,
	CAM_BAND_036_WCDMA_WB26 = 36,
	CAM_BAND_037_WCDMA_WB27 = 37,
	CAM_BAND_038_WCDMA_WB28 = 38,
	CAM_BAND_039_WCDMA_WB29 = 39,
	CAM_BAND_040_WCDMA_WB30 = 40,
	CAM_BAND_041_WCDMA_WB31 = 41,
	CAM_BAND_042_WCDMA_WB32 = 42,

	CAM_BAND_051_TDSCDMA_TD1 = 51,
	CAM_BAND_052_TDSCDMA_TD2 = 52,
	CAM_BAND_053_TDSCDMA_TD3 = 53,
	CAM_BAND_054_TDSCDMA_TD4 = 54,
	CAM_BAND_055_TDSCDMA_TD5 = 55,
	CAM_BAND_056_TDSCDMA_TD6 = 56,

	CAM_BAND_061_CDMA_BC0 = 61,
	CAM_BAND_062_CDMA_BC1 = 62,
	CAM_BAND_063_CDMA_BC2 = 63,
	CAM_BAND_064_CDMA_BC3 = 64,
	CAM_BAND_065_CDMA_BC4 = 65,
	CAM_BAND_066_CDMA_BC5 = 66,
	CAM_BAND_067_CDMA_BC6 = 67,
	CAM_BAND_068_CDMA_BC7 = 68,
	CAM_BAND_069_CDMA_BC8 = 69,
	CAM_BAND_070_CDMA_BC9 = 70,
	CAM_BAND_071_CDMA_BC10 = 71,
	CAM_BAND_072_CDMA_BC11 = 72,
	CAM_BAND_073_CDMA_BC12 = 73,
	CAM_BAND_074_CDMA_BC13 = 74,
	CAM_BAND_075_CDMA_BC14 = 75,
	CAM_BAND_076_CDMA_BC15 = 76,
	CAM_BAND_077_CDMA_BC16 = 77,
	CAM_BAND_078_CDMA_BC17 = 78,
	CAM_BAND_079_CDMA_BC18 = 79,
	CAM_BAND_080_CDMA_BC19 = 80,
	CAM_BAND_081_CDMA_BC20 = 81,
	CAM_BAND_082_CDMA_BC21 = 82,

	CAM_BAND_091_LTE_LB01 = 91,
	CAM_BAND_092_LTE_LB02 = 92,
	CAM_BAND_093_LTE_LB03 = 93,
	CAM_BAND_094_LTE_LB04 = 94,
	CAM_BAND_095_LTE_LB05 = 95,
	CAM_BAND_096_LTE_LB06 = 96,
	CAM_BAND_097_LTE_LB07 = 97,
	CAM_BAND_098_LTE_LB08 = 98,
	CAM_BAND_099_LTE_LB09 = 99,
	CAM_BAND_100_LTE_LB10 = 100,
	CAM_BAND_101_LTE_LB11 = 101,
	CAM_BAND_102_LTE_LB12 = 102,
	CAM_BAND_103_LTE_LB13 = 103,
	CAM_BAND_104_LTE_LB14 = 104,
	CAM_BAND_105_LTE_LB15 = 105,
	CAM_BAND_106_LTE_LB16 = 106,
	CAM_BAND_107_LTE_LB17 = 107,
	CAM_BAND_108_LTE_LB18 = 108,
	CAM_BAND_109_LTE_LB19 = 109,
	CAM_BAND_110_LTE_LB20 = 110,
	CAM_BAND_111_LTE_LB21 = 111,
	CAM_BAND_112_LTE_LB22 = 112,
	CAM_BAND_113_LTE_LB23 = 113,
	CAM_BAND_114_LTE_LB24 = 114,
	CAM_BAND_115_LTE_LB25 = 115,
	CAM_BAND_116_LTE_LB26 = 116,
	CAM_BAND_117_LTE_LB27 = 117,
	CAM_BAND_118_LTE_LB28 = 118,
	CAM_BAND_119_LTE_LB29 = 119,
	CAM_BAND_120_LTE_LB30 = 120,
	CAM_BAND_121_LTE_LB31 = 121,
	CAM_BAND_122_LTE_LB32 = 122,
	CAM_BAND_123_LTE_LB33 = 123,
	CAM_BAND_124_LTE_LB34 = 124,
	CAM_BAND_125_LTE_LB35 = 125,
	CAM_BAND_126_LTE_LB36 = 126,
	CAM_BAND_127_LTE_LB37 = 127,
	CAM_BAND_128_LTE_LB38 = 128,
	CAM_BAND_129_LTE_LB39 = 129,
	CAM_BAND_130_LTE_LB40 = 130,
	CAM_BAND_131_LTE_LB41 = 131,
	CAM_BAND_132_LTE_LB42 = 132,
	CAM_BAND_133_LTE_LB43 = 133,
	CAM_BAND_134_LTE_LB44 = 134,
	CAM_BAND_135_LTE_LB45 = 135,
	CAM_BAND_136_LTE_LB46 = 136,
	CAM_BAND_137_LTE_LB47 = 137,
	CAM_BAND_138_LTE_LB48 = 138,
	CAM_BAND_139_LTE_LB49 = 139,
	CAM_BAND_140_LTE_LB50 = 140,
	CAM_BAND_141_LTE_LB51 = 141,
	CAM_BAND_142_LTE_LB52 = 142,
	CAM_BAND_143_LTE_LB53 = 143,
	CAM_BAND_144_LTE_LB54 = 144,
	CAM_BAND_145_LTE_LB55 = 145,
	CAM_BAND_146_LTE_LB56 = 146,
	CAM_BAND_147_LTE_LB57 = 147,
	CAM_BAND_148_LTE_LB58 = 148,
	CAM_BAND_149_LTE_LB59 = 149,
	CAM_BAND_150_LTE_LB60 = 150,
	CAM_BAND_151_LTE_LB61 = 151,
	CAM_BAND_152_LTE_LB62 = 152,
	CAM_BAND_153_LTE_LB63 = 153,
	CAM_BAND_154_LTE_LB64 = 154,
	CAM_BAND_155_LTE_LB65 = 155,
	CAM_BAND_156_LTE_LB66 = 156,
	CAM_BAND_157_LTE_LB67 = 157,
	CAM_BAND_158_LTE_LB68 = 158,
	CAM_BAND_159_LTE_LB69 = 159,
	CAM_BAND_160_LTE_LB70 = 160,
	CAM_BAND_161_LTE_LB71 = 161,

	CAM_BAND_255_NR5G_N000 = 255, /* Default Value, Not available */

	CAM_BAND_256_NR5G_N001 = 256,
	CAM_BAND_257_NR5G_N002 = 257,
	CAM_BAND_258_NR5G_N003 = 258,
	CAM_BAND_259_NR5G_N004 = 259,
	CAM_BAND_260_NR5G_N005 = 260,
	CAM_BAND_261_NR5G_N006 = 261,
	CAM_BAND_262_NR5G_N007 = 262,
	CAM_BAND_263_NR5G_N008 = 263,
	CAM_BAND_264_NR5G_N009 = 264,
	CAM_BAND_265_NR5G_N010 = 265,
	CAM_BAND_266_NR5G_N011 = 266,
	CAM_BAND_267_NR5G_N012 = 267,
	CAM_BAND_268_NR5G_N013 = 268,
	CAM_BAND_269_NR5G_N014 = 269,
	CAM_BAND_270_NR5G_N015 = 270,
	CAM_BAND_271_NR5G_N016 = 271,
	CAM_BAND_272_NR5G_N017 = 272,
	CAM_BAND_273_NR5G_N018 = 273,
	CAM_BAND_274_NR5G_N019 = 274,
	CAM_BAND_275_NR5G_N020 = 275,
	CAM_BAND_276_NR5G_N021 = 276,
	CAM_BAND_277_NR5G_N022 = 277,
	CAM_BAND_278_NR5G_N023 = 278,
	CAM_BAND_279_NR5G_N024 = 279,
	CAM_BAND_280_NR5G_N025 = 280,
	CAM_BAND_281_NR5G_N026 = 281,
	CAM_BAND_282_NR5G_N027 = 282,
	CAM_BAND_283_NR5G_N028 = 283,
	CAM_BAND_284_NR5G_N029 = 284,
	CAM_BAND_285_NR5G_N030 = 285,
	CAM_BAND_286_NR5G_N031 = 286,
	CAM_BAND_287_NR5G_N032 = 287,
	CAM_BAND_288_NR5G_N033 = 288,
	CAM_BAND_289_NR5G_N034 = 289,
	CAM_BAND_290_NR5G_N035 = 290,
	CAM_BAND_291_NR5G_N036 = 291,
	CAM_BAND_292_NR5G_N037 = 292,
	CAM_BAND_293_NR5G_N038 = 293,
	CAM_BAND_294_NR5G_N039 = 294,
	CAM_BAND_295_NR5G_N040 = 295,
	CAM_BAND_296_NR5G_N041 = 296,
	CAM_BAND_297_NR5G_N042 = 297,
	CAM_BAND_298_NR5G_N043 = 298,
	CAM_BAND_299_NR5G_N044 = 299,
	CAM_BAND_300_NR5G_N045 = 300,
	CAM_BAND_301_NR5G_N046 = 301,
	CAM_BAND_302_NR5G_N047 = 302,
	CAM_BAND_303_NR5G_N048 = 303,
	CAM_BAND_304_NR5G_N049 = 304,
	CAM_BAND_305_NR5G_N050 = 305,
	CAM_BAND_306_NR5G_N051 = 306,
	CAM_BAND_307_NR5G_N052 = 307,
	CAM_BAND_308_NR5G_N053 = 308,
	CAM_BAND_309_NR5G_N054 = 309,
	CAM_BAND_310_NR5G_N055 = 310,
	CAM_BAND_311_NR5G_N056 = 311,
	CAM_BAND_312_NR5G_N057 = 312,
	CAM_BAND_313_NR5G_N058 = 313,
	CAM_BAND_314_NR5G_N059 = 314,
	CAM_BAND_315_NR5G_N060 = 315,
	CAM_BAND_316_NR5G_N061 = 316,
	CAM_BAND_317_NR5G_N062 = 317,
	CAM_BAND_318_NR5G_N063 = 318,
	CAM_BAND_319_NR5G_N064 = 319,
	CAM_BAND_320_NR5G_N065 = 320,
	CAM_BAND_321_NR5G_N066 = 321,
	CAM_BAND_322_NR5G_N067 = 322,
	CAM_BAND_323_NR5G_N068 = 323,
	CAM_BAND_324_NR5G_N069 = 324,
	CAM_BAND_325_NR5G_N070 = 325,
	CAM_BAND_326_NR5G_N071 = 326,
	CAM_BAND_327_NR5G_N072 = 327,
	CAM_BAND_328_NR5G_N073 = 328,
	CAM_BAND_329_NR5G_N074 = 329,
	CAM_BAND_330_NR5G_N075 = 330,
	CAM_BAND_331_NR5G_N076 = 331,
	CAM_BAND_332_NR5G_N077 = 332,
	CAM_BAND_333_NR5G_N078 = 333,
	CAM_BAND_334_NR5G_N079 = 334,
	CAM_BAND_335_NR5G_N080 = 335,
	CAM_BAND_336_NR5G_N081 = 336,
	CAM_BAND_337_NR5G_N082 = 337,
	CAM_BAND_338_NR5G_N083 = 338,
	CAM_BAND_339_NR5G_N084 = 339,
	CAM_BAND_340_NR5G_N085 = 340,
	CAM_BAND_341_NR5G_N086 = 341,
	CAM_BAND_342_NR5G_N087 = 342,
	CAM_BAND_343_NR5G_N088 = 343,
	CAM_BAND_344_NR5G_N089 = 344,
	CAM_BAND_345_NR5G_N090 = 345,
	CAM_BAND_512_NR5G_N257 = 512,
	CAM_BAND_513_NR5G_N258 = 513,
	CAM_BAND_514_NR5G_N259 = 514,
	CAM_BAND_515_NR5G_N260 = 515,
	CAM_BAND_516_NR5G_N261 = 516,
};

/* cell connection status */
enum {
	CAM_CON_STATUS_NONE = 0,
	CAM_CON_STATUS_PRIMARY_SERVING = 1,
	CAM_CON_STATUS_SECONDARY_SERVING = 2,
};

struct is_cis; /* to prevent cyclic include */

int is_vendor_select_mipi_by_rf_cell_infos(const int position,
										const struct cam_mipi_sensor_mode *mipi_sensor_mode);
int is_vendor_verify_mipi_cell_ratings(const struct cam_mipi_cell_ratings *channel_list, const int size,
										const int freq_size);
void is_vendor_save_rf_cell_infos_on_error(void);
void is_vendor_get_rf_cell_infos_on_error(struct cam_cp_noti_cell_infos *rf_cell_infos);
void is_vendor_register_ril_notifier(void);
int is_vendor_get_mipi_clock_string(struct is_device_sensor *device, char *cur_mipi_str);
int is_vendor_update_mipi_info(struct is_device_sensor *device);
void is_vendor_set_cp_test_cell(bool on);
void is_vendor_set_cp_test_cell_infos(const struct cam_cp_noti_cell_infos *test_cell_infos);
void is_vendor_get_rf_cell_infos(struct cam_cp_noti_cell_infos *cell_infos);

#endif /* IS_VENDOR_MIPI_H */
