
#ifndef _NPU_VER_H_
#define _NPU_VER_H_

#include "include/vs4l.h"

#define NPU_DD_MAJOR_VERSION		15
#define NPU_DD_MINOR_VERSION		16
#define NPU_DD_SYNC_VERSION		5
#define MAX_FW_VERSION_LEN       128

struct npu_ver {
	char	*driver_git;
	char	*driver_verion;
	char	*signature;
	__u32	ncp_version;
	__u32	mailbox_version;
	__u32	cmd_version;
	__u32	api_version;
	char	*fw_version;
};

int npu_ver_probe(struct npu_device *npu_device);
int npu_ver_release(struct npu_device *npu_device);
int npu_ver_info(struct npu_device *npu_device, struct vs4l_version *version);
int npu_ver_fw_info(struct npu_device *npu_device, struct vs4l_version *version);
void npu_ver_dump(struct npu_device *npu_device);

#endif
