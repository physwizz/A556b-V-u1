/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ALSA SoC - Samsung Abox driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ABOX_IPC_H
#define __ABOX_IPC_H

#define SYSTEM_MSG_BUNDLE_SIZE 720
#define SYSTEM_MSG_S32_SIZE (720 / 4)
#define SYSTEM_MSG_U64_SIZE (720 / 8)

/******** IPC signal ID *********************/
enum SIGNAL_ID {
	SIGID_SYSTEM = 1,
	SIGID_PCMOUT_TASK0,
	SIGID_PCMOUT_TASK1,
	SIGID_PCMOUT_TASK2,
	SIGID_PCMOUT_TASK3,
	SIGID_PCMIN_TASK0,
	SIGID_PCMIN_TASK1,
	SIGID_OFFLOAD,
	SIGID_ERAP,
	SIGID_ASB,
};

/******** PCMTASK IPC ***********************/
enum PCMMSG {
	PCM_OPEN		= 1,
	PCM_CLOSE		= 2,
	PCM_CPUDAI_TRIGGER	= 3,
	PCM_CPUDAI_HW_PARAMS	= 4,
	PCM_CPUDAI_SET_FMT	= 5,
	PCM_CPUDAI_SET_CLKDIV	= 6,
	PCM_CPUDAI_SET_SYSCLK	= 7,
	PCM_CPUDAI_STARTUP	= 8,
	PCM_CPUDAI_SHUTDOWN	= 9,
	PCM_CPUDAI_DELAY	= 10,
	PCM_PLTDAI_OPEN		= 11,
	PCM_PLTDAI_CLOSE	= 12,
	PCM_PLTDAI_IOCTL	= 13,
	PCM_PLTDAI_HW_PARAMS	= 14,
	PCM_PLTDAI_HW_FREE	= 15,
	PCM_PLTDAI_PREPARE	= 16,
	PCM_PLTDAI_TRIGGER	= 17,
	PCM_PLTDAI_POINTER	= 18,
	PCM_PLTDAI_MMAP		= 19,
	PCM_SET_BUFFER		= 20,
	PCM_SYNCHRONIZE		= 21,
	PCM_PLTDAI_ACK		= 22,
	PCM_PLTDAI_CLOSED	= 23,
	PCM_PLTDAI_REGISTER	= 50,
	PCM_TX_BARGE_IN_DETECT  = 202,
};

struct PCMTASK_HW_PARAMS {
	int sample_rate;
	int bit_depth;
	int channels;
	int packed; /* 24bit only */
	int ip_type;	/*For compressed capture, MP3=0, AAC=1, FLAC=2*/
};

struct PCMTASK_SET_BUFFER {
	int addr;
	int size;
	int count;
	unsigned long long phys;
};

struct PCMTASK_HARDWARE {
	char name[32];			/* name */
	unsigned int addr;		/* buffer address */
	unsigned int width_min;		/* min width */
	unsigned int width_max;		/* max width */
	unsigned int rate_min;		/* min rate */
	unsigned int rate_max;		/* max rate */
	unsigned int channels_min;	/* min channels */
	unsigned int channels_max;	/* max channels */
	unsigned int buffer_bytes_max;	/* max buffer size */
	unsigned int period_bytes_min;	/* min period size */
	unsigned int period_bytes_max;	/* max period size */
	unsigned int periods_min;	/* min # of periods */
	unsigned int periods_max;	/* max # of periods */
};

struct PCMTASK_COMPR_POINTER {
	unsigned int pointer;  /*For compressed capture(encoded size)*/
	unsigned int pcm_size; /*For compressed capture(pcm size consumed by encoder)*/
};

/* Channel id of the Virtual DMA should be started from the BASE */
#define PCMTASK_VDMA_ID_BASE 100

/* Parameter of the PCMTASK command */
struct IPC_PCMTASK_MSG {
	enum PCMMSG msgtype;
	int channel_id;
	union {
		struct PCMTASK_HW_PARAMS hw_params;
		struct PCMTASK_SET_BUFFER setbuff;
		struct PCMTASK_HARDWARE hardware;
		struct PCMTASK_COMPR_POINTER compr_pointer;
		unsigned int pointer;
		int trigger;
		int synchronize;
	} param;
};

/******** OFFLOAD IPC ***********************/
enum OFFLOADMSG {
	OFFLOAD_OPEN = 1,
	OFFLOAD_CLOSE,
	OFFLOAD_SETPARAM,
	OFFLOAD_START,
	OFFLOAD_WRITE,
	OFFLOAD_PAUSE,
	OFFLOAD_STOP,
};

/* The parameter of the set_param */
struct OFFLOAD_SET_PARAM {
	int sample_rate;
	int chunk_size;
};

/* The parameter of the start */
struct OFFLOAD_START {
	int id;
};

/* The parameter of the write */
struct OFFLOAD_WRITE {
	int id;
	int buff;
	int size;
};

/* Parameter of the OFFLOADTASK command */
struct IPC_OFFLOADTASK_MSG {
	enum OFFLOADMSG msgtype;
	int channel_id;
	union {
		struct OFFLOAD_SET_PARAM setparam;
		struct OFFLOAD_START start;
		struct OFFLOAD_WRITE write;
	} param;
};

/******** ABOX_LOOPBACK IPC ***********************/
enum ABOX_ERAP_MSG {
	REALTIME_OPEN = 1,
	REALTIME_CLOSE,
	REALTIME_OUTPUT_SRATE,
	REALTIME_INPUT_SRATE,
	REALTIME_BIND_CHANNEL,
	REALTIME_START,
	REALTIME_STOP,
	REALTIME_PREDSM_AUDIO,
	REALTIME_PREDSM_VI_SENSE,
	REALTIME_TONEGEN = 0x20,
	REALTIME_EXTRA = 0xea,
	REALTIME_USB = 0x30,
};

enum ABOX_ERAP_TYPE {
	ERAP_ECHO_CANCEL,
	ERAP_VI_SENSE,
	ERAP_TYPE_COUNT,
};

enum ABOX_USB_MSG {
	IPC_USB_PCM_OPEN,	/* USB -> ABOX */
	IPC_USB_DESC,
	IPC_USB_XHCI,
	IPC_USB_L2,
	IPC_USB_CONN,
	IPC_USB_CTRL,
	IPC_USB_SET_INTF,
	IPC_USB_SAMPLE_RATE,
	IPC_USB_PCM_BUF,
	IPC_USB_TASK = 0x80,	/* ABOX -> USB */
	IPC_USB_STOP_DONE,
};

struct ERAP_ONOFF_PARAM {
	enum ABOX_ERAP_TYPE type;
	int channel_no;
	int version;
};

struct ERAP_SET_SRATE_PARAM {
	int channel_no;
	int srate;
};

struct ERAP_BIND_CHANNEL_PARAM {
	int input_channel;
	int output_channel;
};

struct ERAP_RAW_PARAM {
	unsigned int params[188];
};

struct ERAP_USB_AUDIO_PARAM {
	enum ABOX_USB_MSG type;
	unsigned int param1;
	unsigned int param2;
	unsigned int param3;
	unsigned int param4;
};

struct IPC_ERAP_MSG {
	enum ABOX_ERAP_MSG msgtype;
	union {
		struct ERAP_ONOFF_PARAM onoff;
		struct ERAP_BIND_CHANNEL_PARAM bind;
		struct ERAP_SET_SRATE_PARAM srate;
		struct ERAP_RAW_PARAM raw;
		struct ERAP_USB_AUDIO_PARAM usbaudio;
	} param;
};

/******** ABOX_CONFIG IPC ***********************/
enum ABOX_CONFIGMSG {
	SET_SIFS0_RATE = 0x1,
	SET_SIFS1_RATE,
	SET_SIFS2_RATE,
	SET_SIFS3_RATE,
	SET_SIFS4_RATE,
	SET_SIFS5_RATE,
	SET_SIFS6_RATE,
	SET_SIFS7_RATE,
	SET_SIFM0_RATE,
	SET_SIFM1_RATE,
	SET_SIFM2_RATE,
	SET_SIFM3_RATE,
	SET_SIFM4_RATE,
	SET_SIFM5_RATE,
	SET_SIFM6_RATE,
	SET_SIFM7_RATE,
	SET_SIFM8_RATE,
	SET_SIFM9_RATE,
	SET_SIFM10_RATE,
	SET_SIFM11_RATE,
	SET_SIFM12_RATE,
	SET_SIFM13_RATE,
	SET_SIFM14_RATE,
	SET_SIFM15_RATE,
	SET_SIFS0_FORMAT,
	SET_SIFS1_FORMAT,
	SET_SIFS2_FORMAT,
	SET_SIFS3_FORMAT,
	SET_SIFS4_FORMAT,
	SET_SIFS5_FORMAT,
	SET_SIFS6_FORMAT,
	SET_SIFS7_FORMAT,
	SET_SIFM0_FORMAT,
	SET_SIFM1_FORMAT,
	SET_SIFM2_FORMAT,
	SET_SIFM3_FORMAT,
	SET_SIFM4_FORMAT,
	SET_SIFM5_FORMAT,
	SET_SIFM6_FORMAT,
	SET_SIFM7_FORMAT,
	SET_SIFM8_FORMAT,
	SET_SIFM9_FORMAT,
	SET_SIFM10_FORMAT,
	SET_SIFM11_FORMAT,
	SET_SIFM12_FORMAT,
	SET_SIFM13_FORMAT,
	SET_SIFM14_FORMAT,
	SET_SIFM15_FORMAT,
	SET_ASRC_FACTOR_CP = 0x30,
	SET_ASRC_FACTOR_UAIF0,
	SET_ASRC_FACTOR_UAIF1,
	SET_ASRC_FACTOR_UAIF2,
	SET_ASRC_FACTOR_UAIF3,
	SET_ASRC_FACTOR_UAIF4,
	SET_ASRC_FACTOR_UAIF5,
	SET_ASRC_FACTOR_UAIF6,
	SET_ASRC_FACTOR_USB,
	SET_ASRC_FACTOR_BCLK_CP,
	SET_ROUTE_NSRC0 = 0x40,
	SET_ROUTE_NSRC1,
	SET_ROUTE_NSRC2,
	SET_ROUTE_NSRC3,
	SET_ROUTE_NSRC4,
	SET_ROUTE_NSRC5,
	SET_ROUTE_NSRC6,
	SET_ROUTE_NSRC7,
	SET_ROUTE_NSRC8,
	SET_ROUTE_NSRC9,
	SET_ROUTE_NSRC10,
	SET_ROUTE_NSRC11,
	SET_ROUTE_NSRC12,
	SET_ROUTE_NSRC13,
	SET_ROUTE_NSRC14,
	SET_ROUTE_NSRC15,
};

struct IPC_ABOX_CONFIG_MSG {
	enum ABOX_CONFIGMSG msgtype;
	int param1;
	int param2;
};

/******** ABOX_ASB_TEST IPC ***********************/
enum ABOX_TEST_MSG {
	ASB_MULTITX_SINGLERX = 1,
	ASB_SINGLETX_MULTIRX,
	ASB_MULTITX_MULTIRX,
	ASB_FFT_TEST,
};

struct IPC_ABOX_TEST_MSG {
	enum ABOX_TEST_MSG msgtype;
	int param1;
	int param2;
};

/******** IPC_ABOX_SYSTEM_MSG IPC ***********************/
enum ABOX_SYSTEM_MSG {
	ABOX_SUSPEND = 1,
	ABOX_RESUME,
	ABOX_BOOT_DONE,
	ABOX_CHANGE_GEAR,
	ABOX_START_L2C_CONTROL,
	ABOX_END_L2C_CONTROL,
	ABOX_REQUEST_SYSCLK,
	ABOX_REQUEST_L2C,
	ABOX_CHANGED_GEAR,
	ABOX_RELOAD_AREA,
	ABOX_REQUEST_LLC,
	ABOX_USING_LLC,
	ABOX_RECORD_GEAR,
	ABOX_REPORT_LOG = 0x10,
	ABOX_FLUSH_LOG,
	ABOX_REPORT_DUMP = 0x20,
	ABOX_REQUEST_DUMP,
	ABOX_FLUSH_DUMP,
	ABOX_TRANSFER_DUMP,
	ABOX_COPY_DUMP,
	ABOX_REQUEST_SRAM = 0x30,
	ABOX_READY_SRAM,
	ABOX_RELEASE_SRAM,
	ABOX_REQUEST_PCMC = 0x40,
	ABOX_READY_PCMC,
	ABOX_RELEASE_PCMC,
	ABOX_SET_MODE = 0x50,
	ABOX_SET_TYPE = 0x60,
	ABOX_START_VSS = 0xA0,
	ABOX_STOP_VSS,
	ABOX_RESET_VSS,
	ABOX_WATCHDOG_VSS,
	ABOX_VSS_DISABLED,
	ABOX_REPORT_COMPONENT = 0xC0,
	ABOX_UPDATE_COMPONENT_CONTROL,
	ABOX_REQUEST_COMPONENT_CONTROL,
	ABOX_REPORT_COMPONENT_CONTROL,
	ABOX_UPDATED_COMPONENT_CONTROL,
	ABOX_UPDATE_COMPONENT_VALUE,
	ABOX_REQUEST_DEBUG = 0xDE,
	ABOX_RECOVERY_ACTIVE = 0xDF,
	ABOX_RECOVER_OFFLOAD = 0xF0,
	ABOX_REPORT_FAULT = 0xFA,
	ABOX_UDMA_FLUSH = 0xFD,
	ABOX_REPORT_NOISE = 0xFE,
	ABOX_AP_SUSPEND = 0x101,
	ABOX_AP_RESUME,
	ABOX_PRINT_BUNDLE = 0xB000,
};

struct IPC_SYSTEM_MSG {
	enum ABOX_SYSTEM_MSG msgtype;
	int param1;
	int param2;
	int param3;
	union {
		int param_s32[SYSTEM_MSG_S32_SIZE];
		unsigned long long param_u64[SYSTEM_MSG_U64_SIZE];
		char param_bundle[SYSTEM_MSG_BUNDLE_SIZE];
	} bundle;
};

struct ABOX_LOG_BUFFER {
	unsigned int index_writer;
	unsigned int index_reader;
	unsigned int size;
	char buffer[];
};

enum ABOX_CONTROL_TYPE {
	ABOX_CONTROL_INT,
	ABOX_CONTROL_ENUM,
	ABOX_CONTROL_BYTE,
};

struct ABOX_COMPONENT_CONTROL {
	enum ABOX_CONTROL_TYPE type;
	unsigned int id;
	char name[16];
	unsigned int count;
	int min, max;
	union {
		unsigned int aaddr;
		unsigned long long kaddr;
		const char *addr;
	} texts; /* list of enumeration text delimited by ',' */
};

struct ABOX_COMPONENT_DESCRIPTIOR {
	unsigned int id;
	char name[16];
	unsigned int control_count;
	struct ABOX_COMPONENT_CONTROL controls[];
};

/************ IPC DEFINITION ***************/
/* Categorized IPC, */
enum IPC_ID {
	IPC_RECEIVED,
	IPC_SYSTEM = 1,
	IPC_PCMPLAYBACK,
	IPC_PCMCAPTURE,
	IPC_OFFLOAD,
	IPC_ERAP,
	IPC_USBPLAYBACK,
	IPC_USBCAPTURE,
	WDMA0_BUF_FULL = 0x8,
	WDMA1_BUF_FULL = 0x9,
	IPC_ABOX_CONFIG = 0xA,
	RDMA0_BUF_EMPTY = 0xB,
	RDMA1_BUF_EMPTY = 0xC,
	RDMA2_BUF_EMPTY = 0xD,
	RDMA3_BUF_EMPTY = 0xE,
	IPC_ASB_TEST = 0xF,
	IPC_ID_COUNT,
};

typedef struct {
	enum IPC_ID ipcid;
	int task_id;
	union IPC_MSG {
		struct IPC_SYSTEM_MSG system;
		struct IPC_PCMTASK_MSG pcmtask;
		struct IPC_OFFLOADTASK_MSG offload;
		struct IPC_ERAP_MSG erap;
		struct IPC_ABOX_CONFIG_MSG config;
		struct IPC_ABOX_TEST_MSG asb;
	} msg;
} ABOX_IPC_MSG;

struct abox2host_hndshk_tag {
	unsigned int suspend_wait_flag; /* boot init done */
	unsigned int hndShkFlag1;
	unsigned int hndShkFlag2;
	unsigned int hndShkFlag3;
	unsigned int hndShkFlag4;
	unsigned int hndShkFlag5;
	unsigned int hndShkFlag6;
	unsigned int hndShkFlag7;
};
#endif /* __ABOX_IPC_H */
