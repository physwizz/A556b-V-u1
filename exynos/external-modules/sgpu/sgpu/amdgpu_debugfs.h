/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 * Copyright 2009 Jerome Glisse.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
/*
 * Debugfs
 */

#include <linux/completion.h>

struct amdgpu_device;
struct drm_minor;

struct amdgpu_debugfs {
	const struct drm_info_list	*files;
	unsigned		num_files;
};

struct amdgpu_autodump {
	struct completion		dumping;
	struct wait_queue_head		gpu_hang;
};

int amdgpu_debugfs_regs_init(struct amdgpu_device *adev);
void amdgpu_debugfs_init(struct drm_minor *minor);
void amdgpu_debugfs_fini(struct amdgpu_device *adev);
int amdgpu_debugfs_add_files(struct amdgpu_device *adev,
			     const struct drm_info_list *files,
			     unsigned nfiles);
int amdgpu_debugfs_fence_init(struct amdgpu_device *adev);
int amdgpu_debugfs_firmware_init(struct amdgpu_device *adev);
int amdgpu_debugfs_gem_init(struct amdgpu_device *adev);
int amdgpu_debugfs_wait_dump(struct amdgpu_device *adev);
int sgpu_debugfs_bpmd_init(struct amdgpu_device *sdev);
void sgpu_debugfs_bpmd_cleanup(struct amdgpu_device *sdev);
