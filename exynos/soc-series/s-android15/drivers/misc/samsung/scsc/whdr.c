/****************************************************************************
 *
 * Copyright (c) 2014 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#include <linux/kernel.h>
#include <scsc/scsc_logring.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "fwimage.h"
#include "whdr.h"

/*
 * The Maxwell Firmware Header Format is defined in SC-505846-SW
 */

#define FWHDR_02_TRAMPOLINE_OFFSET 0
#define FWHDR_02_MAGIC_OFFSET 8
#define FWHDR_02_VERSION_MINOR_OFFSET 12
#define FWHDR_02_VERSION_MAJOR_OFFSET 14
#define FWHDR_02_LENGTH_OFFSET 16
#define FWHDR_02_FIRMWARE_API_VERSION_MINOR_OFFSET 20
#define FWHDR_02_FIRMWARE_API_VERSION_MAJOR_OFFSET 22
#define FWHDR_02_FIRMWARE_CRC_OFFSET 24
#define FWHDR_02_CONST_FW_LENGTH_OFFSET 28
#define FWHDR_02_CONST_CRC_OFFSET 32
#define FWHDR_02_FIRMWARE_RUNTIME_LENGTH_OFFSET 36
#define FWHDR_02_FIRMWARE_ENTRY_POINT_OFFSET 40
#define FWHDR_02_BUILD_ID_OFFSET 48
#define FWHDR_02_R4_PANIC_RECORD_OFFSET_OFFSET 176
#define FWHDR_02_M4_PANIC_RECORD_OFFSET_OFFSET 180
#define FWHDR_02_TTID_OFFSET 184
#define FWHDR_02_WHDR_RECORD_OFFSET_OFFSET 424

/*
 * Firmware header format for version 1.0 is same as version for 0.2
 */
#define FWHDR_02_TRAMPOLINE(__fw) (*((u32 *)(__fw + FWHDR_02_TRAMPOLINE_OFFSET)))
#define FWHDR_02_HEADER_FIRMWARE_ENTRY_POINT(__fw) (*((u32 *)(__fw + FWHDR_02_FIRMWARE_ENTRY_POINT_OFFSET)))
#define FWHDR_02_HEADER_FIRMWARE_RUNTIME_LENGTH(__fw) (*((u32 *)(__fw + FWHDR_02_FIRMWARE_RUNTIME_LENGTH_OFFSET)))
#define FWHDR_02_HEADER_BUILD_ID_OFFSET(__fw) (((char *)(__fw + FWHDR_02_BUILD_ID_OFFSET)))
#define FWHDR_02_HEADER_TTID_OFFSET(__fw) (((char *)(__fw + FWHDR_02_TTID_OFFSET)))
#define FWHDR_02_HEADER_VERSION_MAJOR(__fw) (*((u16 *)(__fw + FWHDR_02_VERSION_MAJOR_OFFSET)))
#define FWHDR_02_HEADER_VERSION_MINOR(__fw) (*((u16 *)(__fw + FWHDR_02_VERSION_MINOR_OFFSET)))
#define FWHDR_02_HEADER_FIRMWARE_API_VERSION_MINOR(__fw) (*((u16 *)(__fw + FWHDR_02_FIRMWARE_API_VERSION_MINOR_OFFSET)))
#define FWHDR_02_HEADER_FIRMWARE_API_VERSION_MAJOR(__fw) (*((u16 *)(__fw + FWHDR_02_FIRMWARE_API_VERSION_MAJOR_OFFSET)))
#define FWHDR_02_FW_CRC32(__fw) (*((u32 *)(__fw + FWHDR_02_FIRMWARE_CRC_OFFSET)))
#define FWHDR_02_HDR_LENGTH(__fw) (*((u32 *)(__fw + FWHDR_02_LENGTH_OFFSET)))
#define FWHDR_02_HEADER_CRC32(__fw) (*((u32 *)(__fw + (FWHDR_02_HDR_LENGTH(__fw)) - sizeof(u32))))
#define FWHDR_02_CONST_CRC32(__fw) (*((u32 *)(__fw + FWHDR_02_CONST_CRC_OFFSET)))
#define FWHDR_02_CONST_FW_LENGTH(__fw) (*((u32 *)(__fw + FWHDR_02_CONST_FW_LENGTH_OFFSET)))
#define FWHDR_02_R4_PANIC_RECORD_OFFSET(__fw) (*((u32 *)(__fw + FWHDR_02_R4_PANIC_RECORD_OFFSET_OFFSET)))
#define FWHDR_02_M4_PANIC_RECORD_OFFSET(__fw) (*((u32 *)(__fw + FWHDR_02_M4_PANIC_RECORD_OFFSET_OFFSET)))
#define FWHDR_02_WHDR_OFFSET(__fw) (*((u32 *)(__fw + FWHDR_02_WHDR_RECORD_OFFSET_OFFSET)))

/* firmware header has a panic record if the firmware header length is at least 192 bytes long */
#define MIN_HEADER_LENGTH_WITH_PANIC_RECORD 188

#define FWHDR_MAGIC_STRING "smxf"

#define whdr_from_fwhdr_if(FWHDR_IF_PTR) container_of(FWHDR_IF_PTR, struct whdr, fw_if)

static int crc_check_period_ms = 30000;

static bool crc_allow_none = true;
module_param(crc_allow_none, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(crc_allow_none, "Allow skipping firmware CRC checks if CRC is not present");

const void *whdr_lookup_tag(const void *blob, enum whdr_tag tag, uint32_t *length);


struct whdr {
	struct fwhdr_if fw_if;
	u16 hdr_major;
	u16 hdr_minor;

	u16 fwapi_major;
	u16 fwapi_minor;

	u32 firmware_entry_point;
	u32 fw_runtime_length;

	u32 fw_crc32;
	u32 const_crc32;
	u32 header_crc32;

	u32 const_fw_length;
	u32 hdr_length;
#if defined(CONFIG_SCSC_BB_REDWOOD)
	u32 ucpu0_panic_record_offset;
	u32 ucpu1_panic_record_offset;
	u32 ucpu2_panic_record_offset;
	u32 ucpu3_panic_record_offset;
#endif
	u32 r4_panic_record_offset;
	u32 m4_panic_record_offset;
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	u32 m4_1_panic_record_offset;
#endif
	/* New private attr */
	bool check_crc;
	bool fwhdr_parsed_ok;
	char build_id[FW_BUILD_ID_SZ];
	char ttid[FW_TTID_SZ];
	char *fw_dram_addr;
	u32 fw_size;
	struct workqueue_struct *fw_crc_wq;
	struct delayed_work fw_crc_work;
};

#ifdef CONFIG_WLBT_KUNIT
#include "./kunit/kunit_whdr.c"
#endif

/*
 * This function calulates and checks two or three (depending on crc32_over_binary flag)
 * crc32 values in the firmware header. The function will check crc32 over the firmware binary
 * (i.e. everything in the file following the header) only if the crc32_over_binary is set to 'true'.
 * This includes initialised data regions so it can be used to check when loading but will not be
 * meaningful once execution starts.
 */
static int whdr_do_fw_crc32_checks(struct fwhdr_if *interface, bool crc32_over_binary)
{
	int r;

	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	if ((whdr->fw_crc32 == 0 || whdr->header_crc32 == 0 || whdr->const_crc32 == 0) && crc_allow_none == 0) {
		SCSC_TAG_ERR(FW_LOAD, "error: CRC is missing fw_crc32=%d header_crc32=%d crc_allow_none=%d\n",
			     whdr->fw_crc32, whdr->header_crc32, crc_allow_none);
		return -EINVAL;
	}

	if (whdr->header_crc32 == 0 && crc_allow_none == 1) {
		SCSC_TAG_DBG4(FW_LOAD, "Skipping CRC check header_crc32=%d crc_allow_none=%d\n", whdr->header_crc32,
			      crc_allow_none);
	} else {
		/*
		 * CRC-32-IEEE of all preceding header fields (including other CRCs).
		 * Always the last word in the header.
		 */
		r = fwimage_check_fw_header_crc(whdr->fw_dram_addr, whdr->hdr_length, whdr->header_crc32);
		if (r) {
			SCSC_TAG_ERR(FW_LOAD, "fwimage_check_fw_header_crc() failed\n");
			return r;
		}
	}

	if (whdr->const_crc32 == 0 && crc_allow_none == 1) {
		SCSC_TAG_DBG4(FW_LOAD, "Skipping CRC check const_crc32=%d crc_allow_none=%d\n", whdr->const_crc32,
			      crc_allow_none);
	} else {
		/*
		 * CRC-32-IEEE over the constant sections grouped together at start of firmware binary.
		 * This CRC should remain valid during execution. It can be used by run-time checker on
		 * host to detect firmware corruption (not all memory masters are subject to MPUs).
		 */
		r = fwimage_check_fw_const_section_crc(whdr->fw_dram_addr, whdr->const_crc32, whdr->const_fw_length,
						       whdr->hdr_length);
		if (r) {
			SCSC_TAG_ERR(FW_LOAD, "fwimage_check_fw_const_section_crc() failed\n");
			return r;
		}
	}

	if (crc32_over_binary) {
		if (whdr->fw_crc32 == 0 && crc_allow_none == 1)
			SCSC_TAG_DBG4(FW_LOAD, "Skipping CRC check fw_crc32=%d crc_allow_none=%d\n", whdr->fw_crc32,
				      crc_allow_none);
		else {
			/*
			 * CRC-32-IEEE over the firmware binary (i.e. everything
			 * in the file following this header).
			 * This includes initialised data regions so it can be used to
			 * check when loading but will not be meaningful once execution starts.
			 */
			r = fwimage_check_fw_crc(whdr->fw_dram_addr, whdr->fw_size, whdr->hdr_length, whdr->fw_crc32);
			if (r) {
				SCSC_TAG_ERR(FW_LOAD, "fwimage_check_fw_crc() failed\n");
				return r;
			}
		}
	}

	return 0;
}

/** Return the next item after a given item and decrement the remaining length */
static const struct whdr_tag_length *whdr_next_item(const struct whdr_tag_length *item, uint32_t *whdr_length)
{
	uint32_t skip_length = sizeof(*item) + item->length;

	if (skip_length < *whdr_length) {
		*whdr_length -= skip_length;
		return (const struct whdr_tag_length *)((uintptr_t)item + skip_length);
	}

	return NULL;
}

/** Return the value of an item */
static const void *whdr_item_value(const struct whdr_tag_length *item)
{
	//SCSC_TAG_DBG4(FW_LOAD, "whdr_lookup_tag %p\n", item);
	//SCSC_TAG_DBG4(FW_LOAD, "whdr_lookup_tag current tag 0x%x length %u\n", item->tag, item->length);
	return (const void *)((uintptr_t)item + sizeof(*item));
}

const void *whdr_lookup_tag(const void *blob, enum whdr_tag tag, uint32_t *length)
{
	const struct whdr_tag_length *whdr = (const struct whdr_tag_length *)((char *)blob);
	const struct whdr_tag_length *item = whdr;
	uint32_t whdr_length;

	if (whdr->tag != WHDR_TAG_TOTAL_LENGTH) {
		SCSC_TAG_WARNING(FW_LOAD, "Unexpected first tag in whdr: %u \n", item->tag);
		return NULL;
	}

	/* First item is BHCD_TAG_TOTAL_LENGTH which contains overall length of the BHCD */
	whdr_length = *(const uint32_t *)whdr_item_value(whdr);

	//SCSC_TAG_DBG4(FW_LOAD, "whdr_lookup_tag 0x%x\n", tag);

	while (item != NULL) {
		if (item->tag == tag) {
			if (length != NULL) {
				*length = item->length;
			}
			SCSC_TAG_DBG4(FW_LOAD, "whdr_lookup_tag 0x%x found with size %u\n", item->tag, item->length);
			return whdr_item_value(item);
		}
		item = whdr_next_item(item, &whdr_length);
	}

	return NULL;
}

static bool whdr_parse_v02(struct whdr *whdr, char *fw)
{
#if defined(CONFIG_SCSC_BB_REDWOOD)
	struct whdr_offset_length *wol;
	struct system_error_descriptor *sed;
	uint32_t length;
	u32 whdr_offset = FWHDR_02_WHDR_OFFSET(fw); // offset to "WHDR"
#endif

	if (!memcmp(fw + FWHDR_02_MAGIC_OFFSET, FWHDR_MAGIC_STRING, sizeof(FWHDR_MAGIC_STRING) - 1)) {
		whdr->firmware_entry_point = FWHDR_02_HEADER_FIRMWARE_ENTRY_POINT(fw);
		whdr->hdr_major = FWHDR_02_HEADER_VERSION_MAJOR(fw);
		whdr->hdr_minor = FWHDR_02_HEADER_VERSION_MINOR(fw);
		whdr->fwapi_major = FWHDR_02_HEADER_FIRMWARE_API_VERSION_MAJOR(fw);
		whdr->fwapi_minor = FWHDR_02_HEADER_FIRMWARE_API_VERSION_MINOR(fw);
		whdr->fw_crc32 = FWHDR_02_FW_CRC32(fw);
		whdr->const_crc32 = FWHDR_02_CONST_CRC32(fw);
		whdr->header_crc32 = FWHDR_02_HEADER_CRC32(fw);
		whdr->const_fw_length = FWHDR_02_CONST_FW_LENGTH(fw);
		whdr->hdr_length = FWHDR_02_HDR_LENGTH(fw);
		whdr->fw_runtime_length = FWHDR_02_HEADER_FIRMWARE_RUNTIME_LENGTH(fw);
		SCSC_TAG_DBG4(FW_LOAD, "hdr_length=%d\n", whdr->hdr_length);
		whdr->r4_panic_record_offset = FWHDR_02_R4_PANIC_RECORD_OFFSET(fw);
		whdr->m4_panic_record_offset = FWHDR_02_M4_PANIC_RECORD_OFFSET(fw);
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
		whdr->m4_1_panic_record_offset = FWHDR_02_M4_PANIC_RECORD_OFFSET(fw);
#endif

		memcpy(whdr->build_id, FWHDR_02_HEADER_BUILD_ID_OFFSET(fw), sizeof(whdr->build_id));
		if (whdr->hdr_length < FWHDR_02_TTID_OFFSET)
			whdr->ttid[0] = '\0';
		else
			memcpy(whdr->ttid, FWHDR_02_HEADER_TTID_OFFSET(fw), sizeof(whdr->ttid));

#if defined(CONFIG_SCSC_BB_REDWOOD)
		wol = (struct whdr_offset_length *)whdr_lookup_tag(fw + whdr_offset, WHDR_TAG_FW_SYSTEM_ERROR_DESCRIPTOR, &length);
		if (wol == NULL || length != sizeof(struct whdr_offset_length))
			return false;

		sed = (struct system_error_descriptor *)(fw + wol->offset);
		if (length != sizeof(struct system_error_descriptor))
			return false;
		SCSC_TAG_DBG4(FW_LOAD, "ucpu0 panic record at offset=0x%x length=%d\n", sed->offset, length);
		//SCSC_TAG_DBG4(FW_LOAD, "ucpu1 panic record at offset=0x%x length=%d\n", (sed+1)->offset, length);
		//SCSC_TAG_DBG4(FW_LOAD, "ucpu2 panic record at offset offset=0x%x length=%d\n", (sed+2)->offset, length);
		//SCSC_TAG_DBG4(FW_LOAD, "ucpu3 panic record at offset=0x%x length=%d\n", (sed+3)->offset, length);
		SCSC_TAG_DBG4(FW_LOAD, "mcpu0 panic record at offset=0x%x length=%d\n", (sed+4)->offset, length);
		//SCSC_TAG_DBG4(FW_LOAD, "m7 panic record at offset=0x%x length=%d\n", (sed+5)->offset, length);
		whdr->ucpu0_panic_record_offset = (u32)sed->offset;
		whdr->ucpu1_panic_record_offset = (u32)(sed+1)->offset;
		whdr->ucpu2_panic_record_offset = (u32)(sed+2)->offset;
		whdr->ucpu3_panic_record_offset = (u32)(sed+3)->offset;
		whdr->r4_panic_record_offset = (u32)(sed+4)->offset;
		whdr->m4_panic_record_offset = (u32)(sed+5)->offset;
#endif
		return true;
	}
	return false;
}

static bool whdr_parse(struct whdr *whdr, char *fw_data)
{
	return whdr_parse_v02(whdr, fw_data);
}

static char *whdr_get_build_id(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->build_id;
}

static char *whdr_get_ttid(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->ttid;
}

static u32 whdr_get_entry_point(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->firmware_entry_point;
}

static void whdr_set_entry_point(struct fwhdr_if *interface, u32 entry_point)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	whdr->firmware_entry_point = entry_point;
}

static bool whdr_get_parsed_ok(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->fwhdr_parsed_ok;
}

static u32 whdr_get_fw_rt_len(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->fw_runtime_length;
}

static u32 whdr_get_fw_len(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->fw_size;
}

static void whdr_set_fw_rt_len(struct fwhdr_if *interface, u32 rt_len)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	whdr->fw_runtime_length = rt_len;
}

static void whdr_set_check_crc(struct fwhdr_if *interface, bool check_crc)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	whdr->check_crc = check_crc;
}

static bool whdr_get_check_crc(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->check_crc;
}

static u32 whdr_get_fwapi_major(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->fwapi_major;
}

static u32 whdr_get_fwapi_minor(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	return whdr->fwapi_minor;
}

static u32 whdr_get_panic_record_offset(struct fwhdr_if *interface, enum scsc_mif_abs_target target)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	switch (target) {
	case SCSC_MIF_ABS_TARGET_WLAN:
		return whdr->r4_panic_record_offset;
	case SCSC_MIF_ABS_TARGET_FXM_1:
		return whdr->m4_panic_record_offset;
#ifdef CONFIG_SCSC_MX450_GDB_SUPPORT
	case SCSC_MIF_ABS_TARGET_FXM_2:
		return whdr->m4_1_panic_record_offset;
#if defined(CONFIG_SCSC_BB_REDWOOD)
	case SCSC_MIF_ABS_TARGET_WLAN_5:
		return whdr->ucpu0_panic_record_offset;
	case SCSC_MIF_ABS_TARGET_WLAN_6:
		return whdr->ucpu1_panic_record_offset;
	case SCSC_MIF_ABS_TARGET_WLAN_7:
		return whdr->ucpu2_panic_record_offset;
	case SCSC_MIF_ABS_TARGET_WLAN_8:
		return whdr->ucpu3_panic_record_offset;
#endif
#endif
	default:
		return 0;
	}
}
static int whdr_copy_fw(struct fwhdr_if *interface, char *fw_data, size_t fw_size, void *dram_addr)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	whdr->fw_dram_addr = (char *)dram_addr;
	whdr->fw_size = fw_size;

	memcpy(dram_addr, fw_data, fw_size);

	/* Bit of 'paranoia' here, but make sure FW is copied over and visible
	 * for all CPUs*/
	smp_mb();

	return 0;
}

static void whdr_invalidate_error_records(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);
	void *area_start;
	size_t area_len;

	if (whdr->fw_dram_addr == NULL) {
		SCSC_TAG_WARNING(FW_LOAD, "There is no address for fw dram\n");
		return;
	} else if (whdr->fw_runtime_length <= whdr->fw_size) {
		SCSC_TAG_WARNING(FW_LOAD, "Wrong parameters: fw_len 0x%x runtime_size 0x%x\n",
								whdr->fw_size, whdr->fw_runtime_length);
		return;
	}

	/* 	The area that can be derived below means the memory region that Wi-Fi FW uses during runtime.
		When collecting minimoredump during creation of moredump, then the host accesses this area.	*/
	area_start = whdr->fw_dram_addr + whdr->fw_size;
	area_len = whdr->fw_runtime_length - whdr->fw_size;

	SCSC_TAG_DBG4(FW_LOAD, "WiFi fw_len 0x%x runtime_size 0x%x from start of Shared DRAM\n",
							whdr->fw_size, whdr->fw_runtime_length);

	/* 	In order to secure correct system error records in all situations,
		then the host needs to make sure that the system error records has been invalidated  */
	memset(area_start, 0, area_len);
}

static int whdr_init(struct fwhdr_if *interface, char *fw_data, size_t fw_len, bool skip_header)
{
	/*
	 * Validate the fw image including checking the firmware header, majic #, version, checksum  so on
	 * then do CRC on the entire image
	 *
	 * Derive some values from header -
	 *
	 * PORT: assumes little endian
	 */
	struct whdr *whdr = whdr_from_fwhdr_if(interface);

	if (skip_header)
		whdr->fwhdr_parsed_ok = false; /* Allows the forced start address to be used */
	else
		whdr->fwhdr_parsed_ok = whdr_parse(whdr, fw_data);
	whdr->check_crc = false;
	if (whdr->fwhdr_parsed_ok) {
		SCSC_TAG_DBG4(FW_LOAD, "FW HEADER version: hdr_major: %d hdr_minor: %d\n", whdr->hdr_major,
			      whdr->hdr_minor);
		switch (whdr->hdr_major) {
		case 0:
			switch (whdr->hdr_minor) {
			case 2:
				whdr->check_crc = true;
				break;
			default:
				SCSC_TAG_ERR(FW_LOAD, "Unsupported FW HEADER version: hdr_major: %d hdr_minor: %d\n",
					     whdr->hdr_major, whdr->hdr_minor);
				return -EINVAL;
			}
			break;
		case 1:
			whdr->check_crc = true;
			break;
		default:
			SCSC_TAG_ERR(FW_LOAD, "Unsupported FW HEADER version: hdr_major: %d hdr_minor: %d\n",
				     whdr->hdr_major, whdr->hdr_minor);
			return -EINVAL;
		}
		switch (whdr->fwapi_major) {
		case 0:
			switch (whdr->fwapi_minor) {
			case 2:
				SCSC_TAG_DBG4(FW_LOAD, "FWAPI version: fwapi_major: %d fwapi_minor: %d\n",
					      whdr->fwapi_major, whdr->fwapi_minor);
				break;
			default:
				SCSC_TAG_ERR(FW_LOAD, "Unsupported FWAPI version: fwapi_major: %d fwapi_minor: %d\n",
					     whdr->fwapi_major, whdr->fwapi_minor);
				return -EINVAL;
			}
			break;
		default:
			SCSC_TAG_ERR(FW_LOAD, "Unsupported FWAPI version: fwapi_major: %d fwapi_minor: %d\n",
				     whdr->fwapi_major, whdr->fwapi_minor);
			return -EINVAL;
		}
	}
	return 0;
}

/*********************
 * CRC WQ
 * *******************/
static void whdr_crc_wq_start(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);
	struct fwhdr_if *fw_if = &whdr->fw_if;

	if (whdr_get_check_crc(fw_if) && crc_check_period_ms)
		queue_delayed_work(whdr->fw_crc_wq, &whdr->fw_crc_work, msecs_to_jiffies(crc_check_period_ms));
}

static void whdr_crc_wq_stop(struct fwhdr_if *interface)
{
	struct whdr *whdr = whdr_from_fwhdr_if(interface);
	struct fwhdr_if *fw_if = &whdr->fw_if;

	whdr_set_check_crc(fw_if, false);
	cancel_delayed_work(&whdr->fw_crc_work);
	flush_workqueue(whdr->fw_crc_wq);
}

static void whdr_crc_work_func(struct work_struct *work)
{
	int r;
	struct whdr *whdr = container_of((struct delayed_work *)work, struct whdr, fw_crc_work);
	struct fwhdr_if *fw_if = &whdr->fw_if;

	r = whdr_do_fw_crc32_checks(fw_if, false);
	if (r) {
		SCSC_TAG_ERR(FW_LOAD, "do_fw_crc32_checks() failed r=%d\n", r);
		return;
	}
	whdr_crc_wq_start(fw_if);
}

/* Implementation creation */
struct fwhdr_if *whdr_create(void)
{
	struct fwhdr_if *fw_if;
	struct whdr *whdr = kzalloc(sizeof(struct whdr), GFP_KERNEL);

	if (!whdr)
		return NULL;

	fw_if = &whdr->fw_if;

	fw_if->init = whdr_init;
	fw_if->do_fw_crc32_checks = whdr_do_fw_crc32_checks;
	fw_if->copy_fw = whdr_copy_fw;
	fw_if->invalidate_error_records = whdr_invalidate_error_records;
	fw_if->get_build_id = whdr_get_build_id;
	fw_if->get_ttid = whdr_get_ttid;
	fw_if->get_entry_point = whdr_get_entry_point;
	fw_if->get_parsed_ok = whdr_get_parsed_ok;
	fw_if->get_check_crc = whdr_get_check_crc;
	fw_if->get_fw_rt_len = whdr_get_fw_rt_len;
	fw_if->get_fw_len = whdr_get_fw_len;
	fw_if->get_fwapi_major = whdr_get_fwapi_major;
	fw_if->get_fwapi_minor = whdr_get_fwapi_minor;
	fw_if->get_panic_record_offset = whdr_get_panic_record_offset;

	/* CRC */
	fw_if->crc_wq_stop = whdr_crc_wq_stop;
	fw_if->crc_wq_start = whdr_crc_wq_start;

	/* Setters */
	fw_if->set_entry_point = whdr_set_entry_point;
	fw_if->set_fw_rt_len = whdr_set_fw_rt_len;
	fw_if->set_check_crc = whdr_set_check_crc;
	whdr->fw_crc_wq = create_singlethread_workqueue("fwhdr_crc_wq");
	if (!whdr->fw_crc_wq) {
		SCSC_TAG_ERR(FW_LOAD, "create_singlethread_workqueue() failed\n");
		kfree(whdr);
		whdr = NULL;
		return NULL;
	}

	INIT_DELAYED_WORK(&whdr->fw_crc_work, whdr_crc_work_func);

	return fw_if;
}

/* Implementation destroy */
void whdr_destroy(struct fwhdr_if *interface)
{
	struct whdr *whdr;
	struct fwhdr_if *fw_if;

	if (!interface)
		return;

	whdr = whdr_from_fwhdr_if(interface);
	if (!whdr)
		return;

	fw_if = &whdr->fw_if;

	if (!fw_if)
		goto whdr_destroy_error;
	whdr_crc_wq_stop(fw_if);

	if (!whdr->fw_crc_wq)
		goto whdr_destroy_error;
	destroy_workqueue(whdr->fw_crc_wq);

whdr_destroy_error:
	kfree(whdr);
	whdr = NULL;
}
