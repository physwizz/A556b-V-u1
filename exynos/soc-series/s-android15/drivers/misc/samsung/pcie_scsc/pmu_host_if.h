/****************************************************************************
 *
 * Copyright (c) 2014 - 2021 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/
/* PMU - Host interface definition */

#ifndef PMU_HOST_IF_H__
#define PMU_HOST_IF_H__

/* Enum for subsystems */
typedef enum
{
    PMU_SUBSYS_WPAN = 0,
    PMU_SUBSYS_WLAN,
    PMU_NUM_SUBSYS
} pmu_subsys;

/* Stuff below will need updating to match updated spec and putting into enums */

/* Subsystem mask */
#define PMU_AP_MSG_SUBSYS_MASK                        ((0x1 << PMU_NUM_SUBSYS) -1)
#define PMU_AP_MSG_SUBSYS_BITS                        (PMU_NUM_SUBSYS)

/* Individual Subsystem bits within PMU_AP_MSG_SUBSYS_MASK */
#define PMU_AP_MSG_SUBSYS_WPAN                        (0x1 << PMU_SUBSYS_WPAN)
#define PMU_AP_MSG_SUBSYS_WLAN                        (0x1 << PMU_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYS(subsys)                     (0x1 << subsys)

/* Command mask */
#define PMU_AP_MSG_COMMAND_MASK                       (0xFF ^ PMU_AP_MSG_SUBSYS_MASK)

/* No message indication (applies in either direction */
#define PMU_AP_MSG_NULL                               (0x00)

/*****************************/
/* Messages from host to PMU */
/*****************************/

/* Start the WLBT subsystem(s) indicated */
#define PMU_AP_MSG_SUBSYS_START_CMD                   (0x1 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_SUBSYS_START_WPAN                  (PMU_AP_MSG_SUBSYS_START_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WPAN)
#define PMU_AP_MSG_SUBSYS_START_WLAN                  (PMU_AP_MSG_SUBSYS_START_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYS_START_WPAN_WLAN             (PMU_AP_MSG_SUBSYS_START_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WPAN | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYS_START(subsys)               (PMU_AP_MSG_SUBSYS_START_CMD | \
                                                       PMU_AP_MSG_SUBSYS(subsys))

/* Reset/Stop the subsystem(s) indicated */
#define PMU_AP_MSG_SUBSYS_RESET_CMD                   (0x2 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_SUBSYS_RESET_WPAN                  (PMU_AP_MSG_SUBSYS_RESET_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WPAN)
#define PMU_AP_MSG_SUBSYS_RESET_WLAN                  (PMU_AP_MSG_SUBSYS_RESET_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYS_RESET_WPAN_WLAN             (PMU_AP_MSG_SUBSYS_RESET_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WPAN | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYSTEM_SUBSYS_RESET(subsys)     (PMU_AP_MSG_SUBSYS_RESET_CMD | \
                                                       PMU_AP_MSG_SUBSYS(subsys))

/* Configure subsystem(s) monitor mode behaviour on other subsystem failure
 * If a subsytem is indicated this forces it to go into into monitor mode if
 * there is a subsequent system failure of the other subsytem.
 * Otherwise the subsystem will attempt recovery on failure of the other subsystem */
#define PMU_AP_CFG_MONITOR_CMD                        (0x3 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_CFG_MONITOR_WPAN                       (PMU_AP_CFG_MONITOR_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WPAN)
#define PMU_AP_CFG_MONITOR_WLAN                       (PMU_AP_CFG_MONITOR_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_CFG_MONITOR_WPAN_WLAN                  (PMU_AP_CFG_MONITOR_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WPAN | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_CFG_MONITOR(subsys)                    (PMU_AP_CFG_MONITOR_CMD | \
                                                       PMU_AP_MSG_SUBSYS(subsys))

/* Force the indicated subsystem(s) into monitor mode */
#define PMU_AP_MSG_SUBSYS_FAILURE_CMD                 (0x4 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_SUBSYS_FAILURE_WPAN                (PMU_AP_MSG_SUBSYS_FAILURE_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WPAN)
#define PMU_AP_MSG_SUBSYS_FAILURE_WLAN                (PMU_AP_MSG_SUBSYS_FAILURE_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYS_FAILURE_WPAN_WLAN           (PMU_AP_MSG_SUBSYS_FAILURE_CMD | \
                                                       PMU_AP_MSG_SUBSYS_WPAN | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYSTEM_SUBSYS_FAILURE(subsys)   (PMU_AP_MSG_SUBSYS_FAILURE_CMD | \
                                                       PMU_AP_MSG_SUBSYS(subsys))
/* Acknowledge WLBT PMU to Host IPC interrupt has been handled.
 * WLBT will clear the CFG_REQ interrupt to the host */
#define PMU_AP_MSG_WLBT_IPC_ACK                       (0x5 << PMU_AP_MSG_SUBSYS_BITS)

/* Response to PCIE_OFF_REQ. And PCIE_OFF_REJECT_CANCEL */
#define PMU_AP_MSG_WLBT_PCIE_OFF_ACCEPT               (0x7 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_WLBT_PCIE_OFF_REJECT               (0x8 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_WLBT_PCIE_OFF_REJECT_CANCEL        (0x9 << PMU_AP_MSG_SUBSYS_BITS)

#ifdef CONFIG_SCSC_XO_CDAC_CON
#define PMU_AP_MSG_DCXO_CONFIG                        (0xb << PMU_AP_MSG_SUBSYS_BITS)
#endif
/* Request to start and recover scan2mem scandump from RAMSD
 * In Redwood, BLK_WLBT is dumped then BLK_WCPU and BLK_PCIE get reset during the dump.
 * So have to sepearte full dump(XXX_0) and without BLK_WLBT dump(XXX_1)
 */
#define PMU_AP_MSG_SCAN2MEM_DUMP_START_0              (0xc << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_SCAN2MEM_RECOVERY_START_0          (0xd << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_SCAN2MEM_DUMP_START_1              (0xe << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_SCAN2MEM_RECOVERY_START_1          (0xf << PMU_AP_MSG_SUBSYS_BITS)

/* Ack for dynamic link change request */
#define PMU_AP_MSG_LINK_CHANGE_SUCCESS                 (0x10 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_LINK_CHANGE_ERROR                   (0x11 << PMU_AP_MSG_SUBSYS_BITS)

/*****************************/
/* Messages to host from PMU */
/*****************************/

/* The indicated subsystem has failed */
#define PMU_AP_MSG_SUBSYS_ERROR_IND                   (0x1 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_SUBSYS_ERROR_WPAN                  (PMU_AP_MSG_SUBSYS_ERROR_IND | \
                                                       PMU_AP_MSG_SUBSYS_WPAN)
#define PMU_AP_MSG_SUBSYS_ERROR_WLAN                  (PMU_AP_MSG_SUBSYS_ERROR_IND | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYS_ERROR_WLAN_WPAN             (PMU_AP_MSG_SUBSYS_ERROR_IND | \
                                                       PMU_AP_MSG_SUBSYS_WPAN | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYS_ERROR(subsys)               (PMU_AP_MSG_SUBSYS_ERROR_IND | \
                                                       PMU_AP_MSG_SUBSYS(subsys))

/* The last requested command from the host has been processed and is complete
 * from the PMUs perspective */
#define PMU_AP_MSG_SECOND_BOOT_COMPLETE_IND           (0x0 << PMU_AP_MSG_SUBSYS_BITS)

#define PMU_AP_MSG_COMMAND_COMPLETE_IND               (0x2 << PMU_AP_MSG_SUBSYS_BITS)

#define PMU_AP_MSG_PCIE_OFF_REQ                       (0x3 << PMU_AP_MSG_SUBSYS_BITS)

#if defined(CONFIG_WLBT_SPLIT_RECOVERY)
/* Subsystem recovery has been completed */
#define PMU_AP_MSG_SUBSYS_RECOVER_IND                 (0x4 << PMU_AP_MSG_SUBSYS_BITS)
#define PMU_AP_MSG_SUBSYS_RECOVER_WPAN                (PMU_AP_MSG_SUBSYS_RECOVER_IND | \
                                                       PMU_AP_MSG_SUBSYS_WPAN)
#define PMU_AP_MSG_SUBSYS_RECOVER_WLAN                (PMU_AP_MSG_SUBSYS_RECOVER_IND | \
                                                       PMU_AP_MSG_SUBSYS_WLAN)
#define PMU_AP_MSG_SUBSYS_RECOVER(subsys)             (PMU_AP_MSG_SUBSYS_RECOVER_IND | \
                                                       PMU_AP_MSG_SUBSYS(subsys))
#endif

#define PMU_AP_MSG_LINK_CHANGE                        (0x5 << PMU_AP_MSG_SUBSYS_BITS)

#endif /* PMU_HOST_IF_H__ */
