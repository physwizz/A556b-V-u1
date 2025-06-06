/****************************************************************************
 *
 * Copyright (c) 2014 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <pcie_scsc/scsc_logring.h>
#include "fw_panic_record.h"
#include "panic_record_r4_defs.h"
#include "panic_record_m7_defs.h"

#define PANIC_RECORD_CKSUM_SEED 0xa5a5a5a5
/*
 * version 2 r4 panic record defs
 */
#define R4_PANIC_RECORD_VERSION_2 2
#define R4_PANIC_RECORD_LENGTH_INDEX_V2 1
#define R4_PANIC_STACK_RECORD_OFFSET_INDEX_V2 52
#define R4_PANIC_RECORD_MAX_LENGTH_V2 256

/* Panic stack record (optional) - linked to from panic record */
#define R4_PANIC_STACK_RECORD_VERSION_1 1
#define R4_PANIC_STACK_RECORD_VERSION_INDEX 0
#define R4_PANIC_STACK_RECORD_LENGTH_INDEX 1
#define R4_PANIC_STACK_RECORD_MAX_LENGTH 256

/*
 * version 1 mr4 panic record defs
 */
#define M4_PANIC_RECORD_VERSION_1 1
#define M4_PANIC_RECORD_VERSION_INDEX 0
#define M4_PANIC_RECORD_LENGTH_INDEX 1
#define M4_PANIC_RECORD_MAX_LENGTH 256

#define R4_PANIC_RECORD_V2_SYMPATHETIC_PANIC_FLAG_INDEX 51
#define M4_PANIC_RECORD_SYMPATHETIC_PANIC_FLAG_INDEX 39

/**
 * Compute 32bit xor of specified seed value and data.
 *
 * @param   seed    Initial seed value.
 * @param   data    Array of uint32s to be xored
 * @param   len     Number of uint32s to be xored
 *
 * @return  Computed 32bit xor of specified seed value and data.
 */
static u32 xor32(uint32_t seed, const u32 data[], size_t len)
{
	const u32 *i;
	u32 xor = seed;

	for (i = data; i != data + len; ++i)
		xor ^= *i;
	return xor;
}

static bool panic_record_invalid(u32 *panic_record, const char *record_type)
{
	bool ret = true;

	if (panic_record == NULL)
		SCSC_TAG_INFO(FW_PANIC, "%s is NULL\n", record_type);
	else if ((*panic_record == 0) || (*panic_record >= 3)) /* only 1,2 are valid panic record versions */
		SCSC_TAG_INFO(FW_PANIC, "%s is INVALID (value: %d)\n", record_type, *panic_record);
	else
		ret = false;

	return ret;
}

static void panic_record_dump(u32 *panic_record, u32 panic_record_length, enum cores_type core)
{
	u32 i;

	SCSC_TAG_INFO(FW_PANIC, "%s panic record dump(length=%d):\n",
		      core_names[core], panic_record_length);
	for (i = 0; i < panic_record_length; i++)
		SCSC_TAG_INFO(FW_PANIC, "%s_panic_record[%d] = %08x\n",
			      core_names[core], i, panic_record[i]);
}

static void panic_stack_record_dump(u32 *panic_stack_record, u32 panic_stack_record_length, enum cores_type core)
{
	u32 i;

	SCSC_TAG_INFO(FW_PANIC, "%s panic stack_record dump(length=%d):\n",
		      core_names[core], panic_stack_record_length);
	for (i = 0; i < panic_stack_record_length; i++)
		SCSC_TAG_INFO(FW_PANIC, "%s_panic_stack_record[%d] = %08x\n",
			      core_names[core], i, panic_stack_record[i]);
}

static bool fw_parse_r4_panic_record_v2(u32 *r4_panic_record, u32 *r4_panic_record_length, u32 *r4_panic_stack_record_offset, bool dump)
{
	u32 panic_record_cksum;
	u32 calculated_cksum;
	u32 panic_record_length;

	if (panic_record_invalid(r4_panic_record, "r4_panic_record"))
		return false;

	panic_record_length = *(r4_panic_record + R4_PANIC_RECORD_LENGTH_INDEX_V2) / 4;

	if (dump)
		SCSC_TAG_INFO(FW_PANIC, "panic_record_length: %d\n", panic_record_length);

	if (panic_record_length < R4_PANIC_RECORD_MAX_LENGTH_V2) {
		panic_record_cksum = *(r4_panic_record + panic_record_length - 1);
		calculated_cksum = xor32(PANIC_RECORD_CKSUM_SEED, r4_panic_record, panic_record_length - 1);
		if (calculated_cksum == panic_record_cksum) {
			SCSC_TAG_INFO(FW_PANIC, "panic_record_cksum OK: %08x\n", calculated_cksum);
			if (dump)
				panic_record_dump(r4_panic_record, panic_record_length, R7);
			*r4_panic_record_length = panic_record_length;
			/* Optionally extract the offset of the panic stack record */
			if (r4_panic_stack_record_offset) {
				*r4_panic_stack_record_offset = *(r4_panic_record + R4_PANIC_STACK_RECORD_OFFSET_INDEX_V2);
			}
			return true;
		} else {
			SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_cksum: 0x%x calculated_cksum: 0x%x\n",
				     panic_record_cksum, calculated_cksum);
		}
	} else {
		SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_length: %d\n",
			     panic_record_length);
	}
	return false;
}

static bool fw_parse_r4_panic_stack_record_v1(u32 *r4_panic_stack_record, u32 *r4_panic_stack_record_length, bool dump)
{
	u32 panic_stack_record_length;

	if (panic_record_invalid(r4_panic_stack_record, "r4_panic_stack_record"))
		return false;

	panic_stack_record_length = *(r4_panic_stack_record + R4_PANIC_STACK_RECORD_LENGTH_INDEX) / 4;

	if (panic_stack_record_length < R4_PANIC_STACK_RECORD_MAX_LENGTH) {
		if (dump)
			panic_stack_record_dump(r4_panic_stack_record, panic_stack_record_length, R7);
		*r4_panic_stack_record_length = panic_stack_record_length;
		return true;
	} else {
		SCSC_TAG_ERR(FW_PANIC, "BAD panic_stack_record_length: %d\n",
			     panic_stack_record_length);
	}
	return false;
}

static bool fw_parse_m4_panic_record_v1(u32 *m4_panic_record, u32 *m4_panic_record_length, bool dump)
{
	u32 panic_record_cksum;
	u32 calculated_cksum;
	u32 panic_record_length;

	if (panic_record_invalid(m4_panic_record, "m4_panic_record"))
		return false;

	panic_record_length = *(m4_panic_record + M4_PANIC_RECORD_LENGTH_INDEX) / 4;

	if (dump)
		SCSC_TAG_INFO(FW_PANIC, "panic_record_length: %d\n", panic_record_length);

	if (panic_record_length < M4_PANIC_RECORD_MAX_LENGTH) {
		panic_record_cksum = *(m4_panic_record + panic_record_length - 1);
		calculated_cksum = xor32(PANIC_RECORD_CKSUM_SEED, m4_panic_record, panic_record_length - 1);
		if (calculated_cksum == panic_record_cksum) {
			if (dump) {
				SCSC_TAG_INFO(FW_PANIC, "panic_record_cksum OK: %08x\n", calculated_cksum);
				panic_record_dump(m4_panic_record, panic_record_length, M4);
			}
			*m4_panic_record_length = panic_record_length;
			return true;
		} else {
			SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_cksum: 0x%x calculated_cksum: 0x%x\n",
				     panic_record_cksum, calculated_cksum);
		}
	} else {
		SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_length: %d\n",
			     panic_record_length);
	}
	return false;
}

#if defined(CONFIG_SCSC_BB_REDWOOD)
bool fw_parse_ucpu_panic_record(u32 *ucpu_panic_record, u32 *ucpu_panic_record_length, u32 *ucpu_panic_stack_record_offset, bool dump, enum scsc_mif_abs_target target)
{
	u32 panic_record_version;
	u32 panic_record_cksum;
	u32 calculated_cksum;
	u32 panic_record_length;
	enum cores_type cpu_core;

	if (panic_record_invalid(ucpu_panic_record, "ucpu_panic_record"))
		return false;

	panic_record_length = *(ucpu_panic_record + R4_PANIC_RECORD_LENGTH_INDEX_V2) / 4;
	panic_record_version = *(ucpu_panic_record + PANIC_RECORD_R4_VERSION_INDEX);

	if (dump) {
		SCSC_TAG_INFO(FW_PANIC, "ucpu_panic_record_version: %d\n", panic_record_version);
		SCSC_TAG_INFO(FW_PANIC, "ucpu_panic_record_length: %d\n", panic_record_length);
	}

	if (panic_record_length < R4_PANIC_RECORD_MAX_LENGTH_V2) {
		panic_record_cksum = *(ucpu_panic_record + panic_record_length - 1);
		calculated_cksum = xor32(PANIC_RECORD_CKSUM_SEED, ucpu_panic_record, panic_record_length - 1);
		if (calculated_cksum == panic_record_cksum) {
			SCSC_TAG_INFO(FW_PANIC, "ucpu_panic_record_cksum OK: %08x\n", calculated_cksum);
			if (dump) {
				switch (target) {
				case SCSC_MIF_ABS_TARGET_WLAN_5:
					cpu_core = UCPU0;
					break;
				case SCSC_MIF_ABS_TARGET_WLAN_6:
					cpu_core = UCPU1;
					break;
				case SCSC_MIF_ABS_TARGET_WLAN_7:
					cpu_core = UCPU2;
					break;
				case SCSC_MIF_ABS_TARGET_WLAN_8:
					cpu_core = UCPU3;
					break;
				default:
					SCSC_TAG_INFO(FW_PANIC, "Non UCPU target: %d\n", target);
					return false;
				}
				panic_record_dump(ucpu_panic_record, panic_record_length, cpu_core);
			}
			*ucpu_panic_record_length = panic_record_length;
			/* Optionally extract the offset of the panic stack record */
			if (ucpu_panic_stack_record_offset) {
				*ucpu_panic_stack_record_offset = *(ucpu_panic_record + R4_PANIC_STACK_RECORD_OFFSET_INDEX_V2);
			}

			return true;
		} else {
			SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_cksum: 0x%x calculated_cksum: 0x%x\n",
				     panic_record_cksum, calculated_cksum);
		}
	} else {
		SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_length: %d\n",
			     panic_record_length);
	}
	return false;
}

bool fw_parse_ucpu_panic_stack_record(u32 *ucpu_panic_stack_record, u32 *ucpu_panic_stack_record_length, bool dump, enum scsc_mif_abs_target target)
{
	u32 panic_stack_record_version;
	u32 panic_stack_record_length;
	enum cores_type cpu_core;

	if (panic_record_invalid(ucpu_panic_stack_record, "ucpu_panic_stack_record"))
		return false;

	panic_stack_record_length = *(ucpu_panic_stack_record + R4_PANIC_STACK_RECORD_LENGTH_INDEX) / 4;
	panic_stack_record_version = *(ucpu_panic_stack_record + R4_PANIC_STACK_RECORD_VERSION_INDEX);

	SCSC_TAG_INFO(FW_PANIC, "ucpu_panic_stack_record_version: %d\n", panic_stack_record_version);

	switch (panic_stack_record_version) {
	default:
		SCSC_TAG_ERR(FW_PANIC, "BAD ucpu_panic_stack_record_version: %d\n",
			     panic_stack_record_version);
		break;
	case R4_PANIC_STACK_RECORD_VERSION_1:
		if (panic_stack_record_length < R4_PANIC_STACK_RECORD_MAX_LENGTH) {
			if (dump) {
				switch (target) {
				case SCSC_MIF_ABS_TARGET_WLAN_5:
					cpu_core = UCPU0;
					break;
				case SCSC_MIF_ABS_TARGET_WLAN_6:
					cpu_core = UCPU1;
					break;
				case SCSC_MIF_ABS_TARGET_WLAN_7:
					cpu_core = UCPU2;
					break;
				case SCSC_MIF_ABS_TARGET_WLAN_8:
					cpu_core = UCPU3;
					break;
				default:
					SCSC_TAG_INFO(FW_PANIC, "Non UCPU target: %d\n", target);
					return false;
				}
				panic_stack_record_dump(ucpu_panic_stack_record, panic_stack_record_length, cpu_core);
				*ucpu_panic_stack_record_length = panic_stack_record_length;
			}
			return true;
		} else
			SCSC_TAG_ERR(FW_PANIC, "BAD ucpu_panic_stack_record_length: %d\n", panic_stack_record_length);
	}
	return false;
}

bool fw_parse_get_ucpu_sympathetic_panic_flag(u32 *ucpu_panic_record)
{
	bool sympathetic_panic_flag;

	if (ucpu_panic_record == NULL)   {
                SCSC_TAG_INFO(FW_PANIC, "ucpu_panic_record is %s", ucpu_panic_record == NULL ? "NULL" : "NOT NULL");
                return false;
        }

	sympathetic_panic_flag = *(ucpu_panic_record + R4_PANIC_RECORD_V2_SYMPATHETIC_PANIC_FLAG_INDEX);

	return sympathetic_panic_flag;
}

#endif

bool fw_parse_r4_panic_record(u32 *r4_panic_record, u32 *r4_panic_record_length, u32 *r4_panic_stack_record_offset, bool dump)
{
	u32 panic_record_version;

	if (panic_record_invalid(r4_panic_record, "r4_panic_record"))
		return false;

	panic_record_version = *(r4_panic_record + PANIC_RECORD_R4_VERSION_INDEX);

	if (dump)
		SCSC_TAG_INFO(FW_PANIC, "panic_record_version: %d\n", panic_record_version);

	switch (panic_record_version) {
	default:
		SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_version: %d\n",
			     panic_record_version);
		break;
	case R4_PANIC_RECORD_VERSION_2:
		return fw_parse_r4_panic_record_v2(r4_panic_record, r4_panic_record_length, r4_panic_stack_record_offset, dump);
	}
	return false;
}

bool fw_parse_r4_panic_stack_record(u32 *r4_panic_stack_record, u32 *r4_panic_stack_record_length, bool dump)
{
	u32 panic_stack_record_version;

	if (panic_record_invalid(r4_panic_stack_record, "r4_panic_stack_record"))
		return false;

	panic_stack_record_version = *(r4_panic_stack_record + R4_PANIC_STACK_RECORD_VERSION_INDEX);

	SCSC_TAG_INFO(FW_PANIC, "panic_stack_record_version: %d\n", panic_stack_record_version);

	switch (panic_stack_record_version) {
	default:
		SCSC_TAG_ERR(FW_PANIC, "BAD panic_stack_record_version: %d\n",
			     panic_stack_record_version);
		break;
	case R4_PANIC_STACK_RECORD_VERSION_1:
		return fw_parse_r4_panic_stack_record_v1(r4_panic_stack_record, r4_panic_stack_record_length, dump);
	}
	return false;
}

bool fw_parse_m4_panic_record(u32 *m4_panic_record, u32 *m4_panic_record_length, bool dump)
{
	u32 panic_record_version;

	if (panic_record_invalid(m4_panic_record, "m4_panic_record"))
		return false;

	panic_record_version = *(m4_panic_record + M4_PANIC_RECORD_VERSION_INDEX);

	SCSC_TAG_INFO(FW_PANIC, "panic_record_version: %d\n", panic_record_version);
	switch (panic_record_version) {
	default:
		SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_version: %d\n",
			     panic_record_version);
		break;
	case M4_PANIC_RECORD_VERSION_1:
		return fw_parse_m4_panic_record_v1(m4_panic_record, m4_panic_record_length, dump);
	}
	return false;
}

bool fw_parse_get_r4_sympathetic_panic_flag(u32 *r4_panic_record)
{
	bool sympathetic_panic_flag;

	if (r4_panic_record == NULL)   {
                SCSC_TAG_INFO(FW_PANIC, "r4_panic_record is %s", r4_panic_record == NULL ? "NULL" : "NOT NULL");
                return false;
        }

	sympathetic_panic_flag = *(r4_panic_record + R4_PANIC_RECORD_V2_SYMPATHETIC_PANIC_FLAG_INDEX);

	return sympathetic_panic_flag;
}

bool fw_parse_get_m4_sympathetic_panic_flag(u32 *m4_panic_record)
{
	bool sympathetic_panic_flag;

	if (m4_panic_record == NULL)   {
                SCSC_TAG_INFO(FW_PANIC, "m4_panic_record is %s", m4_panic_record == NULL ? "NULL" : "NOT NULL");
                return false;
        }
	sympathetic_panic_flag = *(m4_panic_record + M4_PANIC_RECORD_SYMPATHETIC_PANIC_FLAG_INDEX);
	return sympathetic_panic_flag;
}

static bool fw_parse_wpan_panic_record_v1(u32 *wpan_panic_record, u32 *full_panic_code, bool dump)
{
	m7_system_error_record *m7_panic;
	u32 calculated_cksum;

	m7_panic = (m7_system_error_record *)wpan_panic_record;

	if (dump) {
		SCSC_TAG_INFO(FW_PANIC, "info[0]: 0x%08x\n", m7_panic->info[0]);
		SCSC_TAG_INFO(FW_PANIC, "info[1]: 0x%08x\n", m7_panic->info[1]);
		SCSC_TAG_INFO(FW_PANIC, "info[2]: 0x%08x\n", m7_panic->info[2]);
		SCSC_TAG_INFO(FW_PANIC, "info[3]: 0x%08x\n", m7_panic->info[3]);
	}

	if (m7_panic->length < M7_PANIC_RECORD_MAX_LENGTH) {
		calculated_cksum = xor32(M7_SYSTEM_ERROR_RECORD_CKSUM_SEED, (const u32 *)m7_panic,
				(m7_panic->length / sizeof(uint32_t)) - 1);
		if (calculated_cksum == m7_panic->cksum) {
			if (dump) {
				SCSC_TAG_INFO(FW_PANIC, "m7_panic->cksum OK: %08x\n", calculated_cksum);
				panic_record_dump(wpan_panic_record, m7_panic->length, M7);
			}
			*full_panic_code = m7_panic->info[1];
			return true;
		} else {
			SCSC_TAG_ERR(FW_PANIC, "BAD m7_panic->cksum: 0x%x calculated_cksum: 0x%x\n",
				     m7_panic->cksum, calculated_cksum);
		}
	} else {
		SCSC_TAG_ERR(FW_PANIC, "BAD m7_panic->length: %d\n",
			     m7_panic->length);
	}
	*full_panic_code = 0;
	return false;
}

bool fw_parse_wpan_panic_record(u32 *wpan_panic_record, u32 *full_panic_code, bool dump)
{
	m7_system_error_record *m7_panic;
	u32 panic_record_version;

	m7_panic = (m7_system_error_record *)wpan_panic_record;

	if (wpan_panic_record == NULL) {
		SCSC_TAG_INFO(FW_PANIC, "wpan_panic_record is NULL");
		return false;
	}

	panic_record_version = m7_panic->version;
	SCSC_TAG_INFO(FW_PANIC, "panic_record_version: 0x%x\n", panic_record_version);

	switch (panic_record_version) {
	default:
		SCSC_TAG_ERR(FW_PANIC, "BAD panic_record_version: %d\n",
			     panic_record_version);
		break;
	case M7_SYSTEM_ERROR_RECORD_VERSION:
		return fw_parse_wpan_panic_record_v1(wpan_panic_record, full_panic_code, dump);
	}
	return false;
}

int panic_record_dump_buffer(char *processor, u32 *panic_record,
			     u32 panic_record_length, char *buffer, size_t blen)
{
	int i, used;

	if (!processor)
		processor = "WLBT";

	if (panic_record == NULL || buffer == NULL)	{
		SCSC_TAG_INFO(FW_PANIC, "panic_record is %s buffer is %s", panic_record == NULL ? "NULL" : "NOT NULL", buffer == NULL ? "NULL" : "NOT NULL");
		return 0;
	}

	used = snprintf(buffer, blen, "%s panic record dump(length=%d):\n",
			processor, panic_record_length);
	for (i = 0; i < panic_record_length && used < blen; i++)
		used += snprintf(buffer + used, blen - used, "%s_panic_record[%d] = %08x\n",
				 processor, i, panic_record[i]);

	return used;
}
