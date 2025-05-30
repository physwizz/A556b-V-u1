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

#ifndef IS_COMMON_CONFIG_H
#define IS_COMMON_CONFIG_H

/*
 * =================================================================================================
 * CONFIG - GLOBAL OPTIONS
 * =================================================================================================
 */
#define IS_STR_LEN 13

#define IS_MAX_PRIO (MAX_RT_PRIO)
#define NI_BACKUP_MAX 32
#define INTENT_RETRY_CNT 3
#define CAPTURE_INTENT_RETRY_CNT 1

#define IS_EXP_BACKUP_COUNT 2
/*
 * =================================================================================================
 * CONFIG - FEATURE ENABLE
 * =================================================================================================
 */

/* #define ENABLE_PANIC_SFR_PRINT */
/* #define ENABLE_MIF_400 */
#define ENABLE_DBG_STATE
#define ENABLE_DYNAMIC_MEM
#if defined(CONFIG_OF_RESERVED_MEM)
#define CLOG_RESERVED_MEM 1
#endif

#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
#define ENABLE_DVFS 1
#endif /* CONFIG_PM_DEVFREQ */

/* Config related to control HW directly */
#define ENABLE_CSIISR /* this feature is only available at csi v1 */

/* notifier for MIF throttling */
#undef CONFIG_CPU_THERMAL_IPA
#if defined(CONFIG_CPU_THERMAL_IPA)
#define EXYNOS_MIF_ADD_NOTIFIER(nb) exynos_mif_add_notifier(nb)
#else
#define EXYNOS_MIF_ADD_NOTIFIER(nb)
#endif

/* BUG_ON | FIMC_BUG Macro control */
#define USE_FIMC_BUG

#if IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON)
#define USE_KERNEL_VFS_READ_WRITE
#endif

#if IS_ENABLED(CONFIG_DMA_BUF_CONTAINER) || IS_ENABLED(CONFIG_DMABUF_CONTAINER)
#define DMABUF_CONTAINER 1
#endif
/*
 * =================================================================================================
 * CONFIG - DEBUG OPTIONS
 * =================================================================================================
 */
/* #define DEBUG_LOG_MEMORY */
/* #define DEBUG_HW_SIZE */
#define DBG_STREAM_ID ((1 << IS_STREAM_COUNT) - 1)
/* #define DBG_JITTER */
#define FW_PANIC_ENABLE
/* #define SENSOR_PANIC_ENABLE */
/* #define ENABLE_KERNEL_LOG_DUMP */
#define FIXED_FPS_DEBUG 0

#if IS_ENABLED(CONFIG_EXYNOS_BCM_DBG)
#define ENABLE_CAMERA_BCM 1
#endif

/* 30fps */
#define FIXED_MAX_FPS_VALUE (30)
#define FIXED_MIN_FPS_VALUE (1)
#define FIXED_EXPOSURE_VALUE (200000) /* 33.333 * 6 */
#define FIXED_AGAIN_VALUE (150 * 6)
#define FIXED_DGAIN_VALUE (150 * 6)
/* #define DBG_CSIISR */
#define DBG_HAL_DEAD_PANIC_DELAY (500) /* ms */
#define DBG_DMA_DUMP_PATH "/data/camera"
/* #define DEBUG_HW_SFR */
/* #define PRINT_CAPABILITY */
/* #define PRINT_BUFADDR */

#define SWAP(x, y, T)                                                                              \
	do {                                                                                       \
		T SWAP = (x);                                                                      \
		(x) = (y);                                                                         \
		(y) = SWAP;                                                                        \
	} while (0)

#define GET_SSX_ID(video) (video->id - IS_VIDEO_SS0_NUM)
#define GET_PAFXS_ID(video)                                                                        \
	((video->id < IS_VIDEO_PAF1S_NUM) ? 0 :                                                    \
	 (video->id < IS_VIDEO_PAF2S_NUM) ? 1 :                                                    \
	 (video->id < IS_VIDEO_PAF3S_NUM) ? 2 :                                                    \
					    3)
#define GET_3XS_ID(video)                                                                          \
	((video->id < IS_VIDEO_31S_NUM) ? 0 :                                                      \
	 (video->id < IS_VIDEO_32S_NUM) ? 1 :                                                      \
	 (video->id < IS_VIDEO_33S_NUM) ? 2 :                                                      \
					  3)
#define GET_3XC_ID(video)                                                                          \
	((video->id < IS_VIDEO_31S_NUM) ? 0 :                                                      \
	 (video->id < IS_VIDEO_32S_NUM) ? 1 :                                                      \
	 (video->id < IS_VIDEO_33S_NUM) ? 2 :                                                      \
					  3)
#define GET_3XP_ID(video)                                                                          \
	((video->id < IS_VIDEO_31S_NUM) ? 0 :                                                      \
	 (video->id < IS_VIDEO_32S_NUM) ? 1 :                                                      \
	 (video->id < IS_VIDEO_33S_NUM) ? 2 :                                                      \
					  3)
#define GET_3XF_ID(video)                                                                          \
	((video->id < IS_VIDEO_31S_NUM) ? 0 :                                                      \
	 (video->id < IS_VIDEO_32S_NUM) ? 1 :                                                      \
	 (video->id < IS_VIDEO_33S_NUM) ? 2 :                                                      \
					  3)
#define GET_3XG_ID(video)                                                                          \
	((video->id < IS_VIDEO_31S_NUM) ? 0 :                                                      \
	 (video->id < IS_VIDEO_32S_NUM) ? 1 :                                                      \
	 (video->id < IS_VIDEO_33S_NUM) ? 2 :                                                      \
					  3)
#define GET_3XO_ID(video)                                                                          \
	((video->id < IS_VIDEO_31S_NUM) ? 0 :                                                      \
	 (video->id < IS_VIDEO_32S_NUM) ? 1 :                                                      \
	 (video->id < IS_VIDEO_33S_NUM) ? 2 :                                                      \
					  3)
#define GET_3XL_ID(video)                                                                          \
	((video->id < IS_VIDEO_31S_NUM) ? 0 :                                                      \
	 (video->id < IS_VIDEO_32S_NUM) ? 1 :                                                      \
	 (video->id < IS_VIDEO_33S_NUM) ? 2 :                                                      \
					  3)
#define GET_IXS_ID(video) ((video->id < IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXC_ID(video) ((video->id < IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXP_ID(video) ((video->id < IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXT_ID(video) ((video->id < IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXG_ID(video) ((video->id < IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXV_ID(video) ((video->id < IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_IXW_ID(video) ((video->id < IS_VIDEO_I1S_NUM) ? 0 : 1)
#define GET_MEXC_ID(video) (video->id - IS_VIDEO_ME0C_NUM)
#define GET_ORBXC_ID(video) (video->id - IS_VIDEO_ORB0C_NUM)
#define GET_MXS_ID(video) (video->id - IS_VIDEO_M0S_NUM)
#define GET_MXP_ID(video) (video->id - IS_VIDEO_M0P_NUM)
#define GET_LMEXS_ID(video) ((video->id < IS_VIDEO_LME1_NUM) ? 0 : 1)

#define GET_STR(str) #str

#define GET_ZOOM_RATIO(in, out) (typeof(out))(((ulong)(in) << MCSC_PRECISION) / (out))
#define GET_SCALED_SIZE(in, ratio) (typeof(ratio))(((ulong)(in) << MCSC_PRECISION) / (ratio))
/* sync log with HAL, FW */
#define log_sync(sync_id) info("IS_SYNC %d\n", sync_id)

#define test_bit_variables(bit, var) test_bit(((bit) % BITS_PER_LONG), (var))

#define CHECK_TAC_ID(vid)                                                                          \
	((vid == IS_VIDEO_30C_NUM) || (vid == IS_VIDEO_31C_NUM) || (vid == IS_VIDEO_32C_NUM) ||    \
	 (vid == IS_VIDEO_33C_NUM))
#define CHECK_TAP_ID(vid)                                                                          \
	((vid == IS_VIDEO_30P_NUM) || (vid == IS_VIDEO_31P_NUM) || (vid == IS_VIDEO_32P_NUM) ||    \
	 (vid == IS_VIDEO_33P_NUM))
#define CHECK_TAG_ID(vid)                                                                          \
	((vid == IS_VIDEO_30G_NUM) || (vid == IS_VIDEO_31G_NUM) || (vid == IS_VIDEO_32G_NUM) ||    \
	 (vid == IS_VIDEO_33G_NUM))
#define CHECK_TAF_ID(vid)                                                                          \
	((vid == IS_VIDEO_30F_NUM) || (vid == IS_VIDEO_31F_NUM) || (vid == IS_VIDEO_32F_NUM) ||    \
	 (vid == IS_VIDEO_33F_NUM))

char *pablo_lib_get_stream_prefix(int instance);
#define stm(i) (pablo_lib_get_stream_prefix(i))

/*
 * =================================================================================================
 * LOG - ERR
 * =================================================================================================
 */
#ifdef err
#undef err
#endif
#define err(fmt, args...) err_common("[ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)

/* multi-stream */
#define merr(fmt, obj, args...)                                                                    \
	merr_common("[%d:%s][ERR]%s:%d:", fmt "\n", obj->instance, obj->stm, __func__, __LINE__,   \
		    ##args)

/* multi-stream with instance */
#define mierr(fmt, instance, args...)                                                              \
	merr_common("[%d:%s][ERR]%s:%d:", fmt "\n", instance, stm(instance), __func__, __LINE__,   \
		    ##args)

/* multi-stream & group error */
#define mgerr(fmt, obj, group, args...)                                                            \
	merr_common("[%d:%s][%s][ERR]%s:%d:", fmt "\n", obj->instance, obj->stm,                   \
		    group_id_name[group->id], __func__, __LINE__, ##args)

/* multi-stream & subdev error */
#define mserr(fmt, obj, subdev, args...)                                                           \
	merr_common("[%d:%s][%s][ERR]%s:%d:", fmt "\n", obj->instance, obj->stm, subdev->name,     \
		    __func__, __LINE__, ##args)

/* multi-stream & video error */
#define mverr(fmt, obj, video, args...)                                                            \
	merr_common("[%d:%s][V%02d][ERR]%s:%d:", fmt "\n", obj->instance, obj->stm, video->id,     \
		    __func__, __LINE__, ##args)

/* multi-stream & runtime error */
#define mrerr(fmt, obj, frame, args...)                                                            \
	merr_common("[%d:%s][F%d][ERR]%s:%d:", fmt "\n", obj->instance, obj->stm, frame->fcount,   \
		    __func__, __LINE__, ##args)

/* multi-stream & group & runtime error */
#define mgrerr(fmt, obj, group, frame, args...)                                                    \
	merr_common("[%d:%s][%s][F%d][ERR]%s:%d:", fmt "\n", obj->instance, obj->stm,              \
		    group_id_name[group->id], frame->fcount, __func__, __LINE__, ##args)

#define mlverr(fmt, obj, vid, args...)                                                             \
	merr_common("[%d:%s][%s][ERR]%s:%d:", fmt "\n", obj->instance, obj->stm, vn_name[vid],     \
		    __func__, __LINE__, ##args)

#define mcerr(fmt, obj, args...)                                                                   \
	merr_common("[%d:%s][CSI%d][D%d][ERR]%s:%d:", fmt "\n", obj->instance, obj->stm,           \
		    obj->otf_info.csi_ch, (obj->wdma[0]) ? obj->wdma[0]->ch : 0, __func__,         \
		    __LINE__, ##args)
/*
 * =================================================================================================
 * LOG - WARN
 * =================================================================================================
 */
#ifdef warn
#undef warn
#endif
#define warn(fmt, args...) warn_common("[WRN]%s:%d:", fmt "\n", __func__, __LINE__, ##args)

#define mwarn(fmt, obj, args...)                                                                   \
	mwarn_common("[%d:%s][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm, __func__, __LINE__,  \
		     ##args)

#define mvwarn(fmt, obj, video, args...)                                                           \
	mwarn_common("[%d:%s][V%02d][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm, video->id,    \
		     __func__, __LINE__, ##args)

#define mgwarn(fmt, obj, group, args...)                                                           \
	mwarn_common("[%d:%s][%s][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm,                  \
		     group_id_name[group->id], __func__, __LINE__, ##args)

#define mrwarn(fmt, obj, frame, args...)                                                           \
	mwarn_common("[%d:%s][F%d][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm, frame->fcount,  \
		     __func__, __LINE__, ##args)

#define mswarn(fmt, obj, subdev, args...)                                                          \
	mwarn_common("[%d:%s][%s][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm, subdev->name,    \
		     __func__, __LINE__, ##args)

#define mgrwarn(fmt, obj, group, frame, args...)                                                   \
	mwarn_common("[%d:%s][%s][F%d][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm,             \
		     group_id_name[group->id], frame->fcount, __func__, __LINE__, ##args)

#define msrwarn(fmt, obj, subdev, frame, args...)                                                  \
	mwarn_common("[%d:%s][%s][F%d][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm,             \
		     subdev->name, frame->fcount, __func__, __LINE__, ##args)

#define mlvwarn(fmt, obj, vid, args...)                                                            \
	mwarn_common("[%d:%s][%s][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm, vn_name[vid],    \
		     __func__, __LINE__, ##args)

#define mcwarn(fmt, obj, args...)                                                                  \
	mwarn_common("[%d:%s][CSI%d][D%d][WRN]%s:%d:", fmt "\n", obj->instance, obj->stm,          \
		     obj->otf_info.csi_ch, (obj->wdma[0]) ? obj->wdma[0]->ch : 0, __func__,        \
		     __LINE__, ##args)
/*
 * =================================================================================================
 * LOG - INFO
 * =================================================================================================
 */
#define info(fmt, args...) info_common("", fmt, ##args)

//#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
//#define sfrinfo(fmt, args...)
//#else
#define sfrinfo(fmt, args...) info_common("[SFR]", fmt, ##args)
//#endif

#define minfo(fmt, obj, args...) minfo_common("[%d:%s]", fmt, obj->instance, obj->stm, ##args)

#define mvinfo(fmt, obj, video, args...)                                                           \
	minfo_common("[%d:%s][V%02d]", fmt, obj->instance, obj->stm, video->id, ##args)

#define msinfo(fmt, obj, subdev, args...)                                                          \
	minfo_common("[%d:%s][%s]", fmt, obj->instance, obj->stm, subdev->name, ##args)

#define msrinfo(fmt, obj, subdev, frame, args...)                                                  \
	minfo_common("[%d:%s][%s][F%d]", fmt, obj->instance, obj->stm, subdev->name,               \
		     frame->fcount, ##args)

#define mginfo(fmt, obj, group, args...)                                                           \
	minfo_common("[%d:%s][%s]", fmt, obj->instance, obj->stm, group_id_name[group->id], ##args)

#define mrinfo(fmt, obj, frame, args...)                                                           \
	minfo_common("[%d:%s][F%d]", fmt, obj->instance, obj->stm, frame->fcount, ##args)

#define mgrinfo(fmt, obj, group, frame, args...)                                                   \
	minfo_common("[%d:%s][%s][F%d]", fmt, obj->instance, obj->stm, group_id_name[group->id],   \
		     frame->fcount, ##args)

#define mlvinfo(fmt, obj, vid, args...)                                                            \
	minfo_common("[%d:%s][%s]", fmt, obj->instance, obj->stm, vn_name[vid], ##args)

#define mcinfo(fmt, obj, args...)                                                                  \
	minfo_common("[%d:%s][CSI%d][D%d]", fmt, obj->instance, obj->stm, obj->otf_info.csi_ch,    \
		     (obj->wdma[0]) ? obj->wdma[0]->ch : 0, ##args)

#define mdbg_pframe(fmt, obj, subdev, frame, args...)                                              \
	minfo_common("[%d:%s][%s][F%d]", fmt, obj->instance, obj->stm, subdev->name,               \
		     frame->fcount, ##args)

#define ssinfo(fmt, cis, args...) \
	info_common("%s [%d][%d][%s] ", fmt,	__func__, \
		(cis)->id, (cis)->cis_data->sen_vsync_count, (cis)->sensor_info->name, ##args)

#define ssinfo_seamless_mode(cis, m) \
	info("%s [%d][%d][%s] SS_DBG LN2[%d] LN4[%d] ADC11[%d] 12bit[%d] AEB[%d] RM[%d,%d]\n", \
		__func__, (cis)->id, (cis)->cis_data->sen_vsync_count, (cis)->sensor_info->name, \
		((m)->mode & SENSOR_MODE_LN2), \
		((m)->mode & SENSOR_MODE_LN4), \
		((m)->mode & SENSOR_MODE_ADC11), \
		((m)->mode & SENSOR_MODE_REAL_12BIT), \
		((m)->mode & SENSOR_MODE_AEB), \
		((m)->mode & SENSOR_MODE_CROPPED_RMS), \
		(m)->remosaic_crop_zoom_ratio)

/*
 * =================================================================================================
 * LOG - DEBUG_SENSOR
 * =================================================================================================
 */
#define dbg(fmt, args...)

int is_get_debug_sensor(void);
#define dbg_sensor(level, fmt, args...)                                                            \
	do {                                                                                       \
		int debug_sensor = is_get_debug_param(IS_DEBUG_PARAM_SENSOR);                      \
		dbg_common((debug_sensor >= (level)) && (debug_sensor < 3), "[SSD]", fmt, ##args); \
	} while (0)

#define dbg_actuator(fmt, args...)                                                                 \
	do {                                                                                       \
		int debug_sensor = is_get_debug_param(IS_DEBUG_PARAM_SENSOR);                      \
		dbg_common((debug_sensor >= 3) && (debug_sensor < 4), "[ACT]", fmt, ##args);       \
	} while (0)
#define dbg_flash(fmt, args...)                                                                    \
	do {                                                                                       \
		int debug_sensor = is_get_debug_param(IS_DEBUG_PARAM_SENSOR);                      \
		dbg_common((debug_sensor >= 4) && (debug_sensor < 5), "[FLS]", fmt, ##args);       \
	} while (0)
#define dbg_preproc(fmt, args...)                                                                  \
	do {                                                                                       \
		int debug_sensor = is_get_debug_param(IS_DEBUG_PARAM_SENSOR);                      \
		dbg_common((debug_sensor >= 5) && (debug_sensor < 6), "[PRE]", fmt, ##args);       \
	} while (0)
#define dbg_aperture(fmt, args...)                                                                 \
	do {                                                                                       \
		int debug_sensor = is_get_debug_param(IS_DEBUG_PARAM_SENSOR);                      \
		dbg_common((debug_sensor >= 6) && (debug_sensor < 7), "[APERTURE]", fmt, ##args);  \
	} while (0)
#define dbg_ois(fmt, args...)                                                                      \
	do {                                                                                       \
		int debug_sensor = is_get_debug_param(IS_DEBUG_PARAM_SENSOR);                      \
		dbg_common((debug_sensor >= 7) && (debug_sensor < 8), "[OIS]", fmt, ##args);       \
	} while (0)

#define dbg_sensor_seamless_mode(cis, m) \
	do { \
		int debug_sensor = is_get_debug_param(IS_DEBUG_PARAM_SENSOR); \
		if ((debug_sensor >= 1) && (debug_sensor < 3)) { \
			ssinfo_seamless_mode(cis, m); \
		} \
	} while (0)
/*
 * =================================================================================================
 * LOG - DEBUG_VIDEO
 * =================================================================================================
 */
#define mdbgv_vid(fmt, args...)                                                                    \
	dbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[VID:V]", fmt, ##args)

#define mdbgv_sensor(fmt, ivc, args...)                                                            \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][%s]", fmt,                     \
		    ((struct is_device_sensor *)(ivc)->device)->instance,                          \
		    vn_name[(ivc)->video->id], ##args)

#define mdbgv_ischain(fmt, ivc, args...)                                                           \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][%s]", fmt,                     \
		    ((struct is_device_ischain *)(ivc)->device)->instance,                         \
		    vn_name[(ivc)->video->id], ##args)

#define mdbgv_3aa(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][3%dS:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_3XS_ID(this->video), \
		    ##args)

#define mdbgv_3xc(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][3%dC:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_3XC_ID(this->video), \
		    ##args)

#define mdbgv_3xp(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][3%dP:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_3XP_ID(this->video), \
		    ##args)

#define mdbgv_3xf(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][3%dF:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_3XF_ID(this->video), \
		    ##args)

#define mdbgv_3xg(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][3%dG:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_3XG_ID(this->video), \
		    ##args)

#define mdbgv_3xo(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][3%dO:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_3XO_ID(this->video), \
		    ##args)

#define mdbgv_3xl(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][3%dL:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_3XL_ID(this->video), \
		    ##args)

#define mdbgv_isp(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][I%dS:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_IXS_ID(this->video), \
		    ##args)

#define mdbgv_ixc(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][I%dC:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_IXC_ID(this->video), \
		    ##args)

#define mdbgv_ixp(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][I%dP:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_IXP_ID(this->video), \
		    ##args)

#define mdbgv_ixt(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][I%dT:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_IXT_ID(this->video), \
		    ##args)

#define mdbgv_ixg(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][I%dG:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_IXG_ID(this->video), \
		    ##args)

#define mdbgv_ixv(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][I%dV:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_IXV_ID(this->video), \
		    ##args)

#define mdbgv_ixw(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][I%dW:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_IXV_ID(this->video), \
		    ##args)

#define mdbgv_mexc(fmt, this, args...)                                                             \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][ME%dC:V]", fmt,                \
		    ((struct is_device_ischain *)this->device)->instance,                          \
		    GET_MEXC_ID(this->video), ##args)

#define mdbgv_orbxc(fmt, this, args...)                                                            \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][ORB%dC:V]", fmt,               \
		    ((struct is_device_ischain *)this->device)->instance,                          \
		    GET_ORBXC_ID(this->video), ##args)

#define mdbgv_mcs(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][M%dS:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_MXS_ID(this->video), \
		    ##args)

#define mdbgv_mxp(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][M%dP:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, GET_MXP_ID(this->video), \
		    ##args)

#define mdbgv_vra(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][VRA:V]", fmt,                  \
		    ((struct is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_ssxvc0(fmt, this, args...)                                                           \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][SSXVC0:V]", fmt,               \
		    ((struct is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc1(fmt, this, args...)                                                           \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][SSXVC1:V]", fmt,               \
		    ((struct is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc2(fmt, this, args...)                                                           \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][SSXVC2:V]", fmt,               \
		    ((struct is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_ssxvc3(fmt, this, args...)                                                           \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][SSXVC3:V]", fmt,               \
		    ((struct is_device_sensor *)this->device)->instance, ##args)

#define mdbgv_paf(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][PAF%dS:V]", fmt,               \
		    ((struct is_device_ischain *)this->device)->instance,                          \
		    GET_PAFXS_ID(this->video), ##args)

#define mdbgv_ypp(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][YPP:V]", fmt,                  \
		    ((struct is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_mcfp(fmt, this, args...)                                                             \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][MCFP:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_lme(fmt, this, args...)                                                              \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][LME:V]", fmt,                  \
		    ((struct is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_lmes(fmt, this, args...)                                                             \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][LMES:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_lmec(fmt, this, args...)                                                             \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][LMEC:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_lvn(level, fmt, obj, video, args...)                                                 \
	mdbg_common((is_get_debug_param(IS_DEBUG_PARAM_LVN)) >= (level), "[%d][V%02d]", fmt,       \
		    obj->instance, video->id, ##args)

#define mdbgv_byrp(fmt, this, args...)                                                             \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][BYRP:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, ##args)

#define mdbgv_rgbp(fmt, this, args...)                                                             \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_VIDEO), "[%d][RGBP:V]", fmt,                 \
		    ((struct is_device_ischain *)this->device)->instance, ##args)

/*
 * =================================================================================================
 * LOG - DEBUG_DEVICE
 * =================================================================================================
 */
#define mdbgd_sensor(fmt, this, args...)                                                           \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_DEVICE), "[%d:%s][SEN:D]", fmt,              \
		    this->instance, this->stm, ##args)

#define mdbgd_front(fmt, this, args...)                                                            \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_DEVICE), "[%d:%s][FRT:D]", fmt,              \
		    this->instance, this->stm, ##args)

#define mdbgd_back(fmt, this, args...)                                                             \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_DEVICE), "[%d:%s][BAK:D]", fmt,              \
		    this->instance, this->stm, ##args)

#define mdbgd_ischain(fmt, this, args...)                                                          \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_DEVICE), "[%d:%s][ISC:D]", fmt,              \
		    this->instance, this->stm, ##args)

#define mdbgd_group(fmt, this, args...)                                                            \
	mdbg_common(is_get_debug_param(IS_DEBUG_PARAM_DEVICE), "[%d:%s][%s:D]", fmt,               \
		    this->instance, this->stm, group_id_name[group->id], ##args)

#define dbgd_resource(fmt, args...)                                                                \
	dbg_common(is_get_debug_param(IS_DEBUG_PARAM_DEVICE), "[RSC:D]", fmt, ##args)

/*
 * =================================================================================================
 * LOG - DEBUG_STREAM
 * =================================================================================================
 */
#define dbg_interface(level, fmt, args...)                                                         \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[ITF:S]", fmt, ##args)

#define mvdbgs(level, fmt, obj, queue, args...)                                                    \
	mdbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[%d:%s][%s:S]", fmt,  \
		    obj->instance, obj->stm, (queue)->name, ##args)

#define mgrdbgs(level, fmt, obj, group, frame, args...)                                            \
	mdbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[%d:%s][%s:S][F%d]",  \
		    fmt, obj->instance, obj->stm, group_id_name[group->id], frame->fcount, ##args)

#define msdbgs(level, fmt, obj, subdev, args...)                                                   \
	mdbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[%d:%s][%s:S]", fmt,  \
		    obj->instance, obj->stm, subdev->name, ##args)

#define msrdbgs(level, fmt, obj, subdev, frame, args...)                                           \
	mdbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[%d:%s][%s:S][F%d]",  \
		    fmt, obj->instance, obj->stm, subdev->name, frame->fcount, ##args)

#define mdbgs_ischain(level, fmt, obj, args...)                                                    \
	mdbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[%d:%s][ISC:S]", fmt, \
		    obj->instance, obj->stm, ##args)

#define mdbgs_sensor(level, fmt, obj, args...)                                                     \
	mdbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[%d:%s][SEN:S]", fmt, \
		    obj->instance, obj->stm, ##args)

#define msdbgs_hw(level, fmt, instance, obj, args...)                                              \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[%d:%s][HW:%s:S]",     \
		   fmt, instance, obj->stm[instance], obj->name, ##args)

#define mlvdbgs(level, fmt, obj, vid, args...)                                                     \
	mdbg_common((is_get_debug_param(IS_DEBUG_PARAM_STREAM)) >= (level), "[%d:%s][%s]", fmt,    \
		    obj->instance, stm(obj->instance), vn_name[vid], ##args)
/*
 * =================================================================================================
 * LOG - PROBE
 * =================================================================================================
 */
#define probe_info(fmt, ...) pr_info("[@]" fmt, ##__VA_ARGS__)
#define probe_err(fmt, args...) pr_err("[@][ERR]%s:%d:" fmt "\n", __func__, __LINE__, ##args)
#define probe_warn(fmt, args...) pr_warn("[@][WRN]" fmt "\n", ##args)

#if defined(DEBUG_LOG_MEMORY)
#define is_err(fmt, ...) printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define is_warn(fmt, ...) printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define is_dbg(fmt, ...) printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define is_info(fmt, ...) printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define is_cont(fmt, ...) printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#else
#define is_err(fmt, ...) pr_err(fmt, ##__VA_ARGS__)
#define is_warn(fmt, ...) pr_warn(fmt, ##__VA_ARGS__)
#define is_dbg(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#define is_info(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#define is_cont(fmt, ...) pr_cont(fmt, ##__VA_ARGS__)
#endif

#define merr_common(prefix, fmt, instance, args...)                                                \
	do {                                                                                       \
		is_err("[@]" prefix fmt, instance, ##args);                                        \
		is_memlog_err(prefix fmt, instance, ##args);                                       \
	} while (0)

#define mwarn_common(prefix, fmt, instance, args...)                                               \
	do {                                                                                       \
		is_warn("[@]" prefix fmt, instance, ##args);                                       \
		is_memlog_warn(prefix fmt, instance, ##args);                                      \
	} while (0)

#define minfo_common(prefix, fmt, instance, args...)                                               \
	do {                                                                                       \
		is_info("[@]" prefix fmt, instance, ##args);                                       \
		is_memlog_info(prefix fmt, instance, ##args);                                      \
	} while (0)

#define mdbg_common(level, prefix, fmt, instance, args...)                                         \
	do {                                                                                       \
		if (unlikely(level))                                                               \
			is_dbg("[@][DBG]" prefix fmt, instance, ##args);                           \
		is_memlog_dbg(prefix fmt, instance, ##args);                                       \
	} while (0)

#define err_common(prefix, fmt, args...)                                                           \
	do {                                                                                       \
		is_err("[@]" prefix fmt, ##args);                                                  \
		is_memlog_err(prefix fmt, ##args);                                                 \
	} while (0)

#define warn_common(prefix, fmt, args...)                                                          \
	do {                                                                                       \
		is_warn("[@]" prefix fmt, ##args);                                                 \
		is_memlog_warn(prefix fmt, ##args);                                                \
	} while (0)

#define info_common(prefix, fmt, args...)                                                          \
	do {                                                                                       \
		is_info("[@]" prefix fmt, ##args);                                                 \
		is_memlog_info(prefix fmt, ##args);                                                \
	} while (0)

#define dbg_common(level, prefix, fmt, args...)                                                    \
	do {                                                                                       \
		if (unlikely(level))                                                               \
			is_dbg("[@][DBG]" prefix fmt, ##args);                                     \
		is_memlog_dbg(prefix fmt, ##args);                                                 \
	} while (0)

/*
 * =================================================================================================
 * LOG - DEBUG_IRQ
 * =================================================================================================
 */
#define dbg_isr(level, fmt, obj, args...)                                                          \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_IRQ)) >= (level), "[%d:%s][%s]", fmt,        \
		   obj->instance, obj->stm, obj->name, ##args)

#define dbg_hw(level, fmt, args...)                                                                \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_HW)) >= (level), "[HW]", fmt, ##args)
#define mdbg_hw(level, fmt, instance, args...)                                                     \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_HW)) >= (level), "[%d:%s][HW]", fmt,         \
		   instance, stm(instance), ##args)
#define sdbg_hw(level, fmt, obj, args...)                                                          \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_HW)) >= (level), "[HW:%s]", fmt, obj->name,  \
		   ##args)
#define msdbg_hw(level, fmt, instance, obj, args...)                                               \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_HW)) >= (level), "[%d:%s][HW:%s]", fmt,      \
		   instance, stm(instance), obj->name, ##args)

#define info_hw(fmt, args...) info_common("[HW]", fmt, ##args)
#define minfo_hw(fmt, instance, args...)                                                           \
	info_common("[%d:%s][HW]", fmt, instance, stm(instance), ##args)
#define sinfo_hw(fmt, obj, args...) info_common("[HW:%s]", fmt, obj->name, ##args)
#define msinfo_hw(fmt, instance, obj, args...)                                                     \
	info_common("[%d:%s][HW:%s]", fmt, instance, obj->stm[instance], obj->name, ##args)

#define warn_hw(fmt, args...) warn_common("[HW][WRN]%d:", fmt "\n", __LINE__, ##args)
#define mwarn_hw(fmt, instance, args...)                                                           \
	warn_common("[%d:%s][HW][WRN]%d:", fmt "\n", instance, stm(instance), __LINE__, ##args)
#define swarn_hw(fmt, obj, args...)                                                                \
	warn_common("[HW:%s][WRN]%d:", fmt "\n", obj->name, __LINE__, ##args)
#define mswarn_hw(fmt, instance, obj, args...)                                                     \
	warn_common("[%d:%s][HW:%s][WRN]%d:", fmt "\n", instance, stm(instance), obj->name,        \
		    __LINE__, ##args)

#define err_hw(fmt, args...) err_common("[HW][ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)
#define merr_hw(fmt, instance, args...)                                                            \
	err_common("[%d:%s][HW][ERR]%s:%d:", fmt "\n", instance, stm(instance), __func__,          \
		   __LINE__, ##args)
#define serr_hw(fmt, obj, args...)                                                                 \
	err_common("[HW:%s][ERR]%s:%d:", fmt "\n", obj->name, __func__, __LINE__, ##args)
#define mserr_hw(fmt, instance, obj, args...)                                                      \
	err_common("[%d:%s][HW:%s][ERR]%s:%d:", fmt "\n", instance, obj->stm[instance], obj->name, \
		   __func__, __LINE__, ##args)

#define dbg_lib(level, fmt, args...)                                                               \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_HW)) >= (level), "[LIB]", fmt, ##args)
#define info_lib(fmt, args...) info_common("[LIB]", fmt, ##args)
#define minfo_lib(fmt, instance, args...) info_common("[%d][LIB]", fmt, instance, ##args)
#define msinfo_lib(fmt, instance, obj, args...)                                                    \
	info_common("[%d:%s][LIB:%s]", fmt, instance, obj->stm[instance], obj->name, ##args)
#define warn_lib(fmt, args...) warn_common("[LIB][WARN]%d:", fmt "\n", __LINE__, ##args)
#define err_lib(fmt, args...) err_common("[LIB][ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)
#define mserr_lib(fmt, instance, obj, args...)                                                     \
	err_common("[%d:%s][LIB:%s][ERR]%s:%d:", fmt "\n", instance, obj->stm[instance],           \
		   obj->name, __func__, __LINE__, ##args)

#define dbg_itfc(fmt, args...)                                                                     \
	dbg_common(is_get_debug_param(IS_DEBUG_PARAM_HW), "[ITFC]", fmt, ##args)
#define info_itfc(fmt, args...) info_common("[ITFC]", fmt, ##args)
#define err_itfc(fmt, args...) err_common("[ITFC][ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)

#define dbg_adt(level, fmt, args...)                                                               \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_HW)) >= (level), "[ADT]", fmt, ##args)
#define info_adt(fmt, args...) info_common("[ADT]", fmt, ##args)
#define warn_adt(fmt, args...) warn_common("[ADT][WRN]%d:", fmt "\n", __LINE__, ##args)
#define err_adt(fmt, args...) err_common("[ADT][ERR]%s:%d:", fmt "\n", __func__, __LINE__, ##args)

#define mdbg_adt(level, fmt, instance, args...)                                                    \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_HW)) >= (level), "[%d:%s][ADT]", fmt,        \
		   instance, stm(instance), ##args)
#define minfo_adt(fmt, instance, args...)                                                          \
	info_common("[%d:%s][ADT]", fmt, instance, stm(instance), ##args)
#define mwarn_adt(fmt, instance, args...)                                                          \
	warn_common("[%d:%s][ADT][WRN]%d:", fmt "\n", instance, stm(instance), __LINE__, ##args)
#define merr_adt(fmt, instance, args...)                                                           \
	err_common("[%d:%s][ADT][ERR]%s:%d:", fmt "\n", instance, stm(instance), __func__,         \
		   __LINE__, ##args)

#define dbg_mem(level, fmt, args...)                                                               \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_MEM)) >= (level), "[MEM]", fmt, ##args)

#define dbg_dvfs(level, fmt, args...)                                                              \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_DVFS)) >= (level), "[DVFS]", fmt, ##args)
#define mdbg_dvfs(level, fmt, device, args...)                                                     \
	dbg_common((is_get_debug_param(IS_DEBUG_PARAM_DVFS)) >= (level), "[%d]", fmt,              \
		   device->instance, ##args)

#ifdef USE_FIMC_BUG
#define FIMC_BUG(condition)                                                                        \
	{                                                                                          \
		if (unlikely(condition)) {                                                         \
			info("[BUG][%s] %s:%d(%s)\n", __FILE__, __func__, __LINE__, #condition);   \
			return -EINVAL;                                                            \
		}                                                                                  \
	}
#define FIMC_BUG_VOID(condition)                                                                   \
	{                                                                                          \
		if (unlikely(condition)) {                                                         \
			info("[BUG][%s] %s:%d(%s)\n", __FILE__, __func__, __LINE__, #condition);   \
			return;                                                                    \
		}                                                                                  \
	}
#define FIMC_BUG_NULL(condition)                                                                   \
	{                                                                                          \
		if (unlikely(condition)) {                                                         \
			info("[BUG][%s] %s:%d(%s)\n", __FILE__, __func__, __LINE__, #condition);   \
			return NULL;                                                               \
		}                                                                                  \
	}
#define FIMC_BUG_FALSE(condition)                                                                  \
	{                                                                                          \
		if (unlikely(condition)) {                                                         \
			info("[BUG][%s] %s:%d(%s)\n", __FILE__, __func__, __LINE__, #condition);   \
			return false;                                                              \
		}                                                                                  \
	}
#else
#define FIMC_BUG(condition) BUG_ON(condition);
#define FIMC_BUG_VOID(condition) BUG_ON(condition);
#define FIMC_BUG_NULL(condition) BUG_ON(condition);
#endif

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
#define KUNIT_EXPORT_SYMBOL(x) EXPORT_SYMBOL_GPL(x)
#else
#define KUNIT_EXPORT_SYMBOL(x)
#endif

#define do_nothing                                                                                 \
	do {                                                                                       \
	} while (0)
#define do_nothing_0 ({ 0; })

#if IS_ENABLED(CONFIG_PABLO_UNIT_TEST)
#define PST_EXPORT_SYMBOL(x) EXPORT_SYMBOL_GPL(x)
#else
#define PST_EXPORT_SYMBOL(x)
#endif

#if IS_ENABLED(CONFIG_PABLO_UNIT_TEST) || IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
#define PST_KUNIT_EXPORT_SYMBOL(X) EXPORT_SYMBOL_GPL(X)
#else
#define PST_KUNIT_EXPORT_SYMBOL(X)
#endif

#endif
