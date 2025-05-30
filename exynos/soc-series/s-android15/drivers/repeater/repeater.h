/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Header file for Exynos REPEATER driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REPEATER_H_
#define _REPEATER_H_

#define MAX_SHARED_BUFFER_NUM		3

struct repeater_info {
	int pixel_format;
	int width;
	int height;
	int buffer_count;
	int fps;
	int buf_fd[MAX_SHARED_BUFFER_NUM];
};

#define REPEATER_START_NORMAL 0
#define REPEATER_START_ONESHOT 1
#define REPEATER_ERR_BAD_STATUS 1
#define REPEATER_ERR_GET_CAPTURING_BUF 2
#define REPEATER_ERR_SET_CAPTURED_BUF 3
#define REPEATER_ERR_SET_ENCODING_DONE 4
#define REPEATER_ERR_GET_LATEST_CAPTURED_BUF_BUSY 5
#define REPEATER_ERR_GET_LATEST_CAPTURED_BUF_NO 6
#define REPEATER_ERR_REQ_BUFFER 7
#define REPEATER_ERR_NOTIFY 8

#define REPEATER_IOCTL_MAGIC		'R'

#define REPEATER_IOCTL_MAP_BUF		\
	_IOWR(REPEATER_IOCTL_MAGIC, 0x10, struct repeater_info)
#define REPEATER_IOCTL_UNMAP_BUF	\
	_IO(REPEATER_IOCTL_MAGIC, 0x11)

#define REPEATER_IOCTL_START		\
	_IOW(REPEATER_IOCTL_MAGIC, 0x20, int)
#define REPEATER_IOCTL_STOP		\
	_IO(REPEATER_IOCTL_MAGIC, 0x21)
#define REPEATER_IOCTL_PAUSE	\
	_IO(REPEATER_IOCTL_MAGIC, 0x22)
#define REPEATER_IOCTL_RESUME	\
	_IO(REPEATER_IOCTL_MAGIC, 0x23)

#define REPEATER_IOCTL_DUMP		\
	_IOR(REPEATER_IOCTL_MAGIC, 0x31, int)

#define REPEATER_IOCTL_SET_MAX_SKIPPED_FRAME	\
	_IOW(REPEATER_IOCTL_MAGIC, 0x40, int)
#define REPEATER_IOCTL_GET_STATUS	\
	_IOR(REPEATER_IOCTL_MAGIC, 0x41, int)

#define REPEATER_IOCTL_GET_LOG_INFO	\
	_IOR(REPEATER_IOCTL_MAGIC, 0x50, char *)

#endif /* _REPEATER_H_ */
