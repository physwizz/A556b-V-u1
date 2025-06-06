/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos CAMERA-PP GDC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CAMERAPP_GDC__H_
#define CAMERAPP_GDC__H_

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_media.h>
#include "videodev2_exynos_camera.h"
#include <linux/io.h>
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ctrls.h>
#if IS_ENABLED(CONFIG_VIDEOBUF2_DMA_SG)
#include <media/videobuf2-dma-sg.h>
#endif
#include "pablo-kernel-variant.h"
#include "pablo-mmio.h"
#include "pablo-mem.h"

#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_VOTF_USE_MFC_API)
#include "pablo-gdc-votf-mfc-api.h"
#else
#include "pablo-gdc-votf-gdc-api.h"
#endif

#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)
#include "pablo-gdc-hdr10p.h"
#endif

#if IS_ENABLED(CONFIG_EXYNOS_PM_QOS)
#define gdc_pm_qos_request exynos_pm_qos_request
#define gdc_pm_qos_add_request exynos_pm_qos_add_request
#define gdc_pm_qos_update_request exynos_pm_qos_update_request
#define gdc_pm_qos_remove_request exynos_pm_qos_remove_request
#else
#define gdc_pm_qos_request dev_pm_qos_request
#define gdc_pm_qos_add_request(arg...)
#define gdc_pm_qos_update_request(arg...)
#define gdc_pm_qos_remove_request(arg...)
#endif
/* #define ENABLE_GDC_FRAMEWISE_DISTORTION_CORRECTION */

int gdc_get_log_level(void);

/* Log without node identifier */
#define gdc_info(fmt, args...) pr_info("[%s:%d] " fmt, __func__, __LINE__, ##args)
#define gdc_dbg(fmt, args...)                                           \
	do {                                                                \
		if (gdc_get_log_level())                                        \
			pr_info("[%s:%d] " fmt, __func__, __LINE__, ##args);        \
	} while (0)

/* Log with node identifier */
#define gdc_dev_err(dev, fmt, args...) dev_err(dev, "[%s:%d] " fmt, __func__, __LINE__, ##args)
#define gdc_dev_info(dev, fmt, args...) dev_info(dev, "[%s:%d] " fmt, __func__, __LINE__, ##args)
#define gdc_dev_dbg(dev, fmt, args...)                                  \
	do {                                                                \
		if (gdc_get_log_level())                                        \
			dev_info(dev, "[%s:%d] " fmt, __func__, __LINE__, ##args);	\
	} while (0)

#define call_bufop(q, op, args...)                                      \
	({                                                                  \
		int ret = 0;                                                    \
		if (q && q->buf_ops && q->buf_ops->op)                          \
			ret = q->buf_ops->op(args);                                 \
		ret;                                                            \
	})

#define GDC_MODULE_NAME "pablo-gdc"

#if IS_ENABLED(CONFIG_ARCH_VELOCE_HYCON)
#define GDC_TIMEOUT msecs_to_jiffies(4000) /* x100 times */
#define GDC_WDT_CNT 15000 /* x100 times */
#define GDC_RESET_WAIT_CNT 1000000 /* x100 times */
#else
#define GDC_TIMEOUT msecs_to_jiffies(40) /* 40ms */
#define GDC_WDT_CNT 150
#define GDC_RESET_WAIT_CNT 10000
#endif

#define GDC_MAX_PLANES 3
#define GDC_MAX_BUFS (VIDEO_MAX_FRAME + 1)
#define GDC_META_PLANE 1

/* GDC hardware device state */
#define DEV_RUN 1
#define DEV_SUSPEND 2

/* GDC context state */
#define CTX_PARAMS 1
#define CTX_STREAMING 2
#define CTX_RUN 3
#define CTX_ABORT 4
#define CTX_SRC_FMT 5
#define CTX_DST_FMT 6
#define CTX_INT_FRAME 7 /* intermediate frame available */

/* GDC grid size (x, y) */
#if defined(CONFIG_CAMERA_PP_GDC_V1_0_0_OBJ) || defined(CONFIG_CAMERA_PP_GDC_V2_0_0_OBJ)
#define GRID_X_SIZE 9
#define GRID_Y_SIZE 7
#else
#define GRID_X_SIZE 33
#define GRID_Y_SIZE 33
#endif

#define fh_to_gdc_ctx(__fh) container_of(__fh, struct gdc_ctx, fh)
#define gdc_fmt_is_rgb888(x) ((x == V4L2_PIX_FMT_RGB32) || (x == V4L2_PIX_FMT_BGR32))
#define gdc_fmt_is_yuv422(x)                                                                 \
	((x == V4L2_PIX_FMT_YUYV) || (x == V4L2_PIX_FMT_UYVY) || (x == V4L2_PIX_FMT_YVYU) ||     \
	 (x == V4L2_PIX_FMT_YUV422P) || (x == V4L2_PIX_FMT_NV16) || (x == V4L2_PIX_FMT_NV61))
#define gdc_fmt_is_yuv420(x)                                                                 \
	((x == V4L2_PIX_FMT_YUV420) || (x == V4L2_PIX_FMT_YVU420) || (x == V4L2_PIX_FMT_NV12) || \
	 (x == V4L2_PIX_FMT_NV21) || (x == V4L2_PIX_FMT_NV12M) || (x == V4L2_PIX_FMT_NV21M) ||   \
	 (x == V4L2_PIX_FMT_YUV420M) || (x == V4L2_PIX_FMT_YVU420M) ||                           \
	 (x == V4L2_PIX_FMT_NV12MT_16X16))
#define gdc_fmt_is_ayv12(x) ((x) == V4L2_PIX_FMT_YVU420)
#define gdc_fmt_is_gray(x) ((x) == V4L2_PIX_FMT_GREY)

#define GDC_SCENARIO_MASK 0xF0000000
#define GDC_SCENARIO_SHIFT 28

#define GDC_LOSSY_COMP_RATE_40 40
#define GDC_LOSSY_COMP_RATE_50 50
#define GDC_LOSSY_COMP_RATE_60 60

/*
 * Get index of crop_param
 * idx 0: default
 * idx 1~ 64: multi-buffer
 */
#define GDC_BUF_IDX_TO_PARAM_IDX(x) \
	((((x) + 1) < GDC_MAX_BUFS) ? ((x) + 1) : (((x) % (GDC_MAX_BUFS - 1)) + 1))

enum gdc_clk_status {
	GDC_CLK_ON,
	GDC_CLK_OFF,
};

enum gdc_clocks { GDC_GATE_CLK, GDC_CHLD_CLK, GDC_PARN_CLK };

enum gdc_scenario {
	GDC_SCEN_NORMAL,
	GDC_SCEN_VOTF,
};

enum gdc_votf_addr {
	GDC_SRC_ADDR,
	GDC_DST_ADDR,
	GDC_VOTF_ADDR_MAX,
};

enum gdc_buf_check {
	GDC_NO_INO = -1,
	GDC_GOOD_INO = 0,
	GDC_DROP_INO = 1,
};

enum gdc_out_mode {
	GDC_OUT_M2M,
	GDC_OUT_VOTF,
	GDC_OUT_OTF,
	GDC_OUT_NONE,
};

enum gdc_buf_plane {
	GDC_PLANE_LUMA,
	GDC_PLANE_CHROMA,
};

enum gdc_sbwc_size_type {
	GDC_SBWC_SIZE_PAYLOAD,
	GDC_SBWC_SIZE_HEADER,
	GDC_SBWC_SIZE_ALL,
};

enum gdc_dbg_mode {
	GDC_DBG_DUMP_REG = 0,
	GDC_DBG_DUMP_REG_ONCE = 1,
	GDC_DBG_DUMP_S2D = 2,
	GDC_DBG_TIME = 3,
	GDC_DBG_PMIO_MODE = 4,
	GDC_DBG_DUMP_PMIO_CACHE = 5,
};

enum gdc_codec_type {
	GDC_CODEC_TYPE_NONE = 0,
	GDC_CODEC_TYPE_HEVC = 1,
	GDC_CODEC_TYPE_H264 = 2,
};

/*
 * struct gdc_size_limit - GDC variant size information
 *
 * @min_w: minimum pixel width size
 * @min_h: minimum pixel height size
 * @max_w: maximum pixel width size
 * @max_h: maximum pixel height size
 */
struct gdc_size_limit {
	u16 min_w;
	u16 min_h;
	u16 max_w;
	u16 max_h;
};

struct gdc_variant {
	struct gdc_size_limit limit_input;
	struct gdc_size_limit limit_output;
	u32 version;
};

/*
 * struct gdc_fmt - the driver's internal color format data
 * @name: format description
 * @pixelformat: the fourcc code for this format, 0 if not applicable
 * @num_planes: number of physically non-contiguous data planes
 * @num_comp: number of color components(ex. RGB, Y, Cb, Cr)
 * @h_div: horizontal division value of C against Y for crop
 * @v_div: vertical division value of C against Y for crop
 * @bitperpixel: bits per pixel
 * @color: the corresponding gdc_color_fmt
 */
struct gdc_fmt {
	char *name;
	u32 pixelformat;
	u8 bitperpixel[GDC_MAX_PLANES];
	u8 num_planes : 2; /* num of buffer */
	u8 num_comp : 2; /* num of hw_plane */
	u8 h_shift : 1;
	u8 v_shift : 1;
};

struct gdc_addr {
	dma_addr_t y;
	dma_addr_t cb;
	dma_addr_t cr;
	unsigned int ysize;
	unsigned int cbsize;
	unsigned int crsize;
	dma_addr_t y_2bit;
	dma_addr_t cbcr_2bit;
	unsigned int ysize_2bit;
	unsigned int cbcrsize_2bit;
};

/*
 * struct gdc_frame - source/target frame properties
 * @fmt:	buffer format(like virtual screen)
 * @crop:	image size / position
 * @addr:	buffer start address(access using GDC_ADDR_XXX)
 * @bytesused:	image size in bytes (w x h x bpp)
 */
struct gdc_frame {
	const struct gdc_fmt *gdc_fmt;
	unsigned short width;
	unsigned short height;
	unsigned short stride;
	__u32 pixelformat;
	struct gdc_addr addr;
	__u32 bytesused[GDC_MAX_PLANES];
	__u32 pixel_size;
	__u32 extra;
	__u32 num_planes;
	int comp_rate;
	u8 io_mode;
};

/*
 * struct gdc_m2m_device - v4l2 memory-to-memory device data
 * @v4l2_dev: v4l2 device
 * @vfd: the video device node
 * @m2m_dev: v4l2 memory-to-memory device data
 * @in_use: the open count
 */
struct gdc_m2m_device {
	struct v4l2_device v4l2_dev;
	struct video_device *vfd;
	struct v4l2_m2m_dev *m2m_dev;
	atomic_t in_use;
	struct mutex lock;
};

struct gdc_wdt {
	struct timer_list timer;
	atomic_t cnt;
};

/*
 * gdc_crop_param - the crop and grid information from user
 * @calculated_grid_x: x-axis grid data
 * @calculated_grid_y: y-axis grid data
 * @buf_index:	index of v4l2 buffer to be applied this param information
 */
struct gdc_crop_param {
	u32 sensor_num;
	u32 sensor_width;
	u32 sensor_height;
	u32 crop_start_x;
	u32 crop_start_y;
	u32 crop_width;
	u32 crop_height;
	bool is_crop_dzoom;
	bool is_scaled;
	bool use_calculated_grid;
	int calculated_grid_x[GRID_Y_SIZE][GRID_X_SIZE];
	int calculated_grid_y[GRID_Y_SIZE][GRID_X_SIZE];
	u32 src_bytesperline[GDC_MAX_PLANES];
	u32 dst_bytesperline[GDC_MAX_PLANES];
	u32 buf_index;
	bool votf_en; /* Deprecated from v11.0 */
	bool is_grid_mode;
	bool is_bypass_mode;
	u8 out_mode;
	u8 codec_type;
	int reserved[23];
};

struct gdc_time_dbg {
	ktime_t shot_time;
	u64 shot_time_stamp;
	ktime_t hw_time;
	u64 hw_time_stamp;
};

/*
 * gdc_ctx - the abstration for GDC open context
 * @node:		list to be added to gdc_dev.context_list
 * @gdc_dev:		the GDC device this context applies to
 * @m2m_ctx:		memory-to-memory device context
 * @frame:		source frame properties
 * @fh:			v4l2 file handle
 * @flip_rot_cfg:	rotation and flip configuration
 * @bl_op:		image blend mode
 * @dith:		image dithering mode
 * @g_alpha:		global alpha value
 * @color_fill:		enable color fill
 * @flags:		context state flags
 * @crop_param:		crop and grid information from user
 * @cur_index:		current array index of @crop_param
 */
struct gdc_ctx {
	struct list_head node;
	struct gdc_dev *gdc_dev;
	struct v4l2_m2m_ctx *m2m_ctx;
	struct gdc_frame s_frame;
	struct gdc_frame d_frame;
	struct v4l2_fh fh;
	struct gdc_time_dbg time_dbg;
	unsigned long flags;
	struct gdc_crop_param crop_param[GDC_MAX_BUFS];
	u32 cur_index;
#if IS_ENABLED(CONFIG_CAMERA_PP_GDC_HAS_HDR10P)
	struct gdc_hdr10p_ctx hdr10p_ctx[GDC_MAX_BUFS];
#endif
	bool use_mfc_votf_ops;
};

struct gdc_priv_buf {
	dma_addr_t daddr;
	void *vaddr;
	size_t size;
};

/*
 * struct gdc_dev - the abstraction for GDC device
 * @dev:	pointer to the GDC device
 * @variant:	the IP variant information
 * @m2m:	memory-to-memory V4L2 device information
 * @aclk:	aclk required for gdc operation
 * @pclk:	pclk required for gdc operation
 * @clk_chld:	child clk of mux required for gdc operation
 * @clk_parn:	parent clk of mux required for gdc operation
 * @regs_base:	the mapped hardware registers
 * @regs_rsc:	the resource claimed for IO registers
 * @wait:	interrupt handler waitqueue
 * @ws:		work struct
 * @state:	device state flags
 * @alloc_ctx:	videobuf2 memory allocator context
 * @slock:	the spinlock pscecting this data structure
 * @lock:	the mutex pscecting this data structure
 * @wdt:	watchdog timer information
 * @version:	IP version number
 * @cfw:	cfw flag
 * @pb_disable:	prefetch-buffer disable flag
 */
struct gdc_dev {
	struct device *dev;
	const struct gdc_variant *variant;
	struct gdc_m2m_device m2m;
	struct clk *aclk;
	struct clk *pclk;
	struct clk *clk_chld;
	struct clk *clk_parn;
	void __iomem *regs_base;
	void __iomem *votf_base;
	void __iomem *mfc_votf_base;
	struct resource *regs_rsc;
	struct workqueue_struct *qogdc_int_wq;
	wait_queue_head_t wait;
	unsigned long state;
	struct vb2_alloc_ctx *alloc_ctx;
	spinlock_t slock;
	struct mutex lock;
	struct gdc_wdt wdt;
	spinlock_t ctxlist_lock;
	struct gdc_ctx *current_ctx;
	struct gdc_ctx *votf_ctx;
	struct list_head context_list; /* for gdc_ctx_abs.node */
	atomic_t wait_mfc;
	unsigned int irq0, irq1;
	int dev_id;
	struct mfc_votf_info mfc_votf;
	u32 version;
	u32 votf_src_dest[2];
	u32 has_votf_mfc;
	bool has_hdr10p;
	u8 stalled;

	bool use_cloader_iommu_group;
	u32 cloader_iommu_group_id;

	struct is_mem mem;
	struct pmio_config pmio_config;
	struct pablo_mmio *pmio;
	struct pmio_field *pmio_fields;
	bool pmio_en;

	struct is_priv_buf *pb_c_loader_payload;
	unsigned long kva_c_loader_payload;
	dma_addr_t dva_c_loader_payload;
	struct is_priv_buf *pb_c_loader_header;
	unsigned long kva_c_loader_header;
	dma_addr_t dva_c_loader_header;
	struct pablo_gdc_v4l2_ops *v4l2_ops;
	struct pablo_gdc_sys_ops *sys_ops;
	struct pablo_camerapp_hw_gdc *hw_gdc_ops;
};

void gdc_fill_curr_frame(struct gdc_dev *gdc, struct gdc_ctx *ctx);
int gdc_add_context_and_run(struct gdc_dev *gdc, struct gdc_ctx *ctx);
void gdc_job_finish(struct gdc_dev *gdc, struct gdc_ctx *ctx);

static inline struct gdc_frame *ctx_get_frame(struct gdc_ctx *ctx, enum v4l2_buf_type type)
{
	struct gdc_frame *frame;

	if (V4L2_TYPE_IS_MULTIPLANAR(type)) {
		if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
			frame = &ctx->s_frame;
		else
			frame = &ctx->d_frame;
	} else {
		gdc_dev_err(ctx->gdc_dev->dev, "Wrong V4L2 buffer type %d\n", type);
		return ERR_PTR(-EINVAL);
	}

	return frame;
}

extern void is_debug_s2d(bool en_s2d, const char *fmt, ...);

#if IS_ENABLED(CONFIG_VIDEOBUF2_DMA_SG)
#if PKV_VER_GE(4, 20, 0)
static inline dma_addr_t gdc_get_dma_address(struct vb2_buffer *vb2_buf, u32 plane)
{
	struct sg_table *sgt;

	sgt = vb2_dma_sg_plane_desc(vb2_buf, plane);

	if (!sgt)
		return 0;

	return (dma_addr_t)sg_dma_address(sgt->sgl);
}

static inline void *gdc_get_kvaddr(struct vb2_buffer *vb2_buf, u32 plane)
{
	return vb2_plane_vaddr(vb2_buf, plane);
}
#else
static inline dma_addr_t gdc_get_dma_address(struct vb2_buffer *vb2_buf, u32 plane)
{
	return vb2_dma_sg_plane_dma_addr(vb2_buf, plane);
}

static inline void *gdc_get_kvaddr(struct vb2_buffer *vb2_buf, u32 plane)
{
	return vb2_plane_vaddr(vb2_buf, plane);
}
#endif
#else
static inline dma_addr_t gdc_get_dma_address(void *cookie, dma_addr_t *addr)
{
	return NULL;
}

static inline void *gdc_get_kernel_address(void *cookie)
{
	return NULL;
}
#endif

struct pablo_gdc_v4l2_ops {
	int (*m2m_streamon)(struct file *file, struct v4l2_m2m_ctx *m2m_ctx,
			    enum v4l2_buf_type type);
	int (*m2m_streamoff)(struct file *file, struct v4l2_m2m_ctx *m2m_ctx,
			     enum v4l2_buf_type type);
};

struct pablo_gdc_sys_ops {
	unsigned long (*copy_from_user)(void *dst, const void *src, unsigned long size);
};

#if IS_ENABLED(CONFIG_PABLO_KUNIT_TEST)
#define KUNIT_EXPORT_SYMBOL(x) EXPORT_SYMBOL_GPL(x)
struct pkt_gdc_ops {
	int (*get_log_level)(void);
	void (*set_log_level)(int level);
	int (*get_debug)(char *buffer, const struct kernel_param *kp);
	int (*votf_set_service_cfg)(int txs, struct gdc_dev *gdc);
	int (*votfitf_set_flush)(struct gdc_dev *gdc);
	const struct gdc_fmt *(*find_format)(struct gdc_dev *gdc, u32 pixfmt, bool output_buf);
	int (*v4l2_querycap)(struct file *file, void *fh, struct v4l2_capability *cap);
	int (*v4l2_g_fmt_mplane)(struct file *file, void *fh, struct v4l2_format *f);
	int (*v4l2_try_fmt_mplane)(struct file *file, void *fh, struct v4l2_format *f);
	int (*image_bound_check)(struct gdc_ctx *ctx, enum v4l2_buf_type type,
				 struct v4l2_pix_format_mplane *pixm);
	int (*v4l2_s_fmt_mplane)(struct file *file, void *fh, struct v4l2_format *f);
	int (*v4l2_reqbufs)(struct file *file, void *fh, struct v4l2_requestbuffers *reqbufs);
	int (*v4l2_querybuf)(struct file *file, void *fh, struct v4l2_buffer *buf);
	int (*vb2_qbuf)(struct vb2_queue *q, struct v4l2_buffer *b);
	int (*check_qbuf)(struct file *file, struct v4l2_m2m_ctx *m2m_ctx, struct v4l2_buffer *buf);
	int (*v4l2_qbuf)(struct file *file, void *fh, struct v4l2_buffer *buf);
	int (*v4l2_dqbuf)(struct file *file, void *fh, struct v4l2_buffer *buf);
	int (*power_clk_enable)(struct gdc_dev *gdc);
	void (*power_clk_disable)(struct gdc_dev *gdc);
	int (*v4l2_streamon)(struct file *file, void *fh, enum v4l2_buf_type type);
	int (*v4l2_streamoff)(struct file *file, void *fh, enum v4l2_buf_type type);
	int (*v4l2_s_ctrl)(struct file *file, void *priv, struct v4l2_control *ctrl);
	int (*v4l2_s_ext_ctrls)(struct file *file, void *priv, struct v4l2_ext_controls *ctrls);
	struct v4l2_m2m_dev *(*v4l2_m2m_init)(void);
	void (*v4l2_m2m_release)(struct v4l2_m2m_dev *m2m_dev);
	int (*ctx_stop_req)(struct gdc_ctx *ctx);
	int (*vb2_queue_setup)(struct vb2_queue *vq, unsigned int *num_buffers,
			       unsigned int *num_planes, unsigned int sizes[],
			       struct device *alloc_devs[]);
	int (*vb2_buf_prepare)(struct vb2_buffer *vb);
	void (*vb2_buf_finish)(struct vb2_buffer *vb);
	void (*vb2_buf_queue)(struct vb2_buffer *vb);
	void (*vb2_lock)(struct vb2_queue *vq);
	void (*vb2_unlock)(struct vb2_queue *vq);
	void (*cleanup_queue)(struct gdc_ctx *ctx);
	int (*vb2_start_streaming)(struct vb2_queue *vq, unsigned int count);
	void (*vb2_stop_streaming)(struct vb2_queue *vq);
	int (*queue_init)(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq);
	int (*pmio_init)(struct gdc_dev *gdc);
	void (*pmio_exit)(struct gdc_dev *gdc);
	void (*pmio_config)(struct gdc_dev *gdc, struct c_loader_buffer *clb);
	int (*run_next_job)(struct gdc_dev *gdc);
	int (*add_context_and_run)(struct gdc_dev *gdc, struct gdc_ctx *ctx);
	void (*m2m_device_run)(void *priv);
	unsigned int (*poll)(struct file *file, struct poll_table_struct *wait);
	int (*mmap)(struct file *file, struct vm_area_struct *vma);
	void (*m2m_job_abort)(void *priv);
	int (*clk_get)(struct gdc_dev *gdc);
	void (*clk_put)(struct gdc_dev *gdc);
	int (*sysmmu_fault_handler)(struct iommu_fault *fault, void *data);
	void (*shutdown)(struct platform_device *pdev);
	int (*suspend)(struct device *dev);
	int (*runtime_resume)(struct device *dev);
	int (*runtime_suspend)(struct device *dev);
	int (*alloc_pmio_mem)(struct gdc_dev *gdc);
	void (*free_pmio_mem)(struct gdc_dev *gdc);
	void (*job_finish)(struct gdc_dev *gdc, struct gdc_ctx *ctx);
	int (*register_m2m_device)(struct gdc_dev *gdc, int dev_id);
	void (*init_frame)(struct gdc_frame *frame);
	void (*get_bufaddr_8_plus_2_fmt)(struct gdc_frame *frame, unsigned int w, unsigned int h);
	int (*get_bufaddr_frame_fmt)(struct gdc_frame *frame, unsigned int pixsize,
				     unsigned int bytesize, struct vb2_buffer *vb2buf);
	int (*get_bufaddr)(struct gdc_dev *gdc, struct gdc_ctx *ctx, struct vb2_buffer *vb2buf,
			   struct gdc_frame *frame);
};
struct pkt_gdc_ops *pablo_kunit_get_gdc(void);
ulong pablo_get_dbg_gdc(void);
void pablo_set_dbg_gdc(ulong dbg);
#else
#define KUNIT_EXPORT_SYMBOL(x)
#endif
#endif /* CAMERAPP_GDC__H_ */
