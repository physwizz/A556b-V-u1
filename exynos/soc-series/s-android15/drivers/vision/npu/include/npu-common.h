#ifndef _NPU_COMMON_H_
#define _NPU_COMMON_H_

#include <linux/types.h>
#include <linux/time.h>
#include <linux/wait.h>

#include "vs4l.h"
#include "npu-errno.h"
#include "ncp_header.h"

/* constants for Magic sign */
#define NPU_NW_MAGIC_TAIL	0x17AA0103
#define NPU_FRAME_MAGIC_TAIL	0XF8A1772E

#define NPU_FRAME_MAX_CONTAINERLIST	256
#define NPU_FRAME_MAX_BUFFER		32
#define NPU_FRAME_QBUF		0xFA36
#define NPU_FRAME_DQBUF		0x36FA
#define NPU_MAX_FW_VERSION_LEN       200
#define NPU_MAX_FW_HASH_LEN       1000

/* Need to be replaced in the future */
typedef u32 npu_uid_t;
typedef u32 npu_req_id_t;

/* Function pointer definition of callback of nw request */
struct npu_session;
struct nw_result;
typedef int (*save_result_func)(struct npu_session *, struct nw_result);

/* Definition of time out - currently not defined yet */
struct npu_time_constraints {
	u64	timestamp;
};

/* Forward declaration */
struct npu_session;

/* Enumeration of commands */
typedef enum {
	NPU_NW_CMD_BASE = 4096,
	NPU_NW_CMD_LOAD,
	NPU_NW_CMD_UNLOAD,
	NPU_NW_CMD_STREAMOFF,
	NPU_NW_CMD_POWER_CTL,
	NPU_NW_CMD_PROFILE_START,
	NPU_NW_CMD_PROFILE_STOP,
	NPU_NW_CMD_FW_TC_EXECUTE,
	NPU_NW_CMD_CORE_CTL,
	NPU_NW_CMD_CLEAR_CB,
	NPU_NW_CMD_MODE,
	NPU_NW_CMD_IMB_SIZE,
	NPU_NW_CMD_SUSPEND,
	NPU_NW_CMD_BUF_VOTF,
	NPU_NW_CMD_END,
} nw_cmd_e;

typedef enum {
	NPU_FRAME_CMD_BASE = 32,
	NPU_FRAME_CMD_Q,
	NPU_FRAME_CMD_Q_CANCEL,
	NPU_FRAME_CMD_END,
} frame_cmd_e;

struct addr_info {
	u32 memory_type;
	u32 av_index;
	void *vaddr;
	dma_addr_t daddr;
	size_t size;
	u32 pixel_format;
	u32 width;
	u32 height;
	u32 channels;
	u32 stride;
	u32 cstride;
};

enum session_buf_state {
	SS_BUF_STATE_UNPREPARE,
	SS_BUF_STATE_PREPARE,
	SS_BUF_STATE_QUEUE,
	SS_BUF_STATE_PROCESS,
	SS_BUF_STATE_DEQUEUE,
	SS_BUF_STATE_DONE,
	SS_BUF_STATE_INVALID,
};

struct av_info {
	int IOFM_buf_idx;
	enum session_buf_state state;
	size_t address_vector_cnt;
	struct addr_info *addr_info;
};

struct npu_nw {
	/* Common part */
	npu_uid_t		uid;
	npu_req_id_t		npu_req_id;
	npu_errno_t		result_code;
	struct npu_session *session;
	int priority;
	int deadline;
	u32 preference;

	/* Session reference list */
	struct list_head	sess_ref_list;

	/* Network mgmt. only part */
	nw_cmd_e		cmd;

	/* npu nw scheduling param boundness, Set during CMD LOAD*/
	u32			bound_id;

	/* Function pointer definition of callback of nw request */
	save_result_func	notify_func;

	u32			param0;
	u32			param1;
	u32			reserved[4];
	u32			magic_tail;
	struct addr_info ncp_addr;
	int msgid;
	int result_value;
};

struct mbox_process_dat {
	u32 address_vector_cnt;
	u32 address_vector_start_daddr;
};

#if 1 // POWER_DOWN
struct system_pwr {
	wait_queue_head_t wq;
	struct system_result system_result;
};
#endif

struct npu_frame {
	/* Common part */
	npu_uid_t		uid;
	u32			frame_id;
	npu_req_id_t		npu_req_id;
	npu_errno_t		result_code;
	struct npu_session *session;
	struct av_info *IFM_info;
	struct av_info *OFM_info;
	struct av_info *av_info;

	/* Frame only part */
	frame_cmd_e		cmd;

	int			priority;
	struct npu_queue	*src_queue;
	struct npu_queue_list *input;
	struct npu_queue_list *output;
	u32			magic_tail;
	struct mbox_process_dat mbox_process_dat;
	int msgid;
	u32 duration;
	u32 ncp_type;
	u32 request_cancel;
};

struct nw_result {
	npu_errno_t	result_code;
	struct npu_nw	nw;
};

/* Return value of S_PARAM handler */
typedef enum {
	S_PARAM_NOMB = 0,	/* None of my business. Pass to next handler */
	S_PARAM_HANDLED,
	S_PARAM_ERROR,
} npu_s_param_ret;

/* Structure to keep iomem mapped area */
struct npu_iomem_area {
	void __iomem            *vaddr;
	u32                     paddr;
	resource_size_t         size;
};

/* Structure for npu nw scheduling parameter */
struct npu_nw_sched_param {
	u32			priority;
	u32			bound_id;
	u32			max_npu_core;
	u32			deadline;
};

/*For the boundness of nw*/
enum npu_nw_priority_val {
		NPU_PRIORITY_MIN_VAL = 0,
		NPU_PRIORITY_MID_PRIO = 65,
		NPU_PRIORITY_PREMPT_MIN = 128,
		NPU_PRIORITY_MAX_VAL = 255,
};

/*For the boundness of nw*/
enum npu_nw_boundness {
		NPU_BOUND_CORE0 = 0,
		NPU_BOUND_UNBOUND = 0xFFFFFFFF,
};

/*For the deadline of nw*/
enum npu_nw_deadline {
		NPU_MIN_DEADLINE = 0,
		NPU_MAX_DEADLINE = 0xFFFFFFFF,
};

/*
 * Convinient macros
 */

/* Used for clean-up routines */
#define BIT_CHECK_AND_EXECUTE(BIT, VAR_R, DESC, CODE)           \
do {                                                            \
	const char *__desc = DESC;                              \
	if (test_bit((BIT), (VAR_R))) {                         \
		if (__desc)                                     \
			npu_info("%s started.\n", __desc);      \
		CODE                                            \
	} else {                                                \
		if (__desc)                                     \
			npu_info("%s skipped.\n", __desc);      \
	}                                                       \
	clear_bit((BIT), (VAR_R));                              \
} while (0)

/* ALIGN base for intermediate buffer offset */
#define NPU_IMB_ALIGN		64
#if IS_ENABLED(CONFIG_NPU_USE_IMB_ALLOCATOR)
#define NPU_IMB_CHUNK_SIZE	0x1000000	/* 16 Mb size of chunk */
#define NPU_IMB_CHUNK_MAX_NUM	64		/* 1024 Mb in total, for PAMIR max available physical space 1920 Mb */
#define NPU_IMB_GRANULARITY	0x2000000	/* 32 Mb */
#endif

#endif	/* _NPU_COMMON_H_ */
