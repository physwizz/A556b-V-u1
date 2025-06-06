/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_DEVICE_SENSOR_H
#define IS_DEVICE_SENSOR_H

#include <dt-bindings/camera/exynos_is_dt.h>

#include "exynos-is-sensor.h"
#include "is-resourcemgr.h"
#include "is-groupmgr.h"
#include "is-helper-ixc.h"
#include "pablo-obte.h"

struct is_video_ctx;
struct is_device_ischain;

#define EXPECT_FRAME_START	0
#define EXPECT_FRAME_END	1
#define LOG_INTERVAL_OF_DROPS 30

#define SENSOR_MAX_ENUM			4 /* 4 modules(2 rear, 2 front) for same csis */
#define ACTUATOR_MAX_ENUM		IS_SENSOR_COUNT
#define SENSOR_DEFAULT_FRAMERATE	30

#define SENSOR_MODE_MASK		0xFFFF0000
#define SENSOR_MODE_SHIFT		16
#define SENSOR_MODE_DEINIT		0xFFFF

#define SENSOR_SCENARIO_MASK		0xF0000000
#define SENSOR_SCENARIO_SHIFT		28
#define ASB_SCENARIO_MASK		0xF000
#define ASB_SCENARIO_SHIFT        	12
#define SENSOR_MODULE_MASK		0x00000FFF
#define SENSOR_MODULE_SHIFT		0

#define SENSOR_SSTREAM_MASK		0x0000000F
#define SENSOR_SSTREAM_SHIFT		0
#define SENSOR_USE_STANDBY_MASK		0x000000F0
#define SENSOR_USE_STANDBY_SHIFT	4
#define SENSOR_INSTANT_MASK		0x0FFF0000
#define SENSOR_INSTANT_SHIFT		16
#define SENSOR_NOBLOCK_MASK		0xF0000000
#define SENSOR_NOBLOCK_SHIFT		28

#define SENSOR_I2C_CH_MASK		0xFF
#define SENSOR_I2C_CH_SHIFT		0
#define ACTUATOR_I2C_CH_MASK		0xFF00
#define ACTUATOR_I2C_CH_SHIFT		8
#define OIS_I2C_CH_MASK 		0xFF0000
#define OIS_I2C_CH_SHIFT		16

#define SENSOR_I2C_ADDR_MASK		0xFF
#define SENSOR_I2C_ADDR_SHIFT		0
#define ACTUATOR_I2C_ADDR_MASK		0xFF00
#define ACTUATOR_I2C_ADDR_SHIFT		8
#define OIS_I2C_ADDR_MASK		0xFF0000
#define OIS_I2C_ADDR_SHIFT		16

#define SENSOR_OTP_PAGE			256
#define SENSOR_OTP_PAGE_SIZE		64

#define SENSOR_SIZE_WIDTH_MASK		0xFFFF0000
#define SENSOR_SIZE_WIDTH_SHIFT		16
#define SENSOR_SIZE_HEIGHT_MASK		0xFFFF
#define SENSOR_SIZE_HEIGHT_SHIFT	0

#define IS_TIMESTAMP_HASH_KEY	20

struct sensor_dma_output {
	struct is_fmt fmt;
	u32 cmd;
	u32 width;
	u32 height;
	u32 sbwc_type;
};

enum is_sensor_subdev_ioctl {
	SENSOR_IOCTL_DMA_CANCEL,
	SENSOR_IOCTL_PATTERN_ENABLE,
	SENSOR_IOCTL_PATTERN_DISABLE,
	SENSOR_IOCTL_G_FRAME_ID,
	SENSOR_IOCTL_G_HW_FCOUNT,
	SENSOR_IOCTL_CSI_DMA_ATTACH,
	SENSOR_IOCTL_G_DMA_CH,
	SENSOR_IOCTL_G_VC1_FRAMEPTR,
	SENSOR_IOCTL_G_VC2_FRAMEPTR,
	SENSOR_IOCTL_G_VC3_FRAMEPTR,
	SENSOR_IOCTL_G_VC4_FRAMEPTR,
	SENSOR_IOCTL_G_VC5_FRAMEPTR,
	SENSOR_IOCTL_G_VC6_FRAMEPTR,
	SENSOR_IOCTL_G_VC7_FRAMEPTR,
	SENSOR_IOCTL_S_OTF_OUT,

	SENSOR_IOCTL_CSI_S_CTRL,
	SENSOR_IOCTL_CSI_G_CTRL,
	SENSOR_IOCTL_MOD_S_CTRL,
	SENSOR_IOCTL_MOD_G_CTRL,
	SENSOR_IOCTL_MOD_S_EXT_CTRL,
	SENSOR_IOCTL_MOD_G_EXT_CTRL,
	SENSOR_IOCTL_ACT_S_CTRL,
	SENSOR_IOCTL_ACT_G_CTRL,
	SENSOR_IOCTL_FLS_S_CTRL,
	SENSOR_IOCTL_FLS_G_CTRL,

};

enum is_sensor_smc_state {
        IS_SENSOR_SMC_INIT = 0,
        IS_SENSOR_SMC_CFW_ENABLE,
        IS_SENSOR_SMC_PREPARE,
        IS_SENSOR_SMC_UNPREPARE,
};

enum is_sensor_output_entity {
	IS_SENSOR_OUTPUT_NONE = 0,
	IS_SENSOR_OUTPUT_FRONT,
};

enum is_sensor_force_stop {
	IS_BAD_FRAME_STOP = 0,
	IS_MIF_THROTTLING_STOP = 1,
	IS_FLITE_OVERFLOW_STOP = 2,
};

enum is_module_state {
	IS_MODULE_GPIO_ON,
	IS_MODULE_STANDBY_ON
};

enum is_sensor_state {
	IS_SENSOR_PROBE,
	IS_SENSOR_OPEN,
	IS_SENSOR_MCLK_ON,
	IS_SENSOR_ICLK_ON,
	IS_SENSOR_GPIO_ON,
	IS_SENSOR_S_INPUT,
	IS_SENSOR_ONLY,	/* SOC sensor, Iris sensor, Vision mode without IS chain */
	IS_SENSOR_FRONT_START,
	IS_SENSOR_FRONT_SNR_STOP,
	IS_SENSOR_BACK_START,
	IS_SENSOR_OTF_OUTPUT,
	IS_SENSOR_ITF_REGISTER,	/* to check whether sensor interfaces are registered */
	IS_SENSOR_WAIT_STREAMING,
	SENSOR_MODULE_GOT_INTO_TROUBLE,
	IS_SENSOR_ESD_RECOVERY,
	IS_SENSOR_ASSERT_CRASH,
	IS_SENSOR_S_POWER,
	IS_SENSOR_START,
	IS_SENSOR_SUSPEND,
};

#define IS_SENSOR_AEB_MODE_MASK	0xFF
#define IS_SENSOR_AEB_INVALID_SHORT_OUT_MASK	(0\
						| BIT(IS_SENSOR_SINGLE_MODE)\
						| BIT(IS_SENSOR_AEB_ERR)\
						| BIT(IS_SKIP_CHAIN_SHOT)\
						| BIT(IS_SKIP_SENSOR_SHOT)\
						| BIT(IS_DO_SENSOR_OTF_OUT_CFG)\
						)
enum is_sensor_aeb_state {
	/* Current AEB mode [7:0] */
	IS_SENSOR_SINGLE_MODE,
	IS_SENSOR_2EXP_MODE,
	IS_SENSOR_AEB_SWITCHING,
	IS_SENSOR_AEB_ERR,

	/* AEB mode change ischain shot state [11:8] */
	IS_SKIP_CHAIN_SHOT = 8,
	IS_DO_SHORT_CHAIN_SHOT,
	IS_DO_SHORT_CHAIN_S_PARAM,

	/* AEB mode change sensor shot state [15:12] */
	IS_SKIP_SENSOR_SHOT = 12,
	IS_DO_SENSOR_OTF_OUT_CFG,
};

enum is_sensor_rms_crop_state {
	IS_SENSOR_RMS_CROP_OFF,
	IS_SENSOR_RMS_CROP_ON,
	IS_SENSOR_RMS_CROP_SWITCHING,
};

/*
 * Cal data status
 * [0]: NO ERROR
 * [1]: CRC FAIL
 * [2]: LIMIT FAIL
 * => Check AWB out of the ratio EEPROM/OTP data
 */

enum is_sensor_cal_status {
	CRC_NO_ERROR = 0,
	CRC_ERROR,
	LIMIT_FAILURE,
};

struct is_device_sensor {
	char						*stm;
	u32						instance;	/* logical stream id: decide at open time*/

	char						*sensor_name;
	u32						device_id;	/* physical sensor node id: it is decided at probe time */
	u32						position;

	struct v4l2_device				v4l2_dev;
	struct platform_device				*pdev;

	struct is_image					image;

	struct is_video_ctx				*vctx;
	struct is_video					video;

	struct is_device_ischain   			*ischain;
	struct is_resourcemgr				*resourcemgr;
	struct is_module_enum module_enum[SENSOR_MAX_ENUM];
	struct is_sensor_cfg *cfg[NFI_TOGGLE_MAX];
	bool nfi_toggle;
	struct work_struct nfi_work;

	/* current control value */
	u64						timestamp[IS_TIMESTAMP_HASH_KEY];
	u64						timestampboot[IS_TIMESTAMP_HASH_KEY];
	u64						frame_id[IS_TIMESTAMP_HASH_KEY]; /* index 0 ~ 7 */
	u64						prev_timestampboot;

	u32						fcount;
	u32						line_fcount;
	u32						instant_cnt;
	int						instant_ret;
	wait_queue_head_t				instant_wait;
	struct work_struct				instant_work;
	unsigned long					state;
	spinlock_t					slock_state;
	struct mutex					mlock_state;
	atomic_t					group_open_cnt;
	enum is_sensor_smc_state			smc_state;

	/* hardware configuration */
	struct v4l2_subdev				*subdev_module;
	struct v4l2_subdev				*subdev_csi;

#if !IS_ENABLED(CONFIG_SENSOR_GROUP_LVN)
	/* sensor dma video node */
	struct is_video					video_ssxvc[CSI_VIRTUAL_CH_MAX];

	/* subdev for dma */
	struct is_subdev ssvc[CSIS_MAX_NUM_DMA_ATTACH][DMA_VIRTUAL_CH_MAX];
#endif

	/* for sensor lvn */
	struct sensor_dma_output			dma_output[CSI_VIRTUAL_CH_MAX];

	/* gain boost */
	u32 min_target_fps;
	u32 max_target_fps;
	int						scene_mode;

	/* for vision control */
	int						exposure_time;
	u64						frame_duration;

	/* Sensor No Response */
	bool						snr_check;
	struct timer_list				snr_timer;
	unsigned long					force_stop;

	struct exynos_platform_is_sensor		*pdata;
	atomic_t					module_count;
	struct v4l2_subdev 				*subdev_actuator[ACTUATOR_MAX_ENUM];
	struct is_actuator				*actuator[ACTUATOR_MAX_ENUM];
	struct v4l2_subdev				*subdev_flash;
	struct is_flash					*flash;
	struct v4l2_subdev				*subdev_ois;
	struct is_ois					*ois;
	struct v4l2_subdev				*subdev_mcu;
	struct is_mcu					*mcu;
	struct v4l2_subdev				*subdev_aperture;
	struct is_aperture				*aperture;
	struct v4l2_subdev				*subdev_eeprom;
	struct is_eeprom				*eeprom;
	struct v4l2_subdev				*subdev_laser_af;
	struct is_laser_af				*laser_af;
	void						*private_data;
	struct is_group					group_sensor;

	u32						sensor_width;
	u32						sensor_height;

	int						num_of_ch_mode;
	bool						dma_abstract;
	u32						use_standby;
	u32						sstream;
	u32						ex_mode;
	u32						ex_mode_extra;
	u32						ex_mode_format;
	u32						ex_scenario;

#ifdef ENABLE_MODECHANGE_CAPTURE
	struct is_frame					*mode_chg_frame;
#endif

	bool					use_otp_cal;
	u32					cal_status[CAMERA_CRC_INDEX_MAX];
	u8					otp_cal_buf[SENSOR_OTP_PAGE][SENSOR_OTP_PAGE_SIZE];

	void					*client;
	struct is_ixc_ops			*ixc_ops;
	struct mutex				mutex_reboot;
	bool					reboot;

	bool					use_stripe_flag;

	/* for set timeout */
	int						exposure_value[IS_EXP_BACKUP_COUNT];
	u32						exposure_fcount[IS_EXP_BACKUP_COUNT];
	u32						obte_config;
	struct pablo_obte_sensor			*obte_sensor;

	ulong						aeb_state;
	ulong						rms_crop_state;
	const struct pablo_device_sensor_ops *ops;
};

#define CALL_DEVICE_SENSOR_OPS(sensor, op, args...)                                                \
	(((sensor)->ops && (sensor)->ops->op) ? ((sensor)->ops->op(args)) : 0)

struct pablo_device_sensor_ops {
	int (*group_tag)(struct is_device_sensor *device, struct is_frame *frame,
			 struct camera2_node *ldr_node);
};

struct is_sensor_mode {
	u32 width;
	u32 height;
	u32 framerate;
	u32 ex_mode;
	u32 ex_mode_extra;
	u32 ex_mode_format;
};

int is_sensor_open(struct is_device_sensor *device,
	struct is_video_ctx *vctx);
int is_sensor_close(struct is_device_sensor *device);
int is_sensor_s_input(struct is_device_sensor *device,
	u32 position,
	u32 scenario,
	u32 stream_leader);
int is_sensor_s_ctrl(struct is_device_sensor *device,
	struct v4l2_control *ctrl);
int is_sensor_s_ext_ctrls(struct is_device_sensor *device,
	struct v4l2_ext_controls *ctrls);
int is_sensor_subdev_buffer_queue(struct is_device_sensor *device,
	enum is_subdev_id subdev_id,
	u32 index);
int is_sensor_buffer_queue(struct is_device_sensor *device,
	struct is_queue *queue,
	u32 index);
int is_sensor_buffer_finish(struct is_device_sensor *device,
	u32 index);

int is_sensor_front_start(struct is_device_sensor *device,
	u32 instant_cnt,
	u32 nonblock);
int is_sensor_front_stop(struct is_device_sensor *device, bool fstop);
void is_sensor_group_force_stop(struct is_device_sensor *device, u32 group_id);

int is_sensor_s_framerate(struct is_device_sensor *device,
	struct v4l2_streamparm *param);
int is_sensor_s_bns(struct is_device_sensor *device,
	u32 width, u32 height);

int is_sensor_s_frame_duration(struct is_device_sensor *device,
	u32 frame_duration);
int is_sensor_s_exposure_time(struct is_device_sensor *device,
	u32 exposure_time);
int is_sensor_s_again(struct is_device_sensor *device, u32 gain);
int is_sensor_s_shutterspeed(struct is_device_sensor *device, u32 shutterspeed);

struct is_sensor_cfg * is_sensor_g_mode(struct is_device_sensor *device);
int is_sensor_mclk_on(struct is_device_sensor *device, u32 scenario, u32 channel, u32 freq);
int is_sensor_mclk_off(struct is_device_sensor *device, u32 scenario, u32 channel);
int is_sensor_gpio_on(struct is_device_sensor *device);
int is_sensor_gpio_off(struct is_device_sensor *device);
int is_sensor_gpio_dbg(struct is_device_sensor *device);
void is_sensor_dump(struct is_device_sensor *device);

int is_sensor_g_ctrl(struct is_device_sensor *device,
	struct v4l2_control *ctrl);
int is_sensor_g_ext_ctrls(struct is_device_sensor *device,
	struct v4l2_ext_controls *ctrls);
int is_sensor_g_instance(struct is_device_sensor *device);
int is_sensor_g_ex_mode(struct is_device_sensor *device);
int is_sensor_g_framerate(struct is_device_sensor *device);
int is_sensor_g_fcount(struct is_device_sensor *device);
int is_sensor_g_width(struct is_device_sensor *device);
int is_sensor_g_height(struct is_device_sensor *device);
int is_sensor_g_bns_width(struct is_device_sensor *device);
int is_sensor_g_bns_height(struct is_device_sensor *device);
int is_sensor_g_bns_ratio(struct is_device_sensor *device);
int is_sensor_g_bratio(struct is_device_sensor *device);
int is_sensor_g_updated_bratio(struct is_device_sensor *device, u32 fcount);
u32 is_sensor_g_hdr_mode(struct is_device_sensor *device, u32 fcount, bool is_short);
int is_sensor_g_module(struct is_device_sensor *device,
	struct is_module_enum **module);
struct is_sensor_interface *is_sensor_get_sensor_interface(struct is_device_sensor *device);
int is_sensor_g_position(struct is_device_sensor *device);
int is_search_sensor_module_with_position(struct is_device_sensor *device,
	u32 position, struct is_module_enum **module);
int is_sensor_dm_tag(struct is_device_sensor *device,
	struct is_frame *frame);
int is_sensor_buf_tag(struct is_device_sensor *device,
	struct is_subdev *f_subdev,
	struct v4l2_subdev *v_subdev,
	struct is_frame *ldr_frame);
int is_sensor_buf_skip(struct is_device_sensor *device,
	struct is_subdev *f_subdev,
	struct v4l2_subdev *v_subdev,
	struct is_frame *ldr_frame);
int is_sensor_sbuf_tag(struct is_device_sensor *device,
	struct is_subdev *f_subdev,
	struct v4l2_subdev *v_subdev,
	struct is_frame *ldr_frame,
	int vc);
int is_sensor_g_csis_error(struct is_device_sensor *device);
void is_sensor_s_batch_mode(struct is_device_sensor *device, struct is_frame *frame);
void is_sensor_s_otf_out(struct is_device_sensor *device);
int is_sensor_dma_cancel(struct is_device_sensor *device);
struct is_queue_ops *is_get_sensor_device_qops(void);
void is_sensor_g_max_size(u32 *max_width, u32 *max_height);
bool is_sensor_check_aeb_mode(struct is_device_sensor *device);
bool is_sensor_g_aeb_mode(struct is_device_sensor *sensor);
int is_sensor_get_frame_type(u32 instance);
int is_sensor_get_mono_mode(struct is_sensor_interface *sensor_itf);
int is_sensor_get_capture_slot(struct is_frame *frame, dma_addr_t **taddr, u32 vid);
int is_sensor_prepare_retention(struct is_device_sensor *sensor, int position);
struct platform_driver *is_sensor_get_platform_driver(void);
#define CALL_MOPS(s, op, args...) (((s)->ops) ? (((s)->ops->op) ? ((s)->ops->op(args)) : 0) : 0)

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
struct pablo_kunit_device_sensor_func {
	int (*g_device)(struct platform_device *pdev, struct is_device_sensor **device);
	bool (*cmp_mode)(struct is_sensor_cfg *ss_cfg, struct is_sensor_mode *mode);
	struct is_sensor_cfg *(*g_mode)(struct is_device_sensor *device);
	int (*mclk_on)(struct is_device_sensor *device, u32 scenario, u32 channel, u32 freq);
	int (*mclk_off)(struct is_device_sensor *device, u32 scenario, u32 channel);
	int (*iclk_on)(struct is_device_sensor *device);
	int (*iclk_off)(struct is_device_sensor *device);
	int (*gpio_on)(struct is_device_sensor *device);
	int (*gpio_off)(struct is_device_sensor *device);
	int (*gpio_dbg)(struct is_device_sensor *device);
	bool (*g_aeb_mode)(struct is_device_sensor *sensor);
	int (*get_mono_mode)(struct is_sensor_interface *sensor_itf);
	void (*set_cis_data)(struct is_device_sensor *s, struct is_module_enum *m);
	void (*set_actuator_data)(struct is_device_sensor *s, struct is_module_enum *m);
	void (*set_flash_data)(struct is_device_sensor *s, struct is_module_enum *m);
	void (*set_ois_data)(struct is_device_sensor *s, struct is_module_enum *m);
	void (*set_mcu_data)(struct is_device_sensor *s, struct is_module_enum *m);
	void (*set_eeprom_data)(struct is_device_sensor *s, struct is_module_enum *m);
	void (*set_laser_af_data)(struct is_device_sensor *s, struct is_module_enum *m);
	int (*g_module)(struct is_device_sensor *device, struct is_module_enum **module);
	struct is_sensor_interface *(*get_sensor_interface)(struct is_device_sensor *device);
	void (*g_max_size)(u32 *max_width, u32 *max_height);
	int (*get_frame_type)(u32 instance);
	int (*s_ext_ctrls)(struct is_device_sensor *device, struct v4l2_ext_controls *ctrls);
	int (*g_ext_ctrls)(struct is_device_sensor *device, struct v4l2_ext_controls *ctrls);
	int (*s_frame_duration)(struct is_device_sensor *device, u32 framerate);
	int (*s_exposure_time)(struct is_device_sensor *device, u32 exposure_time);
	int (*s_again)(struct is_device_sensor *device, u32 gain);
	int (*s_shutterspeed)(struct is_device_sensor *device, u32 shutterspeed);
	int (*g_instance)(struct is_device_sensor *device);
	int (*g_position)(struct is_device_sensor *device);
	int (*g_csis_error)(struct is_device_sensor *device);
	void (*dump_status)(struct is_device_sensor *device);
	int (*map_sensor_module)(struct is_device_ischain *device, int position);
	int (*dma_cancel)(struct is_device_sensor *device);
	int (*s_ctrl)(struct is_device_sensor *device, struct v4l2_control *ctrl);
	int (*notify_by_dma_end)(struct is_device_sensor *device, void *arg,
				 unsigned int notification);
	int (*resume)(struct device *dev);
	int (*standby_flush)(struct is_device_sensor *device);
	int (*notify_by_line)(struct is_device_sensor *device, unsigned int notification);
	int (*buf_skip)(struct is_device_sensor *device, struct is_subdev *f_subdev,
			struct v4l2_subdev *v_subdev, struct is_frame *ldr_frame);
	int (*buf_tag)(struct is_device_sensor *device, struct is_subdev *f_subdev,
		       struct v4l2_subdev *v_subdev, struct is_frame *ldr_frame);
	int (*s_framerate)(struct is_device_sensor *device, struct v4l2_streamparm *param_);
	void (*snr)(struct timer_list *data);
	int (*prepare_retention)(struct is_device_sensor *sensor, int position);
	int (*suspend)(struct device *dev);
};

struct pablo_kunit_device_sensor_func *pablo_kunit_get_device_sensor_func(void);
#endif
#endif
